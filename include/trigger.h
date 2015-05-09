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
	int8_t   nType;
	int16_t   flags;
	fix     value;
	fix     time;
	int8_t   link_num;
	int16_t   nLinks;
	int16_t   segments [MAX_TRIGGER_TARGETS];
	int16_t   sides [MAX_TRIGGER_TARGETS];
} __pack__ tTriggerV29;

typedef struct tTriggerV30 {
	int16_t   flags;
	int8_t   nLinks;
	int8_t   pad;                        //keep alignment
	fix     value;
	fix     time;
	int16_t   segments [MAX_TRIGGER_TARGETS];
	int16_t   sides [MAX_TRIGGER_TARGETS];
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
	uint8_t   type;       //what this trigger does
	uint8_t   flags;      //currently unused
	int8_t   num_links;  //how many doors, etc. linked to this
	int8_t   pad;        //keep alignment
	fix     value;
	fix     time;
	int16_t   seg [MAX_TRIGGER_TARGETS];
	int16_t   side [MAX_TRIGGER_TARGETS];
} __pack__ tCompatibleTrigger;

class CTriggerInfo {
	public:
		uint16_t	nWall;

		uint8_t		nType;   //what this CTrigger does
		uint16_t	flags;   
		uint16_t	flagsD1;
		fix		value;
		fix		time [2];

		uint16_t	nTeleportDest;
		int32_t		nChannel;
		int32_t		nObject;
		int32_t		nPlayer;
		int32_t		bShot;
		fix		tOperated;

	public:
		inline void Clear (void) { memset (this, 0, sizeof (*this)); }
	};

class CTriggerTargets {
	public:
		int16_t		m_nLinks;
		int16_t		m_segments [MAX_TRIGGER_TARGETS];
		int16_t		m_sides [MAX_TRIGGER_TARGETS];

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
		void Read (CFile& cf, int32_t bObjTrigger);
		int32_t Operate (int16_t nObject, int32_t nPlayer, int32_t shot, bool bObjTrigger);
		int32_t OperateD1 (int16_t nObject, int32_t nPlayer, int32_t shot);
		void Countdown (bool bObjTrigger);
		void PrintMessage (int32_t nPlayer, int32_t shot, const char *message);
		void DoLink (void);
		void DoChangeTexture (void);
		int32_t DoExecObjTrigger (int16_t nObject, int32_t bDamage);
		void DoSpawnBots (CObject* pObj);
		bool DoExit (int32_t nPlayer);
		bool DoSecretExit (int32_t nPlayer);
		void DoTeleportBot (CObject* pObj);
		void DoCloseDoor (void);
		int32_t DoLightOn (void);
		int32_t DoLightOff (void);
		void DoUnlockDoors (void);
		void DoLockDoors (void);
		int32_t DoSetSpawnPoints (void);
		int32_t DoMasterTrigger (int16_t nObject, int32_t nPlayer, bool bObjTrigger);
		int32_t DoEnableTrigger (void);
		int32_t DoDisableTrigger (void);
		void DoTeleport (int16_t nObject);
		int32_t DoShowMessage (void);
		int32_t DoPlaySound (int16_t nObject);
		int32_t DoChangeWalls (void);
		void DoObjectProducer (int32_t bMessage);
		void DoIllusionOn (void);
		void DoIllusionOff (void);
		void DoSpeedBoost (int16_t nObject);
		void StopSpeedBoost (int16_t nObject);
		int32_t DoDisarmRobots (void);
		int32_t DoReprogramRobots (void);
		int32_t DoShakeMine (void);
		bool TargetsWall (int32_t nWall);
		inline int32_t Index (void);
		inline int32_t HasTarget (int16_t nSegment, int16_t nSide);
		inline bool ClientOnly (void) { return (m_info.nType == TT_SPEEDBOOST) || (m_info.nType == TT_SHIELD_DAMAGE) || (m_info.nType == TT_ENERGY_DRAIN); }
		int32_t Delay (void);
		bool IsDelayed (void);
		bool IsExit (void);
		bool IsFlyThrough (void);
		void LoadState (CFile& cf, bool bObjTrigger = false);
		void SaveState (CFile& cf, bool bObjTrigger = false);

		inline uint8_t& Type (void) { return m_info.nType; }
		inline uint16_t& Flags (void) { return (Type () == TT_DESCENT1) ? m_info.flagsD1 : m_info.flags; }
		inline fix& Value (void) { return m_info.value; }
		inline fix GetValue (void) { return m_info.value; }
		inline void SetValue (fix value) { m_info.value = value; }
		inline fix GetTime (int32_t i) { return m_info.time [i]; }
		inline void SetTime (int32_t i, fix time) { m_info.time [i] = time; }
		//inline uint16_t& Segment (void) { return m_info.nTeleportDest; }
		inline int32_t& Player (void) { return m_info.nPlayer; }
		inline int32_t& Object (void) { return m_info.nObject; }
		inline int32_t& Channel (void) { return m_info.nChannel; }
		inline bool Flagged (uint16_t mask, uint16_t match = 0) { return match ? (Flags () & mask) == match : (Flags () & mask) != 0; }
		inline void SetFlags (uint16_t flags) { Flags () |= flags; }
		inline void ClearFlags (uint16_t flags) { Flags () &= ~flags; }

	private:
		int32_t WallIsForceField (void);
};

inline int32_t operator- (CTrigger* t, CArray<CTrigger>& a) { return a.Index (t); }

//------------------------------------------------------------------------------

typedef struct tObjTriggerRef {
#if 1
	uint8_t		nFirst;
	uint8_t		nCount;
#else
	int16_t		prev;
	int16_t		next;
	int16_t		nObject;
#endif
} __pack__ tObjTriggerRef;

void TriggerInit();
void TriggersFrameProcess();
void ExecObjTriggers (int16_t nObject, int32_t bDamage);

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

void SetSpeedBoostVelocity (int16_t nObject, fix speed, 
									 int16_t srcSegnum, int16_t srcSidenum,
									 int16_t destSegnum, int16_t destSidenum,
									 CFixVector *pSrcPt, CFixVector *pDestPt,
									 int32_t bSetOrient);

void TriggerSetOrient (tObjTransformation *pPos, int16_t nSegment, int16_t nSide, int32_t bSetPos, int32_t nStep);
void TriggerSetObjOrient (int16_t nObject, int16_t nSegment, int16_t nSide, int32_t bSetPos, int32_t nStep);
void TriggerSetObjPos (int16_t nObject, int16_t nSegment);
void UpdatePlayerOrient (void);
int32_t FindTriggerTarget (int16_t nSegment, int16_t nSide, int32_t i = 0);
CTrigger *FindObjTrigger (int16_t nObject, int16_t nType, int16_t nTrigger);
int32_t OpenExits (void);
void StartTriggeredSounds (void);
void StopTriggeredSounds (void);
int32_t FindNextLevel (void);

extern CFixVector	speedBoostSrc, speedBoostDest;

#endif
