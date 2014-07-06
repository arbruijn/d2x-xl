/* $Id: ipx_drv.h,v 1.5 2003/10/12 09:17:47 btb Exp $ */
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
#include <sys/types.h>

#define IPX_MANUAL_ADDRESS

typedef struct IPXAddressStruct {
	uint8_t Network[4]; // __attribute__((packed));
	uint8_t Node[6]; // __attribute__((packed));
	uint8_t Socket[2]; // __attribute__((packed));
} IPXAddress_t;

typedef struct IPXPacketStructure {
	uint16_t Checksum; // __attribute__((packed));
	uint16_t Length; // __attribute__((packed));
	uint8_t TransportControl; // __attribute__((packed));
	uint8_t PacketType; // __attribute__((packed));
	IPXAddress_t Destination; // __attribute__((packed));
	IPXAddress_t Source; // __attribute__((packed));
} IPXPacket_t;

typedef struct ipx_socket_struct {
#ifdef DOSEMU
	struct ipx_socket_struct *next;
	far_t listenList;
	int listenCount;
	far_t AESList;
	int AESCount;
	uint16_t PSP;
#endif
	uint16_t socket;
	int16_t fd;
} ipx_socket_t;

class CPacketAddress : public CNetworkNode {
	/* all network order */
	private:
		//tNetworkNode	src_addr;
		int32_t				packetType;

	public:
		uint16_t			src_socket;
		uint16_t			dst_socket;

	public:
		inline void SetType (int32_t nType) { packetType = nType; }
#if 0 // inherited from CNetworkNode
		inline uint32_t Server (void) { return src_addr.address.portAddress.ip.a; }
		inline uint16_t Port (void) { return src_addr.address.portAddress.port.s; }
		inline uint8_t* Network (void) { return src_addr.network.b; }
		inline uint8_t* Node (void) { return src_addr.address.node; }
		inline int32_t Type (void) { return packetType; }

		inline void SetServer (void* server) { memcpy (src_addr.address.portAddress.ip.octets, (uint8_t*) server, sizeof (src_addr.address.portAddress.ip.octets)); }
		inline void SetNetwork (void* network) { memcpy (src_addr.network.b, (uint8_t*) network, sizeof (src_addr.network)); }
		inline void SetNode (void* node) { memcpy (src_addr.address.node, (uint8_t*) node, sizeof (src_addr.address.node)); }
		inline void SetPort (void* port) { memcpy (src_addr.address.portAddress.port.b, (uint8_t*) port, sizeof (src_addr.address.portAddress.port)); }

		inline void ResetServer (uint8_t filler = 0) { memset (src_addr.address.portAddress.ip.octets, filler, sizeof (src_addr.address.portAddress.ip.octets)); }
		inline void ResetNetwork (uint8_t filler = 0) { memset (src_addr.network.b, filler, sizeof (src_addr.network)); }
		inline void ResetNode (uint8_t filler = 0) { memset (src_addr.address.node, filler, sizeof (src_addr.address.node)); }
		inline void ResetPort (uint8_t filler = 0) { memset (src_addr.address.portAddress.port.b, filler, sizeof (src_addr.address.portAddress.port)); }
#endif
		inline tNetworkNode& Address (void) { return m_address; }

		inline bool operator== (CPacketAddress& other) { return !memcmp (&Address (), &other.Address (), sizeof (tNetworkNode)); }
		inline CPacketAddress& operator= (CPacketAddress& other) { 
			memcpy (this, &other, sizeof (*this));
			return *this;
			}
};

struct ipx_driver {
	int (*GetMyAddress)(void);
	int (*OpenSocket)(ipx_socket_t *sk, int port);
	void (*CloseSocket)(ipx_socket_t *mysock);
	int (*SendPacket)(ipx_socket_t *mysock, IPXPacket_t *IPXHeader, uint8_t *data, int dataLen);
	int (*ReceivePacket)(ipx_socket_t *s, char *buffer, int bufsize, IPXRecvData_t *rec);
	int (*PacketReady)(ipx_socket_t *s);
	void (*InitNetgameAuxData)(ipx_socket_t *s, uint8_t buf[]);
	int (*HandleNetgameAuxData)(ipx_socket_t *s, const uint8_t buf[]);
	void (*HandleLeaveGame)(ipx_socket_t *s);
	int (*SendGamePacket)(ipx_socket_t *s, uint8_t *data, int dataLen);
};

int IPXGeneralPacketReady(ipx_socket_t *s);

extern uint8_t ipx_MyAddress[10];

#endif /* _IPX_DRV_H */
