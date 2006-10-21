/* $Id: switch.c,v 1.9 2003/10/04 03:14:48 btb Exp $ */
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
 * New gameData.trigs.triggers and Switches.
 *
 * Old Log:
 * Revision 1.2  1995/10/31  10:18:10  allender
 * shareware stuff
 *
 * Revision 1.1  1995/05/16  15:31:21  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/21  14:39:08  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.0  1995/02/27  11:28:41  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn'trigP require parsing BITMAPS.TBL.
 *
 * Revision 1.51  1995/01/31  15:26:23  rob
 * Don'trigP tTrigger matcens in anarchy games.
 *
 * Revision 1.50  1995/01/26  12:18:26  rob
 * Changed NetworkDoFrame call.
 *
 * Revision 1.49  1995/01/18  18:50:35  allender
 * don'trigP process triggers if in demo playback mode.  Fix for Rob to only do
 * MultiSendEndLevelStart if in multi player game
 *
 * Revision 1.48  1995/01/13  11:59:40  rob
 * Added palette fade after secret level exit.
 *
 * Revision 1.47  1995/01/12  17:00:41  rob
 * Fixed a problem with switches and secret levels.
 *
 * Revision 1.46  1995/01/12  13:35:11  rob
 * Added data flush after secret level exit.
 *
 * Revision 1.45  1995/01/03  15:25:11  rob
 * Fixed a compile error.
 *
 * Revision 1.44  1995/01/03  15:12:02  rob
 * Adding multiplayer switching.
 *
 * Revision 1.43  1994/11/29  16:52:12  yuan
 * Removed some obsolete commented out code.
 *
 * Revision 1.42  1994/11/27  23:15:07  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.41  1994/11/22  18:36:45  rob
 * Added new hook for endlevel for secret doors.
 *
 * Revision 1.40  1994/11/21  17:29:43  matt
 * Cleaned up sequencing & game saving for secret levels
 *
 * Revision 1.39  1994/11/19  15:20:32  mike
 * rip out unused code and data
 *
 * Revision 1.38  1994/10/25  16:09:52  yuan
 * Fixed byte bug.
 *
 * Revision 1.37  1994/10/24  16:05:28  matt
 * Removed clear of fuelcen_control_center_destroyed
 *
 * Revision 1.36  1994/10/08  14:21:13  matt
 * Added include
 *
 * Revision 1.35  1994/10/07  12:34:09  matt
 * Added code fot going to/from secret levels
 *
 * Revision 1.34  1994/10/05  15:16:10  rob
 * Used to be that only player #0 could tTrigger switches, now only the
 * LOCAL player can do it (and he's expected to tell the other guy with
 * a com message if its important!)
 *
 * Revision 1.33  1994/09/24  17:42:03  mike
 * Kill temporary version of function written by Yuan, replaced by MK.
 *
 * Revision 1.32  1994/09/24  17:10:00  yuan
 * Added Matcen triggers.
 *
 * Revision 1.31  1994/09/23  18:02:21  yuan
 * Completed wall checking.
 *
 * Revision 1.30  1994/08/19  20:09:41  matt
 * Added end-of-level cut scene with external scene
 *
 * Revision 1.29  1994/08/18  10:47:36  john
 * Cleaned up game sequencing and player death stuff
 * in preparation for making the player explode into
 * pieces when dead.
 *
 * Revision 1.28  1994/08/12  22:42:11  john
 * Took away Player_stats; added gameData.multi.players array.
 *
 * Revision 1.27  1994/07/02  13:50:44  matt
 * Cleaned up includes
 *
 * Revision 1.26  1994/06/27  16:32:25  yuan
 * Commented out incomplete code...
 *
 * Revision 1.25  1994/06/27  15:53:27  john
 * #define'd out the newdemo stuff
 *
 *
 * Revision 1.24  1994/06/27  15:10:04  yuan
 * Might mess up triggers.
 *
 * Revision 1.23  1994/06/24  17:01:43  john
 * Add VFX support; Took Game Sequencing, like EndGame and stuff and
 * took it out of game.c and into gameseq.c
 *
 * Revision 1.22  1994/06/16  16:20:15  john
 * Made player start out in physics mode; Neatend up game loop a bit.
 *
 * Revision 1.21  1994/06/15  14:57:22  john
 * Added triggers to demo recording.
 *
 * Revision 1.20  1994/06/10  17:44:25  mike
 * Assert on result of FindConnectedSide == -1
 *
 * Revision 1.19  1994/06/08  10:20:15  yuan
 * Removed unused testing.
 *
 *
 * Revision 1.18  1994/06/07  13:10:48  yuan
 * Fixed bug in check tTrigger... Still working on other bugs.
 *
 * Revision 1.17  1994/05/30  20:22:04  yuan
 * New triggers.
 *
 * Revision 1.16  1994/05/27  10:32:46  yuan
 * New dialog boxes (gameData.walls.walls and gameData.trigs.triggers) added.
 *
 *
 * Revision 1.15  1994/05/25  18:06:46  yuan
 * Making new dialog box controls for walls and triggers.
 *
 * Revision 1.14  1994/05/10  19:05:32  yuan
 * Made end of level flag rather than menu popping up
 *
 * Revision 1.13  1994/04/29  15:05:25  yuan
 * Added menu pop-up at exit tTrigger.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: switch.c,v 1.9 2003/10/04 03:14:48 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "gauges.h"
