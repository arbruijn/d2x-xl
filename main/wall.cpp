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
#include "gameseg.h"
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

//	Special door on boss level which is locked if not in multiplayer...sorry for this awful solution --MK.
#define	BOSS_LOCKED_DOOR_LEVEL	7
#define	BOSS_LOCKED_DOOR_SEG		595
#define	BOSS_LOCKED_DOOR_SIDE	5

//--unused-- int gameData.walls.bitmaps [MAX_WALL_ANIMS];

//door Doors [MAX_DOORS];


#define CLOAKING_WALL_TIME I2X (1)

//--unused-- CBitmap *wall_title_bms [MAX_WALL_ANIMS];

//#define BM_FLAG_TRANSPARENT			1
//#define BM_FLAG_SUPER_TRANSPARENT	2

//------------------------------------------------------------------------------

void FlushFCDCache (void);

// Opens a door, including animation and other processing.
bool DoDoorOpen (int nDoor);

// Closes a door, including animation and other processing.
bool DoDoorClose (int nDoor);

//------------------------------------------------------------------------------

int AnimFrameCount (tWallClip *anim)
{
	int	n;

CBitmap *bmP = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [anim->frames [0]].index;
if (bmP->Override ())
	bmP = bmP->Override ();
n = (bmP->Type () == BM_TYPE_ALT) ? bmP->FrameCount () : anim->nFrameCount;
return (n > 1) ? n : anim->nFrameCount;
}

//------------------------------------------------------------------------------

fix AnimPlayTime (tWallClip *anim)
{
int nFrames = AnimFrameCount (anim);
fix pt = (fix) (anim->xTotalTime * gameStates.gameplay.slowmo [0].fSpeed);

if (nFrames == anim->nFrameCount)
	return pt;
return (fix) (((double) pt * (double) anim->nFrameCount) / (double) nFrames);
}

// This function determines whether the current CSegment/nSide is transparent
//		1 = YES
//		0 = NO
//------------------------------------------------------------------------------

CActiveDoor* FindActiveDoor (short nWall)
{
	CActiveDoor* doorP = gameData.walls.activeDoors.Buffer ();

for (uint i = gameData.walls.activeDoors.ToS (); i; i--, doorP++) {		//find door
	for (int j = 0; j < doorP->nPartCount; j++)
		if ((doorP->nFrontWall [j] == nWall) || (doorP->nBackWall [j] == nWall))
			return doorP;
	}
return NULL;
}

//------------------------------------------------------------------------------
// This function checks whether we can fly through the given nSide.
//	In other words, whether or not we have a 'doorway'
//	 Flags:
//		WID_FLY_FLAG				1
//		WID_RENDER_FLAG			2
//		WID_RENDPAST_FLAG			4
//	 Return values:
//		WID_WALL						2	// 0/1/0		CWall
//		WID_TRANSPARENT_WALL		6	//	0/1/1		transparent CWall
//		WID_ILLUSORY_WALL			3	//	1/1/0		illusory CWall
//		WID_TRANSILLUSORY_WALL	7	//	1/1/1		transparent illusory CWall
//		WID_NO_WALL					5	//	1/0/1		no CWall, can fly through
int CWall::IsDoorWay (CObject *objP)
{
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (nType == WALL_OPEN)
	return WID_NO_WALL;

if (nType == WALL_ILLUSION) {
	if (flags & WALL_ILLUSION_OFF)
		return WID_NO_WALL;
	if ((cloakValue < FADE_LEVELS) || SEGMENTS [nSegment].CheckTransparency (nSide))
		return WID_TRANSILLUSORY_WALL;
	return WID_ILLUSORY_WALL;
	}

if (nType == WALL_BLASTABLE) {
	if (flags & WALL_BLASTED)
		return WID_TRANSILLUSORY_WALL;
	if ((cloakValue < FADE_LEVELS) || SEGMENTS [nSegment].CheckTransparency (nSide))
		return WID_TRANSPARENT_WALL;
	return WID_WALL;
	}

if (flags & WALL_DOOR_OPENED)
	return WID_TRANSILLUSORY_WALL;

if (nType == WALL_CLOAKED)
	return WID_RENDER_FLAG | WID_RENDPAST_FLAG | WID_CLOAKED_FLAG;

if (nType == WALL_TRANSPARENT)
	return (hps < 0) ?
			 WID_RENDER_FLAG | WID_RENDPAST_FLAG | WID_TRANSPARENT_FLAG | WID_FLY_FLAG :
			 WID_RENDER_FLAG | WID_RENDPAST_FLAG | WID_TRANSPARENT_FLAG;

if (nType == WALL_DOOR) {
	if ((state == WALL_DOOR_OPENING) || (state == WALL_DOOR_CLOSING))
		return WID_TRANSPARENT_WALL;
	if ((cloakValue && (cloakValue < FADE_LEVELS)) || SEGMENTS [nSegment].CheckTransparency (nSide))
		return WID_TRANSPARENT_WALL;
	return WID_WALL;
	}
if (nType == WALL_CLOSED) {
	if (objP && (objP->info.nType == OBJ_PLAYER)) {
		if (IsTeamGame && ((keys >> 1) == GetTeam (objP->info.nId) + 1))
			return WID_ILLUSORY_WALL;
		if ((keys == KEY_BLUE) && (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_BLUE_KEY))
			return WID_ILLUSORY_WALL;
		if ((keys == KEY_RED) && (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_RED_KEY))
			return WID_ILLUSORY_WALL;
		}
	}
// If none of the above flags are set, there is no doorway.
if ((cloakValue && (cloakValue < FADE_LEVELS)) || SEGMENTS [nSegment].CheckTransparency (nSide)) 
	return WID_TRANSPARENT_WALL;
return WID_WALL; // There are children behind the door.
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
		doorP->time = (fix) (gameData.walls.animP [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed) - doorP->time;
		if (doorP->time < 0)
			doorP->time = 0;
		}
	}
