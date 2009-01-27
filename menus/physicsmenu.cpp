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
#include "render.h"
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
} physOpts;

//------------------------------------------------------------------------------

static int nOptDebrisLife;

static const char *pszMslTurnSpeeds [3];
static const char *pszMslStartSpeeds [4];

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

int PhysicsOptionsCallback (CMenu& menu, int& key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

if (gameOpts->app.bExpertMode) {
	m = menu + physOpts.nDrag;
	v = m->m_value;
	if (extraGameInfo [0].nDrag != v) {
		extraGameInfo [0].nDrag = v;
		sprintf (m->m_text, TXT_PLAYER_DRAG, extraGameInfo [0].nDrag * 10, '%');
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nSpeedboost;
	v = m->m_value;
	if (extraGameInfo [0].nSpeedBoost != v) {
		extraGameInfo [0].nSpeedBoost = v;
		sprintf (m->m_text, TXT_SPEEDBOOST, extraGameInfo [0].nSpeedBoost * 10, '%');
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nFusionRamp;
	v = m->m_value + 2;
	if (extraGameInfo [0].nFusionRamp != v) {
		extraGameInfo [0].nFusionRamp = v;
		sprintf (m->m_text, TXT_FUSION_RAMP, extraGameInfo [0].nFusionRamp * 50, '%');
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nOmegaRamp;
	v = m->m_value;
	if (extraGameInfo [0].nOmegaRamp != v) {
		extraGameInfo [0].nOmegaRamp = v;
		SetMaxOmegaCharge ();
		sprintf (m->m_text, TXT_OMEGA_RAMP, OmegaRampStr ());
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

	m = menu + nOptDebrisLife;
	v = m->m_value;
	if (gameOpts->render.nDebrisLife != v) {
		gameOpts->render.nDebrisLife = v;
		sprintf (m->m_text, TXT_DEBRIS_LIFE, nDebrisLife [v]);
		m->m_bRebuild = -1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void PhysicsOptionsMenu (void)
{
	CMenu	m;
	int	i, choice = 0;
	int	optRobHits = -1, optWiggle = -1, optAutoLevel = -1,
			optFluidPhysics = -1, optHitAngles = -1, optKillMissiles = -1, optHitboxes = -1;
	char	szSpeedBoost [50], szMslTurnSpeed [50], szMslStartSpeed [50], szSlowmoSpeedup [50], 
			szFusionRamp [50], szOmegaRamp [50], szDebrisLife [50], szDrag [50];

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
		sprintf (szSpeedBoost + 1, TXT_SPEEDBOOST, extraGameInfo [0].nSpeedBoost * 10, '%');
		*szSpeedBoost = *(TXT_SPEEDBOOST - 1);
		physOpts.nSpeedboost = m.AddSlider (szSpeedBoost + 1, extraGameInfo [0].nSpeedBoost, 0, 10, KEY_B, HTX_GPLAY_SPEEDBOOST);
		sprintf (szFusionRamp + 1, TXT_FUSION_RAMP, extraGameInfo [0].nFusionRamp * 50, '%');
		*szFusionRamp = *(TXT_FUSION_RAMP - 1);
		physOpts.nFusionRamp = m.AddSlider (szFusionRamp + 1, extraGameInfo [0].nFusionRamp - 2, 0, 6, KEY_U, HTX_FUSION_RAMP);
		sprintf (szOmegaRamp + 1, TXT_OMEGA_RAMP, OmegaRampStr ());
		*szOmegaRamp = *(TXT_OMEGA_RAMP - 1);
		physOpts.nOmegaRamp = m.AddSlider (szOmegaRamp + 1, extraGameInfo [0].nOmegaRamp, 0, 6, KEY_O, HTX_OMEGA_RAMP);
		sprintf (szMslTurnSpeed + 1, TXT_MSL_TURNSPEED, pszMslTurnSpeeds [(int) extraGameInfo [0].nMslTurnSpeed]);
		*szMslTurnSpeed = *(TXT_MSL_TURNSPEED - 1);
		physOpts.nMslTurnSpeed = m.AddSlider (szMslTurnSpeed + 1, extraGameInfo [0].nMslTurnSpeed, 0, 2, KEY_T, HTX_GPLAY_MSL_TURNSPEED);
		sprintf (szMslStartSpeed + 1, TXT_MSL_STARTSPEED, pszMslStartSpeeds [(int) 3 - extraGameInfo [0].nMslStartSpeed]);
		*szMslStartSpeed = *(TXT_MSL_STARTSPEED - 1);
		physOpts.nMslStartSpeed = m.AddSlider (szMslStartSpeed + 1, 3 - extraGameInfo [0].nMslStartSpeed, 0, 3, KEY_S, HTX_MSL_STARTSPEED);
		sprintf (szSlowmoSpeedup + 1, TXT_SLOWMOTION_SPEEDUP, (float) gameOpts->gameplay.nSlowMotionSpeedup / 2);
		*szSlowmoSpeedup = *(TXT_SLOWMOTION_SPEEDUP - 1);
		physOpts.nSlomoSpeedup = m.AddSlider (szSlowmoSpeedup + 1, gameOpts->gameplay.nSlowMotionSpeedup, 0, 4, KEY_M, HTX_SLOWMOTION_SPEEDUP);
		sprintf (szDebrisLife + 1, TXT_DEBRIS_LIFE, nDebrisLife [gameOpts->render.nDebrisLife]);
		*szDebrisLife = *(TXT_DEBRIS_LIFE - 1);
		nOptDebrisLife = m.AddSlider (szDebrisLife + 1, gameOpts->render.nDebrisLife, 0, 8, KEY_D, HTX_DEBRIS_LIFE);
		sprintf (szDrag + 1, TXT_PLAYER_DRAG, extraGameInfo [0].nDrag * 10, '%');
		*szDrag = *(TXT_PLAYER_DRAG - 1);
		physOpts.nDrag = m.AddSlider (szDrag + 1, extraGameInfo [0].nDrag, 0, 10, KEY_G, HTX_PLAYER_DRAG);
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

//------------------------------------------------------------------------------
//eof