#include "newmenu.h"
#include "game.h"
#include "switch.h"
#include "segment.h"
#include "error.h"
#include "gameseg.h"
#include "mono.h"
#include "wall.h"
#include "texmap.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "player.h"
#include "endlevel.h"
#include "gameseq.h"
#include "multi.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "palette.h"
#include "robot.h"
#include "bm.h"
#include "timer.h"
#include "segment.h"
#include "kconfig.h"
#include "text.h"
#include "lighting.h"
#include "hudmsg.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#define MAX_ORIENT_STEPS	10

#define TT_OPEN_DOOR        0   // Open a door
#define TT_CLOSE_DOOR       1   // Close a door
#define TT_MATCEN           2   // Activate a matcen
#define TT_EXIT             3   // End the level
#define TT_SECRET_EXIT      4   // Go to secret level
#define TT_ILLUSION_OFF     5   // Turn an illusion off
#define TT_ILLUSION_ON      6   // Turn an illusion on
#define TT_UNLOCK_DOOR      7   // Unlock a door
#define TT_LOCK_DOOR        8   // Lock a door
#define TT_OPEN_WALL        9   // Makes a wall open
#define TT_CLOSE_WALL       10  // Makes a wall closed
#define TT_ILLUSORY_WALL    11  // Makes a wall illusory
#define TT_LIGHT_OFF        12  // Turn a light off
#define TT_LIGHT_ON         13  // Turn s light on
#define TT_TELEPORT			 14
#define TT_SPEEDBOOST		 15
#define TT_CAMERA				 16
#define TT_SHIELD_DAMAGE	 17
#define TT_ENERGY_DRAIN		 18
#define NUM_TRIGGER_TYPES   19

int oppTrigTypes [] = {
	TT_CLOSE_DOOR,
	TT_OPEN_DOOR,
	TT_MATCEN,
	TT_EXIT,
	TT_SECRET_EXIT,
	TT_ILLUSION_ON,
	TT_ILLUSION_OFF,
	TT_LOCK_DOOR,
	TT_UNLOCK_DOOR,
	TT_CLOSE_WALL,
	TT_OPEN_WALL,
	TT_ILLUSORY_WALL,
	TT_LIGHT_ON,
	TT_LIGHT_OFF,
	TT_TELEPORT,
	TT_SPEEDBOOST,
	TT_CAMERA,
	TT_SHIELD_DAMAGE,
	TT_ENERGY_DRAIN
	};

//link Links [MAX_WALL_LINKS];
//int Num_links;

#ifdef EDITOR
fix triggerTime_count=F1_0;

//-----------------------------------------------------------------
// Initializes all the switches.
void trigger_init ()
{
	int i;

	gameData.trigs.nTriggers = 0;

for (i = 0; i < MAX_TRIGGERS; i++) {
	gameData.trigs.triggers [i].nType = 0;
	gameData.trigs.triggers [i].flags = 0;
	gameData.trigs.triggers [i].nLinks = 0;
	gameData.trigs.triggers [i].value = 0;
	gameData.trigs.triggers [i].time = -1;
	}
memset (gameData.trigs.delay, -1, sizeof (gameData.trigs.delay));
}
#endif

//-----------------------------------------------------------------
// Executes a link, attached to a tTrigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
void DoLink (tTrigger *trigP)
{
	int i;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
for (i = trigP->nLinks; i; i--, segs++, sides++)
	WallToggle (gameData.segs.segments + *segs, *sides);
}

//------------------------------------------------------------------------------
//close a door
void DoCloseDoor (tTrigger *trigP)
{
	int i;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
for (i = trigP->nLinks; i; i--, segs++, sides++)
	WallCloseDoor (gameData.segs.segments+*segs, *sides);
}

//------------------------------------------------------------------------------
//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been shot out).
int DoLightOn (tTrigger *trigP)
{
	int i,ret=0;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
short nSegment,nSide;
for (i = trigP->nLinks; i; i--, segs++, sides++) {
	nSegment = *segs;
	nSide = *sides;

	//check if tmap2 casts light before turning the light on.  This
	//is to keep us from turning on blown-out lights
	if (gameData.pig.tex.pTMapInfo [gameData.segs.segments [nSegment].sides [nSide].nOvlTex].lighting) {
		ret |= AddLight (nSegment, nSide); 		//any light sets flag
		EnableFlicker (nSegment, nSide);
	}
}
return ret;
}

//------------------------------------------------------------------------------
//turns lighting off.  returns true if lights were actually turned off. (they
//would not be if they had previously been shot out).
int DoLightOff (tTrigger *trigP)
{
	int i,ret=0;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
short nSegment,nSide;
for (i = trigP->nLinks; i; i--, segs++, sides++) {
	nSegment = *segs;
	nSide = *sides;

	//check if tmap2 casts light before turning the light off.  This
	//is to keep us from turning off blown-out lights
	if (gameData.pig.tex.pTMapInfo [gameData.segs.segments [nSegment].sides [nSide].nOvlTex].lighting) {
		ret |= SubtractLight (nSegment, nSide); 	//any light sets flag
		DisableFlicker (nSegment, nSide);
	}
}
return ret;
}

