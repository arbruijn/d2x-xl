#ifndef __autodl_h
#define autodl_h

#define PID_DL_START		68
#define PID_DL_OPEN		69
#define PID_DL_DATA     70
#define PID_DL_CLOSE		71
#define PID_DL_END		72
#define PID_DL_ERROR		73

#define DL_AVAILABLE		0
#define DL_CONNECT		1
#define DL_CONNECTED		2
#define DL_OPEN_HOG		3
#define DL_SEND_HOG		4
#define DL_OPEN_MSN		5
#define DL_SEND_MSN		6
#define DL_FINISH			7
#define DL_DONE			8
#define DL_CREATE_FILE	9
#define DL_DATA			10
#define DL_CANCEL			254
#define DL_ERROR			255

// Actually using some MTU as packet size for TCP isn't necessary as TCP will properly packetize outgoing data depending on the actual
// local MTU. Some meaningful packet size should be used anyway though, and using a common MTU here shouldn't be the worst idea.

#define DL_HEADER_SIZE		5
#define DL_PACKET_SIZE		PPPoE_MTU // 1024 + 256 + 128 + DL_HEADER_SIZE; data transfer will be unstable at larger sizes for me (?)
#define DL_PAYLOAD_SIZE		(DL_PACKET_SIZE - DL_HEADER_SIZE)


// upload buffer
// format:
// m_uploadBuf [0] == {PID_UPLOAD | PID_DOWNLOAD}
// m_uploadBuf [0] == {PID_DL_START | ... | PID_DL_ERROR}
// m_uploadBuf [1..4] == <data length>
// m_uploadBuf [5..1413] == <data (max. DL_PAYLOAD_SIZE bytes)>

class CDownloadManager {
	public:
		typedef struct tClient {
			ubyte			data [DL_PACKET_SIZE];
			tIpxAddr		addr;
			ubyte			nState;
			CFile			cf;
			int			fLen;
			int			nTimeout;
			TCPsocket	socket;	// 0: host, 1: client
			SDL_Thread*	thread;
		} tUploadDest;

	private:
		ubyte			m_data [DL_PACKET_SIZE];
		tClient 		m_clients [MAX_PLAYERS];
		int			m_nClients;
		ushort		m_freeList [MAX_PLAYERS];
		TCPsocket	m_socket;
		int			m_nState;
		int			m_nResult;
		int			m_nSrcLen;
		int			m_nDestLen;
		int			m_nProgress;
		int			m_nPollTime;
		int			m_nRequestTime;
		int			m_nSemaphore;
		bool			m_bDownloading [MAX_PLAYERS];
		int			m_timeouts [10];
		int			m_iTimeout;
		int			m_nTimeout;
		int			m_nOptProgress;
		int			m_nOptPercentage;
		CFile			m_cf;
		char			m_files [2][FILENAME_LEN];
		int			m_nFiles;

	public:
		CDownloadManager () { Init (); }
		void Init (void);
		int SetTimeoutIndex (int i);
		int MaxTimeoutIndex (void);
		int GetTimeoutIndex (void);
		int GetTimeoutSecs (void);
		int InitUpload (ubyte *data);
		int InitDownload (ubyte *data);
		int Upload (int nClient);
		int Download (void);
		int DownloadMission (char *pszMission);
		void CleanUp (void);
		int Poll (CMenu& menu, int& key, int nCurItem);

		bool Downloading (uint i);
		inline int Timeout (void) { return m_nTimeout; }
		inline tClient& Client (int i) { return m_clients [i]; }
		inline int GetState (void) { return m_nState; }
		inline int SetState (int nState) {
			int nOldState = m_nState;
			m_nState = nState;
			return nOldState;
			}

	private:
		void SetDownloadFlag (int nPlayer, bool bFlag);
		int FindClient (ubyte* server, ubyte* node);
		int FindClient (void);
		int AcceptClient (void);
		int RemoveClient (int i);
		int SendRequest (ubyte pId, ubyte pIdFn, tClient* clientP = NULL);
		int RequestUpload (void);
		int RequestDownload (tClient* clientP);
		int ConnectToServer (void);
		int ConnectToClient (tClient& client);
		void ResendRequest (void);
		int SendData (ubyte nIdFn, tClient& client);
		int OpenFile (tClient& client, const char *pszExt);
		int SendFile (tClient& client);
		int DownloadError (int nReason);
	};

extern CDownloadManager downloadManager;

#endif //__autodl_h
