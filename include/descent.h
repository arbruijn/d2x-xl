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
#	define	PROFILING 1
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

#define MAX_THREADS		8

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
# include <SDL_net/SDL_net.h>
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

//------------------------------------------------------------------------------

#ifdef _OPENMP
typedef void _CDECL_ tThreadFunc (int32_t);
#else
typedef int32_t _CDECL_ tThreadFunc (void *);
#endif

typedef tThreadFunc *pThreadFunc;

class CThreadInfo {
	public:
		SDL_Thread*	pThread;
		SDL_sem*		done;
		SDL_sem*		exec;
		int32_t		nId;
		int32_t		bExec;
		int32_t		bDone;
		int32_t		bBlock;
		int32_t		bQuit;
	};

//------------------------------------------------------------------------------

// The version number of the game
class CLegacyOptions {
	public:
		int32_t bInput;
		int32_t bProducers;
		int32_t bMouse;
		int32_t bHomers;
		int32_t bRender;
		int32_t bSwitches;
		int32_t bWalls;

	public:
		CLegacyOptions () { Init (); }
		void Init (int32_t i = 0);
	};

//------------------------------------------------------------------------------

typedef struct tCameraOptions {
	int32_t nFPS;
	int32_t nSpeed;
	int32_t bFitToWall;
	int32_t bHires;
} tCameraOptions;

//------------------------------------------------------------------------------

typedef struct tWeaponIconOptions {
	int32_t bSmall;
	int32_t nHiliteColor;
	char bShowAmmo;
	char bEquipment;
	char bBoldHighlight;
	char nSort;
	uint8_t alpha;
} tWeaponIconOptions;

//------------------------------------------------------------------------------

typedef struct tColorOptions {
	int32_t bCap;
	int32_t nLevel;
	int32_t bWalls;
	int32_t bMix;
	int32_t bUseLightmaps;
	int32_t nLightmapRange;
	int32_t nSaturation;
} tColorOptions;

//------------------------------------------------------------------------------

typedef struct tCockpitOptions {
	int32_t bHUD;
	int32_t bHUDMsgs;
	int32_t bSplitHUDMsgs;	//split player and other message displays
	int32_t bWideDisplays;
	int32_t bReticle;
	int32_t nShipStateLayout;
	int32_t bMouseIndicator;
	int32_t bTextGauges;
	int32_t bScaleGauges;
	int32_t bFlashGauges;
	int32_t bMissileView;
	int32_t bGuidedInMainView;
	int32_t bObjectTally;
	int32_t bRotateMslLockInd;
	int32_t bPlayerStats;
	int32_t nWindowPos;
	int32_t nWindowSize;
	int32_t nWindowZoom;
	int32_t nRadarPos;
	int32_t nRadarSize;
	int32_t nRadarRange;
	int32_t nRadarColor;
	int32_t nRadarStyle;
	int32_t nColorScheme;
	int32_t nMinimalistWidth;
	int32_t nMinimalistHeight;
} tCockpitOptions;

//------------------------------------------------------------------------------

typedef struct tTextureOptions {
	int32_t bUseHires [2];
	int32_t nQuality;
} tTextureOptions;

//------------------------------------------------------------------------------

typedef struct tParticleOptions {
	int32_t nQuality;
	int32_t nDens [5];
	int32_t nSize [5];
	int32_t nLife [5];
	int32_t nAlpha [5];
	int32_t bSyncSizes;
	int32_t bPlayers;
	int32_t bRobots;
	int32_t bMissiles;
	int32_t bPlasmaTrails;
	int32_t bDebris;
	int32_t bStatic;
	int32_t bBubbles;
	int32_t bWobbleBubbles;
	int32_t bWiggleBubbles;
	int32_t bCollisions;
	int32_t bDisperse;
	int32_t bRotate;
	int32_t bSort;
	int32_t bDecreaseLag;	//only render if player is moving forward
	int32_t bAuxViews;
	int32_t bMonitors;
} tParticleOptions;

//------------------------------------------------------------------------------

typedef struct tLightningOptions {
	int32_t nQuality;
	int32_t nStyle;
	int32_t bGlow;
	int32_t bPlayers;
	int32_t bRobots;
	int32_t bDamage;
	int32_t bExplosions;
	int32_t bStatic;
	int32_t bOmega;
	int32_t bRobotOmega;
	int32_t bAuxViews;
	int32_t bMonitors;
} tLightningOptions;

//------------------------------------------------------------------------------

typedef struct tShadowOptions {
	int32_t nReach;
	int32_t nLights;
	int32_t bFast;
	int32_t nClip;
	int32_t bSoft;
	int32_t bPlayers;
	int32_t bRobots;
	int32_t bMissiles;
	int32_t bPowerups;
	int32_t bReactors;
	} tShadowOptions;

//------------------------------------------------------------------------------

typedef struct tPowerupOptions {
	int32_t b3D;
	int32_t b3DShields;
	int32_t nSpin;
} tPowerupOptions;

//------------------------------------------------------------------------------

typedef struct tAutomapOptions {
	int32_t bTextured;
	int32_t bBright;
	int32_t bCoronas;
	int32_t bParticles;
	int32_t bSparks;
	int32_t bLightning;
	int32_t bGrayOut;
	int32_t bSkybox;
	int32_t nColor;
	//int32_t nRange;
} tAutomapOptions;

//------------------------------------------------------------------------------

typedef struct tShipRenderOptions {
	int32_t nWingtip;
	int32_t bBullets;
	int32_t nColor;
} tShipRenderOptions;

//------------------------------------------------------------------------------

typedef struct tCoronaRenderOptions {
	int32_t bUse;
	int32_t nStyle;
	int32_t bShots;
	int32_t bWeapons;
	int32_t bPowerups;
	int32_t bAdditive; //additive corona blending for wall lights 
	int32_t bAdditiveObjs; //additive corona blending for light emitting weapons
	int32_t nIntensity;
	int32_t nObjIntensity;
} tCoronaRenderOptions;

//------------------------------------------------------------------------------

#define SOFT_BLEND_SPRITES		1
#define SOFT_BLEND_PARTICLES	2

typedef struct tEffectRenderOptions {
	int32_t bEnabled;
	int32_t nShockwaves;
	int32_t nShrapnels;
	int32_t bEnergySparks;
	int32_t bShields;
	int32_t bOnlyShieldHits;
	int32_t bAutoTransparency;
	int32_t bTransparent;
	int32_t bSoftParticles;
	int32_t bMovingSparks;
	int32_t bGlow;
	int32_t bWarpAppearance;
} tEffectRenderOptions;

//------------------------------------------------------------------------------

typedef struct tDebugRenderOptions {
	int32_t bDynamicLight;
	int32_t bObjects;
	int32_t bTextures;
	int32_t bWalls;
	int32_t bWireFrame;
} tDebugRenderOptions;

//------------------------------------------------------------------------------

#define GLASSES_NONE						0
#define GLASSES_AMBER_BLUE				1
#define GLASSES_RED_CYAN				2
#define GLASSES_GREEN_MAGENTA			3
#define DEVICE_STEREO_PHYSICAL		4
#define DEVICE_STEREO_SIDEBYSIDE		4
#define GLASSES_SHUTTER_HDMI			4
#define GLASSES_OCULUS_RIFT			5
#define DEVICE_STEREO_DOUBLE_BUFFER	6
#define GLASSES_SHUTTER_NVIDIA		6

#define STEREO_PARALLEL					0
#define STEREO_TOE_IN					1

typedef struct tStereoRenderOptions {
	int32_t nGlasses;
	int32_t nMethod;
	int32_t nScreenDist;
	int32_t bEnhance;
	int32_t bColorGain;
	int32_t bDeghost;
	int32_t bFlipFrames;
	int32_t bBrighten;
	int32_t bChromAbCorr;
	int32_t nRiftFOV;
	fix xSeparation [2];
} tStereoRenderOptions;

class CRenderOptions {
	public:
		int32_t bAllSegs;
		int32_t nLightingMethod;
		int32_t bHiresModels [2];
		int32_t nMeshQuality;
		int32_t bUseLightmaps;
		int32_t nLightmapQuality;
		int32_t nLightmapPrecision;
		int32_t bUseShaders;
		int32_t bUseRift;
		int32_t nMathFormat;
		int32_t nDefMathFormat;
		int16_t nMaxFPS;
		int32_t nPath;
		int32_t nQuality;
		int32_t nImageQuality;
		int32_t nDebrisLife;
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
		void Init (int32_t i = 0);
		int32_t ShadowQuality (void);
	};

//------------------------------------------------------------------------------

class COglOptions {
	public:
		int32_t bGlTexMerge;
		int32_t bLightObjects;
		int32_t bLightPowerups;
		int32_t bObjLighting;
		int32_t bHeadlight;
		int32_t nMaxLightsPerFace;
		int32_t nMaxLightsPerPass;
		int32_t nMaxLightsPerObject;

	public:
		COglOptions () { Init (); }
		void Init (int32_t i = 0);
	};

//------------------------------------------------------------------------------

class CMovieOptions {
	public:
		int32_t bHires;
		int32_t nQuality;
		int32_t nLevel;
		int32_t bResize;
		int32_t bFullScreen;
		int32_t bSubTitles;

	public:
		CMovieOptions () { Init (); }
		void Init (int32_t i = 0);
	};

//------------------------------------------------------------------------------

class CGameplayOptions {
	public:
		int32_t nAutoSelectWeapon;
		int32_t bSecretSave;
		int32_t bTurboMode;
		int32_t bFastRespawn;
		int32_t nAutoLeveling;
		int32_t bEscortHotKeys;
		int32_t bSkipBriefingScreens;
		int32_t bHeadlightOnWhenPickedUp;
		int32_t bShieldWarning;
		int32_t bInventory;
		int32_t bIdleAnims;
		int32_t nAIAwareness;
		int32_t nAIAggressivity;
		int32_t nSlowMotionSpeedup;
		int32_t bUseD1AI;
		int32_t bNoThief;
		int32_t bObserve;
		int32_t nShip [2];

	public:
		CGameplayOptions () { Init (); }
		void Init (int32_t i = 0);
	};

//------------------------------------------------------------------------------

#define UNIQUE_JOY_AXES	5

typedef struct tMouseInputOptions {
	int32_t bUse;
	int32_t bSyncAxis;
	int32_t bJoystick;
	int32_t nDeadzone;
	int32_t sensitivity [3];
	} tMouseInputOptions;

//------------------------------------------------------------------------------

typedef struct tJoystickInputOptions {
	int32_t bUse;
	int32_t bSyncAxis;
	int32_t bLinearSens;
	int32_t sensitivity [UNIQUE_JOY_AXES];
	int32_t deadzones [UNIQUE_JOY_AXES];
	} tJoystickInputOptions;

//------------------------------------------------------------------------------

typedef struct tOculusRiftInputOptions {
	int32_t nDeadzone;
	} tOculusRiftInputOptions;

//------------------------------------------------------------------------------

typedef struct tTrackIRInputOptions {
	int32_t bPresent;
	int32_t bUse;
	int32_t nMode;
	int32_t bMove [5];
	int32_t nDeadzone;
	int32_t sensitivity [3];
	int32_t bSyncAxis;
	} tTrackIRInputOptions;

//------------------------------------------------------------------------------

typedef struct tKeyboardInputOptions {
	int32_t nType;
	int32_t bUse;
	int32_t nRamp;
	int32_t bRamp [3];
	} tKeyboardInputOptions;

//------------------------------------------------------------------------------

class CInputOptions {
	public:
		int32_t bLimitTurnRate;
		int32_t nMinTurnRate;
		int32_t bUseHotKeys;
		tMouseInputOptions mouse;
		tJoystickInputOptions joystick;
		tOculusRiftInputOptions oculusRift;
		tTrackIRInputOptions trackIR;
		tKeyboardInputOptions keyboard;

	public:
		CInputOptions () { Init (); }
		void Init (int32_t i = 0);
	};

//------------------------------------------------------------------------------

class CSoundOptions {
	public:
		int32_t bUseD1Sounds;
		int32_t bUseRedbook;
		int32_t bHires [2];
		int32_t bUseSDLMixer;
		int32_t bUseOpenAL;
		int32_t bFadeMusic;
		int32_t bShuffleMusic;
		int32_t bLinkVolumes;
		int32_t audioSampleRate;	// what's used by the audio system
		int32_t soundSampleRate;	// what the default sounds are in
		int32_t bShip;
		int32_t bMissiles;
		int32_t bGatling;
		int32_t bSpeedUp;
		fix xCustomSoundVolume;

	public:
		CSoundOptions () { Init (); }
		void Init (int32_t i = 0);
	};

