#include "wireview.h"

void packetHandler(unsigned char *userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
    int size_ip = 0;
    struct ip* ip;

    /* find ethernet header info */
    const struct ether_header* ether = (struct ether_header *)(packet);
    char ether_src[ETH_ADDR_LEN];
    char ether_dst[ETH_ADDR_LEN];
    ether_ntoa_r((struct ether_addr *)(ether->ether_shost), ether_src);
    ether_ntoa_r((struct ether_addr *)(ether->ether_dhost), ether_dst);
    printf("Ethernet src: %s\t Ethernet dst: %s\n", ether_src, ether_dst); /* TODO fix only returning destination*/

    if (ntohs(ether->ether_type) == ETHERTYPE_ARP) { /* check if ARP */
        /* if ARP, we just have to return ethernet addresses (no IP/UDP) */
        printf("ARP packet\n");
    }
    else if (ntohs(ether->ether_type) == ETHERTYPE_IP) { /* check if IP */
        /* getting src/dst IP addresses */
        ip = (struct ip*)(packet + ETH_HEAD_LEN); 
        if (ip->ip_v != 4) {
            printf("IPv6 detected, dont care\n");
        }
        else {
            char ip_src[INET_ADDRSTRLEN];
            char ip_dst[INET_ADDRSTRLEN];
            size_ip = (int)(ip->ip_hl * 4);
            if (size_ip < 20 || size_ip > 24) {
                perror("Invalid IP size");
                exit(1);
            }
            else {
                inet_ntop(AF_INET, &(ip->ip_src), ip_src, INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &(ip->ip_dst), ip_dst, INET_ADDRSTRLEN);
            }
            printf("IP src: %s\t IP dst: %s\n", ip_src, ip_dst); /* TODO figure out where/how we want to store this */
        }
    }
    
    /* if UDP, get src/dst */
    if (ip->ip_p == 17) { // UDP --> ip_p == 17
        struct udphdr* udp = (struct udphdr *)(packet + ETH_HEAD_LEN + size_ip);
        printf("UDP src port: %d\tUDP dst port: %d\n", ntohs(udp->uh_sport), ntohs(udp->uh_dport)); // TODO fix

        // udp->uh_len for added offset to check payload (maybe has time)
    }

    /* TODO find start time of packet capture as well as duration */
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
