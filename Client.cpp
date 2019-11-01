#include "Client.h"

Client::Client(char* x, int portNo) { 
    file = x;
    port = portNo;
}

int Client::runClient() {
    int valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM,0))<0) {
        perror("Socket Creation error");
        exit(1);
    }

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
    { 
        perror("Invalid address/ Address not supported"); 
        exit(1);
    } 
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {   
        perror("Connection Failed"); 
        exit(1); 
    } 

    write(sock, file, strlen(file)); /* not sure if this should be write or send */
    printf("Sent message to server: %s\n", file); 
    valread = read(sock , buffer, 1024); 
    printf("Recieved response from server: %s\n",buffer); 

    return 0;
}


Client::~Client() {
    /* free stuff */
    close(sock);
}


int main(int argc, char** argv) {
    return 0;
}