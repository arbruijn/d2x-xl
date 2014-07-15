#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#ifndef _WIN32
#	include <arpa/inet.h>
#	include <netinet/in.h> /* for htons & co. */
#endif

#include "descent.h"
#include "timer.h"
#include "byteswap.h"
#include "strutil.h"
#include "ipx.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

extern int32_t MultiMsgLen (uint8_t nMsg);

// CMessage instances without a message are used to keep track of incoming packets
// the sender has 

void CMessage::Setup (uint32_t id, uint8_t* message) 
{
m_timestamp = SDL_GetTicks ();
m_players = 0;
if (message) {
	m_id = (id & 0x0FFFFFFF) | (uint32_t (N_LOCALPLAYER) << 28);
	m_resendTime = m_timestamp;
	for (int32_t i = 0; i < gameData.multiplayer.nPlayers; i++)
		if ((i != N_LOCALPLAYER) && (gameData.multiplayer.players [i].IsConnected ()))
			m_players |= (1 << i);
	m_message [0] = PID_RESEND_MESSAGE;
	memcpy (m_message + 1, message, MultiMsgLen (message [0]));
	}
else {
	m_id = id;
	m_message [0] = 0;
	}
}

//------------------------------------------------------------------------------

uint16_t CMessage::Acknowledge (uint8_t nPlayer)
{
m_players &= ~(1 << nPlayer);
return m_players;
}

//------------------------------------------------------------------------------

int32_t CMessage::Update (void)
{
	int32_t t;

if (!m_players)
	m_id = 0;
else if ((t = SDL_GetTicks ()) - m_timestamp < extraGameInfo [0].timeout.nKeepMessage) {
	if (t - m_resendTime >= extraGameInfo [0].timeout.nResendMessage) { // resend every 300 ms
		m_resendTime = t;
		if (m_message [0]) {
			for (uint8_t nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++)
				if (m_players & (1 << nPlayer))
					IpxSendPlayerPacket (nPlayer, m_message, MultiMsgLen (m_message [1]) + 1); // regard the additional PID
			}
		}
	}
else {
	m_id = 0;
	if (m_message [0]) {
		if (extraGameInfo [1].bCompetition) {
			for (uint8_t nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++)
				if (m_players & (1 << nPlayer))
					NetworkDisconnectPlayer (nPlayer);
			}
		}
	}
return m_id;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CMessage* CMessageList::Alloc (void)
{
CMessage* msg = FreeList ();

if (msg)
	SetFreeList (msg->GetNext ());
else
	msg = new CMessage;
return msg;
}

//------------------------------------------------------------------------------

void CMessageList::Free (CMessage* msg)
{
msg->SetPrev (NULL);
msg->SetNext (GetFreeList ());
SetFreeList (msg);
}

//------------------------------------------------------------------------------

int32_t CMessageList::Add (uint8_t* message) 
{
CMessage* msg = Alloc ();
if (!msg)
	return 0;
msg->Setup (++m_id, message);
Link (msg);
return m_id;
}

//------------------------------------------------------------------------------

bool CMessageList::Add (uint32_t id) 
{
if (Find (id))
	return true;
CMessage* msg = Alloc ();
if (!msg)
	return false;
msg->Setup (id);
Link (msg);
return true;
}

//------------------------------------------------------------------------------

CMessage* CMessageList::Find (uint32_t id)
{
if (GetSender (id) != N_LOCALPLAYER)
	return NULL;
for (CMessage* i = Head (); i; i = i->GetNext ())
	if (i->m_id == id)
		return i;
return NULL;
}

//------------------------------------------------------------------------------

CMessage* CMessageList::Acknowledge (uint8_t nPlayer, uint32_t id)
{
if (nPlayer == N_LOCALPLAYER)
	return NULL;
CMessage* i = Find (id);
if (i) {
	if (!i->Acknowledge (nPlayer)) {
		Unlink (i);
		Free (i);
		i = NULL;
		}
	}
return i;
}

//------------------------------------------------------------------------------

void CMessageList::Update (void)
{
CMessage* i = Head ();

while (i) {
	if (!i->Update ()) {
		CMessage* j = i;
		i = i->GetPrev ();
		Unlink (j);
		Free (j);
		}
	i = i ? i->GetNext () : Head ();
	}
}

//------------------------------------------------------------------------------

void CMessageList::Destroy (void)
{
CMessage* i = Head ();

while (i) {
	CMessage* j = i;
	i = i->GetNext ();
	Unlink (j);
	delete j;
	}
SetHead (NULL);
SetTail (NULL);

i = FreeList ();
while (i) {
	CMessage* j = i;
	i = i->GetNext ();
	Unlink (j);
	delete j;
	}
SetFreeList (NULL);
}

//------------------------------------------------------------------------------
// importantMessages [0] are the messages that have been sent from this client and are waiting for their reception to be confirmed
// importantMessages [1] are the messages that have been received by this client and need to be confirmed, but should not be processed again if resent by their originator

CMessageList importantMessages [2];

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
