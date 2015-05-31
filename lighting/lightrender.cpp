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

void CLightManager::Transform (int32_t bStatic, int32_t bVariable)
{
	int32_t			i;
	CDynLight*	pLight = &m_data.lights [0];

m_data.nLights [1] = 0;
for (i = 0; i < m_data.nLights [0]; i++, pLight++) {
#if DBG
	if ((nDbgSeg >= 0) && (nDbgSeg == pLight->info.nSegment) && ((nDbgSide < 0) || (nDbgSide == pLight->info.nSide)))
		BRP;
	if (i == nDbgLight)
		nDbgLight = nDbgLight;
#endif
	pLight->render.vPosf [0].Assign (pLight->info.vPos);
	if (ogl.m_states.bUseTransform)
		pLight->render.vPosf [1] = pLight->render.vPosf [0];
	else {
		transformation.Transform (pLight->render.vPosf [1], pLight->render.vPosf [0], 0);
		pLight->render.vPosf [1].v.coord.w = 1;
		}
	pLight->render.vPosf [0].v.coord.w = 1;
	pLight->render.nType = pLight->info.nType;
	pLight->render.bState = pLight->info.bState && (pLight->info.color.Red () + pLight->info.color.Green () + pLight->info.color.Blue () > 0.0);
	pLight->render.bLightning = (pLight->info.nObject < 0) && (pLight->info.nSide < 0);
	for (int32_t j = 0; j < gameStates.app.nThreads; j++)
		ResetUsed (pLight, j);
	pLight->render.bShadow =
	pLight->render.bExclusive = 0;
	if (pLight->render.bState) {
		if (!bStatic && (pLight->info.nType == 1) && !pLight->info.bVariable)
			pLight->render.bState = 0;
		if (!bVariable && ((pLight->info.nType > 1) || pLight->info.bVariable))
			pLight->render.bState = 0;
		}
	m_data.renderLights [m_data.nLights [1]++] = pLight;
	}
m_headlights.Prepare ();
m_headlights.Update ();
m_headlights.Transform ();
}

//------------------------------------------------------------------------------

#if SORT_ACTIVE_LIGHTS

