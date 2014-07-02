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
			uint8_t				data [DL_PACKET_SIZE];
			CNetworkNode	addr;
			uint8_t				nState;
			CFile				cf;
			int32_t				fLen;
			int32_t				nTimeout;
			TCPsocket		socket;	// 0: host, 1: client
			SDL_Thread*		thread;
		} tUploadDest;

	private:
		uint8_t			m_data [DL_PACKET_SIZE];
		tClient 		m_clients [MAX_PLAYERS];
		int32_t			m_nClients;
		uint16_t		m_freeList [MAX_PLAYERS];
		TCPsocket	m_socket;
		int32_t			m_nState;
		int32_t			m_nResult;
		int32_t			m_nSrcLen;
		int32_t			m_nDestLen;
		int32_t			m_nProgress;
		int32_t			m_nPollTime;
		int32_t			m_nRequestTime;
		int32_t			m_nSemaphore;
		bool			m_bDownloading [MAX_PLAYERS];
		int32_t			m_timeouts [10];
		int32_t			m_iTimeout;
		int32_t			m_nTimeout;
		int32_t			m_nOptProgress;
		int32_t			m_nOptPercentage;
		CFile			m_cf;
		char			m_files [2][FILENAME_LEN];
		int32_t			m_nFiles;

	public:
		CDownloadManager () { Init (); }
		void Init (void);
		int32_t SetTimeoutIndex (int32_t i);
		int32_t MaxTimeoutIndex (void);
		int32_t GetTimeoutIndex (void);
		int32_t GetTimeoutSecs (void);
		int32_t InitUpload (uint8_t *data);
		int32_t InitDownload (uint8_t *data);
		int32_t Upload (int32_t nClient);
		int32_t Download (void);
		int32_t DownloadMission (char *pszMission);
		void CleanUp (void);
		int32_t Poll (CMenu& menu, int32_t& key, int32_t nCurItem);

		bool Downloading (uint32_t i);
		inline int32_t Timeout (void) { return m_nTimeout; }
		inline tClient& Client (int32_t i) { return m_clients [i]; }
		inline int32_t GetState (void) { return m_nState; }
		inline int32_t SetState (int32_t nState) {
			int32_t nOldState = m_nState;
			m_nState = nState;
			return nOldState;
			}

	private:
		void SetDownloadFlag (int32_t nPlayer, bool bFlag);
		int32_t FindClient (uint8_t* server, uint8_t* node);
		int32_t FindClient (void);
		int32_t AcceptClient (void);
		int32_t RemoveClient (int32_t i);
		int32_t SendRequest (uint8_t pId, uint8_t pIdFn, tClient* clientP = NULL);
		int32_t RequestUpload (void);
		int32_t RequestDownload (tClient* clientP);
		int32_t ConnectToServer (void);
		int32_t ConnectToClient (tClient& client);
		void ResendRequest (void);
		int32_t SendData (uint8_t nIdFn, tClient& client);
		int32_t OpenFile (tClient& client, const char *pszExt);
		int32_t SendFile (tClient& client);
		int32_t DownloadError (int32_t nReason);
	};

extern CDownloadManager downloadManager;

#endif //__autodl_h
