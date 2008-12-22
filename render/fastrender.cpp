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
#include "dynlight.h"
#include "lightmap.h"
#include "automap.h"
#include "texmerge.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_fastrender.h"
#include "glare.h"
#include "render.h"
#include "renderthreads.h"
#include "fastrender.h"

#define RENDER_DEPTHMASK_FIRST 0

#define SORT_FACES 3

#if DBG
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

#if SORT_FACES > 1

void ResetFaceList (int nThread)
{
PROF_START
	CFaceListIndex *flxP = gameData.render.faceIndex + nThread;
#if SORT_FACES == 2
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
flxP->nUsedFaces = nThread ? MAX_FACES : 0;
flxP->nUsedKeys = 0;
PROF_END(ptFaceList)
}

//------------------------------------------------------------------------------

int AddFaceListItem (tFace *faceP, int nThread)
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
#if SORT_FACES == 2
j = flxP->roots [nKey];
if (j < 0)
	flxP->usedKeys [flxP->nUsedKeys++] = nKey;
gameData.render.faceList [i].nNextItem = j;
gameData.render.faceList [i].faceP = faceP;
flxP->roots [nKey] = i;
#else
j = flxP->tails [nKey];
if (j < 0) {
	flxP->usedKeys [flxP->nUsedKeys++] = nKey;
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

#endif //SORT_FACES > 1

//------------------------------------------------------------------------------

void LoadFaceBitmaps (CSegment *segP, tFace *faceP)
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
if (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk) {
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
#if DBG
		if (!faceP->bmBot)
			faceP->bmBot = TexMergeGetCachedBitmap (faceP->nBaseTex, faceP->nOvlTex, faceP->nOvlOrient);
#endif
		faceP->bmTop = NULL;
		}
	else {
		faceP->bmBot = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [faceP->nBaseTex].index;
		PIGGY_PAGE_IN (gameData.pig.tex.bmIndexP [faceP->nBaseTex].index, gameStates.app.bD1Mission);
		}
	}
}

//------------------------------------------------------------------------------

#if RENDER_DEPTHMASK_FIRST

void SplitFace (tSegFaces *segFaceP, tFace *faceP)
{
	tFace *newFaceP;

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
newFaceP = segFaceP->pFaces + segFaceP->nFaces++;
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

void FixTriangleFan (CSegment *segP, tFace *faceP)
{
if (((faceP->nType = segP->Type (faceP->nSide)) == SIDE_IS_TRI_13)) {	//rearrange vertex order for TRIANGLE_FAN rendering
	{
	short	h = faceP->index [0];
	memcpy (faceP->index, faceP->index + 1, 3 * sizeof (short));
	faceP->index [3] = h;
	}
	{
	CFloatVector3 h = FACES.vertices [faceP->nIndex];
	memcpy (FACES.vertices + faceP->nIndex, FACES.vertices + faceP->nIndex + 1, 3 * sizeof (CFloatVector3));
	FACES.vertices [faceP->nIndex + 3] = h;
	}
	{
	tTexCoord2f h = FACES.texCoord [faceP->nIndex];
	memcpy (FACES.texCoord + faceP->nIndex, FACES.texCoord + faceP->nIndex + 1, 3 * sizeof (tTexCoord2f));
	FACES.texCoord [faceP->nIndex + 3] = h;
	h = FACES.lMapTexCoord [faceP->nIndex];
	memcpy (FACES.lMapTexCoord + faceP->nIndex, FACES.lMapTexCoord + faceP->nIndex + 1, 3 * sizeof (tTexCoord2f));
	FACES.lMapTexCoord [faceP->nIndex + 3] = h;
	if (faceP->nOvlTex) {
		h = FACES.ovlTexCoord [faceP->nIndex];
		memcpy (FACES.ovlTexCoord + faceP->nIndex, FACES.ovlTexCoord + faceP->nIndex + 1, 3 * sizeof (tTexCoord2f));
		FACES.ovlTexCoord [faceP->nIndex + 3] = h;
		}
	}
	}
}

//------------------------------------------------------------------------------

bool RenderSolidFace (CSegment *segP, tFace *faceP, int bDepthOnly)
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

bool RenderWallFace (CSegment *segP, tFace *faceP, int bDepthOnly)
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

bool RenderColoredFace (CSegment *segP, tFace *faceP, int bDepthOnly)
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

bool RenderCoronaFace (CSegment *segP, tFace *faceP, int bDepthOnly)
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

bool RenderSkyBoxFace (CSegment *segP, tFace *faceP, int bDepthOnly)
{
LoadFaceBitmaps (segP, faceP);
G3DrawFaceArrays (faceP, faceP->bmBot, faceP->bmTop, 1, 1, 0);
return true;
}

//------------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef bool (__fastcall * pRenderHandler) (CSegment *segP, tFace *faceP, int bDepthOnly);
#else
typedef bool (* pRenderHandler) (CSegment *segP, tFace *faceP, int bDepthOnly);
#endif

static pRenderHandler renderHandlers [] = {RenderSolidFace, RenderWallFace, RenderColoredFace, RenderCoronaFace, RenderSkyBoxFace};


static inline bool RenderMineFace (CSegment *segP, tFace *faceP, int nType, int bDepthOnly)
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
	tFace	*faceP;
	} tFaceRef;

static tFaceRef faceRef [2][MAX_SEGMENTS_D2X * 6];

