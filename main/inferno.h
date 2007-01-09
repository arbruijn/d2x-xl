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

#define MAX_SUBMODELS	10		// how many animating sub-objects per model

#define MULTI_THREADED	0
#define USE_SEGRADS		0

#ifdef _DEBUG
#	define	SHADOWS	1
#else
#	define	SHADOWS	1
#endif

#if SHADOWS
#	ifdef _DEBUG
#		define DBG_SHADOWS 1
#	else
#		define DBG_SHADOWS 0
#	endif
#endif

#include "pstypes.h"
#include "3d.h"
#include "bm.h"
#include "gr.h"
#include "piggy.h"
#include "ogl_init.h"
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

// MACRO for single line #ifdef WINDOWS #else DOS
#define WINDOS(x,y) y
#define WIN(x)
#define PA_DFX(x)
#define NO_DFX(x) x


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

#define MAX_PLAYERS 8
#define MAX_MULTI_PLAYERS MAX_PLAYERS+3

//for Function_mode variable
#define FMODE_EXIT		0		// leaving the program
#define FMODE_MENU		1		// Using the menu
#define FMODE_GAME		2		// running the game
#define FMODE_EDITOR		3		// running the editor

#define FLASH_CYCLE_RATE f1_0

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

typedef struct tCameraOptions {
	int nFPS;
	int nSpeed;
	int bFitToWall;
} tCameraOptions;

typedef struct tWeaponIconOptions {
	int bSmall;
	char bShowAmmo;
	char bEquipment;
	char nSort;
	ubyte alpha;
} tWeaponIconOptions;

typedef struct tColorOptions {
	int bCap;
	int bAmbientLight;
	int bGunLight;
	int bWalls;
	int bMix;
	int bUseLightMaps;
	int nLightMapRange;
} tColorOptions;

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
	int nWindowPos;
	int nWindowSize;
	int nWindowZoom;
} tCockpitOptions;

typedef struct tTextureOptions {
	int bUseHires;
	int nQuality;
} tTextureOptions;

typedef struct tSmokeOptions {
	int nDens [4];
	int nSize [4];
	int nLife [4];
	int bSyncSizes;
	int bPlayers;
	int bRobots;
	int bMissiles;
	int bDebris;
	int bCollisions;
	int bDisperse;
	int bSort;
	int bDecreaseLag;	//only render if tPlayer is moving forward
} tSmokeOptions;

typedef struct tShadowOptions {
	int nReach;
	int nLights;
	int bFast;
	int nClip;
	int bSoft;
	int bPlayers;
	int bRobots;
	int bMissiles;
	int bReactors;
	} tShadowOptions;

typedef struct tPowerupOptions {
	int b3D;
	int nSpin;
} tPowerupOptions;

typedef struct tRenderOptions {
	int bAllSegs;
	int bAutomapAlwaysHires;
	int bDynamicLight;
	int bDynLighting;
	int bObjects;
	int bTextures;
	int bWalls;
	int bWireFrame;
	int bOptimize;
	int bTransparentEffects;
	int bUseShaders;
	int bHiresModels;
	int bAutoTransparency;
	int nMathFormat;
	int nDefMathFormat;
	int bEnableSSE;
	short nMaxFPS;
	int nQuality;
	int nTextureQuality;
	tCameraOptions cameras;
	tColorOptions color;
	tCockpitOptions cockpit;
	tTextureOptions textures;
	tWeaponIconOptions weaponIcons;
	tSmokeOptions smoke;
	tShadowOptions shadows;
	tPowerupOptions powerups;
} tRenderOptions;

typedef struct tOglOptions {
	int bSetGammaRamp;
	int bGlTexMerge;
	int bLightObjects;
	int nMaxLights;
	int bRgbaFormat;
	int bIntensity4;
	int bLuminance4Alpha4;
	int bRgba2;
	int bReadPixels;
	int bGetTexLevelParam;
	int bVoodooHack;
#ifdef GL_ARB_multitexture
	int bArbMultiTexture;
#endif
#ifdef GL_SGIS_multitexture
	int bSgisMultiTexture;
#endif
} tOglOptions;

typedef struct tMovieOptions {
	int bHires;
	int nQuality;
	int nLevel;
	int bResize;
	int bFullScreen;
	int bSubTitles;
} tMovieOptions;

typedef struct tGameplayOptions {
	int nAutoSelectWeapon;
	int bSecretSave;
	int bTurboMode;
	int bFastRespawn;
	int bAutoLeveling;
	int bDefaultLeveling;
	int bEscortHotKeys;
	int nPlayerDifficultyLevel;
	int bSkipBriefingScreens;
	int bHeadlightOn;
	int bShieldWarning;
	int bInventory;
	int bIdleAnims;
} tGameplayOptions;

#define UNIQUE_JOY_AXES	5

