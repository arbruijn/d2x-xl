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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "descent.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "key.h"
#include "object.h"
#include "objeffects.h"
#include "objsmoke.h"
#include "transprender.h"
#include "lightning.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "rendermine.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "cockpit.h"
#include "hudicons.h"
#include "texmap.h"
#include "menu.h"
#include "segmath.h"
#include "u_mem.h"
#include "light.h"
#include "newdemo.h"
#include "collide.h"
#include "automap.h"
#include "text.h"
#include "fireball.h"
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "gamepal.h"
#include "sphere.h"
#include "textdata.h"
#include "tracker.h"
#include "input.h"
#include "interp.h"
#include "cheats.h"
#include "rle.h"
#include "hiresmodels.h"
#include "monsterball.h"
#include "dropobject.h"
#include "trackobject.h"
#include "soundthreads.h"
#include "sparkeffect.h"
#include "createmesh.h"
#ifdef __macosx__
#include "SDL/SDL_syswm.h"
#else
#include "SDL_syswm.h"
#endif
#include "renderthreads.h"
#include "collision_math.h"
#include "banlist.h"
#include "songs.h"
#include "headlight.h"
#include "playerprofile.h"
#include "addon_bitmaps.h"
#include "postprocessing.h"
#include "menubackground.h"
#include "findpath.h"
#include "waypoint.h"
#include "network_lib.h"

#define PHYSICS_IN_BACKGROUND 0

uint32_t nCurrentVGAMode;

#ifdef MWPROFILER
#include <profiler.h>
#endif

//#define TEST_TIMER	1		//if this is set, do checking on timer

//do menus work in 640x480 or 320x200?
//PC version sets this in main ().  Mac versios is always high-res, so set to 1 here
//	Toggle_var points at a variable which gets !ed on ctrl-alt-T press.

#if DBG                          //these only exist if debugging

int32_t bGameDoubleBuffer = 1;     //double buffer by default
fix xFixedFrameTime = 0;          //if non-zero, set frametime to this

#endif

CBitmap bmBackground;

//	Function prototypes for GAME.C exclusively.

#define cv_w  Bitmap ().Width ()
#define cv_h  Bitmap ().Height ()

void FireGun (void);
void SlideTextures (void);
void PowerupGrabCheatAll (void);

//	Other functions
void MultiCheckForScoreGoalWinner (bool bForce);
void MultiCheckForEntropyWinner (void);
void MultiSendSoundFunction (char, char);
void ProcessSmartMinesFrame (void);
void DoSeismicStuff (void);
void ReadControls (void);		// located in gamecntl.c
void DoFinalBossFrame (void);

void MultiRemoveGhostShips (void);

void GameRenderFrame (void);
void OmegaChargeFrame (void);
void FlickerLights (void);

void DoLunacyOn (void);
void DoLunacyOff (void);

extern char OldHomingState [20];
extern char old_IntMethod;

jmp_buf gameExitPoint;

//------------------------------------------------------------------------------

tDetailData	detailData = {
 {15, 31, 63, 127, 255},
 { 1,  2,  3,   5,   8},
 { 3,  5,  7,  10,  50},
 { 1,  2,  3,   7,  20},
 { 2,  4,  7,  10,  15},
 { 2,  4,  7,  10,  15},
 { 2,  4,  8,  16,  50},
 { 2, 16,  32, 64, 128}};

//------------------------------------------------------------------------------

void GameFlushInputs (void)
{
	int32_t dx, dy;

#if 1
controls.FlushInput ();
#else
KeyFlush ();
JoyFlush ();
MouseFlush ();
#endif
#ifdef MACINTOSH
if ((gameStates.app.nFunctionMode != FMODE_MENU) && !joydefs_calibrating)		// only reset mouse when not in menu or not calibrating
#endif
MouseGetDelta (&dx, &dy);	// Read mouse
controls.Reset ();
}

//------------------------------------------------------------------------------

void MovePlayerToSegment (CSegment* pSeg, int32_t nSide)
{
	CFixVector vp;

gameData.objData.pConsole->info.position.vPos = pSeg->Center ();
vp = pSeg->SideCenter (nSide) - gameData.objData.pConsole->info.position.vPos;
/*
gameData.objData.pConsole->info.position.mOrient = CFixMatrix::Create(vp, NULL, NULL);
*/
gameData.objData.pConsole->info.position.mOrient = CFixMatrix::CreateF (vp);
gameData.objData.pConsole->RelinkToSeg (pSeg->Index ());
}

//------------------------------------------------------------------------------
//this is called once per game

bool InitGame (int32_t nSegments, int32_t nVertices)
{
if (!gameData.Create (nSegments, nVertices))
	return false;
/*---*/PrintLog (1, "Initializing game data\n");
PrintLog (1, "Objects ...\n");
InitObjects ();
PrintLog (-1);
/*---*/PrintLog (1, "Special effects...\n");
InitSpecialEffects ();
PrintLog (-1);
/*---*/PrintLog (1, "AI system...\n");
InitAISystem ();
PrintLog (-1);
//*---*/PrintLog (1, "gauge canvases...\n");
//	InitGaugeCanvases ();
/*---*/PrintLog (1, "Exploding walls data...\n");
InitExplodingWalls ();
PrintLog (-1);
ResetGenerators ();
/*---*/PrintLog (1, "Background bitmap...\n");
LoadGameBackground ();
PrintLog (-1);
/*---*/PrintLog (1, "Automap...\n");
automap.Init ();
PrintLog (-1);
/*---*/PrintLog (1, "Default ship data...\n");
InitDefaultShipProps ();
PrintLog (-1);
nClearWindow = 2;		//	do portal only window clear.
PrintLog (-1);
/*---*/PrintLog (1, "Detail levels (%d)...\n", gameStates.app.nDetailLevel);
gameStates.app.nDetailLevel = InitDetailLevels (gameStates.app.nDetailLevel);
importantMessages [0].Destroy ();
importantMessages [1].Destroy ();
PrintLog (-1);
PrintLog (-1);
fpDrawTexPolyMulti = G3DrawTexPolyMulti;
return true;
}

//------------------------------------------------------------------------------

// Sets up the canvases we will be rendering to (NORMAL VERSION)
void GameInitRenderBuffers (int32_t nScreenSize, int32_t render_w, int32_t render_h, int32_t render_method, int32_t flags)
{
gameData.render.screen.Set (nScreenSize);
SetupCanvasses ();
}

//------------------------------------------------------------------------------

