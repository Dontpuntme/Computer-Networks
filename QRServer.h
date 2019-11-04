#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include <sys/types.h> 
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>

/* struct for storing info for server clients */
struct clientInfo {
    struct sockaddr_in cli_addr;
    char* clientData; /* determine size by reading first 32 bits of buffer recieved */
    char clientResponse[2048];
};

class Server {
protected:
    /* input arg attributes */
    int rr;
    int rs;
    int users;
    int time;

    /* socket-related attributes */
    int sockfd;
    int newsockfd;
    int port;

    /* array of client info structs -- can store up to max_users  */
    struct clientInfo* clients;

    /* old fields for accept/recieve/return without arguments -- can mostly ignore */
    char buffer[2048]; 
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

public:
    /* leaving these thread-fields public for now, will revisit later */
    int thread_idx;
    int oldest_thread;
    pthread_t** threads;

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
    void ProcessQRCode(char* filename, int idx);
    static void *Client_Thread(void *context);
    ~Server(); 
};

/* struct for passing to threads created for processing clients */
struct threadArgs {
    Server* s;
    int idx;
};

#endif