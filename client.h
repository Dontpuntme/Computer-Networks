#ifndef CLIENT_H_
#define CLIENT_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

class Client {
    Client(int port, char* file);
    virtual ~Client();
};

#endif