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

#include "inferno.h"
#include "error.h"
#include "interp.h"

//	-----------------------------------------------------------------------------------------------------------
//Big array of joint positions.  All robots index into this array

#define deg(a) ((int) (a) * 32768 / 180)

#if 0
//test data for one robot
tJointPos test_joints [MAX_ROBOT_JOINTS] = {

//gun 0
	{2,vmsAngVec::Create(deg(-30),0,0)},         //rest (2 joints)
	{3,vmsAngVec::Create(deg(-40),0,0)},

	{2,vmsAngVec::Create(deg(0),0,0)},           //alert
	{3,vmsAngVec::Create(deg(0),0,0)},

	{2,vmsAngVec::Create(deg(0),0,0)},           //fire
	{3,vmsAngVec::Create(deg(0),0,0)},

	{2,vmsAngVec::Create(deg(50),0,0)},          //recoil
	{3,vmsAngVec::Create(deg(-50),0,0)},

	{2,vmsAngVec::Create(deg(10),0,deg(70))},    //flinch
	{3,vmsAngVec::Create(deg(0),deg(20),0)},

//gun 1
	{4,vmsAngVec::Create(deg(-30),0,0)},         //rest (2 joints)
	{5,vmsAngVec::Create(deg(-40),0,0)},

	{4,vmsAngVec::Create(deg(0),0,0)},           //alert
	{5,vmsAngVec::Create(deg(0),0,0)},

	{4,vmsAngVec::Create(deg(0),0,0)},           //fire
	{5,vmsAngVec::Create(deg(0),0,0)},

	{4,vmsAngVec::Create(deg(50),0,0)},          //recoil
	{5,vmsAngVec::Create(deg(-50),0,0)},

	{4,vmsAngVec::Create(deg(20),0,deg(-50))},   //flinch
	{5,vmsAngVec::Create(deg(0),0,deg(20))},

//rest of body (the head)

	{1,vmsAngVec::Create(deg(70),0,0)},          //rest (1 joint, head)

	{1,vmsAngVec::Create(deg(0),0,0)},           //alert

	{1,vmsAngVec::Create(deg(0),0,0)},           //fire

	{1,vmsAngVec::Create(deg(0),0,0)},           //recoil

	{1,vmsAngVec::Create(deg(-20),deg(15),0)},   //flinch

};
#endif

//	-----------------------------------------------------------------------------------------------------------
//given an CObject and a gun number, return position in 3-space of gun
//fills in gun_point
int CalcGunPoint (vmsVector *vGunPoint, CObject *objP, int nGun)
{
	tPolyModel	*pm = gameData.models.polyModels + objP->rType.polyObjInfo.nModel;
	tRobotInfo	*botInfoP;
	vmsVector	*vGunPoints, vGunPos, vRot;
	vmsMatrix	m;
	int			nSubModel;				//submodel number

Assert(objP->info.renderType == RT_POLYOBJ || objP->info.renderType == RT_MORPH);
//Assert(objP->info.nId < gameData.bots.nTypes [gameStates.app.bD1Data]);

botInfoP = &ROBOTINFO (objP->info.nId);
if (!(vGunPoints = GetGunPoints (objP, nGun)))
	return 0;
vGunPos = vGunPoints [nGun];
nSubModel = botInfoP->gunSubModels [nGun];
//instance up the tree for this gun
while (nSubModel != 0) {
	m = vmsMatrix::Create(objP->rType.polyObjInfo.animAngles [nSubModel]);
	vmsMatrix::Transpose(m);
	vRot = m * vGunPos;
	vGunPos = vRot + pm->subModels.offsets[nSubModel];
	nSubModel = pm->subModels.parents [nSubModel];
	}
//now instance for the entire CObject
//VmVecInc (&vGunPos, gameData.models.offsets + botInfoP->nModel);
*vGunPoint = *ObjectView(objP) * vGunPos;
*vGunPoint += objP->info.position.vPos;
return 1;
}

//	-----------------------------------------------------------------------------------------------------------
//fills in ptr to list of joints, and returns the number of joints in list
//takes the robot nType (CObject id), gun number, and desired state
int RobotGetAnimState (tJointPos **jointPosP, int robotType, int nGun, int state)
{
Assert(nGun <= ROBOTINFO (robotType).nGuns);
*jointPosP = &gameData.bots.joints [ROBOTINFO (robotType).animStates[nGun][state].offset];
return ROBOTINFO (robotType).animStates[nGun][state].n_joints;
}