int QCmpFaces (tFace *fp, tFace *mp)
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
	tFace	m = *pf [(l + r) / 2].faceP;

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
	tFace		*faceP;
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
if (bVertexArrays) {
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
	tSegFaces	*segFaceP;
	tFace		*faceP;
	short			*segP;
	int			i, j, nSegment, bVertexArrays, bFullBright = gameStates.render.bFullBright;

if (gameStates.render.bHaveSkyBox) {
	glDepthMask (1);
	gameStates.render.nType = 4;
	gameStates.render.bFullBright = 1;
	bVertexArrays = BeginRenderFaces (4, 0);
	for (i = gameData.segs.skybox.ToS (), segP = gameData.segs.skybox.Buffer (); i; i--, segP++) {
		nSegment = *segP;
		segFaceP = SEGFACES + nSegment;
		for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++) {
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
	if (gameStates.render.automap.bDisplay) {
		if (!(gameStates.render.automap.bFull || gameData.render.mine.bAutomapVisible [nSegment]))
			return 0;
		if (!gameOpts->render.automap.bSkybox && (SEGMENTS [nSegment].m_nType == SEGMENT_IS_SKYBOX))
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

short RenderFaceList (CFaceListIndex& flx, int nType, int bDepthOnly, int bHeadlight)
{
	tFaceListItem*	fliP;
	tFace*			faceP;
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
					gameData.render.lights.dynamic.shader.index [0][0].nActive = -1;
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

void RenderCoronaFaceList (CFaceListIndex *flxP, int nPass)
{
	CFaceListIndex	flx = *flxP;
	tFaceListItem	*fliP;
	tFace			*faceP;
	short				i, j, nSegment;

for (i = 0; i < flx.nUsedKeys; i++) {
	for (j = flx.roots [flx.usedKeys [i]]; j >= 0; j = fliP->nNextItem) {
		fliP = gameData.render.faceList + j;
		faceP = fliP->faceP;
		if (!faceP->nCorona)
			continue;
		nSegment = faceP->nSegment;
		if (gameStates.render.automap.bDisplay) {
			if (!(gameStates.render.automap.bFull || gameData.render.mine.bAutomapVisible [nSegment]))
				return;
			if (!gameOpts->render.automap.bSkybox && (SEGMENTS [nSegment].m_nType == SEGMENT_IS_SKYBOX))
				continue;
			}
		if (nPass == 1) {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
			else
				continue;
#endif
			CoronaVisibility (faceP->nCorona);
			}
		else if (nPass == 2) {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
			else
				continue;
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
			else
				continue;
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
	RenderCoronaFaceList (gameData.render.faceIndex, 0);
	if (gameStates.app.bMultiThreaded)
		RenderCoronaFaceList (gameData.render.faceIndex + 1, 0);
	glFlush ();
	// then query how many samples (pixels) were rendered for each corona
	RenderCoronaFaceList (gameData.render.faceIndex, 1);
	if (gameStates.app.bMultiThreaded)
		RenderCoronaFaceList (gameData.render.faceIndex + 1, 1);
	}
else { //now find out how many fragments are rendered for each corona if geometry interferes
	gameStates.render.bQueryCoronas = 2;
	RenderCoronaFaceList (gameData.render.faceIndex, 2);
	if (gameStates.app.bMultiThreaded)
		RenderCoronaFaceList (gameData.render.faceIndex + 1, 2);
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
	tFace		*faceP;
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

static short RenderSegmentFaces (int nType, short nSegment, int bVertexArrays, int bDepthOnly, int bAutomap, int bHeadlight)
{
if (nSegment < 0)
	return 0;

	tSegFaces	*segFaceP = SEGFACES + nSegment;
	tFace		*faceP;
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
	gameData.render.lights.dynamic.shader.index [0][0].nActive = -1;
for (i = segFaceP->nFaces, faceP = segFaceP->pFaces; i; i--, faceP++) {
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
		tFace *faceP = FACES.faces.Buffer ();
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
#if SORT_FACES > 1
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

// -----------------------------------------------------------------------------------

inline int SegmentIsVisible (CSegment *segP)
{
if (gameStates.render.automap.bDisplay)
	return 1;
return RotateVertexList (8, segP->m_verts).ccAnd == 0;
}

//------------------------------------------------------------------------------

int SetupFace (short nSegment, short nSide, CSegment *segP, tFace *faceP, tFaceColor *pFaceColor, float *pfAlpha)
{
	ubyte	bTextured, bWall;
	int	nColor = 0;

#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if (FACE_IDX (faceP) == nDbgFace)
	nDbgFace = nDbgFace;
#endif
bWall = IS_WALL (faceP->nWall);
if (bWall) {
	faceP->widFlags = segP->IsDoorWay (nSide, NULL);
	if (!(faceP->widFlags & (WID_RENDER_FLAG | WID_RENDPAST_FLAG)))
		return -1;
	}
else
	faceP->widFlags = WID_RENDER_FLAG;
faceP->nCamera = IsMonitorFace (nSegment, nSide, 0);
bTextured = 1;
if (bWall)
	*pfAlpha = WallAlpha (nSegment, nSide, faceP->nWall, faceP->widFlags, faceP->nCamera >= 0, 
								 &pFaceColor [1].color, &nColor, &bTextured);
else
	*pfAlpha = 1;
faceP->bTextured = bTextured;
if (!gameData.app.nFrameCount) {
	if ((faceP->nSegColor = IsColoredSegFace (nSegment, nSide))) {
		pFaceColor [2].color = *ColoredSegmentColor (nSegment, nSide, faceP->nSegColor);
		if (faceP->nBaseTex < 0)
			*pfAlpha = pFaceColor [2].color.alpha;
		nColor = 2;
		}
	}
if ((*pfAlpha < 1) || ((nColor == 2) && (faceP->nBaseTex < 0)))
	faceP->bTransparent = 1;
return nColor;
}

//------------------------------------------------------------------------------

void UpdateSlidingFaces (void)
{
	tFace		*faceP;
	short			h, k, nOffset;
	tTexCoord2f	*texCoordP, *ovlTexCoordP;
	tUVL			*uvlP;

for (faceP = FACES.slidingFaces; faceP; faceP = faceP->nextSlidingFace) {
#if DBG
	if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		faceP = faceP;
#endif
	texCoordP = FACES.texCoord + faceP->nIndex;
	ovlTexCoordP = FACES.ovlTexCoord + faceP->nIndex;
	uvlP = SEGMENTS [faceP->nSegment].m_sides [faceP->nSide].m_uvls;
	nOffset = faceP->nType == SIDE_IS_TRI_13;
	if (gameStates.render.bTriangleMesh) {
		static short nTriVerts [2][6] = {{0,1,2,0,2,3},{0,1,3,1,2,3}};
		for (h = 0; h < 6; h++) {
			k = nTriVerts [nOffset][h];
			texCoordP [h].v.u = X2F (uvlP [k].u);
			texCoordP [h].v.v = X2F (uvlP [k].v);
			RotateTexCoord2f (ovlTexCoordP [h], texCoordP [h], faceP->nOvlOrient);
			}
		}
	else {
		for (h = 0; h < 4; h++) {
			texCoordP [h].v.u = X2F (uvlP [(h + nOffset) % 4].u);
			texCoordP [h].v.v = X2F (uvlP [(h + nOffset) % 4].v);
			RotateTexCoord2f (ovlTexCoordP [h], texCoordP [h], faceP->nOvlOrient);
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
	CFloatVector				buffers [VERTLIGHT_BUFFERS][VLBUF_SIZE];
	CFloatVector				colors [VLBUF_SIZE];
	tVertLightIndex	index [VLBUF_SIZE];
} tVertLightData;

static tVertLightData vld;

//------------------------------------------------------------------------------

void InitDynLighting (void)
{
if (gameStates.ogl.bVertexLighting)
	gameStates.ogl.bVertexLighting = gameData.render.lights.dynamic.fbo.Create (VLBUF_WIDTH, VLBUF_WIDTH, 2);
}

//------------------------------------------------------------------------------

void CloseDynLighting (void)
{
if (gameStates.ogl.bVertexLighting) {
	PrintLog ("unloading dynamic lighting buffers\n");
	gameData.render.lights.dynamic.fbo.Destroy ();
	}
}

//------------------------------------------------------------------------------

#if SHADER_VERTEX_LIGHTING
static char	*szTexNames [VERTLIGHT_BUFFERS] = {"vertPosTex", "vertNormTex", "lightPosTex", "lightColorTex"};
#endif

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
	CFloatVector	*vertPos, *vertNorm, *lightPos, lightColor, lightDir;
	CFloatVector	vReflect, vertColor = CFloatVector::ZERO;
	float		nType, radius, brightness, specular;
	float		attenuation, lightDist, NdotL, RdotE;
	int		i;

	static CFloatVector matAmbient = CFloatVector::Create(0.01f, 0.01f, 0.01f, 1.0f);
	//static CFloatVector matDiffuse = {{1.0f, 1.0f, 1.0f, 1.0f}};
	//static CFloatVector matSpecular = {{1.0f, 1.0f, 1.0f, 1.0f}};
	static float shininess = 96.0;

for (i = 0; i < vld.nLights; i++) {
	vertPos = vld.buffers [0] + i;
	vertNorm = vld.buffers [1] + i;
	lightPos = vld.buffers [2] + i;
	lightColor = vld.buffers [3][i];
	nType = (*vertNorm)[W];
	radius = (*lightPos)[W];
	brightness = lightColor[W];
	lightDir = *lightPos - *vertPos;
	lightDist = lightDir.Mag() / lightRange;
	CFloatVector::Normalize(lightDir);
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
		NdotL = CFloatVector::Dot (*vertNorm, lightDir);
		if (NdotL < 0.0f)
			NdotL = 0.0f;
		}	
	attenuation = lightDist / brightness;
	vertColor[R] = (matAmbient[R] + NdotL) * lightColor[R];
	vertColor[G] = (matAmbient[G] + NdotL) * lightColor[G];
	vertColor[B] = (matAmbient[B] + NdotL) * lightColor[B];
	if (NdotL > 0.0f) {
		vReflect = CFloatVector::Reflect(lightDir.Neg(), *vertNorm);
		CFloatVector::Normalize(vReflect);
		lightPos->Neg();
		CFloatVector::Normalize(*lightPos);
		RdotE = CFloatVector::Dot (vReflect, *lightPos);
		if (RdotE < 0.0f)
			RdotE = 0.0f;
		specular = (float) pow (RdotE, shininess);
		vertColor[R] += lightColor[R] * specular;
		vertColor[G] += lightColor[G] * specular;
		vertColor[B] += lightColor[B] * specular;
		}	
	vld.colors [i][R] = vertColor[R] / attenuation;
	vld.colors [i][G] = vertColor[G] / attenuation;
	vld.colors [i][B] = vertColor[B] / attenuation;
	}
}

//------------------------------------------------------------------------------

int RenderVertLightBuffers (void)
{
	tFaceColor	*vertColorP;
	tRgbaColorf	vertColor;
	CFloatVector		*pc;
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
#if DBG
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
OglSetReadBuffer (GL_COLOR_ATTACHMENT0_EXT, 1);
glReadPixels (0, 0, VLBUF_WIDTH, VLBUF_WIDTH, GL_RGBA, GL_FLOAT, vld.colors);
#endif

for (i = 0, pc = vld.colors; i < vld.nVertices; i++) {
	nVertex = vld.index [i].nVertex;
#if DBG
	if (nVertex == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	vertColor = vld.index [i].color;
	vertColor.red += gameData.render.color.ambient [nVertex].color.red;
	vertColor.green += gameData.render.color.ambient [nVertex].color.green;
	vertColor.blue += gameData.render.color.ambient [nVertex].color.blue;
	if (gameOpts->render.color.nSaturation == 2) {
		for (j = 0, nLights = vld.index [i].nLights; j < nLights; j++, pc++) {
			if (vertColor.red < (*pc)[R])
				vertColor.red = (*pc)[R];
			if (vertColor.green < (*pc)[G])
				vertColor.green = (*pc)[G];
			if (vertColor.blue < (*pc)[B])
				vertColor.blue = (*pc)[B];
			}
		}
	else {
		for (j = 0, nLights = vld.index [i].nLights; j < nLights; j++, pc++) {
			vertColor.red += (*pc)[R];
			vertColor.green += (*pc)[G];
			vertColor.blue += (*pc)[B];
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
	static const char	*szTexNames [VERTLIGHT_BUFFERS] = {"vertPosTex", "vertNormTex", "lightPosTex", "lightColorTex"};
#if 0
	static CFloatVector3	matSpecular = {{1.0f, 1.0f, 1.0f}};
#endif

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
	gameData.render.lights.dynamic.fbo.Enable ();
#if 1
	glUseProgramObject (hVertLightShader);
	for (i = 0; i < VL_SHADER_BUFFERS; i++) {
		glUniform1i (glGetUniformLocation (hVertLightShader, szTexNames [i]), i);
		if ((j = glGetError ())) {
			ComputeVertexLight (-1, 2, NULL);
			return 0;
			}
		}
	glUniform1f (glGetUniformLocation (hVertLightShader, "lightRange"), gameStates.ogl.fLightRange);
#endif
#if 0
	glUniform1f (glGetUniformLocation (hVertLightShader, "shininess"), 64.0f);
	glUniform3fv (glGetUniformLocation (hVertLightShader, "matAmbient"), 1, reinterpret_cast<GLfloat*> (&gameData.render.vertColor.matAmbient));
	glUniform3fv (glGetUniformLocation (hVertLightShader, "matDiffuse"), 1, reinterpret_cast<GLfloat*> (&gameData.render.vertColor.matDiffuse));
	glUniform3fv (glGetUniformLocation (hVertLightShader, "matSpecular"), 1, reinterpret_cast<GLfloat*> (&matSpecular));
#endif
	OglSetDrawBuffer (GL_COLOR_ATTACHMENT0_EXT, 0); 
	OglSetReadBuffer (GL_COLOR_ATTACHMENT0_EXT, 0);
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
	if ((j = glGetError ())) {
		ComputeVertexLight (-1, 2, NULL);
		return 0;
		}
	glDepthMask (0);
	}
else if (nState == 1) {
	CShaderLight	*psl;
	int				bSkipHeadlight = gameStates.ogl.bHeadlight && (gameData.render.lights.dynamic.headlights.nLights > 0) && !gameStates.render.nState;
	CFloatVector			vPos = gameData.segs.fVertices [nVertex],
						vNormal = gameData.segs.points [nVertex].p3_normal.vNormal;
		
	SetNearestVertexLights (-1, nVertex, NULL, 1, 0, 1, 0);
	if (!(h = gameData.render.lights.dynamic.shader.index [0][0].nActive))
		return 1;
	if (h > VLBUF_SIZE)
		h = VLBUF_SIZE;
	if (vld.nVertices >= VLBUF_SIZE)
		RenderVertLightBuffers ();
	else if (vld.nLights + h > VLBUF_SIZE)
		RenderVertLightBuffers ();
#if DBG
	if (nVertex == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	for (nLights = 0, i = vld.nLights, j = 0; j < h; i++, j++) {
		psl = gameData.render.lights.dynamic.shader.activeLights [0][j].psl;
		if (bSkipHeadlight && (psl->info.nType == 3))
			continue;
		vld.buffers [0][i] = vPos;
		vld.buffers [1][i] = vNormal;
		vld.buffers [2][i] = psl->vPosf [0];
		vld.buffers [3][i] = *(reinterpret_cast<CFloatVector*> (&psl->info.color));
		vld.buffers [0][i][W] = 1.0f;
		vld.buffers [1][i][W] = (psl->info.nType < 2) ? 1.0f : 0.0f;
		vld.buffers [2][i][W] = psl->info.fRad;
		vld.buffers [3][i][W] = psl->info.fBrightness;
		nLights++;
		}
	if (nLights) {
		vld.index [vld.nVertices].nVertex = nVertex;
		vld.index [vld.nVertices].nLights = nLights;
		vld.index [vld.nVertices].color = colorP->color;
		vld.nVertices++;
		vld.nLights += nLights;
		}
	//gameData.render.lights.dynamic.shader.index [0][0].nActive = gameData.render.lights.dynamic.shader.iVertexLights [0];
	}	
else if (nState == 2) {
	RenderVertLightBuffers ();
	glUseProgramObject (0);
	gameData.render.lights.dynamic.fbo.Disable ();
	glMatrixMode (GL_PROJECTION);    
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);                         
	glPopMatrix ();
	glPopAttrib ();
	OglSetDrawBuffer (GL_BACK, 1);
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
	OglSetDrawBuffer (GL_BACK, 1);
	}
return 1;
}

//------------------------------------------------------------------------------

void ComputeDynamicFaceLight (int nStart, int nEnd, int nThread)
{
PROF_START
	tFace		*faceP;
	tRgbaColorf	*pc;
	tFaceColor	c, faceColor [3] = {{{0,0,0,1},1},{{0,0,0,0},1},{{0,0,0,1},1}};
#if 0
	ubyte			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	short			nVertex, nSegment, nSide;
	float			fAlpha;
	int			h, i, nStep, nColor, nLights = 0,
					bVertexLight = gameStates.render.bPerPixelLighting != 2,
					bLightmaps = lightmapManager.HaveLightmaps ();
	static		tFaceColor brightColor = {{1,1,1,1},1};

#if SORT_FACES > 1
ResetFaceList (nThread);
#endif
//memset (&gameData.render.lights.dynamic.shader.index, 0, sizeof (gameData.render.lights.dynamic.shader.index));
gameStates.ogl.bUseTransform = 1;
gameStates.render.nState = 0;
#	if SHADER_VERTEX_LIGHTING
if (gameStates.ogl.bVertexLighting)
	gameStates.ogl.bVertexLighting = ComputeVertexLight (-1, 0, NULL);
#endif
for (i = nStart, nStep = (nStart > nEnd) ? -1 : 1; i != nEnd; i += nStep) {
	faceP = FACES.faces + i;
	nSegment = faceP->nSegment;
	nSide = faceP->nSide;
	if (bVertexLight && !gameStates.render.bFullBright)
		nLights = SetNearestSegmentLights (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in SetNearestVertexLights ())
	if (faceP->bSparks && gameOpts->render.effects.bEnergySparks) {
		faceP->bVisible = 0;
		continue;
		}
	if (!(faceP->bVisible = FaceIsVisible (nSegment, nSide)))
		continue;
	if (0 > (nColor = SetupFace (nSegment, nSide, SEGMENTS + nSegment, faceP, faceColor, &fAlpha))) {
		faceP->bVisible = 0;
		continue;
		}
#if SORT_FACES > 1
	AddFaceListItem (faceP, nThread);
#endif
	faceP->color = faceColor [nColor].color;
	pc = FACES.color + faceP->nIndex;
	for (h = 0; h < 4; h++, pc++) {
		if (gameStates.render.bFullBright) 
			*pc = nColor ? faceColor [nColor].color : brightColor.color;
		else {
			if (bLightmaps) {
				c = faceColor [nColor];
				if (nColor)
					*pc = c.color;
				}
			else {
				nVertex = faceP->index [h];
#if DBG
				if (nVertex == nDbgVertex)
					nDbgVertex = nDbgVertex;
#endif
				if (gameStates.render.bPerPixelLighting == 2) 
					*pc = gameData.render.color.ambient [nVertex].color;
				else {
					c.color = gameData.render.color.ambient [nVertex].color;
					tFaceColor *pvc = gameData.render.color.vertices + nVertex;
					if (pvc->index != gameStates.render.nFrameFlipFlop + 1) {
						if (nLights + gameData.render.lights.dynamic.variableVertLights [nVertex] == 0) {
							pvc->color = c.color;
							pvc->index = gameStates.render.nFrameFlipFlop + 1;
							}
						else {
#if DBG
							if (nVertex == nDbgVertex)
								nDbgVertex = nDbgVertex;
#endif
							G3VertexColor(gameData.segs.points[nVertex].p3_normal.vNormal.V3(), 
							              gameData.segs.fVertices[nVertex].V3(), nVertex, 
							              NULL, &c, 1, 0, nThread);
							gameData.render.lights.dynamic.shader.index [0][nThread] = gameData.render.lights.dynamic.shader.index [1][nThread];
							ResetNearestVertexLights (nVertex, nThread);
							}
#if DBG
						if (nVertex == nDbgVertex)
							nVertex = nVertex;
#endif
						}
					*pc = pvc->color;
					}
				}
			if (nColor == 1) {
				c = faceColor [nColor];
				pc->red *= c.color.red;
				pc->blue *= c.color.green;
				pc->green *= c.color.blue;
				}
			pc->alpha = fAlpha;
			}
			}
	gameData.render.lights.dynamic.material.bValid = 0;
	}
PROF_END(ptLighting)
gameStates.ogl.bUseTransform = 0;
}

//------------------------------------------------------------------------------

void ComputeDynamicQuadLight (int nStart, int nEnd, int nThread)
{
PROF_START
	CSegment		*segP;
	tSegFaces	*segFaceP;
	tFace		*faceP;
	tRgbaColorf	*pc;
	tFaceColor	c, faceColor [3] = {{{0,0,0,1},1},{{0,0,0,0},1},{{0,0,0,1},1}};
#if 0
	ubyte			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	short			nVertex, nSegment, nSide;
	float			fAlpha;
	int			h, i, j, nColor, nLights = 0,
					nStep = nStart ? -1 : 1,
					bVertexLight = gameStates.render.bPerPixelLighting != 2,
					bLightmaps = lightmapManager.HaveLightmaps ();
	static		tFaceColor brightColor = {{1,1,1,1},1};

#if SORT_FACES > 1
ResetFaceList (nThread);
#endif
//memset (&gameData.render.lights.dynamic.shader.index, 0, sizeof (gameData.render.lights.dynamic.shader.index));
gameStates.ogl.bUseTransform = 1;
gameStates.render.nState = 0;
#	if SHADER_VERTEX_LIGHTING
if (gameStates.ogl.bVertexLighting)
	gameStates.ogl.bVertexLighting = ComputeVertexLight (-1, 0, NULL);
#endif
for (i = nStart; i != nEnd; i += nStep) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	segP = SEGMENTS + nSegment;
	segFaceP = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [i] = -gameData.render.mine.nSegRenderList [i];
		continue;
		}
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (bVertexLight && !gameStates.render.bFullBright)
		nLights = SetNearestSegmentLights (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in SetNearestVertexLights ())
	for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++) {
		nSide = faceP->nSide;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
			nSegment = nSegment;
			if (gameData.render.lights.dynamic.shader.index [0][nThread].nActive)
				nSegment = nSegment;
			}
#endif
		if (faceP->bSparks && gameOpts->render.effects.bEnergySparks) {
			faceP->bVisible = 0;
			continue;
			}
		if (!(faceP->bVisible = FaceIsVisible (nSegment, nSide)))
			continue;
		if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
			faceP->bVisible = 0;
			continue;
			}
#if SORT_FACES > 1
		if (!AddFaceListItem (faceP, nThread))
			continue;
#endif
		faceP->color = faceColor [nColor].color;
//			SetDynLightMaterial (nSegment, faceP->nSide, -1);
		pc = FACES.color + faceP->nIndex;
		for (h = 0; h < 4; h++, pc++) {
			if (gameStates.render.bFullBright) 
				*pc = nColor ? faceColor [nColor].color : brightColor.color;
			else {
				c = faceColor [nColor];
				if (nColor)
					*pc = c.color;
				if (!bLightmaps) {
					nVertex = faceP->index [h];
#if DBG
					if (nVertex == nDbgVertex)
						nDbgVertex = nDbgVertex;
#endif
					if (gameStates.render.bPerPixelLighting == 2) 
						*pc = gameData.render.color.ambient [nVertex].color;
					else {
						tFaceColor *pvc = gameData.render.color.vertices + nVertex;
						if (pvc->index != gameStates.render.nFrameFlipFlop + 1) {
#if DBG
							if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) 
								nSegment = nSegment;
#endif
							if (nLights + gameData.render.lights.dynamic.variableVertLights [nVertex] == 0) {
								pvc->color.red = c.color.red + gameData.render.color.ambient [nVertex].color.red;
								pvc->color.green = c.color.green + gameData.render.color.ambient [nVertex].color.green;
								pvc->color.blue = c.color.blue + gameData.render.color.ambient [nVertex].color.blue;
								pvc->index = gameStates.render.nFrameFlipFlop + 1;
								}
							else {
#if DBG
								if (nVertex == nDbgVertex)
									nDbgVertex = nDbgVertex;
#endif
								G3VertexColor (gameData.segs.points [nVertex].p3_normal.vNormal.V3(), 
													gameData.segs.fVertices [nVertex].V3(), nVertex, 
													NULL, &c, 1, 0, nThread);
								gameData.render.lights.dynamic.shader.index [0][nThread] = gameData.render.lights.dynamic.shader.index [1][nThread];
								ResetNearestVertexLights (nVertex, nThread);
								}
#if DBG
							if (nVertex == nDbgVertex)
								nVertex = nVertex;
#endif
							}
						*pc = pvc->color;
						}
					}
				if (nColor == 1) {
					c = faceColor [nColor];
					pc->red *= c.color.red;
					pc->blue *= c.color.green;
					pc->green *= c.color.blue;
					}
				pc->alpha = fAlpha;
				}
			}
		gameData.render.lights.dynamic.material.bValid = 0;
		}
	}
#	if SHADER_VERTEX_LIGHTING
if (gameStates.ogl.bVertexLighting)
	ComputeVertexLight (-1, 2, NULL);
#endif
PROF_END(ptLighting)
gameStates.ogl.bUseTransform = 0;
}

//------------------------------------------------------------------------------

void ComputeDynamicTriangleLight (int nStart, int nEnd, int nThread)
{
PROF_START
	CSegment		*segP;
	tSegFaces	*segFaceP;
	tFace		*faceP;
	grsTriangle	*triP;
	tRgbaColorf	*pc;
	tFaceColor	c, faceColor [3] = {{{0,0,0,1},1},{{0,0,0,0},1},{{0,0,0,1},1}};
#if 0
	ubyte			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	short			nVertex, nSegment, nSide;
	float			fAlpha;
	int			h, i, j, k, nIndex, nColor, nLights = 0,
					nStep = nStart ? -1 : 1;
	bool			bNeedLight = !gameStates.render.bFullBright && (gameStates.render.bPerPixelLighting != 2);

	static		tFaceColor brightColor = {{1,1,1,1},1};

#if SORT_FACES > 1
ResetFaceList (nThread);
#endif
memset (&gameData.render.lights.dynamic.shader.index, 0, sizeof (gameData.render.lights.dynamic.shader.index));
gameStates.ogl.bUseTransform = 1;
gameStates.render.nState = 0;
#	if SHADER_VERTEX_LIGHTING
if (gameStates.ogl.bVertexLighting)
	gameStates.ogl.bVertexLighting = ComputeVertexLight (-1, 0, NULL);
#endif
for (i = nStart; i != nEnd; i += nStep) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	segP = SEGMENTS + nSegment;
	segFaceP = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [i] = -gameData.render.mine.nSegRenderList [i] - 1;
		continue;
		}
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (bNeedLight)
		nLights = SetNearestSegmentLights (nSegment, -1, 0, 0, nThread);	//only get light emitting objects here (variable geometry lights are caught in SetNearestVertexLights ())
	for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++) {
		nSide = faceP->nSide;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
			nSegment = nSegment;
			if (gameData.render.lights.dynamic.shader.index [0][nThread].nActive)
				nSegment = nSegment;
			}
#endif
		if (!(faceP->bVisible = FaceIsVisible (nSegment, nSide)))
			continue;
		if (faceP->bSparks && gameOpts->render.effects.bEnergySparks) {
			faceP->bVisible = 0;
			continue;
			}
		if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
			faceP->bVisible = 0;
			continue;
			}
#if SORT_FACES > 1
		if (!AddFaceListItem (faceP, nThread))
			continue;
#endif
		faceP->color = faceColor [nColor].color;
#if 1
		if (!bNeedLight && faceP->bHasColor)
			continue;
		faceP->bHasColor = 1;
#endif
//			SetDynLightMaterial (nSegment, faceP->nSide, -1);
		for (k = faceP->nTris, triP = FACES.tris + faceP->nTriIndex; k; k--, triP++) {
			nIndex = triP->nIndex;
			pc = FACES.color + nIndex;
			for (h = 0; h < 3; h++, pc++, nIndex++) {
				if (gameStates.render.bFullBright) 
					*pc = nColor ? faceColor [nColor].color : brightColor.color;
				else {
					c = faceColor [nColor];
					if (nColor)
						*pc = c.color;
					nVertex = triP->index [h];
#if DBG
					if (nVertex == nDbgVertex)
						nDbgVertex = nDbgVertex;
#endif
					if (gameStates.render.bPerPixelLighting == 2)
						*pc = gameData.render.color.ambient [nVertex].color;
					else {
						tFaceColor *pvc = gameData.render.color.vertices + nVertex;
						if (pvc->index != gameStates.render.nFrameFlipFlop + 1) {
							if (nLights + gameData.render.lights.dynamic.variableVertLights [nVertex] == 0) {
								pvc->color.red = c.color.red + gameData.render.color.ambient [nVertex].color.red;
								pvc->color.green = c.color.green + gameData.render.color.ambient [nVertex].color.green;
								pvc->color.blue = c.color.blue + gameData.render.color.ambient [nVertex].color.blue;
								pvc->index = gameStates.render.nFrameFlipFlop + 1;
								}
							else {
								G3VertexColor (FACES.normals + nIndex, 
													FACES.vertices + nIndex, nVertex, 
													NULL, &c, 1, 0, nThread);
								gameData.render.lights.dynamic.shader.index [0][nThread] = gameData.render.lights.dynamic.shader.index [1][nThread];
								ResetNearestVertexLights (nVertex, nThread);
								}
#if DBG
							if (nVertex == nDbgVertex)
								nVertex = nVertex;
#endif
							}
						*pc = pvc->color;
						}
					if (nColor) {
						c = faceColor [nColor];
						pc->red *= c.color.red;
						pc->blue *= c.color.green;
						pc->green *= c.color.blue;
						}
					pc->alpha = fAlpha;
					}
				}
			}
		gameData.render.lights.dynamic.material.bValid = 0;
		}
	}
#	if SHADER_VERTEX_LIGHTING
if (gameStates.ogl.bVertexLighting)
	ComputeVertexLight (-1, 2, NULL);
#endif
PROF_END(ptLighting)
gameStates.ogl.bUseTransform = 0;
}

//------------------------------------------------------------------------------

void ComputeStaticFaceLight (int nStart, int nEnd, int nThread)
{
	CSegment		*segP;
	tSegFaces	*segFaceP;
	tFace		*faceP;
	tRgbaColorf	*pc;
	tFaceColor	c, faceColor [3] = {{{0,0,0,1},1},{{0,0,0,0},1},{{0,0,0,1},1}};
#if 0
	ubyte			nThreadFlags [3] = {1 << nThread, 1 << !nThread, ~(1 << nThread)};
#endif
	short			nVertex, nSegment, nSide;
	fix			xLight;
	float			fAlpha;
	tUVL			*uvlP;
	int			h, i, j, uvi, nColor, 
					nStep = nStart ? -1 : 1;

	static		tFaceColor brightColor = {{1,1,1,1},1};

#if SORT_FACES > 1
ResetFaceList (nThread);
#endif
gameStates.ogl.bUseTransform = 1;
gameStates.render.nState = 0;
for (i = nStart; i != nEnd; i += nStep) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
		continue;
	segP = SEGMENTS + nSegment;
	segFaceP = SEGFACES + nSegment;
	if (!(/*gameStates.app.bMultiThreaded ||*/ SegmentIsVisible (segP))) {
		gameData.render.mine.nSegRenderList [i] = -gameData.render.mine.nSegRenderList [i] - 1;
		continue;
		}
	for (j = segFaceP->nFaces, faceP = segFaceP->pFaces; j; j--, faceP++) {
		nSide = faceP->nSide;
		if (!(faceP->bVisible = FaceIsVisible (nSegment, nSide)))
			continue;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			nSegment = nSegment;
#endif
		if (faceP->bSparks && gameOpts->render.effects.bEnergySparks) {
			faceP->bVisible = 0;
			continue;
			}
		if (0 > (nColor = SetupFace (nSegment, nSide, segP, faceP, faceColor, &fAlpha))) {
			faceP->bVisible = 0;
			continue;
			}
#if SORT_FACES > 1
		if (!AddFaceListItem (faceP, nThread))
			continue;
#endif
		faceP->color = faceColor [nColor].color;
		pc = FACES.color + faceP->nIndex;
		uvlP = segP->m_sides [nSide].m_uvls;
		for (h = 0, uvi = (segP->m_sides [nSide].m_nType == SIDE_IS_TRI_13); h < 4; h++, pc++, uvi++) {
			if (gameStates.render.bFullBright) 
				*pc = nColor ? faceColor [nColor].color : brightColor.color;
			else {
				c = faceColor [nColor];
				nVertex = faceP->index [h];
#if DBG
				if (nVertex == nDbgVertex)
					nDbgVertex = nDbgVertex;
#endif
				SetVertexColor (nVertex, &c);
				xLight = SetVertexLight (nSegment, nSide, nVertex, &c, uvlP [uvi % 4].l);
				AdjustVertexColor (NULL, &c, xLight);
				}
			*pc = c.color;
			pc->alpha = fAlpha;
			}
		}
	}
gameStates.ogl.bUseTransform = 0;
}

//------------------------------------------------------------------------------

int CountRenderFaces (void)
{
	CSegment		*segP;
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
	tFace		*faceP;
	short			nSegment;
	int			h, i, j, n;

nRenderVertices = 0;
for (h = i = 0; h < gameData.render.mine.nRenderSegs; h++) {
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
	tFace		*faceP;
	short			nSegment;
	int			h, i, j;

nRenderVertices = 0;
for (h = 0; h < gameData.render.mine.nRenderSegs; h++) {
	if (0 > (nSegment = gameData.render.mine.nSegRenderList [h]))
		continue;
	segFaceP = SEGFACES + gameData.render.mine.nSegRenderList [h];
	for (i = segFaceP->nFaces, faceP = segFaceP->pFaces; i; i--, faceP++)
		for (j = 0; j < 4; j++)
			FACES.color [j] = gameData.render.color.vertices [faceP->index [j]].color;
	}
}

//------------------------------------------------------------------------------

void RenderMineObjects (int nType)
{
	int	nListPos, nSegLights = 0;
	short	nSegment;

#if DBG
if (!gameOpts->render.debug.bObjects)
	return;
#endif
if ((nType < 1) || (nType > 2))
	return;
gameStates.render.nState = 1;
for (nListPos = gameData.render.mine.nRenderSegs; nListPos; ) {
	nSegment = gameData.render.mine.nSegRenderList [--nListPos];
	if (nSegment < 0) {
		if (nSegment == -0x7fff)
			continue;
		nSegment = -nSegment - 1;
		}
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (0 > gameData.render.mine.renderObjs.ref [nSegment]) 
		continue;
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (nType == 1) {	// render opaque objects
#if DBG
		if (nSegment == nDbgSeg)
			nSegment = nSegment;
#endif
		if (gameStates.render.bUseDynLight && !gameStates.render.bQueryCoronas) {
			nSegLights = SetNearestSegmentLights (nSegment, -1, 0, 1, 0);
			SetNearestStaticLights (nSegment, 1, 1, 0);
			gameStates.render.bApplyDynLight = gameOpts->ogl.bLightObjects;
			}
		else
			gameStates.render.bApplyDynLight = 0;
		RenderObjList (nListPos, gameStates.render.nWindow);
		if (gameStates.render.bUseDynLight && !gameStates.render.bQueryCoronas) {
			ResetNearestStaticLights (nSegment, 0);
			}
		gameStates.render.bApplyDynLight = gameStates.render.nLightingMethod != 0;
		//gameData.render.lights.dynamic.shader.index [0][0].nActive = gameData.render.lights.dynamic.shader.iStaticLights [0];
		}
	else if (nType == 2)	// render objects containing transparency, like explosions
		RenderObjList (nListPos, gameStates.render.nWindow);
	}	
gameStates.render.nState = 0;
}

//------------------------------------------------------------------------------

const char *vertLightFS = 
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
	"	lightDir = Normalize (lightDir);\r\n" \
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
	"		RdotE = max (dot (Normalize (Reflect (-lightDir, vertNorm)), Normalize (-lightPos)), 0.0);\r\n" \
	"		vertColor += lightColor * pow (RdotE, shininess);\r\n" \
	"		}\r\n" \
	"  gl_FragColor = vec4 (vertColor / attenuation, 1.0);\r\n" \
	"	}";

const char *vertLightVS = 
	"void main(void){" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;"\
	"/*gl_TexCoord [1] = gl_MultiTexCoord1;"\
	"gl_TexCoord [2] = gl_MultiTexCoord2;"\
	"gl_TexCoord [3] = gl_MultiTexCoord3;*/"\
	"gl_Position = ftransform() /*gl_ModelViewProjectionMatrix * gl_Vertex*/;" \
	"gl_FrontColor = gl_Color;}";

//-------------------------------------------------------------------------

void InitVertLightShader (void)
{
#if SHADER_VERTEX_LIGHTING
	int	bOk;
#endif

if (!gameStates.ogl.bVertexLighting)
	return;
gameStates.ogl.bVertexLighting = 0;
#if SHADER_VERTEX_LIGHTING
if (gameStates.ogl.bRender2TextureOk && gameStates.ogl.bShadersOk && RENDERPATH) {
	PrintLog ("building vertex lighting shader program\n");
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
