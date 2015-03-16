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

inline int32_t CurFrame (CObject *objP, int32_t nClip, fix timeToLive, int32_t nFrames = -1)
{
tAnimationInfo	*animInfoP = gameData.effects.animations [0] + nClip;
if (nFrames < 0)
	nFrames = animInfoP->nFrameCount;
//	iFrame = (nFrames - X2I (FixDiv ((nFrames - 1) * timeToLive, animInfoP->xTotalTime))) - 1;
int32_t iFrame;
if (timeToLive > animInfoP->xTotalTime)
	timeToLive = timeToLive % animInfoP->xTotalTime;
if ((nClip == ANIM_AFTERBURNER_BLOB) && objP->rType.animationInfo.xFrameTime)
	iFrame = (objP->rType.animationInfo.xTotalTime - timeToLive) / objP->rType.animationInfo.xFrameTime;
else
	iFrame = (animInfoP->xTotalTime - timeToLive) / animInfoP->xFrameTime;
return (iFrame < nFrames) ? iFrame : nFrames - 1;
}

//------------------------------------------------------------------------------

CRGBColor *AnimationColor (CObject *objP)
{
	int32_t				nVClip = gameData.weapons.info [objP->info.nId].nAnimationIndex;
	tBitmapIndex	bmi;
	CBitmap			*bmP;

if (nVClip) {
	tAnimationInfo *animInfoP = gameData.effects.animations [0] + nVClip;
	bmi = animInfoP->frames [0];
	}
else
	bmi = gameData.weapons.info [objP->info.nId].bitmap;
LoadTexture (bmi.index, 0, 0);
bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
if ((bmP->Type () == BM_TYPE_STD) && bmP->Override ())
	bmP = bmP->Override ();
bmP->AvgColor (NULL, false);
return bmP->GetAvgColor ();
}

//------------------------------------------------------------------------------

int32_t SetupHiresVClip (tAnimationInfo *animInfoP, tAnimationState *vciP, CBitmap* bmP)
{
	int32_t nFrames = animInfoP->nFrameCount;

if (animInfoP->flags & WCF_ALTFMT) {
	if (animInfoP->flags & WCF_INITIALIZED) {
		bmP = gameData.pig.tex.bitmaps [0][animInfoP->frames [0].index].Override ();
		nFrames = ((bmP->Type () != BM_TYPE_ALT) && bmP->Parent ()) ? bmP->Parent ()->FrameCount () : bmP->FrameCount ();
		}
	else {
		bmP = SetupHiresAnim (reinterpret_cast<int16_t*> (animInfoP->frames), nFrames, -1, 0, vciP ? 1 : -1, &nFrames, bmP);
		if (!bmP)
			animInfoP->flags &= ~WCF_ALTFMT;
		else if (!gameOpts->ogl.bGlTexMerge)
			animInfoP->flags &= ~WCF_ALTFMT;
		else
			animInfoP->flags |= WCF_INITIALIZED;
		if (vciP)
			vciP->nCurFrame = Rand (nFrames);
		}
	}
return nFrames;
}

//------------------------------------------------------------------------------
//draw an CObject which renders as a tAnimationInfo

#define	FIREBALL_ALPHA		0.8
#define	THRUSTER_ALPHA		(1.0 / 4.0)
#define	WEAPON_ALPHA		0.7

void DrawVClipObject (CObject *objP, fix timeToLive, int32_t bLit, int32_t nVClip, CFloatVector *color)
{
	double		ta = 0, alpha = 0;
	tAnimationInfo*	animInfoP = gameData.effects.animations [0] + nVClip;
	int32_t		nFrames = SetupHiresVClip (animInfoP, &objP->rType.animationInfo);
	int32_t		iFrame = CurFrame (objP, nVClip, timeToLive, nFrames);
	int32_t		bThruster = (objP->info.renderType == RT_THRUSTER) && (objP->mType.physInfo.flags & PF_WIGGLE);

if ((iFrame < 0) || (iFrame >= MAX_ANIMATION_FRAMES))
	return;
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
	if (objP->IsMine ())
		alpha = 1.0;
	else
		alpha = WEAPON_ALPHA;
	}
#if 0
if ((objP->info.nType == OBJ_FIREBALL) || (objP->info.nType == OBJ_EXPLOSION))
	ogl.SetDepthWrite (true);	//don't set z-buffer for transparent objects
