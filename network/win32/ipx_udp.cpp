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
 * 0.99.1 - now the default broadcast list also contains all point-to-point
 *          links with their destination peer addresses
 * 0.99.2 - commented a bit :-) 
 *        - now adds to broadcast list each host it gets some packet from
 *          which is already not covered by local physical ethernet broadcast
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
 * broadcasts too all the local interfaces found. But you may also use
 * additional parameter specified after "-udp" to modify the default
 * broadcasting style and UDP port numbers binding:
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
 *    192.168.4.0 with netmask 255.255.255.0 (broadcast has 192.168.4.255)
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
#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <carray.h>

#ifdef _WIN32
#	include <winsock2.h>
#else
#	include <sys/socket.h>
#endif

#include "../win32/include/ipx_drv.h"
#include "../linux/include/ipx_udp.h"
#include "ipx.h"
#include "inferno.h"
#include "network.h"
#include "network_lib.h"
#include "tracker.h"
#include "error.h"
#include "args.h"
#include "u_mem.h"
#include "byteswap.h"

ubyte ipx_LocalAddress[10] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
ubyte ipx_ServerAddress[10] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};

// #define UDPDEBUG

#define PORTSHIFT_TOLERANCE	0x100

/* Packet format: first is the nSignature { 0xD1,'X' } which can be also
 * { 'D','1','X','u','d','p'} for old-fashioned packets.
 * Then follows virtual socket number (as changed during PgDOWN/PgUP)
 * in network-byte-order as 2 bytes (u_short). After such 4/8 byte header
 * follows raw data as communicated with D1X network core functions.
 */

#define D2XUDP "D2XUDP"

static int nOpenSockets = 0;
static const int val_one=1;

//------------------------------------------------------------------------------
// OUR port. Can be changed by "@X[+=]..." argument (X is the shift value)

int udpBasePorts [2] = {UDP_BASEPORT, UDP_BASEPORT};

static int have_empty_address() 
{
	int i;

for (i = 0; (i < 10); i++)
	if (ipx_MyAddress [i])
		return 0;
return 1;
}

#define MSGHDR "IPX_udp: "

//------------------------------------------------------------------------------

static void _CDECL_ msg(const char *fmt,...)
{
	va_list ap;

	fputs(MSGHDR,stdout);
	va_start(ap,fmt);
	vprintf(fmt,ap);
	va_end(ap);
	//putchar('\n');
}

//------------------------------------------------------------------------------

static void chk(void *p)
{
	if (p) return;
	msg("FATAL: Virtual memory exhausted!");
	exit(EXIT_FAILURE);
}

//------------------------------------------------------------------------------

static char szFailMsg [1024];

static int _CDECL_ Fail (const char *fmt, ...)
{
   va_list  argP;

va_start (argP, fmt);
vsprintf (szFailMsg, fmt, argP);
va_end (argP);   
MsgBox (NULL, NULL, 1, "OK", "UDP Error\n\n%s\nError code: %d", szFailMsg, WSAGetLastError ());
//Warning ("%s\nError code: %d", szFailMsg, WSAGetLastError ());
return 1;
}

#define FAIL	return Fail

//------------------------------------------------------------------------------

/* Find as much as MAX_BRDINTERFACES during local iface autoconfiguration.
 * Note that more interfaces can be added during manual configuration
 * or host-received-packet autoconfiguration
 */

#define MAX_BRDINTERFACES 16

/* We require the interface to be UP and RUNNING to accept it.
 */

#define IF_REQFLAGS (IFF_UP|IFF_RUNNING)

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

#define PACKET_BUF_SIZE		(MAX_BUF_PACKETS * MAX_PACKETSIZE)

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

static CArray<tDestListEntry> destList;

static struct sockaddr_in broadmasks [MAX_BRDINTERFACES];

static int	destAddrNum = 0,
				masksNum = 0,
				destListSize = 0;

//------------------------------------------------------------------------------
// We'll check whether the "destList" array of destination addresses is now
// full and so needs expanding.

static int ChkDestListSize(void)
{
if (destAddrNum < destListSize)
	return 1;
destList.Resize (destListSize = destListSize ? destListSize * 2 : 8);
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
pdl = &destList [destAddrNum++];
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
destList.Destroy ();
destAddrNum =
masksNum =
destListSize = 0;
}

