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
#include "findpath.h"
#include "segmath.h"
#include "menu.h"
#include "netmisc.h"
#include "key.h"
#include "u_mem.h"
#include "paging.h"
#include "light.h"
#include "dynlight.h"
#include "findpath.h"

#if !USE_OPENMP
#include "SDL_mutex.h"

//static SDL_mutex* semaphore;
#endif

int SegmentIsVisible (CSegment *segP, CTransformation& transformation, int nThread);

//------------------------------------------------------------------------------

#define LIGHT_DATA_VERSION 30

#define	VERTVIS(_nSegment, _nVertex) \
	(gameData.segs.bVertVis.Buffer () ? gameData.segs.bVertVis [(_nSegment) * VERTVIS_FLAGS + ((_nVertex) >> 3)] & (1 << ((_nVertex) & 7)) : 0)

#define FAST_POINTVIS 1
#define FAST_LIGHTVIS 1

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
	int	bCompressed;
	} tLightDataHeader;

//------------------------------------------------------------------------------

static int loadIdx = 0;
static int loadOp = 0;

//------------------------------------------------------------------------------

static int GetLoopLimits (int nStart, int& nEnd, int nMax, int nThread)
{
if (!gameStates.app.bMultiThreaded) {
	INIT_PROGRESS_LOOP (nStart, nEnd, nMax);
	}
else {
	nEnd = (nThread + 1) * (nMax + gameStates.app.nThreads - 1) / gameStates.app.nThreads;
	if (nEnd > nMax)
		nEnd = nMax;
	}
return (nStart < 0) ? 0 : nStart;
}

//------------------------------------------------------------------------------

