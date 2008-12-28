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
	int	nMaxFPS;
	int	nRenderQual;
	int	nTexQual;
	int	nMeshQual;
	int	nWallTransp;
} renderOpts;

static struct {
	int	nMethod;
	int	nHWObjLighting;
	int	nHWHeadlight;
	int	nMaxLightsPerFace;
	int	nMaxLightsPerPass;
	int	nMaxLightsPerObject;
	int	nLightmapQual;
	int	nGunColor;
	int	nObjectLight;
	int	nLightmaps;
} lightOpts;

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

static struct {
	int	nUse;
	int	nSpeed;
	int	nFPS;
} camOpts;

static struct {
	int	nOptTextured;
	int	nOptRadar;
	int	nOptRadarRange;
} automapOpts;

static struct {
	int	nWeapons;
	int	nColor;
} shipRenderOpts;

static int fpsTable [16] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 125, 150, 175, 200, 250};

static int nMaxLightsPerFaceTable [] = {3,4,5,6,7,8,12,16,20,24,32};

static const char *pszTexQual [4];
static const char *pszMeshQual [5];
static const char *pszLMapQual [5];
static const char *pszRendQual [5];

//------------------------------------------------------------------------------

static inline const char *ContrastText (void)
{
return (gameStates.ogl.nContrast == 8) ? TXT_STANDARD : 
		 (gameStates.ogl.nContrast < 8) ? TXT_LOW : 
		 TXT_HIGH;
}

//------------------------------------------------------------------------------

int FindTableFps (int nFps)
{
	int	i, j = 0, d, dMin = 0x7fffffff;

for (i = 0; i < 16; i++) {
	d = abs (nFps - fpsTable [i]);
	if (d < dMin) {
		j = i;
		dMin = d;
		}
	}
return j;
}

//------------------------------------------------------------------------------

static const char *pszRadarRange [3];

int AutomapOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem * m;
	int			v;

m = menus + automapOpts.nOptTextured;
v = m->value;
if (v != gameOpts->render.automap.bTextured) {
	gameOpts->render.automap.bTextured = v;
	*key = -2;
	return nCurItem;
	}
if (!m [automapOpts.nOptRadar + extraGameInfo [0].nRadar].value) {
	*key = -2;
	return nCurItem;
	}
