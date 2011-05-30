#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>	// for memset ()

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "fix.h"
#include "vecmat.h"
#include "network.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "findpath.h"
#include "segmath.h"
#include "endlevel.h"
#include "renderthreads.h"
#include "light.h"
#include "lightmap.h"
#include "headlight.h"
#include "dynlight.h"
#include "lightprecalc.h"
#include "createmesh.h"
#include "text.h"

#define SORT_LIGHTS 0
#define PREFER_GEOMETRY_LIGHTS 1

class CLightManager lightManager;

#define FAST_POINTVIS 2

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
m_headlights.Init ();
return m_data.Create ();
}

//------------------------------------------------------------------------------

void CLightManager::SetColor (short nLight, float red, float green, float blue, float fBrightness)
{
	CDynLight*	pl = m_data.lights + nLight;
	int			i;

if ((pl->info.nType == 2) ? (gameOpts->render.color.nLevel >= 1) : (gameOpts->render.color.nLevel == 2)) {
	pl->info.color.red = red;
	pl->info.color.green = green;
	pl->info.color.blue = blue;
	}
else if (gameStates.app.bNostalgia) {
	pl->info.color.red =
	pl->info.color.green =
	pl->info.color.blue = 1.0f;
	}
else {
	pl->info.color.red =
	pl->info.color.green =
	pl->info.color.blue = (red + green + blue) / 3.0f;
	}
pl->info.color.alpha = 1.0;
pl->info.fBrightness = fBrightness;
pl->info.fRange = (float) sqrt (fBrightness / 2.0f);
pl->fSpecular.v.color.r = red;
pl->fSpecular.v.color.g = green;
pl->fSpecular.v.color.b = blue;
for (i = 0; i < 3; i++) {
#if USE_OGL_LIGHTS
	pl->info.fAmbient.dir [i] = pl->info.fDiffuse [i] * 0.01f;
	pl->info.fDiffuse.dir [i] =
#endif
	pl->fEmissive.v.vec [i] = pl->fSpecular.v.vec [i] * fBrightness;
	}
// light alphas
#if USE_OGL_LIGHTS
pl->info.fAmbient.dir [3] = 1.0f;
pl->info.fDiffuse.dir [3] = 1.0f;
pl->fSpecular.dir [3] = 1.0f;
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
		m_data.lights [nLight].info.vPos = OBJECTS [nObject].info.position.vPos;
	}
}

//------------------------------------------------------------------------------

short CLightManager::Find (short nSegment, short nSide, short nObject)
{
//if (gameStates.render.nLightingMethod && !gameStates.app.bNostalgia) 
	{
	CDynLight*	pl = &m_data.lights [0];

	if (nObject >= 0)
		return m_data.owners.Buffer () ? m_data.owners [nObject] : -1;
	if (nSegment >= 0)
		for (short i = 0; i < m_data.nLights [0]; i++, pl++)
			if ((pl->info.nSegment == nSegment) && (pl->info.nSide == nSide))
				return i;
	}
return -1;
}

//------------------------------------------------------------------------------

bool CLightManager::Ambient (short nSegment, short nSide)
{
	short i = Find (nSegment, nSide, -1);

return (i < 0) ? false : m_data.lights [i].info.bAmbient != 0;
}

//------------------------------------------------------------------------------

short CLightManager::Update (tRgbaColorf *colorP, float fBrightness, short nSegment, short nSide, short nObject)
{
	short	nLight = Find (nSegment, nSide, nObject);

if (nLight >= 0) {
	CDynLight* pl = m_data.lights + nLight;
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
	if (m_data.lights [--i].info.bState)
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
		m_data.owners [pl1->info.nObject] =	short (m_data.lights.Index (pl1));
	if (pl2->info.nObject >= 0)
		m_data.owners [pl2->info.nObject] =	short (m_data.lights.Index (pl2));
	}
}

//------------------------------------------------------------------------------

