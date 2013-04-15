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
#include "playerprofile.h"
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

void CRenderOptions::Init (int i)
{
nLightingMethod = 0;

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

	nPath = 0;
	shadows.bPlayers = 0;
	shadows.bRobots = 0;
	shadows.bMissiles = 0;
	shadows.bPowerups = 0;
	shadows.bReactors = 0;
	shadows.bFast = 1;
	shadows.nClip = 1;
	shadows.nReach = 1;
	ship.nWingtip = 1;
	ship.bBullets = 1;
	nMaxFPS = 60;
	effects.bTransparent = 0;
	debug.bDynamicLight = 1;
	textures.bUseHires [0] =
	textures.bUseHires [1] = 0;
	if (gameStates.app.bNostalgia > 2)
		nImageQuality = 0;
	coronas.bUse = 0;
	coronas.nStyle = 1;
	coronas.bShots = 0;
	coronas.bPowerups = 0;
	coronas.bWeapons = 0;
	coronas.bAdditive = 0;
	coronas.bAdditiveObjs = 0;
	effects.bRobotShields = 0;
	effects.bOnlyShieldHits = 0;
	coronas.nIntensity = 1;
	coronas.nObjIntensity = 1;
	effects.nShockwaves = 1;
	effects.nShrapnels = 1;
	particles.bAuxViews = 0;
	lightning.bAuxViews = 0;
	debug.bWireFrame = 0;
	debug.bTextures = 1;
	debug.bObjects = 1;
	debug.bWalls = 1;
	bUseShaders = 1;
	bHiresModels [0] =
	bHiresModels [1] = 0;
	bUseLightmaps = 0;
	effects.bAutoTransparency = 0;
	nMeshQuality = 0;
	nMathFormat = 2;
	nDefMathFormat = 2;
	nDebrisLife = 0;
	shadows.nLights = 0;
	cameras.bFitToWall = 0;
	cameras.nFPS = 0;
	cameras.nSpeed = 0;
	cockpit.bHUD = 1;
	cockpit.bHUDMsgs = 1;
	cockpit.bSplitHUDMsgs = 0;
	cockpit.bMouseIndicator = 1;
	cockpit.bTextGauges = 1;
	cockpit.bObjectTally = 0;
	cockpit.bScaleGauges = 1;
	cockpit.bFlashGauges = 1;
	cockpit.bReticle = 1;
	cockpit.bRotateMslLockInd = 0;
	cockpit.nWindowSize = 0;
	cockpit.nWindowZoom = 0;
	cockpit.nWindowPos = 1;
	color.bCap = 0;
	color.nSaturation = 0;
	color.nLevel = 0;
	color.bMix = 1;
	color.bUseLightmaps = 0;
	color.bWalls = 0;
	color.nLightmapRange = 5;
	weaponIcons.bSmall = 0;
	weaponIcons.bShowAmmo = 0;
	weaponIcons.bEquipment = 0;
	weaponIcons.nSort = 0;
	textures.bUseHires [0] =
	textures.bUseHires [1] = 0;
	textures.nQuality = 0;
	cockpit.bMissileView = 1;
	cockpit.bGuidedInMainView = 1;
	cockpit.nRadarPos = 0;
	cockpit.nRadarRange = 0;
	particles.nDens [0] =
	particles.nDens [1] =
	particles.nDens [2] =
	particles.nDens [3] =
	particles.nDens [4] = 0;
	particles.nSize [0] =
	particles.nSize [1] =
	particles.nSize [2] =
	particles.nSize [3] =
	particles.nSize [4] = 0;
	particles.nLife [0] =
	particles.nLife [1] =
	particles.nLife [2] = 0;
	particles.nLife [3] = 1;
	particles.nLife [4] = 0;
	particles.nAlpha [0] =
	particles.nAlpha [1] =
	particles.nAlpha [2] =
	particles.nAlpha [3] =
	particles.nAlpha [4] = 0;
	particles.bPlayers = 0;
	particles.bRobots = 0;
	particles.bMissiles = 0;
	particles.bDebris = 0;
	particles.bStatic = 0;
	particles.bBubbles = 0;
	particles.bWobbleBubbles = 1;
	particles.bWiggleBubbles = 1;
	particles.bCollisions = 0;
	particles.bDisperse = 0;
	particles.bRotate = 0;
	particles.bSort = 0;
	particles.bDecreaseLag = 0;
	lightning.bOmega = 0;
	lightning.bDamage = 0;
	lightning.bExplosions = 0;
	lightning.bPlayers = 0;
	lightning.bRobots = 0;
	lightning.bStatic = 0;
	lightning.bGlow = 0;
	lightning.nQuality = 0;
	lightning.nStyle = 0;
	powerups.b3D = gameStates.app.bStandalone;
	powerups.nSpin = 0;
	automap.bTextured = 0;
	automap.bParticles = 0;
	automap.bLightning = 0;
	automap.bSkybox = 0;
	automap.bBright = 1;
	automap.bCoronas = 0;
	automap.nColor = 0;
	}
