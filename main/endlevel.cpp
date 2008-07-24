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

//#define SLEW_ON 1

//#define _MARK_ON

#include <stdlib.h>


#include <stdio.h>
#include <string.h>
#include <ctype.h> // for isspace

#include "inferno.h"
#include "error.h"
#include "palette.h"
#include "iff.h"
#include "texmap.h"
#include "u_mem.h"
#include "endlevel.h"
#include "objrender.h"
#include "transprender.h"
#include "screens.h"
#include "gauges.h"
#include "terrain.h"
#include "newdemo.h"
#include "fireball.h"
#include "network.h"
#include "text.h"
#include "compbit.h"
#include "movie.h"
#include "render.h"
#include "gameseg.h"
#include "key.h"
#include "joy.h"

//------------------------------------------------------------------------------

typedef struct tExitFlightData {
	tObject		*objP;
	vmsAngVec	angles;			//orientation in angles
	vmsVector	step;				//how far in a second
	vmsVector	angstep;			//rotation per second
	fix			speed;			//how fast tObject is moving
	vmsVector 	headvec;			//where we want to be pointing
	int			firstTime;		//flag for if first time through
	fix			offset_frac;	//how far off-center as portion of way
	fix			offsetDist;	//how far currently off-center
} tExitFlightData;

//endlevel sequence states

#define SHORT_SEQUENCE	1		//if defined, end sequnce when panning starts
//#define STATION_ENABLED	1		//if defined, load & use space station model

#define FLY_SPEED i2f (50)

void DoEndLevelFlyThrough (int n);
void DrawStars ();
int FindExitSide (tObject *objP);
void GenerateStarfield ();
void StartEndLevelFlyThrough (int n, tObject *objP, fix speed);
void StartRenderedEndLevelSequence ();

char movieTable [2][30] = {
	{'a', 'b', 'c', 
	'a', 
	'd', 'f', 'd', 'f', 
	'g', 'h', 'i', 'g', 
	'j', 'k', 'l', 'j', 
	'm', 'o', 'm', 'o', 
	'p', 'q', 'p', 'q', 
	0, 0, 0, 0, 0, 0
	}, 
	{'f', 'f', 'f', 'a', 'a', 'b', 'b', 'c', 'c', 
	 'c', 'g', 'f', 'g', 'g', 'f', 'g', 'f', 'g', 
	 'f', 'f', 'f', 'g', 'f', 'g', 'd', 'd', 'f', 
	 'e', 'e', 'e'
	}
};

#define FLY_ACCEL i2f (5)

//static tUVL satUVL [4] = {{0,0,F1_0},{F1_0,0,F1_0},{F1_0,F1_0,F1_0},{0,F1_0,F1_0}};
static tUVL satUVL [4] = {{0,0,F1_0},{F1_0,0,F1_0},{F1_0,F1_0,F1_0},{0,F1_0,F1_0}};

extern int ELFindConnectedSide (int seg0, int seg1);

//------------------------------------------------------------------------------

#define MAX_EXITFLIGHT_OBJS 2

tExitFlightData exitFlightObjects [MAX_EXITFLIGHT_OBJS];
tExitFlightData *exitFlightDataP;

//------------------------------------------------------------------------------

void InitEndLevelData (void)
{
gameData.endLevel.station.vPos.p.x = 0xf8c4 << 10;
gameData.endLevel.station.vPos.p.y = 0x3c1c << 12;
gameData.endLevel.station.vPos.p.z = 0x0372 << 10;
}

//------------------------------------------------------------------------------
//find delta between two angles
inline fixang DeltaAng (fixang a, fixang b)
{
fixang delta0, delta1;

return (abs (delta0 = a - b) < abs (delta1 = b - a)) ? delta0 : delta1;
}

//------------------------------------------------------------------------------
//return though which tSide of seg0 is seg1
int ELFindConnectedSide (int seg0, int seg1)
{
	tSegment *segP = gameData.segs.segments + seg0;
	int		i;

for (i = MAX_SIDES_PER_SEGMENT;i--; ) 
	if (segP->children [i] == seg1) 
		return i;
return -1;
}

//------------------------------------------------------------------------------

#define MOVIE_REQUIRED 1

//returns movie played status.  see movie.h
int StartEndLevelMovie (void)
{
	char szMovieName [SHORT_FILENAME_LEN];
	int r;

strcpy (szMovieName, gameStates.app.bD1Mission ? "exita.mve" : "esa.mve");
if (!IS_D2_OEM)
	if (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)
		return 1;   //don't play movie
if (gameData.missions.nCurrentLevel > 0)
	if (gameStates.app.bD1Mission)
		szMovieName [4] = movieTable [1][gameData.missions.nCurrentLevel-1];
	else
		szMovieName [2] = movieTable [0][gameData.missions.nCurrentLevel-1];
else if (gameStates.app.bD1Mission) {
	szMovieName [4] = movieTable [1][26 - gameData.missions.nCurrentLevel];
	}
else {
#ifndef SHAREWARE
	return 0;       //no escapes for secret level
#else
	Error ("Invalid level number <%d>", gameData.missions.nCurrentLevel);
#endif
}
#ifndef SHAREWARE
r = PlayMovie (szMovieName, (gameData.app.nGameMode & GM_MULTI) ? 0 : MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
#else
return 0;	// movie not played for shareware
#endif
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	SetScreenMode (SCREEN_GAME);
gameData.score.bNoMovieMessage = (r == MOVIE_NOT_PLAYED) && IsMultiGame;
return r;
}

//------------------------------------------------------------------------------

void _CDECL_ FreeEndLevelData (void)
{
PrintLog ("unloading endlevel data\n");
if (gameData.endLevel.terrain.bmInstance.bmTexBuf) {
	OglFreeBmTexture (&gameData.endLevel.terrain.bmInstance);
	D2_FREE (gameData.endLevel.terrain.bmInstance.bmTexBuf);
	}
if (gameData.endLevel.satellite.bmInstance.bmTexBuf) {
	OglFreeBmTexture (&gameData.endLevel.satellite.bmInstance);
	D2_FREE (gameData.endLevel.satellite.bmInstance.bmTexBuf);
	}
}

//-----------------------------------------------------------------------------

void InitEndLevel (void)
{
#ifdef STATION_ENABLED
	gameData.endLevel.station.bmP = bm_load ("steel3.bbm");
	gameData.endLevel.station.bmList [0] = &gameData.endLevel.station.bmP;

	gameData.endLevel.station.nModel = LoadPolygonModel ("station.pof", 1, gameData.endLevel.station.bmList, NULL);
#endif
GenerateStarfield ();
atexit (FreeEndLevelData);
gameData.endLevel.terrain.bmInstance.bmTexBuf = 
gameData.endLevel.satellite.bmInstance.bmTexBuf = NULL;
}

//------------------------------------------------------------------------------

tObject externalExplosion;

vmsAngVec vExitAngles = {-0xa00, 0, 0};

vmsMatrix mSurfaceOrient;

extern char szLastPaletteLoaded [];

void StartEndLevelSequence (int bSecret)
{
	int	i, nMoviePlayed = MOVIE_NOT_PLAYED;

if (gameData.demo.nState == ND_STATE_RECORDING)		// stop demo recording
	gameData.demo.nState = ND_STATE_PAUSED;
if (gameData.demo.nState == ND_STATE_PLAYBACK) {		// don't do this if in playback mode
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) ||
		 ((gameData.missions.nCurrentMission == gameData.missions.nD1BuiltinMission) && 
		 gameStates.app.bHaveExtraMovies))
		StartEndLevelMovie ();
	strcpy (szLastPaletteLoaded, "");		//force palette load next time
	return;
	}