//------------------------------------------------------------------------------

typedef struct tAltBgOptions {
	int32_t bHave;
	double alpha;
	double brightness;
	int32_t grayscale;
	char szName [2][FILENAME_LEN];
} tAltBgOptions;

//------------------------------------------------------------------------------

class CMenuOptions {
	public:
		int32_t	nStyle;
		int32_t	bFastMenus;
		uint32_t	nFade;
		int32_t	bSmartFileSearch;
		int32_t	bShowLevelVersion;
		char		nHotKeys;
		tAltBgOptions altBg;

	public:
		CMenuOptions () { Init (); }
		void Init (int32_t i = 0);
	};

//------------------------------------------------------------------------------

class CDemoOptions {
	public:
		int32_t bOldFormat;
		int32_t bRevertFormat;

	public:
		CDemoOptions () { Init (); }
		void Init (int32_t i = 0);
	};

//------------------------------------------------------------------------------

class CMultiplayerOptions {
	public:
		int32_t bNoRankings;
		int32_t bTimeoutPlayers;
		int32_t bUseMacros;
		int32_t bNoRedundancy;

	public:
		CMultiplayerOptions () { Init (); }
		void Init (int32_t i = 0);
	};

//------------------------------------------------------------------------------

class CApplicationOptions {
	public:
		int32_t bAutoRunMission;
		int32_t nVersionFilter;
		int32_t bSinglePlayer;
		int32_t bEnableMods;
		int32_t bExpertMode;
		int32_t bEpilepticFriendly;
		int32_t bColorblindFriendly;
		int32_t bNotebookFriendly;
		int32_t nScreenShotInterval;

	public:
		CApplicationOptions () { Init (); }
		void Init (int32_t i = 0);
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
		void Init (int32_t i = 0);
		bool Use3DPowerups (void);
		int32_t UseHiresSound (void);
		inline int32_t SoftBlend (int32_t nFlag) { return (render.effects.bSoftParticles & nFlag) != 0; }
};

//------------------------------------------------------------------------------
// states
//------------------------------------------------------------------------------

class CSeismicStates {
	public:
		fix		nMagnitude;
		fix		nStartTime;
		fix		nEndTime;
		fix		nNextSoundTime;
		int32_t	nLevel;
		int32_t	nShakeFrequency;
		int32_t	nShakeDuration;
		int32_t	nSound;
		int32_t	bSound;
		int32_t	nVolume;
	};

//------------------------------------------------------------------------------

class CSlowMotionStates {
	public:
		float			fSpeed;
		int32_t		nState;
		time_t		tUpdate;
		int32_t		bActive;
	};

//------------------------------------------------------------------------------

class CGameplayStates {
	public:	
		int32_t				bMultiBosses;
		int32_t				bFinalBossIsDead;
		int32_t				bHaveSmartMines;
		int32_t				bMineDestroyed;
		int32_t				bKillBossCheat;
		int32_t				bTagFlag;
		int32_t				nReactorCount [2];
		int32_t				nLastReactor;
		int32_t				bMineMineCheat;
		int32_t				bAfterburnerCheat;
		int32_t				bTripleFusion;
		int32_t				bInLevelTeleport;
		int32_t				bLastAfterburnerState;
		fix					xLastAfterburnerCharge;
		fix					nPlayerSpeed;
		fix					xInitialShield [2];
		fix					xInitialEnergy [2];
		CFixVector			vTgtDir;
		int32_t				nDirSteps;
		int32_t				nInitialLives;
		CSeismicStates		seismic;
		CSlowMotionStates	slowmo [2];

	public:
		fix InitialShield (void) {
			int32_t h = xInitialShield [1];
			if (h <= 0)
				return xInitialShield [0];
			xInitialShield [1] = -1;
			return h;
			}

		fix InitialEnergy (void) {
			int32_t h = xInitialEnergy [1];
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
		uint8_t 		nBufferType;		// 0=No buffer, 1=buffer ASCII, 2=buffer scans
		uint8_t 		bRepeat;
		uint8_t 		bEditorMode;
		uint8_t		nLastPressed;
		uint8_t		nLastReleased;
		uint8_t		pressed [256];
		int32_t		xLastPressTime;
		};

class CInputStates {
	public:
		int32_t		nPlrFileVersion;
		int32_t		nMouseType;
		int32_t		nJoyType;
		int32_t		nJoysticks;
		int32_t		bGrabMouse;
		int32_t		bHaveTrackIR;
		uint8_t		bCybermouseActive;
		int32_t		bSkipControls;
		int32_t		bControlsSkipFrame;
		int32_t		bKeepSlackTime;
		time_t		kcPollTime;
		float			kcFrameTime;
		fix			nCruiseSpeed;
		CKeyStates	keys;
	};

//------------------------------------------------------------------------------

class CMenuStates {
	public:
		int32_t bHires;
		int32_t bHiresAvailable;
		int32_t nInMenu;
		int32_t bInitBG;
		int32_t bDrawCopyright;
		int32_t bFullScreen;
		int32_t bReordering;
		int32_t bNoBackground;
	};

//------------------------------------------------------------------------------

class CMovieStates {
	public:
		int32_t bIntroPlayed;
	};

//------------------------------------------------------------------------------

#include "player.h"

class CMultiplayerStates {
	public:
		int32_t	bUseTracker;
		int32_t	bTrackerCall;
		int32_t	bServer [2];
		int32_t	bKeepClients;
		int32_t	bHaveLocalAddress;
		int32_t	nGameType;
		int32_t	nGameSubType;
		int32_t	bTryAutoDL;
		int32_t	nConnection;
		int32_t	bIWasKicked;
		int32_t	bCheckPorts;
		uint8_t	bSurfingNet;
		uint8_t	bSuicide;
		int32_t	bPlayerIsTyping [MAX_PLAYERS];
	};

//------------------------------------------------------------------------------

class CGfxStates {
	public:
		int32_t bInstalled;
		int32_t bOverride;
		int32_t nStartScrSize;
		int32_t nStartScrMode;
	};

//------------------------------------------------------------------------------

class CCameraStates {
	public:
		int32_t bActive;
	};

//------------------------------------------------------------------------------

class CTextureStates {
	public:
		int32_t bGlTexMergeOk;
		int32_t bHaveMaskShader;
		int32_t bHaveGrayScaleShader;
		int32_t bHaveEnhanced3DShader;
		int32_t bHaveRiftWarpShader;
		int32_t bHaveShadowMapShader;
	};

//------------------------------------------------------------------------------

class CCockpitStates {
	public:
		int32_t bShowPingStats;
		int32_t nType;
		int32_t nNextType;
		int32_t nTypeSave;
		int32_t nShieldFlash;
		int32_t bRedraw;
		int32_t bBigWindowSwitch;
		int32_t nLastDrawn [2];
		int32_t n3DView [2];
		int32_t nCoopPlayerView [2];
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
		uint8_t			nScreenFlags;	//see values in screens.h
		uint8_t			nCurrentPage;
		fix			xEyeWidth;
		int32_t			nRenderMode;
		int32_t			nLowRes;			// Default to low res
		int32_t 			bShowHUD;
		int32_t			nSensitivity;	// 0 - 2
		int32_t			xStereoSeparation;
		int32_t			nEyeSwitch;
		int32_t			bEyeOffsetChanged;
		int32_t			bUseRegCode;
		CVRBuffers	buffers;
	};
#endif

//------------------------------------------------------------------------------

class CFontStates {
	public:
		int32_t bHires;
		int32_t bHiresAvailable;
		int32_t bInstalled;
	};

//------------------------------------------------------------------------------

#define DEFAULT_RENDER_DEPTH	255
#define DEFAULT_ZOOM				0x9000
#define RIFT_DEFAULT_ZOOM		1.5

typedef struct tRenderDetail {
	int32_t nLevel;
	int32_t nRenderDepth;
	int32_t nMaxDebrisObjects;
	int32_t nMaxObjectsOnScreenDetailed;
	int32_t nMaxLinearDepthObjects;
	int32_t nMaxPerspectiveDepth;
	int32_t nMaxLinearDepth;
	int32_t nMaxFlatDepth;
	int8_t nObjectComplexity;
	int8_t nObjectDetail;
	int8_t nWallDetail;
	int8_t nWallRenderDepth;
	int8_t nDebrisAmount; 
	int8_t nSoundChannels;
} tRenderDetail;

typedef struct tRenderHistory {
	CBitmap*		bmBot;
	CBitmap*		bmTop;
	CBitmap*		bmMask;
	uint8_t		bSuperTransp;
	uint8_t		bShaderMerge;
	int32_t		bOverlay;
	int32_t		bColored;
	int32_t		nType;
	int32_t		nBlendMode;
} tRenderHistory;

class CZoomStates {
	public:
		int32_t		nState;
		float			nFactor;
		fix			nMinFactor;
		fix			nMaxFactor;
		fix			nDestFactor;
		int32_t		nChannel;
		float			nStep;
		time_t		nTime;
	};

class CRenderStates {
	public:
		int32_t bChaseCam;
		int32_t bFreeCam;
		int32_t bEnableFreeCam;
		int32_t bObserving;
		int32_t bQueryOcclusion;
		int32_t bVertexArrays;
		int32_t bAutoMap;
		int32_t bLightmapsOk;
		int32_t bBuildLightmaps;
		int32_t bHaveLightmaps;
		int32_t bUseLightmaps;
		int32_t bDropAfterburnerBlob;
		int32_t bOutsideMine;
		int32_t bExtExplPlaying;
		int32_t bDoAppearanceEffect;
		int32_t bGlLighting;
		int32_t bColored;
		int32_t bBriefing;
		int32_t bRearView;
		int32_t bDepthSort;
		int32_t nInterpolationMethod;
		int32_t bTMapFlat;
		int32_t bCloaked;
		int32_t bBrightObject;
		int32_t nWindow [2];
		int32_t xStereoSeparation [2];
		int32_t nStartSeg;
		int32_t nLighting;
		int32_t nMaxTextureQuality;
		int32_t bTransparency;
		int32_t bSplitPolys;
		int32_t bHaveOculusRift;
		int32_t bHaveDynLights;
		int32_t bHaveSparks;
		int32_t bUsePerPixelLighting;
		int32_t nRenderPass;
		int32_t nShadowPass;
		int32_t nShadowBlurPass;
		int32_t bBlurPass;
		int32_t nShadowMap;
		int32_t bLoResShadows;
		int32_t bUseCameras;
		int32_t bUseDynLight;
		int32_t bApplyDynLight;
		int32_t bClusterLights;
		int32_t nSoften;
		int32_t bHeadlightOn;
		int32_t bHeadlights;
		int32_t bHaveSkyBox;
		int32_t bAllVisited;
		int32_t bViewDist;
		int32_t bD2XLights;
		int32_t bRendering;
		int32_t bFullBright;
		int32_t bQueryCoronas;
		int32_t bDoLightmaps;
		int32_t bAmbientColor;
		int32_t bSpecularColor;
		int32_t bDoCameras;
		int32_t bRenderIndirect;
		int32_t bBuildModels;
		int32_t bShowFrameRate;
		int32_t bShowTime;
		int32_t bShowProfiler;
		int32_t bTriangleMesh;
		int32_t bOmegaModded;
		int32_t bPlasmaModded;
		int32_t nFrameFlipFlop;
		int32_t nModelQuality;
		int32_t nMeshQuality;
		int32_t nState;	//0: render geometry, 1: render objects
		int32_t nType;
		int32_t nFrameCount;
		int32_t bUpdateEffects;
		int32_t nLightingMethod;
		int32_t bPerPixelLighting;
		int32_t nMaxLightsPerPass;
		int32_t nMaxLightsPerFace;
		int32_t nMaxLightsPerObject;
		int32_t bVSync;
		int32_t bVSyncOk;
		int32_t nThreads;
		int32_t xPlaneDistTolerance;
		fix xZoom;
		fix xZoomScale;
		uint8_t nRenderingType;
		fix nFlashScale;
		fix nFlashRate;
		CCameraStates cameras;
		CCockpitStates cockpit;
		//CVRStates vr;
		CFontStates fonts;
		CTextureStates textures;
		int32_t bDetriangulation;
		GLenum nFacePrimitive;
		double glFOV;
		double glAspect;
		float grAlpha;
		tRenderDetail detail;
		tRenderHistory history;

		inline void SetRenderWindow (int32_t w) {
			nWindow [1] = nWindow [0];
			nWindow [0] = w;
			}

		inline void SetStereoSeparation (fix s) {
			xStereoSeparation [1] = xStereoSeparation [0];
			xStereoSeparation [0] = s;
			}

