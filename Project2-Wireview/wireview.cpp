#include "wireview.h"

void packetHandler(unsigned char *userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
    if(packetInfo.firstFlag){
        struct timeval tv;
        first = pkthdr->ts;
        nowtm = localtime(&pkthdr->ts.tv_sec);
        printf("Date: %d-%d-%d Time: %d:%d:%d\n",nowtm->tm_year+1900,nowtm->tm_mon+1,nowtm->tm_mday,nowtm->tm_hour,
        nowtm->tm_min,nowtm->tm_sec);
    }
    last=pkthdr->ts;

    uint32_t packet_size = pkthdr->len *8;
    printf("Packet info:\n"); 
    printf("Packet lengh: %d \n", packet_size);
    
    /* update records if first in capture or >max or <min, add to total packet size */
    if(packet_size > packetInfo.maxPacketSize || packetInfo.firstFlag) { 
        packetInfo.maxPacketSize = packet_size; 
    }
    if(packet_size < packetInfo.minPacketSize || packetInfo.firstFlag) { 
        packetInfo.minPacketSize = packet_size; 
    }
    packetInfo.avgPacketSize = packetInfo.avgPacketSize + packet_size; /* actual avg is taken right before we print stats */


    uint32_t size_ip = 0;
    struct ip* ip;

    /* find ethernet header info */
    const struct ether_header* ether = (struct ether_header *)(packet);
    char ether_src[ETH_ADDR_LEN];
    char ether_dst[ETH_ADDR_LEN];
    ether_ntoa_r((struct ether_addr *)(ether->ether_shost), ether_src);
    printf("Ethernet SRC: %s\t", ether_src);
    packetInfo.eth_src_map.insert(std::pair<std::string, uint32_t>(ether_src, packetInfo.eth_src_map.count(ether_src)+1)); /* TODO fix map count */
    ether_ntoa_r((struct ether_addr *)(ether->ether_dhost), ether_dst);
    printf("Ethernet DST: %s\n", ether_dst);
    packetInfo.eth_dst_map.insert(std::pair<std::string, uint32_t>(ether_dst, packetInfo.eth_dst_map.count(ether_dst)+1));

    if (ntohs(ether->ether_type) == ETHERTYPE_ARP) { /* check if ARP */
        /* if ARP, we just have to return ethernet addresses (no IP/UDP) -- protocol fields? */
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
            packetInfo.ip_src_map.insert(std::pair<std::string, uint32_t>(ip_src, packetInfo.ip_src_map.count(ip_src)+1));
            packetInfo.ip_dst_map.insert(std::pair<std::string, uint32_t>(ip_dst, packetInfo.ip_dst_map.count(ip_dst)+1));
            printf("IP src: %s\t IP dst: %s\n", ip_src, ip_dst);
        }
    }
    
    /* if UDP, get src/dst */
    if (ip->ip_p == 17) { // UDP --> ip_p == 17
        struct udphdr* udp = (struct udphdr *)(packet + ETH_HEAD_LEN + size_ip);
        packetInfo.udp_src_set.insert(ntohs(udp->uh_sport));
        packetInfo.udp_dst_set.insert(ntohs(udp->uh_dport));

        printf("UDP src port: %d\tUDP dst port: %d\n", ntohs(udp->uh_sport), ntohs(udp->uh_dport));
        // udp->uh_len for added offset to check payload (maybe)
    }

    packetInfo.totalPackets++;
    packetInfo.firstFlag = false;
    printf("\n");
}

void initGlobalStats() {
    packetInfo.minPacketSize = 0;
    packetInfo.avgPacketSize = 0;
    packetInfo.maxPacketSize = 0;
    packetInfo.totalPackets = 0;
}

void printGlobalStats() {
    /* first compute average packet size */
    packetInfo.avgPacketSize = packetInfo.avgPacketSize / packetInfo.totalPackets;

    /* print general packet stats */
    printf("Total packets: %d\n", packetInfo.totalPackets);
    printf("Min packet size: %d\n", packetInfo.minPacketSize);
    printf("Max packet size: %d\n", packetInfo.maxPacketSize);
    printf("Avg packet size: %d\n", packetInfo.avgPacketSize);

    /* print ethernet stats */
    printf("Ethernet src addresses:\n");
    for (auto it = packetInfo.eth_src_map.begin(); it != packetInfo.eth_src_map.end(); it++) {
        printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
    }
    printf("\nEthernet dst addresses:\n");
    for (auto it = packetInfo.eth_dst_map.begin(); it != packetInfo.eth_dst_map.end(); it++) {
        printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
    }

    /* print ethernet stats */
    printf("\nIPv4 srcs:\n");
    for (auto it = packetInfo.ip_src_map.begin(); it != packetInfo.ip_src_map.end(); it++) {
        printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
    }
    printf("\nIPv4 dsts:\n");
    for (auto it = packetInfo.ip_dst_map.begin(); it != packetInfo.ip_dst_map.end(); it++) {
        printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
    }

    /* print out unique UDP src/dst ports */
    printf("\nUDP src ports:\n");
    for (auto it = packetInfo.udp_src_set.begin(); it != packetInfo.udp_src_set.end(); it++) {
        printf("%d\t", *it);
    }
    printf("\nUDP dst ports:\n");
    for (auto it = packetInfo.udp_dst_set.begin(); it != packetInfo.udp_dst_set.end(); it++) {
        printf("%d\t", *it);
    }
    printf("\n");
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

    /* initialize relevant fields of global struct */
    initGlobalStats();
    
    // opens pcap for given file name errbuff is just used and filled if there is an error
	if ((desc = pcap_open_offline(filename,errbuf)) == NULL) {
		printf("%s\n",errbuf);
		return(2);
	}

    // file session created by pcap_open int for number of packets processed a negative number
    //meants process packets until another conditions comes up packetHandler is run on each packet
    pcap_loop(desc,-1,packetHandler,NULL);
    pcap_close(desc);

    /* after loop, print stats */
    printGlobalStats();
}