int CLightManager::Toggle (short nSegment, short nSide, short nObject, int bState)
{
	short nLight = Find (nSegment, nSide, nObject);

if (nLight >= 0) {
	CDynLight* pl = m_data.lights + nLight;
	pl->info.bOn = bState;
	pl->info.bState = 1;
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
	CAngleVector	vec;
#endif
	pli->nIndex = (int) nSegment * 6 + nSide;
	pli->coord = SEGMENTS [nSegment].m_SideCenter (nSide);
	OOF_VecVms2Gl (pli->glPos, &pli->coord);
	pli->nSegNum = nSegment;
	pli->nSideNum = (ubyte) nSide;
#if DBG
	VmExtractAnglesVector (&vec, SEGMENTS [nSegment].m_sides [nSide].m_normals);
	VmAngles2Matrix (&pli->position.mOrient, &vec);
#endif
	}
#endif
}

//-----------------------------------------------------------------------------

int CLightManager::IsTriggered (short nSegment, short nSide, bool bOppSide)
{
if ((nSegment < 0) || (nSide < 0))
	return 0;

	CTrigger*	trigP;
	int			i = 0;
	bool			bForceField = (gameData.pig.tex.tMapInfoP [SEGMENTS [nSegment].m_sides [nSide].m_nBaseTex].flags & TMI_FORCE_FIELD) != 0;

while ((i = FindTriggerTarget (nSegment, nSide, i))) {
	if (i < 0)
		trigP = &OBJTRIGGERS [-i - 1];
	else
		trigP = &TRIGGERS [i - 1];
	if ((trigP->m_info.nType == TT_OPEN_WALL) || (trigP->m_info.nType == TT_CLOSE_WALL))
		return 1;
	if (!bForceField) {
		if ((trigP->m_info.nType == TT_LIGHT_OFF) || (trigP->m_info.nType == TT_LIGHT_ON))
			return 1;
		}
	}
if (bOppSide || !bForceField)
	return 0;
short nConnSeg = SEGMENTS [nSegment].m_children [nSide];
return (nConnSeg >= 0) && IsTriggered (nConnSeg, SEGMENTS [nSegment].ConnectedSide (SEGMENTS + nConnSeg), true);
}

//-----------------------------------------------------------------------------

int CLightManager::IsFlickering (short nSegment, short nSide)
{
	CVariableLight* flP;

if ((flP = gameData.render.lights.flicker.Buffer ()))
	for (int l = gameData.render.lights.flicker.Length (); l; l--, flP++)
		if ((flP->m_nSegment == nSegment) && (flP->m_nSide == nSide))
			return 1;
return 0;
}

//-----------------------------------------------------------------------------

int CLightManager::IsDestructible (short nTexture)
{
if (!nTexture)
	return 0;
if (IsMultiGame && netGame.m_info.bIndestructibleLights)
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

int CLightManager::Add (CSegFace* faceP, tRgbaColorf *colorP, fix xBrightness, short nSegment,
							   short nSide, short nObject, short nTexture, CFixVector *vPos, ubyte bAmbient)
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
pl = m_data.lights + i;
pl->info.faceP = faceP;
pl->info.nSegment = nSegment;
pl->info.nSide = nSide;
pl->info.nObject = nObject;
pl->info.nPlayer = -1;
pl->info.bState = 1;
pl->info.bSpot = 0;
pl->info.fBoost = 0;
pl->info.bPowerup = 0;
pl->info.bAmbient = bAmbient;
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
#if 0
		if (pl->info.bPowerup > gameData.render.nPowerupFilter)
			return -1;
#endif
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
		pl->info.bVariable = 1;
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
		pl->info.fRad = faceP ? faceP->m_info.fRads [1] / 2.0f : 0;
		//RegisterLight (NULL, nSegment, nSide);
		pl->info.bVariable = IsDestructible (nTexture) || IsFlickering (nSegment, nSide) || IsTriggered (nSegment, nSide) || 
									SEGMENTS [nSegment].Side (nSide)->IsVolatile ();
		m_data.nVariable += pl->info.bVariable;
		CSide* sideP = SEGMENTS [nSegment].m_sides + nSide;
		pl->info.vPos = sideP->Center ();
		CFixVector vOffs = sideP->m_normals [2];
		pl->info.vDirf.Assign (vOffs);
		CFloatVector::Normalize (pl->info.vDirf);
#if 0
		if (gameStates.render.bPerPixelLighting) {
			vOffs *= I2X (1) / 64;
			pl->info.vPos += vOffs;
			}
#endif
		}
	}
