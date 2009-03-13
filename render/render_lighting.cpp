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

// -----------------------------------------------------------------------------------

inline int SegmentIsVisible (CSegment *segP)
{
if (automap.m_bDisplay)
	return 1;
return RotateVertexList (8, segP->m_verts).ccAnd == 0;
}

//------------------------------------------------------------------------------

int SetupFace (short nSegment, short nSide, CSegment *segP, CSegFace *faceP, tFaceColor *pFaceColor, float *pfAlpha)
{
	ubyte	bTextured, bCloaked, bWall;
	int	nColor = 0;

#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if (FACE_IDX (faceP) == nDbgFace)
	nDbgFace = nDbgFace;
#endif
bWall = IS_WALL (faceP->nWall);
if (bWall) {
	faceP->widFlags = segP->IsDoorWay (nSide, NULL);
	if (!(faceP->widFlags & (WID_RENDER_FLAG | WID_RENDPAST_FLAG)))
		return -1;
	}
else
	faceP->widFlags = WID_RENDER_FLAG;
faceP->nCamera = IsMonitorFace (nSegment, nSide, 0);
bTextured = 1;
bCloaked = 0;
if (bWall)
	*pfAlpha = WallAlpha (nSegment, nSide, faceP->nWall, faceP->widFlags, faceP->nCamera >= 0, faceP->bAdditive,
								 &pFaceColor [1].color, &nColor, &bTextured, &bCloaked);
else
	*pfAlpha = 1;
faceP->bTextured = bTextured;
faceP->bCloaked = bCloaked;
if (!gameData.app.nFrameCount) {
	if ((faceP->nSegColor = IsColoredSegFace (nSegment, nSide))) {
		pFaceColor [2].color = *ColoredSegmentColor (nSegment, nSide, faceP->nSegColor);
		if (faceP->nBaseTex < 0)
			*pfAlpha = pFaceColor [2].color.alpha;
		nColor = 2;
		}
	}
if ((*pfAlpha < 1) || ((nColor == 2) && (faceP->nBaseTex < 0)))
	faceP->bTransparent = 1;
return nColor;
}

//------------------------------------------------------------------------------

void ComputeDynamicFaceLight (int nStart, int nEnd, int nThread)
{
PROF_START
	CSegFace		*faceP;
	tRgbaColorf	*pc;
	tFaceColor	c, faceColor [3] = {{{0,0,0,1},1},{{0,0,0,0},1},{{0,0,0,1},1}};
#if 0
	ubyte			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	short			nVertex, nSegment, nSide;
	float			fAlpha;
	int			h, i, nColor, nLights = 0, nStep = nStart ? -1 : 1,
					bVertexLight = gameStates.render.bPerPixelLighting != 2,
					bLightmaps = lightmapManager.HaveLightmaps ();
	bool			bNeedLight = !gameStates.render.bFullBright && (gameStates.render.bPerPixelLighting != 2);
	static		tFaceColor brightColor = {{1,1,1,1},1};

#if SORT_RENDER_FACES > 1
ResetFaceList (nThread);
#endif
//memset (&gameData.render.lights.dynamic.shader.index, 0, sizeof (gameData.render.lights.dynamic.shader.index));
gameStates.ogl.bUseTransform = 1;
gameStates.render.nState = 0;
#	if GPGPU_VERTEX_LIGHTING
if (gameStates.ogl.bVertexLighting)
	gameStates.ogl.bVertexLighting = gpgpuLighting.Compute (-1, 0, NULL);
#endif

for (i = nStart, nStep = (nStart > nEnd) ? -1 : 1; i != nEnd; i += nStep) {
	faceP = FACES.faces + i;
	nSegment = faceP->nSegment;
	nSide = faceP->nSide;

#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (faceP->bSparks && gameOpts->render.effects.bEnergySparks) {
		faceP->bVisible = 0;
		continue;
		}
	if (!(faceP->bVisible = FaceIsVisible (nSegment, nSide)))
		continue;

	if (bNeedLight)
		nLights = lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
		nSegment = nSegment;
		if (lightManager.Index (0)[nThread].nActive)
			nSegment = nSegment;
		}
#endif
	if (0 > (nColor = SetupFace (nSegment, nSide, SEGMENTS + nSegment, faceP, faceColor, &fAlpha))) {
		faceP->bVisible = 0;
		continue;
		}
#if SORT_RENDER_FACES > 1
	AddFaceListItem (faceP, nThread);
