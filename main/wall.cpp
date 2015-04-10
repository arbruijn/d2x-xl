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
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "descent.h"
#include "error.h"
#include "findpath.h"
#include "segmath.h"
#include "cockpit.h"
#include "text.h"
#include "fireball.h"
#include "textures.h"
#include "newdemo.h"
#include "collide.h"
#include "rendermine.h"
#include "light.h"
#include "segment.h"
#include "dynlight.h"
#include "loadobjects.h"
#include "savegame.h"
#include "audio.h"

//	Special door on boss level which is locked if not in multiplayer...sorry for this awful solution --MK.
#define	BOSS_LOCKED_DOOR_LEVEL	7
#define	BOSS_LOCKED_DOOR_SEG		595
#define	BOSS_LOCKED_DOOR_SIDE	5

//--unused-- int32_t gameData.wallData.bitmaps [MAX_WALL_ANIMS];

//door Doors [MAX_DOORS];


#define CLOAKING_WALL_TIME I2X (1)

//--unused-- CBitmap *wall_title_bms [MAX_WALL_ANIMS];

//#define BM_FLAG_TRANSPARENT			1
//#define BM_FLAG_SUPER_TRANSPARENT	2

//------------------------------------------------------------------------------

// Opens a door, including animation and other processing.
bool DoDoorOpen (int32_t nDoor);

// Closes a door, including animation and other processing.
bool DoDoorClose (int32_t nDoor);

//------------------------------------------------------------------------------

static CActiveDoor* AddActiveDoor (void)
{
if (!gameData.wallData.activeDoors.Grow ())
	return NULL;
audio.Update ();
return gameData.wallData.activeDoors.Top ();
}

//------------------------------------------------------------------------------

static CCloakingWall*	AddCloakingWall (void)
{
if (!gameData.wallData.cloaking.Grow ())
	return NULL;
audio.Update ();
return gameData.wallData.cloaking.Top ();
}

//------------------------------------------------------------------------------

static CExplodingWall* AddExplodingWall (void)
{
if (!gameData.wallData.exploding.Grow ())
	return NULL;
audio.Update ();
return gameData.wallData.exploding.Top ();
}

//------------------------------------------------------------------------------

static void DeleteActiveDoor (int32_t nDoor)
{
gameData.wallData.activeDoors.Delete (static_cast<uint32_t> (nDoor));
audio.Update ();
}

//------------------------------------------------------------------------------

static void DeleteCloakingWall (int32_t nWall)
{
gameData.wallData.cloaking.Delete (static_cast<uint32_t> (nWall));
audio.Update ();
}

//------------------------------------------------------------------------------

static void DeleteExplodingWall (int32_t nWall)
{
gameData.wallData.exploding.Delete (static_cast<uint32_t> (nWall));
audio.Update ();
}

//------------------------------------------------------------------------------

int32_t WallEffectFrameCount (tWallEffect *anim)
{
	int32_t	n;

CBitmap *bmP = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [anim->frames [0]].index;
if (bmP->Override ())
	bmP = bmP->Override ();
n = (bmP->Type () == BM_TYPE_ALT) ? bmP->FrameCount () : anim->nFrameCount;
return (n > 1) ? n : anim->nFrameCount;
}

//------------------------------------------------------------------------------

fix AnimPlayTime (tWallEffect *anim)
{
int32_t nFrames = WallEffectFrameCount (anim);
fix pt = (fix) (anim->xTotalTime * gameStates.gameplay.slowmo [0].fSpeed);

if (nFrames == anim->nFrameCount)
	return pt;
return (fix) (((double) pt * (double) anim->nFrameCount) / (double) nFrames);
}

// This function determines whether the current CSegment/nSide is transparent
//		1 = YES
//		0 = NO
//------------------------------------------------------------------------------

CActiveDoor* FindActiveDoor (int16_t nWall)
{
	CActiveDoor* doorP = gameData.wallData.activeDoors.Buffer ();

for (uint32_t i = gameData.wallData.activeDoors.ToS (); i; i--, doorP++) {		//find door
	for (int32_t j = 0; j < doorP->nPartCount; j++)
		if ((doorP->nFrontWall [j] == nWall) || (doorP->nBackWall [j] == nWall))
			return doorP;
	}
return NULL;
}

//------------------------------------------------------------------------------
// This function checks whether we can fly through the given side.
//	In other words, whether or not we have a 'doorway'
//	 Flags:
//		WID_PASSABLE_FLAG			1
//		WID_VISIBLE_FLAG			2
//		WID_TRANSPARENT_FLAG		4
//	 Return values:
//		WID_SOLID_WALL				2	// 0/1/0		solid wall
//		WID_TRANSPARENT_WALL		6	//	0/1/1		transparent wall
//		WID_ILLUSORY_WALL			3	//	1/1/0		illusory wall
//		WID_TRANSILLUSORY_WALL	7	//	1/1/1		transparent illusory wall
//		WID_NO_WALL					5	//	1/0/1		no wall, can fly through

int32_t CWall::IsPassable (CObject *objP, bool bIgnoreDoors)
{
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif
if (nType == WALL_OPEN)
	return WID_NO_WALL;

if (nType == WALL_ILLUSION) {
	if (flags & WALL_ILLUSION_OFF)
		return WID_NO_WALL;
	if ((cloakValue < FADE_LEVELS) || SEGMENT (nSegment)->CheckTransparency (nSide))
		return WID_TRANSILLUSORY_WALL;
	return WID_ILLUSORY_WALL;
	}

if (nType == WALL_BLASTABLE) {
	if (flags & WALL_BLASTED)
		return WID_TRANSILLUSORY_WALL;
	if ((cloakValue < FADE_LEVELS) || SEGMENT (nSegment)->CheckTransparency (nSide))
		return WID_TRANSPARENT_WALL;
	return WID_SOLID_WALL;
	}

if (nType == WALL_CLOAKED)
	return WID_TRANSPARENT_WALL | WID_CLOAKED_FLAG;

if (nType == WALL_COLORED)
	return (hps < 0) ? WID_TRANSILLUSORY_WALL | WID_TRANSPCOLOR_FLAG : WID_TRANSPARENT_WALL | WID_TRANSPCOLOR_FLAG;

if (nType == WALL_DOOR) {
	if (bIgnoreDoors)
		return WID_TRANSPARENT_WALL;
	if (flags & WALL_DOOR_OPENED)
		return WID_TRANSILLUSORY_WALL;
	if ((state == WALL_DOOR_OPENING) || (state == WALL_DOOR_CLOSING))
		return WID_TRANSPARENT_WALL;
	if ((cloakValue && (cloakValue < FADE_LEVELS)) || SEGMENT (nSegment)->CheckTransparency (nSide))
		return WID_TRANSPARENT_WALL;
	return WID_SOLID_WALL;
	}

if (nType == WALL_CLOSED) {
	if (objP && (objP->info.nType == OBJ_PLAYER)) {
		if (IsTeamGame && ((keys >> 1) == GetTeam (objP->info.nId) + 1))
			return WID_ILLUSORY_WALL;
		if ((keys == KEY_BLUE) && (PLAYER (objP->info.nId).flags & PLAYER_FLAGS_BLUE_KEY))
			return WID_ILLUSORY_WALL;
		if ((keys == KEY_RED) && (PLAYER (objP->info.nId).flags & PLAYER_FLAGS_RED_KEY))
			return WID_ILLUSORY_WALL;
		}
	}
// If none of the above flags are set, there is no doorway.
if ((cloakValue && (cloakValue < FADE_LEVELS)) || SEGMENT (nSegment)->CheckTransparency (nSide)) 
	return WID_TRANSPARENT_WALL;
return WID_SOLID_WALL; // There are children behind the door.
}

