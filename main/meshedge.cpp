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
#include "segment.h"
#include "byteswap.h"
#include "vecmat.h"
#include "maths.h"

#if POLYGONAL_OUTLINE
bool bPolygonalOutline = false;
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32_t CMeshEdge::Visibility (void)
{
	CFloatVector		vViewDir, vViewer;
	
vViewer.Assign (gameData.objData.pViewer->Position ());

int32_t nVisible = 0;
for (int32_t j = 0; j < m_nFaces; j++) {
	vViewDir = vViewer - m_faces [j].m_vCenter [0];
	CFloatVector::Normalize (vViewDir);
	float dot = CFloatVector::Dot (vViewDir, Normal (j));
	if (dot >= 0.0f)
		nVisible |= 1 << j;
	}
return nVisible;
}

//------------------------------------------------------------------------------

int32_t CMeshEdge::Type (int32_t nDistScale)
{
#if DBG
if ((m_nVertices [0] == 110) && (m_nVertices [1] == 111)) {
	for (int32_t i = 0; i < m_nFaces; i++)
		if ((m_faces [i].m_nItem == nDbgSeg) && ((nDbgSide < 0) || (m_faces [i].m_nFace == nDbgSide)))
			BRP;
	}
#endif
for (int32_t i = 0; i < m_nFaces; i++)
	if (!m_faces [i].Visible ())
		return -1;
int32_t h = Visibility ();
return ((h == 0) ? -1 : (h != 3) ? 0 : Planar () ? -1 : Partial () ? 2 : 1);
}

//------------------------------------------------------------------------------

float CMeshEdge::PlanarAngle (void)
{
return 0.97f;
}

//------------------------------------------------------------------------------

float CMeshEdge::PartialAngle (void)
{
return 0.9f;
}

//------------------------------------------------------------------------------

int32_t CMeshEdge::Planar (void)
{
return m_fDot >= PlanarAngle ();
}

//------------------------------------------------------------------------------

int32_t CMeshEdge::Partial (void)
{
return m_fDot >= PartialAngle ();
}

//------------------------------------------------------------------------------

CFloatVector& CMeshEdge::Normal (int32_t i)
{
return m_faces [i].m_vNormal [0];
}

//------------------------------------------------------------------------------

CFloatVector& CMeshEdge::Vertex (int32_t i, int32_t bTransformed)
{
#if POLYGONAL_OUTLINE
if (bPolygonalOutline)
	return m_vertices [1][i]; // gameData.segData.fVertices [m_nVertices [i]];
else
#endif
	return m_vertices [bTransformed][i]; // gameData.segData.fVertices [m_nVertices [i]];
}

//------------------------------------------------------------------------------

void CMeshEdge::Transform (void)
{
#if POLYGONAL_OUTLINE
if (bPolygonalOutline) {
	for (int32_t i = 0; i < 2; i++)
		transformation.Transform (m_vertices [1][i], m_vertices [0][i]);
	}
#endif
}

//------------------------------------------------------------------------------

