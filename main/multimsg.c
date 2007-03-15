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

extern fix xStartingShields;
fix xPingReturnTime;

extern void NetworkSendPing (ubyte);
extern int NetworkWhoIsMaster ();
extern char bNameReturning;

void network_dump_appletalk_player (ubyte node, ushort net, ubyte socket, int why);

//-----------------------------------------------------------------------------

void MultiSendMessage (void)
{
	int bufP = 0;

if (multiData.msg.nReceiver != -1) {
	multiData.msg.buf [bufP++] = (char)MULTI_MESSAGE;            
	multiData.msg.buf [bufP++] = (char)gameData.multi.nLocalPlayer;                       
	strncpy (multiData.msg.buf + bufP, multiData.msg.szMsg, MAX_MESSAGE_LEN); 
	bufP += MAX_MESSAGE_LEN;
	multiData.msg.buf [bufP-1] = '\0';
	MultiSendData (multiData.msg.buf, bufP, 0);
	multiData.msg.nReceiver = -1;
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
	
char *colon = strrchr (multiData.msg.szMsg, ':');
if (!colon)
	return 0;
l = (int) (colon - multiData.msg.szMsg);
if (!l || (l > CALLSIGN_LEN))
	return 0;
sprintf (szFeedbackResult, "%s ", TXT_MESSAGE_SENT_TO);
if ((gameData.app.nGameMode & GM_TEAM) && (atoi (multiData.msg.szMsg) > 0) && 
	 (atoi (multiData.msg.szMsg) < 3)) {
	sprintf (szFeedbackResult+strlen (szFeedbackResult), "%s '%s'", 
				TXT_TEAM, netGame.team_name [atoi (multiData.msg.szMsg)-1]);
	bFound = 1;
	}
if (!bFound)
	if (gameData.app.nGameMode & GM_TEAM) {
		for (i = 0; i < gameData.multi.nPlayers; i++) {
			if (!strnicmp (netGame.team_name [i], multiData.msg.szMsg, l)) {
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
	for (i = 0; i < gameData.multi.nPlayers; i++) {
		if ((!strnicmp (gameData.multi.players [i].callsign, multiData.msg.szMsg, l)) && 
			(i != gameData.multi.nLocalPlayer) && 
			(gameData.multi.players [i].connected)) {
			if (bFound)
				strcat (szFeedbackResult, ", ");
			bFound++;
			if (!(bFound % 4))
				strcat (szFeedbackResult, "\n");
			sprintf (szFeedbackResult+strlen (szFeedbackResult), "%s", 
						gameData.multi.players [i].callsign);
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
if (!multiData.msg.szMacro [key][0]) {
	HUDInitMessage (TXT_NO_MACRO);
	return;
	}
strcpy (multiData.msg.szMsg, multiData.msg.szMacro [key]);
multiData.msg.nReceiver = 100;
HUDInitMessage ("%s '%s'", TXT_SENDING, multiData.msg.szMsg);
MultiMessageFeedback ();
}


//-----------------------------------------------------------------------------

void MultiDoStartTyping (char *buf)
{
gameStates.multi.bPlayerIsTyping [buf [1]] = 1;
}

//-----------------------------------------------------------------------------

void MultiDoQuitTyping (char *buf)
{
gameStates.multi.bPlayerIsTyping [buf [1]] = 0;
}

//-----------------------------------------------------------------------------

int nTypingTimeout = 0;

void MultiSendTyping (void)
{
if (gameStates.multi.bPlayerIsTyping [gameData.multi.nLocalPlayer]) {
	int t = gameStates.app.nSDLTicks;
	if (t - nTypingTimeout > 1000) {
		nTypingTimeout = t;
		multiData.msg.buf [0] = (char) MULTI_START_TYPING;
		multiData.msg.buf [1] = (char) gameData.multi.nLocalPlayer; 
		multiData.msg.buf [2] = multiData.msg.bSending;
		MultiSendData (multiData.msg.buf, 3, 0);
		}
	}
}

//-----------------------------------------------------------------------------

void MultiSendMsgStart (char nMsg)
{
if (gameData.app.nGameMode&GM_MULTI) {
	if (nMsg > 0)
		multiData.msg.bDefining = nMsg;
	else
		multiData.msg.bSending = -nMsg;
	multiData.msg.nIndex = 0;
	multiData.msg.szMsg [multiData.msg.nIndex] = 0;
	gameStates.multi.bPlayerIsTyping [gameData.multi.nLocalPlayer] = 1;
	nTypingTimeout = 0;
	MultiSendTyping ();
	}
}

//-----------------------------------------------------------------------------

void MultiSendMsgQuit ()
{
multiData.msg.bSending = 0;
multiData.msg.bDefining = 0;
multiData.msg.nIndex = 0;
gameStates.multi.bPlayerIsTyping [gameData.multi.nLocalPlayer] = 0;
multiData.msg.buf [0] = (char) MULTI_QUIT_TYPING;
multiData.msg.buf [1] = (char) gameData.multi.nLocalPlayer; 
multiData.msg.buf [2] = 0;
MultiSendData (multiData.msg.buf, 3, 0);
}

//-----------------------------------------------------------------------------

int KickPlayer (int bBan)
{
	int i, name_index = 5 - bBan;
	char *pszKick = GT (589 + bBan);

if (strlen (multiData.msg.szMsg) > 5)
	while (multiData.msg.szMsg [name_index] == ' ')
		name_index++;

if (!NetworkIAmMaster ()) {
	HUDInitMessage (TXT_KICK_RIGHTS, gameData.multi.players [NetworkWhoIsMaster ()].callsign, pszKick);
	MultiSendMsgQuit ();
	return 1;
	}
if (strlen (multiData.msg.szMsg) <= (size_t) name_index) {
	HUDInitMessage (TXT_KICK_NAME, pszKick);
	MultiSendMsgQuit ();
	return 1;
	}

if (multiData.msg.szMsg [name_index] == '#' && isdigit (multiData.msg.szMsg [name_index+1])) {
	int players [MAX_NUM_NET_PLAYERS];
	int listpos = multiData.msg.szMsg [name_index+1] - '0';

	if (multiData.kills.bShowList == 1 || multiData.kills.bShowList == 2) {
		if (listpos == 0 || listpos  >= gameData.multi.nPlayers) {
			HUDInitMessage (TXT_KICK_PLR, pszKick);
			MultiSendMsgQuit ();
			return 1;
			}
		MultiGetKillList (players);
		i = players [listpos];
		if ((i != gameData.multi.nLocalPlayer) && (gameData.multi.players [i].connected))
			goto kick_player;
		}
	else 
		HUDInitMessage (TXT_KICK_NUMBER, pszKick);
	MultiSendMsgQuit ();
	return 1;
	}

for (i = 0; i < gameData.multi.nPlayers; i++)
	if ((!strnicmp (gameData.multi.players [i].callsign, &multiData.msg.szMsg [name_index], strlen (multiData.msg.szMsg)-name_index)) && (i != gameData.multi.nLocalPlayer) && (gameData.multi.players [i].connected)) {
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

		HUDInitMessage (TXT_DUMPING, gameData.multi.players [i].callsign);
		if (bBan)
			AddPlayerToBanList (gameData.multi.players [i].callsign);
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
		if (strlen (multiData.msg.szMsg) > 5)
			while (multiData.msg.szMsg [name_index] == ' ')
				name_index++;
		if (strlen (multiData.msg.szMsg) <= (size_t) name_index) {
			HUDInitMessage (TXT_PING_NAME);
			return 1;
			}
		for (i = 0; i < gameData.multi.nPlayers; i++) {
			if ((!strnicmp (gameData.multi.players [i].callsign, &multiData.msg.szMsg [name_index], strlen (multiData.msg.szMsg)-name_index)) && 
				 (i != gameData.multi.nLocalPlayer) && (gameData.multi.players [i].connected)) {
				pingStats [i].launchTime = TimerGetFixedSeconds ();
				NetworkSendPing ((ubyte) i);
				HUDInitMessage (TXT_PINGING, gameData.multi.players [i].callsign);
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
	char *mytempbuf = multiData.msg.szMsg + 9;

xStartingShields = atol (mytempbuf);
if (xStartingShields < 10) {
	xStartingShields = 10;
	sprintf (multiData.msg.szMsg, TXT_NEW_HANDICAP, gameData.multi.players [gameData.multi.nLocalPlayer].callsign, xStartingShields);
	}
else if (xStartingShields > 100) {
	sprintf (multiData.msg.szMsg, TXT_CHEAT_ALERT, gameData.multi.players [gameData.multi.nLocalPlayer].callsign);
	xStartingShields = 100;
	}
else
	sprintf (multiData.msg.szMsg, TXT_NEW_HANDICAP, gameData.multi.players [gameData.multi.nLocalPlayer].callsign, xStartingShields);
HUDInitMessage (TXT_HANDICAP_ALERT, xStartingShields);
xStartingShields = i2f (xStartingShields);
return 0;
}

//-----------------------------------------------------------------------------

int MovePlayer (void)
{
	int	i;

if ((gameData.app.nGameMode & GM_NETWORK) && (gameData.app.nGameMode & GM_TEAM)) {
	int name_index = 5;
	if (strlen (multiData.msg.szMsg) > 5)
		while (multiData.msg.szMsg [name_index] == ' ')
			name_index++;

	if (!NetworkIAmMaster ()) {
		HUDInitMessage (TXT_MOVE_RIGHTS, gameData.multi.players [NetworkWhoIsMaster ()].callsign);
		return 1;
		}
	if (strlen (multiData.msg.szMsg) <= (size_t) name_index) {
		HUDInitMessage (TXT_MOVE_NAME);
		return 1;
		}
	for (i = 0; i < gameData.multi.nPlayers; i++)
		if ((!strnicmp (gameData.multi.players [i].callsign, &multiData.msg.szMsg [name_index], strlen (multiData.msg.szMsg)-name_index)) && (gameData.multi.players [i].connected)) {
			if ((gameData.app.nGameMode & GM_CAPTURE) && (gameData.multi.players [i].flags & PLAYER_FLAGS_FLAG)) {
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
			for (t = 0;t<gameData.multi.nPlayers;t++)
				if (gameData.multi.players [t].connected)
					MultiResetObjectTexture (gameData.objs.objects + gameData.multi.players [t].nObject);

			NetworkSendNetgameUpdate ();
			sprintf (multiData.msg.szMsg, TXT_TEAMCHANGE3, gameData.multi.players [i].callsign);
			if (i == gameData.multi.nLocalPlayer) {
				HUDInitMessage (TXT_TEAMCHANGE1);
				ResetCockpit ();
				}
			else
				HUDInitMessage (TXT_TEAMCHANGE2, gameData.multi.players [i].callsign);
#endif
			break;
		}
	}
return 0;
}

//-----------------------------------------------------------------------------

void MultiSendMsgEnd ()
{
multiData.msg.nReceiver = 100;
if (!strnicmp (multiData.msg.szMsg, TXT_NAMES_OFF, 6)) {
	bNameReturning = 1-bNameReturning;
	HUDInitMessage (TXT_NAMERET, bNameReturning? TXT_NR_ACTIVE : TXT_NR_DISABLED);
	}
else if (!strnicmp (multiData.msg.szMsg, TXT_HANDICAP, 9)) {
	if (HandicapPlayer ())
		return;
	}
else if (!strnicmp (multiData.msg.szMsg, TXT_BOMBS_OFF, 7))
	netGame.DoSmartMine = 0;
else if (!(gameStates.render.cockpit.bShowPingStats || strnicmp (multiData.msg.szMsg, TXT_PING, 5))) {
	if (PingPlayer (-1))
		return;
	}
else if (!strnicmp (multiData.msg.szMsg, TXT_MOVE, 5)) {
	if (MovePlayer ())
		return;
	}
else if (!strnicmp (multiData.msg.szMsg, TXT_KICK, 5) && (gameData.app.nGameMode & GM_NETWORK)) {
	if (KickPlayer (0))
		return;
	}
else if (!strnicmp (multiData.msg.szMsg, TXT_BAN, 4) && (gameData.app.nGameMode & GM_NETWORK)) {
	if (KickPlayer (1))
		return;
	}
else
	HUDInitMessage ("%s '%s'", TXT_SENDING, multiData.msg.szMsg);
MultiSendMessage ();
MultiMessageFeedback ();
MultiSendMsgQuit ();
}

//-----------------------------------------------------------------------------

void MultiDefineMacroEnd ()
{
Assert (multiData.msg.bDefining > 0);
strcpy (multiData.msg.szMacro [multiData.msg.bDefining-1], multiData.msg.szMsg);
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
		if (multiData.msg.nIndex > 0)
			multiData.msg.nIndex--;
		multiData.msg.szMsg [multiData.msg.nIndex] = 0;
		break;

	case KEY_ENTER:
		if (multiData.msg.bSending)
			MultiSendMsgEnd ();
		else if (multiData.msg.bDefining)
			MultiDefineMacroEnd ();
		GameFlushInputs ();
		break;

	default:
		if (key > 0) {
			int ascii = KeyToASCII (key);
			if (ascii < 255) {
				if (multiData.msg.nIndex < MAX_MESSAGE_LEN-2) {
					multiData.msg.szMsg [multiData.msg.nIndex++] = ascii;
					multiData.msg.szMsg [multiData.msg.nIndex] = 0;
					}
				else if (multiData.msg.bSending) {
					int i;
					char * ptext, *pcolon;
					ptext = NULL;
					multiData.msg.szMsg [multiData.msg.nIndex++] = ascii;
					multiData.msg.szMsg [multiData.msg.nIndex] = 0;
					for (i = multiData.msg.nIndex-1; i >= 0; i--) {
						if (multiData.msg.szMsg [i] == 32) {
							ptext = &multiData.msg.szMsg [i+1];
							multiData.msg.szMsg [i] = 0;
							break;
							}
						}
					MultiSendMsgEnd ();
					if (ptext) {
						multiData.msg.bSending = 1;
						pcolon = strchr (multiData.msg.szMsg, ':');
						if (pcolon)
							strcpy (pcolon+1, ptext);
						else
							strcpy (multiData.msg.szMsg, ptext);
						multiData.msg.nIndex = (int) strlen (multiData.msg.szMsg);
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
multiData.msg.szMsg [0] = 0;             // Get rid of old contents
memset (m, 0, sizeof (m));
m [0].nType = NM_TYPE_INPUT; 
m [0].text = multiData.msg.szMsg; 
m [0].text_len = MAX_MESSAGE_LEN-1;
choice = ExecMenu (NULL, TXT_SEND_MESSAGE, 1, m, NULL, NULL);
if ((choice > -1) && (strlen (multiData.msg.szMsg) > 0)) {
	multiData.msg.nReceiver = 100;
	HUDInitMessage ("%s '%s'", TXT_SENDING, multiData.msg.szMsg);
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
i = GetTeam (gameData.multi.nLocalPlayer);
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

for (i = 0; i < gameData.multi.nPlayers; i++)
	if (!strnicmp (gameData.multi.players [i].callsign, bufP, nLen))
		return 1;
return 0;
}

//-----------------------------------------------------------------------------

static int IsMyPlayerId (char *bufP, int nLen)
{
return strnicmp (gameData.multi.players [gameData.multi.nLocalPlayer].callsign, bufP, nLen) == 0;
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
	strcpy (msgBuf + tloc, gameData.multi.players [gameData.multi.nLocalPlayer].callsign);
	strcpy (msgBuf + strlen (gameData.multi.players [gameData.multi.nLocalPlayer].callsign) + tloc, buf + bufP + tloc + 1);
	strcpy (buf + bufP, msgBuf);
	}
if (colon = strrchr (buf + bufP, ':')) {	//message may be addressed to a certain team or tPlayer
	l = (int) (colon - (buf + bufP));
	if (l && (l <= CALLSIGN_LEN) &&
		 ((IsTeamId (buf + bufP, l) && !IsMyTeamId (buf + bufP, l)) ||
		  (IsPlayerId (buf + bufP, l) && !IsMyPlayerId (buf + bufP, l))))
		return;
	}
msgBuf [0] = 1;
msgBuf [1] = 127 + 128;
msgBuf [2] = 95 + 128;
msgBuf [3] = 0 + 128;
strcpy (msgBuf + 4, gameData.multi.players [buf [1]].callsign);
t = (int) strlen (msgBuf);
msgBuf [t] = ':';
msgBuf [t+1] = ' ';
msgBuf [t+2] = 1;
msgBuf [t+3] = 63 + 128;
msgBuf [t+4] = 47 + 128;
msgBuf [t+5] = 0 + 128;
msgBuf [t+6] = 0;
DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
HUDPlayerMessage ("%s %s", msgBuf, buf+2);
}

//-----------------------------------------------------------------------------
//eof