else if (state != WALL_DOOR_CLOSED)
	return NULL;
if (!doorP) {
	if (!gameData.walls.activeDoors.Grow ())
		return NULL;
	doorP = gameData.walls.activeDoors.Top ();
	doorP->time = 0;
	}
state = WALL_DOOR_OPENING;
return doorP;
}

//------------------------------------------------------------------------------

CCloakingWall* FindCloakingWall (short nWall)
{
	CCloakingWall* cloakWallP = gameData.walls.cloaking.Buffer ();

for (uint i = gameData.walls.cloaking.ToS (); i; i--, cloakWallP++) {		//find door
	if ((cloakWallP->nFrontWall == nWall) || (cloakWallP->nBackWall == nWall))
		return cloakWallP;
	}
return NULL;
}

//------------------------------------------------------------------------------

void DeleteActiveDoor (int nDoor)
{
gameData.walls.activeDoors.Delete (static_cast<uint> (nDoor));
}

//------------------------------------------------------------------------------
// This function closes the specified door and restuckObjPres the closed
//  door texture.  This is called when the animation is done
void CloseDoor (int nDoor)
{
	CActiveDoor*	doorP = gameData.walls.activeDoors + nDoor;

for (int i = doorP->nPartCount; i; )
	WALLS [doorP->nFrontWall [--i]].CloseActiveDoor ();
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
	if (!gameData.walls.cloaking.Grow ()) 
		return NULL;
	cloakWallP = gameData.walls.cloaking.Top ();
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
	if (!gameData.walls.cloaking.Grow ())
		return NULL;
	cloakWallP = gameData.walls.cloaking.Top ();
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
CSegment* segP = SEGMENTS + nSegment;
CSegment* connSegP = SEGMENTS + segP->m_children [nSide];
short nConnSide = segP->ConnectedSide (connSegP);

state = WALL_DOOR_CLOSED;
segP->SetTexture (nSide, NULL, -1, nClip, 0);

CWall* wallP = connSegP->Wall (nConnSide);
if (wallP) {
	wallP->state = WALL_DOOR_CLOSED;
	connSegP->SetTexture (nConnSide, NULL, -1, wallP->nClip, 0);
	}
}

//------------------------------------------------------------------------------

