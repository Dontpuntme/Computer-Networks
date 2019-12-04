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
            vmIPs.push_back(currIp);
        }
        else
        {
            overlayIPs.push_back(currIp);
        }
        currIp = strtok(NULL, ipDelim);
        countIp++;
    }
}

void sendUDP(char *routeraddr, char *sourceaddr, char *destaddr, uint32_t ttl)
{
    const char *data = "Hello";
    char *packet = (char *)malloc(sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data));
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
    if (inet_pton(AF_INET, destaddr, &dest_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(1);
    }
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
    udp->source = htons(DEFAULT_UDP_PORT);
    udp->dest = htons(DEFAULT_UDP_PORT);
    udp->len = sizeof(struct udphdr);
    udp->check = 0;
    strcpy(send_buff, data);
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket Creation error");
        exit(1);
    }
    size_t msg_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data);
    sendto(sock, packet, msg_len, 0, (struct sockaddr *)&router_addr, sizeof(router_addr));
}

/* decrement ttl, send udp to dest ip specified in packet (return -1 if packet dropped, 0 if ip not in overlay table, 1 if sent successfully) */
int routePacket(char *packet, std::vector<std::string> &overlayIPs, std::vector<std::string> &vmIPs)
{
    uint32_t size_ip_system = 0;
    uint32_t size_ip = 0;

    struct ip *ip = (struct ip *)(packet + ETH_HEAD_LEN);
    size_ip = (int)(ip->ip_hl * 4);
    struct udphdr *udp = (struct udphdr *)packet + ETH_HEAD_LEN + size_ip;

    // first check if ip is in our routing table
    std::string overlayIP = (std::string)(inet_ntoa(ip->ip_src));
    if (std::find(overlayIPs.begin(), overlayIPs.end(), overlayIP) != overlayIPs.end())
    {
        // overlay IP found in table
        printf("Overlay IP %s found in lookup table!\n", overlayIP.c_str());
    }
    else
    {
        // overlay IP not found in table, drop packet
        printf("Overlay IP %s not found in lookup table. :(\n", overlayIP.c_str());
        return 0;
    }

    // update time to live, dropping packet if drops below 0
    if (ip->ip_ttl < 1)
    {
        // drop packet
        return -1;
    }
    else
    {
        ip->ip_ttl--;
        int sock;
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            perror("Socket Creation error");
            exit(1);
        }
        size_t msg_len = strlen(packet);
        struct sockaddr dest;
        dest.sa_family = AF_INET;
        strncpy(dest.sa_data, (char *)(ip->ip_dst.s_addr), sizeof(dest.sa_data));
        sendto(sock, packet, msg_len, 0, &dest, sizeof(dest));
        return 1;
    }
}
void recieveUDP(char *buffer)
{
    struct addrinfo hints, *res;
    int sockfd;
    int byte_count;
    socklen_t fromlen;
    struct sockaddr addr;
    bzero(buffer, MAX_SEGMENT_SIZE);

    struct sockaddr_in test;
    test.sin_family = AF_INET;
    test.sin_port = htons(DEFAULT_UDP_PORT);
    test.sin_addr.s_addr = INADDR_ANY;

    int socktest = socket(AF_INET, SOCK_DGRAM, 0);
    bind(socktest, (struct sockaddr *)&test, sizeof(test));
    printf("Waiting at recvfrom");
    byte_count = recvfrom(socktest, buffer, MAX_SEGMENT_SIZE, 0, &addr, &fromlen);

    printf("recv()'d %d bytes of data in buf\n", byte_count);
    printf("Recieved data from socket\n");

}

/* run process as router */
int runRouter(char *ipMappings)
{
    // parsing mappings for list of overlay and VM IPs
    std::vector<std::string> overlayIPs;
    std::vector<std::string> vmIPs;
    parseMappings(ipMappings, overlayIPs, vmIPs);

    printf("Number of overlay networks parsed: %ld\n", overlayIPs.size());
    printf("Number of overlay networks parsed: %ld\n", vmIPs.size());

    // listen for UDP messages, check for the overlay IP and send to corresponding vm IP, decrement ttl/drop packets as needed
    char *udpSegment = (char *)malloc(sizeof(char) * MAX_SEGMENT_SIZE);
    uint8_t retcode;
    while (true)
    {
        recieveUDP(udpSegment); // read from socket into buffer;
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
        bzero(udpSegment, MAX_SEGMENT_SIZE);
    }
    // free memory
    free(udpSegment);
}

/* run process as end-host */
// TODO maybe refactor to just take in one string and parse the ips/ttl from there (see sample input at top of doc)
int runEndHost(char *routerIP, char *hostIP, uint32_t ttl)
{
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
    sendUDP(routerIP, hostIP, test, ttl);
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_SEGMENT_SIZE];
    //make sure socket is non blocking eventually
    n = recvfrom(sockfd, (char *)buffer, MAX_SEGMENT_SIZE,
                 MSG_WAITALL, (struct sockaddr *)&router_IP,
                 &len);
    buffer[n] = '\0';
    printf("Server : %s\n", buffer);
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