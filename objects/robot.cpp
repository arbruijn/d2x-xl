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
#include <string.h>

#include "descent.h"
#include "error.h"
#include "interp.h"

//	-----------------------------------------------------------------------------------------------------------
//Big array of joint positions.  All robots index into this array

#define deg(a) ((int32_t) (a) * 32768 / 180)

//	-----------------------------------------------------------------------------------------------------------
//given an CObject and a gun number, return position in 3-space of gun
//fills in gun_point
int32_t CalcGunPoint (CFixVector *vGunPoint, CObject *pObj, int32_t nGun)
{
ENTER (0, 0);
	CPolyModel*	pm = gameData.modelData.polyModels [0] + pObj->ModelId ();
	CFixVector*	vGunPoints, vGunPos, vRot;
	CFixMatrix	m;
	int32_t		nSubModel;				//submodel number

//Assert(pObj->info.nId < gameData.botData.nTypes [gameStates.app.bD1Data]);

tRobotInfo*	pRobotInfo = ROBOTINFO (pObj);
if (!pRobotInfo)
	RETVAL (0)
if (!(vGunPoints = GetGunPoints (pObj, nGun)))
	RETVAL (0)
vGunPos = vGunPoints [nGun];
nSubModel = pRobotInfo->gunSubModels [nGun];
//instance up the tree for this gun
while (nSubModel != 0) {
	m = CFixMatrix::Create (pObj->rType.polyObjInfo.animAngles [nSubModel]);
	CFixMatrix::Transpose (m);
	vRot = m * vGunPos;
	vGunPos = vRot + pm->SubModels ().offsets [nSubModel];
	nSubModel = pm->SubModels ().parents [nSubModel];
	}
//now instance for the entire CObject
//VmVecInc (&vGunPos, gameData.modelData.offsets + pRobotInfo->nModel);
*vGunPoint = *pObj->View (0) * vGunPos;
*vGunPoint += pObj->info.position.vPos;
RETVAL (1)
}

//	-----------------------------------------------------------------------------------------------------------
//fills in ptr to list of joints, and returns the number of joints in list
//takes the robot nType (CObject id), gun number, and desired state
int32_t RobotGetAnimState (tJointPos **jointPosP, int32_t robotType, int32_t nGun, int32_t state)
{
ENTER (0, 0);
tRobotInfo*	pRobotInfo = ROBOTINFO (robotType);
if (!pRobotInfo)
	RETVAL (0)

int32_t nJoints = pRobotInfo->animStates [nGun][state].n_joints;

if (nJoints <= 0)
	memset (jointPosP, 0, sizeof (*jointPosP));
else
	*jointPosP = &gameData.botData.joints [pRobotInfo->animStates [nGun][state].offset];
RETVAL (nJoints)
}


//	-----------------------------------------------------------------------------------------------------------
//for test, set a robot to a specific state
void SetRobotState (CObject *pObj, int32_t state)
{
ENTER (0, 0);
	int32_t		g, j, jo;
	jointlist*	jl;

tRobotInfo*	pRobotInfo = ROBOTINFO (pObj);
if (!pRobotInfo)
	RETURN;
for (g = 0; g < pRobotInfo->nGuns + 1; g++) {
	jl = &pRobotInfo->animStates [g][state];
	jo = jl->offset;
	for (j = 0; j < jl->n_joints; j++, jo++) {
		int32_t jn = gameData.botData.joints [jo].jointnum;
		pObj->rType.polyObjInfo.animAngles [jn] = gameData.botData.joints [jo].angles;
		}
	}
RETURN
}

//	-----------------------------------------------------------------------------------------------------------
//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void SetRobotAngles (tRobotInfo *pRobotInfo, CPolyModel* pModel, CAngleVector angs [N_ANIM_STATES][MAX_SUBMODELS])
{
ENTER (0, 0);
	int32_t m, g, state;
	int32_t nGunCounts [MAX_SUBMODELS];			//which gun each submodel is part of

for (m = 0; m < pModel->ModelCount (); m++)
	nGunCounts [m] = pRobotInfo->nGuns;		//assume part of body...
nGunCounts [0] = -1;		//body never animates, at least for now

for (g = 0; g < pRobotInfo->nGuns; g++) {
	m = pRobotInfo->gunSubModels [g];
	while (m != 0) {
		nGunCounts [m] = g;				//...unless we find it in a gun
		m = pModel->SubModels ().parents [m];
		}
	}

for (g = 0; g < pRobotInfo->nGuns + 1; g++) {
	for (state = 0; state < N_ANIM_STATES; state++) {
		pRobotInfo->animStates [g][state].n_joints = 0;
		pRobotInfo->animStates [g][state].offset = gameData.botData.nJoints;
		for (m = 0; m < pModel->ModelCount (); m++) {
			if (nGunCounts [m] == g) {
				gameData.botData.joints [gameData.botData.nJoints].jointnum = m;
				gameData.botData.joints [gameData.botData.nJoints].angles = angs [state][m];
				pRobotInfo->animStates [g][state].n_joints++;
				gameData.botData.nJoints++;
				Assert(gameData.botData.nJoints < MAX_ROBOT_JOINTS);
				}
			}
		}
	}
RETURN
}