//------------------------------------------------------------------------------
// Unlocks all doors linked to the switch.
void DoUnlockDoors (tTrigger *trigP)
{
	int i;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
short nSegment,nSide, wallnum;
for (i = trigP->nLinks; i; i--, segs++, sides++) {
	nSegment = *segs;
	nSide = *sides;
	wallnum=WallNumI (nSegment, nSide);
	gameData.walls.walls [wallnum].flags &= ~WALL_DOOR_LOCKED;
	gameData.walls.walls [wallnum].keys = KEY_NONE;
}
}

//------------------------------------------------------------------------------
// Return tTrigger number if door is controlled by a wall switch, else return -1.
int DoorIsWallSwitched (int nWall)
{
	int i, trigger_num;
	tTrigger *trigP = gameData.trigs.triggers;

for (trigger_num=0; trigger_num<gameData.trigs.nTriggers; trigger_num++,trigP++) {
	short *segs = trigP->nSegment;
	short *sides = trigP->nSide;
	for (i = trigP->nLinks; i; i--, segs++, sides++) {
		if (WallNumI (*segs, *sides) == nWall) {
			return trigger_num;
			}
	  	}
	}
return -1;
}

//------------------------------------------------------------------------------

void FlagWallSwitchedDoors (void)
{
	int	i;

for (i = 0; i < gameData.walls.nWalls; i++)
	if (DoorIsWallSwitched (i))
		gameData.walls.walls [i].flags |= WALL_WALL_SWITCH;
}

//------------------------------------------------------------------------------
// Locks all doors linked to the switch.
void DoLockDoors (tTrigger *trigP)
{
	int i;
	short *segs = trigP->nSegment;
	short *sides = trigP->nSide;

for (i = trigP->nLinks; i; i--, segs++, sides++) {
	gameData.walls.walls [WallNumI (*segs, *sides)].flags |= WALL_DOOR_LOCKED;
}
}

//------------------------------------------------------------------------------
// Changes walls pointed to by a tTrigger. returns true if any walls changed
int DoChangeWalls (tTrigger *trigP)
{
	int i,ret=0;
	short *segs = trigP->nSegment;
	short *sides = trigP->nSide;

for (i = trigP->nLinks; i; i--, segs++, sides++) {
	tSegment *segp,*csegp;
	short tSide,cside,wallnum,cwallnum;
	int new_wallType;

	segp = gameData.segs.segments+*segs;
	tSide = *sides;

	if (segp->children [tSide] < 0) {
		if (gameOpts->legacy.bSwitches)
			Warning (TXT_TRIG_SINGLE,
				*segs, tSide, TRIG_IDX (trigP));
		csegp = NULL;
		cside = -1;
		}
	else {
		csegp = gameData.segs.segments + segp->children [tSide];
		cside = FindConnectedSide (segp, csegp);
		}
	switch (trigP->nType) {
		case TT_OPEN_WALL:
			new_wallType = WALL_OPEN; 
			break;
		case TT_CLOSE_WALL:		
			new_wallType = WALL_CLOSED; 
			break;
		case TT_ILLUSORY_WALL:	
			new_wallType = WALL_ILLUSION; 
			break;
			default:
			Assert (0); /* new_wallType unset */
			return (0);
			break;
		}
	wallnum = WallNumP (segp, tSide);
	cwallnum = (cside < 0) ? NO_WALL : WallNumP (csegp, cside);
	if ((gameData.walls.walls [wallnum].nType == new_wallType) &&
			(!IS_WALL (cwallnum) || (gameData.walls.walls [cwallnum].nType == new_wallType)))
		continue;		//already in correct state, so skip
	ret = 1;
	switch (trigP->nType) {
		case TT_OPEN_WALL:
			if ((gameData.pig.tex.pTMapInfo [segp->sides [tSide].nBaseTex].flags & TMI_FORCE_FIELD)) {
				vmsVector pos;
				COMPUTE_SIDE_CENTER (&pos, segp, tSide);
				DigiLinkSoundToPos (SOUND_FORCEFIELD_OFF, SEG_IDX (segp), tSide, &pos, 0, F1_0);
				gameData.walls.walls [wallnum].nType = new_wallType;
				DigiKillSoundLinkedToSegment (SEG_IDX (segp),tSide,SOUND_FORCEFIELD_HUM);
				if (IS_WALL (cwallnum)) {
					gameData.walls.walls [cwallnum].nType = new_wallType;
					DigiKillSoundLinkedToSegment (SEG_IDX (csegp),cside,SOUND_FORCEFIELD_HUM);
					}
				}
			else
				StartWallCloak (segp,tSide);
			ret = 1;
			break;

		case TT_CLOSE_WALL:
			if ((gameData.pig.tex.pTMapInfo [segp->sides [tSide].nBaseTex].flags & TMI_FORCE_FIELD)) {
				vmsVector pos;
				COMPUTE_SIDE_CENTER (&pos, segp, tSide);
				DigiLinkSoundToPos (SOUND_FORCEFIELD_HUM, SEG_IDX (segp),tSide,&pos,1, F1_0/2);
				gameData.walls.walls [wallnum].nType = new_wallType;
				if (IS_WALL (cwallnum))
					gameData.walls.walls [cwallnum].nType = new_wallType;
				}
			else
				StartWallDecloak (segp,tSide);
			break;

		case TT_ILLUSORY_WALL:
			gameData.walls.walls [WallNumP (segp, tSide)].nType = new_wallType;
			if (IS_WALL (cwallnum))
				gameData.walls.walls [cwallnum].nType = new_wallType;
			break;
		}
	KillStuckObjects (WallNumP (segp, tSide));
	if (IS_WALL (cwallnum))
		KillStuckObjects (cwallnum);
  	}
return ret;
}

