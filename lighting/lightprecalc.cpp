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

#include "descent.h"
#include "text.h"
#include "gamemine.h"
#include "error.h"
#include "lightmap.h"
#include "rendermine.h"
#include "gameseg.h"
#include "menu.h"
#include "key.h"
#include "u_mem.h"
#include "paging.h"
#include "light.h"
#include "dynlight.h"

//------------------------------------------------------------------------------

#define LIGHT_DATA_VERSION 14


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

static inline int IsLightVert (int nVertex, CSegFace *faceP)
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
if (!lightManager.LightCount (0))
	return 0;
if (!(pDists = new tLightDist [lightManager.LightCount (0)])) {
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
	pl = lightManager.Lights ();
	for (l = n = 0; l < lightManager.LightCount (0); l++, pl++) {
		m = (pl->info.nSegment < 0) ? OBJECTS [pl->info.nObject].info.nSegment : pl->info.nSegment;
		if (!gameData.segs.LightVis (m, i))
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
		lightManager.NearestSegLights  ()[k + l] = pDists [l].nIndex;
	for (; l < MAX_NEAREST_LIGHTS; l++)
		lightManager.NearestSegLights  ()[k + l] = -1;
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
	CFixVector*				vertP;
	CDynLight*				pl;
	CSide*					sideP;
	int						h, j, k, l, n, nMaxLights;
	CFixVector				vLightToVert;
	struct tLightDist*	pDists;

if (!lightManager.LightCount (0))
	return 0;
if (!(pDists = new tLightDist [lightManager.LightCount (0)])) {
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
	pl = lightManager.Lights ();
	for (l = n = 0; l < lightManager.LightCount (0); l++, pl++) {
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
			h = CFixVector::Normalize (vLightToVert) - (int) (pl->info.fRad * 6553.6f);
			if (h > MAX_LIGHT_RANGE * pl->info.fRange)
				continue;
			if ((pl->info.nSegment >= 0) && (pl->info.nSide >= 0)) {
				sideP = SEGMENTS [pl->info.nSegment].m_sides + pl->info.nSide;
#if DBG
				fix xDot = CFixVector::Dot (sideP->m_normals [0], vLightToVert);
				if (xDot < -I2X (1) / 6) {
					if (sideP->m_nType == SIDE_IS_QUAD) 
						continue;
					xDot = CFixVector::Dot (sideP->m_normals [1], vLightToVert);
					if (xDot < -I2X (1) / 6)
						continue;
					}
#else
				if ((CFixVector::Dot (sideP->m_normals [0], vLightToVert) < -I2X (1) / 6) &&
					 ((sideP->m_nType == SIDE_IS_QUAD) || (CFixVector::Dot (sideP->m_normals [1], vLightToVert) < -I2X (1) / 6)))
					continue;
#endif
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
		lightManager.NearestVertLights ()[k + l] = pDists [l].nIndex;
	for (; l < MAX_NEAREST_LIGHTS; l++)
		lightManager.NearestVertLights ()[k + l] = -1;
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

static void SetSegAndVertVis (short nStartSeg, short nSegment, int bLights)
{
if (!bLights && gameData.segs.SegVis (nStartSeg, nSegment))
	return;
while (!gameData.segs.SetSegVis (nStartSeg, nSegment, bLights))
	;
if (!bLights)
	return;

	tSegFaces*		segFaceP = SEGFACES + nSegment;
	CSegFace*		faceP;
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
// Do this for each side of the current segment, using the side Normal (s) as forward vector
// of the viewer

void ComputeSingleSegmentVisibility (short nStartSeg, short nFirstSide = 0, short nLastSide = 5, int bLights = 0)
{
	CSegment*		segP, *childP;
	CSide*			sideP;
	short				nSegment, nSide, nChildSeg, nChildSide, i;
	CFixVector		vNormal;
	CObject			viewer;

//PrintLog ("computing visibility of segment %d\n", nStartSeg);
gameStates.ogl.bUseTransform = 1;
#if DBG
if (nStartSeg == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
segP = SEGMENTS + nStartSeg;
sideP = segP->m_sides + nFirstSide;
if (bLights)
	viewer.info.position.vPos = VERTICES [sideP->m_corners [bLights - 1]];
else
	viewer.info.position.vPos = SEGMENTS [nStartSeg].Center ();
viewer.info.nSegment = nStartSeg;
gameData.objs.viewerP = &viewer;
for (nSide = nFirstSide; nSide <= nLastSide; nSide++, sideP++) {
#if 1
	if (bLights && gameStates.render.bPerPixelLighting) {
		if (0 <= (nChildSeg = segP->m_children [nSide])) {
			while (!gameData.segs.SetSegVis (nStartSeg, nChildSeg, bLights))
				;
			childP = SEGMENTS + nChildSeg;
			for (nChildSide = 0; nChildSide < 6; nChildSide++) {
				if (0 <= (nSegment = childP->m_children [nSide])) {
					while (!gameData.segs.SetSegVis (nChildSeg, nSegment, bLights))
						;
					}
				}
			}
		}
	// view from segment center towards current side
	vNormal = CFixVector::Avg (sideP->m_normals [0], sideP->m_normals [1]);
	vNormal.Neg ();
	viewer.info.position.mOrient = CFixMatrix::Create (&vNormal, 0);
	G3StartFrame (0, 0);
	RenderStartFrame ();
	G3SetViewMatrix (viewer.info.position.vPos, viewer.info.position.mOrient, gameStates.render.xZoom, 1);
#endif
#if DBG
if ((nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
	gameStates.render.nShadowPass = 1;	// enforce culling of segments behind viewer
	BuildRenderSegList (nStartSeg, 0, true);
	gameStates.render.nShadowPass = 0;
	G3EndFrame ();
	//PrintLog ("   flagging visible segments\n");
	for (i = 0; i < gameData.render.mine.nRenderSegs; i++) {
		if (0 > (nSegment = gameData.render.mine.nSegRenderList [i]))
			continue;
#if DBG
		if (nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
		if (nSegment >= gameData.segs.nSegments)
			continue;
#endif
		SetSegAndVertVis (nStartSeg, nSegment, bLights);
		if (!bLights)
			SetSegAndVertVis (nSegment, nStartSeg, 0);
		}
	}
gameStates.ogl.bUseTransform = 0;
}

//------------------------------------------------------------------------------

void ComputeSegmentVisibility (int startI)
{
	int			i, endI;

PrintLog ("computing segment visibility (%d)\n", startI);
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

void ComputeLightVisibility (int startI)
{
	int			i, endI;

PrintLog ("computing light visibility (%d)\n", startI);
if (startI <= 0) {
	i = sizeof (gameData.segs.bVertVis [0]) * gameData.segs.nVertices * VERTVIS_FLAGS;
	if (!gameData.segs.bVertVis.Create (i))
		return;
	gameData.segs.bVertVis.Clear ();
	}
else if (!gameData.segs.bVertVis)
	return;
if (gameStates.app.bMultiThreaded) {
	endI = startI ? lightManager.LightCount (0) : lightManager.LightCount (0)  / 2;
	}
else
	INIT_PROGRESS_LOOP (startI, endI, lightManager.LightCount (0));
if (startI < 0)
	startI = 0;
// every segment can see itself and its neighbours
CDynLight* pl = lightManager.Lights () + startI;
for (i = endI - startI; i; i--, pl++)
	if ((pl->info.nSegment >= 0) && (pl->info.nSide >= 0))
		for (int j = 1; j <= 4; j++)
			ComputeSingleSegmentVisibility (pl->info.nSegment, pl->info.nSide, pl->info.nSide, j);
}

//------------------------------------------------------------------------------

static int SortLightsPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

paletteManager.LoadEffect ();
if (loadOp == 0) {
	ComputeSegmentVisibility (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 1;
		}
	}
if (loadOp == 1) {
	ComputeLightVisibility (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= lightManager.LightCount (0)) {
		loadIdx = 0;
		loadOp = 2;
		}
	}
else if (loadOp == 2) {
	ComputeNearestSegmentLights (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 3;
		}
	}
else if (loadOp == 3) {
	ComputeNearestVertexLights (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nVertices) {
		loadIdx = 0;
		loadOp = 4;
		}
	}
if (loadOp == 4) {
	key = -2;
	paletteManager.LoadEffect ();
	return nCurItem;
	}
menu [0].m_value++;
menu [0].m_bRebuild = 1;
key = 0;
paletteManager.LoadEffect ();
return nCurItem;
}

//------------------------------------------------------------------------------

int SortLightsGaugeSize (void)
{
if (gameStates.app.bNostalgia)
	return 0;
if (gameStates.app.bMultiThreaded)
	return 0;
return PROGRESS_STEPS (gameData.segs.nSegments) * 2 + 
		 PROGRESS_STEPS (gameData.segs.nVertices) + 
		 PROGRESS_STEPS (lightManager.LightCount (0));
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
			(ldh.nLights == lightManager.LightCount (0)) &&
			(ldh.nMaxLightRange == MAX_LIGHT_RANGE) &&
			(ldh.nMethod = LightingMethod () &&
			((ldh.bPerPixelLighting != 0) == (gameStates.render.bPerPixelLighting != 0)));
if (bOk)
	bOk = (gameData.segs.bSegVis [0].Read (cf, gameData.segs.SegVisSize (ldh.nSegments), 0) == size_t (gameData.segs.SegVisSize (ldh.nSegments))) &&
			(gameData.segs.bSegVis [1].Read (cf, gameData.segs.LightVisSize (ldh.nSegments), 0) == size_t (gameData.segs.LightVisSize (ldh.nSegments))) &&
			(lightManager.NearestSegLights  ().Read (cf, ldh.nSegments * MAX_NEAREST_LIGHTS, 0) == size_t (ldh.nSegments * MAX_NEAREST_LIGHTS)) &&
			(lightManager.NearestVertLights ().Read (cf, ldh.nVertices * MAX_NEAREST_LIGHTS, 0) == size_t (ldh.nVertices * MAX_NEAREST_LIGHTS));
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
								   lightManager.LightCount (0),
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
		(gameData.segs.bSegVis [0].Write (cf, gameData.segs.SegVisSize (ldh.nSegments)) == size_t (gameData.segs.SegVisSize (ldh.nSegments))) &&
		(gameData.segs.bSegVis [1].Write (cf, gameData.segs.LightVisSize (ldh.nSegments)) == size_t (gameData.segs.LightVisSize (ldh.nSegments))) &&
		(lightManager.NearestSegLights  ().Write (cf, ldh.nSegments * MAX_NEAREST_LIGHTS) == size_t (ldh.nSegments * MAX_NEAREST_LIGHTS)) &&
		(lightManager.NearestVertLights ().Write (cf, ldh.nVertices * MAX_NEAREST_LIGHTS) == size_t (ldh.nVertices * MAX_NEAREST_LIGHTS));
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
	   !COMPETITION))
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
	PrintLog ("Computing light visibility\n");
	ComputeLightVisibility (-1);
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
		PrintLog ("Computing light visibility\n");
		ComputeLightVisibility (-1);
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
