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
#include "inferno.h"
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
#include "render.h"
#include "sphere.h"
#include "endlevel.h"
#include "interp.h"
#include "autodl.h"
#include "hiresmodels.h"
#include "soundthreads.h"
#include "gameargs.h"

// ----------------------------------------------------------------------------

void InitGameplayStates (void)
{
gameStates.gameplay.bFinalBossIsDead = 0;
gameStates.gameplay.bHaveSmartMines = 1;
gameStates.gameplay.bMineDestroyed = 0;
gameStates.gameplay.bNoBotAI = 0;
gameStates.gameplay.nPlayerSpeed = 0;
gameStates.gameplay.seismic.nMagnitude = 0;
gameStates.gameplay.seismic.nStartTime = 0;
gameStates.gameplay.seismic.nEndTime = 0;
gameStates.gameplay.seismic.nLevel = 0;
gameStates.gameplay.seismic.nShakeFrequency = 0;
gameStates.gameplay.seismic.nShakeDuration = 0;
gameStates.gameplay.seismic.nSound = SOUND_SEISMIC_DISTURBANCE_START;
gameStates.gameplay.seismic.bSound = 0;
gameStates.gameplay.seismic.nVolume = 0;
gameStates.gameplay.xStartingShields = INITIAL_SHIELDS;
gameStates.gameplay.slowmo [0].nState = 0;
gameStates.gameplay.slowmo [0].fSpeed = 1;
gameStates.gameplay.slowmo [0].tUpdate = 0;
gameStates.gameplay.slowmo [0].bActive = 0;
gameStates.gameplay.slowmo [1].nState = 0;
gameStates.gameplay.slowmo [1].fSpeed = 1;
gameStates.gameplay.slowmo [1].tUpdate = 0;
gameStates.gameplay.slowmo [1].bActive = 0;
gameOpts->gameplay.nSlowMotionSpeedup = 6;	//max slow motion delay
}

// ----------------------------------------------------------------------------

void InitInputStates (void)
{
gameStates.input.nMouseType = -1;
gameStates.input.nJoyType = -1;
gameStates.input.bCybermouseActive = 0;
#if !DBG
gameStates.input.bGrabMouse = 1;
#else
gameStates.input.bGrabMouse = 0;
#endif
gameStates.input.bSkipControls = 0;
gameStates.input.bControlsSkipFrame = 0;
gameStates.input.bKeepSlackTime = 0;
gameStates.input.nCruiseSpeed = 0;
}

// ----------------------------------------------------------------------------

void InitMenuStates (void)
{
gameStates.menus.bInitBG = 1;
gameStates.menus.nInMenu = 0;
gameStates.menus.bHires = 1;
gameStates.menus.bHiresAvailable = 1;
gameStates.menus.bDrawCopyright = 0;
gameStates.menus.bFullScreen = 0;
gameStates.menus.bReordering = 0;
}

// ----------------------------------------------------------------------------

void InitMovieStates (void)
{
gameStates.movies.nRobots = 0;
gameStates.movies.bIntroPlayed = 0;
gameStates.movies.bMac = 0;
}

// ----------------------------------------------------------------------------

void InitMultiplayerStates (void)
{
gameStates.multi.nGameType = 0;
gameStates.multi.nGameSubType = 0;
gameStates.multi.bUseTracker = 0;
gameStates.multi.bServer = 1;
gameStates.multi.bTryAutoDL = 0;
gameStates.multi.bHaveLocalAddress = 0;
gameStates.multi.bSurfingNet = 0;
gameStates.multi.bCheckPorts = 0;
}

// ----------------------------------------------------------------------------

void InitGfxStates (void)
{
gameStates.gfx.bInstalled = 0;
gameStates.gfx.nStartScrMode = 2;
gameStates.gfx.nStartScrSize = 2;
}

// ----------------------------------------------------------------------------

void InitOglStates (void)
{
gameStates.ogl.bInitialized = 0;
gameStates.ogl.bShadersOk = 0;
gameStates.ogl.bRender2TextureOk = 0;
gameStates.ogl.bPerPixelLightingOk = 1;
gameStates.ogl.bUseRender2Texture = 1;
gameStates.ogl.bVoodooHack = 0;
gameStates.ogl.bBrightness = 0;
gameStates.ogl.nContrast = 8;
gameStates.ogl.bFullScreen = 0;
gameStates.ogl.bUseTransform = 0;
gameStates.ogl.nColorBits = 32;
gameStates.ogl.nDepthBits = 24;
gameStates.ogl.bEnableTexture2D = -1;
gameStates.ogl.bEnableTexClamp = -1;
gameStates.ogl.bEnableScissor = 0;
gameStates.ogl.bNeedMipMaps = 0;
gameStates.ogl.texMinFilter = GL_NEAREST;
gameStates.ogl.texMinFilter = GL_NEAREST;
gameStates.ogl.nTexMagFilterState = -1, 
gameStates.ogl.nTexMinFilterState = -1;
gameStates.ogl.nTexEnvModeState = -1, 
gameStates.ogl.nLastX =
gameStates.ogl.nLastY =
gameStates.ogl.nLastW =
gameStates.ogl.nLastH = 1;
gameStates.ogl.nCurWidth =
gameStates.ogl.nCurHeight = -1;
gameStates.ogl.bCurFullScreen = -1;
gameStates.ogl.bAntiAliasing = 0;
gameStates.ogl.bAntiAliasingOk = 0;
gameStates.ogl.bpp = 32;
gameStates.ogl.nRGBAFormat = GL_RGBA;
gameStates.ogl.nRGBFormat = GL_RGB;
gameStates.ogl.bIntensity4 = 1;
gameStates.ogl.bLuminance4Alpha4 = 1;
gameStates.ogl.bReadPixels = 1;
gameStates.ogl.bGetTexLevelParam = 1;
gameStates.ogl.nDrawBuffer = -1;
#ifdef GL_ARB_multitexture
gameStates.ogl.bArbMultiTexture = 0;
#endif
#ifdef GL_SGIS_multitexture
gameStates.ogl.bSgisMultiTexture = 0;
#endif
}

