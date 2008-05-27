/* $Id: ogl.c, v 1.14 204/05/11 23:15:55 btb Exp $ */
/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
#else
# include <malloc.h>
# include <SDL.h>
#endif

#include "inferno.h"
#include "error.h"
#include "maths.h"
#include "light.h"
#include "dynlight.h"
#include "headlight.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texture.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "ogl_fastrender.h"
#include "ogl_tmu.h"
#include "texmerge.h"
#include "transprender.h"
#include "gameseg.h"

#define MAX_PP_LIGHTS_PER_FACE 32
#define MAX_PP_LIGHTS_PER_PASS 1

#define G3_BUFFER_FACES		1

extern tG3FaceDrawerP g3FaceDrawer = G3DrawFaceArrays;

//------------------------------------------------------------------------------

#define FACE_BUFFER_SIZE			1000
#define FACE_BUFFER_INDEX_SIZE	(FACE_BUFFER_SIZE * 4 * 4)

typedef struct tFaceBuffer {
	grsBitmap	*bmBot;
	grsBitmap	*bmTop;
	short			nFaces;
	short			nElements;
	int			bTextured;
	int			index [FACE_BUFFER_INDEX_SIZE];
} tFaceBuffer;

static tFaceBuffer faceBuffer = {NULL, 0, 0, 0, {0}};

//------------------------------------------------------------------------------

extern GLhandleARB headlightShaderProgs [2][4];
extern GLhandleARB perPixelLightingShaderProgs [MAX_LIGHTS_PER_PIXEL][4];
extern GLhandleARB gsShaderProg [2][3];

//------------------------------------------------------------------------------
// this is a work around for OpenGL's per vertex light interpolation
// rendering a quad is always started with the brightest vertex

void G3QuadIndex (grsFace *faceP, int *indexP)
{
	tRgbaColorf	*pc = gameData.segs.faces.color + faceP->nIndex;
	float			l, lMax = 0;
	int			i, j, nIndex;
	int			iMax = 0;

for (i = 0; i < 4; i++, pc++) {
	l = pc->red + pc->green + pc->blue;
	if (lMax < l) {
		lMax = l;
		iMax = i;
		}
	}
nIndex = faceP->nIndex;
if (!iMax) {
	for (i = 0; i < 4; i++)
		*indexP++ = nIndex++;
	}
else {
	for (i = 0, j = iMax; i < 4; i++, j %= 4)
		*indexP++ = nIndex + j++;
	}
}

//------------------------------------------------------------------------------

void G3FlushFaceBuffer (int bForce)
{
#if G3_BUFFER_FACES
if ((faceBuffer.nFaces && bForce) || (faceBuffer.nFaces >= FACE_BUFFER_SIZE)) {
	if (gameStates.render.bTriangleMesh)
		glDrawElements (GL_TRIANGLES, faceBuffer.nElements, GL_UNSIGNED_INT, faceBuffer.index);
	else
		glDrawElements (GL_QUADS, faceBuffer.nElements, GL_UNSIGNED_INT, faceBuffer.index);
	faceBuffer.nFaces = 
	faceBuffer.nElements = 0;
	}
#endif
}

//------------------------------------------------------------------------------

void G3FillFaceBuffer (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bTextured)
{
int i = faceP->nIndex,
	 j = gameStates.render.bTriangleMesh ? faceP->nTris * 3 : 4;

if ((faceBuffer.bmBot != bmBot) || (faceBuffer.bmTop != bmTop) || (faceBuffer.nElements + j > FACE_BUFFER_INDEX_SIZE)) {
	if (faceBuffer.nFaces)
		G3FlushFaceBuffer (1);
	faceBuffer.bmBot = faceP->bmBot;
	faceBuffer.bmTop = faceP->bmTop;
	}
faceBuffer.bTextured = bTextured;

if (gameStates.render.bTriangleMesh) {
	for (; j; j--)
		faceBuffer.index [faceBuffer.nElements++] = i++;
	}
else {
	G3QuadIndex (faceP, faceBuffer.index + faceBuffer.nElements);
	faceBuffer.nElements += 4;
	}
faceBuffer.nFaces++;
}

//------------------------------------------------------------------------------