if (gameStates.app.bPlayerIsDead || (gameData.objs.console->flags & OF_SHOULD_BE_DEAD))
	return;				//don't start if dead!
//	Dematerialize Buddy!
for (i = 0; i <= gameData.objs.nLastObject [0]; i++)
	if (OBJECTS [i].nType == OBJ_ROBOT)
		if (ROBOTINFO (OBJECTS [i].id).companion) {
			ObjectCreateExplosion (OBJECTS [i].nSegment, &OBJECTS [i].position.vPos, F1_0*7/2, VCLIP_POWERUP_DISAPPEARANCE);
			KillObject (OBJECTS + i);
		}
LOCALPLAYER.homingObjectDist = -F1_0; // Turn off homing sound.
ResetRearView ();		//turn off rear view if set
if (IsMultiGame) {
	MultiSendEndLevelStart (0);
	NetworkDoFrame (1, 1);
	}
if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) ||
		((gameData.missions.nCurrentMission == gameData.missions.nD1BuiltinMission) && 
		gameStates.app.bHaveExtraMovies)) {
	// only play movie for built-in mission
	if (!IsMultiGame)
		nMoviePlayed = StartEndLevelMovie ();
	}
if ((nMoviePlayed == MOVIE_NOT_PLAYED) && gameStates.app.bEndLevelDataLoaded) {   //don't have movie.  Do rendered sequence, if available
	int bExitModelsLoaded = (gameData.pig.tex.nHamFileVersion < 3) ? 1 : LoadExitModels ();
	if (bExitModelsLoaded) {
		StartRenderedEndLevelSequence ();
		return;
		}
	}
//don't have movie or rendered sequence, fade out
GrPaletteFadeOut (NULL, 32, 0);
if (!bSecret)
	PlayerFinishedLevel (0);		//done with level
}

//------------------------------------------------------------------------------

void StartRenderedEndLevelSequence (void)
{
	int nLastSeg, nExitSide, nTunnelLength;
	int nSegment, nOldSeg, nEntrySide, i;

//count segments in exit tunnel
nOldSeg = gameData.objs.console->nSegment;
nExitSide = FindExitSide (gameData.objs.console);
nSegment = gameData.segs.segments [nOldSeg].children [nExitSide];
nTunnelLength = 0;
do {
	nEntrySide = ELFindConnectedSide (nSegment, nOldSeg);
	nExitSide = sideOpposite [nEntrySide];
	nOldSeg = nSegment;
	nSegment = gameData.segs.segments [nSegment].children [nExitSide];
	nTunnelLength++;
	} while (nSegment >= 0);
if (nSegment != -2) {
	PlayerFinishedLevel (0);		//don't do special sequence
	return;
	}
nLastSeg = nOldSeg;
//now pick transition nSegment 1/3 of the way in
nOldSeg = gameData.objs.console->nSegment;
nExitSide = FindExitSide (gameData.objs.console);
nSegment = gameData.segs.segments [nOldSeg].children [nExitSide];
i = nTunnelLength / 3;
while (i--) {
	nEntrySide = ELFindConnectedSide (nSegment, nOldSeg);
	nExitSide = sideOpposite [nEntrySide];
	nOldSeg = nSegment;
	nSegment = gameData.segs.segments [nSegment].children [nExitSide];
	}
gameData.endLevel.exit.nTransitSegNum = nSegment;
gameStates.render.cockpit.nModeSave = gameStates.render.cockpit.nMode;
if (IsMultiGame) {
	MultiSendEndLevelStart (0);
	NetworkDoFrame (1, 1);
	}
Assert (nLastSeg == gameData.endLevel.exit.nSegNum);
if (gameData.missions.list [gameData.missions.nCurrentMission].nDescentVersion == 1)
	SongsPlaySong (SONG_ENDLEVEL, 0);
gameStates.app.bEndLevelSequence = EL_FLYTHROUGH;
gameData.objs.console->movementType = MT_NONE;			//movement handled by flythrough
gameData.objs.console->controlType = CT_NONE;
gameStates.app.bGameSuspended |= SUSP_ROBOTS;          //robots don't move
gameData.endLevel.xCurFlightSpeed = gameData.endLevel.xDesiredFlightSpeed = FLY_SPEED;
StartEndLevelFlyThrough (0, gameData.objs.console, gameData.endLevel.xCurFlightSpeed);		//initialize
HUDInitMessage (TXT_EXIT_SEQUENCE);
gameStates.render.bOutsideMine = gameStates.render.bExtExplPlaying = 0;
gameStates.render.nFlashScale = f1_0;
gameStates.gameplay.bMineDestroyed = 0;
}

//------------------------------------------------------------------------------

vmsAngVec vPlayerAngles, vPlayerDestAngles;
vmsAngVec vDesiredCameraAngles, vCurrentCameraAngles;

#define CHASE_TURN_RATE (0x4000/4)		//max turn per second

//returns bitmask of which angles are at dest. bits 0, 1, 2 = p, b, h
int ChaseAngles (vmsAngVec *cur_angles, vmsAngVec *desired_angles)
{
	vmsAngVec	delta_angs, alt_angles, alt_delta_angs;
	fix			total_delta, altTotal_delta;
	fix			frame_turn;
	int			mask = 0;

delta_angs.p = desired_angles->p - cur_angles->p;
delta_angs.h = desired_angles->h - cur_angles->h;
delta_angs.b = desired_angles->b - cur_angles->b;
total_delta = abs (delta_angs.p) + abs (delta_angs.b) + abs (delta_angs.h);

alt_angles.p = f1_0/2 - cur_angles->p;
alt_angles.b = cur_angles->b + f1_0/2;
alt_angles.h = cur_angles->h + f1_0/2;

alt_delta_angs.p = desired_angles->p - alt_angles.p;
alt_delta_angs.h = desired_angles->h - alt_angles.h;
alt_delta_angs.b = desired_angles->b - alt_angles.b;
//alt_delta_angs.b = 0;

altTotal_delta = abs (alt_delta_angs.p) + abs (alt_delta_angs.b) + abs (alt_delta_angs.h);

////printf ("Total delta = %x, alt total_delta = %x\n", total_delta, altTotal_delta);

if (altTotal_delta < total_delta) {
	////printf ("FLIPPING ANGLES!\n");
	*cur_angles = alt_angles;
	delta_angs = alt_delta_angs;
}

frame_turn = FixMul (gameData.time.xFrame, CHASE_TURN_RATE);

if (abs (delta_angs.p) < frame_turn) {
	cur_angles->p = desired_angles->p;
	mask |= 1;
	}
else
	if (delta_angs.p > 0)
		cur_angles->p += (fixang) frame_turn;
	else
		cur_angles->p -= (fixang) frame_turn;

if (abs (delta_angs.b) < frame_turn) {
	cur_angles->b = (fixang) desired_angles->b;
	mask |= 2;
	}
else
	if (delta_angs.b > 0)
		cur_angles->b += (fixang) frame_turn;
	else
		cur_angles->b -= (fixang) frame_turn;
if (abs (delta_angs.h) < frame_turn) {
	cur_angles->h = (fixang) desired_angles->h;
	mask |= 4;
	}
else
	if (delta_angs.h > 0)
		cur_angles->h += (fixang) frame_turn;
	else
		cur_angles->h -= (fixang) frame_turn;
return mask;
}

