/* $Id: robot.c,v 1.5 2003/10/10 09:36:35 btb Exp $ */
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

#include "error.h"

#include "inferno.h"

#include "robot.h"
#include "object.h"
#include "polyobj.h"
#include "mono.h"
#include "ai.h"
#include "interp.h"

//	-----------------------------------------------------------------------------------------------------------
//Big array of joint positions.  All robots index into this array

#define deg(a) ((int) (a) * 32768 / 180)

//test data for one robot
tJointPos test_joints [MAX_ROBOT_JOINTS] = {

//gun 0
	{2,{deg(-30),0,0}},         //rest (2 joints)
	{3,{deg(-40),0,0}},

	{2,{deg(0),0,0}},           //alert
	{3,{deg(0),0,0}},

	{2,{deg(0),0,0}},           //fire
	{3,{deg(0),0,0}},

	{2,{deg(50),0,0}},          //recoil
	{3,{deg(-50),0,0}},

	{2,{deg(10),0,deg(70)}},    //flinch
	{3,{deg(0),deg(20),0}},

//gun 1
	{4,{deg(-30),0,0}},         //rest (2 joints)
	{5,{deg(-40),0,0}},

	{4,{deg(0),0,0}},           //alert
	{5,{deg(0),0,0}},

	{4,{deg(0),0,0}},           //fire
	{5,{deg(0),0,0}},

	{4,{deg(50),0,0}},          //recoil
	{5,{deg(-50),0,0}},

	{4,{deg(20),0,deg(-50)}},   //flinch
	{5,{deg(0),0,deg(20)}},

//rest of body (the head)

	{1,{deg(70),0,0}},          //rest (1 joint, head)

	{1,{deg(0),0,0}},           //alert

	{1,{deg(0),0,0}},           //fire

	{1,{deg(0),0,0}},           //recoil

	{1,{deg(-20),deg(15),0}},   //flinch

};

//	-----------------------------------------------------------------------------------------------------------
//given an tObject and a gun number, return position in 3-space of gun
//fills in gun_point
int CalcGunPoint (vmsVector *vGunPoint, tObject *objP, int nGun)
{
	tPolyModel	*pm;
	tRobotInfo	*botInfoP;
	vmsVector	*vGunPoints, vGunPos;
	vmsMatrix	m;
	int			nSubModel, bCustom = 0;				//submodel number

Assert(objP->renderType == RT_POLYOBJ || objP->renderType==RT_MORPH);
//Assert(objP->id < gameData.bots.nTypes [gameStates.app.bD1Data]);

botInfoP = &ROBOTINFO (objP->id);
if (!(vGunPoints = GetGunPoints (objP, nGun)))
	return 0;
vGunPos = vGunPoints [nGun];
nSubModel = botInfoP->gunSubModels [nGun];
//instance up the tree for this gun
while (nSubModel != 0) {
	vmsVector vRot;

	VmAngles2Matrix (&m, &objP->rType.polyObjInfo.animAngles [nSubModel]);
	VmTransposeMatrix (&m);
	VmVecRotate (&vRot, &vGunPos, &m);
	VmVecAdd (&vGunPos, &vRot, &gameData.models.polyModels [objP->rType.polyObjInfo.nModel].subModels.offsets [nSubModel]);
	nSubModel = pm->subModels.parents [nSubModel];
	}
//now instance for the entire tObject
VmVecRotate (vGunPoint, &vGunPos, ObjectView (objP));
VmVecInc (vGunPoint, &objP->position.vPos);
return 1;
}

//	-----------------------------------------------------------------------------------------------------------
//fills in ptr to list of joints, and returns the number of joints in list
//takes the robot nType (tObject id), gun number, and desired state
int robot_get_animState(tJointPos **jp_list_ptr,int robotType,int gun_num,int state)
{
Assert(gun_num <= ROBOTINFO (robotType).nGuns);
*jp_list_ptr = &gameData.bots.joints [ROBOTINFO (robotType).animStates[gun_num][state].offset];
return ROBOTINFO (robotType).animStates[gun_num][state].n_joints;
}


