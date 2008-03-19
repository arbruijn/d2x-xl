/* $Id: cntrlcen.c,v 1.14 2003/11/26 12:26:29 btb Exp $ */
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

#ifdef RCS
static char rcsid[] = "$Id: cntrlcen.c,v 1.14 2003/11/26 12:26:29 btb Exp $";
#endif

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "pstypes.h"
#include "error.h"
#include "mono.h"

#include "inferno.h"
#include "reactor.h"
#include "game.h"
#include "laser.h"
#include "loadgame.h"
#include "ai.h"
#include "multi.h"
#include "wall.h"
#include "object.h"
#include "robot.h"
#include "vclip.h"
#include "fireball.h"
#include "endlevel.h"
#include "network.h"

//@@vmsVector controlcen_gun_points[MAX_CONTROLCEN_GUNS];
//@@vmsVector controlcen_gun_dirs[MAX_CONTROLCEN_GUNS];

void DoCountdownFrame ();

//	-----------------------------------------------------------------------------
//return the position & orientation of a gun on the control center tObject
void CalcReactorGunPoint (vmsVector *vGunPoint, vmsVector *vGunDir, tObject *objP, int nGun)
{
	tReactorProps	*props;
	vmsMatrix		*viewP = ObjectView (objP);

Assert (objP->nType == OBJ_REACTOR);
Assert (objP->renderType == RT_POLYOBJ);
props = &gameData.reactor.props [objP->id];
//instance gun position & orientation
VmVecRotate (vGunPoint, props->gunPoints + nGun, viewP);
VmVecInc (vGunPoint, &objP->position.vPos);
VmVecRotate (vGunDir, props->gun_dirs + nGun, viewP);
}

//	-----------------------------------------------------------------------------
//	Look at control center guns, find best one to fire at *objP.
//	Return best gun number (one whose direction dotted with vector to tPlayer is largest).
//	If best gun has negative dot, return -1, meaning no gun is good.
int CalcBestReactorGun (int nGunCount, vmsVector *vGunPos, vmsVector *vGunDir, vmsVector *vObjPos)
{
	int	i;
	fix	xBestDot;
	int	nBestGun;

xBestDot = -F1_0*2;
nBestGun = -1;

for (i = 0; i < nGunCount; i++) {
	fix			dot;
	vmsVector	vGun;

	VmVecSub (&vGun, vObjPos, vGunPos + i);
	VmVecNormalizeQuick (&vGun);
	dot = VmVecDot (vGunDir + i, &vGun);
	if (dot > xBestDot) {
		xBestDot = dot;
		nBestGun = i;
		}
	}
Assert (nBestGun != -1);		// Contact Mike.  This is impossible.  Or maybe you're getting an unnormalized vector somewhere.
if (xBestDot < 0)
	return -1;
return nBestGun;
}

//how long to blow up on insane
int nAlanPavlishReactorTimes [2][NDL] = {{90,60,45,35,30},{50,45,40,35,30}};

//	-----------------------------------------------------------------------------
//	Called every frame.  If control center been destroyed, then actually do something.
void DoReactorDeadFrame (void)
{
if (gameStates.gameplay.nReactorCount) {
	int	i;
	tReactorStates	*rStatP = gameData.reactor.states;

	for (i = 0; i < gameStates.gameplay.nReactorCount; i++, rStatP++) {
		if ((rStatP->nDeadObj != -1) && 
			 (gameData.objs.objects [rStatP->nDeadObj].nType == OBJ_REACTOR) &&
			 (gameData.reactor.countdown.nSecsLeft > 0))
		if (d_rand () < gameData.time.xFrame * 4)
			CreateSmallFireballOnObject (gameData.objs.objects + rStatP->nDeadObj, F1_0, 1);
		}
	}
if (!gameStates.app.bEndLevelSequence)
	DoCountdownFrame ();
}

//	-----------------------------------------------------------------------------

#define COUNTDOWN_VOICE_TIME fl2f (12.75)

