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

#include "inferno.h"
#include "error.h"
#include "gameseg.h"
#include "fireball.h"
#include "network.h"
#include "multibot.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif
//#define _DEBUG
#if DBG
#include "string.h"
#include <time.h>
#endif

#define	MAX_SPEW_BOT		3

int spewBots [2][NUM_D2_BOSSES][MAX_SPEW_BOT] = {
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

int	maxSpewBots [NUM_D2_BOSSES] = {2, 1, 2, 3, 3, 3, 3, 3};

// ---------------------------------------------------------------------------------------------------------------------
//	Call every time the CPlayerData starts a new ship.
void AIInitBossForShip (void)
{
	int	i;

for (i = 0; i < MAX_BOSS_COUNT; i++)
	gameData.boss [i].nHitTime = -F1_0 * 10;
}

#define	QUEUE_SIZE	256

// --------------------------------------------------------------------------------------------------------------------
//	Create list of segments boss is allowed to teleport to at segListP.
//	Set *segCountP.
//	Boss is allowed to teleport to segments he fits in (calls ObjectIntersectsWall) and
//	he can reach from his initial position (calls FindConnectedDistance).
//	If bSizeCheck is set, then only add CSegment if boss can fit in it, else any segment is legal.
//	bOneWallHack added by MK, 10/13/95: A mega-hack! Set to !0 to ignore the
void InitBossSegments (int objList, short segListP [], short *segCountP, int bSizeCheck, int bOneWallHack)
{
	CSegment		*segP;
	CObject		*bossObjP = OBJECTS + objList;
	CFixVector	vBossHomePos;
	int			nBossHomeSeg;
	int			head, tail, w, childSeg;
	int			nGroup, nSide, nSegments;
	int			seqQueue [QUEUE_SIZE];
	fix			xBossSizeSave;

#ifdef EDITOR
nSelectedSegs = 0;
#endif
//	See if there is a boss.  If not, quick out.
nSegments = 0;
xBossSizeSave = bossObjP->info.xSize;
// -- Causes problems!!	-- bossObjP->info.xSize = FixMul ((F1_0/4)*3, bossObjP->info.xSize);
nBossHomeSeg = bossObjP->info.nSegment;
vBossHomePos = bossObjP->info.position.vPos;
nGroup = SEGMENTS [nBossHomeSeg].m_group;
head =
tail = 0;
seqQueue [head++] = nBossHomeSeg;
segListP [nSegments++] = nBossHomeSeg;
#ifdef EDITOR
Selected_segs [nSelectedSegs++] = nBossHomeSeg;
#endif

memset (gameData.render.mine.bVisited, 0, gameData.segs.nSegments);

while (tail != head) {
	segP = SEGMENTS + seqQueue [tail++];
	tail &= QUEUE_SIZE-1;
	for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
		childSeg = segP->m_children [nSide];
		if (!bOneWallHack) {
			w = segP->IsDoorWay (nSide, NULL);
			if (!(w & WID_FLY_FLAG))
				continue;
			}
		//	If we get here and w == WID_WALL, then we want to process through this CWall, else not.
		if (!IS_CHILD (childSeg))
			continue;
		if (bOneWallHack)
			bOneWallHack--;
		if (gameData.render.mine.bVisited [childSeg])
			continue;
		if (nGroup != SEGMENTS [childSeg].m_group)
			continue;
		seqQueue [head++] = childSeg;
		gameData.render.mine.bVisited [childSeg] = 1;
		head &= QUEUE_SIZE - 1;
		if (head > tail) {
			if (head == tail + QUEUE_SIZE-1)
				goto errorExit;	//	queue overflow.  Make it bigger!
			}
		else if (head + QUEUE_SIZE == tail + QUEUE_SIZE - 1)
			goto errorExit;	//	queue overflow.  Make it bigger!
		if (bSizeCheck && !BossFitsInSeg (bossObjP, childSeg))
			continue;
		if (nSegments >= MAX_BOSS_TELEPORT_SEGS - 1)
			goto errorExit;
		segListP [nSegments++] = childSeg;
#ifdef EDITOR
		Selected_segs [nSelectedSegs++] = childSeg;
#endif
		}
	}

errorExit:

bossObjP->info.xSize = xBossSizeSave;
bossObjP->info.position.vPos = vBossHomePos;
OBJECTS [objList].RelinkToSeg (nBossHomeSeg);
*segCountP = nSegments;
}