int G3SetupShader (grsFace *faceP, int bDepthOnly, int bColorKey, int bMultiTexture, int bTextured, int bColored, tRgbaColorf *colorP)
{
	static grsBitmap	*nullBmP = NULL;

	int	nType, nShader = gameStates.render.history.nShader;

if (!gameStates.ogl.bShadersOk)
	return -1;
#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
nType = bColorKey ? 3 : bMultiTexture ? 2 : bTextured;
if (!bColored && gameOpts->render.automap.bGrayOut) 
	nShader = G3SetupGrayScaleShader (nType, colorP);
else if ((gameStates.render.nType != 4) && faceP && gameStates.render.bPerPixelLighting)
	nShader = G3SetupPerPixelShader (faceP, bDepthOnly, nType, false);
else if (gameData.render.lights.dynamic.headlights.nLights && !(bDepthOnly || gameStates.render.automap.bDisplay))
	nShader = G3SetupHeadlightShader (nType, HaveLightmaps (), colorP);
else if (bColorKey || bMultiTexture) 
	nShader = G3SetupTexMergeShader (bColorKey, bColored);
else if (gameStates.render.history.nShader >= 0) {
	gameData.render.nShaderChanges++;
	glUseProgramObject (0);
	nShader = -1;
	}
gameStates.render.history.nType = nType;
return gameStates.render.history.nShader = nShader;
}

//------------------------------------------------------------------------------

#ifdef _DEBUG

void RenderWireFrame (grsFace *faceP, int bTextured)
{
if (gameOpts->render.debug.bWireFrame) {
	if ((nDbgFace < 0) || (faceP - gameData.segs.faces.faces == nDbgFace)) {
		grsTriangle	*triP = gameData.segs.faces.tris + faceP->nTriIndex;
		glDisableClientState (GL_COLOR_ARRAY);
		if (bTextured)
			glDisable (GL_TEXTURE_2D);
		glColor3f (1.0f, 0.5f, 0.0f);
		glLineWidth (6);
		glBegin (GL_LINE_LOOP);
		for (int i = 0; i < 4; i++)
			glVertex3fv ((GLfloat *) (gameData.segs.fVertices + faceP->index [i]));
		glEnd ();
		if (gameStates.render.bTriangleMesh) {
			glLineWidth (2);
			glColor3f (1,1,1);
			for (int i = 0; i < faceP->nTris; i++, triP++)
				glDrawArrays (GL_LINE_LOOP, triP->nIndex, 3);
			glLineWidth (1);
			}
		if (gameOpts->render.debug.bDynamicLight)
			glEnableClientState (GL_COLOR_ARRAY);
		if (bTextured)
			glEnable (GL_TEXTURE_2D);
		}
	glLineWidth (1);
	}
}

#endif

//------------------------------------------------------------------------------

static inline void G3SetBlendMode (grsFace *faceP)
{
if (faceP->bAdditive)
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
else if (faceP->bTransparent)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
else
	glBlendFunc (GL_ONE, GL_ZERO);
}

//------------------------------------------------------------------------------

int G3SetRenderStates (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bDepthOnly, int bTextured, int bColorKey, int bColored)
{
PROF_START
if (bTextured) {
	bool bStateChange = false;
	grsBitmap *bmMask = NULL;
	if (bmBot != gameStates.render.history.bmBot) {
		bStateChange = true;
		gameStates.render.history.bmBot = bmBot;
		{INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightmapData.buffers, 1, 0);}
		}
	if (bmTop != gameStates.render.history.bmTop) {
		bStateChange = true;
		gameStates.render.history.bmTop = bmTop;
		if (bmTop) {
			{INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightmapData.buffers, 1, 0);}
			}
		else {
			glActiveTexture (GL_TEXTURE1);
			glClientActiveTexture (GL_TEXTURE1);
			OGL_BINDTEX (0);
			bmMask = NULL;
			}
		}
	if (bColorKey)
		bmMask = (bColorKey && gameStates.render.textures.bHaveMaskShader) ? BM_MASK (bmTop) : NULL;
	if (bmMask != gameStates.render.history.bmMask) {
		bStateChange = true;
		gameStates.render.history.bmMask = bmMask;
		if (bmMask) {
			{INIT_TMU (InitTMU2, GL_TEXTURE2, bmMask, lightmapData.buffers, 2, 0);}
			G3EnableClientState (GL_TEXTURE_COORD_ARRAY, GL_TEXTURE2);
			}
		else {
			glActiveTexture (GL_TEXTURE2);
			glClientActiveTexture (GL_TEXTURE2);
			OGL_BINDTEX (0);
			bColorKey = 0;
			}
		}
	if (bColored != gameStates.render.history.bColored) {
		bStateChange = true;
		gameStates.render.history.bColored = bColored;
		}
	if (bStateChange) {

		gameData.render.nStateChanges++;
		G3SetupShader (faceP, bDepthOnly, bColorKey, bmTop != NULL, bmBot != NULL, bColored, bmBot ? NULL : &faceP->color);
		}
	}
