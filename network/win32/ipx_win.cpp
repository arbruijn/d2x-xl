/*
 *
 * IPX driver using BSD style sockets
 * Mostly taken from dosemu
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <wsipx.h>
#include <errno.h>
#include <stdio.h>

#include "../win32/include/ipx_drv.h"
#include "mono.h"

extern int _CDECL_ Fail (const char *fmt, ...);
extern ubyte *QueryHost(char *buf);

#define FAIL	return Fail

//------------------------------------------------------------------------------

static int ipx_win_GetMyAddress( void )
{
#if 1
#	if 0
	char buf[256];
	ubyte *qhbuf;

if (gethostname(buf,sizeof(buf))) 
	FAIL("Error getting my hostname");
if (!(qhbuf = QueryHost(buf))) 
	FAIL("Querying my own hostname \"%s\"",buf);
	memset(ipx_MyAddress+0,0,4);
	memcpy(ipx_MyAddress+4,qhbuf,6);
#	endif
#else
  int sock;
  struct sockaddr_ipx ipxs;
  struct sockaddr_ipx ipxs2;
  int len;
  int i;
  
//  sock=socket(AF_IPX,SOCK_DGRAM,PF_IPX);
  sock=socket(AF_IPX,SOCK_DGRAM,0);

  if(sock==-1)
  {
#if TRACE
    PrintLog ("IPX: could not open socket in GetMyAddress\n");
#endif
    return(-1);
  }

  /* bind this socket to network 0 */  
  ipxs.sa_family=AF_IPX;
#ifdef IPX_MANUAL_ADDRESS
  memcpy(ipxs.sa_netnum, ipx_MyAddress, 4);
#else
  memset(ipxs.sa_netnum, 0, 4);
#endif  
  ipxs.sa_socket=0;
  
  if (bind (sock, reinterpret_cast<struct sockaddr*> (&ipxs), sizeof (ipxs)) == -1)
  {
#if TRACE
    PrintLog ("IPX: could bind to network 0 in GetMyAddress\n");
#endif
    closesocket( sock );
    return(-1);
  }
  
  len = sizeof(ipxs2);
  if (getsockname(sock, reinterpret_cast<struct sockaddr*> (&ipxs2), &len) < 0) {
#if TRACE
    PrintLog ("IPX: could not get socket name in GetMyAddress\n");
#endif
    closesocket( sock );
    return(-1);
  }

  memcpy(ipx_MyAddress, ipxs2.sa_netnum, 4);
  for (i = 0; i < 6; i++) {
    ipx_MyAddress[4+i] = ipxs2.sa_nodenum[i];
  }
/*printf("My address is 0x%.4X:%.2X.%.2X.%.2X.%.2X.%.2X.%.2X\n", *reinterpret_cast<int*> (ipxs2.sa_netnum), ipxs2.sa_nodenum[0],
  ipxs2.sa_nodenum[1], ipxs2.sa_nodenum[2], ipxs2.sa_nodenum[3], ipxs2.sa_nodenum[4], ipxs2.sa_nodenum[5]);
*/
    closesocket( sock );
#endif
  return(0);
}

//------------------------------------------------------------------------------

static int ipx_win_OpenSocket(ipx_socket_t *sk, int port)
{
  int sock;			/* sock here means Linux socket handle */
  int opt;
  struct sockaddr_ipx ipxs;
  int len;
  struct sockaddr_ipx ipxs2;

  /* DANG_FIXTHIS - kludge to support broken linux IPX stack */
  /* need to convert dynamic socket open into a real socket number */
/*  if (port == 0) {
#if TRACE
    PrintLog ("IPX: using socket %x\n", nextDynamicSocket);
#endif
    port = nextDynamicSocket++;
  }
*/
  /* do a socket call, then bind to this port */
//  sock = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
//  sock = socket(AF_IPX, SOCK_DGRAM, 0);
sock = (int) socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);//why NSPROTO_IPX?  I looked in the quake source and thats what they used. :) -MPM  (on w2k 0 and PF_IPX don't work)
if (sock == -1) {
	PrintLog ("IPX: could not open IPX socket.\n");
	return -1;
	}

