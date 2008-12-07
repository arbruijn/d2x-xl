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

/*
 *
 * Header file for Inferno.  Should be included in all source files.
 *
 */

#ifndef _INFERNO_H
#define _INFERNO_H

#include <math.h>

#include "carray.h"

#include "irrstuff.h"

#ifdef _DEBUG
#	define DBG 1
#else
#	define DBG 0
#endif

#define SHOW_EXIT_PATH  1

#define MAX_SUBMODELS	10		// how many animating sub-objects per model

#define MULTI_THREADED_SHADOWS	0
#define MULTI_THREADED_LIGHTS		0
#define MULTI_THREADED_PRECALC	1

#define USE_SEGRADS		0
#define CALC_SEGRADS		1
#define GEOMETRY_VBOS	0
#if 1
#	define RENDERPATH		1
#else
#	define RENDERPATH		gameOpts->render.nPath
#endif

#if DBG
#	define	SHADOWS	1
#else
#	define	SHADOWS	1
#endif

#if DBG
#	define	PROFILING 1
#else
#	define	PROFILING 0
#endif

#if SHADOWS
#	if DBG
#		define DBG_SHADOWS 1
#	else
#		define DBG_SHADOWS 0
#	endif
#endif

#if USE_IRRLICHT
#	define FBO_DRAW_BUFFER 0
#else
#	define FBO_DRAW_BUFFER 1
#endif

#define PER_PIXEL_LIGHTING 1

#include "vers_id.h"
#include "pstypes.h"
#include "3d.h"
#include "bm.h"
#include "gr.h"
#include "piggy.h"
#include "ogl_defs.h"
#include "segment.h"
#include "aistruct.h"
#include "object.h"
#include "switch.h"
#include "robot.h"
#include "wall.h"
#include "powerup.h"
#include "vclip.h"
#include "effects.h"
#include "oof.h"
#include "ase.h"
#include "cfile.h"
#include "segment.h"
#include "console.h"
#include "vecmat.h"

#ifdef __macosx__
# include <SDL/SDL.h>
# include <SDL/SDL_thread.h>
#else
# include <SDL.h>
# include <SDL_thread.h>
#endif

/**
 **	Constants
 **/

#ifndef M_PI
#	define M_PI 3.141592653589793240
#endif

// How close two points must be in all dimensions to be considered the
// same point.
#define	FIX_EPSILON	10

#define DEFAULT_DIFFICULTY		1

#define DEFAULT_CONTROL_CENTER_EXPLOSION_TIME 30    // Note: Usually uses Alan_pavlish_reactorTimes, but can be overridden in editor.

//for Function_mode variable
#define FMODE_EXIT		0		// leaving the program
#define FMODE_MENU		1		// Using the menu
#define FMODE_GAME		2		// running the game
#define FMODE_EDITOR		3		// running the editor

#define FLASH_CYCLE_RATE f1_0

#define AMBIENT_LIGHT	0.3f
#define DIFFUSE_LIGHT	0.7f

#define MAX_THREADS		4

//------------------------------------------------------------------------------

typedef int _CDECL_	tThreadFunc (void *);
typedef tThreadFunc *pThreadFunc;

typedef struct tThreadInfo {
	SDL_Thread		*pThread;
	SDL_sem			*done;
	SDL_sem			*exec;
	int				nId;
	int				bExec;
	int				bDone;
	int				bBlock;
	int				bQuit;
	} tThreadInfo;

//------------------------------------------------------------------------------

// The version number of the game
typedef struct tLegacyOptions {
	int bInput;
	int bFuelCens;
	int bMouse;
	int bHomers;
	int bRender;
	int bSwitches;
	int bWalls;
} tLegacyOptions;

//------------------------------------------------------------------------------

typedef struct tCameraOptions {
	int nFPS;
	int nSpeed;
	int bFitToWall;
	int bHires;
} tCameraOptions;

//------------------------------------------------------------------------------

typedef struct tWeaponIconOptions {
	int bSmall;
	char bShowAmmo;
	char bEquipment;
	char nSort;
	ubyte alpha;
} tWeaponIconOptions;

//------------------------------------------------------------------------------

typedef struct tColorOptions {
	int bCap;
	int bAmbientLight;
	int bGunLight;
	int bWalls;
	int bMix;
	int bUseLightmaps;
	int nLightmapRange;
	int nSaturation;
} tColorOptions;

//------------------------------------------------------------------------------

typedef struct tCockpitOptions {
	int bHUD;
	int bHUDMsgs;
	int bSplitHUDMsgs;	//split tPlayer and other message displays
	int bReticle;
	int bMouseIndicator;
	int bTextGauges;
	int bScaleGauges;
	int bFlashGauges;
	int bMissileView;
	int bGuidedInMainView;
	int bObjectTally;
	int bRotateMslLockInd;
	int bPlayerStats;
	int nWindowPos;
	int nWindowSize;
	int nWindowZoom;
} tCockpitOptions;

//------------------------------------------------------------------------------

typedef struct tTextureOptions {
	int bUseHires;
	int nQuality;
} tTextureOptions;

//------------------------------------------------------------------------------

typedef struct tParticleOptions {
	int nDens [5];
	int nSize [5];
	int nLife [5];
	int nAlpha [5];
	int bSyncSizes;
	int bPlayers;
	int bRobots;
	int bMissiles;
	int bPlasmaTrails;
	int bDebris;
	int bStatic;
	int bBubbles;
	int bWobbleBubbles;
	int bWiggleBubbles;
	int bCollisions;
	int bDisperse;
	int bRotate;
	int bSort;
	int bDecreaseLag;	//only render if tPlayer is moving forward
	int bAuxViews;
	int bMonitors;
} tParticleOptions;

//------------------------------------------------------------------------------

typedef struct tLightningOptions {
	int nQuality;
	int nStyle;
	int bPlasma;
	int bPlayers;
	int bRobots;
	int bDamage;
	int bExplosions;
	int bStatic;
	int bOmega;
	int bRobotOmega;
	int bAuxViews;
	int bMonitors;
} tLightningOptions;

//------------------------------------------------------------------------------

typedef struct tShadowOptions {
	int nReach;
	int nLights;
	int bFast;
	int nClip;
	int bSoft;
	int bPlayers;
	int bRobots;
	int bMissiles;
	int bPowerups;
	int bReactors;
	} tShadowOptions;

//------------------------------------------------------------------------------

typedef struct tPowerupOptions {
	int b3D;
	int nSpin;
} tPowerupOptions;

//------------------------------------------------------------------------------

typedef struct tAutomapOptions {
	int bTextured;
	int bBright;
	int bCoronas;
	int bParticles;
	int bSparks;
	int bLightnings;
	int bGrayOut;
	int bSkybox;
	int nColor;
	int nRange;
} tAutomapOptions;

//------------------------------------------------------------------------------

typedef struct tShipRenderOptions {
	int nWingtip;
	int bBullets;
	int nColor;
} tShipRenderOptions;

//------------------------------------------------------------------------------

typedef struct tCoronaRenderOptions {
	int bUse;
	int nStyle;
	int bShots;
	int bWeapons;
	int bPowerups;
	int bAdditive; //additive corona blending for wall lights 
	int bAdditiveObjs; //additive corona blending for light emitting weapons
	int nIntensity;
	int nObjIntensity;
} tCoronaRenderOptions;

//------------------------------------------------------------------------------

typedef struct tEffectRenderOptions {
	int bExplBlasts;
	int nShrapnels;
	int bEnergySparks;
	int bRobotShields;
	int bOnlyShieldHits;
	int bAutoTransparency;
	int bTransparent;
	int bSoftParticles;
	int bMovingSparks;
} tEffectRenderOptions;

//------------------------------------------------------------------------------

typedef struct tDebugRenderOptions {
	int bDynamicLight;
	int bObjects;
	int bTextures;
	int bWalls;
	int bWireFrame;
} tDebugRenderOptions;

//------------------------------------------------------------------------------

typedef struct tRenderOptions {
	int bAllSegs;
	int nLightingMethod;
	int bHiresModels;
	int nMeshQuality;
	int bUseLightmaps;
	int nLightmapQuality;
	int bUseShaders;
	int bBrightObjects;
	int nMathFormat;
	int nDefMathFormat;
	short nMaxFPS;
	int nPath;
	int nQuality;
	int nTextureQuality;
	int nDebrisLife;
	int bDepthSort;
	tCameraOptions cameras;
	tColorOptions color;
	tCockpitOptions cockpit;
	tTextureOptions textures;
	tWeaponIconOptions weaponIcons;
	tParticleOptions particles;
	tLightningOptions lightnings;
	tShadowOptions shadows;
	tPowerupOptions powerups;
	tAutomapOptions automap;
	tShipRenderOptions ship;
	tCoronaRenderOptions coronas;
	tEffectRenderOptions effects;
	tDebugRenderOptions debug;
} tRenderOptions;

//------------------------------------------------------------------------------

typedef struct tOglOptions {
	int bSetGammaRamp;
	int bGlTexMerge;
	int bLightObjects;
	int bLightPowerups;
	int bObjLighting;
	int bHeadlight;
	int nMaxLightsPerFace;
	int nMaxLightsPerPass;
	int nMaxLightsPerObject;
	int bVoodooHack;
} tOglOptions;

//------------------------------------------------------------------------------

typedef struct tMovieOptions {
	int bHires;
	int nQuality;
	int nLevel;
	int bResize;
	int bFullScreen;
	int bSubTitles;
} tMovieOptions;

//------------------------------------------------------------------------------

typedef struct tGameplayOptions {
	int nAutoSelectWeapon;
	int bSecretSave;
	int bTurboMode;
	int bFastRespawn;
	int nAutoLeveling;
	int bEscortHotKeys;
	int bSkipBriefingScreens;
	int bHeadlightOnWhenPickedUp;
	int bShieldWarning;
	int bInventory;
	int bIdleAnims;
	int nAIAwareness;
	int nAIAggressivity;
	int nSlowMotionSpeedup;
	int bUseD1AI;
	int bNoThief;
} tGameplayOptions;

//------------------------------------------------------------------------------

#define UNIQUE_JOY_AXES	5

typedef struct tMouseInputOptions {
	int bUse;
	int bSyncAxes;
	int bJoystick;
	int nDeadzone;
	int sensitivity [3];
	} tMouseInputOptions;

//------------------------------------------------------------------------------

typedef struct tJoystickInputOptions {
	int bUse;
	int bSyncAxes;
	int bLinearSens;
	int sensitivity [UNIQUE_JOY_AXES];
	int deadzones [UNIQUE_JOY_AXES];
	} tJoystickInputOptions;

//------------------------------------------------------------------------------

typedef struct tTrackIRInputOptions {
	int bPresent;
	int bUse;
	int nMode;
	int bMove [5];
	int nDeadzone;
	int sensitivity [3];
	int bSyncAxes;
	} tTrackIRInputOptions;

//------------------------------------------------------------------------------

typedef struct tKeyboardInputOptions {
	int bUse;
	int nRamp;
	int bRamp [3];
	} tKeyboardInputOptions;

//------------------------------------------------------------------------------

typedef struct tInputOptions {
	int bLimitTurnRate;
	int nMinTurnRate;
	int nMaxPitch;
	int bUseHotKeys;
	tMouseInputOptions mouse;
	tJoystickInputOptions joystick;
	tTrackIRInputOptions trackIR;
	tKeyboardInputOptions keyboard;
} tInputOptions;

//------------------------------------------------------------------------------

typedef struct tSoundOptions {
	int bUseD1Sounds;
	int bUseRedbook;
	int bHires;
	int bUseSDLMixer;
	int bUseOpenAL;
	int bFadeMusic;
	int digiSampleRate;
	int bShip;
	int bMissiles;
	int bGatling;
	int bSpeedUp;
	fix xCustomSoundVolume;
} tSoundOptions;

//------------------------------------------------------------------------------

typedef struct tAltBgOptions {
	int bHave;
	double alpha;
	double brightness;
	int grayscale;
	char szName [FILENAME_LEN];
} tAltBgOptions;

//------------------------------------------------------------------------------

typedef struct tMenuOptions {
	int nStyle;
	int bFastMenus;
	int bSmartFileSearch;
	int bShowLevelVersion;
	char nHotKeys;
	tAltBgOptions altBg;
} tMenuOptions;

//------------------------------------------------------------------------------

typedef struct tDemoOptions {
	int bOldFormat;
	int bRevertFormat;
} tDemoOptions;

//------------------------------------------------------------------------------

typedef struct tMultiplayerOptions {
	int bNoRankings;
	int bTimeoutPlayers;
	int bUseMacros;
	int bNoRedundancy;
} tMultiplayerOptions;

//------------------------------------------------------------------------------

typedef struct tApplicationOptions {
	int bAutoRunMission;
	int nVersionFilter;
	int bSinglePlayer;
	int bExpertMode;
	int nScreenShotInterval;
} tApplicationOptions;

//------------------------------------------------------------------------------

typedef struct tGameOptions {
	tRenderOptions			render;
	tGameplayOptions		gameplay;
	tInputOptions			input;
	tMenuOptions			menus;
	tSoundOptions			sound;
	tMovieOptions			movies;
	tLegacyOptions			legacy;
	tOglOptions				ogl;
	tApplicationOptions	app;
	tMultiplayerOptions	multi;
	tDemoOptions			demo;
} tGameOptions;

