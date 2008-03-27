#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "inferno.h"
#include "u_mem.h"
#include "timer.h"
#include "strutil.h"
#include "network.h"
#include "multi.h"
#include "object.h"
#include "error.h"
#include "powerup.h"
#include "bm.h"
#include "sounds.h"
#include "kconfig.h"
#include "config.h"
#include "textures.h"
#include "byteswap.h"
#include "sounds.h"
#include "args.h"
#include "cfile.h"
#include "effects.h"
#include "hudmsg.h"
#include "key.h"
#include "banlist.h"
#include "text.h"
#include "playsave.h"
#include "multimsg.h"

//-----------------------------------------------------------------------------

fix xPingReturnTime;

extern void NetworkSendPing (ubyte);
extern int NetworkWhoIsMaster ();
extern char bNameReturning;

void network_dump_appletalk_player (ubyte node, ushort net, ubyte socket, int why);

//-----------------------------------------------------------------------------

void MultiSendMessage (void)
{
	int bufP = 0;

if (gameData.multigame.msg.nReceiver != -1) {
	gameData.multigame.msg.buf [bufP++] = (char)MULTI_MESSAGE;            
	gameData.multigame.msg.buf [bufP++] = (char)gameData.multiplayer.nLocalPlayer;                       
	strncpy (gameData.multigame.msg.buf + bufP, gameData.multigame.msg.szMsg, MAX_MESSAGE_LEN); 
	bufP += MAX_MESSAGE_LEN;
	gameData.multigame.msg.buf [bufP-1] = '\0';
	MultiSendData (gameData.multigame.msg.buf, bufP, 0);
	gameData.multigame.msg.nReceiver = -1;
	}
}

//-----------------------------------------------------------------------------

void MultiDefineMacro (int key)
{
int nMsg = 0;
#ifndef _DEBUG
if (!(gameOpts->multi.bUseMacros && (gameData.app.nGameMode & GM_MULTI)))
	return;
#endif
key &= (~KEY_SHIFTED);
switch (key) {
	case KEY_F9:
		nMsg = 1; 
		break;
	case KEY_F10:
		nMsg = 2; 
		break;
	case KEY_F11:
		nMsg = 3; 
		break;
	case KEY_F12:
		nMsg = 4; 
		break;
	default:
		Int3 ();
	}
if (nMsg)  
	MultiSendMsgStart ((char) nMsg);
}

//-----------------------------------------------------------------------------

char szFeedbackResult [200];

int MultiMessageFeedback (void)
{
	int bFound = 0;
	int i, l;

char *colon = strrchr (gameData.multigame.msg.szMsg, ':');
if (!colon)
	return 0;
l = (int) (colon - gameData.multigame.msg.szMsg);
if (!l || (l > CALLSIGN_LEN))
	return 0;
sprintf (szFeedbackResult, "%s ", TXT_MESSAGE_SENT_TO);
if ((gameData.app.nGameMode & GM_TEAM) && (atoi (gameData.multigame.msg.szMsg) > 0) && 
	 (atoi (gameData.multigame.msg.szMsg) < 3)) {
	sprintf (szFeedbackResult+strlen (szFeedbackResult), "%s '%s'", 
				TXT_TEAM, netGame.team_name [atoi (gameData.multigame.msg.szMsg)-1]);
	bFound = 1;
	}
if (!bFound)
	if (gameData.app.nGameMode & GM_TEAM) {
		for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
			if (!strnicmp (netGame.team_name [i], gameData.multigame.msg.szMsg, l)) {
				if (bFound)
					strcat (szFeedbackResult, ", ");
				bFound++;
				if (!(bFound % 4))
					strcat (szFeedbackResult, "\n");
				sprintf (szFeedbackResult+strlen (szFeedbackResult), "%s '%s'", 
							TXT_TEAM, netGame.team_name [i]);
				}
			}
		}
