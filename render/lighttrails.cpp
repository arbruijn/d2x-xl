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

#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

#define LIGHTTRAIL_BLENDMODE	2	// 1:GL_ONE, GL_ONE, 2: GL_ONE, GL_ONE_MINUS_SOURCE_COLOR

// -----------------------------------------------------------------------------

void RenderObjectHalo (CFixVector *vPos, fix xSize, float red, float green, float blue, float alpha, int bCorona)
{
if ((gameOpts->render.coronas.bShots && (bCorona ? LoadCorona () : LoadHalo ()))) {
	tRgbaColorf	c = {red, green, blue, alpha};
	ogl.SetDepthWrite (false);
	CBitmap* bmP = bCorona ? bmpCorona : bmpHalo;
	bmP->SetColor (&c);
	ogl.RenderSprite (bmP, *vPos, xSize, xSize, alpha * 4.0f / 3.0f, LIGHTTRAIL_BLENDMODE, 1);
	ogl.SetDepthWrite (true);
	}
}

// -----------------------------------------------------------------------------

void RenderPowerupCorona (CObject *objP, float red, float green, float blue, float alpha)
{
if ((IsEnergyPowerup (objP->info.nId) ? gameOpts->render.coronas.bPowerups : gameOpts->render.coronas.bWeapons) && LoadGlare ()) {
	static tRgbaColorf keyColors [3] = {
	 {0.2f, 0.2f, 0.9f, 0.2f},
	 {0.9f, 0.2f, 0.2f, 0.2f},
	 {0.9f, 0.8f, 0.2f, 0.2f}
		};

	tRgbaColorf color;
	fix			xSize;
	float			fScale;

	if ((objP->info.nId >= POW_KEY_BLUE) && (objP->info.nId <= POW_KEY_GOLD)) {
		int i = objP->info.nId - POW_KEY_BLUE;

		color = keyColors [(((i < 0) || (i > 2)) ? 3 : i)];
		xSize = I2X (12);
		}
	else {
		float b = (float) sqrt ((red * 3 + green * 5 + blue * 2) / 10);
		color.red = red / b;
		color.green = green / b;
		color.blue = blue / b;
		xSize = 2 * objP->info.xSize; //I2X (8);
		}
	color.alpha = alpha;
	fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
	color.red *= fScale;
	color.green *= fScale;
	color.blue *= fScale;
	bmpGlare->SetColor (&color);
	ogl.RenderSprite (bmpGlare, objP->info.position.vPos, xSize, xSize, alpha, LIGHTTRAIL_BLENDMODE, 5);
	bmpGlare->SetColor (NULL);
	}
}

// -----------------------------------------------------------------------------

void RenderLaserCorona (CObject *objP, tRgbaColorf *colorP, float alpha, float fScale)
{
if (!SHOW_OBJ_FX)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if (gameOpts->render.coronas.bShots && LoadGlare ()) {
	tHitbox*			phb = &gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes [0];
	float				fLength = X2F (phb->vMax [Z] - phb->vMin [Z]) / 2;
	tRgbaColorf		color;

	static CFloatVector	vEye = CFloatVector::ZERO;

	colorP->alpha = alpha;
	float fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
	color = *colorP;
	colorP = &color;
	color.red *= fScale;
	color.green *= fScale;
	color.blue *= fScale;
	bmpGlare->SetColor (colorP);
	ogl.RenderSprite (bmpGlare, objP->info.position.vPos + objP->info.position.mOrient.FVec () * (F2X (fLength - 0.5f)), I2X (1), I2X (1), alpha, LIGHTTRAIL_BLENDMODE, 1);
	bmpGlare->SetColor (NULL);
	}
}

// -----------------------------------------------------------------------------

static inline float WeaponBlobSize (int nId)
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

