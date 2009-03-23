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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(__unix__) || defined(__macosx__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __macosx__
#	include "SDL/SDL_main.h"
#	include "SDL/SDL_keyboard.h"
#	include "FolderDetector.h"
#else
#	include "SDL_main.h"
#	include "SDL_keyboard.h"
#endif
#include "descent.h"
#include "u_mem.h"
#include "strutil.h"
#include "key.h"
#include "timer.h"
#include "error.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "iff.h"
#include "pcx.h"
#include "args.h"
#include "hogfile.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "sdlgl.h"
#include "text.h"
#include "newdemo.h"
#include "objrender.h"
#include "renderthreads.h"
#include "network.h"
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "joy.h"
#include "desc_id.h"
#include "joydefs.h"
#include "gamepal.h"
#include "movie.h"
#include "compbit.h"
#include "playsave.h"
#include "tracker.h"
#include "rendermine.h"
#include "sphere.h"
#include "endlevel.h"
#include "interp.h"
#include "autodl.h"
#include "hiresmodels.h"
#include "soundthreads.h"
#include "gameargs.h"

//------------------------------------------------------------------------------

void InitRenderOptions (int i)
{
if (i) {
	extraGameInfo [0].bPowerupsOnRadar = 0;
	extraGameInfo [0].bRobotsOnRadar = 0;
	extraGameInfo [0].bUseCameras = 0;
	if (gameStates.app.bNostalgia > 2)
		extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].bWiggle = 1;
	extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].bDamageExplosions = 0;
	extraGameInfo [0].bThrusterFlames = 0;
	extraGameInfo [0].bShadows = 0;
	extraGameInfo [0].bShowWeapons = 1;
	gameOptions [0].render.nPath = 0;
	gameOptions [1].render.shadows.bPlayers = 0;
	gameOptions [1].render.shadows.bRobots = 0;
	gameOptions [1].render.shadows.bMissiles = 0;
	gameOptions [1].render.shadows.bPowerups = 0;
	gameOptions [1].render.shadows.bReactors = 0;
	gameOptions [1].render.shadows.bFast = 1;
	gameOptions [1].render.shadows.nClip = 1;
	gameOptions [1].render.shadows.nReach = 1;
	gameOptions [1].render.ship.nWingtip = 1;
	gameOptions [1].render.ship.bBullets = 1;
	gameOptions [1].render.nMaxFPS = 150;
	gameOptions [1].render.bDepthSort = 0;
	gameOptions [1].render.effects.bTransparent = 0;
	gameOptions [1].render.debug.bDynamicLight = 1;
	gameOptions [1].render.textures.bUseHires [0] =
	gameOptions [1].render.textures.bUseHires [1] = 0;
	if (gameStates.app.bNostalgia > 2)
		gameOptions [1].render.nImageQuality = 0;
	gameOptions [1].render.coronas.bUse = 0;
	gameOptions [1].render.coronas.nStyle = 1;
	gameOptions [1].render.coronas.bShots = 0;
	gameOptions [1].render.coronas.bPowerups = 0;
	gameOptions [1].render.coronas.bWeapons = 0;
	gameOptions [1].render.coronas.bAdditive = 0;
	gameOptions [1].render.coronas.bAdditiveObjs = 0;
	gameOptions [1].render.effects.bRobotShields = 0;
	gameOptions [1].render.effects.bOnlyShieldHits = 0;
	gameOptions [1].render.coronas.nIntensity = 2;
	gameOptions [1].render.coronas.nObjIntensity = 1;
	gameOptions [1].render.effects.bExplBlasts = 1;
	gameOptions [1].render.effects.nShrapnels = 1;
	gameOptions [1].render.particles.bAuxViews = 0;
	gameOptions [1].render.lightnings.bAuxViews = 0;
	gameOptions [1].render.debug.bWireFrame = 0;
	gameOptions [1].render.debug.bTextures = 1;
	gameOptions [1].render.debug.bObjects = 1;
	gameOptions [1].render.debug.bWalls = 1;
	gameOptions [1].render.bUseShaders = 1;
	gameOptions [1].render.bHiresModels [0] =
	gameOptions [1].render.bHiresModels [1] = 0;
	gameOptions [1].render.bUseLightmaps = 0;
	gameOptions [1].render.effects.bAutoTransparency = 0;
	gameOptions [1].render.nMeshQuality = 0;
	gameOptions [1].render.nMathFormat = 2;
	gameOptions [1].render.nDefMathFormat = 2;
	gameOptions [1].render.nDebrisLife = 0;
	gameOptions [1].render.shadows.nLights = 0;
	gameOptions [1].render.cameras.bFitToWall = 0;
	gameOptions [1].render.cameras.nFPS = 0;
	gameOptions [1].render.cameras.nSpeed = 0;
	gameOptions [1].render.cockpit.bHUD = 1;
	gameOptions [1].render.cockpit.bHUDMsgs = 1;
	gameOptions [1].render.cockpit.bSplitHUDMsgs = 0;
	gameOptions [1].render.cockpit.bMouseIndicator = 1;
	gameOptions [1].render.cockpit.bTextGauges = 1;
	gameOptions [1].render.cockpit.bObjectTally = 0;
	gameOptions [1].render.cockpit.bScaleGauges = 1;
	gameOptions [1].render.cockpit.bFlashGauges = 1;
	gameOptions [1].render.cockpit.bReticle = 1;
	gameOptions [1].render.cockpit.bRotateMslLockInd = 0;
	gameOptions [1].render.cockpit.nWindowSize = 0;
	gameOptions [1].render.cockpit.nWindowZoom = 0;
	gameOptions [1].render.cockpit.nWindowPos = 1;
	gameOptions [1].render.color.bAmbientLight = 0;
	gameOptions [1].render.color.bCap = 0;
	gameOptions [1].render.color.nSaturation = 0;
	gameOptions [1].render.color.bGunLight = 0;
	gameOptions [1].render.color.bMix = 1;
	gameOptions [1].render.color.bUseLightmaps = 0;
	gameOptions [1].render.color.bWalls = 0;
	gameOptions [1].render.color.nLightmapRange = 5;
	gameOptions [1].render.weaponIcons.bSmall = 0;
	gameOptions [1].render.weaponIcons.bShowAmmo = 0;
	gameOptions [1].render.weaponIcons.bEquipment = 0;
	gameOptions [1].render.weaponIcons.nSort = 0;
	gameOptions [1].render.textures.bUseHires [0] =
	gameOptions [1].render.textures.bUseHires [1] = 0;
	gameOptions [1].render.textures.nQuality = 0;
	gameOptions [1].render.cockpit.bMissileView = 1;
	gameOptions [1].render.cockpit.bGuidedInMainView = 1;
	gameOptions [1].render.particles.nDens [0] =
	gameOptions [1].render.particles.nDens [1] =
	gameOptions [1].render.particles.nDens [2] =
	gameOptions [1].render.particles.nDens [3] =
	gameOptions [1].render.particles.nDens [4] = 0;
	gameOptions [1].render.particles.nSize [0] =
	gameOptions [1].render.particles.nSize [1] =
	gameOptions [1].render.particles.nSize [2] =
	gameOptions [1].render.particles.nSize [3] =
	gameOptions [1].render.particles.nSize [4] = 0;
	gameOptions [1].render.particles.nLife [0] =
	gameOptions [1].render.particles.nLife [1] =
	gameOptions [1].render.particles.nLife [2] = 0;
	gameOptions [1].render.particles.nLife [3] = 1;
	gameOptions [1].render.particles.nLife [4] = 0;
	gameOptions [1].render.particles.nAlpha [0] =
	gameOptions [1].render.particles.nAlpha [1] =
	gameOptions [1].render.particles.nAlpha [2] =
	gameOptions [1].render.particles.nAlpha [3] =
	gameOptions [1].render.particles.nAlpha [4] = 0;
	gameOptions [1].render.particles.bPlayers = 0;
	gameOptions [1].render.particles.bRobots = 0;
	gameOptions [1].render.particles.bMissiles = 0;
	gameOptions [1].render.particles.bDebris = 0;
	gameOptions [1].render.particles.bStatic = 0;
	gameOptions [1].render.particles.bBubbles = 0;
	gameOptions [1].render.particles.bWobbleBubbles = 1;
	gameOptions [1].render.particles.bWiggleBubbles = 1;
	gameOptions [1].render.particles.bCollisions = 0;
	gameOptions [1].render.particles.bDisperse = 0;
	gameOptions [1].render.particles.bRotate = 0;
	gameOptions [1].render.particles.bSort = 0;
	gameOptions [1].render.particles.bDecreaseLag = 0;
	gameOptions [1].render.lightnings.bOmega = 0;
	gameOptions [1].render.lightnings.bDamage = 0;
	gameOptions [1].render.lightnings.bExplosions = 0;
	gameOptions [1].render.lightnings.bPlayers = 0;
	gameOptions [1].render.lightnings.bRobots = 0;
	gameOptions [1].render.lightnings.bStatic = 0;
	gameOptions [1].render.lightnings.bPlasma = 0;
	gameOptions [1].render.lightnings.nQuality = 0;
	gameOptions [1].render.lightnings.nStyle = 0;
	gameOptions [1].render.powerups.b3D = 0;
	gameOptions [1].render.powerups.nSpin = 0;
	gameOptions [1].render.automap.bTextured = 0;
	gameOptions [1].render.automap.bParticles = 0;
	gameOptions [1].render.automap.bLightnings = 0;
	gameOptions [1].render.automap.bSkybox = 0;
	gameOptions [1].render.automap.bBright = 1;
	gameOptions [1].render.automap.bCoronas = 0;
	gameOptions [1].render.automap.nColor = 0;
	gameOptions [1].render.automap.nRange = 2;
	}