else {
	extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].bShadows = 0;

	nPath = 1;
	shadows.bPlayers = 1;
	shadows.bRobots = 0;
	shadows.bMissiles = 0;
	shadows.bPowerups = 0;
	shadows.bReactors = 0;
	shadows.bFast = 1;
	shadows.nClip = 1;
	shadows.nReach = 1;
	nMaxFPS = 60;
	effects.bTransparent = 1;
	debug.bDynamicLight = 1;
	nImageQuality = 3;
	debug.bWireFrame = 0;
	debug.bTextures = 1;
	debug.bObjects = 1;
	debug.bWalls = 1;
	bUseShaders = 1;
	bHiresModels [0] =
	bHiresModels [1] = 1;
	bUseLightmaps = 0;
	effects.bAutoTransparency = 1;
	nMathFormat = 2;
	nDefMathFormat = 2;
	nDebrisLife = 0;
	particles.bAuxViews = 0;
	lightning.bAuxViews = 0;
	coronas.bUse = 0;
	coronas.nStyle = 1;
	coronas.bShots = 0;
	coronas.bPowerups = 0;
	coronas.bWeapons = 0;
	coronas.bAdditive = 0;
	coronas.bAdditiveObjs = 0;
	coronas.nIntensity = 1;
	coronas.nObjIntensity = 1;
	effects.bRobotShields = 0;
	effects.bOnlyShieldHits = 0;
	effects.nShockwaves = 1;
	effects.nShrapnels = 1;
#if DBG
	shadows.nLights = 1;
#else
	shadows.nLights = 3;
#endif
	cameras.bFitToWall = 1;
	cameras.nFPS = 0;
	cameras.nSpeed = 5000;
	cockpit.bHUD = 1;
	cockpit.bHUDMsgs = 1;
	cockpit.bSplitHUDMsgs = 0;
	cockpit.bMouseIndicator = 0;
	cockpit.bTextGauges = 1;
	cockpit.bObjectTally = 1;
	cockpit.bScaleGauges = 1;
	cockpit.bFlashGauges = 1;
	cockpit.bRotateMslLockInd = 0;
	cockpit.bReticle = 1;
	cockpit.nWindowSize = 0;
	cockpit.nWindowZoom = 0;
	cockpit.nWindowPos = 1;
	cockpit.nRadarPos = 0;
	cockpit.nRadarRange = 0;
	color.nLevel = 2;
	color.bMix = 1;
	color.nSaturation = 0;
	color.bUseLightmaps = 0;
	color.bWalls = 0;
	color.nLightmapRange = 5;
	weaponIcons.bSmall = 1;
	weaponIcons.bShowAmmo = 1;
	weaponIcons.bEquipment = 1;
	weaponIcons.nSort = 1;
	weaponIcons.alpha = 4;
	textures.bUseHires [0] = 
	gameOptions [1].render.textures.bUseHires [1] = 0;
	textures.nQuality = 3;
	nMeshQuality = 0;
	cockpit.bMissileView = 1;
	cockpit.bGuidedInMainView = 1;
	particles.nDens [0] =
	particles.nDens [1] =
	particles.nDens [2] =
	particles.nDens [3] =
	particles.nDens [4] = 2;
	particles.nSize [0] =
	particles.nSize [1] =
	particles.nSize [2] =
	particles.nSize [3] =
	particles.nSize [4] = 1;
	particles.nLife [0] =
	particles.nLife [1] =
	particles.nLife [2] = 0;
	particles.nLife [3] = 1;
	particles.nLife [4] = 0;
	particles.nAlpha [0] =
	particles.nAlpha [1] =
	particles.nAlpha [2] =
	particles.nAlpha [3] =
	particles.nAlpha [4] = 0;
	particles.bPlayers = 1;
	particles.bRobots = 1;
	particles.bMissiles = 1;
	particles.bDebris = 1;
	particles.bStatic = 1;
	particles.bBubbles = 1;
	particles.bWobbleBubbles = 1;
	particles.bWiggleBubbles = 1;
	particles.bCollisions = 0;
	particles.bDisperse = 1;
	particles.bRotate = 1;
	particles.bSort = 1;
	particles.bDecreaseLag = 1;
	lightning.bOmega = 1;
	lightning.bDamage = 1;
	lightning.bExplosions = 1;
	lightning.bPlayers = 1;
	lightning.bRobots = 1;
	lightning.bStatic = 1;
	lightning.bGlow = 1;
	lightning.nQuality = 0;
	lightning.nStyle = 1;
	powerups.b3D = gameStates.app.bStandalone;
	powerups.nSpin = 0;
	automap.bTextured = 0;
	automap.bParticles = 0;
	automap.bLightning = 0;
	automap.bSkybox = 0;
	automap.bBright = 1;
	automap.bCoronas = 0;
	automap.nColor = 0;
	}
}