int RenderWeaponCorona (CObject *objP, tRgbaColorf *colorP, float alpha, fix xOffset,
								float fScale, int bSimple, int bViewerOffset, int bDepthSort)
{
if (!SHOW_OBJ_FX)
	return 0;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return 0;
if ((objP->info.nType == OBJ_WEAPON) && (objP->info.renderType == RT_POLYOBJ))
	RenderLaserCorona (objP, colorP, alpha, fScale);
else if (gameOpts->render.coronas.bShots && LoadCorona ()) {
	fix			xSize;
	tRgbaColorf	color;

	static tTexCoord2f	tcCorona [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

	CFixVector	vPos = objP->info.position.vPos;
	xSize = (fix) (WeaponBlobSize (objP->info.nId) * F2X (fScale));
	if (xOffset) {
		if (bViewerOffset) {
			CFixVector o = gameData.render.mine.viewerEye - vPos;
			CFixVector::Normalize (o);
			vPos += o * xOffset;
			}
		else
			vPos += objP->info.position.mOrient.FVec () * xOffset;
		}
	if (xSize < I2X (1))
		xSize = I2X (1);
	color.alpha = alpha;
	alpha = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
	color.red = colorP->red * alpha;
	color.green = colorP->green * alpha;
	color.blue = colorP->blue * alpha;
	return transparencyRenderer.AddSprite (bmpCorona, vPos, &color, FixMulDiv (xSize, bmpCorona->Width (), bmpCorona->Height ()), xSize, 0, LIGHTTRAIL_BLENDMODE, 3);
	}
return 0;
}

// -----------------------------------------------------------------------------

#if 0
static CFloatVector vTrailOffs [2][4] = {{{{0,0,0}},{{0,-10,-5}},{{0,-10,-50}},{{0,0,-50}}},
											 {{{0,0,0}},{{0,10,-5}},{{0,10,-50}},{{0,0,-50}}}};
#endif

void RenderLightTrail (CObject *objP)
{
	tRgbaColorf		color, *colorP;
	int				nTrailItem = -1, /*nCoronaItem = -1,*/ bGatling = 0; 

if (!SHOW_OBJ_FX)
	return;
if (!gameData.objs.bIsWeapon [objP->info.nId])
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;

bGatling = (objP->info.nId == VULCAN_ID) || (objP->info.nId == GAUSS_ID);
if (objP->info.renderType == RT_POLYOBJ)
	colorP = gameData.weapons.color + objP->info.nId;
else {
	tRgbColorb	*pcb = VClipColor (objP);
	color.red = pcb->red / 255.0f;
	color.green = pcb->green / 255.0f;
	color.blue = pcb->blue / 255.0f;
	colorP = &color;
	}

if (!gameData.objs.bIsSlowWeapon [objP->info.nId] && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bLightTrails, 0, 0, 0)) {
	if (gameOpts->render.particles.bPlasmaTrails)
		;//DoObjectSmoke (objP);
	else if (EGI_FLAG (bLightTrails, 1, 1, 0) && (objP->info.nType == OBJ_WEAPON) &&
				!gameData.objs.bIsSlowWeapon [objP->info.nId] &&
				(!objP->mType.physInfo.velocity.IsZero ()) &&
				LoadGlare ()) {
			CFloatVector	vNorm, vCenter, vOffs, vTrailVerts [8];
			float				h, l, r, dx, dy;

			static CFloatVector vEye = CFloatVector::ZERO;

			static tRgbaColorf	trailColor = {0,0,0,0.33f};
			static tTexCoord2f	tTexCoordTrail [8] = {
				//{{0.1f,0.1f}},{{0.9f,0.1f}},{{0.9f,0.9f}},{{0.1f,0.9f}}
				{{0.0f,0.0f}},{{1.0f,0.0f}},{{1.0f,0.5f}},{{0.0f,0.5f}},
				{{0.0f,0.5f}},{{1.0f,0.5f}},{{1.0f,1.0f}},{{0.0f,1.0f}}
				};

		vCenter.Assign (objP->info.position.vPos);
		vOffs.Assign (objP->info.position.mOrient.FVec ());
		if (objP->info.renderType == RT_POLYOBJ) {
			tHitbox*	phb = &gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes [0];
			l = X2F (phb->vMax [Z] - phb->vMin [Z]);
			dx = X2F (phb->vMax [X] - phb->vMin [X]);
			dy = X2F (phb->vMax [Y] - phb->vMin [Y]);
			r = float (sqrt (dx * dx + dy * dy)) * ((objP->info.nId == FUSION_ID) ? 1.5f : 3.0f);
			vCenter += vOffs * (l / 2.0f);
			}
		else {
			r = WeaponBlobSize (objP->info.nId);
			l = r * 1.5f;
			r *= 2;
			}
		memcpy (&trailColor, colorP, 3 * sizeof (float));
		float fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
		trailColor.red *= fScale;
		trailColor.green *= fScale;
		trailColor.blue *= fScale;
		vTrailVerts [0] = vCenter + vOffs * l;
		h = X2F (CFixVector::Dist (objP->info.position.vPos, objP->Origin ()));
		if (h > 50.0f)
			h = 50.0f;
		else if (h < 1.0f)
			h = 1.0f;
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
		transparencyRenderer.AddPoly (NULL, NULL, bmpGlare, vTrailVerts, 8, tTexCoordTrail, &trailColor, NULL, 1, 0, GL_QUADS, GL_CLAMP, LIGHTTRAIL_BLENDMODE, -1);
		}
	}

if ((objP->info.renderType != RT_POLYOBJ) || (objP->info.nId == FUSION_ID))
	RenderWeaponCorona (objP, colorP, 0.5f, 0, 2.0f + X2F (d_rand() % (I2X (1) / 8)), 1, 0, 1);
else
	RenderWeaponCorona (objP, colorP, 0.75f, 0, bGatling ? 1.0f : 2.0f, 0, 0, 0);
}

// -----------------------------------------------------------------------------

void DrawDebrisCorona (CObject *objP)
{
	static	tRgbaColorf	debrisGlow = {0.66f, 0, 0, 1};
	static	tRgbaColorf	markerGlow = {0, 0.66f, 0, 1};
	static	time_t t0 = 0;

if (objP->info.nType == OBJ_MARKER)
	RenderWeaponCorona (objP, &markerGlow, 0.75f, 0, 4, 1, 1, 0);
#if DBG
else if (objP->info.nType == OBJ_DEBRIS) {
#else
else if ((objP->info.nType == OBJ_DEBRIS) && gameOpts->render.nDebrisLife) {
#endif
	float	h = (float) nDebrisLife [gameOpts->render.nDebrisLife] - X2F (objP->info.xLifeLeft);
	if (h < 0)
		h = 0;
	if (h < 10) {
		h = (10 - h) / 20.0f;
		if (gameStates.app.nSDLTicks - t0 > 50) {
			t0 = gameStates.app.nSDLTicks;
			debrisGlow.red = 0.5f + X2F (d_rand () % (I2X (1) / 4));
			debrisGlow.green = X2F (d_rand () % (I2X (1) / 4));
			}
		RenderWeaponCorona (objP, &debrisGlow, h, 5 * objP->info.xSize, 1.5f, 1, LIGHTTRAIL_BLENDMODE, 0);
		}
	}
}

//------------------------------------------------------------------------------
//eof
