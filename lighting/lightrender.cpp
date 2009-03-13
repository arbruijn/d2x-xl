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

#include "descent.h"
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

#define SORT_ACTIVE_LIGHTS 0

//------------------------------------------------------------------------------

void CLightManager::Transform (int bStatic, int bVariable)
{
	int			i;
	CDynLight*	pl = &m_data.lights [0];

m_data.nLights [1] = 0;
for (i = 0; i < m_data.nLights [0]; i++, pl++) {
#if DBG
	if ((nDbgSeg >= 0) && (nDbgSeg == pl->info.nSegment) && ((nDbgSide < 0) || (nDbgSide == pl->info.nSide)))
		nDbgSeg = nDbgSeg;
#endif
	pl->render.vPosf [0].Assign (pl->info.vPos);
	if (gameStates.ogl.bUseTransform)
		pl->render.vPosf [1] = pl->render.vPosf [0];
	else {
		transformation.Transform (pl->render.vPosf [1], pl->render.vPosf [0], 0);
		pl->render.vPosf [1][W] = 1;
		}
	pl->render.vPosf [0][W] = 1;
	pl->render.nType = pl->info.nType;
	pl->render.bState = pl->info.bState && (pl->info.color.red + pl->info.color.green + pl->info.color.blue > 0.0);
	pl->render.bLightning = (pl->info.nObject < 0) && (pl->info.nSide < 0);
	ResetUsed (pl, 0);
	if (gameStates.app.bMultiThreaded)
		ResetUsed (pl, 1);
	pl->render.bShadow =
	pl->render.bExclusive = 0;
	if (pl->render.bState) {
		if (!bStatic && (pl->info.nType == 1) && !pl->info.bVariable)
			pl->render.bState = 0;
		if (!bVariable && ((pl->info.nType > 1) || pl->info.bVariable))
			pl->render.bState = 0;
		}
	m_data.renderLights [m_data.nLights [1]++] = pl;
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
					m = activeLightsP [(l + r) / 2]->xDistance;

do {
	while (activeLightsP [l]->xDistance < m)
		l++;
	while (activeLightsP [r]->xDistance > m)
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

int CLightManager::SetActive (CActiveDynLight* activeLightsP, CDynLight* prl, short nType, int nThread)
{
if (prl->render.bUsed [nThread])
	return 0;
fix xDist = prl->info.bSpot ? 0 : (prl->render.xDistance / 2000 + 5) / 10;
if (xDist >= MAX_SHADER_LIGHTS)
	return 0;
if (xDist < 0)
	xDist = 0;
#if PREFER_GEOMETRY_LIGHTS
else if (prl->info.nSegment >= 0)
	xDist /= 2;
else if (!prl->info.bSpot)
	xDist += (MAX_SHADER_LIGHTS - xDist) / 2;
#endif
#if 1
while (activeLightsP [xDist].nType) {
	if (activeLightsP [xDist].pl == prl)
		return 0;
	if (++xDist >= MAX_SHADER_LIGHTS)
		return 0;
	}
#else
if (activeLightsP [xDist].info.nType) {
	for (int j = xDist; j < MAX_SHADER_LIGHTS - 1; j++) {
		if (!activeLightsP [j].info.nType) {
			memmove (activeLightsP + xDist + 1, activeLightsP + xDist, (j - xDist) * sizeof (CActiveDynLight));
			xDist = j;
			break;
			}
		else if (activeLightsP [j].pl == prl)
			return 0;
		}
	}
#endif
activeLightsP [xDist].nType = nType;
activeLightsP [xDist].pl = prl;
prl->render.activeLightsP [nThread] = activeLightsP + xDist;
prl->render.bUsed [nThread] = (ubyte) nType;

CDynLightIndex* sliP = &m_data.index [0][nThread];

sliP->nActive++;
if (sliP->nFirst > xDist)
	sliP->nFirst = (short) xDist;
if (sliP->nLast < xDist)
	sliP->nLast = (short) xDist;
return 1;
}

//------------------------------------------------------------------------------

CDynLight* CLightManager::GetActive (CActiveDynLight* activeLightsP, int nThread)
{
	CDynLight*	prl = activeLightsP->pl;
#if 0
if (prl) {
	if (prl->render.bUsed [nThread] > 1)
		prl->render.bUsed [nThread] = 0;
	if (activeLightsP->nType > 1) {
		activeLightsP->nType = 0;
		activeLightsP->pl = NULL;
		m_data.index [0][nThread].nActive--;
		}
	}
#endif
if (prl == reinterpret_cast<CDynLight*> (0xffffffff))
	return NULL;
return prl;
}

//------------------------------------------------------------------------------

ubyte CLightManager::VariableVertexLights (int nVertex)
{
	short*		pnl = m_data.nearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	CDynLight*	pl;
	short			i, j;
	ubyte			h;

#if DBG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
for (h = 0, i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
	if ((j = *pnl) < 0)
		break;
	if ((pl = RenderLights (j)))
		h += pl->info.bVariable;
	}
return h;
}

//------------------------------------------------------------------------------

void CLightManager::SetNearestToVertex (int nFace, int nVertex, CFixVector *vNormalP, ubyte nType, int bStatic, int bVariable, int nThread)
{
if (bStatic || m_data.variableVertLights [nVertex]) {
	PROF_START
	short*				pnl = m_data.nearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	CDynLightIndex*	sliP = &m_data.index [0][nThread];
	short					i, j, nActiveLightI = sliP->nActive;
	CDynLight*			prl;
	CActiveDynLight*	activeLightsP = m_data.active [nThread].Buffer ();
	CFixVector			vVertex = gameData.segs.vertices [nVertex], vLightDir;
	fix					xLightDist, xMaxLightRange = (gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 : MAX_LIGHT_RANGE;

#if DBG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
	sliP->iVertex = nActiveLightI;
	for (i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
#if DBG
		if (j >= m_data.nLights [1])
			break;
#endif
		if (!(prl = RenderLights (j)))
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (prl->info.nSegment == nDbgSeg))
			nDbgSeg = nDbgSeg;
#endif
		if (prl->render.bUsed [nThread])
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (prl->info.nSegment == nDbgSeg))
			nDbgSeg = nDbgSeg;
#endif
		if (gameData.threads.vertColor.data.bNoShadow && prl->render.bShadow)
			continue;
		if (prl->info.bVariable) {
			if (!(bVariable && prl->info.bOn))
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		vLightDir = vVertex - prl->info.vPos;
		xLightDist = vLightDir.Mag();
#if 1
		if (vNormalP) {
			vLightDir /= xLightDist;
			if(CFixVector::Dot (*vNormalP, vLightDir) > I2X (1) / 2)
				continue;
		}
#endif
#if 0
		prl->render.xDistance = (fix) (xLightDist / prl->info.fRange) /*- F2X (prl->info.fRad*/;
#else
		prl->render.xDistance = (fix) (xLightDist / prl->info.fRange);
		if (prl->info.nSegment >= 0)
			prl->render.xDistance -= SEGMENTS [prl->info.nSegment].AvgRad ();
#endif
		if (prl->render.xDistance > xMaxLightRange)
			continue;
		if (SetActive (activeLightsP, prl, 2, nThread)) {
			prl->render.nType = nType;
			//prl->render.bState = 1;
#if DBG
			prl->render.nTarget = nFace + 1;
			prl->render.nFrame = gameData.app.nFrameCount;
#endif
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
	CSide*		sideP = SEGMENTS [faceP->nSegment].m_sides + faceP->nSide;

#if DBG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if (faceP - FACES.faces == nDbgFace)
	nDbgFace = nDbgFace;
#endif
#if 1//!DBG
if (m_data.index [0][0].nActive < 0)
	lightManager.SetNearestToSegment (faceP->nSegment, faceP - FACES.faces, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
else {
#if 0//def _DEBUG
	CheckUsedLights2 ();
#endif
	m_data.index [0][0] = m_data.index [1][0];
	}
#else
lightManager.SetNearestToSegment (faceP->nSegment, faceP - FACES, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
#endif
vNormal = sideP->m_normals[0] + sideP->m_normals[1];
vNormal *= (I2X (1) / 2);
#if 1
for (i = 0; i < 4; i++)
	lightManager.SetNearestToVertex (faceP - FACES.faces, faceP->index [i], &vNormal, 0, 0, 1, 0);
#endif
PROF_END(ptPerPixelLighting)
return m_data.index [0][0].nActive;
}

//------------------------------------------------------------------------------

void CLightManager::SetNearestStatic (int nSegment, int bStatic, ubyte nType, int nThread)
{
	static short nActiveLights [4] = {-1, -1, -1, -1};

if (gameStates.render.nLightingMethod) {
	short*				pnl = m_data.nearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	short					i, j;
	CDynLight*			prl;
	CActiveDynLight*	activeLightsP = m_data.active [nThread].Buffer ();

	//m_data.iStaticLights [nThread] = m_data.index [0][nThread].nActive;
	for (i = gameStates.render.nMaxLightsPerFace; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
		if (!(prl = RenderLights (j)))
			continue;
		if (gameData.threads.vertColor.data.bNoShadow && prl->render.bShadow)
			continue;
		if (prl->info.bVariable) {
			if (!prl->info.bOn)
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		SetActive (activeLightsP, prl, 3, nThread);
		}
	}
nActiveLights [nThread] = m_data.index [0][nThread].nActive;
}

//------------------------------------------------------------------------------

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
	short						i = m_data.nLights [1],
								nLightSeg;
	int						bSkipHeadlight = !gameStates.render.nState && ((gameStates.render.bPerPixelLighting == 2) || gameOpts->ogl.bHeadlight);
	fix						xMaxLightRange = SEGMENTS [nSegment].AvgRad () + ((gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 : MAX_LIGHT_RANGE);
	CDynLight*				prl;
	CFixVector				c;
	CActiveDynLight*		activeLightsP = m_data.active [nThread].Buffer ();

	c = SEGMENTS [nSegment].Center ();
	lightManager.ResetAllUsed (1, nThread);
	lightManager.ResetActive (nThread, 0);
	while (i) {
		if (!(prl = RenderLights (--i)))
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (prl->info.nSegment == nDbgSeg))
			prl = prl;
#endif
		nType = prl->info.nType;
		if (nType == 3) {
			if (bSkipHeadlight)
				continue;
			}
		if (nType < 2) {
			if (!bVariable)
				break;
			if (!(prl->info.bVariable && prl->info.bOn))
				continue;
			}
		if (gameData.threads.vertColor.data.bNoShadow && prl->render.bShadow)
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (prl->info.nSegment == nDbgSeg))
			prl = prl;
		if ((prl->info.nSegment >= 0) && (prl->info.nSide < 0))
			prl = prl;
#endif
		if (nType < 3) {
			if (prl->info.bPowerup > gameData.render.nPowerupFilter)
				continue;
#if DBG
			if (prl->info.nObject >= 0)
				nDbgObj = nDbgObj;
#endif
			nLightSeg = (prl->info.nSegment < 0) ? (prl->info.nObject < 0) ? -1 : OBJECTS [prl->info.nObject].info.nSegment : prl->info.nSegment;
			if ((nLightSeg < 0) || !SEGVIS (nLightSeg, nSegment))
				continue;
			}
#if DBG
		else
			prl = prl;
#endif
		prl->render.xDistance = (fix) ((CFixVector::Dist(c, prl->info.vPos) /*- F2X (prl->info.fRad)*/) / (prl->info.fRange * max (prl->info.fRad, 1.0f)));
		if (prl->render.xDistance > xMaxLightRange)
			continue;
#if DBG
		if (SetActive (activeLightsP, prl, 1, nThread)) {
			if ((nSegment == nDbgSeg) && (nDbgObj >= 0) && (prl->info.nObject == nDbgObj))
				prl = prl;
			if (nFace < 0)
				prl->render.nTarget = -nSegment - 1;
			else
				prl->render.nTarget = nFace + 1;
			prl->render.nFrame = gameData.app.nFrameCount;
			}
#else
		SetActive (activeLightsP, prl, 1, nThread);
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
	int						nLightSeg;
	short						i, n = m_data.nLights [1];
	fix						xLightDist, xMaxLightRange = F2X (fLightRad) + ((gameStates.render.bPerPixelLighting == 2) ? MAX_LIGHT_RANGE * 2 : MAX_LIGHT_RANGE);
	CDynLight*				prl;
	CFixVector				vLightDir;
	CActiveDynLight*		activeLightsP = m_data.active [nThread].Buffer ();

	ResetActive (nThread, 0);
	ResetAllUsed (0, nThread);
	for (i = 0; i < n; i++) {
		if (!(prl = RenderLights (i)))
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (prl->info.nSegment == nDbgSeg))
			prl = prl;
#endif
		if (prl->info.nType)
			break;
		if (prl->info.bVariable)
			continue;
#if DBG
		if ((nDbgSeg >= 0) && (prl->info.nSegment == nDbgSeg))
			prl = prl;
#endif
		vLightDir = *vPixelPos - prl->info.vPos;
		xLightDist = vLightDir.Mag();
		nLightSeg = prl->info.nSegment;
#if 0
		if ((nLightSeg != nSegment) || (prl->info.nSide != nSide)) {
			vLightDir.p.x = FixDiv (vLightDir.p.x, xLightDist);
			vLightDir.p.y = FixDiv (vLightDir.p.y, xLightDist);
			vLightDir.p.z = FixDiv (vLightDir.p.z, xLightDist);
			if (VmVecDot (vNormal, &vLightDir) >= 32768)
				continue;
			}
#endif
		if ((nLightSeg < 0) || !SEGVIS (nLightSeg, nSegment))
			continue;
		prl->render.xDistance = (fix) ((CFixVector::Dist (*vPixelPos, prl->info.vPos) /*- F2X (prl->info.fRad)*/) / prl->info.fRange);
		if (prl->render.xDistance > xMaxLightRange)
			continue;
		SetActive (activeLightsP, prl, 1, nThread);
		}
	}
return m_data.index [0][nThread].nActive;
}

//------------------------------------------------------------------------------

int CLightManager::SetNearestToSgmAvg (short nSegment)
{
	int			i;
	CSegment		*segP = SEGMENTS + nSegment;

#if DBG
if (nSegment == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
lightManager.SetNearestToSegment (nSegment, -1, 0, 0, 0);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
#if 1
for (i = 0; i < 8; i++)
	lightManager.SetNearestToVertex (-1, segP->m_verts [i], NULL, 0, 1, 1, 0);
#endif
return m_data.index [0][0].nActive;
}

//------------------------------------------------------------------------------

tFaceColor* CLightManager::AvgSgmColor (int nSegment, CFixVector *vPosP)
{
	tFaceColor	c, *pvc, *psc = gameData.render.color.segments + nSegment;
	short			i, *pv;
	CFixVector	vCenter, vVertex;
	float			d, ds;

#if DBG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
if (!vPosP && (psc->index == (char) (gameData.app.nFrameCount & 0xff)) && (psc->color.red + psc->color.green + psc->color.blue != 0))
	return psc;
#if DBG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
if (SEGMENTS [nSegment].m_nType == SEGMENT_IS_SKYBOX) {
	psc->color.red = psc->color.green = psc->color.blue = 1.0f;
	psc->index = 1;
	}
else if (gameStates.render.bPerPixelLighting) {
	psc->color.red =
	psc->color.green =
	psc->color.blue = 0;
	if (SetNearestToSgmAvg (nSegment)) {
			CVertColorData	vcd;

		InitVertColorData (vcd);
		vcd.vertNorm[X] =
		vcd.vertNorm[Y] =
		vcd.vertNorm[Z] = 0;
		vcd.vertNorm.SetZero ();
		if (vPosP)
			vcd.vertPos.Assign (*vPosP);
		else {
			vCenter = SEGMENTS [nSegment].Center ();
			vcd.vertPos.Assign (vCenter);
			}
		vcd.vertPosP = &vcd.vertPos;
		vcd.fMatShininess = 4;
		G3AccumVertColor (-1, reinterpret_cast<CFloatVector3*> (psc), &vcd, 0);
		}
#if DBG
	if (psc->color.red + psc->color.green + psc->color.blue == 0)
		psc = psc;
#endif
	lightManager.ResetAllUsed (0, 0);
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
	pv = SEGMENTS [nSegment].m_verts;
	c.color.red = c.color.green = c.color.blue = 0.0f;
	c.index = 0;
	for (i = 0; i < 8; i++, pv++) {
		pvc = gameData.render.color.vertices + *pv;
		if (vPosP) {
			vVertex = gameData.segs.vertices [*pv];
			//transformation.Transform (&vVertex, &vVertex);
			d = 2.0f - X2F (CFixVector::Dist(vVertex, *vPosP)) / X2F (CFixVector::Dist(vCenter, vVertex));
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
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	psc->color.red = c.color.red / 8.0f;
	psc->color.green = c.color.green / 8.0f;
	psc->color.blue = c.color.blue / 8.0f;
#if 0
	if (lightManager.SetNearestToSegment (nSegment, 1)) {
		CDynLight*		prl;
		float				fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightfRange];
		float				fLightDist, fAttenuation;
		CFloatVector	vPosf;
		if (vPosP)
			vPosf.Assign (vPosP);
		for (i = 0; i < m_data.nActiveLights; i++) {
			prl = m_data.active [i];
#if 1
			if (vPosP) {
				vVertex = gameData.segs.vertices [*pv];
				//transformation.Transform (&vVertex, &vVertex);
				fLightDist = VmVecDist (prl->render.vPosf, &vPosf) / fLightRange;
				fAttenuation = fLightDist / prl->info.fBrightness;
				VmVecScaleAdd (reinterpret_cast<CFloatVector*> (&c.color), reinterpret_cast<CFloatVector*> (&c.color, reinterpret_cast<CFloatVector*> (&prl->render.color, 1.0f / fAttenuation);
				}
			else
#endif
			 {
				VmVecInc (reinterpret_cast<CFloatVector*> (&psc->color), reinterpret_cast<CFloatVector*> (&prl->render.color);
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
psc->index = (char) (gameData.app.nFrameCount & 0xff);
return psc;
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
	short*		pnl = m_data.nearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	short			i, j;
	CDynLight*	prl;

	for (i = gameStates.render.nMaxLightsPerFace; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
		if ((prl = RenderLights (j)) && (prl->render.bUsed [nThread] == 3))
			ResetUsed (prl, nThread);
		}
	}
}

//------------------------------------------------------------------------------

void CLightManager::ResetNearestToVertex (int nVertex, int nThread)
{
//if (gameStates.render.nLightingMethod)
 {
	short*		pnl = m_data.nearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	short			i, j;
	CDynLight*	prl;

#if DBG
	if (nVertex == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	for (i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
		if ((prl = RenderLights (j)) && (prl->render.bUsed [nThread] == 2))
			ResetUsed (prl, nThread);
		}
	}
}

//------------------------------------------------------------------------------

void CLightManager::ResetUsed (CDynLight* prl, int nThread)
{
	CActiveDynLight* activeLightsP = prl->render.activeLightsP [nThread];

if (activeLightsP) {
	activeLightsP->pl = NULL;
	activeLightsP->nType = 0;
	prl->render.activeLightsP [nThread] = NULL;
	}
prl->render.bUsed [nThread] = 0;
}

//------------------------------------------------------------------------------

void CLightManager::ResetAllUsed (int bVariable, int nThread)
{
	int			i = m_data.nLights [1];
	CDynLight*	prl;

while (i) {
	if (!(prl = RenderLights (--i)))
		continue;
	if (bVariable && (prl->info.nType < 2))
		break;
	ResetUsed (prl, nThread);
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
sliP->nFirst = MAX_SHADER_LIGHTS;
sliP->nLast = 0;
}

// ----------------------------------------------------------------------------------------------
//eof