//	-----------------------------------------------------------------------------------------------------------
//for test, set a robot to a specific state
void setRobotState(CObject *objP,int state)
{
	int g,j,jo;
	tRobotInfo *ri;
	jointlist *jl;

	Assert(objP->info.nType == OBJ_ROBOT);

	ri = &ROBOTINFO (objP->info.nId);

	for (g=0;g<ri->nGuns+1;g++) {

		jl = &ri->animStates[g][state];

		jo = jl->offset;

		for (j=0;j<jl->n_joints;j++,jo++) {
			int jn;

			jn = gameData.bots.joints[jo].jointnum;

			objP->rType.polyObjInfo.animAngles[jn] = gameData.bots.joints[jo].angles;

		}
	}
}

//	-----------------------------------------------------------------------------------------------------------
//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void robot_set_angles(tRobotInfo *r,tPolyModel *pm,vmsAngVec angs[N_ANIM_STATES][MAX_SUBMODELS])
{
	int m,g,state;
	int nGunCounts [MAX_SUBMODELS];			//which gun each submodel is part of

for (m = 0; m < pm->nModels;m++)
	nGunCounts [m] = r->nGuns;		//assume part of body...
nGunCounts [0] = -1;		//body never animates, at least for now

for (g = 0; g < r->nGuns; g++) {
	m = r->gunSubModels [g];
	while (m != 0) {
		nGunCounts [m] = g;				//...unless we find it in a gun
		m = pm->subModels.parents [m];
		}
	}

for (g = 0; g < r->nGuns + 1; g++) {
	for (state = 0; state <N_ANIM_STATES; state++) {
		r->animStates [g][state].n_joints = 0;
		r->animStates [g][state].offset = gameData.bots.nJoints;
		for (m = 0; m < pm->nModels; m++) {
			if (nGunCounts[m] == g) {
				gameData.bots.joints [gameData.bots.nJoints].jointnum = m;
				gameData.bots.joints [gameData.bots.nJoints].angles = angs[state][m];
				r->animStates [g][state].n_joints++;
				gameData.bots.nJoints++;
				Assert(gameData.bots.nJoints < MAX_ROBOT_JOINTS);
				}
			}
		}
	}
}

//	-----------------------------------------------------------------------------------------------------------

void InitCamBots (int bReset)
{
	tRobotInfo&	camBotInfo = gameData.bots.info [0][gameData.bots.nCamBotId];
	CObject		*objP;
	int			i;

if ((gameData.bots.nCamBotId < 0) || gameStates.app.bD1Mission)
	return;
camBotInfo.nModel = gameData.bots.nCamBotModel;
camBotInfo.attackType = 0;
camBotInfo.containsId = 0;
camBotInfo.containsCount = 0;
camBotInfo.containsProb = 0;
camBotInfo.containsType = 0;
camBotInfo.scoreValue = 0;
camBotInfo.strength = -1;
camBotInfo.mass = F1_0 / 2;
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
FORALL_STATIC_OBJS (objP, i) 
	if (objP->info.nType == OBJ_CAMBOT) {
		objP->info.nId	= gameData.bots.nCamBotId;
		objP->info.xSize = G3PolyModelSize (gameData.models.polyModels + gameData.bots.nCamBotModel, gameData.bots.nCamBotModel);
		objP->info.xLifeLeft = IMMORTAL_TIME;
		objP->info.controlType = CT_CAMERA;
		objP->info.movementType = MT_NONE;
		objP->rType.polyObjInfo.nModel = gameData.bots.nCamBotModel;
		gameData.ai.localInfo [OBJ_IDX (objP)].mode = AIM_IDLING;
		}
	else if (objP->info.nType == OBJ_EFFECT) {
		objP->info.xSize = 0;
		objP->info.xLifeLeft = IMMORTAL_TIME;
		objP->info.controlType = CT_NONE;
		objP->info.movementType = MT_NONE;
		}
}

//	-----------------------------------------------------------------------------------------------------------

void UnloadCamBot (void)
{
if (gameData.bots.nCamBotId >= 0) {
	gameData.bots.nTypes [0] = gameData.bots.nCamBotId;
	gameData.bots.nCamBotId = -1;
	}
}

//	-----------------------------------------------------------------------------------------------------------

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads n jointlist structs from a CFile
 */
