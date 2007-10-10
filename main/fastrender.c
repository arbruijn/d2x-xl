/* $Id: render.c, v 1.18 2003/10/10 09:36:35 btb Exp $ */
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

#include "inferno.h"
#include "u_mem.h"
#include "lighting.h"
#include "lightmap.h"
#include "automap.h"
#include "texmerge.h"
#include "glare.h"
#include "render.h"
#include "renderlib.h"
#include "renderthreads.h"
#include "transprender.h"
#include "fastrender.h"

#define RENDER_DEPTHMASK_FIRST 1

//------------------------------------------------------------------------------

void LoadFaceBitmaps (tSegment *segP, grsFace *faceP)
{
	short	nFrame;
	tSide	*sideP;

if (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk) {
	sideP = segP->sides + faceP->nSide;
	nFrame = sideP->nFrame;
	if (faceP->bOverlay)
		faceP->nBaseTex = sideP->nOvlTex;
	else {
		faceP->nBaseTex = sideP->nBaseTex;
		if (!faceP->bSplit)
			faceP->nOvlTex = sideP->nOvlTex;
		}
	faceP->bmBot = LoadFaceBitmap (faceP->nBaseTex, nFrame);
	if (nFrame)
		nFrame = nFrame;
	if (faceP->nOvlTex)
		faceP->bmTop = LoadFaceBitmap ((short) (faceP->nOvlTex), nFrame);
	}
else {
	if (faceP->nOvlTex != 0) {
		faceP->bmBot = TexMergeGetCachedBitmap (faceP->nBaseTex, faceP->nOvlTex, faceP->nOvlOrient);
#ifdef _DEBUG
		if (!faceP->bmBot)
			faceP->bmBot = TexMergeGetCachedBitmap (faceP->nBaseTex, faceP->nOvlTex, faceP->nOvlOrient);
#endif
		}
	else {
		faceP->bmBot = gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [faceP->nBaseTex].index;
		PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [faceP->nBaseTex], gameStates.app.bD1Mission);
		}
	}
}

//------------------------------------------------------------------------------

#if RENDER_DEPTHMASK_FIRST

void SplitFace (tSegment *segP, grsFace *faceP)
{
if (gameStates.ogl.bGlTexMerge && (gameStates.render.history.bOverlay < 0) && 
	 faceP->nOvlTex && faceP->bmTop && !strchr (faceP->bmTop->szName, '#')) {	//last rendered face was multi-textured but not super-transparent
		grsFace *newFaceP = segP->pFaces + segP->nFaces++;

	*newFaceP = *faceP;
	newFaceP->nIndex = (newFaceP - 1)->nIndex + 4;
	memcpy (newFaceP->index, faceP->index, sizeof (faceP->index));
	memcpy (gameData.segs.faces.vertices + newFaceP->nIndex, gameData.segs.faces.vertices + faceP->nIndex, 4 * sizeof (fVector3));
	memcpy (gameData.segs.faces.texCoord + newFaceP->nIndex, gameData.segs.faces.ovlTexCoord + faceP->nIndex, 4 * sizeof (tTexCoord2f));
	memcpy (gameData.segs.faces.color + newFaceP->nIndex, gameData.segs.faces.color + faceP->nIndex, 4 * sizeof (tRgbaColorf));
	newFaceP->nBaseTex = faceP->nOvlTex;
	faceP->nOvlTex = newFaceP->nOvlTex = 0;
	faceP->bmTop = NULL;
	faceP->bSplit = 1;
	newFaceP->bOverlay = 1;
	if (newFaceP->bIsLight = IsLight (newFaceP->nBaseTex))
		faceP->bIsLight = 0;
	}
}

#endif

//------------------------------------------------------------------------------

