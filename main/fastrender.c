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
#include "error.h"
#include "u_mem.h"
#include "lighting.h"
#include "lightmap.h"
#include "automap.h"
#include "texmerge.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_fastrender.h"
#include "endlevel.h"
#include "wall.h"
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
	tSide	*sideP = segP->sides + faceP->nSide;
	short	nFrame = sideP->nFrame;

if ((faceP->nBaseTex < 0) || !faceP->bTextured)
	return;
if (faceP->bOverlay)
	faceP->nBaseTex = sideP->nOvlTex;
else {
	faceP->nBaseTex = sideP->nBaseTex;
	if (!faceP->bSplit)
		faceP->nOvlTex = sideP->nOvlTex;
	}
if (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk) {
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
		faceP->bmTop = NULL;
		}
	else {
		faceP->bmBot = gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [faceP->nBaseTex].index;
		PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [faceP->nBaseTex], gameStates.app.bD1Mission);
		}
	}
}

//------------------------------------------------------------------------------

#if RENDER_DEPTHMASK_FIRST

void SplitFace (tSegFaces *segFaceP, grsFace *faceP)
{
if (GEO_LIGHTING)
	return;
if (gameStates.ogl.bGlTexMerge && (gameStates.render.history.bOverlay < 0) && 
	 !faceP->bSlide && (faceP->nCamera < 0) && faceP->nOvlTex && faceP->bmTop && !faceP->bAnimation) {	//last rendered face was multi-textured but not super-transparent
		grsFace *newFaceP = segFaceP->pFaces + segFaceP->nFaces++;

	*newFaceP = *faceP;
	newFaceP->nIndex = (newFaceP - 1)->nIndex + 4;
	memcpy (newFaceP->index, faceP->index, sizeof (faceP->index));
	memcpy (gameData.segs.faces.vertices + newFaceP->nIndex, gameData.segs.faces.vertices + faceP->nIndex, 4 * sizeof (fVector3));
	memcpy (gameData.segs.faces.texCoord + newFaceP->nIndex, gameData.segs.faces.ovlTexCoord + faceP->nIndex, 4 * sizeof (tTexCoord2f));
	memcpy (gameData.segs.faces.color + newFaceP->nIndex, gameData.segs.faces.color + faceP->nIndex, 4 * sizeof (tRgbaColorf));
	newFaceP->nBaseTex = faceP->nOvlTex;
	faceP->nOvlTex = newFaceP->nOvlTex = 0;
	faceP->bmTop = 
	newFaceP->bmTop = NULL;
	faceP->bSplit = 1;
	newFaceP->bOverlay = 1;
	if (newFaceP->bIsLight = IsLight (newFaceP->nBaseTex))
		faceP->bIsLight = 0;
	}
}

#endif

//------------------------------------------------------------------------------

