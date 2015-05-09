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
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "automap.h"
#include "texmerge.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_fastrender.h"
#include "glare.h"
#include "rendermine.h"
#include "renderthreads.h"
#include "gpgpu_lighting.h"
#include "fastrender.h"

// 0: use average of the normals of all faces a vertex belongs to for lighting and only compute light once per frame and vertex (faster and smoother)
// 1: compute light for each face using the face's normal, computing lights multiple times for each vertex per frame (more correct)
#define LIGHTING_QUALITY 0 

// -----------------------------------------------------------------------------------

#if LIGHTING_QUALITY == 1

static void WaitWithUpdate (CFaceColor* pColor)
{
	int32_t bUpdate = false;

for (;;) {
#pragma omp critical
	{
	if (pColor->index >= 0) {
		pColor->index = -1;
		bUpdate = true;
		}
	}
// end critical section
	if (bUpdate)
		return;
	G3_SLEEP (0);
	}
}

#endif

// -----------------------------------------------------------------------------------

static int32_t UpdateColor (CFaceColor* pColor)
{
	int32_t bUpdate;

#if 0

#pragma omp critical
{
bUpdate = (pColor->index >= 0) && (pColor->index != gameStates.render.nFrameFlipFlop + 1);
}
if (bUpdate) { // another thread is already updating this vertex
	pColor->index = -1;
	return 1;
	}
while (pColor->index < 0) // wait until that thread is done
	G3_SLEEP (0);
return 0;

#else

#pragma omp critical
{
bUpdate = (pColor->index >= 0) && (pColor->index != gameStates.render.nFrameFlipFlop + 1);
if (bUpdate) 
	pColor->index = -1; // this thread is the first to light this vertex this frame
}
return bUpdate;

#endif
}

// -----------------------------------------------------------------------------------

int32_t SegmentIsVisible (CSegment *pSeg)
{
if (automap.Active ())
	return 1;
uint8_t code = 0xFF;

#if DBG
if (pSeg->Index () == nDbgSeg)
	BRP;
#endif
uint16_t* vertices = pSeg->m_vertices;
for (int32_t i = 0; i < 8; i++) {
	if (vertices [i] == 0xFFFF) 
		continue;
#if 0 //DBG
	RENDERPOINTS [vertices [i]].m_flags = 0;
#endif
	code &= RENDERPOINTS [vertices [i]].ProjectAndEncode (transformation, vertices [i]);
	if (!code)
		return 1;
	}
return 0;
}

// -----------------------------------------------------------------------------------

