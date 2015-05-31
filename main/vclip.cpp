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

#include "descent.h"
#include "error.h"
#include "timer.h"
#include "rendermine.h"
#include "objrender.h"
#include "fireball.h"
#include "sphere.h"
#include "u_mem.h"
#include "hiresmodels.h"
#include "addon_bitmaps.h"
#include "transprender.h"

#define BLAST_TYPE 0
#define MOVE_BLAST 1

#define BLAST_SCALE	(I2X (6) / BLAST_LIFE)

//----------------- Variables for video clips -------------------

inline int32_t CurFrame (CObject *pObj, int32_t nClip, fix timeToLive, int32_t nFrames = -1)
{
tAnimationInfo	*pAnimInfo = gameData.effectData.animations [0] + nClip;
if (nFrames < 0)
	nFrames = pAnimInfo->nFrameCount;
//	iFrame = (nFrames - X2I (FixDiv ((nFrames - 1) * timeToLive, pAnimInfo->xTotalTime))) - 1;
int32_t iFrame;
if (timeToLive > pAnimInfo->xTotalTime)
	timeToLive = timeToLive % pAnimInfo->xTotalTime;
if ((nClip == ANIM_AFTERBURNER_BLOB) && pObj->rType.animationInfo.xFrameTime)
	iFrame = (pObj->rType.animationInfo.xTotalTime - timeToLive) / pObj->rType.animationInfo.xFrameTime;
else
	iFrame = (pAnimInfo->xTotalTime - timeToLive) / pAnimInfo->xFrameTime;
return (iFrame < nFrames) ? iFrame : nFrames - 1;
}

//------------------------------------------------------------------------------

CRGBColor *AnimationColor (CObject *pObj)
{
	CWeaponInfo		*pWeaponInfo = WEAPONINFO (pObj);

if (!pWeaponInfo)
	return NULL;

	int32_t			nVClip = pWeaponInfo->nAnimationIndex;
	tBitmapIndex	bmi;
	CBitmap			*pBm;

if (nVClip) {
	tAnimationInfo *pAnimInfo = gameData.effectData.animations [0] + nVClip;
	bmi = pAnimInfo->frames [0];
	}
else
	bmi = pWeaponInfo->bitmap;
LoadTexture (bmi.index, 0, 0);
pBm = gameData.pigData.tex.bitmaps [0] + bmi.index;
if ((pBm->Type () == BM_TYPE_STD) && pBm->Override ())
	pBm = pBm->Override ();
pBm->AvgColor (NULL, false);
return pBm->GetAvgColor ();
}

//------------------------------------------------------------------------------

int32_t SetupHiresVClip (tAnimationInfo *pAnimInfo, tAnimationState *pState, CBitmap* pBm)
{
	int32_t nFrames = pAnimInfo->nFrameCount;

if (pAnimInfo->flags & WCF_ALTFMT) {
	if (pAnimInfo->flags & WCF_INITIALIZED) {
		pBm = gameData.pigData.tex.bitmaps [0][pAnimInfo->frames [0].index].Override ();
		nFrames = ((pBm->Type () != BM_TYPE_ALT) && pBm->Parent ()) ? pBm->Parent ()->FrameCount () : pBm->FrameCount ();
		}
	else {
		pBm = SetupHiresAnim (reinterpret_cast<int16_t*> (pAnimInfo->frames), nFrames, -1, 0, pState ? 1 : -1, &nFrames, pBm);
		if (!pBm)
			pAnimInfo->flags &= ~WCF_ALTFMT;
		else if (!gameOpts->ogl.bGlTexMerge)
			pAnimInfo->flags &= ~WCF_ALTFMT;
		else
			pAnimInfo->flags |= WCF_INITIALIZED;
		if (pState)
			pState->nCurFrame = Rand (nFrames);
		}
	}
return nFrames;
}

//------------------------------------------------------------------------------
//draw an CObject which renders as a tAnimationInfo

#define	FIREBALL_ALPHA		0.8
#define	THRUSTER_ALPHA		(1.0 / 4.0)
#define	WEAPON_ALPHA		0.7

