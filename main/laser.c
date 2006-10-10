/* $Id: laser.c,v 1.10 2003/10/10 09:36:35 btb Exp $ */
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
 * This will contain the laser code
 *
 * Old Log:
 * Revision 1.1  1993/11/29  17:19:02  john
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
char laser_rcsid [] = "$Id: laser.c,v 1.10 2003/10/10 09:36:35 btb Exp $";
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "inferno.h"
#include "game.h"
#include "bm.h"
#include "object.h"
#include "laser.h"
#include "args.h"
#include "segment.h"
#include "fvi.h"
#include "segpoint.h"
#include "error.h"
#include "mono.h"
#include "key.h"
#include "texmap.h"
#include "textures.h"
#include "render.h"
#include "vclip.h"
#include "fireball.h"
#include "polyobj.h"
#include "robot.h"
#include "weapon.h"
#include "newdemo.h"
#include "timer.h"
#include "player.h"
#include "sounds.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "ai.h"
#ifdef NETWORK
#include "modem.h"
#endif
#include "powerup.h"
#include "multi.h"
#include "physics.h"
#include "multi.h"
#include "hudmsg.h"
#include "gameseg.h"

#ifdef TACTILE
#include "tactile.h"
#endif

int FindHomingObjectComplete (vms_vector *curpos, object *tracker, int track_obj_type1, int track_obj_type2);

extern void NDRecordGuidedEnd ();
extern void NDRecordGuidedStart ();

int FindHomingObject (vms_vector *curpos, object *tracker);

extern int bDoingLightingHack;

//	-------------------------------------------------------------------------------------------------------------------------------
//	***** HEY ARTISTS!!*****
//	Here are the constants you're looking for!--MK

//	Change the following constants to affect the look of the omega cannon.
//	Changing these constants will not affect the damage done.
//	WARNING: If you change DESIRED_OMEGA_DIST and MAX_OMEGA_BLOBS, you don't merely change the look of the cannon,
//	you change its range.  If you decrease DESIRED_OMEGA_DIST, you decrease how far the gun can fire.
#define	MIN_OMEGA_BLOBS		3				//	No matter how close the obstruction, at this many blobs created.
#define	MIN_OMEGA_DIST			(F1_0*3)		//	At least this distance between blobs, unless doing so would violate MIN_OMEGA_BLOBS
#define	DESIRED_OMEGA_DIST	(F1_0*5)		//	This is the desired distance between blobs.  For distances > MIN_OMEGA_BLOBS*DESIRED_OMEGA_DIST, but not very large, this will apply.
#define	MAX_OMEGA_BLOBS		16				//	No matter how far away the obstruction, this is the maximum number of blobs.
#define	MAX_OMEGA_DIST			(MAX_OMEGA_BLOBS * DESIRED_OMEGA_DIST)		//	Maximum extent of lightning blobs.

//	Additionally, several constants which apply to homing gameData.objs.objects in general control the behavior of the Omega Cannon.
//	They are defined in laser.h.  They are copied here for reference.  These values are valid on 1/10/96:
//	If you want the Omega Cannon view cone to be different than the Homing Missile viewcone, contact MK to make the change.
//	 (Unless you are a programmer, in which case, do it yourself!)
#define	OMEGA_MIN_TRACKABLE_DOT			 (15*F1_0/16)		//	Larger values mean narrower cone.  F1_0 means damn near impossible.  0 means 180 degree field of view.
#define	OMEGA_MAX_TRACKABLE_DIST		MAX_OMEGA_DIST	//	An object must be at least this close to be tracked.

//	Note, you don't need to change these constants.  You can control damage and energy consumption by changing the
//	usual bitmaps.tbl parameters.
#define	OMEGA_DAMAGE_SCALE			32				//	Controls how much damage is done.  This gets multiplied by gameData.time.xFrame and then again by the damage specified in bitmaps.tbl in the $WEAPON line.
#define	OMEGA_ENERGY_CONSUMPTION	16				//	Controls how much energy is consumed.  This gets multiplied by gameData.time.xFrame and then again by the energy parameter from bitmaps.tbl.

#define	MIN_OMEGA_CHARGE	 (MAX_OMEGA_CHARGE/8)
#define	OMEGA_CHARGE_SCALE	4			//	gameData.time.xFrame / OMEGA_CHARGE_SCALE added to xOmegaCharge every frame.

#define	FULL_COCKPIT_OFFS 0
#define	LASER_OFFS	 ((F1_0 * 29) / 100)

#define	HOMING_MISSILE_SCALE	16

#define	MAX_SMART_DISTANCE	 (F1_0*150)
#define	MAX_OBJDISTS			30

fix	xOmegaCharge = MAX_OMEGA_CHARGE;
int	nLastOmegaFireFrame = 0;

//---------------------------------------------------------------------------------
// Called by render code.... determines if the laser is from a robot or the
// player and calls the appropriate routine.

void RenderLaser (object *objP)
{

//	Commented out by John (sort of, typed by Mike) on 6/8/94
#if 0
	switch (objP->id)	{
	case WEAPON_TYPE_WEAK_LASER:
	case WEAPON_TYPE_STRONG_LASER:
	case WEAPON_TYPE_CANNON_BALL:
	case WEAPON_TYPE_MISSILE:
		break;
	default:
		Error ("Invalid weapon type in RenderLaser\n");
	}
#endif
	
switch (gameData.weapons.info [objP->id].render_type)	{
	case WEAPON_RENDER_LASER:
		Int3 ();	// Not supported anymore!
					//Laser_draw_one (OBJ_IDX (objP), gameData.weapons.info [objP->id].bitmap);
		break;
	case WEAPON_RENDER_BLOB:
		DrawObjectBlob (objP, gameData.weapons.info [objP->id].bitmap, gameData.weapons.info [objP->id].bitmap, 0, NULL, 2.0f / 3.0f);
		break;
	case WEAPON_RENDER_POLYMODEL:
		break;
	case WEAPON_RENDER_VCLIP:
		Int3 ();	//	Oops, not supported, type added by mk on 09/09/94, but not for lasers...
	default:
		Error ("Invalid weapon render type in RenderLaser\n");
	}
}

//---------------------------------------------------------------------------------
// Draws a texture-mapped laser bolt

//void Laser_draw_one (int objnum, grs_bitmap * bmp)
//{
//	int t1, t2, t3;
//	g3s_point p1, p2;
//	object *objP;
//	vms_vector start_pos,end_pos;
//
//	obj = &gameData.objs.objects [objnum];
//
//	start_pos = objP->pos;
//	VmVecScaleAdd (&end_pos,&start_pos,&objP->orient.fvec,-Laser_length);
//
//	G3TransformAndEncodePoint (&p1,&start_pos);
//	G3TransformAndEncodePoint (&p2,&end_pos);
//
//	t1 = gameStates.render.nLighting;
//	t2 = gameStates.render.nInterpolationMethod;
//	t3 = gameStates.render.bTransparency;
//
//	gameStates.render.nLighting  = 0;
//	//gameStates.render.nInterpolationMethod = 3;	// Full perspective
//	gameStates.render.nInterpolationMethod = 1;	// Linear
//	gameStates.render.bTransparency = 1;
//
//	//GrSetColor (gr_getcolor (31,15,0);
//	//g3_draw_line_ptrs (p1,p2);
//	//g3_draw_rod (p1,0x2000,p2,0x2000);
//	//g3_draw_rod (p1,Laser_width,p2,Laser_width);
//	G3DrawRodTexPoly (bmp,&p2,Laser_width,&p1,Laser_width,0);
//	gameStates.render.nLighting = t1;
//	gameStates.render.nInterpolationMethod = t2;
//	gameStates.render.bTransparency = t3;
//
//}

//	Changed by MK on 09/07/94
//	I want you to be able to blow up your own bombs.
//	AND...Your proximity bombs can blow you up if they're 2.0 seconds or more old.
//	Changed by MK on 06/06/95: Now must be 4.0 seconds old.  Much valid Net-complaining.
int LasersAreRelated (int o1, int o2)
{
	object	*objP1, *objP2;
	short		id1, id2;
	fix		ct1, ct2;

if ((o1 < 0) || (o2 < 0))	
	return 0;
objP1 = gameData.objs.objects + o1;
objP2 = gameData.objs.objects + o2;
id1 = objP1->id;
id2 = objP2->id;
ct1 = objP1->ctype.laser_info.creation_time;
ct2 = objP2->ctype.laser_info.creation_time;
// See if o2 is the parent of o1
if (objP1->type == OBJ_WEAPON)
	if ((objP1->ctype.laser_info.parent_num == o2) && 
		 (objP1->ctype.laser_info.parent_signature == objP2->signature))
		//	o1 is a weapon, o2 is the parent of 1, so if o1 is PROXIMITY_BOMB and o2 is player, they are related only if o1 < 2.0 seconds old
		if ((id1 == PHOENIX_ID && (gameData.time.xGame > ct1 + F1_0/4)) || 
		   (id1 == GUIDEDMISS_ID && (gameData.time.xGame > ct1 + F1_0*2)) || 
		   (((id1 == PROXIMITY_ID) || (id1 == SUPERPROX_ID)) && (gameData.time.xGame > ct1 + F1_0*4)))
			return 0;
		else
			return 1;

	// See if o1 is the parent of o2
if (objP2->type == OBJ_WEAPON)
	if ((objP2->ctype.laser_info.parent_num == o1) && 
			 (objP2->ctype.laser_info.parent_signature == objP1->signature))
		//	o2 is a weapon, o1 is the parent of 2, so if o2 is PROXIMITY_BOMB and o1 is player, they are related only if o1 < 2.0 seconds old
		if ((id2 == PHOENIX_ID && (gameData.time.xGame > ct2 + F1_0/4)) || 
			  (id2 == GUIDEDMISS_ID && (gameData.time.xGame > ct2 + F1_0*2)) || 
				 (((id2 == PROXIMITY_ID) || (id2 == SUPERPROX_ID)) && (gameData.time.xGame > ct2 + F1_0*4)))
			return 0;
		else
			return 1;

// They must both be weapons
if (objP1->type != OBJ_WEAPON || objP2->type != OBJ_WEAPON)	
	return 0;

//	Here is the 09/07/94 change -- Siblings must be identical, others can hurt each other
// See if they're siblings...
//	MK: 06/08/95, Don't allow prox bombs to detonate for 3/4 second.  Else too likely to get toasted by your own bomb if hit by opponent.
if (objP1->ctype.laser_info.parent_signature==objP2->ctype.laser_info.parent_signature) {
	if (id1 != PROXIMITY_ID  && id2 != PROXIMITY_ID && id1 != SUPERPROX_ID && id2 != SUPERPROX_ID)
		return 1;
	//	If neither is older than 1/2 second, then can't blow up!
	if ((gameData.time.xGame > (ct1 + F1_0/2)) || (gameData.time.xGame > (ct2 + F1_0/2)))
		return 0;
	return 1;
	}

//	Anything can cause a collision with a robot super prox mine.
if (id1 == ROBOT_SUPERPROX_ID || id2 == ROBOT_SUPERPROX_ID ||
	 id1 == PROXIMITY_ID || id2 == PROXIMITY_ID ||
	 id1 == SUPERPROX_ID || id2 == SUPERPROX_ID ||
	 id1 == PMINE_ID || id2 == PMINE_ID)
	return 0;
return 1;
}

//---------------------------------------------------------------------------------
//--unused-- int Muzzle_scale=2;
int nLaserOffset=0;

void DoMuzzleStuff (int segnum, vms_vector *pos)
{
gameData.muzzle.info [gameData.muzzle.queueIndex].create_time = TimerGetFixedSeconds ();
gameData.muzzle.info [gameData.muzzle.queueIndex].segnum = segnum;
gameData.muzzle.info [gameData.muzzle.queueIndex].pos = *pos;
gameData.muzzle.queueIndex++;
if (gameData.muzzle.queueIndex >= MUZZLE_QUEUE_MAX)
	gameData.muzzle.queueIndex = 0;
}