		inline bool Dirty (void) { return (nWindow [0] != nWindow [1]) || (xStereoSeparation [0] != xStereoSeparation [1]); }
	};

//------------------------------------------------------------------------------

class CAudioStates {
	public:
		int32_t bNoMusic;
		int32_t bSoundsInitialized;
		int32_t bLoMem;
		int32_t nNextSignature;
		int32_t nActiveObjects;
		int32_t nMaxChannels;
	};

//------------------------------------------------------------------------------

class CSoundStates {
	public:
		int32_t bWasRecording;
		int32_t bDontStartObjects;
		int32_t nConquerWarningSoundChannel;
		int32_t nSoundChannels;
		int32_t bD1Sound;
		int32_t bMidiFix;
		int32_t bDynamic;
		CAudioStates audio;
	};

//------------------------------------------------------------------------------

class CVideoStates {
	public:
		int32_t nDisplayMode;
		int32_t nDefaultDisplayMode;
		uint32_t nScreenMode;
		uint32_t nLastScreenMode;
		int32_t nWidth;
		int32_t nHeight;
		int32_t bFullScreen;
	};

//------------------------------------------------------------------------------

class CCheatStates {
	public:
		int32_t bEnabled;
		int32_t bTurboMode;
		int32_t bMonsterMode;
		int32_t bLaserRapidFire;
		int32_t bRobotsFiring;
		int32_t bRobotsKillRobots;
		int32_t bJohnHeadOn;
		int32_t bHomingWeapons;
		int32_t bBouncingWeapons;
		int32_t bMadBuddy;
		int32_t bAcid;
		int32_t bPhysics;
		int32_t bSpeed;
		int32_t bD1CheatsEnabled;
		int32_t nUnlockLevel;
	};

//------------------------------------------------------------------------------

class CEntropyStates {
	public:
		int32_t bConquering;
		int32_t bConquerWarning;
		int32_t bExitSequence;
		int32_t nTimeLastMoved;
	};

//------------------------------------------------------------------------------

typedef struct tSlowTick {
	int32_t bTick;
	int32_t nTime;
	int32_t nSlack;
	int32_t nLastTick;
} tSlowTick;

class CApplicationStates {
	public:
		tSlowTick	tick40fps;
		tSlowTick	tick60fps;
	#if 1 //MULTI_THREADED
		int32_t		bExit;
		int32_t		bMultiThreaded;
		int32_t		nThreads;
	#endif
		int32_t		bDemoData;
		int32_t		bCheckAndFixSetup;
	#ifdef __unix__
		int32_t	bLinuxMsgBox;
	#endif
		int32_t		nSDLTicks [2];
		int32_t		nExtGameStatus;
		int32_t		nFunctionMode;
		int32_t		nLastFuncMode;
		int32_t		nCriticalError;
		int32_t		bStandalone;
		int32_t		bNostalgia;
		int32_t		iNostalgia;
		int32_t		bInitialized;
		int32_t		bD2XLevel;
		int32_t		bEnterGame;
		int32_t		bSaveScreenShot;
		int32_t		bShowVersionInfo;
		int32_t		bGameRunning;
		int32_t		bGameSuspended;
		int32_t		bGameAborted;
		int32_t		bBetweenLevels;
		int32_t		bPlayerIsDead;
		int32_t		bPlayerEggsDropped;
		int32_t		bDeathSequenceAborted;
		int32_t		bChangingShip;
		int32_t		bPlayerFiredLaserThisFrame;
		int32_t		bUseSound;
		int32_t		bMacData;
		int32_t		bCompressData;
		int32_t		bLunacy;
		int32_t		bEnglish;
		int32_t		bD1Data;
		int32_t		bD1Model;
		int32_t		bD1Mission;
		int32_t		bHaveD1Data;
		int32_t		bHaveD1Textures;
		int32_t		bHaveMod;
		int32_t		bEndLevelDataLoaded;
		int32_t		bEndLevelSequence;
		int32_t		bFirstSecretVisit;
		int32_t		bHaveExtraGameInfo [2];
		int32_t		bConfigMenu;
		int32_t		nDifficultyLevel;
		int32_t		nDetailLevel;
		int32_t		nBaseCtrlCenExplTime;
		int32_t		bUseDefaults;
		int32_t		nCompSpeed;
		int32_t		bHaveExtraData;
		int32_t		bHaveExtraMovies;
		int32_t		bDebugSpew;
		int32_t		bAutoRunMission;
		int32_t		bProgressBars;
		int32_t		bComputeLightmaps;
		int32_t		bLittleEndian;
		int32_t		bUsingConverter;
		int32_t		bFixModels;
		int32_t		bAltModels;
		int32_t		bReadOnly;
		int32_t		bCacheTextures;
		int32_t		bCacheLights;
		int32_t		bCacheMeshes;
		int32_t		bCacheLightmaps;
		int32_t		bCacheModelData;
		int32_t		bUseSwapFile;
		int32_t		bSingleStep;
		int32_t		bAutoDemos;	//automatically play demos or intro movie if user is idling in the main menu
		int32_t		bShowError;
		int32_t		bClearMessage;
		int32_t		nLogLevel;
		int32_t		iDownloadTimeout;
		int32_t		bHaveSDLNet;
		int32_t		nSpawnPos;
		int16_t		nPlayerSegment;
		uint32_t		nRandSeed;
		bool			bCustomData;
		bool			bCustomSounds;
		fix			xThisLevelTime;
		fix			nPlayerTimeOfDeath;
		char*			szCurrentMission;
		char*			szCurrentMissionFile;
		tObjTransformation playerPos;
		CCheatStates cheats;

		inline void SRand (uint32_t seed = 0xffffffff) {
			if (seed == 0xffffffff) {
				seed = SDL_GetTicks ();
				seed *= seed;
				seed ^= (uint32_t) time (NULL);
				}
			srand (nRandSeed = seed);
			}

	};

//------------------------------------------------------------------------------

class CLimitFPSStates {
	public:
		uint8_t	bControls;
		uint8_t	bJoystick;
		uint8_t	bSeismic;
		uint8_t	bCountDown;
		uint8_t	bHomers;
		uint8_t	bFusion;
		uint8_t	bOmega;
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
		int32_t	m_shipsAllowed [MAX_SHIP_TYPES];
		int32_t	m_playerShip;
		int32_t	m_bTeleport;
		int32_t	m_bSecretSave;
		int32_t	m_bColoredSegments;
		int32_t	m_b3DPowerups;
		int32_t	m_nCollisionModel;

	public:
		CMissionConfig () { Init (); }
		void Init (void);
		int32_t Load (char* szFilename = NULL);
		void Apply (void);
		int32_t SelectShip (int32_t nShip);
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
		uint32_t		m_refCount;
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
		int16_t				nMap;
		uint8_t				nFrame;	//set per frame when scene as seen from a light source has been rendered
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
	int16_t			nSegment;
	int16_t			nSide;
	int16_t			nTexture;
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
		int32_t						nVisibleLights;
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
		int16_t		m_nSegment;
		int16_t		m_nSide;
		uint32_t		m_mask;     // determines flicker pattern
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
		int32_t								nStatic;
		int32_t								nCoronas;
		CArray<fix>						segDeltas;
		CArray<CLightDeltaIndex>	deltaIndices;
		CArray<CLightDelta>			deltas;
		CArray<uint8_t>					subtracted;
		CFlickerLightData				flicker;
		CArray<fix>						dynamicLight;
		CArray<CFloatVector3>			dynamicColor;
		CArray<uint8_t>					bGotDynColor;
		uint8_t								bGotGlobalDynColor;
		uint8_t								bStartDynColoring;
		uint8_t								bInitDynColoring;
		CFloatVector3						globalDynColor;
		CArray<int16_t>					vertices;
		CArray<int8_t>					vertexFlags;
		CArray<int8_t>					newObjects;
		CArray<int8_t>					objects;
		CArray<GLuint>					coronaQueries;
		CArray<GLuint>					coronaSamples;
	public:
		CLightData ();
		void Init (void);
		bool Create (void);
		bool Resize (void);
		void Destroy (void);
};

inline int32_t operator- (CLightDeltaIndex* o, CArray<CLightDeltaIndex>& a) { return a.Index (o); }
inline int32_t operator- (CLightDelta* o, CArray<CLightDelta>& a) { return a.Index (o); }

//------------------------------------------------------------------------------

class CShadowData {
	public:
		int16_t					nLight;
		int16_t					nLights;
		int16_t					nShadowMaps;
		CDynLight*			lightP;
		CObject				lightSource;
		CFloatVector		vLightPos;
		CFixVector			vLightDir [MAX_SHADOW_LIGHTS];
		CArray<int16_t>		objLights;
		uint8_t					nFrame;	//flipflop for testing whether a light source's view has been rendered the current frame
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
		CArray<uint8_t>			heightmap;
		CArray<fix>				lightmap;
		CArray<CFixVector>	points;
		CBitmap*					bmP;
		CStaticArray< CRenderPoint, TERRAIN_GRID_MAX_SIZE >		saveRow; // [TERRAIN_GRID_MAX_SIZE];
		CFixVector				vStartPoint;
		tUVL						uvlList [2][3];
		int32_t						bOutline;
		int32_t						nGridW, nGridH;
		int32_t						orgI, orgJ;
		int32_t						nMineTilesDrawn;    //flags to tell if all 4 tiles under mine have drawn
	public:
		CTerrainRenderData ();
};

//------------------------------------------------------------------------------

#include "flightpath.h"

class CThrusterData {
	public:
		CFlightPath		path;
		float				fSpeed;
		int16_t				nPulse;
		time_t			tPulse;
	public:
		CThrusterData ();
};

//------------------------------------------------------------------------------

#define OBJS_PER_SEG        5
#define N_EXTRA_OBJ_LISTS   50

typedef struct tObjRenderListItem {
	int16_t	nNextItem;
	int16_t	nObject;
	fix	xDist;
} tObjRenderListItem;

class CObjRenderList {
	public:
		CStaticArray< int16_t, MAX_SEGMENTS_D2X >	ref; // [MAX_SEGMENTS_D2X];	//points to each segment's first render object list entry in renderObjs
		CStaticArray< tObjRenderListItem, MAX_OBJECTS_D2X >	objs; // [MAX_OBJECTS_D2X];
		int32_t						nUsed;
	};


typedef struct tSegZRef {
	fix	z;
	int16_t	nSegment;
} tSegZRef;

typedef struct tPortal {
	fix	left, right, top, bot;
	char  bProjected;
	uint8_t bVisible;
} tPortal;

class CVisibilityData {
	public:
		int32_t					nSegments;
		CShortArray				segments; //[MAX_SEGMENTS_D2X];
		CByteArray				bVisited; //[MAX_SEGMENTS_D2X];
		CByteArray				bVisible; //[MAX_SEGMENTS_D2X];
		CByteArray				bProcessed; //[MAX_SEGMENTS_D2X];		//whether each entry has been nProcessed
		uint8_t 					nVisited;
		uint8_t					nProcessed;
		uint8_t					nVisible;
		CShortArray				nDepth; //[MAX_SEGMENTS_D2X];		//depth for each seg in nRenderList
		CArray<tSegZRef>		zRef [2]; // segment indexes sorted by distance from viewer
		CArray<tPortal>		portals;
		CShortArray				position; //[MAX_SEGMENTS_D2X];
		CArray<CRenderPoint>	points;

	public:
		CVisibilityData ();
		~CVisibilityData () { Destroy (); }
		bool Create (int32_t nState);
		bool Resize (int32_t nLength = -1);
		void Destroy (void);

		inline bool Visible (int16_t nSegment) { return bVisible [nSegment] == nVisible; }
		inline bool Visited (int16_t nSegment) { return bVisited [nSegment] == nVisited; }
		inline void Visit (int16_t nSegment) { bVisited [nSegment] = nVisited; }
		uint8_t BumpVisitedFlag (void);
		uint8_t BumpProcessedFlag (void);
		uint8_t BumpVisibleFlag (void);
		int32_t SegmentMayBeVisible (int16_t nStartSeg, int32_t nRadius, int32_t nMaxDist);
		void BuildSegList (CTransformation& transformation, int16_t nStartSeg, int32_t nWindow, bool bIgnoreDoors = false);
		void InitZRef (int32_t i, int32_t j, int32_t nThread);
		void QSortZRef (int16_t left, int16_t right);