int32_t SegmentIsVisible (CSegment *pSeg, CTransformation& transformation, int32_t nThread)
{
#if DBG
	CArray<CRenderPoint>& points = gameData.render.mine.visibility [nThread].points;
#else
	CRenderPoint* points = gameData.render.mine.visibility [nThread].points.Buffer ();
#endif
	uint8_t code = 0xFF;

#if DBG
if (pSeg->Index () == nDbgSeg)
	BRP;
#endif
uint16_t* vertices = pSeg->m_vertices;
for (int32_t i = 0; i < 8; i++) {
	if (vertices [i] == 0xFFFF) 
		continue;
#if 0 //DBG
	RENDERPOINTS [vertices [i]].m_flags = 0;
#endif
	code &= points [vertices [i]].ProjectAndEncode (transformation, vertices [i]);
	if (!code)
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

static int32_t FaceIsVisible (CSegFace* pFace)
{
#if DBG
if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
#endif
if (!FaceIsVisible (pFace->m_info.nSegment, pFace->m_info.nSide))
	return pFace->m_info.bVisible = 0;
if ((pFace->m_info.bSparks == 1) && gameOpts->render.effects.bEnabled && gameOpts->render.effects.bEnergySparks)
	return pFace->m_info.bVisible = 0;
return pFace->m_info.bVisible = 1;
}

//------------------------------------------------------------------------------

int32_t SetupFace (int16_t nSegment, int16_t nSide, CSegment *pSeg, CSegFace *pFace, CFaceColor *faceColors, float& fAlpha)
{
	uint8_t	bTextured, bCloaked, bTransparent, bWall;
	int32_t	nColor = 0;

#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
if (FACE_IDX (pFace) == nDbgFace)
	BRP;
#endif

if (!FaceIsVisible (pFace))
	return -1;
bWall = IS_WALL (pFace->m_info.nWall);
if (bWall) {
	pFace->m_info.widFlags = pSeg->IsPassable (nSide, NULL);
	if (!(pFace->m_info.widFlags & WID_VISIBLE_FLAG)) //(WID_VISIBLE_FLAG | WID_TRANSPARENT_FLAG)))
		return -1;
	}
else
	pFace->m_info.widFlags = WID_VISIBLE_FLAG;
pFace->m_info.nCamera = IsMonitorFace (nSegment, nSide, 0);
bTextured = 1;
bCloaked = 0;
bTransparent = 0;
fAlpha = bWall
			? WallAlpha (nSegment, nSide, pFace->m_info.nWall, pFace->m_info.widFlags, pFace->m_info.nCamera >= 0, pFace->m_info.bAdditive,
							 &faceColors [1], nColor, bTextured, bCloaked, bTransparent)
			: 1.0f;
pFace->m_info.bTextured = bTextured;
pFace->m_info.bCloaked = bCloaked;
pFace->m_info.bTransparent |= bTransparent;
pFace->m_info.nSegColor = 0;
if (pFace->m_info.bSegColor) {
	if ((pFace->m_info.nSegColor = IsColoredSegFace (nSegment, nSide))) {
		pFace->m_info.color = *ColoredSegmentColor (nSegment, nSide, pFace->m_info.nSegColor);
		faceColors [2].Assign (pFace->m_info.color);
		if (pFace->m_info.nBaseTex < 0)
			fAlpha = pFace->m_info.color.Alpha ();
		nColor = 2;
		}
	else
		pFace->m_info.bVisible = (pFace->m_info.nBaseTex >= 0);
	}
else if (!bTextured) {
	faceColors [nColor].Alpha () = fAlpha;
	pFace->m_info.color = faceColors [nColor];
	}
if ((fAlpha < 1.0f) || ((nColor == 2) && (pFace->m_info.nBaseTex < 0)))
	pFace->m_info.bTransparent = 1;
return nColor;
}

//------------------------------------------------------------------------------

void ComputeDynamicFaceLight (int32_t nStart, int32_t nEnd, int32_t nThread)
{
	CSegFace*		pFace;
	CFloatVector*	pColor;
	CFaceColor		faceColor [3];
#if 0
	uint8_t			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	int32_t			nVertex, nSegment, nSide;
	float				fAlpha;
	int32_t			h, i, nColor, nLights = 0;
	//int32_t				bVertexLight = gameStates.render.bPerPixelLighting != 2;
	bool				bNeedLight = !gameStates.render.bFullBright && (gameStates.render.bPerPixelLighting != 2);
	static			CFaceColor brightColor;

for (i = 0; i < 3; i++)
	faceColor [i].Set (0.0f, 0.0f, 0.0f, 1.0f);
//memset (&gameData.render.lights.dynamic.shader.index, 0, sizeof (gameData.render.lights.dynamic.shader.index));
ogl.SetTransform (1);
gameStates.render.nState = 0;
#	if GPGPU_VERTEX_LIGHTING
if (ogl.m_states.bVertexLighting)
	ogl.m_states.bVertexLighting = gpgpuLighting.Compute (-1, 0, NULL);
#endif

for (i = nStart; i < nEnd; i++) {
	pFace = FACES.faces + i;
	nSegment = pFace->m_info.nSegment;
	nSide = pFace->m_info.nSide;

#if DBG
	if (nSegment == nDbgSeg)
		BRP;
#endif
	if (0 > (nColor = SetupFace (nSegment, nSide, SEGMENT (nSegment), pFace, faceColor, fAlpha))) {
		pFace->m_info.bVisible = 0;
		continue;
		}
	if (bNeedLight)
		nLights = lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
		BRP;
	if (lightManager.Index (0, nThread).nActive)
		BRP;
	}
#endif
	AddFaceListItem (pFace, nThread);
	faceColor [nColor].Alpha () = fAlpha;
	pFace->m_info.color.Assign (faceColor [nColor]);
	if (gameStates.render.bPerPixelLighting)
		nColor = 0;
	pColor = FACES.color + pFace->m_info.nIndex;
	for (h = 0; h < pFace->m_info.nVerts; h++, pColor++) {
		if (gameStates.render.bFullBright)
			*pColor = nColor ? faceColor [nColor] : brightColor;
		else {
			nVertex = pFace->m_info.index [h];
#if DBG
			if (nVertex == nDbgVertex)
				BRP;
#endif
			CFaceColor *pVertexColor = gameData.render.color.vertices + nVertex;
			if (UpdateColor (pVertexColor)) {
				if (nLights + lightManager.VariableVertLights (nVertex) == 0) {
					*pVertexColor = gameData.render.color.ambient [nVertex];
					pVertexColor->index = gameStates.render.nFrameFlipFlop + 1;
					}
				else {
#if DBG
					if (nVertex == nDbgVertex)
						BRP;
#endif
					GetVertexColor (nSegment, nSide, nVertex, RENDERPOINTS [nVertex].GetNormal ()->XYZ (), FVERTICES [nVertex].XYZ (), NULL, &gameData.render.color.ambient [nVertex], 1, 0, nThread);
					lightManager.Index (0, nThread) = lightManager.Index (1, nThread);
					lightManager.ResetNearestToVertex (nVertex, nThread);
					}
#if DBG
				if (nVertex == nDbgVertex)
					BRP;
#endif
				}
			*pColor = *pVertexColor;
			if (nColor) {
				if (gameStates.render.bPerPixelLighting == 2)
					*pColor = faceColor [nColor]; // multiply the material color in for not lightmap driven lighting models
				else
					*pColor *= faceColor [nColor]; // multiply the material color in for not lightmap driven lighting models
				}
			pColor->Alpha () = fAlpha;
			}
		}
	lightManager.Material ().bValid = 0;
	}
ogl.SetTransform (0);
}

//------------------------------------------------------------------------------

void FixTriangleFan (CSegment* pSeg, CSegFace* pFace)
{
if (pSeg->Type (pFace->m_info.nSide) == SIDE_IS_TRI_13) 
#if USE_OPEN_MP > 1
#pragma omp critical
#endif
	{	//rearrange vertex order for TRIANGLE_FAN rendering
#if !USE_OPENMP
	SDL_mutexP (tiRender.semaphore);
#endif
	pSeg->SetType (pFace->m_info.nSide, pFace->m_info.nType = SIDE_IS_TRI_02);

	int16_t	h = pFace->m_info.index [0];
	memcpy (pFace->m_info.index, pFace->m_info.index + 1, 3 * sizeof (int16_t));
	pFace->m_info.index [3] = h;

	CFloatVector3 v = FACES.vertices [pFace->m_info.nIndex];
	memcpy (FACES.vertices + pFace->m_info.nIndex, FACES.vertices + pFace->m_info.nIndex + 1, 3 * sizeof (CFloatVector3));
	FACES.vertices [pFace->m_info.nIndex + 3] = v;

	tTexCoord2f tc = FACES.texCoord [pFace->m_info.nIndex];
	memcpy (FACES.texCoord + pFace->m_info.nIndex, FACES.texCoord + pFace->m_info.nIndex + 1, 3 * sizeof (tTexCoord2f));
	FACES.texCoord [pFace->m_info.nIndex + 3] = tc;
	tc = FACES.lMapTexCoord [pFace->m_info.nIndex];
	memcpy (FACES.lMapTexCoord + pFace->m_info.nIndex, FACES.lMapTexCoord + pFace->m_info.nIndex + 1, 3 * sizeof (tTexCoord2f));
	FACES.lMapTexCoord [pFace->m_info.nIndex + 3] = tc;
	if (pFace->m_info.nOvlTex) {
		tc = FACES.ovlTexCoord [pFace->m_info.nIndex];
		memcpy (FACES.ovlTexCoord + pFace->m_info.nIndex, FACES.ovlTexCoord + pFace->m_info.nIndex + 1, 3 * sizeof (tTexCoord2f));
		FACES.ovlTexCoord [pFace->m_info.nIndex + 3] = tc;
		}
#if !USE_OPENMP
	SDL_mutexV (tiRender.semaphore);
#endif
	}
}

//------------------------------------------------------------------------------

void ComputeDynamicQuadLight (int32_t nStart, int32_t nEnd, int32_t nThread)
{
#if 0
	static int32_t bSemaphore [2] = {0, 0};

while (bSemaphore [nThread])
	G3_SLEEP (0);
bSemaphore [nThread] = 1;
#endif
	CSegment*		pSeg;
	tSegFaces*		pSegFace;
	CSegFace*		pFace;
	CFloatVector*	pColor;
	CFaceColor		faceColor [3];
#if 0
	uint8_t			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	int32_t			nVertex, nSegment, nSide;
	float			fAlpha;
	int32_t			h, i, j, nColor, nLights = 0,
					bVertexLight = gameStates.render.bPerPixelLighting != 2;
	static		CStaticFaceColor<255,255,255,255> brightColor1;
	static		CStaticFaceColor<127,127,127,127> brightColor2;
	static		CFaceColor* brightColors [2] = {&brightColor1, &brightColor2};

//memset (&gameData.render.lights.dynamic.shader.index, 0, sizeof (gameData.render.lights.dynamic.shader.index));
for (i = 0; i < 3; i++)
	faceColor [i].Set (0.0f, 0.0f, 0.0f, 1.0f);
ogl.SetTransform (1);
gameStates.render.nState = 0;
#	if GPGPU_VERTEX_LIGHTING
if (ogl.m_states.bVertexLighting)
	ogl.m_states.bVertexLighting = gpgpuLighting.Compute (-1, 0, NULL);
#endif
for (i = nStart; i < nEnd; i++) {
	if (0 > (nSegment = gameData.render.mine.visibility [0].segments [i]))
		continue;
	pSeg = SEGMENT (nSegment);
	pSegFace = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (pSeg))) {
		gameData.render.mine.visibility [0].segments [i] = -gameData.render.mine.visibility [0].segments [i];
		continue;
		}
#if DBG
	if (nSegment == nDbgSeg)
		BRP;
#endif
	if (bVertexLight && !gameStates.render.bFullBright)
		nLights = lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
	for (j = pSegFace->nFaces, pFace = pSegFace->pFace; j; j--, pFace++) {
		nSide = pFace->m_info.nSide;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
			BRP;
			if (lightManager.Index (0, nThread).nActive)
				BRP;
			}
#endif
		if (0 > (nColor = SetupFace (nSegment, nSide, pSeg, pFace, faceColor, fAlpha))) {
			pFace->m_info.bVisible = 0;
			continue;
			}
		if (!AddFaceListItem (pFace, nThread))
			continue;
		FixTriangleFan (pSeg, pFace);
		faceColor [nColor].Alpha () = fAlpha;
		pFace->m_info.color.Assign (faceColor [nColor]);
		if (gameStates.render.bPerPixelLighting && (nColor == 1))
			nColor = -1;
//			SetDynLightMaterial (nSegment, pFace->m_info.nSide, -1);
		pColor = FACES.color + pFace->m_info.nIndex;
		for (h = 0; h < pFace->m_info.nVerts; h++, pColor++) {
			if (gameStates.render.bFullBright)
				pColor->Assign (nColor ? faceColor [nColor] : *brightColors [gameStates.render.bFullBright - 1]);
			else {
				nVertex = pFace->m_info.index [h];
#if DBG
				if (nVertex == nDbgVertex)
					BRP;
#endif
				CFaceColor *pVertexColor = gameData.render.color.vertices + nVertex;
				if (UpdateColor (pVertexColor)) {
#if DBG
					if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
						BRP;
#endif
					if (nLights + lightManager.VariableVertLights (nVertex) == 0) {
						*pVertexColor = gameData.render.color.ambient [nVertex];
						pVertexColor->index = gameStates.render.nFrameFlipFlop + 1;
						}
					else {
#if DBG
						if (nVertex == nDbgVertex)
							BRP;
#endif
						GetVertexColor (nSegment, nSide, nVertex, RENDERPOINTS [nVertex].GetNormal ()->XYZ (), FVERTICES [nVertex].XYZ (), NULL, &gameData.render.color.ambient [nVertex], 1, 0, nThread);
						lightManager.Index (0, nThread) = lightManager.Index (1, nThread);
						lightManager.ResetNearestToVertex (nVertex, nThread);
						}
					}
#if DBG
				if (nVertex == nDbgVertex)
					BRP;
#endif
				*pColor = *pVertexColor;
				if (!gameStates.render.bPerPixelLighting && (nColor > 0))
					AlphaBlend (*pColor, faceColor [nColor], fAlpha);
				pColor->Alpha () = fAlpha;
				}
			}
		lightManager.Material ().bValid = 0;
		}
	}
