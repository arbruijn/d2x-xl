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

#include <stdlib.h>
#include <math.h>

#include "inferno.h"
#include "error.h"
#include "render.h"
#include "objrender.h"
#include "fireball.h"
#include "sphere.h"
#include "u_mem.h"
#include "hiresmodels.h"

#define BLAST_TYPE 0
#define MOVE_BLAST 1

//----------------- Variables for video clips -------------------

inline int CurFrame (tObject *objP, int nClip, fix timeToLive)
{
tVideoClip	*pvc = gameData.eff.vClips [0] + nClip;
int nFrames = pvc->nFrameCount;
//	iFrame = (nFrames - f2i (FixDiv ((nFrames - 1) * timeToLive, pvc->xTotalTime))) - 1;
int iFrame;
if (timeToLive > pvc->xTotalTime)
	timeToLive = timeToLive % pvc->xTotalTime;
if ((nClip == VCLIP_AFTERBURNER_BLOB) && objP->rType.vClipInfo.xFrameTime)
	iFrame = (objP->rType.vClipInfo.xTotalTime - timeToLive) / objP->rType.vClipInfo.xFrameTime;
else
	iFrame = (pvc->xTotalTime - timeToLive) / pvc->xFrameTime;
return (iFrame < nFrames) ? iFrame : nFrames - 1;
}

//------------------------------------------------------------------------------

tRgbColorb *VClipColor (tObject *objP)
{
	int				nVClip = gameData.weapons.info [objP->id].nVClipIndex;
	tBitmapIndex	bmi;
	grsBitmap		*bmP;

if (nVClip) {
	tVideoClip *vcP = gameData.eff.vClips [0] + nVClip;
	bmi = vcP->frames [0];
	}
else
	bmi = gameData.weapons.info [objP->id].bitmap;
PIGGY_PAGE_IN (bmi.index, 0);
bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
if ((bmP->bmType == BM_TYPE_STD) && BM_OVERRIDE (bmP))
	bmP = BM_OVERRIDE (bmP);
return &bmP->bmAvgRGB;
}

//------------------------------------------------------------------------------

int SetupHiresVClip (tVideoClip *vcP, tVClipInfo *vciP)
{
	int nFrames = vcP->nFrameCount;

if (vcP->flags & WCF_ALTFMT) {
	grsBitmap	*bmP;
	if (vcP->flags & WCF_INITIALIZED) {
		bmP = BM_OVERRIDE (gameData.pig.tex.bitmaps [0] + vcP->frames [0].index);
		nFrames = ((bmP->bmType != BM_TYPE_ALT) && BM_PARENT (bmP)) ? BM_FRAMECOUNT (BM_PARENT (bmP)) : BM_FRAMECOUNT (bmP);
		}
	else {
		bmP = SetupHiresAnim ((short *) vcP->frames, nFrames, -1, 0, 1, &nFrames);
		if (!bmP)
			vcP->flags &= ~WCF_ALTFMT;
		else if (!gameOpts->ogl.bGlTexMerge)
			vcP->flags &= ~WCF_ALTFMT;
		else
			vcP->flags |= WCF_INITIALIZED;
		vciP->nCurFrame = d_rand () % nFrames;
		}
	}
return nFrames;
}

//------------------------------------------------------------------------------
//draw an tObject which renders as a tVideoClip

#define	FIREBALL_ALPHA		0.8
#define	THRUSTER_ALPHA		(1.0 / 4.0)
#define	WEAPON_ALPHA		0.7

void DrawVClipObject (tObject *objP, fix timeToLive, int bLit, int nVClip, tRgbaColorf *color)
{
	double		ta = 0, alpha = 0;
	tVideoClip	*vcP = gameData.eff.vClips [0] + nVClip;
	int			nFrames = SetupHiresVClip (vcP, &objP->rType.vClipInfo);
	int			iFrame = CurFrame (objP, nVClip, timeToLive);
	int			bThruster = (objP->renderType == RT_THRUSTER) && (objP->mType.physInfo.flags & PF_WIGGLE);

if ((objP->nType == OBJ_FIREBALL) || (objP->nType == OBJ_EXPLOSION)) {
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
	if (WeaponIsMine (objP->id))
		alpha = 1.0;
	else
		alpha = WEAPON_ALPHA;
	}
#if 1
if ((objP->nType == OBJ_FIREBALL) || (objP->nType == OBJ_EXPLOSION))
	glDepthMask (1);	//don't set z-buffer for transparent objects
#endif
if (vcP->flags & VF_ROD)
	DrawObjectRodTexPoly (objP, vcP->frames [iFrame], bLit, iFrame);
else {
	Assert(bLit == 0);		//blob cannot now be bLit
	DrawObjectBlob (objP, vcP->frames [0].index, vcP->frames [iFrame].index, iFrame, color, (float) alpha);
	}
#if 1
if ((objP->nType == OBJ_FIREBALL) || (objP->nType == OBJ_EXPLOSION))
	glDepthMask (1);
#endif
}

// -----------------------------------------------------------------------------

