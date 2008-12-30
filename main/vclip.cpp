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

inline int CurFrame (CObject *objP, int nClip, fix timeToLive)
{
tVideoClip	*pvc = gameData.eff.vClips [0] + nClip;
int nFrames = pvc->nFrameCount;
//	iFrame = (nFrames - X2I (FixDiv ((nFrames - 1) * timeToLive, pvc->xTotalTime))) - 1;
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

tRgbColorb *VClipColor (CObject *objP)
{
	int				nVClip = gameData.weapons.info [objP->info.nId].nVClipIndex;
	tBitmapIndex	bmi;
	CBitmap			*bmP;

if (nVClip) {
	tVideoClip *vcP = gameData.eff.vClips [0] + nVClip;
	bmi = vcP->frames [0];
	}
else
	bmi = gameData.weapons.info [objP->info.nId].bitmap;
PIGGY_PAGE_IN (bmi.index, 0);
bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
if ((bmP->Type () == BM_TYPE_STD) && bmP->Override ())
	bmP = bmP->Override ();
bmP->AvgColor ();
return bmP->GetAvgColor ();
}

//------------------------------------------------------------------------------

int SetupHiresVClip (tVideoClip *vcP, tVClipInfo *vciP)
{
	int nFrames = vcP->nFrameCount;

if (vcP->flags & WCF_ALTFMT) {
	CBitmap	*bmP;
	if (vcP->flags & WCF_INITIALIZED) {
		bmP = gameData.pig.tex.bitmaps [0][vcP->frames [0].index].Override ();
		nFrames = ((bmP->Type () != BM_TYPE_ALT) && bmP->Parent ()) ? bmP->Parent ()->FrameCount () : bmP->FrameCount ();
		}
	else {
		bmP = SetupHiresAnim (reinterpret_cast<short*> (vcP->frames), nFrames, -1, 0, 1, &nFrames);
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
//draw an CObject which renders as a tVideoClip

#define	FIREBALL_ALPHA		0.8
#define	THRUSTER_ALPHA		(1.0 / 4.0)
#define	WEAPON_ALPHA		0.7

void DrawVClipObject (CObject *objP, fix timeToLive, int bLit, int nVClip, tRgbaColorf *color)
{
	double		ta = 0, alpha = 0;
	tVideoClip	*vcP = gameData.eff.vClips [0] + nVClip;
	int			nFrames = SetupHiresVClip (vcP, &objP->rType.vClipInfo);
	int			iFrame = CurFrame (objP, nVClip, timeToLive);
	int			bThruster = (objP->info.renderType == RT_THRUSTER) && (objP->mType.physInfo.flags & PF_WIGGLE);

if ((objP->info.nType == OBJ_FIREBALL) || (objP->info.nType == OBJ_EXPLOSION)) {
	if (bThruster) {
		alpha = THRUSTER_ALPHA;
		//if (objP->mType.physInfo.flags & PF_WIGGLE)	//CPlayerData ship
			iFrame = nFrames - iFrame - 1;	//render the other way round
		}
	else
		alpha = FIREBALL_ALPHA;
	ta = (double) iFrame / (double) nFrames * alpha;
	alpha = (ta >= 0) ? alpha - ta : alpha + ta;
	}
else if (objP->info.nType == OBJ_WEAPON) {
	if (WeaponIsMine (objP->info.nId))
		alpha = 1.0;
	else
		alpha = WEAPON_ALPHA;
	}
#if 1
if ((objP->info.nType == OBJ_FIREBALL) || (objP->info.nType == OBJ_EXPLOSION))
	glDepthMask (1);	//don't set z-buffer for transparent objects
#endif
if (vcP->flags & VF_ROD)
	DrawObjectRodTexPoly (objP, vcP->frames [iFrame], bLit, iFrame);
else {
	Assert(bLit == 0);		//blob cannot now be bLit
	DrawObjectBlob (objP, vcP->frames [0].index, vcP->frames [iFrame].index, iFrame, color, (float) alpha);
	}
#if 1
if ((objP->info.nType == OBJ_FIREBALL) || (objP->info.nType == OBJ_EXPLOSION))
	glDepthMask (1);
#endif
}

// -----------------------------------------------------------------------------

void DrawExplBlast (CObject *objP)
{
	float			fLife, fAlpha;
	fix			xSize;
	CFixVector	vPos, vDir;
#if BLAST_TYPE == 1
	int			i;
	fix			xSize2;
#elif BLAST_TYPE == 2
	float			r;
	int			i;
	CSphereData	sd;
	CFloatVector	p = {0,0,0};
#endif
	tRgbaColorf	color;
#if 0
	static tRgbaColorf blastColors [] = {
	 {0.5, 0.0f, 0.5f, 1},
	 {1, 0.5f, 0, 1},
	 {1, 0.75f, 0, 1},
	 {1, 1, 1, 3}};
#endif
if (objP->info.xLifeLeft <= 0)
	return;
if (!LoadExplBlast ())
	return;
fLife = X2F (BLAST_LIFE * 2 - objP->info.xLifeLeft);
xSize = (fix) (objP->info.xSize * fLife * BLAST_SCALE);
vPos = objP->info.position.vPos;
#if MOVE_BLAST
vDir = gameData.objs.consoleP->info.position.vPos - vPos;
CFixVector::Normalize (vDir);
vDir *= (xSize - objP->info.xSize);
vPos += vDir;
#endif
glDepthMask (0);
#if BLAST_TYPE == 0
fAlpha = (float) sqrt (X2F (objP->info.xLifeLeft) * 3);
color.red =
color.green =
color.blue =
color.alpha = fAlpha;
G3DrawSprite (vPos, xSize, xSize, bmpExplBlast, &color, fAlpha, 2, 10);
#elif BLAST_TYPE == 1
xSize2 = xSize / 20;
fAlpha = (float) sqrt (X2F (objP->info.xLifeLeft)) / 4;
for (i = 0; i < 4; i++, xSize -= xSize2)
	G3DrawSprite (&vPos, xSize, xSize, bmpExplBlast, &color, fAlpha * blastColors [i].alpha, 0);
#elif BLAST_TYPE == 2
CreateSphere (&sd);
fAlpha = (float) sqrt (X2F (objP->info.xLifeLeft) * 3);
r = X2F (xSize);
sd.pulseP = 0;
transformation.Begin(objP->info.position.vPos, &objP->info.position.mOrient);
for (i = 0; i < 3; i++) {
	RenderSphere (&sd, reinterpret_cast<CFloatVector*> (OOF_VecVms2Oof (&p, &objP->info.position.vPos)),
					  r, r, r, 1, 1, 1, fAlpha, NULL, 1);
	r *= i ? 0.5f : 0.8f;
	}
transformation.End ();
#endif
glDepthMask (1);
}

// -----------------------------------------------------------------------------

void ConvertPowerupToVClip (CObject *objP)
{
objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
objP->rType.vClipInfo.xFrameTime = gameData.eff.vClipP [objP->rType.vClipInfo.nClipIndex].xFrameTime;
objP->rType.vClipInfo.nCurFrame = 0;
objP->info.xSize = gameData.objs.pwrUp.info [objP->info.nId].size;
objP->info.controlType = CT_POWERUP;
objP->info.renderType = RT_POWERUP;
objP->mType.physInfo.mass = I2X (1);
objP->mType.physInfo.drag = 512;
}

// -----------------------------------------------------------------------------

void ConvertWeaponToVClip (CObject *objP)
{
objP->rType.vClipInfo.nClipIndex = gameData.weapons.info [objP->info.nId].nVClipIndex;
objP->rType.vClipInfo.xFrameTime = gameData.eff.vClipP [objP->rType.vClipInfo.nClipIndex].xFrameTime;
objP->rType.vClipInfo.nCurFrame = 0;
objP->info.controlType = CT_WEAPON;
objP->info.renderType = RT_WEAPON_VCLIP;
objP->mType.physInfo.mass = gameData.weapons.info [objP->info.nId].mass;
objP->mType.physInfo.drag = gameData.weapons.info [objP->info.nId].drag;
objP->info.xSize = gameData.weapons.info [objP->info.nId].strength [gameStates.app.nDifficultyLevel] / 10;
objP->info.movementType = MT_PHYSICS;
}

// -----------------------------------------------------------------------------

int ConvertVClipToPolymodel (CObject *objP)
{
	CAngleVector	a;
	short			nModel;

if (gameStates.app.bNostalgia || !gameOpts->render.powerups.b3D)
	return 0;
if (objP->info.renderType == RT_POLYOBJ)
	return 1;
nModel = WeaponToModel (objP->info.nId);
if (!(nModel && HaveReplacementModel (nModel)))
	return 0;
a[PA] = (rand () % I2X (1)) - I2X (1) / 2;
a[BA] = (rand () % I2X (1)) - I2X (1) / 2;
a[HA] = (rand () % I2X (1)) - I2X (1) / 2;
objP->info.position.mOrient = CFixMatrix::Create(a);
#if 0
objP->mType.physInfo.mass = I2X (1);
objP->mType.physInfo.drag = 512;
#endif
objP->mType.physInfo.rotVel[Z] =
objP->mType.physInfo.rotVel[Y] = 0;
objP->mType.physInfo.rotVel[X] = gameOpts->render.powerups.nSpin ? I2X (1) / (5 - gameOpts->render.powerups.nSpin) : 0;
//objP->info.controlType = CT_WEAPON;
objP->info.renderType = RT_POLYOBJ;
objP->info.movementType = MT_PHYSICS;
objP->mType.physInfo.flags = PF_BOUNCE | PF_FREE_SPINNING;
if (0 > (objP->rType.polyObjInfo.nModel = gameData.weapons.info [objP->info.nId].nModel))
	objP->rType.polyObjInfo.nModel = nModel;
#if 0
objP->info.xSize = FixDiv (gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad,
							gameData.weapons.info [objP->info.nId].po_len_to_width_ratio);
#endif
objP->rType.polyObjInfo.nTexOverride = -1;
if (objP->info.nType == OBJ_POWERUP)
	objP->info.xLifeLeft = IMMORTAL_TIME;
return 1;
}

//------------------------------------------------------------------------------

void DrawWeaponVClip (CObject *objP)
{
	int	nVClip;
	fix	modtime, playTime;

Assert (objP->info.nType == OBJ_WEAPON);
nVClip = gameData.weapons.info [objP->info.nId].nVClipIndex;
modtime = objP->info.xLifeLeft;
playTime = gameData.eff.vClipP [nVClip].xTotalTime;
//	Special values for modtime were causing enormous slowdown for omega blobs.
if (modtime == IMMORTAL_TIME)
	modtime = playTime;
//	Should cause Omega blobs (which live for one frame) to not always be the same.
if (modtime == ONE_FRAME_TIME)
	modtime = d_rand();
if (objP->info.nId == PROXMINE_ID) {		//make prox bombs spin out of sync
	int nObject = objP->Index ();
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
	DrawVClipObject (objP, modtime, 0, nVClip, gameData.weapons.color + objP->info.nId);
}

//------------------------------------------------------------------------------

/*
 * reads n tVideoClip structs from a CFile
 */
void ReadVideoClip (tVideoClip& vc, CFile& cf)
{
vc.xTotalTime = cf.ReadFix ();
vc.nFrameCount = cf.ReadInt ();
vc.xFrameTime = cf.ReadFix ();
vc.flags = cf.ReadInt ();
vc.nSound = cf.ReadShort ();
for (int j = 0; j < VCLIP_MAX_FRAMES; j++)
	vc.frames [j].index = cf.ReadShort ();
vc.lightValue = cf.ReadFix ();
}


int ReadVideoClips (CArray<tVideoClip>& vc, int n, CFile& cf)
{
	int i;

for (i = 0; i < n; i++)
	ReadVideoClip (vc [i], cf);
return i;
}

//------------------------------------------------------------------------------
