#include "Client.h"
#include "getopt.h"
#include <iostream>

#define DEFAULT_PORT 2012
#define DEFAULT_ADDRESS "127.0.0.1"

Client::Client(char* x, int portNo) { 
    file = x;
    port = portNo;
}

int Client::runClient() {
    int valread;
    
    char buffer[1024] = {0};
    char bufferTwo[1024] ={0};

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
    FILE * fp;
    fp = fopen(file,"r+");
    fscanf(fp,bufferTwo,'c');
    fclose (fp);
    int x = strlen(bufferTwo);
    write(sock, strncat((char*)x,bufferTwo,sizeof(x)+sizeof(bufferTwo)), strlen(file)); /* not sure if this should be write or send */
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
    /* should take image file, port, and server ip (later two have defaults as seen at top) */
    int port = DEFAULT_PORT;
    std::string servAddress = DEFAULT_ADDRESS;
    std::string file;
    bool fileFlag = false;
    const char* const short_opts = "f:p:a:";
    const option long_opts[] = {
        {"FILE", required_argument, nullptr, 'f'},
        {"PORT", required_argument, nullptr, 'p'},
        {"ADDRESS", required_argument, nullptr, 'a'},
    };

    while(true)
    {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
        if(-1 == opt)
        break;
         switch (opt)
        {
        case 'p':
            port = std::stoi(optarg);
            std::cout << "Port set to: " << port << std::endl;
            break;

        case 'f':
            file = std::string(optarg);
            std::cout << "File set to: " << file << std::endl;
            fileFlag = true;
            break;

        case 'a':
            servAddress = std::string(optarg);
            std::cout << "Address set to: " << servAddress << std::endl;
            break;
        case '?': // Unrecognized option
        default:
            break;
        }
        
    }
    if(!fileFlag)
        {
            printf("Error no file given\n");
            return -1;
        }
    return 0;
}