/* $Id: linuxnet.c,v 1.13 2003/11/18 01:08:07 btb Exp $ */
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
 * Linux lower-level network code.
 * implements functions declared in include/ipx.h
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h> /* for htons & co. */

#include "pstypes.h"
#include "descent.h"
#include "args.h"
#include "error.h"

#include "ipx.h"
#include "ipx_drv.h"
#ifdef NATIVE_IPX
# include "ipx_bsd.h"
#endif //NATIVE_IPX
#ifdef KALINIX
#include "ipx_kali.h"
#endif
#include "ipx_udp.h"
#include "ipx_mcast4.h"
#include "error.h"
#include "player.h"	/* for gameData.multiplayer.players */
#include "multi.h"	/* for netPlayers */
//added 05/17/99 Matt Mueller - needed to redefine FD_* so that no asm is used
//#include "checker.h"
//end addition -MM
#include "byteswap.h"
#include "network.h"
#include "network_lib.h"
#include "player.h"	/* for gameData.multiplayer.players */
#include "multi.h"	/* for netPlayers */
#include "tracker.h"
#include "hudmsgs.h"

int32_t ipx_fd;
ipx_socket_t ipxSocketData;
uint8_t bIpxInstalled=0;
uint16_t ipx_socket = 0;
uint32_t ipxNetwork = 0;
uint8_t ipx_MyAddress[10];
int32_t nIpxPacket = 0;			/* Sequence number */
//int32_t     ipx_packettotal=0,ipx_lastspeed=0;

/* User defined routing stuff */
typedef struct user_address {
	uint8_t network[4];
	uint8_t node[6];
	uint8_t address[6];
} user_address;
#define MAX_USERS 64
int32_t nIpxUsers = 0;
user_address ipxUsers[MAX_USERS];

#define MAX_NETWORKS 64
int32_t nIpxNetworks = 0;
uint32_t ipxNetworks[MAX_NETWORKS];

typedef union tUintCast {
	uint8_t	b [4];
	uint32_t		i;
} tUintCast;

typedef union tUshortCast {
	uint8_t	b [2];
	uint16_t	i;
} tUshortCast;

//------------------------------------------------------------------------------

int32_t IPXGeneralPacketReady (ipx_socket_t *s) 
{
	fd_set set;
	struct timeval tv;

FD_ZERO (&set);
FD_SET (s->fd, &set);
tv.tv_sec = tv.tv_usec = 0;
return (select (s->fd + 1, &set, NULL, NULL, &tv) > 0) ? 1 : 0;
}

//------------------------------------------------------------------------------

struct ipx_driver *driver = &ipx_udp;

uint8_t * IpxGetMyServerAddress ()
{
return reinterpret_cast<uint8_t*> (&ipxNetwork);
}

//------------------------------------------------------------------------------

uint8_t * IpxGetMyLocalAddress ()
{
return reinterpret_cast<uint8_t*> (ipx_MyAddress + 4);
}

//------------------------------------------------------------------------------

