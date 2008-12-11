/* $Id: playsave.c,v 1.16 2003/11/26 12:26:33 btb Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
#	include <unistd.h>
#endif
#ifndef _WIN32_WCE
#	include <errno.h>
#endif
#include <limits.h>

#include "inferno.h"
#include "error.h"
#include "pa_enabl.h"
#include "strutil.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "game.h"
#include "loadgame.h"
#include "player.h"
#include "playsave.h"
#include "joy.h"
#include "kconfig.h"
#include "digi.h"
#include "crypt.h"
#include "newmenu.h"
#include "joydefs.h"
#include "palette.h"
#include "multi.h"
#include "menu.h"
#include "config.h"
#include "text.h"
#include "mono.h"
#include "state.h"
#include "gauges.h"
#include "screens.h"
#include "powerup.h"
#include "makesig.h"
#include "byteswap.h"
#include "escort.h"
#include "network.h"
#include "network_lib.h"
#include "weapon.h"
#include "omega.h"
#include "autodl.h"
#include "args.h"
#include "collide.h"
#include "findfile.h"
#include "u_mem.h"
#include "dynlight.h"

//------------------------------------------------------------------------------

#define SAVE_FILE_ID			MAKE_SIG('D','P','L','R')

#if defined(_WIN32_WCE)
# define errno -1
# define ENOENT -1
# define strerror(x) "Unknown Error"
#endif

int GetLifetimeChecksum (int a,int b);

typedef struct hli {
	char	shortname[9];
	ubyte	nLevel;
} hli;

short nHighestLevels;

hli highestLevels [MAX_MISSIONS];

//version 5  ->  6: added new highest level information
//version 6  ->  7: stripped out the old saved_game array.
//version 7  ->  8: added reticle flag, & window size
//version 8  ->  9: removed player_structVersion
//version 9  -> 10: added default display mode
//version 10 -> 11: added all toggles in toggle menu
//version 11 -> 12: added weapon ordering
//version 12 -> 13: added more keys
//version 13 -> 14: took out marker key
//version 14 -> 15: added guided in big window
//version 15 -> 16: added small windows in cockpit
//version 16 -> 17: ??
//version 17 -> 18: save guidebot name
//version 18 -> 19: added automap-highres flag
//version 19 -> 20: added KConfig data for windows joysticks
//version 20 -> 21: save seperate config types for DOS & Windows
//version 21 -> 22: save lifetime netstats
//version 22 -> 23: ??
//version 23 -> 24: add name of joystick for windows version.
//version 24 -> 25: add d2x keys array

void InitWeaponOrdering();

//------------------------------------------------------------------------------

typedef struct tParam {
	struct tParam	*next;
	char				*valP;
	short				nValues;
	ubyte				nSize;
	char				szTag [1];
} tParam;

tParam	*paramList = NULL;
tParam	*lastParam = NULL;

//------------------------------------------------------------------------------

void FreeParams (void)
{
	tParam	*pp;

while (paramList) {
	pp = paramList;
	paramList = paramList->next;
	D2_FREE (pp);
	}
}

//------------------------------------------------------------------------------

char *MakeTag (char *pszTag, const char *pszIdent, int i, int j)
{
	char	*pi;
	const char *ph;
	int	h = 0, l;

for (pi = pszTag; *pszIdent; h++) {
	if (!((ph = strstr (pszIdent, " [")) || (ph = strchr (pszIdent, '[')))) {
		strcpy (pi, pszIdent);
		break;
		}
	memcpy (pi, pszIdent, l = (int) (ph - pszIdent));
	sprintf (pi + l, "[%d]", h ? j : i);
	pi = pszTag + strlen (pszTag);
	strcat (pszTag, pszIdent = strchr (ph + 1, ']') + 1);
	}
return pszTag;
}

//------------------------------------------------------------------------------

int RegisterParam (void *valP, const char *pszIdent, int i, int j, ubyte nSize)
{
	char		szTag [200];
	int		l;
	tParam	*pp;

l = (int) strlen (MakeTag (szTag, pszIdent, i, j));
pp = reinterpret_cast<tParam*> (D2_ALLOC (sizeof (tParam) + l));
if (!pp)
	return 0;
memcpy (pp->szTag, szTag, l + 1);
pp->valP = reinterpret_cast<char*> (valP);
pp->nSize = nSize;
pp->nValues = 1;
pp->next = NULL;
if (lastParam)
	lastParam->next = pp;
else
	paramList = pp;
lastParam = pp;
return 1;
}

#define RP(_v,_i,_j)	RegisterParam (reinterpret_cast<void*> (&(_v)), #_v, _i, _j, sizeof (_v))

//------------------------------------------------------------------------------
// returns number of config items with identical ids before the current one

int FindConfigParam (kcItem *cfgP, int nItems, int iItem, const char *pszText)
{
	int	h, i;

for (h = i = 0; i < nItems; i++) {
	if (!strcmp (pszText, cfgP [i].text)) {
		if (i < iItem)
			h++;
		else if (i > iItem)
			return h;
		}
	}
return h ? h : -1;
}

//------------------------------------------------------------------------------

void RegisterConfigParams (kcItem *cfgP, int nItems, const char *pszId)
{
	char	szTag [200], *p;
	int	i, j = 0;

strcpy (szTag, pszId);
p = szTag + strlen (szTag);

for (i = 0; i < nItems; i++) {
#if 0
	sprintf (p, "%s.type", cfgP [i].text);
	RegisterParam (&cfgP [i].nType, szTag, 0, 0, sizeof (cfgP [i].nType));
#endif
	j = FindConfigParam (cfgP, nItems, i, cfgP [i].text);
	if (j < 0) {
		sprintf (p, "%s.value", cfgP [i].text);
		RegisterParam (&cfgP [i].value, szTag, 0, 0, sizeof (cfgP [i].value));
		}
	else {
		sprintf (p, "%s[%d].value", cfgP [i].text, j);
		RegisterParam (&cfgP [i].value, szTag, j, 0, sizeof (cfgP [i].value));
		}
	}
}

//------------------------------------------------------------------------------

void RegisterParams (void)
{
	uint	i, j;

	static int bRegistered = 0;

if (bRegistered)
	return;
bRegistered = 1;

for (i = 0; i < 2; i++) {
	if (i) {	// i == 1: nostalgia/pure D2 mode
		}
	else {
		RP (gameData.render.window.w, 0, 0);
		RP (gameData.render.window.h, 0, 0);

		RP (iDlTimeout, 0, 0);

		RP (gameStates.render.bShowFrameRate, 0, 0);
		RP (gameStates.render.bShowTime, 0, 1);
		RP (gameStates.sound.digi.nMaxChannels, 0, 16);
		RP (gameStates.render.cockpit.nMode, 0, 0);
		RP (gameStates.video.nDefaultDisplayMode, 0, 0);
		RP (gameStates.video.nDefaultDisplayMode, 0, 0);
		RP (gameOptions [i].render.cockpit.bGuidedInMainView, 0, 0);
		RP (networkData.nNetLifeKills, 0, 0);
		RP (networkData.nNetLifeKilled, 0, 0);
		RP (gameData.app.nLifetimeChecksum, 0, 0);
		for (j = 0; j < rtTaskCount; j++)
			RP (gameData.app.bUseMultiThreading [j], j, 0);
		RP (gameData.escort.szName, 0, 0);
		for (j = 0; j < 4; j++)
			RP (gameData.multigame.msg.szMacro [j], j, 0);

		RP (displayModeInfo [NUM_DISPLAY_MODES].w, NUM_DISPLAY_MODES, 0);
		RP (displayModeInfo [NUM_DISPLAY_MODES].h, NUM_DISPLAY_MODES, 0);

		RP (gameStates.app.nDifficultyLevel, 0, 0);
		RP (gameStates.ogl.nContrast, 0, 0);
		RP (gameStates.multi.nConnection, 0, 0);
		RP (gameStates.multi.bUseTracker, 0, 0);

		RP (gameData.menu.alpha, 0, 0);

		RP (mpParams.nLevel, 0, 0);
		RP (mpParams.nGameType, 0, 0);
		RP (mpParams.nGameMode, 0, 0);
		RP (mpParams.nGameAccess, 0, 0);
		RP (mpParams.bShowPlayersOnAutomap, 0, 0);
		RP (mpParams.nDifficulty, 0, 0);
		RP (mpParams.nWeaponFilter, 0, 0);
		RP (mpParams.nReactorLife, 0, 0);
		RP (mpParams.nMaxTime, 0, 0);
		RP (mpParams.nKillGoal, 0, 0);
		RP (mpParams.bInvul, 0, 0);
		RP (mpParams.bMarkerView, 0, 0);
		RP (mpParams.bIndestructibleLights, 0, 0);
		RP (mpParams.bBrightPlayers, 0, 0);
		RP (mpParams.bShowAllNames, 0, 0);
		RP (mpParams.bShortPackets, 0, 0);
		RP (mpParams.nPPS, 0, 0);
		RP (mpParams.udpClientPort, 0, 0);
		RP (mpParams.szServerIpAddr, 0, 0);

		RP (extraGameInfo [i].bAutoBalanceTeams, 0, 0);
		RP (extraGameInfo [i].bAutoDownload, 0, 0);
		RP (extraGameInfo [i].bDamageExplosions, 0, 0);
		RP (extraGameInfo [1].bDisableReactor, 0, 0);
		RP (extraGameInfo [i].bDropAllMissiles, 0, 0);
		RP (extraGameInfo [i].bEnhancedCTF, 0, 0);
		RP (extraGameInfo [i].bFixedRespawns, 0, 0);
		RP (extraGameInfo [i].bFluidPhysics, 0, 0);
		RP (extraGameInfo [i].bFriendlyFire, 0, 0);
		RP (extraGameInfo [i].bGatlingTrails, 0, 0);
		RP (extraGameInfo [i].bImmortalPowerups, 0, 0);
		RP (extraGameInfo [i].bLightTrails, 0, 0);
		RP (extraGameInfo [i].bMultiBosses, 0, 0);
		RP (extraGameInfo [i].bPowerupsOnRadar, 0, 0);
		RP (extraGameInfo [i].bPlayerShield, 0, 0);
		RP (extraGameInfo [i].bRobotsHitRobots, 0, 0);
		RP (extraGameInfo [i].bRobotsOnRadar, 0, 0);
		RP (extraGameInfo [i].bRotateLevels, 0, 0);
		RP (extraGameInfo [i].bSafeUDP, 0, 0);
		RP (extraGameInfo [i].bShadows, 0, 0);
		RP (extraGameInfo [i].bTeleporterCams, i, 0);
		RP (extraGameInfo [i].bSmartWeaponSwitch, 0, 0);
		RP (extraGameInfo [i].bSmokeGrenades, 0, 0);
		RP (extraGameInfo [i].bThrusterFlames, 0, 0);
		RP (extraGameInfo [i].bTracers, 0, 0);
		RP (extraGameInfo [i].bUseCameras, 0, 0);
		RP (extraGameInfo [i].bUseParticles, 0, 0);
		RP (extraGameInfo [i].bUseLightnings, 0, 0);
		RP (extraGameInfo [i].bUseHitAngles, 0, 0);
		RP (extraGameInfo [i].bWiggle, 0, 0);
		RP (extraGameInfo [i].bGatlingSpeedUp, i, 0);
		RP (extraGameInfo [i].bRotateMarkers, i, 0);

		RP (extraGameInfo [i].grWallTransparency, 0, 0);

		RP (extraGameInfo [i].nFusionRamp, 0, 0);
		RP (extraGameInfo [i].nOmegaRamp, 0, 0);
		RP (extraGameInfo [i].nLightRange, 0, 0);
		RP (extraGameInfo [i].nMaxSmokeGrenades, 0, 0);
		RP (extraGameInfo [i].nMslTurnSpeed, 0, 0);
		RP (extraGameInfo [i].nMslStartSpeed, 0, 0);
		RP (extraGameInfo [i].nRadar, 0, 0);
		RP (extraGameInfo [i].nDrag, 0, 0);
		RP (extraGameInfo [i].nSpawnDelay, 0, 0); // / 1000
		RP (extraGameInfo [i].nSpeedBoost, 0, 0);
		RP (extraGameInfo [i].nWeaponDropMode, 0, 0);
		RP (extraGameInfo [i].nWeaponIcons, 0, 0);
		RP (extraGameInfo [i].nZoomMode, 0, 0);
		RP (extraGameInfo [i].bShowWeapons, 0, 0);

		RP (extraGameInfo [i].entropy.nCaptureVirusLimit, 0, 0);
		RP (extraGameInfo [i].entropy.nCaptureTimeLimit, 0, 0);
		RP (extraGameInfo [i].entropy.nMaxVirusCapacity, 0, 0);
		RP (extraGameInfo [i].entropy.nBumpVirusCapacity, 0, 0);
		RP (extraGameInfo [i].entropy.nBashVirusCapacity, 0, 0);
		RP (extraGameInfo [i].entropy.nVirusGenTime, 0, 0);
		RP (extraGameInfo [i].entropy.nVirusLifespan, 0, 0);
		RP (extraGameInfo [i].entropy.nVirusStability, 0, 0);
		RP (extraGameInfo [i].entropy.nEnergyFillRate, 0, 0);
		RP (extraGameInfo [i].entropy.nShieldFillRate, 0, 0);
		RP (extraGameInfo [i].entropy.nShieldDamageRate, 0, 0);
		RP (extraGameInfo [i].entropy.bRevertRooms, 0, 0);
		RP (extraGameInfo [i].entropy.bDoConquerWarning, 0, 0);
		RP (extraGameInfo [i].entropy.nOverrideTextures, 0, 0);
		RP (extraGameInfo [i].entropy.bBrightenRooms, 0, 0);
		RP (extraGameInfo [i].entropy.bPlayerHandicap, 0, 0);

		RP (extraGameInfo [i].loadout.nGuns, 0, 0);
		RP (extraGameInfo [i].loadout.nDevices, 0, 0);

		RP (extraGameInfo [i].headlight.bAvailable, i, 0);
		RP (extraGameInfo [i].headlight.bDrainPower, 0, 0);
		RP (extraGameInfo [i].headlight.bBuiltIn, 0, 0);

		RP (extraGameInfo [i].monsterball.nBonus, 0, 0);
		RP (extraGameInfo [i].monsterball.nSizeMod, 0, 0);
		for (j = 0; j < MAX_MONSTERBALL_FORCES; j++) {
			RP (extraGameInfo [i].monsterball.forces [j].nWeaponId, 0, j);
			RP (extraGameInfo [i].monsterball.forces [j].nForce, 0, j);
			}
		for (j = 0; j < 3; j++) {
			RP (gameOptions [i].input.keyboard.bRamp [j], i, j);
			RP (gameOptions [i].input.mouse.sensitivity [j], 0, j);
			RP (gameOptions [i].input.trackIR.sensitivity [j], 0, j);
			}
		for (j = 0; j < 5; j++) {
			RP (gameOptions [i].render.particles.nDens [j], i, j);
			RP (gameOptions [i].render.particles.nSize [j], i, j);
			RP (gameOptions [i].render.particles.nLife [j], i, j);
			RP (gameOptions [i].render.particles.nAlpha [j], i, j);
			}
		for (j = 0; j < 5; j++) {
			RP (gameOptions [i].input.joystick.deadzones [j], 0, j);
			RP (gameOptions [i].input.joystick.sensitivity [j], 0, j);
			RP (gameOptions [i].input.trackIR.bMove [j], 0, j);
			}
		RP (gameOptions [i].input.bUseHotKeys, i, 0);
		RP (gameOptions [i].input.mouse.bJoystick, i, 0);
		RP (gameOptions [i].input.mouse.bSyncAxes, i, 0);
		RP (gameOptions [i].input.mouse.nDeadzone, i, 0);
		RP (gameOptions [i].input.joystick.bLinearSens, i, 0);
		RP (gameOptions [i].input.joystick.bSyncAxes, i, 0);
		RP (gameOptions [i].input.trackIR.nMode, i, 0);
		RP (gameOptions [i].input.trackIR.nDeadzone, i, 0);
		RP (gameOptions [i].input.keyboard.nRamp, i, 0);

		RP (gameOptions [i].ogl.bLightObjects, i, 0);
		RP (gameOptions [i].ogl.bHeadlight, i, 0);
		RP (gameOptions [i].ogl.bLightPowerups, i, 0);
		RP (gameOptions [i].ogl.bObjLighting, i, 0);
		RP (gameOptions [i].ogl.nMaxLightsPerFace, i, 0);
		RP (gameOptions [i].ogl.nMaxLightsPerPass, i, 0);
		RP (gameOptions [i].ogl.nMaxLightsPerObject, i, 0);
		RP (gameOptions [i].render.nLightingMethod, i, 0);
		RP (gameOptions [i].render.nDebrisLife, i, 0);

		RP (gameOptions [i].render.textures.nQuality, i, 0);
		RP (gameOptions [i].render.effects.bAutoTransparency, i, 0);
		RP (gameOptions [i].render.effects.bSoftParticles, i, 0);
		RP (gameOptions [i].render.effects.bMovingSparks, i, 0);
		RP (gameOptions [i].render.effects.bExplBlasts, i, 0);
		RP (gameOptions [i].render.effects.nShrapnels, i, 0);
		RP (gameOptions [i].render.coronas.bUse, i, 0);
		RP (gameOptions [i].render.coronas.nStyle, i, 0);
		RP (gameOptions [i].render.coronas.bShots, i, 0);
		RP (gameOptions [i].render.coronas.bPowerups, i, 0);
		RP (gameOptions [i].render.coronas.bWeapons, i, 0);
		RP (gameOptions [i].render.coronas.nIntensity, i, 0);
		RP (gameOptions [i].render.coronas.nObjIntensity, i, 0);
		RP (gameOptions [i].render.coronas.bAdditive, i, 0);
		RP (gameOptions [i].render.coronas.bAdditiveObjs, i, 0);
		RP (gameOptions [i].render.effects.bEnergySparks, i, 0);
		RP (gameOptions [i].render.bBrightObjects, i, 0);
		RP (gameOptions [i].render.effects.bRobotShields, i, 0);
		RP (gameOptions [i].render.effects.bOnlyShieldHits, 0, 0);
		RP (gameOptions [i].render.effects.bTransparent, i, 0);
		RP (gameOptions [i].render.bDepthSort, i, 0);

		RP (gameOptions [i].render.lightnings.bAuxViews, i, 0);
		RP (gameOptions [i].render.lightnings.bMonitors, i, 0);
		RP (gameOptions [i].render.lightnings.bPlasma, i, 0);
		RP (gameOptions [i].render.lightnings.bDamage, i, 0);
		RP (gameOptions [i].render.lightnings.bExplosions, i, 0);
		RP (gameOptions [i].render.lightnings.bOmega, i, 0);
		RP (gameOptions [i].render.lightnings.bRobotOmega, i, 0);
		RP (gameOptions [i].render.lightnings.bPlayers, i, 0);
		RP (gameOptions [i].render.lightnings.bRobots, i, 0);
		RP (gameOptions [i].render.lightnings.bStatic, i, 0);
		RP (gameOptions [i].render.lightnings.nStyle, i, 0);
		RP (gameOptions [i].render.lightnings.nQuality, i, 0);

		RP (gameOptions [i].render.cameras.bFitToWall, i, 0);
		RP (gameOptions [i].render.cameras.bHires, i, 0);
		RP (gameOptions [i].render.cameras.nFPS, i, 0);
		RP (gameOptions [i].render.cameras.nSpeed, i, 0);

		RP (gameOptions [i].render.automap.bBright, i, 0);
		RP (gameOptions [i].render.automap.bCoronas, i, 0);
		RP (gameOptions [i].render.automap.bGrayOut, i, 0);
		RP (gameOptions [i].render.automap.bSparks, i, 0);
		RP (gameOptions [i].render.automap.bParticles, i, 0);
		RP (gameOptions [i].render.automap.bLightnings, i, 0);
		RP (gameOptions [i].render.automap.bTextured, i, 0);
		RP (gameOptions [i].render.automap.bSkybox, i, 0);
		RP (gameOptions [i].render.automap.nColor, i, 0);
		RP (gameOptions [i].render.automap.nRange, i, 0);

		RP (gameOptions [i].render.cockpit.bMouseIndicator, i, 0);
		RP (gameOptions [i].render.cockpit.bObjectTally, i, 0);
		RP (gameOptions [i].render.cockpit.bPlayerStats, i, 0);
		RP (gameOptions [i].render.cockpit.bRotateMslLockInd, i, 0);
		RP (gameOptions [i].render.cockpit.bScaleGauges, i, 0);
		RP (gameOptions [i].render.cockpit.bSplitHUDMsgs, i, 0);
		RP (gameOptions [i].render.cockpit.bTextGauges, i, 0);
		RP (gameOptions [i].render.cockpit.nWindowPos, i, 0);
		RP (gameOptions [i].render.cockpit.nWindowSize, i, 0);
		RP (gameOptions [i].render.cockpit.nWindowZoom, i, 0);

		RP (gameOptions [i].render.color.bAmbientLight, i, 0);
		RP (gameOptions [i].render.color.bCap, i, 0);
		RP (gameOptions [i].render.color.bGunLight, i, 0);
		RP (gameOptions [i].render.color.bMix, i, 0);
		RP (gameOptions [i].render.color.nSaturation, i, 0);
		RP (gameOptions [i].render.color.bWalls, i, 0);
		RP (gameOptions [i].render.color.bUseLightmaps, i, 0);
		RP (gameOptions [i].render.color.nLightmapRange, i, 0);

		RP (gameOptions [i].render.powerups.b3D, i, 0);
		RP (gameOptions [i].render.powerups.nSpin, i, 0);

		RP (gameOptions [i].render.shadows.bFast, i, 0);
		RP (gameOptions [i].render.shadows.bMissiles, i, 0);
		RP (gameOptions [i].render.shadows.bPowerups, i, 0);
		RP (gameOptions [i].render.shadows.bPlayers, i, 0);
		RP (gameOptions [i].render.shadows.bReactors, i, 0);
		RP (gameOptions [i].render.shadows.bRobots, i, 0);
		RP (gameOptions [i].render.shadows.nClip, i, 0);
		RP (gameOptions [i].render.shadows.nLights, i, 0);
		RP (gameOptions [i].render.shadows.nReach, i, 0);

		RP (gameOptions [i].render.ship.nWingtip, i, 0);
		RP (gameOptions [i].render.ship.bBullets, i, 0);
		RP (gameOptions [i].render.ship.nColor, i, 0);

		RP (gameOptions [i].render.particles.bAuxViews, i, 0);
		RP (gameOptions [i].render.particles.bMonitors, i, 0);
		RP (gameOptions [i].render.particles.bPlasmaTrails, i, 0);
		RP (gameOptions [i].render.particles.bDecreaseLag, i, 0);
		RP (gameOptions [i].render.particles.bDebris, i, 0);
		RP (gameOptions [i].render.particles.bDisperse, i, 0);
		RP (gameOptions [i].render.particles.bRotate, i, 0);
		RP (gameOptions [i].render.particles.bMissiles, i, 0);
		RP (gameOptions [i].render.particles.bPlayers, i, 0);
		RP (gameOptions [i].render.particles.bRobots, i, 0);
		RP (gameOptions [i].render.particles.bStatic, i, 0);
		RP (gameOptions [i].render.particles.bBubbles, i, 0);
		RP (gameOptions [i].render.particles.bWobbleBubbles, i, 1);
		RP (gameOptions [i].render.particles.bWiggleBubbles, i, 1);
		RP (gameOptions [i].render.particles.bSyncSizes, i, 0);
		RP (gameOptions [i].render.particles.bSort, i, 0);

		RP (gameOptions [i].render.weaponIcons.alpha, i, 0);
		RP (gameOptions [i].render.weaponIcons.bEquipment, i, 0);
		RP (gameOptions [i].render.weaponIcons.bShowAmmo, i, 0);
		RP (gameOptions [i].render.weaponIcons.bSmall, i, 0);
		RP (gameOptions [i].render.weaponIcons.nSort, i, 0);
		RP (gameOptions [i].render.nMaxFPS, i, 0);
		RP (gameOptions [i].render.nQuality, i, 0);
		RP (gameOptions [i].render.cockpit.bFlashGauges, i, 0);
		RP (gameOptions [i].app.bExpertMode, i, 0);
		RP (gameOptions [i].app.nVersionFilter, i, 0);
		RP (gameOptions [i].demo.bOldFormat, i, 0);

		RP (gameOptions [i].sound.bFadeMusic, i, 1);
		RP (gameOptions [i].sound.bGatling, i, 0);
		RP (gameOptions [i].sound.bMissiles, i, 0);
		RP (gameOptions [i].sound.bShip, i, 0);
		RP (gameOptions [i].sound.xCustomSoundVolume, i, 0);

		RP (gameOptions [i].gameplay.bIdleAnims, i, 0);
		RP (gameOptions [i].gameplay.bInventory, i, 0);
		RP (gameOptions [i].gameplay.bNoThief, i, 0);
		RP (gameOptions [i].gameplay.bShieldWarning, i, 0);
		RP (gameOptions [i].gameplay.nAIAwareness, i, 0);
		RP (gameOptions [i].gameplay.nAIAggressivity, i, 0);
		RP (gameOptions [i].gameplay.nAutoSelectWeapon, i, 0);
		RP (gameOptions [i].gameplay.nSlowMotionSpeedup, i, 0);
		RP (gameOptions [i].gameplay.bUseD1AI, i, 0);
		}
	// options applicable for both enhanced and pure D2 mode
	for (j = 0; j < sizeofa (nWeaponOrder [i]); j++)
		RP (nWeaponOrder [i][j], i, j);
	RP (gameStates.render.cockpit.n3DView [i], i, 0);
	if (i)
		RP (extraGameInfo [i].bCompetition, i, 0);
	RP (extraGameInfo [i].bCloakedIndicators, i, 0);
	RP (extraGameInfo [i].bDamageIndicators, i, 0);
	RP (extraGameInfo [i].bDarkness, i, 0);
	RP (extraGameInfo [i].bDualMissileLaunch, i, 0);
	RP (extraGameInfo [i].bEnableCheats, i, 0);
	RP (extraGameInfo [i].bFastPitch, i, 0);
	RP (extraGameInfo [i].bFlickerLights, i, 0);
	RP (extraGameInfo [i].bFriendlyIndicators, i, 0);
	RP (extraGameInfo [i].bMslLockIndicators, i, 0);
	RP (extraGameInfo [i].bMouseLook, i, 0);
	RP (extraGameInfo [i].bPowerupLights, i, 0);
	RP (extraGameInfo [i].bKillMissiles, i, 0);
	RP (extraGameInfo [i].bTripleFusion, i, 0);
	RP (extraGameInfo [i].bEnhancedShakers, i, 0);
	RP (extraGameInfo [i].bTagOnlyHitObjs, i, 0);
	RP (extraGameInfo [i].bTargetIndicators, i, 0);
	RP (extraGameInfo [i].bTowFlags, i, 0);
	RP (extraGameInfo [i].bTeamDoors, i, 0);
	RP (extraGameInfo [i].nCoopPenalty, i, 0);
	RP (extraGameInfo [i].nHitboxes, i, 0);
	RP (extraGameInfo [i].nSpotSize, i, 0);

	RP (gameOptions [i].input.joystick.bUse, i, 0);
	RP (gameOptions [i].input.mouse.bUse, i, 0);
	RP (gameOptions [i].input.trackIR.bUse, i, 0);

	RP (gameOptions [i].gameplay.nAutoLeveling, i, 0);
	RP (gameOptions [i].gameplay.bFastRespawn, i, 0);

	RP (gameOptions [i].movies.bResize, i, 0);
	RP (gameOptions [i].movies.bSubTitles, i, 0);
	RP (gameOptions [i].movies.nQuality, i, 0);

	RP (gameOptions [i].menus.bShowLevelVersion, i, 0);
	RP (gameOptions [i].menus.bSmartFileSearch, i, 0);

	RP (gameOptions [i].multi.bUseMacros, i, 0);

	RP (gameOptions [i].ogl.bSetGammaRamp, i, 0);

	RP (gameOptions [i].render.bAllSegs, i, 0);
	RP (gameOptions [i].render.nMeshQuality, i, 0);
	RP (gameOptions [i].render.bUseLightmaps, i, 0);
	RP (gameOptions [i].render.nLightmapQuality, i, 0);

	RP (gameOptions [i].render.cockpit.bMissileView, i, 0);
	RP (gameOptions [i].render.cockpit.bHUD, i, 0);
	RP (gameOptions [i].render.cockpit.bReticle, i, 0);
	}
RegisterConfigParams (kcKeyboard, KcKeyboardSize (), "keyboard.");
RegisterConfigParams (kcMouse, KcMouseSize (), "mouse.");
RegisterConfigParams (kcJoystick, KcJoystickSize (), "joystick.");
RegisterConfigParams (kcSuperJoy, KcSuperJoySize (), "superjoy.");
RegisterConfigParams (kcHotkeys, KcHotkeySize (), "hotkeys.");
}

//------------------------------------------------------------------------------

int WriteParam (CFile& cf, tParam *pp)
{
	char	szVal [200];

if (strstr (pp->szTag, "Slowmo/Speed"))
	pp = pp;
#if 1
switch (pp->nSize) {
	case 1:
		sprintf (szVal, "=%d\n", *reinterpret_cast<sbyte*> (pp->valP));
		break;
	case 2:
		sprintf (szVal, "=%d\n", *reinterpret_cast<short*> (pp->valP));
		break;
	case 4:
		sprintf (szVal, "=%d\n", *reinterpret_cast<int*> (pp->valP));
		break;
	default:
		sprintf (szVal, "=%s\n", pp->valP);
		break;
	}
#else
	char	*valP = pp->valP, *p;
	short	nValues, nSize = pp->nSize;

strcpy (szVal, "=");
for (nValues = pp->nValues; nValues; nValues--, valP += nSize) {
	p = szVal + strlen (szVal);
	switch (nSize) {
		case 1:
			sprintf (p, "%d ", *reinterpret_cast<sbyte*> (pp->valP));
			break;
		case 2:
			sprintf (p, "%d ", *reinterpret_cast<short*> (pp->valP));
			break;
		case 4:
			sprintf (p, "%d ", *reinterpret_cast<int*> (pp->valP));
			break;
		default:
			sprintf (p, "%s", reinterpret_cast<char*> (pp->valP));
			goto done;
		}
	}

done:

strcat (szVal, "\n");
#endif
cf.Write (pp->szTag, 1, (int) strlen (pp->szTag));
cf.Write (szVal, 1, (int) strlen (szVal));
fflush (cf.File ());
return 1;
}

//------------------------------------------------------------------------------

int WriteParams (void)
{
	CFile			cf;
	char			fn [FILENAME_LEN];
	tParam		*pp;

RegisterParams ();
sprintf (fn, "%s.plx", LOCALPLAYER.callsign);
if (!cf.Open (fn, gameFolders.szProfDir, "wt", 0))
	return 0;
for (pp = paramList; pp; pp = pp->next)
	WriteParam (cf, pp);
return cf.Close ();
}

//------------------------------------------------------------------------------

tParam *FindParam (const char *pszTag)
{
	tParam	*pp;

for (pp = paramList; pp; pp = pp->next)
	if (!stricmp (pszTag, pp->szTag))
		return pp;
return NULL;
}

//------------------------------------------------------------------------------

#define issign(_c)	(((_c) == '-') || ((_c) == '+'))

//------------------------------------------------------------------------------

int SetParam (const char *pszIdent, const char *pszValue)
{
	tParam	*pp;
	int		nVal;

if (!(pp = FindParam (pszIdent))) {
#if DBG
	FindParam (pszIdent);
#endif
	return 0;
	}
nVal = atoi (pszValue);
switch (pp->nSize) {
	case 1:
		if (!(::isdigit (*pszValue) || issign (*pszValue)) || (nVal < SCHAR_MIN) || (nVal > SCHAR_MAX))
			return 0;
		*reinterpret_cast<sbyte*> (pp->valP) = (sbyte) nVal;
		break;
	case 2:
		if (!(::isdigit (*pszValue) || issign (*pszValue))  || (nVal < SHRT_MIN) || (nVal > SHRT_MAX))
			return 0;
		*reinterpret_cast<short*> (pp->valP) = (short) nVal;
		break;
	case 4:
		if (!(::isdigit (*pszValue) || issign (*pszValue)))
			return 0;
		*reinterpret_cast<int*> (pp->valP) = (int) nVal;
		break;
	default:
		strncpy (reinterpret_cast<char*> (pp->valP), pszValue, pp->nSize);
		break;
	}
return 1;
}

//------------------------------------------------------------------------------

int ReadParam (CFile& cf)
{
	char		szParam	[200], *pszValue;

cf.GetS (szParam, sizeof (szParam));
szParam [sizeof (szParam) - 1] = '\0';
if ((pszValue = strchr (szParam, '\n')))
	*pszValue = '\0';
if (!(pszValue = strchr (szParam, '=')))
	return 0;
*pszValue++ = '\0';
return SetParam (szParam, pszValue);
}

//------------------------------------------------------------------------------

int ReadParams (void)
{
	CFile			cf;
	char			fn [FILENAME_LEN];

RegisterParams ();
sprintf (fn, "%s.plx", LOCALPLAYER.callsign);
if (!cf.Open (fn, gameFolders.szProfDir, "rt", 0))
	return 0;
while (!cf.EoF ())
	ReadParam (cf);
return cf.Close ();
}

//------------------------------------------------------------------------------

typedef struct tParamValue {
	const char	*pszIdent;
	const char	*pszValue;
	} tParamValue;

tParamValue defaultParams [] = {
	{"gameData.render.window.w", "640"},
	{"gameData.render.window.h", "480"},
	{"iDlTimeout", "5"},
	{"gameStates.render.cockpit.nMode", "3"},
	{"gameStates.render.bShowFrameRate", "0"},
	{"gameStates.render.bShowTime", "1"},
	{"gameStates.sound.digi.nMaxChannels", "16"},
	{"gameStates.video.nDefaultDisplayMode", "3"},
	{"gameStates.video.nDefaultDisplayMode", "3"},
	{"gameOptions[0].render.cockpit.bGuidedInMainView", "1"},
	{"networkData.nNetLifeKills", "0"},
	{"networkData.nNetLifeKilled", "0"},
	{"gameData.app.nLifetimeChecksum", "0"},
	{"gameData.app.bUseMultiThreading[0]", "1"},
	{"gameData.app.bUseMultiThreading[1]", "1"},
	{"gameData.app.bUseMultiThreading[2]", "1"},
	{"gameData.app.bUseMultiThreading[3]", "1"},
	{"gameData.app.bUseMultiThreading[4]", "1"},
	{"gameData.app.bUseMultiThreading[5]", "1"},
	{"gameData.app.bUseMultiThreading[6]", "1"},
	{"gameData.app.bUseMultiThreading[7]", "1"},
	{"gameData.escort.szName", "GUIDE-BOT"},
	{"gameData.multigame.msg.szMacro[0]", "Why can't we all just get along?"},
	{"gameData.multigame.msg.szMacro[1]", "Hey, I got a present for ya"},
	{"gameData.multigame.msg.szMacro[2]", "I got a hankerin' for a spankerin'"},
	{"gameData.multigame.msg.szMacro[3]", "This one's headed for Uranus"},
	{"displayModeInfo[21].w", "0"},
	{"displayModeInfo[21].h", "0"},
	{"gameStates.app.nDifficultyLevel", "2"},
	{"gameStates.ogl.nContrast", "8"},
	{"gameStates.multi.nConnection", "1"},
	{"gameStates.multi.bUseTracker", "0"},
	{"gameData.menu.alpha", "79"},
	{"mpParams.nLevel", "1"},
	{"mpParams.nGameType", "3"},
	{"mpParams.nGameMode", "3"},
	{"mpParams.nGameAccess", "0"},
	{"mpParams.bShowPlayersOnAutomap", "0"},
	{"mpParams.nDifficulty", "2"},
	{"mpParams.nWeaponFilter", "67108863"},
	{"mpParams.nReactorLife", "2"},
	{"mpParams.nMaxTime", "0"},
	{"mpParams.nKillGoal", "2"},
	{"mpParams.bInvul", "-1"},
	{"mpParams.bMarkerView", "0"},
	{"mpParams.bIndestructibleLights", "0"},
	{"mpParams.bBrightPlayers", "0"},
	{"mpParams.bShowAllNames", "0"},
	{"mpParams.bShortPackets", "0"},
	{"mpParams.nPPS", "10"},
	{"mpParams.udpClientPort", "28342"},
	{"mpParams.szServerIpAddr", "127.0.0.1"},
	{"extraGameInfo[0].bAutoBalanceTeams", "0"},
	{"extraGameInfo[0].bAutoDownload", "1"},
	{"extraGameInfo[0].bDamageExplosions", "0"},
	{"extraGameInfo[0].bDisableReactor", "0"},
	{"extraGameInfo[0].bDropAllMissiles", "1"},
	{"extraGameInfo[0].bEnhancedCTF", "0"},
	{"extraGameInfo[0].bFixedRespawns", "0"},
	{"extraGameInfo[0].bFluidPhysics", "1"},
	{"extraGameInfo[0].bFriendlyFire", "1"},
	{"extraGameInfo[0].bGatlingTrails", "1"},
	{"extraGameInfo[0].bImmortalPowerups", "0"},
	{"extraGameInfo[0].bLightTrails", "1"},
	{"extraGameInfo[0].bMultiBosses", "1"},
	{"extraGameInfo[0].bPowerupsOnRadar", "1"},
	{"extraGameInfo[0].bPlayerShield", "0"},
	{"extraGameInfo[0].bRobotsHitRobots", "1"},
	{"extraGameInfo[0].bRobotsOnRadar", "1"},
	{"extraGameInfo[0].bRotateLevels", "0"},
	{"extraGameInfo[0].bSafeUDP", "0"},
	{"extraGameInfo[0].bShadows", "1"},
	{"extraGameInfo[0].bTeleporterCams", "0"},
	{"extraGameInfo[0].bSmartWeaponSwitch", "1"},
	{"extraGameInfo[0].bSmokeGrenades", "0"},
	{"extraGameInfo[0].bThrusterFlames", "1"},
	{"extraGameInfo[0].bTracers", "1"},
	{"extraGameInfo[0].bUseCameras", "1"},
	{"extraGameInfo[0].bUseParticles", "1"},
	{"extraGameInfo[0].bUseLightnings", "1"},
	{"extraGameInfo[0].bUseHitAngles", "0"},
	{"extraGameInfo[0].bGatlingSpeedUp", "0"},
	{"extraGameInfo[0].bRotateMarkers", "0"},
	{"extraGameInfo[0].bWiggle", "1"},
	{"extraGameInfo[0].grWallTransparency", "19"},
	{"extraGameInfo[0].nFusionRamp", "4"},
	{"extraGameInfo[0].nOmegaRamp", "4"},
	{"extraGameInfo[0].nLightRange", "2"},
	{"extraGameInfo[0].nMaxSmokeGrenades", "1"},
	{"extraGameInfo[0].nMslTurnSpeed", "1"},
	{"extraGameInfo[0].nMslStartSpeed", "0"},
	{"extraGameInfo[0].nRadar", "1"},
	{"extraGameInfo[0].nDrag", "10"},
	{"extraGameInfo[0].nSpawnDelay", "0"},
	{"extraGameInfo[0].nSpeedBoost", "10"},
	{"extraGameInfo[0].nWeaponDropMode", "1"},
	{"extraGameInfo[0].nWeaponIcons", "3"},
	{"extraGameInfo[0].nZoomMode", "1"},
	{"extraGameInfo[0].entropy.nCaptureVirusLimit", "1"},
	{"extraGameInfo[0].entropy.nCaptureTimeLimit", "1"},
	{"extraGameInfo[0].entropy.nMaxVirusCapacity", "0"},
	{"extraGameInfo[0].entropy.nBumpVirusCapacity", "2"},
	{"extraGameInfo[0].entropy.nBashVirusCapacity", "1"},
	{"extraGameInfo[0].entropy.nVirusGenTime", "2"},
	{"extraGameInfo[0].entropy.nVirusLifespan", "0"},
	{"extraGameInfo[0].entropy.nVirusStability", "0"},
	{"extraGameInfo[0].entropy.nEnergyFillRate", "25"},
	{"extraGameInfo[0].entropy.nShieldFillRate", "11"},
	{"extraGameInfo[0].entropy.nShieldDamageRate", "11"},
	{"extraGameInfo[0].entropy.bRevertRooms", "0"},
	{"extraGameInfo[0].entropy.bDoConquerWarning", "0"},
	{"extraGameInfo[0].entropy.nOverrideTextures", "2"},
	{"extraGameInfo[0].entropy.bBrightenRooms", "0"},
	{"extraGameInfo[0].entropy.bPlayerHandicap", "0"},
	{"extraGameInfo[0].headlight.bAvailable", "1"},
	{"extraGameInfo[0].headlight.bDrainPower", "1"},
	{"extraGameInfo[0].headlight.bBuiltIn", "0"},
	{"extraGameInfo[0].monsterball.nBonus", "1"},
	{"extraGameInfo[0].monsterball.nSizeMod", "7"},
	{"extraGameInfo[0].monsterball.forces[0].nWeaponId", "0"},
	{"extraGameInfo[0].monsterball.forces[0].nForce", "10"},
	{"extraGameInfo[0].monsterball.forces[1].nWeaponId", "1"},
	{"extraGameInfo[0].monsterball.forces[1].nForce", "15"},
	{"extraGameInfo[0].monsterball.forces[2].nWeaponId", "2"},
	{"extraGameInfo[0].monsterball.forces[2].nForce", "20"},
	{"extraGameInfo[0].monsterball.forces[3].nWeaponId", "3"},
	{"extraGameInfo[0].monsterball.forces[3].nForce", "25"},
	{"extraGameInfo[0].monsterball.forces[4].nWeaponId", "12"},
	{"extraGameInfo[0].monsterball.forces[4].nForce", "20"},
	{"extraGameInfo[0].monsterball.forces[5].nWeaponId", "11"},
	{"extraGameInfo[0].monsterball.forces[5].nForce", "10"},
	{"extraGameInfo[0].monsterball.forces[6].nWeaponId", "13"},
	{"extraGameInfo[0].monsterball.forces[6].nForce", "30"},
	{"extraGameInfo[0].monsterball.forces[7].nWeaponId", "14"},
	{"extraGameInfo[0].monsterball.forces[7].nForce", "100"},
	{"extraGameInfo[0].monsterball.forces[8].nWeaponId", "30"},
	{"extraGameInfo[0].monsterball.forces[8].nForce", "50"},
	{"extraGameInfo[0].monsterball.forces[9].nWeaponId", "31"},
	{"extraGameInfo[0].monsterball.forces[9].nForce", "60"},
	{"extraGameInfo[0].monsterball.forces[10].nWeaponId", "33"},
	{"extraGameInfo[0].monsterball.forces[10].nForce", "40"},
	{"extraGameInfo[0].monsterball.forces[11].nWeaponId", "32"},
	{"extraGameInfo[0].monsterball.forces[11].nForce", "30"},
	{"extraGameInfo[0].monsterball.forces[12].nWeaponId", "34"},
	{"extraGameInfo[0].monsterball.forces[12].nForce", "60"},
	{"extraGameInfo[0].monsterball.forces[13].nWeaponId", "35"},
	{"extraGameInfo[0].monsterball.forces[13].nForce", "30"},
	{"extraGameInfo[0].monsterball.forces[14].nWeaponId", "9"},
	{"extraGameInfo[0].monsterball.forces[14].nForce", "5"},
	{"extraGameInfo[0].monsterball.forces[15].nWeaponId", "8"},
	{"extraGameInfo[0].monsterball.forces[15].nForce", "-6"},
	{"extraGameInfo[0].monsterball.forces[16].nWeaponId", "15"},
	{"extraGameInfo[0].monsterball.forces[16].nForce", "50"},
	{"extraGameInfo[0].monsterball.forces[17].nWeaponId", "17"},
	{"extraGameInfo[0].monsterball.forces[17].nForce", "50"},
	{"extraGameInfo[0].monsterball.forces[18].nWeaponId", "18"},
	{"extraGameInfo[0].monsterball.forces[18].nForce", "80"},
	{"extraGameInfo[0].monsterball.forces[19].nWeaponId", "36"},
	{"extraGameInfo[0].monsterball.forces[19].nForce", "-106"},
	{"extraGameInfo[0].monsterball.forces[20].nWeaponId", "37"},
	{"extraGameInfo[0].monsterball.forces[20].nForce", "30"},
	{"extraGameInfo[0].monsterball.forces[21].nWeaponId", "39"},
	{"extraGameInfo[0].monsterball.forces[21].nForce", "40"},
	{"extraGameInfo[0].monsterball.forces[22].nWeaponId", "40"},
	{"extraGameInfo[0].monsterball.forces[22].nForce", "70"},
	{"extraGameInfo[0].monsterball.forces[23].nWeaponId", "54"},
	{"extraGameInfo[0].monsterball.forces[23].nForce", "-56"},
	{"extraGameInfo[0].monsterball.forces[24].nWeaponId", "-1"},
	{"extraGameInfo[0].monsterball.forces[24].nForce", "4"},
	{"extraGameInfo[0].headlight.bAvailable", "1"},
	{"extraGameInfo[0].headlight.bDrainPower", "1"},
	{"extraGameInfo[0].headlight.bBuiltIn", "0"},
	{"extraGameInfo[0].loadout.nGuns", "0"},
	{"extraGameInfo[0].loadout.nDevices", "0"},
	{"extraGameInfo [0].bShowWeapons", "1"},
	{"gameOptions[0].input.keyboard.bRamp[0]", "0"},
	{"gameOptions[0].input.mouse.sensitivity[0]", "8"},
	{"gameOptions[0].input.trackIR.sensitivity[0]", "8"},
	{"gameOptions[0].input.keyboard.bRamp[1]", "0"},
	{"gameOptions[0].input.mouse.sensitivity[1]", "8"},
	{"gameOptions[0].input.trackIR.sensitivity[1]", "8"},
	{"gameOptions[0].input.keyboard.bRamp[2]", "0"},
	{"gameOptions[0].input.mouse.sensitivity[2]", "8"},
	{"gameOptions[0].input.trackIR.sensitivity[2]", "4"},
	{"gameOptions[0].render.particles.nDens[0]", "1"},
	{"gameOptions[0].render.particles.nSize[0]", "1"},
	{"gameOptions[0].render.particles.nLife[0]", "1"},
	{"gameOptions[0].render.particles.nAlpha[0]", "1"},
	{"gameOptions[0].render.particles.nDens[1]", "1"},
	{"gameOptions[0].render.particles.nSize[1]", "1"},
	{"gameOptions[0].render.particles.nLife[1]", "1"},
	{"gameOptions[0].render.particles.nAlpha[1]", "1"},
	{"gameOptions[0].render.particles.nDens[2]", "1"},
	{"gameOptions[0].render.particles.nSize[2]", "1"},
	{"gameOptions[0].render.particles.nLife[2]", "1"},
	{"gameOptions[0].render.particles.nAlpha[2]", "1"},
	{"gameOptions[0].render.particles.nDens[3]", "1"},
	{"gameOptions[0].render.particles.nSize[3]", "1"},
	{"gameOptions[0].render.particles.nLife[3]", "1"},
	{"gameOptions[0].render.particles.nAlpha[3]", "1"},
	{"gameOptions[0].render.particles.nDens[4]", "1"},
	{"gameOptions[0].render.particles.nSize[4]", "1"},
	{"gameOptions[0].render.particles.nLife[4]", "1"},
	{"gameOptions[0].render.particles.nAlpha[4]", "1"},
	{"gameOptions[0].input.joystick.deadzones[0]", "1"},
	{"gameOptions[0].input.joystick.sensitivity[0]", "7"},
	{"gameOptions[0].input.trackIR.bMove[0]", "1"},
	{"gameOptions[0].input.joystick.deadzones[1]", "1"},
	{"gameOptions[0].input.joystick.sensitivity[1]", "7"},
	{"gameOptions[0].input.trackIR.bMove[1]", "1"},
	{"gameOptions[0].input.joystick.deadzones[2]", "1"},
	{"gameOptions[0].input.joystick.sensitivity[2]", "7"},
	{"gameOptions[0].input.trackIR.bMove[2]", "1"},
	{"gameOptions[0].input.joystick.deadzones[3]", "1"},
	{"gameOptions[0].input.joystick.sensitivity[3]", "7"},
	{"gameOptions[0].input.trackIR.bMove[3]", "0"},
	{"gameOptions[0].input.joystick.deadzones[4]", "1"},
	{"gameOptions[0].input.joystick.sensitivity[4]", "7"},
	{"gameOptions[0].input.trackIR.bMove[4]", "1"},
	{"gameOptions[0].input.bUseHotKeys", "1"},
	{"gameOptions[0].input.mouse.bJoystick", "0"},
	{"gameOptions[0].input.mouse.bSyncAxes", "1"},
	{"gameOptions[0].input.mouse.nDeadzone", "2"},
	{"gameOptions[0].input.joystick.bLinearSens", "0"},
	{"gameOptions[0].input.joystick.bSyncAxes", "1"},
	{"gameOptions[0].input.trackIR.nMode", "0"},
	{"gameOptions[0].input.trackIR.nDeadzone", "4"},
	{"gameOptions[0].input.keyboard.nRamp", "50"},
	{"gameOptions[0].ogl.bLightObjects", "1"},
	{"gameOptions[0].ogl.bHeadlights", "0"},
	{"gameOptions[0].ogl.bLightPowerups", "0"},
	{"gameOptions[0].ogl.bObjLighting", "0"},
	{"gameOptions[0].ogl.nMaxLightsPerFace", "16"},
	{"gameOptions[0].ogl.nMaxLightsPerPass", "8"},
	{"gameOptions[0].ogl.nMaxLightsPerObject", "8"},
	{"gameOptions[0].render.nLightingMethod", "0"},
	{"gameOptions[0].render.nDebrisLife", "0"},
	{"gameOptions[0].render.textures.nQuality", "2"},
	{"gameOptions[0].render.effects.bAutoTransparency", "1"},
	{"gameOptions[0].render.effects.bSoftParticles", "0"},
	{"gameOptions[0].render.effects.bMovingSparks", "0"},
	{"gameOptions[0].render.effects.bExplBlasts", "1"},
	{"gameOptions[0].render.effects.nShrapnels", "1"},
	{"gameOptions[0].render.coronas.bUse", "1"},
	{"gameOptions[0].render.coronas.nStyle", "1"},
	{"gameOptions[0].render.coronas.bShots", "1"},
	{"gameOptions[0].render.coronas.bPowerups", "1"},
	{"gameOptions[0].render.coronas.bWeapons", "0"},
	{"gameOptions[0].render.coronas.nIntensity", "2"},
	{"gameOptions[0].render.coronas.nObjIntensity", "1"},
	{"gameOptions[0].render.coronas.bAdditive", "0"},
	{"gameOptions[0].render.effects.bEnergySparks", "0"},
	{"gameOptions[0].render.bBrightObjects", "0"},
	{"gameOptions[0].render.effects.bRobotShields", "0"},
	{"gameOptions[0].render.effects.bOnlyShieldHits", "0"},
	{"gameOptions[0].render.effects.bTransparent", "1"},
	{"gameOptions[0].render.bDepthSort", "1"},
	{"gameOptions[0].render.cameras.bFitToWall", "0"},
	{"gameOptions[0].render.cameras.bHires", "0"},
	{"gameOptions[0].render.cameras.nFPS", "0"},
	{"gameOptions[0].render.cameras.nSpeed", "5000"},
	{"gameOptions[0].render.automap.bBright", "0"},
	{"gameOptions[0].render.automap.bGrayOut", "1"},
	{"gameOptions[0].render.automap.bCoronas", "1"},
	{"gameOptions[0].render.automap.bSparks", "1"},
	{"gameOptions[0].render.automap.bParticles", "1"},
	{"gameOptions[0].render.automap.bLightnings", "1"},
	{"gameOptions[0].render.automap.bTextured", "1"},
	{"gameOptions[0].render.automap.bSkybox", "0"},
	{"gameOptions[0].render.automap.nColor", "1"},
	{"gameOptions[0].render.automap.nRange", "0"},
	{"gameOptions[0].render.cockpit.bMouseIndicator", "1"},
	{"gameOptions[0].render.cockpit.bObjectTally", "1"},
	{"gameOptions[0].render.cockpit.bPlayerStats", "0"},
	{"gameOptions[0].render.cockpit.bRotateMslLockInd", "1"},
	{"gameOptions[0].render.cockpit.bScaleGauges", "1"},
	{"gameOptions[0].render.cockpit.bSplitHUDMsgs", "1"},
	{"gameOptions[0].render.cockpit.bTextGauges", "0"},
	{"gameOptions[0].render.cockpit.nWindowPos", "1"},
	{"gameOptions[0].render.cockpit.nWindowSize", "0"},
	{"gameOptions[0].render.cockpit.nWindowZoom", "1"},
	{"gameOptions[0].render.color.bAmbientLight", "1"},
	{"gameOptions[0].render.color.bCap", "0"},
	{"gameOptions[0].render.color.bGunLight", "1"},
	{"gameOptions[0].render.color.bMix", "1"},
	{"gameOptions[0].render.color.nColorSaturation", "0"},
	{"gameOptions[0].render.color.bWalls", "1"},
	{"gameOptions[0].render.color.bUseLightmaps", "0"},
	{"gameOptions[0].render.color.nLightmapRange", "0"},
	{"gameOptions[0].render.lightnings.bAuxViews", "0"},
	{"gameOptions[0].render.lightnings.bMonitors", "0"},
	{"gameOptions[0].render.lightnings.bDamage", "1"},
	{"gameOptions[0].render.lightnings.bExplosions", "1"},
	{"gameOptions[0].render.lightnings.bOmega", "1"},
	{"gameOptions[0].render.lightnings.bRobotOmega", "0"},
	{"gameOptions[0].render.lightnings.bPlayers", "1"},
	{"gameOptions[0].render.lightnings.bRobots", "1"},
	{"gameOptions[0].render.lightnings.bStatic", "1"},
	{"gameOptions[0].render.lightnings.bCoronas", "1"},
	{"gameOptions[0].render.lightnings.nQuality", "1"},
	{"gameOptions[0].render.lightnings.nStyle", "1"},
	{"gameOptions[0].render.powerups.b3D", "1"},
	{"gameOptions[0].render.powerups.nSpin", "1"},
	{"gameOptions[0].render.shadows.bFast", "1"},
	{"gameOptions[0].render.shadows.bMissiles", "1"},
	{"gameOptions[0].render.shadows.bPowerups", "1"},
	{"gameOptions[0].render.shadows.bPlayers", "1"},
	{"gameOptions[0].render.shadows.bReactors", "0"},
	{"gameOptions[0].render.shadows.bRobots", "1"},
	{"gameOptions[0].render.shadows.nClip", "1"},
	{"gameOptions[0].render.shadows.nLights", "2"},
	{"gameOptions[0].render.shadows.nReach", "2"},
	{"gameOptions[0].render.ship.nWingtip", "1"},
	{"gameOptions[0].render.ship.bBullets", "1"},
	{"gameOptions[0].render.ship.nColor", "1"},
	{"gameOptions[0].render.particles.bAuxViews", "0"},
	{"gameOptions[0].render.particles.bMonitors", "0"},
	{"gameOptions[0].render.particles.bPlasmaTrails", "0"},
	{"gameOptions[0].render.particles.bDecreaseLag", "0"},
	{"gameOptions[0].render.particles.bDebris", "1"},
	{"gameOptions[0].render.particles.bDisperse", "1"},
	{"gameOptions[0].render.particles.bRotate", "1"},
	{"gameOptions[0].render.particles.bMissiles", "1"},
	{"gameOptions[0].render.particles.bPlayers", "1"},
	{"gameOptions[0].render.particles.bRobots", "1"},
	{"gameOptions[0].render.particles.bStatic", "1"},
	{"gameOptions[0].render.particles.bBubbles", "1"},
	{"gameOptions[0].render.particles.bWiggleBubbles", "1"},
	{"gameOptions[0].render.particles.bWobbleBubbles", "1"},
	{"gameOptions[0].render.particles.bSyncSizes", "1"},
	{"gameOptions[0].render.particles.bSort", "1"},
	{"gameOptions[0].render.weaponIcons.alpha", "3"},
	{"gameOptions[0].render.weaponIcons.bEquipment", "1"},
	{"gameOptions[0].render.weaponIcons.bShowAmmo", "1"},
	{"gameOptions[0].render.weaponIcons.bSmall", "1"},
	{"gameOptions[0].render.weaponIcons.nSort", "1"},
	{"gameOptions[0].render.nMaxFPS", "250"},
	{"gameOptions[0].render.nQuality", "3"},
	{"gameOptions[0].render.cockpit.bFlashGauges", "1"},
	{"gameOptions[0].sound.bFadeMusic", "1"},
	{"gameOptions[0].sound.bGatling", "0"},
	{"gameOptions[0].sound.bMissiles", "0"},
	{"gameOptions[0].sound.bShip", "0"},
	{"gameOptions[0].sound.xCustomVolume", "5"},
	{"gameOptions[0].app.bExpertMode", "1"},
	{"gameOptions[0].app.nVersionFilter", "3"},
	{"gameOptions[0].demo.bOldFormat", "0"},
	{"gameOptions[0].gameplay.bIdleAnims", "1"},
	{"gameOptions[0].gameplay.bNoThief", "0"},
	{"gameOptions[0].gameplay.bInventory", "1"},
	{"gameOptions[0].gameplay.bShieldWarning", "1"},
	{"gameOptions[0].gameplay.nAIAwareness", "0"},
	{"gameOptions[0].gameplay.nAIAggressivity", "0"},
	{"gameOptions[0].gameplay.nAutoSelectWeapon", "1"},
	{"gameOptions[0].gameplay.nSlowMotionSpeedup", "6"},
	{"gameOptions[0].gameplay.bUseD1AI", "1"},
	{"nWeaponOrder[0][0]", "6"},
	{"nWeaponOrder[0][1]", "5"},
	{"nWeaponOrder[0][2]", "8"},
	{"nWeaponOrder[0][3]", "3"},
	{"nWeaponOrder[0][4]", "4"},
	{"nWeaponOrder[0][5]", "9"},
	{"nWeaponOrder[0][6]", "7"},
	{"nWeaponOrder[0][7]", "0"},
	{"nWeaponOrder[0][8]", "1"},
	{"nWeaponOrder[0][9]", "2"},
	{"nWeaponOrder[0][10]", "-1"},
	{"gameStates.render.cockpit.n3DView[0]", "0"},
	{"extraGameInfo[0].bCloakedIndicators", "0"},
	{"extraGameInfo[0].bDamageIndicators", "1"},
	{"extraGameInfo[0].bDarkness", "0"},
	{"extraGameInfo[0].bDualMissileLaunch", "0"},
	{"extraGameInfo[0].bEnableCheats", "0"},
	{"extraGameInfo[0].bFastPitch", "2"},
	{"extraGameInfo[0].bFlickerLights", "1"},
	{"extraGameInfo[0].bFriendlyIndicators", "0"},
	{"extraGameInfo[0].bHeadlights", "0"},
	{"extraGameInfo[0].bMslLockIndicators", "1"},
	{"extraGameInfo[0].bMouseLook", "0"},
	{"extraGameInfo[0].bPowerupLights", "1"},
	{"extraGameInfo[0].bKillMissiles", "0"},
	{"extraGameInfo[0].bTripleFusion", "1"},
	{"extraGameInfo[0].bEnhancedShakers", "0"},
	{"extraGameInfo[0].bTagOnlyHitObjs", "1"},
	{"extraGameInfo[0].bTargetIndicators", "1"},
	{"extraGameInfo[0].bTowFlags", "0"},
	{"extraGameInfo[0].bTeamDoors", "0"},
	{"extraGameInfo[0].nCoopPenalty", "0"},
	{"extraGameInfo[0].nHitboxes", "2"},
	{"extraGameInfo[0].nSpotSize", "0"},
	{"gameOptions[0].input.joystick.bUse", "1"},
	{"gameOptions[0].input.mouse.bUse", "1"},
	{"gameOptions[0].input.trackIR.bUse", "1"},
	{"gameOptions[0].gameplay.nAutoLeveling", "0"},
	{"gameOptions[0].gameplay.bFastRespawn", "0"},
	{"gameOptions[0].movies.bResize", "1"},
	{"gameOptions[0].movies.bSubTitles", "0"},
	{"gameOptions[0].movies.nQuality", "0"},
	{"gameOptions[0].menus.bShowLevelVersion", "1"},
	{"gameOptions[0].menus.bSmartFileSearch", "1"},
	{"gameOptions[0].multi.bUseMacros", "1"},
	{"gameOptions[0].ogl.bSetGammaRamp", "0"},
	{"gameOptions[0].render.bAllSegs", "0"},
	{"gameOptions[0].render.nMeshQuality", "0"},
	{"gameOptions[0].render.bUseLightmaps", "0"},
	{"gameOptions[0].render.nLightmapQuality", "1"},
	{"gameOptions[0].render.cockpit.bMissileView", "0"},
	{"gameOptions[0].render.cockpit.bHUD", "1"},
	{"gameOptions[0].render.cockpit.bReticle", "1"},
	{"nWeaponOrder[1][0]", "9"},
	{"nWeaponOrder[1][1]", "4"},
	{"nWeaponOrder[1][2]", "8"},
	{"nWeaponOrder[1][3]", "3"},
	{"nWeaponOrder[1][4]", "1"},
	{"nWeaponOrder[1][5]", "5"},
	{"nWeaponOrder[1][6]", "0"},
	{"nWeaponOrder[1][7]", "6"},
	{"nWeaponOrder[1][8]", "-1"},
	{"nWeaponOrder[1][9]", "7"},
	{"nWeaponOrder[1][10]", "2"},
	{"gameStates.render.cockpit.n3DView[1]", "0"},
	{"extraGameInfo[1].bCompetition", "1"},
	{"extraGameInfo[1].bCloakedIndicators", "0"},
	{"extraGameInfo[1].bDamageIndicators", "1"},
	{"extraGameInfo[1].bDarkness", "0"},
	{"extraGameInfo[1].bDualMissileLaunch", "0"},
	{"extraGameInfo[1].bEnableCheats", "0"},
	{"extraGameInfo[1].bFastPitch", "2"},
	{"extraGameInfo[1].bFlickerLights", "1"},
	{"extraGameInfo[1].bFriendlyIndicators", "1"},
	{"extraGameInfo[1].bHeadlights", "1"},
	{"extraGameInfo[1].bMslLockIndicators", "1"},
	{"extraGameInfo[1].bMouseLook", "0"},
	{"extraGameInfo[1].bPowerupLights", "0"},
	{"extraGameInfo[1].bKillMissiles", "0"},
	{"extraGameInfo[1].bTripleFusion", "1"},
	{"extraGameInfo[1].bTagOnlyHitObjs", "0"},
	{"extraGameInfo[1].bTargetIndicators", "1"},
	{"extraGameInfo[1].bTowFlags", "1"},
	{"extraGameInfo[1].bTeamDoors", "0"},
	{"extraGameInfo[1].nCoopPenalty", "1"},
	{"extraGameInfo[1].nHitboxes", "2"},
	{"extraGameInfo[1].nSpotSize", "0"},
	{"gameOptions[1].input.joystick.bUse", "0"},
	{"gameOptions[1].input.mouse.bUse", "0"},
	{"gameOptions[1].input.trackIR.bUse", "0"},
	{"gameOptions[1].gameplay.nAutoLeveling", "0"},
	{"gameOptions[1].gameplay.bFastRespawn", "0"},
	{"gameOptions[1].movies.bResize", "0"},
	{"gameOptions[1].movies.bSubTitles", "0"},
	{"gameOptions[1].movies.nQuality", "0"},
	{"gameOptions[1].menus.bShowLevelVersion", "0"},
	{"gameOptions[1].menus.bSmartFileSearch", "1"},
	{"gameOptions[1].multi.bUseMacros", "0"},
	{"gameOptions[1].ogl.bSetGammaRamp", "0"},
	{"gameOptions[1].render.bAllSegs", "0"},
	{"gameOptions[1].render.nMeshQuality", "0"},
	{"gameOptions[1].render.nLightmapQuality", "1"},
	{"gameOptions[1].render.cockpit.bMissileView", "0"},
	{"gameOptions[1].render.cockpit.bHUD", "1"},
	{"gameOptions[1].render.cockpit.bReticle", "1"},
	{"keyboard.Pitch forward[0].value", "-56"},
	{"keyboard.Pitch forward[1].value", "-1"},
	{"keyboard.Pitch backward[0].value", "-48"},
	{"keyboard.Pitch backward[1].value", "-1"},
	{"keyboard.Turn left[0].value", "-53"},
	{"keyboard.Turn left[1].value", "-1"},
	{"keyboard.Turn right[0].value", "-51"},
	{"keyboard.Turn right[1].value", "-1"},
	{"keyboard.Slide on[0].value", "-1"},
	{"keyboard.Slide on[1].value", "-1"},
	{"keyboard.Slide left[0].value", "75"},
	{"keyboard.Slide left[1].value", "30"},
	{"keyboard.Slide right[0].value", "77"},
	{"keyboard.Slide right[1].value", "32"},
	{"keyboard.Slide up[0].value", "71"},
	{"keyboard.Slide up[1].value", "16"},
	{"keyboard.Slide down[0].value", "73"},
	{"keyboard.Slide down[1].value", "18"},
	{"keyboard.Bank on[0].value", "-1"},
	{"keyboard.Bank on[1].value", "-1"},
	{"keyboard.Bank left[0].value", "79"},
	{"keyboard.Bank left[1].value", "-1"},
	{"keyboard.Bank right[0].value", "81"},
	{"keyboard.Bank right[1].value", "-1"},
	{"keyboard.Fire primary[0].value", "-1"},
	{"keyboard.Fire primary[1].value", "-1"},
	{"keyboard.Fire secondary[0].value", "57"},
	{"keyboard.Fire secondary[1].value", "-1"},
	{"keyboard.Fire flare[0].value", "74"},
	{"keyboard.Fire flare[1].value", "-1"},
	{"keyboard.Accelerate[0].value", "72"},
	{"keyboard.Accelerate[1].value", "17"},
	{"keyboard.reverse[0].value", "76"},
	{"keyboard.reverse[1].value", "31"},
	{"keyboard.Drop Bomb[0].value", "82"},
	{"keyboard.Drop Bomb[1].value", "-1"},
	{"keyboard.Rear View[0].value", "14"},
	{"keyboard.Rear View[1].value", "-1"},
	{"keyboard.Cruise Faster[0].value", "-55"},
	{"keyboard.Cruise Faster[1].value", "-1"},
	{"keyboard.Cruise Slower[0].value", "-47"},
	{"keyboard.Cruise Slower[1].value", "-1"},
	{"keyboard.Cruise Off[0].value", "-45"},
	{"keyboard.Cruise Off[1].value", "-1"},
	{"keyboard.Automap[0].value", "41"},
	{"keyboard.Automap[1].value", "-1"},
	{"keyboard.Afterburner[0].value", "-100"},
	{"keyboard.Afterburner[1].value", "-1"},
	{"keyboard.Cycle Primary[0].value", "52"},
	{"keyboard.Cycle Primary[1].value", "-1"},
	{"keyboard.Cycle Second[0].value", "51"},
	{"keyboard.Cycle Second[1].value", "-1"},
	{"keyboard.Zoom In[0].value", "-57"},
	{"keyboard.Zoom In[1].value", "-1"},
	{"keyboard.Headlight[0].value", "-49"},
	{"keyboard.Headlight[1].value", "-1"},
	{"keyboard.Energy->Shield[0].value", "78"},
	{"keyboard.Energy->Shield[1].value", "-1"},
	{"keyboard.Toggle Bomb.value", "83"},
	{"keyboard.Toggle Icons.value", "35"},
	{"keyboard.Use Cloak[0].value", "46"},
	{"keyboard.Use Cloak[1].value", "-1"},
	{"keyboard.Use Invul[0].value", "23"},
	{"keyboard.Use Invul[1].value", "-1"},
	{"keyboard.Slowmo/Speed[0].value", "-75"},
	{"keyboard.Slowmo/Speed[1].value", "55"},
	{"mouse.Fire primary.value", "0"},
	{"mouse.Fire secondary.value", "1"},
	{"mouse.Accelerate.value", "-1"},
	{"mouse.reverse.value", "-1"},
	{"mouse.Fire flare.value", "-1"},
	{"mouse.Slide on.value", "-1"},
	{"mouse.Slide left.value", "-1"},
	{"mouse.Slide right.value", "-1"},
	{"mouse.Slide up.value", "-1"},
	{"mouse.Slide down.value", "-1"},
	{"mouse.Bank on.value", "-1"},
	{"mouse.Bank left.value", "-1"},
	{"mouse.Bank right.value", "-1"},
	{"mouse.Pitch U/D[0].value", "1"},
	{"mouse.Pitch U/D[1].value", "0"},
	{"mouse.Turn L/R[0].value", "0"},
	{"mouse.Turn L/R[1].value", "0"},
	{"mouse.Slide L/R[0].value", "-1"},
	{"mouse.Slide L/R[1].value", "0"},
	{"mouse.Slide U/D[0].value", "-1"},
	{"mouse.Slide U/D[1].value", "0"},
	{"mouse.Bank L/R[0].value", "-1"},
	{"mouse.Bank L/R[1].value", "0"},
	{"mouse.Throttle[0].value", "-1"},
	{"mouse.Throttle[1].value", "0"},
	{"mouse.Rear View.value", "-1"},
	{"mouse.Drop Bomb.value", "-1"},
	{"mouse.Afterburner.value", "2"},
	{"mouse.Cycle Primary.value", "3"},
	{"mouse.Cycle Second.value", "4"},
	{"mouse.Zoom in.value", "-1"},
	{"joystick.Fire primary[0].value", "0"},
	{"joystick.Fire secondary[0].value", "1"},
	{"joystick.Accelerate[0].value", "-1"},
	{"joystick.reverse[0].value", "-1"},
	{"joystick.Fire flare[0].value", "-1"},
	{"joystick.Slide on[0].value", "-1"},
	{"joystick.Slide left[0].value", "15"},
	{"joystick.Slide right[0].value", "13"},
	{"joystick.Slide up[0].value", "12"},
	{"joystick.Slide down[0].value", "14"},
	{"joystick.Bank on[0].value", "-1"},
	{"joystick.Bank left[0].value", "-1"},
	{"joystick.Bank right[0].value", "-1"},
	{"joystick.Pitch U/D[0].value", "1"},
	{"joystick.Turn L/R[0].value", "0"},
	{"joystick.Slide L/R[0].value", "-1"},
	{"joystick.Slide U/D[0].value", "-1"},
	{"joystick.Bank L/R[0].value", "3"},
	{"joystick.throttle[0].value", "2"},
	{"joystick.Rear View[0].value", "-1"},
	{"joystick.Drop Bomb[0].value", "5"},
	{"joystick.Afterburner[0].value", "4"},
	{"joystick.Cycle Primary[0].value", "2"},
	{"joystick.Cycle Secondary[0].value", "3"},
	{"joystick.Headlight[0].value", "-1"},
	{"joystick.Toggle Bomb[0].value", "-1"},
	{"joystick.Toggle Icons[0].value", "-1"},
	{"joystick.Automap[0].value", "-1"},
	{"joystick.Use Cloak[0].value", "-1"},
	{"joystick.Use Invul[0].value", "-1"},
	{"joystick.Fire primary[1].value", "32"},
	{"joystick.Fire secondary[1].value", "33"},
	{"joystick.Accelerate[1].value", "-1"},
	{"joystick.reverse[1].value", "-1"},
	{"joystick.Fire flare[1].value", "-1"},
	{"joystick.Slide on[1].value", "-1"},
	{"joystick.Slide left[1].value", "43"},
	{"joystick.Slide right[1].value", "41"},
	{"joystick.Slide up[1].value", "40"},
	{"joystick.Slide down[1].value", "42"},
	{"joystick.Bank on[1].value", "-1"},
	{"joystick.Bank left[1].value", "-1"},
	{"joystick.Bank right[1].value", "-1"},
	{"joystick.Pitch U/D[1].value", "9"},
	{"joystick.Turn L/R[1].value", "8"},
	{"joystick.Slide L/R[1].value", "-1"},
	{"joystick.Slide U/D[1].value", "-1"},
	{"joystick.Bank L/R[1].value", "11"},
	{"joystick.throttle[1].value", "10"},
	{"joystick.Rear View[1].value", "-1"},
	{"joystick.Drop Bomb[1].value", "37"},
	{"joystick.Afterburner[1].value", "36"},
	{"joystick.Cycle Primary[1].value", "34"},
	{"joystick.Cycle Secondary[1].value", "35"},
	{"joystick.Headlight[1].value", "-1"},
	{"joystick.Toggle Bomb[1].value", "-1"},
	{"joystick.Toggle Icons[1].value", "-1"},
	{"joystick.Automap[1].value", "-1"},
	{"joystick.Use Cloak[1].value", "38"},
	{"joystick.Use Invul[1].value", "39"},
	{"joystick.Pitch U/D[2].value", "0"},
	{"joystick.Turn L/R[2].value", "0"},
	{"joystick.Slide L/R[2].value", "0"},
	{"joystick.Slide U/D[2].value", "0"},
	{"joystick.Bank L/R[2].value", "0"},
	{"joystick.throttle[2].value", "0"},
	{"superjoy.Fire primary.value", "-1"},
	{"superjoy.Fire secondary.value", "-1"},
	{"superjoy.Accelerate.value", "-1"},
	{"superjoy.reverse.value", "-1"},
	{"superjoy.Fire flare.value", "-1"},
	{"superjoy.Slide on.value", "-1"},
	{"superjoy.Slide left.value", "-1"},
	{"superjoy.Slide right.value", "-1"},
	{"superjoy.Slide up.value", "-1"},
	{"superjoy.Slide down.value", "-1"},
	{"superjoy.Bank on.value", "-1"},
	{"superjoy.Bank left.value", "-1"},
	{"superjoy.Bank right.value", "-1"},
	{"superjoy.Pitch U/D[0].value", "-1"},
	{"superjoy.Pitch U/D[1].value", "-1"},
	{"superjoy.Turn L/R[0].value", "-1"},
	{"superjoy.Turn L/R[1].value", "-1"},
	{"superjoy.Slide L/R[0].value", "-1"},
	{"superjoy.Slide L/R[1].value", "-1"},
	{"superjoy.Slide U/D[0].value", "-1"},
	{"superjoy.Slide U/D[1].value", "-1"},
	{"superjoy.Bank L/R[0].value", "-1"},
	{"superjoy.Bank L/R[1].value", "-1"},
	{"superjoy.throttle[0].value", "-1"},
	{"superjoy.throttle[1].value", "-1"},
	{"superjoy.Rear View.value", "-1"},
	{"superjoy.Drop Bomb.value", "-1"},
	{"superjoy.Afterburner.value", "-1"},
	{"superjoy.Cycle Primary.value", "-1"},
	{"superjoy.Cycle Secondary.value", "-1"},
	{"superjoy.Headlight.value", "-1"},
	{"superjoy.Toggle Bomb.value", "-1"},
	{"superjoy.Toggle Icons.value", "-1"},
	{"superjoy.Automap.value", "-1"},
	{"hotkeys.Weapon 1[0].value", "2"},
	{"hotkeys.Weapon 1[1].value", "-1"},
	{"hotkeys.Weapon 2[0].value", "3"},
	{"hotkeys.Weapon 2[1].value", "-1"},
	{"hotkeys.Weapon 3[0].value", "4"},
	{"hotkeys.Weapon 3[1].value", "-1"},
	{"hotkeys.Weapon 4[0].value", "5"},
	{"hotkeys.Weapon 4[1].value", "-1"},
	{"hotkeys.Weapon 5[0].value", "6"},
	{"hotkeys.Weapon 5[1].value", "-1"},
	{"hotkeys.Weapon 6[0].value", "7"},
	{"hotkeys.Weapon 6[1].value", "-1"},
	{"hotkeys.Weapon 7[0].value", "8"},
	{"hotkeys.Weapon 7[1].value", "-1"},
	{"hotkeys.Weapon 8[0].value", "9"},
	{"hotkeys.Weapon 8[1].value", "-1"},
	{"hotkeys.Weapon 9[0].value", "10"},
	{"hotkeys.Weapon 9[1].value", "-1"},
	{"hotkeys.Weapon 10[0].value", "11"},
	{"hotkeys.Weapon 10[1].value", "-1"}
	};

//------------------------------------------------------------------------------

void InitGameParams (void)
{
	tParamValue	*pv;
	int			i;

for (i = sizeofa (defaultParams), pv = defaultParams; i; i--, pv++) {
	SetParam (pv->pszIdent, pv->pszValue);
	}
}

//------------------------------------------------------------------------------

int NewPlayerConfig (void)
{
	int nitems;
	int i,j,choice;
	tMenuItem m[8];
   int mct=CONTROL_MAX_TYPES;

mct--;
InitWeaponOrdering ();		//setup default weapon priorities

RetrySelection:

memset (m, 0, sizeof (m));
for (i = 0; i < mct; i++ )	{
	m [i].nType = NM_TYPE_MENU;
	m [i].text = const_cast<char*> (CONTROL_TEXT(i));
	m [i].key = -1;
	}
nitems = i;
m [0].text = const_cast<char*> (TXT_CONTROL_KEYBOARD);
choice = gameConfig.nControlType;				// Assume keyboard
#ifndef APPLE_DEMO
i = ExecMenu1( NULL, TXT_CHOOSE_INPUT, i, m, NULL, &choice );
#else
choice = 0;
#endif
if (i < 0)
	return 0;
for (i = 0; i < CONTROL_MAX_TYPES; i++)
	for (j = 0; j < MAX_CONTROLS; j++)
		controlSettings.custom [i][j] = controlSettings.defaults [i][j];
//added on 2/4/99 by Victor Rachels for new keys
for(i = 0; i < MAX_HOTKEY_CONTROLS; i++)
	controlSettings.d2xCustom[i] = controlSettings.d2xDefaults[i];
//end this section addition - VR
KCSetControls (0);
gameConfig.nControlType = choice;
if (gameConfig.nControlType == CONTROL_THRUSTMASTER_FCS) {
	i = ExecMessageBox (TXT_IMPORTANT_NOTE, NULL, 2, "Choose another", TXT_OK, TXT_FCS);
	if (i == 0)
		goto RetrySelection;
	}
if ((gameConfig.nControlType > 0) && (gameConfig.nControlType < 5))
	JoyDefsCalibrate();
gameStates.app.nDifficultyLevel = DEFAULT_DIFFICULTY;
gameOptions [0].gameplay.nAutoLeveling = 1;
nHighestLevels = 1;
highestLevels [0].shortname [0] = 0;			//no name for mission 0
highestLevels [0].nLevel = 1;				//was highest level in old struct
gameOpts->input.joystick.sensitivity [0] =
gameOpts->input.joystick.sensitivity [1] =
gameOpts->input.joystick.sensitivity [2] =
gameOpts->input.joystick.sensitivity [3] =
gameOpts->input.joystick.sensitivity [4] = 8;
gameOpts->input.mouse.sensitivity [0] =
gameOpts->input.mouse.sensitivity [1] =
gameOpts->input.mouse.sensitivity [2] = 8;
gameStates.render.cockpit.n3DView[0]=CV_NONE;
gameStates.render.cockpit.n3DView[1]=CV_NONE;

// Default taunt macros
strcpy(gameData.multigame.msg.szMacro[0], TXT_GET_ALONG);
strcpy(gameData.multigame.msg.szMacro[1], TXT_GOT_PRESENT);
strcpy(gameData.multigame.msg.szMacro[2], TXT_HANKERING);
strcpy(gameData.multigame.msg.szMacro[3], TXT_URANUS);
networkData.nNetLifeKills = 0;
networkData.nNetLifeKilled = 0;
gameData.app.nLifetimeChecksum = GetLifetimeChecksum (networkData.nNetLifeKills, networkData.nNetLifeKilled);
InitGameParams ();
#if 0
InitGameOptions (0);
InitArgs (0, NULL);
InitGameOptions (1);
#endif
return 1;
}

//------------------------------------------------------------------------------

void ReadBinD2XParams (CFile& cf)
{
	uint	i, j, gameOptsSize = 0;

if (gameStates.input.nPlrFileVersion >= 97)
	gameOptsSize = cf.ReadInt ();
for (i = 0; i < 2; i++) {
	if (i && (gameOptsSize > sizeof (tGameOptions)))
		cf.Seek (gameOptsSize - sizeof (tGameOptions), SEEK_CUR);
	if (!i) {
		if (gameStates.input.nPlrFileVersion >= 26)
			extraGameInfo [0].bFixedRespawns = (int) cf.ReadByte ();
		if (gameStates.input.nPlrFileVersion >= 27)
			/*extraGameInfo [0].bFriendlyFire = (int)*/ cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 28) {
		gameOptions [i].render.nMaxFPS = (int) cf.ReadByte ();
		if (gameOptions [i].render.nMaxFPS < 0) {
			gameOptions [i].render.nMaxFPS += 256;
			if (gameOptions [i].render.nMaxFPS < 0)
				gameOptions [i].render.nMaxFPS = 250;
			}
		if (!i)
			extraGameInfo [0].nSpawnDelay = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 29)
		gameOptions [i].input.joystick.deadzones [0] = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 30)
		gameOptions [i].render.cockpit.nWindowSize = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 31)
		gameOptions [i].render.cockpit.nWindowPos = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 32)
		gameOptions [i].render.cockpit.nWindowZoom = (int) cf.ReadByte ();
	if (!i && (gameStates.input.nPlrFileVersion >= 33))
		extraGameInfo [0].bPowerupsOnRadar = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 34) {
		gameOptions [i].input.keyboard.nRamp = (int) cf.ReadByte ();
		if (gameOptions [i].input.keyboard.nRamp < 10)
			gameOptions [i].input.keyboard.nRamp = 10;
		else if (gameOptions [i].input.keyboard.nRamp > 100)
			gameOptions [i].input.keyboard.nRamp = 100;
		}
	if (gameStates.input.nPlrFileVersion >= 35)
		gameOptions [i].render.color.bAmbientLight = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 36)
		gameOptions [i].ogl.bSetGammaRamp = (int) cf.ReadByte ();
	if (!i && (gameStates.input.nPlrFileVersion >= 37))
		extraGameInfo [0].nZoomMode = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 38)
		for (j = 0; j < 3; j++)
			gameOptions [i].input.keyboard.bRamp [j] = (int) cf.ReadByte ();
	if (!i && (gameStates.input.nPlrFileVersion >= 39))
