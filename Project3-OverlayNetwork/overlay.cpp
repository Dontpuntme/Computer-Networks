#include "overlay.h"
#include <getopt.h>
#include "getopt.h"
#include <iostream>
#include <fstream>
#include "string.h"

void printUsage() {
    std::cout << "Router Usage: ./overlay --router <ip mappings>" << std::endl;
    std::cout << "End-Host Usage: ./overlay --host <router ip> <host ip> <ttl>" << std::endl;
}

int main(int argc, char** argv) {
    bool router = false;
    bool host = false;
    char* ipMappings;
    char* routerIP;
    char* hostIP;
    uint32_t ttl;

    if (argc <= 1) {
        printUsage();
        exit(1);
    }
    if (argc > 1) {
        const char* const short_opts = "r:h:";
        const option long_opts[] = {
            {"router", required_argument, nullptr, 'r'},
            {"host", required_argument, nullptr, 'h'}
        };
        while(true) {
            const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
            if(-1 == opt) break;
            switch (opt) {
            case 'h':
                if (argc < 4) {
                    printUsage();
                    return 1;
                }
                else {
                    routerIP = argv[2];
                    hostIP = argv[3];
                    ttl = atoi(argv[4]);
                    printf("Running in host mode with routerIP:%s | hostIP:%s | ttl:%d\n", routerIP, hostIP, ttl);
                    host = true;
                    runEndHost(routerIP, hostIP, ttl);
                    break;
                }
            case 'r':
                if(argc < 2){
                    printUsage();
                    return 1;
                }
                else {
                    ipMappings = argv[2];
                    printf("Running in router mode with IP mappings: %s\n", ipMappings);
                    router = true;
                    runRouter(ipMappings);
                    break;
                }                
            default:
                printUsage();
                break;
            }
        }
    }
    return 0;
}

/* run process as router */
int runRouter(char* ipMappings) {

}

/* run process as end-host */
int runEndHost(char* routerIP, char* hostIP, uint32_t ttl) {

}