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
variable ipx_udpSrc (happens in ipx_udp.c::UDPReceivePacket()), which is needed on some special 
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
address data is not valid in UDP/IP communications, and this is where we need ipx_udpSrc from 
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
		(pid != PID_PDATA) &&
		(pid != PID_NAKED_PDATA) &&
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
	memcpy (&their->player.network.ipx.server, &ipx_udpSrc.src_network, 10);
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

#include "inferno.h"
#include "args.h"
#include "error.h"
#include "ipx.h"
#include "../win32/include/ipx_drv.h"
#include "../win32/include/ipx_udp.h"
#include "../win32/include/ipx_mcast4.h"
#include "network.h"
#include "player.h"	/* for gameData.multiplayer.players */
#include "multi.h"	/* for netPlayers */
#include "tracker.h"
#include "hudmsg.h"

extern struct ipx_driver ipx_win;

int ipx_fd;
ipx_socket_t ipxSocketData;
ubyte bIpxInstalled = 0;
ushort ipx_socket = 0;
uint ipx_network = 0;
ubyte ipx_MyAddress [10];
int nIpxPacket = 0;			/* Sequence number */

/* User defined routing stuff */
typedef struct user_address {
	ubyte network [4];
	ubyte node [6];
	ubyte address [6];
} user_address;

#define MAX_USERS 64
int nIpxUsers = 0;
user_address ipxUsers [MAX_USERS];

#define MAX_NETWORKS 64
int nIpxNetworks = 0;
uint ipxNetworks [MAX_NETWORKS];

								/*---------------------------*/

int IxpGeneralPacketReady (ipx_socket_t *s) 
{
	fd_set set;
	struct timeval tv;
	
FD_ZERO (&set);
FD_SET (s->fd, &set);
tv.tv_sec = tv.tv_usec = 0;
return (select (FD_SETSIZE, &set, NULL, NULL, &tv) > 0) ? 1 : 0;
}

								/*---------------------------*/

struct ipx_driver *driver = &ipx_win;

ubyte *IpxGetMyServerAddress ()
{
return reinterpret_cast<ubyte*> (&ipx_network);
}

								/*---------------------------*/

ubyte *IpxGetMyLocalAddress ()
{
return reinterpret_cast<ubyte*> (ipx_MyAddress + 4);
}

								/*---------------------------*/

void ArchIpxSetDriver (int ipx_driver)
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

								/*---------------------------*/

int IpxInit (int nSocket)
{
	int i;
	WSADATA wsaData;
   WORD wVersionRequested = MAKEWORD (2, 0);

if (WSAStartup (wVersionRequested, &wsaData))
	return IPX_SOCKET_ALREADY_OPEN;

if ((i = FindArg ("-ipxnetwork")) && pszArgList [i + 1]) {
	ulong n = strtol (pszArgList [i + 1], NULL, 16);
	for (i = 3; i >= 0; i--, n >>= 8)
		ipx_MyAddress [i] = (ubyte) n & 0xff; 
	}
if ((nSocket >= 0) && driver->OpenSocket (&ipxSocketData, nSocket)) {
	return IPX_NOT_INSTALLED;
	}
driver->GetMyAddress ();
memcpy (&ipx_network, ipx_MyAddress, 4);
nIpxNetworks = 0;
memcpy (ipxNetworks + nIpxNetworks++, &ipx_network, 4);
bIpxInstalled = 1;
atexit (IpxClose);
return IPX_INIT_OK;
}

								/*---------------------------*/

void _CDECL_ IpxClose (void)
{
PrintLog ("closing IPX socket\n");
if (bIpxInstalled) {
   WSACleanup ();
	driver->CloseSocket (&ipxSocketData);
   }
bIpxInstalled = 0;
}

								/*---------------------------*/

struct ipx_recv_data ipx_udpSrc;

int IpxGetPacketData (ubyte *data)
{
	static ubyte	buf [MAX_PACKETSIZE];

	int size, offs;

while (driver->PacketReady (&ipxSocketData)) {
	size = driver->ReceivePacket (reinterpret_cast<ipx_socket_t*> (&ipxSocketData), buf, sizeof (buf), &ipx_udpSrc);
#if 0//def _DEBUG
	HUDMessage (0, "received %d bytes from %d.%d.%d.%d:%u", 
					size,
					ipx_udpSrc.src_node [0],
					ipx_udpSrc.src_node [1],
					ipx_udpSrc.src_node [2],
					ipx_udpSrc.src_node [3],
					*(reinterpret_cast<ushort*> (ipx_udpSrc.src_node + 4)));
#endif
	if (size < 0)
		break;
	if (size < 6)
		continue;
	if (size > DATALIMIT - 4)
		continue;
	offs = IsTracker (*reinterpret_cast<uint*> (ipx_udpSrc.src_node), *reinterpret_cast<ushort*> (ipx_udpSrc.src_node + 4)) ? 0 : 4;
	memcpy (data, buf + offs, size - offs);
	return size - offs;
	}
return 0;
}

								/*---------------------------*/

void IPXSendPacketData
	 (ubyte *data, int datasize, ubyte *network, ubyte *source, ubyte *dest)
{
	static u_char buf [MAX_PACKETSIZE];
	IPXPacket_t ipxHeader;
	
if (datasize <= MAX_DATASIZE - 4) {
	memcpy (ipxHeader.Destination.Network, network, 4);
	memcpy (ipxHeader.Destination.Node, dest, 6);
	*reinterpret_cast<u_short*> (ipxHeader.Destination.Socket) = htons (ipxSocketData.socket);
	ipxHeader.PacketType = 4; /* Packet Exchange */
	if (gameStates.multi.bTrackerCall)
		memcpy (buf, data, datasize);
	else {
		*reinterpret_cast<uint*> (buf) = nIpxPacket++;
		memcpy (buf + 4, data, datasize);
		}
	driver->SendPacket (&ipxSocketData, &ipxHeader, buf, datasize + (gameStates.multi.bTrackerCall ? 0 : 4));
	}
}

								/*---------------------------*/

