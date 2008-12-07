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

/*
 *
 * Header for robot.c
 *
 *
 */

#ifndef _ROBOT_H
#define _ROBOT_H

#include "vecmat.h"
#include "object.h"
#include "game.h"
#include "cfile.h"

#define MAX_GUNS 8      //should be multiple of 4 for ubyte array

//Animation states
#define AS_REST         0
#define AS_ALERT        1
#define AS_FIRE         2
#define AS_RECOIL       3
#define AS_FLINCH       4
#define N_ANIM_STATES   5

#define RI_CLOAKED_NEVER            0
#define RI_CLOAKED_ALWAYS           1
#define RI_CLOAKED_EXCEPT_FIRING    2

//describes the position of a certain joint
typedef struct tJointPos {
	short jointnum;
	vmsAngVec angles;
} __pack__ tJointPos;

//describes a list of joint positions
typedef struct jointlist {
	short n_joints;
	short offset;
} jointlist;

//robot info flags
#define RIF_BIG_RADIUS  1   //pad the radius to fix robots firing through walls
#define RIF_THIEF       2   //this guy steals!

//  Robot information
typedef struct tRobotInfo {
	int     nModel;                  // which polygon model?
	vmsVector  gunPoints[MAX_GUNS];   // where each gun model is
	ubyte   gunSubModels[MAX_GUNS];    // which submodel is each gun in?

	short   nExp1VClip;
	short   nExp1Sound;
	short   nExp2VClip;
	short   nExp2Sound;
	sbyte   nWeaponType;
	sbyte   nSecWeaponType; //  Secondary weapon number, -1 means none, otherwise gun #0 fires this weapon.
	sbyte   nGuns;         // how many different gun positions
	sbyte   containsId;     //  ID of powerup this robot can contain.
	sbyte   containsCount; //  Max number of things this instance can contain.
	sbyte   containsProb;  //  Probability that this instance will contain something in N/16
	sbyte   containsType;  //  Type of thing contained, robot or powerup, in bitmaps.tbl, !0=robot, 0=powerup
	sbyte   kamikaze;       //  !0 means commits suicide when hits you, strength thereof. 0 means no.
	short   scoreValue;    //  Score from this robot.
	sbyte   badass;         //  Dies with badass explosion, and strength thereof, 0 means NO.
	sbyte   energyDrain;   //  Points of energy drained at each collision.
	fix     lighting;       // should this be here or with polygon model?
	fix     strength;       // Initial shields of robot
	fix     mass;           // how heavy is this thing?
	fix     drag;           // how much drag does it have?
	fix     fieldOfView[NDL]; // compare this value with forward_vector.dot.vector_to_player, if fieldOfView <, then robot can see tPlayer
	fix     primaryFiringWait[NDL];   //  time in seconds between shots
	fix     secondaryFiringWait[NDL];  //  time in seconds between shots
	fix     turnTime[NDL];     // time in seconds to rotate 360 degrees in a dimension
// -- unused, mk, 05/25/95  fix fire_power[NDL];    //  damage done by a hit from this robot
// -- unused, mk, 05/25/95  fix shield[NDL];        //  shield strength of this robot
	fix     xMaxSpeed[NDL];         //  maximum speed attainable by this robot
	fix     circleDistance[NDL];   //  distance at which robot circles tPlayer

	sbyte   nRapidFireCount[NDL];   //  number of shots fired rapidly
	sbyte   evadeSpeed[NDL];       //  rate at which robot can evade shots, 0=none, 4=very fast
	sbyte   cloakType;     //  0=never, 1=always, 2=except-when-firing
	sbyte   attackType;    //  0=firing, 1=charge (like green guy)

	ubyte   seeSound;      //  sound robot makes when it first sees the tPlayer
	ubyte   attackSound;   //  sound robot makes when it attacks the tPlayer
	ubyte   clawSound;     //  sound robot makes as it claws you (attackType should be 1)
	ubyte   tauntSound;    //  sound robot makes after you die

	sbyte   bossFlag;      //  0 = not boss, 1 = boss.  Is that surprising?
	sbyte   companion;      //  Companion robot, leads you to things.
	sbyte   smartBlobs;    //  how many smart blobs are emitted when this guy dies!
	sbyte   energyBlobs;   //  how many smart blobs are emitted when this guy gets hit by energy weapon!

	sbyte   thief;          //  !0 means this guy can steal when he collides with you!
	sbyte   pursuit;        //  !0 means pursues tPlayer after he goes around a corner.  4 = 4/2 pursue up to 4/2 seconds after becoming invisible if up to 4 segments away
	sbyte   lightcast;      //  Amount of light cast. 1 is default.  10 is very large.
	sbyte   bDeathRoll;     //  0 = dies without death roll. !0 means does death roll, larger = faster and louder

	//bossFlag, companion, thief, & pursuit probably should also be bits in the flags byte.
	ubyte   flags;          // misc properties
	ubyte   pad[3];         // alignment

	ubyte   deathrollSound;    // if has deathroll, what sound?
	ubyte   glow;               // apply this light to robot itself. stored as 4:4 fixed-point
	ubyte   behavior;           //  Default behavior.
	ubyte   aim;                //  255 = perfect, less = more likely to miss.  0 != Random, would look stupid.  0=45 degree spread.  Specify in bitmaps.tbl in range 0.0..1.0

	//animation info
	jointlist animStates[MAX_GUNS+1][N_ANIM_STATES];

	int     always_0xabcd;      // debugging

} tRobotInfo;

