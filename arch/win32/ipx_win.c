/* $Id: ipx_win.c,v 1.8 2003/11/26 12:26:25 btb Exp $ */

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
#include <wsipx.h>
#include <errno.h>
#include <stdio.h>

#include "ipx_drv.h"
#include "mono.h"

extern int _CDECL_ Fail (const char *fmt, ...);
extern unsigned char *queryhost(char *buf);

#define FAIL	return Fail

//------------------------------------------------------------------------------

static int ipx_win_GetMyAddress( void )
{
#if 1
#	if 0
	char buf[256];
	unsigned char *qhbuf;

if (gethostname(buf,sizeof(buf))) 
	FAIL("Error getting my hostname");
if (!(qhbuf = queryhost(buf))) 
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
    con_printf (1,"IPX: could not open socket in GetMyAddress\n");
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
  
  if(bind(sock,(struct sockaddr *)&ipxs,sizeof(ipxs))==-1)
  {
#if TRACE
    con_printf (1,"IPX: could bind to network 0 in GetMyAddress\n");
#endif
    closesocket( sock );
    return(-1);
  }
  
  len = sizeof(ipxs2);
  if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0) {
#if TRACE
    con_printf (1,"IPX: could not get socket name in GetMyAddress\n");
#endif
    closesocket( sock );
    return(-1);
  }

  memcpy(ipx_MyAddress, ipxs2.sa_netnum, 4);
  for (i = 0; i < 6; i++) {
    ipx_MyAddress[4+i] = ipxs2.sa_nodenum[i];
  }
/*printf("My address is 0x%.4X:%.2X.%.2X.%.2X.%.2X.%.2X.%.2X\n", *((int *)ipxs2.sa_netnum), ipxs2.sa_nodenum[0],
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
    con_printf (1,"IPX: using socket %x\n", nextDynamicSocket);
#endif
    port = nextDynamicSocket++;
  }
*/
  /* do a socket call, then bind to this port */
//  sock = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
//  sock = socket(AF_IPX, SOCK_DGRAM, 0);
  sock = (int) socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);//why NSPROTO_IPX?  I looked in the quake source and thats what they used. :) -MPM  (on w2k 0 and PF_IPX don't work)
  if (sock == -1) {
#if TRACE
    con_printf (1,"IPX: could not open IPX socket.\n");
#endif
    return -1;
  }


  opt = 1;
  /* Permit broadcast output */
  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
		 (const char *)&opt, sizeof(opt)) == -1) {
#if TRACE
    con_printf (1,"IPX: could not set socket option for broadcast.\n");
#endif
    return -1;
  }
  ipxs.sa_family = AF_IPX;
  memcpy(ipxs.sa_netnum, ipx_MyAddress, 4);
//  *((unsigned int *)&ipxs.sa_netnum[0]) = *((unsigned int *)&ipx_MyAddress[0]);
/*  ipxs.sa_netnum = htonl(MyNetwork); */
  memset(ipxs.sa_nodenum, 0, 6);
//  bzero(ipxs.sa_nodenum, 6);	/* Please fill in my node name */
  ipxs.sa_socket = htons((short)port);

  /* now bind to this port */
  if (bind(sock, (struct sockaddr *) &ipxs, sizeof(ipxs)) == -1) {
#if TRACE
    con_printf (1,"IPX: could not bind socket to address\n");
#endif
    closesocket( sock );
    return -1;
  }
  
//  if( port==0 ) {
//    len = sizeof(ipxs2);
//    if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0) {
//     closesocket( sock );
//      return -1;
//    } else {
//      port = htons(ipxs2.sa_socket);
//    }
  len = sizeof(ipxs2);
  if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0) {
#if TRACE
    con_printf (1,"IPX: could not get socket name in IPXOpenSocket\n");
#endif
    closesocket( sock );
    return -1;
  } 
  if (port == 0) {
    port = htons(ipxs2.sa_socket);
#if TRACE
    con_printf (1,"IPX: opened dynamic socket %04x\n", port);
#endif
  }

  memcpy(ipx_MyAddress, ipxs2.sa_netnum, 4);
  memcpy(ipx_MyAddress + 4, ipxs2.sa_nodenum, 6);

  sk->fd = (int) sock;
  sk->socket = port;
  return 0;
}

//------------------------------------------------------------------------------

static void ipx_win_CloseSocket(ipx_socket_t *mysock) {
  /* now close the file descriptor for the socket, and D2_FREE it */
#if TRACE
  con_printf (1,"IPX: closing file descriptor on socket %x\n", mysock->socket);
#endif
  closesocket(mysock->fd);
}

//------------------------------------------------------------------------------

static int ipx_win_SendPacket(ipx_socket_t *mysock, IPXPacket_t *IPXHeader,
 u_char *data, int dataLen) {
  struct sockaddr_ipx ipxs;
 
  ipxs.sa_family = AF_IPX;
  /* get destination address from IPX packet header */
  memcpy(&ipxs.sa_netnum, IPXHeader->Destination.Network, 4);
  /* if destination address is 0, then send to my net */
  if ((*(unsigned int *)&ipxs.sa_netnum) == 0) {
    (*(unsigned int *)&ipxs.sa_netnum)= *((unsigned int *)&ipx_MyAddress[0]);
/*  ipxs.sa_netnum = htonl(MyNetwork); */
  }
  memcpy(&ipxs.sa_nodenum, IPXHeader->Destination.Node, 6);
  memcpy(&ipxs.sa_socket, IPXHeader->Destination.Socket, 2);
//  ipxs.saType = IPXHeader->PacketType;
  /*	ipxs.sipx_port=htons(0x452); */
  return sendto(mysock->fd, data, dataLen, 0,
	     (struct sockaddr *) &ipxs, sizeof(ipxs));
}

//------------------------------------------------------------------------------

static int ipx_win_ReceivePacket(ipx_socket_t *s, ubyte *buffer, int bufsize, 
 struct ipx_recv_data *rd) {
	int sz, size;
	struct sockaddr_ipx ipxs;
 
	sz = sizeof(ipxs);
	if ((size = recvfrom(s->fd, buffer, bufsize, 0, (struct sockaddr *) &ipxs, &sz)) <= 0)
	   return size;
   memcpy(rd->src_network, ipxs.sa_netnum, 4);
	memcpy(rd->src_node, ipxs.sa_nodenum, 6);
	rd->src_socket = ipxs.sa_socket;
	rd->dst_socket = s->socket;
//	rd->pktType = ipxs.sipxType;
  
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