else {
	gameStates.render.history.bmBot = NULL;
	glActiveTexture (GL_TEXTURE0);
	glClientActiveTexture (GL_TEXTURE0);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	}
PROF_END(ptRenderStates)
return 1;
}

//------------------------------------------------------------------------------

int G3SetRenderStatesLM (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bDepthOnly, int bTextured, int bColorKey, int bColored)
{
PROF_START
if (bTextured) {
	bool bStateChange = false;
	grsBitmap *bmMask = NULL;
	if (bmBot != gameStates.render.history.bmBot) {
		bStateChange = true;
		gameStates.render.history.bmBot = bmBot;
		{INIT_TMU (InitTMU1, GL_TEXTURE1, bmBot, lightmapData.buffers, 1, 0);}
		}
	if (bmTop != gameStates.render.history.bmTop) {
		bStateChange = true;
		gameStates.render.history.bmTop = bmTop;
		if (bmTop) {
			{INIT_TMU (InitTMU2, GL_TEXTURE2, bmTop, lightmapData.buffers, 1, 0);}
			}
		else {
			glActiveTexture (GL_TEXTURE2);
			glClientActiveTexture (GL_TEXTURE2);
			OGL_BINDTEX (0);
			bmMask = NULL;
			}
		}
	if (bColorKey)
		bmMask = (bColorKey && gameStates.render.textures.bHaveMaskShader) ? BM_MASK (bmTop) : NULL;
	if (bmMask != gameStates.render.history.bmMask) {
		bStateChange = true;
		gameStates.render.history.bmMask = bmMask;
		if (bmMask) {
			{INIT_TMU (InitTMU3, GL_TEXTURE3, bmMask, lightmapData.buffers, 2, 0);}
			G3EnableClientState (GL_TEXTURE_COORD_ARRAY, GL_TEXTURE2);
			}
		else {
			glActiveTexture (GL_TEXTURE3);
			glClientActiveTexture (GL_TEXTURE3);
			OGL_BINDTEX (0);
			bColorKey = 0;
			}
		}
	if (bColored != gameStates.render.history.bColored) {
		gameStates.render.history.bColored = bColored;
		}
	if (bStateChange)
		gameData.render.nStateChanges++;
	}
else {
	gameStates.render.history.bmBot = NULL;
	glActiveTexture (GL_TEXTURE1);
	glClientActiveTexture (GL_TEXTURE1);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	}
PROF_END(ptRenderStates)
return 1;
}

//------------------------------------------------------------------------------

void G3SetupMonitor (grsFace *faceP, grsBitmap *bmTop, int bTextured, int bLightmaps)
{
if (bmTop) {
	glActiveTexture (GL_TEXTURE1 + bLightmaps);
	glClientActiveTexture (GL_TEXTURE1 + bLightmaps);
	}
else {
	glActiveTexture (GL_TEXTURE0 + bLightmaps);
	glClientActiveTexture (GL_TEXTURE0 + bLightmaps);
	}
if (bTextured)
	glTexCoordPointer (2, GL_FLOAT, 0, faceP->pTexCoord - faceP->nIndex);
else {
	glDisable (GL_TEXTURE_2D);
	OGL_BINDTEX (0);
	}
}

//------------------------------------------------------------------------------

void G3ResetMonitor (grsBitmap *bmTop, int bLightmaps)
{
if (bmTop) {
	glActiveTexture (GL_TEXTURE1 + bLightmaps);
	glClientActiveTexture (GL_TEXTURE1 + bLightmaps);
	glEnable (GL_TEXTURE_2D);
	glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.ovlTexCoord);
	gameStates.render.history.bmTop = NULL;
	}
else {
	glActiveTexture (GL_TEXTURE0 + bLightmaps);
	glClientActiveTexture (GL_TEXTURE0 + bLightmaps);
	glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.texCoord);
	gameStates.render.history.bmBot = NULL;
	}
}

