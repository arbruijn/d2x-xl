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

#if DBG
#	define LISTEN_TIMEOUT	5
#	define SEND_TIMEOUT		20
#	define UPDATE_TIMEOUT	500
#	define MAX_PACKET_AGE	300000
#else
#	define LISTEN_TIMEOUT	5
#	define SEND_TIMEOUT		5
#	define UPDATE_TIMEOUT	500
#	define MAX_PACKET_AGE	3000
#endif

#if DBG
#	define PPS		(DEFAULT_PPS / 2)
#else
#	define PPS		netGame.GetPacketsPerSec ()
#endif

#define SENDLOCK 1
#define RECVLOCK 1
#define PROCLOCK 0

#define MULTI_THREADED_NETWORKING 1 // set to 0 to have D2X-XL manage network traffic the old way

//------------------------------------------------------------------------------

CNetworkThread networkThread;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CNetworkPacket::Transmit (void)
{
if (m_owner.m_bHaveLocalAddress)
	IPXSendPacketData (m_data, m_size, m_owner.Network (), m_owner.Node (), m_owner.LocalNode ());
else
	IPXSendInternetPacketData (m_data, m_size, m_owner.Network (), m_owner.Node ());
}

//------------------------------------------------------------------------------

bool CNetworkPacket::Combineable (uint8_t type)
{
return (type != PID_GAME_INFO) && (type != PID_EXTRA_GAMEINFO) && (type != PID_PLAYERSINFO);
}

//------------------------------------------------------------------------------

