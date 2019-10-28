#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

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
    char buffer[2048];
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

public:
    Server(int portNo, int rateRequests, int rateSeconds, int maxUsers, int timeOut);
    void Recieve();
    void Return();
    ~Server(); 
};

#endif