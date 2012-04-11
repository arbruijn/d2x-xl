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

#define SORT_ACTIVE_LIGHTS 0

//------------------------------------------------------------------------------

void CLightManager::Transform (int bStatic, int bVariable)
{
	int			i;
	CDynLight*	lightP = &m_data.lights [0];

m_data.nLights [1] = 0;
for (i = 0; i < m_data.nLights [0]; i++, lightP++) {
#if DBG
	if ((nDbgSeg >= 0) && (nDbgSeg == lightP->info.nSegment) && ((nDbgSide < 0) || (nDbgSide == lightP->info.nSide)))
		nDbgSeg = nDbgSeg;
	if (i == nDbgLight)
		nDbgLight = nDbgLight;
#endif
	lightP->render.vPosf [0].Assign (lightP->info.vPos);
	if (ogl.m_states.bUseTransform)
		lightP->render.vPosf [1] = lightP->render.vPosf [0];
	else {
		transformation.Transform (lightP->render.vPosf [1], lightP->render.vPosf [0], 0);
		lightP->render.vPosf [1].v.coord.w = 1;
		}
	lightP->render.vPosf [0].v.coord.w = 1;
	lightP->render.nType = lightP->info.nType;
	lightP->render.bState = lightP->info.bState && (lightP->info.color.Red () + lightP->info.color.Green () + lightP->info.color.Blue () > 0.0);
	lightP->render.bLightning = (lightP->info.nObject < 0) && (lightP->info.nSide < 0);
	for (int j = 0; j < gameStates.app.nThreads; j++)
		ResetUsed (lightP, j);
	lightP->render.bShadow =
	lightP->render.bExclusive = 0;
	if (lightP->render.bState) {
		if (!bStatic && (lightP->info.nType == 1) && !lightP->info.bVariable)
			lightP->render.bState = 0;
		if (!bVariable && ((lightP->info.nType > 1) || lightP->info.bVariable))
			lightP->render.bState = 0;
		}
	m_data.renderLights [m_data.nLights [1]++] = lightP;
	}
m_headlights.Prepare ();
m_headlights.Update ();
m_headlights.Transform ();
}

//------------------------------------------------------------------------------

#if SORT_ACTIVE_LIGHTS

