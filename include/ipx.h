/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _IPX_H
#define _IPX_H

#include "pstypes.h"

// The default socket to use.
#define IPX_DEFAULT_SOCKET 0x5130

#define IPX_PACKET_SIZE		576
#define IPX_PAYLOAD_SIZE	542
#define PPPoE_MTU				1492	// typical PPPoE based DSL MTU (MTUs can differ depending on protocols used for DSL connection)
#define UDP_PACKET_SIZE		PPPoE_MTU
#define UDP_HEADER_SIZE		18		//4 bytes for general networking, 14 bytes for udp stuff
#define UDP_PAYLOAD_SIZE	(UDP_PACKET_SIZE - UDP_HEADER_SIZE)		

#define MAX_PACKET_SIZE		UDP_PACKET_SIZE

#define IPX_DRIVER_IPX  1 // IPX "IPX driver" :-)
#define IPX_DRIVER_KALI 2
#define IPX_DRIVER_UDP  3 // UDP/IP, user datagrams protocol over the internet
#define IPX_DRIVER_MCAST4 4 // UDP/IP, user datagrams protocol over multicast networks

/* Sets the "IPX driver" (net driver).  Takes one of the above consts as argument. */
extern void ArchIpxSetDriver(int32_t ipx_driver);

#define IPX_INIT_OK              0
#define IPX_SOCKET_ALREADY_OPEN -1
#define IPX_SOCKET_TABLE_FULL   -2
#define IPX_NOT_INSTALLED       -3
#define IPX_NO_LOW_DOS_MEM      -4 // couldn't allocate low dos memory
#define IPX_ERROR_GETTING_ADDR  -5 // error with getting internetwork address

//------------------------------------------------------------------------------

typedef union tPort {
	uint8_t		b [2];
	uint16_t	s;
} __pack__ tPort;

typedef union tIP {
	uint32_t		a;
	uint8_t		octets [4];
} __pack__ tIP;

typedef struct tPortAddress {
	tIP		ip;
	tPort		port;
} __pack__ tPortAddress;

typedef union tNetworkAddr {
	tPortAddress	portAddress;
	uint8_t				node [6];
} tNetworkAddr;

typedef struct tNetworkNode {
	union {
		uint8_t		b [4];
		uint32_t		n;
	} network;
	tNetworkAddr	address;
} __pack__ tNetworkNode;

typedef struct tAppleTalkAddr {
	uint16_t  net;
	uint8_t   node;
	uint8_t   socket;
} __pack__ tAppleTalkAddr;

typedef union {
	public:
		tNetworkNode	node;
		tAppleTalkAddr	appletalk;
} __pack__ tNetworkInfo;

class CNetworkNode {
	public:
		tNetworkNode	m_address;

		inline uint8_t* Network (void) { return m_address.network.b; }
		inline uint8_t* Node (void) { return m_address.address.node; }
		inline uint8_t* Server (void) { return m_address.address.portAddress.ip.octets; }
		inline uint16_t Port (void) { return m_address.address.portAddress.port.s; }

		inline void SetNetwork (void* network) { memcpy (m_address.network.b, (uint8_t*) network, sizeof (m_address.network)); }
		inline void SetNode (void* node) { memcpy (m_address.address.node, (uint8_t*) node, sizeof (m_address.address.node)); }
		inline void SetServer (void* ip) { memcpy (m_address.address.portAddress.ip.octets, (uint8_t*) ip, sizeof (m_address.address.portAddress.ip.octets)); }
		inline void SetServer (uint32_t ip) { m_address.address.portAddress.ip.a = ip; }
		inline void SetPort (void* port) { memcpy (m_address.address.portAddress.port.b, (uint8_t*) port, sizeof (m_address.address.portAddress.port.b)); }
		inline void SetPort (uint16_t port) { m_address.address.portAddress.port.s = port; }

		inline void ResetServer (uint8_t filler = 0) { memset (m_address.address.portAddress.ip.octets, filler, sizeof (m_address.address.portAddress.ip.octets)); }
		inline void ResetNetwork (uint8_t filler = 0) { memset (m_address.network.b, filler, sizeof (m_address.network)); }
		inline void ResetNode (uint8_t filler = 0) { memset (m_address.address.node, filler, sizeof (m_address.address.node)); }
		inline void ResetPort (uint8_t filler = 0) { memset (m_address.address.portAddress.port.b, filler, sizeof (m_address.address.portAddress.port)); }
	};

