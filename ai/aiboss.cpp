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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "descent.h"
#include "error.h"
#include "segmath.h"
#include "fireball.h"
#include "network.h"
#include "multibot.h"

#if DBG
#include "string.h"
#include <time.h>
#endif

#define	MAX_SPEW_BOT		3

int32_t spewBots [2][NUM_D2_BOSSES][MAX_SPEW_BOT] = {
 {
 { 38, 40, -1},
 { 37, -1, -1},
 { 43, 57, -1},
 { 26, 27, 58},
 { 59, 58, 54},
 { 60, 61, 54},
 { 69, 29, 24},
 { 72, 60, 73}
	},
 {
 {  3,  -1, -1},
 {  9,   0, -1},
 {255, 255, -1},
 { 26,  27, 58},
 { 59,  58, 54},
 { 60,  61, 54},
 { 69,  29, 24},
 { 72,  60, 73}
	}
};

int32_t	maxSpewBots [NUM_D2_BOSSES] = {2, 1, 2, 3, 3, 3, 3, 3};

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
//	Return nObject if CObject created, else return -1.
//	If pos == NULL, pick random spot in CSegment.
int32_t CObject::CreateGatedRobot (int16_t nSegment, uint8_t nObjId, CFixVector* vPos)
{
ENTER (0);
tRobotInfo*	pRobotInfo = ROBOTINFO (nObjId);
if (!pRobotInfo)
	RETURN (-1)

	int32_t		nObject, nTries = 5;
	CObject*		pObj;
	CSegment*	pSeg = SEGMENT (nSegment);
	CFixVector	vObjPos;
	int32_t		nBoss, count = 0;
	fix			objsize = gameData.modelData.polyModels [0][pRobotInfo->nModel].Rad ();
	uint8_t		default_behavior;

#if !DBG
if (gameStates.app.bGameSuspended & SUSP_ROBOTS)
#endif
	RETURN (-1)
nBoss = gameData.bossData.Find (OBJ_IDX (this));
if (nBoss < 0)
	RETURN (-1)
if (gameData.timeData.xGame - gameData.bossData [nBoss].m_nLastGateTime < gameData.bossData [nBoss].m_nGateInterval)
	RETURN (-1)
FORALL_ROBOT_OBJS (pObj)
	if (pObj->info.nCreator == BOSS_GATE_PRODUCER_NUM)
		count++;
if (count > 2 * gameStates.app.nDifficultyLevel + 6) {
	gameData.bossData [nBoss].m_nLastGateTime = gameData.timeData.xGame - 3 * gameData.bossData [nBoss].m_nGateInterval / 4;
	RETURN (-1)
	}
vObjPos = pSeg->Center ();
for (;;) {
	if (!vPos)
		vObjPos = pSeg->RandomPoint ();
	else
		vObjPos = *vPos;

	//	See if legal to place CObject here.  If not, move about in CSegment and try again.
	if (CheckObjectObjectIntersection (&vObjPos, objsize, pSeg)) {
		if (!--nTries) {
			gameData.bossData [nBoss].m_nLastGateTime = gameData.timeData.xGame - 3 * gameData.bossData [nBoss].m_nGateInterval / 4;
			RETURN (-1)
			}
		vPos = NULL;
		}
	else
		break;
	}
nObject = CreateRobot (nObjId, nSegment, vObjPos);
pObj = OBJECT (nObject);
if (!pObj) {
	gameData.bossData [nBoss].m_nLastGateTime = gameData.timeData.xGame - 3 * gameData.bossData [nBoss].m_nGateInterval / 4;
	RETURN (-1)
	}
// added lifetime increase depending on difficulty level 04/26/06 DM
gameData.multigame.create.nObjNums [0] = nObject; // A convenient global to get nObject back to caller for multiplayer
pObj->SetLife (I2X (30) + (I2X (1) / 2) * (gameStates.app.nDifficultyLevel * 15));	//	Gated in robots only live 30 seconds.
//Set polygon-CObject-specific data
pObj->rType.polyObjInfo.nModel = pRobotInfo->nModel;
pObj->rType.polyObjInfo.nSubObjFlags = 0;
//set Physics info
pObj->mType.physInfo.mass = pRobotInfo->mass;
pObj->mType.physInfo.drag = pRobotInfo->drag;
pObj->mType.physInfo.flags |= (PF_LEVELLING);
pObj->SetShield (pRobotInfo->strength);
pObj->info.nCreator = BOSS_GATE_PRODUCER_NUM;	//	flag this robot as having been created by the boss.
pRobotInfo = ROBOTINFO (pObj);
default_behavior = pRobotInfo ? pRobotInfo->behavior : 0;
InitAIObject (pObj->Index (), default_behavior, -1);		//	Note, -1 = CSegment this robot goes to to hide, should probably be something useful
CreateExplosion (nSegment, vObjPos, I2X (10), ANIM_MORPHING_ROBOT);
audio.CreateSegmentSound (gameData.effectData.animations [0][ANIM_MORPHING_ROBOT].nSound, nSegment, 0, vObjPos, 0 , I2X (1));
pObj->MorphStart ();
gameData.bossData [nBoss].m_nLastGateTime = gameData.timeData.xGame;
LOCALPLAYER.numRobotsLevel++;
LOCALPLAYER.numRobotsTotal++;
RETURN (pObj->Index ())
}

