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

#include "menu.h"
#include "descent.h"
#include "ipx.h"
#include "key.h"
#include "iff.h"
#include "u_mem.h"
#include "error.h"
#include "screens.h"
#include "joy.h"
#include "slew.h"
#include "args.h"
#include "hogfile.h"
#include "newdemo.h"
#include "timer.h"
#include "text.h"
#include "gamefont.h"
#include "menu.h"
#include "network.h"
#include "network_lib.h"
#include "netmenu.h"
#include "scores.h"
#include "joydefs.h"
#include "playsave.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "state.h"
#include "movie.h"
#include "gamepal.h"
#include "cockpit.h"
#include "strutil.h"
#include "reorder.h"
#include "rendermine.h"
#include "light.h"
#include "lightmap.h"
#include "autodl.h"
#include "tracker.h"
#include "omega.h"
#include "lightning.h"
#include "vers_id.h"
#include "input.h"
#include "collide.h"
#include "objrender.h"
#include "sparkeffect.h"
#include "renderthreads.h"
#include "soundthreads.h"
#include "menubackground.h"

//------------------------------------------------------------------------------

static struct {
	int	nSpeedboost;
	int	nFusionRamp;
	int	nOmegaRamp;
	int	nMslTurnSpeed;
	int	nMslStartSpeed;
	int	nSlomoSpeedup;
	int	nDrag;
	int	nAutoLevel;
	int	nHitDetection;
} physOpts;

//------------------------------------------------------------------------------

static int nOptDebrisLife;

static const char *pszMslTurnSpeeds [3];
static const char *pszMslStartSpeeds [4];
static const char *pszAutoLevel [3];
static const char *pszHitDetection [4];

static int nHitDetection;

//------------------------------------------------------------------------------

static const char *OmegaRampStr (void)
{
	static char szRamp [20];

if (extraGameInfo [0].nOmegaRamp == 0)
	return "1 sec";
sprintf (szRamp, "%d secs", nOmegaDuration [(int) extraGameInfo [0].nOmegaRamp]);
return szRamp;
}

//------------------------------------------------------------------------------

int PhysicsOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			v;

m = menu + physOpts.nFusionRamp;
v = m->m_value + 2;
if (extraGameInfo [0].nFusionRamp != v) {
	extraGameInfo [0].nFusionRamp = v;
	sprintf (m->m_text, TXT_FUSION_RAMP, extraGameInfo [0].nFusionRamp * 50, '%');
	m->m_bRebuild = 1;
	}

