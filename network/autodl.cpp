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
#include "descent.h"
#include "ipx.h"
#include "key.h"
#include "network.h"
#include "network_lib.h"
#include "menu.h"
#include "menu.h"
#include "byteswap.h"
#include "text.h"
#include "strutil.h"
#include "error.h"
#include "autodl.h"

CDownloadManager downloadManager;

//------------------------------------------------------------------------------

extern ubyte ipx_ServerAddress [10];
extern ubyte ipx_LocalAddress [10];

#if 0
static char *sznStates [] = {
	"start",
	"open file",
	"data",
	"close file",
	"end",
	"error"
	};
#endif

//------------------------------------------------------------------------------

void CDownloadManager::Init (void)
{
m_timeouts [0] = 1;
m_timeouts [1] = 2;
m_timeouts [2] = 3;
m_timeouts [3] = 5;
m_timeouts [4] = 10;
m_timeouts [5] = 15;
m_timeouts [6] = 20;
m_timeouts [7] = 30;
m_timeouts [8] = 45;
m_timeouts [8] = 60;
m_nPollTime = -1;
#if DBG
m_iTimeout = 5;
#else
m_iTimeout = 4;
#endif
memset (m_uploadDests, 0, sizeof (m_uploadDests));
m_nUploadDests = 0;
m_nPacketId = -1;
m_nPacketTimeout = -1;
m_nState = PID_DL_OPEN;
m_nResult = 1;
}

//------------------------------------------------------------------------------

int CDownloadManager::MaxTimeoutIndex (void)
{
return sizeofa (m_timeouts) - 1;
}

//------------------------------------------------------------------------------

int CDownloadManager::GetTimeoutIndex (void)
{
return m_iTimeout;
}

//------------------------------------------------------------------------------

int CDownloadManager::GetTimeoutSecs (void)
{
return m_timeouts [m_iTimeout];
}

//------------------------------------------------------------------------------

int CDownloadManager::SetTimeoutIndex (int i)
{
if ((i >= 0) && (i <= MaxTimeoutIndex ()))
	m_iTimeout = i;
m_nPollTime = m_timeouts [m_iTimeout] * 1000;
return m_iTimeout;
}

//------------------------------------------------------------------------------

void CDownloadManager::SetDownloadFlag (int nPlayer, bool bFlag)
{
for (int i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (!memcmp (&m_uploadDests [nPlayer].addr.server, &netPlayers.players [i].network.ipx.server, 4) &&
		 !memcmp (&m_uploadDests [nPlayer].addr.node, &netPlayers.players [i].network.ipx.node, 6)) {
		m_bDownloading [i] = bFlag;
		return;
		}
	}
}

//------------------------------------------------------------------------------

int CDownloadManager::FindUploadDest (void)
{
for (int i = 0; i < m_nUploadDests; i++)
	if (!memcmp (&m_uploadDests [i].addr.server, ipx_udpSrc.src_network, 4) &&
		 !memcmp (&m_uploadDests [i].addr.node, ipx_udpSrc.src_node, 6)) {
		m_uploadDests [i].nTimeout = SDL_GetTicks ();
		return i;
		}
return -1;
}

//------------------------------------------------------------------------------

int CDownloadManager::AddUploadDest (void)
{
	int	i = FindUploadDest ();

if (i >= 0) {
	if (m_uploadDests [i].nState && m_uploadDests [i].cf.File ()) {
		m_uploadDests [i].cf.Close ();
		}
	}
else {
	if (m_nUploadDests >= MAX_PLAYERS)
		return 0;
	i = m_nUploadDests++;
	memcpy (&m_uploadDests [i].addr.server, ipx_udpSrc.src_network, 4);
	memcpy (&m_uploadDests [i].addr.node, ipx_udpSrc.src_node, 6);
	SetDownloadFlag (i, 1);
	m_uploadDests [i].nTimeout = SDL_GetTicks ();
	}
m_uploadDests [i].nState = DL_OPEN_HOG;
m_uploadDests [i].cf.File () = NULL;
return 1;
}

//------------------------------------------------------------------------------

