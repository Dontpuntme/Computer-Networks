#include "overlay.h"
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <string.h>

#define MAXLINE 1024
// sample router input:  ./overlay --router 1.2.3.4:10.0.0.2,5.6.7.8:10.0.0.3,9.10.11.12:10.0.0.4,13.14.15.16:10.0.0.5
// sample endhost input:   ./overlay --host 10.0.0.1 1.2.3.4 12

void printUsage()
{
    printf("Router Usage: ./overlay --router <ip mappings>\n");
    printf("End-Host Usage: ./overlay --host <router ip> <host ip> <ttl>\n");
}

void parseMappings(char *ipMappings, std::vector<std::string> &overlayIPs, std::vector<std::string> &vmIPs)
{
    char ipDelim[] = ":,";
    char *currIp; // current ip
    uint32_t countIp = 1;
    currIp = strtok(ipMappings, ipDelim);
    while (currIp != NULL)
    {
        if (countIp % 2 == 0)
        {
            overlayIPs.push_back(currIp); // TODO make sure these are correct (overlay vs VM)
            printf("Mapped Overlay IP: %s\n", currIp);
        }
        else
        {
            vmIPs.push_back(currIp);
            printf("Mapped VM IP: %s\n", currIp);
        }
        currIp = strtok(NULL, ipDelim);
        countIp++;
    }
}

// TODO add functionality to send the filesize then
void sendUDP(char *routeraddr, char *sourceaddr, uint32_t destAddr, uint32_t ttl, char *data, uint32_t datalen, uint16_t id)
{
    char *packet = (char *)malloc(sizeof(struct iphdr) + sizeof(struct udphdr) + datalen);
    struct sockaddr_in dest_addr;
    struct sockaddr_in src_addr;
    struct sockaddr_in router_addr;
    struct iphdr *ip = (struct iphdr *)packet;
    struct udphdr *udp = (struct udphdr *)(packet + sizeof(struct iphdr));
    char *send_buff = (char *)(packet + sizeof(struct iphdr) + sizeof(struct udphdr));
    router_addr.sin_family = AF_INET;
    router_addr.sin_port = htons(DEFAULT_UDP_PORT);
    if (inet_pton(AF_INET, routeraddr, &router_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(1);
    }
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEFAULT_UDP_PORT);
    // if (inet_pton(AF_INET, destaddr, &dest_addr.sin_addr) <= 0)
    // {
    //     perror("Invalid address/ Address not supported");
    //     exit(1);
    // }
    dest_addr.sin_addr.s_addr = destAddr;
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(DEFAULT_UDP_PORT);
    if (inet_pton(AF_INET, sourceaddr, &src_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(1);
    }
    ip->saddr = src_addr.sin_addr.s_addr;
    ip->daddr = dest_addr.sin_addr.s_addr;
    ip->ttl = ttl;
    ip->check = 0;
    ip->frag_off = 0;
    ip->protocol = 17;
    ip->tot_len = 5;
    ip->tos = 0;
    ip->id = id;
    udp->source = htons(DEFAULT_UDP_PORT);
    udp->dest = htons(DEFAULT_UDP_PORT);
    udp->len = sizeof(struct udphdr);
    udp->check = 0;
    strcpy(send_buff, data);
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("Socket Creation error");
        exit(1);
    }

    // TODO fix send msg size
    size_t msg_len = datalen + sizeof(struct iphdr) + sizeof(struct udphdr); // sizeof(struct iphdr) + sizeof(struct udphdr) + 1000;
    //size_t msg_len = (sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data));

    sendto(sock, packet, msg_len, 0, (struct sockaddr *)&router_addr, sizeof(router_addr));
    usleep(100000); // packets must be separated 100ms
}

void routerlog(std::string srcOverlayIP, std::string dstOverlayIP, unsigned short ipdent, int8_t retcode)
{
    FILE *logFile;
    logFile = fopen("ROUTER_log.txt", "a");
    struct timespec time;
    if (clock_gettime(CLOCK_REALTIME, &time))
    {
        perror("Could not get time");
        return;
    }
    fprintf(logFile, "%d  %s  %s  %d  %s\n", (uint32_t)time.tv_sec, srcOverlayIP.c_str(), dstOverlayIP.c_str(), ipdent, retcodeString(retcode).c_str());
    fclose(logFile);
}