//------------------------------------------------------------------------------
// This function is called during init and has to grab all system interfaces
// and collect their broadcast-destination addresses (and their netmasks).
// Typically it founds only one ethernet card and so returns address in
// the style "192.168.1.255" with netmask "255.255.255.0".
// Broadcast addresses are filled into "destList", netmasks to "broadmasks".

/* Stolen from my GGN */
static int addiflist(void)
{
#if 0
	unsigned cnt=MAX_BRDINTERFACES,i,j;
	WSADATA info;
	INTERFACE_INFO *ifo;
	SOCKET sock;
	struct sockaddr_in *sinp,*sinmp;

	destList.Destroy ();
	if ((sock=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))<0)
		FAIL("Creating socket() failure during broadcast detection.");

#ifdef SIOCGIFCOUNT
	if (ioctl(sock,SIOCGIFCOUNT,&cnt))
	 { /* msg("Getting iterface count error."); */ }
	else
		cnt=cnt*2+2;
#endif

	memset(&ifo[0], 0, sizeof(ifo);
	chk (ifo = new INTERFACE_INFO [cnt]);

	if (wsaioctl(sock, SIO_GET_INTERFACE_LIST, NULL, 0, &ifo[0], cnt * sizeof(INTERFACE_INFO), &br, NULL, NULL)) != 0) {
		closesocket(sock);
		FAIL("ioctl(SIOCGIFCONF) failure during broadcast detection.");
		}
#if 0
	cnt=ifconf.ifc_len/sizeof(struct ifreq);
#endif
	destList.Create (cnt);
	chk (destList.Buffer ());
	destListSize=cnt;
	for (i=j=0;i<cnt;i++) {
		if (ioctl(sock,SIOCGIFFLAGS,ifconf.ifc_req+i)) {
			closesocket(sock);
			FAIL("ioctl(udp,\"%s\",SIOCGIFFLAGS) error.",ifconf.ifc_req[i].ifr_name);
			}
		if (((ifconf.ifc_req[i].ifrFlags&IF_REQFLAGS)!=IF_REQFLAGS)||
				 (ifconf.ifc_req[i].ifrFlags&IF_NOTFLAGS))
			continue;
		if (ioctl(sock,(ifconf.ifc_req[i].ifrFlags&IFF_BROADCAST?SIOCGIFBRDADDR:SIOCGIFDSTADDR),ifconf.ifc_req+i)) {
			closesocket(sock);
			FAIL("ioctl(udp,\"%s\",SIOCGIF{DST/BRD}ADDR) error.",ifconf.ifc_req[i].ifr_name);
			}

		sinp = reinterpret_cast<struct sockaddr_in*> (&ifconf.ifc_req[i].ifr_broadaddr);
#if 0 // old, not portable code
		sinmp = reinterpret_cast<struct sockaddr_in*> (&ifconf.ifc_req[i].ifr_netmask);
#else // portable code
		if (ioctl(sock, SIOCGIFNETMASK, ifconf.ifc_req+i)) {
			closesocket(sock);
			FAIL("ioctl(udp,\"%s\",SIOCGIFNETMASK) error.", ifconf.ifc_req[i].ifr_name);
		}
		sinmp = reinterpret_cast<struct sockaddr_in*> (&ifconf.ifc_req[i].ifr_addr);
#endif
		if (sinp->sin_family!=AF_INET || sinmp->sin_family!=AF_INET) continue;
		destList [j]=*sinp;
		destList [j].sin_port=UDP_BASEPORT; //FIXME: No possibility to override from cmdline
		broadmasks[j]=*sinmp;
		j++;
		}
	destAddrNum=j;
	masksNum=j;
#endif
	return(0);
}

//------------------------------------------------------------------------------
// Previous function addiflist() can (and probably will) report multiple
// same addresses. On some Linux boxes is present both device "eth0" and
// "dummy0" with the same IP addreesses - we'll filter it here.

static void unifyiflist(void)
{
int d=0,s,i;

for (s = 0; s < destAddrNum; s++) {
	for (i = 0; i < s; i++)
		if (addreq (&destList [s].addr, &destList [i].addr)) 
			break;
	if (i>=s) 
		destList [d++] = destList [s];
	}
destAddrNum=d;
}

static ubyte qhbuf[6];

//------------------------------------------------------------------------------
// Parse PORTSHIFT numeric parameter