void RenderMineFace (tSegment *segP, grsFace *faceP, short nSegment, int nType, int bVertexArrays, int bTextured, int bDepthOnly)
{
#if 0//def _DEBUG
if (((nDbgSeg >= 0) && (nSegment != nDbgSeg)) || ((nDbgSide >= 0) && (faceP->nSide != nDbgSide)))
	return;
#endif
if ((faceP->nType < 0) && ((faceP->nType = segP->sides [faceP->nSide].nType) == SIDE_IS_TRI_13)) {	//rearrange vertex order for TRIANGLE_FAN rendering
	{
	short	h = faceP->index [0];
	memcpy (faceP->index, faceP->index + 1, 3 * sizeof (short));
	faceP->index [3] = h;
	}
	{
	fVector3 h = gameData.segs.faces.vertices [faceP->nIndex];
	memcpy (gameData.segs.faces.vertices + faceP->nIndex, gameData.segs.faces.vertices + faceP->nIndex + 1, 3 * sizeof (fVector3));
	gameData.segs.faces.vertices [faceP->nIndex + 3] = h;
	}
	{
	tTexCoord2f h = gameData.segs.faces.texCoord [faceP->nIndex];
	memcpy (gameData.segs.faces.texCoord + faceP->nIndex, gameData.segs.faces.texCoord + faceP->nIndex + 1, 3 * sizeof (tTexCoord2f));
	gameData.segs.faces.texCoord [faceP->nIndex + 3] = h;
	if (faceP->nOvlTex) {
		h = gameData.segs.faces.ovlTexCoord [faceP->nIndex];
		memcpy (gameData.segs.faces.ovlTexCoord + faceP->nIndex, gameData.segs.faces.ovlTexCoord + faceP->nIndex + 1, 3 * sizeof (tTexCoord2f));
		gameData.segs.faces.ovlTexCoord [faceP->nIndex + 3] = h;
		}
	}
	}
if (nType == 0) {
	if (IS_WALL (faceP->nWall))
		return;
	}
else if (nType == 1) {
	if (!IS_WALL (faceP->nWall))
		return;
	}
else if (nType == 2) {
	short special = gameData.segs.segment2s [nSegment].special;
	if (((special < SEGMENT_IS_WATER) || (special > SEGMENT_IS_TEAM_RED)) &&
		 (gameData.segs.xSegments [nSegment].owner < 1))
			return;
	}
else {
	tSide *sideP = segP->sides + faceP->nSide;
	if (faceP->bIsLight)
		RenderCorona (nSegment, faceP->nSide);
	return;
	}
#ifdef _DEBUG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nSegment = nSegment;
#endif
if (!(bTextured && faceP->bTextured))
	faceP->bmBot = faceP->bmTop = NULL;
else if ((faceP->nCamera < 0) || !SetupMonitorFace (nSegment, faceP->nSide, faceP->nCamera, faceP))
	LoadFaceBitmaps (segP, faceP);
#ifdef _DEBUG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nSegment = nSegment;
#endif
G3DrawFace (faceP, faceP->bmBot, faceP->bmTop, (faceP->nCamera < 0) || faceP->bTeleport, bVertexArrays, bTextured && faceP->bTextured, bDepthOnly);
#if RENDER_DEPTHMASK_FIRST
if (bTextured)
	SplitFace (segP, faceP);
#endif
}

//------------------------------------------------------------------------------

typedef struct tFaceRef {
	short		nSegment;
	grsFace	*faceP;
	} tFaceRef;

static tFaceRef faceRef [2][MAX_SEGMENTS_D2X * 6];

int QCmpFaces (grsFace *fp, grsFace *mp)
{
if (!fp->bOverlay && mp->bOverlay)
	return -1;
if (fp->bOverlay && !mp->bOverlay)
	return 1;
if (!fp->bTextured && mp->bTextured)
	return -1;
if (fp->bTextured && !mp->bTextured)
	return 1;
if (!fp->nOvlTex && mp->nOvlTex)
	return -1;
if (fp->nOvlTex && !mp->nOvlTex)
	return 1;
if (fp->nBaseTex < mp->nBaseTex)
	return -1;
if (fp->nBaseTex > mp->nBaseTex)
	return -1;
if (fp->nOvlTex < mp->nOvlTex)
	return -1;
if (fp->nOvlTex > mp->nOvlTex)
	return -1;
return 0;
}