//------------------------------------------------------------------------------

int CRenderOptions::ShadowQuality (void)
{
if (!SHOW_SHADOWS)
	return 0;
if ((gameOpts->render.shadows.nReach < 2) || (gameOpts->render.shadows.nClip < 2))
	return 1;
return 2;
}

//------------------------------------------------------------------------------

void CGameplayOptions::Init (int i)
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

	nAutoSelectWeapon = 2;
	bSecretSave = 0;
	bTurboMode = 0;
	bFastRespawn = 0;
	nAutoLeveling = 1;
	bEscortHotKeys = 1;
	bSkipBriefingScreens = 0;
	bHeadlightOnWhenPickedUp = 1;
	bShieldWarning = 0;
	bInventory = 0;
	bIdleAnims = 0;
	nAIAwareness = 0;
	nAIAggressivity = 0;
	nShip [0] = 0;
	nShip [1] = -1;
	}
else {
	nAutoSelectWeapon = 2;
	bSecretSave = 0;
	bTurboMode = 0;
	bFastRespawn = 0;
	nAutoLeveling = 1;
	bEscortHotKeys = 1;
	bSkipBriefingScreens = 0;
	bHeadlightOnWhenPickedUp = 0;
	bShieldWarning = 0;
	bInventory = 0;
	bIdleAnims = 0;
	nAIAwareness = 0;
	nAIAggressivity = 0;
	nShip [0] = 0;
	nShip [1] = -1;
	}
}

//------------------------------------------------------------------------------

void CMovieOptions::Init (int i)
{
bHires = 1;
nQuality = 0;
nLevel = 2;
bResize = !i;
bFullScreen = !i;
bSubTitles = 1;
}

//------------------------------------------------------------------------------

void CLegacyOptions::Init (int i)
{
bInput = 0;
bProducers = 0;
bMouse = 0;
bHomers = 0;
bRender = 0;
bSwitches = 0;
bWalls = 0;
}

//------------------------------------------------------------------------------

void CSoundOptions::Init (int i)
{
bUseRedbook = 1;
audioSampleRate = SAMPLE_RATE_22K;
#if 0
#	if USE_SDL_MIXER
	gameOptions [1].sound.bUseSDLMixer = 1;
#	else
	gameOptions [1].sound.bUseSDLMixer = 0;
#	endif
#endif
bHires [0] = gameStates.app.bStandalone;
bHires [1] = 0;
bLinkVolumes = 1;
bShip = 0;
bMissiles = 0;
bFadeMusic = 1;
bUseD1Sounds = 1;
}

//------------------------------------------------------------------------------

void CInputOptions::Init (int i)
{
if (i) {
	extraGameInfo [0].bMouseLook = 0;

	bLimitTurnRate = 1;
	nMinTurnRate = 20;	//turn time for a 360 deg rotation around a single ship axis in 1/10 sec units
	if (joystick.bUse)
		mouse.bUse = 0;
	mouse.bSyncAxis = 1;
	mouse.bJoystick = 0;
	mouse.nDeadzone = 0;
	joystick.bSyncAxis = 1;
	keyboard.bUse = 1;
	bUseHotKeys = 1;
	keyboard.nRamp = 100;
	keyboard.bRamp [0] =
	keyboard.bRamp [1] =
	keyboard.bRamp [2] = 0;
	joystick.bLinearSens = 1;
	mouse.sensitivity [0] =
	mouse.sensitivity [1] =
	mouse.sensitivity [2] = 8;
	joystick.sensitivity [0] =
	joystick.sensitivity [1] =
	joystick.sensitivity [2] =
	joystick.sensitivity [3] = 8;
	joystick.deadzones [0] =
	joystick.deadzones [1] =
	joystick.deadzones [2] =
	joystick.deadzones [3] = 1;
	}
else {
	bLimitTurnRate = 1;
	nMinTurnRate = 20;	//turn time for a 360 deg rotation around a single ship axis in 1/10 sec units
	joystick.bLinearSens = 0;
	keyboard.nRamp = 100;
	keyboard.bRamp [0] =
	keyboard.bRamp [1] =
	keyboard.bRamp [2] = 0;
	mouse.bUse = 1;
	joystick.bUse = 0;
	mouse.bSyncAxis = 1;
	mouse.nDeadzone = 0;
	mouse.bJoystick = 0;
	joystick.bSyncAxis = 1;
	keyboard.bUse = 1;
	bUseHotKeys = 1;
	mouse.nDeadzone = 2;
	mouse.sensitivity [0] =
	mouse.sensitivity [1] =
	mouse.sensitivity [2] = 8;
	trackIR.nDeadzone = 0;
	trackIR.bMove [0] =
	trackIR.bMove [1] = 1;
	trackIR.bMove [2] = 0;
	trackIR.sensitivity [0] =
	trackIR.sensitivity [1] =
	trackIR.sensitivity [2] = 8;
	joystick.sensitivity [0] =
	joystick.sensitivity [1] =
	joystick.sensitivity [2] =
	joystick.sensitivity [3] = 8;
	joystick.deadzones [0] =
	joystick.deadzones [1] =
	joystick.deadzones [2] =
	joystick.deadzones [3] = 1;
	}
}

