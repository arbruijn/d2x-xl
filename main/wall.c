/* $Id: wall.c,v 1.10 2003/10/04 03:14:48 btb Exp $ */
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
 * Destroyable wall stuff
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:32:08  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/21  14:39:04  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.0  1995/02/27  11:28:32  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.112  1995/02/22  13:53:07  allender
 * remove anonymous unions from tObject structure
 *
 * Revision 1.111  1995/02/01  17:32:17  adam
 * Took out a bogus int3.
 *
 * Revision 1.110  1995/02/01  17:20:24  john
 * Lintized.
 *
 * Revision 1.109  1995/01/21  17:39:50  matt
 * Cleaned up laser/player hit wall confusions
 *
 * Revision 1.108  1995/01/21  17:14:17  rob
 * Fixed bug in multiplayer door-butting.
 *
 * Revision 1.107  1995/01/18  18:57:11  rob
 * Added new hostage door hooks.
 *
 * Revision 1.106  1995/01/18  18:48:18  allender
 * removed #ifdef newdemo's.  Added function call to record a door that
 * starts to open. This fixes the rewind problem
 *
 * Revision 1.105  1995/01/16  11:55:39  mike
 * make control center (and robots whose id == your playernum) not able to open doors.
 *
 * Revision 1.104  1994/12/11  23:07:21  matt
 * Fixed stuck gameData.objs.objects & blastable walls
 *
 * Revision 1.103  1994/12/10  16:44:34  matt
 * Added debugging code to track down door that turns into rock
 *
 * Revision 1.102  1994/12/06  16:27:05  matt
 * Added debugging
 *
 * Revision 1.101  1994/12/02  10:50:27  yuan
 * Localization
 *
 * Revision 1.100  1994/11/30  19:41:22  rob
 * Put in a fix so that door opening sounds travel through the door.
 *
 * Revision 1.99  1994/11/28  11:59:50  yuan
 * *** empty log message ***
 *
 * Revision 1.98  1994/11/28  11:25:45  matt
 * Cleaned up key hud messages
 *
 * Revision 1.97  1994/11/27  23:15:11  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.96  1994/11/19  15:18:29  mike
 * rip out unused code and data.
 *
 * Revision 1.95  1994/11/17  14:57:12  mike
 * moved tSegment validation functions from editor to main.
 *
 * Revision 1.94  1994/11/07  08:47:30  john
 * Made wall state record.
 *
 * Revision 1.93  1994/11/04  16:06:37  rob
 * Fixed network damage of blastable walls.
 *
 * Revision 1.92  1994/11/02  21:54:01  matt
 * Don't let gameData.objs.objects with zero size keep door from shutting
 *
 * Revision 1.91  1994/10/31  13:48:42  rob
 * Fixed bug in opening doors over network/modem.  Added a new message
 * nType to multi.c that communicates door openings across the net.
 * Changed includes in multi.c and wall.c to accomplish this.
 *
 * Revision 1.90  1994/10/28  14:42:41  john
 * Added sound volumes to all sound calls.
 *
 * Revision 1.89  1994/10/23  19:16:55  matt
 * Fixed bug with "no key" messages
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: wall.c,v 1.10 2003/10/04 03:14:48 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "mono.h"
#include "gr.h"
#include "wall.h"
#include "switch.h"
#include "inferno.h"
#include "segment.h"
#include "error.h"
#include "gameseg.h"
#include "game.h"
#include "bm.h"
#include "vclip.h"
#include "player.h"
#include "gauges.h"
#include "text.h"
#include "fireball.h"
#include "textures.h"
#include "sounds.h"
#include "newdemo.h"
#include "multi.h"
#include "gameseq.h"
#include "laser.h"		//	For seeing if a flare is stuck in a wall.
#include "collide.h"
#include "effects.h"
#include "ogl_init.h"
#include "hudmsg.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

//	Special door on boss level which is locked if not in multiplayer...sorry for this awful solution --MK.
#define	BOSS_LOCKED_DOOR_LEVEL	7
#define	BOSS_LOCKED_DOOR_SEG		595
#define	BOSS_LOCKED_DOOR_SIDE	5

//--unused-- int gameData.walls.bitmaps[MAX_WALL_ANIMS];

//door Doors[MAX_DOORS];


#define CLOAKING_WALL_TIME f1_0

//--unused-- grs_bitmap *wall_title_bms[MAX_WALL_ANIMS];

//#define BM_FLAG_TRANSPARENT			1
//#define BM_FLAG_SUPER_TRANSPARENT	2

#ifdef EDITOR
char	pszWallNames[7][10] = {
	"NORMAL   ",
	"BLASTABLE",
	"DOOR     ",
	"ILLUSION ",
	"OPEN     ",
	"CLOSED   ",
	"EXTERNAL "
};
#endif

// Function prototypes
void KillStuckObjects(int wallnum);



//-----------------------------------------------------------------

int AnimFrameCount (wclip *anim)
{
grs_bitmap *bmP = gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [anim->frames [0]].index;
return ((bmP->bmType == BM_TYPE_ALT) && BM_FRAMES (bmP)) ? BM_FRAMECOUNT (bmP) : anim->nFrameCount;
}

//-----------------------------------------------------------------

fix AnimPlayTime (wclip *anim)
{
int nFrames = AnimFrameCount (anim);
fix pt = anim->xTotalTime;

if (nFrames == anim->nFrameCount)
	return pt;
return (fix) (((double) pt * (double) anim->nFrameCount) / (double) nFrames);
}

// This function determines whether the current tSegment/tSide is transparent
//		1 = YES
//		0 = NO
//-----------------------------------------------------------------

int CheckTransparency (tSegment *segP, short nSide)
{
	tSide *sideP = segP->sides + nSide;
	grs_bitmap	*bmP;

if (sideP->nOvlTex) {
	bmP = BmOverride (gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [sideP->nOvlTex].index);
	if ((bmP->bmType == BM_TYPE_ALT) && BM_FRAMES (bmP)) {
		int i = (int) (BM_CURFRAME (bmP) - BM_FRAMES (bmP));
		if (bmP->bm_data.alt.bm_supertranspFrames [i / 32] & (1 << (i % 32)))
			return 1;
		}
	else if (bmP->bm_props.flags & BM_FLAG_SUPER_TRANSPARENT)
		return 1;
	}
else {
	bmP = BmOverride (gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [sideP->nBaseTex].index);
	if ((bmP->bmType == BM_TYPE_ALT) && BM_FRAMES (bmP)) {
		int i = (int) (BM_CURFRAME (bmP) - BM_FRAMES (bmP));
		if (bmP->bm_data.alt.bm_transparentFrames [i / 32] & (1 << (i % 32)))
			return 1;
		}
	else if (bmP->bm_props.flags & BM_FLAG_TRANSPARENT)
		return 1;
	}
if (gameStates.app.bD2XLevel) {
	short	c, nWallNum = WallNumP (segP, nSide);
	if (IS_WALL (nWallNum)) {
		c = gameData.walls.walls [nWallNum].cloakValue;
		if (c && (c < GR_ACTUAL_FADE_LEVELS))
			return 1;
		}
	}
return 0;
}

//define these here so I don't have to change WallIsDoorWay and run
//the risk of screwing it up.
#define WID_WALL						2	// 0/1/0		wall
#define WID_TRANSPARENT_WALL		6	//	0/1/1		transparent wall
#define WID_ILLUSORY_WALL			3	//	1/1/0		illusory wall
#define WID_TRANSILLUSORY_WALL	7	//	1/1/1		transparent illusory wall
#define WID_NO_WALL					5	//	1/0/1		no wall, can fly through
#define WID_EXTERNAL					8	// 0/0/0/1	don't see it, dont fly through it

