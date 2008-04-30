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

void SetDynLightColor (short nLight, float red, float green, float blue, float brightness)
{
	tDynLight	*pl = gameData.render.lights.dynamic.lights + nLight;
	int			i;

if ((pl->nType == 1) ? gameOpts->render.color.bGunLight : gameOpts->render.color.bAmbientLight) {
	pl->color.red = red;
	pl->color.green = green;
	pl->color.blue = blue;
	}
else
	pl->color.red = 
	pl->color.green =
	pl->color.blue = 1.0f;
pl->brightness = brightness;
pl->range = (float) sqrt (brightness / 2.0f);
pl->fSpecular.v [0] = red;
pl->fSpecular.v [1] = green;
pl->fSpecular.v [2] = blue;
for (i = 0; i < 3; i++) {
#if USE_OGL_LIGHTS
	pl->fAmbient.v [i] = pl->fDiffuse [i] * 0.01f;
	pl->fDiffuse.v [i] = 
#endif
	pl->fEmissive.v [i] = pl->fSpecular.v [i] * brightness;
	}
// light alphas
#if USE_OGL_LIGHTS
pl->fAmbient.v [3] = 1.0f;
pl->fDiffuse.v [3] = 1.0f;
pl->fSpecular.v [3] = 1.0f;
glLightfv (pl->handle, GL_AMBIENT, pl->fAmbient);
glLightfv (pl->handle, GL_DIFFUSE, pl->fDiffuse);
glLightfv (pl->handle, GL_SPECULAR, pl->fSpecular);
glLightf (pl->handle, GL_SPOT_EXPONENT, 0.0f);
#endif
}

// ----------------------------------------------------------------------------------------------

void SetDynLightPos (short nObject)
{
if (SHOW_DYN_LIGHT) {
	int	nLight = gameData.render.lights.dynamic.owners [nObject];
	if (nLight >= 0)
		gameData.render.lights.dynamic.lights [nLight].vPos = gameData.objs.objects [nObject].position.vPos;
	}
}

//------------------------------------------------------------------------------

short FindDynLight (short nSegment, short nSide, short nObject)
{
if (gameOpts->render.bDynLighting) {
		tDynLight	*pl = gameData.render.lights.dynamic.lights;
		short			i;

	if (nObject >= 0)
		return gameData.render.lights.dynamic.owners [nObject];
	if (nSegment >= 0)
		for (i = 0; i < gameData.render.lights.dynamic.nLights; i++, pl++)
			if ((pl->nSegment == nSegment) && (pl->nSide == nSide))
				return i;
	}
return -1;
}

//------------------------------------------------------------------------------

short UpdateDynLight (tRgbaColorf *pc, float brightness, short nSegment, short nSide, short nObject)
{
	short	nLight = FindDynLight (nSegment, nSide, nObject);

if (nLight >= 0) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;
	if (!pc)
		pc = &pl->color;
	if (nObject >= 0)
		pl->vPos = OBJECTS [nObject].position.vPos;
	if ((pl->brightness != brightness) || 
		 (pl->color.red != pc->red) || (pl->color.green != pc->green) || (pl->color.blue != pc->blue)) {
		SetDynLightColor (nLight, pc->red, pc->green, pc->blue, brightness);
		}
	}
return nLight;
}

//------------------------------------------------------------------------------

int LastEnabledDynLight (void)
{
	short	i = gameData.render.lights.dynamic.nLights;

while (i)
	if (gameData.render.lights.dynamic.lights [--i].bState)
		return i;
return -1;
}

//------------------------------------------------------------------------------