void DoCountdownFrame (void)
{
	fix	oldTime;
	int	fc, h, xScale;

	static fix cdtFrameTime;

if (!gameData.reactor.bDestroyed) {
	cdtFrameTime = 0;
	return;
	}
#if 0//def _DEBUG
return;
#endif
cdtFrameTime += gameData.time.xRealFrame;
if (gameStates.limitFPS.bCountDown && !gameStates.app.tick40fps.bTick)
	return;
if (!IS_D2_OEM && !IS_MAC_SHARE && !IS_SHAREWARE) {  // get countdown in OEM and SHAREWARE only
	// On last level, we don't want a countdown.
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) && 
		 (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)) {
		if (!IsMultiGame)
			return;
		if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
			return;
		}
	}

//	Control center destroyed, rock the tPlayer's ship.
fc = gameData.reactor.countdown.nSecsLeft;
if (fc > 16)
	fc = 16;
//	At Trainee, decrease rocking of ship by 4x.
xScale = 1;
if (gameStates.app.nDifficultyLevel == 0)
	xScale = 4;
h = 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32;
gameData.objs.console->mType.physInfo.rotVel.p.x += (FixMul (d_rand () - 16384, h)) / xScale;
gameData.objs.console->mType.physInfo.rotVel.p.z += (FixMul (d_rand () - 16384, h)) / xScale;
//	Hook in the rumble sound effect here.
oldTime = gameData.reactor.countdown.nTimer;
if (!TimeStopped ())
	gameData.reactor.countdown.nTimer -= cdtFrameTime;
if (IsMultiGame &&  NetworkIAmMaster ())
	MultiSendCountdown ();
cdtFrameTime = 0;
gameData.reactor.countdown.nSecsLeft = f2i (gameData.reactor.countdown.nTimer + F1_0 * 7 / 8);
if ((oldTime > COUNTDOWN_VOICE_TIME) && (gameData.reactor.countdown.nTimer <= COUNTDOWN_VOICE_TIME))	{
	DigiPlaySample (SOUND_COUNTDOWN_13_SECS, F3_0);
	}
if (f2i (oldTime + F1_0 * 7 / 8) != gameData.reactor.countdown.nSecsLeft) {
	if ((gameData.reactor.countdown.nSecsLeft >= 0) && (gameData.reactor.countdown.nSecsLeft < 10))
		DigiPlaySample ((short) (SOUND_COUNTDOWN_0_SECS + gameData.reactor.countdown.nSecsLeft), F3_0);
	if (gameData.reactor.countdown.nSecsLeft == gameData.reactor.countdown.nTotalTime - 1)
		DigiPlaySample (SOUND_COUNTDOWN_29_SECS, F3_0);
	}					
if (gameData.reactor.countdown.nTimer > 0) {
	fix size = (i2f (gameData.reactor.countdown.nTotalTime) - gameData.reactor.countdown.nTimer) / fl2f (0.65);
	fix oldSize = (i2f (gameData.reactor.countdown.nTotalTime) - oldTime) / fl2f (0.65);
	if ((size != oldSize) && (gameData.reactor.countdown.nSecsLeft < gameData.reactor.countdown.nTotalTime - 5))	// Every 2 seconds!
		DigiPlaySample (SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0);
	}
else {
	int flashValue = f2i (-gameData.reactor.countdown.nTimer * (64 / 4));	// 4 seconds to total whiteness
	if (oldTime > 0)
		DigiPlaySample (SOUND_MINE_BLEW_UP, F1_0);
	PALETTE_FLASH_SET (flashValue, flashValue, flashValue);
	if (gameStates.ogl.palAdd.blue > 64) {
		GrSetCurrentCanvas (NULL);
		GrClearCanvas (RGBA_PAL2 (31,31,31));	//make screen all white to match palette effect
		ResetCockpit ();		//force cockpit redraw next time
		ResetPaletteAdd ();	//restore palette for death message
		DoPlayerDead ();		//kill_player ();
		}																			
	}
}

//	-----------------------------------------------------------------------------

