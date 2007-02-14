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
#include "cntrlcen.h"
#include "game.h"
#include "laser.h"
#include "gameseq.h"
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

int	nReactorGuns;

vmsVector	Gun_pos[MAX_CONTROLCEN_GUNS], Gun_dir[MAX_CONTROLCEN_GUNS];

void DoCountdownFrame ();

//	-----------------------------------------------------------------------------
//return the position & orientation of a gun on the control center tObject
void CalcReactorGunPoint (vmsVector *gun_point,vmsVector *vGunDir,tObject *objP,int gun_num)
{
	reactor *reactor;
	vmsMatrix m;

Assert (objP->nType == OBJ_CNTRLCEN);
Assert (objP->renderType==RT_POLYOBJ);
reactor = &gameData.reactor.reactors[objP->id];
Assert (gun_num < reactor->nGuns);
//instance gun position & orientation
VmCopyTransposeMatrix (&m,&objP->position.mOrient);
VmVecRotate (gun_point,&reactor->gunPoints[gun_num],&m);
VmVecInc (gun_point,&objP->position.vPos);
VmVecRotate (vGunDir,&reactor->gun_dirs[gun_num],&m);
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
	vmsVector	gun_vec;

	VmVecSub (&gun_vec, vObjPos, &vGunPos[i]);
	VmVecNormalizeQuick (&gun_vec);
	dot = VmVecDot (vGunDir + i, &gun_vec);
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
if ((gameData.reactor.nDeadObj != -1) && 
	 (gameData.objs.objects [gameData.reactor.nDeadObj].nType == OBJ_CNTRLCEN) &&
	 (gameData.reactor.countdown.nSecsLeft > 0))
	if (d_rand () < gameData.time.xFrame * 4)
		CreateSmallFireballOnObject (&gameData.objs.objects[gameData.reactor.nDeadObj], F1_0, 1);
if (gameData.reactor.bDestroyed && !gameStates.app.bEndLevelSequence)
	DoCountdownFrame ();
}

//	-----------------------------------------------------------------------------

#define COUNTDOWN_VOICE_TIME fl2f (12.75)

void DoCountdownFrame ()
{
	fix	oldTime;
	int	fc, h, div_scale;
	static fix cdtFrameTime;

if (!gameData.reactor.bDestroyed) {
	cdtFrameTime = 0;
	return;
	}
cdtFrameTime += gameData.time.xRealFrame;
if (gameStates.limitFPS.bCountDown && !gameStates.app.b40fpsTick)
	return;
if (!IS_D2_OEM && !IS_MAC_SHARE && !IS_SHAREWARE) {  // get countdown in OEM and SHAREWARE only
	// On last level, we don't want a countdown.
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) && 
		 (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)) {
		if (!(gameData.app.nGameMode & GM_MULTI))
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
div_scale = 1;
if (gameStates.app.nDifficultyLevel == 0)
	div_scale = 4;
h = 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32;
gameData.objs.console->mType.physInfo.rotVel.p.x += (FixMul (d_rand () - 16384, h)) / div_scale;
gameData.objs.console->mType.physInfo.rotVel.p.z += (FixMul (d_rand () - 16384, h)) / div_scale;
//	Hook in the rumble sound effect here.
oldTime = gameData.reactor.countdown.nTimer;
if (!TimeStopped ())
	gameData.reactor.countdown.nTimer -= cdtFrameTime;
cdtFrameTime = 0;
gameData.reactor.countdown.nSecsLeft = f2i (gameData.reactor.countdown.nTimer + F1_0 * 7 / 8);
if ((oldTime > COUNTDOWN_VOICE_TIME) && (gameData.reactor.countdown.nTimer <= COUNTDOWN_VOICE_TIME))	{
	DigiPlaySample (SOUND_COUNTDOWN_13_SECS, F3_0);
	}
if (f2i (oldTime + F1_0*7/8) != gameData.reactor.countdown.nSecsLeft) {
	if ((gameData.reactor.countdown.nSecsLeft >= 0) && (gameData.reactor.countdown.nSecsLeft < 10))
		DigiPlaySample ((short) (SOUND_COUNTDOWN_0_SECS + gameData.reactor.countdown.nSecsLeft), F3_0);
	if (gameData.reactor.countdown.nSecsLeft == gameData.reactor.countdown.nTotalTime - 1)
		DigiPlaySample (SOUND_COUNTDOWN_29_SECS, F3_0);
	}						
if (gameData.reactor.countdown.nTimer > 0) {
	fix size,old_size;
	size = (i2f (gameData.reactor.countdown.nTotalTime) - gameData.reactor.countdown.nTimer) / fl2f (0.65);
	old_size = (i2f (gameData.reactor.countdown.nTotalTime) - oldTime) / fl2f (0.65);
	if (size != old_size && (gameData.reactor.countdown.nSecsLeft < (gameData.reactor.countdown.nTotalTime-5)))	// Every 2 seconds!
		DigiPlaySample (SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0);
	}
else {
	int flashValue = f2i (-gameData.reactor.countdown.nTimer * (64 / 4));	// 4 seconds to total whiteness
	if (oldTime > 0)
		DigiPlaySample (SOUND_MINE_BLEW_UP, F1_0);
	PALETTE_FLASH_SET (flashValue,flashValue,flashValue);
	if (gameStates.ogl.palAdd.blue > 64) {
		WINDOS (
			DDGrSetCurrentCanvas (NULL),
			GrSetCurrentCanvas (NULL)
			);
		WINDOS (
			dd_gr_clear_canvas (RGBA_PAL2 (31,31,31)),
			GrClearCanvas (RGBA_PAL2 (31,31,31))
			);														//make screen all white to match palette effect
		ResetCockpit ();								//force cockpit redraw next time
		ResetPaletteAdd ();							//restore palette for death message
		//controlcen->MaxCapacity = gameData.matCens.xFuelMaxAmount;
		//gauge_message ("Control Center Reset");
		DoPlayerDead ();		//kill_player ();
		}																				
	}
}

