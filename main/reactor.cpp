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

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "inferno.h"
#include "error.h"
#include "endlevel.h"
#include "network.h"
#include "switch.h"
#include "cheats.h"
#include "gamesave.h"

//@@CFixVector controlcen_gun_points[MAX_CONTROLCEN_GUNS];
//@@CFixVector controlcen_gun_dirs[MAX_CONTROLCEN_GUNS];

void DoCountdownFrame ();

//	-----------------------------------------------------------------------------
//return the position & orientation of a gun on the control center CObject
void CalcReactorGunPoint (CFixVector *vGunPoint, CFixVector *vGunDir, CObject *objP, int nGun)
{
	tReactorProps	*props;
	CFixMatrix		*viewP = ObjectView (objP);

Assert (objP->info.nType == OBJ_REACTOR);
Assert (objP->info.renderType == RT_POLYOBJ);
props = &gameData.reactor.props [objP->info.nId];
//instance gun position & orientation
*vGunPoint = *viewP * props->gunPoints[nGun];
*vGunPoint += objP->info.position.vPos;
*vGunDir = *viewP * props->gunDirs[nGun];
}

//	-----------------------------------------------------------------------------
//	Look at control center guns, find best one to fire at *objP.
//	Return best gun number (one whose direction dotted with vector to CPlayerData is largest).
//	If best gun has negative dot, return -1, meaning no gun is good.
int CalcBestReactorGun (int nGunCount, CFixVector *vGunPos, CFixVector *vGunDir, CFixVector *vObjPos)
{
	int	i;
	fix	xBestDot;
	int	nBestGun;

xBestDot = -F1_0*2;
nBestGun = -1;

for (i = 0; i < nGunCount; i++) {
	fix			dot;
	CFixVector	vGun;

	vGun = *vObjPos - vGunPos[i];
	CFixVector::Normalize(vGun);
	dot = CFixVector::Dot (vGunDir[i], vGun);
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
			 (OBJECTS [rStatP->nDeadObj].info.nType == OBJ_REACTOR) &&
			 (gameData.reactor.countdown.nSecsLeft > 0))
		if (d_rand () < gameData.time.xFrame * 4)
			CreateSmallFireballOnObject (OBJECTS + rStatP->nDeadObj, F1_0, 1);
		}
	}
if (!gameStates.app.bEndLevelSequence)
	DoCountdownFrame ();
}

//	-----------------------------------------------------------------------------

#define COUNTDOWN_VOICE_TIME F2X (12.75)

