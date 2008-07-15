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
int CollideTwoObjects (tObject *objP0, tObject *objP1, vmsVector *vCollision);
int CollideObjectWithWall (tObject *objP, fix xHitSpeed, short nHitSeg, short nHitWall, vmsVector *vHit);
void ApplyDamageToPlayer (tObject *playerP, tObject *killerP, fix cDamage);

// Returns 1 if robot died, else 0.
int ApplyDamageToRobot (tObject *robotP, fix xDamage, int nKillerObj);

int CollidePlayerAndWeapon (tObject *nPlayer, tObject *weaponP, vmsVector *vCollision);
int CollidePlayerAndMatCen (tObject *objP);
int CollideRobotAndMatCen (tObject *objP);

void ScrapeObjectOnWall (tObject *objP, short nHitSeg, short nHitWall, vmsVector *vHit);
int MaybeDetonateWeapon (tObject *obj0p, tObject *obj1P, vmsVector *vPos);

int CollidePlayerAndNastyRobot (tObject * nPlayer, tObject * robot, vmsVector *vCollision);

int NetDestroyReactor (tObject *reactorP);
int CollidePlayerAndPowerup (tObject * nPlayer, tObject * powerup, vmsVector *vCollision);
int CheckEffectBlowup (tSegment *segP, short nSide, vmsVector *vPos, tObject *blowerP, int bForceBlowup);
void ApplyDamageToReactor (tObject *reactorP, fix xDamage, short nAttacker);
void BumpOneObject (tObject *objP, vmsVector *vHitDir, fix xDamage);
void SetMonsterballForces (void);
void SetDebrisCollisions (void);

extern int Immaterial;

#endif /* _COLLIDE_H */