void ArchIpxSetDriver (int32_t ipx_driver)
{
switch (ipx_driver) {
#ifdef NATIVE_IPX
	case IPX_DRIVER_IPX: 
		driver = &ipx_bsd; 
		break;
#endif //NATIVE_IPX
#ifdef KALINIX
	case IPX_DRIVER_KALI: 
		driver = &ipx_kali; 
		break;
#endif
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

int32_t IpxInit (int32_t socket_number)
{
	int32_t i;

if ((i = FindArg ("-ipxnetwork")) && appConfig [i + 1]) {
	ulong n = strtol (appConfig [i + 1], NULL, 16);
	for (i = 3; i >= 0; i--, n >>= 8)
		ipx_MyAddress [i] = n & 0xff; 
	}
if (driver->OpenSocket (&ipxSocketData, socket_number)) {
	return IPX_NOT_INSTALLED;
	}
driver->GetMyAddress ();
memcpy (&ipxNetwork, ipx_MyAddress, 4);
nIpxNetworks = 0;
memcpy (ipxNetworks + nIpxNetworks++, &ipxNetwork, 4);
bIpxInstalled = 1;
atexit (IpxClose);
return IPX_INIT_OK;
}

//------------------------------------------------------------------------------

void IpxClose ()
{
if (bIpxInstalled)
   driver->CloseSocket (&ipxSocketData);
bIpxInstalled = 0;
}

#if 1

//------------------------------------------------------------------------------

//IPXRecvData_t networkData.packetSource;

int32_t IpxGetPacketData (uint8_t * data)
{
	static char buf [MAX_PACKET_SIZE];
	int32_t dataSize, dataOffs;

while (driver->PacketReady (&ipxSocketData)) {
	dataSize = driver->ReceivePacket (&ipxSocketData, buf, sizeof (buf), &networkData.packetSource);
	if (dataSize < 0)
		break;
	if (dataSize < 6)
		continue;
	dataOffs = tracker.IsTracker (networkData.packetSource.Server (), networkData.packetSource.Port (), (char*) buf) ? 0 : 4;
	if (dataSize > MAX_PAYLOAD_SIZE + dataOffs) {
		PrintLog (0, "incoming data package too large (%d bytes)\n", dataSize);
		continue;
		}
	memcpy (data, buf + dataOffs, dataSize - dataOffs);
	return dataSize - dataOffs;
	}
return 0;
}

//------------------------------------------------------------------------------

void IPXSendPacketData (uint8_t * data, int32_t dataSize, uint8_t *network, uint8_t *source, uint8_t *dest)
{
	static uint8_t buf [MAX_PACKET_SIZE];
	IPXPacket_t ipxHeader;

if (dataSize > MAX_PAYLOAD_SIZE)
	PrintLog (0, "outgoing data package too large (%d bytes)\n", dataSize);
else {
	memcpy (ipxHeader.Destination.Network, network, 4);
	memcpy (ipxHeader.Destination.Node, dest, 6);
	uint16_t s = htons (ipxSocketData.socket);
	memcpy (&ipxHeader.Destination.Socket [0], s, sizeof (s));
	ipxHeader.PacketType = 4; /* Packet Exchange */
	if (gameStates.multi.bTrackerCall)
		memcpy (buf, data, dataSize);
	else {
		memcpy (buf, nIpxPacket, sizeof (nIpxPacket));
		memcpy (buf + 4, data, dataSize);
		nIpxPacket++;
		}
	driver->SendPacket (&ipxSocketData, &ipxHeader, buf, dataSize + (gameStates.multi.bTrackerCall ? 0 : 4));
	}
}

#else

int32_t IpxGetPacketData (uint8_t * data)
{
	IPXRecvData_t rd;
	char buf[MAX_PACKETSIZE];
	int32_t size;
	int32_t best_size = 0;

while (driver->PacketReady (&ipxSocketData)) {
	if ((size = driver->ReceivePacket (&ipxSocketData, buf, sizeof (buf), &rd)) > 4) {
   	if (!memcmp (rd.src_network, ipx_MyAddress, 10)) 
	   	continue;	// don't get own pkts
	     	memcpy (data, buf + 4, size - 4);
			return size - 4;
		}
	}
return best_size;
}

//------------------------------------------------------------------------------

void IPXSendPacketData (uint8_t * data, int32_t dataSize, uint8_t *network, uint8_t *address, uint8_t *immediate_address)
{
if (dataSize > MAX_PAYLOAD_SIZE) 
	PrintLog (0, "IpxSendPacketData: packet too large (%d bytes)\n", dataSize);
else {
		uint8_t buf [MAX_PACKET_SIZE];
		IPXPacket_t ipxHeader;

	memcpy (ipxHeader.Destination.Network, network, 4);
	memcpy (ipxHeader.Destination.Node, immediate_address, 6);
	*reinterpret_cast<u_short*> (ipxHeader.Destination.Socket) = htons (ipxSocketData.socket);
	ipxHeader.PacketType = 4; /* Packet Exchange */
	u_int32_t i = INTEL_INT (nIpxPacket);
	memcpy (buf, i, sizeof (i));
	nIpxPacket++;
	memcpy (buf + 4, data, dataSize);
	driver->SendPacket (&ipxSocketData, &ipxHeader, buf, dataSize + 4);
	}
}

#endif

void IpxGetLocalTarget (uint8_t * server, uint8_t * node, uint8_t * local_target)
{
	// let's hope Linux knows how to route it
	memcpy (local_target, node, 6);
}

//------------------------------------------------------------------------------

void IPXSendBroadcastData (uint8_t * data, int32_t dataSize)
{
	int32_t i, j;
	uint8_t broadcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	uint8_t local_address[6];

	// Set to all networks besides mine
if (gameStates.multi.nGameType > IPX_GAME)
	IPXSendPacketData (data, dataSize, reinterpret_cast<uint8_t*> (ipxNetworks), broadcast, broadcast);
else {
	for (i = 0; i < nIpxNetworks; i++)	{
		if (memcmp (ipxNetworks + i, &ipxNetwork, 4))	{
			IpxGetLocalTarget (reinterpret_cast<uint8_t*> (ipxNetworks + i), broadcast, local_address);
			IPXSendPacketData (data, dataSize, reinterpret_cast<uint8_t*> (ipxNetworks + i), broadcast, local_address);
			} 
		else {
			IPXSendPacketData (data, dataSize, reinterpret_cast<uint8_t*> (&ipxNetworks[i]), broadcast, broadcast);
			}
		}
	//OLDipx_send_packet_data (data, dataSize, reinterpret_cast<uint8_t*> (&ipxNetwork), broadcast, broadcast);
	// Send directly to all users not on my network or in the network list.
	for (i = 0; i < nIpxUsers; i++)	{
		if (memcmp (ipxUsers [i].network, &ipxNetwork, 4)) {
			for (j = 0; j < nIpxNetworks; j++)
				if (!memcmp (ipxUsers [i].network, ipxNetworks + j, 4))
					break;
			if (j == nIpxNetworks)
				IPXSendPacketData (data, dataSize, ipxUsers[i].network, ipxUsers[i].node, ipxUsers[i].address);
			j = 0;
			}
		}
	}
}

//------------------------------------------------------------------------------
// Sends a non-localized packet... needs 4 byte server, 6 byte address
void IPXSendInternetPacketData (uint8_t * data, int32_t dataSize, uint8_t * server, uint8_t *address)
{
	uint8_t local_address[6];

#ifdef WORDS_NEED_ALIGNMENT
int32_t zero = 0;
if (memcmp (server, &zero, 4)) {
#else // WORDS_NEED_ALIGNMENT
if (*reinterpret_cast<uint32_t*> (server) != 0) {
#endif // WORDS_NEED_ALIGNMENT
	IpxGetLocalTarget (server, address, local_address);
	IPXSendPacketData (data, dataSize, server, address, local_address);
	} 
else { // Old method, no server info.
	IPXSendPacketData (data, dataSize, server, address, address);
	}
}

//------------------------------------------------------------------------------

int32_t IpxChangeDefaultSocket (uint16_t socket_number, int32_t bKeepClients)
{
if (!bIpxInstalled)
	return -3;
gameStates.multi.bKeepClients = bKeepClients;
driver->CloseSocket (&ipxSocketData);
gameStates.multi.bKeepClients = 0;
if (driver->OpenSocket (&ipxSocketData, socket_number))
	return -3;
return 0;
}

//------------------------------------------------------------------------------

void IpxReadUserFile (const char * filename)
{
	FILE * fp;
	user_address tmp;
	char szTemp[132], *p1;
	int32_t n, ln=0, x;

if (!filename) 
	return;
nIpxUsers = 0;
if (! (fp = fopen (filename, "rt")))
	return;
//printf ("Broadcast Users:\n");
while (fgets (szTemp, sizeof (szTemp), fp)) {
	ln++;
	if ((p1 = strchr (szTemp,'\n')))
		*p1 = '\0';
	if ((p1 = strchr (szTemp,';')))
		*p1 = '\0';
#if 1 // adb: replaced sscanf (..., "%2x...", (char *) (...) with better, but longer code
	if (strlen (szTemp) < 21 || szTemp[8] != '/')
		continue;
	for (n = 0; n < 4; n++) {
		if (sscanf (szTemp + n * 2, "%2x", &x) != 1)
			break;
		tmp.network[n] = x;
		}
	if (n != 4)
		continue;
	for (n = 0; n < 6; n++) {
		if (sscanf (szTemp + 9 + n * 2, "%2x", &x) != 1)
			break;
		tmp.node[n] = x;
		}
	if (n != 6)
		continue;
#else
		n = sscanf (szTemp, "%2x%2x%2x%2x/%2x%2x%2x%2x%2x%2x", &tmp.network[0], &tmp.network[1], &tmp.network[2], &tmp.network[3], &tmp.node[0], &tmp.node[1], &tmp.node[2],&tmp.node[3], &tmp.node[4], &tmp.node[5]);
		if (n != 10) continue;
#endif
	if (nIpxUsers < MAX_USERS)	{
		//uint8_t * ipx_real_buffer = reinterpret_cast<uint8_t*> (&tmp);
		IpxGetLocalTarget (tmp.network, tmp.node, tmp.address);
		ipxUsers[nIpxUsers++] = tmp;
		//printf ("%02X%02X%02X%02X/", ipx_real_buffer[0],ipx_real_buffer[1],ipx_real_buffer[2],ipx_real_buffer[3]);
		//printf ("%02X%02X%02X%02X%02X%02X\n", ipx_real_buffer[4],ipx_real_buffer[5],ipx_real_buffer[6],ipx_real_buffer[7],ipx_real_buffer[8],ipx_real_buffer[9]);
		} 
	else {
		//printf ("Too many addresses in %s! (Limit of %d)\n", filename, MAX_USERS);
		fclose (fp);
		return;
		}
	}
fclose (fp);
}

//------------------------------------------------------------------------------

void IpxReadNetworkFile (const char * filename)
{
	FILE 				*fp;
	user_address	tmp;
	char 				szTemp[132], *p1;
	int32_t 				n, ln=0, x;

if (!filename) 
	return;
if (! (fp = fopen (filename, "rt")))
	return;
#if 0
//printf ("Using Networks:\n");
for (i=0; i<nIpxNetworks; i++)		{
	uint8_t * n1 = reinterpret_cast<uint8_t*> (&ipxNetworks[i]);
	//printf ("* %02x%02x%02x%02x\n", n1[0], n1[1], n1[2], n1[3]);
	}
#endif
while (fgets (szTemp, sizeof (szTemp), fp)) {
	ln++;
	if ((p1 = strchr (szTemp,'\n')))
		*p1 = '\0';
	if ((p1 = strchr (szTemp,';')))
		*p1 = '\0';
#if 1 // adb: replaced sscanf (..., "%2x...", (char *) (...) with better, but longer code
	if (strlen (szTemp) < 8)
		continue;
	for (n = 0; n < 4; n++) {
		if (sscanf (szTemp + n * 2, "%2x", &x) != 1)
			break;
		tmp.network[n] = x;
		}
	if (n != 4)
		continue;
#else
	n = sscanf (szTemp, "%2x%2x%2x%2x", &tmp.network[0], &tmp.network[1], &tmp.network[2], &tmp.network[3]);
	if (n != 4) continue;
#endif
	if (nIpxNetworks < MAX_NETWORKS)	{
		int32_t j;
		for (j=0; j<nIpxNetworks; j++)
			if (!memcmp (ipxNetworks + j, tmp.network, 4))
				break;
		if (j >= nIpxNetworks)	{
			memcpy (ipxNetworks + nIpxNetworks++, tmp.network, 4);
			//printf ("  %02x%02x%02x%02x\n", tmp.network[0], tmp.network[1], tmp.network[2], tmp.network[3]);
			}
		} 
	else {
		//printf ("Too many networks in %s! (Limit of %d)\n", filename, MAX_NETWORKS);
		fclose (fp);
		return;
		}
	}
fclose (fp);
}

//------------------------------------------------------------------------------
// Initalizes the protocol-specific member of the netgame packet.
void IpxInitNetGameAuxData (uint8_t buf[])
{
if (driver->InitNetgameAuxData)
	driver->InitNetgameAuxData (&ipxSocketData, buf);
}

//------------------------------------------------------------------------------
// Handles the protocol-specific member of the netgame packet.
int32_t IpxHandleNetGameAuxData (const uint8_t buf[])
{
if (driver->HandleNetgameAuxData)
	return driver->HandleNetgameAuxData (&ipxSocketData, buf);
return 0;
}

//------------------------------------------------------------------------------
// Notifies the protocol that we're done with a particular game
void IpxHandleLeaveGame (void)
{
	if (driver->HandleLeaveGame)
		driver->HandleLeaveGame (&ipxSocketData);
}

//------------------------------------------------------------------------------
// Send a packet to every member of the game.
int32_t IpxSendGamePacket (uint8_t *data, int32_t dataSize)
{
if (driver->SendGamePacket) {
	if (dataSize > MAX_PACKET_SIZE - 4)
		PrintLog (0, "IpxSendGamePacket: packet too large (%d bytes)\n", dataSize);
	else {
		static uint8_t buf [MAX_PACKET_SIZE];
		tUintCast uintCast;
		uintCast.i = (uint32_t) nIpxPacket++;
		memcpy (buf, uintCast.b, sizeof (uintCast.b));
		memcpy (buf + 4, data, dataSize);
		*reinterpret_cast<uint32_t*> (data) = nIpxPacket++;
		return driver->SendGamePacket (&ipxSocketData, buf, dataSize + 4);
		}
	}
else {
	// Loop through all the players unicasting the packet.
	//printf ("Sending game packet: gameData.multiplayer.nPlayers = %i\n", gameData.multiplayer.nPlayers);
	for (int32_t i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (gameData.multiplayer.players [i].connected && (i != gameData.multiplayer.nLocalPlayer))
			IPXSendPacketData (
				data, dataSize, 
				netPlayers [0].m_info.players [i].network.Server (),
				netPlayers [0].m_info.players [i].network.Node (),
				gameData.multiplayer.players[i].netAddress);
		}
	return dataSize;
	}
return 0;
}

//------------------------------------------------------------------------------