typedef struct tInputOptions {
	int bUseJoystick;
	int bUseMouse;
	int bJoyMouse;
	int bUseHotKeys;
	int bUseKeyboard;
	int bSyncMouseAxes;
	int bSyncJoyAxes;
	int keyRampScale;
	int bRampKeys [3];
	int bLinearJoySens;
	int nJoysticks;
	int bJoyPresent;
	int bLimitTurnRate;
	int nMinTurnRate;
	int nMaxPitch;
	ubyte mouseSensitivity [3];
	ubyte joySensitivity [UNIQUE_JOY_AXES];
	int joyDeadZones [UNIQUE_JOY_AXES];
} tInputOptions;

typedef struct tSoundOptions {
	int bUseD1Sounds;
	int bUseRedbook;
	int bD1Sound;
	int bUseSDLMixer;
	int digiSampleRate;
} tSoundOptions;

typedef struct tAltBgOptions {
	int bHave;
	double alpha;
	double brightness;
	int grayscale;
	char szName [FILENAME_LEN];
} tAltBgOptions;

typedef struct tMenuOptions {
	int nStyle;
	int bFastMenus;
	int bSmartFileSearch;
	int bShowLevelVersion;
	char nHotKeys;
	tAltBgOptions altBg;
} tMenuOptions;

typedef struct tDemoOptions {
	int bOldFormat;
} tDemoOptions;

typedef struct tMultiplayerOptions {
	int bNoRankings;
	int bTimeoutPlayers;
	int bUseMacros;
	int bNoRedundancy;
} tMultiplayerOptions;

typedef struct tApplicationOptions {
	int bAutoRunMission;
	int nVersionFilter;
	int bDemoData;
	int bSinglePlayer;
	int bExpertMode;
	int nScreenShotInterval;
} tApplicationOptions;

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

typedef struct tGameplayStates {
	int bMultiBosses;
	int bFinalBossIsDead;
	int bHaveSmartMines;
	int bMineDestroyed;
	int bNoBotAI;
	int bTagFlag;
	int nShieldFlash;
	fix nPlayerSpeed;
	vmsVector vTgtDir;
	int nDirSteps;
	tSeismicStates seismic;
} tGameplayStates;

typedef struct tInputStates {
	int		nMouseType;
	int		nJoyType;
	int		bGrabMouse;
	ubyte		bCybermouseActive;
	int		bSkipControls;
	int		bControlsSkipFrame;
	int		bKeepSlackTime;
	time_t	kcFrameTime;
	fix		nCruiseSpeed;
} tInputStates;

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

typedef struct tMovieStates {
	int nRobots;
	int bIntroPlayed;
} tMovieStates;

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

typedef struct tGfxStates {
	int bInstalled;
	int bOverride;
	int nStartScrSize;
	int nStartScrMode;
} tGfxStates;

typedef struct tOglStates {
	int bInitialized;
	int bShadersOk;
	int bFullScreen;
	int bLastFullScreen;
	int bUseTransform;
	int bBrightness;
	int bDoPalStep;
	int nColorBits;
	int nDepthBits;
	int nStencilBits;
	int bEnableTexture2D;
	int bEnableTexClamp;
	int bEnableScissor;
	int bNeedMipMaps;
	int bAntiAliasing;
	int bAntiAliasingOk;
	int bVoodooHack;
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
	int nReticle;
	tRgbColorb bright;
	tRgbColorf fBright;
	tRgbColors palAdd;
} tOglStates;

typedef struct tCameraStates {
	int bActive;
} tCameraStates;

typedef struct tColorStates {
	int bLightMapsOk;
} tColorStates;

typedef struct tTextureStates {
	int bGlsTexMergeOk;
	int bHaveMaskShader;
} tTextureStates;

typedef struct tCockpitStates {
	int bShowPingStats;
	int nMode;
	int nNextMode;
	int nModeSave;
	int bRedraw;
	int bBigWindowSwitch;
} tCockpitStates;

typedef struct tFontStates {
	int bHires;
	int bHiresAvailable;
	int bInstalled;
} tFontStates;

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

typedef struct tRenderStates {
	int bTopDownRadar;
	int bExternalView;
	int bQueryOcclusion;
	int bPointSprites;
	int bVertexArrays;
	int bAutomapUseGameRes;
	int bDisableFades;
	int bBlendBackground;
	int bDropAfterburnerBlob;
	int bOutsideMine;
	int bExtExplPlaying;
	int bDoAppearanceEffect;
	int bGlLighting;
	int bColored;
	int bRearView;
	int nInterpolationMethod;
	int bTMapFlat;
	int nWindow;
	int nLighting;
	int bTransparency;
	int bHaveDynLights;
	int bPaletteFadedOut;
	int bHaveStencilBuffer;
	int nRenderPass;
	int nShadowPass;
	int nShadowBlurPass;
	int bShadowMaps;
	int bLoResShadows;
	int nSoften;
	int bHeadlightOn;
	int bHaveSkyBox;
	int bAllVisited;
	int bViewDist;
	int bD2XLights;
	int bRendering;
	int nFrameFlipFlop;
	int nState;	//0: render geometry, 1: render objects
	ubyte nRenderingType;
	fix nFlashScale;
	fix nFlashRate;
	cvar_t frameRate;
	tCameraStates cameras;
	tCockpitStates cockpit;
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
} tRenderStates;

