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
#	include <conf.h>
#endif
#ifdef RCS
static char rcsid [] = "$Id: lighting.c,v 1.4 2003/10/04 03:14:47 btb Exp $";
#endif
#include <math.h>
#include <stdio.h>
#include <string.h>	// for memset ()

#include "inferno.h"
#include "error.h"
#include "u_mem.h"
#include "fix.h"
#include "vecmat.h"
#include "network.h"
#include "ogl_defs.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "gameseg.h"
#include "endlevel.h"
#include "renderthreads.h"
#include "light.h"
#include "lightmap.h"
#include "headlight.h"
#include "dynlight.h"

#define SORT_LIGHTS 0
#define PREFER_GEOMETRY_LIGHTS 1

//------------------------------------------------------------------------------

#ifdef _DEBUG

int CheckUsedLights (void)
{
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [0];

for (int i = MAX_SHADER_LIGHTS; i; i--, activeLightsP++)
	if (activeLightsP->psl && !activeLightsP->psl->activeLightsP) 
		return 1;
return 0;
}

int CheckUsedLights1 (void)
{
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights;

for (int i = gameData.render.lights.dynamic.shader.nLights; i; i--, psl++)
	if (psl->bUsed [0] == 1)
		return 1;
return 0;
}

int CheckUsedLights2 (void)
{
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights;

for (int i = gameData.render.lights.dynamic.shader.nLights; i; i--, psl++)
	if (psl->bUsed [0] == 2)
		return 1;
return 0;
}

#endif

//------------------------------------------------------------------------------

unsigned GetDynLightHandle (void)
{
#if USE_OGL_LIGHTS
	GLint	nMaxLights;
if (gameData.render.lights.dynamic.nLights >= MAX_SEGMENTS)
	return 0xffffffff;
glGetIntegerv (GL_MAX_LIGHTS, &nMaxLights);
if (gameData.render.lights.dynamic.nLights >= nMaxLights)
	return 0xffffffff;
return (unsigned) (GL_LIGHT0 + gameData.render.lights.dynamic.nLights);
#else
return 0xffffffff;
#endif
}

//------------------------------------------------------------------------------

void SetDynLightColor (short nLight, float red, float green, float blue, float fBrightness)
{
	tDynLight	*pl = gameData.render.lights.dynamic.lights + nLight;
	int			i;

if ((pl->info.nType == 1) ? gameOpts->render.color.bGunLight : gameStates.render.bAmbientColor) {
	pl->info.color.red = red;
	pl->info.color.green = green;
	pl->info.color.blue = blue;
	}
else
	pl->info.color.red = 
	pl->info.color.green =
	pl->info.color.blue = 1.0f;
pl->info.color.alpha = 1.0;
pl->info.fBrightness = fBrightness;
pl->info.fRange = (float) sqrt (fBrightness / 2.0f);
pl->fSpecular.v [0] = red;
pl->fSpecular.v [1] = green;
pl->fSpecular.v [2] = blue;
for (i = 0; i < 3; i++) {
#if USE_OGL_LIGHTS
	pl->info.fAmbient.v [i] = pl->info.fDiffuse [i] * 0.01f;
	pl->info.fDiffuse.v [i] = 
#endif
	pl->fEmissive.v [i] = pl->fSpecular.v [i] * fBrightness;
	}
// light alphas
#if USE_OGL_LIGHTS
pl->info.fAmbient.v [3] = 1.0f;
pl->info.fDiffuse.v [3] = 1.0f;
pl->fSpecular.v [3] = 1.0f;
glLightfv (pl->info.handle, GL_AMBIENT, pl->info.fAmbient);
glLightfv (pl->info.handle, GL_DIFFUSE, pl->info.fDiffuse);
glLightfv (pl->info.handle, GL_SPECULAR, pl->fSpecular);
glLightf (pl->info.handle, GL_SPOT_EXPONENT, 0.0f);
#endif
}

// ----------------------------------------------------------------------------------------------

void SetDynLightPos (short nObject)
{
if (SHOW_DYN_LIGHT) {
	int	nLight = gameData.render.lights.dynamic.owners [nObject];
	if (nLight >= 0)
		gameData.render.lights.dynamic.lights [nLight].info.vPos = gameData.objs.objects [nObject].position.vPos;
	}
}

//------------------------------------------------------------------------------

short FindDynLight (short nSegment, short nSide, short nObject)
{
if (gameOpts->render.nLightingMethod) {
		tDynLight	*pl = gameData.render.lights.dynamic.lights;
		short			i;

	if (nObject >= 0)
		return gameData.render.lights.dynamic.owners [nObject];
	if (nSegment >= 0)
		for (i = 0; i < gameData.render.lights.dynamic.nLights; i++, pl++)
			if ((pl->info.nSegment == nSegment) && (pl->info.nSide == nSide))
				return i;
	}
return -1;
}

//------------------------------------------------------------------------------

short UpdateDynLight (tRgbaColorf *pc, float fBrightness, short nSegment, short nSide, short nObject)
{
	short	nLight = FindDynLight (nSegment, nSide, nObject);

if (nLight >= 0) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;
	if (!pc)
		pc = &pl->info.color;
	if (nObject >= 0)
		pl->info.vPos = OBJECTS [nObject].position.vPos;
#if 0
	if (gameStates.render.bPerPixelLighting && (pl->info.nType < 2))
		info.fBrightness /= 2;
#endif
	if ((pl->info.fBrightness != fBrightness) || 
		 (pl->info.color.red != pc->red) || (pl->info.color.green != pc->green) || (pl->info.color.blue != pc->blue)) {
		SetDynLightColor (nLight, pc->red, pc->green, pc->blue, fBrightness);
		}
	}
return nLight;
}

//------------------------------------------------------------------------------

int LastEnabledDynLight (void)
{
	short	i = gameData.render.lights.dynamic.nLights;

while (i)
	if (gameData.render.lights.dynamic.lights [--i].info.bState)
		return i;
return -1;
}

//------------------------------------------------------------------------------

void RefreshDynLight (tDynLight *pl)
{
#if USE_OGL_LIGHTS
glLightfv (pl->info.handle, GL_AMBIENT, pl->info.fAmbient);
glLightfv (pl->info.handle, GL_DIFFUSE, pl->info.fDiffuse);
glLightfv (pl->info.handle, GL_SPECULAR, pl->fSpecular);
glLightf (pl->info.handle, GL_CONSTANT_ATTENUATION, pl->info.fAttenuation [0]);
glLightf (pl->info.handle, GL_LINEAR_ATTENUATION, pl->info.fAttenuation [1]);
glLightf (pl->info.handle, GL_QUADRATIC_ATTENUATION, pl->info.fAttenuation [2]);
#endif
}