std::string retcodeString(int8_t retcode)
{
    if (retcode == -1)
    {
        return (std::string) "TTL_EXPIRED";
    }
    else if (retcode == 0)
    {
        return (std::string) "NO_ROUTE_TO_HOST";
    }
    else if (retcode == 1)
    {
        return (std::string) "SENT_OKAY";
    }
    else
    {
        return (std::string) "UNDEF_BEHAVIOR";
    }
}

/* decrement ttl, send udp to dest ip specified in packet (return -1 if packet dropped, 0 if ip not in overlay table, 1 if sent successfully) */
int8_t routePacket(char *packet, std::vector<std::string> &overlayIPs, std::vector<std::string> &vmIPs)
{
    int8_t retcode = -2;
    uint32_t size_ip = 0;

    struct ip *ip = (struct ip *)(packet);
    size_ip = (int)(ip->ip_hl * 4);
    struct udphdr *udp = (struct udphdr *)(packet + size_ip);

    // check checksum
    printf("Packet checksum at router: %d (SHOULD BE 0)\n", ip->ip_sum);

    // first check src/dst overlay IPs
    char srcOverlayIP[INET_ADDRSTRLEN];
    const char *ret1 = inet_ntop(AF_INET, &(ip->ip_src), srcOverlayIP, sizeof(srcOverlayIP));
    if (ret1 == NULL)
    {
        perror("Invalid src overlay IP\n");
        exit(1);
    }
    char dstOverlayIP[INET_ADDRSTRLEN];
    const char *ret2 = inet_ntop(AF_INET, &(ip->ip_dst), dstOverlayIP, sizeof(dstOverlayIP));
    if (ret2 == NULL)
    {
        perror("Invalid dst overlay IP\n");
        exit(1);
    }

    // check routing map for where src IP should go
    std::string overlayIP;
    uint32_t i = 0;
    bool foundMatch = false;
    const char *tableIP = (char *)malloc(sizeof(char) * INET_ADDRSTRLEN);
    for (i = 0; i < overlayIPs.size(); i++)
    {
        tableIP = overlayIPs.at(i).c_str();
        printf("Checking overlay IP: %s against IP in table: %s\n", srcOverlayIP, tableIP);
        if ((strcmp(srcOverlayIP, overlayIPs[i].c_str()) == 0))
        {
            printf("Found match for overlay IP in routing table!\n");
            foundMatch = true;
            break;
        }
        printf("Overlay IP: %s\n", overlayIPs[i].c_str());
    }
    if (!foundMatch)
    {
        printf("No match found for router string, exiting. :(\n");
        retcode = 0;
        routerlog(srcOverlayIP, dstOverlayIP, ip->ip_id, retcode);
        return retcode;
    }

    // update time to live, dropping packet if drops to 0
    if (ip->ip_ttl <= 1)
    {
        printf("TTL dropped to zero, dropping packet\n");
        retcode = -1;
        routerlog(srcOverlayIP, dstOverlayIP, ip->ip_id, retcode);
        return retcode;
    }
    else
    {
        ip->ip_ttl = (ip->ip_ttl)--;
        int sock;
        if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
            perror("Socket Creation error");
            exit(1);
        }
        // TODO fix send msg size
        size_t msg_len = sizeof(struct iphdr) + sizeof(struct udphdr) + 1000;
        struct sockaddr_in dest;
        dest.sin_family = AF_INET;
        dest.sin_port = htons(DEFAULT_UDP_PORT);
        if (inet_pton(AF_INET, vmIPs[i].c_str(), &dest.sin_addr) <= 0)
        {
            perror("Invalid address/ Address not supported");
            exit(1);
        }
        printf("Router sending out to dest %s\n", inet_ntoa(dest.sin_addr));
        sendto(sock, packet, msg_len, 0, (struct sockaddr *)&dest, sizeof(dest));
        retcode = 1;
        routerlog(srcOverlayIP, dstOverlayIP, ip->ip_id, retcode);
        return retcode;
    }
}

int recvSocket()
{
    struct sockaddr_in test;
    test.sin_family = AF_INET;
    test.sin_port = htons(DEFAULT_UDP_PORT);
    test.sin_addr.s_addr = INADDR_ANY;

    int socktest = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
    bind(socktest, (struct sockaddr *)&test, sizeof(test));

    return socktest;
}

