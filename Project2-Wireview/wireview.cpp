#include "wireview.h"

void packetHandler(unsigned char *userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
    if(packetInfo.firstFlag){
        struct timeval tv;
        first = pkthdr->ts;
        start= localtime(&pkthdr->ts.tv_sec);
        //printf("Date: %d-%d-%d Time: %d:%d:%d\n",start->tm_year+1900,start->tm_mon+1,start->tm_mday,start->tm_hour,
        //start->tm_min,start->tm_sec);
    }
    last=pkthdr->ts;
    uint32_t packet_size = pkthdr->caplen;
    //printf("Packet info:\n"); 
    //printf("Packet lengh: %d \n", packet_size);
    
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
    //printf("Ethernet SRC: %s\t", ether_src);
    packetInfo.eth_src_map[ether_src] = packetInfo.eth_src_map[ether_src]+1;
    ether_ntoa_r((struct ether_addr *)(ether->ether_dhost), ether_dst);
    //printf("Ethernet DST: %s\n", ether_dst);
    packetInfo.eth_dst_map[ether_dst] = packetInfo.eth_dst_map[ether_dst]+1;

    if (ntohs(ether->ether_type) == ETHERTYPE_ARP) { /* check if ARP */
        /* if ARP, check for sender/reciever addresses and protocol formats */
        packetInfo.countARP++;
        packetInfo.arp.isArp = true; /* mark capture as containing ARP */
        struct ether_arp* arpinfo = (struct ether_arp *)(packet+ETH_HEAD_LEN);
        char* mac_src;
        char* mac_dst;
        char* arp_ip_src;
        char* arp_ip_dst;
        asprintf(&mac_src, "%02x:%02x:%02x:%02x:%02x:%02x\n", arpinfo->arp_sha[0], arpinfo->arp_sha[1], arpinfo->arp_sha[2], arpinfo->arp_sha[3], arpinfo->arp_sha[4], arpinfo->arp_sha[5]);
        asprintf(&mac_dst, "%02x:%02x:%02x:%02x:%02x:%02x\n", arpinfo->arp_tha[0], arpinfo->arp_tha[1], arpinfo->arp_tha[2], arpinfo->arp_tha[3], arpinfo->arp_tha[4], arpinfo->arp_tha[5]);
        asprintf(&arp_ip_src, "%u.%u.%u.%u", arpinfo->arp_spa[0], arpinfo->arp_spa[1],arpinfo->arp_spa[2],arpinfo->arp_spa[3]);
        asprintf(&arp_ip_dst, "%u.%u.%u.%u", arpinfo->arp_tpa[0], arpinfo->arp_tpa[1],arpinfo->arp_tpa[2],arpinfo->arp_tpa[3]);
        packetInfo.arp.arp_mac_src_map[mac_src] = packetInfo.arp.arp_mac_src_map[mac_src]+1;
        packetInfo.arp.arp_mac_dst_map[mac_dst] = packetInfo.arp.arp_mac_dst_map[mac_dst]+1;
        packetInfo.arp.arp_ip_src_map[arp_ip_src] = packetInfo.arp.arp_ip_src_map[arp_ip_src]+1;
        packetInfo.arp.arp_ip_dst_map[arp_ip_dst] = packetInfo.arp.arp_ip_dst_map[arp_ip_dst]+1;
    }
    else if (ntohs(ether->ether_type) == ETHERTYPE_IP) { /* check if IP */
        /* getting src/dst IP addresses */
        ip = (struct ip*)(packet + ETH_HEAD_LEN); 
        if (ip->ip_v != 4) {
            printf("IPv6 detected, dont care\n");
        }
        else {
            packetInfo.ipInfo.isIP4 = true;
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
            packetInfo.ipInfo.ip_src_map[ip_src] = packetInfo.ipInfo.ip_src_map[ip_src]+1;
            packetInfo.ipInfo.ip_dst_map[ip_dst] = packetInfo.ipInfo.ip_src_map[ip_dst]+1;
            //packetInfo.ip_dst_map[ip_dst] = packetInfo.ip_dst_map.count(ip_dst)+1;
            //printf("IP src: %s\t IP dst: %s\n", ip_src, ip_dst);
        }
        
        /* if UDP, get src/dst */
        if (ip->ip_p == 17) { // UDP --> ip_p == 17
            struct udphdr* udp = (struct udphdr *)(packet + ETH_HEAD_LEN + size_ip);
            packetInfo.udpInfo.isUDP = true;
            packetInfo.udpInfo.udp_src_set.insert(ntohs(udp->uh_sport));
            packetInfo.udpInfo.udp_dst_set.insert(ntohs(udp->uh_dport));
            packetInfo.countUDP++;
            //printf("UDP src port: %d\tUDP dst port: %d\n", ntohs(udp->uh_sport), ntohs(udp->uh_dport));
        }
        /* check for TCP */
        if (ip->ip_p == 6) {
            packetInfo.countTCP++;
        }
    }

    packetInfo.totalPackets++;
    packetInfo.firstFlag = false;
}