#if 0
		extraGameInfo [0].bEnhancedCTF = (int) cf.ReadByte ();
#else
		cf.ReadByte ();
#endif
	if (!i && (gameStates.input.nPlrFileVersion >= 40))
		extraGameInfo [0].bRobotsHitRobots = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 41)
		gameOptions [i].gameplay.nAutoSelectWeapon = (int) cf.ReadByte ();
	if (!i && (gameStates.input.nPlrFileVersion >= 42))
		extraGameInfo [0].bAutoDownload = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 43)
		gameOptions [i].render.color.bGunLight = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 44)
		gameStates.multi.bUseTracker = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 45)
		gameOptions [i].gameplay.bFastRespawn = (int) cf.ReadByte ();
	if (!i && (gameStates.input.nPlrFileVersion >= 46))
		extraGameInfo [0].bDualMissileLaunch = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 47) {
		gameStates.app.nDifficultyLevel = (int) cf.ReadByte ();
		gameStates.app.nDifficultyLevel = NMCLAMP (gameStates.app.nDifficultyLevel, 0, 4);
		}
	if (gameStates.input.nPlrFileVersion >= 48)
		gameOptions [i].render.effects.bTransparent = (int) cf.ReadByte ();
	if (!i && (gameStates.input.nPlrFileVersion >= 49))
		extraGameInfo [0].bRobotsOnRadar = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 50)
		gameOptions [i].render.bAllSegs = (int) cf.ReadByte ();
	if (!i && (gameStates.input.nPlrFileVersion >= 51))
		extraGameInfo [0].grWallTransparency = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 52)
		gameOptions [i].input.mouse.sensitivity [0] = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 53) {
		gameOptions [i].multi.bUseMacros = (int) cf.ReadByte ();
		if (!i)
			extraGameInfo [0].bWiggle = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 54)
		gameOptions [i].movies.nQuality = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 55)
		gameOptions [i].render.color.bWalls = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 56)
		gameOptions [i].input.joystick.bLinearSens = (int) cf.ReadByte ();
	if (!i) {
		if (gameStates.input.nPlrFileVersion >= 57)
			extraGameInfo [0].nSpeedBoost = (int) cf.ReadByte ();
		if (gameStates.input.nPlrFileVersion >= 58) {
			extraGameInfo [0].bDropAllMissiles = (int) cf.ReadByte ();
			extraGameInfo [0].bImmortalPowerups = (int) cf.ReadByte ();
			}
		if (gameStates.input.nPlrFileVersion >= 59)
			extraGameInfo [0].bUseCameras = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 60) {
		gameOptions [i].render.cameras.bFitToWall = (int) cf.ReadByte ();
		gameOptions [i].render.cameras.nFPS = (int) cf.ReadByte ();
		if (!i)
			extraGameInfo [0].nFusionRamp = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 62)
		gameOptions [i].render.color.bUseLightmaps = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 63)
		gameOptions [i].render.cockpit.bHUD = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 64)
		gameOptions [i].render.color.nLightmapRange = (int) cf.ReadByte ();
	if (!i) {
		if (gameStates.input.nPlrFileVersion >= 65)
			extraGameInfo [0].bMouseLook = (int) cf.ReadByte ();
		if	(gameStates.input.nPlrFileVersion >= 66)
			extraGameInfo [0].bMultiBosses = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 67)
		gameOptions [i].app.nVersionFilter = (int) cf.ReadByte ();
	if (!i) {
		if (gameStates.input.nPlrFileVersion >= 68)
			extraGameInfo [0].bSmartWeaponSwitch = (int) cf.ReadByte ();
		if (gameStates.input.nPlrFileVersion >= 69)
			extraGameInfo [0].bFluidPhysics = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 70) {
		gameOptions [i].render.nQuality = (int) cf.ReadByte ();
		SetRenderQuality ();
		}
	if (gameStates.input.nPlrFileVersion >= 71)
		gameOptions [i].movies.bSubTitles = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 72)
		gameOptions [i].render.textures.nQuality = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 73)
		gameOptions [i].render.cameras.nSpeed = cf.ReadInt ();
	if (!i && (gameStates.input.nPlrFileVersion >= 74))
		extraGameInfo [0].nWeaponDropMode = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 75)
		gameOptions [i].menus.bSmartFileSearch = cf.ReadInt ();
	if (gameStates.input.nPlrFileVersion >= 76)
		SetDlTimeout (cf.ReadInt ());
	if (!i && (gameStates.input.nPlrFileVersion >= 79)) {
		extraGameInfo [0].entropy.nCaptureVirusLimit = cf.ReadByte ();
		extraGameInfo [0].entropy.nCaptureTimeLimit = cf.ReadByte ();
		extraGameInfo [0].entropy.nMaxVirusCapacity = cf.ReadByte ();
		extraGameInfo [0].entropy.nBumpVirusCapacity = cf.ReadByte ();
		extraGameInfo [0].entropy.nBashVirusCapacity = cf.ReadByte ();
		extraGameInfo [0].entropy.nVirusGenTime = cf.ReadByte ();
		extraGameInfo [0].entropy.nVirusLifespan = cf.ReadByte ();
		extraGameInfo [0].entropy.nVirusStability = cf.ReadByte ();
		extraGameInfo [0].entropy.nEnergyFillRate = cf.ReadShort ();
		extraGameInfo [0].entropy.nShieldFillRate = cf.ReadShort ();
		extraGameInfo [0].entropy.nShieldDamageRate = cf.ReadShort ();
		extraGameInfo [0].entropy.bRevertRooms = cf.ReadByte ();
		extraGameInfo [0].entropy.bDoConquerWarning = cf.ReadByte ();
		extraGameInfo [0].entropy.nOverrideTextures = cf.ReadByte ();
		extraGameInfo [0].entropy.bBrightenRooms = cf.ReadByte ();
		extraGameInfo [0].entropy.bPlayerHandicap = cf.ReadByte ();

		mpParams.nLevel = cf.ReadByte ();
		mpParams.nGameType = cf.ReadByte ();
		mpParams.nGameMode = cf.ReadByte ();
		mpParams.nGameAccess = cf.ReadByte ();
		mpParams.bShowPlayersOnAutomap = cf.ReadByte ();
		mpParams.nDifficulty = cf.ReadByte ();
		mpParams.nWeaponFilter = cf.ReadInt ();
		mpParams.nReactorLife = cf.ReadInt ();
		mpParams.nMaxTime = cf.ReadByte ();
		mpParams.nKillGoal = cf.ReadByte ();
		mpParams.bInvul = cf.ReadByte ();
		mpParams.bMarkerView = cf.ReadByte ();
		mpParams.bIndestructibleLights = cf.ReadByte ();
		mpParams.bBrightPlayers = cf.ReadByte ();
		mpParams.bShowAllNames = cf.ReadByte ();
		mpParams.bShortPackets = cf.ReadByte ();
		mpParams.nPPS = cf.ReadByte ();
		mpParams.udpClientPort = cf.ReadInt ();
		cf.Read(mpParams.szServerIpAddr, 16, 1);
		}
	if (gameStates.input.nPlrFileVersion >= 80)
		cf.ReadByte ();
	if (!i && (gameStates.input.nPlrFileVersion >= 81)) {
		extraGameInfo [1].bRotateLevels = (int) cf.ReadByte ();
		extraGameInfo [1].bDisableReactor = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 82) {
		for (j = 0; j < 4; j++)
			gameOptions [i].input.joystick.deadzones [j] = (int) cf.ReadByte ();
		for (j = 0; j < 4; j++)
			gameOptions [i].input.joystick.sensitivity [j] = cf.ReadByte ();
		gameOptions [i].input.joystick.bSyncAxes = (int) cf.ReadByte ();
		}
	if (!i && (gameStates.input.nPlrFileVersion >= 83))
		extraGameInfo [1].bDualMissileLaunch = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 84) {
		if (!i)
			extraGameInfo [1].bMouseLook = (int) cf.ReadByte ();
		for (j = 0; j < 3; j++)
			gameOptions [i].input.mouse.sensitivity [j] = (int) cf.ReadByte ();
		gameOptions [i].input.mouse.bSyncAxes = (int) cf.ReadByte ();
		gameOptions [i].render.color.bMix = (int) cf.ReadByte ();
		gameOptions [i].render.color.bCap = (int) cf.ReadByte ();
		}
	if (!i && (gameStates.input.nPlrFileVersion >= 85))
		extraGameInfo [0].nWeaponIcons = (ubyte) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 86)
		gameOptions [i].render.weaponIcons.bSmall = (ubyte) cf.ReadByte ();
	if (!i && (gameStates.input.nPlrFileVersion >= 87))
		extraGameInfo [0].bAutoBalanceTeams = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 88) {
		gameOptions [i].movies.bResize = (int) cf.ReadByte ();
		gameOptions [i].menus.bShowLevelVersion = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 89)
		gameOptions [i].render.weaponIcons.nSort = (ubyte) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 90)
		gameOptions [i].render.weaponIcons.bShowAmmo = (ubyte) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 91) {
		gameOptions [i].input.mouse.bUse = (ubyte) cf.ReadByte ();
		gameOptions [i].input.joystick.bUse = (ubyte) cf.ReadByte ();
		gameOptions [i].input.bUseHotKeys = (ubyte) cf.ReadByte ();
		}
	if (!i) {
		if (gameStates.input.nPlrFileVersion >= 92)
			extraGameInfo [0].bSafeUDP = (char) cf.ReadByte ();
		if (gameStates.input.nPlrFileVersion >= 93)
			cf.Read(mpParams.szServerIpAddr + 16, 6, 1);
		}
	if (gameStates.input.nPlrFileVersion >= 95) {
		gameOptions [i].render.cockpit.bTextGauges = (int) cf.ReadByte ();
		gameOptions [i].render.cockpit.bScaleGauges = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 96) {
		gameOptions [i].render.weaponIcons.bEquipment = (int) cf.ReadByte ();
		gameOptions [i].render.weaponIcons.alpha = (ubyte) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 97)
		gameOptions [i].render.cockpit.bFlashGauges = (int) cf.ReadByte ();
	if (!i && (gameStates.input.nPlrFileVersion >= 98))
		for (j = 0; j < 2; j++)
			if (!(extraGameInfo [j].bFastPitch = (int) cf.ReadByte ()))
				extraGameInfo [j].bFastPitch = 2;
	if (!i && (gameStates.input.nPlrFileVersion >= 99))
		gameStates.multi.nConnection = cf.ReadInt ();
	if (gameStates.input.nPlrFileVersion >= 100) {
		if (!i)
			extraGameInfo [0].bUseParticles = (int) cf.ReadByte ();
		gameOptions [i].render.particles.nDens [0] = cf.ReadInt ();
		gameOptions [i].render.particles.nSize [0] = cf.ReadInt ();
		gameOptions [i].render.particles.nDens [0] = NMCLAMP (gameOptions [i].render.particles.nDens [0], 0, 4);
		gameOptions [i].render.particles.nSize [0] = NMCLAMP (gameOptions [i].render.particles.nSize [0], 0, 3);
		gameOptions [i].render.particles.bPlayers = (int) cf.ReadByte ();
		gameOptions [i].render.particles.bRobots = (int) cf.ReadByte ();
		gameOptions [i].render.particles.bMissiles = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 101) {
		if (!i) {
			extraGameInfo [0].bDamageExplosions = (int) cf.ReadByte ();
			extraGameInfo [0].bThrusterFlames = (int) cf.ReadByte ();
			}
		}
	if (gameStates.input.nPlrFileVersion >= 102) {
		cf.ReadByte ();
		gameOptions [i].render.particles.bCollisions = 0;
		}
	if (gameStates.input.nPlrFileVersion >= 103)
		gameOptions [i].gameplay.bShieldWarning = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 104)
		gameOptions [i].app.bExpertMode = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 105) {
		if (!i)
#if SHADOWS
			extraGameInfo [0].bShadows = cf.ReadByte ();
		gameOptions [i].render.shadows.nLights = cf.ReadInt ();
		gameOptions [i].render.shadows.nLights = NMCLAMP (gameOptions [i].render.shadows.nLights, 0, 8);
#else
			cf.ReadByte ();
		cf.ReadInt ();
#endif
		}
	if (gameStates.input.nPlrFileVersion >= 106)
		if (!i)
			gameStates.ogl.nContrast = cf.ReadInt ();
	if (gameStates.input.nPlrFileVersion >= 107)
		if (!i)
			extraGameInfo [0].bPlayerShield = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 108)
		gameOptions [i].gameplay.bInventory = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 109)
		gameOptions [i].input.mouse.bJoystick = cf.ReadInt ();
	if (gameStates.input.nPlrFileVersion >= 110)
		if (!i) {
			int	w, h;
			w = cf.ReadInt ();
			h = cf.ReadInt ();
			if (!gameStates.gfx.bOverride && (gameStates.video.nDefaultDisplayMode == NUM_DISPLAY_MODES))
				SetCustomDisplayMode (w, h);
			}
	if (gameStates.input.nPlrFileVersion >= 111)
		gameOptions [i].render.cockpit.bMouseIndicator = cf.ReadInt ();
	if (gameStates.input.nPlrFileVersion >= 112)
		extraGameInfo [i].bTeleporterCams = cf.ReadInt ();
	if (gameStates.input.nPlrFileVersion >= 113)
		gameOptions [i].render.cockpit.bSplitHUDMsgs = cf.ReadInt ();
	if (gameStates.input.nPlrFileVersion >= 114) {
		gameOptions [i].input.joystick.deadzones [4] = (int) cf.ReadByte ();
		gameOptions [i].input.joystick.sensitivity [4] = cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 115)
		if (!i) {
			tMonsterballForce *pf = extraGameInfo [0].monsterball.forces;
			extraGameInfo [0].monsterball.nBonus = cf.ReadByte ();
			for (j = 0; j < MAX_MONSTERBALL_FORCES; j++, pf++) {
				pf->nWeaponId = cf.ReadByte ();
				pf->nForce = cf.ReadByte ();
				}
			}
	if (gameStates.input.nPlrFileVersion >= 116)
		if (!i)
			extraGameInfo [0].monsterball.nSizeMod = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 117) {
		gameOptions [i].render.nLightingMethod = cf.ReadInt ();
		gameOptions [i].ogl.bLightObjects = cf.ReadInt ();
		gameOptions [i].ogl.nMaxLightsPerFace = cf.ReadInt ();
		}
	if (gameStates.input.nPlrFileVersion >= 118)
		extraGameInfo [i].bDarkness = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 119) {
		extraGameInfo [i].bTeamDoors = cf.ReadByte ();
		extraGameInfo [i].bEnableCheats = cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 120) {
		extraGameInfo [i].bTargetIndicators = cf.ReadByte ();
		extraGameInfo [i].bDamageIndicators = cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 121) {
		extraGameInfo [i].bCloakedIndicators = cf.ReadByte ();
		extraGameInfo [i].bFriendlyIndicators = cf.ReadByte ();
		extraGameInfo [i].headlight.bAvailable = cf.ReadByte ();
		extraGameInfo [i].bPowerupLights = cf.ReadByte ();
		extraGameInfo [i].nSpotSize = cf.ReadByte ();
		extraGameInfo [i].nSpotStrength = extraGameInfo [i].nSpotSize;
		}
	if (gameStates.input.nPlrFileVersion >= 122)
		extraGameInfo [i].bTowFlags = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 123)
		gameOptions [i].render.particles.bDecreaseLag = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 124)
		gameOptions [i].render.effects.bAutoTransparency = cf.ReadByte ();
	if (!i) {
		if (gameStates.input.nPlrFileVersion >= 125)
			extraGameInfo [i].bUseHitAngles = cf.ReadByte ();
		if (gameStates.input.nPlrFileVersion >= 126)
			extraGameInfo [i].bLightTrails = cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 126) {
		gameOptions [i].render.particles.bSyncSizes = cf.ReadInt ();
		for (j = 1; j < 4; j++) {
			gameOptions [i].render.particles.nDens [j] = cf.ReadInt ();
			gameOptions [i].render.particles.nSize [j] = cf.ReadInt ();
			}
		}
	if (!i) {
		if (gameStates.input.nPlrFileVersion >= 127) {
			extraGameInfo [i].bTracers = cf.ReadByte ();
			cf.ReadByte ();
			extraGameInfo [i].bShockwaves = 0;
			}
		}
	if (gameStates.input.nPlrFileVersion >= 128)
		gameOptions [i].render.particles.bDisperse = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 129)
		extraGameInfo [i].bTagOnlyHitObjs = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 130)
		for (j = 0; j < 4; j++)
			gameOptions [i].render.particles.nLife [j] = cf.ReadInt ();
	if (gameStates.input.nPlrFileVersion >= 131) {
		gameOptions [i].render.shadows.bRobots = (int) cf.ReadByte ();
		gameOptions [i].render.shadows.bMissiles = (int) cf.ReadByte ();
		gameOptions [i].render.shadows.bReactors = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 132)
		gameOptions [i].render.shadows.bPlayers = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 133)
		gameOptions [i].render.shadows.bFast = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 134)
		gameOptions [i].render.shadows.nReach = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 135)
		gameOptions [i].render.shadows.nClip = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 136) {
		gameOptions [i].render.powerups.b3D = (int) cf.ReadByte ();
		gameOptions [i].render.powerups.nSpin = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 137)
		gameOptions [i].gameplay.bIdleAnims = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 138)
		if (!i)
			gameStates.app.nDifficultyLevel = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 139)
		gameOptions [i].demo.bOldFormat = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 140)
		gameOptions [i].render.cockpit.bObjectTally = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 141)
		if (!i)
			extraGameInfo [0].nLightRange = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 142)
		if (i)
			extraGameInfo [i].bCompetition = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 143)
		extraGameInfo [i].bFlickerLights = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 144)
		if (!i)
			extraGameInfo [i].bSmokeGrenades = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 145)
		if (!i)
			extraGameInfo [i].nMaxSmokeGrenades = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 146)
		if (!i)
			extraGameInfo [i].nMslTurnSpeed = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 147) {
		gameOptions [i].render.particles.bDebris = (int) cf.ReadByte ();
		gameOptions [i].render.particles.bStatic = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 148)
		gameOptions [i].gameplay.nAIAwareness = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 149) {
		extraGameInfo [i].nCoopPenalty = cf.ReadByte ();
		extraGameInfo [i].bKillMissiles = cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 150)
		extraGameInfo [i].nHitboxes = cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 151)
		gameOptions [i].render.cockpit.bPlayerStats = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 152)
		gameOptions [i].render.coronas.bUse = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 153) {
		gameOptions [i].render.automap.bTextured = (int) cf.ReadByte ();
		gameOptions [i].render.automap.bBright = (int) cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 154)
		gameOptions [i].render.automap.bCoronas = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 155) {
		gameOptions [i].render.automap.nColor = (int) cf.ReadByte ();
		if (!i)
			extraGameInfo [0].nRadar = cf.ReadByte ();
		}
	if (gameStates.input.nPlrFileVersion >= 156)
		gameOptions [i].render.automap.nRange = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 157)
		gameOptions [i].render.automap.bParticles = (int) cf.ReadByte ();
	if (gameStates.input.nPlrFileVersion >= 158)
		gameOptions [i].render.nDebrisLife = cf.ReadInt ();
	if (gameStates.input.nPlrFileVersion >= 159) {
		extraGameInfo [i].bMslLockIndicators = cf.ReadByte ();
		gameOpts->render.cockpit.bRotateMslLockInd = cf.ReadByte ();
		}
	}
}

