/* $Id: inferno.h,v 1.3 2003/10/10 09:36:35 btb Exp $ */
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

#include "irrstuff.h"

#define SHOW_EXIT_PATH  1

#define MAX_SUBMODELS	10		// how many animating sub-objects per model

#define MULTI_THREADED_SHADOWS	0
#define MULTI_THREADED_LIGHTS		0
#define MULTI_THREADED_PRECALC	1

#define USE_SEGRADS	0
#define CALC_SEGRADS	1

#ifdef _DEBUG
#	define	SHADOWS	1
#else
#	define	SHADOWS	1
#endif

#ifdef _DEBUG
#	define	PROFILING 1
#else
#	define	PROFILING 0
#endif

#if SHADOWS
#	ifdef _DEBUG
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
	int bUseLightMaps;
	int nLightMapRange;
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

typedef struct tSmokeOptions {
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
	int bCollisions;
	int bDisperse;
	int bRotate;
	int bSort;
	int bDecreaseLag;	//only render if tPlayer is moving forward
	int bAuxViews;
} tSmokeOptions;

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
	int bSmoke;
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

typedef struct tRenderOptions {
	int bAllSegs;
	int bDynamicLight;
	int bDynLighting;
	int bObjects;
	int bTextures;
	int bWalls;
	int bWireFrame;
	int bOptimize;
	int bTransparentEffects;
	int bExplBlast;
	int nExplShrapnels;
	int bCoronas;
	int nCoronaStyle;
	int bShotCoronas;
	int bWeaponCoronas;
	int bPowerupCoronas;
	int bAdditiveCoronas; //additive corona blending for wall lights 
	int bAdditiveObjCoronas; //additive corona blending for light emitting weapons
	int nCoronaIntensity;
	int nObjCoronaIntensity;
	int bEnergySparks;
	int bUseShaders;
	int bHiresModels;
	int bBrightObjects;
	int bRobotShields;
	int bOnlyShieldHits;
	int bAutoTransparency;
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
	tSmokeOptions smoke;
	tLightningOptions lightnings;
	tShadowOptions shadows;
	tPowerupOptions powerups;
	tAutomapOptions automap;
	tShipRenderOptions ship;
} tRenderOptions;

//------------------------------------------------------------------------------

typedef struct tOglOptions {
	int bSetGammaRamp;
	int bGlTexMerge;
	int bLightObjects;
	int bLightPowerups;
	int bGeoLighting;
	int bObjLighting;
	int bHeadLight;
	int nMaxLights;
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
	int bHeadLightOnWhenPickedUp;
	int bShieldWarning;
	int bInventory;
	int bIdleAnims;
	int nAIAwareness;
	int nSlowMotionSpeedup;
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
	int bD1Sound;
	int bHires;
	int bUseSDLMixer;
	int bUseOpenAL;
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
	unsigned char 	nBufferType;		// 0=No buffer, 1=buffer ASCII, 2=buffer scans
	unsigned char 	bRepeat;
	unsigned char 	bEditorMode;
	unsigned char 	nLastPressed;
	unsigned char 	nLastReleased;
	unsigned char	pressed [256];
	volatile int	xLastPressTime;
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
	int bUseRender2Texture;
	int bDrawBufferActive;
	int bReadBufferActive;
	int bFullScreen;
	int bLastFullScreen;
	int bUseTransform;
	int bGlTexMerge;
	int bBrightness;
	int bDoPalStep;
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
	int bCurFullScreen;
	int nReticle;
	int bpp;
	int bScaleLight;
	int bDynObjLight;
	int bVertexLighting;
	int bHeadLight;
	int bStandardContrast;
	int nRGBAFormat;
	int nRGBFormat;
	int bIntensity4;
	int bLuminance4Alpha4;
	int bReadPixels;
	int bOcclusionQuery;
	int bDepthBlending;
	int bGetTexLevelParam;
	int nStencil;
#ifdef GL_ARB_multitexture
	int bArbMultiTexture;
#endif
#ifdef GL_SGIS_multitexture
	int bSgisMultiTexture;
#endif
	tRgbColorb bright;
	tRgbColorf fBright;
	tRgbColors palAdd;
	float fAlpha;
	float fLightRange;
} tOglStates;

//------------------------------------------------------------------------------

typedef struct tCameraStates {
	int bActive;
} tCameraStates;

//------------------------------------------------------------------------------

typedef struct tColorStates {
	int bLightMapsOk;
	int bRenderLightMaps;
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
	gsrCanvas  *offscreen;			// The offscreen data buffer
	gsrCanvas	render [2];			//  Two offscreen buffers for left/right eyes.
	gsrCanvas	subRender [2];		//  Two sub buffers for left/right eyes.
	gsrCanvas	screenPages [2];	//  Two pages of VRAM if paging is available
	gsrCanvas	editorCanvas;		//  The canvas that the editor writes to.
} tVRBuffers;

typedef struct tVRStates {
	u_int32_t	nScreenMode;
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
	grsBitmap	*bmBot;
	grsBitmap	*bmTop;
	grsBitmap	*bmMask;
	ubyte			bSuperTransp;
	ubyte			bShaderMerge;
	int			bOverlay;
	int			bColored;
	int			nShader;
} tRenderHistory;