if (!bFound)
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if ((!strnicmp (gameData.multiplayer.players [i].callsign, gameData.multigame.msg.szMsg, l)) && 
			(i != gameData.multiplayer.nLocalPlayer) && 
			(gameData.multiplayer.players [i].connected)) {
			if (bFound)
				strcat (szFeedbackResult, ", ");
			bFound++;
			if (!(bFound % 4))
				strcat (szFeedbackResult, "\n");
			sprintf (szFeedbackResult+strlen (szFeedbackResult), "%s", 
						gameData.multiplayer.players [i].callsign);
			}
		}
if (!bFound)
	strcat (szFeedbackResult, TXT_NOBODY);
else
	strcat (szFeedbackResult, ".");
DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
Assert (strlen (szFeedbackResult) < 200);
HUDInitMessage (szFeedbackResult);
return 1;
}

//-----------------------------------------------------------------------------

void MultiSendMacro (int key)
{
if (!(gameOpts->multi.bUseMacros && (gameData.app.nGameMode & GM_MULTI)))
	return;
switch (key) {
	case KEY_F9:
		key = 0; 
		break;
	case KEY_F10:
		key = 1; 
		break;
	case KEY_F11:
		key = 2; 
		break;
	case KEY_F12:
		key = 3; 
		break;
	default:
		Int3 ();
	}
if (!gameData.multigame.msg.szMacro [key][0]) {
	HUDInitMessage (TXT_NO_MACRO);
	return;
	}
strcpy (gameData.multigame.msg.szMsg, gameData.multigame.msg.szMacro [key]);
gameData.multigame.msg.nReceiver = 100;
HUDInitMessage ("%s '%s'", TXT_SENDING, gameData.multigame.msg.szMsg);
MultiMessageFeedback ();
}


//-----------------------------------------------------------------------------

void MultiDoStartTyping (char *buf)
{
gameStates.multi.bPlayerIsTyping [(int) buf [1]] = 1;
}

//-----------------------------------------------------------------------------

void MultiDoQuitTyping (char *buf)
{
gameStates.multi.bPlayerIsTyping [(int) buf [1]] = 0;
}

//-----------------------------------------------------------------------------

void MultiSendTyping (void)
{
if (gameStates.multi.bPlayerIsTyping [gameData.multiplayer.nLocalPlayer]) {
	int t = gameStates.app.nSDLTicks;
	if (t - gameData.multigame.nTypingTimeout > 1000) {
		gameData.multigame.nTypingTimeout = t;
		gameData.multigame.msg.buf [0] = (char) MULTI_START_TYPING;
		gameData.multigame.msg.buf [1] = (char) gameData.multiplayer.nLocalPlayer; 
		gameData.multigame.msg.buf [2] = gameData.multigame.msg.bSending;
		MultiSendData (gameData.multigame.msg.buf, 3, 0);
		}
	}
}

//-----------------------------------------------------------------------------

void MultiSendMsgStart (char nMsg)
{
if (gameData.app.nGameMode&GM_MULTI) {
	if (nMsg > 0)
		gameData.multigame.msg.bDefining = nMsg;
	else
		gameData.multigame.msg.bSending = -nMsg;
	gameData.multigame.msg.nIndex = 0;
	gameData.multigame.msg.szMsg [gameData.multigame.msg.nIndex] = 0;
	gameStates.multi.bPlayerIsTyping [gameData.multiplayer.nLocalPlayer] = 1;
	gameData.multigame.nTypingTimeout = 0;
	MultiSendTyping ();
	}
}

//-----------------------------------------------------------------------------

void MultiSendMsgQuit ()
{
gameData.multigame.msg.bSending = 0;
gameData.multigame.msg.bDefining = 0;
gameData.multigame.msg.nIndex = 0;
gameStates.multi.bPlayerIsTyping [gameData.multiplayer.nLocalPlayer] = 0;
gameData.multigame.msg.buf [0] = (char) MULTI_QUIT_TYPING;
gameData.multigame.msg.buf [1] = (char) gameData.multiplayer.nLocalPlayer; 
gameData.multigame.msg.buf [2] = 0;
MultiSendData (gameData.multigame.msg.buf, 3, 0);
}

//-----------------------------------------------------------------------------