CActiveDoor* CWall::CloseDoor (void)
{
	CActiveDoor* doorP = NULL;

if (state == WALL_DOOR_OPENING) {	//reuse door
	if ((doorP = FindActiveDoor (this - WALLS))) {
		doorP->time = (fix) (gameData.walls.animP [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed) - doorP->time;
		if (doorP->time < 0)
			doorP->time = 0;
		}
	}
else if (state != WALL_DOOR_OPEN)
	return NULL;
if (!doorP) {
	if (!gameData.walls.activeDoors.Grow ())
		return NULL;
	doorP = gameData.walls.activeDoors.Top ();
	doorP->time = 0;
	}
state = WALL_DOOR_CLOSING;
return doorP;
}

//------------------------------------------------------------------------------

int CWall::AnimateOpeningDoor (fix xElapsedTime)
{
int nFrames = AnimFrameCount (gameData.walls.animP + nClip);
if (!nFrames)
	return 3;

fix xTotalTime = (fix) (gameData.walls.animP [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed);
fix xFrameTime = xTotalTime / nFrames;
int i = (xElapsedTime < 0) ? nFrames : xElapsedTime / xFrameTime;
CSegment* segP = SEGMENTS + nSegment;
if (i < nFrames)
	segP->SetTexture (nSide, NULL, -1, nClip, i);
if (i > nFrames / 2)
	flags |= WALL_DOOR_OPENED;
if (i < nFrames - 1)
	return 0;
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

int CWall::AnimateClosingDoor (fix xElapsedTime)
{
int nFrames = AnimFrameCount (gameData.walls.animP + nClip);
if (!nFrames)
	return 0;

fix xTotalTime = (fix) (gameData.walls.animP [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed);
fix xFrameTime = xTotalTime / nFrames;
int i = nFrames - xElapsedTime / xFrameTime - 1;
if (i < nFrames / 2)
	flags &= ~WALL_DOOR_OPENED;
if (i <= 0)
	return 1;
SEGMENTS [nSegment].SetTexture (nSide, NULL, -1, nClip, i);
state = WALL_DOOR_CLOSING;
return 0;
}

//------------------------------------------------------------------------------
// Animates opening of a door.
// Called in the game loop.
bool DoDoorOpen (int nDoor)
{
	CActiveDoor*	doorP;
	CWall*			wallP;
	short				nConnSide, nSide;
	CSegment*		connSegP, * segP;
	int				i, nWall, bFlags = 3;

if (nDoor < -1)
	return false;
doorP = gameData.walls.activeDoors + nDoor;
for (i = 0; i < doorP->nPartCount; i++) {
	doorP->time += gameData.time.xFrame;
	nWall = doorP->nFrontWall [i];
	if (!IS_WALL (nWall)) {
		PrintLog ("Trying to open non existant door\n");
		continue;
		}
	wallP = WALLS + doorP->nFrontWall [i];
	KillStuckObjects (doorP->nFrontWall [i]);
	KillStuckObjects (doorP->nBackWall [i]);

	segP = SEGMENTS + wallP->nSegment;
	nSide = wallP->nSide;
	if (!segP->IsWall (nSide)) {
		PrintLog ("Trying to open non existant door @ %d,%d\n", wallP->nSegment, nSide);
		continue;
		}
	bFlags &= segP->AnimateOpeningDoor (nSide, doorP->time);
	connSegP = SEGMENTS + segP->m_children [nSide];
	nConnSide = segP->ConnectedSide (connSegP);
	if (nConnSide < 0) {
		PrintLog ("Door %d @ %d,%d has no oppposite door in %d\n", segP->WallNum (nSide), wallP->nSegment, nSide, segP->m_children [nSide]);
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
bool DoDoorClose (int nDoor)
{
if (nDoor < -1)
	return false;
CActiveDoor* doorP = gameData.walls.activeDoors + nDoor;
CWall* wallP = WALLS + doorP->nFrontWall [0];
CSegment* segP = SEGMENTS + wallP->nSegment;

	int			i, bFlags = 1;
	short			nConnSide, nSide;
	CSegment*	connSegP;

//check for OBJECTS in doorway before closing
if (wallP->flags & WALL_DOOR_AUTO)
	if (segP->DoorIsBlocked (short (wallP->nSide))) {
		audio.DestroySegmentSound (short (wallP->nSegment), short (wallP->nSide), -1);
		segP->OpenDoor (short (wallP->nSide));		//re-open door
		return false;
		}

for (i = 0; i < doorP->nPartCount; i++) {
	wallP = WALLS + doorP->nFrontWall [i];
	segP = SEGMENTS + wallP->nSegment;
	nSide = wallP->nSide;
	if (!segP->IsWall (nSide)) {
#if DBG
		PrintLog ("Trying to close non existant door\n");
#endif
		continue;
		}
	//if here, must be auto door
	//Assert(WALLS [WallNumP (segP, nSide)].flags & WALL_DOOR_AUTO);
	//don't assert here, because now we have triggers to close non-auto doors
	// Otherwise, close it.
	connSegP = SEGMENTS + segP->m_children [nSide];
	nConnSide = segP->ConnectedSide (connSegP);
	if (nConnSide < 0) {
		PrintLog ("Door %d @ %d,%d has no oppposite door in %d\n", 
					 segP->WallNum (nSide), wallP->nSegment, nSide, segP->m_children [nSide]);
		continue;
		}
	if ((gameData.demo.nState != ND_STATE_PLAYBACK) && !(i || doorP->time)) {
		if (gameData.walls.animP [wallP->nClip].closeSound  > -1)
			audio.CreateSegmentSound ((short) gameData.walls.animP [segP->Wall (nSide)->nClip].closeSound,
											  wallP->nSegment, nSide, SEGMENTS [wallP->nSegment].SideCenter (nSide), 0, I2X (1));
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
int AllowToOpenSpecialBossDoor (int nSegment, short nSide)
{
if (IsMultiGame)
	return (gameData.missions.nCurrentLevel == BOSS_LOCKED_DOOR_LEVEL) &&
			 (nSegment == BOSS_LOCKED_DOOR_SEG) &&
			 (nSide == BOSS_LOCKED_DOOR_SIDE);
return 0;
}

//------------------------------------------------------------------------------

void UnlockAllWalls (int bOnlyDoors)
{
	int	i;
	CWall	*wallP;

for (i = gameData.walls.nWalls, wallP = WALLS.Buffer (); i; wallP++, i--) {
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
	int			h, i;
	CWall			*wallP;
	CSegment		*segP;
	tWallClip	*animP;

for (i = 0, wallP = WALLS.Buffer (); i < gameData.walls.nWalls; wallP++, i++) {
	if (wallP->nType == WALL_DOOR) {
		animP = gameData.walls.animP + wallP->nClip;
		if (!(animP->flags & WCF_ALTFMT))
			continue;
		h = (animP->flags & WCF_TMAP1) ? -1 : 1;
		segP = SEGMENTS + wallP->nSegment;
		segP->m_sides [wallP->nSide].m_nFrame = h;
		}
	}
}

//------------------------------------------------------------------------------
// Determines what happens when a CWall is shot
//returns info about CWall.  see wall[HA] for codes
//obj is the CObject that hit...either a weapon or the CPlayerData himself
//nPlayer is the number the CPlayerData who hit the CWall or fired the weapon,
//or -1 if a robot fired the weapon

int CWall::ProcessHit (int nPlayer, CObject* objP)
{
	bool bShowMessage;

if (objP->info.nType == OBJ_PLAYER)
	bShowMessage = (CFixVector::Dot (objP->info.position.mOrient.FVec (), objP->mType.physInfo.velocity) > 0);
else if (objP->info.nType == OBJ_ROBOT)
	bShowMessage = false;
else if ((objP->info.nType == OBJ_WEAPON) && (objP->cType.laserInfo.parent.nType == OBJ_ROBOT))
	bShowMessage = false;
else
	bShowMessage = true;

if (keys == KEY_BLUE) {
	if (!(gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_BLUE_KEY)) {
		if (bShowMessage && (nPlayer == gameData.multiplayer.nLocalPlayer))
			HUDInitMessage ("%s %s", TXT_BLUE, TXT_ACCESS_DENIED);
		return WHP_NO_KEY;
		}
	}
else if (keys == KEY_RED) {
	if (!(gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_RED_KEY)) {
		if (bShowMessage && (nPlayer == gameData.multiplayer.nLocalPlayer))
			HUDInitMessage("%s %s", TXT_RED, TXT_ACCESS_DENIED);
		return WHP_NO_KEY;
		}
	}
else if (keys == KEY_GOLD) {
	if (!(gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_GOLD_KEY)) {
		if (bShowMessage && (nPlayer == gameData.multiplayer.nLocalPlayer))
			HUDInitMessage("%s %s", TXT_YELLOW, TXT_ACCESS_DENIED);
		return WHP_NO_KEY;
		}
	}
if (nType == WALL_DOOR) {
	if ((flags & WALL_DOOR_LOCKED) && !(AllowToOpenSpecialBossDoor (nSegment, nSide))) {
		if (bShowMessage && (nPlayer == gameData.multiplayer.nLocalPlayer))
			HUDInitMessage (TXT_CANT_OPEN_DOOR);
		return WHP_NO_KEY;
		}
	else {
		if (state != WALL_DOOR_OPENING) {
			SEGMENTS [nSegment].OpenDoor (nSide);
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
for (int i = 0; i < gameData.walls.nWalls; i++)
	WALLS [i].Init ();
}

//------------------------------------------------------------------------------

bool DoCloakingWallFrame (int nCloakingWall)
{
	CCloakingWall*	cloakWallP;
	CWall*			frontWallP,*backWallP;
	bool				bDeleted = false;

if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return false;
cloakWallP = gameData.walls.cloaking + nCloakingWall;
frontWallP = WALLS + cloakWallP->nFrontWall;
backWallP = IS_WALL (cloakWallP->nBackWall) ? WALLS + cloakWallP->nBackWall : NULL;
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
	gameData.walls.cloaking.Delete (nCloakingWall);
	}
else if (SHOW_DYN_LIGHT || (cloakWallP->time > CLOAKING_WALL_TIME / 2)) {
	int oldType = frontWallP->nType;
	if (SHOW_DYN_LIGHT)
		frontWallP->cloakValue = (FADE_LEVELS * (CLOAKING_WALL_TIME - cloakWallP->time)) / CLOAKING_WALL_TIME;
	else
		frontWallP->cloakValue = ((cloakWallP->time - CLOAKING_WALL_TIME / 2) * (FADE_LEVELS - 2)) / (CLOAKING_WALL_TIME / 2);
	if (backWallP)
		backWallP->cloakValue = frontWallP->cloakValue;
	if (oldType != WALL_CLOAKED) {		//just switched
		int i;
		frontWallP->nType = WALL_CLOAKED;
		if (backWallP)
			backWallP->nType = WALL_CLOAKED;
		for (i = 0; i < 4; i++) {
			SEGMENTS [frontWallP->nSegment].m_sides [frontWallP->nSide].m_uvls [i].l = cloakWallP->front_ls [i];
			if (backWallP)
				SEGMENTS [backWallP->nSegment].m_sides [backWallP->nSide].m_uvls [i].l = cloakWallP->back_ls [i];
			}
		}
	}
else {		//fading out
	fix xLightScale = FixDiv (CLOAKING_WALL_TIME / 2 - cloakWallP->time, CLOAKING_WALL_TIME / 2);
	for (int i = 0; i < 4; i++) {
		SEGMENTS [frontWallP->nSegment].m_sides [frontWallP->nSide].m_uvls [i].l = FixMul (cloakWallP->front_ls [i], xLightScale);
		if (backWallP)
			SEGMENTS [backWallP->nSegment].m_sides [backWallP->nSide].m_uvls [i].l = FixMul (cloakWallP->back_ls [i], xLightScale);
		}
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		tUVL *uvlP = SEGMENTS [frontWallP->nSegment].m_sides [frontWallP->nSide].m_uvls;
		NDRecordCloakingWall (cloakWallP->nFrontWall, cloakWallP->nBackWall, frontWallP->nType, frontWallP->state, frontWallP->cloakValue,
									 uvlP [0].l, uvlP [1].l, uvlP [2].l, uvlP [3].l);
		}
	}
return bDeleted;
}

//------------------------------------------------------------------------------

void DoDecloakingWallFrame (int nCloakingWall)
{
	CCloakingWall	*cloakWallP;
	CWall				*frontWallP,*backWallP;

if (gameData.demo.nState == ND_STATE_PLAYBACK)
return;

cloakWallP = gameData.walls.cloaking + nCloakingWall;
frontWallP = WALLS + cloakWallP->nFrontWall;
backWallP = IS_WALL (cloakWallP->nBackWall) ? WALLS + cloakWallP->nBackWall : NULL;

cloakWallP->time += gameData.time.xFrame;
if (cloakWallP->time > CLOAKING_WALL_TIME) {
	uint i;
	frontWallP->state = WALL_DOOR_CLOSED;
	if (backWallP)
	backWallP->state = WALL_DOOR_CLOSED;
	for (i = 0; i < 4; i++) {
		SEGMENTS [frontWallP->nSegment].m_sides [frontWallP->nSide].m_uvls [i].l = cloakWallP->front_ls [i];
		if (backWallP)
			SEGMENTS [backWallP->nSegment].m_sides [backWallP->nSide].m_uvls [i].l = cloakWallP->back_ls [i];
		}
	gameData.walls.cloaking.Delete (nCloakingWall);
	}
else if (cloakWallP->time > CLOAKING_WALL_TIME/2) {		//fading in
	frontWallP->nType = WALL_CLOSED;
	if (SHOW_DYN_LIGHT)
		lightManager.Toggle (frontWallP->nSegment, frontWallP->nSide, -1, 0);
	if (backWallP) {
		backWallP->nType = WALL_CLOSED;
		if (SHOW_DYN_LIGHT)
			lightManager.Toggle (backWallP->nSegment, backWallP->nSide, -1, 0);
		}
	fix xLightScale = FixDiv(cloakWallP->time-CLOAKING_WALL_TIME/2,CLOAKING_WALL_TIME/2);
	for (int i = 0; i < 4; i++) {
		SEGMENTS [frontWallP->nSegment].m_sides [frontWallP->nSide].m_uvls [i].l = FixMul(cloakWallP->front_ls [i],xLightScale);
		if (backWallP)
			SEGMENTS [backWallP->nSegment].m_sides [backWallP->nSide].m_uvls [i].l = FixMul(cloakWallP->back_ls [i],xLightScale);
		}
	}
else {		//cloaking in
	frontWallP->cloakValue = ((CLOAKING_WALL_TIME/2 - cloakWallP->time) * (FADE_LEVELS-2)) / (CLOAKING_WALL_TIME/2);
	frontWallP->nType = WALL_CLOAKED;
	if (backWallP) {
		backWallP->cloakValue = frontWallP->cloakValue;
		backWallP->nType = WALL_CLOAKED;
		}
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	tUVL *uvlP = SEGMENTS [frontWallP->nSegment].m_sides [frontWallP->nSide].m_uvls;
	NDRecordCloakingWall (cloakWallP->nFrontWall, cloakWallP->nBackWall, frontWallP->nType, frontWallP->state, frontWallP->cloakValue,
								 uvlP [0].l, uvlP [1].l, uvlP [2].l, uvlP [3].l);
	}
}

//------------------------------------------------------------------------------

void WallFrameProcess (void)
{
	CCloakingWall*	cloakWallP;
	CActiveDoor*	doorP = gameData.walls.activeDoors.Buffer ();
	CWall*			wallP, *backWallP;
	uint			i;

for (i = 0; i < gameData.walls.activeDoors.ToS (); i++, doorP++) {
	backWallP = NULL,
	wallP = WALLS + doorP->nFrontWall [0];
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
			WALLS [doorP->nBackWall [0]].state = WALL_DOOR_CLOSING;
		if ((doorP->time > DOOR_WAIT_TIME) && !SEGMENTS [wallP->nSegment].DoorIsBlocked (wallP->nSide)) {
			wallP->state = WALL_DOOR_CLOSING;
			doorP->time = 0;
			}
		else {
			wallP->flags |= WALL_DOOR_OPENED;
			if (backWallP)
				backWallP->flags |= WALL_DOOR_OPENED;
			}
		}
	else if (wallP->state == WALL_DOOR_CLOSED || wallP->state == WALL_DOOR_OPEN) {
		//this shouldn't happen.  if the CWall is in one of these states,
		//there shouldn't be an activedoor entry for it.  So we'll kill
		//the activedoor entry.  Tres simple.
		gameData.walls.activeDoors.Delete (i);
		i--;
		}
	}

cloakWallP = gameData.walls.cloaking.Buffer ();
for (i = 0; i < gameData.walls.cloaking.ToS (); i++, cloakWallP++) {
	ubyte s = WALLS [cloakWallP->nFrontWall].state;
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

int				nStuckObjects = 0;
tStuckObject	stuckObjects [MAX_STUCK_OBJECTS];

//------------------------------------------------------------------------------
//	An CObject got stuck in a door (like a flare).
//	Add global entry.
void AddStuckObject (CObject *objP, short nSegment, short nSide)
{
	int				i;
	short				nWall;
	tStuckObject	*stuckObjP;

CWall* wallP = SEGMENTS [nSegment].Wall (nSide);
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
	short	nObject, nWall;
	CObject			*objP;
	tStuckObject	*stuckObjP;

	//	Safety and efficiency code.  If no stuck OBJECTS, should never get inside the IF, but this is faster.
if (!nStuckObjects)
	return;
nObject = gameData.app.nFrameCount % MAX_STUCK_OBJECTS;
stuckObjP = stuckObjects + nObject;
objP = OBJECTS + stuckObjP->nObject;
nWall = stuckObjP->nWall;
if (IS_WALL (nWall) &&
	 ((WALLS [nWall].state != WALL_DOOR_CLOSED) ||
	  (objP->info.nSignature != stuckObjP->nSignature))) {
	nStuckObjects--;
	objP->info.xLifeLeft = I2X (1)/8;
	stuckObjP->nWall = NO_WALL;
	}
}

//	----------------------------------------------------------------------------------------------------
//	Door with CWall index nWall is opening, kill all OBJECTS stuck in it.
void KillStuckObjects (int nWall)
{
	int				i;
	tStuckObject	*stuckObjP;
	CObject			*objP;

if (!IS_WALL (nWall) || (nStuckObjects == 0))
	return;
nStuckObjects = 0;

for (i = 0, stuckObjP = stuckObjects; i < MAX_STUCK_OBJECTS; i++, stuckObjP++)
	if (stuckObjP->nWall == nWall) {
		objP = OBJECTS + stuckObjP->nObject;
		if (objP->info.nType == OBJ_WEAPON)
			objP->info.xLifeLeft = I2X (1)/8;
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
FlushFCDCache();
}

// -----------------------------------------------------------------------------------
// Initialize stuck OBJECTS array.  Called at start of level
void InitStuckObjects (void)
{
for (int i = 0; i < MAX_STUCK_OBJECTS; i++)
	stuckObjects [i].nWall = NO_WALL;
nStuckObjects = 0;
}

// -----------------------------------------------------------------------------------
// Clear out all stuck OBJECTS.  Called for a new ship
void ClearStuckObjects (void)
{
	tStuckObject	*stuckObjP = stuckObjects;
	CObject			*objP;

for (int i = 0; i < MAX_STUCK_OBJECTS; i++, stuckObjP++) {
	if (IS_WALL (stuckObjP->nWall)) {
		objP = OBJECTS + stuckObjP->nObject;
		if ((objP->info.nType == OBJ_WEAPON) && (objP->info.nId == FLARE_ID))
			objP->info.xLifeLeft = I2X (1)/8;
		stuckObjP->nWall = NO_WALL;
		nStuckObjects--;
		}
	}
if (nStuckObjects)
	nStuckObjects = 0;
}

// -----------------------------------------------------------------------------------
#define	MAX_BLAST_GLASS_DEPTH	5

void BngProcessSegment (CObject *objP, fix damage, CSegment *segP, int depth, sbyte *visited)
{
	int	i;
	short	nSide;

if (depth > MAX_BLAST_GLASS_DEPTH)
	return;

depth++;

for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	int			tm;
	fix			dist;
	CFixVector	pnt;

	//	Process only walls which have glass.
	if ((tm = segP->m_sides [nSide].m_nOvlTex)) {
		int				ec, db;
		tEffectClip*	ecP;

		ec=gameData.pig.tex.tMapInfoP [tm].nEffectClip;
		ecP = (ec < 0) ? NULL : gameData.eff.effectP + ec;
		db = ecP ? ecP->nDestBm : -1;

		if (((ec != -1) && (db != -1) && !(ecP->flags & EF_ONE_SHOT)) ||
		 	 ((ec == -1) && (gameData.pig.tex.tMapInfoP [tm].destroyed != -1))) {
			pnt = segP->SideCenter (nSide);
			dist = CFixVector::Dist(pnt, objP->info.position.vPos);
			if (dist < damage/2) {
				dist = FindConnectedDistance (pnt, segP->Index (), objP->info.position.vPos, objP->info.nSegment, MAX_BLAST_GLASS_DEPTH, WID_RENDPAST_FLAG, 0);
				if ((dist > 0) && (dist < damage / 2) &&
					 segP->CheckEffectBlowup (nSide, pnt, (objP->cType.laserInfo.parent.nObject < 0) ? NULL : OBJECTS + objP->cType.laserInfo.parent.nObject, 1))
						segP->OperateTrigger (nSide, OBJECTS + objP->cType.laserInfo.parent.nObject, 1);
				}
			}
		}
	}

for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
	short nSegment = segP->m_children [i];

	if ((nSegment != -1) && !visited [nSegment] && (segP->IsDoorWay (i, NULL) & WID_FLY_FLAG)) {
		visited [nSegment] = 1;
		BngProcessSegment (objP, damage, &SEGMENTS [nSegment], depth, visited);
		}
	}
}

// -----------------------------------------------------------------------------------

bool CWall::IsTriggerTarget (void)
{
CTrigger *triggerP = TRIGGERS.Buffer ();
for (int i = gameData.trigs.m_nTriggers; i; i--, triggerP++) {
	short *nSegP = triggerP->segments;
	short *nSideP = triggerP->sides;
	for (int j = triggerP->nLinks; j; j--, nSegP++, nSideP++)
		if ((*nSegP == nSegment) && (*nSideP == nSide))
			return true;
	}
return false;
}

// -----------------------------------------------------------------------------------

bool CWall::IsVolatile (void)
{
if ((nType == WALL_DOOR) || (nType == WALL_BLASTABLE))
	return true;
return CWall::IsTriggerTarget ();
}

// -----------------------------------------------------------------------------------

bool CWall::IsInvisible (void)
{
if ((nType != WALL_OPEN) && ((nType != WALL_CLOAKED) || (cloakValue <  FADE_LEVELS)))
	return false;
return !IsTriggerTarget ();
}

// -----------------------------------------------------------------------------------
//	objP is going to detonate
//	blast nearby monitors, lights, maybe other things
void BlastNearbyGlass (CObject *objP, fix damage)
{
	int		i;
	sbyte   visited [MAX_SEGMENTS_D2X];
	CSegment	*cursegp;

cursegp = &SEGMENTS [objP->info.nSegment];
for (i=0; i<=gameData.segs.nLastSegment; i++)
memset (visited, 0, sizeof (visited));
visited [objP->info.nSegment] = 1;
BngProcessSegment(objP, damage, cursegp, 0, visited);
}

// -----------------------------------------------------------------------------------

#define MAX_CLIP_FRAMES_D1 20

/*
 * reads a tWallClip structure from a CFile
 */
int ReadD1WallClips(tWallClip *wc, int n, CFile& cf)
{
	int i, j;

	for (i = 0; i < n; i++) {
		wc [i].xTotalTime = cf.ReadFix ();
		wc [i].nFrameCount = cf.ReadShort ();
		for (j = 0; j < MAX_CLIP_FRAMES_D1; j++)
			wc [i].frames [j] = cf.ReadShort ();
		wc [i].openSound = cf.ReadShort ();
		wc [i].closeSound = cf.ReadShort ();
		wc [i].flags = cf.ReadShort ();
		cf.Read (wc [i].filename, 13, 1);
		wc [i].pad = cf.ReadByte ();
	}
	return i;
}

// -----------------------------------------------------------------------------------

/*
 * reads a tWallClip structure from a CFile
 */
int ReadWallClips(CArray<tWallClip>& wc, int n, CFile& cf)
{
	int i, j;

for (i = 0; i < n; i++) {
	wc [i].xTotalTime = cf.ReadFix ();
	wc [i].nFrameCount = cf.ReadShort ();
	for (j = 0; j < MAX_CLIP_FRAMES; j++)
		wc [i].frames [j] = cf.ReadShort ();
	wc [i].openSound = cf.ReadShort ();
	wc [i].closeSound = cf.ReadShort ();
	wc [i].flags = cf.ReadShort ();
	cf.Read(wc [i].filename, 13, 1);
	wc [i].pad = cf.ReadByte ();
	}
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
w.nTrigger = (ubyte) cf.ReadByte ();
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
w.nTrigger = (ubyte) cf.ReadByte ();
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
flags = cf.ReadByte ();
state = cf.ReadByte ();
nTrigger = (ubyte) cf.ReadByte ();
nClip = cf.ReadByte ();
keys = cf.ReadByte ();
controllingTrigger = cf.ReadByte ();
cloakValue = cf.ReadByte ();
}

//------------------------------------------------------------------------------

void CWall::SaveState (CFile& cf)
{
cf.WriteInt (nSegment);
cf.WriteInt (nSide);
cf.WriteFix (hps);    
cf.WriteInt (nLinkedWall);
cf.WriteByte ((sbyte) nType);       
cf.WriteByte ((sbyte) flags);      
cf.WriteByte ((sbyte) state);      
cf.WriteByte ((sbyte) nTrigger);    
cf.WriteByte (nClip);   
cf.WriteByte ((sbyte) keys);       
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
nType = (ubyte) cf.ReadByte ();       
flags = (ubyte) cf.ReadByte ();      
state = (ubyte) cf.ReadByte ();      
nTrigger = (ubyte) cf.ReadByte ();    
nClip = cf.ReadByte ();   
keys = (ubyte) cf.ReadByte ();       
controllingTrigger = cf.ReadByte ();
cloakValue = cf.ReadByte (); 
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
return ((nTrigger == NO_TRIGGER) || (nTrigger >= gameData.trigs.m_nTriggers)) ? NULL : &TRIGGERS [nTrigger];
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CActiveDoor::LoadState (CFile& cf)
{
nPartCount = cf.ReadInt ();
for (int i = 0; i < 2; i++) {
	nFrontWall [i] = cf.ReadShort ();
	nBackWall [i] = cf.ReadShort ();
	}
time = cf.ReadFix ();    
}

//------------------------------------------------------------------------------

void CActiveDoor::SaveState (CFile& cf)
{
cf.WriteInt (nPartCount);
for (int i = 0; i < 2; i++) {
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
for (int i = 0; i < 4; i++) {
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
for (int i = 0; i < 4; i++) {
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
gameData.walls.exploding.Reset ();
}

//------------------------------------------------------------------------------
//explode the given CWall
void ExplodeWall (short nSegment, short nSide)
{
if (gameData.walls.exploding.Grow ()) {
	gameData.walls.exploding.Top ()->nSegment = nSegment;
	gameData.walls.exploding.Top ()->nSide = nSide;
	gameData.walls.exploding.Top ()->time = 0;
	SEGMENTS [nSegment].CreateSound (SOUND_EXPLODING_WALL, nSide);
	}
}

//------------------------------------------------------------------------------
//handle walls for this frame
//note: this CWall code assumes the CWall is not triangulated
void DoExplodingWallFrame (void)
{
for (uint i = 0; i < gameData.walls.exploding.ToS (); ) {
	short nSegment = gameData.walls.exploding [i].nSegment;
	if (nSegment < 0) {
		gameData.walls.exploding.Delete (i);
		continue;
		}
	short nSide = gameData.walls.exploding [i].nSide;
	fix oldfrac, newfrac;
	int oldCount, newCount, e;		//n,
	oldfrac = FixDiv (gameData.walls.exploding [i].time, EXPL_WALL_TIME);
	gameData.walls.exploding [i].time += gameData.time.xFrame;
	if (gameData.walls.exploding [i].time > EXPL_WALL_TIME)
		gameData.walls.exploding [i].time = EXPL_WALL_TIME;
	else if (gameData.walls.exploding [i].time > 3 * EXPL_WALL_TIME / 4) {
		CSegment *segP = SEGMENTS + nSegment,
					*connSegP = SEGMENTS + segP->m_children [nSide];
		ubyte	a = (ubyte) segP->Wall (nSide)->nClip;
		short n = AnimFrameCount (gameData.walls.animP + a);
		short nConnSide = segP->ConnectedSide (connSegP);
		segP->SetTexture (nSide, connSegP, nConnSide, a, n - 1);
		segP->Wall (nSide)->flags |= WALL_BLASTED;
		if (nConnSide >= 0)
			connSegP->Wall (nConnSide)->flags |= WALL_BLASTED;
		}
	newfrac = FixDiv (gameData.walls.exploding [i].time, EXPL_WALL_TIME);
	oldCount = X2I (EXPL_WALL_TOTAL_FIREBALLS * FixMul (oldfrac, oldfrac));
	newCount = X2I (EXPL_WALL_TOTAL_FIREBALLS * FixMul (newfrac, newfrac));
	//n = newCount - oldCount;
	//now create all the next explosions
	for (e = oldCount; e < newCount; e++) {
		short*		corners;
		CFixVector	*v0, *v1, *v2;
		CFixVector	vv0, vv1, vPos;
		fix			size;

		//calc expl position

		corners = SEGMENTS [nSegment].Corners (nSide);
		v0 = gameData.segs.vertices + corners [0];
		v1 = gameData.segs.vertices + corners [1];
		v2 = gameData.segs.vertices + corners [2];
		vv0 = *v0 - *v1;
		vv1 = *v2 - *v1;
		vPos = *v1 + vv0 * (d_rand () * 2);
		vPos += vv1 * (d_rand ()*2);
		size = EXPL_WALL_FIREBALL_SIZE + (2*EXPL_WALL_FIREBALL_SIZE * e / EXPL_WALL_TOTAL_FIREBALLS);
		//fireballs start away from door, with subsequent ones getting closer
		vPos += SEGMENTS [nSegment].m_sides [nSide].m_normals [0] * (size * (EXPL_WALL_TOTAL_FIREBALLS - e) / EXPL_WALL_TOTAL_FIREBALLS);
		if (e & 3)		//3 of 4 are Normal
			/*Object*/CreateExplosion ((short) gameData.walls.exploding [i].nSegment, vPos, size, (ubyte) VCLIP_SMALL_EXPLOSION);
		else
			CreateBadassExplosion (NULL, (short) gameData.walls.exploding [i].nSegment, vPos,
										  size, (ubyte) VCLIP_SMALL_EXPLOSION, I2X (4), I2X (20), I2X (50), -1);
		}
	if (gameData.walls.exploding [i].time >= EXPL_WALL_TIME)
		gameData.walls.exploding.Delete (i);
	else
		i++;
	}
}

// -----------------------------------------------------------------------------------
