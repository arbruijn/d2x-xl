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

#define RENDER_DEPTHMASK_FIRST 0

//------------------------------------------------------------------------------

#if SORT_RENDER_FACES > 1

void ResetFaceList (int nThread)
{
PROF_START
	CFaceListIndex *flxP = gameData.render.faceIndex + nThread;
#if SORT_RENDER_FACES == 2
#	if 0//def _DEBUG
memset (flxP->roots, 0xff, sizeof (flxP->roots));
#	else
short *roots = flxP->roots,
		*usedKeys = flxP->usedKeys;
for (int i = 0, h = flxP->nUsedKeys; i < h; i++) 
	roots [usedKeys [i]] = -1;
#	endif
#else
#	if 0//def _DEBUG
memset (flxP->tails, 0xff, sizeof (flxP->tails));
#	else
short *tails = flxP->tails.Buffer (),
		*usedKeys = flxP->usedKeys.Buffer ();
for (int i = 0, h = flxP->nUsedKeys; i < h; i++) 
	tails [usedKeys [i]] = -1;
#	endif
#endif
flxP->nUsedFaces = nThread ? LEVEL_FACES : 0;
flxP->nUsedKeys = 0;
PROF_END(ptFaceList)
}

//------------------------------------------------------------------------------

int AddFaceListItem (CSegFace *faceP, int nThread)
{
if (!(faceP->widFlags & WID_RENDER_FLAG))
	return 0;
#if 1
if (faceP->nFrame == gameData.app.nMineRenderCount)
	return 0;
#endif
#if DBG
if (faceP - FACES.faces >= gameData.segs.nFaces)
	return 0;
#endif
short	i, j, nKey = faceP->nBaseTex;

if (nKey < 0)
	return 0;

PROF_START
CFaceListIndex	*flxP = gameData.render.faceIndex + nThread;

if (faceP->nOvlTex)
	nKey += MAX_WALL_TEXTURES + faceP->nOvlTex;
if (nThread)
	i = --flxP->nUsedFaces;
else
	i = flxP->nUsedFaces++;
#if SORT_RENDER_FACES == 2
j = flxP->roots [nKey];
if (j < 0)
	flxP->usedKeys [flxP->nUsedKeys++] = nKey;
gameData.render.faceList [i].nNextItem = j;
gameData.render.faceList [i].faceP = faceP;
flxP->roots [nKey] = i;
#else
j = flxP->tails [nKey];
if (j < 0) {
	flxP->usedKeys [flxP->nUsedKeys] = nKey;
	flxP->nUsedKeys++;
	flxP->roots [nKey] = i;
	}
else
	gameData.render.faceList [j].nNextItem = i;
gameData.render.faceList [i].nNextItem = -1;
gameData.render.faceList [i].faceP = faceP;
flxP->tails [nKey] = i;
faceP->nFrame = gameData.app.nFrameCount;
#endif
PROF_END(ptFaceList)
return 1;
}

#endif //SORT_RENDER_FACES > 1

//------------------------------------------------------------------------------

void LoadFaceBitmaps (CSegment *segP, CSegFace *faceP)
{
	CSide	*sideP = segP->m_sides + faceP->nSide;
	short	nFrame = sideP->m_nFrame;

if (faceP->nCamera >= 0) {
	if (SetupMonitorFace (faceP->nSegment, faceP->nSide, faceP->nCamera, faceP)) 
		return;
	faceP->nCamera = -1;
	}
#if DBG
if (FACE_IDX (faceP) == nDbgFace)
	nDbgFace = nDbgFace;
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if ((faceP->nBaseTex < 0) || !faceP->bTextured)
	return;
if (faceP->bOverlay)
	faceP->nBaseTex = sideP->m_nOvlTex;
else {
	faceP->nBaseTex = sideP->m_nBaseTex;
	if (!faceP->bSplit)
		faceP->nOvlTex = sideP->m_nOvlTex;
	}
if (gameOpts->ogl.bGlTexMerge) {
	faceP->bmBot = LoadFaceBitmap (faceP->nBaseTex, nFrame);
	if (nFrame)
		nFrame = nFrame;
	if (faceP->nOvlTex)
		faceP->bmTop = LoadFaceBitmap ((short) (faceP->nOvlTex), nFrame);
	else
		faceP->bmTop = NULL;
	}
else {
	if (faceP->nOvlTex != 0) {
		faceP->bmBot = TexMergeGetCachedBitmap (faceP->nBaseTex, faceP->nOvlTex, faceP->nOvlOrient);
		if (faceP->bmBot)
			faceP->bmBot->SetupTexture (1, 1);
#if DBG
		else
			faceP->bmBot = TexMergeGetCachedBitmap (faceP->nBaseTex, faceP->nOvlTex, faceP->nOvlOrient);
#endif
		faceP->bmTop = NULL;
		}
	else {
		faceP->bmBot = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [faceP->nBaseTex].index;
		LoadBitmap (gameData.pig.tex.bmIndexP [faceP->nBaseTex].index, gameStates.app.bD1Mission);
		}
	}
}