int KickPlayer (int bBan)
{
	int i, name_index = 5 - bBan;
	char *pszKick = GT (589 + bBan);

if (strlen (gameData.multigame.msg.szMsg) > 5)
	while (gameData.multigame.msg.szMsg [name_index] == ' ')
		name_index++;

if (!NetworkIAmMaster ()) {
	HUDInitMessage (TXT_KICK_RIGHTS, gameData.multiplayer.players [NetworkWhoIsMaster ()].callsign, pszKick);
	MultiSendMsgQuit ();
	return 1;
	}
if (strlen (gameData.multigame.msg.szMsg) <= (size_t) name_index) {
	HUDInitMessage (TXT_KICK_NAME, pszKick);
	MultiSendMsgQuit ();
	return 1;
	}

if (gameData.multigame.msg.szMsg [name_index] == '#' && ::isdigit (gameData.multigame.msg.szMsg [name_index+1])) {
	int players [MAX_NUM_NET_PLAYERS];
	int listpos = gameData.multigame.msg.szMsg [name_index+1] - '0';

	if (gameData.multigame.kills.bShowList == 1 || gameData.multigame.kills.bShowList == 2) {
		if (listpos == 0 || listpos  >= gameData.multiplayer.nPlayers) {
			HUDInitMessage (TXT_KICK_PLR, pszKick);
			MultiSendMsgQuit ();
			return 1;
			}
		MultiGetKillList (players);
		i = players [listpos];
		if ((i != gameData.multiplayer.nLocalPlayer) && (gameData.multiplayer.players [i].connected))
			goto kick_player;
		}
	else 
		HUDInitMessage (TXT_KICK_NUMBER, pszKick);
	MultiSendMsgQuit ();
	return 1;
	}

for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if ((!strnicmp (gameData.multiplayer.players [i].callsign, &gameData.multigame.msg.szMsg [name_index], strlen (gameData.multigame.msg.szMsg)-name_index)) && (i != gameData.multiplayer.nLocalPlayer) && (gameData.multiplayer.players [i].connected)) {
kick_player:;
		if (gameStates.multi.nGameType  >= IPX_GAME)
			NetworkDumpPlayer (
				netPlayers.players [i].network.ipx.server, 
				netPlayers.players [i].network.ipx.node, 
				7);
		else
			network_dump_appletalk_player (
				netPlayers.players [i].network.appletalk.node, 
				netPlayers.players [i].network.appletalk.net, 
				netPlayers.players [i].network.appletalk.socket, 
				7);

		HUDInitMessage (TXT_DUMPING, gameData.multiplayer.players [i].callsign);
		if (bBan)
			AddPlayerToBanList (gameData.multiplayer.players [i].callsign);
		MultiSendMsgQuit ();
		return 1;
		}
return 0;
}

//-----------------------------------------------------------------------------

int PingPlayer (int i)
{
if (gameData.app.nGameMode & GM_NETWORK) {
	if (i  >= 0) {
		pingStats [i].launchTime = TimerGetFixedSeconds ();
		NetworkSendPing ((ubyte) i);
		MultiSendMsgQuit ();
		pingStats [i].sent++;
		}
	else {
		int name_index = 5;
		if (strlen (gameData.multigame.msg.szMsg) > 5)
			while (gameData.multigame.msg.szMsg [name_index] == ' ')
				name_index++;
		if (strlen (gameData.multigame.msg.szMsg) <= (size_t) name_index) {
			HUDInitMessage (TXT_PING_NAME);
			return 1;
			}
		for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
			if ((!strnicmp (gameData.multiplayer.players [i].callsign, &gameData.multigame.msg.szMsg [name_index], strlen (gameData.multigame.msg.szMsg)-name_index)) && 
				 (i != gameData.multiplayer.nLocalPlayer) && (gameData.multiplayer.players [i].connected)) {
				pingStats [i].launchTime = TimerGetFixedSeconds ();
				NetworkSendPing ((ubyte) i);
				HUDInitMessage (TXT_PINGING, gameData.multiplayer.players [i].callsign);
				MultiSendMsgQuit ();
				return 1;
				}
			}
		}
	}
