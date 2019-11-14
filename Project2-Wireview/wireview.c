#include "wireview.h"

void packetHandler(unsigned char *userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
    /* getting src/dst IP addresses */
    struct ip* ip =(struct ip*)(packet + ETH_HEAD_LEN); 
    char ip_src[INET_ADDRSTRLEN];
    char ip_dst[INET_ADDRSTRLEN];
    uint32_t size_ip = (uint32_t)(ip->ip_hl * 4);
    if (size_ip < 20 || size_ip > 24) {
        perror("Invalid IP size");
        exit(1);
    }
    else {
        inet_ntop(AF_INET, &(ip->ip_src), ip_src, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(ip->ip_dst), ip_dst, INET_ADDRSTRLEN);
    }
    printf("IP src: %s\t IP dst: %s\n", ip_src, ip_dst); /* TODO figure out where/how we want to store this */

    /* TODO find ethernet header related info */
    // check ether_ntoa and cast to ether_addr
    const struct ether_addr* ether = (struct ether_addr *)(packet);
    char ether_addr[ETH_ADDR_LEN];
    ether_ntoa_r(ether, ether_addr);
    printf("Ethernet address: %s\n", ether_addr);

    /* TODO determine ARP/IP, find related fields for ARP */

    /* TODO determine if carrying UDP, if so print related ports */

    packetInfo.totalPackets++;
}

int main(int argc, char** argv) {
    char errbuf[PCAP_ERRBUF_SIZE];
    char* filename;
    pcap_t* desc;
     
    if (argc < 2) {
        printf("No file provided, exiting\n");
        return 1;
    }
    else {
        filename = argv[1];
    }
    
    // opens pcap for given file name errbuff is just used and filled if there is an error
	if ((desc = pcap_open_offline(filename,errbuf)) == NULL) {
		printf("%s\n",errbuf);
		return(2);
	}

    // file session created by pcap_open int for number of packets processed a negative number
    //meants process packets until another conditions comes up packetHandler is run on each packet
    pcap_loop(desc,-1,packetHandler,NULL);
    pcap_close(desc);

    printf("Total packets: %d\n", packetInfo.totalPackets);
}
