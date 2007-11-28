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
#include "light.h"
#include "lightmap.h"
#include "automap.h"
#include "texmerge.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_fastrender.h"
#include "ogl_shader.h"
#include "fbuffer.h"
#include "endlevel.h"
#include "wall.h"
#include "glare.h"
#include "render.h"
#include "renderlib.h"
#include "renderthreads.h"
#include "transprender.h"
#include "fastrender.h"

#define RENDER_DEPTHMASK_FIRST 1

#ifdef _DEBUG
#	define SHADER_VERTEX_LIGHTING 0
#else
#	define SHADER_VERTEX_LIGHTING 0
#endif

#define VERTLIGHT_DRAWARRAYS 1
#define VERTLIGHT_BUFFERS	4
#define VL_SHADER_BUFFERS VERTLIGHT_BUFFERS
#define VLBUF_WIDTH 32
#define VLBUF_SIZE (VLBUF_WIDTH * VLBUF_WIDTH)

GLhandleARB hVertLightShader = 0;
GLhandleARB hVertLightVS = 0; 
GLhandleARB hVertLightFS = 0; 

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
	grsFace *newFaceP;

if (GEO_LIGHTING)
	return;
if (!gameStates.ogl.bGlTexMerge)
	return;
if (faceP->bSolid)
	return;
if (faceP->bSlide || !faceP->nOvlTex || !faceP->bmTop || faceP->bAnimation ||
	 (gameStates.render.history.bOverlay >= 0) || (faceP->nCamera >= 0))
	faceP->bSolid = 1;
else if (faceP->bmTop && strstr (faceP->bmTop->szName, "door"))
	faceP->bAnimation = faceP->bSolid = 1;
if (faceP->bSolid)
	return;
#ifdef _DEBUG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
newFaceP = segFaceP->pFaces + segFaceP->nFaces++;
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
gameStates.render.bQueryCoronas = 0;
glEnable (GL_CULL_FACE);
OglTexWrap (NULL, GL_REPEAT);
glDepthFunc (GL_LEQUAL);
if (nType == 3) {
	if (CoronaStyle () == 2)
		LoadGlareShader ();
	return 0;
	}
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
if (gameStates.ogl.bShadersOk)
	glUseProgramObject (0);
if (nType != 3)
	OglResetTransform (1);
else 	if (CoronaStyle () == 2)
	UnloadGlareShader ();
