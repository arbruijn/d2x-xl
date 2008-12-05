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

#include "inferno.h"
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

static int bTestTracker = 0;
#if 0
static tUdpAddress testServer;
#endif

typedef struct tServerListTable {
	struct tServerListTable	*nextList;
	tServerList					serverList;
	tUdpAddress					*tracker;
	time_t						lastActive;
} tServerListTable;

static tUdpAddress	d2xTracker = {85,119,152,28,0,0};
static tUdpAddress	kbTracker = {207,210,100,66,0,0};

tServerListTable *serverListTable = NULL;
tServerList trackerList;

#if DBG
#	define	S_TIMEOUT	1000
#	define	R_TIMEOUT	1000
#else
#	define	S_TIMEOUT	20000
#	define	R_TIMEOUT	3000
#endif

//------------------------------------------------------------------------------

int FindTracker (tUdpAddress *addr)
{
	int				i;
	unsigned	int	a = UDP_ADDR (addr);
	short				p = (short) ntohs (UDP_PORT (addr));

for (i = 0; i < trackerList.nServers; i++)
	if ((a == UDP_ADDR (trackerList.servers + i)) && 
		 (p == UDP_PORT (trackerList.servers + i)))
		return i;
return -1;
}

//------------------------------------------------------------------------------

static void CallTracker (int i, ubyte *pData, int nDataLen)
{
	uint	network = 0;
	tUdpAddress		tracker;

UDP_ADDR (&tracker) = UDP_ADDR (trackerList.servers + i);
UDP_PORT (&tracker) = htons (UDP_PORT (trackerList.servers + i));
gameStates.multi.bTrackerCall = 1;
IPXSendInternetPacketData (pData, nDataLen, reinterpret_cast<ubyte*> (&network, reinterpret_cast<ubyte*> (&tracker));
gameStates.multi.bTrackerCall = 0;
}

//------------------------------------------------------------------------------

int AddServerToTracker (void)
{
if ((gameStates.multi.bServer || bTestTracker) && gameStates.multi.bUseTracker) {
		int					i, t;
		static int			nTimeout = 0;
		ubyte					id = 'S';

	if ((t = SDL_GetTicks ()) - nTimeout < S_TIMEOUT)
		return 0;
	nTimeout = t;
	for (i = 0; i < trackerList.nServers; i++)
		CallTracker (i, &id, sizeof (id));
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

int RequestServerListFromTracker (void)
{
	int				i, t;
	static int		nTimeout = 0;
	ubyte				id = 'R';

if (!gameStates.multi.bUseTracker)
	return 0;
if (bTestTracker)
	AddServerToTracker ();
if ((t = SDL_GetTicks ()) - nTimeout < R_TIMEOUT)
	return 0;
nTimeout = t;
for (i = 0; i < trackerList.nServers; i++)
	CallTracker (i, &id, sizeof (id));
return 1;
}

//------------------------------------------------------------------------------

int ReceiveServerListFromTracker (ubyte *data)
{
	tServerListTable	*pslt;
	int					i = FindTracker (reinterpret_cast<tUdpAddress*> (&ipx_udpSrc.src_node));

if (i < 0)
	return 0;
for (pslt = serverListTable; pslt; pslt = pslt->nextList)
	if (--i < 0) {
			tUdpAddress	*ps = pslt->serverList.servers;

		memcpy (&pslt->serverList, data, sizeof (tServerList));
		for (i = pslt->serverList.nServers; i; i--, ps++)
			UDP_PORT (ps) = (ushort) ntohs (UDP_PORT (ps));
		pslt->lastActive = SDL_GetTicks ();
		return 1;
		}
return 0;
}

//------------------------------------------------------------------------------

void SetServerFromList (tServerList *psl, int i)
{
memcpy (ipx_ServerAddress + 4, psl->servers + i, 4);
*reinterpret_cast<ushort*> (ipx_ServerAddress + 8) = (ushort) htons (UDP_PORT (psl->servers + i));
//udpBasePort = psl->servers [i].port;
}

//------------------------------------------------------------------------------

int GetServerFromList (int i)
{
	tServerListTable	*pslt = serverListTable;

while (pslt) {
	if (i < pslt->serverList.nServers) {
		SetServerFromList (&pslt->serverList, i);
		return 1;
		}
	i -= pslt->serverList.nServers;
	pslt = pslt->nextList;
	}
return 0;
}

//------------------------------------------------------------------------------

int IsTracker (uint addr, ushort port)
{
	int	i;
#if DBG
	uint a;
	ushort p;
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

int AddTracker (tUdpAddress *addr)
{
	int					i;
	tServerListTable	*pslt;

if (trackerList.nServers >= MAX_TRACKER_SERVERS)
	return -1;
if (0 < (i = FindTracker (addr)))
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

inline int CountActiveTrackers (void)
{
	tServerListTable	*pslt;
	time_t t = SDL_GetTicks ();
	int i = 0;

for (pslt = serverListTable; pslt; pslt = pslt->nextList)
	if (pslt->lastActive && (t - pslt->lastActive < 30000))
		i++;
return i;
}

//------------------------------------------------------------------------------

static time_t	nQueryTimeout;

int TrackerPoll (int nItems, tMenuItem *m, int *key, int nCurItem)
{
	time_t t;

if (NetworkListen () && CountActiveTrackers ())
	*key = -2;
else if (*key == KEY_ESC)
	*key = -3;
else if ((t = SDL_GetTicks () - nQueryTimeout) > 60000)
	*key = -4;
else {
	int v = (int) (t / 60);
	if (m [0].value != v) {
		m [0].value = v;
		m [0].rebuild = 1;
		}
	*key = 0;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int QueryTrackers (void)
{
	tMenuItem	m [3];
	int i;

if (!RequestServerListFromTracker ())
	return 0;
memset (m, 0, sizeof (m));
ADD_GAUGE (0, "                    ", 0, 1000); 
ADD_TEXT (1, "", 0);
ADD_TEXT (2, "(Press Escape to cancel)", 0);
m [2].centered = 1;
nQueryTimeout = SDL_GetTicks ();
do {
	i = ExecMenu2 (NULL, "Looking for Trackers", 3, m, TrackerPoll, 0, NULL);
	} while (i >= 0);
return i;
}

//------------------------------------------------------------------------------

int ActiveTrackerCount (int bQueryTrackers)
{
if (bQueryTrackers)
	return QueryTrackers ();
return CountActiveTrackers ();
}

//------------------------------------------------------------------------------

extern int stoip (char *szServerIpAddr, ubyte *pIpAddr);
int stoport (char *szPort, int *pPort, int *pSign);

static int ParseIpAndPort (char *pszAddr, tUdpAddress *addr)
{
	int	port;
	char	szAddr [22], *pszPort;

strncpy (szAddr, pszAddr, sizeof (szAddr));
szAddr [21] = '\0';
if (!(pszPort = strchr (szAddr, ':')))
	return 0;
*pszPort++ = '\0';
if (!stoip (szAddr, reinterpret_cast<ubyte*> (addr)) 
	return 0;
if (!stoport (pszPort, &port, NULL))
	return 0;
if (port > 0xFFFF)
	return 0;
UDP_PORT (addr) = port; 
return 1;
}

//------------------------------------------------------------------------------

void AddTrackersFromCmdLine (void)
{
	uint	i, j, t;
	char			szKwd [20];
	tUdpAddress	tracker;

if ((t = FindArg ("-test_tracker")))
	bTestTracker = 1;
if (!(t = FindArg ("-num_trackers")))
	return;
if (!(i = atoi (pszArgList [t + 1])))
	return;
if (i > MAX_TRACKER_SERVERS)
	i = MAX_TRACKER_SERVERS;
for (j = 0; j < i; j++) {
	sprintf (szKwd, "-tracker%d", j + 1);
	if (!(t = FindArg (szKwd)))
		continue;
	if (ParseIpAndPort (pszArgList [t + 1], &tracker))
		AddTracker (&tracker);
	}
}

//------------------------------------------------------------------------------

void ResetTrackerList (void)
{
serverListTable = NULL;
gameStates.multi.bTrackerCall = 0;
memset (&trackerList, 0, sizeof (trackerList));
}

//------------------------------------------------------------------------------

void CreateTrackerList (void)
{
	int	a;

ResetTrackerList ();
if (!(a = FindArg ("-internal_tracker")) || atoi (pszArgList [a + 1])) {
	AddTracker (&d2xTracker);
	AddTracker (&kbTracker);
	}
AddTrackersFromCmdLine ();
}

//------------------------------------------------------------------------------

void DestroyTrackerList (void)
{
	tServerListTable	*pslt;

while (serverListTable) {
	pslt = serverListTable;
	serverListTable = serverListTable->nextList;
	delete pslt;
	pslt = NULL;
	}
ResetTrackerList ();
}

//------------------------------------------------------------------------------

// eof
