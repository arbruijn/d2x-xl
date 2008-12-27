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

#if DBG

const char *menuBgNames [4][2] = {
 {"menu.pcx", "menub.pcx"},
 {"menuo.pcx", "menuob.pcx"},
 {"menud.pcx", "menud.pcx"},
 {"menub.pcx", "menub.pcx"}
	};

#else

const char *menuBgNames [4][2] = {
 {"\x01menu.pcx", "\x01menub.pcx"},
 {"\x01menuo.pcx", "\x01menuob.pcx"},
 {"\x01menud.pcx", "\x01menud.pcx"},
 {"\x01menub.pcx", "\x01menub.pcx"}
	};
#endif

//------------------------------------------------------------------------------

static struct {
	int	nUseCompSpeed;
	int	nCompSpeed;
} performanceOpts;

//------------------------------------------------------------------------------

int DifficultyMenu (void)
{
	int		i, choice = gameStates.app.nDifficultyLevel;
	CMenu		m (5);

for (i = 0; i < 5; i++)
	m.AddMenu ( MENU_DIFFICULTY_TEXT (i), 0, "");
i = m.Menu (NULL, TXT_DIFFICULTY_LEVEL, NDL, m, NULL, &choice);
if (i <= -1)
	return 0;
if (choice != gameStates.app.nDifficultyLevel) {       
	gameStates.app.nDifficultyLevel = choice;
	WritePlayerFile ();
	}
return 1;
}

//------------------------------------------------------------------------------

typedef struct tDetailData {
	ubyte		renderDepths [NUM_DETAIL_LEVELS - 1];
	sbyte		maxPerspectiveDepths [NUM_DETAIL_LEVELS - 1];
	sbyte		maxLinearDepths [NUM_DETAIL_LEVELS - 1];
	sbyte		maxLinearDepthObjects [NUM_DETAIL_LEVELS - 1];
	sbyte		maxDebrisObjects [NUM_DETAIL_LEVELS - 1];
	sbyte		maxObjsOnScreenDetailed [NUM_DETAIL_LEVELS - 1];
	sbyte		simpleModelThresholdScales [NUM_DETAIL_LEVELS - 1];
	sbyte		nSoundChannels [NUM_DETAIL_LEVELS - 1];
} tDetailData;

tDetailData	detailData = {
 {15, 31, 63, 127, 255},
 { 1,  2,  3,   5,   8},
 { 3,  5,  7,  10,  50},
 { 1,  2,  3,   7,  20},
 { 2,  4,  7,  10,  15},
 { 2,  4,  7,  10,  15},
 { 2,  4,  8,  16,  50},
 { 2,  8, 16,  32,  64}};


// -----------------------------------------------------------------------------
// Set detail level based stuff.
// Note: Highest detail level (gameStates.render.detail.nLevel == NUM_DETAIL_LEVELS-1) is custom detail level.
void InitDetailLevels (int nDetailLevel)
{
	Assert ((nDetailLevel >= 0) && (nDetailLevel < NUM_DETAIL_LEVELS));

if (nDetailLevel < NUM_DETAIL_LEVELS - 1) {
	gameStates.render.detail.nRenderDepth = detailData.renderDepths [nDetailLevel];
	gameStates.render.detail.nMaxPerspectiveDepth = detailData.maxPerspectiveDepths [nDetailLevel];
	gameStates.render.detail.nMaxLinearDepth = detailData.maxLinearDepths [nDetailLevel];
	gameStates.render.detail.nMaxLinearDepthObjects = detailData.maxLinearDepthObjects [nDetailLevel];
	gameStates.render.detail.nMaxDebrisObjects = detailData.maxDebrisObjects [nDetailLevel];
	gameStates.render.detail.nMaxObjectsOnScreenDetailed = detailData.maxObjsOnScreenDetailed [nDetailLevel];
	gameData.models.nSimpleModelThresholdScale = detailData.simpleModelThresholdScales [nDetailLevel];
	DigiSetMaxChannels (detailData.nSoundChannels [nDetailLevel]);
	//      Set custom menu defaults.
	gameStates.render.detail.nObjectComplexity = nDetailLevel;
	gameStates.render.detail.nWallRenderDepth = nDetailLevel;
	gameStates.render.detail.nObjectDetail = nDetailLevel;
	gameStates.render.detail.nWallDetail = nDetailLevel;
	gameStates.render.detail.nDebrisAmount = nDetailLevel;
	gameStates.sound.nSoundChannels = nDetailLevel;
	gameStates.render.detail.nLevel = nDetailLevel;
	}
}

