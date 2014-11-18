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

void CLightManager::SetColor (int16_t nLight, float red, float green, float blue, float fBrightness)
{
	CDynLight*	lightP = m_data.lights + nLight;
	int32_t			i;

if ((lightP->info.nType == 2) ? (gameOpts->render.color.nLevel >= 1) : (gameOpts->render.color.nLevel == 2)) {
	lightP->info.color.Red () = red;
	lightP->info.color.Green () = green;
	lightP->info.color.Blue () = blue;
	}
else if (gameStates.app.bNostalgia) {
	lightP->info.color.Red () =
	lightP->info.color.Green () =
	lightP->info.color.Blue () = 1.0f;
	}
else {
	lightP->info.color.Red () =
	lightP->info.color.Green () =
	lightP->info.color.Blue () = red * 0.30f + green * 0.584f + blue * 0.116f; //(red + green + blue) / 3.0f;
	}
lightP->info.color.Alpha () = 1.0;
lightP->info.fBrightness = fBrightness;
lightP->info.fRange = (float) sqrt (fBrightness / 2.0f);
lightP->fSpecular.Red () = red;
lightP->fSpecular.Green () = green;
lightP->fSpecular.Blue () = blue;
for (i = 0; i < 3; i++) {
#if USE_OGL_LIGHTS
	lightP->info.fAmbient.dir [i] = lightP->info.fDiffuse [i] * 0.01f;
	lightP->info.fDiffuse.dir [i] =
#endif
	lightP->fEmissive.v.vec [i] = lightP->fSpecular.v.vec [i] * fBrightness;
	}
// light alphas
#if USE_OGL_LIGHTS
lightP->info.fAmbient.dir [3] = 1.0f;
lightP->info.fDiffuse.dir [3] = 1.0f;
lightP->fSpecular.dir [3] = 1.0f;
glLightfv (lightP->info.handle, GL_AMBIENT, lightP->info.fAmbient);
glLightfv (lightP->info.handle, GL_DIFFUSE, lightP->info.fDiffuse);
glLightfv (lightP->info.handle, GL_SPECULAR, lightP->fSpecular);
glLightf (lightP->info.handle, GL_SPOT_EXPONENT, 0.0f);
#endif
}

// ----------------------------------------------------------------------------------------------

void CLightManager::SetPos (int16_t nObject)
{
if (SHOW_DYN_LIGHT) {
	int32_t	nLight = m_data.owners [nObject];
	if (nLight >= 0)
		m_data.lights [nLight].info.vPos = OBJECTS [nObject].info.position.vPos;
	}
}

//------------------------------------------------------------------------------

int16_t CLightManager::Find (int16_t nSegment, int16_t nSide, int16_t nObject)
{
//if (gameStates.render.nLightingMethod && !gameStates.app.bNostalgia) 
	{
	CDynLight*	lightP = &m_data.lights [0];

	if (nObject >= 0)
		return m_data.owners.Buffer () ? m_data.owners [nObject] : -1;
	if (nSegment >= 0)
		for (int16_t i = 0; i < m_data.nLights [0]; i++, lightP++)
			if ((lightP->info.nSegment == nSegment) && (lightP->info.nSide == nSide))
				return i;
	}
return -1;
}

//------------------------------------------------------------------------------

bool CLightManager::Ambient (int16_t nSegment, int16_t nSide)
{
	int16_t i = Find (nSegment, nSide, -1);

return (i < 0) ? false : m_data.lights [i].info.bAmbient != 0;
}

//------------------------------------------------------------------------------

int16_t CLightManager::Update (CFloatVector *colorP, float fBrightness, int16_t nSegment, int16_t nSide, int16_t nObject)
{
	int16_t	nLight = Find (nSegment, nSide, nObject);

#if DBG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	BRP;
if ((nDbgObj >= 0) && (nObject == nDbgObj))
	BRP;
#endif

if (nLight >= 0) {
	CDynLight* lightP = m_data.lights + nLight;
	if (!colorP)
		colorP = &lightP->info.color;
	if (nObject >= 0)
		lightP->info.vPos = OBJECTS [nObject].info.position.vPos;
	if ((lightP->info.fBrightness != fBrightness) ||
		 (lightP->info.color.Red () != colorP->Red ()) || (lightP->info.color.Green () != colorP->Green ()) || (lightP->info.color.Blue () != colorP->Blue ())) {
		SetColor (nLight, colorP->Red (), colorP->Green (), colorP->Blue (), fBrightness);
		}
	}
return nLight;
}

