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
#define ET_SPARKS       0   //little sparks, like when laser hits CWall
#define ET_MULTI_START  1   //first part of multi-part explosion
#define ET_MULTI_SECOND 2   //second part of multi-part explosion

#define BLAST_LIFE	(2 * F1_0 / 5)
#define BLAST_SCALE	(5 * F1_0 / BLAST_LIFE)

CObject *ObjectCreateBadassExplosion(CObject *objp, short nSegment,
		CFixVector *position, fix size, ubyte vclipType,
		fix maxdamage, fix maxdistance, fix maxforce, short parent);

// blows up a badass weapon, creating the badass explosion
// return the explosion CObject
CObject *ExplodeBadassWeapon(CObject *obj,CFixVector *pos);

// blows up the CPlayerData with a badass explosion
// return the explosion CObject
CObject *ExplodeBadassPlayer(CObject *obj);
CObject *CreateExplBlast (CObject *objP);
void ExplodeObject(CObject *obj,fix delayTime);
void DoExplosionSequence(CObject *obj);
void DoDebrisFrame(CObject *obj);      // deal with debris for this frame
void DrawFireball(CObject *obj);
CObject *ObjectCreateExplosionSub (CObject *objP, short nSegment, CFixVector *vPos, fix xSize, 
											  ubyte nVClip, fix xMaxDamage, fix xMaxDistance, fix xMaxForce, short nParent);


void ExplodeWall(short nSegment, short nSide);
void DoExplodingWallFrame(void);
void InitExplodingWalls(void);

short GetExplosionVClip(CObject *obj, int stage);

//------------------------------------------------------------------------------

static inline CObject *ObjectCreateMuzzleFlash (short nSegment, CFixVector * position, fix size, ubyte nVClip)
{
return ObjectCreateExplosionSub (NULL, nSegment, position, size, nVClip, 0, 0, 0, -1);
}

//------------------------------------------------------------------------------

static inline CObject *ObjectCreateExplosion (short nSegment, CFixVector * position, fix size, ubyte nVClip)
{
return ObjectCreateExplosionSub (NULL, nSegment, position, size, nVClip, 0, 0, 0, -1);
}

//------------------------------------------------------------------------------

#endif /* _FIREBALL_H */