typedef struct tDigiStates {
	int bInitialized;
	int bSoundsInitialized;
	int bLoMem;
	int nMaxChannels;
	int nNextChannel;
	int nVolume;
	int nNextSignature;
	int nActiveObjects;
	short nLoopingSound;
	short nLoopingVolume;
	short nLoopingStart;
	short nLoopingEnd;
	short nLoopingChannel;
} tDigiStates;

typedef struct tSoundStates {
	int nCurrentSong;
	int bRedbookEnabled;
	int bRedbookPlaying;
	int bWasRecording;
	int bDontStartObjects;
	int nConquerWarningSoundChannel;
	int nMaxSoundChannels;
	tDigiStates digi;
} tSoundStates;

typedef struct tVideoStates {
	int nDisplayMode;
	int nDefaultDisplayMode;
	u_int32_t nScreenMode;
	u_int32_t nLastScreenMode;
	int nWidth;
	int nHeight;
	int bFullScreen;
} tVideoStates;

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
	int bD1CheatsEnabled;
	int nUnlockLevel;
} tCheatStates;

typedef struct tEntropyStates {
	int bConquering;
	int bConquerWarning;
	int bExitSequence;
	int nTimeLastMoved;
} tEntropyStates;

typedef struct tApplicationStates {
#if MULTI_THREADED
	int bExit;
	int bMultiThreaded;
#endif
	int nSDLTicks;
	int nLastTick;
	int b40fpsTick;
	int nDeltaTime;
	int nExtGameStatus;
	int nFunctionMode;
	int nLastFuncMode;
	int nCriticalError;
	int bNostalgia;
	int iNostalgia;
	int bInitialized;
	int bD2XLevel;
	int bEnterGame;
	int bSpectating;
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
	int bAutoMap;
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
	fix nPlayerTimeOfDeath;
	char *szCurrentMission;
	char *szCurrentMissionFile;
	tPosition playerPos;
	tCheatStates cheats;
} tApplicationStates;

typedef struct tLimitFPSStates {
	ubyte	bControls;
	ubyte bJoystick;
	ubyte	bSeismic;
	ubyte bCountDown;
	ubyte	bHomers;
	ubyte	bFusion;
	ubyte bOmega;
} tLimitFPSStates;

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

extern tGameOptions	gameOptions [2];
extern tGameStates	gameStates;
extern tGameOptions	*gameOpts;

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

typedef struct tColorData {
	tFaceColor	lights [MAX_SEGMENTS][6];
	tFaceColor	sides [MAX_SEGMENTS][6];
	tFaceColor	segments [MAX_SEGMENTS];
	tFaceColor	vertices [MAX_VERTICES];
	float			vertBright [MAX_VERTICES];
	tFaceColor	ambient [MAX_VERTICES];	//static light values
	tFaceColor	textures [MAX_WALL_TEXTURES];
	tLightRef	visibleLights [MAX_SEGMENTS * 6];
	int			nVisibleLights;
	tRgbColorf	flagTag;
} tColorData;

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

#define USE_OGL_LIGHTS	0
#define MAX_OGL_LIGHTS  (64 * 64) //MUST be a power of 2!

typedef struct tDynLight {
	vmsVector	vPos;
	vmsVector	vDir;
	tRgbColorf	color;
	float			brightness;
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
	ubyte			nPlayer;
	ubyte			nType;
	ubyte			bState;
	ubyte			bVariable;
	ubyte			bOn;
	ubyte			bSpot;
	ubyte			bTransform;
	tShadowLightInfo	shadow;
} tDynLight;

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

typedef struct tShaderLight {
#if 1
	vmsVector	vPos;
	fVector		pos [2];
	fVector		dir;
	fVector		color;
	float			rad;
#endif
	float			brightness;
	float			spotAngle;
	float			spotExponent;
	ubyte			nType;
	ubyte			bState;
	ubyte			bVariable;
	ubyte			bOn;
	ubyte			bSpot;
	ubyte			bShadow;
	ubyte			bExclusive;
} tShaderLight;

typedef struct tShaderLightData {
	tShaderLight	lights [MAX_OGL_LIGHTS];
	int				nLights;
	GLuint			nTexHandle;
} tShaderLightData;

#define MAX_NEAREST_LIGHTS 32

