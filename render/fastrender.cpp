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
#include "ogl_shader.h"

//------------------------------------------------------------------------------

void ResetFaceList (void)
{
PROF_START
int* tails = gameData.render.faceIndex.tails.Buffer (),
	* usedKeys = gameData.render.faceIndex.usedKeys.Buffer ();
for (int i = 0; i < gameData.render.faceIndex.nUsedKeys; i++) 
	tails [usedKeys [i]] = -1;
gameData.render.faceIndex.nUsedKeys = 0;
PROF_END(ptFaceList)
}

//------------------------------------------------------------------------------
// AddFaceListItem creates lists of faces with the same texture
// The lists themselves are kept in gameData.render.faceList
// gameData.render.faceIndex holds the root index of each texture's face list

int AddFaceListItem (CSegFace *faceP, int nThread)
{
if (!(faceP->m_info.widFlags & WID_RENDER_FLAG))
	return 0;
#if 1
if (faceP->m_info.nFrame == gameData.app.nMineRenderCount)
	return 0;
#endif
#if DBG
if (faceP - FACES.faces >= gameData.segs.nFaces)
	return 0;
#endif

#ifdef _OPENMP
#	pragma omp critical
#endif
{
PROF_START
int nKey = faceP->m_info.nKey;
int i = gameData.render.nUsedFaces++;
int j = gameData.render.faceIndex.tails [nKey];
if (j < 0) {
	gameData.render.faceIndex.usedKeys [gameData.render.faceIndex.nUsedKeys++] = nKey;
	gameData.render.faceIndex.roots [nKey] = i;
	}
else {
#if DBG
	if (!i)
		i = i;
#endif
	gameData.render.faceList [j].nNextItem = i;
	}
gameData.render.faceList [i].nNextItem = -1;
gameData.render.faceList [i].faceP = faceP;
gameData.render.faceIndex.tails [nKey] = i;
faceP->m_info.nTransparent = -1;
faceP->m_info.nColored = -1;
faceP->m_info.nFrame = gameData.app.nFrameCount;
PROF_END(ptFaceList)
}
return 1;
}

//------------------------------------------------------------------------------

void LoadFaceBitmaps (CSegment *segP, CSegFace *faceP)
{
	CSide	*sideP = segP->m_sides + faceP->m_info.nSide;
	short	nFrame = sideP->m_nFrame;

if (faceP->m_info.nCamera >= 0) {
	if (SetupMonitorFace (faceP->m_info.nSegment, faceP->m_info.nSide, faceP->m_info.nCamera, faceP)) 
		return;
	faceP->m_info.nCamera = -1;
	}
#if DBG
if (FACE_IDX (faceP) == nDbgFace)
	nDbgFace = nDbgFace;
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if ((faceP->m_info.nBaseTex < 0) || !faceP->m_info.bTextured)
	return;
if (faceP->m_info.bOverlay)
	faceP->m_info.nBaseTex = sideP->m_nOvlTex;
else {
	faceP->m_info.nBaseTex = sideP->m_nBaseTex;
	if (!faceP->m_info.bSplit)
		faceP->m_info.nOvlTex = sideP->m_nOvlTex;
	}
if (gameOpts->ogl.bGlTexMerge) {
	faceP->bmBot = LoadFaceBitmap (faceP->m_info.nBaseTex, nFrame);
	if (nFrame)
		nFrame = nFrame;
	if (faceP->m_info.nOvlTex)
		faceP->bmTop = LoadFaceBitmap ((short) (faceP->m_info.nOvlTex), nFrame);
	else
		faceP->bmTop = NULL;
	}
else {
	if (faceP->m_info.nOvlTex != 0) {
		faceP->bmBot = TexMergeGetCachedBitmap (faceP->m_info.nBaseTex, faceP->m_info.nOvlTex, faceP->m_info.nOvlOrient);
		if (faceP->bmBot)
			faceP->bmBot->SetupTexture (1, 1);
#if DBG
		else
			faceP->bmBot = TexMergeGetCachedBitmap (faceP->m_info.nBaseTex, faceP->m_info.nOvlTex, faceP->m_info.nOvlOrient);
#endif
		faceP->bmTop = NULL;
		}
	else {
		faceP->bmBot = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [faceP->m_info.nBaseTex].index;
		LoadBitmap (gameData.pig.tex.bmIndexP [faceP->m_info.nBaseTex].index, gameStates.app.bD1Mission);
		}
	}
}

