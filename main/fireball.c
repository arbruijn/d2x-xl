/* $Id: fireball.c,v 1.4 2003/10/04 03:14:47 btb Exp $ */
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
 * Code for rendering & otherwise dealing with explosions
 *
 * Old Log:
 * Revision 1.2  1995/10/31  10:23:56  allender
 * shareware stuff
 *
 * Revision 1.1  1995/05/16  15:24:41  allender
 * Initial revision
 *
 * Revision 2.2  1995/03/21  14:39:57  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.1  1995/03/20  18:15:47  john
 * Added code to not store the normals in the segment structure.
 *
 * Revision 2.0  1995/02/27  11:30:34  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.200  1995/02/22  13:18:41  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.199  1995/02/14  19:58:32  mike
 * comment out "something bad has happened" int3.
 *
 * Revision 1.198  1995/02/09  13:11:01  mike
 * remove an annoying con_printf and Int3().
 *
 * Revision 1.197  1995/02/08  17:10:14  mike
 * don't drop cloaks if one nearby.
 *
 * Revision 1.196  1995/02/08  13:27:14  rob
 * Give keys dropped by robots 0 velocity in coop game.
 *
 * Revision 1.195  1995/02/08  11:57:40  mike
 * determine whether debris object failed to create because buffer was
 * exhausted or because limit was hit.
 *
 * Revision 1.194  1995/02/08  11:37:58  mike
 * Check for failures in call to CreateObject.
 *
 * Revision 1.193  1995/02/07  21:09:41  mike
 * only replace weapon with energy 1/2 time.
 *
 * Revision 1.192  1995/01/30  18:21:52  rob
 * Replace extra life powerups in multiplayer to invul when
 * dropped by robots.
 *
 * Revision 1.191  1995/01/28  17:40:59  mike
 * fix stupidity in converting quad lasers to energy.
 *
 * Revision 1.190  1995/01/27  15:05:59  rob
 * Trying to fix a bug with damaging robots with player badass explosions.
 *
 * Revision 1.189  1995/01/26  18:59:04  rob
 * Powerups were flying too far in robot-cooperative games.
 *
 * Revision 1.188  1995/01/25  10:53:35  mike
 * make badass damage go through grates.
 *
 * Revision 1.187  1995/01/25  09:37:23  mike
 * fix gameData.objs.objects containing robots, worked for powerups, bad {} placement.
 *
 * Revision 1.186  1995/01/23  22:51:20  mike
 * drop energy instead of primary weapon if you already have primary weapon.
 *
 * Revision 1.185  1995/01/20  16:56:37  mike
 * Cut damage done by badass weapons.
 *
 * Revision 1.184  1995/01/19  17:44:57  mike
 * damage_force removed, that information coming from strength field.
 *
 * Revision 1.183  1995/01/16  21:06:54  mike
 * Move function pick_random_point_in_segment from fireball.c to gameseg.c.
 *
 * Revision 1.182  1995/01/16  19:24:04  mike
 * If a gated-in robot and going to drop energy powerup, don't!
 *
 * Revision 1.181  1995/01/15  20:48:03  mike
 * drop energy in place of quad lasers if player already has quad lasers.
 *
 * Revision 1.180  1995/01/14  19:32:19  rob
 * Fixed an error.
 *
 * Revision 1.179  1995/01/14  18:50:55  rob
 * Make robot egg creation suitable for mutliplayer situations.
 *
 * Revision 1.178  1995/01/14  14:55:07  rob
 * Make weapons/keys/etc never disappear in network mode.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "error.h"
#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"

#include "inferno.h"
#include "object.h"
#include "vclip.h"
#include "game.h"
#include "mono.h"
#include "polyobj.h"
#include "sounds.h"
#include "player.h"
#include "gauges.h"
#include "powerup.h"
#include "bm.h"
#include "ai.h"
#include "weapon.h"
#include "fireball.h"
#include "collide.h"
#include "newmenu.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "gameseq.h"
#include "physics.h"
#include "scores.h"
#include "laser.h"
#include "wall.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "endlevel.h"
#include "timer.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "gameseg.h"
#include "gamesave.h"
#include "automap.h"
#include "kconfig.h"
#include "text.h"
#include "hudmsg.h"

#define EXPLOSION_SCALE (F1_0*5/2)		//explosion is the obj size times this 

//--unused-- ubyte	Frame_processed[MAX_OBJECTS];

int	PK1=1, PK2=8;

//------------------------------------------------------------------------------

object *ObjectCreateExplosionSub (
	object *objP, 
	short segnum, 
	vms_vector * position, 
	fix size, 
	ubyte vclip_type, 
	fix maxdamage, 
	fix maxdistance, 
	fix maxforce, 
	short parent)
{
	short objnum;
	object *explObjP;

objnum = CreateObject (OBJ_FIREBALL, vclip_type, -1, segnum, position, &vmdIdentityMatrix,size,
							  CT_EXPLOSION, MT_NONE, RT_FIREBALL, 1);

	if (objnum < 0) {
#if TRACE
		con_printf (1, "Can't create object in ObjectCreateExplosionSub.\n");
#endif
		return NULL;
	}

	explObjP = gameData.objs.objects + objnum;
	//now set explosion-specific data
	explObjP->lifeleft = gameData.eff.vClips [0][vclip_type].xTotalTime;
	explObjP->ctype.expl_info.spawn_time = -1;
	explObjP->ctype.expl_info.delete_objnum = -1;
	explObjP->ctype.expl_info.delete_time = -1;

	if (maxdamage > 0) {
		fix dist, force;
		vms_vector pos_hit, vforce;
		fix damage;
		int i, t, id;
		object * obj0P = gameData.objs.objects;
					 
		// -- now legal for badass explosions on a wall. Assert(objP != NULL);

		for (i=0; i<=gameData.objs.nLastObject; i++)	{
			t = obj0P->type;
			id = obj0P->id;
			//	Weapons used to be affected by badass explosions, but this introduces serious problems.
			//	When a smart bomb blows up, if one of its children goes right towards a nearby wall, it will
			//	blow up, blowing up all the children.  So I remove it.  MK, 09/11/94
			if ((obj0P!=objP) && !(obj0P->flags&OF_SHOULD_BE_DEAD) && 
			    ((t==OBJ_WEAPON && (id==PROXIMITY_ID || id==SUPERPROX_ID || id==PMINE_ID)) || 
			     (t == OBJ_CNTRLCEN) || 
			     (t==OBJ_PLAYER) || ((t==OBJ_ROBOT) && 
			     ((gameData.objs.objects[parent].type != OBJ_ROBOT) || (gameData.objs.objects[parent].id != id))))) {
				dist = VmVecDistQuick(&obj0P->pos, &explObjP->pos);
				// Make damage be from 'maxdamage' to 0.0, where 0.0 is 'maxdistance' away;
				if (dist < maxdistance) {
					if (ObjectToObjectVisibility(explObjP, obj0P, FQ_TRANSWALL)) {

						damage = maxdamage - fixmuldiv(dist, maxdamage, maxdistance);
						force = maxforce - fixmuldiv(dist, maxforce, maxdistance);

						// Find the force vector on the object
						VmVecNormalizedDirQuick(&vforce, &obj0P->pos, &explObjP->pos);
						VmVecScale(&vforce, force);
	
						// Find where the point of impact is... (pos_hit)
						VmVecSub(&pos_hit, &explObjP->pos, &obj0P->pos);
						VmVecScale(&pos_hit, fixdiv (obj0P->size, obj0P->size + dist));
	
						switch (obj0P->type)	{
							case OBJ_WEAPON:
								PhysApplyForce(obj0P,&vforce);

								if (obj0P->id == PROXIMITY_ID || obj0P->id == SUPERPROX_ID) {		//prox bombs have chance of blowing up
									if (fixmul(dist,force) > i2f(8000)) {
										obj0P->flags |= OF_SHOULD_BE_DEAD;
										ExplodeBadassWeapon(obj0P,&obj0P->pos);
									}
								}
								break;

							case OBJ_ROBOT:
								{
								PhysApplyForce(obj0P,&vforce);

								//	If not a boss, stun for 2 seconds at 32 force, 1 second at 16 force
								if ((objP != NULL) && (!gameData.bots.pInfo[obj0P->id].boss_flag) && (gameData.weapons.info[objP->id].flash)) {
									ai_static	*aip = &obj0P->ctype.ai_info;
									int			force_val = f2i(fixdiv(VmVecMagQuick(&vforce) * gameData.weapons.info[objP->id].flash, gameData.app.xFrameTime)/128) + 2;

									if (explObjP->ctype.ai_info.SKIP_AI_COUNT * gameData.app.xFrameTime < F1_0) {
										aip->SKIP_AI_COUNT += force_val;
										obj0P->mtype.phys_info.rotthrust.x = ((d_rand() - 16384) * force_val)/16;
										obj0P->mtype.phys_info.rotthrust.y = ((d_rand() - 16384) * force_val)/16;
										obj0P->mtype.phys_info.rotthrust.z = ((d_rand() - 16384) * force_val)/16;
										obj0P->mtype.phys_info.flags |= PF_USES_THRUST;

										//@@if (gameData.bots.pInfo[obj0P->id].companion)
										//@@	BuddyMessage("Daisy, Daisy, Give me...");
									} else
										aip->SKIP_AI_COUNT--;

								}

								//	When a robot gets whacked by a badass force, he looks towards it because robots tend to get blasted from behind.
								{
									vms_vector neg_vforce;
									neg_vforce.x = vforce.x * -2 * (7 - gameStates.app.nDifficultyLevel)/8;
									neg_vforce.y = vforce.y * -2 * (7 - gameStates.app.nDifficultyLevel)/8;
									neg_vforce.z = vforce.z * -2 * (7 - gameStates.app.nDifficultyLevel)/8;
									PhysApplyRot(obj0P,&neg_vforce);
								}
								if (obj0P->shields >= 0) {
									if (gameData.bots.pInfo[obj0P->id].boss_flag)
										if (bossProps [gameStates.app.bD1Mission][gameData.bots.pInfo[obj0P->id].boss_flag-BOSS_D2].bInvulKinetic)
											damage /= 4;

									if (ApplyDamageToRobot(obj0P, damage, parent))
										if ((objP != NULL) && (parent == gameData.multi.players[gameData.multi.nLocalPlayer].objnum))
											AddPointsToScore(gameData.bots.pInfo[obj0P->id].score_value);
								}

								if ((objP != NULL) && (gameData.bots.pInfo[obj0P->id].companion) && (!gameData.weapons.info[objP->id].flash)) {
									int	i, count;
									char	ouch_str[6*4 + 2];

									count = f2i(damage/8);
									if (count > 4)
										count = 4;
									else if (count <= 0)
										count = 1;
									ouch_str[0] = 0;
									for (i=0; i<count; i++) {
										strcat(ouch_str, TXT_BUDDY_OUCH);
										strcat(ouch_str, " ");
										}
									BuddyMessage(ouch_str);
								}
								break;
								}
							case OBJ_CNTRLCEN:
								if (obj0P->shields >= 0) {
									ApplyDamageToReactor(obj0P, damage, parent);
								}
								break;
							case OBJ_PLAYER:	{
								object * killer=NULL;
								vms_vector	vforce2;

								//	Hack! Warning! Test code!
								if ((objP != NULL) && gameData.weapons.info[objP->id].flash && obj0P->id==gameData.multi.nLocalPlayer) {
									int	fe;

									fe = min(F1_0*4, force*gameData.weapons.info[objP->id].flash/32);	//	For four seconds or less

									if (objP->ctype.laser_info.parent_signature == gameData.objs.console->signature) {
										fe /= 2;
										force /= 2;
									}
									if (force > F1_0) {
										gameData.render.xFlashEffect = fe;
										PALETTE_FLASH_ADD(PK1 + f2i(PK2*force), PK1 + f2i(PK2*force), PK1 + f2i(PK2*force));
#if TRACE
										con_printf (CON_DEBUG, "force = %7.3f, adding %i\n", f2fl(force), PK1 + f2i(PK2*force));
#endif
									}
								}

								if ((objP != NULL) && (gameData.app.nGameMode & GM_MULTI) && (objP->type == OBJ_PLAYER)) {
									killer = objP;
								}
								vforce2 = vforce;
								if (parent > -1) {
									killer = gameData.objs.objects + parent;
									if (killer != gameData.objs.console)		// if someone else whacks you, cut force by 2x
										vforce2.x /= 2;	
										vforce2.y /= 2;	
										vforce2.z /= 2;
								}
								vforce2.x /= 2;	
								vforce2.y /= 2;	
								vforce2.z /= 2;

								PhysApplyForce(obj0P,&vforce);
								PhysApplyRot(obj0P,&vforce2);
								if (gameStates.app.nDifficultyLevel == 0)
									damage /= 4;
								if (obj0P->shields >= 0)
									ApplyDamageToPlayer(obj0P, killer, damage);
							}
								break;

							default:
								Int3();	//	Illegal object type
						}	// end switch
					} else {
						;
					}	// end if (ObjectToObjectVisibility...
				}	// end if (dist < maxdistance)
			}
			obj0P++;
		}	// end for
	}	// end if (maxdamage...

	return explObjP;

}

