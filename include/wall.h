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

#ifndef _WALL_H
#define _WALL_H

#include "descent.h"
#include "segment.h"
#include "object.h"
#include "cfile.h"

//#include "vclip.h"

#define MAX_WALLS               2047 // Maximum number of walls
#define MAX_WALL_ANIMS          60  // Maximum different types of doors
#define D1_MAX_WALL_ANIMS       30  // Maximum different types of doors
#define MAX_DOORS               90  // Maximum number of open doors
#define MAX_CLOAKING_WALLS		  100
#define MAX_EXPLODING_WALLS     10

// Various CWall types.
#define WALL_NORMAL             0   // Normal CWall
#define WALL_BLASTABLE          1   // Removable (by shooting) CWall
#define WALL_DOOR               2   // Door
#define WALL_ILLUSION           3   // Wall that appears to be there, but you can fly thru
#define WALL_OPEN               4   // Just an open CSide. (Trigger)
#define WALL_CLOSED             5   // Wall.  Used for transparent walls.
#define WALL_OVERLAY            6   // Goes over an actual solid CSide.  For triggers
#define WALL_CLOAKED            7   // Can see it, and see through it
#define WALL_COLORED				  8   // like cloaked, but fixed transparency and colored

// Various CWall flags.
#define WALL_BLASTED            1   // Blasted out CWall.
#define WALL_DOOR_OPENED        2   // Open door.
#define WALL_RENDER_ADDITIVE	  4
#define WALL_DOOR_LOCKED        8   // Door is locked.
#define WALL_DOOR_AUTO          16  // Door automatically closes after time.
#define WALL_ILLUSION_OFF       32  // Illusionary CWall is shut off.
#define WALL_WALL_SWITCH        64  // This CWall is openable by a CWall switch.
#define WALL_BUDDY_PROOF        128 // Buddy assumes he cannot get through this CWall.
#define WALL_IGNORE_MARKER		  256

// Wall states
#define WALL_DOOR_CLOSED        0       // Door is closed
#define WALL_DOOR_OPENING       1       // Door is opening.
#define WALL_DOOR_WAITING       2       // Waiting to close
#define WALL_DOOR_CLOSING       3       // Door is closing
#define WALL_DOOR_OPEN          4       // Door is open, and staying open
#define WALL_DOOR_CLOAKING      5       // Wall is going from closed -> open
#define WALL_DOOR_DECLOAKING    6       // Wall is going from open -> closed

//note: a door is considered opened (i.e., it has WALL_OPENED set) when it
//is more than half way open.  Thus, it can have any of OPENING, CLOSING,
//or WAITING bits set when OPENED is set.

#define KEY_NONE                1
#define KEY_BLUE                2
#define KEY_RED                 4
#define KEY_GOLD                8

#define WALL_HPS                I2X(100)    // Normal wall's hp
#define WALL_DOOR_INTERVAL      I2X(5)      // How many seconds a door is open

#define DOOR_OPEN_TIME          I2X(2)      // How long takes to open
#define DOOR_WAIT_TIME          I2X(5)      // How long before auto door closes

#define MAX_WALL_EFFECT_FRAMES		50
#define MAX_WALL_EFFECT_FRAMES_D1	20

// WALL_IS_DOORWAY flags.
#define WID_PASSABLE_FLAG			1
#define WID_VISIBLE_FLAG         2
#define WID_TRANSPARENT_FLAG		4
#define WID_EXTERNAL_FLAG			8
#define WID_CLOAKED_FLAG        16
#define WID_TRANSPCOLOR_FLAG	  32

#define WID_SOLID_WALL				WID_VISIBLE_FLAG
#define WID_TRANSPARENT_WALL		(WID_VISIBLE_FLAG | WID_TRANSPARENT_FLAG)
#define WID_ILLUSORY_WALL			(WID_PASSABLE_FLAG | WID_VISIBLE_FLAG)
#define WID_TRANSILLUSORY_WALL	(WID_PASSABLE_FLAG | WID_TRANSPARENT_WALL)
#define WID_NO_WALL					(WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG)

#define MAX_STUCK_OBJECTS   64

//------------------------------------------------------------------------------

typedef struct tStuckObject {
	int16_t   nObject, nWall;
	int32_t   nSignature;
} __pack__ tStuckObject;

//------------------------------------------------------------------------------