typedef struct tDynLightData {
	tDynLight			lights [MAX_OGL_LIGHTS];
	short					nNearestSegLights [MAX_SEGMENTS][MAX_NEAREST_LIGHTS];	//the 8 nearest static lights for every tSegment
	short					nNearestVertLights [MAX_VERTICES][MAX_NEAREST_LIGHTS];	//the 8 nearest static lights for every tSegment
	short					owners [MAX_OBJECTS];
	short					nLights;
	short					nHeadLights [MAX_PLAYERS];
	short					nSegment;
	tShaderLightData	shader;
	tOglMaterial		material;
} tDynLightData;

extern int nMaxNearestLights [21];

//Flickering light system
typedef struct tFlickeringLight {
	short				nSegment;
	short				nSide;
	unsigned long	mask;     // determines flicker pattern
	fix				timer;    // time until next change
	fix				delay;    // time between changes
} tFlickeringLight;

#define MAX_FLICKERING_LIGHTS 1000

typedef struct tFlickerLightData {
	tFlickeringLight	lights [MAX_FLICKERING_LIGHTS];
	int					nLights;
} tFlickerLightData;

typedef struct tLightData {
	int					nStatic;
	fix					segDeltas [MAX_SEGMENTS][6];
	dl_index				deltaIndices [MAX_DL_INDICES];
	delta_light			deltas [MAX_DELTA_LIGHTS];
	ubyte					subtracted [MAX_SEGMENTS];
	tDynLightData		dynamic;
	tFlickerLightData	flicker;
} tLightData;

typedef struct tShadowData {
	short				nLight;
	short				nLights;
	tShaderLight	*pLight;
	short				nShadowMaps;
	tCamera			shadowMaps [MAX_SHADOW_MAPS];
	tObject			lightSource;
	tOOF_vector		vLightPos;
	vmsVector		vLightDir [MAX_SHADOW_LIGHTS];
	short				objLights [MAX_OBJECTS][MAX_SHADOW_LIGHTS];
	ubyte				nFrame;	//flipflop for testing whether a light source's view has been rendered the current frame
} tShadowData;

#include "morph.h"

typedef struct tMorphData {
	tMorphInfo	objects [MAX_MORPH_OBJECTS];
	fix			xRate;
} tMorphData;

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

#define OGLTEXBUFSIZE (2048*2048*4)

typedef struct tOglData {
	GLubyte					texBuf [OGLTEXBUFSIZE];
	ubyte						*palette;
	GLenum					nSrcBlend;
	GLenum					nDestBlend;
} tOglData;

#define TERRAIN_GRID_MAX_SIZE   64
#define TERRAIN_GRID_SCALE      i2f (2*20)
#define TERRAIN_HEIGHT_SCALE    f1_0

typedef struct tTerrainRenderData {
	ubyte			*pHeightMap;
	fix			*pLightMap;
	grsBitmap	*bmP;
	g3sPoint	saveRow [TERRAIN_GRID_MAX_SIZE];
	vmsVector	vStartPoint;
	uvl			uvlList [2][3];
	int			bOutline;
	int			nGridW, nGridH;
	int			orgI, orgJ;
	int			nMineTilesDrawn;    //flags to tell if all 4 tiles under mine have drawn
} tTerrainRenderData;

typedef struct tThrusterData {
	tFlightPath				path;
	float						fSpeed;
	short						nPulse;
	time_t					tPulse;
} tThrusterData;

typedef struct tRenderData {
	tColorData				color;
	int						transpColor;
	tSphereData				shield;
	tSphereData				monsterball;
	int						nPaletteGamma;
	int						nComputedColors;
	fix						xFlashEffect;
	fix						xTimeFlashLastPlayed;
	fVector					*pVerts;
	tLightData				lights;
	tMorphData				morph;
	tShadowData				shadows;
	tPaletteData			pal;
	tOglData					ogl;
	tTerrainRenderData	terrain;
	tThrusterData			thrusters [MAX_PLAYERS];
} tRenderData;

typedef struct tSecretData {
	int			nReturnSegment;
	vmsMatrix	returnOrient;

} tSecretData;

typedef struct tSlideSegs {
	short	nSegment;
	ubyte	nSides;
} tSlideSegs;

typedef struct tSegmentData {
	vmsVector			vertices [MAX_VERTICES];
	fVector				fVertices [MAX_VERTICES];
	tSegment				segments [MAX_SEGMENTS];
	segment2				segment2s [MAX_SEGMENTS];
	xsegment				xSegments [MAX_SEGMENTS];
	g3sPoint				points [MAX_VERTICES];
#if USE_SEGRADS
	fix					segRads [MAX_SEGMENTS];
#endif
	vmsVector			segCenters [MAX_SEGMENTS][2];
	vmsVector			sideCenters [MAX_SEGMENTS * 6];
	int					nVertices;
	int					nLastVertex;
	short					nSegments;
	short					nLastSegment;
	int					nLevelVersion;
	char					szLevelFilename [FILENAME_LEN];
	tSecretData			secret;
	tSlideSegs			slideSegs [MAX_SEGMENTS];
	short					nSlideSegs;
	int					bHaveSlideSegs;
} tSegmentData;