//------------------------------------------------------------------------------

object *ObjectCreateMuzzleFlash(short segnum, vms_vector * position, fix size, ubyte vclip_type)
{
return ObjectCreateExplosionSub(NULL, segnum, position, size, vclip_type, 0, 0, 0, -1);
}

//------------------------------------------------------------------------------

object *ObjectCreateExplosion(short segnum, vms_vector * position, fix size, ubyte vclip_type)
{
return ObjectCreateExplosionSub(NULL, segnum, position, size, vclip_type, 0, 0, 0, -1);
}

//------------------------------------------------------------------------------

object *ObjectCreateBadassExplosion(
	object *objP, 
	short segnum, 
	vms_vector *position, 
	fix size, 
	ubyte vclip_type, 
	fix maxdamage, 
	fix maxdistance, 
	fix maxforce, 
	short parent)
{
	object	*rval;

	rval = ObjectCreateExplosionSub(objP, segnum, position, size, vclip_type, maxdamage, maxdistance, maxforce, parent);

	if ((objP != NULL) && (objP->type == OBJ_WEAPON))
		CreateSmartChildren(objP, NUM_SMART_CHILDREN);

// -- 	if (objP->type == OBJ_ROBOT)
// -- 		if (gameData.bots.pInfo[objP->id].smart_blobs)
// -- 			CreateSmartChildren(objP, gameData.bots.pInfo[objP->id].smart_blobs);

	return rval;
}

//------------------------------------------------------------------------------
//blows up a badass weapon, creating the badass explosion
//return the explosion object
object *ExplodeBadassWeapon(object *objP,vms_vector *pos)
{
	weapon_info *wi = &gameData.weapons.info[objP->id];

	Assert(wi->damage_radius);

	if ((objP->id == EARTHSHAKER_ID) || (objP->id == ROBOT_EARTHSHAKER_ID))
		ShakerRockStuff();

	DigiLinkSoundToObject(SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F1_0);

	return ObjectCreateBadassExplosion(objP, objP->segnum, pos,
	                                      wi->impact_size,
	                                      wi->robot_hit_vclip,
	                                      wi->strength[gameStates.app.nDifficultyLevel],
	                                      wi->damage_radius,wi->strength[gameStates.app.nDifficultyLevel],
	                                      objP->ctype.laser_info.parent_num);

}

//------------------------------------------------------------------------------

object *ExplodeBadassObject(object *objP, fix damage, fix distance, fix force)
{

	object 	*rval;

	rval = ObjectCreateBadassExplosion(objP, objP->segnum, &objP->pos, objP->size,
					(ubyte) GetExplosionVClip(objP, 0),
					damage, distance, force,
					OBJ_IDX (objP));
	if (rval)
		DigiLinkSoundToObject(SOUND_BADASS_EXPLOSION, OBJ_IDX (rval), 0, F1_0);

	return (rval);

}

//------------------------------------------------------------------------------
//blows up the player with a badass explosion
//return the explosion object
object *ExplodeBadassPlayer(object *objP)
{
	return ExplodeBadassObject(objP, F1_0*50, F1_0*40, F1_0*150);
}

//------------------------------------------------------------------------------
#define DEBRIS_LIFE (f1_0 * 2)		//lifespan in seconds

object *ObjectCreateDebris(object *parentObjP, int subobj_num)
{
	int objnum;
	object *debrisObjP;
	polymodel *po;

	Assert((parentObjP->type == OBJ_ROBOT) || (parentObjP->type == OBJ_PLAYER));

	objnum = CreateObject(
		OBJ_DEBRIS, 0, -1, parentObjP->segnum, &parentObjP->pos, &parentObjP->orient,
		gameData.models.polyModels[parentObjP->rtype.pobj_info.model_num].submodel_rads[subobj_num],
		CT_DEBRIS, MT_PHYSICS, RT_POLYOBJ, 1);

	if ((objnum < 0) && (gameData.objs.nLastObject >= MAX_OBJECTS-1)) {
#if TRACE
		con_printf (1, "Can't create object in ObjectCreateDebris.\n");
#endif
		Int3();
		return NULL;
	}
	if (objnum < 0)
		return NULL;				// Not enough debris slots!
	debrisObjP = &gameData.objs.objects[objnum];

	Assert(subobj_num < 32);

	//Set polygon-object-specific data

	debrisObjP->rtype.pobj_info.model_num = parentObjP->rtype.pobj_info.model_num;
	debrisObjP->rtype.pobj_info.subobj_flags = 1<<subobj_num;
	debrisObjP->rtype.pobj_info.tmap_override = parentObjP->rtype.pobj_info.tmap_override;

	//Set physics data for this object

	po = &gameData.models.polyModels[debrisObjP->rtype.pobj_info.model_num];

	debrisObjP->mtype.phys_info.velocity.x = RAND_MAX/2 - d_rand();
	debrisObjP->mtype.phys_info.velocity.y = RAND_MAX/2 - d_rand();
	debrisObjP->mtype.phys_info.velocity.z = RAND_MAX/2 - d_rand();
	VmVecNormalizeQuick(&debrisObjP->mtype.phys_info.velocity);
	VmVecScale(&debrisObjP->mtype.phys_info.velocity,i2f(10 + (30 * d_rand() / RAND_MAX)));

	VmVecInc(&debrisObjP->mtype.phys_info.velocity,&parentObjP->mtype.phys_info.velocity);

	// -- used to be: Notice, not random! VmVecMake(&debrisObjP->mtype.phys_info.rotvel,10*0x2000/3,10*0x4000/3,10*0x7000/3);
	VmVecMake(&debrisObjP->mtype.phys_info.rotvel, d_rand() + 0x1000, d_rand()*2 + 0x4000, d_rand()*3 + 0x2000);
	VmVecZero(&debrisObjP->mtype.phys_info.rotthrust);

	debrisObjP->lifeleft = 3*DEBRIS_LIFE/4 + fixmul(d_rand(), DEBRIS_LIFE);	//	Some randomness, so they don't all go away at the same time.

	debrisObjP->mtype.phys_info.mass = fixmuldiv(parentObjP->mtype.phys_info.mass,debrisObjP->size,parentObjP->size);
	debrisObjP->mtype.phys_info.drag = 0; //fl2f(0.2);		//parentObjP->mtype.phys_info.drag;

	return debrisObjP;

}

// --------------------------------------------------------------------------------------------------------------------

void DrawFireball(object *objP)
{
	if (objP->lifeleft > 0)
		draw_vclip_object (objP, objP->lifeleft, 0, objP->id, (objP->type == OBJ_WEAPON) ? gameData.weapons.color + objP->id : NULL);

}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if there is a door here and it is openable
//	It is assumed that the player has all keys.
int PlayerCanOpenDoor(segment *segP, short sidenum)
{
	short	wall_num, wall_type;

	wall_num = WallNumP (segP, sidenum);

	if (!IS_WALL (wall_num))
		return 0;						//	no wall here.

	wall_type = gameData.walls.walls[wall_num].type;
	//	Can't open locked doors.
	if (((wall_type == WALL_DOOR) && (gameData.walls.walls[wall_num].flags & WALL_DOOR_LOCKED)) || (wall_type == WALL_CLOSED))
		return 0;

	return 1;

}

#define	QUEUE_SIZE	64

