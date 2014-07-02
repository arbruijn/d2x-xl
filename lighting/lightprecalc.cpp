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

#if !USE_OPENMP
#	ifdef __macosx__  // BEGIN itaylo 06 NOV 2013
#		include "SDL/SDL_mutex.h"
#	else
#		include "SDL_mutex.h"
#	endif              // END itaylo 06 NOV 2013
#endif

//static SDL_mutex* semaphore;
#endif

int32_t SegmentIsVisible (CSegment *segP, CTransformation& transformation, int32_t nThread);

//------------------------------------------------------------------------------

#define LIGHT_DATA_VERSION 32

#define	VERTVIS(_nSegment, _nVertex) \
	(gameData.segs.bVertVis.Buffer () ? gameData.segs.bVertVis [(_nSegment) * VERTVIS_FLAGS + ((_nVertex) >> 3)] & (1 << ((_nVertex) & 7)) : 0)

#define FAST_POINTVIS 1
#define FAST_LIGHTVIS 1

//------------------------------------------------------------------------------

typedef struct tLightDataHeader {
	int32_t	nVersion;
	int32_t	nCheckSum;
	int32_t	nSegments;
	int32_t	nVertices;
	int32_t	nLights;
	int32_t	nMaxLightRange;
	int32_t	nMethod;
	int32_t	bPerPixelLighting;
	int32_t	bCompressed;
	} tLightDataHeader;

//------------------------------------------------------------------------------

static int32_t loadIdx = 0;
static int32_t loadOp = 0;

//------------------------------------------------------------------------------

static int32_t GetLoopLimits (int32_t nStart, int32_t& nEnd, int32_t nMax, int32_t nThread)
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

#if DBG
static int32_t nDistCount = 0;
static int32_t nSavedCount = 0;
#endif

