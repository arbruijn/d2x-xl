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

#include "ipx.h"
#include "../win32/include/ipx_drv.h"
#include "mono.h"
#include "error.h"

int32_t _CDECL_ Fail (const char *fmt, ...);
uint8_t *QueryHost(char *buf);

#define FAIL	return Fail

//------------------------------------------------------------------------------

static int32_t ipx_win_GetMyAddress( void )
{
#if 1
#	if 0
	char buf[256];
	uint8_t *qhbuf;

if (gethostname(buf,sizeof(buf))) 
	FAIL("Error getting my hostname");
if (!(qhbuf = QueryHost(buf))) 
	FAIL("Querying my own hostname \"%s\"",buf);
	ipx_MyAddress.SetServer (0);
	ipx_MyAddress.SetNode (qhbuf);
#	endif
#else
  int32_t sock;
  struct sockaddr_ipx ipxs;
  struct sockaddr_ipx ipxs2;
  int32_t len;
  int32_t i;
  
//  sock=socket(AF_IPX,SOCK_DGRAM,PF_IPX);
  sock=socket(AF_IPX,SOCK_DGRAM,0);

  if(sock==-1)
  {
#if TRACE
    PrintLog (0, "IPX: could not open socket in GetMyAddress\n");
#endif
    return(-1);
  }

  /* bind this socket to network 0 */  
  ipxs.sa_family=AF_IPX;
#ifdef IPX_MANUAL_ADDRESS
  memcpy (ipxs.sa_netnum, ipx_MyAddress.Network (), 4);
#else
  memset(ipxs.sa_netnum, 0, 4);
#endif  
  ipxs.sa_socket=0;
  
  if (bind (sock, reinterpret_cast<struct sockaddr*> (&ipxs), sizeof (ipxs)) == -1)
  {
#if TRACE
    PrintLog (0, "IPX: could bind to network 0 in GetMyAddress\n");
#endif
    closesocket( sock );
    return(-1);
  }
  
  len = sizeof(ipxs2);
  if (getsockname(sock, reinterpret_cast<struct sockaddr*> (&ipxs2), &len) < 0) {
#if TRACE
    PrintLog (0, "IPX: could not get socket name in GetMyAddress\n");
#endif
    closesocket( sock );
    return(-1);
  }

  ipx_MyAddress.SetNetwork (ipxs2.sa_netnum);
  ipx_MyAddress.SetNode (ipxs2.sa_nodenum);
  }
/*printf("My address is 0x%.4X:%.2X.%.2X.%.2X.%.2X.%.2X.%.2X\n", *reinterpret_cast<int32_t*> (ipxs2.sa_netnum), ipxs2.sa_nodenum[0],
  ipxs2.sa_nodenum[1], ipxs2.sa_nodenum[2], ipxs2.sa_nodenum[3], ipxs2.sa_nodenum[4], ipxs2.sa_nodenum[5]);
*/
    closesocket( sock );
#endif
  return(0);
}

//------------------------------------------------------------------------------

static int32_t ipx_win_OpenSocket(ipx_socket_t *sk, int32_t port)
{
  int32_t sock;			/* sock here means Linux socket handle */
  int32_t opt;
  struct sockaddr_ipx ipxs;
  int32_t len;
  struct sockaddr_ipx ipxs2;

  /* DANG_FIXTHIS - kludge to support broken linux IPX stack */
  /* need to convert dynamic socket open into a real socket number */
/*  if (port == 0) {
#if TRACE
    PrintLog (0, "IPX: using socket %x\n", nextDynamicSocket);
#endif
    port = nextDynamicSocket++;
  }
*/
  /* do a socket call, then bind to this port */
//  sock = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
//  sock = socket(AF_IPX, SOCK_DGRAM, 0);
sock = (int32_t) socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);//why NSPROTO_IPX?  I looked in the quake source and thats what they used. :) -MPM  (on w2k 0 and PF_IPX don't work)
if (sock == -1) {
	PrintLog (0, "IPX: could not open IPX socket.\n");
	return -1;
	}