else {
	extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].bShadows = 0;
	gameOptions [0].render.nPath = 1;
	gameOptions [0].render.shadows.bPlayers = 1;
	gameOptions [0].render.shadows.bRobots = 0;
	gameOptions [0].render.shadows.bMissiles = 0;
	gameOptions [0].render.shadows.bPowerups = 0;
	gameOptions [0].render.shadows.bReactors = 0;
	gameOptions [0].render.shadows.bFast = 1;
	gameOptions [0].render.shadows.nClip = 1;
	gameOptions [0].render.shadows.nReach = 1;
	gameOptions [0].render.nMaxFPS = 150;
	gameOptions [1].render.bDepthSort = 1;
	gameOptions [0].render.effects.bTransparent = 1;
	gameOptions [0].render.debug.bDynamicLight = 1;
	gameOptions [0].render.nImageQuality = 3;
	gameOptions [0].render.debug.bWireFrame = 0;
	gameOptions [0].render.debug.bTextures = 1;
	gameOptions [0].render.debug.bObjects = 1;
	gameOptions [0].render.debug.bWalls = 1;
	gameOptions [0].render.bUseShaders = 1;
	gameOptions [0].render.bHiresModels [0] =
	gameOptions [0].render.bHiresModels [1] = 1;
	gameOptions [0].render.bUseLightmaps = 0;
	gameOptions [0].render.effects.bAutoTransparency = 1;
	gameOptions [0].render.nMathFormat = 2;
	gameOptions [0].render.nDefMathFormat = 2;
	gameOptions [0].render.nDebrisLife = 0;
	gameOptions [0].render.particles.bAuxViews = 0;
	gameOptions [0].render.lightnings.bAuxViews = 0;
	gameOptions [0].render.coronas.bUse = 0;
	gameOptions [0].render.coronas.nStyle = 1;
	gameOptions [0].render.coronas.bShots = 0;
	gameOptions [0].render.coronas.bPowerups = 0;
	gameOptions [0].render.coronas.bWeapons = 0;
	gameOptions [0].render.coronas.bAdditive = 0;
	gameOptions [0].render.coronas.bAdditiveObjs = 0;
	gameOptions [0].render.coronas.nIntensity = 2;
	gameOptions [0].render.coronas.nObjIntensity = 1;
	gameOptions [0].render.effects.bRobotShields = 0;
	gameOptions [0].render.effects.bOnlyShieldHits = 0;
	gameOptions [0].render.effects.bExplBlasts = 1;
	gameOptions [0].render.effects.nShrapnels = 1;
