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

#define HW_VERTEX_LIGHTING 0

extern tG3FaceDrawerP g3FaceDrawer = G3DrawFaceArrays;

//------------------------------------------------------------------------------

#define FACE_BUFFER_SIZE	100

typedef struct tFaceBuffer {
	grsBitmap	*bmP;
	short			nFaces;
	short			nElements;
	int			bTextured;
	ushort		index [FACE_BUFFER_SIZE * 4];
} tFaceBuffer;

static tFaceBuffer faceBuffer = {NULL, 0, 0, 0, {0}};

//------------------------------------------------------------------------------

extern GLhandleARB headLightShaderProgs [2][4];
extern GLhandleARB perPixelLightingShaderProgs [MAX_LIGHTS_PER_PIXEL][4];
extern GLhandleARB gsShaderProg [2][3];

//------------------------------------------------------------------------------

void G3FlushFaceBuffer (int bForce)
{
#if G3_BUFFER_FACES
if ((faceBuffer.nFaces && bForce) || (faceBuffer.nFaces >= FACE_BUFFER_SIZE)) {
	glDrawElements (GL_TRIANGLE_FAN, faceBuffer.nElements, GL_UNSIGNED_SHORT, faceBuffer.index);
	faceBuffer.nFaces = 
	faceBuffer.nElements = 0;
	}
#endif
}

//------------------------------------------------------------------------------

void G3FillFaceBuffer (grsFace *faceP, int bTextured)
{
	int	i, j = faceP->nIndex;

faceBuffer.bmP = faceP->bmBot;
faceBuffer.bTextured = bTextured;
for (i = 4; i; i--)
	faceBuffer.index [faceBuffer.nElements++] = j++;
faceBuffer.nFaces++;
}

//------------------------------------------------------------------------------

int G3SetupShader (grsFace *faceP, int bColorKey, int bMultiTexture, int bTextured, int bColored, tRgbaColorf *colorP)
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
else if (faceP && gameStates.render.bPerPixelLighting)
	nShader = G3SetupPerPixelShader (faceP, nType);
else if (gameData.render.lights.dynamic.headLights.nLights && !gameStates.render.automap.bDisplay)
	nShader = G3SetupHeadLightShader (nType, HaveLightMaps (), colorP);
else if (bColorKey || bMultiTexture) 
	nShader = G3SetupTexMergeShader (bColorKey, bColored);
else if (gameStates.render.history.nShader >= 0) {
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
	}
}

#endif

//------------------------------------------------------------------------------

int G3DrawFaceArrays (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly)
{
	int			bOverlay, bColored, bTransparent, bColorKey = 0, bMonitor = 0, bMultiTexture = 0, 
					iMax = 0, index [4];
	grsBitmap	*bmMask = NULL;
	tTexCoord2f	*ovlTexCoordP;

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#if 0
else
	return 1;
#endif
if (bmBot && strstr (bmBot->szName, "door45#4"))
	bmBot = bmBot;
if (bmTop && strstr (bmTop->szName, "door35#4"))
	bmTop = bmTop;
#endif

if (!faceP->bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = BmOverride (bmBot, -1);
#if 0
if (bmBot && strstr (bmBot->szName, "door"))
	bTransparent = 0;
else
#endif
	bTransparent = faceP->bTransparent || (bmBot && (((bmBot->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU)) == (BM_FLAG_TRANSPARENT))));

if (bDepthOnly) {
	if (bTransparent || faceP->bOverlay)
		return 0;
	bOverlay = 0;
	}
else {
	bMonitor = (faceP->nCamera >= 0);
	bColored = !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [faceP->nSegment];
	if (bmTop && !bMonitor) {
		if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
			bmTop = BM_CURFRAME (bmTop);
			}
		else
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bOverlay = (bColorKey && gameStates.ogl.bGlTexMerge) ? 1 : -1;
		bMultiTexture = gameOpts->ogl.bPerPixelLighting && ((bOverlay > 0) || ((bOverlay < 0) && !bMonitor));
		}
	else
		bOverlay = 0;
#ifdef _DEBUG
	if (bMonitor)
		faceP = faceP;
#endif
	if (bTransparent && (gameStates.render.nType < 4) && !(bMonitor || bmTop || faceP->bSplit || faceP->bOverlay)) {
#ifdef _DEBUG
		if (gameOpts->render.bDepthSort > 0) 
#endif
			{
#if 0//def _DEBUG
			if ((faceP->nSegment != nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide != nDbgSide)))
				return 1;
#endif
			faceP->nRenderType = gameStates.render.history.nType;
			faceP->bColored = bColored;
			RIAddFace (faceP);
			return 0;
			}
		}
	}