//------------------------------------------------------------------------------

#if RENDER_DEPTHMASK_FIRST

void SplitFace (tSegFaces *segFaceP, CSegFace *faceP)
{
	CSegFace *newFaceP;

if (gameStates.render.bPerPixelLighting)
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
#if DBG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
newFaceP = segFaceP->faceP + segFaceP->nFaces++;
*newFaceP = *faceP;
newFaceP->nIndex = (newFaceP - 1)->nIndex + (newFaceP - 1)->nTris * 3;
memcpy (newFaceP->index, faceP->index, sizeof (faceP->index));
int nVerts = newFaceP->nTris * 3;
memcpy (FACES.vertices + newFaceP->nIndex, FACES.vertices + faceP->nIndex, nVerts * sizeof (CFloatVector3));
memcpy (FACES.texCoord + newFaceP->nIndex, FACES.ovlTexCoord + faceP->nIndex, nVerts * sizeof (tTexCoord2f));
memcpy (FACES.color + newFaceP->nIndex, FACES.color + faceP->nIndex, nVerts * sizeof (tRgbaColorf));
newFaceP->nBaseTex = faceP->nOvlTex;
faceP->nOvlTex = newFaceP->nOvlTex = 0;
faceP->bmTop = 
newFaceP->bmTop = NULL;
faceP->bSplit = 1;
newFaceP->bOverlay = 1;
if ((newFaceP->bIsLight = IsLight (newFaceP->nBaseTex)))
	faceP->bIsLight = 0;
}

#endif

//------------------------------------------------------------------------------

bool RenderSolidFace (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
if (!(faceP->widFlags & WID_RENDER_FLAG))
	return false;
if (IS_WALL (faceP->nWall))
	return false;
LoadFaceBitmaps (segP, faceP);
g3FaceDrawer (faceP, faceP->bmBot, faceP->bmTop, (faceP->nCamera < 0) || faceP->bTeleport, !bDepthOnly && faceP->bTextured, bDepthOnly);
return true;
}

//------------------------------------------------------------------------------

bool RenderWallFace (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
if (!(faceP->widFlags & WID_RENDER_FLAG))
	return false;
if (!IS_WALL (faceP->nWall))
	return false;
LoadFaceBitmaps (segP, faceP);
g3FaceDrawer (faceP, faceP->bmBot, faceP->bmTop, (faceP->nCamera < 0) || faceP->bTeleport, !bDepthOnly && faceP->bTextured, bDepthOnly);
return true;
}

//------------------------------------------------------------------------------

bool RenderColoredFace (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
if (!(faceP->widFlags & WID_RENDER_FLAG))
	return false;
if (faceP->bmBot)
	return false;
short nSegment = faceP->nSegment;
short special = SEGMENTS [nSegment].m_nType;
if ((special < SEGMENT_IS_WATER) || (special > SEGMENT_IS_TEAM_RED) || 
	 (SEGMENTS [nSegment].m_group < 0) || (SEGMENTS [nSegment].m_owner < 1))
	return false;
if (!gameData.app.nFrameCount)
	gameData.render.nColoredFaces++;
g3FaceDrawer (faceP, faceP->bmBot, faceP->bmTop, (faceP->nCamera < 0) || faceP->bTeleport, !bDepthOnly && faceP->bTextured, bDepthOnly);
return true;
}

//------------------------------------------------------------------------------

bool RenderCoronaFace (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
if (!(faceP->widFlags & WID_RENDER_FLAG))
	return false;
if (!faceP->nCorona)
	return false;
#if DBG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
RenderCorona (faceP->nSegment, faceP->nSide, CoronaVisibility (faceP->nCorona), faceP->fRads [0]);
return true;
}

//------------------------------------------------------------------------------

