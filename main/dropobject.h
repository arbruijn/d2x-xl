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

#ifndef _DROPOBJECT_H
#define _DROPOBJECT_H

#define FORCE_DROP	0
#define INIT_DROP		1
#define CHECK_DROP	2
#define EXEC_DROP		3

int ChooseDropSegment (tObject *objP, int *pbFixedPos, int nDropState);
int ObjectCreateEgg (tObject *objP);
int CallObjectCreateEgg (tObject *objP, int count, int nType, int id);
int MaybeDropNetPowerup (short nObject, int powerupType, int nDropState);
void RespawnDestroyedWeapon (short nObject);
void MaybeReplacePowerupWithEnergy (tObject *del_obj);
void DropPowerups ();
int DropPowerup (ubyte nType, ubyte id, short owner, int num, vmsVector *init_vel, vmsVector *pos, short nSegment);
// creates afterburner blobs behind the specified tObject
void DropAfterburnerBlobs (tObject *obj, int count, fix size_scale, fix lifetime, tObject *pParent, int bThruster);
int ReturnFlagHome (tObject *pObj);
int PickConnectedSegment (tObject *objP, int max_depth);

#endif /* _DROPOBJECT_H */