// -----------------------------------------------------------------------------

void DetailLevelMenu (void)
{
	int		i, choice = gameStates.app.nDetailLevel;
	CMenu		m (8);

	char szMenuDetails [5][20];

for (i = 0; i < 5; i++) {
	sprintf (szMenuDetails [i], "%d. %s", i + 1, MENU_DETAIL_TEXT (i));
	m.AddMenu (szMenuDetails [i], 0, HTX_ONLINE_MANUAL);
	}
m.AddText ("", 0);
m.AddMenu (MENU_DETAIL_TEXT (5), KEY_C, HTX_ONLINE_MANUAL);
m.AddCheck (TXT_HIRES_MOVIES, gameOpts->movies.bHires, KEY_S, HTX_ONLINE_MANUAL);
i = m.Menu (NULL, TXT_DETAIL_LEVEL, NDL + 3, m, NULL, &choice);
if (i > -1) {
	switch (choice) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			gameStates.app.nDetailLevel = choice;
			InitDetailLevels (gameStates.app.nDetailLevel);
			break;
		case 6:
			gameStates.app.nDetailLevel = 5;
			CustomDetailsMenu ();
			break;
		}
	}
gameOpts->movies.bHires = m [7].value;
}

// -----------------------------------------------------------------------------

int CustomDetailsCallback (CMenu& m, int *nLastKey, int nCurItem)
{
	nitems = nitems;
	*nLastKey = *nLastKey;
	nCurItem = nCurItem;

gameStates.render.detail.nObjectComplexity = m [0].value;
gameStates.render.detail.nObjectDetail = m [1].value;
gameStates.render.detail.nWallDetail = m [2].value;
gameStates.render.detail.nWallRenderDepth = m [3].value;
gameStates.render.detail.nDebrisAmount = m [4].value;
if (!gameStates.app.bGameRunning)
	gameStates.sound.nSoundChannels = m [5].value;
return nCurItem;
}

// -----------------------------------------------------------------------------

void SetMaxCustomDetails (void)
{
gameStates.render.detail.nRenderDepth = detailData.renderDepths [NDL - 1];
gameStates.render.detail.nMaxPerspectiveDepth = detailData.maxPerspectiveDepths [NDL - 1];
gameStates.render.detail.nMaxLinearDepth = detailData.maxLinearDepths [NDL - 1];
gameStates.render.detail.nMaxDebrisObjects = detailData.maxDebrisObjects [NDL - 1];
gameStates.render.detail.nMaxObjectsOnScreenDetailed = detailData.maxObjsOnScreenDetailed [NDL - 1];
gameData.models.nSimpleModelThresholdScale = detailData.simpleModelThresholdScales [NDL - 1];
gameStates.render.detail.nMaxLinearDepthObjects = detailData.maxLinearDepthObjects [NDL - 1];
}

// -----------------------------------------------------------------------------

void InitCustomDetails (void)
{
gameStates.render.detail.nRenderDepth = detailData.renderDepths [gameStates.render.detail.nWallRenderDepth];
gameStates.render.detail.nMaxPerspectiveDepth = detailData.maxPerspectiveDepths [gameStates.render.detail.nWallDetail];
gameStates.render.detail.nMaxLinearDepth = detailData.maxLinearDepths [gameStates.render.detail.nWallDetail];
gameStates.render.detail.nMaxDebrisObjects = detailData.maxDebrisObjects [gameStates.render.detail.nDebrisAmount];
gameStates.render.detail.nMaxObjectsOnScreenDetailed = detailData.maxObjsOnScreenDetailed [gameStates.render.detail.nObjectComplexity];
gameData.models.nSimpleModelThresholdScale = detailData.simpleModelThresholdScales [gameStates.render.detail.nObjectComplexity];
gameStates.render.detail.nMaxLinearDepthObjects = detailData.maxLinearDepthObjects [gameStates.render.detail.nObjectDetail];
DigiSetMaxChannels (detailData.nSoundChannels [gameStates.sound.nSoundChannels]);
}

#define	DL_MAX	10

// -----------------------------------------------------------------------------