//-----------------------------------------------------------------
// This function checks whether we can fly through the given tSide.
//	In other words, whether or not we have a 'doorway'
//	 Flags:
//		WID_FLY_FLAG				1
//		WID_RENDER_FLAG			2
//		WID_RENDPAST_FLAG			4
//	 Return values:
//		WID_WALL						2	// 0/1/0		wall
//		WID_TRANSPARENT_WALL		6	//	0/1/1		transparent wall
//		WID_ILLUSORY_WALL			3	//	1/1/0		illusory wall
//		WID_TRANSILLUSORY_WALL	7	//	1/1/1		transparent illusory wall
//		WID_NO_WALL					5	//	1/0/1		no wall, can fly through
int WallIsDoorWay (tSegment * seg, short tSide)
{
	int flags, nType;
	int state;
	wall * wallP = gameData.walls.walls + WallNumP (seg, tSide);
#ifdef _DEBUG
	short nSegment = SEG_IDX (seg);
#endif
//--Covered by macro	// No child.
//--Covered by macro	if (seg->children[tSide] == -1)
//--Covered by macro		return WID_WALL;

//--Covered by macro	if (seg->children[tSide] == -2)
//--Covered by macro		return WID_EXTERNAL_FLAG;

//--Covered by macro // No wall present.
//--Covered by macro	if (!IS_WALL (WallNumP (seg, tSide)))
//--Covered by macro		return WID_NO_WALL;

Assert(nSegment>=0 && nSegment<=gameData.segs.nLastSegment);
Assert(tSide>=0 && tSide<6);


nType = wallP->nType;
flags = wallP->flags;

if (nType == WALL_OPEN)
	return WID_NO_WALL;

if (nType == WALL_ILLUSION) {
	if (flags & WALL_ILLUSION_OFF)
		return WID_NO_WALL;
	else {
		if ((wallP->cloakValue < GR_ACTUAL_FADE_LEVELS) || CheckTransparency(seg, tSide))
			return WID_TRANSILLUSORY_WALL;
		else
			return WID_ILLUSORY_WALL;
		}
	}

if (nType == WALL_BLASTABLE) {
	if (flags & WALL_BLASTED)
		return WID_TRANSILLUSORY_WALL;

	if ((wallP->cloakValue < GR_ACTUAL_FADE_LEVELS) || CheckTransparency(seg, tSide))
		return WID_TRANSPARENT_WALL;
	else
		return WID_WALL;
	}

if (flags & WALL_DOOR_OPENED)
	return WID_TRANSILLUSORY_WALL;

if (nType == WALL_CLOAKED)
	return WID_RENDER_FLAG | WID_RENDPAST_FLAG | WID_CLOAKED_FLAG;

if (nType == WALL_TRANSPARENT)
	return 
#if 1
		(wallP->hps < 0) ? 
		WID_RENDER_FLAG | WID_RENDPAST_FLAG | WID_TRANSPARENT_FLAG | WID_FLY_FLAG : 
#endif
		WID_RENDER_FLAG | WID_RENDPAST_FLAG | WID_TRANSPARENT_FLAG;

state = wallP->state;
if (nType == WALL_DOOR) 
	if ((state == WALL_DOOR_OPENING) || (state == WALL_DOOR_CLOSING))
		return WID_TRANSPARENT_WALL;
	else if (CheckTransparency (seg, tSide))
		return WID_TRANSPARENT_WALL;
	else
		return WID_WALL;

// If none of the above flags are set, there is no doorway.
if ((wallP->cloakValue && (wallP->cloakValue < GR_ACTUAL_FADE_LEVELS)) || 
	 CheckTransparency (seg, tSide))
	return WID_TRANSPARENT_WALL;
else
	return WID_WALL; // There are children behind the door.
}

//-----------------------------------------------------------------

int WALL_IS_DOORWAY (tSegment *seg, short tSide, tObject *objP)
{
	int	wallnum, nSegment, childnum = seg->children [tSide];
	
if (childnum == -1)
	return WID_RENDER_FLAG;
if (childnum == -2) 
	return WID_EXTERNAL_FLAG;
nSegment = SEG_IDX (seg);
wallnum = WallNumP (seg, tSide);
if (gameData.objs.speedBoost [OBJ_IDX (objP)].bBoosted &&
	 (objP == gameData.objs.console) && 
	 (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SPEEDBOOST) &&
	 (gameData.segs.segment2s [childnum].special != SEGMENT_IS_SPEEDBOOST) &&
	 ((wallnum < 0) || (gameData.trigs.triggers [gameData.walls.walls [wallnum].nTrigger].nType != TT_SPEEDBOOST)))
	return objP ? WID_RENDER_FLAG : (!IS_WALL (wallnum)) ? WID_RENDPAST_FLAG : WallIsDoorWay (seg, tSide);
if ((gameData.segs.segment2s [childnum].special == SEGMENT_IS_BLOCKED) ||
	 (gameData.segs.segment2s [childnum].special == SEGMENT_IS_SKYBOX))
	return (objP && ((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT))) ? WID_RENDER_FLAG : 
			 (!IS_WALL (wallnum)) ? WID_FLY_FLAG | WID_RENDPAST_FLAG : WallIsDoorWay (seg, tSide);
if (!IS_WALL (wallnum)) 
	return (WID_FLY_FLAG|WID_RENDPAST_FLAG);
return WallIsDoorWay (seg, tSide);
}
#ifdef EDITOR

//-----------------------------------------------------------------
// Initializes all the walls (in other words, no special walls)
void WallInit ()
{
	int i;
	wall *w = gameData.walls.walls;

	gameData.walls.nWalls = 0;
	for (i = 0; i < MAX_WALLS; i++, w++) {
		w->nSegment = 
		w->nSide = -1;
		w->nType = WALL_NORMAL;
		w->flags = 0;
		w->hps = 0;
		w->nTrigger = NO_TRIGGER;
		w->nClip = -1;
		w->nLinkedWall = NO_WALL;
		}
	gameData.walls.nOpenDoors = 0;
	gameData.walls.nCloaking = 0;

}

//-----------------------------------------------------------------
// Initializes one wall.
void WallReset (tSegment *seg, short tSide)
{
	wall *w;
	int i = WallNumP (seg, tSide);

if (!IS_WALL (i)) {
#if TRACE
	con_printf (CON_DEBUG, "Resetting Illegal Wall\n");
#endif
	return;
	}
w = gameData.walls.walls;
w->nSegment = SEG_IDX (seg);
w->nSide = tSide;
w->nType = WALL_NORMAL;
w->flags = 0;
w->hps = 0;
w->nTrigger = NO_TRIGGER;
w->nClip = -1;
w->nLinkedWall = NO_WALL;
}
#endif

//-----------------------------------------------------------------
//set the nBaseTex or nOvlTex field for a wall/door
void WallSetTMapNum (tSegment *seg, short tSide, tSegment *csegp, short cside, int anim_num, int nFrame)
{
wclip *anim = gameData.walls.pAnims + anim_num;
short tmap = anim->frames [(anim->flags & WCF_ALTFMT) ? 0 : nFrame];
grs_bitmap *bmP;
int nFrames;

//if (gameData.demo.nState == ND_STATE_PLAYBACK) 
//	return;

if (cside < 0)
	csegp = NULL;
if (anim->flags & WCF_ALTFMT) {
	nFrames = anim->nFrameCount;
	bmP = SetupHiresAnim ((short *) anim->frames, nFrames, -1, 1, 0, &nFrames);
	//if (anim->flags & WCF_TMAP1)
	if (!bmP)
		anim->flags &= ~WCF_ALTFMT;
	else {
		bmP->bm_wallAnim = 1;
		if (!gameOpts->ogl.bGlTexMerge) 
			anim->flags &= ~WCF_ALTFMT;
		else if (!BM_FRAMES (bmP)) 
			anim->flags &= ~WCF_ALTFMT;
		else {
			anim->flags |= WCF_INITIALIZED;
			BM_CURFRAME (bmP) = BM_FRAMES (bmP) + nFrame;
			OglLoadBmTexture (BM_CURFRAME (bmP), 1, 0);
			nFrame++;
			if (nFrame > nFrames)
				nFrame = nFrames;
			}
		}
	}
else if (anim->flags & WCF_TMAP1) {
	seg->sides [tSide].nBaseTex = tmap;
	if (csegp)
		csegp->sides [cside].nBaseTex = tmap;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordWallSetTMapNum1(
			SEG_IDX (seg), (ubyte) tSide, (short) (csegp ? SEG_IDX (csegp) : -1), (ubyte) cside, tmap);
	}
else {
	//Assert(tmap!=0 && seg->sides[tSide].nOvlTex!=0);
	seg->sides[tSide].nOvlTex = tmap;
	if (csegp)
		csegp->sides[cside].nOvlTex = tmap;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordWallSetTMapNum2(
		SEG_IDX (seg), (ubyte) tSide, (short) (csegp ? SEG_IDX (csegp) : -1), (ubyte) cside, tmap);
	}
seg->sides [tSide].nFrame = -nFrame;
if (csegp)
	csegp->sides [cside].nFrame = -nFrame;
}