// ---------------------------------------------------------------------------------------------------------------------

void InitBossData (int i, int nObject)
{
if (nObject >= 0)
	gameData.boss [i].nObject = nObject;
else if (gameData.boss [i].nObject < 0)
	return;
InitBossSegments (gameData.boss [i].nObject, gameData.boss [i].gateSegs, &gameData.boss [i].nGateSegs, 0, 0);
InitBossSegments (gameData.boss [i].nObject, gameData.boss [i].teleportSegs, &gameData.boss [i].nTeleportSegs, 1, 0);
if (gameData.boss [i].nTeleportSegs == 1)
	InitBossSegments (gameData.boss [i].nObject, gameData.boss [i].teleportSegs, &gameData.boss [i].nTeleportSegs, 1, 1);
gameData.boss [i].bDyingSoundPlaying = 0;
gameData.boss [i].nDying = 0;
if (gameStates.app.bD1Mission)
	gameData.boss [i].nGateInterval = F1_0 * 5 - gameStates.app.nDifficultyLevel * F1_0 / 2;
else
	gameData.boss [i].nGateInterval = F1_0 * 4 - gameStates.app.nDifficultyLevel * I2X (2) / 3;
if (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel) {
	gameData.boss [i].nTeleportInterval = F1_0*10;
	gameData.boss [i].nCloakInterval = F1_0*15;					//	Time between cloaks
	}
else {
	gameData.boss [i].nTeleportInterval = F1_0*7;
	gameData.boss [i].nCloakInterval = F1_0*10;					//	Time between cloaks
	}
}

// ---------------------------------------------------------------------------------------------------------------------

int AddBoss (int nObject)
{
	int	i = FindBoss (nObject);

if (i >= 0)
	return i;
i = extraGameInfo [0].nBossCount++;
if (i >= MAX_BOSS_COUNT)
	return -1;
InitBossData (i, nObject);
return i;
}

// ---------------------------------------------------------------------------------------------------------------------

void RemoveBoss (int i)
{
extraGameInfo [0].nBossCount--;
if (i < BOSS_COUNT)
	gameData.boss [i] = gameData.boss [BOSS_COUNT];
memset (gameData.boss + BOSS_COUNT, 0, sizeof (gameData.boss [BOSS_COUNT]));
}