class CActiveDoor {
	public:
		int32_t  nPartCount;           // for linked walls
		int16_t  nFrontWall [2];			// front CWall numbers for this door
		int16_t  nBackWall [2];			// back CWall numbers for this door
		fix		time;						// how long been opening, closing, waiting

	public:
		void LoadState (CFile& cf);
		void SaveState (CFile& cf);
	};

//------------------------------------------------------------------------------
// data for exploding walls (such as hostage door)

class CExplodingWall {
	public:
		int32_t	nSegment, nSide;
		fix	time;

	public:
		void LoadState (CFile& cf);
		void SaveState (CFile& cf);
	};

//------------------------------------------------------------------------------

class CCloakingWall {
	public:
		int16_t   nFrontWall;			 // front CWall numbers for this door
		int16_t   nBackWall;			 // back CWall numbers for this door
		fix     front_ls [4];       // front CWall saved light values
		fix     back_ls [4];        // back CWall saved light values
		fix     time;               // how long been cloaking or decloaking

	public:
		void LoadState (CFile& cf);
		void SaveState (CFile& cf);
	};

//------------------------------------------------------------------------------
//Start old CWall structures

typedef struct tWallV16 {
	int8_t   nType;             // What kind of special CWall.
	int8_t   flags;             // Flags for the CWall.
	fix     hps;               // "Hit points" of the CWall.
	int8_t   nTrigger;          // Which CTrigger is associated with the CWall.
	int8_t   nClip;					// Which animation associated with the CWall.
	int8_t   keys;
} __pack__ tWallV16;

typedef struct tWallV19 {
	int32_t     nSegment,nSide;     // Seg & CSide for this CWall
	int8_t   nType;              // What kind of special CWall.
	int8_t   flags;              // Flags for the CWall.
	fix     hps;                // "Hit points" of the CWall.
	int8_t   nTrigger;            // Which CTrigger is associated with the CWall.
	int8_t   nClip;           // Which animation associated with the CWall.
	int8_t   keys;
	int32_t nLinkedWall;            // number of linked CWall
} __pack__ tWallV19;

typedef struct tCompatibleWall {
	int32_t		nSegment, nSide;
	fix		hps;				
	int32_t		nLinkedWall;	
	uint8_t		nType;			
	uint8_t		flags;			
	uint8_t		state;			
	uint8_t		nTrigger;		
	int8_t		nClip;			
	uint8_t		keys;				
	int8_t		controllingTrigger;
	int8_t		cloakValue;			
} __pack__ tCompatibleWall;

typedef struct v19_door {
	int32_t     nPartCount;            // for linked walls
	int16_t   seg[2];             // Segment pointer of door.
	int16_t   nSide [2];            // Side number of door.
	int16_t   nType [2];            // What kind of door animation.
	fix     open;               // How long it has been open.
} __pack__ v19_door;

//End old CWall structures

//------------------------------------------------------------------------------

class CWall {
	public:
		int32_t		nSegment, nSide;		// Seg & CSide for this CWall
		fix			hps;						// "Hit points" of the CWall.
		int32_t		nLinkedWall;			// number of linked CWall
		uint8_t		nType;					// What kind of special CWall.
		uint16_t		flags;					// Flags for the CWall.
		uint8_t		state;					// Opening, closing, etc.
		uint8_t		nTrigger;				// Which CTrigger is associated with the CWall.
		int8_t		nClip;					// Which animation associated with the CWall.
		uint8_t		keys;						// which keys are required
		int8_t		controllingTrigger;	// which CTrigger causes something to happen here.  Not like "CTrigger" above, which is the CTrigger on this CWall.
												//  Note: This gets stuffed at load time in gamemine.c.  Don't try to use it in the editor.  You will be sorry!
		int8_t		cloakValue;				// if this CWall is cloaked, the fade value
		int8_t		bVolatile;
	
	public:
		void Init (void);
		void Read (CFile& cf);
		void LoadTextures (void);
		int32_t IsPassable (CObject* pObj, bool bIgnoreDoors = false);
		bool IsOpenableDoor (void);
		int32_t IsTriggerTarget (int32_t i = 0);
		bool IsVolatile (void);
		bool IsInvisible (void);
		bool IsSolid (bool bIgnoreDoors = false);
		CActiveDoor* OpenDoor (void);
		CActiveDoor* CloseDoor (bool bForce = false);
		CCloakingWall* StartCloak (void);
		CCloakingWall* StartDecloak (void);
		void CloseDoor (int32_t nDoor);
		void CloseActiveDoor (void);
		int32_t AnimateOpeningDoor (fix xElapsedTime);
		int32_t AnimateClosingDoor (fix xElapsedTime);
		int32_t ProcessHit (int32_t nPlayer, CObject* pObj);
		CWall* OppositeWall (void);
		CTrigger* Trigger (void);
		void LoadState (CFile& cf);
		void SaveState (CFile& cf);
		inline bool IgnoreMarker (void) { return (flags & WALL_IGNORE_MARKER) != 0; }