//------------------------------------------------------------------------------

void QSortFaces (int left, int right)
{
	int		l = left,
				r = right;
	tFaceRef	*pf = faceRef [0];
	grsFace	m = *pf [(l + r) / 2].faceP;

do {
	while (QCmpFaces (pf [l].faceP, &m) < 0)
		l++;
	while (QCmpFaces (pf [r].faceP, &m) > 0)
		r--;
	if (l <= r) {
		if (l < r) {
			tFaceRef h = pf [l];
			pf [l] = pf [r];
			pf [r] = h;
			}
		l++;
		r--;
		}
	}
while (l < r);
if (l < right)
	QSortFaces (l, right);
if (left < r)
	QSortFaces (left, r);
}

//------------------------------------------------------------------------------

int SortFaces (void)
{
	tSegment	*segP;
	grsFace	*faceP;
	tFaceRef	*ph, *pi, *pj;
	int		h, i, j;
	short		nSegment;

for (h = i = 0, ph = faceRef [0]; i < gameData.render.mine.nRenderSegs; i++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	segP = SEGMENTS + nSegment;
	for (j = segP->nFaces, faceP = segP->pFaces; j; j--, faceP++, h++, ph++) {
		ph->nSegment = nSegment;
		ph->faceP = faceP;
		}
	}
tiRender.nFaces = h;
if (h > 1) {
	if (RunRenderThreads (1)) {
		for (i = h / 2, j = h - i, ph = faceRef [1], pi = faceRef [0], pj = pi + h / 2; h; h--) {
			if (i && (!j || (QCmpFaces (pi->faceP, pj->faceP) <= 0))) {
				*ph++ = *pi++;
				i--;
				}
			else if (j) {
				*ph++ = *pj++;
				j--;
				}
			}
		}
	else 
		QSortFaces (0, h - 1);
	}
return tiRender.nFaces;
}

//------------------------------------------------------------------------------

void RenderFaceList (int nType)
{
	tSegment		*segP;
	grsFace		*faceP;
	tFaceRef		*pfr;
	short			nSegment;
	int			i, j, bVertexArrays;

gameStates.render.nType = nType;
gameStates.render.history.nShader = -1;
gameStates.render.history.bOverlay = 0;
gameStates.render.history.bmBot = 
gameStates.render.history.bmTop = NULL;
gameData.threads.vertColor.data.bDarkness = 0;
if (nType != 3)
	OglSetupTransform (1);
if (bVertexArrays = G3EnableClientStates (1, 1, GL_TEXTURE0)) {
	glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.texCoord);
	glColorPointer (4, GL_FLOAT, 0, gameData.segs.faces.color);
	glVertexPointer (3, GL_FLOAT, 0, gameData.segs.faces.vertices);

	if (bVertexArrays = G3EnableClientStates (1, 1, GL_TEXTURE1)) {
		glVertexPointer (3, GL_FLOAT, 0, gameData.segs.faces.vertices);
		glColorPointer (4, GL_FLOAT, 0, gameData.segs.faces.color);
		glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.ovlTexCoord);
		}
	else
		G3DisableClientStates (1, 1, GL_TEXTURE0);
	}
else {
	G3DisableClientStates (1, 1, GL_TEXTURE1);
	G3DisableClientStates (1, 1, GL_TEXTURE0);
	}
if (nType) {	//back to front
	for (i = gameData.render.mine.nRenderSegs; i; ) {
		if (0 > (nSegment = gameData.render.mine.nSegRenderList [--i]))
			continue;
		if (VISITED (nSegment))
			return;
		VISIT (nSegment);
		segP = SEGMENTS + nSegment;
		for (j = segP->nFaces, faceP = segP->pFaces; j; j--, faceP++) {
#ifdef _DEBUG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
				nSegment = nSegment;
#endif
			RenderMineFace (segP, faceP, nSegment, nType, bVertexArrays, 1, 0);
			}
		}
	}
