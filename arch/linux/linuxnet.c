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
#include "inferno.h"
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
#include "../../main/player.h"	/* for gameData.multiplayer.players */
#include "../../main/multi.h"	/* for netPlayers */
//added 05/17/99 Matt Mueller - needed to redefine FD_* so that no asm is used
//#include "checker.h"
//end addition -MM
#include "byteswap.h"
#include "network.h"
#include "../../main/player.h"	/* for gameData.multiplayer.players */
#include "../../main/multi.h"	/* for netPlayers */
#include "tracker.h"
#include "hudmsg.h"

#define MAX_IPX_DATA 576

int ipx_fd;
ipx_socket_t ipxSocketData;
ubyte bIpxInstalled=0;
ushort ipx_socket = 0;
uint ipxNetwork = 0;
ubyte ipx_MyAddress[10];
int nIpxPacket = 0;			/* Sequence number */
//int     ipx_packettotal=0,ipx_lastspeed=0;

/* User defined routing stuff */
typedef struct user_address {
	ubyte network[4];
	ubyte node[6];
	ubyte address[6];
} user_address;
#define MAX_USERS 64
int nIpxUsers = 0;
user_address ipxUsers[MAX_USERS];

#define MAX_NETWORKS 64
int nIpxNetworks = 0;
uint ipxNetworks[MAX_NETWORKS];

//------------------------------------------------------------------------------

int IxpGeneralPacketReady (ipx_socket_t *s) 
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

ubyte * IpxGetMyServerAddress ()
{
return (ubyte *)&ipxNetwork;
}

//------------------------------------------------------------------------------

ubyte * IpxGetMyLocalAddress ()
{
return (ubyte *) (ipx_MyAddress + 4);
}

//------------------------------------------------------------------------------

void ArchIpxSetDriver (int ipx_driver)
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