//initialize flying
void FlyInit (CObject *pObj)
{
pObj->info.controlType = CT_FLYING;
pObj->info.movementType = MT_PHYSICS;
pObj->mType.physInfo.velocity.SetZero ();
pObj->mType.physInfo.thrust.SetZero ();
pObj->mType.physInfo.rotVel.SetZero ();
pObj->mType.physInfo.rotThrust.SetZero ();
}

//void morph_test (), morph_step ();


//	------------------------------------------------------------------------------------

void DoCloakStuff (void)
{
for (int32_t i = 0; i < N_PLAYERS; i++) {
	if (gameData.multiplayer.players[i].flags & PLAYER_FLAGS_CLOAKED) {
		if (gameData.time.xGame - PLAYER (i).cloakTime > CLOAK_TIME_MAX) {
			gameData.multiplayer.players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
			if (i == N_LOCALPLAYER) {
				audio.PlaySound (SOUND_CLOAK_OFF);
				if (IsMultiGame)
					MultiSendPlaySound (SOUND_CLOAK_OFF, I2X (1));
				MaybeDropNetPowerup (-1, POW_CLOAK, FORCE_DROP);
				MultiSendDeCloak (); // For demo recording
				}
			}
		}
	}
}

//	------------------------------------------------------------------------------------

int32_t bFakingInvul = 0;

void DoInvulnerableStuff (void)
{
if ((LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) &&
	 (LOCALPLAYER.invulnerableTime != 0x7fffffff)) {
	if (gameData.time.xGame - LOCALPLAYER.invulnerableTime > INVULNERABLE_TIME_MAX) {
		LOCALPLAYER.flags ^= PLAYER_FLAGS_INVULNERABLE;
		if (!bFakingInvul) {
			audio.PlaySound (SOUND_INVULNERABILITY_OFF);
			if (IsMultiGame) {
				MultiSendPlaySound (SOUND_INVULNERABILITY_OFF, I2X (1));
				MaybeDropNetPowerup (-1, POW_INVUL, FORCE_DROP);
				}
			}
		bFakingInvul = 0;
		}
	}
}

//@@//	------------------------------------------------------------------------------------
//@@void afterburner_shake (void)
//@@{
//@@	int32_t	rx, rz;
//@@
//@@	rx = (abScale * FixMul (SRandShort (), I2X (1)/8 + (((gameData.time.xGame + 0x4000)*4) & 0x3fff)))/16;
//@@	rz = (abScale * FixMul (SRandShort (), I2X (1)/2 + ((gameData.time.xGame*4) & 0xffff)))/16;
//@@
//@@	gameData.objData.pConsole->mType.physInfo.rotVel.x += rx;
//@@	gameData.objData.pConsole->mType.physInfo.rotVel.z += rz;
//@@
//@@}

//	------------------------------------------------------------------------------------

#define AFTERBURNER_LOOP_START	 ((gameOpts->sound.audioSampleRate==SAMPLE_RATE_22K)?32027: (32027/2))		//20098
#define AFTERBURNER_LOOP_END		 ((gameOpts->sound.audioSampleRate==SAMPLE_RATE_22K)?48452: (48452/2))		//25776

//static int32_t abScale = 4;