static void portshift(const char *cs)
{
int port;
ushort srcPort=0;

	port=atoi(cs);
	if (port<-PORTSHIFT_TOLERANCE || port>+PORTSHIFT_TOLERANCE)
		msg("Invalid portshift in \"%s\", tolerance is +/-%d",cs,PORTSHIFT_TOLERANCE);
	else 
		srcPort=(ushort) htons((u_short) port);
	memcpy(qhbuf+4,&srcPort,2);
}

//------------------------------------------------------------------------------
// Do hostname resolve on name "buf" and return the address in buffer "qhbuf".
 
ubyte *queryhost(char *buf)
{
struct hostent *he;
char *s;
char c=0;

	if ((s=strrchr(buf,':'))) {
		c=*s;
		*s='\0';
		portshift(s+1);
		}
	else 
		memset(qhbuf+4,0,2);
	he=gethostbyname(reinterpret_cast<char*> (buf));
	if (s) 
		*s=c;
	if (!he) {
		msg("Error resolving my hostname \"%s\"",buf);
		return(NULL);
		}
	if (he->h_addrtype!=AF_INET || he->h_length!=4) {
		msg("Error parsing resolved my hostname \"%s\"",buf);
		return(NULL);
		}
	if (!*he->h_addr_list) {
		msg("My resolved hostname \"%s\" address list empty",buf);
		return(NULL);
		}
	memcpy(qhbuf,(*he->h_addr_list),4);
	return(qhbuf);
}

//------------------------------------------------------------------------------
// Dump raw form of IP address/port by fancy output to user

static void dumpraddr(ubyte *a)
{
short port;

PrintLog ("[%u.%u.%u.%u]", a[0],a[1],a[2],a[3]);
console.printf (0, "[%u.%u.%u.%u]", a[0],a[1],a[2],a[3]);
port=(signed short)ntohs (*reinterpret_cast<ushort*> (a+4));
if (port) {
	PrintLog (":%+d",port);
	console.printf (0, ":%+d",port);
	}
PrintLog ("\n");
}

//------------------------------------------------------------------------------
// Like dumpraddr() but for structure "sockaddr_in"
#if 0
static void dumpaddr(struct sockaddr_in *sin)
{
ushort srcPort;

memcpy(qhbuf, &sin->sin_addr, 4);
srcPort = htons ((u_short) (ntohs (sin->sin_port) - UDP_BASEPORT));
memcpy(qhbuf + 4, &srcPort, 2);
dumpraddr (qhbuf);
}
#endif
//------------------------------------------------------------------------------
// Startup... Uninteresting parsing...

int UDPGetMyAddress(void) 
{

char buf[256];
int i;
char *s,*s2,*ns;

if (!have_empty_address())
	return 0;
if (!((i = FindArg("-udp")) && (s = pszArgList [i + 1]) && ((*s == '=') || (*s == '+') || (*s == '@')))) 
	s = NULL;
if (gethostname (buf, sizeof (buf))) 
	FAIL("Error getting my hostname");
	if (!(queryhost (buf))) 
		FAIL("Querying my own hostname \"%s\"",buf);
if (s) 
	while (*s=='@') {
		portshift (++s);
		while (::isdigit (*s)) 
			s++;
		}
memset(ipx_MyAddress, 0, 4);
memcpy(ipx_MyAddress + 4, qhbuf, 6);
//udpBasePorts [gameStates.multi.bServer] += (short)ntohs(*reinterpret_cast<ushort*> (qhbuf+4));
if (!(s && *s)) 
	addiflist();
else {
	struct sockaddr_in *sin;
	if (*s=='+') 
		addiflist ();
	s++;
	for (;;) {
		while (::isspace (*s)) 
			s++;
		if (!*s) 
			break;
		for (s2 = s; *s2 && *s2 != ','; s2++)
			;
		chk (ns = new char [s2 - s + 1]);
		memcpy(ns, s, s2 - s);
		ns[s2-s]='\0';
		if (!queryhost(ns)) 
			msg ("Ignored broadcast-destination \"%s\" as being invalid", ns);
		delete[] ns;
		sin = &destList [destAddrNum].addr;
		sin->sin_family = AF_INET;
		memcpy(&sin->sin_addr, qhbuf, 4);
		sin->sin_port = htons ((u_short) (ntohs (*(reinterpret_cast<u_short*> (qhbuf + 4))) + UDP_BASEPORT));
		if (AddDestToList (sin) < 0)
			FAIL ("Error allocating client table");
		s=s2+(*s2==',');
		}
	}
unifyiflist();
return 0;
}

