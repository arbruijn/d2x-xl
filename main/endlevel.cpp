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
	CObject*			objP;
	CAngleVector	angles;			//orientation in angles
	CFixVector		vDest;
	CFixVector		vStep;			//how far in a second
	CFixVector		aStep;			//rotation per second
	fix				speed;			//how fast CObject is moving
	CFixVector 		vHeading;		//where we want to be pointing
	short				entrySide;		//side at which the current exit segment was entered
	short				exitSide;		//side at which to leave the current exit segment
	fix				lateralOffset;	//how far off-center as portion of way
	fix				offsetDist;		//how far currently off-center
	fix				destDist;
	fix				pathDot;
} tExitFlightData;

//endlevel sequence states

#define SHORT_SEQUENCE	1		//if defined, end sequnce when panning starts
//#define STATION_ENABLED	1		//if defined, load & use space station model

#define FLY_SPEED I2X (50)

int DoEndLevelFlyThrough (int nObject);
void DrawStars ();
int FindExitSide (CObject *objP);
void GenerateStarfield ();
void StartEndLevelFlyThrough (int nObject, CObject *objP, fix speed);
void StartRenderedEndLevelSequence (void);

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

int ELFindConnectedSide (int seg0, int seg1);
int PointInSeg (CSegment* segP, CFixVector vPos);

//------------------------------------------------------------------------------

#define MAX_EXITFLIGHT_OBJS 2

tExitFlightData exitFlightObjects [MAX_EXITFLIGHT_OBJS];

//------------------------------------------------------------------------------

void InitEndLevelData (void)
{
gameData.endLevel.station.vPos.v.coord.x = 0xf8c4 << 10;
gameData.endLevel.station.vPos.v.coord.y = 0x3c1c << 12;
gameData.endLevel.station.vPos.v.coord.z = 0x0372 << 10;
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
int ELFindConnectedSide (int seg0, int seg1)
{
	CSegment *segP = SEGMENTS + seg0;
	
int i = SEGMENT_SIDE_COUNT;
for (; i; )
	if (segP->m_children [--i] == seg1)
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
r = movieManager.Play (szMovieName, (gameData.app.nGameMode & GM_MULTI) ? 0 : MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
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
if (gameData.endLevel.terrain.bmInstance.Buffer ()) {
	gameData.endLevel.terrain.bmInstance.ReleaseTexture ();
	gameData.endLevel.terrain.bmInstance.DestroyBuffer ();
	}
if (gameData.endLevel.satellite.bmInstance.Buffer ()) {
	gameData.endLevel.satellite.bmInstance.ReleaseTexture ();
	gameData.endLevel.satellite.bmInstance.DestroyBuffer ();
	}
}

//-----------------------------------------------------------------------------

void InitEndLevel (void)
{
#ifdef STATION_ENABLED
	gameData.endLevel.station.bmP = bm_load ("steel3.bbm");
	gameData.endLevel.station.bmList [0] = &gameData.endLevel.station.bmP;

	gameData.endLevel.station.nModel = LoadPolyModel ("station.pof", 1, gameData.endLevel.station.bmList, NULL);
#endif
GenerateStarfield ();
atexit (FreeEndLevelData);
gameData.endLevel.terrain.bmInstance.SetBuffer (NULL);
gameData.endLevel.satellite.bmInstance.SetBuffer (NULL);
}

//------------------------------------------------------------------------------

CObject externalExplosion;

CAngleVector vExitAngles = CAngleVector::Create(-0xa00, 0, 0);

CFixMatrix mSurfaceOrient;

void StartEndLevelSequence (int bSecret)
{
	CObject	*objP;
	int		nMoviePlayed = MOVIE_NOT_PLAYED;
	//int		i;

if (gameData.demo.nState == ND_STATE_RECORDING)		// stop demo recording
	gameData.demo.nState = ND_STATE_PAUSED;
if (gameData.demo.nState == ND_STATE_PLAYBACK) {		// don't do this if in playback mode
	if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) ||
		 ((missionManager.nCurrentMission == missionManager.nBuiltInMission [1]) &&
		 movieManager.m_bHaveExtras))
		StartEndLevelMovie ();
	paletteManager.SetLastLoaded ("");		//force palette load next time
	return;
	}
if (gameStates.app.bPlayerIsDead || (gameData.objs.consoleP->info.nFlags & OF_SHOULD_BE_DEAD))
	return;				//don't start if dead!
//	Dematerialize Buddy!
FORALL_ROBOT_OBJS (objP, i)
	if (IS_GUIDEBOT (objP)) {
			CreateExplosion (objP->info.nSegment, objP->info.position.vPos, I2X (7) / 2, VCLIP_POWERUP_DISAPPEARANCE);
			objP->Die ();
		}
LOCALPLAYER.homingObjectDist = -I2X (1); // Turn off homing sound.
ResetRearView ();		//turn off rear view if set
if (IsMultiGame) {
	MultiSendEndLevelStart (0);
	NetworkDoFrame (1, 1);
	}
if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) ||
	 ((missionManager.nCurrentMission == missionManager.nBuiltInMission [1]) && movieManager.m_bHaveExtras)) {
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
paletteManager.DisableEffect ();
if (!bSecret)
	PlayerFinishedLevel (0);		//done with level
}

//------------------------------------------------------------------------------

