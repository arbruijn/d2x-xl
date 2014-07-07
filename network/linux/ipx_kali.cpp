/* $Id: ipx_kali.c,v 1.7 2003/12/08 22:55:27 btb Exp $ */
/*
 *
 * IPX driver for KaliNix interface
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h> /* for htons & co. */
#include "pstypes.h"
#include "ipx_drv.h"
#include "ukali.h"

extern CNetworkAddress ipx_MyAddress;

static int32_t open_sockets = 0;
static int32_t dynamic_socket = 0x401;
static int32_t last_socket = 0;

int32_t have_empty_address() {
	int32_t i;
	for (i = 0; i < 10 && !ipx_MyAddress[i]; i++) ;
	return i == 10;
}

int32_t ipx_kali_GetMyAddress(void)
{

	kaliaddr_ipx mKaliAddr;

	if (!have_empty_address())
		return 0;

	if (KaliGetNodeNum(&mKaliAddr) < 0)
		return -1;

	ipx_MyAddress.SetServer ((uint32_t) 0);
	ipx_MyAddress.SetNode ((uint8_t*) mKaliAddr.sa_nodenum);

	return 0;
}

int32_t ipx_kali_OpenSocket(ipx_socket_t *sk, int32_t port)
{
	//printf("IPX_kali: OpenSocket on port(%d)\n", port);

	if (!open_sockets) {
		if (have_empty_address()) {
			if (ipx_kali_GetMyAddress() < 0) {
				//printf("IPX_kali: Error communicating with KaliNix\n");
				return -1;
			}
		}
	}
	if (!port)
		port = dynamic_socket++;

	if ((sk->fd = KaliOpenSocket(htons(port))) < 0) {
		//printf("IPX_kali: OpenSocket Failed on port(%d)\n", port);
		sk->fd = -1;
		return -1;
	}
	open_sockets++;
	last_socket = port;
	sk->socket = port;
	return 0;
}

void ipx_kali_CloseSocket(ipx_socket_t *mysock)
{
	if (!open_sockets) {
		//printf("IPX_kali: close w/o open\n");
		return;
	}
	//printf("IPX_kali: CloseSocket on port(%d)\n", mysock->socket);
	KaliCloseSocket(mysock->fd);
	if (--open_sockets) {
		//printf("IPX_kali: (closesocket) %d sockets left\n", open_sockets);
		return;
	}
}

int32_t ipx_kali_SendPacket(ipx_socket_t *mysock, IPXPacket_t *IPXHeader, uint8_t *data, int32_t dataLen)
{
	kaliaddr_ipx toaddr;
	int32_t i;

	memcpy(toaddr.sa_nodenum, IPXHeader->Destination.Node, sizeof(toaddr.sa_nodenum));
	memcpy(&toaddr.sa_socket, IPXHeader->Destination.Socket, sizeof(toaddr.sa_socket));

	if ((i = KaliSendPacket(mysock->fd, reinterpret_cast<char*> (data), dataLen, &toaddr)) < 0)
		return -1;

	return i;
}

int32_t ipx_kali_ReceivePacket(ipx_socket_t *s, uint8_t *outbuf, int32_t outbufsize, CPacketSource *rd)
{
	int32_t size;
	kaliaddr_ipx fromaddr;

	if ((size = KaliReceivePacket(s->fd, outbuf, outbufsize, &fromaddr)) < 0)
		return -1;

	rd->SetSockets (ntohs(fromaddr.sa_socket), s->socket);
	rd->SetNode ((uint8_t) fromaddr.sa_nodenum);
	rd->SetNetwork ((uint32_t) 0);
	rd->SetType (0);

	return size;
}

struct ipx_driver ipx_kali = {
	ipx_kali_GetMyAddress,
	ipx_kali_OpenSocket,
	ipx_kali_CloseSocket,
	ipx_kali_SendPacket,
	ipx_kali_ReceivePacket,
	IPXGeneralPacketReady,
	NULL,	// InitNetgameAuxData
	NULL,	// HandleNetgameAuxData
	NULL,	// HandleLeaveGame
	NULL	// SendGamePacket
};
