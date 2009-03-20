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

#if DBG_SHADOWS
extern int bShadowTest;
extern int bFrontCap;
extern int bRearCap;
extern int bShadowVolume;
extern int bFrontFaces;
extern int bBackFaces;
extern int bWallShadows;
extern int bSWCulling;
#endif
#if SHADOWS
extern int bZPass;
#endif

//------------------------------------------------------------------------------

static struct {
	int	nUse;
	int	nReach;
	int	nMaxLights;
#if DBG
	int	nZPass;
	int	nVolume;
	int	nTest;
#endif
} shadowOpts;

//------------------------------------------------------------------------------

#if SHADOWS

#if !SIMPLE_MENUS

static const char *pszReach [4];
static const char *pszClip [4];

int ShadowOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v;

m = menu + shadowOpts.nUse;
v = m->m_value;
if (v != extraGameInfo [0].bShadows) {
	extraGameInfo [0].bShadows = v;
	key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bShadows) {
	m = menu + shadowOpts.nMaxLights;
	v = m->m_value + 1;
	if (gameOpts->render.shadows.nLights != v) {
		gameOpts->render.shadows.nLights = v;
		sprintf (m->m_text, TXT_MAX_LIGHTS, gameOpts->render.shadows.nLights);
		m->m_bRebuild = 1;
		}
	m = menu + shadowOpts.nReach;
	v = m->m_value;
	if (gameOpts->render.shadows.nReach != v) {
		gameOpts->render.shadows.nReach = v;
		sprintf (m->m_text, TXT_SHADOW_REACH, pszReach [gameOpts->render.shadows.nReach]);
		m->m_bRebuild = 1;
		}
#if DBG_SHADOWS
	if (shadowOpts.nTest >= 0) {
		m = menu + shadowOpts.nTest;
		v = m->m_value;
		if (bShadowTest != v) {
			bShadowTest = v;
			sprintf (m->m_text, "Test mode: %d", bShadowTest);
			m->m_bRebuild = 1;
			}
		m = menu + shadowOpts.nZPass;
		v = m->m_value;
		if (bZPass != v) {
			bZPass = v;
			m->m_bRebuild = 1;
			key = -2;
			return nCurItem;
			}
		m = menu + shadowOpts.nVolume;
		v = m->m_value;
		if (bShadowVolume != v) {
			bShadowVolume = v;
			m->m_bRebuild = 1;
			key = -2;
			return nCurItem;
			}
		}
#endif
	}
return nCurItem;
}

#endif //SIMPLE_MENUS

//------------------------------------------------------------------------------

#if !SIMPLE_MENUS

