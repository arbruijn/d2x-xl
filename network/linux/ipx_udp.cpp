/* $Id: ipx_udp.c,v 1.10 2004/01/08 16:48:34 schaffner Exp $ */
/*
 *
 * IPX driver for native Linux TCP/IP networking (UDP implementation)
 *   Version 0.99.2
 * Contact Jan [Lace] Kratochvil <short@ucw.cz> for assistance
 * (no "It somehow doesn't work! What should I do?" complaints, please)
 * Special thanks to Vojtech Pavlik <vojtech@ucw.cz> for testing.
 *
 * Also you may see KIX - KIX kix out KaliNix (in Linux-Linux only):
 *   http://atrey.karlin.mff.cuni.cz/~short/sw/kix.c.gz
 *
 * Primarily based on ipx_kali.c - "IPX driver for KaliNix interface"
 *   which is probably mostly by Jay Cotton <jay@kali.net>.
 * Parts shamelessly stolen from my KIX v0.99.2 and GGN v0.100
 *
 * Changes:
 * --------
 * 0.99.1 - now the default IP interface list also contains all point-to-point
 *          links with their destination peer addresses
 * 0.99.2 - commented a bit :-) 
 *        - now adds to IP interface list each host it gets some packet from
 *          which is already not covered by local physical ethernet IP interface
 *        - implemented short-nSignature packet format
 *        - compatibility mode for old D1X releases due to the previous bullet
 *
 * Configuration:
 * --------------
 * No network server software is needed, neither KIX nor KaliNIX.
 *
 *    NOTE: with the change to allow the user to choose the network
 *    driver from the game menu, the following is not anymore precise:
 *    the command line argument "-udp" is only necessary to supply udp
 *    options!
 *
 * Add command line argument "-udp". In default operation D1X will send
 * IP interfaces too all the local interfaces found. But you may also use
 * additional parameter specified after "-udp" to modify the default
 * IP interfaceing style and UDP port numbers binding:
 *
 * ./d1x -udp [@SHIFT]=HOST_LIST   Broadcast ONLY to HOST_LIST
 * ./d1x -udp [@SHIFT]+HOST_LIST   Broadcast both to local ifaces & to HOST_LIST
 *
 * HOST_LIST is a comma (',') separated list of HOSTs:
 * HOST is an IPv4 address (so-called quad like 192.168.1.2) or regular hostname
 * HOST can also be in form 'address:SHIFT'
 * SHIFT sets the UDP port base offset (e.g. +2), can be used to run multiple
 *       clients on one host simultaneously. This SHIFT has nothing to do
 *       with the dynamic-sockets (PgUP/PgDOWN) option in Descent, it's another
 *       higher-level differentiation option.
 *
 * Examples:
 * ---------
 * ./d1x -udp
 *  - Run D1X to participate in Normal local network (Linux only, of course)
 *
 * ./d1x -udp @1=localhost:2 & ./d1x -udp @2=localhost:1
 *  - Run two clients simultaneously fighting each other (only each other)
 *
 * ./d1x -udp =192.168.4.255
 *  - Run distant Descent which will participate with remote network
 *    192.168.4.0 with netmask 255.255.255.0 (IP interface has 192.168.4.255)
 *  - You'll have to also setup hosts in that network accordingly:
 * ./d1x -udp +UPPER_DISTANT_MACHINE_NAME
 *
 * Have fun!
 *
 */
 
//------------------------------------------------------------------------------
/*
The general proceedings of D2X-XL when establishing a UDP/IP communication between two peers 
is as follows:

The sender places the destination address (IP + port) right after the game data in the data 
packet. This happens in UDPSendPacket in the following two lines:

	memcpy (buf + 8 + dataLen, &dest->sin_addr, 4);
	memcpy (buf + 12 + dataLen, &dest->sin_port, 2);

The receiver that way gets to know the IP + port it is sending on. These are not always to be 
determined by the sender itself, as it may sit behind a NAT or proxy, or be using port 0 
(in which case the OS will chose a port for it). The sender's IP + port are stored in the global 
variable ipx_udpSrc (happens in ipx_udp.c::UDPReceivePacket()), which is needed on some special 
occasions.

That's my mechanism to make every participant in a game reliably know about its own IP + port.

All this starts with a client connecting to a server: Consider this the bootstrapping part of 
establishing the communication. This always works, as the client knows the servers' IP + port 
(if it doesn't, no connection ;). The server receives a game info request from the client 
(containing the server IP + port after the game data) and thus gets to know its IP + port. It 
replies to the client, and puts the client's IP + port after the end of the game data.

There is a message nType where the server tells all participants about all other participants; 
that's how clients find out about each other in a game.

When the server receives a participation request from a client, it adds its IP + port to a table 
containing all participants of the game. When the server subsequently sends data to the client, 
it uses that IP + port. 

This however takes only part after the client has sent a game info request and received a game 
info from the server. When the server sends that game info, it hasn't added that client to the 
participants table. Therefore, some game data contains client address data. Unfortunately, this 
address data is not valid in UDP/IP communications, and this is where we need ipx_udpSrc from 
above: It's contents is patched into the game data address. This happens in 
main/network.c::NetworkProcessPacket()) and now is used by the function returning the game info.

the following is the address mangling code from network.c::NetworkProcessPacket(). All packet 
types that do not contain a network address are excluded by the lengthy if statement (I can 
only hope I got the right ones and didn't forget any, but sofar everything seems to work, at 
least on LE machines, so ...)

if	 ((gameStates.multi.nGameType == UDP_GAME) &&
		(pid != PID_LITE_INFO) &&
		(pid != PID_GAME_INFO) &&
		(pid != PID_EXTRA_GAMEINFO) &&
		(pid != PID_PDATA) &&
		(pid != PID_NAKED_PDATA) &&
		(pid != PID_OBJECT_DATA) &&
		(pid != PID_ENDLEVEL) && 
		(pid != PID_ENDLEVEL_SHORT) &&
		(pid != PID_UPLOAD) &&
		(pid != PID_DOWNLOAD) &&
		(pid != PID_ADDPLAYER) &&
		(pid != PID_TRACKER_GET_SERVERLIST) &&
		(pid != PID_TRACKER_ADD_SERVER)
	)
	{
	memcpy (&their->player.network.ipx.server, &ipx_udpSrc.src_network, 10);
	}
*/
//------------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h> /* for htons & co. */
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <stdlib.h>
#ifdef __sun
#  include <alloca.h>
#endif
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef __sun__
#  include <sys/sockio.h>
#endif
#include <net/if.h>
#include <ctype.h>