//------------------------------------------------------------------------------

void StopEndLevelSequence (void)
{
	gameStates.render.nInterpolationMethod = 0;

GrPaletteFadeOut (NULL, 32, 0);
SelectCockpit (gameStates.render.cockpit.nModeSave);
gameStates.app.bEndLevelSequence = EL_OFF;
PlayerFinishedLevel (0);
}

//------------------------------------------------------------------------------

#define VCLIP_BIG_PLAYER_EXPLOSION	58

//--unused-- vmsVector upvec = {0, f1_0, 0};

//find the angle between the tPlayer's heading & the station
inline void GetAnglesToObject (vmsAngVec *av, vmsVector *targ_pos, vmsVector *cur_pos)
{
	vmsVector tv;

VmVecSub (&tv, targ_pos, cur_pos);
VmExtractAnglesVector (av, &tv);
}

//------------------------------------------------------------------------------

void DoEndLevelFrame (void)
{
	static fix timer;
	static fix bank_rate;
	vmsVector save_last_pos;
	static fix explosion_wait1=0;
	static fix explosion_wait2=0;
	static fix ext_expl_halflife;

save_last_pos = gameData.objs.console->vLastPos;	//don't let move code change this
UpdateAllObjects ();
gameData.objs.console->vLastPos = save_last_pos;

if (gameStates.render.bExtExplPlaying) {
	externalExplosion.lifeleft -= gameData.time.xFrame;
	DoExplosionSequence (&externalExplosion);
	if (externalExplosion.lifeleft < ext_expl_halflife)
		gameStates.gameplay.bMineDestroyed = 1;
	if (externalExplosion.flags & OF_SHOULD_BE_DEAD)
		gameStates.render.bExtExplPlaying = 0;
	}

if (gameData.endLevel.xCurFlightSpeed != gameData.endLevel.xDesiredFlightSpeed) {
	fix delta = gameData.endLevel.xDesiredFlightSpeed - gameData.endLevel.xCurFlightSpeed;
	fix frame_accel = FixMul (gameData.time.xFrame, FLY_ACCEL);

	if (abs (delta) < frame_accel)
		gameData.endLevel.xCurFlightSpeed = gameData.endLevel.xDesiredFlightSpeed;
	else if (delta > 0)
		gameData.endLevel.xCurFlightSpeed += frame_accel;
	else
		gameData.endLevel.xCurFlightSpeed -= frame_accel;
	}

//do big explosions
if (!gameStates.render.bOutsideMine) {
	if (gameStates.app.bEndLevelSequence == EL_OUTSIDE) {
		vmsVector tvec;

		VmVecSub (&tvec, &gameData.objs.console->position.vPos, &gameData.endLevel.exit.vSideExit);
		if (VmVecDot (&tvec, &gameData.endLevel.exit.mOrient.fVec) > 0) {
			tObject *objP;
			gameStates.render.bOutsideMine = 1;
			objP = ObjectCreateExplosion (gameData.endLevel.exit.nSegNum, &gameData.endLevel.exit.vSideExit, i2f (50), VCLIP_BIG_PLAYER_EXPLOSION);
			if (objP) {
				externalExplosion = *objP;
				KillObject (objP);
				gameStates.render.nFlashScale = 0;	//kill lights in mine
				ext_expl_halflife = objP->lifeleft;
				gameStates.render.bExtExplPlaying = 1;
				}
			DigiLinkSoundToPos (SOUND_BIG_ENDLEVEL_EXPLOSION, gameData.endLevel.exit.nSegNum, 0, &gameData.endLevel.exit.vSideExit, 0, i2f (3)/4);
			}
		}

	//do explosions chasing tPlayer
	if ((explosion_wait1 -= gameData.time.xFrame) < 0) {
		vmsVector	tpnt;
		short			nSegment;
		tObject		*expl;
		static int	soundCount;

		VmVecScaleAdd (&tpnt, &gameData.objs.console->position.vPos, &gameData.objs.console->position.mOrient.fVec, -gameData.objs.console->size * 5);
		VmVecScaleInc (&tpnt, &gameData.objs.console->position.mOrient.rVec, (d_rand ()- RAND_MAX / 2) * 15);
		VmVecScaleInc (&tpnt, &gameData.objs.console->position.mOrient.uVec, (d_rand ()- RAND_MAX / 2) * 15);
		nSegment = FindSegByPos (&tpnt, gameData.objs.console->nSegment, 1, 0);
		if (nSegment != -1) {
			expl = ObjectCreateExplosion (nSegment, &tpnt, i2f (20), VCLIP_BIG_PLAYER_EXPLOSION);
			if (d_rand ()<10000 || ++soundCount==7) {		//pseudo-random
				DigiLinkSoundToPos (SOUND_TUNNEL_EXPLOSION, nSegment, 0, &tpnt, 0, F1_0);
				soundCount=0;
				}
			}
		explosion_wait1 = 0x2000 + d_rand ()/4;
		}
	}

//do little explosions on walls
if ((gameStates.app.bEndLevelSequence >= EL_FLYTHROUGH) && (gameStates.app.bEndLevelSequence < EL_OUTSIDE))
	if ((explosion_wait2 -= gameData.time.xFrame) < 0) {
		vmsVector tpnt;
		tFVIQuery fq;
		tFVIData hit_data;
		//create little explosion on tWall
		VmVecCopyScale (&tpnt, &gameData.objs.console->position.mOrient.rVec, (d_rand ()-RAND_MAX/2)*100);
		VmVecScaleInc (&tpnt, &gameData.objs.console->position.mOrient.uVec, (d_rand ()-RAND_MAX/2)*100);
		VmVecInc (&tpnt, &gameData.objs.console->position.vPos);
		if (gameStates.app.bEndLevelSequence == EL_FLYTHROUGH)
			VmVecScaleInc (&tpnt, &gameData.objs.console->position.mOrient.fVec, d_rand ()*200);
		else
			VmVecScaleInc (&tpnt, &gameData.objs.console->position.mOrient.fVec, d_rand ()*60);
		//find hit point on tWall
		fq.p0					= &gameData.objs.console->position.vPos;
		fq.p1					= &tpnt;
		fq.startSeg			= gameData.objs.console->nSegment;
		fq.radP0				= 
		fq.radP1				= 0;
		fq.thisObjNum		= 0;
		fq.ignoreObjList	= NULL;
		fq.flags				= 0;
		FindVectorIntersection (&fq, &hit_data);
		if ((hit_data.hit.nType == HIT_WALL) && (hit_data.hit.nSegment != -1))
			ObjectCreateExplosion ((short) hit_data.hit.nSegment, &hit_data.hit.vPoint, i2f (3)+d_rand ()*6, VCLIP_SMALL_EXPLOSION);
		explosion_wait2 = (0xa00 + d_rand ()/8)/2;
		}

switch (gameStates.app.bEndLevelSequence) {
	case EL_OFF:
		return;

	case EL_FLYTHROUGH: {
		DoEndLevelFlyThrough (0);
		if (gameData.objs.console->nSegment == gameData.endLevel.exit.nTransitSegNum) {
			if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) &&
					(StartEndLevelMovie () != MOVIE_NOT_PLAYED))
				StopEndLevelSequence ();
			else {
				int nObject;
				gameStates.app.bEndLevelSequence = EL_LOOKBACK;
				nObject = CreateObject (OBJ_CAMERA, 0, -1, 
												gameData.objs.console->nSegment, 
												&gameData.objs.console->position.vPos, 
												&gameData.objs.console->position.mOrient, 0, 
												CT_NONE, MT_NONE, RT_NONE, 1);
				if (nObject == -1) { //can't get tObject, so abort
#if TRACE
					con_printf (1, "Can't get tObject for endlevel sequence.  Aborting endlevel sequence.\n");
#endif
					StopEndLevelSequence ();
					return;
				}
				gameData.objs.viewer = gameData.objs.endLevelCamera = OBJECTS + nObject;
				SelectCockpit (CM_LETTERBOX);
				gameOpts->render.cockpit.bHUD = 0;	//will be restored by reading plr file when loading next level
				exitFlightObjects [1] = exitFlightObjects [0];
				exitFlightObjects [1].objP = gameData.objs.endLevelCamera;
				exitFlightObjects [1].speed = (5 * gameData.endLevel.xCurFlightSpeed) / 4;
				exitFlightObjects [1].offset_frac = 0x4000;
				VmVecScaleInc (&gameData.objs.endLevelCamera->position.vPos, &gameData.objs.endLevelCamera->position.mOrient.fVec, i2f (7));
				timer=0x20000;
				}
			}
		break;
		}

	case EL_LOOKBACK: {
		DoEndLevelFlyThrough (0);
		DoEndLevelFlyThrough (1);
		if (timer > 0) {
			timer -= gameData.time.xFrame;
			if (timer < 0)		//reduce speed
				exitFlightObjects [1].speed = exitFlightObjects [0].speed;
			}
		if (gameData.objs.endLevelCamera->nSegment == gameData.endLevel.exit.nSegNum) {
			vmsAngVec cam_angles, exit_seg_angles;
			gameStates.app.bEndLevelSequence = EL_OUTSIDE;
			timer = i2f (2);
			VmVecNegate (&gameData.objs.endLevelCamera->position.mOrient.fVec);
			VmVecNegate (&gameData.objs.endLevelCamera->position.mOrient.rVec);
			VmExtractAnglesMatrix (&cam_angles, &gameData.objs.endLevelCamera->position.mOrient);
			VmExtractAnglesMatrix (&exit_seg_angles, &gameData.endLevel.exit.mOrient);
			bank_rate = (-exit_seg_angles.b - cam_angles.b)/2;
			gameData.objs.console->controlType = gameData.objs.endLevelCamera->controlType = CT_NONE;
#ifdef SLEW_ON
			slewObjP = gameData.objs.endLevelCamera;
#endif
			}
		break;
		}

	case EL_OUTSIDE: {
#ifndef SLEW_ON
		vmsAngVec cam_angles;
#endif
		VmVecScaleInc (&gameData.objs.console->position.vPos, &gameData.objs.console->position.mOrient.fVec, FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed));