else {	//front to back
#if RENDER_DEPTHMASK_FIRST
	glColorMask (0,0,0,0);
	glDepthMask (1);
	glDepthFunc (GL_LESS);
#else
	glDepthFunc (GL_LEQUAL);
#endif
	for (i = 0; i < gameData.render.mine.nRenderSegs; i++) {
		if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
			continue;
		if (gameStates.render.automap.bDisplay) {
			if (!(gameStates.render.automap.bFull || bAutomapVisited [nSegment]))
				return;
			if (!gameOpts->render.automap.bSkybox && (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SKYBOX))
				return;
			}
		else
			bAutomapVisited [nSegment] = gameData.render.mine.bSetAutomapVisited;
		if (VISITED (nSegment))
			return;
		VISIT (nSegment);
		segP = SEGMENTS + nSegment;
#ifdef _DEBUG
		if (nSegment == nDbgSeg)
			nSegment = nSegment;
#endif
		for (j = segP->nFaces, faceP = segP->pFaces; j; j--, faceP++)
			RenderMineFace (segP, faceP, nSegment, nType, bVertexArrays, !RENDER_DEPTHMASK_FIRST, RENDER_DEPTHMASK_FIRST);
		}
#if RENDER_DEPTHMASK_FIRST
	j = SortFaces ();
	glColorMask (1,1,1,1);
	glDepthMask (0);
	glDepthFunc (GL_LEQUAL);
	for (i = 0, pfr = faceRef [gameStates.app.bMultiThreaded]; i < j; i++) {
#ifdef _DEBUG
		if ((pfr [i].nSegment == nDbgSeg) && ((nDbgSide < 0) || (pfr[i].faceP->nSide == nDbgSide)))
			nSegment = nSegment;
#endif
		RenderMineFace (SEGMENTS + pfr [i].nSegment, pfr [i].faceP, pfr [i].nSegment, nType, bVertexArrays, 1, 0);
		}
	glDepthMask (1);
#endif
	}
if (bVertexArrays) {
	G3DisableClientStates (1, 1, GL_TEXTURE1);
	glActiveTexture (GL_TEXTURE1);
	glEnable (GL_TEXTURE_2D);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);

	G3DisableClientStates (1, 1, GL_TEXTURE0);
	glActiveTexture (GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	}
if (gameStates.render.history.bOverlay > 0)
	glUseProgramObject (0);
if (nType != 3)
	OglResetTransform (1);
}

// -----------------------------------------------------------------------------------

inline int SegmentIsVisible (tSegment *segP)
{
if (gameStates.render.automap.bDisplay)
	return 1;
return RotateVertexList (8, segP->verts).and == 0;
}

//------------------------------------------------------------------------------

void ComputeFaceLight (int nStart, int nEnd, int nThread)
{
	tSegment		*segP;
	grsFace		*faceP;
	tRgbaColorf	*pc;
	tFaceColor	c, faceColor [3] = {{{0,0,0,1},1},{{0,0,0,0},1},{{0,0,0,1},1}};
	ubyte			bTextured,
					nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~((1 << nThread) | (1 << !nThread))};
	short			nVertex, nSegment, nSide;
	fix			xLight;
	float			fAlpha;
	int			h, i, j, nColor, 
					bDynLight = gameStates.render.bApplyDynLight && !gameStates.app.bEndLevelSequence;

gameStates.ogl.bUseTransform = 1;
gameStates.render.nState = 0;
if (gameStates.render.bFullBright)
	c.color.red = c.color.green = c.color.blue = 1;
