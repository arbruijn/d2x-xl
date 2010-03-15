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
#include "ogl_tmu.h"
#include "texmerge.h"
#include "transprender.h"
#include "gameseg.h"
#include "automap.h"

#define CHECK_TRANSPARENCY	0

#if DBG
#	define G3_BUFFER_FACES	0	// well ... no speed increase :/
#else
#	define G3_BUFFER_FACES	0
#endif

//tRenderFaceDrawerP faceRenderFunc = RenderFace;

//------------------------------------------------------------------------------

#define FACE_BUFFER_SIZE			1000
#define FACE_BUFFER_INDEX_SIZE	(FACE_BUFFER_SIZE * 4 * 4)

typedef struct tFaceBuffer {
	CBitmap	*bmBot;
	CBitmap	*bmTop;
	short			nFaces;
	short			nElements;
	int			bTextured;
	int			index [FACE_BUFFER_INDEX_SIZE];
} tFaceBuffer;

static tFaceBuffer faceBuffer = {NULL, NULL, 0, 0, 0, {0}};

//------------------------------------------------------------------------------

extern GLhandleARB headlightShaderProgs [2][4];
extern GLhandleARB perPixelLightingShaderProgs [MAX_LIGHTS_PER_PIXEL][4];
extern GLhandleARB gsShaderProg [2][3];

//------------------------------------------------------------------------------
// this is a work around for OpenGL's per vertex light interpolation
// rendering a quad is always started with the brightest vertex

void G3BuildQuadIndex (CSegFace *faceP, int *indexP)
{
#if G3_BUFFER_FACES
int nIndex = faceP->m_info.nIndex;
//for (int i = 0; i < 4; i++)
*indexP++ = nIndex++;
*indexP++ = nIndex++;
*indexP++ = nIndex++;
*indexP = nIndex;
#else
	tRgbaColorf	*pc = FACES.color + faceP->m_info.nIndex;
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
nIndex = faceP->m_info.nIndex;
if (!iMax) {
	for (i = 0; i < 4; i++)
		*indexP++ = nIndex++;
	}
else {
	for (i = 0, j = iMax; i < 4; i++, j %= 4)
		*indexP++ = nIndex + j++;
	}
#endif
}

//------------------------------------------------------------------------------

void FlushFaceBuffer (int bForce)
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
		PrintLog ("error calling glDrawElements (%d, %d) in FlushFaceBuffer\n", gameStates.render.bTriangleMesh ? "GL_TRIANGLES" : "GL_TRIANGLE_FAN");
		}
	faceBuffer.nFaces =
	faceBuffer.nElements = 0;
	}
#endif
}

//------------------------------------------------------------------------------

void FillFaceBuffer (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bTextured)
{
#if DBG
if (!gameOpts->render.debug.bTextures)
	return;
#endif
#if 0
if (!gameStates.render.bTriangleMesh) {
	if (gameStates.render.bFullBright)
		glColor3f (1,1,1);
	OglDrawArrays (GL_TRIANGLE_FAN, faceP->m_info.nIndex, 4);
	}
else
#endif
	{
	int	i = faceP->m_info.nIndex,
			j = gameStates.render.bTriangleMesh ? faceP->m_info.nTris * 3 : 4;

#if 0 //DBG
		if (i == nDbgVertex)
			nDbgVertex = nDbgVertex;
		if (i + j > int (FACES.vertices.Length ())) {
			PrintLog ("invalid vertex index %d in FillFaceBuffer\n");
			return;
			}
#endif
	if (/*!gameStates.render.bTriangleMesh ||*/ (faceBuffer.bmBot != bmBot) || (faceBuffer.bmTop != bmTop)) {
		if (faceBuffer.nFaces)
			FlushFaceBuffer (1);
		faceBuffer.bmBot = bmBot;
		faceBuffer.bmTop = bmTop;
		}
	else if (faceBuffer.nElements + j > FACE_BUFFER_INDEX_SIZE)
		FlushFaceBuffer (1);
	faceBuffer.bTextured = bTextured;
	for (; j; j--)
		faceBuffer.index [faceBuffer.nElements++] = i++;
	faceBuffer.nFaces++;
	}
}