//------------------------------------------------------------------------------

int32_t CLightManager::LastEnabled (void)
{
	int16_t	i = m_data.nLights [0];

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
	pl1->handle = (uint32_t) (GL_LIGHT0 + (pl1 - m_data.lights [0]));
	pl2->handle = (uint32_t) (GL_LIGHT0 + (pl2 - m_data.lights [0]));
#endif
	if (pl1->info.nObject >= 0)
		m_data.owners [pl1->info.nObject] =	int16_t (m_data.lights.Index (pl1));
	if (pl2->info.nObject >= 0)
		m_data.owners [pl2->info.nObject] =	int16_t (m_data.lights.Index (pl2));
	}
}

//------------------------------------------------------------------------------

int32_t CLightManager::Toggle (int16_t nSegment, int16_t nSide, int16_t nObject, int32_t bState)
{
	int16_t nLight = Find (nSegment, nSide, nObject);

if (nLight >= 0) {
	CDynLight* lightP = m_data.lights + nLight;
	lightP->info.bOn = bState;
	lightP->info.bState = 1;
	}
return nLight;
}

//------------------------------------------------------------------------------

void CLightManager::Register (CFaceColor *colorP, int16_t nSegment, int16_t nSide)
{
#if 0
if (!colorP || colorP->index) {
	tLightInfo	*pli = gameData.render.shadows.lightInfo + gameData.render.shadows.nLights++;
#if DBG
	CAngleVector	vec;
#endif
	pli->nIndex = (int32_t) nSegment * 6 + nSide;
	pli->coord = SEGMENTS [nSegment].m_SideCenter (nSide);
	OOF_VecVms2Gl (pli->glPos, &pli->coord);
	pli->nSegNum = nSegment;
	pli->nSideNum = (uint8_t) nSide;
#if DBG
	VmExtractAnglesVector (&vec, SEGMENTS [nSegment].m_sides [nSide].m_normals);
	VmAngles2Matrix (&pli->position.mOrient, &vec);
#endif
	}
#endif
}

//-----------------------------------------------------------------------------

int32_t CLightManager::IsTriggered (int16_t nSegment, int16_t nSide, bool bOppSide)
{
if ((nSegment < 0) || (nSide < 0))
	return 0;

	CTrigger*	trigP;
	int32_t			i = 0;
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
int16_t nConnSeg = SEGMENTS [nSegment].m_children [nSide];
return (nConnSeg >= 0) && IsTriggered (nConnSeg, SEGMENTS [nSegment].ConnectedSide (SEGMENTS + nConnSeg), true);
}

//-----------------------------------------------------------------------------

int32_t CLightManager::IsFlickering (int16_t nSegment, int16_t nSide)
{
	CVariableLight* flP;

if ((flP = gameData.render.lights.flicker.Buffer ()))
	for (int32_t l = gameData.render.lights.flicker.Length (); l; l--, flP++)
		if ((flP->m_nSegment == nSegment) && (flP->m_nSide == nSide))
			return 1;
return 0;
}

//-----------------------------------------------------------------------------

int32_t CLightManager::IsDestructible (int16_t nTexture)
{
if (!nTexture)
	return 0;
if (IsMultiGame && netGameInfo.m_info.bIndestructibleLights)
	return 0;
int16_t nClip = gameData.pig.tex.tMapInfoP [nTexture].nEffectClip;
tEffectClip	*ecP = (nClip < 0) ? NULL : gameData.effects.effectP + nClip;
int16_t	nDestBM = ecP ? ecP->nDestBm : -1;
uint8_t	bOneShot = ecP ? (ecP->flags & EF_ONE_SHOT) != 0 : 0;
if (nClip == -1)
	return gameData.pig.tex.tMapInfoP [nTexture].destroyed != -1;
return (nDestBM != -1) && !bOneShot;
}

//------------------------------------------------------------------------------