void CustomDetailsMenu (void)
{
	int	nOptions;
	int	i, choice = 0;
	CMenuItem m [DL_MAX];

do {
	nOptions = 0;
	memset (m, 0, sizeof (m));
	m.AddSlider (nOptions, TXT_OBJ_COMPLEXITY, gameStates.render.detail.nObjectComplexity, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	nOptions++;
	m.AddSlider (nOptions, TXT_OBJ_DETAIL, gameStates.render.detail.nObjectDetail, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	nOptions++;
	m.AddSlider (nOptions, TXT_WALL_DETAIL, gameStates.render.detail.nWallDetail, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	nOptions++;
	m.AddSlider (nOptions, TXT_WALL_RENDER_DEPTH, gameStates.render.detail.nWallRenderDepth, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	nOptions++;
	m.AddSlider (nOptions, TXT_DEBRIS_AMOUNT, gameStates.render.detail.nDebrisAmount, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	nOptions++;
	if (!gameStates.app.bGameRunning) {
		m.AddSlider (nOptions, TXT_SOUND_CHANNELS, gameStates.sound.nSoundChannels, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
		nOptions++;
		}
	m.AddText (nOptions, TXT_LO_HI, 0);
	nOptions++;
	Assert (sizeofa (m) >= nOptions);
	i = ExecMenu1 (NULL, TXT_DETAIL_CUSTOM, nOptions, m, CustomDetailsCallback, &choice);
} while (i > -1);
InitCustomDetails ();
}

//------------------------------------------------------------------------------

void UseDefaultPerformanceSettings (void)
{
if (gameStates.app.bNostalgia || !gameStates.app.bUseDefaults)
	return;
	
SetMaxCustomDetails ();
gameOpts->render.nMaxFPS = 60;
gameOpts->render.effects.bTransparent = 1;
gameOpts->render.color.nLightmapRange = 5;
gameOpts->render.color.bMix = 1;
gameOpts->render.color.bWalls = 1;
gameOpts->render.cameras.bFitToWall = 0;
gameOpts->render.cameras.nSpeed = 5000;
gameOpts->ogl.bSetGammaRamp = 0;
gameStates.ogl.nContrast = 8;
if (gameStates.app.nCompSpeed == 0) {
	gameOpts->render.bUseLightmaps = 0;
	gameOpts->render.nQuality = 1;
	gameOpts->render.cockpit.bTextGauges = 1;
	gameOpts->render.nLightingMethod = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->movies.nQuality = 0;
	gameOpts->movies.bResize = 0;
	gameOpts->render.shadows.nClip = 0;
	gameOpts->render.shadows.nReach = 0;
	gameOpts->render.effects.nShrapnels = 0;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.coronas.nStyle = 0;
	gameOpts->ogl.nMaxLightsPerFace = 4;
	gameOpts->ogl.nMaxLightsPerPass = 4;
	gameOpts->ogl.nMaxLightsPerObject = 4;
	extraGameInfo [0].bShadows = 0;
	extraGameInfo [0].bUseParticles = 0;
	extraGameInfo [0].bUseLightnings = 0;
	extraGameInfo [0].bUseCameras = 0;
	extraGameInfo [0].bPlayerShield = 0;
	extraGameInfo [0].bThrusterFlames = 0;
	extraGameInfo [0].bDamageExplosions = 0;
	}
else if (gameStates.app.nCompSpeed == 1) {
	gameOpts->render.nQuality = 2;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 0;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 1;
	gameOpts->render.particles.bStatic = 0;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 0;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 0;
	gameOpts->render.coronas.nStyle = 0;
	gameOpts->render.cockpit.bTextGauges = 1;
	gameOpts->render.nLightingMethod = 0;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->ogl.nMaxLightsPerFace = 8;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 8;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cameras.nFPS = 5;
	gameOpts->movies.nQuality = 0;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 0;
	gameOpts->render.shadows.nReach = 0;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 0;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 0;
	}
else if (gameStates.app.nCompSpeed == 2) {
	gameOpts->render.nQuality = 2;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 0;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 2;
	gameOpts->render.particles.bStatic = 1;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 1;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 0;
	gameOpts->render.coronas.nStyle = 1;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.lightnings.nQuality = 1;
	gameOpts->render.lightnings.nStyle = 1;
	gameOpts->render.lightnings.bPlasma = 0;
	gameOpts->render.lightnings.bPlayers = 0;
	gameOpts->render.lightnings.bRobots = 0;
	gameOpts->render.lightnings.bDamage = 0;
	gameOpts->render.lightnings.bExplosions = 0;
	gameOpts->render.lightnings.bStatic = 0;
	gameOpts->render.lightnings.bOmega = 1;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->ogl.nMaxLightsPerFace = 12;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 8;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cameras.nFPS = 0;
	gameOpts->movies.nQuality = 0;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 1;
	gameOpts->render.shadows.nReach = 1;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
else if (gameStates.app.nCompSpeed == 3) {
	gameOpts->render.nQuality = 3;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 1;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 3;
	gameOpts->render.particles.bStatic = 1;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 2;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 1;
	gameOpts->render.coronas.nStyle = 2;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.lightnings.nQuality = 1;
	gameOpts->render.lightnings.nStyle = 1;
	gameOpts->render.lightnings.bPlasma = 0;
	gameOpts->render.lightnings.bPlayers = 1;
	gameOpts->render.lightnings.bRobots = 1;
	gameOpts->render.lightnings.bDamage = 1;
	gameOpts->render.lightnings.bExplosions = 1;
	gameOpts->render.lightnings.bStatic = 1;
	gameOpts->render.lightnings.bOmega = 1;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->ogl.nMaxLightsPerFace = 16;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 8;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cameras.nFPS = 0;
	gameOpts->movies.nQuality = 1;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 1;
	gameOpts->render.shadows.nReach = 1;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
else if (gameStates.app.nCompSpeed == 4) {
	gameOpts->render.nQuality = 4;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 1;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 1;
	gameOpts->render.effects.nShrapnels = 4;
	gameOpts->render.particles.bStatic = 1;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 3;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 1;
	gameOpts->render.coronas.nStyle = 2;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.particles.bAuxViews = 1;
	gameOpts->render.particles.bMonitors = 1;
	gameOpts->render.lightnings.bMonitors = 1;
	gameOpts->render.lightnings.nQuality = 1;
	gameOpts->render.lightnings.nStyle = 2;
	gameOpts->render.lightnings.bPlasma = 1;
	gameOpts->render.lightnings.bPlayers = 1;
	gameOpts->render.lightnings.bRobots = 1;
	gameOpts->render.lightnings.bDamage = 1;
	gameOpts->render.lightnings.bExplosions = 1;
	gameOpts->render.lightnings.bStatic = 1;
	gameOpts->render.lightnings.bOmega = 1;
	gameOpts->render.lightnings.bAuxViews = 1;
	gameOpts->ogl.bLightObjects = 1;
	gameOpts->ogl.nMaxLightsPerFace = 20;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 16;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.cameras.nFPS = 0;
	gameOpts->movies.nQuality = 1;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 1;
	gameOpts->render.shadows.nReach = 1;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
}

// -----------------------------------------------------------------------------

static const char *pszCompSpeeds [5];

int PerformanceSettingsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem * m;
	int			v;

m = menus + performanceOpts.nUseCompSpeed;
v = m->value;
if (gameStates.app.bUseDefaults != v) {
	gameStates.app.bUseDefaults = v;
	*key = -2;
	return nCurItem;
	}
if (gameStates.app.bUseDefaults) {
	m = menus + performanceOpts.nCompSpeed;
	v = m->value;
	if (gameStates.app.nCompSpeed != v) {
		gameStates.app.nCompSpeed = v;
		sprintf (m->text, TXT_COMP_SPEED, pszCompSpeeds [v]);
		}
	m->rebuild = 1;
	}
return nCurItem;
}

// -----------------------------------------------------------------------------

void PerformanceSettingsMenu (void)
{
	int		i, nOptions, choice = gameStates.app.nDetailLevel;
	char		szCompSpeed [50];
	CMenuItem m [10];

pszCompSpeeds [0] = TXT_VERY_SLOW;
pszCompSpeeds [1] = TXT_SLOW;
pszCompSpeeds [2] = TXT_MEDIUM;
pszCompSpeeds [3] = TXT_FAST;
pszCompSpeeds [4] = TXT_VERY_FAST;

do {
	i = nOptions = 0;
	memset (m, 0, sizeof (m));
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddCheck (nOptions, TXT_USE_DEFAULTS, gameStates.app.bUseDefaults, KEY_U, HTX_MISC_DEFAULTS);
	performanceOpts.nUseCompSpeed = nOptions++;
	if (gameStates.app.bUseDefaults) {
		sprintf (szCompSpeed + 1, TXT_COMP_SPEED, pszCompSpeeds [gameStates.app.nCompSpeed]);
		*szCompSpeed = *(TXT_COMP_SPEED - 1);
		m.AddSlider (nOptions, szCompSpeed + 1, gameStates.app.nCompSpeed, 0, 4, KEY_C, HTX_MISC_COMPSPEED);
		performanceOpts.nCompSpeed = nOptions++;
		}
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_SETPERF_MENUTITLE, nOptions, m, PerformanceSettingsCallback, &choice);
		} while (i >= 0);
	} while (i == -2);
UseDefaultPerformanceSettings ();
}

//------------------------------------------------------------------------------
//eof
