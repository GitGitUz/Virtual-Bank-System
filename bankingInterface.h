#ifndef BANKINGINTERFACE_H
#define BANKINGINTERFACE_H

#include <pthread.h>
#include <ctype.h>

    typedef enum command_state_ {
        cs_create,
        cs_serve,
        cs_deposit,
        cs_withdraw,
        cs_query,
        cs_end,
        cs_quit,
        cs_notquit,
    } command_state;

    typedef struct account_ {
        char name[255];
        float balance;
        int service_flag;              // -1: Not created, 1: In service,  0: Not in service
        pthread_mutex_t lock;
    } account;

    int isamount(char * amount);

    command_state determineCommand(char * command);

    void initAccounts(account ***arr_account);

    // searches bank for account.
    // return < 0 if not found. And index number if found.
    int searchAccount(account ** arr_account, char * name);

    void accCreate(int client_socket_fd, int *numAccount, account ***arr_account, char * name, int *session, pthread_mutex_t * openAuthLock);

    void accServe(int client_socket_fd,  account ***arr_account, char * name, int *session);

    void accDeposit(int client_socket_fd, account ***arr_account, char * amount, int *session);

    void accWithdraw(int client_socket_fd, account ***arr_account, char * amount, int *session);

    void accQuery(int client_socket_fd,  account ***arr_account, int *session);

    void accEnd(int client_socket_fd, account ***arr_account,  int *session);

#endif