typedef struct tRenderStates {
	int bTopDownRadar;
	int bExternalView;
	int bQueryOcclusion;
	int bPointSprites;
	int bVertexArrays;
	int bAutoMap;
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
	int bTransparency;
	int bSplitPolys;
	int bHaveDynLights;
	int bPaletteFadedOut;
	int bHaveStencilBuffer;
	int nRenderPass;
	int nShadowPass;
	int nShadowBlurPass;
	int bShadowMaps;
	int bLoResShadows;
	int bUseCameras;
	int bUseDynLight;
	int bApplyDynLight;
	int nSoften;
	int bHeadLightOn;
	int bHaveSkyBox;
	int bAllVisited;
	int bViewDist;
	int bD2XLights;
	int bRendering;
	int bFullBright;
	int bQueryCoronas;
	int bDoLightMaps;
	int bDoCameras;
	int bRenderIndirect;
	int bBuildModels;
	int nFrameFlipFlop;
	int nModelQuality;
	int nState;	//0: render geometry, 1: render objects
	int nType;
	int nFrameCount;
	fix xZoom;
	fix xZoomScale;
	ubyte nRenderingType;
	fix nFlashScale;
	fix nFlashRate;
	cvar_t frameRate;
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
	int bGameRunning;
	int bGameSuspended;
	int bGameAborted;
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
	int bUseSwapFile;
	int bSingleStep;
	int bAutoDemos;	//automatically play demos or intro movie if user is idling in the main menu
	fix xThisLevelTime;
	fix nPlayerTimeOfDeath;
	char *szCurrentMission;
	char *szCurrentMissionFile;
	tPosition playerPos;
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
#ifdef _DEBUG
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
	tFaceColor	*lights;
	tFaceColor	*sides;
	tFaceColor	*segments;
	tFaceColor	*vertices;
	float			*vertBright;
	tFaceColor	*ambient;	//static light values
	tFaceColor	textures [MAX_WALL_TEXTURES];
	tLightRef	*visibleLights;
	int			nVisibleLights;
	tRgbColorf	flagTag;
} tColorData;

//------------------------------------------------------------------------------

typedef struct tPulseData {
	float			fScale;
	float			fMin;
	float			fDir;
	float			fSpeed;
} tPulseData;

typedef struct tSphereData {
	tOOF_vector	*pSphere;
	int			nTessDepth;
	int			nFaces;
	int			nFaceNodes;
	tPulseData	pulse;
	tPulseData	*pPulse;
} tSphereData;

//------------------------------------------------------------------------------

#define USE_OGL_LIGHTS	0
#define MAX_OGL_LIGHTS  (64 * 64) //MUST be a power of 2!

