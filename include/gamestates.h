#ifndef __gamestates_h
#define __gamestates_h

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
		int bNoBotAI;
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

#define BOSS_COUNT	(extraGameInfo [0].nBossCount - gameStates.gameplay.nReactorCount [0])


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

class CRenderAPIStates {
	public:
		int bInitialized;
		int bRebuilding;
		int bShadersOk;
		int bMultiTexturingOk;
		int bRender2TextureOk;
		int bPerPixelLightingOk;
		int bUseRender2Texture;
		int bReadBufferActive;
		int bFullScreen;
		int bLastFullScreen;
		int bUseTransform;
		int bGlTexMerge;
		int bBrightness;
		int bLowMemory;
		int nColorBits;
		int nPreloadTextures;
		ubyte nTransparencyLimit;
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
		int nCurWidth;
		int nCurHeight;
		int nLights;
		int iLight;
		int nFirstLight;
		int bCurFullScreen;
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
		int bOcclusionQuery;
		int bDepthBlending;
		int bUseDepthBlending;
		int bHaveDepthBuffer;
		int bHaveBlur;
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
		u_int32_t	nScreenSize;
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
		int bHaveDynLights;
		int bHaveSparks;
		int bHaveStencilBuffer;
		int bHaveStereoBuffers;
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
		int bOmegaModded;
		int bPlasmaModded;
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
		int bVSync;
		int bVSyncOk;
		int nThreads;
		fix xZoom;
		fix xZoomScale;
		ubyte nRenderingType;
		fix nFlashScale;
		fix nFlashRate;
		CCameraStates cameras;
		CCockpitStates cockpit;
		CVRStates vr;
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
		short nRandSeed;
		CCheatStates cheats;
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
#define RENDER_TYPE_OBJECTS			4
#define RENDER_TYPE_TRANSPARENCY		5
#define RENDER_PASSES					6

class CGameStates {
	public:
		CGameplayStates		gameplay;
		CInputStates			input;
		CMenuStates				menus;
		CMovieStates			movies;
		CMultiplayerStates	multi;
		CGfxStates				gfx;
		CRenderAPIStates		ogl;
		CRenderStates			render;
		CZoomStates				zoom;
		CSoundStates			sound;
		CVideoStates			video;
		CApplicationStates	app;
		CEntropyStates			entropy;
		CLimitFPSStates		limitFPS;
	};

extern CGameStates	gameStates;

//------------------------------------------------------------------------------

#endif //__gamestates_h