void recieveUDP(char *buffer, int socket, size_t segment_size)
{
    struct addrinfo hints, *res;
    int sockfd;
    int byte_count;
    socklen_t fromlen;
    struct sockaddr addr;

    byte_count = recvfrom(socket, buffer, segment_size, 0, &addr, &fromlen);

    printf("recv()'d %d bytes of data in buf\n", byte_count);
    printf("Recieved data from socket\n");
}

/* run process as router */
int runRouter(char *ipMappings)
{
    // create log file/clear contents
    FILE *logFile;
    logFile = fopen("log.txt", "w");
    fclose(logFile);

    // parsing mappings for list of overlay and VM IPs
    std::vector<std::string> overlayIPs;
    std::vector<std::string> vmIPs;
    parseMappings(ipMappings, overlayIPs, vmIPs);

    printf("Number of overlay networks parsed: %u\n", overlayIPs.size());
    printf("Number of overlay networks parsed: %u\n", vmIPs.size());

    // listen for UDP messages, check for the overlay IP and send to corresponding vm IP, decrement ttl/drop packets as needed
    char *udpSegment = (char *)malloc(sizeof(char) * MAX_SEGMENT_SIZE);
    int8_t retcode;

    // create and bind a socket to recv from
    int socket = recvSocket();

    while (true)
    {
        memset(udpSegment, 0, MAX_SEGMENT_SIZE);
        recieveUDP(udpSegment, socket, MAX_SEGMENT_SIZE + sizeof(struct iphdr) + sizeof(struct udphdr)); // read from socket into buffer;
        retcode = routePacket(udpSegment, overlayIPs, vmIPs);
        if (retcode == -1)
        {
            printf("Router dropped packet\n");
        }
        else if (retcode == 0)
        {
            printf("Router could not find overlay IP in routing table\n");
        }
        else if (retcode == 1)
        {
            printf("Router successfully sent message\n");
        }
        else
        { // This shouldnt ever happen
            printf("Unhandled return code\n");
        }
    }
    // free memory
    free(udpSegment);
}

