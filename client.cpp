#include "client.h"

Client::Client(char* x, int portNo) { 
    file = x;
    port = portNo;
}

int Client::runClient() {
    int valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM,0))<0) {
        printf("\n Socket Creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {   
        printf("\nConnection Failed \n"); 
        return -1; 
    } 

    write(sock, file, sizeof(file)); /* not sure if this should be write or send */
    printf("Hello message sent\n"); 
    valread = read(sock , buffer, 1024); 
    printf("%s\n",buffer); 

    return 0;
}


Client::~Client() {
    /* free stuff */
    close(sock);
}