		inline uint16_t& Flags (void) { return flags; }
		inline void SetFlags (uint16_t flags) { Flags () |= flags; }
		inline void ClearFlags (uint16_t flags) { Flags () &= ~flags; }

	};

inline int32_t operator- (CWall* o, CArray<CWall>& a) { return a.Index (o); }

//------------------------------------------------------------------------------
//wall clip flags
#define WCF_EXPLODES    1       //door explodes when opening
#define WCF_BLASTABLE   2       //this is a blastable CWall
#define WCF_TMAP1       4       //this uses primary tmap, not tmap2
#define WCF_HIDDEN      8       //this uses primary tmap, not tmap2
#define WCF_ALTFMT		16
#define WCF_FROMPOG		32
#define WCF_INITIALIZED	64

typedef struct tWallEffect {
	fix		xTotalTime;
	int16_t  nFrameCount;
	int16_t  frames [MAX_WALL_EFFECT_FRAMES];
	int16_t  openSound;
	int16_t  closeSound;
	int16_t  flags;
	char		filename [13];
	char		pad;
} __pack__ tWallEffect;

typedef struct tWallEffectD1 {
	fix		 playTime;
	int16_t   nFrameCount;
	int16_t   frames [MAX_WALL_EFFECT_FRAMES_D1];
	int16_t   openSound;
	int16_t   closeSound;
	int16_t   flags;
	char		 filename [13];
	char      pad;
} __pack__  tWallEffectD1;

extern char pszWallNames[7][10];

// Initializes all walls (i.e. no special walls.)
void WallInit();

//return codes for CSegment::ProcessWallHit ()
#define WHP_NOT_SPECIAL     0       //wasn't a quote-CWall-unquote
#define WHP_NO_KEY          1       //hit door, but didn't have key
#define WHP_BLASTABLE       2       //hit blastable CWall
#define WHP_DOOR            3       //a door (which will now be opening)

int32_t WallEffectFrameCount (tWallEffect *anim);

// Determines what happens when a CWall is shot
//obj is the CObject that hit...either a weapon or the player himself
// Tidy up Walls array for load/save purposes.
void ResetWalls();

void UnlockAllWalls (int32_t bOnlyDoors);

// Called once per frame..
void WallFrameProcess();

extern tStuckObject StuckObjects [MAX_STUCK_OBJECTS];

//  An CObject got stuck in a door (like a flare).
//  Add global entry.
void AddStuckObject (CObject *pObj, int16_t nSegment, int16_t nSide);
void RemoveObsoleteStuckObjects(void);

//set the tmap_num or tmap_num2 field for a CWall/door
void WallSetTMapNum(CSegment *pSeg,int16_t nSide,CSegment *csegp, int16_t cside,int32_t anim_num,int32_t nFrame);

void InitDoorAnims (void);

// Remove any flares from a CWall
void KillStuckObjects(int32_t nWall);

int32_t ReadWallEffectInfoD1(tWallEffect *wc, int32_t n, CFile& cf);
/*
 * reads n tWallEffect structs from a CFILE
 */
int32_t ReadWallEffectInfo(CArray<tWallEffect>& wc, int32_t n, CFile& cf);

/*
 * reads a tWallV16 structure from a CFILE
 */
void ReadWallV16(tWallV16& w, CFile& cf);

/*
 * reads a tWallV19 structure from a CFILE
 */
void ReadWallV19(tWallV19& w, CFile& cf);

/*
 * reads a v19_door structure from a CFILE
 */
void ReadActiveDoorV19(v19_door& d, CFile& cf);

/*
 * reads an CActiveDoor structure from a CFILE
 */
void ReadActiveDoor(CActiveDoor& d, CFile& cf);

CActiveDoor* FindActiveDoor (int16_t nWall);

void ExplodeWall (int16_t nSegment, int16_t nSide);
void DoExplodingWallFrame (void);
void InitExplodingWalls (void);
void SetupWalls (void);

#endif