/* run process as end-host */
int runEndHost(char *routerIP, char *hostIP, uint32_t ttl)
{
    // create and bind a socket to recv from
    int rSocket = recvSocket();

    struct sockaddr_in router_IP;
    router_IP.sin_port = DEFAULT_UDP_PORT;
    router_IP.sin_family = AF_INET;
    int sockfd;
    int n;
    unsigned int len;
    if (inet_pton(AF_INET, routerIP, &router_IP.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(1);
    }
    char test[] = "1.1.1.1";
    //  sendUDP(routerIP, test, hostIP, ttl);
    bool fileFlag = true;
    while (fileFlag)
    {
        fileFlag = !lookForFileAndProcess(routerIP, hostIP, ttl);
    }
    if ((sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    char *serverUDP = (char *)malloc(sizeof(char) * MAX_SEGMENT_SIZE);

    recieveUDP(serverUDP, rSocket, MAX_SEGMENT_SIZE + sizeof(struct iphdr) + sizeof(struct udphdr));
    printf("Endhost recieved data\n");
    uint32_t size_ip = 0;
    struct ip *ip = (struct ip *)(serverUDP);
    size_ip = (int)(ip->ip_hl * 4);
    struct udphdr *udp = (struct udphdr *)(serverUDP + size_ip);

    char ipBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip->ip_src.s_addr,
                    ipBuffer, sizeof(ipBuffer));
        
    printf("Recieved data from src %s\n", ipBuffer);

    int numSegmentsToRecv = 1;
    if (ip->ip_id == 0) { // figure out how many more segments we should be expecting
        printf("Looking at packet id 0, figuring out how many segments to recieve\n");
        //numSegmentsToRecv = ...
    }

     // Find filename to write to
    std::string outputFile;
    outputFile += ipBuffer;
    outputFile += ".bin";

    // char filename[INET_ADDRSTRLEN + 4 + 1];
    // snprintf(filename, INET_ADDRSTRLEN+4 + 1, "%s.bin", ipBuffer);
    // printf("Prepared to write to file: %s\n", filename);

    printf("Prepared to write to file: %s\n", outputFile.c_str());

    std::ofstream outfile;
    if (outfile) {
        outfile.open(outputFile, std::ios_base::out | std::ios_base::binary | std::ios_base::app); // append instead of overwrite
        printf("ofstream open didnt segfault lol\n");
    }
    else {
        perror("ofstream");
        exit(1);
    }

    for (int i = 0; i < numSegmentsToRecv; i++ ) { // recv and write to file 
        printf("Trying to recieve segment %d from router\n", i);
        //memset(serverUDP, 0, MAX_SEGMENT_SIZE); TODO 
        recieveUDP(serverUDP, rSocket, MAX_SEGMENT_SIZE + sizeof(struct iphdr) + sizeof(struct udphdr));
        printf("Recieved data from src %s\n", ipBuffer);

        // Write file data to filename
        if (outfile) {
            //outfile << (serverUDP + sizeof(struct iphdr) + sizeof(struct udphdr));
            outfile.write(serverUDP + sizeof(struct iphdr) + sizeof(struct udphdr), 1000);
            printf("Finished writing to file\n");
        }
        outfile.close();
    }

    printf("Server : %s\n", serverUDP);
}

bool lookForFileAndProcess(char *routerIP, char *sourceaddr, uint32_t ttl)
{
    std::ifstream my_file("tosend.bin");
    if (my_file)
    {
        sleep(2);
        std::ifstream ifs;
        ifs.open("tosend.bin", std::ifstream::binary);
        int i = 0;
        char c = ifs.get();
        char *buffer = (char *)malloc(sizeof(char) * (8));
        int a = 0;
        while (ifs.good())
        {

            buffer[i] = c;
            c = ifs.get();
            i++;
            if (i == 8)
            {
                a = int((char)(buffer[7]) << 24 |
                        (char)(buffer[6]) << 16 |
                        (char)(buffer[5]) << 8 |
                        (char)(buffer[4]));
                char *bufferTwo = (char *)realloc(buffer, 8 + a);
                if (bufferTwo)
                {
                    buffer = bufferTwo;
                }
                else
                {
                    break;
                    // deal with realloc failing because memory could not be allocated.
                }
            }
        }

        for (int k = 0; k < a; k++)
        {
            //std::cout << a;
            std::cout << buffer[8 + k];
        }
        int l;
        l = int((char)(buffer[0]) << 24 |
                (char)(buffer[1]) << 16 |
                (char)(buffer[2]) << 8 |
                (char)(buffer[3]));
        sendUDP(routerIP, sourceaddr, l, ttl, (char *)&a, 4, 0);
        int numberOfThousands = 0;
        numberOfThousands = a / 1000;
        for (int k = 0; k < numberOfThousands; k++)
        {
            sendUDP(routerIP, sourceaddr, l, ttl, &buffer[8 + (k * 1000)], 1000, k + 1);
        }
        int numberLeft = 0;
        numberLeft = a % 1000;
        if (numberLeft > 0)
        {
            sendUDP(routerIP, sourceaddr, l, ttl, &buffer[8 + a - numberLeft], numberLeft, numberOfThousands + 1);
        }
        ifs.close();
        free(buffer);
        return true;
    }
    return false;
}

int main(int argc, char **argv)
{
    bool router = false;
    bool host = false;
    char *ipMappings;
    char *routerIP;
    char *hostIP;
    uint32_t ttl;

    if (argc <= 1)
    {
        printUsage();
        exit(1);
    }
    if (argc > 1)
    {
        const char *const short_opts = "r:h:";
        const option long_opts[] = {
            {"router", required_argument, nullptr, 'r'},
            {"host", required_argument, nullptr, 'h'}};
        while (true)
        {
            const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
            if (-1 == opt)
                break;
            switch (opt)
            {
            case 'h':
                if (argc < 4)
                {
                    printUsage();
                    return 1;
                }
                else
                {
                    routerIP = argv[2];
                    hostIP = argv[3];
                    ttl = atoi(argv[4]);
                    printf("Running in host mode with routerIP:%s | hostIP:%s | ttl:%d\n", routerIP, hostIP, ttl);
                    host = true;
                    runEndHost(routerIP, hostIP, ttl);
                    break;
                }
            case 'r':
                if (argc < 2)
                {
                    printUsage();
                    return 1;
                }
                else
                {
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