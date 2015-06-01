#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "descent.h"
#include "newdemo.h"
#include "network.h"
#include "interp.h"
#include "ogl_lib.h"
#include "rendermine.h"
#include "transprender.h"
#include "glare.h"
#include "sphere.h"
#include "marker.h"
#include "fireball.h"
#include "objsmoke.h"
#include "objrender.h"
#include "objeffects.h"
#include "hiresmodels.h"
#include "hitbox.h"
#include "addon_bitmaps.h"

#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

#define LIGHTTRAIL_BLENDMODE	1	// 1:GL_ONE, GL_ONE, 2: GL_ONE, GL_ONE_MINUS_SOURCE_COLOR

// -----------------------------------------------------------------------------

void RenderObjectHalo (CFixVector *vPos, fix xSize, float red, float green, float blue, float alpha, int32_t bCorona)
{
if ((gameOpts->render.coronas.bShots && (bCorona ? corona.Load () : halo.Load ()))) {
	CFloatVector	c = {{{red, green, blue, alpha}}};
	ogl.SetDepthWrite (false);
	CBitmap* pBm = bCorona ? corona.Bitmap () : halo.Bitmap ();
	pBm->SetColor (&c);
	ogl.RenderSprite (pBm, *vPos, xSize, xSize, alpha * 4.0f / 3.0f, LIGHTTRAIL_BLENDMODE, 1);
	ogl.SetDepthWrite (true);
	}
}

// -----------------------------------------------------------------------------

void RenderPowerupCorona (CObject *pObj, float red, float green, float blue, float alpha)
{
ENTER (1, 0);
if ((IsEnergyPowerup (pObj->info.nId) ? gameOpts->render.coronas.bPowerups : gameOpts->render.coronas.bWeapons) && glare.Load ()) {
	static CFloatVector keyColors [3] = {
	 {{{0.2f, 0.2f, 0.9f, 0.2f}}},
	 {{{0.9f, 0.2f, 0.2f, 0.2f}}},
	 {{{0.9f, 0.8f, 0.2f, 0.2f}}}
		};

	CFloatVector	color;
	fix				xSize;
	float				fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] * 0.5f;

	if ((pObj->info.nId >= POW_KEY_BLUE) && (pObj->info.nId <= POW_KEY_GOLD)) {
		int32_t i = pObj->info.nId - POW_KEY_BLUE;

		color = keyColors [(((i < 0) || (i > 2)) ? 3 : i)];
		xSize = I2X (12);
		}
	else {
		float b = (pObj->info.nId >= POW_SHIELD_BOOST) ? Max (red, Max (green, blue)) * 0.5f : float (sqrt ((red + green + blue) / 3.0f));
		color.Set (red, green, blue);
		color /= b;
		xSize = 2 * pObj->info.xSize; //I2X (8);
		}
	color *= fScale;
	color.Alpha () = alpha;
	glare.Bitmap ()->SetColor (&color);
	ogl.RenderSprite (glare.Bitmap (), pObj->info.position.vPos, xSize, xSize, alpha, -LIGHTTRAIL_BLENDMODE, 5);
	glare.Bitmap ()->SetColor (NULL);
	}
LEAVE
}

// -----------------------------------------------------------------------------

void RenderLaserCorona (CObject *pObj, CFloatVector *pColor, float alpha, float fScale)
{
ENTER (1, 0);
if (!SHOW_OBJ_FX)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if (gameOpts->render.coronas.bShots && glare.Load ()) {
	tHitbox*			phb = &gameData.modelData.hitboxes [pObj->ModelId ()].hitboxes [0];
	float				fLength = X2F (phb->vMax.v.coord.z - phb->vMin.v.coord.z) / 2;
	CFloatVector	color;

	float fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
	color = *pColor;
	pColor = &color;
	color *= fScale;
	color.Alpha () = alpha;
	glare.Bitmap ()->SetColor (pColor);
	ogl.RenderSprite (glare.Bitmap (), pObj->info.position.vPos + pObj->info.position.mOrient.m.dir.f * (F2X (fLength - 0.5f)), I2X (1), I2X (1), alpha, LIGHTTRAIL_BLENDMODE, 1);
	glare.Bitmap ()->SetColor (NULL);
	}
LEAVE
}

// -----------------------------------------------------------------------------

