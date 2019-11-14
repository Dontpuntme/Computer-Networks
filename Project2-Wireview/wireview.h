#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pcap/pcap.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define ETH_ADDR_LEN 6
#define ETH_HEAD_LEN 14

struct packetInfo {
    int totalPackets;
    // keep track of unique src IPs
    // keep track of unique dst IPs
};

struct packetInfo packetInfo; /* global struct for accumulating data */

void packetHandler(unsigned char *userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet);