// ----------------------------------------------------------------------------

void InitRenderStates (void)
{
gameStates.render.xZoom = 0x9000;
gameStates.render.xZoomScale = 1;
gameStates.render.nFrameCount = -1;
gameStates.render.bQueryOcclusion = 0;
gameStates.render.bPointSprites = 0;
gameStates.render.bVertexArrays = 1;
gameStates.render.bExternalView = 0;
gameStates.render.bDisableFades = 0;
gameStates.render.bDropAfterburnerBlob = 0;
gameStates.render.grAlpha = FADE_LEVELS;
gameStates.render.bShowFrameRate = 0;
gameStates.render.bShowTime = 0;
gameStates.render.cameras.bActive = 0;
gameStates.render.color.bLightmapsOk = 1;
gameStates.render.nZoomFactor = I2X (1);
gameStates.render.nMinZoomFactor = I2X (1);
gameStates.render.nMaxZoomFactor = I2X (5);
gameStates.render.textures.bGlsTexMergeOk = 0;
gameStates.render.textures.bHaveMaskShader = 0;
gameStates.render.bBlendBackground = 0;
gameStates.render.fonts.bHires = 0;
gameStates.render.fonts.bHiresAvailable = 0;
gameStates.render.fonts.bInstalled = 0;
gameStates.render.nFlashScale = 0;
gameStates.render.nFlashRate = FLASH_CYCLE_RATE;
gameStates.render.bOutsideMine = 0;
gameStates.render.bExtExplPlaying = 0;
gameStates.render.bDoAppearanceEffect = 0;
gameStates.render.glFOV = DEFAULT_FOV;
gameStates.render.glAspect = 1.0f;
gameStates.render.bDetriangulation = 0;
gameStates.render.nInterpolationMethod = 0;
gameStates.render.bTMapFlat = 0;
gameStates.render.nLighting = 1;
gameStates.render.bTransparency = 0;
gameStates.render.bClusterLights = 1;
gameStates.render.bSplitPolys = 1;
gameStates.render.bHaveStencilBuffer = 0;
gameStates.render.nRenderPass = -1;
gameStates.render.nShadowPass = 0;
gameStates.render.bShadowMaps = 0;
gameStates.render.bHeadlightOn = 0;
gameStates.render.bRenderIndirect = 0;
gameStates.render.nModelQuality = 3;
gameStates.render.nType = -1;
gameStates.render.cockpit.bShowPingStats = 0;
gameStates.render.cockpit.nMode = CM_FULL_COCKPIT;
gameStates.render.cockpit.nNextMode = -1;
gameStates.render.cockpit.nModeSave = -1;
gameStates.render.cockpit.bRedraw = 0;
gameStates.render.cockpit.bBigWindowSwitch = 0;
gameStates.render.cockpit.nShieldFlash = 0;
gameStates.render.cockpit.nLastDrawn [0] =
gameStates.render.cockpit.nLastDrawn [1] = -1;
gameStates.render.cockpit.nCoopPlayerView [0] = 
gameStates.render.cockpit.nCoopPlayerView [1] = -1;
gameStates.render.cockpit.n3DView [0] =
gameStates.render.cockpit.n3DView [1] = CV_NONE;
gameStates.render.vr.xEyeWidth = I2X (1);
gameStates.render.vr.nRenderMode	= VR_NONE;
gameStates.render.vr.nLowRes = 3;
gameStates.render.vr.bShowHUD	= 1;
gameStates.render.vr.nSensitivity = 1;
gameStates.render.detail.nRenderDepth = DEFAULT_RENDER_DEPTH;
gameStates.render.detail.nObjectComplexity = 2; 
gameStates.render.detail.nObjectDetail = 2;
gameStates.render.detail.nWallDetail = 2; 
gameStates.render.detail.nWallRenderDepth = 2; 
gameStates.render.detail.nDebrisAmount = 2; 
gameStates.render.bUsePerPixelLighting = 1;
gameStates.render.nMaxLightsPerPass = 8;
gameStates.render.nMaxLightsPerFace = 16;
}

// ----------------------------------------------------------------------------