//------------------------------------------------------------------------------
// We should probably avoid such insanity here as during PgUP/PgDOWN
// (virtual port number change) we wastefully destroy/create the same
// socket here (bound always to "udpBasePorts"). FIXME.
// "nOpenSockets" can be only 0 or 1 anyway.
 
static int UDPOpenSocket (ipx_socket_t *sk, int port) 
{
	struct sockaddr_in sin;
	u_long sockBlockMode = 1;	//non blocking
#if 0 //for testing only
	static ubyte inAddrLoopBack [4] = {127,0,0,1};
#endif
	u_short	nLocalPort, nServerPort;


udpBasePorts [1] = UDP_BASEPORT + networkData.nSocket;	//server port as set by the server
nLocalPort = gameStates.multi.bServer ? udpBasePorts [1] : mpParams.udpClientPort;
gameStates.multi.bHaveLocalAddress = 0;
if (!nOpenSockets)
	if (UDPGetMyAddress() < 0) {
		FAIL ("Couldn't get my address.");
		}

msg("OpenSocket on D1X socket nLocalPort %d", nLocalPort);

if (!gameStates.multi.bServer) {		//set up server address and add it to destination list
	if (!ChkDestListSize ())
		FAIL ("Error allocating client table");
	nServerPort = udpBasePorts [0] + networkData.nSocket;
	*(reinterpret_cast<u_short*> (ipx_ServerAddress + 8)) = htons (nServerPort);
	sin.sin_family = AF_INET;
	memcpy (&sin.sin_addr.s_addr, ipx_ServerAddress + 4, 4);
	sin.sin_port = htons (nServerPort);
	if (!gameStates.multi.bUseTracker)
		AddDestToList (&sin);
	}

if (0 > (sk->fd = (int) socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP))) {
	sk->fd = -1;
	FAIL ("Couldn't create socket on nLocalPort %d.\nError code: %d.", nLocalPort);
	}
ioctlsocket (sk->fd, FIONBIO, &sockBlockMode);
*(reinterpret_cast<u_short*> (ipx_MyAddress + 8)) = nLocalPort;
#ifdef UDP_BROADCAST
if (setsockopt (sk->fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*> (&val_one), sizeof (val_one))) {
	closesocket (sk->fd);
	sk->fd = -1;
	FAIL ("Setting broadcast socket option failed.");
	}
#endif
if (gameStates.multi.bServer || mpParams.udpClientPort) {
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl (INADDR_ANY); //ipx_ServerAddress + 4);
	sin.sin_port = htons (nLocalPort);
	if (bind (sk->fd, reinterpret_cast<struct sockaddr*> (&sin), sizeof (sin))) {
		closesocket (sk->fd);
		sk->fd = -1;
		FAIL ("Couldn't bind to nLocalPort %d.", nLocalPort);
		}
	memcpy (ipx_MyAddress + 8, &nLocalPort, 2);
	}
PrintLog ("Opened UDP connection (socket %d, port %d)\n", sk->fd, nLocalPort);
nOpenSockets++;
sk->socket = nLocalPort;
return 0;
}

//------------------------------------------------------------------------------
// The same comment as in previous "UDPOpenSocket"...