// --------------------------------------------------------------------------------------------------------------------
//	Return nObject if CObject created, else return -1.
//	If pos == NULL, pick random spot in CSegment.
int CObject::CreateGatedRobot (short nSegment, ubyte nObjId, CFixVector* vPos)
{
	int			nObject, nTries = 5;
	CObject		*objP;
	CSegment		*segP = SEGMENTS + nSegment;
	CFixVector	vObjPos;
	tRobotInfo	*botInfoP = &ROBOTINFO (nObjId);
	int			i, nBoss, count = 0;
	fix			objsize = gameData.models.polyModels [botInfoP->nModel].rad;
	ubyte			default_behavior;

if (gameStates.gameplay.bNoBotAI)
	return -1;
nBoss = FindBoss (OBJ_IDX (this));
if (nBoss < 0)
	return -1;
if (gameData.time.xGame - gameData.boss [nBoss].nLastGateTime < gameData.boss [nBoss].nGateInterval)
	return -1;
FORALL_ROBOT_OBJS (objP, i)
	if (objP->info.nCreator == BOSS_GATE_MATCEN_NUM)
		count++;
if (count > 2 * gameStates.app.nDifficultyLevel + 6) {
	gameData.boss [nBoss].nLastGateTime = gameData.time.xGame - 3 * gameData.boss [nBoss].nGateInterval / 4;
	return -1;
	}
vObjPos = segP->Center ();
for (;;) {
	if (!vPos)
		vObjPos = segP->RandomPoint ();
	else
		vObjPos = *vPos;

	//	See if legal to place CObject here.  If not, move about in CSegment and try again.
	if (CheckObjectObjectIntersection (&vObjPos, objsize, segP)) {
		if (!--nTries) {
			gameData.boss [nBoss].nLastGateTime = gameData.time.xGame - 3 * gameData.boss [nBoss].nGateInterval / 4;
			return -1;
			}
		vPos = NULL;
		}
	else
		break;
	}
nObject = CreateRobot (nObjId, nSegment, vObjPos);
if (nObject < 0) {
	gameData.boss [nBoss].nLastGateTime = gameData.time.xGame - 3 * gameData.boss [nBoss].nGateInterval / 4;
	return -1;
	}
// added lifetime increase depending on difficulty level 04/26/06 DM
gameData.multigame.create.nObjNums [0] = nObject; // A convenient global to get nObject back to caller for multiplayer
objP = OBJECTS + nObject;
objP->info.xLifeLeft = F1_0 * 30 + F0_5 * (gameStates.app.nDifficultyLevel * 15);	//	Gated in robots only live 30 seconds.
//Set polygon-CObject-specific data
objP->rType.polyObjInfo.nModel = botInfoP->nModel;
objP->rType.polyObjInfo.nSubObjFlags = 0;
//set Physics info
objP->mType.physInfo.mass = botInfoP->mass;
objP->mType.physInfo.drag = botInfoP->drag;
objP->mType.physInfo.flags |= (PF_LEVELLING);
objP->info.xShields = botInfoP->strength;
objP->info.nCreator = BOSS_GATE_MATCEN_NUM;	//	flag this robot as having been created by the boss.
default_behavior = ROBOTINFO (objP->info.nId).behavior;
InitAIObject (OBJ_IDX (objP), default_behavior, -1);		//	Note, -1 = CSegment this robot goes to to hide, should probably be something useful
/*Object*/CreateExplosion (nSegment, &vObjPos, I2X (10), VCLIP_MORPHING_ROBOT);
DigiLinkSoundToPos (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, nSegment, 0, &vObjPos, 0 , F1_0);
MorphStart (objP);
gameData.boss [nBoss].nLastGateTime = gameData.time.xGame;
LOCALPLAYER.numRobotsLevel++;
LOCALPLAYER.numRobotsTotal++;
return OBJ_IDX (objP);
}

