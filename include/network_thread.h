#ifndef _NETWORK_THREAD_H
#define _NETWORK_THREAD_H

//------------------------------------------------------------------------------

class CNetworkClientInfo : public CNetworkAddress {
	public:
		int32_t		m_nPacketId [2];
		uint32_t		m_nLost;
		uint32_t		m_timestamp;

		inline void Reset (void) {
			m_nPacketId [0] = 0;
			m_nPacketId [1] = 0;
			m_nLost = 0;
			m_timestamp = 0;
			}
		inline int32_t SetPacketId (int32_t nType, int32_t nId) { return m_nPacketId [nType] = nId; }
		inline int32_t GetPacketId (int32_t nType) { return m_nPacketId [nType]; }
		inline void SetTime (uint32_t t) { m_timestamp = t; }
		inline uint32_t GetTime (void) { return m_timestamp; }
	};

//------------------------------------------------------------------------------

class CNetworkClientList : public CStack< CNetworkClientInfo >	{
	public:
		bool Create (void);
		void Destroy (void);
		CNetworkClientInfo* Find (CNetworkAddress& client);
		CNetworkClientInfo* Update (CNetworkAddress& client);
		void Cleanup (void);
	};

//------------------------------------------------------------------------------

class CNetworkPacketOwner {
	public:
		CPacketAddress		m_address;
		tNetworkNode		m_localAddress;
		uint8_t				m_bHaveLocalAddress;
		uint8_t				m_nPlayer;

	public:
		CNetworkPacketOwner () { m_bHaveLocalAddress = 0; }

		void SetAddress (uint8_t* network, uint8_t* node) {
			m_address.SetNetwork (network);
			m_address.SetNode (node);
			}

		CNetworkAddress GetAddress (void) { return CNetworkAddress (m_address.Address ()); }

		int32_t CmpAddress (uint8_t* network, uint8_t* node) {
			return memcmp (m_address.Network (), network, 4) || memcpy (m_address.Node (), node, 6); 
			}

		void SetLocalAddress (uint8_t* node) {
			if (m_bHaveLocalAddress = (node != NULL))
				memcpy (&m_localAddress, node, sizeof (m_localAddress));
			else
				memset (&m_localAddress, 0, sizeof (m_localAddress));
			}

		inline uint8_t* Network (void) { return m_address.Network (); }
		inline uint8_t* Node (void) { return m_address.Node (); }
		inline uint8_t* LocalNode (void) { return m_localAddress.b; }
};

//------------------------------------------------------------------------------

class CNetworkPacketData {
	public:
		uint16_t	m_size;
		struct {
			int32_t	nId;
			uint8_t	buffer [MAX_PACKET_SIZE]; // buffer must be big enough to hold the amount of data read by the UDP interface, which contains some additional info
		} m_data;

	public:
		CNetworkPacketData () : m_size (0) {}

		inline void SetData (uint8_t* data, int32_t size, int32_t offset = 0) { 
			memcpy (m_data.buffer + offset, data, size); 
			m_size = offset + size;
			}

		inline CNetworkPacketData& operator= (CNetworkPacketData& other) { 
			SetData (other.Buffer (), other.m_size); 
			return *this;
			}

		inline void SetId (int32_t nId) { m_data.nId = nId; }
		inline int32_t GetId (void) { return m_data.nId; }
		inline uint8_t* Buffer (int32_t offset = 0) { return m_data.buffer + offset; }
		inline int32_t Size (void) { return m_size; }
		inline int32_t SetSize (int32_t size) { return m_size = size; }
		inline bool operator== (CNetworkPacketData& other) { return (m_size == other.m_size) && !memcmp (Buffer (), other.Buffer (), m_size); }
};

//------------------------------------------------------------------------------

class CNetworkPacket : public CNetworkPacketData {
	public:	
		CNetworkPacket*		m_nextPacket;
		uint32_t					m_timestamp;
		int32_t					m_bUrgent;
		int32_t					m_bImportant;
		CNetworkPacketOwner	m_owner;

	public:
		CNetworkPacket () : m_nextPacket (NULL), m_timestamp (0), m_bUrgent (0), m_bImportant (0) {}
		void Transmit (void);
		inline void Reset (void) {
			SetId (0);
			SetSize (0);
			SetUrgent (0);
			SetImportant (0);
			}

		inline int32_t SetTime (int32_t timestamp) { return m_timestamp = timestamp; }
		inline CNetworkPacket* Next (void) { return m_nextPacket; }
		inline CNetworkPacketOwner& Owner (void) { return m_owner; }
		inline uint32_t Timestamp (void) { return m_timestamp; }
		inline uint8_t Type (void) { return (m_size > 0) ? m_data.buffer [0]  & ~0x80 : 0xff; }
		inline int32_t IsUrgent (void) { return m_bUrgent; }
		inline void SetUrgent (int32_t bUrgent) { m_bUrgent = bUrgent; }
		inline void SetImportant (int32_t bImportant) { 
			if (bImportant && !m_bImportant) {
				m_bImportant = 1;
				int32_t nId = GetId ();
				if (nId > 0)
					SetId (-nId); // mark packet as import w/o using additional data space
				}
			}
		inline int32_t IsImportant (void) { return m_bImportant; }
		bool Combineable (uint8_t type);
		bool Combine (uint8_t* data, int32_t size, uint8_t* network, uint8_t* node);
		inline bool operator== (CNetworkPacket& other) { return (m_owner.m_address == other.m_owner.m_address) && ((CNetworkPacketData&) *this == (CNetworkPacketData&) other); }

	private:
		bool HasId (void);
		int32_t DataOffset (void);
};

