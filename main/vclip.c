/* $Id: tVideoClip.c,v 1.5 2003/10/10 09:36:35 btb Exp $ */
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: tVideoClip.c,v 1.5 2003/10/10 09:36:35 btb Exp $";
#endif

#include <stdlib.h>

#include "error.h"

#include "inferno.h"
#include "vclip.h"
#include "weapon.h"
#include "laser.h"
#include "hudmsg.h"

//----------------- Variables for video clips -------------------

inline int CurFrame (int nClip, fix timeToLive)
{
tVideoClip	*pvc = gameData.eff.vClips [0] + nClip;
int nFrames = pvc->nFrameCount;
//	iFrame = (nFrames - f2i (FixDiv ((nFrames - 1) * timeToLive, pvc->xTotalTime))) - 1;
int iFrame;
if (timeToLive > pvc->xTotalTime)
	timeToLive = timeToLive % pvc->xTotalTime;
iFrame = (pvc->xTotalTime - timeToLive) / pvc->xFrameTime;
return (iFrame < nFrames) ? iFrame : nFrames - 1;
}

//----------------- Variables for video clips -------------------
//draw an tObject which renders as a tVideoClip

#define	FIREBALL_ALPHA		0.9
#define	THRUSTER_ALPHA		(1.0 / 3.0)
#define	WEAPON_ALPHA		0.7

void DrawVClipObject (tObject *objP,fix timeToLive, int lighted, int vclip_num, tRgbColorf *color)
{
	double		ta = 0, alpha = 0;
	tVideoClip	*pvc = gameData.eff.vClips [0] + vclip_num;
	int			nFrames = pvc->nFrameCount;
	int			iFrame = CurFrame (vclip_num, timeToLive);
	int			bThruster = (objP->renderType == RT_THRUSTER) && (objP->mType.physInfo.flags & PF_WIGGLE);

if (objP->nType == OBJ_FIREBALL) {
	if (bThruster) {
		alpha = THRUSTER_ALPHA;
		//if (objP->mType.physInfo.flags & PF_WIGGLE)	//tPlayer ship
			iFrame = nFrames - iFrame - 1;	//render the other way round
		}
	else
		alpha = FIREBALL_ALPHA;
	ta = (double) iFrame / (double) nFrames * alpha;
	alpha = (ta >= 0) ? alpha - ta : alpha + ta;
	}
else if (objP->nType == OBJ_WEAPON) {
	if ((objP->id == PROXMINE_ID) || (objP->id == SMARTMINE_ID) || (objP->id == SMALLMINE_ID)) 
		alpha = 1.0;
	else
		alpha = WEAPON_ALPHA;
	}
#if 1
if (objP->nType == OBJ_FIREBALL)
	glDepthMask (0);	//don't set z-buffer for transparent objects
#endif
if (pvc->flags & VF_ROD)
	DrawObjectRodTexPoly (objP, pvc->frames [iFrame], lighted);
else {
	Assert(lighted == 0);		//blob cannot now be lighted
	DrawObjectBlob (objP, pvc->frames [0], pvc->frames [iFrame], iFrame, color, (float) alpha);
	}
#if 1
if (objP->nType == OBJ_FIREBALL)
	glDepthMask (1);
#endif
}

// -----------------------------------------------------------------------------

void ConvertWeaponToVClip (tObject *objP)
{
objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
objP->rType.vClipInfo.xFrameTime = gameData.eff.pVClips [objP->rType.vClipInfo.nClipIndex].xFrameTime;
objP->rType.vClipInfo.nCurFrame = 0;
objP->size = gameData.objs.pwrUp.info [objP->id].size;
objP->controlType = CT_POWERUP;
objP->renderType = RT_POWERUP;
objP->mType.physInfo.mass = F1_0;
objP->mType.physInfo.drag = 512;
}

// -----------------------------------------------------------------------------

int ConvertVClipToPolymodel (tObject *objP)
{
	vmsAngVec	a;
	short			nModel;

if (gameStates.app.bNostalgia || !gameOpts->render.powerups.b3D)
	return 0;
if (objP->renderType == RT_POLYOBJ)
	return 1;
nModel = WeaponToModel (objP->id);
if (!(nModel && gameData.models.modelToOOF [nModel]))
	return 0;
a.p = (rand () % F1_0) - F1_0 / 2;
a.b = (rand () % F1_0) - F1_0 / 2;
a.h = (rand () % F1_0) - F1_0 / 2;
VmAngles2Matrix (&objP->position.mOrient, &a);
#if 0
objP->mType.physInfo.mass = F1_0;
objP->mType.physInfo.drag = 512;
#endif
objP->mType.physInfo.rotVel.p.z = 
objP->mType.physInfo.rotVel.p.y = 0;
objP->mType.physInfo.rotVel.p.x = gameOpts->render.powerups.nSpin ? F1_0 / (5 - gameOpts->render.powerups.nSpin) : 0;
//objP->controlType = CT_WEAPON;
objP->renderType = RT_POLYOBJ;
objP->movementType = MT_PHYSICS;
objP->mType.physInfo.flags = PF_BOUNCE | PF_FREE_SPINNING;
objP->rType.polyObjInfo.nModel = gameData.weapons.info [objP->id].nModel;
#if 0
objP->size = FixDiv (gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad, 
							gameData.weapons.info [objP->id].po_len_to_width_ratio);
#endif
objP->rType.polyObjInfo.nTexOverride = -1;
objP->lifeleft = IMMORTAL_TIME;
return 1;
}

//------------------------------------------------------------------------------

void DrawWeaponVClip (tObject *objP)
{
	int	nVClip;
	fix	modtime, playTime;

Assert (objP->nType == OBJ_WEAPON);
nVClip = gameData.weapons.info [objP->id].weapon_vclip;
modtime = objP->lifeleft;
playTime = gameData.eff.pVClips [nVClip].xTotalTime;
//	Special values for modtime were causing enormous slowdown for omega blobs.
if (modtime == IMMORTAL_TIME)
	modtime = playTime;
//	Should cause Omega blobs (which live for one frame) to not always be the same.
if (modtime == ONE_FRAME_TIME)
	modtime = d_rand();
if (objP->id == PROXMINE_ID) {		//make prox bombs spin out of sync
	int nObject = OBJ_IDX (objP);
	modtime += (modtime * (nObject&7)) / 16;	//add variance to spin rate
	while (modtime > playTime)
		modtime -= playTime;
	if ((nObject&1) ^ ((nObject>>1)&1))			//make some spin other way
		modtime = playTime - modtime;
	}
else {
	if (modtime > playTime)
		modtime %= playTime;
	}
if (ConvertVClipToPolymodel (objP))
	DrawPolygonObject (objP);
else
	DrawVClipObject (objP, modtime, 0, nVClip, gameData.weapons.color + objP->id);
}

//------------------------------------------------------------------------------

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads n tVideoClip structs from a CFILE
 */
int vclip_read_n(tVideoClip *vc, int n, CFILE *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		vc[i].xTotalTime = CFReadFix(fp);
		vc[i].nFrameCount = CFReadInt(fp);
		vc[i].xFrameTime = CFReadFix(fp);
		vc[i].flags = CFReadInt(fp);
		vc[i].nSound = CFReadShort(fp);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			vc[i].frames[j].index = CFReadShort(fp);
		vc[i].lightValue = CFReadFix(fp);
	}
	return i;
}
#endif

//------------------------------------------------------------------------------
