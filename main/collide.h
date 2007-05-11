/* $Id: collide.h,v 1.2 2003/10/10 09:36:34 btb Exp $ */
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

void CollideInit();
int CollideTwoObjects(tObject * A, tObject * B, vmsVector *collision_point);
int CollideObjectWithWall(tObject * A, fix hitspeed, short hitseg, short hitwall, vmsVector * hitpt);
void ApplyDamageToPlayer(tObject *tPlayer, tObject *killer, fix damage);

// Returns 1 if robot died, else 0.
int ApplyDamageToRobot(tObject *robot, fix damage, int nKillerObj);

extern int Immaterial;

int CollidePlayerAndWeapon(tObject * tPlayer, tObject * weapon, vmsVector *collision_point);
int CollidePlayerAndMatCen(tObject *objp);
int CollideRobotAndMatCen(tObject *objp);

void ScrapeObjectOnWall(tObject *obj, short hitseg, short hitwall, vmsVector * hitpt);
int MaybeDetonateWeapon(tObject *obj0p, tObject *obj, vmsVector *pos);

int CollidePlayerAndNastyRobot(tObject * tPlayer, tObject * robot, vmsVector *collision_point);

void NetDestroyReactor(tObject *controlcen);
int CollidePlayerAndPowerup(tObject * tPlayer, tObject * powerup, vmsVector *collision_point);
int CheckEffectBlowup(tSegment *seg,short tSide,vmsVector *pnt, tObject *blower, int force_blowupFlag);
void ApplyDamageToReactor(tObject *controlcen, fix damage, short who);
void BumpOneObject(tObject *obj0, vmsVector *hit_dir, fix damage);
void SetMonsterballForces (void);
void SetDebrisCollisions (void);

#endif /* _COLLIDE_H */
