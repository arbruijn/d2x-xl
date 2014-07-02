#ifndef _NETWORK_THREAD_H
#define _NETWORK_THREAD_H

//------------------------------------------------------------------------------

class CNetworkPacketOwner {
	public:
		CPacketOrigin				m_source;
		tNetworkAddr				m_dest;
		ubyte							m_bHaveDest;
		ubyte							m_nPlayer;

	public:
		CNetworkPacketOwner () { m_bHaveDest = 0; }

		void SetSource (ubyte* network, ubyte* node) {
			m_source.SetNetwork (network);
			m_source.SetNode (node);
			}

		void SetDest (ubyte* node) {
			if (m_bHaveDest = (node != NULL))
				memcpy (&m_dest, node, sizeof (m_dest));
			else
				memset (&m_dest, 0, sizeof (m_dest));
			}

		inline ubyte* SrcNetwork (void) { return m_source.Network (); }
		inline ubyte* SrcNode (void) { return m_source.Node (); }
		inline ubyte* DestNode (void) { return m_dest.node; }
};

class CNetworkData {
	public:
		ushort					m_size;
		ubyte						m_data [MAX_PACKET_SIZE];

	public:
		CNetworkData () : m_size (0) {}
		inline void SetData (ubyte* data, int size) { memcpy (m_data, data, m_size = size); }
		inline CNetworkData& operator= (CNetworkData& other) { 
			SetData (other.m_data, other.m_size); 
			return *this;
			}
		inline int Size (void) { return m_size; }
		inline int SetSize (int size) { return m_size = size; }
		inline bool operator== (CNetworkData& other)  { return (m_size == other.m_size) && !memcmp (m_data, other.m_data, m_size); }
};

//------------------------------------------------------------------------------

class CNetworkPacket : public CNetworkData {
	public:	
		CNetworkPacket*		m_nextPacket;
		uint						m_timestamp;
		int						m_bUrgent;
		CNetworkPacketOwner	m_owner;

	public:
		CNetworkPacket () : m_nextPacket (NULL), m_timestamp (0), m_bUrgent (0) {}
		void Transmit (void);
		inline int SetTime (int timestamp) { return m_timestamp = timestamp; }
		inline CNetworkPacket* Next (void) { return m_nextPacket; }
		inline uint Timestamp (void) { return m_timestamp; }
		inline ubyte Type (void) { return (m_size > 0) ? m_data [0] : 0xff; }
		inline int Urgent (void) { return m_bUrgent; }
		inline void SetUrgent (int bUrgent) { m_bUrgent = bUrgent; }
		inline bool operator== (CNetworkPacket& other) { return (m_owner.m_source == other.m_owner.m_source) && ((CNetworkData) *this == (CNetworkData) other); }
};

//------------------------------------------------------------------------------

class CNetworkPacketQueue {
	private:
		int						m_nPackets;
		CNetworkPacket*		m_packets [2];
		CNetworkPacket*		m_current;
		SDL_sem*					m_semaphore;

	public:
		CNetworkPacketQueue ();
		~CNetworkPacketQueue ();
		inline CNetworkPacket* Head (void) { return m_packets [0]; }
		inline CNetworkPacket* Tail (void) { return m_packets [1]; }
		inline void SetHead (CNetworkPacket* packet) { m_packets [0] = packet; }
		inline void SetTail (CNetworkPacket* packet) { m_packets [1] = packet; }
		CNetworkPacket* Start (int nPacket = 0);
		inline CNetworkPacket* Step (void) { return m_current ? m_current = m_current->Next () : NULL; }
		inline CNetworkPacket* Current (void) { return m_current; }
		void Flush (void);
		CNetworkPacket* Append (CNetworkPacket* packet = NULL, bool bAllowDuplicates = true);
		CNetworkPacket* Pop (bool bDrop = false, bool bLock = true);
		CNetworkPacket* Get (void);
		int Lock (void);
		int Unlock (void);
		bool Validate (void);
		inline int Length (void) { return m_nPackets; }
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
		int						m_nThreadId;
		int						m_bUrgent;
		CNetworkPacketQueue	m_txPacketQueue; // transmit
		CNetworkPacketQueue	m_rxPacketQueue; // receive
		CNetworkPacket*		m_packet;
		CNetworkPacket*		m_syncPackets;
		CTimeout					m_toSend;
		int						m_pps;

	public:
		CNetworkThread ();
		bool Available (void) { return m_thread != NULL; }
		void Run (void);
		void Start (void);
		void End (void);
		int CheckPlayerTimeouts (void);
		void Update (void);
		int Listen (void);
		CNetworkPacket* GetPacket (void);
		int GetPacketData (ubyte* data);
		int ProcessPackets (void);
		void FlushPackets (void);
		void Cleanup (void);
		int Lock (void);
		int Unlock (void);
		int LockSend (void);
		int UnlockSend (void);
		int LockRecv (void);
		int UnlockRecv (void);
		int LockProcess (void);
		int UnlockProcess (void);
		bool Send (ubyte* data, int size, ubyte* network, ubyte* srcNode, ubyte* destNode = NULL);
		void Transmit (void);
		int InitSync (void);
		void AbortSync (void);
		void SendSync (void);
		bool SyncInProgress (void);
		inline void SetUrgent (int bUrgent) { m_bUrgent = bUrgent; }
		inline bool Sending (void) { return Available () && !m_txPacketQueue.Empty (); }

	private:
		int ConnectionStatus (int nPlayer);
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