//	-----------------------------------------------------------------------------------------------------------
//for test, set a robot to a specific state
void setRobotState(tObject *objP,int state)
{
	int g,j,jo;
	tRobotInfo *ri;
	jointlist *jl;

	Assert(objP->nType == OBJ_ROBOT);

	ri = &ROBOTINFO (objP->id);

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
#include "mono.h"

//--unused-- int curState=0;

//--unused-- test_animStates()
//--unused-- {
//--unused-- 	setRobotState(&gameData.objs.objects[1],curState);
//--unused--
//--unused--
//--unused-- 	curState = (curState+1)%N_ANIM_STATES;
//--unused--
//--unused-- }

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void robot_set_angles(tRobotInfo *r,tPolyModel *pm,vmsAngVec angs[N_ANIM_STATES][MAX_SUBMODELS])
{
	int m,g,state;
	int gun_nums[MAX_SUBMODELS];			//which gun each submodel is part of

	for (m=0;m<pm->nModels;m++)
		gun_nums[m] = r->nGuns;		//assume part of body...

	gun_nums[0] = -1;		//body never animates, at least for now

	for (g=0;g<r->nGuns;g++) {
		m = r->gunSubModels[g];

		while (m != 0) {
			gun_nums[m] = g;				//...unless we find it in a gun
			m = pm->subModels.parents[m];
		}
	}

	for (g=0;g<r->nGuns+1;g++) {

		for (state=0;state<N_ANIM_STATES;state++) {

			r->animStates[g][state].n_joints = 0;
			r->animStates[g][state].offset = gameData.bots.nJoints;

			for (m=0;m<pm->nModels;m++) {
				if (gun_nums[m] == g) {
					gameData.bots.joints[gameData.bots.nJoints].jointnum = m;
					gameData.bots.joints[gameData.bots.nJoints].angles = angs[state][m];
					r->animStates[g][state].n_joints++;
					gameData.bots.nJoints++;
					Assert(gameData.bots.nJoints < MAX_ROBOT_JOINTS);
				}
			}
		}
	}
}

//	-----------------------------------------------------------------------------------------------------------

#define DEG90	(F1_0 / 4)

void InitCamBots (int bReset)
{
	tObject		*objP = gameData.objs.objects;
	int			i;

if ((gameData.bots.nCamBotId < 0) || gameStates.app.bD1Mission)
	return;
gameData.bots.info [0][gameData.bots.nCamBotId].nModel = gameData.bots.nCamBotModel;
gameData.bots.info [0][gameData.bots.nCamBotId].attackType = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].containsId = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].containsCount = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].containsProb = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].containsType = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].scoreValue = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].strength = -1;
gameData.bots.info [0][gameData.bots.nCamBotId].mass = F1_0 / 2;
gameData.bots.info [0][gameData.bots.nCamBotId].drag = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].seeSound = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].attackSound = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].clawSound = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].tauntSound = 0;
gameData.bots.info [0][gameData.bots.nCamBotId].behavior = AIB_STILL;
gameData.bots.info [0][gameData.bots.nCamBotId].aim = AIM_IDLING;
memset (gameData.bots.info [0][gameData.bots.nCamBotId].turnTime, 0, sizeof (gameData.bots.info [0][gameData.bots.nCamBotId].turnTime));
memset (gameData.bots.info [0][gameData.bots.nCamBotId].xMaxSpeed, 0, sizeof (gameData.bots.info [0][gameData.bots.nCamBotId].xMaxSpeed));
memset (gameData.bots.info [0][gameData.bots.nCamBotId].circleDistance, 0, sizeof (gameData.bots.info [0][gameData.bots.nCamBotId].circleDistance));
memset (gameData.bots.info [0][gameData.bots.nCamBotId].nRapidFireCount, 0, sizeof (gameData.bots.info [0][gameData.bots.nCamBotId].nRapidFireCount));
for (i = 0; i <= gameData.objs.nLastObject; i++, objP++)
	if (objP->nType == OBJ_CAMBOT) {
		objP->id	= gameData.bots.nCamBotId;
		objP->size = G3PolyModelSize (gameData.models.polyModels + gameData.bots.nCamBotModel, gameData.bots.nCamBotModel);
		objP->lifeleft = IMMORTAL_TIME;
		objP->controlType = CT_CAMERA;
		objP->movementType = MT_NONE;
		objP->rType.polyObjInfo.nModel = gameData.bots.nCamBotModel;
		gameData.ai.localInfo [i].mode = AIM_IDLING;
		}
	else if (objP->nType == OBJ_EFFECT) {
		objP->size = 0;
		objP->lifeleft = IMMORTAL_TIME;
		objP->controlType = CT_NONE;
		objP->movementType = MT_NONE;
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
 * reads n jointlist structs from a CFILE
 */
static int jointlist_read_n(jointlist *jl, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		jl[i].n_joints = CFReadShort(fp);
		jl[i].offset = CFReadShort(fp);
	}
	return i;
}