typedef struct tDynLight {
	vmsVector	vPos;
	vmsVector	vDir;
	tRgbaColorf	color;
	float			brightness;
	float			range;
	float			rad;
#if USE_OGL_LIGHTS
	unsigned		handle;
	fVector		fAttenuation;	// constant, linear quadratic
	fVector		fAmbient;
	fVector		fDiffuse;
#endif
	fVector		fSpecular;
	fVector		fEmissive;
	float			spotAngle;
	float			spotExponent;
	short			nSegment;
	short			nSide;
	short			nObject;
	short			nVerts [4];
	ubyte			nPlayer;
	ubyte			nType;
	ubyte			bState;
	ubyte			bOn;
	ubyte			bSpot;
	ubyte			bTransform;
	ubyte			bVariable;
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

typedef struct tShaderLight {
	vmsVector	vPos;
	fVector		pos [2];
	fVector		dir;
	fVector		color;
	float			rad;
	float			brightness;
	float			range;
	float			spotAngle;
	float			spotExponent;
	fix			xDistance;
	short			nSegment;
	short			nObject;
	short			nVerts [4];
	ubyte			nType;
	ubyte			bState;
	ubyte			bVariable;
	ubyte			bOn;
	ubyte			bSpot;
	ubyte			bShadow;
	ubyte			bLightning;
	ubyte			bExclusive;
} tShaderLight;

//------------------------------------------------------------------------------

typedef struct tShaderLightData {
	tShaderLight	lights [MAX_OGL_LIGHTS];
	int				nLights;
	tShaderLight	*activeLights [4][MAX_OGL_LIGHTS];
	int				nActiveLights [4];
	int				iVariableLights [4];
	int				iStaticLights [4];
	GLuint			nTexHandle;
} tShaderLightData;

//------------------------------------------------------------------------------

#define MAX_NEAREST_LIGHTS 32

typedef struct tHeadLightData {
	tDynLight			*pl [MAX_PLAYERS];
	tShaderLight		*psl [MAX_PLAYERS];
	fVector3				pos [MAX_PLAYERS];
	fVector3				dir [MAX_PLAYERS];
	float					brightness [MAX_PLAYERS];
	int					nLights;
} tHeadLightData;

typedef struct tDynLightData {
	tDynLight			lights [MAX_OGL_LIGHTS];
	short					*nNearestSegLights;	//the 8 nearest static lights for every tSegment
	short					*nNearestVertLights;	//the 8 nearest static lights for every tSegment
	short					*owners;
	short					nLights;
	short					nDynLights;
	short					nVertLights;
	short					nHeadLights [MAX_PLAYERS];
	short					nSegment;
	tShaderLightData	shader;
	tHeadLightData		headLights;
	tOglMaterial		material;
	tFrameBuffer		fb;
} tDynLightData;

extern int nMaxNearestLights [21];

//------------------------------------------------------------------------------

//Flickering light system
typedef struct tVariableLight {
	short				nSegment;
	short				nSide;
	unsigned int	mask;     // determines flicker pattern
	fix				timer;    // time until next change
	fix				delay;    // time between changes
} tVariableLight;

#define MAX_FLICKERING_LIGHTS 1000

typedef struct tFlickerLightData {
	tVariableLight	lights [MAX_FLICKERING_LIGHTS];
	int					nLights;
} tFlickerLightData;

//------------------------------------------------------------------------------

typedef struct tLightData {
	int					nStatic;
	int					nCoronas;
	fix					*segDeltas;
	tLightDeltaIndex				*deltaIndices;
	tLightDelta			*deltas;
	ubyte					*subtracted;
	tDynLightData		dynamic;
	tFlickerLightData	flicker;
	fix					*dynamicLight;
	tRgbColorf			*dynamicColor;
	char					*bGotDynColor;
	char					bGotGlobalDynColor;
	char					bStartDynColoring;
	char					bInitDynColoring;
	tRgbColorf			globalDynColor;
	short					*vertices;
	sbyte					*vertexFlags;
	sbyte					*newObjects;
	sbyte					*objects;
	GLuint				*coronaQueries;
	GLuint				*coronaSamples;
} tLightData;

//------------------------------------------------------------------------------

typedef struct tShadowData {
	short				nLight;
	short				nLights;
	tShaderLight	*pLight;
	short				nShadowMaps;
	tCamera			shadowMaps [MAX_SHADOW_MAPS];
	tObject			lightSource;
	tOOF_vector		vLightPos;
	vmsVector		vLightDir [MAX_SHADOW_LIGHTS];
	short				*objLights;
	ubyte				nFrame;	//flipflop for testing whether a light source's view has been rendered the current frame
} tShadowData;

//------------------------------------------------------------------------------

#include "morph.h"

typedef struct tMorphData {
	tMorphInfo	objects [MAX_MORPH_OBJECTS];
	fix			xRate;
} tMorphData;

//------------------------------------------------------------------------------

#define	MAX_COMPUTED_COLORS	64

typedef struct {
	ubyte		r,g,b,nColor;
} color_record;

typedef struct tPaletteList {
	tPalette					palette;
	color_record			computedColors [MAX_COMPUTED_COLORS];
	short						nComputedColors;
	struct tPaletteList	*pNextPal;
} tPaletteList;

typedef struct tPaletteData {
	struct tPaletteList	*palettes;
	int						nPalettes;
	ubyte						*pCurPal;
} tPaletteData;

//------------------------------------------------------------------------------

#define OGLTEXBUFSIZE (2048*2048*4)

typedef struct tOglData {
	GLubyte					texBuf [OGLTEXBUFSIZE];
	ubyte						*palette;
	GLenum					nSrcBlend;
	GLenum					nDestBlend;
	float						zNear;
	float						zFar;
	fVector3					depthScale;
	struct {float x, y;}	screenScale;
	tFrameBuffer			drawBuffer;
	int						nHeadLights;
} tOglData;

//------------------------------------------------------------------------------

#define TERRAIN_GRID_MAX_SIZE   64
#define TERRAIN_GRID_SCALE      i2f (2*20)
#define TERRAIN_HEIGHT_SCALE    f1_0

typedef struct tTerrainRenderData {
	ubyte			*pHeightMap;
	fix			*pLightMap;
	vmsVector	*pPoints;
	grsBitmap	*bmP;
	g3sPoint	saveRow [TERRAIN_GRID_MAX_SIZE];
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
	grsFace					*pFaceRenderList [MAX_SEGMENTS_D2X * 6];
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
	fVector	vertNorm;
	fVector	colorSum;
	fVector	*pVertPos;
	float		fMatShininess;
	} tVertColorData;

typedef struct tRenderData {
	tColorData				color;
	int						transpColor;
	tVertColorData			vertColor;
	tSphereData				shield;
	tSphereData				monsterball;
	int						nPaletteGamma;
	int						nComputedColors;
	fix						xFlashEffect;
	fix						xTimeFlashLastPlayed;
	fVector					*pVerts;
	fVector					*vertexList;
	tLightData				lights;
	tMorphData				morph;
	tShadowData				shadows;
	tPaletteData			pal;
	tOglData					ogl;
	tTerrainRenderData	terrain;
	tThrusterData			thrusters [MAX_PLAYERS];
	tMineRenderData		mine;
	tGameWindowData		window;
	fix						zMin;
	fix						zMax;
	tFrameBuffer			glareBuffer;
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

typedef struct tFaceData {
	grsFace				*faces;
	fVector3				*vertices;
	fVector3				*normals;
	tTexCoord2f			*texCoord;
	tTexCoord2f			*ovlTexCoord;
	tRgbaColorf			*color;
	} tFaceData;

typedef struct tSegList {
	int					nSegments;
	short					*segments;
} tSegList;

typedef struct tSegExtent {
	vmsVector			vMin;
	vmsVector			vMax;
	} tSegExtent;

typedef struct tSegmentData {
	int					nMaxSegments;
	vmsVector			*vertices;
	fVector				*fVertices;
	tSegment				*segments;
	tSegment2			*segment2s;
	xsegment				*xSegments;
	tSegFaces			*segFaces;
	g3sPoint				*points;
	tSegList				skybox;
#if CALC_SEGRADS
	fix					*segRads [2];
	tSegExtent			*extent;
#endif
	vmsVector			*segCenters [2];
	vmsVector			*sideCenters;
	ubyte					*bVertVis;
	ubyte					*bSegVis;
	int					nVertices;
	int					nLastVertex;
	int					nSegments;
	int					nLastSegment;
	int					nFaces;
	int					nLevelVersion;
	char					szLevelFilename [FILENAME_LEN];
	tSecretData			secret;
	tSlideSegs			*slideSegs;
	short					nSlideSegs;
	int					bHaveSlideSegs;
	tFaceData			faces;
} tSegmentData;

//------------------------------------------------------------------------------

typedef struct tWallData {
	tWall					walls [MAX_WALLS];
	tExplWall			explWalls [MAX_EXPLODING_WALLS];
	tActiveDoor			activeDoors [MAX_DOORS];
	tCloakingWall		cloaking [MAX_CLOAKING_WALLS];
	tWallClip			anims [2][MAX_WALL_ANIMS];
	int					bitmaps [MAX_WALL_ANIMS];
	int					nWalls;
	int					nOpenDoors; 
	int					nCloaking;
	int					nAnims [2];
	tWallClip			*pAnims;
} tWallData;

//------------------------------------------------------------------------------

typedef struct tTriggerData {
	tTrigger				triggers [MAX_TRIGGERS];
	tTrigger				objTriggers [MAX_TRIGGERS];
	tObjTriggerRef		objTriggerRefs [MAX_OBJ_TRIGGERS];
	short					firstObjTrigger [MAX_OBJECTS_D2X];
	int					delay [MAX_TRIGGERS];
	int					nTriggers;
	int					nObjTriggers;
} tTriggerData;

//------------------------------------------------------------------------------

typedef struct tPowerupData {
	powerupType_info info [MAX_POWERUP_TYPES];
	int					nTypes;
} tPowerupData;

//------------------------------------------------------------------------------

typedef struct tObjTypeData {
	int					nTypes;
	sbyte					nType [MAX_OBJTYPE];
	sbyte					nId [MAX_OBJTYPE];  
	fix					nStrength [MAX_OBJTYPE];   
} tObjTypeData;

//------------------------------------------------------------------------------

typedef struct tSpeedBoostData {
	int					bBoosted;
	vmsVector			vVel;
	vmsVector			vMinVel;
	vmsVector			vMaxVel;
	vmsVector			vSrc;
	vmsVector			vDest;
} tSpeedBoostData;

//------------------------------------------------------------------------------

typedef struct tQuad {
	vmsVector			v [4];	//corner vertices
	vmsVector			n [2];	//normal, transformed normal
#ifdef _DEBUG
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

typedef struct tShrapnelData {
	int			nShrapnels;
	tShrapnel	*shrapnels;
} tShrapnelData;

//------------------------------------------------------------------------------

typedef struct tObjectViewData {
	vmsMatrix			mView;
	int					nFrame;
} tObjectViewData;

typedef struct tObjectData {
	tObjTypeData		types;
	tObject				*objects;
	short					*freeList;
	short					*parentObjs;
	tObjectRef			*childObjs;
	short					*firstChild;
	tObject				*init;
	tObjDropInfo		*dropInfo;
	tSpeedBoostData	*speedBoost;
	vmsVector			*vRobotGoals;
	vmsVector			*vStartVel;
	fix					*xLastAfterburnerTime;
	fix					*xCreationTime;
	fix					*xTimeLastHit;
	fix					*xLight;
	int					*nLightSig;
	ubyte					*nTracers;
	tFaceColor			color;
	short					nFirstDropped;
	short					nLastDropped;
	short					nFreeDropped;
	short					nDropped;
	ushort				*cameraRef;
	tObject				*guidedMissile [MAX_PLAYERS];
	int					guidedMissileSig [MAX_PLAYERS];
	tObject				*console;
	tObject				*viewer;
	tObject				*trackGoals [2];
	tObject				*missileViewer;
	tObject				*deadPlayerCamera;
	tObject				*endLevelCamera;
	int					nObjects;
	int					nLastObject;
	int					nObjectLimit;
	int					nMaxUsedObjects;
	int					nNextSignature;
	int					nChildFreeList;
	int					nDrops;
	int					nDeadControlCenter;
	int					nVertigoBotFlags;
	short					*nHitObjects;
	tShrapnelData		*shrapnels;
	tPowerupData		pwrUp;
	ubyte					collisionResult [MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];
	tObjectViewData	*viewData;
	ubyte					bIsMissile [100];
	ubyte					bIsWeapon [100];
	ubyte					bIsSlowWeapon [100];
	short					idToOOF [100];
	int					nFrameCount;
} tObjectData;

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
	short					*ignoreObjs;
	tFVISideData		side;
	fix					xTime;
	fix					xAfterburnerCharge;
	fix					xBossInvulDot;
} tPhysicsData;

//------------------------------------------------------------------------------

typedef struct tPOF_face {
	short					nVerts;
	short					*pVerts;
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
	tPOF_face			*pFaces;
} tPOF_faceList;

typedef struct tPOF_faceRef {
	short					nFaces;
	tPOF_face			**pFaces;
} tPOF_faceRef;

typedef struct tPOFSubObject {
	tPOF_faceList		faces;
	tPOF_faceRef		litFaces;	//submodel faces facing the current light source
	vmsVector			vPos;
	vmsAngVec			vAngles;
	float					fClipDist;
	short					nParent;
	short					*pAdjFaces;
	short					nRenderFlipFlop;
	short					bCalcClipDist;
} tPOFSubObject;

typedef struct tPOF_subObjList {
	short					nSubObjs;
	tPOFSubObject		*pSubObjs;
} tPOF_subObjList;

typedef struct tPOFObject {
	tPOF_subObjList	subObjs;
	short					nVerts;
	vmsVector			*pvVerts;
	tOOF_vector			*pvVertsf;
	float					*pfClipDist;
	ubyte					*pVertFlags;
	g3sNormal			*pVertNorms;
	vmsVector			vCenter;
	vmsVector			*pvRotVerts;
	tPOF_faceList		faces;
	tPOF_faceRef		litFaces;
	short					nAdjFaces;
	short					*pAdjFaces;
	short					*pFaceVerts;
	short					*pVertMap;
	short					iSubObj;
	short					iVert;
	short					iFace;
	short					iFaceVert;
	char					nState;
	ubyte					nVertFlag;
} tPOFObject;

//	-----------------------------------------------------------------------------

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
	ubyte						bGlow :1;
	ubyte						bThruster :1;
} tG3ModelFace;

typedef struct tG3SubModel {
#ifdef _DEBUG
	char						szName [256];
#endif
	vmsVector				vOffset;
	vmsVector				vCenter;
	fVector3					vMin;
	fVector3					vMax;
	tG3ModelFace			*pFaces;
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
	grsBitmap				*pTextures;
	int						teamTextures [8];
	fVector3					*pVerts;
	fVector3					*pVertNorms;
	tFaceColor				*pColor;
	tG3ModelVertex			*pFaceVerts;
	tG3ModelVertex			*pSortedVerts;
	char						*pVBData;
	tTexCoord2f				*pVBTexCoord;
	tRgbaColorf				*pVBColor;
	fVector3					*pVBVerts;
	fVector3					*pVBNormals;
	tG3SubModel				*pSubModels;
	tG3ModelFace			*pFaces;
	tG3RenderVertex		*pVertBuf [2];
	short						*pIndex [2];
	short						nGunSubModels [MAX_GUNS];
	float						fScale;
	short						nFaces;
	short						iFace;
	short						nVerts;
	short						nFaceVerts;
	short						iFaceVert;
	short						nSubModels;
	short						nTextures;
	short						iSubModel;
	short						bHasTransparency;
	short						bValid;
	short						bRendered;
	short						bBullets;
	vmsVector				vBullets;
	GLuint					vboDataHandle;
	GLuint					vboIndexHandle;
} tG3Model;

//------------------------------------------------------------------------------

#define MAX_POLYGON_VERTS 1000

typedef struct tRobotData {
	char					*robotNames [MAX_ROBOT_TYPES][ROBOT_NAME_LENGTH];
	tRobotInfo			info [2][MAX_ROBOT_TYPES];
	tRobotInfo			defaultInfo [MAX_ROBOT_TYPES];
	tJointPos			joints [MAX_ROBOT_JOINTS];
	tJointPos			defaultJoints [MAX_ROBOT_JOINTS];
	int					nJoints;
	int					nDefaultJoints;
	int					nCamBotId;
	int					nCamBotModel;
	int					nTypes [2];
	int					nDefaultTypes;
	int					bReplacementsLoaded;
	tRobotInfo			*pInfo;
	tPOFObject			*pPofData;
} tRobotData;

#define D1ROBOT(_id)		(gameStates.app.bD1Mission && ((_id) < gameData.bots.nTypes [1]))
#define ROBOTINFO(_id)	gameData.bots.info [D1ROBOT (_id)][_id]

//------------------------------------------------------------------------------

#if USE_OPENAL

typedef struct tOpenALData {
	ALCdevice			*device;
	ALCcontext			*context;
} tOpenALData;

#endif

typedef struct tSoundData {
	ubyte					*data [2];
	tDigiSound			sounds [2][MAX_SOUND_FILES];
	int					nSoundFiles [2];
	tDigiSound			*pSounds;
#if USE_OPENAL
	tOpenALData			openAL;
#endif
} tSoundData;

//------------------------------------------------------------------------------

#define N_COCKPIT_BITMAPS 6
#define D1_N_COCKPIT_BITMAPS 4

typedef struct tTextureData {
	tBitmapFile			bitmapFiles [2][MAX_BITMAP_FILES];
	sbyte					bitmapFlags [2][MAX_BITMAP_FILES];
	grsBitmap			bitmaps [2][MAX_BITMAP_FILES];
	grsBitmap			altBitmaps [2][MAX_BITMAP_FILES];
	grsBitmap			addonBitmaps [MAX_ADDON_BITMAP_FILES];
	ushort				bitmapXlat [MAX_BITMAP_FILES];
	alias					aliases [MAX_ALIASES];
	tBitmapIndex		bmIndex [2][MAX_TEXTURES];
	tBitmapIndex		objBmIndex [MAX_OBJ_BITMAPS];
	short					textureIndex [2][MAX_BITMAP_FILES];
	ushort				pObjBmIndex [MAX_OBJ_BITMAPS];
	tBitmapIndex		cockpitBmIndex [N_COCKPIT_BITMAPS];
	tRgbColorf			bitmapColors [MAX_BITMAP_FILES];
	int					nBitmaps [2];
	int					nObjBitmaps;
	int					bPageFlushed;
	tTexMapInfo      	tMapInfo [2][MAX_TEXTURES];
	int					nExtraBitmaps;
	int					nAliases;
	int					nHamFileVersion;
	int					nTextures [2];
	int					nFirstMultiBitmap;
	tBitmapFile			*pBitmapFiles;
	grsBitmap			*pBitmaps;
	grsBitmap			*pAltBitmaps;
	tBitmapIndex		*pBmIndex;
	tTexMapInfo			*pTMapInfo;
	ubyte					*rleBuffer;
	int					brightness [MAX_WALL_TEXTURES];
} tTextureData;

//------------------------------------------------------------------------------

typedef struct tEffectData {
	tEffectClip			effects [2][MAX_EFFECTS];
	tVideoClip 			vClips [2][VCLIP_MAXNUM];
	int					nEffects [2];
	int 					nClips [2];
	tEffectClip			*pEffects;
	tVideoClip			*pVClips;
} tEffectData;

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

typedef struct tPigData {
	tTextureData		tex;
	tSoundData			sound;
	tShipData			ship;
	tFlagData			flags [2];
} tPigData;

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
	sbyte					nPrimary;
	sbyte					nSecondary;
	sbyte					nOverridden;
	sbyte					bTripleFusion;
	tFiringData			firing [2];
	int					nTypes [2];
	tWeaponInfo			info [MAX_WEAPON_TYPES];
	tD1WeaponInfo		infoD1 [D1_MAX_WEAPON_TYPES];
	tRgbaColorf			*color;
	ubyte					bLastWasSuper [2][MAX_PRIMARY_WEAPONS];
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
#ifdef _DEBUG
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
	tOOFObject			*modelToOOF [2][MAX_POLYGON_MODELS];
	tASEModel			*modelToASE [2][MAX_POLYGON_MODELS];
	tPolyModel			*modelToPOL [MAX_POLYGON_MODELS];
	int					nPolyModels;
	int					nDefPolyModels;
	g3sPoint				polyModelPoints [MAX_POLYGON_VERTS];
	fVector				fPolyModelVerts [MAX_POLYGON_VERTS];
	grsBitmap			*textures [MAX_POLYOBJ_TEXTURES];
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
	int					nScale;
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
	tObject				*spitterP;
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
	int 						nPlayers;				
	int						nMaxPlayers;
	int 						nLocalPlayer;				
	int						nPlayerPositions;
	int						bMoving;
	tPlayer					players [MAX_PLAYERS + 4];  
	tObjPosition			playerInit [MAX_PLAYERS];
	short						nVirusCapacity [MAX_PLAYERS];
	int						nLastHitTime [MAX_PLAYERS];
	tWeaponState			weaponStates [MAX_PLAYERS];
	char						bWasHit [MAX_PLAYERS];
	int						bulletEmitters [MAX_PLAYERS];
	tPulseData				spherePulse [MAX_PLAYERS];
	ubyte						powerupsInMine [MAX_POWERUP_TYPES];
	ubyte						powerupsOnShip [MAX_POWERUP_TYPES];
	ubyte						maxPowerupsAllowed [MAX_POWERUP_TYPES];
	tLeftoverPowerup		*leftoverPowerups;
	tAutoNetGame			autoNG;
	fix						xStartAbortMenuTime;
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
	short					*remoteToLocal;  // Remote tObject number for each local tObject
	short					*localToRemote;
	sbyte					*nObjOwner;   // Who created each tObject in my universe, -1 = loaded at start
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
	fix					xSlack;
	fix					xStarted;
	fix					xStopped;
	int					nPaused;
	int					nStarts;
	int					nStops;
} tTimeData;