void StartRenderedEndLevelSequence (void)
{
	int nLastSeg, nExitSide, nTunnelLength;
	int nSegment, nOldSeg, nEntrySide, i;

//count segments in exit tunnel
nOldSeg = gameData.objs.consoleP->info.nSegment;
nExitSide = FindExitSide (gameData.objs.consoleP);
exitFlightObjects [0].entrySide = oppSideTable [nExitSide];
nSegment = SEGMENTS [nOldSeg].m_children [nExitSide];
nTunnelLength = 0;
do {
	nEntrySide = ELFindConnectedSide (nSegment, nOldSeg);
	nExitSide = oppSideTable [nEntrySide];
	nOldSeg = nSegment;
	nSegment = SEGMENTS [nSegment].m_children [nExitSide];
	nTunnelLength++;
	} while (nSegment >= 0);
if (nSegment != -2) {
	PlayerFinishedLevel (0);		//don't do special sequence
	return;
	}
nLastSeg = nOldSeg;
//now pick transition nSegment 1/3 of the way in
nOldSeg = gameData.objs.consoleP->info.nSegment;
nExitSide = FindExitSide (gameData.objs.consoleP);
nSegment = SEGMENTS [nOldSeg].m_children [nExitSide];
i = nTunnelLength / 3;
while (i--) {
	nEntrySide = ELFindConnectedSide (nSegment, nOldSeg);
	nExitSide = oppSideTable [nEntrySide];
	nOldSeg = nSegment;
	nSegment = SEGMENTS [nSegment].m_children [nExitSide];
	}
gameData.endLevel.exit.nTransitSegNum = nSegment;
CGenericCockpit::Save ();
if (IsMultiGame) {
	MultiSendEndLevelStart (0);
	NetworkDoFrame (1, 1);
	}
Assert (nLastSeg == gameData.endLevel.exit.nSegNum);
if (missionManager [missionManager.nCurrentMission].nDescentVersion == 1)
	songManager.Play (SONG_ENDLEVEL, 0);
gameStates.app.bEndLevelSequence = EL_FLYTHROUGH;
gameData.objs.consoleP->info.movementType = MT_NONE;			//movement handled by flythrough
gameData.objs.consoleP->info.controlType = CT_NONE;
if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS))
	gameStates.app.bGameSuspended |= SUSP_ROBOTS | SUSP_TEMPORARY;          //robots don't move
gameData.endLevel.xCurFlightSpeed = gameData.endLevel.xDesiredFlightSpeed = FLY_SPEED;
StartEndLevelFlyThrough (0, gameData.objs.consoleP, gameData.endLevel.xCurFlightSpeed);		//initialize
HUDInitMessage (TXT_EXIT_SEQUENCE);
gameStates.render.bOutsideMine = gameStates.render.bExtExplPlaying = 0;
gameStates.render.nFlashScale = I2X (1);
gameStates.gameplay.bMineDestroyed = 0;
}

//------------------------------------------------------------------------------

CAngleVector vPlayerAngles, vPlayerDestAngles;
CAngleVector vDesiredCameraAngles, vCurrentCameraAngles;

#define CHASE_TURN_RATE (0x4000/4)		//max turn per second

