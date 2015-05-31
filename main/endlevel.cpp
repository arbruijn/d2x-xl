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

#include "descent.h"
#include "error.h"
#include "palette.h"
#include "iff.h"
#include "texmap.h"
#include "u_mem.h"
#include "endlevel.h"
#include "objrender.h"
#include "transprender.h"
#include "screens.h"
#include "cockpit.h"
#include "terrain.h"
#include "newdemo.h"
#include "fireball.h"
#include "network.h"
#include "text.h"
#include "compbit.h"
#include "movie.h"
#include "rendermine.h"
#include "segmath.h"
#include "key.h"
#include "joy.h"
#include "songs.h"

//------------------------------------------------------------------------------

typedef struct tExitFlightData {
	CObject		*pObj;
	CAngleVector	angles;			//orientation in angles
	CFixVector	vStep;				//how far in a second
	CFixVector	aStep;			//rotation per second
	fix			speed;			//how fast CObject is moving
	CFixVector 	vHeading;			//where we want to be pointing
	int32_t			bStart;		//flag for if first time through
	fix			lateralOffset;	//how far off-center as portion of way
	fix			offsetDist;	//how far currently off-center
} tExitFlightData;

//endlevel sequence states

#define SHORT_SEQUENCE	1		//if defined, end sequnce when panning starts
//#define STATION_ENABLED	1		//if defined, load & use space station model

#define FLY_SPEED I2X (50)

void DoEndLevelFlyThrough (int32_t n);
void DrawStars ();
int32_t FindExitSide (CObject *pObj);
void GenerateStarfield ();
void StartEndLevelFlyThrough (int32_t n, CObject *pObj, fix speed);
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

#define FLY_ACCEL I2X (5)

//static tUVL satUVL [4] = {{0,0,I2X (1)},{I2X (1),0,I2X (1)},{I2X (1),I2X (1),I2X (1)},{0,I2X (1),I2X (1)}};
static tUVL satUVL [4] = {{0,0,I2X (1)},{I2X (1),0,I2X (1)},{I2X (1),I2X (1),I2X (1)},{0,I2X (1),I2X (1)}};

extern int32_t ELFindConnectedSide (int32_t seg0, int32_t seg1);

//------------------------------------------------------------------------------

#define MAX_EXITFLIGHT_OBJS 2

tExitFlightData exitFlightObjects [MAX_EXITFLIGHT_OBJS];
tExitFlightData *pExitFlightData;

//------------------------------------------------------------------------------

void InitEndLevelData (void)
{
gameData.endLevelData.station.vPos.v.coord.x = 0xf8c4 << 10;
gameData.endLevelData.station.vPos.v.coord.y = 0x3c1c << 12;
gameData.endLevelData.station.vPos.v.coord.z = 0x0372 << 10;
}

//------------------------------------------------------------------------------
//find delta between two angles
inline fixang DeltaAng (fixang a, fixang b)
{
fixang delta0, delta1;

return (abs (delta0 = a - b) < abs (delta1 = b - a)) ? delta0 : delta1;
}

//------------------------------------------------------------------------------
//return though which CSide of seg0 is seg1
int32_t ELFindConnectedSide (int32_t nSegment, int32_t nChildSeg)
{
CSegment *pSeg = SEGMENT (nSegment);

for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++)
	if (pSeg->m_children [i] == nChildSeg)
		return i;
return -1;
}

//------------------------------------------------------------------------------

#define MOVIE_REQUIRED 1

//returns movie played status.  see movie.h
int32_t StartEndLevelMovie (void)
{
	char szMovieName [FILENAME_LEN];
	int32_t r;

strcpy (szMovieName, gameStates.app.bD1Mission ? "exita.mve" : "esa.mve");
if (!IS_D2_OEM)
	if (missionManager.nCurrentLevel == missionManager.nLastLevel)
		return 1;   //don't play movie
if (missionManager.nCurrentLevel > 0)
	if (gameStates.app.bD1Mission)
		szMovieName [4] = movieTable [1][missionManager.nCurrentLevel-1];
	else
		szMovieName [2] = movieTable [0][missionManager.nCurrentLevel-1];
else if (gameStates.app.bD1Mission) {
	szMovieName [4] = movieTable [1][26 - missionManager.nCurrentLevel];
	}
else {
#ifndef SHAREWARE
	return 0;       //no escapes for secret level
#else
	Error ("Invalid level number <%d>", missionManager.nCurrentLevel);
#endif
}
#ifndef SHAREWARE
r = movieManager.Play (szMovieName, IsMultiGame ? 0 : MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
#else
return 0;	// movie not played for shareware
#endif
if (gameData.demoData.nState == ND_STATE_PLAYBACK)
	SetScreenMode (SCREEN_GAME);
gameData.scoreData.bNoMovieMessage = (r == MOVIE_NOT_PLAYED) && IsMultiGame;
return r;
}

//------------------------------------------------------------------------------

void _CDECL_ FreeEndLevelData (void)
{
PrintLog (1, "unloading endlevel data\n");
if (gameData.endLevelData.terrain.bmInstance.Buffer ()) {
	gameData.endLevelData.terrain.bmInstance.ReleaseTexture ();
	gameData.endLevelData.terrain.bmInstance.DestroyBuffer ();
	}
if (gameData.endLevelData.satellite.bmInstance.Buffer ()) {
	gameData.endLevelData.satellite.bmInstance.ReleaseTexture ();
	gameData.endLevelData.satellite.bmInstance.DestroyBuffer ();
	}
PrintLog (-1);
}

//-----------------------------------------------------------------------------

void InitEndLevel (void)
{
#ifdef STATION_ENABLED
	gameData.endLevelData.station.pBm = bm_load ("steel3.bbm");
	gameData.endLevelData.station.bmList [0] = &gameData.endLevelData.station.pBm;

	gameData.endLevelData.station.nModel = LoadPolyModel ("station.pof", 1, gameData.endLevelData.station.bmList, NULL);
#endif
GenerateStarfield ();
atexit (FreeEndLevelData);
gameData.endLevelData.terrain.bmInstance.SetBuffer (NULL);
gameData.endLevelData.satellite.bmInstance.SetBuffer (NULL);
}

//------------------------------------------------------------------------------

CObject externalExplosion;

CAngleVector vExitAngles = CAngleVector::Create(-0xa00, 0, 0);

CFixMatrix mSurfaceOrient;

void StartEndLevelSequence (int32_t bSecret)
{
	CObject*		pObj;
	int32_t		nMoviePlayed = MOVIE_NOT_PLAYED;

if (gameData.demoData.nState == ND_STATE_RECORDING)		// stop demo recording
	gameData.demoData.nState = ND_STATE_PAUSED;
if (gameData.demoData.nState == ND_STATE_PLAYBACK) {		// don't do this if in playback mode
	if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) ||
		 ((missionManager.nCurrentMission == missionManager.nBuiltInMission [1]) &&
		 movieManager.m_bHaveExtras))
		StartEndLevelMovie ();
	paletteManager.SetLastLoaded ("");		//force palette load next time
	return;
	}
