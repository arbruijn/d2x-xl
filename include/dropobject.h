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

int32_t ChooseDropSegment (CObject *pObj, int32_t *pbFixedPos, int32_t nDropState);
int32_t PrepareObjectCreateEgg (CObject *pObj, int32_t count, int32_t nType, int32_t id, bool bLocal = false, bool bUpdateLimits = false);
int32_t MaybeDropNetPowerup (int16_t nObject, int32_t powerupType, int32_t nDropState);
void RespawnDestroyedWeapon (int16_t nObject);
void MaybeReplacePowerupWithEnergy (CObject *del_obj);
void DropPowerups (void);
int32_t DropPowerup (uint8_t nType, uint8_t nId, int16_t nOwner, int32_t bDropExtras, const CFixVector& vInitVel, const CFixVector& vPos, int16_t nSegment, bool bLocal = false);
// creates afterburner blobs behind the specified CObject
void DropAfterburnerBlobs (CObject *obj, int32_t count, fix size_scale, fix lifetime, CObject *pParent, int32_t bThruster);
int32_t MaybeDropPrimaryWeaponEgg (CObject *pPlayerObj, int32_t weapon_index);
void MaybeDropSecondaryWeaponEgg (CObject *pPlayerObj, int32_t weapon_index, int32_t count);
void DropPlayerEggs (CObject *pPlayerObj);
void DropExcessAmmo (void);
int32_t ReturnFlagHome (CObject *pObj);
int32_t PickConnectedSegment (CObject *pObj, int32_t nMaxDepth, int32_t *nDepthP);
int32_t AddDropInfo (int16_t nObject, int16_t nPowerupType, int32_t nDropTime = -1);
void DelDropInfo (int32_t h);
int32_t FindDropInfo (int32_t nSignature);

#endif /* _DROPOBJECT_H */
