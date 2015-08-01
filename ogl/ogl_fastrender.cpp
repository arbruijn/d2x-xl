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
# include <SDL.h>
#endif

#include "descent.h"
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
#include "renderlib.h"
#include "ogl_tmu.h"
#include "texmerge.h"
#include "transprender.h"
#include "segmath.h"
#include "automap.h"

#if DBG
#	define G3_BUFFER_FACES	0	// well ... no speed increase :/
#else
#	define G3_BUFFER_FACES	0
#endif

tFaceRenderFunc faceRenderFunc = RenderFace;

//------------------------------------------------------------------------------

#define FACE_BUFFER_SIZE			1000
#define FACE_BUFFER_INDEX_SIZE	(FACE_BUFFER_SIZE * 4 * 4)

typedef struct tFaceBuffer {
	CBitmap	*bmBot;
	CBitmap	*bmTop;
	int16_t	nFaces;
	int16_t	nElements;
	int32_t	bTextured;
	int32_t	index [FACE_BUFFER_INDEX_SIZE];
} tFaceBuffer;

static tFaceBuffer faceBuffer = {NULL, NULL, 0, 0, 0, {0}};

//------------------------------------------------------------------------------

extern GLhandleARB headlightShaderProgs [2][4];
extern GLhandleARB perPixelLightingShaderProgs [MAX_LIGHTS_PER_PIXEL][4];
extern GLhandleARB gsShaderProg [2][3];

//------------------------------------------------------------------------------
// this is a work around for OpenGL's per vertex light interpolation
// rendering a quad is always started with the brightest vertex

void G3BuildQuadIndex (CSegFace *pFace, int32_t *pIndex)
{
#if G3_BUFFER_FACES
int32_t nIndex = pFace->m_info.nIndex;
//for (int32_t i = 0; i < 4; i++)
*pIndex++ = nIndex++;
*pIndex++ = nIndex++;
*pIndex++ = nIndex++;
*pIndex = nIndex;
#else
	CFloatVector	*pc = FACES.color + pFace->m_info.nIndex;
	float			l, lMax = 0;
	int32_t			i, j, nIndex;
	int32_t			iMax = 0;

for (i = 0; i < 4; i++, pc++) {
	l = pc->Red () + pc->Green () + pc->Blue ();
	if (lMax < l) {
		lMax = l;
		iMax = i;
		}
	}
nIndex = pFace->m_info.nIndex;
if (!iMax) {
	for (i = 0; i < 4; i++)
		*pIndex++ = nIndex++;
	}
else {
	for (i = 0, j = iMax; i < 4; i++, j %= 4)
		*pIndex++ = nIndex + j++;
	}
#endif
}

//------------------------------------------------------------------------------

void G3FlushFaceBuffer (int32_t bForce)
{
#if G3_BUFFER_FACES
if (faceBuffer.nFaces && (bForce || (faceBuffer.nFaces >= FACE_BUFFER_SIZE))) {
	if (gameStates.render.bFullBright)
		glColor3f (1,1,1);
	try {
		if (gameStates.render.bTriangleMesh)
			glDrawElements (GL_TRIANGLES, faceBuffer.nElements, GL_UNSIGNED_INT, faceBuffer.index);
		else
			glDrawElements (GL_TRIANGLE_FAN, faceBuffer.nElements, GL_UNSIGNED_INT, faceBuffer.index);
		}
	catch(...) {
		PrintLog (0, "error calling glDrawElements (%d, %d) in G3FlushFaceBuffer\n", gameStates.render.bTriangleMesh ? "GL_TRIANGLES" : "GL_TRIANGLE_FAN");
		}
	faceBuffer.nFaces =
	faceBuffer.nElements = 0;
	}
#endif
}

//------------------------------------------------------------------------------