static void UDPCloseSocket (ipx_socket_t *mysock) 
{
FreeDestList ();
gameStates.multi.bHaveLocalAddress = 0;
if (!nOpenSockets) {
	msg("close w/o open");
	return;
	}
msg("CloseSocket on D1X socket port %d",mysock->socket);
if (closesocket(mysock->fd))
	msg("closesocket() failed on CloseSocket D1X socket port %d.",mysock->socket);
mysock->fd=-1;
if (--nOpenSockets) {
	msg("(closesocket) %d sockets left", nOpenSockets);
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
return sendto (pdl->fd, buf, SAFEMODE_ID_LEN, 0, reinterpret_cast<struct sockaddr*> (&pdl->addr), sizeof (pdl->addr));
}

//------------------------------------------------------------------------------

static void QuerySafeMode (tDestListEntry *pdl)
{
if ((pdl->bSafeMode < 0) && (!--(pdl->modeCountdown))) {
	ubyte buf [40];
	int i;

	memcpy (buf, SAFEMODE_ID, SAFEMODE_ID_LEN);
	i = sendto (pdl->fd, buf, SAFEMODE_ID_LEN, 0, reinterpret_cast<struct sockaddr*> (&pdl->addr), sizeof (pdl->addr));
	pdl->modeCountdown = 2;
	}
}

//------------------------------------------------------------------------------

static int EvalSafeMode (ipx_socket_t *s, struct sockaddr_in *fromAddr, ubyte *buf)
{
	int				i, bReport = 0;
	tDestListEntry	*pdl;

if (strncmp (buf, SAFEMODE_ID, SAFEMODE_ID_LEN - 2))
	return 0;
if (destAddrNum <= (i = FindDestInList (fromAddr)))
	return 1;
pdl = destList + i;
pdl->fd = s->fd;
if (buf [SAFEMODE_ID_LEN - 2] == '?')
	bReport = 1;
else if (pdl->bSafeMode != buf [SAFEMODE_ID_LEN - 2]) {
	bReport = 1;
	pdl->bSafeMode = buf [SAFEMODE_ID_LEN - 2];
	}
if (buf [SAFEMODE_ID_LEN - 1] == '?')
	bReport = 1;
else if (pdl->bOurSafeMode != buf [SAFEMODE_ID_LEN - 1]) {
	bReport = 1;
	pdl->bOurSafeMode = buf [SAFEMODE_ID_LEN - 1];
	}
if (bReport)
	ReportSafeMode (pdl);
return 1;
}

#endif

//------------------------------------------------------------------------------
// Here we'll send the packet to our host. If it is unicast packet, send
// it to IP address/port as retrieved from IPX address. Otherwise (broadcast)
// we'll repeat the same data to each host in our broadcasting list.

static int UDPSendPacket
	(ipx_socket_t *mysock, IPXPacket_t *ipxHeader, u_char *data, int dataLen) 
{
 	struct sockaddr_in destAddr, *dest;
#if DBG
	int h;
#endif
	int				iDest, nUdpRes, extraDataLen = 0, bBroadcast = 0;
	tDestListEntry *pdl;
#if UDP_SAFEMODE
	tPacketProps	*ppp;
	int				j;
#endif
	static ubyte	buf [MAX_PACKETSIZE];
	ubyte				*bufP = buf;

#ifdef UDPDEBUG
msg("SendPacket enter, dataLen=%d",dataLen);
#endif
if ((dataLen < 0) || (dataLen > MAX_DATASIZE))
	return -1;
if (gameStates.multi.bTrackerCall) 
	memcpy (buf, data, dataLen);
else {
	memcpy (buf, D2XUDP, 6);
	memcpy (buf + 6, ipxHeader->Destination.Socket, 2);	//telling the receiver *my* port number here
	memcpy (buf + 8, data, dataLen);
	}
destAddr.sin_family = AF_INET;
memcpy (&destAddr.sin_addr, ipxHeader->Destination.Node, 4);
destAddr.sin_port = *(reinterpret_cast<ushort*> (ipxHeader->Destination.Node + 4));
memset (&(destAddr.sin_zero), '\0', 8);

if (!(gameStates.multi.bTrackerCall || (AddDestToList (&destAddr) >= 0)))
	return -1;
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
		pdl = &destList [iDest];
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
			memcpy (buf + 20 + dataLen, &dest->sin_port, 2);
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
				ppp->data = pdl->packetBuf + j * MAX_PACKETSIZE;
				*(reinterpret_cast<int*> (buf + dataLen + 8)) = INTEL_INT (pdl->nSent);
				memcpy (buf + dataLen + 12, "SAFE", 4);
				memcpy (ppp->data, buf, ppp->len);
				ppp->id = pdl->nSent++;
				pdl->fd = mysock->fd;
				}
			ppp->timeStamp = SDL_GetTicks ();
			}
#endif
		}

#ifdef UDPDEBUG
	/*printf(MSGHDR "sendto((%d),Node=[4] %02X %02X,Socket=%02X %02X,s_port=%u,",
		dataLen,
		ipxHeader->Destination.Node  [4],ipxHeader->Destination.Node [5],
		ipxHeader->Destination.Socket[0],ipxHeader->Destination.Socket [1],
		ntohs (dest->sin_port);
	*/
#endif
	nUdpRes = sendto (mysock->fd, reinterpret_cast<const char*> (bufP), dataLen + extraDataLen, 0, reinterpret_cast<struct sockaddr*> (dest), sizeof (*dest));
#if DBG
	if (!gameStates.multi.bTrackerCall && (nUdpRes < extraDataLen + 8))
		h = WSAGetLastError ();
