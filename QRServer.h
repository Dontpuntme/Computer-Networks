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
#include <netinet/in.h>

/* struct for storing info for server clients */
struct clientInfo {
    struct sockaddr_in cli_addr;
    char buffer[2048]; // we should maybe pick and be able to justify a different (maybe larger) buffer size depending on what QR decoder takes in
    // maybe some fields here to keep track of client usage -- make sure they dont make too many requests per time frame
    // ^ this information may be obtained by checking what we have written in the log files
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
    Server(int portNo, int rateRequests, int rateSeconds, int maxUsers, int timeOut);
    void Accept();
    void Accept(int idx);
    void Recieve();
    void Recieve(int idx);
    void Return();
    void Return(int idx);
    void Handle_Client();
    void Handle_Client(int idx);
    ~Server(); 
};

#endif