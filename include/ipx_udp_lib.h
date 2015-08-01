#ifndef _IPX_UDP_LIB_H
#define _IPX_UDP_LIB_H

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <carray.h>

#ifdef _WIN32
//#	include <winsock2.h>
#else
#	include <sys/socket.h>
#endif

#include "../win32/include/ipx_drv.h"
#include "../linux/include/ipx_udp.h"
#include "ipx.h"
#include "descent.h"
#include "network.h"
#include "network_lib.h"
#include "tracker.h"
#include "error.h"
#include "args.h"
#include "u_mem.h"
#include "byteswap.h"

#define MSGHDR						"IPX_udp: "

#define MAX_BUF_PACKETS		100

#define PACKET_BUF_SIZE		(MAX_BUF_PACKETS * MAX_PACKET_SIZE)

typedef struct tPacketProps {
	long						id;
	ubyte						*data;
	short						len;
	time_t					timeStamp;
} tPacketProps;

typedef struct tDestListEntry {
	struct sockaddr_in	addr;
#if UDP_SAFEMODE
	tPacketProps			packetProps [MAX_BUF_PACKETS];
	ubyte						packetBuf [PACKET_BUF_SIZE];
	long						nSent;
	long						nReceived;
	short						firstPacket;
	short						numPackets;
	int						fd;
	char						bSafeMode;		//safe mode a peer of the local CPlayerData uses (unknown: -1)
	char						bOurSafeMode;	//our safe mode as that peer knows it (unknown: -1)
	char						modeCountdown;
#endif
} tDestListEntry;

extern int FindDestInList (struct sockaddr_in *destAddr);
extern int AddDestToList (struct sockaddr_in *destAddr);
extern void FreeDestList (void);
extern int BuildInterfaceList (void);
extern void UnifyInterfaceList (void);
extern void PortShift (const char *pszShift);
ubyte* QueryHost (char* buf);
extern void DumpRawAddr (ubyte *a);
int _CDECL_ Fail (const char *fmt, ...);

#endif //_IPX_UDP_LIB_H