void RefreshDynLight (tDynLight *pl)
{
#if USE_OGL_LIGHTS
glLightfv (pl->handle, GL_AMBIENT, pl->fAmbient);
glLightfv (pl->handle, GL_DIFFUSE, pl->fDiffuse);
glLightfv (pl->handle, GL_SPECULAR, pl->fSpecular);
glLightf (pl->handle, GL_CONSTANT_ATTENUATION, pl->fAttenuation [0]);
glLightf (pl->handle, GL_LINEAR_ATTENUATION, pl->fAttenuation [1]);
glLightf (pl->handle, GL_QUADRATIC_ATTENUATION, pl->fAttenuation [2]);
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
	if (pl1->nObject >= 0)
		gameData.render.lights.dynamic.owners [pl1->nObject] = 
			(short) (pl1 - gameData.render.lights.dynamic.lights);
	if (pl2->nObject >= 0)
		gameData.render.lights.dynamic.owners [pl2->nObject] = 
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
	pl->bOn = bState;
#else
	if (pl->bState != bState) {
		int i = LastEnabledDynLight ();
		if (bState) {
			SwapDynLights (pl, gameData.render.lights.dynamic.lights + i + 1);
			pl = gameData.render.lights.dynamic.lights + i + 1;
#if USE_OGL_LIGHTS
			glEnable (pl->handle);
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
			glDisable (pl->handle);
#endif
			}
		pl->bState = bState;
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

int AddDynLight (grsFace *faceP, tRgbaColorf *pc, fix xBrightness, short nSegment, short nSide, short nObject, vmsVector *vPos)
{
	tDynLight	*pl;
	short			h, i;
#if USE_OGL_LIGHTS
	GLint			nMaxLights;
#endif

#if 0
if (xBrightness > F1_0)
	xBrightness = F1_0;
#endif
if (xBrightness <= 0)
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
if (0 <= (h = UpdateDynLight (pc, f2fl (xBrightness), nSegment, nSide, nObject)))
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
pl->handle = GetDynLightHandle (); 
if (pl->handle == 0xffffffff)
	return -1;
#endif
#if 0
if (i < gameData.render.lights.dynamic.nLights)
	SwapDynLights (pl, gameData.render.lights.dynamic.lights + gameData.render.lights.dynamic.nLights);
#endif
pl->faceP = faceP;
pl->nSegment = nSegment;
pl->nSide = nSide;
pl->nObject = nObject;
pl->nPlayer = -1;
pl->bState = 1;
pl->bSpot = 0;
//0: static light
//2: object/lightning
//3: headlight
if (nObject >= 0) {
	pl->nType = 2;
	pl->vPos = OBJECTS [nObject].position.vPos;
	pl->rad = 0; //f2fl (gameData.objs.objects [nObject].size) / 2;
	gameData.render.lights.dynamic.owners [nObject] = gameData.render.lights.dynamic.nLights;
	}
else if (nSegment >= 0) {
#if 0
	vmsVector	vOffs;
	tSide			*sideP = gameData.segs.segments [nSegment].sides + nSide;
#endif
	if (nSide < 0) {
		pl->nType = 2;
		pl->bVariable = 0;
		pl->rad = 0;
		if (vPos)
			pl->vPos = *vPos;
		else
			COMPUTE_SEGMENT_CENTER_I (&pl->vPos, nSegment);
		}
	else {
		int t = gameData.segs.segments [nSegment].sides [nSide].nOvlTex;
		pl->nType = 0;
		pl->rad = faceP ? faceP->rad : 0;
		//RegisterLight (NULL, nSegment, nSide);
		pl->bVariable = IsDestructibleLight (t) || IsFlickeringLight (nSegment, nSide) || IS_WALL (SEGMENTS [nSegment].sides [nSide].nWall);
		COMPUTE_SIDE_CENTER_I (&pl->vPos, nSegment, nSide);
		}
#if 1
	if (gameOpts->ogl.bPerPixelLighting) {
		tSide			*sideP = SEGMENTS [nSegment].sides + nSide;
		vmsVector	vOffs;
		VmVecAdd (&vOffs, sideP->normals, sideP->normals + 1);
		VmVecScaleFrac (&vOffs, 1, 4);
		VmVecInc (&pl->vPos, &vOffs);
		}
#endif
	}
else {
	pl->nType = 3;
	pl->bVariable = 0;
	}
#if USE_OGL_LIGHTS
#	if 0
pl->fAttenuation [0] = 1.0f / f2fl (xBrightness); //0.5f;
pl->fAttenuation [1] = 0.0f; //pl->fAttenuation [0] / 25.0f; //0.01f;
pl->fAttenuation [2] = pl->fAttenuation [0] / 1000.0f; //0.004f;
#	else
pl->fAttenuation [0] = 0.0f;
pl->fAttenuation [1] = 0.0f; //f2fl (xBrightness) / 10.0f;
pl->fAttenuation [2] = (1.0f / f2fl (xBrightness)) / 625.0f;
#	endif
glEnable (pl->handle);
if (!glIsEnabled (pl->handle)) {
	gameStates.render.bHaveDynLights = 0;
	return -1;
	}
glLightf (pl->handle, GL_CONSTANT_ATTENUATION, pl->fAttenuation [0]);
glLightf (pl->handle, GL_LINEAR_ATTENUATION, pl->fAttenuation [1]);
glLightf (pl->handle, GL_QUADRATIC_ATTENUATION, pl->fAttenuation [2]);
#endif
#if 0
PrintLog ("adding light %d,%d\n", 
		  gameData.render.lights.dynamic.nLights, pl - gameData.render.lights.dynamic.lights);
#endif
pl->bOn = 1;
pl->bTransform = 1;
SetDynLightColor (gameData.render.lights.dynamic.nLights, pc->red, pc->green, pc->blue, f2fl (xBrightness));
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
	if (pl->nObject >= 0)
		gameData.render.lights.dynamic.owners [pl->nObject] = nLight;
	if (pl->nPlayer < MAX_PLAYERS)
		gameData.render.lights.dynamic.nHeadLights [pl->nPlayer] = nLight;
	}
}

//------------------------------------------------------------------------------

void DeleteDynLight (short nLight)
{
if ((nLight >= 0) && (nLight < gameData.render.lights.dynamic.nLights)) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;
#ifdef _DEBUG
	if ((nDbgSeg >= 0) && (nDbgSeg == pl->nSegment) && ((nDbgSide < 0) || (nDbgSide == pl->nSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (!pl->nType) {
		// do not remove static lights, or the nearest lights to segment info will get messed up!
		pl->bState = pl->bOn = 0;
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
	if ((pl->nSegment >= 0) && (pl->nSide < 0))
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
	if ((pl->nSegment >= 0) && (pl->nSide < 0)) {
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
	if (pl->bState) {
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
return (pl->bVariable == pm->bVariable) ? 0 : pl->bVariable ? 1 : -1;
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
	int			nFace, nSegment, nSide, t, nLight;
	grsFace		*faceP;
	tFaceColor	*pc;
	short			*pSegLights, *pVertLights, *pOwners;

#if 0
for (t = 0; t < 910; t++)
	nLight = IsLight (t);
#endif
gameStates.render.bHaveDynLights = 1;
//glEnable (GL_LIGHTING);
if (gameOpts->render.bDynLighting)
	memset (gameData.render.color.vertices, 0, sizeof (*gameData.render.color.vertices) * MAX_VERTICES);
//memset (gameData.render.color.ambient, 0, sizeof (*gameData.render.color.ambient) * MAX_VERTICES);
pSegLights = gameData.render.lights.dynamic.nNearestSegLights;
pVertLights = gameData.render.lights.dynamic.nNearestVertLights;
pOwners = gameData.render.lights.dynamic.owners;
memset (&gameData.render.lights.dynamic, 0xff, sizeof (gameData.render.lights.dynamic));
memset (&gameData.render.lights.dynamic, 0xff, sizeof (gameData.render.lights.dynamic));
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
	t = faceP->nBaseTex;
	if (t >= MAX_WALL_TEXTURES) 
		continue;
	pc = gameData.render.color.textures + t;
	if ((nLight = IsLight (t)))
		AddDynLight (faceP, &pc->color, nLight, (short) nSegment, (short) nSide, -1, NULL);
	t = faceP->nOvlTex;
	if ((t > 0) && (t < MAX_WALL_TEXTURES) && (nLight = IsLight (t)) /*gameData.pig.tex.brightness [t]*/) {
		pc = gameData.render.color.textures + t;
		AddDynLight (faceP, &pc->color, nLight, (short) nSegment, (short) nSide, -1, NULL);
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
		vPos = pl->vPos;
	else
		G3TransformPoint (&vPos, &pl->vPos);
	fPos [0] = f2fl (vPos.x);
	fPos [1] = f2fl (vPos.y);
	fPos [2] = f2fl (vPos.z);
	glLightfv (pl->handle, GL_POSITION, fPos);
	}
OglResetTransform ();
#else
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights;

gameData.render.lights.dynamic.shader.nLights = 0;
memset (&gameData.render.lights.dynamic.headLights, 0, sizeof (gameData.render.lights.dynamic.headLights));
UpdateOglHeadLight ();
for (i = 0; i < gameData.render.lights.dynamic.nLights; i++, pl++) {
#ifdef _DEBUG
	if ((nDbgSeg >= 0) && (nDbgSeg == pl->nSegment) && ((nDbgSide < 0) || (nDbgSide == pl->nSide)))
		nDbgSeg = nDbgSeg;
#endif
	psl->faceP = pl->faceP;
	memcpy (&psl->color, &pl->color, sizeof (pl->color));
	psl->color.c.a = 1.0f;
	psl->vPos = pl->vPos;
	VmVecFixToFloat (psl->pos, &pl->vPos);
	if (gameStates.ogl.bUseTransform)
		psl->pos [1] = psl->pos [0];
	else {
		G3TransformPoint (psl->pos + 1, psl->pos, 0);
		psl->pos [1].p.w = 1;
		}
	psl->pos [0].p.w = 1;
	psl->brightness = pl->brightness;
	psl->range = pl->range;
	if ((psl->bSpot = pl->bSpot))
		SetupHeadLight (pl, psl);
	psl->bState = pl->bState && (pl->color.red + pl->color.green + pl->color.blue > 0.0);
	psl->bVariable = pl->bVariable;
	psl->bOn = pl->bOn;
	psl->nType = pl->nType;
	psl->nSegment = pl->nSegment;
	psl->nObject = pl->nObject;
	psl->bLightning = (pl->nObject < 0) && (pl->nSide < 0);
	psl->bShadow =
	psl->bExclusive = 0;
	if (psl->bState) {
		if (!bStatic && (pl->nType == 1) && !pl->bVariable)
			psl->bState = 0;
		if (!bVariable && ((pl->nType > 1) || pl->bVariable))
			psl->bState = 0;
		}
	psl->rad = pl->rad;
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
fix xDist = (psl->xDistance / 10000 + 5) / 10;
if (xDist >= MAX_SHADER_LIGHTS)
	return 0;
if (xDist < 0)
	xDist = 0;
#if PREFER_GEOMETRY_LIGHTS
else if (psl->nSegment >= 0)
	xDist /= 2;
#endif
#if 1
while (activeLightsP [xDist].nType) {
	if (activeLightsP [xDist].psl == psl)
		return 0;
	if (++xDist >= MAX_SHADER_LIGHTS)
		return 0;
	}
#else
if (activeLightsP [xDist].nType) {
	for (int j = xDist; j < MAX_SHADER_LIGHTS - 1; j++) {
		if (!activeLightsP [j].nType) {
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
gameData.render.lights.dynamic.shader.nActiveLights [nThread]++;
if (gameData.render.lights.dynamic.shader.nFirstLight [nThread] > xDist)
	gameData.render.lights.dynamic.shader.nFirstLight [nThread] = (short) xDist;
if (gameData.render.lights.dynamic.shader.nLastLight [nThread] < xDist)
	gameData.render.lights.dynamic.shader.nLastLight [nThread] = (short) xDist;
return 1;
}

//------------------------------------------------------------------------------

tShaderLight *GetActiveShaderLight (tActiveShaderLight *activeLightsP, int nThread)
{
	tShaderLight	*psl = activeLightsP->psl;

if (psl && (activeLightsP->nType > 1)) {
	activeLightsP->nType = 0;
	activeLightsP->psl = NULL;
	gameData.render.lights.dynamic.shader.nActiveLights [nThread]--;
	}
if (psl == (tShaderLight *) 0xffffffff)
	return NULL;
return psl;
}

//------------------------------------------------------------------------------

void SetNearestVertexLights (int nVertex, vmsVector *vNormalP, ubyte nType, int bStatic, int bVariable, int nThread)
{
//if (gameOpts->render.bDynLighting) 
	{
	short						*pnl = gameData.render.lights.dynamic.nNearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	short						i, j, nActiveLightI = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
	tShaderLight			*psl;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread];
	vmsVector				vVertex = gameData.segs.vertices [nVertex], vLightDir;
	fix						xLightDist, xMaxLightRange = MAX_LIGHT_RANGE;// * (gameOpts->ogl.bPerPixelLighting * 3 + 1);

#ifdef _DEBUG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
	gameData.render.lights.dynamic.shader.iVertexLights [nThread] = nActiveLightI;
	for (i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
#ifdef _DEBUG
		if (j >= gameData.render.lights.dynamic.nLights)
			break;
#endif
		psl = gameData.render.lights.dynamic.shader.lights + j;
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->nSegment == nDbgSeg))
			nDbgSeg = nDbgSeg;
#endif
		if (gameData.threads.vertColor.data.bNoShadow && psl->bShadow)
			continue;
		if (psl->bVariable) {
			if (!(bVariable && psl->bOn))
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		VmVecSub (&vLightDir, &vVertex, &psl->vPos);
		xLightDist = VmVecMag (&vLightDir);
		if (vNormalP) {
			vLightDir.p.x /= xLightDist;
			vLightDir.p.y /= xLightDist;
			vLightDir.p.z /= xLightDist;
			if (VmVecDot (vNormalP, &vLightDir) < 0)
				continue;
			}
		psl->xDistance = (fix) (xLightDist / psl->range);
		if (psl->nSegment >= 0)
			psl->xDistance -= AvgSegRad (psl->nSegment);
		if (psl->xDistance > xMaxLightRange)
			continue;
		if (SetActiveShaderLight (activeLightsP, psl, 2, nThread)) {
			psl->nType = nType;
			psl->bState = 1;
			}
		}
	}
}

//------------------------------------------------------------------------------

int SetNearestFaceLights (grsFace *faceP, int bTextured)
{
#if 0
	static int nFrameFlipFlop = -1;
	static short nLastSeg = -1;
	static short nLastSide = -1;

if ((nFrameFlipFlop == gameStates.render.nFrameFlipFlop + 1) && (nLastSeg == faceP->nSegment) && (nLastSide == faceP->nSide))
	return gameData.render.lights.dynamic.shader.nActiveLights [0];
nFrameFlipFlop = gameStates.render.nFrameFlipFlop + 1;
nLastSeg = faceP->nSegment;
nLastSide = faceP->nSide;
#endif

	int			i;
	vmsVector	vNormal;
	tSide			*sideP = SEGMENTS [faceP->nSegment].sides + faceP->nSide;

#ifdef _DEBUG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (gameData.render.lights.dynamic.shader.nActiveLights [0] < 0)
	SetNearestSegmentLights (faceP->nSegment, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in SetNearestVertexLights ())
VmVecAdd (&vNormal, sideP->normals, sideP->normals + 1);
VmVecScale (&vNormal, F1_0 / 2);
for (i = 0; i < 4; i++)
	SetNearestVertexLights (faceP->index [i], &vNormal, 0, 0, 1, 0);
#if SORT_LIGHTS
if (gameData.render.lights.dynamic.shader.nActiveLights [0]) {
	if (gameData.render.lights.dynamic.shader.nActiveLights [0] > MAX_LIGHTS_PER_PIXEL) {
#ifdef _DEBUG
		gameData.render.lights.dynamic.shader.nActiveLights [0] = iVertexLights;
		for (i = 0; i < 4; i++)
			SetNearestVertexLights (faceP->index [i], &vNormal, 0, 0, 1, 0);
#endif
		QSortDynamicLights (0, gameData.render.lights.dynamic.shader.nActiveLights [0] - 1, 0);
		gameData.render.lights.dynamic.shader.nActiveLights [0] = MAX_LIGHTS_PER_PIXEL;
		}
	}
#elif 0
if (gameData.render.lights.dynamic.shader.nActiveLights [0] > MAX_LIGHTS_PER_PIXEL)
	gameData.render.lights.dynamic.shader.nActiveLights [0] = MAX_LIGHTS_PER_PIXEL;
#endif
return gameData.render.lights.dynamic.shader.nActiveLights [0];
}

//------------------------------------------------------------------------------

void SetNearestStaticLights (int nSegment, int bStatic, ubyte nType, int nThread)
{
	static short nActiveLights [4] = {-1, -1, -1, -1};
#if 0
	static int nFrameFlipFlop = -1;
	static int nLastSeg [4] = {-1, -1, -1, -1};
	static ubyte nLastType [4] = {255, 255, 255, 255};
#endif
	int	nMaxLights = gameOpts->ogl.bObjLighting ? 8 : gameOpts->ogl.nMaxLights;

#if 0
if ((nFrameFlipFlop == gameStates.render.nFrameFlipFlop + 1) && (nLastSeg [nThread] == nSegment) && (nLastType [nThread] == nType)) {
	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = nActiveLights [nThread];
	return;
	}
nFrameFlipFlop = gameStates.render.nFrameFlipFlop + 1;
nLastSeg [nThread] = nSegment;
nLastType [nThread] = nType;
#endif
if (gameOpts->render.bDynLighting) {
	short						*pnl = gameData.render.lights.dynamic.nNearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	short						i, j;
	tShaderLight			*psl;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread];

	//gameData.render.lights.dynamic.shader.iStaticLights [nThread] = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
	for (i = nMaxLights; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
		//gameData.render.lights.dynamic.shader.lights [j].nType = nType;
		if (gameData.threads.vertColor.data.bNoShadow && gameData.render.lights.dynamic.shader.lights [j].bShadow)
			continue;
		psl = gameData.render.lights.dynamic.shader.lights + j;
		if (psl->bVariable) {
			if (!psl->bOn)
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		SetActiveShaderLight (activeLightsP, psl, 3, nThread);
		}
	}
nActiveLights [nThread] = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
}

//------------------------------------------------------------------------------

#if PROFILING
time_t tSetNearestDynamicLights = 0;
#endif

short SetNearestSegmentLights (int nSegment, int bVariable, int nType, int nThread)
{
#if CACHE_LIGHTS
	static int nLastSeg [4] = {-1, -1, -1, -1};
	static int nActiveLights [4] = {-1, -1, -1, -1};
	static int nFrameFlipFlop = 0;
#endif
#if SORT_LIGHTS
	int	nMaxLights = (nType && gameOpts->ogl.bObjLighting) ? 8 : gameOpts->ogl.nMaxLights;
#endif
#if PROFILING
	time_t t = clock ();
#endif

#if CACHE_LIGHTS
if ((nFrameFlipFlop == gameStates.render.nFrameFlipFlop + 1) && (nLastSeg [nThread] == nSegment))
	return gameData.render.lights.dynamic.shader.nActiveLights [nThread] = nActiveLights [nThread];
nFrameFlipFlop = gameStates.render.nFrameFlipFlop + 1;
nLastSeg [nThread] = nSegment;
#endif
#ifdef _DEBUG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nDbgSeg = nDbgSeg;
#endif
if (gameOpts->render.bDynLighting) {
	short						h, i = gameData.render.lights.dynamic.shader.nLights,
								nLightSeg;
	fix						xMaxLightRange = AvgSegRad (nSegment) + MAX_LIGHT_RANGE;// * (gameOpts->ogl.bPerPixelLighting * 3 + 1);
	tShaderLight			*psl = gameData.render.lights.dynamic.shader.lights + i;
	vmsVector				c;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread];

	h = gameData.render.lights.dynamic.shader.nLastLight [nThread] - gameData.render.lights.dynamic.shader.nFirstLight [nThread] + 1;
	if (h > 0)
		memset (activeLightsP + gameData.render.lights.dynamic.shader.nFirstLight [nThread], 0, sizeof (tActiveShaderLight) * h);
	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = 0;
	gameData.render.lights.dynamic.shader.nFirstLight [nThread] = MAX_SHADER_LIGHTS;
	gameData.render.lights.dynamic.shader.nLastLight [nThread] = 0;
	COMPUTE_SEGMENT_CENTER_I (&c, nSegment);
	while (i--) {
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->nSegment == nDbgSeg))
			psl = psl;
#endif
		if ((--psl)->nType < 2) {
			if (!bVariable)
				break;
			if (!(psl->bVariable && psl->bOn))
				continue;
			}
		if (gameData.threads.vertColor.data.bNoShadow && psl->bShadow)
			continue;
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->nSegment == nDbgSeg))
			psl = psl;
#endif
		if (psl->nType < 3) {
			nLightSeg = (psl->nSegment < 0) ? (psl->nObject < 0) ? -1 : gameData.objs.objects [psl->nObject].nSegment : psl->nSegment;
			if ((nLightSeg < 0) || !SEGVIS (nLightSeg, nSegment)) 
				continue;
			psl->xDistance = (fix) (VmVecDist (&c, &psl->vPos) / psl->range);
			if (psl->xDistance > xMaxLightRange)
				continue;
			}
		SetActiveShaderLight (activeLightsP, psl, 1, nThread);
		}
	}
#if SORT_LIGHTS
if (nType && (gameData.render.lights.dynamic.shader.nActiveLights [nThread] > nMaxLights)) {
	QSortDynamicLights (0, gameData.render.lights.dynamic.shader.nActiveLights [nThread] - 1, nThread);
	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = nMaxLights;
	}
#endif
#if PROFILING
tSetNearestDynamicLights += clock () - t;
#endif
#if CACHE_LIGHTS
return nActiveLights [nThread] = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
#else
return gameData.render.lights.dynamic.shader.nActiveLights [nThread];
#endif
}

//------------------------------------------------------------------------------

short SetNearestPixelLights (int nSegment, vmsVector *vPixelPos, float fLightRad, int nThread)
{
#ifdef _DEBUG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nDbgSeg = nDbgSeg;
#endif
if (gameOpts->render.bDynLighting) {
	int						nLightSeg;
	short						h, i = gameData.render.lights.dynamic.shader.nLights;
	fix						xMaxLightRange = fl2f (fLightRad) + MAX_LIGHT_RANGE * (gameOpts->ogl.bPerPixelLighting + 1);
	tShaderLight			*psl = gameData.render.lights.dynamic.shader.lights;
	vmsVector				c;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread];

	h = gameData.render.lights.dynamic.shader.nLastLight [nThread] - gameData.render.lights.dynamic.shader.nFirstLight [nThread] + 1;
	if (h > 0)
		memset (activeLightsP + gameData.render.lights.dynamic.shader.nFirstLight [nThread], 0, sizeof (tActiveShaderLight) * h);
	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = 0;
	gameData.render.lights.dynamic.shader.nFirstLight [nThread] = MAX_SHADER_LIGHTS;
	gameData.render.lights.dynamic.shader.nLastLight [nThread] = 0;
	COMPUTE_SEGMENT_CENTER_I (&c, nSegment);
	for (; i; i--, psl++) {
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->nSegment == nDbgSeg))
			psl = psl;
#endif
		if (psl->nType)
			break;
		if (psl->bVariable)
			continue;
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->nSegment == nDbgSeg))
			psl = psl;
#endif
		nLightSeg = psl->nSegment;
		if ((nLightSeg < 0) || !SEGVIS (nLightSeg, nSegment)) 
			continue;
		psl->xDistance = (fix) (VmVecDist (vPixelPos, &psl->vPos) / psl->range);
		if (psl->xDistance > xMaxLightRange)
			continue;
		SetActiveShaderLight (activeLightsP, psl, 1, nThread);
		}
	}
