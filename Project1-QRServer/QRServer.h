#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/socket.h>
#include <pthread.h>
#include <vector>
#include <netinet/in.h>

/* global thread-related fields */
pthread_t** threads; /* list of pointers to threads */
int numThreads; /* protect with mutex */
int* threadStatus; /* whether or not thread is done */

/* taking care of synchronization issues */
pthread_mutex_t mutex;


/* struct for storing info for server clients */
struct clientInfo {
    int cli_sockfd;
    uint32_t filesize;
    struct sockaddr_in cli_addr;
    char* clientData; /* determine size by reading first 32 bits of buffer recieved */
    char clientResponse[1024];
};

/* struct for storing timestamps/ip for rate limiting */
struct entry {
    struct tm time;
    struct sockaddr_in addr;
};

class Server {
protected:
    /* input arg attributes */
    int rr;
    int rs;
    int users;
    int time;

    /* for timestamps */
    std::vector<entry> entries;

    /* socket-related attributes */
    int sockfd;
    int port;

    /* old fields for accept/recieve/return without arguments -- can mostly ignore */
    char buffer[2048]; 
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

public:
    struct clientInfo* clients; /* array of client info structs -- can store up to max_users  */

    /* methods */
    Server(int portNo, int rateRequests, int rateSeconds, int maxUsers, int timeOut);
    void Accept();
    void Accept(int idx);
    void Recieve();
    void Recieve(int idx);
    void Return();
    void Return(int idx);
    void Handle_Client();
    void Handle_Client(int idx);
    void Reject(); 
    void ProcessQRCode(char* filename, int idx);
    static void *Client_Thread(void *context);
    void Write_Text_To_Log_File(int idx, const char* message);
    ~Server(); 
};

/* struct for passing to threads created for processing clients */
struct threadArgs {
    Server* s;
    int idx;
};

#endif