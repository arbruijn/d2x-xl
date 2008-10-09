// autodl.c
// automatic level up/download

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include <time.h>
#include <string.h>

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif
#include "inferno.h"
#include "ipx.h"
#include "key.h"
#include "network.h"
#include "network_lib.h"
#include "menu.h"
#include "newmenu.h"
#include "byteswap.h"
#include "text.h"
#include "strutil.h"
#include "error.h"
#include "autodl.h"

//------------------------------------------------------------------------------

extern unsigned char ipx_ServerAddress [10];
extern unsigned char ipx_LocalAddress [10];

//------------------------------------------------------------------------------

static int nDlTimeouts [] = {1, 2, 3, 5, 10, 15, 20, 30, 45, 60};
static int nDlTimeout = -1;

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

#if 0
static char *szDlStates [] = {
	"start",
	"open file",
	"data",
	"close file",
	"end",
	"error"
	};
#endif

typedef struct tUploadDest {
	ipx_addr	addr;
	ubyte		dlState;
	CFILE		cf;
	int		fLen;
	int		nPacketId;
	int		nTimeout;
} tUploadDest;

tUploadDest uploadDests [MAX_PLAYERS];

static ubyte nUploadDests = 0;
static int nPacketId = -1;
static int nPacketTimeout = -1;
static ubyte dlState = PID_DL_OPEN;
static int dlResult = 1;
static int nSrcLen, nDestLen, nPercent;
static int nTimeout;
static CFILE cf = {NULL, 0, 0, 0};

// upload buffer
// format:
// uploadBuf [0] == {PID_UPLOAD | PID_DOWNLOAD}
// uploadBuf [1] == {PID_DL_START | ... | PID_DL_ERROR}
// uploadBuf [2..11] == <source address>
// uploadBuf [12..1035] == <data (max. DL_BUFSIZE bytes)>

#define DL_BUFSIZE (MAX_DATASIZE - 4 - 14)

static ubyte uploadBuf [MAX_PACKETSIZE];

int bDownloading [MAX_PLAYERS] = {0,0,0,0,0,0,0,0};

//------------------------------------------------------------------------------

void InitAutoDL (void)
{
memset (uploadDests, 0, sizeof (uploadDests));
}

//------------------------------------------------------------------------------

int MaxDlTimeout (void)
{
return sizeof (nDlTimeouts) / sizeof (*nDlTimeouts) - 1;
}

//------------------------------------------------------------------------------

int GetDlTimeout (void)
{
return iDlTimeout;
}

//------------------------------------------------------------------------------

int GetDlTimeoutSecs (void)
{
return nDlTimeouts [iDlTimeout];
}

//------------------------------------------------------------------------------

int SetDlTimeout (int i)
{
if ((i >= 0) && (i <= MaxDlTimeout ()))
	iDlTimeout = i;
nDlTimeout = nDlTimeouts [iDlTimeout] * 1000;
return iDlTimeout;
}

//------------------------------------------------------------------------------

static void SetDownloadFlag (int nPlayer, int nFlag)
{
	int	i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (!memcmp (&uploadDests [nPlayer].addr.server, &netPlayers.players [i].network.ipx.server, 4) &&
		 !memcmp (&uploadDests [nPlayer].addr.node, &netPlayers.players [i].network.ipx.node, 6)) {
		bDownloading [i] = nFlag;
		return;
		}
	}
}

//------------------------------------------------------------------------------

static int FindUploadDest (void)
{
	ubyte	i;

for (i = 0; i < nUploadDests; i++)
	if (!memcmp (&uploadDests [i].addr.server, ipx_udpSrc.src_network, 4) &&
		 !memcmp (&uploadDests [i].addr.node, ipx_udpSrc.src_node, 6)) {
		uploadDests [i].nTimeout = SDL_GetTicks ();
		return i;
		}
return -1;
}

//------------------------------------------------------------------------------

static int AddUploadDest (void)
{
	int	i = FindUploadDest ();

if (i >= 0) {
	if (uploadDests [i].dlState && uploadDests [i].cf.file) {
		CFClose (&uploadDests [i].cf);
		}
	}
else {
	if (nUploadDests >= MAX_PLAYERS)
		return 0;
	i = nUploadDests++;
	memcpy (&uploadDests [i].addr.server, ipx_udpSrc.src_network, 4);
	memcpy (&uploadDests [i].addr.node, ipx_udpSrc.src_node, 6);
	SetDownloadFlag (i, 1);
	uploadDests [i].nTimeout = SDL_GetTicks ();
	}
uploadDests [i].dlState = DL_OPEN_HOG;
uploadDests [i].cf.file = NULL;
return 1;
}

