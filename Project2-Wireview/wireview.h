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
#include <map>
#include <time.h>

#define ETH_ADDR_LEN 6
#define ETH_HEAD_LEN 14

struct packetInfo {
    bool firstFlag = true; /* whether or not it is the firstpacket in the capture */
    uint32_t totalPackets;
    uint32_t minPacketSize;
    uint32_t maxPacketSize;
    uint32_t avgPacketSize;
    std::set<int> udp_src_set;
    std::set<int> udp_dst_set;
    std::map<std::string, uint32_t> eth_src_map;
    std::map<std::string, uint32_t> eth_dst_map;
    std::map<std::string, uint32_t> ip_src_map;
    std::map<std::string, uint32_t> ip_dst_map;
    // keep track of unique src IPs -- as well as the number of packets for each (vector + integer fields)
    // keep track of unique dst IPs -- as well as the number of packets for each (vector + integer fields)
};

struct packetInfo packetInfo; /* global struct for accumulating data */
struct timeval first;
struct timeval last;
struct tm *nowtm;


void packetHandler(unsigned char *userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet); /* on-loop packet paser */
void printGlobalStats(); /* helper to print struct info */
void initGlobalStats(); /* not sure if necessary, why not tho lol */