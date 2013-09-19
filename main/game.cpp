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

u_int32_t nCurrentVGAMode;

#ifdef MWPROFILER
#include <profiler.h>
#endif

//#define TEST_TIMER	1		//if this is set, do checking on timer

//do menus work in 640x480 or 320x200?
//PC version sets this in main ().  Mac versios is always high-res, so set to 1 here
//	Toggle_var points at a variable which gets !ed on ctrl-alt-T press.

#if DBG                          //these only exist if debugging

int bGameDoubleBuffer = 1;     //double buffer by default
fix xFixedFrameTime = 0;          //if non-zero, set frametime to this

#endif

CBitmap bmBackground;

//	Function prototypes for GAME.C exclusively.

#define cv_w  Bitmap ().Width ()
#define cv_h  Bitmap ().Height ()

void NDRecordCockpitChange (int);

void FireGun (void);
void SlideTextures (void);
void PowerupGrabCheatAll (void);

//	Other functions
void MultiCheckForScoreGoalWinner (bool bForce);
void MultiCheckForEntropyWinner (void);
void MultiSendSoundFunction (char, char);
void DefaultAllSettings (void);
void ProcessSmartMinesFrame (void);
void DoSeismicStuff (void);
void ReadControls (void);		// located in gamecntl.c
void DoFinalBossFrame (void);

void MultiRemoveGhostShips (void);

void GameRenderFrame (void);
void OmegaChargeFrame (void);
void FlickerLights (void);

#if DBG
void DrawFrameRate (void);
#endif

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
	int dx, dy;

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

void MovePlayerToSegment (CSegment* segP, int nSide)
{
	CFixVector vp;

gameData.objs.consoleP->info.position.vPos = segP->Center ();
vp = segP->SideCenter (nSide) - gameData.objs.consoleP->info.position.vPos;
/*
gameData.objs.consoleP->info.position.mOrient = CFixMatrix::Create(vp, NULL, NULL);
*/
gameData.objs.consoleP->info.position.mOrient = CFixMatrix::CreateF (vp);
gameData.objs.consoleP->RelinkToSeg (segP->Index ());
}

//------------------------------------------------------------------------------
//this is called once per game

bool InitGame (int nSegments, int nVertices)
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
/*---*/PrintLog (1, "Default settings...\n");
DefaultAllSettings ();
PrintLog (-1);
/*---*/PrintLog (1, "Detail levels (%d)...\n", gameStates.app.nDetailLevel);
gameStates.app.nDetailLevel = InitDetailLevels (gameStates.app.nDetailLevel);
PrintLog (-1);
PrintLog (-1);
fpDrawTexPolyMulti = G3DrawTexPolyMulti;
return true;
}

//------------------------------------------------------------------------------

void GameInitRenderSubBuffers (int x, int y, int w, int h)
{
gameData.render.frame.SetLeft (x);
gameData.render.frame.SetTop (y);
gameData.render.frame.SetWidth (w);
gameData.render.frame.SetHeight (h);
}

//------------------------------------------------------------------------------

// Sets up the canvases we will be rendering to (NORMAL VERSION)
void GameInitRenderBuffers (int nScreenSize, int render_w, int render_h, int render_method, int flags)
{
gameData.render.screen.Set (nScreenSize);
gameData.render.frame.SetLeft (0);
gameData.render.frame.SetTop (0);
gameData.render.frame.SetWidth (render_w);
gameData.render.frame.SetHeight (render_h);
}

//------------------------------------------------------------------------------

//initialize flying
void FlyInit (CObject *objP)
{
objP->info.controlType = CT_FLYING;
objP->info.movementType = MT_PHYSICS;
objP->mType.physInfo.velocity.SetZero ();
objP->mType.physInfo.thrust.SetZero ();
objP->mType.physInfo.rotVel.SetZero ();
objP->mType.physInfo.rotThrust.SetZero ();
}

//void morph_test (), morph_step ();


//	------------------------------------------------------------------------------------