//------------------------------------------------------------------------------

void PrintTriggerMessage (int pnum,int trig,int shot,char *message)
 {
	char *pl;		//points to 's' or nothing for plural word

   if (pnum != gameData.multi.nLocalPlayer)
		return;

	pl = (gameData.trigs.triggers [trig].nLinks>1)?"s":"";

    if (!(gameData.trigs.triggers [trig].flags & TF_NO_MESSAGE) && shot)
     HUDInitMessage (message,pl);
 }


//------------------------------------------------------------------------------

void DoMatCen (tTrigger *trigP)
{
	int i;

short *segs = trigP->nSegment;
for (i = trigP->nLinks; i; i--, segs++)
	MatCenTrigger (*segs);
}

//------------------------------------------------------------------------------

void DoIllusionOn (tTrigger *trigP)
{
	int i;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
for (i = trigP->nLinks; i; i--, segs++, sides++) {
	WallIllusionOn (&gameData.segs.segments [*segs], *sides);
}
}

//------------------------------------------------------------------------------

void DoIllusionOff (tTrigger *trigP)
{
	int i;
	short *segs = trigP->nSegment;
	short *sides = trigP->nSide;
	tSegment *seg;

for (i = trigP->nLinks; i; i--, segs++, sides++) {
	vmsVector	cp;
	seg = gameData.segs.segments + *segs;
	WallIllusionOff (seg, *sides);
	COMPUTE_SIDE_CENTER (&cp, seg, *sides);
	DigiLinkSoundToPos (SOUND_WALL_REMOVED, SEG_IDX (seg), *sides, &cp, 0, F1_0);
  	}
}

//------------------------------------------------------------------------------

void TriggerSetObjOrient (short nObject, short nSegment, short nSide, int bSetPos, int nStep)
{
	vmsAngVec	ad, an, av;
	vmsVector	n, vel;
	vmsMatrix	rm;
	tObject		*objP = gameData.objs.objects + nObject;

if (nStep <= 0) {
#ifdef COMPACT_SEGS
	GetSideNormals (gameData.segs.segments + nSegment, int nSide, &n1, &n2);
#else
	n = *gameData.segs.segments [nSegment].sides [nSide].normals;
#endif
	n.x = -n.x;
	n.y = -n.y;
	n.z = -n.z;
	gameStates.gameplay.vTgtDir = n;
	if (nStep < 0)
		nStep = MAX_ORIENT_STEPS;
	}
else
	n = gameStates.gameplay.vTgtDir;
// turn the ship so that it is facing the destination tSide of the destination tSegment
// invert the normal as it points into the tSegment
// compute angles from the normal
VmExtractAnglesVector (&an, &n);
// create new orientation matrix
if (!nStep)
	VmAngles2Matrix (&objP->orient, &an);
if (bSetPos)
	COMPUTE_SEGMENT_CENTER_I (&objP->pos, + nSegment); 
// rotate the ships vel vector accordingly
VmExtractAnglesVector (&av, &objP->mType.physInfo.velocity);
av.p -= an.p;
av.b -= an.b;
av.h -= an.h;
if (nStep) {
	if (nStep > 1) {
		av.p /= nStep;
		av.b /= nStep;
		av.h /= nStep;
		VmExtractAnglesMatrix (&ad, &objP->orient);
		ad.p += (an.p - ad.p) / nStep;
		ad.b += (an.b - ad.b) / nStep;
		ad.h += (an.h - ad.h) / nStep;
		VmAngles2Matrix (&objP->orient, &ad);
		}
	else
		VmAngles2Matrix (&objP->orient, &an);
	}
VmAngles2Matrix (&rm, &av);
VmVecRotate (&vel, &objP->mType.physInfo.velocity, &rm);
objP->mType.physInfo.velocity = vel;
//StopPlayerMovement ();
}

//------------------------------------------------------------------------------

void TriggerSetObjPos (short nObject, short nSegment)
{
RelinkObject (nObject, nSegment);
}

//------------------------------------------------------------------------------