//------------------------------------------------------------------------------

//this length must match the value in escort.c
#define GUIDEBOT_NAME_LEN 9

ubyte dosControlType,winControlType;

//read in the CPlayerData's saved games.  returns errno (0 == no error)
int ReadPlayerFile (int bOnlyWindowSizes)
{
	CFile		cf;
	char		filename [FILENAME_LEN], buf [128];
	ubyte		nDisplayMode;
	short		nControlTypes, gameWindowW, gameWindowH;
	int		funcRes = EZERO;
	int		id, bRewriteIt = 0, nMaxControls;
	uint i;

Assert(gameData.multiplayer.nLocalPlayer>=0 && gameData.multiplayer.nLocalPlayer<MAX_PLAYERS);

sprintf(filename, "%.8s.plr", LOCALPLAYER.callsign);
if (!cf.Open (filename, gameFolders.szProfDir, "rb", 0)) {
	PrintLog ("   couldn't read player file '%s'\n", filename);
	return errno;
	}
id = cf.ReadInt ();
// SWAPINT added here because old versions of d2x
// used the wrong byte order.
if (nCFileError || ((id != SAVE_FILE_ID) && (id != SWAPINT (SAVE_FILE_ID)))) {
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, "Invalid player file");
	cf.Close ();
	return -1;
	}

