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
#include "network.h"
#include "network_lib.h"
#include "autodl.h"
#include "tracker.h"
#include "text.h"

#ifndef _WIN32
#	include "linux/include/ipx_udp.h"
#	include "linux/include/ipx_drv.h"
#endif

#if DBG
#	define LISTEN_TIMEOUT	5
#	define SEND_TIMEOUT		20
#	define UPDATE_TIMEOUT	500
#	define MAX_PACKET_AGE	300000
#	define MAX_CLIENT_AGE	30000
#else
#	define LISTEN_TIMEOUT	5
#	define SEND_TIMEOUT		5
#	define UPDATE_TIMEOUT	500
#	define MAX_PACKET_AGE	3000
#	define MAX_CLIENT_AGE	30000
#endif

#if DBG
#	define PPS		(DEFAULT_PPS / 2)
#else
#	define PPS		netGameInfo.GetPacketsPerSec ()
#endif

#if DBG
#	define DBG_LOCKS		1
#else
#	define DBG_LOCKS		0
#endif

#define SENDLOCK 1
#define RECVLOCK 1
#define PROCLOCK 0

#define MULTI_THREADED_NETWORKING 1 // set to 0 to have D2X-XL manage network traffic the old way

//------------------------------------------------------------------------------

CNetworkThread networkThread;

#if DBG
int MultiCheckPData (uint8_t* pd);
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Certain packets must be sent w/o a leading frame counter (e.g. tracker or XML game info)

bool CNetworkPacket::HasId (void)
{
CNetworkAddress address = m_owner.GetAddress ();
if (tracker.IsTracker (address.m_address.node.portAddress.ip.a, address.m_address.node.portAddress.port.p, reinterpret_cast<char*>(m_data.buffer)))
	return false;
if (Type () == PID_XML_GAMEINFO)
	return false;
return true;
}

//------------------------------------------------------------------------------
// Certain packets must be sent w/o a type tag (e.g. XML game info)

int32_t CNetworkPacket::DataOffset (void)
{
if (Type () == PID_XML_GAMEINFO)
	return 1;
return 0;
}

//------------------------------------------------------------------------------

void CNetworkPacket::Transmit (void)
{
#if DBG
MultiCheckPData (Buffer ());
#endif
if (HasId ()) {
	if (m_owner.m_bHaveLocalAddress)
		IPXSendPacketData (reinterpret_cast<uint8_t*>(&m_data), m_size + sizeof (m_data.nId), m_owner.Network (), m_owner.Node (), m_owner.LocalNode ());
	else
		IPXSendInternetPacketData (reinterpret_cast<uint8_t*>(&m_data), m_size + sizeof (m_data.nId), m_owner.Network (), m_owner.Node ());
	}
else {
	int32_t offset = DataOffset ();
	if (m_owner.m_bHaveLocalAddress)
		IPXSendPacketData (Buffer (offset), m_size - offset, m_owner.Network (), m_owner.Node (), m_owner.LocalNode ());
	else
		IPXSendInternetPacketData (Buffer (offset), m_size - offset, m_owner.Network (), m_owner.Node ());
	}
}

//------------------------------------------------------------------------------

bool CNetworkPacket::Combineable (uint8_t type)
{
#if DBG
return false;
#else
return (type != PID_GAME_INFO) && (type != PID_EXTRA_GAMEINFO) && (type != PID_PLAYERSINFO) && (type != PID_PDATA);
#endif
}

//------------------------------------------------------------------------------