//---------------------------------------------------------------------------------
//creates a weapon object
int CreateWeaponObject (ubyte weapon_type, short segnum,vms_vector *position)
{
	int rtype=-1;
	fix laser_radius = -1;
	int objnum;
	object *objP;

switch (gameData.weapons.info [weapon_type].render_type)	{
	case WEAPON_RENDER_BLOB:
		rtype = RT_LASER;			// Render as a laser even if blob (see render code above for explanation)
		laser_radius = gameData.weapons.info [weapon_type].blob_size;
		break;
	case WEAPON_RENDER_POLYMODEL:
		laser_radius = 0;	//	Filled in below.
		rtype = RT_POLYOBJ;
		break;
	case WEAPON_RENDER_LASER:
		Int3 (); 	// Not supported anymore
		break;
	case WEAPON_RENDER_NONE:
		rtype = RT_NONE;
		laser_radius = F1_0;
		break;
	case WEAPON_RENDER_VCLIP:
		rtype = RT_WEAPON_VCLIP;
		laser_radius = gameData.weapons.info [weapon_type].blob_size;
		break;
	default:
		Error ("Invalid weapon render type in CreateNewLaser\n");
		return -1;
	}

Assert (laser_radius != -1);
Assert (rtype != -1);
objnum = CreateObject ((ubyte) OBJ_WEAPON, weapon_type, -1, segnum, position, NULL, laser_radius, (ubyte) CT_WEAPON, (ubyte) MT_PHYSICS, (ubyte) rtype, 1);
objP = gameData.objs.objects + objnum;
if (gameData.weapons.info [weapon_type].render_type == WEAPON_RENDER_POLYMODEL) {
	objP->rtype.pobj_info.model_num = gameData.weapons.info [objP->id].model_num;
	objP->size = FixDiv (gameData.models.polyModels [objP->rtype.pobj_info.model_num].rad,gameData.weapons.info [objP->id].po_len_to_width_ratio);
	}
objP->mtype.phys_info.mass = WI_mass (weapon_type);
objP->mtype.phys_info.drag = WI_drag (weapon_type);
VmVecZero (&objP->mtype.phys_info.thrust);
if (gameData.weapons.info [weapon_type].bounce==1)
	objP->mtype.phys_info.flags |= PF_BOUNCE;
if (gameData.weapons.info [weapon_type].bounce==2 || gameStates.app.cheats.bBouncingWeapons)
	objP->mtype.phys_info.flags |= PF_BOUNCE+PF_BOUNCES_TWICE;
return objnum;
}

//	-------------------------------------------------------------------------------------------------------------------------------

void DeleteOldOmegaBlobs (object *parentObjP)
{
	short	i;
	int	count = 0;
	int	parent_num = parentObjP->ctype.laser_info.parent_num;

for (i = 0; i <= gameData.objs.nLastObject; i++)
	if (gameData.objs.objects [i].type == OBJ_WEAPON)
		if (gameData.objs.objects [i].id == OMEGA_ID)
			if (gameData.objs.objects [i].ctype.laser_info.parent_num == parent_num) {
				ReleaseObject (i);
				count++;
				}
}

// ---------------------------------------------------------------------------------

void CreateOmegaBlobs (short nFiringSeg, vms_vector *vFiringPos, vms_vector *vGoalPos, object *parentObjP)
{
	short			nLastSeg, nLastCreatedObj = -1;
	vms_vector	vGoal;
	fix			xGoalDist;
	int			nOmegaBlobs;
	fix			xOmegaBlobDist;
	vms_vector	vOmegaDelta;
	vms_vector	vBlobPos, vPerturb;
	fix			xPerturbArray [MAX_OMEGA_BLOBS];
	int			i;

if (gameData.app.nGameMode & GM_MULTI)
	DeleteOldOmegaBlobs (parentObjP);
VmVecSub (&vGoal, vGoalPos, vFiringPos);
xGoalDist = VmVecNormalizeQuick (&vGoal);
if (xGoalDist < MIN_OMEGA_BLOBS * MIN_OMEGA_DIST) {
	xOmegaBlobDist = MIN_OMEGA_DIST;
	nOmegaBlobs = xGoalDist/xOmegaBlobDist;
	if (nOmegaBlobs == 0)
		nOmegaBlobs = 1;
	} 
else {
	xOmegaBlobDist = DESIRED_OMEGA_DIST;
	nOmegaBlobs = xGoalDist / xOmegaBlobDist;
	if (nOmegaBlobs > MAX_OMEGA_BLOBS) {
		nOmegaBlobs = MAX_OMEGA_BLOBS;
		xOmegaBlobDist = xGoalDist / nOmegaBlobs;
		} 
	else if (nOmegaBlobs < MIN_OMEGA_BLOBS) {
		nOmegaBlobs = MIN_OMEGA_BLOBS;
		xOmegaBlobDist = xGoalDist / nOmegaBlobs;
		}
	}
vOmegaDelta = vGoal;
VmVecScale (&vOmegaDelta, xOmegaBlobDist);
//	Now, create all the blobs
vBlobPos = *vFiringPos;
nLastSeg = nFiringSeg;

//	If nearby, don't perturb vector.  If not nearby, start halfway out.
if (xGoalDist < MIN_OMEGA_DIST*4) {
	for (i=0; i<nOmegaBlobs; i++)
		xPerturbArray [i] = 0;
	} 
else {
	VmVecScaleInc (&vBlobPos, &vOmegaDelta, F1_0/2);	//	Put first blob half way out.
	for (i=0; i<nOmegaBlobs/2; i++) {
		xPerturbArray [i] = F1_0*i + F1_0/4;
		xPerturbArray [nOmegaBlobs-1-i] = F1_0*i;
		}
	}

//	Create random perturbation vector, but favor _not_ going up in player's reference.
MakeRandomVector (&vPerturb);
VmVecScaleInc (&vPerturb, &parentObjP->orient.uvec, -F1_0/2);
//bDoingLightingHack = 1;	//	Ugly, but prevents blobs which are probably outside the mine from killing framerate.
for (i=0; i<nOmegaBlobs; i++) {
	vms_vector	temp_pos;
	short			blob_objnum, segnum;

	//	This will put the last blob right at the destination object, causing damage.
	if (i == nOmegaBlobs-1)
		VmVecScaleInc (&vBlobPos, &vOmegaDelta, 15*F1_0/32);	//	Move last blob another (almost) half section
	//	Every so often, re-perturb blobs
	if ((i % 4) == 3) {
		vms_vector	temp_vec;

		MakeRandomVector (&temp_vec);
		VmVecScaleInc (&vPerturb, &temp_vec, F1_0/4);
		}
	VmVecScaleAdd (&temp_pos, &vBlobPos, &vPerturb, xPerturbArray [i]);
	segnum = FindSegByPoint (&temp_pos, nLastSeg);
	if (segnum != -1) {
		object		*objP;

		nLastSeg = segnum;
		blob_objnum = CreateObject (OBJ_WEAPON, OMEGA_ID, -1, segnum, &temp_pos, NULL, 0, 
											 CT_WEAPON, MT_PHYSICS, RT_WEAPON_VCLIP, 1);
		if (blob_objnum == -1)
			break;
		nLastCreatedObj = blob_objnum;
		objP = gameData.objs.objects + blob_objnum;
		objP->lifeleft = ONE_FRAME_TIME;
		objP->mtype.phys_info.velocity = vGoal;
		//	Only make the last one move fast, else multiple blobs might collide with target.
		VmVecScale (&objP->mtype.phys_info.velocity, F1_0*4);
		objP->size = gameData.weapons.info [objP->id].blob_size;
		objP->shields = FixMul (OMEGA_DAMAGE_SCALE*gameData.time.xFrame, WI_strength (objP->id,gameStates.app.nDifficultyLevel));
		objP->ctype.laser_info.parent_type			= parentObjP->type;
		objP->ctype.laser_info.parent_signature	= parentObjP->signature;
		objP->ctype.laser_info.parent_num			= OBJ_IDX (parentObjP);
		objP->movement_type = MT_NONE;	//	Only last one moves, that will get bashed below.
		}
	VmVecInc (&vBlobPos, &vOmegaDelta);
	}

	//	Make last one move faster, but it's already moving at speed = F1_0*4.
if (nLastCreatedObj != -1) {
	VmVecScale (
		&gameData.objs.objects [nLastCreatedObj].mtype.phys_info.velocity, 
		gameData.weapons.info [OMEGA_ID].speed [gameStates.app.nDifficultyLevel]/4);
	gameData.objs.objects [nLastCreatedObj].movement_type = MT_PHYSICS;
	}
bDoingLightingHack = 0;
}

// ---------------------------------------------------------------------------------
//	Call this every frame to recharge the Omega Cannon.
void OmegaChargeFrame (void)
{
	fix	xDeltaCharge, xOldOmegaCharge;

if (xOmegaCharge == MAX_OMEGA_CHARGE)
	return;

if (!(PlayerHasWeapon (OMEGA_INDEX, 0, -1) & HAS_WEAPON_FLAG))
	return;
if (gameStates.app.bPlayerIsDead)
	return;
if ((gameData.weapons.nPrimary == OMEGA_INDEX) && 
		!xOmegaCharge && 
		!gameData.multi.players [gameData.multi.nLocalPlayer].energy) {
	gameData.weapons.nPrimary--;
	AutoSelectWeapon (0, 1);
	}
//	Don't charge while firing.
if ((nLastOmegaFireFrame == gameData.app.nFrameCount) || 
		 (nLastOmegaFireFrame == gameData.app.nFrameCount-1))
	return;

if (gameData.multi.players [gameData.multi.nLocalPlayer].energy) {
	fix	xEnergyUsed;

	xOldOmegaCharge = xOmegaCharge;
	xOmegaCharge += gameData.time.xFrame/OMEGA_CHARGE_SCALE;
	if (xOmegaCharge > MAX_OMEGA_CHARGE)
		xOmegaCharge = MAX_OMEGA_CHARGE;
	xDeltaCharge = xOmegaCharge - xOldOmegaCharge;
	xEnergyUsed = FixMul (F1_0*190/17, xDeltaCharge);
	if (gameStates.app.nDifficultyLevel < 2)
		xEnergyUsed = FixMul (xEnergyUsed, i2f (gameStates.app.nDifficultyLevel+2)/4);

	gameData.multi.players [gameData.multi.nLocalPlayer].energy -= xEnergyUsed;
	if (gameData.multi.players [gameData.multi.nLocalPlayer].energy < 0)
		gameData.multi.players [gameData.multi.nLocalPlayer].energy = 0;
	}
}

// -- fix	Last_omega_muzzle_flash_time;

// ---------------------------------------------------------------------------------
//	*objP is the object firing the omega cannon
//	*pos is the location from which the omega bolt starts
void DoOmegaStuff (object *parentObjP, vms_vector *vFiringPos, object *weaponObjP)
{
	short			nLockObj, nFiringSeg;
	vms_vector	vGoalPos;
	int			pnum = parentObjP->id;

if (pnum == gameData.multi.nLocalPlayer) {
	//	If charge >= min, or (some charge and zero energy), allow to fire.
	if (!((xOmegaCharge >= MIN_OMEGA_CHARGE) || 
			 (xOmegaCharge && !gameData.multi.players [pnum].energy))) {
		ReleaseObject (OBJ_IDX (weaponObjP));
		return;
		}
	xOmegaCharge -= gameData.time.xFrame;
	if (xOmegaCharge < 0)
		xOmegaCharge = 0;
	//	Ensure that the lightning cannon can be fired next frame.
	xNextLaserFireTime = gameData.time.xGame+1;
	nLastOmegaFireFrame = gameData.app.nFrameCount;
	}

weaponObjP->ctype.laser_info.parent_type = OBJ_PLAYER;
weaponObjP->ctype.laser_info.parent_num = gameData.multi.players [pnum].objnum;
weaponObjP->ctype.laser_info.parent_signature = gameData.objs.objects [gameData.multi.players [pnum].objnum].signature;

if (gameStates.limitFPS.bOmega && !gameStates.app.b40fpsTick)
	nLockObj = -1;
else
	nLockObj = FindHomingObject (vFiringPos, weaponObjP);
if (0 > (nFiringSeg = FindSegByPoint (vFiringPos, parentObjP->segnum)))
	return;

//	Play sound.
if (parentObjP == gameData.objs.viewer)
	DigiPlaySample (gameData.weapons.info [weaponObjP->id].flash_sound, F1_0);
else
	DigiLinkSoundToPos (gameData.weapons.info [weaponObjP->id].flash_sound, weaponObjP->segnum, 0, &weaponObjP->pos, 0, F1_0);

// -- if ((Last_omega_muzzle_flash_time + F1_0/4 < gameData.time.xGame) || (Last_omega_muzzle_flash_time > gameData.time.xGame)) {
// -- 	DoMuzzleStuff (nFiringSeg, vFiringPos);
// -- 	Last_omega_muzzle_flash_time = gameData.time.xGame;
// -- }

//	Delete the original object.  Its only purpose in life was to determine which object to home in on.
ReleaseObject (OBJ_IDX (weaponObjP));
if (nLockObj != -1)
	vGoalPos = gameData.objs.objects [nLockObj].pos;
else {	//	If couldn't lock on anything, fire straight ahead.
	fvi_query	fq;
	fvi_info		hit_data;
	int			fate;
	vms_vector	vPerturb, perturbed_fvec;

	MakeRandomVector (&vPerturb);
	VmVecScaleAdd (&perturbed_fvec, &parentObjP->orient.fvec, &vPerturb, F1_0/16);
	VmVecScaleAdd (&vGoalPos, vFiringPos, &perturbed_fvec, MAX_OMEGA_DIST);
	fq.startSeg = nFiringSeg;
	fq.p0 = vFiringPos;
	fq.p1	= &vGoalPos;
	fq.rad = 0;
	fq.thisObjNum = OBJ_IDX (parentObjP);
	fq.ignoreObjList = NULL;
	fq.flags = FQ_IGNORE_POWERUPS | FQ_TRANSPOINT | FQ_CHECK_OBJS;		//what about trans walls???
	fate = FindVectorIntersection (&fq, &hit_data);
	if (fate != HIT_NONE) {
		Assert (hit_data.hit.nSegment != -1);		//	How can this be?  We went from inside the mine to outside without hitting anything?
		vGoalPos = hit_data.hit.vPoint;
		}
	}
//	This is where we create a pile of omega blobs!
CreateOmegaBlobs (nFiringSeg, vFiringPos, &vGoalPos, parentObjP);
}