else {// Modem/Serial ping
	pingStats [0].launchTime = TimerGetFixedSeconds ();
	MultiSendModemPing ();
	HUDInitMessage (TXT_PING_OTHER);
	MultiSendMsgQuit ();
	return 1;
	}
return 0;
}

//-----------------------------------------------------------------------------

int HandicapPlayer (void)
{
	char *mytempbuf = gameData.multigame.msg.szMsg + 9;

gameStates.gameplay.xStartingShields = atol (mytempbuf);
if (gameStates.gameplay.xStartingShields < 10) {
	gameStates.gameplay.xStartingShields = 10;
	sprintf (gameData.multigame.msg.szMsg, TXT_NEW_HANDICAP, LOCALPLAYER.callsign, gameStates.gameplay.xStartingShields);
	}
else if (gameStates.gameplay.xStartingShields > 100) {
	sprintf (gameData.multigame.msg.szMsg, TXT_CHEAT_ALERT, LOCALPLAYER.callsign);
	gameStates.gameplay.xStartingShields = 100;
	}
else
	sprintf (gameData.multigame.msg.szMsg, TXT_NEW_HANDICAP, LOCALPLAYER.callsign, gameStates.gameplay.xStartingShields);
HUDInitMessage (TXT_HANDICAP_ALERT, gameStates.gameplay.xStartingShields);
gameStates.gameplay.xStartingShields = i2f (gameStates.gameplay.xStartingShields);
return 0;
}

//-----------------------------------------------------------------------------

int MovePlayer (void)
{
	int	i;

if ((gameData.app.nGameMode & GM_NETWORK) && (gameData.app.nGameMode & GM_TEAM)) {
	int name_index = 5;
	if (strlen (gameData.multigame.msg.szMsg) > 5)
		while (gameData.multigame.msg.szMsg [name_index] == ' ')
			name_index++;

	if (!NetworkIAmMaster ()) {
		HUDInitMessage (TXT_MOVE_RIGHTS, gameData.multiplayer.players [NetworkWhoIsMaster ()].callsign);
		return 1;
		}
	if (strlen (gameData.multigame.msg.szMsg) <= (size_t) name_index) {
		HUDInitMessage (TXT_MOVE_NAME);
		return 1;
		}
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		if ((!strnicmp (gameData.multiplayer.players [i].callsign, &gameData.multigame.msg.szMsg [name_index], strlen (gameData.multigame.msg.szMsg)-name_index)) && (gameData.multiplayer.players [i].connected)) {
			if ((gameData.app.nGameMode & GM_CAPTURE) && (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_FLAG)) {
				HUDInitMessage (TXT_MOVE_FLAG);
				return 1;
				}
#if 1
			SetTeam (i, -1);
#else
#if 0
			if (netGame.teamVector & (1<<i))
				netGame.teamVector&= (~ (1<<i));
			else
				netGame.teamVector|= (1<<i);
#else
				netGame.teamVector ^= (1<<i);
#endif
			for (t = 0;t<gameData.multiplayer.nPlayers;t++)
				if (gameData.multiplayer.players [t].connected)
					MultiResetObjectTexture (gameData.objs.objects + gameData.multiplayer.players [t].nObject);

			NetworkSendNetgameUpdate ();
			sprintf (gameData.multigame.msg.szMsg, TXT_TEAMCHANGE3, gameData.multiplayer.players [i].callsign);
			if (i == gameData.multiplayer.nLocalPlayer) {
				HUDInitMessage (TXT_TEAMCHANGE1);
				ResetCockpit ();
				}
			else
				HUDInitMessage (TXT_TEAMCHANGE2, gameData.multiplayer.players [i].callsign);
#endif
			break;
		}
	}
return 0;
}

//-----------------------------------------------------------------------------

