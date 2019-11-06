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
    FILE * pFile;
    pFile = fopen("log.txt","w");
    time_t now = std::time(0);
    struct tm *timeinfo = (struct tm *)malloc(sizeof(struct tm));
    timeinfo = localtime(&now);
    const char* message = "Server booted up :D";
    fprintf(pFile,"%d-%d-%d %d:%d:%d %s\n",timeinfo->tm_year+1900,timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,
    timeinfo->tm_min,timeinfo->tm_sec, message);
    fclose(pFile);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }

    numThreads = 0;

    port = portNo;
    users = maxUsers;
    time = timeOut;
    rr = rateRequests;
    rs = rateSeconds;

    threads = (pthread_t **)malloc(sizeof(pthread_t *)*users); /* allocate array of pthread pointers, will allocate actual threads as needed */
    threadStatus = (int *)malloc(sizeof(int)* maxUsers);

    clients = (struct clientInfo *)malloc(sizeof(struct clientInfo) * maxUsers);
    for (int i=0; i<maxUsers; i++) {
        threadStatus[i] = 0; /* mark all client structs as inactive */
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
    /* we are fine to accept socket and store info in client struct as usual */
    struct sockaddr_in curr_addr;
    socklen_t curr_len;
    curr_addr = clients[idx].cli_addr;
    curr_len = sizeof(curr_addr);
    clients[idx].cli_sockfd = accept(sockfd, (struct sockaddr *)&curr_addr, &curr_len);
    
    /* handle timeouts */
    struct timeval tv;
    tv.tv_sec = time; /* timeout val */
    setsockopt(clients[idx].cli_sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
    
    /* handle if we should reject */
    if (numThreads >= users) { 
        const char* response = "Server is either too busy or you have gone over your use limit. Try again later\n";
        printf("%s\n", response);
        if (write(clients[idx].cli_sockfd, response, sizeof(response) < 0)) {
            perror("Error writing to socket");
        }
        close(clients[idx].cli_sockfd);
    }
    if (clients[idx].cli_sockfd < 0) {
        perror("Error accepting socket");
        exit(1);
    }
    
    /* handles rate limiting */
    // int i = 0;
    // int recentEntries = 0;
    // int tts=0;//total time seconds
    // int ttsCur=0;//total time seconds current
    // time_t now = std::time(0);
    // struct tm *curTime = (struct tm *)malloc(sizeof(struct tm));
    // curTime = localtime(&now);
    // ttsCur = ((curTime->tm_hour*3600)+(curTime->tm_min*60)+(curTime->tm_sec));
    // struct entry curEntry;
    // curEntry.addr=curr_addr;
    // curEntry.time=*curTime;
    // while(i < entries.size()) {        
    //     if(entries.at(i).addr.sin_addr.s_addr == curr_addr.sin_addr.s_addr) {
    //         tts = ((entries.at(i).time.tm_hour * 3600)+(entries.at(i).time.tm_min*60)+(entries.at(i).time.tm_sec));
    //         if (ttsCur-tts<rs) {
    //             recentEntries++;
    //         }
    //         else {
    //             entries.erase (entries.begin()+i);
    //         }        
    //     }
    //     if (recentEntries >= rr) { /* deny connection */
    //         uint32_t retcode = 3;
    //         const char* failMessage = "Rate limit surpassed";
    //         uint32_t msgLength = strlen(failMessage) + 1;
    //         clients[idx].cli_addr = curr_addr; /* record client ip address */
    //         write(clients[idx].cli_sockfd, &retcode, 4); /* return code */
    //         write(clients[idx].cli_sockfd, &msgLength, 4); /* return code */
    //         write(clients[idx].cli_sockfd, &failMessage, msgLength); /* return code */
    //         const char* message = "Client went over rate message";
    //         Write_Text_To_Log_File(idx,message);
    //         //close(clients[idx].cli_sockfd);
    //         return;
    //     }
    //     i++;
    // }
    // entries.push_back(curEntry);
    printf("Server socket accept success\n");
    clients[idx].cli_addr = curr_addr; /* record client ip address */
    const char* message = "Client Accepted";
    Write_Text_To_Log_File(idx,message);
}

/* Read information from connected socket into buffer for specific client index*/
void Server::Recieve(int idx) {
    uint32_t filesize = 0;
    if (read(clients[idx].cli_sockfd, &filesize, 4) < 0) { /* failed to read because timeout */
        uint32_t retcode = 2;
        const char* failMessage = "Server timeout";
        uint32_t msgLength = strlen(failMessage) + 1;
        write(clients[idx].cli_sockfd, &retcode, 4); /* return code */
        write(clients[idx].cli_sockfd, &msgLength, 4); /* return code */
        write(clients[idx].cli_sockfd, &failMessage, msgLength); /* return code */
        const char* message = "Client interaction timed out";
        Write_Text_To_Log_File(idx,message);
        return;
    }
    printf("File size found to be: %d\n", filesize);
    clients[idx].filesize = filesize;
    if (filesize > UINT32_MAX || filesize < 0) { /* invalid filesize */
        uint32_t retcode = 1;
        uint32_t msgLength = 0;
        write(clients[idx].cli_sockfd, &retcode, 4); /* return code */
        write(clients[idx].cli_sockfd, &msgLength, 4); /* return code */
        const char* message = "Client image file invalid size";
        Write_Text_To_Log_File(idx,message);
        return;
    }
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
    uint32_t retcode = 0; /* success code */
    uint32_t responseSize = strlen(clients[idx].clientResponse);
    if (write(clients[idx].cli_sockfd, &retcode, 4) < 0) {
        perror("Error writing to socket");
    }
    if (write(clients[idx].cli_sockfd, &responseSize, 4) < 0) {
        perror("Error writing to socket");
    }
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
    curr_len = sizeof(curr_addr);
    int rejectfd;
    const char* response = "Client is either too busy or you have gone over your use limit. Try again later\n";
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
    Server::Recieve(idx);
    Server::ProcessQRCode(filename, idx);
    Server::Return(idx);
    free(filename);
    //close(clients[idx].cli_sockfd);
    printf("Finished handling client index %d\n", idx);
}
void Server::Write_Text_To_Log_File(int idx, const char* message) {
    FILE * pFile;
    pFile = fopen ("log.txt","a");
    time_t now = std::time(0);
    struct tm *timeinfo = (struct tm *)malloc(sizeof(struct tm));
    timeinfo = localtime(&now);
    fprintf(pFile,"%d-%d-%d %d:%d:%d %d.%d.%d.%d %s\n",timeinfo->tm_year+1900,timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,
    timeinfo->tm_min,timeinfo->tm_sec, int(clients[idx].cli_addr.sin_addr.s_addr&0xFF),(int(clients[idx].cli_addr.sin_addr.s_addr&0xFF00)>>8),
  (int(clients[idx].cli_addr.sin_addr.s_addr&0xFF0000)>>16),(int(clients[idx].cli_addr.sin_addr.s_addr&0xFF000000)>>24), message);
    fclose(pFile);
}
void Server::ProcessQRCode(char* filename, int idx) {
    clients[idx].clientData = clients[idx].clientData; /* add 4 to ignore first 4 bytes (filesize) */
    char inBuffer[1000];

    /* write image file locally for use in jar file QR interpreter */
    FILE * fp;
    fp = fopen(filename, "wb+");
    std::ofstream output(filename, std::ios::binary);
    output.write(clients[idx].clientData, clients[idx].filesize);
    output.close();

    /* run java program for QR code */
    FILE* progOutput; // a file pointer representing the popen output
    char* cmd; /* string of java command */
    char* base = (char *)"java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner";
    asprintf(&cmd, "%s %s", base, filename);
    //printf("Running command: %s\n", cmd);
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
            lineCounter++;
        }
        else if(lineCounter == 4) {
            clients[idx].clientResponse[z]= inBuffer[n];
            z++;
        }
        if(lineCounter == 5) {
            break;
        }
    }
    if (lineCounter < 4) { /* no barcode was found */
        uint32_t retcode = 1;
        uint32_t msgLength = 0;
        write(clients[idx].cli_sockfd, &retcode, 4); /* return code */
        write(clients[idx].cli_sockfd, &msgLength, 4); /* return code */
        const char* message = "Client image file unable to be decoded";
        Write_Text_To_Log_File(idx,message);
    }
    printf("Done processing QR code\n");
    //free(clients[idx].clientData); /* cleanup memory */
    remove(filename);
}