void DoCountdownFrame (void)
{
	fix	oldTime;
	int	fc, h, xScale;

	static fix cdtFrameTime;

if (!gameData.reactor.bDestroyed) {
	cdtFrameTime = 0;
	return;
	}
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

//	Control center destroyed, rock the CPlayerData's ship.
fc = gameData.reactor.countdown.nSecsLeft;
if (fc > 16)
	fc = 16;
//	At Trainee, decrease rocking of ship by 4x.
xScale = 1;
if (gameStates.app.nDifficultyLevel == 0)
	xScale = 4;
h = 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32;
gameData.objs.consoleP->mType.physInfo.rotVel[X] += (FixMul (d_rand () - 16384, h)) / xScale;
gameData.objs.consoleP->mType.physInfo.rotVel[Z] += (FixMul (d_rand () - 16384, h)) / xScale;
//	Hook in the rumble sound effect here.
oldTime = gameData.reactor.countdown.nTimer;
if (!TimeStopped ())
	gameData.reactor.countdown.nTimer -= cdtFrameTime;
if (IsMultiGame &&  NetworkIAmMaster ())
	MultiSendCountdown ();
cdtFrameTime = 0;
gameData.reactor.countdown.nSecsLeft = X2I (gameData.reactor.countdown.nTimer + F1_0 * 7 / 8);
if ((oldTime > COUNTDOWN_VOICE_TIME) && (gameData.reactor.countdown.nTimer <= COUNTDOWN_VOICE_TIME)) 
	DigiPlaySample (SOUND_COUNTDOWN_13_SECS, F3_0);
if (X2I (oldTime + F1_0 * 7 / 8) != gameData.reactor.countdown.nSecsLeft) {
	if ((gameData.reactor.countdown.nSecsLeft >= 0) && (gameData.reactor.countdown.nSecsLeft < 10))
		DigiPlaySample ((short) (SOUND_COUNTDOWN_0_SECS + gameData.reactor.countdown.nSecsLeft), F3_0);
	if (gameData.reactor.countdown.nSecsLeft == gameData.reactor.countdown.nTotalTime - 1)
		DigiPlaySample (SOUND_COUNTDOWN_29_SECS, F3_0);
	}					
if (gameData.reactor.countdown.nTimer > 0) {
	fix size = (I2X (gameData.reactor.countdown.nTotalTime) - gameData.reactor.countdown.nTimer) / F2X (0.65);
	fix oldSize = (I2X (gameData.reactor.countdown.nTotalTime) - oldTime) / F2X (0.65);
	if ((size != oldSize) && (gameData.reactor.countdown.nSecsLeft < gameData.reactor.countdown.nTotalTime - 5))	// Every 2 seconds!
		DigiPlaySample (SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0);
	}
else {
	int flashValue = X2I (-gameData.reactor.countdown.nTimer * (64 / 4));	// 4 seconds to total whiteness
	if (oldTime > 0)
		DigiPlaySample (SOUND_MINE_BLEW_UP, F1_0);
	paletteManager.SetEffect (flashValue, flashValue, flashValue);
	if (paletteManager.BlueEffect () > 64) {
		CCanvas::SetCurrent (NULL);
		CCanvas::Current ()->Clear (RGBA_PAL2 (31,31,31));	//make screen all white to match palette effect
		ResetCockpit ();		//force cockpit redraw next time
		paletteManager.ResetEffect ();	//restore palette for death message
		DoPlayerDead ();		//kill_player ();
		}																			
	}
}

//	-----------------------------------------------------------------------------

void InitCountdown (CTrigger *trigP, int bReactorDestroyed, int nTimer)
{
if (trigP && (trigP->time > 0))
	gameData.reactor.countdown.nTotalTime = trigP->time;
else if (gameStates.app.nBaseCtrlCenExplTime != DEFAULT_CONTROL_CENTER_EXPLOSION_TIME)
	gameData.reactor.countdown.nTotalTime = 
		gameStates.app.nBaseCtrlCenExplTime + gameStates.app.nBaseCtrlCenExplTime * (NDL - gameStates.app.nDifficultyLevel - 1) / 2;
else
	gameData.reactor.countdown.nTotalTime = nAlanPavlishReactorTimes [gameStates.app.bD1Mission][gameStates.app.nDifficultyLevel];
gameData.reactor.countdown.nTimer = (nTimer < 0) ? I2X (gameData.reactor.countdown.nTotalTime) : (nTimer ? nTimer : I2X (1));
if (bReactorDestroyed)
	gameData.reactor.bDestroyed = 1;
}

//	-----------------------------------------------------------------------------
//	Called when control center gets destroyed.
//	This code is common to whether control center is implicitly imbedded in a boss,
//	or is an CObject of its own.
//	if objP == NULL that means the boss was the control center and don't set gameData.reactor.nDeadObj
void DoReactorDestroyedStuff (CObject *objP)
{
	int		i, bFinalCountdown, bReactor = objP && (objP->info.nType == OBJ_REACTOR);
	CTrigger	*trigP = NULL;

if ((gameData.app.nGameMode & GM_MULTI_ROBOTS) && gameData.reactor.bDestroyed)
   return; // Don't allow resetting if control center and boss on same level
// Must toggle walls whether it is a boss or control center.
if ((!objP || (objP->info.nType == OBJ_ROBOT)) && gameStates.gameplay.bKillBossCheat)
	return;
// And start the countdown stuff.
bFinalCountdown = !(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses && extraGameInfo [0].nBossCount);
if (bFinalCountdown ||
	 (gameStates.app.bD2XLevel && bReactor && (trigP = FindObjTrigger (objP->Index (), TT_COUNTDOWN, -1)))) {
//	If a secret level, delete secret.sgc to indicate that we can't return to our secret level.
	if (bFinalCountdown) {
		if (extraGameInfo [0].nBossCount)
			KillAllBossRobots (0);
		for (i = 0; i < gameData.reactor.triggers.nLinks; i++)
			SEGMENTS [gameData.reactor.triggers.segments [i]].ToggleWall (gameData.reactor.triggers.sides [i]);
		if (gameData.missions.nCurrentLevel < 0)
			CFile::Delete ("secret.sgc", gameFolders.szSaveDir);
		}
	InitCountdown (trigP, bFinalCountdown, -1);
	}
if (bReactor) {
	ExecObjTriggers (objP->Index (), 0);
	if (0 <= (i = FindReactor (objP))) {
		gameData.reactor.states [i].nDeadObj = objP->Index ();
		gameStates.gameplay.nReactorCount--;
		}
	}
}