void DoAfterburnerStuff (void)
{
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	gameData.physics.xAfterburnerCharge = 0;
if (gameStates.app.bEndLevelSequence || gameStates.app.bPlayerIsDead) {
	if (audio.DestroyObjectSound (LOCALPLAYER.nObject))
		MultiSendSoundFunction (0,0);
	}
else /*if ((gameStates.gameplay.xLastAfterburnerCharge && (controls [0].afterburnerState != gameStates.gameplay.bLastAfterburnerState)) || 
	 		(gameStates.gameplay.bLastAfterburnerState && (gameStates.gameplay.xLastAfterburnerCharge && !gameData.physics.xAfterburnerCharge)))*/ {
	int32_t nSoundObj = audio.FindObjectSound (LOCALPLAYER.nObject, SOUND_AFTERBURNER_IGNITE);
	if (gameData.physics.xAfterburnerCharge && controls [0].afterburnerState && (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER)) {
		if (nSoundObj < 0) {
			audio.CreateObjectSound ((int16_t) SOUND_AFTERBURNER_IGNITE, SOUNDCLASS_PLAYER, (int16_t) LOCALPLAYER.nObject, 
											 1, I2X (1), I2X (256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
			if (IsMultiGame)
				MultiSendSoundFunction (3, (char) SOUND_AFTERBURNER_IGNITE);
			}
		}
	else if (nSoundObj >= 0) { //gameStates.gameplay.xLastAfterburnerCharge || gameStates.gameplay.bLastAfterburnerState) {
#if 1
		audio.DeleteSoundObject (nSoundObj);
		audio.CreateObjectSound ((int16_t) SOUND_AFTERBURNER_PLAY, SOUNDCLASS_PLAYER, (int16_t) LOCALPLAYER.nObject);
		if (IsMultiGame) {
			if (gameStates.multi.nGameType == UDP_GAME)
			 	MultiSendSoundFunction (3, (char) SOUND_AFTERBURNER_PLAY);
		 	MultiSendSoundFunction (0, 0);
			}
#endif
		}
	}
gameStates.gameplay.bLastAfterburnerState = controls [0].afterburnerState;
gameStates.gameplay.xLastAfterburnerCharge = gameData.physics.xAfterburnerCharge;
}

//------------------------------------------------------------------------------

void ShowHelp (void)
{
	CMenu	m (40);
#ifdef MACINTOSH
	char command_help[64], pixel_double_help[64], save_help[64], restore_help[64];
#endif

m.AddText ("", TXT_HELP_ESC);
#ifndef MACINTOSH
m.AddText ("", TXT_HELP_ALT_F2);
m.AddText ("", TXT_HELP_ALT_F3);
#else
sprintf (save_help, TXT_HLP_SAVE, 133);
sprintf (restore_help, TXT_HLP_LOAD, 133);
mat.AddText (save_help);
mat.AddText (restore_help);
#endif
m.AddText ("", TXT_HELP_QUICKSAVE);
m.AddText ("", TXT_HELP_QUICKLOAD);
m.AddText ("", TXT_HELP_F2);
m.AddText ("", TXT_HELP_F3);
m.AddText ("", TXT_HELP_F4);
m.AddText ("", TXT_HELP_F5);
#ifndef MACINTOSH
m.AddText ("", TXT_HELP_PAUSE_D2X);
#else
mat.AddText (TXT_HLP_PAUSE);
#endif
m.AddText ("", TXT_HELP_MINUSPLUS);
#ifndef MACINTOSH
m.AddText ("", TXT_HELP_PRTSCN_D2X);
#else
mat.AddText (TXT_HLP_SCREENIE);
#endif
m.AddText ("", TXT_HELP_1TO5);
m.AddText ("", TXT_HELP_6TO10);
m.AddText ("", TXT_HLP_CYCLE_LEFT_WIN);
m.AddText ("", TXT_HLP_CYCLE_RIGHT_WIN);
if (!gameStates.app.bNostalgia) {
	m.AddText ("", TXT_HLP_RESIZE_WIN);
	m.AddText ("", TXT_HLP_REPOS_WIN);
	m.AddText ("", TXT_HLP_ZOOM_WIN);
	}
m.AddText ("", TXT_HLP_GUIDEBOT);
m.AddText ("", TXT_HLP_RENAMEGB);
m.AddText ("", TXT_HLP_DROP_PRIM);
m.AddText ("", TXT_HLP_DROP_SEC);
if (!gameStates.app.bNostalgia) {
	m.AddText ("", TXT_HLP_CHASECAM);
	m.AddText ("", TXT_HLP_RADAR);
	}
m.AddText ("", TXT_HLP_GBCMDS);
#ifdef MACINTOSH
sprintf (pixel_double_help, "%c-D\t  Toggle Pixel Double Mode", 133);
mat.AddText (pixel_double_help);
mat.AddText ("");
sprintf (command_help, " (Use %c-# for F#. i.e. %c-1 for F1)", 133, 133);
mat.AddText (command_help);
#endif
//paletteManager.SuspendEffect ();
m.TinyMenu (NULL, TXT_KEYS);
//paletteManager.ResumeEffect ();
}

//------------------------------------------------------------------------------

//turns off active cheats
void TurnCheatsOff (void)
{
	int32_t i;

if (gameStates.app.cheats.bHomingWeapons)
	for (i = 0; i < 20; i++)
		WI_set_homingFlag (i, OldHomingState [i]);

if (gameStates.app.cheats.bAcid) {
	gameStates.app.cheats.bAcid = 0;
	gameStates.render.nInterpolationMethod = old_IntMethod;
	}
gameStates.app.cheats.bMadBuddy = 0;
gameStates.app.cheats.bBouncingWeapons = 0;
gameStates.app.cheats.bHomingWeapons=0;
DoLunacyOff ();
gameStates.app.cheats.bLaserRapidFire = 0;
gameStates.app.cheats.bPhysics = 0;
gameStates.app.cheats.bMonsterMode = 0;
gameStates.app.cheats.bRobotsKillRobots = 0;
gameStates.app.cheats.bRobotsFiring = 1;
}

// ----------------------------------------------------------------------------
//turns off all cheats & resets cheater flag
void GameDisableCheats ()
{
TurnCheatsOff ();
gameStates.app.cheats.bEnabled = 0;
}

// ----------------------------------------------------------------------------
//	GameSetup ()

void CleanupAfterGame (bool bHaveLevel)
{
#ifdef MWPROFILE
ProfilerSetStatus (0);
#endif
DestroyEffectsThread ();
networkThread.Stop ();
importantMessages [0].Destroy ();
importantMessages [1].Destroy ();
songManager.DestroyPlaylists ();
gameData.time.xGameTotal = (SDL_GetTicks () - gameData.time.xGameStart) / 1000;
gameStates.render.bRenderIndirect = 0;
G3EndFrame (transformation, 0);
audio.StopAll ();
if (gameStates.sound.bD1Sound) {
	gameStates.sound.bD1Sound = 0;
	//audio.Shutdown ();
	//audio.Setup ();
	}
if ((gameData.demo.nState == ND_STATE_RECORDING) || (gameData.demo.nState == ND_STATE_PAUSED))
	NDStopRecording ();
if (bHaveLevel)
	MultiLeaveGame ();
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	NDStopPlayback ();
gameData.Destroy ();
postProcessManager.Destroy ();
transparencyRenderer.ResetBuffers ();
CGenericCockpit::Rewind (false);
ClearWarnFunc (ShowInGameWarning);     //don't use this func anymore
StopPlayerMovement ();
GameDisableCheats ();
UnloadCamBot ();
#ifdef APPLE_DEMO
SetFunctionMode (FMODE_EXIT);		// get out of game in Apple OEM version
#endif
if (pfnTIRStop)
	pfnTIRStop ();
shaderManager.Destroy (true);
ogl.InitEnhanced3DShader (); // always need the stereo rendering shaders 
meshBuilder.DestroyVBOs ();
UnloadLevelData ();
missionConfig.Init ();
PiggyCloseFile ();
SavePlayerProfile ();
ResetModFolders ();
gameStates.app.bGameRunning = 0;
backgroundManager.Rebuild ();
SetFunctionMode (FMODE_MENU);	
}

//	-----------------------------------------------------------------------------

#if PHYSICS_FPS >= 0

int32_t GameFrame (int32_t bRenderFrame, int32_t bReadControls, int32_t fps)
{
if (bRenderFrame)
	DoRenderFrame ();

	fix		xFrameTime = gameData.time.xFrame; 
	float		physicsFrameTime = 1000.0f / float (PHYSICS_FPS);
	float		t = float (SDL_GetTicks ()), dt = t - gameData.physics.fLastTick;
	int32_t		h = 1, i = 0;

while (dt >= physicsFrameTime) {
	dt -= physicsFrameTime;
	i++;
	}
if (i) {
	gameData.physics.fLastTick = t - dt;
	gameData.time.SetTime (I2X (1) / PHYSICS_FPS);
	gameData.physics.xTime = gameData.time.xFrame; 
	gameStates.app.nSDLTicks [0] = gameStates.app.nSDLTicks [1];
	if (bReadControls > 0)
		ReadControls ();
	else if (bReadControls < 0)
		controls.Reset ();
	gameStates.app.tick40fps.bTick = 1;
	while (i--) {
		HUDMessage (0, "physics tick %d", i);
		gameData.objData.nFrameCount++;
		h = DoGameFrame (0, 0, 0);
		controls.ResetTriggers ();
		gameStates.app.tick40fps.bTick = 0;
		if (0 >= h)
			return h;
		gameStates.input.bSkipControls = 1;
		}
	gameStates.app.tick40fps.bTick = 1;
	DoEffectsFrame ();
	CalcFrameTime ();
	gameStates.app.nSDLTicks [1] = gameStates.app.nSDLTicks [0];
	}
else {
	CalcFrameTime ();
	}
gameStates.input.bSkipControls = 0;
gameStates.app.tick40fps.bTick = 0;
return h;
}

#endif

//	------------------------------------------------------------------------------------
//	------------------------------------------------------------------------------------
//	------------------------------------------------------------------------------------
//this function is the game.  called when game mode selected.  runs until
//editor mode or exit selected

extern bool bRegisterBitmaps;

//-----------------------------------------------------------------------------
//called at the end of the program

void _CDECL_ CloseGame (void)
{
	static	int32_t bGameClosed = 0;
#if MULTI_THREADED
	int32_t		i;
#endif
if (bGameClosed)
	return;
bGameClosed = 1;
if (gameStates.app.bMultiThreaded) {
	DestroyRenderThreads ();
	}
GrClose ();
PrintLog (1, "unloading addon sounds\n");
FreeAddonSounds ();
if (!StartSoundThread (stCloseAudio))
	audio.Shutdown ();
DestroySoundThread ();
PrintLog (-1);
RLECacheClose ();
FreeObjExtensionBitmaps ();
FreeModelExtensions ();
PrintLog (1, "unloading render buffers\n");
transparencyRenderer.FreeBuffers ();
PrintLog (-1);	
PrintLog (1, "unloading string pool\n");
FreeStringPool ();
PrintLog (-1);
PrintLog (1, "unloading level messages\n");
FreeTextData (gameData.messages);
FreeTextData (&gameData.sounds);
PrintLog (-1);
PrintLog (1, "unloading hires animations\n");
UnloadHiresAnimations ();
UnloadTextures ();
PrintLog (-1);
PrintLog (1, "freeing sound buffers\n");
FreeSoundReplacements ();
PrintLog (-1);
PrintLog (1, "unloading auxiliary poly model data\n");
gameData.models.Destroy ();
PrintLog (-1);
PrintLog (1, "unloading hires models\n");
FreeHiresModels (0);
PrintLog (-1);
PrintLog (1, "unloading tracker list\n");
tracker.DestroyList ();
PrintLog (-1);
PrintLog (1, "unloading lightmap data\n");
lightmapManager.Destroy ();
PrintLog (-1);
PrintLog (1, "unloading particle data\n");
particleManager.Shutdown ();
PrintLog (-1);
PrintLog (1, "unloading shield sphere data\n");
gameData.render.shield.Destroy ();
gameData.render.monsterball.Destroy ();
PrintLog (-1);
PrintLog (1, "unloading HUD icons\n");
hudIcons.Destroy ();
PrintLog (-1);
PrintLog (1, "unloading extra texture data\n");
UnloadAddonImages ();
PrintLog (-1);
PrintLog (1, "unloading palettes\n");
gameData.segData.skybox.Destroy ();
PrintLog (-1);
#if GPGPU_VERTEX_LIGHTING
gpgpuLighting.End ();
#endif
PrintLog (1, "restoring effect bitmaps\n");
RestoreEffectBitmapIcons ();
PrintLog (-1);
if (bmBackground.Buffer ()) {
	PrintLog (1, "unloading background bitmap\n");
	bmBackground.DestroyBuffer ();
	PrintLog (-1);
	}
ClearWarnFunc (ShowInGameWarning);     //don't use this func anymore
PrintLog (1, "unloading ban list\n");
banList.Save ();
banList.Destroy ();
PrintLog (-1);
#if 1 //defined(__unix__)
if (ogl.m_states.bFullScreen)
	ogl.ToggleFullScreen ();
#endif
//PrintLog (1, "peak memory consumption: %ld bytes\n", nMaxAllocd);
#if 0 //!defined(__unix__)
SDL_Quit (); // hangs on Linux
#endif
#if 0
if (fLog) {
	fclose (fLog);
	fLog = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------

CCanvas* CurrentGameScreen (void)
{
return &gameData.render.scene;
}

//-----------------------------------------------------------------------------
//returns ptr to escort robot, or NULL

CObject *FindEscort ()
{
	CObject	*pObj = OBJECTS.Buffer ();

FORALL_ROBOT_OBJS (pObj)
	if (pObj->IsGuideBot ())
		return pObj;
return NULL;
}

//-----------------------------------------------------------------------------
//if water or fire level, make occasional sound
void DoAmbientSounds (void)
{
if (gameStates.app.bPlayerIsDead)
	return;

CSegment* pSeg = SEGMENT (gameData.objData.pConsole->info.nSegment);
if (!pSeg)
	return;

	int32_t bLava = (pSeg->m_flags & S2F_AMBIENT_LAVA);
	int32_t bWater = (pSeg->m_flags & S2F_AMBIENT_WATER);
	int16_t nSound;

if (bLava) {							//has lava
	nSound = SOUND_AMBIENT_LAVA;
	if (bWater && (RandShort () & 1))	//both, pick one
		nSound = SOUND_AMBIENT_WATER;
	}
else if (bWater)						//just water
	nSound = SOUND_AMBIENT_WATER;
else
	return;
if (((RandShort () << 3) < gameData.time.xFrame))	//play the nSound
	audio.PlaySound (nSound, SOUNDCLASS_GENERIC, (fix) (RandShort () + I2X (1) / 2));
}

//-----------------------------------------------------------------------------

void DoEffectsFrame (void)
{
gameStates.render.bUpdateEffects = true;
UpdateEffects ();
}

//-----------------------------------------------------------------------------

void UpdateEffects (void) 
{
if (gameStates.render.bUpdateEffects) {
	gameStates.render.bUpdateEffects = 0;
	wayPointManager.Update ();
	lightningManager.DoFrame ();
	sparkManager.DoFrame ();
	DoParticleFrame ();
	particleManager.Cleanup ();
	}
}

//------------------------------------------------------------------------------

void SetFunctionMode (int32_t newFuncMode)
{
if (gameStates.app.nFunctionMode != newFuncMode) {
	gameStates.app.nLastFuncMode = gameStates.app.nFunctionMode;
	if ((gameStates.app.nFunctionMode = newFuncMode) == FMODE_GAME)
		gameStates.app.bEnterGame = 2;
#if DBG
	else if (newFuncMode == FMODE_MENU)
		gameStates.app.bEnterGame = 0;
#endif
	}
}

//	------------------------------------------------------------------------------------
//	------------------------------------------------------------------------------------
//	------------------------------------------------------------------------------------

#if DBG
static int32_t nRenderCalls = 0;
static int32_t nStateCalls = 0;
static int32_t tStateLoop = 0;
static int32_t tRenderLoop = 0;
#endif

class CGameLoop {
	private:
		SDL_Thread*	m_thread;
		SDL_sem*		m_lock;
		bool			m_bRunning;
		int32_t		m_bRender;
		int32_t		m_bControls;
		int32_t		m_fps;
		int32_t		m_nResult;

	public:
		CGameLoop () : m_thread (NULL), m_lock (NULL), m_bRunning (false), m_bRender (0), m_bControls (0), m_fps (0), m_nResult (0) {}
		void Start (void);
		void Stop (void);
		void Run (void);
		void StateLoop (void);
		void Setup (void);
		void Cleanup (void);
		int32_t Step (int32_t bRenderFrame = 1, int32_t bReadControls = 1, int32_t fps = 0);

	private:
		int32_t Preprocess (void);
		int32_t Postprocess (void);
		void Render (void);
		void HandleQuit (void);
		void HandleAutomap (void);
		void HandleControls (int32_t bControls);
		inline int32_t SetResult (int32_t nResult) { return m_nResult = nResult; }

		void Lock (void) { 
			if (m_lock)
				SDL_SemWait (m_lock); 
			}
		void Unlock (void) { 
			if (m_lock)
				SDL_SemPost (m_lock); 
			}
};

CGameLoop gameLoop;

//	-----------------------------------------------------------------------------

int32_t _CDECL_ GameThreadHandler (void* nThreadP)
{
gameLoop.StateLoop ();
return 0;
}

//-----------------------------------------------------------------------------

void CGameLoop::StateLoop (void)
{
m_bRunning = true;
while (m_bRunning) {
#if DBG
	++nStateCalls;
#endif
	int32_t playerShield = LOCALPLAYER.Shield ();

	CalcFrameTime (120);

	Lock ();
#if DBG
	int32_t t = SDL_GetTicks ();
#endif
	m_nResult = Preprocess ();
#if DBG
	tStateLoop += SDL_GetTicks () - t;
#endif
	Unlock ();
	if (1 > m_nResult)
		break;
	Lock ();
#if DBG
	t = SDL_GetTicks ();
#endif
	m_nResult = Postprocess ();
#if DBG
	tStateLoop += SDL_GetTicks () - t;
#endif
	Unlock ();
	if (1 > m_nResult)
		break;

	if (LOCALPLAYER.Shield () != playerShield)
		ReleaseGuidedMissile (N_LOCALPLAYER);
#if DBG
	tStateLoop += SDL_GetTicks () - t;
#endif
	}
m_bRunning = false;
}

//-----------------------------------------------------------------------------

void CGameLoop::Render (void)
{
Lock ();
#if DBG
++nRenderCalls;
#endif
gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;

if (gameStates.render.cockpit.bRedraw) {
	cockpit->Init ();
	gameStates.render.cockpit.bRedraw = 0;
	}
GameRenderFrame ();
gameStates.app.bUsingConverter = 0;
AutoScreenshot ();

Unlock ();
}

//-----------------------------------------------------------------------------

void CGameLoop::Setup (void)
{
DoLunacyOn ();		//	Copy values for insane into copy buffer in ai.c
DoLunacyOff ();	//	Restore true insane mode.
gameStates.app.bGameAborted = 0;
gameStates.app.bEndLevelSequence = 0;
paletteManager.ResetEffect ();
SetScreenMode (SCREEN_GAME);
SetWarnFunc (ShowInGameWarning);
cockpit->Init ();
gameStates.input.keys.bRepeat = 1;                // Do allow repeat in game
gameData.objData.pViewer = gameData.objData.pConsole;
FlyInit (gameData.objData.pConsole);
if (gameStates.app.bGameSuspended & SUSP_TEMPORARY)
	gameStates.app.bGameSuspended &= ~(SUSP_ROBOTS | SUSP_TEMPORARY);
ResetTime ();
gameData.time.SetTime (0);			//make first frame zero
GameFlushInputs ();
lightManager.SetMethod ();
gameStates.app.nSDLTicks [0] = 
gameStates.app.nSDLTicks [1] = 
gameData.time.xGameStart = SDL_GetTicks ();
gameData.physics.fLastTick = float (gameData.time.xGameStart);
ogl.m_features.bShaders.Available (gameOpts->render.bUseShaders);
gameData.render.rift.SetCenter ();
if (pfnTIRStart)
	pfnTIRStart ();
}

//	------------------------------------------------------------------------------------

int32_t CGameLoop::Preprocess (void)
{
PROF_START
//gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
gameData.objData.nLastObject [1] = gameData.objData.nLastObject [0];

if (gameStates.gameplay.bMineMineCheat) {
	DoWowieCheat (0, 0);
	GasolineCheat (0);
	}

UpdatePlayerStats ();
UpdatePlayerWeaponInfo ();
paletteManager.FadeEffect ();		//	Should leave palette effect up for as long as possible by putting right before render.
DoAfterburnerStuff ();
DoCloakStuff ();
DoInvulnerableStuff ();
RemoveObsoleteStuckObjects ();
InitAIFrame ();
DoFinalBossFrame ();
DrainHeadpLightower ();
PROF_END(ptGameStates)

if (IsMultiGame) {
	if (!MultiProtectGame ()) {
		SetFunctionMode (FMODE_MENU);
		return -1;
		}
	PROF_CONT
	AutoBalanceTeams ();
	MultiSendTyping ();
	MultiSyncKills ();
	MultiAdjustPowerups ();
	tracker.AddServer ();
	MultiDoFrame ();
	CheckMonsterballScore ();
	MultiCheckForScoreGoalWinner (netGameInfo.GetPlayTimeAllowed () && (gameStates.app.xThisLevelTime >= I2X ((netGameInfo.GetPlayTimeAllowed () * 5 * 60))));
	MultiCheckForEntropyWinner ();
	MultiRemoveGhostShips ();
	PROF_END (ptGameStates)
	}

#if PHYSICS_FPS < 0
PROF_CONT
DoEffectsFrame ();
UpdatePlayerEffects ();
PROF_END(ptGameStates)
#endif

return 1;
}

//-----------------------------------------------------------------------------

int32_t CGameLoop::Postprocess (void)
{
PROF_START

DeadPlayerFrame ();
ObserverFrame ();
LOCALPLAYER.Recharge ();
if (gameData.demo.nState != ND_STATE_PLAYBACK) 
	DoReactorDeadFrame ();
ProcessSmartMinesFrame ();
DoSeismicStuff ();
DoAmbientSounds ();
DropPowerups ();
gameData.time.xGame += gameData.time.xFrame;
if ((gameData.time.xGame < 0) || (gameData.time.xGame > I2X (0x7fff - 600))) 
	gameData.time.xGame = gameData.time.xFrame;	//wrap when goes negative, or gets within 10 minutes
if (IsMultiGame && netGameInfo.GetPlayTimeAllowed ())
	gameStates.app.xThisLevelTime += gameData.time.xFrame;
audio.SyncSounds ();
PROF_END (ptGameStates)

if (gameStates.app.bEndLevelSequence) {
	DoEndLevelFrame ();
	PowerupGrabCheatAll ();
	DoSpecialEffects ();
	return 1;					//skip everything else
	}

PROF_CONT
if (gameData.demo.nState != ND_STATE_PLAYBACK) 
	DoExplodingWallFrame ();
if ((gameData.demo.nState != ND_STATE_PLAYBACK) || (gameData.demo.nVcrState != ND_STATE_PAUSED)) {
	DoSpecialEffects ();
	WallFrameProcess ();
	TriggersFrameProcess ();
	}
if (gameData.reactor.bDestroyed && (gameData.demo.nState == ND_STATE_RECORDING)) 
	NDRecordControlCenterDestroyed ();
UpdateFlagClips ();
MultiSetFlagPos ();
UpdatePlayerPaths ();
FlashFrame ();
PROF_END (ptGameStates)

if (gameData.demo.nState == ND_STATE_PLAYBACK) {
	NDPlayBackOneFrame ();
	if (gameData.demo.nState != ND_STATE_PLAYBACK)
		longjmp (gameExitPoint, 0);		// Go back to menu
	}
else { // Note the link to above!
	PROF_CONT
	LOCALPLAYER.homingObjectDist = -1;		//	Assume not being tracked.  CObject::UpdateWeapon modifies this.
	if (!UpdateAllObjects ())
		return 0;
	PowerupGrabCheatAll ();
	if (gameStates.app.bEndLevelSequence)	//might have been started during move
		return 1;
	UpdateAllProducers ();
	DoAIFrameAll ();
	if (AllowedToFireGun ()) 
		FireGun ();				// Fire Laser!
	if (!FusionBump ())
		return 1;
	if (gameData.laser.nGlobalFiringCount)
		gameData.laser.nGlobalFiringCount -= LocalPlayerFireGun ();	
	if (gameData.laser.nGlobalFiringCount < 0)
		gameData.laser.nGlobalFiringCount = 0;
	PROF_END (ptGameStates)
	}

PROF_CONT
if (gameStates.render.bDoAppearanceEffect) {
	gameData.objData.pConsole->CreateAppearanceEffect ();
	gameStates.render.bDoAppearanceEffect = 0;
	if (IsMultiGame && netGameInfo.m_info.invul) {
		LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
		LOCALPLAYER.invulnerableTime = gameData.time.xGame - I2X (27);
		bFakingInvul = 1;
		SetupSpherePulse (gameData.multiplayer.spherePulse + N_LOCALPLAYER, 0.02f, 0.5f);
		}
	}

DoSlowMotionFrame ();
CheckInventory ();
OmegaChargeFrame ();
SlideTextures ();
FlickerLights ();
PROF_END (ptGameStates)
return 1;
}

//-----------------------------------------------------------------------------

void CGameLoop::HandleControls (int32_t bControls)
{
if (bControls < 0)
	controls.Reset ();
else if (bControls)
	ReadControls ();
}

//-----------------------------------------------------------------------------

extern int32_t MaxFPS (void);

int32_t CGameLoop::Step (int32_t bRender, int32_t bControls, int32_t fps)
{
gameStates.app.bGameRunning = 1;

if (m_bRunning) { // game states are updated in separate thread
	int32_t t = fps ? SDL_GetTicks () : 0;
	HandleControls (bControls);
	if (!bRender)
		return 0;
	Render ();
#if DBG
	tRenderLoop += SDL_GetTicks () - t;
#endif
	if (fps) {
		t += 1000 / fps - SDL_GetTicks ();
		if (t > 0)
			G3_SLEEP (t);
		}	
	}
else {
	CalcFrameTime (fps);
	HandleControls (bControls);
	gameStates.render.EnableCartoonStyle (1, 1, 1);
	m_nResult = Preprocess ();
	gameStates.render.DisableCartoonStyle ();
	if (0 > m_nResult)
		return m_nResult;
	if (bRender)
		Render ();
	gameStates.render.EnableCartoonStyle (1, 1, 1);
	m_nResult = Postprocess ();
	gameStates.render.DisableCartoonStyle ();
	if (0 > m_nResult)
		return m_nResult;
	}

return m_nResult;
}

//------------------------------------------------------------------------------

void CGameLoop::HandleQuit (void)
{
if (gameStates.app.bConfigMenu)
	ConfigMenu ();

if ((gameStates.app.nFunctionMode != FMODE_GAME) &&
		gameData.demo.bAuto && !gameOpts->demo.bRevertFormat &&
		(gameData.demo.nState != ND_STATE_NORMAL)) {
	int32_t choice, fmode;
	fmode = gameStates.app.nFunctionMode;
	SetFunctionMode (FMODE_GAME);
	choice = InfoBox (NULL,NULL,  BG_STANDARD, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO);
	SetFunctionMode (fmode);
	if (choice)
		SetFunctionMode (FMODE_GAME);
	else {
		gameData.demo.bAuto = 0;
		NDStopPlayback ();
		SetFunctionMode (FMODE_MENU);
		}
	}

if ((gameStates.app.nFunctionMode != FMODE_GAME) &&
		(gameData.demo.nState != ND_STATE_PLAYBACK) &&
		(gameStates.app.nFunctionMode != FMODE_EDITOR) &&
		!gameStates.multi.bIWasKicked) {
	if (QuitSaveLoadMenu ())
		SetFunctionMode (FMODE_GAME);
	}
}

//------------------------------------------------------------------------------

void CGameLoop::Start (void)
{
#if PHYSICS_IN_BACKGROUND
m_bRunning = false;
m_lock = SDL_CreateSemaphore (1);
m_thread = SDL_CreateThread (GameThreadHandler, NULL);
while (!m_bRunning)
	G3_SLEEP (0);
#endif
}

//------------------------------------------------------------------------------

void CGameLoop::Stop (void)
{
m_bRunning = false;
if (m_thread) {
	SDL_WaitThread (m_thread, &m_nResult);
	m_thread = NULL;
	}
if (m_lock) {
	SDL_DestroySemaphore (m_lock);
	m_lock = NULL;
	}
}

//------------------------------------------------------------------------------

void CGameLoop::HandleAutomap (void)
{
if (automap.Active ()) {
	int32_t	save_w = gameData.render.frame.Width (),
				save_h = gameData.render.frame.Height ();
	automap.DoFrame (0, 0);
	gameStates.app.bEnterGame = 1;
	gameStates.video.nScreenMode = -1;
	SetScreenMode (SCREEN_GAME);
	gameData.render.frame.SetWidth (save_w);
	gameData.render.frame.SetHeight (save_h);
	cockpit->Init ();
	}
}

//------------------------------------------------------------------------------

void CGameLoop::Run (void)
{
Setup ();								// Replaces what was here earlier.

for (;;) {
	PROF_TOGGLE
	PROF_START
	// GAME LOOP!
#if DBG
	if (automap.Active ())
#endif
	automap.SetActive (0);
	gameStates.app.bConfigMenu = 0;
	gameStates.app.nExtGameStatus = GAMESTAT_RUNNING;

	try {
		Step (1, 1, MaxFPS ());
		}
	catch (int32_t e) {
		ClearWarnFunc (ShowInGameWarning);
		if (e == EX_OUT_OF_MEMORY) {
			Warning ("Out of memory.");
			break;
			}
		else if (e == EX_IO_ERROR) {
			Warning ("Couldn't load game data.");
			break;
			}
		else {
			Warning ("Well ... something went wrong.");
			break;
			}
		}
	catch (...) {
		ClearWarnFunc (ShowInGameWarning);
		Warning ("Well ... something went really wrong.");
		break;
		}

	PROF_END (ptFrame);
	if (m_nResult < 0)
		break;
	if (m_nResult == 0)
		continue;

	PROF_CONT
	if (gameStates.app.bSingleStep) {
		char c;
		while (!(c = KeyInKey ()))
			;
		if (c == KEY_ALTED + KEY_CTRLED + KEY_ESC)
			gameStates.app.bSingleStep = 0;
		}

	redbook.CheckRepeat ();	// Handle RedBook Audio Repeating.
	HandleAutomap ();
	HandleQuit ();
	gameStates.multi.bIWasKicked = 0;
	PROF_END(ptFrame);
	if (gameStates.app.nFunctionMode != FMODE_GAME)
		break; 
	}

Stop ();

try {
	CleanupAfterGame ();
	}
catch (...) {
	Warning ("Internal error when cleaning up.");
	}
bRegisterBitmaps = false;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void RunGame (void)
{
gameLoop.Setup ();
gameLoop.Start ();
if (!setjmp (gameExitPoint))
	gameLoop.Run ();
gameLoop.Stop ();
}

//------------------------------------------------------------------------------

int32_t GameFrame (int32_t bRenderFrame, int32_t bReadControls, int32_t fps)
{
return gameLoop.Step (bRenderFrame, bReadControls, fps);
}

//------------------------------------------------------------------------------

void ComputeSlideSegs (void)
{
	int32_t		nSegment, nSide, bIsSlideSeg, nTexture;
	CSegment*	pSeg = SEGMENTS.Buffer ();

gameData.segData.nSlideSegs = 0;
for (nSegment = 0; nSegment <= gameData.segData.nLastSegment; nSegment++, pSeg++) {
	bIsSlideSeg = 0;
	for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
		if (!pSeg->Side (nSide)->FaceCount ())
			continue;
		nTexture = pSeg->m_sides [nSide].m_nBaseTex;
		if (gameData.pig.tex.pTexMapInfo [nTexture].slide_u  || gameData.pig.tex.pTexMapInfo [nTexture].slide_v) {
			if (!bIsSlideSeg) {
				bIsSlideSeg = 1;
				gameData.segData.slideSegs [gameData.segData.nSlideSegs].nSegment = nSegment;
				gameData.segData.slideSegs [gameData.segData.nSlideSegs].nSides = 0;
				}
			gameData.segData.slideSegs [gameData.segData.nSlideSegs].nSides |= (1 << nSide);
			}
		}
	if (bIsSlideSeg)
		gameData.segData.nSlideSegs++;
	}
gameData.segData.bHaveSlideSegs = 1;
}

//	-----------------------------------------------------------------------------

void SlideTextures (void)
{
	int32_t			nSegment, nSide, h, i, j, tmn;
	uint8_t			sides;
	CSegment*	pSeg;
	CSide*		pSide;
	tUVL*			uvlP;
	fix			slideU, slideV, xDelta;

if (!gameData.segData.bHaveSlideSegs)
	ComputeSlideSegs ();
for (h = 0; h < gameData.segData.nSlideSegs; h++) {
	nSegment = gameData.segData.slideSegs [h].nSegment;
	pSeg = SEGMENT (nSegment);
	sides = gameData.segData.slideSegs [h].nSides;
	for (nSide = 0, pSide = pSeg->m_sides; nSide < SEGMENT_SIDE_COUNT; nSide++, pSide++) {
		if (!(pSeg->Side (nSide)->FaceCount () && (sides & (1 << nSide))))
			continue;
		tmn = pSide->m_nBaseTex;
		slideU = (fix) gameData.pig.tex.pTexMapInfo [tmn].slide_u;
		slideV = (fix) gameData.pig.tex.pTexMapInfo [tmn].slide_v;
		if (!(slideU || slideV))
			continue;
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				BRP;
#endif
		i = (pSeg->m_function == SEGMENT_FUNC_SKYBOX) ? 3 : 8;
		slideU = FixMul (gameData.time.xFrame, slideU << i);
		slideV = FixMul (gameData.time.xFrame, slideV << i);
		for (i = 0, uvlP = pSide->m_uvls; i < 4; i++) {
			uvlP [i].u += slideU;
			if (uvlP [i].u > I2X (2)) {
				xDelta = I2X (uvlP [i].u / I2X (1) - 1);
				for (j = 0; j < 4; j++)
					uvlP [j].u -= xDelta;
				}
			else if (uvlP [i].u < -I2X (2)) {
				xDelta = I2X (-uvlP [i].u / I2X (1) - 1);
				for (j = 0; j < 4; j++)
					uvlP [j].u += xDelta;
				}
			uvlP [i].v += slideV;
			if (uvlP [i].v > I2X (2)) {
				xDelta = I2X (uvlP [i].v / I2X (1) - 1);
				for (j = 0; j < 4; j++)
					uvlP [j].v -= xDelta;
				}
			else if (uvlP [i].v < -I2X (2)) {
				xDelta = I2X (-uvlP [i].v / I2X (1) - 1);
				for (j = 0; j < 4; j++)
					uvlP [j].v += xDelta;
				}
			}
		}
	}
}

//	-------------------------------------------------------------------------------------------------------
//	If player is close enough to nObject, which ought to be a powerup, pick it up!
//	This could easily be made difficulty level dependent.
void PowerupGrabCheat (CObject *pPlayer, int32_t nObject)
{
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return;

	CObject*					powerupP = OBJECT (nObject);
	tObjTransformation*	pPos = OBJPOS (pPlayer);
	CFixVector				vCollision;

Assert (powerupP->info.nType == OBJ_POWERUP);
if (powerupP->info.nFlags & OF_SHOULD_BE_DEAD)
	return;
if (CFixVector::Dist (powerupP->info.position.vPos, pPos->vPos) >=
	 2 * (pPlayer->info.xSize + powerupP->info.xSize) / (gameStates.app.bHaveExtraGameInfo [IsMultiGame] + 1))
	return;
vCollision = CFixVector::Avg (powerupP->info.position.vPos, pPos->vPos);
pPlayer->CollidePlayerAndPowerup (powerupP, vCollision);
}

//	-------------------------------------------------------------------------------------------------------
//	Make it easier to pick up powerups.
//	For all powerups in this CSegment, pick them up at up to twice pickuppable distance based on dot product
//	from CPlayerData to powerup and CPlayerData's forward vector.
//	This has the effect of picking them up more easily left/right and up/down, but not making them disappear
//	way before the player gets there.
void PowerupGrabCheatAll (void)
{
if (gameStates.app.tick40fps.bTick && (gameData.objData.pConsole->info.nSegment != -1)) {
	int16_t nObject = SEGMENT (gameData.objData.pConsole->info.nSegment)->m_objects;
	while (nObject != -1) {
		if (OBJECT (nObject)->info.nType == OBJ_POWERUP)
			PowerupGrabCheat (gameData.objData.pConsole, nObject);
		nObject = OBJECT (nObject)->info.nNextInSeg;
		}
	}
}

//	------------------------------------------------------------------------------------------------------------------
//	Create path for CPlayerData from current CSegment to goal CSegment.
//	Return true if path created, else return false.

int32_t nLastLevelPathCreated = -1;

int32_t MarkPlayerPathToSegment (int32_t nSegment)
{
	int32_t		i;
	CObject* pObj = gameData.objData.pConsole;
	int16_t		playerPathLength = 0;
	int32_t		playerHideIndex = -1;

if (nLastLevelPathCreated == missionManager.nCurrentLevel)
	return 0;
nLastLevelPathCreated = missionManager.nCurrentLevel;
if (CreatePathPoints (pObj, pObj->info.nSegment, nSegment, gameData.ai.freePointSegs, &playerPathLength, 100, 0, 0, -1) == -1) {
#if TRACE
	//console.printf (CON_DBG, "Unable to form path of length %i for myself\n", 100);
#endif
	return 0;
	}
playerHideIndex = int32_t (gameData.ai.routeSegs.Index (gameData.ai.freePointSegs));
gameData.ai.freePointSegs += playerPathLength;
if (int32_t (gameData.ai.routeSegs.Index (gameData.ai.freePointSegs)) + MAX_PATH_LENGTH * 2 > MAX_POINT_SEGS) {
#if TRACE
	//console.printf (1, "Can't create path.  Not enough tPointSegs.\n");
#endif
	AIResetAllPaths ();
	return 0;
	}
for (i = 1; i < playerPathLength; i++) {
	int16_t			nSegment, nObject;
	CFixVector	vSegCenter;
	CObject		*pObj;

	nSegment = gameData.ai.routeSegs [playerHideIndex + i].nSegment;
#if TRACE
	//console.printf (CON_DBG, "%3i ", nSegment);
#endif
	vSegCenter = gameData.ai.routeSegs[playerHideIndex+i].point;
	nObject = CreatePowerup (POW_ENERGY, -1, nSegment, vSegCenter, 1);
	if (nObject == -1) {
		Int3 ();		//	Unable to drop energy powerup for path
		return 1;
		}
	pObj = OBJECT (nObject);
	pObj->rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [pObj->info.nId].nClipIndex;
	pObj->rType.animationInfo.xFrameTime = gameData.effects.animations [0][pObj->rType.animationInfo.nClipIndex].xFrameTime;
	pObj->rType.animationInfo.nCurFrame = 0;
	pObj->SetLife (I2X (100) + RandShort () * 4);
	pObj->Ignore (1, 1);
	}
return 1;
}

//-----------------------------------------------------------------------------
//	Return true if it happened, else return false.
int32_t MarkPathToExit (void)
{
for (int32_t i = 0; i <= gameData.segData.nLastSegment; i++) {
	for (int32_t j = 0, h = SEGMENT_SIDE_COUNT; j < h; j++)
		if (SEGMENT (i)->m_children [j] == -2)
			return MarkPlayerPathToSegment (i);
	}
return 0;
}


//------------------------------------------------------------------------------

void ShowInGameWarning (const char *s)
{
if (gameData.render.screen.Width () && gameData.render.screen.Height ()) {
	const char	*hs, *ps = strstr (s, "Error");

	if (ps > s) {	//skip trailing non alphanum chars
		for (hs = ps - 1; (hs > s) && !isalnum (*hs); hs++)
			;
		if (hs > s)
			ps = NULL;
		}
	if (!(IsMultiGame && (gameStates.app.nFunctionMode == FMODE_GAME)))
		StopTime ();
	gameData.menu.warnColor = RED_RGBA;
	gameData.menu.colorOverride = gameData.menu.warnColor;
	if (!ps)
		InfoBox (TXT_WARNING, (pMenuCallback) NULL, BG_STANDARD, -3, s, " ", TXT_OK, "");
	else {
		for (ps += 5; *ps && !isalnum (*ps); ps++)
			;
		InfoBox (TXT_ERROR, (pMenuCallback) NULL, BG_STANDARD, -3, ps, " ", TXT_OK, "");
		}
	gameData.menu.colorOverride = 0;
	if (!(IsMultiGame && (gameStates.app.nFunctionMode == FMODE_GAME)))
		StartTime (0);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//eof