void DoTeleport (tTrigger *trigP, short nObject)
{
if (trigP->nLinks > 0) {
		int		i;
		short		nSegment, nSide;

	d_srand (TimerGetFixedSeconds ());
	i = d_rand () % trigP->nLinks;
	nSegment = trigP->nSegment [i];
	nSide = trigP->nSide [i];
	// set new player direction, facing the destination tSide
	TriggerSetObjOrient (nObject, nSegment, nSide, 1, 0);
	TriggerSetObjPos (nObject, nSegment);
	gameStates.render.bDoAppearanceEffect = 1;
	MultiSendTeleport ((char) nObject, nSegment, (char) nSide);
	}
}

//------------------------------------------------------------------------------

wall *TriggerParentWall (short trigger_num)
{
	int	i;

for (i = 0; i < gameData.walls.nWalls; i++)
	if (gameData.walls.walls [i].tTrigger == trigger_num)
		return gameData.walls.walls + i;
return NULL;
}

//------------------------------------------------------------------------------

vmsVector	boostedVel, minBoostedVel, maxBoostedVel;
vmsVector	speedBoostSrc, speedBoostDest;
fix			speedBoostSpeed = 0;

void SetSpeedBoostVelocity (short nObject, fix speed, 
									 short srcSegnum, short srcSidenum,
									 short destSegnum, short destSidenum,
									 vmsVector *pSrcPt, vmsVector *pDestPt,
									 int bSetOrient)
{
	vmsVector	n, h;
	tObject		*objP = gameData.objs.objects + nObject;
	int			v;

if (speed < 0)
	speed = speedBoostSpeed;
if ((speed <= 0) || (speed > 10))
	speed = 10;
speedBoostSpeed = speed;
v = 60 + extraGameInfo [IsMultiGame].nSpeedBoost * 4 * speed;
if (gameStates.gameplay.bSpeedBoost) {
	if (pSrcPt && pDestPt) {
		VmVecSub (&n, pDestPt, pSrcPt);
		VmVecNormalize (&n);
		}
	else if (srcSegnum >= 0) {
		COMPUTE_SIDE_CENTER (&speedBoostSrc, gameData.segs.segments + srcSegnum, srcSidenum);
		COMPUTE_SIDE_CENTER (&speedBoostDest, gameData.segs.segments + destSegnum, destSidenum);
		if (memcmp (&speedBoostSrc, &speedBoostDest, sizeof (vmsVector))) {
			VmVecSub (&n, &speedBoostDest, &speedBoostSrc);
			VmVecNormalize (&n);
			}
		else {
			Controls.vertical_thrustTime =
			Controls.forward_thrustTime =
			Controls.sideways_thrustTime = 0;
#ifdef COMPACT_SEGS
			GetSideNormals (gameData.segs.segments + destSegnum, destSidenum, &n1, &n2);
#else
			memcpy (&n, gameData.segs.segments [destSegnum].sides [destSidenum].normals, sizeof (n));
#endif
		// turn the ship so that it is facing the destination tSide of the destination tSegment
		// invert the normal as it points into the tSegment
			n.x = -n.x;
			n.y = -n.y;
			n.z = -n.z;
			}
		}
	else {
#ifdef COMPACT_SEGS
		GetSideNormals (gameData.segs.segments + destSegnum, destSidenum, &n1, &n2);
#else
		memcpy (&n, gameData.segs.segments [destSegnum].sides [destSidenum].normals, sizeof (n));
#endif
	// turn the ship so that it is facing the destination tSide of the destination tSegment
	// invert the normal as it points into the tSegment
		n.x = -n.x;
		n.y = -n.y;
		n.z = -n.z;
		}
	boostedVel.x = n.x * v;
	boostedVel.y = n.y * v;
	boostedVel.z = n.z * v;
#if 0
	d = (double) (labs (n.x) + labs (n.y) + labs (n.z)) / ((double) F1_0 * 60.0);
	h.x = n.x ? (fix) ((double) n.x / d) : 0;
	h.y = n.y ? (fix) ((double) n.y / d) : 0;
	h.z = n.z ? (fix) ((double) n.z / d) : 0;
#else
#	if 1
	h.x =
	h.y =
	h.z = F1_0 * 60;
#	else
	h.x = (n.x ? n.x : F1_0) * 60;
	h.y = (n.y ? n.y : F1_0) * 60;
	h.z = (n.z ? n.z : F1_0) * 60;
#	endif
#endif
	VmVecSub (&minBoostedVel, &boostedVel, &h);
/*
	if (!minBoostedVel.x)
		minBoostedVel.x = F1_0 * -60;
	if (!minBoostedVel.y)
		minBoostedVel.y = F1_0 * -60;
	if (!minBoostedVel.z)
		minBoostedVel.z = F1_0 * -60;
*/
	VmVecAdd (&maxBoostedVel, &boostedVel, &h);
/*
	if (!maxBoostedVel.x)
		maxBoostedVel.x = F1_0 * 60;
	if (!maxBoostedVel.y)
		maxBoostedVel.y = F1_0 * 60;
	if (!maxBoostedVel.z)
		maxBoostedVel.z = F1_0 * 60;
*/
	if (minBoostedVel.x > maxBoostedVel.x) {
		fix h = minBoostedVel.x;
		minBoostedVel.x = maxBoostedVel.x;
		maxBoostedVel.x = h;
		}
	if (minBoostedVel.y > maxBoostedVel.y) {
		fix h = minBoostedVel.y;
		minBoostedVel.y = maxBoostedVel.y;
		maxBoostedVel.y = h;
		}
	if (minBoostedVel.z > maxBoostedVel.z) {
		fix h = minBoostedVel.z;
		minBoostedVel.z = maxBoostedVel.z;
		maxBoostedVel.z = h;
		}
	objP->mType.physInfo.velocity = boostedVel;
	if (bSetOrient) {
		TriggerSetObjOrient (nObject, destSegnum, destSidenum, 0, -1);
		gameStates.gameplay.nDirSteps = MAX_ORIENT_STEPS - 1;
		}
	}
else {
	objP->mType.physInfo.velocity.x = objP->mType.physInfo.velocity.x / v * 60;
	objP->mType.physInfo.velocity.y = objP->mType.physInfo.velocity.y / v * 60;
	objP->mType.physInfo.velocity.z = objP->mType.physInfo.velocity.z / v * 60;
	}
}