//------------------------------------------------------------------------------

bool RenderIllumination (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
if (!(faceP->m_info.widFlags & WID_RENDER_FLAG))
	return false;
if (IS_WALL (faceP->m_info.nWall))
	return false;
LoadFaceBitmaps (segP, faceP);
if (!faceP->bmBot)
	return false;
RenderLighting (faceP, faceP->bmBot, faceP->bmTop);
return true;
}

//------------------------------------------------------------------------------

bool RenderSolidFace (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
if (!(faceP->m_info.widFlags & WID_RENDER_FLAG))
	return false;
if (IS_WALL (faceP->m_info.nWall))
	return false;
LoadFaceBitmaps (segP, faceP);
if (!faceP->bmBot)
	return false;
faceRenderFunc (faceP, faceP->bmBot, faceP->bmTop, (faceP->m_info.nCamera < 0) || faceP->m_info.bTeleport, !bDepthOnly && faceP->m_info.bTextured, bDepthOnly);
return true;
}

//------------------------------------------------------------------------------

bool RenderWallFace (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
if (!(faceP->m_info.widFlags & WID_RENDER_FLAG))
	return false;
if (!IS_WALL (faceP->m_info.nWall))
	return false;
LoadFaceBitmaps (segP, faceP);
faceRenderFunc (faceP, faceP->bmBot, faceP->bmTop, (faceP->m_info.nCamera < 0) || faceP->m_info.bTeleport, !bDepthOnly && faceP->m_info.bTextured, bDepthOnly);
return true;
}

//------------------------------------------------------------------------------

bool RenderColoredFace (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
if (!(faceP->m_info.widFlags & WID_RENDER_FLAG))
	return false;
if (faceP->bmBot)
	return false;
short nSegment = faceP->m_info.nSegment;
short special = SEGMENTS [nSegment].m_nType;
if ((special < SEGMENT_IS_WATER) || (special > SEGMENT_IS_TEAM_RED) || 
	 (SEGMENTS [nSegment].m_group < 0) || (SEGMENTS [nSegment].m_owner < 1))
	return false;
if (!gameData.app.nFrameCount)
	gameData.render.nColoredFaces++;
faceRenderFunc (faceP, faceP->bmBot, faceP->bmTop, (faceP->m_info.nCamera < 0) || faceP->m_info.bTeleport, !bDepthOnly && faceP->m_info.bTextured, bDepthOnly);
return true;
}

//------------------------------------------------------------------------------

bool RenderCoronaFace (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
if (!(faceP->m_info.widFlags & WID_RENDER_FLAG))
	return false;
if (!faceP->m_info.nCorona)
	return false;
#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
glareRenderer.Render (faceP->m_info.nSegment, faceP->m_info.nSide, glareRenderer.Visibility (faceP->m_info.nCorona), faceP->m_info.fRads [0]);
return true;
}

//------------------------------------------------------------------------------

bool RenderSkyBoxFace (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
LoadFaceBitmaps (segP, faceP);
RenderFaceVL (faceP, faceP->bmBot, faceP->bmTop, 1, 1, 0);
return true;
}

//------------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef bool (__fastcall * pRenderHandler) (CSegment *segP, CSegFace *faceP, int bDepthOnly);
#else
typedef bool (* pRenderHandler) (CSegment *segP, CSegFace *faceP, int bDepthOnly);
#endif

static pRenderHandler renderHandlers [] = {RenderIllumination, RenderSolidFace, RenderWallFace, RenderColoredFace, RenderCoronaFace, RenderSkyBoxFace};


static inline bool RenderMineFace (CSegment *segP, CSegFace *faceP, int nType, int bDepthOnly)
{
if (!faceP->m_info.bVisible)
	return false;
#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
return renderHandlers [nType] (segP, faceP, bDepthOnly);
}

