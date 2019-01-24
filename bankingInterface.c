#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#include "bankingInterface.h"


int isamount(char * amount){
    int periodCount = 0;
    int len = strlen(amount);
    int i;
    for(i = 0 ; i < len ; i++){
        if(isdigit(amount[i]) == 0){
            if(amount[i] == '.'){
                if(periodCount++ > 1){
                    return 0;
                }
            }else{
                return 0;
            }
        }
    }
    return 1;
}

command_state determineCommand(char * command){
    int i;
    for(i = 0; i < strlen(command); i++){
        command[i] = tolower(command[i]);
    }

    if(strcmp(command,"create") == 0){
        return cs_create;
    }else if(strcmp(command,"serve") == 0){
        return cs_serve;
    }else if(strcmp(command,"deposit") == 0){
        return cs_deposit;
    }else if(strcmp(command,"withdraw") == 0){
        return cs_withdraw;
    }else if(strcmp(command,"query") == 0){
        return cs_query;
    }else if(strcmp(command,"end") == 0){
        return cs_end;
    }else if(strcmp(command,"quit") == 0){
        return cs_quit;
    }else{
      return cs_notquit;
    }

    return cs_notquit;
}


void initAccounts(account ***arr_account){
    *arr_account = (account ** ) malloc(sizeof(account *)*1000);
    int i;
    for(i = 0 ; i < 1000 ; i++){
        (*arr_account)[i] = (account *) malloc(sizeof(account));
        // initialize all accounts as not created
        ((*arr_account)[i])->service_flag = -1;
        // initialize each account's lock
        pthread_mutex_init(&(((*arr_account)[i])->lock), NULL);
    }
    return;
}

// searches bank for account.
// return < 0 if not found. And index number if found.
int searchAccount(account ** arr_account, char * name){
    int i;
    for(i = 0 ; i < 1000 ; i++){
        if(strcmp( (arr_account[i])->name ,name) == 0){
          return i;
        }
    }
    return -1;
}

// need to add a mutex lock for open()
void accCreate(int client_socket_fd, int *numAccount, account ***arr_account, char * name, int * session, pthread_mutex_t * openAuthLock){
    // lock the openAuthLock
    pthread_mutex_lock(openAuthLock);
    // init buffer
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));
    // supply name of account
    if(strlen(name) <= 0){
      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Must specify name of account\n") ) < 0 ){
          printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
          pthread_mutex_unlock(openAuthLock);
          exit(1);
      }
      pthread_mutex_unlock(openAuthLock);
      return;
    }
    // session is active. user can't create an account while session is active.
    if(*session >= 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Already in session. Can't open an account: %s\n",name) ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            pthread_mutex_unlock(openAuthLock);
            exit(1);
        }
        pthread_mutex_unlock(openAuthLock);
        return;
    }
    // max capacity reached
    if(*numAccount >= 1000){
      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Bank has reached capacity. Can't open any new accounts: %s\n",name) ) < 0 ){
          printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
          pthread_mutex_unlock(openAuthLock);
          exit(1);
      }
      pthread_mutex_unlock(openAuthLock);
      return;
    }
    // account exists already
    if(searchAccount(*arr_account,name) >= 0 ){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Account exists already: %s\n",name) ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            pthread_mutex_unlock(openAuthLock);
            exit(1);
        }
        pthread_mutex_unlock(openAuthLock);
        return;
    }
    // create this account // 1
    account * client_account = (*arr_account)[*numAccount];

    strcpy(client_account->name, name);   // init account name (CASE sensitive)
    client_account->balance = 0.0;       // 0 balance initialize
    client_account->service_flag = 0;    // not in session

    if ( write(client_socket_fd, obuffer, sprintf(obuffer, "- - -Account has been created: %s - - -\n",name) ) < 0 ){
        printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
        pthread_mutex_unlock(openAuthLock);
        exit(1);
    }

    // update # of accounts
    *numAccount = *numAccount + 1;
    // release lock for other clients to use
    pthread_mutex_unlock(openAuthLock);
    return;

}

