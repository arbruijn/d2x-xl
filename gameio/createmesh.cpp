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
#include "text.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "loadgeometry.h"
#include "segmath.h"
#include "trigger.h"
#include "ogl_defs.h"
#include "oof.h"
#include "lightmap.h"
#include "rendermine.h"
#include "segmath.h"

#include "game.h"
#include "menu.h"
#include "menu.h"
#include "createmesh.h"

#include "cfile.h"
#include "producers.h"

#include "hash.h"
#include "key.h"
#include "piggy.h"

#include "byteswap.h"
#include "loadobjects.h"
#include "u_mem.h"
#include "vecmat.h"
#include "gamepal.h"
#include "paging.h"
#include "maths.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "renderlib.h"
#include "netmisc.h"
#include "createmesh.h"

using namespace Mesh;

void LoadFaceBitmaps (CSegment *segP, CSegFace *faceP);

//------------------------------------------------------------------------------

#define	MAX_EDGE_LEN(nMeshQuality)	fMaxEdgeLen [nMeshQuality]

#define MESH_DATA_VERSION 16

//------------------------------------------------------------------------------

typedef struct tMeshDataHeader {
	int32_t	nVersion;
	int32_t	nCheckSum;
	int32_t	nSegments;
	int32_t	nVertices;
	int32_t	nFaceVerts;
	int32_t	nFaces;
	int32_t	nTris;
	int32_t	bCompressed;
	} tMeshHeaderData;

//------------------------------------------------------------------------------

float fMaxEdgeLen [] = {1e30f, 30.9f, 20.9f, 19.9f, 9.9f};

CQuadMeshBuilder meshBuilder;

//------------------------------------------------------------------------------