#endif
	if (bBroadcast <= 0) {
		if (gameStates.multi.bTrackerCall)
			return ((nUdpRes < 1) ? -1 : nUdpRes);
		return ((nUdpRes < extraDataLen + 8) ? -1 : nUdpRes - extraDataLen);
		}
	}
return dataLen;
}

//------------------------------------------------------------------------------

#if UDP_SAFEMODE

#define RESEND_ID			"D2XUDP:RESEND:"
#define RESEND_ID_LEN	(sizeof (RESEND_ID) - 1)

static void RequestResend (struct tDestListEntry *pdl, int nLastPacket)
{
	ubyte buf [40];
	int i;
	static int h = 0;

memcpy (buf, RESEND_ID, RESEND_ID_LEN);
*(reinterpret_cast<int*> (buf + RESEND_ID_LEN)) = INTEL_INT (pdl->nReceived);
*(reinterpret_cast<int*> (buf + RESEND_ID_LEN + 4)) = INTEL_INT (nLastPacket);
i = sendto (pdl->fd, buf, RESEND_ID_LEN + 8, 0, reinterpret_cast<struct sockaddr*> (&pdl->addr), sizeof (pdl->addr));
}

//------------------------------------------------------------------------------

#define FORGET_ID			"D2XUDP:FORGET:"
#define FORGET_ID_LEN	(sizeof (FORGET_ID) - 1)

int DropData (tDestListEntry *pdl, int nDrop)
{
	ubyte	buf [40];

memcpy (buf, FORGET_ID, FORGET_ID_LEN);
*(reinterpret_cast<int*> (buf + FORGET_ID_LEN)) = INTEL_INT (nDrop);
sendto (pdl->fd, buf, FORGET_ID_LEN + 4, 0, reinterpret_cast<struct sockaddr*> (&pdl->addr), sizeof (pdl->addr));
return 1;
}

//------------------------------------------------------------------------------

int ForgetData (struct sockaddr_in *fromAddr, ubyte *buf)
{
	int				i, nDrop;
	tDestListEntry	*pdl;

if (!extraGameInfo [0].bSafeUDP)
	return 0;
if (strncmp (buf, FORGET_ID, FORGET_ID_LEN))
	return 0;
if (destAddrNum <= (i = FindDestInList (fromAddr)))
	return 1;
pdl = destList + i;
nDrop = *(reinterpret_cast<int*> (buf + FORGET_ID_LEN));
pdl->nReceived = INTEL_INT (nDrop);
return 1;
}

//------------------------------------------------------------------------------

int ResendData (struct sockaddr_in *fromAddr, ubyte *buf)
{
	int				i, j, nFirst, nLast, nDrop;
	tDestListEntry	*pdl;
	tPacketProps	*ppp;
	time_t			t;

//if (!extraGameInfo [0].bSafeUDP)
//	return 0;
if (strncmp (buf, RESEND_ID, sizeof (RESEND_ID) - 1))
	return 0;
if (destAddrNum <= (i = FindDestInList (fromAddr)))
	return 1;
nFirst = *(reinterpret_cast<int*> (buf + RESEND_ID_LEN));
nFirst = INTEL_INT (nFirst);
nLast = *(reinterpret_cast<int*> (buf + RESEND_ID_LEN + 4));
nLast = INTEL_INT (nLast);
pdl = destList + i;
if (!pdl->numPackets)
	return 1;
ppp = pdl->packetProps + pdl->firstPacket;
if (nFirst < (nDrop = ppp->id)) {
	DropData (pdl, nDrop);
	if (nDrop > nLast)
		return 1;
	nFirst = nDrop;
	}
t = SDL_GetTicks ();
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
	sendto (pdl->fd, ppp->data, ppp->len, 0, reinterpret_cast<struct sockaddr*> (&pdl->addr), sizeof (pdl->addr));
	}
return 1;
}

#endif //UDP_SAFEMODE

//------------------------------------------------------------------------------
// Here we will receive a new packet and place it in the buffer passed.

