#include <pcap/pcap.h>
#include <net/ethernet.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define ETH_ADDR_LEN 6
#define ETH_HEAD_LEN 14

void packetHandler(char *userData, const struct pcap_pkthdr* pkthdr, const char* packet);

