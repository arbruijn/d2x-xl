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

int32_t SegmentIsVisible (CSegment *pSeg, CTransformation& transformation, int32_t nThread);

//------------------------------------------------------------------------------

#define LIGHT_DATA_VERSION 34

#define	VERTVIS(_nSegment, _nVertex) \
	(gameData.segData.bVertVis.Buffer () ? gameData.segData.bVertVis [(_nSegment) * VERTVIS_FLAGS + ((_nVertex) >> 3)] & (1 << ((_nVertex) & 7)) : 0)

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
static int32_t nLevel = 0;
static int32_t bSecret = 0;

//------------------------------------------------------------------------------

int32_t CSegmentData::SegDist (uint16_t i, uint16_t j, const CFixVector& vStart, const CFixVector& vDest) 
{
int16_t nStartSeg, nDestSeg;
int32_t nDistance = segDistTable [i].Get (j, &nStartSeg, &nDestSeg);
if (nDistance < 0)
	return nDistance;
if ((nStartSeg < 0) && (nDestSeg < 0))
	return CFixVector::Dist (vStart, vDest);
if (nDestSeg < 0)
	return CFixVector::Dist (SEGMENT (nStartSeg)->Center (), vStart) + CFixVector::Dist (SEGMENT (nStartSeg)->Center (), vDest);
return nDistance + CFixVector::Dist (SEGMENT (nStartSeg)->Center (), vStart) + CFixVector::Dist (SEGMENT (nDestSeg)->Center (), vDest);
}

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
ENTER (0, nThread);

	fix xMaxDist = 0;
	float scale = 1.0f;
	int16_t nMinSeg = -1, nMaxSeg = -1;
	CDACSUniDirRouter& router = uniDacsRouter [nThread];

G3_SLEEP (0);
#if DBG
if (nSegment == nDbgSeg)
	BRP;
#endif
//PrintLog (0, "[%d] ComputeSingleSegmentDistance\n", nThread);
router.PathLength (CFixVector::ZERO, nSegment, CFixVector::ZERO, -1, I2X (1024), WID_TRANSPARENT_FLAG | WID_PASSABLE_FLAG, -1);
for (int32_t i = 0; i < gameData.segData.nSegments; i++) {
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

CSegDistList& segDist = gameData.segData.segDistTable [nSegment];
segDist.offset = nMinSeg;
segDist.length = nMaxSeg - nMinSeg + 1;
segDist.scale = scale;
#if DBG
if (nSegment == nDbgSeg)
	BRP;
#endif
//PrintLog (0, "[%d] creating distance vector for segment %d, length = %d\n", nThread, nSegment, segDist.length);
if (!segDist.Create ())
	return;
//PrintLog (0, "[%d] storing distances for segment %d (nMinSeg = %d, nMaxSeg = %d)\n", nThread, nSegment, nMinSeg, nMaxSeg);
segDist.Set (nSegment, 0, -1, -1);
for (int32_t i = nMinSeg; i <= nMaxSeg; i++) {
	fix xDistance = router.Distance (i);
	int16_t nStartSeg, nDestSeg;
	//PrintLog (0, "[%d] retrieving route length for segment %d\n", nThread, i);
	int16_t l = (xDistance < 0) ? 0 : router.RouteLength (i);
	//PrintLog (0, "[%d] route segment count for segment %d is %d\n", nThread, i, l);
	if (l < 3) {
		nStartSeg = nDestSeg = -1;
		if (l < 0)
			xDistance = -1;
		}
	else {
		nStartSeg = router.Route (1)->nNode;
		nDestSeg = (l == 3) ? nStartSeg : router.Route (l - 2)->nNode;
		}
	segDist.Set ((uint16_t) i, xDistance, nStartSeg, nDestSeg);
	}
#if DBG
nSavedCount += gameData.segData.nSegments - segDist.length;
#endif
RETURN
}

//------------------------------------------------------------------------------

void ComputeSegmentDistance (int32_t startI, int32_t nThread)
{
	int32_t endI;

uniDacsRouter [nThread].Create (gameData.segData.nSegments);
for (int32_t i = GetLoopLimits (startI, endI, gameData.segData.nSegments, nThread); i < endI; i++)
	ComputeSingleSegmentDistance (i, nThread);
}

//------------------------------------------------------------------------------

typedef struct tLightDist {
	int32_t		nIndex;
	int32_t		nDist;
} tLightDist;