#if G3_BUFFER_FACES
	G3FlushFaceBuffer (bMonitor || (bTextured != faceBuffer.bTextured) || faceP->bmTop || (faceP->bmBot != faceBuffer.bmP) || (faceP->nType != SIDE_IS_QUAD));
#endif
if (bTextured) {
	if ((bmBot != gameStates.render.history.bmBot) || 
		 (bmTop != gameStates.render.history.bmTop) || 
		 (bOverlay != gameStates.render.history.bOverlay) || 
		 (bColored != gameStates.render.history.bColored)) {
		if (bOverlay > 0) {
#ifdef _DEBUG
		if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
			if (bDepthOnly)
				nDbgSeg = nDbgSeg;
			else
				nDbgSeg = nDbgSeg;
		if (bmBot && strstr (bmBot->szName, "door"))
			bmBot = bmBot;
#endif
			// set base texture
			if (bmBot != gameStates.render.history.bmBot) {
				{INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightMapData.buffers, 1, 0);}
				gameStates.render.history.bmBot = bmBot;
				}
			// set overlay texture
			if (bmTop != gameStates.render.history.bmTop) {
				{INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightMapData.buffers, 1, 0);}
				gameStates.render.history.bmTop = bmTop;
				if (gameStates.render.history.bOverlay != 1) {	//enable multitexturing
					if (!G3EnableClientState (GL_TEXTURE_COORD_ARRAY, GL_TEXTURE1))
						return 1;
					}
				}
			if (!(bmMask = gameStates.render.textures.bHaveMaskShader ? BM_MASK (bmTop) : NULL))
				bColorKey = 0;
			else {
				{INIT_TMU (InitTMU2, GL_TEXTURE2, bmMask, lightMapData.buffers, 2, 0);}
				if (!G3EnableClientState (GL_TEXTURE_COORD_ARRAY, GL_TEXTURE2))
					return 1;
				}
			gameStates.render.history.bmMask = bmMask;
			G3SetupShader (faceP, bColorKey, 1, 1, bColored, NULL);
			}
		else {
			if (gameStates.render.history.bOverlay > 0) {
				if (gameStates.ogl.bShadersOk)
					glUseProgramObject (0);
				if (gameStates.render.history.bmMask) {
					glActiveTexture (GL_TEXTURE2);
					glClientActiveTexture (GL_TEXTURE2);
					glDisableClientState (GL_TEXTURE_COORD_ARRAY);
					OGL_BINDTEX (0);
					gameStates.render.history.bmMask = NULL;
					}
				}
			if (bMultiTexture) {
				if (bmTop != gameStates.render.history.bmTop) {
					glActiveTexture (GL_TEXTURE1);
					if (bmTop) {
						{INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightMapData.buffers, 1, 0);}
						}
					else {
						glClientActiveTexture (GL_TEXTURE1);
						OGL_BINDTEX (0);
						}
					gameStates.render.history.bmTop = bmTop;
					}
				}
			else {
				if (gameStates.render.history.bmTop) {
					glActiveTexture (GL_TEXTURE1);
					glClientActiveTexture (GL_TEXTURE1);
					OGL_BINDTEX (0);
					gameStates.render.history.bmTop = NULL;
					}
				}
			if (bmBot != gameStates.render.history.bmBot) {
				{INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightMapData.buffers, 1, 0);}
				gameStates.render.history.bmBot = bmBot;
				}
			G3SetupShader (faceP, 0, bMultiTexture, bmBot != NULL, bColored, bmBot ? NULL : &faceP->color);
			}
		}
	else
		G3SetupShader (faceP, 0, bMultiTexture, bmBot != NULL, bColored, bmBot ? NULL : &faceP->color);
	gameStates.render.history.bOverlay = bOverlay;
	gameStates.render.history.bColored = bColored;
	}