void IpxGetLocalTarget (ubyte *server, ubyte *node, ubyte *local_target)
{
// let's hope Linux knows how to route it
memcpy (local_target, node, 6);
}

								/*---------------------------*/

void IPXSendBroadcastData (ubyte *data, int datasize)	
{
	int i, j;
	ubyte broadcast [] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	ubyte localAddress [6];

if (gameStates.multi.nGameType > IPX_GAME)
	IPXSendPacketData (data, datasize, reinterpret_cast<ubyte*> (ipxNetworks), broadcast, broadcast);
else {
	// send to all networks besides mine
	for (i = 0; i < nIpxNetworks; i++) {
		if (memcmp (ipxNetworks + i, &ipx_network, 4)) {
			IpxGetLocalTarget (reinterpret_cast<ubyte*> (ipxNetworks) + i, broadcast, localAddress);
			IPXSendPacketData (data, datasize, reinterpret_cast<ubyte*> (ipxNetworks + i), broadcast, localAddress);
			} 
		else {
			IPXSendPacketData (data, datasize, reinterpret_cast<ubyte*> (ipxNetworks + i), broadcast, broadcast);
			}
		}
	// Send directly to all users not on my network or in the network list.
	for (i = 0; i < nIpxUsers; i++) {
		if (memcmp (ipxUsers [i].network, &ipx_network, 4)) {
			for (j = 0; j < nIpxNetworks; j++)	
				if (!memcmp (ipxUsers [i].network, ipxNetworks + j, 4))
					break;
				if (j < nIpxNetworks)
					IPXSendPacketData (data, datasize, ipxUsers [i].network, 
											ipxUsers [i].node, ipxUsers [i].address);
			}
		}
	}
}

								/*---------------------------*/

// Sends a non-localized packet... needs 4 byte server, 6 byte address
void IPXSendInternetPacketData (ubyte *data, int datasize, ubyte *server, ubyte *address)
{
	ubyte localAddress [6];

if (*reinterpret_cast<uint*> (server) != 0) {
	IpxGetLocalTarget (server, address, localAddress);
	IPXSendPacketData (data, datasize, server, address, localAddress);
	} 
else {
	// Old method, no server info.
	IPXSendPacketData (data, datasize, server, address, address);
	}
}

								/*---------------------------*/

int IpxChangeDefaultSocket (ushort nSocket)
{
if (!bIpxInstalled) 
	return -3;
driver->CloseSocket (&ipxSocketData);
return (driver->OpenSocket (&ipxSocketData, nSocket)) ? -3 : 0;
}

								/*---------------------------*/

void IpxReadUserFile (const char * filename)
{
	FILE * fp;
	user_address tmp;
	char szTemp [132], *p1;
	int n, ln=0, x;

if (!filename) 
	return;
nIpxUsers = 0;
if (!(fp = fopen (filename, "rt")))
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

								/*---------------------------*/

void IpxReadNetworkFile (const char * filename)
{
	FILE * fp;
	user_address tmp;
	char szTemp [132], *p1;
	int j, n, ln = 0, x;

if (!(filename && *filename)) 
	return;
if (!(fp = fopen (filename, "rt")))
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

								/*---------------------------*/

// Initalizes the protocol-specific member of the netgame packet.
void IpxInitNetGameAuxData (ubyte buf [])
{
if (driver->InitNetgameAuxData)
	driver->InitNetgameAuxData (&ipxSocketData, buf);
}

								/*---------------------------*/

// Handles the protocol-specific member of the netgame packet.
int IpxHandleNetGameAuxData (const ubyte buf [])
{
if (driver->HandleNetgameAuxData)
	return driver->HandleNetgameAuxData (&ipxSocketData, buf);
return 0;
}

								/*---------------------------*/

// Notifies the protocol that we're done with a particular game
void IpxHandleLeaveGame ()
{
if (driver->HandleLeaveGame)
	driver->HandleLeaveGame (&ipxSocketData);
}

								/*---------------------------*/

// Send a packet to every member of the game.
int IpxSendGamePacket (ubyte *data, int datasize)
{
if ((driver->SendGamePacket) && (datasize <= MAX_DATASIZE - 4)) {
	static u_char buf [MAX_PACKETSIZE];

	*reinterpret_cast<uint*> (buf) = nIpxPacket++;
	memcpy (buf + 4, data, datasize);
	*reinterpret_cast<uint*> (data) = nIpxPacket++;
	return driver->SendGamePacket (&ipxSocketData, buf, datasize + 4);
	} 
else {
	// Loop through all the players unicasting the packet.
	int i;
	//printf ("Sending game packet: gameData.multiplayer.nPlayers = %i\n", gameData.multiplayer.nPlayers);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (gameData.multiplayer.players [i].connected && (i != gameData.multiplayer.nLocalPlayer))
			IPXSendPacketData (
				data, datasize, 
				netPlayers.players [i].network.ipx.server, 
				netPlayers.players [i].network.ipx.node,
				gameData.multiplayer.players [i].netAddress);
		}
	return datasize;
	}
return 0;
}