//------------------------------------------------------------------------------
// states
//------------------------------------------------------------------------------

typedef struct tSeismicStates {
	fix	nMagnitude;
	fix	nStartTime;
	fix	nEndTime;
	fix	nNextSoundTime;
	int	nLevel;
	int	nShakeFrequency;
	int	nShakeDuration;
	int	nSound;
	int	bSound;
	int	nVolume;
} tSeismicStates;

//------------------------------------------------------------------------------

typedef struct tSlowMotionStates {
	float fSpeed;
	int	nState;
	time_t tUpdate;
	int	bActive;
	} tSlowMotionStates;

//------------------------------------------------------------------------------

typedef struct tGameplayStates {
	int bMultiBosses;
	int bFinalBossIsDead;
	int bHaveSmartMines;
	int bMineDestroyed;
	int bKillBossCheat;
	int bNoBotAI;
	int bTagFlag;
	int nReactorCount;
	int nLastReactor;
	int bMineMineCheat;
	int bAfterburnerCheat;
	int bTripleFusion;
	int bLastAfterburnerState;
	fix xLastAfterburnerCharge;
	fix nPlayerSpeed;
	fix xStartingShields;
	vmsVector vTgtDir;
	int nDirSteps;
	tSeismicStates seismic;
	tSlowMotionStates slowmo [2];
} tGameplayStates;

//------------------------------------------------------------------------------

#define BOSS_COUNT	(extraGameInfo [0].nBossCount - gameStates.gameplay.nReactorCount)


typedef struct tKeyStates {
	ubyte 	nBufferType;		// 0=No buffer, 1=buffer ASCII, 2=buffer scans
	ubyte 	bRepeat;
	ubyte 	bEditorMode;
	ubyte 	nLastPressed;
	ubyte 	nLastReleased;
	ubyte		pressed [256];
	int		xLastPressTime;
	} tKeyStates;

typedef struct tInputStates {
	int			nPlrFileVersion;
	int			nMouseType;
	int			nJoyType;
	int			nJoysticks;
	int			bGrabMouse;
	int			bHaveTrackIR;
	ubyte			bCybermouseActive;
	int			bSkipControls;
	int			bControlsSkipFrame;
	int			bKeepSlackTime;
	time_t		kcPollTime;
	float			kcFrameTime;
	fix			nCruiseSpeed;
	tKeyStates	keys;
} tInputStates;

//------------------------------------------------------------------------------

typedef struct tMenuStates {
	int bHires;
	int bHiresAvailable;
	int nInMenu;
	int bInitBG;
	int bDrawCopyright;
	int bUseTracker;
	int bFullScreen;
	int bReordering;
	int bNoBackground;
} tMenuStates;

//------------------------------------------------------------------------------

typedef struct tMovieStates {
	int nRobots;
	int bIntroPlayed;
	int bMac;
} tMovieStates;

//------------------------------------------------------------------------------

#include "player.h"

typedef struct tMultiplayerStates {
	int bUseTracker;
	int bTrackerCall;
	int bServer;
	int bHaveLocalAddress;
	int nGameType;
	int nGameSubType;
	int bTryAutoDL;
	int nConnection;
	int bIWasKicked;
	int bCheckPorts;
	ubyte bSurfingNet;
	int bPlayerIsTyping [MAX_PLAYERS];
} tMultiplayerStates;

//------------------------------------------------------------------------------

typedef struct tGfxStates {
	int bInstalled;
	int bOverride;
	int nStartScrSize;
	int nStartScrMode;
} tGfxStates;

//------------------------------------------------------------------------------

typedef struct tOglStates {
	int bInitialized;
	int bShadersOk;
	int bMultiTexturingOk;
	int bRender2TextureOk;
	int bPerPixelLightingOk;
	int bUseRender2Texture;
	int bDrawBufferActive;
	int bReadBufferActive;
	int bFullScreen;
	int bLastFullScreen;
	int bUseTransform;
	int bGlTexMerge;
	int bBrightness;
	int nColorBits;
	GLint nDepthBits;
	GLint nStencilBits;
	int bEnableTexture2D;
	int bEnableTexClamp;
	int bEnableScissor;
	int bNeedMipMaps;
	int bAntiAliasing;
	int bAntiAliasingOk;
	int bVoodooHack;
	int bTextureCompression;
	int bHaveTexCompression;
	int bHaveVBOs;
	int texMinFilter;
	int texMagFilter;
	int nTexMagFilterState;
	int nTexMinFilterState;
	int nTexEnvModeState;
	int nContrast;
	int nLastX;
	int nLastY;
	int nLastW;
	int nLastH;
	int nCurWidth;
	int nCurHeight;
	int nLights;
	int iLight;
	int nFirstLight;
	int bCurFullScreen;
	int nReticle;
	int bpp;
	int bScaleLight;
	int bDynObjLight;
	int bVertexLighting;
	int bHeadlight;
	int bStandardContrast;
	int nRGBAFormat;
	int nRGBFormat;
	int bIntensity4;
	int bLuminance4Alpha4;
	int bReadPixels;
	int bOcclusionQuery;
	int bDepthBlending;
	int bUseDepthBlending;
	int bHaveDepthBuffer;
	int bHaveBlur;
	int bGetTexLevelParam;
	int nDrawBuffer;
	int nStencil;
#ifdef GL_ARB_multitexture
	int bArbMultiTexture;
#endif
#ifdef GL_SGIS_multitexture
	int bSgisMultiTexture;
#endif
	float fAlpha;
	float fLightRange;
	GLuint hDepthBuffer; 
} tOglStates;

//------------------------------------------------------------------------------

typedef struct tCameraStates {
	int bActive;
} tCameraStates;

//------------------------------------------------------------------------------

typedef struct tColorStates {
	int bLightmapsOk;
	int bRenderLightmaps;
} tColorStates;

//------------------------------------------------------------------------------

typedef struct tTextureStates {
	int bGlsTexMergeOk;
	int bHaveMaskShader;
	int bHaveGrayScaleShader;
} tTextureStates;

//------------------------------------------------------------------------------

typedef struct tCockpitStates {
	int bShowPingStats;
	int nMode;
	int nNextMode;
	int nModeSave;
	int nShieldFlash;
	int bRedraw;
	int bBigWindowSwitch;
	int nLastDrawn [2];
	int n3DView [2];
	int nCoopPlayerView [2];
} tCockpitStates;

//------------------------------------------------------------------------------

typedef struct tVRBuffers {
	CCanvas		*offscreen;			// The offscreen data buffer
	CCanvas		render [2];			//  Two offscreen buffers for left/right eyes.
	CCanvas		subRender [2];		//  Two sub buffers for left/right eyes.
	CCanvas		screenPages [2];	//  Two pages of VRAM if paging is available
	CCanvas		editorCanvas;		//  The canvas that the editor writes to.
} tVRBuffers;

typedef struct tVRStates {
	u_int32_t	nScreenSize;
	ubyte			nScreenFlags;	//see values in screens.h
	ubyte			nCurrentPage;
	fix			xEyeWidth;
	int			nRenderMode;
	int			nLowRes;			// Default to low res
	int 			bShowHUD;
	int			nSensitivity;	// 0 - 2
	int			nEyeOffset;
	int			nEyeSwitch;
	int			bEyeOffsetChanged;
	int			bUseRegCode;
	tVRBuffers	buffers;
} tVRStates;

//------------------------------------------------------------------------------

typedef struct tFontStates {
	int bHires;
	int bHiresAvailable;
	int bInstalled;
} tFontStates;

//------------------------------------------------------------------------------

#define DEFAULT_RENDER_DEPTH 255

typedef struct tRenderDetail {
	int nLevel;
	int nRenderDepth;
	int nMaxDebrisObjects;
	int nMaxObjectsOnScreenDetailed;
	int nMaxLinearDepthObjects;
	int nMaxPerspectiveDepth;
	int nMaxLinearDepth;
	int nMaxFlatDepth;
	sbyte nObjectComplexity;
	sbyte nObjectDetail;
	sbyte nWallDetail;
	sbyte nWallRenderDepth;
	sbyte nDebrisAmount; 
	sbyte nSoundChannels;
} tRenderDetail;

typedef struct tAutomapStates {
	int bDisplay;
	int bFull;
	int bRadar;
	int nSegmentLimit;
	int nMaxSegsAway;
	} tAutomapStates;

typedef struct tRenderHistory {
	CBitmap		*bmBot;
	CBitmap		*bmTop;
	CBitmap		*bmMask;
	ubyte			bSuperTransp;
	ubyte			bShaderMerge;
	int			bOverlay;
	int			bColored;
	int			nShader;
	int			nType;
	int			nBlendMode;
} tRenderHistory;

typedef struct tRenderStates {
	int bTopDownRadar;
	int bExternalView;
	int bQueryOcclusion;
	int bPointSprites;
	int bVertexArrays;
	int bAutoMap;
	int bLightmaps;
	int bUseLightmaps;
	int bDisableFades;
	int bBlendBackground;
	int bDropAfterburnerBlob;
	int bOutsideMine;
	int bExtExplPlaying;
	int bDoAppearanceEffect;
	int bGlLighting;
	int bColored;
	int bBriefing;
	int bRearView;
	int bEnableSSE;
	int nInterpolationMethod;
	int bTMapFlat;
	int bCloaked;
	int bBrightObject;
	int nWindow;
	int nEyeOffset;
	int nStartSeg;
	int nLighting;
	int nMaxTextureQuality;
	int bTransparency;
	int bSplitPolys;
	int bHaveDynLights;
	int bPaletteFadedOut;
	int bHaveStencilBuffer;
	int bUsePerPixelLighting;
	int nRenderPass;
	int nShadowPass;
	int nShadowBlurPass;
	int bBlurPass;
	int bShadowMaps;
	int bLoResShadows;
	int bUseCameras;
	int bUseDynLight;
	int bApplyDynLight;
	int bClusterLights;
	int nSoften;
	int bHeadlightOn;
	int bHeadlights;
	int bHaveSkyBox;
	int bAllVisited;
	int bViewDist;
	int bD2XLights;
	int bRendering;
	int bFullBright;
	int bQueryCoronas;
	int bDoLightmaps;
	int bAmbientColor;
	int bDoCameras;
	int bRenderIndirect;
	int bBuildModels;
	int bShowFrameRate;
	int bShowTime;
	int bShowProfiler;
	int bTriangleMesh;
	int nFrameFlipFlop;
	int nModelQuality;
	int nMeshQuality;
	int nState;	//0: render geometry, 1: render objects
	int nType;
	int nFrameCount;
	int nLightingMethod;
	int bPerPixelLighting;
	int nMaxLightsPerPass;
	int nMaxLightsPerFace;
	int nMaxLightsPerObject;
	fix xZoom;
	fix xZoomScale;
	ubyte nRenderingType;
	fix nFlashScale;
	fix nFlashRate;
	tCameraStates cameras;
	tCockpitStates cockpit;
	tVRStates vr;
	tColorStates color;
	tFontStates fonts;
	tTextureStates textures;
	fix nZoomFactor;
	fix nMinZoomFactor;
	fix nMaxZoomFactor;
	int bDetriangulation;
	GLenum nFacePrimitive;
	double glFOV;
	double glAspect;
	float grAlpha;
	tRenderDetail detail;
	tAutomapStates automap;
	tRenderHistory history;
} tRenderStates;

//------------------------------------------------------------------------------

typedef struct tDigiStates {
	int bInitialized;
	int bAvailable;
	int bSoundsInitialized;
	int bLoMem;
	int nMaxChannels;
	int nFreeChannel;
	int nVolume;
	int nNextSignature;
	int nActiveObjects;
	short nLoopingSound;
	short nLoopingVolume;
	short nLoopingStart;
	short nLoopingEnd;
	short nLoopingChannel;
} tDigiStates;

//------------------------------------------------------------------------------

typedef struct tSoundStates {
	int nCurrentSong;
	int bRedbookEnabled;
	int bRedbookPlaying;
	int bWasRecording;
	int bDontStartObjects;
	int nConquerWarningSoundChannel;
	int nSoundChannels;
	int bD1Sound;
	tDigiStates digi;
} tSoundStates;

//------------------------------------------------------------------------------

typedef struct tVideoStates {
	int nDisplayMode;
	int nDefaultDisplayMode;
	u_int32_t nScreenMode;
	u_int32_t nLastScreenMode;
	int nWidth;
	int nHeight;
	int bFullScreen;
} tVideoStates;

//------------------------------------------------------------------------------

typedef struct tCheatStates {
	int bEnabled;
	int bTurboMode;
	int bMonsterMode;
	int bLaserRapidFire;
	int bRobotsFiring;
	int bRobotsKillRobots;
	int bJohnHeadOn;
	int bHomingWeapons;
	int bBouncingWeapons;
	int bMadBuddy;
	int bAcid;
	int bPhysics;
	int bSpeed;
	int bD1CheatsEnabled;
	int nUnlockLevel;
} tCheatStates;

//------------------------------------------------------------------------------

typedef struct tEntropyStates {
	int bConquering;
	int bConquerWarning;
	int bExitSequence;
	int nTimeLastMoved;
} tEntropyStates;

//------------------------------------------------------------------------------