#if DBG
	gameOptions [0].render.shadows.nLights = 1;
#else
	gameOptions [0].render.shadows.nLights = 3;
#endif
	gameOptions [0].render.cameras.bFitToWall = 0;
	gameOptions [0].render.cameras.nFPS = 0;
	gameOptions [0].render.cameras.nSpeed = 5000;
	gameOptions [0].render.cockpit.bHUD = 1;
	gameOptions [0].render.cockpit.bHUDMsgs = 1;
	gameOptions [0].render.cockpit.bSplitHUDMsgs = 0;
	gameOptions [0].render.cockpit.bMouseIndicator = 0;
	gameOptions [0].render.cockpit.bTextGauges = 1;
	gameOptions [0].render.cockpit.bObjectTally = 1;
	gameOptions [0].render.cockpit.bScaleGauges = 1;
	gameOptions [0].render.cockpit.bFlashGauges = 1;
	gameOptions [0].render.cockpit.bRotateMslLockInd = 0;
	gameOptions [0].render.cockpit.bReticle = 1;
	gameOptions [0].render.cockpit.nWindowSize = 0;
	gameOptions [0].render.cockpit.nWindowZoom = 0;
	gameOptions [0].render.cockpit.nWindowPos = 1;
	gameOptions [0].render.color.bAmbientLight = 0;
	gameOptions [0].render.color.bGunLight = 0;
	gameOptions [0].render.color.bMix = 1;
	gameOptions [0].render.color.nSaturation = 0;
	gameOptions [0].render.color.bUseLightmaps = 0;
	gameOptions [0].render.color.bWalls = 0;
	gameOptions [0].render.color.nLightmapRange = 5;
	gameOptions [0].render.weaponIcons.bSmall = 0;
	gameOptions [0].render.weaponIcons.bShowAmmo = 1;
	gameOptions [0].render.weaponIcons.bEquipment = 1;
	gameOptions [0].render.weaponIcons.nSort = 1;
	gameOptions [0].render.weaponIcons.alpha = 4;
	gameOptions [0].render.textures.bUseHires [0] = 
	gameOptions [1].render.textures.bUseHires [1] = 0;
	gameOptions [0].render.textures.nQuality = 3;
	gameOptions [0].render.nMeshQuality = 0;
	gameOptions [0].render.cockpit.bMissileView = 1;
	gameOptions [0].render.cockpit.bGuidedInMainView = 1;
	gameOptions [0].render.particles.nDens [0] =
	gameOptions [0].render.particles.nDens [0] =
	gameOptions [0].render.particles.nDens [2] =
	gameOptions [0].render.particles.nDens [3] =
	gameOptions [0].render.particles.nDens [4] = 2;
	gameOptions [0].render.particles.nSize [0] =
	gameOptions [0].render.particles.nSize [0] =
	gameOptions [0].render.particles.nSize [2] =
	gameOptions [0].render.particles.nSize [3] =
	gameOptions [0].render.particles.nSize [4] = 1;
	gameOptions [0].render.particles.nLife [0] =
	gameOptions [0].render.particles.nLife [0] =
	gameOptions [0].render.particles.nLife [2] = 0;
	gameOptions [0].render.particles.nLife [3] = 1;
	gameOptions [0].render.particles.nLife [4] = 0;
	gameOptions [0].render.particles.nAlpha [0] =
	gameOptions [0].render.particles.nAlpha [0] =
	gameOptions [0].render.particles.nAlpha [2] =
	gameOptions [0].render.particles.nAlpha [3] =
	gameOptions [0].render.particles.nAlpha [4] = 0;
	gameOptions [0].render.particles.bPlayers = 1;
	gameOptions [0].render.particles.bRobots = 1;
	gameOptions [0].render.particles.bMissiles = 1;
	gameOptions [0].render.particles.bDebris = 1;
	gameOptions [0].render.particles.bStatic = 1;
	gameOptions [0].render.particles.bBubbles = 1;
	gameOptions [0].render.particles.bWobbleBubbles = 1;
	gameOptions [0].render.particles.bWiggleBubbles = 1;
	gameOptions [0].render.particles.bCollisions = 0;
	gameOptions [0].render.particles.bDisperse = 1;
	gameOptions [0].render.particles.bRotate = 1;
	gameOptions [0].render.particles.bSort = 1;
	gameOptions [0].render.particles.bDecreaseLag = 1;
	gameOptions [0].render.lightnings.bOmega = 1;
	gameOptions [0].render.lightnings.bDamage = 1;
	gameOptions [0].render.lightnings.bExplosions = 1;
	gameOptions [0].render.lightnings.bPlayers = 1;
	gameOptions [0].render.lightnings.bRobots = 1;
	gameOptions [0].render.lightnings.bStatic = 1;
	gameOptions [0].render.lightnings.bPlasma = 1;
	gameOptions [0].render.lightnings.nQuality = 0;
	gameOptions [0].render.lightnings.nStyle = 1;
	gameOptions [0].render.powerups.b3D = 0;
	gameOptions [0].render.powerups.nSpin = 0;
	gameOptions [0].render.automap.bTextured = 0;
	gameOptions [0].render.automap.bParticles = 0;
	gameOptions [0].render.automap.bLightnings = 0;
	gameOptions [0].render.automap.bSkybox = 0;
	gameOptions [0].render.automap.bBright = 1;
	gameOptions [0].render.automap.bCoronas = 0;
	gameOptions [0].render.automap.nColor = 0;
	gameOptions [0].render.automap.nRange = 2;
	}
}