void FixTriangleFan (tSegment *segP, grsFace *faceP)
{
if (((faceP->nType = segP->sides [faceP->nSide].nType) == SIDE_IS_TRI_13)) {	//rearrange vertex order for TRIANGLE_FAN rendering
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
}

//------------------------------------------------------------------------------

void RenderMineFace (tSegment *segP, tSegFaces *segFaceP, grsFace *faceP, short nSegment, int nType, int bVertexArrays, int bDepthOnly)
{
	short				special;

#ifdef _DEBUG
	static grsFace *prevFaceP = NULL;

if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nSegment = nSegment;
#endif
if ((nType < 4) && !(faceP->widFlags & WID_RENDER_FLAG))
	return;
if (faceP->nType < 0)
	FixTriangleFan (segP, faceP);
#ifdef _DEBUG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nSegment = nSegment;
#endif
if (nType == 0) {
	if (IS_WALL (faceP->nWall))
		return;
	}
else if (nType == 1) {
	if (!IS_WALL (faceP->nWall))
		return;
	}
else if (nType == 2) {
	if (faceP->bmBot)
		return;
	special = gameData.segs.segment2s [nSegment].special;
	if ((special < SEGMENT_IS_WATER) || (special > SEGMENT_IS_TEAM_RED) || 
		 (gameData.segs.xSegments [nSegment].group < 0) || (gameData.segs.xSegments [nSegment].owner < 1))
			return;
	}
else if (nType == 3) {
#ifdef _DEBUG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		nSegment = nSegment;
#endif
#if 1
	if (faceP->nCorona)
		RenderCorona (nSegment, faceP->nSide, CoronaVisibility (faceP->nCorona));
#endif
	return;
	}
#ifdef _DEBUG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nSegment = nSegment;
	else
		nSegment = nSegment;
#endif
if ((faceP->nCamera >= 0) && !SetupMonitorFace (nSegment, faceP->nSide, faceP->nCamera, faceP)) 
	faceP->nCamera = -1;
if (faceP->nCamera < 0) {
#if 0
	if (!(bTextured && faceP->bTextured))
		faceP->bmBot = faceP->bmTop = NULL;
	else
#endif
		LoadFaceBitmaps (segP, faceP);
	}
#ifdef _DEBUG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nSegment = nSegment;
	else
		nSegment = nSegment;
#endif
G3DrawFace (faceP, faceP->bmBot, faceP->bmTop, (faceP->nCamera < 0) || faceP->bTeleport, 
				!bDepthOnly && faceP->bTextured, bDepthOnly, bVertexArrays);
#ifdef _DEBUG
prevFaceP = faceP;
#endif
#if RENDER_DEPTHMASK_FIRST
if (faceP->bTextured)
	SplitFace (segFaceP, faceP);
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
	tSegFaces	*segFaceP;
	grsFace		*faceP;
	tFaceRef		*ph, *pi, *pj;
	int			h, i, j;
	short			nSegment;

for (h = i = 0, ph = faceRef [0]; i < gameData.render.mine.nRenderSegs; i++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	segFaceP = SEGFACES + nSegment;
	for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++, h++, ph++) {
		ph->nSegment = nSegment;
		ph->faceP = faceP;
		}
	}
tiRender.nFaces = h;
if (h > 1) {
	if (RunRenderThreads (rtSortFaces)) {
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

int BeginRenderFaces (int nType)
{
	int	bVertexArrays;

gameData.threads.vertColor.data.bDarkness = 0;
gameStates.render.nType = nType;
gameStates.render.history.nShader = -1;
gameStates.render.history.bOverlay = -1;
gameStates.render.history.bmBot = 
gameStates.render.history.bmTop =
gameStates.render.history.bmMask = NULL;
glEnable (GL_CULL_FACE);
OglTexWrap (NULL, GL_REPEAT);
glDepthFunc (GL_LEQUAL);
if (nType == 3)
	return 0;
OglSetupTransform (1);
if (GEO_LIGHTING) {
	G3DisableClientStates (1, 1, 1, GL_TEXTURE2);
	G3DisableClientStates (1, 1, 1, GL_TEXTURE1);
	G3DisableClientStates (1, 1, 1, GL_TEXTURE0);
	return 0;
	}
if (bVertexArrays = G3EnableClientStates (1, 1, 0, GL_TEXTURE0)) {
	glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.texCoord);
	glVertexPointer (3, GL_FLOAT, 0, gameData.segs.faces.vertices);
	glColorPointer (4, GL_FLOAT, 0, gameData.segs.faces.color);

	if (bVertexArrays = G3EnableClientStates (1, 1, 0, GL_TEXTURE1)) {
		glVertexPointer (3, GL_FLOAT, 0, gameData.segs.faces.vertices);
		glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.ovlTexCoord);
		glColorPointer (4, GL_FLOAT, 0, gameData.segs.faces.color);
		}
	else
		G3DisableClientStates (1, 1, 0, GL_TEXTURE0);
	if (G3EnableClientStates (1, 1, 0, GL_TEXTURE2)) {
		glVertexPointer (3, GL_FLOAT, 0, gameData.segs.faces.vertices);
		glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.ovlTexCoord);
		glColorPointer (4, GL_FLOAT, 0, gameData.segs.faces.color);
		}
	}
else {
	G3DisableClientStates (1, 1, 0, GL_TEXTURE1);
	G3DisableClientStates (1, 1, 0, GL_TEXTURE0);
	}
return bVertexArrays;
}

//------------------------------------------------------------------------------

void EndRenderFaces (int nType, int bVertexArrays)
{
#if 0
G3FlushFaceBuffer (1);
#endif
if (bVertexArrays) {
	G3DisableClientStates (1, 1, 0, GL_TEXTURE2);
	glActiveTexture (GL_TEXTURE2);
	glEnable (GL_TEXTURE_2D);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);

	G3DisableClientStates (1, 1, 0, GL_TEXTURE1);
	glActiveTexture (GL_TEXTURE1);
	glEnable (GL_TEXTURE_2D);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);

	G3DisableClientStates (1, 1, 0, GL_TEXTURE0);
	glActiveTexture (GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	}
