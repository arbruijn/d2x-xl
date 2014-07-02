#ifndef _NETWORK_THREAD_H
#define _NETWORK_THREAD_H

//------------------------------------------------------------------------------

class CNetworkPacketOwner {
	public:
		CPacketAddress	m_address;
		tNetworkAddr	m_localAddress;
		uint8_t			m_bHaveLocalAddress;
		uint8_t			m_nPlayer;

	public:
		CNetworkPacketOwner () { m_bHaveLocalAddress = 0; }

		void SetAddress (uint8_t* network, uint8_t* node) {
			m_address.SetNetwork (network);
			m_address.SetNode (node);
			}

		int CmpAddress (uint8_t* network, uint8_t* node) {
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
		inline uint8_t* LocalNode (void) { return m_localAddress.node; }
};

//------------------------------------------------------------------------------

class CNetworkData {
	public:
		uint16_t	m_size;
		uint8_t	m_data [MAX_PACKET_SIZE];

	public:
		CNetworkData () : m_size (0) {}
		inline void SetData (uint8_t* data, int32_t size, int32_t offset = 0) { 
			memcpy (m_data, data, size); 
			m_size += size;
			}
		inline CNetworkData& operator= (CNetworkData& other) { 
			SetData (other.m_data, other.m_size); 
			return *this;
			}
		inline int32_t Size (void) { return m_size; }
		inline int32_t SetSize (int32_t size) { return m_size = size; }
		inline bool operator== (CNetworkData& other) { return (m_size == other.m_size) && !memcmp (m_data, other.m_data, m_size); }
};

//------------------------------------------------------------------------------

class CNetworkPacket : public CNetworkData {
	public:	
		CNetworkPacket*		m_nextPacket;
		uint32_t					m_timestamp;
		int32_t					m_bUrgent;
		CNetworkPacketOwner	m_owner;

	public:
		CNetworkPacket () : m_nextPacket (NULL), m_timestamp (0), m_bUrgent (0) {}
		void Transmit (void);
		inline int32_t SetTime (int32_t timestamp) { return m_timestamp = timestamp; }
		inline CNetworkPacket* Next (void) { return m_nextPacket; }
		inline CNetworkPacketOwner& Owner (void) { return m_owner; }
		inline uint32_t Timestamp (void) { return m_timestamp; }
		inline uint8_t Type (void) { return (m_size > 0) ? m_data [0] : 0xff; }
		inline int32_t Urgent (void) { return m_bUrgent; }
		inline void SetUrgent (int32_t bUrgent) { m_bUrgent = bUrgent; }
		inline bool operator== (CNetworkPacket& other) { return (m_owner.m_address == other.m_owner.m_address) && ((CNetworkData) *this == (CNetworkData) other); }
};

//------------------------------------------------------------------------------

class CNetworkPacketQueue {
	private:
		int32_t				m_nPackets;
		CNetworkPacket*	m_packets [3];
		CNetworkPacket*	m_current;
		SDL_sem*				m_semaphore;

	public:
		CNetworkPacketQueue ();
		~CNetworkPacketQueue ();
		CNetworkPacket* Alloc (void);
		void Free (CNetworkPacket* packet);
		inline CNetworkPacket* Head (void) { return m_packets [0]; }
		inline CNetworkPacket* Tail (void) { return m_packets [1]; }
		inline CNetworkPacket* FreeList (void) { return m_packets [2]; }
		inline void SetHead (CNetworkPacket* packet) { m_packets [0] = packet; }
		inline void SetTail (CNetworkPacket* packet) { m_packets [1] = packet; }
		CNetworkPacket* Start (int32_t nPacket = 0);
		inline CNetworkPacket* Step (void) { return m_current ? m_current = m_current->Next () : NULL; }
		inline CNetworkPacket* Current (void) { return m_current; }
		void Flush (void);
		CNetworkPacket* Append (CNetworkPacket* packet = NULL, bool bAllowDuplicates = true);
		CNetworkPacket* Pop (bool bDrop = false, bool bLock = true);
		CNetworkPacket* Get (void);
		int32_t Lock (void);
		int32_t Unlock (void);
		bool Validate (void);
		inline int32_t Length (void) { return m_nPackets; }
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
		SDL_mutex*				m_sendLock;
		SDL_mutex*				m_recvLock;
		SDL_mutex*				m_processLock;
		int32_t					m_nThreadId;
		int32_t					m_bUrgent;
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
		void End (void);
		int32_t CheckPlayerTimeouts (void);
		void Update (void);
		int32_t Listen (void);
		CNetworkPacket* GetPacket (void);
		int32_t GetPacketData (uint8_t* data);
		int32_t ProcessPackets (void);
		void FlushPackets (void);
		void Cleanup (void);
		int32_t Lock (void);
		int32_t Unlock (void);
		int32_t LockSend (void);
		int32_t UnlockSend (void);
		int32_t LockRecv (void);
		int32_t UnlockRecv (void);
		int32_t LockProcess (void);
		int32_t UnlockProcess (void);
		bool Send (uint8_t* data, int32_t size, uint8_t* network, uint8_t* srcNode, uint8_t* destNode = NULL);
		void Transmit (void);
		int32_t InitSync (void);
		void AbortSync (void);
		void SendSync (void);
		bool SyncInProgress (void);
		inline void SetUrgent (int32_t bUrgent) { m_bUrgent = bUrgent; }
		inline bool Sending (void) { return Available () && !m_txPacketQueue.Empty (); }

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