	private:
		int32_t BuildAutomapSegList (void);
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
		int32_t					nObjRenderSegs;
		CObjRenderList			objRenderList;
		CArray< CSegFace* >	renderFaceListP; //[MAX_SEGMENTS_D2X * 6];
		CIntArray				bObjectRendered; //[MAX_OBJECTS_D2X];
		CByteArray				bRenderSegment; //[MAX_SEGMENTS_D2X];
		CShortArray				nRenderObjList; //[MAX_SEGMENTS_D2X+N_EXTRA_OBJ_LISTS][OBJS_PER_SEG];
		CByteArray				bCalcVertexColor; //[MAX_VERTICES_D2X];
		CUShortArray			bAutomapVisited; //[MAX_SEGMENTS_D2X];
		CUShortArray			bAutomapVisible; //[MAX_SEGMENTS_D2X];
		CUShortArray			bRadarVisited; //[MAX_SEGMENTS_D2X];
		uint8_t					bSetAutomapVisited;

	public:
		CMineRenderData ();
		~CMineRenderData () { Destroy (); }
		bool Create (int32_t nState);
		bool Resize (int32_t nLength = -1);
		void Destroy (void);
		bool Visible (int16_t nSegment, int32_t nThread = 0) { return visibility [nThread].Visible (nSegment); }
		bool Visited (int16_t nSegment, int32_t nThread = 0) { return visibility [nThread].Visited (nSegment); }
		void Visit (int16_t nSegment, int32_t nThread = 0) { return visibility [nThread].Visit (nSegment); }
		CByteArray& VisibleFlags (int32_t nThread = 0) { return visibility [nThread].bVisible; }
		CByteArray& VisitedFlags (int32_t nThread = 0) { return visibility [nThread].bVisited; }
};

//------------------------------------------------------------------------------

class CVertColorData {
	public:
		int32_t				bExclusive;
		int32_t				bNoShadow;
		int32_t				bDarkness;
		int32_t				bMatEmissive;
		int32_t				bMatSpecular;
		int32_t				nMatLight;
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
	int32_t					nNextItem;
} tFaceListItem;

class CFaceListIndex {
	public:
		CIntArray				roots; // [MAX_WALL_TEXTURES * 3];
		CIntArray				tails; // [MAX_WALL_TEXTURES * 3];
		CIntArray				usedKeys; // [MAX_WALL_TEXTURES * 3];
		//int32_t						nUsedFaces;
		int32_t						nUsedKeys;
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
#	include "timeout.h"
#endif

class CRiftData {
	public:
#if OCULUS_RIFT
		OVR::Ptr<OVR::DeviceManager>			m_managerP;
		OVR::Ptr<OVR::HMDDevice>				m_hmdP;
		OVR::Ptr<OVR::SensorDevice>			m_sensorP;
		OVR::HMDInfo								m_hmdInfo;
		OVR::SensorFusion*						m_sensorFusion;
		OVR::Util::Render::StereoConfig		m_stereoConfig;
		OVR::Util::Render::StereoEyeParams	m_eyes [2];
#	if 0 // manual calibration removed from Rift SDK since v0.25
		OVR::Util::MagCalibration				m_magCal;
		CTimeout			m_magCalTO;
		bool				m_bCalibrating;
#	endif
#endif

		float				m_renderScale;
		float				m_fov;
		float				m_projectionCenterOffset;
		int32_t				m_ipd;
		int32_t				m_nResolution;
		int32_t				m_bUse;
		int32_t				m_bAvailable;
		CFloatVector	m_center;

		CRiftData () : m_renderScale (1.0f), m_fov (125.0f), m_projectionCenterOffset (0.0f), m_ipd (0), m_nResolution (0), m_bUse (0), m_bAvailable (false) {}
		~CRiftData () { Destroy (); }
		bool Create (void);
		void Destroy (void);
		inline int32_t Available (void) { return m_bAvailable; }
		inline int32_t Resolution (void) { return m_nResolution; }
		int32_t GetViewMatrix (CFixMatrix& m);
		int32_t GetHeadAngles (CAngleVector* angles);
		void AutoCalibrate (void);
		inline void SetCenter (void) { GetHeadAngles (NULL); }
#if OCULUS_RIFT
		inline int32_t HResolution (void) { return (Available () &&  m_hmdInfo.HResolution) ? m_hmdInfo.HResolution : 1920; }
		inline int32_t VResolution (void) { return (Available () && m_hmdInfo.VResolution) ? m_hmdInfo.VResolution : 1200; }
#else
		inline int32_t HResolution (void) { return 1920; }
		inline int32_t VResolution (void) { return 1200; }
#endif
	};

#define STEREO_OFFSET_NONE			0
#define STEREO_OFFSET_FIXED		1
#define STEREO_OFFSET_FLOATING	2

class CRenderData {
	public:
		CRiftData					rift;
		CColorData					color;
		int32_t							transpColor;
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
		CScreen						screen; // entire screen
		CGameScreenData			frame;  // current part of the screen being rendered too (usually entire screen or left and right halves of it)
		CGameScreenData			scene; // part of the frame being rendered to (can be smaller for certain cockpit types)
		CGameScreenData			window; // cockpit windows (rear view, guide bot, etc)
		fix							zMin;
		fix							zMax;
		double						dAspect;
		CFBO							glareBuffer;
		int32_t							nTotalFaces;
		int32_t							nTotalObjects;
		int32_t							nTotalSprites;
		int32_t							nTotalLights;
		int32_t							nMaxLights;
		int32_t							nColoredFaces;
		int32_t							nStateChanges;
		int32_t							nShaderChanges;
		int32_t							nUsedFaces;
		int32_t							nStereoOffsetType;
		float							fAttScale [2];
		float							fBrightness;
		uint8_t							nPowerupFilter;

	public:
		CRenderData ();
		void Init (void);
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

class CSecretData {
	public:
		int32_t			nReturnSegment;
		CFixMatrix	returnOrient;
	public:
		CSecretData () { memset (this, 0, sizeof (*this)); }
};

//------------------------------------------------------------------------------

typedef struct tSlideSegs {
	int16_t	nSegment;
	uint8_t	nSides;
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
		CArray<uint16_t>				faceVerts;
		CSegFace*					slidingFaces;
#if USE_RANGE_ELEMENTS
		CArray<GLuint>				vertIndex;
#endif	
		GLuint						vboDataHandle;
		GLuint						vboIndexHandle;
		uint8_t*						vertexP;
		uint16_t*						indexP;
		int32_t							nFaces;
		int32_t							nTriangles;
		int32_t							nVertices;
		int32_t							iVertices;
		int32_t							iNormals;
		int32_t							iColor;
		int32_t							iTexCoord;
		int32_t							iOvlTexCoord;
		int32_t							iLMapTexCoord;
	public:
		CFaceData ();
		void Init (void);
		bool Create (void);
		bool Resize (void);
		void Destroy (void);
};

inline int32_t operator- (tFaceTriangle* o, CArray<tFaceTriangle>& a) { return a.Index (o); }
inline int32_t operator- (CFloatVector3* o, CArray<CFloatVector3>& a) { return a.Index (o); }
inline int32_t operator- (tTexCoord2f* o, CArray<tTexCoord2f>& a) { return a.Index (o); }

typedef struct tSegList {
	int32_t					nSegments;
	CArray<int16_t>		segments;
} tSegList;

typedef struct tSegExtent {
	CFixVector			vMin;
	CFixVector			vMax;
	} tSegExtent;


class CSkyBox : public CStack< int16_t > {
	public:
		void Destroy (void);
		int32_t CountSegments (void);
		inline int32_t GetSegList (int16_t*& listP) {
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
	int32_t		nIndex;
	uint16_t	nSegments;
	} tSegGridIndex;

class CSegmentGrid {
	private:
		CArray<tSegGridIndex>	m_index;
		CShortArray					m_segments;
		CFixVector					m_vDim;
		CFixVector					m_vMin;
		CFixVector					m_vMax;
		int32_t							m_nGridSize;

	public:
		bool Create (int32_t nGridSize, int32_t bSkyBox);
		void Destroy (void);
		int32_t GetSegList (CFixVector vPos, int16_t*& listP);
		inline int32_t GridIndex (int32_t x, int32_t y, int32_t z);
		inline bool Available (void) { return (m_segments.Buffer () != NULL); }
};


class CSegDistHeader {
	public:
		uint16_t	offset;
		uint16_t	length;
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
		CArray<uint16_t> dist;

	inline void Set (uint16_t nSegment, fix xDistance) {
		nSegment -= offset;
		if (nSegment < length)
			dist [nSegment] = (xDistance < 0) ? 0xFFFF : (uint16_t) ((float) xDistance / scale);
		}

	inline int32_t Get (uint16_t nSegment) {
		nSegment -= offset;
		if (nSegment >= length)
			return -1;
		uint16_t d = dist [nSegment];
		if (d == 0xFFFF)
			return -1;
		return (fix) ((float) d * scale);
		}

	inline bool Read (CFile& cf, int32_t bCompressed) {
		if (!CSegDistHeader::Read (cf))
			return false;
		if (!length)
			return true;
		if (!(dist.Resize (length, false)))
			return false;
		return (dist.Read (cf, length, 0, bCompressed) > 0);
		}

	inline bool Write (CFile& cf, int32_t bCompressed) {
		return CSegDistHeader::Write (cf) && dist.Write (cf, length, 0, bCompressed);
		}

	inline bool Create (void) { return (length > 0) && (dist.Create (length) != NULL); }

	inline void Destroy (void) { dist.Destroy (); }

	};

typedef struct tVertexOwner {
	uint16_t	nSegment;
	uint8_t		nSide;
} tVertexOwner;

class CSegmentData {
	public:
		int32_t							nMaxSegments;
		CArray<CFixVector>		vertices;
		CArray<CFloatVector>		fVertices;
		CArray<tVertexOwner>		vertexOwners;
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
		CArray<uint8_t>				bSegVis;
		CArray<uint8_t>				bVertVis;
		CArray<uint8_t>				bLightVis;
		CArray<CSegDistList>		segDistTable;
		CArray<int16_t>				vertexSegments; // all segments using this vertex
		int32_t							nVertices;
		int32_t							nFaceVerts;
		int32_t							nLastVertex;
		int32_t							nSegments;
		int32_t							nLastSegment;
		int32_t							nFaces;
		int32_t							nFaceKeys;
		int32_t							nLevelVersion;
		char							szLevelFilename [FILENAME_LEN];
		CSecretData					secret;
		CArray<tSlideSegs>		slideSegs;
		int16_t							nSlideSegs;
		int32_t							bHaveSlideSegs;
		CFaceData					faces;

	public:
		CSegmentData ();
		void Init (void);
		bool Create (int32_t nSegments, int32_t nVertices);
		bool Resize (void);
		void Destroy (void);

		inline int32_t SegVisSize (int32_t nElements = 0) {
			if (!nElements)
				nElements = nSegments;
			return (TRIAMATSIZE (nSegments) + 7) / 8;
			}

		inline int32_t SegVisIdx (int32_t i, int32_t j) {
			return TRIAMATIDX (i, j);
			}

		inline int32_t SegVis (int32_t i, int32_t j) {
			i = SegVisIdx (i, j);	// index in triangular matrix, enforce j <= i
			return (i >= 0) && (bSegVis [i >> 3] & (1 << (i & 7))) != 0;
			}

		inline int32_t SetSegVis (int16_t nSrcSeg, int16_t nDestSeg) {
			int32_t i = SegVisIdx (nSrcSeg, nDestSeg);
			uint8_t* flagP = &bSegVis [i >> 3];
			uint8_t flag = 1 << (i & 7);
#ifdef _OPENMP
#	pragma omp atomic
#endif
			*flagP |= flag;
			return 1;
			}

		inline int32_t SegDistSize (int32_t nElements = 0) {
			if (!nElements)
				nElements = nSegments;
			return QUADMATSIZE (nSegments);
			}

		inline int32_t SegDistIdx (int32_t i, int32_t j) {
			return QUADMATIDX (i, j, nSegments);
			}

		inline int32_t SegDist (uint16_t i, uint16_t j) {
			return segDistTable [i].Get (j);
			}

		inline void SetSegDist (uint16_t i, uint16_t j, fix xDistance) {
			segDistTable [i].Set (j, xDistance);
			}

		inline int32_t LightVisIdx (int32_t i, int32_t j) {
			return QUADMATIDX (i, j, nSegments);
			}

		inline int8_t LightVis (int32_t nLight, int32_t nSegment) {
			int32_t i = LightVisIdx (nLight, nSegment);
			if ((i >> 2) >= (int32_t) bLightVis.Length ())
				return 0;
			return int8_t (((bLightVis [i >> 2] >> ((i & 3) << 1)) & 3) - 1);
			}

		inline int32_t SetLightVis (int32_t nLight, int32_t nSegment, uint8_t flag) {
			int32_t i = LightVisIdx (nLight, nSegment);
			uint8_t* flagP = &bLightVis [i >> 2];
			flag <<= ((i & 3) << 1);
#ifdef _OPENMP
#	pragma omp atomic
#endif
			*flagP |= flag;
			return 1;
			}