void initGlobalStats() {
    packetInfo.minPacketSize = 0;
    packetInfo.avgPacketSize = 0;
    packetInfo.maxPacketSize = 0;
    packetInfo.totalPackets = 0;
    packetInfo.countARP = 0;
    packetInfo.countTCP = 0;
    packetInfo.countUDP = 0;
}

double time_2_dbl(struct timeval time_value) {
    double new_time = 0;
    new_time = (double)(time_value.tv_usec);
    new_time/=1000000;
    new_time+=(double)time_value.tv_sec;
    return new_time;
}

void printGlobalStats() {
    /* first compute average packet size */
    packetInfo.avgPacketSize = packetInfo.avgPacketSize / packetInfo.totalPackets;

    /* print general packet stats */
    __time_t newfirst = first.tv_sec*1000000+first.tv_usec;
    __time_t newlast = last.tv_sec*1000000+last.tv_usec;
    struct timeval timedifference;
    double timeDouble;
    timedifference.tv_sec = (newlast-newfirst)/1000000;
    timedifference.tv_usec = (newlast-newfirst)%1000000;
    timeDouble = time_2_dbl(timedifference);
    printf("Time Stats:\n");
    printf("Date: %d-%d-%d Time: %02d:%02d:%02d\n",start->tm_year+1900,start->tm_mon+1,start->tm_mday,start->tm_hour,
        start->tm_min,start->tm_sec);
    printf("Duration: %f seconds\n", timeDouble);
    printf("General Packet Stats:\n");
    printf("Total packets: %d\n", packetInfo.totalPackets);
    printf("Min packet size: %d\n", packetInfo.minPacketSize);
    printf("Max packet size: %d\n", packetInfo.maxPacketSize);
    printf("Avg packet size: %d\n", packetInfo.avgPacketSize);
    printf("Total ARP: %d\n", packetInfo.countARP);
    printf("Total UDP: %d\n", packetInfo.countUDP);
    printf("Total TCP: %d\n\n", packetInfo.countTCP);

    /* print ethernet stats */
    printf("Ethernet SRC addresses:\n");
    for (auto it = packetInfo.eth_src_map.begin(); it != packetInfo.eth_src_map.end(); it++) {
        printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
    }
    printf("\nEthernet DST addresses\n");
    for (auto it = packetInfo.eth_dst_map.begin(); it != packetInfo.eth_dst_map.end(); it++) {
        printf("%s\t\tCount:%d\n", (*it).first.c_str(), (*it).second);
    }

    /* print ethernet stats */
    if (packetInfo.ipInfo.isIP4) {
        printf("\nIPv4 srcs:\n");
        for (auto it = packetInfo.ipInfo.ip_src_map.begin(); it != packetInfo.ipInfo.ip_src_map.end(); it++) {
            printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
        }
        printf("\nIPv4 dsts:\n");
        for (auto it = packetInfo.ipInfo.ip_dst_map.begin(); it != packetInfo.ipInfo.ip_dst_map.end(); it++) {
            printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
        }
    } 

    /* if ARP, print ARP mac addresses/IPs */
    if (packetInfo.arp.isArp) {
        printf("\nCapture contained ARP, information below:");
        printf("\nARP MAC srcs:\n");
        for (auto it = packetInfo.arp.arp_mac_src_map.begin(); it != packetInfo.arp.arp_mac_src_map.end(); it++) {
            printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
        }
        printf("\nARP IP srcs:\n");
        for (auto it = packetInfo.arp.arp_ip_src_map.begin(); it != packetInfo.arp.arp_ip_src_map.end(); it++) {
            printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
        }
        printf("\nARP MAC dsts:\n");
        for (auto it = packetInfo.arp.arp_mac_dst_map.begin(); it != packetInfo.arp.arp_mac_dst_map.end(); it++) {
            printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
        }
        printf("\nARP IP dsts:\n");
        for (auto it = packetInfo.arp.arp_ip_dst_map.begin(); it != packetInfo.arp.arp_ip_dst_map.end(); it++) {
            printf("%s\tCount:%d\n", (*it).first.c_str(), (*it).second);
        }
    }

    /* print out unique UDP src/dst ports */
    if (packetInfo.udpInfo.isUDP) {
        printf("\nUDP src ports:\n");
        for (auto it = packetInfo.udpInfo.udp_src_set.begin(); it != packetInfo.udpInfo.udp_src_set.end(); it++) {
            printf("%d\t", *it);
        }
        printf("\nUDP dst ports:\n");
        for (auto it = packetInfo.udpInfo.udp_dst_set.begin(); it != packetInfo.udpInfo.udp_dst_set.end(); it++) {
            printf("%d\t", *it);
        }
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