//	----------------------------------------------------------------------------------------------------------
//	objP points at a boss.  He was presumably just hit and he's supposed to create a bot at the hit location *pos.
int CObject::BossSpewRobot (CFixVector *vPos, short objType, int bObjTrigger)
{
	short			nObject, nSegment, maxRobotTypes;
	short			nBossIndex, nBossId = ROBOTINFO (info.nId).bossFlag;
	tRobotInfo	*pri;

if (!bObjTrigger && FindObjTrigger (OBJ_IDX (this), TT_SPAWN_BOT, -1))
	return -1;
nBossIndex = (nBossId >= BOSS_D2) ? nBossId - BOSS_D2 : nBossId;
Assert ((nBossIndex >= 0) && (nBossIndex < NUM_D2_BOSSES));
nSegment = vPos ? FindSegByPos (*vPos, info.nSegment, 1, 0) : info.nSegment;
if (nSegment == -1) {
#if TRACE
	console.printf (CON_DBG, "Tried to spew a bot outside the mine! Aborting!\n");
#endif
	return -1;
	}
if (!vPos) 
	vPos = SEGMENTS [nSegment].Center ();
if (objType < 0)
	objType = spewBots [gameStates.app.bD1Mission][nBossIndex][(maxSpewBots [nBossIndex] * d_rand ()) >> 15];
if (objType == 255) {	// spawn an arbitrary robot
	maxRobotTypes = gameData.bots.nTypes [gameStates.app.bD1Mission];
	do {
		objType = d_rand () % maxRobotTypes;
		pri = gameData.bots.info [gameStates.app.bD1Mission] + objType;
		} while (pri->bossFlag ||	//well ... don't spawn another boss, huh? ;)
					pri->companion || //the buddy bot isn't exactly an enemy ... ^_^
					(pri->scoreValue < 700)); //avoid spawning a ... spawn nType bot
	}
nObject = CreateGatedRobot (nSegment, (ubyte) objType, vPos);
//	Make spewed robot come tumbling out as if blasted by a flash missile.
if (nObject != -1) {
	CObject	*newObjP = OBJECTS + nObject;
	int		force_val = F1_0 / (gameData.time.xFrame ? gameData.time.xFrame : 1);
	if (force_val) {
		newObjP->cType.aiInfo.SKIP_AI_COUNT += force_val;
		newObjP->mType.physInfo.rotThrust[X] = ((d_rand () - 16384) * force_val)/16;
		newObjP->mType.physInfo.rotThrust[Y] = ((d_rand () - 16384) * force_val)/16;
		newObjP->mType.physInfo.rotThrust[Z] = ((d_rand () - 16384) * force_val)/16;
		newObjP->mType.physInfo.flags |= PF_USES_THRUST;

		//	Now, give a big initial velocity to get moving away from boss.
		newObjP->mType.physInfo.velocity = *vPos - info.position.vPos;
		CFixVector::Normalize (newObjP->mType.physInfo.velocity);
		newObjP->mType.physInfo.velocity *= (F1_0*128);
		}
	}
return nObject;
}

// --------------------------------------------------------------------------------------------------------------------
//	Make CObject objP gate in a robot.
//	The process of him bringing in a robot takes one second.
//	Then a robot appears somewhere near the player.
//	Return nObject if robot successfully created, else return -1
int GateInRobot (short nObject, ubyte nType, short nSegment)
{
if (nSegment < 0) {
	int i = FindBoss (nObject);
	if (i < 0)
		return -1;
	nSegment = gameData.boss [i].gateSegs [(d_rand () * gameData.boss [i].nGateSegs) >> 15];
	}
Assert ((nSegment >= 0) && (nSegment <= gameData.segs.nLastSegment));
return CreateGatedRobot (OBJECTS + nObject, nSegment, nType, NULL);
}

// --------------------------------------------------------------------------------------------------------------------