#ifdef __macosx__
#include <ifaddrs.h>
#include <netdb.h>
#endif

#include "inferno.h"
#include "ipx_udp.h"
#include "ipx_drv.h"
#include "ipx.h"
#include "network.h"
#include "network_lib.h"
#include "tracker.h"
#include "args.h"
#include "u_mem.h"
#include "byteswap.h"
#include "error.h"

extern unsigned char ipx_MyAddress [10];

#define UDPDEBUG	0

#define PORTSHIFT_TOLERANCE 0x100

unsigned char ipx_LocalAddress [10] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
unsigned char ipx_ServerAddress [10] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
int udpBasePorts [2] = {UDP_BASEPORT, UDP_BASEPORT};

/* Packet format: first is the nSignature { 0xD1,'X' } which can be also
 * { 'D','1','X','u','d','p'} for old-fashioned packets.
 * Then follows virtual socket number (as changed during PgDOWN/PgUP) 
 * in network-byte-order as 2 bytes (u_short). After such 4/8 byte header
 * follows raw data as communicated with D1X network core functions.
 */

// Length HAS TO BE 6!
#define D1Xudp "D1Xudp"
// Length HAS TO BE 2!
#define D1Xid "\xD1X"

#define D2XUDP "D2XUDP"

static int nOpenSockets = 0;
#if 0
static int dynamic_socket = 0x401;
#endif
static const int val_one=1;

/* OUR port. Can be changed by "@X [+=]..." argument (X is the shift value)
 */

/* Have we some old D1X in network and have we to maintain compatibility?
 * FIXME: Following scenario is braindead:
 *                                           A  (NEW) , B  (OLD) , C  (NEW)
 *   host A) We start new D1X.               A-newcomm, B-none   , C-none
 *   host B) We start OLD D1X.               A-newcomm, B-oldcomm, C-none
 *   Now host A hears host B and switches:   A-oldcomm, B-oldcomm, C-none
 *   host C) We start new D1X.               A-oldcomm, B-oldcomm, C-newcomm
 *   Now host C hears host A/B and switches: A-oldcomm, B-oldcomm, C-oldcomm
 *   Now host B finishes:                    A-oldcomm, B-none   , C-oldcomm
 *
 * But right now we have hosts A and C, both new code equipped but
 * communicating wastefully by the OLD protocol! Bummer.
 */
#if 0
static char compatibility=0;
#endif

static int have_empty_address () {
	int i;
	for (i = 0; i < 10 && !ipx_MyAddress [i]; i++) ;
	return i == 10;
}

#define MSGHDR "IPX_udp: "

//------------------------------------------------------------------------------

#if 0

static void msg (const char *fmt,...)
{
	va_list ap;

	fputs (MSGHDR,stdout);
	va_start (ap,fmt);
	vprintf (fmt,ap);
	va_end (ap);
	putchar ('\n');
}

#endif

//------------------------------------------------------------------------------

static void chk (void *p)
{
	if (p) return;
	//msg ("FATAL: Virtual memory exhausted!");
	exit (EXIT_FAILURE);
}

//------------------------------------------------------------------------------

int Fail (const char *fmt, ...);

#define FAIL	return Fail

//------------------------------------------------------------------------------

/* Find as much as MAX_BRDINTERFACES during local iface autoconfiguration.
 * Note that more interfaces can be added during manual configuration
 * or host-received-packet autoconfiguration
 */

#define MAX_BRDINTERFACES 16

/* We require the interface to be UP and RUNNING to accept it.
 */

#ifdef __macosx__
#define IF_REQFLAGS (IFF_UP | IFF_RUNNING | IFF_BROADCAST)
#else
#define IF_REQFLAGS (IFF_UP|IFF_RUNNING)
#endif

/* We reject any interfaces declared as LOOPBACK nType.
 */
#define IF_NOTFLAGS (IFF_LOOPBACK)


inline int addreq (struct sockaddr_in *a, struct sockaddr_in *b)
{
if (a->sin_addr.s_addr != b->sin_addr.s_addr)
	return (a->sin_port != b->sin_port) ? 3 : 2;
return (a->sin_port != b->sin_port) ? 1 : 0;
}


inline int addreqm (struct sockaddr_in *a, struct sockaddr_in *b, struct sockaddr_in *m)
{
if ((a->sin_addr.s_addr & m->sin_addr.s_addr) != (b->sin_addr.s_addr & m->sin_addr.s_addr))
	return (a->sin_port != b->sin_port) ? 3 : 2;
return (a->sin_port != b->sin_port) ? 1 : 0;
}


#define MAX_BUF_PACKETS		100

#define PACKET_BUF_SIZE		 (MAX_BUF_PACKETS * MAX_PACKETSIZE)

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
	char						bSafeMode;
	char						bOurSafeMode;
	char						modeCountdown;
#endif
} tDestListEntry;

static tDestListEntry *destList = NULL;

static struct sockaddr_in *broads, broadmasks [MAX_BRDINTERFACES];
static int	destAddrNum = 0,
				masksNum = 0,
				destListSize = 0;
static int broadnum,masksnum,broadsize;

int ReportSafeMode (tDestListEntry *pdl);
#if 0
static void QuerySafeMode (tDestListEntry *pdl);
#endif

//------------------------------------------------------------------------------
/* Dump raw form of IP address/port by fancy output to user
 */
 
#if 0//def _DEBUG

static void dumpraddr (unsigned char *a)
{
unsigned short port;

PrintLog (" [%u.%u.%u.%u",a [0],a [1],a [2],a [3]);
port= (unsigned short)ntohs (* (unsigned short *) (a+4));
if (port) 
	PrintLog (":%u",port);
PrintLog ("]");
}

#endif

