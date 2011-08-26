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

#include "descent.h"
#include "error.h"
#include "pa_enabl.h"
#include "strutil.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "game.h"
#include "loadgame.h"
#include "player.h"
#include "joy.h"
#include "kconfig.h"
#include "audio.h"
#include "crypt.h"
#include "menu.h"
#include "joydefs.h"
#include "palette.h"
#include "multi.h"
#include "menu.h"
#include "config.h"
#include "text.h"
#include "mono.h"
#include "savegame.h"
#include "cockpit.h"
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
#include "cockpit.h"
#include "playerprofile.h"
#include "tracker.h"
#include "gr.h"

CPlayerProfile profile;
CDisplayModeInfo customDisplayMode;

void DefaultAllSettings (void);
int FindDisplayMode (short w, short h);

//------------------------------------------------------------------------------

#define SAVE_FILE_ID			MAKE_SIG('D','P','L','R')

#if defined(_WIN32_WCE)
# define errno -1
# define ENOENT -1
# define strerror(x) "Unknown Error"
#endif

int GetLifetimeChecksum (int a,int b);

typedef struct hli {
	char	shortname [9];
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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define issign(_c)	(((_c) == '-') || ((_c) == '+'))

//------------------------------------------------------------------------------

int CParam::Set (const char *pszIdent, const char *pszValue)
{
int nVal = atoi (pszValue);
switch (nSize) {
	case 1:
		if (!(::isdigit (*pszValue) || issign (*pszValue)) || (nVal < SCHAR_MIN) || (nVal > SCHAR_MAX))
			return 0;
		*reinterpret_cast<sbyte*> (valP) = (sbyte) nVal;
		break;
	case 2:
		if (!(::isdigit (*pszValue) || issign (*pszValue))  || (nVal < SHRT_MIN) || (nVal > SHRT_MAX))
			return 0;
		*reinterpret_cast<short*> (valP) = (short) nVal;
		break;
	case 4:
		if (!(::isdigit (*pszValue) || issign (*pszValue)))
			return 0;
		*reinterpret_cast<int*> (valP) = (int) nVal;
		break;
	default:
		strncpy (reinterpret_cast<char*> (valP), pszValue, nSize);
		break;
	}
return 1;
}

//------------------------------------------------------------------------------

int CParam::Save (CFile& cf)
{
	char	szVal [200];

switch (nSize) {
	case 1:
		sprintf (szVal, "=%d\n", *reinterpret_cast<sbyte*> (valP));
		break;
	case 2:
		sprintf (szVal, "=%d\n", *reinterpret_cast<short*> (valP));
		break;
	case 4:
		sprintf (szVal, "=%d\n", *reinterpret_cast<int*> (valP));
		break;
	default:
		sprintf (szVal, "=%s\n", valP);
		break;
	}
cf.Write (szTag, 1, (int) strlen (szTag));
cf.Write (szVal, 1, (int) strlen (szVal));
fflush (cf.File ());
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CPlayerProfile::Init (void)
{ 
paramList = lastParam = NULL; 
bRegistered = false;
Create ();
Setup ();
}

//------------------------------------------------------------------------------

void CPlayerProfile::Destroy (void)
{
	CParam	*pp;

while (paramList) {
	pp = paramList;
	paramList = paramList->next;
	delete pp;
	}
}

//------------------------------------------------------------------------------

char* CPlayerProfile::MakeTag (char *pszTag, const char *pszIdent, int i, int j)
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

int CPlayerProfile::Register (void *valP, const char *pszIdent, int i, int j, ubyte nSize)
{
	char		szTag [200];
	int		l;
	CParam	*pp;

l = (int) strlen (MakeTag (szTag, pszIdent, i, j));
pp = reinterpret_cast<CParam*> (new ubyte [sizeof (CParam) + l]);
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

#define RP(_v,_i,_j)	Register (reinterpret_cast<void*> (&(_v)), #_v, _i, _j, sizeof (_v))

//------------------------------------------------------------------------------
// returns number of config items with identical ids before the current one

int CPlayerProfile::FindInConfig (kcItem *cfgP, int nItems, int iItem, const char *pszText)
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

void CPlayerProfile::RegisterConfig (kcItem *cfgP, int nItems, const char *pszId)
{
	char	szTag [200], *p;
	int	i, j = 0;

strcpy (szTag, pszId);
p = szTag + strlen (szTag);

for (i = 0; i < nItems; i++) {
#if 0
	sprintf (p, "%s.type", cfgP [i].text);
	Register (&cfgP [i].nType, szTag, 0, 0, sizeof (cfgP [i].nType));
#endif
	j = FindInConfig (cfgP, nItems, i, cfgP [i].text);
	if (j < 0) {
		sprintf (p, "%s.value", cfgP [i].text);
		Register (&cfgP [i].value, szTag, 0, 0, sizeof (cfgP [i].value));
		}
	else {
		sprintf (p, "%s[%d].value", cfgP [i].text, j);
		Register (&cfgP [i].value, szTag, j, 0, sizeof (cfgP [i].value));
		}
	}
}

//------------------------------------------------------------------------------

void CPlayerProfile::Create (void)
{
	uint	i, j;

if (bRegistered)
	return;
bRegistered = true;

for (i = 0; i < 2; i++) {
	if (i) {	// i == 1: nostalgia/pure D2 mode
		}
	else {
		RP (gameData.render.window.w, 0, 0);
		RP (gameData.render.window.h, 0, 0);
		RP (gameStates.app.iDownloadTimeout, 0, 0);
		RP (gameStates.render.bShowFrameRate, 0, 0);
		RP (gameStates.render.bShowTime, 0, 1);
		RP (gameStates.render.cockpit.nType, 0, 0);
		RP (gameStates.video.nDefaultDisplayMode, 0, 0);
		RP (networkData.nNetLifeKills, 0, 0);
		RP (networkData.nNetLifeKilled, 0, 0);
		RP (gameData.app.nLifetimeChecksum, 0, 0);
		for (j = 0; j < rtTaskCount; j++)
			RP (gameData.app.bUseMultiThreading [j], j, 0);
		RP (gameData.escort.szName, 0, 0);
		for (j = 0; j < 4; j++)
			RP (gameData.multigame.msg.szMacro [j], j, 0);

		RP (customDisplayMode.w, 0, 0);
		RP (customDisplayMode.h, 0, 0);

		RP (gameStates.app.nDifficultyLevel, 0, 0);
		RP (ogl.m_states.nContrast, 0, 0);
		RP (gameStates.multi.nConnection, 0, 0);
		RP (tracker.m_bUse, 0, 0);

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
		RP (mpParams.nScoreGoal, 0, 0);
		RP (mpParams.bInvul, 0, 0);
		RP (mpParams.bMarkerView, 0, 0);
		RP (mpParams.bIndestructibleLights, 0, 0);
		RP (mpParams.bBrightPlayers, 0, 0);
		RP (mpParams.bShowAllNames, 0, 0);
		RP (mpParams.bShortPackets, 0, 0);
		RP (mpParams.nPPS, 0, 0);
		RP (mpParams.udpPorts [0], 0, 0);
		RP (mpParams.udpPorts [1], 1, 0);
		RP (mpParams.szServerIpAddr, 0, 0);

		RP (extraGameInfo [i].bAutoBalanceTeams, 0, 0);
		RP (extraGameInfo [i].bAutoDownload, 0, 0);
		RP (extraGameInfo [i].bDamageExplosions, 0, 0);
		RP (extraGameInfo [1].bDisableReactor, 0, 0);
		RP (extraGameInfo [i].bDropAllMissiles, 0, 0);
		RP (extraGameInfo [i].bEnhancedCTF, 0, 0);
		RP (extraGameInfo [i].bFixedRespawns, 0, 0);
		RP (extraGameInfo [i].nSpawnDelay, 0, 0); // / 1000
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
		RP (extraGameInfo [i].bUseLightning, 0, 0);
		RP (extraGameInfo [i].bUseHitAngles, 0, 0);
		RP (extraGameInfo [i].bWiggle, 0, 0);
		RP (extraGameInfo [i].bGatlingSpeedUp, i, 0);
		RP (extraGameInfo [i].nLightRange, 0, 0);
		RP (extraGameInfo [i].nMaxSmokeGrenades, 0, 0);
		RP (extraGameInfo [i].nMslTurnSpeed, 0, 0);
		RP (extraGameInfo [i].nMslStartSpeed, 0, 0);
		RP (extraGameInfo [i].nRadar, 0, 0);
		RP (extraGameInfo [i].nDrag, 0, 0);
		RP (extraGameInfo [i].nWeaponDropMode, 0, 0);
		RP (extraGameInfo [i].nWeaponIcons, 0, 0);
		RP (extraGameInfo [i].nZoomMode, 0, 0);
		RP (extraGameInfo [i].bShowWeapons, 0, 0);
		RP (extraGameInfo [i].nFusionRamp, 0, 0);

		RP (extraGameInfo [i].entropy.nCaptureVirusThreshold, 0, 0);
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
		RP (extraGameInfo [i].entropy.bDoCaptureWarning, 0, 0);
		RP (extraGameInfo [i].entropy.nOverrideTextures, 0, 0);
		RP (extraGameInfo [i].entropy.bBrightenRooms, 0, 0);
		RP (extraGameInfo [i].entropy.bPlayerHandicap, 0, 0);

		RP (extraGameInfo [i].loadout.nGuns, 0, 0);
		RP (extraGameInfo [i].loadout.nDevice, 0, 0);
		for (j = 0; j < 10; j++)
			RP (extraGameInfo [i].loadout.nMissiles [j], i, j);

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
		RP (gameOptions [i].render.particles.nQuality, i, 0);
		RP (gameOptions [i].render.particles.bStatic, i, 0);
		RP (gameOptions [i].render.particles.bPlasmaTrails, i, 0);
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
		RP (gameOptions [i].input.keyboard.nType, i, 0);
		RP (gameOptions [i].input.keyboard.nRamp, i, 0);
		RP (gameOptions [i].ogl.nMaxLightsPerPass, i, 0);
		RP (gameOptions [i].render.nLightingMethod, i, 0);

		RP (gameOptions [i].render.nQuality, i, 0);
		RP (gameOptions [i].render.effects.bEnabled, i, 0);
		RP (gameOptions [i].render.effects.bGlow, i, 0);
		RP (gameOptions [i].render.effects.bSoftParticles, i, 0);
		RP (gameOptions [i].render.effects.nShrapnels, i, 0);
		RP (gameOptions [i].render.coronas.bUse, i, 0);
		RP (gameOptions [i].render.coronas.nStyle, i, 0);
		RP (gameOptions [i].render.effects.bEnergySparks, i, 0);
		RP (gameOptions [i].render.effects.nShockwaves, i, 0);
		RP (gameOptions [i].render.automap.bTextured, i, 0);
		RP (gameOptions [i].render.automap.bBright, i, 0);

		RP (gameOptions [i].render.color.nLevel, i, 0);

		RP (gameOptions [i].render.cockpit.bMouseIndicator, i, 0);
		RP (gameOptions [i].render.cockpit.bObjectTally, i, 0);
		RP (gameOptions [i].render.cockpit.bPlayerStats, i, 0);
		RP (gameOptions [i].render.cockpit.bTextGauges, i, 0);
		RP (gameOptions [i].render.cockpit.nWindowPos, i, 0);
		RP (gameOptions [i].render.cockpit.nWindowSize, i, 0);
		RP (gameOptions [i].render.cockpit.nWindowZoom, i, 0);
		RP (gameOptions [i].render.cockpit.nRadarPos, i, 0);
		RP (gameOptions [i].render.cockpit.nRadarSize, i, 0);
		RP (gameOptions [i].render.cockpit.nRadarRange, i, 0);
		RP (gameOptions [i].render.cockpit.nRadarColor, i, 0);
		RP (gameOptions [i].render.cockpit.nRadarStyle, i, 0);
		RP (gameOptions [i].render.color.bUseLightmaps, i, 0);
		RP (gameOptions [i].render.color.nLightmapRange, i, 0);

		RP (gameOptions [i].render.powerups.b3D, i, 0);
		RP (gameOptions [i].render.powerups.b3DShields, i, 0);
		RP (gameOptions [i].render.shadows.nClip, i, 0);
		RP (gameOptions [i].render.shadows.nReach, i, 0);
		RP (gameOptions [i].render.shadows.nLights, i, 0);
		RP (gameOptions [i].render.nMaxFPS, i, 0);
		RP (gameOptions [i].render.nImageQuality, i, 0);
		RP (gameOptions [i].render.stereo.nGlasses, i, 0);
		RP (gameOptions [i].render.stereo.nMethod, i, 0);
		RP (gameOptions [i].render.stereo.nScreenDist, i, 0);
		RP (gameOptions [i].render.stereo.xSeparation, i, 0);
		RP (gameOptions [i].render.stereo.bEnhance, i, 0);
		RP (gameOptions [i].render.stereo.bColorGain, i, 0);
		RP (gameOptions [i].render.stereo.bDeghost, i, 0);
		RP (gameOptions [i].render.stereo.bBrighten, i, 0);
		RP (gameOptions [i].render.cameras.bHires, i, 0);
		RP (gameOptions [i].render.cockpit.bFlashGauges, i, 0);
		RP (gameOptions [i].demo.bOldFormat, i, 0);
		RP (gameOptions [i].app.bEpilepticFriendly, i, 0);
		RP (gameOptions [i].app.bColorblindFriendly, i, 0);
		RP (gameOptions [i].app.bNotebookFriendly, i, 0);
		RP (gameOptions [i].app.bExpertMode, i, 0);
		RP (gameOptions [i].app.bEnableMods, i, 0);
		RP (gameOptions [i].app.bEpilepticFriendly, i, 0);
		RP (gameOptions [i].app.bColorblindFriendly, i, 0);
		RP (gameOptions [i].app.bNotebookFriendly, i, 0);
		RP (gameOptions [i].sound.bFadeMusic, i, 1);
		RP (gameOptions [i].sound.bLinkVolumes, i, 1);
		RP (gameOptions [i].sound.bGatling, i, 0);
		RP (gameOptions [i].sound.bMissiles, i, 0);
		RP (gameOptions [i].sound.bShip, i, 0);
		RP (gameOptions [i].gameplay.bInventory, i, 0);
		RP (gameOptions [i].gameplay.bNoThief, i, 0);
		RP (gameOptions [i].gameplay.nShip [0], i, 0);
		RP (gameOptions [i].gameplay.bShieldWarning, i, 0);
		RP (gameOptions [i].gameplay.nAIAwareness, i, 0);
		RP (gameOptions [i].gameplay.nAIAggressivity, i, 0);
		RP (gameOptions [i].gameplay.nAutoSelectWeapon, i, 0);
		RP (gameOptions [i].gameplay.nSlowMotionSpeedup, i, 0);
		RP (gameStates.sound.audio.nMaxChannels, 0, 128);

		for (j = 0; j < 5; j++) {
			RP (gameOptions [i].render.particles.nDens [j], i, j);
			RP (gameOptions [i].render.particles.nSize [j], i, j);
			RP (gameOptions [i].render.particles.nLife [j], i, j);
			RP (gameOptions [i].render.particles.nAlpha [j], i, j);
			}
		RP (gameOptions [i].render.particles.bAuxViews, i, 0);
		RP (gameOptions [i].render.particles.bMonitors, i, 0);
		RP (gameOptions [i].render.particles.bDecreaseLag, i, 0);
		RP (gameOptions [i].render.particles.bDebris, i, 0);
		RP (gameOptions [i].render.particles.bDisperse, i, 0);
		RP (gameOptions [i].render.particles.bRotate, i, 0);
		RP (gameOptions [i].render.particles.bMissiles, i, 0);
		RP (gameOptions [i].render.particles.bPlayers, i, 0);
		RP (gameOptions [i].render.particles.bRobots, i, 0);
#if 0
		RP (gameOpts->render.cockpit.bWideDisplays, 0, 1);
		RP (gameOptions [i].render.cockpit.bGuidedInMainView, 0, 0);
		RP (extraGameInfo [i].bRotateMarkers, i, 0);
		RP (extraGameInfo [i].bBrightObjects, i, 0);
		RP (extraGameInfo [i].grWallTransparency, 0, 0);
		RP (extraGameInfo [i].nOmegaRamp, 0, 0);
		RP (extraGameInfo [i].nSpeedBoost, 0, 0);
		RP (gameOptions [i].render.particles.bBubbles, i, 0);
		RP (gameOptions [i].render.particles.bWobbleBubbles, i, 1);
		RP (gameOptions [i].render.particles.bWiggleBubbles, i, 1);
		RP (gameOptions [i].render.particles.bSyncSizes, i, 0);
		RP (gameOptions [i].render.particles.bSort, i, 0);
		RP (gameOptions [i].ogl.bLightObjects, i, 0);
		RP (gameOptions [i].ogl.bHeadlight, i, 0);
		RP (gameOptions [i].ogl.bLightPowerups, i, 0);
		RP (gameOptions [i].ogl.bObjLighting, i, 0);
		RP (gameOptions [i].ogl.nMaxLightsPerFace, i, 0);
		RP (gameOptions [i].ogl.nMaxLightsPerObject, i, 0);
		RP (gameOptions [i].render.nDebrisLife, i, 0);
		RP (gameOptions [i].render.effects.bAutoTransparency, i, 0);
		RP (gameOptions [i].render.effects.bMovingSparks, i, 0);
		RP (gameOptions [i].render.coronas.bShots, i, 0);
		RP (gameOptions [i].render.coronas.bPowerups, i, 0);
		RP (gameOptions [i].render.coronas.bWeapons, i, 0);
		RP (gameOptions [i].render.coronas.nIntensity, i, 0);
		RP (gameOptions [i].render.coronas.nObjIntensity, i, 0);
		RP (gameOptions [i].render.coronas.bAdditive, i, 0);
		RP (gameOptions [i].render.coronas.bAdditiveObjs, i, 0);
		RP (gameOptions [i].render.effects.bRobotShields, i, 0);
		RP (gameOptions [i].render.effects.bOnlyShieldHits, 0, 0);
		RP (gameOptions [i].render.effects.bTransparent, i, 0);
		RP (gameOptions [i].render.lightning.bAuxViews, i, 0);
		RP (gameOptions [i].render.lightning.bMonitors, i, 0);
		RP (gameOptions [i].render.lightning.bGlow, i, 0);
		RP (gameOptions [i].render.lightning.bDamage, i, 0);
		RP (gameOptions [i].render.lightning.bExplosions, i, 0);
		RP (gameOptions [i].render.lightning.bOmega, i, 0);
		RP (gameOptions [i].render.lightning.bRobotOmega, i, 0);
		RP (gameOptions [i].render.lightning.bPlayers, i, 0);
		RP (gameOptions [i].render.lightning.bRobots, i, 0);
		RP (gameOptions [i].render.lightning.bStatic, i, 0);
		RP (gameOptions [i].render.lightning.nStyle, i, 0);
		RP (gameOptions [i].render.lightning.nQuality, i, 0);
		RP (gameOptions [i].render.cameras.bFitToWall, i, 0);
		RP (gameOptions [i].render.cameras.nFPS, i, 0);
		RP (gameOptions [i].render.cameras.nSpeed, i, 0);
		RP (gameOptions [i].render.automap.bCoronas, i, 0);
		RP (gameOptions [i].render.automap.bGrayOut, i, 0);
		RP (gameOptions [i].render.automap.bSparks, i, 0);
		RP (gameOptions [i].render.automap.bParticles, i, 0);
		RP (gameOptions [i].render.automap.bLightning, i, 0);
		RP (gameOptions [i].render.automap.bSkybox, i, 0);
		RP (gameOptions [i].render.automap.nColor, i, 0);
		RP (gameOptions [i].render.cockpit.bRotateMslLockInd, i, 0);
		RP (gameOptions [i].render.cockpit.bScaleGauges, i, 0);
		RP (gameOptions [i].render.cockpit.bSplitHUDMsgs, i, 0);
		RP (gameOptions [i].render.cockpit.bWideDisplays, i, !i);
		RP (gameOptions [i].render.color.bCap, i, 0);
		RP (gameOptions [i].render.color.bMix, i, 0);
		RP (gameOptions [i].render.color.nSaturation, i, 0);
		RP (gameOptions [i].render.color.bWalls, i, 0);
		RP (gameOptions [i].render.powerups.nSpin, i, 0);
		RP (gameOptions [i].render.shadows.bFast, i, 0);
		RP (gameOptions [i].render.shadows.bMissiles, i, 0);
		RP (gameOptions [i].render.shadows.bPowerups, i, 0);
		RP (gameOptions [i].render.shadows.bPlayers, i, 0);
		RP (gameOptions [i].render.shadows.bReactors, i, 0);
		RP (gameOptions [i].render.shadows.bRobots, i, 0);

		RP (gameOptions [i].render.ship.nWingtip, i, 0);
		RP (gameOptions [i].render.ship.bBullets, i, 0);
		RP (gameOptions [i].render.ship.nColor, i, 0);

		RP (gameOptions [i].render.weaponIcons.alpha, i, 0);
		RP (gameOptions [i].render.weaponIcons.bEquipment, i, 0);
		RP (gameOptions [i].render.weaponIcons.bShowAmmo, i, 0);
		RP (gameOptions [i].render.weaponIcons.bSmall, i, 0);
		RP (gameOptions [i].render.weaponIcons.bBoldHighlight, i, 0);
		RP (gameOptions [i].render.weaponIcons.nSort, i, 0);
		RP (gameOptions [i].render.weaponIcons.nHiliteColor, i, 0);
		RP (gameOptions [i].app.nVersionFilter, i, 0);
		RP (gameOptions [i].sound.xCustomSoundVolume, i, 0);
		RP (gameOptions [i].gameplay.bIdleAnims, i, 0);
		RP (gameOptions [i].gameplay.bUseD1AI, i, 0);
#endif
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
	RP (extraGameInfo [i].bFriendlyIndicators, i, 0);
	RP (extraGameInfo [i].bMslLockIndicators, i, 0);
	RP (extraGameInfo [i].bMouseLook, i, 0);
	RP (extraGameInfo [i].bPowerupLights, i, 0);
	RP (extraGameInfo [i].bTargetIndicators, i, 0);
	RP (extraGameInfo [i].bTowFlags, i, 0);
	RP (extraGameInfo [i].bTeamDoors, i, 0);
	RP (extraGameInfo [i].nCoopPenalty, i, 0);
	RP (extraGameInfo [i].nHitboxes, i, 0);
	RP (extraGameInfo [i].nDamageModel, i, 0);
	RP (gameOptions [i].input.joystick.bUse, i, 0);
	RP (gameOptions [i].input.mouse.bUse, i, 0);
	RP (gameOptions [i].input.trackIR.bUse, i, 0);
	RP (gameOptions [i].gameplay.nAutoLeveling, i, 0);
	RP (gameOptions [i].movies.bSubTitles, i, 0);
	RP (gameOptions [i].render.nMeshQuality, i, 0);
	RP (gameOptions [i].render.bUseLightmaps, i, 0);
	RP (gameOptions [i].render.nLightmapQuality, i, 0);
	RP (gameOptions [i].render.nLightmapPrecision, i, 0);

	RP (gameOptions [i].render.cockpit.bMissileView, i, 0);
	RP (gameOptions [i].render.cockpit.bHUD, i, 0);
	RP (gameOptions [i].render.cockpit.bReticle, i, 0);

#if 0
	RP (extraGameInfo [i].bFlickerLights, i, 0);
	RP (extraGameInfo [i].bKillMissiles, i, 0);
	RP (extraGameInfo [i].bTripleFusion, i, 0);
	RP (extraGameInfo [i].bEnhancedShakers, i, 0);
	RP (extraGameInfo [i].bTagOnlyHitObjs, i, 0);
	RP (extraGameInfo [i].nSpotSize, i, 0);
	RP (gameOptions [i].gameplay.bFastRespawn, i, 0);
	RP (gameOptions [i].movies.bResize, i, 0);
	RP (gameOptions [i].movies.nQuality, i, 0);
	RP (gameOptions [i].menus.bShowLevelVersion, i, 0);
	RP (gameOptions [i].menus.bSmartFileSearch, i, 0);
	RP (gameOptions [i].multi.bUseMacros, i, 0);
	RP (gameOptions [i].ogl.bSetGammaRamp, i, 0);
	RP (gameOptions [i].render.cockpit.bWideDisplays, i, 0);
#endif
	}
RegisterConfig (kcKeyboard, KcKeyboardSize (), "keyboard.");
RegisterConfig (kcMouse, KcMouseSize (), "mouse.");
RegisterConfig (kcJoystick, KcJoystickSize (), "joystick.");
RegisterConfig (kcSuperJoy, KcSuperJoySize (), "superjoy.");
RegisterConfig (kcHotkeys, KcHotkeySize (), "hotkeys.");
}

//------------------------------------------------------------------------------

int CPlayerProfile::Save (void)
{
if (Busy ())
	return 1;

	char			fn [FILENAME_LEN];
	CParam		*pp;

gameStates.sound.audio.nMaxChannels = audio.MaxChannels ();
gameStates.app.iDownloadTimeout = downloadManager.GetTimeoutIndex ();
sprintf (fn, "%s.plx", LOCALPLAYER.callsign);
if (!m_cf.Open (fn, gameFolders.szProfDir, "wt", 0))
	return 0;
for (pp = paramList; pp; pp = pp->next)
	pp->Save (m_cf);
return !m_cf.Close ();
}

//------------------------------------------------------------------------------

CParam* CPlayerProfile::Find (const char *pszTag)
{
	CParam	*pp;

for (pp = paramList; pp; pp = pp->next)
	if (!stricmp (pszTag, pp->szTag))
		return pp;
return NULL;
}

//------------------------------------------------------------------------------

int CPlayerProfile::Set (const char *pszIdent, const char *pszValue)
{
	CParam*	pp;

#if DBG
if (!strcmp (pszIdent, "gameStates.video.nDefaultDisplayMode"))
	pszIdent = pszIdent;
#endif
if (!(pp = Find (pszIdent)))
	return 0;
pp->Set (pszIdent, pszValue);
return 1;
}

//------------------------------------------------------------------------------

int CPlayerProfile::LoadParam (void)
{
	char		szParam	[200], *pszValue;

m_cf.GetS (szParam, sizeof (szParam));
szParam [sizeof (szParam) - 1] = '\0';
if ((pszValue = strchr (szParam, '\n')))
	*pszValue = '\0';
if (!(pszValue = strchr (szParam, '=')))
	return 0;
*pszValue++ = '\0';
return Set (szParam, pszValue);
}

//------------------------------------------------------------------------------

int CPlayerProfile::Load (bool bOnlyWindowSizes)
{
if (Busy ())
	return 1;

	char	fn [FILENAME_LEN];
	int	nParams = 0;

sprintf (fn, "%s.plx", LOCALPLAYER.callsign);
if (!m_cf.Open (fn, gameFolders.szProfDir, "rb", 0))
	return 0;
while (!m_cf.EoF ()) {
	LoadParam ();
	if (bOnlyWindowSizes && (++nParams == 2))
		return !m_cf.Close ();
	}
// call this before closing the file to prevent the profile being overwritten
audio.SetMaxChannels (NMCLAMP (gameStates.sound.audio.nMaxChannels, MIN_SOUND_CHANNELS, MAX_SOUND_CHANNELS));
cockpit->Activate (gameStates.render.cockpit.nType);	
downloadManager.SetTimeoutIndex (gameStates.app.iDownloadTimeout);
for (int i = 0; i < 2; i++) {
	if (gameStates.render.cockpit.n3DView [i] < CV_NONE)
		gameStates.render.cockpit.n3DView [i] = CV_NONE;
	else if (gameStates.render.cockpit.n3DView [i] >= CV_FUNC_COUNT)
		gameStates.render.cockpit.n3DView [i] = CV_FUNC_COUNT - 1;
	}
return !m_cf.Close ();
}

//------------------------------------------------------------------------------

typedef struct tParamValue {
	const char	*pszIdent;
	const char	*pszValue;
	} tParamValue;

tParamValue defaultParams [] = {
	 {"gameData.render.window.w", "640"},
	 {"gameData.render.window.h", "480"},
	 {"gameStates.app.iDownloadTimeout", "5"},
	 {"gameStates.render.cockpit.nType", "3"},
	 {"gameStates.render.bShowFrameRate", "0"},
	 {"gameStates.render.bShowTime", "1"},
	 {"gameStates.sound.audio.nMaxChannels", "128"},
	 {"gameStates.video.nDefaultDisplayMode", "3"},
	 {"gameOptions[0].render.cockpit.bGuidedInMainView", "1"},
	 {"networkData.nNetLifeKills", "0"},
	 {"networkData.nNetLifeKilled", "0"},
	 {"gameData.app.nLifetimeChecksum", "0"},
	 {"gameData.escort.szName", "GUIDE-BOT"},
	 {"gameData.multigame.msg.szMacro[0]", "Why can't we all just get along?"},
	 {"gameData.multigame.msg.szMacro[1]", "Hey, I got a present for ya"},
	 {"gameData.multigame.msg.szMacro[2]", "I got a hankerin' for a spankerin'"},
	 {"gameData.multigame.msg.szMacro[3]", "This one's headed for Uranus"},
	 {"customDisplayMode.w", "0"},
	 {"customDisplayMode.h", "0"},
	 {"gameStates.app.nDifficultyLevel", "2"},
	 {"ogl.m_states.nContrast", "8"},
	 {"gameStates.multi.nConnection", "1"},
	 {"tracker.m_bUse", "0"},
	 {"gameData.menu.Alpha ()", "79"},
	 {"mpParams.nLevel", "1"},
	 {"mpParams.nGameType", "3"},
	 {"mpParams.nGameMode", "3"},
	 {"mpParams.nGameAccess", "0"},
	 {"mpParams.bShowPlayersOnAutomap", "0"},
	 {"mpParams.nDifficulty", "2"},
	 {"mpParams.nWeaponFilter", "67108863"},
	 {"mpParams.nReactorLife", "2"},
	 {"mpParams.nMaxTime", "0"},
	 {"mpParams.nScoreGoal", "2"},
	 {"mpParams.bInvul", "-1"},
	 {"mpParams.bMarkerView", "0"},
	 {"mpParams.bIndestructibleLights", "0"},
	 {"mpParams.bBrightPlayers", "0"},
	 {"mpParams.bShowAllNames", "0"},
	 {"mpParams.bShortPackets", "0"},
	 {"mpParams.nPPS", "10"},
	 {"mpParams.udpPorts[0]", "28342"},
	 {"mpParams.udpPorts[1]", "28342"},
	 {"mpParams.szServerIpAddr", "127.0.0.1"},
	 {"extraGameInfo[0].bAutoBalanceTeams", "0"},
	 {"extraGameInfo[0].bAutoDownload", "0"},
	 {"extraGameInfo[0].bDamageExplosions", "0"},
	 {"extraGameInfo[0].bDisableReactor", "0"},
	 {"extraGameInfo[0].bDropAllMissiles", "1"},
	 {"extraGameInfo[0].bEnhancedCTF", "0"},
	 {"extraGameInfo[0].bFriendlyFire", "1"},
	 {"extraGameInfo[0].bGatlingTrails", "1"},
	 {"extraGameInfo[0].bLightTrails", "1"},
	 {"extraGameInfo[0].bPowerupsOnRadar", "1"},
	 {"extraGameInfo[0].bRobotsOnRadar", "1"},
	 {"extraGameInfo[0].bRotateLevels", "0"},
	 {"extraGameInfo[0].bSafeUDP", "0"},
	 {"extraGameInfo[0].bShadows", "1"},
	 {"extraGameInfo[0].bSmartWeaponSwitch", "1"},
	 {"extraGameInfo[0].bSmokeGrenades", "0"},
	 {"extraGameInfo[0].bThrusterFlames", "1"},
	 {"extraGameInfo[0].bTracers", "1"},
	 {"extraGameInfo[0].bUseCameras", "1"},
	 {"extraGameInfo[0].bUseParticles", "1"},
	 {"extraGameInfo[0].bUseLightning", "1"},
	 {"extraGameInfo[0].bUseHitAngles", "0"},
	 {"extraGameInfo[0].bGatlingSpeedUp", "0"},
	 {"extraGameInfo[0].nLightRange", "2"},
	 {"extraGameInfo[0].nMaxSmokeGrenades", "1"},
	 {"extraGameInfo[0].nRadar", "1"},
	 {"extraGameInfo[0].nDrag", "10"},
	 {"extraGameInfo[0].nWeaponDropMode", "1"},
	 {"extraGameInfo[0].nWeaponIcons", "3"},
	 {"extraGameInfo[0].nZoomMode", "1"},
	 {"extraGameInfo[0].entropy.nCaptureVirusThreshold", "1"},
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
	 {"extraGameInfo[0].entropy.bDoCaptureWarning", "0"},
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
	 {"extraGameInfo[0].monsterball.forces[1].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[2].nWeaponId", "2"},
	 {"extraGameInfo[0].monsterball.forces[2].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[3].nWeaponId", "3"},
	 {"extraGameInfo[0].monsterball.forces[3].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[4].nWeaponId", "12"},
	 {"extraGameInfo[0].monsterball.forces[4].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[5].nWeaponId", "11"},
	 {"extraGameInfo[0].monsterball.forces[5].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[6].nWeaponId", "13"},
	 {"extraGameInfo[0].monsterball.forces[6].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[7].nWeaponId", "14"},
	 {"extraGameInfo[0].monsterball.forces[7].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[8].nWeaponId", "30"},
	 {"extraGameInfo[0].monsterball.forces[8].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[9].nWeaponId", "31"},
	 {"extraGameInfo[0].monsterball.forces[9].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[10].nWeaponId", "33"},
	 {"extraGameInfo[0].monsterball.forces[10].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[11].nWeaponId", "32"},
	 {"extraGameInfo[0].monsterball.forces[11].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[12].nWeaponId", "34"},
	 {"extraGameInfo[0].monsterball.forces[12].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[13].nWeaponId", "35"},
	 {"extraGameInfo[0].monsterball.forces[13].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[14].nWeaponId", "9"},
	 {"extraGameInfo[0].monsterball.forces[14].nForce", "5"},
	 {"extraGameInfo[0].monsterball.forces[15].nWeaponId", "8"},
	 {"extraGameInfo[0].monsterball.forces[15].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[16].nWeaponId", "15"},
	 {"extraGameInfo[0].monsterball.forces[16].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[17].nWeaponId", "17"},
	 {"extraGameInfo[0].monsterball.forces[17].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[18].nWeaponId", "18"},
	 {"extraGameInfo[0].monsterball.forces[18].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[19].nWeaponId", "36"},
	 {"extraGameInfo[0].monsterball.forces[19].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[20].nWeaponId", "37"},
	 {"extraGameInfo[0].monsterball.forces[20].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[21].nWeaponId", "39"},
	 {"extraGameInfo[0].monsterball.forces[21].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[22].nWeaponId", "40"},
	 {"extraGameInfo[0].monsterball.forces[22].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[23].nWeaponId", "54"},
	 {"extraGameInfo[0].monsterball.forces[23].nForce", "10"},
	 {"extraGameInfo[0].monsterball.forces[24].nWeaponId", "-1"},
	 {"extraGameInfo[0].monsterball.forces[24].nForce", "4"},
	 {"extraGameInfo[0].headlight.bDrainPower", "1"},
	 {"extraGameInfo[0].headlight.bBuiltIn", "0"},
	 {"extraGameInfo[0].loadout.nGuns", "0"},
	 {"extraGameInfo[0].loadout.nDevice", "0"},
	 {"extraGameInfo[0].loadout.nMissiles[0]", "-1"},
	 {"extraGameInfo[0].loadout.nMissiles[1]", "-1"},
	 {"extraGameInfo[0].loadout.nMissiles[2]", "-1"},
	 {"extraGameInfo[0].loadout.nMissiles[3]", "-1"},
	 {"extraGameInfo[0].loadout.nMissiles[4]", "-1"},
	 {"extraGameInfo[0].loadout.nMissiles[5]", "-1"},
	 {"extraGameInfo[0].loadout.nMissiles[6]", "-1"},
	 {"extraGameInfo[0].loadout.nMissiles[7]", "-1"},
	 {"extraGameInfo[0].loadout.nMissiles[8]", "-1"},
	 {"extraGameInfo[0].loadout.nMissiles[9]", "-1"},
	 {"extraGameInfo [0].bShowWeapons", "1"},
	 {"extraGameInfo [0].bBrightObjects", "0"},
	 {"extraGameInfo[0].nFusionRamp", "2"},
	 {"gameOptions[0].input.keyboard.bRamp[0]", "0"},
	 {"gameOptions[0].input.mouse.sensitivity[0]", "8"},
	 {"gameOptions[0].input.trackIR.sensitivity[0]", "8"},
	 {"gameOptions[0].input.keyboard.bRamp[1]", "0"},
	 {"gameOptions[0].input.mouse.sensitivity[1]", "8"},
	 {"gameOptions[0].input.trackIR.sensitivity[1]", "8"},
	 {"gameOptions[0].input.keyboard.bRamp[2]", "0"},
	 {"gameOptions[0].input.mouse.sensitivity[2]", "8"},
	 {"gameOptions[0].input.trackIR.sensitivity[2]", "4"},
	 {"gameOptions[0].render.particles.nQuality", "2"},
	 {"gameOptions[0].render.particles.bStatic", "1"},
	 {"gameOptions[0].input.joystick.deadzones[0]", "1"},
	 {"gameOptions[0].input.joystick.sensitivity[0]", "7"},
	 {"gameOptions[0].input.trackIR.bMove [0]", "1"},
	 {"gameOptions[0].input.joystick.deadzones[1]", "1"},
	 {"gameOptions[0].input.joystick.sensitivity[1]", "7"},
	 {"gameOptions[0].input.trackIR.bMove [1]", "1"},
	 {"gameOptions[0].input.joystick.deadzones[2]", "1"},
	 {"gameOptions[0].input.joystick.sensitivity[2]", "7"},
	 {"gameOptions[0].input.trackIR.bMove [2]", "1"},
	 {"gameOptions[0].input.joystick.deadzones[3]", "1"},
	 {"gameOptions[0].input.joystick.sensitivity[3]", "7"},
	 {"gameOptions[0].input.trackIR.bMove [3]", "0"},
	 {"gameOptions[0].input.joystick.deadzones[4]", "1"},
	 {"gameOptions[0].input.joystick.sensitivity[4]", "7"},
	 {"gameOptions[0].input.trackIR.bMove [4]", "1"},
	 {"gameOptions[0].input.mouse.bJoystick", "0"},
	 {"gameOptions[0].input.mouse.bSyncAxes", "1"},
	 {"gameOptions[0].input.mouse.nDeadzone", "2"},
	 {"gameOptions[0].input.joystick.bLinearSens", "0"},
	 {"gameOptions[0].input.joystick.bSyncAxes", "1"},
	 {"gameOptions[0].input.trackIR.nMode", "0"},
	 {"gameOptions[0].input.trackIR.nDeadzone", "4"},
	 {"gameOptions[0].input.keyboard.nType", "0"},
	 {"gameOptions[0].input.keyboard.nRamp", "50"},
	 {"gameOptions[0].ogl.nMaxLightsPerPass", "8"},
	 {"gameOptions[0].render.nLightingMethod", "0"},
	 {"gameOptions[0].render.nQuality", "2"},
	 {"gameOptions[0].render.stereo.nGlasses", "0"},
	 {"gameOptions[0].render.stereo.nMethod", "0"},
	 {"gameOptions[0].render.stereo.nScreenDist", "3"},
	 {"gameOptions[0].render.stereo.xSeparation", "32768"},
	 {"gameOptions[0].render.stereo.bEnhance", "0"},
	 {"gameOptions[0].render.stereo.bColorGain", "0"},
	 {"gameOptions[0].render.stereo.bDeghost", "0"},
	 {"gameOptions[0].render.stereo.bBrighten", "1"},
	 {"gameOptions[0].render.effects.bEnabled", "1"},
	 {"gameOptions[0].render.effects.bGlow", "0"},
	 {"gameOptions[0].render.effects.bSoftParticles", "0"},
	 {"gameOptions[0].render.effects.bEnergySparks", "1"},
	 {"gameOptions[0].render.effects.nShockwaves", "1"},
	 {"gameOptions[0].render.automap.bTextured", "1"},
	 {"gameOptions[0].render.automap.bBright", "0"},
	 {"gameOptions[0].render.cockpit.bMouseIndicator", "1"},
	 {"gameOptions[0].render.cockpit.bObjectTally", "1"},
	 {"gameOptions[0].render.cockpit.bPlayerStats", "0"},
	 {"gameOptions[0].render.cockpit.bTextGauges", "0"},
	 {"gameOptions[0].render.cockpit.nWindowPos", "1"},
	 {"gameOptions[0].render.cockpit.nWindowSize", "0"},
	 {"gameOptions[0].render.cockpit.nWindowZoom", "1"},
	 {"gameOptions[0].render.cockpit.nRadarPos", "0"},
	 {"gameOptions[0].render.cockpit.nRadarSize", "1"},
	 {"gameOptions[0].render.cockpit.nRadarRange", "1"},
	 {"gameOptions[0].render.cockpit.nRadarColor", "0"},
	 {"gameOptions[0].render.cockpit.nRadarStyle", "0"},
	 {"gameOptions[0].render.color.bUseLightmaps", "0"},
	 {"gameOptions[0].render.color.nLightmapRange", "0"},
	 {"gameOptions[0].render.powerups.b3D", "1"},
	 {"gameOptions[0].render.powerups.b3DShields", "1"},
	 {"gameOptions[0].render.shadows.nClip", "1"},
	 {"gameOptions[0].render.shadows.nReach", "2"},
	 {"gameOptions[0].render.nMaxFPS", "60"},
	 {"gameOptions[0].render.nImageQuality", "3"},
	 {"gameOptions[0].render.cockpit.bFlashGauges", "1"},
	 {"gameOptions[0].sound.bFadeMusic", "1"},
	 {"gameOptions[0].sound.bLinkVolumes", "1"},
	 {"gameOptions[0].sound.bGatling", "0"},
	 {"gameOptions[0].sound.bMissiles", "0"},
	 {"gameOptions[0].sound.bShip", "0"},
	 {"gameOptions[0].sound.xCustomVolume", "5"},
	 {"gameOptions[0].app.nVersionFilter", "3"},
	 {"gameOptions[0].demo.bOldFormat", "0"},
	 {"gameOptions[0].gameplay.bInventory", "1"},
	 {"gameOptions[0].gameplay.bNoThief", "0"},
	 {"gameOptions[0].gameplay.nShip[0]", "0"},
	 {"gameOptions[0].gameplay.nAIAwareness", "0"},
	 {"gameOptions[0].gameplay.nAIAggressivity", "0"},
	 {"gameOptions[0].gameplay.nAutoSelectWeapon", "1"},
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
	 {"extraGameInfo[0].bDarkness", "0"},
	 {"extraGameInfo[0].bEnableCheats", "0"},
	 {"extraGameInfo[0].bFastPitch", "2"},
	 {"extraGameInfo[0].bFriendlyIndicators", "0"},
	 {"extraGameInfo[0].bHeadlights", "0"},
	 {"extraGameInfo[0].bMouseLook", "0"},
	 {"extraGameInfo[0].bPowerupLights", "1"},
	 {"extraGameInfo[0].bTowFlags", "0"},
	 {"extraGameInfo[0].bTeamDoors", "0"},
	 {"extraGameInfo[0].nCoopPenalty", "0"},
	 {"extraGameInfo[0].nHitboxes", "2"},
	 {"extraGameInfo[0].nDamageModel", "0"},
	 {"extraGameInfo[0].nSpotSize", "0"},
	 {"extraGameInfo[0].bFixedRespawns", "0"},
	 {"extraGameInfo[0].nSpawnDelay", "0"},
	 {"gameOptions[0].input.joystick.bUse", "1"},
	 {"gameOptions[0].input.mouse.bUse", "1"},
	 {"gameOptions[0].input.trackIR.bUse", "1"},
	 {"gameOptions[0].gameplay.nAutoLeveling", "0"},
	 {"gameOptions[0].movies.bSubTitles", "0"},
	 {"gameOptions[0].render.nMeshQuality", "0"},
	 {"gameOptions[0].render.bUseLightmaps", "0"},
	 {"gameOptions[0].render.nLightmapQuality", "1"},
	 {"gameOptions[0].render.nLightmapPrecision", "1"},
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
	 {"keyboard.Fire flare [0].value", "74"},
	 {"keyboard.Fire flare [1].value", "-1"},
	 {"keyboard.Accelerate [0].value", "72"},
	 {"keyboard.Accelerate [1].value", "17"},
	 {"keyboard.reverse [0].value", "76"},
	 {"keyboard.reverse [1].value", "31"},
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
	 {"mouse.Throttle [0].value", "-1"},
	 {"mouse.Throttle [1].value", "0"},
	 {"mouse.Rear View.value", "-1"},
	 {"mouse.Drop Bomb.value", "-1"},
	 {"mouse.Afterburner.value", "2"},
	 {"mouse.Cycle Primary.value", "3"},
	 {"mouse.Cycle Second.value", "4"},
	 {"mouse.Zoom in.value", "-1"},
	 {"joystick.Fire primary[0].value", "0"},
	 {"joystick.Fire secondary[0].value", "1"},
	 {"joystick.Accelerate [0].value", "-1"},
	 {"joystick.reverse [0].value", "-1"},
	 {"joystick.Fire flare [0].value", "-1"},
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
	 {"joystick.throttle [0].value", "2"},
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
	 {"joystick.Accelerate [1].value", "-1"},
	 {"joystick.reverse [1].value", "-1"},
	 {"joystick.Fire flare [1].value", "-1"},
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
	 {"joystick.throttle [1].value", "10"},
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
	 {"joystick.throttle [2].value", "0"},
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
	 {"superjoy.throttle [0].value", "-1"},
	 {"superjoy.throttle [1].value", "-1"},
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
	 {"hotkeys.Weapon 10[1].value", "-1"},
	 {"gameOptions[0].render.particles.nDens [0]", "1"},
	 {"gameOptions[0].render.particles.nSize [0]", "1"},
	 {"gameOptions[0].render.particles.nLife [0]", "1"},
	 {"gameOptions[0].render.particles.nAlpha[0]", "1"},
	 {"gameOptions[0].render.particles.nDens [1]", "1"},
	 {"gameOptions[0].render.particles.nSize [1]", "1"},
	 {"gameOptions[0].render.particles.nLife [1]", "1"},
	 {"gameOptions[0].render.particles.nAlpha[1]", "1"},
	 {"gameOptions[0].render.particles.nDens [2]", "1"},
	 {"gameOptions[0].render.particles.nSize [2]", "1"},
	 {"gameOptions[0].render.particles.nLife [2]", "1"},
	 {"gameOptions[0].render.particles.nAlpha[2]", "1"},
	 {"gameOptions[0].render.particles.nDens [3]", "1"},
	 {"gameOptions[0].render.particles.nSize [3]", "1"},
	 {"gameOptions[0].render.particles.nLife [3]", "1"},
	 {"gameOptions[0].render.particles.nAlpha[3]", "1"},
	 {"gameOptions[0].render.particles.nDens [4]", "1"},
	 {"gameOptions[0].render.particles.nSize [4]", "1"},
	 {"gameOptions[0].render.particles.nLife [4]", "1"},
	 {"gameOptions[0].render.particles.nAlpha[4]", "1"},
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
	 {"gameOptions[0].render.particles.bBubbles", "1"}

#if 0
	 {"gameData.app.bUseMultiThreading[0]", "1"},
	 {"gameData.app.bUseMultiThreading[1]", "1"},
	 {"gameData.app.bUseMultiThreading[2]", "1"},
	 {"gameData.app.bUseMultiThreading[3]", "1"},
	 {"gameData.app.bUseMultiThreading[4]", "1"},
	 {"gameData.app.bUseMultiThreading[5]", "1"},
	 {"gameData.app.bUseMultiThreading[6]", "1"},
	 {"gameData.app.bUseMultiThreading[7]", "1"},
	 {"extraGameInfo[1].bMouseLook", "0"},
	 {"extraGameInfo[1].bPowerupLights", "0"},
	 {"extraGameInfo[1].bKillMissiles", "0"},
	 {"extraGameInfo[1].bTripleFusion", "1"},
	 {"extraGameInfo[0].bFluidPhysics", "1"},
	 {"extraGameInfo[0].bImmortalPowerups", "0"},
	 {"extraGameInfo[0].bMultiBosses", "1"},
	 {"extraGameInfo[0].bPlayerShield", "0"},
	 {"extraGameInfo[0].bRobotsHitRobots", "1"},
	 {"extraGameInfo[0].bTeleporterCams", "0"},
	 {"extraGameInfo[0].bRotateMarkers", "0"},
	 {"extraGameInfo[0].bWiggle", "1"},
	 {"extraGameInfo[0].grWallTransparency", "19"},
	 {"extraGameInfo[0].nOmegaRamp", "4"},
	 {"extraGameInfo[0].nMslTurnSpeed", "1"},
	 {"extraGameInfo[0].nMslStartSpeed", "0"},
	 {"extraGameInfo[0].nSpeedBoost", "10"},
	 {"extraGameInfo[0].headlight.bAvailable", "1"},
	 {"gameOptions[0].render.particles.bWiggleBubbles", "1"},
	 {"gameOptions[0].render.particles.bWobbleBubbles", "1"},
	 {"gameOptions[0].render.particles.bSyncSizes", "1"},
	 {"gameOptions[0].render.particles.bSort", "1"},
	 {"gameOptions[0].input.bUseHotKeys", "1"},
	 {"gameOptions[0].ogl.bLightObjects", "1"},
	 {"gameOptions[0].ogl.bHeadlights", "0"},
	 {"gameOptions[0].ogl.bLightPowerups", "0"},
	 {"gameOptions[0].ogl.bObjLighting", "0"},
	 {"gameOptions[0].ogl.nMaxLightsPerFace", "16"},
	 {"gameOptions[0].ogl.nMaxLightsPerObject", "8"},
	 {"gameOptions[0].render.nDebrisLife", "0"},
	 {"gameOptions[0].render.effects.bAutoTransparency", "1"},
	 {"gameOptions[0].render.effects.bMovingSparks", "0"},
	 {"gameOptions[0].render.effects.nShrapnels", "1"},
	 {"gameOptions[0].render.coronas.bUse", "1"},
	 {"gameOptions[0].render.coronas.nStyle", "1"},
	 {"gameOptions[0].render.coronas.bShots", "1"},
	 {"gameOptions[0].render.coronas.bPowerups", "1"},
	 {"gameOptions[0].render.coronas.bWeapons", "0"},
	 {"gameOptions[0].render.coronas.nIntensity", "1"},
	 {"gameOptions[0].render.coronas.nObjIntensity", "1"},
	 {"gameOptions[0].render.coronas.bAdditive", "1"},
	 {"gameOptions[0].render.effects.bRobotShields", "1"},
	 {"gameOptions[0].render.effects.bOnlyShieldHits", "1"},
	 {"gameOptions[0].render.effects.bTransparent", "1"},
	 {"gameOptions[0].render.cameras.bFitToWall", "0"},
	 {"gameOptions[0].render.cameras.nFPS", "0"},
	 {"gameOptions[0].render.cameras.nSpeed", "5000"},
	 {"gameOptions[0].render.automap.nColor", "1"},
	 {"gameOptions[0].render.automap.bGrayOut", "1"},
	 {"gameOptions[0].render.automap.bCoronas", "0"},
	 {"gameOptions[0].render.automap.bSparks", "1"},
	 {"gameOptions[0].render.automap.bParticles", "0"},
	 {"gameOptions[0].render.automap.bLightning", "0"},
	 {"gameOptions[0].render.automap.bSkybox", "0"},
	 {"gameOptions[0].render.cockpit.bRotateMslLockInd", "1"},
	 {"gameOptions[0].render.cockpit.bScaleGauges", "1"},
	 {"gameOptions[0].render.cockpit.bSplitHUDMsgs", "1"},
	 {"gameOptions[0].render.color.nLevel", "2"},
	 {"gameOptions[0].render.color.bCap", "0"},
	 {"gameOptions[0].render.color.bMix", "1"},
	 {"gameOptions[0].render.color.nColorSaturation", "0"},
	 {"gameOptions[0].render.color.bWalls", "1"},
	 {"gameOptions[0].render.lightning.bAuxViews", "0"},
	 {"gameOptions[0].render.lightning.bMonitors", "0"},
	 {"gameOptions[0].render.lightning.bDamage", "1"},
	 {"gameOptions[0].render.lightning.bExplosions", "1"},
	 {"gameOptions[0].render.lightning.bOmega", "1"},
	 {"gameOptions[0].render.lightning.bRobotOmega", "0"},
	 {"gameOptions[0].render.lightning.bPlayers", "1"},
	 {"gameOptions[0].render.lightning.bRobots", "1"},
	 {"gameOptions[0].render.lightning.bStatic", "1"},
	 {"gameOptions[0].render.lightning.bCoronas", "1"},
	 {"gameOptions[0].render.lightning.nQuality", "1"},
	 {"gameOptions[0].render.lightning.nStyle", "1"},
	 {"gameOptions[0].render.powerups.nSpin", "1"},
	 {"gameOptions[0].render.shadows.bFast", "1"},
	 {"gameOptions[0].render.shadows.bMissiles", "1"},
	 {"gameOptions[0].render.shadows.bPowerups", "1"},
	 {"gameOptions[0].render.shadows.bPlayers", "1"},
	 {"gameOptions[0].render.shadows.bReactors", "0"},
	 {"gameOptions[0].render.shadows.bRobots", "1"},
	 {"gameOptions[0].render.shadows.nLights", "2"},
	 {"gameOptions[0].render.ship.nWingtip", "1"},
	 {"gameOptions[0].render.ship.bBullets", "1"},
	 {"gameOptions[0].render.ship.nColor", "0"},
	 {"gameOptions[0].render.weaponIcons.alpha", "3"},
	 {"gameOptions[0].render.weaponIcons.bEquipment", "1"},
	 {"gameOptions[0].render.weaponIcons.bShowAmmo", "1"},
	 {"gameOptions[0].render.weaponIcons.bSmall", "1"},
	 {"gameOptions[0].render.weaponIcons.bBoldHighlight", "0"},
	 {"gameOptions[0].render.weaponIcons.nSort", "1"},
	 {"gameOptions[0].render.weaponIcons.nHiliteColor", "0"},
	 {"gameOptions[0].gameplay.bIdleAnims", "1"},
	 {"gameOptions[0].gameplay.bShieldWarning", "1"},
	 {"gameOptions[0].gameplay.nSlowMotionSpeedup", "6"},
	 {"gameOptions[0].gameplay.bUseD1AI", "1"},
	 {"extraGameInfo[0].bCloakedIndicators", "0"},
	 {"extraGameInfo[0].bDamageIndicators", "1"},
	 {"extraGameInfo[0].bDualMissileLaunch", "0"},
	 {"extraGameInfo[0].bFlickerLights", "1"},
	 {"extraGameInfo[0].bMslLockIndicators", "1"},
	 {"extraGameInfo[0].bKillMissiles", "0"},
	 {"extraGameInfo[0].bTripleFusion", "1"},
	 {"extraGameInfo[0].bEnhancedShakers", "0"},
	 {"extraGameInfo[0].bTagOnlyHitObjs", "1"},
	 {"extraGameInfo[0].bTargetIndicators", "1"},
	 {"gameOptions[0].gameplay.bFastRespawn", "0"},
	 {"gameOptions[0].movies.bResize", "1"},
	 {"gameOptions[0].movies.nQuality", "0"},
	 {"gameOptions[0].menus.bShowLevelVersion", "1"},
	 {"gameOptions[0].menus.bSmartFileSearch", "1"},
	 {"gameOptions[0].multi.bUseMacros", "1"},
	 {"gameOptions[0].ogl.bSetGammaRamp", "0"},
	 {"gameOptions[0].render.cockpit.bWideDisplays", "1"},
	 {"gameOptions[1].movies.nQuality", "0"},
	 {"gameOptions[1].menus.bShowLevelVersion", "0"},
	 {"gameOptions[1].menus.bSmartFileSearch", "1"},
	 {"gameOptions[1].multi.bUseMacros", "0"},
	 {"gameOptions[1].ogl.bSetGammaRamp", "0"},
#endif

	};

//------------------------------------------------------------------------------

void CPlayerProfile::Setup (void)
{
	tParamValue	*pv;
	int			i;

for (i = sizeofa (defaultParams), pv = defaultParams; i; i--, pv++) {
	Set (pv->pszIdent, pv->pszValue);
	}
}

//------------------------------------------------------------------------------

int NewPlayerConfig (void)
{
	int	nitems;
	int	i, j, choice;
	CMenu	m (8);
   int	mct = CONTROL_MAX_TYPES;

mct--;
InitWeaponOrdering ();		//setup default weapon priorities

RetrySelection:

for (i = 0; i < mct; i++ )
	m.AddMenu (const_cast<char*> (CONTROL_TEXT(i)), -1);
nitems = i;
m [0].SetText (const_cast<char*> (TXT_CONTROL_KEYBOARD));
choice = gameConfig.nControlType;				// Assume keyboard
#ifndef APPLE_DEMO
i = m.Menu (NULL, TXT_CHOOSE_INPUT, NULL, &choice);
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
	controlSettings.d2xCustom [i] = controlSettings.d2xDefaults[i];
//end this section addition - VR
KCSetControls (0);
gameConfig.nControlType = choice;
if (gameConfig.nControlType == CONTROL_THRUSTMASTER_FCS) {
	i = MsgBox (TXT_IMPORTANT_NOTE, NULL, 2, "Choose another", TXT_OK, TXT_FCS);
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
gameStates.render.cockpit.n3DView[0] = CV_NONE;
gameStates.render.cockpit.n3DView[1] = CV_NONE;

// Default taunt macros
strcpy(gameData.multigame.msg.szMacro[0], TXT_GET_ALONG);
strcpy(gameData.multigame.msg.szMacro[1], TXT_GOT_PRESENT);
strcpy(gameData.multigame.msg.szMacro[2], TXT_HANKERING);
strcpy(gameData.multigame.msg.szMacro[3], TXT_URANUS);
networkData.nNetLifeKills = 0;
networkData.nNetLifeKilled = 0;
gameData.app.nLifetimeChecksum = GetLifetimeChecksum (networkData.nNetLifeKills, networkData.nNetLifeKilled);
profile.Setup ();
return 1;
}

//------------------------------------------------------------------------------

//this length must match the value in escort.c
#define GUIDEBOT_NAME_LEN 9

ubyte dosControlType,winControlType;

//read in the CPlayerData's saved games.  returns errno (0 == no error)
int LoadPlayerProfile (int nStage)
{
if (profile.Busy ())
	return 1;

	CFile		cf;
	int		funcRes = EZERO;
	int		bRewriteIt = 0;
	uint		i;

	short gameWindowW = gameData.render.window.w;
	short	gameWindowH = gameData.render.window.h;
	ubyte nDisplayMode = gameStates.video.nDefaultDisplayMode;

	char		filename [FILENAME_LEN];
	int		id;

memset (highestLevels, 0, sizeof (highestLevels));
nHighestLevels = 0;
sprintf (filename, "%.8s.plr", LOCALPLAYER.callsign);
if (!cf.Open (filename, gameFolders.szProfDir, "rb", 0)) {
	PrintLog ("   couldn't read player file '%s'\n", filename);
	}
else {
	id = cf.ReadInt ();
	if (nCFileError || ((id != SAVE_FILE_ID) && (id != SWAPINT (SAVE_FILE_ID)))) {
		PrintLog ("Player profile '%s' is invalid\r\n", filename);
		}
	else {
		gameStates.input.nPlrFileVersion = cf.ReadShort ();
		if ((gameStates.input.nPlrFileVersion < COMPATIBLE_PLAYER_FILE_VERSION) ||
			 ((gameStates.input.nPlrFileVersion > D2W95_PLAYER_FILE_VERSION) &&
			  (gameStates.input.nPlrFileVersion < D2XW32_PLAYER_FILE_VERSION))) {
			PrintLog ("Player profile '%s' is invalid\r\n", filename);
			}
		else {
			if (gameStates.input.nPlrFileVersion < 161)
				cf.Seek (12 + (gameStates.input.nPlrFileVersion >= 19), SEEK_CUR);
			nHighestLevels = cf.ReadShort ();
			Assert (nHighestLevels <= MAX_MISSIONS);
			if (cf.Read (highestLevels, sizeof (hli), nHighestLevels) != (size_t) nHighestLevels) {
				PrintLog ("Player profile '%s' is damaged\r\n", filename);
				nHighestLevels = 0;
				}
			}
		}
	cf.Close ();
	}

if (!nHighestLevels)
	memset (highestLevels, 0, sizeof (highestLevels));

if (nStage < 1)
	return funcRes;

if (!profile.Load (nStage < 2))
	funcRes = errno;

if (gameStates.gfx.bOverride) {
	gameStates.video.nDefaultDisplayMode = nDisplayMode;
	gameData.render.window.w = gameWindowW;
	gameData.render.window.h = gameWindowH;
	}
else if (gameStates.video.nDefaultDisplayMode < 0) {
	gameStates.video.nDefaultDisplayMode = CUSTOM_DISPLAY_MODE;
	}
else 
	gameStates.video.nDefaultDisplayMode = FindDisplayMode (gameData.render.window.w, gameData.render.window.h);
SetCustomDisplayMode (customDisplayMode.w, customDisplayMode.h);

if (nStage < 2)
	return funcRes;

DefaultAllSettings ();

if (funcRes != EZERO) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, "%s\n\n%s", TXT_ERROR_READING_PLR, strerror (funcRes));
	return funcRes;
	}

KCSetControls (1);
//post processing of parameters
if (gameStates.input.nPlrFileVersion >= 23) {
	if (gameData.app.nLifetimeChecksum != GetLifetimeChecksum (networkData.nNetLifeKills, networkData.nNetLifeKilled)) {
		networkData.nNetLifeKills =
		networkData.nNetLifeKilled = 0;
		gameData.app.nLifetimeChecksum = 0;
 		MsgBox (NULL, NULL, 1, TXT_PROFILE_DAMAGED, TXT_WARNING);
		bRewriteIt = 1;
		}
	}
for (i = 0; i < sizeof (gameData.escort.szName); i++) {
	if (!gameData.escort.szName [i])
		break;
	if (!isprint (gameData.escort.szName [i])) {
		strcpy (gameData.escort.szName, "GUIDE-BOT");
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

if (bRewriteIt)
	SavePlayerProfile ();

gameStates.render.nLightingMethod = gameStates.app.bNostalgia ? 0 : gameOpts->render.nLightingMethod;
if ((gameOpts->render.nLightingMethod > 1) && !ogl.m_features.bShaders)
	gameOpts->render.nLightingMethod = 1;
if (gameStates.render.nLightingMethod == 2)
	gameStates.render.bPerPixelLighting = 2;
else if ((gameStates.render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps && ogl.m_features.bShaders)
	gameStates.render.bPerPixelLighting = 1;
else
	gameStates.render.bPerPixelLighting = 0;
gameStates.render.nMaxLightsPerPass = gameOpts->ogl.nMaxLightsPerPass;
gameStates.render.nMaxLightsPerFace = gameOpts->ogl.nMaxLightsPerFace;
gameStates.render.nMaxLightsPerObject = gameOpts->ogl.nMaxLightsPerObject;
gameStates.render.bAmbientColor = /*gameStates.render.bPerPixelLighting ||*/ (gameOpts->render.color.nLevel == 2);
gameOpts->sound.xCustomSoundVolume = fix (float (gameConfig.nAudioVolume [0]) * 10.0f / 8.0f + 0.5f);
extraGameInfo [0].bFlickerLights = gameOpts->app.bEpilepticFriendly;
if ((extraGameInfo [0].bFastPitch < 1) || (extraGameInfo [0].bFastPitch > 2))
	extraGameInfo [0].bFastPitch = 2;
extraGameInfo [1].bFastPitch = 2;
for (i = 0; i < UNIQUE_JOY_AXES; i++)
	JoySetDeadzone (gameOpts->input.joystick.deadzones [i], i);
DefaultAllSettings ();
#if _WIN32
if (gameStates.render.bVSyncOk)
	wglSwapIntervalEXT (gameOpts->render.nMaxFPS < 0);
#endif
return funcRes;
}

//------------------------------------------------------------------------------

//finds entry for this level in table.  if not found, returns ptr to
//empty entry.  If no empty entries, takes over last one
int FindHLIEntry (void)
{
	int i;

for (i = 0; i < nHighestLevels; i++) {
	if (!stricmp (highestLevels [i].shortname, missionManager [missionManager.nCurrentMission].filename))
		break;
	}
if (i == nHighestLevels) {		//not found.  create entry
	if (i == MAX_MISSIONS)
		i--;		//take last entry
	else
		nHighestLevels++;
	strcpy (highestLevels [i].shortname, missionManager [missionManager.nCurrentMission].filename);
	highestLevels [i].nLevel = 0;
	}
return i;
}

//------------------------------------------------------------------------------
//set a new highest level for CPlayerData for this mission
void SetHighestLevel (ubyte nLevel)
{
int ret = LoadPlayerProfile (0);
if ((ret == EZERO) || (ret == ENOENT))	{	//if file doesn't exist, that's ok
	int i = FindHLIEntry ();
	if (nLevel > highestLevels [i].nLevel)
		highestLevels [i].nLevel = nLevel;
	SavePlayerProfile ();
	}
}

//------------------------------------------------------------------------------
//gets the CPlayerData's highest level from the file for this mission
int GetHighestLevel(void)
{
	int i;
	int nHighestSaturnLevel = 0;

LoadPlayerProfile (0);
#ifndef SATURN
if (strlen (missionManager [missionManager.nCurrentMission].filename) == 0) {
	for (i = 0; i < nHighestLevels; i++)
		if (!stricmp (highestLevels [i].shortname, "DESTSAT")) 	//	Destination Saturn.
		 	nHighestSaturnLevel = highestLevels [i].nLevel;
}
#endif
i = highestLevels [FindHLIEntry()].nLevel;
if (i > missionManager.nLastLevel) 
	i = highestLevels [FindHLIEntry()].nLevel = missionManager.nLastLevel;
if (nHighestSaturnLevel > i)
   i = nHighestSaturnLevel;
return i;
}

//------------------------------------------------------------------------------

//write out CPlayerData's saved games.  returns errno (0 == no error)
int SavePlayerProfile (void)
{
if (gameStates.app.bReadOnly)
	return EZERO;
if (profile.Busy ())
	return 1;

	CFile	cf;
	char	filename [FILENAME_LEN];		// because of ":gameData.multiplayer.players:" path
	int	funcRes = EZERO;

funcRes = WriteConfigFile ();
sprintf (filename, "%s.plr", LOCALPLAYER.callsign);
cf.Open (filename, gameFolders.szProfDir, "wb", 0);
if (cf.File ()) {
	cf.WriteInt (SAVE_FILE_ID);
	cf.WriteShort (PLAYER_FILE_VERSION);
	Assert (nHighestLevels <= MAX_MISSIONS);
	cf.WriteShort (nHighestLevels);
	cf.Write (highestLevels, sizeof (hli), nHighestLevels);
	cf.Close ();
	}
// write D2X-XL stuff
if (gameStates.video.nDefaultDisplayMode >= CUSTOM_DISPLAY_MODE)
	gameStates.video.nDefaultDisplayMode = -1;
customDisplayMode = displayModeInfo [CUSTOM_DISPLAY_MODE];
if (!profile.Save ()) {
	funcRes = errno;
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, "%s\n\n%s", TXT_ERROR_WRITING_PLR, strerror (funcRes));
	}
if (gameStates.video.nDefaultDisplayMode < 0)
	gameStates.video.nDefaultDisplayMode = CUSTOM_DISPLAY_MODE;
return funcRes;
}

//------------------------------------------------------------------------------
//update the CPlayerData's highest level.  returns errno (0 == no error)
int UpdatePlayerFile (void)
{
	int ret = LoadPlayerProfile (2);

if ((ret != EZERO) && (ret != ENOENT))		//if file doesn't exist, that's ok
	return ret;
return SavePlayerProfile ();
}

//------------------------------------------------------------------------------

int GetLifetimeChecksum (int a, int b)
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
	CMenu	m;
	CFile cf;
	int	x;
	char	filename [FILENAME_LEN];
	char	text [CALLSIGN_LEN + 1] = "";

strncpy (text, LOCALPLAYER.callsign,CALLSIGN_LEN);

for (;;) {
	m.Destroy ();
	m.Create (1);
	m.AddInput (text, 8);

	nmAllowedChars = playername_allowed_chars;
	x = m.Menu (NULL, TXT_ENTER_PILOT_NAME, NULL, NULL);
	nmAllowedChars = NULL;
	if (x < 0) {
		if (bAllowAbort) 
			return 0;
		continue;
		}
	if (text [0] == 0)	//null string
		continue;
	sprintf (filename, "%s.plr", text);
	if (cf.Exist (filename,gameFolders.szProfDir, 0)) {
		MsgBox (NULL, NULL, 1, TXT_OK, "%s '%s' %s", TXT_PLAYER, text, TXT_ALREADY_EXISTS);
		continue;
		}
	if (!NewPlayerConfig ())
		continue;			// They hit Esc during New CPlayerData config
	break;
	}
strncpy (LOCALPLAYER.callsign, text, CALLSIGN_LEN);
DefaultAllSettings ();
SavePlayerProfile ();
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

	CFileSelector	fs;

if (LOCALPLAYER.callsign [0] == 0) {
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
if ((bAutoPlr = (gameData.multiplayer.autoNG.bValid > 0) && *gameData.multiplayer.autoNG.szPlayer))
	strncpy (filename, gameData.multiplayer.autoNG.szPlayer, 8);
else if ((bAutoPlr = bStartup && (i = FindArg ("-player")) && *appConfig [++i]))
	strncpy (filename, appConfig [i], 8);
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
if (!fs.FileSelector (TXT_SELECT_PILOT, filespec, filename, bAllowAbort)) {
	if (bAllowAbort) {
		return 0;
		}
	goto callMenu; //return 0;		// They hit Esc in file selector
	}

got_player:

bStartup = 0;
if (filename [0] == '<') { // They selected 'create new pilot'
	if (!MakeNewPlayerFile (1))
		goto callMenu;	// They hit Esc during enter name stage
	}
else
	strncpy (LOCALPLAYER.callsign, filename, CALLSIGN_LEN);
if (LoadPlayerProfile (2) != EZERO)
	goto callMenu;
KCSetControls (0);
SetDisplayMode (gameStates.video.nDefaultDisplayMode, 1);
WriteConfigFile ();		// Update lastplr
D2SetCaption ();
return 1;
}

//------------------------------------------------------------------------------