void InitCountdown (tTrigger *trigP, int bReactorDestroyed, int nTimer)
{
if (trigP && (trigP->time > 0))
	gameData.reactor.countdown.nTotalTime = trigP->time;
else if (gameStates.app.nBaseCtrlCenExplTime != DEFAULT_CONTROL_CENTER_EXPLOSION_TIME)
	gameData.reactor.countdown.nTotalTime = gameStates.app.nBaseCtrlCenExplTime + gameStates.app.nBaseCtrlCenExplTime * (NDL-gameStates.app.nDifficultyLevel-1)/2;
else
	gameData.reactor.countdown.nTotalTime = nAlanPavlishReactorTimes [gameStates.app.bD1Mission][gameStates.app.nDifficultyLevel];
gameData.reactor.countdown.nTimer = (nTimer < 0) ? i2f (gameData.reactor.countdown.nTotalTime) : (nTimer ? nTimer : i2f (1));
if (bReactorDestroyed)
	gameData.reactor.bDestroyed = 1;
}

//	-----------------------------------------------------------------------------
//	Called when control center gets destroyed.
//	This code is common to whether control center is implicitly imbedded in a boss,
//	or is an tObject of its own.
//	if objP == NULL that means the boss was the control center and don't set gameData.reactor.nDeadObj
void DoReactorDestroyedStuff (tObject *objP)
{
	int		i, bFinalCountdown, bReactor = objP && (objP->nType == OBJ_REACTOR);
	tTrigger	*trigP = NULL;

if ((gameData.app.nGameMode & GM_MULTI_ROBOTS) && gameData.reactor.bDestroyed)
   return; // Don't allow resetting if control center and boss on same level
// Must toggle walls whether it is a boss or control center.
if ((!objP || (objP->nType == OBJ_ROBOT)) && gameStates.gameplay.bKillBossCheat)
	return;
// And start the countdown stuff.
bFinalCountdown = !(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses && extraGameInfo [0].nBossCount);
if (bFinalCountdown ||
	 (gameStates.app.bD2XLevel && bReactor && (trigP = FindObjTrigger (OBJ_IDX (objP), TT_COUNTDOWN, -1)))) {
//	If a secret level, delete secret.sgc to indicate that we can't return to our secret level.
	if (bFinalCountdown) {
		for (i = 0; i < gameData.reactor.triggers.nLinks; i++)
			WallToggle (gameData.segs.segments + gameData.reactor.triggers.nSegment [i], gameData.reactor.triggers.nSide [i]);
		if (gameData.missions.nCurrentLevel < 0)
			CFDelete ("secret.sgc", gameFolders.szSaveDir);
		}
	InitCountdown (trigP, bFinalCountdown, -1);
	}
if (bReactor) {
	ExecObjTriggers (OBJ_IDX (objP), 0);
	if (0 <= (i = FindReactor (objP))) {
		gameData.reactor.states [i].nDeadObj = OBJ_IDX (objP);
		gameStates.gameplay.nReactorCount--;
		}
	}
}

//	-----------------------------------------------------------------------------

int FindReactor (tObject *objP)
{
	int	i, nObject = OBJ_IDX (objP);

for (i = 0; i <= gameStates.gameplay.nLastReactor; i++)
	if (gameData.reactor.states [i].nObject == nObject)
		return i;
return -1;
}

//	-----------------------------------------------------------------------------

void RemoveReactor (tObject *objP)
{
	int	i = FindReactor (objP);

if (i < 0)
	return;
if (i < --gameStates.gameplay.nReactorCount) 
	gameData.reactor.states [i] = gameData.reactor.states [gameStates.gameplay.nReactorCount];
memset (gameData.reactor.states + gameStates.gameplay.nReactorCount, 0, 
		  sizeof (gameData.reactor.states [gameStates.gameplay.nReactorCount]));
}