#endif
	faceP->color = faceColor [nColor].color;
	pc = FACES.color + faceP->nIndex;
	for (h = 0; h < 4; h++, pc++) {
		if (gameStates.render.bFullBright) 
			*pc = nColor ? faceColor [nColor].color : brightColor.color;
		else {
			if (bLightmaps) {
				c = faceColor [nColor];
				if (nColor)
					*pc = c.color;
				}
			else {
				nVertex = faceP->index [h];
#if DBG
				if (nVertex == nDbgVertex)
					nDbgVertex = nDbgVertex;
#endif
				if (gameStates.render.bPerPixelLighting == 2) 
					*pc = gameData.render.color.ambient [nVertex].color;
				else {
					c.color = gameData.render.color.ambient [nVertex].color;
					tFaceColor *pvc = gameData.render.color.vertices + nVertex;
					if (pvc->index != gameStates.render.nFrameFlipFlop + 1) {
						if (nLights + lightManager.VariableVertLights () [nVertex] == 0) {
							pvc->color = c.color;
							pvc->index = gameStates.render.nFrameFlipFlop + 1;
							}
						else {
#if DBG
							if (nVertex == nDbgVertex)
								nDbgVertex = nDbgVertex;
#endif
							G3VertexColor(gameData.segs.points[nVertex].p3_normal.vNormal.V3(), 
											  gameData.segs.fVertices[nVertex].V3(), nVertex, 
											  NULL, &c, 1, 0, nThread);
							lightManager.Index (0)[nThread] = lightManager.Index (1)[nThread];
							lightManager.ResetNearestToVertex (nVertex, nThread);
							}
#if DBG
						if (nVertex == nDbgVertex)
							nVertex = nVertex;
#endif
						}
					*pc = pvc->color;
					}
				}
			if (nColor == 1) {
				c = faceColor [nColor];
				pc->red *= c.color.red;
				pc->blue *= c.color.green;
				pc->green *= c.color.blue;
				}
			pc->alpha = fAlpha;
			}
		}
	lightManager.Material ().bValid = 0;
	}
PROF_END(ptLighting)
gameStates.ogl.bUseTransform = 0;
}

//------------------------------------------------------------------------------

void ComputeDynamicQuadLight (int nStart, int nEnd, int nThread)
{
	static int bSemaphore [2] = {0, 0};

while (bSemaphore [nThread])
	G3_SLEEP (0);
bSemaphore [nThread] = 1;

PROF_START
	CSegment		*segP;
	tSegFaces	*segFaceP;
	CSegFace		*faceP;
	tRgbaColorf	*pc;
	tFaceColor	c, faceColor [3] = {{{0,0,0,1},1},{{0,0,0,0},1},{{0,0,0,1},1}};
#if 0
	ubyte			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	short			nVertex, nSegment, nSide;
	float			fAlpha;
	int			h, i, j, nColor, nLights = 0,
					nStep = nStart ? -1 : 1,
					bVertexLight = gameStates.render.bPerPixelLighting != 2,
					bLightmaps = lightmapManager.HaveLightmaps ();
	static		tFaceColor brightColor = {{1,1,1,1},1};

#if SORT_RENDER_FACES > 1
ResetFaceList (nThread);
#endif
//memset (&gameData.render.lights.dynamic.shader.index, 0, sizeof (gameData.render.lights.dynamic.shader.index));
gameStates.ogl.bUseTransform = 1;
gameStates.render.nState = 0;
#	if GPGPU_VERTEX_LIGHTING
if (gameStates.ogl.bVertexLighting)
	gameStates.ogl.bVertexLighting = gpgpuLighting.Compute (-1, 0, NULL);
#endif
for (i = nStart; i != nEnd; i += nStep) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	segP = SEGMENTS + nSegment;
	segFaceP = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [i] = -gameData.render.mine.nSegRenderList [i];
		continue;
		}
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (bVertexLight && !gameStates.render.bFullBright)
		nLights = lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
	for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++) {
		nSide = faceP->nSide;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
			nSegment = nSegment;
			if (lightManager.Index (0)[nThread].nActive)
				nSegment = nSegment;
			}
#endif
		if (faceP->bSparks && gameOpts->render.effects.bEnergySparks) {
			faceP->bVisible = 0;
			continue;
			}
		if (!(faceP->bVisible = FaceIsVisible (nSegment, nSide)))
			continue;
		if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
			faceP->bVisible = 0;
			continue;
			}