//	----------------------------------------------------------------------------------------------------------
//	pObj points at a boss.  He was presumably just hit and he's supposed to create a bot at the hit location *pos.
int32_t CObject::BossSpewRobot (CFixVector *vPos, int16_t objType, int32_t bObjTrigger)
{
ENTER (0);
if (!bObjTrigger && FindObjTrigger (OBJ_IDX (this), TT_SPAWN_BOT, -1)) // not caused by an object trigger, but object has an object trigger
	RETURN (-1)

tRobotInfo *pRobotInfo = ROBOTINFO (info.nId);
if (!pRobotInfo)
	RETURN (-1)

	int16_t			nObject, nSegment, maxRobotTypes;
	int16_t			nBossIndex, nBossId = pRobotInfo->bossFlag;

nBossIndex = (nBossId >= BOSS_D2) ? nBossId - BOSS_D2 : nBossId;
nSegment = vPos ? FindSegByPos (*vPos, info.nSegment, 1, 0) : info.nSegment;
if (nSegment == -1) {
#if TRACE
	console.printf (CON_DBG, "Tried to spew a bot outside the mine! Aborting!\n");
#endif
	RETURN (-1)
	}
if (!vPos) 
	vPos = &SEGMENT (nSegment)->Center ();
if (objType < 0)
	objType = spewBots [gameStates.app.bD1Mission][nBossIndex][(maxSpewBots [nBossIndex] * RandShort ()) >> 15];
if (objType == 255) {	// spawn an arbitrary robot
	maxRobotTypes = gameData.botData.nTypes [gameStates.app.bD1Mission];
	do {
		pRobotInfo = ROBOTINFO (Rand (maxRobotTypes));
		} while (!pRobotInfo ||							//should be a valid robot ...
					pRobotInfo->bossFlag ||				//well ... don't spawn another boss, huh? ;)
					pRobotInfo->companion ||				//the buddy bot isn't exactly an enemy ... ^_^
					(pRobotInfo->scoreValue < 700));	//avoid spawning a ... spawn nType bot
	}
nObject = CreateGatedRobot (nSegment, (uint8_t) objType, vPos);
//	Make spewed robot come tumbling out as if blasted by a flash missile.
if (nObject != -1) {
	CObject	*pNewObj = OBJECT (nObject);
	int32_t		xForce = I2X (1) / (gameData.timeData.xFrame ? gameData.timeData.xFrame : 1);
	if (xForce) {
		pNewObj->cType.aiInfo.SKIP_AI_COUNT += xForce;
		pNewObj->mType.physInfo.rotThrust.v.coord.x = (SRandShort () * xForce) / 16;
		pNewObj->mType.physInfo.rotThrust.v.coord.y = (SRandShort () * xForce) / 16;
		pNewObj->mType.physInfo.rotThrust.v.coord.z = (SRandShort () * xForce) / 16;
		pNewObj->mType.physInfo.flags |= PF_USES_THRUST;

		//	Now, give a big initial velocity to get moving away from boss.
		pNewObj->mType.physInfo.velocity = *vPos - info.position.vPos;
		CFixVector::Normalize (pNewObj->mType.physInfo.velocity);
		pNewObj->mType.physInfo.velocity *= I2X (128);
		}
	}
RETURN (nObject)
}