// --------------------------------------------------------------------------------------------------------------------
//	Return a segment %i segments away from initial segment.
//	Returns -1 if can't find a segment that distance away.
int PickConnectedSegment(object *objP, int max_depth)
{
	int		i;
	int		cur_depth;
	int		start_seg;
	int		head, tail;
	int		seg_queue[QUEUE_SIZE*2];
	sbyte		bVisited[MAX_SEGMENTS];
	sbyte		depth[MAX_SEGMENTS];
	sbyte		side_rand[MAX_SIDES_PER_SEGMENT];

	start_seg = objP->segnum;
	head = 0;
	tail = 0;
	seg_queue[head++] = start_seg;

	memset(bVisited, 0, gameData.segs.nLastSegment+1);
	memset(depth, 0, gameData.segs.nLastSegment+1);
	cur_depth = 0;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
		side_rand[i] = i;

	//	Now, randomize a bit to start, so we don't always get started in the same direction.
	for (i=0; i<4; i++) {
		int	ind1, temp;

		ind1 = (d_rand() * MAX_SIDES_PER_SEGMENT) >> 15;
		temp = side_rand[ind1];
		side_rand[ind1] = side_rand[i];
		side_rand[i] = temp;
	}


	while (tail != head) {
		int		sidenum, count;
		segment	*segP;
		int		ind1, ind2, temp;

		if (cur_depth >= max_depth) {
			return seg_queue[tail];
		}

		segP = gameData.segs.segments + seg_queue[tail++];
		tail &= QUEUE_SIZE-1;

		//	to make random, switch a pair of entries in side_rand.
		ind1 = (d_rand() * MAX_SIDES_PER_SEGMENT) >> 15;
		ind2 = (d_rand() * MAX_SIDES_PER_SEGMENT) >> 15;
		temp = side_rand[ind1];
		side_rand[ind1] = side_rand[ind2];
		side_rand[ind2] = temp;

		count = 0;
		for (sidenum=ind1; count<MAX_SIDES_PER_SEGMENT; count++) {
			short	snrand, wall_num;

			if (sidenum == MAX_SIDES_PER_SEGMENT)
				sidenum = 0;

			snrand = side_rand[sidenum];
			wall_num = WallNumP (segP, snrand);
			sidenum++;

			if (((!IS_WALL (wall_num)) && (segP->children[snrand] > -1)) || PlayerCanOpenDoor(segP, snrand)) {
				if (!bVisited[segP->children[snrand]]) {
					seg_queue [head++] = segP->children[snrand];
					bVisited [segP->children[snrand]] = 1;
					depth [segP->children [snrand]] = cur_depth+1;
					head &= QUEUE_SIZE-1;
					if (head > tail) {
						if (head == tail + QUEUE_SIZE-1)
							Int3();	//	queue overflow.  Make it bigger!
					} else
						if (head+QUEUE_SIZE == tail + QUEUE_SIZE-1)
							Int3();	//	queue overflow.  Make it bigger!
				}
			}
		}
		if ((seg_queue[tail] < 0) || (seg_queue[tail] > gameData.segs.nLastSegment)) {
			// -- Int3();	//	Something bad has happened.  Queue is trashed.  --MK, 12/13/94
			return -1;
		}
		cur_depth = depth[seg_queue[tail]];
	}
#if TRACE
	con_printf (CON_DEBUG, "...failed at depth %i, returning -1\n", cur_depth);
#endif
	return -1;
}

//------------------------------------------------------------------------------
#ifdef NETWORK
#define	BASE_NET_DROP_DEPTH	8

int InitObjectCount (object *objP)
{
	int	nFree, nTotal, i, j, bFree;
	short	type = objP->type;
	short	id = objP->id; 

nFree = nTotal = 0;
for (i = 0, objP = gameData.objs.init; i < gameFileInfo.object.count; i++, objP++) {
	if ((objP->type != type) || (objP->id != id))
		continue;
	nTotal++;
	for (bFree = 1, j = gameData.segs.segments [objP->segnum].objects; j != -1; j = gameData.objs.objects [j].next)
		if ((gameData.objs.objects [j].type == type) && (gameData.objs.objects [j].id == id)) {
			bFree = 0;
			break;
			}
	if (bFree)
		nFree++;
	}
return nFree ? -nFree : nTotal;
}

//------------------------------------------------------------------------------

object *FindInitObject (object *objP)
{
	int	h, i, j, bUsed, 
			bUseFree,
			objCount = InitObjectCount (objP);
	short	type = objP->type;
	short	id = objP->id; 

// due to gameData.objs.objects being deleted from the object list when picked up and recreated when dropped,
// cannot determine exact respawn segment, so randomly chose one from all segments where powerups
// of this type had initially been placed in the level.
if (!objCount)		//no gameData.objs.objects of this type had initially been placed in the mine. 
	return NULL;	//can happen with missile packs
d_srand(TimerGetFixedSeconds());
if (bUseFree = (objCount < 0))
	objCount = -objCount;
h = d_rand () % objCount + 1;	
for (i = 0, objP = gameData.objs.init; i < gameFileInfo.object.count; i++, objP++) {
	if ((objP->type != type) || (objP->id != id))
		continue;
	// if the current segment does not contain a powerup of the type being looked for,
	// return that segment
	if (bUseFree) {
		for (bUsed = 0, j = gameData.segs.segments [objP->segnum].objects; j != -1; j = gameData.objs.objects [j].next)
			if ((gameData.objs.objects [j].type == type) && (gameData.objs.objects [j].id == id)) {
				bUsed = 1;
				break;
				}
		if (bUsed)
			continue;
		}
	if (!--h)
		return objP;
	}
return NULL;
}

//	------------------------------------------------------------------------------------------------------

int ReturnFlagHome (object *objP)
{
	object	*initObjP;

if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF) {
	if (gameData.segs.segment2s [objP->segnum].special == ((objP->id == POW_FLAG_RED) ? SEGMENT_IS_GOAL_RED : SEGMENT_IS_GOAL_BLUE))
		return objP->segnum;
	if (initObjP = FindInitObject (objP)) {
	//objP->segnum = initObjP->segnum;
		objP->pos = initObjP->pos;
		objP->orient = initObjP->orient;
		RelinkObject (OBJ_IDX (objP), initObjP->segnum);
		HUDInitMessage(TXT_FLAG_RETURN);
		DigiPlaySample (SOUND_DROP_WEAPON,F1_0);
		MultiSendReturnFlagHome (OBJ_IDX (objP));
		}
	}
return objP->segnum;
}

//	------------------------------------------------------------------------------------------------------

int FindFuelCen (int segnum)
{
	int	i;

for (i = 0; i < gameData.matCens.nFuelCenters; i++)
	if (gameData.matCens.fuelCenters [i].segnum == segnum)
		return i;
return -1;
}

//-----------------------------------------------------------------------------

int CountRooms (void)
{
	int		i;
	xsegment	*segP = gameData.segs.xSegments;

memset (gameData.entropy.nRoomOwners, 0xFF, sizeof (gameData.entropy.nRoomOwners));
memset (gameData.entropy.nTeamRooms, 0, sizeof (gameData.entropy.nTeamRooms));
for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++)
	if ((segP->owner >= 0) && (segP->group >= 0) && 
		 /*(segP->group <= N_MAX_ROOMS) &&*/ (gameData.entropy.nRoomOwners [segP->group] < 0))
		gameData.entropy.nRoomOwners [segP->group] = segP->owner;
for (i = 0; i < N_MAX_ROOMS; i++)
	if (gameData.entropy.nRoomOwners [i] >= 0) {
		gameData.entropy.nTeamRooms [gameData.entropy.nRoomOwners [i]]++;
		gameData.entropy.nTotalRooms++;
		}
return gameData.entropy.nTotalRooms;
}

//-----------------------------------------------------------------------------

void ConquerRoom (int newOwner, int oldOwner, int roomId)
{
	int			f, h, i, j, jj, k, kk, objnum;
	segment		*segP;
	xsegment		*xsegP;
	object		*objP;
	fuelcen_info	*fuelP;
	short			virusGens [MAX_FUEL_CENTERS];

// this loop with
// a) convert all segments with group 'roomId' to newOwner 'newOwner'
// b) count all virus centers newOwner 'newOwner' was owning already,
//    j holding the total number of virus centers, jj holding the number of 
//    virus centers converted from fuel/repair centers
// c) count all virus centers newOwner 'newOwner' has just conquered,
//    k holding the total number of conquered virus centers, kk holding the number of 
//    conquered virus centers converted from fuel/repair centers
// So if j > jj or k < kk, all virus centers placed in virusGens can be converted
// back to their original type
gameData.entropy.nTeamRooms [oldOwner]--;
gameData.entropy.nTeamRooms [newOwner]++;
for (i = 0, j = jj = 0, k = kk = MAX_FUEL_CENTERS, segP = gameData.segs.segments, xsegP = gameData.segs.xSegments; 
	  i <= gameData.segs.nLastSegment; 
	  i++, segP++, xsegP++) {
	if ((xsegP->group == roomId) && (xsegP->owner == oldOwner)) {
		xsegP->owner = newOwner;
		ChangeSegmentTexture (i, oldOwner);
		if (gameData.segs.segment2s [i].special == SEGMENT_IS_ROBOTMAKER) {
			--k;
			if (extraGameInfo [1].entropy.bRevertRooms && (-1 < (f = FindFuelCen (i))) &&
				 (gameData.matCens.origStationTypes [f] != SEGMENT_IS_NOTHING))
				virusGens [--kk] = f;
			for (objnum = segP->objects; objnum >= 0; objnum = objP->next) {
				objP = gameData.objs.objects + objnum;
				if ((objP->type == OBJ_POWERUP) && (objP->type == POW_ENTROPY_VIRUS))
					objP->matcen_creator = newOwner;
				}
			}
		}
	else {
		if ((xsegP->owner == newOwner) && (gameData.segs.segment2s [i].special == SEGMENT_IS_ROBOTMAKER)) {
			j++;
			if (extraGameInfo [1].entropy.bRevertRooms && (-1 < (f = FindFuelCen (i))) &&
				 (gameData.matCens.origStationTypes [f] != SEGMENT_IS_NOTHING))
				virusGens [jj++] = f;
			}
		}
	}
// if the new owner has conquered a virus generator, turn all generators that had
// been turned into virus generators because the new owner had lost his last virus generators
// back to their original type
if (extraGameInfo [1].entropy.bRevertRooms && (jj + (MAX_FUEL_CENTERS - kk)) && ((j > jj) || (k < kk))) {
	if (kk < MAX_FUEL_CENTERS) {
		memcpy (virusGens + jj, virusGens + kk, (MAX_FUEL_CENTERS - kk) * sizeof (short));
		jj += (MAX_FUEL_CENTERS - kk);
		for (j = 0; j < jj; j++) {
			fuelP = gameData.matCens.fuelCenters + virusGens [j];
			h = fuelP->segnum;
			gameData.segs.segment2s [h].special = gameData.matCens.origStationTypes [fuelP - gameData.matCens.fuelCenters];
			FuelCenCreate (gameData.segs.segments + h, gameData.segs.segment2s [h].special);
			ChangeSegmentTexture (h, newOwner);
			}
		}
	}

// check if the other newOwner's last virus center has been conquered
// if so, find a fuel or repair center owned by that and turn it into a virus generator
// preferrably convert repair centers
for (i = 0, h = -1, xsegP = gameData.segs.xSegments; i <= gameData.segs.nLastSegment; i++, xsegP++)
	if (xsegP->owner == oldOwner) 
		switch (gameData.segs.segment2s [i].special) {
			case SEGMENT_IS_ROBOTMAKER:
				return;
			case SEGMENT_IS_FUELCEN:
				if (h < 0)
					h = i;
				break;
			case SEGMENT_IS_REPAIRCEN:
				if ((h < 0) || (gameData.segs.segment2s [h].special == SEGMENT_IS_FUELCEN))
					h = i;
			}
if (h < 0)
	return;
i = gameData.segs.segment2s [h].special;
gameData.segs.segment2s [h].special = SEGMENT_IS_ROBOTMAKER;
MatCenCreate (gameData.segs.segments + h, i);
ChangeSegmentTexture (h, newOwner);
}