#if SORT_RENDER_FACES > 1
		if (!AddFaceListItem (faceP, nThread))
			continue;
#endif
		faceP->color = faceColor [nColor].color;
//			SetDynLightMaterial (nSegment, faceP->nSide, -1);
		pc = FACES.color + faceP->nIndex;
		for (h = 0; h < 4; h++, pc++) {
			if (gameStates.render.bFullBright) 
				*pc = nColor ? faceColor [nColor].color : brightColor.color;
			else {
				c = faceColor [nColor];
				if (nColor)
					*pc = c.color;
				if (!bLightmaps) {
					nVertex = faceP->index [h];
#if DBG
					if (nVertex == nDbgVertex)
						nDbgVertex = nDbgVertex;
#endif
					if (gameStates.render.bPerPixelLighting == 2) 
						*pc = gameData.render.color.ambient [nVertex].color;
					else {
						tFaceColor *pvc = gameData.render.color.vertices + nVertex;
						if (pvc->index != gameStates.render.nFrameFlipFlop + 1) {
#if DBG
							if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) 
								nSegment = nSegment;
#endif
							if (nLights + lightManager.VariableVertLights ()[nVertex] == 0) {
								pvc->color.red = c.color.red + gameData.render.color.ambient [nVertex].color.red;
								pvc->color.green = c.color.green + gameData.render.color.ambient [nVertex].color.green;
								pvc->color.blue = c.color.blue + gameData.render.color.ambient [nVertex].color.blue;
								pvc->index = gameStates.render.nFrameFlipFlop + 1;
								}
							else {
#if DBG
								if (nVertex == nDbgVertex)
									nDbgVertex = nDbgVertex;
#endif
								G3VertexColor (gameData.segs.points [nVertex].p3_normal.vNormal.V3(), 
													gameData.segs.fVertices [nVertex].V3(), nVertex, 
													NULL, &c, 1, 0, nThread);
								lightManager.Index (0)[nThread] = lightManager.Index (1)[nThread];
								lightManager.ResetNearestToVertex (nVertex, nThread);
								}
#if DBG
							if (nVertex == nDbgVertex)
								nVertex = nVertex;
#endif
							}
						*pc = pvc->color;
						}
					}
				if (nColor == 1) {
					c = faceColor [nColor];
					pc->red *= c.color.red;
					pc->blue *= c.color.green;
					pc->green *= c.color.blue;
					}
				pc->alpha = fAlpha;
				}
			}
		lightManager.Material ().bValid = 0;
		}
	}
#	if GPGPU_VERTEX_LIGHTING
if (gameStates.ogl.bVertexLighting)
	gpgpuLighting.Compute (-1, 2, NULL);
#endif
PROF_END(ptLighting)
gameStates.ogl.bUseTransform = 0;

bSemaphore [nThread] = 0;
}

//------------------------------------------------------------------------------

void ComputeDynamicTriangleLight (int nStart, int nEnd, int nThread)
{
PROF_START
	CSegment*		segP;
	tSegFaces*		segFaceP;
	CSegFace*		faceP;
	tFaceTriangle*	triP;
	tRgbaColorf*	pc;
	tFaceColor		c, faceColor [3] = {{{0,0,0,1},1},{{0,0,0,0},1},{{0,0,0,1},1}};
#if 0
	ubyte			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	short			nVertex, nSegment, nSide;
	float			fAlpha;
	int			h, i, j, k, nIndex, nColor, nLights = 0,
					nStep = nStart ? -1 : 1;
	bool			bNeedLight = !gameStates.render.bFullBright && (gameStates.render.bPerPixelLighting != 2);

	static		tFaceColor brightColor = {{1,1,1,1},1};

#if SORT_RENDER_FACES > 1
ResetFaceList (nThread);
#endif
lightManager.ResetIndex ();
gameStates.ogl.bUseTransform = 1;
gameStates.render.nState = 0;
#	if GPGPU_VERTEX_LIGHTING
if (gameStates.ogl.bVertexLighting)
	gameStates.ogl.bVertexLighting = gpgpuLighting.Compute (-1, 0, NULL);
#endif
for (i = nStart; i != nEnd; i += nStep) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	segP = SEGMENTS + nSegment;
	segFaceP = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [i] = -gameData.render.mine.nSegRenderList [i] - 1;
		continue;
		}
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (bNeedLight)
		nLights = lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
	for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++) {
		nSide = faceP->nSide;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
			nSegment = nSegment;
			if (lightManager.Index (0)[nThread].nActive)
				nSegment = nSegment;
			}
