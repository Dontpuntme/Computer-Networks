#ifndef CLIENT_H_
#define CLIENT_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <string.h>

class Client {
protected:
    char* file;
    int port;
    char* address;
    int sock;
    struct sockaddr_in serv_addr;
public:
    Client(char* f, int portNo, char* addr);
    int runClient();
    ~Client();
};

#endif