void DoCloakStuff (void)
{
for (int i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (gameData.multiplayer.players[i].flags & PLAYER_FLAGS_CLOAKED) {
		if (gameData.time.xGame - gameData.multiplayer.players [i].cloakTime > CLOAK_TIME_MAX) {
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

int bFakingInvul = 0;

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
//@@	int	rx, rz;
//@@
//@@	rx = (abScale * FixMul (SRandShort (), I2X (1)/8 + (((gameData.time.xGame + 0x4000)*4) & 0x3fff)))/16;
//@@	rz = (abScale * FixMul (SRandShort (), I2X (1)/2 + ((gameData.time.xGame*4) & 0xffff)))/16;
//@@
//@@	gameData.objs.consoleP->mType.physInfo.rotVel.x += rx;
//@@	gameData.objs.consoleP->mType.physInfo.rotVel.z += rz;
//@@
//@@}

//	------------------------------------------------------------------------------------

#define AFTERBURNER_LOOP_START	 ((gameOpts->sound.audioSampleRate==SAMPLE_RATE_22K)?32027: (32027/2))		//20098
#define AFTERBURNER_LOOP_END		 ((gameOpts->sound.audioSampleRate==SAMPLE_RATE_22K)?48452: (48452/2))		//25776

//static int abScale = 4;

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
	int nSoundObj = audio.FindObjectSound (LOCALPLAYER.nObject, SOUND_AFTERBURNER_IGNITE);
	if (gameData.physics.xAfterburnerCharge && controls [0].afterburnerState && (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER)) {
		if (nSoundObj < 0) {
			audio.CreateObjectSound ((short) SOUND_AFTERBURNER_IGNITE, SOUNDCLASS_PLAYER, (short) LOCALPLAYER.nObject, 
											 1, I2X (1), I2X (256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
			if (IsMultiGame)
				MultiSendSoundFunction (3, (char) SOUND_AFTERBURNER_IGNITE);
			}
		}
	else if (nSoundObj >= 0) { //gameStates.gameplay.xLastAfterburnerCharge || gameStates.gameplay.bLastAfterburnerState) {
#if 1
		audio.DeleteSoundObject (nSoundObj);
		audio.CreateObjectSound ((short) SOUND_AFTERBURNER_PLAY, SOUNDCLASS_PLAYER, (short) LOCALPLAYER.nObject);
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
	int i;

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

void GameSetup (void)
{
DoLunacyOn ();		//	Copy values for insane into copy buffer in ai.c
DoLunacyOff ();		//	Restore true insane mode.
gameStates.app.bGameAborted = 0;
gameStates.app.bEndLevelSequence = 0;
paletteManager.ResetEffect ();
SetScreenMode (SCREEN_GAME);
SetWarnFunc (ShowInGameWarning);
#if TRACE
//console.printf (CON_DBG, "   cockpit->Init () d:\temp\dm_test.\n");
#endif
cockpit->Init ();
#if TRACE
//console.printf (CON_DBG, "   InitGauges d:\temp\dm_test.\n");
#endif
//DigiInitSounds ();
//gameStates.input.keys.bRepeat = 0;                // Don't allow repeat in game
gameStates.input.keys.bRepeat = 1;                // Do allow repeat in game
gameData.objs.viewerP = gameData.objs.consoleP;
#if TRACE
//console.printf (CON_DBG, "   FlyInit d:\temp\dm_test.\n");
#endif
FlyInit (gameData.objs.consoleP);
if (gameStates.app.bGameSuspended & SUSP_TEMPORARY)
	gameStates.app.bGameSuspended &= ~(SUSP_ROBOTS | SUSP_TEMPORARY);
ResetTime ();
gameData.time.SetTime (0);			//make first frame zero
#if TRACE
//console.printf (CON_DBG, "   FixObjectSegs d:\temp\dm_test.\n");
#endif
GameFlushInputs ();
lightManager.SetMethod ();
gameStates.app.nSDLTicks [0] = 
gameStates.app.nSDLTicks [1] = 
gameData.time.xGameStart = SDL_GetTicks ();
gameData.physics.fLastTick = float (gameData.time.xGameStart);
ogl.m_features.bShaders.Available (gameOpts->render.bUseShaders);
gameData.render.rift.SetCenter ();
}

//------------------------------------------------------------------------------

void SetFunctionMode (int newFuncMode)
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

void CleanupAfterGame (bool bHaveLevel)
{
#ifdef MWPROFILE
ProfilerSetStatus (0);
#endif
DestroyEffectsThread ();
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
gameData.Destroy ();
missionConfig.Init ();
PiggyCloseFile ();
SavePlayerProfile ();
ResetModFolders ();
gameStates.app.bGameRunning = 0;
backgroundManager.Rebuild ();
SetFunctionMode (FMODE_MENU);	
}

//	-----------------------------------------------------------------------------

void DoRenderFrame (void)
{
gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
if (gameStates.render.cockpit.bRedraw) {
	cockpit->Init ();
	gameStates.render.cockpit.bRedraw = 0;
	}
GameRenderFrame ();
gameStates.app.bUsingConverter = 0;
AutoScreenshot ();
}

//	-----------------------------------------------------------------------------

#if PHYSICS_FPS >= 0

int GameFrame (int bRenderFrame, int bReadControls, int fps)
{
if (bRenderFrame)
	DoRenderFrame ();

	fix		xFrameTime = gameData.time.xFrame; 
	float		physicsFrameTime = 1000.0f / float (PHYSICS_FPS);
	float		t = float (SDL_GetTicks ()), dt = t - gameData.physics.fLastTick;
	int		h = 1, i = 0;

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
		gameData.objs.nFrameCount++;
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
//this function is the game.  called when game mode selected.  runs until
//editor mode or exit selected
void RunGame (void)
{
	int i, c;

GameSetup ();								// Replaces what was here earlier.
#ifdef MWPROFILE
ProfilerSetStatus (1);
#endif

if (pfnTIRStart)
	pfnTIRStart ();

if (!setjmp (gameExitPoint)) {
	for (;;) {
		PROF_START
		int playerShield;
			// GAME LOOP!
#if DBG
		if (automap.Display ())
#endif
		automap.m_bDisplay = 0;
		gameStates.app.bConfigMenu = 0;
		if (gameData.objs.consoleP != OBJECTS + LOCALPLAYER.nObject) {
#if TRACE
			 //console.printf (CON_DBG,"N_LOCALPLAYER=%d nObject=%d",N_LOCALPLAYER,LOCALPLAYER.nObject);
#endif
			 //Assert (gameData.objs.consoleP == &OBJECTS[LOCALPLAYER.nObject]);
			}
		playerShield = LOCALPLAYER.Shield ();
		gameStates.app.nExtGameStatus = GAMESTAT_RUNNING;

		try {
			i = GameFrame (1, 1);
			}
		catch (int e) {
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
		if (i < 0)
			break;
		if (i == 0)
			continue;

		if (gameStates.app.bSingleStep) {
			while (!(c = KeyInKey ()))
				;
			if (c == KEY_ALTED + KEY_CTRLED + KEY_ESC)
				gameStates.app.bSingleStep = 0;
			}
		//if the player is taking damage, give up guided missile control
		if (LOCALPLAYER.Shield () != playerShield)
			ReleaseGuidedMissile (N_LOCALPLAYER);
		//see if redbook song needs to be restarted
		redbook.CheckRepeat ();	// Handle RedBook Audio Repeating.
		if (gameStates.app.bConfigMenu) {
			//paletteManager.SuspendEffect (!IsMultiGame);
			ConfigMenu ();
			//paletteManager.ResumeEffect (!IsMultiGame);
			}
		if (automap.Display ()) {
			int	save_w = gameData.render.frame.Width (),
					save_h = gameData.render.frame.Height ();
			automap.DoFrame (0, 0);
			gameStates.app.bEnterGame = 1;
			//	FlushInput ();
			//	StopPlayerMovement ();
			gameStates.video.nScreenMode = -1;
			SetScreenMode (SCREEN_GAME);
			gameData.render.frame.SetWidth (save_w);
			gameData.render.frame.SetHeight (save_h);
			cockpit->Init ();
			}
		if ((gameStates.app.nFunctionMode != FMODE_GAME) &&
			 gameData.demo.bAuto && !gameOpts->demo.bRevertFormat &&
			 (gameData.demo.nState != ND_STATE_NORMAL)) {
			int choice, fmode;
			fmode = gameStates.app.nFunctionMode;
			SetFunctionMode (FMODE_GAME);
			//paletteManager.SuspendEffect ();
			choice = MsgBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO);
			//paletteManager.ResumeEffect ();
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
		gameStates.multi.bIWasKicked = 0;
		PROF_END(ptFrame);
#if PROFILING
		if (gameData.app.nFrameCount % 10000 == 0)
			memset (&gameData.profiler, 0, sizeof (gameData.profiler));
#endif
		if (gameStates.app.nFunctionMode != FMODE_GAME)
			break; //longjmp (gameExitPoint, 0);
		}
	}
try {
	CleanupAfterGame ();
	}
catch (...) {
	Warning ("Internal error when cleaning up.");
	}
}

//-----------------------------------------------------------------------------
//called at the end of the program

void _CDECL_ CloseGame (void)
{
	static	int bGameClosed = 0;
#if MULTI_THREADED
	int		i;
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
gameData.segs.skybox.Destroy ();
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
//	int 		i;
	CObject	*objP = OBJECTS.Buffer ();

FORALL_ROBOT_OBJS (objP, i)
	if (IS_GUIDEBOT (objP))
		return objP;
return NULL;
}

//-----------------------------------------------------------------------------
//if water or fire level, make occasional sound
void DoAmbientSounds (void)
{
if (gameStates.app.bPlayerIsDead)
	return;

	int	bLava, bWater;
	short nSound;

	bLava = (SEGMENTS [gameData.objs.consoleP->info.nSegment].m_flags & S2F_AMBIENT_LAVA);
	bWater = (SEGMENTS [gameData.objs.consoleP->info.nSegment].m_flags & S2F_AMBIENT_WATER);

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

//-----------------------------------------------------------------------------
// -- extern void lightning_frame (void);

#if PHYSICS_FPS >= 0
int DoGameFrame (int bRenderFrame, int bReadControls, int bFrameTime)
#else
int GameFrame (int bRenderFrame, int bReadControls, int bFrameTime)
#endif
{
gameStates.app.bGameRunning = 1;
//gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
gameData.objs.nLastObject [1] = gameData.objs.nLastObject [0];

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
DrainHeadlightPower ();

if (IsMultiGame) {
	if (!MultiProtectGame ()) {
		SetFunctionMode (FMODE_MENU);
		return -1;
		}
	AutoBalanceTeams ();
	MultiSendTyping ();
	MultiSendWeapons (0);
	MultiSyncKills ();
	MultiAdjustPowerups ();
	tracker.AddServer ();
	MultiDoFrame ();
	CheckMonsterballScore ();
	MultiCheckForScoreGoalWinner (netGame.GetPlayTimeAllowed () && (gameStates.app.xThisLevelTime >= I2X ((netGame.GetPlayTimeAllowed () * 5 * 60))));
	MultiCheckForEntropyWinner ();
	MultiRemoveGhostShips ();
	}

#if PHYSICS_FPS < 0
DoEffectsFrame ();
#endif
if (bRenderFrame)
	DoRenderFrame ();
if (bFrameTime)
	CalcFrameTime ();
if (bReadControls > 0)
	ReadControls ();
else if (bReadControls < 0)
	controls.Reset ();

DeadPlayerFrame ();
if (gameData.demo.nState != ND_STATE_PLAYBACK) 
	DoReactorDeadFrame ();
ProcessSmartMinesFrame ();
DoSeismicStuff ();
DoAmbientSounds ();
DropPowerups ();
gameData.time.xGame += gameData.time.xFrame;
if ((gameData.time.xGame < 0) || (gameData.time.xGame > I2X (0x7fff - 600))) 
	gameData.time.xGame = gameData.time.xFrame;	//wrap when goes negative, or gets within 10 minutes
if (IsMultiGame && netGame.GetPlayTimeAllowed ())
	gameStates.app.xThisLevelTime += gameData.time.xFrame;
audio.SyncSounds ();
if (gameStates.app.bEndLevelSequence) {
	DoEndLevelFrame ();
	PowerupGrabCheatAll ();
	DoSpecialEffects ();
	return 1;					//skip everything else
	}
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
SetPlayerPaths ();
FlashFrame ();
if (gameData.demo.nState == ND_STATE_PLAYBACK) {
	NDPlayBackOneFrame ();
	if (gameData.demo.nState != ND_STATE_PLAYBACK)
		longjmp (gameExitPoint, 0);		// Go back to menu
	}
else { // Note the link to above!
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
	}
if (gameStates.render.bDoAppearanceEffect) {
	gameData.objs.consoleP->CreateAppearanceEffect ();
	gameStates.render.bDoAppearanceEffect = 0;
	if (IsMultiGame && netGame.m_info.invul) {
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
return 1;
}


//	-----------------------------------------------------------------------------

void ComputeSlideSegs (void)
{
	int			nSegment, nSide, bIsSlideSeg, nTexture;
	CSegment*	segP = SEGMENTS.Buffer ();

gameData.segs.nSlideSegs = 0;
for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++, segP++) {
	bIsSlideSeg = 0;
	for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
		if (!segP->Side (nSide)->FaceCount ())
			continue;
		nTexture = segP->m_sides [nSide].m_nBaseTex;
		if (gameData.pig.tex.tMapInfoP [nTexture].slide_u  || gameData.pig.tex.tMapInfoP [nTexture].slide_v) {
			if (!bIsSlideSeg) {
				bIsSlideSeg = 1;
				gameData.segs.slideSegs [gameData.segs.nSlideSegs].nSegment = nSegment;
				gameData.segs.slideSegs [gameData.segs.nSlideSegs].nSides = 0;
				}
			gameData.segs.slideSegs [gameData.segs.nSlideSegs].nSides |= (1 << nSide);
			}
		}
	if (bIsSlideSeg)
		gameData.segs.nSlideSegs++;
	}
gameData.segs.bHaveSlideSegs = 1;
}

//	-----------------------------------------------------------------------------

void SlideTextures (void)
{
	int			nSegment, nSide, h, i, j, tmn;
	ubyte			sides;
	CSegment*	segP;
	CSide*		sideP;
	tUVL*			uvlP;
	fix			slideU, slideV, xDelta;

if (!gameData.segs.bHaveSlideSegs)
	ComputeSlideSegs ();
for (h = 0; h < gameData.segs.nSlideSegs; h++) {
	nSegment = gameData.segs.slideSegs [h].nSegment;
	segP = SEGMENTS + nSegment;
	sides = gameData.segs.slideSegs [h].nSides;
	for (nSide = 0, sideP = segP->m_sides; nSide < SEGMENT_SIDE_COUNT; nSide++, sideP++) {
		if (!(segP->Side (nSide)->FaceCount () && (sides & (1 << nSide))))
			continue;
		tmn = sideP->m_nBaseTex;
		slideU = (fix) gameData.pig.tex.tMapInfoP [tmn].slide_u;
		slideV = (fix) gameData.pig.tex.tMapInfoP [tmn].slide_v;
		if (!(slideU || slideV))
			continue;
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nSegment = nSegment;
#endif
		i = (segP->m_function == SEGMENT_FUNC_SKYBOX) ? 3 : 8;
		slideU = FixMul (gameData.time.xFrame, slideU << i);
		slideV = FixMul (gameData.time.xFrame, slideV << i);
		for (i = 0, uvlP = sideP->m_uvls; i < 4; i++) {
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
void PowerupGrabCheat (CObject *playerP, int nObject)
{
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return;

	CObject*					powerupP = OBJECTS + nObject;
	tObjTransformation*	posP = OBJPOS (playerP);
	CFixVector				vCollision;

Assert (powerupP->info.nType == OBJ_POWERUP);
if (powerupP->info.nFlags & OF_SHOULD_BE_DEAD)
	return;
if (CFixVector::Dist (powerupP->info.position.vPos, posP->vPos) >=
	 2 * (playerP->info.xSize + powerupP->info.xSize) / (gameStates.app.bHaveExtraGameInfo [IsMultiGame] + 1))
	return;
vCollision = CFixVector::Avg (powerupP->info.position.vPos, posP->vPos);
playerP->CollidePlayerAndPowerup (powerupP, vCollision);
}

//	-------------------------------------------------------------------------------------------------------
//	Make it easier to pick up powerups.
//	For all powerups in this CSegment, pick them up at up to twice pickuppable distance based on dot product
//	from CPlayerData to powerup and CPlayerData's forward vector.
//	This has the effect of picking them up more easily left/right and up/down, but not making them disappear
//	way before the player gets there.
void PowerupGrabCheatAll (void)
{
if (gameStates.app.tick40fps.bTick) {
	short nObject = SEGMENTS [gameData.objs.consoleP->info.nSegment].m_objects;
	while (nObject != -1) {
		if (OBJECTS [nObject].info.nType == OBJ_POWERUP)
			PowerupGrabCheat (gameData.objs.consoleP, nObject);
		nObject = OBJECTS [nObject].info.nNextInSeg;
		}
	}
}

//	------------------------------------------------------------------------------------------------------------------
//	Create path for CPlayerData from current CSegment to goal CSegment.
//	Return true if path created, else return false.

int nLastLevelPathCreated = -1;

int MarkPlayerPathToSegment (int nSegment)
{
	int		i;
	CObject* objP = gameData.objs.consoleP;
	short		playerPathLength = 0;
	int		playerHideIndex = -1;

if (nLastLevelPathCreated == missionManager.nCurrentLevel)
	return 0;
nLastLevelPathCreated = missionManager.nCurrentLevel;
if (CreatePathPoints (objP, objP->info.nSegment, nSegment, gameData.ai.freePointSegs, &playerPathLength, 100, 0, 0, -1) == -1) {
#if TRACE
	//console.printf (CON_DBG, "Unable to form path of length %i for myself\n", 100);
#endif
	return 0;
	}
playerHideIndex = int (gameData.ai.routeSegs.Index (gameData.ai.freePointSegs));
gameData.ai.freePointSegs += playerPathLength;
if (int (gameData.ai.routeSegs.Index (gameData.ai.freePointSegs)) + MAX_PATH_LENGTH * 2 > MAX_POINT_SEGS) {
#if TRACE
	//console.printf (1, "Can't create path.  Not enough tPointSegs.\n");
#endif
	AIResetAllPaths ();
	return 0;
	}
for (i = 1; i < playerPathLength; i++) {
	short			nSegment, nObject;
	CFixVector	vSegCenter;
	CObject		*objP;

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
	objP = OBJECTS + nObject;
	objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
	objP->rType.vClipInfo.xFrameTime = gameData.effects.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
	objP->rType.vClipInfo.nCurFrame = 0;
	objP->SetLife (I2X (100) + RandShort () * 4);
	objP->Ignore (1, 1);
	}
return 1;
}

//-----------------------------------------------------------------------------
//	Return true if it happened, else return false.
int MarkPathToExit (void)
{
for (int i = 0; i <= gameData.segs.nLastSegment; i++) {
	for (int j = 0, h = SEGMENT_SIDE_COUNT; j < h; j++)
		if (SEGMENTS [i].m_children [j] == -2)
			return MarkPlayerPathToSegment (i);
	}
return 0;
}


//------------------------------------------------------------------------------

void ShowInGameWarning (const char *s)
{
if (screen.Width () && screen.Height ()) {
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
		MsgBox (TXT_WARNING, NULL, -3, s, " ", TXT_OK);
	else {
		for (ps += 5; *ps && !isalnum (*ps); ps++)
			;
		MsgBox (TXT_ERROR, NULL, -3, ps, " ", TXT_OK);
		}
	gameData.menu.colorOverride = 0;
	if (!(IsMultiGame && (gameStates.app.nFunctionMode == FMODE_GAME)))
		StartTime (0);
	}
}

//-----------------------------------------------------------------------------
//eof
