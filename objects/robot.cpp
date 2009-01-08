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

//	-----------------------------------------------------------------------------------------------------------
//given an CObject and a gun number, return position in 3-space of gun
//fills in gun_point
int CalcGunPoint (CFixVector *vGunPoint, CObject *objP, int nGun)
{
	CPolyModel	*pm = gameData.models.polyModels [0] + objP->rType.polyObjInfo.nModel;
	tRobotInfo	*botInfoP;
	CFixVector	*vGunPoints, vGunPos, vRot;
	CFixMatrix	m;
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
	m = CFixMatrix::Create(objP->rType.polyObjInfo.animAngles [nSubModel]);
	CFixMatrix::Transpose(m);
	vRot = m * vGunPos;
	vGunPos = vRot + pm->SubModels ().offsets[nSubModel];
	nSubModel = pm->SubModels ().parents [nSubModel];
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
void setRobotState (CObject *objP, int state)
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
void SetRobotAngles (tRobotInfo *botInfoP, CPolyModel* modelP, CAngleVector angs [N_ANIM_STATES][MAX_SUBMODELS])
{
	int m,g,state;
	int nGunCounts [MAX_SUBMODELS];			//which gun each submodel is part of

for (m = 0; m < modelP->ModelCount ();m++)
	nGunCounts [m] = botInfoP->nGuns;		//assume part of body...
nGunCounts [0] = -1;		//body never animates, at least for now

for (g = 0; g < botInfoP->nGuns; g++) {
	m = botInfoP->gunSubModels [g];
	while (m != 0) {
		nGunCounts [m] = g;				//...unless we find it in a gun
		m = modelP->SubModels ().parents [m];
		}
	}

for (g = 0; g < botInfoP->nGuns + 1; g++) {
	for (state = 0; state <N_ANIM_STATES; state++) {
		botInfoP->animStates [g][state].n_joints = 0;
		botInfoP->animStates [g][state].offset = gameData.bots.nJoints;
		for (m = 0; m < modelP->ModelCount (); m++) {
			if (nGunCounts[m] == g) {
				gameData.bots.joints [gameData.bots.nJoints].jointnum = m;
				gameData.bots.joints [gameData.bots.nJoints].angles = angs[state][m];
				botInfoP->animStates [g][state].n_joints++;
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
FORALL_STATIC_OBJS (objP, i) 
	if (objP->info.nType == OBJ_CAMBOT) {
		objP->info.nId	= gameData.bots.nCamBotId;
		objP->info.xSize = gameData.models.polyModels [0][gameData.bots.nCamBotModel].Size ();
		objP->info.xLifeLeft = IMMORTAL_TIME;
		objP->info.controlType = CT_CAMERA;
		objP->info.movementType = MT_NONE;
		objP->rType.polyObjInfo.nModel = gameData.bots.nCamBotModel;
		gameData.ai.localInfo [objP->Index ()].mode = AIM_IDLING;
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
static int ReadJointLists (jointlist *jl, int n, CFile& cf)
{
	int i;

for (i = 0; i < n; i++) {
	jl [i].n_joints = cf.ReadShort ();
	jl [i].offset = cf.ReadShort ();
	}
return i;
}

//	-----------------------------------------------------------------------------------------------------------
/*
 * reads n tRobotInfo structs from a CFile
 */
int ReadRobotInfos (CArray<tRobotInfo>& pri, int n, CFile& cf, int o)
{
	int h, i, j;

for (i = 0; i < n; i++) {
	h = i + o;
	pri [h].nModel = cf.ReadInt ();
	for (j = 0; j < MAX_GUNS; j++)
		cf.ReadVector(pri [h].gunPoints[j]);
	cf.Read(pri [h].gunSubModels, MAX_GUNS, 1);

	pri [h].nExp1VClip = cf.ReadShort ();
	pri [h].nExp1Sound = cf.ReadShort ();
	pri [h].nExp2VClip = cf.ReadShort ();
	pri [h].nExp2Sound = cf.ReadShort ();
	pri [h].nWeaponType = cf.ReadByte ();
	pri [h].nSecWeaponType = cf.ReadByte ();
	pri [h].nGuns = cf.ReadByte ();
	pri [h].containsId = cf.ReadByte ();
	pri [h].containsCount = cf.ReadByte ();
	pri [h].containsProb = cf.ReadByte ();
	pri [h].containsType = cf.ReadByte ();
	pri [h].kamikaze = cf.ReadByte ();
	pri [h].scoreValue = cf.ReadShort ();
	pri [h].badass = cf.ReadByte ();
	pri [h].energyDrain = cf.ReadByte ();
	pri [h].lighting = cf.ReadFix ();
	pri [h].strength = cf.ReadFix ();
	pri [h].mass = cf.ReadFix ();
	pri [h].drag = cf.ReadFix ();

	for (j = 0; j < NDL; j++)
		pri [h].fieldOfView[j] = cf.ReadFix ();
	for (j = 0; j < NDL; j++)
		pri [h].primaryFiringWait[j] = cf.ReadFix ();
	for (j = 0; j < NDL; j++)
		pri [h].secondaryFiringWait[j] = cf.ReadFix ();
	for (j = 0; j < NDL; j++)
		pri [h].turnTime [j] = cf.ReadFix ();
	for (j = 0; j < NDL; j++)
		pri [h].xMaxSpeed[j] = cf.ReadFix ();
	for (j = 0; j < NDL; j++)
		pri [h].circleDistance [j] = cf.ReadFix ();
	cf.Read(pri [h].nRapidFireCount, NDL, 1);
	cf.Read(pri [h].evadeSpeed, NDL, 1);
	pri [h].cloakType = cf.ReadByte ();
	pri [h].attackType = cf.ReadByte ();
	pri [h].seeSound = cf.ReadByte ();
	pri [h].attackSound = cf.ReadByte ();
	pri [h].clawSound = cf.ReadByte ();
	pri [h].tauntSound = cf.ReadByte ();
	pri [h].bossFlag = cf.ReadByte ();
	pri [h].companion = cf.ReadByte ();
	pri [h].smartBlobs = cf.ReadByte ();
	pri [h].energyBlobs = cf.ReadByte ();
	pri [h].thief = cf.ReadByte ();
	pri [h].pursuit = cf.ReadByte ();
	pri [h].lightcast = cf.ReadByte ();
	pri [h].bDeathRoll = cf.ReadByte ();
	pri [h].flags = cf.ReadByte ();
	cf.Read(pri [h].pad, 3, 1);
	pri [h].deathrollSound = cf.ReadByte ();
	pri [h].glow = cf.ReadByte ();
	pri [h].behavior = cf.ReadByte ();
	pri [h].aim = cf.ReadByte ();
	for (j = 0; j < MAX_GUNS + 1; j++)
		ReadJointLists (pri [h].animStates[j], N_ANIM_STATES, cf);
	pri [h].always_0xabcd = cf.ReadInt ();
	}
return i;
}

//	-----------------------------------------------------------------------------------------------------------
/*
 * reads n tJointPos structs from a CFile
 */
int ReadJointPositions (CArray<tJointPos>& jp, int n, CFile& cf, int o)
{
	int i;

	for (i = 0; i < n; i++) {
		jp [i + o].jointnum = cf.ReadShort ();
		cf.ReadAngVec (jp [i].angles);
	}
	return i;
}
#endif

//	-----------------------------------------------------------------------------------------------------------
//eof
