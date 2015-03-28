// tracker.c
// game server tracker communication

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include  <string.h>
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
#include "args.h"
#include "u_mem.h"
#include "ipx.h"
#include "network.h"
#include "network_lib.h"
#include "key.h"
#include "menu.h"
#include "tracker.h"

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#if DBG
static int32_t bTestTracker = 0;
#else
#	define bTestTracker	0
#endif
#if 0
static tUdpAddress testServer;
#endif

static tUdpAddress	d2xTracker = {87,106,88,224,0,0};

tServerListTable *serverListTable = NULL;
tServerList trackerList;

#if DBG
#	define	S_TIMEOUT	1000
#	define	R_TIMEOUT	1000
#else
#	define	S_TIMEOUT	20000
#	define	R_TIMEOUT	3000
#endif

CTracker tracker;

//------------------------------------------------------------------------------

void CTracker::Init (void)
{
memset (&m_list, 0, sizeof (m_list));
m_table = NULL;
m_bUse = true;
}

//------------------------------------------------------------------------------

void CTracker::Destroy (void)
{
DestroyList ();
Init ();
}

//------------------------------------------------------------------------------

int32_t CTracker::Find (tUdpAddress *addr)
{
	int32_t	i;
	uint32_t	a = UDP_ADDR (addr);
	int16_t	p = (int16_t) ntohs (UDP_PORT (addr));

for (i = 0; i < trackerList.nServers; i++)
	if ((a == UDP_ADDR (trackerList.servers + i)) && 
		 (p == UDP_PORT (trackerList.servers + i)))
		return i;
return -1;
}

//------------------------------------------------------------------------------

void CTracker::Call (int32_t i, uint8_t *pData, int32_t nDataLen)
{
	uint32_t	network = 0;
	tUdpAddress		tracker;

UDP_ADDR (&tracker) = UDP_ADDR (trackerList.servers + i);
UDP_PORT (&tracker) = htons (UDP_PORT (trackerList.servers + i));
networkThread.Send (pData, nDataLen, reinterpret_cast<uint8_t*> (&network), NULL, reinterpret_cast<uint8_t*> (&tracker), 1);
}

//------------------------------------------------------------------------------

