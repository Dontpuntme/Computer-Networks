#include "wireview.h"
#include "pcap.h"
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

void packetHandler(char *userData, const struct pcap_pkthdr* pkthdr, const char* packet)
{

};
int main(int argc, char** argv) {

		char *dev, errbuf[PCAP_ERRBUF_SIZE];
        char* filename;
        pcap_t* desc;
        // opens pcap for given file name errbuff is just used and filled if there is an error
		if (desc = pcap_open_offline(filename,errbuf) == NULL) {
			printf(errbuf);
			return(2);
		}
        // file session created by pcap_open int for number of packets processed a negative number
        //meants process packets until another conditions comes up packetHandler is run on each packet
        //idk for the last part
        pcap_loop(desc,-1,packetHandler,NULL);
        pcap_close(desc);

}