//	------------------------------------------------------------------------------------------------------

void StartConquerWarning (void)
{
if (extraGameInfo [1].entropy.bDoConquerWarning) {
	gameStates.sound.nConquerWarningSoundChannel = DigiPlaySample (SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0);
	MultiSendConquerWarning ();
	gameStates.entropy.bConquerWarning = 1;
	}
}

//	------------------------------------------------------------------------------------------------------

void StopConquerWarning (void)
{
if (gameStates.entropy.bConquerWarning) {
	if (gameStates.sound.nConquerWarningSoundChannel >= 0)
		DigiStopSound (gameStates.sound.nConquerWarningSoundChannel);
	MultiSendStopConquerWarning ();
	gameStates.entropy.bConquerWarning = 0;
	}
}

//	------------------------------------------------------------------------------------------------------

int CheckConquerRoom (xsegment *segP)
{
	player	*playerP = gameData.multi.players + gameData.multi.nLocalPlayer;
	int		team = GetTeam (gameData.multi.nLocalPlayer) + 1;
	time_t	t;

gameStates.entropy.bConquering = 0;
if (!(gameData.app.nGameMode & GM_ENTROPY))
	return 0;
#if 0
if (gameStates.entropy.nTimeLastMoved < 0) {
	HUDMessage (0, "you are moving");
	StopConquerWarning ();
	return 0;
	}
if (playerP->shields < 0) {
	HUDMessage (0, "you are dead");
	StopConquerWarning ();
	return 0;
	}
if (playerP->secondary_ammo [PROXIMITY_INDEX] < extraGameInfo [1].nCaptureVirusLimit) {
	HUDMessage (0, "too few viruses");
	StopConquerWarning ();
	return 0;
	}
if (segP->owner < 0) {
	HUDMessage (0, "neutral room");
	StopConquerWarning ();
	return 0;
	}
if (segP->owner == team) {
	HUDMessage (0, "own room");
	StopConquerWarning ();
	return 0;
	}
#else
if ((gameStates.entropy.nTimeLastMoved < 0) || 
	 (playerP->shields < 0) || 
	 (playerP->secondary_ammo [PROXIMITY_INDEX] < extraGameInfo [1].entropy.nCaptureVirusLimit) ||
	 (segP->owner < 0) || 
	 (segP->owner == team)) {
	StopConquerWarning ();
	return 0;
	}
#endif
t = SDL_GetTicks ();
if (!gameStates.entropy.nTimeLastMoved)
	gameStates.entropy.nTimeLastMoved = (int) t;
if (t - gameStates.entropy.nTimeLastMoved < extraGameInfo [1].entropy.nCaptureTimeLimit * 1000) {
	gameStates.entropy.bConquering = 1;
	if (segP->owner > 0)
		StartConquerWarning ();
	return 0;
	}
StopConquerWarning ();
if (segP->owner)
	MultiSendCaptureBonus ((char) gameData.multi.nLocalPlayer);
playerP->secondary_ammo [PROXIMITY_INDEX] -= extraGameInfo [1].entropy.nCaptureVirusLimit;
if (playerP->secondary_ammo [SMART_MINE_INDEX] > extraGameInfo [1].entropy.nBashVirusCapacity)
	playerP->secondary_ammo [SMART_MINE_INDEX] -= extraGameInfo [1].entropy.nBashVirusCapacity;
else
	playerP->secondary_ammo [SMART_MINE_INDEX] = 0;
MultiSendConquerRoom ((char) team, (char) segP->owner, (char) segP->group);
ConquerRoom ((char) team, (char) segP->owner, (char) segP->group);
return 1;
}

//	------------------------------------------------------------------------------------------------------
//	Choose segment to drop a powerup in.
//	For all active net players, try to create a N segment path from the player.  If possible, return that
//	segment.  If not possible, try another player.  After a few tries, use a random segment.
//	Don't drop if control center in segment.
int ChooseDropSegment(object *objP, int *pbFixedPos, int nDropState)
{
	int			pnum = 0;
	short			segnum = -1;
	int			nCurDropDepth;
	int			count;
	short			player_seg;
	vms_vector	tempv, *player_pos;
	fix			nDist;
	int			bUseInitSgm = 
						objP &&
						EGI_FLAG (bFixedRespawns, 0, 0) || 
						(EGI_FLAG (bEnhancedCTF, 0, 0) && 
						 (objP->type == OBJ_POWERUP) && ((objP->id == POW_FLAG_BLUE) || (objP->id == POW_FLAG_RED)));
#if TRACE
con_printf (CON_DEBUG,"ChooseDropSegment:");
#endif
if (bUseInitSgm) {
	object *initObjP = FindInitObject (objP);
	if (initObjP) {
		*pbFixedPos = 1;
		objP->pos = initObjP->pos;
		objP->orient = initObjP->orient;
		return initObjP->segnum;
		}
	}
if (pbFixedPos)
	*pbFixedPos = 0;
d_srand(TimerGetFixedSeconds());
nCurDropDepth = BASE_NET_DROP_DEPTH + ((d_rand() * BASE_NET_DROP_DEPTH*2) >> 15);
player_pos = &gameData.objs.objects [gameData.multi.players[gameData.multi.nLocalPlayer].objnum].pos;
player_seg = gameData.objs.objects [gameData.multi.players[gameData.multi.nLocalPlayer].objnum].segnum;
while ((segnum == -1) && (nCurDropDepth > BASE_NET_DROP_DEPTH/2)) {
	pnum = (d_rand() * gameData.multi.nPlayers) >> 15;
	count = 0;
	while ((count < gameData.multi.nPlayers) && 
				((gameData.multi.players[pnum].connected == 0) || (pnum==gameData.multi.nLocalPlayer) || ((gameData.app.nGameMode & (GM_TEAM|GM_CAPTURE|GM_ENTROPY)) && 
				(GetTeam(pnum)==GetTeam(gameData.multi.nLocalPlayer))))) {
		pnum = (pnum+1)%gameData.multi.nPlayers;
		count++;
		}
	if (count == gameData.multi.nPlayers) {
		//if can't valid non-player person, use the player
		pnum = gameData.multi.nLocalPlayer;
		//return (d_rand() * gameData.segs.nLastSegment) >> 15;
		}
	segnum = PickConnectedSegment(gameData.objs.objects + gameData.multi.players[pnum].objnum, nCurDropDepth);
#if TRACE
	con_printf (CON_DEBUG," %d",segnum);
#endif
	if (segnum == -1) {
		nCurDropDepth--;
		continue;
		}
	if ((gameData.segs.segment2s [segnum].special == SEGMENT_IS_CONTROLCEN) ||
		 (gameData.segs.segment2s [segnum].special == SEGMENT_IS_BLOCKED) ||
		 (gameData.segs.segment2s [segnum].special == SEGMENT_IS_GOAL_BLUE) ||
		 (gameData.segs.segment2s [segnum].special == SEGMENT_IS_GOAL_RED))
		segnum = -1;
	else {	//don't drop in any children of control centers
		int i;
		for (i=0;i<6;i++) {
			int ch = gameData.segs.segments[segnum].children[i];
			if (IS_CHILD(ch) && gameData.segs.segment2s[ch].special == SEGMENT_IS_CONTROLCEN) {
				segnum = -1;
				break;
				}
			}
		}
	//bail if not far enough from original position
	if (segnum != -1) {
		COMPUTE_SEGMENT_CENTER_I(&tempv, segnum);
		nDist = FindConnectedDistance (player_pos, player_seg, &tempv, segnum, -1, WID_FLY_FLAG);
		if ((nDist >= 0) && (nDist < i2f(20) * nCurDropDepth)) {
			segnum = -1;
			}
		}
	nCurDropDepth--;
}
#if TRACE
if (segnum != -1)
	con_printf (CON_DEBUG," dist=%x\n", nDist);
#endif
if (segnum == -1) {
#if TRACE
	con_printf (1, "Warning: Unable to find a connected segment.  Picking a random one.\n");
#endif
	return (d_rand() * gameData.segs.nLastSegment) >> 15;
	}
else
	return segnum;
}

#endif // NETWORK
#ifdef NETWORK
//	------------------------------------------------------------------------------------------------------

void DropPowerups (void)
{
	short	h = gameData.objs.nFirstDropped, i;

while (h >= 0) {
	i = h;
	h = gameData.objs.dropInfo [i].nNextPowerup;
	if (!MaybeDropNetPowerup (i, gameData.objs.dropInfo [i].nPowerupType, CHECK_DROP))
		break;
	}
}

//	------------------------------------------------------------------------------------------------------

