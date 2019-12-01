#include "overlay.h"
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <string.h>

// sample router input:  ./overlay --router 1.2.3.4:10.0.0.2,5.6.7.8:10.0.0.3,9.10.11.12:10.0.0.4,13.14.15.16:10.0.0.5
// sample endhost input:   ./overlay --host 10.0.0.1,1.2.3.4,128

void printUsage() {
    printf("Router Usage: ./overlay --router <ip mappings>\n");
    printf("End-Host Usage: ./overlay --host <router ip> <host ip> <ttl>\n");
}

void parseMappings(char* ipMappings, std::vector<std::string> overlayIPs, std::vector<std::string> vmIPs) {
    char ipDelim[] = ":,";
    char* currIp; // current ip
    uint32_t countIp = 1;
    currIp = strtok(ipMappings, ipDelim);
    while (currIp != NULL) {
        if (countIp % 2 == 0) {
            vmIPs.push_back(currIp);
        }
        else {
            overlayIPs.push_back(currIp);
        }
        currIp = strtok(NULL, ipDelim);
        countIp++;
    }
}

void sendUDP(char* sourceaddr, char* destaddr, uint32_t ttl){
    const char *data = "Hello";
    char *packet = (char *)malloc(sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data));
    struct iphdr *ip = (struct iphdr*) packet;
    struct udphdr *udp = (struct udphdr*) (packet + sizeof(struct iphdr));
    char *send_buff = (char *) (packet + sizeof(struct iphdr) + sizeof(struct udphdr));
    ip->saddr = inet_addr(sourceaddr);
    ip->daddr = inet_addr(destaddr);
    ip->ttl = ttl;
    ip->check = 0;
    ip->frag_off=0;
    ip->protocol=17;
    ip->tot_len=5;
    ip->tos=0;
    udp->source = DEFAULT_UDP_PORT;
    udp->dest = DEFAULT_UDP_PORT;
    udp->len = sizeof(struct udphdr);
    udp->check = 0;
    strcpy(send_buff, data);
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM,0))<0) {
        perror("Socket Creation error");
        exit(1);
    }
    size_t msg_len = strlen(packet);
    sendto(sock, packet, msg_len,0, (sockaddr*)destaddr, sizeof(destaddr));
    // stuff might be wrong with addr might need conversion to this format
    //    serv_addr.sin_family = AF_INET; 
    // serv_addr.sin_port = htons(port); 
    
    // if (inet_pton(AF_INET, address, &serv_addr.sin_addr)<=0) 
    // { 
    //     perror("Invalid address/ Address not supported"); 
    //     exit(1);
    // } 
    
}

void readUDP(char* packet) {
    uint32_t size_ip_system = 0;
    uint32_t size_ip = 0;
    
    struct ip* ipSystem = (struct ip*)(packet + ETH_HEAD_LEN); 
    size_ip_system = (int)(ipSystem->ip_hl * 4);
    struct udphdr* udpSystem = (struct udphdr *) packet + ETH_HEAD_LEN + size_ip;
    struct ip* ip = (struct ip*)(packet+ETH_HEAD_LEN+size_ip+udpSystem->uh_ulen);
    size_ip= (int)(ip->ip_hl*4);
    struct udphdr* udp = (struct udphdr *)(packet + ETH_HEAD_LEN + size_ip+udpSystem->uh_ulen);
    
}

void recieveUDP(char* buffer) {
    int sockfd, optVal, recvVal;
    struct sockaddr_in servAddr, cliAddr;
    struct hostent *hostp; /* maybe use for logging */
    char *hostaddrp; /* maybe use for logging */
    socklen_t cliLen;
    
    // configuration/initialization
    cliLen = sizeof(cliAddr);
    bzero(buffer, MAX_SEGMENT_SIZE);
    bzero((char *) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons((unsigned short)DEFAULT_UDP_PORT);

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error opening socket");
    }
    
    // allows us to rerun server after segfaults
    optVal = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optVal, sizeof(int));

    // bind socket
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("Error binding socket");
    }

    // recieve segment from socket
    if ((recvVal = recvfrom(sockfd, buffer, MAX_SEGMENT_SIZE, 0, (struct sockaddr *)&cliAddr, &cliLen)) < 0) {
        perror("Error reading from socket");
    }
}

/* run process as router */
int runRouter(char* ipMappings) {
    // parsing mappings for list of overlay and VM IPs
    std::vector<std::string> overlayIPs;
    std::vector<std::string> vmIPs;
    parseMappings(ipMappings, overlayIPs, vmIPs);

    // listen for UDP messages, check for the overlay IP and send to corresponding vm IP, decrement ttl/drop packets as needed
    char* udpSegment = (char*)malloc(sizeof(char) * MAX_SEGMENT_SIZE);
    while (true) {
        recieveUDP(udpSegment); // read from socket into buffer;
        
    }
    
    // free memory
    free(udpSegment);
}

/* run process as end-host */
// TODO maybe refactor to just take in one string and parse the ips/ttl from there (see sample input at top of doc)
int runEndHost(char* routerIP, char* hostIP, uint32_t ttl) {
    // craft packet and send message to router

}

int main(int argc, char** argv) {
    bool router = false;
    bool host = false;
    char* ipMappings;
    char* routerIP;
    char* hostIP;
    uint32_t ttl;

    if (argc <= 1) {
        printUsage();
        exit(1);
    }
    if (argc > 1) {
        const char* const short_opts = "r:h:";
        const option long_opts[] = {
            {"router", required_argument, nullptr, 'r'},
            {"host", required_argument, nullptr, 'h'}
        };
        while(true) {
            const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
            if(-1 == opt) break;
            switch (opt) {
            case 'h':
                if (argc < 4) {
                    printUsage();
                    return 1;
                }
                else {
                    routerIP = argv[2];
                    hostIP = argv[3];
                    ttl = atoi(argv[4]);
                    printf("Running in host mode with routerIP:%s | hostIP:%s | ttl:%d\n", routerIP, hostIP, ttl);
                    host = true;
                    runEndHost(routerIP, hostIP, ttl);
                    break;
                }
            case 'r':
                if(argc < 2){
                    printUsage();
                    return 1;
                }
                else {
                    ipMappings = argv[2];
                    printf("Running in router mode with IP mappings: %s\n", ipMappings);
                    router = true;
                    runRouter(ipMappings);
                    break;
                }                
            default:
                printUsage();
                break;
            }
        }
    }
    return 0;
}