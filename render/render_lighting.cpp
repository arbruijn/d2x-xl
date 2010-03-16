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
if (automap.Display ())
	return 1;
return RotateVertexList (8, segP->m_verts).ccAnd == 0;
}

//------------------------------------------------------------------------------

static int FaceIsVisible (CSegFace* faceP)
{
if (!FaceIsVisible (faceP->m_info.nSegment, faceP->m_info.nSide))
	return faceP->m_info.bVisible = 0;
if ((faceP->m_info.bSparks == 1) && gameOpts->render.effects.bEnabled && gameOpts->render.effects.bEnergySparks)
	return faceP->m_info.bVisible = 0;
return faceP->m_info.bVisible = 1;
}

//------------------------------------------------------------------------------

int SetupFace (short nSegment, short nSide, CSegment *segP, CSegFace *faceP, tFaceColor *pFaceColor, float *pfAlpha)
{
	ubyte	bTextured, bCloaked, bTransparent, bWall;
	int	nColor = 0;

#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if (FACE_IDX (faceP) == nDbgFace)
	nDbgFace = nDbgFace;
#endif

if (!FaceIsVisible (faceP))
	return -1;
bWall = IS_WALL (faceP->m_info.nWall);
if (bWall) {
	faceP->m_info.widFlags = segP->IsDoorWay (nSide, NULL);
	if (!(faceP->m_info.widFlags & WID_RENDER_FLAG)) //(WID_RENDER_FLAG | WID_RENDPAST_FLAG)))
		return -1;
	}
else
	faceP->m_info.widFlags = WID_RENDER_FLAG;
faceP->m_info.nCamera = IsMonitorFace (nSegment, nSide, 0);
bTextured = 1;
bCloaked = 0;
bTransparent = 0;
if (bWall)
	*pfAlpha = WallAlpha (nSegment, nSide, faceP->m_info.nWall, faceP->m_info.widFlags, faceP->m_info.nCamera >= 0, faceP->m_info.bAdditive,
								 &pFaceColor [1].color, &nColor, &bTextured, &bCloaked, &bTransparent);
else
	*pfAlpha = 1;
faceP->m_info.bTextured = bTextured;
faceP->m_info.bCloaked = bCloaked;
faceP->m_info.bTransparent |= bTransparent;
if (faceP->m_info.bSegColor) {
	if ((faceP->m_info.nSegColor = IsColoredSegFace (nSegment, nSide))) {
		faceP->m_info.color = *ColoredSegmentColor (nSegment, nSide, faceP->m_info.nSegColor);
		pFaceColor [2].color = faceP->m_info.color;
		if (faceP->m_info.nBaseTex < 0)
			*pfAlpha = faceP->m_info.color.alpha;
		nColor = 2;
		}
	else
		faceP->m_info.bVisible = (faceP->m_info.nBaseTex >= 0);
	}
if ((*pfAlpha < 1) || ((nColor == 2) && (faceP->m_info.nBaseTex < 0)))
	faceP->m_info.bTransparent = 1;
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
	int			h, i, nColor, nLights = 0;
	//int				bVertexLight = gameStates.render.bPerPixelLighting != 2;
	int				bLightmaps = lightmapManager.HaveLightmaps ();
	bool			bNeedLight = !gameStates.render.bFullBright && (gameStates.render.bPerPixelLighting != 2);
	static		tFaceColor brightColor = {{1,1,1,1},1};

//memset (&gameData.render.lights.dynamic.shader.index, 0, sizeof (gameData.render.lights.dynamic.shader.index));
ogl.SetTransform (1);
gameStates.render.nState = 0;
#	if GPGPU_VERTEX_LIGHTING
if (ogl.m_states.bVertexLighting)
	ogl.m_states.bVertexLighting = gpgpuLighting.Compute (-1, 0, NULL);
#endif

for (i = nStart; i < nEnd; i++) {
	faceP = FACES.faces + i;
	nSegment = faceP->m_info.nSegment;
	nSide = faceP->m_info.nSide;

#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (0 > (nColor = SetupFace (nSegment, nSide, SEGMENTS + nSegment, faceP, faceColor, &fAlpha))) {
		faceP->m_info.bVisible = 0;
		continue;
		}
	if (bNeedLight)
		nLights = lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
		nSegment = nSegment;
		if (lightManager.Index (0)[nThread].nActive)
			nSegment = nSegment;
		}
#endif
	AddFaceListItem (faceP, nThread);
	faceP->m_info.color = faceColor [nColor].color;
	pc = FACES.color + faceP->m_info.nIndex;
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
				nVertex = faceP->m_info.index [h];
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
							G3VertexColor(gameData.segs.points[nVertex].p3_normal.vNormal.XYZ (),
											  gameData.segs.fVertices[nVertex].XYZ (), nVertex,
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
ogl.SetTransform (0);
}

//------------------------------------------------------------------------------