//	-----------------------------------------------------------------------------
//	Called when control center gets destroyed.
//	This code is common to whether control center is implicitly imbedded in a boss,
//	or is an tObject of its own.
//	if objP == NULL that means the boss was the control center and don't set gameData.reactor.nDeadObj
void DoReactorDestroyedStuff (tObject *objP)
{
	int	i;

if ((gameData.app.nGameMode & GM_MULTI_ROBOTS) && gameData.reactor.bDestroyed)
   return; // Don't allow resetting if control center and boss on same level
if (gameStates.gameplay.bKillBossCheat)
	return;
// Must toggle walls whether it is a boss or control center.
for (i = 0; i < gameData.reactor.triggers.nLinks; i++)
	WallToggle (gameData.segs.segments + gameData.reactor.triggers.nSegment [i], gameData.reactor.triggers.nSide [i]);
ExecObjTriggers (OBJ_IDX (objP));
// And start the countdown stuff.
if (!(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses && extraGameInfo [0].nBossCount))
	gameData.reactor.bDestroyed = 1;
//	If a secret level, delete secret.sgc to indicate that we can't return to our secret level.
if (gameData.missions.nCurrentLevel < 0)
	CFDelete ("secret.sgc", gameFolders.szSaveDir);
if (gameStates.app.nBaseCtrlCenExplTime != DEFAULT_CONTROL_CENTER_EXPLOSION_TIME)
	gameData.reactor.countdown.nTotalTime = gameStates.app.nBaseCtrlCenExplTime + gameStates.app.nBaseCtrlCenExplTime * (NDL-gameStates.app.nDifficultyLevel-1)/2;
else
	gameData.reactor.countdown.nTotalTime = nAlanPavlishReactorTimes [gameStates.app.bD1Mission][gameStates.app.nDifficultyLevel];
gameData.reactor.countdown.nTimer = i2f (gameData.reactor.countdown.nTotalTime);
if (!(gameData.reactor.bPresent && objP)) {
	//Assert (objP == NULL);
	return;
	}
gameData.reactor.nDeadObj = OBJ_IDX (objP);
}

int	LastTime_cc_vis_check = 0;