int32_t CTracker::AddServer (void)
{
if ((IAmGameHost () || bTestTracker) && tracker.m_bUse) {
		int32_t					i, t;
		static int32_t			nTimeout = 0;
		uint8_t					id = 'S';

	if ((t = SDL_GetTicks ()) - nTimeout < S_TIMEOUT)
		return 0;
	nTimeout = t;
	for (i = 0; i < trackerList.nServers; i++)
		Call (i, &id, sizeof (id));
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t CTracker::RequestServerList (void)
{
	int32_t				i, t;
	static int32_t		nTimeout = 0;
	uint8_t				id = 'R';

if (!tracker.m_bUse)
	return 0;
#if DBG
if (bTestTracker)
	AddServer ();
#endif
if ((t = SDL_GetTicks ()) - nTimeout < R_TIMEOUT)
	return 0;
nTimeout = t;
for (i = 0; i < trackerList.nServers; i++)
	Call (i, &id, sizeof (id));
return 1;
}

//------------------------------------------------------------------------------

int32_t CTracker::ReceiveServerList (uint8_t *data)
{
	tServerListTable	*pslt;
	int32_t				i = Find (reinterpret_cast<tUdpAddress*> (networkData.packetSource.Node ()));

if (i < 0)
	return 0;
for (pslt = serverListTable; pslt; pslt = pslt->nextList)
	if (--i < 0) {
			tUdpAddress	*ps = pslt->serverList.servers;

		memcpy (&pslt->serverList, data, sizeof (tServerList));
		for (i = pslt->serverList.nServers; i; i--, ps++)
			UDP_PORT (ps) = (uint16_t) ntohs (UDP_PORT (ps));
		pslt->lastActive = SDL_GetTicks ();
		return 1;
		}
return 0;
}

//------------------------------------------------------------------------------

void CTracker::SetServerFromList (tServerList *psl, int32_t i, uint8_t* serverAddress)
{
memcpy (serverAddress + 4, psl->servers + i, 4);
*reinterpret_cast<uint16_t*> (serverAddress + 8) = (uint16_t) htons (UDP_PORT (psl->servers + i));
//udpBasePort = psl->servers [i].port;
}

//------------------------------------------------------------------------------

int32_t CTracker::GetServerFromList (int32_t i, uint8_t* serverAddress)
{
if (!m_bUse)
	return 0;

	tServerListTable	*pslt = serverListTable;

while (pslt) {
	if (i < pslt->serverList.nServers) {
		SetServerFromList (&pslt->serverList, i, serverAddress);
		return 1;
		}
	i -= pslt->serverList.nServers;
	pslt = pslt->nextList;
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t CTracker::IsTracker (uint32_t addr, uint16_t port, const char* msg)
{
if (msg) { 
	if (!strcmp (msg, "FDescent Game Info Request"))
		return 2;
	 if (!strcmp (msg, "GDescent Game Status Request"))
		return 2;
	}

if (!m_bUse)
	return 0;

	int32_t	i;
#if DBG
	uint32_t a;
	uint16_t p;
#endif

port = ntohs (port);
for (i = 0; i < trackerList.nServers; i++) {
#if DBG
	a = UDP_ADDR (trackerList.servers + i);
	p = UDP_PORT (trackerList.servers + i);
#endif
	if ((addr == UDP_ADDR (trackerList.servers + i)) && 
		 (port == UDP_PORT (trackerList.servers + i)))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t CTracker::Add (tUdpAddress *addr)
{
	int32_t					i;
	tServerListTable	*pslt;

if (trackerList.nServers >= MAX_TRACKER_SERVERS)
	return -1;
if (0 < (i = Find (addr)))
	return i;
if (!(pslt = new tServerListTable))
	return -1;
memset (pslt, 0, sizeof (*pslt));
pslt->nextList = serverListTable;
serverListTable = pslt;
pslt->tracker = trackerList.servers + trackerList.nServers;
UDP_ADDR (pslt->tracker) = UDP_ADDR (addr);
UDP_PORT (pslt->tracker) = UDP_PORT (addr) ? UDP_PORT (addr) : 9424;
pslt->lastActive = 0;
return trackerList.nServers++;
}

//------------------------------------------------------------------------------

int32_t CTracker::CountActive (void)
{
	tServerListTable	*pslt;
	time_t t = SDL_GetTicks ();
	int32_t i = 0;

for (pslt = serverListTable; pslt; pslt = pslt->nextList)
	if (pslt->lastActive && (t - pslt->lastActive < 30000))
		i++;
return i;
}

//------------------------------------------------------------------------------

static time_t	nQueryTimeout;

int32_t TrackerPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	time_t t;

if (NetworkListen () && tracker.CountActive ())
	key = -2;
else if (key == KEY_ESC)
	key = -3;
else if ((t = (SDL_GetTicks () - nQueryTimeout)) > 60000)
	key = -4;
else {
	int32_t v = (int32_t) (t / 60);
	if (menu [0].Value () != v) {
		menu [0].Value () = v;
		menu [0].Rebuild ();
		tracker.RequestServerList ();
		}
	key = 0;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int32_t CTracker::Query (void)
{
	CMenu	menu (3);
	int32_t	i;

NetworkInit ();
if (!RequestServerList ())
	return 0;
menu.AddGauge ("progress bar", "                    ", -1, 1000); 
menu.AddText ("", "", 0);
menu.AddText ("", "(Press Escape to cancel)", 0);
menu.Top ()->m_bCentered = 1;
nQueryTimeout = SDL_GetTicks ();
do {
	i = menu.Menu (NULL, "Looking for Trackers", TrackerPoll);
	} while (i >= 0);
return i;
}

//------------------------------------------------------------------------------

int32_t CTracker::ActiveCount (int32_t bQueryTrackers)
{
if (bQueryTrackers)
	return Query ();
return CountActive ();
}

//------------------------------------------------------------------------------

//int32_t stoip (char *szServerIpAddr, uint8_t *ipAddrP, int32_t* portP = NULL);
//int32_t stoport (char *szPort, int32_t *pPort, int32_t *pSign);

int32_t CTracker::ParseIpAndPort (char *pszAddr, tUdpAddress *addr)
{
	int32_t	port;
	char	szAddr [22], *pszPort;

strncpy (szAddr, pszAddr, sizeof (szAddr));
szAddr [21] = '\0';
if (!(pszPort = strchr (szAddr, ':')))
	return 0;
*pszPort++ = '\0';
if (!stoip (szAddr, reinterpret_cast<uint8_t*> (addr)))
	return 0;
if (!stoport (pszPort, &port, NULL))
	return 0;
if (port > 0xFFFF)
	return 0;
UDP_PORT (addr) = port; 
return 1;
}

//------------------------------------------------------------------------------

void CTracker::AddFromCmdLine (void)
{
	uint32_t	i, j, t;
	char			szKwd [20];
	tUdpAddress	tracker;

#if DBG
if (0 < (t = FindArg ("-test_tracker")))
	bTestTracker = 1;
#endif
if (!(t = FindArg ("-num_trackers")))
	return;
if (!(i = atoi (appConfig [t + 1])))
	return;
if (i > MAX_TRACKER_SERVERS)
	i = MAX_TRACKER_SERVERS;
for (j = 1; j <= i; j++) {
	sprintf (szKwd, "-tracker%d", j);
	if (!(t = FindArg (szKwd)))
		continue;
	if (ParseIpAndPort (appConfig [t + 1], &tracker))
		Add (&tracker);
	}
}

//------------------------------------------------------------------------------

void CTracker::ResetList (void)
{
serverListTable = NULL;
gameStates.multi.bTrackerCall = 0;
memset (&trackerList, 0, sizeof (trackerList));
}

//------------------------------------------------------------------------------

void CTracker::CreateList (void)
{
	int32_t	a;

ResetList ();
if (!(a = FindArg ("-internal_tracker")) || atoi (appConfig [a + 1])) {
	Add (&d2xTracker);
	}
AddFromCmdLine ();
}

//------------------------------------------------------------------------------

void CTracker::DestroyList (void)
{
	tServerListTable	*pslt;

while (serverListTable) {
	pslt = serverListTable;
	serverListTable = serverListTable->nextList;
	delete pslt;
	pslt = NULL;
	}
ResetList ();
}

//------------------------------------------------------------------------------

// eof