int CDownloadManager::DelUploadDest (int i)
{
if (i < 0)
	i = FindUploadDest ();
if (i < 0)
	return 0;
m_uploadDests [i].cf.Close ();
SetDownloadFlag (i, 0);
if (i < --m_nUploadDests)
	memcpy (m_uploadDests + i, m_uploadDests + i + 1, (m_nUploadDests - i) * sizeof (tUploadDest));
return 1;
}

//------------------------------------------------------------------------------

void CDownloadManager::CleanUp (void)
{
	int	t, i = 0;
	static int m_nPollTime = 0;

if (m_nPollTime < 0)
	SetTimeoutIndex (-1);
if ((t = SDL_GetTicks ()) - m_nPollTime > m_nPollTime) {
	m_nPollTime = t;
	while (i < m_nUploadDests)
		if ((int) SDL_GetTicks () - m_uploadDests [i].nTimeout > m_nPollTime)
			DelUploadDest (i);
		else
			i++;
	}
}

//------------------------------------------------------------------------------

int CDownloadManager::SendRequest (ubyte pId, ubyte pIdFn, int nSize, int nPacketId)
{
m_uploadBuf [0] = pId;
m_uploadBuf [1] = pIdFn;
if (pIdFn == PID_DL_DATA) {
	PUT_INTEL_INT (m_uploadBuf + 2, nPacketId);
	nSize += 4;
	m_nPacketTimeout = SDL_GetTicks ();
	}
if ((pId == PID_UPLOAD) && (gameStates.multi.nGameType == IPX_GAME))
	IPXSendBroadcastData (m_uploadBuf, nSize + 2);
else if (pId == PID_UPLOAD)
	IPXSendInternetPacketData (m_uploadBuf, nSize + 2, ipx_ServerAddress, ipx_ServerAddress + 4);
else
	IPXSendInternetPacketData (m_uploadBuf, nSize + 2, ipx_udpSrc.src_network, ipx_udpSrc.src_node);
return 1;
}

//------------------------------------------------------------------------------
// ask the game host for the next data packet

int CDownloadManager::RequestUpload (ubyte pIdFn, int nSize)
{
return SendRequest (PID_UPLOAD, pIdFn, nSize, m_nPacketId);
}

//------------------------------------------------------------------------------
// tell the client the game host is ready to send more data

int CDownloadManager::RequestDownload (ubyte pIdFn, int nSize, int nPacketId)
{
return SendRequest (PID_DOWNLOAD, pIdFn, nSize, nPacketId);
}

//------------------------------------------------------------------------------

void CDownloadManager::ResendRequest (void)
{
if ((m_nState == PID_DL_DATA) && (m_nPacketId > -1) && (SDL_GetTicks () - m_nPacketTimeout > 500)) 
	RequestUpload (PID_DL_DATA, 0);
}

//------------------------------------------------------------------------------

int CDownloadManager::UploadError (void)
{
DelUploadDest (-1);
RequestDownload (PID_DL_ERROR, 0, -1);
return 0;
}

//------------------------------------------------------------------------------
// open a file on the game host

int CDownloadManager::UploadOpenFile (int i, const char *pszExt)
{
	char	szFile [FILENAME_LEN];
	int	l = (int) strlen (gameFolders.szMissionDirs [0]);

sprintf (szFile, "%s%s%s%s", 
			gameFolders.szMissionDirs [0], (l && (gameFolders.szMissionDirs [0][l-1] != '/')) ? "/" : "", 
			netGame.szMissionName, pszExt);
if (m_uploadDests [i].cf.File ())
	m_uploadDests [i].cf.Close ();
if (!m_uploadDests [i].cf.Open (szFile, "", "rb", 0))
	return UploadError ();
m_uploadDests [i].fLen = m_uploadDests [i].cf.Length ();
PUT_INTEL_INT (m_uploadBuf + 2, m_uploadDests [i].fLen);
sprintf (szFile, "%s%s", netGame.szMissionName, pszExt);
l = (int) strlen (szFile) + 1;
memcpy (m_uploadBuf + 6, szFile, l);
m_uploadDests [i].nPacketId = -1;
RequestDownload (PID_DL_OPEN, l + 4, -1);
return 1;
}

//------------------------------------------------------------------------------
// send a file from the game host

