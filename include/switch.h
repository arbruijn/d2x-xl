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

#ifndef _SWITCH_H
#define _SWITCH_H

#include "inferno.h"
#include "segment.h"

#define MAX_TRIGGERS        254
#define MAX_OBJ_TRIGGERS	 254
#define NO_TRIGGER			 255
#define MAX_TRIGGER_TARGETS  10

// Trigger types

#define TT_OPEN_DOOR        0   // Open a door
#define TT_CLOSE_DOOR       1   // Close a door
#define TT_MATCEN           2   // Activate a matcen
#define TT_EXIT             3   // End the level
#define TT_SECRET_EXIT      4   // Go to secret level
#define TT_ILLUSION_OFF     5   // Turn an illusion off
#define TT_ILLUSION_ON      6   // Turn an illusion on
#define TT_UNLOCK_DOOR      7   // Unlock a door
#define TT_LOCK_DOOR        8   // Lock a door
#define TT_OPEN_WALL        9   // Makes a CWall open
#define TT_CLOSE_WALL       10  // Makes a CWall closed
#define TT_ILLUSORY_WALL    11  // Makes a CWall illusory
#define TT_LIGHT_OFF        12  // Turn a light off
#define TT_LIGHT_ON         13  // Turn s light on
#define TT_TELEPORT			 14
#define TT_SPEEDBOOST		 15
#define TT_CAMERA				 16
#define TT_SHIELD_DAMAGE	 17
#define TT_ENERGY_DRAIN		 18
#define TT_CHANGE_TEXTURE	 19
#define TT_SMOKE_LIFE		 20
#define TT_SMOKE_SPEED		 21
#define TT_SMOKE_DENS		 22
#define TT_SMOKE_SIZE		 23
#define TT_SMOKE_DRIFT		 24
#define TT_COUNTDOWN			 25
#define TT_SPAWN_BOT			 26
#define TT_SMOKE_BRIGHTNESS 27
#define TT_SET_SPAWN			 28
#define TT_MESSAGE			 29
#define TT_SOUND				 30
#define TT_MASTER				 31
#define NUM_TRIGGER_TYPES   32

// Trigger flags

//could also use flags for one-shots

#define TF_NO_MESSAGE       1   // Don't show a message when triggered
#define TF_ONE_SHOT         2   // Only CTrigger once
#define TF_DISABLED         4   // Set after one-shot fires
#define TF_PERMANENT			 8
#define TF_ALTERNATE			 16
#define TF_SET_ORIENT		 32

//old CTrigger structs

typedef struct tTriggerV29 {
	sbyte   nType;
	short   flags;
	fix     value;
	fix     time;
	sbyte   link_num;
	short   nLinks;
	short   nSegment [MAX_TRIGGER_TARGETS];
	short   nSide [MAX_TRIGGER_TARGETS];
} __pack__ tTriggerV29;

typedef struct tTriggerV30 {
	short   flags;
	sbyte   nLinks;
	sbyte   pad;                        //keep alignment
	fix     value;
	fix     time;
	short   nSegment [MAX_TRIGGER_TARGETS];
	short   nSide [MAX_TRIGGER_TARGETS];
} __pack__ tTriggerV30;

//flags for V30 & below triggers
#define TRIGGER_CONTROL_DOORS      1    // Control Trigger
#define TRIGGER_SHIELD_DAMAGE      2    // Shield Damage Trigger
#define TRIGGER_ENERGY_DRAIN       4    // Energy Drain Trigger
#define TRIGGER_EXIT               8    // End of level Trigger
#define TRIGGER_ON                16    // Whether Trigger is active
#define TRIGGER_ONE_SHOT          32    // If Trigger can only be triggered once
#define TRIGGER_MATCEN            64    // Trigger for materialization centers
#define TRIGGER_ILLUSION_OFF     128    // Switch Illusion OFF CTrigger
#define TRIGGER_SECRET_EXIT      256    // Exit to secret level
#define TRIGGER_ILLUSION_ON      512    // Switch Illusion ON CTrigger
#define TRIGGER_UNLOCK_DOORS    1024    // Unlocks a door
#define TRIGGER_OPEN_WALL       2048    // Makes a CWall open
#define TRIGGER_CLOSE_WALL      4096    // Makes a CWall closed
#define TRIGGER_ILLUSORY_WALL   8192    // Makes a CWall illusory

//------------------------------------------------------------------------------
//the CTrigger really should have both a nType & a flags, since most of the
//flags bits are exclusive of the others.

class CTrigger {
	public:
		ubyte   nType;       //what this CTrigger does
		short   flags;      //currently unused
		sbyte   nLinks;  //how many doors, etc. linked to this
		fix     value;
		fix     time;
		short   segments [MAX_TRIGGER_TARGETS];
		short   sides [MAX_TRIGGER_TARGETS];

	public:
		void Read (CFile& cf, int bObjTrigger);
		int Operate (short nObject, int nPlayer, int shot, bool bObjTrigger);
		void PrintMessage (int nPlayer, int shot, const char *message);
		void DoLink (void);
		void DoChangeTexture (CTrigger *trigP);
		int DoExecObjTrigger (short nObject, int bDamage);
		void DoSpawnBots (CObject* objP);
		bool DoExit (int nPlayer);
		bool DoSecretExit (int nPlayer);
		void DoTeleportBot (CObject* objP);
		void DoCloseDoor (void);
		int DoLightOn (void);
		int DoLightOff (void);
		void DoUnlockDoors (void);
		void DoLockDoors (void);
		int DoSetSpawnPoints (void);
		int DoMasterTrigger (short nObject);
		int DoShowMessage (void);
		int DoPlaySound (short nObject);
		int DoChangeWalls (void);
		void DoMatCen (int bMessage);
		void DoIllusionOn (void);
		void DoIllusionOff (void);
		void DoSpeedBoost (short nObject);
		bool TargetsWall (int nWall);
		inline int Index (void);

	private:
		int WallIsForceField (void);
		inline int HasTarget (short nSegment, short nSide);
};

inline int operator- (CTrigger* t, CArray<CTrigger>& a) { return a.Index (t); }

//------------------------------------------------------------------------------

typedef struct tObjTriggerRef {
	short		prev;
	short		next;
	short		nObject;
} tObjTriggerRef;

void TriggerInit();
void CheckTrigger (CSegment *seg, short CSide, short nObject,int shot);
void TriggersFrameProcess();
void ExecObjTriggers (short nObject, int bDamage);

/*
 * reads a tTriggerV29 structure from a CFILE
 */
void V29TriggerRead (tTriggerV29& trigger, CFile& cf);

/*
 * reads a tTriggerV30 structure from a CFILE
 */
void V30TriggerRead (tTriggerV30& trigger, CFile& cf);

/*
 * reads a CTrigger structure from a CFILE
 */

void SetSpeedBoostVelocity (short nObject, fix speed, 
									 short srcSegnum, short srcSidenum,
									 short destSegnum, short destSidenum,
									 CFixVector *pSrcPt, CFixVector *pDestPt,
									 int bSetOrient);

void TriggerSetOrient (tObjTransformation *posP, short nSegment, short nSide, int bSetPos, int nStep);
void TriggerSetObjOrient (short nObject, short nSegment, short nSide, int bSetPos, int nStep);
void TriggerSetObjPos (short nObject, short nSegment);
void UpdatePlayerOrient (void);
int FindTriggerTarget (short nSegment, short nSide);
CTrigger *FindObjTrigger (short nObject, short nType, short nTrigger);
int OpenExits (void);

extern CFixVector	speedBoostSrc, speedBoostDest;

#endif
