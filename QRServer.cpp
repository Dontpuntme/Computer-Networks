#include "QRServer.h"
#include "getopt.h"
#include <iostream>
#include <fstream>
#include "string.h"
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

    thread_idx = 0; /* public field to keep track of what tread to put next client on */
    oldest_thread = 0;
    
    port = portNo;
    users = maxUsers;
    time = timeOut;
    rr = rateRequests;
    rs = rateSeconds;

    threads = (pthread_t **)malloc(sizeof(pthread_t *)*users); /* allocate array of pthread pointers, will allocate actual threads as needed */
    available = (int *)malloc(sizeof(int)*maxUsers);

    clients = (struct clientInfo *)malloc(sizeof(struct clientInfo) * maxUsers);
    for (int i=0; i<maxUsers; i++) {
        available[i] = 1; /* mark all client structs as inactive */
    }

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

/* Accept incoming connections for specific client index*/
void Server::Accept(int idx) {
    struct sockaddr_in curr_addr;
    socklen_t curr_len;
    curr_addr = clients[idx].cli_addr;
    curr_len = sizeof(curr_addr);
    clients[idx].cli_sockfd = accept(sockfd, (struct sockaddr *)&curr_addr, &curr_len);
    if (clients[idx].cli_sockfd < 0) {
        perror("Error accepting socket");
        exit(1);
    }
    printf("Server socket accept success\n");
    clients[idx].cli_addr = curr_addr; /* record client ip address */
    const char * message = "Client Accepted";
    Write_Text_To_Log_File(idx,message);
}

/* Read information from connected socket into buffer for specific client index*/
void Server::Recieve(int idx) {
    uint32_t filesize = 0;
    if (read(clients[idx].cli_sockfd, &filesize, 4) < 0) { perror("error finding filesize");}
    printf("File size found to be: %d\n", filesize);
    clients[idx].filesize = filesize;
    clients[idx].clientData = (char* )malloc(sizeof(char) * filesize); /* note that we may need to add 4 to account for filesize */
    bzero(clients[idx].clientData, filesize);
    if (read(clients[idx].cli_sockfd, clients[idx].clientData, filesize) < 0) {
        perror("Error reading from socket");
    }
    const char * message = "Received Message From Client";
    Write_Text_To_Log_File(idx,message);
}

/* Write information into connected socket for specific client index*/
void Server::Return(int idx) {
    if (write(clients[idx].cli_sockfd, clients[idx].clientResponse, sizeof(clients[idx].clientResponse)-1) < 0) {
        perror("Error writing to socket");
    }
    const char * message = "Responded to Client";
    Write_Text_To_Log_File(idx,message);
}

/* Immediately accept/reject client (deals with maxUsers/ratelimit) */
void Server::Reject() {
    struct sockaddr_in curr_addr;
    socklen_t curr_len;
    int rejectfd;
    char* response = (char*)"Client is either too busy or you have gone over your use limit. Try again later\n";
    if ((rejectfd = accept(sockfd, (struct sockaddr *)&curr_addr, &curr_len) < 0)) {perror("Error accepting bad client");};
    if (write(rejectfd, response, sizeof(response) < 0)) {
        perror("Error writing to socket");
    }
    close(rejectfd);
}

/* Helper which handles client interaction from accept to return */
void Server::Handle_Client(int idx) {
    char idxchar = (char) (idx + 48);
    char* filename = (char*)malloc(sizeof(char)*12);
    asprintf(&filename, "QR%c.png", idxchar);
    Server::Accept(idx);
    Server::Recieve(idx);
    Server::ProcessQRCode(filename, idx); /* TODO figure out if we should deal with multiple filenames or just have mutexes */
    Server::Return(idx);
    free(filename);
    close(clients[idx].cli_sockfd);
    printf("Finished handling client index %d\n", idx);
}
void Server::Write_Text_To_Log_File(int idx, const char* message)
{

    FILE * pFile;
    pFile = fopen ("log.txt","a");
    time_t now = std::time(0);
    struct tm *timeinfo = localtime(&now);
    fprintf(pFile,"%d-%d-%d %d:%d:%d %d.%d.%d.%d %s\n",timeinfo->tm_year+1900,timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,
    timeinfo->tm_min,timeinfo->tm_sec, int(clients[idx].cli_addr.sin_addr.s_addr&0xFF),(int(clients[idx].cli_addr.sin_addr.s_addr&0xFF00)>>8),
  (int(clients[idx].cli_addr.sin_addr.s_addr&0xFF0000)>>16),(int(clients[idx].cli_addr.sin_addr.s_addr&0xFF000000)>>24), message);
    fclose(pFile);
}
void Server::ProcessQRCode(char* filename, int idx) {
    //filename qrcode.jpeg
    printf("Processing QR code, filename: %s\n", filename);
    printf("Client image file size: %d\n", clients[idx].filesize);
    clients[idx].clientData = clients[idx].clientData; /* add 4 to ignore first 4 bytes (filesize) */
    char inBuffer[1000];

    /* write image file locally for use in jar file QR interpreter */
    FILE * fp;
    fp = fopen(filename, "wb+");
    std::ofstream output(filename, std::ios::binary);
    output.write(clients[idx].clientData, clients[idx].filesize);
    output.close();
    //fwrite(clients[idx].clientData, clients[idx].filesize, 1, fp);
    //fclose (fp);

    /* run java program for QR code */
    FILE* progOutput; // a file pointer representing the popen output
    char* cmd; /* string of java command */
    char* base = (char *)"java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner";
    asprintf(&cmd, "%s %s", base, filename);
    printf("Running command: %s\n", cmd);
    progOutput = popen(cmd, "r");
 
    // make sure that popen succeeded
    if(!progOutput) {
        perror("popen failed");
        exit(1);
    }
 
    // reset buffer to all NULLS 
    memset(inBuffer, (char)NULL, sizeof(inBuffer));
 
    // read the popen output into the char array buffer
    if(fread(inBuffer, sizeof(char), sizeof(char) * sizeof(inBuffer), progOutput) < 0) {
        perror("fread failed");
        exit(1);
    }

    // close the file pointer representing the popen output
    if(pclose(progOutput) < 0) {
        perror("pclose failed");
        exit(1);
    }

    int lineCounter = 0;
    int z = 0;
    memset(clients[idx].clientResponse, (char)NULL, sizeof(clients[idx].clientResponse));

    for(int n = 0; n < 1000; n++) {
        if(inBuffer[n]=='\n') {
        lineCounter ++;
        }
        else if(lineCounter == 4) {
            clients[idx].clientResponse[z]= inBuffer[n];
            z++;
        }
        if(lineCounter == 5) {
            break;
        }
    }
    printf("Done processing QR code\n");
    free(clients[idx].clientData); /* cleanup memory */
}

