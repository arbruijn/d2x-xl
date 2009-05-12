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
	int	nCollHandling;
} physOpts;

//------------------------------------------------------------------------------

static const char *pszMslTurnSpeeds [3];
static const char *pszMslStartSpeeds [4];
static const char *pszAutoLevel [3];
static const char *pszStdAdv [2];
static const char *pszDrag [4];

static int nDrag;

//------------------------------------------------------------------------------

#if 0

static const char *OmegaRampStr (void)
{
	static char szRamp [20];

if (extraGameInfo [0].nOmegaRamp == 0)
	return "1 sec";
sprintf (szRamp, "%d secs", nOmegaDuration [(int) extraGameInfo [0].nOmegaRamp]);
return szRamp;
}

#endif

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

	m = menu + physOpts.nAutoLevel;
	v = m->m_value;
	if (gameOpts->gameplay.nAutoLeveling != v) {
		gameOpts->gameplay.nAutoLeveling = v;
		sprintf (m->m_text, TXT_AUTOLEVEL, pszAutoLevel [v]);
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nHitDetection;
	v = m->m_value;
	if (extraGameInfo [0].nHitboxes != v) {
		extraGameInfo [0].nHitboxes = v;
		sprintf (m->m_text, TXT_HIT_DETECTION, pszStdAdv [v]);
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nCollHandling;
	v = m->m_value;
	if (extraGameInfo [0].bUseHitAngles != v) {
		extraGameInfo [0].bUseHitAngles = v;
		sprintf (m->m_text, TXT_COLLISION_HANDLING, pszStdAdv [v]);
		m->m_bRebuild = 1;
		}

	m = menu + physOpts.nDrag;
	v = m->m_value;
	if (nDrag != v) {
		nDrag = v;
		sprintf (m->m_text, TXT_PLAYER_DRAG, pszDrag [v]);
		m->m_bRebuild = 1;
		}

if (gameOpts->app.bExpertMode == SUPERUSER) {
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

void DefaultPhysicsSettings (void);

void PhysicsOptionsMenu (void)
{
	static char nDragTable [] = {0, 3, 6, 10};
	static int choice = 0;

	CMenu	m;
	int	i;
	int	optWiggle = -1;
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

pszStdAdv [0] = TXT_STANDARD;
pszStdAdv [1] = TXT_ADVANCED;

pszDrag [0] = TXT_OFF;
pszDrag [1] = TXT_LOW;
pszDrag [2] = TXT_MEDIUM;
pszDrag [3] = TXT_STANDARD;

gameOpts->gameplay.nAutoLeveling = NMCLAMP (gameOpts->gameplay.nAutoLeveling, 0, 2);
extraGameInfo [0].nHitboxes = NMCLAMP (extraGameInfo [0].nHitboxes, 0, 2) >> 1;
for (nDrag = sizeofa (nDragTable); nDrag; ) {
	nDrag--;
	if (extraGameInfo [0].nDrag >= nDragTable [nDrag])
		break;
	}

do {
	m.Destroy ();
	m.Create (30);
	sprintf (szSlider + 1, TXT_FUSION_RAMP, extraGameInfo [0].nFusionRamp * 50, '%');
	*szSlider = *(TXT_FUSION_RAMP - 1);
	physOpts.nFusionRamp = m.AddSlider (szSlider + 1, extraGameInfo [0].nFusionRamp - 2, 0, 6, KEY_F, HTX_FUSION_RAMP);
	sprintf (szSlider + 1, TXT_PLAYER_DRAG, pszDrag [nDrag]);
	*szSlider = *(TXT_PLAYER_DRAG - 1);
	physOpts.nDrag = m.AddSlider (szSlider + 1, nDrag, 0, 3, KEY_P, HTX_PLAYER_DRAG);
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
		m.AddText ("", 0);
		if (extraGameInfo [0].nDrag)
			optWiggle = m.AddCheck (TXT_WIGGLE_SHIP, extraGameInfo [0].bWiggle, KEY_W, HTX_MISC_WIGGLE);
		else
			optWiggle = -1;
		}
	m.AddText ("", 0);

	sprintf (szSlider + 1, TXT_AUTOLEVEL, pszAutoLevel [gameOpts->gameplay.nAutoLeveling]);
	*szSlider = *(TXT_AUTOLEVEL - 1);
	physOpts.nAutoLevel = m.AddSlider (szSlider + 1, gameOpts->gameplay.nAutoLeveling, 0, 2, KEY_S, HTX_AUTO_LEVELING);

	sprintf (szSlider + 1, TXT_HIT_DETECTION, pszStdAdv [extraGameInfo [0].nHitboxes]);
	*szSlider = *(TXT_HIT_DETECTION - 1);
	physOpts.nHitDetection = m.AddSlider (szSlider + 1, extraGameInfo [0].nHitboxes, 0, 1, KEY_H, HTX_GPLAY_HITBOXES);

	sprintf (szSlider + 1, TXT_COLLISION_HANDLING, pszStdAdv [extraGameInfo [0].bUseHitAngles]);
	*szSlider = *(TXT_COLLISION_HANDLING - 1);
	physOpts.nHitDetection = m.AddSlider (szSlider + 1, extraGameInfo [0].bUseHitAngles, 0, 1, KEY_C, HTX_GPLAY_COLLHANDLING);

	do {
		i = m.Menu (NULL, TXT_PHYSICS_MENUTITLE, PhysicsOptionsCallback, &choice);
		} while (i >= 0);
	} while (i == -2);

extraGameInfo [0].nHitboxes <<= 1;
extraGameInfo [0].nDrag = nDragTable [nDrag];
if (gameOpts->app.bExpertMode == SUPERUSER) {
	if (optWiggle >= 0)
		extraGameInfo [0].bWiggle = m [optWiggle].m_value;
	}
DefaultPhysicsSettings ();
if (IsMultiGame)
	NetworkSendExtraGameInfo (NULL);
}

//------------------------------------------------------------------------------
//eof