typedef struct tSlowTick {
	int bTick;
	int nTime;
	int nSlack;
	int nLastTick;
} tSlowTick;

typedef struct tApplicationStates {
	tSlowTick tick40fps;
	tSlowTick tick60fps;
#if 1 //MULTI_THREADED
	int bExit;
	int bMultiThreaded;
#endif
	int bDemoData;
	int nSDLTicks;
	int nExtGameStatus;
	int nFunctionMode;
	int nLastFuncMode;
	int nCriticalError;
	int bNostalgia;
	int iNostalgia;
	int bInitialized;
	int bD2XLevel;
	int bEnterGame;
	int bFreeCam;
	int bSaveScreenshot;
	int bGameRunning;
	int bGameSuspended;
	int bGameAborted;
	int bBetweenLevels;
	int bPlayerIsDead;
	int bPlayerExploded;
	int bPlayerEggsDropped;
	int bDeathSequenceAborted;
	int bPlayerFiredLaserThisFrame;
	int bUseSound;
	int bMacData;
	int bLunacy;
	int bEnglish;
	int bD1Data;
	int bD1Model;
	int bD1Mission;
	int bHaveD1Data;
	int bHaveD1Textures;
	int bEndLevelDataLoaded;
	int bEndLevelSequence;
	int bFirstSecretVisit;
	int bHaveExtraGameInfo [2];
	int bConfigMenu;
	int nDifficultyLevel;
	int nDetailLevel;
	int nBaseCtrlCenExplTime;
	int bUseDefaults;
	int nCompSpeed;
	int bHaveExtraData;
	int bHaveExtraMovies;
	int bDebugSpew;
	int bAutoRunMission;
	int bProgressBars;
	int bLittleEndian;
	int bUsingConverter;
	int bFixModels;
	int bAltModels;
	int bEnableShadows;
	int bEnableFreeCam;
	int bCacheTextures;
	int bCacheLights;
	int bCacheMeshes;
	int bCacheLightmaps;
	int bCacheModelData;
	int bUseSwapFile;
	int bSingleStep;
	int bAutoDemos;	//automatically play demos or intro movie if user is idling in the main menu
	int bShowError;
	fix xThisLevelTime;
	fix nPlayerTimeOfDeath;
	char *szCurrentMission;
	char *szCurrentMissionFile;
	tTransformation playerPos;
	short nPlayerSegment;
	tCheatStates cheats;
} tApplicationStates;

//------------------------------------------------------------------------------

typedef struct tLimitFPSStates {
	ubyte	bControls;
	ubyte bJoystick;
	ubyte	bSeismic;
	ubyte bCountDown;
	ubyte	bHomers;
	ubyte	bFusion;
	ubyte bOmega;
} tLimitFPSStates;

//------------------------------------------------------------------------------

typedef struct tGameStates {
	tGameplayStates		gameplay;
	tInputStates			input;
	tMenuStates				menus;
	tMovieStates			movies;
	tMultiplayerStates	multi;
	tGfxStates				gfx;
	tOglStates				ogl;
	tRenderStates			render;
	tSoundStates			sound;
	tVideoStates			video;
	tApplicationStates	app;
	tEntropyStates			entropy;
	tLimitFPSStates		limitFPS;
} tGameStates;

//------------------------------------------------------------------------------

extern tGameOptions	gameOptions [2];
extern tGameStates	gameStates;
extern tGameOptions	*gameOpts;

//------------------------------------------------------------------------------
// data
//------------------------------------------------------------------------------


#define MAX_PATH_POINTS		20

typedef struct tPathPoint {
	vmsVector			vPos;
	vmsVector			vOrgPos;
	vmsMatrix			mOrient;
} tPathPoint;

typedef struct tFlightPath {
	tPathPoint			path [MAX_PATH_POINTS];
	tPathPoint			*pPos;
	int					nSize;
	int					nStart;
	int					nEnd;
	time_t				tRefresh;
	time_t				tUpdate;
} tFlightPath;

//------------------------------------------------------------------------------

#include "cameras.h"

typedef struct tShadowLightInfo {
	fVector		vPosf;
	short			nMap;
	ubyte			nFrame;	//set per frame when scene as seen from a light source has been rendered
#if DBG
	vmsMatrix	orient;
#endif
} tShadowLightInfo;

#define MAX_SHADOW_MAPS	20
#define MAX_SHADOW_LIGHTS 8

typedef struct tLightRef {
	short			nSegment;
	short			nSide;
	short			nTexture;
} tLightRef;

//------------------------------------------------------------------------------

typedef struct tColorData {
	CArray<tFaceColor>	lights;
	CArray<tFaceColor>	sides;
	CArray<tFaceColor>	segments;
	CArray<tFaceColor>	vertices;
	CArray<float>			vertBright;
	CArray<tFaceColor>	ambient;	//static light values
	tFaceColor				textures [MAX_WALL_TEXTURES];
	tFaceColor				defaultTextures [2][MAX_WALL_TEXTURES];
	CArray<tLightRef>		visibleLights;
	int						nVisibleLights;
	tRgbColorf				flagTag;
} tColorData;

//------------------------------------------------------------------------------

typedef struct tPulseData {
	float			fScale;
	float			fMin;
	float			fDir;
	float			fSpeed;
} tPulseData;

typedef struct tSphereData {
	CArray<tOOF_vector>	sphere;
	int						nTessDepth;
	int						nFaces;
	int						nFaceNodes;
	tPulseData				pulse;
	tPulseData*				pulseP;
} tSphereData;

//------------------------------------------------------------------------------

#define USE_OGL_LIGHTS	0
#define MAX_OGL_LIGHTS  (64 * 64) //MUST be a power of 2!
#define MAX_SHADER_LIGHTS	1000

typedef struct tDynLightInfo {
	tFace		*faceP;
	vmsVector	vPos;
	fVector		vDirf;
	tRgbaColorf	color;
	float			fBrightness;
	float			fBoost;
	float			fRange;
	float			fRad;
	float			fSpotAngle;
	float			fSpotExponent;
	short			nSegment;
	short			nSide;
	short			nObject;
	ubyte			nPlayer;
	ubyte			nType;
	ubyte			bState;
	ubyte			bOn;
	ubyte			bSpot;
	ubyte			bVariable;
	ubyte			bPowerup;
	} tDynLightInfo;

typedef struct tDynLight {
	vmsVector	vDir;
	fVector		color;
#if USE_OGL_LIGHTS
	unsigned		handle;
	fVector		fAttenuation;	// constant, linear quadratic
	fVector		fAmbient;
	fVector		fDiffuse;
#endif
	fVector		fSpecular;
	fVector		fEmissive;
	ubyte			bTransform;
	tDynLightInfo info;
	tShadowLightInfo	shadow;
} tDynLight;

//------------------------------------------------------------------------------

typedef struct tOglMaterial {
#if 0 //using global default values instead
	fVector		diffuse;
	fVector		ambient;
#endif
	fVector		specular;
	fVector		emissive;
	ubyte			shininess;
	ubyte			bValid;
	short			nLight;
} tOglMaterial;

//------------------------------------------------------------------------------

struct tActiveShaderLight;

typedef struct tShaderLight {
	fVector		vPosf [2];
	fix			xDistance;
	short			nVerts [4];
	int			nTarget;	//lit segment/face
	int			nFrame;
	ubyte			bShadow;
	ubyte			bLightning;
	ubyte			bExclusive;
	ubyte			bUsed [MAX_THREADS];
	struct tActiveShaderLight *activeLightsP [MAX_THREADS];
	tDynLightInfo info;
} tShaderLight;

//------------------------------------------------------------------------------

typedef struct tActiveShaderLight {
	short				nType;
	tShaderLight	*psl;
} tActiveShaderLight;

typedef struct tShaderLightIndex {
	short	nFirst;
	short	nLast;
	short	nActive;
	short	iVertex;
	short	iStatic;
	} tShaderLightIndex;

typedef struct tShaderLightData {
	tShaderLight			lights [MAX_OGL_LIGHTS];
	int						nLights;
	tActiveShaderLight	activeLights [MAX_THREADS][MAX_OGL_LIGHTS];
	tShaderLightIndex		index [2][MAX_THREADS];
	GLuint					nTexHandle;
} tShaderLightData;

//------------------------------------------------------------------------------

#define MAX_NEAREST_LIGHTS 32

typedef struct tHeadlightData {
	tDynLight*			pl [MAX_PLAYERS];
	tShaderLight*		psl [MAX_PLAYERS];
	fVector				pos [MAX_PLAYERS];
	fVector3				dir [MAX_PLAYERS];
	float					brightness [MAX_PLAYERS];
	int					nLights;
} tHeadlightData;

typedef struct tDynLightData {
	tDynLight			lights [MAX_OGL_LIGHTS];
	CArray<short>		nearestSegLights;		//the 8 nearest static lights for every CSegment
	CArray<short>		nearestVertLights;		//the 8 nearest static lights for every CSegment
	CArray<ubyte>		variableVertLights;	//the 8 nearest static lights for every CSegment
	CArray<short>		owners;
	short					nLights;
	short					nVariable;
	short					nDynLights;
	short					nVertLights;
	short					nHeadlights [MAX_PLAYERS];
	short					nSegment;
	tShaderLightData	shader;
	tHeadlightData		headlights;
	tOglMaterial		material;
	CFBO					fbo;
} tDynLightData;

extern int nMaxNearestLights [21];

//------------------------------------------------------------------------------

//Flickering light system
typedef struct tVariableLight {
	short				nSegment;
	short				nSide;
	uint	mask;     // determines flicker pattern
	fix				timer;    // time until next change
	fix				delay;    // time between changes
} tVariableLight;

#define MAX_FLICKERING_LIGHTS 1000

typedef struct tFlickerLightData {
	tVariableLight	lights [MAX_FLICKERING_LIGHTS];
	int				nLights;
} tFlickerLightData;

//------------------------------------------------------------------------------

typedef struct tLightData {
	int								nStatic;
	int								nCoronas;
	CArray<fix>						segDeltas;
	CArray<tLightDeltaIndex>	deltaIndices;
	CArray<tLightDelta>			deltas;
	CArray<ubyte>					subtracted;
	tDynLightData					dynamic;
	tFlickerLightData				flicker;
	CArray<fix>						dynamicLight;
	CArray<tRgbColorf>			dynamicColor;
	CArray<ubyte>					bGotDynColor;
	ubyte								bGotGlobalDynColor;
	ubyte								bStartDynColoring;
	ubyte								bInitDynColoring;
	tRgbColorf						globalDynColor;
	CArray<short>					vertices;
	CArray<sbyte>					vertexFlags;
	CArray<sbyte>					newObjects;
	CArray<sbyte>					objects;
	CArray<GLuint>					coronaQueries;
	CArray<GLuint>					coronaSamples;
} tLightData;

int operator- (tLightDeltaIndex* o, CArray<tLightDeltaIndex>& a) { return a.Index (o); }
int operator- (tLightDelta* o, CArray<tLightDelta>& a) { return a.Index (o); }

//------------------------------------------------------------------------------

typedef struct tShadowData {
	short						nLight;
	short						nLights;
	CArray<tShaderLight>	lights;
	short						nShadowMaps;
	CCamera					shadowMaps [MAX_SHADOW_MAPS];
	CObject					lightSource;
	tOOF_vector				vLightPos;
	vmsVector				vLightDir [MAX_SHADOW_LIGHTS];
	CArray<short>			objLights;
	ubyte						nFrame;	//flipflop for testing whether a light source's view has been rendered the current frame
} tShadowData;

//------------------------------------------------------------------------------

#include "morph.h"

typedef struct tMorphData {
	tMorphInfo	objects [MAX_MORPH_OBJECTS];
	fix			xRate;
} tMorphData;

//------------------------------------------------------------------------------

#define OGLTEXBUFSIZE (2048*2048*4)

typedef struct tOglData {
	GLubyte					buffer [OGLTEXBUFSIZE];
	CPalette					*palette;
	GLenum					nSrcBlend;
	GLenum					nDestBlend;
	float						zNear;
	float						zFar;
	fVector3					depthScale;
	struct {float x, y;}	screenScale;
	CFBO						drawBuffer;
	CFBO						glowBuffer;
	int						nPerPixelLights [8];
	float						lightRads [8];
	fVector					lightPos [8];
	int						bLightmaps;
	int						nHeadlights;
} tOglData;

//------------------------------------------------------------------------------

#define TERRAIN_GRID_MAX_SIZE   64
#define TERRAIN_GRID_SCALE      I2X (2*20)
#define TERRAIN_HEIGHT_SCALE    f1_0

typedef struct tTerrainRenderData {
	ubyte			*heightmap;
	fix			*lightmap;
	vmsVector	*points;
	CBitmap		*bmP;
	g3sPoint		saveRow [TERRAIN_GRID_MAX_SIZE];
	vmsVector	vStartPoint;
	tUVL			uvlList [2][3];
	int			bOutline;
	int			nGridW, nGridH;
	int			orgI, orgJ;
	int			nMineTilesDrawn;    //flags to tell if all 4 tiles under mine have drawn
} tTerrainRenderData;

//------------------------------------------------------------------------------

typedef struct tThrusterData {
	tFlightPath				path;
	float						fSpeed;
	short						nPulse;
	time_t					tPulse;
} tThrusterData;