//------------------------------------------------------------------------------

typedef struct tApplicationData {
	int					nFrameCount;
	int					nGameMode;
	int					bGamePaused;
	uint					nStateGameId;
int						nLifetimeChecksum;
} tApplicationData;

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
#ifdef _DEBUG
	fix					xPrevShields;
#endif
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
	int					bInitialized;
	int					nOverallAgitation;
	int					bEvaded;
	int					bEnableAnimation;
	int					bInfoEnabled;
	vmsVector			vHitPos;
	int					nHitType;
	int					nHitSeg;
	tFVIData				hitData;
	short					nBelievedPlayerSeg;
	vmsVector			vBelievedPlayerPos;
	vmsVector			vLastPlayerPosFiredAt;
	fix					nDistToLastPlayerPosFiredAt;
	tAILocal				*localInfo;
	tAICloakInfo		cloakInfo [MAX_AI_CLOAK_INFO];
	tPointSeg			pointSegs [MAX_POINT_SEGS];
	tPointSeg			*freePointSegs;
	int					nAwarenessEvents;
	int					nMaxAwareness;
	fix					xDistToPlayer;
	vmsVector			vVecToPlayer;
	vmsVector			vGunPoint;
	int					nPlayerVisibility;
	int					bObjAnimates;
	int					nLastMissileCamera;
	tAwarenessEvent	awarenessEvents [MAX_AWARENESS_EVENTS];
} tAIData;