void CMeshEdge::Setup (void)
{
if (!m_bValid) {
	m_bValid = 1;
	for (int32_t i = 0; i < m_nFaces; i++)
		m_faces [i].Setup ();

	m_fScale = 1.0f;
	m_fSplit = 0.0f;
	m_vOffset.SetZero ();
	m_fDot = (m_nFaces == 1) ? 0.0f : fabs (CFloatVector::Dot (Normal (0), Normal (1)));
	if (Planar () && (m_faces [0].m_nTexture != m_faces [1].m_nTexture))
		m_fDot = PartialAngle () + (PlanarAngle () - PartialAngle ()) * 0.5f;
	if (Partial () && (m_faces [0].m_nTexture == m_faces [1].m_nTexture)) { 
		m_fScale = 0.5f + 0.25f * (1.0f - m_fDot * m_fDot);
		if (m_fScale < 0.75f) {
			if (Rand (8) == 0)
				m_fSplit = 0.3f + 0.4f * RandFloat ();
			else {
				m_fSplit = 0.0f;
				m_vOffset = Vertex (1);
				m_vOffset -= Vertex (0);
#if 1
				if (Rand (2) == 0)
					m_fOffset = 1.0f - m_fScale;
#else
				m_fOffset = (1.0f - m_fScale) * 0.25f;
				m_fOffset *= m_fOffset + (2.0f * m_fOffset) * RandFloat ();
#endif
				m_vOffset *= m_fOffset;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

int32_t CMeshEdge::Prepare (CFloatVector vViewer, int32_t nFilter, float fDistance)
{
#if DBG
if ((gameStates.render.nType == RENDER_TYPE_OBJECTS) && (m_nFaces < 2))
	BRP;
#endif

int32_t bAutoScale = (fDistance < 0.0f);
int32_t nDistScale = DistToScale (bAutoScale ? Min (CFloatVector::Dist (vViewer, Vertex (0)), CFloatVector::Dist (vViewer, Vertex (1))) : fDistance);

int32_t nType = Type (nDistScale);
if (nType < 0)
	return -1;
if (nType > nFilter)
	return -1;

#if 0 //DBG
if (gameStates.render.nType == RENDER_TYPE_OBJECTS) {
	if (nType)
		return;
#	if 0
	gameData.segData.edgeVertices [nVertices [0]++] = m_faces [0].m_vCenter [1];
	gameData.segData.edgeVertices [nVertices [0]++] = m_faces [0].m_vCenter [1] + m_faces [0].m_vNormal [1] * 0.5f;
	gameData.segData.edgeVertices [nVertices [0]++] = m_faces [1].m_vCenter [1];
	gameData.segData.edgeVertices [nVertices [0]++] = m_faces [1].m_vCenter [1] + m_faces [1].m_vNormal [1] * 0.5f;
#	endif
#	if 0
	return;
#	endif
	}
#endif

CFloatVector vertices [2];

int32_t bPartial = nType == 2;
int32_t bSplit = bPartial && (m_fSplit != 0.0f);
#if POLYGONAL_OUTLINE
float fLineWidths [2] = { automap.Active () ? 3.0f : 6.0f, automap.Active () ? 1.0f : 2.0f };
float wPixel = 2.0f / float (CCanvas::Current ()->Width ());
float fScale = Max (1.0f, float (CCanvas::Current ()->Width ()) / 640.0f);
#endif

#if DBG
if ((m_faces [0].m_nItem == 166) && (m_faces [0].m_nFace == 3) && (m_faces [1].m_nItem == 176) && (m_faces [1].m_nFace == 3))
	BRP;
#endif

for (int32_t n = bSplit ? 0 : 1; n < 2; n++) {
	for (int32_t j = 0; j < 2; j++) {
		vertices [j] = Vertex (j);
		CFloatVector v;
		if (j && bPartial) {
			v = vertices [1];
			v -= vertices [0];
			v *= m_fScale;
			if (bSplit) { // if outline is split, each partial outline starts at one of the edge's vertices
				v *= n ? 1.0f - m_fSplit : m_fSplit;
				if (n == 1) 
					vertices [0] = vertices [1] - v;
				else
					vertices [1] = vertices [0] + v;
				}
			else { // otherwise the outline is offset from the edge's start vertex
				vertices [1] = vertices [0] + v;
				vertices [0] += m_vOffset;
				vertices [1] += m_vOffset;
				}
			}
		}

	for (int32_t j = 0; j < 2; j++) {
		CFloatVector v;	// pull a bit closer to viewer to avoid z fighting with related polygon
		float l;
		if (gameStates.render.nType == RENDER_TYPE_OBJECTS)
			v = vertices [j];
		else {
			v = vViewer;
			v -= vertices [j];
			l = CFloatVector::Normalize (v);
#if POLYGONAL_OUTLINE
			if (bPolygonalOutline)
#endif
				//v *= 2.0f;
				//if (l > 1.0f)
			v /= pow (l, 0.25f);
#if POLYGONAL_OUTLINE
			if (bPolygonalOutline)
				vertices [j] += v; 
			else
#endif
			v += vertices [j]; 
			}
#if POLYGONAL_OUTLINE
		if (!bPolygonalOutline) 
#endif
#if DBG
		if ((gameData.segData.edgeVertexData [bPartial].m_nVertices >= (int32_t) gameData.segData.edgeVertexData [bPartial].m_vertices.Length ()) &&
			 !gameData.segData.edgeVertexData [bPartial].m_vertices.Resize (2 * gameData.segData.edgeVertexData [bPartial].m_nVertices))
			return -1;
#endif
		gameData.segData.edgeVertexData [bPartial].Add (v);
		}
	if (bAutoScale)
		gameData.segData.edgeVertexData [bPartial].SetDistance (nDistScale);
#if POLYGONAL_OUTLINE
	if (bPolygonalOutline) {
		CFloatVector p = vertices [0];
		p += vertices [1];
		p *= 0.5f;
		float l = p.Mag ();
		p = vertices [0];
		p -= vertices [1];
		CFloatVector::Normalize (p);
		p *= wPixel * fScale * fLineWidths [nType != 0] / l;
		vertices [0] += p;
		vertices [1] -= p;
		CFloatVector::Perp (p, vertices [0], vertices [1], CFloatVector::ZERO);
		p *= wPixel * fScale * fLineWidths [nType != 0] / l;
#if DBG
		if (nVertices [0] > nVertices [1])
			BRP;
#endif
		if (bPartial) {
			gameData.segData.edgeVertices [--nVertices [1]] = vertices [0] - p;
			gameData.segData.edgeVertices [--nVertices [1]] = vertices [0] + p;
			gameData.segData.edgeVertices [--nVertices [1]] = vertices [1] + p;
			gameData.segData.edgeVertices [--nVertices [1]] = vertices [1] - p;
			}
		else {
			gameData.segData.edgeVertices [nVertices [0]++] = vertices [0] - p;
			gameData.segData.edgeVertices [nVertices [0]++] = vertices [0] + p;
			gameData.segData.edgeVertices [nVertices [0]++] = vertices [1] + p;
			gameData.segData.edgeVertices [nVertices [0]++] = vertices [1] - p;
			}
#if DBG
		if (nVertices [0] > nVertices [1])
			BRP;
#endif
		}
#endif
	}
return bAutoScale ? 0 : nDistScale;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32_t CSegmentData::CountEdges (void)
{
	CSegment	*pSeg = gameData.Segment (0);
	int32_t	nEdges = 0;

for (int32_t i = 0; i < gameData.segData.nSegments; i++, pSeg++) {
	CSide* pSide = pSeg->Side (0);
	for (int32_t j = 0; j < 6; j++, pSide++) {
		if ((pSeg->ChildId (j) < 0) || (pSide->Wall () && !pSide->Wall ()->IsInvisible ())) {
			switch (pSide->Shape ()) {
				case SIDE_SHAPE_QUAD:
					nEdges += 4;
					break;
				case SIDE_SHAPE_TRIANGLE:
					nEdges += 3;
					break;
				}
			}
		}
	}
return nEdges;
}

//------------------------------------------------------------------------------

int32_t CSegmentData::FindEdge (int16_t nVertex1, int16_t nVertex2, int32_t nStart)
{
	CMeshEdge *pEdge = &edges [nStart];

for (int32_t i = nStart; i < nEdges; i++, pEdge++)
	if ((pEdge->m_nVertices [0] == nVertex1) && (pEdge->m_nVertices [1] == nVertex2))
		return i;
return -1;
}

//------------------------------------------------------------------------------

void SortEdgeFaces (CEdgeFaceInfo faces [])
{
if ((faces [1].m_nItem < faces [0].m_nItem) || ((faces [1].m_nItem == faces [0].m_nItem) && ((faces [1].m_nFace < faces [0].m_nFace)))) {
	Swap (faces [1].m_nItem, faces [0].m_nItem);
	Swap (faces [1].m_nFace, faces [0].m_nFace);
	}
}


int32_t CSegmentData::AddEdge (int16_t nSegment, int16_t nSide, int16_t nVertex1, int16_t nVertex2)
{
if (nVertex1 > nVertex2)
	Swap (nVertex1, nVertex2);

#if DBG
if ((nVertex1 == 110) && (nVertex2 == 111))
	BRP;
#endif

int32_t nEdge = FindEdge (nVertex1, nVertex2, 0);

int32_t i;

if (nEdge >= 0) {
	CMeshEdge *pEdge = &edges [nEdge];
	if (pEdge->m_nFaces < 2) {
		pEdge->m_nFaces = 2;
		pEdge->m_faces [1].Setup (nSegment, nSide);
		SortEdgeFaces (pEdge->m_faces);
		}
	else {
		// this edge shares vertices with another edge
		// create two new edges with the faces from the existing edge
#if DBG
		if ((nVertex1 == 110) && (nVertex2 == 111))
			BRP;
#endif
		int32_t nEdges = gameData.segData.nEdges;
		do {
			if ((nEdges >= (int32_t) edges.Length () - 1) && !edges.Resize (nEdges * 2))
				return -1;
			pEdge = &edges [nEdge];
			CMeshEdge *pNewEdge = &edges [nEdges++];
			memcpy (pNewEdge, pEdge, sizeof (CMeshEdge));
			pNewEdge->m_faces [0].Setup (nSegment, nSide);
			SortEdgeFaces (pNewEdge->m_faces);
			pNewEdge = &edges [nEdges++];
			memcpy (pNewEdge, pEdge, sizeof (CMeshEdge));
			pNewEdge->m_faces [1].Setup (nSegment, nSide);
			SortEdgeFaces (pNewEdge->m_faces);
			nEdge = FindEdge (nVertex1, nVertex2, nEdge + 1);
			} while (nEdge >= 0);
		gameData.segData.nEdges = nEdges;
		}
	i = 1;
	}
else {
	i = 0;
	if ((nEdges >= (int32_t) edges.Length ()) && !edges.Resize (nEdges * 2))
		return -1;
	CMeshEdge *pEdge = &edges [nEdges++];
	pEdge->m_nVertices [0] = nVertex1;
	pEdge->m_nVertices [1] = nVertex2;
	pEdge->m_vertices [0][0] = fVertices [nVertex1];
	pEdge->m_vertices [0][1] = fVertices [nVertex2];
	pEdge->m_nFaces = 1;
	pEdge->m_faces [0].Setup (nSegment, nSide);
	}
if (i == 0)
	return 1;
return 0;
}

//------------------------------------------------------------------------------

int32_t CSegmentData::CreateEdgeBuffers (int32_t nLength)
{
for (int32_t i = 0; i < 2; i++) {
	if ((gameData.segData.edgeVertexData [i].m_vertices.Length () < uint32_t (nLength)) && !gameData.segData.edgeVertexData [i].m_vertices.Resize (nLength, false))
		return 0;
	if ((gameData.segData.edgeVertexData [i].m_dists.Length () < uint32_t (nLength)) && !gameData.segData.edgeVertexData [i].m_dists.Resize (nLength, false))
		return 0;
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t CSegmentData::BuildEdgeList (void)
{
if (!gameData.segData.edges.Create (CountEdges ()))
	return -1;

CSegment	*pSeg = gameData.Segment (0);
gameData.segData.nEdges = 0;

for (int32_t i = 0; i < gameData.segData.nSegments; i++, pSeg++) {
	if (pSeg->m_function == SEGMENT_FUNC_SKYBOX)
		continue;
	CSide* pSide = pSeg->Side (0);
	for (int32_t j = 0; j < 6; j++, pSide++) {
#if DBG
		if ((i == nDbgSeg) && ((nDbgSide < 0) || (j == nDbgSide)))
			BRP;
#endif
		if ((pSeg->ChildId (j) >= 0) && !pSide->Wall ())
				continue;
		int32_t nVertices;
		switch (pSide->Shape ()) {
			case SIDE_SHAPE_QUAD:
				nVertices = 4;
				break;
			case SIDE_SHAPE_TRIANGLE:
				nVertices = 3;
				break;
			default:
				continue;
			}
		for (int32_t k = 0; k < nVertices; k++)
			AddEdge (i, j, pSide->m_corners [k], pSide->m_corners [(k + 1) % nVertices]);
		}
	}
#if POLYGONAL_OUTLINE
if (!gameData.segData.edgeVertices.Create (gameData.segData.nEdges * (bPolygonalOutline ? 8 : 4)))
#else
if (!CreateEdgeBuffers (gameData.segData.nEdges * 2))
#endif
	return -1;
return gameData.segData.nEdges;
}

//------------------------------------------------------------------------------
//eof