else {
	pl->info.nType = 3;
	pl->info.bVariable = 0;
	}
#if 0
PrintLog ("adding light %d,%d\n", m_data.nLights [0], pl - m_data.lights [0]);
#endif
pl->info.bOn = 1;
pl->bTransform = 1;
SetColor (m_data.nLights [0], colorP->red, colorP->green, colorP->blue, fBrightness);
return m_data.nLights [0]++;
}

//------------------------------------------------------------------------------

bool CLightManager::DeleteFromList (CDynLight* pl, short nLight)
{
//PrintLog ("removing light %d,%d\n", nLight, pl - m_data.lights [0]);
// if not removing last light in list, move last light down to the now free list entry
// and keep the freed light handle thus avoiding gaps in used handles
if (!m_data.lights.Buffer () || (nLight >= m_data.nLights [0]))
	return false;
*pl = m_data.lights [--m_data.nLights [0]];
if (pl->info.nObject >= 0)
	m_data.owners [pl->info.nObject] = nLight;
if (pl->info.nPlayer < MAX_PLAYERS)
	m_headlights.lightIds [pl->info.nPlayer] = nLight;
return true;
}

//------------------------------------------------------------------------------

void CLightManager::Delete (short nLight)
{
if ((nLight >= 0) && (nLight < m_data.nLights [0])) {
	CDynLight* pl = m_data.lights + nLight;
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
	int nLight = Find (nSegment, nSide, nObject);

if (nLight < 0)
	return 0;
Delete (nLight);
if (nObject >= 0)
	m_data.owners [nObject] = -1;
return 1;
}

//------------------------------------------------------------------------------

void CLightManager::DeleteLightning (void)
{
	CDynLight* pl = &m_data.lights [0];

if (pl != NULL) {
	for (short i = 0; i < m_data.nLights [0]; ) {
		if (!((pl->info.nSegment >= 0) && (pl->info.nSide < 0) && DeleteFromList (pl, i))) {
			i++;
			pl++;
			}
		}
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
	CDynLight* pl = m_data.lights + nLight;
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

int CDynLight::LightPathLength (const short nLightSeg, const short nDestSeg, const CFixVector& vDestPos, fix xMaxLightRange, int bFastRoute, int nThread)
{
fix dist = (bFastRoute) 
			  ? gameData.segs.SegDist (nLightSeg, nDestSeg)
			  : simpleRouter [nThread].PathLength (info.vPos, nLightSeg, vDestPos, nDestSeg, X2I (xMaxLightRange / 5), WID_RENDPAST_FLAG | WID_FLY_FLAG, 0);
if (dist < 0)
	return -1;
return fix (dist / info.fRange);
}

//------------------------------------------------------------------------------
// Check whether a point can be seen from a light emitting face. Returns:
// -1: Point is behind light face or light face is on rear side of point
// 0: Point is occluded by geometry
// 1: Point is visible

#define LIGHTING_LEVEL 1

int CDynLight::SeesPoint (const short nDestSeg, const CFixVector* vNormal, CFixVector* vPoint, int nLevel, int nThread)
{
#if 1
int nLightSeg = info.nSegment;
#else
int nLightSeg = LightSeg ();
#endif

		CFloatVector	v0, v1, vLightToPointf, vNormalf;

#if FAST_POINTVIS

if (info.nSide < 0) {
	if (nLightSeg < 0)
		return 1;
	v1.Assign (*vPoint);
	v0.Assign (info.vPos);
	vLightToPointf = v1 - v0;
	CFloatVector::Normalize (vLightToPointf);
	if (CFloatVector::Dot (vLightToPointf, info.vDirf) < -0.001f) // light doesn't see point
		return 0;
	if (vNormal) {
		vNormalf.Assign (*vNormal);
		if (CFloatVector::Dot (vLightToPointf, vNormalf) > 0.001f) // light doesn't "see" face
			return 0;
		}
	return (nDestSeg < 0) || PointSeesPoint (&v0, &v1, nLightSeg, nDestSeg, FAST_POINTVIS - 1, nThread);
	}
else {
		static int nLevels [3] = {4, 0, -4};

		CSide*	sideP = SEGMENTS [nLightSeg].Side (info.nSide);
		int		i, j = nLevels [nLevel];

	v1.Assign (*vPoint);
	if (vNormal) 
		vNormalf.Assign (*vNormal);
	for (i = 4; i >= j; i--) {
		if (i == 4)
			v0.Assign (info.vPos);
		else if (i >= 0)
			v0 = FVERTICES [sideP->m_corners [i]];
		else
			v0 = CFloatVector::Avg (FVERTICES [sideP->m_corners [4 + i]], FVERTICES [sideP->m_corners [(5 + i) & 3]]); // center of face's edges

		vLightToPointf = v1 - v0;
		CFloatVector::Normalize (vLightToPointf);
		if (CFloatVector::Dot (vLightToPointf, info.vDirf) < -0.001f) // light doesn't see point
			continue;
		if (vNormal && (CFloatVector::Dot (vLightToPointf, vNormalf) > 0.001f)) // light doesn't "see" face
			continue;
		break;
		}
	return (i >= j);
#if DBG
	if ((nDbgSeg >= 0) && (nDbgVertex >= 0) && (nLightSeg == nDbgSeg) && ((nDbgSide < 0) || (info.nSide == nDbgSide)) && (nDbgVertex >= 0) && (*vPoint == VERTICES [nDbgVertex]))
		nDbgVertex = nDbgVertex;
#endif
	}

#else

CHitQuery fq (FQ_TRANSWALL | FQ_TRANSPOINT | FQ_VISIBILITY, &info.vPos, vPoint, nLightSeg, -1, 1, 0);
CHitData	hitData;
int nHitType = FindHitpoint (&fq, &hitData);
return (!nHitType || ((nHitType == HIT_WALL) && (hitData.hit.nSegment == nDestSeg)));

#endif
}

//------------------------------------------------------------------------------

int CDynLight::SeesPoint (const short nSegment, const short nSide, CFixVector* vPoint, const int nLevel, int nThread)
{
return SeesPoint (nSegment, &SEGMENTS [nSegment].Side (nSide)->Normal (2), vPoint, nLevel, nThread);
}

//------------------------------------------------------------------------------

int CDynLight::LightSeg (void)
{
if (info.nSegment >= 0)
	return info.nSegment;
CObject* objP = Object ();
if (objP)
	return objP->Segment ();
return -1;
}

//------------------------------------------------------------------------------

CObject* CDynLight::Object (void)
{
if (info.nSegment >= 0)
	return NULL;
if (info.nObject >= 0)
	return &OBJECTS [info.nObject];
if (!info.bSpot)
	return NULL;
return &OBJECTS [gameData.multiplayer.players [info.nPlayer].nObject];
}

//------------------------------------------------------------------------------

int CDynLight::Contribute (const short nDestSeg, const short nDestSide, CFixVector& vDestPos, const CFixVector* vNormal, fix xMaxLightRange, float fRangeMod, fix xDistMod, int nThread)
{
	short nLightSeg = info.nSegment;

if ((nLightSeg >= 0) && (nDestSeg >= 0)) {
#if DBG
	if ((nLightSeg == nDbgSeg) && ((nDbgSide < 0) || (nDbgSide == info.nSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (info.nSide < 0) 
		info.bDiffuse [nThread] = gameData.segs.SegVis (nLightSeg, nDestSeg);
	else {
#if DBG
		if (nDestSeg == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		if (SEGMENTS [nLightSeg].HasOutdoorsProp ()) {
			if ((nDestSide >= 0) && ((info.nSide != nDestSide) || (nLightSeg != nDestSeg)))
				return 0;
			info.bDiffuse [nThread] = 1;
			}
		else if (0 > (info.bDiffuse [nThread] = gameData.segs.LightVis (nLightSeg, nDestSeg)))
			return 0;
		}
	}
#if 0 //DBG
#else
else if ((nLightSeg = LightSeg ()) >= 0)
	info.bDiffuse [nThread] = (nDestSeg < 0) || gameData.segs.SegVis (nLightSeg, nDestSeg);
#endif
else
	return 0;

	fix xDistance, xRad;

if ((nLightSeg < 0) || (nDestSeg < 0) || (info.nSide < 0)) {
	CFixVector vLightToPoint = vDestPos - info.vPos;
	xRad = F2X (info.fRad);
	xDistance = vLightToPoint.Mag ();
	xDistance = fix (float (xDistance) / (info.fRange * fRangeMod)) + xDistMod;
	if (xDistance - xRad > xMaxLightRange)
		return 0;
	if (nDestSeg < 0)
		return 1;
	}
else {
	CFloatVector3 v;
	v.Assign (vDestPos);
	float distance = DistToFace (v, nLightSeg, ubyte (info.nSide)) / (info.fRange * fRangeMod) + X2F (xDistMod);
	if (distance > X2F (xMaxLightRange))
		return 0;
	xDistance = F2X (distance);
	xRad = 0;
	}
if (nLightSeg == nDestSeg)
	info.bDiffuse [nThread] = 1;
else { // check whether light only contributes ambient light to point
	int bDiffuse = info.bDiffuse [nThread] && SeesPoint (nDestSeg, vNormal, &vDestPos, gameOpts->render.nLightmapPrecision, nThread);
	if (nDestSeg >= 0) {
		int bSeesPoint = (info.nSide < 0) ? bDiffuse : SEGMENTS [nLightSeg].Side (info.nSide)->SeesPoint (vDestPos, nDestSeg, bDiffuse ? gameOpts->render.nLightmapPrecision : -gameOpts->render.nLightmapPrecision - 1, nThread);
		info.bDiffuse [nThread] = bDiffuse && bSeesPoint;

		// if point is occluded, use segment path distance to point for light range and attenuation
		// if bDiffuse == 0 then point is completely occluded (determined by above call to SeesPoint ()), otherwise use SeesPoint() to test occlusion
		if (!bSeesPoint) { // => ambient contribution only
			fix xPathLength = LightPathLength (nLightSeg, nDestSeg, vDestPos, xMaxLightRange, 1, nThread);
			if (xPathLength < 0)
				return 0;
#if 0
			if (xPathLength > xMaxLightRange)
				return 0;
			xDistance = xPathLength;
#else
			if (xDistance < xPathLength) {
				// since the path length goes via segment centers and is therefore usually to great, adjust it a bit
#if 1
				xDistance = (xDistance + xPathLength) / 2; 
#else
				xDistance = (xDistance + 2 * xPathLength) / 3; 
#endif
				if (xDistance - xRad > xMaxLightRange)
					return 0;
				}
			}
		}
#endif
	}
#if DBG
if ((nDbgSeg == nDestSeg) && ((nDbgSide < 0) || (nDestSide == nDbgSide)) && info.bDiffuse [nThread])
	nDbgSeg = nDbgSeg;
#endif
render.xDistance [nThread] = xDistance;
return 1;
}

//------------------------------------------------------------------------------

void CLightManager::Sort (void)
{
CQuickSort<CDynLight> qs;
qs.SortAscending (m_data.lights.Buffer (), 0, m_data.nLights [0] - 1);
}

//------------------------------------------------------------------------------

void CLightManager::AddGeometryLights (void)
{
	int			nFace, nSegment, nSide, nTexture, nLight;
	CSegFace*	faceP;
	tFaceColor*	colorP;

#if 0
for (nTexture = 0; nTexture < 910; nTexture++)
	nLight = IsLight (nTexture);
#endif
gameStates.render.bHaveDynLights = 1;
#if 0
if (gameStates.app.bD1Mission)
	gameData.render.fAttScale [0] *= 2;
#endif
ogl.m_states.fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightRange];
m_headlights.Init ();
if (gameStates.render.nLightingMethod)
	gameData.render.color.vertices.Clear ();
m_data.Init ();
for (nFace = gameData.segs.nFaces, faceP = FACES.faces.Buffer (); nFace; nFace--, faceP++) {
	nSegment = faceP->m_info.nSegment;
	if (SEGMENTS [nSegment].m_function == SEGMENT_FUNC_SKYBOX)
		continue;
#if DBG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	nSide = faceP->m_info.nSide;
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	nTexture = faceP->m_info.nBaseTex;
	if ((nTexture < 0) || (nTexture >= MAX_WALL_TEXTURES))
		continue;
	colorP = gameData.render.color.textures + nTexture;
	if ((nLight = IsLight (nTexture)))
		Add (faceP, &colorP->color, nLight, (short) nSegment, (short) nSide, -1, nTexture, NULL, 1);
	faceP->m_info.nOvlTex = SEGMENTS [faceP->m_info.nSegment].Side (faceP->m_info.nSide)->m_nOvlTex;
	nTexture = faceP->m_info.nOvlTex;
#if 0//DBG
	if (gameStates.app.bD1Mission && (nTexture == 289)) //empty, light
		continue;
#endif
	if ((nTexture > 0) && (nTexture < MAX_WALL_TEXTURES) && (nLight = IsLight (nTexture)) /*gameData.pig.tex.info.fBrightness [nTexture]*/) {
		colorP = gameData.render.color.textures + nTexture;
		Add (faceP, &colorP->color, nLight, (short) nSegment, (short) nSide, -1, nTexture, NULL);
		}
	//if (m_data.nLights [0])
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
	lightManager.ResetActive (nThread, 0);
	lightManager.ResetAllUsed (0, nThread);
	lightManager.SetNearestToVertex (-1, -1, nVertex, NULL, 1, 1, bColorize, nThread);
	gameData.render.color.vertices [nVertex].index = 0;
	G3VertexColor (-1, -1, nVertex, gameData.segs.points [nVertex].GetNormal ()->XYZ (), vVertex.XYZ (), pf, NULL, 1, 0, nThread);
	}
}

//------------------------------------------------------------------------------

#if DBG
extern int nDbgVertex;
#endif

void CLightManager::GatherStaticLights (int nLevel)
{
//if (gameStates.app.bNostalgia)
//	return;

int i, j, bColorize = !gameStates.render.nLightingMethod;

PrintLog ("Computing static lighting\n");
gameData.render.vertColor.bDarkness = IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [IsMultiGame].bDarkness;
gameStates.render.nState = 0;
m_data.renderLights.Clear ();
for (i = 0; i < MAX_THREADS; i++)
	m_data.active [i].Clear (0);
Transform (1, bColorize);
for (i = 0; i < gameData.segs.nVertices; i++) {
#if DBG
	if (i == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	m_data.variableVertLights [i] = VariableVertexLights (i);
	}
if (gameStates.render.bPerPixelLighting/* && lightmapManager.HaveLightmaps ()*/) {
	gameData.render.color.ambient.Clear ();
	return;
	}
if (gameStates.render.nLightingMethod || (gameStates.render.bAmbientColor && !gameStates.render.bColored)) {
		tFaceColor*	pfh, *pf = gameData.render.color.ambient.Buffer ();
		CSegment*	segP;

	memset (pf, 0, gameData.segs.nVertices * sizeof (*pf));
#if USE_OPENMP > 1
	if (gameStates.app.bMultiThreaded) {
		int nStart, nEnd;
#	pragma omp parallel
		{
#		pragma omp for private (nStart, nEnd)
			for (i = 0; i < gameStates.app.nThreads; i++) {
				ComputeThreadRange (i, gameData.segs.nVertices, nStart, nEnd);
				lightManager.GatherStaticVertexLights (nStart, nEnd, i);
				}
			}
		}
	else
#else
	//if (!RunRenderThreads (rtStaticVertLight))
#endif
		lightManager.GatherStaticVertexLights (0, gameData.segs.nVertices, 0);
	pf = gameData.render.color.ambient.Buffer ();
	for (i = 0, segP = SEGMENTS.Buffer (); i < gameData.segs.nSegments; i++, segP++) {
		if (segP->m_function == SEGMENT_FUNC_SKYBOX) {
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

int CLightManager::SetMethod (void)
{
gameStates.render.nLightingMethod = (gameStates.app.bNostalgia > 1) ? 0 : gameOpts->render.nLightingMethod;
if (gameStates.render.nLightingMethod && (ogl.m_features.bPerPixelLighting < 2)) {
	gameStates.render.nLightingMethod = 1;
	if (!ogl.m_features.bPerPixelLighting)
		gameOpts->render.bUseLightmaps = 0;
	}
if (!ogl.m_features.bShaders) {
	if (gameOpts->render.nLightingMethod > 1)
		gameOpts->render.nLightingMethod = 1;
	gameOpts->render.bUseLightmaps = 0;
	}
if (gameStates.render.nLightingMethod == 2) {
	gameStates.render.bPerPixelLighting = 2;
	if (gameOpts->ogl.nMaxLightsPerPass < 4)
		gameOpts->ogl.nMaxLightsPerPass = 4;
	else if (gameOpts->ogl.nMaxLightsPerPass > 8)
		gameOpts->ogl.nMaxLightsPerPass = 8;
	gameStates.render.nMaxLightsPerPass = gameOpts->ogl.nMaxLightsPerPass;
#if 1
	gameOpts->ogl.nMaxLightsPerFace = 3 * gameStates.render.nMaxLightsPerPass;
#else
	int nPasses = (16 + gameStates.render.nMaxLightsPerPass - 1) / gameStates.render.nMaxLightsPerPass;
	gameOpts->ogl.nMaxLightsPerFace = nPasses * gameStates.render.nMaxLightsPerPass;
#endif
	gameStates.render.nMaxLightsPerFace = gameOpts->ogl.nMaxLightsPerFace;
	}
else if ((gameStates.render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps)
	gameStates.render.bPerPixelLighting = 1;
else
	gameStates.render.bPerPixelLighting = 0;
gameStates.render.nMaxLightsPerObject = gameOpts->ogl.nMaxLightsPerObject;
gameStates.render.bAmbientColor = /*gameStates.render.bPerPixelLighting ||*/ (gameOpts->render.color.nLevel == 2);
return gameStates.render.bPerPixelLighting;
}

// ----------------------------------------------------------------------------------------------

void CLightManager::Setup (int nLevel)
{
SetMethod ();
//if (!gameStates.app.bNostalgia) 
	{
	gameData.render.fBrightness = 1.0f;
	messageBox.Show (TXT_PREPARING);
	lightManager.AddGeometryLights ();
	ComputeNearestLights (nLevel);
	lightmapManager.Setup (nLevel);
	messageBox.Clear ();
	}
GatherStaticLights (nLevel);
}

// ----------------------------------------------------------------------------------------------
//eof