static int UDPReceivePacket
	(ipx_socket_t *s, ubyte *outBuf, int outBufSize, struct ipx_recv_data *rd) 
{
	struct sockaddr_in	fromAddr;
	int						i, dataLen, bTracker, 
								fromAddrSize = sizeof (fromAddr);
	tDestListEntry			*pdl;
	ushort			srcPort;
#if UDP_SAFEMODE
	int						packetId = -1, bSafeMode = 0;
#endif
#if DBG
	//char						szIP [30];
#endif

if (0 > (dataLen = recvfrom (s->fd, reinterpret_cast<char*> (outBuf), outBufSize, 0, reinterpret_cast<struct sockaddr*> (&fromAddr), &fromAddrSize))) {
#if DBG
	int error = WSAGetLastError ();
#endif
	return -1;
	}
bTracker = IsTracker (*reinterpret_cast<ulong*> (&fromAddr.sin_addr), *reinterpret_cast<ushort*> (&fromAddr.sin_port));
if (fromAddr.sin_family != AF_INET) 
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
	rd->src_socket = ntohs (*reinterpret_cast<ushort*> (outBuf + 6));
	rd->dst_socket = s->socket;
	srcPort = ntohs (fromAddr.sin_port);
	// check if we already have sender of this packet in broadcast list
	memcpy (ipx_LocalAddress + 4, outBuf + dataLen - 6, 6);
	// add sender to client list if the packet is not from ourself
	if (!memcmp (&fromAddr.sin_addr, ipx_LocalAddress + 4, 4) &&
		 !memcmp (&srcPort, ipx_LocalAddress + 8, 2))
		return -1;
	i = AddDestToList (&fromAddr);
	if (i < 0)
		return -1;
	if (i < destAddrNum) {	//i.e. sender already in list or successfully added to list
		pdl = destList + i;
#if UDP_SAFEMODE
		bSafeMode = 0;
		pdl->fd = s->fd;
		pdl->bOurSafeMode = (memcmp (outBuf + dataLen - 10, "SAFE", 4) == 0);
		if (pdl->bOurSafeMode != extraGameInfo [0].bSafeUDP)
			ReportSafeMode (pdl);
		if (pdl->bOurSafeMode == 1) {
			bSafeMode = 1;
			packetId = *(reinterpret_cast<int*> (outBuf + dataLen - 14));
			packetId = INTEL_INT (packetId);
			if (packetId == pdl->nReceived) 
				pdl->nReceived++;
			else if (packetId > pdl->nReceived) {
				RequestResend (pdl, packetId);
				return -1;
				}
			}
#	if DBG
		console.printf (0, "%s: %d bytes, packet id: %d, safe modes: %d,%d", 
						iptos (szIP, reinterpret_cast<char*> (&fromAddr), dataLen, packetId, pdl->bSafeMode, pdl->bOurSafeMode);
#	endif
#endif //UDP_SAFEMODE
		}
	gameStates.multi.bHaveLocalAddress = 1;
	memcpy (netPlayers.players [gameData.multiplayer.nLocalPlayer].network.ipx.node, ipx_LocalAddress + 4, 6);
#if UDP_SAFEMODE
	dataLen -= (bSafeMode ? 22 : 14);
#else
	dataLen -= 14;
#endif
	memcpy (outBuf, outBuf + 8, dataLen);
	} //bTracker
#if DBG
else
	bTracker = bTracker;
#endif
memset (rd->src_network, 0, 4);
memcpy (rd->src_node, &fromAddr.sin_addr, 4);
memcpy (rd->src_node + 4, &fromAddr.sin_port, 2);
rd->pktType = 0;
#if 0//def UDPDEBUG
//printf(MSGHDR "ReceivePacket: dataLen=%d,from=",dataLen);
console.printf (0, "received %d bytes from %u.%u.%u.%u:%u\n", 
				dataLen, rd->src_node [0], rd->src_node [1], rd->src_node [2], rd->src_node [3],
				(signed short)ntohs (*reinterpret_cast<ushort*> (rd->src_node + 4)));
//dumpraddr (rd->src_node);
//putchar('\n');
#endif
return dataLen;
}

//------------------------------------------------------------------------------

int UDPPacketReady(ipx_socket_t *s) 
{
	ulong nAvailBytes = 0;

return !ioctlsocket(s->fd, FIONREAD, &nAvailBytes) && (nAvailBytes > 0);
}

//------------------------------------------------------------------------------

struct ipx_driver ipx_udp = {
	UDPGetMyAddress,
	UDPOpenSocket,
	UDPCloseSocket,
	UDPSendPacket,
	UDPReceivePacket,
	UDPPacketReady, //IxpGeneralPacketReady,
	NULL,	// InitNetgameAuxData
	NULL,	// HandleNetgameAuxData
	NULL,	// HandleLeaveGame
	NULL	// SendGamePacke
};