//------------------------------------------------------------------------------
/* Like //dumpraddr () but for structure "sockaddr_in"
 */
static unsigned char qhbuf [6];

#if 0

static void dumpaddr (struct sockaddr_in *sinP)
{
unsigned short ports;

memcpy (qhbuf, &sinP->sin_addr, 4);
ports=htons (((short) ntohs (sinP->sin_port)));
memcpy (qhbuf + 4, &ports, 2);
dumpraddr (qhbuf);
}

#endif

//------------------------------------------------------------------------------
/* We'll check whether the "broads" array of destination addresses is now
 * full and so needs expanding.
 */

static void chkbroadsize (void)
{
if (broadnum < broadsize)
	return;
broadsize = broadsize ? broadsize * 2 : 8;
chk (broads = (sockaddr_in *) D2_REALLOC (broads, sizeof (*broads) * broadsize));
}

//------------------------------------------------------------------------------
// We'll check whether the "destList" array of destination addresses is now
// full and so needs expanding.

static int ChkDestListSize (void)
{
	struct tDestListEntry *b;

if (destAddrNum < destListSize)
	return 1;
destListSize = destListSize ? destListSize * 2 : 8;
if (!(b = (tDestListEntry *) D2_ALLOC (sizeof (*destList) * destListSize)))
	 return -1;
if (destList) {
	memcpy (b, destList, sizeof (*destList) * destListSize / 2);
	D2_FREE (destList);
	}
destList = b;
return 1;
}

//------------------------------------------------------------------------------

static int FindDestInList (struct sockaddr_in *destAddr)
{
	int i;

for (i = 0; i < destAddrNum; i++) 
	if ((i < masksNum) ?
		!addreqm (destAddr, &destList [i].addr, broadmasks + i) :
		!addreq (destAddr, &destList [i].addr)) 
	return i;
return destAddrNum;
}

//------------------------------------------------------------------------------

static int AddDestToList (struct sockaddr_in *destAddr)
{
	int				h, i;
	tDestListEntry	*pdl;

if (!destAddr->sin_addr.s_addr) 
	return -1;
if (destAddr->sin_addr.s_addr == htonl (INADDR_BROADCAST))
	return destAddrNum;

for (i = 0; i < destAddrNum; i++) {
	h = (i < masksNum) ?
		 addreqm (destAddr, &destList [i].addr, broadmasks + i) :
		 addreq (destAddr, &destList [i].addr);
	if (h < 2)
		break;
	}
if (i < destAddrNum) {
	if (h)
		destList [i].addr = *destAddr;
	return i;
	}
if (!ChkDestListSize ())
	return -1;
pdl = destList + destAddrNum++;
pdl->addr = *destAddr;
#if UDP_SAFEMODE
pdl->nSent = 0;
pdl->nReceived = 0;
pdl->firstPacket = 0;
pdl->numPackets = 0;
pdl->bSafeMode = -1;
pdl->bOurSafeMode = -1;
pdl->modeCountdown = 1;
#endif
return i;
}

//------------------------------------------------------------------------------

void FreeDestList (void)
{
if (destList) {
	D2_FREE (destList);
	destList = NULL;
	}
destAddrNum =
masksNum =
destListSize = 0;
}

//------------------------------------------------------------------------------
/* This function is called during init and has to grab all system interfaces
 * and collect their IP interface-destination addresses (and their netmasks).
 * Typically it finds only one ethernet card and so returns address in
 * the style "192.168.1.255" with netmask "255.255.255.0".
 * Broadcast addresses are filled into "broads", netmasks to "broadmasks".
 */
 
#ifdef __macosx__

static int addiflist (void)
{
	unsigned 				j;
	struct sockaddr_in	*sinp, *sinmp;

D2_FREE (broads);
/* This code is for Mac OS X, whose BSD layer does bizarre things with variable-length
* structures when calling ioctl using SIOCGIFCOUNT. Or any other architecture that
* has this call, for that matter, since it's much simpler than the other code below.
*/
	struct ifaddrs *ifap, *ifa;

if (getifaddrs (&ifap) != 0)
	FAIL ("Getting list of interface addresses: %m");

// First loop to count the number of valid addresses and allocate enough memory
j = 0;
for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
	// Only count the address if it meets our criteria.
	if (ifa->ifa_flags & IF_NOTFLAGS || !((ifa->ifa_flags & IF_REQFLAGS) && (ifa->ifa_addr->sa_family == AF_INET)))
		continue;
	j++;
	}
broadsize = j;
chk (broads = (sockaddr_in *) D2_ALLOC (j * sizeof (*broads)));
// Second loop to copy the addresses
j = 0;
for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
	// Only copy the address if it meets our criteria.
	if ((ifa->ifa_flags & IF_NOTFLAGS) || !((ifa->ifa_flags & IF_REQFLAGS) && (ifa->ifa_addr->sa_family == AF_INET)))
		continue;
	j++;
	sinp = (struct sockaddr_in *) ifa->ifa_broadaddr;
	sinmp = (struct sockaddr_in *) ifa->ifa_dstaddr;

	// Code common to both getifaddrs () and ioctl () approach
	broads [j] = *sinp;
	broads [j].sin_port = UDP_BASEPORT; //FIXME: No possibility to override from cmdline
	broadmasks [j] = *sinmp;
	j++;
	}
freeifaddrs (ifap);
broadnum = j;
masksnum = j;
return 0;
}

#else // !__maxosx__ -----------------------------------------------------------

static int _ioRes;

#define	_IOCTL(_s,_f,_c)	((_ioRes = ioctl (_s, _f, _c)) != 0)