//------------------------------------------------------------------------------

#define OBJS_PER_SEG        5
#define N_EXTRA_OBJ_LISTS   50

typedef struct tObjRenderListItem {
	short	nNextItem;
	short	nObject;
	fix	xDist;
} tObjRenderListItem;

typedef struct tObjRenderList {
	short 					ref [MAX_SEGMENTS_D2X];	//points to each segment's first render object list entry in renderObjs
	tObjRenderListItem	objs [MAX_OBJECTS_D2X];
	int						nUsed;
} tObjRenderList;

typedef struct tMineRenderData {
	vmsVector				viewerEye;
	short 					nSegRenderList [MAX_SEGMENTS_D2X];
	tFace*					pFaceRenderList [MAX_SEGMENTS_D2X * 6];
	tObjRenderList			renderObjs;
	int						nRenderSegs;
	ubyte 					bVisited [MAX_SEGMENTS_D2X];
	ubyte 					bVisible [MAX_SEGMENTS_D2X];
	ubyte 					bProcessed [MAX_SEGMENTS_D2X];		//whether each entry has been nProcessed
	ubyte 					nVisited;
	ubyte						nProcessed;
	ubyte						nVisible;
	short 					nSegDepth [MAX_SEGMENTS_D2X];		//depth for each seg in nRenderList
	int						lCntSave;
	int						sCntSave;
	ubyte						bObjectRendered [MAX_OBJECTS_D2X];
	ubyte						bRenderSegment [MAX_SEGMENTS_D2X];
	short						nRenderObjList [MAX_SEGMENTS_D2X+N_EXTRA_OBJ_LISTS][OBJS_PER_SEG];
	short						nRenderPos [MAX_SEGMENTS_D2X];
	int						nRotatedLast [MAX_VERTICES_D2X];
	ubyte						bCalcVertexColor [MAX_VERTICES_D2X];
	ushort					bAutomapVisited [MAX_SEGMENTS_D2X];
	ushort					bAutomapVisible [MAX_SEGMENTS_D2X];
	ushort					bRadarVisited [MAX_SEGMENTS_D2X];
	ubyte						bSetAutomapVisited;
} tMineRenderData;

//------------------------------------------------------------------------------

typedef struct tGameWindowData {
	int						x, y;
	int						w, h;
	int						wMax, hMax;
} tGameWindowData;

//------------------------------------------------------------------------------

typedef struct tVertColorData {
	int		bExclusive;
	int		bNoShadow;
	int		bDarkness;
	int		bMatEmissive;
	int		bMatSpecular;
	int		nMatLight;
	fVector	matAmbient;
	fVector	matDiffuse;
	fVector	matSpecular;
	fVector	colorSum;
	fVector3	vertNorm;
	fVector3	vertPos;
	fVector3	*pVertPos;
	float		fMatShininess;
	} tVertColorData;

typedef struct tFaceListItem {
	tFace					*faceP;
	short					nNextItem;
} tFaceListItem;

typedef struct tFaceListIndex {
	short						roots [MAX_WALL_TEXTURES * 3];
	short						tails [MAX_WALL_TEXTURES * 3];
	short						usedKeys [MAX_WALL_TEXTURES * 3];
	short						nUsedFaces;
	short						nUsedKeys;
	} tFaceListIndex;

typedef struct tRenderData {
	tColorData					color;
	int							transpColor;
	tFaceListIndex				faceIndex [2];
	tVertColorData				vertColor;
	tSphereData					shield;
	tSphereData					monsterball;
	CArray<tFaceListItem>	faceList;
	fix							xFlashEffect;
	fix							xTimeFlashLastPlayed;
	fVector*						vertP;
	CArray<fVector>			vertexList;
	tLightData					lights;
	tMorphData					morph;
	tShadowData					shadows;
	tOglData						ogl;
	tTerrainRenderData		terrain;
	tThrusterData				thrusters [MAX_PLAYERS];
	tMineRenderData			mine;
	tGameWindowData			window;
	fix							zMin;
	fix							zMax;
	double						dAspect;
	CFBO							glareBuffer;
	int							nTotalFaces;
	int							nTotalObjects;
	int							nTotalSprites;
	int							nTotalLights;
	int							nMaxLights;
	int							nColoredFaces;
	int							nStateChanges;
	int							nShaderChanges;
	float							fAttScale;
	ubyte							nPowerupFilter;
} tRenderData;

//------------------------------------------------------------------------------

typedef struct tSecretData {
	int			nReturnSegment;
	vmsMatrix	returnOrient;

} tSecretData;

//------------------------------------------------------------------------------

typedef struct tSlideSegs {
	short	nSegment;
	ubyte	nSides;
} tSlideSegs;

//------------------------------------------------------------------------------

#define MAX_SEGVIS_FLAGS	((MAX_SEGMENTS + 7) >> 3)
#define MAX_VERTVIS_FLAGS	((MAX_VERTICES + 7) >> 3)
#define SEGVIS_FLAGS			((gameData.segs.nSegments + 7) >> 3)
#define VERTVIS_FLAGS		((gameData.segs.nVertices + 7) >> 3)

#define USE_RANGE_ELEMENTS	0

//------------------------------------------------------------------------------

typedef struct tFaceRenderVertex {
	fVector3				vertex;
	fVector3				normal;
	tRgbaColorf			color;
	tTexCoord2f			texCoord;
	tTexCoord2f			ovlTexCoord;
	tTexCoord2f			lMapTexCoord;
	} tFaceRenderVertex;

typedef struct tFaceData {
	CArray<tFace>			faces;
	CArray<grsTriangle>	tris;
	CArray<fVector3>		vertices;
	CArray<fVector3>		normals;
	CArray<tTexCoord2f>	texCoord;
	CArray<tTexCoord2f>	ovlTexCoord;
	CArray<tTexCoord2f>	lMapTexCoord;
	CArray<tRgbaColorf>	color;
	CArray<ushort>			faceVerts;
	tFace*					slidingFaces;
#if USE_RANGE_ELEMENTS
	CArray<GLuint			vertIndex;
#endif
	GLuint					vboDataHandle;
	GLuint					vboIndexHandle;
	ubyte						*vertexP;
	ushort					*indexP;
	int						nVertices;
	int						iVertices;
	int						iNormals;
	int						iColor;
	int						iTexCoord;
	int						iOvlTexCoord;
	int						iLMapTexCoord;
	} tFaceData;

int operator- (grsTriangle* o, CArray<grsTriangle>& a) { return a.Index (o); }
int operator- (fVector3* o, CArray<fVector3>& a) { return a.Index (o); }
int operator- (tTexCoord2f* o, CArray<tTexCoord2f>& a) { return a.Index (o); }

typedef struct tSegList {
	int					nSegments;
	CArray<short>		segments;
} tSegList;

typedef struct tSegExtent {
	vmsVector			vMin;
	vmsVector			vMax;
	} tSegExtent;

typedef struct tSegmentData {
	int						nMaxSegments;
	CArray<vmsVector>		vertices;
	CArray<fVector>		fVertices;
	CArray<CSegment>		segments;
	CArray<tSegment2>		segment2s;
	CArray<xsegment>		xSegments;
	CArray<tSegFaces>		segFaces;
	CArray<g3sPoint>		points;
	tSegList					skybox;
#if CALC_SEGRADS
	CArray<fix>				segRads [2];
	CArray<tSegExtent>	extent;
#endif
	vmsVector				vMin;
	vmsVector				vMax;
	float						fRad;
	CArray<vmsVector>		segCenters [2];
	CArray<vmsVector>		sideCenters;
	CArray<ubyte>			bVertVis;
	CArray<ubyte>			bSegVis;
	int						nVertices;
	int						nFaceVerts;
	int						nLastVertex;
	int						nSegments;
	int						nLastSegment;
	int						nFaces;
	int						nTris;
	int						nLevelVersion;
	char						szLevelFilename [FILENAME_LEN];
	tSecretData				secret;
	CArray<tSlideSegs>	slideSegs;
	short						nSlideSegs;
	int						bHaveSlideSegs;
	tFaceData				faces;
} tSegmentData;

//------------------------------------------------------------------------------

class CWallData {
	public:
		CArray<tWall>				walls ; //[MAX_WALLS];
		CArray<tExplWall>			explWalls ; //[MAX_EXPLODING_WALLS];
		CArray<tActiveDoor>		activeDoors ; //[MAX_DOORS];
		CArray<tCloakingWall>	cloaking ; //[MAX_CLOAKING_WALLS];
		CArray<tWallClip>			anims [2]; //[MAX_WALL_ANIMS];
		CArray<int>					bitmaps ; //[MAX_WALL_ANIMS];
		int							nWalls;
		int							nOpenDoors; 
		int							nCloaking;
		int							nAnims [2];
		CArray<tWallClip>			animP;

	public:
		CWallData ();
		~CWallData () {}
};

//------------------------------------------------------------------------------

typedef struct tTriggerData {
	tTrigger					triggers [MAX_TRIGGERS];
	tTrigger					objTriggers [MAX_TRIGGERS];
	tObjTriggerRef			objTriggerRefs [MAX_OBJ_TRIGGERS];
	short						firstObjTrigger [MAX_OBJECTS_D2X];
	int						delay [MAX_TRIGGERS];
	int						nTriggers;
	int						nObjTriggers;
} tTriggerData;

//------------------------------------------------------------------------------

typedef struct tPowerupData {
	tPowerupTypeInfo		info [MAX_POWERUP_TYPES];
	int						nTypes;
} tPowerupData;

//------------------------------------------------------------------------------

typedef struct tObjTypeData {
	int						nTypes;
	sbyte						nType [MAX_OBJTYPE];
	sbyte						nId [MAX_OBJTYPE];  
	fix						nStrength [MAX_OBJTYPE];   
} tObjTypeData;

//------------------------------------------------------------------------------

typedef struct tSpeedBoostData {
	int						bBoosted;
	vmsVector				vVel;
	vmsVector				vMinVel;
	vmsVector				vMaxVel;
	vmsVector				vSrc;
	vmsVector				vDest;
} tSpeedBoostData;

//------------------------------------------------------------------------------

typedef struct tQuad {
	vmsVector			v [4];	//corner vertices
	vmsVector			n [2];	//normal, transformed normal
#if DBG
	time_t				t;
#endif
} tQuad;

typedef struct tBox {
	vmsVector			vertices [8];
	tQuad					faces [6];
#if 0
	tQuad					rotFaces [6];	//transformed faces
	short					nTransformed;
#endif
	} tBox;

typedef struct tHitbox {
	vmsVector			vMin;
	vmsVector			vMax;
	vmsVector			vSize;
	vmsVector			vOffset;
	tBox					box;
	vmsAngVec			angles;			//rotation angles
	short					nParent;			//parent hitbox
} tHitbox;

//------------------------------------------------------------------------------

typedef struct tShrapnel {
	vmsVector	vPos, vStartPos;
	vmsVector	vDir;
	vmsVector	vOffs;
	fix			xSpeed;
	fix			xBaseSpeed;
	fix			xLife;
	fix			xTTL;
	int			nSmoke;
	int			nTurn;
	int			nParts;
	int			d;
	double		fScale;
	time_t		tUpdate;
} tShrapnel;

class CShrapnel {
	private:
		tShrapnel	m_info;

	public:
		void Create (CObject* objP);
		void Destroy (void);
		void Move (void);
		void Draw (void);
		int Update (void);
};

class CShrapnelCloud : private CStack<CShrapnel> {
	public:
		~CShrapnelCloud () { Destroy (); }
		int Create (CObject* objP);
		uint Update (void);
		void Draw (void);
		void Destroy (void);
	};

class CShrapnelManager : private CArray<CShrapnelCloud> {
	public:
		int Create (CObject *objP);
		void Draw (CObject *objP);
		int Update (CObject *objP);
		void Move (CObject *objP);
		void Destroy (CObject *objP); 
		void DoFrame (void);
	};

extern CShrapnelManager shrapnelManager;

//------------------------------------------------------------------------------

#define MAX_WEAPONS	100

typedef struct tObjectViewData {
	vmsMatrix			mView;
	int					nFrame;
} tObjectViewData;

typedef struct tLightObjId {
	short					nObject;
	int					nSignature;
} tLightObjId;

typedef struct tGuidedMissileInfo {
	CObject				*objP;
	int					nSignature;
} tGuidedMissileInfo;

typedef struct tObjLists {
	tObjListRef				all;
	tObjListRef				players;
	tObjListRef				robots;
	tObjListRef				powerups;
	tObjListRef				weapons;
	tObjListRef				effects;
	tObjListRef				lights;
	tObjListRef				actors;
	tObjListRef				statics;
} tObjLists;

