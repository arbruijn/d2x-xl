#ifndef _NETWORK_THREAD_H
#define _NETWORK_THREAD_H

//------------------------------------------------------------------------------

typedef struct tNetworkPacketOwner {
	tIPXRecvData				address;
	ubyte							nPlayer;
} tNetworkPacketOwner;

typedef struct tNetworkPacket {
	struct tNetworkPacket*	nextPacket;
	ushort						size;
	ubyte							data [MAX_PACKET_SIZE];
	tNetworkPacketOwner		owner;
} tNetworkPacket;

//------------------------------------------------------------------------------

class CNetworkThread {
	private:
		SDL_Thread*				m_thread;
		SDL_sem*					m_semaphore;
		SDL_mutex*				m_sendLock;
		SDL_mutex*				m_recvLock;
		SDL_mutex*				m_processLock;
		int						m_nThreadId;
		bool						m_bListen;
		int						m_nPackets;
		tNetworkPacketOwner	m_owner;
		tNetworkPacket*		m_packets [2];
		tNetworkPacket*		m_packet;

	public:
		CNetworkThread ();
		bool Available (void) { return m_thread != NULL; }
		void Process (void);
		void Start (void);
		void End (void);
		int CheckPlayerTimeouts (void);
		void Update (void);
		int Listen (void);
		tNetworkPacket* GetPacket (void);
		int GetPacketData (ubyte* data);
		int ProcessPackets (void);
		void FlushPackets (void);
		int SemWait (void);
		int SemPost (void);
		int LockSend (void);
		int UnlockSend (void);
		int LockRecv (void);
		int UnlockRecv (void);
		int LockProcess (void);
		int UnlockProcess (void);
		inline void SetListen (bool bListen) { m_bListen = bListen; }

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
