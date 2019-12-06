#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <bits/stdc++.h> 
#include <netinet/if_ether.h>
#include <sys/stat.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>

#define ETH_ADDR_LEN 6
#define ETH_HEAD_LEN 14
#define DEFAULT_UDP_PORT 34567
#define MAX_SEGMENT_SIZE 1000

void printUsage();
int runRouter(char* ipMappings);
int runEndHost(char* routerIP, char* hostIP, uint32_t ttl);
int routePacket(char* packet, std::vector<std::string> &overlayIPs, std::vector<std::string> &vmIPs);
void readUDP(char* packet);
void sendUDP(char* routerIP,char* sourceaddr, char* destaddr, uint32_t ttl,char* data, uint32_t datalen,uint16_t id);
bool lookForFileAndProcess(char* routerIP,char* sourceaddr, char* destaddr, uint32_t ttl);