void DrawExplBlast (tObject *objP)
{
	float			fLife, fAlpha;
	fix			xSize;
	vmsVector	vPos, vDir;
#if BLAST_TYPE == 1
	int			i;
	fix			xSize2;
#elif BLAST_TYPE == 2
	float			r;
	int			i;
	tSphereData	sd;
	tOOF_vector	p = {0,0,0};
#endif
	tRgbaColorf	color;
#if 0
	static tRgbaColorf blastColors [] = {
		{0.5, 0.0f, 0.5f, 1},
		{1, 0.5f, 0, 1},
		{1, 0.75f, 0, 1},
		{1, 1, 1, 3}};
#endif
if (objP->lifeleft <= 0)
	return;
if (!LoadExplBlast ())
	return;
fLife = f2fl (BLAST_LIFE * 2 - objP->lifeleft);
xSize = (fix) (objP->size * fLife * BLAST_SCALE);
vPos = objP->position.vPos;
#if MOVE_BLAST
vDir = gameData.objs.console->position.vPos - vPos;
vmsVector::normalize(vDir);
vDir *= (xSize - objP->size);
vPos += vDir;
#endif
glDepthMask (0);
#if BLAST_TYPE == 0
fAlpha = (float) sqrt (f2fl (objP->lifeleft) * 3);
color.red =
color.green =
color.blue =
color.alpha = fAlpha;
G3DrawSprite(vPos, xSize, xSize, bmpExplBlast, &color, fAlpha, 2, 10);
#elif BLAST_TYPE == 1
xSize2 = xSize / 20;
fAlpha = (float) sqrt (f2fl (objP->lifeleft)) / 4;
for (i = 0; i < 4; i++, xSize -= xSize2)
	G3DrawSprite (&vPos, xSize, xSize, bmpExplBlast, &color, fAlpha * blastColors [i].alpha, 0);
#elif BLAST_TYPE == 2
CreateSphere (&sd);
fAlpha = (float) sqrt (f2fl (objP->lifeleft) * 3);
r = f2fl (xSize);
sd.pPulse = 0;
G3StartInstanceMatrix(objP->position.vPos, &objP->position.mOrient);
for (i = 0; i < 3; i++) {
	RenderSphere (&sd, (tOOF_vector *) OOF_VecVms2Oof (&p, &objP->position.vPos),
					  r, r, r, 1, 1, 1, fAlpha, NULL, 1);
	r *= i ? 0.5f : 0.8f;
	}
G3DoneInstance ();
#endif
glDepthMask (1);
}

// -----------------------------------------------------------------------------

void ConvertPowerupToVClip (tObject *objP)
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

void ConvertWeaponToVClip (tObject *objP)
{
objP->rType.vClipInfo.nClipIndex = gameData.weapons.info [objP->id].nVClipIndex;
objP->rType.vClipInfo.xFrameTime = gameData.eff.pVClips [objP->rType.vClipInfo.nClipIndex].xFrameTime;
objP->rType.vClipInfo.nCurFrame = 0;
objP->controlType = CT_WEAPON;
objP->renderType = RT_WEAPON_VCLIP;
objP->mType.physInfo.mass = gameData.weapons.info [objP->id].mass;
objP->mType.physInfo.drag = gameData.weapons.info [objP->id].drag;
objP->size = gameData.weapons.info [objP->id].strength [gameStates.app.nDifficultyLevel] / 10;
objP->movementType = MT_PHYSICS;
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
if (!(nModel && HaveReplacementModel (nModel)))
	return 0;
a[PA] = (rand () % F1_0) - F1_0 / 2;
a[BA] = (rand () % F1_0) - F1_0 / 2;
a[HA] = (rand () % F1_0) - F1_0 / 2;
objP->position.mOrient = vmsMatrix::Create(a);
#if 0
objP->mType.physInfo.mass = F1_0;
objP->mType.physInfo.drag = 512;
#endif
objP->mType.physInfo.rotVel[Z] =
objP->mType.physInfo.rotVel[Y] = 0;
objP->mType.physInfo.rotVel[X] = gameOpts->render.powerups.nSpin ? F1_0 / (5 - gameOpts->render.powerups.nSpin) : 0;
//objP->controlType = CT_WEAPON;
objP->renderType = RT_POLYOBJ;
objP->movementType = MT_PHYSICS;
objP->mType.physInfo.flags = PF_BOUNCE | PF_FREE_SPINNING;
if (0 > (objP->rType.polyObjInfo.nModel = gameData.weapons.info [objP->id].nModel))
	objP->rType.polyObjInfo.nModel = nModel;
#if 0
objP->size = FixDiv (gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad,
							gameData.weapons.info [objP->id].po_len_to_width_ratio);
#endif
objP->rType.polyObjInfo.nTexOverride = -1;
if (objP->nType == OBJ_POWERUP)
	objP->lifeleft = IMMORTAL_TIME;
return 1;
}

//------------------------------------------------------------------------------

void DrawWeaponVClip (tObject *objP)
{
	int	nVClip;
	fix	modtime, playTime;

Assert (objP->nType == OBJ_WEAPON);
nVClip = gameData.weapons.info [objP->id].nVClipIndex;
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
	modtime += (modtime * (nObject & 7)) / 16;	//add variance to spin rate
	while (modtime > playTime)
		modtime -= playTime;
	if ((nObject & 1) ^ ((nObject >> 1) & 1))			//make some spin other way
		modtime = playTime - modtime;
	}
else {
	if (modtime > playTime)
		modtime %= playTime;
	}
if (ConvertVClipToPolymodel (objP))
	DrawPolygonObject (objP, 0, 0);
else
	DrawVClipObject (objP, modtime, 0, nVClip, gameData.weapons.color + objP->id);
}

//------------------------------------------------------------------------------

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads n tVideoClip structs from a CFILE
 */
int VClipReadN(tVideoClip *vc, int n, CFILE *fp)
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