void MultiSendMsgEnd ()
{
gameData.multigame.msg.nReceiver = 100;
if (!strnicmp (gameData.multigame.msg.szMsg, TXT_NAMES_OFF, 6)) {
	bNameReturning = 1-bNameReturning;
	HUDInitMessage (TXT_NAMERET, bNameReturning? TXT_NR_ACTIVE : TXT_NR_DISABLED);
	}
else if (!strnicmp (gameData.multigame.msg.szMsg, TXT_HANDICAP, 9)) {
	if (HandicapPlayer ())
		return;
	}
else if (!strnicmp (gameData.multigame.msg.szMsg, TXT_BOMBS_OFF, 7))
	netGame.DoSmartMine = 0;
else if (!(gameStates.render.cockpit.bShowPingStats || strnicmp (gameData.multigame.msg.szMsg, TXT_PING, 5))) {
	if (PingPlayer (-1))
		return;
	}
else if (!strnicmp (gameData.multigame.msg.szMsg, TXT_MOVE, 5)) {
	if (MovePlayer ())
		return;
	}
else if (!strnicmp (gameData.multigame.msg.szMsg, TXT_KICK, 5) && (gameData.app.nGameMode & GM_NETWORK)) {
	if (KickPlayer (0))
		return;
	}
else if (!strnicmp (gameData.multigame.msg.szMsg, TXT_BAN, 4) && (gameData.app.nGameMode & GM_NETWORK)) {
	if (KickPlayer (1))
		return;
	}
else
	HUDInitMessage ("%s '%s'", TXT_SENDING, gameData.multigame.msg.szMsg);
MultiSendMessage ();
MultiMessageFeedback ();
MultiSendMsgQuit ();
}

//-----------------------------------------------------------------------------

void MultiDefineMacroEnd ()
{
Assert (gameData.multigame.msg.bDefining > 0);
strcpy (gameData.multigame.msg.szMacro [gameData.multigame.msg.bDefining-1], gameData.multigame.msg.szMsg);
WritePlayerFile ();
MultiSendMsgQuit ();
}

//-----------------------------------------------------------------------------

void MultiMsgInputSub (int key)
{
switch (key) {
	case KEY_F8:
	case KEY_ESC:
		MultiSendMsgQuit ();
		GameFlushInputs ();
		break;

	case KEY_LEFT:
	case KEY_BACKSP:
	case KEY_PAD4:
		if (gameData.multigame.msg.nIndex > 0)
			gameData.multigame.msg.nIndex--;
		gameData.multigame.msg.szMsg [gameData.multigame.msg.nIndex] = 0;
		break;

	case KEY_ENTER:
		if (gameData.multigame.msg.bSending)
			MultiSendMsgEnd ();
		else if (gameData.multigame.msg.bDefining)
			MultiDefineMacroEnd ();
		GameFlushInputs ();
		break;

	default:
		if (key > 0) {
			int ascii = KeyToASCII (key);
			if (ascii < 255) {
				if (gameData.multigame.msg.nIndex < MAX_MESSAGE_LEN-2) {
					gameData.multigame.msg.szMsg [gameData.multigame.msg.nIndex++] = ascii;
					gameData.multigame.msg.szMsg [gameData.multigame.msg.nIndex] = 0;
					}
				else if (gameData.multigame.msg.bSending) {
					int i;
					char * ptext, *pcolon;
					ptext = NULL;
					gameData.multigame.msg.szMsg [gameData.multigame.msg.nIndex++] = ascii;
					gameData.multigame.msg.szMsg [gameData.multigame.msg.nIndex] = 0;
					for (i = gameData.multigame.msg.nIndex-1; i >= 0; i--) {
						if (gameData.multigame.msg.szMsg [i] == 32) {
							ptext = &gameData.multigame.msg.szMsg [i+1];
							gameData.multigame.msg.szMsg [i] = 0;
							break;
							}
						}
					MultiSendMsgEnd ();
					if (ptext) {
						gameData.multigame.msg.bSending = 1;
						pcolon = strchr (gameData.multigame.msg.szMsg, ':');
						if (pcolon)
							strcpy (pcolon+1, ptext);
						else
							strcpy (gameData.multigame.msg.szMsg, ptext);
						gameData.multigame.msg.nIndex = (int) strlen (gameData.multigame.msg.szMsg);
						}
					}
				}
			}
	}
}

//-----------------------------------------------------------------------------