if (gameStates.app.bPlayerIsDead || (gameData.objData.pConsole->info.nFlags & OF_SHOULD_BE_DEAD))
	return;				//don't start if dead!
//	Dematerialize Buddy!
FORALL_ROBOT_OBJS (pObj)
	if (pObj->IsGuideBot ()) {
			CreateExplosion (pObj->info.nSegment, pObj->info.position.vPos, I2X (7) / 2, ANIM_POWERUP_DISAPPEARANCE);
			pObj->Die ();
		}
LOCALPLAYER.homingObjectDist = -I2X (1); // Turn off homing sound.
ResetRearView ();		//turn off rear view if set
if (IsMultiGame) {
	MultiSendEndLevelStart (0);
	NetworkDoFrame (1);
	}
if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) ||
	 ((missionManager.nCurrentMission == missionManager.nBuiltInMission [1]) && movieManager.m_bHaveExtras)) {
	// only play movie for built-in mission
	if (!IsMultiGame)
		nMoviePlayed = StartEndLevelMovie ();
	}
if ((nMoviePlayed == MOVIE_NOT_PLAYED) && gameStates.app.bEndLevelDataLoaded) {   //don't have movie.  Do rendered sequence, if available
	int32_t bExitModelsLoaded = (gameData.pigData.tex.nHamFileVersion < 3) ? 1 : LoadExitModels ();
	if (bExitModelsLoaded) {
		StartRenderedEndLevelSequence ();
		return;
		}
	}
//don't have movie or rendered sequence, fade out
paletteManager.DisableEffect ();
if (!bSecret)
	PlayerFinishedLevel (0);		//done with level
}

//------------------------------------------------------------------------------

void StartRenderedEndLevelSequence (void)
{
	int32_t nExitSide, nTunnelLength;
	int32_t nSegment, nOldSeg, nEntrySide, i;

//count segments in exit tunnel
nOldSeg = gameData.objData.pConsole->info.nSegment;
nExitSide = FindExitSide (gameData.objData.pConsole);
nSegment = SEGMENT (nOldSeg)->m_children [nExitSide];
nTunnelLength = 0;
do {
	nEntrySide = ELFindConnectedSide (nSegment, nOldSeg);
	nExitSide = oppSideTable [nEntrySide];
	nOldSeg = nSegment;
	nSegment = SEGMENT (nSegment)->m_children [nExitSide];
	nTunnelLength++;
	} while (nSegment >= 0);
if (nSegment != -2) {
	PlayerFinishedLevel (0);		//don't do special sequence
	return;
	}
//now pick transition nSegment 1/3 of the way in
nOldSeg = gameData.objData.pConsole->info.nSegment;
nExitSide = FindExitSide (gameData.objData.pConsole);
nSegment = SEGMENT (nOldSeg)->m_children [nExitSide];
i = nTunnelLength / 3;
while (i--) {
	nEntrySide = ELFindConnectedSide (nSegment, nOldSeg);
	nExitSide = oppSideTable [nEntrySide];
	nOldSeg = nSegment;
	nSegment = SEGMENT (nSegment)->m_children [nExitSide];
	}
gameData.endLevelData.exit.nTransitSegNum = nSegment;
CGenericCockpit::Save ();
if (IsMultiGame) {
	MultiSendEndLevelStart (0);
	NetworkDoFrame (1);
	}
if (missionManager [missionManager.nCurrentMission].nDescentVersion == 1)
	songManager.Play (SONG_ENDLEVEL, 0);
gameStates.app.bEndLevelSequence = EL_FLYTHROUGH;
gameData.objData.pConsole->info.movementType = MT_NONE;			//movement handled by flythrough
gameData.objData.pConsole->info.controlType = CT_NONE;
if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS))
	gameStates.app.bGameSuspended |= SUSP_ROBOTS | SUSP_TEMPORARY;          //robots don't move
gameData.endLevelData.xCurFlightSpeed = gameData.endLevelData.xDesiredFlightSpeed = FLY_SPEED;
StartEndLevelFlyThrough (0, gameData.objData.pConsole, gameData.endLevelData.xCurFlightSpeed);		//initialize
HUDInitMessage (TXT_EXIT_SEQUENCE);
gameStates.render.bOutsideMine = gameStates.render.bExtExplPlaying = 0;
gameStates.render.nFlashScale = 0;
gameStates.gameplay.bMineDestroyed = 0;
}

//------------------------------------------------------------------------------

CAngleVector vPlayerAngles, vPlayerDestAngles;
CAngleVector vDesiredCameraAngles, vCurrentCameraAngles;

#define CHASE_TURN_RATE (0x4000/4)		//max turn per second

//returns bitmask of which angles are at dest. bits 0, 1, 2 = p, b, h
int32_t ChaseAngles (CAngleVector *cur_angles, CAngleVector *desired_angles)
{
	CAngleVector	delta_angs, alt_angles, alt_delta_angs;
	fix			total_delta, altTotal_delta;
	fix			frame_turn;
	int32_t			mask = 0;

delta_angs.v.coord.p = desired_angles->v.coord.p - cur_angles->v.coord.p;
delta_angs.v.coord.h = desired_angles->v.coord.h - cur_angles->v.coord.h;
delta_angs.v.coord.b = desired_angles->v.coord.b - cur_angles->v.coord.b;
total_delta = abs (delta_angs.v.coord.p) + abs (delta_angs.v.coord.b) + abs (delta_angs.v.coord.h);

alt_angles.v.coord.p = I2X (1)/2 - cur_angles->v.coord.p;
alt_angles.v.coord.b = cur_angles->v.coord.b + I2X (1)/2;
alt_angles.v.coord.h = cur_angles->v.coord.h + I2X (1)/2;

alt_delta_angs.v.coord.p = desired_angles->v.coord.p - alt_angles.v.coord.p;
alt_delta_angs.v.coord.h = desired_angles->v.coord.h - alt_angles.v.coord.h;
alt_delta_angs.v.coord.b = desired_angles->v.coord.b - alt_angles.v.coord.b;
//alt_delta_angs.b = 0;

altTotal_delta = abs (alt_delta_angs.v.coord.p) + abs (alt_delta_angs.v.coord.b) + abs (alt_delta_angs.v.coord.h);

////printf ("Total delta = %x, alt total_delta = %x\n", total_delta, altTotal_delta);

if (altTotal_delta < total_delta) {
	////printf ("FLIPPING ANGLES!\n");
	*cur_angles = alt_angles;
	delta_angs = alt_delta_angs;
}

frame_turn = FixMul (gameData.timeData.xFrame, CHASE_TURN_RATE);

if (abs (delta_angs.v.coord.p) < frame_turn) {
	cur_angles->v.coord.p = desired_angles->v.coord.p;
	mask |= 1;
	}
else
	if (delta_angs.v.coord.p > 0)
		cur_angles->v.coord.p += (fixang) frame_turn;
	else
		cur_angles->v.coord.p -= (fixang) frame_turn;

if (abs (delta_angs.v.coord.b) < frame_turn) {
	cur_angles->v.coord.b = (fixang) desired_angles->v.coord.b;
	mask |= 2;
	}
else
	if (delta_angs.v.coord.b > 0)
		cur_angles->v.coord.b += (fixang) frame_turn;
	else
		cur_angles->v.coord.b -= (fixang) frame_turn;
if (abs (delta_angs.v.coord.h) < frame_turn) {
	cur_angles->v.coord.h = (fixang) desired_angles->v.coord.h;
	mask |= 4;
	}
else
	if (delta_angs.v.coord.h > 0)
		cur_angles->v.coord.h += (fixang) frame_turn;
	else
		cur_angles->v.coord.h -= (fixang) frame_turn;
return mask;
}

