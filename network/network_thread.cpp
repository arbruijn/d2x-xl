#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#	include <winsock.h>
#else
#	include <sys/socket.h>
#endif
#ifndef _WIN32
#	include <arpa/inet.h>
#	include <netinet/in.h> /* for htons & co. */
#endif

#include "descent.h"
#include "byteswap.h"
#include "timer.h"
#include "error.h"
#include "objeffects.h"
#include "ipx.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "multibot.h"
#include "netmenu.h"
#include "autodl.h"
#include "tracker.h"
#include "playerprofile.h"
#include "gamecntl.h"
#include "text.h"
#include "console.h"

#ifndef _WIN32
#	include "linux/include/ipx_udp.h"
#	include "linux/include/ipx_drv.h"
#endif

//------------------------------------------------------------------------------

CNetworkThread networkThread;

//------------------------------------------------------------------------------

static int _CDECL_ NetworkThreadHandler (void* nThreadP)
{
networkThread.Process ();
return 0;
}

//------------------------------------------------------------------------------

#define SENDLOCK 1
#define RECVLOCK 1
#define PROCLOCK 1

void CNetworkThread::Process (void)
{
for (;;) {
	UpdatePlayers ();
	G3_SLEEP (10);
	}
}

//------------------------------------------------------------------------------

CNetworkThread::CNetworkThread () 
	: m_thread (NULL), m_semaphore (NULL), m_sendLock (NULL), m_recvLock (NULL), m_processLock (NULL), m_nThreadId (0), m_bListen (false), m_nPackets (0)
{
m_packets [0] = 
m_packets [1] = NULL;
}

//------------------------------------------------------------------------------

void CNetworkThread::Start (void)
{
if (!m_thread) {
	m_thread = SDL_CreateThread (NetworkThreadHandler, &m_nThreadId);
	m_semaphore = SDL_CreateSemaphore (1);
	m_sendLock = SDL_CreateMutex ();
	m_recvLock = SDL_CreateMutex ();
	m_processLock = SDL_CreateMutex ();
	m_bListen = true;
	}
}

//------------------------------------------------------------------------------

void CNetworkThread::End (void)
{
if (m_thread) {
	SDL_KillThread (m_thread);
	if (m_semaphore) {
		SDL_DestroySemaphore (m_semaphore);
		m_semaphore = NULL;
		}
	if (m_sendLock) {
		SDL_DestroyMutex (m_sendLock);
		m_sendLock = NULL;
		}
	if (m_recvLock) {
		SDL_DestroyMutex (m_recvLock);
		m_recvLock = NULL;
		}
	if (m_processLock) {
		SDL_DestroyMutex (m_processLock);
		m_processLock = NULL;
		}
	FlushPackets ();
	m_thread = NULL;
	m_bListen = false;
	}
}

//------------------------------------------------------------------------------

void CNetworkThread::FlushPackets (void)
{
while (m_packets [0]) {
	m_packets [1] = m_packets [0];
	m_packets [0] = m_packets [0]->nextPacket;
	delete m_packets [1];
	}
m_packets [1] = NULL;
m_nPackets = 0;

}

//------------------------------------------------------------------------------

int CNetworkThread::SemWait (void) 
{ 
if (!m_semaphore)
	return 0;
SDL_SemWait (m_semaphore); 
return 1;
}

//------------------------------------------------------------------------------

int CNetworkThread::SemPost (void) 
{ 
if (!m_semaphore)
	return 0;
SDL_SemPost (m_semaphore); 
return 1;
}

//------------------------------------------------------------------------------

int CNetworkThread::LockSend (void) 
{ 
#if SENDLOCK
if (!m_sendLock)
	return 0;
SDL_LockMutex (m_sendLock); 
#endif
return 1;
}

//------------------------------------------------------------------------------

int CNetworkThread::UnlockSend (void) 
{ 
#if SENDLOCK
if (!m_sendLock)
	return 0;
SDL_UnlockMutex (m_sendLock); 
#endif
return 1;
}

//------------------------------------------------------------------------------

int CNetworkThread::LockRecv (void) 
{ 
#if RECVLOCK
if (!m_recvLock)
	return 0;
SDL_LockMutex (m_recvLock); 
#endif
return 1;
}

//------------------------------------------------------------------------------

int CNetworkThread::UnlockRecv (void) 
{ 
#if RECVLOCK
if (!m_recvLock)
	return 0;
SDL_UnlockMutex (m_recvLock); 
#endif
return 1;
}

//------------------------------------------------------------------------------

int CNetworkThread::LockProcess (void) 
{ 
#if PROCLOCK
if (!m_processLock)
	return 0;
SDL_LockMutex (m_processLock); 
#endif
return 1;
}

//------------------------------------------------------------------------------

int CNetworkThread::UnlockProcess (void) 
{ 
#if PROCLOCK
if (!m_processLock)
	return 0;
SDL_UnlockMutex (m_processLock); 
#endif
return 1;
}

//------------------------------------------------------------------------------
// Check for player timeouts