// ----------------------------------------------------------------------------

void CMultiplayerOptions::Init (int i)
{
if (i) {
	extraGameInfo [0].bFriendlyFire = 1;
	extraGameInfo [0].bInhibitSuicide = 0;
	extraGameInfo [1].bMouseLook = 0;
	extraGameInfo [1].bDualMissileLaunch = 0;
	extraGameInfo [0].bAutoBalanceTeams = 0;
	extraGameInfo [1].bRotateLevels = 0;
	extraGameInfo [1].bDisableReactor = 0;
	}
bNoRankings = 0;
bTimeoutPlayers = 1;
bUseMacros = 0;
bNoRedundancy = 1;
}

// ----------------------------------------------------------------------------

void CDemoOptions::Init (int i)
{
bOldFormat = !i;
bRevertFormat = 0;
}

// ----------------------------------------------------------------------------

void CMenuOptions::Init (int i)
{
if (i) {
	nStyle = 0;
	nFade = 0;
	bFastMenus = 1;
	bSmartFileSearch = 0;
	bShowLevelVersion = 0;
	altBg.alpha = 0;
	altBg.brightness = 0;
	altBg.grayscale = 0;
	nHotKeys = 0;
	*altBg.szName [0] = '\0';
	}
else {
	nStyle = 0;
	nFade = 150;
	bFastMenus = 1;
	bSmartFileSearch = 1;
	bShowLevelVersion = 1;
	altBg.alpha = 0.75;
	altBg.brightness = 0.5;
	altBg.grayscale = 0;
	nHotKeys = gameStates.app.bEnglish ? 1 : -1;
	strcpy (altBg.szName [0], "menubg.tga");
	}
*altBg.szName [1] = '\0';
}

// ----------------------------------------------------------------------------

void COglOptions::Init (int i)
{
bLightObjects = 0;
bHeadlight = 0;
bLightPowerups = 0;
bObjLighting = 0;
bSetGammaRamp = 0;
bGlTexMerge = 1;
}

// ----------------------------------------------------------------------------

void CApplicationOptions::Init (int i)
{
bEnableMods = 0;
nVersionFilter = 2;
bSinglePlayer = 0;
bExpertMode = i;
nScreenShotInterval = 0;
}

// ----------------------------------------------------------------------------

void CGameOptions::Init (int i)
{
if (i) {
	if (gameStates.app.bNostalgia)
		*this = gameOptions [0];
	else
		return;
	}
else
	memset (this, 0, sizeof (*this));
input.Init (i);
gameplay.Init (i);
render.Init (i);
multi.Init (i);
menus.Init (i);
demo.Init (i);
sound.Init (i);
movies.Init (i);
ogl.Init (i);
legacy.Init (i);
gameplay.Init (i);
}

// ----------------------------------------------------------------------------

bool CGameOptions::Use3DPowerups (void)
{
return !gameStates.app.bNostalgia && missionConfig.m_b3DPowerups && (gameStates.app.bStandalone || gameOpts->render.powerups.b3D);
}

