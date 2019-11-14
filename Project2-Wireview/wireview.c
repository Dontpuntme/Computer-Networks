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
    printf("IP src:%s\t IP dst:%s\n", ip_src, ip_dst);


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