static int JointListReadN(jointlist *jl, int n, CFile& cf)
{
	int i;

	for (i = 0; i < n; i++) {
		jl[i].n_joints = cf.ReadShort ();
		jl[i].offset = cf.ReadShort ();
	}
	return i;
}

//	-----------------------------------------------------------------------------------------------------------
/*
 * reads n tRobotInfo structs from a CFile
 */
int RobotInfoReadN (CArray<tRobotInfo>& pri, int n, CFile& cf)
{
	int i, j;

for (i = 0; i < n; i++) {
	pri [i].nModel = cf.ReadInt ();
	for (j = 0; j < MAX_GUNS; j++)
		cf.ReadVector(pri [i].gunPoints[j]);
	cf.Read(pri [i].gunSubModels, MAX_GUNS, 1);

	pri [i].nExp1VClip = cf.ReadShort ();
	pri [i].nExp1Sound = cf.ReadShort ();
	pri [i].nExp2VClip = cf.ReadShort ();
	pri [i].nExp2Sound = cf.ReadShort ();
	pri [i].nWeaponType = cf.ReadByte ();
	pri [i].nSecWeaponType = cf.ReadByte ();
	pri [i].nGuns = cf.ReadByte ();
	pri [i].containsId = cf.ReadByte ();
	pri [i].containsCount = cf.ReadByte ();
	pri [i].containsProb = cf.ReadByte ();
	pri [i].containsType = cf.ReadByte ();
	pri [i].kamikaze = cf.ReadByte ();
	pri [i].scoreValue = cf.ReadShort ();
	pri [i].badass = cf.ReadByte ();
	pri [i].energyDrain = cf.ReadByte ();
	pri [i].lighting = cf.ReadFix ();
	pri [i].strength = cf.ReadFix ();
	pri [i].mass = cf.ReadFix ();
	pri [i].drag = cf.ReadFix ();

	for (j = 0; j < NDL; j++)
		pri [i].fieldOfView[j] = cf.ReadFix ();
	for (j = 0; j < NDL; j++)
		pri [i].primaryFiringWait[j] = cf.ReadFix ();
	for (j = 0; j < NDL; j++)
		pri [i].secondaryFiringWait[j] = cf.ReadFix ();
	for (j = 0; j < NDL; j++)
		pri [i].turnTime[j] = cf.ReadFix ();
	for (j = 0; j < NDL; j++)
		pri [i].xMaxSpeed[j] = cf.ReadFix ();
	for (j = 0; j < NDL; j++)
		pri [i].circleDistance[j] = cf.ReadFix ();
	cf.Read(pri [i].nRapidFireCount, NDL, 1);
	cf.Read(pri [i].evadeSpeed, NDL, 1);
	pri [i].cloakType = cf.ReadByte ();
	pri [i].attackType = cf.ReadByte ();
	pri [i].seeSound = cf.ReadByte ();
	pri [i].attackSound = cf.ReadByte ();
	pri [i].clawSound = cf.ReadByte ();
	pri [i].tauntSound = cf.ReadByte ();
	pri [i].bossFlag = cf.ReadByte ();
	pri [i].companion = cf.ReadByte ();
	pri [i].smartBlobs = cf.ReadByte ();
	pri [i].energyBlobs = cf.ReadByte ();
	pri [i].thief = cf.ReadByte ();
	pri [i].pursuit = cf.ReadByte ();
	pri [i].lightcast = cf.ReadByte ();
	pri [i].bDeathRoll = cf.ReadByte ();
	pri [i].flags = cf.ReadByte ();
	cf.Read(pri [i].pad, 3, 1);
	pri [i].deathrollSound = cf.ReadByte ();
	pri [i].glow = cf.ReadByte ();
	pri [i].behavior = cf.ReadByte ();
	pri [i].aim = cf.ReadByte ();
	for (j = 0; j < MAX_GUNS + 1; j++)
		JointListReadN (pri [i].animStates[j], N_ANIM_STATES, cf);
	pri [i].always_0xabcd = cf.ReadInt ();
	}
return i;
}

//	-----------------------------------------------------------------------------------------------------------
/*
 * reads n tJointPos structs from a CFile
 */
int JointPosReadN (CArray<tJointPos>& jp, int n, CFile& cf)
{
	int i;

	for (i = 0; i < n; i++) {
		jp [i].jointnum = cf.ReadShort ();
		cf.ReadAngVec (jp [i].angles);
	}
	return i;
}
#endif

//	-----------------------------------------------------------------------------------------------------------
//eof
