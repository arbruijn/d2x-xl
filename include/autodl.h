#ifndef __autodl_h
#define autodl_h

#if DBG
int iDlTimeout = 5;
#else
int iDlTimeout = 4;
#endif

#define PID_DL_START		68
#define PID_DL_OPEN		69
#define PID_DL_DATA     70
#define PID_DL_CLOSE		71
#define PID_DL_END		72
#define PID_DL_ERROR		73

#define	DL_OPEN_HOG		0
#define	DL_SEND_HOG		1
#define	DL_OPEN_MSN		2
#define	DL_SEND_MSN		3
#define	DL_FINISH		4

typedef struct tUploadDest {
	ipx_addr	addr;
	ubyte		nState;
	CFile		cf;
	int		fLen;
	int		nPacketId;
	int		nTimeout;
} tUploadDest;

// upload buffer
// format:
// uploadBuf [0] == {PID_UPLOAD | PID_DOWNLOAD}
// uploadBuf [1] == {PID_DL_START | ... | PID_DL_ERROR}
// uploadBuf [2..11] == <source address>
// uploadBuf [12..1035] == <data (max. DL_BUFSIZE bytes)>

#define DL_BUFSIZE (MAX_DATASIZE - 4 - 14)

static ubyte uploadBuf [MAX_PACKETSIZE];

int bDownloading [MAX_PLAYERS] = {0,0,0,0,0,0,0,0};

class CDownloadManager {
	private:
		tUploadDest m_uploadDests [MAX_PLAYERS];
		int			m_nUploadDests;
		int			m_nPacketId;
		int			m_nPacketTimeout;
		int			m_nState;
		int			m_nResult;
		int			m_nSrcLen;
		int			m_nDestLen;
		int			m_nProgress;
		int			m_nPollTime;
		bool			m_bDownloading [MAX_PLAYERS];
		int			m_timeouts [10];
		int			m_iTimeout;
		int			m_nTimeout;
		int			m_nOptProgress;
		int			m_nOptPercentage;
		CFile			m_cf;

	public:
		CDownloadManager () { Init (); }
		void Init (void);
		int SetTimeoutIndex (int i);
		int MaxTimeoutIndex (void);
		int GetTimeoutIndex (void);
		int GetTimeoutSecs (void);
		int Upload (ubyte *data);
		int Download (ubyte *data);
		int DownloadMission (char *pszMission);
		void CleanUp (void);
		int Poll (CMenu& menu, int& key, int nCurItem);

		inline bool Downloading (uint i) { return (i < MAX_PLAYERS) ? m_bDownloading [i] : false; }
		inline int Timeout (void) { return m_nTimeout; }

	private:
		void SetDownloadFlag (int nPlayer, int nFlag);
		int FindUploadDest (void);
		int AddUploadDest (void);
		int DelUploadDest (int i);
		int SendRequest (ubyte pId, ubyte pIdFn, int nSize, int nPacketId);
		int RequestUpload (ubyte pIdFn, int nSize);
		int RequestDownload (ubyte pIdFn, int nSize, int nPacketId);
		void ResendRequest (void);
		int UploadError (void);
		int UploadOpenFile (int i, const char *pszExt);
		int UploadSendFile (int i, int nPacketId);
		int DownloadError (int nReason);
	};

extern CDownloadManager downloadManager;

#endif //__autodl_h