#ifndef SLEW_ON
		VmVecScaleInc (&gameData.objs.endLevelCamera->position.vPos, 
							&gameData.objs.endLevelCamera->position.mOrient.fVec, 
							FixMul (gameData.time.xFrame, -2*gameData.endLevel.xCurFlightSpeed));
		VmVecScaleInc (&gameData.objs.endLevelCamera->position.vPos, 
							&gameData.objs.endLevelCamera->position.mOrient.uVec, 
							FixMul (gameData.time.xFrame, -gameData.endLevel.xCurFlightSpeed/10));
		VmExtractAnglesMatrix (&cam_angles, &gameData.objs.endLevelCamera->position.mOrient);
		cam_angles.b += (fixang) FixMul (bank_rate, gameData.time.xFrame);
		VmAngles2Matrix (&gameData.objs.endLevelCamera->position.mOrient, &cam_angles);
#endif
		timer -= gameData.time.xFrame;
		if (timer < 0) {
			gameStates.app.bEndLevelSequence = EL_STOPPED;
			VmExtractAnglesMatrix (&vPlayerAngles, &gameData.objs.console->position.mOrient);
			timer = i2f (3);
			}
		break;
		}

	case EL_STOPPED: {
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevel.station.vPos, &gameData.objs.console->position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		VmAngles2Matrix (&gameData.objs.console->position.mOrient, &vPlayerAngles);
		VmVecScaleInc (&gameData.objs.console->position.vPos, &gameData.objs.console->position.mOrient.fVec, FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed));
		timer -= gameData.time.xFrame;
		if (timer < 0) {
#ifdef SLEW_ON
			slewObjP = gameData.objs.endLevelCamera;
			DoSlewMovement (gameData.objs.endLevelCamera, 1, 1);
			timer += gameData.time.xFrame;		//make time stop
			break;
#else
#	ifdef SHORT_SEQUENCE
			StopEndLevelSequence ();
#	else
			gameStates.app.bEndLevelSequence = EL_PANNING;
			VmExtractAnglesMatrix (&vCurrentCameraAngles, &gameData.objs.endLevelCamera->position.mOrient);
			timer = i2f (3);
			if (gameData.app.nGameMode & GM_MULTI) { // try to skip part of the seq if multiplayer
				StopEndLevelSequence ();
				return;
				}
#	endif		//SHORT_SEQUENCE
#endif		//SLEW_ON
			}
		break;
		}
	#ifndef SHORT_SEQUENCE
	case EL_PANNING: {
#ifndef SLEW_ON
		int mask;
#endif
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevel.station.vPos, &gameData.objs.console->position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		VmAngles2Matrix (&gameData.objs.console->position.mOrient, &vPlayerAngles);
		VmVecScaleInc (&gameData.objs.console->position.vPos, &gameData.objs.console->position.mOrient.fVec, FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed);

#ifdef SLEW_ON
		DoSlewMovement (gameData.objs.endLevelCamera, 1, 1);
#else
		GetAnglesToObject (&vDesiredCameraAngles, &gameData.objs.console->position.vPos, &gameData.objs.endLevelCamera->position.vPos);
		mask = ChaseAngles (&vCurrentCameraAngles, &vDesiredCameraAngles);
		VmAngles2Matrix (&gameData.objs.endLevelCamera->position.mOrient, &vCurrentCameraAngles);
		if ((mask & 5) == 5) {
			vmsVector tvec;
			gameStates.app.bEndLevelSequence = EL_CHASING;
			VmVecNormalizedDir (&tvec, &gameData.endLevel.station.vPos, &gameData.objs.console->position.vPos);
			VmVector2Matrix (&gameData.objs.console->position.mOrient, &tvec, &mSurfaceOrient.uVec, NULL);
			gameData.endLevel.xDesiredFlightSpeed *= 2;
			}
#endif
		break;
	}

	case EL_CHASING: 
		{
		fix d, speed_scale;

#ifdef SLEW_ON
		DoSlewMovement (gameData.objs.endLevelCamera, 1, 1);
#endif
		GetAnglesToObject (&vDesiredCameraAngles, &gameData.objs.console->position.vPos, &gameData.objs.endLevelCamera->position.vPos);
		ChaseAngles (&vCurrentCameraAngles, &vDesiredCameraAngles);
#ifndef SLEW_ON
		VmAngles2Matrix (&gameData.objs.endLevelCamera->position.mOrient, &vCurrentCameraAngles);
#endif
		d = VmVecDistQuick (&gameData.objs.console->position.vPos, &gameData.objs.endLevelCamera->position.vPos);
		speed_scale = FixDiv (d, i2f (0x20);
		if (d < f1_0) 
			d = f1_0;
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevel.station.vPos, &gameData.objs.console->position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		VmAngles2Matrix (&gameData.objs.console->position.mOrient, &vPlayerAngles);
		VmVecScaleInc (&gameData.objs.console->position.vPos, &gameData.objs.console->position.mOrient.fVec, FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed);
#ifndef SLEW_ON
		VmVecScaleInc (&gameData.objs.endLevelCamera->position.vPos, &gameData.objs.endLevelCamera->position.mOrient.fVec, FixMul (gameData.time.xFrame, FixMul (speed_scale, gameData.endLevel.xCurFlightSpeed));
		if (VmVecDist (&gameData.objs.console->position.vPos, &gameData.endLevel.station.vPos) < i2f (10))
			StopEndLevelSequence ();
#endif
		break;
		}
#endif		//ifdef SHORT_SEQUENCE
	}
}