opt = 1;
/* Permit broadcast output */
if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*> (&opt), sizeof(opt)) == -1) {
	PrintLog ("IPX: could not set socket option for broadcast.\n");
	return -1;
	}

ipxs.sa_family = AF_IPX;
memcpy(ipxs.sa_netnum, ipx_MyAddress, 4);
memset(ipxs.sa_nodenum, 0, 6);
ipxs.sa_socket = htons((short)port);

/* now bind to this port */
if (bind (sock, reinterpret_cast<struct sockaddr*> (&ipxs), sizeof(ipxs)) == -1) {
	PrintLog ("IPX: could not bind socket to address\n");
	closesocket( sock );
	return -1;
	}

len = sizeof (ipxs2);
if (getsockname(sock, reinterpret_cast<struct sockaddr*> (&ipxs2), &len) < 0) {
	PrintLog (1,"IPX: could not get socket name in IPXOpenSocket\n");
	closesocket( sock );
	return -1;
	} 
if (port == 0) {
	port = htons (ipxs2.sa_socket);
	PrintLog ("IPX: opened dynamic socket %04x\n", port);
	}

memcpy (ipx_MyAddress, ipxs2.sa_netnum, 4);
memcpy (ipx_MyAddress + 4, ipxs2.sa_nodenum, 6);
sk->fd = (int) sock;
sk->socket = port;
return 0;
}

//------------------------------------------------------------------------------

static void ipx_win_CloseSocket(ipx_socket_t *mysock) {
  /* now close the file descriptor for the socket, and D2_FREE it */
#if TRACE
PrintLog ("IPX: closing file descriptor on socket %x\n", mysock->socket);
#endif
closesocket (mysock->fd);
}

//------------------------------------------------------------------------------

static int ipx_win_SendPacket (ipx_socket_t *mysock, IPXPacket_t *IPXHeader, u_char *data, int dataLen) 
{
  struct sockaddr_ipx ipxs;
 
ipxs.sa_family = AF_IPX;
/* get destination address from IPX packet header */
memcpy(&ipxs.sa_netnum, IPXHeader->Destination.Network, 4);
/* if destination address is 0, then send to my net */
if (*reinterpret_cast<uint*> (&ipxs.sa_netnum) == 0) {
 *reinterpret_cast<uint*> (&ipxs.sa_netnum) = *reinterpret_cast<uint*> (&ipx_MyAddress[0]);
}
memcpy (&ipxs.sa_nodenum, IPXHeader->Destination.Node, 6);
memcpy (&ipxs.sa_socket, IPXHeader->Destination.Socket, 2);
return sendto (mysock->fd, reinterpret_cast<const char*> (data), dataLen, 0,
				   reinterpret_cast<struct sockaddr*> (&ipxs), sizeof(ipxs));
}

//------------------------------------------------------------------------------

static int ipx_win_ReceivePacket(ipx_socket_t *s, ubyte *buffer, int bufsize, struct ipx_recv_data *rd) 
{
	int sz, size;
	struct sockaddr_ipx ipxs;
 
int sz = sizeof(ipxs);
int size = recvfrom (s->fd, reinterpret_cast<char*> (buffer), bufsize, 0,  reinterpret_cast<struct sockaddr*> (&ipxs), &sz);
if (size <= 0)
   return size;
memcpy (rd->src_network, ipxs.sa_netnum, 4);
memcpy (rd->src_node, ipxs.sa_nodenum, 6);
rd->src_socket = ipxs.sa_socket;
rd->dst_socket = s->socket;
return size;
}

//------------------------------------------------------------------------------

struct ipx_driver ipx_win = {
	ipx_win_GetMyAddress,
	ipx_win_OpenSocket,
	ipx_win_CloseSocket,
	ipx_win_SendPacket,
	ipx_win_ReceivePacket,
	IxpGeneralPacketReady,
	NULL,	// InitNetgameAuxData
	NULL,	// HandleNetgameAuxData
	NULL,	// HandleLeaveGame
	NULL	// SendGamePack
};

//------------------------------------------------------------------------------
