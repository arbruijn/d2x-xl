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

/*
 *
 * Win32 lower-level network code.
 * implements functions declared in include/ipx.h
 *
 */

//------------------------------------------------------------------------------
/*
The general proceedings of D2X-XL when establishing a UDP/IP communication between two peers 
is as follows:

The sender places the destination address (IP + port) right after the game data in the data 
packet. This happens in UDPSendPacket in the following two lines:

	memcpy (buf + 8 + dataLen, &dest->sin_addr, 4);
	memcpy (buf + 12 + dataLen, &dest->sin_port, 2);

The receiver that way gets to know the IP + port it is sending on. These are not always to be 
determined by the sender itself, as it may sit behind a NAT or proxy, or be using port 0 
(in which case the OS will chose a port for it). The sender's IP + port are stored in the global 
variable networkData.packetSource (happens in ipx_udp.c::UDPReceivePacket()), which is needed on some special 
occasions.

That's my mechanism to make every participant in a game reliably know about its own IP + port.

All this starts with a client connecting to a server: Consider this the bootstrapping part of 
establishing the communication. This always works, as the client knows the servers' IP + port 
(if it doesn't, no connection ;). The server receives a game info request from the client 
(containing the server IP + port after the game data) and thus gets to know its IP + port. It 
replies to the client, and puts the client's IP + port after the end of the game data.

There is a message nType where the server tells all participants about all other participants; 
that's how clients find out about each other in a game.

When the server receives a participation request from a client, it adds its IP + port to a table 
containing all participants of the game. When the server subsequently sends data to the client, 
it uses that IP + port. 

This however takes only part after the client has sent a game info request and received a game 
info from the server. When the server sends that game info, it hasn't added that client to the 
participants table. Therefore, some game data contains client address data. Unfortunately, this 
address data is not valid in UDP/IP communications, and this is where we need networkData.packetSource from 
above: It's contents is patched into the game data address. This happens in 
main/network.c::NetworkProcessPacket()) and now is used by the function returning the game info.

the following is the address mangling code from network.c::NetworkProcessPacket(). All packet 
types that do not contain a network address are excluded by the lengthy if statement (I can 
only hope I got the right ones and didn't forget any, but sofar everything seems to work, at 
least on LE machines, so ...)

if	 ((gameStates.multi.nGameType == UDP_GAME) &&
		(pid != PID_LITE_INFO) &&
		(pid != PID_GAME_INFO) &&
		(pid != PID_EXTRA_GAMEINFO) &&
		(pid != PID_PLAYER_DATA) &&
		(pid != PID_MINE_DATA) &&
		(pid != PID_OBJECT_DATA) &&
		(pid != PID_ENDLEVEL) && 
		(pid != PID_ENDLEVEL_SHORT) &&
		(pid != PID_UPLOAD) &&
		(pid != PID_DOWNLOAD) &&
		(pid != PID_ADDPLAYER) &&
		(pid != PID_TRACKER_GET_SERVERLIST) &&
		(pid != PID_TRACKER_ADD_SERVER)
	)
 {
	memcpy (&their->player.network.Network (), &networkData.packetSource.src_network, 10);
	}
*/
//------------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winsock.h>

#include "descent.h"
#include "args.h"
#include "error.h"
#include "ipx.h"
#include "../win32/include/ipx_drv.h"
#include "../win32/include/ipx_udp.h"
#include "../win32/include/ipx_mcast4.h"
#include "network.h"
#include "network_lib.h"
#include "player.h"	/* for gameData.multiplayer.players */
#include "multi.h"	/* for netPlayers [0] */
#include "tracker.h"
#include "hudmsgs.h"

extern struct ipx_driver ipx_win;

int32_t				ipx_fd;
ipx_socket_t		ipxSocketData;
uint8_t				bIpxInstalled = 0;
tPort					ipx_socket = {0};
tIPAddress			ipx_network = {0};
CNetworkAddress	ipx_MyAddress;
int32_t				nIpxPacket = 0;			/* Sequence number */

/* User defined routing stuff */
typedef struct user_address {
	uint8_t network [4];
	uint8_t node [6];
	uint8_t address [6];
} __pack__ user_address;

#define MAX_USERS 64
int32_t nIpxUsers = 0;
user_address ipxUsers [MAX_USERS];