static inline float WeaponBlobSize (int32_t nId)
{
if (nId == PHOENIX_ID)
	return 2.25f;
else if (nId == PLASMA_ID)
	return 2.25f;
else if (nId == HELIX_ID)
	return 1.25f;
else if (nId == SPREADFIRE_ID)
	return 1.25f;
else if (nId == OMEGA_ID)
	return 1.5f;
else if (SMARTMSL_BLOB_ID)
	return 2.25f;
else if (ROBOT_SMARTMSL_BLOB_ID)
	return 2.25f;
else if (SMARTMINE_BLOB_ID)
	return 2.25f;
else if (ROBOT_SMARTMINE_BLOB_ID)
	return 2.25f;
else
	return 1.0f;
}

// -----------------------------------------------------------------------------

int32_t RenderWeaponCorona (CObject *pObj, CFloatVector *pColor, float alpha, fix xOffset,
									 float fScale, int32_t bSimple, int32_t bViewerOffset, int32_t bDepthSort)
{
ENTER (1, 0);
if (!SHOW_OBJ_FX)
	RETURN (0)
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	RETURN (0)
if ((pObj->info.nType == OBJ_WEAPON) && (pObj->info.renderType == RT_POLYOBJ))
	RenderLaserCorona (pObj, pColor, alpha, fScale);
else if (gameOpts->render.coronas.bShots && corona.Load ()) {
	fix				xSize;
	CFloatVector	color;

	//static tTexCoord2f	tcCorona [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

	CFixVector	vPos = pObj->info.position.vPos;
	xSize = (fix) (WeaponBlobSize (pObj->info.nId) * F2X (fScale));
	if (xOffset) {
		if (bViewerOffset) {
			CFixVector o = gameData.renderData.mine.viewer.vPos - vPos;
			CFixVector::Normalize (o);
			vPos += o * xOffset;
			}
		else
			vPos += pObj->info.position.mOrient.m.dir.f * xOffset;
		}
	if (xSize < I2X (1))
		xSize = I2X (1);
	color.Alpha () = alpha;
	alpha = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
	color.Red () = pColor->Red () * alpha;
	color.Green () = pColor->Green () * alpha;
	color.Blue () = pColor->Blue () * alpha;
	return transparencyRenderer.AddSprite (corona.Bitmap (), vPos, &color, FixMulDiv (xSize, corona.Bitmap ()->Width (), corona.Bitmap ()->Height ()), 
														xSize, 0, LIGHTTRAIL_BLENDMODE, 3);
	}
RETURN (0)
}

// -----------------------------------------------------------------------------

#if 0
static CFloatVector vTrailOffs [2][4] = {{{{0,0,0}},{{0,-10,-5}},{{0,-10,-50}},{{0,0,-50}}},
											 {{{0,0,0}},{{0,10,-5}},{{0,10,-50}},{{0,0,-50}}}};
#endif

