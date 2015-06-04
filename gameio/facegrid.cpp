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
#include "error.h"

#include "byteswap.h"
#include "vecmat.h"
#include "maths.h"

//------------------------------------------------------------------------------
// Create an octree containing all triangles forming the static level mesh

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool CGridFace::LineIntersects (CFloatVector& v1, CFloatVector& v2)
{
if (!FindPlaneLineIntersectionf (m_vIntersect, m_vertices [0], m_vNormal, v1, v2, 0.0f))
	return false;
if (PointToFaceRelationf (m_vIntersect, m_vertices, 3, m_vNormal))
	return false;
m_fDist = CFloatVector::Dist (v1, m_vIntersect);
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CFaceGridSegment::CFaceGridSegment () : m_pParent (NULL), m_pFaces (NULL), m_nFaces (0)
{
memset (m_pChildren, 0, sizeof (m_pChildren));
m_vMin.Set (1e6f, 1e6f, 1e6f);
m_vMax.Set (-1e6f, -1e6f, -1e6f);
m_nVisited = 0;
}

//------------------------------------------------------------------------------

void CFaceGridSegment::Setup (CFaceGridSegment *pParent, CFloatVector& vMin, CFloatVector& vMax)
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
	m_faces [i].m_vNormal = CFloatVector::Normal (m_faces [i].m_vertices [0], m_faces [i].m_vertices [1], m_faces [i].m_vertices [2]);
	}
}

//------------------------------------------------------------------------------

bool CFaceGridSegment::ContainsPoint (CFloatVector& v)
{
return (v >= m_vMin) && (v <= m_vMax);
}

//------------------------------------------------------------------------------
// Check whether the line between v1 and v2 intersects with at least one of the grid segment's faces

bool CFaceGridSegment::LineIntersectsFace (CFloatVector *vertices, CFloatVector& vNormal, CFloatVector& v1, CFloatVector& v2)
{
CFloatVector vIntersect;
return FindPlaneLineIntersectionf (vIntersect, vertices [0], vNormal, v1, v2, 0.0f) && !PointToFaceRelationf (vIntersect, vertices, 3, vNormal);
}

//------------------------------------------------------------------------------
// Check whether the line between v1 and v2 intersects with at least one of the grid segment's faces

bool CFaceGridSegment::ContainsLine (CFloatVector& v1, CFloatVector& v2)
{
if (ContainsPoint (v1) || ContainsPoint (v2))
	return true;

CFloatVector vMin, vMax;
vMin.Set (Min (v1.v.coord.x, v2.v.coord.x), Min (v1.v.coord.x, v2.v.coord.y), Min (v1.v.coord.x, v2.v.coord.z));
vMax.Set (Max (v1.v.coord.x, v2.v.coord.x), Max (v1.v.coord.x, v2.v.coord.y), Max (v1.v.coord.x, v2.v.coord.z));
if (!(vMin <= m_vMax) || !(vMax >= m_vMin))
	return false;

for (int32_t i = 0; i < 6; i++) {
	if (LineIntersectsFace (m_faces [i].m_vertices, m_faces [i].m_vNormal, v1, v2))
		return true;
	}
return false;
}

//------------------------------------------------------------------------------
// check whether a triangle is at least partially inside a grid segment

