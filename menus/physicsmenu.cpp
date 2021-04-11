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
#include "playerprofile.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "savegame.h"
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

static const char *pszMslTurnSpeeds [4];
static const char *pszMslStartSpeeds [4];
static const char *pszAutoLevel [4];
static const char *pszStdAdv [2];
static const char *pszDrag [4];

static int32_t nDrag, nFusionRamp;

//------------------------------------------------------------------------------

#if 0

static const char *OmegaRampStr (void)
{
	static char szRamp [20];

if (extraGameInfo [0].nOmegaRamp == 0)
	return "1 sec";
sprintf (szRamp, "%d secs", nOmegaDuration [(int32_t) extraGameInfo [0].nOmegaRamp]);
return szRamp;
}

#endif

//------------------------------------------------------------------------------

int32_t PhysicsOptionsCallback (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int32_t			v;

if ((m = menu ["fusion ramp"])) {
	v = m->Value () + 1;
	if (nFusionRamp != v) {
		nFusionRamp = v;
		extraGameInfo [0].nFusionRamp = 6 - 2 * nFusionRamp;
		sprintf (m->m_text, TXT_FUSION_RAMP, nFusionRamp);
		m->m_bRebuild = 1;
		}
	}

if ((m = menu ["auto leveling"])) {
	v = m->Value ();
	if (gameOpts->gameplay.nAutoLeveling != v) {
		gameOpts->gameplay.nAutoLeveling = v;
		sprintf (m->m_text, TXT_AUTOLEVEL, pszAutoLevel [v]);
		m->m_bRebuild = 1;
		}
	}
#if 0
if ((m = menu ["hit detection"])) {
	v = m->Value ();
	if (extraGameInfo [0].nHitboxes != v) {
		extraGameInfo [0].nHitboxes = v;
		sprintf (m->m_text, TXT_HIT_DETECTION, pszStdAdv [v]);
		m->m_bRebuild = 1;
		}
	}
#endif
if ((m = menu ["unnerf D1 weapons"])) {
	v = m->Value ();
	if (extraGameInfo [0].bUnnerfD1Weapons != v) {
		extraGameInfo [0].bUnnerfD1Weapons = v;
		m->m_bRebuild = 1;
		key = -2;
		}
	}

if ((m = menu ["collision handling"])) {
	v = m->Value ();
	if (extraGameInfo [0].bUseHitAngles != v) {
		extraGameInfo [0].bUseHitAngles = v;
		sprintf (m->m_text, TXT_COLLISION_HANDLING, pszStdAdv [v]);
		m->m_bRebuild = 1;
		}
	}

if ((m = menu ["damage model"])) {
	v = m->Value ();
	if (extraGameInfo [0].nDamageModel != v) {
		extraGameInfo [0].nDamageModel = v;
		sprintf (m->m_text, TXT_DAMAGE_MODEL, pszStdAdv [v]);
		m->m_bRebuild = 1;
		}
	}

if (!(IsMultiGame && gameStates.app.bGameRunning) && (m = menu ["speed"])) {
	v = m->Value ();
	if (extraGameInfo [0].nSpeedScale != v) {
		extraGameInfo [0].nSpeedScale = v;
		v = 100 + v * 25;
		sprintf (m->m_text, TXT_GAME_SPEED, v / 100, v % 100);
		m->m_bRebuild = 1;
		}
	}

if ((m = menu ["drag"])) {
	v = m->Value ();
	if (nDrag != v) {
		nDrag = v;
		sprintf (m->m_text, TXT_PLAYER_DRAG, pszDrag [v]);
		m->m_bRebuild = 1;
		}
	}

if (gameOpts->app.bExpertMode == SUPERUSER) {
	if ((m = menu ["msl turn speed"])) {
		v = m->Value ();
		if (extraGameInfo [0].nWeaponTurnSpeed != v) {
			extraGameInfo [0].nWeaponTurnSpeed = v;
			sprintf (m->m_text, TXT_MSL_TURNSPEED, pszMslTurnSpeeds [v]);
			m->m_bRebuild = 1;
			}
		}

	if ((m = menu ["msl start speed"])) {
		v = m->Value ();
		if (extraGameInfo [0].nMslStartSpeed != 3 - v) {
			extraGameInfo [0].nMslStartSpeed = 3 - v;
			sprintf (m->m_text, TXT_MSL_STARTSPEED, pszMslStartSpeeds [v]);
			m->m_bRebuild = 1;
			}
		}

	if ((m = menu ["slow motion speedup"])) {
		v = m->Value () + 4;
		if (gameOpts->gameplay.nSlowMotionSpeedup != v) {
			gameOpts->gameplay.nSlowMotionSpeedup = v;
			sprintf (m->m_text, TXT_SLOWMOTION_SPEEDUP, (float) v / 2);
			m->m_bRebuild = 1;
			return nCurItem;
			}
		}	
	}
return nCurItem;
}