//------------------------------------------------------------------------------

typedef struct tSatelliteData {
	grsBitmap			bmInstance;
	grsBitmap			*bmP;
	vmsVector			vPos;
	vmsVector			vUp;
} tSatelliteData;

typedef struct tStationData {
	grsBitmap			*bmP;
	grsBitmap			**bmList [1];
	vmsVector			vPos;
	int					nModel;
} tStationData;

typedef struct tTerrainData {
	grsBitmap			bmInstance;
	grsBitmap			*bmP;
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
	unsigned int		tinyColors [2][2];
	unsigned int		warnColor;
	unsigned int		keyColor;
	unsigned int		tabbedColor;
	unsigned int		helpColor;
	unsigned int		colorOverride;
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
	short				nFrame;
	fix				xSize;
	time_t			tRender;
	time_t			tCreate;
	vmsVector		vPos;
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
	tSegment			*playerSegP;
} tMatCenData;

//------------------------------------------------------------------------------

typedef struct tDemoData {
	int				bAuto;
	char				fnAuto [FILENAME_LEN];

	sbyte				*bWasRecorded;
	sbyte				*bViewWasRecorded;
	sbyte				bRenderingWasRecorded [32];

	char				callSignSave [CALLSIGN_LEN+1];
	int				nVersion;
	int				nState;
	int				nVcrState;
	int				nStartFrame;
	unsigned int	nSize;
	unsigned int	nWritten;
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

#include "particles.h"

typedef struct tPartList {
	struct tPartList	*pNextPart;
	tParticle			*pParticle;
	float					fBrightness;
} tPartList;

typedef struct tPartDepthBuf {
	tPartList		**pDepthBuffer;
	tPartList		*pPartList;
	int				nParts;
	int				nFreeParts;
	int				zMin;
	int				zMax;
} tPartDepthBuf;

typedef struct tSmokeData {
	tSmoke			buffer [MAX_SMOKE];
	short				*objects;
	time_t			*objExplTime;
	int				iFree;
	int				iUsed;
	int				nLastType;
	int				bAnimate;
	int				bStencil;
	tPartDepthBuf	depthBuf;
} tSmokeData;

//------------------------------------------------------------------------------

#define MAX_LIGHTNINGS	1000
#define MAX_LIGHTNING_NODES	1000

typedef struct tLightningNode {
	struct tLightning	*pChild;
	vmsVector			vPos;
	vmsVector			vNewPos;
	vmsVector			vOffs;
	vmsVector			vBase;
	vmsVector			vDelta [2];
	int					nDelta [2];
} tLightningNode;

typedef struct tLightning {
	struct tLightning *pParent;
	vmsVector			vBase;
	vmsVector			vPos;
	vmsVector			vEnd;
	vmsVector			vDir;
	vmsVector			vRefEnd;
	vmsVector			vDelta;
	tLightningNode		*pNodes;
	tRgbaColorf			color;
	int					nIndex;
	int					nNext;
	int					nLife;
	int					nTTL;
	int					nDelay;
	int					nLength;
	int					nOffset;
	int					nAmplitude;
	short					nSmoothe;
	short					nSteps;
	short					iStep;
	short					nNodes;
	short					nChildren;
	short					nObject;
	short					nSegment;
	short					nNode;
	char					nStyle;
	char					nAngle;
	char					nDepth;
	char					bClamp;
	char					bPlasma;
	char					bRandom;
	char					bInPlane;
} tLightning;

typedef struct tLightningBundle {
	int				nNext;
	tLightning		*pl;
	int				nLightnings;
	short				nObject;
	int				nKey [2];
	time_t			tUpdate;
	int				nSound;
	char				bSound;
	char				bForcefield;
	char				bDestroy;
} tLightningBundle;

typedef struct tLightningLight {
	vmsVector		vPos;
	tRgbaColorf		color;
	int				nNext;
	int				nLights;
	int				nBrightness;
	int				nDynLight;
	short				nSegment;
	char				nFrameFlipFlop;
} tLightningLight;

typedef struct tLightningData {
	short					*objects;
	tLightningLight	*lights;
	tLightningBundle	buffer [MAX_LIGHTNINGS];
	int					iFree;
	int					iUsed;
	int					nNext;
	int					bDestroy;
	int					nFirstLight;
} tLightningData;

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
	grsBitmap	bm;
	ubyte			*palette;
} tHoardItem;