typedef struct tWallData {
	wall					walls [MAX_WALLS];
	tExplWall			explWalls [MAX_EXPLODING_WALLS];
	tActiveDoor			activeDoors [MAX_DOORS];
	tCloakingWall		cloaking [MAX_CLOAKING_WALLS];
	tWallClip					anims [2][MAX_WALL_ANIMS];
	int					bitmaps [MAX_WALL_ANIMS];
	int					nWalls;
	int					nOpenDoors; 
	int					nCloaking;
	int					nAnims [2];
	tWallClip					*pAnims;
} tWallData;

typedef struct tTriggerData {
	tTrigger				triggers [MAX_TRIGGERS];
	tTrigger				objTriggers [MAX_TRIGGERS];
	tObjTriggerRef		objTriggerRefs [MAX_OBJ_TRIGGERS];
	short					firstObjTrigger [MAX_OBJECTS_D2X];
	long					delay [MAX_TRIGGERS];
	int					nTriggers;
	int					nObjTriggers;
} tTriggerData;

typedef struct tPowerupData {
	powerupType_info info [MAX_POWERUP_TYPES];
	int					nTypes;
} tPowerupData;

typedef struct tObjTypeData {
	int					nTypes;
	sbyte					nType [MAX_OBJTYPE];
	sbyte					nId [MAX_OBJTYPE];  
	fix					nStrength [MAX_OBJTYPE];   
} tObjTypeData;

typedef struct tSpeedBoostData {
	int					bBoosted;
	vmsVector			vVel;
	vmsVector			vMinVel;
	vmsVector			vMaxVel;
	vmsVector			vSrc;
	vmsVector			vDest;
} tSpeedBoostData;

typedef struct tObjectData {
	tObjTypeData		types;
	tObject				objects [MAX_OBJECTS];
	short					freeList [MAX_OBJECTS];
	short					parentObjs [MAX_OBJECTS];
	tObjectRef			childObjs [MAX_OBJECTS];
	short					firstChild [MAX_OBJECTS];
	tObject				init [MAX_OBJECTS];
	tObjDropInfo		dropInfo [MAX_OBJECTS];
	tSpeedBoostData	speedBoost [MAX_OBJECTS];
	vmsVector			vRobotGoals [MAX_OBJECTS];
	fix					xLastAfterburnerTime [MAX_OBJECTS];
	short					nFirstDropped;
	short					nLastDropped;
	short					nFreeDropped;
	short					nDropped;
	ushort				cameraRef [MAX_OBJECTS];
	tObject				*guidedMissile [MAX_PLAYERS];
	int					guidedMissileSig [MAX_PLAYERS];
	tObject				*console;
	tObject				*viewer;
	tObject				*missileViewer;
	tObject				*deadPlayerCamera;
	tObject				*endLevelCamera;
	int					nObjects;
	int					nLastObject;
	int					nObjectLimit;
	int					nNextSignature;
	int					nChildFreeList;
	int					nDrops;
	int					nDeadControlCenter;
	tPowerupData		pwrUp;
} tObjectData;


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

typedef struct tSoundData {
	tDigiSound			sounds [2][MAX_SOUND_FILES];
	int					nSoundFiles [2];
	tDigiSound			*pSounds;
} tSoundData;

#define N_COCKPIT_BITMAPS 6
#define D1_N_COCKPIT_BITMAPS 4

typedef struct tTextureData {
	BitmapFile			bitmapFiles [2][MAX_BITMAP_FILES];
	grsBitmap			bitmaps [2][MAX_BITMAP_FILES];
	grsBitmap			altBitmaps [2][MAX_BITMAP_FILES];
	ushort				bitmapXlat [MAX_BITMAP_FILES];
	alias					aliases [MAX_ALIASES];
	tBitmapIndex		bmIndex [2][MAX_TEXTURES];
	tBitmapIndex		objBmIndex [MAX_OBJ_BITMAPS];
	ushort				pObjBmIndex [MAX_OBJ_BITMAPS];
	tBitmapIndex		cockpitBmIndex [N_COCKPIT_BITMAPS];
	int					nBitmaps [2];
	int					nObjBitmaps;
	int					bPageFlushed;
	tTexMapInfo      	tMapInfo [2][MAX_TEXTURES];
	int					nExtraBitmaps;
	int					nAliases;
	int					nHamFileVersion;
	int					nTextures [2];
	int					nFirstMultiBitmap;
	BitmapFile			*pBitmapFiles;
	grsBitmap			*pBitmaps;
	grsBitmap			*pAltBitmaps;
	tBitmapIndex		*pBmIndex;
	tTexMapInfo			*pTMapInfo;
	int					brightness [MAX_WALL_TEXTURES];
} tTextureData;