void ComputeSingleSegmentDistance (int32_t nSegment, int32_t nThread)
{
	fix xMaxDist = 0;
	float scale = 1.0f;
	int16_t nMinSeg = -1, nMaxSeg = -1;
	CDACSUniDirRouter& router = uniDacsRouter [nThread];

G3_SLEEP (0);
#if DBG
if (nSegment == nDbgSeg)
	BRP;
#endif
router.PathLength (CFixVector::ZERO, nSegment, CFixVector::ZERO, -1, I2X (1024), WID_TRANSPARENT_FLAG | WID_PASSABLE_FLAG, -1);
for (int32_t i = 0; i < gameData.segs.nSegments; i++) {
	fix xDistance = router.Distance (i);
	if (xDistance < 0) 
		continue;
#if DBG
	nDistCount++;
#endif
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
segDist.offset = nMinSeg;
segDist.length = nMaxSeg - nMinSeg + 1;
segDist.scale = scale;
if (!segDist.Create ())
	return;
segDist.Set (nSegment, 0);
for (int32_t i = nMinSeg; i <= nMaxSeg; i++) 
	segDist.Set ((uint16_t) i, router.Distance (i));
#if DBG
nSavedCount += gameData.segs.nSegments - segDist.length;
#endif
}

//------------------------------------------------------------------------------

void ComputeSegmentDistance (int32_t startI, int32_t nThread)
{
	int32_t endI;

uniDacsRouter [nThread].Create (gameData.segs.nSegments);
for (int32_t i = GetLoopLimits (startI, endI, gameData.segs.nSegments, nThread); i < endI; i++)
	ComputeSingleSegmentDistance (i, nThread);
}

//------------------------------------------------------------------------------

typedef struct tLightDist {
	int32_t		nIndex;
	int32_t		nDist;
} tLightDist;

void QSortLightDist (tLightDist *pDist, int32_t left, int32_t right)
{
	int32_t			l = left,
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

static inline int32_t IsLightVert (int32_t nVertex, CSegFace *faceP)
{
uint16_t *pv = (gameStates.render.bTriangleMesh ? faceP->triIndex : faceP->m_info.index);
for (int32_t i = faceP->m_info.nVerts; i; i--, pv++)
	if (*pv == (uint16_t) nVertex)
		return 1;
return 0;
}

//------------------------------------------------------------------------------

int32_t ComputeNearestSegmentLights (int32_t i, int32_t nThread)
{
	CSegment*			segP;
	CDynLight*			lightP;
	int32_t					h, j, k, l, n, nMaxLights;
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
		if (!((lightP->info.nSegment < 0) ? gameData.segs.SegVis (OBJECTS [lightP->info.nObject].info.nSegment, i) : gameData.segs.LightVis (lightP->Index (), i)))
			continue;
		h = int32_t (CFixVector::Dist (center, lightP->info.vPos) - F2X (lightP->info.fRad));
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
extern int32_t nDbgVertex;
#endif

int32_t ComputeNearestVertexLights (int32_t nVertex, int32_t nThread)
{
	CFixVector*				vertP;
	CDynLight*				lightP;
	int32_t						h, j, k, l, n, nMaxLights;
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
	BRP;
#endif

nMaxLights = MAX_NEAREST_LIGHTS;
nVertex = GetLoopLimits (nVertex, j, gameData.segs.nVertices, nThread);

for (vertP = gameData.segs.vertices + nVertex; nVertex < j; nVertex++, vertP++) {
#if DBG
	if (nVertex == nDbgVertex)
		BRP;
#endif
	lightP = lightManager.Lights ();
	for (l = n = 0; l < lightManager.LightCount (0); l++, lightP++) {
#if DBG
		if (lightP->info.nSegment == nDbgSeg)
			BRP;
#endif
		if (IsLightVert (nVertex, lightP->info.faceP))
			h = 0;
		else {
			h = (lightP->info.nSegment < 0) ? OBJECTS [lightP->info.nObject].info.nSegment : lightP->info.nSegment;
			if (!VERTVIS (h, nVertex))
				continue;
			vLightToVert = *vertP - lightP->info.vPos;
			h = CFixVector::Normalize (vLightToVert) - (int32_t) (lightP->info.fRad * 6553.6f);
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

inline int32_t SetVertVis (int16_t nSegment, int16_t nVertex, uint8_t b)
{
if (gameData.segs.bVertVis.Buffer ()) {
	uint8_t* flagP = gameData.segs.bVertVis.Buffer (nSegment * VERTVIS_FLAGS + (nVertex >> 3));
	uint8_t flag = 1 << (nVertex & 7);
#ifdef _OPENMP
#	pragma omp atomic
#endif
	*flagP |= flag;
	}
return 1;
}

//------------------------------------------------------------------------------

static void SetSegVis (int16_t nStartSeg, int16_t nSegment)
{
if (gameData.segs.SegVis (nStartSeg, nSegment))
	return;
while (!gameData.segs.SetSegVis (nStartSeg, nSegment))
	;
}

//------------------------------------------------------------------------------

static void SetLightVis (int16_t nLight, int16_t nSegment)
{
#if DBG
if ((lightManager.Lights (nLight)->info.nSegment == nDbgSeg) && (lightManager.Lights (nLight)->info.nSide == nDbgSide))
	BRP;
if (gameData.segs.LightVisIdx (nLight, nSegment) / 4 >= int32_t (gameData.segs.bLightVis.Length ()))
	return;
#endif
while (!gameData.segs.SetLightVis (nLight, nSegment, 2))
	;

	tSegFaces*		segFaceP = SEGFACES + nSegment;
	CSegFace*		faceP;
	tFaceTriangle*	triP;
	int32_t				i, nFaces, nTris;
	int16_t				nStartSeg = lightManager.Lights (nLight)->info.nSegment;

for (nFaces = segFaceP->nFaces, faceP = segFaceP->faceP; nFaces; nFaces--, faceP++) {
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	BRP;
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
// This function must be called ahead of computing segment to segment visibility
// when multi-threading is used because only thread 0 has a valid OpenGL context!

static CTransformation projection;

static void SetupProjection (void)
{
	int32_t		w = gameData.render.screen.Width (),
				h = gameData.render.screen.Height ();
	CCanvas canvas;

gameData.render.screen.Setup (NULL, 0, 0, 1024, 1024);
canvas.Setup (&gameData.render.screen);
canvas.Activate ("SetupProjection");
canvas.SetWidth ();
canvas.SetHeight ();
gameData.render.screen.SetWidth (1024);
gameData.render.screen.SetHeight (1024);
ogl.SetTransform (1);
ogl.SetViewport (0, 0, 1024, 1024);
gameStates.render.nShadowPass = 1;	// enforce culling of segments behind viewer
SetupTransformation (projection, CFixVector::ZERO, CFixMatrix::IDENTITY, gameStates.render.xZoom, -1, 0, true);
gameStates.render.nShadowPass = 0;
gameStates.render.nShadowMap = 0;
ogl.SetTransform (0);
canvas.Deactivate ();
gameData.render.screen.Setup (NULL, 0, 0, w, h);
gameData.render.screen.SetWidth (w);
gameData.render.screen.SetHeight (h);
ogl.EndFrame (-1);
}

//------------------------------------------------------------------------------
// Check segment to segment visibility by calling the renderer's visibility culling routine
// Do this for each side of the current segment, using the side Normal (s) as forward vector
// of the viewer

static void ComputeSingleSegmentVisibility (int32_t nThread, int16_t nStartSeg, int16_t nFirstSide, int16_t nLastSide, int32_t bLights)
{
	CSegment*			startSegP;
	CSide*				sideP;
	int16_t					nSegment, nSide, nLight = -1, i;
	CFixVector			fVec, uVec, rVec;
	CObject				viewer;
	CTransformation	transformation = projection;
	CVisibilityData&	visibility = gameData.render.mine.visibility [nThread];

#if DBG
if (nStartSeg == nDbgSeg)
	BRP;
#endif

if (!bLights) 
	SetSegVis (nStartSeg, nStartSeg);
else {
	nLight = nStartSeg;
	nStartSeg = lightManager.Lights (nLight)->info.nSegment;
	SetLightVis (nLight, nStartSeg);
	}
startSegP = SEGMENTS + nStartSeg;
sideP = startSegP->m_sides + nFirstSide;
	
viewer.info.nSegment = nStartSeg;
gameData.objs.viewerP = &viewer;

transformation.ComputeAspect (1024, 1024);

for (nSide = nFirstSide; nSide <= nLastSide; nSide++, sideP++) {
#if DBG
	sideP = startSegP->m_sides + nSide;
#endif
	fVec = sideP->m_normals [2];
	if (!bLights) {
		viewer.info.position.vPos = SEGMENTS [nStartSeg].Center ();// + fVec;
		fVec.Neg (); // point from segment center outwards
		rVec = CFixVector::Avg (VERTICES [sideP->m_corners [0]], VERTICES [sideP->m_corners [1]]) - sideP->Center ();
		CFixVector::Normalize (rVec);
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
				BRP;
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
		}

	CFixVector::Cross (uVec, fVec, rVec); // create uVec perpendicular to fVec and rVec
	CFixVector::Cross (rVec, fVec, uVec); // adjust rVec to be perpendicular to fVec and uVec (eliminate rounding errors)
	CFixVector::Cross (uVec, fVec, rVec); // adjust uVec to be perpendicular to fVec and rVec (eliminate rounding errors)
	CFixVector::Normalize (rVec);
	CFixVector::Normalize (uVec);
	CFixVector::Normalize (fVec);
	viewer.info.position.mOrient = CFixMatrix::Create (rVec, uVec, fVec);
#if DBG
	if ((nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
		BRP;
#endif
	SetupTransformation (transformation, viewer.info.position.vPos, viewer.info.position.mOrient, gameStates.render.xZoom, -1, 0, false);

#if DBG
if ((nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif
	visibility.BuildSegList (transformation, nStartSeg, 0, true);
	for (i = 0; i < visibility.nSegments; i++) {
		if (0 > (nSegment = visibility.segments [i]))
			continue;
#if DBG
		if (nSegment == nDbgSeg)
			BRP;
		if (nSegment >= gameData.segs.nSegments)
			continue;
#endif
		if (!SegmentIsVisible (SEGMENTS + nSegment, transformation, nThread))
			continue;
		if (!bLights)
			SetSegVis (nStartSeg, nSegment);
		else {
			if (gameData.segs.SegDist (nStartSeg, nSegment) < 0)
				continue;
			SetLightVis (nLight, nSegment);
			}
		}
	}
}

//------------------------------------------------------------------------------

static void ComputeSegmentVisibilityST (int32_t startI)
{
	int32_t i, endI;

if (gameStates.app.bMultiThreaded)
	endI = gameData.segs.nSegments;
else
	INIT_PROGRESS_LOOP (startI, endI, gameData.segs.nSegments);
if (startI < 0)
	startI = 0;
for (i = startI; i < endI; i++)
	ComputeSingleSegmentVisibility (0, i, 0, 5, 0);
}

//------------------------------------------------------------------------------

void ComputeSegmentVisibilityMT (int32_t startI, int32_t nThread)
{
	int32_t endI;

for (int32_t i = GetLoopLimits (startI, endI, gameData.segs.nSegments, nThread); i < endI; i++)
	ComputeSingleSegmentVisibility (nThread, i, 0, 5, 0);
}

//------------------------------------------------------------------------------
// This function checks whether segment nDestSeg has a chance to receive diffuse (direct)
// light from the light source at (nLightSeg,nSide).

static void CheckLightVisibility (int16_t nLight, int16_t nDestSeg, fix xMaxDist, float fRange, int32_t nThread)
{
#if DBG
if ((lightManager.Lights (nLight)->info.nSegment == nDbgSeg) && (lightManager.Lights (nLight)->info.nSide == nDbgSide))
	BRP;
#endif

	CDynLight* lightP = lightManager.Lights (nLight);
	int16_t nLightSeg = lightP->info.nSegment;

#if DBG
	CArray<uint8_t>& lightVis = gameData.segs.bLightVis;
#else
	uint8_t*			lightVis = gameData.segs.bLightVis.Buffer ();
#endif

	int32_t i;

	fix dPath = 0;

#if DBG
if (nLightSeg == nDbgSeg)
	BRP;
if (nDestSeg == nDbgSeg)
	BRP;
#endif
i = gameData.segs.LightVisIdx (nLight, nDestSeg);
#if DBG
if (i / 4 >= int32_t (gameData.segs.bLightVis.Length ()))
	return;
#endif
uint8_t* flagP = &lightVis [i >> 2];
uint8_t flag = 3 << ((i & 3) << 1);

if ((0 < dPath) || // path from light to dest blocked
	 (lightP->info.bSelf && (nLightSeg != nDestSeg)) || // light only illuminates its own face
	 (CFixVector::Dist (SEGMENTS [nLightSeg].Center (), SEGMENTS [nDestSeg].Center ()) - SEGMENTS [nLightSeg].MaxRad () - SEGMENTS [nDestSeg].MaxRad () >= xMaxDist)) { // distance to great

#ifdef _OPENMP
#	pragma omp atomic
#endif
	*flagP &= ~flag;
	return;
	}
if (*flagP & flag) // face visible 
	return;

	CHitQuery		hitQuery (FQ_TRANSWALL | FQ_TRANSPOINT | FQ_VISIBILITY, &VERTICES [0], &VERTICES [0], nLightSeg, -1, 1, 0);
	CHitResult		hitResult;
	CFloatVector	v0, v1;
	CSegment*		segP = SEGMENTS + nLightSeg;
	CSide*			sideP = segP->m_sides;
	fix				d, dMin = 0x7FFFFFFF;

// cast rays from light segment (corners and center) to target segment and see if at least one of them isn't blocked by geometry
segP = SEGMENTS + nDestSeg;
for (i = 4; i >= -4; i--) {
	if (i == 4) 
		v0.Assign (sideP->Center ());
	else if (i >= 0) 
		v0 = FVERTICES [sideP->m_corners [i]];
	else 
		v0 = CFloatVector::Avg (FVERTICES [sideP->m_corners [4 + i]], FVERTICES [sideP->m_corners [(5 + i) & 3]]); // center of face's edges
	for (int32_t j = 8; j >= 0; j--) {
		if (j == 8)
			v1.Assign (*hitQuery.p1);
		else if (segP->m_vertices [j] == 0xFFFF)
			continue;
		else
			v1 = FVERTICES [segP->m_vertices [j]];
		if ((d = CFixVector::Dist (*hitQuery.p0, *hitQuery.p1)) > xMaxDist)
			continue;
		if (dMin > d)
			dMin = d;
		if (PointSeesPoint (&v0, &v1, nLightSeg, nDestSeg, 0, nThread)) {
			gameData.segs.SetLightVis (nLight, nDestSeg, 2);
			return;
			}
		}
	}
if (dMin > xMaxDist)
	return;
gameData.segs.SetLightVis (nLight, nDestSeg, 1);
}

//------------------------------------------------------------------------------

static bool InitLightVisibility (int32_t nStage)
{
if (nStage) {
	// Why does this get called here? I don't remember, but it seems to be unnecessary. [DM]
	//SetupSegments (fix (I2X (1) * 0.001f)); 
	int32_t i = sizeof (gameData.segs.bVertVis [0]) * gameData.segs.nVertices * VERTVIS_FLAGS;
	if (!gameData.segs.bVertVis.Create (i)) 
		return false;
	}
else {
	gameData.segs.bVertVis.Clear ();
	if (lightManager.GeometryLightCount ()) {
		if (!gameData.segs.bLightVis.Create ((lightManager.GeometryLightCount () * LEVEL_SEGMENTS + 3) / 4))
			return false;
		gameData.segs.bLightVis.Clear ();
		}
	}
return true;
}

//------------------------------------------------------------------------------

static void ComputeSingleLightVisibility (int32_t nLight, int32_t nThread)
{
CDynLight* lightP = lightManager.Lights () + nLight;
if ((lightP->info.nSegment >= 0) && (lightP->info.nSide >= 0)) {
#if DBG
	if ((nDbgSeg >= 0) && (lightP->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (lightP->info.nSide == nDbgSide)))
		BRP;
#endif
#if FAST_LIGHTVIS
	for (int32_t i = 1; i <= 5; i++)
		ComputeSingleSegmentVisibility (nThread, lightP->Index (), lightP->info.nSide, lightP->info.nSide, i);
#endif
#if 1
	fix xLightRange = fix (MAX_LIGHT_RANGE * lightP->info.fRange);
	for (int32_t i = 0; i < gameData.segs.nSegments; i++)
		CheckLightVisibility (lightP->Index (), i, xLightRange, lightP->info.fRange, nThread);
#endif
	}
}

//------------------------------------------------------------------------------

static void ComputeLightVisibilityST (int32_t startI)
{
	int32_t endI;

if (gameStates.app.bMultiThreaded)
	endI = lightManager.LightCount (0);
else
	INIT_PROGRESS_LOOP (startI, endI, lightManager.LightCount (0));
if (startI < 0)
	startI = 0;
// every segment can see itself and its neighbours

for (int32_t i = startI; i < endI; i++)
	ComputeSingleLightVisibility (i, 0);
}

//------------------------------------------------------------------------------

void ComputeLightVisibilityMT (int32_t startI, int32_t nThread)
{
	int32_t endI;

for (int32_t i = GetLoopLimits (startI, endI, lightManager.LightCount (0), nThread); i < endI; i++)
	ComputeSingleLightVisibility (i, nThread);
}

//------------------------------------------------------------------------------

void ComputeLightsVisibleVertices (int32_t startI)
{
	int32_t	i, endI;

for (i = GetLoopLimits (startI, endI, lightManager.LightCount (0), 0); i < endI; i++)
	lightManager.Lights (i)->ComputeVisibleVertices (0);
}

//------------------------------------------------------------------------------

int32_t SegDistGaugeSize (void)
{
return PROGRESS_STEPS (gameData.segs.nSegments);
}

//------------------------------------------------------------------------------

int32_t SortLightsGaugeSize (void)
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

char *LightDataFilename (char *pszFilename, int32_t nLevel)
{
return GameDataFilename (pszFilename, "light", nLevel, -1);
}

//------------------------------------------------------------------------------

static int32_t LoadSegDistData (CFile& cf, tLightDataHeader& ldh)
{
	int32_t nDistances = 0;

for (int32_t i = 0; i < ldh.nSegments; i++) {
	if (!gameData.segs.segDistTable [i].Read (cf, ldh.bCompressed))
		return 0;
	CSegDistList& segDist = gameData.segs.segDistTable [i];
	for (uint16_t nSegment = segDist.offset; nSegment < gameData.segs.nSegments; nSegment++) {
		if (segDist.Get (nSegment) >= 0)
			nDistances++;
		}
	}
return nDistances;
}

//------------------------------------------------------------------------------

int32_t LoadLightData (int32_t nLevel)
{
	CFile					cf;
	tLightDataHeader	ldh;
	int32_t					bOk, nDistances = 0;
	char					szFilename [FILENAME_LEN];

if (!gameStates.app.bCacheLights)
	return 0;
if (!cf.Open (LightDataFilename (szFilename, nLevel), gameFolders.var.szLightData, "rb", 0) &&
	 !cf.Open (LightDataFilename (szFilename, nLevel), gameFolders.var.szCache, "rb", 0))
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
	bOk = (gameData.segs.bSegVis.Read (cf, gameData.segs.SegVisSize (ldh.nSegments), 0, ldh.bCompressed) == size_t (gameData.segs.SegVisSize (ldh.nSegments))) &&
			(gameData.segs.bLightVis.Read (cf, size_t ((ldh.nLights * ldh.nSegments + 3) / 4), 0, ldh.bCompressed) == size_t ((ldh.nLights * ldh.nSegments + 3) / 4)) &&
			(nDistances = LoadSegDistData (cf, ldh)) &&
			(lightManager.NearestSegLights  ().Read (cf, ldh.nSegments * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nSegments * MAX_NEAREST_LIGHTS)) &&
			(lightManager.NearestVertLights ().Read (cf, ldh.nVertices * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nVertices * MAX_NEAREST_LIGHTS));
cf.Close ();
PrintLog (0, "Read light data (%d lights, %d distance values)\n", ldh.nLights, nDistances);
return bOk;
}

//------------------------------------------------------------------------------

static int32_t SaveSegDistData (CFile& cf, tLightDataHeader& ldh)
{
	int32_t nDistances = 0;

for (int32_t i = 0; i < ldh.nSegments; i++) {
	if (!gameData.segs.segDistTable [i].Write (cf, ldh.bCompressed))
		return 0;
	CSegDistList& segDist = gameData.segs.segDistTable [i];
	for (uint16_t nSegment = segDist.offset; nSegment < gameData.segs.nSegments; nSegment++) {
		if (segDist.Get (nSegment) >= 0)
			nDistances++;
		}
	}
return nDistances;
}

//------------------------------------------------------------------------------

int32_t SaveLightData (int32_t nLevel)
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
	int32_t				bOk, nDistances = 0;
	char				szFilename [FILENAME_LEN];

if (!gameStates.app.bCacheLights)
	return 0;
if (!cf.Open (LightDataFilename (szFilename, nLevel), gameFolders.var.szLightData, "wb", 0))
	return 0;
bOk = (cf.Write (&ldh, sizeof (ldh), 1) == 1) &&
		(gameData.segs.bSegVis.Write (cf, gameData.segs.SegVisSize (ldh.nSegments), 0, ldh.bCompressed) == size_t (gameData.segs.SegVisSize (ldh.nSegments))) &&
		(gameData.segs.bLightVis.Write (cf, size_t ((ldh.nLights * ldh.nSegments + 3) / 4), 0, ldh.bCompressed) == size_t ((ldh.nLights * ldh.nSegments + 3) / 4)) &&
		(nDistances = SaveSegDistData (cf, ldh)) &&
		(lightManager.NearestSegLights  ().Write (cf, ldh.nSegments * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nSegments * MAX_NEAREST_LIGHTS)) &&
		(lightManager.NearestVertLights ().Write (cf, ldh.nVertices * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nVertices * MAX_NEAREST_LIGHTS));
cf.Close ();
PrintLog (0, "Saved light data (%d lights, %d distance values)\n", ldh.nLights, nDistances);
return bOk;
}

#if MULTI_THREADED_PRECALC //---------------------------------------------------
#ifdef _OPENMP //---------------------------------------------------------------

void _CDECL_ SegVisThread (int32_t nId)
{
ComputeSegmentVisibilityMT (nId * (gameData.segs.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ LightVisThread (int32_t nId)
{
ComputeLightVisibilityMT (nId * (lightManager.LightCount (0) + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ SegDistThread (int32_t nId)
{
ComputeSegmentDistance (nId * (gameData.segs.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ SegLightsThread (int32_t nId)
{
ComputeNearestSegmentLights (nId * (gameData.segs.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ VertLightsThread (int32_t nId)
{
ComputeNearestVertexLights (nId * (gameData.segs.nVertices + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

static void StartLightPrecalcThreads (pThreadFunc threadFunc)
{
#pragma omp parallel for
for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
	threadFunc (i);
}

//------------------------------------------------------------------------------

static int32_t PrecalcLightPollMT (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
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


int32_t _CDECL_ SegDistThread (void *pThreadId)
{
	int32_t nId = *(reinterpret_cast<int32_t*> (pThreadId));

ComputeSegmentDistance (nId * (gameData.segs.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
return 0;
}

//------------------------------------------------------------------------------

int32_t _CDECL_ SegLightsThread (void *pThreadId)
{
	int32_t nId = *(reinterpret_cast<int32_t*> (pThreadId));

ComputeNearestSegmentLights (nId * (gameData.segs.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
return 0;
}

//------------------------------------------------------------------------------

int32_t _CDECL_ VertLightsThread (void *pThreadId)
{
	int32_t nId = *(reinterpret_cast<int32_t*> (pThreadId));

ComputeNearestVertexLights (nId * (gameData.segs.nVertices + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
return 0;
}

//------------------------------------------------------------------------------

static void StartLightPrecalcThreads (pThreadFunc threadFunc)
{
	int32_t	i;

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

static int32_t PrecalcLightPollST (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
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

void ComputeNearestLights (int32_t nLevel)
{
//if (gameStates.app.bNostalgia)
//	return;
if (!(SHOW_DYN_LIGHT ||
	  (gameStates.render.bAmbientColor && !gameStates.render.bColored) ||
	   !COMPETITION))
	return;
loadOp = 0;
loadIdx = 0;

if (!InitLightVisibility (0))
	throw (EX_OUT_OF_MEMORY);

PrintLog (1, "Looking for precompiled light data\n");
if (LoadLightData (nLevel)) {
	PrintLog (-1);
	return;
	}
PrintLog (-1);

if (!InitLightVisibility (1))
	throw (EX_OUT_OF_MEMORY);
SetupProjection ();

#if DBG
nDistCount = 0;
nSavedCount = 0;
#endif

#ifdef _OPENMP
if (gameStates.app.bMultiThreaded && (gameData.segs.nSegments > 15)) {
	gameData.physics.side.bCache = 0;
	if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
		ProgressBar (TXT_LOADING, 0, 6, PrecalcLightPollMT);
	else {
		PrintLog (0, "Computing segment visibility\n");
		StartLightPrecalcThreads (SegVisThread);
		PrintLog (0, "Computing segment distances\n");
		StartLightPrecalcThreads (SegDistThread);
		PrintLog (0, "Computing light visibility\n");
		StartLightPrecalcThreads (LightVisThread);
		PrintLog (0, "Starting segment light calculation threads\n");
		StartLightPrecalcThreads (SegLightsThread);
		PrintLog (0, "Starting vertex light calculation threads\n");
		StartLightPrecalcThreads (VertLightsThread);
		PrintLog (0, "Computing vertices visible to lights\n");
		ComputeLightsVisibleVertices (-1);
		}
	gameData.physics.side.bCache = 1;
	}
else 
#endif
	{
	int32_t bMultiThreaded = gameStates.app.bMultiThreaded;
	gameStates.app.bMultiThreaded = 0;
	if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
		ProgressBar (TXT_LOADING,
						 LoadMineGaugeSize () + PagingGaugeSize (),
						 LoadMineGaugeSize () + PagingGaugeSize () + SortLightsGaugeSize () + SegDistGaugeSize (), PrecalcLightPollST);
	else {
		PrintLog (0, "Computing segment visibility\n");
		ComputeSegmentVisibilityST (-1);
		PrintLog (0, "Computing segment distances\n");
		ComputeSegmentDistance (-1, 0);
		PrintLog (0, "Computing light visibility\n");
		ComputeLightVisibilityST (-1);
		PrintLog (0, "Computing segment lights\n");
		ComputeNearestSegmentLights (-1, 0);
		PrintLog (0, "Computing vertex lights\n");
		ComputeNearestVertexLights (-1, 0);
		PrintLog (0, "Computing vertices visible to lights\n");
		ComputeLightsVisibleVertices (-1);
		}
	gameStates.app.bMultiThreaded = bMultiThreaded;
	}

gameData.segs.bVertVis.Destroy ();
PrintLog (1, "Saving precompiled light data\n");
#if DBG
PrintLog (0, "Distance table usage = %1.2f %%\n", 
			 100.0f * float (nDistCount) / (float (gameData.segs.nSegments) * float (gameData.segs.nSegments) - float (nSavedCount)));
#endif
SaveLightData (nLevel);
PrintLog (-1);
}

//------------------------------------------------------------------------------
//eof