static void QSortDynamicLights (int left, int right, int nThread)
{
	CDynLight**	activeLightsP = m_data.active [nThread];
	int			l = left,
					r = right,
					mat = activeLightsP [(l + r) / 2]->xDistance;

do {
	while (activeLightsP [l]->xDistance < mat)
		l++;
	while (activeLightsP [r]->xDistance > mat)
		r--;
	if (l <= r) {
		if (l < r) {
			CDynLight* h = activeLightsP [l];
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

#endif //SORT_ACTIVE_LIGHTS

//------------------------------------------------------------------------------
// SetActive puts lights to be activated in a jump table depending on their 
// distance to the lit object. If a jump table entry is already occupied, the 
// light will be moved back in the table to the next free entry.

int CLightManager::SetActive (CActiveDynLight* activeLightsP, CDynLight* lightP, short nType, int nThread, bool bForce)
{
#if DBG
if ((nDbgSeg >= 0) && (lightP->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (lightP->info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if ((nDbgObj >= 0) && (lightP->info.nObject == nDbgObj))
	nDbgObj = nDbgObj;
#endif

if (lightP->render.bUsed [nThread])
	return 0;
fix xDist;
if (bForce || lightP->info.bSpot) 
	xDist = 0;
else {
	xDist = (lightP->render.xDistance [nThread] / 2000 + 5) / 10;
	if (xDist >= MAX_OGL_LIGHTS)
		return 0;
	if (xDist < 0)
		xDist = 0;
	}
#if PREFER_GEOMETRY_LIGHTS
else if (lightP->info.nSegment >= 0)
	xDist /= 2;
else if (!lightP->info.bSpot)
	xDist += (MAX_OGL_LIGHTS - xDist) / 2;
#endif
activeLightsP += xDist;
while (activeLightsP->nType) {
	if (activeLightsP->lightP == lightP)
		return 0;
	if (++xDist >= MAX_OGL_LIGHTS)
		return 0;
	activeLightsP++;
	}

#if DBG
if ((nDbgSeg >= 0) && (lightP->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (lightP->info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if ((nDbgObj >= 0) && (lightP->info.nObject == nDbgObj))
	nDbgObj = nDbgObj;
#endif

activeLightsP->nType = nType;
activeLightsP->lightP = lightP;
lightP->render.activeLightsP [nThread] = activeLightsP;
lightP->render.bUsed [nThread] = ubyte (nType);

CDynLightIndex* sliP = &m_data.index [0][nThread];

sliP->nActive++;
if (sliP->nFirst > xDist)
	sliP->nFirst = short (xDist);
if (sliP->nLast < xDist)
	sliP->nLast = short (xDist);
return 1;
}

//------------------------------------------------------------------------------

CDynLight* CLightManager::GetActive (CActiveDynLight* activeLightsP, int nThread)
{
	CDynLight*	lightP = activeLightsP->lightP;
#if 0
if (lightP) {
	if (lightP->render.bUsed [nThread] > 1)
		lightP->render.bUsed [nThread] = 0;
	if (activeLightsP->nType > 1) {
		activeLightsP->nType = 0;
		activeLightsP->lightP = NULL;
		m_data.index [0][nThread].nActive--;
		}
	}
#endif
if (lightP == reinterpret_cast<CDynLight*> (0xffffffff))
	return NULL;
return lightP;
}

//------------------------------------------------------------------------------

ubyte CLightManager::VariableVertexLights (int nVertex)
{
	short*		nearestLightP = m_data.nearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	CDynLight*	lightP;
	short			i, j;
	ubyte			h;

#if DBG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
for (h = 0, i = MAX_NEAREST_LIGHTS; i; i--, nearestLightP++) {
	if ((j = *nearestLightP) < 0)
		break;
	if ((lightP = RenderLights (j)))
		h += lightP->info.bVariable;
	}
return h;
}

//------------------------------------------------------------------------------

void CLightManager::SetNearestToVertex (int nSegment, int nSide, int nVertex, CFixVector *vNormal, ubyte nType, int bStatic, int bVariable, int nThread)
{
if (bStatic || m_data.variableVertLights [nVertex]) {
	PROF_START
	short*				nearestLightP = m_data.nearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	CDynLightIndex*	sliP = &m_data.index [0][nThread];
	short					i, j, nActiveLightI = sliP->nActive;
	CDynLight*			lightP;
	CActiveDynLight*	activeLightsP = m_data.active [nThread].Buffer ();
	CFixVector			vVertex = gameData.segs.vertices [nVertex];
	fix					xMaxLightRange = /*(gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 :*/ MAX_LIGHT_RANGE;

#if DBG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
	sliP->iVertex = nActiveLightI;
	for (i = MAX_NEAREST_LIGHTS; i; i--, nearestLightP++) {
		if ((j = *nearestLightP) < 0)
			break;
		if (!(lightP = RenderLights (j)))
			continue;
		if (lightP->render.bUsed [nThread])
			continue;
		if (gameData.threads.vertColor.data.bNoShadow && lightP->render.bShadow)
			continue;
		if (lightP->info.bVariable) {
			if (!(bVariable && lightP->info.bOn))
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
#if 1
		if (!lightP->Contribute (nSegment, nSide, nVertex, vVertex, vNormal, xMaxLightRange, 1.0f, (lightP->info.nSegment >= 0) ? -SEGMENTS [lightP->info.nSegment].AvgRad () : 0, nThread))
			continue;
#else
		CFixVector vLightToVertex = vVertex - lightP->info.vPos;
		fix xLightDist = CFixVector::Normalize (vLightToVertex.Mag ());
		if ((lightP->info.bDiffuse [nThread] = gameData.segs.LightVis (lightP->Index (), nSegment) && lightP->SeesPoint (vNormal, &vLightToVertex)) || (nSegment < 0))
			lightP->render.xDistance [nThread] = fix (xLightDist / lightP->info.fRange);
		else if (nSegment >= 0) {
			lightP->render.xDistance [nThread] = lightP->LightPathLength (nSegment, vVertex);
			if (lightP->render.xDistance [nThread] < 0)
				continue;
			}
		if (lightP->info.nSegment >= 0)
			lightP->render.xDistance [nThread] -= SEGMENTS [lightP->info.nSegment].AvgRad ();
		if (lightP->render.xDistance [nThread] > xMaxLightRange)
			continue;
#endif
		if (SetActive (activeLightsP, lightP, 2, nThread)) {
			lightP->render.nType = nType;
			//lightP->render.bState = 1;
			}
		}
	PROF_END(ptVertexLighting)
	}
}

//------------------------------------------------------------------------------

int CLightManager::SetNearestToFace (CSegFace* faceP, int bTextured)
{
PROF_START
#if 0
	static		int nFrameCount = -1;
if ((faceP == prevFaceP) && (nFrameCount == gameData.app.nFrameCount))
	return m_data.index [0][0].nActive;

prevFaceP = faceP;
nFrameCount = gameData.app.nFrameCount;
#endif
	int			i;
	CFixVector	vNormal;
	CSide*		sideP = SEGMENTS [faceP->m_info.nSegment].m_sides + faceP->m_info.nSide;

#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if (faceP - FACES.faces == nDbgFace)
	nDbgFace = nDbgFace;
#endif
#if 1//!DBG
if (m_data.index [0][0].nActive < 0)
	lightManager.SetNearestToSegment (faceP->m_info.nSegment, faceP - FACES.faces, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
else {
	m_data.index [0][0] = m_data.index [1][0];
	}
#else
lightManager.SetNearestToSegment (faceP->m_info.nSegment, faceP - FACES, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
#endif
vNormal = sideP->m_normals [2];
#if 1
for (i = 0; i < 4; i++)
	lightManager.SetNearestToVertex (-1, -1, faceP->m_info.index [i], &vNormal, 0, 0, 1, 0);
#endif
PROF_END(ptPerPixelLighting)
return m_data.index [0][0].nActive;
}

//------------------------------------------------------------------------------

void CLightManager::SetNearestStatic (int nSegment, int bStatic, int nThread)
{
	static short nActiveLights [4] = {-1, -1, -1, -1};

if (gameStates.render.nLightingMethod) {
	short*				nearestLightP = m_data.nearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	short					i, j;
	CDynLight*			lightP;
	CActiveDynLight*	activeLightsP = m_data.active [nThread].Buffer ();
	fix					xMaxLightRange = SEGMENTS [nSegment].AvgRad () + (/*(gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 :*/ MAX_LIGHT_RANGE);
	CFixVector			c = SEGMENTS [nSegment].Center ();

	//m_data.iStaticLights [nThread] = m_data.index [0][nThread].nActive;
	for (i = MAX_NEAREST_LIGHTS; i; i--, nearestLightP++) {
		if ((j = *nearestLightP) < 0)
			break;
		if (!(lightP = RenderLights (j)))
			continue;
		if (gameData.threads.vertColor.data.bNoShadow && lightP->render.bShadow)
			continue;
		if (lightP->info.bVariable) {
			if (!lightP->info.bOn)
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		lightP->render.xDistance [nThread] = (fix) ((CFixVector::Dist (c, lightP->info.vPos) /*- F2X (lightP->info.fRad)*/) / (lightP->info.fRange * max (lightP->info.fRad, 1.0f)));
		if (lightP->render.xDistance [nThread] > xMaxLightRange)
			continue;
		lightP->info.bDiffuse [nThread] = 1;
		SetActive (activeLightsP, lightP, 3, nThread);
		}
	}
nActiveLights [nThread] = m_data.index [0][nThread].nActive;
}

//------------------------------------------------------------------------------
// Retrieve closest variable (from objects and destructible/flickering geometry) lights.
// If bVariable == 0, only retrieve light emitting objects (will be stored in light buffer
// after all geometry lights).

short CLightManager::SetNearestToSegment (int nSegment, int nFace, int bVariable, int nType, int nThread)
{
PROF_START
	CDynLightIndex*	sliP = &m_data.index [0][nThread];

#if DBG
	static int nPrevSeg = -1;

if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nDbgSeg = nDbgSeg;
#endif
if (gameStates.render.nLightingMethod) {
	ubyte						nType;
	short						i = m_data.nLights [1];
	int						bSkipHeadlight = !gameStates.render.nState && ((gameStates.render.bPerPixelLighting == 2) || gameOpts->ogl.bHeadlight);
	fix						xMaxLightRange = SEGMENTS [nSegment].AvgRad () + (/*(gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 :*/ MAX_LIGHT_RANGE);
	CDynLight*				lightP;
	CFixVector&				vDestPos = SEGMENTS [nSegment].Center ();
	CActiveDynLight*		activeLightsP = m_data.active [nThread].Buffer ();

	lightManager.ResetAllUsed (0, nThread);
	lightManager.ResetActive (nThread, 0);
	while (i) {
		if (!(lightP = RenderLights (--i)))
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (lightP->info.nSegment == nDbgSeg))
			lightP = lightP;
#endif
		if (gameData.threads.vertColor.data.bNoShadow && lightP->render.bShadow)
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (lightP->info.nSegment == nDbgSeg))
			lightP = lightP;
		if ((lightP->info.nSegment >= 0) && (lightP->info.nSide < 0))
			lightP = lightP;
#endif
		nType = lightP->info.nType;
		lightP->info.bDiffuse [nThread] = 1;
		if (nType == 3) {
			if (bSkipHeadlight)
				continue;
			}
		else {
			if (lightP->info.bPowerup > gameData.render.nPowerupFilter)
				continue;
			if (nType < 2) {	// all light emitting objects scanned
				if (!bVariable)
					break;
				if (!(lightP->info.bVariable && lightP->info.bOn))
					continue;
				}
#if DBG
			if (lightP->info.nObject >= 0)
				nDbgObj = nDbgObj;
#endif
			}
#if 1
		if (!lightP->Contribute (nSegment, -1, -1, vDestPos, NULL, xMaxLightRange, max (lightP->info.fRad, 1.0f), 0, nThread))
			continue;
#else
		short nLightSeg = lightP->info.nSegment;
		if (nLightSeg >= 0) 
			lightP->info.bDiffuse [nThread] = (lightP->info.nSide >= 0) 
													 ? gameData.segs.LightVis (lightP->Index (), nSegment) 
													 : gameData.segs.SegVis (lightP->info.nSegment, nSegment);
		else if ((lightP->info.nObject >= 0) && ((lightP->info.nSegment = OBJECTS [lightP->info.nObject].info.nSegment) >= 0))
			lightP->info.bDiffuse [nThread] = gameData.segs.SegVis (lightP->info.nSegment, nSegment);
		else
			continue;
		lightP->render.xDistance [nThread] = (fix) ((float) CFixVector::Dist (vDestPos, lightP->info.vPos) / (lightP->info.fRange * max (lightP->info.fRad, 1.0f)));
		if (lightP->render.xDistance [nThread] > xMaxLightRange)
			continue;
		if (lightP->info.bDiffuse [nThread])
			lightP->info.bDiffuse [nThread] = lightP->SeesPoint (&vDestPos, NULL);
		if (!lightP->info.bDiffuse [nThread]) {
			lightP->render.xDistance [nThread] = lightP->LightPathLength (nSegment, vDestPos);
			if (lightP->render.xDistance [nThread] > xMaxLightRange)
				continue;
			}
#endif
#if DBG
		if (SetActive (activeLightsP, lightP, 1, nThread)) {
			if ((nSegment == nDbgSeg) && (nDbgObj >= 0) && (lightP->info.nObject == nDbgObj))
				lightP = lightP;
			if (nFace < 0)
				lightP->render.nTarget = -nSegment - 1;
			else
				lightP->render.nTarget = nFace + 1;
			lightP->render.nFrame = gameData.app.nFrameCount;
			}
#else
		SetActive (activeLightsP, lightP, 1, nThread);
#endif
		}
	m_data.index [1][nThread] = *sliP;
#if DBG
	if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
		nDbgSeg = nDbgSeg;
#endif
	}
#if DBG
nPrevSeg = nSegment;
#endif
PROF_END(ptSegmentLighting)
return sliP->nActive;
}

//------------------------------------------------------------------------------

short CLightManager::SetNearestToPixel (short nSegment, short nSide, CFixVector *vNormal, CFixVector *vPixelPos, float fLightRad, int nThread)
{
#if DBG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nDbgSeg = nDbgSeg;
#endif
if (gameStates.render.nLightingMethod) {
	short						i, n = m_data.nLights [1];
	fix						xMaxLightRange = F2X (fLightRad) + (/*(gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 :*/ MAX_LIGHT_RANGE);
	CDynLight*				lightP;
	CActiveDynLight*		activeLightsP = m_data.active [nThread].Buffer ();
	bool						bForce, bLight = Find (nSegment, nSide, -1) >= 0;

	ResetActive (nThread, 0);
	ResetAllUsed (0, nThread);
	for (i = 0; i < n; i++) {
		if (!(lightP = RenderLights (i)))
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (lightP->info.nSegment == nDbgSeg))
			lightP = lightP;
#endif
		if (lightP->info.nType)
			break;
		if (lightP->info.bVariable)
			continue;
		if (bLight && !lightP->Illuminate (nSegment, nSide))
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (nDbgSeg == lightP->info.nSegment))
			nDbgSeg = nDbgSeg;
		if ((nDbgSeg >= 0) && (nDbgSeg == nSegment))
			nDbgSeg = nDbgSeg;
#endif
		if (lightP->info.nSegment < 0)
			continue;
		if ((bForce = (lightP->info.nSegment == nSegment) && (lightP->info.nSide == nSide)))
			lightP->info.bDiffuse [nThread] = 1;
#if 1
		else if (!lightP->Contribute (nSegment, nSide, -1, *vPixelPos, &SEGMENTS [nSegment].Normal (nSide, 2), xMaxLightRange, 1.0f, 0, nThread))
			continue;
#else
		else {
			lightP->info.bDiffuse [nThread] = gameData.segs.LightVis (lightP->Index (), nSegment);
			CFixVector vLightToPixel = *vPixelPos - lightP->info.vPos;
			fix xLightDist = CFixVector::Normalize (vLightToPixel);
			lightP->render.xDistance [nThread] = (fix) (float (xLightDist) / lightP->info.fRange);
			if (lightP->render.xDistance [nThread] > xMaxLightRange)
				continue;
			if (lightP->info.bDiffuse [nThread]) {
				vLightToPixel /= xLightDist;
				lightP->info.bDiffuse [nThread] = lightP->SeesPoint (nSegment, nSide, &vLightToPixel);
				}
			if (!lightP->info.bDiffuse [nThread]) {
				lightP->render.xDistance [nThread] = lightP->LightPathLength (lightP->info.nSegment, nSegment, *vPixelPos, xMaxLightRange, 1, nThread);
				if ((lightP->render.xDistance [nThread] < 0) || (lightP->render.xDistance [nThread] > xMaxLightRange))
					continue;
				}
			}
#endif
#if DBG
		if (lightP->info.bDiffuse [nThread]) {
			nDbgSeg = nDbgSeg;
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
			}
#endif
		SetActive (activeLightsP, lightP, 1, nThread, bForce);
		}
	}
return m_data.index [0][nThread].nActive;
}

//------------------------------------------------------------------------------

int CLightManager::SetNearestToSgmAvg (short nSegment, int nThread)
{
	int			i;
	CSegment		*segP = SEGMENTS + nSegment;

#if DBG
if (nSegment == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
#if 1
for (i = 0; i < 8; i++)
	if (segP->m_vertices [i] != 0xFFFF)
		lightManager.SetNearestToVertex (-1, -1, segP->m_vertices [i], NULL, 0, 1, 1, 0);
#endif
return m_data.index [0][0].nActive;
}

//------------------------------------------------------------------------------

CFaceColor* CLightManager::AvgSgmColor (int nSegment, CFixVector *vPosP, int nThread)
{
	CFaceColor	c, *vertColorP, *segColorP = gameData.render.color.segments + nSegment;
	ushort		*pv;
	int			i;
	CFixVector	vCenter, vVertex;
	float			d, ds;

#if DBG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
if (!vPosP && (segColorP->index == (char) (gameData.app.nFrameCount & 0xff)) && (segColorP->Red () + segColorP->Green () + segColorP->Blue () != 0))
	return segColorP;
#if DBG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
nThread = ThreadId (nThread);
if (SEGMENTS [nSegment].m_function == SEGMENT_FUNC_SKYBOX) {
	segColorP->Red () = segColorP->Green () = segColorP->Blue () = segColorP->Alpha () = 1.0f;
	segColorP->index = 1;
	}
else if (gameStates.render.bPerPixelLighting) {
	segColorP->Red () =
	segColorP->Green () =
	segColorP->Blue () = 0;
	segColorP->Alpha () = 1.0f;
	if (SetNearestToSgmAvg (nSegment, nThread)) {
			CVertColorData	vcd;

		InitVertColorData (vcd);
		vcd.vertNorm.SetZero ();
		if (vPosP)
			vcd.vertPos.Assign (*vPosP);
		else {
			vCenter = SEGMENTS [nSegment].Center ();
			vcd.vertPos.Assign (vCenter);
			}
		vcd.vertPosP = &vcd.vertPos;
		vcd.fMatShininess = 4;
		ComputeVertexColor (nSegment, -1, -1, reinterpret_cast<CFloatVector3*> (segColorP), &vcd, 0);
		}
#if DBG
	if (segColorP->Red () + segColorP->Green () + segColorP->Blue () == 0)
		segColorP = segColorP;
#endif
	lightManager.ResetAllUsed (0, nThread);
	m_data.index [0][0].nActive = -1;
	}
else {
	if (vPosP) {
		vCenter = SEGMENTS [nSegment].Center ();
		//transformation.Transform (&vCenter, &vCenter);
		ds = 0.0f;
		}
	else
		ds = 1.0f;
	pv = SEGMENTS [nSegment].m_vertices;
	c.Set (0.0f, 0.0f, 0.0f, 1.0f);
	c.index = 0;
	for (i = 0; i < 8; i++, pv++) {
		if (*pv == 0xFFFF)
			continue;
		vertColorP = gameData.render.color.vertices + *pv;
		if (vPosP) {
			vVertex = gameData.segs.vertices [*pv];
			//transformation.Transform (&vVertex, &vVertex);
			d = 2.0f - X2F (CFixVector::Dist(vVertex, *vPosP)) / X2F (CFixVector::Dist(vCenter, vVertex));
			c += *vertColorP * d;
			ds += d;
			}
		else {
			c += *vertColorP;
			}
		}
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	c *= 0.125f; // => / 8.0f
	segColorP->Assign (c);
	}
segColorP->index = (char) (gameData.app.nFrameCount & 0xff);
return segColorP;
}

//------------------------------------------------------------------------------

void CLightManager::ResetSegmentLights (void)
{
for (short i = 0; i < gameData.segs.nSegments; i++)
	gameData.render.color.segments [i].index = -1;
}

//------------------------------------------------------------------------------

void CLightManager::ResetNearestStatic (int nSegment, int nThread)
{
if (gameStates.render.nLightingMethod) {
	short*		nearestLightP = m_data.nearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	short			i, j;
	CDynLight*	lightP;

	for (i = MAX_NEAREST_LIGHTS /*gameStates.render.nMaxLightsPerFace*/; i; i--, nearestLightP++) {
		if ((j = *nearestLightP) < 0)
			break;
		if ((lightP = RenderLights (j)) && (lightP->render.bUsed [nThread] == 3))
			ResetUsed (lightP, nThread);
		}
	}
}

//------------------------------------------------------------------------------

void CLightManager::ResetNearestToVertex (int nVertex, int nThread)
{
//if (gameStates.render.nLightingMethod)
 {
	short*		nearestLightP = m_data.nearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	short			i, j;
	CDynLight*	lightP;

#if DBG
	if (nVertex == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	for (i = MAX_NEAREST_LIGHTS; i; i--, nearestLightP++) {
		if ((j = *nearestLightP) < 0)
			break;
		if ((lightP = RenderLights (j)) && (lightP->render.bUsed [nThread] == 2))
			ResetUsed (lightP, nThread);
		}
	}
}

//------------------------------------------------------------------------------

void CLightManager::ResetUsed (CDynLight* lightP, int nThread)
{
	CActiveDynLight* activeLightsP = lightP->render.activeLightsP [nThread];

if (activeLightsP) {
	lightP->render.activeLightsP [nThread] = NULL;
	activeLightsP->lightP = NULL;
	activeLightsP->nType = 0;
	}
lightP->render.bUsed [nThread] = 0;
}

//------------------------------------------------------------------------------

void CLightManager::ResetAllUsed (int bVariable, int nThread)
{
	int			i = m_data.nLights [1];
	CDynLight*	lightP;

if (bVariable) {
	while (i) {
		if ((lightP = m_data.renderLights [--i])) {
			if (lightP->info.nType < 2)
				break;
			ResetUsed (lightP, nThread);
			}
		}
	}
else {
	while (i) {
		if ((lightP = m_data.renderLights [--i])) {
			ResetUsed (lightP, nThread);
			}
		}
	}
}

//------------------------------------------------------------------------------

void CLightManager::ResetActive (int nThread, int nActive)
{
	CDynLightIndex*	sliP = &m_data.index [0][nThread];
	int					h;

if (0 < (h = sliP->nLast - sliP->nFirst + 1))
	memset (m_data.active [nThread] + sliP->nFirst, 0, sizeof (CActiveDynLight) * h);
sliP->nActive = nActive;
sliP->nFirst = MAX_OGL_LIGHTS;
sliP->nLast = 0;
}

// ----------------------------------------------------------------------------------------------
//eof