//------------------------------------------------------------------------------

static int DelUploadDest (int i)
{
if (i < 0)
	i = FindUploadDest ();
if (i < 0)
	return 0;
CFClose (&uploadDests [i].cf);
SetDownloadFlag (i, 0);
if (i < --nUploadDests)
	memcpy (uploadDests + i, uploadDests + i + 1, (nUploadDests - i) * sizeof (tUploadDest));
return 1;
}

//------------------------------------------------------------------------------

void CleanUploadDests (void)
{
	int	t, i = 0;
	static int nTimeout = 0;

if (nDlTimeout < 0)
	SetDlTimeout (-1);
if ((t = SDL_GetTicks ()) - nTimeout > nDlTimeout) {
	nTimeout = t;
	while (i < nUploadDests)
		if ((int) SDL_GetTicks () - uploadDests [i].nTimeout > nDlTimeout)
			DelUploadDest (i);
		else
			i++;
	}
}

//------------------------------------------------------------------------------

static int SendRequest (ubyte pId, ubyte pIdFn, int nSize, int nPacketId)
{
uploadBuf [0] = pId;
uploadBuf [1] = pIdFn;
if (pIdFn == PID_DL_DATA) {
	PUT_INTEL_INT (uploadBuf + 2, nPacketId);
	nSize += 4;
	nPacketTimeout = SDL_GetTicks ();
	}
if ((pId == PID_UPLOAD) && (gameStates.multi.nGameType == IPX_GAME))
	IPXSendBroadcastData (uploadBuf, nSize + 2);
else if (pId == PID_UPLOAD)
	IPXSendInternetPacketData (uploadBuf, nSize + 2, ipx_ServerAddress, ipx_ServerAddress + 4);
else
	IPXSendInternetPacketData (uploadBuf, nSize + 2, ipx_udpSrc.src_network, ipx_udpSrc.src_node);
return 1;
}

//------------------------------------------------------------------------------
// ask the game host for the next data packet

static int RequestUpload (ubyte pIdFn, int nSize)
{
return SendRequest (PID_UPLOAD, pIdFn, nSize, nPacketId);
}

//------------------------------------------------------------------------------
// tell the client the game host is ready to send more data

static int RequestDownload (ubyte pIdFn, int nSize, int nPacketId)
{
return SendRequest (PID_DOWNLOAD, pIdFn, nSize, nPacketId);
}

//------------------------------------------------------------------------------

static void ResendRequest (void)
{
if ((dlState == PID_DL_DATA) && (nPacketId > -1) && (SDL_GetTicks () - nPacketTimeout > 500)) 
	RequestUpload (PID_DL_DATA, 0);
}

//------------------------------------------------------------------------------

static int UploadError (void)
{
DelUploadDest (-1);
RequestDownload (PID_DL_ERROR, 0, -1);
return 0;
}

//------------------------------------------------------------------------------
// open a file on the game host

static int UploadOpenFile (int i, const char *pszExt)
{
	char	szFile [FILENAME_LEN];
	int	l = (int) strlen (gameFolders.szMissionDirs [0]);

sprintf (szFile, "%s%s%s%s", 
			gameFolders.szMissionDirs [0], (l && (gameFolders.szMissionDirs [0][l-1] != '/')) ? "/" : "", 
			netGame.szMissionName, pszExt);
if (uploadDests [i].cf.file)
	CFClose (&uploadDests [i].cf);
if (!CFOpen (&uploadDests [i].cf, szFile, "", "rb", 0))
	return UploadError ();
uploadDests [i].fLen = CFLength (&uploadDests [i].cf, 0);
PUT_INTEL_INT (uploadBuf + 2, uploadDests [i].fLen);
sprintf (szFile, "%s%s", netGame.szMissionName, pszExt);
l = (int) strlen (szFile) + 1;
memcpy (uploadBuf + 6, szFile, l);
uploadDests [i].nPacketId = -1;
RequestDownload (PID_DL_OPEN, l + 4, -1);
return 1;
}

//------------------------------------------------------------------------------
// send a file from the game host