void QSortLightDist (tLightDist *pDist, int32_t left, int32_t right)
{
	int32_t		l = left,
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

static inline int32_t IsLightVert (int32_t nVertex, CSegFace *pFace)
{
uint16_t *pv = (gameStates.render.bTriangleMesh ? pFace->triIndex : pFace->m_info.index);
for (int32_t i = pFace->m_info.nVerts; i; i--, pv++)
	if (*pv == (uint16_t) nVertex)
		return 1;
return 0;
}

//------------------------------------------------------------------------------

int32_t ComputeNearestSegmentLights (int32_t i, int32_t nThread)
{
ENTER (0, nThread);

	CSegment				*pSeg;
	CDynLight			*pLight;
	int32_t				h, j, k, l, n, nMaxLights;
	CFixVector			center;
	struct tLightDist	*pDists;

if (!lightManager.LightCount (0))
	RETVAL (0)

if (!(pDists = new tLightDist [lightManager.LightCount (0)])) {
	gameOpts->render.nLightingMethod = 0;
	gameData.renderData.shadows.nLights = 0;
	RETVAL (0)
	}

nMaxLights = MAX_NEAREST_LIGHTS;
i = GetLoopLimits (i, j, gameData.segData.nSegments, nThread);
for (pSeg = SEGMENT (i); i < j; i++, pSeg++) {
	center = pSeg->Center ();
	pLight = lightManager.Lights ();
	for (l = n = 0; l < lightManager.LightCount (0); l++, pLight++) {
		if (!((pLight->info.nSegment < 0) ? gameData.segData.SegVis (OBJECT (pLight->info.nObject)->info.nSegment, i) : gameData.segData.LightVis (pLight->Index (), i)))
			continue;
		h = int32_t (CFixVector::Dist (center, pLight->info.vPos) - F2X (pLight->info.fRad));
		if (h < 0)
			h = 0;
		else if (h > MAX_LIGHT_RANGE * pLight->info.fRange)
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
RETVAL (1)
}

//------------------------------------------------------------------------------

#if DBG
extern int32_t nDbgVertex;
#endif

int32_t ComputeNearestVertexLights (int32_t nVertex, int32_t nThread)
{
ENTER (0, nThread);

	CFixVector			*pVertex;
	CDynLight			*pLight;
	int32_t				h, j, k, l, n, nMaxLights;
	CFixVector			vLightToVert;
	struct tLightDist	*pDists;

G3_SLEEP (0);

if (!lightManager.LightCount (0))
	RETVAL (0)

if (!(pDists = new tLightDist [lightManager.LightCount (0)])) {
	gameOpts->render.nLightingMethod = 0;
	gameData.renderData.shadows.nLights = 0;
	RETVAL (0)
	}

#if DBG
if (nVertex == nDbgVertex)
	BRP;
#endif

nMaxLights = MAX_NEAREST_LIGHTS;
nVertex = GetLoopLimits (nVertex, j, gameData.segData.nVertices, nThread);

for (pVertex = gameData.segData.vertices + nVertex; nVertex < j; nVertex++, pVertex++) {
#if DBG
	if (nVertex == nDbgVertex)
		BRP;
#endif
	pLight = lightManager.Lights ();
	for (l = n = 0; l < lightManager.LightCount (0); l++, pLight++) {
#if DBG
		if (pLight->info.nSegment == nDbgSeg)
			BRP;
#endif
		if (IsLightVert (nVertex, pLight->info.pFace))
			h = 0;
		else {
			h = (pLight->info.nSegment < 0) ? OBJECT (pLight->info.nObject)->info.nSegment : pLight->info.nSegment;
			if (!VERTVIS (h, nVertex))
				continue;
			vLightToVert = *pVertex - pLight->info.vPos;
			h = CFixVector::Normalize (vLightToVert) - (int32_t) (pLight->info.fRad * 6553.6f);
			if (h > MAX_LIGHT_RANGE * pLight->info.fRange)
				continue;
#if 0
			if ((pLight->info.nSegment >= 0) && (pLight->info.nSide >= 0)) {
				CSide* pSide = SEGMENT (pLight->info.nSegment)->m_sides + pLight->info.nSide;
				if ((CFixVector::Dot (pSide->m_normals [0], vLightToVert) < 0) &&
					 ((pSide->m_nType == SIDE_IS_QUAD) || (CFixVector::Dot (pSide->m_normals [1], vLightToVert) < 0))) {
					h = simpleRouter [nThread].PathLength (VERTICES [nVertex], -1, pLight->info.vPos, pLight->info.nSegment, X2I (xMaxLightRange / 5), WID_TRANSPARENT_FLAG | WID_PASSABLE_FLAG, 0);
					if (h > 4 * MAX_LIGHT_RANGE / 3 * pLight->info.fRange)
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
RETVAL (1)
}

//------------------------------------------------------------------------------

inline int32_t SetVertVis (int16_t nSegment, int16_t nVertex, uint8_t b)
{
if (gameData.segData.bVertVis.Buffer ()) {
	uint8_t* flagP = gameData.segData.bVertVis.Buffer (nSegment * VERTVIS_FLAGS + (nVertex >> 3));
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
if (gameData.segData.SegVis (nStartSeg, nSegment))
	return;
while (!gameData.segData.SetSegVis (nStartSeg, nSegment))
	;
}

//------------------------------------------------------------------------------

static void SetLightVis (int16_t nLight, int16_t nSegment)
{
ENTER (0, 0);
#if DBG
if ((lightManager.Lights (nLight)->info.nSegment == nDbgSeg) && (lightManager.Lights (nLight)->info.nSide == nDbgSide))
	BRP;
if (gameData.segData.LightVisIdx (nLight, nSegment) / 4 >= int32_t (gameData.segData.bLightVis.Length ()))
	RETURN
#endif
while (!gameData.segData.SetLightVis (nLight, nSegment, 2))
	;

	tSegFaces		*pSegFace = SEGFACES + nSegment;
	CSegFace			*pFace;
	tFaceTriangle	*pTriangle;
	int32_t			i, nFaces, nTris;
	int16_t			nStartSeg = lightManager.Lights (nLight)->info.nSegment;

for (nFaces = pSegFace->nFaces, pFace = pSegFace->pFace; nFaces; nFaces--, pFace++) {
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
#endif
	if (gameStates.render.bTriangleMesh) {
		for (nTris = pFace->m_info.nTris, pTriangle = FACES.tris + pFace->m_info.nTriIndex; nTris; nTris--, pTriangle++)
			for (i = 0; i < 3; i++)
				while (!SetVertVis (nStartSeg, pTriangle->index [i], 1))
					;
		}
	else {
		for (i = 0; i < 4; i++)
			while (!SetVertVis (nStartSeg, pFace->m_info.index [i], 1))
				;
		}
	}
RETURN
}

//------------------------------------------------------------------------------
// This function must be called ahead of computing segment to segment visibility
// when multi-threading is used because only thread 0 has a valid OpenGL context!

static CTransformation projection;

static void SetupProjection (void)
{
ENTER (0, 0);
	int32_t	w = gameData.renderData.screen.Width (),
				h = gameData.renderData.screen.Height ();
	CCanvas canvas;

ogl.ClearError (0);
gameData.renderData.screen.Setup (NULL, 0, 0, 1024, 1024);
canvas.Setup (&gameData.renderData.screen);
canvas.Activate ("SetupProjection");
canvas.SetWidth ();
canvas.SetHeight ();
gameData.renderData.screen.SetWidth (1024);
gameData.renderData.screen.SetHeight (1024);
SetupCanvasses ();
ogl.SetTransform (1);
ogl.SetViewport (0, 0, 1024, 1024);
gameStates.render.nShadowPass = 1;	// enforce culling of segments behind viewer
ogl.ClearError (0);
SetupTransformation (projection, CFixVector::ZERO, CFixMatrix::IDENTITY, gameStates.render.xZoom, -1, 0, true);
ogl.ClearError (0);
gameStates.render.nShadowPass = 0;
gameStates.render.nShadowMap = 0;
ogl.SetTransform (0);
canvas.Deactivate ();
gameData.renderData.screen.Setup (NULL, 0, 0, w, h);
gameData.renderData.screen.SetWidth (w);
gameData.renderData.screen.SetHeight (h);
ogl.ClearError (0);
ogl.EndFrame (-1);
RETURN
}

//------------------------------------------------------------------------------
// Check segment to segment visibility by calling the renderer's visibility culling routine
// Do this for each side of the current segment, using the side Normal (s) as forward vector
// of the viewer

static void ComputeSingleSegmentVisibility (int32_t nThread, int16_t nStartSeg, int16_t nFirstSide, int16_t nLastSide, int32_t bLights)
{
ENTER (0, nThread);

	CSegment				*pStartSeg;
	CSide					*pSide;
	int16_t				nSegment, nSide, nLight = -1, i;
	CFixVector			fVec, uVec, rVec;
	CObject				viewer;
	CTransformation	transformation = projection;
	CVisibilityData&	visibility = gameData.renderData.mine.visibility [nThread];

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
pStartSeg = SEGMENT (nStartSeg);
pSide = pStartSeg->m_sides + nFirstSide;
	
viewer.info.nSegment = nStartSeg;
gameData.SetViewer (&viewer);

transformation.ComputeAspect (1024, 1024);

for (nSide = nFirstSide; nSide <= nLastSide; nSide++, pSide++) {
#if DBG
	pSide = pStartSeg->m_sides + nSide;
#endif
	fVec = pSide->m_normals [2];
	if (!bLights) {
		viewer.info.position.vPos = SEGMENT (nStartSeg)->Center ();// + fVec;
		fVec.Neg (); // point from segment center outwards
		rVec = CFixVector::Avg (VERTICES [pSide->m_corners [0]], VERTICES [pSide->m_corners [1]]) - pSide->Center ();
		CFixVector::Normalize (rVec);
		}
	else { // point from side center across the segment
		if (bLights == 5) { // from side center, pointing to normal
			viewer.info.position.vPos = pSide->Center () + fVec;
			rVec = CFixVector::Avg (VERTICES [pSide->m_corners [0]], VERTICES [pSide->m_corners [1]]) - pSide->Center ();
			CFixVector::Normalize (rVec);
			}
		else { // from side corner, pointing to average between vector from center to corner and side normal
#if DBG
			if ((nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nDbgSide == nSide)))
				BRP;
#endif
			viewer.info.position.vPos = VERTICES [pSide->m_corners [bLights - 1]];
			CFixVector h = pSide->Center () - viewer.info.position.vPos;
			CFixVector::Normalize (h);
			fVec = CFixVector::Avg (fVec, h);
			CFixVector::Normalize (fVec);
			rVec = pSide->m_normals [2];
			h.Neg ();
			rVec = CFixVector::Avg (rVec, h);
			CFixVector::Normalize (rVec);
			}
		}
#if 0
	CFixVector::Cross (uVec, fVec, rVec); // create uVec perpendicular to fVec and rVec
	CFixVector::Cross (rVec, fVec, uVec); // adjust rVec to be perpendicular to fVec and uVec (eliminate rounding errors)
	CFixVector::Cross (uVec, fVec, rVec); // adjust uVec to be perpendicular to fVec and rVec (eliminate rounding errors)
	CFixVector::Normalize (rVec);
	CFixVector::Normalize (uVec);
	CFixVector::Normalize (fVec);
	viewer.info.position.mOrient = CFixMatrix::Create (rVec, uVec, fVec);
#else
	viewer.info.position.mOrient = CFixMatrix::CreateF (fVec);
#endif
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
		if (nSegment >= gameData.segData.nSegments)
			continue;
#endif
		if (!SegmentIsVisible (SEGMENT (nSegment), transformation, nThread))
			continue;
		if (!bLights)
			SetSegVis (nStartSeg, nSegment);
		else {
			if (gameData.segData.SegDist (nStartSeg, nSegment, CFixVector::ZERO, CFixVector::ZERO) < 0)
				continue;
			SetLightVis (nLight, nSegment);
			}
		}
	}
RETURN
}

//------------------------------------------------------------------------------

static void ComputeSegmentVisibilityST (int32_t startI)
{
ENTER (0, 0);
	int32_t i, endI;

if (gameStates.app.bMultiThreaded)
	endI = gameData.segData.nSegments;
else
	INIT_PROGRESS_LOOP (startI, endI, gameData.segData.nSegments);
if (startI < 0)
	startI = 0;
for (i = startI; i < endI; i++)
	ComputeSingleSegmentVisibility (0, i, 0, 5, 0);
RETURN
}

//------------------------------------------------------------------------------

void ComputeSegmentVisibilityMT (int32_t startI, int32_t nThread)
{
ENTER (0, nThread);
	int32_t endI;

for (int32_t i = GetLoopLimits (startI, endI, gameData.segData.nSegments, nThread); i < endI; i++)
	ComputeSingleSegmentVisibility (nThread, i, 0, 5, 0);
RETURN
}

//------------------------------------------------------------------------------
// This function checks whether segment nDestSeg has a chance to receive diffuse (direct)
// light from the light source at (nLightSeg,nSide).

static void CheckLightVisibility (int16_t nLight, int16_t nDestSeg, fix xMaxDist, float fRange, int32_t nThread)
{
ENTER (0, nThread);
#if DBG
if ((lightManager.Lights (nLight)->info.nSegment == nDbgSeg) && (lightManager.Lights (nLight)->info.nSide == nDbgSide))
	BRP;
#endif

	CDynLight* pLight = lightManager.Lights (nLight);
	int16_t nLightSeg = pLight->info.nSegment;

#if DBG
	CArray<uint8_t>& lightVis = gameData.segData.bLightVis;
#else
	uint8_t*			lightVis = gameData.segData.bLightVis.Buffer ();
#endif

	int32_t i;

	fix dPath = 0;

#if DBG
if (nLightSeg == nDbgSeg)
	BRP;
if (nDestSeg == nDbgSeg)
	BRP;
#endif
i = gameData.segData.LightVisIdx (nLight, nDestSeg);
#if DBG
if (i / 4 >= int32_t (gameData.segData.bLightVis.Length ()))
	RETURN
#endif
uint8_t* flagP = &lightVis [i >> 2];
uint8_t flag = 3 << ((i & 3) << 1);

if ((0 < dPath) || // path from light to dest blocked
	 (pLight->info.bSelf && (nLightSeg != nDestSeg)) || // light only illuminates its own face
	 (CFixVector::Dist (SEGMENT (nLightSeg)->Center (), SEGMENT (nDestSeg)->Center ()) - SEGMENT (nLightSeg)->MaxRad () - SEGMENT (nDestSeg)->MaxRad () >= xMaxDist)) { // distance to great

#ifdef _OPENMP
#	pragma omp atomic
#endif
	*flagP &= ~flag;
	RETURN
	}
if (*flagP & flag) // face visible 
	RETURN

	CHitQuery		hitQuery (FQ_TRANSWALL | FQ_TRANSPOINT | FQ_VISIBILITY, &VERTICES [0], &VERTICES [0], nLightSeg, -1, 1, 0);
	CHitResult		hitResult;
	CFloatVector	v0, v1;
	CSegment*		pSeg = SEGMENT (nLightSeg);
	CSide*			pSide = pSeg->m_sides;
	fix				d, dMin = 0x7FFFFFFF;

// cast rays from light segment (corners and center) to target segment and see if at least one of them isn't blocked by geometry
pSeg = SEGMENT (nDestSeg);
for (i = 4; i >= -4; i--) {
	if (i == 4) 
		v0.Assign (pSide->Center ());
	else if (i >= 0) 
		v0 = FVERTICES [pSide->m_corners [i]];
	else 
		v0 = CFloatVector::Avg (FVERTICES [pSide->m_corners [4 + i]], FVERTICES [pSide->m_corners [(5 + i) & 3]]); // center of face's edges
	for (int32_t j = 8; j >= 0; j--) {
		if (j == 8)
			v1.Assign (*hitQuery.p1);
		else if (pSeg->m_vertices [j] == 0xFFFF)
			continue;
		else
			v1 = FVERTICES [pSeg->m_vertices [j]];
		if ((d = CFixVector::Dist (*hitQuery.p0, *hitQuery.p1)) > xMaxDist)
			continue;
		if (dMin > d)
			dMin = d;
		if (PointSeesPoint (&v0, &v1, nLightSeg, nDestSeg, -1, 0, nThread)) {
			gameData.segData.SetLightVis (nLight, nDestSeg, 2);
			RETURN
			}
		}
	}
if (dMin > xMaxDist)
	RETURN
gameData.segData.SetLightVis (nLight, nDestSeg, 1);
RETURN
}

//------------------------------------------------------------------------------

static bool InitLightVisibility (int32_t nStage)
{
ENTER (0, 0);
if (nStage) {
	// Why does this get called here? I don't remember, but it seems to be unnecessary. [DM]
	//SetupSegments (fix (I2X (1) * 0.001f)); 
	int32_t i = sizeof (gameData.segData.bVertVis [0]) * gameData.segData.nVertices * VERTVIS_FLAGS;
	if (!gameData.segData.bVertVis.Create (i)) 
		RETVAL (false);
	}
else {
	gameData.segData.bVertVis.Clear ();
	if (lightManager.GeometryLightCount ()) {
		if (!gameData.segData.bLightVis.Create ((lightManager.GeometryLightCount () * LEVEL_SEGMENTS + 3) / 4))
			RETVAL (false);
		gameData.segData.bLightVis.Clear ();
		}
	}
RETVAL (true);
}

//------------------------------------------------------------------------------

static void ComputeSingleLightVisibility (int32_t nLight, int32_t nThread)
{
ENTER (0, nThread);
CDynLight* pLight = lightManager.Lights () + nLight;
if ((pLight->info.nSegment >= 0) && (pLight->info.nSide >= 0)) {
#if DBG
	if ((nDbgSeg >= 0) && (pLight->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pLight->info.nSide == nDbgSide)))
		BRP;
#endif
#if FAST_LIGHTVIS
	for (int32_t i = 1; i <= 5; i++)
		ComputeSingleSegmentVisibility (nThread, pLight->Index (), pLight->info.nSide, pLight->info.nSide, i);
#endif
#if 1
	fix xLightRange = fix (MAX_LIGHT_RANGE * pLight->info.fRange);
	for (int32_t i = 0; i < gameData.segData.nSegments; i++)
		CheckLightVisibility (pLight->Index (), i, xLightRange, pLight->info.fRange, nThread);
#endif
	}
RETURN
}

//------------------------------------------------------------------------------

static void ComputeLightVisibilityST (int32_t startI)
{
ENTER (0, 0);
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
RETURN
}

//------------------------------------------------------------------------------

void ComputeLightVisibilityMT (int32_t startI, int32_t nThread)
{
ENTER (0, nThread);
	int32_t endI;

for (int32_t i = GetLoopLimits (startI, endI, lightManager.LightCount (0), nThread); i < endI; i++)
	ComputeSingleLightVisibility (i, nThread);
RETURN
}

//------------------------------------------------------------------------------

void ComputeLightsVisibleVertices (int32_t startI)
{
ENTER (0, 0);
	int32_t	i, endI;

for (i = GetLoopLimits (startI, endI, lightManager.LightCount (0), 0); i < endI; i++)
	lightManager.Lights (i)->ComputeVisibleVertices (0);
RETURN
}

//------------------------------------------------------------------------------

int32_t SegDistGaugeSize (void)
{
return PROGRESS_STEPS (gameData.segData.nSegments);
}

//------------------------------------------------------------------------------

int32_t SortLightsGaugeSize (void)
{
//if (gameStates.app.bNostalgia)
//	return 0;
if (gameStates.app.bMultiThreaded)
	return 0;
return PROGRESS_STEPS (gameData.segData.nSegments) * 2 + 
		 PROGRESS_STEPS (gameData.segData.nVertices) + 
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
ENTER (0, 0);
	int32_t nDistances = 0;

for (int32_t i = 0; i < ldh.nSegments; i++) {
#if DBG
	if (i == nDbgSeg)
		BRP;
#endif
	if (!gameData.segData.segDistTable [i].Read (cf, ldh.bCompressed))
		RETVAL (0)
	CSegDistList& segDist = gameData.segData.segDistTable [i];
	for (uint16_t nSegment = segDist.offset; nSegment < segDist.offset + segDist.length; nSegment++) {
		if (segDist.Get (nSegment) >= 0)
			nDistances++;
		}
	}
RETVAL (nDistances);
}

//------------------------------------------------------------------------------

int32_t LoadLightData (int32_t nLevel)
{
ENTER (0, 0);
	CFile					cf;
	tLightDataHeader	ldh;
	int32_t				bOk, nDistances = 0;
	char					szFilename [FILENAME_LEN];

if (!gameStates.app.bCacheLights)
	RETVAL (0)
if (!cf.Open (LightDataFilename (szFilename, nLevel), gameFolders.var.szLightData, "rb", 0) &&
	 !cf.Open (LightDataFilename (szFilename, nLevel), gameFolders.var.szCache, "rb", 0))
	RETVAL (0)
bOk = (cf.Read (&ldh, sizeof (ldh), 1) == 1);
if (bOk)
	bOk = (ldh.nVersion == LIGHT_DATA_VERSION)
			&& (ldh.nCheckSum == CalcSegmentCheckSum ())
			&& (ldh.nSegments == gameData.segData.nSegments)
			&& (ldh.nVertices == gameData.segData.nVertices)
			&& (ldh.nLights == lightManager.LightCount (0))
			&& (ldh.nMaxLightRange == MAX_LIGHT_RANGE)
#if 0
			&& (ldh.nMethod = LightingMethod () &&
			&& ((ldh.bPerPixelLighting != 0) == (gameStates.render.bPerPixelLighting != 0)))
#endif
			;
if (bOk) 
	bOk = (gameData.segData.bSegVis.Read (cf, gameData.segData.SegVisSize (ldh.nSegments), 0, ldh.bCompressed) == size_t (gameData.segData.SegVisSize (ldh.nSegments))) &&
			(gameData.segData.bLightVis.Read (cf, size_t ((ldh.nLights * ldh.nSegments + 3) / 4), 0, ldh.bCompressed) == size_t ((ldh.nLights * ldh.nSegments + 3) / 4)) &&
			(nDistances = LoadSegDistData (cf, ldh)) &&
			(lightManager.NearestSegLights  ().Read (cf, ldh.nSegments * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nSegments * MAX_NEAREST_LIGHTS)) &&
			(lightManager.NearestVertLights ().Read (cf, ldh.nVertices * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nVertices * MAX_NEAREST_LIGHTS));
cf.Close ();
PrintLog (0, "Read light data (%d lights, %d distance values)\n", ldh.nLights, nDistances);
RETVAL (bOk);
}

//------------------------------------------------------------------------------

static int32_t SaveSegDistData (CFile& cf, tLightDataHeader& ldh)
{
ENTER (0, 0);
	int32_t nDistances = 0;

for (int32_t i = 0; i < ldh.nSegments; i++) {
#if DBG
	if (i == nDbgSeg)
		BRP;
#endif
	if (!gameData.segData.segDistTable [i].Write (cf, ldh.bCompressed))
		RETVAL (0)
	CSegDistList& segDist = gameData.segData.segDistTable [i];
	for (uint16_t nSegment = segDist.offset; nSegment < segDist.offset + segDist.length; nSegment++) {
		if (segDist.Get (nSegment) >= 0)
			nDistances++;
		}
	}
RETVAL (nDistances);
}

//------------------------------------------------------------------------------

int32_t SaveLightData (int32_t nLevel)
{
ENTER (0, 0);
	CFile				cf;
	tLightDataHeader ldh = {LIGHT_DATA_VERSION,
									CalcSegmentCheckSum (),
								   gameData.segData.nSegments,
								   gameData.segData.nVertices,
								   lightManager.LightCount (0),
									MAX_LIGHT_RANGE,
									LightingMethod (),
									gameStates.render.bPerPixelLighting,
									gameStates.app.bCompressData
									};
	int32_t				bOk, nDistances = 0;
	char				szFilename [FILENAME_LEN];

if (!gameStates.app.bCacheLights)
	RETVAL (0)
if (!cf.Open (LightDataFilename (szFilename, nLevel), gameFolders.var.szLightData, "wb", 0))
	RETVAL (0)
bOk = (cf.Write (&ldh, sizeof (ldh), 1) == 1) &&
		(gameData.segData.bSegVis.Write (cf, gameData.segData.SegVisSize (ldh.nSegments), 0, ldh.bCompressed) == size_t (gameData.segData.SegVisSize (ldh.nSegments))) &&
		(gameData.segData.bLightVis.Write (cf, size_t ((ldh.nLights * ldh.nSegments + 3) / 4), 0, ldh.bCompressed) == size_t ((ldh.nLights * ldh.nSegments + 3) / 4)) &&
		(nDistances = SaveSegDistData (cf, ldh)) &&
		(lightManager.NearestSegLights  ().Write (cf, ldh.nSegments * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nSegments * MAX_NEAREST_LIGHTS)) &&
		(lightManager.NearestVertLights ().Write (cf, ldh.nVertices * MAX_NEAREST_LIGHTS, 0, ldh.bCompressed) == size_t (ldh.nVertices * MAX_NEAREST_LIGHTS));
cf.Close ();
PrintLog (0, "Saved light data (%d lights, %d distance values)\n", ldh.nLights, nDistances);
RETVAL (bOk);
}

#if MULTI_THREADED_PRECALC //---------------------------------------------------
#ifdef _OPENMP //---------------------------------------------------------------

void _CDECL_ SegVisThread (int32_t nId)
{
ComputeSegmentVisibilityMT (nId * (gameData.segData.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ LightVisThread (int32_t nId)
{
ComputeLightVisibilityMT (nId * (lightManager.LightCount (0) + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ SegDistThread (int32_t nId)
{
nId = omp_get_thread_num ();
ComputeSegmentDistance (nId * (gameData.segData.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ SegLightsThread (int32_t nId)
{
ComputeNearestSegmentLights (nId * (gameData.segData.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

void _CDECL_ VertLightsThread (int32_t nId)
{
ComputeNearestVertexLights (nId * (gameData.segData.nVertices + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
}

//------------------------------------------------------------------------------

static void StartLightPrecalcThreads (pThreadFunc threadFunc)
{
#	pragma omp parallel for
for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
	threadFunc (i);
}

//------------------------------------------------------------------------------

static int32_t PrecalcLightPollMT (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
ENTER (0, 0);
if (nState)
	RETVAL (nCurItem);

//paletteManager.ResumeEffect ();
if (loadOp == 0) {
	loadOp = 1;
	menu [0].Rebuild ();
	key = 0;
	RETVAL (nCurItem);
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
	RETVAL (nCurItem);
	}
menu [0].Value ()++;
menu [0].Rebuild ();
key = 0;
RETVAL (nCurItem);
}

#else // _OPENMP ---------------------------------------------------------------

static CThreadInfo	ti [MAX_THREADS];


int32_t _CDECL_ SegDistThread (void *pThreadId)
{
	int32_t nId = *(reinterpret_cast<int32_t*> (pThreadId));

ENTER (0, nId);
ComputeSegmentDistance (nId * (gameData.segData.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
RETVAL (0)
}

//------------------------------------------------------------------------------

int32_t _CDECL_ SegLightsThread (void *pThreadId)
{
	int32_t nId = *(reinterpret_cast<int32_t*> (pThreadId));

ENTER (0, nId);
ComputeNearestSegmentLights (nId * (gameData.segData.nSegments + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
RETVAL (0)
}

//------------------------------------------------------------------------------

int32_t _CDECL_ VertLightsThread (void *pThreadId)
{
	int32_t nId = *(reinterpret_cast<int32_t*> (pThreadId));

ENTER (0, nId);
ComputeNearestVertexLights (nId * (gameData.segData.nVertices + gameStates.app.nThreads - 1) / gameStates.app.nThreads, nId);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
RETVAL (0)
}

//------------------------------------------------------------------------------

#if 0

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

#endif

#endif // _OPENMP --------------------------------------------------------------
#endif //MULTI_THREADED_PRECALC ------------------------------------------------

static int32_t PrecalcLightPollST (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
ENTER (0, 0);
if (nState)
	RETVAL (nCurItem);

//paletteManager.ResumeEffect ();
if (loadOp == 0) {
	if (loadIdx == 0)
		PrintLog (0, "computing segment visibility\n");
	ComputeSegmentVisibilityST (loadIdx);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segData.nSegments) {
		loadIdx = 0;
		loadOp = 1;
		}
	}
else if (loadOp == 1) {
	if (loadIdx == 0)
		PrintLog (0, "computing segment distances \n");
	ComputeSegmentDistance (loadIdx, 0);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segData.nSegments) {
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
	if (loadIdx >= gameData.segData.nSegments) {
		loadIdx = 0;
		loadOp = 4;
		}
	}
else if (loadOp == 4) {
	if (loadIdx == 0)
		PrintLog (0, "computing nearest vertex lights\n");
	ComputeNearestVertexLights (loadIdx, 0);
	loadIdx += PROGRESS_INCR;
	if (loadIdx >= gameData.segData.nVertices) {
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
	RETVAL (nCurItem);
	}
menu [0].Value ()++;
menu [0].Rebuild ();
key = 0;
//paletteManager.ResumeEffect ();
RETVAL (nCurItem);
}

//------------------------------------------------------------------------------

void ComputeNearestLights (int32_t nLevel)
{
ENTER (0, 0);
if (!(SHOW_DYN_LIGHT ||
	  (gameStates.render.bAmbientColor && !gameStates.render.bColored) ||
	   !COMPETITION))
	RETURN
loadOp = 0;
loadIdx = 0;

if (!InitLightVisibility (0))
	throw (EX_OUT_OF_MEMORY);

PrintLog (1, "Looking for precompiled light data\n");
if (LoadLightData (nLevel)) {
	PrintLog (-1);
	RETURN
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
if (gameStates.app.bMultiThreaded && (gameData.segData.nSegments > 15)) {
	gameData.physicsData.side.bCache = 0;
	if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
		ProgressBar (TXT_LOADING, 1, 0, 6, PrecalcLightPollMT);
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
	gameData.physicsData.side.bCache = 1;
	}
else 
#endif
	{
	int32_t bMultiThreaded = gameStates.app.bMultiThreaded;
	gameStates.app.bMultiThreaded = 0;
	if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
		ProgressBar (TXT_LOADING, 1,
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

gameData.segData.bVertVis.Destroy ();
PrintLog (1, "Saving precompiled light data\n");
#if DBG
PrintLog (0, "Distance table usage = %1.2f %%\n", 
			 100.0f * float (nDistCount) / (float (gameData.segData.nSegments) * float (gameData.segData.nSegments) - float (nSavedCount)));
#endif
SaveLightData (nLevel);
PrintLog (-1);
}

//------------------------------------------------------------------------------

int32_t PrecomputeLevelLightmaps (int32_t nLevel)
{
ENTER (0, 0);
int32_t nResult = LoadLevel (nLevel, false, false) >= 0;
CleanupAfterGame (false);
RETVAL (nResult);
}

//------------------------------------------------------------------------------

const char *LightmapQualityText (void);
const char *LightmapPrecisionText (void);

int32_t PrecomputeLightmapsPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
ENTER (0, 0);
if (nState) {
	if (nState == -1)
		RETVAL (9);

	if (nState == -2) {
#if 0
		char szLabel [50];
		int32_t h = menu.AddText ("", "settings:");
		sprintf (szLabel, "%s / %s", LightmapQualityText (), LightmapPrecisionText ());
		menu.AddText ("", szLabel);
		menu.AddText ("", "");
		menu.AddText ("", "total progress");
		for (int32_t i = 0; i < 4; i++)
			menu [h + i].m_bCentered = 1;
#endif
		RETVAL (0)
		}

	if (nState == -3) {
		int32_t h = menu.AddText ("", "");
		menu.AddText ("", "time:");
		menu.AddText ("time", "00:00:00 / 00:00:00");
		menu.AddText ("", "");
		menu.AddText ("", "levels:");
		menu.AddText ("level count", "00 / 00");
		menu.AddText ("", "");
		menu.AddText ("", "settings:");
		char szLabel [50];
		sprintf (szLabel, "%s / %s", LightmapQualityText (), LightmapPrecisionText ());
		menu.AddText ("", szLabel);
		for (int32_t i = 0; i < 9; i++)
			menu [h + i].m_bCentered = 1;
		RETVAL (0)
		}

	RETVAL (nCurItem);
	}

int32_t bProgressBars = gameStates.app.bProgressBars;
gameStates.app.bProgressBars = 0;
if (!PrecomputeLevelLightmaps (bSecret * nLevel)) {
	key = -2;
	gameStates.app.bProgressBars = bProgressBars;
	RETVAL (nCurItem);
	}
gameStates.app.bProgressBars = bProgressBars;
if (bSecret > 0) {
	if (++nLevel > missionManager.nLastLevel) {
		bSecret = -1;
		nLevel = 0;
		}
	}
if (bSecret < 0) {
	if (++nLevel > -missionManager.nLastSecretLevel) {
		key = -2;
		RETVAL (nCurItem);
		}
	}
#if 0
menu [0].Value ()++;
menu [0].Rebuild ();
#endif
RETVAL (nCurItem);
}

//------------------------------------------------------------------------------

void PrecomputeMissionLightmaps (void)
{
ENTER (0, 0);
gameStates.app.bPrecomputeLightmaps = 1;

int32_t bUseLightmaps = gameOpts->render.bUseLightmaps;
int32_t bPerPixelLighting = gameStates.render.bPerPixelLighting;
int32_t nLightmapQuality = gameOpts->render.nLightmapQuality;
int32_t nLightmapPrecision = gameOpts->render.nLightmapPrecision;

gameOpts->render.bUseLightmaps = 1;
gameStates.render.bPerPixelLighting = 1;

SetFunctionMode (FMODE_GAME);
nLevel = NewGameMenu ();
if (nLevel > -1) {
	if (gameStates.app.bProgressBars) {
		bSecret = 1;
		lightmapManager.ResetProgress ();
		ProgressBar (TXT_CALC_LIGHTMAPS, 2, 0, 
						 (missionManager.nLastLevel - nLevel + 1 - missionManager.nLastSecretLevel) * lightmapManager.Progress ().Scale (), 
						 PrecomputeLightmapsPoll);
		lightmapManager.ResetProgress ();
		}
	else {
		for (; nLevel <= missionManager.nLastLevel; nLevel++)
			PrecomputeLevelLightmaps (nLevel);
		for (nLevel = 1; nLevel <= missionManager.nLastSecretLevel; nLevel++)
			PrecomputeLevelLightmaps (-nLevel);
		}
	}
SetFunctionMode (FMODE_MENU);

gameOpts->render.bUseLightmaps = bUseLightmaps;
gameStates.render.bPerPixelLighting = bPerPixelLighting;
gameOpts->render.nLightmapQuality = nLightmapQuality;
gameOpts->render.nLightmapPrecision = nLightmapPrecision;

gameStates.app.bPrecomputeLightmaps = 0;
RETURN
}

//------------------------------------------------------------------------------
//eof