//------------------------------------------------------------------------------

static inline int G3FaceIsTransparent (grsFace *faceP, grsBitmap *bmBot)
{
return faceP->bTransparent || (bmBot && (((bmBot->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU)) == (BM_FLAG_TRANSPARENT))));
}

//------------------------------------------------------------------------------

static inline int G3FaceIsColored (grsFace *faceP)
{
return !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [faceP->nSegment] || !gameOpts->render.automap.bGrayOut;
}

//------------------------------------------------------------------------------

int G3DrawFaceArrays (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly)
{
PROF_START
	int			bColored, bTransparent, bColorKey = 0, bMonitor = 0;

#if 0//def _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#endif

if (!faceP->bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = BmOverride (bmBot, -1);
bTransparent = G3FaceIsTransparent (faceP, bmBot);
if (bDepthOnly) {
	if (bTransparent || faceP->bOverlay)
		return 0;
	bColored = 0;
	}
else {
	bColored = G3FaceIsColored (faceP);
	bMonitor = (faceP->nCamera >= 0);
	if (bmTop) {
		if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
			bmTop = BM_CURFRAME (bmTop);
			}
		else
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
		}
	gameStates.render.history.nType = bColorKey ? 3 : (bmTop != NULL) ? 2 : (bmBot != NULL);
	if (bTransparent && (gameStates.render.nType < 4) && !(bMonitor || bmTop)) {
		faceP->nRenderType = gameStates.render.history.nType;
		faceP->bColored = bColored;
		RIAddFace (faceP);
		return 0;
		}
	}

#if G3_BUFFER_FACES
if (!bDepthOnly) {
	if (bMonitor)
		G3FlushFaceBuffer (1);
	else
		G3FillFaceBuffer (faceP, bmBot, bmTop, bTextured);
	}
#endif
gameStates.ogl.iLight = 0;
G3SetRenderStates (faceP, bmBot, bmTop, bDepthOnly, bTextured, bColorKey, bColored);
G3SetBlendMode (faceP);
if (bDepthOnly) {
	if (gameStates.render.bTriangleMesh)
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, faceP->nTris * 3);
	else
		glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
	return 0;
	}

gameData.render.nTotalFaces++;
#ifdef _DEBUG
RenderWireFrame (faceP, bTextured);
if (!gameOpts->render.debug.bTextures)
	return 0;
#endif

if (bMonitor)
	G3SetupMonitor (faceP, bmTop, bTextured, 0);
#if G3_BUFFER_FACES
else 
	return 0;
#endif
if (gameStates.render.bTriangleMesh) {
#if USE_RANGE_ELEMENTS
		{
		GLsizei nElements = faceP->nTris * 3;
		glDrawRangeElements (GL_TRIANGLES, faceP->vertIndex [0], faceP->vertIndex [nElements - 1], nElements, GL_UNSIGNED_INT, faceP->vertIndex);
		}	
#else
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, faceP->nTris * 3);
#endif
	}	
else {
#if 1
	int	index [4];

	G3QuadIndex (faceP, index);
	glDrawElements (GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, index);
#else
	glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
#endif
	}

if (bMonitor)
	G3ResetMonitor (bmTop, 0);
PROF_END(ptRenderFaces)
return 0;
}

//------------------------------------------------------------------------------