#	if GPGPU_VERTEX_LIGHTING
if (ogl.m_states.bVertexLighting)
	gpgpuLighting.Compute (-1, 2, NULL);
#endif
ogl.SetTransform (0);
#if 0
bSemaphore [nThread] = 0;
#endif
}

//------------------------------------------------------------------------------

void ComputeDynamicTriangleLight (int32_t nStart, int32_t nEnd, int32_t nThread)
{
	CSegment*		pSeg;
	tSegFaces*		pSegFace;
	CSegFace*		pFace;
	tFaceTriangle*	pTriangle;
	CFloatVector*	pColor;
	CFaceColor		faceColor [3];
#if 0
	uint8_t			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	int32_t			nVertex, nSegment, nSide;
	float			fAlpha;
	int32_t			h, i, j, k, nIndex, nColor, nLights = 0;
	bool			bNeedLight = !gameStates.render.bFullBright && (gameStates.render.bPerPixelLighting != 2);
#if DBG
	CShortArray& visibleSegs = gameData.render.mine.visibility [0].segments;
#else
	int16_t*		visibleSegs = gameData.render.mine.visibility [0].segments.Buffer ();
#endif
	static		CStaticFaceColor<255,255,255,255> brightColor1;
	static		CStaticFaceColor<127,127,127,127> brightColor2;
	static		CFaceColor* brightColors [2] = {&brightColor1, &brightColor2};

for (i = 0; i < 3; i++)
	faceColor [i].Set (1.0f, 1.0f, 1.0f, 1.0f);
ogl.SetTransform (1);
gameStates.render.nState = 0;
#	if GPGPU_VERTEX_LIGHTING
if (ogl.m_states.bVertexLighting)
	ogl.m_states.bVertexLighting = gpgpuLighting.Compute (-1, 0, NULL);
#endif
for (i = nStart; i < nEnd; i++) {
	if (0 > (nSegment = visibleSegs [i]))
		continue;
	pSeg = SEGMENT (nSegment);
	pSegFace = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (pSeg))) {
		visibleSegs [i] = -visibleSegs [i] - 1;
		continue;
		}
