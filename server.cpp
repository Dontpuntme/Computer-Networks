#include "server.h"

/* Constructor, creates server socket and attempts to accept client socket */
Server::Server(int portNo, int rateRequests, int rateSeconds, int maxUsers, int timeOut){
    if (sockfd = socket(AF_INET, SOCK_STREAM, 0) < 0) {
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

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        perror("Erorr binding socket");
        exit(1);
    }

    if (listen(sockfd, 5) < 0 ) {
        perror("Error listening to socket");
        exit(1);
    }

    clilen = sizeof(cli_addr);

    if (newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen) < 0) {
        perror("Error accepting socket");
        exit(1);
    }
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