//------------------------------------------------------------------------------

#define MIN_D 0x100

//find which tSide to fly out of
int FindExitSide (tObject *objP)
{
	vmsVector	prefvec, segcenter, sidevec;
	fix			d, best_val = -f2_0;
	int			best_side, i;
	tSegment		*segP = gameData.segs.segments + objP->nSegment;

//find exit tSide
VmVecNormalizedDir (&prefvec, &objP->position.vPos, &objP->vLastPos);
COMPUTE_SEGMENT_CENTER (&segcenter, segP);
best_side = -1;
for (i = MAX_SIDES_PER_SEGMENT; --i >= 0;) {
	if (segP->children [i] != -1) {
		COMPUTE_SIDE_CENTER (&sidevec, segP, i);
		VmVecNormalizedDir (&sidevec, &sidevec, &segcenter);
		d = VmVecDot (&sidevec, &prefvec);
		if (labs (d) < MIN_D) 
			d = 0;
		if (d > best_val) {
			best_val = d; 
			best_side = i;
			}
		}
	}
Assert (best_side!=-1);
return best_side;
}

//------------------------------------------------------------------------------

void DrawExitModel ()
{
	vmsVector	vModelPos;
	int			f = 15, u = 0;	//21;

VmVecScaleAdd (&vModelPos, &gameData.endLevel.exit.vMineExit, &gameData.endLevel.exit.mOrient.fVec, i2f (f));
VmVecScaleInc (&vModelPos, &gameData.endLevel.exit.mOrient.uVec, i2f (u));
gameStates.app.bD1Model = gameStates.app.bD1Mission && gameStates.app.bD1Data;
DrawPolygonModel (NULL, &vModelPos, &gameData.endLevel.exit.mOrient, NULL, 
						 (gameStates.gameplay.bMineDestroyed) ? gameData.endLevel.exit.nDestroyedModel : gameData.endLevel.exit.nModel, 
						 0, f1_0, NULL, NULL, NULL);
gameStates.app.bD1Model = 0;
}

//------------------------------------------------------------------------------

int nExitPointBmX, nExitPointBmY;

fix xSatelliteSize = i2f (400);

#define SATELLITE_DIST		i2f (1024)
#define SATELLITE_WIDTH		xSatelliteSize
#define SATELLITE_HEIGHT	 ((xSatelliteSize*9)/4)		// ((xSatelliteSize*5)/2)

void RenderExternalScene (fix xEyeOffset)
{
	vmsVector vDelta;
	g3sPoint p, pTop;

gameData.render.mine.viewerEye = gameData.objs.viewer->position.vPos;
if (xEyeOffset)
	VmVecScaleInc (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient.rVec, xEyeOffset);
G3SetViewMatrix (&gameData.objs.viewer->position.vPos, &gameData.objs.viewer->position.mOrient, gameStates.render.xZoom, 1);
GrClearCanvas (BLACK_RGBA);
G3StartInstanceMatrix (&vmdZeroVector, &mSurfaceOrient);
DrawStars ();
G3DoneInstance ();
//draw satellite
G3TransformAndEncodePoint (&p, &gameData.endLevel.satellite.vPos);
G3RotateDeltaVec (&vDelta, &gameData.endLevel.satellite.vUp);
G3AddDeltaVec (&pTop, &p, &vDelta);
if (!(p.p3_codes & CC_BEHIND)&& !(p.p3_flags & PF_OVERFLOW)) {
	int imSave = gameStates.render.nInterpolationMethod;
	gameStates.render.nInterpolationMethod = 0;
	if (!OglBindBmTex (gameData.endLevel.satellite.bmP, 1, 0))
		G3DrawRodTexPoly (gameData.endLevel.satellite.bmP, &p, SATELLITE_WIDTH, &pTop, SATELLITE_WIDTH, f1_0, satUVL);
	gameStates.render.nInterpolationMethod = imSave;
	}
#ifdef STATION_ENABLED
DrawPolygonModel (NULL, &gameData.endLevel.station.vPos, &vmdIdentityMatrix, NULL, 
						gameData.endLevel.station.nModel, 0, f1_0, NULL, NULL);
#endif
RenderTerrain (&gameData.endLevel.exit.vGroundExit, nExitPointBmX, nExitPointBmY);
DrawExitModel ();
if (gameStates.render.bExtExplPlaying)
	DrawFireball (&externalExplosion);
gameStates.render.nLighting = 0;
RenderObject (gameData.objs.console, 0, 0);
RenderItems ();
gameStates.render.nLighting = 1;
}