static int addiflist (void)
{
	unsigned					i, cnt = MAX_BRDINTERFACES;
	struct ifconf 			ifconf;
	int 						sock;
#ifdef _DEBUG
	int						ioRes;
#endif
	unsigned 				j;
	struct sockaddr_in	*sinp, *sinmp;

D2_FREE (broads);
sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
if (sock < 0)
	FAIL ("Creating IP socket failed:\n%m");
#	ifdef SIOCGIFCOUNT
if (!_IOCTL (sock, SIOCGIFCOUNT, &cnt))
	{ /* //msg ("Getting iterface count error: %m"); */ }
else
	cnt = cnt * 2 + 2;
#	endif
ifconf.ifc_len = cnt * sizeof (struct ifreq);
chk (ifconf.ifc_req = (ifreq *) D2_ALLOC (ifconf.ifc_len));
#	ifdef _DEBUG
memset (ifconf.ifc_req, 0, ifconf.ifc_len);
ioRes = ioctl (sock, SIOCGIFCONF, &ifconf);
if (ioRes < 0) {
#	else
 if (!_IOCTL (sock, SIOCGIFCONF, &ifconf)) {
#	endif
	close (sock);
	FAIL ("ioctl (SIOCGIFCONF)\nIP interface detection failed:\n%m");
	}
if (ifconf.ifc_len % sizeof (struct ifreq)) {
	close (sock);
	FAIL ("ioctl (SIOCGIFCONF)\nIP interface detection failed:\n%m");
	}
cnt = ifconf.ifc_len / sizeof (struct ifreq);
chk (broads = (sockaddr_in *) D2_ALLOC (cnt * sizeof (*broads)));
broadsize = cnt;
for (i = j = 0; i < cnt; i++) {
	if (!_IOCTL (sock, SIOCGIFFLAGS, ifconf.ifc_req + i)) {
		close (sock);
		FAIL ("ioctl (UDP (%d),\"%s\",SIOCGIFFLAGS)\nerror: %m", i, ifconf.ifc_req [i].ifr_name);
		}
	if (((ifconf.ifc_req [i].ifr_flags & IF_REQFLAGS) != IF_REQFLAGS) ||
		  (ifconf.ifc_req [i].ifr_flags & IF_NOTFLAGS))
		continue;
	if (!_IOCTL (sock, (ifconf.ifc_req [i].ifr_flags & IFF_BROADCAST) ? SIOCGIFBRDADDR : SIOCGIFDSTADDR, ifconf.ifc_req + i)) {
		close (sock);
		FAIL ("ioctl (UDP (%d),\"%s\",SIOCGIF{DST/BRD}ADDR)\nerror: %m", i, ifconf.ifc_req [i].ifr_name);
		}
	sinp = (struct sockaddr_in *) &ifconf.ifc_req [i].ifr_broadaddr;
	if (!_IOCTL (sock, SIOCGIFNETMASK, ifconf.ifc_req + i)) {
		close (sock);
		FAIL ("ioctl (UDP (%d),\"%s\",SIOCGIFNETMASK)\nerror: %m", i, ifconf.ifc_req [i].ifr_name);
		}
	sinmp = (struct sockaddr_in *)&ifconf.ifc_req [i].ifr_addr;
	if (sinp->sin_family!=AF_INET || sinmp->sin_family!=AF_INET) 
		continue;
	// Code common to both getifaddrs () and ioctl () approach
	broads [j] = *sinp;
	broads [j].sin_port = UDP_BASEPORT; //FIXME: No possibility to override from cmdline
	broadmasks [j] = *sinmp;
	j++;
	}
broadnum = j;
masksnum = j;
return 0;
}

#endif

//------------------------------------------------------------------------------
/* Previous function addiflist () can (and probably will) report multiple
 * same addresses. On some Linux boxes is present both device "eth0" and
 * "dummy0" with the same IP addreesses - we'll filter it here.
 */

static void unifyiflist (void)
{
	int d = 0, s, i;

for (s = 0; s < broadnum; s++) {
	for (i = 0; i < s; i++)
		if (addreq (broads + s, broads + i)) 
			break;
	if (i >= s) 
		broads [d++] = broads [s];
	}
broadnum = d;
}

//------------------------------------------------------------------------------

/* Parse PORTSHIFT numeric parameter
 */

static void portshift (const char *cs)
{
unsigned short port = atol (cs);

if (port<-PORTSHIFT_TOLERANCE || port>+PORTSHIFT_TOLERANCE)
	//msg ("Invalid portshift in \"%s\", tolerance is +/-%d",cs,PORTSHIFT_TOLERANCE)
	;
else 
	port = htons (port);
memcpy (qhbuf + 4, &port, 2);
}

//------------------------------------------------------------------------------

#ifdef __macosx__

static void setupHints (struct addrinfo *hints) {
    hints->ai_family = PF_INET;
    hints->ai_protocol = IPPROTO_UDP;
    hints->ai_socktype = 0;
    hints->ai_flags = 0;
    hints->ai_addrlen = 0;
    hints->ai_addr = NULL;
    hints->ai_canonname = NULL;
    hints->ai_next = NULL;
}

//------------------------------------------------------------------------------

#	if 0

static void printinaddr (struct sockaddr_in *addr) {

    char theAddress [200];
    const char *myResult;
    myResult = inet_ntop (AF_INET, (const void *) &addr->sin_addr, theAddress, 200);
    //myResult = inet_net_ntop (family, addr, 32, theAddress, 200);
    printf ("%s", theAddress);
}

#	endif

#endif

//------------------------------------------------------------------------------
/* Do hostname resolve on name "buf" and return the address in buffer "qhbuf".
 */
static unsigned char *queryhost (char *buf)
{
// For some reason, Mac OS X is finicky with the gethostbyname () call. The getaddrinfo () call is
// apparently newer and seems to work more reliably, but even then it seems to be the case that
// sometimes you can ping <hostname> and get nothing, but then ping <hostname>.local and get
// your address. Hence the kludge until we know what's really going on.
#ifdef __macosx__
    struct addrinfo *info;
    struct addrinfo *ip;
    struct addrinfo hints;
    int error, found;
    
    setupHints (&hints);
    error = 0;
    if (error = getaddrinfo (buf, NULL, &hints, &info) != 0) {
        // Trying again, but appending ".local" to the hostname. Why does this work?
        // AFAIK, this suffix has to do with zeroconf (aka Bonjour aka Rendezvous).
        strcat (buf, ".local");
        setupHints (&hints);
        error = getaddrinfo (buf, NULL, &hints, &info);
    }
    
    if (error)
        return NULL;
    
    // Here's another kludge: for some reason we have to filter out PF_INET6 protocol family
    // entries in the results list. Then we just grab the first regular IPv4 address we find
    // and cross our fingers.
    ip = info;
    found = 0;
    while (!found && ip != NULL) {
        if (ip->ai_family == PF_INET)
            found = 1;
        else
            ip = ip->ai_next;
    }
    
    if (ip == NULL)
        return NULL;
    
	//printf ("Found my address: ");
	//printinaddr ((struct sockaddr_in *)ip->ai_addr);
    memcpy (qhbuf, & (( (struct sockaddr_in *) ip->ai_addr)->sin_addr), 4);
	memset (qhbuf+4,0,2);
    
	freeaddrinfo (info);
    
    return qhbuf;
    
#else
struct hostent *he;
char *s;
char c=0;

	if ((s=strrchr (buf,':'))) {
	  c=*s;
	  *s='\0';
	  portshift (s+1);
	  }
	else memset (qhbuf+4,0,2);
	he=gethostbyname ((char *)buf);
	if (s) *s=c;
	if (!he) {
		//msg ("Error resolving my hostname \"%s\"",buf);
		return (NULL);
		}
	if (he->h_addrtype!=AF_INET || he->h_length!=4) {
	  //msg ("Error parsing resolved my hostname \"%s\"",buf);
		return (NULL);
		}
	if (!*he->h_addr_list) {
	  //msg ("My resolved hostname \"%s\" address list empty",buf);
		return (NULL);
		}
	memcpy (qhbuf, (*he->h_addr_list),4);
	return (qhbuf);
#endif
}

//------------------------------------------------------------------------------
/* Startup... Uninteresting parsing...
 */

int UDPGetMyAddress (void) {

char buf [256];
int i = 0;
char *s,*s2,*ns;

if (!have_empty_address ())
	return 0;
if (!((i=FindArg ("-udp")) && (s=pszArgList [i+1]) && (*s=='=' || *s=='+' || *s=='@'))) 
	s = NULL;
if (gethostname (buf,sizeof (buf))) 
	FAIL ("Error getting my hostname");
if (!(queryhost (buf))) 
	FAIL ("Querying my own hostname \"%s\"",buf);
if (s) 
	while (*s == '@') {
		portshift (++s);
		while (isdigit (*s)) 
			s++;
		}
memset (ipx_MyAddress, 0, 4);
memcpy (ipx_MyAddress + 4, qhbuf, 6);
udpBasePorts [gameStates.multi.bServer] += (short) ntohs (*(unsigned short *) (qhbuf + 4));
if (!s || (s && !*s)) 
	addiflist ();
else {
	struct sockaddr_in *sin;
	if (*s=='+') 
		addiflist ();
	s++;
	for (;;) {
		while (isspace (*s)) 
			s++;
		if (!*s) 
			break;
		for (s2=s;*s2 && *s2!=',';s2++)
			;
		chk (ns = (char *) D2_ALLOC (s2-s+1));
		memcpy (ns,s,s2-s);
		ns [s2-s]='\0';
		if (!queryhost (ns)) 
			//msg ("Ignored IP interface-destination \"%s\" as being invalid",ns);
		D2_FREE (ns);
		chkbroadsize ();
		sin=broads + broadnum++;
		sin->sin_family=AF_INET;
		memcpy (&sin->sin_addr,qhbuf+0,4);
		sin->sin_port=htons (( (short)ntohs (* (unsigned short *) (qhbuf+4)))+UDP_BASEPORT);
		s=s2+ (*s2==',');
		}
	}
unifyiflist ();
return 0;
}

//------------------------------------------------------------------------------

/* We should probably avoid such insanity here as during PgUP/PgDOWN
 * (virtual port number change) we wastefully destroy/create the same
 * socket here (binded always to "udpBasePorts"). FIXME.
 * "nOpenSockets" can be only 0 or 1 anyway.
 */

static int UDPOpenSocket (ipx_socket_t *sk, int port) 
{
	struct sockaddr_in sin;
	u_short nLocalPort, nServerPort;
//	u_long sockBlockMode = 1;	//non blocking

udpBasePorts [1] = UDP_BASEPORT + networkData.nSocket;
nLocalPort = gameStates.multi.bServer ? udpBasePorts [1] : mpParams.udpClientPort;	//override with UDP nLocalPort settings
if (!nOpenSockets)
	if (UDPGetMyAddress () < 0) 
		FAIL ("Error getting my address");

//msg ("OpenSocket on D1X socket nLocalPort %d", nLocalPort);

if (!gameStates.multi.bServer) {
	if (!ChkDestListSize ())
		FAIL ("Error allocating client table");
	nServerPort = udpBasePorts [0] + networkData.nSocket;
	sin.sin_family = AF_INET;
	*((u_short *) (ipx_ServerAddress + 8)) = htons (nServerPort);
	memcpy (&sin.sin_addr.s_addr, ipx_ServerAddress + 4, 4);
	sin.sin_port = htons (nServerPort);
	if (!gameStates.multi.bUseTracker)	
		AddDestToList (&sin);
	}

if ((sk->fd = socket (AF_INET,SOCK_DGRAM, IPPROTO_UDP)) < 0) {
	sk->fd = -1;
	FAIL ("Couldn't create socket on nLocalPort %d.\nError code: %d.", nLocalPort);
	}
//ioctlsocket (sk->fd, FIONBIO, &sockBlockMode);
fcntl (sk->fd, F_SETFL, O_NONBLOCK);
*((u_short *) (ipx_MyAddress + 8)) = nLocalPort;
#ifdef UDP_BROADCAST
if (setsockopt (sk->fd, SOL_SOCKET, SO_BROADCAST, &val_one, sizeof (val_one))) {
	close (sk->fd);
	sk->fd = -1;
	FAIL ("setsockopt (SO_BROADCAST) failed: %m");
	}
#endif
if (gameStates.multi.bServer || mpParams.udpClientPort) {
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl (INADDR_ANY);
	sin.sin_port = htons ((ushort) nLocalPort);
	if (bind (sk->fd, (struct sockaddr *) &sin, sizeof (sin))) {
		close (sk->fd);
		sk->fd = -1;
		FAIL ("bind () to UDP nLocalPort %d failed: %m", nLocalPort);
		}
	memcpy (ipx_MyAddress + 8, &nLocalPort, 2);
	}
nOpenSockets++;
sk->socket = nLocalPort;
return 0;
}

//------------------------------------------------------------------------------
/* The same comment as in previous "UDPOpenSocket"...
 */

static void UDPCloseSocket (ipx_socket_t *mysock) 
{
FreeDestList ();
gameStates.multi.bHaveLocalAddress = 0;
if (!nOpenSockets) {
	//msg ("close w/o open");
	return;
	}
//msg ("CloseSocket on D1X socket port %d",mysock->socket);
if (close (mysock->fd))
	//msg ("close () failed on CloseSocket D1X socket port %d: %m",mysock->socket);
mysock->fd=-1;
if (--nOpenSockets) {
	//msg (" (closesocket) %d sockets left", nOpenSockets);
	return;
	}
}

//------------------------------------------------------------------------------

#if UDP_SAFEMODE

#define SAFEMODE_ID			"D2XUDP:SAFEMODE:??"
#define SAFEMODE_ID_LEN		(sizeof (SAFEMODE_ID) - 1)

int ReportSafeMode (tDestListEntry *pdl)
{
	ubyte buf [40];

memcpy (buf, SAFEMODE_ID, SAFEMODE_ID_LEN);
buf [SAFEMODE_ID_LEN - 2] = extraGameInfo [0].bSafeUDP;
if (pdl->bSafeMode != -1)
	buf [SAFEMODE_ID_LEN - 1] = pdl->bSafeMode;
return sendto (pdl->fd, buf, SAFEMODE_ID_LEN, 0, (struct sockaddr *) &pdl->addr, sizeof (pdl->addr));
}

//------------------------------------------------------------------------------

static void QuerySafeMode (tDestListEntry *pdl)
{
if ((pdl->bSafeMode < 0) && (!--(pdl->modeCountdown))) {
	ubyte buf [40];
	int i;

	memcpy (buf, SAFEMODE_ID, SAFEMODE_ID_LEN);
	i = sendto (pdl->fd, buf, SAFEMODE_ID_LEN, 0, (struct sockaddr *) &pdl->addr, sizeof (pdl->addr));
	pdl->modeCountdown = 2;
	}
}

//------------------------------------------------------------------------------

static int EvalSafeMode (ipx_socket_t *s, struct sockaddr_in *fromAddr, ubyte *buf)
{
	int				i;
	tDestListEntry	*pdl;

if (strncmp (buf, SAFEMODE_ID, SAFEMODE_ID_LEN - 2))
	return 0;
if (destAddrNum <= (i = FindDestInList (fromAddr)))
	return 1;
pdl = destList + i;
pdl->fd = s->fd;
if (buf [SAFEMODE_ID_LEN - 2] == '?')
	QuerySafeMode (pdl);
else
	pdl->bSafeMode = buf [SAFEMODE_ID_LEN - 2];
if (buf [SAFEMODE_ID_LEN - 1] == '?')
	ReportSafeMode (pdl);
else
	pdl->bOurSafeMode = buf [SAFEMODE_ID_LEN - 1];
return 1;
}

#endif

//------------------------------------------------------------------------------
/* Here we'll send the packet to our host. If it is unicast packet, send
 * it to IP address/port as retrieved from IPX address. Otherwise (IP interface)
 * we'll repeat the same data to each host in our IP interfaceing list.
 */

static int UDPSendPacket (
	ipx_socket_t *mysock, IPXPacket_t *ipxHeader, u_char *data, int dataLen) 
{
	static ubyte			buf [MAX_PACKETSIZE + 14 + 4];

	int						iDest, extraDataLen = 0, nUdpRes, bBroadcast;
	tDestListEntry 		*pdl;
	ubyte						*bufP = buf;
	struct sockaddr_in	destAddr, *dest;
#if UDP_SAFEMODE
	tPacketProps			*ppp;
#endif

if ((dataLen < 0) || (dataLen > MAX_PACKETSIZE))
	return -1;
if (gameStates.multi.bTrackerCall) 
	memcpy (buf, data, dataLen);
else {
	memcpy (buf, D2XUDP, 6);
	memcpy (buf + 6, ipxHeader->Destination.Socket, 2);
	memcpy (buf + 8, data, dataLen);
	}
destAddr.sin_family = AF_INET;
memcpy (&destAddr.sin_addr, ipxHeader->Destination.Node, 4);
destAddr.sin_port = *((ushort *) (((ubyte *) &(ipxHeader->Destination.Node)) + 4));
#if UDPDEBUG
msg ("INADDR_BROADCAST = %ld", htonl (INADDR_BROADCAST)); 
msg ("SendPacket to %d.%d.%d.%d:%d, dataLen=%d", 
	  ipxHeader->Destination.Node [0],
	  ipxHeader->Destination.Node [1],
	  ipxHeader->Destination.Node [2],
	  ipxHeader->Destination.Node [3],
	  ntohs (* ((short *) ipxHeader->Destination.Socket)), dataLen);
#endif
memset (& (destAddr.sin_zero), '\0', 8);

if (!(gameStates.multi.bTrackerCall || (AddDestToList (&destAddr) >= 0)))
	return 1;
if (destAddr.sin_addr.s_addr == htonl (INADDR_BROADCAST)) {
	bBroadcast = 1;
	iDest = 0;
	}
else if (destAddrNum <= (iDest = FindDestInList (&destAddr)))
	iDest = -1;
for (; iDest < destAddrNum; iDest++) { 
	if (iDest < 0)
		dest = &destAddr;
	else {
		pdl = destList + iDest;
		dest = &pdl->addr;
		}
	// copy destination IP and port to outBuf
	if (!gameStates.multi.bTrackerCall) {
#if UDP_SAFEMODE
		if (pdl->bOurSafeMode < 0)
			ReportSafeMode (pdl);
		if (pdl->bSafeMode <= 0) {
#endif		
			memcpy (buf + 8 + dataLen, &dest->sin_addr, 4);
			memcpy (buf + 12 + dataLen, &dest->sin_port, 2);
			extraDataLen = 14;
#if UDP_SAFEMODE
			}
		else {
			int bResend = 0;
			memcpy (buf + 16 + dataLen, &dest->sin_addr, 4);
			memcpy (buf + 22 + dataLen, &dest->sin_port, 2);
			extraDataLen = 22;
			if (pdl->numPackets) {
				j = (pdl->firstPacket + pdl->numPackets - 1) % MAX_BUF_PACKETS;
				ppp = pdl->packetProps + j;
				if ((ppp->len == dataLen + extraDataLen) && 
					 !memcmp (ppp->data + 12, buf + 12, ppp->len - 22)) { //+12: skip header data
					bufP = ppp->data;
					bResend = 1;
					}
				}
			if (!bResend) {			
				if (pdl->numPackets < MAX_BUF_PACKETS)
					pdl->numPackets++;
				else
					pdl->firstPacket = (pdl->firstPacket + 1) % MAX_BUF_PACKETS;
				j = (pdl->firstPacket + pdl->numPackets - 1) % MAX_BUF_PACKETS;
				ppp = pdl->packetProps + j;
				ppp->len = dataLen + extraDataLen;
				ppp->data = pdl->packetBuf + j * (MAX_PACKETSIZE + extraDataLen);
				* ((int *) (buf + dataLen + 8)) = INTEL_INT (pdl->nSent);
				memcpy (buf + dataLen + 12, "SAFE", 4);
				memcpy (ppp->data, buf, ppp->len);
				ppp->id = pdl->nSent++;
				pdl->fd = mysock->fd;
				}
			ppp->timeStamp = SDL_GetTicks ();
			}
#endif		
		}

#if UDPDEBUG
	/*printf (MSGHDR "sendto ((%d),Node= [4] %02X %02X,Socket=%02X %02X,s_port=%u,",
		dataLen,
		ipxHeader->Destination.Node  [4],ipxHeader->Destination.Node  [5],
		ipxHeader->Destination.Socket [0],ipxHeader->Destination.Socket [1],
		ntohs (dest->sin_port);
	*/
#endif
	nUdpRes = sendto (mysock->fd, bufP, dataLen + extraDataLen, 0, (struct sockaddr *) dest, sizeof (*dest));
//msg ("sendto (%d) returned %d", dataLen + (gameStates.multi.bTrackerCall ? 0 : 14), iDest);
	if (bBroadcast <= 0) { 
		if (gameStates.multi.bTrackerCall)
			return ((nUdpRes < 1) ? -1 : nUdpRes);
		else
			return ((nUdpRes < extraDataLen + 8) ? -1 : nUdpRes - extraDataLen);
		}
	}
return (dataLen);
}

//------------------------------------------------------------------------------

#if UDP_SAFEMODE

#define RESEND_ID			"D2XUDP:RESEND:"
#define RESEND_ID_LEN	 (sizeof (RESEND_ID) - 1)

static void RequestResend (struct tDestListEntry *pdl, int nLastPacket)
{
	ubyte buf [40];

//PrintLog ("RequestResend (%d, %d)\n", pdl->nReceived, nLastPacket);
memcpy (buf, RESEND_ID, RESEND_ID_LEN);
* ((int *) (buf + RESEND_ID_LEN)) = INTEL_INT (pdl->nReceived);
* ((int *) (buf + RESEND_ID_LEN + 4)) = INTEL_INT (nLastPacket);
sendto (pdl->fd, buf, RESEND_ID_LEN + 8, 0, (struct sockaddr *) &pdl->addr, sizeof (pdl->addr));
}

//------------------------------------------------------------------------------

#define FORGET_ID			"D2XUDP:FORGET:"
#define FORGET_ID_LEN	 (sizeof (FORGET_ID) - 1)

static int DropData (tDestListEntry *pdl, int nDrop)
{
	ubyte	buf [40];

memcpy (buf, FORGET_ID, FORGET_ID_LEN);
* ((int *) (buf + FORGET_ID_LEN)) = INTEL_INT (nDrop);
sendto (pdl->fd, buf, FORGET_ID_LEN + 4, 0, (struct sockaddr *) &pdl->addr, sizeof (pdl->addr));
return 1;
}

//------------------------------------------------------------------------------

static int ForgetData (struct sockaddr_in *fromAddr, ubyte *buf)
{
	int				i, nDrop;
	tDestListEntry	*pdl;

if (!extraGameInfo [0].bSafeUDP)
	return 0;
if (strncmp ((char *) buf, FORGET_ID, FORGET_ID_LEN))
	return 0;
if (destAddrNum <= (i = FindDestInList (fromAddr)))
	return 1;
pdl = destList + i;
nDrop = * ((int *) (buf + FORGET_ID_LEN));
pdl->nReceived = INTEL_INT (nDrop);
return 1;
}

//------------------------------------------------------------------------------

static int ResendData (struct sockaddr_in *fromAddr, ubyte *buf)
{
	int				i, j, nFirst, nLast, nDrop;
	tDestListEntry	*pdl;
	tPacketProps	*ppp;
	time_t			t;

//if (!extraGameInfo [1].bSafeUDP)
//	return 0;
if (strncmp ((char *) buf, RESEND_ID, RESEND_ID_LEN))
	return 0;
if (destAddrNum <= (i = FindDestInList (fromAddr)))
	return 1;
nFirst = * ((int *) (buf + RESEND_ID_LEN));
nFirst = INTEL_INT (nFirst);
nLast = * ((int *) (buf + RESEND_ID_LEN + 4));
nLast = INTEL_INT (nLast);
pdl = destList + i;
if (!pdl->numPackets)
	return 1;
ppp = pdl->packetProps + pdl->firstPacket;
if (nFirst < (nDrop = ppp->id)) {
	DropData (pdl, nDrop);	//have the receiver 'forget' data not buffered any more
	if (nDrop > nLast)
		return 1;
	nFirst = nDrop;
	}
nDrop = -1;
t = SDL_GetTicks ();
#ifdef _DEBUG
//PrintLog ("resending packets %d - %d to", nFirst, nLast); dumpaddr (&pdl->addr); //PrintLog ("\n");
#endif
for (i = pdl->numPackets, j = pdl->firstPacket; i; i--, j++) {
	j %= MAX_BUF_PACKETS;
	ppp = pdl->packetProps + j;
	if (ppp->id > nLast)
		break;
	if (ppp->id < nFirst)
		continue;
	if (t - ppp->timeStamp > 3000) {
		nDrop = ppp->id;
		continue;
		}
	if (nDrop >= 0) {	//have the receiver 'forget' outdated data
		DropData (pdl, nDrop);
		nDrop = -1;
		}
//	nDrop = * ((int *) (ppp->data + ppp->len - 4));
//	nDrop = INTEL_INT (nDrop);
//PrintLog ("   resending packet %d (%d)\n", ppp - pdl->packetProps, pdl->numPackets);
	sendto (pdl->fd, ppp->data, ppp->len, 0, (struct sockaddr *) &pdl->addr, sizeof (pdl->addr));
	}
return 1;
}

#endif //UDP_SAFEMODE

//------------------------------------------------------------------------------
/* Here we will receive new packet to the given buffer. Both formats of packets
 * are supported, we fallback to old format when first obsolete packet is seen.
 * If the (valid) packet is received from unknown host, we will add it to our
 * IP interfaceing list. FIXME: For now such autoconfigured hosts are NEVER removed.
 */

static int UDPReceivePacket (
	ipx_socket_t *s, char *outBuf, int outBufSize, struct ipx_recv_data *rd) 
{
	struct sockaddr_in 	fromAddr;
	int						i, dataLen, bTracker;  
	unsigned int			fromAddrSize = sizeof (fromAddr);
	unsigned short 		srcPort;
	//char 						szIP [30];
#if UDP_SAFEMODE
	tDestListEntry			pdl;
	int						packetId = -1, bSafeMode = 0;
#endif

dataLen = recvfrom (s->fd, outBuf, outBufSize, 0, (struct sockaddr *) &fromAddr, &fromAddrSize);
if (0 > dataLen) {
		return -1;
	}
bTracker = IsTracker (* ((unsigned int *) &fromAddr.sin_addr), * ((ushort *) &fromAddr.sin_port));
#if UDPDEBUG
	//msg ("ReceivePacket, dataLen=%d", dataLen);
	//printf (MSGHDR "recvfrom ((%d-8=%d),",dataLen,dataLen-8);
	//dumpaddr (&fromAddr);
	//puts (").");
#endif
if (fromAddr.sin_family!=AF_INET) 
	return -1;
if ((dataLen < 6) || (!bTracker && (memcmp (outBuf, D2XUDP, 6) 
#if UDP_SAFEMODE
    || EvalSafeMode (s, &fromAddr, outBuf)
#endif    
    )))
	return -1;
if (!(bTracker
#if UDP_SAFEMODE
	 || ResendData (&fromAddr, outBuf) || ForgetData (&fromAddr, outBuf)
#endif	 
	 )) {
	rd->src_socket = ntohs (* (unsigned short *) (outBuf + 6));
	rd->dst_socket = s->socket;
	srcPort = ntohs (fromAddr.sin_port);
	memcpy (ipx_LocalAddress + 4, outBuf + dataLen - 6, 6);
	// check if the packet came from ourself
	if (!memcmp (&fromAddr.sin_addr, ipx_LocalAddress + 4, 4) &&
		 !memcmp (&srcPort, ipx_LocalAddress + 8, 2))
		return -1;
	// add sender to client list if the packet is not from myself
	i = AddDestToList (&fromAddr);
	if (i < 0)
		return -1;
	if (i < destAddrNum) {	//i.e. sender already in list or successfully added to list
#if UDP_SAFEMODE
		bSafeMode = 0;
		*pdl = destList + i;
		pdl->fd = s->fd;
		pdl->bOurSafeMode = (memcmp (outBuf + dataLen - 10, "SAFE", 4) == 0);
		if (pdl->bOurSafeMode != extraGameInfo [0].bSafeUDP)
			ReportSafeMode (pdl);
		if (pdl->bOurSafeMode == 1) {
			bSafeMode = 1;
			packetId = *((int *) (outBuf + dataLen - 14));
			packetId = INTEL_INT (packetId);
			//PrintLog ("received packet %d (%d) from ", packetId, pdl->nReceived); dumpaddr (&pdl->addr); //PrintLog ("\n");
			if (packetId == pdl->nReceived)
				pdl->nReceived++;
			else if (packetId > pdl->nReceived) {
				RequestResend (pdl, packetId);
				return -1;
				}
			}
#endif		
		}
	gameStates.multi.bHaveLocalAddress = 1;
	memcpy (netPlayers.players [gameData.multiplayer.nLocalPlayer].network.ipx.node, ipx_LocalAddress + 4, 6);
#if UDP_SAFEMODE
	dataLen -= bSafeMode ? 22 : 14;
#else
	dataLen -= 14;
#endif
	memcpy (outBuf, outBuf + 8, dataLen);
	} //bTracker
#ifdef _DEBUG
else
	bTracker = 1;
#endif
memset (rd->src_network, 0, 4);
memcpy (rd->src_node, &fromAddr.sin_addr, 4);
memcpy (rd->src_node + 4, &fromAddr.sin_port, 2);
rd->pktType = 0;
#if UDPDEBUG
//printf (MSGHDR "ReceivePacket: dataLen=%d,from=",dataLen);
//dumpraddr (rd->src_node);
//putchar ('\n');
#endif
return dataLen;
}

//------------------------------------------------------------------------------

int UDPPacketReady (ipx_socket_t *s) 
{
	unsigned long nAvailBytes = 0;

return !fcntl (s->fd, FIONREAD, &nAvailBytes) && (nAvailBytes > 0);
}

//------------------------------------------------------------------------------

struct ipx_driver ipx_udp = {
	UDPGetMyAddress,
	UDPOpenSocket,
	UDPCloseSocket,
	UDPSendPacket,
	UDPReceivePacket,
#if 0
	UDPPacketReady,
#else
	IxpGeneralPacketReady,
#endif
	NULL,	// InitNetgameAuxData
	NULL,	// HandleNetgameAuxData
	NULL,	// HandleLeaveGame
	NULL	// SendGamePacket
};

//------------------------------------------------------------------------------
//eof