static void QSortDynamicLights (int32_t left, int32_t right, int32_t nThread)
{
	CDynLight**	pActiveLights = m_data.active [nThread];
	int32_t			l = left,
					r = right,
					mat = pActiveLights [(l + r) / 2]->xDistance;

do {
	while (pActiveLights [l]->xDistance < mat)
		l++;
	while (pActiveLights [r]->xDistance > mat)
		r--;
	if (l <= r) {
		if (l < r) {
			CDynLight* h = pActiveLights [l];
			pActiveLights [l] = pActiveLights [r];
			pActiveLights [r] = h;
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

int32_t CLightManager::SetActive (CActiveDynLight* pActiveLights, CDynLight* pLight, int16_t nType, int32_t nThread, bool bForce)
{
ENTER (nThread);
#if DBG
if ((nDbgSeg >= 0) && (pLight->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pLight->info.nSide == nDbgSide)))
	BRP;
if ((nDbgObj >= 0) && (pLight->info.nObject == nDbgObj))
	BRP;
#endif

if (pLight->render.bUsed [nThread])
	RETURN (0);
fix xDist;
if (bForce || pLight->info.bSpot) 
	xDist = 0;
else {
	xDist = (pLight->render.xDistance [nThread] / 2000 + 5) / 10;
	if (xDist >= MAX_OGL_LIGHTS)
	RETURN (0);
	if (xDist < 0)
		xDist = 0;
	}
#if PREFER_GEOMETRY_LIGHTS
else if (pLight->info.nSegment >= 0)
	xDist /= 2;
else if (!pLight->info.bSpot)
	xDist += (MAX_OGL_LIGHTS - xDist) / 2;
#endif
pActiveLights += xDist;
while (pActiveLights->nType) {
	if (pActiveLights->pLight == pLight)
		RETURN (0);
	if (++xDist >= MAX_OGL_LIGHTS)
		RETURN (0);
	pActiveLights++;
	}

#if DBG
if ((nDbgSeg >= 0) && (pLight->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pLight->info.nSide == nDbgSide)))
	BRP;
if ((nDbgObj >= 0) && (pLight->info.nObject == nDbgObj))
	BRP;
#endif

pActiveLights->nType = nType;
pActiveLights->pLight = pLight;
pLight->render.pActiveLights [nThread] = pActiveLights;
pLight->render.bUsed [nThread] = uint8_t (nType);

CDynLightIndex* pLightIndex = &m_data.index [0][nThread];

pLightIndex->nActive++;
if (pLightIndex->nFirst > xDist)
	pLightIndex->nFirst = int16_t (xDist);
if (pLightIndex->nLast < xDist)
	pLightIndex->nLast = int16_t (xDist);
RETURN (1);
}

//------------------------------------------------------------------------------

CDynLight* CLightManager::GetActive (CActiveDynLight* pActiveLights, int32_t nThread)
{
	CDynLight*	pLight = pActiveLights->pLight;
#if 0
if (pLight) {
	if (pLight->render.bUsed [nThread] > 1)
		pLight->render.bUsed [nThread] = 0;
	if (pActiveLights->nType > 1) {
		pActiveLights->nType = 0;
		pActiveLights->pLight = NULL;
		m_data.index [0][nThread].nActive--;
		}
	}
#endif
if (pLight == reinterpret_cast<CDynLight*> (0xffffffff))
	return NULL;
return pLight;
}

//------------------------------------------------------------------------------

uint8_t CLightManager::VariableVertexLights (int32_t nVertex)
{
	int16_t*		pNearestLight = m_data.nearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	CDynLight*	pLight;
	int16_t		i, j;
	uint8_t		h;

#if DBG
if (nVertex == nDbgVertex)
	BRP;
#endif
for (h = 0, i = MAX_NEAREST_LIGHTS; i; i--, pNearestLight++) {
	if ((j = *pNearestLight) < 0)
		break;
	if ((pLight = RenderLights (j)))
		h += pLight->info.bVariable;
	}
return h;
}

//------------------------------------------------------------------------------

void CLightManager::SetNearestToVertex (int32_t nSegment, int32_t nSide, int32_t nVertex, CFixVector *vNormal, uint8_t nType, int32_t bStatic, int32_t bVariable, int32_t nThread)
{
ENTER (nThread);

if (bStatic || m_data.variableVertLights [nVertex]) {
	int16_t				*pNearestLight = m_data.nearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	CDynLightIndex		*pLightIndex = &m_data.index [0][nThread];
	int16_t				i, j, nActiveLightI = pLightIndex->nActive;
	CDynLight*			pLight;
	CActiveDynLight	*pActiveLights = m_data.active [nThread].Buffer ();
	CFixVector			vVertex = gameData.segData.vertices [nVertex];
	fix					xMaxLightRange = /*(gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 :*/ MAX_LIGHT_RANGE;

#if DBG
if (nVertex == nDbgVertex)
	BRP;
#endif
	pLightIndex->iVertex = nActiveLightI;
	for (i = MAX_NEAREST_LIGHTS; i; i--, pNearestLight++) {
		if ((j = *pNearestLight) < 0)
			break;
		if (!(pLight = RenderLights (j)))
			continue;
		if (pLight->render.bUsed [nThread])
			continue;
		if (gameData.threadData.vertColor.data.bNoShadow && pLight->render.bShadow)
			continue;
		if (pLight->info.bVariable) {
			if (!(bVariable && pLight->info.bOn))
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
#if 1
		CSegment *pLightSeg = SEGMENT (pLight->info.nSegment);
		if (!pLight->Contribute (nSegment, nSide, nVertex, vVertex, vNormal, xMaxLightRange, 1.0f, pLightSeg ? -pLightSeg->AvgRad () : 0, nThread))
			continue;
#else
		CFixVector vLightToVertex = vVertex - pLight->info.vPos;
		fix xLightDist = CFixVector::Normalize (vLightToVertex.Mag ());
		if ((pLight->info.bDiffuse [nThread] = gameData.segData.LightVis (pLight->Index (), nSegment) && pLight->SeesPoint (vNormal, &vLightToVertex)) || (nSegment < 0))
			pLight->render.xDistance [nThread] = fix (xLightDist / pLight->info.fRange);
		else if (nSegment >= 0) {
			pLight->render.xDistance [nThread] = pLight->LightPathLength (nSegment, vVertex);
			if (pLight->render.xDistance [nThread] < 0)
				continue;
			}
		if (pLight->info.nSegment >= 0)
			pLight->render.xDistance [nThread] -= SEGMENT (pLight->info.nSegment)->AvgRad ();
		if (pLight->render.xDistance [nThread] > xMaxLightRange)
			continue;
#endif
		if (SetActive (pActiveLights, pLight, 2, nThread)) {
			pLight->render.nType = nType;
			//pLight->render.bState = 1;
			}
		}
	}
LEAVE;
}

//------------------------------------------------------------------------------

int32_t CLightManager::SetNearestToFace (CSegFace* pFace, int32_t bTextured)
{
ENTER (0);

PROF_START
#if 0
	static		int32_t nFrameCount = -1;
if ((pFace == prevFaceP) && (nFrameCount == gameData.appData.nFrameCount))
	RETURN (m_data.index [0][0].nActive);

prevFaceP = pFace;
nFrameCount = gameData.appData.nFrameCount;
#endif
	int32_t		i;
	CFixVector	vNormal;
	CSide*		pSide = SEGMENT (pFace->m_info.nSegment)->m_sides + pFace->m_info.nSide;

#if DBG
if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
if (pFace - FACES.faces == nDbgFace)
	BRP;
#endif
#if 1//!DBG
if (m_data.index [0][0].nActive < 0)
	lightManager.SetNearestToSegment (pFace->m_info.nSegment, pFace - FACES.faces, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
else {
	m_data.index [0][0] = m_data.index [1][0];
	}
#else
lightManager.SetNearestToSegment (pFace->m_info.nSegment, pFace - FACES, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
#endif
vNormal = pSide->m_normals [2];
#if 1
for (i = 0; i < 4; i++)
	lightManager.SetNearestToVertex (-1, -1, pFace->m_info.index [i], &vNormal, 0, 0, 1, 0);
#endif
PROF_END(ptPerPixelLighting)
RETURN (m_data.index [0][0].nActive);
}

//------------------------------------------------------------------------------

void CLightManager::SetNearestStatic (int32_t nSegment, int32_t bStatic, int32_t nThread)
{
ENTER (nThread);
if (gameStates.render.nLightingMethod) {
	CSegment				*pSeg = SEGMENT (nSegment);
	if (!pSeg)
		LEAVE;
	int16_t				*pNearestLight = m_data.nearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	int16_t				i, j;
	CDynLight*			pLight;
	CActiveDynLight	*pActiveLights = m_data.active [nThread].Buffer ();
	fix					xMaxLightRange = pSeg->AvgRad () + (/*(gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 :*/ MAX_LIGHT_RANGE);
	CFixVector			c = pSeg->Center ();

	//m_data.iStaticLights [nThread] = m_data.index [0][nThread].nActive;
	for (i = MAX_NEAREST_LIGHTS; i; i--, pNearestLight++) {
		if ((j = *pNearestLight) < 0)
			break;
		if (!(pLight = RenderLights (j)))
			continue;
		if (gameData.threadData.vertColor.data.bNoShadow && pLight->render.bShadow)
			continue;
		if (pLight->info.bVariable) {
			if (!pLight->info.bOn)
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		pLight->render.xDistance [nThread] = (fix) ((CFixVector::Dist (c, pLight->info.vPos) /*- F2X (pLight->info.fRad)*/) / (pLight->info.fRange * Max (pLight->info.fRad, 1.0f)));
		if (pLight->render.xDistance [nThread] > xMaxLightRange)
			continue;
		pLight->info.bDiffuse [nThread] = 1;
		SetActive (pActiveLights, pLight, 3, nThread);
		}
	}
LEAVE;
}

//------------------------------------------------------------------------------
// Retrieve closest variable (from objects and destructible/flickering geometry) lights.
// If bVariable == 0, only retrieve light emitting objects (will be stored in light buffer
// after all geometry lights).

int16_t CLightManager::SetNearestToSegment (int32_t nSegment, int32_t nFace, int32_t bVariable, int32_t nType, int32_t nThread)
{
ENTER (nThread);

	CDynLightIndex*	pLightIndex = &m_data.index [0][nThread];

#if DBG
	static int32_t nPrevSeg = -1;

if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	BRP;
#endif
if (gameStates.render.nLightingMethod) {
	uint8_t					nType;
	int16_t					i = m_data.nLights [1];
	int32_t					bSkipHeadlight = (gameStates.render.nType != RENDER_TYPE_OBJECTS) && ((gameStates.render.bPerPixelLighting == 2) || gameOpts->ogl.bHeadlight);
	fix						xMaxLightRange = SEGMENT (nSegment)->AvgRad () + (/*(gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 :*/ MAX_LIGHT_RANGE);
	CDynLight*				pLight;
	CFixVector&				vDestPos = SEGMENT (nSegment)->Center ();
	CActiveDynLight*		pActiveLights = m_data.active [nThread].Buffer ();

	lightManager.ResetAllUsed (0, nThread);
	lightManager.ResetActive (nThread, 0);
	while (i) {
		if (!(pLight = RenderLights (--i)))
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (pLight->info.nSegment == nDbgSeg))
			BRP;
#endif
		if (gameData.threadData.vertColor.data.bNoShadow && pLight->render.bShadow)
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (pLight->info.nSegment == nDbgSeg))
			BRP;
		if ((pLight->info.nSegment >= 0) && (pLight->info.nSide < 0))
			BRP;
#endif
		nType = pLight->info.nType;
		pLight->info.bDiffuse [nThread] = 1;
		if (nType == 3) {
			if (bSkipHeadlight)
				continue;
			}
		else {
			if (pLight->info.bPowerup > gameData.renderData.nPowerupFilter)
				continue;
			if (nType < 2) {	// all light emitting objects scanned
				if (!bVariable)
					break;
				if (!(pLight->info.bVariable && pLight->info.bOn))
					continue;
				}
#if DBG
			if (pLight->info.nObject >= 0)
				BRP;
#endif
			}
#if 1
		if (!pLight->Contribute (nSegment, -1, -1, vDestPos, NULL, xMaxLightRange, Max (pLight->info.fRad, 1.0f), 0, nThread))
			continue;
#else
		int16_t nLightSeg = pLight->info.nSegment;
		if (nLightSeg >= 0) 
			pLight->info.bDiffuse [nThread] = (pLight->info.nSide >= 0) 
													 ? gameData.segData.LightVis (pLight->Index (), nSegment) 
													 : gameData.segData.SegVis (pLight->info.nSegment, nSegment);
		else if ((pLight->info.nObject >= 0) && ((pLight->info.nSegment = OBJECT (pLight->info.nObject)->info.nSegment) >= 0))
			pLight->info.bDiffuse [nThread] = gameData.segData.SegVis (pLight->info.nSegment, nSegment);
		else
			continue;
		pLight->render.xDistance [nThread] = (fix) ((float) CFixVector::Dist (vDestPos, pLight->info.vPos) / (pLight->info.fRange * Max (pLight->info.fRad, 1.0f)));
		if (pLight->render.xDistance [nThread] > xMaxLightRange)
			continue;
		if (pLight->info.bDiffuse [nThread])
			pLight->info.bDiffuse [nThread] = pLight->SeesPoint (&vDestPos, NULL);
		if (!pLight->info.bDiffuse [nThread]) {
			pLight->render.xDistance [nThread] = pLight->LightPathLength (nSegment, vDestPos);
			if (pLight->render.xDistance [nThread] > xMaxLightRange)
				continue;
			}
#endif
#if DBG
		if (SetActive (pActiveLights, pLight, 1, nThread)) {
			if ((nSegment == nDbgSeg) && (nDbgObj >= 0) && (pLight->info.nObject == nDbgObj))
				BRP;
			if (nFace < 0)
				pLight->render.nTarget = -nSegment - 1;
			else
				pLight->render.nTarget = nFace + 1;
			pLight->render.nFrame = gameData.appData.nFrameCount;
			}
#else
		SetActive (pActiveLights, pLight, 1, nThread);
#endif
		}
	m_data.index [1][nThread] = *pLightIndex;
#if DBG
	if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
		BRP;
#endif
	}
#if DBG
nPrevSeg = nSegment;
#endif
RETURN (pLightIndex->nActive);
}

//------------------------------------------------------------------------------

int16_t CLightManager::SetNearestToPixel (int16_t nSegment, int8_t nSide, CFixVector *vNormal, CFixVector *vPixelPos, float fLightRad, int32_t nThread)
{
ENTER (nThread);
#if DBG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	BRP;
#endif
if (gameStates.render.nLightingMethod) {
	CSegment					*pSeg = SEGMENT (nSegment);
	if (!pSeg) {
		PrintLog (0, "Error: Invalid segment in \n");
		RETURN (0);
		}
	int16_t					i, n = m_data.nLights [1];
	fix						xMaxLightRange = F2X (fLightRad) + (/*(gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 :*/ MAX_LIGHT_RANGE);
	CDynLight*				pLight;
	CActiveDynLight		*pActiveLights = m_data.active [nThread].Buffer ();
	bool						bForce, bLight = Find (nSegment, nSide, -1) >= 0;

	ResetActive (nThread, 0);
	ResetAllUsed (0, nThread);
	for (i = 0; i < n; i++) {
		if (!(pLight = RenderLights (i)))
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (pLight->info.nSegment == nDbgSeg))
			BRP;
#endif
		if (pLight->info.nType)
			break;
		if (pLight->info.bVariable)
			continue;
		if (bLight && !pLight->Illuminate (nSegment, nSide))
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (nDbgSeg == pLight->info.nSegment))
			BRP;
		if ((nDbgSeg >= 0) && (nDbgSeg == nSegment))
			BRP;
#endif
		if (pLight->info.nSegment < 0)
			continue;
		if ((bForce = (pLight->info.nSegment == nSegment) && (pLight->info.nSide == nSide)))
			pLight->info.bDiffuse [nThread] = 1;
#if 1
		else if (!pLight->Contribute (nSegment, nSide, -1, *vPixelPos, &pSeg->Normal (nSide, 2), xMaxLightRange, 1.0f, 0, nThread))
			continue;
#else
		else {
			pLight->info.bDiffuse [nThread] = gameData.segData.LightVis (pLight->Index (), nSegment);
			CFixVector vLightToPixel = *vPixelPos - pLight->info.vPos;
			fix xLightDist = CFixVector::Normalize (vLightToPixel);
			pLight->render.xDistance [nThread] = (fix) (float (xLightDist) / pLight->info.fRange);
			if (pLight->render.xDistance [nThread] > xMaxLightRange)
				continue;
			if (pLight->info.bDiffuse [nThread]) {
				vLightToPixel /= xLightDist;
				pLight->info.bDiffuse [nThread] = pLight->SeesPoint (nSegment, nSide, &vLightToPixel);
				}
			if (!pLight->info.bDiffuse [nThread]) {
				pLight->render.xDistance [nThread] = pLight->LightPathLength (pLight->info.nSegment, nSegment, *vPixelPos, xMaxLightRange, 1, nThread);
				if ((pLight->render.xDistance [nThread] < 0) || (pLight->render.xDistance [nThread] > xMaxLightRange))
					continue;
				}
			}
#endif
#if DBG
		if (pLight->info.bDiffuse [nThread]) {
			BRP;
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				BRP;
			}
#endif
		SetActive (pActiveLights, pLight, 1, nThread, bForce);
		}
	}
RETURN (m_data.index [0][nThread].nActive);
}

//------------------------------------------------------------------------------

int32_t CLightManager::SetNearestToSgmAvg (int16_t nSegment, int32_t nThread)
{
ENTER (nThread);

	CSegment		*pSeg = SEGMENT (nSegment);

#if DBG
if (nSegment == nDbgSeg)
	BRP;
#endif
if (!pSeg)
	RETURN (0);
lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
#if 1
for (int32_t i = 0; i < 8; i++) {
	if (pSeg->m_vertices [i] != 0xFFFF)
		lightManager.SetNearestToVertex (-1, -1, pSeg->m_vertices [i], NULL, 0, 1, 1, 0);
	}
#endif
RETURN (m_data.index [0][0].nActive);
}

//------------------------------------------------------------------------------

CFaceColor* CLightManager::AvgSgmColor (int32_t nSegment, CFixVector *vPosP, int32_t nThread)
{
	CFaceColor	c, *pVertexColor, *pSegColor = gameData.renderData.color.segments + nSegment;
	uint16_t		*pv;
	int32_t		i;
	CFixVector	vCenter, vVertex;
	float			d, ds;

#if DBG
if (nSegment == nDbgSeg)
	BRP;
#endif
if (!vPosP && (pSegColor->index == (char) (gameData.appData.nFrameCount & 0xff)) && (pSegColor->Red () + pSegColor->Green () + pSegColor->Blue () != 0))
	return pSegColor;
#if DBG
if (nSegment == nDbgSeg)
	BRP;
#endif
nThread = ThreadId (nThread);
if (SEGMENT (nSegment)->m_function == SEGMENT_FUNC_SKYBOX) {
	pSegColor->Red () = pSegColor->Green () = pSegColor->Blue () = pSegColor->Alpha () = 1.0f;
	pSegColor->index = 1;
	}
else if (gameStates.render.bPerPixelLighting) {
	pSegColor->Red () =
	pSegColor->Green () =
	pSegColor->Blue () = 0;
	pSegColor->Alpha () = 1.0f;
	if (SetNearestToSgmAvg (nSegment, nThread)) {
			CVertColorData	vcd;

		InitVertColorData (vcd);
		vcd.vertNorm.SetZero ();
		if (vPosP)
			vcd.vertPos.Assign (*vPosP);
		else {
			vCenter = SEGMENT (nSegment)->Center ();
			vcd.vertPos.Assign (vCenter);
			}
		vcd.vertPosP = &vcd.vertPos;
		vcd.fMatShininess = 4;
		ComputeVertexColor (nSegment, -1, -1, reinterpret_cast<CFloatVector3*> (pSegColor), &vcd, 0);
		}
#if DBG
	if (pSegColor->Red () + pSegColor->Green () + pSegColor->Blue () == 0)
		pSegColor = pSegColor;
#endif
	lightManager.ResetAllUsed (0, nThread);
	m_data.index [0][0].nActive = -1;
	}
else {
	if (vPosP) {
		vCenter = SEGMENT (nSegment)->Center ();
		//transformation.Transform (&vCenter, &vCenter);
		ds = 0.0f;
		}
	else
		ds = 1.0f;
	pv = SEGMENT (nSegment)->m_vertices;
	c.Set (0.0f, 0.0f, 0.0f, 1.0f);
	c.index = 0;
	for (i = 0; i < 8; i++, pv++) {
		if (*pv == 0xFFFF)
			continue;
		pVertexColor = gameData.renderData.color.vertices + *pv;
		if (vPosP) {
			vVertex = gameData.segData.vertices [*pv];
			//transformation.Transform (&vVertex, &vVertex);
			d = 2.0f - X2F (CFixVector::Dist(vVertex, *vPosP)) / X2F (CFixVector::Dist(vCenter, vVertex));
			c += *pVertexColor * d;
			ds += d;
			}
		else {
			c += *pVertexColor;
			}
		}
#if DBG
	if (nSegment == nDbgSeg)
		BRP;
#endif
	c *= 0.125f; // => / 8.0f
	pSegColor->Assign (c);
	}
pSegColor->index = (char) (gameData.appData.nFrameCount & 0xff);
return pSegColor;
}

//------------------------------------------------------------------------------

void CLightManager::ResetSegmentLights (void)
{
for (int16_t i = 0; i < gameData.segData.nSegments; i++)
	gameData.renderData.color.segments [i].index = -1;
}

//------------------------------------------------------------------------------

void CLightManager::ResetNearestStatic (int32_t nSegment, int32_t nThread)
{
if (gameStates.render.nLightingMethod) {
	int16_t*		pNearestLight = m_data.nearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	int16_t			i, j;
	CDynLight*	pLight;

	for (i = MAX_NEAREST_LIGHTS /*gameStates.render.nMaxLightsPerFace*/; i; i--, pNearestLight++) {
		if ((j = *pNearestLight) < 0)
			break;
		if ((pLight = RenderLights (j)) && (pLight->render.bUsed [nThread] == 3))
			ResetUsed (pLight, nThread);
		}
	}
}

//------------------------------------------------------------------------------

void CLightManager::ResetNearestToVertex (int32_t nVertex, int32_t nThread)
{
//if (gameStates.render.nLightingMethod)
 {
	int16_t*		pNearestLight = m_data.nearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	int16_t			i, j;
	CDynLight*	pLight;

#if DBG
	if (nVertex == nDbgVertex)
		BRP;
#endif
	for (i = MAX_NEAREST_LIGHTS; i; i--, pNearestLight++) {
		if ((j = *pNearestLight) < 0)
			break;
		if ((pLight = RenderLights (j)) && (pLight->render.bUsed [nThread] == 2))
			ResetUsed (pLight, nThread);
		}
	}
}

//------------------------------------------------------------------------------

void CLightManager::ResetUsed (CDynLight* pLight, int32_t nThread)
{
	CActiveDynLight* pActiveLights = pLight->render.pActiveLights [nThread];

if (pActiveLights) {
	pLight->render.pActiveLights [nThread] = NULL;
	pActiveLights->pLight = NULL;
	pActiveLights->nType = 0;
	}
pLight->render.bUsed [nThread] = 0;
}

//------------------------------------------------------------------------------

void CLightManager::ResetAllUsed (int32_t bVariable, int32_t nThread)
{
	int32_t			i = m_data.nLights [1];
	CDynLight*	pLight;

if (bVariable) {
	while (i) {
		if ((pLight = m_data.renderLights [--i])) {
			if (pLight->info.nType < 2)
				break;
			ResetUsed (pLight, nThread);
			}
		}
	}
else {
	while (i) {
		if ((pLight = m_data.renderLights [--i])) {
			ResetUsed (pLight, nThread);
			}
		}
	}
}

//------------------------------------------------------------------------------

void CLightManager::ResetActive (int32_t nThread, int32_t nActive)
{
	CDynLightIndex*	pLightIndex = &m_data.index [0][nThread];
	int32_t					h;

if (0 < (h = pLightIndex->nLast - pLightIndex->nFirst + 1))
	memset (m_data.active [nThread] + pLightIndex->nFirst, 0, sizeof (CActiveDynLight) * h);
pLightIndex->nActive = nActive;
pLightIndex->nFirst = MAX_OGL_LIGHTS;
pLightIndex->nLast = 0;
}

// ----------------------------------------------------------------------------------------------
//eof