gameStates.input.nPlrFileVersion = cf.ReadShort ();
if (gameStates.input.nPlrFileVersion < D2XW32_PLAYER_FILE_VERSION)
	nMaxControls = 48;
else if (gameStates.input.nPlrFileVersion < 108)
	nMaxControls = 60;
else if (gameStates.input.nPlrFileVersion < D2XXL_PLAYER_FILE_VERSION)
	nMaxControls = 64;
else
	nMaxControls = 64;	//MAX_CONTROLS

if ((gameStates.input.nPlrFileVersion < COMPATIBLE_PLAYER_FILE_VERSION) ||
	 ((gameStates.input.nPlrFileVersion > D2W95_PLAYER_FILE_VERSION) &&
	  (gameStates.input.nPlrFileVersion < D2XW32_PLAYER_FILE_VERSION))) {
	ExecMessageBox(TXT_ERROR, NULL, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
	cf.Close ();
	return -1;
	}

gameWindowW = gameData.render.window.w;
gameWindowH = gameData.render.window.h;
gameData.render.window.w = cf.ReadShort ();
gameData.render.window.h = cf.ReadShort ();
if (bOnlyWindowSizes)
	goto done;

gameStates.app.nDifficultyLevel = cf.ReadByte ();
gameOpts->gameplay.nAutoLeveling = cf.ReadByte ();
gameOpts->render.cockpit.bReticle = cf.ReadByte ();
gameStates.render.cockpit.nMode = cf.ReadByte ();
nDisplayMode = gameStates.video.nDefaultDisplayMode;
gameStates.video.nDefaultDisplayMode = cf.ReadByte ();
gameOpts->render.cockpit.bMissileView = cf.ReadByte ();
extraGameInfo [0].headlight.bAvailable = cf.ReadByte ();
gameOptions [0].render.cockpit.bGuidedInMainView = cf.ReadByte ();
if (gameStates.input.nPlrFileVersion >= 19)
	cf.ReadByte ();	//skip obsolete byte value
//read new highest level info
nHighestLevels = cf.ReadShort ();
Assert(nHighestLevels <= MAX_MISSIONS);
if (cf.Read (highestLevels, sizeof (hli), nHighestLevels) != (size_t) nHighestLevels) {
	funcRes = errno;
	cf.Close ();
	return funcRes;
	}
//read taunt macros
for (i = 0; i < 4; i++)
	if (cf.Read (gameData.multigame.msg.szMacro [i], MAX_MESSAGE_LEN, 1) != 1) {
		funcRes = errno;
		break;
	}
//read KConfig data
nControlTypes = (gameStates.input.nPlrFileVersion < 20) ? 7 : CONTROL_MAX_TYPES;
if (cf.Read (controlSettings.custom, nMaxControls * nControlTypes, 1) != 1)
	funcRes = errno;
else if (cf.Read (reinterpret_cast<ubyte*> (&dosControlType), sizeof (ubyte), 1) != 1)
	funcRes = errno;
else if ((gameStates.input.nPlrFileVersion >= 21) && cf.Read (reinterpret_cast<ubyte*> (&winControlType), sizeof (ubyte), 1) != 1)
	funcRes = errno;
else if (cf.Read (gameOptions [0].input.joystick.sensitivity, sizeof (ubyte), 1) != 1)
	funcRes = errno;
gameConfig.nControlType = dosControlType;
for (i = 0; i < 11; i++) {
	nWeaponOrder [0][i] = cf.ReadByte ();
	nWeaponOrder [1][i] = cf.ReadByte ();
	}
if (gameStates.input.nPlrFileVersion >= 16) {
	gameStates.render.cockpit.n3DView [0] = cf.ReadInt ();
	gameStates.render.cockpit.n3DView [1] = cf.ReadInt ();
	}

if (gameStates.input.nPlrFileVersion >= 22) {
	networkData.nNetLifeKills = cf.ReadInt ();
	networkData.nNetLifeKilled = cf.ReadInt ();
	}
else {
	networkData.nNetLifeKills = 0;
	networkData.nNetLifeKilled = 0;
	}

if (gameStates.input.nPlrFileVersion >= 23)
  gameData.app.nLifetimeChecksum = cf.ReadInt ();

//read guidebot name
if (gameStates.input.nPlrFileVersion >= 18)
	cf.ReadString (gameData.escort.szName, GUIDEBOT_NAME_LEN);
else
	strcpy (gameData.escort.szName, "GUIDE-BOT");
gameData.escort.szName [sizeof (gameData.escort.szName) - 1] = '\0';
if (gameStates.input.nPlrFileVersion >= D2W95_PLAYER_FILE_VERSION)
	cf.ReadString (buf, 127);
if (gameStates.input.nPlrFileVersion >= 25)
	cf.Read (controlSettings.d2xCustom, MAX_HOTKEY_CONTROLS, 1);
else {
	for(i = 0; i < MAX_HOTKEY_CONTROLS; i++)
		controlSettings.d2xCustom [i] = controlSettings.d2xDefaults [i];
	}
if (gameStates.input.nPlrFileVersion >= D2XXL_PLAYER_FILE_VERSION) {
	ReadParams ();
	KCSetControls (1);
	}
else {
	ReadBinD2XParams (cf);
	KCSetControls (0);
	}

//post processing of parameters
if (gameStates.input.nPlrFileVersion >= 23) {
	if (gameData.app.nLifetimeChecksum != GetLifetimeChecksum (networkData.nNetLifeKills, networkData.nNetLifeKilled)) {
		networkData.nNetLifeKills =
		networkData.nNetLifeKilled = 0;
		gameData.app.nLifetimeChecksum = 0;
 		ExecMessageBox (NULL, NULL, 1, TXT_PROFILE_DAMAGED, TXT_WARNING);
		bRewriteIt = 1;
		}
	}
if (gameStates.gfx.bOverride) {
	gameStates.video.nDefaultDisplayMode = nDisplayMode;
	gameData.render.window.w = gameWindowW;
	gameData.render.window.h = gameWindowH;
	}
else if (gameStates.video.nDefaultDisplayMode == NUM_DISPLAY_MODES)
	displayModeInfo [NUM_DISPLAY_MODES].VGA_mode = ((int) gameData.render.window.w << 16) + gameData.render.window.h;

for (i = 0; i < sizeof (gameData.escort.szName); i++) {
	if (!gameData.escort.szName [i])
		break;
	if (!isprint (gameData.escort.szName [i])) {
		strcpy(gameData.escort.szName, "GUIDE-BOT");
		break;
		}
	}
strcpy (gameData.escort.szRealName, gameData.escort.szName);
mpParams.bDarkness = extraGameInfo [1].bDarkness;
mpParams.bTeamDoors = extraGameInfo [1].bTeamDoors;
mpParams.bEnableCheats = extraGameInfo [1].bEnableCheats;
extraGameInfo [0].nSpawnDelay *= 1000;
extraGameInfo [1].bDisableReactor = 0;
ValidatePrios (primaryOrder, defaultPrimaryOrder, MAX_PRIMARY_WEAPONS);
ValidatePrios (secondaryOrder, defaultSecondaryOrder, MAX_SECONDARY_WEAPONS);
SetDebrisCollisions ();
SetMaxOmegaCharge ();

done:

if (cf.Close ())
	if (funcRes == EZERO)
		funcRes = errno;
if (bRewriteIt)
	WritePlayerFile ();

gameStates.render.nLightingMethod = gameOpts->render.nLightingMethod;
if (gameStates.render.nLightingMethod == 2)
	gameStates.render.bPerPixelLighting = 2;
else if ((gameStates.render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps)
	gameStates.render.bPerPixelLighting = 1;
else
	gameStates.render.bPerPixelLighting = 0;
gameStates.render.nMaxLightsPerPass = gameOpts->ogl.nMaxLightsPerPass;
gameStates.render.nMaxLightsPerFace = gameOpts->ogl.nMaxLightsPerFace;
gameStates.render.nMaxLightsPerObject = gameOpts->ogl.nMaxLightsPerObject;
gameStates.render.bAmbientColor = gameStates.render.bPerPixelLighting || gameOpts->render.color.bAmbientLight;

return funcRes;
}

//------------------------------------------------------------------------------

//finds entry for this level in table.  if not found, returns ptr to
//empty entry.  If no empty entries, takes over last one
int FindHLIEntry()
{
	int i;

for (i = 0; i <nHighestLevels; i++)
	if (!stricmp (highestLevels [i].shortname, gameData.missions.list [gameData.missions.nCurrentMission].filename))
		break;
	if (i == nHighestLevels) {		//not found.  create entry
		if (i == MAX_MISSIONS)
			i--;		//take last entry
		else
			nHighestLevels++;
		strcpy (highestLevels[i].shortname, gameData.missions.list [gameData.missions.nCurrentMission].filename);
		highestLevels [i].nLevel = 0;
		}
return i;
}

//------------------------------------------------------------------------------
//set a new highest level for CPlayerData for this mission
void SetHighestLevel (ubyte nLevel)
{
	int ret, i;

ret = ReadPlayerFile (0);
if ((ret != EZERO) && (ret != ENOENT))		//if file doesn't exist, that's ok
	return;
i = FindHLIEntry ();
if (nLevel > highestLevels [i].nLevel)
	highestLevels [i].nLevel = nLevel;
WritePlayerFile ();
}

//------------------------------------------------------------------------------
//gets the CPlayerData's highest level from the file for this mission
int GetHighestLevel(void)
{
	int i;
	int nHighestSaturnLevel = 0;

ReadPlayerFile (0);
#ifndef SATURN
if (strlen (gameData.missions.list [gameData.missions.nCurrentMission].filename) == 0)	{
	for (i = 0; i < nHighestLevels; i++)
		if (!stricmp (highestLevels [i].shortname, "DESTSAT")) 	//	Destination Saturn.
		 	nHighestSaturnLevel = highestLevels [i].nLevel;
}
#endif
i = highestLevels [FindHLIEntry()].nLevel;
if (nHighestSaturnLevel > i)
   i = nHighestSaturnLevel;
return i;
}

//------------------------------------------------------------------------------
#if 0
void WriteBinD2XParams (CFile cf)
{
	int	i, j;

cf.WriteInt (sizeof (tGameOptions));
for (i = 0; i < 2; i++) {
	if (!i) {
		cf.WriteByte (extraGameInfo [0].bFixedRespawns);
		cf.WriteByte (extraGameInfo [0].bFriendlyFire);
		}
	cf.WriteByte ((sbyte) gameOptions [i].render.nMaxFPS);
	if (!i)
		cf.WriteByte ((sbyte) (extraGameInfo [0].nSpawnDelay / 1000));
	cf.WriteByte ((sbyte) gameOptions [i].input.joystick.deadzones [0]);
	cf.WriteByte ((sbyte) gameOptions [i].render.cockpit.nWindowSize);
	cf.WriteByte ((sbyte) gameOptions [i].render.cockpit.nWindowPos);
	cf.WriteByte ((sbyte) gameOptions [i].render.cockpit.nWindowZoom);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].bPowerupsOnRadar);
	cf.WriteByte ((sbyte) gameOptions [i].input.keyboard.nRamp);
	cf.WriteByte ((sbyte) gameOptions [i].render.color.bAmbientLight);
	cf.WriteByte ((sbyte) gameOptions [i].ogl.bSetGammaRamp);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].nZoomMode);
	for (j = 0; j < 3; j++)
		cf.WriteByte ((sbyte) gameOptions [i].input.keyboard.bRamp [j]);
	if (!i) {
		cf.WriteByte ((sbyte) extraGameInfo [0].bEnhancedCTF);
		cf.WriteByte ((sbyte) extraGameInfo [0].bRobotsHitRobots);
		}
	cf.WriteByte ((sbyte) gameOptions [i].gameplay.nAutoSelectWeapon);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].bAutoDownload);
	cf.WriteByte ((sbyte) gameOptions [i].render.color.bGunLight);
	cf.WriteByte ((sbyte) gameStates.multi.bUseTracker);
	cf.WriteByte ((sbyte) gameOptions [i].gameplay.bFastRespawn);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].bDualMissileLaunch);
	cf.WriteByte ((sbyte) gameStates.app.nDifficultyLevel);
	cf.WriteByte ((sbyte) gameOptions [i].render.effects.bTransparent);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].bRobotsOnRadar);
	cf.WriteByte ((sbyte) gameOptions [i].render.bAllSegs);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].grWallTransparency);
	cf.WriteByte ((sbyte) gameOptions [i].input.mouse.sensitivity [0]);
	cf.WriteByte ((sbyte) gameOptions [i].multi.bUseMacros);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].bWiggle);
	cf.WriteByte ((sbyte) gameOptions [i].movies.nQuality);
	cf.WriteByte ((sbyte) gameOptions [i].render.color.bWalls);
	cf.WriteByte ((sbyte) gameOptions [i].input.bLinearJoySens);
	if (!i) {
		cf.WriteByte ((sbyte) extraGameInfo [0].nSpeedBoost);
		cf.WriteByte ((sbyte) extraGameInfo [0].bDropAllMissiles);
		cf.WriteByte ((sbyte) extraGameInfo [0].bImmortalPowerups);
		cf.WriteByte ((sbyte) extraGameInfo [0].bUseCameras);
		}
	cf.WriteByte ((sbyte) gameOptions [i].render.cameras.bFitToWall);
	cf.WriteByte ((sbyte) gameOptions [i].render.cameras.nFPS);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].nFusionRamp);
	cf.WriteByte ((sbyte) gameOptions [i].render.color.bUseLightmaps);
	cf.WriteByte ((sbyte) gameOptions [i].render.cockpit.bHUD);
	cf.WriteByte ((sbyte) gameOptions [i].render.color.nLightmapRange);
	if (!i) {
		cf.WriteByte ((sbyte) extraGameInfo [0].bMouseLook);
		cf.WriteByte ((sbyte) extraGameInfo [0].bMultiBosses);
		}
	cf.WriteByte ((sbyte) gameOptions [i].app.nVersionFilter);
	if (!i) {
		cf.WriteByte ((sbyte) extraGameInfo [0].bSmartWeaponSwitch);
		cf.WriteByte ((sbyte) extraGameInfo [0].bFluidPhysics);
		}
	cf.WriteByte ((sbyte) gameOptions [i].render.nQuality);
	cf.WriteByte ((sbyte) gameOptions [i].movies.bSubTitles);
	cf.WriteByte ((sbyte) gameOptions [i].render.textures.nQuality);
	cf.WriteInt (gameOptions [i].render.cameras.nSpeed);
	if (!i)
		cf.WriteByte (extraGameInfo [0].nWeaponDropMode);
	cf.WriteInt (gameOptions [i].menus.bSmartFileSearch);
	cf.WriteInt (GetDlTimeout ());
	if (!i) {
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.nCaptureVirusLimit);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.nCaptureTimeLimit);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.nMaxVirusCapacity);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.nBumpVirusCapacity);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.nBashVirusCapacity);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.nVirusGenTime);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.nVirusLifespan);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.nVirusStability);
		cf.WriteShort((short) extraGameInfo [0].entropy.nEnergyFillRate);
		cf.WriteShort((short) extraGameInfo [0].entropy.nShieldFillRate);
		cf.WriteShort((short) extraGameInfo [0].entropy.nShieldDamageRate);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.bRevertRooms);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.bDoConquerWarning);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.nOverrideTextures);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.bBrightenRooms);
		cf.WriteByte ((sbyte) extraGameInfo [0].entropy.bPlayerHandicap);

		cf.WriteByte ((sbyte) mpParams.nLevel);
		cf.WriteByte ((sbyte) mpParams.nGameType);
		cf.WriteByte ((sbyte) mpParams.nGameMode);
		cf.WriteByte ((sbyte) mpParams.nGameAccess);
		cf.WriteByte ((sbyte) mpParams.bShowPlayersOnAutomap);
		cf.WriteByte ((sbyte) mpParams.nDifficulty);
		cf.WriteInt(mpParams.nWeaponFilter);
		cf.WriteInt(mpParams.nReactorLife);
		cf.WriteByte ((sbyte) mpParams.nMaxTime);
		cf.WriteByte ((sbyte) mpParams.nKillGoal);
		cf.WriteByte ((sbyte) mpParams.bInvul);
		cf.WriteByte ((sbyte) mpParams.bMarkerView);
		cf.WriteByte ((sbyte) mpParams.bIndestructibleLights);
		cf.WriteByte ((sbyte) mpParams.bBrightPlayers);
		cf.WriteByte ((sbyte) mpParams.bShowAllNames);
		cf.WriteByte ((sbyte) mpParams.bShortPackets);
		cf.WriteByte ((sbyte) mpParams.nPPS);
		cf.WriteInt(mpParams.udpClientPort);
		cf.Write(mpParams.szServerIpAddr, 16, 1);
		}
	cf.WriteByte ((sbyte) gameOptions [i].render.nMeshQuality);
	if (!i) {
		cf.WriteByte ((sbyte) extraGameInfo [1].bRotateLevels);
		cf.WriteByte ((sbyte) extraGameInfo [1].bDisableReactor);
		}
	for (j = 0; j < 4; j++)
		cf.WriteByte ((sbyte) gameOptions [0].input.joystick.deadzones [j]);
	for (j = 0; j < 4; j++)
		cf.WriteByte ((sbyte) gameOptions [0].input.joystick.sensitivity [j]);
	cf.WriteByte ((sbyte) gameOptions [i].input.joystick.bSyncAxes);
	if (!i) {
		cf.WriteByte ((sbyte) extraGameInfo [1].bDualMissileLaunch);
		cf.WriteByte ((sbyte) extraGameInfo [1].bMouseLook);
		}
	for (j = 0; j < 3; j++)
		cf.WriteByte ((sbyte) gameOptions [0].input.mouse.sensitivity [j]);
	cf.WriteByte ((sbyte) gameOptions [i].input.mouse.bSyncAxes);
	cf.WriteByte ((sbyte) gameOptions [i].render.color.bMix);
	cf.WriteByte ((sbyte) gameOptions [i].render.color.bCap);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].nWeaponIcons);
	cf.WriteByte ((sbyte) gameOptions [i].render.weaponIcons.bSmall);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].bAutoBalanceTeams);
	cf.WriteByte ((sbyte) gameOptions [i].movies.bResize);
	cf.WriteByte ((sbyte) gameOptions [i].menus.bShowLevelVersion);
	cf.WriteByte ((sbyte) gameOptions [i].render.weaponIcons.nSort);
	cf.WriteByte ((sbyte) gameOptions [i].render.weaponIcons.bShowAmmo);
	cf.WriteByte ((sbyte) gameOptions [i].input.mouse.bUse);
	cf.WriteByte ((sbyte) gameOptions [i].input.joystick.bUse);
	cf.WriteByte ((sbyte) gameOptions [i].input.bUseHotKeys);
	if (!i) {
		cf.WriteByte ((sbyte) extraGameInfo [0].bSafeUDP);
		cf.Write(mpParams.szServerIpAddr + 16, 6, 1);
		}
	cf.WriteByte ((sbyte) gameOptions [i].render.cockpit.bTextGauges);
	cf.WriteByte ((sbyte) gameOptions [i].render.cockpit.bScaleGauges);
	cf.WriteByte ((sbyte) gameOptions [i].render.weaponIcons.bEquipment);
	cf.WriteByte ((sbyte) gameOptions [i].render.weaponIcons.alpha);
	cf.WriteByte ((sbyte) gameOptions [i].render.cockpit.bFlashGauges);
	if (!i) {
		for (j = 0; j < 2; j++)
			cf.WriteByte ((sbyte) extraGameInfo [j].bFastPitch);
		cf.WriteInt (gameStates.multi.nConnection);
		cf.WriteByte ((sbyte) extraGameInfo [0].bUseParticles);
		}
	cf.WriteInt (gameOptions [i].render.particles.nDens [0]);
	cf.WriteInt (gameOptions [i].render.particles.nSize [0]);
	cf.WriteByte ((sbyte) gameOptions [i].render.particles.bPlayers);
	cf.WriteByte ((sbyte) gameOptions [i].render.particles.bRobots);
	cf.WriteByte ((sbyte) gameOptions [i].render.particles.bMissiles);
	if (!i) {
		cf.WriteByte ((sbyte) extraGameInfo [0].bDamageExplosions);
		cf.WriteByte ((sbyte) extraGameInfo [0].bThrusterFlames);
		}
	cf.WriteByte ((sbyte) 0);
	cf.WriteByte ((sbyte) gameOptions [i].gameplay.bShieldWarning);
	cf.WriteByte ((sbyte) gameOptions [i].app.bExpertMode);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].bShadows);
	cf.WriteInt (gameOptions [i].render.shadows.nLights);
	if (!i) {
		cf.WriteInt (gameStates.ogl.nContrast);
		cf.WriteByte ((sbyte) extraGameInfo [0].bPlayerShield);
		}
	cf.WriteByte ((sbyte) gameOptions [i].gameplay.bInventory);
	cf.WriteInt (gameOptions [i].input.mouse.bJoystick);
	if (!i) {
		cf.WriteInt (displayModeInfo [NUM_DISPLAY_MODES].w);
		cf.WriteInt (displayModeInfo [NUM_DISPLAY_MODES].h);
		}
	cf.WriteInt (gameOptions [i].render.cockpit.bMouseIndicator);
	cf.WriteInt (extraGameInfo [i].bTeleporterCams);
	cf.WriteInt (gameOptions [i].render.cockpit.bSplitHUDMsgs);
	cf.WriteByte (gameOptions [i].input.joystick.deadzones [4]);
	cf.WriteByte (gameOptions [i].input.joystick.sensitivity [4]);
	if (!i) {
		tMonsterballForce *pf = extraGameInfo [0].monsterball.forces;
		cf.WriteByte (extraGameInfo [0].monsterball.nBonus);
		for (h = 0; h < MAX_MONSTERBALL_FORCES; h++, pf++) {
			cf.WriteByte (pf->nWeaponId);
			cf.WriteByte (pf->nForce);
			}
		}
	if (!i)
		cf.WriteByte (extraGameInfo [0].monsterball.nSizeMod);
	cf.WriteInt (gameOptions [i].render.nLightingMethod);
	cf.WriteInt (gameOptions [i].ogl.bLightObjects);
	cf.WriteInt (gameOptions [i].ogl.nMaxLights);
	cf.WriteByte (extraGameInfo [i].bDarkness);
	cf.WriteByte (extraGameInfo [i].bTeamDoors);
	cf.WriteByte (extraGameInfo [i].bEnableCheats);
	cf.WriteByte (extraGameInfo [i].bTargetIndicators);
	cf.WriteByte (extraGameInfo [i].bDamageIndicators);
	cf.WriteByte (extraGameInfo [i].bFriendlyIndicators);
	cf.WriteByte (extraGameInfo [i].bCloakedIndicators);
	cf.WriteByte (extraGameInfo [i].bHeadlights);
	cf.WriteByte (extraGameInfo [i].bPowerupLights);
	cf.WriteByte (extraGameInfo [i].nSpotSize);
	cf.WriteByte (extraGameInfo [i].bTowFlags);
	cf.WriteByte (gameOptions [i].render.particles.bDecreaseLag);
	cf.WriteByte (gameOptions [i].render.effects.bAutoTransparency);
	if (!i) {
		cf.WriteByte (extraGameInfo [i].bUseHitAngles);
		cf.WriteByte (extraGameInfo [i].bLightTrails);
		}
	cf.WriteInt (gameOptions [i].render.particles.bSyncSizes);
	for (j = 1; j < 4; j++) {
		cf.WriteInt (gameOptions [i].render.particles.nDens [j]);
		cf.WriteInt (gameOptions [i].render.particles.nSize [j]);
		}
	if (!i) {
		cf.WriteByte (extraGameInfo [i].bTracers);
		cf.WriteByte (extraGameInfo [i].bShockwaves);
		}
	cf.WriteByte (gameOptions [i].render.particles.bDisperse);
	cf.WriteByte (extraGameInfo [i].bTagOnlyHitObjs);
	for (j = 0; j < 4; j++)
		cf.WriteInt (gameOptions [i].render.particles.nLife [j]);
	cf.WriteByte (gameOptions [i].render.shadows.bRobots);
	cf.WriteByte (gameOptions [i].render.shadows.bMissiles);
	cf.WriteByte (gameOptions [i].render.shadows.bReactors);
	cf.WriteByte (gameOptions [i].render.shadows.bPlayers);
	cf.WriteByte (gameOptions [i].render.shadows.bFast);
	cf.WriteByte (gameOptions [i].render.shadows.nReach);
	cf.WriteByte (gameOptions [i].render.shadows.nClip);
	cf.WriteByte (gameOptions [i].render.powerups.b3D);
	cf.WriteByte (gameOptions [i].render.powerups.nSpin);
	cf.WriteByte (gameOptions [i].gameplay.bIdleAnims);
	if (!i)
		cf.WriteByte (gameStates.app.nDifficultyLevel);
	cf.WriteByte (gameOptions [i].demo.bOldFormat);
	cf.WriteByte (gameOptions [i].render.cockpit.bObjectTally);
	if (!i)
		cf.WriteByte (extraGameInfo [0].nLightRange);
	if (i)
		cf.WriteByte (extraGameInfo [i].bCompetition);
	cf.WriteByte (extraGameInfo [i].bFlickerLights);
	if (!i) {
		cf.WriteByte (extraGameInfo [i].bSmokeGrenades);
		cf.WriteByte (extraGameInfo [i].nMaxSmokeGrenades);
		cf.WriteByte (extraGameInfo [i].nMslTurnSpeed);
		}
	cf.WriteByte ((sbyte) gameOptions [i].render.particles.bDebris);
	cf.WriteByte ((sbyte) gameOptions [i].render.particles.bStatic);
	cf.WriteByte ((sbyte) gameOptions [i].gameplay.nAIAwareness);
	cf.WriteByte ((sbyte) extraGameInfo [i].nCoopPenalty);
	cf.WriteByte ((sbyte) extraGameInfo [i].bKillMissiles);
	cf.WriteByte ((sbyte) extraGameInfo [i].nHitboxes);
	cf.WriteByte ((sbyte) gameOptions [i].render.cockpit.bPlayerStats);
	cf.WriteByte ((sbyte) gameOptions [i].render.coronas.bUse);
	cf.WriteByte ((sbyte) gameOptions [i].render.automap.bTextured);
	cf.WriteByte ((sbyte) gameOptions [i].render.automap.bBright);
	cf.WriteByte ((sbyte) gameOptions [i].render.automap.bCoronas);
	cf.WriteByte ((sbyte) gameOptions [i].render.automap.nColor);
	if (!i)
		cf.WriteByte ((sbyte) extraGameInfo [0].nRadar);
	cf.WriteByte ((sbyte) gameOptions [i].render.automap.nRange);
	cf.WriteByte ((sbyte) gameOptions [i].render.automap.bParticles);
	cf.WriteInt (gameOptions [i].render.nDebrisLife);
	cf.WriteByte ((sbyte) extraGameInfo [i].bMslLockIndicators);
	cf.WriteByte ((sbyte) gameOpts->render.cockpit.bRotateMslLockInd);
