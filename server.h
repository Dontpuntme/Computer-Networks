#ifndef SERVER_H_
#define SERVER_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

class Server {

public:
    Server(int port, int rateRequests, int rateSeconds, int maxUsers, int timeOut);
    virtual ~Server(); 
};

#endif