else {
	bOverlay = 0;
	glActiveTexture (GL_TEXTURE0);
	glClientActiveTexture (GL_TEXTURE0);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	}
#if G3_BUFFER_FACES
if (!(bMonitor || bOverlay)) {
	G3FillFaceBuffer (faceP, bTextured);
	return 0;
	}
#endif
#if 0
if (bBlend) {
	glEnable (GL_BLEND);
	if (FaceIsAdditive (faceP))
		glBlendFunc (GL_ONE, GL_ONE);
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
else
	glDisable (GL_BLEND);
#endif
#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#endif
gameData.render.nTotalFaces++;
if (gameStates.render.bTriangleMesh && !bMonitor) {
#ifdef _DEBUG
	if ((nDbgFace >= 0) && (faceP - gameData.segs.faces.faces != nDbgFace))
		return 0;
	if (!bDepthOnly)
		RenderWireFrame (faceP, bTextured);
	if (gameOpts->render.debug.bTextures && ((nDbgFace < 0) || (faceP - gameData.segs.faces.faces == nDbgFace)))
#endif
#if USE_RANGE_ELEMENTS
		{
		GLsizei nElements = faceP->nTris * 3;
		glDrawRangeElements (GL_TRIANGLES, faceP->vertIndex [0], faceP->vertIndex [nElements - 1], nElements, GL_UNSIGNED_INT, faceP->vertIndex);
		}	
#else
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, faceP->nTris * 3);
#endif
	}	
else if (gameOpts->ogl.bPerPixelLighting) {
	for (;;) {
		glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
		if (gameStates.ogl.iLight >= gameStates.ogl.nLights)
			break;
		G3SetupPerPixelShader (faceP, gameStates.render.history.nType);
		glUniform1f (glGetUniformLocation (activeShaderProg, "bStaticColor"), 0.0f);
		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
		glDepthFunc (GL_EQUAL);
		}
	glDepthFunc (GL_LEQUAL);
	}
else {
#ifdef _DEBUG
	if (!bDepthOnly)
		RenderWireFrame (faceP, bTextured);
	if (gameOpts->render.debug.bTextures) {
#endif
#if 1
	// this is a work around for OpenGL's per vertex light interpolation
	// rendering a quad is always started with the brightest vertex
	tRgbaColorf	*pc = gameData.segs.faces.color + faceP->nIndex;
	float l, lMax = 0;
	int i, j, nIndex;

	for (i = 0; i < 4; i++, pc++) {
		l = pc->red + pc->green + pc->blue;
		if (lMax < l) {
			lMax = l;
			iMax = i;
			}
		}
	if (!iMax)
		glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
	else {
		nIndex = faceP->nIndex;
		for (i = 0, j = iMax; i < 4; i++, j %= 4)
			index [i] = nIndex + j++;
		glDrawElements (GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, index);
		}
#else
	glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
#endif
#ifdef _DEBUG
	}
#endif
	}

#ifdef _DEBUG
if (!gameOpts->render.debug.bTextures)
	return 0;
#endif
if (!bMultiTexture && (bOverlay || bMonitor)) {
gameData.render.nTotalFaces++;
#ifdef _DEBUG
	if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		if (bDepthOnly)
			nDbgSeg = nDbgSeg;
		else
			nDbgSeg = nDbgSeg;
#endif
	ovlTexCoordP = bMonitor ? faceP->pTexCoord - faceP->nIndex : gameData.segs.faces.ovlTexCoord;
	if (bTextured) {
		{INIT_TMU (InitTMU0, GL_TEXTURE0, bmTop, lightMapData.buffers, 1, 0);}
		if (gameData.render.lights.dynamic.headLights.nLights)
			glUniform1i (glGetUniformLocation (activeShaderProg, "baseTex"), 0);
		glActiveTexture (GL_TEXTURE0);
		glClientActiveTexture (GL_TEXTURE0);
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
		}
	else {
		glActiveTexture (GL_TEXTURE0);
		glClientActiveTexture (GL_TEXTURE0);
		glDisable (GL_TEXTURE_2D);
		OGL_BINDTEX (0);
		}
	glTexCoordPointer (2, GL_FLOAT, 0, ovlTexCoordP);
	if (gameStates.render.bTriangleMesh)
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, faceP->nTris * 3);
	else if (iMax)
		glDrawElements (GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, index);
	else
		glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
	glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.texCoord);
	gameStates.render.history.bmBot = bmTop;
	}