// end of D2X-XL stuff
	}
}
#endif
//------------------------------------------------------------------------------

//write out CPlayerData's saved games.  returns errno (0 == no error)
int WritePlayerFile (void)
{
	CFile	cf;
	char	filename [FILENAME_LEN];		// because of ":gameData.multiplayer.players:" path
	char	buf [128];
	int	funcRes, i;

funcRes = WriteConfigFile ();
sprintf (filename,"%s.plr",LOCALPLAYER.callsign);
cf.Open (filename, gameFolders.szProfDir, "wb", 0);
#if 0
//check filename
if (cf.file && isatty(fileno ()) {
	//if the callsign is the name of a tty device, prepend a char
	fclose ();
	sprintf(filename,"$%.7s.plr",LOCALPLAYER.callsign);
	&cf = fopen(filename,"wb");
	}
#endif
if (!cf.File ())
	return errno;
funcRes = EZERO;
//Write out CPlayerData's info
cf.WriteInt (SAVE_FILE_ID);
cf.WriteShort (PLAYER_FILE_VERSION);
cf.WriteShort ((short) gameData.render.window.w);
cf.WriteShort ((short) gameData.render.window.h);
cf.WriteByte ((sbyte) gameStates.app.nDifficultyLevel);
cf.WriteByte ((sbyte) gameOptions [0].gameplay.nAutoLeveling);
cf.WriteByte ((sbyte) gameOptions [0].render.cockpit.bReticle);
cf.WriteByte ((sbyte) ((gameStates.render.cockpit.nModeSave != -1)?gameStates.render.cockpit.nModeSave:gameStates.render.cockpit.nMode));   //if have saved mode, write it instead of letterbox/rear view
cf.WriteByte ((sbyte) gameStates.video.nDefaultDisplayMode);
cf.WriteByte ((sbyte) gameOptions [0].render.cockpit.bMissileView);
cf.WriteByte ((sbyte) extraGameInfo [0].headlight.bAvailable);
cf.WriteByte ((sbyte) gameOptions [0].render.cockpit.bGuidedInMainView);
cf.WriteByte ((sbyte) 0);	//place holder for an obsolete value
//write higest level info
Assert(nHighestLevels <= MAX_MISSIONS);
cf.WriteShort (nHighestLevels);
if ((cf.Write (highestLevels, sizeof (hli), nHighestLevels) != nHighestLevels)) {
	funcRes = errno;
	cf.Close ();
	return funcRes;
	}

if ((cf.Write(gameData.multigame.msg.szMacro, MAX_MESSAGE_LEN, 4) != 4)) {
	funcRes = errno;
	cf.Close ();
	return funcRes;
	}

//write KConfig info
dosControlType = gameConfig.nControlType;
if (cf.Write(controlSettings.custom, MAX_CONTROLS * CONTROL_MAX_TYPES, 1) != 1)
	funcRes = errno;
else if (cf.Write(&dosControlType, sizeof(ubyte), 1) != 1)
	funcRes = errno;
else if (cf.Write(&winControlType, sizeof(ubyte), 1) != 1)
	funcRes = errno;
else if (cf.Write(gameOptions [0].input.joystick.sensitivity, sizeof(ubyte), 1) != 1)
	funcRes = errno;

for (i = 0; i < 11; i++) {
	cf.Write (primaryOrder + i, sizeof(ubyte), 1);
	cf.Write (secondaryOrder + i, sizeof(ubyte), 1);
	}
cf.WriteInt(gameStates.render.cockpit.n3DView [0]);
cf.WriteInt(gameStates.render.cockpit.n3DView [1]);
cf.WriteInt(networkData.nNetLifeKills);
cf.WriteInt(networkData.nNetLifeKilled);
i = GetLifetimeChecksum (networkData.nNetLifeKills, networkData.nNetLifeKilled);
#if TRACE
con_printf (CONDBG,"Writing: Lifetime checksum is %d\n",i);
#endif
cf.WriteInt (i);
//write guidebot name
cf.WriteString(gameData.escort.szRealName);
strcpy(buf, "DOS joystick");
cf.WriteString(buf);  // Write out current joystick for player.

cf.Write(controlSettings.d2xCustom, MAX_HOTKEY_CONTROLS, 1);
// write D2X-XL stuff
#if 1
WriteParams ();
#else
WriteBinD2XParams ();
#endif

if (cf.Close ())
	funcRes = errno;
if (funcRes != EZERO) {
	cf.Delete (filename, gameFolders.szProfDir);         //delete bogus &cf
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, "%s\n\n%s",TXT_ERROR_WRITING_PLR, strerror(funcRes));
	}
return funcRes;
}