#endif
if (animInfoP->flags & VF_ROD)
	DrawObjectRodTexPoly (objP, animInfoP->frames [iFrame], bLit, iFrame);
else
	DrawObjectBitmap (objP, animInfoP->frames [0].index, animInfoP->frames [iFrame].index, iFrame, color, (float) alpha);
#if 1
if ((objP->info.nType == OBJ_FIREBALL) || (objP->info.nType == OBJ_EXPLOSION))
	ogl.SetDepthWrite (true);
#endif
}

// -----------------------------------------------------------------------------

void DrawExplBlast (CObject *objP)
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
if (objP->info.xLifeLeft <= 0)
	return;
if (!explBlast.Load ())
	return;

float fScale;
if (true || objP->Collapsing ()) {
	fix xTotalLife = objP->TotalLife ();
	fix xPivot = xTotalLife / 8;
	if (objP->LifeLeft () < xPivot)
		fScale = 1.0f - X2F (xPivot - objP->LifeLeft ()) / X2F (xPivot);
	else
		fScale = 1.0f - X2F (objP->LifeLeft () - xPivot) / X2F (xTotalLife - xPivot);
	}
else {
	fScale = 1.0f - X2F (objP->LifeLeft ()) / X2F (objP->TotalLife ());
	}

fix xSize = objP->info.xSize + (fix) ((float) objP->info.xSize * fScale * 12);
CFixVector vPos = objP->info.position.vPos;
#if MOVE_BLAST
CFixVector vViewPos;
if (gameStates.render.bChaseCam)
	FLIGHTPATH.GetViewPoint (&vViewPos);
else
	vViewPos = OBJPOS (gameData.objs.consoleP)->vPos;
CFixVector vDir = vViewPos - vPos;
CFixVector::Normalize (vDir);
vDir *= (xSize - objP->info.xSize);
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
fAlpha = (float) sqrt (X2F (objP->info.xLifeLeft)) / 4;
for (i = 0; i < 4; i++, xSize -= xSize2)
	ogl.RenderSprite (explBlast.Bitmap (), &vPos, xSize, xSize, &color, fAlpha * blastColors [i].Alpha (), 0);
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
ogl.SetDepthWrite (true);
}

// -----------------------------------------------------------------------------

void DrawShockwave (CObject *objP)
{

	CBitmap* bmP = shockwave.Bitmap (SHOCKWAVE_LIFE, objP->info.xLifeLeft);

if (!bmP)
	return;

	CFloatVector	v, vertices [4];

	static tTexCoord2f texCoord [4] = {{{0,0}},{{0,1}},{{1,1}},{{1,0}}};
	static CFloatVector color = {{{1.0f, 1.0f, 1.0f, 1.0f}}};

if (objP->info.xLifeLeft <= 0)
	return;
if (!shockwave.Load ())
	return;

vertices [0].Assign (objP->Position ());
vertices [1] = 
vertices [2] = 
vertices [3] = vertices [0];

float fSize = X2F (objP->info.xSize);
v.Assign (objP->Orientation ().m.dir.f);
v *= fSize;
vertices [0] += v;
vertices [2] -= v;
//vertices [0] += r;
//vertices [1] += f;
v.Assign (objP->Orientation ().m.dir.r);
v *= fSize;
vertices [1] -= v;
vertices [3] += v;
//vertices [2] -= r;
//vertices [3] -= f;

bmP->SetColor (&color);
#if 0
ogl.RenderSprite (bmP, objP->Position (), objP->info.xSize * 5, objP->info.xSize * 5, 1.0f, 2, 10.0f);
#else
for (int32_t i = 0; i < 4; i++)
	transformation.Transform (vertices [i], vertices [i], 0);
transparencyRenderer.AddPoly (NULL, NULL, bmP, vertices, 4, texCoord, &color, NULL, 1, 0, GL_QUADS, GL_CLAMP, 2, -1);
#endif
}

// -----------------------------------------------------------------------------

void ConvertPowerupToVClip (CObject *objP)
{
objP->rType.animationInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
objP->rType.animationInfo.xFrameTime = gameData.effects.vClipP [objP->rType.animationInfo.nClipIndex].xFrameTime;
objP->rType.animationInfo.nCurFrame = 0;
objP->SetSizeFromPowerup ();
objP->info.controlType = CT_POWERUP;
objP->info.renderType = RT_POWERUP;
objP->mType.physInfo.mass = I2X (1);
objP->mType.physInfo.drag = 512;
}