RespawnDestroyedWeapon (short nObject)
{
	int	h = gameData.objs.nFirstDropped, i;

while (h >= 0) {
	i = h;
	h = gameData.objs.dropInfo [i].nNextPowerup;
	if ((gameData.objs.dropInfo [i].nObject == nObject) &&
		 (gameData.objs.dropInfo [i].nDropTime < 0)) {
		gameData.objs.dropInfo [i].nDropTime = 0;
		MaybeDropNetPowerup (i, gameData.objs.dropInfo [i].nPowerupType, CHECK_DROP);
		}
	}
}

//	------------------------------------------------------------------------------------------------------

static int AddDropInfo (void)
{
	int	h;

if (gameData.objs.nFreeDropped < 0)
	return -1;
h = gameData.objs.nFreeDropped;
gameData.objs.nFreeDropped = gameData.objs.dropInfo [h].nNextPowerup;
gameData.objs.dropInfo [h].nPrevPowerup = gameData.objs.nLastDropped;
gameData.objs.dropInfo [h].nNextPowerup = -1;
if (gameData.objs.nLastDropped >= 0)
	gameData.objs.dropInfo [gameData.objs.nLastDropped].nNextPowerup = h;
else
	gameData.objs.nFirstDropped =
	gameData.objs.nLastDropped = h;
gameData.objs.nDropped++;
return h;
}

//	------------------------------------------------------------------------------------------------------

static void DelDropInfo (int h)
{
	int	i, j;

if (h < 0)
	return;
i = gameData.objs.dropInfo [h].nPrevPowerup;
j = gameData.objs.dropInfo [h].nNextPowerup;
if (i < 0)
	gameData.objs.nFirstDropped = j;
else
	gameData.objs.dropInfo [i].nNextPowerup = j;
if (j < 0)
	gameData.objs.nLastDropped = i;
else
	gameData.objs.dropInfo [j].nPrevPowerup = i;
gameData.objs.dropInfo [h].nNextPowerup = gameData.objs.nFreeDropped;
gameData.objs.nFreeDropped = h;
gameData.objs.nDropped--;
}

//	------------------------------------------------------------------------------------------------------
//	Drop cloak powerup if in a network game.

int MaybeDropNetPowerup (short objnum, int nPowerupType, int nDropState)
{
if (EGI_FLAG (bImmortalPowerups, 0, 0) || 
		((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP))) {
	short	segnum;
	int h, bFixedPos = 0;
	vms_vector vNewPos;

	MultiSendWeapons (1);
#if 0
	if ((gameData.app.nGameMode & GM_NETWORK) && (nDropState < CHECK_DROP) && (nPowerupType >= 0)) {
		if (gameData.multi.powerupsInMine[nPowerupType] >= gameData.multi.maxPowerupsAllowed[nPowerupType])
			return 0;
		}
#endif
	if (gameData.reactor.bDestroyed || gameStates.app.bEndLevelSequence)
		return 0;
	multiData.create.nLoc = 0;
	if (gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (extraGameInfo [IsMultiGame].nSpawnDelay != 0)) {
		if (nDropState == CHECK_DROP) {
			if ((gameData.objs.dropInfo [objnum].nDropTime < 0) ||
				 (gameStates.app.nSDLTicks - gameData.objs.dropInfo [objnum].nDropTime < 
				  extraGameInfo [IsMultiGame].nSpawnDelay))
				return 0;
			nDropState = EXEC_DROP;
			}
		else if (nDropState == INIT_DROP) {
			if (0 > (h = AddDropInfo ()))
				return 0;
			gameData.objs.dropInfo [h].nObject = objnum;
			gameData.objs.dropInfo [h].nPowerupType = nPowerupType;
			gameData.objs.dropInfo [h].nDropTime = 
				(extraGameInfo [IsMultiGame].nSpawnDelay < 0) ? -1 : gameStates.app.nSDLTicks;
			return 0;
			}
		if (nDropState == EXEC_DROP) {
			DelDropInfo (objnum);
			}
		}
	objnum = CallObjectCreateEgg (gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].objnum, 
											1, OBJ_POWERUP, nPowerupType);
	if (objnum < 0)
		return 0;
	segnum = ChooseDropSegment (gameData.objs.objects + objnum, &bFixedPos, nDropState);
	if (bFixedPos)
		vNewPos = gameData.objs.objects[objnum].pos;
	else
		PickRandomPointInSeg (&vNewPos, segnum);
	MultiSendCreatePowerup (nPowerupType, segnum, objnum, &vNewPos);
	if (!bFixedPos)
		gameData.objs.objects[objnum].pos = vNewPos;
	VmVecZero(&gameData.objs.objects[objnum].mtype.phys_info.velocity);
	RelinkObject(objnum, segnum);
	ObjectCreateExplosion(segnum, &vNewPos, i2f(5), VCLIP_POWERUP_DISAPPEARANCE);
	return 1;
	}
return 0;
}
#endif

//	------------------------------------------------------------------------------------------------------
//	Return true if current segment contains some object.
int SegmentContainsObject(int obj_type, int obj_id, int segnum)
{
	int	objnum;

if (segnum == -1)
	return 0;
objnum = gameData.segs.segments[segnum].objects;
while (objnum != -1)
	if ((gameData.objs.objects[objnum].type == obj_type) && (gameData.objs.objects[objnum].id == obj_id))
		return 1;
	else
		objnum = gameData.objs.objects[objnum].next;
return 0;
}

//	------------------------------------------------------------------------------------------------------
int ObjectNearbyAux(int segnum, int object_type, int object_id, int depth)
{
	int	i, seg2;

if (depth == 0)
	return 0;
if (SegmentContainsObject(object_type, object_id, segnum))
	return 1;
for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
	seg2 = gameData.segs.segments[segnum].children [i];
	if (seg2 != -1)
		if (ObjectNearbyAux(seg2, object_type, object_id, depth-1))
			return 1;
		}
	return 0;
}


//	------------------------------------------------------------------------------------------------------
//	Return true if some powerup is nearby (within 3 segments).
int WeaponNearby(object *objP, int weapon_id)
{
	return ObjectNearbyAux(objP->segnum, OBJ_POWERUP, weapon_id, 3);
}

//	------------------------------------------------------------------------------------------------------

void MaybeReplacePowerupWithEnergy(object *del_obj)
{
	int	weapon_index=-1;

	if (del_obj->contains_type != OBJ_POWERUP)
		return;

	if (del_obj->contains_id == POW_CLOAK) {
		if (WeaponNearby(del_obj, del_obj->contains_id)) {
#if TRACE
			con_printf (CON_DEBUG, "Bashing cloak into nothing because there's one nearby.\n");
#endif
			del_obj->contains_count = 0;
		}
		return;
	}
	switch (del_obj->contains_id) {
		case POW_VULCAN_WEAPON:			
			weapon_index = VULCAN_INDEX;		
			break;
		case POW_GAUSS_WEAPON:			
			weapon_index = GAUSS_INDEX;		
			break;
		case POW_SPREADFIRE_WEAPON:	
			weapon_index = SPREADFIRE_INDEX;	
			break;
		case POW_PLASMA_WEAPON:			
			weapon_index = PLASMA_INDEX;		
			break;
		case POW_FUSION_WEAPON:			
			weapon_index = FUSION_INDEX;		
			break;
		case POW_HELIX_WEAPON:			
			weapon_index = HELIX_INDEX;		
			break;
		case POW_PHOENIX_WEAPON:		
			weapon_index = PHOENIX_INDEX;		
			break;
		case POW_OMEGA_WEAPON:			
			weapon_index = OMEGA_INDEX;		
			break;

	}

	//	Don't drop vulcan ammo if player maxed out.
	if (((weapon_index == VULCAN_INDEX) || (del_obj->contains_id == POW_VULCAN_AMMO)) && (gameData.multi.players[gameData.multi.nLocalPlayer].primary_ammo[VULCAN_INDEX] >= VULCAN_AMMO_MAX))
		del_obj->contains_count = 0;
	else if (((weapon_index == GAUSS_INDEX) || (del_obj->contains_id == POW_VULCAN_AMMO)) && (gameData.multi.players[gameData.multi.nLocalPlayer].primary_ammo[VULCAN_INDEX] >= VULCAN_AMMO_MAX))
		del_obj->contains_count = 0;
	else if (weapon_index != -1) {
		if ((PlayerHasWeapon(weapon_index, 0, -1) & HAS_WEAPON_FLAG) || 
			 WeaponNearby(del_obj, del_obj->contains_id)) {
			if (d_rand() > 16384) {
				del_obj->contains_type = OBJ_POWERUP;
				if (weapon_index == VULCAN_INDEX) {
					del_obj->contains_id = POW_VULCAN_AMMO;
				} else if (weapon_index == GAUSS_INDEX) {
					del_obj->contains_id = POW_VULCAN_AMMO;
				} else {
					del_obj->contains_id = POW_ENERGY;
				}
			} else {
				del_obj->contains_type = OBJ_POWERUP;
				del_obj->contains_id = POW_SHIELD_BOOST;
			}
		}
	} else if (del_obj->contains_id == POW_QUAD_FIRE)
		if ((gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_QUAD_LASERS) || WeaponNearby(del_obj, del_obj->contains_id)) {
			if (d_rand() > 16384) {
				del_obj->contains_type = OBJ_POWERUP;
				del_obj->contains_id = POW_ENERGY;
			} else {
				del_obj->contains_type = OBJ_POWERUP;
				del_obj->contains_id = POW_SHIELD_BOOST;
			}
		}

	//	If this robot was gated in by the boss and it now contains energy, make it contain nothing,
	//	else the room gets full of energy.
	if ((del_obj->matcen_creator == BOSS_GATE_MATCEN_NUM) && (del_obj->contains_id == POW_ENERGY) && (del_obj->contains_type == OBJ_POWERUP)) {
#if TRACE
		con_printf (CON_DEBUG, "Converting energy powerup to nothing because robot %i gated in by boss.\n", OBJ_IDX (del_obj));
#endif
		del_obj->contains_count = 0;
	}

	// Change multiplayer extra-lives into invulnerability
	if ((gameData.app.nGameMode & GM_MULTI) && (del_obj->contains_id == POW_EXTRA_LIFE))
	{
		del_obj->contains_id = POW_INVULNERABILITY;
	}
}

//------------------------------------------------------------------------------