		inline bool BuildGrid (int32_t nSize, int32_t bSkyBox) { return grids [bSkyBox].Create (nSize, bSkyBox); }
		inline int32_t GetSegList (CFixVector vPos, int16_t*& listP, int32_t bSkyBox) { return grids [bSkyBox].GetSegList (vPos, listP); }
		inline bool HaveGrid (int32_t bSkyBox) { return grids [bSkyBox].Available (); }
};

//------------------------------------------------------------------------------

class CWallData {
	public:
		CArray<CWall>				walls; //[MAX_WALLS];
		CStack<CExplodingWall>	exploding; //[MAX_EXPLODING_WALLS];
		CStack<CActiveDoor>		activeDoors; //[MAX_DOORS];
		CStack<CCloakingWall>	cloaking; //[MAX_CLOAKING_WALLS];
		CArray<tWallEffect>		anims [2]; //[MAX_WALL_ANIMS];
		CArray<int32_t>			bitmaps; //[MAX_WALL_ANIMS];
		int32_t						nWalls;
		int32_t						nAnims [2];
		CArray<tWallEffect>		animP;

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
//		CArray<int16_t>				firstObjTrigger; // [MAX_OBJECTS_D2X];
		CArray<int32_t>			delay; // [MAX_TRIGGERS];
		int32_t						m_nTriggers;
		int32_t						m_nObjTriggers;
	public:
		CTriggerData ();
		bool Create (int32_t nTriggers, bool bObjTriggers);
		void Destroy (void);
};

//------------------------------------------------------------------------------

class CPowerupData {
	public:
		CStaticArray< tPowerupTypeInfo, MAX_POWERUP_TYPES >	info; // [MAX_POWERUP_TYPES];
		int32_t						nTypes;

	public:
		CPowerupData () { nTypes = 0; }
} ;

//------------------------------------------------------------------------------

class CObjTypeData {
	public:
		int32_t						nTypes;
		CStaticArray< int8_t, MAX_OBJTYPE >	nType; //[MAX_OBJTYPE];
		CStaticArray< int8_t, MAX_OBJTYPE >	nId; //[MAX_OBJTYPE];  
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
	int16_t				nTransformed;
#endif
	} tBox;

typedef struct tHitbox {
	CFixVector			vMin;
	CFixVector			vMax;
	CFixVector			vSize;
	CFixVector			vOffset;
	tBox					box;
	CAngleVector		angles;			//rotation angles
	int16_t				nParent;			//parent hitbox
} tHitbox;

//------------------------------------------------------------------------------

typedef struct tObjectViewData {
	CFixMatrix			mView [2];
	int32_t				nFrame [2];
} tObjectViewData;

typedef struct tGuidedMissileInfo {
	CObject*				objP;
	int32_t				nSignature;
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
		CArray<int16_t>			freeList;
		CArray<int16_t>			freeListIndex;
		CArray<int16_t>			parentObjs;
		CArray<tObjectRef>		childObjs;
		CArray<int16_t>			firstChild;
		CArray<CObject>			init;
		CArray<tObjDropInfo>		dropInfo;
		CArray<tSpeedBoostData>	speedBoost;
		CArray<CFixVector>		vRobotGoals;
		CArray<fix>					xLastAfterburnerTime;
		CArray<fix>					xLight;
		CArray<int32_t>			nLightSig;
		CFaceColor					color;
		int16_t						nFirstDropped;
		int16_t						nLastDropped;
		int16_t						nFreeDropped;
		int16_t						nDropped;
		tGuidedMissileInfo		guidedMissile [MAX_PLAYERS];
		CObject*						consoleP;
		CObject*						viewerP;
		CObject*						trackGoals [2];
		CObject*						missileViewerP;
		CObject*						deadPlayerCamera;
		CObject*						endLevelCamera;
		int32_t						nMaxObjects;
		int32_t						nObjects;
		int32_t						nLastObject [2];
		int32_t						nObjectLimit;
		int32_t						nMaxUsedObjects;
		int32_t						nInitialRobots;
		int32_t						nEffects;
		int32_t						nDebris;
		int32_t						nNextSignature;
		int32_t						nChildFreeList;
		int32_t						nDrops;
		int32_t						nDeadControlCenter;
		int32_t						nVertigoBotFlags;
		int32_t						nFrameCount;
		CArray<int16_t>			nHitObjects;
		CPowerupData				pwrUp;
		uint8_t						collisionResult [MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];
		CArray<tObjectViewData>	viewData;
		CStaticArray< int16_t, MAX_WEAPONS >	idToOOF; //[MAX_WEAPONS];
		CByteArray				bWantEffect; //[MAX_OBJECTS_D2X];

	public:
		CObjectData ();
		void Init (void);
		bool Create (void);
		void Destroy (void);
		void InitFreeList (void);
		void GatherEffects (void);
		void SetGuidedMissile (uint8_t nPlayer, CObject* objP);
		bool IsGuidedMissile (CObject* objP);
		bool HasGuidedMissile (uint8_t nPlayer);
		CObject* GetGuidedMissile (uint8_t nPlayer);
		int32_t RebuildEffects (void);
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
		int32_t					bCache;
		int32_t					vertices [6];
		int32_t					nFaces;
		int16_t					nSegment;
		int16_t					nSide;
		int16_t					nType;

	public:
		CFVISideData ();
};

class CPhysicsData {
	public:
		CStaticArray< int16_t, MAX_FVI_SEGS >	segments;
		CFVISideData		side;
		float					fLastTick;
		fix					xTime;
		fix					xAfterburnerCharge;
		fix					xBossInvulDot;
		CFixVector			playerThrust;
		int32_t				nSegments;
		int32_t				bIgnoreObjFlag;

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
		int32_t					nJoints;
		int32_t					nDefaultJoints;
		int32_t					nCamBotId;
		int32_t					nCamBotModel;
		int32_t					nTypes [2];
		int32_t					nDefaultTypes;
		int32_t					bReplacementsLoaded;
		CArray<tRobotInfo>	infoP;
		CArray<POF::CModel>	pofData;
	public:
		CRobotData ();
		~CRobotData () {}
};

#define D1ROBOT(_id)		(gameStates.app.bD1Mission && ((_id) < gameData.bots.nTypes [1]))
//#define ROBOTINFO(_id)	gameData.bots.info [D1ROBOT (_id)][_id]

//------------------------------------------------------------------------------

#if USE_OPENAL

typedef struct tOpenALData {
	ALCdevice			*device;
	ALCcontext			*context;
} tOpenALData;

#endif

class CSoundData {
	public:
		int32_t					nType;	// 0: D2, 1: D1
		//CArray<uint8_t>			data [2];
		CArray<CSoundSample>	sounds [2]; //[MAX_SOUND_FILES];
		int32_t					nSoundFiles [2];
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
		CArray<uint16_t>			bitmapFlags [2]; //[MAX_BITMAP_FILES];
		CArray<CBitmap>			bitmaps [2]; //[MAX_BITMAP_FILES];
		CArray<CBitmap>			altBitmaps [2]; //[MAX_BITMAP_FILES];
		CArray<CBitmap>			addonBitmaps ; //[MAX_ADDON_BITMAP_FILES];
		CArray<uint16_t>			bitmapXlat ; //[MAX_BITMAP_FILES];
		CArray<alias>				aliases ; //[MAX_ALIASES];
		CArray<tBitmapIndex>		bmIndex [2]; //[MAX_TEXTURES];
		CArray<tBitmapIndex>		objBmIndex ; //[MAX_OBJ_BITMAPS];
		CArray<tBitmapIndex>		defaultObjBmIndex ; //[MAX_OBJ_BITMAPS];
		CArray<int16_t>			textureIndex [2]; //[MAX_BITMAP_FILES];
		CArray<uint16_t>			objBmIndexP ; //[MAX_OBJ_BITMAPS];
		CArray<tBitmapIndex>		cockpitBmIndex; //[N_COCKPIT_BITMAPS];
		CArray<CFloatVector3>	bitmapColors ; //[MAX_BITMAP_FILES];
		int32_t						nBitmaps [2];
		int32_t						nObjBitmaps;
		int32_t						bPageFlushed;
		CArray<tTexMapInfo>		tMapInfo [2] ; //[MAX_TEXTURES];
		int32_t						nExtraBitmaps;
		int32_t						nAliases;
		int32_t						nHamFileVersion;
		int32_t						nTextures [2];
		int32_t						nFirstMultiBitmap;
		CArray<tBitmapFile>		bitmapFileP;
		CArray<CBitmap>			bitmapP;
		CArray<CBitmap>			altBitmapP;
		CArray<tBitmapIndex>		bmIndexP;
		CArray<tTexMapInfo>		tMapInfoP;
		CArray<uint8_t>			rleBuffer;
		CArray<int32_t>			brightness; // [MAX_WALL_TEXTURES];
		CArray<int32_t>			defaultBrightness [2]; //[MAX_WALL_TEXTURES];

	public:
		CTextureData ();
		~CTextureData () {}
};

//------------------------------------------------------------------------------

class CEffectData {
	public:
		CArray<tEffectInfo>		effects [2]; //[MAX_EFFECTS];
		CArray<tAnimationInfo>	animations [2]; //[MAX_ANIMATIONS_D2];
		int32_t						nEffects [2];
		int32_t 						nClips [2];
		CArray<tEffectInfo>		effectP;
		CArray<tAnimationInfo>	vClipP;

	public:
		CEffectData ();
		~CEffectData () {}
};

inline int32_t operator- (tEffectInfo* o, CArray<tEffectInfo>& a) { return a.Index (o); }
inline int32_t operator- (tAnimationInfo* o, CArray<tAnimationInfo>& a) { return a.Index (o); }

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
	tAnimationInfo*	animInfoP;
	tAnimationState	animState;
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
		int32_t				queueIndex;
		tMuzzleInfo		info [MUZZLE_QUEUE_MAX];
};

#include "weapon.h"

#define GATLING_DELAY	700

typedef struct tFiringData {
	int32_t					nStart;
	int32_t					nStop;
	int32_t					nDuration;
	int32_t					bSound;
	int32_t					bSpeedUp;
	} tFiringData;

class CWeaponData {
	public:
		int8_t					nPrimary;
		int8_t					nSecondary;
		int8_t					nOverridden;
		int8_t					bTripleFusion;
		tFiringData				firing [2];
		int32_t					nTypes [2];
		CStaticArray< CWeaponInfo, MAX_WEAPON_TYPES >	info; // [MAX_WEAPON_TYPES];
		CStaticArray< CD1WeaponInfo, D1_MAX_WEAPON_TYPES >	infoD1; // [D1_MAX_WEAPON_TYPES];
		CArray<CFloatVector>	color;
		uint8_t					bLastWasSuper [2][MAX_PRIMARY_WEAPONS];

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
		int32_t			nHitboxes;
		int32_t			nFrame;
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
		uint8_t			nType [MAX_THRUSTERS];
		int16_t			nCount;
	};

typedef struct tGunInfo {
	int32_t					nGuns;
	CFixVector			vGunPoints [MAX_GUNS];
	} tGunInfo;

typedef struct tModelSphere {
	int16_t					nSubModels;
	int16_t					nFaces;
	int16_t					nFaceVerts;
	fix					xRads [3];
	CFixVector			vOffsets [3];
} tModelSphere;

#include "rendermodel.h"

class CModelData {
	public:
		int32_t									nLoresModels;
		int32_t									nHiresModels;
		int32_t									nPolyModels;
		int32_t									nDefPolyModels;
		int32_t									nSimpleModelThresholdScale;
		int32_t									nMarkerModel;
		int32_t									nCockpits;
		int32_t									nLightScale;
		CArray<int32_t>							nDyingModels ; //[MAX_POLYGON_MODELS];
		CArray<int32_t>							nDeadModels ; //[MAX_POLYGON_MODELS];
		CArray<ASE::CModel>				aseModels [2]; //[MAX_POLYGON_MODELS];
		CArray<OOF::CModel>				oofModels [2]; //[MAX_POLYGON_MODELS];
		CArray<POF::CModel>				pofData [2][2]; //[MAX_POLYGON_MODELS];
		CArray<uint8_t>						bHaveHiresModel ; //[MAX_POLYGON_MODELS];
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
		int32_t					nLevel;
		uint8_t					ipAddr [4];
		int32_t					nPort;
		uint8_t					uConnect;
		uint8_t					uType;
		uint8_t					bHost;
		uint8_t					bTeam;				// team game?
		uint8_t					bValid;

