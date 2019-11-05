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
    if (idx >= users) { /* invalid, we should reject */
        printf("Rejecting user\n");
        int rejectfd;
        struct sockaddr_in temp_addr;
        socklen_t temp_len;
        rejectfd = accept(sockfd, (struct sockaddr *)&temp_addr, &temp_len);
        char* response = (char*)"Client is either too busy or you have gone over your use limit. Try again later\n";
        if (write(rejectfd, response, sizeof(response) < 0)) {
            perror("Error writing to socket");
        }
        close(rejectfd);
    }
    /* we are fine to accept socket and store info in client struct as usual */
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
    const char* message = "Client Accepted";
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
    pthread_mutex_lock(&mutex);
    numThreads++;
    pthread_mutex_unlock(&mutex);
    char idxchar = (char) (idx + 48);
    char* filename = (char*)malloc(sizeof(char)*12);
    asprintf(&filename, "QR%c.png", idxchar);
    Server::Recieve(idx);
    printf("Recieved data from client on thread %d\n", idx);
    Server::ProcessQRCode(filename, idx); /* TODO figure out if we should deal with multiple filenames or just have mutexes */
    printf("Processed QR code on thread %d\n", idx);
    Server::Return(idx);
    printf("Returned result to client on thread %d\n", idx);
    free(filename);
    close(clients[idx].cli_sockfd);
    printf("Finished handling client index %d\n", idx);
    pthread_mutex_lock(&mutex);
    threadStatus[idx] = 0;
    numThreads--;
    pthread_mutex_unlock(&mutex);
    return (void)0;
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
    pthread_mutex_init(&mutex, NULL);
    
    /* working instead with processes */
    int index = 0;
    while (true) {
        if (numThreads < maxUsers) { /* check if we can take another connection */
            for (int i = 0; i<maxUsers; i++) { /* find empty index */
                if (threadStatus[i] == 0) {
                    index = i;
                    threadStatus[i] = 1;
                    break;
                }
            }
            server.Accept(index);
            if ((pid = fork()) < 0) { perror("Fork error"); }
            else if (pid == 0) { /* child process, handle client */
                server.Handle_Client(index);
            }
            else { /* parent */
                //goes back to top of loop to deal with next client
            }
        }
        else {
            server.Reject();
        }
    }

    /* accept connections on main thread, pass off to other threads, update maxusers, deny bad connections */
    // int index = 0;
    // while (true) {
    //     index = numThreads;
    //     printf("Attempting to accept a client on thread %d\n", numThreads);
    //     server.Accept(index); /* will reject immediately if numThreads out of bounds */
    //     printf("Client accepted\n");
    //     if (index < maxUsers) { /* handle request with thread */
    //         // if (threadStatus[numThreads] != 0) {/* thread is not available */
    //         //     pthread_join(*(threads[index]), NULL);
    //         //     numThreads--;
    //         // }
    //         struct threadArgs* arguments = (struct threadArgs *)malloc(sizeof(struct threadArgs));
    //         arguments->s = &server;
    //         arguments->idx = index;
    //         printf("dispatching thread on index %d\n", index);
    //         pthread_mutex_lock(&mutex);
    //         threads[index] = (pthread_t *)malloc(sizeof(pthread_t));
    //         pthread_create(threads[index], NULL, &Client_Thread, (void*)arguments);
    //         threadStatus[index] = 1;
    //         numThreads = numThreads + 1;
    //         pthread_mutex_unlock(&mutex);
    //         printf("done managing thread %d from main\n", index);
    //     }
    //     usleep(10000);
    // }

    return 0;
}