typedef struct tObjectData {
	CArray<tObjTypeData>		types;
	CArray<CObject>			objects;
	tObjLists					lists;
	CArray<short>				freeList;
	CArray<short>				parentObjs;
	CArray<tObjectRef>		childObjs;
	CArray<short>				firstChild;
	CArray<tLightObjId>		lightObjs;
	CArray<CObject>			init;
	CArray<tObjDropInfo>		dropInfo;
	CArray<tSpeedBoostData>	speedBoost;
	CArray<vmsVector>			vRobotGoals;
	CArray<fix>					xLastAfterburnerTime;
	CArray<fix>					xLight;
	CArray<int>					nLightSig;
	tFaceColor					color;
	short							nFirstDropped;
	short							nLastDropped;
	short							nFreeDropped;
	short							nDropped;
	tGuidedMissileInfo		guidedMissile [MAX_PLAYERS];
	CObject*						consoleP;
	CObject*						viewerP;
	CObject*						trackGoals [2];
	CObject*						missileViewerP;
	CObject*						deadPlayerCamera;
	CObject*						endLevelCamera;
	int							nObjects;
	int							nLastObject [2];
	int							nObjectLimit;
	int							nMaxUsedObjects;
	int							nNextSignature;
	int							nChildFreeList;
	int							nDrops;
	int							nDeadControlCenter;
	int							nVertigoBotFlags;
	CArray<short>				nHitObjects;
	tPowerupData				pwrUp;
	ubyte							collisionResult [MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];
	CArray<tObjectViewData>	viewData;
	ubyte							bIsMissile [MAX_WEAPONS];
	ubyte							bIsWeapon [MAX_WEAPONS];
	ubyte							bIsSlowWeapon [MAX_WEAPONS];
	short							idToOOF [MAX_WEAPONS];
	ubyte							bWantEffect [MAX_OBJECTS_D2X];
	int							nFrameCount;
} tObjectData;

#define PLAYER_LIGHTNINGS	1
#define ROBOT_LIGHTNINGS	2
#define MISSILE_LIGHTNINGS	4
#define EXPL_LIGHTNINGS		8
#define DESTROY_LIGHTNINGS	16
#define SHRAPNEL_SMOKE		32
#define DESTROY_SMOKE		64

//------------------------------------------------------------------------------

typedef struct tFVISideData {
	int					bCache;
	int					vertices [6];
	int					nFaces;
	short					nSegment;
	short					nSide;
	short					nType;
} tFVISideData;

typedef struct tPhysicsData {
	CArray<short>		ignoreObjs;
	tFVISideData		side;
	fix					xTime;
	fix					xAfterburnerCharge;
	fix					xBossInvulDot;
	vmsVector			playerThrust;
} tPhysicsData;

//------------------------------------------------------------------------------

typedef struct tPOF_face {
	short					nVerts;
	CArray<short>		verts;
	vmsVector			vCenter;
	vmsVector			vNorm;
	vmsVector			vRotNorm;
	tOOF_vector			vNormf;
	tOOF_vector			vCenterf;
	float					fClipDist;
	ubyte					bFacingLight :1;
	ubyte					bFrontFace :1;
	ubyte					bGlow :1;
	ubyte					bTest :1;
	ubyte					bIgnore :1;
	short					nAdjFaces;
} tPOF_face;

typedef struct tPOF_faceList {
	short					nFaces;
	CArray<tPOF_face>	faces;
} tPOF_faceList;

typedef struct tPOF_faceRef {
	short						nFaces;
	CArray<tPOF_face*>	*faceP;
} tPOF_faceRef;

typedef struct tPOFSubObject {
	tPOF_faceList		faces;
	tPOF_faceRef		litFaces;	//submodel faces facing the current light source
	vmsVector			vPos;
	vmsAngVec			vAngles;
	float					fClipDist;
	short					nParent;
	CArray<short>		adjFaces;
	short					nRenderFlipFlop;
	short					bCalcClipDist;
} tPOFSubObject;

typedef struct tPOF_subObjList {
	short							nSubObjs;
	CArray<tPOFSubObject>	subObjs;
} tPOF_subObjList;

typedef struct tPOFObject {
	tPOF_subObjList		subObjs;
	short						nVerts;
	CArray<vmsVector>		verts;
	CArray<tOOF_vector>	vertsf;
	CArray<float>			fClipDist;
	CArray<ubyte>			vertFlags;
	CArray<g3sNormal>		vertNorms;
	CArray<vmsVector>		vCenter;
	CArray<vmsVector>		vRotVerts;
	tPOF_faceList			faces;
	tPOF_faceRef			litFaces;
	short						nAdjFaces;
	CArray<short>			adjFaces;
	CArray<short>			*faceVerts;
	CArray<short>			*vertMap;
	short						iSubObj;
	short						iVert;
	short						iFace;
	short						iFaceVert;
	char						nState;
	ubyte						nVertFlag;
} tPOFObject;

//	-----------------------------------------------------------------------------

#define G3_BUFFER_OFFSET(_i)	(GLvoid *) ((char *) NULL + (_i))

typedef struct tG3RenderVertex {
	fVector3					vertex;
	fVector3					normal;
	tRgbaColorf				color;
	tTexCoord2f				texCoord;
	} tG3RenderVertex;

typedef struct tG3ModelVertex {
	tTexCoord2f				texCoord;
	tRgbaColorf				renderColor;
	fVector3					vertex;
	fVector3					normal;
	tRgbaColorf				baseColor;
	short						nIndex;
	char						bTextured;
} tG3ModelVertex;

typedef struct tG3ModelFace {
	vmsVector				vNormal;
	short						nVerts;
	short						nBitmap;
	short						nIndex;
	short						nId;
	ubyte						nSubModel;
	ubyte						bGlow :1;
	ubyte						bThruster :1;
} tG3ModelFace;

typedef struct tG3SubModel {
#if DBG
	char						szName [256];
#endif
	vmsVector				vOffset;
	vmsVector				vCenter;
	fVector3					vMin;
	fVector3					vMax;
	CArray<tG3ModelFace>	faces;
	short						nParent;
	short						nFaces;
	short						nIndex;
	short						nBitmap;
	short						nHitbox;
	int						nRad;
	ushort					nAngles;
	ubyte						bRender :1;
	ubyte						bGlow :1;
	ubyte						bThruster :1;
	ubyte						bWeapon :1;
	ubyte						bBullets :1;
	ubyte						nType :2;
	char						nGunPoint;
	char						nGun;
	char						nBomb;
	char						nMissile;
	char						nWeaponPos;
	ubyte						nFrames;
	ubyte						iFrame;
	time_t					tFrame;
} tG3SubModel;

typedef struct tG3VertNorm {
	fVector3					vNormal;
	ubyte						nVerts;
} tG3VertNorm;

typedef struct tG3Model {
	CArray<CBitmap>			textures;
	int							teamTextures [8];
	CArray<fVector3>			verts;
	CArray<fVector3>			vertNorms;
	CArray<tFaceColor>		color;
	CArray<tG3ModelVertex>	faceVerts;
	CArray<tG3ModelVertex>	sortedVerts;
	CArray<ubyte>				vbData;
	CArray<tTexCoord2f>		vbTexCoord;
	CArray<tRgbaColorf>		vbColor;
	CArray<fVector3>			vbVerts;
	CArray<fVector3>			vbNormals;
	CArray<tG3SubModel>		subModels;
	CArray<tG3ModelFace>		faces;
	CArray<tG3RenderVertex>	vertBuf [2];
	CArray<short>				index [2];
	short							nGunSubModels [MAX_GUNS];
	float							fScale;
	short							nType; //-1: custom mode, 0: default model, 1: alternative model, 2: hires model
	short							nFaces;
	short							iFace;
	short							nVerts;
	short							nFaceVerts;
	short							iFaceVert;
	short							nSubModels;
	short							nTextures;
	short							iSubModel;
	short							bHasTransparency;
	short							bValid;
	short							bRendered;
	short							bBullets;
	vmsVector					vBullets;
	GLuint						vboDataHandle;
	GLuint						vboIndexHandle;
} tG3Model;

//------------------------------------------------------------------------------

#define MAX_POLYGON_VERTS 1000

class CRobotData {
	public:
		char*						robotNames [MAX_ROBOT_TYPES][ROBOT_NAME_LENGTH];
		CArray<tRobotInfo>	info [2]; //[MAX_ROBOT_TYPES];
		CArray<tRobotInfo>	defaultInfo ; //[MAX_ROBOT_TYPES];
		CArray<tJointPos>		joints ; //[MAX_ROBOT_JOINTS];
		CArray<tJointPos>		defaultJoints ; //[MAX_ROBOT_JOINTS];
		int						nJoints;
		int						nDefaultJoints;
		int						nCamBotId;
		int						nCamBotModel;
		int						nTypes [2];
		int						nDefaultTypes;
		int						bReplacementsLoaded;
		CArray<tRobotInfo>	infoP;
		CArray<tPOFObject>	pofData;
	public:
		CRobotData ();
		~CRobotData () {}
};

#define D1ROBOT(_id)		(gameStates.app.bD1Mission && ((_id) < gameData.bots.nTypes [1]))
#define ROBOTINFO(_id)	gameData.bots.info [D1ROBOT (_id)][_id]

//------------------------------------------------------------------------------

#if USE_OPENAL

typedef struct tOpenALData {
	ALCdevice			*device;
	ALCcontext			*context;
} tOpenALData;

#endif

class CSoundData {
	public:
		CArray<ubyte>			data [2];
		CArray<tDigiSound>	sounds [2]; //[MAX_SOUND_FILES];
		int						nSoundFiles [2];
		CArray<tDigiSound>	soundP;
#if USE_OPENAL
		tOpenALData				openAL;
#endif
			
	public:
		CSoundData ();
		~CSoundData () {}
};

//------------------------------------------------------------------------------

#define N_COCKPIT_BITMAPS 6
#define D1_N_COCKPIT_BITMAPS 4

class CTextureData {
	public:
		CArray<tBitmapFile>		bitmapFiles [2]; //[MAX_BITMAP_FILES];
		CArray<sbyte>				bitmapFlags [2]; //[MAX_BITMAP_FILES];
		CArray<CBitmap>			bitmaps [2]; //[MAX_BITMAP_FILES];
		CArray<CBitmap>			altBitmaps [2]; //[MAX_BITMAP_FILES];
		CArray<CBitmap>			addonBitmaps ; //[MAX_ADDON_BITMAP_FILES];
		CArray<ushort>				bitmapXlat ; //[MAX_BITMAP_FILES];
		CArray<alias>				aliases ; //[MAX_ALIASES];
		CArray<tBitmapIndex>		bmIndex [2]; //[MAX_TEXTURES];
		CArray<tBitmapIndex>		objBmIndex ; //[MAX_OBJ_BITMAPS];
		CArray<short>				textureIndex [2]; //[MAX_BITMAP_FILES];
		CArray<ushort>				objBmIndexP ; //[MAX_OBJ_BITMAPS];
		CArray<tBitmapIndex>		cockpitBmIndex; //[N_COCKPIT_BITMAPS];
		CArray<tRgbColorf>		bitmapColors ; //[MAX_BITMAP_FILES];
		int							nBitmaps [2];
		int							nObjBitmaps;
		int							bPageFlushed;
		CArray<tTexMapInfo>		tMapInfo [2] ; //[MAX_TEXTURES];
		int							nExtraBitmaps;
		int							nAliases;
		int							nHamFileVersion;
		int							nTextures [2];
		int							nFirstMultiBitmap;
		CArray<tBitmapFile>		bitmapFileP;
		CArray<CBitmap>			bitmapP;
		CArray<CBitmap>			altBitmapP;
		CArray<tBitmapIndex>		bmIndexP;
		CArray<tTexMapInfo>		tMapInfoP;
		CArray<ubyte>				rleBuffer;
		int							brightness [MAX_WALL_TEXTURES];
		int							defaultBrightness [2][MAX_WALL_TEXTURES];

	public:
		CTextureData ();
		~CTextureData () {}
};

//------------------------------------------------------------------------------

class CEffectData {
	public:
		CArray<tEffectClip>	effects [2]; //[MAX_EFFECTS];
		CArray<tVideoClip>	vClips [2]; //[VCLIP_MAXNUM];
		int						nEffects [2];
		int 						nClips [2];
		CArray<tEffectClip>	effectP;
		CArray<tVideoClip>	vClipP;

	public:
		CEffectData ();
		~CEffectData () {}
} tEffectData;

int operator- (tEffectClip* o, CArray<tEffectClip>& a) { return a.Index (o); }
int operator- (tVideoClip* o, CArray<tVideoClip>& a) { return a.Index (o); }

typedef struct tShipData {
	tPlayerShip			only;
	tPlayerShip			*player;
} tShipData;

//------------------------------------------------------------------------------

typedef struct tFlagData {
	tBitmapIndex		bmi;
	tVideoClip			*vcP;
	tVClipInfo			vci;
	tFlightPath			path;
} tFlagData;

//------------------------------------------------------------------------------

class CPigData {
	public:
		CTextureData		tex;
		CSoundData			sound;
		tShipData			ship;
		tFlagData			flags [2];

	public:
		CPigData () {}
		~CPigData () {}
};

//------------------------------------------------------------------------------

#include "laser.h"

typedef struct tMuzzleData {
	int				queueIndex;
	tMuzzleInfo		info [MUZZLE_QUEUE_MAX];
} tMuzzleData;

#include "weapon.h"

#define GATLING_DELAY	700

typedef struct tFiringData {
	int					nStart;
	int					nStop;
	int					nDuration;
	int					bSound;
	int					bSpeedUp;
	} tFiringData;