#if SIMPLE_MENUS

	m = menu + physOpts.nAutoLevel;
	v = m->m_value;
	if (gameOpts->gameplay.nAutoLeveling != v) {
		gameOpts->gameplay.nAutoLeveling = v;
		sprintf (m->m_text, TXT_AUTOLEVEL, pszAutoLevel [v]);
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nHitDetection;
	v = m->m_value;
	if (nHitDetection != v) {
		nHitDetection = v;
		sprintf (m->m_text, TXT_HIT_DETECTION, pszHitDetection [v]);
		m->m_bRebuild = 1;
		}

#endif

if (gameOpts->app.bExpertMode == SUPERUSER) {

#if !SIMPLE_MENUS

	m = menu + physOpts.nOmegaRamp;
	v = m->m_value;
	if (extraGameInfo [0].nOmegaRamp != v) {
		extraGameInfo [0].nOmegaRamp = v;
		SetMaxOmegaCharge ();
		sprintf (m->m_text, TXT_OMEGA_RAMP, OmegaRampStr ());
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nSpeedboost;
	v = m->m_value;
	if (extraGameInfo [0].nSpeedBoost != v) {
		extraGameInfo [0].nSpeedBoost = v;
		sprintf (m->m_text, TXT_SPEEDBOOST, extraGameInfo [0].nSpeedBoost * 10, '%');
		m->m_bRebuild = 1;
		}

	m = menu + nOptDebrisLife;
	v = m->m_value;
	if (gameOpts->render.nDebrisLife != v) {
		gameOpts->render.nDebrisLife = v;
		sprintf (m->m_text, TXT_DEBRIS_LIFE, nDebrisLife [v]);
		m->m_bRebuild = -1;
		}

#endif

	m = menu + physOpts.nDrag;
	v = m->m_value;
	if (extraGameInfo [0].nDrag != v) {
		extraGameInfo [0].nDrag = v;
		sprintf (m->m_text, TXT_PLAYER_DRAG, extraGameInfo [0].nDrag * 10, '%');
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nMslTurnSpeed;
	v = m->m_value;
	if (extraGameInfo [0].nMslTurnSpeed != v) {
		extraGameInfo [0].nMslTurnSpeed = v;
		sprintf (m->m_text, TXT_MSL_TURNSPEED, pszMslTurnSpeeds [v]);
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nMslStartSpeed;
	v = m->m_value;
	if (extraGameInfo [0].nMslStartSpeed != 3 - v) {
		extraGameInfo [0].nMslStartSpeed = 3 - v;
		sprintf (m->m_text, TXT_MSL_STARTSPEED, pszMslStartSpeeds [v]);
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nSlomoSpeedup;
	v = m->m_value + 4;
	if (gameOpts->gameplay.nSlowMotionSpeedup != v) {
		gameOpts->gameplay.nSlowMotionSpeedup = v;
		sprintf (m->m_text, TXT_SLOWMOTION_SPEEDUP, (float) v / 2);
		m->m_bRebuild = 1;
		return nCurItem;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

#if SIMPLE_MENUS

void DefaultPhysicsSettings (void);

void PhysicsOptionsMenu (void)
{
	static int choice = 0;

	CMenu	m;
	int	i;
	int	optRobHits = -1, optWiggle = -1, optAutoLevel = -1,
			optFluidPhysics = -1, optHitAngles = -1, optKillMissiles = -1, optHitboxes = -1;
	char	szSlider [50];

pszMslTurnSpeeds [0] = TXT_SLOW;
pszMslTurnSpeeds [1] = TXT_MEDIUM;
pszMslTurnSpeeds [2] = TXT_STANDARD;

pszMslStartSpeeds [0] = TXT_VERY_SLOW;
pszMslStartSpeeds [1] = TXT_SLOW;
pszMslStartSpeeds [2] = TXT_MEDIUM;
pszMslStartSpeeds [3] = TXT_STANDARD;

pszAutoLevel [0] = TXT_NONE;
pszAutoLevel [1] = TXT_STANDARD;
pszAutoLevel [2] = TXT_ADVANCED;

pszHitDetection [0] = TXT_BASIC;
pszHitDetection [1] = TXT_STANDARD;
pszHitDetection [2] = TXT_ADVANCED;
pszHitDetection [3] = TXT_BEST;

gameOpts->gameplay.nAutoLeveling = NMCLAMP (gameOpts->gameplay.nAutoLeveling, 0, 2);
extraGameInfo [0].nHitboxes = NMCLAMP (extraGameInfo [0].nHitboxes, 0, 2);

nHitDetection = (extraGameInfo [0].nHitboxes ? 2 : 0) + extraGameInfo [0].bUseHitAngles;

do {
	m.Destroy ();
	m.Create (30);
	sprintf (szSlider + 1, TXT_FUSION_RAMP, extraGameInfo [0].nFusionRamp * 50, '%');
	*szSlider = *(TXT_FUSION_RAMP - 1);
	physOpts.nFusionRamp = m.AddSlider (szSlider + 1, extraGameInfo [0].nFusionRamp - 2, 0, 6, KEY_U, HTX_FUSION_RAMP);
	if (gameOpts->app.bExpertMode == SUPERUSER) {
		sprintf (szSlider + 1, TXT_MSL_TURNSPEED, pszMslTurnSpeeds [int (extraGameInfo [0].nMslTurnSpeed)]);
		*szSlider = *(TXT_MSL_TURNSPEED - 1);
		physOpts.nMslTurnSpeed = m.AddSlider (szSlider + 1, extraGameInfo [0].nMslTurnSpeed, 0, 2, KEY_T, HTX_GPLAY_MSL_TURNSPEED);
		sprintf (szSlider + 1, TXT_MSL_STARTSPEED, pszMslStartSpeeds [int (3) - extraGameInfo [0].nMslStartSpeed]);
		*szSlider = *(TXT_MSL_STARTSPEED - 1);
		physOpts.nMslStartSpeed = m.AddSlider (szSlider + 1, 3 - extraGameInfo [0].nMslStartSpeed, 0, 3, KEY_S, HTX_MSL_STARTSPEED);
		sprintf (szSlider + 1, TXT_SLOWMOTION_SPEEDUP, float (gameOpts->gameplay.nSlowMotionSpeedup) / 2);
		*szSlider = *(TXT_SLOWMOTION_SPEEDUP - 1);
		physOpts.nSlomoSpeedup = m.AddSlider (szSlider + 1, gameOpts->gameplay.nSlowMotionSpeedup - 4, 0, 4, KEY_M, HTX_SLOWMOTION_SPEEDUP);
		sprintf (szSlider + 1, TXT_PLAYER_DRAG, extraGameInfo [0].nDrag * 10, '%');
		*szSlider = *(TXT_PLAYER_DRAG - 1);
		physOpts.nDrag = m.AddSlider (szSlider + 1, extraGameInfo [0].nDrag, 0, 10, KEY_G, HTX_PLAYER_DRAG);
		m.AddText ("", 0);
		if (extraGameInfo [0].nDrag)
			optWiggle = m.AddCheck (TXT_WIGGLE_SHIP, extraGameInfo [0].bWiggle, KEY_W, HTX_MISC_WIGGLE);
		else
			optWiggle = -1;
		optHitAngles = m.AddCheck (TXT_USE_HITANGLES, extraGameInfo [0].bUseHitAngles, KEY_H, HTX_GPLAY_HITANGLES);
		}
	m.AddText ("", 0);

	sprintf (szSlider + 1, TXT_AUTOLEVEL, pszAutoLevel [gameOpts->gameplay.nAutoLeveling]);
	*szSlider = *(TXT_AUTOLEVEL - 1);
	physOpts.nAutoLevel = m.AddSlider (szSlider + 1, gameOpts->gameplay.nAutoLeveling, 0, 2, KEY_A, HTX_AUTO_LEVELING);

	sprintf (szSlider + 1, TXT_HIT_DETECTION, pszHitDetection [nHitDetection]);
	*szSlider = *(TXT_HIT_DETECTION - 1);
	physOpts.nHitDetection = m.AddSlider (szSlider + 1, nHitDetection, 0, 3, KEY_H, HTX_GPLAY_HITBOXES);

	do {
		i = m.Menu (NULL, TXT_PHYSICS_MENUTITLE, PhysicsOptionsCallback, &choice);
		} while (i >= 0);
	} while (i == -2);

extraGameInfo [0].bUseHitAngles = nHitDetection & 1;
extraGameInfo [0].nHitboxes = nHitDetection & 2;
if (gameOpts->app.bExpertMode == SUPERUSER) {
	extraGameInfo [0].bUseHitAngles = m [optHitAngles].m_value;
	if (optWiggle >= 0)
		extraGameInfo [0].bWiggle = m [optWiggle].m_value;
	}
DefaultPhysicsSettings ();
if (IsMultiGame)
	NetworkSendExtraGameInfo (NULL);
}

#else

void PhysicsOptionsMenu (void)
{
	CMenu	m;
	int	i, choice = 0;
	int	optRobHits = -1, optWiggle = -1, optAutoLevel = -1,
			optFluidPhysics = -1, optHitAngles = -1, optKillMissiles = -1, optHitboxes = -1;
	char	szSlider [50];

pszMslTurnSpeeds [0] = TXT_SLOW;
pszMslTurnSpeeds [1] = TXT_MEDIUM;
pszMslTurnSpeeds [2] = TXT_STANDARD;

pszMslStartSpeeds [0] = TXT_VERY_SLOW;
pszMslStartSpeeds [1] = TXT_SLOW;
pszMslStartSpeeds [2] = TXT_MEDIUM;
pszMslStartSpeeds [3] = TXT_STANDARD;

do {
	m.Destroy ();
	m.Create (30);
	if (gameOpts->app.bExpertMode) {
		sprintf (szSlider + 1, TXT_SPEEDBOOST, extraGameInfo [0].nSpeedBoost * 10, '%');
		*szSlider = *(TXT_SPEEDBOOST - 1);
		physOpts.nSpeedboost = m.AddSlider (szSlider + 1, extraGameInfo [0].nSpeedBoost, 0, 10, KEY_B, HTX_GPLAY_SPEEDBOOST);
		sprintf (szSlider + 1, TXT_FUSION_RAMP, extraGameInfo [0].nFusionRamp * 50, '%');
		*szSlider = *(TXT_FUSION_RAMP - 1);
		physOpts.nFusionRamp = m.AddSlider (szSlider + 1, extraGameInfo [0].nFusionRamp - 2, 0, 6, KEY_U, HTX_FUSION_RAMP);
		sprintf (szSlider + 1, TXT_OMEGA_RAMP, OmegaRampStr ());
		*szSlider = *(TXT_OMEGA_RAMP - 1);
		physOpts.nOmegaRamp = m.AddSlider (szSlider + 1, extraGameInfo [0].nOmegaRamp, 0, 6, KEY_O, HTX_OMEGA_RAMP);
		sprintf (szSlider + 1, TXT_MSL_TURNSPEED, pszMslTurnSpeeds [int (extraGameInfo [0].nMslTurnSpeed)]);
		*szSlider = *(TXT_MSL_TURNSPEED - 1);
		physOpts.nMslTurnSpeed = m.AddSlider (szSlider + 1, extraGameInfo [0].nMslTurnSpeed, 0, 2, KEY_T, HTX_GPLAY_MSL_TURNSPEED);
		sprintf (szSlider + 1, TXT_MSL_STARTSPEED, pszMslStartSpeeds [int (3) - extraGameInfo [0].nMslStartSpeed]);
		*szSlider = *(TXT_MSL_STARTSPEED - 1);
		physOpts.nMslStartSpeed = m.AddSlider (szSlider + 1, 3 - extraGameInfo [0].nMslStartSpeed, 0, 3, KEY_S, HTX_MSL_STARTSPEED);
		sprintf (szSlider + 1, TXT_SLOWMOTION_SPEEDUP, float (gameOpts->gameplay.nSlowMotionSpeedup) / 2);
		*szSlider = *(TXT_SLOWMOTION_SPEEDUP - 1);
		physOpts.nSlomoSpeedup = m.AddSlider (szSlider + 1, gameOpts->gameplay.nSlowMotionSpeedup - 4, 0, 4, KEY_M, HTX_SLOWMOTION_SPEEDUP);
		sprintf (szSlider + 1, TXT_DEBRIS_LIFE, nDebrisLife [gameOpts->render.nDebrisLife]);
		*szSlider = *(TXT_DEBRIS_LIFE - 1);
		nOptDebrisLife = m.AddSlider (szSlider + 1, gameOpts->render.nDebrisLife, 0, 8, KEY_D, HTX_DEBRIS_LIFE);
		sprintf (szSlider + 1, TXT_PLAYER_DRAG, extraGameInfo [0].nDrag * 10, '%');
		*szSlider = *(TXT_PLAYER_DRAG - 1);
		physOpts.nDrag = m.AddSlider (szSlider + 1, extraGameInfo [0].nDrag, 0, 10, KEY_G, HTX_PLAYER_DRAG);
		m.AddText ("", 0);
		if (!extraGameInfo [0].nDrag)
			optWiggle = -1;
		else
			optWiggle = m.AddCheck (TXT_WIGGLE_SHIP, extraGameInfo [0].bWiggle, KEY_W, HTX_MISC_WIGGLE);
		optRobHits = m.AddCheck (TXT_BOTS_HIT_BOTS, extraGameInfo [0].bRobotsHitRobots, KEY_O, HTX_GPLAY_BOTSHITBOTS);
		optFluidPhysics = m.AddCheck (TXT_FLUID_PHYS, extraGameInfo [0].bFluidPhysics, KEY_Y, HTX_GPLAY_FLUIDPHYS);
		optHitAngles = m.AddCheck (TXT_USE_HITANGLES, extraGameInfo [0].bUseHitAngles, KEY_H, HTX_GPLAY_HITANGLES);
		m.AddText ("", 0);
		optKillMissiles = m.AddRadio (TXT_IMMUNE_MISSILES, 0, KEY_I, HTX_KILL_MISSILES);
		m.AddRadio (TXT_OMEGA_KILLS_MISSILES, 0, KEY_K, HTX_KILL_MISSILES);
		m.AddRadio (TXT_KILL_MISSILES, 0, KEY_A, HTX_KILL_MISSILES);
		m [optKillMissiles + extraGameInfo [0].bKillMissiles].m_value = 1;
		m.AddText ("", 0);
		optAutoLevel = m.AddRadio (TXT_AUTOLEVEL_NONE, 0, KEY_N, HTX_AUTO_LEVELLING);
		m.AddRadio (TXT_AUTOLEVEL_SIDE, 0, KEY_S, HTX_AUTO_LEVELLING);
		m.AddRadio (TXT_AUTOLEVEL_FLOOR, 0, KEY_F, HTX_AUTO_LEVELLING);
		m.AddRadio (TXT_AUTOLEVEL_GLOBAL, 0, KEY_M, HTX_AUTO_LEVELLING);
		m [optAutoLevel + NMCLAMP (gameOpts->gameplay.nAutoLeveling, 0, 3)].m_value = 1;
		m.AddText ("", 0);
		optHitboxes = m.AddRadio (TXT_HIT_SPHERES, 0, KEY_W, HTX_GPLAY_HITBOXES);
		m.AddRadio (TXT_SIMPLE_HITBOXES, 0, KEY_W, HTX_GPLAY_HITBOXES);
		m.AddRadio (TXT_COMPLEX_HITBOXES, 0, KEY_W, HTX_GPLAY_HITBOXES);
		m [optHitboxes + NMCLAMP (extraGameInfo [0].nHitboxes, 0, 2)].m_value = 1;
		}
	do {
		i = m.Menu (NULL, TXT_PHYSICS_MENUTITLE, PhysicsOptionsCallback, &choice);
		} while (i >= 0);
	} while (i == -2);
if (gameOpts->app.bExpertMode) {
	gameOpts->gameplay.nAutoLeveling = m [optAutoLevel].m_value;
	extraGameInfo [0].bRobotsHitRobots = m [optRobHits].m_value;
	extraGameInfo [0].bKillMissiles = m [optKillMissiles].m_value;
	extraGameInfo [0].bFluidPhysics = m [optFluidPhysics].m_value;
	extraGameInfo [0].bUseHitAngles = m [optHitAngles].m_value;
	if (optWiggle >= 0)
		extraGameInfo [0].bWiggle = m [optWiggle].m_value;
	for (i = 0; i < 3; i++)
		if (m [optKillMissiles + i].m_value) {
			extraGameInfo [0].bKillMissiles = i;
			break;
			}
	for (i = 0; i < 3; i++)
		if (m [optHitboxes + i].m_value) {
			extraGameInfo [0].nHitboxes = i;
			break;
			}
	for (i = 0; i < 4; i++)
		if (m [optAutoLevel + i].m_value) {
			gameOpts->gameplay.nAutoLeveling = i;
			break;
			}
	for (i = 0; i < 2; i++)
		if (gameStates.gameplay.slowmo [i].fSpeed > (float) gameOpts->gameplay.nSlowMotionSpeedup / 2)
			gameStates.gameplay.slowmo [i].fSpeed = (float) gameOpts->gameplay.nSlowMotionSpeedup / 2;
		else if ((gameStates.gameplay.slowmo [i].fSpeed > 1) && 
					(gameStates.gameplay.slowmo [i].fSpeed < (float) gameOpts->gameplay.nSlowMotionSpeedup / 2))
			gameStates.gameplay.slowmo [i].fSpeed = (float) gameOpts->gameplay.nSlowMotionSpeedup / 2;
	}
else {
#if EXPMODE_DEFAULTS
	extraGameInfo [0].bRobotsHitRobots = 0;
	extraGameInfo [0].bKillMissiles = 0;
	extraGameInfo [0].bFluidPhysics = 1;
	extraGameInfo [0].nHitboxes = 0;
	extraGameInfo [0].bWiggle = 0;
#endif
	}
if (IsMultiGame)
	NetworkSendExtraGameInfo (NULL);
}

#endif

//------------------------------------------------------------------------------
//eof
