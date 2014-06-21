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

#include "descent.h"
#include "segment.h"

#define MAX_TRIGGERS        254
#define MAX_OBJ_TRIGGERS	 254
#define NO_TRIGGER			 255
#define MAX_TRIGGER_TARGETS  10

// Trigger types

#define TT_OPEN_DOOR        0   // Open a door
#define TT_CLOSE_DOOR       1   // Close a door
#define TT_OBJECT_PRODUCER  2   // Activate an object producer
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
#define TT_ENABLE_TRIGGER	 32
#define TT_DISABLE_TRIGGER	 33
#define TT_DISARM_ROBOT		 34
#define TT_REPROGRAM_ROBOT  35
#define TT_SHAKE_MINE		 36
#define NUM_TRIGGER_TYPES   37
#define TT_DESCENT1			 255

// Trigger flags

//could also use flags for one-shots

#define TF_NO_MESSAGE        1   // Don't show a message when triggered
#define TF_ONE_SHOT          2   // Only trigger once
#define TF_DISABLED          4   // Set after one-shot fires
#define TF_PERMANENT			  8
#define TF_ALTERNATE			 16
#define TF_SET_ORIENT		 32
#define TF_SILENT				 64
#define TF_AUTOPLAY			128
#define TF_PLAYING_SOUND	256	// only used internally
#define TF_FLY_THROUGH		512	// helper flag to quickly identify non-wall switch based triggers

//old CTrigger structs

typedef struct tTriggerV29 {
	sbyte   nType;
	short   flags;
	fix     value;
	fix     time;
	sbyte   link_num;
	short   nLinks;
	short   segments [MAX_TRIGGER_TARGETS];
	short   sides [MAX_TRIGGER_TARGETS];
} __pack__ tTriggerV29;

typedef struct tTriggerV30 {
	short   flags;
	sbyte   nLinks;
	sbyte   pad;                        //keep alignment
	fix     value;
	fix     time;
	short   segments [MAX_TRIGGER_TARGETS];
	short   sides [MAX_TRIGGER_TARGETS];
} __pack__ tTriggerV30;

//flags for V30 & below triggers
#define TRIGGER_CONTROL_DOORS      1    // Control Trigger
#define TRIGGER_SHIELD_DAMAGE      2    // Shield Damage Trigger
#define TRIGGER_ENERGY_DRAIN       4    // Energy Drain Trigger
#define TRIGGER_EXIT               8    // End of level Trigger
#define TRIGGER_ON                16    // Whether Trigger is active
#define TRIGGER_ONE_SHOT          32    // If Trigger can only be triggered once
#define TRIGGER_OBJECT_PRODUCER   64    // Trigger for materialization centers
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

typedef struct tCompatibleTrigger {
	ubyte   type;       //what this trigger does
	ubyte   flags;      //currently unused
	sbyte   num_links;  //how many doors, etc. linked to this
	sbyte   pad;        //keep alignment
	fix     value;
	fix     time;
	short   seg [MAX_TRIGGER_TARGETS];
	short   side [MAX_TRIGGER_TARGETS];
} __pack__ tCompatibleTrigger;

class CTriggerInfo {
	public:
		ushort	nWall;

		ubyte		nType;   //what this CTrigger does
		ushort	flags;   
		ushort	flagsD1;
		fix		value;
		fix		time [2];

		ushort	nTeleportDest;
		int		nChannel;
		int		nObject;
		int		nPlayer;
		int		bShot;
		fix		tOperated;
	};

class CTriggerTargets {
	public:
		short		m_nLinks;
		short		m_segments [MAX_TRIGGER_TARGETS];
		short		m_sides [MAX_TRIGGER_TARGETS];

		void Read (CFile& cf);
		void SaveState (CFile& cf);
		void LoadState (CFile& cf);

	private:	
		void Check (void);
	};

class CTrigger : public CTriggerTargets {
	public:
		CTriggerInfo	m_info;

	public:
		void Read (CFile& cf, int bObjTrigger);
		int Operate (short nObject, int nPlayer, int shot, bool bObjTrigger);
		int OperateD1 (short nObject, int nPlayer, int shot);
		void Countdown (bool bObjTrigger);
		void PrintMessage (int nPlayer, int shot, const char *message);
		void DoLink (void);
		void DoChangeTexture (void);
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
		int DoMasterTrigger (short nObject, int nPlayer, bool bObjTrigger);
		int DoEnableTrigger (void);
		int DoDisableTrigger (void);
		void DoTeleport (short nObject);
		int DoShowMessage (void);
		int DoPlaySound (short nObject);
		int DoChangeWalls (void);
		void DoObjectProducer (int bMessage);
		void DoIllusionOn (void);
		void DoIllusionOff (void);
		void DoSpeedBoost (short nObject);
		void StopSpeedBoost (short nObject);
		int DoDisarmRobots (void);
		int DoReprogramRobots (void);
		int DoShakeMine (void);
		bool TargetsWall (int nWall);
		inline int Index (void);
		inline int HasTarget (short nSegment, short nSide);
		inline bool ClientOnly (void) { return (m_info.nType == TT_SPEEDBOOST) || (m_info.nType == TT_SHIELD_DAMAGE) || (m_info.nType == TT_ENERGY_DRAIN); }
		int Delay (void);
		bool IsDelayed (void);
		bool IsExit (void);
		bool IsFlyThrough (void);
		void LoadState (CFile& cf, bool bObjTrigger = false);
		void SaveState (CFile& cf, bool bObjTrigger = false);

		inline byte& Type (void) { return m_info.nType; }
		inline ushort& Flags (void) { return m_info.flags; }
		inline fix& Value (void) { return m_info.value; }
		inline fix GetValue (void) { return m_info.value; }
		inline void SetValue (fix value) { m_info.value = value; }
		inline fix GetTime (int i) { return m_info.time [i]; }
		inline void SetTime (int i, fix time) { m_info.time [i] = time; }
		//inline ushort& Segment (void) { return m_info.nTeleportDest; }
		inline int& Player (void) { return m_info.nPlayer; }
		inline int& Object (void) { return m_info.nObject; }
		inline int& Channel (void) { return m_info.nChannel; }
		inline bool Flagged (ushort mask, ushort match = 0) { return match ? (Flags () & mask) == match : (Flags () & mask) != 0; }
		inline void SetFlags (ushort flags) { Flags () |= flags; }
		inline void ClearFlags (ushort flags) { Flags () &= ~flags; }

	private:
		int WallIsForceField (void);
};

inline int operator- (CTrigger* t, CArray<CTrigger>& a) { return a.Index (t); }

//------------------------------------------------------------------------------

typedef struct tObjTriggerRef {
#if 1
	ubyte		nFirst;
	ubyte		nCount;
#else
	short		prev;
	short		next;
	short		nObject;
#endif
} __pack__ tObjTriggerRef;

void TriggerInit();
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
int FindTriggerTarget (short nSegment, short nSide, int i = 0);
CTrigger *FindObjTrigger (short nObject, short nType, short nTrigger);
int OpenExits (void);
void StartTriggeredSounds (void);
void StopTriggeredSounds (void);
int FindNextLevel (void);

extern CFixVector	speedBoostSrc, speedBoostDest;

#endif