//------------------------------------------------------------------------------

void InitGameplayOptions (int i)
{
if (i) {
	extraGameInfo [0].nSpawnDelay = 0;
	extraGameInfo [0].nFusionRamp = 2;
	extraGameInfo [0].bFixedRespawns = 0;
	extraGameInfo [0].bRobotsHitRobots = 0;
	extraGameInfo [0].bDualMissileLaunch = 0;
	extraGameInfo [0].bDropAllMissiles = 0;
	extraGameInfo [0].bImmortalPowerups = 0;
	extraGameInfo [0].bMultiBosses = 0;
	extraGameInfo [0].bSmartWeaponSwitch = 0;
	extraGameInfo [0].bFluidPhysics = 0;
	extraGameInfo [0].nWeaponDropMode = 0;
	extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].nZoomMode = 0;
	extraGameInfo [0].nHitboxes = 0;
	extraGameInfo [0].bTripleFusion = 0;
	extraGameInfo [0].bKillMissiles = 0;
	gameOptions [1].gameplay.nAutoSelectWeapon = 2;
	gameOptions [1].gameplay.bSecretSave = 0;
	gameOptions [1].gameplay.bTurboMode = 0;
	gameOptions [1].gameplay.bFastRespawn = 0;
	gameOptions [1].gameplay.nAutoLeveling = 1;
	gameOptions [1].gameplay.bEscortHotKeys = 1;
	gameOptions [1].gameplay.bSkipBriefingScreens = 0;
	gameOptions [1].gameplay.bHeadlightOnWhenPickedUp = 1;
	gameOptions [1].gameplay.bShieldWarning = 0;
	gameOptions [1].gameplay.bInventory = 0;
	gameOptions [1].gameplay.bIdleAnims = 0;
	gameOptions [1].gameplay.nAIAwareness = 0;
	gameOptions [1].gameplay.nAIAggressivity = 0;
	}