static inline float Intensity (float r, float g, float b)
{
   float i = 0;
   int32_t   j = 0;

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

int32_t CLightManager::Add (CSegFace* faceP, CFloatVector *colorP, fix xBrightness, int16_t nSegment,
									 int16_t nSide, int16_t nObject, int16_t nTexture, CFixVector *vPos, uint8_t bAmbient)
{
	int16_t			h, i;
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
	BRP;
if ((nDbgObj >= 0) && (nObject == nDbgObj))
	BRP;
if (colorP && ((colorP->Red () > 1) || (colorP->Green () > 1) || (colorP->Blue () > 1)))
	BRP;
#endif

if (gameStates.render.nLightingMethod && (nSegment >= 0) && (nSide >= 0)) {
#if 1
	fBrightness /= Intensity (colorP->Red (), colorP->Green (), colorP->Blue ());
#else
	if (fBrightness < 1)
		fBrightness = (float) sqrt (fBrightness);
	else
		fBrightness *= fBrightness;
#endif
	}
if (colorP)
	colorP->Alpha () = 1.0f;
if (0 <= (h = Update (colorP, fBrightness, nSegment, nSide, nObject)))
	return h;
if (!colorP)
	return -1;
if ((colorP->Red () == 0.0f) && (colorP->Green () == 0.0f) && (colorP->Blue () == 0.0f)) {
	if (gameStates.app.bD2XLevel && gameStates.render.bColored)
		return -1;
	colorP->Red () = colorP->Green () = colorP->Blue () = 1.0f;
	}
if (m_data.nLights [0] >= MAX_OGL_LIGHTS) {
	gameStates.render.bHaveDynLights = 0;
	return -1;	//too many lights
	}
i = m_data.nLights [0]; //LastEnabledDynLight () + 1;
CDynLight& light = m_data.lights [i];
light.info.nIndex = i;
light.info.faceP = faceP;
light.info.nSegment = nSegment;
light.info.nSide = nSide;
light.info.nObject = nObject;
light.info.nPlayer = -1;
light.info.bState = 1;
light.info.bSpot = 0;
light.info.fBoost = 0;
light.info.bPowerup = 0;
light.info.bAmbient = bAmbient;
light.info.bSelf = 0;
//0: static light
//2: object/lightning
//3: headlight
if (nObject >= 0) {
	CObject *objP = OBJECTS + nObject;
	//HUDMessage (0, "Adding object light %d, type %d", m_data.nLights [0], objP->info.nType);
	light.info.nType = 2;
	light.info.bVariable = 1;
	if (objP->info.nType == OBJ_POWERUP) {
		int32_t id = objP->info.nId;
		if ((id == POW_EXTRA_LIFE) || (id == POW_ENERGY) || (id == POW_SHIELD_BOOST) ||
			 (id == POW_HOARD_ORB) || (id == POW_MONSTERBALL) || (id == POW_INVUL))
			light.info.bPowerup = 1;
		else
			light.info.bPowerup = 2;
#if 0
		if (light.info.bPowerup > gameData.render.nPowerupFilter)
			return -1;
#endif
		}
	light.info.vPos = objP->info.position.vPos;
	light.info.fRad = 0; //X2F (OBJECTS [nObject].size) / 2;
	if (fBrightness > 1) {
		if ((objP->info.nType == OBJ_FIREBALL) || (objP->info.nType == OBJ_EXPLOSION)) {
			light.info.fBoost = 1;
			light.info.fRad = fBrightness;
			}
		else if ((objP->info.nType == OBJ_WEAPON) && (objP->info.nId == FLARE_ID)) {
			light.info.fBoost = 1;
			light.info.fRad = 2 * fBrightness;
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
		light.info.nType = 2;
		light.info.bVariable = 1;
		light.info.fRad = 0;
		if (vPos)
			light.info.vPos = *vPos;
		else
			light.info.vPos = SEGMENTS [nSegment].Center ();
		}
	else {
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			BRP;
#endif
		light.info.nType = 0;
		light.info.bSelf = SEGMENTS [nSegment].HasOutdoorsProp ();
		light.info.fRad = faceP ? faceP->m_info.fRads [1] / 2.0f : 0;
		//RegisterLight (NULL, nSegment, nSide);
		light.info.bVariable = IsDestructible (nTexture) || IsFlickering (nSegment, nSide) || IsTriggered (nSegment, nSide) || SEGMENTS [nSegment].Side (nSide)->IsVolatile ();
		m_data.nVariable += light.info.bVariable;
		CSide* sideP = SEGMENTS [nSegment].m_sides + nSide;
		light.info.vPos = sideP->Center ();
		CFixVector vOffs = sideP->m_normals [2];
		light.info.vDirf.Assign (vOffs);
		CFloatVector::Normalize (light.info.vDirf);
#if 0
		if (gameStates.render.bPerPixelLighting) {
			vOffs *= I2X (1) / 64;
			light.info.vPos += vOffs;
			}
#endif
		}
	}
else {
	light.info.nType = 3;
	light.info.bVariable = 0;
	}
light.info.bOn = 1;
light.bTransform = 1;
SetColor (m_data.nLights [0], colorP->Red (), colorP->Green (), colorP->Blue (), fBrightness);
return m_data.nLights [0]++;
}

//------------------------------------------------------------------------------

bool CLightManager::DeleteFromList (CDynLight* lightP, int16_t nLight)
{
if (!m_data.lights.Buffer () || (nLight >= m_data.nLights [0]))
	return false;
lightP->Destroy ();
*lightP = m_data.lights [--m_data.nLights [0]];
if (lightP->info.nObject >= 0)
	m_data.owners [lightP->info.nObject] = nLight;
if (lightP->info.nPlayer < MAX_PLAYERS)
	m_headlights.lightIds [lightP->info.nPlayer] = nLight;
return true;
}

//------------------------------------------------------------------------------

void CLightManager::Delete (int16_t nLight)
{
if ((nLight >= 0) && (nLight < m_data.nLights [0])) {
	CDynLight* lightP = m_data.lights + nLight;
#if DBG
	if ((nDbgSeg >= 0) && (nDbgSeg == lightP->info.nSegment) && ((nDbgSide < 0) || (nDbgSide == lightP->info.nSide)))
		BRP;
#endif
	if (!lightP->info.nType) {
		// do not remove static lights, or the nearest lights to segment info will get messed up!
		lightP->info.bState = lightP->info.bOn = 0;
		return;
		}
	DeleteFromList (lightP, nLight);
	}
}

//------------------------------------------------------------------------------

int32_t CLightManager::Delete (int16_t nSegment, int16_t nSide, int16_t nObject)
{
	int32_t nLight = Find (nSegment, nSide, nObject);

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
	CDynLight* lightP = &m_data.lights [0];

if (lightP != NULL) {
	for (int16_t i = 0; i < m_data.nLights [0]; ) {
		if (!((lightP->info.nSegment >= 0) && (lightP->info.nSide < 0) && DeleteFromList (lightP, i))) {
			i++;
			lightP++;
			}
		}
	}
}

//------------------------------------------------------------------------------

void CLightManager::Reset (void)
{
for (int16_t i = 0; i < m_data.nLights [0]; i++)
	Delete (i);
}

//------------------------------------------------------------------------------

void CLightManager::SetMaterial (int16_t nSegment, int16_t nSide, int16_t nObject)
{
	//static float fBlack [4] = {0.0f, 0.0f, 0.0f, 1.0f};
	int32_t nLight = Find (nSegment, nSide, nObject);

if (nLight >= 0) {
	CDynLight* lightP = m_data.lights + nLight;
	if (lightP->info.bState) {
		m_data.material.emissive = *reinterpret_cast<CFloatVector*> (&lightP->fEmissive);
		m_data.material.specular = *reinterpret_cast<CFloatVector*> (&lightP->fEmissive);
		m_data.material.shininess = 8;
		m_data.material.bValid = 1;
		m_data.material.nLight = nLight;
		return;
		}
	}
m_data.material.bValid = 0;
}

//------------------------------------------------------------------------------

int32_t CDynLight::Compare (CDynLight &other)
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

int32_t CDynLight::LightPathLength (const int16_t nLightSeg, const int16_t nDestSeg, const CFixVector& vDestPos, fix xMaxLightRange, int32_t bFastRoute, int32_t nThread)
{
fix dist = (bFastRoute) 
			  ? gameData.segs.SegDist (nLightSeg, nDestSeg)
			  : simpleRouter [nThread].PathLength (info.vPos, nLightSeg, vDestPos, nDestSeg, X2I (xMaxLightRange / 5), WID_TRANSPARENT_FLAG | WID_PASSABLE_FLAG, 0);
if (dist < 0)
	return -1;
return fix (dist / info.fRange);
}

//------------------------------------------------------------------------------
// Check whether a point can be seen from a light emitting face. Returns:
// -1: Point is behind light face or light face is on rear side of point
//  0: Point is occluded by geometry or too far away
//  1: Point is visible

#define LIGHTING_LEVEL 1

int32_t CDynLight::SeesPoint (const int16_t nDestSeg, const CFixVector* vNormal, CFixVector* vPoint, int32_t nLevel, int32_t nThread)
{
#if 1
int32_t nLightSeg = info.nSegment;
#else
int32_t nLightSeg = LightSeg ();
#endif

		CFloatVector	v0, v1, vLightToPointf, vNormalf;

#if FAST_POINTVIS

if (info.nSide < 0) {
	if (nLightSeg < 0)
		return 1;
	v1.Assign (*vPoint);
	v0.Assign (info.vPos);
	vLightToPointf = v1 - v0;
	float dist = CFloatVector::Normalize (vLightToPointf);
	if (dist > info.fRange * X2F (MAX_LIGHT_RANGE))
		return 0;
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
		static int32_t nLevels [3] = {4, 0, -4};

		CSide*	sideP = SEGMENTS [nLightSeg].Side (info.nSide);
		int32_t		i, j = nLevels [nLevel];

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
		BRP;
#endif
	}

#else

CHitQuery fq (FQ_TRANSWALL | FQ_TRANSPOINT | FQ_VISIBILITY, &info.vPos, vPoint, nLightSeg, -1, 1, 0);
CHitResult	hitResult;
int32_t nHitType = FindHitpoint (&fq, &hitResult);
return (!nHitType || ((nHitType == HIT_WALL) && (hitResult.nSegment == nDestSeg)));

#endif
}

//------------------------------------------------------------------------------

int32_t CDynLight::SeesPoint (const int16_t nSegment, const int16_t nSide, CFixVector* vPoint, const int32_t nLevel, int32_t nThread)
{
return SeesPoint (nSegment, &SEGMENTS [nSegment].Side (nSide)->Normal (2), vPoint, nLevel, nThread);
}

//------------------------------------------------------------------------------

int32_t CDynLight::LightSeg (void)
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
return &PLAYEROBJECT (info.nPlayer);
}

//------------------------------------------------------------------------------

int32_t CDynLight::Contribute (const int16_t nDestSeg, const int16_t nDestSide, const int16_t nDestVertex, CFixVector& vDestPos, const CFixVector* vNormal, 
										 fix xMaxLightRange, float fRangeMod, fix xDistMod, int32_t nThread)
{
	int16_t nLightSeg = info.nSegment;

if ((nLightSeg >= 0) && (nDestSeg >= 0)) {
#if DBG
	if ((nLightSeg == nDbgSeg) && ((nDbgSide < 0) || (nDbgSide == info.nSide)))
		BRP;
#endif
	if (info.nSide < 0) 
		info.bDiffuse [nThread] = gameData.segs.SegVis (nLightSeg, nDestSeg);
	else {
#if DBG
		if (nDestSeg == nDbgSeg)
			BRP;
#endif
		if (SEGMENTS [nLightSeg].HasOutdoorsProp ()) {
			if ((nDestSide >= 0) && ((info.nSide != nDestSide) || (nLightSeg != nDestSeg)))
				return 0;
			info.bDiffuse [nThread] = 1;
			}
		else if (0 > (info.bDiffuse [nThread] = gameData.segs.LightVis (Index (), nDestSeg)))
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
	CFloatVector3 v, vHit;
	v.Assign (vDestPos);
	DistToFace (v, nLightSeg, uint8_t (info.nSide), &vHit);
	float distance = CFloatVector3::Dist (v, vHit) / (info.fRange * fRangeMod) + X2F (xDistMod);
	if (distance > X2F (xMaxLightRange))
		return 0;
	xDistance = F2X (distance);
	xRad = 0;
	}
if (nLightSeg == nDestSeg)
	info.bDiffuse [nThread] = 1;
else if (info.bVariable && (nDestVertex >= 0)) {
	if (info.visibleVertices && info.visibleVertices->Buffer ())
		info.bDiffuse [nThread] = (*info.visibleVertices) [nDestVertex];
	else
		info.bDiffuse [nThread] = SeesPoint (nDestSeg, vNormal, &vDestPos, gameOpts->render.nLightmapPrecision, nThread);
	}
else { // check whether light only contributes ambient light to point
	int32_t bDiffuse = info.bDiffuse [nThread] && SeesPoint (nDestSeg, vNormal, &vDestPos, gameOpts->render.nLightmapPrecision, nThread);
	if (nDestSeg >= 0) {
		int32_t bSeesPoint = (info.nSide < 0) 
									? bDiffuse 
									: SEGMENTS [nLightSeg].Side (info.nSide)->SeesPoint (vDestPos, nDestSeg, bDiffuse ? gameOpts->render.nLightmapPrecision : -gameOpts->render.nLightmapPrecision - 1, nThread);
		info.bDiffuse [nThread] = bDiffuse && bSeesPoint;

		// if point is occluded, use segment path distance to point for light range and attenuation
		// if bDiffuse == 0 then point is completely occluded (determined by above call to SeesPoint ()), otherwise use SeesPoint() to test occlusion
		if (!bSeesPoint) { // => ambient contribution only
			fix xPathLength = LightPathLength (nLightSeg, nDestSeg, vDestPos, xMaxLightRange, 1, nThread);
			if (xPathLength < 0)
				return 0;
			if (xDistance < xPathLength) {
				// since the path length goes via segment centers and is therefore usually to great, adjust it a bit
				xDistance = (xDistance + xPathLength) / 2; 
				if (xDistance - xRad > xMaxLightRange)
					return 0;
				}
			}
		}
	}
#if DBG
if ((nDbgSeg == nDestSeg) && ((nDbgSide < 0) || (nDestSide == nDbgSide)) && info.bDiffuse [nThread])
	BRP;
#endif
render.xDistance [nThread] = xDistance;
return 1;
}

//------------------------------------------------------------------------------

int32_t CDynLight::ComputeVisibleVertices (int32_t nThread)
{
if (!info.bVariable)
	return 0;
int16_t nLightSeg = LightSeg ();
if ((nLightSeg < 0) || (info.nSide < 0))
	return 0;
if (!(info.visibleVertices || (info.visibleVertices = new CByteArray ())))
	return -1;
if (!info.visibleVertices->Create (gameData.segs.nVertices))
	return -1;
CSide* sideP = SEGMENTS [nLightSeg].Side (info.nSide);
for (int32_t i = 0; i < gameData.segs.nVertices; i++)
	(*info.visibleVertices) [i] = sideP->SeesPoint (VERTICES [i], -1, 2, nThread);
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CLightManager::Sort (void)
{
CQuickSort<CDynLight> qs;
qs.SortAscending (m_data.lights.Buffer (), 0, m_data.nLights [0] - 1);
for (int32_t i = 0; i < m_data.nLights [0]; i++)
	m_data.lights [i].info.nIndex = i;
}

//------------------------------------------------------------------------------

void CLightManager::AddGeometryLights (void)
{
	int32_t			nFace, nSegment, nSide, nTexture, nLight;
	CSegFace*	faceP;
	CFaceColor*	colorP;

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
for (nFace = FACES.nFaces, faceP = FACES.faces.Buffer (); nFace; nFace--, faceP++) {
	nSegment = faceP->m_info.nSegment;
	if (SEGMENTS [nSegment].m_function == SEGMENT_FUNC_SKYBOX)
		continue;
#if DBG
	if (nSegment == nDbgSeg)
		BRP;
#endif
	nSide = faceP->m_info.nSide;
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
		BRP;
#endif
	nTexture = faceP->m_info.nBaseTex;
	if ((nTexture < 0) || (nTexture >= MAX_WALL_TEXTURES))
		continue;
	colorP = gameData.render.color.textures + nTexture;
	if ((nLight = IsLight (nTexture)))
		Add (faceP, colorP, nLight, (int16_t) nSegment, (int16_t) nSide, -1, nTexture, NULL, 1);
	faceP->m_info.nOvlTex = SEGMENTS [faceP->m_info.nSegment].Side (faceP->m_info.nSide)->m_nOvlTex;
	nTexture = faceP->m_info.nOvlTex;
#if 0//DBG
	if (gameStates.app.bD1Mission && (nTexture == 289)) //empty, light
		continue;
#endif
	if ((nTexture > 0) && (nTexture < MAX_WALL_TEXTURES) && (nLight = IsLight (nTexture)) /*gameData.pig.tex.info.fBrightness [nTexture]*/) {
		colorP = gameData.render.color.textures + nTexture;
		Add (faceP, colorP, nLight, (int16_t) nSegment, (int16_t) nSide, -1, nTexture, NULL);
		}
	//if (m_data.nLights [0])
	//	return;
	if (!gameStates.render.bHaveDynLights) {
		Reset ();
		return;
		}
	}
Sort ();
}

//------------------------------------------------------------------------------

void CLightManager::GatherStaticVertexLights (int32_t nVertex, int32_t nMax, int32_t nThread)
{
	CFaceColor*		pf = gameData.render.color.ambient + nVertex;
	CFloatVector	vVertex;
	int32_t				bColorize = !gameStates.render.nLightingMethod;

for (; nVertex < nMax; nVertex++, pf++) {
#if DBG
	if (nVertex == nDbgVertex)
		BRP;
#endif
	vVertex.Assign (gameData.segs.vertices [nVertex]);
	lightManager.ResetActive (nThread, 0);
	lightManager.ResetAllUsed (0, nThread);
	lightManager.SetNearestToVertex (-1, -1, nVertex, NULL, 1, 1, bColorize, nThread);
	gameData.render.color.vertices [nVertex].index = 0;
	GetVertexColor (-1, -1, nVertex, RENDERPOINTS [nVertex].GetNormal ()->XYZ (), vVertex.XYZ (), pf, NULL, 1, 0, nThread);
	}
}

//------------------------------------------------------------------------------

#if DBG
extern int32_t nDbgVertex;
#endif

void CLightManager::GatherStaticLights (int32_t nLevel)
{
//if (gameStates.app.bNostalgia)
//	return;

int32_t i, j, bColorize = !gameStates.render.nLightingMethod;

PrintLog (1, "Computing static lighting\n");
gameData.render.vertColor.bDarkness = IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [IsMultiGame].bDarkness;
gameStates.render.nState = 0;
m_data.renderLights.Clear ();
for (i = 0; i < MAX_THREADS; i++)
	m_data.active [i].Clear (0);
Transform (1, bColorize);
for (i = 0; i < gameData.segs.nVertices; i++) {
#if DBG
	if (i == nDbgVertex)
		BRP;
#endif
	m_data.variableVertLights [i] = VariableVertexLights (i);
	if (gameStates.render.nLightingMethod)
		gameData.render.color.ambient [i].Set (0.0f, 0.0f, 0.0f, 1.0f);
	}
if (gameStates.render.bPerPixelLighting/* && lightmapManager.HaveLightmaps ()*/)
	return;
if (gameStates.render.nLightingMethod || (gameStates.render.bAmbientColor && !gameStates.render.bColored)) {
		CFaceColor*	pf = gameData.render.color.ambient.Buffer ();
		CSegment*	segP;

	memset (pf, 0, gameData.segs.nVertices * sizeof (*pf));
#if USE_OPENMP // > 1
	if (gameStates.app.bMultiThreaded) {
		int32_t nStart, nEnd;
#	pragma omp parallel for private (nStart, nEnd)
		for (i = 0; i < gameStates.app.nThreads; i++) {
			ComputeThreadRange (i, gameData.segs.nVertices, nStart, nEnd);
			lightManager.GatherStaticVertexLights (nStart, nEnd, i);
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
			uint16_t* sv = segP->m_vertices;
			for (j = 8; j; j--, sv++)
				if (*sv != 0xFFFF)
					pf [*sv].Set (1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
	}
PrintLog (-1);
}

// ----------------------------------------------------------------------------------------------

int32_t CLightManager::SetMethod (void)
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
	int32_t nPasses = (16 + gameStates.render.nMaxLightsPerPass - 1) / gameStates.render.nMaxLightsPerPass;
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

int32_t CLightManager::Setup (int32_t nLevel)
{
SetMethod ();
gameData.render.fBrightness = 1.0f;
messageBox.Show (TXT_PREPARING);
lightManager.AddGeometryLights ();
m_data.nGeometryLights = m_data.nLights [0];
ComputeNearestLights (nLevel);
lightmapManager.Setup (nLevel);
messageBox.Clear ();
GatherStaticLights (nLevel);
return 1;
}

// ----------------------------------------------------------------------------------------------
//eof