//	-----------------------------------------------------------------------------------------------------------

void InitCamBots (int32_t bReset)
{
ENTER (0, 0);
	tRobotInfo&	camBotInfo = gameData.botData.info [0][gameData.botData.nCamBotId];
	CObject*		pObj;

if ((gameData.botData.nCamBotId < 0) || gameStates.app.bD1Mission)
	RETURN;
camBotInfo.nModel = gameData.botData.nCamBotModel;
camBotInfo.attackType = 0;
camBotInfo.containsId = 0;
camBotInfo.containsCount = 0;
camBotInfo.containsProb = 0;
camBotInfo.containsType = 0;
camBotInfo.scoreValue = 0;
camBotInfo.strength = -1;
camBotInfo.mass = I2X (1) / 2;
camBotInfo.drag = 0;
camBotInfo.seeSound = 0;
camBotInfo.attackSound = 0;
camBotInfo.clawSound = 0;
camBotInfo.tauntSound = 0;
camBotInfo.behavior = AIB_STILL;
camBotInfo.aim = AIM_IDLING;
memset (camBotInfo.turnTime, 0, sizeof (camBotInfo.turnTime));
memset (camBotInfo.xMaxSpeed, 0, sizeof (camBotInfo.xMaxSpeed));
memset (camBotInfo.circleDistance, 0, sizeof (camBotInfo.circleDistance));
memset (camBotInfo.nRapidFireCount, 0, sizeof (camBotInfo.nRapidFireCount));
FORALL_STATIC_OBJS (pObj) 
	if (pObj->info.nType == OBJ_CAMBOT) {
		pObj->info.nId	= gameData.botData.nCamBotId;
		pObj->AdjustSize ();
		pObj->SetLife (IMMORTAL_TIME);
		pObj->info.controlType = CT_CAMERA;
		pObj->info.movementType = MT_NONE;
		pObj->rType.polyObjInfo.nModel = gameData.botData.nCamBotModel;
		gameData.aiData.localInfo [pObj->Index ()].mode = AIM_IDLING;
		}
	else if (pObj->info.nType == OBJ_EFFECT) {
		pObj->SetSize (0);
		pObj->SetLife (IMMORTAL_TIME);
		pObj->info.movementType = MT_NONE;
		if (pObj->info.controlType != CT_WAYPOINT)
			pObj->info.controlType = CT_NONE;
		}
RETURN
}

//	-----------------------------------------------------------------------------------------------------------

void UnloadCamBot (void)
{
ENTER (0, 0);
if (gameData.botData.nCamBotId >= 0) {
	gameData.botData.nTypes [0] = gameData.botData.nCamBotId;
	gameData.botData.nCamBotId = -1;
	}
RETURN
}

//	-----------------------------------------------------------------------------------------------------------

/*
 * reads n jointlist structs from a CFile
 */
static int32_t ReadJointLists (jointlist *jl, int32_t n, CFile& cf)
{
	int32_t i;

for (i = 0; i < n; i++) {
	jl [i].n_joints = cf.ReadShort ();
	jl [i].offset = cf.ReadShort ();
	if (jl [i].n_joints <= 0) {
		jl [i].n_joints = 0;
		jl [i].offset = 0;
		}
	}
return i;
}

//	-----------------------------------------------------------------------------------------------------------
/*
 * reads n tRobotInfo structs from a CFile
 */