typedef struct tEffectData {
	eclip					effects [2][MAX_EFFECTS];
	tVideoClip 				vClips [2][VCLIP_MAXNUM];
	int					nEffects [2];
	int 					nClips [2];
	eclip					*pEffects;
	tVideoClip					*pVClips;
} tEffectData;

#define N_PLAYER_GUNS 8

typedef struct tPlayerShip {
	int					nModel;
	int					expl_vclip_num;
	fix					mass,drag;
	fix					max_thrust,
							reverse_thrust,
							brakes;
	fix					wiggle;
	fix					max_rotthrust;
	vmsVector			gunPoints [N_PLAYER_GUNS];
} tPlayerShip;

typedef struct tShipData {
	tPlayerShip			only;
	tPlayerShip			*player;
} tShipData;

typedef struct tFlagData {
	tBitmapIndex		bmi;
	tVideoClip			*vcP;
	tVClipInfo			vci;
	tFlightPath			path;
} tFlagData;

typedef struct tPigData {
	tTextureData		tex;
	tSoundData			snd;
	tShipData			ship;
	tFlagData			flags [2];
} tPigData;

#include "laser.h"

typedef struct tMuzzleData {
	int				queueIndex;
	tMuzzleInfo		info [MUZZLE_QUEUE_MAX];
} tMuzzleData;

#include "weapon.h"

typedef struct tWeaponData {
	sbyte					nPrimary;
	sbyte					nSecondary;
	sbyte					nOverridden;
	int					nTypes [2];
	tWeaponInfo			info [MAX_WEAPON_TYPES];
	tD1WeaponInfo		infoD1 [D1_MAX_WEAPON_TYPES];
	tRgbColorf			color [MAX_OBJECTS];
	ubyte					bLastWasSuper [2][MAX_PRIMARY_WEAPONS];
} tWeaponData;

#define bLastPrimaryWasSuper (gameData.weapons.bLastWasSuper [0])
#define bLastSecondaryWasSuper (gameData.weapons.bLastWasSuper [1])

#include "polyobj.h"

#define OOF_PYRO			0
#define OOF_MEGA			1

typedef struct tModelData {
	int					nHiresModels;
	tOOFObject			hiresModels [MAX_POLYGON_MODELS];
	tPOFObject			pofData [2][2][MAX_POLYGON_MODELS];
	ubyte					bHaveHiresModel [MAX_POLYGON_MODELS];
	tPolyModel			polyModels [MAX_POLYGON_MODELS];
	tPolyModel			defPolyModels [MAX_POLYGON_MODELS];
	tPolyModel			altPolyModels [MAX_POLYGON_MODELS];
	tOOFObject			*modelToOOF [MAX_POLYGON_MODELS];
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
} tModelData;

#include "player.h"

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

typedef struct tMultiplayerData {
	int 					nPlayers;					
	int					nMaxPlayers;
	int 					nLocalPlayer;					
	int					nPlayerPositions;
	tPlayer				players [MAX_PLAYERS + 4];                   
	tObjPosition		playerInit [MAX_PLAYERS];
	short					nVirusCapacity [MAX_PLAYERS];
	int					nLastHitTime [MAX_PLAYERS];
	char					bWasHit [MAX_PLAYERS];
	tPulseData			spherePulse [MAX_PLAYERS];
	ubyte					powerupsInMine [MAX_POWERUP_TYPES];
	ubyte					powerupsOnShip [MAX_POWERUP_TYPES];
	ubyte					maxPowerupsAllowed [MAX_POWERUP_TYPES];
	tLeftoverPowerup	leftoverPowerups [MAX_OBJECTS];
	tAutoNetGame		autoNG;
	fix					xStartAbortMenuTime;
} tMultiplayerData;

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
	mle					list [MAXMSLIONS + 1];
	char					szLevelNames [MAX_LEVELS_PERMSLION][FILENAME_LEN];
	char					szSecretLevelNames [MAX_SECRET_LEVELS_PERMSLION][FILENAME_LEN];
	int					secretLevelTable [MAX_SECRET_LEVELS_PERMSLION];
} tMissionData;

#define N_MAX_ROOMS	128

typedef struct tEntropyData {
	char	nRoomOwners [N_MAX_ROOMS];
	char	nTeamRooms [3];
	int   nTotalRooms;
} tEntropyData;

tEntropyData entropyData;

typedef struct tCountdownData {
	fix					nTimer;
	int					nSecsLeft;
	int					nTotalTime;
} tCountdownData;

#include "cntrlcen.h"

typedef struct tReactorData {
	reactor				reactors [MAX_REACTORS];
	tReactorTriggers	triggers;
	int					nReactors;
	int					bPresent;
	int					bDisabled;
	int					bHit;
	int					bDestroyed;
	int					bSeenPlayer;
	int					nNextFireTime;
	int					nDeadObj;
	int					nStrength;
	tCountdownData		countdown;
} tReactorData;

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
} tMarkerData;