typedef struct tHoardData {
	int			bInitialized;
	int			nBitmaps;
	tHoardItem	orb;
	tHoardItem	icon [2];
	tHoardItem	goal;
	tHoardItem	monsterball;
	short			nMonsterballSeg;
	tObject		*monsterballP;
	short			nLastHitter;
} tHoardData;

//------------------------------------------------------------------------------

#include "hudmsg.h"

typedef struct tHUDMessage {
	int					nFirst;
	int					nLast;
	int					nMessages;
	fix					xTimer;
	unsigned int		nColor;
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
	tObject			*objP;
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

#ifdef _DEBUG
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

typedef struct tOmegaLightningHandles {
	int		nLightning;
	short		nParentObj;
	short		nTargetObj;
} tOmegaLightningHandles;

typedef struct tOmegaLightningData {
	tOmegaLightningHandles	handles [MAX_OBJECTS_D2X];
	int							nHandles;
} tOmegaLightningData;

typedef struct tOmegaData {
	fix		xCharge [2];
	fix		xMaxCharge;
	int		nLastFireFrame;
	tOmegaLightningData	lightnings;
} tOmegaData;

//------------------------------------------------------------------------------

typedef struct tMissileData {
	fix		xLastFiredTime;
	fix		xNextFireTime;
	int		nGlobalFiringCount;
} tMissileData;

//------------------------------------------------------------------------------

typedef struct tCameraData {
	short		nCameras;
	tCamera	cameras [MAX_CAMERAS];
	char		*nSides;
} tCameraData;

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
	grsBitmap	*bmP;
} tTextData;

