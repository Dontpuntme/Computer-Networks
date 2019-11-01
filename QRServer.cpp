#include "QRServer.h"

#define DEFAULT_PORT 2012
#define DEFAULT_RATE_REQUESTS 3
#define DEFAULT_RATE_SECONDS 60
#define DEFAULT_MAX_USERS 3
#define DEFAULT_TIMEOUT 80

/* Constructor, creates server socket and attempts to accept client socket */
Server::Server(int portNo, int rateRequests, int rateSeconds, int maxUsers, int timeOut){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }
    
    port = portNo;
    users = maxUsers;
    time = timeOut;
    rr = rateRequests;
    rs = rateSeconds;

    clients = (struct clientInfo *)malloc(sizeof(struct clientInfo) * maxUsers);

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portNo);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { 
        perror("Error binding socket");
        exit(1);
    }
    printf("Server socket bind success\n");

    if (listen(sockfd, 5) < 0 ) {
        perror("Error listening to socket");
        exit(1);
    }
    printf("Server socket listen success\n");

    clilen = sizeof(cli_addr);
}

/* Accept incoming connections */
void Server::Accept() {
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) {
        perror("Error accepting socket");
        exit(1);
    }
    printf("Server socket accept success\n");
}

/* Accept incoming connections for specific client index*/
void Server::Accept(int idx) {
    struct sockaddr_in curr_addr;
    socklen_t curr_len;
    curr_addr = clients[idx].cli_addr;
    curr_len = sizeof(curr_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&curr_addr, &curr_len);
    if (newsockfd < 0) {
        perror("Error accepting socket");
        exit(1);
    }
    printf("Server socket accept success\n");
}

/* Read information from connected socket into buffer */
void Server::Recieve() {
    bzero(buffer, sizeof(buffer));
    if (read(newsockfd, buffer, sizeof(buffer)-1) < 0) {
        perror("Error reading from socket");
    }
}

/* Read information from connected socket into buffer for specific client index*/
void Server::Recieve(int idx) {
    bzero(clients[idx].buffer, sizeof(clients[idx].buffer));
    if (read(newsockfd, clients[idx].buffer, sizeof(clients[idx].buffer)-1) < 0) {
        perror("Error reading from socket");
    }
}

/* Write information into connected socket */
void Server::Return() {
    if (write(newsockfd, buffer, sizeof(buffer)-1) < 0) {
        perror("Error writing to socket");
    }
}

/* Write information into connected socket for specific client index*/
void Server::Return(int idx) {
    if (write(newsockfd, clients[idx].buffer, sizeof(clients[idx].buffer)-1) < 0) {
        perror("Error writing to socket");
    }
}

/* Helper which handles client interaction from accept to return */
void Server::Handle_Client() {
    Server::Accept();
    Server::Recieve();
    Server::Return();
}

/* Helper which handles client interaction from accept to return */
void Server::Handle_Client(int idx) {
    
    Server::Accept(idx);
    Server::Recieve(idx);
    Server::Return(idx);
}

Server::~Server() {
    /* free stuff */
    //free(clients);
    close(newsockfd);
    close(sockfd);
}

int main(int argc, char** argv) {
    /* TODO parse arguments in form ./QRServer --PORT=2050 ... with getopt() */
    int port, rateRequests, rateSeconds, maxUsers, timeOut;
    int curr_users;
    char testMessage[] = "Hello\n";
    char testMessage2[] = "Hello2\n";
    char testMessage3[] = "Hello3\n";
    
    // TODO refactor input parsing to use getopt() instead of ordered like this
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
    
    Server server = Server(port, rateRequests, rateSeconds, maxUsers, timeOut);
 
    return 0;
}