//------------------------------------------------------------------------------

typedef struct tFaceRef {
	short		nSegment;
	CSegFace	*faceP;
	} tFaceRef;

static tFaceRef faceRef [2][MAX_SEGMENTS_D2X * 6];

int QCmpFaces (CSegFace *fp, CSegFace *mp)
{
if (!fp->m_info.bOverlay && mp->m_info.bOverlay)
	return -1;
if (fp->m_info.bOverlay && !mp->m_info.bOverlay)
	return 1;
if (!fp->m_info.bTextured && mp->m_info.bTextured)
	return -1;
if (fp->m_info.bTextured && !mp->m_info.bTextured)
	return 1;
if (!fp->m_info.nOvlTex && mp->m_info.nOvlTex)
	return -1;
if (fp->m_info.nOvlTex && !mp->m_info.nOvlTex)
	return 1;
if (fp->m_info.nBaseTex < mp->m_info.nBaseTex)
	return -1;
if (fp->m_info.nBaseTex > mp->m_info.nBaseTex)
	return -1;
if (fp->m_info.nOvlTex < mp->m_info.nOvlTex)
	return -1;
if (fp->m_info.nOvlTex > mp->m_info.nOvlTex)
	return -1;
return 0;
}

//------------------------------------------------------------------------------

void QSortFaces (int left, int right)
{
	int		l = left,
				r = right;
	tFaceRef	*pf = faceRef [0];
	CSegFace	m = *pf [(l + r) / 2].faceP;

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

#if 1

#define SortFaces()	0

#else

int SortFaces (void)
{
	tSegFaces	*segFaceP;
	CSegFace		*faceP;
	tFaceRef		*ph, *pi, *pj;
	int			h, i, j;
	short			nSegment;

for (h = i = 0, ph = faceRef [0]; i < gameData.render.mine.nRenderSegs; i++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [0][i]))
		continue;
	segFaceP = SEGFACES + nSegment;
	for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++, h++, ph++) {
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

#endif

//------------------------------------------------------------------------------

int BeginRenderFaces (int nType, int bDepthOnly)
{
	int	//bVBO = 0,
			bLightmaps = (nType < RENDER_SKYBOX) && !gameStates.render.bFullBright && lightmapManager.HaveLightmaps (),
			bNormals = !bDepthOnly; 

gameData.threads.vertColor.data.bDarkness = 0;
gameStates.render.nType = nType;
gameStates.render.history.bOverlay = -1;
gameStates.render.history.bColored = 1;
gameStates.render.history.nBlendMode = -1;
gameStates.render.history.bmBot = 
gameStates.render.history.bmTop =
gameStates.render.history.bmMask = NULL;
gameStates.render.bQueryCoronas = 0;
ogl.ResetClientStates ();
shaderManager.Deploy (-1);
ogl.SetFaceCulling (true);
CTexture::Wrap (GL_REPEAT);
if (!bDepthOnly) {
	if (!gameStates.render.bPerPixelLighting || gameStates.render.bFullBright || (nType >= RENDER_OBJECTS)) 
		ogl.SetDepthMode (GL_LEQUAL); 
	else {
		if (nType == RENDER_LIGHTING) {
			ogl.SetDepthMode (GL_LEQUAL); 
			ogl.SetBlendMode (GL_ONE, GL_ONE);
			gameStates.render.bFullBright = 0;
			}
		else {
			ogl.SetBlendMode (GL_DST_COLOR, GL_ZERO);
			ogl.SetDepthMode (GL_EQUAL); 
			ogl.SetDepthWrite (false);
			gameStates.render.bFullBright = 1;
			}
		}
	}
else {
	ogl.ColorMask (0,0,0,0,0);
	ogl.SetDepthWrite (true);
	ogl.SetDepthMode (GL_LESS);
	}
if (nType == RENDER_CORONAS) {
	if (glareRenderer.Style () == 2)
		glareRenderer.LoadShader (10);
	return 0;
	}
else {
#if GEOMETRY_VBOS
	if (FACES.vboDataHandle) {
		glBindBufferARB (GL_ARRAY_BUFFER_ARB, FACES.vboDataHandle);
		bVBO = 1;
		}
#endif
	if ((nType < RENDER_SKYBOX) && (gameStates.render.bPerPixelLighting == 2)) {
		ogl.EnableLighting (1);
		for (int i = 0; i < 8; i++)
			glEnable (GL_LIGHT0 + i);
		ogl.SetLighting (false);
		glColor4f (1,1,1,1);
		}
	}
ogl.SetupTransform (1);
ogl.EnableClientStates (!bDepthOnly, !(bDepthOnly || gameStates.render.bFullBright), bNormals, GL_TEXTURE0);
#if GEOMETRY_VBOS
if (bVBO) {
	if (bNormals)
		OglNormalPointer (GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iNormals));
	if (!bDepthOnly) {
		if (bLightmaps)
			OglTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iLMapTexCoord));
		else
			OglTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iTexCoord));
		OglColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iColor));
		}
	OglVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iVertices));
	if (bLightmaps) {
		ogl.EnableClientStates (1, 1, bNormals, GL_TEXTURE1);
		OglTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iTexCoord));
		OglColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iColor));
		OglVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iVertices));
		}
	ogl.EnableClientStates (1, 1, 0, GL_TEXTURE1 + bLightmaps);
	OglTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iOvlTexCoord));
	OglColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iColor));
	OglVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iVertices));
	ogl.EnableClientStates (1, 1, 0, GL_TEXTURE2 + bLightmaps);
	OglTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iOvlTexCoord));
	OglColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iColor));
	OglVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iVertices));
	if (FACES.vboIndexHandle)
		glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, FACES.vboIndexHandle);
	}	