//------------------------------------------------------------------------------

void StopEndLevelSequence (void)
{
	gameStates.render.nInterpolationMethod = 0;

paletteManager.DisableEffect ();
CGenericCockpit::Restore ();
gameStates.app.bEndLevelSequence = EL_OFF;
PlayerFinishedLevel (0);
}

//------------------------------------------------------------------------------

#define ANIM_BIG_PLAYER_EXPLOSION	58

//--unused-- CFixVector upvec = {0, I2X (1), 0};

//find the angle between the player's heading & the station
inline void GetAnglesToObject (CAngleVector *av, CFixVector *targ_pos, CFixVector *cur_pos)
{
	CFixVector tv = *targ_pos - *cur_pos;
	*av = tv.ToAnglesVec ();
}

//------------------------------------------------------------------------------

void DoEndLevelFrame (void)
{
	static fix timer;
	static fix bank_rate;
	static fix explosion_wait1=0;
	static fix explosion_wait2=0;
	static fix ext_expl_halflife;
	CFixVector vLastPosSave;

vLastPosSave = gameData.objData.pConsole->info.vLastPos;	//don't let move code change this
UpdateAllObjects ();
gameData.objData.pConsole->SetLastPos (vLastPosSave);

if (gameStates.render.bExtExplPlaying) {
	externalExplosion.info.xLifeLeft -= gameData.timeData.xFrame;
	externalExplosion.DoExplosionSequence ();
	if (externalExplosion.info.xLifeLeft < ext_expl_halflife)
		gameStates.gameplay.bMineDestroyed = 1;
	if (externalExplosion.info.nFlags & OF_SHOULD_BE_DEAD)
		gameStates.render.bExtExplPlaying = 0;
	}

if (gameData.endLevelData.xCurFlightSpeed != gameData.endLevelData.xDesiredFlightSpeed) {
	fix delta = gameData.endLevelData.xDesiredFlightSpeed - gameData.endLevelData.xCurFlightSpeed;
	fix frame_accel = FixMul (gameData.timeData.xFrame, FLY_ACCEL);

	if (abs (delta) < frame_accel)
		gameData.endLevelData.xCurFlightSpeed = gameData.endLevelData.xDesiredFlightSpeed;
	else if (delta > 0)
		gameData.endLevelData.xCurFlightSpeed += frame_accel;
	else
		gameData.endLevelData.xCurFlightSpeed -= frame_accel;
	}

//do big explosions
if (!gameStates.render.bOutsideMine) {
	if (gameStates.app.bEndLevelSequence == EL_OUTSIDE) {
		CFixVector tvec;

		tvec = gameData.objData.pConsole->info.position.vPos - gameData.endLevelData.exit.vSideExit;
		if (CFixVector::Dot (tvec, gameData.endLevelData.exit.mOrient.m.dir.f) > 0) {
			CObject *pObj;
			gameStates.render.bOutsideMine = 1;
			pObj = CreateExplosion (gameData.endLevelData.exit.nSegNum, gameData.endLevelData.exit.vSideExit, I2X (50), ANIM_BIG_PLAYER_EXPLOSION);
			if (pObj) {
				externalExplosion = *pObj;
				pObj->Die ();
				gameStates.render.nFlashScale = I2X (1);	//kill lights in mine
				ext_expl_halflife = pObj->info.xLifeLeft;
				gameStates.render.bExtExplPlaying = 1;
				}
			audio.CreateSegmentSound (SOUND_BIG_ENDLEVEL_EXPLOSION, gameData.endLevelData.exit.nSegNum, 0, gameData.endLevelData.exit.vSideExit, 0, I2X (3)/4);
			}
		}

	//do explosions chasing player
	if ((explosion_wait1 -= gameData.timeData.xFrame) < 0) {
		CFixVector	vPos;
		int16_t			nSegment;
		static int32_t	nSounds = 0;

		vPos = gameData.objData.pConsole->info.position.vPos + gameData.objData.pConsole->info.position.mOrient.m.dir.f * (-gameData.objData.pConsole->info.xSize * 5);
		vPos += gameData.objData.pConsole->info.position.mOrient.m.dir.r * (SRandShort () * 15);
		vPos += gameData.objData.pConsole->info.position.mOrient.m.dir.u * (SRandShort () * 15);
		nSegment = FindSegByPos (vPos, gameData.objData.pConsole->info.nSegment, 1, 0);
		if (nSegment != -1) {
			CreateExplosion (nSegment, vPos, I2X (20), ANIM_BIG_PLAYER_EXPLOSION);
			if ((RandShort () < 10000) || (++nSounds == 7)) {		//pseudo-random
				audio.CreateSegmentSound (SOUND_TUNNEL_EXPLOSION, nSegment, 0, vPos, 0, I2X (1));
				nSounds = 0;
				}
			}
		explosion_wait1 = 0x2000 + RandShort () / 4;
		}
	}

//do little explosions on walls
if ((gameStates.app.bEndLevelSequence >= EL_FLYTHROUGH) && (gameStates.app.bEndLevelSequence < EL_OUTSIDE))
	if ((explosion_wait2 -= gameData.timeData.xFrame) < 0) {
		CFixVector	vPos;
		//create little explosion on CWall
		vPos = gameData.objData.pConsole->info.position.mOrient.m.dir.r * (SRandShort () * 100);
		vPos += gameData.objData.pConsole->info.position.mOrient.m.dir.u * (SRandShort () * 100);
		vPos += gameData.objData.pConsole->info.position.vPos;
		if (gameStates.app.bEndLevelSequence == EL_FLYTHROUGH)
			vPos += gameData.objData.pConsole->info.position.mOrient.m.dir.f * (RandShort () * 200);
		else
			vPos += gameData.objData.pConsole->info.position.mOrient.m.dir.f * (RandShort () * 60);

		//find hit point on CWall
		CHitQuery	hitQuery (0, &gameData.objData.pConsole->info.position.vPos, &vPos, gameData.objData.pConsole->info.nSegment);
		CHitResult	hitResult;

		FindHitpoint (hitQuery, hitResult);
		if ((hitResult.nType == HIT_WALL) && (hitResult.nSegment != -1))
			CreateExplosion ((int16_t) hitResult.nSegment, hitResult.vPoint, I2X (3) + RandShort () * 6, ANIM_SMALL_EXPLOSION);
		explosion_wait2 = (0xa00 + RandShort () / 8) / 2;
		}

switch (gameStates.app.bEndLevelSequence) {
	case EL_OFF:
		return;

	case EL_FLYTHROUGH: {
		DoEndLevelFlyThrough (0);
		if (gameData.objData.pConsole->info.nSegment == gameData.endLevelData.exit.nTransitSegNum) {
			if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) &&
					(StartEndLevelMovie () != MOVIE_NOT_PLAYED))
				StopEndLevelSequence ();
			else {
				gameStates.app.bEndLevelSequence = EL_LOOKBACK;
				int32_t nObject = CreateCamera (gameData.objData.pConsole);
				if (nObject == -1) { //can't get CObject, so abort
#if TRACE
					console.printf (1, "Can't get CObject for endlevel sequence.  Aborting endlevel sequence.\n");
#endif
					StopEndLevelSequence ();
					return;
					}
				gameData.SetViewer (gameData.objData.endLevelCamera = OBJECT (nObject));
				cockpit->Activate (CM_LETTERBOX);
				exitFlightObjects [1] = exitFlightObjects [0];
				exitFlightObjects [1].pObj = gameData.objData.endLevelCamera;
				exitFlightObjects [1].speed = (5 * gameData.endLevelData.xCurFlightSpeed) / 4;
				exitFlightObjects [1].lateralOffset = 0x4000;
				gameData.objData.endLevelCamera->info.position.vPos += gameData.objData.endLevelCamera->info.position.mOrient.m.dir.f * (I2X (7));
				timer=0x20000;
				}
			}
		break;
		}

	case EL_LOOKBACK: {
		DoEndLevelFlyThrough (0);
		DoEndLevelFlyThrough (1);
		if (timer > 0) {
			timer -= gameData.timeData.xFrame;
			if (timer < 0)		//reduce speed
				exitFlightObjects [1].speed = exitFlightObjects [0].speed;
			}
		if (gameData.objData.endLevelCamera->info.nSegment == gameData.endLevelData.exit.nSegNum) {
			CAngleVector cam_angles, exit_seg_angles;
			gameStates.app.bEndLevelSequence = EL_OUTSIDE;
			timer = I2X (2);
			gameData.objData.endLevelCamera->info.position.mOrient.m.dir.f = -gameData.objData.endLevelCamera->info.position.mOrient.m.dir.f;
			gameData.objData.endLevelCamera->info.position.mOrient.m.dir.r = -gameData.objData.endLevelCamera->info.position.mOrient.m.dir.r;
			cam_angles = gameData.objData.endLevelCamera->info.position.mOrient.ComputeAngles ();
			exit_seg_angles = gameData.endLevelData.exit.mOrient.ComputeAngles ();
			bank_rate = (-exit_seg_angles.v.coord.b - cam_angles.v.coord.b)/2;
			gameData.objData.pConsole->info.controlType = gameData.objData.endLevelCamera->info.controlType = CT_NONE;
#ifdef SLEW_ON
			pSlewObj = gameData.objData.endLevelCamera;
#endif
			}
		break;
		}

	case EL_OUTSIDE: {
#ifndef SLEW_ON
		CAngleVector cam_angles;
#endif
		gameData.objData.pConsole->info.position.vPos += gameData.objData.pConsole->info.position.mOrient.m.dir.f * (FixMul (gameData.timeData.xFrame, gameData.endLevelData.xCurFlightSpeed));
#ifndef SLEW_ON
		gameData.objData.endLevelCamera->info.position.vPos +=
							gameData.objData.endLevelCamera->info.position.mOrient.m.dir.f *
							(FixMul (gameData.timeData.xFrame, -2*gameData.endLevelData.xCurFlightSpeed));
		gameData.objData.endLevelCamera->info.position.vPos +=
							gameData.objData.endLevelCamera->info.position.mOrient.m.dir.u *
							(FixMul (gameData.timeData.xFrame, -gameData.endLevelData.xCurFlightSpeed/10));
		cam_angles = gameData.objData.endLevelCamera->info.position.mOrient.ComputeAngles ();
		cam_angles.v.coord.b += (fixang) FixMul (bank_rate, gameData.timeData.xFrame);
		gameData.objData.endLevelCamera->info.position.mOrient = CFixMatrix::Create (cam_angles);
#endif
		timer -= gameData.timeData.xFrame;
		if (timer < 0) {
			gameStates.app.bEndLevelSequence = EL_STOPPED;
			vPlayerAngles = gameData.objData.pConsole->info.position.mOrient.ComputeAngles ();
			timer = I2X (3);
			}
		break;
		}

	case EL_STOPPED: {
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevelData.station.vPos, &gameData.objData.pConsole->info.position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		gameData.objData.pConsole->info.position.mOrient = CFixMatrix::Create (vPlayerAngles);
		gameData.objData.pConsole->info.position.vPos += gameData.objData.pConsole->info.position.mOrient.m.dir.f * (FixMul (gameData.timeData.xFrame, gameData.endLevelData.xCurFlightSpeed));
		timer -= gameData.timeData.xFrame;
		if (timer < 0) {
#ifdef SLEW_ON
			pSlewObj = gameData.objData.endLevelCamera;
			DoSlewMovement (gameData.objData.endLevelCamera, 1, 1);
			timer += gameData.timeData.xFrame;		//make time stop
			break;
#else
#	ifdef SHORT_SEQUENCE
			StopEndLevelSequence ();
#	else
			gameStates.app.bEndLevelSequence = EL_PANNING;
			VmExtractAnglesMatrix (&vCurrentCameraAngles, &gameData.objData.endLevelCamera->info.position.mOrient);
			timer = I2X (3);
			if (IsMultiGame) { // try to skip part of the playerSyncData if multiplayer
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
		int32_t mask;
#endif
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevelData.station.vPos, &gameData.objData.pConsole->info.position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		VmAngles2Matrix (&gameData.objData.pConsole->info.position.mOrient, &vPlayerAngles);
		VmVecScaleInc (&gameData.objData.pConsole->info.position.vPos, &gameData.objData.pConsole->info.position.mOrient.mat.dir.f, FixMul (gameData.timeData.xFrame, gameData.endLevelData.xCurFlightSpeed);

#ifdef SLEW_ON
		DoSlewMovement (gameData.objData.endLevelCamera, 1, 1);
#else
		GetAnglesToObject (&vDesiredCameraAngles, &gameData.objData.pConsole->info.position.vPos, &gameData.objData.endLevelCamera->info.position.vPos);
		mask = ChaseAngles (&vCurrentCameraAngles, &vDesiredCameraAngles);
		VmAngles2Matrix (&gameData.objData.endLevelCamera->info.position.mOrient, &vCurrentCameraAngles);
		if ((mask & 5) == 5) {
			CFixVector tvec;
			gameStates.app.bEndLevelSequence = EL_CHASING;
			VmVecNormalizedDir (&tvec, &gameData.endLevelData.station.vPos, &gameData.objData.pConsole->info.position.vPos);
			VmVector2Matrix (&gameData.objData.pConsole->info.position.mOrient, &tvec, &mSurfaceOrient.mat.dir.u, NULL);
			gameData.endLevelData.xDesiredFlightSpeed *= 2;
			}
#endif
		break;
	}

	case EL_CHASING:
	 {
		fix d, speed_scale;

#ifdef SLEW_ON
		DoSlewMovement (gameData.objData.endLevelCamera, 1, 1);
#endif
		GetAnglesToObject (&vDesiredCameraAngles, &gameData.objData.pConsole->info.position.vPos, &gameData.objData.endLevelCamera->info.position.vPos);
		ChaseAngles (&vCurrentCameraAngles, &vDesiredCameraAngles);
#ifndef SLEW_ON
		VmAngles2Matrix (&gameData.objData.endLevelCamera->info.position.mOrient, &vCurrentCameraAngles);
#endif
		d = VmVecDistQuick (&gameData.objData.pConsole->info.position.vPos, &gameData.objData.endLevelCamera->info.position.vPos);
		speed_scale = FixDiv (d, I2X (0x20);
		if (d < I2X (1))
			d = I2X (1);
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevelData.station.vPos, &gameData.objData.pConsole->info.position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		VmAngles2Matrix (&gameData.objData.pConsole->info.position.mOrient, &vPlayerAngles);
		VmVecScaleInc (&gameData.objData.pConsole->info.position.vPos, &gameData.objData.pConsole->info.position.mOrient.mat.dir.f, FixMul (gameData.timeData.xFrame, gameData.endLevelData.xCurFlightSpeed);
#ifndef SLEW_ON
		VmVecScaleInc (&gameData.objData.endLevelCamera->info.position.vPos, &gameData.objData.endLevelCamera->info.position.mOrient.mat.dir.f, FixMul (gameData.timeData.xFrame, FixMul (speed_scale, gameData.endLevelData.xCurFlightSpeed));
		if (VmVecDist (&gameData.objData.pConsole->info.position.vPos, &gameData.endLevelData.station.vPos) < I2X (10))
			StopEndLevelSequence ();
#endif
		break;
		}
#endif		//ifdef SHORT_SEQUENCE
	}
}

//------------------------------------------------------------------------------

#define MIN_D 0x100

//find which CSide to fly out of
int32_t FindExitSide (CObject *pObj)
{
ENTER (0);
	CFixVector	vPreferred, vSegCenter, vSide;
	fix			d, xBestVal = -I2X (2);
	int32_t		nBestSide, i;
	CSegment		*pSeg = SEGMENT (pObj->info.nSegment);

//find exit CSide
CFixVector::NormalizedDir (vPreferred, pObj->info.position.vPos, pObj->info.vLastPos);
vSegCenter = pSeg->Center ();
nBestSide = -1;
for (i = SEGMENT_SIDE_COUNT; --i >= 0;) {
	if (pSeg->m_children [i] != -1) {
		vSide = pSeg->SideCenter (i);
		CFixVector::NormalizedDir (vSide, vSide, vSegCenter);
		d = CFixVector::Dot (vSide, vPreferred);
		if (labs (d) < MIN_D)
			d = 0;
		if (d > xBestVal) {
			xBestVal = d;
			nBestSide = i;
			}
		}
	}
Assert (nBestSide!=-1);
RETURN (nBestSide)
}

//------------------------------------------------------------------------------

void DrawExitModel (void)
{
ENTER (0);
	CFixVector	vModelPos;
	int32_t		f = 15, u = 0;	//21;

vModelPos = gameData.endLevelData.exit.vMineExit + gameData.endLevelData.exit.mOrient.m.dir.f * (I2X (f));
vModelPos += gameData.endLevelData.exit.mOrient.m.dir.u * (I2X (u));
gameStates.app.bD1Model = gameStates.app.bD1Mission && gameStates.app.bD1Data;
DrawPolyModel (NULL, &vModelPos, &gameData.endLevelData.exit.mOrient, NULL,
					gameStates.gameplay.bMineDestroyed ? gameData.endLevelData.exit.nDestroyedModel : gameData.endLevelData.exit.nModel,
					0, I2X (1), NULL, NULL, NULL);
gameStates.app.bD1Model = 0;
LEAVE
}

//------------------------------------------------------------------------------

int32_t nExitPointBmX, nExitPointBmY;

fix xSatelliteSize = I2X (400);

#define SATELLITE_DIST		I2X (1024)
#define SATELLITE_WIDTH		xSatelliteSize
#define SATELLITE_HEIGHT	 ((xSatelliteSize*9)/4)		// ((xSatelliteSize*5)/2)

void RenderExternalScene (fix xEyeOffset)
{
ENTER (0);
	CFixVector		vDelta;
	CRenderPoint	p, pTop;

gameData.renderData.mine.viewer.vPos = gameData.objData.pViewer->info.position.vPos;
if (xEyeOffset)
	gameData.renderData.mine.viewer.vPos += gameData.objData.pViewer->info.position.mOrient.m.dir.r * (xEyeOffset);
SetupTransformation (transformation, gameData.objData.pViewer->info.position.vPos, gameData.objData.pViewer->info.position.mOrient, gameStates.render.xZoom, 1);
CCanvas::Current ()->Clear (BLACK_RGBA);
transformation.Begin (CFixVector::ZERO, mSurfaceOrient, __FILE__, __LINE__);
DrawStars ();
transformation.End (__FILE__, __LINE__);
//draw satellite
p.TransformAndEncode (gameData.endLevelData.satellite.vPos);
transformation.RotateScaled (vDelta, gameData.endLevelData.satellite.vUp);
pTop.Add (p, vDelta);
if (!(p.Codes () & (CC_BEHIND | PF_OVERFLOW))) {
	int32_t imSave = gameStates.render.nInterpolationMethod;
	gameStates.render.nInterpolationMethod = 0;
	//gameData.endLevelData.satellite.pBm->SetTranspType (0);
	if (!gameData.endLevelData.satellite.pBm->Bind (1))
		G3DrawRodTexPoly (gameData.endLevelData.satellite.pBm, &p, SATELLITE_WIDTH, &pTop, SATELLITE_WIDTH, I2X (1), satUVL);
	gameStates.render.nInterpolationMethod = imSave;
	}
#ifdef STATION_ENABLED
DrawPolyModel (NULL, &gameData.endLevelData.station.vPos, &vmdIdentityMatrix, NULL,
						gameData.endLevelData.station.nModel, 0, I2X (1), NULL, NULL);
#endif
RenderTerrain (&gameData.endLevelData.exit.vGroundExit, nExitPointBmX, nExitPointBmY);
DrawExitModel ();
if (gameStates.render.bExtExplPlaying)
	DrawFireball (&externalExplosion);
int32_t nLighting = gameStates.render.nLighting;
gameStates.render.nLighting = 0;
RenderObject (gameData.objData.pConsole, 0, 0);
gameStates.render.nState = 0;
transparencyRenderer.Render (0);
gameStates.render.nLighting = nLighting;
LEAVE
}

//------------------------------------------------------------------------------

#define MAX_STARS 500

CFixVector stars [MAX_STARS];

void GenerateStarfield (void)
{
	int32_t i;

for (i = 0; i < MAX_STARS; i++) {
	stars [i].v.coord.x = SRandShort () << 14;
	stars [i].v.coord.z = SRandShort () << 14;
	stars [i].v.coord.y = (RandShort () / 2) << 14;
	}
}

//------------------------------------------------------------------------------

void DrawStars (void)
{
	int32_t i;
	int32_t intensity = 31;
	CRenderPoint p;

for (i = 0; i < MAX_STARS; i++) {
	if ((i&63) == 0) {
		CCanvas::Current ()->SetColorRGBi (RGB_PAL (intensity, intensity, intensity));
		intensity-=3;
		}
	transformation.RotateScaled (p.ViewPos (), stars [i]);
	p.Encode ();
	if (p.Visible ()) {
		p.Project ();
		DrawPixelClipped (p.X (), p.Y ());
		}
	}
}

//------------------------------------------------------------------------------

void RenderEndLevelMine (fix xEyeOffset, int32_t nWindowNum)
{
ENTER (0);
	int16_t nStartSeg;

gameData.renderData.mine.viewer.vPos = gameData.objData.pViewer->info.position.vPos;
if (gameData.objData.pViewer->info.nType == OBJ_PLAYER)
	gameData.renderData.mine.viewer.vPos += gameData.objData.pViewer->info.position.mOrient.m.dir.f * ((gameData.objData.pViewer->info.xSize * 3) / 4);
if (xEyeOffset)
	gameData.renderData.mine.viewer.vPos += gameData.objData.pViewer->info.position.mOrient.m.dir.r * (xEyeOffset);
if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE)
	nStartSeg = gameData.endLevelData.exit.nSegNum;
else {
	nStartSeg = FindSegByPos (gameData.renderData.mine.viewer.vPos, gameData.objData.pViewer->info.nSegment, 1, 0);
	if (nStartSeg == -1)
		nStartSeg = gameData.objData.pViewer->info.nSegment;
	}
if (gameStates.app.bEndLevelSequence == EL_LOOKBACK) {
	CFixMatrix headm, viewm;
	CAngleVector angles = CAngleVector::Create(0, 0, 0x7fff);
	headm = CFixMatrix::Create (angles);
	viewm = gameData.objData.pViewer->info.position.mOrient * headm;
	SetupTransformation (transformation, gameData.renderData.mine.viewer.vPos, viewm, gameStates.render.xZoom, 1);
	}
else
	SetupTransformation (transformation, gameData.renderData.mine.viewer.vPos, gameData.objData.pViewer->info.position.mOrient, gameStates.render.xZoom, 1);
RenderMine (nStartSeg, xEyeOffset, nWindowNum);
transparencyRenderer.Render (0);
LEAVE
}

//-----------------------------------------------------------------------------

void RenderEndLevelFrame (fix xStereoSeparation, int32_t nWindow)
{
G3StartFrame (transformation, 0, !nWindow, xStereoSeparation);
//gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
if (gameStates.app.bEndLevelSequence < EL_OUTSIDE)
	RenderEndLevelMine (xStereoSeparation, nWindow);
else if (!nWindow)
	RenderExternalScene (xStereoSeparation);
G3EndFrame (transformation, nWindow);
}

//------------------------------------------------------------------------------
///////////////////////// copy of flythrough code for endlevel

int32_t ELFindConnectedSide (int32_t seg0, int32_t seg1);

fixang DeltaAng (fixang a, fixang b);

#define DEFAULT_SPEED I2X (16)

#define MIN_D 0x100

//if speed is zero, use default speed
void StartEndLevelFlyThrough (int32_t n, CObject *pObj, fix speed)
{
pExitFlightData = exitFlightObjects + n;
pExitFlightData->pObj = pObj;
pExitFlightData->bStart = 1;
pExitFlightData->speed = speed ? speed : DEFAULT_SPEED;
pExitFlightData->lateralOffset = 0;
songManager.Play (SONG_INTER, 0);
}

//------------------------------------------------------------------------------

static CAngleVector *angvec_add2_scale (CAngleVector *dest, CFixVector *src, fix s)
{
(*dest).v.coord.p += (fixang) FixMul ((*src).v.coord.x, s);
(*dest).v.coord.b += (fixang) FixMul ((*src).v.coord.z, s);
(*dest).v.coord.h += (fixang) FixMul ((*src).v.coord.y, s);
return dest;
}

//-----------------------------------------------------------------------------

#define MAX_ANGSTEP	0x4000		//max turn per second

#define MAX_SLIDE_PER_SEGMENT 0x10000

void DoEndLevelFlyThrough (int32_t n)
{
ENTER (0);

	CObject	*pObj;
	CSegment *pSeg;
	int32_t	nOldPlayerSeg;

pExitFlightData = exitFlightObjects + n;
pObj = pExitFlightData->pObj;
nOldPlayerSeg = pObj->info.nSegment;

//move the player for this frame

if (!pExitFlightData->bStart) {
	pObj->info.position.vPos += pExitFlightData->vStep * gameData.timeData.xFrame;
	angvec_add2_scale (&pExitFlightData->angles, &pExitFlightData->aStep, gameData.timeData.xFrame);
	pObj->info.position.mOrient = CFixMatrix::Create (pExitFlightData->angles);
	}
//check new player seg
if (pExitFlightData->bStart || (UpdateObjectSeg (pObj, false) > 0)) {
	pSeg = SEGMENT (pObj->info.nSegment);
	if (pExitFlightData->bStart || (pObj->info.nSegment != nOldPlayerSeg)) {		//moved into new seg
		CFixVector curcenter, nextcenter;
		fix xStepSize, xSegTime;
		int16_t nEntrySide, nExitSide = -1;//what sides we entry and leave through
		CFixVector vDest;		//where we are heading (center of nExitSide)
		CAngleVector aDest;		//where we want to be pointing
		CFixMatrix mDest;
		int32_t nUpSide = 0;

	#if DBG
		if (n && pObj->info.nSegment == nDbgSeg)
			BRP;
	#endif
		nEntrySide = 0;
		//find new exit CSide
		if (!pExitFlightData->bStart) {
			nEntrySide = ELFindConnectedSide (pObj->info.nSegment, nOldPlayerSeg);
			nExitSide = oppSideTable [nEntrySide];
			}
		if (pExitFlightData->bStart || (nEntrySide == -1) || (pSeg->m_children [nExitSide] == -1))
			nExitSide = FindExitSide (pObj);
		if ((nExitSide >= 0) && (pSeg->m_children [nExitSide] >= 0)) {
			fix d, dLargest = -I2X (1);
			CSide* pSide = pSeg->m_sides;
			for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
				if (pSide->FaceCount ()) {
					d = CFixVector::Dot (pSide->m_normals [0], pExitFlightData->pObj->info.position.mOrient.m.dir.u);
					if (d > dLargest) {
						dLargest = d; 
						nUpSide = i;
						}
					}
				}
			//update target point & angles
			vDest = pSeg->SideCenter (nExitSide);
			//update target point and movement points
			//offset CObject sideways
			if (pExitFlightData->lateralOffset) {
				int32_t s0 = -1, s1 = 0, i;
				CFixVector s0p, s1p;
				fix dist;

				for (i = 0; i < SEGMENT_SIDE_COUNT; i++)
					if ((i != nEntrySide) && (i != nExitSide) && (i != nUpSide) && (i != oppSideTable [nUpSide])) {
						if (s0 == -1)
							s0 = i;
						else
							s1 = i;
						}
				s0p = pSeg->SideCenter (s0);
				s1p = pSeg->SideCenter (s1);
				dist = FixMul (CFixVector::Dist (s0p, s1p), pExitFlightData->lateralOffset);
				if (dist-pExitFlightData->offsetDist > MAX_SLIDE_PER_SEGMENT)
					dist = pExitFlightData->offsetDist + MAX_SLIDE_PER_SEGMENT;
				pExitFlightData->offsetDist = dist;
				vDest += pObj->info.position.mOrient.m.dir.r * dist;
				}
			pExitFlightData->vStep = vDest - pObj->info.position.vPos;
			xStepSize = CFixVector::Normalize (pExitFlightData->vStep);
			pExitFlightData->vStep *= pExitFlightData->speed;
			curcenter = pSeg->Center ();
			nextcenter = SEGMENT (pSeg->m_children [nExitSide])->Center ();
			pExitFlightData->vHeading = nextcenter - curcenter;
			mDest = CFixMatrix::CreateFU (pExitFlightData->vHeading, pSeg->m_sides [nUpSide].m_normals [0]);
			aDest = mDest.ComputeAngles ();
			if (pExitFlightData->bStart)
				pExitFlightData->angles = pObj->info.position.mOrient.ComputeAngles ();
			xSegTime = FixDiv (xStepSize, pExitFlightData->speed);	//how long through seg
			if (xSegTime) {
				pExitFlightData->aStep.v.coord.x = Max (-MAX_ANGSTEP, Min (MAX_ANGSTEP, FixDiv (DeltaAng (pExitFlightData->angles.v.coord.p, aDest.v.coord.p), xSegTime)));
				pExitFlightData->aStep.v.coord.z = Max (-MAX_ANGSTEP, Min (MAX_ANGSTEP, FixDiv (DeltaAng (pExitFlightData->angles.v.coord.b, aDest.v.coord.b), xSegTime)));
				pExitFlightData->aStep.v.coord.y = Max (-MAX_ANGSTEP, Min (MAX_ANGSTEP, FixDiv (DeltaAng (pExitFlightData->angles.v.coord.h, aDest.v.coord.h), xSegTime)));
				}
			else {
				pExitFlightData->angles = aDest;
				pExitFlightData->aStep.SetZero ();
				}
			}
		}
	}
pExitFlightData->bStart = 0;
LEAVE
}

//------------------------------------------------------------------------------

#ifdef SLEW_ON		//this is a special routine for slewing around external scene
int32_t DoSlewMovement (CObject *pObj, int32_t check_keys, int32_t check_joy)
{
	int32_t moved = 0;
	CFixVector svel, movement;				//scaled velocity (per this frame)
	CFixMatrix rotmat, new_pm;
	int32_t joy_x, joy_y, btns;
	int32_t joyx_moved, joyy_moved;
	CAngleVector rotang;

if (gameStates.input.keys.dir.coord.pressed [KEY_PAD5])
	VmVecZero (&pObj->physInfo.velocity);

if (check_keys) {
	pObj->physInfo.velocity.x += VEL_SPEED * (KeyDownTime (KEY_PAD9) - KeyDownTime (KEY_PAD7);
	pObj->physInfo.velocity.y += VEL_SPEED * (KeyDownTime (KEY_PADMINUS) - KeyDownTime (KEY_PADPLUS);
	pObj->physInfo.velocity.z += VEL_SPEED * (KeyDownTime (KEY_PAD8) - KeyDownTime (KEY_PAD2);

	rotang.dir.coord.pitch = (KeyDownTime (KEY_LBRACKET) - KeyDownTime (KEY_RBRACKET))/ROT_SPEED;
	rotang.bank  = (KeyDownTime (KEY_PAD1) - KeyDownTime (KEY_PAD3))/ROT_SPEED;
	rotang.head  = (KeyDownTime (KEY_PAD6) - KeyDownTime (KEY_PAD4))/ROT_SPEED;
	}
	else
		rotang.dir.coord.pitch = rotang.bank  = rotang.head  = 0;
//check for joystick movement
if (check_joy && bJoyPresent) {
	JoyGetpos (&joy_x, &joy_y);
	btns=JoyGetBtns ();
	joyx_moved = (abs (joy_x - old_joy_x)>JOY_NULL);
	joyy_moved = (abs (joy_y - old_joy_y)>JOY_NULL);
	if (abs (joy_x) < JOY_NULL) joy_x = 0;
	if (abs (joy_y) < JOY_NULL) joy_y = 0;
	if (btns)
		if (!rotang.dir.coord.pitch) rotang.dir.coord.pitch = FixMul (-joy_y * 512, gameData.timeData.xFrame); else;
	else
		if (joyy_moved) pObj->physInfo.velocity.z = -joy_y * 8192;
	if (!rotang.head) rotang.head = FixMul (joy_x * 512, gameData.timeData.xFrame);
	if (joyx_moved)
		old_joy_x = joy_x;
	if (joyy_moved)
		old_joy_y = joy_y;
	}
moved = rotang.dir.coord.pitch | rotang.bank | rotang.head;
VmAngles2Matrix (&rotmat, &rotang);
VmMatMul (&new_pm, &pObj->info.position.mOrient, &rotmat);
pObj->info.position.mOrient = new_pm;
VmTransposeMatrix (&new_pm);		//make those columns rows
moved |= pObj->physInfo.velocity.x | pObj->physInfo.velocity.y | pObj->physInfo.velocity.z;
svel = pObj->physInfo.velocity;
VmVecScale (&svel, gameData.timeData.xFrame);		//movement in this frame
VmVecRotate (&movement, &svel, &new_pm);
VmVecInc (&pObj->info.position.vPos, &movement);
moved |= (movement.x || movement.y || movement.z);
return moved;
}
#endif

//-----------------------------------------------------------------------------

#define LINE_LEN	80
#define NUM_VARS	8

#define STATION_DIST	I2X (1024)

int32_t ConvertExt (char *dest, const char *ext)
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
void LoadEndLevelData (int32_t nLevel)
{
ENTER (0);

	char			filename [13];
	char			line [LINE_LEN], *p = NULL;
	CFile			cf;
	int32_t		var, nSegment, nSide;
	int32_t		nExitSide = 0, i;
	int32_t		bHaveBinary = 0;

gameStates.app.bEndLevelDataLoaded = 0;		//not loaded yet
if (!gameOpts->movies.nLevel)
	LEAVE

try_again:

if (gameStates.app.bAutoRunMission)
	strcpy (filename, szAutoMission);
else if (nLevel < 0)		//secret level
	strcpy (filename, missionManager.szSecretLevelNames [-nLevel-1]);
else					//Normal level
	strcpy (filename, missionManager.szLevelNames [nLevel-1]);
if (!ConvertExt (filename, "end"))
	Error ("Error converting filename\n'<%s>'\nfor endlevel data\n", filename);
if (!cf.Open (filename, gameFolders.game.szData [0], "rb", gameStates.app.bD1Mission)) {
	ConvertExt (filename, "txb");
	if (!cf.Open (filename, gameFolders.game.szData [0], "rb", gameStates.app.bD1Mission)) {
		if (nLevel == 1) {
#if TRACE
			console.printf (CON_DBG, "Cannot load file text\nof binary version of\n'<%s>'\n", filename);
#endif
			gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
			LEAVE
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
PrintLog (1, "parsing endlevel description\n");
while (cf.GetS (line, LINE_LEN)) {
	if (bHaveBinary) {
		int32_t l = (int32_t) strlen (line);
		for (i = 0; i < l; i++)
			line [i] = EncodeRotateLeft ((char) (EncodeRotateLeft (line [i]) ^ BITMAP_TBL_XOR));
		p = line;
		}
	if ((p = strchr (line, ';')))
		*p = 0;		//cut off comment
	for (p = line + strlen (line) - 1; (p > line) && ::isspace ((uint8_t) *p); *p-- = 0)
		;
	for (p = line; ::isspace ((uint8_t) *p); p++)
		;
	if (!*p)		//empty line
		continue;
	switch (var) {
		case 0: {						//ground terrain
			CIFF iff;
			int32_t iffError;

			PrintLog (1, "loading terrain bitmap\n");
			if (gameData.endLevelData.terrain.bmInstance.Buffer ()) {
				gameData.endLevelData.terrain.bmInstance.ReleaseTexture ();
				gameData.endLevelData.terrain.bmInstance.DestroyBuffer ();
				}
			Assert (gameData.endLevelData.terrain.bmInstance.Buffer () == NULL);
			iffError = iff.ReadBitmap (p, &gameData.endLevelData.terrain.bmInstance, BM_LINEAR);
			if (iffError != IFF_NO_ERROR) {
#if DBG
				PrintLog (0, TXT_EXIT_TERRAIN, p, iff.ErrorMsg (iffError));
				PrintLog (0, "\n");
#endif
				gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
				PrintLog (-1);
				LEAVE
				}
			gameData.endLevelData.terrain.pBm = &gameData.endLevelData.terrain.bmInstance;
			//GrRemapBitmapGood (gameData.endLevelData.terrain.pBm, NULL, iff_transparent_color, -1);
			break;
			}

		case 1:							//height map
			PrintLog (1, "loading endlevel terrain\n");
			LoadTerrain (p);
			break;

		case 2:
			PrintLog (1, "loading exit point\n");
			sscanf (p, "%d, %d", &nExitPointBmX, &nExitPointBmY);
			break;

		case 3:							//exit heading
			PrintLog (1, "loading exit angle\n");
			vExitAngles.v.coord.h = I2X (atoi (p))/360;
			break;

		case 4: {						//planet bitmap
			CIFF iff;
			int32_t iffError;

			PrintLog (1, "loading satellite bitmap\n");
			if (gameData.endLevelData.satellite.bmInstance.Buffer ()) {
				gameData.endLevelData.satellite.bmInstance.ReleaseTexture ();
				gameData.endLevelData.satellite.bmInstance.DestroyBuffer ();
				}
			iffError = iff.ReadBitmap (p, &gameData.endLevelData.satellite.bmInstance, BM_LINEAR);
			if (iffError != IFF_NO_ERROR) {
				Warning (TXT_SATELLITE, p, iff.ErrorMsg (iffError));
				gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
				PrintLog (-1);
				LEAVE
				}
			gameData.endLevelData.satellite.pBm = &gameData.endLevelData.satellite.bmInstance;
			//GrRemapBitmapGood (gameData.endLevelData.satellite.pBm, NULL, iff_transparent_color, -1);
			break;
			}

		case 5:							//earth pos
		case 7: {						//station pos
			CFixMatrix tm;
			CAngleVector ta;
			int32_t pitch, head;

			PrintLog (1, "loading satellite and station position\n");
			sscanf (p, "%d, %d", &head, &pitch);
			ta.v.coord.h = I2X (head)/360;
			ta.v.coord.p = -I2X (pitch)/360;
			ta.v.coord.b = 0;
			tm = CFixMatrix::Create(ta);
			if (var == 5)
				gameData.endLevelData.satellite.vPos = tm.m.dir.f;
				//VmVecCopyScale (&gameData.endLevelData.satellite.vPos, &tm.m.v.f, SATELLITE_DIST);
			else
				gameData.endLevelData.station.vPos = tm.m.dir.f;
			break;
			}

		case 6:						//planet size
			PrintLog (1, "loading satellite size\n");
			xSatelliteSize = I2X (atoi (p));
			break;
		}
	var++;
	PrintLog (-1);
}

Assert (var == NUM_VARS);
// OK, now the data is loaded.  Initialize everything
//find the exit sequence by searching all segments for a CSide with
//children == -2
CSegment* pSeg = SEGMENTS.Buffer ();
for (nSegment = 0, gameData.endLevelData.exit.nSegNum = -1;
	  (gameData.endLevelData.exit.nSegNum == -1) && (nSegment <= gameData.segData.nLastSegment);
	  nSegment++, pSeg++)
	for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++)
		if (pSeg->m_children [nSide] == -2) {
			gameData.endLevelData.exit.nSegNum = nSegment;
			nExitSide = nSide;
			break;
			}

if (gameData.endLevelData.exit.nSegNum == -1) {
#if DBG
	Warning (TXT_EXIT_TERRAIN, p ? p : "", cf.Name ());
#endif
	gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
	LEAVE
	}
PrintLog (1, "computing endlevel element orientation\n");
gameData.endLevelData.exit.vMineExit = SEGMENT (gameData.endLevelData.exit.nSegNum)->Center ();
ExtractOrientFromSegment (&gameData.endLevelData.exit.mOrient, SEGMENT (gameData.endLevelData.exit.nSegNum));
gameData.endLevelData.exit.vSideExit = SEGMENT (gameData.endLevelData.exit.nSegNum)->SideCenter (nExitSide);
gameData.endLevelData.exit.vGroundExit = gameData.endLevelData.exit.vMineExit + gameData.endLevelData.exit.mOrient.m.dir.u * (-I2X (20));
//compute orientation of surface
{
	CFixVector tv;
	CFixMatrix exit_orient, tm;

	exit_orient = CFixMatrix::Create (vExitAngles);
	CFixMatrix::Transpose (exit_orient);
	mSurfaceOrient = gameData.endLevelData.exit.mOrient * exit_orient;
	tm = mSurfaceOrient.Transpose();
	tv = tm * gameData.endLevelData.station.vPos;
	gameData.endLevelData.station.vPos = gameData.endLevelData.exit.vMineExit + tv * STATION_DIST;
	tv = tm * gameData.endLevelData.satellite.vPos;
	gameData.endLevelData.satellite.vPos = gameData.endLevelData.exit.vMineExit + tv * SATELLITE_DIST;
	tm = CFixMatrix::CreateFU (tv, mSurfaceOrient.m.dir.u);
	gameData.endLevelData.satellite.vUp = tm.m.dir.u * SATELLITE_HEIGHT;
	}
PrintLog (-1);
cf.Close ();
gameStates.app.bEndLevelDataLoaded = 1;
LEAVE
}

//------------------------------------------------------------------------------
//eof