//if (gameStates.render.history.bOverlay > 0)
	glUseProgramObject (0);
if (nType != 3)
	OglResetTransform (1);
else if (gameOpts->render.nCoronaStyle && gameStates.ogl.bOcclusionQuery && gameData.render.lights.nCoronas && !gameStates.render.bQueryCoronas)
	glDeleteQueries (gameData.render.lights.nCoronas, gameData.render.lights.coronaQueries);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//------------------------------------------------------------------------------

void RenderSkyBoxFaces (void)
{
	tSegFaces	*segFaceP;
	grsFace		*faceP;
	int			i, j, bVertexArrays, bFullBright = gameStates.render.bFullBright;

if (gameStates.render.bHaveSkyBox) {
	glDepthMask (1);
	gameStates.render.bHaveSkyBox = 0;
	gameStates.render.nType = 4;
	gameStates.render.bFullBright = 1;
	bVertexArrays = BeginRenderFaces (4);
	for (i = 0, segFaceP = SEGFACES; i < gameData.segs.nSegments; i++, segFaceP++) {
		if (gameData.segs.segment2s [i].special != SEGMENT_IS_SKYBOX)
			continue;
		gameStates.render.bHaveSkyBox = 1;
		for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++) {
			if (!(faceP->bVisible = FaceIsVisible (i, faceP->nSide)))
				continue;
			RenderMineFace (SEGMENTS + i, segFaceP, faceP, i, 4, bVertexArrays, 0);
			}
		}
	gameStates.render.bFullBright = bFullBright;
	EndRenderFaces (4, bVertexArrays);
	}
}

//------------------------------------------------------------------------------

void QueryCoronas (int nFaces, int nPass)
{
	grsFace		*faceP;
	tFaceRef		*pfr;
	tSegFaces	*segFaceP;
	short			nSegment;
	int			i, j, bVertexArrays = BeginRenderFaces (3);

gameStates.render.bQueryCoronas = nPass;
glDepthMask (0);
if (nPass == 1) {	//find out how many total fragments each corona has
	for (i = 0; i < gameData.render.mine.nRenderSegs; i++) {
		if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
			continue;
		if (gameStates.render.automap.bDisplay) {
			if (!(gameStates.render.automap.bFull || gameData.render.mine.bAutomapVisited [nSegment]))
				return;
			if (!gameOpts->render.automap.bSkybox && (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SKYBOX))
				continue;
			}
		segFaceP = SEGFACES + nSegment;
		for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++)
			if (faceP->nCorona) {
#ifdef _DEBUG
				if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
					nSegment = nSegment;
#endif
				glBeginQuery (GL_SAMPLES_PASSED_ARB, gameData.render.lights.coronaQueries [faceP->nCorona - 1]);
				RenderCorona (nSegment, faceP->nSide, 1);
				glEndQuery (GL_SAMPLES_PASSED_ARB);
#if 0
				CoronaVisibility (faceP->nCorona);
#endif
				}
		}
	glFlush ();
#if 1
	for (i = 0; i < gameData.render.mine.nRenderSegs; i++) {
		if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
			continue;
		if (gameStates.render.automap.bDisplay) {
			if (!(gameStates.render.automap.bFull || gameData.render.mine.bAutomapVisited [nSegment]))
				return;
			if (!gameOpts->render.automap.bSkybox && (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SKYBOX))
				continue;
			}
		segFaceP = SEGFACES + nSegment;
		for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++)
			if (faceP->nCorona) {
#ifdef _DEBUG
				if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
					nSegment = nSegment;
#endif
				CoronaVisibility (faceP->nCorona);
				}
		}
#endif
	glColorMask (1,1,1,1);
	glClearColor (0,0,0,0);
	glClear (GL_COLOR_BUFFER_BIT);
	}