else 
#endif
	{
	if (bNormals)
		OglNormalPointer (GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.normals.Buffer ()));
	if (!bDepthOnly) {
		if (bLightmaps)
			OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.lMapTexCoord.Buffer ()));
		else
			OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.texCoord.Buffer ()));
		if (!gameStates.render.bFullBright)
			OglColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
		}
	OglVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.vertices.Buffer ()));
	if (bLightmaps) {
		ogl.EnableClientStates (1, !gameStates.render.bFullBright, bNormals, GL_TEXTURE1);
		OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.texCoord.Buffer ()));
		if (!gameStates.render.bFullBright)
			OglColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
		OglVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.vertices.Buffer ()));
		}
	ogl.EnableClientStates (1, !gameStates.render.bFullBright, bNormals, GL_TEXTURE1 + bLightmaps);
	OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.ovlTexCoord.Buffer ()));
	if (!gameStates.render.bFullBright)
		OglColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
	OglVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid*> (FACES.vertices.Buffer ()));
	if (nType < RENDER_CORONAS) {
		ogl.EnableClientStates (1, !gameStates.render.bFullBright, 0, GL_TEXTURE2 + bLightmaps);
		OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.ovlTexCoord.Buffer ()));
		if (!gameStates.render.bFullBright)
			OglColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
		OglVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.vertices.Buffer ()));
		}
	}
if (bNormals)	// somehow these get disabled above
	ogl.EnableClientState (GL_NORMAL_ARRAY, GL_TEXTURE0);
if (gameStates.render.bFullBright)
	glColor3f (1,1,1);
ogl.SetBlendMode (GL_ONE, GL_ZERO);
ogl.ClearError (0);
return 1;
}

//------------------------------------------------------------------------------

void EndRenderFaces (int bDepthOnly)
{
#if 1
FlushFaceBuffer (1);
#endif
ogl.ResetClientStates ();
shaderManager.Deploy (-1);
if (gameStates.render.nType != RENDER_CORONAS) {
	if (gameStates.render.bPerPixelLighting == 2)
		ogl.DisableLighting ();
	ogl.ResetTransform (1);
	if (FACES.vboDataHandle)
		glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
	}
else if (glareRenderer.Style () == 2)
	glareRenderer.UnloadShader ();
else if (ogl.m_states.bOcclusionQuery && gameData.render.lights.nCoronas && !gameStates.render.bQueryCoronas && (glareRenderer.Style () == 1))
	glDeleteQueries (gameData.render.lights.nCoronas, gameData.render.lights.coronaQueries.Buffer ());
ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ogl.SetDepthWrite (true);
ogl.SetDepthTest (true);
ogl.SetDepthMode (GL_LEQUAL);
ogl.ClearError (0);
}