void G3FillFaceBuffer (CSegFace *pFace, CBitmap *bmBot, CBitmap *bmTop, int32_t bTextured)
{
#if DBG
if (!gameOpts->render.debug.bTextures)
	return;
#endif
#if 0
if (!gameStates.render.bTriangleMesh) {
	if (gameStates.render.bFullBright)
		glColor3f (1,1,1);
	OglDrawArrays (GL_TRIANGLE_FAN, pFace->m_info.nIndex, 4);
	}
else
#endif
	{
	int32_t	i = pFace->m_info.nIndex,
			j = gameStates.render.bTriangleMesh ? pFace->m_info.nTris * 3 : 4;

#if 0 //DBG
		if (i == nDbgVertex)
			BRP;
		if (i + j > int32_t (FACES.vertices.Length ())) {
			PrintLog (0, "invalid vertex index %d in G3FillFaceBuffer\n");
			return;
			}
#endif
	if (/*!gameStates.render.bTriangleMesh ||*/ (faceBuffer.bmBot != bmBot) || (faceBuffer.bmTop != bmTop)) {
		if (faceBuffer.nFaces)
			G3FlushFaceBuffer (1);
		faceBuffer.bmBot = bmBot;
		faceBuffer.bmTop = bmTop;
		}
	else if (faceBuffer.nElements + j > FACE_BUFFER_INDEX_SIZE)
		G3FlushFaceBuffer (1);
	faceBuffer.bTextured = bTextured;
	for (; j; j--)
		faceBuffer.index [faceBuffer.nElements++] = i++;
	faceBuffer.nFaces++;
	}
}

//------------------------------------------------------------------------------

