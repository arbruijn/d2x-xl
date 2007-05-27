/* $Id: cntrlcen.h,v 1.6 2003/10/10 09:36:34 btb Exp $ */
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

#ifndef _CNTRLCEN_H
#define _CNTRLCEN_H

#include "vecmat.h"
#include "object.h"
#include "wall.h"
//#include "switch.h"

#define MAX_CONTROLCEN_GUNS     8

#define CONTROLCEN_WEAPON_NUM   6

#define MAX_CONTROLCEN_LINKS    10

typedef struct tReactorTriggers {
	short   nLinks;
	short   nSegment [MAX_CONTROLCEN_LINKS];
	short   nSide [MAX_CONTROLCEN_LINKS];
} __pack__ tReactorTriggers;

typedef struct tReactorProps {
	int nModel;
	int nGuns;
	vmsVector gunPoints[MAX_CONTROLCEN_GUNS];
	vmsVector gun_dirs[MAX_CONTROLCEN_GUNS];
} __pack__ tReactorProps;

#define MAX_REACTORS 7

//@@extern vmsVector controlcen_gun_points[MAX_CONTROLCEN_GUNS];
//@@extern vmsVector controlcen_gun_dirs[MAX_CONTROLCEN_GUNS];
extern vmsVector Gun_pos[MAX_CONTROLCEN_GUNS];

// do whatever this thing does in a frame
void DoReactorFrame(tObject *obj);
// Initialize control center for a level.
// Call when a new level is started.
void InitReactorForLevel(int bRestore);
void DoReactorDestroyedStuff(tObject *objp);
void DoReactorDeadFrame(void);
fix ReactorStrength (void);

#if 0
#define ReactorReadN(r, n, fp) CFRead(r, sizeof(reactor), n, fp)
#define ControlCenterTriggersReadN(cct, n, fp) CFRead(cct, sizeof(tReactorTriggers), n, fp)
#else
/*
 * reads n reactor structs from a CFILE
 */
extern int ReactorReadN(tReactorProps *r, int n, CFILE *fp);

/*
 * reads n tReactorTriggers structs from a CFILE
 */
extern int ControlCenterTriggersReadN(tReactorTriggers *cct, int n, CFILE *fp);
#endif

int FindReactor (tObject *objP);
void InitCountdown (tTrigger *trigP, int bReactorDestroyed);

#endif /* _CNTRLCEN_H */