//returns bitmask of which angles are at dest. bits 0, 1, 2 = p, b, h
int ChaseAngles (CAngleVector *cur_angles, CAngleVector *desired_angles)
{
	CAngleVector	delta_angs, alt_angles, alt_delta_angs;
	fix			total_delta, altTotal_delta;
	fix			frame_turn;
	int			mask = 0;

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

frame_turn = FixMul (gameData.time.xFrame, CHASE_TURN_RATE);

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

#define VCLIP_BIG_PLAYER_EXPLOSION	58

//--unused-- CFixVector upvec = {0, I2X (1), 0};

//find the angle between the CPlayerData's heading & the station
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
	CFixVector vLastPosSave;
	static fix explosion_wait1=0;
	static fix explosion_wait2=0;
	static fix ext_expl_halflife;

vLastPosSave = gameData.objs.consoleP->info.vLastPos;	//don't let move code change this
UpdateAllObjects ();
gameData.objs.consoleP->info.vLastPos = vLastPosSave;

if (gameStates.render.bExtExplPlaying) {
	externalExplosion.info.xLifeLeft -= gameData.time.xFrame;
	externalExplosion.DoExplosionSequence ();
	if (externalExplosion.info.xLifeLeft < ext_expl_halflife)
		gameStates.gameplay.bMineDestroyed = 1;
	if (externalExplosion.info.nFlags & OF_SHOULD_BE_DEAD)
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
		CFixVector tvec;

		tvec = gameData.objs.consoleP->info.position.vPos - gameData.endLevel.exit.vSideExit;
		if (CFixVector::Dot (tvec, gameData.endLevel.exit.mOrient.m.dir.f) > 0) {
			CObject *objP;
			gameStates.render.bOutsideMine = 1;
			objP = CreateExplosion (gameData.endLevel.exit.nSegNum, gameData.endLevel.exit.vSideExit, I2X (50), VCLIP_BIG_PLAYER_EXPLOSION);
			if (objP) {
				externalExplosion = *objP;
				objP->Die ();
				gameStates.render.nFlashScale = 0;	//kill lights in mine
				ext_expl_halflife = objP->info.xLifeLeft;
				gameStates.render.bExtExplPlaying = 1;
				}
			audio.CreateSegmentSound (SOUND_BIG_ENDLEVEL_EXPLOSION, gameData.endLevel.exit.nSegNum, 0, gameData.endLevel.exit.vSideExit, 0, I2X (3)/4);
			}
		}

	//do explosions chasing CPlayerData
	if ((explosion_wait1 -= gameData.time.xFrame) < 0) {
		CFixVector	vPos;
		short			nSegment;
		static int	nSounds = 0;

		vPos = gameData.objs.consoleP->info.position.vPos + gameData.objs.consoleP->info.position.mOrient.m.dir.f * (-gameData.objs.consoleP->info.xSize * 5);
		vPos += gameData.objs.consoleP->info.position.mOrient.m.dir.r * (SRandShort () * 15);
		vPos += gameData.objs.consoleP->info.position.mOrient.m.dir.u * (SRandShort () * 15);
		nSegment = FindSegByPos (vPos, gameData.objs.consoleP->info.nSegment, 1, 0);
		if (nSegment != -1) {
			CreateExplosion (nSegment, vPos, I2X (20), VCLIP_BIG_PLAYER_EXPLOSION);
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
	if ((explosion_wait2 -= gameData.time.xFrame) < 0) {
		CFixVector	vPos;
		//create little explosion on CWall
		vPos = gameData.objs.consoleP->info.position.mOrient.m.dir.r * (SRandShort () * 100);
		vPos += gameData.objs.consoleP->info.position.mOrient.m.dir.u * (SRandShort () * 100);
		vPos += gameData.objs.consoleP->info.position.vPos;
		if (gameStates.app.bEndLevelSequence == EL_FLYTHROUGH)
			vPos += gameData.objs.consoleP->info.position.mOrient.m.dir.f * (RandShort () * 200);
		else
			vPos += gameData.objs.consoleP->info.position.mOrient.m.dir.f * (RandShort () * 60);

		//find hit point on CWall
		CHitQuery	hitQuery (0, &gameData.objs.consoleP->info.position.vPos, &vPos, gameData.objs.consoleP->info.nSegment);
		CHitResult	hitResult;

		FindHitpoint (hitQuery, hitResult);
		if ((hitResult.nType == HIT_WALL) && (hitResult.nSegment != -1))
			CreateExplosion ((short) hitResult.nSegment, hitResult.vPoint, I2X (3) + RandShort () * 6, VCLIP_SMALL_EXPLOSION);
		explosion_wait2 = (0xa00 + RandShort () / 8) / 2;
		}

switch (gameStates.app.bEndLevelSequence) {
	case EL_OFF:
		return;

	case EL_FLYTHROUGH: {
		if (!DoEndLevelFlyThrough (0) || (gameData.objs.consoleP->info.nSegment == gameData.endLevel.exit.nTransitSegNum)) {
			if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) && (StartEndLevelMovie () != MOVIE_NOT_PLAYED))
				StopEndLevelSequence ();
			else {
				gameStates.app.bEndLevelSequence = EL_LOOKBACK;
				int nObject = CreateCamera (gameData.objs.consoleP);
				if (nObject == -1) { //can't get CObject, so abort
#if TRACE
					console.printf (1, "Can't get CObject for endlevel sequence.  Aborting endlevel sequence.\n");
#endif
					StopEndLevelSequence ();
					return;
					}
				gameData.objs.viewerP = gameData.objs.endLevelCamera = OBJECTS + nObject;
				cockpit->Activate (CM_LETTERBOX);
				exitFlightObjects [1] = exitFlightObjects [0];
				exitFlightObjects [1].objP = gameData.objs.endLevelCamera;
				exitFlightObjects [1].speed = (5 * gameData.endLevel.xCurFlightSpeed) / 4;
				exitFlightObjects [1].lateralOffset = I2X (1) / 0x4000;
				gameData.objs.endLevelCamera->info.position.vPos += gameData.objs.endLevelCamera->info.position.mOrient.m.dir.f * (I2X (7));
				timer = 0x20000;
				}
			}
		break;
		}

	case EL_LOOKBACK: {
		int bFlyThrough = DoEndLevelFlyThrough (0);
		if (bFlyThrough)
			bFlyThrough = DoEndLevelFlyThrough (1);
		if (timer > 0) {
			timer -= gameData.time.xFrame;
			if (timer < 0)		//reduce speed
				exitFlightObjects [1].speed = exitFlightObjects [0].speed;
			}
		if (!bFlyThrough ||
			(gameData.objs.endLevelCamera->Segment () == gameData.endLevel.exit.nSegNum) || 
			 (OBJECTS [LOCALPLAYER.nObject].Segment () == gameData.endLevel.exit.nSegNum)) {
			gameStates.app.bEndLevelSequence = EL_OUTSIDE;
			timer = I2X (2);
			if (gameData.objs.endLevelCamera->Segment () != gameData.endLevel.exit.nSegNum) {
				gameData.objs.endLevelCamera->Position () = OBJECTS [LOCALPLAYER.nObject].Position ();
				gameData.objs.endLevelCamera->Orientation () = OBJECTS [LOCALPLAYER.nObject].Orientation ();
				}
			gameData.objs.endLevelCamera->info.position.mOrient.m.dir.f = -gameData.objs.endLevelCamera->info.position.mOrient.m.dir.f;
			gameData.objs.endLevelCamera->info.position.mOrient.m.dir.r = -gameData.objs.endLevelCamera->info.position.mOrient.m.dir.r;
			CAngleVector camAngles = gameData.objs.endLevelCamera->info.position.mOrient.ExtractAnglesVec ();
			CAngleVector exitAngles = gameData.endLevel.exit.mOrient.ExtractAnglesVec ();
			fix bankRate = (-exitAngles.v.coord.b - camAngles.v.coord.b) / 2;
			gameData.objs.consoleP->info.controlType = gameData.objs.endLevelCamera->info.controlType = CT_NONE;
#ifdef SLEW_ON
			slewObjP = gameData.objs.endLevelCamera;
#endif
			}
		break;
		}

	case EL_OUTSIDE: {
#ifndef SLEW_ON
		CAngleVector cam_angles;
#endif
		gameData.objs.consoleP->info.position.vPos += gameData.objs.consoleP->info.position.mOrient.m.dir.f * (FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed));
#ifndef SLEW_ON
		gameData.objs.endLevelCamera->info.position.vPos +=
							gameData.objs.endLevelCamera->info.position.mOrient.m.dir.f *
							(FixMul (gameData.time.xFrame, -2*gameData.endLevel.xCurFlightSpeed));
		gameData.objs.endLevelCamera->info.position.vPos +=
							gameData.objs.endLevelCamera->info.position.mOrient.m.dir.u *
							(FixMul (gameData.time.xFrame, -gameData.endLevel.xCurFlightSpeed/10));
		cam_angles = gameData.objs.endLevelCamera->info.position.mOrient.ExtractAnglesVec ();
		cam_angles.v.coord.b += (fixang) FixMul (bank_rate, gameData.time.xFrame);
		gameData.objs.endLevelCamera->info.position.mOrient = CFixMatrix::Create (cam_angles);
#endif
		timer -= gameData.time.xFrame;
		if (timer < 0) {
			gameStates.app.bEndLevelSequence = EL_STOPPED;
			vPlayerAngles = gameData.objs.consoleP->info.position.mOrient.ExtractAnglesVec ();
			timer = I2X (3);
			}
		break;
		}

	case EL_STOPPED: {
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevel.station.vPos, &gameData.objs.consoleP->info.position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		gameData.objs.consoleP->info.position.mOrient = CFixMatrix::Create (vPlayerAngles);
		gameData.objs.consoleP->info.position.vPos += gameData.objs.consoleP->info.position.mOrient.m.dir.f * (FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed));
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
			VmExtractAnglesMatrix (&vCurrentCameraAngles, &gameData.objs.endLevelCamera->info.position.mOrient);
			timer = I2X (3);
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
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevel.station.vPos, &gameData.objs.consoleP->info.position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		VmAngles2Matrix (&gameData.objs.consoleP->info.position.mOrient, &vPlayerAngles);
		VmVecScaleInc (&gameData.objs.consoleP->info.position.vPos, &gameData.objs.consoleP->info.position.mOrient.mat.dir.f, FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed);

#ifdef SLEW_ON
		DoSlewMovement (gameData.objs.endLevelCamera, 1, 1);
#else
		GetAnglesToObject (&vDesiredCameraAngles, &gameData.objs.consoleP->info.position.vPos, &gameData.objs.endLevelCamera->info.position.vPos);
		mask = ChaseAngles (&vCurrentCameraAngles, &vDesiredCameraAngles);
		VmAngles2Matrix (&gameData.objs.endLevelCamera->info.position.mOrient, &vCurrentCameraAngles);
		if ((mask & 5) == 5) {
			CFixVector tvec;
			gameStates.app.bEndLevelSequence = EL_CHASING;
			VmVecNormalizedDir (&tvec, &gameData.endLevel.station.vPos, &gameData.objs.consoleP->info.position.vPos);
			VmVector2Matrix (&gameData.objs.consoleP->info.position.mOrient, &tvec, &mSurfaceOrient.mat.dir.u, NULL);
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
		GetAnglesToObject (&vDesiredCameraAngles, &gameData.objs.consoleP->info.position.vPos, &gameData.objs.endLevelCamera->info.position.vPos);
		ChaseAngles (&vCurrentCameraAngles, &vDesiredCameraAngles);
#ifndef SLEW_ON
		VmAngles2Matrix (&gameData.objs.endLevelCamera->info.position.mOrient, &vCurrentCameraAngles);
#endif
		d = VmVecDistQuick (&gameData.objs.consoleP->info.position.vPos, &gameData.objs.endLevelCamera->info.position.vPos);
		speed_scale = FixDiv (d, I2X (0x20);
		if (d < I2X (1))
			d = I2X (1);
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevel.station.vPos, &gameData.objs.consoleP->info.position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		VmAngles2Matrix (&gameData.objs.consoleP->info.position.mOrient, &vPlayerAngles);
		VmVecScaleInc (&gameData.objs.consoleP->info.position.vPos, &gameData.objs.consoleP->info.position.mOrient.mat.dir.f, FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed);
#ifndef SLEW_ON
		VmVecScaleInc (&gameData.objs.endLevelCamera->info.position.vPos, &gameData.objs.endLevelCamera->info.position.mOrient.mat.dir.f, FixMul (gameData.time.xFrame, FixMul (speed_scale, gameData.endLevel.xCurFlightSpeed));
		if (VmVecDist (&gameData.objs.consoleP->info.position.vPos, &gameData.endLevel.station.vPos) < I2X (10))
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
int FindExitSide (CObject *objP)
{
	CFixVector	vPreferred, vSegCenter, vSide;
	fix			d, xBestVal = -I2X (2);
	int			nBestSide, i;
	CSegment		*segP = SEGMENTS + objP->info.nSegment;

//find exit CSide
CFixVector::NormalizedDir (vPreferred, objP->info.position.vPos, objP->info.vLastPos);
vSegCenter = segP->Center ();
nBestSide = -1;
for (i = 0; i < SEGMENT_SIDE_COUNT; i++) {
	if (segP->m_children [i] != -1) {
		vSide = segP->SideCenter (i);
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
return nBestSide;
}

//------------------------------------------------------------------------------

void DrawExitModel (void)
{
	CFixVector	vModelPos;
	int			f = 15, u = 0;	//21;

vModelPos = gameData.endLevel.exit.vMineExit + gameData.endLevel.exit.mOrient.m.dir.f * (I2X (f));
vModelPos += gameData.endLevel.exit.mOrient.m.dir.u * (I2X (u));
gameStates.app.bD1Model = gameStates.app.bD1Mission && gameStates.app.bD1Data;
DrawPolyModel (NULL, &vModelPos, &gameData.endLevel.exit.mOrient, NULL,
					gameStates.gameplay.bMineDestroyed ? gameData.endLevel.exit.nDestroyedModel : gameData.endLevel.exit.nModel,
					0, I2X (1), NULL, NULL, NULL);
gameStates.app.bD1Model = 0;
}

//------------------------------------------------------------------------------

int nExitPointBmX, nExitPointBmY;

fix xSatelliteSize = I2X (400);

#define SATELLITE_DIST		I2X (1024)
#define SATELLITE_WIDTH		xSatelliteSize
#define SATELLITE_HEIGHT	 ((xSatelliteSize*9)/4)		// ((xSatelliteSize*5)/2)

void RenderExternalScene (fix xEyeOffset)
{
	CFixVector vDelta;
	CRenderPoint p, pTop;

gameData.render.mine.viewer.vPos = gameData.objs.viewerP->info.position.vPos;
if (xEyeOffset)
	gameData.render.mine.viewer.vPos += gameData.objs.viewerP->info.position.mOrient.m.dir.r * (xEyeOffset);
SetupTransformation (transformation, gameData.objs.viewerP->info.position.vPos, gameData.objs.viewerP->info.position.mOrient, gameStates.render.xZoom, 1);
CCanvas::Current ()->Clear (BLACK_RGBA);
transformation.Begin (CFixVector::ZERO, mSurfaceOrient);
DrawStars ();
transformation.End ();
//draw satellite
p.TransformAndEncode (gameData.endLevel.satellite.vPos);
transformation.RotateScaled (vDelta, gameData.endLevel.satellite.vUp);
pTop.Add (p, vDelta);
if (!(p.Codes () & (CC_BEHIND | PF_OVERFLOW))) {
	int imSave = gameStates.render.nInterpolationMethod;
	gameStates.render.nInterpolationMethod = 0;
	//gameData.endLevel.satellite.bmP->SetTranspType (0);
	if (!gameData.endLevel.satellite.bmP->Bind (1))
		G3DrawRodTexPoly (gameData.endLevel.satellite.bmP, &p, SATELLITE_WIDTH, &pTop, SATELLITE_WIDTH, I2X (1), satUVL);
	gameStates.render.nInterpolationMethod = imSave;
	}
#ifdef STATION_ENABLED
DrawPolyModel (NULL, &gameData.endLevel.station.vPos, &vmdIdentityMatrix, NULL,
						gameData.endLevel.station.nModel, 0, I2X (1), NULL, NULL);
#endif
RenderTerrain (&gameData.endLevel.exit.vGroundExit, nExitPointBmX, nExitPointBmY);
DrawExitModel ();
if (gameStates.render.bExtExplPlaying)
	DrawFireball (&externalExplosion);
int nLighting = gameStates.render.nLighting;
gameStates.render.nLighting = 0;
RenderObject (gameData.objs.consoleP, 0, 0);
gameStates.render.nState = 0;
transparencyRenderer.Render (0);
gameStates.render.nLighting = nLighting;
}

//------------------------------------------------------------------------------

#define MAX_STARS 500

CFixVector stars [MAX_STARS];

void GenerateStarfield (void)
{
	int i;

for (i = 0; i < MAX_STARS; i++) {
	stars [i].v.coord.x = SRandShort () << 14;
	stars [i].v.coord.z = SRandShort () << 14;
	stars [i].v.coord.y = (RandShort () / 2) << 14;
	}
}

//------------------------------------------------------------------------------

void DrawStars ()
{
	int i;
	int intensity = 31;
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

void RenderEndLevelMine (fix xEyeOffset, int nWindowNum)
{
	short nStartSeg;

gameData.render.mine.viewer.vPos = gameData.objs.viewerP->info.position.vPos;
if (gameData.objs.viewerP->info.nType == OBJ_PLAYER)
	gameData.render.mine.viewer.vPos += gameData.objs.viewerP->info.position.mOrient.m.dir.f * ((gameData.objs.viewerP->info.xSize * 3) / 4);
if (xEyeOffset)
	gameData.render.mine.viewer.vPos += gameData.objs.viewerP->info.position.mOrient.m.dir.r * (xEyeOffset);
if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE) {
	nStartSeg = gameData.endLevel.exit.nSegNum;
	}
else {
	nStartSeg = FindSegByPos (gameData.render.mine.viewer.vPos, gameData.objs.viewerP->info.nSegment, 1, 0);
	if (nStartSeg == -1)
		nStartSeg = gameData.objs.viewerP->info.nSegment;
	}
if (gameStates.app.bEndLevelSequence == EL_LOOKBACK) {
	CFixMatrix headm, viewm;
	CAngleVector angles = CAngleVector::Create(0, 0, 0x7fff);
	headm = CFixMatrix::Create (angles);
	viewm = gameData.objs.viewerP->info.position.mOrient * headm;
	SetupTransformation (transformation, gameData.render.mine.viewer.vPos, viewm, gameStates.render.xZoom, 1);
	}
else
	SetupTransformation (transformation, gameData.render.mine.viewer.vPos, gameData.objs.viewerP->info.position.mOrient, gameStates.render.xZoom, 1);
RenderMine (nStartSeg, xEyeOffset, nWindowNum);
transparencyRenderer.Render (0);
}

//-----------------------------------------------------------------------------

void RenderEndLevelFrame (fix xStereoSeparation, int nWindow)
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

int ELFindConnectedSide (int seg0, int seg1);

fixang DeltaAng (fixang a, fixang b);

#define DEFAULT_SPEED I2X (16)

#define MIN_D 0x100

//if speed is zero, use default speed
void StartEndLevelFlyThrough (int nObject, CObject *objP, fix speed)
{
tExitFlightData& exitFlightData = exitFlightObjects [nObject];
exitFlightData.objP = objP;
exitFlightData.exitSide = -1;
exitFlightData.speed = speed ? speed : DEFAULT_SPEED;
exitFlightData.lateralOffset = 0;
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

int DoEndLevelFlyThrough (int nObject)
{
tExitFlightData& exitFlightData = exitFlightObjects [nObject];
CObject* objP = exitFlightData.objP;
int nPrevSegment = objP->info.nSegment;
int bStart = exitFlightData.exitSide < 0;
//move the CPlayerData for this frame
if (!bStart) {
	CFixVector vNewPos = objP->info.position.vPos + exitFlightData.vStep * gameData.time.xFrame;
	CFixVector v1 = exitFlightData.vStep;
	CFixVector::Normalize (v1);
	CFixVector v2 = exitFlightData.vDest - vNewPos;
	CFixVector::Normalize (v2);
	exitFlightData.destDist = CFixVector::Dist (objP->info.position.vPos, exitFlightData.vDest);
	exitFlightData.pathDot = CFixVector::Dot (v1, v2);
	objP->info.position.vPos = (exitFlightData.pathDot < 0) ? exitFlightData.vDest : vNewPos;
	PrintLog (0, "object %d: dest dist %1.2f, path dot %1.2f\n", OBJ_IDX (objP), X2F (exitFlightData.destDist), X2F (exitFlightData.pathDot));
	angvec_add2_scale (&exitFlightData.angles, &exitFlightData.aStep, gameData.time.xFrame);
	objP->info.position.mOrient = CFixMatrix::Create (exitFlightData.angles);
	if (UpdateObjectSeg (objP, false) < 0)
		objP->RelinkToSeg (nPrevSegment);
	else if (objP->Segment () != nPrevSegment)
		objP->RelinkToSeg (nPrevSegment);
	}
//check new CPlayerData seg
if (exitFlightData.pathDot < 0)
	nDbgSeg = nDbgSeg;
if (objP->info.nSegment < 0)
	return 0;
if (bStart || (exitFlightData.pathDot < 0)) {
	if (objP->info.nSegment < 0)
		return 0;
	CSegment*		segP = SEGMENTS + objP->info.nSegment;
	fix				xStepSize, xSegTime;
	short				nUpSide = 0;	//what sides we entry and leave through
	CFixVector		vDest;			//where we are heading (center of exitFlightData.exitSide)
	CAngleVector	aDest;			//where we want to be pointing
	CFixMatrix		mDest;

#if DBG
	if (nObject && objP->info.nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	if (!bStart) {
		if (segP->m_children [exitFlightData.exitSide] < 0) {
			PrintLog (0, "Exit sequence failure!\n");
			return 0;
			}
		exitFlightData.entrySide = ELFindConnectedSide (segP->m_children [exitFlightData.exitSide], objP->info.nSegment);
		}
	exitFlightData.exitSide = oppSideTable [exitFlightData.entrySide];
	fix dLargest = -I2X (1);
	for (short i = 0; i < 6; i++) {
		fix d = CFixVector::Dot (segP->m_sides [i].m_normals [2], exitFlightData.objP->info.position.mOrient.m.dir.u);
		if (d > dLargest) {
			dLargest = d; 
			nUpSide = i;
			}
		}
	PrintLog (0, "object %d: exit seg %d, exit side %d, up side %d\n", OBJ_IDX (objP), objP->info.nSegment, exitFlightData.exitSide, nUpSide);
	//update target point & angles
	vDest = segP->SideCenter (exitFlightData.exitSide);
	//update target point and movement points
	//offset CObject sideways
	if (exitFlightData.lateralOffset) {
		short s0 = 0;
		for (; s0 < 6; s0++)
			if ((s0 != exitFlightData.entrySide) && (s0 != exitFlightData.exitSide) && (s0 != nUpSide) && (s0 != oppSideTable [nUpSide])) 
				break;
		if (s0 < 6) {
			int s1 = oppSideTable [s0];
			CFixVector s0Center = segP->SideCenter (s0);
			CFixVector s1Center = segP->SideCenter (s1);
			fix dist = CFixVector::Dist (s0Center, s1Center);
			PrintLog (0, "object %d: width (%d, %d) %1.2f\n", OBJ_IDX (objP), s0, s1, X2F (dist));
#if 1
			dist /= exitFlightData.lateralOffset;
#else
			dist = FixMul (dist, exitFlightData.lateralOffset);
#endif
			if (dist - exitFlightData.offsetDist > MAX_SLIDE_PER_SEGMENT)
				dist = exitFlightData.offsetDist + MAX_SLIDE_PER_SEGMENT;
			PrintLog (0, "object %d: offset %1.2f\n", OBJ_IDX (objP), X2F (dist));
			exitFlightData.offsetDist = dist;
			CFixVector v;
			do {
				v = vDest + objP->info.position.mOrient.m.dir.r * dist;
				dist = 9 * dist / 10;
				} while (dist && !PointInSeg (segP, v));
			vDest = v;
			}	
		}
	exitFlightData.vDest = vDest;
	exitFlightData.vStep = vDest - objP->info.position.vPos;
	xStepSize = CFixVector::Normalize (exitFlightData.vStep);
	if (exitFlightData.pathDot < 0) 
		objP->info.position.vPos += exitFlightData.vStep * exitFlightData.destDist;
	exitFlightData.vStep *= exitFlightData.speed;
	exitFlightData.vHeading = SEGMENTS [segP->m_children [exitFlightData.exitSide]].Center ();
	exitFlightData.vHeading -= segP->Center ();
	mDest = CFixMatrix::CreateFU (exitFlightData.vHeading, segP->m_sides [nUpSide].m_normals [2]);
	aDest = mDest.ExtractAnglesVec ();
	if (bStart)
		exitFlightData.angles = objP->info.position.mOrient.ExtractAnglesVec ();
	xSegTime = FixDiv (xStepSize, exitFlightData.speed);	//how long through seg
	if (xSegTime) {
		exitFlightData.aStep.v.coord.x = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (exitFlightData.angles.v.coord.p, aDest.v.coord.p), xSegTime)));
		exitFlightData.aStep.v.coord.z = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (exitFlightData.angles.v.coord.b, aDest.v.coord.b), xSegTime)));
		exitFlightData.aStep.v.coord.y = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (exitFlightData.angles.v.coord.h, aDest.v.coord.h), xSegTime)));
		}
	else {
		exitFlightData.angles = aDest;
		exitFlightData.aStep.SetZero ();
		}
	}