bool RenderSkyBoxFace (CSegment *segP, CSegFace *faceP, int bDepthOnly)
{
LoadFaceBitmaps (segP, faceP);
G3DrawFaceArrays (faceP, faceP->bmBot, faceP->bmTop, 1, 1, 0);
return true;
}

//------------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef bool (__fastcall * pRenderHandler) (CSegment *segP, CSegFace *faceP, int bDepthOnly);
#else
typedef bool (* pRenderHandler) (CSegment *segP, CSegFace *faceP, int bDepthOnly);
#endif

static pRenderHandler renderHandlers [] = {RenderSolidFace, RenderWallFace, RenderColoredFace, RenderCoronaFace, RenderSkyBoxFace};


static inline bool RenderMineFace (CSegment *segP, CSegFace *faceP, int nType, int bDepthOnly)
{
if (!faceP->bVisible)
	return false;
#if DBG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
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
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
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
			bLightmaps = (nType < 4) && !gameStates.render.bFullBright && lightmapManager.HaveLightmaps (),
			bNormals = !bDepthOnly; 

gameData.threads.vertColor.data.bDarkness = 0;
gameStates.render.nType = nType;
gameStates.render.history.nShader = -1;
gameStates.render.history.bOverlay = -1;
gameStates.render.history.bColored = 1;
gameStates.render.history.nBlendMode = -1;
gameStates.render.history.bmBot = 
gameStates.render.history.bmTop =
gameStates.render.history.bmMask = NULL;
gameStates.render.bQueryCoronas = 0;
glEnable (GL_CULL_FACE);
CTexture::Wrap (GL_REPEAT);
if (!bDepthOnly) 
	glDepthFunc (GL_LEQUAL);
else {
	glColorMask (0,0,0,0);
	glDepthMask (1);
	glDepthFunc (GL_LESS);
	}
if (nType == 3) {
	if (CoronaStyle () == 2)
		LoadGlareShader (10);
	return 0;
	}
else {
#if GEOMETRY_VBOS
	if (FACES.vboDataHandle) {
		glBindBufferARB (GL_ARRAY_BUFFER_ARB, FACES.vboDataHandle);
		bVBO = 1;
		}
#endif
	if ((nType < 4) && (gameStates.render.bPerPixelLighting == 2)) {
		OglEnableLighting (1);
		for (int i = 0; i < 8; i++)
			glEnable (GL_LIGHT0 + i);
		glDisable (GL_LIGHTING);
		glColor4f (1,1,1,1);
		}
	}
OglSetupTransform (1);
G3EnableClientStates (!bDepthOnly, !(bDepthOnly || gameStates.render.bFullBright), bNormals, GL_TEXTURE0);
#if GEOMETRY_VBOS
if (bVBO) {
	if (bNormals)
		glNormalPointer (GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iNormals));
	if (!bDepthOnly) {
		if (bLightmaps)
			glTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iLMapTexCoord));
		else
			glTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iTexCoord));
		glColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iColor));
		}
	glVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iVertices));
	if (bLightmaps) {
		G3EnableClientStates (1, 1, bNormals, GL_TEXTURE1);
		glTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iTexCoord));
		glColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iColor));
		glVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iVertices));
		}
	G3EnableClientStates (1, 1, 0, GL_TEXTURE1 + bLightmaps);
	glTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iOvlTexCoord));
	glColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iColor));
	glVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iVertices));
	G3EnableClientStates (1, 1, 0, GL_TEXTURE2 + bLightmaps);
	glTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iOvlTexCoord));
	glColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iColor));
	glVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (FACES.iVertices));
	if (FACES.vboIndexHandle)
		glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, FACES.vboIndexHandle);
	}	
else 
#endif
 {
	if (bNormals)
		glNormalPointer (GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.normals.Buffer ()));
	if (!bDepthOnly) {
		if (bLightmaps)
			glTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.lMapTexCoord.Buffer ()));
		else
			glTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.texCoord.Buffer ()));
		if (!gameStates.render.bFullBright)
			glColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
		}
	glVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.vertices.Buffer ()));
	if (bLightmaps) {
		G3EnableClientStates (1, !gameStates.render.bFullBright, bNormals, GL_TEXTURE1);
		glTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.texCoord.Buffer ()));
		if (!gameStates.render.bFullBright)
			glColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
		glVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.vertices.Buffer ()));
		}
	G3EnableClientStates (1, !gameStates.render.bFullBright, bNormals, GL_TEXTURE1 + bLightmaps);
	glTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.ovlTexCoord.Buffer ()));
	if (!gameStates.render.bFullBright)
		glColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
	glVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid*> (FACES.vertices.Buffer ()));
	G3EnableClientStates (1, !gameStates.render.bFullBright, 0, GL_TEXTURE2 + bLightmaps);
	glTexCoordPointer (2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.ovlTexCoord.Buffer ()));
	if (!gameStates.render.bFullBright)
		glColorPointer (4, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.color.Buffer ()));
	glVertexPointer (3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *> (FACES.vertices.Buffer ()));
	}