int BossFitsInSeg (CObject *bossObjP, int nSegment)
{
	int			nObject = OBJ_IDX (bossObjP);
	int			nPos;
	CFixVector	vSegCenter, vVertPos;

gameData.collisions.nSegsVisited = 0;
vSegCenter = SEGMENTS [nSegment].Center ();
for (nPos = 0; nPos < 9; nPos++) {
	if (!nPos)
		bossObjP->info.position.vPos = vSegCenter;
	else {
		vVertPos = gameData.segs.vertices [SEGMENTS [nSegment].m_verts [nPos-1]];
		bossObjP->info.position.vPos = CFixVector::Avg(vVertPos, vSegCenter);
		}
	OBJECTS [nObject].RelinkToSeg (nSegment);
	if (!ObjectIntersectsWall (bossObjP))
		return 1;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int IsValidTeleportDest (CFixVector *vPos, int nMinDist)
{
	CObject		*objP;
	int			i;
	CFixVector	vOffs;
	fix			xDist;

FORALL_ACTOR_OBJS (objP, i) {
	vOffs = *vPos - objP->info.position.vPos;
	xDist = vOffs.Mag();
	if (xDist > ((nMinDist + objP->info.xSize) * 3 / 2))
		return 1;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

void TeleportBoss (CObject *objP)
{
	short			i, nAttempts = 5, nRandSeg = 0, nRandIndex, nObject = OBJ_IDX (objP);
	CFixVector	vBossDir, vNewPos;

//	Pick a random CSegment from the list of boss-teleportable-to segments.
Assert (nObject >= 0);
i = FindBoss (nObject);
if (i < 0)
	return;
//Assert (gameData.boss [i].nTeleportSegs > 0);
if (gameData.boss [i].nTeleportSegs <= 0)
	return;
do {
	nRandIndex = (d_rand () * gameData.boss [i].nTeleportSegs) >> 15;
	nRandSeg = gameData.boss [i].teleportSegs [nRandIndex];
	Assert ((nRandSeg >= 0) && (nRandSeg <= gameData.segs.nLastSegment));
	if (IsMultiGame)
		MultiSendBossActions (nObject, 1, nRandSeg, 0);
	vNewPos = SEGMENTS [nRandSeg].Center ();
	if (IsValidTeleportDest (&vNewPos, objP->info.xSize))
		break;
	}
	while (--nAttempts);
if (!nAttempts)
	return;
OBJECTS [nObject].RelinkToSeg (nRandSeg);
gameData.boss [i].nLastTeleportTime = gameData.time.xGame;
//	make boss point right at CPlayerData
objP->info.position.vPos = vNewPos;
vBossDir = OBJECTS [LOCALPLAYER.nObject].info.position.vPos - vNewPos;
objP->info.position.mOrient = CFixMatrix::CreateF(vBossDir);
DigiLinkSoundToPos (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, nRandSeg, 0, &objP->info.position.vPos, 0 , F1_0);
DigiKillSoundLinkedToObject (nObject);
DigiLinkSoundToObject2 (ROBOTINFO (objP->info.nId).seeSound, OBJ_IDX (objP), 1, F1_0, F1_0*512, SOUNDCLASS_ROBOT);	//	F1_0*512 means play twice as loud
//	After a teleport, boss can fire right away.
gameData.ai.localInfo [nObject].nextPrimaryFire = 0;
gameData.ai.localInfo [nObject].nextSecondaryFire = 0;
}

//	----------------------------------------------------------------------

void StartBossDeathSequence (CObject *objP)
{
if (ROBOTINFO (objP->info.nId).bossFlag) {
	int	nObject = OBJ_IDX (objP),
			i = FindBoss (nObject);

	if (i < 0)
		StartRobotDeathSequence (objP);	//kill it anyway, somehow
	else {
		gameData.boss [i].nDying = nObject;
		gameData.boss [i].nDyingStartTime = gameData.time.xGame;
		}
	}
}

//	----------------------------------------------------------------------

void DoBossDyingFrame (CObject *objP)
{
	int	rval, i = FindBoss (OBJ_IDX (objP));

if (i < 0)
	return;
rval = DoRobotDyingFrame (objP, gameData.boss [i].nDyingStartTime, BOSS_DEATH_DURATION,
								 &gameData.boss [i].bDyingSoundPlaying,
								 ROBOTINFO (objP->info.nId).deathrollSound, F1_0*4, F1_0*4);
if (rval) {
	RemoveBoss (i);
	DoReactorDestroyedStuff (NULL);
	ExplodeObject (objP, F1_0/4);
	DigiLinkSoundToObject2 (SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F2_0, F1_0*512, SOUNDCLASS_EXPLOSION);
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void DoBossStuff (CObject *objP, int nPlayerVisibility)
{
	int	i, nBossId, nBossIndex, nObject;

nObject = OBJ_IDX (objP);
i = FindBoss (nObject);
if (i < 0)
	return;
nBossId = ROBOTINFO (objP->info.nId).bossFlag;
//	Assert ((nBossId >= BOSS_D2) && (nBossId < BOSS_D2 + NUM_D2_BOSSES));
nBossIndex = (nBossId >= BOSS_D2) ? nBossId - BOSS_D2 : nBossId;
#if DBG
if (objP->info.xShields != gameData.boss [i].xPrevShields) {
#if TRACE
	console.printf (CON_DBG, "Boss shields = %7.3f, CObject %i\n", X2F (objP->info.xShields), OBJ_IDX (objP));
#endif
	gameData.boss [i].xPrevShields = objP->info.xShields;
	}
#endif
	//	New code, fixes stupid bug which meant boss never gated in robots if > 32767 seconds played.
if (gameData.boss [i].nLastTeleportTime > gameData.time.xGame)
	gameData.boss [i].nLastTeleportTime = gameData.time.xGame;

if (gameData.boss [i].nLastGateTime > gameData.time.xGame)
	gameData.boss [i].nLastGateTime = gameData.time.xGame;

//	@mk, 10/13/95:  Reason:
//		Level 4 boss behind locked door.  But he's allowed to teleport out of there.  So he
//		teleports out of there right away, and blasts CPlayerData right after first door.
if (!gameData.ai.nPlayerVisibility && (gameData.time.xGame - gameData.boss [i].nHitTime > F1_0*2))
	return;

if (bossProps [gameStates.app.bD1Mission][nBossIndex].bTeleports) {
	if (objP->cType.aiInfo.CLOAKED == 1) {
		gameData.boss [i].nHitTime = gameData.time.xGame;	//	Keep the cloak:teleport process going.
		if ((gameData.time.xGame - gameData.boss [i].nCloakStartTime > BOSS_CLOAK_DURATION / 3) &&
			 (gameData.boss [i].nCloakEndTime - gameData.time.xGame > BOSS_CLOAK_DURATION / 3) &&
			 (gameData.time.xGame - gameData.boss [i].nLastTeleportTime > gameData.boss [i].nTeleportInterval)) {
			if (AIMultiplayerAwareness (objP, 98)) {
				TeleportBoss (objP);
				if (bossProps [gameStates.app.bD1Mission][nBossIndex].bSpewBotsTeleport) {
					CFixVector	spewPoint;
					spewPoint = objP->info.position.mOrient.FVec () * (objP->info.xSize * 2);
					spewPoint += objP->info.position.vPos;
					if (bossProps [gameStates.app.bD1Mission][nBossIndex].bSpewMore && (d_rand () > 16384) &&
						 (BossSpewRobot (objP, &spewPoint, -1, 0) != -1))
						gameData.boss [i].nLastGateTime = gameData.time.xGame - gameData.boss [i].nGateInterval - 1;	//	Force allowing spew of another bot.
					BossSpewRobot (objP, &spewPoint, -1, 0);
					}
				}
			}
		else if (gameData.time.xGame - gameData.boss [i].nHitTime > F1_0*2) {
			gameData.boss [i].nLastTeleportTime -= gameData.boss [i].nTeleportInterval/4;
			}
	if (!gameData.boss [i].nCloakDuration)
		gameData.boss [i].nCloakDuration = BOSS_CLOAK_DURATION;
	if ((gameData.time.xGame > gameData.boss [i].nCloakEndTime) ||
			(gameData.time.xGame < gameData.boss [i].nCloakStartTime))
		objP->cType.aiInfo.CLOAKED = 0;
		}
	else if ((gameData.time.xGame - gameData.boss [i].nCloakEndTime > gameData.boss [i].nCloakInterval) ||
				(gameData.time.xGame - gameData.boss [i].nCloakEndTime < -gameData.boss [i].nCloakDuration)) {
		if (AIMultiplayerAwareness (objP, 95)) {
			gameData.boss [i].nCloakStartTime = gameData.time.xGame;
			gameData.boss [i].nCloakEndTime = gameData.time.xGame + gameData.boss [i].nCloakDuration;
			objP->cType.aiInfo.CLOAKED = 1;
			if (IsMultiGame)
				MultiSendBossActions (OBJ_IDX (objP), 2, 0, 0);
			}
		}
	}
}

//	---------------------------------------------------------------
// eof