#if 0
if (!bBlend)
	glEnable (GL_BLEND);
#endif
return 0;
}

//------------------------------------------------------------------------------

int G3DrawFaceArraysPPLM (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly)
{
	int			bOverlay, bColored, bTransparent, bColorKey = 0, bMonitor = 0, bMultiTexture = 0;
	grsBitmap	*bmMask = NULL;

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#if 0
else
	return 1;
#endif
if (bmBot && strstr (bmBot->szName, "door45#4"))
	bmBot = bmBot;
if (bmTop && strstr (bmTop->szName, "door35#4"))
	bmTop = bmTop;
#endif

bColored = bDepthOnly || !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [faceP->nSegment];
if (!faceP->bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = BmOverride (bmBot, -1);
#if 0
if (bmBot && strstr (bmBot->szName, "door"))
	bTransparent = 0;
else
#endif
	bTransparent = faceP->bTransparent || (bmBot && (((bmBot->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU)) == (BM_FLAG_TRANSPARENT))));

if (bDepthOnly) {
	if (bTransparent || faceP->bOverlay)
		return 0;
	bOverlay = 0;
	}
else {
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
		bOverlay = bColorKey ? 1 : -1;
		bMultiTexture = 1;
		}
	else
		bOverlay = 0;
	gameStates.render.history.nType = bColorKey ? 3 : bMultiTexture ? 2 : (bmBot != NULL);
#ifdef _DEBUG
	if (bMonitor)
		faceP = faceP;
#endif
	if (bTransparent && (gameStates.render.nType < 4) && !(bMonitor || bmTop || faceP->bSplit || faceP->bOverlay)) {
#ifdef _DEBUG
		if (gameOpts->render.bDepthSort > 0) 
#endif
			{
#if 0//def _DEBUG
			if ((faceP->nSegment != nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide != nDbgSide)))
				return 1;
#endif
			faceP->nRenderType = gameStates.render.history.nType;
			faceP->bColored = bColored;
			RIAddFace (faceP);
			return 0;
			}
		}
	}

#if G3_BUFFER_FACES
	G3FlushFaceBuffer (bMonitor || (bTextured != faceBuffer.bTextured) || faceP->bmTop || (faceP->bmBot != faceBuffer.bmP) || (faceP->nType != SIDE_IS_QUAD));