//------------------------------------------------------------------------------

#define MAX_GAUGE_BMS 100   // increased from 56 to 80 by a very unhappy MK on 10/24/94.
#define D1_MAX_GAUGE_BMS 80   // increased from 56 to 80 by a very unhappy MK on 10/24/94.

typedef struct tCockpitData {
	tBitmapIndex		gauges [2][MAX_GAUGE_BMS];
} tCockpitData;

typedef struct tGameData {
	tSegmentData		segs;
	tWallData			walls;
	tTriggerData		trigs;
	tObjectData			objs;
	tRobotData			bots;
	tRenderData			render;
	tEffectData			eff;
	tPigData				pig;
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
	tSmokeData			smoke;
	tLightningData		lightnings;
	tEscortData			escort;
	tThiefData			thief;
	tHoardData			hoard;
	tHUDData				hud;
	tTerrainData		terrain;
	tTimeData			time;
	tFCDData				fcd;
	tVertColorData		vertColor;
	tThreadData			threads;
#ifdef _DEBUG
	tSpeedtestData		speedtest;
#endif
	tPhysicsData		physics;
	tLaserData			laser;
	tFusionData			fusion;
	tOmegaData			omega;
	tMissileData		missiles;
	tCameraData			cameras;
	tCockpitData		cockpit;
	tCollisionData		collisions;
	tScoreData			score;
	tTrackIRData		trackIR;
	tStatsData			stats;
	tTextData			messages [2];
	tTextData			sounds;
	tApplicationData	app;
} tGameData;

