/* $Id: switch.h,v 1.4 2003/10/04 03:14:48 btb Exp $ */
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
#define MAX_WALLS_PER_LINK  10

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
#define TT_OPEN_WALL        9   // Makes a tWall open
#define TT_CLOSE_WALL       10  // Makes a tWall closed
#define TT_ILLUSORY_WALL    11  // Makes a tWall illusory
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
#define NUM_TRIGGER_TYPES   27

// Trigger flags

//could also use flags for one-shots

#define TF_NO_MESSAGE       1   // Don't show a message when triggered
#define TF_ONE_SHOT         2   // Only tTrigger once
#define TF_DISABLED         4   // Set after one-shot fires
#define TF_PERMANENT			 8
#define TF_ALTERNATE			 16
#define TF_SET_ORIENT		 32

//old tTrigger structs

typedef struct v29_trigger {
	sbyte   nType;
	short   flags;
	fix     value;
	fix     time;
	sbyte   link_num;
	short   nLinks;
	short   nSegment [MAX_WALLS_PER_LINK];
	short   nSide [MAX_WALLS_PER_LINK];
} __pack__ v29_trigger;

typedef struct v30_trigger {
	short   flags;
	sbyte   nLinks;
	sbyte   pad;                        //keep alignment
	fix     value;
	fix     time;
	short   nSegment [MAX_WALLS_PER_LINK];
	short   nSide [MAX_WALLS_PER_LINK];
} __pack__ v30_trigger;

//flags for V30 & below triggers
#define TRIGGER_CONTROL_DOORS      1    // Control Trigger
#define TRIGGER_SHIELD_DAMAGE      2    // Shield Damage Trigger
#define TRIGGER_ENERGY_DRAIN       4    // Energy Drain Trigger
#define TRIGGER_EXIT               8    // End of level Trigger
#define TRIGGER_ON                16    // Whether Trigger is active
#define TRIGGER_ONE_SHOT          32    // If Trigger can only be triggered once
#define TRIGGER_MATCEN            64    // Trigger for materialization centers
#define TRIGGER_ILLUSION_OFF     128    // Switch Illusion OFF tTrigger
#define TRIGGER_SECRET_EXIT      256    // Exit to secret level
#define TRIGGER_ILLUSION_ON      512    // Switch Illusion ON tTrigger
#define TRIGGER_UNLOCK_DOORS    1024    // Unlocks a door
#define TRIGGER_OPEN_WALL       2048    // Makes a tWall open
#define TRIGGER_CLOSE_WALL      4096    // Makes a tWall closed
#define TRIGGER_ILLUSORY_WALL   8192    // Makes a tWall illusory

//the tTrigger really should have both a nType & a flags, since most of the
//flags bits are exclusive of the others.
typedef struct tTrigger {
	ubyte   nType;       //what this tTrigger does
	short   flags;      //currently unused
	sbyte   nLinks;  //how many doors, etc. linked to this
	fix     value;
	fix     time;
	short   nSegment [MAX_WALLS_PER_LINK];
	short   nSide [MAX_WALLS_PER_LINK];
} __pack__ tTrigger;

typedef struct tObjTriggerRef {
	short		prev;
	short		next;
	short		nObject;
} tObjTriggerRef;

void TriggerInit();
void CheckTrigger(tSegment *seg, short tSide, short nObject,int shot);
int CheckTriggerSub (short nObject, tTrigger *triggers, int nTriggerCount, int nTrigger, 
							int nPlayer, int shot, int bBotTrigger);
void TriggersFrameProcess();
void ExecObjTriggers (short nObject, int bDamage);

#if 0
#define v29_trigger_read(t, fp) CFRead(t, sizeof(v29_trigger), 1, fp)
#define v30_trigger_read(t, fp) CFRead(t, sizeof(v30_trigger), 1, fp)
#define TriggerRead(t, fp) CFRead(t, sizeof(tTrigger), 1, fp)
#else
/*
 * reads a v29_trigger structure from a CFILE
 */
void v29_trigger_read(v29_trigger *t, CFILE *fp);

/*
 * reads a v30_trigger structure from a CFILE
 */
void v30_trigger_read(v30_trigger *t, CFILE *fp);

/*
 * reads a tTrigger structure from a CFILE
 */
void TriggerRead(tTrigger *t, CFILE *fp, int bObjTrigger);
#endif

void SetSpeedBoostVelocity (short nObject, fix speed, 
									 short srcSegnum, short srcSidenum,
									 short destSegnum, short destSidenum,
									 vmsVector *pSrcPt, vmsVector *pDestPt,
									 int bSetOrient);

void TriggerSetObjPos (short nObject, short nSegment);
void UpdatePlayerOrient (void);
int FindTriggerTarget (short nSegment, short nSide);
tTrigger *FindObjTrigger (short nObject, short nType, short nTrigger);

extern vmsVector	speedBoostSrc, speedBoostDest;

#endif
