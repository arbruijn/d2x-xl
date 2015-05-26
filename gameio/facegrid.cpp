#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#define LIGHT_VERSION 4

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"

#include "descent.h"
#include "error.h"

#include "byteswap.h"
#include "vecmat.h"
#include "maths.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool CGridFace::LineIntersects (CFixVector& v1, CFixVector& v2)
{
if (!FindPlaneLineIntersection (m_vIntersect, m_vertices, &m_vNormal, &v1, &v2, 0))
	return false;
if (PointToFaceRelation (&m_vIntersect, m_vertices, 3, &m_vNormal))
	return false;
m_xDist = CFixVector::Dist (v1, m_vIntersect);
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CFaceGridSegment::CFaceGridSegment () : m_pParent (NULL), m_pFaces (NULL), m_nFaces (0)
{
memset (m_pChildren, 0, sizeof (m_pChildren));
m_vMin.Set (0x7fffffff, 0x7fffffff, 0x7fffffff);
m_vMax.Set (-0x7fffffff, -0x7fffffff, -0x7fffffff);
m_nVisited = 0;
}

//------------------------------------------------------------------------------

void CFaceGridSegment::Setup (CFaceGridSegment *pParent, CFixVector& vMin, CFixVector& vMax)
{
m_pParent = pParent;
m_vMin = vMin;
m_vMax = vMax;
m_corners [0] = m_vMin;
m_corners [1].Set (m_vMax.v.coord.x, m_vMin.v.coord.y, m_vMin.v.coord.z);
m_corners [2].Set (m_vMax.v.coord.x, m_vMax.v.coord.y, m_vMin.v.coord.z);
m_corners [3].Set (m_vMin.v.coord.x, m_vMax.v.coord.y, m_vMin.v.coord.z);
m_corners [4].Set (m_vMin.v.coord.x, m_vMin.v.coord.y, m_vMax.v.coord.z);
m_corners [5].Set (m_vMax.v.coord.x, m_vMin.v.coord.y, m_vMax.v.coord.z);
m_corners [6] = m_vMax;
m_corners [7].Set (m_vMin.v.coord.x, m_vMax.v.coord.y, m_vMax.v.coord.z);

	static int32_t nFaces [6][4] = {{0,1,2,3},{1,5,6,2},{5,4,7,6},{2,0,3,7},{3,2,6,7},{0,1,5,4}};

for (int32_t i = 0; i < 6; i++) {
	for (int32_t j = 0; j < 3; j++)
		m_faces [i].m_vertices [j] = m_corners [nFaces [i][j]];
	m_faces [i].m_vNormal = CFixVector::Normal (m_faces [i].m_vertices [0], m_faces [i].m_vertices [1], m_faces [i].m_vertices [2]);
	}
}

//------------------------------------------------------------------------------

bool CFaceGridSegment::ContainsPoint (CFixVector& v)
{
return (v > m_vMin) && (v < m_vMax);
}

//------------------------------------------------------------------------------
// Check whether the line between v1 and v2 intersects with at least one of the grid segment's faces

bool CFaceGridSegment::ContainsLine (CFixVector& v1, CFixVector& v2)
{
	static int32_t nFaces [6][4] = {{0,1,2,3},{1,5,6,2},{5,4,7,6},{2,0,3,7},{3,2,6,7},{0,1,5,4}};

if (ContainsPoint (v1) || ContainsPoint (v2))
	return true;
for (int32_t i = 0; i < 6; i++) {
	CFixVector vIntersect;
	if (FindPlaneLineIntersection (vIntersect, m_faces [i].m_vertices, &m_faces [i].m_vNormal, &v1, &v2, 0) &&
		 !PointToFaceRelation (&vIntersect, m_faces [i].m_vertices, 3, &m_faces [i].m_vNormal))
		return true;
	}
return false;
}

//------------------------------------------------------------------------------
// check whether a triangle is at least partially inside a grid segment

bool CFaceGridSegment::ContainsTriangle (CFixVector vertices [])
{
	CFixVector vMin, vMax;

vMin.Set (0x7fffffff, 0x7fffffff, 0x7fffffff);
vMax.Set (-0x7fffffff, -0x7fffffff, -0x7fffffff);
for (int32_t i = 0; i < 3; i++) {
	if (ContainsPoint (vertices [i]))
		return true; // vertex contained in grid segment
	vMin.Set (Min (vMin.v.coord.x, vertices [i].v.coord.x), Min (vMin.v.coord.x, vertices [i].v.coord.y), Min (vMin.v.coord.x, vertices [i].v.coord.z));
	vMax.Set (Max (vMax.v.coord.x, vertices [i].v.coord.x), Max (vMax.v.coord.x, vertices [i].v.coord.y), Max (vMax.v.coord.x, vertices [i].v.coord.z));
	}
if ((vMin > m_vMax) || (vMax < m_vMin))
	return false;
// check whether grid segment intersects triangle
CFixVector vNormal = CFixVector::Normal (vertices [0], vertices [1], vertices [2]), vPlane = (vertices [0] + vertices [1] + vertices [2]) / 3;

static int32_t nEdges [12][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};

for (int32_t i = 0; i < 12; i++) {
	CFixVector vIntersect;
	if (FindPlaneLineIntersection (vIntersect, &vPlane, &vNormal, &m_corners [nEdges [i][0]], &m_corners [nEdges [i][1]], 0) &&
		 !PointToFaceRelation (&vIntersect, vertices, 3, &vNormal))
		return true;
	}
return false;
}

//------------------------------------------------------------------------------

void CFaceGridSegment::InsertFace (CGridFace *pFace)
{
pFace->m_pNextFace = m_pFaces;
m_pFaces = pFace;
++m_nFaces;
}

//------------------------------------------------------------------------------

bool CFaceGridSegment::AddFace (CGridFace *pFace)
{
if (ContainsTriangle (pFace->m_vertices))
	InsertFace (pFace);
return true;
}

//------------------------------------------------------------------------------

bool CFaceGridSegment::AddFace (uint16_t nSegment, uint8_t nSide, CFixVector vertices [], CFixVector vNormal)
{
if (!ContainsTriangle (vertices))
	return true;
CGridFace *pFace = new CGridFace;
if (!pFace)
	return false;
memcpy (pFace->m_vertices, vertices, 3 * sizeof (vertices [0]));
pFace->m_vNormal = vNormal;
pFace->m_nSegment = nSegment;
pFace->m_nSide = nSide;
InsertFace (pFace);
return true;
}

//------------------------------------------------------------------------------

bool CFaceGridSegment::AddFace (uint16_t nSegment, uint8_t nSide, uint16_t vertices [], CFixVector vNormal)
{
CFixVector v [3];
for (int32_t i = 0; i < 3; i++)
	v [i] = gameData.segData.vertices [vertices [i]];
return AddFace (nSegment, nSide, v, vNormal);
}

//------------------------------------------------------------------------------

void CFaceGridSegment::Destroy (void)
{
for (int32_t i = 0; i < 8; i++) {
	if (m_pChildren [i]) {
		delete m_pChildren [i];
		m_pChildren [i] = NULL;
		}
	}

for (;;) {
	CGridFace *pFace = m_pFaces;
	if (!pFace)	
		break;
	m_pFaces = m_pFaces->m_pNextFace;
	delete pFace;
	}
m_nFaces = 0;
}

//------------------------------------------------------------------------------

CFaceGridSegment::~CFaceGridSegment ()
{
Destroy ();
}

//------------------------------------------------------------------------------

bool CFaceGridSegment::Split (int32_t nMaxFaces)
{
if (!m_pFaces)
	return true;
if (m_nFaces <= nMaxFaces)
	return true;

	CFixVector	vOffs = (m_vMax - m_vMin) / 2;

for (int32_t nChild = 0, z = 0; z < 2; z++) {
	for (int32_t y = 0; y < 2; y++) {
		for (int32_t x = 0; x < 2; x++, nChild++) {
			if (!(m_pChildren [nChild] = new CFaceGridSegment ()))
				return false;
			CFixVector vMin, vMax;
			vMin.Set (m_vMin.v.coord.x + vOffs.v.coord.x * x, m_vMin.v.coord.y + vOffs.v.coord.y * y, m_vMin.v.coord.z + vOffs.v.coord.z * z);
			vMax.Set (m_vMin.v.coord.x + vOffs.v.coord.x * (x + 1), m_vMin.v.coord.y + vOffs.v.coord.y * (y + 1), m_vMin.v.coord.z + vOffs.v.coord.z * (z + 1));
			m_pChildren [nChild]->Setup (this, vMin, vMax);
			}
		}
	}

for (;;) {
	CGridFace *pFace = m_pFaces;
	if (!pFace)
		break;
	m_pFaces = m_pFaces->m_pNextFace;
	--m_nFaces;
	for (int32_t i = 0; i < 8; i++) {
		if (!m_pChildren [i]->AddFace (pFace->m_nSegment, pFace->m_nSide, pFace->m_vertices, pFace->m_vNormal))
			return false;
		}
	}

for (int32_t i = 0; i < 8; i++) {
	if (m_pChildren [i]->m_pFaces && !m_pChildren [i]->Split (nMaxFaces))
		return false;
	}

return true;
}

//------------------------------------------------------------------------------

CFaceGridSegment *CFaceGridSegment::Origin (CFixVector& v)
{
if (!ContainsPoint (v))
	return NULL;
for (int32_t i = 0; i < 8; i++) {
	if (!m_pChildren [i])
		continue;
	CFaceGridSegment *pOrigin = m_pChildren [i]->Origin (v);
	if (pOrigin)
		return pOrigin;
	}
return this;
}

//------------------------------------------------------------------------------

CGridFace *CFaceGridSegment::Occluder (CFixVector& vStart, CFixVector& vEnd, CGridFace *pOccluder, int32_t nVisited)
{
if (m_nVisited == nVisited) 
	return pOccluder;
m_nVisited = nVisited;
if (m_pFaces)
	pOccluder = FindOccluder (vStart, vEnd, pOccluder);
else {
	for (int32_t i = 0; i < 8; i++) {
		if (m_pChildren [i])
			pOccluder = m_pChildren [i]->Occluder (vStart, vEnd, pOccluder, nVisited);
		}
	}
return m_pParent ? m_pParent->Occluder (vStart, vEnd, pOccluder, nVisited) : pOccluder;
}

//------------------------------------------------------------------------------

CGridFace *CFaceGridSegment::FindOccluder (CFixVector& vStart, CFixVector& vEnd, CGridFace *pOccluder)
{
fix xMinDist = 0x7fffffff;

for (CGridFace *pFace = m_pFaces; pFace; pFace = pFace->m_pNextFace) {
	if (pFace->LineIntersects (vStart, vEnd) && (!pOccluder || (pFace->m_xDist < pOccluder->m_xDist)))
		pOccluder = pFace;
	}
return pOccluder;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CFaceGrid::ComputeDimensions (int32_t nSize)
{
m_vMin.Set (0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF);
m_vMax.Set (-0x7FFFFFFF, -0x7FFFFFFF, -0x7FFFFFFF);

CSegment *pSeg = SEGMENT (0);
for (uint16_t i = gameData.segData.nSegments; i; i--, pSeg++) {
	if (pSeg->Function () != SEGMENT_FUNC_SKYBOX) {
		for (uint8_t j = (uint8_t) pSeg->m_nVertices; j; --j) {
			CFixVector v = gameData.segData.vertices [pSeg->m_vertices [j]];
			m_vMin.Set (Min (m_vMin.v.coord.x, v.v.coord.x), Min (m_vMin.v.coord.x, v.v.coord.y), Min (m_vMin.v.coord.x, v.v.coord.z));
			m_vMax.Set (Max (m_vMax.v.coord.x, v.v.coord.x), Max (m_vMax.v.coord.x, v.v.coord.y), Max (m_vMax.v.coord.x, v.v.coord.z));
			}
		}
	}
}

//------------------------------------------------------------------------------

bool CFaceGrid::Create (int32_t nSize)
{
#if !DBG
return false;
#else
ComputeDimensions (nSize);
if (!(m_pRoot = new CFaceGridSegment))
	return false;

CSegment *pSeg = SEGMENT (0);
for (uint16_t i = 0; i < gameData.segData.nSegments; i++, pSeg++) {
	if (pSeg->Function () != SEGMENT_FUNC_SKYBOX) {
		CSide *pSide = pSeg->Side (0);
		for (uint8_t j = 0; j < 6; j++, pSide++) {
			switch (pSide->Shape ()) {
				case SIDE_SHAPE_QUAD:
					if (!m_pRoot->AddFace (i, j, pSide->m_faceVerts + 3, pSide->m_normals [1]))
						return false;
					// fall through
				case SIDE_SHAPE_TRIANGLE:
					if (!m_pRoot->AddFace (i, j, pSide->m_faceVerts, pSide->m_normals [0]))
						return false;
					break;
				default:
					break;
				}
			CFixVector v = gameData.segData.vertices [pSeg->m_vertices [j]];
			m_vMin.Set (Min (m_vMin.v.coord.x, v.v.coord.x), Min (m_vMin.v.coord.x, v.v.coord.y), Min (m_vMin.v.coord.x, v.v.coord.z));
			m_vMax.Set (Max (m_vMax.v.coord.x, v.v.coord.x), Max (m_vMax.v.coord.x, v.v.coord.y), Max (m_vMax.v.coord.x, v.v.coord.z));
			}
		}
	}

m_pRoot->Setup (NULL, m_vMin, m_vMax);
return m_pRoot->Split ();
#endif
}

//------------------------------------------------------------------------------

CFaceGridSegment *CFaceGrid::Origin (CFixVector v)
{
return m_pRoot ? m_pRoot->Origin (v) : NULL;
}

//------------------------------------------------------------------------------

CGridFace *CFaceGrid::Occluder (CFixVector& vStart, CFixVector &vEnd)
{
CFaceGridSegment *pOrigin = Origin (vStart);
return pOrigin ? pOrigin->Occluder (vStart, vEnd, NULL, ++m_nVisited) : NULL;
}

//------------------------------------------------------------------------------
//eof