class CNetworkInfo {
	public:
		tNetworkInfo	m_info;

	public:
		inline uint8_t* Network (void) { return m_info.node.network.b; }
		inline uint8_t* Node (void) { return m_info.node.address.node; }
		inline uint8_t* Server (void) { return m_info.node.address.portAddress.ip.octets; }
		inline uint16_t* Port (void) { return &m_info.node.address.portAddress.port.s; }

		inline void SetNetwork (void* network) { memcpy (m_info.node.network.b, (uint8_t*) network, sizeof (m_info.node.network)); }
		inline void SetNetwork (uint32_t network) {m_info.node.network.n = network; }
		inline void SetNode (void* node) { memcpy (m_info.node.address.node, (uint8_t*) node, sizeof (m_info.node.address.node)); }
		inline void SetServer (void* ip) { memcpy (m_info.node.address.portAddress.ip.octets, (uint8_t*) ip, sizeof (m_info.node.address.portAddress.ip.octets)); }
		inline void SetServer (uint32_t ip) { m_info.node.address.portAddress.ip.a = ip; }
		inline void SetPort (void* port) { memcpy (m_info.node.address.portAddress.port.b, (uint8_t*) port, sizeof (m_info.node.address.portAddress.port.b)); }
		inline void SetPort (uint16_t port) { m_info.node.address.portAddress.port.s = port; }

		inline void ResetNetwork (uint8_t filler = 0) { memset (m_info.node.network.b, filler, sizeof (m_info.node.network)); }
		inline void ResetNode (uint8_t filler = 0) { memset (m_info.node.address.node, filler, sizeof (m_info.node.address.node)); }

		inline bool HaveNetwork (void) { return m_info.node.network.n != 0; }

		inline bool operator== (CNetworkInfo& other) { return !memcmp (&m_info, &other.m_info, sizeof (m_info)); }

		inline tAppleTalkAddr& AppleTalk (void) { return m_info.appletalk; }
		inline tNetworkInfo& operator= (tNetworkInfo& other) {
			m_info = other;
			return m_info;
			}
	};


//------------------------------------------------------------------------------

/* returns one of the above constants */
extern int32_t IpxInit(int32_t socket_number);

void _CDECL_ IpxClose(void);

int32_t IpxChangeDefaultSocket (uint16_t nSocket, int32_t bKeepClients = 0);

// Returns a pointer to 6-uint8_t address
uint8_t * IpxGetMyLocalAddress (void);
// Returns a pointer to 4-uint8_t server
uint8_t * IpxGetMyServerAddress (void);

// Determines the local address equivalent of an internetwork address.
void IpxGetLocalTarget( uint8_t * server, uint8_t * node, uint8_t * local_target );

// If any packets waiting to be read in, this fills data in with the packet data and returns
// the number of bytes read.  Else returns 0 if no packets waiting.
int32_t IpxGetPacketData (uint8_t * data);

// Sends a broadcast packet to everyone on this socket.
void IPXSendBroadcastData (uint8_t * data, int32_t datasize);

// Sends a packet to a certain address
void IPXSendPacketData (uint8_t * data, int32_t datasize, uint8_t *network, uint8_t *address, uint8_t *immediate_address);
void IPXSendInternetPacketData (uint8_t * data, int32_t datasize, uint8_t * server, uint8_t *address);

// Sends a packet to everyone in the game
int32_t IpxSendGamePacket (uint8_t *data, int32_t datasize);

// Initialize and handle the protocol-specific field of the netgame struct.
void IpxInitNetGameAuxData (uint8_t data[]);
int32_t IpxHandleNetGameAuxData (const uint8_t data[]);
// Handle disconnecting from the game
void IpxHandleLeaveGame (void);

void IpxReadUserFile (const char * filename);
void IpxReadNetworkFile (const char * filename);

//------------------------------------------------------------------------------

#endif