#define MAX_NETWORKS 64
int32_t nIpxNetworks = 0;
tIPAddress ipxNetworks [MAX_NETWORKS];

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class CNetworkLocks {
	public:
		SDL_mutex*	m_sendLock;
		SDL_mutex*	m_recvLock;

	public:
		CNetworkLocks () : m_sendLock (NULL), m_recvLock (NULL) {
			m_sendLock = SDL_CreateMutex ();
			m_recvLock = SDL_CreateMutex ();
			}

		~CNetworkLocks () {
			DestroyLock (m_sendLock);
			DestroyLock (m_recvLock);
			}

		inline void DestroyLock (SDL_mutex*& m) {
			if (m) {
				SDL_DestroyMutex (m);
				m = NULL;
				}	
			}
		inline void Lock (SDL_mutex* m) {
			if (m)
				SDL_LockMutex (m);
			}

		inline void Unlock (SDL_mutex* m) {
			if (m)
				SDL_UnlockMutex (m);
			}

		inline int32_t Abort (SDL_mutex* m) {
			Unlock (m);
			return -1;
			}

		inline void LockSend (void) { Lock (m_sendLock); }
		inline void UnlockSend (void) { Unlock (m_sendLock); }
		inline void LockRecv (void) { Lock (m_recvLock); }
		inline void UnlockRecv (void) { Unlock (m_recvLock); }
		inline int32_t AbortSend (void) { return Abort (m_sendLock); }
		inline int32_t AbortRecv (void) { return Abort (m_recvLock); }
};

static CNetworkLocks locks;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32_t IPXGeneralPacketReady (ipx_socket_t *s) 
{
	fd_set set;
	struct timeval tv = {0, 0};
	
FD_ZERO (&set);
FD_SET (s->fd, &set);
return (select (FD_SETSIZE, &set, NULL, NULL, &tv) > 0);
}

//------------------------------------------------------------------------------

struct ipx_driver *driver = &ipx_win;

uint8_t *IpxGetMyServerAddress ()
{
return reinterpret_cast<uint8_t*> (&ipx_network);
}

//------------------------------------------------------------------------------

uint8_t *IpxGetMyLocalAddress ()
{
return ipx_MyAddress.Node ();
}

//------------------------------------------------------------------------------

void ArchIpxSetDriver (int32_t ipx_driver)
{
switch (ipx_driver) {
	case IPX_DRIVER_IPX: 
		driver = &ipx_win; 
		break;
	case IPX_DRIVER_UDP: 
		driver = &ipx_udp; 
		break;
	case IPX_DRIVER_MCAST4: 
		driver = &ipx_mcast4; 
		break;
	default: 
		Int3 ();
	}
}

//------------------------------------------------------------------------------

int32_t IpxInit (int32_t nSocket)
{
	int32_t i;
	WSADATA wsaData;
   WORD wVersionRequested = MAKEWORD (2, 0);

if (WSAStartup (wVersionRequested, &wsaData))
	return IPX_SOCKET_ALREADY_OPEN;

if ((i = FindArg ("-ipxnetwork")) && appConfig [i + 1]) {
	int32_t n = strtol (appConfig [i + 1], NULL, 16);
	uint8_t b [4]; 
	for (i = 3; i >= 0; i--, n >>= 8)
		b [i] = n &0xFF;
	ipx_MyAddress.SetServer (b);
	}
if ((nSocket >= 0) && driver->OpenSocket (&ipxSocketData, nSocket))
	return IPX_NOT_INSTALLED;
driver->GetMyAddress ();
ipx_network.a = ipx_MyAddress.network.a;
nIpxNetworks = 0;
ipxNetworks [nIpxNetworks++].a = ipx_network.a;
bIpxInstalled = 1;
atexit (IpxClose);
return IPX_INIT_OK;
}

//------------------------------------------------------------------------------

void _CDECL_ IpxClose (void)
{
if (bIpxInstalled) {
	PrintLog (1, "closing IPX socket\n");
   WSACleanup ();
	driver->CloseSocket (&ipxSocketData);
	PrintLog (-1);
   }
bIpxInstalled = 0;
}

//------------------------------------------------------------------------------

int32_t IpxGetPacketData (uint8_t *data)
{
	static uint8_t	buf [MAX_PACKET_SIZE];

	int32_t dataSize, dataOffs;

locks.LockRecv ();
while (driver->PacketReady (&ipxSocketData)) {
	dataSize = driver->ReceivePacket (reinterpret_cast<ipx_socket_t*> (&ipxSocketData), buf, sizeof (buf), &networkData.packetSource);
#if 0//DBG
	HUDMessage (0, "received %d bytes from %d.%d.%d.%d:%u", 
					size,
					networkData.packetSource.src_node [0],
					networkData.packetSource.src_node [1],
					networkData.packetSource.src_node [2],
					networkData.packetSource.src_node [3],
					*(reinterpret_cast<uint16_t*> (networkData.packetSource.src_node + 4)));
#endif
	if (dataSize < 0)
		break;
	if (dataSize < 6)
		continue;
	dataOffs = tracker.IsTracker (networkData.packetSource.GetServer (), networkData.packetSource.GetPort (), (char*) buf) ? 0 : 4;
	if (dataSize > int32_t (MAX_PAYLOAD_SIZE + dataOffs)) {
		PrintLog (0, "incoming data package too large (%d bytes)\n", dataSize);
		continue;
		}
	memcpy (data, buf + dataOffs, dataSize - dataOffs);
	locks.UnlockRecv ();
	return dataSize - dataOffs;
	}
locks.UnlockRecv ();
return 0;
}