void DrawVClipObject (CObject *pObj, fix timeToLive, int32_t bLit, int32_t nVClip, CFloatVector *color)
{
	double		ta = 0, alpha = 0;
	tAnimationInfo*	pAnimInfo = gameData.effectData.animations [0] + nVClip;
	int32_t		nFrames = SetupHiresVClip (pAnimInfo, &pObj->rType.animationInfo);
	int32_t		iFrame = CurFrame (pObj, nVClip, timeToLive, nFrames);
	int32_t		bThruster = (pObj->info.renderType == RT_THRUSTER) && (pObj->mType.physInfo.flags & PF_WIGGLE);

if ((iFrame < 0) || (iFrame >= MAX_ANIMATION_FRAMES))
	return;
if ((pObj->info.nType == OBJ_FIREBALL) || (pObj->info.nType == OBJ_EXPLOSION)) {
	if (bThruster) {
		alpha = THRUSTER_ALPHA;
		//if (pObj->mType.physInfo.flags & PF_WIGGLE)	//CPlayerData ship
			iFrame = nFrames - iFrame - 1;	//render the other way round
		}
	else
		alpha = FIREBALL_ALPHA;
	ta = (double) iFrame / (double) nFrames * alpha;
	alpha = (ta >= 0) ? alpha - ta : alpha + ta;
	}
else if (pObj->info.nType == OBJ_WEAPON) {
	if (pObj->IsMine ())
		alpha = 1.0;
	else
		alpha = WEAPON_ALPHA;
	}
#if 0
if ((pObj->info.nType == OBJ_FIREBALL) || (pObj->info.nType == OBJ_EXPLOSION))
	ogl.SetDepthWrite (true);	//don't set z-buffer for transparent objects
#endif
if (pAnimInfo->flags & VF_ROD)
	DrawObjectRodTexPoly (pObj, pAnimInfo->frames [iFrame], bLit, iFrame);
else
	DrawObjectBitmap (pObj, pAnimInfo->frames [0].index, pAnimInfo->frames [iFrame].index, iFrame, color, (float) alpha);
#if 1
if ((pObj->info.nType == OBJ_FIREBALL) || (pObj->info.nType == OBJ_EXPLOSION))
	ogl.SetDepthWrite (true);
#endif
}

// -----------------------------------------------------------------------------

void DrawExplBlast (CObject *pObj)
{
#if BLAST_TYPE == 1
	int32_t			i;
	fix			xSize2;
#elif BLAST_TYPE == 2
	float			r;
	int32_t			i;
	CSphereData	sd;
	CFloatVector	p = {0,0,0};
#endif
	CFloatVector	color;
#if 0
	static CFloatVector blastColors [] = {
	 {0.5, 0.0f, 0.5f, 1},
	 {1, 0.5f, 0, 1},
	 {1, 0.75f, 0, 1},
	 {1, 1, 1, 3}};
#endif
if (pObj->info.xLifeLeft <= 0)
	return;
if (!explBlast.Load ())
	return;

float fScale;
if (true || pObj->Collapsing ()) {
	fix xTotalLife = pObj->TotalLife ();
	fix xPivot = xTotalLife / 8;
	if (pObj->LifeLeft () < xPivot)
		fScale = 1.0f - X2F (xPivot - pObj->LifeLeft ()) / X2F (xPivot);
	else
		fScale = 1.0f - X2F (pObj->LifeLeft () - xPivot) / X2F (xTotalLife - xPivot);
	}
else {
	fScale = 1.0f - X2F (pObj->LifeLeft ()) / X2F (pObj->TotalLife ());
	}

fix xSize = pObj->info.xSize + (fix) ((float) pObj->info.xSize * fScale * 12);
CFixVector vPos = pObj->info.position.vPos;
#if MOVE_BLAST
CFixVector vViewPos;
if (gameStates.render.bChaseCam)
	FLIGHTPATH.GetViewPoint (&vViewPos);
else
	vViewPos = transformation.m_info.viewer;
CFixVector vDir = vViewPos - vPos;
CFixVector::Normalize (vDir);
vDir *= (xSize - pObj->info.xSize);
vPos += vDir;
#endif
ogl.SetDepthWrite (false);
#if BLAST_TYPE == 0
float fAlpha = (float) sqrt (fScale);
color.Red () =
color.Green () =
color.Blue () =
color.Alpha () = fAlpha;
explBlast.Bitmap ()->SetColor (&color);
ogl.RenderSprite (explBlast.Bitmap (), vPos, xSize, xSize, fAlpha, 2, 10);
#elif BLAST_TYPE == 1
xSize2 = xSize / 20;
fAlpha = (float) sqrt (X2F (pObj->info.xLifeLeft)) / 4;
for (i = 0; i < 4; i++, xSize -= xSize2)
	ogl.RenderSprite (explBlast.Bitmap (), &vPos, xSize, xSize, &color, fAlpha * blastColors [i].Alpha (), 0);
#elif BLAST_TYPE == 2
CreateSphere (&sd);
fAlpha = (float) sqrt (X2F (pObj->info.xLifeLeft) * 3);
r = X2F (xSize);
sd.pPulse = 0;
transformation.Begin (pObj->info.position.vPos, &pObj->info.position.mOrient, __FILE__, __LINE__);
for (i = 0; i < 3; i++) {
	RenderSphere (&sd, reinterpret_cast<CFloatVector*> (OOF_VecVms2Oof (&p, &pObj->info.position.vPos)),
					  r, r, r, 1, 1, 1, fAlpha, NULL, 1);
	r *= i ? 0.5f : 0.8f;
	}
transformation.End (__FILE__, __LINE__);
#endif
ogl.SetDepthWrite (true);
}