//	-----------------------------------------------------------------------------
//do whatever this thing does in a frame
void DoReactorFrame (tObject *objP)
{
	int				nBestGun, i;
	tReactorStates	*rStatP;

	//	If a boss level, then gameData.reactor.bPresent will be 0.
if (!gameData.reactor.bPresent)
	return;
i = FindReactor (objP);
if (i < 0)
	return;
rStatP = gameData.reactor.states + i;
#ifdef _DEBUG
if (!gameStates.app.cheats.bRobotsFiring || (gameStates.app.bGameSuspended & SUSP_ROBOTS))
	return;
#else
if (!gameStates.app.cheats.bRobotsFiring)
	return;
#endif

if (!(rStatP->bHit || rStatP->bSeenPlayer)) {
	if (gameStates.app.tick40fps.bTick) {		//	Do ever so often...
		vmsVector	vecToPlayer;
		fix			xDistToPlayer;
		int			i;
		tSegment		*segP = gameData.segs.segments + objP->nSegment;

		// This is a hack.  Since the control center is not processed by
		// ai_do_frame, it doesn't know how to deal with cloaked dudes.  It
		// seems to work in single-tPlayer mode because it is actually using
		// the value of Believed_player_position that was set by the last
		// person to go through ai_do_frame.  But since a no-robots game
		// never goes through ai_do_frame, I'm making it so the control
		// center can spot cloaked dudes.

		if (IsMultiGame)
			gameData.ai.vBelievedPlayerPos = gameData.objs.objects [LOCALPLAYER.nObject].position.vPos;

		//	Hack for special control centers which are isolated and not reachable because the
		//	real control center is inside the boss.
		for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
			if (IS_CHILD (segP->children [i]))
				break;
		if (i == MAX_SIDES_PER_SEGMENT)
			return;

		VmVecSub (&vecToPlayer, &gameData.objs.console->position.vPos, &objP->position.vPos);
		xDistToPlayer = VmVecNormalizeQuick (&vecToPlayer);
		if (xDistToPlayer < F1_0 * 200) {
			rStatP->bSeenPlayer = ObjectCanSeePlayer (objP, &objP->position.vPos, 0, &vecToPlayer);
			rStatP->nNextFireTime = 0;
			}
		}		
	return;
	}

//	Periodically, make the reactor fall asleep if tPlayer not visible.
if (rStatP->bHit || rStatP->bSeenPlayer) {
	if ((rStatP->xLastVisCheckTime + F1_0 * 5 < gameData.time.xGame) || 
		 (rStatP->xLastVisCheckTime > gameData.time.xGame)) {
		vmsVector	vecToPlayer;
		fix			xDistToPlayer;

		VmVecSub (&vecToPlayer, &gameData.objs.console->position.vPos, &objP->position.vPos);
		xDistToPlayer = VmVecNormalizeQuick (&vecToPlayer);
		rStatP->xLastVisCheckTime = gameData.time.xGame;
		if (xDistToPlayer < F1_0 * 120) {
			rStatP->bSeenPlayer = ObjectCanSeePlayer (objP, &objP->position.vPos, 0, &vecToPlayer);
			if (!rStatP->bSeenPlayer)
				rStatP->bHit = 0;
			}
		}
	}

if ((rStatP->nNextFireTime < 0) && 
	 !(gameStates.app.bPlayerIsDead && (gameData.time.xGame > gameStates.app.nPlayerTimeOfDeath + F1_0 * 2))) {
	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)
		nBestGun = CalcBestReactorGun (gameData.reactor.props [objP->id].nGuns, rStatP->vGunPos, rStatP->vGunDir, &gameData.ai.vBelievedPlayerPos);
	else
		nBestGun = CalcBestReactorGun (gameData.reactor.props [objP->id].nGuns, rStatP->vGunPos, rStatP->vGunDir, &gameData.objs.console->position.vPos);

	if (nBestGun != -1) {
		int			nRandProb, count;
		vmsVector	vecToGoal;
		fix			xDistToPlayer;
		fix			xDeltaFireTime;

		if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
			VmVecSub (&vecToGoal, &gameData.ai.vBelievedPlayerPos, &rStatP->vGunPos [nBestGun]);
			xDistToPlayer = VmVecNormalizeQuick (&vecToGoal);
			} 
		else {
			VmVecSub (&vecToGoal, &gameData.objs.console->position.vPos, &rStatP->vGunPos [nBestGun]);
			xDistToPlayer = VmVecNormalizeQuick (&vecToGoal);
			}
		if (xDistToPlayer > F1_0 * 300) {
			rStatP->bHit = 0;
			rStatP->bSeenPlayer = 0;
			return;
			}
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendCtrlcenFire (&vecToGoal, nBestGun, OBJ_IDX (objP));
		CreateNewLaserEasy (&vecToGoal, &rStatP->vGunPos[nBestGun], OBJ_IDX (objP), CONTROLCEN_WEAPON_NUM, 1);
		//	some of time, based on level, fire another thing, not directly at tPlayer, so it might hit him if he's constantly moving.
		nRandProb = F1_0 / (abs (gameData.missions.nCurrentLevel) / 4 + 2);
		count = 0;
		while ((d_rand () > nRandProb) && (count < 4)) {
			vmsVector	vRand;

			MakeRandomVector (&vRand);
			VmVecScaleInc (&vecToGoal, &vRand, F1_0/6);
			VmVecNormalizeQuick (&vecToGoal);
			if (IsMultiGame)
				MultiSendCtrlcenFire (&vecToGoal, nBestGun, OBJ_IDX (objP));
			CreateNewLaserEasy (&vecToGoal, &rStatP->vGunPos[nBestGun], OBJ_IDX (objP), CONTROLCEN_WEAPON_NUM, 0);
			count++;
			}
		xDeltaFireTime = (NDL - gameStates.app.nDifficultyLevel) * F1_0/4;
		if (gameStates.app.nDifficultyLevel == 0)
			xDeltaFireTime += (fix) (F1_0 / 2 * gameStates.gameplay.slowmo [0].fSpeed);
		if (IsMultiGame) // slow down rate of fire in multi player
			xDeltaFireTime *= 2;
		rStatP->nNextFireTime = xDeltaFireTime;
		}
	} 
