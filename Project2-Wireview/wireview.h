#include <stdio.h>
#include <stdlib.h>
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

};

/* from tcpdump.org/pcap.html */
/* IP header */
struct sniff_ip {
    unsigned char ip_vhl;		/* version << 4 | header length >> 2 */
	unsigned char ip_tos;		/* type of service */
	unsigned char ip_len;		/* total length */
	unsigned char ip_id;		/* identification */
	unsigned char ip_off;		/* fragment offset field */
	unsigned char ip_ttl;		/* time to live */
	unsigned char ip_p;		/* protocol */
	unsigned char ip_sum;		/* checksum */
	struct in_addr ip_src,ip_dst; /* source and dest address */
};
#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)



struct packetInfo packetInfo; /* global struct for accumulating data */

void packetHandler(unsigned char *userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet);