else {
	for (i = 0, pfr = faceRef [gameStates.app.bMultiThreaded]; i < nFaces; i++) {
		faceP = pfr [i].faceP;
		if (!faceP->nCorona)
			continue;
		if (gameStates.render.automap.bDisplay) {
			if (!gameOpts->render.automap.bSkybox && (gameData.segs.segment2s [pfr [i].nSegment].special == SEGMENT_IS_SKYBOX))
				continue;
			}
#ifdef _DEBUG
		if ((pfr->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
			faceP = faceP;
#endif
		glBeginQuery (GL_SAMPLES_PASSED_ARB, gameData.render.lights.coronaQueries [faceP->nCorona - 1]);
		if (!glGetError ())
			RenderCorona (pfr [i].nSegment, faceP->nSide, 1);
		glEndQuery (GL_SAMPLES_PASSED_ARB);
		}
	glFlush ();
	}
EndRenderFaces (3, bVertexArrays);
gameStates.render.bQueryCoronas = 0;
}

//------------------------------------------------------------------------------

int SetupCoronaFaces (void)
{
	tSegFaces	*segFaceP;
	grsFace		*faceP;
	int			i, j, nSegment;

if (!gameOpts->render.bCoronas)
	return 0;
gameData.render.lights.nCoronas = 0;
for (i = 0; i < gameData.render.mine.nRenderSegs; i++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	if ((gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SKYBOX) ||
		 (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_OUTDOOR))
		continue;
	segFaceP = SEGFACES + nSegment;
#ifdef _DEBUG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++)
		if (faceP->bVisible && (faceP->widFlags & WID_RENDER_FLAG) && faceP->bIsLight && (faceP->nCamera < 0) &&
			 FaceHasCorona (nSegment, faceP->nSide, NULL, NULL))
			faceP->nCorona = ++gameData.render.lights.nCoronas;
		else
			faceP->nCorona = 0;
	}
return gameData.render.lights.nCoronas;
}

//------------------------------------------------------------------------------

static inline int VisitSegment (short nSegment, int bAutomap)
{
if (nSegment < 0)
	return 0;
if (bAutomap) {
	if (gameStates.render.automap.bDisplay) {
		if (!(gameStates.render.automap.bFull || gameData.render.mine.bAutomapVisited [nSegment]))
			return 0;
		if (!gameOpts->render.automap.bSkybox && (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SKYBOX))
			return 0;
		}
	else
		gameData.render.mine.bAutomapVisited [nSegment] = gameData.render.mine.bSetAutomapVisited;
	}
if (VISITED (nSegment))
	return 0;
VISIT (nSegment);
return 1;
}

//------------------------------------------------------------------------------

static void RenderSegmentFaces (int nType, short nSegment, int bVertexArrays, int bDepthOnly, int bLighting, int bAutomap)
{
	tSegFaces	*segFaceP = SEGFACES + nSegment;
	grsFace		*faceP;
	int			i;

if (!VisitSegment (nSegment, bAutomap))
	return;
#ifdef _DEBUG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
if (bLighting)
	SetNearestDynamicLights (nSegment, 0, 0, 0);
for (i = segFaceP->nFaces, faceP = segFaceP->pFaces; i; i--, faceP++) {
#ifdef _DEBUG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		nSegment = nSegment;
#endif
	RenderMineFace (SEGMENTS + nSegment, segFaceP, faceP, nSegment, nType, bVertexArrays, bDepthOnly);
	}
}

//------------------------------------------------------------------------------

void RenderSegments (int nType, int bVertexArrays, int bDepthOnly)
{
	int				i, bAutomap = (nType == 0), bLighting = GEO_LIGHTING && (nType < 3) && !bDepthOnly;

if (nType) {
	for (i = gameData.render.mine.nRenderSegs; i; )
		RenderSegmentFaces (nType, gameData.render.mine.nSegRenderList [--i], bVertexArrays, bDepthOnly, bLighting, bAutomap);
	}
else {
	for (i = 0; i < gameData.render.mine.nRenderSegs; i++)
		RenderSegmentFaces (nType, gameData.render.mine.nSegRenderList [i], bVertexArrays, bDepthOnly, bLighting, bAutomap);
	}
}

//------------------------------------------------------------------------------

void RenderFaceList (int nType)
{
	tFaceRef		*pfr;
	short			nSegment;
	int			i, j, bVertexArrays;

bVertexArrays = BeginRenderFaces (nType);
if (nType) {	//back to front
	RenderSegments (nType, bVertexArrays, 0);
	}
else {	//front to back
#if RENDER_DEPTHMASK_FIRST
	if (SetupCoronaFaces ()) {
		if (gameOpts->render.nCoronaStyle && gameStates.ogl.bOcclusionQuery) {
			EndRenderFaces (nType, bVertexArrays);
			glGenQueries (gameData.render.lights.nCoronas, gameData.render.lights.coronaQueries);
			QueryCoronas (0, 1);
			bVertexArrays = BeginRenderFaces (nType);
			}
		}
	glColorMask (0,0,0,0);
	glDepthMask (1);
	glDepthFunc (GL_LESS);
	glClientActiveTexture (GL_TEXTURE0);
	glDisableClientState (GL_COLOR_ARRAY);
	glClientActiveTexture (GL_TEXTURE1);
	glDisableClientState (GL_COLOR_ARRAY);
#else
	glDepthFunc (GL_LEQUAL);
#endif
	RenderSegments (nType, bVertexArrays, RENDER_DEPTHMASK_FIRST);
#if RENDER_DEPTHMASK_FIRST
	G3EnableClientState (GL_COLOR_ARRAY, GL_TEXTURE1);
	G3EnableClientState (GL_COLOR_ARRAY, GL_TEXTURE0);
	j = SortFaces ();
	if (gameOpts->render.nCoronaStyle && gameStates.ogl.bOcclusionQuery && gameData.render.lights.nCoronas) {
		EndRenderFaces (nType, bVertexArrays);
		gameStates.render.bQueryCoronas = 2;
		gameStates.render.nType = 1;
		RenderMineObjects (1);
		gameStates.render.nType = 0;
		QueryCoronas (j, 2);
		bVertexArrays = BeginRenderFaces (nType);
		}
	glDepthMask (1);
	glDepthFunc (GL_LEQUAL);
	glColorMask (1,1,1,1);
	if (GEO_LIGHTING) {
		gameData.render.mine.nVisited++;
		RenderSegments (nType, bVertexArrays, 0);
		}
	else {
		for (i = 0, pfr = faceRef [gameStates.app.bMultiThreaded]; i < j; i++) {
			nSegment = pfr [i].nSegment;
#ifdef _DEBUG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (pfr[i].faceP->nSide == nDbgSide)))
				pfr = pfr;
#endif
			if (gameStates.render.automap.bDisplay) {
				if (!gameOpts->render.automap.bSkybox && (gameData.segs.segment2s [pfr [i].nSegment].special == SEGMENT_IS_SKYBOX))
					continue;
				}
			RenderMineFace (SEGMENTS + nSegment, SEGFACES + nSegment, pfr [i].faceP, nSegment, nType, bVertexArrays, 0);
			}
		}
	glDepthMask (1);
#endif
	}
EndRenderFaces (nType, bVertexArrays);
}

// -----------------------------------------------------------------------------------

inline int SegmentIsVisible (tSegment *segP)
{
if (gameStates.render.automap.bDisplay)
	return 1;
return RotateVertexList (8, segP->verts).and == 0;
}

//------------------------------------------------------------------------------

int SetupFace (short nSegment, short nSide, tSegment *segP, grsFace *faceP, tFaceColor *pFaceColor, float *pfAlpha)
{
	ubyte	bTextured;
	int	nColor = 0;

faceP->widFlags = WALL_IS_DOORWAY (segP, nSide, NULL);
if (!(faceP->widFlags & (WID_RENDER_FLAG | WID_RENDPAST_FLAG)))
	return -1;
faceP->nCamera = IsMonitorFace (nSegment, nSide);
bTextured = 1;
*pfAlpha = WallAlpha (nSegment, nSide, faceP->nWall, faceP->widFlags, faceP->nCamera >= 0, 
							 &pFaceColor [1].color, &nColor, &bTextured);
faceP->bTextured = bTextured;
if ((faceP->nSegColor = IsColoredSegFace (nSegment, nSide))) {
	pFaceColor [2].color = *ColoredSegmentColor (nSegment, nSide, faceP->nSegColor);
	if (faceP->nBaseTex < 0)
		*pfAlpha = pFaceColor [2].color.alpha;
	nColor = 2;
	}
if ((*pfAlpha < 1) || ((nColor == 2) && (faceP->nBaseTex < 0)))
	faceP->bTransparent = 1;
return nColor;
}

//------------------------------------------------------------------------------

void UpdateSlidingFaces (void)
{
	tSegment		*segP;
	tSegFaces	*segFaceP;
	grsFace		*faceP;
	short			h, i, j, nOffset;
	tTexCoord2f	*texCoordP, *ovlTexCoordP;
	tUVL			*uvlP;

for (i = gameData.segs.nSegments, segP = SEGMENTS, segFaceP = SEGFACES; i; i--, segP++, segFaceP++) {
	for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++) {
		if (faceP->bSlide) {
#ifdef _DEBUG
			if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
				faceP = faceP;
#endif
			texCoordP = gameData.segs.faces.texCoord + faceP->nIndex;
			ovlTexCoordP = gameData.segs.faces.ovlTexCoord + faceP->nIndex;
			uvlP = segP->sides [faceP->nSide].uvls;
			nOffset = faceP->nType == SIDE_IS_TRI_13;
			for (h = 0; h < 4; h++) {
				texCoordP [h].v.u = f2fl (uvlP [(h + nOffset) % 4].u);
				texCoordP [h].v.v = f2fl (uvlP [(h + nOffset) % 4].v);
				RotateTexCoord2f (ovlTexCoordP + h, texCoordP + h, faceP->nOvlOrient);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void ComputeFaceLight (int nStart, int nEnd, int nThread)
{
	tSegment		*segP;
	tSegFaces	*segFaceP;
	grsFace		*faceP;
	tRgbaColorf	*pc;
	tFaceColor	c, faceColor [3] = {{{0,0,0,1},1},{{0,0,0,0},1},{{0,0,0,1},1}};
	ubyte			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
	short			nVertex, nSegment, nSide;
	fix			xLight;
	float			fAlpha;
	tUVL			*uvlP;
	int			h, i, j, uvi, nColor, 
					nIncr = nStart ? -1 : 1,
					bDynLight = gameStates.render.bApplyDynLight && (gameStates.app.bEndLevelSequence < EL_OUTSIDE);

gameData.render.lights.dynamic.shader.nActiveLights [0] =
gameData.render.lights.dynamic.shader.nActiveLights [1] =
gameData.render.lights.dynamic.shader.nActiveLights [2] =
gameData.render.lights.dynamic.shader.nActiveLights [3] = 0;
gameStates.ogl.bUseTransform = 1;
gameStates.render.nState = 0;
if (gameStates.render.bFullBright)
	c.color.red = c.color.green = c.color.blue = c.color.alpha = 1;
for (i = nStart; i != nEnd; i += nIncr) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	segP = SEGMENTS + nSegment;
	segFaceP = SEGFACES + nSegment;
	if (!(gameStates.app.bMultiThreaded || SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [i] = -gameData.render.mine.nSegRenderList [i];
		continue;
		}
	if (bDynLight) {
#ifdef _DEBUG
		if (nSegment == nDbgSeg)
			nSegment = nSegment;
#endif
		if (!(gameStates.render.bFullBright || GEO_LIGHTING))
			SetNearestDynamicLights (nSegment, 0, 0, nThread);
		for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++) {
			nSide = faceP->nSide;
#ifdef _DEBUG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
				nSegment = nSegment;
				if (gameData.render.lights.dynamic.shader.nActiveLights [nThread])
					nSegment = nSegment;
				}
#endif
			if (!(faceP->bVisible = FaceIsVisible (nSegment, nSide)))
				continue;
			if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
				faceP->bVisible = 0;
				continue;
				}
			faceP->color = faceColor [nColor].color;
//			SetDynLightMaterial (nSegment, faceP->nSide, -1);
			pc = gameData.segs.faces.color + faceP->nIndex;
			for (h = 0; h < 4; h++, pc++) {
				if (gameStates.render.bFullBright) 
					*pc = c.color;
				else {
					c = faceColor [nColor];
					if (nColor)
						*pc = c.color;
					else {
						nVertex = faceP->index [h];
#ifdef _DEBUG
						if (nVertex == nDbgVertex)
							nDbgVertex = nDbgVertex;
#endif
						if (gameData.render.color.vertices [nVertex].index != gameStates.render.nFrameFlipFlop + 1) {
#if 0
							if (gameStates.app.bMultiThreaded) {
								if (gameData.render.mine.bCalcVertexColor [nVertex] & nThreadFlags [1])
									goto skipVertex;
								}
#endif
							if (GEO_LIGHTING)
								c.color = gameData.render.color.ambient [nVertex].color;
							else
								G3VertexColor (&gameData.segs.points [nVertex].p3_normal.vNormal, gameData.segs.fVertices + nVertex, nVertex, 
													NULL, &c, 1, 0, nThread);
#if 0
							if (gameStates.app.bMultiThreaded)
								gameData.render.mine.bCalcVertexColor [nVertex] &= nThreadFlags [2];
#endif
#ifdef _DEBUG
							if (nVertex == nDbgVertex)
								nVertex = nVertex;
#endif
							}
						*pc = gameData.render.color.vertices [nVertex].color;
						}
					if (nColor == 1) {
						pc->red *= fAlpha;
						pc->blue *= fAlpha;
						pc->green *= fAlpha;
						}
					pc->alpha = fAlpha;
					}
				}
			gameData.render.lights.dynamic.material.bValid = 0;
			}
		}
	else {
		for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++) {
			nSide = faceP->nSide;
			if (!(faceP->bVisible = FaceIsVisible (nSegment, nSide)))
				continue;
#ifdef _DEBUG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nSegment = nSegment;
#endif
			if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
				faceP->bVisible = 0;
				continue;
				}
			faceP->color = faceColor [nColor].color;
			pc = gameData.segs.faces.color + faceP->nIndex;
			uvlP = segP->sides [nSide].uvls;
			for (h = 0, uvi = (segP->sides [nSide].nType == SIDE_IS_TRI_13); h < 4; h++, pc++, uvi++) {
				if (!gameStates.render.bFullBright) {
					c = faceColor [nColor];
					nVertex = faceP->index [h];
					SetVertexColor (nVertex, &c);
					xLight = SetVertexLight (nSegment, nSide, nVertex, &c, uvlP [uvi % 4].l);
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

int CountRenderFaces (void)
{
	tSegment		*segP;
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
if (!nFaces)
	tiRender.nMiddle = 0;
else {
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
	grsFace		*faceP;
	short			nSegment;
	int			h, i, j, n;

nRenderVertices = 0;
for (h = 0; h < gameData.render.mine.nRenderSegs; h++) {
	nSegment = gameData.render.mine.nSegRenderList [h];
	segFaceP = SEGFACES + nSegment;
	if (!SegmentIsVisible (SEGMENTS + nSegment)) 
		gameData.render.mine.nSegRenderList [h] = -gameData.render.mine.nSegRenderList [i];
	else {
		for (i = segFaceP->nFaces, faceP = segFaceP->pFaces; i; i--, faceP++) {
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
	grsFace		*faceP;
	short			nSegment;
	int			h, i, j;

nRenderVertices = 0;
for (h = 0; h < gameData.render.mine.nRenderSegs; h++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [h]))
		continue;
	segFaceP = SEGFACES + gameData.render.mine.nSegRenderList [h];
	for (i = segFaceP->nFaces, faceP = segFaceP->pFaces; i; i--, faceP++)
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
	nSegment = abs (gameData.render.mine.nSegRenderList [--nListPos]);
#ifdef _DEBUG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (0 > gameData.render.mine.renderObjs.ref [nSegment]) 
		continue;
#ifdef _DEBUG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (nType == 1) {	// render opaque objects
#ifdef _DEBUG
		if (nSegment == nDbgSeg)
			nSegment = nSegment;
#endif
		if (gameStates.render.bUseDynLight && !gameStates.render.bQueryCoronas) {
			SetNearestDynamicLights (nSegment, 0, 1, 0);
			SetNearestStaticLights (nSegment, 1, 1, 0);
			gameStates.render.bApplyDynLight = gameOpts->ogl.bLightObjects;
			if (gameData.render.lights.dynamic.shader.nActiveLights [0])
				nType = nType;
			}
		else
			gameStates.render.bApplyDynLight = 0;
		RenderObjList (nListPos, gameStates.render.nWindow);
		gameStates.render.bApplyDynLight = gameStates.render.bUseDynLight;
		gameData.render.lights.dynamic.shader.nActiveLights [0] = gameData.render.lights.dynamic.shader.iStaticLights [0];
		}
	else if (nType == 2)	// render objects containing transparency, like explosions
		RenderObjList (nListPos, gameStates.render.nWindow);
	}	
gameStates.render.nState = 0;
}

//------------------------------------------------------------------------------
// eof