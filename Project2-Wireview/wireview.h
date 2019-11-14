#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pcap/pcap.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <iterator>
#include <set>

#define ETH_ADDR_LEN 6
#define ETH_HEAD_LEN 14

struct packetInfo {
    int totalPackets;
    std::set<int> udp_src_set;
    std::set<int> udp_dst_set;
    // keep track of unique src IPs -- as well as the number of packets for each (vector + integer fields)
    // keep track of unique dst IPs -- as well as the number of packets for each (vector + integer fields)
    // ^ same for ethernet
    // keep track of unique UDP src and dst ports
    // keep track of avg/min/max packet sizes
};

struct packetInfo packetInfo; /* global struct for accumulating data */

void packetHandler(unsigned char *userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet); /* on-loop packet paser */