int DropPowerup(ubyte type, ubyte id, short owner, int num, vms_vector *init_vel, vms_vector *pos, short segnum)
{
	short			objnum=-1;
	object		*objP;
	vms_vector	new_velocity, vNewPos;
	fix			old_mag;
   int			count;

switch (type) {
	case OBJ_POWERUP:
		for (count = 0; count < num; count++) {
			int	rand_scale;
			new_velocity = *init_vel;
			old_mag = VmVecMagQuick(init_vel);

			//	We want powerups to move more in network mode.
			if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_ROBOTS)) {
				rand_scale = 4;
				//	extra life powerups are converted to invulnerability in multiplayer, for what is an extra life, anyway?
				if (id == POW_EXTRA_LIFE)
					id = POW_INVULNERABILITY;
				}
			else
				rand_scale = 2;
			new_velocity.x += fixmul(old_mag+F1_0*32, d_rand()*rand_scale - 16384*rand_scale);
			new_velocity.y += fixmul(old_mag+F1_0*32, d_rand()*rand_scale - 16384*rand_scale);
			new_velocity.z += fixmul(old_mag+F1_0*32, d_rand()*rand_scale - 16384*rand_scale);
			// Give keys zero velocity so they can be tracked better in multi
			if ((gameData.app.nGameMode & GM_MULTI) && 
				 (((id >= POW_KEY_BLUE) && (id <= POW_KEY_GOLD)) || (id == POW_MONSTERBALL)))
				VmVecZero(&new_velocity);
			vNewPos = *pos;

#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI) {	
				if (multiData.create.nLoc >= MAX_NET_CREATE_OBJECTS) {
#if TRACE
					con_printf (1, "Not enough slots to drop all powerups!\n");
#endif
					return -1;
					}
#if 0
				if (TooManyPowerups (id)) {
#if TRACE
					con_printf (1, "More powerups of type %d in mine than at start of game!\n", id);
#endif
					return -1;
					}
#endif					
				if ((gameData.app.nGameMode & GM_NETWORK) && networkData.nStatus == NETSTAT_ENDLEVEL)
					return -1;
				}
#endif
			objnum = CreateObject (type, id, owner, segnum, &vNewPos, &vmdIdentityMatrix, gameData.objs.pwrUp.info[id].size, 
										  CT_POWERUP, MT_PHYSICS, RT_POWERUP, 0);
			if (objnum < 0) {
#if TRACE
				con_printf (1, "Can't create object in ObjectCreateEgg.  Aborting.\n");
#endif
				Int3();
				return objnum;
				}
#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI)
				multiData.create.nObjNums[multiData.create.nLoc++] = objnum;
#endif
			objP = gameData.objs.objects + objnum;
			objP->mtype.phys_info.velocity = new_velocity;
			objP->mtype.phys_info.drag = 512;	//1024;
			objP->mtype.phys_info.mass = F1_0;
			objP->mtype.phys_info.flags = PF_BOUNCE;
			objP->rtype.vclip_info.nClipIndex = gameData.objs.pwrUp.info[objP->id].nClipIndex;
			objP->rtype.vclip_info.xFrameTime = gameData.eff.vClips [0][objP->rtype.vclip_info.nClipIndex].xFrameTime;
			objP->rtype.vclip_info.nCurFrame = 0;

			switch (objP->id) {
				case POW_MISSILE_1:
				case POW_MISSILE_4:
				case POW_SHIELD_BOOST:
				case POW_ENERGY:
					objP->lifeleft = (d_rand() + F1_0*3) * 64;		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
					if (gameData.app.nGameMode & GM_MULTI)
						objP->lifeleft /= 2;
					break;
				default:
//						if (gameData.app.nGameMode & GM_MULTI)
//							objP->lifeleft = (d_rand() + F1_0*3) * 64;		//	Lives for 5 to 5.5 binary minutes (a binary minute is 64 seconds)
					break;
				}
			}
		break;

	case OBJ_ROBOT:
		for (count=0; count<num; count++) {
			int	rand_scale;
			new_velocity = *init_vel;
			old_mag = VmVecMagQuick(init_vel);
			VmVecNormalizeQuick(&new_velocity);
			//	We want powerups to move more in network mode.
			rand_scale = 2;
			new_velocity.x += (d_rand()-16384)*2;
			new_velocity.y += (d_rand()-16384)*2;
			new_velocity.z += (d_rand()-16384)*2;
			VmVecNormalizeQuick(&new_velocity);
			VmVecScale(&new_velocity, (F1_0*32 + old_mag) * rand_scale);
			vNewPos = *pos;
			//	This is dangerous, could be outside mine.
//				vNewPos.x += (d_rand()-16384)*8;
//				vNewPos.y += (d_rand()-16384)*7;
//				vNewPos.z += (d_rand()-16384)*6;
			objnum = CreateObject(OBJ_ROBOT, id, -1, segnum, &vNewPos, &vmdIdentityMatrix, 
										 gameData.models.polyModels [gameData.bots.pInfo[id].model_num].rad, 
										 CT_AI, MT_PHYSICS, RT_POLYOBJ, 1);
			if (objnum < 0) {
#if TRACE
				con_printf (1, "Can't create object in ObjectCreateEgg, robots.  Aborting.\n");
#endif
				Int3();
				return objnum;
				}
#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI)
				multiData.create.nObjNums[multiData.create.nLoc++] = objnum;
#endif
			objP = &gameData.objs.objects[objnum];
			//Set polygon-object-specific data
			objP->rtype.pobj_info.model_num = gameData.bots.pInfo[objP->id].model_num;
			objP->rtype.pobj_info.subobj_flags = 0;
			//set Physics info
			objP->mtype.phys_info.velocity = new_velocity;
			objP->mtype.phys_info.mass = gameData.bots.pInfo[objP->id].mass;
			objP->mtype.phys_info.drag = gameData.bots.pInfo[objP->id].drag;
			objP->mtype.phys_info.flags |= (PF_LEVELLING);
			objP->shields = gameData.bots.pInfo[objP->id].strength;
			objP->ctype.ai_info.behavior = AIB_NORMAL;
			gameData.ai.localInfo[OBJ_IDX (objP)].player_awareness_type = PA_WEAPON_ROBOT_COLLISION;
			gameData.ai.localInfo[OBJ_IDX (objP)].player_awareness_time = F1_0*3;
			objP->ctype.ai_info.CURRENT_STATE = AIS_LOCK;
			objP->ctype.ai_info.GOAL_STATE = AIS_LOCK;
			objP->ctype.ai_info.REMOTE_OWNER = -1;
			}
		// At JasenW's request, robots which contain robots
		// sometimes drop shields.
		if (d_rand() > 16384)
			DropPowerup(OBJ_POWERUP, POW_SHIELD_BOOST, -1, 1, init_vel, pos, segnum);
		break;

	default:
		Error("Error: Illegal type (%i) in object spawning.\n", type);
	}
return objnum;
}

// ----------------------------------------------------------------------------
// Returns created object number.
// If object dropped by player, set flag.
int ObjectCreateEgg (object *objP)
{
	int	rval;

if (!(gameData.app.nGameMode & GM_MULTI) & (objP->type != OBJ_PLAYER)) {
	if (objP->contains_type == OBJ_POWERUP) {
		if (objP->contains_id == POW_SHIELD_BOOST) {
			if (gameData.multi.players[gameData.multi.nLocalPlayer].shields >= i2f(100)) {
				if (d_rand() > 16384) {
#if TRACE
					con_printf (CON_DEBUG, "Not dropping shield!\n");
#endif
					return -1;
					}
				} 
			else  if (gameData.multi.players[gameData.multi.nLocalPlayer].shields >= i2f(150)) {
				if (d_rand() > 8192) {
#if TRACE
					con_printf (CON_DEBUG, "Not dropping shield!\n");
#endif
					return -1;
					}
				}
			} 
		else if (objP->contains_id == POW_ENERGY) {
			if (gameData.multi.players[gameData.multi.nLocalPlayer].energy >= i2f(100)) {
				if (d_rand() > 16384) {
#if TRACE
					con_printf (CON_DEBUG, "Not dropping energy!\n");
#endif
					return -1;
					}
				} 
			else if (gameData.multi.players[gameData.multi.nLocalPlayer].energy >= i2f(150)) {
				if (d_rand() > 8192) {
#if TRACE
					con_printf (CON_DEBUG, "Not dropping energy!\n");
#endif
					return -1;
					}
				}
			}
		}
	}

rval = DropPowerup (
	objP->contains_type, 
	(ubyte) objP->contains_id, 
	(short) (
		((gameData.app.nGameMode & GM_ENTROPY) && 
		 (objP->contains_type == OBJ_POWERUP) && 
		 (objP->contains_id == POW_ENTROPY_VIRUS) && 
		 (objP->type == OBJ_PLAYER)) ? 
		GetTeam (objP->id) + 1 : -1
		),
	objP->contains_count, 
	&objP->mtype.phys_info.velocity, 
	&objP->pos, objP->segnum);
if (rval != -1) {
	if ((objP->type == OBJ_PLAYER) && (objP->id == gameData.multi.nLocalPlayer))
		gameData.objs.objects[rval].flags |= OF_PLAYER_DROPPED;
	if (objP->type == OBJ_ROBOT && objP->contains_type==OBJ_POWERUP) {
		if (objP->contains_id==POW_VULCAN_WEAPON || objP->contains_id==POW_GAUSS_WEAPON)
			gameData.objs.objects[rval].ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
		else if (objP->contains_id==POW_OMEGA_WEAPON)
			gameData.objs.objects[rval].ctype.powerup_info.count = MAX_OMEGA_CHARGE;
		}
	}
return rval;
}

// -- extern int Items_destroyed;

//	-------------------------------------------------------------------------------------------------------
//	Put count gameData.objs.objects of type type (eg, powerup), id = id (eg, energy) into *objP, then drop them!  Yippee!
//	Returns created object number.
int CallObjectCreateEgg(object *objP, int count, int type, int id)
{
// -- 	if (!(gameData.app.nGameMode & GM_MULTI) && (objP == gameData.objs.console))
// -- 		if (d_rand() < 32767/6) {
// -- 			Items_destroyed++;
// -- 			return -1;
// -- 		}

if (count <= 0)
	return -1;
objP->contains_count = count;
objP->contains_type = type;
objP->contains_id = id;
return ObjectCreateEgg (objP);
}