//------------------------------------------------------------------------------

void SwapDynLights (tDynLight *pl1, tDynLight *pl2)
{
if (pl1 != pl2) {
		tDynLight	h;
	h = *pl1;
	*pl1 = *pl2;
	*pl2 = h;
#if USE_OGL_LIGHTS
	pl1->handle = (unsigned) (GL_LIGHT0 + (pl1 - gameData.render.lights.dynamic.lights));
	pl2->handle = (unsigned) (GL_LIGHT0 + (pl2 - gameData.render.lights.dynamic.lights));
#endif
	if (pl1->info.nObject >= 0)
		gameData.render.lights.dynamic.owners [pl1->info.nObject] = 
			(short) (pl1 - gameData.render.lights.dynamic.lights);
	if (pl2->info.nObject >= 0)
		gameData.render.lights.dynamic.owners [pl2->info.nObject] = 
			(short) (pl2 - gameData.render.lights.dynamic.lights);
	}
}

//------------------------------------------------------------------------------

int ToggleDynLight (short nSegment, short nSide, short nObject, int bState)
{
	short nLight = FindDynLight (nSegment, nSide, nObject);

if (nLight >= 0) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;
#if 1
	pl->info.bOn = bState;
#else
	if (pl->info.bState != bState) {
		int i = LastEnabledDynLight ();
		if (bState) {
			SwapDynLights (pl, gameData.render.lights.dynamic.lights + i + 1);
			pl = gameData.render.lights.dynamic.lights + i + 1;
#if USE_OGL_LIGHTS
			glEnable (pl->info.handle);
			RefreshDynLight (pl);
#endif
			}
		else {
			SwapDynLights (pl, gameData.render.lights.dynamic.lights + i);
#if USE_OGL_LIGHTS
			RefreshDynLight (pl);
#endif
			pl = gameData.render.lights.dynamic.lights + i;
#if USE_OGL_LIGHTS
			glDisable (pl->info.handle);
#endif
			}
		pl->info.bState = bState;
		}
#endif
	}
return nLight;
}

//------------------------------------------------------------------------------

void RegisterLight (tFaceColor *pc, short nSegment, short nSide)
{
#if 0
if (!pc || pc->index) {
	tLightInfo	*pli = gameData.render.shadows.lightInfo + gameData.render.shadows.nLights++;
#ifdef _DEBUG
	vmsAngVec	a;
#endif
	pli->nIndex = (int) nSegment * 6 + nSide;
	COMPUTE_SIDE_CENTER_I (&pli->pos, nSegment, nSide);
	OOF_VecVms2Gl (pli->glPos, &pli->pos);
	pli->nSegNum = nSegment;
	pli->nSideNum = (ubyte) nSide;
#ifdef _DEBUG
	VmExtractAnglesVector (&a, gameData.segs.segments [nSegment].sides [nSide].normals);
	VmAngles2Matrix (&pli->position.mOrient, &a);
#endif
	}
#endif
}

//-----------------------------------------------------------------------------

static int IsFlickeringLight (short nSegment, short nSide)
{
	tVariableLight	*flP = gameData.render.lights.flicker.lights;
	int					l;
for (l = gameData.render.lights.flicker.nLights; l; l--, flP++)
	if ((flP->nSegment == nSegment) && (flP->nSide == nSide))
		return 1;
return 0;
}

//-----------------------------------------------------------------------------

static int IsDestructibleLight (short nTexture)
{
if (!nTexture)
	return 0;
if (IsMultiGame && netGame.bIndestructibleLights)
	return 0;
short nClip = gameData.pig.tex.pTMapInfo [nTexture].nEffectClip;
tEffectClip	*ecP = (nClip < 0) ? NULL : gameData.eff.pEffects + nClip;
short	nDestBM = ecP ? ecP->nDestBm : -1;
ubyte	bOneShot = ecP ? (ecP->flags & EF_ONE_SHOT) != 0 : 0;
if (nClip == -1) 
	return gameData.pig.tex.pTMapInfo [nTexture].destroyed != -1;
return (nDestBM != -1) && !bOneShot;
}

//------------------------------------------------------------------------------