//------------------------------------------------------------------------------

void IPXSendPacketData (uint8_t *data, int32_t dataSize, uint8_t *network, uint8_t *source, uint8_t *dest)
{
if (dataSize > int32_t (MAX_PAYLOAD_SIZE)) 
	PrintLog (0, "IpxSendPacketData: packet too large (%d bytes)\n", dataSize);
else {
		static uint8_t buf [MAX_PACKET_SIZE];
		IPXPacket_t ipxHeader;
	
	locks.LockSend ();
	memcpy (ipxHeader.Destination.Network, network, 4);
	memcpy (ipxHeader.Destination.Node, dest, 6);
	*reinterpret_cast<uint16_t*> (ipxHeader.Destination.Socket) = htons (ipxSocketData.socket);
	ipxHeader.PacketType = 4; /* Packet Exchange */
	if (gameStates.multi.bTrackerCall)
		memcpy (buf, data, dataSize);
	else {
		*reinterpret_cast<uint32_t*> (buf) = nIpxPacket++;
		memcpy (buf + 4, data, dataSize);
		}
	driver->SendPacket (&ipxSocketData, &ipxHeader, buf, dataSize + (gameStates.multi.bTrackerCall ? 0 : 4));
	locks.UnlockSend ();
	}
}

//------------------------------------------------------------------------------

void IpxGetLocalTarget (uint8_t *server, uint8_t *node, uint8_t *local_target)
{
// let's hope Linux knows how to route it
memcpy (local_target, node, 6);
}

//------------------------------------------------------------------------------

void IPXSendBroadcastData (uint8_t *data, int32_t dataSize)	
{
	int32_t i, j;
	uint8_t broadcast [] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	uint8_t localAddress [6];

if (gameStates.multi.nGameType > IPX_GAME)
	networkThread.Send (data, dataSize, reinterpret_cast<uint8_t*> (ipxNetworks), broadcast, broadcast);
else {
	// send to all networks besides mine
	for (i = 0; i < nIpxNetworks; i++) {
		if (memcmp (ipxNetworks + i, &ipx_network, 4)) {
			IpxGetLocalTarget (reinterpret_cast<uint8_t*> (ipxNetworks) + i, broadcast, localAddress);
			networkThread.Send (data, dataSize, reinterpret_cast<uint8_t*> (ipxNetworks + i), broadcast, localAddress);
			} 
		else {
			networkThread.Send (data, dataSize, reinterpret_cast<uint8_t*> (ipxNetworks + i), broadcast, broadcast);
			}
		}
	// Send directly to all users not on my network or in the network list.
	for (i = 0; i < nIpxUsers; i++) {
		if (memcmp (ipxUsers [i].network, &ipx_network, 4)) {
			for (j = 0; j < nIpxNetworks; j++)	
				if (!memcmp (ipxUsers [i].network, ipxNetworks + j, 4))
					break;
				if (j < nIpxNetworks)
					networkThread.Send (data, dataSize, ipxUsers [i].network, ipxUsers [i].node, ipxUsers [i].address);
			}
		}
	}
}

//------------------------------------------------------------------------------

// Sends a non-localized packet... needs 4 byte network, 6 byte address
void IPXSendInternetPacketData (uint8_t *data, int32_t dataSize, uint8_t *network, uint8_t *node)
{
	uint8_t localAddress [6];

if (*reinterpret_cast<uint32_t*> (network) != 0) {
	IpxGetLocalTarget (network, node, localAddress);
	IPXSendPacketData (data, dataSize, network, node, localAddress);
	} 
else {
	// Old method, no server info.
	IPXSendPacketData (data, dataSize, network, node, node);
	}
}

//------------------------------------------------------------------------------

int32_t IpxChangeDefaultSocket (uint16_t nSocket, int32_t bKeepClients)
{
if (!bIpxInstalled) 
	return -3;
gameStates.multi.bKeepClients = bKeepClients;
driver->CloseSocket (&ipxSocketData);
gameStates.multi.bKeepClients = 0;
return (driver->OpenSocket (&ipxSocketData, nSocket)) ? -3 : 0;
}

//------------------------------------------------------------------------------