int32_t ReadRobotInfos (CArray<tRobotInfo>& botInfo, int32_t n, CFile& cf, int32_t o)
{
ENTER (0, 0);
	int32_t h, i, j;

for (i = 0; i < n; i++) {
	h = i + o;
	botInfo [h].nModel = cf.ReadInt ();
	for (j = 0; j < MAX_GUNS; j++)
		cf.ReadVector (botInfo [h].gunPoints [j]);
	cf.Read (botInfo [h].gunSubModels, MAX_GUNS, 1);

	botInfo [h].nExp1VClip = cf.ReadShort ();
	botInfo [h].nExp1Sound = cf.ReadShort ();
	botInfo [h].nExp2VClip = cf.ReadShort ();
	botInfo [h].nExp2Sound = cf.ReadShort ();
	botInfo [h].nWeaponType = cf.ReadByte ();
	botInfo [h].nSecWeaponType = cf.ReadByte ();
	botInfo [h].nGuns = cf.ReadByte ();
	botInfo [h].containsId = cf.ReadByte ();
	botInfo [h].containsCount = cf.ReadByte ();
	botInfo [h].containsProb = cf.ReadByte ();
	botInfo [h].containsType = cf.ReadByte ();
	botInfo [h].kamikaze = cf.ReadByte ();
	botInfo [h].scoreValue = cf.ReadShort ();
	botInfo [h].splashDamage = cf.ReadByte ();
	botInfo [h].energyDrain = cf.ReadByte ();
	botInfo [h].lighting = cf.ReadFix ();
	botInfo [h].strength = cf.ReadFix ();
	botInfo [h].mass = cf.ReadFix ();
	botInfo [h].drag = cf.ReadFix ();

	for (j = 0; j < DIFFICULTY_LEVEL_COUNT; j++)
		botInfo [h].fieldOfView [j] = cf.ReadFix ();
	for (j = 0; j < DIFFICULTY_LEVEL_COUNT; j++)
		botInfo [h].primaryFiringWait [j] = cf.ReadFix ();
	for (j = 0; j < DIFFICULTY_LEVEL_COUNT; j++)
		botInfo [h].secondaryFiringWait [j] = cf.ReadFix ();
	for (j = 0; j < DIFFICULTY_LEVEL_COUNT; j++)
		botInfo [h].turnTime [j] = cf.ReadFix ();
	for (j = 0; j < DIFFICULTY_LEVEL_COUNT; j++)
		botInfo [h].xMaxSpeed [j] = cf.ReadFix ();
	for (j = 0; j < DIFFICULTY_LEVEL_COUNT; j++)
		botInfo [h].circleDistance [j] = cf.ReadFix ();
	cf.Read (botInfo [h].nRapidFireCount, DIFFICULTY_LEVEL_COUNT, 1);
	cf.Read (botInfo [h].evadeSpeed, DIFFICULTY_LEVEL_COUNT, 1);
	botInfo [h].cloakType = cf.ReadByte ();
	botInfo [h].attackType = cf.ReadByte ();
	botInfo [h].seeSound = cf.ReadByte ();
	botInfo [h].attackSound = cf.ReadByte ();
	botInfo [h].clawSound = cf.ReadByte ();
	botInfo [h].tauntSound = cf.ReadByte ();
	botInfo [h].bossFlag = cf.ReadByte ();
	if (!(botInfo [h].bEndsLevel = (botInfo [h].bossFlag > 0)))
		botInfo [h].bossFlag = -botInfo [h].bossFlag;
	botInfo [h].companion = cf.ReadByte ();
	botInfo [h].smartBlobs = cf.ReadByte ();
	botInfo [h].energyBlobs = cf.ReadByte ();
	botInfo [h].thief = cf.ReadByte ();
	botInfo [h].pursuit = cf.ReadByte ();
	botInfo [h].lightcast = cf.ReadByte ();
	botInfo [h].bDeathRoll = cf.ReadByte ();
	botInfo [h].flags = cf.ReadByte ();
	cf.Read(botInfo [h].pad, 3, 1);
	botInfo [h].deathrollSound = cf.ReadByte ();
	botInfo [h].glow = cf.ReadByte ();
	botInfo [h].behavior = cf.ReadByte ();
	botInfo [h].aim = cf.ReadByte ();
	for (j = 0; j < MAX_GUNS + 1; j++)
		ReadJointLists (botInfo [h].animStates [j], N_ANIM_STATES, cf);
	botInfo [h].always_0xabcd = cf.ReadInt ();
	}
RETVAL (i)
}

//	-----------------------------------------------------------------------------------------------------------
/*
 * reads n tJointPos structs from a CFile
 */
int32_t ReadJointPositions (CArray<tJointPos>& jp, int32_t n, CFile& cf, int32_t o)
{
	int32_t	i;

for (i = 0; i < n; i++) {
	jp [i + o].jointnum = cf.ReadShort ();
	cf.ReadAngVec (jp [i + o].angles);
	}
return i;
}

//	-----------------------------------------------------------------------------------------------------------
//eof