int32_t SetupShader (CSegFace *pFace, int32_t bColorKey, int32_t bMultiTexture, int32_t bTextured, int32_t bColored, CFloatVector *pColor)
{
ENTER (0, 0);
	int32_t	nType, nShader = -1;

if (!ogl.m_features.bShaders || (gameStates.render.nType == RENDER_TYPE_SKYBOX))
	RETVAL (-1)
#if DBG
if (pFace && (pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
#endif
nType = bColorKey ? 3 : bMultiTexture ? 2 : bTextured;
#if 0
if ((gameStates.render.nType == RENDER_TYPE_ZCULL) && (nType > 1))
	nShader = SetupTexMergeShader (bColorKey, bColored, nType);
else 
#endif
if (!bColored && gameOpts->render.automap.bGrayOut)
	nShader = SetupGrayScaleShader (nType, pColor);
else if (!gameStates.render.bFullBright && pFace && (gameStates.render.bPerPixelLighting == 2))
	nShader = SetupPerPixelLightingShader (pFace, nType, false);
else if (gameStates.render.bHeadlights)
	nShader = lightManager.Headlights ().SetupShader (nType, lightmapManager.HaveLightmaps (), pColor);
else if (bColorKey || bMultiTexture)
	nShader = SetupTexMergeShader (bColorKey, bColored, nType);
else
	shaderManager.Deploy (-1);
ogl.ClearError (0);
RETVAL (nShader)
}

//------------------------------------------------------------------------------

#if DBG

void RenderWireFrame (CSegFace *pFace, int32_t bTextured)
{
if (gameOpts->render.debug.bWireFrame & 1) {
	if ((nDbgFace < 0) || (pFace - FACES.faces == nDbgFace)) {
		tFaceTriangle	*pTriangle = FACES.tris + pFace->m_info.nTriIndex;
		ogl.DisableClientState (GL_COLOR_ARRAY);
		if (bTextured)
			ogl.SetTexturing (false);
		glColor3f (1.0f, 0.5f, 0.0f);
		glLineWidth (6);
		if (ogl.SizeVertexBuffer (4)) {
			for (int32_t i = 0; i < 4; i++)
				ogl.VertexBuffer () [i] = gameData.segData.fVertices [pFace->m_info.index [i]];
			ogl.FlushBuffers (GL_LINE_LOOP, 4);
			}	
		if (gameStates.render.bTriangleMesh) {
			glLineWidth (2);
			glColor3f (1,1,1);
			for (int32_t i = 0; i < pFace->m_info.nTris; i++, pTriangle++)
				OglDrawArrays (GL_LINE_LOOP, pTriangle->nIndex, 3);
			glLineWidth (1);
			}
		if (gameOpts->render.debug.bDynamicLight)
			ogl.EnableClientState (GL_COLOR_ARRAY);
		if (bTextured)
			ogl.SetTexturing (true);
		}
	glLineWidth (1);
	}
}

#endif

//------------------------------------------------------------------------------

static inline void G3SetBlendMode (CSegFace *pFace)
{
if (pFace->m_info.bAdditive)
	ogl.SetBlendMode (OGL_BLEND_ADD_WEAK);
else if (pFace->m_info.bTransparent)
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
else
	ogl.SetBlendMode (OGL_BLEND_REPLACE);
}

//------------------------------------------------------------------------------

static inline CBitmap* SetupTMU (CBitmap* pBm, int32_t nTMU, int32_t nMode)
{
ogl.SelectTMU (nTMU, true);
ogl.SetTexturing (true);
glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, nMode);
if (pBm->Bind (1)) 
	return NULL; 
pBm = pBm->CurFrame (-1); 
pBm->Texture ()->Wrap (GL_REPEAT); 
return pBm;
}

//------------------------------------------------------------------------------

void inline ResetTMU (int32_t nTMU)
{
ogl.SelectTMU (nTMU, true);
ogl.BindTexture (0);
ogl.SetTexturing (false);
}

//------------------------------------------------------------------------------

int32_t SetRenderStates (CSegFace *pFace, CBitmap *bmBot, CBitmap *bmTop, 
								 int32_t bTextured, int32_t bColorKey, int32_t bColored, int32_t bLightmaps, 
								 bool bForce = false)
{
ENTER (0, 0);
PROF_START
if (bTextured) {
	bool bStateChange = false;
	CBitmap *bmMask = NULL;
	if (bForce || (bmBot != gameStates.render.history.bmBot)) {
		bStateChange = true;
		if (!(gameStates.render.history.bmBot = SetupTMU (bmBot, GL_TEXTURE0 + bLightmaps, GL_MODULATE)))
			RETVAL (0)
		}
	if (bForce || (bmTop != gameStates.render.history.bmTop)) {
		bStateChange = true;
		if (bmTop) {
			if (!(gameStates.render.history.bmTop = SetupTMU (bmTop, GL_TEXTURE1 + bLightmaps, GL_DECAL)))
				RETVAL (0)
			}
		else {
			gameStates.render.history.bmTop = NULL;
			ResetTMU (GL_TEXTURE1 + bLightmaps);
			}
		}
	bmMask = (bColorKey && gameStates.render.textures.bHaveMaskShader) ? bmTop->Mask () : NULL;
	if (bForce || (bmMask != gameStates.render.history.bmMask)) {
		bStateChange = true;
		if (bmMask) {
			if (!(gameStates.render.history.bmMask = SetupTMU (bmMask, GL_TEXTURE2 + bLightmaps, GL_MODULATE)))
				RETVAL (0)
			}
		else {
			gameStates.render.history.bmMask = NULL;
			ResetTMU (GL_TEXTURE2 + bLightmaps);
			bColorKey = 0;
			}
		}
	if (bColored != gameStates.render.history.bColored) {
		bStateChange = true;
		gameStates.render.history.bColored = bColored;
		}
	if (bStateChange) {
		gameData.renderData.nStateChanges++;
		if (!(bLightmaps || gameStates.render.bFullBright))
			SetupShader (pFace, bColorKey, bmTop != NULL, bmBot != NULL, bColored, bmBot ? NULL : &pFace->m_info.color);
		}
	}
else {
	gameStates.render.history.bmBot = NULL;
	ResetTMU (GL_TEXTURE0);
	}
PROF_END(ptRenderStates)
RETVAL (1)
}

//------------------------------------------------------------------------------

void SetupMonitor (CSegFace *pFace, CBitmap *bmTop, int32_t bTextured, int32_t bLightmaps)
{
ogl.SelectTMU ((bmTop ? GL_TEXTURE1 : GL_TEXTURE0) + bLightmaps, true);
if (bTextured)
	OglTexCoordPointer (2, GL_FLOAT, 0, pFace->pTexCoord - pFace->m_info.nIndex);
else {
	ogl.SetTexturing (false);
	ogl.BindTexture (0);
	}
}

//------------------------------------------------------------------------------

void ResetMonitor (CBitmap *bmTop, int32_t bLightmaps)
{
if (bmTop) {
	ogl.SelectTMU (GL_TEXTURE1 + bLightmaps, true);
	ogl.SetTexturing (true);
	OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<GLvoid*> (FACES.ovlTexCoord.Buffer ()));
	//gameStates.render.history.bmTop = NULL;
	}
else {
	ogl.SelectTMU (GL_TEXTURE0 + bLightmaps, true);
	OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<GLvoid*> (FACES.texCoord.Buffer ()));
	//gameStates.render.history.bmBot = NULL;
	}
}

