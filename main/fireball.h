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

#ifndef _FIREBALL_H
#define _FIREBALL_H

// explosion types
#define ET_SPARKS       0   //little sparks, like when laser hits tWall
#define ET_MULTI_START  1   //first part of multi-part explosion
#define ET_MULTI_SECOND 2   //second part of multi-part explosion

#define FORCE_DROP	0
#define INIT_DROP		1
#define CHECK_DROP	2
#define EXEC_DROP		3

#define BLAST_LIFE	(F1_0 / 2)

tObject *ObjectCreateExplosion(short nSegment, vmsVector *position, fix size, ubyte vclipType);
tObject *ObjectCreateMuzzleFlash(short nSegment, vmsVector *position, fix size, ubyte vclipType);

tObject *ObjectCreateBadassExplosion(tObject *objp, short nSegment,
		vmsVector *position, fix size, ubyte vclipType,
		fix maxdamage, fix maxdistance, fix maxforce, short parent);

// blows up a badass weapon, creating the badass explosion
// return the explosion tObject
tObject *ExplodeBadassWeapon(tObject *obj,vmsVector *pos);

// blows up the tPlayer with a badass explosion
// return the explosion tObject
tObject *ExplodeBadassPlayer(tObject *obj);
tObject *CreateExplBlast (tObject *objP);
void ExplodeObject(tObject *obj,fix delayTime);
void DoExplosionSequence(tObject *obj);
void DoDebrisFrame(tObject *obj);      // deal with debris for this frame

void DrawFireball(tObject *obj);

void ExplodeWall(short nSegment, short nSide);
void DoExplodingWallFrame(void);
void InitExplodingWalls(void);
int MaybeDropNetPowerup(short nObject, int powerupType, int nDropState);
void RespawnDestroyedWeapon (short nObject);
void MaybeReplacePowerupWithEnergy(tObject *del_obj);
void DropPowerups();

short GetExplosionVClip(tObject *obj, int stage);
int DropPowerup(ubyte nType, ubyte id, short owner, int num, vmsVector *init_vel, vmsVector *pos, short nSegment);

// creates afterburner blobs behind the specified tObject
void DropAfterburnerBlobs(tObject *obj, int count, fix size_scale, fix lifetime, tObject *pParent, int bThruster);

int ReturnFlagHome (tObject *pObj);
int CountRooms (void);
int GatherFlagGoals (void);
int CheckConquerRoom (xsegment *segP);
void ConquerRoom (int newOwner, int oldOwner, int roomId);
void StartConquerWarning (void);
void StopConquerWarning (void);
int FindMonsterball (void);
int CreateMonsterball (void);
int CheckMonsterballScore (void);

#endif /* _FIREBALL_H */