	public:
		CAutoNetGame () { memset (this, 0, sizeof (*this)); }
};

typedef struct tLeftoverPowerup {
	CObject				*spitterP;
	uint8_t					nCount;
	uint8_t					nType;
} tLeftoverPowerup;

class CWeaponState {
	public:
		tFiringData				firing [2];
		fix						xMslFireTime;
		fix						xFusionCharge;
		int16_t					nAmmoUsed;
		char						nMissiles;
		char						nPrimary;
		char						nSecondary;
		char						bQuadLasers;
		char						nLaserLevel;
		char						bTripleFusion;
		char						nMslLaunchPos;
		uint8_t					nBuiltinMissiles;
		uint8_t					nThrusters [5];
		uint8_t					nShip;

	public:
		CWeaponState () { memset (this, 0, sizeof (*this)); }
	};

#define BUILTIN_MISSILES	(2 + NDL - gameStates.app.nDifficultyLevel)

class CMultiplayerData {
	public:
		int32_t 								nPlayers;				
		int32_t								nMaxPlayers;
		int32_t 								nLocalPlayer;				
		int32_t								nPlayerPositions;
		int32_t								bMoving;
		CPlayerData							players [MAX_PLAYERS + 4];  
		tObjPosition						playerInit [MAX_PLAYERS];
		int16_t								nVirusCapacity [MAX_PLAYERS];
		int32_t								nLastHitTime [MAX_PLAYERS];
		int32_t								tAppearing [MAX_PLAYERS][2];
		int8_t								bTeleport [MAX_PLAYERS];
		CWeaponState						weaponStates [MAX_PLAYERS];
		char									bWasHit [MAX_PLAYERS];
		int32_t								bulletEmitters [MAX_PLAYERS];
		int32_t					 			gatlingSmoke [MAX_PLAYERS];
		CPulseData							spherePulse [MAX_PLAYERS];
		CStaticArray< uint16_t, MAX_POWERUP_TYPES >	powerupsInMine; //[MAX_POWERUP_TYPES];
		CStaticArray< uint16_t, MAX_POWERUP_TYPES >	powerupsOnShip; //[MAX_POWERUP_TYPES];
		CStaticArray< uint16_t, MAX_POWERUP_TYPES >	maxPowerupsAllowed; //[MAX_POWERUP_TYPES];
		bool									bAdjustPowerupCap [MAX_PLAYERS];
		CArray<tLeftoverPowerup>		leftoverPowerups;
		CAutoNetGame						autoNG;
		fix									xStartAbortMenuTime;

	public:
		CMultiplayerData ();
		bool Create (void);
		void Destroy (void);
#if DBG
		void Connect (int32_t nPlayer, int8_t nStatus);
#else
		inline void Connect (int32_t nPlayer, int8_t nStatus) {
			players [nPlayer].Connect (nStatus);
			}
#endif

		bool WaitingForExplosion (void) {
			for (int32_t i = 0; i < nPlayers; i++)
				if (players [i].WaitingForExplosion ())
					return true;
			return false;
			}

		bool WaitingForWeaponInfo (void) {
			for (int32_t i = 0; i < nPlayers; i++) {
				if (weaponStates [i].nShip == 255)
					return true;
				if ((i != nLocalPlayer) && players [i].WaitingForWeaponInfo ())
					return true;
				}
			return false;
			}

		inline uint16_t PrimaryAmmo (int16_t nPlayer, int16_t nWeapon) { return players [nPlayer].primaryAmmo [nWeapon]; }
		inline uint16_t SecondaryAmmo (int16_t nPlayer, int16_t nWeapon, int32_t bBuiltin = 1) { 
			uint16_t nAmmo = players [nPlayer].secondaryAmmo [nWeapon];
			if (nWeapon || bBuiltin)
				return nAmmo; 
			uint16_t nBuiltin = BuiltinMissiles (nPlayer);
			return (nAmmo > nBuiltin) ? nAmmo - nBuiltin : 0;
			}
		inline uint16_t BuiltinMissiles (int16_t nPlayer) { return weaponStates [nPlayer].nBuiltinMissiles; }
		inline bool Flag (int16_t nPlayer, uint32_t nFlag) { return (players [nPlayer].flags & nFlag) != 0; }
};

#include "multi.h"

//------------------------------------------------------------------------------

class CMultiCreateData {
	public:
		CStaticArray< int32_t, MAX_NET_CREATE_OBJECTS >	nObjNums; // [MAX_NET_CREATE_OBJECTS];
		int32_t					nCount;

	public:
		CMultiCreateData () { nCount = 0; }
};

#define MAX_FIRED_OBJECTS	8

class CMultiWeaponData {
	public:
		int32_t					bFired;
		uint8_t					nFired [2];
		int16_t					nObjects [2][MAX_FIRED_OBJECTS];
		int32_t					nGun;
		int32_t					nFlags;
		int32_t					nLevel;
		int16_t					nTrack;

	public:
		CMultiWeaponData () { memset (this, 0, sizeof (*this)); }
};


class CMultiMsgData {
	public:
		char					bSending;
		char					bDefining;
		int32_t				nIndex;
		char					szMsg [MAX_MESSAGE_LEN];
		char					szMacro [4][MAX_MESSAGE_LEN];
		uint8_t				buf [MULTI_MAX_MSG_LEN+4];            // This is where multiplayer message are built
		int32_t				nReceiver;

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
		int32_t				nSorted [MAX_PLAYERS];
		int16_t				matrix [MAX_PLAYERS][MAX_PLAYERS];
		int16_t				nTeam [2];
		char					bShowList;
		fix					xShowListTimer;

	public:
		CMultiScoreData () { memset (this, 0, sizeof (*this)); }
};

class CMultiGameData {
	public:
		int32_t				nWhoKilledCtrlcen;
		char					bShowReticleName;
		char					bIsGuided;
		char					bQuitGame;
		CMultiCreateData	create;
		CMultiWeaponData	weapon;
		CMultiMsgData		msg;
		CMultiMenuData		menu;
		CMultiScoreData	score;
		tMultiRobotData	robots;
		CArray<int16_t>	remoteToLocal [MAX_PLAYERS];  // Remote CObject number for each local CObject
		CArray<int16_t>	localToRemote;
		CArray<int8_t>		nObjOwner;   // Who created each CObject in my universe, -1 = loaded at start
		int32_t				bGotoSecret;
		int32_t				nTypingTimeout;

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
		int32_t   nTotalRooms;

	public:
		CEntropyData () { memset (this, 0, sizeof (*this)); }
};

extern CEntropyData entropyData;

typedef struct tCountdownData {
	fix					nTimer;
	int32_t					nSecsLeft;
	int32_t					nTotalTime;
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
		CStaticArray< int16_t, NUM_MARKERS >			objects; // [NUM_MARKERS];
		int16_t					viewers [2];
		int32_t					nHighlight;
		float					fScale;
		uint8_t					nDefiningMsg;
		char					szInput [40];
		int32_t					nIndex;
		int32_t					nCurrent;
		int32_t					nLast;
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
		int32_t					tLast;
		fix					xSlack;
		fix					xStarted;
		fix					xStopped;
		fix					xLastThiefHitTime;
		int32_t					nPaused;
		int32_t					nStarts;
		int32_t					nStops;

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
		int32_t					nFrameCount;
		int32_t					nMineRenderCount;
		int32_t					nGameMode;
		int32_t					bGamePaused;
		uint32_t					nStateGameId;
		uint32_t					semaphores [4];
		int32_t					nLifetimeChecksum;
		int32_t					bUseMultiThreading [rtTaskCount];
		int32_t					argC;
		char**				argV;

	public:
		CApplicationData ();
		inline int32_t GameMode (int32_t nFlags = 0xFFFFFFFF) { return nGameMode & nFlags; }
		inline void SetGameMode (int32_t nMode) {
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
	ptLighting,
	ptRenderPass,
	ptFaceList,
	ptRenderFaces,
	ptVertexColor,
	ptPerPixelLighting,
	ptRenderObjects,
	ptRenderObjectMeshes,
	ptRenderStates,
	ptShaderStates,
	ptTranspPolys,
	ptEffects,
	ptParticles,
	ptCockpit,
	ptTransform,
	ptAux,
	ptGameStates,
	ptObjectStates,
	ptPhysics,
	ptTagCount
	} tProfilerTags;

typedef struct tProfilerData {
	time_t				t [ptTagCount];
	int32_t					nFrameCount;
	int32_t					bToggle;
} tProfilerData;

#define PROF_INIT				memset(&gameData.profiler, 0, sizeof (gameData.profiler));
#define PROF_START			time_t tProf = clock ();
#define PROF_CONT				tProf = clock ();
#define PROF_END(_tag)		(gameData.profiler.t [_tag]) += clock () - tProf;
#define PROF_RESET(_tag)	(gameData.profiler.t [_tag]) = 0;
#define PROF_TOGGLE			if (gameData.profiler.bToggle) { \
										gameStates.render.bShowProfiler = (gameStates.render.bShowProfiler + 1) % 3; \
										PROF_INIT; \
										}

#else

#define PROF_INIT
#define PROF_START	
#define PROF_CONT	
#define PROF_END(_tAcc)
#define PROF_TOGGLE			

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
		int16_t					m_nTeleportSegs;
		CShortArray			m_teleportSegs; // [MAX_BOSS_TELEPORT_SEGS];
		int16_t					m_nGateSegs;
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
		int32_t					m_bHitThisFrame;
		int32_t					m_bHasBeenHit;
		int32_t					m_nObject;
		int16_t					m_nDying;
		int8_t					m_bDyingSoundPlaying;

	public:
		CBossInfo () { Init (); }
		~CBossInfo () { Destroy (); }
		void Init (void);
		bool Setup (int16_t nObject);
		void Destroy (void);
		bool SetupSegments (CShortArray& segments, int32_t bSizeCheck, int32_t bOneWallHack);
		void InitGateInterval (void);
		void SaveState (CFile& cf);
		void SaveSizeStates (CFile& cf);
		void SaveBufferStates (CFile& cf);
		void LoadBinState (CFile& cf);
		void LoadState (CFile& cf, int32_t nVersion);
		void LoadSizeStates (CFile& cf);
		int32_t LoadBufferStates (CFile& cf);

		inline void ResetHitTime (void) { m_nHitTime = -I2X (10); }
		void ResetCloakTime (void);

	private:
		void SaveBufferState (CFile& cf, CShortArray& buffer);
		int32_t LoadBufferState (CFile& cf, CShortArray& buffer, int32_t nBufSize);
	};

class CBossData {
	private:	
		CStack<CBossInfo>	m_info;

	public:
		CBossData ();
		bool Create (uint32_t nBosses = 0);
		void Destroy (void);
		void Setup (int32_t nObject = -1);
		int16_t Find (int16_t nBossObj);
		int32_t Add (int16_t nObject);
		void Remove (int16_t nBoss);
		void ResetHitTimes (void);
		void ResetCloakTimes (void);
		void InitGateIntervals (void);

		inline void Shrink (uint32_t i = 1) { m_info.Shrink (i); }
		inline uint32_t ToS (void) { return m_info.ToS (); }
		inline bool Grow (uint32_t i = 1) { return m_info.Grow (i); }
		inline CBossInfo& Boss (uint32_t i) { return m_info [i]; }
		inline uint32_t BossCount (void) { return m_info.Buffer () ? m_info.ToS () : 0; }
		inline uint32_t Count (void) { return m_info.Buffer () ? m_info.ToS () : 0; }
		inline CBossInfo& operator[] (uint32_t i) { return m_info [i]; }

		void SaveStates (CFile& cf);
		void SaveSizeStates (CFile& cf);
		void SaveBufferStates (CFile& cf);
		void LoadBinStates (CFile& cf);
		void LoadStates (CFile& cf, int32_t nVersion);
		void LoadSizeStates (CFile& cf);
		int32_t LoadBufferStates (CFile& cf);
	};

//------------------------------------------------------------------------------

#include "reactor.h"

typedef struct tReactorStates {
	int32_t					nObject;
	int32_t					bHit;
	int32_t					bSeenPlayer;
	int32_t					nNextFireTime;
	int32_t					nDeadObj;
	int32_t					nStrength;
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
		int32_t					nReactors;
		int32_t					nStrength;
		int32_t					bPresent;
		int32_t					bDisabled;
		int32_t					bDestroyed;

	public:
		CReactorData ();
};

//------------------------------------------------------------------------------

#include "ai.h"

class CAITarget {
	public:
		int16_t							nBelievedSeg;
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
		int32_t							bInitialized;
		int32_t							nOverallAgitation;
		int32_t							bEvaded;
		int32_t							bEnableAnimation;
		int32_t							bInfoEnabled;
		CFixVector					vHitPos;
		int32_t							nHitType;
		int32_t							nHitSeg;
		CHitResult					hitResult;
		CAITarget					target;
		CArray<tAILocalInfo>		localInfo;
		CArray<tAICloakInfo>		cloakInfo; // [MAX_AI_CLOAK_INFO];
		CArray<tPointSeg>			routeSegs; // [MAX_POINT_SEGS];
		tPointSeg*					freePointSegs;
		int32_t							nAwarenessEvents;
		int32_t							nMaxAwareness;
		CFixVector					vGunPoint;
		int32_t							nTargetVisibility;
		int32_t							bObjAnimates;
		int32_t							nLastMissileCamera;
		CArray<tAwarenessEvent>	awarenessEvents; //[MAX_AWARENESS_EVENTS];

