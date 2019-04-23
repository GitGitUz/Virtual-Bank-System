# Virtual-Bank-System

//this program implements a client/server system to create a virtual bank that can deal with many users and perform different tasks

Two executables are created
  bankingServer
    - takes 1 command arguments (port number of 9001)
        e.g. ./bankingServer 9001
  bankingClient
    - takes 2 command line arguments, a valid machine that hosts the server and port 9001
        e.g ./bankingServer composite.cs.rutgers.edu 9001 )
1. Run the make command to produce the two executables.
2. Run the make clean command to remove any unwanted object files.
3. Open a terminal to run the server on and run ./bankingServer 9001
4. Open another terminal to run the client on and run ./bankingClient [host name] 9001

Project
  - Most of the banking and server functionalities run separately
  - Banking functionalities in bankingInterface.h bankingInterface.c
  - Server and client code in bankingServer.c and bankingClient.c

Thread Synchronization
  Client: 2 Threads
    - For user-server and server-user processes.

  Server: x threads for x clients in the server side
    - Used mutex locks in each account to synchronize the access of each account. Only one account can be serviced as a time.
    - Used mutex lock in the open command function (accCreate) so only one client can add an account at a time.
    - One extra thread is used to print all bank account balances on the server side every 15 seconds.

Client Side
  - pthread_create() was used to create the new server-user thread, while the user-server thread runs in the main process thread.
  - For every write and read, the validity of the connection is checked. In case the server shuts down, the client will notify the user     the bank has closed and stop the entire process.

Note:If client shuts down due to SIGINT (ctrl-c) a SIGNAL HANDLER FUNCTION is used to send the exit command to the server so that the account can properly be closed before the client process ends.

Server Side
  - x threads created for x client connections
  - Each newly created thread is rerouted to the function clientServiceFunction where it reads input continuously to call appropriate       functions and send back data through the client socket.
- Additionally, another thread prints out to the bank server the account information every 20 seconds.
- The main component of the client-service thread is a switch statement. Also, it is very important that we make a local variable         session for each thread to determine if any session is available and if any, which account is being accessed.

// session state < 0 if not in session,
// session state >= 0 if in session (specifying location of account
// initialize session: int session = -1;

Bank
  - bankingInterface.h and bankingInterface.c are where most bank related structures and functions are used.