if (automapOpts.nOptRadarRange >= 0) {
	m = menus + automapOpts.nOptRadarRange;
	v = m->value;
	if (v != gameOpts->render.automap.nRange) {
		gameOpts->render.automap.nRange = v;
		sprintf (m->text, TXT_RADAR_RANGE, pszRadarRange [v]);
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void AutomapOptionsMenu (void)
{
	CMenuItem m [30];
	int	i, j, choice = 0;
	int	nOptions;
	int	optBright, optGrayOut, optShowRobots, optShowPowerups, optCoronas, optSmoke, optLightnings, optColor, optSkybox, optSparks;
	char	szRadarRange [50];

pszRadarRange [0] = TXT_SHORT;
pszRadarRange [1] = TXT_MEDIUM;
pszRadarRange [2] = TXT_FAR;
*szRadarRange = '\0';
do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	m.AddCheck (nOptions, TXT_AUTOMAP_TEXTURED, gameOpts->render.automap.bTextured, KEY_T, HTX_AUTOMAP_TEXTURED);
	automapOpts.nOptTextured = nOptions++;
	if (gameOpts->render.automap.bTextured) {
		m.AddCheck (nOptions, TXT_AUTOMAP_BRIGHT, gameOpts->render.automap.bBright, KEY_B, HTX_AUTOMAP_BRIGHT);
		optBright = nOptions++;
		m.AddCheck (nOptions, TXT_AUTOMAP_GRAYOUT, gameOpts->render.automap.bGrayOut, KEY_Y, HTX_AUTOMAP_GRAYOUT);
		optGrayOut = nOptions++;
		m.AddCheck (nOptions, TXT_AUTOMAP_CORONAS, gameOpts->render.automap.bCoronas, KEY_C, HTX_AUTOMAP_CORONAS);
		optCoronas = nOptions++;
		m.AddCheck (nOptions, TXT_RENDER_SPARKS, gameOpts->render.automap.bSparks, KEY_P, HTX_RENDER_SPARKS);
		optSparks = nOptions++;
		m.AddCheck (nOptions, TXT_AUTOMAP_SMOKE, gameOpts->render.automap.bParticles, KEY_S, HTX_AUTOMAP_SMOKE);
		optSmoke = nOptions++;
		m.AddCheck (nOptions, TXT_AUTOMAP_LIGHTNINGS, gameOpts->render.automap.bLightnings, KEY_S, HTX_AUTOMAP_LIGHTNINGS);
		optLightnings = nOptions++;
		m.AddCheck (nOptions, TXT_AUTOMAP_SKYBOX, gameOpts->render.automap.bSkybox, KEY_K, HTX_AUTOMAP_SKYBOX);
		optSkybox = nOptions++;
		}
	else
		optGrayOut =
		optSmoke =
		optLightnings =
		optCoronas =
		optSkybox =
		optBright =
		optSparks = -1;
	m.AddCheck (nOptions, TXT_AUTOMAP_ROBOTS, extraGameInfo [0].bRobotsOnRadar, KEY_R, HTX_AUTOMAP_ROBOTS);
	optShowRobots = nOptions++;
	m.AddRadio (nOptions, TXT_AUTOMAP_NO_POWERUPS, 0, KEY_D, 3, HTX_AUTOMAP_POWERUPS);
	optShowPowerups = nOptions++;
	m.AddRadio (nOptions, TXT_AUTOMAP_POWERUPS, 0, KEY_P, 3, HTX_AUTOMAP_POWERUPS);
	nOptions++;
	if (extraGameInfo [0].nRadar) {
		m.AddRadio (nOptions, TXT_RADAR_POWERUPS, 0, KEY_A, 3, HTX_AUTOMAP_POWERUPS);
		nOptions++;
		}
	m [optShowPowerups + extraGameInfo [0].bPowerupsOnRadar].value = 1;
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddRadio (nOptions, TXT_RADAR_OFF, 0, KEY_R, 1, HTX_AUTOMAP_RADAR);
	automapOpts.nOptRadar = nOptions++;
	m.AddRadio (nOptions, TXT_RADAR_TOP, 0, KEY_T, 1, HTX_AUTOMAP_RADAR);
	nOptions++;
	m.AddRadio (nOptions, TXT_RADAR_BOTTOM, 0, KEY_O, 1, HTX_AUTOMAP_RADAR);
	nOptions++;
	if (extraGameInfo [0].nRadar) {
		m.AddText (nOptions, "", 0);
		nOptions++;
		sprintf (szRadarRange + 1, TXT_RADAR_RANGE, pszRadarRange [gameOpts->render.automap.nRange]);
		*szRadarRange = *(TXT_RADAR_RANGE - 1);
		m.AddSlider (nOptions, szRadarRange + 1, gameOpts->render.automap.nRange, 0, 2, KEY_A, HTX_RADAR_RANGE);
		automapOpts.nOptRadarRange = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddRadio (nOptions, TXT_RADAR_WHITE, 0, KEY_W, 2, NULL);
		optColor = nOptions++;
		m.AddRadio (nOptions, TXT_RADAR_BLACK, 0, KEY_L, 2, NULL);
		nOptions++;
		m [optColor + gameOpts->render.automap.nColor].value = 1;
		m [automapOpts.nOptRadar + extraGameInfo [0].nRadar].value = 1;
		}
	else
		automapOpts.nOptRadarRange =
		optColor = -1;
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_AUTOMAP_MENUTITLE, nOptions, m, AutomapOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	//gameOpts->render.automap.bTextured = m [automapOpts.nOptTextured].value;
	GET_VAL (gameOpts->render.automap.bBright, optBright);
	GET_VAL (gameOpts->render.automap.bGrayOut, optGrayOut);
	GET_VAL (gameOpts->render.automap.bCoronas, optCoronas);
	GET_VAL (gameOpts->render.automap.bSparks, optSparks);
	GET_VAL (gameOpts->render.automap.bParticles, optSmoke);
	GET_VAL (gameOpts->render.automap.bLightnings, optLightnings);
	GET_VAL (gameOpts->render.automap.bSkybox, optSkybox);
	if (automapOpts.nOptRadarRange >= 0)
		gameOpts->render.automap.nRange = m [automapOpts.nOptRadarRange].value;
	extraGameInfo [0].bPowerupsOnRadar = m [optShowPowerups].value;
	extraGameInfo [0].bRobotsOnRadar = m [optShowRobots].value;
	for (j = 0; j < 2 + extraGameInfo [0].nRadar; j++)
		if (m [optShowPowerups + j].value) {
			extraGameInfo [0].bPowerupsOnRadar = j;
			break;
			}
	for (j = 0; j < 3; j++)
		if (m [automapOpts.nOptRadar + j].value) {
			extraGameInfo [0].nRadar = j;
			break;
			}
	if (optColor >= 0) {
		for (j = 0; j < 2; j++)
			if (m [optColor + j].value) {
				gameOpts->render.automap.nColor = j;
				break;
				}
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

static int nOpt3D;

int PowerupOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem * m;
	int			v;

m = menus + nOpt3D;
v = m->value;
if (v != gameOpts->render.powerups.b3D) {
	if ((gameOpts->render.powerups.b3D = v))
		ConvertAllPowerupsToWeapons ();
	*key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void PowerupOptionsMenu (void)
{
	CMenuItem m [10];
	int	i, j, choice = 0;
	int	nOptions;
	int	optSpin;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	m.AddCheck (nOptions, TXT_3D_POWERUPS, gameOpts->render.powerups.b3D, KEY_D, HTX_3D_POWERUPS);
	nOpt3D = nOptions++;
	if (!gameOpts->render.powerups.b3D)
		optSpin = -1;
	else {
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddRadio (nOptions, TXT_SPIN_OFF, 0, KEY_O, 1, NULL);
		optSpin = nOptions++;
		m.AddRadio (nOptions, TXT_SPIN_SLOW, 0, KEY_S, 1, NULL);
		nOptions++;
		m.AddRadio (nOptions, TXT_SPIN_MEDIUM, 0, KEY_M, 1, NULL);
		nOptions++;
		m.AddRadio (nOptions, TXT_SPIN_FAST, 0, KEY_F, 1, NULL);
		nOptions++;
		m [optSpin + gameOpts->render.powerups.nSpin].value = 1;
		}
	for (;;) {
		i = ExecMenu1 (NULL, TXT_POWERUP_MENUTITLE, nOptions, m, PowerupOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (gameOpts->render.powerups.b3D && (optSpin >= 0))
		for (j = 0; j < 4; j++)
			if (m [optSpin + j].value) {
				gameOpts->render.powerups.nSpin = j;
				break;
			}
	} while (i == -2);
}

//------------------------------------------------------------------------------

#if SHADOWS

static const char *pszReach [4];
static const char *pszClip [4];

int ShadowOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
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

int CameraOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + camOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseCameras) {
	extraGameInfo [0].bUseCameras = v;
	*key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bUseCameras) {
	if (camOpts.nFPS >= 0) {
		m = menus + camOpts.nFPS;
		v = m->value * 5;
		if (gameOpts->render.cameras.nFPS != v) {
			gameOpts->render.cameras.nFPS = v;
			sprintf (m->text, TXT_CAM_REFRESH, gameOpts->render.cameras.nFPS);
			m->rebuild = 1;
			}
		}
	if (gameOpts->app.bExpertMode && (camOpts.nSpeed >= 0)) {
		m = menus + camOpts.nSpeed;
		v = (m->value + 1) * 1000;
		if (gameOpts->render.cameras.nSpeed != v) {
			gameOpts->render.cameras.nSpeed = v;
			sprintf (m->text, TXT_CAM_SPEED, v / 1000);
			m->rebuild = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void CameraOptionsMenu (void)
{
	CMenuItem m [10];
	int	i, choice = 0;
	int	nOptions;
	int	bFSCameras = gameOpts->render.cameras.bFitToWall;
	int	optFSCameras, optTeleCams, optHiresCams;
#if 0
	int checks;
#endif

	char szCameraFps [50];
	char szCameraSpeed [50];

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	m.AddCheck (nOptions, TXT_USE_CAMS, extraGameInfo [0].bUseCameras, KEY_C, HTX_ADVRND_USECAMS);
	camOpts.nUse = nOptions++;
	if (extraGameInfo [0].bUseCameras && gameOpts->app.bExpertMode) {
		if (gameStates.app.bGameRunning) 
			optHiresCams = -1;
		else {
			m.AddCheck (nOptions, TXT_HIRES_CAMERAS, gameOpts->render.cameras.bHires, KEY_H, HTX_HIRES_CAMERAS);
			optHiresCams = nOptions++;
			}
		m.AddCheck (nOptions, TXT_TELEPORTER_CAMS, extraGameInfo [0].bTeleporterCams, KEY_U, HTX_TELEPORTER_CAMS);
		optTeleCams = nOptions++;
		m.AddCheck (nOptions, TXT_ADJUST_CAMS, gameOpts->render.cameras.bFitToWall, KEY_A, HTX_ADVRND_ADJUSTCAMS);
		optFSCameras = nOptions++;
		sprintf (szCameraFps + 1, TXT_CAM_REFRESH, gameOpts->render.cameras.nFPS);
		*szCameraFps = *(TXT_CAM_REFRESH - 1);
		m.AddSlider (nOptions, szCameraFps + 1, gameOpts->render.cameras.nFPS / 5, 0, 6, KEY_A, HTX_ADVRND_CAMREFRESH);
		camOpts.nFPS = nOptions++;
		sprintf (szCameraSpeed + 1, TXT_CAM_SPEED, gameOpts->render.cameras.nSpeed / 1000);
		*szCameraSpeed = *(TXT_CAM_SPEED - 1);
		m.AddSlider (nOptions, szCameraSpeed + 1, (gameOpts->render.cameras.nSpeed / 1000) - 1, 0, 9, KEY_D, HTX_ADVRND_CAMSPEED);
		camOpts.nSpeed = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		}
	else {
		optHiresCams = 
		optTeleCams = 
		optFSCameras =
		camOpts.nFPS =
		camOpts.nSpeed = -1;
		}

	do {
		i = ExecMenu1 (NULL, TXT_CAMERA_MENUTITLE, nOptions, m, &CameraOptionsCallback, &choice);
	} while (i >= 0);

	if ((extraGameInfo [0].bUseCameras = m [camOpts.nUse].value)) {
		GET_VAL (extraGameInfo [0].bTeleporterCams, optTeleCams);
		GET_VAL (gameOpts->render.cameras.bFitToWall, optFSCameras);
		if (!gameStates.app.bGameRunning)
			GET_VAL (gameOpts->render.cameras.bHires, optHiresCams);
		}
	if (bFSCameras != gameOpts->render.cameras.bFitToWall) {
		cameraManager.Destroy ();
		cameraManager.Create ();
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

int AdvancedRenderOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

if (optContrast >= 0) {
	m = menus + optContrast;
	v = m->value;
	if (v != gameStates.ogl.nContrast) {
		gameStates.ogl.nContrast = v;
		sprintf (m->text, TXT_CONTRAST, ContrastText ());
		m->rebuild = 1;
		}
	}
if (gameOpts->app.bExpertMode) {
	m = menus + renderOpts.nRenderQual;
	v = m->value;
	if (gameOpts->render.nQuality != v) {
		gameOpts->render.nQuality = v;
		sprintf (m->text, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		m->rebuild = 1;
		}
	if (renderOpts.nTexQual > 0) {
		m = menus + renderOpts.nTexQual;
		v = m->value;
		if (gameOpts->render.textures.nQuality != v) {
			gameOpts->render.textures.nQuality = v;
			sprintf (m->text, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			m->rebuild = 1;
			}
		}
	if (renderOpts.nMeshQual > 0) {
		m = menus + renderOpts.nMeshQual;
		v = m->value;
		if (gameOpts->render.nMeshQuality != v) {
			gameOpts->render.nMeshQuality = v;
			sprintf (m->text, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			m->rebuild = 1;
			}
		}
	m = menus + renderOpts.nWallTransp;
	v = (FADE_LEVELS * m->value + 5) / 10;
	if (extraGameInfo [0].grWallTransparency != v) {
		extraGameInfo [0].grWallTransparency = v;
		sprintf (m->text, TXT_WALL_TRANSP, m->value * 10, '%');
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int LightOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

if (lightOpts.nMethod >= 0) {
	for (v = 0; v < 3; v++)
		if (menus [lightOpts.nMethod + v].value)
			break;
	if (v > 2)
		v = 0;
	if (v != gameOpts->render.nLightingMethod) {
		gameOpts->render.nLightingMethod = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nLightmapQual >= 0) {
	m = menus + lightOpts.nLightmapQual;
	v = m->value;
	if (gameOpts->render.nLightmapQuality != v) {
		gameOpts->render.nLightmapQuality = v;
		sprintf (m->text, TXT_LMAP_QUALITY, pszLMapQual [gameOpts->render.nLightmapQuality]);
		m->rebuild = 1;
		}
	}
if (lightOpts.nLightmaps >= 0) {
	m = menus + lightOpts.nLightmaps;
	v = m->value;
	if (v != gameOpts->render.bUseLightmaps) {
		gameOpts->render.bUseLightmaps = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nHWObjLighting >= 0) {
	m = menus + lightOpts.nHWObjLighting;
	v = m->value;
	if (v != gameOpts->ogl.bObjLighting) {
		gameOpts->ogl.bObjLighting = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nGunColor >= 0) {
	m = menus + lightOpts.nGunColor;
	v = m->value;
	if (v != gameOpts->render.color.bGunLight) {
		gameOpts->render.color.bGunLight = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nObjectLight >= 0) {
	m = menus + lightOpts.nObjectLight;
	v = m->value;
	if (v != gameOpts->ogl.bLightObjects) {
		gameOpts->ogl.bLightObjects = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nMaxLightsPerFace >= 0) {
	m = menus + lightOpts.nMaxLightsPerFace;
	v = m->value;
	if (v != gameOpts->ogl.nMaxLightsPerFace) {
		gameOpts->ogl.nMaxLightsPerFace = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_FACE, nMaxLightsPerFaceTable [v]);
		m->rebuild = 1;
		return nCurItem;
		}
	}
if (lightOpts.nMaxLightsPerObject >= 0) {
	m = menus + lightOpts.nMaxLightsPerObject;
	v = m->value;
	if (v != gameOpts->ogl.nMaxLightsPerObject) {
		gameOpts->ogl.nMaxLightsPerObject = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_OBJECT, nMaxLightsPerFaceTable [v]);
		m->rebuild = 1;
		return nCurItem;
		}
	}
if (lightOpts.nMaxLightsPerPass >= 0) {
	m = menus + lightOpts.nMaxLightsPerPass;
	v = m->value + 1;
	if (v != gameOpts->ogl.nMaxLightsPerPass) {
		gameOpts->ogl.nMaxLightsPerPass = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_PASS, v);
		m->rebuild = 1;
		return nCurItem;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

static int LightTableIndex (int nValue)
{
	int i, h = (int) sizeofa (nMaxLightsPerFaceTable);

for (i = 0; i < h; i++)
	if (nValue < nMaxLightsPerFaceTable [i])
		break;
return i ? i - 1 : 0;
}

//------------------------------------------------------------------------------

void LightOptionsMenu (void)
{
	CMenuItem m [30];
	int	i, choice = 0;
	int	nOptions;
	int	optColoredLight, optMixColors, optPowerupLights, optFlickerLights, optColorSat, optBrightObjects, nPowerupLight = -1;
#if 0
	int checks;
#endif

	char szMaxLightsPerFace [50];
	char szMaxLightsPerPass [50];
	char szMaxLightsPerObject [50];
	char szLightmapQual [50];

	pszLMapQual [0] = TXT_LOW;
	pszLMapQual [1] = TXT_MEDIUM;
	pszLMapQual [2] = TXT_HIGH;
	pszLMapQual [3] = TXT_VERY_HIGH;
	pszLMapQual [4] = TXT_EXTREME;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	optColorSat = 
	optColoredLight =
	optMixColors = 
	optPowerupLights = -1;
	memset (&lightOpts, 0xff, sizeof (lightOpts));
	if (!gameStates.app.bGameRunning) {
		if ((gameOpts->render.nLightingMethod == 2) && 
			 !(gameStates.render.bUsePerPixelLighting && gameStates.ogl.bShadersOk && gameStates.ogl.bPerPixelLightingOk))
			gameOpts->render.nLightingMethod = 1;
		m.AddRadio (nOptions, TXT_STD_LIGHTING, gameOpts->render.nLightingMethod == 0, KEY_S, 1, NULL);
		lightOpts.nMethod = nOptions++;
		m.AddRadio (nOptions, TXT_VERTEX_LIGHTING, gameOpts->render.nLightingMethod == 1, KEY_V, 1, HTX_VERTEX_LIGHTING);
		nOptions++;
		if (gameStates.render.bUsePerPixelLighting && gameStates.ogl.bShadersOk && gameStates.ogl.bPerPixelLightingOk) {
			m.AddRadio (nOptions, TXT_PER_PIXEL_LIGHTING, gameOpts->render.nLightingMethod == 2, KEY_P, 1, HTX_PER_PIXEL_LIGHTING);
			nOptions++;
			}	
		m.AddText (nOptions, "", 0);
		nOptions++;
		}
	gameOpts->ogl.nMaxLightsPerObject = LightTableIndex (gameOpts->ogl.nMaxLightsPerObject);
	gameOpts->ogl.nMaxLightsPerFace = LightTableIndex (gameOpts->ogl.nMaxLightsPerFace);
	if (gameOpts->render.nLightingMethod) {
#if 0
		m.AddText (nOptions, "", 0);
		nOptions++;
#endif
		if (gameOpts->render.nLightingMethod == 1) {
			if (gameStates.ogl.bHeadlight) {
				m.AddCheck (nOptions, TXT_HW_HEADLIGHT, gameOpts->ogl.bHeadlight, KEY_H, HTX_HW_HEADLIGHT);
				lightOpts.nHWHeadlight = nOptions++;
				}
			m.AddCheck (nOptions, TXT_OBJECT_HWLIGHTING, gameOpts->ogl.bObjLighting, KEY_A, HTX_OBJECT_HWLIGHTING);
			lightOpts.nHWObjLighting = nOptions++;
			if (!gameOpts->ogl.bObjLighting) {
				m.AddCheck (nOptions, TXT_OBJECT_LIGHTING, gameOpts->ogl.bLightObjects, KEY_B, HTX_OBJECT_LIGHTING);
				lightOpts.nObjectLight = nOptions++;
				if (gameOpts->ogl.bLightObjects) {
					m.AddCheck (nOptions, TXT_POWERUP_LIGHTING, gameOpts->ogl.bLightPowerups, KEY_W, HTX_POWERUP_LIGHTING);
					nPowerupLight = nOptions++;
					}
				else
					nPowerupLight = -1;
				}
			}
		sprintf (szMaxLightsPerObject + 1, TXT_MAX_LIGHTS_PER_OBJECT, nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerObject]);
		*szMaxLightsPerObject = *(TXT_MAX_LIGHTS_PER_OBJECT - 1);
		m.AddSlider (nOptions, szMaxLightsPerObject + 1, gameOpts->ogl.nMaxLightsPerObject, 0, (int) sizeofa (nMaxLightsPerFaceTable) - 1, KEY_O, HTX_MAX_LIGHTS_PER_OBJECT);
		lightOpts.nMaxLightsPerObject = nOptions++;

		if (gameOpts->render.nLightingMethod == 2) {
			sprintf (szMaxLightsPerFace + 1, TXT_MAX_LIGHTS_PER_FACE, nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerFace]);
			*szMaxLightsPerFace = *(TXT_MAX_LIGHTS_PER_FACE - 1);
			m.AddSlider (nOptions, szMaxLightsPerFace + 1, gameOpts->ogl.nMaxLightsPerFace, 0,  (int) sizeofa (nMaxLightsPerFaceTable) - 1, KEY_A, HTX_MAX_LIGHTS_PER_FACE);
			lightOpts.nMaxLightsPerFace = nOptions++;
			sprintf (szMaxLightsPerPass + 1, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
			*szMaxLightsPerPass = *(TXT_MAX_LIGHTS_PER_PASS - 1);
			m.AddSlider (nOptions, szMaxLightsPerPass + 1, gameOpts->ogl.nMaxLightsPerPass - 1, 0, 7, KEY_S, HTX_MAX_LIGHTS_PER_PASS);
			lightOpts.nMaxLightsPerPass = nOptions++;
			}
		if (!gameStates.app.bGameRunning && 
			 ((gameOpts->render.nLightingMethod == 2) || ((gameOpts->render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps))) {
			sprintf (szLightmapQual + 1, TXT_LMAP_QUALITY, pszLMapQual [gameOpts->render.nLightmapQuality]);
			*szLightmapQual = *(TXT_LMAP_QUALITY + 1);
			m.AddSlider (nOptions, szLightmapQual + 1, gameOpts->render.nLightmapQuality, 0, 4, KEY_Q, HTX_LMAP_QUALITY);
			lightOpts.nLightmapQual = nOptions++;
			}

		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddRadio (nOptions, TXT_FULL_COLORSAT, 0, KEY_F, 2, HTX_COLOR_SATURATION);
		optColorSat = nOptions++;
		m.AddRadio (nOptions, TXT_LIMIT_COLORSAT, 0, KEY_L, 2, HTX_COLOR_SATURATION);
		nOptions++;
		m.AddRadio (nOptions, TXT_NO_COLORSAT, 0, KEY_N, 2, HTX_COLOR_SATURATION);
		nOptions++;
		m [optColorSat + NMCLAMP (gameOpts->render.color.nSaturation, 0, 2)].value = 1;
		m.AddText (nOptions, "", 0);
		nOptions++;
		}
	if (gameOpts->render.nLightingMethod != 1)
		lightOpts.nLightmaps = -1;
	else {
		m.AddCheck (nOptions, TXT_USE_LIGHTMAPS, gameOpts->render.bUseLightmaps, KEY_G, HTX_USE_LIGHTMAPS);
		lightOpts.nLightmaps = nOptions++;
		}
	if (gameOpts->render.nLightingMethod < 2) {
		m.AddCheck (nOptions, TXT_USE_COLOR, gameOpts->render.color.bAmbientLight, KEY_C, HTX_RENDER_AMBICOLOR);
		optColoredLight = nOptions++;
		}
	if (!gameOpts->render.nLightingMethod) {
		m.AddCheck (nOptions, TXT_USE_WPNCOLOR, gameOpts->render.color.bGunLight, KEY_W, HTX_RENDER_WPNCOLOR);
		lightOpts.nGunColor = nOptions++;
		if (gameOpts->app.bExpertMode) {
			if (!gameOpts->render.nLightingMethod && gameOpts->render.color.bGunLight) {
				m.AddCheck (nOptions, TXT_MIX_COLOR, gameOpts->render.color.bMix, KEY_X, HTX_ADVRND_MIXCOLOR);
				optMixColors = nOptions++;
				}
			}
		}
	m.AddCheck (nOptions, TXT_POWERUPLIGHTS, !extraGameInfo [0].bPowerupLights, KEY_P, HTX_POWERUPLIGHTS);
	optPowerupLights = nOptions++;
	m.AddCheck (nOptions, TXT_FLICKERLIGHTS, extraGameInfo [0].bFlickerLights, KEY_F, HTX_FLICKERLIGHTS);
	optFlickerLights = nOptions++;
	if (gameOpts->render.bHiresModels) {
		m.AddCheck (nOptions, TXT_BRIGHT_OBJECTS, extraGameInfo [0].bBrightObjects, KEY_B, HTX_BRIGHT_OBJECTS);
		optBrightObjects = nOptions++;
		}
	else
		optBrightObjects = -1;
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_LIGHTING_MENUTITLE, nOptions, m, &LightOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (gameOpts->render.nLightingMethod == 1) {
		if (lightOpts.nObjectLight >= 0) {
			gameOpts->ogl.bLightObjects = m [lightOpts.nObjectLight].value;
			if (nPowerupLight >= 0)
				gameOpts->ogl.bLightPowerups = m [nPowerupLight].value;
			}
		}
	if (gameOpts->render.nLightingMethod < 2) {
		if (optColoredLight >= 0)
			gameOpts->render.color.bAmbientLight = m [optColoredLight].value;
		if (lightOpts.nGunColor >= 0)
			gameOpts->render.color.bGunLight = m [lightOpts.nGunColor].value;
		if (gameOpts->app.bExpertMode) {
			if (gameStates.render.color.bLightmapsOk && gameOpts->render.color.bUseLightmaps)
			gameStates.ogl.nContrast = 8;
			if (gameOpts->render.color.bGunLight)
				GET_VAL (gameOpts->render.color.bMix, optMixColors);
#if EXPMODE_DEFAULTS
				else
					gameOpts->render.color.bMix = 1;
#endif
			}
		}
	if (optPowerupLights >= 0)
		extraGameInfo [0].bPowerupLights = !m [optPowerupLights].value;
	extraGameInfo [0].bFlickerLights = m [optFlickerLights].value;
	GET_VAL (gameOpts->ogl.bHeadlight, lightOpts.nHWHeadlight);
	GET_VAL (extraGameInfo [0].bBrightObjects, optBrightObjects);
	gameOpts->ogl.nMaxLightsPerFace = nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerFace];
	gameOpts->ogl.nMaxLightsPerObject = nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerObject];
	} while (i == -2);
if (optColorSat >= 0) {
	for (i = 0; i < 3; i++)
		if (m [optColorSat + i].value) {
			gameOpts->render.color.nSaturation = i;
			break;
			}
	}
gameStates.render.nLightingMethod = gameOpts->render.nLightingMethod;
if (gameStates.render.nLightingMethod == 2)
	gameStates.render.bPerPixelLighting = 2;
else if ((gameStates.render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps)
	gameStates.render.bPerPixelLighting = 1;
else
	gameStates.render.bPerPixelLighting = 0;
if (gameStates.render.bPerPixelLighting == 2) {
	gameStates.render.nMaxLightsPerPass = gameOpts->ogl.nMaxLightsPerPass;
	gameStates.render.nMaxLightsPerFace = gameOpts->ogl.nMaxLightsPerFace;
	}
gameStates.render.nMaxLightsPerObject = gameOpts->ogl.nMaxLightsPerObject;
gameStates.render.bAmbientColor = gameStates.render.bPerPixelLighting || gameOpts->render.color.bAmbientLight;
}

//------------------------------------------------------------------------------

void MovieOptionsMenu (void)
{
	CMenuItem m [5];
	int	i, choice = 0;
	int	nOptions;
	int	optMovieQual, optMovieSize, optSubTitles;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	m.AddCheck (nOptions, TXT_MOVIE_SUBTTL, gameOpts->movies.bSubTitles, KEY_O, HTX_RENDER_SUBTTL);
	optSubTitles = nOptions++;
	if (gameOpts->app.bExpertMode) {
		m.AddCheck (nOptions, TXT_MOVIE_QUAL, gameOpts->movies.nQuality, KEY_Q, HTX_RENDER_MOVIEQUAL);
		optMovieQual = nOptions++;
		m.AddCheck (nOptions, TXT_MOVIE_FULLSCR, gameOpts->movies.bResize, KEY_U, HTX_RENDER_MOVIEFULL);
		optMovieSize = nOptions++;
		}
	else
		optMovieQual = 
		optMovieSize = -1;

	for (;;) {
		i = ExecMenu1 (NULL, TXT_MOVIE_OPTIONS, nOptions, m, NULL, &choice);
		if (i < 0)
			break;
		} 
	gameOpts->movies.bSubTitles = m [optSubTitles].value;
	if (gameOpts->app.bExpertMode) {
		gameOpts->movies.nQuality = m [optMovieQual].value;
		gameOpts->movies.bResize = m [optMovieSize].value;
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

static const char *pszShipColors [8];

int ShipRenderOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + shipRenderOpts.nWeapons;
v = m->value;
if (v != extraGameInfo [0].bShowWeapons) {
	extraGameInfo [0].bShowWeapons = v;
	*key = -2;
	}
if (shipRenderOpts.nColor >= 0) {
	m = menus + shipRenderOpts.nColor;
	v = m->value;
	if (v != gameOpts->render.ship.nColor) {
		gameOpts->render.ship.nColor = v;
		sprintf (m->text, TXT_SHIPCOLOR, pszShipColors [v]);
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void ShipRenderOptionsMenu (void)
{
	CMenuItem m [10];
	int	i, j, choice = 0;
	int	nOptions;
	int	optBullets, optWingtips;
	char	szShipColor [50];

pszShipColors [0] = TXT_SHIP_WHITE;
pszShipColors [1] = TXT_SHIP_BLUE;
pszShipColors [2] = TXT_RED;
pszShipColors [3] = TXT_SHIP_GREEN;
pszShipColors [4] = TXT_SHIP_YELLOW;
pszShipColors [5] = TXT_SHIP_PURPLE;
pszShipColors [6] = TXT_SHIP_ORANGE;
pszShipColors [7] = TXT_SHIP_CYAN;
do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	m.AddCheck (nOptions, TXT_SHIP_WEAPONS, extraGameInfo [0].bShowWeapons, KEY_W, HTX_SHIP_WEAPONS);
	shipRenderOpts.nWeapons = nOptions++;
	if (extraGameInfo [0].bShowWeapons) {
		m.AddCheck (nOptions, TXT_SHIP_BULLETS, gameOpts->render.ship.bBullets, KEY_B, HTX_SHIP_BULLETS);
		optBullets = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddRadio (nOptions, TXT_SHIP_WINGTIP_LASER, 0, KEY_A, 1, HTX_SHIP_WINGTIPS);
		optWingtips = nOptions++;
		m.AddRadio (nOptions, TXT_SHIP_WINGTIP_SUPLAS, 0, KEY_U, 1, HTX_SHIP_WINGTIPS);
		nOptions++;
		m.AddRadio (nOptions, TXT_SHIP_WINGTIP_SHORT, 0, KEY_S, 1, HTX_SHIP_WINGTIPS);
		nOptions++;
		m.AddRadio (nOptions, TXT_SHIP_WINGTIP_LONG, 0, KEY_L, 1, HTX_SHIP_WINGTIPS);
		nOptions++;
		m [optWingtips + gameOpts->render.ship.nWingtip].value = 1;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddText (nOptions, TXT_SHIPCOLOR_HEADER, 0);
		nOptions++;
		sprintf (szShipColor + 1, TXT_SHIPCOLOR, pszShipColors [gameOpts->render.ship.nColor]);
		*szShipColor = 0;
		m.AddSlider (nOptions, szShipColor + 1, gameOpts->render.ship.nColor, 0, 7, KEY_C, HTX_SHIPCOLOR);
		shipRenderOpts.nColor = nOptions++;
		}
	else
		optBullets =
		optWingtips =
		shipRenderOpts.nColor = -1;
	for (;;) {
		i = ExecMenu1 (NULL, TXT_SHIP_RENDERMENU, nOptions, m, ShipRenderOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if ((extraGameInfo [0].bShowWeapons = m [shipRenderOpts.nWeapons].value)) {
		gameOpts->render.ship.bBullets = m [optBullets].value;
		for (j = 0; j < 4; j++)
			if (m [optWingtips + j].value) {
				gameOpts->render.ship.nWingtip = j;
				break;
				}
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

int RenderOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

if (!gameStates.app.bNostalgia) {
	m = menus + optBrightness;
	v = m->value;
	if (v != paletteManager.GetGamma ())
		paletteManager.SetGamma (v);
	}
m = menus + renderOpts.nMaxFPS;
v = fpsTable [m->value];
if (gameOpts->render.nMaxFPS != (v ? v : 1)) {
	gameOpts->render.nMaxFPS = v ? v : 1;
	if (v)
		sprintf (m->text, TXT_FRAMECAP, gameOpts->render.nMaxFPS);
	else
		sprintf (m->text, TXT_NO_FRAMECAP);
	m->rebuild = 1;
	}
if (gameOpts->app.bExpertMode) {
	if (optContrast >= 0) {
		m = menus + optContrast;
		v = m->value;
		if (v != gameStates.ogl.nContrast) {
			gameStates.ogl.nContrast = v;
			sprintf (m->text, TXT_CONTRAST, ContrastText ());
			m->rebuild = 1;
			}
		}
	m = menus + renderOpts.nRenderQual;
	v = m->value;
	if (gameOpts->render.nQuality != v) {
		gameOpts->render.nQuality = v;
		sprintf (m->text, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		m->rebuild = 1;
		}
	if (renderOpts.nTexQual > 0) {
		m = menus + renderOpts.nTexQual;
		v = m->value;
		if (gameOpts->render.textures.nQuality != v) {
			gameOpts->render.textures.nQuality = v;
			sprintf (m->text, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			m->rebuild = 1;
			}
		}
	if (renderOpts.nMeshQual > 0) {
		m = menus + renderOpts.nMeshQual;
		v = m->value;
		if (gameOpts->render.nMeshQuality != v) {
			gameOpts->render.nMeshQuality = v;
			sprintf (m->text, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			m->rebuild = 1;
			}
		}
	m = menus + renderOpts.nWallTransp;
	v = (FADE_LEVELS * m->value + 5) / 10;
	if (extraGameInfo [0].grWallTransparency != v) {
		extraGameInfo [0].grWallTransparency = v;
		sprintf (m->text, TXT_WALL_TRANSP, m->value * 10, '%');
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void RenderOptionsMenu (void)
{
	CMenuItem m [40];
	int	h, i, choice = 0;
	int	nOptions;
	int	optSmokeOpts, optShadowOpts, optCameraOpts, optLightOpts, optMovieOpts,
			optAdvOpts, optEffectOpts, optPowerupOpts, optAutomapOpts, optLightningOpts,
			optUseGamma, optColoredWalls, optDepthSort, optCoronaOpts, optShipRenderOpts;
#if DBG
	int	optWireFrame, optTextures, optObjects, optWalls, optDynLight;
#endif

	char szMaxFps [50];
	char szWallTransp [50];
	char szRendQual [50];
	char szTexQual [50];
	char szMeshQual [50];
	char szContrast [50];

	int nRendQualSave = gameOpts->render.nQuality;

	pszRendQual [0] = TXT_QUALITY_LOW;
	pszRendQual [1] = TXT_QUALITY_MED;
	pszRendQual [2] = TXT_QUALITY_HIGH;
	pszRendQual [3] = TXT_VERY_HIGH;
	pszRendQual [4] = TXT_QUALITY_MAX;

	pszTexQual [0] = TXT_QUALITY_LOW;
	pszTexQual [1] = TXT_QUALITY_MED;
	pszTexQual [2] = TXT_QUALITY_HIGH;
	pszTexQual [3] = TXT_QUALITY_MAX;

	pszMeshQual [0] = TXT_NONE;
	pszMeshQual [1] = TXT_SMALL;
	pszMeshQual [2] = TXT_MEDIUM;
	pszMeshQual [3] = TXT_HIGH;
	pszMeshQual [4] = TXT_EXTREME;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	optPowerupOpts = optAutomapOpts = -1;
	if (!gameStates.app.bNostalgia) {
		m.AddSlider (nOptions, TXT_BRIGHTNESS, paletteManager.GetGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
		optBrightness = nOptions++;
		}
	if (gameOpts->render.nMaxFPS > 1)
		sprintf (szMaxFps + 1, TXT_FRAMECAP, gameOpts->render.nMaxFPS);
	else
		sprintf (szMaxFps + 1, TXT_NO_FRAMECAP);
	*szMaxFps = *(TXT_FRAMECAP - 1);
	m.AddSlider (nOptions, szMaxFps + 1, FindTableFps (gameOpts->render.nMaxFPS), 0, 15, KEY_F, HTX_RENDER_FRAMECAP);
	renderOpts.nMaxFPS = nOptions++;

	if (gameOpts->app.bExpertMode) {
		if ((gameOpts->render.nLightingMethod < 2) && !gameOpts->render.bUseLightmaps) {
			sprintf (szContrast, TXT_CONTRAST, ContrastText ());
			m.AddSlider (nOptions, szContrast, gameStates.ogl.nContrast, 0, 16, KEY_C, HTX_ADVRND_CONTRAST);
			optContrast = nOptions++;
			}
		sprintf (szRendQual + 1, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		*szRendQual = *(TXT_RENDQUAL - 1);
		m.AddSlider (nOptions, szRendQual + 1, gameOpts->render.nQuality, 0, 4, KEY_Q, HTX_ADVRND_RENDQUAL);
		renderOpts.nRenderQual = nOptions++;
		if (gameStates.app.bGameRunning)
			renderOpts.nTexQual =
			renderOpts.nMeshQual = 
			lightOpts.nLightmapQual = -1;
		else {
			sprintf (szTexQual + 1, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			*szTexQual = *(TXT_TEXQUAL + 1);
			m.AddSlider (nOptions, szTexQual + 1, gameOpts->render.textures.nQuality, 0, 3, KEY_U, HTX_ADVRND_TEXQUAL);
			renderOpts.nTexQual = nOptions++;
			if ((gameOpts->render.nLightingMethod == 1) && !gameOpts->render.bUseLightmaps) {
				sprintf (szMeshQual + 1, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
				*szMeshQual = *(TXT_MESH_QUALITY + 1);
				m.AddSlider (nOptions, szMeshQual + 1, gameOpts->render.nMeshQuality, 0, 4, KEY_O, HTX_MESH_QUALITY);
				renderOpts.nMeshQual = nOptions++;
				}
			else
				renderOpts.nMeshQual = -1;
			}
		m.AddText (nOptions, "", 0);
		nOptions++;
		h = extraGameInfo [0].grWallTransparency * 10 / FADE_LEVELS;
		sprintf (szWallTransp + 1, TXT_WALL_TRANSP, h * 10, '%');
		*szWallTransp = *(TXT_WALL_TRANSP - 1);
		m.AddSlider (nOptions, szWallTransp + 1, h, 0, 10, KEY_T, HTX_ADVRND_WALLTRANSP);
		renderOpts.nWallTransp = nOptions++;
		m.AddCheck (nOptions, TXT_COLOR_WALLS, gameOpts->render.color.bWalls, KEY_W, HTX_ADVRND_COLORWALLS);
		optColoredWalls = nOptions++;
		if (RENDERPATH)
			optDepthSort = -1;
		else {
			m.AddCheck (nOptions, TXT_TRANSP_DEPTH_SORT, gameOpts->render.bDepthSort, KEY_D, HTX_TRANSP_DEPTH_SORT);
			optDepthSort = nOptions++;
			}
#if 0
		m.AddCheck (nOptions, TXT_GAMMA_BRIGHT, gameOpts->ogl.bSetGammaRamp, KEY_V, HTX_ADVRND_GAMMA);
		optUseGamma = nOptions++;
#else
		optUseGamma = -1;
#endif
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddMenu (nOptions, TXT_LIGHTING_OPTIONS, KEY_L, HTX_RENDER_LIGHTINGOPTS);
		optLightOpts = nOptions++;
		m.AddMenu (nOptions, TXT_SMOKE_OPTIONS, KEY_S, HTX_RENDER_SMOKEOPTS);
		optSmokeOpts = nOptions++;
		m.AddMenu (nOptions, TXT_LIGHTNING_OPTIONS, KEY_I, HTX_LIGHTNING_OPTIONS);
		optLightningOpts = nOptions++;
		if (!(gameStates.app.bEnableShadows && gameStates.render.bHaveStencilBuffer))
			optShadowOpts = -1;
		else {
			m.AddMenu (nOptions, TXT_SHADOW_OPTIONS, KEY_A, HTX_RENDER_SHADOWOPTS);
			optShadowOpts = nOptions++;
			}
		m.AddMenu (nOptions, TXT_EFFECT_OPTIONS, KEY_E, HTX_RENDER_EFFECTOPTS);
		optEffectOpts = nOptions++;
		m.AddMenu (nOptions, TXT_CORONA_OPTIONS, KEY_O, HTX_RENDER_CORONAOPTS);
		optCoronaOpts = nOptions++;
		m.AddMenu (nOptions, TXT_CAMERA_OPTIONS, KEY_C, HTX_RENDER_CAMERAOPTS);
		optCameraOpts = nOptions++;
		m.AddMenu (nOptions, TXT_POWERUP_OPTIONS, KEY_P, HTX_RENDER_PRUPOPTS);
		optPowerupOpts = nOptions++;
		m.AddMenu (nOptions, TXT_AUTOMAP_OPTIONS, KEY_M, HTX_RENDER_AUTOMAPOPTS);
		optAutomapOpts = nOptions++;
		m.AddMenu (nOptions, TXT_SHIP_RENDEROPTIONS, KEY_H, HTX_RENDER_SHIPOPTS);
		optShipRenderOpts = nOptions++;
		m.AddMenu (nOptions, TXT_MOVIE_OPTIONS, KEY_M, HTX_RENDER_MOVIEOPTS);
		optMovieOpts = nOptions++;
		}
	else
		renderOpts.nRenderQual =
		renderOpts.nTexQual =
		renderOpts.nMeshQual =
		renderOpts.nWallTransp = 
		optUseGamma = 
		optColoredWalls =
		optDepthSort =
		optContrast =
		optLightOpts =
		optLightningOpts =
		optSmokeOpts =
		optShadowOpts =
		optEffectOpts =
		optCoronaOpts =
		optCameraOpts = 
		optMovieOpts = 
		optShipRenderOpts =
		optAdvOpts = -1;

#if DBG
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddCheck (nOptions, "Draw wire frame", gameOpts->render.debug.bWireFrame, 0, NULL);
	optWireFrame = nOptions++;
	m.AddCheck (nOptions, "Draw textures", gameOpts->render.debug.bTextures, 0, NULL);
	optTextures = nOptions++;
	m.AddCheck (nOptions, "Draw walls", gameOpts->render.debug.bWalls, 0, NULL);
	optWalls = nOptions++;
	m.AddCheck (nOptions, "Draw objects", gameOpts->render.debug.bObjects, 0, NULL);
	optObjects = nOptions++;
	m.AddCheck (nOptions, "Dynamic Light", gameOpts->render.debug.bDynamicLight, 0, NULL);
	optDynLight = nOptions++;
#endif

	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_RENDER_OPTS, nOptions, m, &RenderOptionsCallback, &choice);
		if (i < 0)
			break;
		if (gameOpts->app.bExpertMode) {
			if ((optLightOpts >= 0) && (i == optLightOpts))
				i = -2, LightOptionsMenu (void);
			else if ((optSmokeOpts >= 0) && (i == optSmokeOpts))
				i = -2, SmokeOptionsMenu (void);
			else if ((optLightningOpts >= 0) && (i == optLightningOpts))
				i = -2, LightningOptionsMenu (void);
			else if ((optShadowOpts >= 0) && (i == optShadowOpts))
				i = -2, ShadowOptionsMenu (void);
			else if ((optEffectOpts >= 0) && (i == optEffectOpts))
				i = -2, EffectOptionsMenu (void);
			else if ((optCoronaOpts >= 0) && (i == optCoronaOpts))
				i = -2, CoronaOptionsMenu (void);
			else if ((optCameraOpts >= 0) && (i == optCameraOpts))
				i = -2, CameraOptionsMenu (void);
			else if ((optPowerupOpts >= 0) && (i == optPowerupOpts))
				i = -2, PowerupOptionsMenu (void);
			else if ((optAutomapOpts >= 0) && (i == optAutomapOpts))
				i = -2, AutomapOptionsMenu (void);
			else if ((optMovieOpts >= 0) && (i == optMovieOpts))
				i = -2, MovieOptionsMenu (void);
			else if ((optShipRenderOpts >= 0) && (i == optShipRenderOpts))
				i = -2, ShipRenderOptionsMenu (void);
			}
		} while (i >= 0);
	if (!gameStates.app.bNostalgia)
		paletteManager.SetGamma (m [optBrightness].value);
	if (gameOpts->app.bExpertMode) {
		gameOpts->render.color.bWalls = m [optColoredWalls].value;
		GET_VAL (gameOpts->render.bDepthSort, optDepthSort);
		GET_VAL (gameOpts->ogl.bSetGammaRamp, optUseGamma);
		if (optContrast >= 0)
			gameStates.ogl.nContrast = m [optContrast].value;
		if (nRendQualSave != gameOpts->render.nQuality)
			SetRenderQuality ();
		}
#if EXPMODE_DEFAULTS
	else {
		gameOpts->render.nMaxFPS = 250;
		gameOpts->render.color.nLightmapRange = 5;
		gameOpts->render.color.bMix = 1;
		gameOpts->render.nQuality = 3;
		gameOpts->render.color.bWalls = 1;
		gameOpts->render.effects.bTransparent = 1;
		gameOpts->render.particles.bPlayers = 0;
		gameOpts->render.particles.bRobots =
		gameOpts->render.particles.bMissiles = 1;
		gameOpts->render.particles.bCollisions = 0;
		gameOpts->render.particles.bDisperse = 0;
		gameOpts->render.particles.nDens = 2;
		gameOpts->render.particles.nSize = 3;
		gameOpts->render.cameras.bFitToWall = 0;
		gameOpts->render.cameras.nSpeed = 5000;
		gameOpts->render.cameras.nFPS = 0;
		gameOpts->movies.nQuality = 0;
		gameOpts->movies.bResize = 1;
		gameStates.ogl.nContrast = 8;
		gameOpts->ogl.bSetGammaRamp = 0;
		}
#endif
#if DBG
	gameOpts->render.debug.bWireFrame = m [optWireFrame].value;
	gameOpts->render.debug.bTextures = m [optTextures].value;
	gameOpts->render.debug.bObjects = m [optObjects].value;
	gameOpts->render.debug.bWalls = m [optWalls].value;
	gameOpts->render.debug.bDynamicLight = m [optDynLight].value;
#endif
	} while (i == -2);
}

//------------------------------------------------------------------------------
//eof
