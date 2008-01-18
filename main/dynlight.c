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
#include "gameseg.h"
#include "endlevel.h"
#include "renderthreads.h"
#include "light.h"
#include "headlight.h"
#include "dynlight.h"

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

if (pl->nType ? gameOpts->render.color.bGunLight : gameOpts->render.color.bAmbientLight) {
	pl->color.red = red;
	pl->color.green = green;
	pl->color.blue = blue;
	}
else
	pl->color.red = 
	pl->color.green =
	pl->color.blue = 1.0f;
pl->brightness = brightness;
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
else {
	short nClip = gameData.pig.tex.pTMapInfo [nTexture].eclip_num;
	tEffectClip	*ecP = (nClip < 0) ? NULL : gameData.eff.pEffects + nClip;
	short	nDestBM = ecP ? ecP->nDestBm : -1;
	ubyte	bOneShot = ecP ? (ecP->flags & EF_ONE_SHOT) != 0 : 0;
	if (nClip == -1) 
		return gameData.pig.tex.pTMapInfo [nTexture].destroyed != -1;
	return (nDestBM != -1) && !bOneShot;
	}
}

//------------------------------------------------------------------------------

int AddDynLight (tRgbaColorf *pc, fix xBrightness, short nSegment, short nSide, short nObject, vmsVector *vPos)
{
	tDynLight	*pl;
	short			h, i;
	fix			rMin, rMax;
#if USE_OGL_LIGHTS
	GLint			nMaxLights;
#endif

#if 0
if (xBrightness > F1_0)
	xBrightness = F1_0;
#endif
#ifdef _DEBUG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nSegment = nSegment;
#endif
if (0 <= (h = UpdateDynLight (pc, f2fl (xBrightness), nSegment, nSide, nObject)))
	return h;
if (!pc)
	return -1;
if ((pc->red == 0.0f) && (pc->green == 0.0f) && (pc->blue == 0.0f)) {
	if (gameStates.app.bD2XLevel)
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
pl->nSegment = nSegment;
pl->nSide = nSide;
pl->nObject = nObject;
pl->nPlayer = -1;
pl->bState = 1;
pl->bSpot = 0;
//0: static light
//2: object/lightning
//3: headlight
SetDynLightColor (gameData.render.lights.dynamic.nLights, pc->red, pc->green, pc->blue, f2fl (xBrightness));
if (nObject >= 0) {
	pl->nType = 2;
	pl->vPos = gameData.objs.objects [nObject].position.vPos;
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
		int	t = gameData.segs.segments [nSegment].sides [nSide].nOvlTex;
		pl->nType = 0;
		ComputeSideRads (nSegment, nSide, &rMin, &rMax);
		pl->rad = f2fl ((rMin + rMax) / 20);
		//RegisterLight (NULL, nSegment, nSide);
		pl->bVariable = IsDestructibleLight (t) || IsFlickeringLight (nSegment, nSide) || IS_WALL (SEGMENTS [nSegment].sides [nSide].nWall);
		COMPUTE_SIDE_CENTER_I (&pl->vPos, nSegment, nSide);
		}
#if 0
	VmVecAdd (&vOffs, sideP->normals, sideP->normals + 1);
	VmVecScaleFrac (&vOffs, 1, 200);
	VmVecInc (&pl->vPos, &vOffs);
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
LogErr ("adding light %d,%d\n", 
		  gameData.render.lights.dynamic.nLights, pl - gameData.render.lights.dynamic.lights);
#endif
pl->bOn = 1;
pl->bTransform = 1;
return gameData.render.lights.dynamic.nLights++;
}

//------------------------------------------------------------------------------

void RemoveDynLightFromList (tDynLight *pl, short nLight)
{
//LogErr ("removing light %d,%d\n", nLight, pl - gameData.render.lights.dynamic.lights);
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
	if (!pl->nType) {
		// do not remove static lights, or the nearest lights to tSegment info will get messed up!
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
	short			i;
for (i = 0; i < gameData.render.lights.dynamic.nLights; )
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
	short	i;
for (i = 0; i < gameData.render.lights.dynamic.nLights; i++)
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
		gameData.render.lights.dynamic.material.shininess = 96;
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

void AddDynLights (void)
{
	int			nSegment, nSide, t;
	tSegment		*segP;
	tSide			*sideP;
	tFaceColor	*pc;
	short			*pSegLights, *pVertLights, *pOwners;

gameStates.render.bHaveDynLights = 1;
//glEnable (GL_LIGHTING);
if (gameOpts->render.bDynLighting)
	memset (gameData.render.color.vertices, 0, sizeof (*gameData.render.color.vertices) * MAX_VERTICES);
//memset (gameData.render.color.ambient, 0, sizeof (*gameData.render.color.ambient) * MAX_VERTICES);
pSegLights = gameData.render.lights.dynamic.nNearestSegLights;
pVertLights = gameData.render.lights.dynamic.nNearestVertLights;
pOwners = gameData.render.lights.dynamic.owners;
memset (&gameData.render.lights.dynamic, 0xff, sizeof (gameData.render.lights.dynamic));
memset (pSegLights, 0xff, sizeof (*pSegLights) * MAX_SEGMENTS * MAX_NEAREST_LIGHTS);
memset (pVertLights, 0xff, sizeof (*pVertLights) * MAX_SEGMENTS * MAX_NEAREST_LIGHTS);
memset (pOwners, 0xff, sizeof (*pOwners) * MAX_OBJECTS);
gameData.render.lights.dynamic.nNearestSegLights = pSegLights;
gameData.render.lights.dynamic.nNearestVertLights = pVertLights;
gameData.render.lights.dynamic.owners = pOwners;
gameData.render.lights.dynamic.nLights = 0;
gameData.render.lights.dynamic.material.bValid = 0;
for (nSegment = 0, segP = gameData.segs.segments; nSegment < gameData.segs.nSegments; nSegment++, segP++) {
	if (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SKYBOX)
		continue;
#ifdef _DEBUG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	for (nSide = 0, sideP = segP->sides; nSide < 6; nSide++, sideP++) {
#ifdef _DEBUG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			nDbgSeg = nDbgSeg;
#endif
		if ((segP->children [nSide] >= 0) && !IS_WALL (sideP->nWall))
			continue;
		t = sideP->nBaseTex;
		if (t >= MAX_WALL_TEXTURES) 
			continue;
		pc = gameData.render.color.textures + t;
		if (gameData.pig.tex.brightness [t])
			AddDynLight (&pc->color, gameData.pig.tex.brightness [t], (short) nSegment, (short) nSide, -1, NULL);
		t = sideP->nOvlTex;
		if ((t > 0) && (t < MAX_WALL_TEXTURES) && gameData.pig.tex.brightness [t]) {
			pc = gameData.render.color.textures + t;
			if ((pc->color.red > 0.0f) || (pc->color.green > 0.0f) || (pc->color.blue > 0.0f))
				AddDynLight (&pc->color, gameData.pig.tex.brightness [t], (short) nSegment, (short) nSide, -1, NULL);
			}
		//if (gameData.render.lights.dynamic.nLights)
		//	return;
		if (!gameStates.render.bHaveDynLights) {
			RemoveDynLights ();
			return;
			}
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
	memcpy (&psl->color, &pl->color, sizeof (pl->color));
	psl->vPos = pl->vPos;
	VmsVecToFloat (psl->pos, &pl->vPos);
	if (gameStates.ogl.bUseTransform)
		psl->pos [1] = psl->pos [0];
	else {
		G3TransformPointf (psl->pos + 1, psl->pos, 0);
		psl->pos [1].p.w = 1;
		}
	psl->brightness = pl->brightness;
	if ((psl->bSpot = pl->bSpot))
		TransformOglHeadLight (pl, psl);
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
#if 1
if (gameData.render.lights.dynamic.headLights.nLights) {
	//G3EndFrame ();
#if 1
	G3SetViewMatrix (&gameData.objs.viewer->position.vPos, 
						  &gameData.objs.viewer->position.mOrient, 
						  gameStates.render.xZoom, 1);
#endif
	}
#endif
}

//------------------------------------------------------------------------------

void SetNearestVertexLights (int nVertex, ubyte nType, int bStatic, int bVariable, int nThread)
{
//if (gameOpts->render.bDynLighting) 
	{
	short				*pnl = gameData.render.lights.dynamic.nNearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	short				i, j;
	tShaderLight	*psl;

#ifdef _DEBUG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
	gameData.render.lights.dynamic.shader.iVariableLights [nThread] = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
	for (i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
#ifdef _DEBUG
		if (j >= gameData.render.lights.dynamic.nLights)
			break;
#endif
		psl = gameData.render.lights.dynamic.shader.lights + j;
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
		psl->nType = nType;
		psl->bState = 1;
		gameData.render.lights.dynamic.shader.activeLights [nThread][gameData.render.lights.dynamic.shader.nActiveLights [nThread]++] = psl;
		}
	}
}

//------------------------------------------------------------------------------

void SetNearestStaticLights (int nSegment, int bStatic, ubyte nType, int nThread)
{
	static int nActiveLights [4] = {-1, -1, -1, -1};
#if 0
	static int nFrameFlipFlop = -1;
	static int nLastSeg [4] = {-1, -1, -1, -1};
	static ubyte nLastType [4] = {255, 255, 255, 255};
#endif
   tShaderLight *psl;
	int nMaxLights = gameOpts->ogl.bObjLighting ? 8 : gameOpts->ogl.nMaxLights;

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
	short	*pnl = gameData.render.lights.dynamic.nNearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	short	i, j;
	gameData.render.lights.dynamic.shader.iStaticLights [nThread] = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
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
		gameData.render.lights.dynamic.shader.activeLights [nThread][gameData.render.lights.dynamic.shader.nActiveLights [nThread]++] =
			psl;
		}
	}
nActiveLights [nThread] = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
}

//------------------------------------------------------------------------------

void QSortDynamicLights (int left, int right, int nThread)
{
	int	l = left,
			r = right,
			m = gameData.render.lights.dynamic.shader.activeLights [nThread][(l + r) / 2]->xDistance;
		
do {
	while (gameData.render.lights.dynamic.shader.activeLights [nThread][l]->xDistance < m)
		l++;
	while (gameData.render.lights.dynamic.shader.activeLights [nThread][r]->xDistance > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tShaderLight *h = gameData.render.lights.dynamic.shader.activeLights [nThread][l];
			gameData.render.lights.dynamic.shader.activeLights [nThread][l] = gameData.render.lights.dynamic.shader.activeLights [nThread][r];
			gameData.render.lights.dynamic.shader.activeLights [nThread][r] = h;
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

//------------------------------------------------------------------------------

#if PROFILING
time_t tSetNearestDynamicLights = 0;
#endif

short SetNearestDynamicLights (int nSegment, int bVariable, int nType, int nThread)
{
#if CACHE_LIGHTS
	static int nLastSeg [4] = {-1, -1, -1, -1};
	static int nActiveLights [4] = {-1, -1, -1, -1};
	static int nFrameFlipFlop = 0;
#endif
	int nMaxLights = (nType && gameOpts->ogl.bObjLighting) ? 8 : gameOpts->ogl.nMaxLights;
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
	short				i = gameData.render.lights.dynamic.shader.nLights,
						nLightSeg;
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights + i;
	vmsVector		c;

	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = 0;
	COMPUTE_SEGMENT_CENTER_I (&c, nSegment);
	while (i--) {
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
			if ((psl->xDistance = VmVecDist (&c, &psl->vPos)) > MAX_LIGHT_RANGE)
				continue;
			}
		gameData.render.lights.dynamic.shader.activeLights [nThread][gameData.render.lights.dynamic.shader.nActiveLights [nThread]++] = psl;
		}
	}
#if 1
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
	if (SetNearestDynamicLights (nSegment, 1)) {
		short				nLights = gameData.render.lights.dynamic.shader.nLights;
		tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights + nLights;
		float				fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightRange];
		float				fLightDist, fAttenuation;
		fVector			vPosf;
		if (pvPos)
			VmsVecToFloat (&vPosf, pvPos);
		for (i = 0; i < gameData.render.lights.dynamic.shader.nActiveLights; i++) {
			psl = gameData.render.lights.dynamic.shader.activeLights [i];
	#if 1
			if (pvPos) {
				vVertex = gameData.segs.vertices [*pv];
				//G3TransformPoint (&vVertex, &vVertex);
				fLightDist = VmVecDistf (psl->pos, &vPosf) / fLightRange;
				fAttenuation = fLightDist / psl->brightness;
				VmVecScaleAddf ((fVector *) &c.color, (fVector *) &c.color, (fVector *) &psl->color, 1.0f / fAttenuation);
				}
			else 
	#endif
				{
				VmVecIncf ((fVector *) &psc->color, (fVector *) &psl->color);
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
	VmsVecToFloat (&vVertex, gameData.segs.vertices + nVertex);
	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = 0;
	SetNearestVertexLights (nVertex, 1, 1, bColorize, nThread);
	gameData.render.color.vertices [nVertex].index = 0;
	G3VertexColor (&gameData.segs.points [nVertex].p3_normal.vNormal, &vVertex, nVertex, pf, NULL, 1, 0, nThread);
	}
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
extern int nDbgVertex;
#endif

void ComputeStaticDynLighting (void)
{
memset (&gameData.render.lights.dynamic.headLights, 0, sizeof (gameData.render.lights.dynamic.headLights));
if (gameStates.app.bNostalgia)
	return;
if (gameOpts->render.bDynLighting || (gameOpts->render.color.bAmbientLight && !gameStates.render.bColored)) {
		int				i, j, bColorize = !gameOpts->render.bDynLighting;
		tFaceColor		*pfh, *pf = gameData.render.color.ambient;
		tSegment2		*seg2P;

	LogErr ("Computing static lighting\n");
	gameStates.ogl.fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightRange];
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
	G3TransformPointf (&v, &v, 0);
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
//eof