void ComputeSingleSegmentDistance (int nSegment, int nThread)
{
	fix xMaxDist = 0;
	float scale = 1.0f;
	short nMinSeg = -1, nMaxSeg = -1;
	CDACSUniDirRouter& router = uniDacsRouter [nThread];

G3_SLEEP (0);
#if DBG
if (nSegment == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
router.PathLength (CFixVector::ZERO, nSegment, CFixVector::ZERO, -1, I2X (1024), WID_TRANSPARENT_FLAG | WID_PASSABLE_FLAG, -1);
for (int i = 0; i < gameData.segs.nSegments; i++) {
	fix xDistance = router.Distance (i);
	if (xDistance < 0) 
		continue;
	if (xMaxDist < xDistance)
		xMaxDist = xDistance;
	if (nMinSeg < 0)
		nMinSeg = i;
	nMaxSeg = i;
	}
// find constant factor to scale all distances from current segment down to 16 bits
#if 1
if (xMaxDist > 0xFFFE) // 0xFFFF is reserved and means "not reachable"
	scale = (float) xMaxDist / (float) 0xFFFE;
#else
while (xMaxDist & 0xFFFF0000) {
	xMaxDist >>= 1;
	++scale;
	}
#endif

CSegDistList& segDist = gameData.segs.segDistTable [nSegment];
#if 0 //DBG
nMinSeg = 0;
nMaxSeg = LEVEL_SEGMENTS - 1;
#endif
segDist.offset = nMinSeg;
segDist.length = nMaxSeg - nMinSeg + 1;
segDist.scale = scale;
if (!segDist.Create ())
	return;
segDist.Set (nSegment, 0);
for (int i = nMinSeg; i <= nMaxSeg; i++) 
	segDist.Set ((ushort) i, router.Distance (i));
}

//------------------------------------------------------------------------------

void ComputeSegmentDistance (int startI, int nThread)
{
	int endI;

uniDacsRouter [nThread].Create (gameData.segs.nSegments);
for (int i = GetLoopLimits (startI, endI, gameData.segs.nSegments, nThread); i < endI; i++)
	ComputeSingleSegmentDistance (i, nThread);
}

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

int ComputeNearestSegmentLights (int i, int nThread)
{
	CSegment*			segP;
	CDynLight*			lightP;
	int					h, j, k, l, m, n, nMaxLights;
	CFixVector			center;
	struct tLightDist	*pDists;

if (!lightManager.LightCount (0))
	return 0;

if (!(pDists = new tLightDist [lightManager.LightCount (0)])) {
	gameOpts->render.nLightingMethod = 0;
	gameData.render.shadows.nLights = 0;
	return 0;
	}

nMaxLights = MAX_NEAREST_LIGHTS;
i = GetLoopLimits (i, j, gameData.segs.nSegments, nThread);
for (segP = SEGMENTS + i; i < j; i++, segP++) {
	center = segP->Center ();
	lightP = lightManager.Lights ();
	for (l = n = 0; l < lightManager.LightCount (0); l++, lightP++) {
		m = (lightP->info.nSegment < 0) ? OBJECTS [lightP->info.nObject].info.nSegment : lightP->info.nSegment;
		if (!gameData.segs.LightVis (m, i))
			continue;
		h = int (CFixVector::Dist (center, lightP->info.vPos) - F2X (lightP->info.fRad));
		if (h < 0)
			h = 0;
		else if (h > MAX_LIGHT_RANGE * lightP->info.fRange)
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

int ComputeNearestVertexLights (int nVertex, int nThread)
{
	CFixVector*				vertP;
	CDynLight*				lightP;
	int						h, j, k, l, n, nMaxLights;
	CFixVector				vLightToVert;
	struct tLightDist*	pDists;

G3_SLEEP (0);

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
nVertex = GetLoopLimits (nVertex, j, gameData.segs.nVertices, nThread);

for (vertP = gameData.segs.vertices + nVertex; nVertex < j; nVertex++, vertP++) {
#if DBG
	if (nVertex == nDbgVertex)
		nVertex = nVertex;
#endif
	lightP = lightManager.Lights ();
	for (l = n = 0; l < lightManager.LightCount (0); l++, lightP++) {
#if DBG
		if (lightP->info.nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		if (IsLightVert (nVertex, lightP->info.faceP))
			h = 0;
		else {
			h = (lightP->info.nSegment < 0) ? OBJECTS [lightP->info.nObject].info.nSegment : lightP->info.nSegment;
			if (!VERTVIS (h, nVertex))
				continue;
			vLightToVert = *vertP - lightP->info.vPos;
			h = CFixVector::Normalize (vLightToVert) - (int) (lightP->info.fRad * 6553.6f);
			if (h > MAX_LIGHT_RANGE * lightP->info.fRange)
				continue;
#if 0
			if ((lightP->info.nSegment >= 0) && (lightP->info.nSide >= 0)) {
				CSide* sideP = SEGMENTS [lightP->info.nSegment].m_sides + lightP->info.nSide;
				if ((CFixVector::Dot (sideP->m_normals [0], vLightToVert) < 0) &&
					 ((sideP->m_nType == SIDE_IS_QUAD) || (CFixVector::Dot (sideP->m_normals [1], vLightToVert) < 0))) {
					h = simpleRouter [nThread].PathLength (VERTICES [nVertex], -1, lightP->info.vPos, lightP->info.nSegment, X2I (xMaxLightRange / 5), WID_TRANSPARENT_FLAG | WID_PASSABLE_FLAG, 0);
					if (h > 4 * MAX_LIGHT_RANGE / 3 * lightP->info.fRange)
						continue;
					}
				}
#endif
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
if (gameData.segs.bVertVis.Buffer ()) {
#ifdef _OPENMP
	ubyte* flagP = gameData.segs.bVertVis.Buffer (nSegment * VERTVIS_FLAGS + (nVertex >> 3));
	ubyte flag = 1 << (nVertex & 7);
#	pragma omp atomic
	*flagP |= flag;
#else
	gameData.segs.bVertVis [nSegment * VERTVIS_FLAGS + (nVertex >> 3)] |= (1 << (nVertex & 7));
#endif
	}
return 1;
}

//------------------------------------------------------------------------------

static void SetSegAndVertVis (short nStartSeg, short nSegment, int bLights)
{
#if FAST_LIGHTVIS
if (!bLights && gameData.segs.SegVis (nStartSeg, nSegment))
	return;
while (!gameData.segs.SetSegVis (nStartSeg, nSegment, bLights))
	;
if (!bLights)
	return;
#else
if (!bLights) {
	if (!gameData.segs.SegVis (nStartSeg, nSegment))
		while (!gameData.segs.SetSegVis (nStartSeg, nSegment, bLights))
			;
	return;
	}
#endif

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

static void ComputeSingleSegmentVisibility (int nThread, short nStartSeg, short nFirstSide = 0, short nLastSide = 5, int bLights = 0)
{
	CSegment*			startSegP;
	CSide*				sideP;
	short					nSegment, nSide,i;
	CFixVector			fVec, uVec, rVec;
	CObject				viewer;
	CTransformation	transformation;

//G3_SLEEP (0);
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
#if 0
		if (gameStates.render.bPerPixelLighting) {
			int nChildSeg = startSegP->m_children [nSide];
			if (0 <= nChildSeg) {
				gameData.segs.SetSegVis (nStartSeg, nChildSeg, bLights);
				CSegment* childP = SEGMENTS + nChildSeg;
				for (int nChildSide = 0; nChildSide < 6; nChildSide++) {
					if (0 <= (nSegment = childP->m_children [nSide])) {
						while (!gameData.segs.SetSegVis (nChildSeg, nSegment, bLights))
							;
						}
					}
				}
			}
#endif
		}

#if 0 //DBG
		viewer.info.position.mOrient = CFixMatrix::Create (&fVec, 0);
#else
		CFixVector::Cross (uVec, fVec, rVec); // create uVec perpendicular to fVec and rVec
		CFixVector::Cross (rVec, fVec, uVec); // adjust rVec to be perpendicular to fVec and uVec (eliminate rounding errors)
		CFixVector::Cross (uVec, fVec, rVec); // adjust uVec to be perpendicular to fVec and rVec (eliminate rounding errors)
		viewer.info.position.mOrient = CFixMatrix::Create (rVec, uVec, fVec);
#endif

	transformation.ComputeAspect (1024, 1024);
	//gameStates.render.bRenderIndirect = -1;
	//gameStates.render.nShadowMap = -1;
	//G3StartFrame (transformation, 0, 0, 0);
	//RenderStartFrame ();
	SetupTransformation (transformation, viewer.info.position.vPos, viewer.info.position.mOrient, gameStates.render.xZoom, -1, 0, false);

#if DBG
if ((nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
	gameStates.render.nShadowPass = 1;	// enforce culling of segments behind viewer
	gameData.render.mine.visibility [nThread].BuildSegList (transformation, nStartSeg, 0, true);
	for (i = 0; i < gameData.render.mine.visibility [nThread].nSegments; i++) {
		if (0 > (nSegment = gameData.render.mine.visibility [nThread].segments [i]))
			continue;
#if DBG
		if (nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
		if (nSegment >= gameData.segs.nSegments)
			continue;
#endif
#if 1
#	if 0
		if (bLights && !gameData.segs.SegVis (nStartSeg, nSegment))
			continue;
#	endif
		if (!SegmentIsVisible (SEGMENTS + nSegment, transformation, nThread))
			continue;
		if (bLights && (gameData.segs.SegDist (nStartSeg, nSegment) < 0))
			continue;
#endif
		SetSegAndVertVis (nStartSeg, nSegment, bLights);
		}
	gameStates.render.nShadowPass = 0;
	//G3EndFrame (transformation, 0);
	gameStates.render.nShadowMap = 0;
	}
ogl.SetTransform (0);
}

//------------------------------------------------------------------------------

static void ComputeSegmentVisibilityST (int startI)
{
	int i, endI;

if (gameStates.app.bMultiThreaded)
	endI = gameData.segs.nSegments;
else
	INIT_PROGRESS_LOOP (startI, endI, gameData.segs.nSegments);
if (startI < 0)
	startI = 0;
for (i = startI; i < endI; i++)
	ComputeSingleSegmentVisibility (0, i);
}

//------------------------------------------------------------------------------

void ComputeSegmentVisibilityMT (int startI, int nThread)
{
	int endI;

for (int i = GetLoopLimits (startI, endI, gameData.segs.nSegments, nThread); i < endI; i++)
	ComputeSingleSegmentVisibility (nThread, i);
}

//------------------------------------------------------------------------------
// This function checks whether segment nDestSeg has a chance to receive diffuse (direct)
// light from the light source at (nLightSeg,nSide).

static void CheckLightVisibility (short nLightSeg, short nSide, short nDestSeg, fix xMaxDist, float fRange, int nThread)
{
#if 0
	int bVisible = gameData.segs.LightVis (nLightSeg, nDestSeg);
if (!bVisible)
	return;
#else
	int bVisible = gameData.segs.SegVis (nLightSeg, nDestSeg);
#endif
	int i;

#if FAST_LIGHTVIS == 2
	fix dPath = gameData.segs.SegDist (nLightSeg, nDestSeg);
#else
	fix dPath = 0;
#endif

#if DBG
if (nLightSeg == nDbgSeg)
	nDbgSeg = nDbgSeg;
if (nDestSeg == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
i = gameData.segs.LightVisIdx (nLightSeg, nDestSeg);
if ((0 < dPath) || // path from light to dest blocked
	 (SEGMENTS [nLightSeg].HasOutdoorsProp () && (nLightSeg != nDestSeg)) || // light only illuminates its own face
	 (CFixVector::Dist (SEGMENTS [nLightSeg].Center (), SEGMENTS [nDestSeg].Center ()) - SEGMENTS [nLightSeg].MaxRad () - SEGMENTS [nDestSeg].MaxRad () >= xMaxDist)) { // distance to great
#if FAST_LIGHTVIS // segVis is initialized to zero
#	ifdef _OPENMP
	ubyte* flagP = &gameData.segs.bSegVis [1][i >> 2];
	ubyte flag = ~(3 << ((i & 3) << 1));
#		pragma omp atomic
	*flagP &= flag;
#	else
	gameData.segs.bSegVis [1][i >> 2] &= ~(3 << ((i & 3) << 1)); // no light contribution
#	endif
	return;
#endif
	}
#if FAST_LIGHTVIS
if (gameData.segs.bSegVis [1][i >> 2] & (3 << ((i & 3) << 1))) // face visible 
	return;
#endif

	CHitQuery		hitQuery (FQ_TRANSWALL | FQ_TRANSPOINT | FQ_VISIBILITY, &VERTICES [0], &VERTICES [0], nLightSeg, -1, 1, 0);
	CHitResult		hitResult;
#if !FAST_POINTVIS
	CFixVector		p0;
#endif
	CFloatVector	v0, v1;
	CSegment*		segP = SEGMENTS + nLightSeg;
	CSide*			sideP = segP->m_sides;
	fix				d, dMin = 0x7FFFFFFF;

// cast rays from light segment to target segment and see if at least one of them isn't blocked by geometry
segP = SEGMENTS + nDestSeg;
for (i = 4; i >= -4; i--) {
#if FAST_POINTVIS
	if (i == 4) 
		v0.Assign (sideP->Center ());
	else if (i >= 0) 
		v0 = FVERTICES [sideP->m_corners [i]];
	else 
		v0 = CFloatVector::Avg (FVERTICES [sideP->m_corners [4 + i]], FVERTICES [sideP->m_corners [(5 + i) & 3]]); // center of face's edges
#else
	if (i == 4) 
		hitQuery.p0 = &sideP->Center ();
	else if (i >= 0) {
		hitQuery.p0 = &VERTICES [sideP->m_corners [i]];
		}
	else {
		p0 = CFixVector::Avg (VERTICES [sideP->m_corners [4 + i]], VERTICES [sideP->m_corners [(5 + i) & 3]]); // center of face's edges
		hitQuery.p0 = &p0;
		}
#endif
	for (int j = 8; j >= 0; j--) {
#	if FAST_POINTVIS
		if (j == 8)
			v1.Assign (*hitQuery.p1);
		else
			v1 = FVERTICES [segP->m_vertices [j]];
#else
		hitQuery.p1 = (j == 8) ? &segP->Center () : &VERTICES [segP->m_vertices [j]];
#endif
		if ((d = CFixVector::Dist (*hitQuery.p0, *hitQuery.p1)) > xMaxDist)
			continue;
		if (dMin > d)
			dMin = d;
#if 0
		int canSee = PointSeesPoint (&v0, &v1, nLightSeg, nDestSeg, 0, 0);
		int nHitType = FindHitpoint (&hitQuery, &hitResult);
		if (canSee != (!nHitType || ((nHitType == HIT_WALL) && (hitResult.nSegment == nDestSeg)))) {
			canSee = PointSeesPoint (&v0, &v1, nLightSeg, nDestSeg, 0, 0);
			nHitType = FindHitpoint (&hitQuery, &hitResult);
			}
		if (!nHitType || ((nHitType == HIT_WALL) && (hitResult.nSegment == nDestSeg))) {
#else
#	if FAST_POINTVIS
		if (PointSeesPoint (&v0, &v1, nLightSeg, nDestSeg, 0, nThread)) {
#	else
		int nHitType = FindHitpoint (&hitQuery, &hitResult);
		if (!nHitType || ((nHitType == HIT_WALL) && (hitResult.nSegment == nDestSeg))) {
#	endif
#endif
			i = gameData.segs.LightVisIdx (nLightSeg, nDestSeg);
#ifdef _OPENMP
ubyte* flagP = &gameData.segs.bSegVis [1][i >> 2];
ubyte flag = (2 << ((i & 3) << 1));
#	pragma omp atomic
*flagP |= flag;
#else
			gameData.segs.bSegVis [1][i >> 2] |= (2 << ((i & 3) << 1)); // diffuse + ambient
#endif
			return;
			}
		}
	}

if (dMin > xMaxDist)
	return;
#if FAST_LIGHTVIS == 2
dMin = (dMin + dPath) / 2;
if (dMin > xMaxDist)
	return;
#endif

i = gameData.segs.LightVisIdx (nLightSeg, nDestSeg);
#ifdef _OPENMP
ubyte* flagP = &gameData.segs.bSegVis [1][i >> 2];
ubyte flag = (1 << ((i & 3) << 1));
#	pragma omp atomic
*flagP |= flag;
#else
gameData.segs.bSegVis [1][i >> 2] |= (1 << ((i & 3) << 1)); // diffuse
#endif
}

//------------------------------------------------------------------------------

static bool InitLightVisibility (void)
{
SetupSegments (fix (I2X (1) * 0.001f));
int i = sizeof (gameData.segs.bVertVis [0]) * gameData.segs.nVertices * VERTVIS_FLAGS;
if (!gameData.segs.bVertVis.Create (i)) 
	return false;
gameData.segs.bVertVis.Clear ();
return true;
}

//------------------------------------------------------------------------------

static void ComputeSingleLightVisibility (int nLight, int nThread)
{
CDynLight* lightP = lightManager.Lights () + nLight;
if ((lightP->info.nSegment >= 0) && (lightP->info.nSide >= 0)) {
#if DBG
	if ((nDbgSeg >= 0) && (lightP->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (lightP->info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
#if FAST_LIGHTVIS
	for (int i = 1; i <= 5; i++)
		ComputeSingleSegmentVisibility (nThread, lightP->info.nSegment, lightP->info.nSide, lightP->info.nSide, i);
#endif
#if 1
	fix xLightRange = fix (MAX_LIGHT_RANGE * lightP->info.fRange);
	for (int i = 0; i < gameData.segs.nSegments; i++)
		CheckLightVisibility (lightP->info.nSegment, lightP->info.nSide, i, xLightRange, lightP->info.fRange, nThread);
#endif
	}
}

//------------------------------------------------------------------------------

static void ComputeLightVisibilityST (int startI)
{
	int endI;

if (gameStates.app.bMultiThreaded)
	endI = lightManager.LightCount (0);
else
	INIT_PROGRESS_LOOP (startI, endI, lightManager.LightCount (0));
if (startI < 0)
	startI = 0;
// every segment can see itself and its neighbours
for (int i = endI - startI, j = 0; j < i; j++)
	ComputeSingleLightVisibility (j, 0);
}

//------------------------------------------------------------------------------

void ComputeLightVisibilityMT (int startI, int nThread)
{
	int endI;

for (int i = GetLoopLimits (startI, endI, lightManager.LightCount (0), nThread); i < endI; i++)
	ComputeSingleLightVisibility (i, nThread);
}

//------------------------------------------------------------------------------

void ComputeLightsVisibleVertices (int startI)
{
	int	i, endI;

for (i = GetLoopLimits (startI, endI, lightManager.LightCount (0), 0); i < endI; i++)
	lightManager.Lights (i)->ComputeVisibleVertices (0);
}

//------------------------------------------------------------------------------

int SegDistGaugeSize (void)
{
return PROGRESS_STEPS (gameData.segs.nSegments);
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
		 PROGRESS_STEPS (lightManager.LightCount (0) * 2);
}

//------------------------------------------------------------------------------

char *LightDataFilename (char *pszFilename, int nLevel)
{
return GameDataFilename (pszFilename, "light", nLevel, -1);
}

//------------------------------------------------------------------------------

static int LoadSegDistData (CFile& cf, tLightDataHeader& ldh)
{
for (int i = 0; i < ldh.nSegments; i++)
	if (!gameData.segs.segDistTable [i].Read (cf, ldh.bCompressed))
		return 0;
return 1;
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
	bOk = (ldh.nVersion == LIGHT_DATA_VERSION)
			&& (ldh.nCheckSum == CalcSegmentCheckSum ())
			&& (ldh.nSegments == gameData.segs.nSegments)
			&& (ldh.nVertices == gameData.segs.nVertices)
			&& (ldh.nLights == lightManager.LightCount (0))
			&& (ldh.nMaxLightRange == MAX_LIGHT_RANGE)
#if 0
			&& (ldh.nMethod = LightingMethod () &&
			&& ((ldh.bPerPixelLighting != 0) == (gameStates.render.bPerPixelLighting != 0)))
#endif
			;
if (bOk)
	bOk = (gameData.segs.bSegVis [0].Read (cf, gameData.segs.SegVisSize (ldh.nSegments), 0, ldh.bCompressed) == size_t (gameData.segs.SegVisSize (ldh.nSegments))) &&
			(gameData.segs.bSegVis [1].Read (cf, gameData.segs.LightVisSize (ldh.nSegments), 0, ldh.bCompressed) == size_t (gameData.segs.LightVisSize (ldh.nSegments))) &&
			LoadSegDistData (cf, ldh) &&
			(lightManager.NearestSegLights  ().Read (cf, ldh.nSegments * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nSegments * MAX_NEAREST_LIGHTS)) &&
			(lightManager.NearestVertLights ().Read (cf, ldh.nVertices * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nVertices * MAX_NEAREST_LIGHTS));
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------

static int SaveSegDistData (CFile& cf, tLightDataHeader& ldh)
{
for (int i = 0; i < ldh.nSegments; i++)
	if (!gameData.segs.segDistTable [i].Write (cf, ldh.bCompressed))
		return 0;
return 1;
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
									gameStates.render.bPerPixelLighting,
									gameStates.app.bCompressData
									};
	int				bOk;
	char				szFilename [FILENAME_LEN];

if (!gameStates.app.bCacheLights)
	return 0;
if (!cf.Open (LightDataFilename (szFilename, nLevel), gameFolders.szCacheDir, "wb", 0))
	return 0;
bOk = (cf.Write (&ldh, sizeof (ldh), 1) == 1) &&
		(gameData.segs.bSegVis [0].Write (cf, gameData.segs.SegVisSize (ldh.nSegments), 0, ldh.bCompressed) == size_t (gameData.segs.SegVisSize (ldh.nSegments))) &&
		(gameData.segs.bSegVis [1].Write (cf, gameData.segs.LightVisSize (ldh.nSegments), 0, ldh.bCompressed) == size_t (gameData.segs.LightVisSize (ldh.nSegments))) &&
		SaveSegDistData (cf, ldh) &&
		(lightManager.NearestSegLights  ().Write (cf, ldh.nSegments * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nSegments * MAX_NEAREST_LIGHTS)) &&
		(lightManager.NearestVertLights ().Write (cf, ldh.nVertices * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nVertices * MAX_NEAREST_LIGHTS));
cf.Close ();
return bOk;
}

#if MULTI_THREADED_PRECALC //---------------------------------------------------
#ifdef _OPENMP //---------------------------------------------------------------

void _CDECL_ SegVisThread (int nId)
{
ComputeSegmentVisibilityMT (nId * (gameData.segs.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ LightVisThread (int nId)
{
ComputeLightVisibilityMT (nId * (lightManager.LightCount (0) + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ SegDistThread (int nId)
{
ComputeSegmentDistance (nId * (gameData.segs.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ SegLightsThread (int nId)
{
ComputeNearestSegmentLights (nId * (gameData.segs.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ VertLightsThread (int nId)
{
ComputeNearestVertexLights (nId * (gameData.segs.nVertices + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

static void StartLightPrecalcThreads (pThreadFunc threadFunc)
{
#pragma omp parallel for
for (int i = 0; i < gameStates.app.nThreads; i++) 
	threadFunc (i);
}

//------------------------------------------------------------------------------

static int PrecalcLightPollMT (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

//paletteManager.ResumeEffect ();
if (loadOp == 0) {
	loadOp = 1;
	menu [0].Rebuild ();
	key = 0;
	return nCurItem;
	}
else if (loadOp == 1) {
	PrintLog (0, "computing segment visibility\n");
	StartLightPrecalcThreads (SegVisThread);
	loadOp = 2;
	}
else if (loadOp == 2) {
	PrintLog (0, "computing segment distances \n");
	StartLightPrecalcThreads (SegDistThread);
	loadOp = 3;
	}
else if (loadOp == 3) {
	PrintLog (1, "Computing light visibility\n");
	StartLightPrecalcThreads (LightVisThread);
	loadOp = 4;
	}
else if (loadOp == 4) {
	PrintLog (1, "Starting segment light calculation threads\n");
	StartLightPrecalcThreads (SegLightsThread);
	loadOp = 5;
	}
else if (loadOp == 5) {
	PrintLog (1, "Starting vertex light calculation threads\n");
	StartLightPrecalcThreads (VertLightsThread);
	loadOp = 6;
	}
else if (loadOp == 6) {
	PrintLog (1, "Computing vertices visible to lights\n");
	ComputeLightsVisibleVertices (-1);
	loadOp = 7;
	}
if (loadOp == 7) {
	key = -2;
	return nCurItem;
	}
menu [0].Value ()++;
menu [0].Rebuild ();
key = 0;
return nCurItem;
}

#else // _OPENMP ---------------------------------------------------------------

static CThreadInfo	ti [MAX_THREADS];


int _CDECL_ SegDistThread (void *pThreadId)
{
	int nId = *(reinterpret_cast<int*> (pThreadId));

ComputeSegmentDistance (nId * (gameData.segs.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
return 0;
}

//------------------------------------------------------------------------------

int _CDECL_ SegLightsThread (void *pThreadId)
{
	int nId = *(reinterpret_cast<int*> (pThreadId));

ComputeNearestSegmentLights (nId * (gameData.segs.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
return 0;
}

//------------------------------------------------------------------------------

int _CDECL_ VertLightsThread (void *pThreadId)
{
	int nId = *(reinterpret_cast<int*> (pThreadId));

ComputeNearestVertexLights (nId * (gameData.segs.nVertices + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
return 0;
}

//------------------------------------------------------------------------------

static void StartLightPrecalcThreads (pThreadFunc threadFunc)
{
	int	i;

for (i = 0; i < gameStates.app.nThreads; i++) {
	ti [i].bDone = 0;
	ti [i].done = SDL_CreateSemaphore (0);
	ti [i].nId = i;
	ti [i].pThread = SDL_CreateThread (threadFunc, &ti [i].nId);
	}
#if 1
for (i = 0; i < gameStates.app.nThreads; i++)
	SDL_SemWait (ti [i].done);
#else
while (!(ti [0].bDone && ti [1].bDone))
	G3_SLEEP (0);
#endif
for (i = 0; i < gameStates.app.nThreads; i++) {
	SDL_WaitThread (ti [i].pThread, NULL);
	SDL_DestroySemaphore (ti [i].done);
	}
}

#endif // _OPENMP --------------------------------------------------------------
#endif //MULTI_THREADED_PRECALC ------------------------------------------------

static int PrecalcLightPollST (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

//paletteManager.ResumeEffect ();
if (loadOp == 0) {
	if (loadIdx == 0)
		PrintLog (0, "computing segment visibility\n");
	ComputeSegmentVisibilityST (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 1;
		}
	}
else if (loadOp == 1) {
	if (loadIdx == 0)
		PrintLog (0, "computing segment distances \n");
	ComputeSegmentDistance (loadIdx, 0);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 2;
		}
	}
else if (loadOp == 2) {
	if (loadIdx == 0)
		PrintLog (0, "computing light visibility\n");
	ComputeLightVisibilityST (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= lightManager.LightCount (0)) {
		loadIdx = 0;
		loadOp = 3;
		}
	}
else if (loadOp == 3) {
	if (loadIdx == 0)
		PrintLog (0, "computing nearest segment lights\n");
	ComputeNearestSegmentLights (loadIdx, 0);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nSegments) {
		loadIdx = 0;
		loadOp = 4;
		}
	}
else if (loadOp == 4) {
	if (loadIdx == 0)
		PrintLog (0, "computing nearest vertex lights\n");
	ComputeNearestVertexLights (loadIdx, 0);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segs.nVertices) {
		loadIdx = 0;
		loadOp = 5;
		}
	}
else if (loadOp == 5) {
	if (loadIdx == 0)
		PrintLog (0, "Computing vertices visible to lights\n");
	ComputeLightsVisibleVertices (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= lightManager.LightCount (0)) {
		loadIdx = 0;
		loadOp = 6;
		}
	}
if (loadOp == 6) {
	key = -2;
	//paletteManager.ResumeEffect ();
	return nCurItem;
	}
menu [0].Value ()++;
menu [0].Rebuild ();
key = 0;
//paletteManager.ResumeEffect ();
return nCurItem;
}

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
PrintLog (1, "Looking for precompiled light data\n");
if (LoadLightData (nLevel)) {
	PrintLog (-1);
	return;
	}
PrintLog (-1);

if (!InitLightVisibility ())
	throw (EX_OUT_OF_MEMORY);
#ifdef _OPENMP
if (gameStates.app.bMultiThreaded && (gameData.segs.nSegments > 15)) {
	gameData.physics.side.bCache = 0;
	if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
		ProgressBar (TXT_LOADING, 0, 6, PrecalcLightPollMT);
	else {
		PrintLog (1, "Computing segment visibility\n");
		StartLightPrecalcThreads (SegVisThread);
		PrintLog (-1);
		PrintLog (1, "Computing segment distances\n");
		StartLightPrecalcThreads (SegDistThread);
		PrintLog (-1);
		PrintLog (1, "Computing light visibility\n");
		StartLightPrecalcThreads (LightVisThread);
		PrintLog (-1);
		PrintLog (1, "Starting segment light calculation threads\n");
		StartLightPrecalcThreads (SegLightsThread);
		PrintLog (-1);
		PrintLog (1, "Starting vertex light calculation threads\n");
		StartLightPrecalcThreads (VertLightsThread);
		PrintLog (-1);
		PrintLog (1, "Computing vertices visible to lights\n");
		ComputeLightsVisibleVertices (-1);
		PrintLog (-1);
		}
	gameData.physics.side.bCache = 1;
	}
else 
#endif
	{
	int bMultiThreaded = gameStates.app.bMultiThreaded;
	gameStates.app.bMultiThreaded = 0;
	if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
		ProgressBar (TXT_LOADING,
						 LoadMineGaugeSize () + PagingGaugeSize (),
						 LoadMineGaugeSize () + PagingGaugeSize () + SortLightsGaugeSize () + SegDistGaugeSize (), PrecalcLightPollST);
	else {
		PrintLog (1, "Computing segment visibility\n");
		ComputeSegmentVisibilityST (-1);
		PrintLog (-1);
		PrintLog (1, "Computing segment distances\n");
		ComputeSegmentDistance (-1, 0);
		PrintLog (-1);
		PrintLog (1, "Computing light visibility\n");
		ComputeLightVisibilityST (-1);
		PrintLog (-1);
		PrintLog (1, "Computing segment lights\n");
		ComputeNearestSegmentLights (-1, 0);
		PrintLog (-1);
		PrintLog (1, "Computing vertex lights\n");
		ComputeNearestVertexLights (-1, 0);
		PrintLog (-1);
		PrintLog (1, "Computing vertices visible to lights\n");
		ComputeLightsVisibleVertices (-1);
		PrintLog (-1);
		}
	gameStates.app.bMultiThreaded = bMultiThreaded;
	}

gameData.segs.bVertVis.Destroy ();
PrintLog (1, "Saving precompiled light data\n");
SaveLightData (nLevel);
PrintLog (-1);
}

//------------------------------------------------------------------------------
//eof