// --------------------------------------------------------------------------------------------------------------------
//	Make CObject pObj gate in a robot.
//	The process of him bringing in a robot takes one second.
//	Then a robot appears somewhere near the player.
//	Return nObject if robot successfully created, else return -1
int32_t GateInRobot (int16_t nObject, uint8_t nType, int16_t nSegment)
{
ENTER (0);
if (!gameData.bossData.ToS ())
	RETURN (-1)
if (nSegment < 0) {
	int32_t nBoss = gameData.bossData.Find (nObject);
	if (nBoss < 0)
		RETURN (-1)
	if (!(gameData.bossData [nBoss].m_gateSegs.Buffer () && gameData.bossData [nBoss].m_nGateSegs))
		RETURN (-1)
	nSegment = gameData.bossData [nBoss].m_gateSegs [(RandShort () * gameData.bossData [nBoss].m_nGateSegs) >> 15];
	}
Assert ((nSegment >= 0) && (nSegment <= gameData.segData.nLastSegment));
RETURN (OBJECT (nObject) ? OBJECT (nObject)->CreateGatedRobot (nSegment, nType, NULL) : -1)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t BossFitsInSeg (CObject *pBossObj, int32_t nSegment)
{
ENTER (0);
	int32_t		nObject = OBJ_IDX (pBossObj);
	int32_t		nPos;
	CFixVector	vSegCenter, vVertPos;
	CSegment*	pSeg = SEGMENT (nSegment);

memset (gameData.collisionData.nSegsVisited, 0, sizeof (gameData.collisionData.nSegsVisited));
vSegCenter = pSeg->Center ();
for (nPos = 0; nPos < 9; nPos++) {
	if (!nPos)
		pBossObj->info.position.vPos = vSegCenter;
	else if (pSeg->m_vertices [nPos - 1] == 0xFFFF)
		continue;
	else {
		vVertPos = gameData.segData.vertices [pSeg->m_vertices [nPos - 1]];
		pBossObj->info.position.vPos = CFixVector::Avg (vVertPos, vSegCenter);
		}
	OBJECT (nObject)->RelinkToSeg (nSegment);
	if (!ObjectIntersectsWall (pBossObj))
		RETURN (1)
	}
RETURN (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t IsValidTeleportDest (CFixVector *vPos, int32_t nMinDist)
{
ENTER (0);
	CObject		*pObj;
	CFixVector	vOffs;
	fix			xDist;

FORALL_ACTOR_OBJS (pObj) {
	vOffs = *vPos - pObj->info.position.vPos;
	xDist = vOffs.Mag();
	if (xDist > ((nMinDist + pObj->info.xSize) * 3 / 2))
		RETURN (1)
	}
RETURN (0)
}

// --------------------------------------------------------------------------------------------------------------------

void TeleportBoss (CObject *pObj)
{
ENTER (0);
	int16_t		i, nAttempts = 5, nRandSeg = 0, nRandIndex, nObject = pObj->Index ();
	CFixVector	vBossDir, vNewPos;

//	Pick a random CSegment from the list of boss-teleportable-to segments.
i = gameData.bossData.Find (nObject);
if (i < 0)
	LEAVE
// do not teleport if less than 1% of initial shield strength left or drives heavily damaged
if ((pObj->Damage () < 0.01) || (RandShort () > pObj->DriveDamage ()))
	LEAVE
//Assert (gameData.bossData [i].m_nTeleportSegs > 0);
if (gameData.bossData [i].m_nTeleportSegs <= 0)
	LEAVE
if (gameData.bossData [i].m_nDyingStartTime > 0)
	LEAVE
do {
	nRandIndex = Rand (gameData.bossData [i].m_nTeleportSegs);
	nRandSeg = gameData.bossData [i].m_teleportSegs [nRandIndex];
	Assert ((nRandSeg >= 0) && (nRandSeg <= gameData.segData.nLastSegment));
	if (IsMultiGame)
		MultiSendBossActions (nObject, 1, nRandSeg, 0);
	vNewPos = SEGMENT (nRandSeg)->Center ();
	if (IsValidTeleportDest (&vNewPos, pObj->info.xSize))
		break;
	}
	while (--nAttempts);
if (!nAttempts)
	LEAVE
pObj->RelinkToSeg (nRandSeg);
gameData.bossData [i].m_nLastTeleportTime = gameData.timeData.xGame;
//	make boss point right at CPlayerData
pObj->info.position.vPos = vNewPos;
vBossDir = LOCALOBJECT->info.position.vPos - vNewPos;
pObj->info.position.mOrient = CFixMatrix::CreateF(vBossDir);
audio.CreateSegmentSound (gameData.effectData.animations [0][ANIM_MORPHING_ROBOT].nSound, nRandSeg, 0, pObj->info.position.vPos, 0 , I2X (1));
audio.DestroyObjectSound (nObject);
tRobotInfo* pRobotInfo = ROBOTINFO (pObj);
if (pRobotInfo)
	audio.CreateObjectSound (pRobotInfo->seeSound, SOUNDCLASS_ROBOT, pObj->Index (), 1, I2X (1), I2X (512));	//	I2X (512) means play twice as loud
//	After a teleport, boss can fire right away.
gameData.aiData.localInfo [nObject].pNextrimaryFire = 0;
gameData.aiData.localInfo [nObject].nextSecondaryFire = 0;
LEAVE
}

//	----------------------------------------------------------------------

void StartBossDeathSequence (CObject *pObj)
{
ENTER (0);
if (pObj && pObj->IsBoss ()) {
	int32_t	nObject = pObj->Index (),
				i = gameData.bossData.Find (nObject);

	if (i < 0)
		StartRobotDeathSequence (pObj);	//kill it anyway, somehow
	else if (!gameData.bossData [i].m_nDyingStartTime) { // not already dying
		gameData.bossData [i].m_nDying = nObject;
		gameData.bossData [i].m_nDyingStartTime = gameData.timeData.xGame;
		}
	}
LEAVE
}

//	----------------------------------------------------------------------

void DoBossDyingFrame (CObject *pObj)
{
ENTER (0);
tRobotInfo	*pRobotInfo = ROBOTINFO (pObj);
if (!pRobotInfo)
	LEAVE

int32_t	i = gameData.bossData.Find (pObj->Index ());
if (i < 0)
	LEAVE

int32_t rval = DoRobotDyingFrame (pObj, gameData.bossData [i].m_nDyingStartTime, BOSS_DEATH_DURATION,
											 &gameData.bossData [i].m_bDyingSoundPlaying, pRobotInfo->deathrollSound, I2X (4), I2X (4));
if (rval) {
	gameData.bossData.Remove (i);
	if (pRobotInfo->bEndsLevel)
		DoReactorDestroyedStuff (NULL);
#if 0
	audio.CreateObjectSound (-1, SOUNDCLASS_EXPLOSION, pObj->Index (), 0, I2X (4), I2X (512), -1, -1, AddonSoundName (SND_ADDON_NUKE_EXPLOSION), 1);
#else
	audio.CreateObjectSound (SOUND_BADASS_EXPLOSION_ACTOR, SOUNDCLASS_EXPLOSION, pObj->Index (), 0, I2X (4), I2X (512));
#endif
	pObj->Explode (I2X (1)/4);
	}
LEAVE
}

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void DoBossStuff (CObject *pObj, int32_t nTargetVisibility)
{
ENTER (0);
int32_t nBossId = pObj->BossId ();
if (!nBossId)
	LEAVE

int32_t nObject = pObj->Index ();
int32_t i = gameData.bossData.Find (nObject);
if (i < 0)
	LEAVE
int32_t nBossIndex = (nBossId >= BOSS_D2) ? nBossId - BOSS_D2 : nBossId;
#if DBG
if (pObj->info.xShield != gameData.bossData [i].m_xPrevShield) {
#if TRACE
	console.printf (CON_DBG, "Boss shield = %7.3f, CObject %i\n", X2F (pObj->info.xShield), pObj->Index ());
#endif
	gameData.bossData [i].m_xPrevShield = pObj->info.xShield;
	}
#endif
	//	New code, fixes stupid bug which meant boss never gated in robots if > 32767 seconds played.
if (gameData.bossData [i].m_nLastTeleportTime > gameData.timeData.xGame)
	gameData.bossData [i].m_nLastTeleportTime = gameData.timeData.xGame;

if (gameData.bossData [i].m_nLastGateTime > gameData.timeData.xGame)
	gameData.bossData [i].m_nLastGateTime = gameData.timeData.xGame;

//	@mk, 10/13/95:  Reason:
//		Level 4 boss behind locked door.  But he's allowed to teleport out of there.  So he
//		teleports out of there right away, and blasts player right after first door.
if (!gameData.aiData.nTargetVisibility && (gameData.timeData.xGame - gameData.bossData [i].m_nHitTime > I2X (2)))
	LEAVE

if (bossProps [gameStates.app.bD1Mission][nBossIndex].bTeleports) {
	if (pObj->cType.aiInfo.CLOAKED == 1) {
		gameData.bossData [i].m_nHitTime = gameData.timeData.xGame;	//	Keep the cloak:teleport process going.
		if ((gameData.timeData.xGame - gameData.bossData [i].m_nCloakStartTime > BOSS_CLOAK_DURATION / 3) &&
			 (gameData.bossData [i].m_nCloakEndTime - gameData.timeData.xGame > BOSS_CLOAK_DURATION / 3) &&
			 (gameData.timeData.xGame - gameData.bossData [i].m_nLastTeleportTime > gameData.bossData [i].m_nTeleportInterval)) {
			if (AILocalPlayerControlsRobot (pObj, 98)) {
				TeleportBoss (pObj);
				if (bossProps [gameStates.app.bD1Mission][nBossIndex].bSpewBotsTeleport) {
					CFixVector	spewPoint;
					spewPoint = pObj->info.position.mOrient.m.dir.f * (pObj->info.xSize * 2);
					spewPoint += pObj->info.position.vPos;
					if (bossProps [gameStates.app.bD1Mission][nBossIndex].bSpewMore && (RandShort () > 16384) &&
						 (pObj->BossSpewRobot (&spewPoint, -1, 0) != -1))
						gameData.bossData [i].m_nLastGateTime = gameData.timeData.xGame - gameData.bossData [i].m_nGateInterval - 1;	//	Force allowing spew of another bot.
					pObj->BossSpewRobot (&spewPoint, -1, 0);
					}
				}
			}
		else if (gameData.timeData.xGame - gameData.bossData [i].m_nHitTime > I2X (2)) {
			gameData.bossData [i].m_nLastTeleportTime -= gameData.bossData [i].m_nTeleportInterval/4;
			}
		if (!gameData.bossData [i].m_nCloakDuration)
			gameData.bossData [i].m_nCloakDuration = BOSS_CLOAK_DURATION;
		if ((gameData.timeData.xGame > gameData.bossData [i].m_nCloakEndTime) ||
			 (gameData.timeData.xGame < gameData.bossData [i].m_nCloakStartTime) || 
			 (pObj->Damage () < 0.01) || (RandShort () > pObj->DriveDamage ()))
			pObj->cType.aiInfo.CLOAKED = 0;
		}
	else if (((gameData.timeData.xGame - gameData.bossData [i].m_nCloakEndTime > gameData.bossData [i].m_nCloakInterval) ||
				 (gameData.timeData.xGame - gameData.bossData [i].m_nCloakEndTime < -gameData.bossData [i].m_nCloakDuration)) && 
				 (pObj->Damage () >= 0.01) && (RandShort () <= pObj->DriveDamage ())) {
		if (AILocalPlayerControlsRobot (pObj, 95)) {
			gameData.bossData [i].m_nCloakStartTime = gameData.timeData.xGame;
			gameData.bossData [i].m_nCloakEndTime = gameData.timeData.xGame + gameData.bossData [i].m_nCloakDuration;
			pObj->cType.aiInfo.CLOAKED = 1;
			if (IsMultiGame)
				MultiSendBossActions (pObj->Index (), 2, 0, 0);
			}
		}
	}
LEAVE
}

//	---------------------------------------------------------------
// eof