/* thread method to call handle_client */
static void *Client_Thread(void *context) {
    struct threadArgs* args = (struct threadArgs *)context;
    /* should probably use mutex around these */
    Server serv = *(args->s);
    int index = args->idx;
    args->s->available[index] = 0; /* in use */
    serv.Handle_Client(index);
    args->s->available[index] = 1; /* done processing */
    printf("Thread %d is now available\n", index);
    return (void*)NULL;
}

Server::~Server() {
    /* make sure to clean up threads */
    for (int i=0; i<users; i++) {
        pthread_join(*(threads[i]), NULL);
    }
    free(clients);
    close(sockfd);
}


int main(int argc, char** argv) {
    /* TODO parse arguments in form ./QRServer --PORT=2050 ... with getopt() */
    int port, rateRequests, rateSeconds, maxUsers, timeOut;
    int pid;
    char testMessage[] = "Hello\n";
    char testMessage2[] = "Hello2\n";
    char testMessage3[] = "Hello3\n";
    timeOut = DEFAULT_TIMEOUT;
    maxUsers = DEFAULT_MAX_USERS;
    rateSeconds = DEFAULT_RATE_SECONDS;
    rateRequests = DEFAULT_RATE_REQUESTS;
    port = DEFAULT_PORT;
    const char* const short_opts = "p:r:u:t:";
    const option long_opts[] = {
        {"PORT", required_argument, nullptr, 'p'},
        {"RATE_MSGS", required_argument, nullptr, 'm'},
        {"RATE_TIME", required_argument, nullptr, 'r'},
        {"MAX_USERS", required_argument, nullptr, 'u'},
        {"TIME_OUT", required_argument, nullptr, 't'},

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

        case 'm':
            rateRequests = std::stoi(optarg);
            std::cout << "rateRequests set to: " << rateRequests << std::endl;
            break;

        case 'r':
            rateSeconds = std::stoi(optarg);
            std::cout << "rateSeconds set to: " << rateSeconds << std::endl;
            break;

        case 'u':
            maxUsers = std::stoi(optarg);
            std::cout << "MaxUsers set to: " << maxUsers << std::endl;
            break;

        case 't':
            timeOut = std::stoi(optarg);
            std::cout << "Port set to: " << timeOut << std::endl;
            break;
        case '?': // Unrecognized option
        default:
            break;
        }
    }

    Server server = Server(port, rateRequests, rateSeconds, maxUsers, timeOut); 

    /* have MaxUsers number of threads running/ready to accept new connections at any given time */
    while (true) {
        if (server.thread_idx < maxUsers) { /* make sure we have at least MaxUers threads running*/
            struct threadArgs* arguments = (struct threadArgs *)malloc(sizeof(struct threadArgs));
            arguments->s = &server;
            printf("Creating thread for index %d\n", server.thread_idx);
            arguments->idx = server.thread_idx;
            server.threads[server.thread_idx] = (pthread_t *)malloc(sizeof(pthread_t));
            pthread_create(server.threads[server.thread_idx], NULL, &Client_Thread, (void *)arguments); 
            server.thread_idx++; /* maybe should do this in Client_Thread function instead */
        }
        else { /* we must reject other incoming connections and check for thread completion */
            for (int i=0; i<maxUsers; i++) {
                printf("Checking if thread %d has finished\n", i);
                if (server.available[i] = 1) { /* loop through clients and re-dispatch completed threads */
                    printf("reclaiming thread %d\n", i);
                    struct threadArgs* arguments = (struct threadArgs *)malloc(sizeof(struct threadArgs));
                    arguments->s = &server;
                    arguments->idx = i;
                    pthread_join(*(server.threads[i]), NULL);
                    printf("joined thread %d\n", i);
                    pthread_create(server.threads[i], NULL, &Client_Thread, (void *)arguments);
                }
            }
        }
        usleep(100000); /* less spinning */
        /* TODO write to log file somewhere */
    }
    /* TODO have MaxUsers+1 thread which accepts other clients and immediately rejects them */
    
    return 0;
}