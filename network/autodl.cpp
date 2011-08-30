// autodl.c
// automatic level up/download

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include <time.h>
#include <string.h>

#ifdef __macosx__
# include <SDL/SDL.h>
# include <SDL/SDL_thread.h>
# include <SDL_net.h>
#else
# include <SDL.h>
# include <SDL_thread.h>
# include <SDL_net.h>
#endif
#include "descent.h"
#include "network.h"
#include "network_lib.h"
#include "cfile.h"
#include "ipx.h"
#include "key.h"
#include "menu.h"
#include "menu.h"
#include "byteswap.h"
#include "text.h"
#include "strutil.h"
#include "error.h"
#include "hogfile.h"
#include "timeout.h"
#include "autodl.h"

CDownloadManager downloadManager;

//------------------------------------------------------------------------------

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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int _CDECL_ UploadThread (void *pThreadId)
{
return downloadManager.Upload (*((int*) pThreadId));
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CDownloadManager::Init (void)
{
for (int i = 0; i < MAX_PLAYERS; i++)
	m_freeList [i] = i;
m_timeouts [0] = 1;
m_timeouts [1] = 2;
m_timeouts [2] = 3;
m_timeouts [3] = 5;
m_timeouts [4] = 10;
m_timeouts [5] = 15;
m_timeouts [6] = 20;
m_timeouts [7] = 30;
m_timeouts [8] = 45;
m_timeouts [9] = 60;
m_nPollTime = -1;
#if DBG
m_iTimeout = 5;
#else
m_iTimeout = 4;
#endif
memset (m_clients, 0, sizeof (m_clients));
m_nClients = 0;
m_socket = 0;
m_nState = DL_CONNECT;
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
m_nTimeout = m_timeouts [m_iTimeout] * 1000;
return m_iTimeout;
}

//------------------------------------------------------------------------------

void CDownloadManager::SetDownloadFlag (int nPlayer, bool bFlag)
{
for (int i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (!memcmp (&m_clients [nPlayer].addr.server, netPlayers [0].m_info.players [i].network.Server (), 4) &&
		 !memcmp (&m_clients [nPlayer].addr.node, netPlayers [0].m_info.players [i].network.Node (), 6)) {
		m_bDownloading [i] = bFlag;
		return;
		}
	}
}

//------------------------------------------------------------------------------

int CDownloadManager::FindClient (void)
{
for (int i = 0; i < m_nClients; i++)
	if (m_clients [i].nState &&
		 !memcmp (&m_clients [i].addr.server, networkData.packetSource.src_network, 4) &&
		 !memcmp (&m_clients [i].addr.node, networkData.packetSource.src_node, 6)) {
		m_clients [i].nTimeout = SDL_GetTicks ();
		return i;
		}
return -1;
}

//------------------------------------------------------------------------------

int CDownloadManager::AcceptClient (void)
{
	int	i = FindClient ();

if (i >= 0)
	RemoveClient (i);
else if (m_nClients >= MAX_PLAYERS)
	return -1;
i = m_freeList [MAX_PLAYERS - ++m_nClients];
memcpy (&m_clients [i].addr.server, networkData.packetSource.src_network, 4);
memcpy (&m_clients [i].addr.node, networkData.packetSource.src_node, 6);
SetDownloadFlag (i, 1);
m_clients [i].nTimeout = SDL_GetTicks ();
m_clients [i].nState = DL_CONNECT;
m_clients [i].cf.File () = NULL;
if (!(m_clients [i].thread = SDL_CreateThread (UploadThread, &i))) {
	RemoveClient (i);
	return -1;
	}
return i;
}

//------------------------------------------------------------------------------

int CDownloadManager::RemoveClient (int i)
{
if (i < 0)
	i = FindClient ();
if (i < 0)
	return 0;

tClient& client = m_clients [i];

SetDownloadFlag (i, 0);
if (client.thread && (client.nState != DL_DONE)) {
	client.nState = DL_CANCEL;
	do {
		G3_SLEEP (1);
	} while (client.thread && (client.nState != DL_DONE));
}
SDLNet_TCP_Close (client.socket);
client.cf.Close ();
memset (&client, 0, sizeof (client));
m_freeList [MAX_PLAYERS - m_nClients--] = i;
if (!m_nClients && m_socket) {
	SDLNet_TCP_Close (m_socket);
	m_socket = 0;
	}
return 1;
}

//------------------------------------------------------------------------------

int CDownloadManager::SendRequest (ubyte pId, ubyte pIdFn, tClient* clientP)
{
m_data [0] = pId;
m_data [1] = pIdFn;
if (pId == PID_UPLOAD) {
	if (gameStates.multi.nGameType == IPX_GAME)
		IPXSendBroadcastData (m_data, 2);
	else
		IPXSendInternetPacketData (m_data, 2, networkData.serverAddress, networkData.serverAddress + 4);
	}
else
	IPXSendInternetPacketData (m_data, 2, clientP->addr.server, clientP->addr.node.v);
return 1;
}

//------------------------------------------------------------------------------
// ask the game host for the next data packet

int CDownloadManager::RequestUpload (void)
{
m_nState = DL_CONNECT;
return SendRequest (PID_UPLOAD, PID_DL_START);
}

//------------------------------------------------------------------------------
// tell the client the game host is ready to send more data

int CDownloadManager::RequestDownload (tClient* clientP)
{
return SendRequest (PID_DOWNLOAD, PID_DL_START, clientP);
}

//------------------------------------------------------------------------------

int CDownloadManager::ConnectToClient (tClient& client)
{
if (!m_socket) {
	IPaddress ip;

	if (SDLNet_ResolveHost (&ip, NULL, UDP_BASEPORT) < 0)
		return DL_DONE;
	if (!(m_socket = SDLNet_TCP_Open (&ip)))
		return DL_DONE;
	}
RequestDownload (&client);
for (CTimeout to1 (30000), to2 (3000); !to1.Expired ();) {
	if ((client.socket = SDLNet_TCP_Accept (m_socket)))
		return DL_OPEN_HOG;
	G3_SLEEP (10);
	if (to2.Expired ())
		RequestDownload (&client);
	}
return DL_DONE;
}

//------------------------------------------------------------------------------

int CDownloadManager::ConnectToServer (void)
{
	IPaddress ip;
	char szIp [16];

if (m_socket) {
	SDLNet_TCP_Close (m_socket);
	m_socket = 0;
	}
sprintf (szIp, "%d.%d.%d.%d",
			 networkData.serverAddress [4], networkData.serverAddress [5], networkData.serverAddress [6], networkData.serverAddress [7]);
if (SDLNet_ResolveHost (&ip, szIp, UDP_BASEPORT) < 0)
	return 0;

for (CTimeout to (30000); !to.Expired (); ) {
	if ((m_socket = SDLNet_TCP_Open (&ip)))
		return 1;
	G3_SLEEP (10);
	}
return 0;
}

//------------------------------------------------------------------------------

void CDownloadManager::CleanUp (void)
{
	int	t, i = 0;
	static int nTimeout = 0;

if (m_nTimeout < 0)
	SetTimeoutIndex (-1);
if ((t = SDL_GetTicks ()) - nTimeout > m_nTimeout) {
	nTimeout = t;
	while (i < m_nClients)
		if ((int) SDL_GetTicks () - m_clients [i].nTimeout > m_nTimeout)
			RemoveClient (i);
		else
			i++;
	}
}

//------------------------------------------------------------------------------

int CDownloadManager::SendData (ubyte nIdFn, tClient& client)
{
	static CTimeout to (2);

// slow down to about 100 KB/sec
to.Throttle ();
client.data [0] = nIdFn;
return SDLNet_TCP_Send (client.socket, (void *) client.data, DL_PACKET_SIZE) == DL_PACKET_SIZE;

}

//------------------------------------------------------------------------------
// open a file on the game host

int CDownloadManager::OpenFile (tClient& client, const char *pszExt)
{
	char	szFile [FILENAME_LEN];
	int	l = (int) strlen (gameFolders.szMissionDirs [0]);

sprintf (szFile, "%s%s%s%s", 
			gameFolders.szMissionDirs [0], (l && (gameFolders.szMissionDirs [0][l-1] != '/')) ? "/" : "", 
			netGame.m_info.szMissionName, pszExt);
if (client.cf.File ())
	client.cf.Close ();
if (!client.cf.Open (szFile, "", "rb", 0))
	return 0;
client.fLen = client.cf.Length ();
sprintf (szFile, "%s%s", netGame.m_info.szMissionName, pszExt);
PUT_INTEL_INT (client.data + 1, client.fLen);
memcpy (client.data + 5, szFile, (int) strlen (szFile) + 1);
return SendData (DL_CREATE_FILE, client);
G3_SLEEP (50);
}

//------------------------------------------------------------------------------
// send a file from the game host
// returns -1: error, 0: more data to read and send, 1: complete file transmitted

int CDownloadManager::SendFile (tClient& client)
{
	int l = (int) client.fLen;

if (l > DL_PAYLOAD_SIZE)
	l = DL_PAYLOAD_SIZE;
PUT_INTEL_INT (client.data + 1, l);
if ((int) client.cf.Read (client.data + 5, 1, l) != l)
	return -1;
client.fLen -= l;
if (!SendData (DL_DATA, client))
	return -1; // error 
if (0 < client.fLen)
	return 0; // something left to send
client.cf.Close ();
return 1; // all sent
}

//------------------------------------------------------------------------------
// Initialize file upload via TCP.

int CDownloadManager::InitUpload (ubyte *data)
{
	static int semaphore = 0;

if (semaphore)
	return 0;
semaphore++;
if (!gameStates.app.bHaveSDLNet)
	return -1;
if (!extraGameInfo [0].bAutoDownload)
	return -1;
if (data [1] != PID_DL_START)
	return -1;
if (0 > AcceptClient ())
	return -1;
semaphore--;
return 0;
}

//------------------------------------------------------------------------------
// Game host sending data to client

int CDownloadManager::Upload (int nClient)
{
tClient& client = Client (nClient);

while (client.nState != DL_DONE) {
	switch (client.nState) {
		case DL_CONNECT:
			client.nState = ConnectToClient (client);
			break;

		case DL_OPEN_HOG:	// try all possible level file types
			if (OpenFile (client, ".hog") || OpenFile (client, ".rl2") || OpenFile (client, ".rdl"))
				client.nState = DL_SEND_HOG;
			else
				client.nState = DL_ERROR;
			break;

		case DL_OPEN_MSN:
			if (OpenFile (client, ".mn2") || OpenFile (client, ".msn"))
				client.nState = DL_SEND_MSN;
			else
				client.nState = DL_ERROR;
			break;

		case DL_SEND_HOG:
		case DL_SEND_MSN:
			switch (SendFile (client)) {
				case -1:
					client.nState = DL_ERROR;
					break;
				case 1:
					client.nState = (client.nState == DL_SEND_HOG) ? DL_OPEN_MSN : DL_FINISH;
					break;
				default:
					break;
				}
			break;

		case DL_CANCEL:
			client.nState = DL_DONE;
			return 0;

		case DL_FINISH:
		case DL_ERROR:
			SendData (client.nState, client);
			client.nState = DL_DONE;
			break;
		}
	}
RemoveClient (nClient);
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
m_nResult = 0;
return -1;
}

//------------------------------------------------------------------------------

int CDownloadManager::InitDownload (ubyte *data)
{
if (!gameStates.app.bHaveSDLNet)
	return -1;
if (!extraGameInfo [0].bAutoDownload)
	return -1;
if (data [1] != PID_DL_START)
	return -1;
if (m_nState != DL_CONNECT)
	return -1;
if (!ConnectToServer ())
	return -1;
m_nState = DL_CONNECTED;
return 1;
}

//------------------------------------------------------------------------------

int CDownloadManager::Download (void)
{
	static CTimeout to (1);

if (!m_socket)
	return 0;

to.Throttle ();
int h, l = 0;
do {
	if (0 >= (h = SDLNet_TCP_Recv (m_socket, m_data + l, DL_PACKET_SIZE - l)))
		return 0;
	l += h;
	} while (l < DL_PACKET_SIZE);

switch (m_nState = m_data [0]) {
	case DL_CREATE_FILE: {
		char	szDest [FILENAME_LEN];
		char	szFolder [FILENAME_LEN];
		char	szFile [2][FILENAME_LEN];
		char	szExt [FILENAME_LEN];
		char	*pszFile = reinterpret_cast<char*> (m_data + 5);

		if (strlen (pszFile) > FILENAME_LEN)
			return DownloadError (1);
		if (m_cf.File ())
			m_cf.Close ();
		if (!pszFile)
			return DownloadError (2);
		strlwr (pszFile);
		CFile::SplitPath (pszFile, NULL, szFile [0], szExt);
		CFile::SplitPath (hogFileManager.m_files.MsnHogFiles.szName, szFolder, szFile [1], NULL);
		strlwr (szFile [1]);
		for (int i = 1 + (strcmp (szFile [0], szFile [1]) == 0); i > 0; i--) {
			if (i == 2)
#if DBG
				;
#else
				sprintf (szDest, "%s/%s%s", szFolder, szFile [0], szExt);
#endif
			else
				sprintf (szDest, "%s/%s%s", gameFolders.szMissionDownloadDir, *gameFolders.szMissionDir ? "/" : "", pszFile);
			if (m_cf.Open (szDest, "", "wb", 0))
				break;
			}
		if (!m_cf.File ())
			return DownloadError (3);
		strcpy (m_files [m_nFiles++], szDest);
		m_nSrcLen = GET_INTEL_INT (m_data + 1);
		m_nProgress = 0;
		m_nDestLen = 0;
		break;
		}

	case DL_DATA: {
		int l = GET_INTEL_INT (m_data + 1);
		if (l > DL_PAYLOAD_SIZE)
			return DownloadError (1);
		if (m_cf.Write (m_data + 5, 1, l) != l)
			return DownloadError (3);
		m_nDestLen += l;
		break;
		}

	case DL_FINISH:
		return 0;

	case DL_ERROR:
	default:
		return DownloadError (1);
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

if (m_nTimeout < 0)
	SetTimeoutIndex (-1);

int t = (int) SDL_GetTicks ();

if (t - m_nPollTime > m_nTimeout) {
	menu [1].SetText ("download timed out");
	menu [1].m_bRedraw = 1;
	key = -2;
	return nCurItem;
	}

if (m_nState == DL_CONNECT) {
	if (t - m_nRequestTime > 3000) {
		if (!RequestUpload ())
			return 0;
		m_nRequestTime = t;
		}
	G3_SLEEP (10);
	NetworkListen ();
	}
else {
	m_nResult = Download ();
	if (m_nResult == -1) {
		key = -2;
		return nCurItem;
		}
	else if (m_nResult == -0) {
		key = -3;
		return nCurItem;
		}
	else if (m_nResult == 1) {
		m_nPollTime = t;
		if ((m_nState == DL_CREATE_FILE) || (m_nState == DL_DATA)) {
			if (m_nSrcLen && m_nDestLen) {
				int h = m_nDestLen * 100 / m_nSrcLen;
				if (h != m_nProgress) {
					m_nProgress = h;
					sprintf (menu [m_nOptPercentage].m_text, TXT_PROGRESS, m_nProgress, '%');
					menu [m_nOptPercentage].m_bRebuild = 1;
					h = m_nProgress;
					if (menu [m_nOptProgress].Value () != h) {
						menu [m_nOptProgress].Value () = h;
						menu [m_nOptProgress].Rebuild ();
						}
					}
				}
			}
		}
	}

key = 0;
return nCurItem;

#if 0
menu [m_nOptPercentage].SetText ("download failed");
menu [m_nOptPercentage].m_bRedraw = 1;
key = -2;
return nCurItem;
#endif
}

//------------------------------------------------------------------------------

int DownloadPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;
return downloadManager.Poll (menu, key, nCurItem);
}

//------------------------------------------------------------------------------

int CDownloadManager::DownloadMission (char *pszMission)
{
	static CTimeout to (3500);

if (!gameStates.app.bHaveSDLNet)
	return 0;

	CMenu	m (3);
	char	szTitle [30];
	char	szProgress [30];
	int	i;

Printlog (1, "trying to download mission '%s'\n", pszMission);
gameStates.multi.bTryAutoDL = 0;
#if 0
if (!(/*gameStates.app.bHaveExtraGameInfo [1] &&*/ extraGameInfo [0].bAutoDownload))
	return 0;
#endif
m.AddText ("", "");
sprintf (szProgress, "0%c done", '%');
m_nOptPercentage = m.AddText (szProgress, 0);
m [m_nOptPercentage].m_x = (short) 0x8000;	//centered
m [m_nOptPercentage].m_bCentered = 1;
m_nOptProgress = m.AddGauge ("progress bar", "                    ", -1, 100);
m_socket = 0;
m_nFiles = 0;
m_nResult = 1;
m_nState = DL_CONNECT;
m_nPollTime = SDL_GetTicks ();
m_nRequestTime = m_nPollTime - 3000;
sprintf (szTitle, "Downloading <%s>", pszMission);
*gameFolders.szMsnSubDir = '\0';
to.Throttle ();
do {
	i = m.Menu (NULL, szTitle, DownloadPoll);
	} while (i >= 0);
to.Start ();
m_cf.Close ();
if (m_socket) {
	SDLNet_TCP_Close (m_socket);
	m_socket = 0;
	}
m_nState = DL_DONE;
if (i != -3) {
	for (int i = 0; i < m_nFiles; i++)
		CFile::Delete (m_files [i], "");
	return 0;
	}
return 1;
}

//------------------------------------------------------------------------------
// eof