//	-----------------------------------------------------------------------------

int FindReactor (CObject *objP)
{
	int	i, nObject = objP->Index ();

for (i = 0; i <= gameStates.gameplay.nLastReactor; i++)
	if (gameData.reactor.states [i].nObject == nObject)
		return i;
return -1;
}

//	-----------------------------------------------------------------------------

void RemoveReactor (CObject *objP)
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
void DoReactorFrame (CObject *objP)
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
#if DBG
if (!gameStates.app.cheats.bRobotsFiring || (gameStates.app.bGameSuspended & SUSP_ROBOTS))
	return;
#else
if (!gameStates.app.cheats.bRobotsFiring)
	return;
#endif

if (!(rStatP->bHit || rStatP->bSeenPlayer)) {
	if (gameStates.app.tick40fps.bTick) {		//	Do ever so often...
		CFixVector	vecToPlayer;
		fix			xDistToPlayer;
		int			i;
		CSegment		*segP = SEGMENTS + objP->info.nSegment;

		// This is a hack.  Since the control center is not processed by
		// ai_do_frame, it doesn't know how to deal with cloaked dudes.  It
		// seems to work in single-CPlayerData mode because it is actually using
		// the value of Believed_player_position that was set by the last
		// person to go through ai_do_frame.  But since a no-robots game
		// never goes through ai_do_frame, I'm making it so the control
		// center can spot cloaked dudes.

		if (IsMultiGame)
			gameData.ai.vBelievedPlayerPos = OBJPOS (OBJECTS + LOCALPLAYER.nObject)->vPos;

		//	Hack for special control centers which are isolated and not reachable because the
		//	real control center is inside the boss.
		for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
			if (IS_CHILD (segP->m_children [i]))
				break;
		if (i == MAX_SIDES_PER_SEGMENT)
			return;

		vecToPlayer = OBJPOS (gameData.objs.consoleP)->vPos - objP->info.position.vPos;
		xDistToPlayer = CFixVector::Normalize(vecToPlayer);
		if (xDistToPlayer < F1_0 * 200) {
			rStatP->bSeenPlayer = ObjectCanSeePlayer (objP, &objP->info.position.vPos, 0, &vecToPlayer);
			rStatP->nNextFireTime = 0;
			}
		}		
	return;
	}

//	Periodically, make the reactor fall asleep if CPlayerData not visible.
if (rStatP->bHit || rStatP->bSeenPlayer) {
	if ((rStatP->xLastVisCheckTime + F1_0 * 5 < gameData.time.xGame) || 
		 (rStatP->xLastVisCheckTime > gameData.time.xGame)) {
		CFixVector	vecToPlayer;
		fix			xDistToPlayer;

		vecToPlayer = gameData.objs.consoleP->info.position.vPos - objP->info.position.vPos;
		xDistToPlayer = CFixVector::Normalize (vecToPlayer);
		rStatP->xLastVisCheckTime = gameData.time.xGame;
		if (xDistToPlayer < F1_0 * 120) {
			rStatP->bSeenPlayer = ObjectCanSeePlayer (objP, &objP->info.position.vPos, 0, &vecToPlayer);
			if (!rStatP->bSeenPlayer)
				rStatP->bHit = 0;
			}
		}
	}