void InitSoundStates (void)
{
gameStates.sound.bWasRecording = 0;
gameStates.sound.bDontStartObjects = 0;
gameStates.sound.nConquerWarningSoundChannel = -1;
gameStates.sound.nSoundChannels = 2;
gameStates.sound.digi.bSoundsInitialized = 0;
gameStates.sound.digi.bLoMem = 0;
}

// ----------------------------------------------------------------------------

void InitVideoStates (void)
{
gameStates.video.nDisplayMode = 2;
gameStates.video.nDefaultDisplayMode = 2;
gameStates.video.nScreenMode = (u_int32_t) -1;
gameStates.video.nLastScreenMode = (u_int32_t) -1;
gameStates.video.nWidth = -1;
gameStates.video.nHeight = -1;
gameStates.video.bFullScreen = -1;
}

// ----------------------------------------------------------------------------

void InitAppStates (void)
{
gameStates.app.bHaveExtraData = 0;
gameStates.app.nExtGameStatus = 1;
gameStates.app.nFunctionMode = FMODE_MENU;
gameStates.app.nLastFuncMode = -1;
gameStates.app.nCriticalError = 0;
gameStates.app.bNostalgia = 0;
gameStates.app.iNostalgia = 0;
gameStates.app.bD2XLevel = 0;
gameStates.app.bGameRunning = 0;
gameStates.app.bGameAborted = 0;
gameStates.app.bGameSuspended = 0;
gameStates.app.bEnterGame = 0;
gameStates.app.bUseSound = 1;
gameStates.app.bLunacy = 0;
gameStates.app.bEnableShadows = 1;
gameStates.app.bHaveExtraGameInfo [0] = 1;
gameStates.app.bHaveExtraGameInfo [1] = 0;
gameStates.app.nSDLTicks = -1;
#if DBG
gameStates.app.bEnglish = 1;
#else
gameStates.app.bEnglish = 1;
#endif
gameStates.app.bMacData = 0;
gameStates.app.bD1Model = 0;
gameStates.app.bD1Data = 0;
gameStates.app.bD1Mission = 0;
gameStates.app.bHaveD1Data = 0;
gameStates.app.bHaveD1Textures = 0;
gameStates.app.bEndLevelDataLoaded = 0;
gameStates.app.bEndLevelSequence = 0;
gameStates.app.bPlayerIsDead = 0;
gameStates.app.bPlayerExploded = 0;
gameStates.app.bPlayerEggsDropped = 0;
gameStates.app.bDeathSequenceAborted = 0;
gameStates.app.bUseDefaults = 0;
gameStates.app.nCompSpeed = 3;
gameStates.app.bPlayerFiredLaserThisFrame = 0;
gameStates.app.bConfigMenu = 0;
gameStates.app.bAutoRunMission = 0;
gameStates.entropy.bConquering = 0;
gameStates.entropy.bConquerWarning = 0;
gameStates.entropy.nTimeLastMoved = -1;
gameStates.app.bFirstSecretVisit = 1;
gameStates.app.nDifficultyLevel = DEFAULT_DIFFICULTY;
gameStates.app.nDetailLevel = NUM_DETAIL_LEVELS - 1;
gameStates.app.nBaseCtrlCenExplTime = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;
gameStates.app.bDebugSpew = 1;
gameStates.app.bProgressBars = 1;
gameStates.app.bFixModels = 0;
gameStates.app.bAltModels = 0;
gameStates.app.cheats.bEnabled = 0;
gameStates.app.cheats.bTurboMode = 0;
gameStates.app.cheats.bMonsterMode = 0;
gameStates.app.cheats.bLaserRapidFire = 0;
gameStates.app.cheats.bRobotsKillRobots = 0;
gameStates.app.cheats.bJohnHeadOn = 0;
gameStates.app.cheats.bHomingWeapons = 0;
gameStates.app.cheats.bBouncingWeapons = 0;
gameStates.app.cheats.bMadBuddy = 0;
gameStates.app.cheats.bAcid = 0;
gameStates.app.cheats.bPhysics = 0;
gameStates.app.cheats.bRobotsFiring = 1;
gameStates.app.cheats.bD1CheatsEnabled = 0;
gameStates.app.cheats.nUnlockLevel = 0;
gameStates.limitFPS.bControls = 1;
gameStates.limitFPS.bJoystick = !gameStates.limitFPS.bControls;
gameStates.limitFPS.bFusion = 0;
gameStates.limitFPS.bCountDown = 1;
gameStates.limitFPS.bSeismic = 1;
gameStates.limitFPS.bHomers = 1;
gameStates.limitFPS.bOmega = 1;
}

// ----------------------------------------------------------------------------

void InitGameStates (void)
{
memset (&gameStates, 0, sizeof (gameStates));
CheckEndian ();
InitGameConfig ();
InitAppStates ();
InitGameplayStates ();
InitInputStates ();
InitMenuStates ();
InitMovieStates ();
InitMultiplayerStates ();
InitGfxStates ();
InitOglStates ();
InitRenderStates ();
InitSoundStates ();
InitVideoStates ();
}

// ----------------------------------------------------------------------------