else {
	gameOptions [0].gameplay.nAutoSelectWeapon = 2;
	gameOptions [0].gameplay.bSecretSave = 0;
	gameOptions [0].gameplay.bTurboMode = 0;
	gameOptions [0].gameplay.bFastRespawn = 0;
	gameOptions [0].gameplay.nAutoLeveling = 1;
	gameOptions [0].gameplay.bEscortHotKeys = 1;
	gameOptions [0].gameplay.bSkipBriefingScreens = 0;
	gameOptions [0].gameplay.bHeadlightOnWhenPickedUp = 0;
	gameOptions [0].gameplay.bShieldWarning = 0;
	gameOptions [0].gameplay.bInventory = 0;
	gameOptions [0].gameplay.bIdleAnims = 0;
	gameOptions [0].gameplay.nAIAwareness = 0;
	gameOptions [0].gameplay.nAIAggressivity = 0;
	}
}

//------------------------------------------------------------------------------

void InitMovieOptions (int i)
{
if (i) {
	gameOptions [1].movies.bHires = 1;
	gameOptions [1].movies.nQuality = 0;
	gameOptions [1].movies.nLevel = 2;
	gameOptions [1].movies.bResize = 0;
	gameOptions [1].movies.bFullScreen = 0;
	gameOptions [1].movies.bSubTitles = 1;
	}
else {
	gameOptions [0].movies.bHires = 1;
	gameOptions [0].movies.nQuality = 0;
	gameOptions [0].movies.nLevel = 2;
	gameOptions [0].movies.bResize = 1;
	gameOptions [1].movies.bFullScreen = 1;
	gameOptions [0].movies.bSubTitles = 1;
	}
}

