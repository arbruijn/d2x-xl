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
 * Header file for Descent.  Should be included in all source files.
 *
 */

#ifndef _DESCENT_H
#define _DESCENT_H

#include <math.h>

#include "carray.h"

#ifndef DBG
#	ifdef _DEBUG
#		define DBG 1
#	else
#		define DBG 0
#	endif
#endif

#if 0 //DBG
#	include "vld.h"
#endif

#ifdef _WIN32
#	define USE_SDL_IMAGE	1
#endif

#ifndef OCULUS_RIFT
#	ifdef _WIN32
#		define OCULUS_RIFT 1
#	else
#		define OCULUS_RIFT 0
#	endif
#endif

#ifdef _OPENMP
#	include "omp.h"
#	ifndef _WIN32
#		define USE_OPENMP	2
#	endif
#elif !defined(USE_OPENMP)
#	define USE_OPENMP		0
#endif

#define SUPERUSER			9773

#define SHOW_EXIT_PATH  1

#define MAX_SUBMODELS	10		// how many animating sub-objects per model

#define MULTI_THREADED_SHADOWS	0
#define MULTI_THREADED_LIGHTS		0
#define MULTI_THREADED_PRECALC	1

#define USE_SEGRADS		0
#define CALC_SEGRADS		1
#define GEOMETRY_VBOS	0

#if DBG
#	define	PROFILING 0
#else
#	define	PROFILING 0
#endif

#if DBG
#	define DBG_SHADOWS 0
#else
#	define DBG_SHADOWS 0
#endif

#define PER_PIXEL_LIGHTING 1

#if defined(__macosx__) || defined(__unix__)
#	pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

#include "vers_id.h"
#include "pstypes.h"
#include "3d.h"
#include "loadgamedata.h"
#include "gr.h"
#include "piggy.h"
#include "ogl_defs.h"
#include "segment.h"
#include "aistruct.h"
#include "object.h"
#include "trigger.h"
#include "robot.h"
#include "wall.h"
#include "powerup.h"
#include "vclip.h"
#include "effects.h"
#include "oof.h"
#include "ase.h"
#include "cfile.h"
#include "segment.h"
#include "canvas.h"
//#include "console.h"
#include "vecmat.h"

#ifdef __macosx__
# include <SDL/SDL.h>
# include <SDL/SDL_thread.h>
# include <SDL/SDL_net.h>
#else
# include <SDL.h>
# include <SDL_thread.h>
# include <SDL_net.h>
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

#define MAX_THREADS		8

//------------------------------------------------------------------------------

#ifdef _OPENMP
typedef void _CDECL_ tThreadFunc (int);
#else
typedef int _CDECL_ tThreadFunc (void *);
#endif

typedef tThreadFunc *pThreadFunc;

class CThreadInfo {
	public:
		SDL_Thread*	pThread;
		SDL_sem*		done;
		SDL_sem*		exec;
		int			nId;
		int			bExec;
		int			bDone;
		int			bBlock;
		int			bQuit;
	};

//------------------------------------------------------------------------------

// The version number of the game
class CLegacyOptions {
	public:
		int bInput;
		int bProducers;
		int bMouse;
		int bHomers;
		int bRender;
		int bSwitches;
		int bWalls;

	public:
		CLegacyOptions () { Init (); }
		void Init (int i = 0);
	};

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
	int nHiliteColor;
	char bShowAmmo;
	char bEquipment;
	char bBoldHighlight;
	char nSort;
	ubyte alpha;
} tWeaponIconOptions;

//------------------------------------------------------------------------------

typedef struct tColorOptions {
	int bCap;
	int nLevel;
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
	int bSplitHUDMsgs;	//split player and other message displays
	int bWideDisplays;
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
	int nRadarPos;
	int nRadarSize;
	int nRadarRange;
	int nRadarColor;
	int nRadarStyle;
} tCockpitOptions;

//------------------------------------------------------------------------------

typedef struct tTextureOptions {
	int bUseHires [2];
	int nQuality;
} tTextureOptions;

//------------------------------------------------------------------------------

typedef struct tParticleOptions {
	int nQuality;
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
	int bDecreaseLag;	//only render if player is moving forward
	int bAuxViews;
	int bMonitors;
} tParticleOptions;

//------------------------------------------------------------------------------

typedef struct tLightningOptions {
	int nQuality;
	int nStyle;
	int bGlow;
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
	int b3DShields;
	int nSpin;
} tPowerupOptions;

//------------------------------------------------------------------------------

