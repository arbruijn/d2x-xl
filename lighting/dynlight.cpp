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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CLightManager::Init (void)
{
m_data.Init ();
}

//------------------------------------------------------------------------------

void CLightManager::Destroy (void)
{
m_data.Destroy ();
}

//------------------------------------------------------------------------------

bool CLightManager::Create (void)
{
return m_data.Create ();
}

//------------------------------------------------------------------------------

void CLightManager::SetColor (short nLight, float red, float green, float blue, float fBrightness)
{
	CDynLight*	pl = m_data.lights [0] + nLight;
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
pl->fSpecular[0] = red;
pl->fSpecular[1] = green;
pl->fSpecular[2] = blue;
for (i = 0; i < 3; i++) {
#if USE_OGL_LIGHTS
	pl->info.fAmbient.v [i] = pl->info.fDiffuse [i] * 0.01f;
	pl->info.fDiffuse.v [i] =
#endif
	pl->fEmissive [i] = pl->fSpecular[i] * fBrightness;
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

void CLightManager::SetPos (short nObject)
{
if (SHOW_DYN_LIGHT) {
	int	nLight = m_data.owners [nObject];
	if (nLight >= 0)
		m_data.lights [0][nLight].info.vPos = OBJECTS [nObject].info.position.vPos;
	}
}

//------------------------------------------------------------------------------

short CLightManager::Find (short nSegment, short nSide, short nObject)
{
if (gameStates.render.nLightingMethod && !gameStates.app.bNostalgia) {
	CDynLight*	pl = m_data.lights [0];

	if (nObject >= 0)
		return m_data.owners [nObject];
	if (nSegment >= 0)
		for (short i = 0; i < m_data.nLights [0]; i++, pl++)
			if ((pl->info.nSegment == nSegment) && (pl->info.nSide == nSide))
				return i;
	}
return -1;
}

//------------------------------------------------------------------------------

short CLightManager::Update (tRgbaColorf *colorP, float fBrightness, short nSegment, short nSide, short nObject)
{
	short	nLight = Find (nSegment, nSide, nObject);

if (nLight >= 0) {
	CDynLight* pl = m_data.lights [0] + nLight;
	if (!colorP)
		colorP = &pl->info.color;
	if (nObject >= 0)
		pl->info.vPos = OBJECTS [nObject].info.position.vPos;
	if ((pl->info.fBrightness != fBrightness) ||
		 (pl->info.color.red != colorP->red) || (pl->info.color.green != colorP->green) || (pl->info.color.blue != colorP->blue)) {
		SetColor (nLight, colorP->red, colorP->green, colorP->blue, fBrightness);
		}
	}
return nLight;
}

//------------------------------------------------------------------------------

int CLightManager::LastEnabled (void)
{
	short	i = m_data.nLights [0];

while (i)
	if (m_data.lights [0][--i].info.bState)
		return i;
return -1;
}

//------------------------------------------------------------------------------

void CLightManager::Swap (CDynLight* pl1, CDynLight* pl2)
{
if (pl1 != pl2) {
	CDynLight h = *pl1;
	*pl1 = *pl2;
	*pl2 = h;
#if USE_OGL_LIGHTS
	pl1->handle = (unsigned) (GL_LIGHT0 + (pl1 - m_data.lights [0]));
	pl2->handle = (unsigned) (GL_LIGHT0 + (pl2 - m_data.lights [0]));
#endif
	if (pl1->info.nObject >= 0)
		m_data.owners [pl1->info.nObject] =
			(short) (pl1 - m_data.lights [0]);
	if (pl2->info.nObject >= 0)
		m_data.owners [pl2->info.nObject] =
			(short) (pl2 - m_data.lights [0]);
	}
}

//------------------------------------------------------------------------------

int CLightManager::Toggle (short nSegment, short nSide, short nObject, int bState)
{
	short nLight = Find (nSegment, nSide, nObject);

if (nLight >= 0) {
	CDynLight* pl = m_data.lights [0] + nLight;
	pl->info.bOn = bState;
	}
return nLight;
}

//------------------------------------------------------------------------------

void CLightManager::Register (tFaceColor *colorP, short nSegment, short nSide)
{
#if 0
if (!colorP || colorP->index) {
	tLightInfo	*pli = gameData.render.shadows.lightInfo + gameData.render.shadows.nLights++;
#if DBG
	CAngleVector	a;
#endif
	pli->nIndex = (int) nSegment * 6 + nSide;
	pli->pos = SEGMENTS [nSegment].m_SideCenter (nSide);
	OOF_VecVms2Gl (pli->glPos, &pli->pos);
	pli->nSegNum = nSegment;
	pli->nSideNum = (ubyte) nSide;
#if DBG
	VmExtractAnglesVector (&a, SEGMENTS [nSegment].m_sides [nSide].m_normals);
	VmAngles2Matrix (&pli->position.mOrient, &a);
#endif
	}
#endif
}

//-----------------------------------------------------------------------------

int CLightManager::IsFlickering (short nSegment, short nSide)
{
	tVariableLight	*flP = gameData.render.lights.flicker.lights;

for (int l = gameData.render.lights.flicker.nLights; l; l--, flP++)
	if ((flP->nSegment == nSegment) && (flP->nSide == nSide))
		return 1;
return 0;
}

//-----------------------------------------------------------------------------

int CLightManager::IsDestructible (short nTexture)
{
if (!nTexture)
	return 0;
if (IsMultiGame && netGame.bIndestructibleLights)
	return 0;
short nClip = gameData.pig.tex.tMapInfoP [nTexture].nEffectClip;
tEffectClip	*ecP = (nClip < 0) ? NULL : gameData.eff.effectP + nClip;
short	nDestBM = ecP ? ecP->nDestBm : -1;
ubyte	bOneShot = ecP ? (ecP->flags & EF_ONE_SHOT) != 0 : 0;
if (nClip == -1)
	return gameData.pig.tex.tMapInfoP [nTexture].destroyed != -1;
return (nDestBM != -1) && !bOneShot;
}

//------------------------------------------------------------------------------

static inline float Intensity (float r, float g, float b)
{
   float i = 0;
   int   j = 0;

 if (r > 0) {
   i += r;
   j++;
   }
 if (g > 0) {
   i += g;
   j++;
   }
 if (b > 0) {
   i += b;
   j++;
   }
return j ? i / j : 1;
}

//------------------------------------------------------------------------------

int CLightManager::Add (tFace* faceP, tRgbaColorf *colorP, fix xBrightness, short nSegment,
							   short nSide, short nObject, short nTexture, CFixVector *vPos)
{
	CDynLight*	pl;
	short			h, i;
	float			fBrightness = X2F (xBrightness);
#if USE_OGL_LIGHTS
	GLint			nMaxLights;
#endif

#if 0
if (xBrightness > I2X (1))
	xBrightness = I2X (1);
#endif
if (fBrightness <= 0)
	return -1;

#if DBG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nSegment = nSegment;
if ((nDbgObj >= 0) && (nObject == nDbgObj))
	nDbgObj = nDbgObj;
if (colorP && ((colorP->red > 1) || (colorP->green > 1) || (colorP->blue > 1)))
	colorP = colorP;
#endif

if (gameStates.render.nLightingMethod && (nSegment >= 0) && (nSide >= 0)) {
#if 1
	fBrightness /= Intensity (colorP->red, colorP->green, colorP->blue);
#else
	if (fBrightness < 1)
		fBrightness = (float) sqrt (fBrightness);
	else
		fBrightness *= fBrightness;
#endif
	}
if (colorP)
	colorP->alpha = 1.0f;
if (0 <= (h = Update (colorP, fBrightness, nSegment, nSide, nObject)))
	return h;
if (!colorP)
	return -1;
if ((colorP->red == 0.0f) && (colorP->green == 0.0f) && (colorP->blue == 0.0f)) {
	if (gameStates.app.bD2XLevel && gameStates.render.bColored)
		return -1;
	colorP->red = colorP->green = colorP->blue = 1.0f;
	}
if (m_data.nLights [0] >= MAX_OGL_LIGHTS) {
	gameStates.render.bHaveDynLights = 0;
	return -1;	//too many lights
	}
i = m_data.nLights [0]; //LastEnabledDynLight () + 1;
pl = m_data.lights [0] + i;
pl->info.faceP = faceP;
pl->info.nSegment = nSegment;
pl->info.nSide = nSide;
pl->info.nObject = nObject;
pl->info.nPlayer = -1;
pl->info.bState = 1;
pl->info.bSpot = 0;
pl->info.fBoost = 0;
pl->info.bPowerup = 0;
//0: static light
//2: object/lightning
//3: headlight
if (nObject >= 0) {
	CObject *objP = OBJECTS + nObject;
	//HUDMessage (0, "Adding object light %d, type %d", m_data.nLights [0], objP->info.nType);
	pl->info.nType = 2;
	if (objP->info.nType == OBJ_POWERUP) {
		int id = objP->info.nId;
		if ((id == POW_EXTRA_LIFE) || (id == POW_ENERGY) || (id == POW_SHIELD_BOOST) ||
			 (id == POW_HOARD_ORB) || (id == POW_MONSTERBALL) || (id == POW_INVUL))
			pl->info.bPowerup = 1;
		else
			pl->info.bPowerup = 2;
		}
	pl->info.vPos = objP->info.position.vPos;
	pl->info.fRad = 0; //X2F (OBJECTS [nObject].size) / 2;
	if (fBrightness > 1) {
		if ((objP->info.nType == OBJ_FIREBALL) || (objP->info.nType == OBJ_EXPLOSION)) {
			pl->info.fBoost = 1;
			pl->info.fRad = fBrightness;
			}
		else if ((objP->info.nType == OBJ_WEAPON) && (objP->info.nId == FLARE_ID)) {
			pl->info.fBoost = 1;
			pl->info.fRad = 2 * fBrightness;
			}
		}
	m_data.owners [nObject] = m_data.nLights [0];
	}
else if (nSegment >= 0) {
#if 0
	CFixVector	vOffs;
	CSide			*sideP = SEGMENTS [nSegment].m_sides + nSide;
#endif
	if (nSide < 0) {
		pl->info.nType = 2;
		pl->info.bVariable = 0;
		pl->info.fRad = 0;
		if (vPos)
			pl->info.vPos = *vPos;
		else
			pl->info.vPos = SEGMENTS [nSegment].Center ();
		}
	else {
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			nDbgSeg = nDbgSeg;
#endif
		pl->info.nType = 0;
		pl->info.fRad = faceP ? faceP->fRads [1] : 0;
		//RegisterLight (NULL, nSegment, nSide);
		pl->info.bVariable = IsDestructible (nTexture) || IsFlickering (nSegment, nSide) || SEGMENTS [nSegment].Side (nSide)->IsVolatile ();
		m_data.nVariable += pl->info.bVariable;
		pl->info.vPos = SEGMENTS [nSegment].SideCenter (nSide);
		CSide			*sideP = SEGMENTS [nSegment].m_sides + nSide;
		CFixVector	vOffs;
		vOffs = sideP->m_normals[0] + sideP->m_normals[1];
		pl->info.vDirf.Assign (vOffs);
		pl->info.vDirf *= 0.5f;
		if (gameStates.render.bPerPixelLighting) {
			vOffs *= FixDiv(1, 4);
			pl->info.vPos += vOffs;
			}
		}
	}
else {
	pl->info.nType = 3;
	pl->info.bVariable = 0;
	}
#if 0
PrintLog ("adding light %d,%d\n",
		  m_data.nLights [0], pl - m_data.lights [0]);
#endif
pl->info.bOn = 1;
pl->bTransform = 1;
SetColor (m_data.nLights [0], colorP->red, colorP->green, colorP->blue, fBrightness);
return m_data.nLights [0]++;
}

//------------------------------------------------------------------------------

void CLightManager::DeleteFromList (CDynLight* pl, short nLight)
{
//PrintLog ("removing light %d,%d\n", nLight, pl - m_data.lights [0]);
// if not removing last light in list, move last light down to the now free list entry
// and keep the freed light handle thus avoiding gaps in used handles
if (nLight < --m_data.nLights [0]) {
	*pl = m_data.lights [0][m_data.nLights [0]];
	if (pl->info.nObject >= 0)
		m_data.owners [pl->info.nObject] = nLight;
	if (pl->info.nPlayer < MAX_PLAYERS)
		m_data.nHeadlights [pl->info.nPlayer] = nLight;
	}
}

//------------------------------------------------------------------------------

void CLightManager::Delete (short nLight)
{
if ((nLight >= 0) && (nLight < m_data.nLights [0])) {
	CDynLight* pl = m_data.lights [0] + nLight;
#if DBG
	if ((nDbgSeg >= 0) && (nDbgSeg == pl->info.nSegment) && ((nDbgSide < 0) || (nDbgSide == pl->info.nSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (!pl->info.nType) {
		// do not remove static lights, or the nearest lights to segment info will get messed up!
		pl->info.bState = pl->info.bOn = 0;
		return;
		}
	DeleteFromList (pl, nLight);
	}
}

//------------------------------------------------------------------------------

int CLightManager::Delete (short nSegment, short nSide, short nObject)
{
	int	nLight = Find (nSegment, nSide, nObject);

if (nLight < 0)
	return 0;
Delete (nLight);
if (nObject >= 0)
	m_data.owners [nObject] = -1;
return 1;
}

//------------------------------------------------------------------------------

void CLightManager::DeleteLightnings (void)
{
	CDynLight* pl = m_data.lights [0];

for (short i = 0; i < m_data.nLights [0]; )
	if ((pl->info.nSegment >= 0) && (pl->info.nSide < 0)) {
		DeleteFromList (pl, i);
		}
	else {
		i++;
		pl++;
		}
}

//------------------------------------------------------------------------------

void CLightManager::Reset (void)
{
for (short i = 0; i < m_data.nLights [0]; i++)
	Delete (i);
}

//------------------------------------------------------------------------------

void CLightManager::SetMaterial (short nSegment, short nSide, short nObject)
{
	//static float fBlack [4] = {0.0f, 0.0f, 0.0f, 1.0f};
	int nLight = Find (nSegment, nSide, nObject);

if (nLight >= 0) {
	CDynLight* pl = m_data.lights [0] + nLight;
	if (pl->info.bState) {
		m_data.material.emissive = *reinterpret_cast<CFloatVector*> (&pl->fEmissive);
		m_data.material.specular = *reinterpret_cast<CFloatVector*> (&pl->fEmissive);
		m_data.material.shininess = 8;
		m_data.material.bValid = 1;
		m_data.material.nLight = nLight;
		return;
		}
	}
m_data.material.bValid = 0;
}

//------------------------------------------------------------------------------

int CDynLight::Compare (CDynLight &other)
{
if (info.bVariable < other.info.bVariable) 
	return -1;
if (info.bVariable > other.info.bVariable) 
	return 1;
if (info.nSegment < other.info.nSegment) 
	return -1;
if (info.nSegment > other.info.nSegment) 
	return 1;
if (info.nSide < other.info.nSide) 
	return -1;
if (info.nSide > other.info.nSide) 
	return 1;
if (info.nObject < other.info.nObject) 
	return -1;
if (info.nObject > other.info.nObject) 
	return 1;
if (info.fBrightness < other.info.fBrightness) 
	return -1;
if (info.fBrightness > other.info.fBrightness) 
	return 1;
return 0;
}

//------------------------------------------------------------------------------

void CLightManager::Sort (void)
{
CQuickSort<CDynLight> qs;
qs.SortAscending (m_data.lights [0], 0, m_data.nLights [0] - 1);
}

//------------------------------------------------------------------------------

void CLightManager::AddFromGeometry (void)
{
	int			nFace, nSegment, nSide, nTexture, nLight;
	tFace*		faceP;
	tFaceColor*	colorP;

#if 0
for (nTexture = 0; nTexture < 910; nTexture++)
	nLight = IsLight (nTexture);
#endif
gameStates.render.bHaveDynLights = 1;
#if 0
if (gameStates.app.bD1Mission)
	gameData.render.fAttScale *= 2;
#endif
gameStates.ogl.fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightRange];
memset (&m_data.headlights, 0, sizeof (m_data.headlights));
if (gameStates.render.nLightingMethod)
	gameData.render.color.vertices.Clear ();
m_data.Init ();
for (nFace = gameData.segs.nFaces, faceP = FACES.faces.Buffer (); nFace; nFace--, faceP++) {
	nSegment = faceP->nSegment;
	if (SEGMENTS [nSegment].m_nType == SEGMENT_IS_SKYBOX)
		continue;
#if DBG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	nSide = faceP->nSide;
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	nTexture = faceP->nBaseTex;
	if ((nTexture < 0) || (nTexture >= MAX_WALL_TEXTURES))
		continue;
	colorP = gameData.render.color.textures + nTexture;
	if ((nLight = IsLight (nTexture)))
		Add (faceP, &colorP->color, nLight, (short) nSegment, (short) nSide, -1, nTexture, NULL);
	nTexture = faceP->nOvlTex;
#if 0//def _DEBUG
	if (gameStates.app.bD1Mission && (nTexture == 289)) //empty, light
		continue;
#endif
	if ((nTexture > 0) && (nTexture < MAX_WALL_TEXTURES) && (nLight = IsLight (nTexture)) /*gameData.pig.tex.info.fBrightness [nTexture]*/) {
		colorP = gameData.render.color.textures + nTexture;
		Add (faceP, &colorP->color, nLight, (short) nSegment, (short) nSide, -1, nTexture, NULL);
		}
	//if (m_data.nLights)
	//	return;
	if (!gameStates.render.bHaveDynLights) {
		Reset ();
		return;
		}
	}
Sort ();
#if 0
PrintLog ("light sources:\n");
CDynLight* pl = m_data.lights [0];
for (int i = m_data.nLights [0]; i; i--, pl++)
	PrintLog ("%d,%d,%d,%d\n", pl->info.nSegment, pl->info.nSide, pl->info.nObject, pl->info.bVariable);
PrintLog ("\n");
#endif
}

//------------------------------------------------------------------------------

void CLightManager::GatherStaticVertexLights (int nVertex, int nMax, int nThread)
{
	tFaceColor*		pf = gameData.render.color.ambient + nVertex;
	CFloatVector	vVertex;
	int				bColorize = !gameStates.render.nLightingMethod;

for (; nVertex < nMax; nVertex++, pf++) {
#if DBG
	if (nVertex == nDbgVertex)
		nVertex = nVertex;
#endif
	vVertex.Assign (gameData.segs.vertices [nVertex]);
	ResetActiveLights (nThread, 0);
	ResetUsedLights (0, nThread);
	SetNearestVertexLights (-1, nVertex, NULL, 1, 1, bColorize, nThread);
	gameData.render.color.vertices [nVertex].index = 0;
	G3VertexColor (gameData.segs.points [nVertex].p3_normal.vNormal.V3(), vVertex.V3(), nVertex, pf, NULL, 1, 0, nThread);
	}
}

//------------------------------------------------------------------------------

#if DBG
extern int nDbgVertex;
#endif

void CLightManager::GatherStaticLights (int nLevel)
{
if (gameStates.app.bNostalgia)
	return;

int i, j, bColorize = !gameStates.render.nLightingMethod;

PrintLog ("Computing static lighting\n");
gameData.render.vertColor.bDarkness = IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [IsMultiGame].bDarkness;
gameStates.render.nState = 0;
Transform (1, bColorize);
for (i = 0; i < gameData.segs.nVertices; i++)
	m_data.variableVertLights [i] = VariableVertexLights (i);
if (RENDERPATH && gameStates.render.bPerPixelLighting && lightmapManager.HaveLightmaps ()) {
	gameData.render.color.ambient.Clear ();
	return;
	}
if (gameStates.render.nLightingMethod || (gameStates.render.bAmbientColor && !gameStates.render.bColored)) {
		tFaceColor*	pfh, *pf = gameData.render.color.ambient.Buffer ();
		CSegment*	segP;

	memset (pf, 0, gameData.segs.nVertices * sizeof (*pf));
	if (!RunRenderThreads (rtStaticVertLight))
		ComputeStaticVertexLights (0, gameData.segs.nVertices, 0);
	pf = gameData.render.color.ambient.Buffer ();
	for (i = 0, segP = SEGMENTS.Buffer (); i < gameData.segs.nSegments; i++, segP++) {
		if (segP->m_nType == SEGMENT_IS_SKYBOX) {
			short* sv = segP->m_verts;
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