static int UploadSendFile (int i, int nPacketId)
{
	int	l, h = nPacketId - uploadDests [i].nPacketId;

if ((h < 0) || (h > 1))
	return -1;
if (!h || (uploadDests [i].fLen > 0)) {
	if (h) {	// send next data packet
		l = (int) uploadDests [i].fLen;
		if (l > 512) //DL_BUFSIZE - 6)
			l = 512; //DL_BUFSIZE - 6;
		if ((int) CFRead (uploadBuf + 10, 1, l, &uploadDests [i].cf) != l)
			return UploadError ();
		PUT_INTEL_INT (uploadBuf + 2, nPacketId);
		PUT_INTEL_INT (uploadBuf + 6, l);
		uploadDests [i].nPacketId = nPacketId;
		}
	else
		l = 0;	// resend last packet
	RequestDownload (PID_DL_DATA, l + 8, nPacketId);
	uploadDests [i].fLen -= l;
	}
else {
	CFClose (&uploadDests [i].cf);
	RequestDownload (PID_DL_CLOSE, 0, -1);
	return -1;
	}
return 1;
}

//------------------------------------------------------------------------------
// Game host sending data to client

int NetworkUpload (ubyte *data)
{
	int	i;
	ubyte pId = data [1];

switch (pId) {
	case PID_DL_START:
		if (!(/*gameStates.app.bHaveExtraGameInfo [1] &&*/ extraGameInfo [0].bAutoDownload) || !AddUploadDest ()) {
			RequestDownload (PID_DL_ERROR, 0, -1);
			return 0;
			}

	case PID_DL_DATA:
		if (0 > (i = FindUploadDest ()))
			return UploadError ();

		switch (uploadDests [i].dlState) {
			case DL_OPEN_HOG:
				if (!(UploadOpenFile (i, ".hog") || 
						UploadOpenFile (i, ".rl2") || 
						UploadOpenFile (i, ".rdl")))
					return 0;
				uploadDests [i].dlState = DL_SEND_HOG;
				return 1;

			case DL_SEND_HOG:
				switch (UploadSendFile (i, GET_INTEL_INT (data + 2))) {
					case 0:
						return 0;
					case -1:
						uploadDests [i].dlState = DL_OPEN_MSN;
					case 1:
						return 1;
					}
				break;

			case DL_OPEN_MSN:
				if (!(UploadOpenFile (i, ".mn2") || UploadOpenFile (i, ".msn")))
					return 0;
				uploadDests [i].dlState = DL_SEND_MSN;
				return 1;
				break;

			case DL_SEND_MSN:
				switch (UploadSendFile (i, GET_INTEL_INT (data + 2))) {
					case 0:
						return 0;
					case -1:
						uploadDests [i].dlState = DL_FINISH;
					case 1:
						return 1;
					}
				break;

			case DL_FINISH:
				RequestDownload (PID_DL_END, 0, -1);
				DelUploadDest (i);
				return -1;
			}
		break;

	case PID_DL_ERROR:
		DelUploadDest (-1);
		return 0;
	}
return 0;
}

//------------------------------------------------------------------------------
// Client receiving data from game host

inline int DownloadError (int nReason)
{
if (nReason == 1)
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_AUTODL_SYNC);
else if (nReason == 2)
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_AUTODL_MISSPKTS);
else if (nReason == 3)
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_AUTODL_FILEIO);
else
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_AUTODL_FAILED);
CFClose (&cf);
RequestUpload (PID_DL_ERROR, 0);
dlResult = 0;
dlState = PID_DL_OPEN;
return 0;
}

//------------------------------------------------------------------------------