#if DBG
	if (nSegment == nDbgSeg)
		BRP;
#endif
	bool bComputeLight = bNeedLight;
	for (j = pSegFace->nFaces, pFace = pSegFace->pFace; j; j--, pFace++) {
		nSide = pFace->m_info.nSide;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
			BRP;
			if (lightManager.Index (0, nThread).nActive)
				BRP;
			}
#endif
		if (0 > (nColor = SetupFace (nSegment, nSide, pSeg, pFace, faceColor, fAlpha))) {
			pFace->m_info.bVisible = 0;
			continue;
			}
		if (!AddFaceListItem (pFace, nThread))
			continue;
		//faceColor [nColor].Alpha () = fAlpha;
		pFace->m_info.color.Assign (faceColor [nColor]);
		if (!(bNeedLight || nColor) && pFace->m_info.bHasColor)
			continue;
		if (bComputeLight) {
			nLights = lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
			bComputeLight = false;
			}
		//if (gameStates.render.bPerPixelLighting && (nColor == 1))
		//	nColor = -1;
		pFace->m_info.bHasColor = 1;
		for (k = pFace->m_info.nTris, pTriangle = FACES.tris + pFace->m_info.nTriIndex; k; k--, pTriangle++) {
			nIndex = pTriangle->nIndex;
			pColor = FACES.color + nIndex;
			for (h = 0; h < 3; h++, pColor++, nIndex++) {
				if (gameStates.render.bFullBright)
					*pColor = nColor ? faceColor [nColor] : *brightColors [gameStates.render.bFullBright - 1];
				else {
					if (gameStates.render.bPerPixelLighting == 2)
						*pColor = *brightColors [0]; //gameData.render.color.ambient [nVertex];
					else {
						nVertex = pTriangle->index [h];
#if DBG
						if (nVertex == nDbgVertex)
							BRP;
#endif
						CFaceColor *pVertexColor = gameData.render.color.vertices + nVertex;
#if LIGHTING_QUALITY == 1
						WaitWithUpdate (pVertexColor);
#else
						if (UpdateColor (pVertexColor))
#endif
							{
#if DBG
							if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
								BRP;
#endif
							if (nLights + lightManager.VariableVertLights (nVertex) == 0) { // no dynamic lights => only ambient light contribution
								*pVertexColor = gameData.render.color.ambient [nVertex];
								pVertexColor->index = gameStates.render.nFrameFlipFlop + 1;
								}
							else {
#if LIGHTING_QUALITY == 1
								GetVertexColor (nSegment, nSide, nVertex, FACES.normals + nIndex, FACES.vertices + nIndex, NULL, &c, 1, 0, nThread);
#else
								GetVertexColor (nSegment, nSide, nVertex, RENDERPOINTS [nVertex].GetNormal ()->XYZ (), FACES.vertices + nIndex, NULL, NULL, 1, 0, nThread);
#endif
								lightManager.Index (0, nThread) = lightManager.Index (1, nThread);
								lightManager.ResetNearestToVertex (nVertex, nThread);
								}
#	if DBG
							if (nVertex == nDbgVertex) {
								BRP;
								pVertexColor->index = -1;
								GetVertexColor (nSegment, nSide, nVertex, FACES.normals + nIndex, FACES.vertices + nIndex, NULL, NULL, 1, 0, nThread);
								}
#	endif
							}
						*pColor = *pVertexColor;
						}
					if (nColor > 0)
						*pColor *= faceColor [nColor];
					pColor->Alpha () = fAlpha;
					}
				}
			}
		lightManager.Material ().bValid = 0;
		}
	}