int IpxInit (int socket_number)
{
	int i;

if ((i = FindArg ("-ipxnetwork")) && Args[i + 1]) {
	unsigned long n = strtol (Args[i + 1], NULL, 16);
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

struct ipx_recv_data ipx_udpSrc;

int IpxGetPacketData (ubyte * data)
{
	static char buf [MAX_IPX_DATA];
	int size, offs;

while (driver->PacketReady (&ipxSocketData)) {
	size = driver->ReceivePacket (&ipxSocketData, buf, sizeof (buf), &ipx_udpSrc);
	if (size < 0)
		break;
	if (size < 6)
		continue;
	if (size > MAX_IPX_DATA - 4)
		continue;
	offs = IsTracker (*((unsigned int *) ipx_udpSrc.src_node), *((ushort *) (ipx_udpSrc.src_node + 4))) ? 0 : 4;
	memcpy (data, buf + offs, size - offs);
	return size - offs;
	}
return 0;
}

//------------------------------------------------------------------------------

void IPXSendPacketData
	 (ubyte * data, int datasize, ubyte *network, ubyte *source, ubyte *dest)
{
	static u_char buf[MAX_IPX_DATA];
	IPXPacket_t ipxHeader;

if (datasize <= MAX_IPX_DATA - 4) {
	memcpy (ipxHeader.Destination.Network, network, 4);
	memcpy (ipxHeader.Destination.Node, dest, 6);
	*((u_short *) &ipxHeader.Destination.Socket [0]) = htons (ipxSocketData.socket);
	ipxHeader.PacketType = 4; /* Packet Exchange */
	if (gameStates.multi.bTrackerCall)
		memcpy (buf, data, datasize);
	else {
		*(uint *)buf = nIpxPacket++;
		memcpy (buf + 4, data, datasize);
		}
	driver->SendPacket (&ipxSocketData, &ipxHeader, buf, datasize + (gameStates.multi.bTrackerCall ? 0 : 4));
	}
}

#else

int IpxGetPacketData (ubyte * data)
{
	struct ipx_recv_data rd;
	char buf[MAX_IPX_DATA];
	int size;
	int best_size = 0;

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

void IPXSendPacketData (ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address)
{
	u_char buf[MAX_IPX_DATA];
	IPXPacket_t ipxHeader;

	Assert (datasize <= MAX_IPX_DATA+4);

memcpy (ipxHeader.Destination.Network, network, 4);
memcpy (ipxHeader.Destination.Node, immediate_address, 6);
*(u_short *) ipxHeader.Destination.Socket = htons (ipxSocketData.socket);
ipxHeader.PacketType = 4; /* Packet Exchange */
*(uint *) buf = INTEL_INT (nIpxPacket);
nIpxPacket++;
memcpy (buf + 4, data, datasize);
driver->SendPacket (&ipxSocketData, &ipxHeader, buf, datasize + 4);
}

#endif

void IpxGetLocalTarget (ubyte * server, ubyte * node, ubyte * local_target)
{
	// let's hope Linux knows how to route it
	memcpy (local_target, node, 6);
}

//------------------------------------------------------------------------------

void IPXSendBroadcastData (ubyte * data, int datasize)
{
	int i, j;
	ubyte broadcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	ubyte local_address[6];

	// Set to all networks besides mine
if (gameStates.multi.nGameType > IPX_GAME)
	IPXSendPacketData (data, datasize, (ubyte *) ipxNetworks, broadcast, broadcast);
else {
	for (i = 0; i < nIpxNetworks; i++)	{
		if (memcmp (ipxNetworks + i, &ipxNetwork, 4))	{
			IpxGetLocalTarget ((ubyte *) (ipxNetworks + i), broadcast, local_address);
			IPXSendPacketData (data, datasize, (ubyte *) (ipxNetworks + i), broadcast, local_address);
			} 
		else {
			IPXSendPacketData (data, datasize, (ubyte *)&ipxNetworks[i], broadcast, broadcast);
			}
		}
	//OLDipx_send_packet_data (data, datasize, (ubyte *)&ipxNetwork, broadcast, broadcast);
	// Send directly to all users not on my network or in the network list.
	for (i = 0; i < nIpxUsers; i++)	{
		if (memcmp (ipxUsers [i].network, &ipxNetwork, 4)) {
			for (j = 0; j < nIpxNetworks; j++)
				if (!memcmp (ipxUsers [i].network, ipxNetworks + j, 4))
					break;
			if (j == nIpxNetworks)
				IPXSendPacketData (data, datasize, ipxUsers[i].network, ipxUsers[i].node, ipxUsers[i].address);
			j = 0;
			}
		}
	}
}

//------------------------------------------------------------------------------
// Sends a non-localized packet... needs 4 byte server, 6 byte address
void IPXSendInternetPacketData (ubyte * data, int datasize, ubyte * server, ubyte *address)
{
	ubyte local_address[6];

#ifdef WORDS_NEED_ALIGNMENT
int zero = 0;
if (memcmp (server, &zero, 4)) {
#else // WORDS_NEED_ALIGNMENT
if ((*(uint *)server) != 0) {
#endif // WORDS_NEED_ALIGNMENT
	IpxGetLocalTarget (server, address, local_address);
	IPXSendPacketData (data, datasize, server, address, local_address);
	} 
else { // Old method, no server info.
	IPXSendPacketData (data, datasize, server, address, address);
	}
}

//------------------------------------------------------------------------------

int IpxChangeDefaultSocket (ushort socket_number)
{
	if (!bIpxInstalled) return -3;

	driver->CloseSocket (&ipxSocketData);
	if (driver->OpenSocket (&ipxSocketData, socket_number)) {
		return -3;
	}
	return 0;
}

//------------------------------------------------------------------------------

void IpxReadUserFile (char * filename)
{
	FILE * fp;
	user_address tmp;
	char szTemp[132], *p1;
	int n, ln=0, x;

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
#if 1 // adb: replaced sscanf (..., "%2x...", (char *)...) with better, but longer code
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
		//ubyte * ipx_real_buffer = (ubyte *) (&tmp);
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

void IpxReadNetworkFile (char * filename)
{
	FILE 				*fp;
	user_address	tmp;
	char 				szTemp[132], *p1;
	int 				n, ln=0, x;

if (!filename) 
	return;
if (! (fp = fopen (filename, "rt")))
	return;
#if 0
//printf ("Using Networks:\n");
for (i=0; i<nIpxNetworks; i++)		{
	ubyte * n1 = (ubyte *)&ipxNetworks[i];
	//printf ("* %02x%02x%02x%02x\n", n1[0], n1[1], n1[2], n1[3]);
	}
#endif
while (fgets (szTemp, sizeof (szTemp), fp)) {
	ln++;
	if ((p1 = strchr (szTemp,'\n')))
		*p1 = '\0';
	if ((p1 = strchr (szTemp,';')))
		*p1 = '\0';
#if 1 // adb: replaced sscanf (..., "%2x...", (char *)...) with better, but longer code
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
		int j;
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
void IpxInitNetGameAuxData (ubyte buf[])
{
if (driver->InitNetgameAuxData)
	driver->InitNetgameAuxData (&ipxSocketData, buf);
}

//------------------------------------------------------------------------------
// Handles the protocol-specific member of the netgame packet.
int IpxHandleNetGameAuxData (const ubyte buf[])
{
if (driver->HandleNetgameAuxData)
	return driver->HandleNetgameAuxData (&ipxSocketData, buf);
return 0;
}

//------------------------------------------------------------------------------
// Notifies the protocol that we're done with a particular game
void ipx_handle_leave_game ()
{
	if (driver->HandleLeaveGame)
		driver->HandleLeaveGame (&ipxSocketData);
}

//------------------------------------------------------------------------------
// Send a packet to every member of the game.
int IpxSendGamePacket (ubyte *data, int datasize)
{
if (driver->SendGamePacket) {
		u_char buf[MAX_IPX_DATA];

	*((uint *)buf) = nIpxPacket++;
	memcpy (buf + 4, data, datasize);
	*((uint *)data) = nIpxPacket++;
	return driver->SendGamePacket (&ipxSocketData, buf, datasize + 4);
	}
else {
	// Loop through all the players unicasting the packet.
	int i;
	//printf ("Sending game packet: gameData.multiplayer.nPlayers = %i\n", gameData.multiplayer.nPlayers);
	for (i=0; i<gameData.multiplayer.nPlayers; i++) {
		if (gameData.multiplayer.players [i].connected && (i != gameData.multiplayer.nLocalPlayer))
			IPXSendPacketData (data, datasize, netPlayers.players[i].network.ipx.server, 
									 netPlayers.players[i].network.ipx.node, gameData.multiplayer.players[i].netAddress);
		}
	return datasize;
	}
return 0;
}

//------------------------------------------------------------------------------