//------------------------------------------------------------------------------

static void InitStrings (void)
{
	static bool bInitialized = false;

if (bInitialized)
	return;
bInitialized = true;

pszMslTurnSpeeds [0] = TXT_LOW;
pszMslTurnSpeeds [1] = TXT_MEDIUM;
pszMslTurnSpeeds [2] = TXT_HIGH;
pszMslTurnSpeeds [3] = TXT_DODGE_THIS;

pszMslStartSpeeds [0] = TXT_VERY_SLOW;
pszMslStartSpeeds [1] = TXT_SLOW;
pszMslStartSpeeds [2] = TXT_MEDIUM;
pszMslStartSpeeds [3] = TXT_STANDARD;

pszAutoLevel [0] = TXT_NONE;
pszAutoLevel [1] = TXT_STANDARD;
pszAutoLevel [2] = TXT_AUTOLEVEL_FACE;
pszAutoLevel [3] = TXT_AUTOLEVEL_MINE;

pszStdAdv [0] = TXT_STANDARD;
pszStdAdv [1] = TXT_ADVANCED;

pszDrag [0] = TXT_OFF;
pszDrag [1] = TXT_LOW;
pszDrag [2] = TXT_MEDIUM;
pszDrag [3] = TXT_STANDARD;
}

//------------------------------------------------------------------------------

void DefaultPhysicsSettings (bool bSetup = false);