// -------------------------------------------------------------------------------
//when the wall has used all its hitpoints, this will destroy it
void BlastBlastableWall (tSegment *seg, short tSide)
{
	short Connectside;
	tSegment *csegp;
	int a, n;
	short nWall, cwall_num;
	wall *w;

nWall = WallNumP (seg, tSide);
Assert(IS_WALL (nWall));
w = gameData.walls.walls + nWall;
w->hps = -1;	//say it's blasted

if (seg->children [tSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_BLAST_SINGLE,
			gameData.segs.segments - seg, tSide, nWall);
	csegp = NULL;
	Connectside = -1;
	cwall_num = NO_WALL;
	}
else {
	csegp = gameData.segs.segments + seg->children[tSide];
	Connectside = FindConnectedSide(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = WallNumP (csegp, Connectside);
	}
KillStuckObjects(WallNumP (seg, tSide));
if (IS_WALL (cwall_num))
	KillStuckObjects(cwall_num);

//if this is an exploding wall, explode it
if (gameData.walls.pAnims[w->nClip].flags & WCF_EXPLODES)
	ExplodeWall(SEG_IDX (seg),tSide);
else {
	//if not exploding, set final frame, and make door passable
	a = w->nClip;
	n = AnimFrameCount (gameData.walls.pAnims + a);
	WallSetTMapNum(seg,tSide,csegp,Connectside,a,n-1);
	w->flags |= WALL_BLASTED;
	if (IS_WALL (cwall_num))
		gameData.walls.walls [cwall_num].flags |= WALL_BLASTED;
	}
}


//-----------------------------------------------------------------
// Destroys a blastable wall.
void WallDestroy (tSegment *seg, short tSide)
{
Assert(IS_WALL (WallNumP (seg, tSide)));
Assert(SEG_IDX (seg) != 0);

if (gameData.walls.walls[WallNumP (seg, tSide)].nType == WALL_BLASTABLE)
	BlastBlastableWall(seg, tSide);
else
	Error("Hey bub, you are trying to destroy an indestructable wall.");
}

//-----------------------------------------------------------------
// Deteriorate appearance of wall. (Changes bitmap (paste-ons))
void WallDamage (tSegment *seg, short tSide, fix damage)
{
	int a, i, n;
	short cwall_num, nWall = WallNumP (seg, tSide);
	wall *w;

if (!IS_WALL (nWall)) {
#if TRACE
	con_printf (CON_DEBUG, "Damaging illegal wall\n");
#endif
	return;
	}
w = gameData.walls.walls + nWall;
if (w->nType != WALL_BLASTABLE)
	return;

if (!(w->flags & WALL_BLASTED) && w->hps >= 0)
	{
	short Connectside;
	tSegment *csegp;

if (seg->children [tSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_DMG_SINGLE,
			gameData.segs.segments - seg, tSide, nWall);
	csegp = NULL;
	Connectside = -1;
	cwall_num = NO_WALL;
	}
else {
	csegp = gameData.segs.segments + seg->children[tSide];
	Connectside = FindConnectedSide(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = WallNumP (csegp, Connectside);
	}
	w->hps -= damage;
	if (IS_WALL (cwall_num))
		gameData.walls.walls[cwall_num].hps -= damage;

	a = w->nClip;
	n = AnimFrameCount (gameData.walls.pAnims + a);

	if (w->hps < WALL_HPS*1/n) {
		BlastBlastableWall(seg, tSide);
		#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendDoorOpen(SEG_IDX (seg), tSide,w->flags);
		#endif
	}
	else
		for (i=0;i<n;i++)
			if (w->hps < WALL_HPS*(n-i)/n) {
				WallSetTMapNum(seg,tSide,csegp,Connectside,a,i);
			}
	}
}


//-----------------------------------------------------------------
// Opens a door
void WallOpenDoor (tSegment *seg, short tSide)
{
	wall *w;
	active_door *d;
	short  Connectside, nWall, cwall_num;
	tSegment *csegp;

nWall = WallNumP (seg, tSide);
Assert(IS_WALL (nWall)); 	//Opening door on illegal wall
if (!IS_WALL (nWall))
	return;

w = gameData.walls.walls + nWall;
//KillStuckObjects(WallNumP (seg, tSide));

if ((w->state == WALL_DOOR_OPENING) ||		//already opening
	 (w->state == WALL_DOOR_WAITING)	||		//open, waiting to close
	 (w->state == WALL_DOOR_OPEN))			//open, & staying open
	return;

if (w->state == WALL_DOOR_CLOSING) {		//closing, so reuse door

	int i, j;

	d = gameData.walls.activeDoors;
	for (i = 0; i < gameData.walls.nOpenDoors; i++, d++) {		//find door
		for (j = 0; j < d->n_parts; j++)
			if ((d->nFrontWall[j]==nWall) || (d->nBackWall[j]==nWall))
				goto foundDoor;
		}
	if (gameData.app.nGameMode & GM_MULTI)
		goto FastFix;
foundDoor:

	Assert(i<gameData.walls.nOpenDoors);				//didn't find door!
	Assert(d!=NULL); // Get John!

	d->time = gameData.walls.pAnims[w->nClip].xTotalTime - d->time;
	if (d->time < 0)
		d->time = 0;

}
else {											//create new door
	Assert(w->state == WALL_DOOR_CLOSED);
	FastFix:
	Assert(gameData.walls.nOpenDoors < MAX_DOORS);
	d = gameData.walls.activeDoors + gameData.walls.nOpenDoors++;
	d->time = 0;
	}


w->state = WALL_DOOR_OPENING;

// So that door can't be shot while opening
if (seg->children [tSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_OPEN_SINGLE, gameData.segs.segments - seg, tSide, nWall);
	csegp = NULL;
	Connectside = -1;
	cwall_num = NO_WALL;
	}
else {
	csegp = gameData.segs.segments + seg->children[tSide];
	Connectside = FindConnectedSide(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = WallNumP (csegp, Connectside);
	}

if (IS_WALL (cwall_num))
	gameData.walls.walls[cwall_num].state = WALL_DOOR_OPENING;

//KillStuckObjects(WallNumP (csegp, Connectside));

d->nFrontWall[0] = nWall;
d->nBackWall[0] = cwall_num;

Assert(SEG_IDX (seg) != -1);

if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordDoorOpening(SEG_IDX (seg), tSide);

if (IS_WALL (w->nLinkedWall) && IS_WALL (cwall_num) && (w->nLinkedWall == cwall_num)) {
	wall *w2;
	tSegment *seg2;

	w2		= gameData.walls.walls + w->nLinkedWall;
	seg2	= gameData.segs.segments + w2->nSegment;

	//Assert(w2->nLinkedWall == WallNumP (seg, tSide));
	//Assert(!(w2->flags & WALL_DOOR_OPENING  ||  w2->flags & WALL_DOOR_OPENED);

	w2->state = WALL_DOOR_OPENING;

	csegp = gameData.segs.segments + seg2->children[w2->nSide];
	Connectside = FindConnectedSide(seg2, csegp);
	Assert(Connectside != -1);
	if (IS_WALL (cwall_num))
		gameData.walls.walls[cwall_num].state = WALL_DOOR_OPENING;

	d->n_parts = 2;
	d->nFrontWall[1] = w->nLinkedWall;
	d->nBackWall[1] = cwall_num;
	}
else
	d->n_parts = 1;


if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	// NOTE THE LINK TO ABOVE!!!!
	vmsVector cp;
	COMPUTE_SIDE_CENTER(&cp, seg, tSide);
	if (gameData.walls.pAnims[w->nClip].openSound > -1)
		DigiLinkSoundToPos(gameData.walls.pAnims[w->nClip].openSound, SEG_IDX (seg), tSide, &cp, 0, F1_0);
	}
}

//-----------------------------------------------------------------
// start the transition from closed -> open wall
void StartWallCloak (tSegment *seg, short tSide)
{
	wall *w;
	cloaking_wall *d;
	short Connectside;
	tSegment *csegp;
	int i;
	short cwall_num;

	if (gameData.demo.nState==ND_STATE_PLAYBACK) return;

	Assert(IS_WALL (WallNumP (seg, tSide))); 	//Opening door on illegal wall

	w = gameData.walls.walls + WallNumP (seg, tSide);
	if (w->nType == WALL_OPEN || w->state == WALL_DOOR_CLOAKING)		//already open or cloaking
		return;

	csegp = gameData.segs.segments + seg->children[tSide];
	Connectside = FindConnectedSide(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = WallNumP (csegp, Connectside);

	if (w->state == WALL_DOOR_DECLOAKING) {	//decloaking, so reuse door
			int i;

		d = NULL;
		for (i=0;i<gameData.walls.nCloaking;i++) {		//find door
			d = gameData.walls.cloaking + i;
			if (d->nFrontWall==w-gameData.walls.walls || d->nBackWall==w-gameData.walls.walls)
				break;
		}

		Assert(i<gameData.walls.nCloaking);				//didn't find door!
		Assert(d!=NULL); // Get John!
		d->time = CLOAKING_WALL_TIME - d->time;

	}
	else if (w->state == WALL_DOOR_CLOSED) {	//create new door
		d = gameData.walls.cloaking + gameData.walls.nCloaking;
		d->time = 0;
		if (gameData.walls.nCloaking >= MAX_CLOAKING_WALLS) {		//no more!
			Int3();		//ran out of cloaking wall slots
			w->nType = WALL_OPEN;
			if (IS_WALL (cwall_num))
				gameData.walls.walls[cwall_num].nType = WALL_OPEN;
			return;
		}
		gameData.walls.nCloaking++;
	}
	else {
		Int3();		//unexpected wall state
		return;
	}

	w->state = WALL_DOOR_CLOAKING;
	if (IS_WALL (cwall_num))
		gameData.walls.walls[cwall_num].state = WALL_DOOR_CLOAKING;

	d->nFrontWall = WallNumP (seg, tSide);
	d->nBackWall = cwall_num;

	Assert(SEG_IDX (seg) != -1);

	//Assert(!IS_WALL (w->nLinkedWall));

	if (gameData.demo.nState != ND_STATE_PLAYBACK) {
		vmsVector cp;
		COMPUTE_SIDE_CENTER(&cp, seg, tSide);
		DigiLinkSoundToPos(SOUND_WALL_CLOAK_ON, SEG_IDX (seg), tSide, &cp, 0, F1_0);
	}

	for (i=0;i<4;i++) {
		d->front_ls[i] = seg->sides[tSide].uvls[i].l;
		if (IS_WALL (cwall_num))
			d->back_ls[i] = csegp->sides[Connectside].uvls[i].l;
	}
}

//-----------------------------------------------------------------
// start the transition from open -> closed wall
void StartWallDecloak (tSegment *seg, short tSide)
{
	wall *w;
	cloaking_wall *d;
	short Connectside;
	tSegment *csegp;
	int i;
	short cwall_num;

	if (gameData.demo.nState==ND_STATE_PLAYBACK) return;

	Assert(IS_WALL (WallNumP (seg, tSide))); 	//Opening door on illegal wall

	w = gameData.walls.walls + WallNumP (seg, tSide);
	if (w->nType == WALL_CLOSED || w->state == WALL_DOOR_DECLOAKING)		//already closed or decloaking
		return;

	if (w->state == WALL_DOOR_CLOAKING) {	//cloaking, so reuse door
		int i;
		d = NULL;
		for (i=0;i<gameData.walls.nCloaking;i++) {		//find door
			d = gameData.walls.cloaking + i;
			if (d->nFrontWall==w-gameData.walls.walls || d->nBackWall==w-gameData.walls.walls)
				break;
		}
		Assert(i<gameData.walls.nCloaking);				//didn't find door!
		Assert(d!=NULL); // Get John!
		d->time = CLOAKING_WALL_TIME - d->time;
	}
	else if (w->state == WALL_DOOR_CLOSED) {	//create new door
		d = gameData.walls.cloaking + gameData.walls.nCloaking;
		d->time = 0;
		if (gameData.walls.nCloaking >= MAX_CLOAKING_WALLS) {		//no more!
			Int3();		//ran out of cloaking wall slots
			/* what is this _doing_ here?
			w->nType = WALL_CLOSED;
			gameData.walls.walls[WallNumP (csegp, Connectside)].nType = WALL_CLOSED;
			*/
			return;
		}
		gameData.walls.nCloaking++;
	}
	else {
		Int3();		//unexpected wall state
		return;
	}

	w->state = WALL_DOOR_DECLOAKING;
	// So that door can't be shot while opening
	csegp = gameData.segs.segments + seg->children[tSide];
	Connectside = FindConnectedSide(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = WallNumP (csegp, Connectside);
	if (IS_WALL (cwall_num))
		gameData.walls.walls[cwall_num].state = WALL_DOOR_DECLOAKING;

	d->nFrontWall = WallNumP (seg, tSide);
	d->nBackWall = WallNumP (csegp, Connectside);
	Assert(SEG_IDX (seg) != -1);
	Assert(!IS_WALL (w->nLinkedWall));
	if (gameData.demo.nState != ND_STATE_PLAYBACK) {
		vmsVector cp;
		COMPUTE_SIDE_CENTER(&cp, seg, tSide);
		DigiLinkSoundToPos(SOUND_WALL_CLOAK_OFF, SEG_IDX (seg), tSide, &cp, 0, F1_0);
	}
	for (i=0;i<4;i++) {
		d->front_ls[i] = seg->sides[tSide].uvls[i].l;
		if (IS_WALL (cwall_num))
			d->back_ls[i] = csegp->sides[Connectside].uvls[i].l;
	}
}


//-----------------------------------------------------------------

void DeleteActiveDoor (int nDoor)
{
if (--gameData.walls.nOpenDoors > nDoor)
	memcpy (gameData.walls.activeDoors + nDoor, 
			  gameData.walls.activeDoors + nDoor + 1, 
			  (gameData.walls.nOpenDoors - nDoor) * sizeof (gameData.walls.activeDoors [0]));
}

//-----------------------------------------------------------------
// This function closes the specified door and restores the closed
//  door texture.  This is called when the animation is done
void WallCloseDoorNum (int nDoor)
{
	int p;
	active_door *d;
	short cwall_num;

d = gameData.walls.activeDoors + nDoor;
for (p = 0; p < d->n_parts; p++) {
	wall *w;
	short Connectside, tSide;
	tSegment *csegp, *seg;
	w = gameData.walls.walls + d->nFrontWall[p];
	seg = gameData.segs.segments + w->nSegment;
	tSide = w->nSide;
	Assert(IS_WALL (WallNumP (seg, tSide)));		//Closing door on illegal wall
	csegp = gameData.segs.segments + seg->children[tSide];
	Connectside = FindConnectedSide(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = WallNumP (csegp, Connectside);
	gameData.walls.walls [WallNumP (seg, tSide)].state = WALL_DOOR_CLOSED;
	if (IS_WALL (cwall_num)) {
		gameData.walls.walls [cwall_num].state = WALL_DOOR_CLOSED;
		WallSetTMapNum (csegp, Connectside, NULL, -1, 
							 gameData.walls.walls [WallNumP (csegp, Connectside)].nClip, 0);
		}
	WallSetTMapNum (seg, tSide, NULL, -1, w->nClip, 0);
	}
DeleteActiveDoor (nDoor);
}

//-----------------------------------------------------------------
int CheckPoke (int nObject,int nSegment,short tSide)
{
	tObject *objP = &gameData.objs.objects[nObject];

	//note: don't let gameData.objs.objects with zero size block door

	if (objP->size && GetSegMasks(&objP->pos,nSegment,objP->size).sideMask & (1<<tSide))
		return 1;		//pokes through tSide!
	else
		return 0;		//does not!

}

//-----------------------------------------------------------------
//returns true of door in unobjstructed (& thus can close)
int IsDoorFree (tSegment *seg,short tSide)
{
	short Connectside;
	tSegment *csegp;
	short nObject;

	csegp = gameData.segs.segments + seg->children[tSide];
	Connectside = FindConnectedSide(seg, csegp);
	Assert(Connectside != -1);

	//go through each tObject in each of two segments, and see if
	//it pokes into the connecting seg

	for (nObject=seg->objects;nObject!=-1;nObject=gameData.objs.objects[nObject].next)
		if (gameData.objs.objects[nObject].nType!=OBJ_WEAPON && 
			 gameData.objs.objects[nObject].nType!=OBJ_FIREBALL && 
			CheckPoke(nObject,SEG_IDX (seg),tSide))
			return 0;	//not d_free

	for (nObject=csegp->objects;nObject!=-1;nObject=gameData.objs.objects[nObject].next)
		if (gameData.objs.objects[nObject].nType!=OBJ_WEAPON && 
			 gameData.objs.objects[nObject].nType!=OBJ_FIREBALL && 
			CheckPoke(nObject,(short) (SEG_IDX (csegp)),Connectside))
			return 0;	//not d_free

	return 1; 	//doorway is d_free!
}



//-----------------------------------------------------------------
// Closes a door
void WallCloseDoor(tSegment *seg, short tSide)
{
	wall *w;
	active_door *d;
	short Connectside, nWall, cwall_num;
	tSegment *csegp;

	Assert(IS_WALL (WallNumP (seg, tSide))); 	//Opening door on illegal wall

	w = &gameData.walls.walls[WallNumP (seg, tSide)];
	nWall = WALL_IDX (w);
	if ((w->state == WALL_DOOR_CLOSING) ||		//already closing
		 (w->state == WALL_DOOR_WAITING)	||		//open, waiting to close
		 (w->state == WALL_DOOR_CLOSED))			//closed
		return;

	if (!IsDoorFree(seg,tSide))
		return;

	if (w->state == WALL_DOOR_OPENING) {	//reuse door

		int i;

		d = gameData.walls.activeDoors;
		for (i=0;i<gameData.walls.nOpenDoors;i++, d++) {		//find door
			if (d->nFrontWall[0]==nWall || d->nBackWall[0]==nWall ||
				 (d->n_parts==2 && (d->nFrontWall[1]==nWall || d->nBackWall[1]==nWall)))
				break;
		}
		if (i >= gameData.walls.nOpenDoors)	//no matching open door found
			return;
		Assert(d!=NULL); // Get John!

		d->time = gameData.walls.pAnims [w->nClip].xTotalTime - d->time;

		if (d->time < 0)
			d->time = 0;

	}
	else {											//create new door
		Assert(w->state == WALL_DOOR_OPEN);
		d = gameData.walls.activeDoors + gameData.walls.nOpenDoors;
		d->time = 0;
		gameData.walls.nOpenDoors++;
		Assert(gameData.walls.nOpenDoors < MAX_DOORS);
	}

	w->state = WALL_DOOR_CLOSING;

	// So that door can't be shot while opening
	csegp = gameData.segs.segments + seg->children[tSide];
	Connectside = FindConnectedSide(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = WallNumP (csegp, Connectside);
	if (IS_WALL (cwall_num))
		gameData.walls.walls[cwall_num].state = WALL_DOOR_CLOSING;

	d->nFrontWall[0] = WallNumP (seg, tSide);
	d->nBackWall[0] = cwall_num;

	Assert(SEG_IDX (seg) != -1);

	if (gameData.demo.nState == ND_STATE_RECORDING) {
		NDRecordDoorOpening(SEG_IDX (seg), tSide);
	}

	if (IS_WALL (w->nLinkedWall)) {
		Int3();		//don't think we ever used linked walls
	}
	else
		d->n_parts = 1;


	if (gameData.demo.nState != ND_STATE_PLAYBACK)
	{
		// NOTE THE LINK TO ABOVE!!!!
		vmsVector cp;
		COMPUTE_SIDE_CENTER(&cp, seg, tSide);
		if (gameData.walls.pAnims[w->nClip].openSound > -1)
			DigiLinkSoundToPos(gameData.walls.pAnims[w->nClip].openSound, SEG_IDX (seg), tSide, &cp, 0, F1_0);

	}
}

//-----------------------------------------------------------------

int AnimateOpeningDoor (tSegment *segP, short nSide, fix xElapsedTime)
{
	wall	*wallP;
	int	i, nFrames, nWall, bFlags = 0;
	fix	xTotalTime, xFrameTime;

if (!segP || (nSide < 0))
	return 3;
nWall = WallNumP (segP, nSide);
Assert(IS_WALL (nWall));		//Trying to DoDoorOpen on illegal wall
wallP = gameData.walls.walls + nWall;
nFrames = AnimFrameCount (gameData.walls.pAnims + wallP->nClip);
if (!nFrames)
	return 3;
xTotalTime = gameData.walls.pAnims [wallP->nClip].xTotalTime;
xFrameTime = xTotalTime / nFrames;
i = xElapsedTime / xFrameTime;
if (i < nFrames)
	WallSetTMapNum (segP, nSide, NULL, -1, wallP->nClip, i);
if (i > nFrames / 2)
	wallP->flags |= WALL_DOOR_OPENED;
if (i >= nFrames - 1) {
	WallSetTMapNum (segP, nSide, NULL, -1, wallP->nClip, nFrames - 1);
	// If our door is not automatic just remove it from the list.
	if (!(wallP->flags & WALL_DOOR_AUTO)) {
		wallP->state = WALL_DOOR_OPEN;
		return 1;
		}
	else {
		wallP->state = WALL_DOOR_WAITING;
		return 2;
		}
	}
return 0;
}

//-----------------------------------------------------------------
// Animates opening of a door.
// Called in the game loop.
void DoDoorOpen(int nDoor)
{
	int			p, bFlags = 3;
	active_door *d;

	Assert(nDoor != -1);		//Trying to DoDoorOpen on illegal door

d = gameData.walls.activeDoors + nDoor;
for (p = 0; p < d->n_parts; p++) {
	wall		*w;
	short		cside, tSide;
	tSegment	*csegp, *seg;
	int		wn;

	d->time += gameData.time.xFrame;
	w = gameData.walls.walls + d->nFrontWall [p];
	KillStuckObjects (d->nFrontWall [p]);
	KillStuckObjects (d->nBackWall [p]);

	seg = gameData.segs.segments + w->nSegment;
	tSide = w->nSide;
	wn = WallNumP (seg, tSide);
	Assert(IS_WALL (wn));		//Trying to DoDoorOpen on illegal wall
	bFlags &= AnimateOpeningDoor (seg, tSide, d->time);
	csegp = gameData.segs.segments + seg->children[tSide];
	cside = FindConnectedSide(seg, csegp);
	Assert (cside != -1);
	if (IS_WALL (WallNumP (csegp, cside)))
		bFlags &= AnimateOpeningDoor (csegp, cside, d->time);
	}
if (bFlags & 2)
	gameData.walls.activeDoors [gameData.walls.nOpenDoors].time = 0;	//counts up
if (bFlags & 1)
	DeleteActiveDoor (nDoor);
}

//-----------------------------------------------------------------

int AnimateClosingDoor (tSegment *segP, short nSide, fix xElapsedTime)
{
	wall	*wallP;
	int	i, nFrames, nWall, bFlags = 0;
	fix	xTotalTime, xFrameTime;

if (!segP || (nSide < 0))
	return 3;
nWall = WallNumP (segP, nSide);
Assert(IS_WALL (nWall));		//Trying to DoDoorOpen on illegal wall
wallP = gameData.walls.walls + nWall;
nFrames = AnimFrameCount (gameData.walls.pAnims + wallP->nClip);
if (!nFrames)
	return 0;
xTotalTime = gameData.walls.pAnims [wallP->nClip].xTotalTime;
xFrameTime = xTotalTime / nFrames;
i = nFrames - xElapsedTime / xFrameTime - 1;
if (i < nFrames / 2)
	wallP->flags &= ~WALL_DOOR_OPENED;
if (i <= 0)
	return 1;
WallSetTMapNum (segP, nSide, NULL, -1, wallP->nClip, i);
wallP->state = WALL_DOOR_CLOSING;
return 0;
}

//-----------------------------------------------------------------
// Animates and processes the closing of a door.
// Called from the game loop.
void DoDoorClose (int nDoor)
{
	active_door *d;
	wall			*w;
	int			p, bFlags = 1;

Assert(nDoor != -1);		//Trying to DoDoorOpen on illegal door
d = gameData.walls.activeDoors + nDoor;
w = gameData.walls.walls + d->nFrontWall[0];

	//check for gameData.objs.objects in doorway before closing
if (w->flags & WALL_DOOR_AUTO)
	if (!IsDoorFree(gameData.segs.segments + w->nSegment,(short) w->nSide)) {
		DigiKillSoundLinkedToSegment((short) w->nSegment,(short) w->nSide,-1);
		WallOpenDoor(&gameData.segs.segments[w->nSegment],(short) w->nSide);		//re-open door
		return;
		}

for (p = 0; p < d->n_parts; p++) {
	short		cside, tSide;
	tSegment	*csegp, *seg;

	w = gameData.walls.walls + d->nFrontWall [p];
	seg = gameData.segs.segments + w->nSegment;
	tSide = w->nSide;
	if (!IS_WALL (WallNumP (seg, tSide))) {
#if TRACE
		con_printf (CON_DEBUG, "Trying to DoDoorClose on Illegal wall\n");
#endif
		return;
		}

		//if here, must be auto door
//		Assert(gameData.walls.walls[WallNumP (seg, tSide)].flags & WALL_DOOR_AUTO);
//don't assert here, because now we have triggers to close non-auto doors

		// Otherwise, close it.
	csegp = gameData.segs.segments + seg->children[tSide];
	cside = FindConnectedSide(seg, csegp);
	Assert(cside != -1);

	if (gameData.demo.nState != ND_STATE_PLAYBACK)
		// NOTE THE LINK TO ABOVE!!
		if (p==0)	//only play one sound for linked doors
			if (d->time==0)	{		//first time
				vmsVector cp;
				COMPUTE_SIDE_CENTER(&cp, seg, tSide);
				if (gameData.walls.pAnims[w->nClip].closeSound  > -1)
					DigiLinkSoundToPos((short) gameData.walls.pAnims[gameData.walls.walls[WallNumP (seg, tSide)].nClip].closeSound, SEG_IDX (seg), tSide, &cp, 0, F1_0);
				}
	d->time += gameData.time.xFrame;
	bFlags &= AnimateClosingDoor (seg, tSide, d->time);
	if (IS_WALL (WallNumP (csegp, cside)))
		bFlags &= AnimateClosingDoor (csegp, cside, d->time);
	}
if (bFlags & 1)
	WallCloseDoorNum (nDoor);
else
	gameData.walls.activeDoors [gameData.walls.nOpenDoors].time = 0;		//counts up	
}


//-----------------------------------------------------------------
// Turns off an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void WallIllusionOff(tSegment *seg, short tSide)
{
	tSegment *csegp;
	short cside;

	if (seg->children[tSide] < 0) {
		if (gameOpts->legacy.bWalls)
			Warning (TXT_ILLUS_SINGLE,
				gameData.segs.segments - seg, tSide);
		csegp = NULL;
		cside = -1;
		}
	else {
		csegp = gameData.segs.segments+seg->children[tSide];
		cside = FindConnectedSide(seg, csegp);
		Assert(cside != -1);
		}

	if (!IS_WALL (WallNumP (seg, tSide))) {
#if TRACE
		con_printf (CON_DEBUG, "Trying to shut off illusion illegal wall\n");
#endif
		return;
	}

	gameData.walls.walls[WallNumP (seg, tSide)].flags |= WALL_ILLUSION_OFF;
	if (csegp && (0 >= WallNumP (csegp, cside)))
		gameData.walls.walls[WallNumP (csegp, cside)].flags |= WALL_ILLUSION_OFF;

	KillStuckObjects(WallNumP (seg, tSide));
	if (csegp && (0 >= WallNumP (csegp, cside)))
		KillStuckObjects(WallNumP (csegp, cside));
}

//-----------------------------------------------------------------
// Turns on an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void WallIllusionOn(tSegment *seg, short tSide)
{
	tSegment *csegp;
	short cside;

	if (seg->children[tSide] < 0) {
		if (gameOpts->legacy.bWalls)
			Warning (TXT_ILLUS_SINGLE,
				gameData.segs.segments - seg, tSide);
		csegp = NULL;
		cside = -1;
		}
	else {
		csegp = gameData.segs.segments+seg->children[tSide];
		cside = FindConnectedSide(seg, csegp);
		Assert(cside != -1);
		}

	if (!IS_WALL (WallNumP (seg, tSide))) {
#if TRACE
		con_printf (CON_DEBUG, "Trying to turn on illusion illegal wall\n");
#endif
		return;
	}

	gameData.walls.walls[WallNumP (seg, tSide)].flags &= ~WALL_ILLUSION_OFF;
	if (csegp && (0 >= WallNumP (csegp, cside)))
		gameData.walls.walls[WallNumP (csegp, cside)].flags &= ~WALL_ILLUSION_OFF;
}

//	-----------------------------------------------------------------------------
//	Allowed to open the normally locked special boss door if in multiplayer mode.
int special_boss_opening_allowed(int nSegment, short nSide)
{
if (gameData.app.nGameMode & GM_MULTI)
	return (gameData.missions.nCurrentLevel == BOSS_LOCKED_DOOR_LEVEL) && 
			 (nSegment == BOSS_LOCKED_DOOR_SEG) && 
			 (nSide == BOSS_LOCKED_DOOR_SIDE);
return 0;
}

//-----------------------------------------------------------------

void UnlockAllWalls (int bOnlyDoors)
{
	int	i;
	wall	*w;

for (i = 0, w = gameData.walls.walls; i < gameData.walls.nWalls; w++, i++) {
	if (w->nType == WALL_DOOR) {
		w->flags &= ~WALL_DOOR_LOCKED;
		w->keys = 0;
		}
	else if (!bOnlyDoors 
#ifdef RELEASE
				&& (w->nType == WALL_CLOSED)
#endif
			 )
		w->nType = WALL_OPEN;
	}
}

//-----------------------------------------------------------------
// set up renderer for alternative highres animation method
// (all frames are stored in one texture, and struct tSide.nFrame
// holds the frame index).

void InitDoorAnims (void)
{
	int		h, i;
	wall		*w;
	tSegment	*segP;
	wclip		*animP;

for (i = 0, w = gameData.walls.walls; i < gameData.walls.nWalls; w++, i++) {
	if (w->nType == WALL_DOOR) {
		animP = gameData.walls.pAnims + w->nClip;
		if (!(animP->flags & WCF_ALTFMT))
			continue;
		h = (animP->flags & WCF_TMAP1) ? -1 : 1;
		segP = gameData.segs.segments + w->nSegment;
		segP->sides [w->nSide].nFrame = h;
/*
		csegP = gameData.segs.segments + segP->children [w->nSide];
		csidenum = FindConnectedSide (segP, csegP);
		if (csidenum >= 0)
			csegP->sides [csidenum].nFrame = h;
*/
		//WallSetTMapNum (segP, w->nSide, csegP, cside, w->nClip, 0);
		}
	}
}

//-----------------------------------------------------------------
// Determines what happens when a wall is shot
//returns info about wall.  see wall.h for codes
//obj is the tObject that hit...either a weapon or the player himself
//playernum is the number the player who hit the wall or fired the weapon,
//or -1 if a robot fired the weapon
int WallHitProcess (tSegment *seg, short tSide, fix damage, int playernum, tObject *objP)
{
	wall	*w;
	short	nWall;
	fix	show_message;

Assert (SEG_IDX (seg) != -1);

// If it is not a "wall" then just return.
nWall = WallNumP (seg, tSide);
if (!IS_WALL (nWall))
	return WHP_NOT_SPECIAL;

w = gameData.walls.walls + nWall;

if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordWallHitProcess(SEG_IDX (seg), tSide, damage, playernum);

if (w->nType == WALL_BLASTABLE) {
	if (objP->cType.laserInfo.parentType == OBJ_PLAYER)
		WallDamage(seg, tSide, damage);
	return WHP_BLASTABLE;
	}

if (playernum != gameData.multi.nLocalPlayer)	//return if was robot fire
	return WHP_NOT_SPECIAL;

Assert(playernum > -1);

//	Determine whether player is moving forward.  If not, don't say negative
//	messages because he probably didn't intentionally hit the door.
if (objP->nType == OBJ_PLAYER)
	show_message = (VmVecDot(&objP->orient.fvec, &objP->mType.physInfo.velocity) > 0);
else if (objP->nType == OBJ_ROBOT)
	show_message = 0;
else if ((objP->nType == OBJ_WEAPON) && (objP->cType.laserInfo.parentType == OBJ_ROBOT))
	show_message = 0;
else
	show_message = 1;

if (w->keys == KEY_BLUE) {
	if (!(gameData.multi.players[playernum].flags & PLAYER_FLAGS_BLUE_KEY)) {
		if (playernum==gameData.multi.nLocalPlayer)
			if (show_message)
				HUDInitMessage("%s %s",TXT_BLUE,TXT_ACCESS_DENIED);
		return WHP_NO_KEY;
		}
	}
else if (w->keys == KEY_RED) {
	if (!(gameData.multi.players[playernum].flags & PLAYER_FLAGS_RED_KEY)) {
		if (playernum==gameData.multi.nLocalPlayer)
			if (show_message)
				HUDInitMessage("%s %s",TXT_RED,TXT_ACCESS_DENIED);
		return WHP_NO_KEY;
		}
	}
else if (w->keys == KEY_GOLD) {
	if (!(gameData.multi.players[playernum].flags & PLAYER_FLAGS_GOLD_KEY)) {
		if (playernum==gameData.multi.nLocalPlayer)
			if (show_message)
				HUDInitMessage("%s %s",TXT_YELLOW,TXT_ACCESS_DENIED);
		return WHP_NO_KEY;
		}
	}
if (w->nType == WALL_DOOR) {
	if ((w->flags & WALL_DOOR_LOCKED) && !(special_boss_opening_allowed (SEG_IDX (seg), tSide))) {
		if (playernum == gameData.multi.nLocalPlayer)
			if (show_message)
				HUDInitMessage(TXT_CANT_OPEN_DOOR);
		return WHP_NO_KEY;
		}
	else {
		if (w->state != WALL_DOOR_OPENING) {
			WallOpenDoor(seg, tSide);
		#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendDoorOpen(SEG_IDX (seg), tSide,w->flags);
		#endif
			}
		return WHP_DOOR;

		}
	}
return WHP_NOT_SPECIAL;		//default is treat like normal wall
}

//-----------------------------------------------------------------
// Opens doors/destroys wall/shuts off triggers.
void WallToggle(tSegment *seg, short tSide)
{
	int nWall;

	if (SEG_IDX (seg) > gameData.segs.nLastSegment)
	{
#ifdef _DEBUG
		Warning("Can't toggle tSide %d of tSegment %d - nonexistent tSegment!\n", tSide, SEG_IDX (seg));
#endif
		return;
	}
	Assert((ushort) tSide < MAX_SIDES_PER_SEGMENT);

	nWall = WallNumP (seg, tSide);

	if (!IS_WALL (nWall)) {
#if TRACE
	 	con_printf (CON_DEBUG, "Illegal WallToggle\n");
#endif
		return;
	}

	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordWallToggle(SEG_IDX (seg), tSide);

	if (gameData.walls.walls[nWall].nType == WALL_BLASTABLE)
		WallDestroy(seg, tSide);

	if ((gameData.walls.walls[nWall].nType == WALL_DOOR) && 
		 (gameData.walls.walls[nWall].state == WALL_DOOR_CLOSED))
		WallOpenDoor(seg, tSide);

}


//-----------------------------------------------------------------
// Tidy up gameData.walls.walls array for load/save purposes.
void ResetWalls()
{
	int i;
	wall *pWall = gameData.walls.walls+gameData.walls.nWalls;

	if (gameData.walls.nWalls < 0) {
#if TRACE
		con_printf (CON_DEBUG, "Illegal gameData.walls.nWalls\n");
#endif
		return;
	}

	for (i=gameData.walls.nWalls;i<MAX_WALLS;i++, pWall++) {
		pWall->nType = WALL_NORMAL;
		pWall->flags = 0;
		pWall->hps = 0;
		pWall->nTrigger = -1;
		pWall->nClip = -1;
		}
}

//-----------------------------------------------------------------

void DoCloakingWallFrame(int cloaking_wall_num)
{
	cloaking_wall *d;
	wall *wfront,*wback;

	if (gameData.demo.nState==ND_STATE_PLAYBACK) 
		return;

	d = gameData.walls.cloaking + cloaking_wall_num;
	wfront = gameData.walls.walls + d->nFrontWall;
	wback = IS_WALL (d->nBackWall) ? gameData.walls.walls + d->nBackWall : NULL;

	d->time += gameData.time.xFrame;

	if (d->time > CLOAKING_WALL_TIME) {
		wfront->nType = WALL_OPEN;
		wfront->state = WALL_DOOR_CLOSED;		//why closed? why not?
		if (wback) {
			wback->nType = WALL_OPEN;
			wback->state = WALL_DOOR_CLOSED;		//why closed? why not?
			}
#if 1
		if (cloaking_wall_num < --gameData.walls.nCloaking)
			memcpy (gameData.walls.cloaking + cloaking_wall_num,
					  gameData.walls.cloaking + cloaking_wall_num + 1,
					  (gameData.walls.nCloaking - cloaking_wall_num) * sizeof (*gameData.walls.cloaking));
#else
		for (i=cloaking_wall_num;i<gameData.walls.nCloaking;i++)
			gameData.walls.cloaking[i] = gameData.walls.cloaking[i+1];
		gameData.walls.nCloaking--;
#endif
	}
	else if (d->time > CLOAKING_WALL_TIME/2) {
		int oldType=wfront->nType;

		wfront->cloakValue = ((d->time - CLOAKING_WALL_TIME/2) * (GR_ACTUAL_FADE_LEVELS-2)) / (CLOAKING_WALL_TIME/2);
		if (wback)
			wback->cloakValue = wfront->cloakValue;

		if (oldType != WALL_CLOAKED) {		//just switched
			int i;

			wfront->nType = WALL_CLOAKED;
			if (wback)
				wback->nType = WALL_CLOAKED;

			for (i=0;i<4;i++) {
				gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[i].l = d->front_ls[i];
				if (wback)
					gameData.segs.segments[wback->nSegment].sides[wback->nSide].uvls[i].l = d->back_ls[i];
			}
		}
	}
	else {		//fading out
		fix light_scale;
		int i;

		light_scale = FixDiv(CLOAKING_WALL_TIME/2-d->time,CLOAKING_WALL_TIME/2);

		for (i=0;i<4;i++) {
			gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[i].l = FixMul(d->front_ls[i],light_scale);
			if (wback)
				gameData.segs.segments[wback->nSegment].sides[wback->nSide].uvls[i].l = FixMul(d->back_ls[i],light_scale);
		}
	}

	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordCloakingWall(d->nFrontWall, d->nBackWall, wfront->nType, wfront->state, wfront->cloakValue, gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[0].l, gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[1].l, gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[2].l, gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[3].l);

}

//-----------------------------------------------------------------

void DoDecloakingWallFrame (int cloaking_wall_num)
{
	cloaking_wall *d;
	wall *wfront,*wback;

	if (gameData.demo.nState == ND_STATE_PLAYBACK) 
		return;

	d = gameData.walls.cloaking + cloaking_wall_num;
	wfront = gameData.walls.walls + d->nFrontWall;
	wback = IS_WALL (d->nBackWall) ? gameData.walls.walls + d->nBackWall : NULL;

	d->time += gameData.time.xFrame;

	if (d->time > CLOAKING_WALL_TIME) {
		int i;

		wfront->state = WALL_DOOR_CLOSED;
		if (wback)
			wback->state = WALL_DOOR_CLOSED;

		for (i=0;i<4;i++) {
			gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[i].l = d->front_ls[i];
			if (wback)
				gameData.segs.segments[wback->nSegment].sides[wback->nSide].uvls[i].l = d->back_ls[i];
		}

		for (i=cloaking_wall_num;i<gameData.walls.nCloaking;i++)
			gameData.walls.cloaking[i] = gameData.walls.cloaking[i+1];
		gameData.walls.nCloaking--;

	}
	else if (d->time > CLOAKING_WALL_TIME/2) {		//fading in
		fix light_scale;
		int i;

		wfront->nType = WALL_CLOSED;
		if (wback)
			wback->nType = WALL_CLOSED;

		light_scale = FixDiv(d->time-CLOAKING_WALL_TIME/2,CLOAKING_WALL_TIME/2);

		for (i=0;i<4;i++) {
			gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[i].l = FixMul(d->front_ls[i],light_scale);
			if (wback)
				gameData.segs.segments[wback->nSegment].sides[wback->nSide].uvls[i].l = FixMul(d->back_ls[i],light_scale);
		}
	}
	else {		//cloaking in
		wfront->cloakValue = ((CLOAKING_WALL_TIME/2 - d->time) * (GR_ACTUAL_FADE_LEVELS-2)) / (CLOAKING_WALL_TIME/2);
		wfront->nType = WALL_CLOAKED;
		if (wback) {
			wback->cloakValue = wfront->cloakValue;
			wback->nType = WALL_CLOAKED;
			}
	}

	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordCloakingWall(d->nFrontWall, d->nBackWall, wfront->nType, wfront->state, wfront->cloakValue, gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[0].l, gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[1].l, gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[2].l, gameData.segs.segments[wfront->nSegment].sides[wfront->nSide].uvls[3].l);

}

//-----------------------------------------------------------------

void WallFrameProcess ()
{
	int i;
	cloaking_wall *cw;
	active_door *d = gameData.walls.activeDoors;

for (i = 0;i < gameData.walls.nOpenDoors; i++, d++) {
	wall	*wb = NULL,
			*w = gameData.walls.walls + d->nFrontWall [0];

	if (w->state == WALL_DOOR_OPENING)
		DoDoorOpen (i);
	else if (w->state == WALL_DOOR_CLOSING)
		DoDoorClose (i);
	else if (w->state == WALL_DOOR_WAITING) {
		d->time += gameData.time.xFrame;

		//set flags to fix occatsional netgame problem where door is
		//waiting to close but open flag isn't set
//			Assert(d->n_parts == 1);
		if (IS_WALL (d->nBackWall [0]))
			wb = gameData.walls.walls + d->nBackWall [0];
		if ((d->time > DOOR_WAIT_TIME) && 
			 IsDoorFree (gameData.segs.segments + w->nSegment, (short) w->nSide)) {
			w->state = WALL_DOOR_CLOSING;
			if (wb)
				wb->state = WALL_DOOR_CLOSING;
			d->time = 0;
			}
		else {
			w->flags |= WALL_DOOR_OPENED;
			if (wb)
				wb->flags |= WALL_DOOR_OPENED;
			}
		}
	else if (w->state == WALL_DOOR_CLOSED || w->state == WALL_DOOR_OPEN) {
		//this shouldn't happen.  if the wall is in one of these states,
		//there shouldn't be an activedoor entry for it.  So we'll kill
		//the activedoor entry.  Tres simple.
		int t;
		Int3();		//a bad thing has happened, but I'll try to fix it up
		for (t = i; t < gameData.walls.nOpenDoors; t++)
			gameData.walls.activeDoors[t] = gameData.walls.activeDoors[t+1];
		gameData.walls.nOpenDoors--;
		}
	}

cw = gameData.walls.cloaking;
for (i = 0; i < gameData.walls.nCloaking; i++, cw++) {
	ubyte s = gameData.walls.walls [cw->nFrontWall].state;
	if (s == WALL_DOOR_CLOAKING)
		DoCloakingWallFrame (i);
	else if (s == WALL_DOOR_DECLOAKING)
		DoDecloakingWallFrame (i);
#ifdef _DEBUG
	else
		Int3();	//unexpected wall state
#endif
	}
}

int	Num_stuckObjects=0;

stuckobj	StuckObjects[MAX_STUCK_OBJECTS];

//-----------------------------------------------------------------
//	An tObject got stuck in a door (like a flare).
//	Add global entry.
void AddStuckObject(tObject *objP, short nSegment, short nSide)
{
	int	i;
	short	wallnum;
	stuckobj	*sto;

	wallnum = WallNumI (nSegment, nSide);

	if (IS_WALL (wallnum)) {
		if (gameData.walls.walls[wallnum].flags & WALL_BLASTED)
			objP->flags |= OF_SHOULD_BE_DEAD;

		for (i=0, sto = StuckObjects; i<MAX_STUCK_OBJECTS; i++, sto++) {
			if (sto->wallnum == -1) {
				sto->wallnum = wallnum;
				sto->nObject = OBJ_IDX (objP);
				sto->nSignature = objP->nSignature;
				Num_stuckObjects++;
				break;
			}
		}
#if TRACE
		if (i == MAX_STUCK_OBJECTS)
			con_printf (1, 
				"Warning: Unable to add tObject %i which got stuck in wall %i to StuckObjects\n", 
				OBJ_IDX (objP), wallnum);
#endif
	}



}

//	--------------------------------------------------------------------------------------------------
//	Look at the list of stuck gameData.objs.objects, clean up in case an tObject has gone away, but not been removed here.
//	Removes up to one/frame.
void RemoveObsoleteStuckObjects(void)
{
	int	nObject;

	//	Safety and efficiency code.  If no stuck gameData.objs.objects, should never get inside the IF, but this is faster.
	if (!Num_stuckObjects)
		return;

	nObject = gameData.app.nFrameCount % MAX_STUCK_OBJECTS;

	if (StuckObjects[nObject].wallnum != -1)
		if ((gameData.walls.walls[StuckObjects[nObject].wallnum].state != WALL_DOOR_CLOSED) || (gameData.objs.objects[StuckObjects[nObject].nObject].nSignature != StuckObjects[nObject].nSignature)) {
			Num_stuckObjects--;
			gameData.objs.objects[StuckObjects[nObject].nObject].lifeleft = F1_0/8;
			StuckObjects[nObject].wallnum = -1;
		}

}

//-----------------------------------------------------------------

extern void FlushFCDCache(void);

//	----------------------------------------------------------------------------------------------------
//	Door with wall index wallnum is opening, kill all gameData.objs.objects stuck in it.
void KillStuckObjects(int wallnum)
{
	int	i;

	if (!IS_WALL (wallnum) || Num_stuckObjects == 0) {
		return;
	}

	Num_stuckObjects=0;

	for (i=0; i<MAX_STUCK_OBJECTS; i++)
		if (StuckObjects[i].wallnum == wallnum) {
			if (gameData.objs.objects[StuckObjects[i].nObject].nType == OBJ_WEAPON) {
				gameData.objs.objects[StuckObjects[i].nObject].lifeleft = F1_0/8;
			} else
#if TRACE
				con_printf (1, 
					"Warning: Stuck tObject of nType %i, expected to be of nType %i, see wall.c\n", 
					gameData.objs.objects[StuckObjects[i].nObject].nType, OBJ_WEAPON);
#endif
				// Int3();	//	What?  This looks bad.  Object is not a weapon and it is stuck in a wall!
			StuckObjects[i].wallnum = -1;
		} else if (StuckObjects[i].wallnum != -1) {
			Num_stuckObjects++;
		}
	//	Ok, this is awful, but we need to do things whenever a door opens/closes/disappears, etc.
	FlushFCDCache();

}


// -- unused -- // -----------------------------------------------------------------------------------
// -- unused -- //	Return tObject id of first flare found embedded in segp:nSide.
// -- unused -- //	If no flare, return -1.
// -- unused -- int contains_flare(tSegment *segp, short nSide)
// -- unused -- {
// -- unused -- 	int	i;
// -- unused --
// -- unused -- 	for (i=0; i<Num_stuckObjects; i++) {
// -- unused -- 		tObject	*objP = &gameData.objs.objects[StuckObjects[i].nObject];
// -- unused --
// -- unused -- 		if ((objP->nType == OBJ_WEAPON) && (objP->id == FLARE_ID)) {
// -- unused -- 			if (gameData.walls.walls[StuckObjects[i].wallnum].nSegment == SEG_IDX (segp))
// -- unused -- 				if (gameData.walls.walls[StuckObjects[i].wallnum].nSide == nSide)
// -- unused -- 					return OBJ_IDX (objP);
// -- unused -- 		}
// -- unused -- 	}
// -- unused --
// -- unused -- 	return -1;
// -- unused -- }

// -----------------------------------------------------------------------------------
// Initialize stuck gameData.objs.objects array.  Called at start of level
void InitStuckObjects(void)
{
	int	i;

	for (i=0; i<MAX_STUCK_OBJECTS; i++)
		StuckObjects[i].wallnum = -1;

	Num_stuckObjects = 0;
}

// -----------------------------------------------------------------------------------
// Clear out all stuck gameData.objs.objects.  Called for a new ship
void ClearStuckObjects(void)
{
	int	i;

	for (i=0; i<MAX_STUCK_OBJECTS; i++) {
		if (StuckObjects[i].wallnum != -1) {
			int	nObject;

			nObject = StuckObjects[i].nObject;

			if ((gameData.objs.objects[nObject].nType == OBJ_WEAPON) && (gameData.objs.objects[nObject].id == FLARE_ID))
				gameData.objs.objects[nObject].lifeleft = F1_0/8;

			StuckObjects[i].wallnum = -1;

			Num_stuckObjects--;
		}
	}

	Assert(Num_stuckObjects == 0);

}

// -----------------------------------------------------------------------------------
#define	MAX_BLAST_GLASS_DEPTH	5

void BngProcessSegment(tObject *objP, fix damage, tSegment *segp, int depth, sbyte *visited)
{
	int	i;
	short	nSide;

	if (depth > MAX_BLAST_GLASS_DEPTH)
		return;

	depth++;

	for (nSide=0; nSide<MAX_SIDES_PER_SEGMENT; nSide++) {
		int			tm;
		fix			dist;
		vmsVector	pnt;

		//	Process only walls which have glass.
		if (tm = segp->sides[nSide].nOvlTex) {
			int	ec, db;
			eclip *ecP;

			ec=gameData.pig.tex.pTMapInfo[tm].eclip_num;
			ecP = (ec < 0) ? NULL : gameData.eff.pEffects + ec;
			db = ecP ? ecP->dest_bm_num : -1;

			if (((ec != -1) && (db != -1) && !(ecP->flags & EF_ONE_SHOT)) ||	
			 	 ((ec == -1) && (gameData.pig.tex.pTMapInfo[tm].destroyed != -1))) {
				COMPUTE_SIDE_CENTER(&pnt, segp, nSide);
				dist = VmVecDistQuick(&pnt, &objP->pos);
				if (dist < damage/2) {
					dist = FindConnectedDistance(&pnt, SEG_IDX (segp), &objP->pos, objP->nSegment, MAX_BLAST_GLASS_DEPTH, WID_RENDPAST_FLAG);
					if ((dist > 0) && (dist < damage/2))
						CheckEffectBlowup(segp, nSide, &pnt, gameData.objs.objects + objP->cType.laserInfo.nParentObj, 1);
				}
			}
		}
	}

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		int	nSegment = segp->children[i];

		if (nSegment != -1) {
			if (!visited[nSegment]) {
				if (WALL_IS_DOORWAY(segp, (short) i, NULL) & WID_FLY_FLAG) {
					visited[nSegment] = 1;
					BngProcessSegment(objP, damage, &gameData.segs.segments[nSegment], depth, visited);
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------------
//	objP is going to detonate
//	blast nearby monitors, lights, maybe other things
void BlastNearbyGlass(tObject *objP, fix damage)
{
	int		i;
	sbyte   visited[MAX_SEGMENTS];
	tSegment	*cursegp;

	cursegp = &gameData.segs.segments[objP->nSegment];
	for (i=0; i<=gameData.segs.nLastSegment; i++)
		visited[i] = 0;

	visited[objP->nSegment] = 1;
	BngProcessSegment(objP, damage, cursegp, 0, visited);


}

#define MAX_CLIP_FRAMES_D1 20

/*
 * reads a wclip structure from a CFILE
 */
int wclip_read_n_d1(wclip *wc, int n, CFILE *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		wc[i].xTotalTime = CFReadFix(fp);
		wc[i].nFrameCount = CFReadShort(fp);
		for (j = 0; j < MAX_CLIP_FRAMES_D1; j++)
			wc[i].frames[j] = CFReadShort(fp);
		wc[i].openSound = CFReadShort(fp);
		wc[i].closeSound = CFReadShort(fp);
		wc[i].flags = CFReadShort(fp);
		CFRead(wc[i].filename, 13, 1, fp);
		wc[i].pad = CFReadByte(fp);
	}
	return i;
}

#ifndef FAST_FILE_IO
/*
 * reads a wclip structure from a CFILE
 */
int WClipReadN(wclip *wc, int n, CFILE *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		wc[i].xTotalTime = CFReadFix(fp);
		wc[i].nFrameCount = CFReadShort(fp);
		for (j = 0; j < MAX_CLIP_FRAMES; j++)
			wc[i].frames[j] = CFReadShort(fp);
		wc[i].openSound = CFReadShort(fp);
		wc[i].closeSound = CFReadShort(fp);
		wc[i].flags = CFReadShort(fp);
		CFRead(wc[i].filename, 13, 1, fp);
		wc[i].pad = CFReadByte(fp);
	}
	return i;
}

/*
 * reads a v16_wall structure from a CFILE
 */
extern void v16_wall_read(v16_wall *w, CFILE *fp)
{
	w->nType = CFReadByte(fp);
	w->flags = CFReadByte(fp);
	w->hps = CFReadFix(fp);
	w->nTrigger = (ubyte) CFReadByte(fp);
	w->nClip = CFReadByte(fp);
	w->keys = CFReadByte(fp);
}

/*
 * reads a v19_wall structure from a CFILE
 */
extern void v19_wall_read(v19_wall *w, CFILE *fp)
{
	w->nSegment = CFReadInt(fp);
	w->nSide = CFReadInt(fp);
	w->nType = CFReadByte(fp);
	w->flags = CFReadByte(fp);
	w->hps = CFReadFix(fp);
	w->nTrigger = (ubyte) CFReadByte(fp);
	w->nClip = CFReadByte(fp);
	w->keys = CFReadByte(fp);
	w->nLinkedWall = CFReadInt(fp);
}

/*
 * reads a wall structure from a CFILE
 */
extern void wall_read(wall *w, CFILE *fp)
{
	w->nSegment = CFReadInt(fp);
	w->nSide = CFReadInt(fp);
	w->hps = CFReadFix(fp);
	w->nLinkedWall = CFReadInt(fp);
	w->nType = CFReadByte(fp);
	w->flags = CFReadByte(fp);
	w->state = CFReadByte(fp);
	w->nTrigger = (ubyte) CFReadByte(fp);
	w->nClip = CFReadByte(fp);
	w->keys = CFReadByte(fp);
	w->controllingTrigger = CFReadByte(fp);
	w->cloakValue = CFReadByte(fp);
}

/*
 * reads a v19_door structure from a CFILE
 */
extern void v19_door_read(v19_door *d, CFILE *fp)
{
	d->n_parts = CFReadInt(fp);
	d->seg[0] = CFReadShort(fp);
	d->seg[1] = CFReadShort(fp);
	d->nSide[0] = CFReadShort(fp);
	d->nSide[1] = CFReadShort(fp);
	d->nType[0] = CFReadShort(fp);
	d->nType[1] = CFReadShort(fp);
	d->open = CFReadFix(fp);
}

/*
 * reads an active_door structure from a CFILE
 */
extern void active_door_read(active_door *ad, CFILE *fp)
{
	ad->n_parts = CFReadInt(fp);
	ad->nFrontWall[0] = CFReadShort(fp);
	ad->nFrontWall[1] = CFReadShort(fp);
	ad->nBackWall[0] = CFReadShort(fp);
	ad->nBackWall[1] = CFReadShort(fp);
	ad->time = CFReadFix(fp);
}
#endif