//------------------------------------------------------------------------------

bool CWall::IsSolid (bool bIgnoreDoors)
{
return (IsPassable (NULL, bIgnoreDoors) & (WID_TRANSPARENT_WALL | WID_SOLID_WALL)) != 0;
}

//------------------------------------------------------------------------------

CActiveDoor* CWall::OpenDoor (void)
{
	CActiveDoor* doorP = NULL;

if ((state == WALL_DOOR_OPENING) ||	//already opening
	 (state == WALL_DOOR_WAITING) ||	//open, waiting to close
	 (state == WALL_DOOR_OPEN))		//open, & staying open
	return NULL;

if (state == WALL_DOOR_CLOSING) {		//closing, so reuse door
	if ((doorP = FindActiveDoor (this - WALLS))) {
		doorP->time = (fix) (gameData.wallData.animP [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed) - doorP->time;
		if (doorP->time < 0)
			doorP->time = 0;
		}
	}
else if (state != WALL_DOOR_CLOSED)
	return NULL;
if (!doorP) {
	if (!(doorP = AddActiveDoor ()))
		return NULL;
	doorP->time = 0;
	}
state = WALL_DOOR_OPENING;
audio.Update ();
return doorP;
}

//------------------------------------------------------------------------------

CCloakingWall* FindCloakingWall (int16_t nWall)
{
	CCloakingWall* cloakWallP = gameData.wallData.cloaking.Buffer ();

for (uint32_t i = gameData.wallData.cloaking.ToS (); i; i--, cloakWallP++) {		//find door
	if ((cloakWallP->nFrontWall == nWall) || (cloakWallP->nBackWall == nWall))
		return cloakWallP;
	}
return NULL;
}

//------------------------------------------------------------------------------
// This function closes the specified door and restuckObjPres the closed
//  door texture.  This is called when the animation is done
void CloseDoor (int32_t nDoor)
{
	CActiveDoor*	doorP = gameData.wallData.activeDoors + nDoor;

for (int32_t i = doorP->nPartCount; i; )
	WALL (doorP->nFrontWall [--i])->CloseActiveDoor ();
DeleteActiveDoor (nDoor);
}

//------------------------------------------------------------------------------

CCloakingWall* CWall::StartCloak (void)
{
	CCloakingWall* cloakWallP = NULL;

if (state == WALL_DOOR_DECLOAKING) {	//decloaking, so reuse door
	if (!(cloakWallP = FindCloakingWall (this - WALLS)))
		return NULL;
	cloakWallP->time = (fix) (CLOAKING_WALL_TIME * gameStates.gameplay.slowmo [0].fSpeed) - cloakWallP->time;
	}
else if (state == WALL_DOOR_CLOSED) {	//create new door
	if (!(cloakWallP = AddCloakingWall ()))
		return NULL;
	cloakWallP->time = 0;
	}
else
	return NULL;
state = WALL_DOOR_CLOAKING;
return cloakWallP;
}

//------------------------------------------------------------------------------

CCloakingWall* CWall::StartDecloak (void)
{
	CCloakingWall*	cloakWallP = NULL;

if (state == WALL_DOOR_CLOAKING) {	//cloaking, so reuse door
	if (!(cloakWallP = FindCloakingWall (this - WALLS)))
		return NULL;
	cloakWallP->time = (fix) (CLOAKING_WALL_TIME * gameStates.gameplay.slowmo [0].fSpeed) - cloakWallP->time;
	}
else if (state == WALL_DOOR_CLOSED) {	//create new door
	if (!(cloakWallP = AddCloakingWall ()))
		return NULL;
	cloakWallP->time = 0;
	}
else
	return NULL;
state = WALL_DOOR_DECLOAKING;
return cloakWallP;
}

//------------------------------------------------------------------------------

void CWall::CloseActiveDoor (void)
{
CSegment* segP = SEGMENT (nSegment);
CSegment* connSegP = SEGMENT (segP->m_children [nSide]);
int16_t nConnSide = segP->ConnectedSide (connSegP);

state = WALL_DOOR_CLOSED;
segP->SetTexture (nSide, NULL, -1, nClip, 0);

CWall* wallP = connSegP->Wall (nConnSide);
if (wallP) {
	wallP->state = WALL_DOOR_CLOSED;
	connSegP->SetTexture (nConnSide, NULL, -1, wallP->nClip, 0);
	}
}

//------------------------------------------------------------------------------

CActiveDoor* CWall::CloseDoor (bool bForce)
{
	CActiveDoor* doorP = NULL;

if ((state == WALL_DOOR_OPENING) ||
	 ((state == WALL_DOOR_WAITING) && bForce && gameStates.app.bD2XLevel)) {	//reuse door
	if ((doorP = FindActiveDoor (this - WALLS))) {
		doorP->time = (fix) (gameData.wallData.animP [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed) - doorP->time;
		if (doorP->time < 0)
			doorP->time = 0;
		}
	}
else if (state != WALL_DOOR_OPEN)
	return NULL;
if (!doorP) {
	if (!(doorP = AddActiveDoor ()))
		return NULL;
	doorP->time = 0;
	}
state = WALL_DOOR_CLOSING;
return doorP;
}

//------------------------------------------------------------------------------

int32_t CWall::AnimateOpeningDoor (fix xElapsedTime)
{
if (nClip < 0)
	return 3;

int32_t nFrames = WallEffectFrameCount (gameData.wallData.animP + nClip);
if (!nFrames)
	return 3;

fix xTotalTime = (fix) (gameData.wallData.animP [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed);
fix xFrameTime = xTotalTime / nFrames;
#if DBG
int32_t i;
if (xElapsedTime < 0)
	i = nFrames;
else
	i = xElapsedTime / xFrameTime;
#else
int32_t i = (xElapsedTime < 0) ? nFrames : xElapsedTime / xFrameTime;
#endif
CSegment* segP = SEGMENT (nSegment);
if (i < nFrames)
	segP->SetTexture (nSide, NULL, -1, nClip, i);
if (i > nFrames / 2)
	flags |= WALL_DOOR_OPENED;
if (i < nFrames - 1)
	return 0; // not fully opened yet
segP->SetTexture (nSide, NULL, -1, nClip, nFrames - 1);
if (flags & WALL_DOOR_AUTO) {
	state = WALL_DOOR_WAITING;
	return 2;
	}
// If our door is not automatic just remove it from the list.
state = WALL_DOOR_OPEN;
return 1;
}

//------------------------------------------------------------------------------

int32_t CWall::AnimateClosingDoor (fix xElapsedTime)
{
int32_t nFrames = WallEffectFrameCount (gameData.wallData.animP + nClip);
if (!nFrames)
	return 0;

fix xTotalTime = (fix) (gameData.wallData.animP [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed);
fix xFrameTime = xTotalTime / nFrames;
int32_t i = nFrames - xElapsedTime / xFrameTime - 1;
if (i < nFrames / 2)
	flags &= ~WALL_DOOR_OPENED;
if (i <= 0)
	return 1;
SEGMENT (nSegment)->SetTexture (nSide, NULL, -1, nClip, i);
state = WALL_DOOR_CLOSING;
return 0;
}

//------------------------------------------------------------------------------
// Animates opening of a door.
// Called in the game loop.
bool DoDoorOpen (int32_t nDoor)
{
	CActiveDoor*	doorP;
	CWall*			wallP;
	int16_t				nConnSide, nSide;
	CSegment*		connSegP, * segP;
	int32_t				i, nWall, bFlags = 3;

if (nDoor < -1)
	return false;
doorP = gameData.wallData.activeDoors + nDoor;
for (i = 0; i < doorP->nPartCount; i++) {
	doorP->time += gameData.time.xFrame;
	nWall = doorP->nFrontWall [i];
	if (!IS_WALL (nWall)) {
		PrintLog (0, "Trying to open non existent door\n");
		continue;
		}
	wallP = WALL (doorP->nFrontWall [i]);
	KillStuckObjects (doorP->nFrontWall [i]);
	KillStuckObjects (doorP->nBackWall [i]);

	segP = SEGMENT (wallP->nSegment);
	nSide = wallP->nSide;
	if (!segP->IsWall (nSide)) {
		PrintLog (0, "Trying to open non existent door @ %d,%d\n", wallP->nSegment, nSide);
		continue;
		}
	bFlags &= segP->AnimateOpeningDoor (nSide, doorP->time);
	connSegP = SEGMENT (segP->m_children [nSide]);
	nConnSide = segP->ConnectedSide (connSegP);
	if (nConnSide < 0) {
		PrintLog (0, "Door %d @ %d,%d has no oppposite door in %d\n", segP->WallNum (nSide), wallP->nSegment, nSide, segP->m_children [nSide]);
		continue;
		}
	if (connSegP->IsWall (nConnSide))
		bFlags &= connSegP->AnimateOpeningDoor (nConnSide, doorP->time);
	}
if (bFlags & 1) {
	DeleteActiveDoor (nDoor);
	return true;
	}
if (bFlags & 2)
	doorP->time = 0;	//counts up
return false;
}

//------------------------------------------------------------------------------
// Animates and processes the closing of a door.
// Called from the game loop.
bool DoDoorClose (int32_t nDoor)
{
if (nDoor < -1)
	return false;
CActiveDoor* doorP = gameData.wallData.activeDoors + nDoor;
CWall* wallP = WALL (doorP->nFrontWall [0]);
CSegment* segP = SEGMENT (wallP->nSegment);

	int32_t			i, bFlags = 1;
	int16_t			nConnSide, nSide;
	CSegment*	connSegP;

//check for OBJECTS in doorway before closing
if (wallP->flags & WALL_DOOR_AUTO) {
	if (segP->DoorIsBlocked (int16_t (wallP->nSide), wallP->IgnoreMarker ())) {
		audio.DestroySegmentSound (int16_t (wallP->nSegment), int16_t (wallP->nSide), -1);
		segP->OpenDoor (int16_t (wallP->nSide));		//re-open door
		return false;
		}
	}

for (i = 0; i < doorP->nPartCount; i++) {
	wallP = WALL (doorP->nFrontWall [i]);
	segP = SEGMENT (wallP->nSegment);
	nSide = wallP->nSide;
	if (!segP->IsWall (nSide)) {
#if DBG
		PrintLog (0, "Trying to close non existent door\n");
#endif
		continue;
		}
	//if here, must be auto door
	//don't assert here, because now we have triggers to close non-auto doors
	// Otherwise, close it.
	connSegP = SEGMENT (segP->m_children [nSide]);
	nConnSide = segP->ConnectedSide (connSegP);
	if (nConnSide < 0) {
		PrintLog (0, "Door %d @ %d,%d has no oppposite door in %d\n", 
					 segP->WallNum (nSide), wallP->nSegment, nSide, segP->m_children [nSide]);
		continue;
		}
	if ((gameData.demo.nState != ND_STATE_PLAYBACK) && !(i || doorP->time)) {
		if (gameData.wallData.animP [wallP->nClip].closeSound  > -1)
			audio.CreateSegmentSound ((int16_t) gameData.wallData.animP [segP->Wall (nSide)->nClip].closeSound,
											  wallP->nSegment, nSide, SEGMENT (wallP->nSegment)->SideCenter (nSide), 0, I2X (1));
		}
	doorP->time += gameData.time.xFrame;
	bFlags &= segP->AnimateClosingDoor (nSide, doorP->time);
	if (connSegP->IsWall (nConnSide))
		bFlags &= connSegP->AnimateClosingDoor (nConnSide, doorP->time);
	}
if (bFlags & 1) {
	CloseDoor (nDoor);
	return true;
	}
return false;
}

//	-----------------------------------------------------------------------------
//	Allowed to open the normally locked special boss door if in multiplayer mode.
int32_t AllowToOpenSpecialBossDoor (int32_t nSegment, int16_t nSide)
{
if (IsMultiGame)
	return (missionManager.nCurrentLevel == BOSS_LOCKED_DOOR_LEVEL) &&
			 (nSegment == BOSS_LOCKED_DOOR_SEG) &&
			 (nSide == BOSS_LOCKED_DOOR_SIDE);
return 0;
}

//------------------------------------------------------------------------------

void UnlockAllWalls (int32_t bOnlyDoors)
{
	int32_t	i;
	CWall	*wallP;

for (i = gameData.wallData.nWalls, wallP = WALLS.Buffer (); i; wallP++, i--) {
	if (wallP->nType == WALL_DOOR) {
		wallP->flags &= ~WALL_DOOR_LOCKED;
		wallP->keys = 0;
		}
	else if (!bOnlyDoors
#if !DBG
				&& (wallP->nType == WALL_CLOSED)
#endif
			 )
		wallP->nType = WALL_OPEN;
	}
}

//------------------------------------------------------------------------------
// set up renderer for alternative highres animation method
// (all frames are stuckObjPred in one texture, and struct nSide.nFrame
// holds the frame index).

void InitDoorAnims (void)
{
	int32_t			h, i;
	CWall			*wallP;
	CSegment		*segP;
	tWallEffect	*animP;

for (i = 0, wallP = WALLS.Buffer (); i < gameData.wallData.nWalls; wallP++, i++) {
	if (wallP->nType == WALL_DOOR) {
		animP = gameData.wallData.animP + wallP->nClip;
		if (!(animP->flags & WCF_ALTFMT))
			continue;
		h = (animP->flags & WCF_TMAP1) ? -1 : 1;
		segP = SEGMENT (wallP->nSegment);
		segP->m_sides [wallP->nSide].m_nFrame = h;
		}
	}
}

//------------------------------------------------------------------------------
// Determines what happens when a CWall is shot
//returns info about CWall.  see wall[HA] for codes
//obj is the CObject that hit...either a weapon or the player himself
//nPlayer is the number the player who hit the CWall or fired the weapon,
//or -1 if a robot fired the weapon

int32_t CWall::ProcessHit (int32_t nPlayer, CObject* objP)
{
	bool bShowMessage;

if (objP->info.nType == OBJ_PLAYER)
	bShowMessage = (CFixVector::Dot (objP->info.position.mOrient.m.dir.f, objP->mType.physInfo.velocity) > 0);
else if (objP->info.nType == OBJ_ROBOT)
	bShowMessage = false;
else if ((objP->info.nType == OBJ_WEAPON) && (objP->cType.laserInfo.parent.nType == OBJ_ROBOT))
	bShowMessage = false;
else
	bShowMessage = true;

if (keys == KEY_BLUE) {
	if (!(PLAYER (nPlayer).flags & PLAYER_FLAGS_BLUE_KEY)) {
		if (bShowMessage && (nPlayer == N_LOCALPLAYER))
			HUDInitMessage ("%s %s", TXT_BLUE, TXT_ACCESS_DENIED);
		return WHP_NO_KEY;
		}
	}
else if (keys == KEY_RED) {
	if (!(PLAYER (nPlayer).flags & PLAYER_FLAGS_RED_KEY)) {
		if (bShowMessage && (nPlayer == N_LOCALPLAYER))
			HUDInitMessage("%s %s", TXT_RED, TXT_ACCESS_DENIED);
		return WHP_NO_KEY;
		}
	}
else if (keys == KEY_GOLD) {
	if (!(PLAYER (nPlayer).flags & PLAYER_FLAGS_GOLD_KEY)) {
		if (bShowMessage && (nPlayer == N_LOCALPLAYER))
			HUDInitMessage("%s %s", TXT_YELLOW, TXT_ACCESS_DENIED);
		return WHP_NO_KEY;
		}
	}
if (nType == WALL_DOOR) {
	if ((flags & WALL_DOOR_LOCKED) && !(AllowToOpenSpecialBossDoor (nSegment, nSide))) {
		if (bShowMessage && (nPlayer == N_LOCALPLAYER))
			HUDInitMessage (TXT_CANT_OPEN_DOOR);
		return WHP_NO_KEY;
		}
	else {
		if (state != WALL_DOOR_OPENING) {
			SEGMENT (nSegment)->OpenDoor (nSide);
			if (IsMultiGame)
				MultiSendDoorOpen (nSegment, nSide, flags);
			}
		return WHP_DOOR;
		}
	}
return WHP_NOT_SPECIAL;		//default is treat like Normal CWall
}

//------------------------------------------------------------------------------

void CWall::Init (void)
{
nType = WALL_NORMAL;
flags = 0;
hps = 0;
nTrigger = -1;
nClip = -1;
}

//------------------------------------------------------------------------------
// Tidy up WALLS array for load/save purposes.
void ResetWalls (void)
{
gameData.wallData.cloaking.Reset ();
gameData.wallData.exploding.Reset ();
gameData.wallData.activeDoors.Reset ();
}

//------------------------------------------------------------------------------

bool DoCloakingWallFrame (int32_t nCloakingWall)
{
	CCloakingWall*	cloakWallP;
	CWall*			frontWallP,*backWallP;
	bool				bDeleted = false;

if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return false;
cloakWallP = gameData.wallData.cloaking + nCloakingWall;
frontWallP = WALL (cloakWallP->nFrontWall);
backWallP = IS_WALL (cloakWallP->nBackWall) ? WALL (cloakWallP->nBackWall) : NULL;
cloakWallP->time += gameData.time.xFrame;
if (cloakWallP->time > CLOAKING_WALL_TIME) {
	frontWallP->nType = WALL_OPEN;
	if (SHOW_DYN_LIGHT)
		lightManager.Toggle (frontWallP->nSegment, frontWallP->nSide, -1, 0);
	frontWallP->state = WALL_DOOR_CLOSED;		//why closed? why not?
	if (backWallP) {
		backWallP->nType = WALL_OPEN;
		if (SHOW_DYN_LIGHT)
			lightManager.Toggle (backWallP->nSegment, backWallP->nSide, -1, 0);
		backWallP->state = WALL_DOOR_CLOSED;		//why closed? why not?
		}
	DeleteCloakingWall (nCloakingWall);
	audio.Update ();
	}
else if (SHOW_DYN_LIGHT || (cloakWallP->time > CLOAKING_WALL_TIME / 2)) {
	int32_t oldType = frontWallP->nType;
#if 1
	frontWallP->cloakValue = int8_t (float (FADE_LEVELS) * X2F (cloakWallP->time));
#else
	if (SHOW_DYN_LIGHT)
		frontWallP->cloakValue = (fix) FRound (FADE_LEVELS * X2F (cloakWallP->time));
	else
		frontWallP->cloakValue = ((cloakWallP->time - CLOAKING_WALL_TIME / 2) * (FADE_LEVELS - 2)) / (CLOAKING_WALL_TIME / 2);
#endif
	if (backWallP)
		backWallP->cloakValue = frontWallP->cloakValue;
	if (oldType != WALL_CLOAKED) {		//just switched
		frontWallP->nType = WALL_CLOAKED;
		if (backWallP)
			backWallP->nType = WALL_CLOAKED;
		if (!SHOW_DYN_LIGHT) {
			for (int32_t i = 0; i < 4; i++) {
				SEGMENT (frontWallP->nSegment)->m_sides [frontWallP->nSide].m_uvls [i].l = cloakWallP->front_ls [i];
				if (backWallP)
					SEGMENT (backWallP->nSegment)->m_sides [backWallP->nSide].m_uvls [i].l = cloakWallP->back_ls [i];
				}
			}
		}
	}
else {		//fading out
	fix xLightScale = FixDiv (CLOAKING_WALL_TIME / 2 - cloakWallP->time, CLOAKING_WALL_TIME / 2);
	for (int32_t i = 0; i < 4; i++) {
		SEGMENT (frontWallP->nSegment)->m_sides [frontWallP->nSide].m_uvls [i].l = FixMul (cloakWallP->front_ls [i], xLightScale);
		if (backWallP)
			SEGMENT (backWallP->nSegment)->m_sides [backWallP->nSide].m_uvls [i].l = FixMul (cloakWallP->back_ls [i], xLightScale);
		}
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		tUVL *uvlP = SEGMENT (frontWallP->nSegment)->m_sides [frontWallP->nSide].m_uvls;
		NDRecordCloakingWall (cloakWallP->nFrontWall, cloakWallP->nBackWall, frontWallP->nType, frontWallP->state, frontWallP->cloakValue,
									 uvlP [0].l, uvlP [1].l, uvlP [2].l, uvlP [3].l);
		}
	}
return bDeleted;
}

//------------------------------------------------------------------------------

void DoDecloakingWallFrame (int32_t nCloakingWall)
{
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return;

CCloakingWall* cloakWallP = gameData.wallData.cloaking + nCloakingWall;
CWall* frontWallP = WALL (cloakWallP->nFrontWall);
CWall* backWallP = IS_WALL (cloakWallP->nBackWall) ? WALL (cloakWallP->nBackWall) : NULL;

cloakWallP->time += gameData.time.xFrame;
if (cloakWallP->time > CLOAKING_WALL_TIME) {
	frontWallP->state = WALL_DOOR_CLOSED;
	if (backWallP)
		backWallP->state = WALL_DOOR_CLOSED;
	for (uint32_t i = 0; i < 4; i++) {
		SEGMENT (frontWallP->nSegment)->m_sides [frontWallP->nSide].m_uvls [i].l = cloakWallP->front_ls [i];
		if (backWallP)
			SEGMENT (backWallP->nSegment)->m_sides [backWallP->nSide].m_uvls [i].l = cloakWallP->back_ls [i];
		}
		DeleteCloakingWall (nCloakingWall);
	}
else if (cloakWallP->time > CLOAKING_WALL_TIME / 2) {		//fading in
	frontWallP->nType = WALL_CLOSED;
	if (SHOW_DYN_LIGHT)
		lightManager.Toggle (frontWallP->nSegment, frontWallP->nSide, -1, 0);
	if (backWallP) {
		backWallP->nType = WALL_CLOSED;
		if (SHOW_DYN_LIGHT)
			lightManager.Toggle (backWallP->nSegment, backWallP->nSide, -1, 0);
		}
	fix xLightScale = FixDiv (cloakWallP->time-CLOAKING_WALL_TIME/2,CLOAKING_WALL_TIME / 2);
	for (int32_t i = 0; i < 4; i++) {
		SEGMENT (frontWallP->nSegment)->m_sides [frontWallP->nSide].m_uvls [i].l = FixMul (cloakWallP->front_ls [i], xLightScale);
		if (backWallP)
			SEGMENT (backWallP->nSegment)->m_sides [backWallP->nSide].m_uvls [i].l = FixMul (cloakWallP->back_ls [i], xLightScale);
		}
	}
else {		//cloaking in
	frontWallP->cloakValue = ((CLOAKING_WALL_TIME / 2 - cloakWallP->time) * (FADE_LEVELS - 2)) / (CLOAKING_WALL_TIME / 2);
	frontWallP->nType = WALL_CLOAKED;
	if (backWallP) {
		backWallP->cloakValue = frontWallP->cloakValue;
		backWallP->nType = WALL_CLOAKED;
		}
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	tUVL *uvlP = SEGMENT (frontWallP->nSegment)->m_sides [frontWallP->nSide].m_uvls;
	NDRecordCloakingWall (cloakWallP->nFrontWall, cloakWallP->nBackWall, frontWallP->nType, frontWallP->state, frontWallP->cloakValue,
								 uvlP [0].l, uvlP [1].l, uvlP [2].l, uvlP [3].l);
	}
}

//------------------------------------------------------------------------------

void WallFrameProcess (void)
{
	CCloakingWall*	cloakWallP;
	CActiveDoor*	doorP = gameData.wallData.activeDoors.Buffer ();
	CWall*			wallP, *backWallP;
	uint32_t				i;

for (i = 0; i < gameData.wallData.activeDoors.ToS (); i++) {
	doorP = &gameData.wallData.activeDoors [i];
	backWallP = NULL,
	wallP = WALL (doorP->nFrontWall [0]);
	if (wallP->state == WALL_DOOR_OPENING) {
		if (DoDoorOpen (i))
			i--;
		}
	else if (wallP->state == WALL_DOOR_CLOSING) {
		if (DoDoorClose (i))
			i--;	// active door has been deleted - account for it
		}
	else if (wallP->state == WALL_DOOR_WAITING) {
		doorP->time += gameData.time.xFrame;

		//set flags to fix occatsional netgame problem where door is
		//waiting to close but open flag isn't set
//			Assert(doorP->nPartCount == 1);
		if (IS_WALL (doorP->nBackWall [0]))
			WALL (doorP->nBackWall [0])->state = WALL_DOOR_CLOSING;
		if ((doorP->time > DOOR_WAIT_TIME) && !SEGMENT (wallP->nSegment)->DoorIsBlocked (wallP->nSide, wallP->IgnoreMarker ())) {
			wallP->state = WALL_DOOR_CLOSING;
			doorP->time = 0;
			}
		else {
			wallP->flags |= WALL_DOOR_OPENED;
			if (backWallP)
				backWallP->flags |= WALL_DOOR_OPENED;
			}
		}
	else if ((wallP->state == WALL_DOOR_CLOSED) || (wallP->state == WALL_DOOR_OPEN)) {
		//this shouldn't happen.  if the CWall is in one of these states,
		//there shouldn't be an activedoor entry for it.  So we'll kill
		//the activedoor entry.  Tres simple.
		DeleteActiveDoor (i--);
		}
	}

cloakWallP = gameData.wallData.cloaking.Buffer ();
for (i = 0; i < gameData.wallData.cloaking.ToS (); i++, cloakWallP++) {
	uint8_t s = WALL (cloakWallP->nFrontWall)->state;
	if (s == WALL_DOOR_CLOAKING) {
		if (DoCloakingWallFrame (i))
			i--;	// cloaking wall has been deleted from cloaking wall tracker
		}
	else if (s == WALL_DOOR_DECLOAKING)
		DoDecloakingWallFrame (i);
#if DBG
	else
		Int3();	//unexpected CWall state
#endif
	}
}

//------------------------------------------------------------------------------
//	An CObject got stuck in a door (like a flare).
//	Add global entry.
int32_t				nStuckObjects = 0;
tStuckObject	stuckObjects [MAX_STUCK_OBJECTS];

void AddStuckObject (CObject *objP, int16_t nSegment, int16_t nSide)
{
	int32_t				i;
	int16_t				nWall;
	tStuckObject	*stuckObjP;

CWall* wallP = SEGMENT (nSegment)->Wall (nSide);
if (wallP) {
	if (wallP->flags & WALL_BLASTED)
		objP->Die ();
	nWall = wallP - WALLS;
	for (i = 0, stuckObjP = stuckObjects; i < MAX_STUCK_OBJECTS; i++, stuckObjP++) {
		if (stuckObjP->nWall == NO_WALL) {
			stuckObjP->nWall = nWall;
			stuckObjP->nObject = objP->Index ();
			stuckObjP->nSignature = objP->info.nSignature;
			nStuckObjects++;
			break;
			}
		}
	}
}

//	--------------------------------------------------------------------------------------------------
//	Look at the list of stuck OBJECTS, clean up in case an CObject has gone away, but not been removed here.
//	Removes up to one/frame.
void RemoveObsoleteStuckObjects (void)
{
	int16_t	nObject, nWall;
	CObject			*objP;
	tStuckObject	*stuckObjP;

	//	Safety and efficiency code.  If no stuck OBJECTS, should never get inside the IF, but this is faster.
if (!nStuckObjects)
	return;
nObject = gameData.app.nFrameCount % MAX_STUCK_OBJECTS;
stuckObjP = stuckObjects + nObject;
objP = OBJECT (stuckObjP->nObject);
nWall = stuckObjP->nWall;
if (IS_WALL (nWall) &&
	 ((WALL (nWall)->state != WALL_DOOR_CLOSED) ||
	  (objP->info.nSignature != stuckObjP->nSignature))) {
	nStuckObjects--;
	objP->UpdateLife (I2X (1) / 8);
	stuckObjP->nWall = NO_WALL;
	}
}

//	----------------------------------------------------------------------------------------------------
//	Door with CWall index nWall is opening, kill all OBJECTS stuck in it.
void KillStuckObjects (int32_t nWall)
{
	int32_t				i;
	tStuckObject	*stuckObjP;
	CObject			*objP;

if (!IS_WALL (nWall) || (nStuckObjects == 0))
	return;
nStuckObjects = 0;

for (i = 0, stuckObjP = stuckObjects; i < MAX_STUCK_OBJECTS; i++, stuckObjP++)
	if (stuckObjP->nWall == nWall) {
		objP = OBJECT (stuckObjP->nObject);
		if (objP->info.nType == OBJ_WEAPON)
			objP->UpdateLife (I2X (1) / 8);
		else {
#if TRACE
			console.printf (1,
				"Warning: Stuck CObject of nType %i, expected to be of nType %i, see CWall.c\n",
				objP->info.nType, OBJ_WEAPON);
#endif
			// Int3();	//	What?  This looks bad.  Object is not a weapon and it is stuck in a CWall!
			stuckObjP->nWall = NO_WALL;
			}
		}
	else if (IS_WALL (stuckObjP->nWall)) {
		nStuckObjects++;
	}
//	Ok, this is awful, but we need to do things whenever a door opens/closes/disappears, etc.
simpleRouter [0].Flush ();
}

// -----------------------------------------------------------------------------------
// Initialize stuck OBJECTS array.  Called at start of level
void InitStuckObjects (void)
{
for (int32_t i = 0; i < MAX_STUCK_OBJECTS; i++)
	stuckObjects [i].nWall = NO_WALL;
nStuckObjects = 0;
}

// -----------------------------------------------------------------------------------
// Clear out all stuck OBJECTS.  Called for a new ship
void ClearStuckObjects (void)
{
	tStuckObject	*stuckObjP = stuckObjects;
	CObject			*objP;

for (int32_t i = 0; i < MAX_STUCK_OBJECTS; i++, stuckObjP++) {
	if (IS_WALL (stuckObjP->nWall)) {
		objP = OBJECT (stuckObjP->nObject);
		if ((objP->info.nType == OBJ_WEAPON) && (objP->info.nId == FLARE_ID))
			objP->UpdateLife (I2X (1) / 8);
		stuckObjP->nWall = NO_WALL;
		nStuckObjects--;
		}
	}
if (nStuckObjects)
	nStuckObjects = 0;
}

// -----------------------------------------------------------------------------------
#define	MAX_BLAST_GLASS_DEPTH	5

void BngProcessSegment (CObject *objP, fix damage, CSegment *segP, int32_t depth, int8_t *visited)
{
	int32_t	i;
	int16_t	nSide;

if (depth > MAX_BLAST_GLASS_DEPTH)
	return;

depth++;

for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
	if (!segP->Side (nSide)->FaceCount ())
		continue;
		
		int32_t			tm;
		fix			dist;
		CFixVector	pnt;

	//	Process only walls which have glass.
	if ((tm = segP->m_sides [nSide].m_nOvlTex)) {
		int32_t ec = gameData.pig.tex.tMapInfoP [tm].nEffectClip;
		tEffectInfo* effectInfoP = (ec < 0) ? NULL : gameData.effects.effectP + ec;
		int32_t db = effectInfoP ? effectInfoP->destroyed.nTexture : -1;

		if (((ec != -1) && (db != -1) && !(effectInfoP->flags & EF_ONE_SHOT)) ||
		 	 ((ec == -1) && (gameData.pig.tex.tMapInfoP [tm].destroyed != -1))) {
			pnt = segP->SideCenter (nSide);
			dist = CFixVector::Dist(pnt, objP->info.position.vPos);
			if (dist < damage / 2) {
				dist = simpleRouter [0].PathLength (pnt, segP->Index (), objP->info.position.vPos, objP->info.nSegment, MAX_BLAST_GLASS_DEPTH, WID_TRANSPARENT_FLAG, -1);
				if ((dist > 0) && (dist < damage / 2) &&
					 segP->BlowupTexture (nSide, pnt, (objP->cType.laserInfo.parent.nObject < 0) ? NULL : OBJECT (objP->cType.laserInfo.parent.nObject), 1))
						segP->OperateTrigger (nSide, OBJECT (objP->cType.laserInfo.parent.nObject), 1);
				}
			}
		}
	}

for (i = 0; i < SEGMENT_SIDE_COUNT; i++) {
	int16_t nSegment = segP->m_children [i];

	if ((nSegment != -1) && !visited [nSegment] && (segP->IsPassable (i, NULL) & WID_PASSABLE_FLAG)) {
		visited [nSegment] = 1;
		BngProcessSegment (objP, damage, SEGMENT (nSegment), depth, visited);
		}
	}
}

// -----------------------------------------------------------------------------------

int32_t CWall::IsTriggerTarget (int32_t i)
{
CTrigger *triggerP = GEOTRIGGERS.Buffer (i);
for (; i < gameData.trigData.m_nTriggers [0]; i++, triggerP++) {
	int16_t *nSegP = triggerP->m_segments;
	int16_t *nSideP = triggerP->m_sides;
	for (int32_t j = triggerP->m_nLinks; j; j--, nSegP++, nSideP++)
		if ((*nSegP == nSegment) && (*nSideP == nSide))
			return i;
	}
return -1;
}

// -----------------------------------------------------------------------------------

bool CWall::IsVolatile (void)
{
if (bVolatile < 0) {
	bVolatile = 0;
	if ((nType == WALL_OPEN) || (nType == WALL_DOOR) || (nType == WALL_BLASTABLE) || (nType == WALL_COLORED))
		bVolatile = 1;
	else if ((nType == WALL_CLOAKED) && (cloakValue < FADE_LEVELS))
		bVolatile = 1;
	else {
		CTrigger *triggerP = GEOTRIGGERS.Buffer ();
		for (int32_t i = 0; i < gameData.trigData.m_nTriggers [0]; i++, triggerP++) {
			int16_t *nSegP = triggerP->m_segments;
			int16_t *nSideP = triggerP->m_sides;
			for (int32_t j = triggerP->m_nLinks; j; j--, nSegP++, nSideP++) {
				if ((*nSegP != nSegment) && (*nSideP != nSide))
					continue;
				if ((triggerP->m_info.nType == TT_OPEN_DOOR) || (triggerP->m_info.nType == TT_ILLUSION_OFF) || (triggerP->m_info.nType == TT_OPEN_WALL)) {
					bVolatile = 1;
					break;
					}
				}
			}
		}
	}
return (bVolatile > 0);
}

// -----------------------------------------------------------------------------------

CWall* CWall::OppositeWall (void)
{
CSide* sideP = SEGMENT (nSegment)->OppositeSide (nSide);
return sideP ? sideP->Wall () : NULL;
}

// -----------------------------------------------------------------------------------

bool CWall::IsInvisible (void)
{
if ((nType != WALL_OPEN) && ((nType != WALL_CLOAKED) || (cloakValue <  FADE_LEVELS)))
	return false;
if (IsTriggerTarget () >= 0)
	return false;
CWall* wallP = OppositeWall ();
return (wallP != NULL) && (wallP->IsTriggerTarget () < 0);
}

// -----------------------------------------------------------------------------------
//	objP is going to detonate
//	blast nearby monitors, lights, maybe other things
void BlastNearbyGlass (CObject *objP, fix damage)
{
	int32_t		i;
	int8_t		visited [MAX_SEGMENTS_D2X];
	CSegment	*cursegp;

cursegp = SEGMENT (objP->info.nSegment);
for (i=0; i<=gameData.segData.nLastSegment; i++)
memset (visited, 0, sizeof (visited));
visited [objP->info.nSegment] = 1;
BngProcessSegment(objP, damage, cursegp, 0, visited);
}

// -----------------------------------------------------------------------------------

#define MAX_WALL_EFFECT_FRAMES_D1 20

// -----------------------------------------------------------------------------------

void ReadWallEffectData (tWallEffect& we, CFile& cf, int16_t nFrames)
{
we.xTotalTime = cf.ReadFix ();
we.nFrameCount = cf.ReadShort ();
for (int32_t i = 0; i < nFrames; i++)
	we.frames [i] = cf.ReadShort ();
we.openSound = cf.ReadShort ();
we.closeSound = cf.ReadShort ();
we.flags = cf.ReadShort ();
cf.Read (we.filename, 13, 1);
we.pad = cf.ReadByte ();
}

// -----------------------------------------------------------------------------------

/*
 * reads a tWallEffect structure from a CFile
 */
int32_t ReadWallEffectInfoD1 (tWallEffect *wc, int32_t n, CFile& cf)
{
int32_t i;

for (i = 0; i < n; i++)
	ReadWallEffectData (wc [i], cf, MAX_WALL_EFFECT_FRAMES_D1);
return i;
}

// -----------------------------------------------------------------------------------

/*
 * reads a tWallEffect structure from a CFile
 */
int32_t ReadWallEffectInfo (CArray<tWallEffect>& wc, int32_t n, CFile& cf)
{
	int32_t i;

for (i = 0; i < n; i++) 
	ReadWallEffectData (wc [i], cf, MAX_WALL_EFFECT_FRAMES);
return i;
}

// -----------------------------------------------------------------------------------
/*
 * reads a tWallV16 structure from a CFile
 */
void ReadWallV16(tWallV16&  w, CFile& cf)
{
w.nType = cf.ReadByte ();
w.flags = cf.ReadByte ();
w.hps = cf.ReadFix ();
w.nTrigger = (uint8_t) cf.ReadByte ();
w.nClip = cf.ReadByte ();
w.keys = cf.ReadByte ();
}

// -----------------------------------------------------------------------------------
/*
 * reads a tWallV19 structure from a CFile
 */
void ReadWallV19 (tWallV19& w, CFile& cf)
{
w.nSegment = cf.ReadInt ();
w.nSide = cf.ReadInt ();
w.nType = cf.ReadByte ();
w.flags = cf.ReadByte ();
w.hps = cf.ReadFix ();
w.nTrigger = (uint8_t) cf.ReadByte ();
w.nClip = cf.ReadByte ();
w.keys = cf.ReadByte ();
w.nLinkedWall = cf.ReadInt ();
}

// -----------------------------------------------------------------------------------
/*
 * reads a CWall structure from a CFile
 */
void CWall::Read (CFile& cf)
{
nSegment = cf.ReadInt ();
nSide = cf.ReadInt ();
hps = cf.ReadFix ();
nLinkedWall = cf.ReadInt ();
nType = cf.ReadByte ();
if (gameTopFileInfo.fileinfoVersion < 37)
	flags = uint16_t (cf.ReadByte ());
else
	flags = cf.ReadUShort ();
state = cf.ReadByte ();
nTrigger = uint8_t (cf.ReadByte ());
nClip = cf.ReadByte ();
keys = cf.ReadByte ();
controllingTrigger = cf.ReadByte ();
cloakValue = cf.ReadByte ();
bVolatile = -1;
}

//------------------------------------------------------------------------------

void CWall::SaveState (CFile& cf)
{
cf.WriteInt (nSegment);
cf.WriteInt (nSide);
cf.WriteFix (hps);    
cf.WriteInt (nLinkedWall);
cf.WriteByte ((int8_t) nType);       
cf.WriteShort (int16_t (flags));      
cf.WriteByte (state);      
cf.WriteByte ((int8_t) nTrigger);    
cf.WriteByte (nClip);   
cf.WriteByte ((int8_t) keys);       
cf.WriteByte (controllingTrigger);
cf.WriteByte (cloakValue); 
}

//------------------------------------------------------------------------------

void CWall::LoadState (CFile& cf)
{
nSegment = cf.ReadInt ();
nSide = cf.ReadInt ();
hps = cf.ReadFix ();    
nLinkedWall = cf.ReadInt ();
nType = (uint8_t) cf.ReadByte ();       
if (saveGameManager.Version () < 47)
	flags = uint16_t (cf.ReadByte ());
else
	flags = uint16_t (cf.ReadShort ());
state = (uint8_t) cf.ReadByte ();      
nTrigger = (uint8_t) cf.ReadByte ();    
nClip = cf.ReadByte ();   
keys = (uint8_t) cf.ReadByte ();       
controllingTrigger = cf.ReadByte ();
cloakValue = cf.ReadByte (); 
bVolatile = -1;
}

// -----------------------------------------------------------------------------------
/*
 * reads a v19_door structure from a CFile
 */
void ReadActiveDoorV19 (v19_door& d, CFile& cf)
{
d.nPartCount = cf.ReadInt ();
d.seg [0] = cf.ReadShort ();
d.seg [1] = cf.ReadShort ();
d.nSide [0] = cf.ReadShort ();
d.nSide [1] = cf.ReadShort ();
d.nType [0] = cf.ReadShort ();
d.nType [1] = cf.ReadShort ();
d.open = cf.ReadFix ();
}

// -----------------------------------------------------------------------------------
/*
 * reads an CActiveDoor structure from a CFile
 */
void ReadActiveDoor (CActiveDoor& d, CFile& cf)
{
d.nPartCount = cf.ReadInt ();
d.nFrontWall [0] = cf.ReadShort ();
d.nFrontWall [1] = cf.ReadShort ();
d.nBackWall [0] = cf.ReadShort ();
d.nBackWall [1] = cf.ReadShort ();
d.time = cf.ReadFix ();
}

// -----------------------------------------------------------------------------------

bool CWall::IsOpenableDoor (void)
{
return (nType == WALL_DOOR) && (keys == KEY_NONE) && (state == WALL_DOOR_CLOSED) && !(flags & WALL_DOOR_LOCKED);
}

// -----------------------------------------------------------------------------------

CTrigger* CWall::Trigger (void)
{
return GEOTRIGGER (nTrigger);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CActiveDoor::LoadState (CFile& cf)
{
nPartCount = cf.ReadInt ();
for (int32_t i = 0; i < 2; i++) {
	nFrontWall [i] = cf.ReadShort ();
	nBackWall [i] = cf.ReadShort ();
	}
time = cf.ReadFix ();    
}

//------------------------------------------------------------------------------

void CActiveDoor::SaveState (CFile& cf)
{
cf.WriteInt (nPartCount);
for (int32_t i = 0; i < 2; i++) {
	cf.WriteShort (nFrontWall [i]);
	cf.WriteShort (nBackWall [i]);
	}
cf.WriteFix (time);   
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CCloakingWall::LoadState (CFile& cf)
{
nFrontWall = cf.ReadShort ();
nBackWall = cf.ReadShort (); 
for (int32_t i = 0; i < 4; i++) {
	front_ls [i] = cf.ReadFix (); 
	back_ls [i] = cf.ReadFix ();
	}
time = cf.ReadFix ();    
}

//------------------------------------------------------------------------------

void CCloakingWall::SaveState (CFile& cf)
{
cf.WriteShort (nFrontWall);
cf.WriteShort (nBackWall); 
for (int32_t i = 0; i < 4; i++) {
	cf.WriteFix (front_ls [i]); 
	cf.WriteFix (back_ls [i]);
	}
cf.WriteFix (time);    
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CExplodingWall::LoadState (CFile& cf)
{
nSegment = cf.ReadInt ();
nSide = cf.ReadInt ();
time = cf.ReadFix ();    
}

//------------------------------------------------------------------------------

void CExplodingWall::SaveState (CFile& cf)
{
cf.WriteInt (nSegment);
cf.WriteInt (nSide);
cf.WriteFix (time);    
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define EXPL_WALL_TIME					(I2X (1))
#define EXPL_WALL_TOTAL_FIREBALLS	32
#define EXPL_WALL_FIREBALL_SIZE 		(0x48000*6/10)	//smallest size

void InitExplodingWalls (void)
{
gameData.wallData.exploding.Reset ();
}

//------------------------------------------------------------------------------
//explode the given CWall
void ExplodeWall (int16_t nSegment, int16_t nSide)
{
	CExplodingWall * explodingWallP = AddExplodingWall ();

if (explodingWallP) {
	explodingWallP->nSegment = nSegment;
	explodingWallP->nSide = nSide;
	explodingWallP->time = 0;
	SEGMENT (nSegment)->CreateSound (SOUND_EXPLODING_WALL, nSide);
	}
}

//------------------------------------------------------------------------------
//handle walls for this frame
//note: this CWall code assumes the CWall is not triangulated
void DoExplodingWallFrame (void)
{
for (uint32_t i = 0; i < gameData.wallData.exploding.ToS (); ) {
	int16_t nSegment = gameData.wallData.exploding [i].nSegment;
	if (nSegment < 0) {
		DeleteExplodingWall (i);
		continue;
		}
	int16_t nSide = gameData.wallData.exploding [i].nSide;
	fix oldfrac, newfrac;
	int32_t oldCount, newCount, e;		//n,
	oldfrac = FixDiv (gameData.wallData.exploding [i].time, EXPL_WALL_TIME);
	gameData.wallData.exploding [i].time += gameData.time.xFrame;
	if (gameData.wallData.exploding [i].time > EXPL_WALL_TIME)
		gameData.wallData.exploding [i].time = EXPL_WALL_TIME;
	else if (gameData.wallData.exploding [i].time > 3 * EXPL_WALL_TIME / 4) {
		CSegment *segP = SEGMENT (nSegment),
					*connSegP = SEGMENT (segP->m_children [nSide]);
		uint8_t	a = (uint8_t) segP->Wall (nSide)->nClip;
		int16_t n = WallEffectFrameCount (gameData.wallData.animP + a);
		int16_t nConnSide = segP->ConnectedSide (connSegP);
		segP->SetTexture (nSide, connSegP, nConnSide, a, n - 1);
		segP->Wall (nSide)->flags |= WALL_BLASTED;
		if (nConnSide >= 0) {
			CWall* wallP = connSegP->Wall (nConnSide);
			if (wallP)
				wallP->flags |= WALL_BLASTED;
			}
		}
	newfrac = FixDiv (gameData.wallData.exploding [i].time, EXPL_WALL_TIME);
	oldCount = X2I (EXPL_WALL_TOTAL_FIREBALLS * FixMul (oldfrac, oldfrac));
	newCount = X2I (EXPL_WALL_TOTAL_FIREBALLS * FixMul (newfrac, newfrac));
	//n = newCount - oldCount;
	//now create all the next explosions
	for (e = oldCount; e < newCount; e++) {
		uint16_t*		corners;
		CFixVector	*v0, *v1, *v2;
		CFixVector	vv0, vv1, vPos;
		fix			size;

		//calc expl position

		corners = SEGMENT (nSegment)->Corners (nSide);
		v0 = gameData.segData.vertices + corners [0];
		v1 = gameData.segData.vertices + corners [1];
		v2 = gameData.segData.vertices + corners [2];
		vv0 = *v0 - *v1;
		vv1 = *v2 - *v1;
		vPos = *v1 + vv0 * (RandShort () * 2);
		vPos += vv1 * (RandShort ()*2);
		size = EXPL_WALL_FIREBALL_SIZE + (2*EXPL_WALL_FIREBALL_SIZE * e / EXPL_WALL_TOTAL_FIREBALLS);
		//fireballs start away from door, with subsequent ones getting closer
		vPos += SEGMENT (nSegment)->m_sides [nSide].m_normals [0] * (size * (EXPL_WALL_TOTAL_FIREBALLS - e) / EXPL_WALL_TOTAL_FIREBALLS);
		if (e & 3)		//3 of 4 are Normal
			CreateExplosion ((int16_t) gameData.wallData.exploding [i].nSegment, vPos, size, (uint8_t) ANIM_SMALL_EXPLOSION);
		else
			CreateSplashDamageExplosion (NULL, (int16_t) gameData.wallData.exploding [i].nSegment, vPos, vPos,
										        size, (uint8_t) ANIM_SMALL_EXPLOSION, I2X (4), I2X (20), I2X (50), -1);
		}
	if (gameData.wallData.exploding [i].time >= EXPL_WALL_TIME)
		DeleteExplodingWall (i);
	else
		i++;
	}
}

//------------------------------------------------------------------------------

void SetupWalls (void)
{
	CWall* wallP = WALLS.Buffer ();

for (int32_t i = 0; i < gameData.wallData.nWalls; i++, wallP++) {
	if ((wallP->nType == WALL_DOOR) && (wallP->flags & WALL_DOOR_OPENED)) {
		if ((wallP->flags & WALL_DOOR_AUTO) && !FindActiveDoor (i)) {	// make sure pre-opened automatic doors will close after a while
			wallP->flags &= ~WALL_DOOR_OPENED;
			SEGMENT (wallP->nSegment)->OpenDoor (wallP->nSide);
			}
		SEGMENT (wallP->nSegment)->AnimateOpeningDoor (wallP->nSide, -1);
		}
	else if ((wallP->nType == WALL_BLASTABLE) && (wallP->flags & WALL_BLASTED))
		SEGMENT (wallP->nSegment)->BlastWall (wallP->nSide);
	}
}

//------------------------------------------------------------------------------

void FixWalls (void)
{
	CWall*			wallP;
	uint32_t				i, j;
	uint8_t				bActive [MAX_WALLS];

memset (bActive, 0, sizeof (bActive));
for (i = 0; i < gameData.wallData.activeDoors.ToS (); i++)
	bActive [gameData.wallData.activeDoors [i].nFrontWall [0]] = 1;

for (i = 0; i < gameData.wallData.cloaking.ToS (); i++)
	bActive [gameData.wallData.cloaking [i].nFrontWall] |= 2;

wallP = gameData.wallData.walls.Buffer ();
for (i = 0, j = gameData.wallData.walls.Length (); i < j; i++, wallP++) {
	switch (wallP->state) {
		case WALL_DOOR_OPENING:
			if (bActive [i] & 1)
				continue;
			wallP->state = WALL_DOOR_CLOSED;
			SEGMENT (wallP->nSegment)->OpenDoor (wallP->nSide);
			break;

		case WALL_DOOR_WAITING:
		case WALL_DOOR_CLOSING:
			if (bActive [i] & 1)
				continue;
			wallP->state = WALL_DOOR_OPEN;
			SEGMENT (wallP->nSegment)->CloseDoor (wallP->nSide);
			break;

		case WALL_DOOR_CLOAKING:
			if (bActive [i] & 2)
				continue;
			wallP->state = 0;
			SEGMENT (wallP->nSegment)->StartCloak (wallP->nSide);
			break;

		case WALL_DOOR_DECLOAKING:
			if (bActive [i] & 2)
				continue;
			wallP->state = 0;
			SEGMENT (wallP->nSegment)->StartDecloak (wallP->nSide);
			break;

		default:
			continue;
		}
	}
}

// -----------------------------------------------------------------------------