bool CNetworkPacket::Combine (uint8_t* data, int32_t size, uint8_t* network, uint8_t* node)
{
if (Size () + size > MAX_PAYLOAD_SIZE) 
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
{
Create ();
}

//------------------------------------------------------------------------------

CNetworkPacketQueue::~CNetworkPacketQueue ()
{
Destroy ();
}

//------------------------------------------------------------------------------

void CNetworkPacketQueue::Destroy (void)
{
Flush ();
Lock (true, __FUNCTION__);
CNetworkPacket* packet;
while ((packet = m_packets [2])) {
	m_packets [2] = m_packets [2]->Next ();
	delete packet;
	}
m_clients.Destroy ();
Unlock (true, __FUNCTION__);

if (m_semaphore) {
	SDL_DestroySemaphore (m_semaphore);
	m_semaphore = NULL;
	}
}

//------------------------------------------------------------------------------

void CNetworkPacketQueue::Create (void)
{
m_packets [0] = 
m_packets [1] = 
m_packets [2] = 
m_current = NULL;
m_nPackets = 0;
m_nDuplicate = 0;
m_nCombined = 0;
m_nLost = 0;
m_nType = LISTEN_QUEUE;
m_clients.Create ();
m_semaphore = SDL_CreateSemaphore (1);
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkPacketQueue::Alloc (bool bLock)
{
Lock (bLock, __FUNCTION__);
CNetworkPacket* packet = m_packets [2];
if (m_packets [2])
	m_packets [2] = m_packets [2]->Next ();
else
	packet = new CNetworkPacket;
Unlock (bLock, __FUNCTION__);
return packet;
}

//------------------------------------------------------------------------------

void CNetworkPacketQueue::Free (CNetworkPacket* packet, bool bLock)
{
Lock (bLock, __FUNCTION__);
packet->m_nextPacket = m_packets [2];
m_packets [2] = packet;
packet->Reset ();
Unlock (bLock, __FUNCTION__);
}

//------------------------------------------------------------------------------

int32_t CNetworkPacketQueue::Lock (bool bLock, char* pszCaller) 
{ 
if (!m_semaphore || !bLock)
	return 0;
#if DBG_LOCKS
if (pszCaller)
	PrintLog (0, "%s trying to lock %s queue\n", pszCaller, m_nType ? "send" : "listen");
#endif
SDL_SemWait (m_semaphore); 
#if DBG_LOCKS
if (pszCaller)
	PrintLog (0, "%s has locked %s queue\n", pszCaller, m_nType ? "send" : "listen");
#endif
return 1;
}

//------------------------------------------------------------------------------

int32_t CNetworkPacketQueue::Unlock (bool bLock, char* pszCaller) 
{ 
if (!m_semaphore || !bLock)
	return 0;
#if DBG_LOCKS
if (pszCaller)
	PrintLog (0, "%s trying to unlock %s queue\n", pszCaller, m_nType ? "send" : "listen");
#endif
SDL_SemPost (m_semaphore); 
#if DBG_LOCKS
if (pszCaller)
	PrintLog (0, "%s has unlocked %s queue\n", pszCaller, m_nType ? "send" : "listen");
#endif
return 1;
}

//------------------------------------------------------------------------------

void CNetworkPacketQueue::UpdateClientList (void)
{
CNetworkPacket* packet = Tail ();
if (!packet)
	return;

CNetworkClientInfo* i = m_clients.Update (packet->Owner ().GetAddress ());
if (!i)
	return;

int32_t nClientId = i->GetPacketId (m_nType) + 1; // last packet id sent or received
if (m_nType == SEND_QUEUE)  // send
	packet->SetId (i->SetPacketId (m_nType, nClientId));
else { // listen
	int32_t nPacketId = abs (packet->GetId ());
	if (nClientId > 0) {
		int32_t nLost = nPacketId - nClientId;
		if (nLost) {
			i->m_nLost += nLost;
			m_nLost += nLost;
			}
		}
	i->SetPacketId (m_nType, nPacketId);
	}
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkPacketQueue::Append (CNetworkPacket* packet, bool bAllowDuplicates, bool bLock)
{
Lock (bLock, __FUNCTION__);
if (!packet) {
	packet = Alloc (false);
	if (!packet) {
		Unlock (bLock, __FUNCTION__);
		return NULL;
		}
	}

if (Tail ()) { // list tail
	if (!bAllowDuplicates && (*Tail () == *packet)) {
		++m_nDuplicate;
		Tail ()->SetTime (SDL_GetTicks ());
		UpdateClientList ();
		Unlock (bLock, __FUNCTION__);
		Free (packet, bLock);
		return Tail ();
		}
	Tail ()->m_nextPacket = packet;
	}
else 
	SetHead (packet); // list head
SetTail (packet);
Tail ()->SetTime (SDL_GetTicks ());
Tail ()->m_nextPacket = NULL;
UpdateClientList ();
++m_nPackets;
Unlock (bLock, __FUNCTION__);
return packet;
}

//------------------------------------------------------------------------------

CNetworkPacket* CNetworkPacketQueue::Pop (bool bDrop, bool bLock)
{
CNetworkPacket* packet;
Lock (bLock, __FUNCTION__);
if (packet = m_packets [0]) {
	if (!(m_packets [0] = packet->Next ()))
		m_packets [1] = NULL;
	--m_nPackets;
	if (bDrop) {
		Free (packet, false);
		packet = NULL;
		}
	}
Unlock (bLock, __FUNCTION__);
return packet;
}

//------------------------------------------------------------------------------

void CNetworkPacketQueue::Flush (void)
{
Lock (true, __FUNCTION__);
while (m_packets [0]) {
	m_packets [1] = m_packets [0];
	m_packets [0] = m_packets [0]->Next ();
	delete m_packets [1];
	}
m_packets [1] = NULL;
m_nPackets = 0;
m_nDuplicate = 0;
m_nCombined = 0;
m_nLost = 0;
Unlock (true, __FUNCTION__);
}

//------------------------------------------------------------------------------

void CNetworkPacketQueue::Update (void)
{
m_clients.Cleanup ();
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
bool bOk = (!Head () == !Tail ()); // both must be either NULL or not NULL
return bOk;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool CNetworkClientList::Create (void) 
{ 
if (!CStack<CNetworkClientInfo>::Create (100))
	return false;
SetGrowth (100);
return true;
}

//------------------------------------------------------------------------------

void CNetworkClientList::Destroy (void) 
{ 
CStack<CNetworkClientInfo>::Destroy ();
}

//------------------------------------------------------------------------------

CNetworkClientInfo* CNetworkClientList::Find (CNetworkAddress& client) 
{
for (uint32_t i = 0; i < ToS (); i++)
	if (*Top () == client)
		return Top ();
return NULL;
}

//------------------------------------------------------------------------------

CNetworkClientInfo* CNetworkClientList::Update (CNetworkAddress& client) 
{
CNetworkClientInfo* i = Find (client);
if (!i) {
	if (!Grow ())
		return NULL;
	i = Top ();
	(CNetworkAddress&) *i = client;
	i->Reset ();
	}
i->m_timestamp = SDL_GetTicks ();
return i;
}

//------------------------------------------------------------------------------

void CNetworkClientList::Cleanup (void) 
{
	uint32_t t = SDL_GetTicks ();
	uint32_t i = 0;

while (i < ToS ()) {
	if (t - Buffer (i)->m_timestamp < MAX_CLIENT_AGE)
		++i;
	else {
		*Buffer (i) = *Top ();
		--m_tos;
		}
	}
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
	: m_thread (NULL), m_semaphore (NULL), m_sendLock (NULL), m_recvLock (NULL), m_processLock (NULL), m_nThreadId (0), m_bUrgent (0), m_bRunning (false)
{
m_packet = NULL;
m_syncPackets = NULL;
m_rxPacketQueue.SetType (LISTEN_QUEUE);
m_txPacketQueue.SetType (SEND_QUEUE);
}

//------------------------------------------------------------------------------

void CNetworkThread::Run (void)
{
m_bRunning = true;
while (m_bRunning) {
	if (LockListen (true)) {
		PrintLog (0, "Listen\n");
		Listen ();
		UnlockListen ();
		}
	if (LockSend (true)) {
		PrintLog (0, "Transmit\n");
		Transmit ();
		UnlockSend ();
		}
	PrintLog (0, "SendLifeSign\n");
	SendLifeSign ();
	CheckPlayerTimeouts ();
	G3_SLEEP (1);
	}
}

//------------------------------------------------------------------------------

void CNetworkThread::Start (void)
{
#if MULTI_THREADED_NETWORKING
if (!m_thread) {
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
	m_rxPacketQueue.Create ();
	m_rxPacketQueue.SetType (LISTEN_QUEUE);
	m_txPacketQueue.Create ();
	m_txPacketQueue.SetType (SEND_QUEUE);
	m_toSend.Setup (1000 / PPS);
	m_toSend.Start ();
	m_bUrgent = false;
	m_thread = SDL_CreateThread (NetworkThreadHandler, &m_nThreadId);
	while (!Running ())
		G3_SLEEP (0);
	}
#endif
}

//------------------------------------------------------------------------------

void CNetworkThread::Stop (void)
{
if (m_thread) {
	m_bRunning = false;
#if 1
	int nStatus;
	SDL_WaitThread (m_thread, &nStatus);
#else
	SDL_KillThread (m_thread);
#endif
	m_rxPacketQueue.Destroy ();
	m_txPacketQueue.Destroy ();
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
m_txPacketQueue.Flush ();
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
m_rxPacketQueue.Update ();
m_txPacketQueue.Update ();

uint32_t t = SDL_GetTicks ();
if (t <= MAX_PACKET_AGE)
	return; // drop packets older than 3 seconds
t -= MAX_PACKET_AGE;
m_rxPacketQueue.Lock (true, __FUNCTION__);
for (m_rxPacketQueue.Start (); m_rxPacketQueue.Current (); m_rxPacketQueue.Step ()) {
	if (m_rxPacketQueue.Current ()->Timestamp () < t) 
		m_rxPacketQueue.Pop (true, false);
	}
m_rxPacketQueue.Unlock (true, __FUNCTION__);
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
m_rxPacketQueue.Lock (true, __FUNCTION__);
for (;;) {
	// pre-allocate packet so IpxGetPacketData can directly fill its buffer and no extra copy operation is necessary
	if (!m_packet) { 
		PrintLog (0, "Listen: Alloc\n");
		m_packet = m_rxPacketQueue.Alloc (false);
		if (!m_packet)
			break;
		}
	int32_t nSize = IpxGetPacketData (reinterpret_cast<uint8_t*>(&m_packet->m_data));
	if (!nSize)
		break;
#if DBG
	if (!m_rxPacketQueue.Validate ()) {
		PrintLog (0, "Listen: Validation error\n");
		FlushPackets ();
		}
#endif
	memcpy (&m_packet->Owner ().m_address, &networkData.packetSource, sizeof (networkData.packetSource));
	// tracker packets come without a packet id prefix
	if (m_packet->HasId ()) 
		m_packet->SetSize (nSize - sizeof (int32_t)); // don't count the packet queue's 32 bit packet id!
	else {
		memcpy (m_packet->Buffer (), reinterpret_cast<uint8_t*>(&m_packet->m_data), m_packet->Size ());
		m_packet->SetSize (nSize); // don't count the packet queue's 32 bit packet id!
		m_packet->SetId (0);
		}
	PrintLog (0, "Listen: Append\n");
	m_rxPacketQueue.Append (m_packet, false, false);
	m_packet = NULL;
	}
m_rxPacketQueue.Unlock (true, __FUNCTION__);
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

CNetworkPacket* CNetworkThread::GetPacket (bool bLock)
{
return m_rxPacketQueue.Pop (false, bLock);
}

//------------------------------------------------------------------------------

int32_t CNetworkThread::GetPacketData (uint8_t* data)
{
if (!Available ())
	return IpxGetPacketData (data);

CNetworkPacket* packet = GetPacket ();
if (!packet)
	return 0;

memcpy (data, packet->Buffer (), packet->Size ());
memcpy (&networkData.packetSource, &packet->Owner ().m_address, sizeof (networkData.packetSource));
int32_t size = packet->Size ();
m_rxPacketQueue.Free (packet);
return size;
}

//------------------------------------------------------------------------------

int32_t CNetworkThread::ProcessPackets (void)
{
	int32_t nProcessed = 0;

// grab the entire list of packets available for processing and then unlock again 
// to avoid locking Listen() any longer than necessary
m_rxPacketQueue.Lock (true, __FUNCTION__);
CNetworkPacket* head = m_rxPacketQueue.Head ();
if (!head) {
	m_rxPacketQueue.Unlock (true, __FUNCTION__);
	return 0;
	}
m_rxPacketQueue.SetHead (NULL);
m_rxPacketQueue.SetTail (NULL);
m_rxPacketQueue.Unlock (true, __FUNCTION__);

CNetworkPacket* packet;

for (packet = head; packet; packet = packet->Next ()) {
	networkData.packetSource = packet->Owner ().m_address;
	if (NetworkProcessPacket (packet->Buffer (), packet->Size ()))
		++nProcessed;
	}
m_rxPacketQueue.Lock (true, __FUNCTION__);
while (head) {
	packet = head;
	head = head->Next ();
	m_rxPacketQueue.Free (packet, false);
	}
m_rxPacketQueue.Unlock (true, __FUNCTION__);
return nProcessed;
}

//------------------------------------------------------------------------------
// Remove any sync packets still in the transmission queue

void CNetworkThread::AbortSync (void)
{
if (!Available ())
	return;
m_txPacketQueue.Lock (true, __FUNCTION__);
CNetworkPacket* packet;
m_txPacketQueue.Start (); 
while ((packet = m_txPacketQueue.Current ())) {
	m_txPacketQueue.Step ();
	if (packet->Type () == PID_OBJECT_DATA)
		m_txPacketQueue.Pop (true, false);
	}
m_txPacketQueue.Unlock (true, __FUNCTION__);
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
if (!m_toSend.Expired () && !m_txPacketQueue.Head ()->IsUrgent ())
#endif
	return 1;

int32_t nSize = 0;
CNetworkPacket* packet;

m_txPacketQueue.Lock (true, __FUNCTION__);
#if DBG
	{
	PrintLog (0, "Transmit: Fetch packet\n");
	packet = m_txPacketQueue.Head ();
#else
while ((packet = m_txPacketQueue.Head ()) && (packet->IsUrgent () || (nSize + packet->Size () <= MAX_PAYLOAD_SIZE))) {
#endif
	nSize += packet->Size ();
	PrintLog (0, "Transmit: Send packet\n");
	packet->Transmit ();
	PrintLog (0, "Transmit: Remove packet\n");
	m_txPacketQueue.Pop (true, false);
	}
m_txPacketQueue.Unlock (true, __FUNCTION__);
return 1;
}

//------------------------------------------------------------------------------

bool CNetworkThread::SyncInProgress (void)
{
m_txPacketQueue.Lock (true, __FUNCTION__);
CNetworkPacket* packet = m_txPacketQueue.Head ();
m_txPacketQueue.Unlock (true, __FUNCTION__);
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

LockSend ();
m_txPacketQueue.Lock (true, __FUNCTION__);
// try to combine data sent to the same player
if ((packet = m_txPacketQueue.Head ()) && packet->Combine (data, size, network, node))
	++m_txPacketQueue.m_nCombined;
else {
	packet = m_txPacketQueue.Alloc (false);
	if (!packet) {
		m_txPacketQueue.Unlock (true, __FUNCTION__);
		return false;
		}
	packet->SetData (data, size);
	packet->Owner ().SetAddress (network, node);
	packet->Owner ().SetLocalAddress (localAddress);
	packet = m_txPacketQueue.Append (packet, false, false);
	}

packet->SetUrgent (m_bUrgent);
packet->SetImportant (m_bImportant);
packet->SetTime (SDL_GetTicks ());
m_txPacketQueue.Unlock (true, __FUNCTION__);
UnlockSend ();
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

void CNetworkThread::SendLifeSign (bool bForce)
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
			else if (bDownloading || bForce) {
				//pingStats [nPlayer].launchTime = -1;
				//NetworkSendPing (nPlayer);
				}	
			}
		//Unlock ();
		}
	}
}

//------------------------------------------------------------------------------