return 1;
}

//------------------------------------------------------------------------------

#ifdef SLEW_ON		//this is a special routine for slewing around external scene
int DoSlewMovement (CObject *objP, int check_keys, int check_joy)
{
	int moved = 0;
	CFixVector svel, movement;				//scaled velocity (per this frame)
	CFixMatrix rotmat, new_pm;
	int joy_x, joy_y, btns;
	int joyx_moved, joyy_moved;
	CAngleVector rotang;

if (gameStates.input.keys.dir.coord.pressed [KEY_PAD5])
	VmVecZero (&objP->physInfo.velocity);

if (check_keys) {
	objP->physInfo.velocity.x += VEL_SPEED * (KeyDownTime (KEY_PAD9) - KeyDownTime (KEY_PAD7);
	objP->physInfo.velocity.y += VEL_SPEED * (KeyDownTime (KEY_PADMINUS) - KeyDownTime (KEY_PADPLUS);
	objP->physInfo.velocity.z += VEL_SPEED * (KeyDownTime (KEY_PAD8) - KeyDownTime (KEY_PAD2);

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
		if (!rotang.dir.coord.pitch) rotang.dir.coord.pitch = FixMul (-joy_y * 512, gameData.time.xFrame); else;
	else
		if (joyy_moved) objP->physInfo.velocity.z = -joy_y * 8192;
	if (!rotang.head) rotang.head = FixMul (joy_x * 512, gameData.time.xFrame);
	if (joyx_moved)
		old_joy_x = joy_x;
	if (joyy_moved)
		old_joy_y = joy_y;
	}
moved = rotang.dir.coord.pitch | rotang.bank | rotang.head;
VmAngles2Matrix (&rotmat, &rotang);
VmMatMul (&new_pm, &objP->info.position.mOrient, &rotmat);
objP->info.position.mOrient = new_pm;
VmTransposeMatrix (&new_pm);		//make those columns rows
moved |= objP->physInfo.velocity.x | objP->physInfo.velocity.y | objP->physInfo.velocity.z;
svel = objP->physInfo.velocity;
VmVecScale (&svel, gameData.time.xFrame);		//movement in this frame
VmVecRotate (&movement, &svel, &new_pm);
VmVecInc (&objP->info.position.vPos, &movement);
moved |= (movement.x || movement.y || movement.z);
return moved;
}
#endif

//-----------------------------------------------------------------------------

#define LINE_LEN	80
#define NUM_VARS	8

#define STATION_DIST	I2X (1024)

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
	CFile		cf;
	int		var, nSegment, nSide;
	int		nExitSide = 0, i;
	int		bHaveBinary = 0;

gameStates.app.bEndLevelDataLoaded = 0;		//not loaded yet
if (!gameOpts->movies.nLevel)
	return;

try_again:

if (gameStates.app.bAutoRunMission)
	strcpy (filename, szAutoMission);
else if (nLevel < 0)		//secret level
	strcpy (filename, missionManager.szSecretLevelNames [-nLevel-1]);
else					//Normal level
	strcpy (filename, missionManager.szLevelNames [nLevel-1]);
if (!ConvertExt (filename, "end"))
	Error ("Error converting filename\n'<%s>'\nfor endlevel data\n", filename);
if (!cf.Open (filename, gameFolders.szDataDir [0], "rb", gameStates.app.bD1Mission)) {
	ConvertExt (filename, "txb");
	if (!cf.Open (filename, gameFolders.szDataDir [0], "rb", gameStates.app.bD1Mission)) {
		if (nLevel == 1) {
#if TRACE
			console.printf (CON_DBG, "Cannot load file text\nof binary version of\n'<%s>'\n", filename);
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
while (cf.GetS (line, LINE_LEN)) {
	if (bHaveBinary) {
		int l = (int) strlen (line);
		for (i = 0; i < l; i++)
			line [i] = EncodeRotateLeft ((char) (EncodeRotateLeft (line [i]) ^ BITMAP_TBL_XOR));
		p = line;
		}
	if ((p = strchr (line, ';')))
		*p = 0;		//cut off comment
	for (p = line + strlen (line) - 1; (p > line) && ::isspace ((ubyte) *p); *p-- = 0)
		;
	for (p = line; ::isspace ((ubyte) *p); p++)
		;
	if (!*p)		//empty line
		continue;
	switch (var) {
		case 0: {						//ground terrain
			CIFF iff;
			int iffError;

			if (gameData.endLevel.terrain.bmInstance.Buffer ()) {
				gameData.endLevel.terrain.bmInstance.ReleaseTexture ();
				gameData.endLevel.terrain.bmInstance.DestroyBuffer ();
				}
			Assert (gameData.endLevel.terrain.bmInstance.Buffer () == NULL);
			iffError = iff.ReadBitmap (p, &gameData.endLevel.terrain.bmInstance, BM_LINEAR);
			if (iffError != IFF_NO_ERROR) {
				gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
				return;
				}
			gameData.endLevel.terrain.bmP = &gameData.endLevel.terrain.bmInstance;
			//GrRemapBitmapGood (gameData.endLevel.terrain.bmP, NULL, iff_transparent_color, -1);
			break;
		}

		case 1:							//height map
			LoadTerrain (p);
			break;

		case 2:
			sscanf (p, "%d, %d", &nExitPointBmX, &nExitPointBmY);
			break;

		case 3:							//exit heading
			vExitAngles.v.coord.h = I2X (atoi (p))/360;
			break;

		case 4: {						//planet bitmap
			CIFF iff;
			int iffError;

			if (gameData.endLevel.satellite.bmInstance.Buffer ()) {
				gameData.endLevel.satellite.bmInstance.ReleaseTexture ();
				gameData.endLevel.satellite.bmInstance.DestroyBuffer ();
				}
			iffError = iff.ReadBitmap (p, &gameData.endLevel.satellite.bmInstance, BM_LINEAR);
			if (iffError != IFF_NO_ERROR) {
				Warning (TXT_SATELLITE, p, iff.ErrorMsg (iffError));
				gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
				return;
			}
			gameData.endLevel.satellite.bmP = &gameData.endLevel.satellite.bmInstance;
			//GrRemapBitmapGood (gameData.endLevel.satellite.bmP, NULL, iff_transparent_color, -1);
			break;
		}

		case 5:							//earth pos
		case 7: {						//station pos
			CFixMatrix tm;
			CAngleVector ta;
			int pitch, head;

			sscanf (p, "%d, %d", &head, &pitch);
			ta.v.coord.h = I2X (head)/360;
			ta.v.coord.p = -I2X (pitch)/360;
			ta.v.coord.b = 0;
			tm = CFixMatrix::Create(ta);
			if (var == 5)
				gameData.endLevel.satellite.vPos = tm.m.dir.f;
				//VmVecCopyScale (&gameData.endLevel.satellite.vPos, &tm.m.v.f, SATELLITE_DIST);
			else
				gameData.endLevel.station.vPos = tm.m.dir.f;
			break;
		}

		case 6:						//planet size
			xSatelliteSize = I2X (atoi (p));
			break;
	}
	var++;
}

Assert (var == NUM_VARS);
// OK, now the data is loaded.  Initialize everything
//find the exit sequence by searching all segments for a CSide with
//children == -2
for (nSegment = 0, gameData.endLevel.exit.nSegNum = -1;
	  (gameData.endLevel.exit.nSegNum == -1) && (nSegment <= gameData.segs.nLastSegment);
	  nSegment++)
	for (nSide = 0; nSide < 6; nSide++)
		if (SEGMENTS [nSegment].m_children [nSide] == -2) {
			gameData.endLevel.exit.nSegNum = nSegment;
			nExitSide = nSide;
			break;
			}

if (gameData.endLevel.exit.nSegNum == -1) {
#if DBG
	Warning (TXT_EXIT_TERRAIN, p, 12);
#endif
	gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
	return;
	}
gameData.endLevel.exit.vMineExit = SEGMENTS [gameData.endLevel.exit.nSegNum].Center ();
ExtractOrientFromSegment (&gameData.endLevel.exit.mOrient, &SEGMENTS [gameData.endLevel.exit.nSegNum]);
gameData.endLevel.exit.vSideExit = SEGMENTS [gameData.endLevel.exit.nSegNum].SideCenter (nExitSide);
gameData.endLevel.exit.vGroundExit = gameData.endLevel.exit.vMineExit + gameData.endLevel.exit.mOrient.m.dir.u * (-I2X (20));
//compute orientation of surface
{
	CFixVector tv;
	CFixMatrix exit_orient, tm;

	exit_orient = CFixMatrix::Create (vExitAngles);
	CFixMatrix::Transpose (exit_orient);
	mSurfaceOrient = gameData.endLevel.exit.mOrient * exit_orient;
	tm = mSurfaceOrient.Transpose();
	tv = tm * gameData.endLevel.station.vPos;
	gameData.endLevel.station.vPos = gameData.endLevel.exit.vMineExit + tv * STATION_DIST;
	tv = tm * gameData.endLevel.satellite.vPos;
	gameData.endLevel.satellite.vPos = gameData.endLevel.exit.vMineExit + tv * SATELLITE_DIST;
	tm = CFixMatrix::CreateFU (tv, mSurfaceOrient.m.dir.u);
	gameData.endLevel.satellite.vUp = tm.m.dir.u * SATELLITE_HEIGHT;
	}
cf.Close ();
gameStates.app.bEndLevelDataLoaded = 1;
}

//------------------------------------------------------------------------------
//eof

