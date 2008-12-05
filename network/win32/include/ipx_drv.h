/* $Id: ipx_drv.h,v 1.7 2003/10/12 09:17:47 btb Exp $ */

/*
 *
 * IPX driver interface
 *
 * parts from:
 * ipx[HA] header file for IPX for the DOS emulator
 * 		Tim Bird, tbird@novell.com
 *
 */

#ifndef _IPX_DRV_H
#define _IPX_DRV_H

#define IPX_MANUAL_ADDRESS

#ifdef _WIN32
#	include <winsock.h>
#else
#	include <sys/socket.h>
#endif

#include "pstypes.h"

#define MAX_PACKET_DATA 1500

#ifdef _WIN32
#pragma pack (push, 1)
#endif

typedef struct IPXAddressStruct {
	u_char Network[4] __pack__;
	u_char Node[6] __pack__;
	u_char Socket[2] __pack__;
} IPXAddress_t;

typedef struct IPXPacketStructure {
	u_short Checksum __pack__;
	u_short Length __pack__;
	u_char TransportControl __pack__;
	u_char PacketType __pack__;
	IPXAddress_t Destination __pack__;
	IPXAddress_t Source __pack__;
} IPXPacket_t;

#ifdef _WIN32
#pragma pack (pop)
#endif

typedef struct ipx_socket_struct {
	u_short socket;
	int fd;
} ipx_socket_t;

struct ipx_recv_data {
	/* all network order */
	u_char src_network[4];
	u_char src_node[6];
	u_short src_socket;
	u_short dst_socket;
	int pktType;
};

struct ipx_driver {
	int (*GetMyAddress)();
	int (*OpenSocket)(ipx_socket_t *, int);
	void (*CloseSocket)(ipx_socket_t *);
	int (*SendPacket)(ipx_socket_t *, IPXPacket_t *, u_char *, int);
	int (*ReceivePacket)(ipx_socket_t *, ubyte *, int, struct ipx_recv_data *);
	int (*PacketReady)(ipx_socket_t *s);
	void (*InitNetgameAuxData)(ipx_socket_t *, u_char []);
	int (*HandleNetgameAuxData)(ipx_socket_t *, const u_char []);
	void (*HandleLeaveGame)(ipx_socket_t *);
	int (*SendGamePacket)(ipx_socket_t *s, u_char *, int);
};

int IxpGeneralPacketReady(ipx_socket_t *s);

extern ubyte ipx_MyAddress[10];

#endif /* _IPX_DRV_H */
