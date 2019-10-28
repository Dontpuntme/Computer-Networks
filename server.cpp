#include "server.h"

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

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
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

/* Write information into connected socket */
void Server::Return() {
    if (write(newsockfd, buffer, sizeof(buffer)-1) < 0) {
        perror("Error writing to socket");
    }
}


Server::~Server() {
    /* free stuff */
    close(newsockfd);
    close(sockfd);
}