// -----------------------------------------------------------------------------

void ConvertWeaponToVClip (CObject *objP)
{
objP->rType.animationInfo.nClipIndex = gameData.weapons.info [objP->info.nId].nAnimationIndex;
objP->rType.animationInfo.xFrameTime = gameData.effects.vClipP [objP->rType.animationInfo.nClipIndex].xFrameTime;
objP->rType.animationInfo.nCurFrame = 0;
objP->info.controlType = CT_WEAPON;
objP->info.renderType = RT_WEAPON_VCLIP;
objP->mType.physInfo.mass = gameData.weapons.info [objP->info.nId].mass;
objP->mType.physInfo.drag = gameData.weapons.info [objP->info.nId].drag;
objP->SetSize (gameData.weapons.info [objP->info.nId].strength [gameStates.app.nDifficultyLevel] / 10);
objP->info.movementType = MT_PHYSICS;
}

// -----------------------------------------------------------------------------

int32_t ConvertVClipToPolymodel (CObject *objP)
{
if (gameStates.app.bNostalgia || !gameOpts->Use3DPowerups ())
	return 0;
if (objP->info.renderType == RT_POLYOBJ)
	return 1;
int16_t	nModel = WeaponToModel (objP->info.nId);
if (!(nModel && HaveReplacementModel (nModel)))
	return 0;
#if 0 //DBG
objP->Orientation () = gameData.objs.consoleP->Orientation ();
objP->mType.physInfo.rotVel.v.coord.x =
objP->mType.physInfo.rotVel.v.coord.y =
objP->mType.physInfo.rotVel.v.coord.z = 0;
#else
CAngleVector a;
a.v.coord.p = 2 * SRandShort ();
a.v.coord.b = 2 * SRandShort ();
a.v.coord.h = 2 * SRandShort ();
objP->info.position.mOrient = CFixMatrix::Create (a);
objP->mType.physInfo.rotVel.v.coord.z =
objP->mType.physInfo.rotVel.v.coord.y = 0;
objP->mType.physInfo.rotVel.v.coord.x = gameOpts->render.powerups.nSpin ? I2X (1) / (5 - gameOpts->render.powerups.nSpin) : 0;
#endif
#if 0
objP->mType.physInfo.mass = I2X (1);
objP->mType.physInfo.drag = 512;
#endif
//objP->info.controlType = CT_WEAPON;
objP->info.renderType = RT_POLYOBJ;
objP->info.movementType = MT_PHYSICS;
objP->mType.physInfo.flags = PF_BOUNCE | PF_FREE_SPINNING;
if (0 > (objP->rType.polyObjInfo.nModel = gameData.weapons.info [objP->info.nId].nModel))
	objP->rType.polyObjInfo.nModel = nModel;
#if 0
objP->AdjustSize (0, gameData.weapons.info [objP->info.nId].poLenToWidthRatio);
#endif
objP->rType.polyObjInfo.nTexOverride = -1;
if (objP->info.nType == OBJ_POWERUP)
	objP->SetLife (IMMORTAL_TIME);
return 1;
}

//------------------------------------------------------------------------------

void DrawWeaponVClip (CObject *objP)
{
int32_t nVClip = gameData.weapons.info [objP->info.nId].nAnimationIndex;
fix modTime = objP->info.xLifeLeft;
fix playTime = gameData.effects.vClipP [nVClip].xTotalTime;
if (!playTime)
	return;
//	Special values for modTime were causing enormous slowdown for omega blobs.
if (modTime == IMMORTAL_TIME)
	modTime = playTime;
//	Should cause Omega blobs (which live for one frame) to not always be the same.
if (modTime == ONE_FRAME_TIME)
	modTime = RandShort ();
if (objP->info.nId == PROXMINE_ID) {		//make prox bombs spin out of sync
	int32_t nObject = objP->Index ();
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
if (ConvertVClipToPolymodel (objP))
	DrawPolygonObject (objP, 0);
else
	DrawVClipObject (objP, modTime, 0, nVClip, gameData.weapons.color + objP->info.nId);
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
for (int32_t j = 0; j < MAX_ANIMATION_FRAMES; j++)
	vc.frames [j].index = cf.ReadShort ();
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