void CTriMeshBuilder::FreeData (void)
{
m_triangles.Destroy ();
m_edges.Destroy ();
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::AllocData (void)
{
if (m_nMaxTriangles && m_nMaxEdges) {
	if (!(m_edges.Resize (m_nMaxEdges * 2) && m_triangles.Resize (m_nMaxTriangles * 2))) {
		PrintLog (0, "Not enough memory for building the triangle mesh (%d edges, %d tris).\n", m_nMaxEdges * 2, m_nMaxTriangles * 2);
		FreeData ();
		return 0;
		}
	memset (&m_edges [m_nMaxTriangles], 0xff, m_nMaxTriangles * sizeof (tEdge));
	memset (&m_triangles [m_nMaxEdges], 0xff, m_nMaxTriangles * sizeof (tTriangle));
	m_nMaxTriangles *= 2;
	m_nMaxEdges *= 2;
	}
else {
	m_nMaxTriangles = FACES.nFaces * 4;
	m_nMaxEdges = FACES.nFaces * 4;
	if (!m_edges.Create (m_nMaxEdges)) {
		PrintLog (0, "Not enough memory for building the triangle mesh (%d edges).\n", m_nMaxEdges);
		return 0;
		}
	if (!m_triangles.Create (m_nMaxTriangles)) {
		PrintLog (0, "Not enough memory for building the triangle mesh (%d tris).\n", m_nMaxTriangles);
		FreeData ();
		return 0;
		}
	m_edges.Clear (0xff);
	m_triangles.Clear (0xff);
	}
return 1;
}

//------------------------------------------------------------------------------

tEdge *CTriMeshBuilder::FindEdge (uint16_t nVert1, uint16_t nVert2, int32_t i)
{
	tEdge	*edgeP = &m_edges [++i];

for (; i < m_nEdges; i++, edgeP++)
	if ((edgeP->verts [0] == nVert1) && (edgeP->verts [1] == nVert2))
		return edgeP;
return NULL;
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::AddEdge (int32_t nTri, uint16_t nVert1, uint16_t nVert2)
{
if (nVert2 < nVert1) {
	uint16_t h = nVert1;
	nVert1 = nVert2;
	nVert2 = h;
	}
#if DBG
if ((nTri < 0) || (nTri >= m_nTriangles))
	return -1;
#endif
tEdge *edgeP;
for (int32_t i = -1;;) {
	if (!(edgeP = FindEdge (nVert1, nVert2, i)))
		break;
	i = int32_t (edgeP - &m_edges [0]);
	if (edgeP->tris [0] < 0) {
		edgeP->tris [0] = nTri;
		return i;
		}
	if (edgeP->tris [1] < 0) {
		edgeP->tris [1] = nTri;
		return i;
		}
	}
if (m_nFreeEdges >= 0) {
	edgeP = &m_edges [m_nFreeEdges];
	m_nFreeEdges = edgeP->nNext;
	edgeP->nNext = -1;
	}
else {
	if ((m_nEdges == m_nMaxEdges - 1) && !AllocData ())
		return -1;
	edgeP = &m_edges [m_nEdges++];
	}
edgeP->tris [0] = nTri;
edgeP->verts [0] = nVert1;
edgeP->verts [1] = nVert2;
edgeP->fLength = CFloatVector::Dist(gameData.segData.fVertices [nVert1], gameData.segData.fVertices [nVert2]);
return m_edges.Index (edgeP);
}

//------------------------------------------------------------------------------

tTriangle *CTriMeshBuilder::CreateTriangle (tTriangle *triP, uint16_t index [], int32_t nFace, int32_t nIndex)
{
	int32_t	h, i;

if (triP)
	triP->nIndex = nIndex;
else {
	if ((m_nTriangles == m_nMaxTriangles - 1) && !AllocData ())
		return NULL;
	triP = &m_triangles [m_nTriangles++];
	triP->nIndex = nIndex;
	if (nIndex >= 0) {
		i = TRIANGLES [nIndex].nIndex;
		memcpy (triP->texCoord, FACES.texCoord + i, sizeof (triP->texCoord));
		memcpy (triP->ovlTexCoord, FACES.ovlTexCoord + i, sizeof (triP->ovlTexCoord));
		memcpy (triP->color, FACES.color + i, sizeof (triP->color));
		}
	}
nIndex = m_triangles.Index (triP);
for (i = 0; i < 3; i++) {
	if (0 > (h = AddEdge (nIndex, index [i], index [(i + 1) % 3])))
		return NULL;
	triP = &m_triangles [nIndex];
	triP->lines [i] = h;
	}
triP->nFace = nFace;
memcpy (triP->index, index, sizeof (triP->index));
return triP;
}

//------------------------------------------------------------------------------

tTriangle *CTriMeshBuilder::AddTriangle (tTriangle *triP, uint16_t index [], tFaceTriangle *grsTriP)
{
return CreateTriangle (triP, index, grsTriP->nFace, grsTriP - TRIANGLES);
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::DeleteEdge (tEdge *edgeP)
{
#if 1
edgeP->nNext = m_nFreeEdges;
m_nFreeEdges = m_edges.Index (edgeP);
#else
	tTriangle	*triP;
	int32_t		h = edgeP - m_edges, i, j;

if (h < --m_nEdges) {
	*edgeP = m_edges [m_nEdges];
	for (i = 0; i < 2; i++) {
		if (edgeP->tris [i] >= 0) {
			triP = m_triangles + edgeP->tris [i];
			for (j = 0; j < 3; j++)
				if (triP->lines [j] == m_nEdges)
					triP->lines [j] = h;
			}
		}
	}
#endif
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::DeleteTriangle (tTriangle *triP)
{
	tEdge	*edgeP;
	int32_t	i, nTri = m_triangles.Index (triP);

for (i = 0; i < 3; i++) {
	edgeP = &m_edges [triP->lines [i]];
	if (edgeP->tris [0] == nTri)
		edgeP->tris [0] = edgeP->tris [1];
	edgeP->tris [1] = -1;
	if (edgeP->tris [0] < 0)
		DeleteEdge (edgeP);
	}
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::CreateTriangles (void)
{
PrintLog (1, "adding existing triangles\n");
m_nEdges = 0;
m_nTriangles = 0;
m_nMaxTriangles = 0;
m_nMaxTriangles = 0;
m_nFreeEdges = -1;
m_nVertices = gameData.segData.nVertices;
if (!AllocData ()) {
	PrintLog (-1);
	return 0;
	}

CSegFace *faceP;
tFaceTriangle *grsTriP;
tTriangle *triP;
int32_t i, nFace = -1;
int16_t nId = 0;

#if 0
if (gameStates.render.nMeshQuality) {
	i = LEVEL_VERTICES + ((FACES.nTriangles ? FACES.nTriangles / 2 : FACES.nFaces) << (abs (gameStates.render.nMeshQuality)));
	if (!(gameData.segData.fVertices.Resize (i) && gameData.segData.vertices.Resize (i) && gameData.render.mine.Resize (i)))
		return 0;
	}
#endif
for (i = FACES.nTriangles, grsTriP = TRIANGLES.Buffer (); i; i--, grsTriP++) {
	if (!(triP = AddTriangle (NULL, grsTriP->index, grsTriP))) {
		FreeData ();
		PrintLog (-1);
		return 0;
		}
	if (nFace == grsTriP->nFace)
		nId++;
	else {
		nFace = grsTriP->nFace;
		nId = 0;
		}
	triP->nId = nId;
	faceP = FACES.faces + grsTriP->nFace;
#if DBG
	if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
		BRP;
#endif
	if (faceP->m_info.bSlide || (faceP->m_info.nCamera >= 0))
		triP->nPass = -2;
	}
PrintLog (-1);
return m_nTris = m_nTriangles;
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::SplitTriangleByEdge (int32_t nTri, uint16_t nVert1, uint16_t nVert2, int16_t nPass)
{
if (nTri < 0)
	return 1;

tTriangle *triP = &m_triangles [nTri];

if (triP->nPass < -1)
	return 1;

CSegFace *faceP = FACES.faces + triP->nFace;

#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	BRP;
#endif

	int32_t			h, i, nIndex = triP->nIndex;
	uint16_t		nFace = triP->nFace, *indexP = triP->index, index [4];
	tTexCoord2f	texCoord [4], ovlTexCoord [4];
	CFloatVector	color [4];

// find insertion point for the new vertex
for (i = 0; i < 3; i++) {
	h = indexP [i];
	if ((h == nVert1) || (h == nVert2))
		break;
	}
if (i == 3) {
	PrintLog (0, "Internal error during construction of the triangle mesh.\n");
	return 0;
	}

h = indexP [(i + 1) % 3]; //check next vertex index
if ((h == nVert1) || (h == nVert2))
	h = (i + 1) % 3; //insert before i+1
else
	h = i; //insert before i

// build new quad index containing the new vertex
// the two triangle indices will be derived from it (indices 0,1,2 and 1,2,3)
index [0] = gameData.segData.nVertices - 1;
for (i = 1; i < 4; i++) {
	index [i] = indexP [h];
	texCoord [i] = triP->texCoord [h];
	ovlTexCoord [i] = triP->ovlTexCoord [h];
	color [i] = triP->color [h++];
	h %= 3;
	}

// interpolate texture coordinates and color for the new vertex
texCoord [0].v.v = (texCoord [1].v.v + texCoord [3].v.v) / 2;
texCoord [0].v.u = (texCoord [1].v.u + texCoord [3].v.u) / 2;
ovlTexCoord [0].v.v = (ovlTexCoord [1].v.v + ovlTexCoord [3].v.v) / 2;
ovlTexCoord [0].v.u = (ovlTexCoord [1].v.u + ovlTexCoord [3].v.u) / 2;
color [0].Red () = (color [1].Red () + color [3].Red ()) / 2;
color [0].Green () = (color [1].Green () + color [3].Green ()) / 2;
color [0].Blue () = (color [1].Blue () + color [3].Blue ()) / 2;
color [0].Alpha () = (color [1].Alpha () + color [3].Alpha ()) / 2;
DeleteTriangle (triP); //remove any references to this triangle
if (!(triP = CreateTriangle (triP, index, nFace, nIndex))) //create a new triangle at this location (insert)
	return 0;
triP->nPass = nPass;
triP->nId = (faceP->m_info.nTris)++;
memcpy (triP->color, color, sizeof (triP->color));
memcpy (triP->texCoord, texCoord, sizeof (triP->texCoord));
memcpy (triP->ovlTexCoord, ovlTexCoord, sizeof (triP->ovlTexCoord));

index [1] = index [0];
if (!(triP = CreateTriangle (NULL, index + 1, nFace, -1))) //create a new triangle (append)
	return 0;
triP->nPass = nPass;
triP->nId = (faceP->m_info.nTris)++;
triP->texCoord [0] = texCoord [0];
triP->ovlTexCoord [0] = ovlTexCoord [0];
triP->color [0] = color [0];
memcpy (triP->color + 1, color + 2, 2 * sizeof (triP->color [0]));
memcpy (triP->texCoord + 1, texCoord + 2, 2 * sizeof (triP->texCoord [0]));
memcpy (triP->ovlTexCoord + 1, ovlTexCoord + 2, 2 * sizeof (triP->ovlTexCoord [0]));
FACES.faces [triP->nFace].m_info.nVerts++;
return 1;
}

//------------------------------------------------------------------------------

float CTriMeshBuilder::NewEdgeLen (int32_t nTri, int32_t nVert1, int32_t nVert2)
{
	tTriangle	*triP = &m_triangles [nTri];
	int32_t			h, i;

for (i = 0; i < 3; i++) {
	h = triP->index [i];
	if ((h != nVert1) && (h != nVert2))
		break;
	}
return CFloatVector::Dist(gameData.segData.fVertices [h], gameData.segData.fVertices [gameData.segData.nVertices]);
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::SplitEdge (CSegFace* faceP, tEdge *edgeP, int16_t nPass)
{
	int32_t		tris [2];
	uint16_t	verts [2];

memcpy (tris, edgeP->tris, sizeof (tris));
memcpy (verts, edgeP->verts, sizeof (verts));
int32_t i = gameData.segData.fVertices.Length ();
if (gameData.segData.nVertices >= i) {
	i *= 2;
	if (!(gameData.segData.fVertices.Resize (i) && gameData.segData.vertices.Resize (i) && gameData.segData.vertexOwners.Resize (i) && gameData.render.mine.Resize (i)))
		return 0;
	}
gameData.segData.fVertices [gameData.segData.nVertices] = CFloatVector::Avg (gameData.segData.fVertices [verts [0]], gameData.segData.fVertices [verts [1]]);
gameData.segData.vertices [gameData.segData.nVertices].Assign (gameData.segData.fVertices [gameData.segData.nVertices]);
gameData.segData.vertexOwners [gameData.segData.nVertices].nSegment = faceP->m_info.nSegment;
gameData.segData.vertexOwners [gameData.segData.nVertices].nSide = faceP->m_info.nSide;
#if 0
if (tris [1] >= 0) {
	if (NewEdgeLen (tris [0], verts [0], verts [1]) + NewEdgeLen (tris [1], verts [0], verts [1]) < MAX_EDGE_LEN (m_nQuality))
		return -1;
	}
#endif
gameData.segData.nVertices++;
if (!SplitTriangleByEdge (tris [0], verts [0], verts [1], nPass))
	return 0;
if (!SplitTriangleByEdge (tris [1], verts [0], verts [1], nPass))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::SplitTriangle (CSegFace* faceP, tTriangle *triP, int16_t nPass)
{
	int32_t	h = 0, i;
	float	l, lMax = 0;

for (i = 0; i < 3; i++) {
	l = m_edges [triP->lines [i]].fLength;
	if (lMax < l) {
		lMax = l;
		h = i;
		}
	}
if (lMax <= MAX_EDGE_LEN (m_nQuality))
	return -1;
return SplitEdge (faceP, &m_edges [triP->lines [h]], nPass);
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::SplitTriangles (void)
{
	int32_t	bSplit = 0, h, i, j, nSplitRes;
	int16_t	nPass = 0, nMaxPasses = 10 * m_nQuality;

h = 0;
do {
	bSplit = 0;
	j = m_nTriangles;
	PrintLog (1, "splitting triangles (pass %d)\n", nPass);
	for (i = h, h = 0; i < j; i++) {
#if DBG
		CSegFace *faceP = FACES.faces + m_triangles [i].nFace;
		if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
			BRP;
#endif
		if (m_triangles [i].nPass != nPass - 1)
			continue;
#if DBG
		faceP = FACES.faces + m_triangles [i].nFace;
		if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
			BRP;
#endif
		nSplitRes = SplitTriangle (FACES.faces + m_triangles [i].nFace, &m_triangles [i], nPass);
		if (gameData.segData.nVertices == 65536) {
			PrintLog (-1, "Level too big for requested triangle mesh quality.\n");
			return 0;
			}
		if (!nSplitRes)
			return 0;
		if (nSplitRes < 0)
			m_triangles [i].nPass = -2;
		else {
			bSplit = 1;
			if (!h)
				h = i;
			}
		}
	nPass++;
	PrintLog (-1);
	} while (bSplit && (nPass < nMaxPasses));
FACES.nTriangles = m_nTriangles;
return 1;
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::QSortTriangles (int32_t left, int32_t right)
{
	int32_t	l = left,
			r = right,
			m = m_triangles [(l + r) / 2].nFace;
	int16_t i = m_triangles [(l + r) / 2].nId;

do {
	while ((m_triangles [l].nFace < m) || ((m_triangles [l].nFace == m) && (m_triangles [l].nId < i)))
		l++;
	while ((m_triangles [r].nFace > m) || ((m_triangles [r].nFace == m) && (m_triangles [r].nId > i)))
		r--;
	if (l <= r) {
		if (l < r)
			::Swap (m_triangles [l], m_triangles [r]);
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortTriangles (l, right);
if (left < r)
	QSortTriangles (left, r);
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::SetupVertexNormals (void)
{
	tFaceTriangle*	triP;
	CRenderPoint*	pointP = RENDERPOINTS.Buffer ();
	int32_t				h, i, nVertex;

for (i = gameData.segData.nVertices; i; i--, pointP++) {
/*
	(*pointP->m_normal.vNormal.XYZ ()).v.c.x =
	(*pointP->m_normal.vNormal.XYZ ()).v.c.y =
	(*pointP->m_normal.vNormal.XYZ ()).v.c.z = 0;
*/
	pointP->Normal ().Reset ();
	}
for (h = 0, triP = FACES.tris.Buffer (); h < FACES.nTriangles; h++, triP++) {
	for (i = 0; i < 3; i++) {
		nVertex = triP->index [i];
#if DBG
		if (nVertex == nDbgVertex)
			BRP;
#endif
		RENDERPOINTS [nVertex].Normal () += FACES.normals [3 * h];
		}
	}
ComputeVertexNormals ();
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::InsertTriangles (void)
{
	tTriangle*		triP = &m_triangles [0];
	tFaceTriangle*	grsTriP = TRIANGLES.Buffer ();
	CSegFace*		m_faceP = NULL;
	CFixVector		vNormal;
	int32_t				h, i, nFace = -1;
	GLuint			nIndex = 0;

PrintLog (1, "inserting new triangles\n");
QSortTriangles (0, m_nTriangles - 1);
ResetVertexNormals ();
for (h = 0; h < m_nTriangles; h++, triP++, grsTriP++) {
	grsTriP->nFace = triP->nFace;
	if (grsTriP->nFace == nFace)
		m_faceP->m_info.nTris++;
	else {
		if (m_faceP)
			m_faceP++;
		else
			m_faceP = FACES.faces.Buffer ();
		nFace = grsTriP->nFace;
#if DBG
		if (m_faceP - FACES.faces != nFace)
			return 0;
#endif
		m_faceP->m_info.nFrame = -1;
		m_faceP->m_info.nIndex = nIndex;
		m_faceP->m_info.nTriIndex = h;
		m_faceP->m_info.nTris = 1;
#if USE_RANGE_ELEMENTS
		m_faceP->m_info.vertIndex = FACES.vertIndex + nIndex;
#endif
		}
	grsTriP->nIndex = nIndex;
	memcpy (grsTriP->index, triP->index, sizeof (triP->index));
	for (i = 0; i < 3; i++) {
#if DBG
		if (nIndex + i >= FACES.vertices.Length ())
			BRP;
#endif
		FACES.vertices [nIndex + i].Assign (gameData.segData.fVertices [triP->index [i]]);
		}
	FACES.normals [nIndex] = CFloatVector3::Normal (FACES.vertices [nIndex], FACES.vertices [nIndex + 1], FACES.vertices [nIndex + 2]);
#if DBG
		if (nIndex >= FACES.normals.Length ())
			BRP;
	if (FACES.normals [nIndex].Mag () == 0)
		BRP;
#endif
	vNormal.Assign (FACES.normals [nIndex]);
	for (i = 1; i < 3; i++)
		FACES.normals [nIndex + i] = FACES.normals [nIndex];
	if ((nIndex >= FACES.texCoord.Length ()) || (nIndex >= FACES.ovlTexCoord.Length ()) || (nIndex >= FACES.color.Length ()))
		return 0;
	memcpy (FACES.texCoord + nIndex, triP->texCoord, sizeof (triP->texCoord));
	memcpy (FACES.ovlTexCoord + nIndex, triP->ovlTexCoord, sizeof (triP->ovlTexCoord));
	memcpy (FACES.color + nIndex, triP->color, sizeof (triP->color));
#if USE_RANGE_ELEMENTS
	for (i = 0; i < 3; i++, nIndex++)
		FACES.vertIndex [nIndex] = nIndex;
#else
	nIndex += 3;
#endif
	}
SetupVertexNormals ();
FreeData ();
PrintLog (-1, "created %d new triangles and %d new vertices\n",
			 m_nTriangles - m_nTris, gameData.segData.nVertices - m_nVertices);
CreateFaceVertLists ();
return 1;
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::SortFaceVertList (uint16_t *vertList, int32_t left, int32_t right)
{
	int32_t	l = left,
			r = right,
			m = vertList [(l + r) / 2];

do {
	while (vertList [l] < m)
		l++;
	while (vertList [r] > m)
		r--;
	if (l <= r) {
		if (l < r) {
			int32_t h = vertList [l];
			vertList [l] = vertList [r];
			vertList [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	SortFaceVertList (vertList, l, right);
if (left < r)
	SortFaceVertList (vertList, left, r);
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::CreateSegFaceList (void)
{
	int32_t			h = 0;
	tSegFaces*	segFaceP = SEGFACES.Buffer ();

for (int32_t i = gameData.segData.nSegments; i; i--, segFaceP++) {
	segFaceP->faceP = &FACES.faces [h];
	h += segFaceP->nFaces;
	}
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::CreateFaceVertLists (void)
{
	int32_t*			bTags = new int32_t [gameData.segData.nVertices];
	CSegFace*		faceP;
	tFaceTriangle*	triP;
	int32_t			h, i, j, k, nFace;

//count the vertices of each face
memset (bTags, 0xFF, gameData.segData.nVertices * sizeof (bTags [0]));
gameData.segData.nFaceVerts = 0;
for (i = FACES.nFaces, faceP = FACES.faces.Buffer (), nFace = 0; i; i--, faceP++, nFace++) {
	faceP->m_info.nVerts = 0;
	for (j = faceP->m_info.nTris, triP = TRIANGLES + faceP->m_info.nTriIndex; j; j--, triP++) {
		for (k = 0; k < 3; k++) {
			h = triP->index [k];
			if (bTags [h] != nFace) {
				bTags [h] = nFace;
				faceP->m_info.nVerts++;
				gameData.segData.nFaceVerts++;
				}
			}
		}
	}
//insert each face's vertices' indices in the vertex index buffer
memset (bTags, 0xFF, gameData.segData.nVertices * sizeof (bTags [0]));
if ((uint32_t) gameData.segData.nFaceVerts > FACES.faceVerts.Length ())
	FACES.faceVerts.Resize (gameData.segData.nFaceVerts);
gameData.segData.nFaceVerts = 0;
for (i = FACES.nFaces, faceP = FACES.faces.Buffer (), nFace = 0; i; i--, faceP++, nFace++) {
	faceP->triIndex = FACES.faceVerts + gameData.segData.nFaceVerts;
#if DBG
	if (faceP->m_info.nSegment == nDbgSeg)
		BRP;
#endif
	for (j = faceP->m_info.nTris, triP = TRIANGLES + faceP->m_info.nTriIndex; j; j--, triP++) {
		for (k = 0; k < 3; k++) {
			h = triP->index [k];
			if (bTags [h] != nFace) {
				bTags [h] = nFace;
				FACES.faceVerts [gameData.segData.nFaceVerts++] = h;
				}
			}
		}
	}
#if 1
//sort each face's vertex index list
for (i = FACES.nFaces, faceP = FACES.faces.Buffer (); i; i--, faceP++)
	SortFaceVertList (faceP->triIndex, 0, faceP->m_info.nVerts - 1);
#endif
}

//------------------------------------------------------------------------------

char *CTriMeshBuilder::DataFilename (char *pszFilename, int32_t nLevel)
{
return GameDataFilename (pszFilename, "mesh", nLevel,
								 (gameStates.render.bTriangleMesh < 0) ? -1 : m_nQuality);
}

//------------------------------------------------------------------------------

bool CTriMeshBuilder::Load (int32_t nLevel, bool bForce)
{
	CFile					cf;
	tMeshDataHeader	mdh;
	int32_t					nSize, nExpectedSize;
	bool					bOk;
	char					szFilename [FILENAME_LEN];
	char					*ioBuffer = NULL, *bufP;

if (!(gameStates.render.bTriangleMesh && (gameStates.app.bCacheMeshes || bForce)))
	return false;
if (!cf.Open (DataFilename (szFilename, nLevel), gameFolders.var.szMeshes, "rb", 0) &&
	 !cf.Open (DataFilename (szFilename, nLevel), gameFolders.var.szCache, "rb", 0))
	return false;
bOk = (cf.Read (&mdh, sizeof (mdh), 1) == 1);
if (bOk)
	bOk = (mdh.nVersion == MESH_DATA_VERSION) &&
			(mdh.nCheckSum == CalcSegmentCheckSum ()) &&
			(mdh.nSegments == gameData.segData.nSegments) &&
			(mdh.nFaces == FACES.nFaces);

	uint32_t	nTriVerts = mdh.nTris * 3;
	int32_t nSizes [] = {
		int32_t (sizeof (gameData.segData.vertices [0]) * mdh.nVertices),
		int32_t (sizeof (gameData.segData.fVertices [0]) * mdh.nVertices),
		-int32_t (sizeof (CSegFaceInfo) * mdh.nFaces),
		int32_t (sizeof (FACES.tris [0]) * mdh.nTris),
		int32_t (sizeof (FACES.vertices [0]) * nTriVerts),
		int32_t (sizeof (FACES.normals [0]) * nTriVerts),
		int32_t (sizeof (FACES.texCoord [0]) * nTriVerts),
		int32_t (sizeof (FACES.ovlTexCoord [0]) * nTriVerts),
		int32_t (sizeof (FACES.color [0]) * nTriVerts),
		int32_t (sizeof (FACES.lMapTexCoord [0]) * FACES.lMapTexCoord.Length ()),
		int32_t (sizeof (FACES.faceVerts [0]) * mdh.nFaceVerts),
		int32_t (sizeof (VERTEX_OWNERS [0]) * mdh.nVertices),
		0
		};


if (bOk) {
	nExpectedSize = 0;
	for (int32_t i = 0; i < (int32_t) sizeofa (nSizes); i++)
		nExpectedSize += abs (nSizes [i]);
	}
if (bOk)
	bOk = ((ioBuffer = new char [nExpectedSize]) != NULL);
if (bOk) {
	if (!mdh.bCompressed)
		bOk = cf.Read (ioBuffer, nExpectedSize, 1) == 1;
	else {
		PrintLog (1, "Mesh builder: reading compressed mesh data\n");
		nSize = 0;
		int32_t i;
		for (i = 0; nSizes [i] > 0; i++) {
			if (cf.Read (ioBuffer + nSize, nSizes [i], 1, 1) != 1) {
				PrintLog (0, "Mesh builder: reading mesh element #%d failed\n", i + 1);
				bOk = false;
				break;
				}
			nSize += nSizes [i];
			}
		if (bOk) {
			if (cf.Read (ioBuffer + nSize, sizeof (CSegFaceInfo) * mdh.nFaces, 1) != 1) {
				PrintLog (0, "Mesh builder: reading face info failed\n");
				bOk = false;
				}
			nSize += sizeof (CSegFaceInfo) * mdh.nFaces;
			}
		for (++i; nSizes [i] > 0; i++) {
			if (cf.Read (ioBuffer + nSize, nSizes [i], 1, 1) != 1) {
				PrintLog (0, "Mesh builder: reading mesh element #%d failed\n", i + 1);
				bOk = false;
				break;
				}
			nSize += nSizes [i];
			}
		if (bOk && (nSize != nExpectedSize)) {
			PrintLog (0, "Mesh builder: invalid mesh size\n");
			bOk = false;
			}
		PrintLog (-1);
		}
	}
if (bOk) {
	bufP = ioBuffer;
	FACES.Destroy ();
	gameData.segData.vertices.Create (mdh.nVertices);
	memcpy (gameData.segData.vertices.Buffer (), bufP, nSize = sizeof (gameData.segData.vertices [0]) * mdh.nVertices);
	bufP += nSize;
	gameData.segData.fVertices.Create (mdh.nVertices);
	memcpy (gameData.segData.fVertices.Buffer (), bufP, nSize = sizeof (gameData.segData.fVertices [0]) * mdh.nVertices);
	bufP += nSize;
	FACES.faces.Create (mdh.nFaces);
	FACES.faces.Clear ();
	for (int32_t i = 0; i < mdh.nFaces; i++, bufP += sizeof (CSegFaceInfo))
		memcpy (&FACES.faces [i].m_info, bufP, sizeof (CSegFaceInfo));
	FACES.tris.Create (mdh.nTris);
	memcpy (FACES.tris.Buffer (), bufP, nSize = sizeof (FACES.tris [0]) * mdh.nTris);
	bufP += nSize;
	FACES.vertices.Create (nTriVerts);
	memcpy (FACES.vertices.Buffer (), bufP, nSize = sizeof (FACES.vertices [0]) * nTriVerts);
	bufP += nSize;
	FACES.normals.Create (nTriVerts);
	memcpy (FACES.normals.Buffer (), bufP, nSize = sizeof (FACES.normals [0]) * nTriVerts);
	bufP += nSize;
	FACES.texCoord.Create (nTriVerts);
	memcpy (FACES.texCoord.Buffer (), bufP, nSize = sizeof (FACES.texCoord [0]) * nTriVerts);
	bufP += nSize;
	FACES.ovlTexCoord.Create (nTriVerts);
	memcpy (FACES.ovlTexCoord.Buffer (), bufP, nSize = sizeof (FACES.ovlTexCoord [0]) * nTriVerts);
	bufP += nSize;
	FACES.color.Create (nTriVerts);
	memcpy (FACES.color.Buffer (), bufP, nSize = sizeof (FACES.color [0]) * nTriVerts);
	bufP += nSize;
	FACES.lMapTexCoord.Create (gameData.segData.nFaces * 3 * 2);
	memcpy (FACES.lMapTexCoord.Buffer (), bufP, nSize = sizeof (FACES.lMapTexCoord [0]) * FACES.lMapTexCoord.Length ());
	bufP += nSize;
	FACES.faceVerts.Create (mdh.nFaceVerts);
	memcpy (FACES.faceVerts.Buffer (), bufP, nSize = sizeof (FACES.faceVerts [0]) * mdh.nFaceVerts);
	bufP += nSize;
	VERTEX_OWNERS.Resize (mdh.nVertices);
	memcpy (VERTEX_OWNERS.Buffer (), bufP, nSize = sizeof (VERTEX_OWNERS [0]) * mdh.nVertices);
	gameData.render.mine.Resize (mdh.nVertices);
	}
if (ioBuffer) {
	delete[] ioBuffer;
	ioBuffer = NULL;
	}
if (bOk) {
	gameData.segData.nVertices = mdh.nVertices;
	FACES.nTriangles = mdh.nTris;
	SetupVertexNormals ();
	}
cf.Close ();
if (!gameStates.app.bCacheMeshes)
	cf.Delete (DataFilename (szFilename, nLevel), gameFolders.var.szCache);
CreateSegFaceList ();
CreateFaceVertLists ();
return bOk;
}

//------------------------------------------------------------------------------

bool CTriMeshBuilder::Save (int32_t nLevel)
{
	tMeshDataHeader mdh = {MESH_DATA_VERSION,
								  CalcSegmentCheckSum (),
								  gameData.segData.nSegments,
								  gameData.segData.nVertices,
								  gameData.segData.nFaceVerts,
								  FACES.nFaces,
								  FACES.nTriangles,
								  gameStates.app.bCompressData
								};

	CFile					cf;
	bool					bOk;
	char					szFilename [FILENAME_LEN];
	uint32_t				nTriVerts = uint32_t (mdh.nTris * 3);

if (!(gameStates.render.bTriangleMesh && gameStates.app.bCacheMeshes))
	return 0;
if (!cf.Open (DataFilename (szFilename, nLevel), gameFolders.var.szMeshes, "wb", 0)) {
	PrintLog (0, "Couldn't save mesh data (check access rights of '%s')\n", gameFolders.var.szMeshes);
	return 0;
	}
if (mdh.bCompressed)
	PrintLog (0, "Mesh builder: writing compressed mesh data (%d)\n", gameStates.app.bCompressData);
bOk = (cf.Write (&mdh, sizeof (mdh), 1) == 1) &&
		(gameData.segData.vertices.Write (cf, mdh.nVertices, 0, mdh.bCompressed) == uint32_t (mdh.nVertices)) &&
		(gameData.segData.fVertices.Write (cf, mdh.nVertices, 0, mdh.bCompressed) == uint32_t (mdh.nVertices));
if (bOk) {
	for (int32_t i = 0; i < mdh.nFaces; i++) {
		if (cf.Write (&FACES.faces [i].m_info, sizeof (CSegFaceInfo), 1) != 1) {
			bOk = false;
			break;
			}
		}
	}
if (bOk)
	bOk = (FACES.tris.Write (cf, mdh.nTris, 0, mdh.bCompressed) == uint32_t (mdh.nTris)) &&
			(FACES.vertices.Write (cf, nTriVerts, 0, mdh.bCompressed) == nTriVerts) &&
			(FACES.normals.Write (cf, nTriVerts, 0, mdh.bCompressed) == nTriVerts) &&
			(FACES.texCoord.Write (cf, nTriVerts, 0, mdh.bCompressed) == nTriVerts) &&
			(FACES.ovlTexCoord.Write (cf, nTriVerts, 0, mdh.bCompressed) == nTriVerts) &&
			(FACES.color.Write (cf, nTriVerts, 0, mdh.bCompressed) == nTriVerts) &&
			(FACES.lMapTexCoord.Write (cf, FACES.lMapTexCoord.Length (), 0, mdh.bCompressed) == FACES.lMapTexCoord.Length ()) &&
			(FACES.faceVerts.Write (cf, mdh.nFaceVerts, 0, mdh.bCompressed) == uint32_t (mdh.nFaceVerts)) &&
			(VERTEX_OWNERS.Write (cf, mdh.nVertices, 0, mdh.bCompressed) == uint32_t (mdh.nVertices));
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::Build (int32_t nLevel, int32_t nQuality)
{
PrintLog (1, "creating triangle mesh\n");
m_nQuality = nQuality;
if (Load (nLevel)) {
	PrintLog (-1);
	return 1;
	}
if (!CreateTriangles ()) {
	gameData.segData.nVertices = m_nVertices;
	PrintLog (-1);
	return 0;
	}
if ((gameStates.render.bTriangleMesh > 0) && !SplitTriangles ()) {
	gameData.segData.nVertices = m_nVertices;
	PrintLog (-1);
	return 0;
	}

if ((m_nTriangles > (LEVEL_FACES << gameStates.render.nMeshQuality)) && !gameData.segData.faces.Resize ()) {
	gameData.segData.faces.Destroy ();
	gameData.render.Destroy ();
	PrintLog (-1, "Not enough memory to create mesh data in requested quality\n");
	return 0;
	}

if (!InsertTriangles ()) {
	gameData.segData.nVertices = m_nVertices;
	PrintLog (-1);
	return 0;
	}
if (gameStates.app.bReadOnly) {
	PrintLog (-1);
	return 1;
	}
Save (nLevel);
int32_t nLoadRes = Load (nLevel, true); //Load will rebuild all face data buffers, reducing their memory footprint
PrintLog (-1);
return nLoadRes;
}

//------------------------------------------------------------------------------

int32_t CQuadMeshBuilder::IsBigFace (uint16_t *sideVerts)
{
for (int32_t i = 0; i < 4; i++)
	if (CFloatVector::Dist (gameData.segData.fVertices [sideVerts [i]], gameData.segData.fVertices [sideVerts [(i + 1) % 4]]) > MAX_EDGE_LEN (gameStates.render.nMeshQuality))
		return 1;
return 0;
}

//------------------------------------------------------------------------------

CFloatVector3 *CQuadMeshBuilder::SetTriNormals (tFaceTriangle *triP, CFloatVector3 *m_normalP)
{
	CFloatVector	vNormalf;

vNormalf = CFloatVector::Normal (gameData.segData.fVertices [triP->index [0]],
				 gameData.segData.fVertices [triP->index [1]], gameData.segData.fVertices [triP->index [2]]);
*m_normalP++ = *vNormalf.XYZ ();
*m_normalP++ = *vNormalf.XYZ ();
*m_normalP++ = *vNormalf.XYZ ();
return m_normalP;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitFace (int16_t nSegment, uint8_t nSide, bool bRebuild)
{
if (bRebuild)
	m_faceP->m_info.nTris = 0;
else
	memset (m_faceP, 0, sizeof (*m_faceP));
CSide* sideP = SEGMENT (nSegment)->Side (nSide);
m_faceP->m_info.nSegment = nSegment;
m_faceP->m_info.nVerts = sideP->CornerCount ();
m_faceP->m_info.nFrame = -1;
m_faceP->m_info.nIndex = m_vertexP - FACES.vertices;
if (gameStates.render.bTriangleMesh)
	m_faceP->m_info.nTriIndex = m_triP - TRIANGLES;
memcpy (m_faceP->m_info.index, m_sideVerts, sizeof (m_faceP->m_info.index));
m_faceP->m_info.nType = gameStates.render.bTriangleMesh ? m_sideP->m_nType : -1;
m_faceP->m_info.nSegment = nSegment;
m_faceP->m_info.nSide = nSide;
m_faceP->m_info.nTriangles = sideP->FaceCount ();
m_faceP->m_info.nWall = gameStates.app.bD2XLevel ? m_nWall : IS_WALL (m_nWall) ? m_nWall : (uint16_t) -1;
m_faceP->m_info.bAnimation = IsAnimatedTexture (m_faceP->m_info.nBaseTex) || IsAnimatedTexture (m_faceP->m_info.nOvlTex);
m_faceP->m_info.bHasColor = 0;
m_faceP->m_info.fRads [0] = X2F (sideP->m_rads [0]); //(float) sqrt ((rMinf * rMinf + rMaxf * rMaxf) / 2);
m_faceP->m_info.fRads [1] = X2F (sideP->m_rads [1]); //(float) sqrt ((rMinf * rMinf + rMaxf * rMaxf) / 2);
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitTexturedFace (void)
{
	static char szEmpty [] = "";
	
m_faceP->m_info.nBaseTex = m_sideP->m_nBaseTex;
if ((m_faceP->m_info.nOvlTex = m_sideP->m_nOvlTex))
	m_nOvlTexCount++;
m_faceP->m_info.bSlide = (gameData.pig.tex.tMapInfoP [m_faceP->m_info.nBaseTex].slide_u || gameData.pig.tex.tMapInfoP [m_faceP->m_info.nBaseTex].slide_v);
m_faceP->m_info.nCamera = IsMonitorFace (m_faceP->m_info.nSegment, m_faceP->m_info.nSide, 1);
m_faceP->m_info.bIsLight = IsLight (m_faceP->m_info.nBaseTex) || (m_faceP->m_info.nOvlTex && IsLight (m_faceP->m_info.nOvlTex));
m_faceP->m_info.nOvlOrient = (uint8_t) m_sideP->m_nOvlOrient;
m_faceP->m_info.bTextured = 1;
m_faceP->m_info.bTransparent = 0;
int32_t nTexture = m_faceP->m_info.nOvlTex ? m_faceP->m_info.nOvlTex : m_faceP->m_info.nBaseTex;
char *pszName = (nTexture < MAX_WALL_TEXTURES)
					 ? gameData.pig.tex.bitmapFiles [gameStates.app.bD1Mission][gameData.pig.tex.bmIndexP [nTexture].index].name
					 : szEmpty;
if (strstr (pszName, "misc17") != NULL)
	m_faceP->m_info.bSparks = (nTexture == m_faceP->m_info.nBaseTex) ? 1 : 2;
if (m_nWallType < 2)
	m_faceP->m_info.bAdditive = 0;
else if (WALL (m_nWall)->flags & WALL_RENDER_ADDITIVE)
	m_faceP->m_info.bAdditive = 2;
else if (strstr (pszName, "lava"))
	m_faceP->m_info.bAdditive = 2;
else
	m_faceP->m_info.bAdditive = (strstr (pszName, "force") || m_faceP->m_info.bSparks) ? 1 : 0;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitColoredFace (int16_t nSegment)
{
m_faceP->m_info.nBaseTex = -1;
m_faceP->m_info.bTransparent = 1;
m_faceP->m_info.bSegColor = missionConfig.m_bColoredSegments;
m_faceP->m_info.bAdditive = 0;
#if DBG
if ((SEGMENT (nSegment)->m_props & (SEGMENT_PROP_WATER | SEGMENT_PROP_LAVA)) != 0)
	BRP;
#endif
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SetupLMapTexCoord (tTexCoord2f *texCoordP)
{
#define	LMAP_SIZE	0.0f //(1.0f / 16.0f)
#if 0
	static tTexCoord2f lMapTexCoord [4] = {
	 {{LMAP_SIZE, LMAP_SIZE}},
	 {{1.0f - LMAP_SIZE, LMAP_SIZE}},
	 {{1.0f - LMAP_SIZE, 1.0f - LMAP_SIZE}},
	 {{LMAP_SIZE, 1.0f - LMAP_SIZE}}
	};
#endif
#if DBG
if ((m_faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_faceP->m_info.nSide == nDbgSide)))
	BRP;
#endif
int32_t i = m_faceP->m_info.nLightmap % LIGHTMAP_BUFSIZE;
float x = (float) (i % LIGHTMAP_ROWSIZE);
float y = (float) (i / LIGHTMAP_ROWSIZE);
float step = 1.0f / (float) LIGHTMAP_ROWSIZE;
#if 0
const float border = 0.0f;
#else
const float border = 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
#endif
texCoordP [0].v.u = 
texCoordP [3].v.u = x * step + border;
texCoordP [1].v.u =
texCoordP [2].v.u = (x + 1) * step - border;
texCoordP [0].v.v = 
texCoordP [1].v.v = y * step + border;
texCoordP [2].v.v = 
texCoordP [3].v.v = (y + 1) * step - border;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SetupFace (void)
{
	int32_t				i;
	CFixVector		vNormal;
	CFloatVector3	vNormalf;

vNormal = m_sideP->m_normals [2];
vNormalf.Assign (vNormal);
for (i = 0; i < 4; i++) {
	uint16_t h = m_sideVerts [i];
	if (h != 0xffff)
		*m_vertexP++ = *gameData.segData.fVertices [h].XYZ ();
	*m_normalP++ = vNormalf;
	m_texCoordP->v.u = X2F (m_sideP->m_uvls [i].u);
	m_texCoordP->v.v = X2F (m_sideP->m_uvls [i].v);
	RotateTexCoord2f (*m_ovlTexCoordP, *m_texCoordP, (uint8_t) m_sideP->m_nOvlOrient);
#if DBG
	if (m_texCoordP > FACES.texCoord.Buffer () + FACES.texCoord.Length ())
		return;
	if (m_ovlTexCoordP > FACES.ovlTexCoord.Buffer () + FACES.ovlTexCoord.Length ())
		return;
#endif
	m_texCoordP++;
	m_ovlTexCoordP++;
	if (!gameStates.app.bNostalgia)
		*m_faceColorP = gameData.render.color.ambient [h];
	else {
		m_faceColorP->Red () = 
		m_faceColorP->Green () = 
		m_faceColorP->Blue () = X2F (m_sideP->m_uvls [i].l);
		m_faceColorP->Alpha () = 1;
		}
	m_faceColorP++;
	}
SetupLMapTexCoord (m_lMapTexCoordP);
m_lMapTexCoordP += 4;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SplitIn1or2Tris (void)
{
	static int16_t	n2TriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};

	int32_t			h, i, j, k, v, nType;
	int16_t			*triVertP;
	tTexCoord2f	lMapTexCoord [4];

#if DBG
if ((m_faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_faceP->m_info.nSide == nDbgSide)))
	BRP;
#endif
SetupLMapTexCoord (lMapTexCoord);
nType = (m_sideP->m_nType == SIDE_IS_TRI_13);
if (!(h = m_sideP->FaceCount ()))
	return;
for (i = 0; i < h; i++, m_triP++) {
	FACES.nTriangles++;
	m_faceP->m_info.nTris++;
	m_triP->nFace = m_faceP - FACES.faces;
	m_triP->nIndex = m_vertexP - FACES.vertices;
	triVertP = n2TriVerts [nType][i];
	for (j = 0; j < 3; j++) {
		k = triVertP [j];
		v = m_sideVerts [k];
		m_triP->index [j] = v;
		*m_vertexP++ = *gameData.segData.fVertices [v].XYZ ();
		m_texCoordP->v.u = X2F (m_sideP->m_uvls [k].u);
		m_texCoordP->v.v = X2F (m_sideP->m_uvls [k].v);
		RotateTexCoord2f (*m_ovlTexCoordP, *m_texCoordP, (uint8_t) m_sideP->m_nOvlOrient);
		*m_lMapTexCoordP = lMapTexCoord [k];
		m_texCoordP++;
		m_ovlTexCoordP++;
		m_lMapTexCoordP++;
		m_colorP = gameData.render.color.ambient + v;
		m_faceColorP->Assign (*m_colorP);
		++m_faceColorP;
		}
	m_normalP = SetTriNormals (m_triP, m_normalP);
	}
}

//------------------------------------------------------------------------------

int32_t CQuadMeshBuilder::CompareFaceKeys (const CSegFace** pf, const CSegFace** pm)
{
#if 1
int32_t i = (*pf)->m_info.nKey;
int32_t m = (*pm)->m_info.nKey;
#else
int16_t i = (*pf)->m_info.nBaseTex;
int16_t mat = (*pm)->m_info.nBaseTex;
if (i < mat) 
	return -1;
if (i > mat)
	return 1;
i = (*pf)->m_info.nOvlTex;
mat = (*pm)->m_info.nOvlTex;
#endif
return (i < m) ? -1 : (i > m) ? 1 : 0;
}

//------------------------------------------------------------------------------
// Create a linearized set of keys for all faces, where each unique combination of 
// a face's base and overlay texture has a unique key.
// The renderer will use these to quickly access all faces bearing the same textures.

void CQuadMeshBuilder::ComputeFaceKeys (void)
{
	CSegFace	*faceP = FACES.faces.Buffer ();
	CArray<CSegFace*>	keyFaceRef;

keyFaceRef.Create (FACES.nFaces);
for (int32_t i = 0; i < FACES.nFaces; i++, faceP++) {
#if DBG
	if (i == nDbgFace)
		BRP;
#endif
	keyFaceRef [i] = faceP;
	if (faceP->m_info.nBaseTex < 0)
		faceP->m_info.nKey = 0x7FFFFFFF;
	else if (faceP->m_info.nOvlTex <= 0)
		faceP->m_info.nKey = int32_t (faceP->m_info.nBaseTex);
	else {
		LoadFaceBitmaps (SEGMENT (faceP->m_info.nSegment), faceP);
		if (faceP->bmTop && (faceP->bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT))
			faceP->m_info.nKey = int32_t (faceP->m_info.nBaseTex) + int32_t (faceP->m_info.nOvlTex) * MAX_WALL_TEXTURES * MAX_WALL_TEXTURES;
		else
			faceP->m_info.nKey = int32_t (faceP->m_info.nBaseTex) + int32_t (faceP->m_info.nOvlTex) * MAX_WALL_TEXTURES;
		}
	}

CQuickSort<CSegFace*> qs;
qs.SortAscending (keyFaceRef.Buffer (), 0, FACES.nFaces - 1, reinterpret_cast<CQuickSort<CSegFace*>::comparator> (CQuadMeshBuilder::CompareFaceKeys));

int32_t i, nKey = -1;
gameData.segData.nFaceKeys = -1;

for (i = 0; i < FACES.nFaces; i++, faceP++) {
	faceP = keyFaceRef [i];
	if (nKey != faceP->m_info.nKey) {
		nKey = faceP->m_info.nKey;
		++gameData.segData.nFaceKeys;
		}
	faceP->m_info.nKey = gameData.segData.nFaceKeys;
	}
++gameData.segData.nFaceKeys;
gameData.render.faceIndex.Create ();
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::BuildSlidingFaceList (void)
{
	CSegFace	*faceP = FACES.faces.Buffer ();

FACES.slidingFaces = NULL;
for (int32_t i = FACES.nFaces; i; i--, faceP++)
	if (faceP->m_info.bSlide) {
		faceP->nextSlidingFace = FACES.slidingFaces;
		FACES.slidingFaces = faceP;
		}
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::RebuildLightmapTexCoord (void)
{
	static int16_t	n2TriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};

	int32_t			h, i, j, k, nFace;
	int16_t			*triVertP;
	tTexCoord2f	lMapTexCoord [4];

m_faceP = FACES.faces.Buffer ();
m_lMapTexCoordP = FACES.lMapTexCoord.Buffer ();
for (nFace = FACES.nFaces; nFace; nFace--, m_faceP++) {
	SetupLMapTexCoord (lMapTexCoord);
	h = (SEGMENT (m_faceP->m_info.nSegment)->Type (m_faceP->m_info.nSide) == SIDE_IS_TRI_13);
	for (i = 0; i < m_faceP->m_info.nTriangles; i++, m_triP++) {
		triVertP = n2TriVerts [h][i];
		for (j = 0; j < 3; j++) {
			k = triVertP [j];
			*m_lMapTexCoordP = lMapTexCoord [k];
			m_lMapTexCoordP++;
			}
		}
	}
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SplitIn4Tris (void)
{
	static int16_t	n4TriVerts [4][3] = {{0,1,4},{1,2,4},{2,3,4},{3,0,4}};

	CFloatVector	vSide [4];
	CFloatVector		color;
	tTexCoord2f		texCoord;
	int16_t*			triVertP;
	int32_t				h, i, j, k, v;

texCoord.v.u = texCoord.v.v = 0;
color.Red () = color.Green () = color.Blue () = color.Alpha () = 0;
for (i = 0; i < 4; i++) {
	j = (i + 1) % 4;
	texCoord.v.u += X2F (m_sideP->m_uvls [i].u + m_sideP->m_uvls [j].u) / 8;
	texCoord.v.v += X2F (m_sideP->m_uvls [i].v + m_sideP->m_uvls [j].v) / 8;
	h = m_sideVerts [i];
	k = m_sideVerts [j];
	color += (gameData.render.color.ambient [h] + gameData.render.color.ambient [k]) * 0.125f;
	}
vSide [0] = CFloatVector::Avg (gameData.segData.fVertices [m_sideVerts [0]], gameData.segData.fVertices [m_sideVerts [1]]);
vSide [2] = CFloatVector::Avg (gameData.segData.fVertices [m_sideVerts [2]], gameData.segData.fVertices [m_sideVerts [3]]);
vSide [1] = CFloatVector::Avg (gameData.segData.fVertices [m_sideVerts [1]], gameData.segData.fVertices [m_sideVerts [2]]);
vSide [3] = CFloatVector::Avg (gameData.segData.fVertices [m_sideVerts [3]], gameData.segData.fVertices [m_sideVerts [0]]);

VmLineLineIntersection (vSide [0], vSide [2], vSide [1], vSide [3],
								gameData.segData.fVertices [gameData.segData.nVertices],
								gameData.segData.fVertices [gameData.segData.nVertices]);
gameData.segData.vertices [gameData.segData.nVertices].Assign (gameData.segData.fVertices [gameData.segData.nVertices]);
m_sideVerts [4] = gameData.segData.nVertices++;
m_faceP->m_info.nVerts++;
for (i = 0; i < 4; i++, m_triP++) {
	FACES.nTriangles++;
	m_faceP->m_info.nTris++;
	m_triP->nFace = m_faceP - FACES.faces;
	m_triP->nIndex = m_vertexP - FACES.vertices;
	triVertP = n4TriVerts [i];
	for (j = 0; j < 3; j++) {
		k = triVertP [j];
		v = m_sideVerts [k];
		m_triP->index [j] = v;
		*m_vertexP++ = *gameData.segData.fVertices [v].XYZ ();
		if (j == 2) {
			m_texCoordP [2] = texCoord;
			m_faceColorP [2] = color;
			}
		else {
			m_texCoordP [j].v.u = X2F (m_sideP->m_uvls [k].u);
			m_texCoordP [j].v.v = X2F (m_sideP->m_uvls [k].v);
			m_colorP = gameData.render.color.ambient + v;
			m_faceColorP [j] = *m_colorP;
			}
		RotateTexCoord2f (*m_ovlTexCoordP, m_texCoordP [j], (uint8_t) m_sideP->m_nOvlOrient);
		m_ovlTexCoordP++;
		}
	m_normalP = SetTriNormals (m_triP, m_normalP);
	m_texCoordP += 3;
	m_faceColorP += 3;
	}
}

//------------------------------------------------------------------------------

bool CQuadMeshBuilder::BuildVBOs (void)
{
#if GEOMETRY_VBOS
if (!ogl.m_features.bVertexBufferObjects)
	return false;
DestroyVBOs ();
int32_t h, i;
glGenBuffersARB (1, &FACES.vboDataHandle);
if ((i = glGetError ())) {
	glGenBuffersARB (1, &FACES.vboDataHandle);
	if ((i = glGetError ())) {
#	if DBG
		HUDMessage (0, "glGenBuffersARB failed (%d)", i);
#	endif
		ogl.m_features.bVertexBufferObjects = 0;
		return false;
		}
	}
glBindBufferARB (GL_ARRAY_BUFFER_ARB, FACES.vboDataHandle);
if ((i = glGetError ())) {
#	if DBG
	HUDMessage (0, "glBindBufferARB failed (%d)", i);
#	endif
	ogl.m_features.bVertexBufferObjects = 0;
	return false;
	}
FACES.nVertices = gameStates.render.bTriangleMesh ? FACES.nTriangles * 3 : FACES.nFaces * 4;
glBufferDataARB (GL_ARRAY_BUFFER, FACES.nVertices * sizeof (tFaceRenderVertex), NULL, GL_STATIC_DRAW_ARB));
FACES.vertexP = reinterpret_cast<uint8_t*> (glMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB));
FACES.vboIndexHandle = 0;
glGenBuffersARB (1, &FACES.vboIndexHandle);
if (FACES.vboIndexHandle) {
	glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, FACES.vboIndexHandle);
	glBufferDataARB (GL_ELEMENT_ARRAY_BUFFER_ARB, FACES.nVertices * sizeof (uint16_t), NULL, GL_STATIC_DRAW_ARB);
	FACES.indexP = reinterpret_cast<uint16_t*> (glMapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	}
else
	FACES.indexP = NULL;

uint8_t *vertexP = FACES.vertexP;
i = 0;
FACES.iVertices = 0;
memcpy (vertexP, FACES.vertices, h = FACES.nVertices * sizeof (FACES.vertices [0]));
i += h;
FACES.iNormals = i;
memcpy (vertexP + i, FACES.normals, h = FACES.nVertices * sizeof (FACES.normals [0]));
i += h;
FACES.iColor = i;
memcpy (vertexP + i, FACES.color, h = FACES.nVertices * sizeof (FACES.color [0]));
i += h;
FACES.iTexCoord = i;
memcpy (vertexP + i, FACES.texCoord, h = FACES.nVertices * sizeof (FACES.texCoord [0]));
i += h;
FACES.iOvlTexCoord = i;
memcpy (vertexP + i, FACES.ovlTexCoord, h = FACES.nVertices * sizeof (FACES.ovlTexCoord [0]));
i += h;
FACES.iLMapTexCoord = i;
memcpy (vertexP + i, FACES.lMapTexCoord, h = FACES.nVertices * sizeof (FACES.lMapTexCoord [0]));
for (int32_t i = 0; i < FACES.nVertices; i++)
	FACES.indexP [i] = i;

glUnmapBufferARB (GL_ARRAY_BUFFER_ARB);
glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
glUnmapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB);
glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
return true;
#else
return false;
#endif
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::DestroyVBOs (void)
{
#if GEOMETRY_VBOS
if (FACES.vboDataHandle) {
	glDeleteBuffersARB (1, &FACES.vboDataHandle);
	FACES.vboDataHandle = 0;
	}
if (FACES.vboIndexHandle) {
	glDeleteBuffersARB (1, &FACES.vboIndexHandle);
	FACES.vboIndexHandle = 0;
	}
#endif
}

//------------------------------------------------------------------------------

#define FACE_VERTS	6

int32_t CQuadMeshBuilder::Build (int32_t nLevel, bool bRebuild)
{
	int16_t			nSegment, i;
	uint8_t			nSide;

#if !DBG
if (gameStates.render.nMeshQuality > 3)
	gameStates.render.nMeshQuality = 3;
#endif
lightManager.SetMethod ();
if (gameStates.render.nLightingMethod) {
	if (gameStates.render.bPerPixelLighting) {
		gameStates.render.bTriangleMesh = -1;
		gameStates.render.nMeshQuality = 0;
		}
	else 
		gameStates.render.bTriangleMesh = (gameStates.render.nMeshQuality > 0) ? 1 : -1;
	}
else
	gameStates.render.bTriangleMesh = 0;

FACES.nFaces = 0;
FACES.nTriangles = 0;
m_segP = SEGMENTS.Buffer ();
for (nSegment = 0; nSegment < gameData.segData.nSegments; nSegment++, m_segP++) {
	if (IsColoredSeg (nSegment) != 0)
		FACES.nFaces += 6;
	else {
		for (nSide = 0, m_sideP = m_segP->m_sides; nSide < SEGMENT_SIDE_COUNT; nSide++, m_sideP++) {
			if (!m_sideP->FaceCount ())
				continue;
			m_nWall = m_segP->WallNum (nSide);
			m_nWallType = IS_WALL (m_nWall) ? WALL (m_nWall)->IsInvisible () ? 0 : 2 : (m_segP->m_children [nSide] == -1) ? 1 : 0;
			if (m_nWallType)
				FACES.nFaces++;
			else if (IsColoredSeg (m_segP->m_children [nSide]) != 0)
				FACES.nFaces++;
			}
		}
	}

gameData.segData.nFaces = FACES.nFaces;
if (!(gameData.segData.faces.Create () && gameData.render.Create ())) {
	gameData.segData.faces.Destroy ();
	gameData.render.Destroy ();
	PrintLog (-1, "Not enough memory for mesh data\n");
	return 0;
	}

m_faceP = FACES.faces.Buffer ();
m_triP = TRIANGLES.Buffer ();
m_vertexP = FACES.vertices.Buffer ();
m_normalP = FACES.normals.Buffer ();
m_texCoordP = FACES.texCoord.Buffer ();
m_ovlTexCoordP = FACES.ovlTexCoord.Buffer ();
m_lMapTexCoordP = FACES.lMapTexCoord.Buffer ();
m_faceColorP = FACES.color.Buffer ();
m_colorP = gameData.render.color.ambient.Buffer ();
m_segP = SEGMENTS.Buffer ();
m_segFaceP = SEGFACES.Buffer ();
FACES.slidingFaces = NULL;

gameStates.render.nFacePrimitive = gameStates.render.bTriangleMesh ? GL_TRIANGLES : GL_TRIANGLE_FAN;
if (gameStates.render.bSplitPolys)
	gameStates.render.bSplitPolys = (gameStates.render.bPerPixelLighting || !gameStates.render.nMeshQuality) ? 1 : -1;
if (gameStates.render.bTriangleMesh)
	cameraManager.Create ();
PrintLog (1, "Creating face list\n");
// the mesh builder can theoretically add one vertex per side, so resize the vertex buffers
gameData.segData.vertices.Resize (4 * FACES.nFaces /*LEVEL_VERTICES + LEVEL_SIDES*/);
gameData.segData.fVertices.Resize (4 * FACES.nFaces /*LEVEL_VERTICES + LEVEL_SIDES*/);
gameData.render.mine.Resize (4 * FACES.nFaces /*LEVEL_VERTICES + LEVEL_SIDES*/);

FACES.nFaces = 0;
for (nSegment = 0; nSegment < gameData.segData.nSegments; nSegment++, m_segP++, m_segFaceP++) {
	bool bColoredSeg = IsColoredSeg (nSegment) != 0;
#if DBG
	if (nSegment == nDbgSeg)
		BRP;
	if (m_faceP >= FACES.faces.Buffer () + FACES.faces.Length ()) {
		PrintLog (-1);
		return 0;
		}
	if (m_segFaceP >= SEGFACES.Buffer () + SEGFACES.Length ()) {
		PrintLog (-1);
		return 0;
		}
#endif
	m_faceP->m_info.nSegment = nSegment;
	m_nOvlTexCount = 0;
	m_segFaceP->nFaces = 0;
	for (nSide = 0, m_sideP = m_segP->m_sides; nSide < SEGMENT_SIDE_COUNT; nSide++, m_sideP++) {
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			BRP;
#endif
		if (!m_sideP->FaceCount ())
			continue;
		m_nWall = m_segP->WallNum (nSide);
		m_nWallType = IS_WALL (m_nWall) ? WALL (m_nWall)->IsInvisible () ? 0 : 2 : (m_segP->m_children [nSide] == -1) ? 1 : 0;
		CSegment* childSegP = (m_segP->m_children [nSide] < 0) ? NULL : SEGMENT (m_segP->m_children [nSide]);
		bool bColoredChild = IsColoredSeg (m_segP->m_children [nSide]) != 0;
		if (bColoredSeg || bColoredChild || m_nWallType) {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				BRP;
#endif
			memcpy (m_sideVerts, m_segP->Corners (nSide), 4 * sizeof (uint16_t));
			InitFace (nSegment, nSide, bRebuild);
			if ((bColoredSeg || bColoredChild) && 
				 !(m_segP->IsWater (nSide) && (m_segP->HasWaterProp () || (childSegP && childSegP->HasWaterProp ()))) && 
				 !(m_segP->IsLava (nSide) && (m_segP->HasLavaProp () || (childSegP && childSegP->HasLavaProp ()))))
				InitColoredFace (nSegment);
			if (m_nWallType)
				InitTexturedFace ();
			if (gameStates.render.bTriangleMesh) {
				// split in four triangles, using the quad's center of gravity as additional vertex
				if (!gameStates.render.bPerPixelLighting && (m_sideP->m_nType == SIDE_IS_QUAD) &&
					 !m_faceP->m_info.bSlide && (m_faceP->m_info.nCamera < 0) && IsBigFace (m_sideVerts))
					SplitIn4Tris ();
				else
					SplitIn1or2Tris ();
				}
			else
				SetupFace ();
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				BRP;
#endif
			if (!m_segFaceP->nFaces++)
				m_segFaceP->faceP = m_faceP;
			m_faceP++;
			FACES.nFaces++;
			}
		else {
			m_colorP += FACE_VERTS;
			}
		}
	}
#if 1
// any additional vertices have been stored, so prune the buffers to the minimally required size
if (!(gameData.segData.Resize () && gameData.render.lights.Resize () && gameData.render.color.Resize ())) {
	PrintLog (-1);
	return 0;
	}
#endif
for (m_colorP = gameData.render.color.ambient.Buffer (), i = gameData.segData.nVertices; i; i--, m_colorP++)
	if (m_colorP->Alpha () > 1) {
		m_colorP->Red () /= m_colorP->Alpha ();
		m_colorP->Green () /= m_colorP->Alpha ();
		m_colorP->Blue () /= m_colorP->Alpha ();
		m_colorP->Alpha () = 1;
		}
if (gameStates.render.bTriangleMesh && !m_triMeshBuilder.Build (nLevel, gameStates.render.nMeshQuality)) {
	PrintLog (-1);
	return 0;
	}
#if 1
if (!(gameData.render.lights.Resize () && gameData.render.color.Resize ())) {
	PrintLog (-1);
	return 0;
	}
#endif
BuildSlidingFaceList ();
if (gameStates.render.bTriangleMesh)
	cameraManager.Destroy ();
PrintLog (-1);
return 1;
}

//------------------------------------------------------------------------------

int32_t CSegmentData::CountEdges (void)
{
	CSegment	*segP = gameData.Segment (0);
	int32_t	nEdges = 0;

for (int32_t i = 0; i < gameData.segData.nSegments; i++, segP++) {
	CSide* sideP = segP->Side (0);
	for (int32_t j = 0; j < 6; j++, sideP++) {
		if (segP->ChildId (j) < 0) {
			switch (sideP->Shape ()) {
				case SIDE_SHAPE_RECTANGLE:
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

CGeoEdge *CSegmentData::FindEdge (int16_t nVertex1, int16_t nVertex2)
{
	CGeoEdge	*edgeP = gameData.segData.edges.Buffer ();

for (int32_t i = gameData.segData.nEdges; i; i--, edgeP++)
	if ((edgeP->m_nVertices [0] == nVertex1) && (edgeP->m_nVertices [1] == nVertex2))
		return edgeP;
return NULL;
}

//------------------------------------------------------------------------------

int32_t CSegmentData::AddEdge (int16_t nSegment, int16_t nSide, int16_t nVertex1, int16_t nVertex2)
{
if (nVertex1 > nVertex2)
	Swap (nVertex1, nVertex2);
CGeoEdge *edgeP = FindEdge (nVertex1, nVertex2);

int32_t i;

if (edgeP)
	i = 1;
else {
	i = 0;
	edgeP = &gameData.segData.edges [gameData.segData.nEdges++];
	edgeP->m_nVertices [0] = nVertex1;
	edgeP->m_nVertices [1] = nVertex2;
	}
edgeP->m_faces [i].m_nItem = nSegment;
edgeP->m_faces [i].m_nFace = nSide;
edgeP->m_faces [i].m_vNormal = gameData.Segment (nSegment)->Side (nSide)->Normal (2);
return i == 0;
}

//------------------------------------------------------------------------------

int32_t CSegmentData::BuildEdgeList (void)
{
if (!gameData.segData.edges.Create (CountEdges ()))
	return -1;
gameData.segData.edges.Clear (0xff);

CSegment	*segP = gameData.Segment (0);
gameData.segData.nEdges = 0;

for (int32_t i = 0; i < gameData.segData.nSegments; i++, segP++) {
	if (segP->m_function == SEGMENT_FUNC_SKYBOX)
		continue;
	CSide* sideP = segP->Side (0);
	for (int32_t j = 0; j < 6; j++, sideP++) {
		if (segP->ChildId (j) >= 0)
			continue;
		int32_t nVertices;
		switch (sideP->Shape ()) {
			case SIDE_SHAPE_RECTANGLE:
				nVertices = 4;
				break;
			case SIDE_SHAPE_TRIANGLE:
				nVertices = 3;
				break;
			default:
				continue;
			}
		for (int32_t k = 0; k < nVertices; k++)
			AddEdge (i, j, sideP->m_corners [k], sideP->m_corners [(k + 1) % nVertices]);
		}
	}
return gameData.segData.nEdges;
}

//------------------------------------------------------------------------------
//eof
