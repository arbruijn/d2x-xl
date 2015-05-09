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

CBitmap *pBm = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [anim->frames [0]].index;
if (pBm->Override ())
	pBm = pBm->Override ();
n = (pBm->Type () == BM_TYPE_ALT) ? pBm->FrameCount () : anim->nFrameCount;
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
	CActiveDoor* pDoor = gameData.wallData.activeDoors.Buffer ();

for (uint32_t i = gameData.wallData.activeDoors.ToS (); i; i--, pDoor++) {		//find door
	for (int32_t j = 0; j < pDoor->nPartCount; j++)
		if ((pDoor->nFrontWall [j] == nWall) || (pDoor->nBackWall [j] == nWall))
			return pDoor;
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

int32_t CWall::IsPassable (CObject *pObj, bool bIgnoreDoors)
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
	if (pObj && (pObj->info.nType == OBJ_PLAYER)) {
		if (IsTeamGame && ((keys >> 1) == GetTeam (pObj->info.nId) + 1))
			return WID_ILLUSORY_WALL;
		if ((keys == KEY_BLUE) && (PLAYER (pObj->info.nId).flags & PLAYER_FLAGS_BLUE_KEY))
			return WID_ILLUSORY_WALL;
		if ((keys == KEY_RED) && (PLAYER (pObj->info.nId).flags & PLAYER_FLAGS_RED_KEY))
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
	CActiveDoor* pDoor = NULL;

if ((state == WALL_DOOR_OPENING) ||	//already opening
	 (state == WALL_DOOR_WAITING) ||	//open, waiting to close
	 (state == WALL_DOOR_OPEN))		//open, & staying open
	return NULL;

if (state == WALL_DOOR_CLOSING) {		//closing, so reuse door
	if ((pDoor = FindActiveDoor (this - WALLS))) {
		pDoor->time = (fix) (gameData.wallData.pAnim [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed) - pDoor->time;
		if (pDoor->time < 0)
			pDoor->time = 0;
		}
	}
else if (state != WALL_DOOR_CLOSED)
	return NULL;
if (!pDoor) {
	if (!(pDoor = AddActiveDoor ()))
		return NULL;
	pDoor->time = 0;
	}
state = WALL_DOOR_OPENING;
audio.Update ();
return pDoor;
}

//------------------------------------------------------------------------------

CCloakingWall* FindCloakingWall (int16_t nWall)
{
	CCloakingWall* pCloakWall = gameData.wallData.cloaking.Buffer ();

for (uint32_t i = gameData.wallData.cloaking.ToS (); i; i--, pCloakWall++) {		//find door
	if ((pCloakWall->nFrontWall == nWall) || (pCloakWall->nBackWall == nWall))
		return pCloakWall;
	}
return NULL;
}

//------------------------------------------------------------------------------
// This function closes the specified door and repStuckObjres the closed
//  door texture.  This is called when the animation is done
void CloseDoor (int32_t nDoor)
{
	CActiveDoor*	pDoor = gameData.wallData.activeDoors + nDoor;

for (int32_t i = pDoor->nPartCount; i; )
	WALL (pDoor->nFrontWall [--i])->CloseActiveDoor ();
DeleteActiveDoor (nDoor);
}

//------------------------------------------------------------------------------

CCloakingWall* CWall::StartCloak (void)
{
	CCloakingWall* pCloakWall = NULL;

if (state == WALL_DOOR_DECLOAKING) {	//decloaking, so reuse door
	if (!(pCloakWall = FindCloakingWall (this - WALLS)))
		return NULL;
	pCloakWall->time = (fix) (CLOAKING_WALL_TIME * gameStates.gameplay.slowmo [0].fSpeed) - pCloakWall->time;
	}
else if (state == WALL_DOOR_CLOSED) {	//create new door
	if (!(pCloakWall = AddCloakingWall ()))
		return NULL;
	pCloakWall->time = 0;
	}
else
	return NULL;
state = WALL_DOOR_CLOAKING;
return pCloakWall;
}

//------------------------------------------------------------------------------

CCloakingWall* CWall::StartDecloak (void)
{
	CCloakingWall*	pCloakWall = NULL;

if (state == WALL_DOOR_CLOAKING) {	//cloaking, so reuse door
	if (!(pCloakWall = FindCloakingWall (this - WALLS)))
		return NULL;
	pCloakWall->time = (fix) (CLOAKING_WALL_TIME * gameStates.gameplay.slowmo [0].fSpeed) - pCloakWall->time;
	}
else if (state == WALL_DOOR_CLOSED) {	//create new door
	if (!(pCloakWall = AddCloakingWall ()))
		return NULL;
	pCloakWall->time = 0;
	}
else
	return NULL;
state = WALL_DOOR_DECLOAKING;
return pCloakWall;
}

//------------------------------------------------------------------------------

void CWall::CloseActiveDoor (void)
{
CSegment* pSeg = SEGMENT (nSegment);
CSegment* pConnSeg = SEGMENT (pSeg->m_children [nSide]);
int16_t nConnSide = pSeg->ConnectedSide (pConnSeg);

state = WALL_DOOR_CLOSED;
pSeg->SetTexture (nSide, NULL, -1, nClip, 0);

CWall* pWall = pConnSeg->Wall (nConnSide);
if (pWall) {
	pWall->state = WALL_DOOR_CLOSED;
	pConnSeg->SetTexture (nConnSide, NULL, -1, pWall->nClip, 0);
	}
}

//------------------------------------------------------------------------------

CActiveDoor* CWall::CloseDoor (bool bForce)
{
	CActiveDoor* pDoor = NULL;

if ((state == WALL_DOOR_OPENING) ||
	 ((state == WALL_DOOR_WAITING) && bForce && gameStates.app.bD2XLevel)) {	//reuse door
	if ((pDoor = FindActiveDoor (this - WALLS))) {
		pDoor->time = (fix) (gameData.wallData.pAnim [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed) - pDoor->time;
		if (pDoor->time < 0)
			pDoor->time = 0;
		}
	}
else if (state != WALL_DOOR_OPEN)
	return NULL;
if (!pDoor) {
	if (!(pDoor = AddActiveDoor ()))
		return NULL;
	pDoor->time = 0;
	}
state = WALL_DOOR_CLOSING;
return pDoor;
}

//------------------------------------------------------------------------------

int32_t CWall::AnimateOpeningDoor (fix xElapsedTime)
{
if (nClip < 0)
	return 3;

int32_t nFrames = WallEffectFrameCount (gameData.wallData.pAnim + nClip);
if (!nFrames)
	return 3;

fix xTotalTime = (fix) (gameData.wallData.pAnim [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed);
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
CSegment* pSeg = SEGMENT (nSegment);
if (i < nFrames)
	pSeg->SetTexture (nSide, NULL, -1, nClip, i);
if (i > nFrames / 2)
	flags |= WALL_DOOR_OPENED;
if (i < nFrames - 1)
	return 0; // not fully opened yet
pSeg->SetTexture (nSide, NULL, -1, nClip, nFrames - 1);
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
int32_t nFrames = WallEffectFrameCount (gameData.wallData.pAnim + nClip);
if (!nFrames)
	return 0;

fix xTotalTime = (fix) (gameData.wallData.pAnim [nClip].xTotalTime * gameStates.gameplay.slowmo [0].fSpeed);
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
	CActiveDoor*	pDoor;
	CWall*			pWall;
	int16_t				nConnSide, nSide;
	CSegment*		pConnSeg, * pSeg;
	int32_t				i, nWall, bFlags = 3;

if (nDoor < -1)
	return false;
pDoor = gameData.wallData.activeDoors + nDoor;
for (i = 0; i < pDoor->nPartCount; i++) {
	pDoor->time += gameData.time.xFrame;
	nWall = pDoor->nFrontWall [i];
	if (!IS_WALL (nWall)) {
		PrintLog (0, "Trying to open non existent door\n");
		continue;
		}
	pWall = WALL (pDoor->nFrontWall [i]);
	KillStuckObjects (pDoor->nFrontWall [i]);
	KillStuckObjects (pDoor->nBackWall [i]);

	pSeg = SEGMENT (pWall->nSegment);
	nSide = pWall->nSide;
	if (!pSeg->IsWall (nSide)) {
		PrintLog (0, "Trying to open non existent door @ %d,%d\n", pWall->nSegment, nSide);
		continue;
		}
	bFlags &= pSeg->AnimateOpeningDoor (nSide, pDoor->time);
	pConnSeg = SEGMENT (pSeg->m_children [nSide]);
	nConnSide = pSeg->ConnectedSide (pConnSeg);
	if (nConnSide < 0) {
		PrintLog (0, "Door %d @ %d,%d has no oppposite door in %d\n", pSeg->WallNum (nSide), pWall->nSegment, nSide, pSeg->m_children [nSide]);
		continue;
		}
	if (pConnSeg->IsWall (nConnSide))
		bFlags &= pConnSeg->AnimateOpeningDoor (nConnSide, pDoor->time);
	}
if (bFlags & 1) {
	DeleteActiveDoor (nDoor);
	return true;
	}
if (bFlags & 2)
	pDoor->time = 0;	//counts up
return false;
}

//------------------------------------------------------------------------------
// Animates and processes the closing of a door.
// Called from the game loop.
bool DoDoorClose (int32_t nDoor)
{
if (nDoor < -1)
	return false;
CActiveDoor* pDoor = gameData.wallData.activeDoors + nDoor;
CWall* pWall = WALL (pDoor->nFrontWall [0]);
CSegment* pSeg = SEGMENT (pWall->nSegment);

	int32_t			i, bFlags = 1;
	int16_t			nConnSide, nSide;
	CSegment*	pConnSeg;

//check for OBJECTS in doorway before closing
if (pWall->flags & WALL_DOOR_AUTO) {
	if (pSeg->DoorIsBlocked (int16_t (pWall->nSide), pWall->IgnoreMarker ())) {
		audio.DestroySegmentSound (int16_t (pWall->nSegment), int16_t (pWall->nSide), -1);
		pSeg->OpenDoor (int16_t (pWall->nSide));		//re-open door
		return false;
		}
	}

for (i = 0; i < pDoor->nPartCount; i++) {
	pWall = WALL (pDoor->nFrontWall [i]);
	pSeg = SEGMENT (pWall->nSegment);
	nSide = pWall->nSide;
	if (!pSeg->IsWall (nSide)) {
#if DBG
		PrintLog (0, "Trying to close non existent door\n");
#endif
		continue;
		}
	//if here, must be auto door
	//don't assert here, because now we have triggers to close non-auto doors
	// Otherwise, close it.
	pConnSeg = SEGMENT (pSeg->m_children [nSide]);
	nConnSide = pSeg->ConnectedSide (pConnSeg);
	if (nConnSide < 0) {
		PrintLog (0, "Door %d @ %d,%d has no oppposite door in %d\n", 
					 pSeg->WallNum (nSide), pWall->nSegment, nSide, pSeg->m_children [nSide]);
		continue;
		}
	if ((gameData.demo.nState != ND_STATE_PLAYBACK) && !(i || pDoor->time)) {
		if (gameData.wallData.pAnim [pWall->nClip].closeSound  > -1)
			audio.CreateSegmentSound ((int16_t) gameData.wallData.pAnim [pSeg->Wall (nSide)->nClip].closeSound,
											  pWall->nSegment, nSide, SEGMENT (pWall->nSegment)->SideCenter (nSide), 0, I2X (1));
		}
	pDoor->time += gameData.time.xFrame;
	bFlags &= pSeg->AnimateClosingDoor (nSide, pDoor->time);
	if (pConnSeg->IsWall (nConnSide))
		bFlags &= pConnSeg->AnimateClosingDoor (nConnSide, pDoor->time);
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
	CWall	*pWall;

for (i = gameData.wallData.nWalls, pWall = WALLS.Buffer (); i; pWall++, i--) {
	if (pWall->nType == WALL_DOOR) {
		pWall->flags &= ~WALL_DOOR_LOCKED;
		pWall->keys = 0;
		}
	else if (!bOnlyDoors
#if !DBG
				&& (pWall->nType == WALL_CLOSED)
#endif
			 )
		pWall->nType = WALL_OPEN;
	}
}

//------------------------------------------------------------------------------
// set up renderer for alternative highres animation method
// (all frames are pStuckObjred in one texture, and struct nSide.nFrame
// holds the frame index).

void InitDoorAnims (void)
{
	int32_t		h, i;
	CWall			*pWall;
	CSegment		*pSeg;
	tWallEffect	*pAnim;

for (i = 0, pWall = WALLS.Buffer (); i < gameData.wallData.nWalls; pWall++, i++) {
	if (pWall->nType == WALL_DOOR) {
		pAnim = gameData.wallData.pAnim + pWall->nClip;
		if (!(pAnim->flags & WCF_ALTFMT))
			continue;
		h = (pAnim->flags & WCF_TMAP1) ? -1 : 1;
		pSeg = SEGMENT (pWall->nSegment);
		pSeg->m_sides [pWall->nSide].m_nFrame = h;
		}
	}
}

//------------------------------------------------------------------------------
// Determines what happens when a CWall is shot
//returns info about CWall.  see wall[HA] for codes
//obj is the CObject that hit...either a weapon or the player himself
//nPlayer is the number the player who hit the CWall or fired the weapon,
//or -1 if a robot fired the weapon

int32_t CWall::ProcessHit (int32_t nPlayer, CObject* pObj)
{
	bool bShowMessage;

if (pObj->info.nType == OBJ_PLAYER)
	bShowMessage = (CFixVector::Dot (pObj->info.position.mOrient.m.dir.f, pObj->mType.physInfo.velocity) > 0);
else if (pObj->info.nType == OBJ_ROBOT)
	bShowMessage = false;
else if ((pObj->info.nType == OBJ_WEAPON) && (pObj->cType.laserInfo.parent.nType == OBJ_ROBOT))
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
	CCloakingWall*	pCloakWall;
	CWall*			pFrontWall,*pBackWall;
	bool				bDeleted = false;

if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return false;
pCloakWall = gameData.wallData.cloaking + nCloakingWall;
pFrontWall = WALL (pCloakWall->nFrontWall);
pBackWall = IS_WALL (pCloakWall->nBackWall) ? WALL (pCloakWall->nBackWall) : NULL;
pCloakWall->time += gameData.time.xFrame;
if (pCloakWall->time > CLOAKING_WALL_TIME) {
	pFrontWall->nType = WALL_OPEN;
	if (SHOW_DYN_LIGHT)
		lightManager.Toggle (pFrontWall->nSegment, pFrontWall->nSide, -1, 0);
	pFrontWall->state = WALL_DOOR_CLOSED;		//why closed? why not?
	if (pBackWall) {
		pBackWall->nType = WALL_OPEN;
		if (SHOW_DYN_LIGHT)
			lightManager.Toggle (pBackWall->nSegment, pBackWall->nSide, -1, 0);
		pBackWall->state = WALL_DOOR_CLOSED;		//why closed? why not?
		}
	DeleteCloakingWall (nCloakingWall);
	audio.Update ();
	}
else if (SHOW_DYN_LIGHT || (pCloakWall->time > CLOAKING_WALL_TIME / 2)) {
	int32_t oldType = pFrontWall->nType;
#if 1
	pFrontWall->cloakValue = int8_t (float (FADE_LEVELS) * X2F (pCloakWall->time));
#else
	if (SHOW_DYN_LIGHT)
		pFrontWall->cloakValue = (fix) FRound (FADE_LEVELS * X2F (pCloakWall->time));
	else
		pFrontWall->cloakValue = ((pCloakWall->time - CLOAKING_WALL_TIME / 2) * (FADE_LEVELS - 2)) / (CLOAKING_WALL_TIME / 2);
#endif
	if (pBackWall)
		pBackWall->cloakValue = pFrontWall->cloakValue;
	if (oldType != WALL_CLOAKED) {		//just switched
		pFrontWall->nType = WALL_CLOAKED;
		if (pBackWall)
			pBackWall->nType = WALL_CLOAKED;
		if (!SHOW_DYN_LIGHT) {
			for (int32_t i = 0; i < 4; i++) {
				SEGMENT (pFrontWall->nSegment)->m_sides [pFrontWall->nSide].m_uvls [i].l = pCloakWall->front_ls [i];
				if (pBackWall)
					SEGMENT (pBackWall->nSegment)->m_sides [pBackWall->nSide].m_uvls [i].l = pCloakWall->back_ls [i];
				}
			}
		}
	}
else {		//fading out
	fix xLightScale = FixDiv (CLOAKING_WALL_TIME / 2 - pCloakWall->time, CLOAKING_WALL_TIME / 2);
	for (int32_t i = 0; i < 4; i++) {
		SEGMENT (pFrontWall->nSegment)->m_sides [pFrontWall->nSide].m_uvls [i].l = FixMul (pCloakWall->front_ls [i], xLightScale);
		if (pBackWall)
			SEGMENT (pBackWall->nSegment)->m_sides [pBackWall->nSide].m_uvls [i].l = FixMul (pCloakWall->back_ls [i], xLightScale);
		}
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		tUVL *uvlP = SEGMENT (pFrontWall->nSegment)->m_sides [pFrontWall->nSide].m_uvls;
		NDRecordCloakingWall (pCloakWall->nFrontWall, pCloakWall->nBackWall, pFrontWall->nType, pFrontWall->state, pFrontWall->cloakValue,
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

CCloakingWall* pCloakWall = gameData.wallData.cloaking + nCloakingWall;
CWall* pFrontWall = WALL (pCloakWall->nFrontWall);
CWall* pBackWall = IS_WALL (pCloakWall->nBackWall) ? WALL (pCloakWall->nBackWall) : NULL;

pCloakWall->time += gameData.time.xFrame;
if (pCloakWall->time > CLOAKING_WALL_TIME) {
	pFrontWall->state = WALL_DOOR_CLOSED;
	if (pBackWall)
		pBackWall->state = WALL_DOOR_CLOSED;
	for (uint32_t i = 0; i < 4; i++) {
		SEGMENT (pFrontWall->nSegment)->m_sides [pFrontWall->nSide].m_uvls [i].l = pCloakWall->front_ls [i];
		if (pBackWall)
			SEGMENT (pBackWall->nSegment)->m_sides [pBackWall->nSide].m_uvls [i].l = pCloakWall->back_ls [i];
		}
		DeleteCloakingWall (nCloakingWall);
	}
else if (pCloakWall->time > CLOAKING_WALL_TIME / 2) {		//fading in
	pFrontWall->nType = WALL_CLOSED;
	if (SHOW_DYN_LIGHT)
		lightManager.Toggle (pFrontWall->nSegment, pFrontWall->nSide, -1, 0);
	if (pBackWall) {
		pBackWall->nType = WALL_CLOSED;
		if (SHOW_DYN_LIGHT)
			lightManager.Toggle (pBackWall->nSegment, pBackWall->nSide, -1, 0);
		}
	fix xLightScale = FixDiv (pCloakWall->time-CLOAKING_WALL_TIME/2,CLOAKING_WALL_TIME / 2);
	for (int32_t i = 0; i < 4; i++) {
		SEGMENT (pFrontWall->nSegment)->m_sides [pFrontWall->nSide].m_uvls [i].l = FixMul (pCloakWall->front_ls [i], xLightScale);
		if (pBackWall)
			SEGMENT (pBackWall->nSegment)->m_sides [pBackWall->nSide].m_uvls [i].l = FixMul (pCloakWall->back_ls [i], xLightScale);
		}
	}
else {		//cloaking in
	pFrontWall->cloakValue = ((CLOAKING_WALL_TIME / 2 - pCloakWall->time) * (FADE_LEVELS - 2)) / (CLOAKING_WALL_TIME / 2);
	pFrontWall->nType = WALL_CLOAKED;
	if (pBackWall) {
		pBackWall->cloakValue = pFrontWall->cloakValue;
		pBackWall->nType = WALL_CLOAKED;
		}
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	tUVL *uvlP = SEGMENT (pFrontWall->nSegment)->m_sides [pFrontWall->nSide].m_uvls;
	NDRecordCloakingWall (pCloakWall->nFrontWall, pCloakWall->nBackWall, pFrontWall->nType, pFrontWall->state, pFrontWall->cloakValue,
								 uvlP [0].l, uvlP [1].l, uvlP [2].l, uvlP [3].l);
	}
}

//------------------------------------------------------------------------------

void WallFrameProcess (void)
{
	CCloakingWall*	pCloakWall;
	CActiveDoor*	pDoor = gameData.wallData.activeDoors.Buffer ();
	CWall*			pWall, *pBackWall;
	uint32_t				i;

for (i = 0; i < gameData.wallData.activeDoors.ToS (); i++) {
	pDoor = &gameData.wallData.activeDoors [i];
	pBackWall = NULL,
	pWall = WALL (pDoor->nFrontWall [0]);
	if (pWall->state == WALL_DOOR_OPENING) {
		if (DoDoorOpen (i))
			i--;
		}
	else if (pWall->state == WALL_DOOR_CLOSING) {
		if (DoDoorClose (i))
			i--;	// active door has been deleted - account for it
		}
	else if (pWall->state == WALL_DOOR_WAITING) {
		pDoor->time += gameData.time.xFrame;

		//set flags to fix occatsional netgame problem where door is
		//waiting to close but open flag isn't set
//			Assert(pDoor->nPartCount == 1);
		if (IS_WALL (pDoor->nBackWall [0]))
			WALL (pDoor->nBackWall [0])->state = WALL_DOOR_CLOSING;
		if ((pDoor->time > DOOR_WAIT_TIME) && !SEGMENT (pWall->nSegment)->DoorIsBlocked (pWall->nSide, pWall->IgnoreMarker ())) {
			pWall->state = WALL_DOOR_CLOSING;
			pDoor->time = 0;
			}
		else {
			pWall->flags |= WALL_DOOR_OPENED;
			if (pBackWall)
				pBackWall->flags |= WALL_DOOR_OPENED;
			}
		}
	else if ((pWall->state == WALL_DOOR_CLOSED) || (pWall->state == WALL_DOOR_OPEN)) {
		//this shouldn't happen.  if the CWall is in one of these states,
		//there shouldn't be an activedoor entry for it.  So we'll kill
		//the activedoor entry.  Tres simple.
		DeleteActiveDoor (i--);
		}
	}

pCloakWall = gameData.wallData.cloaking.Buffer ();
for (i = 0; i < gameData.wallData.cloaking.ToS (); i++, pCloakWall++) {
	uint8_t s = WALL (pCloakWall->nFrontWall)->state;
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

void AddStuckObject (CObject *pObj, int16_t nSegment, int16_t nSide)
{
	int32_t				i;
	int16_t				nWall;
	tStuckObject	*pStuckObj;

CWall* pWall = SEGMENT (nSegment)->Wall (nSide);
if (pWall) {
	if (pWall->flags & WALL_BLASTED)
		pObj->Die ();
	nWall = pWall - WALLS;
	for (i = 0, pStuckObj = stuckObjects; i < MAX_STUCK_OBJECTS; i++, pStuckObj++) {
		if (pStuckObj->nWall == NO_WALL) {
			pStuckObj->nWall = nWall;
			pStuckObj->nObject = pObj->Index ();
			pStuckObj->nSignature = pObj->info.nSignature;
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
//	Safety and efficiency code.  If no stuck OBJECTS, should never get inside the IF, but this is faster.
if (!nStuckObjects)
	return;
int16_t nObject = gameData.app.nFrameCount % MAX_STUCK_OBJECTS;
tStuckObject *pStuckObj = stuckObjects + nObject;
CObject *pObj = OBJECT (pStuckObj->nObject);
CWall* pWall = WALL (pStuckObj->nWall);
if ((pWall && (pWall->state != WALL_DOOR_CLOSED)) || !pObj || (pObj->info.nSignature != pStuckObj->nSignature)) {
	nStuckObjects--;
	if (pObj)
		pObj->UpdateLife (I2X (1) / 8);
	pStuckObj->nWall = NO_WALL;
	}
}

//	----------------------------------------------------------------------------------------------------
//	Door with CWall index nWall is opening, kill all OBJECTS stuck in it.
void KillStuckObjects (int32_t nWall)
{
	int32_t			i;
	tStuckObject	*pStuckObj;
	CObject			*pObj;

if (!IS_WALL (nWall) || (nStuckObjects == 0))
	return;
nStuckObjects = 0;

for (i = 0, pStuckObj = stuckObjects; i < MAX_STUCK_OBJECTS; i++, pStuckObj++)
	if (pStuckObj->nWall == nWall) {
		pObj = OBJECT (pStuckObj->nObject);
		if (pObj && (pObj->info.nType == OBJ_WEAPON))
			pObj->UpdateLife (I2X (1) / 8);
		else {
#if TRACE
			console.printf (1,
				"Warning: Stuck CObject of nType %i, expected to be of nType %i, see CWall.c\n",
				pObj->info.nType, OBJ_WEAPON);
#endif
			// Int3();	//	What?  This looks bad.  Object is not a weapon and it is stuck in a CWall!
			pStuckObj->nWall = NO_WALL;
			}
		}
	else if (IS_WALL (pStuckObj->nWall)) {
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
	tStuckObject	*pStuckObj = stuckObjects;
	CObject			*pObj;

for (int32_t i = 0; i < MAX_STUCK_OBJECTS; i++, pStuckObj++) {
	if (IS_WALL (pStuckObj->nWall)) {
		pObj = OBJECT (pStuckObj->nObject);
		if (pObj && (pObj->info.nType == OBJ_WEAPON) && (pObj->info.nId == FLARE_ID))
			pObj->UpdateLife (I2X (1) / 8);
		pStuckObj->nWall = NO_WALL;
		nStuckObjects--;
		}
	}
if (nStuckObjects)
	nStuckObjects = 0;
}

// -----------------------------------------------------------------------------------
#define	MAX_BLAST_GLASS_DEPTH	5

void BngProcessSegment (CObject *pObj, fix damage, CSegment *pSeg, int32_t depth, int8_t *visited)
{
	int32_t	i;
	int16_t	nSide;

if (depth > MAX_BLAST_GLASS_DEPTH)
	return;

depth++;

for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
	if (!pSeg->Side (nSide)->FaceCount ())
		continue;
		
		int32_t		tm;
		fix			dist;
		CFixVector	pnt;

	//	Process only walls which have glass.
	if ((tm = pSeg->m_sides [nSide].m_nOvlTex)) {
		int32_t ec = gameData.pig.tex.tMapInfoP [tm].nEffectClip;
		tEffectInfo* pEffectInfo = (ec < 0) ? NULL : gameData.effects.pEffect + ec;
		int32_t db = pEffectInfo ? pEffectInfo->destroyed.nTexture : -1;

		if (((ec != -1) && (db != -1) && !(pEffectInfo->flags & EF_ONE_SHOT)) ||
		 	 ((ec == -1) && (gameData.pig.tex.tMapInfoP [tm].destroyed != -1))) {
			pnt = pSeg->SideCenter (nSide);
			dist = CFixVector::Dist(pnt, pObj->info.position.vPos);
			if (dist < damage / 2) {
				dist = simpleRouter [0].PathLength (pnt, pSeg->Index (), pObj->info.position.vPos, pObj->info.nSegment, MAX_BLAST_GLASS_DEPTH, WID_TRANSPARENT_FLAG, -1);
				CObject* pParent = OBJECT (pObj->cType.laserInfo.parent.nObject);
				if ((dist > 0) && (dist < damage / 2) && pSeg->BlowupTexture (nSide, pnt, pParent, 1))
						pSeg->OperateTrigger (nSide, pParent, 1);
				}
			}
		}
	}

for (i = 0; i < SEGMENT_SIDE_COUNT; i++) {
	int16_t nSegment = pSeg->m_children [i];

	if ((nSegment != -1) && !visited [nSegment] && (pSeg->IsPassable (i, NULL) & WID_PASSABLE_FLAG)) {
		visited [nSegment] = 1;
		BngProcessSegment (pObj, damage, SEGMENT (nSegment), depth, visited);
		}
	}
}

// -----------------------------------------------------------------------------------

int32_t CWall::IsTriggerTarget (int32_t i)
{
CTrigger *pTrigger = GEOTRIGGERS.Buffer (i);
for (; i < gameData.trigData.m_nTriggers [0]; i++, pTrigger++) {
	int16_t *pSegNum = pTrigger->m_segments;
	int16_t *pSideNum = pTrigger->m_sides;
	for (int32_t j = pTrigger->m_nLinks; j; j--, pSegNum++, pSideNum++)
		if ((*pSegNum == nSegment) && (*pSideNum == nSide))
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
		CTrigger *pTrigger = GEOTRIGGERS.Buffer ();
		for (int32_t i = 0; i < gameData.trigData.m_nTriggers [0]; i++, pTrigger++) {
			int16_t *pSegNum = pTrigger->m_segments;
			int16_t *pSideNum = pTrigger->m_sides;
			for (int32_t j = pTrigger->m_nLinks; j; j--, pSegNum++, pSideNum++) {
				if ((*pSegNum != nSegment) && (*pSideNum != nSide))
					continue;
				if ((pTrigger->m_info.nType == TT_OPEN_DOOR) || (pTrigger->m_info.nType == TT_ILLUSION_OFF) || (pTrigger->m_info.nType == TT_OPEN_WALL)) {
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
CSide* pSide = SEGMENT (nSegment)->OppositeSide (nSide);
return pSide ? pSide->Wall () : NULL;
}

// -----------------------------------------------------------------------------------

bool CWall::IsInvisible (void)
{
if ((nType != WALL_OPEN) && ((nType != WALL_CLOAKED) || (cloakValue <  FADE_LEVELS)))
	return false;
if (IsTriggerTarget () >= 0)
	return false;
CWall* pWall = OppositeWall ();
return (pWall != NULL) && (pWall->IsTriggerTarget () < 0);
}

// -----------------------------------------------------------------------------------
//	pObj is going to detonate
//	blast nearby monitors, lights, maybe other things
void BlastNearbyGlass (CObject *pObj, fix damage)
{
	int32_t	i;
	int8_t	visited [MAX_SEGMENTS_D2X];
	CSegment	*cursegp;

cursegp = SEGMENT (pObj->info.nSegment);
for (i=0; i<=gameData.segData.nLastSegment; i++)
memset (visited, 0, sizeof (visited));
visited [pObj->info.nSegment] = 1;
BngProcessSegment(pObj, damage, cursegp, 0, visited);
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
	CExplodingWall * pExplodingWall = AddExplodingWall ();

if (pExplodingWall) {
	pExplodingWall->nSegment = nSegment;
	pExplodingWall->nSide = nSide;
	pExplodingWall->time = 0;
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
		CSegment *pSeg = SEGMENT (nSegment),
					*pConnSeg = SEGMENT (pSeg->m_children [nSide]);
		uint8_t	a = (uint8_t) pSeg->Wall (nSide)->nClip;
		int16_t n = WallEffectFrameCount (gameData.wallData.pAnim + a);
		int16_t nConnSide = pSeg->ConnectedSide (pConnSeg);
		pSeg->SetTexture (nSide, pConnSeg, nConnSide, a, n - 1);
		pSeg->Wall (nSide)->flags |= WALL_BLASTED;
		if (nConnSide >= 0) {
			CWall* pWall = pConnSeg->Wall (nConnSide);
			if (pWall)
				pWall->flags |= WALL_BLASTED;
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
	CWall* pWall = WALLS.Buffer ();

for (int32_t i = 0; i < gameData.wallData.nWalls; i++, pWall++) {
	if ((pWall->nType == WALL_DOOR) && (pWall->flags & WALL_DOOR_OPENED)) {
		if ((pWall->flags & WALL_DOOR_AUTO) && !FindActiveDoor (i)) {	// make sure pre-opened automatic doors will close after a while
			pWall->flags &= ~WALL_DOOR_OPENED;
			SEGMENT (pWall->nSegment)->OpenDoor (pWall->nSide);
			}
		SEGMENT (pWall->nSegment)->AnimateOpeningDoor (pWall->nSide, -1);
		}
	else if ((pWall->nType == WALL_BLASTABLE) && (pWall->flags & WALL_BLASTED))
		SEGMENT (pWall->nSegment)->BlastWall (pWall->nSide);
	}
}

//------------------------------------------------------------------------------

void FixWalls (void)
{
	CWall*			pWall;
	uint32_t				i, j;
	uint8_t				bActive [MAX_WALLS];

memset (bActive, 0, sizeof (bActive));
for (i = 0; i < gameData.wallData.activeDoors.ToS (); i++)
	bActive [gameData.wallData.activeDoors [i].nFrontWall [0]] = 1;

for (i = 0; i < gameData.wallData.cloaking.ToS (); i++)
	bActive [gameData.wallData.cloaking [i].nFrontWall] |= 2;

pWall = gameData.wallData.walls.Buffer ();
for (i = 0, j = gameData.wallData.walls.Length (); i < j; i++, pWall++) {
	switch (pWall->state) {
		case WALL_DOOR_OPENING:
			if (bActive [i] & 1)
				continue;
			pWall->state = WALL_DOOR_CLOSED;
			SEGMENT (pWall->nSegment)->OpenDoor (pWall->nSide);
			break;

		case WALL_DOOR_WAITING:
		case WALL_DOOR_CLOSING:
			if (bActive [i] & 1)
				continue;
			pWall->state = WALL_DOOR_OPEN;
			SEGMENT (pWall->nSegment)->CloseDoor (pWall->nSide);
			break;

		case WALL_DOOR_CLOAKING:
			if (bActive [i] & 2)
				continue;
			pWall->state = 0;
			SEGMENT (pWall->nSegment)->StartCloak (pWall->nSide);
			break;

		case WALL_DOOR_DECLOAKING:
			if (bActive [i] & 2)
				continue;
			pWall->state = 0;
			SEGMENT (pWall->nSegment)->StartDecloak (pWall->nSide);
			break;

		default:
			continue;
		}
	}
}

// -----------------------------------------------------------------------------