if ((rStatP->nNextFireTime < 0) && 
	 !(gameStates.app.bPlayerIsDead && (gameData.time.xGame > gameStates.app.nPlayerTimeOfDeath + F1_0 * 2))) {
	nBestGun = CalcBestReactorGun (gameData.reactor.props [objP->info.nId].nGuns, rStatP->vGunPos, rStatP->vGunDir, 
											 (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) ? &gameData.ai.vBelievedPlayerPos : &gameData.objs.consoleP->info.position.vPos);
	if (nBestGun != -1) {
		int			nRandProb, count;
		CFixVector	vecToGoal;
		fix			xDistToPlayer;
		fix			xDeltaFireTime;

		if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
			vecToGoal = gameData.ai.vBelievedPlayerPos - rStatP->vGunPos[nBestGun];
			xDistToPlayer = CFixVector::Normalize(vecToGoal);
			} 
		else {
			vecToGoal = gameData.objs.consoleP->info.position.vPos - rStatP->vGunPos [nBestGun];
			xDistToPlayer = CFixVector::Normalize(vecToGoal);
			}
		if (xDistToPlayer > F1_0 * 300) {
			rStatP->bHit = 0;
			rStatP->bSeenPlayer = 0;
			return;
			}
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendCtrlcenFire (&vecToGoal, nBestGun, objP->Index ());
		CreateNewLaserEasy (&vecToGoal, &rStatP->vGunPos [nBestGun], objP->Index (), CONTROLCEN_WEAPON_NUM, 1);
		//	some of time, based on level, fire another thing, not directly at CPlayerData, so it might hit him if he's constantly moving.
		nRandProb = F1_0 / (abs (gameData.missions.nCurrentLevel) / 4 + 2);
		count = 0;
		while ((d_rand () > nRandProb) && (count < 4)) {
			CFixVector	vRand;

			vRand = CFixVector::Random();
			vecToGoal += vRand * (F1_0/6);
			CFixVector::Normalize(vecToGoal);
			if (IsMultiGame)
				MultiSendCtrlcenFire (&vecToGoal, nBestGun, objP->Index ());
			CreateNewLaserEasy (&vecToGoal, &rStatP->vGunPos [nBestGun], objP->Index (), CONTROLCEN_WEAPON_NUM, 0);
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
return I2X (gameData.reactor.nStrength);
}

//	-----------------------------------------------------------------------------
//	This must be called at the start of each level.
//	If this level contains a boss and mode != multiplayer, don't do control center stuff.  (Ghost out control center CObject.)
//	If this level contains a boss and mode == multiplayer, do control center stuff.
void InitReactorForLevel (int bRestore)
{
	int		i, j = 0, nGuns, bNew;
	CObject	*objP;
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
FORALL_ACTOR_OBJS (objP, i) {
	if (objP->info.nType == OBJ_REACTOR) {
		if (gameStates.gameplay.nReactorCount && !(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)) {
#if TRACE
			console.printf (1, "Warning: Two or more control centers including %i and %i\n", 
							gameData.reactor.states [0].nObject, i);
#endif
			}			
		//else 
			{
			//	Compute all gun positions.
			if ((bNew = (!bRestore || (0 > (j = FindReactor (objP))))))
				j = gameStates.gameplay.nReactorCount;
			rStatP = gameData.reactor.states + j;
			if (gameStates.gameplay.nLastReactor < j)
				gameStates.gameplay.nLastReactor = j;
			if (bNew)
				rStatP->nDeadObj = -1;
			rStatP->xLastVisCheckTime = 0;
			if (rStatP->nDeadObj < 0) {
				nGuns = gameData.reactor.props [objP->info.nId].nGuns;
				for (j = 0; j < nGuns; j++)
					CalcReactorGunPoint (rStatP->vGunPos + j, rStatP->vGunDir + j, objP, j);
				gameData.reactor.bPresent = 1;
				rStatP->nObject = objP->Index ();
				if (bNew) {
					objP->info.xShields = ReactorStrength ();
					//	Say the control center has not yet been hit.
					rStatP->bHit = 0;
					rStatP->bSeenPlayer = 0;
					rStatP->nNextFireTime = 0;
					}
				extraGameInfo [0].nBossCount++;
				gameStates.gameplay.nLastReactor = gameStates.gameplay.nReactorCount;
				gameStates.gameplay.nReactorCount++;
				}
			}
		}

	if (IS_BOSS (objP)) {
		extraGameInfo [0].nBossCount++;
		//InitBossData (extraGameInfo [0].nBossCount - 1, objP->Index ());
		if (BOSS_COUNT > 1) {
#if TRACE
			console.printf (1, "Warning: Two or more bosses including %i and %i\n", objP->Index (), nBossObj);
#endif
			}			
		else
			nBossObj = objP->Index ();
		}
	}

#if DBG
if ((BOSS_COUNT <= 0) && !gameStates.gameplay.nReactorCount) {
#if TRACE
	console.printf (1, "Warning: No control center.\n");
#endif
	return;
	}
#endif

if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)
	gameData.reactor.bDisabled = 0;
else if (BOSS_COUNT > 0) {
	for (j = 0; j < gameStates.gameplay.nReactorCount; j++) {
		BashToShield (gameData.reactor.states [j].nObject, "reactor");
		extraGameInfo [0].nBossCount--;
		if (j < --gameStates.gameplay.nReactorCount)
			gameData.reactor.states [j] = gameData.reactor.states [gameStates.gameplay.nReactorCount];
		}
	gameData.reactor.bPresent = 0;
	gameData.reactor.bDisabled = 1;
	//extraGameInfo [0].nBossCount = 1;
	}
}