#	if GPGPU_VERTEX_LIGHTING
if (ogl.m_states.bVertexLighting)
	gpgpuLighting.Compute (-1, 2, NULL);
#endif
ogl.SetTransform (0);
}

//------------------------------------------------------------------------------

void ComputeStaticFaceLight (int32_t nStart, int32_t nEnd, int32_t nThread)
{
	CSegment*		pSeg;
	CSide*			pSide;
	tSegFaces*		pSegFace;
	CSegFace*		pFace;
	CFloatVector*	pColor;
	CFaceColor		c, faceColor [3];
#if 0
	uint8_t			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	int32_t			nVertex, nSegment, nSide;
	fix			xLight;
	float			fAlpha;
	tUVL			*uvlP;
	int32_t			h, i, j, uvi, nColor;

	static		CStaticFaceColor<255,255,255,255> brightColor1;
	static		CStaticFaceColor<127,127,127,127> brightColor2;
	static		CFaceColor* brightColors [2] = {&brightColor1, &brightColor2};

for (i = 0; i < 3; i++)
	faceColor [i].Set (0.0f, 0.0f, 0.0f, 1.0f);
ogl.SetTransform (1);
gameStates.render.nState = 0;
for (i = nStart; i < nEnd; i++) {
	if (0 > (nSegment = gameData.render.mine.visibility [0].segments [i]))
		continue;
	pSeg = SEGMENT (nSegment);
	pSegFace = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (pSeg))) {
		gameData.render.mine.visibility [0].segments [i] = -gameData.render.mine.visibility [0].segments [i] - 1;
		continue;
		}
	for (j = pSegFace->nFaces, pFace = pSegFace->pFace; j; j--, pFace++) {
		nSide = pFace->m_info.nSide;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			BRP;
#endif
		if (0 > (nColor = SetupFace (nSegment, nSide, pSeg, pFace, faceColor, fAlpha))) {
			pFace->m_info.bVisible = 0;
			continue;
			}
		if (!AddFaceListItem (pFace, nThread))
			continue;
		pFace->m_info.color.Assign (faceColor [nColor]);
		pColor = FACES.color + pFace->m_info.nIndex;
		pSide = pSeg->Side (nSide);
		uvlP = pSide->m_uvls;
		for (h = 0, uvi = 0 /*(pSeg->m_sides [nSide].m_nType == SIDE_IS_TRI_13)*/; h < pSide->CornerCount (); h++, pColor++, uvi++) {
			if (gameStates.render.bFullBright)
				*pColor = nColor ? faceColor [nColor] : *brightColors [gameStates.render.bFullBright - 1];
			else {
				c = faceColor [nColor];
				nVertex = pFace->m_info.index [h];
#if DBG
				if (nVertex == nDbgVertex)
					BRP;
#endif
				SetVertexColor (nVertex, &c, nColor ? pFace->m_info.bTextured ? 2 : 1 : 0);
				xLight = SetVertexLight (nSegment, nSide, nVertex, &c, uvlP [uvi % pSide->CornerCount ()].l);
				AdjustVertexColor (NULL, &c, xLight);
				}
			*pColor = c;
			if (nColor > 0)
				*pColor *= faceColor [nColor];
			pColor->Alpha () = fAlpha;
			}
		}
	}