int NetworkDownload (ubyte *data)
{
	ubyte pId = data [1];

nTimeout = SDL_GetTicks ();
switch (pId) {
	case PID_DL_START:
		if (dlState != PID_DL_START)
			return DownloadError (1);
		nPacketId = -1;
		RequestUpload (PID_DL_DATA, 0);
		dlResult = 1;
		dlState = PID_DL_OPEN;
		return 1;
		break;

	case PID_DL_ERROR:
		if (cf.file) {
			CFClose (&cf);
			dlResult = 0;
			dlState = PID_DL_OPEN;
			}
		return 0;

	case PID_DL_OPEN:
		if (dlState != PID_DL_OPEN)
			return DownloadError (1);
		{
			char	szDest [FILENAME_LEN];
			char	*pszFile = (char *) data + 6;

		if (!pszFile)
			return DownloadError (2);
		strlwr (pszFile);
		sprintf (szDest, "%s%s%s", gameFolders.szMissionDir, *gameFolders.szMissionDir ? "/" : "", pszFile);
		if (!CFOpen (&cf, szDest, "", "wb", 0))
			return DownloadError (2);
		nSrcLen = GET_INTEL_INT (data + 2);
		nPercent = 0;
		nDestLen = 0;
		nPacketId = 0;
		RequestUpload (PID_DL_DATA, 0);
		dlState = PID_DL_DATA;
		return 1;
		}

	case PID_DL_DATA:
		if (dlState != PID_DL_DATA)
			return DownloadError (1);
		{
			int id = GET_INTEL_INT (data + 2),
				 h = id - nPacketId;

			if (!h) {	// receiving the requested packet
				int l = GET_INTEL_INT (data + 6);

				if (CFWrite (data + 10, 1, l, &cf) != l)
					return DownloadError (2);
				nPacketId++;
				nDestLen += l;
				}
			else if (h != -1)	// host has been resending the last packet; probably due to request and reply crossing each other
				return DownloadError (3);
			}
		RequestUpload (PID_DL_DATA, 0);
		return 1;

	case PID_DL_CLOSE:
		if (dlState != PID_DL_DATA)
			return DownloadError (1);
		if (cf.file)
			CFClose (&cf);
		dlState = PID_DL_OPEN;
		nPacketId = -1;
		RequestUpload (PID_DL_DATA, 0);
		return 1;

	case PID_DL_END:
		if (dlState != PID_DL_OPEN)
			return DownloadError (1);
		dlResult = -1;
		return -1;
	}
return 1;
}

//------------------------------------------------------------------------------

#define PROGRESS_ITEM	2
#define PERCENT_ITEM		1

void DownloadPoll (int nItems, tMenuItem *m, int *key, int cItem)
{
if (*key == KEY_ESC) {
	m [PERCENT_ITEM].text = (char *) "download aborted";
	m [1].redraw = 1;
	*key = -2;
	return;
	}
ResendRequest ();
NetworkListen ();
if (nDlTimeout < 0)
	SetDlTimeout (-1);
if ((int) SDL_GetTicks () - nTimeout > nDlTimeout) {
	strcpy (m [1].text, "download timed out");
	m [1].redraw = 1;
	*key = -2;
	return;
	}
if (dlResult == -1) {
	*key = -3;
	return;
	}
if (dlResult == 1) {
	if ((dlState == PID_DL_OPEN) || (dlState == PID_DL_DATA)) {
		if (nSrcLen && nDestLen) {
			int h = nDestLen * 100 / nSrcLen;
			if (h != nPercent) {
				nPercent = h;
				sprintf (m [PERCENT_ITEM].text, TXT_PROGRESS, nPercent, '%');
				m [PERCENT_ITEM].rebuild = 1;
				h = nPercent;
				if (m [PROGRESS_ITEM].value != h) {
					m [PROGRESS_ITEM].value = h;
					m [PROGRESS_ITEM].rebuild = 1;
					}
				}
			}
		}
	*key = 0;
	return;
	}
m [PERCENT_ITEM].text = (char *) "download failed";
m [PERCENT_ITEM].redraw = 1;
*key = -2;
}

//------------------------------------------------------------------------------

int DownloadMission (char *pszMission)
{
	tMenuItem	m [3];
	char			szTitle [30];
	char			szProgress [30];
	int			i;

PrintLog ("   trying to download mission '%s'\n", pszMission);
gameStates.multi.bTryAutoDL = 0;
if (!(/*gameStates.app.bHaveExtraGameInfo [1] &&*/ extraGameInfo [0].bAutoDownload))
	return 0;
memset (m, 0, sizeof (m));
ADD_TEXT (0, "", 0);
ADD_GAUGE (PROGRESS_ITEM, "                    ", 0, 100);
sprintf (szProgress, "0%c done", '%');
ADD_TEXT (PERCENT_ITEM, szProgress, 0);
m [PERCENT_ITEM].x = (short) 0x8000;
m [PERCENT_ITEM].centered = 1;
if (!RequestUpload (PID_DL_START, 0))
	return 0;
dlResult = 1;
dlState = PID_DL_OPEN;
nTimeout = SDL_GetTicks ();
sprintf (szTitle, "Downloading <%s>", pszMission);
*gameFolders.szMsnSubDir = '\0';
do {
	i = ExecMenu2 (NULL, szTitle, 3, m, DownloadPoll, 0, NULL);
	} while (i >= 0);
CFClose (&cf);
dlState = PID_DL_END;
return (i == -3);
}

//------------------------------------------------------------------------------
// eof