// ---------------------------------------------------------------------------------

// Initializes a laser after Fire is pressed 
//	Returns object number.
int CreateNewLaser (
	vms_vector * direction, 
	vms_vector * position, 
	short segnum, 
	short parent, 
	ubyte weapon_type, 
	int make_sound)
{
	int objnum;
	object *objP;
	fix parent_speed, weapon_speed;
	fix volume;
	fix laser_length=0;
	Assert (weapon_type < gameData.weapons.nTypes [0]);

	if (weapon_type>=gameData.weapons.nTypes [0])
		weapon_type = 0;

	//	Don't let homing blobs make muzzle flash.
	if (gameData.objs.objects [parent].type == OBJ_ROBOT)
		DoMuzzleStuff (segnum, position);
	else if (gameStates.app.bD2XLevel && 
				 (gameData.objs.objects + parent == gameData.objs.console) && 
				 (gameData.segs.segment2s [gameData.objs.console->segnum].special == SEGMENT_IS_NODAMAGE))
		return -1;
#if 1
	if ((parent == gameData.multi.players [gameData.multi.nLocalPlayer].objnum) &&
		 (weapon_type == PROXIMITY_ID) && 
		 (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
		objnum = CreateObject (OBJ_POWERUP, POW_HOARD_ORB, -1, segnum, position, &vmdIdentityMatrix, 
									  gameData.objs.pwrUp.info [POW_HOARD_ORB].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP, 1);
		if (objnum >= 0) {
			objP = gameData.objs.objects + objnum;
			if (gameData.app.nGameMode & GM_MULTI)
				multiData.create.nObjNums [multiData.create.nLoc++] = objnum;
			objP->rtype.vclip_info.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
			objP->rtype.vclip_info.xFrameTime = gameData.eff.vClips [0] [objP->rtype.vclip_info.nClipIndex].xFrameTime;
			objP->rtype.vclip_info.nCurFrame = 0;
			objP->matcen_creator = GetTeam (gameData.multi.nLocalPlayer) + 1;
			}
		return -1;
		}
#endif
	objnum = CreateWeaponObject (weapon_type, segnum, position);

	if (objnum < 0) {
		return -1;
	}
	objP = gameData.objs.objects + objnum;
	//	Do the special Omega Cannon stuff.  Then return on account of everything that follows does
	//	not apply to the Omega Cannon.
	if (weapon_type == OMEGA_ID) {
		// Create orientation matrix for tracking purposes.
		VmVector2Matrix (&objP->orient, direction, &gameData.objs.objects [parent].orient.uvec ,NULL);

		if ((&gameData.objs.objects [parent] != gameData.objs.viewer) &&
			 (gameData.objs.objects [parent].type != OBJ_WEAPON))	{
			// Muzzle flash		
			if (gameData.weapons.info [objP->id].flash_vclip > -1)
				ObjectCreateMuzzleFlash (objP->segnum, &objP->pos, gameData.weapons.info [objP->id].flash_size, 
												 gameData.weapons.info [objP->id].flash_vclip);
		}
		DoOmegaStuff (gameData.objs.objects + parent, position, objP);
		return objnum;
	}

	if (gameData.objs.objects [parent].type == OBJ_PLAYER) {
		if (weapon_type == FUSION_ID) {

			if (gameData.app.fusion.xCharge <= 0)
				objP->ctype.laser_info.multiplier = F1_0;
			else if (gameData.app.fusion.xCharge <= 4*F1_0)
				objP->ctype.laser_info.multiplier = F1_0 + gameData.app.fusion.xCharge/2;
			else
				objP->ctype.laser_info.multiplier = 4*F1_0;

		} else if (/* (weapon_type >= LASER_ID) &&*/ (weapon_type <= MAX_SUPER_LASER_LEVEL) && 
					 (gameData.multi.players [gameData.objs.objects [parent].id].flags & PLAYER_FLAGS_QUAD_LASERS))
			objP->ctype.laser_info.multiplier = F1_0*3/4;
		else if (weapon_type == GUIDEDMISS_ID) {
			if (parent==gameData.multi.players [gameData.multi.nLocalPlayer].objnum) {
				gameData.objs.guidedMissile [gameData.multi.nLocalPlayer]= objP;
				gameData.objs.guidedMissileSig [gameData.multi.nLocalPlayer] = objP->signature;
				if (gameData.demo.nState==ND_STATE_RECORDING)
					NDRecordGuidedStart ();
			}
		}
	}

	//	Make children of smart bomb bounce so if they hit a wall right away, they
	//	won't detonate.  The frame interval code will clear this bit after 1/2 second.
	if ((weapon_type == PLAYER_SMART_HOMING_ID) || 
		 (weapon_type == SMART_MINE_HOMING_ID) || 
		 (weapon_type == ROBOT_SMART_HOMING_ID) || 
		 (weapon_type == ROBOT_SMART_MINE_HOMING_ID) || 
		 (weapon_type == EARTHSHAKER_MEGA_ID))
		objP->mtype.phys_info.flags |= PF_BOUNCE;
//CBRK (weapon_type == ROBOT_MERCURY_ID);
	if (gameData.weapons.info [weapon_type].render_type == WEAPON_RENDER_POLYMODEL)
		laser_length = gameData.models.polyModels [objP->rtype.pobj_info.model_num].rad * 2;

	if (weapon_type == FLARE_ID)
		objP->mtype.phys_info.flags |= PF_STICK;		//this obj sticks to walls
	
	objP->shields = WI_strength (objP->id,gameStates.app.nDifficultyLevel);
	
	// Fill in laser-specific data

	objP->lifeleft									= WI_lifetime (objP->id);
	objP->ctype.laser_info.parent_type		= gameData.objs.objects [parent].type;
	objP->ctype.laser_info.parent_signature = gameData.objs.objects [parent].signature;
	objP->ctype.laser_info.parent_num			= parent;

	//	Assign parent type to highest level creator.  This propagates parent type down from
	//	the original creator through weapons which create children of their own (ie, smart missile)
	if (gameData.objs.objects [parent].type == OBJ_WEAPON) {
		int	highest_parent = parent;
		int	count;
		count = 0;
		while ((count++ < 10) && (gameData.objs.objects [highest_parent].type == OBJ_WEAPON)) {
			int	next_parent;

			next_parent = gameData.objs.objects [highest_parent].ctype.laser_info.parent_num;
			if (gameData.objs.objects [next_parent].signature != gameData.objs.objects [highest_parent].ctype.laser_info.parent_signature)
				break;	//	Probably means parent was killed.  Just continue.
			if (next_parent == highest_parent) {
				Int3 ();	//	Hmm, object is parent of itself.  This would seem to be bad, no?
				break;
			}
			highest_parent = next_parent;
			objP->ctype.laser_info.parent_num			= highest_parent;
			objP->ctype.laser_info.parent_type		= gameData.objs.objects [highest_parent].type;
			objP->ctype.laser_info.parent_signature = gameData.objs.objects [highest_parent].signature;
		}
	}
	// Create orientation matrix so we can look from this pov
	//	Homing missiles also need an orientation matrix so they know if they can make a turn.
	if ((objP->render_type == RT_POLYOBJ) || (WI_homing_flag (objP->id)))
		VmVector2Matrix (&objP->orient,direction, &gameData.objs.objects [parent].orient.uvec ,NULL);

	if (((gameData.objs.objects + parent) != gameData.objs.viewer) && 
		 (gameData.objs.objects [parent].type != OBJ_WEAPON))	{
		// Muzzle flash		
		if (gameData.weapons.info [objP->id].flash_vclip > -1)
			ObjectCreateMuzzleFlash (objP->segnum, &objP->pos, gameData.weapons.info [objP->id].flash_size, 
											 gameData.weapons.info [objP->id].flash_vclip);
	}

	volume = F1_0;
	if (gameData.weapons.info [objP->id].flash_sound > -1)	{
		if (make_sound)	{
			if (parent == (OBJ_IDX (gameData.objs.viewer)))	{
				if (weapon_type == VULCAN_ID)	// Make your own vulcan gun  1/2 as loud.
					volume = F1_0 / 2;
				DigiPlaySample (gameData.weapons.info [objP->id].flash_sound, volume);
			} else {
				DigiLinkSoundToPos (gameData.weapons.info [objP->id].flash_sound, objP->segnum, 0, &objP->pos, 0, volume);
			}
		}
	}

	//	Fire the laser from the gun tip so that the back end of the laser bolt is at the gun tip.
	// Move 1 frame, so that the end-tip of the laser is touching the gun barrel.
	// This also jitters the laser a bit so that it doesn't alias.
	//	Don't do for weapons created by weapons.
	if ((gameData.objs.objects [parent].type == OBJ_PLAYER) && (gameData.weapons.info [weapon_type].render_type != WEAPON_RENDER_NONE) && (weapon_type != FLARE_ID)) {
		vms_vector	end_pos;
		int			end_segnum;

	 	VmVecScaleAdd (&end_pos, &objP->pos, direction, nLaserOffset+ (laser_length/2));
		end_segnum = FindSegByPoint (&end_pos, objP->segnum);
		if (end_segnum != objP->segnum) {
			if (end_segnum != -1) {
				objP->pos = end_pos;
				RelinkObject (OBJ_IDX (objP), end_segnum);
			} else
				;
		} else
			objP->pos = end_pos;
	}

	//	Here's where to fix the problem with gameData.objs.objects which are moving backwards imparting higher velocity to their weaponfire.
	//	Find out if moving backwards.
	if ((weapon_type == PROXIMITY_ID) || (weapon_type == SUPERPROX_ID)) {
		parent_speed = VmVecMagQuick (&gameData.objs.objects [parent].mtype.phys_info.velocity);
		if (VmVecDot (&gameData.objs.objects [parent].mtype.phys_info.velocity, &gameData.objs.objects [parent].orient.fvec) < 0)
			parent_speed = -parent_speed;
		} 
	else
		parent_speed = 0;

	weapon_speed = WI_speed (objP->id,gameStates.app.nDifficultyLevel);
	if (gameData.weapons.info [objP->id].speedvar != 128) {
		fix	randval;

		//	Get a scale factor between speedvar% and 1.0.
		randval = F1_0 - ((d_rand () * gameData.weapons.info [objP->id].speedvar) >> 6);
		weapon_speed = FixMul (weapon_speed, randval);
	}

	//	Ugly hack (too bad we're on a deadline), for homing missiles dropped by smart bomb, start them out slower.
	if ((objP->id == PLAYER_SMART_HOMING_ID) || 
		 (objP->id == SMART_MINE_HOMING_ID) || 
		 (objP->id == ROBOT_SMART_HOMING_ID) || 
		 (objP->id == ROBOT_SMART_MINE_HOMING_ID) || 
		 (objP->id == EARTHSHAKER_MEGA_ID))
		weapon_speed /= 4;

	if (WI_thrust (objP->id) != 0)
		weapon_speed /= 2;

	VmVecCopyScale (&objP->mtype.phys_info.velocity, direction, weapon_speed + parent_speed);

	//	Set thrust 
	if (WI_thrust (weapon_type) != 0) {
		objP->mtype.phys_info.thrust = objP->mtype.phys_info.velocity;
		VmVecScale (
			&objP->mtype.phys_info.thrust, 
			FixDiv (WI_thrust (objP->id), weapon_speed+parent_speed));
	}

	if ((objP->type == OBJ_WEAPON) && (objP->id == FLARE_ID))
		objP->lifeleft += (d_rand ()-16384) << 2;		//	add in -2..2 seconds

	return objnum;
}

//	-----------------------------------------------------------------------------------------------------------
//	Calls CreateNewLaser, but takes care of the segment and point computation for you.
int CreateNewLaserEasy (vms_vector * direction, vms_vector * position, short parent, ubyte weapon_type, int make_sound)
{
	fvi_query	fq;
	fvi_info		hit_data;
	object		*parentObjP = &gameData.objs.objects [parent];
	int			fate;

	//	Find segment containing laser fire position.  If the robot is straddling a segment, the position from
	//	which it fires may be in a different segment, which is bad news for FindVectorIntersection.  So, cast
	//	a ray from the object center (whose segment we know) to the laser position.  Then, in the call to CreateNewLaser
	//	use the data returned from this call to FindVectorIntersection.
	//	Note that while FindVectorIntersection is pretty slow, it is not terribly slow if the destination point is
	//	in the same segment as the source point.

	fq.p0						= &parentObjP->pos;
	fq.startSeg				= parentObjP->segnum;
	fq.p1						= position;
	fq.rad					= 0;
	fq.thisObjNum			= OBJ_IDX (parentObjP);
	fq.ignoreObjList	= NULL;
	fq.flags					= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???

	fate = FindVectorIntersection (&fq, &hit_data);
	if (fate != HIT_NONE  || hit_data.hit.nSegment==-1) {
		return -1;
	}

	return CreateNewLaser (direction, &hit_data.hit.vPoint, (short) hit_data.hit.nSegment, parent, weapon_type, make_sound);

}

//	-----------------------------------------------------------------------------------------------------------
//	Determine if two gameData.objs.objects are on a line of sight.  If so, return true, else return false.
//	Calls fvi.
int ObjectToObjectVisibility (object *objP1, object *objP2, int trans_type)
{
	fvi_query	fq;
	fvi_info		hit_data;
	int			fate;

fq.p0					= &objP1->pos;
fq.startSeg			= objP1->segnum;
fq.p1					= &objP2->pos;
fq.rad				= 0x10;
fq.thisObjNum		= OBJ_IDX (objP1);
fq.ignoreObjList	= NULL;
fq.flags				= trans_type;
fate = FindVectorIntersection (&fq, &hit_data);
if (fate == HIT_WALL)
	return 0;
else if (fate == HIT_NONE)
	return 1;
else
	Int3 ();		//	Contact Mike: Oops, what happened?  What is fate?
					// 2 = hit object (impossible), 3 = bad starting point (bad)
return 0;
}

fix	xMinTrackableDot = MIN_TRACKABLE_DOT;

//	-----------------------------------------------------------------------------------------------------------
//	Return true if weapon *tracker is able to track object gameData.objs.objects [nTrackGoal], else return false.
//	In order for the object to be trackable, it must be within a reasonable turning radius for the missile
//	and it must not be obstructed by a wall.
int object_is_trackable (int nTrackGoal, object *tracker, fix *xDot)
{
	vms_vector	vGoal;
	object		*objP;

if (nTrackGoal == -1)
	return 0;

if (gameData.app.nGameMode & GM_MULTI_COOP)
	return 0;
objP = gameData.objs.objects + nTrackGoal;
//	Don't track player if he's cloaked.
if ((nTrackGoal == gameData.multi.players [gameData.multi.nLocalPlayer].objnum) && 
	 (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED))
	return 0;
//	Can't track AI object if he's cloaked.
if (objP->type == OBJ_ROBOT) {
	if (objP->ctype.ai_info.CLOAKED)
		return 0;
	//	Your missiles don't track your escort.
	if (gameData.bots.pInfo [objP->id].companion && 
		 (tracker->ctype.laser_info.parent_type == OBJ_PLAYER))
		return 0;
	}
VmVecSub (&vGoal, &objP->pos, &tracker->pos);
VmVecNormalizeQuick (&vGoal);
*xDot = VmVecDot (&vGoal, &tracker->orient.fvec);
if ((*xDot < xMinTrackableDot) && (*xDot > F1_0*9/10)) {
	VmVecNormalize (&vGoal);
	*xDot = VmVecDot (&vGoal, &tracker->orient.fvec);
	}

if (*xDot >= xMinTrackableDot) {
	//	xDot is in legal range, now see if object is visible
	return ObjectToObjectVisibility (tracker, objP, FQ_TRANSWALL);
	}
return 0;
}

//	--------------------------------------------------------------------------------------------
int call_find_homing_object_complete (object *tracker, vms_vector *curpos)
{
	if (gameData.app.nGameMode & GM_MULTI) {
		if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER) {
			//	It's fired by a player, so if robots present, track robot, else track player.
			if (gameData.app.nGameMode & GM_MULTI_COOP)
				return FindHomingObjectComplete (curpos, tracker, OBJ_ROBOT, -1);
			else
				return FindHomingObjectComplete (curpos, tracker, OBJ_PLAYER, OBJ_ROBOT);
		} else {
			int	goal2_type = -1;

			if (gameStates.app.cheats.bRobotsKillRobots)
				goal2_type = OBJ_ROBOT;
			Assert (tracker->ctype.laser_info.parent_type == OBJ_ROBOT);
			return FindHomingObjectComplete (curpos, tracker, OBJ_PLAYER, goal2_type);
		}		
	} else
		return FindHomingObjectComplete (curpos, tracker, OBJ_ROBOT, -1);
}

