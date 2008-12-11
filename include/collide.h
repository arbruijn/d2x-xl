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

#ifndef _COLLIDE_H
#define _COLLIDE_H

void CollideInit ();
int CollideTwoObjects (CObject *objP0, CObject *objP1, CFixVector *vCollision);
int CollideObjectWithWall (CObject *objP, fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector *vHit);
void ApplyDamageToPlayer (CObject *playerP, CObject *killerP, fix cDamage);

// Returns 1 if robot died, else 0.
int ApplyDamageToRobot (CObject *robotP, fix xDamage, int nKillerObj);

int CollidePlayerAndWeapon (CObject *nPlayer, CObject *weaponP, CFixVector *vCollision);
int CollidePlayerAndMatCen (CObject *objP);
int CollideRobotAndMatCen (CObject *objP);

void ScrapeObjectOnWall (CObject *objP, short nHitSeg, short nHitWall, CFixVector *vHit);
int MaybeDetonateWeapon (CObject *obj0p, CObject *obj1P, CFixVector *vPos);

int CollidePlayerAndNastyRobot (CObject * nPlayer, CObject * robot, CFixVector *vCollision);

int NetDestroyReactor (CObject *reactorP);
int CollidePlayerAndPowerup (CObject * nPlayer, CObject * powerup, CFixVector *vCollision);
int CheckEffectBlowup (CSegment *segP, short nSide, CFixVector *vPos, CObject *blowerP, int bForceBlowup);
void ApplyDamageToReactor (CObject *reactorP, fix xDamage, short nAttacker);
void BumpOneObject (CObject *objP, CFixVector *vHitDir, fix xDamage);
void SetDebrisCollisions (void);

extern int Immaterial;

#endif /* _COLLIDE_H */