int AddDynLight (grsFace *faceP, tRgbaColorf *pc, fix xBrightness, short nSegment, 
					  short nSide, short nObject, short nTexture, vmsVector *vPos)
{
	tDynLight	*pl;
	short			h, i;
	float			fBrightness = f2fl (xBrightness);
#if USE_OGL_LIGHTS
	GLint			nMaxLights;
#endif

#if 0
if (xBrightness > F1_0)
	xBrightness = F1_0;
#endif
if (fBrightness <= 0)
	return -1;
#ifdef _DEBUG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nSegment = nSegment;
if ((nDbgObj >= 0) && (nObject == nDbgObj))
	nDbgObj = nDbgObj;
if (pc && ((pc->red > 1) || (pc->green > 1) || (pc->blue > 1)))
	pc = pc;
#endif
if (pc)
	pc->alpha = 1.0f;
if (0 <= (h = UpdateDynLight (pc, fBrightness, nSegment, nSide, nObject)))
	return h;
if (!pc)
	return -1;
if ((pc->red == 0.0f) && (pc->green == 0.0f) && (pc->blue == 0.0f)) {
	if (gameStates.app.bD2XLevel && gameStates.render.bColored)
		return -1;
	pc->red = pc->green = pc->blue = 1.0f;
	}
if (gameData.render.lights.dynamic.nLights >= MAX_OGL_LIGHTS) {
	gameStates.render.bHaveDynLights = 0;
	return -1;	//too many lights
	}
#if USE_OGL_LIGHTS
glGetIntegerv (GL_MAX_LIGHTS, &nMaxLights);
if (gameData.render.lights.dynamic.nLights >= MAX_OGL_LIGHTS) {
	gameStates.render.bHaveDynLights = 0;
	return -1;	//too many lights
	}
#endif
i = gameData.render.lights.dynamic.nLights; //LastEnabledDynLight () + 1;
pl = gameData.render.lights.dynamic.lights + i;
#if USE_OGL_LIGHTS
pl->info.handle = GetDynLightHandle (); 
if (pl->info.handle == 0xffffffff)
	return -1;
#endif
#if 0
if (i < gameData.render.lights.dynamic.nLights)
	SwapDynLights (pl, gameData.render.lights.dynamic.lights + gameData.render.lights.dynamic.nLights);
#endif
pl->info.faceP = faceP;
pl->info.nSegment = nSegment;
pl->info.nSide = nSide;
pl->info.nObject = nObject;
pl->info.nPlayer = -1;
pl->info.bState = 1;
pl->info.bSpot = 0;
pl->info.fBoost = 0;
//0: static light
//2: object/lightning
//3: headlight
if (nObject >= 0) {
	tObject *objP = OBJECTS + nObject;
	//HUDMessage (0, "Adding object light %d, type %d", gameData.render.lights.dynamic.nLights, objP->nType);
	pl->info.nType = 2;
	pl->info.vPos = objP->position.vPos;
	pl->info.fRad = 0; //f2fl (gameData.objs.objects [nObject].size) / 2;
	if (fBrightness > 1) {
		if ((objP->nType == OBJ_FIREBALL) || (objP->nType == OBJ_EXPLOSION)) {
			pl->info.fBoost = 1;
			pl->info.fRad = fBrightness;
			}
		else if ((objP->nType == OBJ_WEAPON) && (objP->id == FLARE_ID)) {
			pl->info.fBoost = 1;
			pl->info.fRad = 2 * fBrightness;
			}
		}
	gameData.render.lights.dynamic.owners [nObject] = gameData.render.lights.dynamic.nLights;
	}
else if (nSegment >= 0) {
#if 0
	vmsVector	vOffs;
	tSide			*sideP = gameData.segs.segments [nSegment].sides + nSide;
#endif
	if (nSide < 0) {
		pl->info.nType = 2;
		pl->info.bVariable = 0;
		pl->info.fRad = 0;
		if (vPos)
			pl->info.vPos = *vPos;
		else
			COMPUTE_SEGMENT_CENTER_I (&pl->info.vPos, nSegment);
		}
	else {
		pl->info.nType = 0;
		pl->info.fRad = faceP ? faceP->fRad : 0;
		//RegisterLight (NULL, nSegment, nSide);
		pl->info.bVariable = IsDestructibleLight (nTexture) || IsFlickeringLight (nSegment, nSide) || WallIsVolatile (SEGMENTS [nSegment].sides [nSide].nWall);
		COMPUTE_SIDE_CENTER_I (&pl->info.vPos, nSegment, nSide);
	#if 1
		if (gameStates.render.bPerPixelLighting) {
			tSide			*sideP = SEGMENTS [nSegment].sides + nSide;
			vmsVector	vOffs;
			VmVecAdd (&vOffs, sideP->normals, sideP->normals + 1);
			VmVecScaleFrac (&vOffs, 1, 4);
			VmVecInc (&pl->info.vPos, &vOffs);
	#	if 0
			fBrightness /= 2;
	#	endif
			}
		}
#endif
	}
else {
	pl->info.nType = 3;
	pl->info.bVariable = 0;
	}
#if 0
PrintLog ("adding light %d,%d\n", 
		  gameData.render.lights.dynamic.nLights, pl - gameData.render.lights.dynamic.lights);
#endif
pl->info.bOn = 1;
pl->bTransform = 1;
SetDynLightColor (gameData.render.lights.dynamic.nLights, pc->red, pc->green, pc->blue, fBrightness);
return gameData.render.lights.dynamic.nLights++;
}

//------------------------------------------------------------------------------

void RemoveDynLightFromList (tDynLight *pl, short nLight)
{
//PrintLog ("removing light %d,%d\n", nLight, pl - gameData.render.lights.dynamic.lights);
// if not removing last light in list, move last light down to the now free list entry
// and keep the freed light handle thus avoiding gaps in used handles
if (nLight < --gameData.render.lights.dynamic.nLights) {
	*pl = gameData.render.lights.dynamic.lights [gameData.render.lights.dynamic.nLights];
	if (pl->info.nObject >= 0)
		gameData.render.lights.dynamic.owners [pl->info.nObject] = nLight;
	if (pl->info.nPlayer < MAX_PLAYERS)
		gameData.render.lights.dynamic.nHeadLights [pl->info.nPlayer] = nLight;
	}
}

//------------------------------------------------------------------------------