//	--------------------------------------------------------------------------------------------
//	Find object to home in on.
//	Scan list of gameData.objs.objects rendered last frame, find one that satisfies function of nearness to center and distance.
int FindHomingObject (vms_vector *curpos, object *tracker)
{
	int	i;
	fix	max_dot = -F1_0*2;
	int	best_objnum = -1;

	//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser type??
	Assert ((WI_homing_flag (tracker->id)) || (tracker->id == OMEGA_ID));

	//	Find an object to track based on game mode (eg, whether in network play) and who fired it.

	if (gameData.app.nGameMode & GM_MULTI)
		return call_find_homing_object_complete (tracker, curpos);
	else {
		int	cur_min_trackable_dot;

		cur_min_trackable_dot = MIN_TRACKABLE_DOT;
		if ((tracker->type == OBJ_WEAPON) && (tracker->id == OMEGA_ID))
			cur_min_trackable_dot = OMEGA_MIN_TRACKABLE_DOT;

		//	Not in network mode.  If not fired by player, then track player.
		if (tracker->ctype.laser_info.parent_num != gameData.multi.players [gameData.multi.nLocalPlayer].objnum) {
			if (!(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED))
				best_objnum = OBJ_IDX (gameData.objs.console);
		} else {
			int	window_num = -1;
			fix	dist, max_trackable_dist;

			//	Find the window which has the forward view.
			for (i=0; i<MAX_RENDERED_WINDOWS; i++)
				if (Window_rendered_data [i].frame >= gameData.app.nFrameCount-1)
					if (Window_rendered_data [i].viewer == gameData.objs.console)
						if (!Window_rendered_data [i].rear_view) {
							window_num = i;
							break;
						}

			//	Couldn't find suitable view from this frame, so do complete search.
			if (window_num == -1) {
				return call_find_homing_object_complete (tracker, curpos);
			}

			max_trackable_dist = MAX_TRACKABLE_DIST;
			if (tracker->id == OMEGA_ID)
				max_trackable_dist = OMEGA_MAX_TRACKABLE_DIST;

			//	Not in network mode and fired by player.
			for (i=Window_rendered_data [window_num].num_objects-1; i>=0; i--) {
				fix			dot; //, dist;
				vms_vector	vecToCurObj;
				int			objnum = Window_rendered_data [window_num].rendered_objects [i];
				object		*curObjP = &gameData.objs.objects [objnum];

				if (objnum == gameData.multi.players [gameData.multi.nLocalPlayer].objnum)
					continue;

				//	Can't track AI object if he's cloaked.
				if (curObjP->type == OBJ_ROBOT) {
					if (curObjP->ctype.ai_info.CLOAKED)
						continue;

					//	Your missiles don't track your escort.
					if (gameData.bots.pInfo [curObjP->id].companion)
						if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER)
							continue;
				}

				VmVecSub (&vecToCurObj, &curObjP->pos, curpos);
				dist = VmVecNormalizeQuick (&vecToCurObj);
				if (dist < max_trackable_dist) {
					dot = VmVecDot (&vecToCurObj, &tracker->orient.fvec);

					//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
					//	to determine if an object is initially trackable.  FindHomingObject is called on subsequent
					//	frames to determine if the object remains trackable.
					if (dot > cur_min_trackable_dot) {
						if (dot > max_dot) {
							if (ObjectToObjectVisibility (tracker, &gameData.objs.objects [objnum], FQ_TRANSWALL)) {
								max_dot = dot;
								best_objnum = objnum;
							}
						}
					} else if (dot > F1_0 - (F1_0 - cur_min_trackable_dot)*2) {
						VmVecNormalize (&vecToCurObj);
						dot = VmVecDot (&vecToCurObj, &tracker->orient.fvec);
						if (dot > cur_min_trackable_dot) {
							if (dot > max_dot) {
								if (ObjectToObjectVisibility (tracker, &gameData.objs.objects [objnum], FQ_TRANSWALL)) {
									max_dot = dot;
									best_objnum = objnum;
								}
							}
						}
					}
				}
			}
		}
	}
return best_objnum;
}

//	--------------------------------------------------------------------------------------------
//	Find object to home in on.
//	Scan list of gameData.objs.objects rendered last frame, find one that satisfies function of nearness to center and distance.
//	Can track two kinds of gameData.objs.objects.  If you are only interested in one type, set track_obj_type2 to NULL
//	Always track proximity bombs.  --MK, 06/14/95
//	Make homing gameData.objs.objects not track parent's prox bombs.
int FindHomingObjectComplete (vms_vector *curpos, object *tracker, int track_obj_type1, int track_obj_type2)
{
	int	objnum;
	fix	max_dot = -F1_0*2;
	int	best_objnum = -1;
	fix	max_trackable_dist;
	fix	min_trackable_dot;

	//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser type??
	Assert ((WI_homing_flag (tracker->id)) || (tracker->id == OMEGA_ID));

	max_trackable_dist = MAX_TRACKABLE_DIST;
	min_trackable_dot = MIN_TRACKABLE_DOT;

	if (tracker->id == OMEGA_ID) {
		max_trackable_dist = OMEGA_MAX_TRACKABLE_DIST;
		min_trackable_dot = OMEGA_MIN_TRACKABLE_DOT;
	}

	for (objnum=0; objnum<=gameData.objs.nLastObject; objnum++) {
		int			is_proximity = 0;
		fix			dot, dist;
		vms_vector	vecToCurObj;
		object		*curObjP = &gameData.objs.objects [objnum];

		if ((curObjP->type != track_obj_type1) && (curObjP->type != track_obj_type2))
		{
			if ((curObjP->type == OBJ_WEAPON) && ((curObjP->id == PROXIMITY_ID) || (curObjP->id == SUPERPROX_ID))) {
				if (curObjP->ctype.laser_info.parent_signature != tracker->ctype.laser_info.parent_signature)
					is_proximity = 1;
				else
					continue;
			} else
				continue;
		}

		if (objnum == tracker->ctype.laser_info.parent_num) // Don't track shooter
			continue;

		//	Don't track cloaked players.
		if (curObjP->type == OBJ_PLAYER)
		{
			if (gameData.multi.players [curObjP->id].flags & PLAYER_FLAGS_CLOAKED)
				continue;
			// Don't track teammates in team games
			#ifdef NETWORK
			if ((gameData.app.nGameMode & GM_TEAM) && (gameData.objs.objects [tracker->ctype.laser_info.parent_num].type == OBJ_PLAYER) && (GetTeam (curObjP->id) == GetTeam (gameData.objs.objects [tracker->ctype.laser_info.parent_num].id)))
				continue;
			#endif
		}

		//	Can't track AI object if he's cloaked.
		if (curObjP->type == OBJ_ROBOT) {
			if (curObjP->ctype.ai_info.CLOAKED)
				continue;

			//	Your missiles don't track your escort.
			if (gameData.bots.pInfo [curObjP->id].companion)
				if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER)
					continue;
		}

		VmVecSub (&vecToCurObj, &curObjP->pos, curpos);
		dist = VmVecMagQuick (&vecToCurObj);

		if (dist < max_trackable_dist) {
			VmVecNormalizeQuick (&vecToCurObj);
			dot = VmVecDot (&vecToCurObj, &tracker->orient.fvec);
			if (is_proximity)
				dot = ((dot << 3) + dot) >> 3;		//	I suspect Watcom would be too stupid to figure out the obvious...

			//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
			//	to determine if an object is initially trackable.  FindHomingObject is called on subsequent
			//	frames to determine if the object remains trackable.
			if (dot > min_trackable_dot) {
				if (dot > max_dot) {
					if (ObjectToObjectVisibility (tracker, &gameData.objs.objects [objnum], FQ_TRANSWALL)) {
						max_dot = dot;
						best_objnum = objnum;
					}
				}
			}
		}
	}
	return best_objnum;
}