int G3DrawFaceArraysPPLM (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly)
{
PROF_START
	int			bColored, bTransparent, bColorKey = 0, bMonitor = 0;

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#endif

if (!faceP->bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = BmOverride (bmBot, -1);
	bTransparent = G3FaceIsTransparent (faceP, bmBot);

if (bDepthOnly) {
	if (bTransparent || faceP->bOverlay)
		return 0;
	bColored = 0;
	}
else {
	bColored = G3FaceIsColored (faceP);
	bMonitor = (faceP->nCamera >= 0);
#ifdef _DEBUG
	if (bmTop)
		bmTop = bmTop;
#endif
	if (bmTop) {
		if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
			bmTop = BM_CURFRAME (bmTop);
			}
		else
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
		}
	gameStates.render.history.nType = bColorKey ? 3 : (bmTop != NULL) ? 2 : (bmBot != NULL);
	if (bTransparent && (gameStates.render.nType < 4) && !(bMonitor || bmTop)) {
		faceP->nRenderType = gameStates.render.history.nType;
		faceP->bColored = bColored;
		RIAddFace (faceP);
		return 0;
		}
	}

gameStates.ogl.iLight = 0;
G3SetRenderStatesLM (faceP, bmBot, bmTop, bDepthOnly, bTextured, bColorKey, bColored);
if (bDepthOnly) {
	glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
	PROF_END(ptRenderFaces)
	return 1;
	}
#ifdef _DEBUG
RenderWireFrame (faceP, bTextured);
if (!gameOpts->render.debug.bTextures)
	return 0;
#endif
G3SetBlendMode (faceP);
if (bMonitor)
	G3SetupMonitor (faceP, bmTop, bTextured, 1);
gameData.render.nTotalFaces++;
if (!bColored) {
	G3SetupGrayScaleShader (gameStates.render.history.nType, &faceP->color);
	glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
	}
else if (gameStates.render.bFullBright) {
	if (bColorKey)
		G3SetupTexMergeShader (1, 0);
	else if (gameStates.render.history.nShader != -1) {
		glUseProgramObject (0);
		gameStates.render.history.nShader = -1;
		gameData.render.nShaderChanges++;
		}
	glColor3f (1,1,1);
	glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
	}
else {
	bool bAdditive = false;
	for (;;) {
		G3SetupPerPixelShader (faceP, 0, gameStates.render.history.nType, false);
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
		if ((gameStates.ogl.iLight >= gameStates.ogl.nLights) || 
			 (gameStates.ogl.iLight >= gameStates.render.nMaxLightsPerFace))
			break;
		if (!bAdditive) {
			bAdditive = true;
			glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
			glDepthFunc (GL_EQUAL);
			}
		}
#if 0
	if (gameStates.render.bHeadlights) {
		if (!bAdditive) {
			bAdditive = true;
			glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
			}
		G3SetupHeadlightShader (gameStates.render.history.nType, 1, bmBot ? NULL : &faceP->color);
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
		}
#endif
	if (bAdditive)
		glDepthFunc (GL_LEQUAL);
	}

if (bMonitor)
	G3ResetMonitor (bmTop, 1);
PROF_END(ptRenderFaces)
return 0;
}

//------------------------------------------------------------------------------

int G3DrawHeadlights (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly)
{
	int			bColorKey = 0, bMonitor = 0;

if (!faceP->bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = BmOverride (bmBot, -1);
if (G3FaceIsTransparent (faceP, bmBot) && !(bMonitor || bmTop))
	return 0;
bMonitor = (faceP->nCamera >= 0);
if (bmTop) {
	if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
		bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bmTop = BM_CURFRAME (bmTop);
		}
	else
		bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
	}
gameStates.render.history.nType = bColorKey ? 3 : (bmTop != NULL) ? 2 : (bmBot != NULL);
G3SetRenderStates (faceP, bmBot, bmTop, 0, 1, bColorKey, 1);
if (bMonitor)
	G3SetupMonitor (faceP, bmTop, bTextured, 1);
gameData.render.nTotalFaces++;
G3SetupHeadlightShader (gameStates.render.history.nType, 1, bmBot ? NULL : &faceP->color);
glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);

if (bMonitor)
	G3ResetMonitor (bmTop, 1);
return 0;
}

//------------------------------------------------------------------------------

int G3DrawHeadlightsPPLM (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly)
{
	int			bColorKey = 0, bMonitor = 0;

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (!faceP->bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = BmOverride (bmBot, -1);
if (G3FaceIsTransparent (faceP, bmBot) && !(bMonitor || bmTop))
	return 0;
bMonitor = (faceP->nCamera >= 0);
if (bmTop) {
	if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
		bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bmTop = BM_CURFRAME (bmTop);
		}
	else
		bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
	}
gameStates.render.history.nType = bColorKey ? 3 : (bmTop != NULL) ? 2 : (bmBot != NULL);
G3SetRenderStatesLM (faceP, bmBot, bmTop, 0, 1, bColorKey, 1);
if (bMonitor)
	G3SetupMonitor (faceP, bmTop, bTextured, 1);
gameData.render.nTotalFaces++;
G3SetupHeadlightShader (gameStates.render.history.nType, 1, bmBot ? NULL : &faceP->color);
glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
if (bMonitor)
	G3ResetMonitor (bmTop, 1);
return 0;
}

//------------------------------------------------------------------------------
//eof
