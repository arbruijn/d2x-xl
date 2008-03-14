/* $Id: physics.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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

#ifndef _PHYSICS_H
#define _PHYSICS_H

#include "vecmat.h"
#include "fvi.h"

//#define FL_NORMAL  0
//#define FL_TURBO   1
//#define FL_HOVER   2
//#define FL_REVERSE 3

// these global vars are set after a call to DoPhysicsSim().  Ugly, I know.
// list of segments went through
extern short physSegList [MAX_FVI_SEGS], nPhysSegs;

// Read contrls and set physics vars
void ReadFlyingControls(tObject *obj);

// Simulate a physics tObject for this frame
void DoPhysicsSim(tObject *obj);

// tell us what the given tObject will do (as far as hiting walls) in
// the given time (in seconds) t.  Igores acceleration (sorry)
// if checkObjects is set, check with objects, else just with walls
// returns fate, fills in hit time.  If fate==HIT_NONE, hitTime undefined
// Stuff hit_info with fvi data as set by FindVectorIntersection.
// for fviFlags, refer to fvi.h for the fvi query flags
int physics_lookahead(tObject *obj, fix t, int fviFlags, fix *hitTime, tFVIData *hit_info);

// Applies an instantaneous force on an tObject, resulting in an instantaneous
// change in velocity.
void PhysApplyForce(tObject *obj, vmsVector *force_vec);
void PhysApplyRot(tObject *obj, vmsVector *force_vec);

// this routine will set the thrust for an tObject to a value that will
// (hopefully) maintain the tObject's current velocity
void SetThrustFromVelocity(tObject *obj);

void PhysicsTurnTowardsVector (vmsVector *vGoal, tObject *objP, fix rate);

#endif /* _PHYSICS_H */
