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
extern void ArchIpxSetDriver(int ipx_driver);

#define IPX_INIT_OK              0
#define IPX_SOCKET_ALREADY_OPEN -1
#define IPX_SOCKET_TABLE_FULL   -2
#define IPX_NOT_INSTALLED       -3
#define IPX_NO_LOW_DOS_MEM      -4 // couldn't allocate low dos memory
#define IPX_ERROR_GETTING_ADDR  -5 // error with getting internetwork address

//------------------------------------------------------------------------------

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

		inline void SetNetwork (void* network) { memcpy (m_info.node.network, (byte*) network, sizeof (m_info.node.network)); }
		inline void SetNode (void* node) { memcpy (m_info.node.address.node, (byte*) node, sizeof (m_info.node.address.node)); }
		inline void SetIP (void* ip) { memcpy (m_info.node.address.portAddress.ip.octets, (byte*) ip, sizeof (m_info.node.address.portAddress.ip.octets)); }
		inline void SetIP (u_int32_t ip) { m_info.node.address.portAddress.ip.a = ip; }
		inline void SetPort (void* port) { memcpy (m_info.node.address.portAddress.port.b, (byte*) port, sizeof (m_info.node.address.portAddress.port.b)); }
		inline void SetPort (u_int16_t port) { m_info.node.address.portAddress.port.s = port; }

		inline tAppleTalkAddr& AppleTalk (void) { return m_info.appletalk; }
		inline tNetworkInfo& operator= (tNetworkInfo& other) {
			m_info = other;
			return m_info;
			}
	};


//------------------------------------------------------------------------------

/* returns one of the above constants */
extern int IpxInit(int socket_number);

void _CDECL_ IpxClose(void);

int IpxChangeDefaultSocket (ushort nSocket, int bKeepClients = 0);

// Returns a pointer to 6-byte address
ubyte * IpxGetMyLocalAddress();
// Returns a pointer to 4-byte server
ubyte * IpxGetMyServerAddress();

// Determines the local address equivalent of an internetwork address.
void IpxGetLocalTarget( ubyte * server, ubyte * node, ubyte * local_target );

// If any packets waiting to be read in, this fills data in with the packet data and returns
// the number of bytes read.  Else returns 0 if no packets waiting.
int IpxGetPacketData( ubyte * data );

// Sends a broadcast packet to everyone on this socket.
void IPXSendBroadcastData( ubyte * data, int datasize );

// Sends a packet to a certain address
void IPXSendPacketData( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address );
void IPXSendInternetPacketData( ubyte * data, int datasize, ubyte * server, ubyte *address );

// Sends a packet to everyone in the game
int IpxSendGamePacket(ubyte *data, int datasize);

// Initialize and handle the protocol-specific field of the netgame struct.
void IpxInitNetGameAuxData(ubyte data[]);
int IpxHandleNetGameAuxData(const ubyte data[]);
// Handle disconnecting from the game
void IpxHandleLeaveGame();

void IpxReadUserFile (const char * filename);
void IpxReadNetworkFile (const char * filename);

//------------------------------------------------------------------------------

#endif