#endif
		if (!(faceP->bVisible = FaceIsVisible (nSegment, nSide)))
			continue;
		if (faceP->bSparks && gameOpts->render.effects.bEnergySparks) {
			faceP->bVisible = 0;
			continue;
			}
		if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
			faceP->bVisible = 0;
			continue;
			}
#if SORT_RENDER_FACES > 1
		if (!AddFaceListItem (faceP, nThread))
			continue;
#endif
		faceP->color = faceColor [nColor].color;
#if 1
		if (!bNeedLight && faceP->bHasColor)
			continue;
		faceP->bHasColor = 1;
#endif
//			SetDynLightMaterial (nSegment, faceP->nSide, -1);
		for (k = faceP->nTris, triP = FACES.tris + faceP->nTriIndex; k; k--, triP++) {
			nIndex = triP->nIndex;
			pc = FACES.color + nIndex;
			for (h = 0; h < 3; h++, pc++, nIndex++) {
				if (gameStates.render.bFullBright) 
					*pc = nColor ? faceColor [nColor].color : brightColor.color;
				else {
					c = faceColor [nColor];
					if (nColor)
						*pc = c.color;
					nVertex = triP->index [h];
#if DBG
					if (nVertex == nDbgVertex)
						nDbgVertex = nDbgVertex;
#endif
					if (gameStates.render.bPerPixelLighting == 2)
						*pc = gameData.render.color.ambient [nVertex].color;
					else {
						tFaceColor *pvc = gameData.render.color.vertices + nVertex;
						if (pvc->index != gameStates.render.nFrameFlipFlop + 1) {
							if (nLights + lightManager.VariableVertLights () [nVertex] == 0) {
								pvc->color.red = c.color.red + gameData.render.color.ambient [nVertex].color.red;
								pvc->color.green = c.color.green + gameData.render.color.ambient [nVertex].color.green;
								pvc->color.blue = c.color.blue + gameData.render.color.ambient [nVertex].color.blue;
								pvc->index = gameStates.render.nFrameFlipFlop + 1;
								}
							else {
								G3VertexColor (FACES.normals + nIndex, 
													FACES.vertices + nIndex, nVertex, 
													NULL, &c, 1, 0, nThread);
								lightManager.Index (0)[nThread] = lightManager.Index (1)[nThread];
								lightManager.ResetNearestToVertex (nVertex, nThread);
								}
#if DBG
							if (nVertex == nDbgVertex)
								nVertex = nVertex;
#endif
							}
						*pc = pvc->color;
						}
					if (nColor) {
						c = faceColor [nColor];
						pc->red *= c.color.red;
						pc->blue *= c.color.green;
						pc->green *= c.color.blue;
						}
					pc->alpha = fAlpha;
					}
				}
			}
		lightManager.Material ().bValid = 0;
		}
	}
#	if GPGPU_VERTEX_LIGHTING
if (gameStates.ogl.bVertexLighting)
	gpgpuLighting.Compute (-1, 2, NULL);
#endif
PROF_END(ptLighting)
gameStates.ogl.bUseTransform = 0;
}

//------------------------------------------------------------------------------

void ComputeStaticFaceLight (int nStart, int nEnd, int nThread)
{
	CSegment		*segP;
	tSegFaces	*segFaceP;
	CSegFace		*faceP;
	tRgbaColorf	*pc;
	tFaceColor	c, faceColor [3] = {{{0,0,0,1},1},{{0,0,0,0},1},{{0,0,0,1},1}};
#if 0
	ubyte			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	short			nVertex, nSegment, nSide;
	fix			xLight;
	float			fAlpha;
	tUVL			*uvlP;
	int			h, i, j, uvi, nColor, 
					nStep = nStart ? -1 : 1;

	static		tFaceColor brightColor = {{1,1,1,1},1};

#if SORT_RENDER_FACES > 1
ResetFaceList (nThread);
#endif
gameStates.ogl.bUseTransform = 1;
gameStates.render.nState = 0;
for (i = nStart; i != nEnd; i += nStep) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	segP = SEGMENTS + nSegment;
	segFaceP = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [i] = -gameData.render.mine.nSegRenderList [i] - 1;
		continue;
		}
	for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++) {
		nSide = faceP->nSide;
		if (!(faceP->bVisible = FaceIsVisible (nSegment, nSide)))
			continue;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			nSegment = nSegment;
#endif
		if (faceP->bSparks && gameOpts->render.effects.bEnergySparks) {
			faceP->bVisible = 0;
			continue;
			}
		if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
			faceP->bVisible = 0;
			continue;
			}