//------------------------------------------------------------------------------

#define MAX_STARS 500

vmsVector stars [MAX_STARS];

void GenerateStarfield (void)
{
	int i;

for (i = 0; i < MAX_STARS; i++) {
	stars [i].p.x = (d_rand () - RAND_MAX/2) << 14;
	stars [i].p.z = (d_rand () - RAND_MAX/2) << 14;
	stars [i].p.y = (d_rand ()/2) << 14;
	}
}

//------------------------------------------------------------------------------

void DrawStars ()
{
	int i;
	int intensity = 31;
	g3sPoint p;

for (i = 0; i < MAX_STARS; i++) {
	if ((i&63) == 0) {
		GrSetColorRGBi (RGBA_PAL (intensity, intensity, intensity));
		intensity-=3;
		}
	G3RotateDeltaVec (&p.p3_vec, &stars [i]);
	G3EncodePoint (&p);
	if (p.p3_codes == 0) {
		p.p3_flags &= ~PF_PROJECTED;
		G3ProjectPoint (&p);
		gr_pixel (p.p3_screen.x, p.p3_screen.y);
		}
	}
}

//------------------------------------------------------------------------------

void RenderEndLevelMine (fix xEyeOffset, int nWindowNum)
{
	short nStartSeg;

gameData.render.mine.viewerEye = gameData.objs.viewer->position.vPos;
if (gameData.objs.viewer->nType == OBJ_PLAYER)
	VmVecScaleInc (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient.fVec, (gameData.objs.viewer->size * 3) / 4);
if (xEyeOffset)
	VmVecScaleInc (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient.rVec, xEyeOffset);
#ifdef EDITOR
if (gameStates.app.nFunctionMode == FMODE_EDITOR)
	gameData.render.mine.viewerEye = gameData.objs.viewer->position.vPos;
#endif
if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE) {
	nStartSeg = gameData.endLevel.exit.nSegNum;
	}
else {
	nStartSeg = FindSegByPos (&gameData.render.mine.viewerEye, gameData.objs.viewer->nSegment, 1, 0);
	if (nStartSeg == -1)
		nStartSeg = gameData.objs.viewer->nSegment;
	}
if (gameStates.app.bEndLevelSequence == EL_LOOKBACK) {
	vmsMatrix headm, viewm;
	vmsAngVec angles = {0, 0, 0x7fff};
	VmAngles2Matrix (&headm, &angles);
	VmMatMul (&viewm, &gameData.objs.viewer->position.mOrient, &headm);
	G3SetViewMatrix (&gameData.render.mine.viewerEye, &viewm, gameStates.render.xZoom, 1);
	}
else
	G3SetViewMatrix (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient, gameStates.render.xZoom, 1);
RenderMine (nStartSeg, xEyeOffset, nWindowNum);
RenderItems ();
}

//-----------------------------------------------------------------------------

void RenderEndLevelFrame (fix xEyeOffset, int nWindowNum)
{
G3StartFrame (0, !nWindowNum);
//gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
if (gameStates.app.bEndLevelSequence < EL_OUTSIDE)
	RenderEndLevelMine (xEyeOffset, nWindowNum);
else if (!nWindowNum)
	RenderExternalScene (xEyeOffset);
G3EndFrame ();
}

//------------------------------------------------------------------------------
///////////////////////// copy of flythrough code for endlevel

int ELFindConnectedSide (int seg0, int seg1);

fixang DeltaAng (fixang a, fixang b);

#define DEFAULT_SPEED i2f (16)

#define MIN_D 0x100

//if speed is zero, use default speed
void StartEndLevelFlyThrough (int n, tObject *objP, fix speed)
{
exitFlightDataP = exitFlightObjects + n;
exitFlightDataP->objP = objP;
exitFlightDataP->firstTime = 1;
exitFlightDataP->speed = speed ? speed : DEFAULT_SPEED;
exitFlightDataP->offset_frac = 0;
SongsPlaySong (SONG_INTER, 0);
}

//------------------------------------------------------------------------------

static vmsAngVec *angvec_add2_scale (vmsAngVec *dest, vmsVector *src, fix s)
{
dest->p += (fixang) FixMul (src->p.x, s);
dest->b += (fixang) FixMul (src->p.z, s);
dest->h += (fixang) FixMul (src->p.y, s);
return dest;
}

//-----------------------------------------------------------------------------

#define MAX_ANGSTEP	0x4000		//max turn per second

#define MAX_SLIDE_PER_SEGMENT 0x10000

void DoEndLevelFlyThrough (int n)
{
	tObject *objP;
	tSegment *segP;
	int old_player_seg;

exitFlightDataP = exitFlightObjects + n;
objP = exitFlightDataP->objP;
old_player_seg = objP->nSegment;

//move the tPlayer for this frame

if (!exitFlightDataP->firstTime) {
	VmVecScaleInc (&objP->position.vPos, &exitFlightDataP->step, gameData.time.xFrame);
	angvec_add2_scale (&exitFlightDataP->angles, &exitFlightDataP->angstep, gameData.time.xFrame);
	VmAngles2Matrix (&objP->position.mOrient, &exitFlightDataP->angles);
	}
//check new tPlayer seg
UpdateObjectSeg (objP);
segP = gameData.segs.segments + objP->nSegment;
if (exitFlightDataP->firstTime || (objP->nSegment != old_player_seg)) {		//moved into new seg
	vmsVector curcenter, nextcenter;
	fix step_size, segTime;
	short nEntrySide, nExitSide = -1;//what sides we entry and leave through
	vmsVector dest_point;		//where we are heading (center of nExitSide)
	vmsAngVec dest_angles;		//where we want to be pointing
	vmsMatrix dest_orient;
	int up_side = 0;

	nEntrySide = 0;
	//find new exit tSide
	if (!exitFlightDataP->firstTime) {
		nEntrySide = ELFindConnectedSide (objP->nSegment, old_player_seg);
		nExitSide = sideOpposite [nEntrySide];
		}
	if (exitFlightDataP->firstTime || nEntrySide==-1 || segP->children [nExitSide]==-1)
		nExitSide = FindExitSide (objP);
	{										//find closest tSide to align to
		fix d, largest_d=-f1_0;
		int i;

		for (i = 0; i < 6; i++) {
			d = VmVecDot (&segP->sides [i].normals [0], &exitFlightDataP->objP->position.mOrient.uVec);
			if (d > largest_d) {largest_d = d; up_side=i;}
			}
		}
	//update target point & angles
	COMPUTE_SIDE_CENTER (&dest_point, segP, nExitSide);
	//update target point and movement points
	//offset tObject sideways
	if (exitFlightDataP->offset_frac) {
		int s0=-1, s1=0, i;
		vmsVector s0p, s1p;
		fix dist;

		for (i = 0; i < 6; i++)
			if (i!=nEntrySide && i!=nExitSide && i!=up_side && i!=sideOpposite [up_side]) {
				if (s0==-1)
					s0 = i;
				else
					s1 = i;
				}
		COMPUTE_SIDE_CENTER (&s0p, segP, s0);
		COMPUTE_SIDE_CENTER (&s1p, segP, s1);
		dist = FixMul (VmVecDist (&s0p, &s1p), exitFlightDataP->offset_frac);
		if (dist-exitFlightDataP->offsetDist > MAX_SLIDE_PER_SEGMENT)
			dist = exitFlightDataP->offsetDist + MAX_SLIDE_PER_SEGMENT;
		exitFlightDataP->offsetDist = dist;
		VmVecScaleInc (&dest_point, &objP->position.mOrient.rVec, dist);
		}
	VmVecSub (&exitFlightDataP->step, &dest_point, &objP->position.vPos);
	step_size = VmVecNormalize (&exitFlightDataP->step);
	VmVecScale (&exitFlightDataP->step, exitFlightDataP->speed);
	COMPUTE_SEGMENT_CENTER (&curcenter, segP);
	COMPUTE_SEGMENT_CENTER_I (&nextcenter, segP->children [nExitSide]);
	VmVecSub (&exitFlightDataP->headvec, &nextcenter, &curcenter);
	VmVector2Matrix (&dest_orient, &exitFlightDataP->headvec, &segP->sides [up_side].normals [0], NULL);
	VmExtractAnglesMatrix (&dest_angles, &dest_orient);
	if (exitFlightDataP->firstTime)
		VmExtractAnglesMatrix (&exitFlightDataP->angles, &objP->position.mOrient);
	segTime = FixDiv (step_size, exitFlightDataP->speed);	//how long through seg
	if (segTime) {
		exitFlightDataP->angstep.p.x = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (exitFlightDataP->angles.p, dest_angles.p), segTime)));
		exitFlightDataP->angstep.p.z = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (exitFlightDataP->angles.b, dest_angles.b), segTime)));
		exitFlightDataP->angstep.p.y = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (exitFlightDataP->angles.h, dest_angles.h), segTime)));
		}
	else {
		exitFlightDataP->angles = dest_angles;
		exitFlightDataP->angstep.p.x = 
		exitFlightDataP->angstep.p.y = 
		exitFlightDataP->angstep.p.z = 0;
		}
	}