void DeleteDynLight (short nLight)
{
if ((nLight >= 0) && (nLight < gameData.render.lights.dynamic.nLights)) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;
#ifdef _DEBUG
	if ((nDbgSeg >= 0) && (nDbgSeg == pl->info.nSegment) && ((nDbgSide < 0) || (nDbgSide == pl->info.nSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (!pl->info.nType) {
		// do not remove static lights, or the nearest lights to segment info will get messed up!
		pl->info.bState = pl->info.bOn = 0;
		return;
		}
	RemoveDynLightFromList (pl, nLight);
	}
}

//------------------------------------------------------------------------------

void DeleteLightningLights (void)
{
	tDynLight	*pl;
	int			nLight;

for (nLight = gameData.render.lights.dynamic.nLights, pl = gameData.render.lights.dynamic.lights + nLight; nLight;) {
	--nLight;
	--pl;
	if ((pl->info.nSegment >= 0) && (pl->info.nSide < 0))
		RemoveDynLightFromList (pl, nLight);
	}
}

//------------------------------------------------------------------------------

int RemoveDynLight (short nSegment, short nSide, short nObject)
{
	int	nLight = FindDynLight (nSegment, nSide, nObject);

if (nLight < 0)
	return 0;
DeleteDynLight (nLight);
if (nObject >= 0)
	gameData.render.lights.dynamic.owners [nObject] = -1;
return 1;
}

//------------------------------------------------------------------------------

void RemoveDynLightningLights (void)
{
	tDynLight	*pl = gameData.render.lights.dynamic.lights;
	
for (short i = 0; i < gameData.render.lights.dynamic.nLights; )
	if ((pl->info.nSegment >= 0) && (pl->info.nSide < 0)) {
		RemoveDynLightFromList (pl, i);
		}
	else {
		i++;
		pl++;
		}
}

//------------------------------------------------------------------------------

void RemoveDynLights (void)
{
for (short i = 0; i < gameData.render.lights.dynamic.nLights; i++)
	DeleteDynLight (i);
}

//------------------------------------------------------------------------------

void SetDynLightMaterial (short nSegment, short nSide, short nObject)
{
	//static float fBlack [4] = {0.0f, 0.0f, 0.0f, 1.0f};
	int nLight = FindDynLight (nSegment, nSide, nObject);

if (nLight >= 0) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;
	if (pl->info.bState) {
		gameData.render.lights.dynamic.material.emissive = *((fVector *) &pl->fEmissive);
		gameData.render.lights.dynamic.material.specular = *((fVector *) &pl->fEmissive);
		gameData.render.lights.dynamic.material.shininess = 8;
		gameData.render.lights.dynamic.material.bValid = 1;
		gameData.render.lights.dynamic.material.nLight = nLight;
		return;
		}
	}
gameData.render.lights.dynamic.material.bValid = 0;
}

//------------------------------------------------------------------------------

static inline int QCmpStaticLights (tDynLight *pl, tDynLight *pm)
{
return (pl->info.bVariable == pm->info.bVariable) ? 0 : pl->info.bVariable ? 1 : -1;
}

//------------------------------------------------------------------------------

void QSortStaticLights (int left, int right)
{
	int	l = left,
			r = right;
			tDynLight m = gameData.render.lights.dynamic.lights [(l + r) / 2];
		
do {
	while (QCmpStaticLights (gameData.render.lights.dynamic.lights + l, &m) < 0)
		l++;
	while (QCmpStaticLights (gameData.render.lights.dynamic.lights + r, &m) > 0)
		r--;
	if (l <= r) {
		if (l < r) {
			tDynLight h = gameData.render.lights.dynamic.lights [l];
			gameData.render.lights.dynamic.lights [l] = gameData.render.lights.dynamic.lights [r];
			gameData.render.lights.dynamic.lights [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortStaticLights (l, right);
if (left < r)
	QSortStaticLights (left, r);
}

//------------------------------------------------------------------------------

void AddDynGeometryLights (void)
{
	int			nFace, nSegment, nSide, nTexture, nLight;
	grsFace		*faceP;
	tFaceColor	*pc;
	short			*pSegLights, *pVertLights, *pOwners;

#if 0
for (nTexture = 0; nTexture < 910; nTexture++)
	nLight = IsLight (nTexture);
#endif
gameStates.render.bHaveDynLights = 1;
gameData.render.fAttScale = gameStates.render.bPerPixelLighting ? 1.0f : 2.0f;
gameStates.ogl.fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightRange];
memset (&gameData.render.lights.dynamic.headLights, 0, sizeof (gameData.render.lights.dynamic.headLights));
//glEnable (GL_LIGHTING);
if (gameOpts->render.nLightingMethod)
	memset (gameData.render.color.vertices, 0, sizeof (*gameData.render.color.vertices) * MAX_VERTICES);
//memset (gameData.render.color.ambient, 0, sizeof (*gameData.render.color.ambient) * MAX_VERTICES);
pSegLights = gameData.render.lights.dynamic.nNearestSegLights;
pVertLights = gameData.render.lights.dynamic.nNearestVertLights;
pOwners = gameData.render.lights.dynamic.owners;
memset (&gameData.render.lights.dynamic, 0xff, sizeof (gameData.render.lights.dynamic));
memset (&gameData.render.lights.dynamic.shader, 0, sizeof (gameData.render.lights.dynamic.shader));
memset (&gameData.render.lights.dynamic.shader.activeLights, 0, sizeof (gameData.render.lights.dynamic.shader.activeLights));
memset (pSegLights, 0xff, sizeof (*pSegLights) * MAX_SEGMENTS * MAX_NEAREST_LIGHTS);
memset (pVertLights, 0xff, sizeof (*pVertLights) * MAX_SEGMENTS * MAX_NEAREST_LIGHTS);
memset (pOwners, 0xff, sizeof (*pOwners) * MAX_OBJECTS);
gameData.render.lights.dynamic.nNearestSegLights = pSegLights;
gameData.render.lights.dynamic.nNearestVertLights = pVertLights;
gameData.render.lights.dynamic.owners = pOwners;
gameData.render.lights.dynamic.nLights = 0;
gameData.render.lights.dynamic.material.bValid = 0;
for (nFace = gameData.segs.nFaces, faceP = gameData.segs.faces.faces; nFace; nFace--, faceP++) {
	nSegment = faceP->nSegment;
	if (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SKYBOX)
		continue;
#ifdef _DEBUG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	nSide = faceP->nSide;
#ifdef _DEBUG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	nTexture = faceP->nBaseTex;
	if ((nTexture < 0) || (nTexture >= MAX_WALL_TEXTURES))
		continue;
	pc = gameData.render.color.textures + nTexture;
	if ((nLight = IsLight (nTexture)))
		AddDynLight (faceP, &pc->color, nLight, (short) nSegment, (short) nSide, -1, nTexture, NULL);
	nTexture = faceP->nOvlTex;
	if ((nTexture > 0) && (nTexture < MAX_WALL_TEXTURES) && (nLight = IsLight (nTexture)) /*gameData.pig.tex.info.fBrightness [nTexture]*/) {
		pc = gameData.render.color.textures + nTexture;
		AddDynLight (faceP, &pc->color, nLight, (short) nSegment, (short) nSide, -1, nTexture, NULL);
		}
	//if (gameData.render.lights.dynamic.nLights)
	//	return;
	if (!gameStates.render.bHaveDynLights) {
		RemoveDynLights ();
		return;
		}
	}
QSortStaticLights (0, gameData.render.lights.dynamic.nLights);
}

//------------------------------------------------------------------------------

void TransformDynLights (int bStatic, int bVariable)
{
	int			i;
	tDynLight	*pl = gameData.render.lights.dynamic.lights;
#if USE_OGL_LIGHTS
OglSetupTransform ();
for (i = 0; i < gameData.render.lights.dynamic.nLights; i++, pl++) {
	if (gameStates.ogl.bUseTransform)
		vPos = pl->info.vPos;
	else
		G3TransformPoint (&vPos, &pl->info.vPos);
	fPos [0] = f2fl (vPos.x);
	fPos [1] = f2fl (vPos.y);
	fPos [2] = f2fl (vPos.z);
	glLightfv (pl->info.handle, GL_POSITION, fPos);
	}
OglResetTransform ();
#else
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights;

gameData.render.lights.dynamic.shader.nLights = 0;
memset (&gameData.render.lights.dynamic.headLights, 0, sizeof (gameData.render.lights.dynamic.headLights));
UpdateOglHeadLight ();
for (i = 0; i < gameData.render.lights.dynamic.nLights; i++, pl++) {
#ifdef _DEBUG
	if ((nDbgSeg >= 0) && (nDbgSeg == pl->info.nSegment) && ((nDbgSide < 0) || (nDbgSide == pl->info.nSide)))
		nDbgSeg = nDbgSeg;
#endif
	psl->info = pl->info;
#ifdef _DEBUG
	if (psl->info.bSpot)
		psl = psl;
#endif
	VmVecFixToFloat (psl->vPosf, &psl->info.vPos);
	if (gameStates.ogl.bUseTransform)
		psl->vPosf [1] = psl->vPosf [0];
	else {
		G3TransformPoint (psl->vPosf + 1, psl->vPosf, 0);
		psl->vPosf [1].p.w = 1;
		}
	psl->vPosf [0].p.w = 1;
	if (psl->info.bSpot)
		SetupHeadLight (pl, psl);
	psl->info.bState = pl->info.bState && (pl->info.color.red + pl->info.color.green + pl->info.color.blue > 0.0);
	psl->bLightning = (pl->info.nObject < 0) && (pl->info.nSide < 0);
	ResetUsedLight (psl, 0);
	if (gameStates.app.bMultiThreaded)
		ResetUsedLight (psl, 1);
	psl->bShadow =
	psl->bExclusive = 0;
	if (psl->info.bState) {
		if (!bStatic && (pl->info.nType == 1) && !pl->info.bVariable)
			psl->info.bState = 0;
		if (!bVariable && ((pl->info.nType > 1) || pl->info.bVariable))
			psl->info.bState = 0;
		}
	gameData.render.lights.dynamic.shader.nLights++;
	psl++;
	}
#	if 0
if (gameData.render.lights.dynamic.shader.nTexHandle)
	OglDeleteTextures (1, &gameData.render.lights.dynamic.shader.nTexHandle);
gameData.render.lights.dynamic.shader.nTexHandle = 0;
OglGenTextures (1, &gameData.render.lights.dynamic.shader.nTexHandle);
glBindTexture (GL_TEXTURE_2D, gameData.render.lights.dynamic.shader.nTexHandle);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
glTexImage2D (GL_TEXTURE_2D, 0, 4, MAX_OGL_LIGHTS / 64, 64, 1, GL_COMPRESSED_RGBA_ARB,
				  GL_FLOAT, gameData.render.lights.dynamic.shader.lights);
#	endif
#endif
#if 0 //HEADLIGHT_TRANSFORMATION == 0
if (gameData.render.lights.dynamic.headLights.nLights && !gameStates.render.automap.bDisplay) {
#if 0
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
#else
	G3StartFrame (0, 0);
#endif
	G3SetViewMatrix (&gameData.objs.viewer->position.vPos, 
						  &gameData.objs.viewer->position.mOrient, 
						  gameStates.render.xZoom, 1);
	}
#endif
}

//------------------------------------------------------------------------------

#if SORT_LIGHTS

static void QSortDynamicLights (int left, int right, int nThread)
{
	tShaderLight	**activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread];
	int				l = left,
						r = right,
						m = activeLightsP [(l + r) / 2]->xDistance;
		
do {
	while (activeLightsP [l]->xDistance < m)
		l++;
	while (activeLightsP [r]->xDistance > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tShaderLight *h = activeLightsP [l];
			activeLightsP [l] = activeLightsP [r];
			activeLightsP [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortDynamicLights (l, right, nThread);
if (left < r)
	QSortDynamicLights (left, r, nThread);
}

#endif //SORT_LIGHTS

//------------------------------------------------------------------------------

static int SetActiveShaderLight (tActiveShaderLight *activeLightsP, tShaderLight *psl, short nType, int nThread)
{
#ifdef _DEBUG
if (((char *) psl - (char *) gameData.render.lights.dynamic.shader.lights) % sizeof (*psl))
	return 0;
#endif
if (psl->bUsed [nThread])
	return 0;
fix xDist = (psl->xDistance / (gameStates.render.bPerPixelLighting ? 2000 : 2000) + 5) / 10;
if (xDist >= MAX_SHADER_LIGHTS)
	return 0;
if (xDist < 0)
	xDist = 0;
#if PREFER_GEOMETRY_LIGHTS
else if (psl->info.nSegment >= 0)
	xDist /= 2;
else
	xDist += (MAX_SHADER_LIGHTS - xDist) / 2;
#endif
#if 1
while (activeLightsP [xDist].nType) {
	if (activeLightsP [xDist].psl == psl)
		return 0;
	if (++xDist >= MAX_SHADER_LIGHTS)
		return 0;
	}
#else
if (activeLightsP [xDist].info.nType) {
	for (int j = xDist; j < MAX_SHADER_LIGHTS - 1; j++) {
		if (!activeLightsP [j].info.nType) {
			memmove (activeLightsP + xDist + 1, activeLightsP + xDist, (j - xDist) * sizeof (tActiveShaderLight));
			xDist = j;
			break;
			}
		else if (activeLightsP [j].psl == psl)
			return 0;
		}
	}
#endif
activeLightsP [xDist].nType = nType;
activeLightsP [xDist].psl = psl;
psl->activeLightsP [nThread] = activeLightsP + xDist;
psl->bUsed [nThread] = (ubyte) nType;

tShaderLightIndex	*sliP = &gameData.render.lights.dynamic.shader.index [0][nThread];

sliP->nActive++;
if (sliP->nFirst > xDist)
	sliP->nFirst = (short) xDist;
if (sliP->nLast < xDist)
	sliP->nLast = (short) xDist;
return 1;
}

//------------------------------------------------------------------------------

tShaderLight *GetActiveShaderLight (tActiveShaderLight *activeLightsP, int nThread)
{
	tShaderLight	*psl = activeLightsP->psl;
#if 0
if (psl) {
	if (psl->bUsed [nThread] > 1)
		psl->bUsed [nThread] = 0;
	if (activeLightsP->nType > 1) {
		activeLightsP->nType = 0;
		activeLightsP->psl = NULL;
		gameData.render.lights.dynamic.shader.index [0][nThread].nActive--;
		}
	}
#endif
if (psl == (tShaderLight *) 0xffffffff)
	return NULL;
return psl;
}

//------------------------------------------------------------------------------

void SetNearestVertexLights (int nFace, int nVertex, vmsVector *vNormalP, ubyte nType, int bStatic, int bVariable, int nThread)
{
//if (gameOpts->render.nLightingMethod) 
	{
	short						*pnl = gameData.render.lights.dynamic.nNearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	tShaderLightIndex		*sliP = &gameData.render.lights.dynamic.shader.index [0][nThread];
	short						i, j, nActiveLightI = sliP->nActive;
	tShaderLight			*psl;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread];
	vmsVector				vVertex = gameData.segs.vertices [nVertex], vLightDir;
	fix						xLightDist, xMaxLightRange = MAX_LIGHT_RANGE * (gameStates.render.bPerPixelLighting + 1);

#ifdef _DEBUG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
	sliP->iVertex = nActiveLightI;
	for (i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
#ifdef _DEBUG
		if (j >= gameData.render.lights.dynamic.nLights)
			break;
#endif
		psl = gameData.render.lights.dynamic.shader.lights + j;
		if (psl->bUsed [nThread])
			continue;
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->info.nSegment == nDbgSeg))
			nDbgSeg = nDbgSeg;
#endif
		if (gameData.threads.vertColor.data.bNoShadow && psl->bShadow)
			continue;
		if (psl->info.bVariable) {
			if (!(bVariable && psl->info.bOn))
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		VmVecSub (&vLightDir, &vVertex, &psl->info.vPos);
		xLightDist = VmVecMag (&vLightDir);
#if 1
		if (vNormalP) {
			vLightDir.p.x = FixDiv (vLightDir.p.x, xLightDist);
			vLightDir.p.y = FixDiv (vLightDir.p.y, xLightDist);
			vLightDir.p.z = FixDiv (vLightDir.p.z, xLightDist);
			if (VmVecDot (vNormalP, &vLightDir) > F1_0 / 2)
				continue;
			}
#endif
#if 0
		psl->xDistance = (fix) (xLightDist / psl->info.fRange) /*- fl2f (psl->info.fRad*/;
#else
		psl->xDistance = (fix) (xLightDist / psl->info.fRange);
		if (psl->info.nSegment >= 0)
			psl->xDistance -= AvgSegRad (psl->info.nSegment);
#endif
		if (psl->xDistance > xMaxLightRange)
			continue;
		if (SetActiveShaderLight (activeLightsP, psl, 2, nThread)) {
			psl->info.nType = nType;
			psl->info.bState = 1;
#ifdef _DEBUG
			psl->nTarget = nFace + 1;
			psl->nFrame = gameData.app.nFrameCount;
#endif
			}
		}
	}
}

//------------------------------------------------------------------------------

int SetNearestFaceLights (grsFace *faceP, int bTextured)
{
	int			i;
	vmsVector	vNormal;
	tSide			*sideP = SEGMENTS [faceP->nSegment].sides + faceP->nSide;

#ifdef _DEBUG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if (faceP - FACES == nDbgFace)
	nDbgFace = nDbgFace;
#endif
#if 1//def RELEASE
if (gameData.render.lights.dynamic.shader.index [0][0].nActive < 0)
	SetNearestSegmentLights (faceP->nSegment, faceP - FACES, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in SetNearestVertexLights ())
else {
#ifdef _DEBUG
	CheckUsedLights2 ();
#endif
	gameData.render.lights.dynamic.shader.index [0][0] = gameData.render.lights.dynamic.shader.index [1][0];
	}
#else
SetNearestSegmentLights (faceP->nSegment, faceP - FACES, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in SetNearestVertexLights ())
#endif
VmVecAdd (&vNormal, sideP->normals, sideP->normals + 1);
VmVecScale (&vNormal, F1_0 / 2);
#if 1
for (i = 0; i < 4; i++)
	SetNearestVertexLights (faceP - FACES, faceP->index [i], &vNormal, 0, 0, 1, 0);
#endif
return gameData.render.lights.dynamic.shader.index [0][0].nActive;
}

//------------------------------------------------------------------------------

void SetNearestStaticLights (int nSegment, int bStatic, ubyte nType, int nThread)
{
	static short nActiveLights [4] = {-1, -1, -1, -1};

if (gameOpts->render.nLightingMethod) {
	short						*pnl = gameData.render.lights.dynamic.nNearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	short						i, j;
	tShaderLight			*psl;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread];

	//gameData.render.lights.dynamic.shader.iStaticLights [nThread] = gameData.render.lights.dynamic.shader.index [0][nThread].nActive;
	for (i = gameStates.render.nMaxLightsPerFace; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
		//gameData.render.lights.dynamic.shader.lights [j].info.nType = nType;
		if (gameData.threads.vertColor.data.bNoShadow && gameData.render.lights.dynamic.shader.lights [j].bShadow)
			continue;
		psl = gameData.render.lights.dynamic.shader.lights + j;
		if (psl->info.bVariable) {
			if (!psl->info.bOn)
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		SetActiveShaderLight (activeLightsP, psl, 3, nThread);
		}
	}
nActiveLights [nThread] = gameData.render.lights.dynamic.shader.index [0][nThread].nActive;
}

//------------------------------------------------------------------------------

void ResetNearestStaticLights (int nSegment, int nThread)
{
if (gameOpts->render.nLightingMethod) {
	short				*pnl = gameData.render.lights.dynamic.nNearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	short				i, j;
	tShaderLight	*psl;

	for (i = gameStates.render.nMaxLightsPerFace; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
		psl = gameData.render.lights.dynamic.shader.lights + j;
		if (psl->bUsed [nThread] == 3)
			ResetUsedLight (psl, nThread);
		}
	}
}

//------------------------------------------------------------------------------

void ResetNearestVertexLights (int nVertex, int nThread)
{
//if (gameOpts->render.nLightingMethod) 
	{
	short				*pnl = gameData.render.lights.dynamic.nNearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	short				i, j;
	tShaderLight	*psl;

#ifdef _DEBUG
	if (nVertex == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	for (i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
		psl = gameData.render.lights.dynamic.shader.lights + j;
		if (psl->bUsed [nThread] == 2)
			ResetUsedLight (psl, nThread);
		}
	}
}

//------------------------------------------------------------------------------

void ResetUsedLight (tShaderLight *psl, int nThread)
{
	tActiveShaderLight *activeLightsP = psl->activeLightsP [nThread];

if (activeLightsP) {
	activeLightsP->psl = NULL;
	activeLightsP->nType = 0;
	psl->activeLightsP [nThread] = NULL;
	}
psl->bUsed [nThread] = 0;
}

//------------------------------------------------------------------------------

void ResetUsedLights (int bVariable, int nThread)
{
	int				i = gameData.render.lights.dynamic.shader.nLights;
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights + i;

for (; i; i--) {
	--psl;
	if (bVariable && (psl->info.nType < 2))
		break;
	ResetUsedLight (psl, nThread);
	}
}

//------------------------------------------------------------------------------

void ResetActiveLights (int nThread, int nActive)
{
	tShaderLightIndex		*sliP = &gameData.render.lights.dynamic.shader.index [0][nThread];
	int						h;

if (0 < (h = sliP->nLast - sliP->nFirst + 1))
	memset (gameData.render.lights.dynamic.shader.activeLights [nThread] + sliP->nFirst, 0, sizeof (tActiveShaderLight) * h);
sliP->nActive = nActive;
sliP->nFirst = MAX_SHADER_LIGHTS;
sliP->nLast = 0;
}

//------------------------------------------------------------------------------

#if PROFILING
time_t tSetNearestDynamicLights = 0;
#endif

short SetNearestSegmentLights (int nSegment, int nFace, int bVariable, int nType, int nThread)
{
	tShaderLightIndex		*sliP = &gameData.render.lights.dynamic.shader.index [0][nThread];
#if PROFILING
	time_t t = clock ();
#endif

#ifdef _DEBUG
	static int nPrevSeg = -1;

if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nDbgSeg = nDbgSeg;
#endif
if (gameOpts->render.nLightingMethod) {
	short						i = gameData.render.lights.dynamic.shader.nLights,
								nLightSeg;
	int						bSkipHeadLight = !gameStates.render.nState && (gameStates.render.bPerPixelLighting || gameOpts->ogl.bHeadLight);
	fix						xMaxLightRange = AvgSegRad (nSegment) + MAX_LIGHT_RANGE * (gameStates.render.bPerPixelLighting + 1);
	tShaderLight			*psl = gameData.render.lights.dynamic.shader.lights + i;
	vmsVector				c;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread];

	COMPUTE_SEGMENT_CENTER_I (&c, nSegment);
	ResetUsedLights (1, nThread);
#ifdef _DEBUG
	if (gameStates.render.bPerPixelLighting) {
		CheckUsedLights ();
		CheckUsedLights1 ();
		}
#endif
	ResetActiveLights (nThread, 0);
	while (i--) {
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->info.nSegment == nDbgSeg))
			psl = psl;
#endif
		psl--;
		if (bSkipHeadLight && (psl->info.nType == 3))
			continue;
		if (psl->info.nType < 2) {
			if (!bVariable)
				break;
			if (!(psl->info.bVariable && psl->info.bOn))
				continue;
			}
		if (gameData.threads.vertColor.data.bNoShadow && psl->bShadow)
			continue;
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->info.nSegment == nDbgSeg))
			psl = psl;
		if ((psl->info.nSegment >= 0) && (psl->info.nSide < 0))
			psl = psl;
#endif
		if (psl->info.nType < 3) {
			nLightSeg = (psl->info.nSegment < 0) ? (psl->info.nObject < 0) ? -1 : gameData.objs.objects [psl->info.nObject].nSegment : psl->info.nSegment;
			if ((nLightSeg < 0) || !SEGVIS (nLightSeg, nSegment)) 
				continue;
			}
#ifdef _DEBUG
		else
			psl = psl;
#endif
		psl->xDistance = (fix) ((VmVecDist (&c, &psl->info.vPos) /*- fl2f (psl->info.fRad)*/) / (psl->info.fRange * psl->info.fBoost));
		if (psl->xDistance > xMaxLightRange)
			continue;
		if (SetActiveShaderLight (activeLightsP, psl, 1, nThread))
#ifdef _DEBUG
			{
			if (nFace < 0)
				psl->nTarget = -nSegment - 1;
			else
				psl->nTarget = nFace + 1;
			psl->nFrame = gameData.app.nFrameCount;
			}
#else
			;
#endif
		}
	gameData.render.lights.dynamic.shader.index [1][nThread] = *sliP;
#ifdef _DEBUG
	if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
		nDbgSeg = nDbgSeg;
#endif
	}
#if PROFILING
tSetNearestDynamicLights += clock () - t;
#endif
#ifdef _DEBUG
nPrevSeg = nSegment;
#endif
return sliP->nActive;
}