bool CFaceGridSegment::ContainsTriangle (CFloatVector vertices [])
{
	CFloatVector vMin, vMax;

vMin.Set (1e6f, 1e6f, 1e6f);
vMax.Set (-1e6f, -1e6f, -1e6f);
for (int32_t i = 0; i < 3; i++) {
	if (ContainsPoint (vertices [i]))
		return true; // vertex contained in grid segment
	vMin.Set (Min (vMin.v.coord.x, vertices [i].v.coord.x), Min (vMin.v.coord.x, vertices [i].v.coord.y), Min (vMin.v.coord.x, vertices [i].v.coord.z));
	vMax.Set (Max (vMax.v.coord.x, vertices [i].v.coord.x), Max (vMax.v.coord.x, vertices [i].v.coord.y), Max (vMax.v.coord.x, vertices [i].v.coord.z));
	}
if (!(vMin <= m_vMax) || !(vMax >= m_vMin)) // Attention: !CFloatVector:operator>= != CFloatVector::operator<
	return false;
// check whether grid segment intersects triangle
CFloatVector vNormal = CFloatVector::Normal (vertices [0], vertices [1], vertices [2]);

static int32_t nEdges [12][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};

for (int32_t i = 0; i < 12; i++) {
	if (LineIntersectsFace (vertices, vNormal, m_corners [nEdges [i][0]], m_corners [nEdges [i][1]]))
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

bool CFaceGridSegment::AddFace (uint16_t nSegment, uint8_t nSide, CFloatVector vertices [], CFloatVector vNormal)
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

bool CFaceGridSegment::AddFace (uint16_t nSegment, uint8_t nSide, uint16_t vertices [], CFloatVector vNormal)
{
CFloatVector v [3];
for (int32_t i = 0; i < 3; i++)
	v [i] = gameData.segData.fVertices [vertices [i]];
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

	CFloatVector	vOffs;
	
vOffs.Set ((m_vMax.v.coord.x - m_vMin.v.coord.x) / 2, (m_vMax.v.coord.y - m_vMin.v.coord.y) / 2, (m_vMax.v.coord.z - m_vMin.v.coord.z) / 2);

for (int32_t nChild = 0, z = 0; z < 2; z++) {
	for (int32_t y = 0; y < 2; y++) {
		for (int32_t x = 0; x < 2; x++, nChild++) {
			if (!(m_pChildren [nChild] = new CFaceGridSegment ()))
				return false;
			CFloatVector vMin, vMax;
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

	bool bPropagated = false;
	for (int32_t i = 0; i < 8; i++) {
		if (bPropagated ? !m_pChildren [i]->AddFace (pFace->m_nSegment, pFace->m_nSide, pFace->m_vertices, pFace->m_vNormal) : !m_pChildren [i]->AddFace (pFace))
			return false;
		if (!bPropagated && (m_pChildren [i]->m_pFaces == pFace))
			bPropagated = true;
		}
#if DBG
	if (!bPropagated) {
		for (int32_t i = 0; i < 8; i++) {
			if (!m_pChildren [i]->AddFace (pFace->m_nSegment, pFace->m_nSide, pFace->m_vertices, pFace->m_vNormal))
				return false;
			}
		}
#endif
	}

for (int32_t i = 0; i < 8; i++) {
	if (m_pChildren [i]->m_pFaces) {
		if (!m_pChildren [i]->Split (nMaxFaces))
			return false;
		}
	else {
		delete m_pChildren [i];
		m_pChildren [i] = NULL;
		}
	}

return true;
}

//------------------------------------------------------------------------------

CFaceGridSegment *CFaceGridSegment::Origin (CFloatVector& v)
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

CGridFace *CFaceGridSegment::Occluder (CGridLine& line, CGridFace *pOccluder, int32_t nVisited)
{
if (m_nVisited == nVisited) 
	return pOccluder;
m_nVisited = nVisited;
if (m_pFaces)
	pOccluder = FindOccluder (line, pOccluder);
else {
	for (int32_t i = 0; i < 8; i++) {
		if (m_pChildren [i])
			pOccluder = m_pChildren [i]->Occluder (line, pOccluder, nVisited);
		}
	}
return m_pParent ? m_pParent->Occluder (line, pOccluder, nVisited) : pOccluder;
}

//------------------------------------------------------------------------------

CGridFace *CFaceGridSegment::FindOccluder (CGridLine& line, CGridFace *pOccluder)
{
for (CGridFace *pFace = m_pFaces; pFace; pFace = pFace->m_pNextFace) {
	if ((CFloatVector::Dot (line.m_vNormal, pFace->m_vNormal) <= 0) && pFace->LineIntersects (line.m_vStart, line.m_vEnd)) {
		if (!pOccluder)
			pOccluder = pFace;
		else if (pFace->m_fDist < pOccluder->m_fDist)
			pOccluder = pFace;
		else if ((pFace->m_fDist == pOccluder->m_fDist) && (pFace->m_nSegment == line.m_nSegment) && ((line.m_nSide < 0) || (pFace->m_nSide == line.m_nSide)))
			pOccluder = pFace;
		}
	}
return pOccluder;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CFaceGrid::ComputeDimensions (int32_t nSize)
{
m_vMin.Set (1e6f, 1e6f, 1e6f);
m_vMax.Set (-1e6f, -1e6f, -1e6f);

CSegment *pSeg = SEGMENT (0);
for (uint16_t i = gameData.segData.nSegments; i; i--, pSeg++) {
	if (pSeg->Function () != SEGMENT_FUNC_SKYBOX) {
		for (uint8_t j = (uint8_t) pSeg->m_nVertices; j; --j) {
			CFloatVector v = gameData.segData.fVertices [pSeg->m_vertices [j]];
			m_vMin.Set (Min (m_vMin.v.coord.x, v.v.coord.x), Min (m_vMin.v.coord.x, v.v.coord.y), Min (m_vMin.v.coord.x, v.v.coord.z));
			m_vMax.Set (Max (m_vMax.v.coord.x, v.v.coord.x), Max (m_vMax.v.coord.x, v.v.coord.y), Max (m_vMax.v.coord.x, v.v.coord.z));
			}
		}
	}
}

//------------------------------------------------------------------------------

CFaceGridSegment *CFaceGrid::Origin (CFloatVector v)
{
return m_pRoot ? m_pRoot->Origin (v) : NULL;
}

//------------------------------------------------------------------------------

CGridFace *CFaceGrid::Occluder (CFloatVector& vStart, CFloatVector &vEnd, int16_t nSegment, int8_t nSide)
{
CFaceGridSegment *pOrigin = Origin (vStart);
if (!pOrigin)
	return NULL;
CGridLine line;
line.m_vStart = vStart;
line.m_vEnd = vEnd;
line.m_vNormal = vEnd - vStart;
line.m_nSegment = nSegment;
line.m_nSide = nSide;
CFloatVector::Normalize (line.m_vNormal);
return pOrigin->Occluder (line, NULL, ++m_nVisited);
}

//------------------------------------------------------------------------------

bool CFaceGrid::PointSeesPoint (CFloatVector& vStart, CFloatVector &vEnd, int16_t nSegment, int8_t nSide)
{
CGridFace *pOccluder = Occluder (vStart, vEnd, nSegment, nSide);
return !pOccluder || ((pOccluder->m_nSegment == nSegment) && (pOccluder->m_nSide == nSide));
}

//------------------------------------------------------------------------------

bool CFaceGrid::AddFace (uint16_t nSegment, uint8_t nSide, uint16_t vertices [], CFloatVector vNormal)
{
	CFloatVector v [3];

for (int32_t i = 0; i < 3; i++) {
	v [i] = gameData.segData.fVertices [vertices [i]];
	m_vMin.Set (Min (m_vMin.v.coord.x, v [i].v.coord.x), Min (m_vMin.v.coord.x, v [i].v.coord.y), Min (m_vMin.v.coord.x, v [i].v.coord.z));
	m_vMax.Set (Max (m_vMax.v.coord.x, v [i].v.coord.x), Max (m_vMax.v.coord.x, v [i].v.coord.y), Max (m_vMax.v.coord.x, v [i].v.coord.z));
	}
return m_pRoot->AddFace (nSegment, nSide, v, vNormal);
}

//------------------------------------------------------------------------------

bool CFaceGrid::Create (int32_t nSize)
{
#if !DBG // don't build, octree based visibility tests are slower than those based on Descent's portal structure. Meh.
return false;
#else
ComputeDimensions (nSize);
if (!(m_pRoot = new CFaceGridSegment))
	return false;

m_vMin.Set (1e6f, 1e6f, 1e6f);
m_vMax.Set (-1e6f, -1e6f, -1e6f);

CSegment *pSeg = SEGMENT (0);

for (uint16_t i = 0; i < gameData.segData.nSegments; i++, pSeg++) {
	if (pSeg->Function () != SEGMENT_FUNC_SKYBOX) {
		for (uint8_t j = 0; j < pSeg->m_nVertices; j++) {
			CFloatVector v = gameData.segData.fVertices [pSeg->m_vertices [j]];
			m_vMin.Set (Min (m_vMin.v.coord.x, v.v.coord.x), Min (m_vMin.v.coord.x, v.v.coord.y), Min (m_vMin.v.coord.x, v.v.coord.z));
			m_vMax.Set (Max (m_vMax.v.coord.x, v.v.coord.x), Max (m_vMax.v.coord.x, v.v.coord.y), Max (m_vMax.v.coord.x, v.v.coord.z));
			}
		}
	}

m_pRoot->Setup (NULL, m_vMin, m_vMax);

pSeg = SEGMENT (0);

for (uint16_t i = 0; i < gameData.segData.nSegments; i++, pSeg++) {
	if (pSeg->Function () != SEGMENT_FUNC_SKYBOX) {
		CSide *pSide = pSeg->Side (0);
		for (uint8_t j = 0; j < 6; j++, pSide++) {
			if (pSeg->ChildId (j) >= 0) {
				CWall *pWall = pSide->Wall ();
				if (!pWall || pWall->IsVolatile () || (pWall->IsPassable (NULL, false) & WID_TRANSPARENT_FLAG))
					continue;
				}
			switch (pSide->Shape ()) {
				case SIDE_SHAPE_QUAD:
					if (!m_pRoot->AddFace (i, j, pSide->m_vertices + 3, pSide->m_fNormals [1]))
						return false;
					// fall through
				case SIDE_SHAPE_TRIANGLE:
					if (!m_pRoot->AddFace (i, j, pSide->m_vertices, pSide->m_fNormals [0]))
						return false;
					break;
				default:
					break;
				}
			}
		}
	}

m_pRoot->Setup (NULL, m_vMin, m_vMax);
return m_pRoot->Split ();
#endif
}

//------------------------------------------------------------------------------

void CFaceGrid::Destroy (void)
{
if (m_pRoot) {
	delete m_pRoot;
	m_pRoot = NULL;
	}
}

//------------------------------------------------------------------------------
//eof