//------------------------------------------------------------------------------

void InitLegacyOptions (int i)
{
if (i) {
	gameOptions [1].legacy.bInput = 0;
	gameOptions [1].legacy.bFuelCens = 0;
	gameOptions [1].legacy.bMouse = 0;
	gameOptions [1].legacy.bHomers = 0;
	gameOptions [1].legacy.bRender = 0;
	gameOptions [1].legacy.bSwitches = 0;
	gameOptions [1].legacy.bWalls = 0;
	}
else {
	gameOptions [0].legacy.bInput = 0;
	gameOptions [0].legacy.bFuelCens = 0;
	gameOptions [0].legacy.bMouse = 0;
	gameOptions [0].legacy.bHomers = 0;
	gameOptions [0].legacy.bRender = 0;
	gameOptions [0].legacy.bSwitches = 0;
	gameOptions [0].legacy.bWalls = 0;
	}
}

//------------------------------------------------------------------------------

void InitSoundOptions (int i)
{
if (i) {
	gameOptions [1].sound.bUseRedbook = 1;
	gameOptions [1].sound.digiSampleRate = SAMPLE_RATE_22K;
#if USE_SDL_MIXER
	gameOptions [1].sound.bUseSDLMixer = 1;
#else
	gameOptions [1].sound.bUseSDLMixer = 0;
#endif
	}
else {
	gameOptions [0].sound.bUseRedbook = 1;
	gameOptions [1].sound.digiSampleRate = SAMPLE_RATE_22K;
#if USE_SDL_MIXER
	gameOptions [0].sound.bUseSDLMixer = 1;
#else
	gameOptions [0].sound.bUseSDLMixer = 0;
#endif
	}
gameOptions [i].sound.bHires [0] =
gameOptions [i].sound.bHires [1] = 0;
gameOptions [i].sound.bShip = 0;
gameOptions [i].sound.bMissiles = 0;
gameOptions [i].sound.bFadeMusic = 1;
gameOptions [1].sound.bUseD1Sounds = 1;
}

//------------------------------------------------------------------------------