int CGameOptions::UseHiresSound (void)
{
return gameStates.app.bNostalgia ? 0 : gameStates.app.bStandalone ? 2 : gameOpts->sound.bHires [0];
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

CMissionConfig missionConfig;

void CMissionConfig::Init (void)
{
for (int i = 0; i < MAX_SHIP_TYPES; i++)
	m_shipsAllowed [i] = 1;
m_playerShip = -1;
m_bTeleport = 1;
m_bColoredSegments = 1;
m_b3DPowerups = 1;
m_bSecretSave = 1;
}

// ----------------------------------------------------------------------------

int CMissionConfig::Load (char* szFilename)
{
	CConfigManager args;
	CFile				cf;
	char				szConfig [FILENAME_LEN];
	bool				bLocal;

	static const char* szShipArgs [MAX_SHIP_TYPES] = {"-medium_ship", "-light_ship", "-heavy_ship"};

if ((bLocal = (szFilename && *szFilename)))
	CFile::ChangeFilenameExtension (szConfig + 1, szFilename, ".ini");
else
	strcpy (szConfig + 1, "global.ini");
szConfig [0] = '\x01'; // only read from mission file
if (!cf.Open (szConfig, gameFolders.szDataDir [0], "rb", 0))
	return 0;
if (args.Parse (&cf)) {
	int h = 0;
	for (int i = 0; i < MAX_SHIP_TYPES; i++) {
		if ((m_shipsAllowed [i] = args.Value (szShipArgs [i], bLocal ? m_shipsAllowed [i] : 1))) // use the global setting as default when parsing a level config
			h++;
		}
	if (!h)
		m_shipsAllowed [0] = 1; // medium ship, the standard ship
	m_playerShip = args.Value ("-player_ship", bLocal ? m_playerShip : -1);
	m_bTeleport = args.Value ("-teleport", bLocal ? m_bTeleport : 1);
	m_bSecretSave = args.Value ("-secret_save", bLocal ? m_bSecretSave : 1);
	m_bColoredSegments = args.Value ("-3d_powerups", bLocal ? m_b3DPowerups : 1);
	m_b3DPowerups = args.Value ("-colored_segments", bLocal ? m_bColoredSegments : 1);
	m_nCollisionModel = args.Value ("-collision_model", bLocal ? m_nCollisionModel : 1);
	}
cf.Close ();
return 1;
}

// ----------------------------------------------------------------------------

void CMissionConfig::Apply (void)
{
if (gameOpts->gameplay.nShip [gameOpts->gameplay.nShip [1] >= 0] == m_playerShip)
	return;

if (m_playerShip > 2)
	m_playerShip = 1;
else if (m_playerShip < -1)
	m_playerShip = -1;
if (m_playerShip == -1) {
	m_playerShip = gameOpts->gameplay.nShip [1];
	if (m_playerShip == -1)
		m_playerShip = gameOpts->gameplay.nShip [0];
	}
for (int i = 0; i < MAX_SHIP_TYPES; i++) {
	if (m_shipsAllowed [m_playerShip])
		break;
	m_playerShip = (m_playerShip + 1) % MAX_SHIP_TYPES;
	}

if (gameOpts->gameplay.nShip [gameOpts->gameplay.nShip [1] >= 0] == m_playerShip)
	return;

float fShield = (float) LOCALPLAYER.Shield (false) / (float) LOCALPLAYER.MaxShield ();
float fEnergy = (float) LOCALPLAYER.Energy (false) / (float) LOCALPLAYER.MaxEnergy ();
gameOpts->gameplay.nShip [0] =
gameOpts->gameplay.nShip [1] = m_playerShip;
LOCALPLAYER.SetEnergy (fix (fEnergy * LOCALPLAYER.MaxShield ()));
LOCALPLAYER.SetShield (fix (fShield * LOCALPLAYER.MaxEnergy ()));
if (m_playerShip == 0) {
	gameData.multiplayer.weaponStates [N_LOCALPLAYER].bTripleFusion = 0;
	}
else if (m_playerShip == 1) {
	LOCALPLAYER.primaryWeaponFlags &= ~(HAS_FLAG (FUSION_INDEX));
	gameData.multiplayer.weaponStates [N_LOCALPLAYER].bTripleFusion = 0;
	LOCALPLAYER.flags &= ~(PLAYER_FLAGS_AMMO_RACK);
	}
else if (m_playerShip == 2) {
	LOCALPLAYER.flags &= ~(PLAYER_FLAGS_AFTERBURNER);
	}
}

// ----------------------------------------------------------------------------

int CMissionConfig::SelectShip (int nShip)
{
if (COMPETITION)
	return m_playerShip = 0;
if ((nShip < 0) || (nShip >= MAX_SHIP_TYPES))
	nShip = 0;
if (m_shipsAllowed [nShip])
	return m_playerShip = nShip;
for (nShip = 0; nShip < MAX_SHIP_TYPES; nShip++)
if (m_shipsAllowed [nShip])
	return m_playerShip = nShip;
return m_playerShip = 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