//	-----------------------------------------------------------------------------------------------------------
/*
 * reads n tRobotInfo structs from a CFILE
 */
int RobotInfoReadN(tRobotInfo *pri, int n, CFILE *fp)
{
	int i, j;

for (i = 0; i < n; i++, pri++) {
	pri->nModel = CFReadInt(fp);
	for (j = 0; j < MAX_GUNS; j++)
		CFReadVector(&(pri->gunPoints[j]), fp);
	CFRead(pri->gunSubModels, MAX_GUNS, 1, fp);

	pri->nExp1VClip = CFReadShort(fp);
	pri->nExp1Sound = CFReadShort(fp);

	pri->nExp2VClip = CFReadShort(fp);
	pri->nExp2Sound = CFReadShort(fp);

	pri->nWeaponType = CFReadByte(fp);
	pri->nSecWeaponType = CFReadByte(fp);
	pri->nGuns = CFReadByte(fp);
	pri->containsId = CFReadByte(fp);

	pri->containsCount = CFReadByte(fp);
	pri->containsProb = CFReadByte(fp);
	pri->containsType = CFReadByte(fp);
	pri->kamikaze = CFReadByte(fp);

	pri->scoreValue = CFReadShort(fp);
	pri->badass = CFReadByte(fp);
	pri->energyDrain = CFReadByte(fp);

	pri->lighting = CFReadFix(fp);
	pri->strength = CFReadFix(fp);

	pri->mass = CFReadFix(fp);
	pri->drag = CFReadFix(fp);

	for (j = 0; j < NDL; j++)
		pri->fieldOfView[j] = CFReadFix(fp);
	for (j = 0; j < NDL; j++)
		pri->primaryFiringWait[j] = CFReadFix(fp);
	for (j = 0; j < NDL; j++)
		pri->secondaryFiringWait[j] = CFReadFix(fp);
	for (j = 0; j < NDL; j++)
		pri->turnTime[j] = CFReadFix(fp);
	for (j = 0; j < NDL; j++)
		pri->xMaxSpeed[j] = CFReadFix(fp);
	for (j = 0; j < NDL; j++)
		pri->circleDistance[j] = CFReadFix(fp);
	CFRead(pri->nRapidFireCount, NDL, 1, fp);

	CFRead(pri->evadeSpeed, NDL, 1, fp);

	pri->cloakType = CFReadByte(fp);
	pri->attackType = CFReadByte(fp);

	pri->seeSound = CFReadByte(fp);
	pri->attackSound = CFReadByte(fp);
	pri->clawSound = CFReadByte(fp);
	pri->tauntSound = CFReadByte(fp);

	pri->bossFlag = CFReadByte(fp);
	pri->companion = CFReadByte(fp);
	pri->smartBlobs = CFReadByte(fp);
	pri->energyBlobs = CFReadByte(fp);

	pri->thief = CFReadByte(fp);
	pri->pursuit = CFReadByte(fp);
	pri->lightcast = CFReadByte(fp);
	pri->bDeathRoll = CFReadByte(fp);

	pri->flags = CFReadByte(fp);
	CFRead(pri->pad, 3, 1, fp);

	pri->deathrollSound = CFReadByte(fp);
	pri->glow = CFReadByte(fp);
	pri->behavior = CFReadByte(fp);
	pri->aim = CFReadByte(fp);

	for (j = 0; j < MAX_GUNS + 1; j++)
		jointlist_read_n(pri->animStates[j], N_ANIM_STATES, fp);

	pri->always_0xabcd = CFReadInt(fp);
	}
return i;
}

//	-----------------------------------------------------------------------------------------------------------
/*
 * reads n tJointPos structs from a CFILE
 */
int JointPosReadN(tJointPos *jp, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		jp[i].jointnum = CFReadShort(fp);
		CFReadAngVec(&jp[i].angles, fp);
	}
	return i;
}
#endif

//	-----------------------------------------------------------------------------------------------------------
//eof