typedef struct tFusionData {
	fix	xAutoFireTime;
	fix	xCharge;
	fix	xNextSoundTime;
	fix	xLastSoundTime;
} tFusionData;

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

typedef struct tApplicationData {
	int					nFrameCount;
	int					nGameMode;
	int					bGamePaused;
	int					nGlobalLaserFiringCount;
	int					nGlobalMissileFiringCount;
	uint					nStateGameId;
	tFusionData			fusion;
} tApplicationData;

#define MAX_BOSS_COUNT				10
#define MAX_BOSS_TELEPORT_SEGS   100
#define NUM_D2_BOSSES				8
#define BOSS_CLOAK_DURATION		(F1_0*7)
#define BOSS_DEATH_DURATION		(F1_0*6)

typedef struct tBossData {
	short					nTeleportSegs [MAX_BOSS_COUNT];
	short					teleportSegs [MAX_BOSS_COUNT][MAX_BOSS_TELEPORT_SEGS];
	short					nGateSegs [MAX_BOSS_COUNT];
	short					gateSegs [MAX_BOSS_COUNT][MAX_BOSS_TELEPORT_SEGS];
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
	int					objList [MAX_BOSS_COUNT];
	short					nDying;
	sbyte					bDyingSoundPlaying;
} tBossData;

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
	fvi_info				hitData;
	short					nBelievedPlayerSeg;
	vmsVector			vBelievedPlayerPos;
	fix					nDistToLastPlayerPosFiredAt;
	tAILocal				localInfo [MAX_OBJECTS];
	tAICloakInfo		cloakInfo [MAX_AI_CLOAK_INFO];
	tPointSeg			pointSegs [MAX_POINT_SEGS];
	tPointSeg			*freePointSegs;
	int					nAwarenessEvents;
	tAwarenessEvent	awarenessEvents [MAX_AWARENESS_EVENTS];
} tAIData;

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

typedef struct tSongData {
	tSongInfo			info [MAX_NUM_SONGS];
	int					bInitialized;
	int					nSongs;
	int					nD1Songs;
	int					nD2Songs;
	int					nFirstLevelSong [2];
	int					nLevelSongs [2];
	int					nD1EndLevelSong;
	tUserMusicData		user;
} tSongData;

typedef struct tMenuData {
	int					bValid;
	unsigned int		tinyColors [2][2];
	unsigned int		warnColor;
	unsigned int		keyColor;
	unsigned int		tabbedColor;
	unsigned int		helpColor;
	unsigned int		colorOverride;
} tMenuData;

#define MAX_FUEL_CENTERS    350
#define MAX_ROBOT_CENTERS   100

#include "fuelcen.h"

typedef struct tMatCenData {
	fix				xFuelRefillSpeed;
	fix				xFuelGiveAmount;
	fix				xFuelMaxAmount;
	int				nFuelCenters;
	tFuelCenInfo	fuelCenters [MAX_FUEL_CENTERS];
	int				nRobotCenters;
	tMatCenInfo		robotCenters [MAX_ROBOT_CENTERS];
	fix				xEnergyToCreateOneRobot;
	int				origStationTypes [MAX_FUEL_CENTERS];
	tSegment			*playerSegP;
} tMatCenData;

typedef struct tDemoData {
	int				bAuto;
	char				fnAuto [FILENAME_LEN];

	sbyte				bWasRecorded [MAX_OBJECTS];
	sbyte				bViewWasRecorded [MAX_OBJECTS];
	sbyte				bRenderingWasRecorded [32];

	char				callSignSave [CALLSIGN_LEN+1];
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
	fix				xPlaybackTotal;
	fix				xRecordedTotal;
	fix				xRecordedTime;
	sbyte				nPlaybackStyle;
	sbyte				bFirstTimePlayback;
	fix				xJasonPlaybackTotal;
	int				bUseShortPos;
	int				bFlyingGuided;
} tDemoData;

#include "particles.h"

typedef struct tSmokeData {
	tSmoke			smoke [MAX_SMOKE];
	short				objects [MAX_OBJECTS];
	time_t			objExplTime [MAX_OBJECTS];
	int				iFreeSmoke;
	int				iUsedSmoke;
} tSmokeData;

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

typedef struct tThiefData {
	ubyte				stolenItems [MAX_STOLEN_ITEMS];
	int				nStolenItem;
	fix				xReInitTime;
	fix				xWaitTimes [NDL];
} tThiefData;

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

#include "hudmsg.h"

typedef struct tHUDMessage {
	int					nFirst;
	int					nLast;
	int					nMessages;
	fix					xTimer;
	unsigned int		nColor;
	char					szMsgs [HUD_MAX_MSGS][HUD_MESSAGE_LENGTH + 5];
} tHUDMessage;

typedef struct tHUDData {
	tHUDMessage			msgs [2];
	int					bPlayerMessage;
} tHUDData;

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
	fVector	colorSum [2];
	fVector	*pVertPos;
	float		fMatShininess;
	} tVertColorData;