void FixTriangleFan (CSegment* segP, CSegFace* faceP)
{
if (segP->Type (faceP->m_info.nSide) == SIDE_IS_TRI_13) {	//rearrange vertex order for TRIANGLE_FAN rendering
	segP->SetType (faceP->m_info.nSide, faceP->m_info.nType = SIDE_IS_TRI_02);
	{
	short	h = faceP->m_info.index [0];
	memcpy (faceP->m_info.index, faceP->m_info.index + 1, 3 * sizeof (short));
	faceP->m_info.index [3] = h;
	}
	{
	CFloatVector3 h = FACES.vertices [faceP->m_info.nIndex];
	memcpy (FACES.vertices + faceP->m_info.nIndex, FACES.vertices + faceP->m_info.nIndex + 1, 3 * sizeof (CFloatVector3));
	FACES.vertices [faceP->m_info.nIndex + 3] = h;
	}
	{
	tTexCoord2f h = FACES.texCoord [faceP->m_info.nIndex];
	memcpy (FACES.texCoord + faceP->m_info.nIndex, FACES.texCoord + faceP->m_info.nIndex + 1, 3 * sizeof (tTexCoord2f));
	FACES.texCoord [faceP->m_info.nIndex + 3] = h;
	h = FACES.lMapTexCoord [faceP->m_info.nIndex];
	memcpy (FACES.lMapTexCoord + faceP->m_info.nIndex, FACES.lMapTexCoord + faceP->m_info.nIndex + 1, 3 * sizeof (tTexCoord2f));
	FACES.lMapTexCoord [faceP->m_info.nIndex + 3] = h;
	if (faceP->m_info.nOvlTex) {
		h = FACES.ovlTexCoord [faceP->m_info.nIndex];
		memcpy (FACES.ovlTexCoord + faceP->m_info.nIndex, FACES.ovlTexCoord + faceP->m_info.nIndex + 1, 3 * sizeof (tTexCoord2f));
		FACES.ovlTexCoord [faceP->m_info.nIndex + 3] = h;
		}
	}
	}
}

//------------------------------------------------------------------------------

void ComputeDynamicQuadLight (int nStart, int nEnd, int nThread)
{
#if 0
	static int bSemaphore [2] = {0, 0};

while (bSemaphore [nThread])
	G3_SLEEP (0);
bSemaphore [nThread] = 1;
#endif
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
					bVertexLight = gameStates.render.bPerPixelLighting != 2,
					bLightmaps = lightmapManager.HaveLightmaps ();
	static		tFaceColor brightColor = {{1,1,1,1},1};

//memset (&gameData.render.lights.dynamic.shader.index, 0, sizeof (gameData.render.lights.dynamic.shader.index));
ogl.SetTransform (1);
gameStates.render.nState = 0;
#	if GPGPU_VERTEX_LIGHTING
if (ogl.m_states.bVertexLighting)
	ogl.m_states.bVertexLighting = gpgpuLighting.Compute (-1, 0, NULL);
#endif
for (i = nStart; i < nEnd; i++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [0][i]))
		continue;
	segP = SEGMENTS + nSegment;
	segFaceP = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [0][i] = -gameData.render.mine.nSegRenderList [0][i];
		continue;
		}
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (bVertexLight && !gameStates.render.bFullBright)
		nLights = lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
	for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++) {
		nSide = faceP->m_info.nSide;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
			nSegment = nSegment;
			if (lightManager.Index (0)[nThread].nActive)
				nSegment = nSegment;
			}
#endif
		if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
			faceP->m_info.bVisible = 0;
			continue;
			}
		if (!AddFaceListItem (faceP, nThread))
			continue;
		FixTriangleFan (segP, faceP);
		faceP->m_info.color = faceColor [nColor].color;