void InitInputOptions (int i)
{
if (i) {
	extraGameInfo [0].bMouseLook = 0;
	gameOptions [1].input.bLimitTurnRate = 1;
	gameOptions [1].input.nMinTurnRate = 20;	//turn time for a 360 deg rotation around a single ship axis in 1/10 sec units
	if (gameOptions [1].input.joystick.bUse)
		gameOptions [1].input.mouse.bUse = 0;
	gameOptions [1].input.mouse.bSyncAxes = 1;
	gameOptions [1].input.mouse.bJoystick = 0;
	gameOptions [1].input.mouse.nDeadzone = 0;
	gameOptions [1].input.joystick.bSyncAxes = 1;
	gameOptions [1].input.keyboard.bUse = 1;
	gameOptions [1].input.bUseHotKeys = 1;
	gameOptions [1].input.keyboard.nRamp = 100;
	gameOptions [1].input.keyboard.bRamp [0] =
	gameOptions [1].input.keyboard.bRamp [1] =
	gameOptions [1].input.keyboard.bRamp [2] = 0;
	gameOptions [1].input.joystick.bLinearSens = 1;
	gameOptions [1].input.mouse.sensitivity [0] =
	gameOptions [1].input.mouse.sensitivity [1] =
	gameOptions [1].input.mouse.sensitivity [2] = 8;
	gameOptions [1].input.joystick.sensitivity [0] =
	gameOptions [1].input.joystick.sensitivity [1] =
	gameOptions [1].input.joystick.sensitivity [2] =
	gameOptions [1].input.joystick.sensitivity [3] = 8;
	gameOptions [1].input.joystick.deadzones [0] =
	gameOptions [1].input.joystick.deadzones [1] =
	gameOptions [1].input.joystick.deadzones [2] =
	gameOptions [1].input.joystick.deadzones [3] = 10;
	}
else {
	gameOptions [0].input.bLimitTurnRate = 1;
	gameOptions [0].input.nMinTurnRate = 20;	//turn time for a 360 deg rotation around a single ship axis in 1/10 sec units
	gameOptions [0].input.joystick.bLinearSens = 0;
	gameOptions [0].input.keyboard.nRamp = 100;
	gameOptions [0].input.keyboard.bRamp [0] =
	gameOptions [0].input.keyboard.bRamp [1] =
	gameOptions [0].input.keyboard.bRamp [2] = 0;
	gameOptions [0].input.mouse.bUse = 1;
	gameOptions [0].input.joystick.bUse = 0;
	gameOptions [0].input.mouse.bSyncAxes = 1;
	gameOptions [0].input.mouse.nDeadzone = 0;
	gameOptions [0].input.mouse.bJoystick = 0;
	gameOptions [0].input.joystick.bSyncAxes = 1;
	gameOptions [0].input.keyboard.bUse = 1;
	gameOptions [0].input.bUseHotKeys = 1;
	gameOptions [0].input.mouse.nDeadzone = 2;
	gameOptions [0].input.mouse.sensitivity [0] =
	gameOptions [0].input.mouse.sensitivity [1] =
	gameOptions [0].input.mouse.sensitivity [2] = 8;
	gameOptions [0].input.trackIR.nDeadzone = 0;
	gameOptions [0].input.trackIR.bMove [0] =
	gameOptions [0].input.trackIR.bMove [1] = 1;
	gameOptions [0].input.trackIR.bMove [2] = 0;
	gameOptions [0].input.trackIR.sensitivity [0] =
	gameOptions [0].input.trackIR.sensitivity [1] =
	gameOptions [0].input.trackIR.sensitivity [2] = 8;
	gameOptions [0].input.joystick.sensitivity [0] =
	gameOptions [0].input.joystick.sensitivity [1] =
	gameOptions [0].input.joystick.sensitivity [2] =
	gameOptions [0].input.joystick.sensitivity [3] = 8;
	gameOptions [0].input.joystick.deadzones [0] =
	gameOptions [0].input.joystick.deadzones [1] =
	gameOptions [0].input.joystick.deadzones [2] =
	gameOptions [0].input.joystick.deadzones [3] = 10;
	}
}

// ----------------------------------------------------------------------------

void InitMultiplayerOptions (int i)
{
if (i) {
	extraGameInfo [0].bFriendlyFire = 1;
	extraGameInfo [0].bInhibitSuicide = 0;
	extraGameInfo [1].bMouseLook = 0;
	extraGameInfo [1].bDualMissileLaunch = 0;
	extraGameInfo [0].bAutoBalanceTeams = 0;
	extraGameInfo [1].bRotateLevels = 0;
	extraGameInfo [1].bDisableReactor = 0;
	gameOptions [1].multi.bNoRankings = 0;
	gameOptions [1].multi.bTimeoutPlayers = 1;
	gameOptions [1].multi.bUseMacros = 0;
	gameOptions [1].multi.bNoRedundancy = 1;
	}
else {
	gameOptions [0].multi.bNoRankings = 0;
	gameOptions [0].multi.bTimeoutPlayers = 1;
	gameOptions [0].multi.bUseMacros = 0;
	gameOptions [0].multi.bNoRedundancy = 1;
	}
}

// ----------------------------------------------------------------------------