void RenderLightTrail (CObject *pObj)
{
ENTER (1, 0);
	CFloatVector	color, *pColor;
	int32_t				/*nTrailItem = -1, nCoronaItem = -1,*/ bGatling = 0;

if (!SHOW_OBJ_FX)
	return;
if (!pObj->IsProjectile ())
	return;
if (pObj->Velocity ().IsZero ())
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;

bGatling = pObj->IsGatlingRound ();
if (pObj->info.renderType == RT_POLYOBJ)
	pColor = gameData.weaponData.color + pObj->info.nId;
else {
	CRGBColor* pClipColor = AnimationColor (pObj);
	color.Set (pClipColor->r, pClipColor->g, pClipColor->b);
	color /= 255.0f;
	pColor = &color;
	}

if (pObj->HasLightTrail () && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bLightTrails, 0, 0, 0)) {
	if (gameOpts->render.particles.bPlasmaTrails)
		;//DoObjectSmoke (pObj);
	else if (EGI_FLAG (bLightTrails, 1, 1, 0) && (pObj->info.nType == OBJ_WEAPON) &&
				pObj->HasLightTrail () &&
				!pObj->Velocity ().IsZero () &&
				glare.Load ()) {
			CFloatVector	vNorm, vCenter, vOffs, vTrailVerts [8];
			float				h, l, r, dx, dy;

			static CFloatVector vEye = CFloatVector::ZERO;

			static CFloatVector	trailColor = {{{0,0,0,0.33f}}};
			static tTexCoord2f	tcTrail [8] = {
				{{0.0f,0.0f}},{{1.0f,0.0f}},{{1.0f,0.5f}},{{0.0f,0.5f}},
				{{0.0f,0.5f}},{{1.0f,0.5f}},{{1.0f,1.0f}},{{0.0f,1.0f}}
				};
		vCenter.Assign (pObj->info.position.vPos);
		vOffs.Assign (pObj->info.position.mOrient.m.dir.f);
		if (pObj->info.renderType == RT_POLYOBJ) {
			tHitbox*	phb = &gameData.modelData.hitboxes [pObj->ModelId ()].hitboxes [0];
			l = X2F (phb->vMax.v.coord.z - phb->vMin.v.coord.z);
			dx = X2F (phb->vMax.v.coord.x - phb->vMin.v.coord.x);
			dy = X2F (phb->vMax.v.coord.y - phb->vMin.v.coord.y);
			r = float (sqrt (dx * dx + dy * dy)) * ((pObj->info.nId == FUSION_ID) ? 1.5f : 3.0f);
			vCenter += vOffs * (l / 2.0f);
			}
		else {
			r = WeaponBlobSize (pObj->info.nId);
			l = r * 1.5f;
			r *= 2;
			}
		memcpy (&trailColor, pColor, 3 * sizeof (float));
		float fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
		trailColor *= fScale;
		vTrailVerts [0] = vCenter + vOffs * l;
		if (pObj->mType.physInfo.flags & PF_STICK)
			h = 2 * l;
		else {
			h = X2F (CFixVector::Dist (pObj->info.position.vPos, pObj->Origin ()));
			if (h > 50.0f)
				h = 50.0f;
			else if (h < 1.0f)
				h = 1.0f;
			}
		vTrailVerts [7] = vTrailVerts [0] - vOffs * (h + l);
		transformation.Transform (vCenter, vCenter, 0);
		transformation.Transform (vTrailVerts [0], vTrailVerts [0], 0);
		transformation.Transform (vTrailVerts [7], vTrailVerts [7], 0);
		vNorm = CFloatVector::Normal (vTrailVerts [0], vTrailVerts [7], vEye);
		vNorm *= r;
		vTrailVerts [2] = 
		vTrailVerts [5] = vCenter + vNorm;
		vTrailVerts [3] = 
		vTrailVerts [4] = vCenter - vNorm;
		//vNorm /= 4;
		vTrailVerts [6] = vTrailVerts [7] + vNorm;
		vTrailVerts [7] -= vNorm;
		vNorm = CFloatVector::Normal (vTrailVerts [2], vTrailVerts [3], vEye);
		vNorm *= r;
		vTrailVerts [0] = vTrailVerts [3] - vNorm;
		vTrailVerts [1] = vTrailVerts [2] - vNorm;
		transparencyRenderer.AddPoly (NULL, NULL, glare.Bitmap (), vTrailVerts, 8, tcTrail, &trailColor, NULL, 1, 0, GL_QUADS, GL_CLAMP, LIGHTTRAIL_BLENDMODE, -1);
		}
	}

if ((pObj->info.renderType != RT_POLYOBJ) || (pObj->info.nId == FUSION_ID))
	RenderWeaponCorona (pObj, pColor, 0.5f, 0, 2.0f + X2F (Rand (I2X (1) / 8)), 1, 0, 1);
else
	RenderWeaponCorona (pObj, pColor, 0.75f, 0, bGatling ? 1.0f : 2.0f, 0, 0, 0);
LEAVE
}

// -----------------------------------------------------------------------------

void DrawDebrisCorona (CObject *pObj)
{
ENTER (1, 0);
	static	CFloatVector	debrisGlow = {{{0.66f, 0, 0, 1}}};
	static	CFloatVector	markerGlow = {{{0, 0.66f, 0, 1}}};
	static	time_t t0 = 0;

if (pObj->info.nType == OBJ_MARKER)
	RenderWeaponCorona (pObj, &markerGlow, 0.75f, 0, 4, 1, 1, 0);
#if DBG
else if (pObj->info.nType == OBJ_DEBRIS) {
#else
else if ((pObj->info.nType == OBJ_DEBRIS) && gameOpts->render.nDebrisLife) {
#endif
	float	h = (float) nDebrisLife [gameOpts->render.nDebrisLife] - X2F (pObj->info.xLifeLeft);
	if (h < 0)
		h = 0;
	if (h < 10) {
		h = (10 - h) / 20.0f;
		if (gameStates.app.nSDLTicks [0] - t0 > 50) {
			t0 = gameStates.app.nSDLTicks [0];
			debrisGlow.Red () = 0.5f + X2F (Rand (I2X (1) / 4));
			debrisGlow.Green () = X2F (Rand (I2X (1) / 4));
			}
		RenderWeaponCorona (pObj, &debrisGlow, h, 5 * pObj->info.xSize, 1.5f, 1, LIGHTTRAIL_BLENDMODE, 0);
		}
	}
LEAVE
}

//------------------------------------------------------------------------------
//eof