//------------------------------------------------------------------------------

void RenderSkyBoxFaces (void)
{
	tSegFaces*	segFaceP;
	CSegFace*	faceP;
	short*		segP;
	int			i, j, nSegment, bFullBright = gameStates.render.bFullBright;

if (gameStates.render.bHaveSkyBox) {
	ogl.SetDepthWrite (true);
	gameStates.render.bFullBright = 1;
	BeginRenderFaces (RENDER_SKYBOX, 0);
	for (i = gameData.segs.skybox.ToS (), segP = gameData.segs.skybox.Buffer (); i; i--, segP++) {
		nSegment = *segP;
		segFaceP = SEGFACES + nSegment;
		for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++) {
			if (!(faceP->m_info.bVisible = FaceIsVisible (nSegment, faceP->m_info.nSide)))
				continue;
			RenderMineFace (SEGMENTS + nSegment, faceP, 4, 0);
			}
		}
	gameStates.render.bFullBright = bFullBright;
	EndRenderFaces (0);
	}
}

//------------------------------------------------------------------------------

static inline int VisitSegment (short nSegment, int bAutomap)
{
if (nSegment < 0)
	return 0;
if (bAutomap) {
	if (automap.Display ()) {
		if (!(automap.m_bFull || automap.m_visible [nSegment]))
			return 0;
		if (!gameOpts->render.automap.bSkybox && (SEGMENTS [nSegment].m_nType == SEGMENT_IS_SKYBOX))
			return 0;
		}
	else
		automap.m_visited [0][nSegment] = gameData.render.mine.bSetAutomapVisited;
	}
if (VISITED (nSegment))
	return 0;
VISIT (nSegment);
return 1;
}

//------------------------------------------------------------------------------

short RenderFaceList (CFaceListIndex& flx, int nType, int bDepthOnly, int bHeadlight)
{
	tFaceListItem*	fliP;
	CSegFace*		faceP;
	int				i, j, nFaces = 0, nSegment = -1;
	int				bAutomap = (nType <= RENDER_FACES);

#if 1
if (automap.Display () && !bDepthOnly)
	flx.usedKeys.SortAscending (0, flx.nUsedKeys - 1);
#endif
for (i = 0; i < flx.nUsedKeys; i++) {
	for (j = flx.roots [flx.usedKeys [i]]; j >= 0; j = fliP->nNextItem) {
		fliP = &gameData.render.faceList [j];
		faceP = fliP->faceP;
		if (nSegment != faceP->m_info.nSegment) {
			nSegment = faceP->m_info.nSegment;
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#endif
			if (!bDepthOnly) {
				if (!bHeadlight)
					VisitSegment (nSegment, bAutomap);
				if (gameStates.render.bPerPixelLighting == 2)
					lightManager.Index (0)[0].nActive = -1;
				}
			}
		if (RenderMineFace (SEGMENTS + nSegment, faceP, nType, bDepthOnly))
			nFaces++;
		}
	}
return nFaces;
}

//------------------------------------------------------------------------------

