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
//#	include <winsock.h>
#else
#	include <sys/socket.h>
#endif

#include "pstypes.h"

#ifdef _WIN32
#pragma pack (push, 1)
#endif

typedef struct IPXAddressStruct {
	u_char Network[4]; // __pack__;
	u_char Node[6]; // __pack__;
	u_char Socket[2]; // __pack__;
} IPXAddress_t;

typedef struct IPXPacketStructure {
	u_short Checksum; // __pack__;
	u_short Length; // __pack__;
	u_char TransportControl; // __pack__;
	u_char PacketType; // __pack__;
	IPXAddress_t Destination; // __pack__;
	IPXAddress_t Source; // __pack__;
} IPXPacket_t;

#ifdef _WIN32
#pragma pack (pop)
#endif

#ifndef _WIN32
#	define UINT_PTR	unsigned int
#	define INT_PTR		int
#endif

typedef struct ipx_socket_struct {
	u_short socket;
	UINT_PTR	fd;
} ipx_socket_t;

class CPacketOrigin : public CNetworkNode {
	/* all network order */
	private:
		//tNetworkNode	src_addr;
		int				packetType;

	public:
		u_short			src_socket;
		u_short			dst_socket;

	public:
		inline void SetType (int nType) { packetType = nType; }
#if 0 // inherited from CNetworkNode
		inline u_int32_t Server (void) { return src_addr.address.portAddress.ip.a; }
		inline u_int16_t Port (void) { return src_addr.address.portAddress.port.s; }
		inline u_char* Network (void) { return src_addr.network.b; }
		inline u_char* Node (void) { return src_addr.address.node; }
		inline int Type (void) { return packetType; }

		inline void SetServer (void* server) { memcpy (src_addr.address.portAddress.ip.octets, (ubyte*) server, sizeof (src_addr.address.portAddress.ip.octets)); }
		inline void SetNetwork (void* network) { memcpy (src_addr.network.b, (ubyte*) network, sizeof (src_addr.network)); }
		inline void SetNode (void* node) { memcpy (src_addr.address.node, (ubyte*) node, sizeof (src_addr.address.node)); }
		inline void SetPort (void* port) { memcpy (src_addr.address.portAddress.port.b, (ubyte*) port, sizeof (src_addr.address.portAddress.port)); }

		inline void ResetServer (ubyte filler = 0) { memset (src_addr.address.portAddress.ip.octets, filler, sizeof (src_addr.address.portAddress.ip.octets)); }
		inline void ResetNetwork (ubyte filler = 0) { memset (src_addr.network.b, filler, sizeof (src_addr.network)); }
		inline void ResetNode (ubyte filler = 0) { memset (src_addr.address.node, filler, sizeof (src_addr.address.node)); }
		inline void ResetPort (ubyte filler = 0) { memset (src_addr.address.portAddress.port.b, filler, sizeof (src_addr.address.portAddress.port)); }
#endif
		inline tNetworkNode& Address (void) { return m_address; }

		inline bool operator== (CPacketOrigin& other) { return !memcmp (&Address (), &other.Address (), sizeof (tNetworkNode)); }
		inline CPacketOrigin& operator= (CPacketOrigin& other) { 
			memcpy (this, &other, sizeof (*this));
			return *this;
			}
};

struct ipx_driver {
	int (*GetMyAddress)();
	int (*OpenSocket)(ipx_socket_t *, int);
	void (*CloseSocket)(ipx_socket_t *);
	int (*SendPacket)(ipx_socket_t *, IPXPacket_t *, u_char *, int);
	int (*ReceivePacket)(ipx_socket_t *, ubyte *, int, CPacketOrigin *);
	int (*PacketReady)(ipx_socket_t *s);
	void (*InitNetGameAuxData)(ipx_socket_t *, u_char []);
	int (*HandleNetGameAuxData)(ipx_socket_t *, const u_char []);
	void (*HandleLeaveGame)(ipx_socket_t *);
	int (*SendGamePacket)(ipx_socket_t *s, u_char *, int);
};

int IPXGeneralPacketReady (ipx_socket_t *s);

extern ubyte ipx_MyAddress[10];

#endif /* _IPX_DRV_H */