else
	rStatP->nNextFireTime -= gameData.physics.xTime;
}

//	-----------------------------------------------------------------------------

fix ReactorStrength (void)
{
if (gameData.reactor.nStrength == -1) {		//use old defaults
	//	Boost control center strength at higher levels.
	if (gameData.missions.nCurrentLevel >= 0)
		return F1_0 * 200 + F1_0 * 50 * gameData.missions.nCurrentLevel;
	return F1_0 * 200 - gameData.missions.nCurrentLevel * F1_0 * 150;
	}
return i2f (gameData.reactor.nStrength);
}

//	-----------------------------------------------------------------------------
//	This must be called at the start of each level.
//	If this level contains a boss and mode != multiplayer, don't do control center stuff.  (Ghost out control center tObject.)
//	If this level contains a boss and mode == multiplayer, do control center stuff.
void InitReactorForLevel (int bRestore)
{
	int		i, j, nGuns, bNew;
	tObject	*objP;
	short		nBossObj = -1;
	tReactorStates	*rStatP = gameData.reactor.states;

gameStates.gameplay.bMultiBosses = gameStates.app.bD2XLevel && EGI_FLAG (bMultiBosses, 0, 0, 0);
extraGameInfo [0].nBossCount = 0;
gameStates.gameplay.nReactorCount = 0;
gameData.reactor.bPresent = 0;
if (bRestore) {
	for (i = 0; i < MAX_BOSS_COUNT; i++)
		if (gameData.reactor.states [i].nObject <= 0) {
			gameStates.gameplay.nLastReactor = i - 1;
			break;
			}
	}
else {
	gameStates.gameplay.nLastReactor = -1;
	memset (gameData.reactor.states, 0xff, sizeof (gameData.reactor.states));
	}
for (i = 0, objP = gameData.objs.objects; i <= gameData.objs.nLastObject; i++, objP++) {
	if (objP->nType == OBJ_REACTOR) {
		if (gameStates.gameplay.nReactorCount && !(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)) {
#if TRACE
			con_printf (1, "Warning: Two or more control centers including %i and %i\n", 
							gameData.reactor.states [0].nObject, i);
#endif
			}			
		//else 
			{
			//	Compute all gun positions.
			objP = gameData.objs.objects + i;
			if ((bNew = (!bRestore || (0 > (j = FindReactor (objP))))))
				j = gameStates.gameplay.nReactorCount;
			rStatP = gameData.reactor.states + j;
			if (gameStates.gameplay.nLastReactor < j)
				gameStates.gameplay.nLastReactor = j;
			if (bNew)
				rStatP->nDeadObj = -1;
			rStatP->xLastVisCheckTime = 0;
			if (rStatP->nDeadObj < 0) {
				nGuns = gameData.reactor.props [objP->id].nGuns;
				for (j = 0; j < nGuns; j++)
					CalcReactorGunPoint (rStatP->vGunPos + j, rStatP->vGunDir + j, objP, j);
				gameData.reactor.bPresent = 1;
				rStatP->nObject = i;
				if (bNew) {
					objP->shields = ReactorStrength ();
					//	Say the control center has not yet been hit.
					rStatP->bHit = 0;
					rStatP->bSeenPlayer = 0;
					rStatP->nNextFireTime = 0;
					}
#if CHECK_REACTOR_BOSSFLAG
				if (ROBOTINFO (objP->id).bossFlag)
					extraGameInfo [0].nBossCount++;
#endif
				gameStates.gameplay.nLastReactor = gameStates.gameplay.nReactorCount;
				gameStates.gameplay.nReactorCount++;
				}
			}
		}

	if (IS_BOSS (objP)) {
		extraGameInfo [0].nBossCount++;
		//InitBossData (extraGameInfo [0].nBossCount - 1, OBJ_IDX (objP));
		if (BOSS_COUNT > 1) {
#if TRACE
			con_printf (1, "Warning: Two or more bosses including %i and %i\n", i, nBossObj);
#endif
			}			
		else
			nBossObj = i;
		}
	}

#ifdef _DEBUG
if (!(BOSS_COUNT || gameStates.gameplay.nReactorCount)) {
#if TRACE
	con_printf (1, "Warning: No control center.\n");
#endif
	return;
	}
#endif

if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)
	gameData.reactor.bDisabled = 0;