int CDownloadManager::UploadSendFile (int i, int nPacketId)
{
	int	l, h = nPacketId - m_uploadDests [i].nPacketId;

if ((h < 0) || (h > 1))
	return -1;
if (!h || (m_uploadDests [i].fLen > 0)) {
	if (h) {	// send next data packet
		l = (int) m_uploadDests [i].fLen;
		if (l > 512) //DL_BUFSIZE - 6)
			l = 512; //DL_BUFSIZE - 6;
		if ((int) m_uploadDests [i].cf.Read (m_uploadBuf + 10, 1, l) != l)
			return UploadError ();
		PUT_INTEL_INT (m_uploadBuf + 2, nPacketId);
		PUT_INTEL_INT (m_uploadBuf + 6, l);
		m_uploadDests [i].nPacketId = nPacketId;
		}
	else
		l = 0;	// resend last packet
	RequestDownload (PID_DL_DATA, l + 8, nPacketId);
	m_uploadDests [i].fLen -= l;
	}
else {
	m_uploadDests [i].cf.Close ();
	RequestDownload (PID_DL_CLOSE, 0, -1);
	return -1;
	}
return 1;
}

//------------------------------------------------------------------------------
// Game host sending data to client

int CDownloadManager::Upload (ubyte *data)
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

		switch (m_uploadDests [i].nState) {
			case DL_OPEN_HOG:
				if (!(UploadOpenFile (i, ".hog") || 
						UploadOpenFile (i, ".rl2") || 
						UploadOpenFile (i, ".rdl")))
					return 0;
				m_uploadDests [i].nState = DL_SEND_HOG;
				return 1;

			case DL_SEND_HOG:
				switch (UploadSendFile (i, GET_INTEL_INT (data + 2))) {
					case 0:
						return 0;
					case -1:
						m_uploadDests [i].nState = DL_OPEN_MSN;
					case 1:
						return 1;
					}
				break;

			case DL_OPEN_MSN:
				if (!(UploadOpenFile (i, ".mn2") || UploadOpenFile (i, ".msn")))
					return 0;
				m_uploadDests [i].nState = DL_SEND_MSN;
				return 1;
				break;

			case DL_SEND_MSN:
				switch (UploadSendFile (i, GET_INTEL_INT (data + 2))) {
					case 0:
						return 0;
					case -1:
						m_uploadDests [i].nState = DL_FINISH;
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

int CDownloadManager::DownloadError (int nReason)
{
if (nReason == 1)
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_AUTODL_SYNC);
else if (nReason == 2)
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_AUTODL_MISSPKTS);
else if (nReason == 3)
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_AUTODL_FILEIO);
else
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_AUTODL_FAILED);
m_cf.Close ();
RequestUpload (PID_DL_ERROR, 0);
m_nResult = 0;
m_nState = PID_DL_OPEN;
return 0;
}

//------------------------------------------------------------------------------

int CDownloadManager::Download (ubyte *data)
{
	ubyte pId = data [1];

m_nPollTime = SDL_GetTicks ();
switch (pId) {
	case PID_DL_START:
		if (m_nState != PID_DL_START)
			return DownloadError (1);
		m_nPacketId = -1;
		RequestUpload (PID_DL_DATA, 0);
		m_nResult = 1;
		m_nState = PID_DL_OPEN;
		return 1;
		break;

	case PID_DL_ERROR:
		if (m_cf.File ()) {
			m_cf.Close ();
			m_nResult = 0;
			m_nState = PID_DL_OPEN;
			}
		return 0;

	case PID_DL_OPEN:
		if (m_nState != PID_DL_OPEN)
			return DownloadError (1);
	 {
			char	szDest [FILENAME_LEN];
			char	*pszFile = reinterpret_cast<char*> (data + 6);

		if (!pszFile)
			return DownloadError (2);
		strlwr (pszFile);
		sprintf (szDest, "%s%s%s", gameFolders.szMissionDir, *gameFolders.szMissionDir ? "/" : "", pszFile);
		if (!m_cf.Open (szDest, "", "wb", 0))
			return DownloadError (2);
		m_nSrcLen = GET_INTEL_INT (data + 2);
		m_nProgress = 0;
		m_nDestLen = 0;
		m_nPacketId = 0;
		RequestUpload (PID_DL_DATA, 0);
		m_nState = PID_DL_DATA;
		return 1;
		}

	case PID_DL_DATA:
		if (m_nState != PID_DL_DATA)
			return DownloadError (1);
	 {
			int id = GET_INTEL_INT (data + 2),
				 h = id - m_nPacketId;

			if (!h) {	// receiving the requested packet
				int l = GET_INTEL_INT (data + 6);

				if (m_cf.Write (data + 10, 1, l) != l)
					return DownloadError (2);
				m_nPacketId++;
				m_nDestLen += l;
				}
			else if (h != -1)	// host has been resending the last packet; probably due to request and reply crossing each other
				return DownloadError (3);
			}
		RequestUpload (PID_DL_DATA, 0);
		return 1;

	case PID_DL_CLOSE:
		if (m_nState != PID_DL_DATA)
			return DownloadError (1);
		if (m_cf.File ())
			m_cf.Close ();
		m_nState = PID_DL_OPEN;
		m_nPacketId = -1;
		RequestUpload (PID_DL_DATA, 0);
		return 1;

	case PID_DL_END:
		if (m_nState != PID_DL_OPEN)
			return DownloadError (1);
		m_nResult = -1;
		return -1;
	}
return 1;
}