void CNetworkThread::UpdatePlayers (void)
{
	static CTimeout toUpdate (500);

if (toUpdate.Expired ()) {
	tracker.AddServer ();

	bool bDownloading = downloadManager.Downloading (N_LOCALPLAYER);
	
	if (LOCALPLAYER.Connected (CONNECT_END_MENU) || LOCALPLAYER.Connected (CONNECT_ADVANCE_LEVEL))
		NetworkSendEndLevelPacket ();
	else {
		//SemWait ();
		for (int nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++) {
			if (nPlayer == N_LOCALPLAYER) 
				continue;
			if (gameData.multiplayer.players [nPlayer].Connected (CONNECT_END_MENU) || gameData.multiplayer.players [nPlayer].Connected (CONNECT_ADVANCE_LEVEL)) {
#if 1
				NetworkSendEndLevelPacket ();
#else
				pingStats [nPlayer].launchTime = -1;
				NetworkSendPing (nPlayer);
#endif
				}
			else if (bDownloading) {
				pingStats [nPlayer].launchTime = -1;
				NetworkSendPing (nPlayer);
				}	
			}
		//SemPost ();
		}
	}

CheckPlayerTimeouts ();
}

//------------------------------------------------------------------------------

int CNetworkThread::Listen (void)
{
	static CTimeout toListen (10);

if (!toListen.Expired ())
	return 0;

#if 1 // network reads all network packets independently of main thread

// read all available network packets and append them to the end of the list of unprocessed network packets
ubyte data [MAX_PACKET_SIZE];
int size;
while (size = IpxGetPacketData (data)) {
	tNetworkPacket* packet = new tNetworkPacket;
	if (!packet)
		break;
	++m_nPackets;
	if (m_packets [1]) // list tail
		m_packets [1]->nextPacket = packet;
	else 
		m_packets [0] = packet; // list head
	m_packets [1] = packet;
	memcpy (&packet->owner.address, &networkData.packetSource, sizeof (networkData.packetSource));

	}

return m_nPackets;

#else

if (!GetListen ())
	return 0;
if (LOCALPLAYER.Connected (CONNECT_PLAYING))
	return 0;
return NetworkListen ();

#endif
}

//------------------------------------------------------------------------------

tNetworkPacket* CNetworkThread::GetPacket (void)
{
tNetworkPacket* packet;
if (packet = m_packets [0]) {
	m_packets [0] = packet->nextPacket;
	--m_nPackets;
	}
return packet;
}

//------------------------------------------------------------------------------

int CNetworkThread::GetPacketData (ubyte* data)
{
if (!Available ())
	return IpxGetPacketData (data);

tNetworkPacket* packet = GetPacket ();
if (!packet)
	return 0;

memcpy (data, packet->data, packet->size);
memcpy (&networkData.packetSource, &packet->owner.address, sizeof (networkData.packetSource));
int size = packet->size;
delete packet;
return size;
}

//------------------------------------------------------------------------------

int CNetworkThread::ProcessPackets (void)
{
	int nProcessed = 0;
	tNetworkPacket* packet;

while (packet = GetPacket ()) {
	if (NetworkProcessPacket (packet->data, packet->size))
		++nProcessed;
	delete packet;
	}
return nProcessed;
}

//------------------------------------------------------------------------------

int CNetworkThread::ConnectionStatus (int nPlayer)
{
if (!gameData.multiplayer.players [nPlayer].callsign [0])
	return 0;
if (gameData.multiplayer.players [nPlayer].m_nLevel && (gameData.multiplayer.players [nPlayer].m_nLevel != missionManager.nCurrentLevel))
	return 3;	// the client being tested is in a different level, so immediately disconnect him to make his ship disappear until he enters the current level
if (downloadManager.Downloading (nPlayer))
	return 1;	// try to reconnect, using relaxed timeout values
if ((gameData.multiplayer.players [nPlayer].connected == CONNECT_DISCONNECTED) || (gameData.multiplayer.players [nPlayer].connected == CONNECT_PLAYING))
	return 2;	// try to reconnect
if (LOCALPLAYER.connected == CONNECT_PLAYING)
	return 3; // the client being tested is in some level transition mode, so immediately disconnect him to make his ship disappear until he enters the current level
return 2;	// we are in some level transition mode too, so try to reconnect
}

//------------------------------------------------------------------------------

int CNetworkThread::CheckPlayerTimeouts (void)
{
SemWait ();
int nTimedOut = 0;
//if ((networkData.xLastTimeoutCheck > I2X (1)) && !gameData.reactor.bDestroyed) 
static CTimeout to (500);
int s = -1;
if (to.Expired () /*&& !gameData.reactor.bDestroyed*/) 
	{
	int t = SDL_GetTicks ();
	for (int nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++) {
		if (nPlayer != N_LOCALPLAYER) {
			switch (s = ConnectionStatus (nPlayer)) {
				case 0:
					break; // no action 

				case 1:
					if (networkData.nLastPacketTime [nPlayer] + downloadManager.GetTimeoutSecs () * 1000 > t) {
						ResetPlayerTimeout (nPlayer, t);
						break;
						}

				case 2:
					if ((networkData.nLastPacketTime [nPlayer] == 0) || (t - networkData.nLastPacketTime [nPlayer] < TIMEOUT_DISCONNECT)) {
						ResetPlayerTimeout (nPlayer, t);
						break;
						}

				default:
					if (NetworkTimeoutPlayer (nPlayer, t))
						nTimedOut++;
					break;
				}
			}
		}
	//networkData.xLastTimeoutCheck = 0;
	}
SemPost ();
return nTimedOut;
}

//------------------------------------------------------------------------------