//------------------------------------------------------------------------------

static inline int32_t FaceIsTransparent (CSegFace *pFace, CBitmap *bmBot, CBitmap *bmTop)
{
return pFace->m_info.nTransparent;
}

//------------------------------------------------------------------------------

static inline int32_t FaceIsColored (CSegFace *pFace)
{
return gameStates.render.bFullBright || pFace->m_info.nColored;
}

//------------------------------------------------------------------------------

static inline void DrawFace (CSegFace* pFace)
{
if (gameStates.render.bTriangleMesh)
	OglDrawArrays (GL_TRIANGLES, pFace->m_info.nIndex, pFace->m_info.nTris * 3);
else
	OglDrawArrays (GL_TRIANGLE_FAN, pFace->m_info.nIndex, 4);
}

//------------------------------------------------------------------------------

int32_t RenderFace (CSegFace *pFace, CBitmap *bmBot, CBitmap *bmTop, int32_t bTextured, int32_t bLightmaps)
{
ENTER (0, 0);
PROF_START
	int32_t bColored, bTransparent, bColorKey = 0, bMonitor = 0;

#if DBG
if (pFace && (pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
if (pFace->m_info.nSegColor)
	BRP;
#endif

if (!pFace->m_info.bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = bmBot->Override (BitmapFrame (bmBot, pFace->m_info.nBaseTex, pFace->m_info.nSegment));
bTransparent = FaceIsTransparent (pFace, bmBot, bmTop);

bColored = FaceIsColored (pFace);
bMonitor = (pFace->m_info.nCamera >= 0);
#if DBG
if (bmTop)
	bmTop = bmTop;
#endif
if (bmTop) {
	if ((bmTop = bmTop->Override (BitmapFrame (bmTop, pFace->m_info.nOvlTex, pFace->m_info.nSegment))) && bmTop->Frames ()) {
		bColorKey = (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bmTop = bmTop->CurFrame ();
		}
	else
		bColorKey = (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0;
	}
gameStates.render.history.nType = (bColorKey ? 3 : (bmTop != NULL) ? 2 : (bmBot != NULL));
if ((bTransparent /*|| (pFace->m_info.nSegColor && gameStates.render.bPerPixelLighting)*/) && (gameStates.render.nType < RENDER_TYPE_SKYBOX) && !bMonitor) {
	pFace->m_info.nRenderType = gameStates.render.history.nType;
	pFace->m_info.bColored = bColored;
#if DBG
if (pFace->m_info.nSegColor)
	BRP;
#endif
	transparencyRenderer.AddFace (pFace);
#if 1 //!DBG
	if (!(pFace->m_info.nSegColor && bmBot))
#endif
		RETVAL (0)
	}

#if 1
SetRenderStates (pFace, bmBot, bmTop, bTextured, bColorKey, bColored, bLightmaps);
#else
SetRenderStates (pFace, bmBot, bmTop, ((gameStates.render.nType == RENDER_TYPE_ZCULL) && !(bColorKey || (bmBot->Flags () & BM_FLAG_SEE_THRU))) ? 0 : bTextured, bColorKey, bColored, bLightmaps);
if (gameStates.render.nType == RENDER_TYPE_ZCULL) {
	DrawFace (pFace);
	RETVAL (1)
	}
#endif

G3SetBlendMode (pFace);
if (bMonitor)
	SetupMonitor (pFace, bmTop, bTextured, bLightmaps);
gameData.renderData.nTotalFaces++;

if (gameOpts->render.debug.bWireFrame & 1)
	glColor3f (1,1,1);
if (!bLightmaps)
	DrawFace (pFace);
else {
	if (!bColored) {
		SetupGrayScaleShader (gameStates.render.history.nType, &pFace->m_info.color);
		DrawFace (pFace);
		}
	else if (gameStates.render.bFullBright) {
		if (gameStates.render.history.nType > 1)
			SetupTexMergeShader (bColorKey, bColored, gameStates.render.history.nType);
		else 
			shaderManager.Deploy (-1);
		float l = 1.0f / float (gameStates.render.bFullBright);
		glColor3f (l,l,l);
		DrawFace (pFace);
		}
	else if (gameStates.render.bPerPixelLighting == 1) {
		SetupLightmapShader (pFace, gameStates.render.history.nType, false);
		DrawFace (pFace);
		}
	else {
		bool bAdditive = false;
		ogl.m_states.iLight = 0;
		for (;;) {
			SetupPerPixelLightingShader (pFace, gameStates.render.history.nType, false);
			DrawFace (pFace);
			if (ogl.m_states.iLight >= ogl.m_states.nLights)
				break;
			if (!bAdditive) {
				bAdditive = true;
				ogl.SetBlendMode (OGL_BLEND_ADD); //_MINUS_SRC_COLOR);
				ogl.SetDepthMode (GL_EQUAL);
				ogl.SetDepthWrite (false);
				}
			}
		if (bAdditive) {
			ogl.SetDepthMode (GL_LEQUAL);
			ogl.SetDepthWrite (true);
			}
		}
	}

if (bMonitor)
	ResetMonitor (bmTop, bLightmaps);
PROF_END(ptRenderFaces)
RETVAL (0)
}

//------------------------------------------------------------------------------

int32_t DrawHeadlights (CSegFace *pFace, CBitmap *bmBot, CBitmap *bmTop, int32_t bTextured, int32_t bLightmaps)
{
ENTER (0, 0);
	int32_t bColorKey = 0, bMonitor = 0;

if (!pFace->m_info.bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = bmBot->Override (-1);
if (FaceIsTransparent (pFace, bmBot, bmTop) && !(bMonitor || bmTop))
	RETVAL (0)
bMonitor = (pFace->m_info.nCamera >= 0);
if (bmTop) {
	if ((bmTop = bmTop->Override (-1)) && bmTop->Frames ()) {
		bColorKey = (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bmTop = bmTop->CurFrame ();
		}
	else
		bColorKey = (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0;
	}
gameStates.render.history.nType = bColorKey ? 3 : (bmTop != NULL) ? 2 : (bmBot != NULL);
SetRenderStates (pFace, bmBot, bmTop, bTextured, bColorKey, 1, bLightmaps);
if (bMonitor)
	SetupMonitor (pFace, bmTop, bTextured, 1);
gameData.renderData.nTotalFaces++;
lightManager.Headlights ().SetupShader (gameStates.render.history.nType, 1, bmBot ? NULL : &pFace->m_info.color);
DrawFace (pFace);
if (bMonitor)
	ResetMonitor (bmTop, 1);
RETVAL (0)
}

//------------------------------------------------------------------------------
//eof