//------------------------------------------------------------------------------

void SpecialReactorStuff (void)
{
#if TRACE
console.printf (CON_DBG, "Mucking with reactor countdown time.\n");
#endif
if (gameData.reactor.bDestroyed) {
	gameData.reactor.countdown.nTimer += 
		I2X (gameStates.app.nBaseCtrlCenExplTime + (NDL - 1 - gameStates.app.nDifficultyLevel) * gameStates.app.nBaseCtrlCenExplTime / (NDL-1));
	gameData.reactor.countdown.nTotalTime = X2I (gameData.reactor.countdown.nTimer) + 2;	//	Will prevent "Self destruct sequence activated" message from replaying.
	}
}

//------------------------------------------------------------------------------

void ReadReactor (tReactorProps& reactor, CFile& cf)
{
	int i;

reactor.nModel = cf.ReadInt ();
reactor.nGuns = cf.ReadInt ();
for (i = 0; i < MAX_CONTROLCEN_GUNS; i++)
	cf.ReadVector (reactor.gunPoints [i]);
for (i = 0; i < MAX_CONTROLCEN_GUNS; i++)
	cf.ReadVector (reactor.gunDirs [i]);
}

//------------------------------------------------------------------------------
/*
 * reads n reactor structs from a CFile
 */
extern int ReadReactors (CFile& cf)
{
	int i;

for (i = 0; i < gameData.reactor.nReactors; i++)
	ReadReactor (gameData.reactor.props [i], cf); 
return gameData.reactor.nReactors;
}

//------------------------------------------------------------------------------

INT ReadReactorTriggers (CFile& cf)
{
	int i, j;

for (i = 0; i < gameFileInfo.control.count; i++) {
	gameData.reactor.triggers.nLinks = cf.ReadShort ();
	for (i = 0; i < MAX_CONTROLCEN_LINKS; i++)
		gameData.reactor.triggers.segments [i] = cf.ReadShort ();
	for (i = 0; i < MAX_CONTROLCEN_LINKS; i++)
		gameData.reactor.triggers.sides [i] = cf.ReadShort ();
	}
return gameFileInfo.control.count;
}

//------------------------------------------------------------------------------