#endif
if (bTextured) {
	if ((bmBot != gameStates.render.history.bmBot) || 
		 (bmTop != gameStates.render.history.bmTop) || 
		 (bOverlay != gameStates.render.history.bOverlay) || 
		 (bColored != gameStates.render.history.bColored)) {
		if (bOverlay > 0) {
#ifdef _DEBUG
			if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
				if (bDepthOnly)
					nDbgSeg = nDbgSeg;
				else
					nDbgSeg = nDbgSeg;
			if (bmBot && strstr (bmBot->szName, "door"))
				bmBot = bmBot;
#endif
			// set base texture
			if (bmBot != gameStates.render.history.bmBot) {
				{INIT_TMU (InitTMU1, GL_TEXTURE1, bmBot, lightMapData.buffers, 1, 1);}
				gameStates.render.history.bmBot = bmBot;
				}
			// set overlay texture
			if (bmTop != gameStates.render.history.bmTop) {
				{INIT_TMU (InitTMU2, GL_TEXTURE2, bmTop, lightMapData.buffers, 1, 1);}
				gameStates.render.history.bmTop = bmTop;
				if (gameStates.render.history.bOverlay != 1) {	//enable multitexturing
					if (!G3EnableClientState (GL_TEXTURE_COORD_ARRAY, GL_TEXTURE2))
						return 1;
					}
				}
			if (!(bmMask = gameStates.render.textures.bHaveMaskShader ? BM_MASK (bmTop) : NULL))
				bColorKey = 0;
			else {
				{INIT_TMU (InitTMU3, GL_TEXTURE3, bmMask, lightMapData.buffers, 2, 1);}
				if (!G3EnableClientState (GL_TEXTURE_COORD_ARRAY, GL_TEXTURE3))
					return 1;
				}
			gameStates.render.history.bmMask = bmMask;
			}
		else {
			if (gameStates.render.history.bOverlay > 0) {
				if (gameStates.ogl.bShadersOk)
					glUseProgramObject (0);
				if (gameStates.render.history.bmMask) {
					glActiveTexture (GL_TEXTURE3);
					glClientActiveTexture (GL_TEXTURE3);
					glDisableClientState (GL_TEXTURE_COORD_ARRAY);
					OGL_BINDTEX (0);
					gameStates.render.history.bmMask = NULL;
					}
				}
			if (bMultiTexture) {
				if (bmTop != gameStates.render.history.bmTop) {
					if (bmTop) {
						{INIT_TMU (InitTMU2, GL_TEXTURE2, bmTop, lightMapData.buffers, 1, 1);}
						}
					else {
						glClientActiveTexture (GL_TEXTURE2);
						glActiveTexture (GL_TEXTURE2);
						OGL_BINDTEX (0);
						}
					gameStates.render.history.bmTop = bmTop;
					}
				}
			else {
				if (gameStates.render.history.bmTop) {
					glActiveTexture (GL_TEXTURE2);
					glClientActiveTexture (GL_TEXTURE2);
					glEnable (GL_TEXTURE_2D);
					OGL_BINDTEX (0);
					gameStates.render.history.bmTop = NULL;
					}
				}
			if (bmBot != gameStates.render.history.bmBot) {
				{INIT_TMU (InitTMU1, GL_TEXTURE1, bmBot, lightMapData.buffers, 1, 1);}
				gameStates.render.history.bmBot = bmBot;
				}
			}
		}
	gameStates.render.history.bOverlay = bOverlay;
	gameStates.render.history.bColored = bColored;
	}
else {
	bOverlay = 0;
	glActiveTexture (GL_TEXTURE1);
	glClientActiveTexture (GL_TEXTURE1);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	}
#if G3_BUFFER_FACES
if (!(bMonitor || bOverlay)) {
	G3FillFaceBuffer (faceP, bTextured);
	return 0;
	}