typedef struct tWeaponData {
	sbyte						nPrimary;
	sbyte						nSecondary;
	sbyte						nOverridden;
	sbyte						bTripleFusion;
	tFiringData				firing [2];
	int						nTypes [2];
	tWeaponInfo				info [MAX_WEAPON_TYPES];
	tD1WeaponInfo			infoD1 [D1_MAX_WEAPON_TYPES];
	CArray<tRgbaColorf>	color;
	ubyte						bLastWasSuper [2][MAX_PRIMARY_WEAPONS];
} tWeaponData;

#define bLastPrimaryWasSuper (gameData.weapons.bLastWasSuper [0])
#define bLastSecondaryWasSuper (gameData.weapons.bLastWasSuper [1])

//------------------------------------------------------------------------------

#include "polyobj.h"

#define OOF_PYRO			0
#define OOF_MEGA			1

#define MAX_HITBOXES		100

typedef struct tModelHitboxes {
	ubyte					nHitboxes;
	tHitbox				hitboxes [MAX_HITBOXES + 1];
#if DBG
	vmsVector			vHit;
	time_t				tHit;
#endif
} tModelHitboxes;

typedef struct tModelThrusters {
	vmsVector			vPos [2];
	vmsVector			vDir [2];
	float					fSize;
	ushort				nCount;
	} tModelThrusters;

typedef struct tGunInfo {
	int					nGuns;
	vmsVector			vGunPoints [MAX_GUNS];
	} tGunInfo;

typedef struct tModelSphere {
	short					nSubModels;
	short					nFaces;
	short					nFaceVerts;
	fix					xRads [3];
	vmsVector			vOffsets [3];
} tModelSphere;

typedef struct tModelData {
	int					nLoresModels;
	int					nHiresModels;
	tASEModel			aseModels [2][MAX_POLYGON_MODELS];
	tOOFObject			oofModels [2][MAX_POLYGON_MODELS];
	tPOFObject			pofData [2][2][MAX_POLYGON_MODELS];
	ubyte					bHaveHiresModel [MAX_POLYGON_MODELS];
	tPolyModel			polyModels [MAX_POLYGON_MODELS];
	tPolyModel			defPolyModels [MAX_POLYGON_MODELS];
	tPolyModel			altPolyModels [MAX_POLYGON_MODELS];
	tOOFObject*			modelToOOF [2][MAX_POLYGON_MODELS];
	tASEModel*			modelToASE [2][MAX_POLYGON_MODELS];
	tPolyModel*			modelToPOL [MAX_POLYGON_MODELS];
	int					nPolyModels;
	int					nDefPolyModels;
	g3sPoint				polyModelPoints [MAX_POLYGON_VERTS];
	fVector				fPolyModelVerts [MAX_POLYGON_VERTS];
	CBitmap*				textures [MAX_POLYOBJ_TEXTURES];
	tBitmapIndex		textureIndex [MAX_POLYOBJ_TEXTURES];
	int					nSimpleModelThresholdScale;
	int					nMarkerModel;
	int					nCockpits;
	int					nDyingModels [MAX_POLYGON_MODELS];
	int					nDeadModels [MAX_POLYGON_MODELS];
	tModelHitboxes		hitboxes [MAX_POLYGON_MODELS];
	tModelThrusters	thrusters [MAX_POLYGON_MODELS];
	tG3Model				g3Models [2][MAX_POLYGON_MODELS];
	vmsVector			offsets [MAX_POLYGON_MODELS];
	tGunInfo				gunInfo [MAX_POLYGON_MODELS];
	tModelSphere		spheres [MAX_POLYGON_MODELS];
	vmsVector			vScale;
	int					nLightScale;
} tModelData;

//------------------------------------------------------------------------------

typedef struct tAutoNetGame {
	char					szPlayer [9];		//tPlayer profile name
	char					szFile [FILENAME_LEN];
	char					szMission [13];
	char					szName [81];		//game name
	int					nLevel;
	ubyte					ipAddr [4];
	int					nPort;
	ubyte					uConnect;
	ubyte					uType;
	ubyte					bHost;
	ubyte					bTeam;				// team game?
	ubyte					bValid;
} tAutoNetGame;

typedef struct tLeftoverPowerup {
	CObject				*spitterP;
	ubyte					nCount;
	ubyte					nType;
} tLeftoverPowerup;

typedef struct tWeaponState {
	char						nMissiles;
	char						nPrimary;
	char						nSecondary;
	char						bQuadLasers;
	tFiringData				firing [2];
	char						nLaserLevel;
	char						bTripleFusion;
	char						nMslLaunchPos;
	fix						xMslFireTime;
	} tWeaponState;

typedef struct tMultiplayerData {
	int 								nPlayers;				
	int								nMaxPlayers;
	int 								nLocalPlayer;				
	int								nPlayerPositions;
	int								bMoving;
	tPlayer							players [MAX_PLAYERS + 4];  
	tObjPosition					playerInit [MAX_PLAYERS];
	short								nVirusCapacity [MAX_PLAYERS];
	int								nLastHitTime [MAX_PLAYERS];
	tWeaponState					weaponStates [MAX_PLAYERS];
	char								bWasHit [MAX_PLAYERS];
	int								bulletEmitters [MAX_PLAYERS];
	int								gatlingSmoke [MAX_PLAYERS];
	tPulseData						spherePulse [MAX_PLAYERS];
	ubyte								powerupsInMine [MAX_POWERUP_TYPES];
	ubyte								powerupsOnShip [MAX_POWERUP_TYPES];
	ubyte								maxPowerupsAllowed [MAX_POWERUP_TYPES];
	CArray<tLeftoverPowerup>	leftoverPowerups;
	tAutoNetGame					autoNG;
	fix								xStartAbortMenuTime;
} tMultiplayerData;

#include "multi.h"

//------------------------------------------------------------------------------

typedef struct tMultiCreateData {
	int					nObjNums [MAX_NET_CREATE_OBJECTS];
	int					nLoc;
} tMultiCreateData;

typedef struct tMultiLaserData {
	int					bFired;
	int					nGun;
	int					nFlags;
	int					nLevel;
	short					nTrack;
} tMultiLaserData;


typedef struct tMultiMsgData {
	char					bSending;
	char					bDefining;
	int					nIndex;
	char					szMsg [MAX_MESSAGE_LEN];
	char					szMacro [4][MAX_MESSAGE_LEN];
	char					buf [MAX_MULTI_MESSAGE_LEN+4];            // This is where multiplayer message are built
	int					nReceiver;
} tMultiMsgData;

typedef struct tMultiMenuData {
	char					bInvoked;
	char					bLeave;
} tMultiMenuData;

typedef struct tMultiKillData {
	char					pFlags [MAX_NUM_NET_PLAYERS];
	int					nSorted [MAX_NUM_NET_PLAYERS];
	short					matrix [MAX_NUM_NET_PLAYERS][MAX_NUM_NET_PLAYERS];
	short					nTeam [2];
	char					bShowList;
	fix					xShowListTimer;
} tMultiKillData;

typedef struct tMultiGameData {
	int					nWhoKilledCtrlcen;
	char					bShowReticleName;
	char					bIsGuided;
	char					bQuitGame;
	tMultiCreateData	create;
	tMultiLaserData	laser;
	tMultiMsgData		msg;
	tMultiMenuData		menu;
	tMultiKillData		kills;
	tMultiRobotData	robots;
	CArray<short>		remoteToLocal;  // Remote CObject number for each local CObject
	CArray<short>		localToRemote;
	CArray<sbyte>		nObjOwner;   // Who created each CObject in my universe, -1 = loaded at start
	int					bGotoSecret;
	int					nTypingTimeout;
} tMultiGameData;

//------------------------------------------------------------------------------

#define LEVEL_NAME_LEN 36       //make sure this is a multiple of 4!

#include "mission.h"

typedef struct tMissionData {
	char					szCurrentLevel [LEVEL_NAME_LEN];
	int					nSecretLevels;
	int					nLastLevel;
	int					nCurrentLevel;
	int					nNextLevel;
	int					nLastSecretLevel;
	int					nLastMission;
	int					nEnteredFromLevel;
	int					nEnhancedMission;
	int					nCurrentMission;
	int					nBuiltinMission;
	int					nD1BuiltinMission;
	int					nBuiltinHogSize;
	int					nD1BuiltinHogSize;
	char					szBuiltinMissionFilename [9];
	char					szD1BuiltinMissionFilename [9];
	char					szBriefingFilename [13];
	char					szEndingFilename [13];
	tMsnListEntry		list [MAX_MISSIONS + 1];
	char					szLevelNames [MAX_LEVELS_PER_MISSION][FILENAME_LEN];
	char					szSecretLevelNames [MAX_SECRET_LEVELS_PER_MISSION][FILENAME_LEN];
	int					secretLevelTable [MAX_SECRET_LEVELS_PER_MISSION];
	char					szSongNames [MAX_LEVELS_PER_MISSION][FILENAME_LEN];
} tMissionData;

//------------------------------------------------------------------------------

#define N_MAX_ROOMS	128

typedef struct tEntropyData {
	char	nRoomOwners [N_MAX_ROOMS];
	char	nTeamRooms [3];
	int   nTotalRooms;
} tEntropyData;

extern tEntropyData entropyData;

typedef struct tCountdownData {
	fix					nTimer;
	int					nSecsLeft;
	int					nTotalTime;
} tCountdownData;

//------------------------------------------------------------------------------

#define NUM_MARKERS         16
#define MARKER_MESSAGE_LEN  40

typedef struct tMarkerData {
	vmsVector			point [NUM_MARKERS];		//these are only used in multi.c, and I'd get rid of them there, but when I tried to do that once, I caused some horrible bug. -MT
	char					szMessage [NUM_MARKERS][MARKER_MESSAGE_LEN];
	char					nOwner [NUM_MARKERS][CALLSIGN_LEN+1];
	short					objects [NUM_MARKERS];
	short					viewers [2];
	int					nHighlight;
	float					fScale;
	ubyte					nDefiningMsg;
	char					szInput [40];
	int					nIndex;
	int					nCurrent;
	int					nLast;
} tMarkerData;

//------------------------------------------------------------------------------

typedef struct tTimeData {
	fix					xFrame;	//  since last frame, in seconds
	fix					xRealFrame;
	fix					xGame;	//	 in game, in seconds
	fix					xLast;
	int					tLast;
	fix					xSlack;
	fix					xStarted;
	fix					xStopped;
	int					nPaused;
	int					nStarts;
	int					nStops;
} tTimeData;

//------------------------------------------------------------------------------

typedef enum {
	rtSound,
	rtStaticVertLight,
	rtComputeFaceLight,
	rtInitSegZRef,
	rtSortSegZRef,
	rtTransparency,
	rtEffects,
	rtPolyModel,
	rtTaskCount,
	rtLightmap
} tRenderTask;

typedef struct tApplicationData {
	int					nFrameCount;
	int					nMineRenderCount;
	int					nGameMode;
	int					bGamePaused;
	uint					nStateGameId;
	uint					semaphores [4];
	int					nLifetimeChecksum;
	int					bUseMultiThreading [rtTaskCount];
} tApplicationData;

//------------------------------------------------------------------------------

#if PROFILING

typedef enum tProfilerTags {
	ptFrame,
	ptRenderMine,
	ptBuildSegList,
	ptBuildObjList,
	ptRenderObjects,
	ptRenderObjectsFast,
	ptLighting,
	ptRenderPass,
	ptSegmentLighting,
	ptVertexLighting,
	ptPerPixelLighting,
	ptRenderFaces,
	ptRenderStates,
	ptShaderStates,
	ptTranspPolys,
	ptTransform,
	ptVertexColor,
	ptFaceList,
	ptAux,
	ptTagCount
	} tProfilerTags;

typedef struct tProfilerData {
	time_t				t [ptTagCount];
} tProfilerData;

#define PROF_START		time_t	tProf = clock ();
#define PROF_END(_tag)	(gameData.profiler.t [_tag]) += clock () - tProf;

#else

#define PROF_START	
#define PROF_END(_tAcc)

#endif

//------------------------------------------------------------------------------

#define MAX_SEGS_VISITED			1000
#define MAX_BOSS_COUNT				50
#define MAX_BOSS_TELEPORT_SEGS	MAX_SEGS_VISITED
#define NUM_D2_BOSSES				8
#define BOSS_CLOAK_DURATION		(F1_0*7)
#define BOSS_DEATH_DURATION		(F1_0*6)

typedef struct tBossData {
	short					nTeleportSegs;
	short					teleportSegs [MAX_BOSS_TELEPORT_SEGS];
	short					nGateSegs;
	short					gateSegs [MAX_BOSS_TELEPORT_SEGS];
	fix					nDyingStartTime;
	fix					nHitTime;
	fix					nCloakStartTime;
	fix					nCloakEndTime;
	fix					nCloakDuration;
	fix					nCloakInterval;
	fix					nLastTeleportTime;
	fix					nTeleportInterval;
	fix					nLastGateTime;
	fix					nGateInterval;
#if DBG
	fix					xPrevShields;
#endif
	int					bHitThisFrame;
	int					bHasBeenHit;
	int					nObject;
	short					nDying;
	sbyte					bDyingSoundPlaying;
} tBossData;

//------------------------------------------------------------------------------

#include "reactor.h"

typedef struct tReactorStates {
	int					nObject;
	int					bHit;
	int					bSeenPlayer;
	int					nNextFireTime;
	int					nDeadObj;
	int					nStrength;
	fix					xLastVisCheckTime;
	vmsVector			vGunPos [MAX_CONTROLCEN_GUNS];
	vmsVector			vGunDir [MAX_CONTROLCEN_GUNS];
} tReactorStates;

