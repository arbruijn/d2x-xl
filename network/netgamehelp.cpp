#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#define PATCH12

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#	include <winsock.h>
#else
#	include <sys/socket.h>
#endif

#include "inferno.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "ipx.h"
#include "ipx_udp.h"
#include "newmenu.h"
#include "key.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "menu.h"
#include "text.h"
#include "byteswap.h"
#include "netmisc.h"
#include "kconfig.h"
#include "autodl.h"
#include "tracker.h"
#include "gamefont.h"
#include "menubackground.h"

//------------------------------------------------------------------------------

extern int NetworkWhoIsMaster(), NetworkHowManyConnected(), GetMyNetRanking();
extern char bPauseableMenu;
//char *NetworkModeNames[]={"Anarchy", "Team Anarchy", "Robo Anarchy", "Cooperative", "Capture the Flag", "Hoard", "Team Hoard", "Unknown"};

void DoShowNetgameHelp (void)
{
	CMenu		m (30);
   char		mTexts [30][60];
	int		i, nTexts = 0, eff;
#if DBG
	int pl;
#endif
	//char *eff_strings[]={"trashing", "really hurting", "seriously affecting", "hurting", "affecting", "tarnishing"};

for (i = 0; i < 30; i++)
	m.AddText (mTexts [i]);

sprintf (mTexts [nTexts++], TXT_INFO_GAME, netGame.szGameName);
sprintf (mTexts [nTexts++], TXT_INFO_MISSION, netGame.szMissionTitle);
sprintf (mTexts [nTexts++], TXT_INFO_LEVEL, netGame.nLevel);
sprintf (mTexts [nTexts++], TXT_INFO_SKILL, MENU_DIFFICULTY_TEXT (netGame.difficulty));
sprintf (mTexts [nTexts++], TXT_INFO_MODE, GT (537 + netGame.gameMode));
sprintf (mTexts [nTexts++], TXT_INFO_SERVER, gameData.multiplayer.players [NetworkWhoIsMaster()].callsign);
sprintf (mTexts [nTexts++], TXT_INFO_PLRNUM, NetworkHowManyConnected(), netGame.nMaxPlayers);
sprintf (mTexts [nTexts++], TXT_INFO_PPS, netGame.nPacketsPerSec);
sprintf (mTexts [nTexts++], TXT_INFO_SHORTPKT, netGame.bShortPackets ? "Yes" : "No");
#if DBG
pl=(int) (((double) networkData.nTotalMissedPackets / (double) networkData.nTotalPacketsGot) * 100.0);
if (pl < 0)
	pl = 0;
sprintf (mTexts [nTexts++], TXT_INFO_LOSTPKT, networkData.nTotalMissedPackets, pl);
#endif
if (netGame.KillGoal)
	sprintf (mTexts [nTexts++], TXT_INFO_KILLGOAL, netGame.KillGoal*5); 
sprintf (mTexts [nTexts++], " "); 
sprintf (mTexts [nTexts++], TXT_INFO_PLRSCONN); 
netPlayers.players [gameData.multiplayer.nLocalPlayer].rank = GetMyNetRanking();
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].connected) {		  
		if (!gameOpts->multi.bNoRankings) {
			if (i == gameData.multiplayer.nLocalPlayer)
				sprintf (mTexts [nTexts++], "%s%s (%d/%d)", 
							pszRankStrings [netPlayers.players [i].rank], 
							gameData.multiplayer.players [i].callsign, 
							networkData.nNetLifeKills, 
							networkData.nNetLifeKilled); 
			else
				sprintf (mTexts [nTexts++], "%s%s %d/%d", 
							pszRankStrings[netPlayers.players [i].rank], 
							gameData.multiplayer.players [i].callsign, 
							gameData.multigame.kills.matrix[gameData.multiplayer.nLocalPlayer][i], 
							gameData.multigame.kills.matrix[i][gameData.multiplayer.nLocalPlayer]); 
			}
		else
			sprintf (mTexts[nTexts++], "%s", gameData.multiplayer.players [i].callsign); 
		}
	sprintf (mTexts [nTexts++], " "); 
	eff = (int)((double)((double) networkData.nNetLifeKills / ((double) networkData.nNetLifeKilled + (double) networkData.nNetLifeKills))*100.0);
	if (eff < 0)
		eff = 0;
	if (gameData.app.nGameMode & GM_HOARD) {
		if (gameData.score.nPhallicMan == -1)
			sprintf (mTexts[nTexts++], TXT_NO_RECORD2); 
		else
			sprintf (mTexts [nTexts++], TXT_RECORD3, gameData.multiplayer.players [gameData.score.nPhallicMan].callsign, gameData.score.nPhallicLimit); 
		}
	else if (!gameOpts->multi.bNoRankings) {
		sprintf (mTexts [nTexts++], TXT_EFF_LIFETIME, eff); 
	if (eff < 60)
		sprintf (mTexts [nTexts++], TXT_EFF_INFLUENCE, GT(546 + eff / 10)); 
	else
	sprintf (mTexts [nTexts++], TXT_EFF_SERVEWELL);
	}  
paletteManager.SaveEffectAndReset();
bPauseableMenu = 1;
m.TinyMenu (NULL, "Netgame Information");
paletteManager.LoadEffect ();
}

//------------------------------------------------------------------------------