	public:
		CAIData ();
		bool Create (void);
		void Destroy (void);
};

inline int32_t operator- (tAILocalInfo* o, CArray<tAILocalInfo>& a) { return a.Index (o); }
inline int32_t operator- (tAICloakInfo* o, CArray<tAICloakInfo>& a) { return a.Index (o); }
inline int32_t operator- (tPointSeg* o, CArray<tPointSeg>& a) { return a.Index (o); }

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
	int32_t			nModel;
} tStationData;

class CTerrainData {
	public:
		CBitmap			bmInstance;
		CBitmap			*bmP;
};

typedef struct tExitData {
	int32_t				nModel;
	int32_t				nDestroyedModel;
	CFixVector			vMineExit;
	CFixVector			vGroundExit;
	CFixVector			vSideExit;
	CFixMatrix			mOrient;
	int16_t				nSegNum;
	int16_t				nTransitSegNum;
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
		int32_t		bValid;
		uint32_t		tinyColors [2][2];
		uint32_t		warnColor;
		uint32_t		keyColor;
		uint32_t		tabbedColor;
		uint32_t		helpColor;
		uint32_t		colorOverride;
		int32_t		nLineWidth;
		uint8_t		alpha;

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
		int32_t			nProducers;
		int32_t			nRobotMakers;
		int32_t			nEquipmentMakers;
		int32_t			nRepairCenters;
		fix				xEnergyToCreateOneRobot;
		CStaticArray< int32_t, MAX_FUEL_CENTERS >				origProducerTypes; // [MAX_FUEL_CENTERS];
		CSegment*		playerSegP;

	public:
		CProducerData ();
		tObjectProducerInfo* Find (int16_t nSegment);

	private:
		tObjectProducerInfo* Find (int16_t nSegment, CStaticArray< tObjectProducerInfo, MAX_ROBOT_CENTERS >& producers);
};

//------------------------------------------------------------------------------

class CDemoData {
	public:
		int32_t			bAuto;
		char				fnAuto [FILENAME_LEN];
		CArray<int8_t>	bWasRecorded;
		CArray<int8_t>	bViewWasRecorded;
		int8_t			bRenderingWasRecorded [32];
		char				callSignSave [CALLSIGN_LEN + 1];
		int32_t			nVersion;
		int32_t			nState;
		int32_t			nVcrState;
		int32_t			nStartFrame;
		int64_t			nSize;
		int64_t			nWritten;
		int32_t			nGameMode;
		int32_t			nOldCockpit;
		int8_t			bNoSpace;
		int8_t			bEof;
		int8_t			bInterpolate;
		int8_t			bPlayersCloaked;
		int8_t			bWarningGiven;
		int8_t			bCtrlcenDestroyed;
		int32_t			nFrameCount;
		int16_t			nFrameBytesWritten;
		fix				xStartTime;
		fix				xPlaybackTotal;
		fix				xRecordedTotal;
		fix				xRecordedTime;
		int8_t			nPlaybackStyle;
		int8_t			bFirstTimePlayback;
		fix				xJasonPlaybackTotal;
		int32_t			bUseShortPos;
		int32_t			bFlyingGuided;

	public:
		CDemoData () {memset (this, 0, sizeof (*this)); }
		bool Create (void);
		void Destroy (void);
};

//------------------------------------------------------------------------------

#define GUIDEBOT_NAME_LEN 9

class CEscortData {
	public:
		int32_t			nMaxLength;
		int32_t			nObjNum;
		int32_t			bSearchingMarker;
		int32_t			nLastKey;
		int32_t			nKillObject;
		int32_t			nGoalObject;
		int32_t			nSpecialGoal;
		int32_t			nGoalIndex;
		int32_t			bMayTalk;
		int32_t			bMsgsSuppressed;
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
		CStaticArray< uint8_t, MAX_STOLEN_ITEMS >	stolenItems; // [MAX_STOLEN_ITEMS];
		int32_t			nStolenItem;
		fix				xReInitTime;
		fix				xWaitTimes [NDL];

	public:
		CThiefData ();
};

//------------------------------------------------------------------------------

typedef struct tHoardItem {
	int32_t		nWidth;
	int32_t		nHeight;
	int32_t		nFrameSize;
	int32_t		nSize;
	int32_t		nFrames;
	int32_t		nClip;
	CBitmap		bm;
	CPalette		*palette;
} tHoardItem;

class CHoardData {
	public:
		int32_t		bInitialized;
		int32_t		nTextures;
		tHoardItem	orb;
		tHoardItem	icon [2];
		tHoardItem	goal;
		tHoardItem	monsterball;
		int16_t		nMonsterballSeg;
		CFixVector	vMonsterballPos;
		CObject		*monsterballP;
		int16_t		nLastHitter;
};

//------------------------------------------------------------------------------

#include "hudmsgs.h"

class CHUDMessage {
	public:
		int32_t			nFirst;
		int32_t			nLast;
		int32_t			nMessages;
		fix				xTimer;
		uint32_t			nColor;
		char				szMsgs [HUD_MAX_MSGS][HUD_MESSAGE_LENGTH + 5];
		int32_t			nMsgIds [HUD_MAX_MSGS];

	public:
		CHUDMessage ();
};

class CHUDData {
	public:	
		CHUDMessage		msgs [2];
		int32_t			bPlayerMessage;

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
		int32_t		bOn;
		int32_t		nMarks;
		int32_t		nStartTime;
		int32_t		nSegment;
		int32_t		nSide;
		int32_t		nFrameStart;
		int32_t		nCount;
	};
#endif

//------------------------------------------------------------------------------

class CLaserData {
	public:
		fix		xLastFiredTime;
		fix		xNextFireTime;
		fix		xUpdateTime;
		int32_t	nGlobalFiringCount;
		int32_t	nMissileGun;
		int32_t	nOffset;
		int32_t	nProximityDropped;
		int32_t	nSmartMinesDropped;
};

//------------------------------------------------------------------------------

class CFusionData {
	public:
		fix		xAutoFireTime;
		fix		xCharge;
		fix		xNextSoundTime;
		fix		xLastSoundTime;
		int32_t	xFrameTime;
};

//------------------------------------------------------------------------------

class COmegaData {
	public:
		fix		xCharge [2];
		fix		xMaxCharge;
		int32_t	nLastFireFrame;

	public:
		COmegaData ();
};

//------------------------------------------------------------------------------

class CMissileData {
	public:
		fix		xLastFiredTime;
		fix		xNextFireTime;
		int32_t		nGlobalFiringCount;
};

//------------------------------------------------------------------------------

typedef struct tPlayerStats {
	int32_t	nShots [2];
	int32_t	nHits [2];
	int32_t	nMisses [2];
	} tPlayerStats;

class CStatsData {
	public:
		tPlayerStats	player [2];	//per level/per session
		int32_t				nDisplayMode;
	};

//------------------------------------------------------------------------------

class CCollisionData {
	public:
		int32_t			nSegsVisited;
		CStaticArray< int16_t, MAX_SEGS_VISITED >	segsVisited; // [MAX_SEGS_VISITED];
		CHitInfo hitResult;
};

//------------------------------------------------------------------------------

class CTrackIRData {
	public:
		int32_t	x, y;
};


//------------------------------------------------------------------------------

class CScoreData {
	public:
		int32_t nKillsChanged;
		int32_t bNoMovieMessage;
		int32_t nHighscore;
		int32_t nChampion;
		int32_t bWaitingForOthers;

	public:
		CScoreData ();
};

//------------------------------------------------------------------------------

typedef struct tTextIndex {
	int32_t	nId;
	int32_t	nLines;
	char	*pszText;
} tTextIndex;

class CTextData {
	public:
		char*			textBuffer;
		tTextIndex*	index;
		tTextIndex*	currentMsg;
		int32_t			nMessages;
		int32_t			nStartTime;
		int32_t			nEndTime;
		CBitmap*		bmP;
};

//------------------------------------------------------------------------------

#define MAX_GAUGE_BMS 100   // increased from 56 to 80 by a very unhappy MK on 10/24/94.
#define D1_MAX_GAUGE_BMS 80   // increased from 56 to 80 by a very unhappy MK on 10/24/94.

#define STEREO_LEFT_FRAME	-1
#define STEREO_RIGHT_FRAME	+1

class CCockpitData {
	public:
		CArray<tBitmapIndex>	gauges [2]; //[MAX_GAUGE_BMS];
	public:
		CCockpitData ();
	};


#define GAMEDATA_CHECK_BUFFER		1
#define GAMEDATA_CHECK_UNDERFLOW	2
#define GAMEDATA_CHECK_OVERFLOW	4
#define GAMEDATA_CHECK_TYPE		8
#define GAMEDATA_ERROR_LOG			16
#define GAMEDATA_CHECK_INDEX		(GAMEDATA_CHECK_UNDERFLOW | GAMEDATA_CHECK_OVERFLOW)
#define GAMEDATA_CHECK_ALL			(GAMEDATA_CHECK_BUFFER | GAMEDATA_CHECK_INDEX | GAMEDATA_ERROR_LOG)

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
		bool Create (int32_t nSegments, int32_t nVertices);
		void Destroy (void);
		void SetFusionCharge (fix xCharge, bool bLocal = false);
		fix FusionCharge (int32_t nId = -1);
		fix FusionDamage (fix xBaseDamage);
		int32_t StereoOffset2D (void);
		int32_t FloatingStereoOffset2D (int32_t x, bool bForce = false);
		void SetStereoSeparation (int32_t nFrame);
		int32_t SetStereoOffsetType (int32_t nType) { 
			int32_t nOldType = render.nStereoOffsetType;
			if (nType >= 0)
				render.nStereoOffsetType = nType; 
			return nOldType;
			}
#if DBG
		CObject* Object (int32_t nObject, int32_t nChecks = GAMEDATA_CHECK_ALL, const char* pszFile = "", int32_t nLine = 0);
		CSegment* Segment (int32_t nSegment, int32_t nChecks = GAMEDATA_CHECK_ALL, const char* pszFile = "", int32_t nLine = 0);
		CWall* Wall (int32_t nWall, int32_t nChecks = GAMEDATA_CHECK_ALL, const char* pszFile = "", int32_t nLine = 0);
		CTrigger* Trigger (int32_t nTrigger, int32_t nChecks = GAMEDATA_CHECK_ALL, const char* pszFile = "", int32_t nLine = 0);
		CTrigger* ObjTrigger (int32_t nTrigger, int32_t nChecks = GAMEDATA_CHECK_ALL, const char* pszFile = "", int32_t nLine = 0);
		tRobotInfo* CGameData::RobotInfo (int32_t nId, int32_t nChecks, const char* pszFile, int32_t nLine);
		tRobotInfo* CGameData::RobotInfo (CObject* objP, int32_t nChecks, const char* pszFile, int32_t nLine);

#else
		inline CObject* Object (int32_t nObject, int32_t nChecks = GAMEDATA_CHECK_ALL) { 
			if (nChecks) {
				if ((nChecks & 1) && !objs.objects.Buffer ())
					return NULL;
				if ((nChecks & 2) && (nObject < 0))
					return NULL;
				if ((nChecks & 4) && (nObject > objs.nLastObject [0]))
					return NULL;
				}
			return objs.objects + nObject; 
			}

		inline CSegment* Segment (int32_t nSegment, int32_t nChecks = GAMEDATA_CHECK_ALL) { 
			if (nChecks) {
				if ((nChecks & 1) && !segs.segments.Buffer ())
					return NULL;
				if ((nChecks & 2) && (nSegment < 0))
					return NULL;
				if ((nChecks & 4) && (nSegment >= segs.nSegments))
					return NULL;
				}
			return segs.segments + nSegment; 
			}