if (bNormals)
	G3EnableClientState (GL_NORMAL_ARRAY, GL_TEXTURE0);
glEnable (GL_BLEND);
glBlendFunc (GL_ONE, GL_ZERO);
OglClearError (0);
return 1;
}

//------------------------------------------------------------------------------

void EndRenderFaces (int nType, int bVertexArrays, int bDepthOnly)
{
#if 1
G3FlushFaceBuffer (1);
#endif
//if (bVertexArrays) 
	{
	if (!bDepthOnly) {
		G3DisableClientStates (1, 1, 0, GL_TEXTURE3);
		glEnable (GL_TEXTURE_2D);
		OGL_BINDTEX (0);
		glDisable (GL_TEXTURE_2D);

		G3DisableClientStates (1, 1, 0, GL_TEXTURE2);
		glEnable (GL_TEXTURE_2D);
		OGL_BINDTEX (0);
		glDisable (GL_TEXTURE_2D);

		G3DisableClientStates (1, 1, 0, GL_TEXTURE1);
		glEnable (GL_TEXTURE_2D);
		OGL_BINDTEX (0);
		glDisable (GL_TEXTURE_2D);
		}
	G3DisableClientStates (!bDepthOnly, !bDepthOnly, 1, GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	}
//if (gameStates.render.history.bOverlay > 0)
if (gameStates.ogl.bShadersOk) {
	glUseProgramObject (0);
	gameStates.render.history.nShader = -1;
	}
if (nType != 3) {
	if (gameStates.render.bPerPixelLighting == 2)
		OglDisableLighting ();
	OglResetTransform (1);
	if (FACES.vboDataHandle)
		glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
	}
else 	if (CoronaStyle () == 2)
	UnloadGlareShader ();
else if (gameStates.ogl.bOcclusionQuery && gameData.render.lights.nCoronas && !gameStates.render.bQueryCoronas && (CoronaStyle () == 1))
	glDeleteQueries (gameData.render.lights.nCoronas, gameData.render.lights.coronaQueries.Buffer ());
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
OglClearError (0);
}

//------------------------------------------------------------------------------

void RenderSkyBoxFaces (void)
{
	tSegFaces*	segFaceP;
	CSegFace*		faceP;
	short*		segP;
	int			i, j, nSegment, bVertexArrays, bFullBright = gameStates.render.bFullBright;

if (gameStates.render.bHaveSkyBox) {
	glDepthMask (1);
	gameStates.render.nType = 4;
	gameStates.render.bFullBright = 1;
	bVertexArrays = BeginRenderFaces (4, 0);
	for (i = gameData.segs.skybox.ToS (), segP = gameData.segs.skybox.Buffer (); i; i--, segP++) {
		nSegment = *segP;
		segFaceP = SEGFACES + nSegment;
		for (j = segFaceP->nFaces, faceP = segFaceP->faceP; j; j--, faceP++) {
			if (!(faceP->bVisible = FaceIsVisible (nSegment, faceP->nSide)))
				continue;
			RenderMineFace (SEGMENTS + nSegment, faceP, 4, 0);
			}
		}
	gameStates.render.bFullBright = bFullBright;
	EndRenderFaces (4, bVertexArrays, 0);
	}
}

//------------------------------------------------------------------------------

static inline int VisitSegment (short nSegment, int bAutomap)
{
if (nSegment < 0)
	return 0;
if (bAutomap) {
	if (automap.m_bDisplay) {
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
	short				i, j, nFaces = 0, nSegment = -1;
	int				bAutomap = (nType == 0);

for (i = 0; i < flx.nUsedKeys; i++) {
	for (j = flx.roots [flx.usedKeys [i]]; j >= 0; j = fliP->nNextItem) {
		fliP = gameData.render.faceList + j;
		faceP = fliP->faceP;
		if (nSegment != faceP->nSegment) {
			nSegment = faceP->nSegment;
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#endif
			if (!bDepthOnly) {
				if (!bHeadlight)
					VisitSegment (nSegment, bAutomap);
				if (gameStates.render.bPerPixelLighting == 2)
					lightManager.Index (0)[0].nActive = -1;
				}
			}
		faceP = fliP->faceP;
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
	CSegFace*			faceP;
	short				i, j, nSegment;

for (i = 0; i < flx.nUsedKeys; i++) {
	for (j = flx.roots [flx.usedKeys [i]]; j >= 0; j = fliP->nNextItem) {
		fliP = gameData.render.faceList + j;
		faceP = fliP->faceP;
		if (!faceP->nCorona)
			continue;
		nSegment = faceP->nSegment;
		if (automap.m_bDisplay) {
			if (!(automap.m_bFull || automap.m_visible [nSegment]))
				return;
			if (!gameOpts->render.automap.bSkybox && (SEGMENTS [nSegment].m_nType == SEGMENT_IS_SKYBOX))
				continue;
			}
		if (nPass == 1) {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#	if 0
			else if (nDbgSeg >= 0)
				continue;
#	endif
#endif
			CoronaVisibility (faceP->nCorona);
			}
		else if (nPass == 2) {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#	if 0
			else if (nDbgSeg >= 0)
				continue;
#	endif
#endif
			glBeginQuery (GL_SAMPLES_PASSED_ARB, gameData.render.lights.coronaQueries [faceP->nCorona - 1]);
			if (!glGetError ())
				RenderCorona (nSegment, faceP->nSide, 1, faceP->fRads [0]);
			glEndQuery (GL_SAMPLES_PASSED_ARB);
			}
		else {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#	if 0
			else if (nDbgSeg >= 0)
				continue;
#	endif
#endif
			glBeginQuery (GL_SAMPLES_PASSED_ARB, gameData.render.lights.coronaQueries [faceP->nCorona - 1]);
			OglClearError (1);
			RenderCorona (nSegment, faceP->nSide, 1, faceP->fRads [0]);
			OglClearError (1);
			glEndQuery (GL_SAMPLES_PASSED_ARB);
			OglClearError (1);
			}	
		}
	}
}

//------------------------------------------------------------------------------

void QueryCoronas (short nFaces, int nPass)
{
	int	bVertexArrays = BeginRenderFaces (3, 0);

glDepthMask (0);
glColorMask (1,1,1,1);
if (nPass == 1) {	//find out how many total fragments each corona has
	gameStates.render.bQueryCoronas = 1;
	// first just render all coronas (happens before any geometry gets rendered)
	RenderCoronaFaceList (gameData.render.faceIndex [0], 0);
	if (gameStates.app.bMultiThreaded)
		RenderCoronaFaceList (gameData.render.faceIndex [1], 0);
	glFlush ();
	// then query how many samples (pixels) were rendered for each corona
	RenderCoronaFaceList (gameData.render.faceIndex [0], 1);
	if (gameStates.app.bMultiThreaded)
		RenderCoronaFaceList (gameData.render.faceIndex [1], 1);
	}
else { //now find out how many fragments are rendered for each corona if geometry interferes
	gameStates.render.bQueryCoronas = 2;
	RenderCoronaFaceList (gameData.render.faceIndex [0], 2);
	if (gameStates.app.bMultiThreaded)
		RenderCoronaFaceList (gameData.render.faceIndex [1], 2);
	glFlush ();
	}
glDepthMask (1);
glClearColor (0,0,0,0);
glClearDepth (0xffffffff);
glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
EndRenderFaces (3, bVertexArrays, 0);
gameStates.render.bQueryCoronas = 0;
}

//------------------------------------------------------------------------------

int SetupCoronaFaces (void)
{
	tSegFaces	*segFaceP;
	CSegFace		*faceP;
	int			i, j, nSegment;

if (!gameOpts->render.coronas.bUse)
	return 0;
gameData.render.lights.nCoronas = 0;
for (i = 0; i < gameData.render.mine.nRenderSegs; i++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
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
		if (faceP->bVisible && (faceP->widFlags & WID_RENDER_FLAG) && faceP->bIsLight && (faceP->nCamera < 0) &&
			 FaceHasCorona (nSegment, faceP->nSide, NULL, NULL))
			faceP->nCorona = ++gameData.render.lights.nCoronas;
		else
			faceP->nCorona = 0;
	}
return gameData.render.lights.nCoronas;
}

//------------------------------------------------------------------------------

static short RenderSegmentFaces (int nType, short nSegment, int bVertexArrays, int bDepthOnly, int bAutomap, int bHeadlight)
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
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		nSegment = nSegment;
#endif
	if (RenderMineFace (SEGMENTS + nSegment, faceP, nType, bDepthOnly))
		nFaces++;
	}
return nFaces;
}

//------------------------------------------------------------------------------

short RenderSegments (int nType, int bVertexArrays, int bDepthOnly, int bHeadlight)
{
	int	i, nFaces = 0, bAutomap = (nType == 0);

if (nType) {
	if (gameData.render.mine.nRenderSegs == gameData.segs.nSegments) {
		CSegFace *faceP = FACES.faces.Buffer ();
		for (i = gameData.segs.nFaces; i; i--, faceP++)
			if (RenderMineFace (SEGMENTS + faceP->nSegment, faceP, nType, bDepthOnly))
				nFaces++;
		}
	else {
		for (i = gameData.render.mine.nRenderSegs; i; )
			nFaces += RenderSegmentFaces (nType, gameData.render.mine.nSegRenderList [--i], bVertexArrays, bDepthOnly, bAutomap, bHeadlight);
		}
	}
else {
#if SORT_RENDER_FACES > 1
	nFaces += RenderFaceList (gameData.render.faceIndex [0], nType, bDepthOnly, bHeadlight);
	if (gameStates.app.bMultiThreaded)
		nFaces += RenderFaceList (gameData.render.faceIndex [1], nType, bDepthOnly, bHeadlight);
#else
	for (i = 0; i < gameData.render.mine.nRenderSegs; i++)
		nFaces += RenderSegmentFaces (nType, gameData.render.mine.nSegRenderList [i], bVertexArrays, bDepthOnly, bAutomap, bHeadlight);
#endif
	}
return nFaces;
}

//------------------------------------------------------------------------------

void RenderHeadlights (int nType, int bVertexArrays)
{
if (gameStates.render.bPerPixelLighting && gameStates.render.bHeadlights) {
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
	g3FaceDrawer = lightmapManager.HaveLightmaps () ? G3DrawHeadlightsPPLM : G3DrawHeadlights;
	RenderSegments (nType, bVertexArrays, 0, 1);
	SetFaceDrawer (-1);
	}
}

//------------------------------------------------------------------------------

int SetupCoronas (int nType)
{
if (!SetupCoronaFaces ())
	return 0;
int nCoronaStyle = CoronaStyle ();
if (nCoronaStyle != 1)
	return 0;
if (gameStates.ogl.bOcclusionQuery) {
	glGenQueries (gameData.render.lights.nCoronas, gameData.render.lights.coronaQueries.Buffer ());
	QueryCoronas (0, 1);
	}
int bVertexArrays = BeginRenderFaces (0, 1);
short nFaces = RenderSegments (nType, bVertexArrays, 1, 0);
EndRenderFaces (0, bVertexArrays, 1);
if (gameOpts->render.coronas.bUse && gameStates.ogl.bOcclusionQuery && gameData.render.lights.nCoronas) {
	gameStates.render.bQueryCoronas = 2;
	gameStates.render.nType = 1;
	RenderMineObjects (1);
	gameStates.render.nType = 0;
	QueryCoronas (nFaces, 2);
	}
EndRenderFaces (0, bVertexArrays, 1);
return nFaces;
}

//------------------------------------------------------------------------------

int SetupDepthBuffer (int nType)
{
int bVertexArrays = BeginRenderFaces (0, 1);
RenderSegments (nType, bVertexArrays, 1, 0);
EndRenderFaces (0, bVertexArrays, 1);
return SortFaces ();
}

//------------------------------------------------------------------------------

void RenderFaceList (int nType)
{
	int	j, bVertexArrays;

if (nType) {	//back to front
	bVertexArrays = BeginRenderFaces (nType, 0);
	RenderSegments (nType, bVertexArrays, 0, 0);
	if (nType < 2)
		RenderHeadlights (nType, bVertexArrays);
	}
else {	//front to back
	if (!gameStates.render.nWindow)
		j = SetupCoronas (nType);
	else
		j = 0;
	bVertexArrays = BeginRenderFaces (0, 0);
	glColorMask (1,1,1,1);
	gameData.render.mine.nVisited++;
	RenderSegments (nType, bVertexArrays, 0, 0);
	glDepthMask (1);
	RenderHeadlights (0, bVertexArrays);
	}
EndRenderFaces (nType, bVertexArrays, 0);
}

//------------------------------------------------------------------------------
// eof
