// tracker.h
// game server tracker communication

#ifndef __tracker_h
#define __tracker_h

typedef ubyte tUdpAddress [6];

#define UDP_ADDR(_a)	*(reinterpret_cast<uint *> (_a))
#define UDP_PORT(_a)	*(reinterpret_cast<ushort *> (((char *) (_a)) + 4))

#define	MAX_TRACKER_SERVERS	(512 / sizeof (tUdpAddress))

typedef struct tServerList {
	ubyte	id;
	ubyte	nServers;
	tUdpAddress		servers [MAX_TRACKER_SERVERS];
} tServerList;

extern tServerList	serverList;
extern int				bUseTracker;

void CreateTrackerList (void);
void DestroyTrackerList (void);
int AddServerToTracker (void);
int RequestServerListFromTracker (void);
int ReceiveServerListFromTracker (ubyte *data);
void SetServerFromList (tServerList *psl, int i);
int IsTracker (uint addr, ushort port);
int GetServerFromList (int i);
int ActiveTrackerCount (int bQueryTrackers);

#endif // __tracker_h

// eof