//------------------------------------------------------------------------------

#define LISTEN_QUEUE	0
#define SEND_QUEUE	1

class CNetworkPacketQueue {
	private:
		int32_t					m_nPackets;
		CNetworkPacket*		m_packets [3];
		CNetworkPacket*		m_current;
		SDL_sem*					m_semaphore;
		CNetworkClientList	m_clients;

	public:
		uint32_t					m_nDuplicate;
		uint32_t					m_nCombined;
		uint32_t					m_nLost;
		uint32_t					m_nType; // 0: listen, 1: send

	public:
		CNetworkPacketQueue ();
		~CNetworkPacketQueue ();
		void Create (void);
		void Destroy (void);
		CNetworkPacket* Alloc (bool bLock = true);
		void Free (CNetworkPacket* packet, bool bLock = true);
		inline CNetworkPacket* Head (void) { return m_packets [0]; }
		inline CNetworkPacket* Tail (void) { return m_packets [1]; }
		inline CNetworkPacket* FreeList (void) { return m_packets [2]; }
		inline void SetHead (CNetworkPacket* packet) { m_packets [0] = packet; }
		inline void SetTail (CNetworkPacket* packet) { m_packets [1] = packet; }
		CNetworkPacket* Start (int32_t nPacket = 0);
		inline CNetworkPacket* Step (void) { return m_current ? m_current = m_current->Next () : NULL; }
		inline CNetworkPacket* Current (void) { return m_current; }
		void Flush (void);
		CNetworkPacket* Append (CNetworkPacket* packet = NULL, bool bAllowDuplicates = true, bool bLock = false);
		CNetworkPacket* Pop (bool bDrop = false, bool bLock = true);
		CNetworkPacket* Get (void);
		void Update (void);
		void UpdateClientList (void);
		int32_t Lock (bool bLock = true);
		int32_t Unlock (bool bLock = true);
		bool Validate (void);
		inline int32_t Length (void) { return m_nPackets; }
		inline void SetType (int32_t nType) { m_nType = nType; }
		inline bool Empty (void) { return Head () == NULL; }
};

//------------------------------------------------------------------------------

class CSyncThread {
	private:
		CNetworkInfo			m_client;
};

//------------------------------------------------------------------------------

class CNetworkThread {
	private:
		SDL_Thread*				m_thread;
		SDL_sem*					m_semaphore;
		SDL_sem*					m_sendLock;
		SDL_sem*					m_recvLock;
		SDL_sem*					m_processLock;
		int32_t					m_nThreadId;
		int32_t					m_bUrgent;
		int32_t					m_bImportant;
		bool						m_bRun;
		CNetworkPacketQueue	m_txPacketQueue; // transmit
		CNetworkPacketQueue	m_rxPacketQueue; // receive
		CNetworkPacket*		m_packet;
		CNetworkPacket*		m_syncPackets;
		CTimeout					m_toSend;

	public:
		CNetworkThread ();
		bool Available (void) { return m_thread != NULL; }
		void Run (void);
		void Start (void);
		void Stop (void);
		int32_t CheckPlayerTimeouts (void);
		void SendLifeSign (bool bForce = false);
		int32_t Listen (void);
		CNetworkPacket* GetPacket (bool bLock = true);
		int32_t GetPacketData (uint8_t* data);
		int32_t ProcessPackets (void);
		void FlushPackets (void);
		void Cleanup (void);
		int32_t Lock (SDL_sem* semaphore, bool bTry = false);
		int32_t Unlock (SDL_sem* semaphore);
		inline int32_t LockThread (bool bTry = false) { return Lock (m_semaphore); }
		inline int32_t UnlockThread (void) { return Unlock (m_semaphore); }
		inline int32_t LockSend (bool bTry = false) { return Lock (m_sendLock); }
		inline int32_t UnlockSend (void) { return Unlock (m_sendLock); }
		inline int32_t LockRecv (bool bTry = false) { return Lock (m_recvLock); }
		inline int32_t UnlockRecv (void) { return Unlock (m_recvLock); }
		int32_t LockProcess (bool bTry = false) { return Lock (m_processLock); }
		int32_t UnlockProcess (void) { return Unlock (m_processLock); }
		bool Send (uint8_t* data, int32_t size, uint8_t* network, uint8_t* srcNode, uint8_t* destNode = NULL);
		int32_t Transmit (bool bForce = false);
		int32_t InitSync (void);
		void AbortSync (void);
		void SendSync (void);
		bool SyncInProgress (void);
		inline CNetworkPacketQueue& RxPacketQueue (void) { return m_rxPacketQueue; }
		inline CNetworkPacketQueue& TxPacketQueue (void) { return m_txPacketQueue; }
		inline void SetUrgent (int32_t bUrgent) { m_bUrgent = bUrgent; }
		inline void SetImportant (int32_t bImportant) { m_bImportant = bImportant; }
		inline bool Sending (void) { return Available () && !m_txPacketQueue.Empty (); }
		inline int32_t PacketsPerSec (void) { return int32_t (1000 / m_toSend.Duration ()); }

	private:
		int32_t ConnectionStatus (int32_t nPlayer);
};

extern CNetworkThread networkThread;

//------------------------------------------------------------------------------

#if 1

#	if 1
#		define TIMEOUT_DISCONNECT	3000
#	else
#		define TIMEOUT_DISCONNECT	15000
#endif

#	define TIMEOUT_KICK			180000

#else

#	define TIMEOUT_DISCONNECT	3000
#	define TIMEOUT_KICK			30000

#endif

//------------------------------------------------------------------------------

#endif /* _NETWORK_THREAD_H */