//------------------------------------------------------------------------------

int SetupRenderShader (CSegFace *faceP, int bColorKey, int bMultiTexture, int bTextured, int bColored, tRgbaColorf *colorP)
{
	int	nType, nShader = -1;

if (!ogl.m_states.bShadersOk || (gameStates.render.nType == RENDER_TYPE_SKYBOX))
	return -1;
#if DBG
if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
nType = bColorKey ? 3 : bMultiTexture ? 2 : bTextured;
if ((gameStates.render.nType >= RENDER_TYPE_GEOMETRY) && !bColored && gameOpts->render.automap.bGrayOut)
	nShader = SetupGrayScaleShader (nType, colorP);
else if (bColorKey || bMultiTexture || (bColored > 1))
	nShader = SetupTexMergeShader (bColored > 1, nType);
else
	shaderManager.Deploy (-1);
ogl.ClearError (0);
return nShader;
}

//------------------------------------------------------------------------------

#if DBG

void RenderWireFrame (CSegFace *faceP, int bTextured)
{
if (gameOpts->render.debug.bWireFrame) {
	if ((nDbgFace < 0) || (faceP - FACES.faces == nDbgFace)) {
		tFaceTriangle	*triP = FACES.tris + faceP->m_info.nTriIndex;
		ogl.DisableClientState (GL_COLOR_ARRAY);
		if (bTextured)
			ogl.SetTexturing (false);
		glColor3f (1.0f, 0.5f, 0.0f);
		glLineWidth (6);
		if (ogl.SizeVertexBuffer (4)) {
			for (int i = 0; i < 4; i++)
				ogl.VertexBuffer () [i] = gameData.segs.fVertices [faceP->m_info.index [i]];
			ogl.FlushBuffers (GL_LINE_LOOP, 4);
			}	
		if (gameStates.render.bTriangleMesh) {
			glLineWidth (2);
			glColor3f (1,1,1);
			for (int i = 0; i < faceP->m_info.nTris; i++, triP++)
				OglDrawArrays (GL_LINE_LOOP, triP->nIndex, 3);
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

static inline void SetFaceBlendMode (CSegFace *faceP)
{
if (faceP->m_info.bAdditive)
	ogl.SetBlendMode (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
else if (faceP->m_info.bTransparent)
	ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
else
	ogl.SetBlendMode (GL_ONE, GL_ZERO);
}

//------------------------------------------------------------------------------

static inline CBitmap* SetupTMU (CBitmap* bmP, int nTMU, int nMode)
{
ogl.SelectTMU (nTMU, true);
ogl.SetTexturing (true);
glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, nMode);
if (bmP->Bind (1)) 
	return NULL; 
bmP = bmP->CurFrame (-1); 
bmP->Texture ()->Wrap (GL_REPEAT); 
return bmP;
}

//------------------------------------------------------------------------------

void inline ResetTMU (int nTMU)
{
ogl.SelectTMU (nTMU, true);
ogl.BindTexture (0);
ogl.SetTexturing (false);
}

//------------------------------------------------------------------------------

int SetRenderStates (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bTextured, int bColorKey, int bColored, bool bForce = false)
{
PROF_START
ogl.ClearError (0);
if (bTextured) {
	bool bStateChange = false;
	if (bForce || (bmBot != gameStates.render.history.bmBot)) {
		bStateChange = true;
		if (!(gameStates.render.history.bmBot = SetupTMU (bmBot, GL_TEXTURE0, GL_MODULATE)))
			return 0;
		}
	if (bForce || (bmTop != gameStates.render.history.bmTop)) {
		bStateChange = true;
		if (bmTop) {
			if (!(gameStates.render.history.bmTop = SetupTMU (bmTop, GL_TEXTURE1, GL_DECAL)))
				return 0;
			}
		else {
			ResetTMU (GL_TEXTURE1);
			}
		}
	CBitmap* bmMask = (bColorKey && bmTop) ? bmTop->Mask () : NULL;
	if (bForce || (bmMask != gameStates.render.history.bmMask)) {
		bStateChange = true;
		if (bmMask) {
			if (!(gameStates.render.history.bmMask = SetupTMU (bmMask, GL_TEXTURE2, GL_MODULATE)))
				return 0;
			}
		else {
			ResetTMU (GL_TEXTURE2);
			bColorKey = 0;
			}
		}
	if (bColored != gameStates.render.history.bColored) {
		bStateChange = true;
		gameStates.render.history.bColored = bColored;
		if (!gameStates.render.bFullBright) {
			if (bColored)
				ogl.SetBlendMode (GL_DST_COLOR, GL_ZERO);
			else
				ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}	
		}
	if (bStateChange) {
		gameData.render.nStateChanges++;
		if (gameStates.render.bFullBright < 1)
			SetupRenderShader (faceP, bColorKey, bmTop != NULL, bmBot != NULL, bColored, bmBot ? NULL : &faceP->m_info.color);
		}
	}
else {
	gameStates.render.history.bmBot = NULL;
	ResetTMU (GL_TEXTURE0);
	}
PROF_END(ptRenderStates)
return 1;
}

//------------------------------------------------------------------------------

int SetLightingRenderStates (CSegFace *faceP, CBitmap *bmTop, int bColorKey)
{
PROF_START
bool bStateChange = false;
CBitmap *mask = bColorKey ? bmTop->Mask () : NULL;
if (mask != gameStates.render.history.bmMask) {
	bStateChange = true;
	gameStates.render.history.bmMask = mask;
	if (mask) {
		SetupTMU (mask, GL_TEXTURE1, GL_MODULATE);
		//ogl.EnableClientStates (1, 0, 0, -1);
		}
	else {
		ResetTMU (GL_TEXTURE1);
		//ogl.DisableClientStates (1, 0, 0, -1);
		}
	}
if (bStateChange)
	gameData.render.nStateChanges++;
PROF_END(ptRenderStates)
return 1;
}

//------------------------------------------------------------------------------

void SetupMonitor (CSegFace *faceP, CBitmap *bmTop, int bTextured, int bLightmaps)
{
ogl.SelectTMU ((bmTop ? GL_TEXTURE1 : GL_TEXTURE0) + bLightmaps, true);
if (bTextured)
	OglTexCoordPointer (2, GL_FLOAT, 0, faceP->texCoordP - faceP->m_info.nIndex);
else {
	ogl.SetTexturing (false);
	ogl.BindTexture (0);
	}
}

//------------------------------------------------------------------------------

void ResetMonitor (CBitmap *bmTop, int bLightmaps)
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

#if CHECK_TRANSPARENCY

static inline int FaceIsTransparent (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop)
{
if (faceP->m_info.nTransparent >= 0)
	return faceP->m_info.nTransparent;
if (!bmBot)
	return faceP->m_info.nTransparent = (faceP->m_info.color.alpha < 1.0f);
if (faceP->m_info.bTransparent || faceP->m_info.bAdditive)
	return faceP->m_info.nTransparent = 1;
if (bmBot->Flags () & BM_FLAG_SEE_THRU)
	return faceP->m_info.nTransparent = 0;
if (!(bmBot->Flags () & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT)))
	return faceP->m_info.nTransparent = 0;
if (!bmTop)
	return faceP->m_info.nTransparent = 1;
if (bmTop->Flags () & BM_FLAG_SEE_THRU)
	return faceP->m_info.nTransparent = 0;
if (bmTop->Flags () & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
	return faceP->m_info.nTransparent = 1;
return faceP->m_info.nTransparent = 0;
}

#endif

//------------------------------------------------------------------------------

static inline int FaceIsColored (CSegFace *faceP)
{
#if CHECK_TRANSPARENCY
return (faceP->m_info.nColored >= 0)
		 ? faceP->m_info.nColored
		 : faceP->m_info.nColored = !automap.Display () || automap.m_visited [0][faceP->m_info.nSegment] || !gameOpts->render.automap.bGrayOut;
#else
return gameStates.render.bFullBright || faceP->m_info.nColored;
#endif
}

//------------------------------------------------------------------------------

#if GEOMETRY_VBOS

inline void DrawFace (CSegFace *faceP)
{
if (FACES.vboDataHandle) {
	int i = faceP->m_info.nIndex;
	glDrawRangeElements (GL_TRIANGLES, i, i + 5, 6, GL_UNSIGNED_SHORT, G3_BUFFER_OFFSET (i * sizeof (ushort)));
	}
else
	OglDrawArrays (GL_TRIANGLES, faceP->m_info.nIndex, 6);
}

#else

#define DrawFace(_faceP)	OglDrawArrays (GL_TRIANGLES, (_faceP)->m_info.nIndex, 6)

#endif

//------------------------------------------------------------------------------
// render to depth buffer (only opaque geometry)

int RenderDepth (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop)
{
	int bColorKey = 0;

#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

#if CHECK_TRANSPARENCY
if (FaceIsTransparent (faceP, bmBot, bmTop) != gameStates.render.bRenderTransparency)
	return 0;
#endif
if (!faceP->m_info.bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = bmBot->Override (-1);
if (bmTop) {
	if ((bmTop = bmTop->Override (-1)) && bmTop->Frames ()) {
		bColorKey = (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bmTop = bmTop->CurFrame ();
		}
	else
		bColorKey = (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0;
	}
gameStates.render.history.nType = bColorKey ? 3 : (bmTop != NULL) ? 2 : (bmBot != NULL);
SetRenderStates (faceP, bmBot, bmTop, bmBot != NULL, bColorKey, faceP->m_info.nColored);
if (gameStates.render.bTriangleMesh)
	DrawFace (faceP);
else
	OglDrawArrays (GL_TRIANGLE_FAN, faceP->m_info.nIndex, 4);
return 0;
}

//------------------------------------------------------------------------------
// render vertex color

int RenderColor (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop)
{
#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (!FaceIsColored (faceP))
	return 0;
#if CHECK_TRANSPARENCY
if (FaceIsTransparent (faceP, bmBot, bmTop) != gameStates.render.bRenderTransparency)
	return 0;
#endif
if (gameStates.render.bTriangleMesh)
	DrawFace (faceP);
else
	OglDrawArrays (GL_TRIANGLE_FAN, faceP->m_info.nIndex, 4);
//DrawFace (faceP);
return 0;
}

//------------------------------------------------------------------------------
// render lightmaps (including vertex color)

int RenderLightmaps (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop)
{
#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (!FaceIsColored (faceP))
	return 0;
#if CHECK_TRANSPARENCY
if (FaceIsTransparent (faceP, bmBot, bmTop) != gameStates.render.bRenderTransparency)
	return 0;
#endif
if (SetupLightmap (faceP))
	DrawFace (faceP);
return 0;
}

//------------------------------------------------------------------------------
// render per pixel light

int RenderLights (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop)
{
#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (!FaceIsColored (faceP))
	return 0;
#if CHECK_TRANSPARENCY
if (FaceIsTransparent (faceP, bmBot, bmTop) != gameStates.render.bRenderTransparency)
	return 0;
#endif
if (!SetupLightmap (faceP))
	return 1;
ogl.m_states.iLight = 0;
//ogl.SetBlendMode (GL_ONE, GL_ZERO);
while (0 < SetupPerPixelLightingShader (faceP)) {
	DrawFace (faceP);
	if (ogl.m_states.iLight >= ogl.m_states.nLights)
		break;
//	ogl.SetBlendMode (GL_ONE, GL_ONE);
	}
return 0;
}

//------------------------------------------------------------------------------
// render per pixel headlight

int RenderHeadlights (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop)
{
#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (!FaceIsColored (faceP))
	return 0;
#if CHECK_TRANSPARENCY
if (FaceIsTransparent (faceP, bmBot, bmTop) != gameStates.render.bRenderTransparency)
	return 0;
#endif
OglDrawArrays (GL_TRIANGLES, faceP->m_info.nIndex, 6);
return 0;
}

//------------------------------------------------------------------------------

int RenderSky (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop)
{
PROF_START
#if DBG
if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

SetRenderStates (faceP, bmBot, bmTop, 1, 0, 1);
gameData.render.nTotalFaces++;
#if DBG
RenderWireFrame (faceP, 1);
if (!gameOpts->render.debug.bTextures)
	return 0;
#endif

if (gameStates.render.bTriangleMesh)
	DrawFace (faceP);
else
	OglDrawArrays (GL_TRIANGLE_FAN, faceP->m_info.nIndex, 4);
PROF_END(ptRenderFaces)
return 0;
}

//------------------------------------------------------------------------------
// render geometry (all multi texturing is done via shaders, 
// also handles color keyed transparency via pre-computed masks)

int RenderFace (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bBlend, int bTextured)
{
PROF_START
	int			bColored, bColorKey = 0, bMonitor = 0;
#if G3_BUFFER_FACES
	int			nBlendMode;
#endif

#if DBG
if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (!faceP->m_info.bTextured)
	bmBot = NULL;
else if (bmBot) {
	bmBot = bmBot->Override (-1);
#if 0 //DBG
	if (strstr (bmBot->Name (), "rock327"))
		bmBot = bmBot;
	else
		return 0;
#endif
	}
bMonitor = (faceP->m_info.nCamera >= 0);
bColored = FaceIsColored (faceP);
if (bmTop) {
	if ((bmTop = bmTop->Override (-1)) && bmTop->Frames ()) {
		bColorKey = (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bmTop = bmTop->CurFrame ();
		}
	else
		bColorKey = (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0;
	}
gameStates.render.history.nType = bColorKey ? 3 : (bmTop != NULL) ? 2 : (bmBot != NULL);
if (faceP->m_info.nTransparent && !bMonitor && (gameStates.render.nType < RENDER_TYPE_SKYBOX)) {
	faceP->m_info.nRenderType = gameStates.render.history.nType;
	faceP->m_info.bColored = bColored;
	transparencyRenderer.AddFace (faceP);
	return 0;
	}

#if G3_BUFFER_FACES
if (bMonitor)
	FlushFaceBuffer (1);
else {
	nBlendMode = faceP->m_info.bAdditive ? 2 : faceP->m_info.bTransparent ? 1 : 0;
	if (nBlendMode != gameStates.render.history.nBlendMode) {
		gameStates.render.history.nBlendMode = nBlendMode;
		FlushFaceBuffer (1);
		SetFaceBlendMode (faceP);
		}
	FillFaceBuffer (faceP, bmBot, bmTop, bTextured);
	}
if (faceBuffer.nFaces <= 1)
#endif

SetRenderStates (faceP, bmBot, bmTop, bTextured, bColorKey, bColored);
gameData.render.nTotalFaces++;
#if DBG
RenderWireFrame (faceP, bTextured);
if (!gameOpts->render.debug.bTextures)
	return 0;
#endif

if (bMonitor)
	SetupMonitor (faceP, bmTop, bTextured, 0);
#if G3_BUFFER_FACES
else
	return 0;
#endif
if (gameStates.render.bTriangleMesh)
	DrawFace (faceP);
else
	OglDrawArrays (GL_TRIANGLE_FAN, faceP->m_info.nIndex, 4);
if (bMonitor)
	ResetMonitor (bmTop, 0);
PROF_END(ptRenderFaces)
return 0;
}

//------------------------------------------------------------------------------
//eof