//------------------------------------------------------------------------------

void UpdatePlayerOrient (void)
{
if (gameStates.app.b40fpsTick && gameStates.gameplay.nDirSteps)
	TriggerSetObjOrient (gameData.multi.players [gameData.multi.nLocalPlayer].nObject, -1, -1, 0, gameStates.gameplay.nDirSteps--);
}

//------------------------------------------------------------------------------

void DoSpeedBoost (tTrigger *trigP, short nObject)
{
if (extraGameInfo [IsMultiGame].nSpeedBoost) {
	wall *w = TriggerParentWall (TRIG_IDX (trigP));
	gameStates.gameplay.bSpeedBoost = (trigP->value && (trigP->nLinks > 0));
	SetSpeedBoostVelocity ((short) nObject, trigP->value, 
								  (short) (w ? w->nSegment : -1), (short) (w ? w->nSide : -1),
								  trigP->nSegment [0], trigP->nSide [0], NULL, NULL, (trigP->flags & TF_SET_ORIENT) != 0);
	}
}

//------------------------------------------------------------------------------

extern void EnterSecretLevel (void);
extern void ExitSecretLevel (void);
extern int PSecretLevelDestroyed (void);

int WallIsForceField (tTrigger *trigP)
{
	int i;
	short *segs = trigP->nSegment;
	short *sides = trigP->nSide;

for (i = trigP->nLinks; i; i--, segs++, sides++)
	if ((gameData.pig.tex.pTMapInfo [gameData.segs.segments [*segs].sides [*sides].nBaseTex].flags & TMI_FORCE_FIELD))
		break;
return (i > 0);
}

//------------------------------------------------------------------------------