//------------------------------------------------------------------------------

short SetNearestPixelLights (int nSegment, vmsVector *vPixelPos, float fLightRad, int nThread)
{
#ifdef _DEBUG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nDbgSeg = nDbgSeg;
#endif
if (gameOpts->render.nLightingMethod) {
	int						nLightSeg;
	short						i = gameData.render.lights.dynamic.shader.nLights;
	fix						xMaxLightRange = fl2f (fLightRad) + MAX_LIGHT_RANGE * (gameStates.render.bPerPixelLighting + 1);
	tShaderLight			*psl = gameData.render.lights.dynamic.shader.lights;
	vmsVector				c;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread];

	ResetActiveLights (nThread, 0);
	ResetUsedLights (0, nThread);
	COMPUTE_SEGMENT_CENTER_I (&c, nSegment);
	for (; i; i--, psl++) {
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->info.nSegment == nDbgSeg))
			psl = psl;
#endif
		if (psl->info.nType)
			break;
		if (psl->info.bVariable)
			continue;
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->info.nSegment == nDbgSeg))
			psl = psl;
#endif
		nLightSeg = psl->info.nSegment;
		if ((nLightSeg < 0) || !SEGVIS (nLightSeg, nSegment)) 
			continue;
		psl->xDistance = (fix) ((VmVecDist (vPixelPos, &psl->info.vPos) /*- fl2f (psl->info.fRad)*/) / psl->info.fRange);
		if (psl->xDistance > xMaxLightRange)
			continue;
		SetActiveShaderLight (activeLightsP, psl, 1, nThread);
		}
	}