//------------------------------------------------------------------------------
//what vclip does this explode with?
short GetExplosionVClip(object *obj,int stage)
{
	if (obj->type==OBJ_ROBOT) {

		if (stage==0 && gameData.bots.pInfo[obj->id].exp1_vclip_num>-1)
				return gameData.bots.pInfo[obj->id].exp1_vclip_num;
		else if (stage==1 && gameData.bots.pInfo[obj->id].exp2_vclip_num>-1)
				return gameData.bots.pInfo[obj->id].exp2_vclip_num;

	}
	else if (obj->type==OBJ_PLAYER && gameData.pig.ship.player->expl_vclip_num>-1)
			return gameData.pig.ship.player->expl_vclip_num;

	return VCLIP_SMALL_EXPLOSION;		//default
}

//------------------------------------------------------------------------------
//blow up a polygon model
void ExplodePolyModel(object *obj)
{
	Assert(obj->render_type == RT_POLYOBJ);

	if (Dying_modelnums[obj->rtype.pobj_info.model_num] != -1)
		obj->rtype.pobj_info.model_num = Dying_modelnums[obj->rtype.pobj_info.model_num];

	if (gameData.models.polyModels[obj->rtype.pobj_info.model_num].n_models > 1) {
		int i;

		for (i=1;i<gameData.models.polyModels[obj->rtype.pobj_info.model_num].n_models;i++)
			if (!(obj->type == OBJ_ROBOT && obj->id == 44 && i == 5)) 	//energy sucker energy part
				ObjectCreateDebris(obj,i);

		//make parent object only draw center part
		obj->rtype.pobj_info.subobj_flags=1;
	}
}

//------------------------------------------------------------------------------
//if the object has a destroyed model, switch to it.  Otherwise, delete it.
void MaybeDeleteObject(object *del_obj)
{
	if (Dead_modelnums[del_obj->rtype.pobj_info.model_num] != -1) {
		del_obj->rtype.pobj_info.model_num = Dead_modelnums[del_obj->rtype.pobj_info.model_num];
		del_obj->flags |= OF_DESTROYED;
	}
	else {		//normal, multi-stage explosion
		if (del_obj->type == OBJ_PLAYER)
			del_obj->render_type = RT_NONE;
		else
			del_obj->flags |= OF_SHOULD_BE_DEAD;
	}
}

//	-------------------------------------------------------------------------------------------------------
//blow up an object.  Takes the object to destroy, and the point of impact
void ExplodeObject(object *hitobj,fix delay_time)
{
	if (hitobj->flags & OF_EXPLODING) 
		return;

	if (delay_time) {		//wait a little while before creating explosion
		int objnum;
		object *obj;

		//create a placeholder object to do the delay, with id==-1
		objnum = CreateObject (OBJ_FIREBALL, -1, -1, hitobj->segnum, &hitobj->pos, &vmdIdentityMatrix, 0,
									  CT_EXPLOSION, MT_NONE, RT_NONE, 1);
		if (objnum < 0) {
			MaybeDeleteObject(hitobj);		//no explosion, die instantly
#if TRACE
			con_printf (1,"Couldn't start explosion, deleting object now\n");
#endif
			Int3();
			return;
		}
		obj = gameData.objs.objects + objnum;
		//now set explosion-specific data
		obj->lifeleft = delay_time;
		obj->ctype.expl_info.delete_objnum = OBJ_IDX (hitobj);
#ifndef NDEBUG
		if (obj->ctype.expl_info.delete_objnum < 0)
		 Int3(); // See Rob!
#endif
		obj->ctype.expl_info.delete_time = -1;
		obj->ctype.expl_info.spawn_time = 0;
	}
	else {
		object *expl_obj;
		ubyte vclip_num;

		vclip_num = (ubyte) GetExplosionVClip(hitobj,0);
		expl_obj = ObjectCreateExplosion(hitobj->segnum, &hitobj->pos, fixmul(hitobj->size,EXPLOSION_SCALE), vclip_num);
		if (! expl_obj) {
			MaybeDeleteObject(hitobj);		//no explosion, die instantly
#if TRACE
			con_printf (CON_DEBUG,"Couldn't start explosion, deleting object now\n");
#endif
			return;
		}

		//don't make debris explosions have physics, because they often
		//happen when the debris has hit the wall, so the fireball is trying
		//to move into the wall, which shows off FVI problems.   	
		if (hitobj->type!=OBJ_DEBRIS && hitobj->movement_type==MT_PHYSICS) {
			expl_obj->movement_type = MT_PHYSICS;
			expl_obj->mtype.phys_info = hitobj->mtype.phys_info;
		}
		if (hitobj->render_type==RT_POLYOBJ && hitobj->type!=OBJ_DEBRIS)
			ExplodePolyModel(hitobj);
		MaybeDeleteObject(hitobj);
	}
	hitobj->flags |= OF_EXPLODING;		//say that this is blowing up
	hitobj->control_type = CT_NONE;		//become inert while exploding
}

//------------------------------------------------------------------------------
//do whatever needs to be done for this piece of debris for this frame
void DoDebrisFrame(object *obj)
{
	Assert(obj->control_type == CT_DEBRIS);

	if (obj->lifeleft < 0)
		ExplodeObject(obj,0);

}

//------------------------------------------------------------------------------

extern void DropStolenItems(object *objP);

//do whatever needs to be done for this explosion for this frame
void DoExplosionSequence(object *obj)
{
	int t;

	Assert(obj->control_type == CT_EXPLOSION);

	//See if we should die of old age
	if (obj->lifeleft <= 0) 	{	// We died of old age
		obj->flags |= OF_SHOULD_BE_DEAD;
		obj->lifeleft = 0;
	}

	//See if we should create a secondary explosion
	if (obj->lifeleft <= obj->ctype.expl_info.spawn_time) {
		object *expl_obj, *del_obj = gameData.objs.objects + obj->ctype.expl_info.delete_objnum;
		ubyte vclip_num;
		vms_vector *spawn_pos;
		fix badass = (fix) gameData.bots.pInfo [del_obj->id].badass;

		if ((obj->ctype.expl_info.delete_objnum < 0) || (obj->ctype.expl_info.delete_objnum > gameData.objs.nLastObject)) {
#if TRACE
			con_printf (CON_DEBUG, "Illegal value for delete_objnum in fireball.c\n");
#endif
			Int3(); // get Rob, please... thanks
			return;
		}
		spawn_pos = &del_obj->pos;
		t = del_obj->type;
		if (((t != OBJ_ROBOT) && (t != OBJ_CLUTTER) && (t != OBJ_CNTRLCEN) && (t != OBJ_PLAYER)) || (del_obj->segnum == -1)) {
			Int3();	//pretty bad
			return;
		}
		vclip_num = (ubyte) GetExplosionVClip (del_obj, 1);
		if (del_obj->type == OBJ_ROBOT && badass)
			expl_obj = ObjectCreateBadassExplosion (
				NULL, 
				del_obj->segnum, 
				spawn_pos, 
				fixmul (del_obj->size, EXPLOSION_SCALE), 
				vclip_num, 
				F1_0 * badass, 
				i2f(4) * badass, 
				i2f(35) * badass, 
				-1);
		else
			expl_obj = ObjectCreateExplosion(del_obj->segnum, spawn_pos, fixmul(del_obj->size, EXPLOSION_SCALE), vclip_num);

		if ((del_obj->contains_count > 0) && !(gameData.app.nGameMode & GM_MULTI)) { // Multiplayer handled outside of this code!!
			//	If dropping a weapon that the player has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
			if (del_obj->contains_type == OBJ_POWERUP)
				MaybeReplacePowerupWithEnergy(del_obj);
			if ((del_obj->contains_type != OBJ_ROBOT) || !(del_obj->flags & OF_ARMAGEDDON))
				ObjectCreateEgg(del_obj);
			}
		else if ((del_obj->type == OBJ_ROBOT) && !(gameData.app.nGameMode & GM_MULTI)) { // Multiplayer handled outside this code!!
			robot_info	*robptr = &gameData.bots.pInfo[del_obj->id];
			if (robptr->contains_count && ((robptr->contains_type != OBJ_ROBOT) || !(del_obj->flags & OF_ARMAGEDDON))) {
				if (((d_rand() * 16) >> 15) < robptr->contains_prob) {
					del_obj->contains_count = ((d_rand() * robptr->contains_count) >> 15) + 1;
					del_obj->contains_type = robptr->contains_type;
					del_obj->contains_id = robptr->contains_id;
					MaybeReplacePowerupWithEnergy(del_obj);
					ObjectCreateEgg(del_obj);
				}
			}

			if (robptr->thief)
				DropStolenItems(del_obj);

			if (robptr->companion) {
				DropBuddyMarker(del_obj);
			}
		}

		if (gameData.bots.pInfo[del_obj->id].exp2_sound_num > -1)
			DigiLinkSoundToPos(gameData.bots.pInfo[del_obj->id].exp2_sound_num, del_obj->segnum, 0, spawn_pos, 0, F1_0);
			//PLAY_SOUND_3D(gameData.bots.pInfo[del_obj->id].exp2_sound_num, spawn_pos, del_obj->segnum);

		obj->ctype.expl_info.spawn_time = -1;

		//make debris
		if (del_obj->render_type==RT_POLYOBJ)
			ExplodePolyModel(del_obj);		//explode a polygon model

		//set some parm in explosion
		if (expl_obj) {

			if (del_obj->movement_type == MT_PHYSICS) {
				expl_obj->movement_type = MT_PHYSICS;
				expl_obj->mtype.phys_info = del_obj->mtype.phys_info;
			}

			expl_obj->ctype.expl_info.delete_time = expl_obj->lifeleft/2;
			expl_obj->ctype.expl_info.delete_objnum = OBJ_IDX (del_obj);
#ifndef NDEBUG
			if (obj->ctype.expl_info.delete_objnum < 0)
		  		Int3(); // See Rob!
#endif

		}
		else {
			MaybeDeleteObject(del_obj);
#if TRACE
			con_printf (CON_DEBUG,"Couldn't create secondary explosion, deleting object now\n");
#endif
		}

	}

	//See if we should delete an object
	if (obj->lifeleft <= obj->ctype.expl_info.delete_time) {
		object *del_obj = gameData.objs.objects+obj->ctype.expl_info.delete_objnum;

		obj->ctype.expl_info.delete_time = -1;

		MaybeDeleteObject(del_obj);
	}
}