int CheckTriggerSub (short nObject, tTrigger *triggers, int num_triggers, int nTrigger, int pnum, int shot)
{
	tTrigger	*trigP;
	tObject	*objP = gameData.objs.objects + nObject;
	ubyte		bIsPlayer = (objP->nType == OBJ_PLAYER);

if (nTrigger >= num_triggers)
	return 1;
trigP = triggers + nTrigger;
if (trigP->flags & TF_DISABLED)
	return 1;		//1 means don'trigP send tTrigger hit to other players
if (bIsPlayer) {
	if (nObject != gameData.multi.players [gameData.multi.nLocalPlayer].nObject)
		return 1;
	}
else if ((trigP->nType != TT_TELEPORT) && (trigP->nType != TT_SPEEDBOOST)) {
	if ((objP->nType != OBJ_ROBOT) || !gameData.bots.pInfo [objP->id].companion)
		return 1;
	}
#if 1
if ((triggers == gameData.trigs.triggers) && (trigP->nType != TT_SPEEDBOOST)) {
	long trigP = gameStates.app.nSDLTicks;
	if ((gameData.trigs.delay [nTrigger] >= 0) && (trigP - gameData.trigs.delay [nTrigger] < 1000))
		return 1;
	gameData.trigs.delay [nTrigger] = trigP;
	}
#endif
if (trigP->flags & TF_ONE_SHOT)		//if this is a one-shot...
	trigP->flags |= TF_DISABLED;		//..then don'trigP let it happen again

switch (trigP->nType) {

	case TT_EXIT:
		if (pnum != gameData.multi.nLocalPlayer)
			break;
		DigiStopAll ();		//kill the sounds
		if ((gameData.missions.nCurrentLevel > 0) || gameStates.app.bD1Mission) {
			StartEndLevelSequence (0);
			} 
		else if (gameData.missions.nCurrentLevel < 0) {
			if ((gameData.multi.players [gameData.multi.nLocalPlayer].shields < 0) || 
					gameStates.app.bPlayerIsDead)
				break;
			ExitSecretLevel ();
			return 1;
			}
		else {
#ifdef EDITOR
				ExecMessageBox ("Yo!", 1, "You have hit the exit tTrigger!", "");
#else
				Int3 ();		//level num == 0, but no editor!
			#endif
			}
		return 1;
		break;

	case TT_SECRET_EXIT: {
#ifndef SHAREWARE
		int	truth;
#endif

		if (pnum != gameData.multi.nLocalPlayer)
			break;
		if ((gameData.multi.players [gameData.multi.nLocalPlayer].shields < 0) || 
				gameStates.app.bPlayerIsDead)
			break;
		if (gameData.app.nGameMode & GM_MULTI) {
			HUDInitMessage (TXT_TELEPORT_MULTI);
			DigiPlaySample (SOUND_BAD_SELECTION, F1_0);
			break;
		}
#ifndef SHAREWARE
		truth = PSecretLevelDestroyed ();

		if (gameData.demo.nState == ND_STATE_RECORDING)			// record whether we're really going to the secret level
			NDRecordSecretExitBlown (truth);

		if ((gameData.demo.nState != ND_STATE_PLAYBACK) && truth) {
			HUDInitMessage (TXT_SECRET_DESTROYED);
			DigiPlaySample (SOUND_BAD_SELECTION, F1_0);
			break;
		}
#endif

#ifdef SHAREWARE
			HUDInitMessage (TXT_TELEPORT_DEMO);
			DigiPlaySample (SOUND_BAD_SELECTION, F1_0);
			break;
#endif
		if (gameData.demo.nState == ND_STATE_RECORDING)		// stop demo recording
			gameData.demo.nState = ND_STATE_PAUSED;
		DigiStopAll ();		//kill the sounds
		DigiPlaySample (SOUND_SECRET_EXIT, F1_0);
		GrPaletteFadeOut (NULL, 32, 0);
		EnterSecretLevel ();
		gameData.reactor.bDestroyed = 0;
		return 1;
		break;
	}

	case TT_OPEN_DOOR:
		DoLink (trigP);
		PrintTriggerMessage (pnum,nTrigger,shot,"Door%s opened!");
		break;

	case TT_CLOSE_DOOR:
		DoCloseDoor (trigP);
		PrintTriggerMessage (pnum,nTrigger,shot,"Door%s closed!");
		break;

	case TT_UNLOCK_DOOR:
		DoUnlockDoors (trigP);
		PrintTriggerMessage (pnum,nTrigger,shot,"Door%s unlocked!");
		break;

	case TT_LOCK_DOOR:
		DoLockDoors (trigP);
		PrintTriggerMessage (pnum,nTrigger,shot,"Door%s locked!");
		break;

	case TT_OPEN_WALL:
		if (DoChangeWalls (trigP))
		{
			if (WallIsForceField (trigP))
				PrintTriggerMessage (pnum,nTrigger,shot,"Force field%s deactivated!");
			else
				PrintTriggerMessage (pnum,nTrigger,shot,"Wall%s opened!");
		}
		break;

	case TT_CLOSE_WALL:
		if (DoChangeWalls (trigP))
		{
			if (WallIsForceField (trigP))
				PrintTriggerMessage (pnum,nTrigger,shot,"Force field%s activated!");
			else
				PrintTriggerMessage (pnum,nTrigger,shot,"Wall%s closed!");
		}
		break;

	case TT_ILLUSORY_WALL:
		//don'trigP know what to say, so say nothing
		DoChangeWalls (trigP);
		break;

	case TT_MATCEN:
		if (!(gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_ROBOTS))
			DoMatCen (trigP);
		break;

	case TT_ILLUSION_ON:
		DoIllusionOn (trigP);
		PrintTriggerMessage (pnum,nTrigger,shot,"Illusion%s on!");
		break;

	case TT_ILLUSION_OFF:
		DoIllusionOff (trigP);
		PrintTriggerMessage (pnum,nTrigger,shot,"Illusion%s off!");
		break;

	case TT_LIGHT_OFF:
		if (DoLightOff (trigP))
			PrintTriggerMessage (pnum,nTrigger,shot,"Lights off!");
		break;

	case TT_LIGHT_ON:
		if (DoLightOn (trigP))
			PrintTriggerMessage (pnum,nTrigger,shot,"Lights on!");
		break;

	case TT_TELEPORT:
		if (bIsPlayer) {
			if (pnum != gameData.multi.nLocalPlayer)
				break;
			if ((gameData.multi.players [gameData.multi.nLocalPlayer].shields < 0) || 
					gameStates.app.bPlayerIsDead)
				break;
			}
		DigiPlaySample (SOUND_SECRET_EXIT, F1_0);
		DoTeleport (trigP, nObject);
		if (bIsPlayer)
			PrintTriggerMessage (pnum,nTrigger,shot,"Teleport!");
		break;

	case TT_SPEEDBOOST:
		if (bIsPlayer) {
			if (pnum != gameData.multi.nLocalPlayer)
				break;
			if ((gameData.multi.players [gameData.multi.nLocalPlayer].shields < 0) || 
					gameStates.app.bPlayerIsDead)
				break;
			}
		DoSpeedBoost (trigP, nObject);
		if (bIsPlayer)
			PrintTriggerMessage (pnum,nTrigger, shot, "Speed Boost!");
		break;

	case TT_SHIELD_DAMAGE:
		gameData.multi.players [gameData.multi.nLocalPlayer].shields += gameData.trigs.triggers [nTrigger].value;
		break;

	case TT_ENERGY_DRAIN:
		gameData.multi.players [gameData.multi.nLocalPlayer].energy += gameData.trigs.triggers [nTrigger].value;
		break;

	default:
		Int3 ();
		break;
	}
if (trigP->flags & TF_ALTERNATE)
		trigP->nType = oppTrigTypes [trigP->nType];
return 0;
}