#endif
#if 0
if (bBlend) {
	glEnable (GL_BLEND);
	if (FaceIsAdditive (faceP))
		glBlendFunc (GL_ONE, GL_ONE);
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
else
	glDisable (GL_BLEND);
#endif

#ifdef _DEBUG
if (!bDepthOnly) {
	RenderWireFrame (faceP, bTextured);
	if (!gameOpts->render.debug.bTextures)
		return 0;
	}
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#endif
if (bDepthOnly) {
	if (gameStates.render.bTriangleMesh)
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
	else
		glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
	return 1;
	}
if (bMonitor) {
	if (bOverlay) {
		glActiveTexture (GL_TEXTURE2);
		glClientActiveTexture (GL_TEXTURE2);
		}
	else {
		glActiveTexture (GL_TEXTURE1);
		glClientActiveTexture (GL_TEXTURE1);
		}
	if (bTextured)
		glTexCoordPointer (2, GL_FLOAT, 0, faceP->pTexCoord - faceP->nIndex);
	else {
		glDisable (GL_TEXTURE_2D);
		OGL_BINDTEX (0);
		}
	}
gameData.render.nTotalFaces++;
gameStates.ogl.iLight = 0;
glBlendFunc (GL_ONE, GL_ZERO);
if (!bColored) {
	G3SetupGrayScaleShader (gameStates.render.history.nType, &faceP->color);
#if 1
	glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
#else
	if (gameStates.render.bTriangleMesh)
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
	else
		glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
#endif
	}
else if (gameStates.render.bFullBright) {
	if (bColorKey)
		G3SetupTexMergeShader (1, 0);
	else {
		glUseProgramObject (0);
		gameStates.render.history.nShader = -1;
		}
#if 1
	glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
#else
	if (gameStates.render.bTriangleMesh)
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
	else
		glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
#endif
	}
else {
	bool bResetBlendMode = false;
	if (gameData.render.lights.dynamic.headLights.nLights && !gameStates.render.automap.bDisplay) {
		G3SetupHeadLightShader (gameStates.render.history.nType, 1, bmBot ? NULL : &faceP->color);
#if 1
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
#else
		if (gameStates.render.bTriangleMesh)
			glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
		else
			glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
#endif
		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
		bResetBlendMode = true;
		//glDepthFunc (GL_EQUAL);
		}
	for (;;) {
		G3SetupPerPixelShader (faceP, gameStates.render.history.nType);
#if 1
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
#else
		if (gameStates.render.bTriangleMesh)
			glDrawArrays (GL_TRIANGLES, faceP->nIndex, 6);
		else
			glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
#endif
#if 1
		if ((gameStates.ogl.iLight >= gameStates.ogl.nLights) || 
			 (gameStates.ogl.iLight >= gameStates.render.nMaxLightsPerFace))
#endif
			break;
		if (!bResetBlendMode) {
			bResetBlendMode = true;
			glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
			glDepthFunc (GL_EQUAL);
			}
		}
	if (bResetBlendMode) {
		glDepthFunc (GL_LEQUAL);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}
if (bMonitor) {
	if (bOverlay) {
		glActiveTexture (GL_TEXTURE2);
		glClientActiveTexture (GL_TEXTURE2);
		glEnable (GL_TEXTURE_2D);
		glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.ovlTexCoord);
		gameStates.render.history.bmTop = NULL;
		}
	else {
		glActiveTexture (GL_TEXTURE1);
		glClientActiveTexture (GL_TEXTURE1);
		glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.texCoord);
		gameStates.render.history.bmBot = NULL;
		}
	}
#if 0
if (!bBlend)
	glEnable (GL_BLEND);
#endif
return 0;
}

//------------------------------------------------------------------------------

#if 0