//------------------------------------------------------------------------------
//update the CPlayerData's highest level.  returns errno (0 == no error)
int UpdatePlayerFile()
{
	int ret = ReadPlayerFile(0);

if ((ret != EZERO) && (ret != ENOENT))		//if file doesn't exist, that's ok
	return ret;
return WritePlayerFile();
}

//------------------------------------------------------------------------------

int GetLifetimeChecksum (int a,int b)
 {
  int num;

// confusing enough to beat amateur disassemblers? Lets hope so
num = (a << 8 ^ b);
num ^= (a | b);
num *= num >> 2;
return num;
}

//------------------------------------------------------------------------------
//
// New Game sequencing functions
//

//pairs of chars describing ranges
char playername_allowed_chars [] = "azAZ09__--";

int MakeNewPlayerFile (int bAllowAbort)
{
	int x;
	char filename [FILENAME_LEN];
	tMenuItem m;
	char text [CALLSIGN_LEN+1]="";

strncpy (text, LOCALPLAYER.callsign,CALLSIGN_LEN);

try_again:

memset (&m, 0, sizeof (m));
m.nType = NM_TYPE_INPUT;
m.text_len = 8;
m.text = text;

nmAllowedChars = playername_allowed_chars;
x = ExecMenu (NULL, TXT_ENTER_PILOT_NAME, 1, &m, NULL, NULL);
nmAllowedChars = NULL;
if (x < 0) {
	if (bAllowAbort) return 0;
	goto try_again;
	}
if (text [0] == 0)	//null string
	goto try_again;
sprintf (filename, "%s.plr", text);

CFile cf;
if (cf.Exist (filename,gameFolders.szProfDir, 0)) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s '%s' %s", TXT_PLAYER, text, TXT_ALREADY_EXISTS);
	goto try_again;
	}