//			SetDynLightMaterial (nSegment, faceP->m_info.nSide, -1);
		pc = FACES.color + faceP->m_info.nIndex;
		for (h = 0; h < 4; h++, pc++) {
			if (gameStates.render.bFullBright)
				*pc = nColor ? faceColor [nColor].color : brightColor.color;
			else {
				c = faceColor [nColor];
				if (nColor)
					*pc = c.color;
				if (!bLightmaps) {
					nVertex = faceP->m_info.index [h];
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
								G3VertexColor (gameData.segs.points [nVertex].p3_normal.vNormal.XYZ (),
													gameData.segs.fVertices [nVertex].XYZ (), nVertex,
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
if (ogl.m_states.bVertexLighting)
	gpgpuLighting.Compute (-1, 2, NULL);
#endif
PROF_END(ptLighting)
ogl.SetTransform (0);
#if 0
bSemaphore [nThread] = 0;
#endif
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
	int			h, i, j, k, nIndex, nColor, nLights = 0;
	bool			bNeedLight = !gameStates.render.bFullBright && (gameStates.render.bPerPixelLighting != 2);

	static		tFaceColor brightColor = {{1,1,1,1},1};

lightManager.ResetIndex ();
ogl.SetTransform (1);
gameStates.render.nState = 0;
#	if GPGPU_VERTEX_LIGHTING
if (ogl.m_states.bVertexLighting)
	ogl.m_states.bVertexLighting = gpgpuLighting.Compute (-1, 0, NULL);
#endif
for (i = nStart; i < nEnd; i++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [0][i]))
		continue;
	segP = SEGMENTS + nSegment;
	segFaceP = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [0][i] = -gameData.render.mine.nSegRenderList [0][i] - 1;
		continue;
		}
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (bNeedLight)
		nLights = lightManager.SetNearestToSegment (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in lightManager.SetNearestToVertex ())
	for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++) {
		nSide = faceP->m_info.nSide;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
			nSegment = nSegment;
			if (lightManager.Index (0)[nThread].nActive)
				nSegment = nSegment;
			}
#endif
		if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
			faceP->m_info.bVisible = 0;
			continue;
			}
		if (!AddFaceListItem (faceP, nThread))
			continue;
		faceP->m_info.color = faceColor [nColor].color;
		if (!(bNeedLight || nColor) && faceP->m_info.bHasColor)
			continue;
		faceP->m_info.bHasColor = 1;
		for (k = faceP->m_info.nTris, triP = FACES.tris + faceP->m_info.nTriIndex; k; k--, triP++) {
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
								G3VertexColor (FACES.normals + nIndex, FACES.vertices + nIndex, nVertex, NULL, &c, 1, 0, nThread);
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
if (ogl.m_states.bVertexLighting)
	gpgpuLighting.Compute (-1, 2, NULL);
#endif
PROF_END(ptLighting)
ogl.SetTransform (0);
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
	int			h, i, j, uvi, nColor;

	static		tFaceColor brightColor = {{1,1,1,1},1};

ogl.SetTransform (1);
gameStates.render.nState = 0;
for (i = nStart; i < nEnd; i++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [0][i]))
		continue;
	segP = SEGMENTS + nSegment;
	segFaceP = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [0][i] = -gameData.render.mine.nSegRenderList [0][i] - 1;
		continue;
		}
	for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++) {
		nSide = faceP->m_info.nSide;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			nSegment = nSegment;
#endif
		if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
			faceP->m_info.bVisible = 0;
			continue;
			}
		FixTriangleFan (segP, faceP);
		if (!AddFaceListItem (faceP, nThread))
			continue;
		faceP->m_info.color = faceColor [nColor].color;
		pc = FACES.color + faceP->m_info.nIndex;
		uvlP = segP->m_sides [nSide].m_uvls;
		for (h = 0, uvi = 0 /*(segP->m_sides [nSide].m_nType == SIDE_IS_TRI_13)*/; h < 4; h++, pc++, uvi++) {
			if (gameStates.render.bFullBright)
				*pc = nColor ? faceColor [nColor].color : brightColor.color;
			else {
				c = faceColor [nColor];
				nVertex = faceP->m_info.index [h];
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
ogl.SetTransform (0);
}

//------------------------------------------------------------------------------

int CountRenderFaces (void)
{
	CSegment		*segP;
	short			nSegment;
	int			h, i, j, nFaces, nSegments;

for (i = nSegments = nFaces = 0; i < gameData.render.mine.nRenderSegs; i++) {
	segP = SEGMENTS + gameData.render.mine.nSegRenderList [0][i];
	if (SegmentIsVisible (segP)) {
		nSegments++;
		nFaces += SEGFACES [i].nFaces;
		}
	else
		gameData.render.mine.nSegRenderList [0][i] = -gameData.render.mine.nSegRenderList [0][i];
	}
tiRender.nMiddle = 0;
if (nFaces) {
	for (h = nFaces / 2, i = j = 0; i < gameData.render.mine.nRenderSegs; i++) {
		if (0 > (nSegment = gameData.render.mine.nSegRenderList [0][i]))
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
	nSegment = gameData.render.mine.nSegRenderList [0][h];
	segFaceP = SEGFACES + nSegment;
	if (!SegmentIsVisible (SEGMENTS + nSegment))
		gameData.render.mine.nSegRenderList [0][h] = -gameData.render.mine.nSegRenderList [0][i];
	else {
		for (i = segFaceP->nFaces, faceP = segFaceP->faceP; i; i--, faceP++) {
			for (j = 0; j < 4; j++) {
				n = faceP->m_info.index [j];
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
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [0][h]))
		continue;
	segFaceP = SEGFACES + gameData.render.mine.nSegRenderList [0][h];
	for (i = segFaceP->nFaces, faceP = segFaceP->faceP; i; i--, faceP++)
		for (j = 0; j < 4; j++)
			FACES.color [j] = gameData.render.color.vertices [faceP->m_info.index [j]].color;
	}
}

//------------------------------------------------------------------------------
// eof