for (i = nStart; i < nEnd; i++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	segP = SEGMENTS + nSegment;
	if (!(gameStates.app.bMultiThreaded || SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [i] = -1;
		continue;
		}
	if (bDynLight) {
		if (!gameStates.render.bFullBright)
			SetNearestDynamicLights (nSegment, 0, nThread);
		for (j = segP->nFaces, faceP = segP->pFaces; j; j--, faceP++) {
			nSide = faceP->nSide;
			nColor = 0;
#ifdef _DEBUG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
				nSegment = nSegment;
				if (gameData.render.lights.dynamic.shader.nActiveLights [nThread])
					nSegment = nSegment;
				}
#endif
			faceP->widFlags = WALL_IS_DOORWAY (segP, nSide, NULL);
			faceP->nCamera = IsMonitorFace (nSegment, nSide);
			bTextured = 1;
			fAlpha = WallAlpha (nSegment, nSide, faceP->nWall, faceP->widFlags, faceP->nCamera >= 0, &faceColor [0].color, &nColor, &bTextured);
			faceP->bTextured = bTextured;
			if (!(faceP->bTransparent = (fAlpha < 1)) &&
				 (faceP->nSegColor = IsColoredSegFace (nSegment, nSide))) {
				faceColor [1].color = *ColoredSegmentColor (nSegment, nSide, faceP->nSegColor);
				nColor = 2;
				}
//			SetDynLightMaterial (nSegment, faceP->nSide, -1);
			pc = gameData.segs.faces.color + faceP->nIndex;
			for (h = 0; h < 4; h++, pc++) {
				if (gameStates.render.bFullBright) 
					*pc = c.color;
				else {
					c = faceColor [nColor];
					nVertex = faceP->index [h];
					while (gameData.render.mine.bCalcVertexColor [nVertex] & nThreadFlags [1])
						G3_SLEEP (0);
					gameData.render.mine.bCalcVertexColor [nVertex] |= nThreadFlags [0];
					if (nColor)
						gameData.render.color.vertices [nVertex].index = 0;
					if (gameData.render.color.vertices [nVertex].index != gameStates.render.nFrameFlipFlop + 1) {
						G3VertexColor (&gameData.segs.points [nVertex].p3_normal.vNormal, gameData.segs.fVertices + nVertex, nVertex, 
											NULL, &c, 1, 0, nThread);
						}
					gameData.render.mine.bCalcVertexColor [nVertex] &= nThreadFlags [2];
					if (nColor)
						gameData.render.color.vertices [nVertex].index = 0;
#ifdef _DEBUG
					if (nVertex == nDbgVertex)
						nVertex = nVertex;
#endif
					*pc = gameData.render.color.vertices [nVertex].color;
					}
				pc->alpha = fAlpha;
				}
			gameData.render.lights.dynamic.material.bValid = 0;
			}
		}
	else {
		for (j = segP->nFaces, faceP = segP->pFaces; j; j--, faceP++) {
			nSide = faceP->nSide;
			nColor = 0;
#ifdef _DEBUG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nSegment = nSegment;
#endif
			faceP->widFlags = WALL_IS_DOORWAY (segP, nSide, NULL);
			faceP->nCamera = IsMonitorFace (nSegment, nSide);
			bTextured = 1;
			fAlpha = WallAlpha (nSegment, nSide, faceP->nWall, faceP->widFlags, faceP->nCamera >= 0, &faceColor [1].color, &nColor, &bTextured);
			faceP->bTextured = bTextured;
			if (!(faceP->bTransparent = (fAlpha < 1)) &&
				 (faceP->nSegColor = IsColoredSegFace (nSegment, nSide))) {
				faceColor [1].color = *ColoredSegmentColor (nSegment, nSide, faceP->nSegColor);
				nColor = 1;
				}
			pc = gameData.segs.faces.color + faceP->nIndex;
			for (h = 0; h < 4; h++, pc++) {
				if (!gameStates.render.bFullBright) {
					c = faceColor [nColor];
					nVertex = faceP->index [h];
					SetVertexColor (nVertex, &c);
					xLight = SetVertexLight (nSegment, nSide, nVertex, i, &c, segP->sides [nSide].uvls [h].l);
					AdjustVertexColor (NULL, &c, xLight);
					}
				*pc = c.color;
				pc->alpha = fAlpha;
				}
			}
		}
	}
