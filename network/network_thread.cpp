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
#include "ipx.h"
#include "byteswap.h"
#include "timer.h"
#include "error.h"
#include "objeffects.h"
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

#define LISTEN_TIMEOUT		5
#define SYNC_TIMEOUT			5
#define UPDATE_TIMEOUT		500
#define MAX_PACKET_AGE		3000

//------------------------------------------------------------------------------

CNetworkThread networkThread;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CNetworkPacket::Send (void)
{
IPXSendInternetPacketData (m_data, m_size, owner.address.Network (), owner.address.Node ());
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CNetworkPacketList::CNetworkPacketList ()
	: m_nPackets (0)
{
m_packets [0] = 
m_packets [1] = 
m_current = NULL;
m_semaphore = SDL_CreateSemaphore (1);
}

//------------------------------------------------------------------------------

CNetworkPacketList::~CNetworkPacketList ()
{
Flush ();
if (m_semaphore) {
	SDL_DestroySemaphore (m_semaphore);
	m_semaphore = NULL;
	}
}

//------------------------------------------------------------------------------

int CNetworkPacketList::Lock (void) 
{ 
if (!m_semaphore)
	return 0;
SDL_SemWait (m_semaphore); 
return 1;
}

//------------------------------------------------------------------------------

int CNetworkPacketList::Unlock (void) 
{ 
if (!m_semaphore)
	return 0;
SDL_SemPost (m_semaphore); 
return 1;
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkPacketList::Append (CNetworkPacket* packet, bool bAllowDuplicates)
{
Lock ();
if (!packet) {
	packet = new CNetworkPacket;
	if (!packet) {
		Unlock ();
		return NULL;
		}
	}

if (Tail ()) {// list tail
	if (!bAllowDuplicates && (*Tail () == *packet)) {
		Tail ()->timeStamp = SDL_GetTicks ();
		Unlock ();
		return packet;
		}
	Tail ()->nextPacket = packet;
	}
else 
	SetHead (packet); // list head
SetTail (packet);
Tail ()->timeStamp = SDL_GetTicks ();
Tail ()->nextPacket = NULL;
++m_nPackets;
Unlock ();
return packet;
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkPacketList::Pop (void)
{
CNetworkPacket* packet;
Lock ();
if (packet = m_packets [0]) {
	if (!(m_packets [0] = packet->nextPacket))
		m_packets [1] = NULL;
	--m_nPackets;
	}
Unlock ();
return packet;
}

//------------------------------------------------------------------------------

void CNetworkPacketList::Flush (void)
{
Lock ();
while (m_packets [0]) {
	m_packets [1] = m_packets [0];
	m_packets [0] = m_packets [0]->nextPacket;
	delete m_packets [1];
	}
m_packets [1] = NULL;
m_nPackets = 0;
Unlock ();
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkPacketList::Start (int nPacket) 
{ 
for (m_current = Head (); m_current && nPacket--; Next ())
	;
return m_current;
}

//------------------------------------------------------------------------------

bool CNetworkPacketList::Validate (void) 
{ 
Lock ();
bool bOk = (!Head () == !Tail ()); // both must be either NULL or not NULL
Unlock ();
return bOk;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static int _CDECL_ NetworkThreadHandler (void* nThreadP)
{
networkThread.Run ();
return 0;
}

//------------------------------------------------------------------------------

#define SENDLOCK 0
#define RECVLOCK 0
#define PROCLOCK 0
#define SYNCLOCK 1

void CNetworkThread::Run (void)
{
for (;;) {
	Update ();
	G3_SLEEP (1);
	}
}

//------------------------------------------------------------------------------

CNetworkThread::CNetworkThread () 
	: m_thread (NULL), m_semaphore (NULL), m_sendLock (NULL), m_recvLock (NULL), m_processLock (NULL), m_nThreadId (0), m_bListen (false)
{
}

//------------------------------------------------------------------------------

void CNetworkThread::Start (void)
{
if (!m_thread) {
	m_thread = SDL_CreateThread (NetworkThreadHandler, &m_nThreadId);
	m_semaphore = SDL_CreateSemaphore (1);
	m_syncLock = SDL_CreateSemaphore (0);
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
	if (m_syncLock) {
		SDL_DestroySemaphore (m_syncLock);
		m_syncLock = NULL;
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
	if (m_packet) {
		delete m_packet;
		m_packet = NULL;
		}
	FlushPackets ();
	m_thread = NULL;
	m_bListen = false;
	}
}

//------------------------------------------------------------------------------

void CNetworkThread::FlushPackets (void)
{
m_rxPacketQueue.Flush ();
}

//------------------------------------------------------------------------------

int CNetworkThread::Lock (void) 
{ 
if (!m_semaphore)
	return 0;
SDL_SemWait (m_semaphore); 
return 1;
}

//------------------------------------------------------------------------------

int CNetworkThread::Unlock (void) 
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

int CNetworkThread::LockSync (void) 
{ 
#if SYNCLOCK
if (!m_syncLock)
	return 0;
SDL_SemWait (m_syncLock); 
#endif
return 1;
}

//------------------------------------------------------------------------------

int CNetworkThread::UnlockSync (void) 
{ 
#if SYNCLOCK
if (!m_syncLock)
	return 0;
SDL_SemPost (m_syncLock); 
#endif
return 1;
}

//------------------------------------------------------------------------------
// Check for player timeouts

void CNetworkThread::Update (void)
{
	static CTimeout toUpdate (UPDATE_TIMEOUT);

if (toUpdate.Expired ()) {
	tracker.AddServer ();

	bool bDownloading = downloadManager.Downloading (N_LOCALPLAYER);
	
	if (LOCALPLAYER.Connected (CONNECT_END_MENU) || LOCALPLAYER.Connected (CONNECT_ADVANCE_LEVEL))
		NetworkSendEndLevelPacket ();
	else {
		//Lock ();
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
		//Unlock ();
		}
	}
if (Available ())
	Listen ();
CheckPlayerTimeouts ();
}

//------------------------------------------------------------------------------

void CNetworkThread::Cleanup (void)
{
m_rxPacketQueue.Lock ();
uint t = SDL_GetTicks () - MAX_PACKET_AGE; // drop packets older than 3 seconds
for (m_rxPacketQueue.Start (); m_rxPacketQueue.Current (); m_rxPacketQueue.Next ()) {
	if (m_rxPacketQueue.Current ()->timeStamp < t) 
		m_rxPacketQueue.Pop ();
	}
m_rxPacketQueue.Unlock ();
}

//------------------------------------------------------------------------------

int CNetworkThread::Listen (void)
{
	static CTimeout toListen (LISTEN_TIMEOUT);

if (!toListen.Expired ())
	return 0;

#if 1 // network reads all network packets independently of main thread
Cleanup ();
// read all available network packets and append them to the end of the list of unprocessed network packets
for (;;) {
	// pre-allocate packet so IpxGetPacketData can directly fill its buffer and no extra copy operation is necessary
	if (!m_packet) { 
		m_packet = new CNetworkPacket;
		if (!m_packet)
			break;
		}
	if (!m_packet->SetSize (IpxGetPacketData (m_packet->data)))
		break;
	m_rxPacketQueue.Lock ();
#if DBG
	if (!m_rxPacketQueue.Validate ())
		FlushPackets ();
#endif
	memcpy (&m_packet->owner.address, &networkData.packetSource, sizeof (networkData.packetSource));
	m_rxPacketQueue.Append (m_packet, false);
	m_packet = NULL;
	}

return m_rxPacketQueue.Length ();

#else

if (!GetListen ())
	return 0;
if (LOCALPLAYER.Connected (CONNECT_PLAYING))
	return 0;
return NetworkListen ();

#endif
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkThread::GetPacket (void)
{
return m_rxPacketQueue.Pop ();
}

//------------------------------------------------------------------------------

int CNetworkThread::GetPacketData (ubyte* data)
{
if (!Available ())
	return IpxGetPacketData (data);

CNetworkPacket* packet = GetPacket ();
if (!packet)
	return 0;

memcpy (data, packet->data, packet->Size ());
memcpy (&networkData.packetSource, &packet->owner.address, sizeof (networkData.packetSource));
int size = packet->Size ();
delete packet;
return size;
}

//------------------------------------------------------------------------------

int CNetworkThread::ProcessPackets (void)
{
	int nProcessed = 0;
	CNetworkPacket* packet;

#if DBG
if (LOCALPLAYER.connected == CONNECT_WAITING)
	BRP;
#endif
while (packet = GetPacket ()) {
	if (NetworkProcessPacket (packet->data, packet->Size ()))
		++nProcessed;
	delete packet;
	}
return nProcessed;
}

//------------------------------------------------------------------------------

int CNetworkThread::InitSync (void)
{
if (!Available ())
	return 0;
m_txPacketQueue.Flush ();
return m_syncLock != NULL;
}

//------------------------------------------------------------------------------
// (Re-) Start sending sync packets with packet # nPacket

bool CNetworkThread::StartSync (int nPacket)
{
if (!Available ())
	return 0;
m_txPacketQueue.Start ();
m_txPacketQueue.Lock ();
m_bSendSync = m_txPacketQueue.Current () != NULL;
m_txPacketQueue.Unlock ();
return m_bSendSync;
}

//------------------------------------------------------------------------------

void CNetworkThread::SendSync (void)
{
	static CTimeout toSync (SYNC_TIMEOUT);

if (!SyncInProgress ())
	return;
if (!toSync.Expired ())
	return;

m_txPacketQueue.Lock ();
if (!m_txPacketQueue.Current ()) 
	m_bSendSync = false;
else {
	m_txPacketQueue.Current ()->Send ();
	m_txPacketQueue.Next ();
	}
m_txPacketQueue.Unlock ();
}

//------------------------------------------------------------------------------

void CNetworkThread::StopSync (void)
{
m_txPacketQueue.Lock ();
m_bSendSync = false;
m_txPacketQueue.Unlock ();
m_txPacketQueue.Flush ();
}

//------------------------------------------------------------------------------

bool CNetworkThread::AddSyncPacket (ubyte* data, int size, ubyte* network, ubyte* node)
{
if (!Available ())
	return false;
CNetworkPacket* packet = m_txPacketQueue.Append ();
if (!packet)
	return false;
packet->SetData (data, size);
packet->owner.address.SetNetwork (network);
packet->owner.address.SetNode (node);
return true;
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
Lock ();
int nTimedOut = 0;
//if ((networkData.xLastTimeoutCheck > I2X (1)) && !gameData.reactor.bDestroyed) 
static CTimeout to (UPDATE_TIMEOUT);
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
Unlock ();
return nTimedOut;
}

//------------------------------------------------------------------------------

