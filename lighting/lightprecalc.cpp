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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"

#include "inferno.h"
#include "text.h"
#include "gamemine.h"
#include "error.h"
#include "lightmap.h"
#include "render.h"
#include "gameseg.h"
#include "menu.h"
#include "key.h"
#include "u_mem.h"
#include "paging.h"
#include "light.h"
#include "dynlight.h"

#ifdef EDITOR
#	include "editor/editor.h"
#endif

//------------------------------------------------------------------------------

#define LIGHT_DATA_VERSION 10

#define	VERTVIS(_nSegment, _nVertex) \
	(gameData.segs.bVertVis.Buffer () ? gameData.segs.bVertVis [(_nSegment) * VERTVIS_FLAGS + ((_nVertex) >> 3)] & (1 << ((_nVertex) & 7)) : 0)

//------------------------------------------------------------------------------

typedef struct tLightDataHeader {
	int	nVersion;
	int	nSegments;
	int	nVertices;
	int	nLights;
	int	nMaxLightRange;
	int	nMethod;
	int	bPerPixelLighting;
	} tLightDataHeader;

//------------------------------------------------------------------------------

static int loadIdx = 0;
static int loadOp = 0;

//------------------------------------------------------------------------------

typedef struct tLightDist {
	int		nIndex;
	int		nDist;
} tLightDist;

