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

#include "descent.h"
#include "error.h"
#include "endlevel.h"
#include "network.h"
#include "trigger.h"
#include "cheats.h"
#include "loadobjects.h"
#include "slowmotion.h"

//@@CFixVector controlcen_gun_points[MAX_CONTROLCEN_GUNS];
//@@CFixVector controlcen_gun_dirs[MAX_CONTROLCEN_GUNS];

void DoCountdownFrame ();

//	-----------------------------------------------------------------------------
//return the position & orientation of a gun on the control center CObject
void CalcReactorGunPoint (CFixVector *vGunPoint, CFixVector *vGunDir, CObject *pObj, int32_t nGun)
{
	tReactorProps	*props;
	CFixMatrix		*pView = pObj->View (0);

Assert (pObj->info.nType == OBJ_REACTOR);
Assert (pObj->info.renderType == RT_POLYOBJ);
props = &gameData.reactor.props [pObj->info.nId];
//instance gun position & orientation
*vGunPoint = *pView * props->gunPoints[nGun];
*vGunPoint += pObj->info.position.vPos;
*vGunDir = *pView * props->gunDirs[nGun];
}

//	-----------------------------------------------------------------------------
//	Look at control center guns, find best one to fire at *pObj.
//	Return best gun number (one whose direction dotted with vector to player is largest).
//	If best gun has negative dot, return -1, meaning no gun is good.
int32_t CalcBestReactorGun (int32_t nGunCount, CFixVector *vGunPos, CFixVector *vGunDir, CFixVector *vObjPos)
{
	int32_t	i;
	fix	xBestDot;
	int32_t	nBestGun;

xBestDot = -I2X (2);
nBestGun = -1;

for (i = 0; i < nGunCount; i++) {
	fix			dot;
	CFixVector	vGun;

	vGun = *vObjPos - vGunPos[i];
	CFixVector::Normalize (vGun);
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
int32_t nAlanPavlishReactorTimes [2][DIFFICULTY_LEVEL_COUNT] = {{90,60,45,35,30},{50,45,40,35,30}};

//	-----------------------------------------------------------------------------
//	Called every frame.  If control center been destroyed, then actually do something.
void DoReactorDeadFrame (void)
{
if (gameStates.gameplay.nReactorCount [0] < gameStates.gameplay.nReactorCount [1]) {
	tReactorStates*	pStates = &gameData.reactor.states [0];

	for (int32_t i = 0; i < gameStates.gameplay.nReactorCount [1]; i++, pStates++) {
		if ((pStates->nDeadObj != -1) && 
			 (OBJECT (pStates->nDeadObj)->info.nType == OBJ_REACTOR) &&
			 (gameData.reactor.countdown.nSecsLeft > 0))
		if (RandShort () < gameData.time.xFrame * 4)
			CreateSmallFireballOnObject (OBJECT (pStates->nDeadObj), I2X (1), 1);
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
	int32_t	fc, h, xScale;

	static fix cdtFrameTime;

if (!gameData.reactor.bDestroyed || gameStates.app.bPlayerIsDead) {
	cdtFrameTime = 0;
	return;
	}
cdtFrameTime += gameData.time.xRealFrame;
if (gameStates.limitFPS.bCountDown && !gameStates.app.tick40fps.bTick)
	return;
if (!IS_D2_OEM && !IS_MAC_SHARE && !IS_SHAREWARE) {  // get countdown in OEM and SHAREWARE only
	// On last level, we don't want a countdown.
	if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) && 
		 (missionManager.nCurrentLevel == missionManager.nLastLevel)) {
		if (!IsMultiGame)
			return;
		if (gameData.app.GameMode (GM_MULTI_ROBOTS))
			return;
		}
	}

//	Control center destroyed, rock the player's ship.
fc = gameData.reactor.countdown.nSecsLeft;
if (fc > 16)
	fc = 16;
//	At Trainee, decrease rocking of ship by 4x.
xScale = 1;
if (gameStates.app.nDifficultyLevel == 0)
	xScale = 4;
h = I2X (3) / 16 + (I2X (16 - fc)) / 32;
gameData.objData.pConsole->mType.physInfo.rotVel.v.coord.x += (FixMul (SRandShort (), h)) / xScale;
gameData.objData.pConsole->mType.physInfo.rotVel.v.coord.z += (FixMul (SRandShort (), h)) / xScale;
//	Hook in the rumble sound effect here.
oldTime = gameData.reactor.countdown.nTimer;
if (!TimeStopped ())
	gameData.reactor.countdown.nTimer -= cdtFrameTime;
if (IsMultiGame &&  IAmGameHost ())
	MultiSendCountdown ();
cdtFrameTime = 0;
gameData.reactor.countdown.nSecsLeft = X2I (gameData.reactor.countdown.nTimer + I2X (7) / 8);
if ((oldTime > COUNTDOWN_VOICE_TIME) && (gameData.reactor.countdown.nTimer <= COUNTDOWN_VOICE_TIME)) 
	audio.PlaySound (SOUND_COUNTDOWN_13_SECS, SOUNDCLASS_GENERIC, I2X (3));
if (X2I (oldTime + I2X (7) / 8) != gameData.reactor.countdown.nSecsLeft) {
	if ((gameData.reactor.countdown.nSecsLeft >= 0) && (gameData.reactor.countdown.nSecsLeft < 10))
		audio.PlaySound ((int16_t) (SOUND_COUNTDOWN_0_SECS + gameData.reactor.countdown.nSecsLeft), SOUNDCLASS_GENERIC, I2X (3));
	if (gameData.reactor.countdown.nSecsLeft == gameData.reactor.countdown.nTotalTime - 1)
		audio.PlaySound (SOUND_COUNTDOWN_29_SECS, SOUNDCLASS_GENERIC, I2X (3));
	}					
if (gameData.reactor.countdown.nTimer > 0) {
	fix size = (I2X (gameData.reactor.countdown.nTotalTime) - gameData.reactor.countdown.nTimer) / F2X (0.65);
	fix oldSize = (I2X (gameData.reactor.countdown.nTotalTime) - oldTime) / F2X (0.65);
	if ((size != oldSize) && (gameData.reactor.countdown.nSecsLeft < gameData.reactor.countdown.nTotalTime - 5))	// Every 2 seconds!
		audio.PlaySound (SOUND_CONTROL_CENTER_WARNING_SIREN, SOUNDCLASS_GENERIC, I2X (3));
	}
else {
	int32_t flashValue = X2I (-gameData.reactor.countdown.nTimer * (64 / 4));	// 4 seconds to total whiteness
	if (oldTime > 0)
		audio.PlaySound (SOUND_MINE_BLEW_UP);
	paletteManager.SetEffect (flashValue, flashValue, flashValue);
	if (paletteManager.BlueEffect () >= 64) {
		gameData.render.frame.Activate ("DoCountdownFrame");
		CCanvas::Current ()->Clear (RGBA_PAL2 (31,31,31));	//make screen all white to match palette effect
		gameData.render.frame.Deactivate ();
		paletteManager.ResetEffect ();	//restore palette for death message
		DoPlayerDead ();		//kill_player ();
		}																			
	}
}

//	-----------------------------------------------------------------------------

void InitCountdown (CTrigger *pTrigger, int32_t bReactorDestroyed, int32_t nTimer)
{
if (pTrigger && (pTrigger->m_info.time > 0))
	gameData.reactor.countdown.nTotalTime = pTrigger->m_info.time [0];
else if (gameStates.app.nBaseCtrlCenExplTime != DEFAULT_CONTROL_CENTER_EXPLOSION_TIME)
	gameData.reactor.countdown.nTotalTime = 
		gameStates.app.nBaseCtrlCenExplTime + gameStates.app.nBaseCtrlCenExplTime * (DIFFICULTY_LEVEL_COUNT - gameStates.app.nDifficultyLevel - 1) / 2;
else
	gameData.reactor.countdown.nTotalTime = nAlanPavlishReactorTimes [gameStates.app.bD1Mission][gameStates.app.nDifficultyLevel];
gameData.reactor.countdown.nTimer = (nTimer < 0) ? I2X (gameData.reactor.countdown.nTotalTime) : (nTimer ? nTimer : I2X (1));
if (bReactorDestroyed) {
	gameData.reactor.bDestroyed = 1;
	SlowMotionOff ();
	}
}

//	-----------------------------------------------------------------------------
//	Called when control center gets destroyed.
//	This code is common to whether control center is implicitly imbedded in a boss,
//	or is an CObject of its own.
//	if pObj == NULL that means the boss was the control center and don't set gameData.reactor.nDeadObj
void DoReactorDestroyedStuff (CObject *pObj)
{
	int32_t		i, bFinalCountdown, bReactor = pObj && (pObj->info.nType == OBJ_REACTOR);
	CTrigger*	pTrigger = NULL;

if (gameData.app.GameMode (GM_MULTI_ROBOTS) && gameData.reactor.bDestroyed)
   return; // Don't allow resetting if control center and boss on same level
// Must toggle walls whether it is a boss or control center.
if ((!pObj || (pObj->info.nType == OBJ_ROBOT)) && gameStates.gameplay.bKillBossCheat)
	return;
// And start the countdown stuff.
bFinalCountdown = !(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses && extraGameInfo [0].nBossCount [0]);
if (bFinalCountdown ||
	 (gameStates.app.bD2XLevel && bReactor && (pTrigger = FindObjTrigger (pObj->Index (), TT_COUNTDOWN, -1)))) {
//	If a secret level, delete secret.sgc to indicate that we can't return to our secret level.
	if (bFinalCountdown) {
		KillAllBossRobots (0);
		for (i = 0; i < gameData.reactor.triggers.m_nLinks; i++)
			SEGMENT (gameData.reactor.triggers.m_segments [i])->ToggleWall (gameData.reactor.triggers.m_sides [i]);
		if (missionManager.nCurrentLevel < 0)
			CFile::Delete ("secret.sgc", gameFolders.user.szSavegames);
		}
	InitCountdown (pTrigger, bFinalCountdown, -1);
	}
if (bReactor) {
	ExecObjTriggers (pObj->Index (), 0);
	if (0 <= (i = FindReactor (pObj))) {
		gameData.reactor.states [i].nDeadObj = pObj->Index ();
		gameStates.gameplay.nReactorCount [0]--;
		}
	}
}

//	-----------------------------------------------------------------------------

int32_t FindReactor (CObject *pObj)
{
	int32_t	i, nObject = pObj->Index ();

for (i = 0; i <= gameStates.gameplay.nLastReactor; i++)
	if (gameData.reactor.states [i].nObject == nObject)
		return i;
return -1;
}

//	-----------------------------------------------------------------------------

void RemoveReactor (CObject *pObj)
{
	int32_t	i = FindReactor (pObj);

if (i < 0)
	return;
if (i < --gameStates.gameplay.nReactorCount [0]) 
	gameData.reactor.states [i] = gameData.reactor.states [gameStates.gameplay.nReactorCount [0]];
memset (gameData.reactor.states + gameStates.gameplay.nReactorCount [0], 0, 
		  sizeof (gameData.reactor.states [gameStates.gameplay.nReactorCount [0]]));
}

//	-----------------------------------------------------------------------------
//do whatever this thing does in a frame
void DoReactorFrame (CObject *pObj)
{
	int32_t			nBestGun, i;
	tReactorStates	*pReactorStates;

	//	If a boss level, then gameData.reactor.bPresent will be 0.
if (!gameData.reactor.bPresent)
	return;
i = FindReactor (pObj);
if (i < 0)
	return;
pReactorStates = gameData.reactor.states + i;
#if DBG
if (gameStates.app.bGameSuspended & SUSP_ROBOTS)
	return;
#endif
if (!pObj->AttacksPlayer ())
	return;

if (!(pReactorStates->bHit || pReactorStates->bSeenPlayer)) {
	if (gameStates.app.tick40fps.bTick) {		//	Do ever so often...
		CFixVector	vecToPlayer;
		fix			xDistToPlayer;
		int32_t			i;
		CSegment		*pSeg = SEGMENT (pObj->info.nSegment);

		// This is a hack.  Since the control center is not processed by
		// ai_do_frame, it doesn't know how to deal with cloaked dudes.  It
		// seems to work in single-player mode because it is actually using
		// the value of Believed_player_position that was set by the last
		// person to go through ai_do_frame.  But since a no-robots game
		// never goes through ai_do_frame, I'm making it so the control
		// center can spot cloaked dudes.

		if (IsMultiGame)
			gameData.ai.target.vBelievedPos = OBJPOS (OBJECT (LOCALPLAYER.nObject))->vPos;

		//	Hack for special control centers which are isolated and not reachable because the
		//	real control center is inside the boss.
		for (i = 0; i < SEGMENT_SIDE_COUNT; i++)
			if (IS_CHILD (pSeg->m_children [i]))
				break;
		if (i == SEGMENT_SIDE_COUNT)
			return;

		vecToPlayer = OBJPOS (gameData.objData.pConsole)->vPos - pObj->info.position.vPos;
		xDistToPlayer = CFixVector::Normalize (vecToPlayer);
		if (xDistToPlayer < I2X (200)) {
			pReactorStates->bSeenPlayer = AICanSeeTarget (pObj, &pObj->info.position.vPos, 0, &vecToPlayer);
			pReactorStates->nNextFireTime = 0;
			}
		}		
	return;
	}

//	Periodically, make the reactor fall asleep if player not visible.
if (pReactorStates->bHit || pReactorStates->bSeenPlayer) {
	if ((pReactorStates->xLastVisCheckTime + I2X (5) < gameData.time.xGame) || 
		 (pReactorStates->xLastVisCheckTime > gameData.time.xGame)) {
		CFixVector	vecToPlayer;
		fix			xDistToPlayer;

		vecToPlayer = gameData.objData.pConsole->info.position.vPos - pObj->info.position.vPos;
		xDistToPlayer = CFixVector::Normalize (vecToPlayer);
		pReactorStates->xLastVisCheckTime = gameData.time.xGame;
		if (xDistToPlayer < I2X (120)) {
			pReactorStates->bSeenPlayer = AICanSeeTarget (pObj, &pObj->info.position.vPos, 0, &vecToPlayer);
			if (!pReactorStates->bSeenPlayer)
				pReactorStates->bHit = 0;
			}
		}
	}

if ((pReactorStates->nNextFireTime < 0) && 
	 !(gameStates.app.bPlayerIsDead && (gameData.time.xGame > gameStates.app.nPlayerTimeOfDeath + I2X (2)))) {
	nBestGun = CalcBestReactorGun (gameData.reactor.props [pObj->info.nId].nGuns, pReactorStates->vGunPos, pReactorStates->vGunDir, 
											 (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) ? &gameData.ai.target.vBelievedPos : &gameData.objData.pConsole->info.position.vPos);
	if (nBestGun != -1) {
		int32_t		nRandProb, count;
		CFixVector	vecToGoal;
		fix			xDistToPlayer;
		fix			xDeltaFireTime;

		if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
			vecToGoal = gameData.ai.target.vBelievedPos - pReactorStates->vGunPos [nBestGun];
			xDistToPlayer = CFixVector::Normalize (vecToGoal);
			} 
		else {
			vecToGoal = gameData.objData.pConsole->info.position.vPos - pReactorStates->vGunPos [nBestGun];
			xDistToPlayer = CFixVector::Normalize (vecToGoal);
			}
		if (xDistToPlayer > I2X (300)) {
			pReactorStates->bHit = 0;
			pReactorStates->bSeenPlayer = 0;
			return;
			}
		if (IsMultiGame)
			MultiSendCtrlcenFire (&vecToGoal, nBestGun, pObj->Index ());
		CreateNewWeaponSimple (&vecToGoal, &pReactorStates->vGunPos [nBestGun], pObj->Index (), CONTROLCEN_WEAPON_NUM, 1);
		//	some of time, based on level, fire another thing, not directly at player, so it might hit him if he's constantly moving.
		nRandProb = I2X (1) / (abs (missionManager.nCurrentLevel) / 4 + 2);
		count = 0;
		while ((RandShort () > nRandProb) && (count < 4)) {
			CFixVector	vRand;

			vRand = CFixVector::Random();
			vecToGoal += vRand * (I2X (1)/6);
			CFixVector::Normalize (vecToGoal);
			if (IsMultiGame)
				MultiSendCtrlcenFire (&vecToGoal, nBestGun, pObj->Index ());
			CreateNewWeaponSimple (&vecToGoal, &pReactorStates->vGunPos [nBestGun], pObj->Index (), CONTROLCEN_WEAPON_NUM, 0);
			count++;
			}
		xDeltaFireTime = I2X (DIFFICULTY_LEVEL_COUNT - gameStates.app.nDifficultyLevel) / 4;
		if (gameStates.app.nDifficultyLevel == 0)
			xDeltaFireTime += (fix) (I2X (1) / 2 * gameStates.gameplay.slowmo [0].fSpeed);
		if (IsMultiGame) // slow down rate of fire in multi player
			xDeltaFireTime *= 2;
		pReactorStates->nNextFireTime = xDeltaFireTime;
		}
	} 
else
	pReactorStates->nNextFireTime -= gameData.physics.xTime;
}

//	-----------------------------------------------------------------------------

fix ReactorStrength (void)
{
if (gameData.reactor.nStrength == -1) {		//use old defaults
	//	Boost control center strength at higher levels.
	if (missionManager.nCurrentLevel >= 0)
		return I2X (200) + I2X (50) * missionManager.nCurrentLevel;
	return I2X (200) - missionManager.nCurrentLevel * I2X (150);
	}
return I2X (gameData.reactor.nStrength);
}

//	-----------------------------------------------------------------------------
//	This must be called at the start of each level.
//	If this level contains a boss and mode != multiplayer, don't do control center stuff.  (Ghost out control center CObject.)
//	If this level contains a boss and mode == multiplayer, do control center stuff.
void InitReactorForLevel (int32_t bRestore)
{
gameStates.gameplay.bMultiBosses = gameStates.app.bD2XLevel && EGI_FLAG (bMultiBosses, 0, 0, 0);
extraGameInfo [0].nBossCount [0] = 
extraGameInfo [0].nBossCount [1] = 0;
gameStates.gameplay.nReactorCount [0] =
gameStates.gameplay.nReactorCount [1] = 0;
gameData.reactor.bPresent = 0;

if (bRestore) {
	for (int32_t i = 0; i < MAX_BOSS_COUNT; i++)
		if (gameData.reactor.states [i].nObject <= 0) {
			gameStates.gameplay.nLastReactor = i - 1;
			break;
			}
	}
else {
	gameStates.gameplay.nLastReactor = -1;
	gameData.reactor.states.Clear (char (0xff));
	}

CObject *pObj;
FORALL_ACTOR_OBJS (pObj) {
	if (pObj->info.nType == OBJ_REACTOR) {
		if (gameStates.gameplay.nReactorCount [0] && !(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses))
			PrintLog (0, "Warning: Two or more control centers including %i and %i\n", gameData.reactor.states [0].nObject, pObj->Index ());
		else {
			//	Compute all gun positions.
			int32_t j = bRestore ? -1 : FindReactor (pObj);
			int32_t bNew = j < 0;
			if (bNew)
				j = gameStates.gameplay.nReactorCount [0];
			tReactorStates *pReactorStates = gameData.reactor.states + j;
			if (gameStates.gameplay.nLastReactor < j)
				gameStates.gameplay.nLastReactor = j;
			if (bNew)
				pReactorStates->nDeadObj = -1;
			pReactorStates->xLastVisCheckTime = 0;
			if (pReactorStates->nDeadObj < 0) {
				int32_t nGuns = gameData.reactor.props [pObj->info.nId].nGuns;
				for (j = 0; j < nGuns; j++)
					CalcReactorGunPoint (pReactorStates->vGunPos + j, pReactorStates->vGunDir + j, pObj, j);
				gameData.reactor.bPresent = 1;
				pReactorStates->nObject = pObj->Index ();
				if (bNew) {
					pObj->SetShield (ReactorStrength ());
					//	Say the control center has not yet been hit.
					pReactorStates->bHit = 0;
					pReactorStates->bSeenPlayer = 0;
					pReactorStates->nNextFireTime = 0;
					}
				++extraGameInfo [0].nBossCount [0];
				++extraGameInfo [0].nBossCount [1];
				gameStates.gameplay.nLastReactor = gameStates.gameplay.nReactorCount [0];
				gameStates.gameplay.nReactorCount [0]++;
				}
			}
		}

	if (pObj->IsBoss ()) {
		int16_t nBossObj = -1;
		if ((BOSS_COUNT < int32_t (gameData.bosses.ToS ())) || gameData.bosses.Grow ()) {
			gameData.bosses [BOSS_COUNT].m_nObject = pObj->Index ();
			++extraGameInfo [0].nBossCount [1];
			if (ROBOTINFO (pObj) && ROBOTINFO (pObj)->bEndsLevel) {
				++extraGameInfo [0].nBossCount [0];
				if ((nBossObj >= 0) && ROBOTINFO (OBJECT (nBossObj)) && !ROBOTINFO (OBJECT (nBossObj))->bEndsLevel)
					nBossObj = pObj->Index ();
				}
			else if (nBossObj < 0)
				nBossObj = pObj->Index ();
			else
				PrintLog (0, "Warning: Two or more bosses including %i and %i\n", pObj->Index (), nBossObj);
			}
		}
	}

#if DBG
if ((BOSS_COUNT <= 0) && !gameStates.gameplay.nReactorCount [0]) {
	PrintLog (0, "Warning: No control center.\n");
	return;
	}
#endif

if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)
	gameData.reactor.bDisabled = 0;
else if (gameData.bosses.ToS () > 0) {
	for (int32_t j = 0; j < gameStates.gameplay.nReactorCount [0]; j++) {
		pObj = OBJECT (gameData.reactor.states [j].nObject);
		if (pObj) {
			--extraGameInfo [0].nBossCount [1];
			if (ROBOTINFO (pObj) && ROBOTINFO (pObj)->bEndsLevel) 
				--extraGameInfo [0].nBossCount [0];
			if (j < --gameStates.gameplay.nReactorCount [0])
				gameData.reactor.states [j] = gameData.reactor.states [gameStates.gameplay.nReactorCount [0]];
			pObj->BashToShield (true);
			}
		}
	gameData.reactor.bPresent = 0;
	gameData.reactor.bDisabled = 1;
	}
gameStates.gameplay.nReactorCount [1] = gameStates.gameplay.nReactorCount [0];
}