//	------------------------------------------------------------------------------------------------------------
//	See if legal to keep tracking currently tracked object.  If not, see if another object is trackable.  If not, return -1,
//	else return object number of tracking object.
//	Computes and returns a fairly precise dot product.
int track_track_goal (int nTrackGoal, object *tracker, fix *dot)
{
	//	Every 8 frames for each object, scan all gameData.objs.objects.
	if (object_is_trackable (nTrackGoal, tracker, dot) && (((OBJ_IDX (tracker) ^ gameData.app.nFrameCount) % 8) != 0)) {
		return nTrackGoal;
	} else if (((OBJ_IDX (tracker) ^ gameData.app.nFrameCount) % 4) == 0) {
		int	rval = -2;

		//	If player fired missile, then search for an object, if not, then give up.
		if (gameData.objs.objects [tracker->ctype.laser_info.parent_num].type == OBJ_PLAYER) {
			int	goal_type;

			if (nTrackGoal == -1) 
			{
				if (gameData.app.nGameMode & GM_MULTI)
				{
					if (gameData.app.nGameMode & GM_MULTI_COOP)
						rval = FindHomingObjectComplete (&tracker->pos, tracker, OBJ_ROBOT, -1);
					else if (gameData.app.nGameMode & GM_MULTI_ROBOTS)		//	Not cooperative, if robots, track either robot or player
						rval = FindHomingObjectComplete (&tracker->pos, tracker, OBJ_PLAYER, OBJ_ROBOT);
					else		//	Not cooperative and no robots, track only a player
						rval = FindHomingObjectComplete (&tracker->pos, tracker, OBJ_PLAYER, -1);
				}
				else
					rval = FindHomingObjectComplete (&tracker->pos, tracker, OBJ_PLAYER, OBJ_ROBOT);
			} 
			else 
			{
				goal_type = gameData.objs.objects [tracker->ctype.laser_info.track_goal].type;
				if ((goal_type == OBJ_PLAYER) || (goal_type == OBJ_ROBOT))
					rval = FindHomingObjectComplete (&tracker->pos, tracker, goal_type, -1);
				else
					rval = -1;
			}
		} 
		else {
			int	goal_type, goal2_type = -1;

			if (gameStates.app.cheats.bRobotsKillRobots)
				goal2_type = OBJ_ROBOT;

			if (nTrackGoal == -1)
				rval = FindHomingObjectComplete (&tracker->pos, tracker, OBJ_PLAYER, goal2_type);
			else {
				goal_type = gameData.objs.objects [tracker->ctype.laser_info.track_goal].type;
				rval = FindHomingObjectComplete (&tracker->pos, tracker, goal_type, goal2_type);
			}
		}

		Assert (rval != -2);		//	This means it never got set which is bad! Contact Mike.
		return rval;
	}
	return -1;
}

//-------------- Initializes a laser after Fire is pressed -----------------

int LaserPlayerFireSpreadDelay (
	object *objP, 
	ubyte laser_type, 
	int gun_num, 
	fix spreadr, 
	fix spreadu, 
	fix delay_time, 
	int make_sound, 
	int harmless)
{
	short			LaserSeg;
	int			Fate; 
	vms_vector	LaserPos, LaserDir, pnt;
	fvi_query	fq;
	fvi_info		hit_data;
	vms_vector	gun_point;
	vms_matrix	m;
	int			objnum;
#if FULL_COCKPIT_OFFS
	int bLaserOffs = ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) && (OBJ_IDX (objP) == 
							gameData.multi.players [gameData.multi.nLocalPlayer].objnum));
#else
	int bLaserOffs = 0;
#endif
	CreateAwarenessEvent (objP, PA_WEAPON_WALL_COLLISION);
	// Find the initial position of the laser
	pnt = gameData.pig.ship.player->gun_points [gun_num];
	if (bLaserOffs)
		VmVecScaleInc (&pnt, &objP->orient.uvec, LASER_OFFS);
	VmCopyTransposeMatrix (&m, &objP->orient);
	VmVecRotate (&gun_point, &pnt, &m);
	memcpy (&m, &objP->orient, sizeof (vms_matrix));
	VmVecAdd (&LaserPos, &objP->pos, &gun_point);
	//	If supposed to fire at a delayed time (delay_time), then move this point backwards.
	if (delay_time)
		VmVecScaleInc (&LaserPos, &m.fvec, -FixMul (delay_time, WI_speed (laser_type,gameStates.app.nDifficultyLevel)));

//	DoMuzzleStuff (objP, &Pos);

	//--------------- Find LaserPos and LaserSeg ------------------
	fq.p0						= &objP->pos;
	fq.startSeg				= objP->segnum;
	fq.p1						= &LaserPos;
	fq.rad					= 0x10;
	fq.thisObjNum			= OBJ_IDX (objP);
	fq.ignoreObjList	= NULL;
	fq.flags					= FQ_CHECK_OBJS | FQ_IGNORE_POWERUPS;
	Fate = FindVectorIntersection (&fq, &hit_data);
	LaserSeg = hit_data.hit.nSegment;
	if (LaserSeg == -1) {	//some sort of annoying error
		return -1;
		}
	//SORT OF HACK... IF ABOVE WAS CORRECT THIS WOULDNT BE NECESSARY.
	if (VmVecDistQuick (&LaserPos, &objP->pos) > 0x50000) {
		return -1;
		}
	if (Fate==HIT_WALL)  {
		return -1;
		}
	if (Fate==HIT_OBJECT) {
//		if (gameData.objs.objects [hit_data.hit_object].type == OBJ_ROBOT)
//			gameData.objs.objects [hit_data.hit_object].flags |= OF_SHOULD_BE_DEAD;
//		if (gameData.objs.objects [hit_data.hit_object].type != OBJ_POWERUP)
//			return;		
	//as of 12/6/94, we don't care if the laser is stuck in an object. We
	//just fire away normally
		}

	//	Now, make laser spread out.
	LaserDir = m.fvec;
	if (spreadr || spreadu) {
		VmVecScaleInc (&LaserDir, &m.rvec, spreadr);
		VmVecScaleInc (&LaserDir, &m.uvec, spreadu);
	}
	if (bLaserOffs)
		VmVecScaleInc (&LaserDir, &m.uvec, LASER_OFFS);
	objnum = CreateNewLaser (&LaserDir, &LaserPos, LaserSeg, OBJ_IDX (objP), laser_type, make_sound);
	//	Omega cannon is a hack, not surprisingly.  Don't want to do the rest of this stuff.
	if (laser_type == OMEGA_ID)
		return -1;
	if (objnum == -1) {
		return -1;
		}
#ifdef NETWORK
	if (laser_type==GUIDEDMISS_ID && multiData.bIsGuided) {
		gameData.objs.guidedMissile [objP->id]=gameData.objs.objects + objnum;
	}

	multiData.bIsGuided=0;
#endif

	if (laser_type == CONCUSSION_ID ||
		 laser_type == HOMING_ID ||
		 laser_type == SMART_ID ||
		 laser_type == MEGA_ID ||
		 laser_type == FLASH_ID ||
		 //laser_type == GUIDEDMISS_ID ||
		 //laser_type == SUPERPROX_ID ||
		 laser_type == MERCURY_ID ||
		 laser_type == EARTHSHAKER_ID)
		if (gameData.objs.missileViewer == NULL && objP->id==gameData.multi.nLocalPlayer)
			gameData.objs.missileViewer = gameData.objs.objects + objnum;

	//	If this weapon is supposed to be silent, set that bit!
	if (!make_sound)
		gameData.objs.objects [objnum].flags |= OF_SILENT;

	//	If this weapon is supposed to be silent, set that bit!
	if (harmless)
		gameData.objs.objects [objnum].flags |= OF_HARMLESS;

	//	If the object firing the laser is the player, then indicate the laser object so robots can dodge.
	//	New by MK on 6/8/95, don't let robots evade proximity bombs, thereby decreasing uselessness of bombs.
	if ((objP == gameData.objs.console) && 
		 ((gameData.objs.objects [objnum].id != PROXIMITY_ID) && 
		 (gameData.objs.objects [objnum].id != SUPERPROX_ID)))
		gameStates.app.bPlayerFiredLaserThisFrame = objnum;

	if (gameData.weapons.info [laser_type].homing_flag) {
		if (objP == gameData.objs.console) {
			gameData.objs.objects [objnum].ctype.laser_info.track_goal = 
				FindHomingObject (&LaserPos, gameData.objs.objects + objnum);
			#ifdef NETWORK
			multiData.laser.nTrack = gameData.objs.objects [objnum].ctype.laser_info.track_goal;
			#endif
		}
		#ifdef NETWORK
		else // Some other player shot the homing thing
		{
			Assert (gameData.app.nGameMode & GM_MULTI);
			gameData.objs.objects [objnum].ctype.laser_info.track_goal = multiData.laser.nTrack;
		}
		#endif
	}
return objnum;
}

//	-----------------------------------------------------------------------------------------------------------
void CreateFlare (object *objP)
{
	fix	energy_usage;

	energy_usage = WI_energy_usage (FLARE_ID);

	if (gameStates.app.nDifficultyLevel < 2)
		energy_usage = FixMul (energy_usage, i2f (gameStates.app.nDifficultyLevel+2)/4);

//	MK, 11/04/95: Allowed to fire flare even if no energy.
// -- 	if (gameData.multi.players [gameData.multi.nLocalPlayer].energy >= energy_usage) {
		gameData.multi.players [gameData.multi.nLocalPlayer].energy -= energy_usage;

		if (gameData.multi.players [gameData.multi.nLocalPlayer].energy <= 0) {
			gameData.multi.players [gameData.multi.nLocalPlayer].energy = 0;	
			// -- AutoSelectWeapon (0);
		}

		LaserPlayerFire (objP, FLARE_ID, 6, 1, 0);

		#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI) {
			multiData.laser.bFired = 1;
			multiData.laser.nGun = FLARE_ADJUST;
			multiData.laser.nFlags = 0;
			multiData.laser.nLevel = 0;
		}
		#endif
// -- 	}

}

//-------------------------------------------------------------------------------------------
//	Set object *objP's orientation to (or towards if I'm ambitious) its velocity.
void HomingMissileTurnTowardsVelocity (object *objP, vms_vector *norm_vel)
{
	vms_vector	new_fvec;
	fix frame_time;

if (!gameStates.limitFPS.bHomers || gameOpts->legacy.bHomers)
	frame_time = gameData.time.xFrame;
else {
	if (!gameStates.app.b40fpsTick)
		return;
	else {
		int fps = (int) ((1000 + gameStates.app.nDeltaTime / 2) / gameStates.app.nDeltaTime);
		frame_time = fps ? (f1_0 + fps / 2) / fps : f1_0;
		}
	}
new_fvec = *norm_vel;
VmVecScale (&new_fvec, /*gameData.time.xFrame*/ frame_time * HOMING_MISSILE_SCALE);
VmVecInc (&new_fvec, &objP->orient.fvec);
VmVecNormalizeQuick (&new_fvec);
//	if ((norm_vel->x == 0) && (norm_vel->y == 0) && (norm_vel->z == 0))
//		return;
VmVector2Matrix (&objP->orient, &new_fvec, NULL, NULL);
}