return gameData.render.lights.dynamic.shader.index [0][nThread].nActive;
}

//------------------------------------------------------------------------------

int SetNearestAvgSgmLights (short nSegment)
{
	int			i;
	tSegment		*segP = SEGMENTS + nSegment;

#ifdef _DEBUG
if (nSegment == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
SetNearestSegmentLights (nSegment, -1, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in SetNearestVertexLights ())
#if 1
for (i = 0; i < 8; i++)
	SetNearestVertexLights (-1, segP->verts [i], NULL, 0, 1, 1, 0);
#endif
return gameData.render.lights.dynamic.shader.index [0][0].nActive;
}

//------------------------------------------------------------------------------

extern short nDbgSeg;

tFaceColor *AvgSgmColor (int nSegment, vmsVector *vPosP)
{
	tFaceColor	c, *pvc, *psc = gameData.render.color.segments + nSegment;
	short			i, *pv;
	vmsVector	vCenter, vVertex;
	float			d, ds;

#ifdef _DEBUG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
if (!vPosP && (psc->index == gameStates.render.nFrameFlipFlop + 1))
	return psc;
if (SEGMENT2S [nSegment].special == SEGMENT_IS_SKYBOX) {
	psc->color.red = psc->color.green = psc->color.blue = 1.0f;
	psc->index = 1;
	}
else if (gameStates.render.bPerPixelLighting == 2) {
	if (SetNearestAvgSgmLights (nSegment)) {
			tVertColorData	vcd;

		InitVertColorData (vcd);
		vcd.vertNorm.p.x =
		vcd.vertNorm.p.y =
		vcd.vertNorm.p.z = 0;
		if (vPosP)
			VmVecFixToFloat (&vcd.vertPos, vPosP);
		else {
			COMPUTE_SEGMENT_CENTER_I (&vCenter, nSegment);
			VmVecFixToFloat (&vcd.vertPos, &vCenter);
			}
		vcd.pVertPos = &vcd.vertPos;
		vcd.fMatShininess = 4;
		G3AccumVertColor (-1, (fVector3 *) psc, &vcd, 0);
		}
	ResetUsedLights (0, 0);
	gameData.render.lights.dynamic.shader.index [0][0].nActive = -1;
	}
else {
	if (vPosP) {
		COMPUTE_SEGMENT_CENTER_I (&vCenter, nSegment);
		//G3TransformPoint (&vCenter, &vCenter);
		ds = 0.0f;
		}
	else
		ds = 1.0f;
	pv = gameData.segs.segments [nSegment].verts;
	c.color.red = c.color.green = c.color.blue = 0.0f;
	c.index = 0;
	for (i = 0; i < 8; i++, pv++) {
		pvc = gameData.render.color.vertices + *pv;
		if (vPosP) {
			vVertex = gameData.segs.vertices [*pv];
			//G3TransformPoint (&vVertex, &vVertex);
			d = 2.0f - f2fl (VmVecDist (&vVertex, vPosP)) / f2fl (VmVecDist (&vCenter, &vVertex));
			c.color.red += pvc->color.red * d;
			c.color.green += pvc->color.green * d;
			c.color.blue += pvc->color.blue * d;
			ds += d;
			}
		else {
			c.color.red += pvc->color.red;
			c.color.green += pvc->color.green;
			c.color.blue += pvc->color.blue;
			}
		}
	psc->color.red = c.color.red / 8.0f;
	psc->color.green = c.color.green / 8.0f;
	psc->color.blue = c.color.blue / 8.0f;
#if 0
	if (SetNearestSegmentLights (nSegment, 1)) {
		short				nLights = gameData.render.lights.dynamic.shader.nLights;
		tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights + nLights;
		float				fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightfRange];
		float				fLightDist, fAttenuation;
		fVector			vPosf;
		if (vPosP)
			VmVecFixToFloat (&vPosf, vPosP);
		for (i = 0; i < gameData.render.lights.dynamic.shader.nActiveLights; i++) {
			psl = gameData.render.lights.dynamic.shader.activeLights [i];
#if 1
			if (vPosP) {
				vVertex = gameData.segs.vertices [*pv];
				//G3TransformPoint (&vVertex, &vVertex);
				fLightDist = VmVecDist (psl->vPosf, &vPosf) / fLightRange;
				fAttenuation = fLightDist / psl->info.fBrightness;
				VmVecScaleAdd ((fVector *) &c.color, (fVector *) &c.color, (fVector *) &psl->color, 1.0f / fAttenuation);
				}
			else 
#endif
				{
				VmVecInc ((fVector *) &psc->color, (fVector *) &psl->color);
				}
			}
		}
#endif
#if 0
	d = psc->color.red;
	if (d < psc->color.green)
		d = psc->color.green;
	if (d < psc->color.blue)
		d = psc->color.blue;
	if (d > 1.0f) {
		psc->color.red /= d;
		psc->color.green /= d;
		psc->color.blue /= d;
		}
#endif
	}
psc->index = gameStates.render.nFrameFlipFlop + 1;
return psc;
}