void RenderCoronaFaceList (CFaceListIndex& flx, int nPass)
{
	tFaceListItem*	fliP;
	CSegFace*		faceP;
	int				i, j, nSegment;

for (i = 0; i < flx.nUsedKeys; i++) {
	for (j = flx.roots [flx.usedKeys [i]]; j >= 0; j = fliP->nNextItem) {
		fliP = &gameData.render.faceList [j];
		faceP = fliP->faceP;
		if (!faceP->m_info.nCorona)
			continue;
		nSegment = faceP->m_info.nSegment;
		if (automap.Display ()) {
			if (!(automap.m_bFull || automap.m_visible [nSegment]))
				return;
			if (!gameOpts->render.automap.bSkybox && (SEGMENTS [nSegment].m_nType == SEGMENT_IS_SKYBOX))
				continue;
			}
		if (nPass == 1) {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#	if 0
			else if (nDbgSeg >= 0)
				continue;
#	endif
#endif
			glareRenderer.Visibility (faceP->m_info.nCorona);
			}
		else if (nPass == 2) {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#	if 0
			else if (nDbgSeg >= 0)
				continue;
#	endif
#endif
			glBeginQuery (GL_SAMPLES_PASSED_ARB, gameData.render.lights.coronaQueries [faceP->m_info.nCorona - 1]);
			if (!glGetError ())
				glareRenderer.Render (nSegment, faceP->m_info.nSide, 1, faceP->m_info.fRads [0]);
			glEndQuery (GL_SAMPLES_PASSED_ARB);
			}
		else {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#	if 0
			else if (nDbgSeg >= 0)
				continue;
#	endif
#endif
			glBeginQuery (GL_SAMPLES_PASSED_ARB, gameData.render.lights.coronaQueries [faceP->m_info.nCorona - 1]);
			ogl.ClearError (1);
			glareRenderer.Render (nSegment, faceP->m_info.nSide, 1, faceP->m_info.fRads [0]);
			ogl.ClearError (1);
			glEndQuery (GL_SAMPLES_PASSED_ARB);
			ogl.ClearError (1);
			}	
		}
	}
}

//------------------------------------------------------------------------------

void QueryCoronas (short nFaces, int nPass)
{
BeginRenderFaces (RENDER_CORONAS, 0);
ogl.SetDepthWrite (false);
ogl.ColorMask (1,1,1,1,1);
if (nPass == 1) {	//find out how many total fragments each corona has
	gameStates.render.bQueryCoronas = 1;
	// first just render all coronas (happens before any geometry gets rendered)
	//for (i = 0; i < gameStates.app.nThreads; i++)
		RenderCoronaFaceList (gameData.render.faceIndex, 0);
	glFlush ();
	// then query how many samples (pixels) were rendered for each corona
	RenderCoronaFaceList (gameData.render.faceIndex, 1);
	}
else { //now find out how many fragments are rendered for each corona if geometry interferes
	gameStates.render.bQueryCoronas = 2;
	RenderCoronaFaceList (gameData.render.faceIndex, 2);
	glFlush ();
	}
ogl.SetDepthWrite (true);
glClearColor (0,0,0,0);
glClearDepth (0xffffffff);
glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
EndRenderFaces (0);
gameStates.render.bQueryCoronas = 0;
}

//------------------------------------------------------------------------------

int SetupCoronaFaces (void)
{
	tSegFaces	*segFaceP;
	CSegFace		*faceP;
	int			i, j, nSegment;

if (!(gameOpts->render.effects.bEnabled && gameOpts->render.coronas.bUse))
	return 0;
gameData.render.lights.nCoronas = 0;
for (i = 0; i < gameData.render.mine.nRenderSegs; i++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [0][i]))
		continue;
	if ((SEGMENTS [nSegment].m_nType == SEGMENT_IS_SKYBOX) ||
		 (SEGMENTS [nSegment].m_nType == SEGMENT_IS_OUTDOOR))
		continue;
	segFaceP = SEGFACES + nSegment;
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++)
		if (faceP->m_info.bVisible && (faceP->m_info.widFlags & WID_RENDER_FLAG) && faceP->m_info.bIsLight && (faceP->m_info.nCamera < 0) &&
			 glareRenderer.FaceHasCorona (nSegment, faceP->m_info.nSide, NULL, NULL))
			faceP->m_info.nCorona = ++gameData.render.lights.nCoronas;
		else
			faceP->m_info.nCorona = 0;
	}
return gameData.render.lights.nCoronas;
}

//------------------------------------------------------------------------------