return gameData.render.lights.dynamic.shader.nActiveLights [nThread];
}

//------------------------------------------------------------------------------

extern short nDbgSeg;

tFaceColor *AvgSgmColor (int nSegment, vmsVector *pvPos)
{
	tFaceColor	c, *pvc, *psc = gameData.render.color.segments + nSegment;
	short			i, *pv;
	vmsVector	vCenter, vVertex;
	float			d, ds;

#ifdef _DEBUG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
if (psc->index == gameStates.render.nFrameFlipFlop + 1)
	return psc;
if (SEGMENT2S [nSegment].special == SEGMENT_IS_SKYBOX) {
	psc->color.red = psc->color.green = psc->color.blue = 1.0f;
	psc->index = 1;
	}
else {
	if (pvPos) {
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
		if (pvPos) {
			vVertex = gameData.segs.vertices [*pv];
			//G3TransformPoint (&vVertex, &vVertex);
			d = 2.0f - f2fl (VmVecDist (&vVertex, pvPos)) / f2fl (VmVecDist (&vCenter, &vVertex));
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
		float				fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightRange];
		float				fLightDist, fAttenuation;
		fVector			vPosf;
		if (pvPos)
			VmVecFixToFloat (&vPosf, pvPos);
		for (i = 0; i < gameData.render.lights.dynamic.shader.nActiveLights; i++) {
			psl = gameData.render.lights.dynamic.shader.activeLights [i];
#if 1
			if (pvPos) {
				vVertex = gameData.segs.vertices [*pv];
				//G3TransformPoint (&vVertex, &vVertex);
				fLightDist = VmVecDist (psl->pos, &vPosf) / fLightRange;
				fAttenuation = fLightDist / psl->brightness;
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
	tFaceColor	*pf = gameData.render.color.ambient + nVertex;
	fVector		vVertex;
	int			bColorize = !gameOpts->render.bDynLighting;

for (; nVertex < nMax; nVertex++, pf++) {
#ifdef _DEBUG
	if (nVertex == nDbgVertex)
		nVertex = nVertex;
#endif
	VmVecFixToFloat (&vVertex, gameData.segs.vertices + nVertex);
	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = 0;
	gameData.render.lights.dynamic.shader.nFirstLight [nThread] = MAX_SHADER_LIGHTS;
	gameData.render.lights.dynamic.shader.nLastLight [nThread] = 0;
	SetNearestVertexLights (nVertex, NULL, 1, 1, bColorize, nThread);
	gameData.render.color.vertices [nVertex].index = 0;
	G3VertexColor (&gameData.segs.points [nVertex].p3_normal.vNormal.v3, &vVertex.v3, nVertex, pf, NULL, 1, 0, nThread);
	}
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
extern int nDbgVertex;
#endif

void ComputeStaticDynLighting (void)
{
gameStates.ogl.fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightRange];
if (gameOpts->ogl.bPerPixelLighting) {
	CreateLightMaps ();
	if (!HaveLightMaps ())
		RestoreLights (0);
	}
memset (&gameData.render.lights.dynamic.headLights, 0, sizeof (gameData.render.lights.dynamic.headLights));
if (gameStates.app.bNostalgia)
	return;
if (gameOpts->render.bDynLighting || (gameOpts->render.color.bAmbientLight && !gameStates.render.bColored)) {
		int				i, j, bColorize = !gameOpts->render.bDynLighting;
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

//------------------------------------------------------------------------------

void CalcDynLightAttenuation (vmsVector *pv)
{
#if !USE_OGL_LIGHTS
	int				i;
	tDynLight		*pl = gameData.render.lights.dynamic.lights;
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights;
	fVector			v, d;
	float				l;

v.p.x = f2fl (pv->p.x);
v.p.y = f2fl (pv->p.y);
v.p.z = f2fl (pv->p.z);
if (!gameStates.ogl.bUseTransform)
	G3TransformPoint (&v, &v, 0);
for (i = gameData.render.lights.dynamic.nLights; i; i--, pl++, psl++) {
	d.p.x = v.p.x - psl->pos [1].p.x;
	d.p.y = v.p.y - psl->pos [1].p.y;
	d.p.z = v.p.z - psl->pos [1].p.z;
	l = (float) (sqrt (d.p.x * d.p.x + d.p.y * d.p.y + d.p.z * d.p.z) / 625.0);
	psl->brightness = l / pl->brightness;
	}
#endif
}

// ----------------------------------------------------------------------------------------------
// per pixel lighting, no lightmaps
// 2 - 8 light sources

char *pszPPXLightingFS [] = {
	"#define LIGHTS 8\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (vec4 (1.0, 1.0, 1.0, 1.0), color);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			vec3 NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;}\r\n" \
	"	}"
	};

// ----------------------------------------------------------------------------------------------
// one light source

char *pszPP1LightingFS [] = {
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (vec4 (1.0, 1.0, 1.0, 1.0), color);\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		vec3 NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;}\r\n" \
	"	}"
	};

// ----------------------------------------------------------------------------------------------

char *pszPPLightingVS [] = {
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	};
	
//-------------------------------------------------------------------------

char *pszLightingFS [] = {
	"void main() {\r\n" \
	"gl_FragColor = gl_Color;\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex;\r\n" \
	"void main() {\r\n" \
	"	gl_FragColor = texture2D (baseTex, gl_TexCoord [0].xy) * gl_Color;\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"void main() {\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	gl_FragColor = texColor * gl_Color;\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	gl_FragColor = texColor * gl_Color;}\r\n" \
	"	}"
	};


char *pszLightingVS [] = {
	"void main() {\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	};
	
// ----------------------------------------------------------------------------------------------

char *pszPPLMXLightingFS [] = {
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (vec4 (1.0, 1.0, 1.0, 1.0), color);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex, decalTex, maskTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [3].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			vec3 NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;}\r\n" \
	"	}"
	};


// ----------------------------------------------------------------------------------------------

char *pszPPLM1LightingFS [] = {
	"uniform sampler2D lMapTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [0].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [0].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [0].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (vec4 (1.0, 1.0, 1.0, 1.0), color);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [0].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [0].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [0].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [0].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [0].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [0].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex, maskTex;\r\n" \
	"uniform float bStaticColor;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [3].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [0].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [0].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"		vec3 NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [0].specular * pow (NdotHV, 2.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;}\r\n" \
	"	}"
	};

// ----------------------------------------------------------------------------------------------

char *pszPPLMLightingVS [] = {
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
	"	gl_TexCoord [3] = gl_MultiTexCoord3;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	};
	
//-------------------------------------------------------------------------

char *pszLMLightingFS [] = {
	"uniform sampler2D lMapTex;\r\n" \
	"void main() {\r\n" \
	"gl_FragColor = color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex, maskTex;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [3].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	gl_FragColor = texColor * color /*min (texColor, texColor * color)*/;}\r\n" \
	"	}"
	};


char *pszLMLightingVS [] = {
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
	"	gl_TexCoord [3] = gl_MultiTexCoord3;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	};
	
//-------------------------------------------------------------------------

char *BuildLightingShader (char *pszTemplate, int nLights)
{
	int	l = (int) strlen (pszTemplate) + 1;
	char	*pszFS, szLights [2];

if (!(pszFS = (char *) D2_ALLOC (l)))
	return NULL;
if (nLights > MAX_LIGHTS_PER_PIXEL)
	nLights = MAX_LIGHTS_PER_PIXEL;
memcpy (pszFS, pszTemplate, l);
if (strstr (pszFS, "#define LIGHTS ") == pszFS) {
	sprintf (szLights, "%d", nLights);
	pszFS [15] = *szLights;
	}
#ifdef _DEBUG
PrintLog (" \n\nShader program:\n");
PrintLog (pszFS);
PrintLog (" \n");
#endif
return pszFS;
}

//-------------------------------------------------------------------------

GLhandleARB perPixelLightingShaderProgs [][4] = 
	{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};

GLhandleARB ppLvs [][4] = 
	{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}; 

GLhandleARB ppLfs [][4] = 
	{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}; 

int InitPerPixelLightingShader (int nType, int nLights)
{
	int	i, j, bOk;
	char	*pszFS, *pszVS;
	char	**fsP, **vsP;

#ifdef RELEASE
gameStates.ogl.bPerPixelLightingOk =
gameOpts->ogl.bPerPixelLighting = 0;
#endif
if (!gameOpts->ogl.bPerPixelLighting)
	return 0;
i = 0;
if (perPixelLightingShaderProgs [i][nType])
	return nLights;
if (HaveLightMaps ()) {
	if (nLights) {
		fsP = (nLights == 1) ? pszPPLM1LightingFS : pszPPLMXLightingFS;
		vsP = pszPPLMLightingVS;
		//nLights = MAX_LIGHTS_PER_PIXEL;
		}
	else {
		fsP = pszLMLightingFS;
		vsP = pszLMLightingVS;
		}
	}
else {
	if (nLights) {
		fsP = (nLights == 1) ? pszPP1LightingFS : pszPPXLightingFS;
		vsP = pszPPLightingVS;
		//nLights = MAX_LIGHTS_PER_PIXEL;
		}
	else {
		fsP = pszLightingFS;
		vsP = pszLightingVS;
		}
	}
PrintLog ("building lighting shader programs\n");
if ((gameStates.ogl.bPerPixelLightingOk = (gameStates.ogl.bShadersOk && gameOpts->render.nPath))) {
	pszFS = BuildLightingShader (fsP [nType], nLights);
	pszVS = BuildLightingShader (vsP [nType], nLights);
	bOk = (pszFS != NULL) && (pszVS != NULL) &&
			CreateShaderProg (perPixelLightingShaderProgs [i] + nType) &&
			CreateShaderFunc (perPixelLightingShaderProgs [i] + nType, ppLfs [i] + nType, ppLvs [i] + nType, pszFS, pszVS, 1) &&
			LinkShaderProg (perPixelLightingShaderProgs [i] + nType);
	D2_FREE (pszFS);
	D2_FREE (pszVS);
	if (!bOk) {
		gameStates.ogl.bPerPixelLightingOk =
		gameOpts->ogl.bPerPixelLighting = 0;
		for (i = 0; i < MAX_LIGHTS_PER_PIXEL; i++)
			for (j = 0; j < 4; j++)
				DeleteShaderProg (perPixelLightingShaderProgs [i] + j);
		nLights = 0;
		}
	}
return gameData.render.ogl.nPerPixelLights [nType] = nLights;
}

// ----------------------------------------------------------------------------------------------

void ResetPerPixelLightingShaders (void)
{
memset (perPixelLightingShaderProgs, 0, sizeof (perPixelLightingShaderProgs));
}

// ----------------------------------------------------------------------------------------------
//eof