if (!NewPlayerConfig ())
	goto try_again;			// They hit Esc during New CPlayerData config
strncpy (LOCALPLAYER.callsign, text, CALLSIGN_LEN);
WritePlayerFile ();
return 1;
}

//------------------------------------------------------------------------------

//Inputs the CPlayerData's name, without putting up the background screen
int SelectPlayer (void)
{
	static int bStartup = 1;
	int 	i,j, bAutoPlr;
	char 	filename [FILENAME_LEN];
	char	filespec [FILENAME_LEN];
	int 	bAllowAbort = !bStartup;

if (LOCALPLAYER.callsign [0] == 0)	{
	//---------------------------------------------------------------------
	// Set default config options in case there is no config file
	// kcKeyboard, kc_joystick, kcMouse are statically defined.
	gameOpts->input.joystick.sensitivity [0] =
	gameOpts->input.joystick.sensitivity [1] =
	gameOpts->input.joystick.sensitivity [2] =
	gameOpts->input.joystick.sensitivity [3] = 8;
	gameOpts->input.mouse.sensitivity [0] =
	gameOpts->input.mouse.sensitivity [1] =
	gameOpts->input.mouse.sensitivity [2] = 8;
	gameConfig.nControlType = CONTROL_NONE;
	for (i = 0; i < CONTROL_MAX_TYPES; i++)
		for (j = 0; j < MAX_CONTROLS; j++)
			controlSettings.custom [i][j] = controlSettings.defaults [i][j];
	KCSetControls (0);
	//----------------------------------------------------------------

	// Read the last CPlayerData's name from config file, not lastplr.txt
	strncpy (LOCALPLAYER.callsign, gameConfig.szLastPlayer, CALLSIGN_LEN);
	if (gameConfig.szLastPlayer [0] == 0)
		bAllowAbort = 0;
	}
if ((bAutoPlr = gameData.multiplayer.autoNG.bValid))
	strncpy (filename, gameData.multiplayer.autoNG.szPlayer, 8);
else if ((bAutoPlr = bStartup && (i = FindArg ("-player")) && *pszArgList [++i]))
	strncpy (filename, pszArgList [i], 8);
if (bAutoPlr) {
	char *psz;
	strlwr (filename);
	if (!(psz = strchr (filename, '.')))
		for (psz = filename; psz - filename < 8; psz++)
			if (!*psz || ::isspace (*psz))
				break;
		*psz = '\0';
	goto got_player;
	}

callMenu:

bStartup = 0;
sprintf (filespec, "%s%s*.plr", gameFolders.szProfDir, *gameFolders.szProfDir ? "/" : "");
if (!ExecMenuFileSelector (TXT_SELECT_PILOT, filespec, filename, bAllowAbort))	{
	if (bAllowAbort) {
		return 0;
		}
	goto callMenu; //return 0;		// They hit Esc in file selector
	}

got_player:

bStartup = 0;
if (filename [0] == '<')	{
	// They selected 'create new pilot'
	if (!MakeNewPlayerFile (1))
		//return 0;		// They hit Esc during enter name stage
		goto callMenu;
	}
else
	strncpy (LOCALPLAYER.callsign, filename, CALLSIGN_LEN);
if (ReadPlayerFile (0) != EZERO)
	goto callMenu;
KCSetControls (0);
SetDisplayMode (gameStates.video.nDefaultDisplayMode, 1);
WriteConfigFile ();		// Update lastplr
D2SetCaption ();
return 1;
}

//------------------------------------------------------------------------------