gameStates.ogl.bUseTransform = 0;
}

//------------------------------------------------------------------------------

void CountRenderFaces (void)
{
	tSegment		*segP;
	short			nSegment;
	int			h, i, j, nFaces;

for (i = nFaces = 0; i < gameData.render.mine.nRenderSegs; i++) {
	segP = SEGMENTS + gameData.render.mine.nSegRenderList [i];
	if (SegmentIsVisible (segP))
		nFaces += segP->nFaces;
	else
		gameData.render.mine.nSegRenderList [i] = -1;
	}
if (!nFaces)
	tiRender.nMiddle = 0;
else {
	for (h = nFaces / 2, i = j = 0; i < gameData.render.mine.nRenderSegs; i++) {
		if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
			continue;
		j += SEGMENTS [nSegment].nFaces;
		if (j > h) {
			tiRender.nMiddle = i;
			break;
			}
		}
	}
}

//------------------------------------------------------------------------------

ushort renderVertices [MAX_VERTICES_D2X];
ubyte	 bIsRenderVertex [MAX_VERTICES_D2X];
int	 nRenderVertices;

void GetRenderVertices (void)
{
	tSegment		*segP;
	grsFace		*faceP;
	int			h, i, j, n;

nRenderVertices = 0;
for (h = 0; h < gameData.render.mine.nRenderSegs; h++) {
	segP = SEGMENTS + gameData.render.mine.nSegRenderList [h];
	if (!SegmentIsVisible (segP)) 
		gameData.render.mine.nSegRenderList [h] = -1;
	else {
		for (i = segP->nFaces, faceP = segP->pFaces; i; i--, faceP++) {
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
	tSegment		*segP;
	grsFace		*faceP;
	short			nSegment;
	int			h, i, j;

nRenderVertices = 0;
for (h = 0; h < gameData.render.mine.nRenderSegs; h++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [h]))
		continue;
	segP = SEGMENTS + gameData.render.mine.nSegRenderList [h];
	for (i = segP->nFaces, faceP = segP->pFaces; i; i--, faceP++)
		for (j = 0; j < 4; j++)
			gameData.segs.faces.color [j] = gameData.render.color.vertices [faceP->index [j]].color;
	}
}

//------------------------------------------------------------------------------

void RenderMineObjects (int nType)
{
	int	nListPos;
	short	nSegment;

if ((nType < 1) || (nType > 2))
	return;
for (nListPos = gameData.render.mine.nRenderSegs; nListPos; ) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [--nListPos]))
		continue;
	if (0 > gameData.render.mine.renderObjs.ref [nSegment]) 
		continue;
#ifdef _DEBUG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (nType == 1) {	// render opaque objects
		SetNearestDynamicLights (nSegment, 0, 0);
		SetNearestStaticLights (nSegment, 1, 0);
		gameStates.render.bApplyDynLight = gameStates.render.bUseDynLight && gameOpts->ogl.bLightObjects;
		RenderObjList (nListPos, gameStates.render.nWindow);
		gameStates.render.bApplyDynLight = gameStates.render.bUseDynLight;
#if 1
		gameData.render.lights.dynamic.shader.nActiveLights [0] = gameData.render.lights.dynamic.shader.iStaticLights [0];
#else
		SetNearestStaticLights (nSegment, 0, 0);
#endif
		}
	else if (nType == 2)	// render objects containing transparency, like explosions
		RenderObjList (nListPos, gameStates.render.nWindow);
	}	
gameStates.render.nState = 0;
}

//------------------------------------------------------------------------------
// eof