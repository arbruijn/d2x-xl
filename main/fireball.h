/* $Id: fireball.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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
 * Header for fireball.c
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:56:23  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:27:03  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.13  1995/01/17  12:14:38  john
 * Made walls, object explosion vclips load at level start.
 *
 * Revision 1.12  1995/01/13  15:41:52  rob
 * Added prototype for MaybeReplacePowerupWithEnergy
 *
 * Revision 1.11  1994/11/17  16:28:36  rob
 * Changed maybe_drop_cloak_powerup to MaybeDropNetPowerup (more
 * generic and useful)
 *
 * Revision 1.10  1994/10/12  08:03:42  mike
 * Prototype maybe_drop_cloak_powerup.
 *
 * Revision 1.9  1994/10/11  12:24:39  matt
 * Cleaned up/change badass explosion calls
 *
 * Revision 1.8  1994/09/07  16:00:34  mike
 * Add object pointer to parameter list of ObjectCreateBadassExplosion.
 *
 * Revision 1.7  1994/09/02  14:00:39  matt
 * Simplified ExplodeObject() & mutliple-stage explosions
 *
 * Revision 1.6  1994/08/17  16:49:58  john
 * Added damaging fireballs, missiles.
 *
 * Revision 1.5  1994/07/14  22:39:19  matt
 * Added exploding doors
 *
 * Revision 1.4  1994/06/08  10:56:36  matt
 * Improved debris: now get submodel size from new POF files; debris now has
 * limited life; debris can now be blown up.
 *
 * Revision 1.3  1994/04/01  13:35:44  matt
 * Added multiple-stage explosions
 *
 * Revision 1.2  1994/02/17  11:33:32  matt
 * Changes in object system
 *
 * Revision 1.1  1994/02/16  22:41:15  matt
 * Initial revision
 *
 *
 */


#ifndef _FIREBALL_H
#define _FIREBALL_H

// explosion types
#define ET_SPARKS       0   //little sparks, like when laser hits wall
#define ET_MULTI_START  1   //first part of multi-part explosion
#define ET_MULTI_SECOND 2   //second part of multi-part explosion

#define FORCE_DROP	0
#define INIT_DROP		1
#define CHECK_DROP	2
#define EXEC_DROP		3

object *ObjectCreateExplosion(short segnum, vms_vector *position, fix size, ubyte vclip_type);
object *ObjectCreateMuzzleFlash(short segnum, vms_vector *position, fix size, ubyte vclip_type);

object *ObjectCreateBadassExplosion(object *objp, short segnum,
		vms_vector *position, fix size, ubyte vclip_type,
		fix maxdamage, fix maxdistance, fix maxforce, short parent);

// blows up a badass weapon, creating the badass explosion
// return the explosion object
object *ExplodeBadassWeapon(object *obj,vms_vector *pos);

// blows up the player with a badass explosion
// return the explosion object
object *ExplodeBadassPlayer(object *obj);

void ExplodeObject(object *obj,fix delay_time);
void DoExplosionSequence(object *obj);
void DoDebrisFrame(object *obj);      // deal with debris for this frame

void DrawFireball(object *obj);

void ExplodeWall(short segnum, short sidenum);
void DoExplodingWallFrame(void);
void InitExplodingWalls(void);
int MaybeDropNetPowerup(short objnum, int powerup_type, int nDropState);
RespawnDestroyedWeapon (short nObject);
void MaybeReplacePowerupWithEnergy(object *del_obj);
void DropPowerups();

short GetExplosionVClip(object *obj, int stage);
int DropPowerup(ubyte type, ubyte id, short owner, int num, vms_vector *init_vel, vms_vector *pos, short segnum);

// creates afterburner blobs behind the specified object
void DropAfterburnerBlobs(object *obj, int count, fix size_scale, fix lifetime, object *pParent, int bThruster);

int ReturnFlagHome (object *pObj);
int CountRooms (void);
int GatherFlagGoals (void);
int CheckConquerRoom (xsegment *segP);
void ConquerRoom (int newOwner, int oldOwner, int roomId);
void StartConquerWarning (void);
void StopConquerWarning (void);
int FindMonsterBall (void);
int CreateMonsterBall (void);
int CheckMonsterScore (void);

#endif /* _FIREBALL_H */
