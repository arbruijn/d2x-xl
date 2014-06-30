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

typedef struct tIPv4Address {
	u_int32_t					server;
	u_int16_t					port;
} tNetworkAddress;

typedef union tNetworkAddress {
	tIPv4Address		address;
	u_char				node [6];
} tNetworkAddress;


typedef union tPort {
	ubyte			b [2];
	u_int16_t	s;
} __pack__ tPort;

typedef union tIP {
	u_int32_t	a;
	ubyte			octets [4];
} __pack__ tIP;

typedef struct tPortAddress {
	tIP		ip;
	tPort		port;
} __pack__ tPortAddress;

typedef union tNetworkAddr {
	tPortAddress	portAddress;
	ubyte				node [6];
} tNetworkAddr;

typedef struct tNetworkNode {
	ubyte				network [4];
	tNetworkAddr	address;
} __pack__ tNetworkNode;

typedef struct tAppleTalkAddr {
	ushort  net;
	ubyte   node;
	ubyte   socket;
} __pack__ tAppleTalkAddr;

typedef union {
	public:
		tNetworkNode	node;
		tAppleTalkAddr	appletalk;
} __pack__ tNetworkInfo;

class CNetworkInfo {
	public:
		tNetworkInfo	m_info;

	public:
		inline ubyte* Network (void) { return m_info.node.network; }
		inline ubyte* Node (void) { return m_info.node.address.node; }
		inline ubyte* IP (void) { return m_info.node.address.portAddress.ip.octets; }
		inline ushort* Port (void) { return &m_info.node.address.portAddress.port.s; }
		inline tAppleTalkAddr& AppleTalk (void) { return m_info.appletalk; }
		inline tNetworkInfo& operator= (tNetworkInfo& other) {
			m_info = other;
			return m_info;
			}
	};



typedef struct IPXRecvData {
	/* all network order */
	private:
		tNetworkNode	src_addr;
		int				packetType;

	public:
		u_short			src_socket;
		u_short			dst_socket;

	public:
		inline u_int32_t Server (void) { return src_addr.address.portAddress.ip.a; }
		inline u_int16_t Port (void) { return src_addr.address.portAddress.port.s; }
		inline u_char* Network (void) { return src_addr.network; }
		inline u_char* Node (void) { return src_addr.address.node; }
		inline int Type (void) { return packetType; }

		inline void SetServer (void* server) { memcpy (src_addr.address.portAddress.ip.octets, (byte*) server, sizeof (src_addr.address.portAddress.ip.octets)); }
		inline void SetNetwork (void* network) { memcpy (src_addr.network, (byte*) network, sizeof (src_addr.network)); }
		inline void SetNode (void* node) { memcpy (src_addr.address.node, (byte*) node, sizeof (src_addr.address.node)); }
		inline void SetPort (void* port) { memcpy (src_addr.address.portAddress.port.b, (byte*) port, sizeof (src_addr.address.portAddress.port)); }
		inline void SetType (int nType) { packetType = nType; }

		inline void ResetServer (byte filler = 0) { memset (src_addr.address.portAddress.ip.octets, filler, sizeof (src_addr.address.portAddress.ip.octets)); }
		inline void ResetNetwork (byte filler = 0) { memset (src_addr.network, filler, sizeof (src_addr.network)); }
		inline void ResetNode (byte filler = 0) { memset (src_addr.address.node, filler, sizeof (src_addr.address.node)); }
		inline void ResetPort (byte filler = 0) { memset (src_addr.address.portAddress.port.b, filler, sizeof (src_addr.address.portAddress.port)); }
} IPXRecvData_t;

struct ipx_driver {
	int (*GetMyAddress)();
	int (*OpenSocket)(ipx_socket_t *, int);
	void (*CloseSocket)(ipx_socket_t *);
	int (*SendPacket)(ipx_socket_t *, IPXPacket_t *, u_char *, int);
	int (*ReceivePacket)(ipx_socket_t *, ubyte *, int, IPXRecvData_t *);
	int (*PacketReady)(ipx_socket_t *s);
	void (*InitNetGameAuxData)(ipx_socket_t *, u_char []);
	int (*HandleNetGameAuxData)(ipx_socket_t *, const u_char []);
	void (*HandleLeaveGame)(ipx_socket_t *);
	int (*SendGamePacket)(ipx_socket_t *s, u_char *, int);
};

int IPXGeneralPacketReady (ipx_socket_t *s);

extern ubyte ipx_MyAddress[10];

#endif /* _IPX_DRV_H */