//	-----------------------------------------------------------------------------
//do whatever this thing does in a frame
void DoReactorFrame (tObject *objP)
{
	int	nBestGun;

	//	If a boss level, then gameData.reactor.bPresent will be 0.
if (!gameData.reactor.bPresent)
	return;

#ifdef _DEBUG
if (!gameStates.app.cheats.bRobotsFiring || (gameStates.app.bGameSuspended & SUSP_ROBOTS))
	return;
#else
if (!gameStates.app.cheats.bRobotsFiring)
	return;
#endif

if (!(gameData.reactor.bHit || gameData.reactor.bSeenPlayer)) {
	if (!(gameData.app.nFrameCount % 8)) {		//	Do every so often...
		vmsVector	vecToPlayer;
		fix			xDistToPlayer;
		int			i;
		tSegment		*segP = gameData.segs.segments + objP->nSegment;

		// This is a hack.  Since the control center is not processed by
		// ai_do_frame, it doesn't know to deal with cloaked dudes.  It
		// seems to work in single-tPlayer mode because it is actually using
		// the value of Believed_player_position that was set by the last
		// person to go through ai_do_frame.  But since a no-robots game
		// never goes through ai_do_frame, I'm making it so the control
		// center can spot cloaked dudes.

		if (gameData.app.nGameMode & GM_MULTI)
			gameData.ai.vBelievedPlayerPos = gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].position.vPos;

		//	Hack for special control centers which are isolated and not reachable because the
		//	real control center is inside the boss.
		for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
			if (IS_CHILD (segP->children[i]))
				break;
		if (i == MAX_SIDES_PER_SEGMENT)
			return;

		VmVecSub (&vecToPlayer, &gameData.objs.console->position.vPos, &objP->position.vPos);
		xDistToPlayer = VmVecNormalizeQuick (&vecToPlayer);
		if (xDistToPlayer < F1_0*200) {
			gameData.reactor.bSeenPlayer = ObjectCanSeePlayer (objP, &objP->position.vPos, 0, &vecToPlayer);
			gameData.reactor.nNextFireTime = 0;
			}
		}			
	return;
	}

//	Periodically, make the reactor fall asleep if tPlayer not visible.
if (gameData.reactor.bHit || gameData.reactor.bSeenPlayer) {
	if ((LastTime_cc_vis_check + F1_0*5 < gameData.time.xGame) || (LastTime_cc_vis_check > gameData.time.xGame)) {
		vmsVector	vecToPlayer;
		fix			xDistToPlayer;

		VmVecSub (&vecToPlayer, &gameData.objs.console->position.vPos, &objP->position.vPos);
		xDistToPlayer = VmVecNormalizeQuick (&vecToPlayer);
		LastTime_cc_vis_check = gameData.time.xGame;
		if (xDistToPlayer < F1_0*120) {
			gameData.reactor.bSeenPlayer = ObjectCanSeePlayer (objP, &objP->position.vPos, 0, &vecToPlayer);
			if (!gameData.reactor.bSeenPlayer)
				gameData.reactor.bHit = 0;
			}
		}
	}

if ((gameData.reactor.nNextFireTime < 0) && !(gameStates.app.bPlayerIsDead && (gameData.time.xGame > gameStates.app.nPlayerTimeOfDeath+F1_0*2))) {
	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)
		nBestGun = CalcBestReactorGun (nReactorGuns, Gun_pos, Gun_dir, &gameData.ai.vBelievedPlayerPos);
	else
		nBestGun = CalcBestReactorGun (nReactorGuns, Gun_pos, Gun_dir, &gameData.objs.console->position.vPos);

	if (nBestGun != -1) {
		int			nRandProb, count;
		vmsVector	vecToGoal;
		fix			xDistToPlayer;
		fix			xDeltaFireTime;

		if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
			VmVecSub (&vecToGoal, &gameData.ai.vBelievedPlayerPos, &Gun_pos[nBestGun]);
			xDistToPlayer = VmVecNormalizeQuick (&vecToGoal);
			} 
		else {
			VmVecSub (&vecToGoal, &gameData.objs.console->position.vPos, &Gun_pos[nBestGun]);
			xDistToPlayer = VmVecNormalizeQuick (&vecToGoal);
			}
		if (xDistToPlayer > F1_0*300) {
			gameData.reactor.bHit = 0;
			gameData.reactor.bSeenPlayer = 0;
			return;
			}
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendCtrlcenFire (&vecToGoal, nBestGun, OBJ_IDX (objP));	
		CreateNewLaserEasy (&vecToGoal, &Gun_pos[nBestGun], OBJ_IDX (objP), CONTROLCEN_WEAPON_NUM, 1);
		//	some of time, based on level, fire another thing, not directly at tPlayer, so it might hit him if he's constantly moving.
		nRandProb = F1_0/ (abs (gameData.missions.nCurrentLevel)/4+2);
		count = 0;
		while ((d_rand () > nRandProb) && (count < 4)) {
			vmsVector	vRand;

			MakeRandomVector (&vRand);
			VmVecScaleInc (&vecToGoal, &vRand, F1_0/6);
			VmVecNormalizeQuick (&vecToGoal);
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendCtrlcenFire (&vecToGoal, nBestGun, OBJ_IDX (objP));
			CreateNewLaserEasy (&vecToGoal, &Gun_pos[nBestGun], OBJ_IDX (objP), CONTROLCEN_WEAPON_NUM, 0);
			count++;
			}
		xDeltaFireTime = (NDL - gameStates.app.nDifficultyLevel) * F1_0/4;
		if (gameStates.app.nDifficultyLevel == 0)
			xDeltaFireTime += F1_0/2;
		if (gameData.app.nGameMode & GM_MULTI) // slow down rate of fire in multi tPlayer
			xDeltaFireTime *= 2;
		gameData.reactor.nNextFireTime = xDeltaFireTime;
		}
	} 