//-------------------------------------------------------------------------------------------
//sequence this laser object for this _frame_ (underscores added here to aid MK in his searching!)
void LaserDoWeaponSequence (object *objP)
{
	object *gmObjP;
	Assert (objP->control_type == CT_WEAPON);

	//	Ok, this is a big hack by MK.
	//	If you want an object to last for exactly one frame, then give it a lifeleft of ONE_FRAME_TIME
	if (objP->lifeleft == ONE_FRAME_TIME) {
		if (gameData.app.nGameMode & GM_MULTI)
			objP->lifeleft = OMEGA_MULTI_LIFELEFT;
		else
			objP->lifeleft = 0;
		objP->render_type = RT_NONE;
	}

	if (objP->lifeleft < 0) {		// We died of old age
		objP->flags |= OF_SHOULD_BE_DEAD;
		if (WI_damage_radius (objP->id))
			ExplodeBadassWeapon (objP,&objP->pos);
		return;
	}

	//delete weapons that are not moving
	if (	!((gameData.app.nFrameCount ^ objP->signature) & 3) &&
			 (objP->id != FLARE_ID) &&
			 (gameData.weapons.info [objP->id].speed [gameStates.app.nDifficultyLevel] > 0) &&
			 (VmVecMagQuick (&objP->mtype.phys_info.velocity) < F2_0)) {
		ReleaseObject (OBJ_IDX (objP));
		return;
	}

	if (objP->id == FUSION_ID) {		//always set fusion weapon to max vel

		VmVecNormalizeQuick (&objP->mtype.phys_info.velocity);

		VmVecScale (&objP->mtype.phys_info.velocity, WI_speed (objP->id,gameStates.app.nDifficultyLevel));
	}

// -- 	//	The Super Spreadfire (Helix) blobs travel in a sinusoidal path.  That is accomplished
// -- 	//	by modifying velocity (direction) in the frame interval.
// -- 	if (objP->id == SSPREADFIRE_ID) {
// -- 		fix	age, sinval, cosval;
// -- 		vms_vector	p, newp;
// -- 		fix	speed;
// -- 
// -- 		speed = VmVecMagQuick (&objP->phys_info.velocity);
// -- 
// -- 		age = gameData.weapons.info [objP->id].lifetime - objP->lifeleft;
// -- 
// -- 		fix_fast_sincos (age, &sinval, &cosval);
// -- 
// -- 		//	Note: Below code assumes x=1, y=0.  Need to scale this for values around a circle for 5 helix positions.
// -- 		p.x = cosval << 3;
// -- 		p.y = sinval << 3;
// -- 		p.z = 0;
// -- 
// -- 		VmVecRotate (&newp, &p, &objP->orient);
// -- 
// -- 		VmVecAdd (&goal_point, &objP->pos, &newp);
// -- 
// -- 		VmVecSub (&vGoal, &goal_point, obj
// -- 	}


	//	For homing missiles, turn towards target. (unless it's the guided missile)
	if (WI_homing_flag (objP->id) && 
	    !(objP->id==GUIDEDMISS_ID && 
		  (objP == (gmObjP = gameData.objs.guidedMissile [gameData.objs.objects [objP->ctype.laser_info.parent_num].id])) && 
		   objP->signature==gmObjP->signature)) {
		vms_vector		vector_to_object, temp_vec;
		fix				dot=F1_0;
		fix				speed, max_speed;

		//	For first 1/2 second of life, missile flies straight.
		if (objP->ctype.laser_info.creation_time + HOMING_MISSILE_STRAIGHT_TIME < gameData.time.xGame) {

			int	nTrackGoal = objP->ctype.laser_info.track_goal;
			int	id = objP->id;

			//	If it's time to do tracking, then it's time to grow up, stop bouncing and start exploding!.
			if ((id == ROBOT_SMART_MINE_HOMING_ID) || 
				 (id == ROBOT_SMART_HOMING_ID) || 
				 (id == SMART_MINE_HOMING_ID) || 
				 (id == PLAYER_SMART_HOMING_ID) || 
				 (id == EARTHSHAKER_MEGA_ID)) {
				objP->mtype.phys_info.flags &= ~PF_BOUNCE;
			}

			//	Make sure the object we are tracking is still trackable.
			nTrackGoal = track_track_goal (nTrackGoal, objP, &dot);
			if (nTrackGoal == gameData.multi.players [gameData.multi.nLocalPlayer].objnum) {
				fix	dist_to_player;

				dist_to_player = VmVecDistQuick (&objP->pos, &gameData.objs.objects [nTrackGoal].pos);
				if ((dist_to_player < gameData.multi.players [gameData.multi.nLocalPlayer].homing_object_dist) || (gameData.multi.players [gameData.multi.nLocalPlayer].homing_object_dist < 0))
					gameData.multi.players [gameData.multi.nLocalPlayer].homing_object_dist = dist_to_player;
					
			}

			if (nTrackGoal != -1) {
				VmVecSub (&vector_to_object, &gameData.objs.objects [nTrackGoal].pos, &objP->pos);

				VmVecNormalizeQuick (&vector_to_object);
				temp_vec = objP->mtype.phys_info.velocity;
				speed = VmVecNormalizeQuick (&temp_vec);
				max_speed = WI_speed (objP->id,gameStates.app.nDifficultyLevel);
				if (speed+F1_0 < max_speed) {
					speed += FixMul (max_speed, gameData.time.xFrame/2);
					if (speed > max_speed)
						speed = max_speed;
				}

				// -- dot = VmVecDot (&temp_vec, &vector_to_object);
				VmVecInc (&temp_vec, &vector_to_object);
				//	The boss' smart children track better...
				if (gameData.weapons.info [objP->id].render_type != WEAPON_RENDER_POLYMODEL)
					VmVecInc (&temp_vec, &vector_to_object);
				VmVecNormalizeQuick (&temp_vec);
				objP->mtype.phys_info.velocity = temp_vec;
				VmVecScale (&objP->mtype.phys_info.velocity, speed);

				//	Subtract off life proportional to amount turned.
				//	For hardest turn, it will lose 2 seconds per second.
				{
					fix	lifelost, absdot;
				
					absdot = abs (F1_0 - dot);
				
					lifelost = FixMul (absdot*32, gameData.time.xFrame);
					objP->lifeleft -= lifelost;
				}

				//	Only polygon gameData.objs.objects have visible orientation, so only they should turn.
				if (gameData.weapons.info [objP->id].render_type == WEAPON_RENDER_POLYMODEL)
					HomingMissileTurnTowardsVelocity (objP, &temp_vec);		//	temp_vec is normalized velocity.
			}
		}
	}

	//	Make sure weapon is not moving faster than allowed speed.
	{
		fix	weapon_speed;

		weapon_speed = VmVecMagQuick (&objP->mtype.phys_info.velocity);
		if (weapon_speed > WI_speed (objP->id,gameStates.app.nDifficultyLevel)) {
			//	Only slow down if not allowed to move.  Makes sense, huh?  Allows proxbombs to get moved by physics force. --MK, 2/13/96
			if (WI_speed (objP->id,gameStates.app.nDifficultyLevel)) {
				fix	scale_factor;

				scale_factor = FixDiv (WI_speed (objP->id,gameStates.app.nDifficultyLevel), weapon_speed);
				VmVecScale (&objP->mtype.phys_info.velocity, scale_factor);
			}
		}
	}
}

fix	Last_laser_fired_time = 0;

int	Zbonkers = 0;

//	--------------------------------------------------------------------------------------------------
// Assumption: This is only called by the actual console player, not for network players

int LaserFireLocalPlayer (void)
{
	player	*playerP = gameData.multi.players + gameData.multi.nLocalPlayer;
	fix		xEnergyUsed;
	int		nAmmoUsed,nPrimaryAmmo;
	int		nWeaponIndex;
	int		rval = 0;
	int 		nfires = 1;
	fix		addval;
	static int nSpreadfireToggle = 0;
	static int nHelixOrient = 0;

if (gameStates.app.bPlayerIsDead)
	return 0;
if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.objects [playerP->objnum].segnum].special == SEGMENT_IS_NODAMAGE))
	return 0;
nWeaponIndex = primaryWeaponToWeaponInfo [gameData.weapons.nPrimary];
xEnergyUsed = WI_energy_usage (nWeaponIndex);
if (gameData.weapons.nPrimary == OMEGA_INDEX)
	xEnergyUsed = 0;	//	Omega consumes energy when recharging, not when firing.
if (gameStates.app.nDifficultyLevel < 2)
	xEnergyUsed = FixMul (xEnergyUsed, i2f (gameStates.app.nDifficultyLevel+2)/4);
//	MK, 01/26/96, Helix use 2x energy in multiplayer.  bitmaps.tbl parm should have been reduced for single player.
if (nWeaponIndex == HELIX_INDEX)
	if (gameData.app.nGameMode & GM_MULTI)
		xEnergyUsed *= 2;
nAmmoUsed = WI_ammo_usage (nWeaponIndex);
addval = 2*gameData.time.xFrame;
if (addval > F1_0)
	addval = F1_0;
if ((Last_laser_fired_time + 2 * gameData.time.xFrame < gameData.time.xGame) || (gameData.time.xGame < Last_laser_fired_time))
	xNextLaserFireTime = gameData.time.xGame;
Last_laser_fired_time = gameData.time.xGame;
nPrimaryAmmo = (gameData.weapons.nPrimary == GAUSS_INDEX)? (playerP->primary_ammo [VULCAN_INDEX]): (playerP->primary_ammo [gameData.weapons.nPrimary]);
if	 (!((playerP->energy >= xEnergyUsed) && (nPrimaryAmmo >= nAmmoUsed)))
	AutoSelectWeapon (0, 1);		//	Make sure the player can fire from this weapon.
if (Zbonkers) {
	Zbonkers = 0;
	gameData.time.xGame = 0;
	}

while (xNextLaserFireTime <= gameData.time.xGame) {
	if	((playerP->energy >= xEnergyUsed) && (nPrimaryAmmo >= nAmmoUsed)) {
		int	nLaserLevel, flags = 0;
		if (gameStates.app.cheats.bLaserRapidFire == 0xBADA55)
			xNextLaserFireTime += F1_0/25;
		else
			xNextLaserFireTime += WI_fire_wait (nWeaponIndex);
		nLaserLevel = gameData.multi.players [gameData.multi.nLocalPlayer].laser_level;
		if (gameData.weapons.nPrimary == SPREADFIRE_INDEX) {
			if (nSpreadfireToggle)
				flags |= LASER_SPREADFIRE_TOGGLED;
			nSpreadfireToggle = !nSpreadfireToggle;
			}
		if (gameData.weapons.nPrimary == HELIX_INDEX) {
			nHelixOrient++;
			flags |= ((nHelixOrient & LASER_HELIX_MASK) << LASER_HELIX_SHIFT);
			}
		if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_QUAD_LASERS)
			flags |= LASER_QUAD;
		rval += LaserFireObject ((short) gameData.multi.players [gameData.multi.nLocalPlayer].objnum, (ubyte) gameData.weapons.nPrimary, nLaserLevel, flags, nfires);
		playerP->energy -= (xEnergyUsed * rval) / gameData.weapons.info [nWeaponIndex].fire_count;
		if (playerP->energy < 0)
			playerP->energy = 0;
		if ((gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX)) {
			if (nAmmoUsed > playerP->primary_ammo [VULCAN_INDEX])
				playerP->primary_ammo [VULCAN_INDEX] = 0;
			else
				playerP->primary_ammo [VULCAN_INDEX] -= nAmmoUsed;
			}
		AutoSelectWeapon (0, 1);		//	Make sure the player can fire from this weapon.
		}
	else {
		AutoSelectWeapon (0, 1);		//	Make sure the player can fire from this weapon.
		xNextLaserFireTime = gameData.time.xGame;	//	Prevents shots-to-fire from building up.
		break;	//	Couldn't fire weapon, so abort.
		}
	}
gameData.app.nGlobalLaserFiringCount = 0;	
return rval;
}

