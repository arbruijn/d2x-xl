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

#include "descent.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "ipx.h"
#include "ipx_udp.h"
#include "menu.h"
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

extern int32_t WhoIsGameHost(), NetworkHowManyConnected(), GetMyNetRanking();
extern char bPauseableMenu;
//char *NetworkModeNames[]={"Anarchy", "Team Anarchy", "Robo Anarchy", "Cooperative", "Capture the Flag", "Hoard", "Team Hoard", "Unknown"};

void DoShowNetGameHelp (void)
{
	CMenu		m (30);
   char		szText [MENU_MAX_TEXTLEN];
	int32_t		i, eff;
#if DBG
	int32_t		nLost;
#endif
	//char *eff_strings[]={"trashing", "really hurting", "seriously affecting", "hurting", "affecting", "tarnishing"};

sprintf (szText, TXT_INFO_GAME, netGameInfo.m_info.szGameName);
m.AddText ("", szText);
sprintf (szText, TXT_INFO_MISSION, netGameInfo.m_info.szMissionTitle);
m.AddText ("", szText);
sprintf (szText, TXT_INFO_LEVEL, netGameInfo.m_info.GetLevel ());
m.AddText ("", szText);
sprintf (szText, TXT_INFO_SKILL, MENU_DIFFICULTY_TEXT (netGameInfo.m_info.difficulty));
m.AddText ("", szText);
sprintf (szText, TXT_INFO_MODE, GT (537 + netGameInfo.m_info.gameMode));
m.AddText ("", szText);
sprintf (szText, TXT_INFO_SERVER, PLAYER (WhoIsGameHost()).callsign);
m.AddText ("", szText);
sprintf (szText, TXT_INFO_PLRNUM, NetworkHowManyConnected (), netGameInfo.m_info.nMaxPlayers);
m.AddText ("", szText);
sprintf (szText, TXT_INFO_PPS, netGameInfo.GetMinPPS ());
m.AddText ("", szText);
sprintf (szText, TXT_INFO_SHORTPKT, netGameInfo.GetShortPackets () ? "Yes" : "No");
m.AddText ("", szText);
#if DBG
nLost = (int32_t) ((double (networkData.nTotalMissedPackets) / double (networkData.nTotalPacketsGot + networkData.nTotalMissedPackets)) * 100.0);
if (nLost < 0)
	nLost = 0;
sprintf (szText, TXT_INFO_LOSTPKT, networkData.nTotalMissedPackets, nLost);
m.AddText ("", szText);
#endif
if (netGameInfo.GetScoreGoal ())
	sprintf (szText, TXT_INFO_SCOREGOAL, netGameInfo.GetScoreGoal () * 5);
sprintf (szText, " ");
sprintf (szText, TXT_INFO_PLRSCONN);
NETPLAYER (N_LOCALPLAYER).rank = GetMyNetRanking ();
for (i = 0; i < N_PLAYERS; i++)
	if (PLAYER (i).connected) {
		if (!gameOpts->multi.bNoRankings) {
			if (i == N_LOCALPLAYER)
				sprintf (szText, "%s%s (%d/%d)",
							pszRankStrings [NETPLAYER (i).rank],
							PLAYER (i).callsign,
							networkData.nNetLifeKills,
							networkData.nNetLifeKilled);
			else
				sprintf (szText, "%s%s %d/%d",
							pszRankStrings[NETPLAYER (i).rank],
							PLAYER (i).callsign,
							gameData.multigame.score.matrix[N_LOCALPLAYER][i],
							gameData.multigame.score.matrix[i][N_LOCALPLAYER]);
			}
		else
			sprintf (szText, "%s", PLAYER (i).callsign);
		m.AddText ("", szText);
		}
	m.AddText ("", "");
	eff = (int32_t)((double)((double) networkData.nNetLifeKills / ((double) networkData.nNetLifeKilled + (double) networkData.nNetLifeKills))*100.0);
	if (eff < 0)
		eff = 0;
	if (IsHoardGame) {
		if (gameData.scoreData.nChampion == -1)
			sprintf (szText, TXT_NO_RECORD2);
		else
			sprintf (szText, TXT_RECORD3, PLAYER (gameData.scoreData.nChampion).callsign, gameData.scoreData.nHighscore);
		}
	else if (!gameOpts->multi.bNoRankings) {
		sprintf (szText, TXT_EFF_LIFETIME, eff);
	m.AddText ("", szText);
	if (eff < 60)
		sprintf (szText, TXT_EFF_INFLUENCE, GT(546 + eff / 10));
	else
		sprintf (szText, TXT_EFF_SERVEWELL);
	m.AddText ("", szText);
	}
//paletteManager.SuspendEffect();
bPauseableMenu = 1;
m.TinyMenu (NULL, "NetGame Information");
//paletteManager.ResumeEffect ();
}

//------------------------------------------------------------------------------