//------------------------------------------------------------------------------
#define EXPL_WALL_TIME					(f1_0)
#define EXPL_WALL_TOTAL_FIREBALLS	32
#define EXPL_WALL_FIREBALL_SIZE 		(0x48000*6/10)	//smallest size

void InitExplodingWalls()
{
	int i;

	for (i=0;i<MAX_EXPLODING_WALLS;i++)
		gameData.walls.explWalls[i].segnum = -1;
}

//------------------------------------------------------------------------------
//explode the given wall
void ExplodeWall(short segnum, short sidenum)
{
	int i;
	vms_vector pos;

	//find a d_free slot

	for (i=0;i<MAX_EXPLODING_WALLS && gameData.walls.explWalls[i].segnum != -1;i++);

	if (i==MAX_EXPLODING_WALLS) {		//didn't find slot.
#if TRACE
		con_printf (CON_DEBUG,"Couldn't find d_free slot for exploding wall!\n");
#endif
		Int3();
		return;
	}

	gameData.walls.explWalls[i].segnum = segnum;
	gameData.walls.explWalls[i].sidenum = sidenum;
	gameData.walls.explWalls[i].time = 0;

	//play one long sound for whole door wall explosion
	COMPUTE_SIDE_CENTER(&pos,&gameData.segs.segments[segnum], sidenum);
	DigiLinkSoundToPos(SOUND_EXPLODING_WALL, segnum, sidenum, &pos, 0, F1_0);

}

//------------------------------------------------------------------------------
//handle walls for this frame
//note: this wall code assumes the wall is not triangulated
void DoExplodingWallFrame()
{
	int i;

	for (i=0;i<MAX_EXPLODING_WALLS;i++) {
		short segnum = gameData.walls.explWalls[i].segnum;
		if (segnum != -1) {
			short sidenum = gameData.walls.explWalls[i].sidenum;
			fix oldfrac,newfrac;
			int old_count,new_count,e;		//n,
			oldfrac = fixdiv(gameData.walls.explWalls[i].time,EXPL_WALL_TIME);
			gameData.walls.explWalls[i].time += gameData.app.xFrameTime;
			if (gameData.walls.explWalls[i].time > EXPL_WALL_TIME)
				gameData.walls.explWalls[i].time = EXPL_WALL_TIME;
			if (gameData.walls.explWalls[i].time>(EXPL_WALL_TIME*3)/4) {
				segment *seg,*csegp;
				short cside, n;
				ubyte	a;
				seg = gameData.segs.segments + segnum;
				a = (ubyte) gameData.walls.walls [WallNumP (seg, sidenum)].clip_num;
				n = AnimFrameCount (gameData.walls.pAnims + a);
				csegp = gameData.segs.segments + seg->children[sidenum];
				cside = FindConnectedSide(seg, csegp);
				WallSetTMapNum(seg,sidenum, csegp, cside, a, n - 1);
				gameData.walls.walls[WallNumP (seg, sidenum)].flags |= WALL_BLASTED;
				if (cside >= 0)
					gameData.walls.walls[WallNumP (csegp, cside)].flags |= WALL_BLASTED;
			}
			newfrac = fixdiv(gameData.walls.explWalls[i].time,EXPL_WALL_TIME);
			old_count = f2i(EXPL_WALL_TOTAL_FIREBALLS * fixmul(oldfrac,oldfrac));
			new_count = f2i(EXPL_WALL_TOTAL_FIREBALLS * fixmul(newfrac,newfrac));
			//n = new_count - old_count;
			//now create all the next explosions
			for (e=old_count;e<new_count;e++) {
				short			vertnum_list[4];
				vms_vector	*v0,*v1,*v2;
				vms_vector	vv0,vv1,pos;
				fix			size;

				//calc expl position

				GetSideVerts(vertnum_list,segnum,sidenum);

				v0 = gameData.segs.vertices + vertnum_list[0];
				v1 = gameData.segs.vertices + vertnum_list[1];
				v2 = gameData.segs.vertices + vertnum_list[2];

				VmVecSub(&vv0,v0,v1);
				VmVecSub(&vv1,v2,v1);

				VmVecScaleAdd(&pos,v1,&vv0,d_rand()*2);
				VmVecScaleInc(&pos,&vv1,d_rand()*2);

				size = EXPL_WALL_FIREBALL_SIZE + (2*EXPL_WALL_FIREBALL_SIZE * e / EXPL_WALL_TOTAL_FIREBALLS);

				//fireballs start away from door, with subsequent ones getting closer
				#ifdef COMPACT_SEGS	
					{
					vms_vector _vn;
					get_side_normal(gameData.segs.segments + segnum, sidenum, 0, &_vn);
					VmVecScaleInc(&pos,&_vn,size*(EXPL_WALL_TOTAL_FIREBALLS-e)/EXPL_WALL_TOTAL_FIREBALLS);
					}
				#else
					VmVecScaleInc(&pos,gameData.segs.segments[segnum].sides[sidenum].normals,size*(EXPL_WALL_TOTAL_FIREBALLS-e)/EXPL_WALL_TOTAL_FIREBALLS);
				#endif

				if (e & 3)		//3 of 4 are normal
					ObjectCreateExplosion((short) gameData.walls.explWalls[i].segnum, &pos, size, (ubyte) VCLIP_SMALL_EXPLOSION);
				else
					ObjectCreateBadassExplosion(NULL, (short) gameData.walls.explWalls[i].segnum, &pos,
					size,
					(ubyte) VCLIP_SMALL_EXPLOSION,
					i2f(4),		// damage strength
					i2f(20),		//	damage radius
					i2f(50),		//	damage force
					-1		//	parent id
					);


			}

			if (gameData.walls.explWalls[i].time >= EXPL_WALL_TIME)
				gameData.walls.explWalls[i].segnum = -1;	//flag this slot as d_free

		}
	}

}

//------------------------------------------------------------------------------
//creates afterburner blobs behind the specified object
void DropAfterburnerBlobs (object *objP, int count, fix size_scale, fix lifetime, object *pParent, int bThruster)
{
	vms_vector pos_left,pos_right;
	short segnum;
	object	*blobObjP;

	VmVecScaleAdd (&pos_left, &objP->pos, &objP->orient.fvec, -objP->size);
	VmVecScaleInc (&pos_left, &objP->orient.rvec, -(8 * objP->size / 50));
	VmVecScaleInc (&pos_left, &objP->orient.uvec, -(objP->size / 50));
	VmVecScaleAdd (&pos_right, &pos_left, &objP->orient.rvec, 8 * objP->size / 25);

	if (count == 1)
		VmVecAvg(&pos_left, &pos_left, &pos_right);
	segnum = FindSegByPoint (&pos_left, objP->segnum);
	if (segnum != -1)
		if (blobObjP = ObjectCreateExplosion (segnum, &pos_left, size_scale, VCLIP_AFTERBURNER_BLOB)) {
			if (lifetime != -1)
				blobObjP->lifeleft = lifetime;
			AddChildObjectP (pParent, blobObjP);
			if (bThruster) {
				blobObjP->render_type = RT_THRUSTER;
				blobObjP->mtype.phys_info.flags |= PF_WIGGLE;
				}
			}

	if (count > 1) {
		segnum = FindSegByPoint(&pos_right, objP->segnum);
		if (segnum != -1) {
			if (blobObjP = ObjectCreateExplosion (segnum, &pos_right, size_scale, VCLIP_AFTERBURNER_BLOB)) {
				if (lifetime != -1)
					blobObjP->lifeleft = lifetime;
			AddChildObjectP (pParent, blobObjP);
			if (bThruster) {
				blobObjP->render_type = RT_THRUSTER;
				blobObjP->mtype.phys_info.flags |= PF_WIGGLE;
				}	
			}
		}
	}
}

//------------------------------------------------------------------------------

int FindMonsterBall (void)
{
	short		i;
	object	*objP;

for (i = gameData.objs.nObjects, objP = gameData.objs.objects; i; i--, objP++)
	if ((objP->type == OBJ_POWERUP) && (objP->id == POW_MONSTERBALL)) {
		gameData.hoard.monsterBallP = objP;
		gameData.hoard.nMonsterBallSeg = objP->segnum;
		return 1;
		}
gameData.hoard.nMonsterBallSeg = -1;
return 0;
}

//------------------------------------------------------------------------------

int CreateMonsterBall (void)
{
	short			nDropSeg, nObject;
	vms_vector	vSegCenter;
	vms_vector	vInitVel = {0,0,0};

if (!(NetworkIAmMaster() && IsMultiGame && (gameData.app.nGameMode & GM_MONSTERBALL)))
	return 0;
nDropSeg = (gameData.hoard.nMonsterBallSeg >= 0) ? 
			  gameData.hoard.nMonsterBallSeg : ChooseDropSegment (NULL, NULL, EXEC_DROP);
if (nDropSeg >= 0) {
	COMPUTE_SEGMENT_CENTER_I (&vSegCenter, nDropSeg);
	nObject = DropPowerup (OBJ_POWERUP, POW_MONSTERBALL, -1, 1, &vInitVel, &vSegCenter, nDropSeg);
	if (nObject >= 0) {
		gameData.hoard.monsterBallP = gameData.objs.objects + nObject;
		gameData.hoard.monsterBallP->mtype.phys_info.mass = F1_0 * 10;
		return 1;
		}
	}
Warning (TXT_NO_MONSTERBALL);
gameData.app.nGameMode &= ~GM_MONSTERBALL;
return 0;
}

//	-----------------------------------------------------------------------------

int CheckMonsterBallScore (void)
{
	ubyte special;

if (!(gameData.app.nGameMode & GM_MONSTERBALL))
	return 0;
special = gameData.segs.segment2s [gameData.hoard.monsterBallP->segnum].special;
if (special == SEGMENT_IS_GOAL_RED)
	MultiSendCaptureBonus (TEAM_BLUE);
else if (special == SEGMENT_IS_GOAL_BLUE)
	MultiSendCaptureBonus (TEAM_BLUE);
else
	return 0;
ReleaseObject (OBJ_IDX (gameData.hoard.monsterBallP));
gameData.hoard.monsterBallP = NULL;
CreateMonsterBall ();
return 1;
}

//------------------------------------------------------------------------------
//eof
