/* $Id: gamemine.c, v 1.26 2003/10/22 15:00:37 schaffner Exp $ */
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

#include "inferno.h"
#include "error.h"
#include "text.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "gameseg.h"
#include "switch.h"
#include "ogl_defs.h"
#include "oof.h"
#include "lightmap.h"
#include "render.h"
#include "gameseg.h"

#include "game.h"
#include "menu.h"
#include "newmenu.h"
#include "createmesh.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "cfile.h"
#include "fuelcen.h"

#include "hash.h"
#include "key.h"
#include "piggy.h"

#include "byteswap.h"
#include "gamesave.h"
#include "u_mem.h"
#include "vecmat.h"
#include "gamepal.h"
#include "paging.h"
#include "maths.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "renderlib.h"
#include "createmesh.h"

using namespace mesh;

//------------------------------------------------------------------------------

#define	MAX_EDGE_LEN	fMaxEdgeLen [gameOpts->render.nMeshQuality]

#define MESH_DATA_VERSION 4

//------------------------------------------------------------------------------

typedef struct tMeshDataHeader {
	int	nVersion;
	int	nSegments;
	int	nVertices;
	int	nFaceVerts;
	int	nFaces;
	int	nTris;
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

int CTriMeshBuilder::AllocData (void)
{
if (m_nMaxTriangles && m_nMaxEdges) {
	if (!(m_edges.Resize (m_nMaxEdges * 2) && m_triangles.Resize (m_nMaxTriangles * 2))) {
		FreeData ();
		return 0;
		}
	memset (&m_edges [m_nMaxTriangles], 0xff, m_nMaxTriangles * sizeof (tEdge));
	memset (&m_triangles [m_nMaxEdges], 0xff, m_nMaxTriangles * sizeof (tTriangle));
	m_nMaxTriangles *= 2;
	m_nMaxEdges *= 2;
	}
else {
	m_nMaxTriangles = gameData.segs.nFaces * 4;
	m_nMaxEdges = gameData.segs.nFaces * 4;
	if (!m_edges.Create (m_nMaxEdges))
		return 0;
	if (!m_triangles.Create (m_nMaxTriangles)) {
		FreeData ();
		return 0;
		}
	m_edges.Clear (0xff);
	m_triangles.Clear (0xff);
	}
return 1;
}

//------------------------------------------------------------------------------

tEdge *CTriMeshBuilder::FindEdge (ushort nVert1, ushort nVert2, int i)
{
	tEdge	*edgeP = &m_edges [++i];

for (; i < m_nEdges; i++, edgeP++)
	if ((edgeP->verts [0] == nVert1) && (edgeP->verts [1] == nVert2))
		return edgeP;
return NULL;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::AddEdge (int nTri, ushort nVert1, ushort nVert2)
{
if (nVert2 < nVert1) {
	ushort h = nVert1;
	nVert1 = nVert2;
	nVert2 = h;
	}
#if DBG
if ((nTri < 0) || (nTri >= m_nTriangles))
	return -1;
#endif
tEdge *edgeP;
for (int i = -1;;) {
	if (!(edgeP = FindEdge (nVert1, nVert2, i)))
		break;
	i = edgeP - &m_edges [0];
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
edgeP->fLength = CFloatVector::Dist(gameData.segs.fVertices [nVert1], gameData.segs.fVertices [nVert2]);
return m_edges.Index (edgeP);
}

//------------------------------------------------------------------------------

tTriangle *CTriMeshBuilder::CreateTriangle (tTriangle *triP, ushort index [], int nFace, int nIndex)
{
	int	h, i;

if (triP)
	triP->nIndex = nIndex;
else {
	if ((m_nTriangles == m_nMaxTriangles - 1) && !AllocData ())
		return NULL;
	triP = &m_triangles [m_nTriangles++];
	triP->nIndex = nIndex;
	if (nIndex >= 0) {
		i = TRIANGLES [nIndex].nIndex;
		memcpy (triP->texCoord, gameData.segs.faces.texCoord + i, sizeof (triP->texCoord));
		memcpy (triP->ovlTexCoord, gameData.segs.faces.ovlTexCoord + i, sizeof (triP->ovlTexCoord));
		memcpy (triP->color, gameData.segs.faces.color + i, sizeof (triP->color));
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

tTriangle *CTriMeshBuilder::AddTriangle (tTriangle *triP, ushort index [], grsTriangle *grsTriP)
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
	int		h = edgeP - m_edges, i, j;

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
	int	i, nTri = m_triangles.Index (triP);

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

int CTriMeshBuilder::CreateTriangles (void)
{
PrintLog ("   adding existing triangles\n");
m_nEdges = 0;
m_nTriangles = 0;
m_nMaxTriangles = 0;
m_nMaxTriangles = 0;
m_nFreeEdges = -1;
m_nVertices = gameData.segs.nVertices;
if (!AllocData ())
	return 0;

tFace *faceP;
grsTriangle *grsTriP;
tTriangle *triP;
int i, nFace = -1;
short nId = 0;

for (i = gameData.segs.nTris, grsTriP = TRIANGLES.Buffer (); i; i--, grsTriP++) {
	if (!(triP = AddTriangle (NULL, grsTriP->index, grsTriP))) {
		FreeData ();
		return 0;
		}
	if (nFace == grsTriP->nFace)
		nId++;
	else {
		nFace = grsTriP->nFace;
		nId = 0;
		}
	triP->nId = nId;
	faceP = FACES + grsTriP->nFace;
#if DBG
	if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (faceP->bSlide || (faceP->nCamera >= 0))
		triP->nPass = -2;
	}
return m_nTris = m_nTriangles;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitTriangleByEdge (int nTri, ushort nVert1, ushort nVert2, short nPass)
{
if (nTri < 0)
	return 1;

tTriangle *triP = &m_triangles [nTri];

if (triP->nPass < -1)
	return 1;

tFace *faceP = FACES + triP->nFace;

#if DBG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

	int			h, i, nIndex = triP->nIndex;
	ushort		nFace = triP->nFace, *indexP = triP->index, index [4];
	tTexCoord2f	texCoord [4], ovlTexCoord [4];
	tRgbaColorf	color [4];

// find insertion point for the new vertex
for (i = 0; i < 3; i++) {
	h = indexP [i];
	if ((h == nVert1) || (h == nVert2))
		break;
	}
if (i == 3)
	return 0;

h = indexP [(i + 1) % 3]; //check next vertex index
if ((h == nVert1) || (h == nVert2))
	h = (i + 1) % 3; //insert before i+1
else
	h = i; //insert before i

// build new quad index containing the new vertex
// the two triangle indices will be derived from it (indices 0,1,2 and 1,2,3)
index [0] = gameData.segs.nVertices - 1;
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
color [0].red = (color [1].red + color [3].red) / 2;
color [0].green = (color [1].green + color [3].green) / 2;
color [0].blue = (color [1].blue + color [3].blue) / 2;
color [0].alpha = (color [1].alpha + color [3].alpha) / 2;
DeleteTriangle (triP); //remove any references to this triangle
if (!(triP = CreateTriangle (triP, index, nFace, nIndex))) //create a new triangle at this location (insert)
	return 0;
triP->nPass = nPass;
triP->nId = (faceP->nTris)++;
memcpy (triP->color, color, sizeof (triP->color));
memcpy (triP->texCoord, texCoord, sizeof (triP->texCoord));
memcpy (triP->ovlTexCoord, ovlTexCoord, sizeof (triP->ovlTexCoord));

index [1] = index [0];
if (!(triP = CreateTriangle (NULL, index + 1, nFace, -1))) //create a new triangle (append)
	return 0;
triP->nPass = nPass;
triP->nId = (faceP->nTris)++;
triP->texCoord [0] = texCoord [0];
triP->ovlTexCoord [0] = ovlTexCoord [0];
triP->color [0] = color [0];
memcpy (triP->color + 1, color + 2, 2 * sizeof (triP->color [0]));
memcpy (triP->texCoord + 1, texCoord + 2, 2 * sizeof (triP->texCoord [0]));
memcpy (triP->ovlTexCoord + 1, ovlTexCoord + 2, 2 * sizeof (triP->ovlTexCoord [0]));
FACES [triP->nFace].nVerts++;
return 1;
}

//------------------------------------------------------------------------------

float CTriMeshBuilder::NewEdgeLen (int nTri, int nVert1, int nVert2)
{
	tTriangle	*triP = &m_triangles [nTri];
	int			h, i;

for (i = 0; i < 3; i++) {
	h = triP->index [i];
	if ((h != nVert1) && (h != nVert2))
		break;
	}
return CFloatVector::Dist(gameData.segs.fVertices [h], gameData.segs.fVertices [gameData.segs.nVertices]);
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitEdge (tEdge *edgeP, short nPass)
{
	int		tris [2];
	ushort	verts [2];

memcpy (tris, edgeP->tris, sizeof (tris));
memcpy (verts, edgeP->verts, sizeof (verts));
gameData.segs.fVertices [gameData.segs.nVertices] = CFloatVector::Avg(
			 gameData.segs.fVertices [verts [0]],
			 gameData.segs.fVertices [verts [1]]);
gameData.segs.vertices [gameData.segs.nVertices] = gameData.segs.fVertices [gameData.segs.nVertices].ToFix();
#if 0
if (tris [1] >= 0) {
	if (NewEdgeLen (tris [0], verts [0], verts [1]) + NewEdgeLen (tris [1], verts [0], verts [1]) < MAX_EDGE_LEN)
		return -1;
	}
#endif
gameData.segs.nVertices++;
if (!SplitTriangleByEdge (tris [0], verts [0], verts [1], nPass))
	return 0;
if (!SplitTriangleByEdge (tris [1], verts [0], verts [1], nPass))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitTriangle (tTriangle *triP, short nPass)
{
	int	h = 0, i;
	float	l, lMax = 0;

for (i = 0; i < 3; i++) {
	l = m_edges [triP->lines [i]].fLength;
	if (lMax < l) {
		lMax = l;
		h = i;
		}
	}
if (lMax <= MAX_EDGE_LEN)
	return -1;
return SplitEdge (&m_edges [triP->lines [h]], nPass);
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitTriangles (void)
{
	int	bSplit = 0, h, i, j, nSplitRes;
	short	nPass = 0, nMaxPasses = 10 * gameOpts->render.nMeshQuality;

h = 0;
do {
	bSplit = 0;
	j = m_nTriangles;
	PrintLog ("   splitting triangles (pass %d)\n", nPass);
	for (i = h, h = 0; i < j; i++) {
		if (m_triangles [i].nPass != nPass - 1)
			continue;
#if DBG
		tFace *faceP = FACES + m_triangles [i].nFace;
		if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
			nDbgSeg = nDbgSeg;
#endif
		nSplitRes = SplitTriangle (&m_triangles [i], nPass);
		if (gameData.segs.nVertices == 65536)
			return 0;
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
	} while (bSplit && (nPass < nMaxPasses));
return 1;
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::QSortTriangles (int left, int right)
{
	int	l = left,
			r = right,
			m = m_triangles [(l + r) / 2].nFace;
	short i = m_triangles [(l + r) / 2].nId;

do {
	while ((m_triangles [l].nFace < m) || ((m_triangles [l].nFace == m) && (m_triangles [l].nId < i)))
		l++;
	while ((m_triangles [r].nFace > m) || ((m_triangles [r].nFace == m) && (m_triangles [r].nId > i)))
		r--;
	if (l <= r) {
		if (l < r) {
			tTriangle h = m_triangles [l];
			m_triangles [l] = m_triangles [r];
			m_triangles [r] = h;
			}
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
	grsTriangle		*triP;
	g3sPoint			*pointP;
	int				h, i, nVertex;

for (i = gameData.segs.nVertices, pointP = gameData.segs.points.Buffer (); i; i--, pointP++) {
/*
	(*pointP->p3_normal.vNormal.V3()) [X] =
	(*pointP->p3_normal.vNormal.V3()) [Y] =
	(*pointP->p3_normal.vNormal.V3()) [Z] = 0;
*/
	pointP->p3_normal.vNormal.V3()->SetZero();

	pointP->p3_normal.nFaces = 0;
	}
for (h = 0, triP = gameData.segs.faces.tris.Buffer (); h < gameData.segs.nTris; h++, triP++) {
	for (i = 0; i < 3; i++) {
		nVertex = triP->index [i];
#if DBG
		if (nVertex == nDbgVertex)
			nVertex = nVertex;
#endif
		*gameData.segs.points [nVertex].p3_normal.vNormal.V3() += gameData.segs.faces.normals [3 * h];
		gameData.segs.points [nVertex].p3_normal.nFaces++;
		}
	}
ComputeVertexNormals ();
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::InsertTriangles (void)
{
	tTriangle	*triP = &m_triangles [0];
	grsTriangle	*grsTriP = TRIANGLES.Buffer ();
	tFace			*m_faceP = NULL;
	CFixVector	vNormal;
	int			h, i, nFace = -1;
	GLuint		nIndex = 0;

PrintLog ("   inserting new triangles\n");
QSortTriangles (0, m_nTriangles - 1);
ResetVertexNormals ();
for (h = 0; h < m_nTriangles; h++, triP++, grsTriP++) {
	grsTriP->nFace = triP->nFace;
	if (grsTriP->nFace == nFace)
		m_faceP->nTris++;
	else {
		if (m_faceP)
			m_faceP++;
		else
			m_faceP = FACES.Buffer ();
		nFace = grsTriP->nFace;
#if DBG
		if (m_faceP - FACES != nFace)
			return 0;
#endif
		m_faceP->nFrame = -1;
		m_faceP->nIndex = nIndex;
		m_faceP->nTriIndex = h;
		m_faceP->nTris = 1;
#if USE_RANGE_ELEMENTS
		m_faceP->vertIndex = gameData.segs.faces.vertIndex + nIndex;
#endif
		}
	grsTriP->nIndex = nIndex;
	memcpy (grsTriP->index, triP->index, sizeof (triP->index));
	for (i = 0; i < 3; i++)
		gameData.segs.faces.vertices [nIndex + i] = *gameData.segs.fVertices [triP->index [i]].V3();
	gameData.segs.faces.normals [nIndex] = CFixVector3::Normal(
					 gameData.segs.faces.vertices [nIndex],
					 gameData.segs.faces.vertices [nIndex + 1],
					 gameData.segs.faces.vertices [nIndex + 2]);
#if DBG
	if (gameData.segs.faces.normals [nIndex].Mag() == 0)
		m_faceP = m_faceP;
#endif
	vNormal = gameData.segs.faces.normals [nIndex].ToFix();
	for (i = 1; i < 3; i++)
		gameData.segs.faces.normals [nIndex + i] = gameData.segs.faces.normals [nIndex];
	memcpy (gameData.segs.faces.texCoord + nIndex, triP->texCoord, sizeof (triP->texCoord));
	memcpy (gameData.segs.faces.ovlTexCoord + nIndex, triP->ovlTexCoord, sizeof (triP->ovlTexCoord));
	memcpy (gameData.segs.faces.color + nIndex, triP->color, sizeof (triP->color));
#if USE_RANGE_ELEMENTS
	for (i = 0; i < 3; i++, nIndex++)
		gameData.segs.faces.vertIndex [nIndex] = nIndex;
#else
	nIndex += 3;
#endif
	}
gameData.segs.nTris = m_nTriangles;
SetupVertexNormals ();
FreeData ();
PrintLog ("   created %d new triangles and %d new vertices\n",
			 m_nTriangles - m_nTris, gameData.segs.nVertices - m_nVertices);
CreateFaceVertLists ();
return 1;
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::SortFaceVertList (ushort *vertList, int left, int right)
{
	int	l = left,
			r = right,
			m = vertList [(l + r) / 2];

do {
	while (vertList [l] < m)
		l++;
	while (vertList [r] > m)
		r--;
	if (l <= r) {
		if (l < r) {
			int h = vertList [l];
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

void CTriMeshBuilder::CreateFaceVertLists (void)
{
	int			*bTags = new int [gameData.segs.nVertices];
	tFace		*faceP;
	grsTriangle	*triP;
	int			h, i, j, k, nFace;

//count the vertices of each face
memset (bTags, 0xFF, gameData.segs.nVertices * sizeof (bTags [0]));
gameData.segs.nFaceVerts = 0;
for (i = gameData.segs.nFaces, faceP = FACES.Buffer (), nFace = 0; i; i--, faceP++, nFace++) {
	faceP->nVerts = 0;
	for (j = faceP->nTris, triP = TRIANGLES + faceP->nTriIndex; j; j--, triP++) {
		for (k = 0; k < 3; k++) {
			h = triP->index [k];
			if (bTags [h] != nFace) {
				bTags [h] = nFace;
				faceP->nVerts++;
				gameData.segs.nFaceVerts++;
				}
			}
		}
	}
//insert each face's vertices' indices in the vertex index buffer
memset (bTags, 0xFF, gameData.segs.nVertices * sizeof (bTags [0]));
gameData.segs.nFaceVerts = 0;
for (i = gameData.segs.nFaces, faceP = FACES.Buffer (), nFace = 0; i; i--, faceP++, nFace++) {
	faceP->triIndex = gameData.segs.faces.faceVerts + gameData.segs.nFaceVerts;
#if DBG
	if (faceP->nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	for (j = faceP->nTris, triP = TRIANGLES + faceP->nTriIndex; j; j--, triP++) {
		for (k = 0; k < 3; k++) {
			h = triP->index [k];
			if (bTags [h] != nFace) {
				bTags [h] = nFace;
				gameData.segs.faces.faceVerts [gameData.segs.nFaceVerts++] = h;
				}
			}
		}
	}
#if 1
//sort each face's vertex index list
for (i = gameData.segs.nFaces, faceP = FACES.Buffer (); i; i--, faceP++)
	SortFaceVertList (faceP->triIndex, 0, faceP->nVerts - 1);
#endif
}

//------------------------------------------------------------------------------

char *CTriMeshBuilder::DataFilename (char *pszFilename, int nLevel)
{
return GameDataFilename (pszFilename, "mesh", nLevel,
								 (gameStates.render.bTriangleMesh < 0) ? -1 : gameOpts->render.nMeshQuality);
}

//------------------------------------------------------------------------------

bool CTriMeshBuilder::Load (int nLevel)
{
	CFile					cf;
	tMeshDataHeader	mdh;
	int					nSize;
	bool					bOk;
	char					szFilename [FILENAME_LEN];
	char					*ioBuffer = NULL, *bufP;

if (!(gameStates.render.bTriangleMesh && gameStates.app.bCacheMeshes))
	return false;
if (!cf.Open (DataFilename (szFilename, nLevel), gameFolders.szCacheDir, "rb", 0))
	return false;
bOk = (cf.Read (&mdh, sizeof (mdh), 1) == 1);
if (bOk)
	bOk = (mdh.nVersion == MESH_DATA_VERSION) &&
			(mdh.nSegments == gameData.segs.nSegments) &&
			(mdh.nFaces == gameData.segs.nFaces);
if (bOk)
	nSize =
		(sizeof (gameData.segs.vertices [0]) + 
		 sizeof (gameData.segs.fVertices [0])) * mdh.nVertices +
		 sizeof (gameData.segs.faces.faces [0]) * mdh.nFaces +
		 sizeof (gameData.segs.faces.tris [0]) * mdh.nTris +
		(sizeof (gameData.segs.faces.vertices [0]) +
		 sizeof (gameData.segs.faces.normals [0]) +
		 sizeof (gameData.segs.faces.texCoord [0]) +
		 sizeof (gameData.segs.faces.ovlTexCoord [0]) +
		 sizeof (gameData.segs.faces.color [0])) * mdh.nTris * 3 +
		 sizeof (gameData.segs.faces.lMapTexCoord [0]) * mdh.nFaces * 2 +
		 sizeof (gameData.segs.faces.faceVerts [0]) * mdh.nFaceVerts;
if (bOk)
	bOk = ((ioBuffer = new char [nSize]) != NULL);
if (bOk)
	bOk = cf.Read (ioBuffer, nSize, 1) == 1;
if (bOk) {
	bufP = ioBuffer;
	memcpy (gameData.segs.vertices.Buffer (), bufP, sizeof (gameData.segs.vertices [0]) * mdh.nVertices);
	bufP += sizeof (gameData.segs.vertices [0]) * mdh.nVertices;
	memcpy (gameData.segs.fVertices.Buffer (), bufP, sizeof (gameData.segs.fVertices [0]) * mdh.nVertices);
	bufP += sizeof (gameData.segs.fVertices [0]) * mdh.nVertices;
	memcpy (gameData.segs.faces.faces.Buffer (), bufP, sizeof (gameData.segs.faces.faces [0]) * mdh.nFaces);
	bufP += sizeof (gameData.segs.faces.faces [0]) * mdh.nFaces;
	memcpy (gameData.segs.faces.tris.Buffer (), bufP, sizeof (gameData.segs.faces.tris [0]) * mdh.nTris);
	bufP += sizeof (gameData.segs.faces.tris [0]) * mdh.nTris;
	memcpy (gameData.segs.faces.vertices.Buffer (), bufP, sizeof (gameData.segs.faces.vertices [0]) * mdh.nTris * 3);
	bufP +=  sizeof (gameData.segs.faces.vertices [0]) * mdh.nTris * 3;
	memcpy (gameData.segs.faces.normals.Buffer (), bufP, sizeof (gameData.segs.faces.normals [0]) * mdh.nTris * 3);
	bufP += sizeof (gameData.segs.faces.normals [0]) * mdh.nTris * 3;
	memcpy (gameData.segs.faces.texCoord.Buffer (), bufP, sizeof (gameData.segs.faces.texCoord [0]) * mdh.nTris * 3);
	bufP += sizeof (gameData.segs.faces.texCoord [0]) * mdh.nTris * 3;
	memcpy (gameData.segs.faces.ovlTexCoord.Buffer (), bufP, sizeof (gameData.segs.faces.ovlTexCoord [0]) * mdh.nTris * 3);
	bufP += sizeof (gameData.segs.faces.ovlTexCoord [0]) * mdh.nTris * 3;
	memcpy (gameData.segs.faces.color.Buffer (), bufP, sizeof (gameData.segs.faces.color [0]) * mdh.nTris * 3);
	bufP += sizeof (gameData.segs.faces.color [0]) * mdh.nTris * 3;
	memcpy (gameData.segs.faces.lMapTexCoord.Buffer (), bufP, sizeof (gameData.segs.faces.lMapTexCoord [0]) * mdh.nFaces * 2);
	bufP += sizeof (gameData.segs.faces.lMapTexCoord [0]) * mdh.nFaces * 2;
	memcpy (gameData.segs.faces.faceVerts.Buffer (), bufP, sizeof (gameData.segs.faces.faceVerts [0]) * mdh.nFaceVerts);
	}
if (ioBuffer) {
	delete[] ioBuffer;
	ioBuffer = NULL;
	}
if (bOk) {
	gameData.segs.nVertices = mdh.nVertices;
	gameData.segs.nTris = mdh.nTris;
	SetupVertexNormals ();
	}
cf.Close ();
CreateFaceVertLists ();
return bOk;
}

//------------------------------------------------------------------------------

bool CTriMeshBuilder::Save (int nLevel)
{
	CFile					cf;
	bool					bOk;
	char					szFilename [FILENAME_LEN];

	tMeshDataHeader mdh = {MESH_DATA_VERSION,
								  gameData.segs.nSegments,
								  gameData.segs.nVertices,
								  gameData.segs.nFaceVerts,
								  gameData.segs.nFaces,
								  gameData.segs.nTris};

if (!(gameStates.render.bTriangleMesh && gameStates.app.bCacheMeshes))
	return 0;
if (!cf.Open (DataFilename (szFilename, nLevel), gameFolders.szCacheDir, "wb", 0))
	return 0;
bOk = (cf.Write (&mdh, sizeof (mdh), 1) == 1) &&
		(cf.Write (gameData.segs.vertices.Buffer (), sizeof (gameData.segs.vertices [0]) * mdh.nVertices, 1) == 1) &&
		(cf.Write (gameData.segs.fVertices.Buffer (), sizeof (gameData.segs.fVertices [0]) * mdh.nVertices, 1) == 1) &&
		(cf.Write (gameData.segs.faces.faces.Buffer (), sizeof (gameData.segs.faces.faces [0]) * mdh.nFaces, 1) == 1) &&
		(cf.Write (gameData.segs.faces.tris.Buffer (), sizeof (gameData.segs.faces.tris [0]) * mdh.nTris, 1) == 1) &&
		(cf.Write (gameData.segs.faces.vertices.Buffer (), sizeof (gameData.segs.faces.vertices [0]) * mdh.nTris, 3) == 3) &&
		(cf.Write (gameData.segs.faces.normals.Buffer (), sizeof (gameData.segs.faces.normals [0]) * mdh.nTris, 3) == 3) &&
		(cf.Write (gameData.segs.faces.texCoord.Buffer (), sizeof (gameData.segs.faces.texCoord [0]) * mdh.nTris, 3) == 3) &&
		(cf.Write (gameData.segs.faces.ovlTexCoord.Buffer (), sizeof (gameData.segs.faces.ovlTexCoord [0]) * mdh.nTris, 3) == 3) &&
		(cf.Write (gameData.segs.faces.color.Buffer (), sizeof (gameData.segs.faces.color [0]) * mdh.nTris, 3) == 3) &&
		(cf.Write (gameData.segs.faces.lMapTexCoord.Buffer (), sizeof (gameData.segs.faces.lMapTexCoord [0]) * mdh.nFaces, 2) == 2) &&
		(cf.Write (gameData.segs.faces.faceVerts.Buffer (), sizeof (gameData.segs.faces.faceVerts [0]) * mdh.nFaceVerts, 1) == 1);
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::Build (int nLevel)
{
PrintLog ("creating triangle mesh\n");
if (Load (nLevel))
	return 1;
if (!CreateTriangles ()) {
	gameData.segs.nVertices = m_nVertices;
	return 0;
	}
if ((gameStates.render.bTriangleMesh > 0) && !SplitTriangles ()) {
	gameData.segs.nVertices = m_nVertices;
	return 0;
	}
if (!InsertTriangles ()) {
	gameData.segs.nVertices = m_nVertices;
	return 0;
	}
Save (nLevel);
return 1;
}

//------------------------------------------------------------------------------

int CQuadMeshBuilder::IsBigFace (short *m_sideVerts)
{
for (int i = 0; i < 4; i++)
	if (CFloatVector::Dist(gameData.segs.fVertices [m_sideVerts [i]], gameData.segs.fVertices [m_sideVerts [(i + 1) % 4]]) > MAX_EDGE_LEN)
		return 1;
return 0;
}

//------------------------------------------------------------------------------

CFixVector3 *CQuadMeshBuilder::SetTriNormals (grsTriangle *triP, CFixVector3 *m_normalP)
{
	CFloatVector	vNormalf;

vNormalf = CFloatVector::Normal(gameData.segs.fVertices [triP->index [0]],
				 gameData.segs.fVertices [triP->index [1]], gameData.segs.fVertices [triP->index [2]]);
*m_normalP++ = *vNormalf.V3();
*m_normalP++ = *vNormalf.V3();
*m_normalP++ = *vNormalf.V3();
return m_normalP;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitFace (short nSegment, ubyte nSide, bool bRebuild)
{
	fix	rMin, rMax;

if (bRebuild)
	m_faceP->nTris = 0;
else
	memset (m_faceP, 0, sizeof (*m_faceP));
m_faceP->nSegment = nSegment;
m_faceP->nVerts = 4;
m_faceP->nFrame = -1;
m_faceP->nIndex = m_vertexP - gameData.segs.faces.vertices;
if (gameStates.render.bTriangleMesh)
	m_faceP->nTriIndex = m_triP - TRIANGLES;
memcpy (m_faceP->index, m_sideVerts, sizeof (m_faceP->index));
m_faceP->nType = gameStates.render.bTriangleMesh ? m_sideP->nType : -1;
m_faceP->nSegment = nSegment;
m_faceP->nSide = nSide;
m_faceP->nWall = gameStates.app.bD2XLevel ? m_nWall : IS_WALL (m_nWall) ? m_nWall : (ushort) -1;
m_faceP->bAnimation = IsAnimatedTexture (m_faceP->nBaseTex) || IsAnimatedTexture (m_faceP->nOvlTex);
m_faceP->bHasColor = 0;
ComputeSideRads (nSegment, nSide, &rMin, &rMax);
//float rMinf = X2F (rMin);
//float rMaxf = X2F (rMax);
m_faceP->fRads [0] = X2F (rMin); //(float) sqrt ((rMinf * rMinf + rMaxf * rMaxf) / 2);
m_faceP->fRads [1] = X2F (rMax); //(float) sqrt ((rMinf * rMinf + rMaxf * rMaxf) / 2);
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitTexturedFace (void)
{
m_faceP->nBaseTex = m_sideP->nBaseTex;
if ((m_faceP->nOvlTex = m_sideP->nOvlTex))
	m_nOvlTexCount++;
m_faceP->bSlide = (gameData.pig.tex.tMapInfoP [m_faceP->nBaseTex].slide_u || gameData.pig.tex.tMapInfoP [m_faceP->nBaseTex].slide_v);
m_faceP->nCamera = IsMonitorFace (m_faceP->nSegment, m_faceP->nSide, 1);
m_faceP->bIsLight = IsLight (m_faceP->nBaseTex) || (m_faceP->nOvlTex && IsLight (m_faceP->nOvlTex));
m_faceP->nOvlOrient = (ubyte) m_sideP->nOvlOrient;
m_faceP->bTextured = 1;
m_faceP->bTransparent = 0;
char *pszName = gameData.pig.tex.bitmapFiles [gameStates.app.bD1Mission][gameData.pig.tex.bmIndexP [m_faceP->nOvlTex ? m_faceP->nOvlTex : m_faceP->nBaseTex].index].name;
m_faceP->bSparks = (strstr (pszName, "misc17") != NULL);
if (m_nWallType < 2)
	m_faceP->bAdditive = 0;
else if (WALLS [m_nWall].flags & WALL_RENDER_ADDITIVE)
	m_faceP->bAdditive = 2;
else if (strstr (pszName, "lava"))
	m_faceP->bAdditive = 2;
else
	m_faceP->bAdditive = (strstr (pszName, "force") || m_faceP->bSparks) ? 1 : 0;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitColoredFace (short nSegment)
{
m_faceP->nBaseTex = -1;
m_faceP->bTransparent = 1;
m_faceP->bAdditive = gameData.segs.segment2s [nSegment].special >= SEGMENT_IS_LAVA;
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
int i = m_faceP->nLightmap % LIGHTMAP_BUFSIZE;
float x = (float) (i % LIGHTMAP_ROWSIZE);
float y = (float) (i / LIGHTMAP_ROWSIZE);
texCoordP [0].v.u = x / (float) LIGHTMAP_ROWSIZE + 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [0].v.v = y / (float) LIGHTMAP_ROWSIZE + 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [1].v.u =
texCoordP [2].v.u = (x + 1) / (float) LIGHTMAP_ROWSIZE - 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [1].v.v = y / (float) LIGHTMAP_ROWSIZE + 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [2].v.v = (y + 1) / (float) LIGHTMAP_ROWSIZE - 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [3].v.u = x / (float) LIGHTMAP_ROWSIZE + 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [3].v.v = (y + 1) / (float) LIGHTMAP_ROWSIZE - 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SetupFace (void)
{
	int			i, j;
	CFixVector	vNormal;
	CFixVector3		vNormalf;

vNormal = m_sideP->normals [0] + m_sideP->normals [1];
vNormal *= F1_0 / 2;
vNormalf = vNormal.ToFloat3();
for (i = 0; i < 4; i++) {
	j = m_sideVerts [i];
	*m_vertexP++ = *gameData.segs.fVertices [j].V3();
	*m_normalP++ = vNormalf;
	m_texCoordP->v.u = X2F (m_sideP->uvls [i].u);
	m_texCoordP->v.v = X2F (m_sideP->uvls [i].v);
	RotateTexCoord2f (*m_ovlTexCoordP, *m_texCoordP, (ubyte) m_sideP->nOvlOrient);
	m_texCoordP++;
	m_ovlTexCoordP++;
	if (!gameStates.app.bNostalgia)
		*m_faceColorP = gameData.render.color.ambient [j].color;
	else {
		m_faceColorP->red = 
		m_faceColorP->green = 
		m_faceColorP->blue = X2F (m_sideP->uvls [i].l);
		m_faceColorP->alpha = 1;
		}
	m_faceColorP++;
	}
SetupLMapTexCoord (m_lMapTexCoordP);
m_lMapTexCoordP += 4;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SplitIn2Tris (void)
{
	static short	n2TriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};

	int			h, i, j, k, v;
	short			*triVertP;
	tTexCoord2f	lMapTexCoord [4];

SetupLMapTexCoord (lMapTexCoord);
h = (m_sideP->nType == SIDE_IS_TRI_13);
for (i = 0; i < 2; i++, m_triP++) {
	gameData.segs.nTris++;
	m_faceP->nTris++;
	m_triP->nFace = m_faceP - FACES;
	m_triP->nIndex = m_vertexP - gameData.segs.faces.vertices;
	triVertP = n2TriVerts [h][i];
	for (j = 0; j < 3; j++) {
		k = triVertP [j];
		v = m_sideVerts [k];
		m_triP->index [j] = v;
		*m_vertexP++ = *gameData.segs.fVertices [v].V3();
		m_texCoordP->v.u = X2F (m_sideP->uvls [k].u);
		m_texCoordP->v.v = X2F (m_sideP->uvls [k].v);
		RotateTexCoord2f (*m_ovlTexCoordP, *m_texCoordP, (ubyte) m_sideP->nOvlOrient);
		*m_lMapTexCoordP = lMapTexCoord [k];
		m_texCoordP++;
		m_ovlTexCoordP++;
		m_lMapTexCoordP++;
		m_colorP = gameData.render.color.ambient + v;
		*m_faceColorP++ = m_colorP->color;
		}
	m_normalP = SetTriNormals (m_triP, m_normalP);
	}
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::BuildSlidingFaceList (void)
{
	tFace	*faceP = gameData.segs.faces.faces.Buffer ();

gameData.segs.faces.slidingFaces = NULL;
for (int i = gameData.segs.nFaces; i; i--, faceP++)
	if (faceP->bSlide) {
		faceP->nextSlidingFace = gameData.segs.faces.slidingFaces;
		gameData.segs.faces.slidingFaces = faceP;
		}
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::RebuildLightmapTexCoord (void)
{
	static short	n2TriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};

	int			h, i, j, k, nFace;
	short			*triVertP;
	tTexCoord2f	lMapTexCoord [4];

m_faceP = FACES.Buffer ();
m_lMapTexCoordP = gameData.segs.faces.lMapTexCoord.Buffer ();
for (nFace = gameData.segs.nFaces; nFace; nFace--, m_faceP++) {
	SetupLMapTexCoord (lMapTexCoord);
	h = (SEGMENTS [m_faceP->nSegment].sides [m_faceP->nSide].nType == SIDE_IS_TRI_13);
	for (i = 0; i < 2; i++, m_triP++) {
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
	static short	n4TriVerts [4][3] = {{0,1,4},{1,2,4},{2,3,4},{3,0,4}};

	CFloatVector	vSide [4];
	tRgbaColorf		color;
	tTexCoord2f		texCoord;
	short				*triVertP;
	int				h, i, j, k, v;

texCoord.v.u = texCoord.v.v = 0;
color.red = color.green = color.blue = color.alpha = 0;
for (i = 0; i < 4; i++) {
	j = (i + 1) % 4;
	texCoord.v.u += X2F (m_sideP->uvls [i].u + m_sideP->uvls [j].u) / 8;
	texCoord.v.v += X2F (m_sideP->uvls [i].v + m_sideP->uvls [j].v) / 8;
	h = m_sideVerts [i];
	k = m_sideVerts [j];
	color.red += (gameData.render.color.ambient [h].color.red + gameData.render.color.ambient [k].color.red) / 8;
	color.green += (gameData.render.color.ambient [h].color.green + gameData.render.color.ambient [k].color.green) / 8;
	color.blue += (gameData.render.color.ambient [h].color.blue + gameData.render.color.ambient [k].color.blue) / 8;
	color.alpha += (gameData.render.color.ambient [h].color.alpha + gameData.render.color.ambient [k].color.alpha) / 8;
	}
vSide [0] = CFloatVector::Avg (gameData.segs.fVertices [m_sideVerts [0]], gameData.segs.fVertices [m_sideVerts [1]]);
vSide [2] = CFloatVector::Avg (gameData.segs.fVertices [m_sideVerts [2]], gameData.segs.fVertices [m_sideVerts [3]]);
vSide [1] = CFloatVector::Avg (gameData.segs.fVertices [m_sideVerts [1]], gameData.segs.fVertices [m_sideVerts [2]]);
vSide [3] = CFloatVector::Avg (gameData.segs.fVertices [m_sideVerts [3]], gameData.segs.fVertices [m_sideVerts [0]]);

VmLineLineIntersection (vSide [0], vSide [2], vSide [1], vSide [3],
								gameData.segs.fVertices [gameData.segs.nVertices],
								gameData.segs.fVertices [gameData.segs.nVertices]);
gameData.segs.vertices [gameData.segs.nVertices] = gameData.segs.fVertices [gameData.segs.nVertices].ToFix();
m_sideVerts [4] = gameData.segs.nVertices++;
m_faceP->nVerts++;
for (i = 0; i < 4; i++, m_triP++) {
	gameData.segs.nTris++;
	m_faceP->nTris++;
	m_triP->nFace = m_faceP - FACES;
	m_triP->nIndex = m_vertexP - gameData.segs.faces.vertices;
	triVertP = n4TriVerts [i];
	for (j = 0; j < 3; j++) {
		k = triVertP [j];
		v = m_sideVerts [k];
		m_triP->index [j] = v;
		*m_vertexP++ = *gameData.segs.fVertices [v].V3();
		if (j == 2) {
			m_texCoordP [2] = texCoord;
			m_faceColorP [2] = color;
			}
		else {
			m_texCoordP [j].v.u = X2F (m_sideP->uvls [k].u);
			m_texCoordP [j].v.v = X2F (m_sideP->uvls [k].v);
			m_colorP = gameData.render.color.ambient + v;
			m_faceColorP [j] = m_colorP->color;
			}
		RotateTexCoord2f (*m_ovlTexCoordP, m_texCoordP [j], (ubyte) m_sideP->nOvlOrient);
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
if (!gameStates.ogl.bHaveVBOs)
	return false;
DestroyVBOs ();
int h, i;
glGenBuffersARB (1, &gameData.segs.faces.vboDataHandle);
if ((i = glGetError ())) {
	glGenBuffersARB (1, &gameData.segs.faces.vboDataHandle);
	if ((i = glGetError ())) {
#	if DBG
		HUDMessage (0, "glGenBuffersARB failed (%d)", i);
#	endif
		gameStates.ogl.bHaveVBOs = 0;
		return false;
		}
	}
glBindBufferARB (GL_ARRAY_BUFFER_ARB, gameData.segs.faces.vboDataHandle);
if ((i = glGetError ())) {
#	if DBG
	HUDMessage (0, "glBindBufferARB failed (%d)", i);
#	endif
	gameStates.ogl.bHaveVBOs = 0;
	return false;
	}
gameData.segs.faces.nVertices = gameStates.render.bTriangleMesh ? gameData.segs.nTris * 3 : gameData.segs.nFaces * 4;
glBufferDataARB (GL_ARRAY_BUFFER, gameData.segs.faces.nVertices * sizeof (tFaceRenderVertex), NULL, GL_STATIC_DRAW_ARB));
gameData.segs.faces.vertexP = reinterpret_cast<ubyte*> (glMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB));
gameData.segs.faces.vboIndexHandle = 0;
glGenBuffersARB (1, &gameData.segs.faces.vboIndexHandle);
if (gameData.segs.faces.vboIndexHandle) {
	glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, gameData.segs.faces.vboIndexHandle);
	glBufferDataARB (GL_ELEMENT_ARRAY_BUFFER_ARB, gameData.segs.faces.nVertices * sizeof (ushort), NULL, GL_STATIC_DRAW_ARB);
	gameData.segs.faces.indexP = reinterpret_cast<ushort*> (glMapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	}
else
	gameData.segs.faces.indexP = NULL;

ubyte *vertexP = gameData.segs.faces.vertexP;
i = 0;
gameData.segs.faces.iVertices = 0;
memcpy (vertexP, gameData.segs.faces.vertices, h = gameData.segs.faces.nVertices * sizeof (gameData.segs.faces.vertices [0]));
i += h;
gameData.segs.faces.iNormals = i;
memcpy (vertexP + i, gameData.segs.faces.normals, h = gameData.segs.faces.nVertices * sizeof (gameData.segs.faces.normals [0]));
i += h;
gameData.segs.faces.iColor = i;
memcpy (vertexP + i, gameData.segs.faces.color, h = gameData.segs.faces.nVertices * sizeof (gameData.segs.faces.color [0]));
i += h;
gameData.segs.faces.iTexCoord = i;
memcpy (vertexP + i, gameData.segs.faces.texCoord, h = gameData.segs.faces.nVertices * sizeof (gameData.segs.faces.texCoord [0]));
i += h;
gameData.segs.faces.iOvlTexCoord = i;
memcpy (vertexP + i, gameData.segs.faces.ovlTexCoord, h = gameData.segs.faces.nVertices * sizeof (gameData.segs.faces.ovlTexCoord [0]));
i += h;
gameData.segs.faces.iLMapTexCoord = i;
memcpy (vertexP + i, gameData.segs.faces.lMapTexCoord, h = gameData.segs.faces.nVertices * sizeof (gameData.segs.faces.lMapTexCoord [0]));
for (int i = 0; i < gameData.segs.faces.nVertices; i++)
	gameData.segs.faces.indexP [i] = i;

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
if (gameData.segs.faces.vboDataHandle) {
	glDeleteBuffersARB (1, &gameData.segs.faces.vboDataHandle);
	gameData.segs.faces.vboDataHandle = 0;
	}
if (gameData.segs.faces.vboIndexHandle) {
	glDeleteBuffersARB (1, &gameData.segs.faces.vboIndexHandle);
	gameData.segs.faces.vboIndexHandle = 0;
	}
#endif
}

//------------------------------------------------------------------------------

#define FACE_VERTS	6

int CQuadMeshBuilder::Build (int nLevel, bool bRebuild)
{
m_faceP = FACES.Buffer ();
m_triP = TRIANGLES.Buffer ();
m_vertexP = gameData.segs.faces.vertices.Buffer ();
m_normalP = gameData.segs.faces.normals.Buffer ();
m_texCoordP = gameData.segs.faces.texCoord.Buffer ();
m_ovlTexCoordP = gameData.segs.faces.ovlTexCoord.Buffer ();
m_lMapTexCoordP = gameData.segs.faces.lMapTexCoord.Buffer ();
m_faceColorP = gameData.segs.faces.color.Buffer ();
m_colorP = gameData.render.color.ambient.Buffer ();
m_segP = SEGMENTS.Buffer ();
m_segFaceP = SEGFACES.Buffer ();
gameData.segs.faces.slidingFaces = NULL;

	short			nSegment, i;
	ubyte			nSide;

#if !DBG
if (gameOpts->render.nMeshQuality > 2)
	gameOpts->render.nMeshQuality = 2;
#endif
if (RENDERPATH && gameOpts->render.nLightingMethod)
	gameStates.render.bTriangleMesh = gameStates.render.bPerPixelLighting ? -1 : gameStates.render.nMeshQuality;
else
	gameStates.render.bTriangleMesh = 0;
gameStates.render.nFacePrimitive = gameStates.render.bTriangleMesh ? GL_TRIANGLES : GL_TRIANGLE_FAN;
if (gameStates.render.bSplitPolys)
	gameStates.render.bSplitPolys = (gameStates.render.bPerPixelLighting || !gameOpts->render.nMeshQuality) ? 1 : -1;
if (gameStates.render.bTriangleMesh)
	cameraManager.Create ();
PrintLog ("   Creating face list\n");
gameData.segs.nFaces = 0;
gameData.segs.nTris = 0;
for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, m_segP++, m_segFaceP++) {
	m_bColoredSeg = ((gameData.segs.segment2s [nSegment].special >= SEGMENT_IS_WATER) &&
					     (gameData.segs.segment2s [nSegment].special <= SEGMENT_IS_TEAM_RED)) ||
					     (gameData.segs.xSegments [nSegment].group >= 0);
#if DBG
	if (nSegment == nDbgSeg)
		m_faceP = m_faceP;
#endif
	m_faceP->nSegment = nSegment;
	m_nOvlTexCount = 0;
	m_segFaceP->nFaces = 0;
	for (nSide = 0, m_sideP = m_segP->sides; nSide < 6; nSide++, m_sideP++) {
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			nDbgSeg = nDbgSeg;
#endif
		m_nWall = WallNumI (nSegment, nSide);
		m_nWallType = IS_WALL (m_nWall) ? WallIsInvisible (m_nWall) ? 0 : 2 : (m_segP->children [nSide] == -1) ? 1 : 0;
		if (m_bColoredSeg || m_nWallType) {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#endif
			GetSideVertIndex (m_sideVerts, nSegment, nSide);
			InitFace (nSegment, nSide, bRebuild);
			if (m_nWallType)
				InitTexturedFace ();
			else if (m_bColoredSeg)
				InitColoredFace (nSegment);
			if (gameStates.render.bTriangleMesh) {
				// split in four triangles, using the quad's center of gravity as additional vertex
				if (!gameStates.render.bPerPixelLighting && (m_sideP->nType == SIDE_IS_QUAD) &&
					 !m_faceP->bSlide && (m_faceP->nCamera < 0) && IsBigFace (m_sideVerts))
					SplitIn4Tris ();
				else // split in two triangles, regarding any non-planarity
					SplitIn2Tris ();
				}
			else
				SetupFace ();
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				m_faceP = m_faceP;
#endif
			if (!m_segFaceP->nFaces++)
				m_segFaceP->pFaces = m_faceP;
			m_faceP++;
			gameData.segs.nFaces++;
			}
		else {
			m_colorP += FACE_VERTS;
			}
		}
	if (!(gameStates.render.bTriangleMesh || gameStates.render.bPerPixelLighting) &&
		 gameStates.ogl.bGlTexMerge && m_nOvlTexCount) { //allow for splitting multi-textured faces into two single textured ones
		gameData.segs.nFaces += m_nOvlTexCount;
		m_faceP += m_nOvlTexCount;
		m_triP += 2;
		m_vertexP += m_nOvlTexCount * FACE_VERTS;
		m_normalP += m_nOvlTexCount * FACE_VERTS;
		m_texCoordP += m_nOvlTexCount * FACE_VERTS;
		m_ovlTexCoordP += m_nOvlTexCount * FACE_VERTS;
		m_faceColorP += m_nOvlTexCount * FACE_VERTS;
		}
	}
for (m_colorP = gameData.render.color.ambient.Buffer (), i = gameData.segs.nVertices; i; i--, m_colorP++)
	if (m_colorP->color.alpha > 1) {
		m_colorP->color.red /= m_colorP->color.alpha;
		m_colorP->color.green /= m_colorP->color.alpha;
		m_colorP->color.blue /= m_colorP->color.alpha;
		m_colorP->color.alpha = 1;
		}
if (gameStates.render.bTriangleMesh && !m_triMeshBuilder.Build (nLevel)) {
	gameStates.render.nMeshQuality = 0;
	return 0;
	}
BuildSlidingFaceList ();
if (gameStates.render.bTriangleMesh)
	cameraManager.Destroy ();
gameStates.render.nMeshQuality = gameOpts->render.nMeshQuality;
return 1;
}

//------------------------------------------------------------------------------
//eof