/* thread method to call handle_client */
static void *Client_Thread(void *context) {
    struct threadArgs* args = (struct threadArgs *)context;
    Server serv = *(args->s);
    int index = args->idx;
    serv.Handle_Client(index);
    pthread_mutex_lock(&mutex);
    threadStatus[index] = 0; /* mark done */
    numThreads--;
    pthread_mutex_lock(&mutex);
    printf("Thread %d is now available\n", index);
    //pthread_exit(NULL);
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

    while(true) {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
        if(-1 == opt) break;
        switch (opt) {
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
            std::cout << "Timeout set to: " << timeOut << std::endl;
            break;
        case '?': // Unrecognized option
        default:
            break;
        }
    }

    Server server = Server(port, rateRequests, rateSeconds, maxUsers, timeOut); 
    pthread_mutex_init(&mutex, NULL);
    
    /* working instead with processes */
    int index = 0;
    pid_t* pids = (pid_t *)malloc(sizeof(pid_t)*maxUsers); /* list of pids */
    for (int i=0; i<maxUsers; i++) {
        pids[i] = -1; /* initialize list of pids */
    }
    while (true) {
        if (numThreads < maxUsers) { /* check if we can take another connection */
            for (int i = 0; i<maxUsers; i++) { /* find empty index */
                if (pids[i] == -1) {
                    index = i;
                    break;
                }
            }
            server.Accept(index);
            printf("num threads: %d\n", numThreads);
            numThreads = numThreads + 1;
            if ((pid = fork()) < 0) { perror("Fork error"); }
            else if (pid == 0) { /* child process, handle client */
                server.Handle_Client(index);
                return 0;
            }
            else { /* parent */
                pids[index] = pid;
            }
        }
        else {
            int status;
            int statloc;
            for (int i=0; i<maxUsers; i++) {
                if (pids[i] != -1 ) {
                    if ((status = waitpid(pids[i], &statloc, WNOHANG) == pids[i])) { /* process has finished */
                        printf("Process complete, number of users %d --> %d\n", numThreads, numThreads-1);
                        numThreads--;
                        pids[i] = -1;
                        break;
                    }
                }
            }
        }
    }

    return 0;
}