// -----------------------------------------------------------------------------

void DrawShockwave (CObject *pObj)
{

	CBitmap* pBm = shockwave.Bitmap (SHOCKWAVE_LIFE, pObj->info.xLifeLeft);

if (!pBm)
	return;

	CFloatVector	v, vertices [4];

	static tTexCoord2f texCoord [4] = {{{0,0}},{{0,1}},{{1,1}},{{1,0}}};
	static CFloatVector color = {{{1.0f, 1.0f, 1.0f, 1.0f}}};

if (pObj->info.xLifeLeft <= 0)
	return;
if (!shockwave.Load ())
	return;

vertices [0].Assign (pObj->Position ());
vertices [1] = 
vertices [2] = 
vertices [3] = vertices [0];

float fSize = X2F (pObj->info.xSize);
v.Assign (pObj->Orientation ().m.dir.f);
v *= fSize;
vertices [0] += v;
vertices [2] -= v;
//vertices [0] += r;
//vertices [1] += f;
v.Assign (pObj->Orientation ().m.dir.r);
v *= fSize;
vertices [1] -= v;
vertices [3] += v;
//vertices [2] -= r;
//vertices [3] -= f;

pBm->SetColor (&color);
#if 0
ogl.RenderSprite (pBm, pObj->Position (), pObj->info.xSize * 5, pObj->info.xSize * 5, 1.0f, 2, 10.0f);
#else
for (int32_t i = 0; i < 4; i++)
	transformation.Transform (vertices [i], vertices [i], 0);
transparencyRenderer.AddPoly (NULL, NULL, pBm, vertices, 4, texCoord, &color, NULL, 1, 0, GL_QUADS, GL_CLAMP, 2, -1);
#endif
}

// -----------------------------------------------------------------------------

void ConvertPowerupToVClip (CObject *pObj)
{
pObj->rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [pObj->info.nId].nClipIndex;
pObj->rType.animationInfo.xFrameTime = gameData.effectData.vClipP [pObj->rType.animationInfo.nClipIndex].xFrameTime;
pObj->rType.animationInfo.nCurFrame = 0;
pObj->SetSizeFromPowerup ();
pObj->info.controlType = CT_POWERUP;
pObj->info.renderType = RT_POWERUP;
pObj->mType.physInfo.mass = I2X (1);
pObj->mType.physInfo.drag = 512;
}

// -----------------------------------------------------------------------------

void ConvertWeaponToVClip (CObject *pObj)
{
	CWeaponInfo *pWeaponInfo = WEAPONINFO (pObj);

if (!pWeaponInfo)
	return;

pObj->rType.animationInfo.nClipIndex = pWeaponInfo->nAnimationIndex;
pObj->rType.animationInfo.xFrameTime = gameData.effectData.vClipP [pObj->rType.animationInfo.nClipIndex].xFrameTime;
pObj->rType.animationInfo.nCurFrame = 0;
pObj->info.controlType = CT_WEAPON;
pObj->info.renderType = RT_WEAPON_VCLIP;
pObj->mType.physInfo.mass = pWeaponInfo->mass;
pObj->mType.physInfo.drag = pWeaponInfo->drag;
pObj->SetSize (pWeaponInfo->strength [gameStates.app.nDifficultyLevel] / 10);
pObj->info.movementType = MT_PHYSICS;
}

