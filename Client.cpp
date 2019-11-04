#include "Client.h"
#include "getopt.h"
#include <iostream>

#define DEFAULT_PORT 2012
#define DEFAULT_ADDRESS "127.0.0.1"

Client::Client(char* f, int portNo, char* addr) { 
    file = f;
    port = portNo;
    address = addr;
}

int Client::runClient() {
    int valread;
    
    char readBuffer[1024]; /* reading back input from server, URL + return code */
    char* sendBuffer; /* 32 bit int + file data */
    char* fileData; /* file data */

    if ((sock = socket(AF_INET, SOCK_STREAM,0))<0) {
        perror("Socket Creation error");
        exit(1);
    }

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
    
    if (inet_pton(AF_INET, address, &serv_addr.sin_addr)<=0) 
    { 
        perror("Invalid address/ Address not supported"); 
        exit(1);
    } 
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {   
        perror("Connection Failed"); 
        exit(1); 
    } 
    printf("Connected to socket!\n");


    uint32_t filesize; /* 32 bit filesize (4 bytes) */
    FILE* fp;
    int symbol;
    if ((fp = fopen(file,"r")) != NULL) {
        if (fseek(fp, 0L, SEEK_END) == 0) {
            filesize = ftell(fp); /* get buffer size */
            fileData = (char *)malloc(sizeof(char) * filesize + 1);
            if (fileData < 0) { /* file too big */
                perror("Invalid filesize");
                exit(1);
            }
            if (fseek(fp, 0L, SEEK_SET) != 0) { /* go back to start to read into buf */
                size_t newlen = fread(fileData, sizeof(char), filesize, fp);
                if (ferror(fp) != 0) { perror("Error reading file");}
                else {fileData[newlen++] = '\0';} 
            }
        }
        fclose(fp);
    }
    else {
        perror("File read error");
    }

    printf("size: %d\n", filesize);

    /* creating buffer to send */
    sendBuffer = (char*)malloc(sizeof(char) * (filesize + 4 + 1));
    strncpy(sendBuffer, (char*)filesize, 4);
    strncat(sendBuffer, fileData, filesize + 1);

    /* send message and recieve response */
    write(sock, sendBuffer, filesize + 4 + 1);
    printf("Sent file to server: %s\n", file); 
    valread = read(sock , readBuffer, 1024); 
    printf("Recieved response from server: %s\n",readBuffer); 

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
            printf("Error: no file given\n");
            return -1;
        }

    Client client = Client((char*)file.c_str(), port, (char*)servAddress.c_str()); /* should probably rewrite using only char*, unless we really want std::string */
    printf("Client initialized\n");
    client.runClient();
    //client.~Client;
    return 0;
}