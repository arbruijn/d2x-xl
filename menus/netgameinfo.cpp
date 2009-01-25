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

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

#define AGI	activeNetGames [choice]
#define AXI activeExtraGameInfo [choice]

/* the following are the possible packet identificators.
 * they are stored in the "nType" field of the packet structs.
 * they are offset 4 bytes from the beginning of the raw IPX data
 * because of the "driver's" ipx_packetnum (see linuxnet.c).
 */

void SetFunctionMode (int);
extern ubyte ipx_MyAddress [10];

//------------------------------------------------------------------------------

extern tNetgameInfo activeNetGames [];
extern tExtraGameInfo activeExtraGameInfo [];

char szHighlight [] = {1, (char) 255, (char) 192, (char) 128, 0};

#define FLAGTEXT(_b)	((_b) ? TXT_ON : TXT_OFF)

#define	INITFLAGS(_t) \
		 {sprintf (mTexts [opt], _t); strcat (mTexts [opt], szHighlight); j = 0;}

#define	ADDFLAG(_f,_t) \
	if (_f) {if (j) strcat (mTexts [opt], ", "); strcat (mTexts [opt], _t); if (++j == 5) {opt++; INITFLAGS ("   ")}; }

//------------------------------------------------------------------------------

void ShowNetGameInfo (int choice)
 {
	CMenu	m (30);
   char	mTexts [30][200];
	int	i, j, nInMenu, opt = 0;

#if !DBG
if (choice >= networkData.nActiveGames)
	return;
#endif
memset (mTexts, 0, sizeof (mTexts));
for (i = 0; i < 20; i++)
	m.AddText (reinterpret_cast<char*> (mTexts + i));
sprintf (mTexts [opt], TXT_NGI_GAME, szHighlight, AGI.szGameName); 
opt++;
sprintf (mTexts [opt], TXT_NGI_MISSION, szHighlight, AGI.szMissionTitle); 
opt++;
sprintf (mTexts [opt], TXT_NGI_LEVEL, szHighlight, AGI.nLevel); 
opt++;
sprintf (mTexts [opt], TXT_NGI_SKILL, szHighlight, MENU_DIFFICULTY_TEXT (AGI.difficulty)); 
opt++;
opt++;
#if !DBG
if (!*AXI.szGameName) {
	sprintf (mTexts [opt], "Gamehost is not using D2X-XL or running in pure mode");
	opt++;
	}
else 
#endif
 {
	if (AXI.bShadows || AXI.bUseParticles || AXI.bBrightObjects || (!AXI.bCompetition && AXI.bUseLightnings)) {
		INITFLAGS ("Graphics Fx: "); 
		ADDFLAG (AXI.bShadows, "Shadows");
		ADDFLAG (AXI.bUseParticles, "Smoke");
		if (!AXI.bCompetition)
			ADDFLAG (AXI.bUseLightnings, "Lightnings");
		ADDFLAG (AXI.bBrightObjects, "Bright Objects");
		}
	else
		strcpy (mTexts [opt], "Graphics Fx: None");
	opt++;
	if (!AXI.bCompetition && (AXI.bLightTrails || AXI.bShockwaves || AXI.bTracers)) {
		INITFLAGS ("Weapon Fx: ");
		ADDFLAG (AXI.bLightTrails, "Light trails");
		ADDFLAG (AXI.bShockwaves, "Shockwaves");
		ADDFLAG (AXI.bTracers, "Tracers");
		ADDFLAG (AXI.bShowWeapons, "Weapons");
		}
	else
		sprintf (mTexts [opt], "Weapon Fx: None");
	opt++;
	if (!AXI.bCompetition && (AXI.bDamageExplosions || AXI.bPlayerShield)) {
		INITFLAGS ("Ship Fx: ");
		ADDFLAG (AXI.bPlayerShield, "Shield");
		ADDFLAG (AXI.bDamageExplosions, "Damage");
		ADDFLAG (AXI.bGatlingSpeedUp, "Gatling speedup");
		}
	else
		sprintf (mTexts [opt], "Ship Fx: None");
	opt++;
	if (AXI.nWeaponIcons || (!AXI.bCompetition && (AXI.bTargetIndicators || AXI.bDamageIndicators))) {
		INITFLAGS ("HUD extensions: ");
		ADDFLAG (AXI.nWeaponIcons != 0, "Icons");
		ADDFLAG (!AXI.bCompetition && AXI.bTargetIndicators, "Tgt indicators");
		ADDFLAG (!AXI.bCompetition && AXI.bDamageIndicators, "Dmg indicators");
		ADDFLAG (!AXI.bCompetition && AXI.bMslLockIndicators, "Trk indicators");
		}
	else
		strcat (mTexts [opt], "HUD extensions: None");
	opt++;
	if (!AXI.bCompetition && AXI.bRadarEnabled) {
		INITFLAGS ("Radar: ");
		ADDFLAG ((AGI.gameFlags & NETGAME_FLAG_SHOW_MAP) != 0, "Players");
		ADDFLAG (AXI.nRadar, "Radar");
		ADDFLAG (AXI.bPowerupsOnRadar, "Powerups");
		ADDFLAG (AXI.bRobotsOnRadar, "Robots");
		}
	else
		strcat (mTexts [opt], "Radar: off");
	opt++;
	if (!AXI.bCompetition && (AXI.bMouseLook || AXI.bFastPitch)) {
		INITFLAGS ("Controls ext.: ");
		ADDFLAG (AXI.bMouseLook, "mouselook");
		ADDFLAG (AXI.bFastPitch, "fast pitch");
		}
	else
		strcat (mTexts [opt], "Controls ext.: None");
	opt++;
	if (!AXI.bCompetition && 
		 (AXI.bDualMissileLaunch || !AXI.bFriendlyFire || AXI.bInhibitSuicide || 
		  AXI.bEnableCheats || AXI.bDarkness || (AXI.nFusionRamp != 2))) {
		INITFLAGS ("Gameplay ext.: ");
		ADDFLAG (AXI.bEnableCheats, "Cheats");
		ADDFLAG (AXI.bDarkness, "Darkness");
		ADDFLAG (AXI.bSmokeGrenades, "Smoke Grens");
		ADDFLAG (AXI.bDualMissileLaunch, "Dual Msls");
		ADDFLAG (AXI.nFusionRamp != 2, "Fusion ramp");
		ADDFLAG (!AXI.bFriendlyFire, "no FF");
		ADDFLAG (AXI.bInhibitSuicide, "no suicide");
		ADDFLAG (AXI.bKillMissiles, "shoot msls");
		ADDFLAG (AXI.bTripleFusion, "tri fusion");
		ADDFLAG (AXI.bEnhancedShakers, "enh shakers");
		ADDFLAG (AXI.nHitboxes, "hit boxes");
		}
	else
		strcat (mTexts [opt], "Gameplay ext.: None");
	opt++;
	}
bAlreadyShowingInfo = 1;
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
gameStates.menus.bNoBackground = 0;
m.TinyMenu (NULL, TXT_NETGAME_INFO);
gameStates.menus.bNoBackground = 0;
gameStates.app.bGameRunning = 0;
gameStates.menus.nInMenu = nInMenu;
bAlreadyShowingInfo = 0;
 }

//------------------------------------------------------------------------------

