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
if (!(faceP->m_info.widFlags & WID_VISIBLE_FLAG))
	return 0;
#if 1
if (faceP->m_info.nFrame == gameData.app.nMineRenderCount)
	return 0;
#endif
#if DBG
if (faceP - FACES.faces >= gameData.segs.nFaces)
	return 0;
#endif

#if USE_OPENMP //> 1
#	pragma omp critical
#elif !USE_OPENMP
SDL_mutexP (tiRender.semaphore);
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
#if !USE_OPENMP
SDL_mutexV (tiRender.semaphore);
#endif

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
		LoadTexture (gameData.pig.tex.bmIndexP [faceP->m_info.nBaseTex].index, gameStates.app.bD1Mission);
		}
	}
}

//------------------------------------------------------------------------------

bool RenderGeometryFace (CSegment *segP, CSegFace *faceP)
{
faceRenderFunc (faceP, faceP->bmBot, faceP->bmTop, faceP->m_info.bTextured, (gameStates.render.bPerPixelLighting != 0) && !gameStates.render.bFullBright);
return true;
}

//------------------------------------------------------------------------------

bool RenderCoronaFace (CSegment *segP, CSegFace *faceP)
{
if (!faceP->m_info.nCorona)
	return false;
glareRenderer.Render (faceP->m_info.nSegment, faceP->m_info.nSide, 1.0f, faceP->m_info.fRads [0]);
return true;
}

//------------------------------------------------------------------------------

bool RenderSkyBoxFace (CSegment *segP, CSegFace *faceP)
{
LoadFaceBitmaps (segP, faceP);
RenderFace (faceP, faceP->bmBot, faceP->bmTop, 1, 0);
return true;
}

//------------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef bool (__fastcall * pRenderHandler) (CSegment *segP, CSegFace *faceP);
#else
typedef bool (* pRenderHandler) (CSegment *segP, CSegFace *faceP);
#endif

static pRenderHandler renderHandlers [] = {RenderGeometryFace, RenderGeometryFace, RenderCoronaFace, RenderSkyBoxFace};


static inline bool RenderMineFace (CSegment *segP, CSegFace *faceP, int nType)
{
#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
return renderHandlers [nType] (segP, faceP);
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

for (h = i = 0, ph = faceRef [0]; i < gameData.render.mine.nRenderSegs [0]; i++) {
	if (0 > (nSegment = gameData.render.mine.segRenderList [0][i]))
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

int BeginRenderFaces (int nType)
{
	//int	bVBO = 0;
	int	bLightmaps = (nType == RENDER_TYPE_GEOMETRY) && !gameStates.render.bFullBright && lightmapManager.HaveLightmaps ();

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
ogl.SetDepthWrite (1);
ogl.SetFaceCulling (true);
CTexture::Wrap (GL_REPEAT);
#if 0
if (nType == RENDER_TYPE_ZCULL) {
	ogl.ColorMask (0,0,0,0,1);
	glDepthMask (1);
	ogl.SetDepthMode (GL_LEQUAL); 
	}
else 
#endif
	{
	ogl.ColorMask (1,1,1,1,1);
#if 0
	glDepthMask (0);
#endif
	if (nType == RENDER_TYPE_CORONAS) {
		if (glareRenderer.Style ())
			glareRenderer.LoadShader (10, 1);
		ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
		ogl.SetBlendMode (OGL_BLEND_ADD);
#if 0
		ogl.SetDepthMode (GL_LEQUAL); 
#endif
		return 0;
		}
#if 0
	ogl.SetDepthMode (GL_EQUAL); 
#else
	ogl.SetDepthMode (GL_LEQUAL); 
#endif
	}

#if GEOMETRY_VBOS
if (FACES.vboDataHandle) {
	glBindBufferARB (GL_ARRAY_BUFFER_ARB, FACES.vboDataHandle);
	bVBO = 1;
	}
#endif
if ((nType == RENDER_TYPE_GEOMETRY) && (gameStates.render.bPerPixelLighting == 2)) {
	ogl.EnableLighting (1);
	for (int i = 0; i < 8; i++)
		glEnable (GL_LIGHT0 + i);
	ogl.SetLighting (false);
	glColor4f (1,1,1,1);
	}
ogl.SetupTransform (1);
#if GEOMETRY_VBOS
if (bVBO) {
	OglNormalPointer (GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iNormals));
	if (bLightmaps)
		OglTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iLMapTexCoord));
	else
		OglTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iTexCoord));
		OglColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iColor));
		}
	OglVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iVertices));
	if (bLightmaps) {
		ogl.EnableClientStates (1, 1, 1, GL_TEXTURE1);
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
	if (bLightmaps) {
		ogl.EnableClientStates (1, !gameStates.render.bFullBright, 1, GL_TEXTURE1);
		OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.texCoord.Buffer ()));
		if (!gameStates.render.bFullBright)
			OglColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
		OglVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.vertices.Buffer ()));
		}
	ogl.EnableClientStates (1, !gameStates.render.bFullBright, 1, GL_TEXTURE1 + bLightmaps);
	OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.ovlTexCoord.Buffer ()));
	if (!gameStates.render.bFullBright)
		OglColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
	OglVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid*> (FACES.vertices.Buffer ()));
	if (nType < RENDER_TYPE_CORONAS) {
		ogl.EnableClientStates (1, !gameStates.render.bFullBright, 0, GL_TEXTURE2 + bLightmaps);
		OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.ovlTexCoord.Buffer ()));
		if (!gameStates.render.bFullBright)
			OglColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
		OglVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.vertices.Buffer ()));
		}
	ogl.EnableClientStates (1, !gameStates.render.bFullBright, 1, GL_TEXTURE0);
	OglNormalPointer (GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.normals.Buffer ()));
	if (bLightmaps)
		OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.lMapTexCoord.Buffer ()));
	else
		OglTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.texCoord.Buffer ()));
	if (!gameStates.render.bFullBright)
		OglColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
	OglVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.vertices.Buffer ()));
	}
