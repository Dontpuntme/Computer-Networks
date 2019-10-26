#include "client.h"
#define PORT 2050
Client::Client(char* x)
{ 
char* file = x;
}
int Client::runClient()
{
int sock, valread;
struct sockaddr_in serv_addr;
char buffer[1024] = {0};

if((sock = socket(AF_INET, SOCK_STREAM,0))<0)
{
    printf("\n Socket Creation error");
    return -1;
}
serv_addr.sin_family = AF_INET; 
serv_addr.sin_port = htons(PORT); 
   
 if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
{ 
    printf("\nInvalid address/ Address not supported \n"); 
    return -1; 
} 
   
if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
{   printf("\nConnection Failed \n"); 
    return -1; 
} 
    send(sock , buffer , strlen(file) , 0 ); 
    printf("Hello message sent\n"); 
    valread = read( sock , buffer, 1024); 
    printf("%s\n",buffer ); 

return 0;

}