opt = 1;
/* Permit broadcast output */
if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*> (&opt), sizeof(opt)) == -1) {
	PrintLog (0, "IPX: could not set socket option for broadcast.\n");
	return -1;
	}

ipxs.sa_family = AF_IPX;
memcpy (ipxs.sa_netnum, ipx_MyAddress.Network (), 4);
memset(ipxs.sa_nodenum, 0, 6);
ipxs.sa_socket = htons((int16_t)port);

/* now bind to this port */
if (bind (sock, reinterpret_cast<struct sockaddr*> (&ipxs), sizeof(ipxs)) == -1) {
	PrintLog (0, "IPX: could not bind socket to address\n");
	closesocket( sock );
	return -1;
	}

len = sizeof (ipxs2);
if (getsockname(sock, reinterpret_cast<struct sockaddr*> (&ipxs2), &len) < 0) {
	PrintLog (0, "IPX interface: could not get socket name in IPXOpenSocket\n");
	closesocket( sock );
	return -1;
	} 
if (port == 0) {
	port = htons (ipxs2.sa_socket);
	PrintLog (0, "IPX: opened dynamic socket %04x\n", port);
	}

ipx_MyAddress.SetNetwork (ipxs2.sa_netnum);
ipx_MyAddress.SetNode (ipxs2.sa_nodenum);
sk->fd = (int32_t) sock;
sk->socket = port;
return 0;
}

//------------------------------------------------------------------------------

static void ipx_win_CloseSocket(ipx_socket_t *mysock) {
  /* now close the file descriptor for the socket, and D2_FREE it */
#if TRACE
PrintLog (0, "IPX: closing file descriptor on socket %x\n", mysock->socket);
#endif
closesocket (mysock->fd);
}

//------------------------------------------------------------------------------

static int32_t ipx_win_SendPacket (ipx_socket_t *mysock, IPXPacket_t *IPXHeader, uint8_t *data, int32_t dataLen) 
{
  struct sockaddr_ipx ipxs;
 
ipxs.sa_family = AF_IPX;
/* get destination address from IPX packet header */
memcpy (&ipxs.sa_netnum, IPXHeader->Destination.Network, 4);
/* if destination address is 0, then send to my net */
if (*reinterpret_cast<uint32_t*> (&ipxs.sa_netnum) == 0) {
	memcpy (&ipxs.sa_netnum, ipx_MyAddress.Network (), 4);
}
memcpy (&ipxs.sa_nodenum, IPXHeader->Destination.Node, 6);
memcpy (&ipxs.sa_socket, IPXHeader->Destination.Socket, 2);
return sendto (mysock->fd, reinterpret_cast<const char*> (data), dataLen, 0,
				   reinterpret_cast<struct sockaddr*> (&ipxs), sizeof(ipxs));
}

//------------------------------------------------------------------------------

static int32_t ipx_win_ReceivePacket (ipx_socket_t *s, uint8_t *buffer, int32_t bufsize, CPacketAddress *rd) 
{
	struct sockaddr_ipx ipxs;
 
int32_t sz = sizeof(ipxs);
int32_t size = recvfrom (s->fd, reinterpret_cast<char*> (buffer), bufsize, 0,  reinterpret_cast<struct sockaddr*> (&ipxs), &sz);
if (size <= 0)
   return size;
rd->SetNetwork (ipxs.sa_netnum);
rd->SetNode (ipxs.sa_nodenum);
rd->SetSockets (ipxs.sa_socket, s->socket);
return size;
}

//------------------------------------------------------------------------------

struct ipx_driver ipx_win = {
	ipx_win_GetMyAddress,
	ipx_win_OpenSocket,
	ipx_win_CloseSocket,
	ipx_win_SendPacket,
	ipx_win_ReceivePacket,
	IPXGeneralPacketReady,
	NULL,	// InitNetGameAuxData
	NULL,	// HandleNetGameAuxData
	NULL,	// HandleLeaveGame
	NULL	// SendGamePack
};

//------------------------------------------------------------------------------