extern tGameData gameData;

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
static inline ushort WallNumP (tSegment *segP, short nSide) { return WallNumS ((segP)->sides + (nSide)); }
static inline ushort WallNumI (short nSegment, short nSide) { return WallNumP(gameData.segs.segments + (nSegment), nSide); }

//	-----------------------------------------------------------------------------------------------------------

static inline short ObjIdx (tObject *objP)
{
	size_t	i = (char *) objP - (char *) gameData.objs.objects;

if ((i < 0) || (i > gameData.objs.nLastObject * sizeof (tObject)) || (i % sizeof (tObject)))
	return -1;
return (short) (i / sizeof (tObject));
}

//	-----------------------------------------------------------------------------------------------------------

#define	NO_WALL		(gameStates.app.bD2XLevel ? 2047 : 255)
#define  IS_WALL(_wallnum)	((ushort) (_wallnum) < NO_WALL)

#define SEG_IDX(_segP)			((short) ((_segP) - gameData.segs.segments))
#define SEG2_IDX(_seg2P)		((short) ((_seg2P) - gameData.segs.segment2s))
#define WALL_IDX(_wallP)		((short) ((_wallP) - gameData.walls.walls))
#define OBJ_IDX(_objP)			ObjIdx (_objP)
#define TRIG_IDX(_trigP)		((short) ((_trigP) - gameData.trigs.triggers))
#define FACE_IDX(_faceP)		((int) ((_faceP) - gameData.segs.faces.faces))

#ifdef PIGGY_USE_PAGING

static inline void PIGGY_PAGE_IN (int bmi, int bD1) 
{
grsBitmap *bmP = gameData.pig.tex.bitmaps [bD1] + bmi;
if (!bmP->bmTexBuf || (bmP->bmProps.flags & BM_FLAG_PAGED_OUT))
	PiggyBitmapPageIn (bmi, bD1);
}

#else //!PIGGY_USE_PAGING
#	define PIGGY_PAGE_IN(bmp)
#endif //!PIGGY_USE_PAGING

void GrabMouse (int bGrab, int bForce);
void InitGameOptions (int i);
void SetDataVersion (int v);

static inline void OglVertex3x (fix x, fix y, fix z)
{
glVertex3f ((float) x / 65536.0f, (float) y / 65536.0f, (float) z / 65536.0f);
}

static inline void OglVertex3f (g3sPoint *p)
{
if (p->p3_index < 0)
	OglVertex3x (p->p3_vec.p.x, p->p3_vec.p.y, p->p3_vec.p.z);
else
	glVertex3fv ((GLfloat *) (gameData.render.pVerts + p->p3_index));
}

static inline float GrAlpha (void)
{
if (gameStates.render.grAlpha >= (float) GR_ACTUAL_FADE_LEVELS)
	return 1.0f;
return 1.0f - gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS;
}

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

#define MAXFPS		((gameStates.render.automap.bDisplay && !(gameStates.render.automap.bRadar || gameStates.render.frameRate.value)) ? 40 : gameOpts->render.nMaxFPS)

#define SPECTATOR(_objP)	(gameStates.app.bFreeCam && (OBJ_IDX (_objP) == LOCALPLAYER.nObject))
#define OBJPOS(_objP)		(SPECTATOR (_objP) ? &gameStates.app.playerPos : &(_objP)->position)
#define OBJSEG(_objP)		(SPECTATOR (_objP) ? gameStates.app.nPlayerSegment : (_objP)->nSegment)

#ifdef RELEASE
#	define FAST_SHADOWS	1
#else
#	define FAST_SHADOWS	gameOpts->render.shadows.bFast
#endif

void D2SetCaption (void);
void PrintVersionInfo (void);

//	-----------------------------------------------------------------------------------------------------------

typedef struct fVector3D {
	float	x, y, z;
} fVector3D;

typedef struct tTIRInfo {
	fVector3D	fvRot;
	fVector3D	fvTrans;
	} tTIRInfo;

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
typedef int (WINAPI *tpfnTIRQuery) (tTIRInfo *);

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

#define GEO_LIGHTING 0 //gameOpts->ogl.bGeoLighting

//	-----------------------------------------------------------------------------------------------------------

#endif