exitFlightDataP->firstTime=0;
}

//------------------------------------------------------------------------------

#ifdef SLEW_ON		//this is a special routine for slewing around external scene
int DoSlewMovement (tObject *objP, int check_keys, int check_joy)
{
	int moved = 0;
	vmsVector svel, movement;				//scaled velocity (per this frame)
	vmsMatrix rotmat, new_pm;
	int joy_x, joy_y, btns;
	int joyx_moved, joyy_moved;
	vmsAngVec rotang;

if (gameStates.input.keys.pressed [KEY_PAD5])
	VmVecZero (&objP->physInfo.velocity);

if (check_keys) {
	objP->physInfo.velocity.x += VEL_SPEED * (KeyDownTime (KEY_PAD9) - KeyDownTime (KEY_PAD7);
	objP->physInfo.velocity.y += VEL_SPEED * (KeyDownTime (KEY_PADMINUS) - KeyDownTime (KEY_PADPLUS);
	objP->physInfo.velocity.z += VEL_SPEED * (KeyDownTime (KEY_PAD8) - KeyDownTime (KEY_PAD2);

	rotang.pitch = (KeyDownTime (KEY_LBRACKET) - KeyDownTime (KEY_RBRACKET))/ROT_SPEED;
	rotang.bank  = (KeyDownTime (KEY_PAD1) - KeyDownTime (KEY_PAD3))/ROT_SPEED;
	rotang.head  = (KeyDownTime (KEY_PAD6) - KeyDownTime (KEY_PAD4))/ROT_SPEED;
	}
	else
		rotang.pitch = rotang.bank  = rotang.head  = 0;
//check for joystick movement
if (check_joy && bJoyPresent)	{
	JoyGetpos (&joy_x, &joy_y);
	btns=JoyGetBtns ();
	joyx_moved = (abs (joy_x - old_joy_x)>JOY_NULL);
	joyy_moved = (abs (joy_y - old_joy_y)>JOY_NULL);
	if (abs (joy_x) < JOY_NULL) joy_x = 0;
	if (abs (joy_y) < JOY_NULL) joy_y = 0;
	if (btns)
		if (!rotang.pitch) rotang.pitch = FixMul (-joy_y * 512, gameData.time.xFrame); else;
	else
		if (joyy_moved) objP->physInfo.velocity.z = -joy_y * 8192;
	if (!rotang.head) rotang.head = FixMul (joy_x * 512, gameData.time.xFrame);
	if (joyx_moved) 
		old_joy_x = joy_x;
	if (joyy_moved) 
		old_joy_y = joy_y;
	}
moved = rotang.pitch | rotang.bank | rotang.head;
VmAngles2Matrix (&rotmat, &rotang);
VmMatMul (&new_pm, &objP->position.mOrient, &rotmat);
objP->position.mOrient = new_pm;
VmTransposeMatrix (&new_pm);		//make those columns rows
moved |= objP->physInfo.velocity.x | objP->physInfo.velocity.y | objP->physInfo.velocity.z;
svel = objP->physInfo.velocity;
VmVecScale (&svel, gameData.time.xFrame);		//movement in this frame
VmVecRotate (&movement, &svel, &new_pm);
VmVecInc (&objP->position.vPos, &movement);
moved |= (movement.x || movement.y || movement.z);
return moved;
}
#endif

//-----------------------------------------------------------------------------

#define LINE_LEN	80
#define NUM_VARS	8

#define STATION_DIST	i2f (1024)

int ConvertExt (char *dest, const char *ext)
{
	char *t = strchr (dest, '.');

if (t && (t-dest <= 8)) {
	t [1] = ext [0];		
	t [2] = ext [1];		
	t [3] = ext [2];
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

extern char szAutoMission [255];
//called for each level to load & setup the exit sequence
void LoadEndLevelData (int nLevel)
{
	char		filename [13];
	char		line [LINE_LEN], *p;
	CFILE		cf;
	int		var, nSegment, nSide;
	int		nExitSide = 0, i;
	int		bHaveBinary = 0;

gameStates.app.bEndLevelDataLoaded = 0;		//not loaded yet
if (!gameOpts->movies.nLevel)
	return;

try_again:

if (gameStates.app.bAutoRunMission)
	strcpy (filename, szAutoMission);
else if (nLevel<0)		//secret level
	strcpy (filename, gameData.missions.szSecretLevelNames [-nLevel-1]);
else					//normal level
	strcpy (filename, gameData.missions.szLevelNames [nLevel-1]);
if (!ConvertExt (filename, "end"))
	Error ("Error converting filename\n'<%s>'\nfor endlevel data\n", filename);
if (!CFOpen (&cf, filename, gameFolders.szDataDir, "rb", gameStates.app.bD1Mission)) {
	ConvertExt (filename, "txb");
	if (!CFOpen (&cf, filename, gameFolders.szDataDir, "rb", gameStates.app.bD1Mission)) {
		if (nLevel == 1) {
#if TRACE
			con_printf (CONDBG, "Cannot load file text\nof binary version of\n'<%s>'\n", filename);
#endif
			gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
			return;
			}
		else {
			nLevel = 1;
			goto try_again;
			}
		}
	bHaveBinary = 1;
	}

//ok...this parser is pretty simple.  It ignores comments, but
//everything else must be in the right place
var = 0;
PrintLog ("      parsing endlevel description\n");
while (CFGetS (line, LINE_LEN, &cf)) {
	if (bHaveBinary) {
		int l = (int) strlen (line);
		for (i = 0; i < l; i++)
			line [i] = EncodeRotateLeft ((char) (EncodeRotateLeft (line [i]) ^ BITMAP_TBL_XOR));
		p = line;
		}
	if ((p = strchr (line, ';')))
		*p = 0;		//cut off comment
	for (p = line + strlen (line) - 1; (p > line) && ::isspace ((unsigned char) *p); *p-- = 0)
		;
	for (p = line; ::isspace ((unsigned char) *p); p++)
		;
	if (!*p)		//empty line
		continue;
	switch (var) {
		case 0: {						//ground terrain
			int iff_error;

			PrintLog ("         loading terrain bitmap\n");
			if (gameData.endLevel.terrain.bmInstance.bmTexBuf) {
				OglFreeBmTexture (&gameData.endLevel.terrain.bmInstance);
				D2_FREE (gameData.endLevel.terrain.bmInstance.bmTexBuf);
				}
			Assert (gameData.endLevel.terrain.bmInstance.bmTexBuf == NULL);
			iff_error = iff_read_bitmap (p, &gameData.endLevel.terrain.bmInstance, BM_LINEAR);
			if (iff_error != IFF_NO_ERROR) {
#ifdef _DEBUG
				Warning (TXT_EXIT_TERRAIN, p, iff_errormsg (iff_error));
#endif
				gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
				return;
				}
			gameData.endLevel.terrain.bmP = &gameData.endLevel.terrain.bmInstance;
			//GrRemapBitmapGood (gameData.endLevel.terrain.bmP, NULL, iff_transparent_color, -1);
			break;
		}

		case 1:							//height map
			PrintLog ("         loading endlevel terrain\n");
			LoadTerrain (p);
			break;

		case 2:
			PrintLog ("         loading exit point\n");
			sscanf (p, "%d, %d", &nExitPointBmX, &nExitPointBmY);
			break;

		case 3:							//exit heading
			PrintLog ("         loading exit angle\n");
			vExitAngles.h = i2f (atoi (p))/360;
			break;

		case 4: {						//planet bitmap
			int iff_error;

			PrintLog ("         loading satellite bitmap\n");
			if (gameData.endLevel.satellite.bmInstance.bmTexBuf) {
				OglFreeBmTexture (&gameData.endLevel.satellite.bmInstance);
				D2_FREE (gameData.endLevel.satellite.bmInstance.bmTexBuf);
				}
			iff_error = iff_read_bitmap (p, &gameData.endLevel.satellite.bmInstance, BM_LINEAR);
			if (iff_error != IFF_NO_ERROR) {
				Warning (TXT_SATELLITE, p, iff_errormsg (iff_error));
				gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
				return;
			}
			gameData.endLevel.satellite.bmP = &gameData.endLevel.satellite.bmInstance;
			//GrRemapBitmapGood (gameData.endLevel.satellite.bmP, NULL, iff_transparent_color, -1);
			break;
		}

		case 5:							//earth pos
		case 7: {						//station pos
			vmsMatrix tm;
			vmsAngVec ta;
			int pitch, head;

			PrintLog ("         loading satellite and station position\n");
			sscanf (p, "%d, %d", &head, &pitch);
			ta.h = i2f (head)/360;
			ta.p = -i2f (pitch)/360;
			ta.b = 0;
			VmAngles2Matrix (&tm, &ta);
			if (var == 5)
				gameData.endLevel.satellite.vPos = tm.fVec;
				//VmVecCopyScale (&gameData.endLevel.satellite.vPos, &tm.fVec, SATELLITE_DIST);
			else
				gameData.endLevel.station.vPos = tm.fVec;
			break;
		}

		case 6:						//planet size
			PrintLog ("         loading satellite size\n");
			xSatelliteSize = i2f (atoi (p));
			break;
	}
	var++;
}

Assert (var == NUM_VARS);
// OK, now the data is loaded.  Initialize everything
//find the exit sequence by searching all segments for a tSide with
//children == -2
for (nSegment = 0, gameData.endLevel.exit.nSegNum = -1;
	  (gameData.endLevel.exit.nSegNum == -1) && (nSegment <= gameData.segs.nLastSegment);
	  nSegment++)
	for (nSide = 0; nSide < 6; nSide++)
		if (gameData.segs.segments [nSegment].children [nSide] == -2) {
			gameData.endLevel.exit.nSegNum = nSegment;
			nExitSide = nSide;
			break;
			}

Assert (gameData.endLevel.exit.nSegNum!=-1);
PrintLog ("      computing endlevel element orientation\n");
COMPUTE_SEGMENT_CENTER_I (&gameData.endLevel.exit.vMineExit, gameData.endLevel.exit.nSegNum);
ExtractOrientFromSegment (&gameData.endLevel.exit.mOrient, &gameData.segs.segments [gameData.endLevel.exit.nSegNum]);
COMPUTE_SIDE_CENTER_I (&gameData.endLevel.exit.vSideExit, gameData.endLevel.exit.nSegNum, nExitSide);
VmVecScaleAdd (&gameData.endLevel.exit.vGroundExit, &gameData.endLevel.exit.vMineExit, &gameData.endLevel.exit.mOrient.uVec, -i2f (20));
//compute orientation of surface
{
	vmsVector tv;
	vmsMatrix exit_orient, tm;

	VmAngles2Matrix (&exit_orient, &vExitAngles);
	VmTransposeMatrix (&exit_orient);
	VmMatMul (&mSurfaceOrient, &gameData.endLevel.exit.mOrient, &exit_orient);
	VmCopyTransposeMatrix (&tm, &mSurfaceOrient);
	VmVecRotate (&tv, &gameData.endLevel.station.vPos, &tm);
	VmVecScaleAdd (&gameData.endLevel.station.vPos, &gameData.endLevel.exit.vMineExit, &tv, STATION_DIST);
	VmVecRotate (&tv, &gameData.endLevel.satellite.vPos, &tm);
	VmVecScaleAdd (&gameData.endLevel.satellite.vPos, &gameData.endLevel.exit.vMineExit, &tv, SATELLITE_DIST);
	VmVector2Matrix (&tm, &tv, &mSurfaceOrient.uVec, NULL);
	VmVecCopyScale (&gameData.endLevel.satellite.vUp, &tm.uVec, SATELLITE_HEIGHT);
	}
CFClose (&cf);
gameStates.app.bEndLevelDataLoaded = 1;
}

//------------------------------------------------------------------------------
//eof