void PhysicsOptionsMenu (void)
{
	static char nDragTable [] = {0, 3, 6, 10};
	static int32_t choice = 0;

	CMenu	m;
	int32_t	i;
	char	szSlider [50];

InitStrings ();

gameOpts->gameplay.nAutoLeveling = NMCLAMP (gameOpts->gameplay.nAutoLeveling, 0, 3);
//extraGameInfo [0].nHitboxes = NMCLAMP (extraGameInfo [0].nHitboxes, 0, 2) >> 1;
for (nDrag = sizeofa (nDragTable); nDrag; ) {
	nDrag--;
	if (extraGameInfo [0].nDrag >= nDragTable [nDrag])
		break;
	}
nFusionRamp = (extraGameInfo [0].nFusionRamp > 2) ? 1 : 2;
do {
	m.Destroy ();
	m.Create (30, "PhysicsOptionsMenu");
	if (!gameStates.app.bGameRunning)
		m.AddCheck ("unnerf D1 weapons", TXT_UNNERF_D1_WEAPONS, extraGameInfo [0].bUnnerfD1Weapons, KEY_U, HTX_UNNERF_D1_WEAPONS);
	if (!extraGameInfo [0].bUnnerfD1Weapons) {
		sprintf (szSlider + 1, TXT_FUSION_RAMP, nFusionRamp);
		*szSlider = *(TXT_FUSION_RAMP - 1);
		m.AddSlider ("fusion ramp", szSlider + 1, nFusionRamp - 1, 0, 1, KEY_F, HTX_FUSION_RAMP);
		}
	if (!(IsMultiGame && gameStates.app.bGameRunning)) {
		int32_t v = 100 + extraGameInfo [0].nSpeedScale * 25;
		sprintf (szSlider + 1, TXT_GAME_SPEED, v / 100, v % 100);
		*szSlider = *(TXT_GAME_SPEED - 1);
		m.AddSlider ("speed", szSlider + 1, extraGameInfo [0].nSpeedScale, 0, 4, KEY_S, HTX_GAME_SPEED);
		}
	sprintf (szSlider + 1, TXT_PLAYER_DRAG, pszDrag [nDrag]);
	*szSlider = *(TXT_PLAYER_DRAG - 1);
	m.AddSlider ("drag", szSlider + 1, nDrag, 0, 3, KEY_P, HTX_PLAYER_DRAG);
	if (gameOpts->app.bExpertMode == SUPERUSER) {
		sprintf (szSlider + 1, TXT_MSL_TURNSPEED, pszMslTurnSpeeds [int32_t (extraGameInfo [0].nWeaponTurnSpeed)]);
		*szSlider = *(TXT_MSL_TURNSPEED - 1);
		m.AddSlider ("msl turn speed", szSlider + 1, extraGameInfo [0].nWeaponTurnSpeed, 0, 3, KEY_T, HTX_GPLAY_MSL_TURNSPEED);
		sprintf (szSlider + 1, TXT_MSL_STARTSPEED, pszMslStartSpeeds [int32_t (3) - extraGameInfo [0].nMslStartSpeed]);
		*szSlider = *(TXT_MSL_STARTSPEED - 1);
		m.AddSlider ("msl start speed", szSlider + 1, 3 - extraGameInfo [0].nMslStartSpeed, 0, 3, KEY_S, HTX_MSL_STARTSPEED);
		sprintf (szSlider + 1, TXT_SLOWMOTION_SPEEDUP, float (gameOpts->gameplay.nSlowMotionSpeedup) / 2);
		*szSlider = *(TXT_SLOWMOTION_SPEEDUP - 1);
		m.AddSlider ("slow motion speedup", szSlider + 1, gameOpts->gameplay.nSlowMotionSpeedup - 4, 0, 4, KEY_M, HTX_SLOWMOTION_SPEEDUP);
		m.AddText ("", "");
		if (extraGameInfo [0].nDrag)
			m.AddCheck ("wiggle", TXT_WIGGLE_SHIP, extraGameInfo [0].bWiggle, KEY_W, HTX_MISC_WIGGLE);
		}
	m.AddText ("", "");

	sprintf (szSlider + 1, TXT_AUTOLEVEL, pszAutoLevel [gameOpts->gameplay.nAutoLeveling]);
	*szSlider = *(TXT_AUTOLEVEL - 1);
	m.AddSlider ("auto leveling", szSlider + 1, gameOpts->gameplay.nAutoLeveling, 0, 3, KEY_S, HTX_AUTO_LEVELING);

#if 0
	sprintf (szSlider + 1, TXT_HIT_DETECTION, pszStdAdv [extraGameInfo [0].nHitboxes]);
	*szSlider = *(TXT_HIT_DETECTION - 1);
	mat.AddSlider ("hit detection", szSlider + 1, extraGameInfo [0].nHitboxes, 0, 1, KEY_H, HTX_GPLAY_HITBOXES);
#endif
	sprintf (szSlider + 1, TXT_COLLISION_HANDLING, pszStdAdv [int32_t (extraGameInfo [0].bUseHitAngles)]);
	*szSlider = *(TXT_COLLISION_HANDLING - 1);
	m.AddSlider ("collision handling", szSlider + 1, extraGameInfo [0].bUseHitAngles, 0, 1, KEY_C, HTX_GPLAY_COLLHANDLING);
	sprintf (szSlider + 1, TXT_DAMAGE_MODEL, pszStdAdv [int32_t (extraGameInfo [0].nDamageModel)]);
	*szSlider = *(TXT_DAMAGE_MODEL - 1);
	m.AddSlider ("damage model", szSlider + 1, extraGameInfo [0].nDamageModel, 0, 1, KEY_D, HTX_DAMAGE_MODEL);
	do {
		i = m.Menu (NULL, TXT_PHYSICS_MENUTITLE, PhysicsOptionsCallback, &choice);
		} while (i >= 0);
	} while (i == -2);

#if 0 //DBG
extraGameInfo [0].nHitboxes = extraGameInfo [0].bUseHitAngles;
#else
extraGameInfo [0].nHitboxes = extraGameInfo [0].bUseHitAngles << 1;
#endif
extraGameInfo [0].nDrag = nDragTable [nDrag];
if (gameOpts->app.bExpertMode == SUPERUSER) {
	if (m.Available ("wiggle"))
		extraGameInfo [0].bWiggle = m.Value ("wiggle");
	}
DefaultPhysicsSettings (false);
if (IsMultiGame)
	NetworkSendExtraGameInfo (NULL);
}

//------------------------------------------------------------------------------
//eof