// -- #define	MAX_LIGHTNING_DISTANCE	 (F1_0*300)
// -- #define	MAX_LIGHTNING_BLOBS		16
// -- #define	LIGHTNING_BLOB_DISTANCE	 (MAX_LIGHTNING_DISTANCE/MAX_LIGHTNING_BLOBS)
// -- 
// -- #define	LIGHTNING_BLOB_ID			13
// -- 
// -- #define	LIGHTNING_TIME		 (F1_0/4)
// -- #define	LIGHTNING_DELAY	 (F1_0/8)
// -- 
// -- int	Lightning_gun_num = 1;
// -- 
// -- fix	Lightning_start_time = -F1_0*10, Lightning_last_time;
// -- 
// -- //	--------------------------------------------------------------------------------------------------
// -- //	Return -1 if failed to create at least one blob.  Else return index of last blob created.
// -- int create_lightning_blobs (vms_vector *direction, vms_vector *start_pos, int start_segnum, int parent)
// -- {
// -- 	int			i;
// -- 	fvi_query	fq;
// -- 	fvi_info		hit_data;
// -- 	vms_vector	end_pos;
// -- 	vms_vector	norm_dir;
// -- 	int			fate;
// -- 	int			num_blobs;
// -- 	vms_vector	tvec;
// -- 	fix			dist_to_hit_point;
// -- 	vms_vector	point_pos, delta_pos;
// -- 	int			objnum;
// -- 	vms_vector	*gun_pos;
// -- 	vms_matrix	m;
// -- 	vms_vector	gun_pos2;
// -- 
// -- 	if (gameData.multi.players [gameData.multi.nLocalPlayer].energy > F1_0)
// -- 		gameData.multi.players [gameData.multi.nLocalPlayer].energy -= F1_0;
// -- 
// -- 	if (gameData.multi.players [gameData.multi.nLocalPlayer].energy <= F1_0) {
// -- 		gameData.multi.players [gameData.multi.nLocalPlayer].energy = 0;	
// -- 		AutoSelectWeapon (0);
// -- 		return -1;
// -- 	}
// -- 
// -- 	norm_dir = *direction;
// -- 
// -- 	VmVecNormalizeQuick (&norm_dir);
// -- 	VmVecScaleAdd (&end_pos, start_pos, &norm_dir, MAX_LIGHTNING_DISTANCE);
// -- 
// -- 	fq.p0						= start_pos;
// -- 	fq.startSeg				= start_segnum;
// -- 	fq.p1						= &end_pos;
// -- 	fq.rad					= 0;
// -- 	fq.thisObjNum			= parent;
// -- 	fq.ignoreObjList	= NULL;
// -- 	fq.flags					= FQ_TRANSWALL | FQ_CHECK_OBJS;
// -- 
// -- 	fate = FindVectorIntersection (&fq, &hit_data);
// -- 	if (hit_data.hit.nSegment == -1) {
// -- 		return -1;
// -- 	}
// -- 
// -- 	dist_to_hit_point = VmVecMag (VmVecSub (&tvec, &hit_data.hit.vPoint, start_pos);
// -- 	num_blobs = dist_to_hit_point/LIGHTNING_BLOB_DISTANCE;
// -- 
// -- 	if (num_blobs > MAX_LIGHTNING_BLOBS)
// -- 		num_blobs = MAX_LIGHTNING_BLOBS;
// -- 
// -- 	if (num_blobs < MAX_LIGHTNING_BLOBS/4)
// -- 		num_blobs = MAX_LIGHTNING_BLOBS/4;
// -- 
// -- 	// Find the initial position of the laser
// -- 	gun_pos = &gameData.pig.ship.player->gun_points [Lightning_gun_num];
// -- 	VmCopyTransposeMatrix (&m,&gameData.objs.objects [parent].orient);
// -- 	VmVecRotate (&gun_pos2, gun_pos, &m);
// -- 	VmVecAdd (&point_pos, &gameData.objs.objects [parent].pos, &gun_pos2);
// -- 
// -- 	delta_pos = norm_dir;
// -- 	VmVecScale (&delta_pos, dist_to_hit_point/num_blobs);
// -- 
// -- 	for (i=0; i<num_blobs; i++) {
// -- 		int			point_seg;
// -- 		object		*obj;
// -- 
// -- 		VmVecInc (&point_pos, &delta_pos);
// -- 		point_seg = FindSegByPoint (&point_pos, start_segnum);
// -- 		if (point_seg == -1)	//	Hey, we thought we were creating points on a line, but we left the mine!
// -- 			continue;
// -- 
// -- 		objnum = CreateNewLaser (direction, &point_pos, point_seg, parent, LIGHTNING_BLOB_ID, 0);
// -- 
// -- 		if (objnum < 0) 	{
// -- 			Int3 ();
// -- 			return -1;
// -- 		}
// -- 
// -- 		obj = &gameData.objs.objects [objnum];
// -- 
// -- 		DigiPlaySample (gameData.weapons.info [objP->id].flash_sound, F1_0);
// -- 
// -- 		// -- VmVecScale (&objP->mtype.phys_info.velocity, F1_0/2);
// -- 
// -- 		objP->lifeleft = (LIGHTNING_TIME + LIGHTNING_DELAY)/2;
// -- 
// -- 	}
// -- 
// -- 	return objnum;
// -- 
// -- }
// -- 
// -- //	--------------------------------------------------------------------------------------------------
// -- //	Lightning Cannon.
// -- //	While being fired, creates path of blobs forward from player until it hits something.
// -- //	Up to MAX_LIGHTNING_BLOBS blobs, spaced LIGHTNING_BLOB_DISTANCE units apart.
// -- //	When the player releases the firing key, the blobs move forward.
// -- void lightning_frame (void)
// -- {
// -- 	if ((gameData.time.xGame - Lightning_start_time < LIGHTNING_TIME) && (gameData.time.xGame - Lightning_start_time > 0)) {
// -- 		if (gameData.time.xGame - Lightning_last_time > LIGHTNING_DELAY) {
// -- 			create_lightning_blobs (&gameData.objs.console->orient.fvec, &gameData.objs.console->pos, gameData.objs.console->segnum, OBJ_IDX (gameData.objs.console));
// -- 			Lightning_last_time = gameData.time.xGame;
// -- 		}
// -- 	}
// -- }

//	--------------------------------------------------------------------------------------------------
//	Object "objnum" fires weapon "weapon_num" of level "level". (Right now (9/24/94) level is used only for type 0 laser.
//	Flags are the player flags.  For network mode, set to 0.
//	It is assumed that this is a player object (as in multiplayer), and therefore the gun positions are known.
//	Returns number of times a weapon was fired.  This is typically 1, but might be more for low frame rates.
//	More than one shot is fired with a pseudo-delay so that players on show machines can fire (for themselves
//	or other players) often enough for things like the vulcan cannon.
int LaserFireObject (short objnum, ubyte nWeapon, int level, int flags, int nFires)
{
	object	*objP = gameData.objs.objects + objnum;

	switch (nWeapon) {
		case LASER_INDEX: {
			ubyte	nLaser;
			nLaserOffset = ((F1_0*2)* (d_rand ()%8))/8;
			if (level <= MAX_LASER_LEVEL)
				nLaser = LASER_ID + level;
			else
				nLaser = SUPER_LASER_ID + (level - MAX_LASER_LEVEL-1);
			LaserPlayerFire (objP, nLaser, 0, 1, 0);
			LaserPlayerFire (objP, nLaser, 1, 0, 0);
			if (flags & LASER_QUAD) {
				//	hideous system to make quad laser 1.5x powerful as normal laser, make every other quad laser bolt harmless
				LaserPlayerFire (objP, nLaser, 2, 0, 0);
				LaserPlayerFire (objP, nLaser, 3, 0, 0);
			}
			break;
		}
		case VULCAN_INDEX: {
			//	Only make sound for 1/4 of vulcan bullets.
			int	make_sound = 1;
			//if (d_rand () > 24576)
			//	make_sound = 1;
			LaserPlayerFireSpread (objP, VULCAN_ID, 6, d_rand ()/8 - 32767/16, d_rand ()/8 - 32767/16, make_sound, 0);
			if (nFires > 1) {
				LaserPlayerFireSpread (objP, VULCAN_ID, 6, d_rand ()/8 - 32767/16, d_rand ()/8 - 32767/16, 0, 0);
				if (nFires > 2) {
					LaserPlayerFireSpread (objP, VULCAN_ID, 6, d_rand ()/8 - 32767/16, d_rand ()/8 - 32767/16, 0, 0);
				}
			}
			break;
		}
		case SPREADFIRE_INDEX:
			if (flags & LASER_SPREADFIRE_TOGGLED) {
				LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, F1_0/16, 0, 0, 0);
				LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, -F1_0/16, 0, 0, 0);
				LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, 0, 0, 1, 0);
			} else {
				LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, 0, F1_0/16, 0, 0);
				LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, 0, -F1_0/16, 0, 0);
				LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, 0, 0, 1, 0);
			}
			break;

		case PLASMA_INDEX:
			LaserPlayerFire (objP, PLASMA_ID, 0, 1, 0);
			LaserPlayerFire (objP, PLASMA_ID, 1, 0, 0);
			if (nFires > 1) {
				LaserPlayerFireSpreadDelay (objP, PLASMA_ID, 0, 0, 0, gameData.time.xFrame/2, 1, 0);
				LaserPlayerFireSpreadDelay (objP, PLASMA_ID, 1, 0, 0, gameData.time.xFrame/2, 0, 0);
			}
			break;

		case FUSION_INDEX: {
			vms_vector	vForce;

			LaserPlayerFire (objP, FUSION_ID, 0, 1, 0);
			LaserPlayerFire (objP, FUSION_ID, 1, 1, 0);

			flags = (sbyte) (gameData.app.fusion.xCharge >> 12);

			gameData.app.fusion.xCharge = 0;

			vForce.x = - (objP->orient.fvec.x << 7);
			vForce.y = - (objP->orient.fvec.y << 7);
			vForce.z = - (objP->orient.fvec.z << 7);
			PhysApplyForce (objP, &vForce);

			vForce.x = (vForce.x >> 4) + d_rand () - 16384;
			vForce.y = (vForce.y >> 4) + d_rand () - 16384;
			vForce.z = (vForce.z >> 4) + d_rand () - 16384;
			PhysApplyRot (objP, &vForce);

		}
			break;
		case SUPER_LASER_INDEX: {
			ubyte super_level = 3;		//make some new kind of laser eventually
			LaserPlayerFire (objP, super_level, 0, 1, 0);
			LaserPlayerFire (objP, super_level, 1, 0, 0);

			if (flags & LASER_QUAD) {
				//	hideous system to make quad laser 1.5x powerful as normal laser, make every other quad laser bolt harmless
				LaserPlayerFire (objP, super_level, 2, 0, 0);
				LaserPlayerFire (objP, super_level, 3, 0, 0);
			}
			break;
		}
		case GAUSS_INDEX: {
			//	Only make sound for 1/4 of vulcan bullets.
			int	make_sound = 1;
			//if (d_rand () > 24576)
			//	make_sound = 1;
			
			LaserPlayerFireSpread (objP, GAUSS_ID, 6, (d_rand ()/8 - 32767/16)/5, (d_rand ()/8 - 32767/16)/5, make_sound, 0);
			if (nFires > 1) {
				LaserPlayerFireSpread (objP, GAUSS_ID, 6, (d_rand ()/8 - 32767/16)/5, (d_rand ()/8 - 32767/16)/5, 0, 0);
				if (nFires > 2) {
					LaserPlayerFireSpread (objP, GAUSS_ID, 6, (d_rand ()/8 - 32767/16)/5, (d_rand ()/8 - 32767/16)/5, 0, 0);
				}
			}
			break;
		}
		case HELIX_INDEX: {
			int helix_orient;
			fix spreadr,spreadu;
			helix_orient = (flags >> LASER_HELIX_SHIFT) & LASER_HELIX_MASK;
			switch (helix_orient) {

				case 0: spreadr =  F1_0/16; spreadu = 0;       break; // Vertical
				case 1: spreadr =  F1_0/17; spreadu = F1_0/42; break; //  22.5 degrees
				case 2: spreadr =  F1_0/22; spreadu = F1_0/22; break; //  45   degrees
				case 3: spreadr =  F1_0/42; spreadu = F1_0/17; break; //  67.5 degrees
				case 4: spreadr =  0;       spreadu = F1_0/16; break; //  90   degrees
				case 5: spreadr = -F1_0/42; spreadu = F1_0/17; break; // 112.5 degrees
				case 6: spreadr = -F1_0/22; spreadu = F1_0/22; break; // 135   degrees	
				case 7: spreadr = -F1_0/17; spreadu = F1_0/42; break; // 157.5 degrees
				default:
					Error ("Invalid helix_orientation value %x\n",helix_orient);
			}

			LaserPlayerFireSpread (objP, HELIX_ID, 6,  0,  0, 1, 0);
			LaserPlayerFireSpread (objP, HELIX_ID, 6,  spreadr,  spreadu, 0, 0);
			LaserPlayerFireSpread (objP, HELIX_ID, 6, -spreadr, -spreadu, 0, 0);
			LaserPlayerFireSpread (objP, HELIX_ID, 6,  spreadr*2,  spreadu*2, 0, 0);
			LaserPlayerFireSpread (objP, HELIX_ID, 6, -spreadr*2, -spreadu*2, 0, 0);
			break;
		}

		case PHOENIX_INDEX:
			LaserPlayerFire (objP, PHOENIX_ID, 0, 1, 0);
			LaserPlayerFire (objP, PHOENIX_ID, 1, 0, 0);
			if (nFires > 1) {
				LaserPlayerFireSpreadDelay (objP, PHOENIX_ID, 0, 0, 0, gameData.time.xFrame/2, 1, 0);
				LaserPlayerFireSpreadDelay (objP, PHOENIX_ID, 1, 0, 0, gameData.time.xFrame/2, 0, 0);
			}
			break;

		case OMEGA_INDEX:
			LaserPlayerFire (objP, OMEGA_ID, 1, 1, 0);
			break;

		default:
			Int3 ();	//	Contact Yuan: Unknown Primary weapon type, setting to 0.
			gameData.weapons.nPrimary = 0;
	}

	// Set values to be recognized during comunication phase, if we are the
	//  one shooting
#ifdef NETWORK
	if ((gameData.app.nGameMode & GM_MULTI) && (objnum == gameData.multi.players [gameData.multi.nLocalPlayer].objnum))
	{
		multiData.laser.bFired = nFires;
		multiData.laser.nGun = nWeapon;
		multiData.laser.nFlags = flags;
		multiData.laser.nLevel = level;
	}
#endif

	return nFires;
}