else if (BOSS_COUNT) {
	for (j = 0; j < gameStates.gameplay.nReactorCount; j++) {
#if CHECK_REACTOR_BOSSFLAG
		if (ROBOTINFO (OBJECTS [gameData.reactor.states [j].nObject].id).bossFlag)
#endif
			{
			BashToShield (gameData.reactor.states [j].nObject, "reactor");
			if (j < --gameStates.gameplay.nReactorCount)
				gameData.reactor.states [j] = gameData.reactor.states [gameStates.gameplay.nReactorCount];
			}
		}
	gameData.reactor.bPresent = 0;
	gameData.reactor.bDisabled = 1;
	extraGameInfo [0].nBossCount = 1;
	}
}

//------------------------------------------------------------------------------

void SpecialReactorStuff (void)
{
#if TRACE
con_printf (CONDBG, "Mucking with reactor countdown time.\n");
#endif
if (gameData.reactor.bDestroyed) {
	gameData.reactor.countdown.nTimer += i2f (gameStates.app.nBaseCtrlCenExplTime + (NDL-1-gameStates.app.nDifficultyLevel)*gameStates.app.nBaseCtrlCenExplTime/ (NDL-1));
	gameData.reactor.countdown.nTotalTime = f2i (gameData.reactor.countdown.nTimer)+2;	//	Will prevent "Self destruct sequence activated" message from replaying.
	}
}

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
//------------------------------------------------------------------------------
/*
 * reads n reactor structs from a CFILE
 */
extern int ReactorReadN (tReactorProps *r, int n, CFILE *fp)
{
	int i, j;

for (i = 0; i < n; i++) {
	r[i].nModel = CFReadInt (fp);
	r[i].nGuns = CFReadInt (fp);
	for (j = 0; j < MAX_CONTROLCEN_GUNS; j++)
		CFReadVector (r[i].gunPoints + j, fp);
	for (j = 0; j < MAX_CONTROLCEN_GUNS; j++)
		CFReadVector (r[i].gun_dirs + j, fp);
	}
return i;
}

//------------------------------------------------------------------------------
/*
 * reads a tReactorTriggers structure from a CFILE
 */
extern int ControlCenterTriggersReadN (tReactorTriggers *cct, int n, CFILE *fp)
{
	int i, j;

for (i = 0; i < n; i++) {
	cct->nLinks = CFReadShort (fp);
	for (j = 0; j < MAX_CONTROLCEN_LINKS; j++)
		cct->nSegment [j] = CFReadShort (fp);
	for (j = 0; j < MAX_CONTROLCEN_LINKS; j++)
		cct->nSide [j] = CFReadShort (fp);
	}
return i;
}
#endif

//------------------------------------------------------------------------------