typedef struct tReactorData {
	tReactorProps		props [MAX_REACTORS];
	tReactorTriggers	triggers;
	tReactorStates		states [MAX_BOSS_COUNT];
	tCountdownData		countdown;
	int					nReactors;
	int					nStrength;
	int					bPresent;
	int					bDisabled;
	int					bDestroyed;
} tReactorData;

//------------------------------------------------------------------------------

#include "ai.h"

typedef struct tAIData {
	int						bInitialized;
	int						nOverallAgitation;
	int						bEvaded;
	int						bEnableAnimation;
	int						bInfoEnabled;
	vmsVector				vHitPos;
	int						nHitType;
	int						nHitSeg;
	tFVIData					hitData;
	short						nBelievedPlayerSeg;
	vmsVector				vBelievedPlayerPos;
	vmsVector				vLastPlayerPosFiredAt;
	fix						nDistToLastPlayerPosFiredAt;
	CArray<tAILocalInfo>	localInfo;
	tAICloakInfo			cloakInfo [MAX_AI_CLOAK_INFO];
	tPointSeg				pointSegs [MAX_POINT_SEGS];
	tPointSeg*				freePointSegs;
	int						nAwarenessEvents;
	int						nMaxAwareness;
	fix						xDistToPlayer;
	vmsVector				vVecToPlayer;
	vmsVector				vGunPoint;
	int						nPlayerVisibility;
	int						bObjAnimates;
	int						nLastMissileCamera;
	tAwarenessEvent		awarenessEvents [MAX_AWARENESS_EVENTS];
} tAIData;

//------------------------------------------------------------------------------

typedef struct tSatelliteData {
	CBitmap			bmInstance;
	CBitmap			*bmP;
	vmsVector			vPos;
	vmsVector			vUp;
} tSatelliteData;

typedef struct tStationData {
	CBitmap			*bmP;
	CBitmap			**bmList [1];
	vmsVector			vPos;
	int					nModel;
} tStationData;

typedef struct tTerrainData {
	CBitmap			bmInstance;
	CBitmap			*bmP;
} tTerrainData;

typedef struct tExitData {
	int					nModel;
	int					nDestroyedModel;
	vmsVector			vMineExit;
	vmsVector			vGroundExit;
	vmsVector			vSideExit;
	vmsMatrix			mOrient;
	short					nSegNum;
	short					nTransitSegNum;
} tExitData;

typedef struct tEndLevelData {
	tSatelliteData		satellite;
	tStationData		station;
	tTerrainData		terrain;
	tExitData			exit;
	fix					xCurFlightSpeed;
	fix					xDesiredFlightSpeed;
} tEndLevelData;

//------------------------------------------------------------------------------

#include "songs.h"

typedef struct tUserMusicData {
	int					nLevelSongs;
	int					nCurrentSong;
	int					bMP3;
	char					**pszLevelSongs;
	char					szIntroSong [FILENAME_LEN];
	char					szBriefingSong [FILENAME_LEN];
	char					szCreditsSong [FILENAME_LEN];
	char					szMenuSong [FILENAME_LEN];
} tUserMusicData;

//------------------------------------------------------------------------------

typedef struct tSongData {
	tSongInfo			info [MAX_NUM_SONGS];
	int					bInitialized;
	int					nTotalSongs;
	int					nSongs [2];
	int					nFirstLevelSong [2];
	int					nLevelSongs [2];
	int					nD1EndLevelSong;
	int					bPlaying;
	time_t				tStart;
	time_t				tSlowDown;
	time_t				tPos;
	tUserMusicData		user;
} tSongData;

//------------------------------------------------------------------------------

typedef struct tMenuData {
	int					bValid;
	uint		tinyColors [2][2];
	uint		warnColor;
	uint		keyColor;
	uint		tabbedColor;
	uint		helpColor;
	uint		colorOverride;
	int					nLineWidth;
	ubyte					alpha;
} tMenuData;

//------------------------------------------------------------------------------

#define MAX_FUEL_CENTERS    500
#define MAX_ROBOT_CENTERS   100
#define MAX_EQUIP_CENTERS   100

#include "fuelcen.h"

typedef struct tEnergySpark {
	short				nProb;
	char				nFrame;
	char				bRendered;
	fix				xSize;
	time_t			tRender;
	time_t			tCreate;
	vmsVector		vPos;
	vmsVector		vDir;
	} tEnergySpark;

typedef struct tSegmentSparks {
	tEnergySpark	*sparks;
	short				nMaxSparks;
	short				bUpdate;
	} tSegmentSparks;

typedef struct tMatCenData {
	fix				xFuelRefillSpeed;
	fix				xFuelGiveAmount;
	fix				xFuelMaxAmount;
	tFuelCenInfo	fuelCenters [MAX_FUEL_CENTERS];
	tMatCenInfo		botGens [MAX_ROBOT_CENTERS];
	tMatCenInfo		equipGens [MAX_EQUIP_CENTERS];
	int				nFuelCenters;
	int				nBotCenters;
	int				nEquipCenters;
	int				nRepairCenters;
	fix				xEnergyToCreateOneRobot;
	int				origStationTypes [MAX_FUEL_CENTERS];
	tSegmentSparks	sparks [2][MAX_FUEL_CENTERS];	//0: repair, 1: fuel center
	short				sparkSegs [2 * MAX_FUEL_CENTERS];
	short				nSparkSegs;
	CSegment			*playerSegP;
} tMatCenData;

//------------------------------------------------------------------------------

typedef struct tDemoData {
	int				bAuto;
	char				fnAuto [FILENAME_LEN];

	CArray<sbyte>	bWasRecorded;
	CArray<sbyte>	bViewWasRecorded;
	sbyte				bRenderingWasRecorded [32];

	char				callSignSave [CALLSIGN_LEN+1];
	int				nVersion;
	int				nState;
	int				nVcrState;
	int				nStartFrame;
	uint				nSize;
	uint				nWritten;
	int				nGameMode;
	int				nOldCockpit;
	sbyte				bNoSpace;
	sbyte				bEof;
	sbyte				bInterpolate;
	sbyte				bPlayersCloaked;
	sbyte				bWarningGiven;
	sbyte				bCtrlcenDestroyed;
	int				nFrameCount;
	short				nFrameBytesWritten;
	fix				xStartTime;
	fix				xPlaybackTotal;
	fix				xRecordedTotal;
	fix				xRecordedTime;
	sbyte				nPlaybackStyle;
	sbyte				bFirstTimePlayback;
	fix				xJasonPlaybackTotal;
	int				bUseShortPos;
	int				bFlyingGuided;
} tDemoData;

//------------------------------------------------------------------------------

typedef struct tParticleData {
	CArray<time_t>		objExplTime;
	int					nLastType;
	int					bAnimate;
	int					bStencil;
} tParticleData;

//------------------------------------------------------------------------------

#define GUIDEBOT_NAME_LEN 9

typedef struct tEscortData {
	int				nMaxLength;
	int				nObjNum;
	int				bSearchingMarker;
	int				nLastKey;
	int				nKillObject;
	int				nGoalObject;
	int				nSpecialGoal;
	int				nGoalIndex;
	int				bMayTalk;
	int				bMsgsSuppressed;
	fix				xSorryTime;
	fix				xLastMsgTime;
	fix				xLastPathCreated;
	char				szName [GUIDEBOT_NAME_LEN + 1];
	char				szRealName [GUIDEBOT_NAME_LEN + 1];
} tEscortData;

//------------------------------------------------------------------------------

typedef struct tThiefData {
	ubyte				stolenItems [MAX_STOLEN_ITEMS];
	int				nStolenItem;
	fix				xReInitTime;
	fix				xWaitTimes [NDL];
} tThiefData;

//------------------------------------------------------------------------------

typedef struct tHoardItem {
	int			nWidth;
	int			nHeight;
	int			nFrameSize;
	int			nSize;
	int			nFrames;
	int			nClip;
	CBitmap	bm;
	CPalette		*palette;
} tHoardItem;

typedef struct tHoardData {
	int			bInitialized;
	int			nBitmaps;
	tHoardItem	orb;
	tHoardItem	icon [2];
	tHoardItem	goal;
	tHoardItem	monsterball;
	short			nMonsterballSeg;
	vmsVector	vMonsterballPos;
	CObject		*monsterballP;
	short			nLastHitter;
} tHoardData;

//------------------------------------------------------------------------------

#include "hudmsg.h"

typedef struct tHUDMessage {
	int					nFirst;
	int					nLast;
	int					nMessages;
	fix					xTimer;
	uint		nColor;
	char					szMsgs [HUD_MAX_MSGS][HUD_MESSAGE_LENGTH + 5];
	int					nMsgIds [HUD_MAX_MSGS];
} tHUDMessage;

typedef struct tHUDData {
	tHUDMessage			msgs [2];
	int					bPlayerMessage;
} tHUDData;

//------------------------------------------------------------------------------

typedef struct {
	int	seg0, seg1, csd;
	fix	dist;
} tFCDCacheData;

#define	MAX_FCD_CACHE	64

typedef struct tFCDData {
	int				nIndex;
	tFCDCacheData	cache [MAX_FCD_CACHE];
	fix				xLastFlushTime;
	int				nConnSegDist;
} tFCDData;

//------------------------------------------------------------------------------

typedef struct tVertColorThreadData {
#if MULTI_THREADED_LIGHTS
	tThreadInfo		info [2];
#endif
	tVertColorData	data;
	} tVertColorThreadData;

typedef struct tClipDistData {
	CObject			*objP;
	tPOFObject		*po;
	tPOFSubObject	*pso;
	float				fClipDist [2];
	} tClipDistData;

typedef struct tClipDistThreadData {
#if MULTI_THREADED_SHADOWS
	tThreadInfo		info [2];
#endif
	tClipDistData	data;
	} tClipDistThreadData;

typedef struct tThreadData {
	tVertColorThreadData		vertColor;
	tClipDistThreadData		clipDist;
	} tThreadData;

//------------------------------------------------------------------------------

#if DBG
typedef struct tSpeedtestData {
	int		bOn;
	int		nMarks;
	int		nStartTime;
	int		nSegment;
	int		nSide;
	int		nFrameStart;
	int		nCount;
	} tSpeedtestData;
#endif

//------------------------------------------------------------------------------

typedef struct tLaserData {
	fix		xLastFiredTime;
	fix		xNextFireTime;
	int		nGlobalFiringCount;
	int		nMissileGun;
	int		nOffset;
} tLaserData;

//------------------------------------------------------------------------------

typedef struct tFusionData {
	fix	xAutoFireTime;
	fix	xCharge;
	fix	xNextSoundTime;
	fix	xLastSoundTime;
} tFusionData;

//------------------------------------------------------------------------------

typedef struct tOmegaData {
	fix		xCharge [2];
	fix		xMaxCharge;
	int		nLastFireFrame;
} tOmegaData;

//------------------------------------------------------------------------------

typedef struct tMissileData {
	fix		xLastFiredTime;
	fix		xNextFireTime;
	int		nGlobalFiringCount;
} tMissileData;

//------------------------------------------------------------------------------

typedef struct tPlayerStats {
	int	nShots [2];
	int	nHits [2];
	int	nMisses [2];
	} tPlayerStats;

typedef struct tStatsData {
	tPlayerStats	player [2];	//per level/per session
	int				nDisplayMode;
	} tStatsData;

//------------------------------------------------------------------------------

typedef struct tCollisionData {
	int			nSegsVisited;
	short			segsVisited [MAX_SEGS_VISITED];
	tFVIHitInfo hitData;
} tCollisionData;

//------------------------------------------------------------------------------

typedef struct tTrackIRData {
	int	x, y;
} tTrackIRData;


//------------------------------------------------------------------------------

typedef struct tScoreData {
	int nKillsChanged;
	int bNoMovieMessage;
	int nPhallicLimit;
	int nPhallicMan;
	int bWaitingForOthers;
} tScoreData;

//------------------------------------------------------------------------------

typedef struct tTextIndex {
	int	nId;
	int	nLines;
	char	*pszText;
} tTextIndex;

typedef struct tTextData {
	char			*textBuffer;
	tTextIndex	*index;
	tTextIndex	*currentMsg;
	int			nMessages;
	int			nStartTime;
	int			nEndTime;
	CBitmap	*bmP;
} tTextData;

//------------------------------------------------------------------------------

#define MAX_GAUGE_BMS 100   // increased from 56 to 80 by a very unhappy MK on 10/24/94.
#define D1_MAX_GAUGE_BMS 80   // increased from 56 to 80 by a very unhappy MK on 10/24/94.

class CCockpitData {
	public:
		CArray<tBitmapIndex>	gauges [2]; //[MAX_GAUGE_BMS];
	public:
		CCockpitData ();
	};