int OglVertexLight (int nLights, int nPass, int iVertex)
{
	tActiveShaderLight	*activeLightsP;
	tShaderLight			*psl;
	int						iLight;
	tRgbaColorf				color = {1,1,1,1};
	GLenum					hLight;

if (nLights < 0)
	nLights = gameData.render.lights.dynamic.shader.nActiveLights [iVertex];
if (nLights) {
	if (nPass) {
		glActiveTexture (GL_TEXTURE1);
		glBlendFunc (GL_ONE, GL_ONE);
		glActiveTexture (GL_TEXTURE0);
		glBlendFunc (GL_ONE, GL_ONE);
		}
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
#if 0
else
	glColor4f (0,0,0,0);
#endif
activeLightsP = gameData.render.lights.dynamic.shader.activeLights [iVertex];
for (iLight = 0; (iLight < 8) && (nLights > 0); activeLightsP++) { 
	if (!(psl = GetActiveShaderLight (activeLightsP, iVertex)))
		continue;
#if 0
	if (psl->info.nType > 1) {
		iLight--;
		continue;
		}
#endif
	nLights--;
	hLight = GL_LIGHT0 + iLight++;
	glEnable (hLight);
	color.red = psl->info.color.red;
	color.green = psl->info.color.green;
	color.blue = psl->info.color.blue;
//			sprintf (szLightSources + strlen (szLightSources), "%d ", (psl->nObject >= 0) ? -psl->nObject : psl->nSegment);
	glLightfv (hLight, GL_POSITION, (GLfloat *) psl->vPosf);
	glLightfv (hLight, GL_DIFFUSE, (GLfloat *) &color);
	glLightfv (hLight, GL_SPECULAR, (GLfloat *) &color);
	if (psl->info.nType == 2) {
		glLightf (hLight, GL_CONSTANT_ATTENUATION,1.0f);
		glLightf (hLight, GL_LINEAR_ATTENUATION, 0.01f / psl->info.fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.001f / psl->info.fBrightness);
		}
	else {
		glLightf (hLight, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf (hLight, GL_LINEAR_ATTENUATION, 0.01f / psl->info.fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.001f / psl->info.fBrightness);
		}
	}
for (; iLight < 8; iLight++)
	glDisable (GL_LIGHT0 + iLight);
return nLights;
}

//------------------------------------------------------------------------------

int G3SetupFaceLight (grsFace *faceP, int bTextured)
{
	int	h, i, nLights = gameData.render.lights.dynamic.shader.index [0][0].nActive;

for (h = i = 0; i < 4; i++) {
	if (i) {
		memcpy (gameData.render.lights.dynamic.shader.activeLights + i, 
					gameData.render.lights.dynamic.shader.activeLights,
					nLights * sizeof (void *));
		gameData.render.lights.dynamic.shader.nActiveLights [i] = nLights;
		}
	SetNearestVertexLights (faceP->index [i], NULL, 0, 0, 1, i);
	h += gameData.render.lights.dynamic.shader.nActiveLights [i];
	
	}
if (h)
   OglEnableLighting (0);
return h;
}

//------------------------------------------------------------------------------

int G3DrawFaceSimple (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly)
{
	int			h, i, j, nTextures, nRemainingLights, nLights [4], nPass = 0;
	int			bOverlay, bTransparent, 
					bColorKey = 0, bMonitor = 0, 
					bLighting = HW_GEO_LIGHTING && !bDepthOnly, 
					bMultiTexture = 0;
	grsBitmap	*bmMask = NULL, *bmP [2];
	tTexCoord2f	*texCoordP, *ovlTexCoordP;

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (!faceP->bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = BmOverride (bmBot, -1);
bTransparent = faceP->bTransparent || (bmBot && ((bmBot->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU)) == BM_FLAG_TRANSPARENT));

if (bDepthOnly) {
	if (bTransparent || faceP->bOverlay)
		return 0;
	bOverlay = 0;
	}
else {
	bMonitor = gameStates.render.bUseCameras && (faceP->nCamera >= 0);
#ifdef _DEBUG
	if (bMonitor)
		faceP = faceP;
#endif
	if (bTransparent && !(bMonitor || bmTop || faceP->bSplit || faceP->bOverlay)) {
#ifdef _DEBUG
		if (gameOpts->render.bDepthSort > 0) 
#endif
			{
#if 0//def _DEBUG
			if ((faceP->nSegment != nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide != nDbgSide)))
				return 1;
#endif
			RIAddFace (faceP);
			return 0;
			}
		}
	}
if (bTextured) {
	if (bmTop && !bMonitor) {
		if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
			bmTop = BM_CURFRAME (bmTop);
			}
		else
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bOverlay = (bColorKey && gameStates.ogl.bGlTexMerge) ? 1 : -1;
		bMultiTexture = HW_GEO_LIGHTING && (bOverlay > 0);
		}
	else
		bOverlay = 0;
	if ((bmBot != gameStates.render.history.bmBot) || 
		 (bmTop != gameStates.render.history.bmTop) || 
		 (bOverlay != gameStates.render.history.bOverlay)) {
		if (bOverlay > 0) {
			bmMask = gameStates.render.textures.bHaveMaskShader ? BM_MASK (bmTop) : NULL;
			// set base texture
			if (bmBot != gameStates.render.history.bmBot) {
				{INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightMapData.buffers, 0, 0);}
				gameStates.render.history.bmBot = bmBot;
				}
			// set overlay texture
			if (bmTop != gameStates.render.history.bmTop) {
				{INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightMapData.buffers, 0, 0);}
				gameStates.render.history.bmTop = bmTop;
				}
			if (bmMask) {
				{INIT_TMU (InitTMU2, GL_TEXTURE2, bmMask, lightMapData.buffers, 0, 0);}
				glUniform1i (glGetUniformLocation (activeShaderProg, "maskTex"), 2);
				}
			gameStates.render.history.bmMask = bmMask;
			bmTop = NULL;
			G3SetupShader (faceP, bColorKey, 1, 1, !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [faceP->nSegment], NULL);
			}
		else {
			if (gameStates.render.history.bOverlay > 0) {
				if (gameStates.ogl.bShadersOk)
					glUseProgramObject (0);
				if (gameStates.render.history.bmMask) {
					glActiveTexture (GL_TEXTURE2);
					OGL_BINDTEX (0);
					gameStates.render.history.bmMask = NULL;
					}
				}
			gameStates.render.history.bmTop = NULL;
			//if (bmBot != gameStates.render.history.bmBot) 
				{
				{INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightMapData.buffers, 0, 0);}
				}
			}
		G3SetupShader (faceP, 0, bMultiTexture, bmBot != NULL, 
							!gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [faceP->nSegment],
							bmBot ? NULL : &faceP->color);
		}
	gameStates.render.history.bOverlay = bOverlay;
	}
