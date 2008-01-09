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

#ifndef _OMEGA_H
#define _OMEGA_H

#define OMEGA_MULTI_LIFELEFT    (F1_0/6)

//	Additionally, several constants which apply to homing gameData.objs.objects in general control the behavior of the Omega Cannon.
//	They are defined in laser.h.  They are copied here for reference.  These values are valid on 1/10/96:
//	If you want the Omega Cannon view cone to be different than the Homing Missile viewcone, contact MK to make the change.
//	 (Unless you are a programmer, in which case, do it yourself!)

void DoOmegaStuff (tObject *parentObjP, vmsVector *vFiringPos, tObject *weaponObjP);
int UpdateOmegaLightnings (tObject *parentObjP, tObject *targetObjP);
void DestroyOmegaLightnings (void);
void SetMaxOmegaCharge (void);

#define MAX_OMEGA_CHARGE (gameStates.app.bHaveExtraGameInfo [IsMultiGame] ? gameData.omega.xMaxCharge : DEFAULT_MAX_OMEGA_CHARGE)

#endif /* _OMEGA_H */