else
	gameData.reactor.nNextFireTime -= gameData.time.xFrame;
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
void InitReactorForLevel (void)
{
	int		i;
	tObject	*objP;
	short		nReactorObj = -1, nBossObj = -1;

gameStates.gameplay.bMultiBosses = gameStates.app.bD2XLevel && EGI_FLAG (bMultiBosses, 0, 0, 0);
extraGameInfo [0].nBossCount = 0;
gameStates.gameplay.nReactorCount = 0;
for (i = 0, objP = gameData.objs.objects; i <= gameData.objs.nLastObject; i++, objP++) {
	if (objP->nType == OBJ_CNTRLCEN) {
		if ((nReactorObj != -1) && !(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)) {
#if TRACE
			con_printf (1, "Warning: Two or more control centers including %i and %i\n", i, nReactorObj);
#endif
			}				
		else {
			nReactorObj = i;
			extraGameInfo [0].nBossCount++;
			gameStates.gameplay.nReactorCount++;
			}
		}

	if ((objP->nType == OBJ_ROBOT) && gameData.bots.pInfo [objP->id].bossFlag) {
		extraGameInfo [0].nBossCount++;
		if (nBossObj != -1) {
#if TRACE
			con_printf (1, "Warning: Two or more bosses including %i and %i\n", i, nBossObj);
#endif
			}				
		else
			nBossObj = i;
		}
	}

#ifdef _DEBUG
if (nReactorObj == -1) {
#if TRACE
	con_printf (1, "Warning: No control center.\n");
#endif
	return;
	}
#endif

if (/*EGI_FLAG (bDisableReactor, 0, 0) ||*/
	 ((nBossObj != -1) && (nReactorObj != -1) && 
	  !(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses))) {
	BashToShield (nReactorObj, "reactor");
	gameData.reactor.bPresent = 0;
	gameData.reactor.bDisabled = 1;
	nReactorObj = -1;
	extraGameInfo [0].nBossCount--;
	gameStates.gameplay.nReactorCount--;
	}
else
	gameData.reactor.bDisabled = 0;
if (nReactorObj != -1) {
//	Compute all gun positions.
	objP = gameData.objs.objects + nReactorObj;
	nReactorGuns = gameData.reactor.reactors [objP->id].nGuns;
	for (i = 0; i < nReactorGuns; i++)
		CalcReactorGunPoint (Gun_pos+  i, Gun_dir + i, objP, i);
	gameData.reactor.bPresent = 1;
	objP->shields = ReactorStrength ();
	}
//	Say the control center has not yet been hit.
gameData.reactor.bHit = 0;
gameData.reactor.bSeenPlayer = 0;
gameData.reactor.nNextFireTime = 0;
gameData.reactor.nDeadObj = -1;
}

//------------------------------------------------------------------------------

void SpecialReactorStuff (void)
{
#if TRACE
con_printf (CON_DEBUG, "Mucking with reactor countdown time.\n");
#endif
if (gameData.reactor.bDestroyed) {
	gameData.reactor.countdown.nTimer += i2f (gameStates.app.nBaseCtrlCenExplTime + (NDL-1-gameStates.app.nDifficultyLevel)*gameStates.app.nBaseCtrlCenExplTime/ (NDL-1));
	gameData.reactor.countdown.nTotalTime = f2i (gameData.reactor.countdown.nTimer)+2;	//	Will prevent "Self destruct sequence activated" message from replaying.
	}
}

#ifndef FAST_FILE_IO
//------------------------------------------------------------------------------
/*
 * reads n reactor structs from a CFILE
 */
extern int ReactorReadN (reactor *r, int n, CFILE *fp)
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
