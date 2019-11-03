#include "QRServer.h"
#include "Client.h"
#include <stdlib.h>
#include <iostream>

/* Testing program */

#define DEFAULT_PORT 2012
#define DEFAULT_RATE_REQUESTS 3
#define DEFAULT_RATE_SECONDS 60
#define DEFAULT_MAX_USERS 3
#define DEFAULT_TIMEOUT 80

int main(int argc, char** argv) {
    int port, rateRequests, rateSeconds, maxUsers, timeOut;
    int pid;
    char testMessage[] = "Hello\n";
    char testMessage2[] = "Hello2\n";
    
    // TODO refactor input parsing
    if (argc < 6) {
        timeOut = DEFAULT_TIMEOUT;
    }
    else {
        timeOut = atoi(argv[5]);
    }
    if (argc < 5) {
        maxUsers = DEFAULT_MAX_USERS;
    }
    else {
        maxUsers = atoi(argv[4]);
    }
    if (argc < 4) {
        rateSeconds = DEFAULT_RATE_SECONDS;
    }
    else {
        rateSeconds = atoi(argv[3]);
    }
    if (argc < 3) {
        rateRequests = DEFAULT_RATE_REQUESTS;
    }
    else {
        rateRequests = atoi(argv[2]);
    }
    if (argc < 2) {
        port = DEFAULT_PORT;
    }
    else {
        port = atoi(argv[1]);
    }

    if ((pid = fork()) < 0) {
        perror("Fork Error");
        exit(1);
    }
    else if (pid == 0) { /* child process, for managing clients */
        sleep(1);
        Client client = Client(testMessage, port);
        client.runClient();
        client.~Client();
        exit(0);
    }
    else { /* parent process, for managing server */ 
        Server server = Server(port, rateRequests, rateSeconds, maxUsers, timeOut);
        /* TODO rewrite with threads */
        server.Handle_Client(1);

        server.~Server();
    }
    
    //printf("Parsed arguments %d %d %d %d %d\n", port, rateRequests, rateSeconds, maxUsers, timeOut);
    return 1;
}