void IpxReadUserFile (const char * filename)
{
	FILE * fp;
	user_address tmp;
	char szTemp [132], *p1;
	int32_t n, ln=0, x;

if (!filename) 
	return;
nIpxUsers = 0;
if (!(fp = fopen (filename, "rb")))
	return;
//printf ("Broadcast Users:\n");
while (fgets (szTemp, sizeof (szTemp), fp)) {
	ln++;
	if ((p1 = strchr (szTemp,'\n')))
		*p1 = '\0';
	if ((p1 = strchr (szTemp,';')))
		*p1 = '\0';
	if ((strlen (szTemp) < 21) || (szTemp [8] != '/'))
		continue;
	for (n = 0; n < 4; n++) {
		if (sscanf (szTemp + n * 2, "%2x", &x) != 1)
			break;
		tmp.network [n] = x;
		}
	if (n != 4)
		continue;
	for (n = 0; n < 6; n++) {
		if (sscanf (szTemp + 9 + n * 2, "%2x", &x) != 1)
			break;
		tmp.node [n] = x;
		}
	if (n != 6)
		continue;
	if (nIpxUsers < MAX_USERS) {
		IpxGetLocalTarget (tmp.network, tmp.node, tmp.address);
		ipxUsers [nIpxUsers++] = tmp;
		} 
	else {
		fclose (fp);
		return;
		}
	}
fclose (fp);
}

//------------------------------------------------------------------------------

void IpxReadNetworkFile (const char * filename)
{
	FILE * fp;
	user_address tmp;
	char szTemp [132], *p1;
	int32_t j, n, ln = 0, x;

if (!(filename && *filename)) 
	return;
if (!(fp = fopen (filename, "rb")))
	return;

while (fgets (szTemp, sizeof (szTemp), fp)) {
	ln++;
	if ((p1 = strchr (szTemp,'\n')))
		*p1 = '\0';
	if ((p1 = strchr (szTemp,';')))
		*p1 = '\0';
	if (strlen (szTemp) < 8)
		continue;
	for (n = 0; n < 4; n++) {
		if (sscanf (szTemp + n * 2, "%2x", &x) != 1)
			break;
		tmp.network [n] = x;
		}
	if (n != 4)
		continue;
	if (nIpxNetworks >= MAX_NETWORKS)
		break;
	for (j = 0; j < nIpxNetworks; j++)	
		if (!memcmp (&ipxNetworks [j], tmp.network, 4))
			break;
		if (j >= nIpxNetworks) {
			memcpy (ipxNetworks + nIpxNetworks++, tmp.network, 4);
		}
	}
fclose (fp);
}

//------------------------------------------------------------------------------

// Initalizes the protocol-specific member of the netgame packet.
void IpxInitNetGameAuxData (uint8_t buf [])
{
if (driver->InitNetGameAuxData)
	driver->InitNetGameAuxData (&ipxSocketData, buf);
}

//------------------------------------------------------------------------------

// Handles the protocol-specific member of the netgame packet.
int32_t IpxHandleNetGameAuxData (const uint8_t buf [])
{
if (driver->HandleNetGameAuxData)
	return driver->HandleNetGameAuxData (&ipxSocketData, buf);
return 0;
}

//------------------------------------------------------------------------------

// Notifies the protocol that we're done with a particular game
void IpxHandleLeaveGame ()
{
if (driver->HandleLeaveGame)
	driver->HandleLeaveGame (&ipxSocketData);
}

//------------------------------------------------------------------------------
// Send a packet to one member of the game.

int32_t IpxSendPlayerPacket (uint8_t nPlayer, uint8_t *data, int32_t dataSize)
{
if (gameData.multiplayer.players [nPlayer].IsConnected () && (nPlayer != N_LOCALPLAYER))
	networkThread.Send (
		data, dataSize, 
		netPlayers [0].m_info.players [nPlayer].network.Network (), 
		netPlayers [0].m_info.players [nPlayer].network.Node (),
		gameData.multiplayer.players [nPlayer].netAddress);
return 0;
}

//------------------------------------------------------------------------------

// Send a packet to every member of the game.
int32_t IpxSendGamePacket (uint8_t *data, int32_t dataSize)
{
if (driver->SendGamePacket) {
	if (dataSize > int32_t (MAX_PAYLOAD_SIZE - 4))
		PrintLog (0, "IpxSendGamePacket: packet too large (%d bytes)\n", dataSize);
	else {
		static uint8_t buf [MAX_PACKET_SIZE];
		*reinterpret_cast<uint32_t*> (buf) = nIpxPacket++;
		memcpy (buf + 4, data, dataSize);
		*reinterpret_cast<uint32_t*> (data) = nIpxPacket++;
		return driver->SendGamePacket (&ipxSocketData, buf, dataSize + 4);
		}
	} 
else {
	// Loop through all the players unicasting the packet.
	//printf ("Sending game packet: gameData.multiplayer.nPlayers = %i\n", gameData.multiplayer.nPlayers);
	for (uint8_t nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++)
		IpxSendPlayerPacket (nPlayer, data, dataSize);
	return dataSize;
	}
return 0;
}

//------------------------------------------------------------------------------