//------------------------------------------------------------------------------

void ExecObjTriggers (short nObject)
{
	short i = gameData.trigs.firstObjTrigger [nObject];

while (i >= 0) {
	CheckTriggerSub (nObject, gameData.trigs.objTriggers, gameData.trigs.nObjTriggers, i, gameData.multi.nLocalPlayer, 1);
#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendObjTrigger (i);
#endif
	i = gameData.trigs.objTriggerRefs [i].next;
	}
}

//-----------------------------------------------------------------
// Checks for a tTrigger whenever an tObject hits a tTrigger tSide.
void CheckTrigger (tSegment *seg, short tSide, short nObject, int shot)
{
	int 	nWall;
	ubyte	nTrigger;	//, cnTrigger;

nWall = WallNumP (seg, tSide);
if (!IS_WALL (nWall)) 
	return;
nTrigger = gameData.walls.walls [nWall].tTrigger;
if (CheckTriggerSub (nObject, gameData.trigs.triggers, gameData.trigs.nTriggers, nTrigger, gameData.multi.nLocalPlayer,shot))
	return;
if (gameData.demo.nState == ND_STATE_RECORDING)
NDRecordTrigger (SEG_IDX (seg), tSide, nObject, shot);
#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI)
	MultiSendTrigger (nTrigger, nObject);
#endif
}

//------------------------------------------------------------------------------

void TriggersFrameProcess ()
{
	int i;

for (i = 0; i < gameData.trigs.nTriggers; i++)
	if (gameData.trigs.triggers [i].time >= 0)
		gameData.trigs.triggers [i].time -= gameData.time.xFrame;
}

//------------------------------------------------------------------------------

#ifndef FAST_FILE_IO
#if 0
	static char d2TriggerMap [10] = {
		TT_OPEN_DOOR,
		TT_SHIELD_DAMAGE,
		TT_ENERGY_DRAIN,
		TT_EXIT,
		-1,
		-1,
		TT_MATCEN,
		TT_ILLUSION_OFF,
		TT_ILLUSION_ON,
		TT_SECRET_EXIT
		};

	static char d2FlagMap [10] = {
		0,
		0,
		0,
		0,
		0,
		TF_ONE_SHOT,
		0,
		0,
		0,
		0
		};
#endif

/*
 * reads a v29_trigger structure from a CFILE
 */
extern void v29_trigger_read (v29_trigger *trigP, CFILE *fp)
{
	int	i;

trigP->nType = CFReadByte (fp);
trigP->flags = CFReadShort (fp);
trigP->value = CFReadFix (fp);
trigP->time = CFReadFix (fp);
trigP->link_num = CFReadByte (fp);
trigP->nLinks = CFReadShort (fp);
for (i=0; i<MAX_WALLS_PER_LINK; i++)
	trigP->nSegment [i] = CFReadShort (fp);
for (i=0; i<MAX_WALLS_PER_LINK; i++)
	trigP->nSide [i] = CFReadShort (fp);
/*
for (i = 0; i < 10; i++)
	if ((d2TriggerMap [i] >= 0) && (flags & (1 << i))) {
		trigP->nType = d2TriggerMap [i];
		break;
		}
trigP->flags = 0;
for (i = 0; i < 10; i++)
	if ((d2FlagMap [i] > 0) && (flags & (1 << i))) {
		trigP->flags |= d2FlagMap [i];
		break;
		}
*/
}

//------------------------------------------------------------------------------

/*
 * reads a v30_trigger structure from a CFILE
 */
extern void v30_trigger_read (v30_trigger *trigP, CFILE *fp)
{
	int i;

trigP->flags = CFReadShort (fp);
trigP->nLinks = CFReadByte (fp);
trigP->pad = CFReadByte (fp);
trigP->value = CFReadFix (fp);
trigP->time = CFReadFix (fp);
for (i=0; i<MAX_WALLS_PER_LINK; i++)
	trigP->nSegment [i] = CFReadShort (fp);
for (i=0; i<MAX_WALLS_PER_LINK; i++)
	trigP->nSide [i] = CFReadShort (fp);
}

//------------------------------------------------------------------------------

/*
 * reads a tTrigger structure from a CFILE
 */
extern void TriggerRead (tTrigger *trigP, CFILE *fp, int bObjTrigger)
{
	int i;

trigP->nType = CFReadByte (fp);
if (bObjTrigger)
	trigP->flags = (short) CFReadShort (fp);
else
	trigP->flags = (short) CFReadByte (fp);
trigP->nLinks = CFReadByte (fp);
CFReadByte (fp);
trigP->value = CFReadFix (fp);
trigP->time = CFReadFix (fp);
for (i=0; i<MAX_WALLS_PER_LINK; i++)
	trigP->nSegment [i] = CFReadShort (fp);
for (i=0; i<MAX_WALLS_PER_LINK; i++)
	trigP->nSide [i] = CFReadShort (fp);
}
#endif

//------------------------------------------------------------------------------
//eof