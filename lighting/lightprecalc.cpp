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
#include "loadgeometry.h"
#include "error.h"
#include "lightmap.h"
#include "rendermine.h"
#include "segmath.h"
#include "menu.h"
#include "netmisc.h"
#include "key.h"
#include "u_mem.h"
#include "paging.h"
#include "light.h"
#include "dynlight.h"

#if !USE_OPENMP
#include "SDL_mutex.h"

//static SDL_mutex* semaphore;
#endif

int SegmentIsVisible (CSegment *segP);

//------------------------------------------------------------------------------

#define LIGHT_DATA_VERSION 18

#define	VERTVIS(_nSegment, _nVertex) \
	(gameData.segs.bVertVis.Buffer () ? gameData.segs.bVertVis [(_nSegment) * VERTVIS_FLAGS + ((_nVertex) >> 3)] & (1 << ((_nVertex) & 7)) : 0)

//------------------------------------------------------------------------------

typedef struct tLightDataHeader {
	int	nVersion;
	int	nCheckSum;
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
ushort *pv = (gameStates.render.bTriangleMesh ? faceP->triIndex : faceP->m_info.index);
for (int i = faceP->m_info.nVerts; i; i--, pv++)
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
		h = int (CFixVector::Dist (center, pl->info.vPos) - F2X (pl->info.fRad));
		if (h < 0)
			h = 0;
		else if (h > MAX_LIGHT_RANGE * pl->info.fRange)
			continue;
		pDists [n].nDist = h;
		pDists [n++].nIndex = l;
		}
	if (n)
		QSortLightDist (pDists, 0, n - 1);
	h = (nMaxLights < n) ? nMaxLights : n;
	k = i * MAX_NEAREST_LIGHTS;
	for (l = 0; l < h; l++)
		lightManager.NearestSegLights ()[k + l] = pDists [l].nIndex;
	for (; l < MAX_NEAREST_LIGHTS; l++)
		lightManager.NearestSegLights ()[k + l] = -1;
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
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nSegment = nSegment;
#endif
	if (gameStates.render.bTriangleMesh) {
		for (nTris = faceP->m_info.nTris, triP = FACES.tris + faceP->m_info.nTriIndex; nTris; nTris--, triP++)
			for (i = 0; i < 3; i++)
				while (!SetVertVis (nStartSeg, triP->index [i], 1))
					;
		}
	else {
		for (i = 0; i < 4; i++)
			while (!SetVertVis (nStartSeg, faceP->m_info.index [i], 1))
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
	CSegment*		startSegP, *childP;
	CSide*			sideP;
	short				nSegment, nSide, nChildSeg, nChildSide, i;
	CFixVector		fVec, uVec, rVec;
	CObject			viewer;

//PrintLog ("computing visibility of segment %d\n", nStartSeg);
ogl.SetTransform (1);
#if DBG
if (nStartSeg == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
SetSegAndVertVis (nStartSeg, nStartSeg, bLights);
startSegP = SEGMENTS + nStartSeg;
sideP = startSegP->m_sides + nFirstSide;
	
viewer.info.nSegment = nStartSeg;
gameData.objs.viewerP = &viewer;

for (nSide = nFirstSide; nSide <= nLastSide; nSide++, sideP++) {
#if DBG
	sideP = startSegP->m_sides + nSide;
#endif
	fVec = sideP->m_normals [2];
	if (!bLights) {
#if 0
		viewer.info.position.vPos = sideP->Center () + fVec;
#else
		viewer.info.position.vPos = SEGMENTS [nStartSeg].Center ();// + fVec;
#endif
		fVec.Neg (); // point from segment center outwards
		rVec = CFixVector::Avg (VERTICES [sideP->m_corners [0]], VERTICES [sideP->m_corners [1]]) - sideP->Center ();
		CFixVector::Normalize (rVec);
#if 0 //DBG
		int i;
		do {
			for (i = 0; i < 4; i++) {
				CFixVector v = VERTICES [sideP->m_corners [i]] - viewer.info.position.vPos;
				CFixVector::Normalize (v);
				if (CFixVector::Dot (v, fVec) < 46341) { // I2X * cos (90 / 2), where 90 is the view angle
					viewer.info.position.vPos -= fVec;
					break;
					}
				}
			} while (i < 4);
#endif
		}
	else { // point from side center across the segment
		if (bLights == 5) { // from side center, pointing to normal
			viewer.info.position.vPos = sideP->Center () + fVec;
			rVec = CFixVector::Avg (VERTICES [sideP->m_corners [0]], VERTICES [sideP->m_corners [1]]) - sideP->Center ();
			CFixVector::Normalize (rVec);
			}
		else { // from side corner, pointing to average between vector from center to corner and side normal
#if DBG
			if ((nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nDbgSide == nSide)))
				nDbgSeg = nDbgSeg;
#endif
			viewer.info.position.vPos = VERTICES [sideP->m_corners [bLights - 1]];
			CFixVector h = sideP->Center () - viewer.info.position.vPos;
			CFixVector::Normalize (h);
			fVec = CFixVector::Avg (fVec, h);
			CFixVector::Normalize (fVec);
			rVec = sideP->m_normals [2];
			h.Neg ();
			rVec = CFixVector::Avg (rVec, h);
			CFixVector::Normalize (rVec);
			}
		if (gameStates.render.bPerPixelLighting) {
			if (0 <= (nChildSeg = startSegP->m_children [nSide])) {
				gameData.segs.SetSegVis (nStartSeg, nChildSeg, bLights);
				childP = SEGMENTS + nChildSeg;
				for (nChildSide = 0; nChildSide < 6; nChildSide++) {
					if (0 <= (nSegment = childP->m_children [nSide])) {
						while (!gameData.segs.SetSegVis (nChildSeg, nSegment, bLights))
							;
						}
					}
				}
			}
		}

#if 0 //DBG
		viewer.info.position.mOrient = CFixMatrix::Create (&fVec, 0);
#else
		CFixVector::Cross (uVec, fVec, rVec); // create uVec perpendicular to fVec and rVec
		CFixVector::Cross (rVec, fVec, uVec); // adjust rVec to be perpendicular to fVec and uVec (eliminate rounding errors)
		CFixVector::Cross (uVec, fVec, rVec); // adjust uVec to be perpendicular to fVec and rVec (eliminate rounding errors)
#	if DBG
		fix dot = CFixVector::Dot (rVec, fVec);
		if ((dot < -1) || (dot > 1))
			dot = 0;
		dot = CFixVector::Dot (rVec, uVec);
		if ((dot < -1) || (dot > 1))
			dot = 0;
		dot = CFixVector::Dot (fVec, uVec);
		if ((dot < -1) || (dot > 1))
			dot = 0;
#	endif
		viewer.info.position.mOrient = CFixMatrix::Create (rVec, uVec, fVec);
#endif

	fix w = CCanvas::Current ()->Width ();
	fix h = CCanvas::Current ()->Height ();
	CCanvas::Current ()->SetWidth (1024);
	CCanvas::Current ()->SetHeight (1024);
	gameStates.render.bRenderIndirect = -1;
	gameStates.render.nShadowMap = 1;
	G3StartFrame (0, 0, 0);
	RenderStartFrame ();
	G3SetViewMatrix (viewer.info.position.vPos, viewer.info.position.mOrient, gameStates.render.xZoom, 1);

#if DBG
if ((nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
	gameStates.render.nShadowPass = 1;	// enforce culling of segments behind viewer
	BuildRenderSegList (nStartSeg, 0, true);
	//PrintLog ("   flagging visible segments\n");
	for (i = 0; i < gameData.render.mine.nRenderSegs [0]; i++) {
		if (0 > (nSegment = gameData.render.mine.segRenderList [0][i]))
			continue;
#if DBG
		if (nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
		if (nSegment >= gameData.segs.nSegments)
			continue;
#endif
#if 1
		if (!SegmentIsVisible (SEGMENTS + nSegment))
			continue;
#endif
		SetSegAndVertVis (nStartSeg, nSegment, bLights);
		}
	gameStates.render.nShadowPass = 0;
	CCanvas::Current ()->SetWidth (w);
	CCanvas::Current ()->SetHeight (h);
	transformation.ComputeAspect ();
	G3EndFrame (0);
	gameStates.render.nShadowMap = 0;
	}
ogl.SetTransform (0);
}

//------------------------------------------------------------------------------

void ComputeSegmentVisibility (int startI)
{
	int			i, endI;

PrintLog ("computing segment visibility (%d)\n", startI);
if (gameStates.app.bMultiThreaded)
	endI = gameData.segs.nSegments;
else
	INIT_PROGRESS_LOOP (startI, endI, gameData.segs.nSegments);
if (startI < 0)
	startI = 0;
for (i = startI; i < endI; i++)
	ComputeSingleSegmentVisibility (i);
}

//------------------------------------------------------------------------------

void CheckLightVisibility (short nStartSeg, short nSide, short nDestSeg, fix xLightRange)
{
	int bVisible = gameData.segs.LightVis (nStartSeg, nDestSeg);

if (!bVisible)
	return;

	CHitQuery fq (FQ_TRANSWALL | FQ_TRANSPOINT | FQ_VISIBILITY, &VERTICES [0], &VERTICES [0], nStartSeg, -1, 1, 0);
	CHitData	hitData;
	CSegment* segP = SEGMENTS + nStartSeg;
	CSide* sideP = segP->m_sides;
	int i;

segP = SEGMENTS + nDestSeg;
for (i = 0; (i < 4); i++) {
	fq.p0 = &VERTICES [sideP->m_corners [i]];
	for (int j = 0; (j < 8); j++) {
		fq.p1 = &VERTICES [segP->m_verts [j]];
		if (CFixVector::Dist (*fq.p0, *fq.p1) > xLightRange)
			continue;
		int nHitType = FindHitpoint (&fq, &hitData);
		if (!nHitType || ((nHitType == HIT_WALL) && (hitData.hit.nSegment == nDestSeg)))
			return;
		}
	}

i = gameData.segs.LightVisIdx (nStartSeg, nDestSeg);
gameData.segs.bSegVis [1][i >> 3] &= ~(1 << (i & 7));
}

//------------------------------------------------------------------------------

void ComputeLightVisibility (int startI)
{
	int	i, j, endI;
	fix	xLightRange;

PrintLog ("computing light visibility (%d)\n", startI);
if (startI <= 0) {
	i = sizeof (gameData.segs.bVertVis [0]) * gameData.segs.nVertices * VERTVIS_FLAGS;
	if (!gameData.segs.bVertVis.Create (i))
		return;
	gameData.segs.bVertVis.Clear ();
	}
else if (!gameData.segs.bVertVis)
	return;
if (gameStates.app.bMultiThreaded)
	endI = lightManager.LightCount (0);
else
	INIT_PROGRESS_LOOP (startI, endI, lightManager.LightCount (0));
if (startI < 0)
	startI = 0;
// every segment can see itself and its neighbours
CDynLight* pl = lightManager.Lights () + startI;
for (i = endI - startI; i; i--, pl++)
	if ((pl->info.nSegment >= 0) && (pl->info.nSide >= 0)) {
		for (j = 1; j <= 5; j++)
			ComputeSingleSegmentVisibility (pl->info.nSegment, pl->info.nSide, pl->info.nSide, j);
		xLightRange = fix (MAX_LIGHT_RANGE * pl->info.fRange);
		for (j = 0; j < gameData.segs.nSegments; j++)
			CheckLightVisibility (pl->info.nSegment, pl->info.nSide, j, xLightRange);
		}
}

//------------------------------------------------------------------------------

static int SortLightsPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

//paletteManager.ResumeEffect ();
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
	//paletteManager.ResumeEffect ();
	return nCurItem;
	}
menu [0].m_value++;
menu [0].m_bRebuild = 1;
key = 0;
//paletteManager.ResumeEffect ();
return nCurItem;
}

//------------------------------------------------------------------------------

int SortLightsGaugeSize (void)
{
//if (gameStates.app.bNostalgia)
//	return 0;
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
			(ldh.nCheckSum == CalcSegmentCheckSum ()) &&
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
									CalcSegmentCheckSum (),
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
//if (gameStates.app.bNostalgia)
//	return;
if (!(SHOW_DYN_LIGHT ||
	  (gameStates.render.bAmbientColor && !gameStates.render.bColored) ||
	   !COMPETITION))
	return;
loadOp = 0;
loadIdx = 0;
PrintLog ("Looking for precompiled light data\n");
if (LoadLightData (nLevel))
	return;

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
		ProgressBar (TXT_LOADING,
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