typedef struct tVertColorThreadData {
#if MULTI_THREADED
	SDL_Thread		*pThread [2];
	int				nId [2];
	SDL_sem			*done [2];	
	SDL_sem			*exec [2];
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
#if MULTI_THREADED
	SDL_Thread		*pThread [2];
	int				nId [2];
	SDL_sem			*done [2];	
	SDL_sem			*exec [2];
#endif
	tClipDistData	data;
	} tClipDistThreadData;

typedef struct tThreadData {
	tVertColorThreadData	vertColor;
	tClipDistThreadData	clipDist;
	} tThreadData;

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
	tMultiplayerData	multi;
	tMuzzleData			muzzle;
	tWeaponData			weapons;
	tMissionData		missions;
	tEntropyData		entropy;
	tReactorData		reactor;
	tMarkerData			marker;
	tBossData			boss;
	tAIData				ai;
	tSongData			songs;
	tEndLevelData		endLevel;
	tMenuData			menu;
	tMatCenData			matCens;
	tDemoData			demo;
	tSmokeData			smoke;
	tEscortData			escort;
	tThiefData			thief;
	tHoardData			hoard;
	tHUDData				hud;
	tTerrainData		terrain;
	tTimeData			time;
	tFCDData				fcd;
	tVertColorData		vertColor;
	tThreadData			threads;
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

extern tBossProps bossProps [2][NUM_D2_BOSSES];

extern char szAutoMission [255];
extern char szAutoHogFile [255];

static inline ushort WallNumS (tSide *sideP) { return (sideP)->nWall; }
static inline ushort WallNumP (tSegment *segP, short nSide) { return WallNumS ((segP)->sides + (nSide)); }
static inline ushort WallNumI (short nSegment, short nSide) { return WallNumP(gameData.segs.segments + (nSegment), nSide); }

#define	NO_WALL		(gameStates.app.bD2XLevel ? 2047 : 255)
#define  IS_WALL(_wallnum)	((ushort) (_wallnum) < NO_WALL)

#define SEG_IDX(_segP)			((short) ((_segP) - gameData.segs.segments))
#define SEG2_IDX(_seg2P)		((short) ((_seg2P) - gameData.segs.segment2s))
#define SEG_PTR_2_NUM(segptr) (Assert(SEG_IDX (segptr) < MAX_SEGMENTS),SEG_IDX (segptr))
#define WALL_IDX(_wallP)		((short) ((_wallP) - gameData.walls.walls))
#define OBJ_IDX(_objP)			((short) ((_objP) - gameData.objs.objects))
#define TRIG_IDX(_trigP)		((short) ((_trigP) - gameData.trigs.triggers))

#ifdef PIGGY_USE_PAGING

static inline void PIGGY_PAGE_IN (tBitmapIndex bmi, int bD1) 
{
grsBitmap *bmP = gameData.pig.tex.bitmaps [bD1] + bmi.index;
if (!bmP->bm_texBuf || (bmP->bm_props.flags & BM_FLAG_PAGED_OUT))
	PiggyBitmapPageIn (bmi, bD1);
}

#else //!PIGGY_USE_PAGING
#	define PIGGY_PAGE_IN(bmp)
#endif //!PIGGY_USE_PAGING

void GrabMouse (int bGrab, int bForce);
void InitGameOptions (int i);
void SetDataVersion (int v);

static inline void glVertex3x (fix x, fix y, fix z)
{
glVertex3f ((float) x / 65536.0f, (float) y / 65536.0f, (float) z / 65536.0f);
}

static inline void OglVertex3f (g3sPoint *p)
{
if (p->p3_index < 0)
	glVertex3x (p->p3_vec.p.x, p->p3_vec.p.y, p->p3_vec.p.z);
else
	glVertex3fv ((GLfloat *) (gameData.render.pVerts + p->p3_index));
}

static inline double GrAlpha (void)
{
if (gameStates.render.grAlpha >= GR_ACTUAL_FADE_LEVELS)
	return 1.0;
return 1.0 - (double) gameStates.render.grAlpha / (double) GR_ACTUAL_FADE_LEVELS;
}

#define	CLAMP(_val,_minVal,_maxVal)	\
			{if ((_val) < (_minVal)) (_val) = (_minVal); else if ((_val) > (_maxVal)) (_val) = (_maxVal);}

#define LOCALPLAYER	gameData.multi.players [gameData.multi.nLocalPlayer]

#define ISLOCALPLAYER(_nPlayer)	((_nPlayer < 0) || ((_nPlayer) == gameData.multi.nLocalPlayer))

#define INFINITY					fInfinity [gameOpts->render.shadows.nReach]

extern float fInfinity [];

#define sizeofa(_a)	(sizeof (_a) / sizeof ((_a) [0]))	//number of array elements

void D2SetCaption (void);

#endif


