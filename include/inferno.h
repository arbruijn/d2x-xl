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

#define FLASH_CYCLE_RATE I2X (1)

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
	int bSplitHUDMsgs;	//split CPlayerData and other message displays
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
	int bDecreaseLag;	//only render if CPlayerData is moving forward
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
	CFixVector vTgtDir;
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
	int bRebuilding;
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
	int bFSAA;
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
	int bHaveSparks;
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
	int bSoundsInitialized;
	int bLoMem;
	int nNextSignature;
	int nActiveObjects;
} tDigiStates;

//------------------------------------------------------------------------------

typedef struct tSoundStates {
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
	int bClearMessage;
	fix xThisLevelTime;
	fix nPlayerTimeOfDeath;
	char *szCurrentMission;
	char *szCurrentMissionFile;
	tObjTransformation playerPos;
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

class CBase {
	private:
		CBase*	m_prev;
		CBase*	m_next;
		CBase*	m_parent;
		CBase*	m_child;
		uint		m_refCount;
	public:
		CBase () { Init (); } 
		virtual ~CBase () { Destroy (); }
		virtual void Init (void) { 
			Link (this, this);
			m_parent = m_child = NULL;
			m_refCount = 0;
			}
		virtual bool Create (void) { return true; }
		virtual void Destroy (void) {
			Unlink ();
			Init ();
			}
		virtual void Link (CBase* prev, CBase *next) {
			m_prev = prev;
			m_next = next;
			m_prev->SetNext (this);
			m_next->SetPrev (this);
			}
		virtual void Unlink (void) {
			if (m_prev)
				m_prev->SetNext (m_next);
			if (m_next)
				m_next->SetPrev (m_prev);
			Link (this, this);
			}
		inline virtual CBase* Prev (void) { return m_prev; }
		inline virtual CBase* Next (void) { return m_next; }
		inline virtual void SetPrev (CBase *prev) { m_prev = prev; }
		inline virtual void SetNext (CBase *next) { m_next = next; }
	};


//------------------------------------------------------------------------------

#include "cameras.h"

class CShadowLightData {
	public:
		CFloatVector	vPosf;
		short				nMap;
		ubyte				nFrame;	//set per frame when scene as seen from a light source has been rendered
#if DBG
		CFixMatrix	orient;
#endif
	public:
		CShadowLightData () { Init (); }
		void Init (void) { memset (this, 0, sizeof (*this)); }
};

#define MAX_SHADOW_MAPS	20
#define MAX_SHADOW_LIGHTS 8

typedef struct tLightRef {
	short			nSegment;
	short			nSide;
	short			nTexture;
} tLightRef;

//------------------------------------------------------------------------------

class CColorData {
	public:
		CArray<tFaceColor>	lights;
		CArray<tFaceColor>	sides;
		CArray<tFaceColor>	segments;
		CArray<tFaceColor>	vertices;
		CArray<float>			vertBright;
		CArray<tFaceColor>	ambient;	//static light values
		CArray<tFaceColor>	textures; // [MAX_WALL_TEXTURES];
		CArray<tFaceColor>	defaultTextures [2]; //[MAX_WALL_TEXTURES];
		CArray<tLightRef>		visibleLights;
		int						nVisibleLights;
		tRgbColorf				flagTag;
	public:
		CColorData ();
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

class CPulseData {
	public:
		float			fScale;
		float			fMin;
		float			fDir;
		float			fSpeed;
	public:
		CPulseData () { memset (this, 0, sizeof (*this)); }
};

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

class CFlickerLightData {
	public:
		CStaticArray< tVariableLight, MAX_FLICKERING_LIGHTS >	lights; // [MAX_FLICKERING_LIGHTS];
		int				nLights;
	public:
		CFlickerLightData () { memset (this, 0, sizeof (*this)); }
};

//------------------------------------------------------------------------------

#include "dynlight.h"

class CLightData {
	public:
		int								nStatic;
		int								nCoronas;
		CArray<fix>						segDeltas;
		CArray<CLightDeltaIndex>	deltaIndices;
		CArray<CLightDelta>			deltas;
		CArray<ubyte>					subtracted;
		CFlickerLightData				flicker;
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
	public:
		CLightData ();
		void Init (void);
		bool Create (void);
		void Destroy (void);
};

inline int operator- (CLightDeltaIndex* o, CArray<CLightDeltaIndex>& a) { return a.Index (o); }
inline int operator- (CLightDelta* o, CArray<CLightDelta>& a) { return a.Index (o); }

//------------------------------------------------------------------------------

class CShadowData {
	public:
		short					nLight;
		short					nLights;
		CDynLight*			lights;
		short					nShadowMaps;
		CCamera				shadowMaps [MAX_SHADOW_MAPS];
		CObject				lightSource;
		CFloatVector		vLightPos;
		CFixVector			vLightDir [MAX_SHADOW_LIGHTS];
		CArray<short>		objLights;
		ubyte					nFrame;	//flipflop for testing whether a light source's view has been rendered the current frame
	public:
		CShadowData ();
		void Init (void);
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

#include "morph.h"

class CMorphData {
	public:
		CStaticArray< tMorphInfo, MAX_MORPH_OBJECTS >	objects; // [MAX_MORPH_OBJECTS];
		fix			xRate;
	public:
		CMorphData ();
};

//------------------------------------------------------------------------------

#define OGLTEXBUFSIZE (4096 * 4096 * 4)

typedef struct tScreenScale {
	float x, y;
} tScreenScale;

class COglData {
	public:
		GLubyte			buffer [OGLTEXBUFSIZE];
		CPalette*		palette;
		GLenum			nSrcBlend;
		GLenum			nDestBlend;
		float				zNear;
		float				zFar;
		CFloatVector3			depthScale;
		tScreenScale	screenScale;
		CFBO				drawBuffer;
		CFBO				glowBuffer;
		int				nPerPixelLights [8];
		float				lightRads [8];
		CFloatVector	lightPos [8];
		int				bLightmaps;
		int				nHeadlights;
	public:
		COglData ();
};

//------------------------------------------------------------------------------

#define TERRAIN_GRID_MAX_SIZE   64
#define TERRAIN_GRID_SCALE      I2X (2*20)
#define TERRAIN_HEIGHT_SCALE    I2X (1)

class CTerrainRenderData {
	public:
		CArray<ubyte>			heightmap;
		CArray<fix>				lightmap;
		CArray<CFixVector>	points;
		CBitmap					*bmP;
		CStaticArray< g3sPoint, TERRAIN_GRID_MAX_SIZE >		saveRow; // [TERRAIN_GRID_MAX_SIZE];
		CFixVector				vStartPoint;
		tUVL						uvlList [2][3];
		int						bOutline;
		int						nGridW, nGridH;
		int						orgI, orgJ;
		int						nMineTilesDrawn;    //flags to tell if all 4 tiles under mine have drawn
	public:
		CTerrainRenderData ();
};

//------------------------------------------------------------------------------

#include "flightpath.h"

class CThrusterData {
	public:
		CFlightPath		path;
		float				fSpeed;
		short				nPulse;
		time_t			tPulse;
	public:
		CThrusterData ();
};

//------------------------------------------------------------------------------

#define OBJS_PER_SEG        5
#define N_EXTRA_OBJ_LISTS   50

typedef struct tObjRenderListItem {
	short	nNextItem;
	short	nObject;
	fix	xDist;
} tObjRenderListItem;

typedef struct tObjRenderList {
	CStaticArray< short, MAX_SEGMENTS_D2X >	ref; // [MAX_SEGMENTS_D2X];	//points to each segment's first render object list entry in renderObjs
	CStaticArray< tObjRenderListItem, MAX_OBJECTS_D2X >	objs; // [MAX_OBJECTS_D2X];
	int						nUsed;
} tObjRenderList;

class CMineRenderData {
	public:
		CFixVector				viewerEye;
		CShortArray				nSegRenderList; //[MAX_SEGMENTS_D2X];
		CArray< tFace* >		pFaceRenderList; //[MAX_SEGMENTS_D2X * 6];
		tObjRenderList			renderObjs;
		int						nRenderSegs;
		CByteArray				bVisited; //[MAX_SEGMENTS_D2X];
		CByteArray				bVisible; //[MAX_SEGMENTS_D2X];
		CByteArray				bProcessed; //[MAX_SEGMENTS_D2X];		//whether each entry has been nProcessed
		ubyte 					nVisited;
		ubyte						nProcessed;
		ubyte						nVisible;
		CShortArray				nSegDepth; //[MAX_SEGMENTS_D2X];		//depth for each seg in nRenderList
		int						lCntSave;
		int						sCntSave;
		CByteArray				bObjectRendered; //[MAX_OBJECTS_D2X];
		CByteArray				bRenderSegment; //[MAX_SEGMENTS_D2X];
		CShortArray				nRenderObjList; //[MAX_SEGMENTS_D2X+N_EXTRA_OBJ_LISTS][OBJS_PER_SEG];
		CShortArray				nRenderPos; //[MAX_SEGMENTS_D2X];
		CIntArray				nRotatedLast; //[MAX_VERTICES_D2X];
		CByteArray				bCalcVertexColor; //[MAX_VERTICES_D2X];
		CUShortArray			bAutomapVisited; //[MAX_SEGMENTS_D2X];
		CUShortArray			bAutomapVisible; //[MAX_SEGMENTS_D2X];
		CUShortArray			bRadarVisited; //[MAX_SEGMENTS_D2X];
		ubyte						bSetAutomapVisited;

	public:
		CMineRenderData ();
		~CMineRenderData () { Destroy (); }
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

class CGameWindowData {
	public:
		int	x, y;
		int	w, h;
		int	wMax, hMax;
	public:
		CGameWindowData () { memset (this, 0, sizeof (*this)); }
};

//------------------------------------------------------------------------------

class CVertColorData {
	public:
		int				bExclusive;
		int				bNoShadow;
		int				bDarkness;
		int				bMatEmissive;
		int				bMatSpecular;
		int				nMatLight;
		CFloatVector	matAmbient;
		CFloatVector	matDiffuse;
		CFloatVector	matSpecular;
		CFloatVector	colorSum;
		CFloatVector3	vertNorm;
		CFloatVector3	vertPos;
		CFloatVector3*	vertPosP;
		float				fMatShininess;
	public:
		CVertColorData () { memset (this, 0, sizeof (*this)); }
	};

typedef struct tFaceListItem {
	tFace					*faceP;
	short					nNextItem;
} tFaceListItem;

class CFaceListIndex {
	public:
		CShortArray				roots; // [MAX_WALL_TEXTURES * 3];
		CShortArray				tails; // [MAX_WALL_TEXTURES * 3];
		CShortArray				usedKeys; // [MAX_WALL_TEXTURES * 3];
		short						nUsedFaces;
		short						nUsedKeys;
	public:
		CFaceListIndex ();
	};

#include "sphere.h"

class CRenderData {
	public:
		CColorData					color;
		int							transpColor;
		CFaceListIndex				faceIndex [2];
		CVertColorData				vertColor;
		CSphere						shield;
		CSphere						monsterball;
		CArray<tFaceListItem>	faceList;
		fix							xFlashEffect;
		fix							xTimeFlashLastPlayed;
		CFloatVector*				vertP;
		CFloatVector*				vertexList;
		CLightData					lights;
		CMorphData					morph;
		CShadowData					shadows;
		COglData						ogl;
		CTerrainRenderData		terrain;
		CThrusterData				thrusters [MAX_PLAYERS];
		CMineRenderData			mine;
		CGameWindowData			window;
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

	public:
		CRenderData ();
		void Init (void);
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

class CSecretData {
	public:
		int			nReturnSegment;
		CFixMatrix	returnOrient;
	public:
		CSecretData () { memset (this, 0, sizeof (*this)); }
};

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
	CFloatVector3		vertex;
	CFloatVector3		normal;
	tRgbaColorf			color;
	tTexCoord2f			texCoord;
	tTexCoord2f			ovlTexCoord;
	tTexCoord2f			lMapTexCoord;
	} tFaceRenderVertex;

class CFaceData {
	public:
		CArray<tFace>				faces;
		CArray<tFaceTriangle>	tris;
		CArray<CFloatVector3>	vertices;
		CArray<CFloatVector3>	normals;
		CArray<tTexCoord2f>		texCoord;
		CArray<tTexCoord2f>		ovlTexCoord;
		CArray<tTexCoord2f>		lMapTexCoord;
		CArray<tRgbaColorf>		color;
		CArray<ushort>				faceVerts;
		tFace*						slidingFaces;
#if USE_RANGE_ELEMENTS
		CArray<GLuint>				vertIndex;
#endif	
		GLuint						vboDataHandle;
		GLuint						vboIndexHandle;
		ubyte*						vertexP;
		ushort*						indexP;
		int							nVertices;
		int							iVertices;
		int							iNormals;
		int							iColor;
		int							iTexCoord;
		int							iOvlTexCoord;
		int							iLMapTexCoord;
	public:
		CFaceData ();
		void Init (void);
		bool Create (void);
		void Destroy (void);
};

inline int operator- (tFaceTriangle* o, CArray<tFaceTriangle>& a) { return a.Index (o); }
inline int operator- (CFloatVector3* o, CArray<CFloatVector3>& a) { return a.Index (o); }
inline int operator- (tTexCoord2f* o, CArray<tTexCoord2f>& a) { return a.Index (o); }

typedef struct tSegList {
	int					nSegments;
	CArray<short>		segments;
} tSegList;

typedef struct tSegExtent {
	CFixVector			vMin;
	CFixVector			vMax;
	} tSegExtent;


class CSkyBox : public CStack< short > {
	public:
		void Destroy (void);
		int CountSegments (void);
};

class CSegmentData {
	public:
		int						nMaxSegments;
		CArray<CFixVector>	vertices;
		CArray<CFloatVector>	fVertices;
		CArray<CSegment>		segments;
		CArray<tSegFaces>		segFaces;
		CArray<g3sPoint>		points;
		CSkyBox					skybox;
#if CALC_SEGRADS
		CArray<fix>				segRads [2];
		CArray<tSegExtent>	extent;
#endif
		CFixVector				vMin;
		CFixVector				vMax;
		float						fRad;
		CArray<CFixVector>	segCenters [2];
		CArray<CFixVector>	sideCenters;
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
		CSecretData				secret;
		CArray<tSlideSegs>	slideSegs;
		short						nSlideSegs;
		int						bHaveSlideSegs;
		CFaceData				faces;
	public:
		CSegmentData ();
		void Init (void);
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

class CWallData {
	public:
		CArray<CWall>				walls ; //[MAX_WALLS];
		CArray<CExplodingWall>	exploding ; //[MAX_EXPLODING_WALLS];
		CStack<CActiveDoor>		activeDoors ; //[MAX_DOORS];
		CStack<CCloakingWall>	cloaking ; //[MAX_CLOAKING_WALLS];
		CArray<tWallClip>			anims [2]; //[MAX_WALL_ANIMS];
		CArray<int>					bitmaps ; //[MAX_WALL_ANIMS];
		int							nWalls;
		int							nAnims [2];
		CArray<tWallClip>			animP;

	public:
		CWallData ();
		void Destroy (void);
};

//------------------------------------------------------------------------------

class CTriggerData {
	public:
		CArray<CTrigger>			triggers; // [MAX_TRIGGERS];
		CArray<CTrigger>			objTriggers; // [MAX_TRIGGERS];
		CArray<tObjTriggerRef>	objTriggerRefs; // [MAX_OBJ_TRIGGERS];
		CArray<short>				firstObjTrigger; // [MAX_OBJECTS_D2X];
		CArray<int>					delay; // [MAX_TRIGGERS];
		int							nTriggers;
		int							nObjTriggers;
	public:
		CTriggerData ();
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

class CPowerupData {
	public:
		CStaticArray< tPowerupTypeInfo, MAX_POWERUP_TYPES >	info; // [MAX_POWERUP_TYPES];
		int						nTypes;

	public:
		CPowerupData () { memset (this, 0, sizeof (*this)); }
} ;

//------------------------------------------------------------------------------

typedef struct tObjTypeData {
	int						nTypes;
	CStaticArray< sbyte, MAX_OBJTYPE >	nType; //[MAX_OBJTYPE];
	CStaticArray< sbyte, MAX_OBJTYPE >	nId; //[MAX_OBJTYPE];  
	CStaticArray< fix, MAX_OBJTYPE >		nStrength; //[MAX_OBJTYPE];   
} tObjTypeData;

//------------------------------------------------------------------------------

typedef struct tSpeedBoostData {
	int						bBoosted;
	CFixVector				vVel;
	CFixVector				vMinVel;
	CFixVector				vMaxVel;
	CFixVector				vSrc;
	CFixVector				vDest;
} tSpeedBoostData;

//------------------------------------------------------------------------------

typedef struct tQuad {
	CFixVector			v [4];	//corner vertices
	CFixVector			n [2];	//normal, transformed normal
#if DBG
	time_t				t;
#endif
} tQuad;

typedef struct tBox {
	CFixVector			vertices [8];
	tQuad					faces [6];
#if 0
	tQuad					rotFaces [6];	//transformed faces
	short					nTransformed;
#endif
	} tBox;

typedef struct tHitbox {
	CFixVector			vMin;
	CFixVector			vMax;
	CFixVector			vSize;
	CFixVector			vOffset;
	tBox					box;
	CAngleVector		angles;			//rotation angles
	short					nParent;			//parent hitbox
} tHitbox;

//------------------------------------------------------------------------------

#define MAX_WEAPONS	100

typedef struct tObjectViewData {
	CFixMatrix			mView;
	int					nFrame;
} tObjectViewData;

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

class CObjectData {
	public:
		CArray<tObjTypeData>		types;
		CArray<CObject>			objects;
		tObjLists					lists;
		CArray<short>				freeList;
		CArray<short>				parentObjs;
		CArray<tObjectRef>		childObjs;
		CArray<short>				firstChild;
		CArray<CObject>			init;
		CArray<tObjDropInfo>		dropInfo;
		CArray<tSpeedBoostData>	speedBoost;
		CArray<CFixVector>		vRobotGoals;
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
		int							nMaxObjects;
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
		CPowerupData				pwrUp;
		ubyte							collisionResult [MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];
		CArray<tObjectViewData>	viewData;
		CStaticArray< ubyte, MAX_WEAPONS >	bIsMissile; //[MAX_WEAPONS];
		CStaticArray< ubyte, MAX_WEAPONS >	bIsWeapon; //[MAX_WEAPONS];
		CStaticArray< ubyte, MAX_WEAPONS >	bIsSlowWeapon; //[MAX_WEAPONS];
		CStaticArray< short, MAX_WEAPONS >	idToOOF; //[MAX_WEAPONS];
		CByteArray				bWantEffect; //[MAX_OBJECTS_D2X];
		int							nFrameCount;

	public:
		CObjectData ();
		void Init (void);
		bool Create (void);
		void Destroy (void);
};

#define PLAYER_LIGHTNINGS	1
#define ROBOT_LIGHTNINGS	2
#define MISSILE_LIGHTNINGS	4
#define EXPL_LIGHTNINGS		8
#define DESTROY_LIGHTNINGS	16
#define SHRAPNEL_SMOKE		32
#define DESTROY_SMOKE		64

//------------------------------------------------------------------------------

class CFVISideData {
	public:
		int					bCache;
		int					vertices [6];
		int					nFaces;
		short					nSegment;
		short					nSide;
		short					nType;

	public:
		CFVISideData ();
};

class CPhysicsData {
	public:
		CArray<short>		ignoreObjs;
		CFVISideData		side;
		fix					xTime;
		fix					xAfterburnerCharge;
		fix					xBossInvulDot;
		CFixVector			playerThrust;

	public:
		CPhysicsData ();
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

#include "pof.h"

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
		CArray<POF::CModel>	pofData;
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
		CArray<CDigiSound>	sounds [2]; //[MAX_SOUND_FILES];
		int						nSoundFiles [2];
		CArray<CDigiSound>	soundP;
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
		CArray<int>					brightness; // [MAX_WALL_TEXTURES];
		CArray<int>					defaultBrightness [2]; //[MAX_WALL_TEXTURES];

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
};

inline int operator- (tEffectClip* o, CArray<tEffectClip>& a) { return a.Index (o); }
inline int operator- (tVideoClip* o, CArray<tVideoClip>& a) { return a.Index (o); }

class CShipData {
	public:
		CPlayerShip		only;
		CPlayerShip		*player;

	public:
		 CShipData ();
};

//------------------------------------------------------------------------------

typedef struct tFlagData {
	tBitmapIndex		bmi;
	tVideoClip			*vcP;
	tVClipInfo			vci;
	CFlightPath			path;
} tFlagData;

//------------------------------------------------------------------------------

class CPigData {
	public:
		CTextureData		tex;
		CSoundData			sound;
		CShipData			ship;
		tFlagData			flags [2];

	public:
		CPigData () {}
		~CPigData () {}
};

//------------------------------------------------------------------------------

#include "laser.h"

class CMuzzleData {
	public:
		int				queueIndex;
		tMuzzleInfo		info [MUZZLE_QUEUE_MAX];
};

#include "weapon.h"

#define GATLING_DELAY	700

typedef struct tFiringData {
	int					nStart;
	int					nStop;
	int					nDuration;
	int					bSound;
	int					bSpeedUp;
	} tFiringData;

class CWeaponData {
	public:
		sbyte						nPrimary;
		sbyte						nSecondary;
		sbyte						nOverridden;
		sbyte						bTripleFusion;
		tFiringData				firing [2];
		int						nTypes [2];
		CStaticArray< CWeaponInfo, MAX_WEAPON_TYPES >	info; // [MAX_WEAPON_TYPES];
		CStaticArray< CWeaponInfo, MAX_WEAPON_TYPES >	infoD1; // [D1_MAX_WEAPON_TYPES];
		CArray<tRgbaColorf>	color;
		ubyte						bLastWasSuper [2][MAX_PRIMARY_WEAPONS];

	public:
		CWeaponData () { memset (this, 0, sizeof (*this)); }
		bool Create (void);
		void Destroy (void);
};

#define bLastPrimaryWasSuper (gameData.weapons.bLastWasSuper [0])
#define bLastSecondaryWasSuper (gameData.weapons.bLastWasSuper [1])

//------------------------------------------------------------------------------

#include "polymodel.h"

#define OOF_PYRO			0
#define OOF_MEGA			1

#define MAX_HITBOXES		100

typedef struct tModelHitboxes {
	ubyte					nHitboxes;
	CStaticArray< tHitbox, MAX_HITBOXES + 1 >	hitboxes; // [MAX_HITBOXES + 1];
#if DBG
	CFixVector			vHit;
	time_t				tHit;
#endif
} tModelHitboxes;

typedef struct tModelThrusters {
	CFixVector			vPos [2];
	CFixVector			vDir [2];
	float					fSize;
	ushort				nCount;
	} tModelThrusters;

typedef struct tGunInfo {
	int					nGuns;
	CFixVector			vGunPoints [MAX_GUNS];
	} tGunInfo;

typedef struct tModelSphere {
	short					nSubModels;
	short					nFaces;
	short					nFaceVerts;
	fix					xRads [3];
	CFixVector			vOffsets [3];
} tModelSphere;

#include "rendermodel.h"

class CModelData {
	public:
		int									nLoresModels;
		int									nHiresModels;
		int									nPolyModels;
		int									nDefPolyModels;
		int									nSimpleModelThresholdScale;
		int									nMarkerModel;
		int									nCockpits;
		int									nLightScale;
		CArray<int>							nDyingModels ; //[MAX_POLYGON_MODELS];
		CArray<int>							nDeadModels ; //[MAX_POLYGON_MODELS];
		CArray<ASE::CModel>				aseModels [2]; //[MAX_POLYGON_MODELS];
		CArray<OOF::CModel>				oofModels [2]; //[MAX_POLYGON_MODELS];
		CArray<POF::CModel>				pofData [2][2]; //[MAX_POLYGON_MODELS];
		CArray<ubyte>						bHaveHiresModel ; //[MAX_POLYGON_MODELS];
		CArray<CPolyModel>				polyModels [3] ; //[MAX_POLYGON_MODELS];
		CArray<OOF::CModel*>				modelToOOF [2]; //[MAX_POLYGON_MODELS];
		CArray<ASE::CModel*>				modelToASE [2]; //[MAX_POLYGON_MODELS];
		CArray<CPolyModel*>				modelToPOL ; //[MAX_POLYGON_MODELS];
		CArray<g3sPoint>					polyModelPoints ; //[MAX_POLYGON_VERTS];
		CArray<CFloatVector>				fPolyModelVerts ; //[MAX_POLYGON_VERTS];
		CArray<CBitmap*>					textures ; //[MAX_POLYOBJ_TEXTURES];
		CArray<tBitmapIndex>				textureIndex ; //[MAX_POLYOBJ_TEXTURES];
		CArray<tModelHitboxes>			hitboxes ; //[MAX_POLYGON_MODELS];
		CArray<tModelThrusters>			thrusters ; //[MAX_POLYGON_MODELS];
		CArray<RenderModel::CModel>	renderModels [2]; //[MAX_POLYGON_MODELS];
		CArray<CFixVector>				offsets ; //[MAX_POLYGON_MODELS];
		CArray<tGunInfo>					gunInfo ; //[MAX_POLYGON_MODELS];
		CArray<tModelSphere>				spheres ; //[MAX_POLYGON_MODELS];
		CFixVector							vScale;

	public:
		CModelData ();
		bool Create (void);
};

//------------------------------------------------------------------------------

class CAutoNetGame {
	public:
		char					szPlayer [9];		//CPlayerData profile name
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

	public:
		CAutoNetGame () { memset (this, 0, sizeof (*this)); }
};

typedef struct tLeftoverPowerup {
	CObject				*spitterP;
	ubyte					nCount;
	ubyte					nType;
} tLeftoverPowerup;

class CWeaponState {
	public:
		char						nMissiles;
		char						nPrimary;
		char						nSecondary;
		char						bQuadLasers;
		tFiringData				firing [2];
		char						nLaserLevel;
		char						bTripleFusion;
		char						nMslLaunchPos;
		fix						xMslFireTime;

	public:
		CWeaponState () { memset (this, 0, sizeof (*this)); }
	};

class CMultiplayerData {
	public:
		int 								nPlayers;				
		int								nMaxPlayers;
		int 								nLocalPlayer;				
		int								nPlayerPositions;
		int								bMoving;
		CPlayerData						players [MAX_PLAYERS + 4];  
		tObjPosition					playerInit [MAX_PLAYERS];
		short								nVirusCapacity [MAX_PLAYERS];
		int								nLastHitTime [MAX_PLAYERS];
		CWeaponState					weaponStates [MAX_PLAYERS];
		char								bWasHit [MAX_PLAYERS];
		int								bulletEmitters [MAX_PLAYERS];
		int					 			gatlingSmoke [MAX_PLAYERS];
		CPulseData						spherePulse [MAX_PLAYERS];
		CStaticArray< ubyte, MAX_POWERUP_TYPES >	powerupsInMine; //[MAX_POWERUP_TYPES];
		CStaticArray< ubyte, MAX_POWERUP_TYPES >	powerupsOnShip; //[MAX_POWERUP_TYPES];
		CStaticArray< ubyte, MAX_POWERUP_TYPES >	maxPowerupsAllowed; //[MAX_POWERUP_TYPES];
		CArray<tLeftoverPowerup>	leftoverPowerups;
		CAutoNetGame					autoNG;
		fix								xStartAbortMenuTime;

	public:
		CMultiplayerData ();
		bool Create (void);
		void Destroy (void);
};

#include "multi.h"

//------------------------------------------------------------------------------

class CMultiCreateData {
	public:
		CStaticArray< int, MAX_NET_CREATE_OBJECTS >	nObjNums; // [MAX_NET_CREATE_OBJECTS];
		int					nLoc;

	public:
		CMultiCreateData () { memset (this, 0, sizeof (*this)); }
};

class CMultiLaserData {
	public:
		int					bFired;
		int					nGun;
		int					nFlags;
		int					nLevel;
		short					nTrack;

	public:
		CMultiLaserData () { memset (this, 0, sizeof (*this)); }
};


class CMultiMsgData {
	public:
		char					bSending;
		char					bDefining;
		int					nIndex;
		char					szMsg [MAX_MESSAGE_LEN];
		char					szMacro [4][MAX_MESSAGE_LEN];
		char					buf [MAX_MULTI_MESSAGE_LEN+4];            // This is where multiplayer message are built
		int					nReceiver;

	public:
		CMultiMsgData () { memset (this, 0, sizeof (*this)); }
};

class CMultiMenuData {
	public:
		char					bInvoked;
		char					bLeave;

	public:
		CMultiMenuData () { memset (this, 0, sizeof (*this)); }
};

class CMultiKillData {
	public:
		char					pFlags [MAX_NUM_NET_PLAYERS];
		int					nSorted [MAX_NUM_NET_PLAYERS];
		short					matrix [MAX_NUM_NET_PLAYERS][MAX_NUM_NET_PLAYERS];
		short					nTeam [2];
		char					bShowList;
		fix					xShowListTimer;

	public:
		CMultiKillData () { memset (this, 0, sizeof (*this)); }
};

class CMultiGameData {
	public:
		int					nWhoKilledCtrlcen;
		char					bShowReticleName;
		char					bIsGuided;
		char					bQuitGame;
		CMultiCreateData	create;
		CMultiLaserData	laser;
		CMultiMsgData		msg;
		CMultiMenuData		menu;
		CMultiKillData		kills;
		tMultiRobotData	robots;
		CArray<short>		remoteToLocal;  // Remote CObject number for each local CObject
		CArray<short>		localToRemote;
		CArray<sbyte>		nObjOwner;   // Who created each CObject in my universe, -1 = loaded at start
		int					bGotoSecret;
		int					nTypingTimeout;

	public:
		CMultiGameData ();
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

#define LEVEL_NAME_LEN 36       //make sure this is a multiple of 4!

#include "mission.h"

class CMissionData {
	public:
		char					szCurrentLevel [LEVEL_NAME_LEN];
		int					nSecretLevels;
		int					nLastLevel;
		int					nCurrentLevel;
		int					nNextLevel;
		int					nLastSecretLevel;
		int					nLastMission;
		int					nEntryLevel;
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

	public:
		CMissionData ();
};

//------------------------------------------------------------------------------

#define N_MAX_ROOMS	128

class CEntropyData {
	public:
		char	nRoomOwners [N_MAX_ROOMS];
		char	nTeamRooms [3];
		int   nTotalRooms;

	public:
		CEntropyData () { memset (this, 0, sizeof (*this)); }
};

extern CEntropyData entropyData;

typedef struct tCountdownData {
	fix					nTimer;
	int					nSecsLeft;
	int					nTotalTime;
} tCountdownData;

//------------------------------------------------------------------------------

#define NUM_MARKERS         16
#define MARKER_MESSAGE_LEN  40

class CMarkerData {
	public:
		CStaticArray< CFixVector, NUM_MARKERS >	point; // [NUM_MARKERS];		//these are only used in multi.c, and I'd get rid of them there, but when I tried to do that once, I caused some horrible bug. -MT
		char					szMessage [NUM_MARKERS][MARKER_MESSAGE_LEN];
		char					nOwner [NUM_MARKERS][CALLSIGN_LEN+1];
		CStaticArray< short, NUM_MARKERS >			objects; // [NUM_MARKERS];
		short					viewers [2];
		int					nHighlight;
		float					fScale;
		ubyte					nDefiningMsg;
		char					szInput [40];
		int					nIndex;
		int					nCurrent;
		int					nLast;

	public:
		CMarkerData ();
		void Init (void);
};

//------------------------------------------------------------------------------

class CTimeData {
	public:
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

	public:
		CTimeData ();
};

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

class CApplicationData {
	public:
		int					nFrameCount;
		int					nMineRenderCount;
		int					nGameMode;
		int					bGamePaused;
		uint					nStateGameId;
		uint					semaphores [4];
		int					nLifetimeChecksum;
		int					bUseMultiThreading [rtTaskCount];

	public:
		CApplicationData ();
};

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
#define BOSS_CLOAK_DURATION		I2X (7)
#define BOSS_DEATH_DURATION		I2X (6)

class CBossData {
	public:
		short					nTeleportSegs;
		CStaticArray< short, MAX_BOSS_TELEPORT_SEGS >	teleportSegs; // [MAX_BOSS_TELEPORT_SEGS];
		short					nGateSegs;
		CStaticArray< short, MAX_BOSS_TELEPORT_SEGS >	gateSegs; // [MAX_BOSS_TELEPORT_SEGS];
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

	public:
		CBossData ();
};

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
	CFixVector			vGunPos [MAX_CONTROLCEN_GUNS];
	CFixVector			vGunDir [MAX_CONTROLCEN_GUNS];
} tReactorStates;

class CReactorData {
	public:
		CStaticArray< tReactorProps, MAX_REACTORS >	props; // [MAX_REACTORS];
		tReactorTriggers	triggers;
		CStaticArray< tReactorStates, MAX_BOSS_COUNT >	states; // [MAX_BOSS_COUNT];
		tCountdownData		countdown;
		int					nReactors;
		int					nStrength;
		int					bPresent;
		int					bDisabled;
		int					bDestroyed;

	public:
		CReactorData ();
};

//------------------------------------------------------------------------------

#include "ai.h"

class CAIData {
	public:
		int							bInitialized;
		int							nOverallAgitation;
		int							bEvaded;
		int							bEnableAnimation;
		int							bInfoEnabled;
		CFixVector					vHitPos;
		int							nHitType;
		int							nHitSeg;
		tFVIData						hitData;
		short							nBelievedPlayerSeg;
		CFixVector					vBelievedPlayerPos;
		CFixVector					vLastPlayerPosFiredAt;
		fix							nDistToLastPlayerPosFiredAt;
		CArray<tAILocalInfo>		localInfo;
		CArray<tAICloakInfo>		cloakInfo; // [MAX_AI_CLOAK_INFO];
		CArray<tPointSeg>			routeSegs; // [MAX_POINT_SEGS];
		tPointSeg*					freePointSegs;
		int							nAwarenessEvents;
		int							nMaxAwareness;
		fix							xDistToPlayer;
		CFixVector					vVecToPlayer;
		CFixVector					vGunPoint;
		int							nPlayerVisibility;
		int							bObjAnimates;
		int							nLastMissileCamera;
		CArray<tAwarenessEvent>	awarenessEvents; //[MAX_AWARENESS_EVENTS];

	public:
		CAIData ();
		bool Create (void);
		void Destroy (void);
};

inline int operator- (tAILocalInfo* o, CArray<tAILocalInfo>& a) { return a.Index (o); }
inline int operator- (tAICloakInfo* o, CArray<tAICloakInfo>& a) { return a.Index (o); }
inline int operator- (tPointSeg* o, CArray<tPointSeg>& a) { return a.Index (o); }

//------------------------------------------------------------------------------

typedef struct tSatelliteData {
	CBitmap			bmInstance;
	CBitmap			*bmP;
	CFixVector		vPos;
	CFixVector		vUp;
} tSatelliteData;

typedef struct tStationData {
	CBitmap			*bmP;
	CBitmap			**bmList [1];
	CFixVector		vPos;
	int				nModel;
} tStationData;

class CTerrainData {
	public:
		CBitmap			bmInstance;
		CBitmap			*bmP;
};

typedef struct tExitData {
	int					nModel;
	int					nDestroyedModel;
	CFixVector			vMineExit;
	CFixVector			vGroundExit;
	CFixVector			vSideExit;
	CFixMatrix			mOrient;
	short					nSegNum;
	short					nTransitSegNum;
} tExitData;

class CEndLevelData {
	public:
		tSatelliteData		satellite;
		tStationData		station;
		CTerrainData		terrain;
		tExitData			exit;
		fix					xCurFlightSpeed;
		fix					xDesiredFlightSpeed;
};

//------------------------------------------------------------------------------

class CMenuData {
	public:
		int		bValid;
		uint		tinyColors [2][2];
		uint		warnColor;
		uint		keyColor;
		uint		tabbedColor;
		uint		helpColor;
		uint		colorOverride;
		int		nLineWidth;
		ubyte		alpha;

	public:
		CMenuData ();
};

//------------------------------------------------------------------------------

#define MAX_FUEL_CENTERS    500
#define MAX_ROBOT_CENTERS   100
#define MAX_EQUIP_CENTERS   100

#include "fuelcen.h"

class CMatCenData {
	public:
		fix				xFuelRefillSpeed;
		fix				xFuelGiveAmount;
		fix				xFuelMaxAmount;
		CStaticArray< tFuelCenInfo, MAX_FUEL_CENTERS >	fuelCenters; //[MAX_FUEL_CENTERS];
		CStaticArray< tMatCenInfo, MAX_ROBOT_CENTERS >	botGens; //[MAX_ROBOT_CENTERS];
		CStaticArray< tMatCenInfo, MAX_EQUIP_CENTERS >	equipGens; //[MAX_EQUIP_CENTERS];
		int				nFuelCenters;
		int				nBotCenters;
		int				nEquipCenters;
		int				nRepairCenters;
		fix				xEnergyToCreateOneRobot;
		CStaticArray< int, MAX_FUEL_CENTERS >				origStationTypes; // [MAX_FUEL_CENTERS];
		CSegment*		playerSegP;

	public:
		CMatCenData ();
};

//------------------------------------------------------------------------------

class CDemoData {
	public:
		int				bAuto;
		char				fnAuto [FILENAME_LEN];
		CArray<sbyte>	bWasRecorded;
		CArray<sbyte>	bViewWasRecorded;
		sbyte				bRenderingWasRecorded [32];
		char				callSignSave [CALLSIGN_LEN + 1];
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

	public:
		CDemoData () {memset (this, 0, sizeof (*this)); }
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

#define GUIDEBOT_NAME_LEN 9

class CEscortData {
	public:
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

	public:
		CEscortData ();
};

//------------------------------------------------------------------------------

class CThiefData {
	public:
		CStaticArray< ubyte, MAX_STOLEN_ITEMS >	stolenItems; // [MAX_STOLEN_ITEMS];
		int				nStolenItem;
		fix				xReInitTime;
		fix				xWaitTimes [NDL];

	public:
		CThiefData ();
};

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

class CHoardData {
	public:
		int			bInitialized;
		int			nBitmaps;
		tHoardItem	orb;
		tHoardItem	icon [2];
		tHoardItem	goal;
		tHoardItem	monsterball;
		short			nMonsterballSeg;
		CFixVector	vMonsterballPos;
		CObject		*monsterballP;
		short			nLastHitter;
};

//------------------------------------------------------------------------------

#include "hudmsg.h"

class CHUDMessage {
	public:
		int					nFirst;
		int					nLast;
		int					nMessages;
		fix					xTimer;
		uint					nColor;
		char					szMsgs [HUD_MAX_MSGS][HUD_MESSAGE_LENGTH + 5];
		int					nMsgIds [HUD_MAX_MSGS];

	public:
		CHUDMessage ();
};

class CHUDData {
	public:	
		CHUDMessage			msgs [2];
		int					bPlayerMessage;

	public:
		CHUDData ();
};

//------------------------------------------------------------------------------

typedef struct {
	int	seg0, seg1, csd;
	fix	dist;
} tFCDCacheData;

#define	MAX_FCD_CACHE	64

class CFCDData {
	public:	
		int				nIndex;
		CStaticArray< tFCDCacheData, MAX_FCD_CACHE >	cache; // [MAX_FCD_CACHE];
		fix				xLastFlushTime;
		int				nConnSegDist;
};

//------------------------------------------------------------------------------

typedef struct tVertColorThreadData {
#if MULTI_THREADED_LIGHTS
	tThreadInfo		info [2];
#endif
	CVertColorData	data;
	} tVertColorThreadData;

typedef struct tClipDistData {
	CObject*				objP;
	POF::CModel*		po;
	POF::CSubModel*	pso;
	float					fClipDist [2];
	} tClipDistData;

typedef struct tClipDistThreadData {
#if MULTI_THREADED_SHADOWS
	tThreadInfo		info [2];
#endif
	tClipDistData	data;
	} tClipDistThreadData;

class CThreadData {
	public:
		tVertColorThreadData		vertColor;
		tClipDistThreadData		clipDist;
	};

//------------------------------------------------------------------------------

#if DBG
class CSpeedtestData {
	public:
		int		bOn;
		int		nMarks;
		int		nStartTime;
		int		nSegment;
		int		nSide;
		int		nFrameStart;
		int		nCount;
	};
#endif

//------------------------------------------------------------------------------

class CLaserData {
	public:
		fix		xLastFiredTime;
		fix		xNextFireTime;
		int		nGlobalFiringCount;
		int		nMissileGun;
		int		nOffset;
};

//------------------------------------------------------------------------------

class CFusionData {
	public:
		fix	xAutoFireTime;
		fix	xCharge;
		fix	xNextSoundTime;
		fix	xLastSoundTime;
};

//------------------------------------------------------------------------------

class COmegaData {
	public:
		fix		xCharge [2];
		fix		xMaxCharge;
		int		nLastFireFrame;

	public:
		COmegaData ();
};

//------------------------------------------------------------------------------

class CMissileData {
	public:
		fix		xLastFiredTime;
		fix		xNextFireTime;
		int		nGlobalFiringCount;
};

//------------------------------------------------------------------------------

typedef struct tPlayerStats {
	int	nShots [2];
	int	nHits [2];
	int	nMisses [2];
	} tPlayerStats;

class CStatsData {
	public:
		tPlayerStats	player [2];	//per level/per session
		int				nDisplayMode;
	};

//------------------------------------------------------------------------------

class CCollisionData {
	public:
		int			nSegsVisited;
		CStaticArray< short, MAX_SEGS_VISITED >	segsVisited; // [MAX_SEGS_VISITED];
		tFVIHitInfo hitData;
};

//------------------------------------------------------------------------------

class CTrackIRData {
	public:
		int	x, y;
};


//------------------------------------------------------------------------------

class CScoreData {
	public:
		int nKillsChanged;
		int bNoMovieMessage;
		int nPhallicLimit;
		int nPhallicMan;
		int bWaitingForOthers;

	public:
		CScoreData ();
};

//------------------------------------------------------------------------------

typedef struct tTextIndex {
	int	nId;
	int	nLines;
	char	*pszText;
} tTextIndex;

class CTextData {
	public:
		char*			textBuffer;
		tTextIndex*	index;
		tTextIndex*	currentMsg;
		int			nMessages;
		int			nStartTime;
		int			nEndTime;
		CBitmap*		bmP;
};

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
		CSegmentData		segs;
		CWallData			walls;
		CTriggerData		trigs;
		CObjectData			objs;
		CRobotData			bots;
		CRenderData			render;
		CEffectData			eff;
		CPigData				pig;
		CModelData			models;
		CMultiplayerData	multiplayer;
		CMultiGameData		multigame;
		CMuzzleData			muzzle;
		CWeaponData			weapons;
		CMissionData		missions;
		CEntropyData		entropy;
		CReactorData		reactor;
		CMarkerData			marker;
		CStaticArray< CBossData, MAX_BOSS_COUNT >		boss; // [MAX_BOSS_COUNT];
		CAIData				ai;
		CEndLevelData		endLevel;
		CMenuData			menu;
		CMatCenData			matCens;
		CDemoData			demo;
		CEscortData			escort;
		CThiefData			thief;
		CHoardData			hoard;
		CHUDData				hud;
		CTerrainData		terrain;
		CTimeData			time;
		CFCDData				fcd;
		CVertColorData		vertColor;
		CThreadData			threads;
	#if DBG
		CSpeedtestData		speedtest;
	#endif
		CPhysicsData		physics;
		CLaserData			laser;
		CFusionData			fusion;
		COmegaData			omega;
		CMissileData		missiles;
		CCockpitData		cockpit;
		CCollisionData		collisions;
		CScoreData			score;
		CTrackIRData		trackIR;
		CStatsData			stats;
		CTextData			messages [2];
		CTextData			sounds;
		CApplicationData	app;
#if PROFILING
		tProfilerData		profiler;
#endif

	public:
		void Init (void);
		bool Create (void);
		void Destroy (void);
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

#if 0
static inline ushort WallNumS (CSide *sideP) { return (sideP)->nWall; }
static inline ushort WallNumP (CSegment *segP, short nSide) { return WallNumS ((segP)->m_sides + (nSide)); }
static inline ushort WallNumI (short nSegment, short nSide) { return WallNumP(SEGMENTS + (nSegment), nSide); }
#endif

//-----------------------------------------------------------------------------

typedef struct tGameItemInfo {
	int		offset;
	int		count;
	int		size;
} tGameItemInfo;

typedef struct {
	ushort  fileinfo_signature;
	ushort  fileinfoVersion;
	int     fileinfo_sizeof;
	char    mine_filename [15];
	int     level;
	tGameItemInfo	player;
	tGameItemInfo	objects;
	tGameItemInfo	walls;
	tGameItemInfo	doors;
	tGameItemInfo	triggers;
	tGameItemInfo	links;
	tGameItemInfo	control;
	tGameItemInfo	botGen;
	tGameItemInfo	lightDeltaIndices;
	tGameItemInfo	lightDeltas;
	tGameItemInfo	equipGen;
} tGameFileInfo;

extern tGameFileInfo gameFileInfo;

//-----------------------------------------------------------------------------

static inline short ObjIdx (CObject *objP)
{
return gameData.objs.objects.Index (objP);
}

//	-----------------------------------------------------------------------------------------------------------

#define	NO_WALL					(gameStates.app.bD2XLevel ? 2047 : 255)
#define  IS_WALL(_wallnum)		((ushort) (_wallnum) < NO_WALL)

#define SEG_IDX(_segP)			((short) ((_segP) - SEGMENTS))
#define SEG2_IDX(_seg2P)		((short) ((_seg2P) - SEGMENTS))
#define WALL_IDX(_wallP)		((short) ((_wallP) - WALLS))
#define OBJ_IDX(_objP)			ObjIdx (_objP)
#define TRIG_IDX(_triggerP)	((short) ((_triggerP) - TRIGGERS))
#define FACE_IDX(_faceP)		((int) ((_faceP) - FACES.faces))

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

#define MAX_LIGHT_RANGE	I2X (125)

#define SEGVIS(_i,_j)	((gameData.segs.bSegVis [SEGVIS_FLAGS * (_i) + ((_j) >> 3)] & (1 << ((_j) & 7))) != 0)

//	-----------------------------------------------------------------------------------------------------------

extern float fInfinity [];
extern fix nDebrisLife [];

#define sizeofa(_a)	(sizeof (_a) / sizeof ((_a) [0]))	//number of array elements

#define CLEAR(_v)		memset (_v, 0, sizeof (_v))

#define SEGMENTS		gameData.segs.segments
#define SEGFACES		gameData.segs.segFaces
#define OBJECTS		gameData.objs.objects
#define WALLS			gameData.walls.walls
#define TRIGGERS		gameData.trigs.triggers
#define OBJTRIGGERS	gameData.trigs.objTriggers
#define FACES			gameData.segs.faces
#define TRIANGLES		FACES.tris

// limit framerate to 30 while recording demo and to 40 when in automap and framerate display is disabled
#define MAXFPS		((gameData.demo.nState == ND_STATE_RECORDING) ? 30 : \
                   (automap.m_bDisplay && !(gameStates.render.automap.bRadar || (gameStates.render.bShowFrameRate == 1))) ? 40 : \
                   gameOpts->render.nMaxFPS)

#define SPECTATOR(_objP)	(gameStates.app.bFreeCam && (OBJ_IDX (_objP) == LOCALPLAYER.nObject))
#define OBJPOS(_objP)		(SPECTATOR (_objP) ? &gameStates.app.playerPos : &(_objP)->info.position)
#define OBJSEG(_objP)		(SPECTATOR (_objP) ? gameStates.app.nPlayerSegment : (_objP)->info.nSegment)

//	-----------------------------------------------------------------------------

static inline CFixVector *PolyObjPos (CObject *objP, CFixVector *vPosP)
{
CFixVector vPos = OBJPOS (objP)->vPos;
if (objP->info.renderType == RT_POLYOBJ) {
	*vPosP = *ObjectView (objP) * gameData.models.offsets [objP->rType.polyObjInfo.nModel];
	*vPosP += vPos;
	return vPosP;
	}
*vPosP = vPos;
return vPosP;
}

//	-----------------------------------------------------------------------------

inline void CObject::RequestEffects (ubyte nEffects)
{
gameData.objs.bWantEffect [OBJ_IDX (this)] |= nEffects;
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

typedef struct tTransRotInfo {
	fVector3D	fvRot;
	fVector3D	fvTrans;
	} tTransRotInfo;

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
typedef int (WINAPI *tpfnTIRQuery) (tTransRotInfo *);

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

template<typename _T> 
inline void Swap (_T& a, _T& b) { _T h = a; a = b; b = h; }

//	-----------------------------------------------------------------------------------------------------------

#ifndef min
#	define min(_a,_b)	((_a) <= (_b) ? (_a) : (_b))
#endif

#ifndef max
#	define max(_a,_b)	((_a) >= (_b) ? (_a) : (_b))
#endif

void CheckEndian (void);

//	-----------------------------------------------------------------------------------------------------------

#define LEVEL_SEGMENTS			gameData.segs.nSegments
#define LEVEL_OBJECTS			(gameData.objs.nMaxObjects)
#define LEVEL_VERTICES			(gameData.segs.nVertices)
#define LEVEL_POINT_SEGS		(gameData.segs.nSegments * 4)
#define LEVEL_SIDES				(LEVEL_SEGMENTS * 6)
#define LEVEL_FACES				(LEVEL_SIDES * 2)
#define LEVEL_TRIANGLES			(LEVEL_FACES * 16)
#define LEVEL_DL_INDICES		(LEVEL_SEGMENTS / 2)
#define LEVEL_DELTA_LIGHTS		(LEVEL_SEGMENTS * 10)
#define LEVEL_SEGVIS_FLAGS		((LEVEL_SEGMENTS + 7) >> 3)
#define LEVEL_VERTVIS_FLAGS	((LEVEL_VERTICES + 7) >> 3)

//	-----------------------------------------------------------------------------------------------------------

#endif