//------------------------------------------------------------------------------

int CDownloadManager::Poll (CMenu& menu, int& key, int nCurItem)
{
if (key == KEY_ESC) {
	menu [m_nOptPercentage].SetText ("download aborted");
	menu [1].m_bRedraw = 1;
	key = -2;
	return nCurItem;
	}
ResendRequest ();
NetworkListen ();
if (m_nPollTime < 0)
	SetTimeoutIndex (-1);
if (int (SDL_GetTicks ()) - m_nPollTime > m_nTimeout) {
	menu [1].SetText ("download timed out");
	menu [1].m_bRedraw = 1;
	key = -2;
	return nCurItem;
	}
if (m_nResult == -1) {
	key = -3;
	return nCurItem;
	}
if (m_nResult == 1) {
	if ((m_nState == PID_DL_OPEN) || (m_nState == PID_DL_DATA)) {
		if (m_nSrcLen && m_nDestLen) {
			int h = m_nDestLen * 100 / m_nSrcLen;
			if (h != m_nProgress) {
				m_nProgress = h;
				sprintf (menu [m_nOptPercentage].m_text, TXT_PROGRESS, m_nProgress, '%');
				menu [m_nOptPercentage].m_bRebuild = 1;
				h = m_nProgress;
				if (menu [m_nOptProgress].m_value != h) {
					menu [m_nOptProgress].m_value = h;
					menu [m_nOptProgress].m_bRebuild = 1;
					}
				}
			}
		}
	key = 0;
	return nCurItem;
	}
menu [m_nOptPercentage].SetText ("download failed");
menu [m_nOptPercentage].m_bRedraw = 1;
key = -2;
return nCurItem;
}

//------------------------------------------------------------------------------

int DownloadPoll (CMenu& menu, int& key, int nCurItem)
{
return downloadManager.Poll (menu, key, nCurItem);
}

//------------------------------------------------------------------------------

int CDownloadManager::DownloadMission (char *pszMission)
{
	CMenu	m (3);
	char	szTitle [30];
	char	szProgress [30];
	int	i;

PrintLog ("   trying to download mission '%s'\n", pszMission);
gameStates.multi.bTryAutoDL = 0;
if (!(/*gameStates.app.bHaveExtraGameInfo [1] &&*/ extraGameInfo [0].bAutoDownload))
	return 0;
m.AddText ("", 0);
sprintf (szProgress, "0%c done", '%');
m_nOptPercentage = m.AddText (szProgress, 0);
m [m_nOptPercentage].m_x = (short) 0x8000;	//centered
m [m_nOptPercentage].m_bCentered = 1;
m_nOptProgress = m.AddGauge ("                    ", -1, 100);
if (!RequestUpload (PID_DL_START, 0))
	return 0;
m_nResult = 1;
m_nState = PID_DL_OPEN;
m_nPollTime = SDL_GetTicks ();
sprintf (szTitle, "Downloading <%s>", pszMission);
*gameFolders.szMsnSubDir = '\0';
do {
	i = m.Menu (NULL, szTitle, DownloadPoll);
	} while (i >= 0);
m_cf.Close ();
m_nState = PID_DL_END;
return (i == -3);
}

//------------------------------------------------------------------------------
// eof