//	-------------------------------------------------------------------------------------------
//	if nGoalObj == -1, then create random vector
int CreateHomingMissile (object *objP, int nGoalObj, ubyte objtype, int make_sound)
{
	short			objnum;
	vms_vector	vGoal;
	vms_vector	random_vector;
	//vms_vector	vGoalPos;

	if (nGoalObj == -1) {
		MakeRandomVector (&vGoal);
	} else {
		VmVecNormalizedDirQuick (&vGoal, &gameData.objs.objects [nGoalObj].pos, &objP->pos);
		MakeRandomVector (&random_vector);
		VmVecScaleInc (&vGoal, &random_vector, F1_0/4);
		VmVecNormalizeQuick (&vGoal);
	}		

	//	Create a vector towards the goal, then add some noise to it.
	objnum = CreateNewLaser (&vGoal, &objP->pos, objP->segnum, 
									  OBJ_IDX (objP), objtype, make_sound);
	if (objnum == -1)
		return -1;

	// Fixed to make sure the right person gets credit for the kill

//	gameData.objs.objects [objnum].ctype.laser_info.parent_num = objP->ctype.laser_info.parent_num;
//	gameData.objs.objects [objnum].ctype.laser_info.parent_type = objP->ctype.laser_info.parent_type;
//	gameData.objs.objects [objnum].ctype.laser_info.parent_signature = objP->ctype.laser_info.parent_signature;

	gameData.objs.objects [objnum].ctype.laser_info.track_goal = nGoalObj;

	return objnum;
}

extern void BlastNearbyGlass (object *objP, fix damage);

//-----------------------------------------------------------------------------
// Create the children of a smart bomb, which is a bunch of homing missiles.
void CreateSmartChildren (object *objP, int num_smart_children)
{
	int	parent_type, parent_num;
	int	make_sound;
	int	numobjs=0;
	int	objlist [MAX_OBJDISTS];
	ubyte	blob_id;

	if (objP->type == OBJ_WEAPON) {
		parent_type = objP->ctype.laser_info.parent_type;
		parent_num = objP->ctype.laser_info.parent_num;
	} else if (objP->type == OBJ_ROBOT) {
		parent_type = OBJ_ROBOT;
		parent_num = OBJ_IDX (objP);
	} else {
		Int3 ();	//	Hey, what kind of object is this!?
		parent_type = 0;
		parent_num = 0;
	}

	if (objP->id == EARTHSHAKER_ID)
		BlastNearbyGlass (objP, gameData.weapons.info [EARTHSHAKER_ID].strength [gameStates.app.nDifficultyLevel]);

// -- DEBUG --
	if ((objP->type == OBJ_WEAPON) && ((objP->id == SMART_ID) || (objP->id == SUPERPROX_ID) || (objP->id == ROBOT_SUPERPROX_ID) || (objP->id == EARTHSHAKER_ID)))
		Assert (gameData.weapons.info [objP->id].children != -1);
// -- DEBUG --

	if (((objP->type == OBJ_WEAPON) && (gameData.weapons.info [objP->id].children != -1)) || (objP->type == OBJ_ROBOT)) {
		int	i, objnum;

		if (gameData.app.nGameMode & GM_MULTI)
			d_srand (8321L);

		for (objnum=0; objnum<=gameData.objs.nLastObject; objnum++) {
			object	*curObjP = &gameData.objs.objects [objnum];

			if ((((curObjP->type == OBJ_ROBOT) && (!curObjP->ctype.ai_info.CLOAKED)) || (curObjP->type == OBJ_PLAYER)) && (objnum != parent_num)) {
				fix	dist;

				if (curObjP->type == OBJ_PLAYER)
				{
					if ((parent_type == OBJ_PLAYER) && (gameData.app.nGameMode & GM_MULTI_COOP))
						continue;
#ifdef NETWORK
					if ((gameData.app.nGameMode & GM_TEAM) && (GetTeam (curObjP->id) == GetTeam (gameData.objs.objects [parent_num].id)))
						continue;
#endif

					if (gameData.multi.players [curObjP->id].flags & PLAYER_FLAGS_CLOAKED)
						continue;
				}

				//	Robot blobs can't track robots.
				if (curObjP->type == OBJ_ROBOT) {
					if (parent_type == OBJ_ROBOT)
						continue;

					//	Your shots won't track the buddy.
					if (parent_type == OBJ_PLAYER)
						if (gameData.bots.pInfo [curObjP->id].companion)
							continue;
				}

				dist = VmVecDistQuick (&objP->pos, &curObjP->pos);
				if (dist < MAX_SMART_DISTANCE) {
					int	oovis;

					oovis = ObjectToObjectVisibility (objP, curObjP, FQ_TRANSWALL);

					if (oovis) { //ObjectToObjectVisibility (objP, curObjP, FQ_TRANSWALL)) {
						objlist [numobjs] = objnum;
						numobjs++;
						if (numobjs >= MAX_OBJDISTS) {
							numobjs = MAX_OBJDISTS;
							break;
						}
					}
				}
			}
		}

		//	Get type of weapon for child from parent.
		if (objP->type == OBJ_WEAPON) {
			blob_id = gameData.weapons.info [objP->id].children;
			Assert (blob_id != -1);		//	Hmm, missing data in bitmaps.tbl.  Need "children=NN" parameter.
		} else {
			Assert (objP->type == OBJ_ROBOT);
			blob_id = ROBOT_SMART_HOMING_ID;
		}

// -- 		//determine what kind of blob to drop
// -- 		//	Note: parent_type is not the type of the weapon's parent.  It is actually the type of the weapon's
// -- 		//	earliest ancestor.  This deals with the issue of weapons spewing weapons which spew weapons.
// -- 		switch (parent_type) {
// -- 			case OBJ_WEAPON:
// -- 				Int3 ();	//	Should this ever happen?
// -- 				switch (objP->id) {
// -- 					case SUPERPROX_ID:			blob_id = SMART_MINE_HOMING_ID; break;
// -- 					case ROBOT_SUPERPROX_ID:	blob_id = ROBOT_SMART_MINE_HOMING_ID; break;
// -- 					case EARTHSHAKER_ID:			blob_id = EARTHSHAKER_MEGA_ID; break;
// -- 					default:							Int3 ();	//bogus id for weapon  
// -- 				}
// -- 				break;
// -- 			case OBJ_PLAYER:
// -- 				switch (objP->id) {
// -- 					case SUPERPROX_ID:			blob_id = SMART_MINE_HOMING_ID; break;
// -- 					case ROBOT_SUPERPROX_ID:	Int3 ();	break;
// -- 					case EARTHSHAKER_ID:			blob_id = EARTHSHAKER_MEGA_ID; break;
// -- 					case SMART_ID:					blob_id = PLAYER_SMART_HOMING_ID; break;
// -- 					default:							Int3 ();	//bogus id for weapon  
// -- 				}
// -- 				break;
// -- 			case OBJ_ROBOT:
// -- 				switch (objP->id) {
// -- 					case ROBOT_SUPERPROX_ID:	blob_id = ROBOT_SMART_MINE_HOMING_ID; break;
// -- 					// -- case EARTHSHAKER_ID:			blob_id = EARTHSHAKER_MEGA_ID; break;
// -- 					case SMART_ID:					blob_id = ROBOT_SMART_HOMING_ID; break;
// -- 					default:							blob_id = ROBOT_SMART_HOMING_ID; break;
// -- 				}
// -- 				break;
// -- 			default:					Int3 ();	//bogus type for parent object
// -- 		}

		make_sound = 1;
		for (i=0; i<num_smart_children; i++) {
			short objnum = (numobjs==0)?-1:objlist [ (d_rand () * numobjs) >> 15];
			CreateHomingMissile (objP, objnum, blob_id, make_sound);
			make_sound = 0;
		}
	}
}

//	-------------------------------------------------------------------------------------------

int Missile_gun = 0;

//give up control of the guided missile
void ReleaseGuidedMissile (int player_num)
{
	if (player_num == gameData.multi.nLocalPlayer)
	 {			
	  if (gameData.objs.guidedMissile [player_num]==NULL)
			return;
	
		gameData.objs.missileViewer = gameData.objs.guidedMissile [player_num];
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)
		 MultiSendGuidedInfo (gameData.objs.guidedMissile [gameData.multi.nLocalPlayer],1);
#endif
		if (gameData.demo.nState==ND_STATE_RECORDING)
		 NDRecordGuidedEnd ();
	 }	

	gameData.objs.guidedMissile [player_num] = NULL;
}

int Proximity_dropped=0,Smartmines_dropped=0;

//	-------------------------------------------------------------------------------------------
//parameter determines whether or not to do autoselect if have run out of ammo
//this is needed because if you drop a bomb with the B key, you don't 
//want to autoselect if the bomb isn't actually selected. 
void DoMissileFiring (int bAutoSelect)
{
	int		h, i, gun_flag = 0;
	short		objnum;
	ubyte		nWeaponId;
	int		nWeaponGun;
	object	*gmP = gameData.objs.guidedMissile [gameData.multi.nLocalPlayer];
	player	*playerP = gameData.multi.players + gameData.multi.nLocalPlayer;

Assert (gameData.weapons.nSecondary < MAX_SECONDARY_WEAPONS);
if (gmP && (gmP->signature == gameData.objs.guidedMissileSig [gameData.multi.nLocalPlayer])) {
	ReleaseGuidedMissile (gameData.multi.nLocalPlayer);
	i = secondaryWeaponToWeaponInfo [gameData.weapons.nSecondary];
	xNextMissileFireTime = gameData.time.xGame + WI_fire_wait (i);
	return;
	}

if (gameStates.app.bPlayerIsDead || (playerP->secondary_ammo [gameData.weapons.nSecondary] <= 0))
	return;

nWeaponId = secondaryWeaponToWeaponInfo [gameData.weapons.nSecondary];
if (gameStates.app.cheats.bLaserRapidFire != 0xBADA55)
	xNextMissileFireTime = gameData.time.xGame + WI_fire_wait (nWeaponId);
else
	xNextMissileFireTime = gameData.time.xGame + F1_0/25;
nWeaponGun = secondaryWeaponToGunNum [gameData.weapons.nSecondary];
h = (EGI_FLAG (bDualMissileLaunch, 1, 0)) ? 1 : 0;
for (i = 0; (i <= h) && (playerP->secondary_ammo [gameData.weapons.nSecondary] > 0); i++) {
	playerP->secondary_ammo [gameData.weapons.nSecondary]--;
	if (IsMultiGame)
		MultiSendWeapons (1);
	if (nWeaponGun == 4) {		//alternate left/right
		nWeaponGun += (gun_flag = (Missile_gun & 1));
		Missile_gun++;
		}
	objnum = LaserPlayerFire (gameData.objs.console, nWeaponId, nWeaponGun, 1, 0);
	if (gameData.weapons.nSecondary == PROXIMITY_INDEX) {
		if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
			if (++Proximity_dropped == 4) {
				Proximity_dropped = 0;
#ifdef NETWORK
				MaybeDropNetPowerup (objnum, POW_PROXIMITY_WEAPON, INIT_DROP);
#endif
				}
			}
		break; //no dual prox bomb drop
		}
	else if (gameData.weapons.nSecondary == SMART_MINE_INDEX) {
		if (!(gameData.app.nGameMode & GM_ENTROPY)) {
			if (++Smartmines_dropped == 4) {
				Smartmines_dropped = 0;
#ifdef NETWORK
				MaybeDropNetPowerup (objnum, POW_SMART_MINE, INIT_DROP);
#endif
				}
			}
		break; //no dual smartmine drop
		}
#ifdef NETWORK
	else if (gameData.weapons.nSecondary != CONCUSSION_INDEX)
		MaybeDropNetPowerup (objnum, secondaryWeaponToPowerup [gameData.weapons.nSecondary], INIT_DROP);
#endif
	if ((gameData.weapons.nSecondary == GUIDED_INDEX) || (gameData.weapons.nSecondary == SMART_INDEX))
		break;
	else if ((gameData.weapons.nSecondary == MEGA_INDEX) || (gameData.weapons.nSecondary == SMISSILE5_INDEX)) {
		vms_vector vForce;

	vForce.x = - (gameData.objs.console->orient.fvec.x << 7);
	vForce.y = - (gameData.objs.console->orient.fvec.y << 7);
	vForce.z = - (gameData.objs.console->orient.fvec.z << 7);
	PhysApplyForce (gameData.objs.console, &vForce);
	vForce.x = (vForce.x >> 4) + d_rand () - 16384;
	vForce.y = (vForce.y >> 4) + d_rand () - 16384;
	vForce.z = (vForce.z >> 4) + d_rand () - 16384;
	PhysApplyRot (gameData.objs.console, &vForce);
	break; //no dual mega/smart missile launch
	}
}

#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI) {
	multiData.laser.bFired = 1;		//how many
	multiData.laser.nGun = gameData.weapons.nSecondary + MISSILE_ADJUST;
	multiData.laser.nFlags = gun_flag;
	multiData.laser.nLevel = 0;
	}
#endif
if (bAutoSelect)
	AutoSelectWeapon (1, 1);		//select next missile, if this one out of ammo
}

//	-------------------------------------------------------------------------------------------
// eof
