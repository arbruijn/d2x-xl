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
int ObjectCreateEgg (CObject *objP, bool bLocal = false, bool bUpdateLimits = true);
int CallObjectCreateEgg (CObject *objP, int count, int nType, int id, bool bLocal = false, bool bUpdateLimits = false);
int MaybeDropNetPowerup (short nObject, int powerupType, int nDropState);
void RespawnDestroyedWeapon (short nObject);
void MaybeReplacePowerupWithEnergy (CObject *del_obj);
void DropPowerups (void);
int DropPowerup (ubyte nType, ubyte nId, short nOwner, int nCount, const CFixVector& vInitVel, const CFixVector& vPos, short nSegment, bool bLocal = false);
// creates afterburner blobs behind the specified CObject
void DropAfterburnerBlobs (CObject *obj, int count, fix size_scale, fix lifetime, CObject *pParent, int bThruster);
int MaybeDropPrimaryWeaponEgg (CObject *playerObjP, int weapon_index);
void MaybeDropSecondaryWeaponEgg (CObject *playerObjP, int weapon_index, int count);
void DropPlayerEggs (CObject *playerObjP);
int ReturnFlagHome (CObject *pObj);
int PickConnectedSegment (CObject *objP, int nMaxDepth, int *nDepthP);
int AddDropInfo (short nObject, short nPowerupType, int nDropTime = -1);
void DelDropInfo (int h);
int FindDropInfo (int nSignature);

#endif /* _DROPOBJECT_H */