static short RenderSegmentFaces (int nType, short nSegment, int bDepthOnly, int bAutomap, int bHeadlight)
{
if (nSegment < 0)
	return 0;

	tSegFaces	*segFaceP = SEGFACES + nSegment;
	CSegFace		*faceP;
	short			nFaces = 0;
	int			i;

#if DBG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
if (!(bHeadlight || VisitSegment (nSegment, bAutomap)))
	return 0;
#if DBG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
if (gameStates.render.bPerPixelLighting == 2)
	lightManager.Index (0)[0].nActive = -1;
for (i = segFaceP->nFaces, faceP = segFaceP->faceP; i; i--, faceP++) {
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
		nSegment = nSegment;
#endif
	if (RenderMineFace (SEGMENTS + nSegment, faceP, nType, bDepthOnly))
		nFaces++;
	}
return nFaces;
}

//------------------------------------------------------------------------------

short RenderSegments (int nType, int bDepthOnly, int bHeadlight)
{
	int	i, nFaces = 0, bAutomap = (nType <= RENDER_FACES);

if (nType > 1) {
	// render mine segment by segment
	if (gameData.render.mine.nRenderSegs == gameData.segs.nSegments) {
		CSegFace *faceP = FACES.faces.Buffer ();
		for (i = gameData.segs.nFaces; i; i--, faceP++)
			if (RenderMineFace (SEGMENTS + faceP->m_info.nSegment, faceP, nType, bDepthOnly))
				nFaces++;
		}
	else {
		for (i = gameData.render.mine.nRenderSegs; i; )
			nFaces += RenderSegmentFaces (nType, gameData.render.mine.nSegRenderList [0][--i], bDepthOnly, bAutomap, bHeadlight);
		}
	}
else {
	// render mine by pre-sorted textures
	nFaces = RenderFaceList (gameData.render.faceIndex, nType, bDepthOnly, bHeadlight);
	}
return nFaces;
}

//------------------------------------------------------------------------------

void RenderHeadlights (int nType)
{
if (gameStates.render.bPerPixelLighting && gameStates.render.bHeadlights) {
	ogl.SetBlendMode (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
	faceRenderFunc = lightmapManager.HaveLightmaps () ? RenderHeadlightsPP : RenderHeadlightsVL;
	RenderSegments (nType, 0, 1);
	SetFaceDrawer (-1);
	}
}

//------------------------------------------------------------------------------

int SetupCoronas (int nType)
{
if (!SetupCoronaFaces ())
	return 0;
if (automap.Display () && !gameOpts->render.automap.bCoronas)
	return 0;
int nCoronaStyle = glareRenderer.Style ();
if (nCoronaStyle != 1)
	return 0;
if (ogl.m_states.bOcclusionQuery) {
	glGenQueries (gameData.render.lights.nCoronas, gameData.render.lights.coronaQueries.Buffer ());
	QueryCoronas (0, 1);
	}
BeginRenderFaces (RENDER_FACES, 1);
short nFaces = RenderSegments (nType, 1, 0);
EndRenderFaces (1);
if (ogl.m_states.bOcclusionQuery && gameData.render.lights.nCoronas) {
	gameStates.render.bQueryCoronas = 2;
	gameStates.render.nType = RENDER_OBJECTS;
	RenderMineObjects (1);
	gameStates.render.nType = RENDER_FACES;
	QueryCoronas (nFaces, 2);
	}
EndRenderFaces (1);
return nFaces;
}

//------------------------------------------------------------------------------

int SetupDepthBuffer (int nType)
{
BeginRenderFaces (RENDER_FACES, 1);
RenderSegments (nType, 1, 0);
EndRenderFaces (1);
return SortFaces ();
}

//------------------------------------------------------------------------------

void RenderFaceList (int nType, int bFrontToBack)
{
if (nType > RENDER_OBJECTS) {	//back to front
	BeginRenderFaces (nType, 0);
	RenderSegments (nType, 0, 0);
	}
else {	//front to back
	if (!(nType || gameStates.render.nWindow))
		SetupCoronas (nType);
	BeginRenderFaces (nType, 0);
	ogl.ColorMask (1,1,1,1,1);
	gameData.render.mine.nVisited++;
	RenderSegments (nType, 0, 0);
	ogl.SetDepthWrite (true);
	RenderHeadlights (nType);
	}
EndRenderFaces (0);
}

//------------------------------------------------------------------------------
// eof
