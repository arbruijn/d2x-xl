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

int ChooseDropSegment (CObject *objP, int *pbFixedPos, int nDropState);
int ObjectCreateEgg (CObject *objP);
int CallObjectCreateEgg (CObject *objP, int count, int nType, int id);
int MaybeDropNetPowerup (short nObject, int powerupType, int nDropState);
void RespawnDestroyedWeapon (short nObject);
void MaybeReplacePowerupWithEnergy (CObject *del_obj);
void DropPowerups ();
int DropPowerup (ubyte nType, ubyte id, short owner, int num, const vmsVector& init_vel, const vmsVector& pos, short nSegment);
// creates afterburner blobs behind the specified CObject
void DropAfterburnerBlobs (CObject *obj, int count, fix size_scale, fix lifetime, CObject *pParent, int bThruster);
int MaybeDropPrimaryWeaponEgg (CObject *playerObjP, int weapon_index);
void MaybeDropSecondaryWeaponEgg (CObject *playerObjP, int weapon_index, int count);
void DropPlayerEggs (CObject *playerObjP);
int ReturnFlagHome (CObject *pObj);
int PickConnectedSegment (CObject *objP, int nMaxDepth, int *nDepthP);

#endif /* _DROPOBJECT_H */