typedef struct D1Robot_info {
	int			nModel;							// which polygon model?
	int			nGuns;								// how many different gun positions
	vmsVector	gunPoints[MAX_GUNS];			// where each gun model is
	ubyte			gunSubModels[MAX_GUNS];		// which submodel is each gun in?
	short 		exp1_vclip_num;
	short			exp1Sound_num;
	short 		exp2_vclip_num;
	short			exp2Sound_num;
	short			weaponType;
	sbyte			contains_id;						//	ID of powerup this robot can contain.
	sbyte			containsCount;					//	Max number of things this instance can contain.
	sbyte			containsProb;						//	Probability that this instance will contain something in N/16
	sbyte			containsType;						//	Type of thing contained, robot or powerup, in bitmaps.tbl, !0=robot, 0=powerup
	int			scoreValue;						//	Score from this robot.
	fix			lighting;							// should this be here or with polygon model?
	fix			strength;							// Initial shields of robot

	fix		mass;										// how heavy is this thing?
	fix		drag;										// how much drag does it have?

	fix		fieldOfView[NDL];					// compare this value with forward_vector.dot.vector_to_player, if fieldOfView <, then robot can see tPlayer
	fix		primaryFiringWait[NDL];						//	time in seconds between shots
	fix		turnTime[NDL];						// time in seconds to rotate 360 degrees in a dimension
	fix		fire_power[NDL];						//	damage done by a hit from this robot
	fix		shield[NDL];							//	shield strength of this robot
	fix		xMaxSpeed[NDL];						//	maximum speed attainable by this robot
	fix		circleDistance[NDL];				//	distance at which robot circles tPlayer

	sbyte		nRapidFireCount[NDL];				//	number of shots fired rapidly
	sbyte		evadeSpeed[NDL];						//	rate at which robot can evade shots, 0=none, 4=very fast
	sbyte		cloakType;								//	0=never, 1=always, 2=except-when-firing
	sbyte		attackType;							//	0=firing, 1=charge (like green guy)
	sbyte		bossFlag;								//	0 = not boss, 1 = boss.  Is that surprising?
	ubyte		seeSound;								//	sound robot makes when it first sees the tPlayer
	ubyte		attackSound;							//	sound robot makes when it attacks the tPlayer
	ubyte		clawSound;								//	sound robot makes as it claws you (attackType should be 1)

	//animation info
	jointlist animStates[MAX_GUNS+1][N_ANIM_STATES];

	int		always_0xabcd;							// debugging

} D1Robot_info;



#define MAX_ROBOT_TYPES 100      // maximum number of robot types
#define D1_MAX_ROBOT_TYPES 30      // maximum number of robot types

#define ROBOT_NAME_LENGTH   16
extern char Robot_names[MAX_ROBOT_TYPES][ROBOT_NAME_LENGTH];

//the array of robots types
extern tRobotInfo Robot_info [2][MAX_ROBOT_TYPES];     // Robot info for AI system, loaded from bitmaps.tbl.
extern tRobotInfo DefRobotInfo [MAX_ROBOT_TYPES];     // Robot info for AI system, loaded from bitmaps.tbl.

//how many kinds of robots
extern  int NRobotTypes [2];      // Number of robot types.  We used to assume this was the same as N_polygon_models.
extern  int N_DefRobotTypes;

//test data for one robot
#define MAX_ROBOT_JOINTS 1600
#define D1_MAX_ROBOT_JOINTS 600
extern tJointPos Robot_joints[MAX_ROBOT_JOINTS];
extern tJointPos DefRobotJoints [MAX_ROBOT_JOINTS];
extern int  NRobot_joints;
extern int  N_DefRobotJoints;
extern int nCamBotId;
extern int nCamBotModel;

void InitCamBots (int bReset);
void UnloadCamBot (void);
//given an CObject and a gun number, return position in 3-space of gun
//fills in gun_point
int CalcGunPoint(vmsVector *gun_point,CObject *obj,int gun_num);
//void CalcGunPoint(vmsVector *gun_point,int nObject,int gun_num);

//  Tells joint positions for a gun to be in a specified state.
//  A gun can have associated with it any number of joints.  In order to tell whether a gun is a certain
//  state (such as FIRE or ALERT), you should call this function and check the returned joint positions
//  against the robot's gun's joint positions.  This function should also be called to determine how to
//  move a gun into a desired position.
//  For now (May 30, 1994), it is assumed that guns will linearly interpolate from one joint position to another.
//  There is no ordering of joint movement, so it's impossible to guarantee that a strange starting position won't
//  cause a gun to move through a robot's body, for example.

//  Given:
//      jp_list_ptr     pointer to list of joint angles, on exit, this is pointing at a static array
//      robotType      nType of robot for which to get joint information.  A particular nType, not an instance of a robot.
//      gun_num         gun number.  If in 0..Robot_info[robotType].nGuns-1, then it is a gun, else it refers to non-animating parts of robot.
//      state           state about which to get information.  Legal states in range 0..N_ANIM_STATES-1, defined in robot.h, are:
//                          AS_REST, AS_ALERT, AS_FIRE, AS_RECOIL, AS_FLINCH

//  On exit:
//      Returns number of joints in list.
//      jp_list_ptr is stuffed with a pointer to a static array of joint positions.  This pointer is valid forever.
extern int RobotGetAnimState(tJointPos **jp_list_ptr,int robotType,int gun_num,int state);

#if 0
#define RobotInfoReadN(ri, n, fp) CFRead(ri, sizeof(tRobotInfo), n, fp)
#define JointPosReadN(jp, n, fp) CFRead(jp, sizeof(tJointPos), n, fp)
#else
/*
 * reads n tRobotInfo structs from a CFILE
 */
extern int RobotInfoReadN(CArray<tRobotInfo>& ri, int n, CFile& cf, int o = 0);

/*
 * reads n tJointPos structs from a CFILE
 */
extern int JointPosReadN (CArray<tJointPos>& jp, int n, CFile& cf, int o = 0);
#endif

#endif