void MultiSendMsgDialog (void)
{
	tMenuItem m [1];
	int choice;

if (!(gameData.app.nGameMode & GM_MULTI))
	return;
gameData.multigame.msg.szMsg [0] = 0;             // Get rid of old contents
memset (m, 0, sizeof (m));
m [0].nType = NM_TYPE_INPUT; 
m [0].text = gameData.multigame.msg.szMsg; 
m [0].text_len = MAX_MESSAGE_LEN-1;
choice = ExecMenu (NULL, TXT_SEND_MESSAGE, 1, m, NULL, NULL);
if ((choice > -1) && (strlen (gameData.multigame.msg.szMsg) > 0)) {
	gameData.multigame.msg.nReceiver = 100;
	HUDInitMessage ("%s '%s'", TXT_SENDING, gameData.multigame.msg.szMsg);
	MultiMessageFeedback ();
	}
}

//-----------------------------------------------------------------------------

static int IsTeamId (char *bufP, int nLen)
{
	int	i;

if (!(gameData.app.nGameMode & GM_TEAM))
	return 0;
i = atoi (bufP);
if ((i >= 1) && (i <= 2))
	return 1;
for (i = 0; i < 2; i++)
	if (!strnicmp (netGame.team_name [i], bufP, nLen))
		return 1;
return 0;
}

//-----------------------------------------------------------------------------

static int IsMyTeamId (char *bufP, int nLen)
{
	int	i;

if (!(gameData.app.nGameMode & GM_TEAM))
	return 0;
i = GetTeam (gameData.multiplayer.nLocalPlayer);
if (i == atoi (bufP) - 1)
	return 1;
if (!strnicmp (netGame.team_name [i], bufP, nLen))
	return 1;
return 0;
}

//-----------------------------------------------------------------------------

static int IsPlayerId (char *bufP, int nLen)
{
	int	i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (!strnicmp (gameData.multiplayer.players [i].callsign, bufP, nLen))
		return 1;
return 0;
}

//-----------------------------------------------------------------------------

static int IsMyPlayerId (char *bufP, int nLen)
{
return strnicmp (LOCALPLAYER.callsign, bufP, nLen) == 0;
}

//-----------------------------------------------------------------------------

void MultiDoMsg (char *buf)
{
	char *colon;
	char *tilde, msgBuf [200];
	int tloc, t, l;
	int bufP = 2;

if ((tilde = strchr (buf + bufP, '$'))) { 
	tloc = (int) (tilde - (buf + bufP));			
	if (tloc > 0)
		strncpy (msgBuf, buf + bufP, tloc);
	strcpy (msgBuf + tloc, LOCALPLAYER.callsign);
	strcpy (msgBuf + strlen (LOCALPLAYER.callsign) + tloc, buf + bufP + tloc + 1);
	strcpy (buf + bufP, msgBuf);
	}
if ((colon = strrchr (buf + bufP, ':'))) {	//message may be addressed to a certain team or tPlayer
	l = (int) (colon - (buf + bufP));
	if (l && (l <= CALLSIGN_LEN) &&
		 ((IsTeamId (buf + bufP, l) && !IsMyTeamId (buf + bufP, l)) ||
		  (IsPlayerId (buf + bufP, l) && !IsMyPlayerId (buf + bufP, l))))
		return;
	}
msgBuf [0] = (char) 1;
msgBuf [1] = (char) (127 + 128);
msgBuf [2] = (char) (95 + 128);
msgBuf [3] = (char) (0 + 128);
strcpy (msgBuf + 4, gameData.multiplayer.players [(int) buf [1]].callsign);
t = (int) strlen (msgBuf);
msgBuf [t] = ':';
msgBuf [t+1] = ' ';
msgBuf [t+2] = (char) 1;
msgBuf [t+3] = (char) (95 + 128);
msgBuf [t+4] = (char) (63 + 128);
msgBuf [t+5] = (char) (0 + 128);
msgBuf [t+6] = (char) 0;
DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
HUDPlayerMessage ("%s %s", msgBuf, buf+2);
}

//-----------------------------------------------------------------------------
//eof