		inline CWall* Wall (int32_t nWall, int32_t nChecks = GAMEDATA_CHECK_ALL) { 
			if (nChecks) {
				if ((nChecks & 1) && !walls.walls.Buffer ())
					return NULL;
				if ((nChecks & 2) && (nWall < 0))
					return NULL;
				if ((nChecks & 4) && (nWall >= walls.nWalls))
					return NULL;
				}
			return walls.walls + nWall; 
			}

		inline CTrigger* Trigger (int32_t nTrigger, int32_t nChecks = GAMEDATA_CHECK_ALL) { 
			if (nTrigger == NO_TRIGGER)
				return NULL;
			if (nChecks) {
				if ((nChecks & 1) && !trigs.triggers.Buffer ())
					return NULL;
				if ((nChecks & 2) && (nTrigger < 0))
					return NULL;
				if ((nChecks & 2) && (nTrigger == NO_TRIGGER))
					return NULL;
				if ((nChecks & 4) && (nTrigger >= trigs.m_nTriggers))
					return NULL;
				}
			return trigs.triggers + nTrigger; 
			}

		inline CTrigger* ObjTrigger (int32_t nTrigger, int32_t nChecks = GAMEDATA_CHECK_ALL) { 
			if (nChecks) {
				if ((nChecks & 1) && !trigs.objTriggers.Buffer ())
					return NULL;
				if ((nChecks & 2) && (nTrigger < 0))
					return NULL;
				if ((nChecks & 2) && (nTrigger == NO_TRIGGER))
					return NULL;
				if ((nChecks & 4) && (nTrigger >= trigs.m_nObjTriggers))
					return NULL;
				}
			return trigs.objTriggers + nTrigger; 
			}

		inline tRobotInfo* CGameData::RobotInfo (int32_t nId, int32_t nChecks) {
			CArray<tRobotInfo>& a = bots.info [D1ROBOT (nId)];
			if (nChecks) {
				if ((nChecks & 1) && !a.Buffer ())
					return NULL;
				if ((nChecks & 2) && (nId < 0))
					return NULL;
				if ((nChecks & 4) && (nId >= a.Length ()))
					return NULL;
				}
			return a + nId; 
			}

		inline tRobotInfo* CGameData::RobotInfo (CObject* objP, int32_t nChecks) {
			return objP->IsRobot () ? RobotInfo (objP->Id (), nChecks) : NULL;
			}

#endif
		inline int32_t X (int32_t x, bool bForce = false) { return render.nStereoOffsetType ? x - ((render.nStereoOffsetType == STEREO_OFFSET_FLOATING) ? FloatingStereoOffset2D (x, bForce) : StereoOffset2D ()) : x; }
};

extern CGameData gameData;

typedef struct tBossProps {
	uint8_t		bTeleports;
	uint8_t		bSpewMore;
	uint8_t		bSpewBotsEnergy;
	uint8_t		bSpewBotsKinetic;
	uint8_t		bSpewBotsTeleport;
	uint8_t		bInvulEnergy;
	uint8_t		bInvulKinetic;
	uint8_t		bInvulSpot;
} tBossProps;

typedef struct tIntervalf {
	float	fMin, fMax, fSize, fRad;
} tIntervalf;

extern tBossProps bossProps [2][NUM_D2_BOSSES];

extern char szAutoMission [255];
extern char szAutoHogFile [255];

#if 0
static inline uint16_t WallNumS (CSide *sideP) { return (sideP)->nWall; }
static inline uint16_t WallNumP (CSegment *segP, int16_t nSide) { return WallNumS ((segP)->m_sides + (nSide)); }
static inline uint16_t WallNumI (int16_t nSegment, int16_t nSide) { return WallNumP (SEGMENT (nSegment), nSide); }
#endif

//-----------------------------------------------------------------------------

typedef struct tGameItemInfo {
	public:
		int32_t		offset;
		int32_t		count;
		int32_t		size;

	public:
		inline void Read (CFile& cf) {
			offset = cf.ReadInt ();				// Player info
			count = cf.ReadInt ();
			size = cf.ReadInt ();
			}

} __pack__ tGameItemInfo;

typedef struct {
	uint16_t  fileinfo_signature;
	uint16_t  fileinfoVersion;
	int32_t     fileinfo_sizeof;
	char    mine_filename [15];
	int32_t     level;
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

static inline int16_t ObjIdx (CObject *objP)
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
#define  IS_WALL(_wallnum)		((uint16_t (_wallnum) != NO_WALL) && (uint16_t (_wallnum) < gameFileInfo.walls.count))

#define SEG_IDX(_segP)			((int16_t) ((_segP) - SEGMENTS))
#define SEG2_IDX(_seg2P)		((int16_t) ((_seg2P) - SEGMENTS))
#define WALL_IDX(_wallP)		((int16_t) ((_wallP) - WALLS))
#define OBJ_IDX(_objP)			ObjIdx (_objP)
#define TRIG_IDX(_triggerP)	((int16_t) ((_triggerP) - TRIGGERS))
#define FACE_IDX(_faceP)		((int32_t) ((_faceP) - FACES.faces))

void GrabMouse (int32_t bGrab, int32_t bForce);
void SetDataVersion (int32_t v);

//	-----------------------------------------------------------------------------------------------------------

static inline void OglVertex3x (fix x, fix y, fix z)
{
glVertex3f ((float) x / 65536.0f, (float) y / 65536.0f, (float) z / 65536.0f);
}

//	-----------------------------------------------------------------------------------------------------------

static inline void OglVertex3f (CRenderPoint *p, CFloatVector* v = NULL)
{
	int32_t i = p->Index ();
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

static inline float GrAlpha (uint8_t alpha)
{
if (alpha >= FADE_LEVELS)
	return 1.0f;
return 1.0f - float (alpha) / float (FADE_LEVELS);
}

//	-----------------------------------------------------------------------------------------------------------

#define	CLAMP(_val,_minVal,_maxVal)	\
		 {if ((_val) < (_minVal)) (_val) = (_minVal); else if ((_val) > (_maxVal)) (_val) = (_maxVal);}

#define N_LOCALPLAYER				gameData.multiplayer.nLocalPlayer
#define N_PLAYERS						gameData.multiplayer.nPlayers
#define NETPLAYER(_nPlayer)		netPlayers [0].m_info.players [_nPlayer]
#define PLAYER(_nPlayer)			gameData.multiplayer.players [_nPlayer]
#define LOCALPLAYER					PLAYER (N_LOCALPLAYER)
#define PLAYEROBJECT(_nPlayer)	OBJECT (PLAYER (_nPlayer).nObject)
#define LOCALOBJECT					PLAYEROBJECT (N_LOCALPLAYER)

#define OBSERVING						(gameStates.render.bObserving && OBJECTS.Buffer () && (LOCALOBJECT->Type () == OBJ_GHOST))

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
#define VERTEX_OWNERS	gameData.segs.vertexOwners
#define SEGFACES			gameData.segs.segFaces
#define OBJECTS			gameData.objs.objects
#define WALLS				gameData.walls.walls
#define TRIGGERS			gameData.trigs.triggers
#define OBJTRIGGERS		gameData.trigs.objTriggers
#define FACES				gameData.segs.faces
#define RENDERPOINTS		gameData.render.mine.visibility [0].points
#define TRIANGLES			FACES.tris

#if DBG
	#define SEGMENT(_id)				gameData.Segment (_id, GAMEDATA_CHECK_ALL, __FILE__, __LINE__)
	#define OBJECT(_id)				gameData.Object (_id, GAMEDATA_CHECK_ALL, __FILE__, __LINE__)
	#define WALL(_id)					gameData.Wall (_id, GAMEDATA_CHECK_ALL, __FILE__, __LINE__)
	#define TRIGGER(_id)				gameData.Trigger (_id, GAMEDATA_CHECK_ALL, __FILE__, __LINE__)
	#define OBJTRIGGER(_id)			gameData.ObjTrigger (_id, GAMEDATA_CHECK_ALL, __FILE__, __LINE__)
	#define ROBOTINFO(_id)			gameData.RobotInfo (_id, GAMEDATA_CHECK_ALL, __FILE__, __LINE__)
	#define SEGMENTEX(_id, _f)		gameData.Segment (_id, _f, __FILE__, __LINE__)
	#define OBJECTEX(_id, _f)		gameData.Object (_id, _f, __FILE__, __LINE__)
	#define WALLEX(_id, _f)			gameData.Wall (_id, _f, __FILE__, __LINE__)
	#define TRIGGEREX(_id, _f)		gameData.Trigger (_id, _f, __FILE__, __LINE__)
	#define OBJTRIGGEREX(_id, _f)	gameData.ObjTrigger (_id, _f, __FILE__, __LINE__)
	#define ROBOTINFOEX(_id, _f)	gameData.RobotInfo (_id, _f, __FILE__, __LINE__)
#else
	#define SEGMENT(_id)				gameData.Segment (_id, GAMEDATA_CHECK_ALL)
	#define OBJECT(_id)				gameData.Object (_id, GAMEDATA_CHECK_ALL)
	#define WALL(_id)					gameData.Wall (_id, GAMEDATA_CHECK_ALL)
	#define TRIGGER(_id)				gameData.Trigger (_id, GAMEDATA_CHECK_ALL)
	#define OBJTRIGGER(_id)			gameData.Trigger (_id, GAMEDATA_CHECK_ALL)
	#define ROBOTINFO(_id)			gameData.RobotInfo (_id, GAMEDATA_CHECK_ALL)
	#define SEGMENTEX(_id, _f)		gameData.Segment (_id, _f)
	#define OBJECTEX(_id, _f)		gameData.Object (_id, _f)
	#define WALLEX(_id, _f)			gameData.Wall (_id, _f)
	#define TRIGGEREX(_id, _f)		gameData.Trigger (_id, _f)
	#define OBJTRIGGEREX(_id, _f)	gameData.Trigger (_id, _f)
	#define ROBOTINFOEX(_id, _f)	gameData.RobotInfo (_id, _f)
#endif


#define SPECTATOR(_objP)	((gameStates.render.bFreeCam > 0) && OBJECTS.IsElement (_objP) && (OBJ_IDX (_objP) == LOCALPLAYER.nObject))
#define OBJPOS(_objP)		(SPECTATOR (_objP) ? &gameStates.app.playerPos : &(_objP)->info.position)
#define OBJSEG(_objP)		(SPECTATOR (_objP) ? gameStates.app.nPlayerSegment : (_objP)->info.nSegment)

//	-----------------------------------------------------------------------------

static inline CFixVector *PolyObjPos (CObject *objP, CFixVector *vPosP)
{
CFixVector vPos = OBJPOS (objP)->vPos;
if (objP->info.renderType == RT_POLYOBJ) {
	*vPosP = *objP->View (0) * gameData.models.offsets [objP->ModelId ()];
	*vPosP += vPos;
	return vPosP;
	}
*vPosP = vPos;
return vPosP;
}

//	-----------------------------------------------------------------------------

inline void CObject::RequestEffects (uint8_t nEffects)
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
#	define HINSTANCE	int32_t
#	define HWND void *
#endif

typedef int32_t (WINAPI *tpfnTIRInit) (HWND);
typedef int32_t (WINAPI *tpfnTIRExit) (void);
typedef int32_t (WINAPI *tpfnTIRStart) (void);
typedef int32_t (WINAPI *tpfnTIRStop) (void);
typedef int32_t (WINAPI *tpfnTIRCenter) (void);
typedef int32_t (WINAPI *tpfnTIRQuery) (tTransRotInfo *);

extern tpfnTIRInit	pfnTIRInit;
extern tpfnTIRExit	pfnTIRExit;
extern tpfnTIRStart	pfnTIRStart;
extern tpfnTIRStop	pfnTIRStop;
extern tpfnTIRCenter	pfnTIRCenter;
extern tpfnTIRQuery	pfnTIRQuery;

int32_t TIRLoad (void);
int32_t TIRUnload (void);

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

static inline void SemWait (uint32_t sem)
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

static inline void SemEnter (uint32_t sem, const char *pszFile, int32_t nLine)
{
SemWait (sem);
//PrintLog (0, "SemEnter (%d) @ %s:%d\n", sem, pszFile, nLine);
gameData.app.semaphores [sem]++;
}

static inline void SemLeave (uint32_t sem, const char *pszFile, int32_t nLine)
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


static inline void SemEnter (uint32_t sem)
{
SemWait (sem);
gameData.app.semaphores [sem]++;
}

static inline void SemLeave (uint32_t sem)
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

template<typename _T> 
inline _T Clamp (_T v, _T vMin, _T vMax) { return (v < vMin) ? vMin : (v > vMax) ? vMax : v; }

template<typename _T> 
inline _T Min (_T a, _T b) { return (a <= b) ? a : b; }

template<typename _T> 
inline _T Max (_T a, _T b) { return (a >= b) ? a : b; }

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


