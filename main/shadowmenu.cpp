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
#include "inferno.h"
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
#include "newmenu.h"
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
#include "gauges.h"
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
#ifdef EDITOR
#	include "editor/editor.h"
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

static const char *pszReach [4];
static const char *pszClip [4];

int ShadowOptionsCallback (CMenu& menu, int& key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + shadowOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bShadows) {
	extraGameInfo [0].bShadows = v;
	*key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bShadows) {
	m = menus + shadowOpts.nMaxLights;
	v = m->value + 1;
	if (gameOpts->render.shadows.nLights != v) {
		gameOpts->render.shadows.nLights = v;
		sprintf (m->text, TXT_MAX_LIGHTS, gameOpts->render.shadows.nLights);
		m->rebuild = 1;
		}
	m = menus + shadowOpts.nReach;
	v = m->value;
	if (gameOpts->render.shadows.nReach != v) {
		gameOpts->render.shadows.nReach = v;
		sprintf (m->text, TXT_SHADOW_REACH, pszReach [gameOpts->render.shadows.nReach]);
		m->rebuild = 1;
		}
#if DBG_SHADOWS
	if (shadowOpts.nTest >= 0) {
		m = menus + shadowOpts.nTest;
		v = m->value;
		if (bShadowTest != v) {
			bShadowTest = v;
			sprintf (m->text, "Test mode: %d", bShadowTest);
			m->rebuild = 1;
			}
		m = menus + shadowOpts.nZPass;
		v = m->value;
		if (bZPass != v) {
			bZPass = v;
			m->rebuild = 1;
			*key = -2;
			return nCurItem;
			}
		m = menus + shadowOpts.nVolume;
		v = m->value;
		if (bShadowVolume != v) {
			bShadowVolume = v;
			m->rebuild = 1;
			*key = -2;
			return nCurItem;
			}
		}
#endif
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void ShadowOptionsMenu (void)
{
	CMenuItem m [30];
	int	i, j, choice = 0;
	int	nOptions;
	int	optClipShadows, optPlayerShadows, optRobotShadows, optMissileShadows, 
			optPowerupShadows, optReactorShadows;
	char	szMaxLightsPerFace [50], szReach [50];
#if DBG_SHADOWS
	char	szShadowTest [50];
	int	optFrontCap, optRearCap, optFrontFaces, optBackFaces, optSWCulling, optWallShadows,
			optFastShadows;
#endif

pszReach [0] = TXT_PRECISE;
pszReach [1] = TXT_SHORT;
pszReach [2] = TXT_MEDIUM;
pszReach [3] = TXT_LONG;

pszClip [0] = TXT_OFF;
pszClip [1] = TXT_FAST;
pszClip [2] = TXT_MEDIUM;
pszClip [3] = TXT_PRECISE;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	if (extraGameInfo [0].bShadows) {
		m.AddText (nOptions, "", 0);
		nOptions++;
		}
	m.AddCheck (nOptions, TXT_RENDER_SHADOWS, extraGameInfo [0].bShadows, KEY_W, HTX_ADVRND_SHADOWS);
	shadowOpts.nUse = nOptions++;
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
		m.AddSlider (nOptions, szMaxLightsPerFace + 1, gameOpts->render.shadows.nLights - 1, 0, MAX_SHADOW_LIGHTS, KEY_S, HTX_ADVRND_MAXLIGHTS);
		shadowOpts.nMaxLights = nOptions++;
		sprintf (szReach + 1, TXT_SHADOW_REACH, pszReach [gameOpts->render.shadows.nReach]);
		*szReach = *(TXT_SHADOW_REACH - 1);
		m.AddSlider (nOptions, szReach + 1, gameOpts->render.shadows.nReach, 0, 3, KEY_R, HTX_RENDER_SHADOWREACH);
		shadowOpts.nReach = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddText (nOptions, TXT_CLIP_SHADOWS, 0);
		optClipShadows = ++nOptions;
		for (j = 0; j < 4; j++) {
			m.AddRadio (nOptions, pszClip [j], gameOpts->render.shadows.nClip == j, 0, 1, HTX_CLIP_SHADOWS);
			nOptions++;
			}
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddCheck (nOptions, TXT_PLAYER_SHADOWS, gameOpts->render.shadows.bPlayers, KEY_P, HTX_PLAYER_SHADOWS);
		optPlayerShadows = nOptions++;
		m.AddCheck (nOptions, TXT_ROBOT_SHADOWS, gameOpts->render.shadows.bRobots, KEY_O, HTX_ROBOT_SHADOWS);
		optRobotShadows = nOptions++;
		m.AddCheck (nOptions, TXT_MISSILE_SHADOWS, gameOpts->render.shadows.bMissiles, KEY_M, HTX_MISSILE_SHADOWS);
		optMissileShadows = nOptions++;
		m.AddCheck (nOptions, TXT_POWERUP_SHADOWS, gameOpts->render.shadows.bPowerups, KEY_W, HTX_POWERUP_SHADOWS);
		optPowerupShadows = nOptions++;
		m.AddCheck (nOptions, TXT_REACTOR_SHADOWS, gameOpts->render.shadows.bReactors, KEY_A, HTX_REACTOR_SHADOWS);
		optReactorShadows = nOptions++;
#if DBG_SHADOWS
		m.AddCheck (nOptions, TXT_FAST_SHADOWS, gameOpts->render.shadows.bFast, KEY_F, HTX_FAST_SHADOWS);
		optFastShadows = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddCheck (nOptions, "use Z-Pass algorithm", bZPass, 0, NULL);
		shadowOpts.nZPass = nOptions++;
		if (!bZPass) {
			m.AddCheck (nOptions, "render front cap", bFrontCap, 0, NULL);
			optFrontCap = nOptions++;
			m.AddCheck (nOptions, "render rear cap", bRearCap, 0, NULL);
			optRearCap = nOptions++;
			}
		m.AddCheck (nOptions, "render shadow volume", bShadowVolume, 0, NULL);
		shadowOpts.nVolume = nOptions++;
		if (bShadowVolume) {
			m.AddCheck (nOptions, "render front faces", bFrontFaces, 0, NULL);
			optFrontFaces = nOptions++;
			m.AddCheck (nOptions, "render back faces", bBackFaces, 0, NULL);
			optBackFaces = nOptions++;
			}
		m.AddCheck (nOptions, "render CWall shadows", bWallShadows, 0, NULL);
		optWallShadows = nOptions++;
		m.AddCheck (nOptions, "software culling", bSWCulling, 0, NULL);
		optSWCulling = nOptions++;
		sprintf (szShadowTest, "test method: %d", bShadowTest);
		m.AddSlider (nOptions, szShadowTest, bShadowTest, 0, 6, KEY_S, NULL);
		shadowOpts.nTest = nOptions++;
#endif
		}
	for (;;) {
		i = ExecMenu1 (NULL, TXT_SHADOW_MENUTITLE, nOptions, m, &ShadowOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	for (j = 0; j < 4; j++)
		if (m [optClipShadows + j].value) {
			gameOpts->render.shadows.nClip = j;
			break;
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

#endif

//------------------------------------------------------------------------------
//eof