bool CNetworkPacket::Combine (uint8_t* data, int32_t size, uint8_t* network, uint8_t* node)
{
if (Size () + size > MAX_PACKET_SIZE) 
	return false; // too large
if (!Combineable (Type ()) || !Combineable (data [0]))
	return false; // at least one of the packets contains data that must not be combined with other data
if (Owner ().CmpAddress (network, node)) 
	return false; // different receivers
SetData (data, size, Size ());
data [0] |= 0x80; // mark the packet as containing combined data
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CNetworkPacketQueue::CNetworkPacketQueue ()
	: m_nPackets (0)
{
m_packets [0] = 
m_packets [1] = 
m_packets [2] = 
m_current = NULL;
m_nDuplicate = 0;
m_nCombined = 0;
m_semaphore = SDL_CreateSemaphore (1);
}

//------------------------------------------------------------------------------

CNetworkPacketQueue::~CNetworkPacketQueue ()
{
Flush ();

Lock ();
CNetworkPacket* packet;
while ((packet = m_packets [2])) {
	m_packets [2] = m_packets [2]->Next ();
	delete packet;
	}
Unlock ();

if (m_semaphore) {
	SDL_DestroySemaphore (m_semaphore);
	m_semaphore = NULL;
	}
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkPacketQueue::Alloc (bool bLock)
{
Lock (bLock);
CNetworkPacket* packet = m_packets [2];
if (m_packets [2])
	m_packets [2] = m_packets [2]->Next ();
else
	packet = new CNetworkPacket;
Unlock (bLock);
return packet;
}

//------------------------------------------------------------------------------

void CNetworkPacketQueue::Free (CNetworkPacket* packet, bool bLock)
{
Lock (bLock);
packet->m_nextPacket = m_packets [2];
m_packets [2] = packet;
packet->SetSize (0);
Unlock (bLock);
}

//------------------------------------------------------------------------------

int32_t CNetworkPacketQueue::Lock (bool bLock) 
{ 
if (!m_semaphore || !bLock)
	return 0;
SDL_SemWait (m_semaphore); 
return 1;
}

//------------------------------------------------------------------------------

int32_t CNetworkPacketQueue::Unlock (bool bLock) 
{ 
if (!m_semaphore || !bLock)
	return 0;
SDL_SemPost (m_semaphore); 
return 1;
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkPacketQueue::Append (CNetworkPacket* packet, bool bAllowDuplicates)
{
Lock ();
if (!packet) {
	packet = Alloc (false);
	if (!packet) {
		Unlock ();
		return NULL;
		}
	}

if (Tail ()) { // list tail
	if (!bAllowDuplicates && (*Tail () == *packet)) {
		++m_nDuplicate;
		Tail ()->SetTime (SDL_GetTicks ());
		Unlock ();
		Free (packet);
		return Tail ();
		}
	Tail ()->m_nextPacket = packet;
	}
else 
	SetHead (packet); // list head
SetTail (packet);
Tail ()->SetTime (SDL_GetTicks ());
Tail ()->m_nextPacket = NULL;
++m_nPackets;
Unlock ();
return packet;
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkPacketQueue::Pop (bool bDrop, bool bLock)
{
CNetworkPacket* packet;
Lock (bLock);
if (packet = m_packets [0]) {
	if (!(m_packets [0] = packet->Next ()))
		m_packets [1] = NULL;
	--m_nPackets;
	if (bDrop) {
		Free (packet, false);
		packet = NULL;
		}
	}
Unlock (bLock);
return packet;
}

//------------------------------------------------------------------------------

void CNetworkPacketQueue::Flush (void)
{
Lock ();
while (m_packets [0]) {
	m_packets [1] = m_packets [0];
	m_packets [0] = m_packets [0]->Next ();
	delete m_packets [1];
	}
m_packets [1] = NULL;
m_nPackets = 0;
m_nDuplicate = 0;
m_nCombined = 0;
Unlock ();
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkPacketQueue::Start (int32_t nPacket) 
{ 
for (m_current = Head (); m_current && nPacket--; Step ())
	;
return m_current;
}

//------------------------------------------------------------------------------

bool CNetworkPacketQueue::Validate (void) 
{ 
Lock ();
bool bOk = (!Head () == !Tail ()); // both must be either NULL or not NULL
Unlock ();
return bOk;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static int32_t _CDECL_ NetworkThreadHandler (void* nThreadP)
{
networkThread.Run ();
return 0;
}

//------------------------------------------------------------------------------

CNetworkThread::CNetworkThread () 
	: m_thread (NULL), m_semaphore (NULL), m_sendLock (NULL), m_recvLock (NULL), m_processLock (NULL), m_nThreadId (0), m_bUrgent (0)
{
m_packet = NULL;
m_syncPackets = NULL;
}

//------------------------------------------------------------------------------

void CNetworkThread::Run (void)
{
for (;;) {
	if (LockRecv (true)) {
		Listen ();
		UnlockRecv ();
		}
	if (LockSend (true)) {
		Transmit ();
		UnlockSend ();
		}
	Update ();
	CheckPlayerTimeouts ();
	G3_SLEEP (1);
	}
}

//------------------------------------------------------------------------------

void CNetworkThread::Start (void)
{
#if MULTI_THREADED_NETWORKING
if (!m_thread) {
	m_thread = SDL_CreateThread (NetworkThreadHandler, &m_nThreadId);
	m_semaphore = SDL_CreateSemaphore (1);
#if SENDLOCK
	m_sendLock = SDL_CreateSemaphore (1);
#endif
#if RECVLOCK
	m_recvLock = SDL_CreateSemaphore (1);
#endif
#if PROCLOCK
	m_processLock = SDL_CreateSemaphore (1);
#endif
	m_toSend.Setup (1000 / PPS);
	m_toSend.Start ();
	m_bUrgent = false;
	}
#endif
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
		SDL_DestroySemaphore (m_sendLock);
		m_sendLock = NULL;
		}
	if (m_recvLock) {
		SDL_DestroySemaphore (m_recvLock);
		m_recvLock = NULL;
		}
	if (m_processLock) {
		SDL_DestroySemaphore (m_processLock);
		m_processLock = NULL;
		}
	if (m_packet) {
		delete m_packet;
		m_packet = NULL;
		}
	FlushPackets ();
	m_thread = NULL;
	m_bUrgent = false;
	}
}

//------------------------------------------------------------------------------

void CNetworkThread::FlushPackets (void)
{
m_rxPacketQueue.Flush ();
}

//------------------------------------------------------------------------------

int32_t CNetworkThread::Lock (SDL_sem* semaphore, bool bTry) 
{ 
if (!semaphore)
	return 0;
if (bTry)
	return SDL_SemTryWait (semaphore) != 0;
SDL_SemWait (semaphore); 
return 1;
}

//------------------------------------------------------------------------------

int32_t CNetworkThread::Unlock (SDL_sem* semaphore) 
{ 
if (!semaphore)
	return 0;
SDL_SemPost (semaphore); 
return 1;
}

//------------------------------------------------------------------------------

void CNetworkThread::Cleanup (void)
{
uint32_t t = SDL_GetTicks ();
if (t <= MAX_PACKET_AGE)
	return; // drop packets older than 3 seconds
t -= MAX_PACKET_AGE;
m_rxPacketQueue.Lock ();
for (m_rxPacketQueue.Start (); m_rxPacketQueue.Current (); m_rxPacketQueue.Step ()) {
	if (m_rxPacketQueue.Current ()->Timestamp () < t) 
		m_rxPacketQueue.Pop (true, false);
	}
m_rxPacketQueue.Unlock ();
}

//------------------------------------------------------------------------------

int32_t CNetworkThread::Listen (void)
{
#if 0
	static CTimeout toListen (LISTEN_TIMEOUT);

if (!toListen.Expired ())
	return 0;
#endif
#if 1 // network reads all network packets independently of main thread
Cleanup ();
// read all available network packets and append them to the end of the list of unprocessed network packets
for (;;) {
	// pre-allocate packet so IpxGetPacketData can directly fill its buffer and no extra copy operation is necessary
	if (!m_packet) { 
		m_packet = m_rxPacketQueue.Alloc ();
		if (!m_packet)
			break;
		}
	if (!m_packet->SetSize (IpxGetPacketData (m_packet->m_data)))
		break;
#if DBG
	if (!m_rxPacketQueue.Validate ())
		FlushPackets ();
#endif
	memcpy (&m_packet->Owner ().m_address, &networkData.packetSource, sizeof (networkData.packetSource));
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

int32_t CNetworkThread::GetPacketData (uint8_t* data)
{
if (!Available ())
	return IpxGetPacketData (data);

CNetworkPacket* packet = GetPacket ();
if (!packet)
	return 0;

memcpy (data, packet->m_data, packet->Size ());
memcpy (&networkData.packetSource, &packet->Owner ().m_address, sizeof (networkData.packetSource));
int32_t size = packet->Size ();
m_rxPacketQueue.Free (packet);
return size;
}

//------------------------------------------------------------------------------

int32_t CNetworkThread::ProcessPackets (void)
{
	int32_t nProcessed = 0;
	CNetworkPacket* packet;

#if DBG
if (LOCALPLAYER.connected == CONNECT_WAITING)
	BRP;
#endif
while (packet = GetPacket ()) {
	if (NetworkProcessPacket (packet->m_data, packet->Size ()))
		++nProcessed;
	m_rxPacketQueue.Free (packet);
	}
return nProcessed;
}

//------------------------------------------------------------------------------
// Remove any sync packets still in the transmission queue

void CNetworkThread::AbortSync (void)
{
if (!Available ())
	return;
m_txPacketQueue.Lock ();
CNetworkPacket* packet;
m_txPacketQueue.Start (); 
while ((packet = m_txPacketQueue.Current ())) {
	m_txPacketQueue.Step ();
	if (packet->Type () == PID_OBJECT_DATA)
		m_txPacketQueue.Pop (true, false);
	}
m_txPacketQueue.Unlock ();
}

//------------------------------------------------------------------------------

int32_t CNetworkThread::Transmit (bool bForce)
{
if (m_toSend.Duration () != 1000 / PPS) {
	m_toSend.Setup (1000 / PPS);
	m_toSend.Start ();
	}

if (m_txPacketQueue.Empty ())
	return 0;
#if DBG
if (!bForce && !m_toSend.Expired ())
#else
if (!m_toSend.Expired () && !m_txPacketQueue.Head ()->Urgent ())
#endif
	return 1;

int32_t nSize = 0;
CNetworkPacket* packet;

m_txPacketQueue.Lock ();
#if DBG
	{
	packet = m_txPacketQueue.Head ();
#else
while ((packet = m_txPacketQueue.Head ()) && (packet->Urgent () || (nSize + packet->Size () <= MAX_PACKET_SIZE))) {
#endif
	nSize += packet->Size ();
	packet->Transmit ();
	m_txPacketQueue.Pop (true, false);
	}
m_txPacketQueue.Unlock ();
return 1;
}

//------------------------------------------------------------------------------

bool CNetworkThread::SyncInProgress (void)
{
m_txPacketQueue.Lock ();
CNetworkPacket* packet = m_txPacketQueue.Head ();
m_txPacketQueue.Unlock ();
return (packet && packet->Type () == PID_OBJECT_DATA);
}

//------------------------------------------------------------------------------

bool CNetworkThread::Send (uint8_t* data, int32_t size, uint8_t* network, uint8_t* node, uint8_t* localAddress)
{
if (!Available ()) {
	if (localAddress)
		IPXSendPacketData (data, size, network, node, localAddress);
	else
		IPXSendInternetPacketData (data, size, network, node);
	return true;
	}

CNetworkPacket* packet;

m_txPacketQueue.Lock ();
// try to combine data sent to the same player
if ((packet = m_txPacketQueue.Head ()) && packet->Combine (data, size, network, node)) {
	++m_txPacketQueue.m_nCombined;
	m_txPacketQueue.Unlock ();
	}
else {
	packet = m_txPacketQueue.Alloc (false);
	m_txPacketQueue.Unlock ();
	if (!packet)
		return false;
	packet->SetData (data, size);
	packet->Owner ().SetAddress (network, node);
	packet->Owner ().SetLocalAddress (localAddress);
	packet = m_txPacketQueue.Append (packet, false);
	}

packet->SetUrgent (m_bUrgent);
packet->SetTime (SDL_GetTicks ());
m_bUrgent = false;
return true;
}

//------------------------------------------------------------------------------

int32_t CNetworkThread::ConnectionStatus (int32_t nPlayer)
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

int32_t CNetworkThread::CheckPlayerTimeouts (void)
{
LockThread ();
int32_t nTimedOut = 0;
//if ((networkData.xLastTimeoutCheck > I2X (1)) && !gameData.reactor.bDestroyed) 
static CTimeout to (UPDATE_TIMEOUT);
int32_t s = -1;
if (to.Expired () /*&& !gameData.reactor.bDestroyed*/) 
	{
	int32_t t = SDL_GetTicks ();
	for (int32_t nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++) {
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
UnlockThread ();
return nTimedOut;
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
		for (int32_t nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++) {
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
}

//------------------------------------------------------------------------------