if (gameStates.render.bFullBright)
	glColor3f (1,1,1);
ogl.SetBlendMode (OGL_BLEND_REPLACE);
ogl.ClearError (0);
return 1;
}

//------------------------------------------------------------------------------

void EndRenderFaces (void)
{
#if 0
G3FlushFaceBuffer (1);
#endif
ogl.ResetClientStates ();
shaderManager.Deploy (-1);
ogl.DisableLighting ();
ogl.ResetTransform (1);
if (FACES.vboDataHandle)
	glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
glareRenderer.UnloadShader ();
ogl.SetBlendMode (OGL_BLEND_ALPHA);
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
	gameStates.render.nType = RENDER_TYPE_SKYBOX;
	gameStates.render.bFullBright = 1;
	BeginRenderFaces (RENDER_TYPE_SKYBOX);
	for (i = gameData.segs.skybox.ToS (), segP = gameData.segs.skybox.Buffer (); i; i--, segP++) {
		nSegment = *segP;
		segFaceP = SEGFACES + nSegment;
		for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++) {
			if (!(faceP->m_info.bVisible = FaceIsVisible (nSegment, faceP->m_info.nSide)))
				continue;
			RenderMineFace (SEGMENTS + nSegment, faceP, RENDER_TYPE_SKYBOX);
			}
		}
	gameStates.render.bFullBright = bFullBright;
	EndRenderFaces ();
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
		if (!gameOpts->render.automap.bSkybox && (SEGMENTS [nSegment].m_function == SEGMENT_FUNC_SKYBOX))
			return 0;
		}
	else if (gameData.objs.viewerP == gameData.objs.consoleP) {
#if DBG
		if (gameStates.render.nWindow)
			nDbgSeg = nDbgSeg;
#endif
		automap.m_visited [nSegment] = gameData.render.mine.bSetAutomapVisited;
		}
	}
if (VISITED (nSegment))
	return 0;
VISIT (nSegment);
return 1;
}

//------------------------------------------------------------------------------

static inline int FaceIsTransparent (CSegFace *faceP, CBitmap *bmBot, CBitmap *bmTop)
{
if (!bmBot)
	return faceP->m_info.nTransparent = faceP->m_info.bTransparent || (faceP->m_info.color.Alpha () < 1.0f);
if (faceP->m_info.bTransparent || faceP->m_info.bAdditive)
	return 1;
if (bmBot->Flags () & BM_FLAG_SEE_THRU)
	return faceP->m_info.nTransparent = 0;
if (bmTop && (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT))
	return 1;
if (!(bmBot->Flags () & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT)))
	return 0;
if (!bmTop)
	return 1;
if (bmTop->Flags () & BM_FLAG_SEE_THRU)
	return 0;
if (bmTop->Flags () & BM_FLAG_TRANSPARENT)
	return 1;
return 0;
}

//------------------------------------------------------------------------------

static inline int FaceIsColored (CSegFace *faceP)
{
return !automap.Display () || automap.m_visited [faceP->m_info.nSegment] || !gameOpts->render.automap.bGrayOut;
}

//------------------------------------------------------------------------------

short RenderFaceList (CFaceListIndex& flx, int nType, int bHeadlight)
{
	tFaceListItem*	fliP = &gameData.render.faceList [0];
	CSegFace*		faceP;
	int				i, j, nFaces = 0, nSegment = -1;
	int				bAutomap = (nType == RENDER_TYPE_GEOMETRY);

#if 1
if (automap.Display ())
	flx.usedKeys.SortAscending (0, flx.nUsedKeys - 1);
#endif
for (i = 0; i < flx.nUsedKeys; i++) {
	for (j = flx.roots [flx.usedKeys [i]]; j >= 0; j = fliP [j].nNextItem) {
		faceP = fliP [j].faceP;
		if (!faceP->m_info.bVisible)
			continue;
		LoadFaceBitmaps (SEGMENTS + faceP->m_info.nSegment, faceP);
#if DBG
		if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
			nDbgSeg = nDbgSeg;
#endif
		faceP->m_info.nTransparent = FaceIsTransparent (faceP, faceP->bmBot, faceP->bmTop);
		faceP->m_info.nColored = FaceIsColored (faceP);
		if (nSegment != faceP->m_info.nSegment) {
			nSegment = faceP->m_info.nSegment;
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#endif
			if (!bHeadlight)
				VisitSegment (nSegment, bAutomap);
			if (gameStates.render.bPerPixelLighting == 2)
				lightManager.Index (0,0).nActive = -1;
			}
		if (RenderMineFace (SEGMENTS + nSegment, faceP, nType))
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
			if (!gameOpts->render.automap.bSkybox && (SEGMENTS [nSegment].m_function == SEGMENT_FUNC_SKYBOX))
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
			glareRenderer.Render (nSegment, faceP->m_info.nSide, 1, faceP->m_info.fRads [0]);
			glEndQuery (GL_SAMPLES_PASSED_ARB);
			}	
		}
	}
}