#if SORT_RENDER_FACES > 1
		if (!AddFaceListItem (faceP, nThread))
			continue;
#endif
		faceP->color = faceColor [nColor].color;
		pc = FACES.color + faceP->nIndex;
		uvlP = segP->m_sides [nSide].m_uvls;
		for (h = 0, uvi = 0 /*(segP->m_sides [nSide].m_nType == SIDE_IS_TRI_13)*/; h < 4; h++, pc++, uvi++) {
			if (gameStates.render.bFullBright) 
				*pc = nColor ? faceColor [nColor].color : brightColor.color;
			else {
				c = faceColor [nColor];
				nVertex = faceP->index [h];
#if DBG
				if (nVertex == nDbgVertex)
					nDbgVertex = nDbgVertex;
#endif
				SetVertexColor (nVertex, &c);
				xLight = SetVertexLight (nSegment, nSide, nVertex, &c, uvlP [uvi % 4].l);
				AdjustVertexColor (NULL, &c, xLight);
				}
			*pc = c.color;
			pc->alpha = fAlpha;
			}
		}
	}
gameStates.ogl.bUseTransform = 0;
}

//------------------------------------------------------------------------------

int CountRenderFaces (void)
{
	CSegment		*segP;
	short			nSegment;
	int			h, i, j, nFaces, nSegments;

for (i = nSegments = nFaces = 0; i < gameData.render.mine.nRenderSegs; i++) {
	segP = SEGMENTS + gameData.render.mine.nSegRenderList [i];
	if (SegmentIsVisible (segP)) {
		nSegments++;
		nFaces += SEGFACES [i].nFaces;
		}
	else
		gameData.render.mine.nSegRenderList [i] = -gameData.render.mine.nSegRenderList [i];
	}
tiRender.nMiddle = 0;
if (nFaces) {
	for (h = nFaces / 2, i = j = 0; i < gameData.render.mine.nRenderSegs; i++) {
		if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
			continue;
		j += SEGFACES [nSegment].nFaces;
		if (j > h) {
			tiRender.nMiddle = i;
			break;
			}
		}
	}
return nSegments;
}

//------------------------------------------------------------------------------

ushort renderVertices [MAX_VERTICES_D2X];
ubyte	 bIsRenderVertex [MAX_VERTICES_D2X];
int	 nRenderVertices;

void GetRenderVertices (void)
{
	tSegFaces	*segFaceP;
	CSegFace		*faceP;
	short			nSegment;
	int			h, i, j, n;

nRenderVertices = 0;
for (h = i = 0; h < gameData.render.mine.nRenderSegs; h++) {
	nSegment = gameData.render.mine.nSegRenderList [h];
	segFaceP = SEGFACES + nSegment;
	if (!SegmentIsVisible (SEGMENTS + nSegment)) 
		gameData.render.mine.nSegRenderList [h] = -gameData.render.mine.nSegRenderList [i];
	else {
		for (i = segFaceP->nFaces, faceP = segFaceP->faceP; i; i--, faceP++) {
			for (j = 0; j < 4; j++) {
				n = faceP->index [j];
				if (bIsRenderVertex [n] != gameStates.render.nFrameFlipFlop + 1) {
					bIsRenderVertex [n] = gameStates.render.nFrameFlipFlop + 1;
					renderVertices [nRenderVertices++] = n;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void SetFaceColors (void)
{
	tSegFaces	*segFaceP;
	CSegFace		*faceP;
	short			nSegment;
	int			h, i, j;

nRenderVertices = 0;
for (h = 0; h < gameData.render.mine.nRenderSegs; h++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [h]))
		continue;
	segFaceP = SEGFACES + gameData.render.mine.nSegRenderList [h];
	for (i = segFaceP->nFaces, faceP = segFaceP->faceP; i; i--, faceP++)
		for (j = 0; j < 4; j++)
			FACES.color [j] = gameData.render.color.vertices [faceP->index [j]].color;
	}
}

//------------------------------------------------------------------------------
// eof