void ShadowOptionsMenu (void)
{
	CMenu	m;
	int	i, choice = 0;
	int	optClipShadows, optPlayerShadows, optRobotShadows, optMissileShadows, optPowerupShadows, optReactorShadows;
	char	szMaxLightsPerFace [50], szReach [50];

pszReach [0] = TXT_PRECISE;
pszReach [1] = TXT_SHORT;
pszReach [2] = TXT_MEDIUM;
pszReach [3] = TXT_LONG;

pszClip [0] = TXT_OFF;
pszClip [1] = TXT_FAST;
pszClip [2] = TXT_MEDIUM;
pszClip [3] = TXT_PRECISE;

#if DBG_SHADOWS
	char	szShadowTest [50];
	int	optFrontCap, optRearCap, optFrontFaces, optBackFaces, optSWCulling, optWallShadows,	optFastShadows;
#endif

do {
	m.Destroy ();
	m.Create (30);

	if (extraGameInfo [0].bShadows)
		m.AddText ("", 0);
	shadowOpts.nUse = m.AddCheck (TXT_RENDER_SHADOWS, extraGameInfo [0].bShadows, KEY_W, HTX_ADVRND_SHADOWS);
	optClipShadows =
	optPlayerShadows =
	optRobotShadows =
	optMissileShadows =
	optPowerupShadows = 
	optReactorShadows = -1;
#if DBG_SHADOWS
	shadowOpts.nZPass =
	optFrontCap =
	optRearCap =
	shadowOpts.nVolume =
	optFrontFaces =
	optBackFaces =
	optSWCulling =
	optWallShadows =
	optFastShadows =
	shadowOpts.nTest = -1;
#endif
	if (extraGameInfo [0].bShadows) {
		sprintf (szMaxLightsPerFace + 1, TXT_MAX_LIGHTS, gameOpts->render.shadows.nLights);
		*szMaxLightsPerFace = *(TXT_MAX_LIGHTS - 1);
		shadowOpts.nMaxLights = m.AddSlider (szMaxLightsPerFace + 1, gameOpts->render.shadows.nLights - 1, 0, MAX_SHADOW_LIGHTS, KEY_S, HTX_ADVRND_MAXLIGHTS);
		sprintf (szReach + 1, TXT_SHADOW_REACH, pszReach [gameOpts->render.shadows.nReach]);
		*szReach = *(TXT_SHADOW_REACH - 1);
		shadowOpts.nReach = m.AddSlider (szReach + 1, gameOpts->render.shadows.nReach, 0, 3, KEY_R, HTX_RENDER_SHADOWREACH);
		m.AddText ("", 0);
		m.AddText (TXT_CLIP_SHADOWS, 0);
		optClipShadows = m.ToS ();
		for (j = 0; j < 4; j++)
			m.AddRadio (pszClip [j], gameOpts->render.shadows.nClip == j, 0, HTX_CLIP_SHADOWS);
		m.AddText ("", 0);
		optPlayerShadows = m.AddCheck (TXT_PLAYER_SHADOWS, gameOpts->render.shadows.bPlayers, KEY_P, HTX_PLAYER_SHADOWS);
		optRobotShadows = m.AddCheck (TXT_ROBOT_SHADOWS, gameOpts->render.shadows.bRobots, KEY_O, HTX_ROBOT_SHADOWS);
		optMissileShadows = m.AddCheck (TXT_MISSILE_SHADOWS, gameOpts->render.shadows.bMissiles, KEY_M, HTX_MISSILE_SHADOWS);
		optPowerupShadows = m.AddCheck (TXT_POWERUP_SHADOWS, gameOpts->render.shadows.bPowerups, KEY_W, HTX_POWERUP_SHADOWS);
		optReactorShadows = m.AddCheck (TXT_REACTOR_SHADOWS, gameOpts->render.shadows.bReactors, KEY_A, HTX_REACTOR_SHADOWS);
#if DBG_SHADOWS
		optFastShadows = m.AddCheck (TXT_FAST_SHADOWS, gameOpts->render.shadows.bFast, KEY_F, HTX_FAST_SHADOWS);
		m.AddText ("", 0);
		shadowOpts.nZPass = m.AddCheck ("use Z-Pass algorithm", bZPass, 0, NULL);
		if (!bZPass) {
			optFrontCap = m.AddCheck ("render front cap", bFrontCap, 0, NULL);
			optRearCap = m.AddCheck ("render rear cap", bRearCap, 0, NULL);
			}
		shadowOpts.nVolume = m.AddCheck ("render shadow volume", bShadowVolume, 0, NULL);
		if (bShadowVolume) {
			optFrontFaces = m.AddCheck ("render front faces", bFrontFaces, 0, NULL);
			optBackFaces = m.AddCheck ("render back faces", bBackFaces, 0, NULL);
			}
		optWallShadows = m.AddCheck ("render CWall shadows", bWallShadows, 0, NULL);
		optSWCulling = m.AddCheck ("software culling", bSWCulling, 0, NULL);
		sprintf (szShadowTest, "test method: %d", bShadowTest);
		shadowOpts.nTest = m.AddSlider (szShadowTest, bShadowTest, 0, 6, KEY_S, NULL);
#endif
		}
	for (;;) {
		i = m.Menu (NULL, TXT_SHADOW_MENUTITLE, ShadowOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (optClipShadows >= 0) {
		for (int j = 0; j < 4; j++)
			if (m [optClipShadows + j].m_value) {
				gameOpts->render.shadows.nClip = j;
				break;
				}
			}
	GET_VAL (gameOpts->render.shadows.bPlayers, optPlayerShadows);
	GET_VAL (gameOpts->render.shadows.bRobots, optRobotShadows);
	GET_VAL (gameOpts->render.shadows.bMissiles, optMissileShadows);
	GET_VAL (gameOpts->render.shadows.bPowerups, optPowerupShadows);
	GET_VAL (gameOpts->render.shadows.bReactors, optReactorShadows);
#if DBG_SHADOWS
	if (extraGameInfo [0].bShadows) {
		GET_VAL (gameOpts->render.shadows.bFast, optFastShadows);
		GET_VAL (bZPass, shadowOpts.nZPass);
		GET_VAL (bFrontCap, optFrontCap);
		GET_VAL (bRearCap, optRearCap);
		GET_VAL (bFrontFaces, optFrontFaces);
		GET_VAL (bBackFaces, optBackFaces);
		GET_VAL (bWallShadows, optWallShadows);
		GET_VAL (bSWCulling, optSWCulling);
		GET_VAL (bShadowVolume, shadowOpts.nVolume);
		}
#endif
	} while (i == -2);
}

#endif //SIMPLE_MENUS

#endif //SHADOWS

//------------------------------------------------------------------------------
//eof