//------------------------------------------------------------------------------

void QueryCoronas (short nFaces, int nPass)
{
BeginRenderFaces (RENDER_TYPE_CORONAS);
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
EndRenderFaces ();
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
for (i = 0; i < gameData.render.mine.nRenderSegs [0]; i++) {
	if (0 > (nSegment = gameData.render.mine.segRenderList [0][i]))
		continue;
	if ((SEGMENTS [nSegment].m_function == SEGMENT_FUNC_SKYBOX) ||
		 (SEGMENTS [nSegment].HasOutdoorsProp ()))
		continue;
	segFaceP = SEGFACES + nSegment;
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++)
		if (faceP->m_info.bVisible && (faceP->m_info.widFlags & WID_VISIBLE_FLAG) && faceP->m_info.bIsLight && (faceP->m_info.nCamera < 0) &&
			 glareRenderer.FaceHasCorona (nSegment, faceP->m_info.nSide, NULL, NULL))
			faceP->m_info.nCorona = ++gameData.render.lights.nCoronas;
		else
			faceP->m_info.nCorona = 0;
	}
return gameData.render.lights.nCoronas;
}

//------------------------------------------------------------------------------

static short RenderSegmentFaces (int nType, short nSegment, int bAutomap, int bHeadlight)
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
	lightManager.Index (0,0).nActive = -1;
for (i = segFaceP->nFaces, faceP = segFaceP->faceP; i; i--, faceP++) {
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
		nSegment = nSegment;
#endif
	if (!faceP->m_info.bVisible)
		continue;
	LoadFaceBitmaps (SEGMENTS + faceP->m_info.nSegment, faceP);
	faceP->m_info.nTransparent = FaceIsTransparent (faceP, faceP->bmBot, faceP->bmTop);
	faceP->m_info.nColored = FaceIsColored (faceP);
	if (RenderMineFace (SEGMENTS + nSegment, faceP, nType))
		nFaces++;
	}
return nFaces;
}

//------------------------------------------------------------------------------

short RenderSegments (int nType, int bHeadlight)
{
	int	i, nFaces = 0, bAutomap = (nType == RENDER_TYPE_GEOMETRY);

if (nType == RENDER_TYPE_CORONAS) {
	// render mine segment by segment
	if (gameData.render.mine.nRenderSegs [0] == gameData.segs.nSegments) {
		CSegFace *faceP = FACES.faces.Buffer ();
		for (i = gameData.segs.nFaces; i; i--, faceP++)
			if (RenderMineFace (SEGMENTS + faceP->m_info.nSegment, faceP, nType))
				nFaces++;
		}
	else {
		short* segListP = gameData.render.mine.segRenderList [0].Buffer ();
		for (i = gameData.render.mine.nRenderSegs [0]; i; )
			nFaces += RenderSegmentFaces (nType, segListP [--i], bAutomap, bHeadlight);
		}
	}
else {
	// render mine by pre-sorted textures
	nFaces = RenderFaceList (gameData.render.faceIndex, nType, bHeadlight);
	}
return nFaces;
}

//------------------------------------------------------------------------------

void RenderHeadlights (int nType)
{
if (gameStates.render.bPerPixelLighting && gameStates.render.bHeadlights) {
	ogl.SetBlendMode (OGL_BLEND_ADD_WEAK);
	faceRenderFunc = DrawHeadlights;
	RenderSegments (nType, 1);
	SetFaceDrawer (-1);
	}
}

//------------------------------------------------------------------------------

int SetupCoronas (void)
{
if (!SetupCoronaFaces ())
	return 0;
if (automap.Display () && !gameOpts->render.automap.bCoronas)
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int SetupDepthBuffer (int nType)
{
BeginRenderFaces (RENDER_TYPE_GEOMETRY);
RenderSegments (nType, 0);
EndRenderFaces ();
return SortFaces ();
}

//------------------------------------------------------------------------------

void RenderFaceList (int nType)
{
BeginRenderFaces (nType);
gameData.render.mine.nVisited++;
RenderSegments (nType, 0);
if (nType == RENDER_TYPE_GEOMETRY)
	RenderHeadlights (nType);
EndRenderFaces ();
}

//------------------------------------------------------------------------------
// eof
