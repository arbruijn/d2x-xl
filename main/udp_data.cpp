#include "ipx_udp.h"

unsigned char ipx_LocalAddress[10] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
unsigned char ipx_ServerAddress[10] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
int gameStates.multi.bHaveLocalAddress = 0;
int udpBasePort = UDP_BASEPORT;
int udpClientPort = UDP_BASEPORT;