else if (gameStates.ogl.bOcclusionQuery && gameData.render.lights.nCoronas && !gameStates.render.bQueryCoronas && (CoronaStyle () == 1))
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
		if ((CoronaStyle () == 1) && gameStates.ogl.bOcclusionQuery) {
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
	if (gameOpts->render.bCoronas && gameStates.ogl.bOcclusionQuery && gameData.render.lights.nCoronas && (CoronaStyle () == 1)) {
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

typedef struct tVertLightIndex {
	short			nVertex;
	short			nLights;
	tRgbaColorf	color;
} tVertLightIndex;

typedef struct tVertLightData {
	short					nVertices;
	short					nLights;
	fVector				buffers [VERTLIGHT_BUFFERS][VLBUF_SIZE];
	fVector				colors [VLBUF_SIZE];
	tVertLightIndex	index [VLBUF_SIZE];
} tVertLightData;

static tVertLightData vld = {0, 0};

//------------------------------------------------------------------------------

void InitDynLighting (void)
{
if (gameStates.ogl.bVertexLighting)
	gameStates.ogl.bVertexLighting = OglCreateFBuffer (&gameData.render.lights.dynamic.fb, VLBUF_WIDTH, VLBUF_WIDTH, 2);
}

//------------------------------------------------------------------------------

void CloseDynLighting (void)
{
if (gameStates.ogl.bVertexLighting) {
	LogErr ("unloading dynamic lighting buffers\n");
	OglDestroyFBuffer (&gameData.render.lights.dynamic.fb);
	}
}

//------------------------------------------------------------------------------

static char	*szTexNames [VERTLIGHT_BUFFERS] = {"vertPosTex", "vertNormTex", "lightPosTex", "lightColorTex"};

GLuint CreateVertLightBuffer (int i)
{
	GLuint	hBuffer;

//create render texture
OglGenTextures (1, &hBuffer);
if (!hBuffer)
	return 0;
glActiveTexture (GL_TEXTURE0 + i);
glClientActiveTexture (GL_TEXTURE0 + i);
glEnable (GL_TEXTURE_2D);
glBindTexture (GL_TEXTURE_2D, hBuffer);
// set up texture parameters, turn off filtering
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
//glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 
// define texture with same size as viewport
glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, VLBUF_WIDTH, VLBUF_WIDTH, 0, GL_RGBA, GL_FLOAT, vld.buffers [i]);
return hBuffer;
}

//------------------------------------------------------------------------------

void ComputeFragLight (float lightRange)
{
	fVector	*vertPos, *vertNorm, *lightPos, lightColor, lightDir;
	fVector	vReflect, vertColor = {{0, 0, 0, 0}};
	float		nType, radius, brightness, specular;
	float		attenuation, lightDist, NdotL, RdotE;
	int		i;

	static fVector matAmbient = {{0.01f, 0.01f, 0.01f, 1.0f}};
	static fVector matDiffuse = {{1.0f, 1.0f, 1.0f, 1.0f}};
	static fVector matSpecular = {{1.0f, 1.0f, 1.0f, 1.0f}};
	static float shininess = 96.0;

for (i = 0; i < vld.nLights; i++) {
	vertPos = vld.buffers [0] + i;
	vertNorm = vld.buffers [1] + i;
	lightPos = vld.buffers [2] + i;
	lightColor = vld.buffers [3][i];
	nType = vertNorm->p.w;
	radius = lightPos->p.w;
	brightness = lightColor.p.w;
	VmVecSubf (&lightDir, lightPos, vertPos);
	lightDist = VmVecMagf (&lightDir) / lightRange;
	VmVecNormalizef (&lightDir, &lightDir);
	if (nType)
		lightDist -= radius;
	if (lightDist < 1.0f) {
		lightDist = 1.4142f;
		NdotL = 1.0f;
		}
	else {
		lightDist *= lightDist;
		if (nType)
			lightDist *= 2.0f;
		NdotL = VmVecDotf (vertNorm, &lightDir);
		if (NdotL < 0.0f)
			NdotL = 0.0f;
		}	
	attenuation = lightDist / brightness;
	vertColor.c.r = (matAmbient.c.r + NdotL) * lightColor.c.r;
	vertColor.c.g = (matAmbient.c.g + NdotL) * lightColor.c.g;
	vertColor.c.b = (matAmbient.c.b + NdotL) * lightColor.c.b;
	if (NdotL > 0.0f) {
		VmVecReflectf (&vReflect, VmVecNegatef (&lightDir), vertNorm);
		VmVecNormalizef (&vReflect, &vReflect);
		VmVecNormalizef (lightPos, VmVecNegatef (lightPos));
		RdotE = VmVecDotf (&vReflect, lightPos);
		if (RdotE < 0.0f)
			RdotE = 0.0f;
		specular = (float) pow (RdotE, shininess);
		vertColor.c.r += lightColor.c.r * specular;
		vertColor.c.g += lightColor.c.g * specular;
		vertColor.c.b += lightColor.c.b * specular;
		}	
	vld.colors [i].c.r = vertColor.c.r / attenuation;
	vld.colors [i].c.g = vertColor.c.g / attenuation;
	vld.colors [i].c.b = vertColor.c.b / attenuation;
	}
}

//------------------------------------------------------------------------------

int RenderVertLightBuffers (void)
{
	tFaceColor	*vertColorP;
	tRgbaColorf	vertColor;
	fVector		*pc;
	int			i, j;
	short			nVertex, nLights;
	GLuint		hBuffer [VERTLIGHT_BUFFERS] = {0,0,0,0};

#if !VERTLIGHT_DRAWARRAYS
	static float	quadCoord [4][2] = {{0, 0}, {0, VLBUF_WIDTH}, {VLBUF_WIDTH, VLBUF_WIDTH}, {VLBUF_WIDTH, 0}};
	static float	texCoord [4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
#endif

#if 0
ComputeFragLight (gameStates.ogl.fLightRange);
#else
if (!vld.nLights)
	return 1;
for (i = 0; i < VL_SHADER_BUFFERS; i++) {
	if (!(hBuffer [i] = CreateVertLightBuffer (i)))
		return 0;
#if 0 //def _DEBUG
	memset (vld.buffers [i] + vld.nLights, 0, (VLBUF_SIZE - vld.nLights) * sizeof (vld.buffers [i][0]));
#endif
	}
#if VERTLIGHT_DRAWARRAYS
glDrawArrays (GL_QUADS, 0, 4);
#else
glBegin (GL_QUADS);
for (i = 0; i < 4; i++) {
	for (j = 0; j < VL_SHADER_BUFFERS; j++)
		glMultiTexCoord2fv (GL_TEXTURE0 + j, texCoord [i]);
	glVertex2fv (quadCoord [i]);
	}
glEnd ();
#endif
#ifdef _DEBUG
memset (vld.colors, 0, sizeof (vld.colors));
#endif
#if 0
for (i = 0; i < VL_SHADER_BUFFERS; i++) {
	glActiveTexture (GL_TEXTURE0 + i);
	glClientActiveTexture (GL_TEXTURE0 + i);
	glBindTexture (GL_TEXTURE_2D, 0);
	}
#endif
OglDeleteTextures (VL_SHADER_BUFFERS, hBuffer);
memset (hBuffer, 0, sizeof (hBuffer));
OglReadBuffer (GL_COLOR_ATTACHMENT0_EXT, 1);
glReadPixels (0, 0, VLBUF_WIDTH, VLBUF_WIDTH, GL_RGBA, GL_FLOAT, vld.colors);
#endif

for (i = 0, pc = vld.colors; i < vld.nVertices; i++) {
	nVertex = vld.index [i].nVertex;
#ifdef _DEBUG
	if (nVertex == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	vertColor = vld.index [i].color;
	vertColor.red += gameData.render.color.ambient [nVertex].color.red;
	vertColor.green += gameData.render.color.ambient [nVertex].color.green;
	vertColor.blue += gameData.render.color.ambient [nVertex].color.blue;
	if (gameOpts->render.color.nSaturation == 2) {
		for (j = 0, nLights = vld.index [i].nLights; j < nLights; j++, pc++) {
			if (vertColor.red < pc->c.r)
				vertColor.red = pc->c.r;
			if (vertColor.green < pc->c.g)
				vertColor.green = pc->c.g;
			if (vertColor.blue < pc->c.b)
				vertColor.blue = pc->c.b;
			}
		}
	else {
		for (j = 0, nLights = vld.index [i].nLights; j < nLights; j++, pc++) {
			vertColor.red += pc->c.r;
			vertColor.green += pc->c.g;
			vertColor.blue += pc->c.b;
			}
		if (gameOpts->render.color.nSaturation) {	//if a color component is > 1, cap color components using highest component value
			float	cMax = vertColor.red;
			if (cMax < vertColor.green)
				cMax = vertColor.green;
			if (cMax < vertColor.blue)
				cMax = vertColor.blue;
			if (cMax > 1) {
				vertColor.red /= cMax;
				vertColor.green /= cMax;
				vertColor.blue /= cMax;
				}
			}
		}
	vertColorP = gameData.render.color.vertices + nVertex;
	vertColorP->color = vertColor;
	vertColorP->index = gameStates.render.nFrameFlipFlop + 1;
	}
vld.nVertices = 0;
vld.nLights = 0;
return 1;
}

//------------------------------------------------------------------------------

static int ComputeVertexLight (short nVertex, int nState, tFaceColor *colorP)
{
	int	nLights, h, i, j;

	static float		quadCoord [4][2] = {{0, 0}, {0, VLBUF_WIDTH}, {VLBUF_WIDTH, VLBUF_WIDTH}, {VLBUF_WIDTH, 0}};
	static float		texCoord [4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
	static fVector3	matSpecular = {{1.0f, 1.0f, 1.0f}};
	static char			*szTexNames [VERTLIGHT_BUFFERS] = {"vertPosTex", "vertNormTex", "lightPosTex", "lightColorTex"};

if (!gameStates.ogl.bVertexLighting)
	return 0;
if (nState == 0) {
	glMatrixMode (GL_PROJECTION);    
	glPushMatrix ();
	glLoadIdentity ();
	glOrtho (0.0, VLBUF_WIDTH, 0.0, VLBUF_WIDTH, -1.0, 1.0);
	glMatrixMode (GL_MODELVIEW);                         
	glPushMatrix ();
	glLoadIdentity ();
	glPushAttrib (GL_VIEWPORT_BIT);
   glViewport (0, 0, VLBUF_WIDTH, VLBUF_WIDTH);
	OglEnableFBuffer (&gameData.render.lights.dynamic.fb);
#if 1
	glUseProgramObject (hVertLightShader);
	for (i = 0; i < VL_SHADER_BUFFERS; i++) {
		glUniform1i (glGetUniformLocation (hVertLightShader, szTexNames [i]), i);
		if (j = glGetError ()) {
			ComputeVertexLight (-1, 2, NULL);
			return 0;
			}
		}
	glUniform1f (glGetUniformLocation (hVertLightShader, "lightRange"), gameStates.ogl.fLightRange);
#endif
#if 0
	glUniform1f (glGetUniformLocation (hVertLightShader, "shininess"), 64.0f);
	glUniform3fv (glGetUniformLocation (hVertLightShader, "matAmbient"), 1, (GLfloat *) &gameData.render.vertColor.matAmbient);
	glUniform3fv (glGetUniformLocation (hVertLightShader, "matDiffuse"), 1, (GLfloat *) &gameData.render.vertColor.matDiffuse);
	glUniform3fv (glGetUniformLocation (hVertLightShader, "matSpecular"), 1, (GLfloat *) &matSpecular);
#endif
	OglDrawBuffer (GL_COLOR_ATTACHMENT0_EXT, 0); 
	OglReadBuffer (GL_COLOR_ATTACHMENT0_EXT, 0);
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);
	glDisable (GL_ALPHA_TEST);
	glDisable (GL_DEPTH_TEST);
	glColor3f (0,0,0);
#if VERTLIGHT_DRAWARRAYS
	for (i = 0; i < VL_SHADER_BUFFERS; i++) {
		G3EnableClientStates (1, 0, 0, GL_TEXTURE0 + i);
		glTexCoordPointer (2, GL_FLOAT, 0, texCoord);
		glVertexPointer (2, GL_FLOAT, 0, quadCoord);
		}
#endif
	if (j = glGetError ()) {
		ComputeVertexLight (-1, 2, NULL);
		return 0;
		}
	glDepthMask (0);
	}
else if (nState == 1) {
	tShaderLight	*psl;
	int				bSkipHeadLight = gameStates.ogl.bHeadLight && (gameData.render.lights.dynamic.headLights.nLights > 0) && !gameStates.render.nState;
	fVector			vPos = gameData.segs.fVertices [nVertex],
						vNormal = gameData.segs.points [nVertex].p3_normal.vNormal;
		
	SetNearestVertexLights (nVertex, 1, 0, 1, 0);
	if (!(h = gameData.render.lights.dynamic.shader.nActiveLights [0]))
		return 1;
	if (h > VLBUF_SIZE)
		h = VLBUF_SIZE;
	if (vld.nVertices >= VLBUF_SIZE)
		RenderVertLightBuffers ();
	else if (vld.nLights + h > VLBUF_SIZE)
		RenderVertLightBuffers ();
#ifdef _DEBUG
	if (nVertex == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	for (nLights = 0, i = vld.nLights, j = 0; j < h; i++, j++) {
		psl = gameData.render.lights.dynamic.shader.activeLights [0][j];
		if (bSkipHeadLight && (psl->nType == 3))
			continue;
		vld.buffers [0][i] = vPos;
		vld.buffers [1][i] = vNormal;
		vld.buffers [2][i] = psl->pos [0];
		vld.buffers [3][i] = psl->color;
		vld.buffers [0][i].p.w = 1.0f;
		vld.buffers [1][i].p.w = (psl->nType < 2) ? 1.0f : 0.0f;
		vld.buffers [2][i].p.w = psl->rad;
		vld.buffers [3][i].p.w = psl->brightness;
		nLights++;
		}
	if (nLights) {
		vld.index [vld.nVertices].nVertex = nVertex;
		vld.index [vld.nVertices].nLights = nLights;
		vld.index [vld.nVertices].color = colorP->color;
		vld.nVertices++;
		vld.nLights += nLights;
		}

	gameData.render.lights.dynamic.shader.nActiveLights [0] = gameData.render.lights.dynamic.shader.iVariableLights [0];
	}	
else if (nState == 2) {
	RenderVertLightBuffers ();
	glUseProgramObject (0);
	OglDisableFBuffer (&gameData.render.lights.dynamic.fb);
	glMatrixMode (GL_PROJECTION);    
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);                         
	glPopMatrix ();
	glPopAttrib ();
	OglDrawBuffer (GL_BACK, 1);
	for (i = 0; i < VL_SHADER_BUFFERS; i++) {
		G3DisableClientStates (1, 0, 0, GL_TEXTURE0 + i);
		glActiveTexture (GL_TEXTURE0 + i);
		glBindTexture (GL_TEXTURE_2D, 0);
		}
	glDepthMask (1);
	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LESS);
	glEnable (GL_ALPHA_TEST);
	glAlphaFunc (GL_GEQUAL, (float) 0.01);	
	OglDrawBuffer (GL_BACK, 1);
	}
return 1;
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
#	if SHADER_VERTEX_LIGHTING
if (bDynLight && gameStates.ogl.bVertexLighting)
	gameStates.ogl.bVertexLighting = ComputeVertexLight (-1, 0, NULL);
#endif
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
#	if SHADER_VERTEX_LIGHTING
								if (!ComputeVertexLight (nVertex, 1, &c))
#	endif
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
#	if SHADER_VERTEX_LIGHTING
if (bDynLight && gameStates.ogl.bVertexLighting)
	ComputeVertexLight (-1, 2, NULL);
#endif
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
	nSegment =gameData.render.mine.nSegRenderList [--nListPos];
	if (nSegment < 0) {
		if (nSegment == -0x7fffffff)
			continue;
		nSegment = -nSegment - 1;
		}
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

char *vertLightFS = 
	"uniform sampler2D vertPosTex, vertNormTex, lightPosTex, lightColorTex;\r\n" \
	"uniform float lightRange;\r\n" \
	"void main (void) {\r\n" \
	"	vec3 vertPos = texture2D (vertPosTex, gl_TexCoord [0].xy).xyz;\r\n" \
	"	vec3 vertNorm = texture2D (vertNormTex, gl_TexCoord [0].xy).xyz;\r\n" \
	"	vec3 lightPos = texture2D (lightPosTex, gl_TexCoord [0].xy).xyz;\r\n" \
	"	vec3 lightColor = texture2D (lightColorTex, gl_TexCoord [0].xy).xyz;\r\n" \
	"	vec3 lightDir = lightPos - vertPos;\r\n" \
	"	float type = texture2D (vertNormTex, gl_TexCoord [0].xy).a;\r\n" \
	"	float radius = texture2D (lightPosTex, gl_TexCoord [0].xy).a;\r\n" \
	"	float brightness = texture2D (lightColorTex, gl_TexCoord [0].xy).a;\r\n" \
	"	float attenuation, lightDist, NdotL, RdotE;\r\n" \
	"  vec3 matAmbient = vec3 (0.01, 0.01, 0.01);\r\n" \
	"  float shininess = 64.0;\r\n" \
	"  vec3 vertColor = vec3 (0.0, 0.0, 0.0);\r\n" \
	"	lightDist = length (lightDir) / lightRange - type * radius;\r\n" \
	"	lightDir = normalize (lightDir);\r\n" \
	"	if (lightDist < 1.0) {\r\n" \
	"		lightDist = 1.4142;\r\n" \
	"		NdotL = 1.0;\r\n" \
	"		}\r\n" \
	"	else {\r\n" \
	"		lightDist *= lightDist * (type + 1.0);\r\n" \
	"		NdotL = max (dot (vertNorm, lightDir), 0.0);\r\n" \
	"		}\r\n" \
	"	attenuation = lightDist / brightness;\r\n" \
	"	vertColor = (matAmbient + vec3 (NdotL, NdotL, NdotL)) * lightColor;\r\n" \
	"	if (NdotL > 0.0) {\r\n" \
	"		RdotE = max (dot (normalize (reflect (-lightDir, vertNorm)), normalize (-lightPos)), 0.0);\r\n" \
	"		vertColor += lightColor * pow (RdotE, shininess);\r\n" \
	"		}\r\n" \
	"  gl_FragColor = vec4 (vertColor / attenuation, 1.0);\r\n" \
	"	}";

char *vertLightVS = 
	"void main(void){" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;"\
	"/*gl_TexCoord [1] = gl_MultiTexCoord1;"\
	"gl_TexCoord [2] = gl_MultiTexCoord2;"\
	"gl_TexCoord [3] = gl_MultiTexCoord3;*/"\
	"gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;" \
	"gl_FrontColor = gl_Color;}";

//-------------------------------------------------------------------------

void InitVertLightShader (void)
{
	int	bOk;

if (!gameStates.ogl.bVertexLighting)
	return;
gameStates.ogl.bVertexLighting = 0;
#if SHADER_VERTEX_LIGHTING
if (gameStates.ogl.bRender2TextureOk && gameStates.ogl.bShadersOk && gameOpts->render.nPath) {
	LogErr ("building vertex lighting shader program\n");
	gameStates.ogl.bVertexLighting = 1;
	if (hVertLightShader)
		DeleteShaderProg (&hVertLightShader);
	bOk = CreateShaderProg (&hVertLightShader) &&
			CreateShaderFunc (&hVertLightShader, &hVertLightFS, &hVertLightVS, vertLightFS, vertLightVS, 1) &&
			LinkShaderProg (&hVertLightShader);
	if (!bOk) {
		gameStates.ogl.bVertexLighting = 0;
		DeleteShaderProg (&hVertLightShader);
		}
	}
#endif
}

//------------------------------------------------------------------------------
// eof