typedef struct tAutomapOptions {
	int bTextured;
	int bBright;
	int bCoronas;
	int bParticles;
	int bSparks;
	int bLightning;
	int bGrayOut;
	int bSkybox;
	int nColor;
	//int nRange;
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

#define SOFT_BLEND_SPRITES		1
#define SOFT_BLEND_SPARKS		2
#define SOFT_BLEND_PARTICLES	4

typedef struct tEffectRenderOptions {
	int bEnabled;
	int nShockwaves;
	int nShrapnels;
	int bEnergySparks;
	int bRobotShields;
	int bOnlyShieldHits;
	int bAutoTransparency;
	int bTransparent;
	int bSoftParticles;
	int bMovingSparks;
	int bGlow;
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

#define GLASSES_NONE						0
#define GLASSES_AMBER_BLUE				1
#define GLASSES_RED_CYAN				2
#define GLASSES_GREEN_MAGENTA			3
#define DEVICE_STEREO_PHYSICAL		4
#define DEVICE_STEREO_SIDEBYSIDE		4
#define GLASSES_OCULUS_RIFT_720p		4
#define GLASSES_OCULUS_RIFT_1080p	5
#define GLASSES_SHUTTER_HDMI			6
#define DEVICE_STEREO_DOUBLE_BUFFER	7
#define GLASSES_SHUTTER_NVIDIA		7

#define STEREO_PARALLEL					0
#define STEREO_TOE_IN					1

typedef struct tStereoRenderOptions {
	int nGlasses;
	int nMethod;
	int nScreenDist;
	int bEnhance;
	int bColorGain;
	int bDeghost;
	int bFlipFrames;
	int bBrighten;
	int nFOV;
	fix xSeparation;
} tStereoRenderOptions;

class CRenderOptions {
	public:
		int bAllSegs;
		int nLightingMethod;
		int bHiresModels [2];
		int nMeshQuality;
		int bUseLightmaps;
		int nLightmapQuality;
		int nLightmapPrecision;
		int bUseShaders;
		int nMathFormat;
		int nDefMathFormat;
		short nMaxFPS;
		int nPath;
		int nQuality;
		int nImageQuality;
		int nDebrisLife;
		tCameraOptions cameras;
		tColorOptions color;
		tCockpitOptions cockpit;
		tTextureOptions textures;
		tWeaponIconOptions weaponIcons;
		tParticleOptions particles;
		tLightningOptions lightning;
		tShadowOptions shadows;
		tPowerupOptions powerups;
		tAutomapOptions automap;
		tShipRenderOptions ship;
		tCoronaRenderOptions coronas;
		tEffectRenderOptions effects;
		tStereoRenderOptions stereo;
		tDebugRenderOptions debug;

	public:
		CRenderOptions () { Init (); }
		void Init (int i = 0);
		int ShadowQuality (void);
	};

//------------------------------------------------------------------------------

class COglOptions {
	public:
		int bSetGammaRamp;
		int bGlTexMerge;
		int bLightObjects;
		int bLightPowerups;
		int bObjLighting;
		int bHeadlight;
		int nMaxLightsPerFace;
		int nMaxLightsPerPass;
		int nMaxLightsPerObject;

	public:
		COglOptions () { Init (); }
		void Init (int i = 0);
	};

//------------------------------------------------------------------------------

class CMovieOptions {
	public:
		int bHires;
		int nQuality;
		int nLevel;
		int bResize;
		int bFullScreen;
		int bSubTitles;

	public:
		CMovieOptions () { Init (); }
		void Init (int i = 0);
	};

//------------------------------------------------------------------------------

class CGameplayOptions {
	public:
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
		int nShip [2];

	public:
		CGameplayOptions () { Init (); }
		void Init (int i = 0);
	};

//------------------------------------------------------------------------------

#define UNIQUE_JOY_AXES	5

typedef struct tMouseInputOptions {
	int bUse;
	int bSyncAxis;
	int bJoystick;
	int nDeadzone;
	int sensitivity [3];
	} tMouseInputOptions;

//------------------------------------------------------------------------------

typedef struct tJoystickInputOptions {
	int bUse;
	int bSyncAxis;
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
	int bSyncAxis;
	} tTrackIRInputOptions;

//------------------------------------------------------------------------------

typedef struct tKeyboardInputOptions {
	int nType;
	int bUse;
	int nRamp;
	int bRamp [3];
	} tKeyboardInputOptions;

//------------------------------------------------------------------------------

class CInputOptions {
	public:
		int bLimitTurnRate;
		int nMinTurnRate;
		int bUseHotKeys;
		tMouseInputOptions mouse;
		tJoystickInputOptions joystick;
		tTrackIRInputOptions trackIR;
		tKeyboardInputOptions keyboard;

	public:
		CInputOptions () { Init (); }
		void Init (int i = 0);
	};

//------------------------------------------------------------------------------

class CSoundOptions {
	public:
		int bUseD1Sounds;
		int bUseRedbook;
		int bHires [2];
		int bUseSDLMixer;
		int bUseOpenAL;
		int bFadeMusic;
		int bLinkVolumes;
		int audioSampleRate;	// what's used by the audio system
		int soundSampleRate;	// what the default sounds are in
		int bShip;
		int bMissiles;
		int bGatling;
		int bSpeedUp;
		fix xCustomSoundVolume;

	public:
		CSoundOptions () { Init (); }
		void Init (int i = 0);
	};

//------------------------------------------------------------------------------

typedef struct tAltBgOptions {
	int bHave;
	double alpha;
	double brightness;
	int grayscale;
	char szName [2][FILENAME_LEN];
} tAltBgOptions;

//------------------------------------------------------------------------------

class CMenuOptions {
	public:
		int nStyle;
		int bFastMenus;
		uint nFade;
		int bSmartFileSearch;
		int bShowLevelVersion;
		char nHotKeys;
		tAltBgOptions altBg;

	public:
		CMenuOptions () { Init (); }
		void Init (int i = 0);
	};

//------------------------------------------------------------------------------

class CDemoOptions {
	public:
		int bOldFormat;
		int bRevertFormat;

	public:
		CDemoOptions () { Init (); }
		void Init (int i = 0);
	};

//------------------------------------------------------------------------------

class CMultiplayerOptions {
	public:
		int bNoRankings;
		int bTimeoutPlayers;
		int bUseMacros;
		int bNoRedundancy;

	public:
		CMultiplayerOptions () { Init (); }
		void Init (int i = 0);
	};

//------------------------------------------------------------------------------

class CApplicationOptions {
	public:
		int bAutoRunMission;
		int nVersionFilter;
		int bSinglePlayer;
		int bEnableMods;
		int bExpertMode;
		int bEpilepticFriendly;
		int bColorblindFriendly;
		int bNotebookFriendly;
		int nScreenShotInterval;

	public:
		CApplicationOptions () { Init (); }
		void Init (int i = 0);
};

//------------------------------------------------------------------------------

class CGameOptions {
	public:
		CRenderOptions			render;
		CGameplayOptions		gameplay;
		CInputOptions			input;
		CMenuOptions			menus;
		CSoundOptions			sound;
		CMovieOptions			movies;
		CLegacyOptions			legacy;
		COglOptions				ogl;
		CApplicationOptions	app;
		CMultiplayerOptions	multi;
		CDemoOptions			demo;

	public:
		CGameOptions () { Init (); }
		void Init (int i = 0);
		bool Use3DPowerups (void);
		int UseHiresSound (void);
		inline int SoftBlend (int nFlag) { return (render.effects.bSoftParticles & nFlag) != 0; }
};

//------------------------------------------------------------------------------
// states
//------------------------------------------------------------------------------

class CSeismicStates {
	public:
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
	};

//------------------------------------------------------------------------------

class CSlowMotionStates {
	public:
		float		fSpeed;
		int		nState;
		time_t	tUpdate;
		int		bActive;
	};

//------------------------------------------------------------------------------

class CGameplayStates {
	public:	
		int bMultiBosses;
		int bFinalBossIsDead;
		int bHaveSmartMines;
		int bMineDestroyed;
		int bKillBossCheat;
		int bTagFlag;
		int nReactorCount [2];
		int nLastReactor;
		int bMineMineCheat;
		int bAfterburnerCheat;
		int bTripleFusion;
		int bLastAfterburnerState;
		fix xLastAfterburnerCharge;
		fix nPlayerSpeed;
		fix xInitialShield [2];
		fix xInitialEnergy [2];
		CFixVector vTgtDir;
		int nDirSteps;
		int nInitialLives;
		CSeismicStates seismic;
		CSlowMotionStates slowmo [2];

	public:
		fix InitialShield (void) {
			int h = xInitialShield [1];
			if (h <= 0)
				return xInitialShield [0];
			xInitialShield [1] = -1;
			return h;
			}

		fix InitialEnergy (void) {
			int h = xInitialEnergy [1];
			if (h <= 0)
				return xInitialEnergy [0];
			xInitialEnergy [1] = -1;
			return h;
			}
	};

//------------------------------------------------------------------------------

#define BOSS_COUNT	(extraGameInfo [0].nBossCount [1] - gameStates.gameplay.nReactorCount [0])


class CKeyStates {
	public:
		ubyte 	nBufferType;		// 0=No buffer, 1=buffer ASCII, 2=buffer scans
		ubyte 	bRepeat;
		ubyte 	bEditorMode;
		ubyte 	nLastPressed;
		ubyte 	nLastReleased;
		ubyte		pressed [256];
		int		xLastPressTime;
		};

class CInputStates {
	public:
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
		CKeyStates	keys;
	};

//------------------------------------------------------------------------------

class CMenuStates {
	public:
		int bHires;
		int bHiresAvailable;
		int nInMenu;
		int bInitBG;
		int bDrawCopyright;
		int bFullScreen;
		int bReordering;
		int bNoBackground;
	};

//------------------------------------------------------------------------------

class CMovieStates {
	public:
		int bIntroPlayed;
	};

//------------------------------------------------------------------------------

#include "player.h"

class CMultiplayerStates {
	public:
		int bUseTracker;
		int bTrackerCall;
		int bServer [2];
		int bKeepClients;
		int bHaveLocalAddress;
		int nGameType;
		int nGameSubType;
		int bTryAutoDL;
		int nConnection;
		int bIWasKicked;
		int bCheckPorts;
		ubyte bSurfingNet;
		int bPlayerIsTyping [MAX_PLAYERS];
	};

//------------------------------------------------------------------------------

class CGfxStates {
	public:
		int bInstalled;
		int bOverride;
		int nStartScrSize;
		int nStartScrMode;
	};

//------------------------------------------------------------------------------

class CCameraStates {
	public:
		int bActive;
	};

//------------------------------------------------------------------------------

class CTextureStates {
	public:
		int bGlTexMergeOk;
		int bHaveMaskShader;
		int bHaveGrayScaleShader;
		int bHaveEnhanced3DShader;
		int bHaveRiftWarpShader;
		int bHaveShadowMapShader;
	};

//------------------------------------------------------------------------------

class CCockpitStates {
	public:
		int bShowPingStats;
		int nType;
		int nNextType;
		int nTypeSave;
		int nShieldFlash;
		int bRedraw;
		int bBigWindowSwitch;
		int nLastDrawn [2];
		int n3DView [2];
		int nCoopPlayerView [2];
	};

//------------------------------------------------------------------------------

#if 0
class CVRBuffers {
	public:
		CCanvas		*offscreen;			// The offscreen data buffer
		CCanvas		render [2];			//  Two offscreen buffers for left/right eyes.
		CCanvas		subRender [2];		//  Two sub buffers for left/right eyes.
		CCanvas		screenPages [2];	//  Two pages of VRAM if paging is available
		CCanvas		editorCanvas;		//  The canvas that the editor writes to.
	};

class CVRStates {
	public:
		CScreenSize	m_screenSize;
		ubyte			nScreenFlags;	//see values in screens.h
		ubyte			nCurrentPage;
		fix			xEyeWidth;
		int			nRenderMode;
		int			nLowRes;			// Default to low res
		int 			bShowHUD;
		int			nSensitivity;	// 0 - 2
		int			xStereoSeparation;
		int			nEyeSwitch;
		int			bEyeOffsetChanged;
		int			bUseRegCode;
		CVRBuffers	buffers;
	};
#endif

//------------------------------------------------------------------------------

class CFontStates {
	public:
		int bHires;
		int bHiresAvailable;
		int bInstalled;
	};

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

typedef struct tRenderHistory {
	CBitmap		*bmBot;
	CBitmap		*bmTop;
	CBitmap		*bmMask;
	ubyte			bSuperTransp;
	ubyte			bShaderMerge;
	int			bOverlay;
	int			bColored;
	int			nType;
	int			nBlendMode;
} tRenderHistory;

class CZoomStates {
	public:
		int			nState;
		float			nFactor;
		fix			nMinFactor;
		fix			nMaxFactor;
		fix			nDestFactor;
		int			nChannel;
		float			nStep;
		time_t		nTime;
	};

class CRenderStates {
	public:
		int bChaseCam;
		int bFreeCam;
		int bEnableFreeCam;
		int bQueryOcclusion;
		int bVertexArrays;
		int bAutoMap;
		int bLightmapsOk;
		int bHaveLightmaps;
		int bUseLightmaps;
		int bDropAfterburnerBlob;
		int bOutsideMine;
		int bExtExplPlaying;
		int bDoAppearanceEffect;
		int bGlLighting;
		int bColored;
		int bBriefing;
		int bRearView;
		int bDepthSort;
		int nInterpolationMethod;
		int bTMapFlat;
		int bCloaked;
		int bBrightObject;
		int nWindow;
		int xStereoSeparation;
		int nStartSeg;
		int nLighting;
		int nMaxTextureQuality;
		int bTransparency;
		int bSplitPolys;
		int bHaveOculusRift;
		int bHaveDynLights;
		int bHaveSparks;
		int bUsePerPixelLighting;
		int nRenderPass;
		int nShadowPass;
		int nShadowBlurPass;
		int bBlurPass;
		int nShadowMap;
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
		int bSpecularColor;
		int bDoCameras;
		int bRenderIndirect;
		int bBuildModels;
		int bShowFrameRate;
		int bShowTime;
		int bShowProfiler;
		int bTriangleMesh;
		int bOmegaModded;
		int bPlasmaModded;
		int nFrameFlipFlop;
		int nModelQuality;
		int nMeshQuality;
		int nState;	//0: render geometry, 1: render objects
		int nType;
		int nFrameCount;
		int bUpdateEffects;
		int nLightingMethod;
		int bPerPixelLighting;
		int nMaxLightsPerPass;
		int nMaxLightsPerFace;
		int nMaxLightsPerObject;
		int bVSync;
		int bVSyncOk;
		int nThreads;
		int xPlaneDistTolerance;
		fix xZoom;
		fix xZoomScale;
		ubyte nRenderingType;
		fix nFlashScale;
		fix nFlashRate;
		CCameraStates cameras;
		CCockpitStates cockpit;
		//CVRStates vr;
		CFontStates fonts;
		CTextureStates textures;
		int bDetriangulation;
		GLenum nFacePrimitive;
		double glFOV;
		double glAspect;
		float grAlpha;
		tRenderDetail detail;
		tRenderHistory history;
	};

//------------------------------------------------------------------------------

class CAudioStates {
	public:
		int bNoMusic;
		int bSoundsInitialized;
		int bLoMem;
		int nNextSignature;
		int nActiveObjects;
		int nMaxChannels;
	};

//------------------------------------------------------------------------------

class CSoundStates {
	public:
		int bWasRecording;
		int bDontStartObjects;
		int nConquerWarningSoundChannel;
		int nSoundChannels;
		int bD1Sound;
		int bMidiFix;
		CAudioStates audio;
	};

//------------------------------------------------------------------------------

class CVideoStates {
	public:
		int nDisplayMode;
		int nDefaultDisplayMode;
		u_int32_t nScreenMode;
		u_int32_t nLastScreenMode;
		int nWidth;
		int nHeight;
		int bFullScreen;
	};

//------------------------------------------------------------------------------

class CCheatStates {
	public:
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
	};

//------------------------------------------------------------------------------

class CEntropyStates {
	public:
		int bConquering;
		int bConquerWarning;
		int bExitSequence;
		int nTimeLastMoved;
	};

//------------------------------------------------------------------------------

typedef struct tSlowTick {
	int bTick;
	int nTime;
	int nSlack;
	int nLastTick;
} tSlowTick;

class CApplicationStates {
	public:
		tSlowTick tick40fps;
		tSlowTick tick60fps;
	#if 1 //MULTI_THREADED
		int bExit;
		int bMultiThreaded;
		int nThreads;
	#endif
		int bDemoData;
		int bCheckAndFixSetup;
	#ifdef __unix__
		int bLinuxMsgBox;
	#endif
		int nSDLTicks [2];
		int nExtGameStatus;
		int nFunctionMode;
		int nLastFuncMode;
		int nCriticalError;
		int bStandalone;
		int bNostalgia;
		int iNostalgia;
		int bInitialized;
		int bD2XLevel;
		int bEnterGame;
		int bSaveScreenShot;
		int bShowVersionInfo;
		int bGameRunning;
		int bGameSuspended;
		int bGameAborted;
		int bBetweenLevels;
		int bPlayerIsDead;
		int bPlayerEggsDropped;
		int bDeathSequenceAborted;
		int bChangingShip;
		int bPlayerFiredLaserThisFrame;
		int bUseSound;
		int bMacData;
		int bCompressData;
		int bLunacy;
		int bEnglish;
		int bD1Data;
		int bD1Model;
		int bD1Mission;
		int bHaveD1Data;
		int bHaveD1Textures;
		int bHaveMod;
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
		int bReadOnly;
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
		int nLogLevel;
		int iDownloadTimeout;
		int bHaveSDLNet;
		bool bCustomData;
		bool bCustomSounds;
		fix xThisLevelTime;
		fix nPlayerTimeOfDeath;
		char *szCurrentMission;
		char *szCurrentMissionFile;
		tObjTransformation playerPos;
		short nPlayerSegment;
		uint nRandSeed;
		CCheatStates cheats;

		inline void SRand (uint seed = 0xffffffff) {
			if (seed == 0xffffffff) {
				seed = SDL_GetTicks ();
				seed *= seed;
				seed ^= (uint) time (NULL);
				}
			srand (nRandSeed = seed);
			}
	};

//------------------------------------------------------------------------------

class CLimitFPSStates {
	public:
		ubyte	bControls;
		ubyte bJoystick;
		ubyte	bSeismic;
		ubyte bCountDown;
		ubyte	bHomers;
		ubyte	bFusion;
		ubyte bOmega;
	};

//------------------------------------------------------------------------------

#define RENDER_TYPE_ZCULL				0
#define RENDER_TYPE_GEOMETRY			1
#define RENDER_TYPE_CORONAS			2
#define RENDER_TYPE_SKYBOX				3
#define RENDER_TYPE_OUTLINE			4
#define RENDER_TYPE_OBJECTS			5
#define RENDER_TYPE_TRANSPARENCY		6
#define RENDER_PASSES					7

class CGameStates {
	public:
		CGameplayStates		gameplay;
		CInputStates			input;
		CMenuStates				menus;
		CMovieStates			movies;
		CMultiplayerStates	multi;
		CGfxStates				gfx;
		CRenderStates			render;
		CZoomStates				zoom;
		CSoundStates			sound;
		CVideoStates			video;
		CApplicationStates	app;
		CEntropyStates			entropy;
		CLimitFPSStates		limitFPS;
	};

//------------------------------------------------------------------------------

extern CGameOptions	gameOptions [2];
extern CGameStates	gameStates;
extern CGameOptions	*gameOpts;

//------------------------------------------------------------------------------

class CMissionConfig {
	public:
		int	m_shipsAllowed [MAX_SHIP_TYPES];
		int	m_playerShip;
		int	m_bTeleport;
		int	m_bSecretSave;
		int	m_bColoredSegments;
		int	m_b3DPowerups;
		int	m_nCollisionModel;

	public:
		CMissionConfig () { Init (); }
		void Init (void);
		int Load (char* szFilename = NULL);
		void Apply (void);
		int SelectShip (int nShip);
};

extern CMissionConfig missionConfig;

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

#if 0
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
#endif

#define MAX_SHADOW_LIGHTS 8

typedef struct tLightRef {
	short			nSegment;
	short			nSide;
	short			nTexture;
} tLightRef;

//------------------------------------------------------------------------------

class CColorData {
	public:
		CArray<CFaceColor>	lights;
		CArray<CFaceColor>	sides;
		CArray<CFaceColor>	segments;
		CArray<CFaceColor>	vertices;
		CArray<float>			vertBright;
		CArray<CFaceColor>	ambient;	//static light values
		CArray<CFaceColor>	textures; // [MAX_WALL_TEXTURES];
		CArray<CFaceColor>	defaultTextures [2]; //[MAX_WALL_TEXTURES];
		CArray<tLightRef>		visibleLights;
		int						nVisibleLights;
		CFloatVector3			flagTag;
	public:
		CColorData ();
		bool Create (void);
		bool Resize (void);
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
class CVariableLight {
	public:
		short		m_nSegment;
		short		m_nSide;
		uint		m_mask;     // determines flicker pattern
		fix		m_timer;    // time until next change
		fix		m_delay;    // time between changes

	public:
		void Read (CFile& cf);
};

#define MAX_FLICKERING_LIGHTS 1000

class CFlickerLightData : public CArray< CVariableLight > {};

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
		CArray<CFloatVector3>			dynamicColor;
		CArray<ubyte>					bGotDynColor;
		ubyte								bGotGlobalDynColor;
		ubyte								bStartDynColoring;
		ubyte								bInitDynColoring;
		CFloatVector3						globalDynColor;
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
		bool Resize (void);
		void Destroy (void);
};

inline int operator- (CLightDeltaIndex* o, CArray<CLightDeltaIndex>& a) { return a.Index (o); }
inline int operator- (CLightDelta* o, CArray<CLightDelta>& a) { return a.Index (o); }

//------------------------------------------------------------------------------

class CShadowData {
	public:
		short					nLight;
		short					nLights;
		short					nShadowMaps;
		CDynLight*			lightP;
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

#define TERRAIN_GRID_MAX_SIZE   64
#define TERRAIN_GRID_SCALE      I2X (2*20)
#define TERRAIN_HEIGHT_SCALE    I2X (1)

class CTerrainRenderData {
	public:
		CArray<ubyte>			heightmap;
		CArray<fix>				lightmap;
		CArray<CFixVector>	points;
		CBitmap*					bmP;
		CStaticArray< CRenderPoint, TERRAIN_GRID_MAX_SIZE >		saveRow; // [TERRAIN_GRID_MAX_SIZE];
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

class CObjRenderList {
	public:
		CStaticArray< short, MAX_SEGMENTS_D2X >	ref; // [MAX_SEGMENTS_D2X];	//points to each segment's first render object list entry in renderObjs
		CStaticArray< tObjRenderListItem, MAX_OBJECTS_D2X >	objs; // [MAX_OBJECTS_D2X];
		int						nUsed;
	};


typedef struct tSegZRef {
	fix	z;
	short	nSegment;
} tSegZRef;

typedef struct tPortal {
	fix	left, right, top, bot;
	char  bProjected;
	ubyte bVisible;
} tPortal;

class CVisibilityData {
	public:
		int						nSegments;
		CShortArray				segments; //[MAX_SEGMENTS_D2X];
		CByteArray				bVisited; //[MAX_SEGMENTS_D2X];
		CByteArray				bVisible; //[MAX_SEGMENTS_D2X];
		CByteArray				bProcessed; //[MAX_SEGMENTS_D2X];		//whether each entry has been nProcessed
		ubyte 					nVisited;
		ubyte						nProcessed;
		ubyte						nVisible;
		CShortArray				nDepth; //[MAX_SEGMENTS_D2X];		//depth for each seg in nRenderList
		CArray<tSegZRef>		zRef [2]; // segment indexes sorted by distance from viewer
		CArray<tPortal>		portals;
		CShortArray				position; //[MAX_SEGMENTS_D2X];
		CArray<CRenderPoint>	points;

	public:
		CVisibilityData ();
		~CVisibilityData () { Destroy (); }
		bool Create (int nState);
		bool Resize (int nLength = -1);
		void Destroy (void);

		inline bool Visible (short nSegment) { return bVisible [nSegment] == nVisible; }
		inline bool Visited (short nSegment) { return bVisited [nSegment] == nVisited; }
		inline void Visit (short nSegment) { bVisited [nSegment] = nVisited; }
		ubyte BumpVisitedFlag (void);
		ubyte BumpProcessedFlag (void);
		ubyte BumpVisibleFlag (void);
		int SegmentMayBeVisible (short nStartSeg, int nRadius, int nMaxDist);
		void BuildSegList (CTransformation& transformation, short nStartSeg, int nWindow, bool bIgnoreDoors = false);
		void InitZRef (int i, int j, int nThread);
		void QSortZRef (short left, short right);

	private:
		int BuildAutomapSegList (void);
		void Sort (void);
		void MergeZRef (void);

	};

class CMineRenderData {
	public:
		//CFixVector				viewer.vPos;
		tObjTransformation	viewer;
		CVisibilityData		visibility [MAX_THREADS + 2];
		//CShortArray				renderSegList [MAX_THREADS]; //[MAX_SEGMENTS_D2X];
		CShortArray				objRenderSegList;
		int						nObjRenderSegs;
		CObjRenderList			objRenderList;
		CArray< CSegFace* >	renderFaceListP; //[MAX_SEGMENTS_D2X * 6];
		CIntArray				bObjectRendered; //[MAX_OBJECTS_D2X];
		CByteArray				bRenderSegment; //[MAX_SEGMENTS_D2X];
		CShortArray				nRenderObjList; //[MAX_SEGMENTS_D2X+N_EXTRA_OBJ_LISTS][OBJS_PER_SEG];
		CByteArray				bCalcVertexColor; //[MAX_VERTICES_D2X];
		CUShortArray			bAutomapVisited; //[MAX_SEGMENTS_D2X];
		CUShortArray			bAutomapVisible; //[MAX_SEGMENTS_D2X];
		CUShortArray			bRadarVisited; //[MAX_SEGMENTS_D2X];
		ubyte						bSetAutomapVisited;

	public:
		CMineRenderData ();
		~CMineRenderData () { Destroy (); }
		bool Create (int nState);
		bool Resize (int nLength = -1);
		void Destroy (void);
		bool Visible (short nSegment, int nThread = 0) { return visibility [nThread].Visible (nSegment); }
		bool Visited (short nSegment, int nThread = 0) { return visibility [nThread].Visited (nSegment); }
		void Visit (short nSegment, int nThread = 0) { return visibility [nThread].Visit (nSegment); }
		CByteArray& VisibleFlags (int nThread = 0) { return visibility [nThread].bVisible; }
		CByteArray& VisitedFlags (int nThread = 0) { return visibility [nThread].bVisited; }
};

//------------------------------------------------------------------------------

class CGameScreenData : public CCanvas {
	public:
		void Set (short l, short t, short w, short h) { SetLeft (l), SetTop (t), SetWidth (w), SetHeight (h); }
		inline uint Scalar (void) { return (((uint) Width ()) << 16) + (((uint) Height ()) & 0xFFFF); }
		inline uint Area (void) { return (uint) Width () * (uint) Height (); }
		void Set (uint scalar) { SetWidth ((short) (scalar >> 16)), SetHeight ((short) (scalar & 0xFFFF)); }
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
	CSegFace*			faceP;
	int					nNextItem;
} tFaceListItem;

class CFaceListIndex {
	public:
		CIntArray				roots; // [MAX_WALL_TEXTURES * 3];
		CIntArray				tails; // [MAX_WALL_TEXTURES * 3];
		CIntArray				usedKeys; // [MAX_WALL_TEXTURES * 3];
		//int						nUsedFaces;
		int						nUsedKeys;
	public:
		CFaceListIndex ();
		~CFaceListIndex ();
		void Create (void);
		void Destroy (bool bRebuild = false);
		void Init (void);
	};

#include "sphere.h"

#if OCULUS_RIFT
#	include "OVR.h"
#endif

class CRiftData {
	public:
#if OCULUS_RIFT
		OVR::Ptr<OVR::DeviceManager>			m_managerP;
		OVR::Ptr<OVR::HMDDevice>				m_hmdP;
		OVR::Ptr<OVR::SensorDevice>			m_sensorP;
		OVR::HMDInfo								m_hmdInfo;
		OVR::Util::Render::StereoConfig		m_stereoConfig;
		OVR::Util::Render::StereoEyeParams	m_eyes [2];
#endif

		float	m_renderScale;
		float m_fov;
		int	m_bAvailable;

		CRiftData () : m_renderScale (1.0f), m_fov (135.0f), m_bAvailable (false) {}
		bool Create (void);
		void Destroy (void);
		inline int Available (void) { return m_bAvailable; }
	};


class CRenderData {
	public:
		CRiftData					rift;
		CColorData					color;
		int							transpColor;
		CFaceListIndex				faceIndex;
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
		//COglData						ogl;
		CTerrainRenderData		terrain;
		CThrusterData				thrusters [MAX_PLAYERS];
		CMineRenderData			mine;
		CGameScreenData			screen; // entire screen
		CGameScreenData			frame;  // current part of the screen being rendered too (usually entire screen or left and right halves of it)
		CGameScreenData			viewport; // current part of the current frame being rendered to without offsets (relative to frame)
		CGameScreenData			scene; // "viewport" inside the viewport for scene rendering with the various HUDs (where parts of the scene are omitted)
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
		int							nUsedFaces;
		float							fAttScale [2];
		float							fBrightness;
		float							fScreenScale;
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

#define SEGVIS_FLAGS			((gameData.segs.nSegments + 7) >> 3)
#define VERTVIS_FLAGS		((gameData.segs.nVertices + 7) >> 3)

#define USE_RANGE_ELEMENTS	0

//------------------------------------------------------------------------------

typedef struct tFaceRenderVertex {
	CFloatVector3		vertex;
	CFloatVector3		normal;
	CFloatVector		color;
	tTexCoord2f			texCoord;
	tTexCoord2f			ovlTexCoord;
	tTexCoord2f			lMapTexCoord;
	} tFaceRenderVertex;

class CFaceData {
	public:
		CArray<CSegFace>			faces;
		CArray<tFaceTriangle>	tris;
		CArray<CFloatVector3>	vertices;
		CArray<CFloatVector3>	normals;
		CArray<tTexCoord2f>		texCoord;
		CArray<tTexCoord2f>		ovlTexCoord;
		CArray<tTexCoord2f>		lMapTexCoord;
		CArray<CFloatVector>		color;
		CArray<ushort>				faceVerts;
		CSegFace*					slidingFaces;
#if USE_RANGE_ELEMENTS
		CArray<GLuint>				vertIndex;
#endif	
		GLuint						vboDataHandle;
		GLuint						vboIndexHandle;
		ubyte*						vertexP;
		ushort*						indexP;
		int							nFaces;
		int							nTriangles;
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
		bool Resize (void);
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
		inline int GetSegList (short*& listP) {
			listP = Buffer ();
			return ToS ();
			}
};

//	-----------------------------------------------------------------------------------------------------------

#define LEVEL_SEGMENTS			gameData.segs.nSegments
#define LEVEL_OBJECTS			(gameData.objs.nMaxObjects)
#define LEVEL_VERTICES			(gameData.segs.nVertices)
#define LEVEL_POINT_SEGS		(gameData.segs.nSegments * 4)
#define LEVEL_SIDES				(LEVEL_SEGMENTS * 6)
#define LEVEL_FACES				(FACES.nFaces * 2)
#define LEVEL_TRIANGLES			(FACES.nTriangles ? FACES.nTriangles : LEVEL_FACES << gameStates.render.nMeshQuality)
#define LEVEL_DL_INDICES		(LEVEL_SEGMENTS / 2)
#define LEVEL_DELTA_LIGHTS		(LEVEL_SEGMENTS * 10)
#define LEVEL_SEGVIS_FLAGS		((LEVEL_SEGMENTS + 7) >> 3)
#define LEVEL_VERTVIS_FLAGS	((LEVEL_VERTICES + 7) >> 3)

#define QUADMATSIZE(_i)						((_i) * (_i))
#define QUADMATIDX(_i,_j,_rowSize)		((_i) * (_rowSize) + (_j))
#define TRIAMATSIZE(_i)						((_i) * ((_i) + 1) / 2)
#define TRIAMATIDX(_i, _j)					(((_j) < (_i)) ? TRIAMATSIZE (_i) + (_j) : TRIAMATSIZE (_j) + (_i))

typedef struct tSegGridIndex {
	int		nIndex;
	ushort	nSegments;
	} tSegGridIndex;

class CSegmentGrid {
	private:
		CArray<tSegGridIndex>	m_index;
		CShortArray					m_segments;
		CFixVector					m_vDim;
		CFixVector					m_vMin;
		CFixVector					m_vMax;
		int							m_nGridSize;

	public:
		bool Create (int nGridSize, int bSkyBox);
		void Destroy (void);
		int GetSegList (CFixVector vPos, short*& listP);
		inline int GridIndex (int x, int y, int z);
		inline bool Available (void) { return (m_segments.Buffer () != NULL); }
};


class CSegDistHeader {
	public:
		ushort	offset;
		ushort	length;
		float		scale;

	inline bool Read (CFile& cf) {
		return (cf.Read (&offset, sizeof (offset), 1, 0) == 1) &&
			    (cf.Read (&length, sizeof (length), 1, 0) == 1) &&
			    (cf.Read (&scale, sizeof (scale), 1, 0) == 1);
		}

	inline bool Write (CFile& cf) {
		return (cf.Write (&offset, sizeof (offset), 1, 0) == 1) &&
			    (cf.Write (&length, sizeof (length), 1, 0) == 1) &&
			    (cf.Write (&scale, sizeof (scale), 1, 0) == 1);
		}

	};


class CSegDistList : public CSegDistHeader {
	public:
		CArray<ushort> dist;

	inline void Set (ushort nSegment, fix xDistance) {
		nSegment -= offset;
		if (nSegment < length)
			dist [nSegment] = (xDistance < 0) ? 0xFFFF : (ushort) ((float) xDistance / scale);
		}

	inline int Get (ushort nSegment) {
		nSegment -= offset;
		if (nSegment >= length)
			return -1;
		ushort d = dist [nSegment];
		if (d == 0xFFFF)
			return -1;
		return (fix) ((float) d * scale);
		}

	inline bool Read (CFile& cf, int bCompressed) {
		if (!CSegDistHeader::Read (cf))
			return false;
		if (!length)
			return true;
		if (!(dist.Resize (length, false)))
			return false;
		return (dist.Read (cf, length, 0, bCompressed) > 0);
		}

	inline bool Write (CFile& cf, int bCompressed) {
		return CSegDistHeader::Write (cf) && dist.Write (cf, length, 0, bCompressed);
		}

	inline bool Create (void) { return (length > 0) && (dist.Create (length) != NULL); }

	inline void Destroy (void) { dist.Destroy (); }

	};


class CSegmentData {
	public:
		int							nMaxSegments;
		CArray<CFixVector>		vertices;
		CArray<CFloatVector>		fVertices;
		CArray<CSegment>			segments;
		CArray<tSegFaces>			segFaces;
		CArray<fix>					segDists;
		CSkyBox						skybox;
		CSegmentGrid				grids [2];
#if CALC_SEGRADS
		CArray<fix>					segRads [2];
		CArray<tSegExtent>		extent;
#endif
		CFixVector					vMin;
		CFixVector					vMax;
		fix							xDistScale;
		float							fRad;
		CArray<CFixVector>		segCenters [2];
		CArray<CFixVector>		sideCenters;
		CArray<ubyte>				bSegVis;
		CArray<ubyte>				bVertVis;
		CArray<ubyte>				bLightVis;
		CArray<CSegDistList>		segDistTable;
		CArray<short>				vertexSegments; // all segments using this vertex
		int							nVertices;
		int							nFaceVerts;
		int							nLastVertex;
		int							nSegments;
		int							nLastSegment;
		int							nFaces;
		int							nFaceKeys;
		int							nLevelVersion;
		char							szLevelFilename [FILENAME_LEN];
		CSecretData					secret;
		CArray<tSlideSegs>		slideSegs;
		short							nSlideSegs;
		int							bHaveSlideSegs;
		CFaceData					faces;

	public:
		CSegmentData ();
		void Init (void);
		bool Create (int nSegments, int nVertices);
		bool Resize (void);
		void Destroy (void);

		inline int SegVisSize (int nElements = 0) {
			if (!nElements)
				nElements = nSegments;
			return (TRIAMATSIZE (nSegments) + 7) / 8;
			}

		inline int SegVisIdx (int i, int j) {
			return TRIAMATIDX (i, j);
			}

		inline int SegVis (int i, int j) {
			i = SegVisIdx (i, j);	// index in triangular matrix, enforce j <= i
			return (i >= 0) && (bSegVis [i >> 3] & (1 << (i & 7))) != 0;
			}

		inline int SetSegVis (short nSrcSeg, short nDestSeg) {
			int i = SegVisIdx (nSrcSeg, nDestSeg);
			ubyte* flagP = &bSegVis [i >> 3];
			ubyte flag = 1 << (i & 7);
#ifdef _OPENMP
#	pragma omp atomic
#endif
			*flagP |= flag;
			return 1;
			}

		inline int SegDistSize (int nElements = 0) {
			if (!nElements)
				nElements = nSegments;
			return QUADMATSIZE (nSegments);
			}

		inline int SegDistIdx (int i, int j) {
			return QUADMATIDX (i, j, nSegments);
			}

		inline int SegDist (ushort i, ushort j) {
			return segDistTable [i].Get (j);
			}

		inline void SetSegDist (ushort i, ushort j, fix xDistance) {
			segDistTable [i].Set (j, xDistance);
			}

		inline int LightVisIdx (int i, int j) {
			return QUADMATIDX (i, j, nSegments);
			}

		inline sbyte LightVis (int nLight, int nSegment) {
			int i = LightVisIdx (nLight, nSegment);
			if ((i >> 2) >= (int) bLightVis.Length ())
				return 0;
			return sbyte (((bLightVis [i >> 2] >> ((i & 3) << 1)) & 3) - 1);
			}

		inline int SetLightVis (int nLight, int nSegment, ubyte flag) {
			int i = LightVisIdx (nLight, nSegment);
			ubyte* flagP = &bLightVis [i >> 2];
			flag <<= ((i & 3) << 1);
#ifdef _OPENMP
#	pragma omp atomic
#endif
			*flagP |= flag;
			return 1;
			}

		inline bool BuildGrid (int nSize, int bSkyBox) { return grids [bSkyBox].Create (nSize, bSkyBox); }
		inline int GetSegList (CFixVector vPos, short*& listP, int bSkyBox) { return grids [bSkyBox].GetSegList (vPos, listP); }
		inline bool HaveGrid (int bSkyBox) { return grids [bSkyBox].Available (); }
};

//------------------------------------------------------------------------------

class CWallData {
	public:
		CArray<CWall>				walls; //[MAX_WALLS];
		CStack<CExplodingWall>	exploding; //[MAX_EXPLODING_WALLS];
		CStack<CActiveDoor>		activeDoors; //[MAX_DOORS];
		CStack<CCloakingWall>	cloaking; //[MAX_CLOAKING_WALLS];
		CArray<tWallClip>			anims [2]; //[MAX_WALL_ANIMS];
		CArray<int>					bitmaps; //[MAX_WALL_ANIMS];
		int							nWalls;
		int							nAnims [2];
		CArray<tWallClip>			animP;

	public:
		CWallData ();
		void Reset (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

class CTriggerData {
	public:
		CArray<CTrigger>			triggers; // [MAX_TRIGGERS];
		CArray<CTrigger>			objTriggers; // [MAX_TRIGGERS];
		CArray<tObjTriggerRef>	objTriggerRefs; // [MAX_OBJ_TRIGGERS];
//		CArray<short>				firstObjTrigger; // [MAX_OBJECTS_D2X];
		CArray<int>					delay; // [MAX_TRIGGERS];
		int							m_nTriggers;
		int							m_nObjTriggers;
	public:
		CTriggerData ();
		bool Create (int nTriggers, bool bObjTriggers);
		void Destroy (void);
};

//------------------------------------------------------------------------------

class CPowerupData {
	public:
		CStaticArray< tPowerupTypeInfo, MAX_POWERUP_TYPES >	info; // [MAX_POWERUP_TYPES];
		int						nTypes;

	public:
		CPowerupData () { nTypes = 0; }
} ;

//------------------------------------------------------------------------------

class CObjTypeData {
	public:
		int						nTypes;
		CStaticArray< sbyte, MAX_OBJTYPE >	nType; //[MAX_OBJTYPE];
		CStaticArray< sbyte, MAX_OBJTYPE >	nId; //[MAX_OBJTYPE];  
		CStaticArray< fix, MAX_OBJTYPE >		nStrength; //[MAX_OBJTYPE];   
};

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

typedef struct tObjectViewData {
	CFixMatrix			mView;
	int					nFrame;
} tObjectViewData;

typedef struct tGuidedMissileInfo {
	CObject				*objP;
	int					nSignature;
} tGuidedMissileInfo;

class CObjLists {
public:
		tObjListRef				all;
		tObjListRef				players;
		tObjListRef				robots;
		tObjListRef				powerups;
		tObjListRef				weapons;
		tObjListRef				effects;
		tObjListRef				lights;
		tObjListRef				actors;
		tObjListRef				statics;

	CObjLists () { Init (); }
	void Init (void) { memset (this, 0, sizeof (*this)); }
	};

class CObjectData {
	public:
		CArray<CObjTypeData>		types;
		CArray<CObject>			objects;
		CStack<CObject*>			update;
		CArray<tBaseObject>		effects;
		CObjLists					lists;
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
		CFaceColor					color;
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
		int							nInitialRobots;
		int							nEffects;
		int							nDebris;
		int							nNextSignature;
		int							nChildFreeList;
		int							nDrops;
		int							nDeadControlCenter;
		int							nVertigoBotFlags;
		int							nFrameCount;
		CArray<short>				nHitObjects;
		CPowerupData				pwrUp;
		ubyte							collisionResult [MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];
		CArray<tObjectViewData>	viewData;
		CStaticArray< short, MAX_WEAPONS >	idToOOF; //[MAX_WEAPONS];
		CByteArray				bWantEffect; //[MAX_OBJECTS_D2X];

	public:
		CObjectData ();
		void Init (void);
		bool Create (void);
		void Destroy (void);
		void InitFreeList (void);
		void GatherEffects (void);
		int RebuildEffects (void);
};

#define PLAYER_LIGHTNING	1
#define ROBOT_LIGHTNING		2
#define MISSILE_LIGHTNING	4
#define EXPL_LIGHTNING		8
#define MOVE_LIGHTNING		16
#define DESTROY_LIGHTNING	32
#define SHRAPNEL_SMOKE		64
#define DESTROY_SMOKE		128

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
		CStaticArray< short, MAX_FVI_SEGS >	segments;
		CFVISideData		side;
		float					fLastTick;
		fix					xTime;
		fix					xAfterburnerCharge;
		fix					xBossInvulDot;
		CFixVector			playerThrust;
		int					nSegments;
		int					bIgnoreObjFlag;

	public:
		CPhysicsData ();
		void Init (void);
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
		int						nType;	// 0: D2, 1: D1
		CArray<ubyte>			data [2];
		CArray<CSoundSample>	sounds [2]; //[MAX_SOUND_FILES];
		int						nSoundFiles [2];
		CArray<CSoundSample>	soundP;
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
		CArray<ushort>				bitmapFlags [2]; //[MAX_BITMAP_FILES];
		CArray<CBitmap>			bitmaps [2]; //[MAX_BITMAP_FILES];
		CArray<CBitmap>			altBitmaps [2]; //[MAX_BITMAP_FILES];
		CArray<CBitmap>			addonBitmaps ; //[MAX_ADDON_BITMAP_FILES];
		CArray<ushort>				bitmapXlat ; //[MAX_BITMAP_FILES];
		CArray<alias>				aliases ; //[MAX_ALIASES];
		CArray<tBitmapIndex>		bmIndex [2]; //[MAX_TEXTURES];
		CArray<tBitmapIndex>		objBmIndex ; //[MAX_OBJ_BITMAPS];
		CArray<tBitmapIndex>		defaultObjBmIndex ; //[MAX_OBJ_BITMAPS];
		CArray<short>				textureIndex [2]; //[MAX_BITMAP_FILES];
		CArray<ushort>				objBmIndexP ; //[MAX_OBJ_BITMAPS];
		CArray<tBitmapIndex>		cockpitBmIndex; //[N_COCKPIT_BITMAPS];
		CArray<CFloatVector3>		bitmapColors ; //[MAX_BITMAP_FILES];
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
		CArray<tVideoClip>	vClips [2]; //[MAX_VCLIPS];
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
	tVideoClip*			vcP;
	tVideoClipInfo		vci;
	CFlightPath			path;
} tFlagData;

//------------------------------------------------------------------------------

class CPigData {
	public:
		CTextureData	tex;
		CSoundData		sound;
		CShipData		ship;
		tFlagData		flags [2];

	public:
		CPigData () {}
		~CPigData () {}
};

//------------------------------------------------------------------------------

#include "fireweapon.h"

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
		CStaticArray< CD1WeaponInfo, D1_MAX_WEAPON_TYPES >	infoD1; // [D1_MAX_WEAPON_TYPES];
		CArray<CFloatVector>	color;
		ubyte						bLastWasSuper [2][MAX_PRIMARY_WEAPONS];

	public:
		CWeaponData ();
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

class CModelHitboxList {
	public:
		int			nHitboxes;
		int			nFrame;
		CStaticArray< tHitbox, MAX_HITBOXES + 1 >	hitboxes; // [MAX_HITBOXES + 1];
#if DBG
		CFixVector	vHit;
		time_t		tHit;
#endif
	};

#define MAX_THRUSTERS		16
#define REAR_THRUSTER		1
#define FRONT_THRUSTER		2
#define LEFT_THRUSTER		4
#define RIGHT_THRUSTER		8
#define TOP_THRUSTER			16
#define BOTTOM_THRUSTER		32
#define FRONTAL_THRUSTER	64
#define LATERAL_THRUSTER	128

#define FR_THRUSTERS			(FRONT_THRUSTER | REAR_THRUSTER)
#define LR_THRUSTERS			(LEFT_THRUSTER | RIGHT_THRUSTER)
#define TB_THRUSTERS			(TOP_THRUSTER | BOTTOM_THRUSTER)

class CModelThrusters {
	public:
		CFixVector	vPos [MAX_THRUSTERS];
		CFixVector	vDir [MAX_THRUSTERS];
		float			fSize [MAX_THRUSTERS];
		ubyte			nType [MAX_THRUSTERS];
		short			nCount;
	};

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
		CArray<CRenderPoint>					polyModelPoints ; //[MAX_POLYGON_VERTS];
		CArray<CFloatVector>				fPolyModelVerts ; //[MAX_POLYGON_VERTS];
		CArray<CBitmap*>					textures ; //[MAX_POLYOBJ_TEXTURES];
		CArray<tBitmapIndex>				textureIndex ; //[MAX_POLYOBJ_TEXTURES];
		CStaticArray< CModelHitboxList,MAX_POLYGON_MODELS >	hitboxes ; //[MAX_POLYGON_MODELS];
		CStaticArray< CModelThrusters,MAX_POLYGON_MODELS >	thrusters ; //[MAX_POLYGON_MODELS];
		CArray<RenderModel::CModel>	renderModels [2]; //[MAX_POLYGON_MODELS];
		CArray<CFixVector>				offsets ; //[MAX_POLYGON_MODELS];
		CArray<tGunInfo>					gunInfo ; //[MAX_POLYGON_MODELS];
		CArray<tModelSphere>				spheres ; //[MAX_POLYGON_MODELS];
		CFixVector							vScale;
		char									szShipModels [MAX_SHIP_TYPES][FILENAME_LEN];

	public:
		CModelData ();
		bool Create (void);
		void Destroy (void);
		void Prepare (void);
};

//------------------------------------------------------------------------------

class CAutoNetGame {
	public:
		char					szPlayer [9];		//player profile name
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
		tFiringData				firing [2];
		fix						xMslFireTime;
		fix						xFusionCharge;
		short						nAmmoUsed;
		char						nMissiles;
		char						nPrimary;
		char						nSecondary;
		char						bQuadLasers;
		char						nLaserLevel;
		char						bTripleFusion;
		char						nMslLaunchPos;
		ubyte						nBuiltinMissiles;
		ubyte						nThrusters [5];
		ubyte						nShip;

	public:
		CWeaponState () { memset (this, 0, sizeof (*this)); }
	};

#define BUILTIN_MISSILES	(2 + NDL - gameStates.app.nDifficultyLevel)

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
		bool								bAdjustPowerupCap [MAX_PLAYERS];
		CArray<tLeftoverPowerup>	leftoverPowerups;
		CAutoNetGame					autoNG;
		fix								xStartAbortMenuTime;

	public:
		CMultiplayerData ();
		bool Create (void);
		void Destroy (void);
		inline void Connect (int nPlayer, sbyte nStatus) {
#if DBG
			if ((nPlayer != nLocalPlayer) && (nStatus < 2))
				nStatus = nStatus;
			if (nPlayer < MAX_PLAYERS + 4)
#endif
			players [nPlayer].Connect (nStatus);
			}

		bool WaitingForExplosion (void) {
			for (int i = 0; i < nPlayers; i++)
				if (players [i].WaitingForExplosion ())
					return true;
			return false;
			}

		bool WaitingForWeaponInfo (void) {
			for (int i = 0; i < nPlayers; i++) {
				if (weaponStates [i].nShip == 255)
					return true;
				if ((i != nLocalPlayer) && players [i].WaitingForWeaponInfo ())
					return true;
				}
			return false;
			}

		inline ushort PrimaryAmmo (short nPlayer, short nWeapon) { return players [nPlayer].primaryAmmo [nWeapon]; }
		inline ushort SecondaryAmmo (short nPlayer, short nWeapon, int bBuiltin = 1) { 
			ushort nAmmo = players [nPlayer].secondaryAmmo [nWeapon];
			if (nWeapon || bBuiltin)
				return nAmmo; 
			ushort nBuiltin = BuiltinMissiles (nPlayer);
			return (nAmmo > nBuiltin) ? nAmmo - nBuiltin : 0;
			}
		inline ushort BuiltinMissiles (short nPlayer) { return weaponStates [nPlayer].nBuiltinMissiles; }
		inline bool Flag (short nPlayer, uint nFlag) { return (players [nPlayer].flags & nFlag) != 0; }
};

#include "multi.h"

//------------------------------------------------------------------------------

class CMultiCreateData {
	public:
		CStaticArray< int, MAX_NET_CREATE_OBJECTS >	nObjNums; // [MAX_NET_CREATE_OBJECTS];
		int					nCount;

	public:
		CMultiCreateData () { nCount = 0; }
};

#define MAX_FIRED_OBJECTS	8

class CMultiLaserData {
	public:
		int					bFired;
		ubyte					nFired [2];
		short					nObjects [2][MAX_FIRED_OBJECTS];
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

class CMultiScoreData {
	public:
		char					pFlags [MAX_PLAYERS];
		int					nSorted [MAX_PLAYERS];
		short					matrix [MAX_PLAYERS][MAX_PLAYERS];
		short					nTeam [2];
		char					bShowList;
		fix					xShowListTimer;

	public:
		CMultiScoreData () { memset (this, 0, sizeof (*this)); }
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
		CMultiScoreData	score;
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

#include "mission.h"

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

#if 0

#define NUM_MARKERS         (MAX_PLAYERS * 3)
#define MARKER_MESSAGE_LEN  40

class CMarkerData {
	public:
		CStaticArray< CFixVector, NUM_MARKERS >	position; // [NUM_MARKERS];	//three markers (two regular + one spawn) per player in multi
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
		bool					bRotate;

	public:
		CMarkerData ();
		void Init (void);
};

#endif

//------------------------------------------------------------------------------

class CTimeData {
	public:
		fix					xFrame;	//  since last frame, in seconds
		float					fFrame;
		fix					xRealFrame;
		fix					xGame;	//	 in game, in seconds
		fix					xGameStart;
		fix					xGameTotal;
		fix					xMaxOnline;
		fix					xLast;
		int					tLast;
		fix					xSlack;
		fix					xStarted;
		fix					xStopped;
		fix					xLastThiefHitTime;
		int					nPaused;
		int					nStarts;
		int					nStops;

	public:
		CTimeData ();
		inline void SetTime (fix xTime) {
			xFrame = xTime;
			fFrame = X2F (xTime);
			}
};

//------------------------------------------------------------------------------

typedef enum {
	rtSound,
	rtStaticVertLight,
	rtComputeFaceLight,
	rtInitSegZRef,
	rtSortSegZRef,
	rtTranspRender,
	rtEffects,
	rtPolyModel,
	rtParticles,
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
		int					argC;
		char**				argV;

	public:
		CApplicationData ();
		inline int GameMode (int nFlags = 0xFFFFFFFF) { return nGameMode & nFlags; }
		inline void SetGameMode (int nMode) {
			nGameMode = nMode;
			}
};

//------------------------------------------------------------------------------

#if PROFILING

typedef enum tProfilerTags {
	ptFrame,
	ptRenderFrame,
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
	ptEffects,
	ptParticles,
	ptCockpit,
	ptTransform,
	ptVertexColor,
	ptFaceList,
	ptUpdateObjects,
	ptAux,
	ptTagCount
	} tProfilerTags;

typedef struct tProfilerData {
	time_t				t [ptTagCount];
} tProfilerData;

#define PROF_INIT			memset(&gameData.profiler.t, 0, sizeof (gameData.profiler.t));
#define PROF_START		time_t tProf = clock ();
#define PROF_CONT			tProf = clock ();
#define PROF_END(_tag)	(gameData.profiler.t [_tag]) += clock () - tProf;

#else

#define PROF_INIT
#define PROF_START	
#define PROF_CONT	
#define PROF_END(_tAcc)

#endif

//------------------------------------------------------------------------------

#define MAX_SEGS_VISITED			1000
#define MAX_BOSS_COUNT				50
#define MAX_BOSS_TELEPORT_SEGS	MAX_SEGS_VISITED
#define NUM_D2_BOSSES				8
#define BOSS_CLOAK_DURATION		I2X (7)
#define BOSS_DEATH_DURATION		I2X (6)

class CBossInfo {
	public:
		short					m_nTeleportSegs;
		CShortArray			m_teleportSegs; // [MAX_BOSS_TELEPORT_SEGS];
		short					m_nGateSegs;
		CShortArray			m_gateSegs; // [MAX_BOSS_TELEPORT_SEGS];
		fix					m_nDyingStartTime;
		fix					m_nHitTime;
		fix					m_nCloakStartTime;
		fix					m_nCloakEndTime;
		fix					m_nCloakDuration;
		fix					m_nCloakInterval;
		fix					m_nLastTeleportTime;
		fix					m_nTeleportInterval;
		fix					m_nLastGateTime;
		fix					m_nGateInterval;
	#if DBG
		fix					m_xPrevShield;
	#endif
		int					m_bHitThisFrame;
		int					m_bHasBeenHit;
		int					m_nObject;
		short					m_nDying;
		sbyte					m_bDyingSoundPlaying;

	public:
		CBossInfo () { Init (); }
		~CBossInfo () { Destroy (); }
		void Init (void);
		bool Setup (short nObject);
		void Destroy (void);
		bool SetupSegments (CShortArray& segments, int bSizeCheck, int bOneWallHack);
		void InitGateInterval (void);
		void SaveState (CFile& cf);
		void SaveSizeStates (CFile& cf);
		void SaveBufferStates (CFile& cf);
		void LoadBinState (CFile& cf);
		void LoadState (CFile& cf, int nVersion);
		void LoadSizeStates (CFile& cf);
		int LoadBufferStates (CFile& cf);

		inline void ResetHitTime (void) { m_nHitTime = -I2X (10); }
		void ResetCloakTime (void);

	private:
		void SaveBufferState (CFile& cf, CShortArray& buffer);
		int LoadBufferState (CFile& cf, CShortArray& buffer, int nBufSize);
	};

class CBossData {
	private:	
		CStack<CBossInfo>	m_info;

	public:
		CBossData ();
		bool Create (uint nBosses = 0);
		void Destroy (void);
		void Setup (int nObject = -1);
		short Find (short nBossObj);
		int Add (short nObject);
		void Remove (short nBoss);
		void ResetHitTimes (void);
		void ResetCloakTimes (void);
		void InitGateIntervals (void);

		inline void Shrink (uint i = 1) { m_info.Shrink (i); }
		inline uint ToS (void) { return m_info.ToS (); }
		inline bool Grow (uint i = 1) { return m_info.Grow (i); }
		inline CBossInfo& Boss (uint i) { return m_info [i]; }
		inline uint BossCount (void) { return m_info.Buffer () ? m_info.ToS () : 0; }
		inline uint Count (void) { return m_info.Buffer () ? m_info.ToS () : 0; }
		inline CBossInfo& operator[] (uint i) { return m_info [i]; }

		void SaveStates (CFile& cf);
		void SaveSizeStates (CFile& cf);
		void SaveBufferStates (CFile& cf);
		void LoadBinStates (CFile& cf);
		void LoadStates (CFile& cf, int nVersion);
		void LoadSizeStates (CFile& cf);
		int LoadBufferStates (CFile& cf);
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
		CTriggerTargets	triggers;
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

class CAITarget {
	public:
		short							nBelievedSeg;
		CFixVector					vBelievedPos;
		CFixVector					vLastPosFiredAt;
		fix							nDistToLastPosFiredAt;
		fix							xDist;
		CFixVector					vDir;
		CObject*						objP;
};

#define TARGETOBJ	(gameData.ai.target.objP ? gameData.ai.target.objP : gameData.objs.consoleP)

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
		CHitResult					hitResult;
		CAITarget					target;
		CArray<tAILocalInfo>		localInfo;
		CArray<tAICloakInfo>		cloakInfo; // [MAX_AI_CLOAK_INFO];
		CArray<tPointSeg>			routeSegs; // [MAX_POINT_SEGS];
		tPointSeg*					freePointSegs;
		int							nAwarenessEvents;
		int							nMaxAwareness;
		CFixVector					vGunPoint;
		int							nTargetVisibility;
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

#include "producers.h"

class CProducerData {
	public:
		fix				xFuelRefillSpeed;
		fix				xFuelGiveAmount;
		fix				xFuelMaxAmount;
		CStaticArray< tProducerInfo, MAX_FUEL_CENTERS >	producers; //[MAX_FUEL_CENTERS];
		CStaticArray< tObjectProducerInfo, MAX_ROBOT_CENTERS >	robotMakers; //[MAX_ROBOT_CENTERS];
		CStaticArray< tObjectProducerInfo, MAX_EQUIP_CENTERS >	equipmentMakers; //[MAX_EQUIP_CENTERS];
		int				nProducers;
		int				nRobotMakers;
		int				nEquipmentMakers;
		int				nRepairCenters;
		fix				xEnergyToCreateOneRobot;
		CStaticArray< int, MAX_FUEL_CENTERS >				origProducerTypes; // [MAX_FUEL_CENTERS];
		CSegment*		playerSegP;

	public:
		CProducerData ();
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
	CBitmap		bm;
	CPalette		*palette;
} tHoardItem;

class CHoardData {
	public:
		int			bInitialized;
		int			nTextures;
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

#include "hudmsgs.h"

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

class CVertColorThreadData {
	public:
#if MULTI_THREADED_LIGHTS
		CThreadInfo		info [2];
#endif
		CVertColorData	data;
	};

class CClipDistData {
	public:
		CObject*				objP;
		POF::CModel*		po;
		POF::CSubModel*	pso;
		float					fClipDist [2];
	};

class CClipDistThreadData {
	public:
#if MULTI_THREADED_SHADOWS
		CThreadInfo		info [2];
#endif
		CClipDistData	data;
	};

class CThreadData {
	public:
		CVertColorThreadData		vertColor;
		CClipDistThreadData		clipDist;
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
		fix		xUpdateTime;
		int		nGlobalFiringCount;
		int		nMissileGun;
		int		nOffset;
		int		nProximityDropped;
		int		nSmartMinesDropped;
};

//------------------------------------------------------------------------------

class CFusionData {
	public:
		fix	xAutoFireTime;
		fix	xCharge;
		fix	xNextSoundTime;
		fix	xLastSoundTime;
		int	xFrameTime;
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
		CHitInfo hitResult;
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
		int nHighscore;
		int nChampion;
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
		CEffectData			effects;
		CPigData				pig;
		CModelData			models;
		CMultiplayerData	multiplayer;
		CMultiGameData		multigame;
		CMuzzleData			muzzle;
		CWeaponData			weapons;
		CEntropyData		entropy;
		CReactorData		reactor;
		//CMarkerData			marker;
		CBossData			bosses; // [MAX_BOSS_COUNT];
		CAIData				ai;
		CEndLevelData		endLevel;
		CMenuData			menu;
		CProducerData		producers;
		CDemoData			demo;
		CEscortData			escort;
		CThiefData			thief;
		CHoardData			hoard;
		CHUDData				hud;
		CTerrainData		terrain;
		CTimeData			time;
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
		bool Create (int nSegments, int nVertices);
		void Destroy (void);
		void SetFusionCharge (fix xCharge, bool bLocal = false);
		fix FusionCharge (int nId = -1);
		fix FusionDamage (fix xBaseDamage);
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
	public:
		int		offset;
		int		count;
		int		size;

	public:
		inline void Read (CFile& cf) {
			offset = cf.ReadInt ();				// Player info
			count = cf.ReadInt ();
			size = cf.ReadInt ();
			}

} __pack__ tGameItemInfo;

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
} __pack__ tGameFileInfo;

extern tGameFileInfo gameFileInfo;

//-----------------------------------------------------------------------------

static inline short ObjIdx (CObject *objP)
{
#if DBG
if (gameData.objs.objects.IsElement (objP))
	return gameData.objs.objects.Index (objP);
return -1;
#else
return gameData.objs.objects.IsElement (objP) ? gameData.objs.objects.Index (objP) : -1;
#endif
}

//	-----------------------------------------------------------------------------------------------------------

#define	NO_WALL					(gameStates.app.bD2XLevel ? 2047 : 255)
#define  IS_WALL(_wallnum)		((ushort (_wallnum) != NO_WALL) && (ushort (_wallnum) < gameFileInfo.walls.count))

#define SEG_IDX(_segP)			((short) ((_segP) - SEGMENTS))
#define SEG2_IDX(_seg2P)		((short) ((_seg2P) - SEGMENTS))
#define WALL_IDX(_wallP)		((short) ((_wallP) - WALLS))
#define OBJ_IDX(_objP)			ObjIdx (_objP)
#define TRIG_IDX(_triggerP)	((short) ((_triggerP) - TRIGGERS))
#define FACE_IDX(_faceP)		((int) ((_faceP) - FACES.faces))

void GrabMouse (int bGrab, int bForce);
void SetDataVersion (int v);

//	-----------------------------------------------------------------------------------------------------------

static inline void OglVertex3x (fix x, fix y, fix z)
{
glVertex3f ((float) x / 65536.0f, (float) y / 65536.0f, (float) z / 65536.0f);
}

//	-----------------------------------------------------------------------------------------------------------

static inline void OglVertex3f (CRenderPoint *p, CFloatVector* v = NULL)
{
	int i = p->Index ();
if (v) {
	if (i < 0)
		v->Assign (p->ViewPos ());
	else
		*v = gameData.render.vertP [i];
	}
else {
	if (i < 0)
		OglVertex3x (p->ViewPos ().v.coord.x, p->ViewPos ().v.coord.y, p->ViewPos ().v.coord.z);
	else
		glVertex3fv (reinterpret_cast<GLfloat *> (gameData.render.vertP + i));
	}
}

//	-----------------------------------------------------------------------------------------------------------

static inline float GrAlpha (ubyte alpha)
{
if (alpha >= FADE_LEVELS)
	return 1.0f;
return 1.0f - float (alpha) / float (FADE_LEVELS);
}

//	-----------------------------------------------------------------------------------------------------------

#define	CLAMP(_val,_minVal,_maxVal)	\
		 {if ((_val) < (_minVal)) (_val) = (_minVal); else if ((_val) > (_maxVal)) (_val) = (_maxVal);}

#define N_LOCALPLAYER	gameData.multiplayer.nLocalPlayer

#define LOCALPLAYER		gameData.multiplayer.players [N_LOCALPLAYER]

#define ISLOCALPLAYER(_nPlayer)	((_nPlayer < 0) || ((_nPlayer) == N_LOCALPLAYER))

#define CONNECT(_nPlayer, _nStatus)	gameData.multiplayer.Connect (_nPlayer, _nStatus)

#define G3_INFINITY			fInfinity [gameOpts->render.shadows.nReach]

#define MAX_LIGHT_RANGE	I2X (222) // light cast by the brightest possible light will have faded to black past this distance

//	-----------------------------------------------------------------------------------------------------------

extern float fInfinity [];
extern fix nDebrisLife [];

#define sizeofa(_a)	(sizeof (_a) / sizeof ((_a) [0]))	//number of array elements

#define CLEAR(_v)		memset (_v, 0, sizeof (_v))

#define SEGMENTS			gameData.segs.segments
#define VERTICES			gameData.segs.vertices
#define FVERTICES			gameData.segs.fVertices
#define SEGFACES			gameData.segs.segFaces
#define OBJECTS			gameData.objs.objects
#define WALLS				gameData.walls.walls
#define TRIGGERS			gameData.trigs.triggers
#define OBJTRIGGERS		gameData.trigs.objTriggers
#define FACES				gameData.segs.faces
#define RENDERPOINTS		gameData.render.mine.visibility [0].points
#define TRIANGLES			FACES.tris

#define SPECTATOR(_objP)	((gameStates.render.bFreeCam > 0) && OBJECTS.IsElement (_objP) && (OBJ_IDX (_objP) == LOCALPLAYER.nObject))
#define OBJPOS(_objP)		(SPECTATOR (_objP) ? &gameStates.app.playerPos : &(_objP)->info.position)
#define OBJSEG(_objP)		(SPECTATOR (_objP) ? gameStates.app.nPlayerSegment : (_objP)->info.nSegment)

//	-----------------------------------------------------------------------------

static inline CFixVector *PolyObjPos (CObject *objP, CFixVector *vPosP)
{
CFixVector vPos = OBJPOS (objP)->vPos;
if (objP->info.renderType == RT_POLYOBJ) {
	*vPosP = *objP->View () * gameData.models.offsets [objP->ModelId ()];
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
} __pack__ fVector3D;

typedef struct tTransRotInfo {
	fVector3D	fvRot;
	fVector3D	fvTrans;
	} __pack__ tTransRotInfo;

#ifndef _WIN32
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

#ifndef G3_SLEEP
#	ifdef _WIN32
#		define	G3_SLEEP(_t)	Sleep (_t)
#	else
#		include <unistd.h>
#		define	G3_SLEEP(_t)	usleep ((_t) * 1000)
#	endif
#endif

#define HW_GEO_LIGHTING 0 //gameOpts->ogl.bGeoLighting

#define SEM_SMOKE			0
#define SEM_LIGHTNING	1
#define SEM_SPARKS		2
#define SEM_SHRAPNEL		3

#include "error.h"

static inline void SemWait (uint sem)
{
	time_t t0 = gameStates.app.nSDLTicks [0];

while (gameData.app.semaphores [sem]) {
	G3_SLEEP (0);
	if (SDL_GetTicks () - t0 > 50) {
		PrintLog (0, "multi threading got stuck (semaphore: %d)\n", sem);
		gameStates.app.bMultiThreaded = 1;
		gameData.app.semaphores [sem] = 0;
		break;
		}
	}
}

#if DBG

static inline void SemEnter (uint sem, const char *pszFile, int nLine)
{
SemWait (sem);
//PrintLog (0, "SemEnter (%d) @ %s:%d\n", sem, pszFile, nLine);
gameData.app.semaphores [sem]++;
}

static inline void SemLeave (uint sem, const char *pszFile, int nLine)
{
if (gameData.app.semaphores [sem]) {
	gameData.app.semaphores [sem]--;
	//PrintLog (0, "SemLeave (%d) @ %s:%d\n", sem, pszFile, nLine);
	}
else
	PrintLog (0, "asymmetric SemLeave (%d) @ %s:%d\n", sem, pszFile, nLine);
}

#define SEM_WAIT(_sem)	if (gameStates.app.bMultiThreaded) SemWait (_sem);

#define SEM_ENTER(_sem)	if (gameStates.app.bMultiThreaded) SemEnter (_sem, __FILE__, __LINE__);

#define SEM_LEAVE(_sem)	if (gameStates.app.bMultiThreaded) SemLeave (_sem, __FILE__, __LINE__);

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

#define SEM_WAIT(_sem)	if (gameStates.app.bMultiThreaded) SemWait (_sem);

#define SEM_ENTER(_sem)	if (gameStates.app.bMultiThreaded) SemEnter (_sem);

#define SEM_LEAVE(_sem)	if (gameStates.app.bMultiThreaded) SemLeave (_sem);

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

#define EX_OUT_OF_MEMORY		1
#define EX_IO_ERROR				2

//	-----------------------------------------------------------------------------------------------------------

#endif