void QSortLightDist (tLightDist *pDist, int left, int right)
{
	int			l = left,
					r = right,
					m = pDist [(l + r) / 2].nDist;
	tLightDist	h;

do {
	while (pDist [l].nDist < m)
		l++;
	while (pDist [r].nDist > m)
		r--;
	if (l <= r) {
		if (l < r) {
			h = pDist [l];
			pDist [l] = pDist [r];
			pDist [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortLightDist (pDist, l, right);
if (left < r)
	QSortLightDist (pDist, left, r);
}

//------------------------------------------------------------------------------

static inline int IsLightVert (int nVertex, tFace *faceP)
{
ushort *pv = (gameStates.render.bTriangleMesh ? faceP->triIndex : faceP->index);
for (int i = faceP->nVerts; i; i--, pv++)
	if (*pv == (ushort) nVertex)
		return 1;
return 0;
}

//------------------------------------------------------------------------------

int ComputeNearestSegmentLights (int i)
{
	CSegment				*segP;
	CDynLight			*pl;
	int					h, j, k, l, m, n, nMaxLights;
	CFixVector			center;
	struct tLightDist	*pDists;

PrintLog ("computing nearest segment lights (%d)\n", i);
if (!gameData.render.lights.dynamic.nLights)
	return 0;
if (!(pDists = new tLightDist [gameData.render.lights.dynamic.nLights])) {
	gameOpts->render.nLightingMethod = 0;
	gameData.render.shadows.nLights = 0;
	return 0;
	}
nMaxLights = MAX_NEAREST_LIGHTS;
if (gameStates.app.bMultiThreaded)
	j = i ? gameData.segs.nSegments : gameData.segs.nSegments / 2;
else
	INIT_PROGRESS_LOOP (i, j, gameData.segs.nSegments);
for (segP = SEGMENTS + i; i < j; i++, segP++) {
	center = segP->Center ();
	pl = gameData.render.lights.dynamic.lights;
	for (l = n = 0; l < gameData.render.lights.dynamic.nLights; l++, pl++) {
		m = (pl->info.nSegment < 0) ? OBJECTS [pl->info.nObject].info.nSegment : pl->info.nSegment;
		if (!SEGVIS (m, i))
			continue;
		h = (int) (CFixVector::Dist (center, pl->info.vPos) - F2X (pl->info.fRad) / 10.0f);
		if (h > MAX_LIGHT_RANGE * pl->info.fRange)
			continue;
		pDists [n].nDist = h;
		pDists [n++].nIndex = l;
		}
	if (n)
		QSortLightDist (pDists, 0, n - 1);
	h = (nMaxLights < n) ? nMaxLights : n;
	k = i * MAX_NEAREST_LIGHTS;
	for (l = 0; l < h; l++)
		gameData.render.lights.dynamic.nearestSegLights [k + l] = pDists [l].nIndex;
	for (; l < MAX_NEAREST_LIGHTS; l++)
		gameData.render.lights.dynamic.nearestSegLights [k + l] = -1;
	}
delete[] pDists;
return 1;
}

//------------------------------------------------------------------------------

#if DBG
extern int nDbgVertex;
#endif

int ComputeNearestVertexLights (int nVertex)
{
	CFixVector			*vertP;
	CDynLight			*pl;
	CSide					*sideP;
	int					h, j, k, l, n, nMaxLights;
	CFixVector			vLightToVert;
	struct tLightDist	*pDists;

PrintLog ("computing nearest vertex lights (%d)\n", nVertex);
if (!gameData.render.lights.dynamic.nLights)
	return 0;
if (!(pDists = new tLightDist [gameData.render.lights.dynamic.nLights])) {
	gameOpts->render.nLightingMethod = 0;
	gameData.render.shadows.nLights = 0;
	return 0;
	}
#if DBG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
nMaxLights = MAX_NEAREST_LIGHTS;
if (gameStates.app.bMultiThreaded)
	j = nVertex ? gameData.segs.nVertices : gameData.segs.nVertices / 2;
else
	INIT_PROGRESS_LOOP (nVertex, j, gameData.segs.nVertices);
for (vertP = gameData.segs.vertices + nVertex; nVertex < j; nVertex++, vertP++) {
#if DBG
	if (nVertex == nDbgVertex)
		nVertex = nVertex;
#endif
	pl = gameData.render.lights.dynamic.lights;
	for (l = n = 0; l < gameData.render.lights.dynamic.nLights; l++, pl++) {
#if DBG
		if (pl->info.nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		if (IsLightVert (nVertex, pl->info.faceP))
			h = 0;
		else {
			h = (pl->info.nSegment < 0) ? OBJECTS [pl->info.nObject].info.nSegment : pl->info.nSegment;
			if (!VERTVIS (h, nVertex))
				continue;
			vLightToVert = *vertP - pl->info.vPos;
			h = CFixVector::Normalize(vLightToVert) - (int) (pl->info.fRad * 6553.6f);
			if (h > MAX_LIGHT_RANGE * pl->info.fRange)
				continue;
			if ((pl->info.nSegment >= 0) && (pl->info.nSide >= 0)) {
				sideP = SEGMENTS [pl->info.nSegment].m_sides + pl->info.nSide;
				if ((CFixVector::Dot (sideP->m_normals[0], vLightToVert) < -I2X (1) / 6) &&
					 ((sideP->m_nType == SIDE_IS_QUAD) || (CFixVector::Dot (sideP->m_normals[1], vLightToVert) < -I2X (1) / 6)))
					continue;
				}
			}
		pDists [n].nDist = h;
		pDists [n].nIndex = l;
		n++;
		}
	if (n)
		QSortLightDist (pDists, 0, n - 1);
	h = (nMaxLights < n) ? nMaxLights : n;
	k = nVertex * MAX_NEAREST_LIGHTS;
	for (l = 0; l < h; l++)
		gameData.render.lights.dynamic.nearestVertLights [k + l] = pDists [l].nIndex;
	for (; l < MAX_NEAREST_LIGHTS; l++)
		gameData.render.lights.dynamic.nearestVertLights [k + l] = -1;
	}
delete[] pDists;
return 1;
}

//------------------------------------------------------------------------------

inline int SetVertVis (short nSegment, short nVertex, ubyte b)
{
	static int bSemaphore = 0;

if (bSemaphore)
	return 0;
bSemaphore = 1;
if (gameData.segs.bVertVis.Buffer ())
	gameData.segs.bVertVis [nSegment * VERTVIS_FLAGS + (nVertex >> 3)] |= (1 << (nVertex & 7));
bSemaphore = 0;
return 1;
}

//------------------------------------------------------------------------------

inline int SetSegVis (short nSrcSeg, short nDestSeg)
{
	static int bSemaphore = 0;

if (bSemaphore)
	return 0;
bSemaphore = 1;
gameData.segs.bSegVis [nSrcSeg * SEGVIS_FLAGS + (nDestSeg >> 3)] |= (1 << (nDestSeg & 7));
bSemaphore = 0;
return 1;
}

//------------------------------------------------------------------------------

static void SetSegAndVertVis (short nStartSeg, short nSegment)
{
if (SEGVIS (nStartSeg, nSegment))
	return;
while (!SetSegVis (nStartSeg, nSegment))
	;

	tSegFaces*		segFaceP = SEGFACES + nSegment;
	tFace*			faceP;
	tFaceTriangle*	triP;
	int				i, nFaces, nTris;

for (nFaces = segFaceP->nFaces, faceP = segFaceP->faceP; nFaces; nFaces--, faceP++) {
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nSegment = nSegment;
#endif
	if (gameStates.render.bTriangleMesh) {
		for (nTris = faceP->nTris, triP = FACES.tris + faceP->nTriIndex; nTris; nTris--, triP++)
			for (i = 0; i < 3; i++)
				while (!SetVertVis (nStartSeg, triP->index [i], 1))
					;
		}
	else {
		for (i = 0; i < 4; i++)
			while (!SetVertVis (nStartSeg, faceP->index [i], 1))
				;
		}
	}
}

//------------------------------------------------------------------------------
// Check segment to segment visibility by calling the renderer's visibility culling routine
// Do this for each side of the current segment, using the side Normal(s) as forward vector
// of the viewer

void ComputeSingleSegmentVisibility (short nStartSeg)
{
	CSegment		*segP, *childP;
	CSide			*sideP;
	short			nSegment, nSide, nChildSeg, nChildSide, i;
	CFixVector	vNormal;
	CAngleVector	vAngles;
	CObject		viewer;

//PrintLog ("computing visibility of segment %d\n", nStartSeg);
gameStates.ogl.bUseTransform = 1;
#if DBG
if (nStartSeg == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
gameData.objs.viewerP = &viewer;
viewer.info.nSegment = nStartSeg;
viewer.info.position.vPos = SEGMENTS [nStartSeg].Center ();
segP = SEGMENTS + nStartSeg;
for (sideP = segP->m_sides, nSide = 0; nSide < 6; nSide++, sideP++) {
#if 1
	if (gameStates.render.bPerPixelLighting) {
		if (0 <= (nChildSeg = segP->m_children [nSide])) {
			while (!SetSegVis (nStartSeg, nChildSeg))
				;
			childP = SEGMENTS + nChildSeg;
			for (nChildSide = 0; nChildSide < 6; nChildSide++) {
				if (0 <= (nSegment = childP->m_children [nSide])) {
					while (!SetSegVis (nChildSeg, nSegment))
						;
					}
				}
			}
		}
	vNormal = sideP->m_normals[0] + sideP->m_normals[1];
	vNormal *= (-I2X (1) / 2);
	vAngles = vNormal.ToAnglesVec();
	viewer.info.position.mOrient = CFixMatrix::Create(vAngles);
	G3StartFrame(0, 0);
	RenderStartFrame();
	G3SetViewMatrix (viewer.info.position.vPos, viewer.info.position.mOrient, gameStates.render.xZoom, 1);
#endif
	BuildRenderSegList (nStartSeg, 0);
	G3EndFrame ();
	//PrintLog ("   flagging visible segments\n");
	for (i = 0; i < gameData.render.mine.nRenderSegs; i++) {
		if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
			continue;
#if DBG
		if (nSegment >= gameData.segs.nSegments)
			continue;
#endif
		SetSegAndVertVis (nStartSeg, nSegment);
		SetSegAndVertVis (nSegment, nStartSeg);
		}
	}
gameStates.ogl.bUseTransform = 0;
}

//------------------------------------------------------------------------------

void ComputeSegmentVisibility (int startI)
{
	int			i, endI;

PrintLog ("computing segment visibility (%d)\n", startI);
if (startI <= 0) {
	i = sizeof (gameData.segs.bVertVis [0]) * gameData.segs.nVertices * VERTVIS_FLAGS;
	if (!gameData.segs.bVertVis.Create (i))
		return;
	gameData.segs.bVertVis.Clear ();
	}
else if (!gameData.segs.bVertVis)
	return;
if (gameStates.app.bMultiThreaded) {
	endI = startI ? gameData.segs.nSegments : gameData.segs.nSegments / 2;
	}
else
	INIT_PROGRESS_LOOP (startI, endI, gameData.segs.nSegments);
if (startI < 0)
	startI = 0;
// every segment can see itself and its neighbours
for (i = startI; i < endI; i++)
	ComputeSingleSegmentVisibility (i);
}

//------------------------------------------------------------------------------

static int SortLightsPoll (CMenu& menu, int& key, int nCurItem)
{
paletteManager.LoadEffect  ();
if (loadOp == 0) {
	ComputeSegmentVisibility (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 1;
		}
	}
else if (loadOp == 1) {
	ComputeNearestSegmentLights (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 2;
		}
	}
else if (loadOp == 2) {
	ComputeNearestVertexLights (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nVertices) {
		loadIdx = 0;
		loadOp = 3;
		}
	}
if (loadOp == 3) {
	key = -2;
	paletteManager.LoadEffect  ();
	return nCurItem;
	}
menu [0].m_value++;
menu [0].m_bRebuild = 1;
key = 0;
paletteManager.LoadEffect  ();
return nCurItem;
}

//------------------------------------------------------------------------------

int SortLightsGaugeSize (void)
{
if (gameStates.app.bNostalgia)
	return 0;
if (gameStates.app.bMultiThreaded)
	return 0;
if (!(gameOpts->render.nLightingMethod ||
	  (gameStates.render.bAmbientColor && !gameStates.render.bColored) ||
	   gameStates.app.bEnableShadows))
	return 0;
return
#if !SHADOWS
	(!SHOW_DYN_LIGHT && gameStates.app.bD2XLevel) ? 0 :
#endif
	PROGRESS_STEPS (gameData.segs.nSegments) * 2 +
	PROGRESS_STEPS (gameData.segs.nVertices)
	;
}

//------------------------------------------------------------------------------

char *LightDataFilename (char *pszFilename, int nLevel)
{
return GameDataFilename (pszFilename, "light", nLevel, -1);
}

//------------------------------------------------------------------------------

int LoadLightData (int nLevel)
{
	CFile					cf;
	tLightDataHeader	ldh;
	int					bOk;
	char					szFilename [FILENAME_LEN];

if (!gameStates.app.bCacheLights)
	return 0;
if (!cf.Open (LightDataFilename (szFilename, nLevel), gameFolders.szCacheDir, "rb", 0))
	return 0;
bOk = (cf.Read (&ldh, sizeof (ldh), 1) == 1);
if (bOk)
	bOk = (ldh.nVersion == LIGHT_DATA_VERSION) &&
			(ldh.nSegments == gameData.segs.nSegments) &&
			(ldh.nVertices == gameData.segs.nVertices) &&
			(ldh.nLights == gameData.render.lights.dynamic.nLights) &&
			(ldh.nMaxLightRange == MAX_LIGHT_RANGE) &&
			(ldh.nMethod = LightingMethod () &&
			((ldh.bPerPixelLighting != 0) == (gameStates.render.bPerPixelLighting != 0)));
if (bOk)
	bOk =
			(cf.Read (gameData.segs.bSegVis.Buffer (), sizeof (ubyte) * ldh.nSegments * SEGVIS_FLAGS, 1) == 1) &&
#if 0
			(cf.Read (gameData.segs.bVertVis.Buffer (), sizeof (ubyte) * ldh.nVertices * VERTVIS_FLAGS, 1) == 1) &&
#endif
			(cf.Read (gameData.render.lights.dynamic.nearestSegLights.Buffer (), sizeof (short) * ldh.nSegments * MAX_NEAREST_LIGHTS, 1) == 1) &&
			(cf.Read (gameData.render.lights.dynamic.nearestVertLights.Buffer (), sizeof (short) * ldh.nVertices * MAX_NEAREST_LIGHTS, 1) == 1);
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------

int SaveLightData (int nLevel)
{
	CFile				cf;
	tLightDataHeader ldh = {LIGHT_DATA_VERSION,
								   gameData.segs.nSegments,
								   gameData.segs.nVertices,
								   gameData.render.lights.dynamic.nLights,
									MAX_LIGHT_RANGE,
									LightingMethod (),
									gameStates.render.bPerPixelLighting};
	int				bOk;
	char				szFilename [FILENAME_LEN];

if (!gameStates.app.bCacheLights)
	return 0;
if (!cf.Open (LightDataFilename (szFilename, nLevel), gameFolders.szCacheDir, "wb", 0))
	return 0;
bOk = (cf.Write (&ldh, sizeof (ldh), 1) == 1) &&
		(cf.Write (gameData.segs.bSegVis.Buffer (), sizeof (ubyte) * ldh.nSegments * SEGVIS_FLAGS, 1) == 1) &&
#if 0
		(cf.Write (gameData.segs.bVertVis.Buffer (), sizeof (ubyte) * ldh.nVertices * VERTVIS_FLAGS, 1) == 1) &&
#endif
		(cf.Write (gameData.render.lights.dynamic.nearestSegLights.Buffer (), sizeof (short) * ldh.nSegments * MAX_NEAREST_LIGHTS, 1) == 1) &&
		(cf.Write (gameData.render.lights.dynamic.nearestVertLights.Buffer (), sizeof (short) * ldh.nVertices * MAX_NEAREST_LIGHTS, 1) == 1);
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------

#if MULTI_THREADED_PRECALC

static tThreadInfo	ti [2];

//------------------------------------------------------------------------------

int _CDECL_ SegLightsThread (void *pThreadId)
{
	int		nId = *(reinterpret_cast<int*> (pThreadId));

ComputeNearestSegmentLights (nId ? gameData.segs.nSegments / 2 : 0);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
return 0;
}

//------------------------------------------------------------------------------

int _CDECL_ VertLightsThread (void *pThreadId)
{
	int		nId = *(reinterpret_cast<int*> (pThreadId));

ComputeNearestVertexLights (nId ? gameData.segs.nVertices / 2 : 0);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
return 0;
}

//------------------------------------------------------------------------------

static void StartOglLightThreads (pThreadFunc pFunc)
{
	int	i;

for (i = 0; i < 2; i++) {
	ti [i].bDone = 0;
	ti [i].done = SDL_CreateSemaphore (0);
	ti [i].nId = i;
	ti [i].pThread = SDL_CreateThread (pFunc, &ti [i].nId);
	}
#if 1
SDL_SemWait (ti [0].done);
SDL_SemWait (ti [1].done);
#else
while (!(ti [0].bDone && ti [1].bDone))
	G3_SLEEP (0);
#endif
for (i = 0; i < 2; i++) {
	SDL_WaitThread (ti [i].pThread, NULL);
	SDL_DestroySemaphore (ti [i].done);
	}
}

#endif //MULTI_THREADED_PRECALC

//------------------------------------------------------------------------------

void ComputeNearestLights (int nLevel)
{
if (gameStates.app.bNostalgia)
	return;
if (!(SHOW_DYN_LIGHT ||
	  (gameStates.render.bAmbientColor && !gameStates.render.bColored) ||
	   (gameStates.app.bEnableShadows && !COMPETITION)))
	return;
loadOp = 0;
loadIdx = 0;
PrintLog ("Looking for precompiled light data\n");
if (LoadLightData (nLevel))
	return;
else
#if MULTI_THREADED_PRECALC
if (gameStates.app.bMultiThreaded && (gameData.segs.nSegments > 15)) {
	gameData.physics.side.bCache = 0;
	PrintLog ("Computing segment visibility\n");
	ComputeSegmentVisibility (-1);
	PrintLog ("Starting segment light calculation threads\n");
	StartOglLightThreads (SegLightsThread);
	PrintLog ("Starting vertex light calculation threads\n");
	StartOglLightThreads (VertLightsThread);
	gameData.physics.side.bCache = 1;
	}
else {
	int bMultiThreaded = gameStates.app.bMultiThreaded;
	gameStates.app.bMultiThreaded = 0;
#endif
	if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
		ProgressBar (TXT_PREP_DESCENT,
						 LoadMineGaugeSize () + PagingGaugeSize (),
						 LoadMineGaugeSize () + PagingGaugeSize () + SortLightsGaugeSize (), SortLightsPoll);
	else {
		PrintLog ("Computing segment visibility\n");
		ComputeSegmentVisibility (-1);
		PrintLog ("Computing segment lights\n");
		ComputeNearestSegmentLights (-1);
		PrintLog ("Computing vertex lights\n");
		ComputeNearestVertexLights (-1);
		}
	gameStates.app.bMultiThreaded = bMultiThreaded;
	}
gameData.segs.bVertVis.Destroy ();
PrintLog ("Saving precompiled light data\n");
SaveLightData (nLevel);
}

//------------------------------------------------------------------------------
//eof