//------------------------------------------------------------------------------

void ComputeStaticVertexLights (int nVertex, int nMax, int nThread)
{
	tFaceColor				*pf = gameData.render.color.ambient + nVertex;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread];
	fVector					vVertex;
	int						bColorize = !gameOpts->render.nLightingMethod;

for (; nVertex < nMax; nVertex++, pf++) {
#ifdef _DEBUG
	if (nVertex == nDbgVertex)
		nVertex = nVertex;
#endif
	VmVecFixToFloat (&vVertex, gameData.segs.vertices + nVertex);
	ResetActiveLights (nThread, 0);
	ResetUsedLights (0, nThread);
	SetNearestVertexLights (-1, nVertex, NULL, 1, 1, bColorize, nThread);
	gameData.render.color.vertices [nVertex].index = 0;
	G3VertexColor (&gameData.segs.points [nVertex].p3_normal.vNormal.v3, &vVertex.v3, nVertex, pf, NULL, 1, 0, nThread);
	}
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
extern int nDbgVertex;
#endif

void ComputeStaticDynLighting (int nLevel)
{
if (gameStates.app.bNostalgia)
	return;
if (gameStates.render.bPerPixelLighting && HaveLightMaps ())
	return;
if (gameOpts->render.nLightingMethod || (gameStates.render.bAmbientColor && !gameStates.render.bColored)) {
		int				i, j, bColorize = !gameOpts->render.nLightingMethod;
		tFaceColor		*pfh, *pf = gameData.render.color.ambient;
		tSegment2		*seg2P;

	PrintLog ("Computing static lighting\n");
	gameData.render.vertColor.bDarkness = IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [IsMultiGame].bDarkness;
	gameStates.render.nState = 0;
	TransformDynLights (1, bColorize);
	memset (pf, 0, gameData.segs.nVertices * sizeof (*pf));
	if (!RunRenderThreads (rtStaticVertLight))
		ComputeStaticVertexLights (0, gameData.segs.nVertices, 0);
	pf = gameData.render.color.ambient;
	for (i = 0, seg2P = gameData.segs.segment2s; i < gameData.segs.nSegments; i++, seg2P++) {
		if (seg2P->special == SEGMENT_IS_SKYBOX) {
			short	*sv = SEGMENTS [i].verts;
			for (j = 8; j; j--, sv++) {
				pfh = pf + *sv;
				pfh->color.red =
				pfh->color.green =
				pfh->color.blue = 
				pfh->color.alpha = 1;
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------------------------
//eof