// -----------------------------------------------------------------------------

void SetupSpin (CObject *pObj, bool bOrient);

int32_t ConvertVClipToPolymodel (CObject *pObj)
{
if (gameStates.app.bNostalgia || !gameOpts->Use3DPowerups ())
	return 0;
if (pObj->info.renderType == RT_POLYOBJ)
	return 1;
int16_t	nModel = WeaponToModel (pObj->info.nId);
if (!(nModel && HaveReplacementModel (nModel)))
	return 0;
#if 0 //DBG
pObj->Orientation () = gameData.objData.pConsole->Orientation ();
pObj->mType.physInfo.rotVel.v.coord.x =
pObj->mType.physInfo.rotVel.v.coord.y =
pObj->mType.physInfo.rotVel.v.coord.z = 0;
#else
SetupSpin (pObj, true);
#endif
#if 0
pObj->mType.physInfo.mass = I2X (1);
pObj->mType.physInfo.drag = 512;
#endif
//pObj->info.controlType = CT_WEAPON;
pObj->info.renderType = RT_POLYOBJ;
pObj->info.movementType = MT_PHYSICS;
pObj->mType.physInfo.flags = PF_BOUNCES | PF_FREE_SPINNING;
CWeaponInfo *pWeaponInfo = WEAPONINFO (pObj);
pObj->rType.polyObjInfo.nModel = pWeaponInfo ? pWeaponInfo->nModel : -1;
if (0 > pObj->rType.polyObjInfo.nModel)
	pObj->rType.polyObjInfo.nModel = nModel;
pObj->rType.polyObjInfo.nTexOverride = -1;
if (pObj->info.nType == OBJ_POWERUP)
	pObj->SetLife (IMMORTAL_TIME);
return 1;
}

//------------------------------------------------------------------------------

void DrawWeaponVClip (CObject *pObj)
{
CWeaponInfo *pWeaponInfo = WEAPONINFO (pObj);
if (!pWeaponInfo)
	return;
int32_t nVClip = pWeaponInfo->nAnimationIndex;
fix modTime = pObj->info.xLifeLeft;
fix playTime = gameData.effectData.vClipP [nVClip].xTotalTime;
if (!playTime)
	return;
//	Special values for modTime were causing enormous slowdown for omega blobs.
if (modTime == IMMORTAL_TIME)
	modTime = playTime;
//	Should cause Omega blobs (which live for one frame) to not always be the same.
if (modTime == ONE_FRAME_TIME)
	modTime = RandShort ();
if (pObj->info.nId == PROXMINE_ID) {		//make prox bombs spin out of sync
	int32_t nObject = pObj->Index ();
	modTime += (modTime * (nObject & 7)) / 16;	//add variance to spin rate
	while (modTime > playTime)
		modTime -= playTime;
	if ((nObject & 1) ^ ((nObject >> 1) & 1))			//make some spin other way
		modTime = playTime - modTime;
	}
else {
	if (modTime > playTime)
		modTime %= playTime;
	}
if (ConvertVClipToPolymodel (pObj))
	DrawPolygonObject (pObj, 0);
else
	DrawVClipObject (pObj, modTime, 0, nVClip, gameData.weaponData.color + pObj->info.nId);
}

//------------------------------------------------------------------------------

/*
 * reads n tAnimationInfo structs from a CFile
 */
void ReadVideoClip (tAnimationInfo& vc, CFile& cf)
{
vc.xTotalTime = cf.ReadFix ();
vc.nFrameCount = cf.ReadInt ();
vc.xFrameTime = cf.ReadFix ();
vc.flags = cf.ReadInt ();
vc.nSound = cf.ReadShort ();
for (int32_t j = 0; j < MAX_ANIMATION_FRAMES; j++) {
	vc.frames [j].index = cf.ReadShort ();
#if DBG
	if ((nDbgTexture >= 0) && (vc.frames [j].index == nDbgTexture))
		BRP;
#endif
	}
vc.lightValue = cf.ReadFix ();
}


int32_t ReadAnimationInfo (CArray<tAnimationInfo>& vc, int32_t n, CFile& cf)
{
	int32_t i;

for (i = 0; i < n; i++)
	ReadVideoClip (vc [i], cf);
return i;
}

//------------------------------------------------------------------------------
