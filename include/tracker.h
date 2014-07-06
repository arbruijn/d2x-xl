// tracker.h
// game server tracker communication

#ifndef __tracker_h
#define __tracker_h

typedef uint8_t tUdpAddress [6];

#if 0

// this causes dereferencing type punned pointer warnings from g++
#define UDP_ADDR(_a)	*((uint32_t *) (_a))
#define UDP_PORT(_a)	*((uint16_t *) ((char *) (_a) + 4))

#else

static inline uint32_t& UDP_ADDR(void *a)
{
uint32_t *tmp = reinterpret_cast<uint32_t*> (a);
return *tmp;
}

static inline uint16_t& UDP_PORT(void *a)
{
uint16_t *tmp = reinterpret_cast<uint16_t*> (reinterpret_cast<uint8_t*>(a) + 4);
return *tmp;
}

#endif

#define	MAX_TRACKER_SERVERS	(512 / sizeof (tUdpAddress))


typedef struct tServerList {
	uint8_t	id;
	uint8_t	nServers;
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
		int32_t					m_bUse;

	public:
		CTracker () { Init (); }
		~CTracker () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void CreateList (void);
		void DestroyList (void);
		int32_t AddServer (void);
		int32_t RequestServerList (void);
		int32_t ReceiveServerList (uint8_t *data);
		void SetServerFromList (tServerList *psl, int32_t i, uint8_t* serverAddress);
		int32_t IsTracker (uint32_t addr, uint16_t port, const char* msg = NULL);
		int32_t GetServerFromList (int32_t i, uint8_t* serverAddress);
		int32_t CountActive (void);
		int32_t ActiveCount (int32_t bQueryTrackers);

		int32_t Poll (CMenu& menu, int32_t& key, int32_t nCurItem);

	private:
		int32_t Find (tUdpAddress *addr);
		void Call (int32_t i, uint8_t *pData, int32_t nDataLen);
		int32_t Add (tUdpAddress *addr);
		int32_t Query (void);
		void AddFromCmdLine (void);
		int32_t ParseIpAndPort (char *pszAddr, tUdpAddress *addr);
		void ResetList (void);
	};

#endif // __tracker_h

extern CTracker tracker;

// eof