void accServe(int client_socket_fd,  account ***arr_account, char * name, int *session){
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));
    // check if name is given
    if(strlen(name) <= 0){
      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Must specify name of account\n") ) < 0 ){
          printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
          exit(1);
      }
      return;
    }
    // session is active. user can't create an account while session is active.
    if(*session >= 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Already in session. Can't open another seesion: %s\n",name) ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    // account does not exist
    if( (*session = searchAccount(*arr_account,name)) < 0 ){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Account not found: %s\n",name) ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    // identify account
    account * client_account = (*arr_account)[*session];

    // if account, exists lock account session
    // if account is locked already, current client will need to wait
    if( pthread_mutex_trylock(&(client_account->lock)) != 0 ){   //pthread_mutex_lock(&(client_account->lock));
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "- - -Please try again later. The account is currently being used: %s- - -\n",name) ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        *session = -1;    // session was not started
          return;
    }

    // in session
    client_account->service_flag = 1;


    if ( write(client_socket_fd, obuffer, sprintf(obuffer, "- - -Your session has begun for account: %s - - -\n",name) ) < 0 ){
        printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
        exit(1);
    }

    return;

}

void accDeposit(int client_socket_fd, account ***arr_account, char * amount, int *session){
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));

    // check if name is given
    if(strlen(amount) <= 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Must specify amount to deposit.\n") ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }
    // if string is not a float
    if(!isamount(amount)){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: You must specify a non-negative numerical amount to deposit.\n") ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    // check if session exists
    if(*session < 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: No session exists.\n") ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    float creditval = atof(amount);

    account * client_account = (*arr_account)[*session];
    client_account->balance += creditval;

    if ( write(client_socket_fd, obuffer, sprintf(obuffer, "- - -The amount has been into your account- - -\n") ) < 0 ){
        printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
        exit(1);
    }

    return;

}

void accWithdraw(int client_socket_fd, account ***arr_account, char * amount, int *session){
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));

    // check if amount is given
    if(strlen(amount) <= 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Must specify amount to withdraw.\n") ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }
    // if string is not a float
    if(!isamount(amount)){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: You must specify a non-negative numerical amount to withdraw.\n") ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    // check if session exists
    if(*session < 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: No session exists.\n") ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    float debitval = atof(amount);

    account * client_account = (*arr_account)[*session];
    // check that withdrawal <= balance
    if(debitval > client_account->balance){
      if( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: You cannot withdraw more than your balance.\n") ) < 0 ){
          printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
          exit(1);
      }
      return;
    }

    client_account->balance -= debitval;

    if( write(client_socket_fd, obuffer, sprintf(obuffer, "- - -The amount has been withdrawn from your account.- - -\n") ) < 0 ){
        printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
        exit(1);
    }

    return;

}

void accQuery(int client_socket_fd,  account ***arr_account, int *session){
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));

    // check if session exists
    if(*session < 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: No session exists.\n") ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    // get balance
    float balance = (*arr_account)[*session]->balance;

    if ( write(client_socket_fd, obuffer, sprintf(obuffer, "- - -Your balance is: %f.- - -\n",balance) ) < 0 ){
        printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
        exit(1);
    }


    return;


}

void accEnd(int client_socket_fd, account ***arr_account,  int *session){
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));
    // check if session exists
    if(*session < 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: No session exists.\n") ) < 0 ){
            printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    account * client_account = (*arr_account)[*session];
    char * name = client_account->name;

    if ( write(client_socket_fd, obuffer, sprintf(obuffer, "- - -You have finised your account session: %s- - -\n",name) ) < 0 ){
        printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
        exit(1);
    }

    // change service flag
    client_account->service_flag = 0;
    // change session flag
    *session = -1;

    pthread_mutex_unlock(&(client_account->lock));

    return;

}
