#ifndef _NETWORK_THREAD_H
#define _NETWORK_THREAD_H

//------------------------------------------------------------------------------

typedef struct tNetworkPacketOwner {
	CPacketOrigin				address;
	ubyte							nPlayer;
} tNetworkPacketOwner;

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
		CNetworkPacket*		nextPacket;
		uint						timeStamp;
		tNetworkPacketOwner	owner;
		ubyte						data [MAX_PACKET_SIZE];

	public:
		CNetworkPacket () : nextPacket (NULL), timeStamp (0) {}
		void Send (void);
		inline bool operator== (CNetworkPacket& other) { return (owner.address == other.owner.address) && ((CNetworkData) *this == (CNetworkData) other); }
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
		inline CNetworkPacket* Next (void) { return m_current ? m_current = m_current->nextPacket : NULL; }
		inline CNetworkPacket* Current (void) { return m_current; }
		void Flush (void);
		CNetworkPacket* Append (CNetworkPacket* packet = NULL, bool bAllowDuplicates = true);
		CNetworkPacket* Pop (void);
		CNetworkPacket* Get (void);
		int Lock (void);
		int Unlock (void);
		bool Validate (void);
		inline int Length (void) { return m_nPackets; }
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
		SDL_sem*					m_syncLock;
		SDL_mutex*				m_sendLock;
		SDL_mutex*				m_recvLock;
		SDL_mutex*				m_processLock;
		int						m_nThreadId;
		bool						m_bListen;
		bool						m_bSendSync;
		tNetworkPacketOwner	m_owner;
		CNetworkPacketQueue	m_txPacketQueue; // transmit
		CNetworkPacketQueue	m_rxPacketQueue; // receive
		CNetworkPacket*		m_packet;

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
		int LockSync (void);
		int UnlockSync (void);
		int LockSend (void);
		int UnlockSend (void);
		int LockRecv (void);
		int UnlockRecv (void);
		int LockProcess (void);
		int UnlockProcess (void);
		inline void SetListen (bool bListen) { m_bListen = bListen; }
		int InitSync (void);
		bool AddSyncPacket (ubyte* data, int size, ubyte* network, ubyte* node);
		bool StartSync (int nPacket = 0);
		void SendSync (void);
		void UpdateSync (int nPacket);
		void StopSync (void);
		inline bool SyncInProgress (void) { return Available () && m_bSendSync; }

	private:
		int ConnectionStatus (int nPlayer);
		inline bool GetListen (void) { return m_bListen; }
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