//------------------------------------------------------------------------------

void SpecialReactorStuff (void)
{
#if TRACE
console.printf (CON_DBG, "Mucking with reactor countdown time.\n");
#endif
if (gameData.reactor.bDestroyed) {
	gameData.reactor.countdown.nTimer += 
		I2X (gameStates.app.nBaseCtrlCenExplTime + (DIFFICULTY_LEVEL_COUNT - 1 - gameStates.app.nDifficultyLevel) * gameStates.app.nBaseCtrlCenExplTime / (DIFFICULTY_LEVEL_COUNT-1));
	gameData.reactor.countdown.nTotalTime = X2I (gameData.reactor.countdown.nTimer) + 2;	//	Will prevent "Self destruct sequence activated" message from replaying.
	}
}

//------------------------------------------------------------------------------

void ReadReactor (tReactorProps& reactor, CFile& cf)
{
	int32_t i;

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
int32_t ReadReactors (CFile& cf)
{
	int32_t i;

for (i = 0; i < gameData.reactor.nReactors; i++)
	ReadReactor (gameData.reactor.props [i], cf); 
return gameData.reactor.nReactors;
}

//------------------------------------------------------------------------------

int32_t ReadReactorTriggers (CFile& cf)
{
for (int32_t i = 0; i < gameFileInfo.control.count; i++) {
	gameData.reactor.triggers.m_nLinks = cf.ReadShort ();
	gameData.reactor.triggers.Read (cf);
	}
return gameFileInfo.control.count;
}

//------------------------------------------------------------------------------
