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

void LoadFaceBitmaps (CSegment *pSeg, CSegFace *pFace);

//------------------------------------------------------------------------------

#define	MAX_EDGE_LEN(nMeshQuality)	fMaxEdgeLen [nMeshQuality]

#define MESH_DATA_VERSION	16

#if POLYGONAL_OUTLINE
extern bool bPolygonalOutline;
#endif

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
ENTER (0, 0);
if (m_nMaxTriangles && m_nMaxEdges) {
	if (!(m_edges.Resize (m_nMaxEdges * 2) && m_triangles.Resize (m_nMaxTriangles * 2))) {
		PrintLog (0, "Not enough memory for building the triangle mesh (%d edges, %d tris).\n", m_nMaxEdges * 2, m_nMaxTriangles * 2);
		FreeData ();
		RETURN (0)
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
		RETURN (0)
		}
	if (!m_triangles.Create (m_nMaxTriangles)) {
		PrintLog (0, "Not enough memory for building the triangle mesh (%d tris).\n", m_nMaxTriangles);
		FreeData ();
		RETURN (0)
		}
	m_edges.Clear (0xff);
	m_triangles.Clear (0xff);
	}
RETURN (1)
}

//------------------------------------------------------------------------------

tEdge *CTriMeshBuilder::FindEdge (uint16_t nVert1, uint16_t nVert2, int32_t i)
{
	tEdge	*pEdge = &m_edges [++i];

for (; i < m_nEdges; i++, pEdge++)
	if ((pEdge->verts [0] == nVert1) && (pEdge->verts [1] == nVert2))
		return pEdge;
return NULL;
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::AddEdge (int32_t nTri, uint16_t nVert1, uint16_t nVert2)
{
ENTER (0, 0);
if (nVert2 < nVert1) {
	uint16_t h = nVert1;
	nVert1 = nVert2;
	nVert2 = h;
	}
#if DBG
if ((nTri < 0) || (nTri >= m_nTriangles))
	RETURN (-1)
#endif
tEdge *pEdge;
for (int32_t i = -1;;) {
	if (!(pEdge = FindEdge (nVert1, nVert2, i)))
		break;
	i = int32_t (pEdge - &m_edges [0]);
	if (pEdge->tris [0] < 0) {
		pEdge->tris [0] = nTri;
		RETURN (i)
		}
	if (pEdge->tris [1] < 0) {
		pEdge->tris [1] = nTri;
		RETURN (i)
		}
	}
if (m_nFreeEdges >= 0) {
	pEdge = &m_edges [m_nFreeEdges];
	m_nFreeEdges = pEdge->nNext;
	pEdge->nNext = -1;
	}
else {
	if ((m_nEdges == m_nMaxEdges - 1) && !AllocData ())
		RETURN (-1)
	pEdge = &m_edges [m_nEdges++];
	}
pEdge->tris [0] = nTri;
pEdge->verts [0] = nVert1;
pEdge->verts [1] = nVert2;
pEdge->fLength = CFloatVector::Dist(gameData.segData.fVertices [nVert1], gameData.segData.fVertices [nVert2]);
RETURN (m_edges.Index (pEdge))
}

//------------------------------------------------------------------------------

tTriangle *CTriMeshBuilder::CreateTriangle (tTriangle *pTriangle, uint16_t index [], int32_t nFace, int32_t nIndex)
{
ENTER (0, 0);
	int32_t	h, i;

if (pTriangle)
	pTriangle->nIndex = nIndex;
else {
	if ((m_nTriangles == m_nMaxTriangles - 1) && !AllocData ())
		RETURN (NULL)
	pTriangle = &m_triangles [m_nTriangles++];
	pTriangle->nIndex = nIndex;
	if (nIndex >= 0) {
		i = TRIANGLES [nIndex].nIndex;
		memcpy (pTriangle->texCoord, FACES.texCoord + i, sizeof (pTriangle->texCoord));
		memcpy (pTriangle->ovlTexCoord, FACES.ovlTexCoord + i, sizeof (pTriangle->ovlTexCoord));
		memcpy (pTriangle->color, FACES.color + i, sizeof (pTriangle->color));
		}
	}
nIndex = m_triangles.Index (pTriangle);
for (i = 0; i < 3; i++) {
	if (0 > (h = AddEdge (nIndex, index [i], index [(i + 1) % 3])))
		RETURN (NULL)
	pTriangle = &m_triangles [nIndex];
	pTriangle->lines [i] = h;
	}
pTriangle->nFace = nFace;
memcpy (pTriangle->index, index, sizeof (pTriangle->index));
RETURN (pTriangle)
}

//------------------------------------------------------------------------------

tTriangle *CTriMeshBuilder::AddTriangle (tTriangle *pTriangle, uint16_t index [], tFaceTriangle *pFaceTriangle)
{
ENTER (0, 0);
RETURN (CreateTriangle (pTriangle, index, pFaceTriangle->nFace, pFaceTriangle - TRIANGLES))
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::DeleteEdge (tEdge *pEdge)
{
ENTER (0, 0);
#if 1
pEdge->nNext = m_nFreeEdges;
m_nFreeEdges = m_edges.Index (pEdge);
#else
	tTriangle	*pTriangle;
	int32_t		h = pEdge - m_edges, i, j;

if (h < --m_nEdges) {
	*pEdge = m_edges [m_nEdges];
	for (i = 0; i < 2; i++) {
		if (pEdge->tris [i] >= 0) {
			pTriangle = m_triangles + pEdge->tris [i];
			for (j = 0; j < 3; j++)
				if (pTriangle->lines [j] == m_nEdges)
					pTriangle->lines [j] = h;
			}
		}
	}
#endif
LEAVE
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::DeleteTriangle (tTriangle *pTriangle)
{
ENTER (0, 0);
	tEdge	*pEdge;
	int32_t	i, nTri = m_triangles.Index (pTriangle);

for (i = 0; i < 3; i++) {
	pEdge = &m_edges [pTriangle->lines [i]];
	if (pEdge->tris [0] == nTri)
		pEdge->tris [0] = pEdge->tris [1];
	pEdge->tris [1] = -1;
	if (pEdge->tris [0] < 0)
		DeleteEdge (pEdge);
	}
LEAVE
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::CreateTriangles (void)
{
ENTER (0, 0);
PrintLog (1, "adding existing triangles\n");
m_nEdges = 0;
m_nTriangles = 0;
m_nMaxTriangles = 0;
m_nMaxTriangles = 0;
m_nFreeEdges = -1;
m_nVertices = gameData.segData.nVertices;
if (!AllocData ()) {
	PrintLog (-1);
	RETURN (0)
	}

CSegFace *pFace;
tFaceTriangle *pFaceTriangle;
tTriangle *pTriangle;
int32_t i, nFace = -1;
int16_t nId = 0;

#if 0
if (gameStates.render.nMeshQuality) {
	i = LEVEL_VERTICES + ((FACES.nTriangles ? FACES.nTriangles / 2 : FACES.nFaces) << (abs (gameStates.render.nMeshQuality)));
	if (!(gameData.segData.fVertices.Resize (i) && gameData.segData.vertices.Resize (i) && gameData.renderData.mine.Resize (i)))
		RETURN (0)
	}
#endif
for (i = FACES.nTriangles, pFaceTriangle = TRIANGLES.Buffer (); i; i--, pFaceTriangle++) {
	if (!(pTriangle = AddTriangle (NULL, pFaceTriangle->index, pFaceTriangle))) {
		FreeData ();
		PrintLog (-1);
		RETURN (0)
		}
	if (nFace == pFaceTriangle->nFace)
		nId++;
	else {
		nFace = pFaceTriangle->nFace;
		nId = 0;
		}
	pTriangle->nId = nId;
	pFace = FACES.faces + pFaceTriangle->nFace;
#if DBG
	if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
		BRP;
#endif
	if (pFace->m_info.bSlide || (pFace->m_info.nCamera >= 0))
		pTriangle->nPass = -2;
	}
PrintLog (-1);
RETURN (m_nTris = m_nTriangles)
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::SplitTriangleByEdge (int32_t nTri, uint16_t nVert1, uint16_t nVert2, int16_t nPass)
{
ENTER (0, 0);
if (nTri < 0)
	RETURN (1)

tTriangle *pTriangle = &m_triangles [nTri];

if (pTriangle->nPass < -1)
	RETURN (1)

CSegFace *pFace = FACES.faces + pTriangle->nFace;

#if DBG
if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
#endif

	int32_t			h, i, nIndex = pTriangle->nIndex;
	uint16_t		nFace = pTriangle->nFace, *pIndex = pTriangle->index, index [4];
	tTexCoord2f	texCoord [4], ovlTexCoord [4];
	CFloatVector	color [4];

// find insertion point for the new vertex
for (i = 0; i < 3; i++) {
	h = pIndex [i];
	if ((h == nVert1) || (h == nVert2))
		break;
	}
if (i == 3) {
	PrintLog (0, "Internal error during construction of the triangle mesh.\n");
	RETURN (0)
	}

h = pIndex [(i + 1) % 3]; //check next vertex index
if ((h == nVert1) || (h == nVert2))
	h = (i + 1) % 3; //insert before i+1
else
	h = i; //insert before i

// build new quad index containing the new vertex
// the two triangle indices will be derived from it (indices 0,1,2 and 1,2,3)
index [0] = gameData.segData.nVertices - 1;
for (i = 1; i < 4; i++) {
	index [i] = pIndex [h];
	texCoord [i] = pTriangle->texCoord [h];
	ovlTexCoord [i] = pTriangle->ovlTexCoord [h];
	color [i] = pTriangle->color [h++];
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
DeleteTriangle (pTriangle); //remove any references to this triangle
if (!(pTriangle = CreateTriangle (pTriangle, index, nFace, nIndex))) //create a new triangle at this location (insert)
	RETURN (0)
pTriangle->nPass = nPass;
pTriangle->nId = (pFace->m_info.nTris)++;
memcpy (pTriangle->color, color, sizeof (pTriangle->color));
memcpy (pTriangle->texCoord, texCoord, sizeof (pTriangle->texCoord));
memcpy (pTriangle->ovlTexCoord, ovlTexCoord, sizeof (pTriangle->ovlTexCoord));

index [1] = index [0];
if (!(pTriangle = CreateTriangle (NULL, index + 1, nFace, -1))) //create a new triangle (append)
	RETURN (0)
pTriangle->nPass = nPass;
pTriangle->nId = (pFace->m_info.nTris)++;
pTriangle->texCoord [0] = texCoord [0];
pTriangle->ovlTexCoord [0] = ovlTexCoord [0];
pTriangle->color [0] = color [0];
memcpy (pTriangle->color + 1, color + 2, 2 * sizeof (pTriangle->color [0]));
memcpy (pTriangle->texCoord + 1, texCoord + 2, 2 * sizeof (pTriangle->texCoord [0]));
memcpy (pTriangle->ovlTexCoord + 1, ovlTexCoord + 2, 2 * sizeof (pTriangle->ovlTexCoord [0]));
FACES.faces [pTriangle->nFace].m_info.nVerts++;
RETURN (1)
}

//------------------------------------------------------------------------------

float CTriMeshBuilder::NewEdgeLen (int32_t nTri, int32_t nVert1, int32_t nVert2)
{
	tTriangle	*pTriangle = &m_triangles [nTri];
	int32_t		h, i;

for (i = 0; i < 3; i++) {
	h = pTriangle->index [i];
	if ((h != nVert1) && (h != nVert2))
		break;
	}
return CFloatVector::Dist(gameData.segData.fVertices [h], gameData.segData.fVertices [gameData.segData.nVertices]);
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::SplitEdge (CSegFace* pFace, tEdge *pEdge, int16_t nPass)
{
ENTER (0, 0);
	int32_t	tris [2];
	uint16_t	verts [2];

memcpy (tris, pEdge->tris, sizeof (tris));
memcpy (verts, pEdge->verts, sizeof (verts));
int32_t i = gameData.segData.fVertices.Length ();
if (gameData.segData.nVertices >= i) {
	i *= 2;
	if (!(gameData.segData.fVertices.Resize (i) && gameData.segData.vertices.Resize (i) && gameData.segData.vertexOwners.Resize (i) && gameData.renderData.mine.Resize (i)))
		RETURN (0)
	}
gameData.segData.fVertices [gameData.segData.nVertices] = CFloatVector::Avg (gameData.segData.fVertices [verts [0]], gameData.segData.fVertices [verts [1]]);
gameData.segData.vertices [gameData.segData.nVertices].Assign (gameData.segData.fVertices [gameData.segData.nVertices]);
gameData.segData.vertexOwners [gameData.segData.nVertices].nSegment = pFace->m_info.nSegment;
gameData.segData.vertexOwners [gameData.segData.nVertices].nSide = pFace->m_info.nSide;
#if 0
if (tris [1] >= 0) {
	if (NewEdgeLen (tris [0], verts [0], verts [1]) + NewEdgeLen (tris [1], verts [0], verts [1]) < MAX_EDGE_LEN (m_nQuality))
		RETURN (-1)
	}
#endif
gameData.segData.nVertices++;
if (!SplitTriangleByEdge (tris [0], verts [0], verts [1], nPass))
	RETURN (0)
if (!SplitTriangleByEdge (tris [1], verts [0], verts [1], nPass))
	RETURN (0)
RETURN (1)
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::SplitTriangle (CSegFace* pFace, tTriangle *pTriangle, int16_t nPass)
{
ENTER (0, 0);
	int32_t	h = 0, i;
	float	l, lMax = 0;

for (i = 0; i < 3; i++) {
	l = m_edges [pTriangle->lines [i]].fLength;
	if (lMax < l) {
		lMax = l;
		h = i;
		}
	}
if (lMax <= MAX_EDGE_LEN (m_nQuality))
	RETURN (-1)
RETURN (SplitEdge (pFace, &m_edges [pTriangle->lines [h]], nPass))
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::SplitTriangles (void)
{
ENTER (0, 0);
	int32_t	bSplit = 0, h, i, j, nSplitRes;
	int16_t	nPass = 0, nMaxPasses = 10 * m_nQuality;

h = 0;
do {
	bSplit = 0;
	j = m_nTriangles;
	PrintLog (1, "splitting triangles (pass %d)\n", nPass);
	for (i = h, h = 0; i < j; i++) {
#if DBG
		CSegFace *pFace = FACES.faces + m_triangles [i].nFace;
		if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
			BRP;
#endif
		if (m_triangles [i].nPass != nPass - 1)
			continue;
#if DBG
		pFace = FACES.faces + m_triangles [i].nFace;
		if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
			BRP;
#endif
		nSplitRes = SplitTriangle (FACES.faces + m_triangles [i].nFace, &m_triangles [i], nPass);
		if (gameData.segData.nVertices == 65536) {
			PrintLog (-1, "Level too big for requested triangle mesh quality.\n");
			RETURN (0)
			}
		if (!nSplitRes)
			RETURN (0)
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
RETURN (1)
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
ENTER (0, 0);
	tFaceTriangle*	pTriangle;
	CRenderPoint*	pPoint = RENDERPOINTS.Buffer ();
	int32_t			h, i, nVertex;

for (i = gameData.segData.nVertices; i; i--, pPoint++) {
/*
	(*pPoint->m_normal.vNormal.XYZ ()).v.c.x =
	(*pPoint->m_normal.vNormal.XYZ ()).v.c.y =
	(*pPoint->m_normal.vNormal.XYZ ()).v.c.z = 0;
*/
	pPoint->Normal ().Reset ();
	}
for (h = 0, pTriangle = FACES.tris.Buffer (); h < FACES.nTriangles; h++, pTriangle++) {
	for (i = 0; i < 3; i++) {
		nVertex = pTriangle->index [i];
#if DBG
		if (nVertex == nDbgVertex)
			BRP;
#endif
		RENDERPOINTS [nVertex].Normal () += FACES.normals [3 * h];
		}
	}
ComputeVertexNormals ();
LEAVE
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::InsertTriangles (void)
{
ENTER (0, 0);
	tTriangle*		pTriangle = &m_triangles [0];
	tFaceTriangle*	pFaceTriangle = TRIANGLES.Buffer ();
	CSegFace*		m_pFace = NULL;
	CFixVector		vNormal;
	int32_t			h, i, nFace = -1;
	GLuint			nIndex = 0;

PrintLog (1, "inserting new triangles\n");
QSortTriangles (0, m_nTriangles - 1);
ResetVertexNormals ();
for (h = 0; h < m_nTriangles; h++, pTriangle++, pFaceTriangle++) {
	pFaceTriangle->nFace = pTriangle->nFace;
	if (pFaceTriangle->nFace == nFace)
		m_pFace->m_info.nTris++;
	else {
		if (m_pFace)
			m_pFace++;
		else
			m_pFace = FACES.faces.Buffer ();
		nFace = pFaceTriangle->nFace;
#if DBG
		if (m_pFace - FACES.faces != nFace)
			RETURN (0)
#endif
		m_pFace->m_info.nFrame = -1;
		m_pFace->m_info.nIndex = nIndex;
		m_pFace->m_info.nTriIndex = h;
		m_pFace->m_info.nTris = 1;
#if USE_RANGE_ELEMENTS
		m_pFace->m_info.vertIndex = FACES.vertIndex + nIndex;
#endif
		}
	pFaceTriangle->nIndex = nIndex;
	memcpy (pFaceTriangle->index, pTriangle->index, sizeof (pTriangle->index));
	for (i = 0; i < 3; i++) {
#if DBG
		if (nIndex + i >= FACES.vertices.Length ())
			BRP;
#endif
		FACES.vertices [nIndex + i].Assign (gameData.segData.fVertices [pTriangle->index [i]]);
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
		RETURN (0)
	memcpy (FACES.texCoord + nIndex, pTriangle->texCoord, sizeof (pTriangle->texCoord));
	memcpy (FACES.ovlTexCoord + nIndex, pTriangle->ovlTexCoord, sizeof (pTriangle->ovlTexCoord));
	memcpy (FACES.color + nIndex, pTriangle->color, sizeof (pTriangle->color));
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
RETURN (1)
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
ENTER (0, 0);
	int32_t			h = 0;
	tSegFaces*	pSegFace = SEGFACES.Buffer ();

for (int32_t i = gameData.segData.nSegments; i; i--, pSegFace++) {
	pSegFace->pFace = &FACES.faces [h];
	h += pSegFace->nFaces;
	}
LEAVE
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::CreateFaceVertLists (void)
{
ENTER (0, 0);
	int32_t*			bTags = new int32_t [gameData.segData.nVertices];
	CSegFace*		pFace;
	tFaceTriangle*	pTriangle;
	int32_t			h, i, j, k, nFace;

//count the vertices of each face
memset (bTags, 0xFF, gameData.segData.nVertices * sizeof (bTags [0]));
gameData.segData.nFaceVerts = 0;
for (i = FACES.nFaces, pFace = FACES.faces.Buffer (), nFace = 0; i; i--, pFace++, nFace++) {
	pFace->m_info.nVerts = 0;
	for (j = pFace->m_info.nTris, pTriangle = TRIANGLES + pFace->m_info.nTriIndex; j; j--, pTriangle++) {
		for (k = 0; k < 3; k++) {
			h = pTriangle->index [k];
			if (bTags [h] != nFace) {
				bTags [h] = nFace;
				pFace->m_info.nVerts++;
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
for (i = FACES.nFaces, pFace = FACES.faces.Buffer (), nFace = 0; i; i--, pFace++, nFace++) {
	pFace->triIndex = FACES.faceVerts + gameData.segData.nFaceVerts;
#if DBG
	if (pFace->m_info.nSegment == nDbgSeg)
		BRP;
#endif
	for (j = pFace->m_info.nTris, pTriangle = TRIANGLES + pFace->m_info.nTriIndex; j; j--, pTriangle++) {
		for (k = 0; k < 3; k++) {
			h = pTriangle->index [k];
			if (bTags [h] != nFace) {
				bTags [h] = nFace;
				FACES.faceVerts [gameData.segData.nFaceVerts++] = h;
				}
			}
		}
	}
#if 1
//sort each face's vertex index list
for (i = FACES.nFaces, pFace = FACES.faces.Buffer (); i; i--, pFace++)
	SortFaceVertList (pFace->triIndex, 0, pFace->m_info.nVerts - 1);
#endif
LEAVE
}

//------------------------------------------------------------------------------

char *CTriMeshBuilder::DataFilename (char *pszFilename, int32_t nLevel)
{
return GameDataFilename (pszFilename, "mesh", nLevel, (gameStates.render.bTriangleMesh < 0) ? -1 : m_nQuality);
}

//------------------------------------------------------------------------------

bool CTriMeshBuilder::Load (int32_t nLevel, bool bForce)
{
ENTER (0, 0);
	CFile					cf;
	tMeshDataHeader	mdh;
	int32_t				nSize, nExpectedSize;
	bool					bOk;
	char					szFilename [FILENAME_LEN];
	char					*ioBuffer = NULL, *pBuffer;

if (!(gameStates.render.bTriangleMesh && (gameStates.app.bCacheMeshes || bForce)))
	RETURN (false)
if (!cf.Open (DataFilename (szFilename, nLevel), gameFolders.var.szMeshes, "rb", 0) &&
	 !cf.Open (DataFilename (szFilename, nLevel), gameFolders.var.szCache, "rb", 0))
	RETURN (false)
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
	pBuffer = ioBuffer;
	FACES.Destroy ();
	gameData.segData.vertices.Create (mdh.nVertices);
	memcpy (gameData.segData.vertices.Buffer (), pBuffer, nSize = sizeof (gameData.segData.vertices [0]) * mdh.nVertices);
	pBuffer += nSize;
	gameData.segData.fVertices.Create (mdh.nVertices);
	memcpy (gameData.segData.fVertices.Buffer (), pBuffer, nSize = sizeof (gameData.segData.fVertices [0]) * mdh.nVertices);
	pBuffer += nSize;
	FACES.faces.Create (mdh.nFaces);
	FACES.faces.Clear ();
	for (int32_t i = 0; i < mdh.nFaces; i++, pBuffer += sizeof (CSegFaceInfo))
		memcpy (&FACES.faces [i].m_info, pBuffer, sizeof (CSegFaceInfo));
	FACES.tris.Create (mdh.nTris);
	memcpy (FACES.tris.Buffer (), pBuffer, nSize = sizeof (FACES.tris [0]) * mdh.nTris);
	pBuffer += nSize;
	FACES.vertices.Create (nTriVerts);
	memcpy (FACES.vertices.Buffer (), pBuffer, nSize = sizeof (FACES.vertices [0]) * nTriVerts);
	pBuffer += nSize;
	FACES.normals.Create (nTriVerts);
	memcpy (FACES.normals.Buffer (), pBuffer, nSize = sizeof (FACES.normals [0]) * nTriVerts);
	pBuffer += nSize;
	FACES.texCoord.Create (nTriVerts);
	memcpy (FACES.texCoord.Buffer (), pBuffer, nSize = sizeof (FACES.texCoord [0]) * nTriVerts);
	pBuffer += nSize;
	FACES.ovlTexCoord.Create (nTriVerts);
	memcpy (FACES.ovlTexCoord.Buffer (), pBuffer, nSize = sizeof (FACES.ovlTexCoord [0]) * nTriVerts);
	pBuffer += nSize;
	FACES.color.Create (nTriVerts);
	memcpy (FACES.color.Buffer (), pBuffer, nSize = sizeof (FACES.color [0]) * nTriVerts);
	pBuffer += nSize;
	FACES.lMapTexCoord.Create (gameData.segData.nFaces * 3 * 2);
	memcpy (FACES.lMapTexCoord.Buffer (), pBuffer, nSize = sizeof (FACES.lMapTexCoord [0]) * FACES.lMapTexCoord.Length ());
	pBuffer += nSize;
	FACES.faceVerts.Create (mdh.nFaceVerts);
	memcpy (FACES.faceVerts.Buffer (), pBuffer, nSize = sizeof (FACES.faceVerts [0]) * mdh.nFaceVerts);
	pBuffer += nSize;
	VERTEX_OWNERS.Resize (mdh.nVertices);
	memcpy (VERTEX_OWNERS.Buffer (), pBuffer, nSize = sizeof (VERTEX_OWNERS [0]) * mdh.nVertices);
	gameData.renderData.mine.Resize (mdh.nVertices);
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
RETURN (bOk)
}

//------------------------------------------------------------------------------

bool CTriMeshBuilder::Save (int32_t nLevel)
{
ENTER (0, 0);
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
	RETURN (false)
if (!cf.Open (DataFilename (szFilename, nLevel), gameFolders.var.szMeshes, "wb", 0)) {
	PrintLog (0, "Couldn't save mesh data (check access rights of '%s')\n", gameFolders.var.szMeshes);
	RETURN (false)
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
RETURN (bOk)
}

//------------------------------------------------------------------------------

int32_t CTriMeshBuilder::Build (int32_t nLevel, int32_t nQuality)
{
ENTER (0, 0);
PrintLog (1, "creating triangle mesh\n");
m_nQuality = nQuality;
if (Load (nLevel)) {
	PrintLog (-1);
	RETURN (1)
	}
if (!CreateTriangles ()) {
	gameData.segData.nVertices = m_nVertices;
	PrintLog (-1);
	RETURN (0)
	}
if ((gameStates.render.bTriangleMesh > 0) && !SplitTriangles ()) {
	gameData.segData.nVertices = m_nVertices;
	PrintLog (-1);
	RETURN (0)
	}

if ((m_nTriangles > (LEVEL_FACES << gameStates.render.nMeshQuality)) && !gameData.segData.faces.Resize ()) {
	gameData.segData.faces.Destroy ();
	gameData.renderData.Destroy ();
	PrintLog (-1, "Not enough memory to create mesh data in requested quality\n");
	RETURN (0)
	}

if (!InsertTriangles ()) {
	gameData.segData.nVertices = m_nVertices;
	PrintLog (-1);
	RETURN (0)
	}
if (gameStates.app.bReadOnly) {
	PrintLog (-1);
	RETURN (1)
	}
Save (nLevel);
int32_t nLoadRes = Load (nLevel, true); //Load will rebuild all face data buffers, reducing their memory footprint
PrintLog (-1);
RETURN (nLoadRes)
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

CFloatVector3 *CQuadMeshBuilder::SetTriNormals (tFaceTriangle *pTriangle, CFloatVector3 *m_pNormal)
{
CFloatVector vNormalf = CFloatVector::Normal (gameData.segData.fVertices [pTriangle->index [0]],
															 gameData.segData.fVertices [pTriangle->index [1]], gameData.segData.fVertices [pTriangle->index [2]]);
*m_pNormal++ = *vNormalf.XYZ ();
*m_pNormal++ = *vNormalf.XYZ ();
*m_pNormal++ = *vNormalf.XYZ ();
return m_pNormal;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitFace (int16_t nSegment, uint8_t nSide, bool bRebuild)
{
ENTER (0, 0);
if (bRebuild)
	m_pFace->m_info.nTris = 0;
else
	memset (m_pFace, 0, sizeof (*m_pFace));
CSide* pSide = SEGMENT (nSegment)->Side (nSide);
m_pFace->m_info.nSegment = nSegment;
m_pFace->m_info.nVerts = pSide->CornerCount ();
m_pFace->m_info.nFrame = -1;
m_pFace->m_info.nIndex = m_pVertex - FACES.vertices;
if (gameStates.render.bTriangleMesh)
	m_pFace->m_info.nTriIndex = m_pFaceTriangle - TRIANGLES;
memcpy (m_pFace->m_info.index, m_sideVerts, sizeof (m_pFace->m_info.index));
m_pFace->m_info.nType = gameStates.render.bTriangleMesh ? m_pSide->m_nType : -1;
m_pFace->m_info.nSegment = nSegment;
m_pFace->m_info.nSide = nSide;
m_pFace->m_info.nTriangles = pSide->FaceCount ();
m_pFace->m_info.nWall = gameStates.app.bD2XLevel ? m_nWall : IS_WALL (m_nWall) ? m_nWall : (uint16_t) -1;
m_pFace->m_info.bAnimation = IsAnimatedTexture (m_pFace->m_info.nBaseTex) || IsAnimatedTexture (m_pFace->m_info.nOvlTex);
m_pFace->m_info.bHasColor = 0;
m_pFace->m_info.fRads [0] = X2F (pSide->m_rads [0]); //(float) sqrt ((rMinf * rMinf + rMaxf * rMaxf) / 2);
m_pFace->m_info.fRads [1] = X2F (pSide->m_rads [1]); //(float) sqrt ((rMinf * rMinf + rMaxf * rMaxf) / 2);
LEAVE
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitTexturedFace (void)
{
ENTER (0, 0);
	static char szEmpty [] = "";
	
m_pFace->m_info.nBaseTex = m_pSide->m_nBaseTex;
if ((m_pFace->m_info.nOvlTex = m_pSide->m_nOvlTex))
	m_nOvlTexCount++;
m_pFace->m_info.bSlide = (gameData.pigData.tex.pTexMapInfo [m_pFace->m_info.nBaseTex].slide_u || gameData.pigData.tex.pTexMapInfo [m_pFace->m_info.nBaseTex].slide_v);
m_pFace->m_info.nCamera = IsMonitorFace (m_pFace->m_info.nSegment, m_pFace->m_info.nSide, 1);
m_pFace->m_info.bIsLight = IsLight (m_pFace->m_info.nBaseTex) || (m_pFace->m_info.nOvlTex && IsLight (m_pFace->m_info.nOvlTex));
m_pFace->m_info.nOvlOrient = (uint8_t) m_pSide->m_nOvlOrient;
m_pFace->m_info.bTextured = 1;
m_pFace->m_info.bTransparent = 0;
int32_t nTexture = m_pFace->m_info.nOvlTex ? m_pFace->m_info.nOvlTex : m_pFace->m_info.nBaseTex;
char *pszName = (nTexture < MAX_WALL_TEXTURES)
					 ? gameData.pigData.tex.bitmapFiles [gameStates.app.bD1Mission][gameData.pigData.tex.pBmIndex [nTexture].index].name
					 : szEmpty;
if (strstr (pszName, "misc17") != NULL)
	m_pFace->m_info.bSparks = (nTexture == m_pFace->m_info.nBaseTex) ? 1 : 2;
if (m_nWallType < 2)
	m_pFace->m_info.bAdditive = 0;
else if (WALL (m_nWall)->flags & WALL_RENDER_ADDITIVE)
	m_pFace->m_info.bAdditive = 2;
else if (strstr (pszName, "lava"))
	m_pFace->m_info.bAdditive = 2;
else
	m_pFace->m_info.bAdditive = (strstr (pszName, "force") || m_pFace->m_info.bSparks) ? 1 : 0;
LEAVE
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitColoredFace (int16_t nSegment)
{
ENTER (0, 0);
m_pFace->m_info.nBaseTex = -1;
m_pFace->m_info.bTransparent = 1;
m_pFace->m_info.bSegColor = missionConfig.m_bColoredSegments;
m_pFace->m_info.bAdditive = 0;
#if DBG
if ((SEGMENT (nSegment)->m_props & (SEGMENT_PROP_WATER | SEGMENT_PROP_LAVA)) != 0)
	BRP;
#endif
LEAVE
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SetupLMapTexCoord (tTexCoord2f *pTexCoord)
{
ENTER (0, 0);
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
if ((m_pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_pFace->m_info.nSide == nDbgSide)))
	BRP;
#endif
int32_t i = m_pFace->m_info.nLightmap % LIGHTMAP_BUFSIZE;
float x = (float) (i % LIGHTMAP_ROWSIZE);
float y = (float) (i / LIGHTMAP_ROWSIZE);
float step = 1.0f / (float) LIGHTMAP_ROWSIZE;
#if 0
const float border = 0.0f;
#else
const float border = 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
#endif
pTexCoord [0].v.u = 
pTexCoord [3].v.u = x * step + border;
pTexCoord [1].v.u =
pTexCoord [2].v.u = (x + 1) * step - border;
pTexCoord [0].v.v = 
pTexCoord [1].v.v = y * step + border;
pTexCoord [2].v.v = 
pTexCoord [3].v.v = (y + 1) * step - border;
LEAVE
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SetupFace (void)
{
ENTER (0, 0);
	int32_t			i;
	CFixVector		vNormal;
	CFloatVector3	vNormalf;

vNormal = m_pSide->m_normals [2];
vNormalf.Assign (vNormal);
for (i = 0; i < 4; i++) {
	uint16_t h = m_sideVerts [i];
	if (h != 0xffff)
		*m_pVertex++ = *gameData.segData.fVertices [h].XYZ ();
	*m_pNormal++ = vNormalf;
	m_pTexCoord->v.u = X2F (m_pSide->m_uvls [i].u);
	m_pTexCoord->v.v = X2F (m_pSide->m_uvls [i].v);
	RotateTexCoord2f (*m_pOvlTexCoord, *m_pTexCoord, (uint8_t) m_pSide->m_nOvlOrient);
#if DBG
	if (m_pTexCoord > FACES.texCoord.Buffer () + FACES.texCoord.Length ())
		LEAVE
	if (m_pOvlTexCoord > FACES.ovlTexCoord.Buffer () + FACES.ovlTexCoord.Length ())
		LEAVE
#endif
	m_pTexCoord++;
	m_pOvlTexCoord++;
	if (!gameStates.app.bNostalgia)
		*m_pFaceColor = gameData.renderData.color.ambient [h];
	else {
		m_pFaceColor->Red () = 
		m_pFaceColor->Green () = 
		m_pFaceColor->Blue () = X2F (m_pSide->m_uvls [i].l);
		m_pFaceColor->Alpha () = 1;
		}
	m_pFaceColor++;
	}
SetupLMapTexCoord (m_pLightmapTexCoord);
m_pLightmapTexCoord += 4;
LEAVE
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SplitIn1or2Tris (void)
{
ENTER (0, 0);
	static int16_t	n2TriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};

	int32_t		h, i, j, k, v, nType;
	int16_t		*triVertP;
	tTexCoord2f	lMapTexCoord [4];

#if DBG
if ((m_pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_pFace->m_info.nSide == nDbgSide)))
	BRP;
#endif
SetupLMapTexCoord (lMapTexCoord);
nType = (m_pSide->m_nType == SIDE_IS_TRI_13);
if (!(h = m_pSide->FaceCount ()))
	LEAVE
for (i = 0; i < h; i++, m_pFaceTriangle++) {
	FACES.nTriangles++;
	m_pFace->m_info.nTris++;
	m_pFaceTriangle->nFace = m_pFace - FACES.faces;
	m_pFaceTriangle->nIndex = m_pVertex - FACES.vertices;
	triVertP = n2TriVerts [nType][i];
	for (j = 0; j < 3; j++) {
		k = triVertP [j];
		v = m_sideVerts [k];
		m_pFaceTriangle->index [j] = v;
		*m_pVertex++ = *gameData.segData.fVertices [v].XYZ ();
		m_pTexCoord->v.u = X2F (m_pSide->m_uvls [k].u);
		m_pTexCoord->v.v = X2F (m_pSide->m_uvls [k].v);
		RotateTexCoord2f (*m_pOvlTexCoord, *m_pTexCoord, (uint8_t) m_pSide->m_nOvlOrient);
		*m_pLightmapTexCoord = lMapTexCoord [k];
		m_pTexCoord++;
		m_pOvlTexCoord++;
		m_pLightmapTexCoord++;
		m_pColor = gameData.renderData.color.ambient + v;
		m_pFaceColor->Assign (*m_pColor);
		++m_pFaceColor;
		}
	m_pNormal = SetTriNormals (m_pFaceTriangle, m_pNormal);
	}
LEAVE
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
	RETURN (-1)
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
ENTER (0, 0);
	CSegFace	*pFace = FACES.faces.Buffer ();
	CArray<CSegFace*>	keyFaceRef;

keyFaceRef.Create (FACES.nFaces);
for (int32_t i = 0; i < FACES.nFaces; i++, pFace++) {
#if DBG
	if (i == nDbgFace)
		BRP;
#endif
	keyFaceRef [i] = pFace;
	if (pFace->m_info.nBaseTex < 0)
		pFace->m_info.nKey = 0x7FFFFFFF;
	else if (pFace->m_info.nOvlTex <= 0)
		pFace->m_info.nKey = int32_t (pFace->m_info.nBaseTex);
	else {
		LoadFaceBitmaps (SEGMENT (pFace->m_info.nSegment), pFace);
		if (pFace->bmTop && (pFace->bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT))
			pFace->m_info.nKey = int32_t (pFace->m_info.nBaseTex) + int32_t (pFace->m_info.nOvlTex) * MAX_WALL_TEXTURES * MAX_WALL_TEXTURES;
		else
			pFace->m_info.nKey = int32_t (pFace->m_info.nBaseTex) + int32_t (pFace->m_info.nOvlTex) * MAX_WALL_TEXTURES;
		}
	}

CQuickSort<CSegFace*> qs;
qs.SortAscending (keyFaceRef.Buffer (), 0, FACES.nFaces - 1, reinterpret_cast<CQuickSort<CSegFace*>::comparator> (CQuadMeshBuilder::CompareFaceKeys));

int32_t i, nKey = -1;
gameData.segData.nFaceKeys = -1;

for (i = 0; i < FACES.nFaces; i++, pFace++) {
	pFace = keyFaceRef [i];
	if (nKey != pFace->m_info.nKey) {
		nKey = pFace->m_info.nKey;
		++gameData.segData.nFaceKeys;
		}
	pFace->m_info.nKey = gameData.segData.nFaceKeys;
	}
++gameData.segData.nFaceKeys;
gameData.renderData.faceIndex.Create ();
LEAVE
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::BuildSlidingFaceList (void)
{
ENTER (0, 0);
	CSegFace	*pFace = FACES.faces.Buffer ();

FACES.slidingFaces = NULL;
for (int32_t i = FACES.nFaces; i; i--, pFace++)
	if (pFace->m_info.bSlide) {
		pFace->nextSlidingFace = FACES.slidingFaces;
		FACES.slidingFaces = pFace;
		}
LEAVE
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::RebuildLightmapTexCoord (void)
{
ENTER (0, 0);
	static int16_t	n2TriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};

	int32_t			h, i, j, k, nFace;
	int16_t			*triVertP;
	tTexCoord2f	lMapTexCoord [4];

m_pFace = FACES.faces.Buffer ();
m_pLightmapTexCoord = FACES.lMapTexCoord.Buffer ();
for (nFace = FACES.nFaces; nFace; nFace--, m_pFace++) {
	SetupLMapTexCoord (lMapTexCoord);
	h = (SEGMENT (m_pFace->m_info.nSegment)->Type (m_pFace->m_info.nSide) == SIDE_IS_TRI_13);
	for (i = 0; i < m_pFace->m_info.nTriangles; i++, m_pFaceTriangle++) {
		triVertP = n2TriVerts [h][i];
		for (j = 0; j < 3; j++) {
			k = triVertP [j];
			*m_pLightmapTexCoord = lMapTexCoord [k];
			m_pLightmapTexCoord++;
			}
		}
	}
LEAVE
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SplitIn4Tris (void)
{
ENTER (0, 0);
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
	texCoord.v.u += X2F (m_pSide->m_uvls [i].u + m_pSide->m_uvls [j].u) / 8;
	texCoord.v.v += X2F (m_pSide->m_uvls [i].v + m_pSide->m_uvls [j].v) / 8;
	h = m_sideVerts [i];
	k = m_sideVerts [j];
	color += (gameData.renderData.color.ambient [h] + gameData.renderData.color.ambient [k]) * 0.125f;
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
m_pFace->m_info.nVerts++;
for (i = 0; i < 4; i++, m_pFaceTriangle++) {
	FACES.nTriangles++;
	m_pFace->m_info.nTris++;
	m_pFaceTriangle->nFace = m_pFace - FACES.faces;
	m_pFaceTriangle->nIndex = m_pVertex - FACES.vertices;
	triVertP = n4TriVerts [i];
	for (j = 0; j < 3; j++) {
		k = triVertP [j];
		v = m_sideVerts [k];
		m_pFaceTriangle->index [j] = v;
		*m_pVertex++ = *gameData.segData.fVertices [v].XYZ ();
		if (j == 2) {
			m_pTexCoord [2] = texCoord;
			m_pFaceColor [2] = color;
			}
		else {
			m_pTexCoord [j].v.u = X2F (m_pSide->m_uvls [k].u);
			m_pTexCoord [j].v.v = X2F (m_pSide->m_uvls [k].v);
			m_pColor = gameData.renderData.color.ambient + v;
			m_pFaceColor [j] = *m_pColor;
			}
		RotateTexCoord2f (*m_pOvlTexCoord, m_pTexCoord [j], (uint8_t) m_pSide->m_nOvlOrient);
		m_pOvlTexCoord++;
		}
	m_pNormal = SetTriNormals (m_pFaceTriangle, m_pNormal);
	m_pTexCoord += 3;
	m_pFaceColor += 3;
	}
LEAVE
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
FACES.pVertex = reinterpret_cast<uint8_t*> (glMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB));
FACES.vboIndexHandle = 0;
glGenBuffersARB (1, &FACES.vboIndexHandle);
if (FACES.vboIndexHandle) {
	glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, FACES.vboIndexHandle);
	glBufferDataARB (GL_ELEMENT_ARRAY_BUFFER_ARB, FACES.nVertices * sizeof (uint16_t), NULL, GL_STATIC_DRAW_ARB);
	FACES.pIndex = reinterpret_cast<uint16_t*> (glMapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	}
else
	FACES.pIndex = NULL;

uint8_t *pVertex = FACES.pVertex;
i = 0;
FACES.iVertices = 0;
memcpy (pVertex, FACES.vertices, h = FACES.nVertices * sizeof (FACES.vertices [0]));
i += h;
FACES.iNormals = i;
memcpy (pVertex + i, FACES.normals, h = FACES.nVertices * sizeof (FACES.normals [0]));
i += h;
FACES.iColor = i;
memcpy (pVertex + i, FACES.color, h = FACES.nVertices * sizeof (FACES.color [0]));
i += h;
FACES.iTexCoord = i;
memcpy (pVertex + i, FACES.texCoord, h = FACES.nVertices * sizeof (FACES.texCoord [0]));
i += h;
FACES.iOvlTexCoord = i;
memcpy (pVertex + i, FACES.ovlTexCoord, h = FACES.nVertices * sizeof (FACES.ovlTexCoord [0]));
i += h;
FACES.iLMapTexCoord = i;
memcpy (pVertex + i, FACES.lMapTexCoord, h = FACES.nVertices * sizeof (FACES.lMapTexCoord [0]));
for (int32_t i = 0; i < FACES.nVertices; i++)
	FACES.pIndex [i] = i;

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
ENTER (0, 0);
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
m_pSeg = SEGMENTS.Buffer ();
for (nSegment = 0; nSegment < gameData.segData.nSegments; nSegment++, m_pSeg++) {
	if (IsColoredSeg (nSegment) != 0)
		FACES.nFaces += 6;
	else {
		for (nSide = 0, m_pSide = m_pSeg->m_sides; nSide < SEGMENT_SIDE_COUNT; nSide++, m_pSide++) {
			if (!m_pSide->FaceCount ())
				continue;
			m_nWall = m_pSeg->WallNum (nSide);
			m_nWallType = IS_WALL (m_nWall) ? WALL (m_nWall)->IsInvisible () ? 0 : 2 : (m_pSeg->m_children [nSide] == -1) ? 1 : 0;
			if (m_nWallType)
				FACES.nFaces++;
			else if (IsColoredSeg (m_pSeg->m_children [nSide]) != 0)
				FACES.nFaces++;
			}
		}
	}

gameData.segData.nFaces = FACES.nFaces;
if (!(gameData.segData.faces.Create () && gameData.renderData.Create ())) {
	gameData.segData.faces.Destroy ();
	gameData.renderData.Destroy ();
	PrintLog (-1, "Not enough memory for mesh data\n");
	RETURN (0)
	}

m_pFace = FACES.faces.Buffer ();
m_pFaceTriangle = TRIANGLES.Buffer ();
m_pVertex = FACES.vertices.Buffer ();
m_pNormal = FACES.normals.Buffer ();
m_pTexCoord = FACES.texCoord.Buffer ();
m_pOvlTexCoord = FACES.ovlTexCoord.Buffer ();
m_pLightmapTexCoord = FACES.lMapTexCoord.Buffer ();
m_pFaceColor = FACES.color.Buffer ();
m_pColor = gameData.renderData.color.ambient.Buffer ();
m_pSeg = SEGMENTS.Buffer ();
m_pSegFace = SEGFACES.Buffer ();
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
gameData.renderData.mine.Resize (4 * FACES.nFaces /*LEVEL_VERTICES + LEVEL_SIDES*/);

FACES.nFaces = 0;
for (nSegment = 0; nSegment < gameData.segData.nSegments; nSegment++, m_pSeg++, m_pSegFace++) {
	bool bColoredSeg = IsColoredSeg (nSegment) != 0;
#if DBG
	if (nSegment == nDbgSeg)
		BRP;
	if (m_pFace >= FACES.faces.Buffer () + FACES.faces.Length ()) {
		PrintLog (-1);
		RETURN (0)
		}
	if (m_pSegFace >= SEGFACES.Buffer () + SEGFACES.Length ()) {
		PrintLog (-1);
		RETURN (0)
		}
#endif
	m_pFace->m_info.nSegment = nSegment;
	m_nOvlTexCount = 0;
	m_pSegFace->nFaces = 0;
	for (nSide = 0, m_pSide = m_pSeg->m_sides; nSide < SEGMENT_SIDE_COUNT; nSide++, m_pSide++) {
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			BRP;
#endif
		if (!m_pSide->FaceCount ())
			continue;
		m_nWall = m_pSeg->WallNum (nSide);
		m_nWallType = IS_WALL (m_nWall) ? WALL (m_nWall)->IsInvisible () ? 0 : 2 : (m_pSeg->m_children [nSide] == -1) ? 1 : 0;
		CSegment* pChildSeg = (m_pSeg->m_children [nSide] < 0) ? NULL : SEGMENT (m_pSeg->m_children [nSide]);
		bool bColoredChild = IsColoredSeg (m_pSeg->m_children [nSide]) != 0;
		if (bColoredSeg || bColoredChild || m_nWallType) {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				BRP;
#endif
			memcpy (m_sideVerts, m_pSeg->Corners (nSide), 4 * sizeof (uint16_t));
			InitFace (nSegment, nSide, bRebuild);
			if ((bColoredSeg || bColoredChild) && 
				 !(m_pSeg->IsWater (nSide) && (m_pSeg->HasWaterProp () || (pChildSeg && pChildSeg->HasWaterProp ()))) && 
				 !(m_pSeg->IsLava (nSide) && (m_pSeg->HasLavaProp () || (pChildSeg && pChildSeg->HasLavaProp ()))))
				InitColoredFace (nSegment);
			if (m_nWallType)
				InitTexturedFace ();
			if (gameStates.render.bTriangleMesh) {
				// split in four triangles, using the quad's center of gravity as additional vertex
				if (!gameStates.render.bPerPixelLighting && (m_pSide->m_nType == SIDE_IS_QUAD) &&
					 !m_pFace->m_info.bSlide && (m_pFace->m_info.nCamera < 0) && IsBigFace (m_sideVerts))
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
			if (!m_pSegFace->nFaces++)
				m_pSegFace->pFace = m_pFace;
			m_pFace++;
			FACES.nFaces++;
			}
		else {
			m_pColor += FACE_VERTS;
			}
		}
	}
#if 1
// any additional vertices have been stored, so prune the buffers to the minimally required size
if (!(gameData.segData.Resize () && gameData.renderData.lights.Resize () && gameData.renderData.color.Resize ())) {
	PrintLog (-1);
	RETURN (0)
	}
#endif
for (m_pColor = gameData.renderData.color.ambient.Buffer (), i = gameData.segData.nVertices; i; i--, m_pColor++)
	if (m_pColor->Alpha () > 1) {
		m_pColor->Red () /= m_pColor->Alpha ();
		m_pColor->Green () /= m_pColor->Alpha ();
		m_pColor->Blue () /= m_pColor->Alpha ();
		m_pColor->Alpha () = 1;
		}
if (gameStates.render.bTriangleMesh && !m_triMeshBuilder.Build (nLevel, gameStates.render.nMeshQuality)) {
	PrintLog (-1);
	RETURN (0)
	}
#if 1
if (!(gameData.renderData.lights.Resize () && gameData.renderData.color.Resize ())) {
	PrintLog (-1);
	RETURN (0)
	}
#endif
BuildSlidingFaceList ();
if (gameStates.render.bTriangleMesh)
	cameraManager.Destroy ();
PrintLog (-1);
RETURN (1)
}

//------------------------------------------------------------------------------
//eof