ogl.SetTransform (0);
}

//------------------------------------------------------------------------------

int32_t CountRenderFaces (void)
{
	CSegment*	pSeg;
	int16_t		nSegment;
	int32_t		h, i, j, nFaces, nSegments;

ogl.m_states.bUseTransform = 1; // prevent vertex transformation from setting FVERTICES!
for (i = nSegments = nFaces = 0; i < gameData.render.mine.visibility [0].nSegments; i++) {
	pSeg = SEGMENT (gameData.render.mine.visibility [0].segments [i]);
	if (SegmentIsVisible (pSeg)) {
		nSegments++;
		nFaces += SEGFACES [i].nFaces;
		}
	else
		gameData.render.mine.visibility [0].segments [i] = -gameData.render.mine.visibility [0].segments [i] - 1;
	}
tiRender.nMiddle = 0;
if (nFaces) {
	for (h = nFaces / 2, i = j = 0; i < gameData.render.mine.visibility [0].nSegments; i++) {
		if (0 > (nSegment = gameData.render.mine.visibility [0].segments [i]))
			continue;
		j += SEGFACES [nSegment].nFaces;
		if (j > h) {
			tiRender.nMiddle = i;
			break;
			}
		}
	}
ogl.m_states.bUseTransform = 0;
return nSegments;
}

//------------------------------------------------------------------------------
// eof
