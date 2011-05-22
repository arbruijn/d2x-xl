// tracker.h
// game server tracker communication

#ifndef __tracker_h
#define __tracker_h

typedef ubyte tUdpAddress [6];

#define UDP_ADDR(_a)	*((uint *) (_a))
#define UDP_PORT(_a)	*((ushort *) ((char *) (_a) + 4))

#define	MAX_TRACKER_SERVERS	(512 / sizeof (tUdpAddress))


typedef struct tServerList {
	ubyte	id;
	ubyte	nServers;
	tUdpAddress		servers [MAX_TRACKER_SERVERS];
} __pack__ tServerList;


typedef struct tServerListTable {
	struct tServerListTable	*nextList;
	tServerList					serverList;
	tUdpAddress					*tracker;
	time_t						lastActive;
} tServerListTable;

class CTracker {
	private:
		tServerList			m_list;
		tServerListTable*	m_table;

	public:
		int					m_bUse;

	public:
		CTracker () { Init (); }
		~CTracker () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void CreateList (void);
		void DestroyList (void);
		int AddServer (void);
		int RequestServerList (void);
		int ReceiveServerList (ubyte *data);
		void SetServerFromList (tServerList *psl, int i, ubyte* serverAddress);
		int IsTracker (uint addr, ushort port, char* msg = NULL);
		int GetServerFromList (int i, ubyte* serverAddress);
		int CountActive (void);
		int ActiveCount (int bQueryTrackers);

		int Poll (CMenu& menu, int& key, int nCurItem);

	private:
		int Find (tUdpAddress *addr);
		void Call (int i, ubyte *pData, int nDataLen);
		int Add (tUdpAddress *addr);
		int Query (void);
		void AddFromCmdLine (void);
		int ParseIpAndPort (char *pszAddr, tUdpAddress *addr);
		void ResetList (void);
	};

#endif // __tracker_h

extern CTracker tracker;

// eof