void InitDemoOptions (int i)
{
if (i)
	gameOptions [i].demo.bOldFormat = 0;
else
	gameOptions [i].demo.bOldFormat = 1;
gameOptions [i].demo.bRevertFormat = 0;
}

// ----------------------------------------------------------------------------

void InitMenuOptions (int i)
{
if (i) {
	gameOptions [1].menus.nStyle = 0;
	gameOptions [1].menus.nFade = 0;
	gameOptions [1].menus.bFastMenus = 1;
	gameOptions [1].menus.bSmartFileSearch = 0;
	gameOptions [1].menus.bShowLevelVersion = 0;
	gameOptions [1].menus.altBg.alpha = 0;
	gameOptions [1].menus.altBg.brightness = 0;
	gameOptions [1].menus.altBg.grayscale = 0;
	gameOptions [1].menus.nHotKeys = 0;
	strcpy (gameOptions [0].menus.altBg.szName, "");
	}
else {
	gameOptions [0].menus.nStyle = 0;
	gameOptions [1].menus.nFade = 150;
	gameOptions [0].menus.bFastMenus = 1;
	gameOptions [0].menus.bSmartFileSearch = 1;
	gameOptions [0].menus.bShowLevelVersion = 0;
	gameOptions [0].menus.altBg.alpha = 0.75;
	gameOptions [0].menus.altBg.brightness = 0.5;
	gameOptions [0].menus.altBg.grayscale = 0;
	gameOptions [0].menus.nHotKeys = gameStates.app.bEnglish ? 1 : -1;
	strcpy (gameOptions [0].menus.altBg.szName, "menubg.tga");
	}
}

// ----------------------------------------------------------------------------

void InitOglOptions (int i)
{
if (i) {
	gameOptions [1].render.nLightingMethod = 0;
	gameOptions [1].ogl.bLightObjects = 0;
	gameOptions [1].ogl.bHeadlight = 0;
	gameOptions [1].ogl.bLightPowerups = 0;
	gameOptions [1].ogl.bObjLighting = 0;
	gameOptions [1].ogl.bSetGammaRamp = 0;
	gameOptions [1].ogl.bVoodooHack = 0;
	gameOptions [1].ogl.bGlTexMerge = 1;
	}
else {
#if DBG
	gameOptions [0].render.nLightingMethod = 0;
#else
	gameOptions [0].render.nLightingMethod = 0;
#endif
	gameOptions [0].ogl.bLightObjects = 0;
	gameOptions [0].ogl.bHeadlight = 0;
	gameOptions [0].ogl.bLightPowerups = 0;
	gameOptions [0].ogl.bObjLighting = 0;
	gameOptions [0].ogl.bSetGammaRamp = 0;
	gameOptions [0].ogl.bVoodooHack = 0;
	gameOptions [0].ogl.bGlTexMerge = 1;
	}
}

// ----------------------------------------------------------------------------

void InitAppOptions (int i)
{
if (i) {
	gameOptions [1].app.bEnableMods = 0;
	gameOptions [1].app.nVersionFilter = 2;
	gameOptions [1].app.bSinglePlayer = 0;
	gameOptions [1].app.bExpertMode = 0;
	gameOptions [1].app.nScreenShotInterval = 0;
	}
else {
	gameOptions [1].app.bEnableMods = 1;
	gameOptions [0].app.nVersionFilter = 2;
	gameOptions [0].app.bSinglePlayer = 0;
	gameOptions [0].app.bExpertMode = 1;
	gameOptions [0].app.nScreenShotInterval = 0;
	}
}

// ----------------------------------------------------------------------------

void InitGameOptions (int i)
{
if (i)
	if (gameStates.app.bNostalgia)
		gameOptions [1] = gameOptions [0];
	else
		return;
else
	memset (gameOptions, 0, sizeof (gameOptions));
InitInputOptions (i);
InitGameplayOptions (i);
InitRenderOptions (i);
InitMultiplayerOptions (i);
InitMenuOptions (i);
InitDemoOptions (i);
InitSoundOptions (i);
InitMovieOptions (i);
InitOglOptions (i);
InitLegacyOptions (i);
InitGameplayOptions (i);
}

// ----------------------------------------------------------------------------