class CGameData {
	public:
		tSegmentData		segs;
		CWallData			walls;
		tTriggerData		trigs;
		tObjectData			objs;
		CRobotData			bots;
		tRenderData			render;
		CEffectData			eff;
		CPigData				pig;
		tModelData			models;
		tMultiplayerData	multiplayer;
		tMultiGameData		multigame;
		tMuzzleData			muzzle;
		tWeaponData			weapons;
		tMissionData		missions;
		tEntropyData		entropy;
		tReactorData		reactor;
		tMarkerData			marker;
		tBossData			boss [MAX_BOSS_COUNT];
		tAIData				ai;
		tSongData			songs;
		tEndLevelData		endLevel;
		tMenuData			menu;
		tMatCenData			matCens;
		tDemoData			demo;
		tParticleData		particles;
		tEscortData			escort;
		tThiefData			thief;
		tHoardData			hoard;
		tHUDData				hud;
		tTerrainData		terrain;
		tTimeData			time;
		tFCDData				fcd;
		tVertColorData		vertColor;
		tThreadData			threads;
	#if DBG
		tSpeedtestData		speedtest;
	#endif
		tPhysicsData		physics;
		tLaserData			laser;
		tFusionData			fusion;
		tOmegaData			omega;
		tMissileData		missiles;
		CCockpitData		cockpit;
		tCollisionData		collisions;
		tScoreData			score;
		tTrackIRData		trackIR;
		tStatsData			stats;
		tTextData			messages [2];
		tTextData			sounds;
		tApplicationData	app;
#if PROFILING
		tProfilerData		profiler;
#endif

	public:
		CGameData () {}
		~CGameData () {}
};

extern CGameData gameData;

typedef struct tBossProps {
	ubyte		bTeleports;
	ubyte		bSpewMore;
	ubyte		bSpewBotsEnergy;
	ubyte		bSpewBotsKinetic;
	ubyte		bSpewBotsTeleport;
	ubyte		bInvulEnergy;
	ubyte		bInvulKinetic;
	ubyte		bInvulSpot;
} tBossProps;

typedef struct tIntervalf {
	float	fMin, fMax, fSize, fRad;
} tIntervalf;

extern tBossProps bossProps [2][NUM_D2_BOSSES];

extern char szAutoMission [255];
extern char szAutoHogFile [255];

static inline ushort WallNumS (tSide *sideP) { return (sideP)->nWall; }
static inline ushort WallNumP (CSegment *segP, short nSide) { return WallNumS ((segP)->sides + (nSide)); }
static inline ushort WallNumI (short nSegment, short nSide) { return WallNumP(gameData.segs.segments + (nSegment), nSide); }

//-----------------------------------------------------------------------------

static inline fix MinSegRad (short nSegment)
	{return gameData.segs.segRads [0][nSegment];}

static inline float MinSegRadf (short nSegment)
	{return X2F (gameData.segs.segRads [0][nSegment]);}

static inline fix MaxSegRad (short nSegment)
	{return gameData.segs.segRads [1][nSegment];}

static inline float MaxSegRadf (short nSegment)
	{return X2F (gameData.segs.segRads [1][nSegment]);}

static inline fix AvgSegRad (short nSegment)
	{return (MinSegRad (nSegment) + MaxSegRad (nSegment)) / 2;}

static inline float AvgSegRadf (short nSegment)
	{return X2F (gameData.segs.segRads [0][nSegment] + gameData.segs.segRads [1][nSegment]) / 2;}

static inline fix SegmentVolume (short nSegment)
	{return (fix) (1.25 * Pi * pow (AvgSegRadf (nSegment), 3) + 0.5);}

//	-----------------------------------------------------------------------------------------------------------

static inline short ObjIdx (CObject *objP)
{
return gameData.objs.objects.Index (objP);
}

//	-----------------------------------------------------------------------------------------------------------

#define	NO_WALL					(gameStates.app.bD2XLevel ? 2047 : 255)
#define  IS_WALL(_wallnum)		((ushort) (_wallnum) < NO_WALL)

#define SEG_IDX(_segP)			((short) ((_segP) - gameData.segs.segments))
#define SEG2_IDX(_seg2P)		((short) ((_seg2P) - gameData.segs.segment2s))
#define WALL_IDX(_wallP)		((short) ((_wallP) - gameData.walls.walls))
#define OBJ_IDX(_objP)			ObjIdx (_objP)
#define TRIG_IDX(_triggerP)	((short) ((_triggerP) - gameData.trigs.triggers))
#define FACE_IDX(_faceP)		((int) ((_faceP) - gameData.segs.faces.faces))

#ifdef PIGGY_USE_PAGING

static inline void PIGGY_PAGE_IN (int bmi, int bD1) 
{
CBitmap *bmP = gameData.pig.tex.bitmaps [bD1] + bmi;
if (!bmP->Buffer () || (bmP->Flags () & BM_FLAG_PAGED_OUT))
	PiggyBitmapPageIn (bmi, bD1);
}

#else //!PIGGY_USE_PAGING
#	define PIGGY_PAGE_IN(bmp)
#endif //!PIGGY_USE_PAGING

void GrabMouse (int bGrab, int bForce);
void InitGameOptions (int i);
void SetDataVersion (int v);

//	-----------------------------------------------------------------------------------------------------------

static inline void OglVertex3x (fix x, fix y, fix z)
{
glVertex3f ((float) x / 65536.0f, (float) y / 65536.0f, (float) z / 65536.0f);
}

//	-----------------------------------------------------------------------------------------------------------

static inline void OglVertex3f (g3sPoint *p)
{
if (p->p3_index < 0)
	OglVertex3x (p->p3_vec[X], p->p3_vec[Y], p->p3_vec[Z]);
else
	glVertex3fv (reinterpret_cast<GLfloat *> (gameData.render.vertP + p->p3_index));
}

//	-----------------------------------------------------------------------------------------------------------

static inline float GrAlpha (void)
{
if (gameStates.render.grAlpha >= (float) FADE_LEVELS)
	return 1.0f;
return 1.0f - gameStates.render.grAlpha / (float) FADE_LEVELS;
}

//	-----------------------------------------------------------------------------------------------------------

#define	CLAMP(_val,_minVal,_maxVal)	\
			{if ((_val) < (_minVal)) (_val) = (_minVal); else if ((_val) > (_maxVal)) (_val) = (_maxVal);}

#define LOCALPLAYER	gameData.multiplayer.players [gameData.multiplayer.nLocalPlayer]

#define ISLOCALPLAYER(_nPlayer)	((_nPlayer < 0) || ((_nPlayer) == gameData.multiplayer.nLocalPlayer))

#define G3_INFINITY			fInfinity [gameOpts->render.shadows.nReach]

#define MAX_LIGHT_RANGE	(125 * F1_0)

#define SEGVIS(_i,_j)	((gameData.segs.bSegVis [SEGVIS_FLAGS * (_i) + ((_j) >> 3)] & (1 << ((_j) & 7))) != 0)

//	-----------------------------------------------------------------------------------------------------------

extern float fInfinity [];
extern fix nDebrisLife [];

#define sizeofa(_a)	(sizeof (_a) / sizeof ((_a) [0]))	//number of array elements

#define SEGMENTS	gameData.segs.segments
#define SEGMENT2S	gameData.segs.segment2s
#define SEGFACES	gameData.segs.segFaces
#define OBJECTS	gameData.objs.objects
#define WALLS		gameData.walls.walls
#define FACES		gameData.segs.faces.faces
#define TRIANGLES	gameData.segs.faces.tris

// limit framerate to 30 while recording demo and to 40 when in automap and framerate display is disabled
#define MAXFPS		((gameData.demo.nState == ND_STATE_RECORDING) ? 30 : \
                   (gameStates.render.automap.bDisplay && !(gameStates.render.automap.bRadar || (gameStates.render.bShowFrameRate == 1))) ? 40 : \
                   gameOpts->render.nMaxFPS)

#define SPECTATOR(_objP)	(gameStates.app.bFreeCam && (OBJ_IDX (_objP) == LOCALPLAYER.nObject))
#define OBJPOS(_objP)		(SPECTATOR (_objP) ? &gameStates.app.playerPos : &(_objP)->info.position)
#define OBJSEG(_objP)		(SPECTATOR (_objP) ? gameStates.app.nPlayerSegment : (_objP)->info.nSegment)

//	-----------------------------------------------------------------------------

static inline vmsVector *PolyObjPos (CObject *objP, vmsVector *vPosP)
{
vmsVector vPos = OBJPOS (objP)->vPos;
if (objP->info.renderType == RT_POLYOBJ) {
	*vPosP = *ObjectView (objP) * gameData.models.offsets [objP->rType.polyObjInfo.nModel];
	*vPosP += vPos;
	return vPosP;
	}
*vPosP = vPos;
return vPosP;
}

//	-----------------------------------------------------------------------------

static inline void RequestEffects (CObject *objP, ubyte nEffects)
{
gameData.objs.bWantEffect [OBJ_IDX (objP)] |= nEffects;
}

//	-----------------------------------------------------------------------------

#ifdef RELEASE
#	define FAST_SHADOWS	1
#else
#	define FAST_SHADOWS	gameOpts->render.shadows.bFast
#endif

void D2SetCaption (void);
void PrintVersionInfo (void);

//	-----------------------------------------------------------------------------


typedef struct fVector3D {
	float	x, y, z;
} fVector3D;

typedef struct tTranspRInfo {
	fVector3D	fvRot;
	fVector3D	fvTrans;
	} tTranspRInfo;

#ifndef WIN32
#	define WINAPI
#	define HINSTANCE	int
#	define HWND void *
#endif

typedef int (WINAPI *tpfnTIRInit) (HWND);
typedef int (WINAPI *tpfnTIRExit) (void);
typedef int (WINAPI *tpfnTIRStart) (void);
typedef int (WINAPI *tpfnTIRStop) (void);
typedef int (WINAPI *tpfnTIRCenter) (void);
typedef int (WINAPI *tpfnTIRQuery) (tTranspRInfo *);

extern tpfnTIRInit	pfnTIRInit;
extern tpfnTIRExit	pfnTIRExit;
extern tpfnTIRStart	pfnTIRStart;
extern tpfnTIRStop	pfnTIRStop;
extern tpfnTIRCenter	pfnTIRCenter;
extern tpfnTIRQuery	pfnTIRQuery;

int TIRLoad (void);
int TIRUnload (void);

#ifdef _WIN32
#	define	G3_SLEEP(_t)	Sleep (_t)
#else
#	include <unistd.h>
#	define	G3_SLEEP(_t)	usleep ((_t) * 1000)
#endif

#define HW_GEO_LIGHTING 0 //gameOpts->ogl.bGeoLighting

#define SEM_SMOKE			0
#define SEM_LIGHTNINGS	1
#define SEM_SPARKS		2
#define SEM_SHRAPNEL		3

#include "error.h"

static inline void SemWait (uint sem)
{
	time_t t0 = gameStates.app.nSDLTicks;

while (gameData.app.semaphores [sem]) {
	G3_SLEEP (0);
	if (SDL_GetTicks () - t0 > 50) {
		PrintLog ("multi threading got stuck (semaphore: %d)\n", sem);
		gameData.app.bUseMultiThreading [rtEffects] = 0;
		gameData.app.semaphores [sem] = 0;
		break;
		}
	}
}


#if DBG

static inline void SemEnter (uint sem, const char *pszFile, int nLine)
{
SemWait (sem);
//PrintLog ("SemEnter (%d) @ %s:%d\n", sem, pszFile, nLine);
gameData.app.semaphores [sem]++;
}

static inline void SemLeave (uint sem, const char *pszFile, int nLine)
{
if (gameData.app.semaphores [sem]) {
	gameData.app.semaphores [sem]--;
	//PrintLog ("SemLeave (%d) @ %s:%d\n", sem, pszFile, nLine);
	}
else
	PrintLog ("asymmetric SemLeave (%d) @ %s:%d\n", sem, pszFile, nLine);
}

#define SEM_WAIT(_sem)	if (gameStates.app.bMultiThreaded && gameData.app.bUseMultiThreading [rtEffects]) SemWait (_sem);

#define SEM_ENTER(_sem)	if (gameStates.app.bMultiThreaded && gameData.app.bUseMultiThreading [rtEffects]) SemEnter (_sem, __FILE__, __LINE__);

#define SEM_LEAVE(_sem)	if (gameStates.app.bMultiThreaded && gameData.app.bUseMultiThreading [rtEffects]) SemLeave (_sem, __FILE__, __LINE__);

#else


static inline void SemEnter (uint sem)
{
SemWait (sem);
gameData.app.semaphores [sem]++;
}

static inline void SemLeave (uint sem)
{
if (gameData.app.semaphores [sem])
	gameData.app.semaphores [sem]--;
}

#define SEM_WAIT(_sem)	if (gameStates.app.bMultiThreaded && gameData.app.bUseMultiThreading [rtEffects]) SemWait (_sem);

#define SEM_ENTER(_sem)	if (gameStates.app.bMultiThreaded && gameData.app.bUseMultiThreading [rtEffects]) SemEnter (_sem);

#define SEM_LEAVE(_sem)	if (gameStates.app.bMultiThreaded && gameData.app.bUseMultiThreading [rtEffects]) SemLeave (_sem);

#endif

//	-----------------------------------------------------------------------------------------------------------

#ifndef min
#	define min(_a,_b)	((_a) <= (_b) ? (_a) : (_b))
#endif

#ifndef max
#	define max(_a,_b)	((_a) >= (_b) ? (_a) : (_b))
#endif

//	-----------------------------------------------------------------------------------------------------------

#endif