else {
	bOverlay = 0;
	glDisable (GL_TEXTURE_2D);
	}
if (!bBlend)
	glDisable (GL_BLEND);

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#endif
bmP [0] = bmBot;
bmP [1] = bmTop;
nTextures = bmP [1] ? 2 : 1;
for (h = 0; h < nTextures; ) {
	gameStates.render.history.bmBot = bmP [h];
	nPass = 0;
	nLights [0] = nLights [1] = nLights [2] = nLights [3] = -1;
	nRemainingLights = bLighting;
	texCoordP = bMonitor ? faceP->pTexCoord : h ? gameData.segs.faces.ovlTexCoord : gameData.segs.faces.texCoord;
	if (bMultiTexture)
		ovlTexCoordP = bMonitor ? faceP->pTexCoord : gameData.segs.faces.ovlTexCoord;
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	do {
		j = faceP->nIndex;
		if (nPass)
			nRemainingLights = 0;
		glBegin (GL_TRIANGLE_FAN);
		for (i = 0; i < 4; i++, j++) {
	#ifdef _DEBUG
			if (faceP->index [i] == nDbgVertex)
				faceP = faceP;
	#endif
			if (bLighting && nPass) {
				if (!(nLights [i] = OglVertexLight (nLights [i], nPass, i)))
					glColor4f (0,0,0,0);
				else {
					nRemainingLights += nLights [i];
					if (bTextured)
						glColor4f (1,1,1,1);
					else
						glColor4fv ((GLfloat *) (gameData.segs.faces.color + faceP->nIndex));
					}
				}
			else
				glColor4fv ((GLfloat *) (gameData.segs.faces.color + j));
			if (bMultiTexture) {
				glMultiTexCoord2fv (GL_TEXTURE0, (GLfloat *) (texCoordP + j));
				glMultiTexCoord2fv (GL_TEXTURE1, (GLfloat *) (ovlTexCoordP + j));
				if (bmMask)
					glMultiTexCoord2fv (GL_TEXTURE2, (GLfloat *) (ovlTexCoordP + j));
				}
			else if (bmP [h])
				glTexCoord2fv ((GLfloat *) (texCoordP + j));
			glVertex3fv ((GLfloat *) (gameData.segs.faces.vertices + j));
			}
		glEnd ();
		if (bLighting && !(nPass || h)) {
			glBlendFunc (GL_ONE, GL_ONE);
			G3SetupFaceLight (faceP, bTextured);
			}
		nPass++;
		} while (nRemainingLights > 0);
	if (bLighting)
		OglDisableLighting ();
	if (++h >= nTextures)
		break;
	{INIT_TMU (InitTMU0, GL_TEXTURE0, bmP [h], lightMapData.buffers, 0, 0);}
	}
#if 0
if (bLighting) {
	for (i = 0; i < 4; i++)
		gameData.render.lights.dynamic.shader.nActiveLights [i] = gameData.render.lights.dynamic.shader.iVertexLights [i];
	}
#endif
if (!bBlend)
	glEnable (GL_BLEND);
return 0;
}

#endif

//------------------------------------------------------------------------------
//eof
