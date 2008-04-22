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

#define LIGHT_VERSION 4

#ifdef RCS
char rcsid [] = "$Id: gamemine.c, v 1.26 2003/10/22 15:00:37 schaffner Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"

#include "inferno.h"
#include "text.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "error.h"
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

CFaceMeshBuilder faceMeshBuilder;

float fMaxSideLen [] = {1e30f, 40, 30, 20, 10};

#define	MAX_SIDE_LEN	fMaxSideLen [gameOpts->render.nMeshQuality]

//------------------------------------------------------------------------------

void CTriMeshBuilder::FreeMeshData (void)
{
D2_FREE (m_meshTris);
D2_FREE (m_meshLines);
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::AllocMeshData (void)
{
if (m_nMaxMeshTris && m_nMaxMeshLines) {
	if (!((m_meshLines = (tMeshLine *) D2_REALLOC (m_meshLines, 2 * m_nMaxMeshTris * sizeof (tMeshLine))) &&
			(m_meshTris = (tMeshTri *) D2_REALLOC (m_meshTris, 2 * m_nMaxMeshTris * sizeof (tMeshTri))))) {
		FreeMeshData ();
		return 0;
		}
	memset (m_meshLines + m_nMaxMeshTris, 0xff, m_nMaxMeshTris * sizeof (tMeshLine));
	memset (m_meshTris + m_nMaxMeshLines, 0xff, m_nMaxMeshTris * sizeof (tMeshTri));
	m_nMaxMeshTris *= 2;
	m_nMaxMeshLines *= 2;
	}
else {
	m_nMaxMeshTris = gameData.segs.nFaces * 4;
	m_nMaxMeshLines = gameData.segs.nFaces * 4;
	if (!(m_meshLines = (tMeshLine *) D2_ALLOC (m_nMaxMeshLines * sizeof (tMeshLine))))
		return 0;
	if (!(m_meshTris = (tMeshTri *) D2_ALLOC (m_nMaxMeshTris * sizeof (tMeshTri)))) {
		FreeMeshData ();
		return 0;
		}
	memset (m_meshLines, 0xff, m_nMaxMeshTris * sizeof (tMeshLine));
	memset (m_meshTris, 0xff, m_nMaxMeshTris * sizeof (tMeshTri));
	}
return 1;
}

//------------------------------------------------------------------------------

tMeshLine *CTriMeshBuilder::FindMeshLine (ushort nVert1, ushort nVert2)
{
	tMeshLine	*mlP = m_meshLines;

for (int i = m_nMeshLines; i; i--, mlP++)
	if ((mlP->verts [0] == nVert1) && (mlP->verts [1] == nVert2))
		return mlP;
return NULL;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::AddMeshLine (int nTri, ushort nVert1, ushort nVert2)
{
if (nVert2 < nVert1) {
	ushort h = nVert1;
	nVert1 = nVert2;
	nVert2 = h;
	}
#ifdef _DEBUG
if ((nTri < 0) || (nTri >= m_nMeshTris))
	return -1;
#endif
tMeshLine *mlP = FindMeshLine (nVert1, nVert2);
if (mlP) {
	if (mlP->tris [0] < 0)
		mlP->tris [0] = nTri;
	else
		mlP->tris [1] = nTri;
	return mlP - m_meshLines;
	}
if (m_nFreeLines) {
	mlP = m_meshLines + m_nFreeLines;
	m_nFreeLines = mlP->nNext;
	mlP->nNext = -1;
	}
else {
	if ((m_nMeshLines == m_nMaxMeshLines - 1) && !AllocMeshData ())
		return -1;
	mlP = m_meshLines + m_nMeshLines;
	}
#ifdef _DEBUG
if (m_nMeshLines == 70939)
	mlP = mlP;
#endif
mlP->tris [0] = nTri;
mlP->verts [0] = nVert1;
mlP->verts [1] = nVert2;
return m_nMeshLines++;
}

//------------------------------------------------------------------------------

tMeshTri *CTriMeshBuilder::CreateMeshTri (tMeshTri *mtP, ushort index [], int nFace, int nIndex)
{
	int	h, i;

if (mtP) 
	mtP->nIndex = nIndex;
else {
	if ((m_nMeshTris == m_nMaxMeshTris - 1) && !AllocMeshData ())
		return NULL;
	mtP = m_meshTris + m_nMeshTris++;
	mtP->nIndex = nIndex;
	if (nIndex >= 0) {
		i = gameData.segs.faces.tris [nIndex].nIndex;
		memcpy (mtP->texCoord, gameData.segs.faces.texCoord + i, sizeof (mtP->texCoord));
		memcpy (mtP->ovlTexCoord, gameData.segs.faces.ovlTexCoord + i, sizeof (mtP->ovlTexCoord));
		memcpy (mtP->color, gameData.segs.faces.color + i, sizeof (mtP->color));
		}
	}
nIndex = mtP - m_meshTris;
for (i = 0; i < 3; i++) {
	if (0 > (h = AddMeshLine (nIndex, index [i], index [(i + 1) % 3])))
		return NULL;
	mtP = m_meshTris + nIndex;
	mtP->lines [i] = h;
	}
mtP->nFace = nFace;
memcpy (mtP->index, index, sizeof (mtP->index));
return mtP;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::AddMeshTri (tMeshTri *mtP, ushort index [], grsTriangle *triP)
{
#if 1
return CreateMeshTri (mtP, index, triP->nFace, triP - gameData.segs.faces.tris) ? m_nMeshTris : 0;
#else
	int		h, i;
	float		l;
	
for (h = i = 0; i < 3; i++)
	if ((l = VmVecDist ((fVector *) (gameData.segs.fVertices + index [i]), 
								(fVector *) (gameData.segs.fVertices + index [(i + 1) % 3]))) > MAX_SIDE_LEN)
		return CreateMeshTri (mtP, index, triP->nFace, triP - gameData.segs.faces.tris) ? m_nMeshTris : 0;
return m_nMeshTris;
#endif
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::DeleteMeshLine (tMeshLine *mlP)
{
#if 1
mlP->nNext = m_nFreeLines;
m_nFreeLines = mlP - m_meshLines;
#else
	tMeshTri	*mtP;
	int		h = mlP - m_meshLines, i, j;

if (h < --m_nMeshLines) {
	*mlP = m_meshLines [m_nMeshLines];
	for (i = 0; i < 2; i++) {
		if (mlP->tris [i] >= 0) {
			mtP = m_meshTris + mlP->tris [i];
			for (j = 0; j < 3; j++)
				if (mtP->lines [j] == m_nMeshLines)
					mtP->lines [j] = h;
			}
		}
	}
#endif
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::DeleteMeshTri (tMeshTri *mtP)
{
	tMeshLine	*mlP;
	int			i, nTri = mtP - m_meshTris;

for (i = 0; i < 3; i++) {
	mlP = m_meshLines + mtP->lines [i];
	if (mlP->tris [0] == nTri)
		mlP->tris [0] = mlP->tris [1];
#ifdef _DEBUG
	if (mlP - m_meshLines == 70939)
		mlP = mlP;
#endif
	mlP->tris [1] = -1;
	if (mlP->tris [0] < 0)
		DeleteMeshLine (mlP);
	}
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::CreateMeshTris (void)
{
PrintLog ("   adding existing triangles\n");
m_nMeshLines = 0;
m_nMeshTris = 0;
m_nMaxMeshTris = 0;
m_nMaxMeshTris = 0;
m_nFreeLines = 0;
m_nVertices = gameData.segs.nVertices;
if (!AllocMeshData ())
	return 0;

grsTriangle *triP;
int i;

for (i = gameData.segs.nTris, triP = gameData.segs.faces.tris; i; i--, triP++)
	if (!AddMeshTri (NULL, triP->index, triP)) {
		FreeMeshData ();
		return 0;
		}
return m_nMeshTris;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitMeshTriByLine (int nTri, ushort nVert1, ushort nVert2, ushort nPass)
{
if (nTri < 0)
	return 1;

	tMeshTri		*mtP = m_meshTris + nTri;
	int			h, i, nIndex = mtP->nIndex;
	ushort		nFace = mtP->nFace, *indexP = mtP->index, index [4];
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
index [0] = gameData.segs.nVertices;
for (i = 1; i < 4; i++) {
	index [i] = indexP [h];
	texCoord [i] = mtP->texCoord [h];
	ovlTexCoord [i] = mtP->ovlTexCoord [h];
	color [i] = mtP->color [h++];
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
#if 0//def _DEBUG
if (VmLinePointDist (gameData.segs.fVertices + index [1], gameData.segs.fVertices + index [2], gameData.segs.fVertices + index [0], 0) < 1)
	return 0;
if (VmLinePointDist (gameData.segs.fVertices + index [2], gameData.segs.fVertices + index [3], gameData.segs.fVertices + index [0], 0) < 1)
	return 0;
if (VmLinePointDist (gameData.segs.fVertices + index [3], gameData.segs.fVertices + index [4], gameData.segs.fVertices + index [0], 0) < 1)
	return 0;
#endif
DeleteMeshTri (mtP); //remove any references to this triangle
if (!(mtP = CreateMeshTri (mtP, index, nFace, nIndex))) //create a new triangle at this location (insert)
	return 0;
mtP->nPass = nPass;
memcpy (mtP->color, color, sizeof (mtP->color));
memcpy (mtP->texCoord, texCoord, sizeof (mtP->texCoord));
memcpy (mtP->ovlTexCoord, ovlTexCoord, sizeof (mtP->ovlTexCoord));

index [1] = index [0];
if (!(mtP = CreateMeshTri (NULL, index + 1, nFace, -1))) //create a new triangle (append)
	return 0;
mtP->nPass = nPass;
mtP->texCoord [0] = texCoord [0];
mtP->ovlTexCoord [0] = ovlTexCoord [0];
mtP->color [0] = color [0];
memcpy (mtP->color + 1, color + 2, 2 * sizeof (mtP->color [0]));
memcpy (mtP->texCoord + 1, texCoord + 2, 2 * sizeof (mtP->texCoord [0]));
memcpy (mtP->ovlTexCoord + 1, ovlTexCoord + 2, 2 * sizeof (mtP->ovlTexCoord [0]));
return 1;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitMeshLine (tMeshLine *mlP, ushort nPass)
{
	int		i = 0;
	int		tris [2];
	ushort	verts [2];

memcpy (tris, mlP->tris, sizeof (tris));
memcpy (verts, mlP->verts, sizeof (verts));
VmVecAvg (gameData.segs.fVertices + gameData.segs.nVertices, 
			  gameData.segs.fVertices + verts [0], 
			  gameData.segs.fVertices + verts [1]);
if (!SplitMeshTriByLine (tris [0], verts [0], verts [1], nPass))
	return 0;
if (tris [1] < 0)
	m_nMeshTris = m_nMeshTris;
if (!SplitMeshTriByLine (tris [1], verts [0], verts [1], nPass))
	return 0;
gameData.segs.faces.faces [m_meshTris [tris [0]].nFace].nVerts++;
if ((tris [0] != tris [1]) && (tris [1] >= 0))
	gameData.segs.faces.faces [m_meshTris [tris [1]].nFace].nVerts++;
VmVecFloatToFix (gameData.segs.vertices + gameData.segs.nVertices, gameData.segs.fVertices + gameData.segs.nVertices);
gameData.segs.nVertices++;
return 1;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitMeshTri (tMeshTri *mtP, ushort nPass)
{
	tMeshLine	*mlP;
	int			h, i;
	float			l, lMax = 0;

for (i = 0; i < 3; i++) {
	mlP = m_meshLines + mtP->lines [i];
	l = VmVecDist (gameData.segs.fVertices + mlP->verts [0], gameData.segs.fVertices + mlP->verts [1]);
	if (lMax < l) {
		lMax = l;
		h = i;
		}
	}
if (lMax <= MAX_SIDE_LEN)
	return -1;
return SplitMeshLine (m_meshLines + mtP->lines [h], nPass);
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitMeshTris (void)
{
	int		bSplit = 0, h, i, j, nSplitRes;
	ushort	nPass = 0;

h = 0;
do {
	bSplit = 0;
	j = m_nMeshTris;
	PrintLog ("   splitting triangles (pass %d)\n", nPass);
	for (i = h, h = 0; i < j; i++) {
		if (m_meshTris [i].nPass != (ushort) (nPass - 1))
			continue;
#ifdef _DEBUG
		if (i == 48936)
			i = i;
#endif
		nSplitRes = SplitMeshTri (m_meshTris + i, nPass);
		if (gameData.segs.nVertices == 65536)
			return 1;
		if (!nSplitRes)
			return 0;
		if (nSplitRes > 0) {
			bSplit = 1;
			if (!h)
				h = i;
			}
		}
	nPass++;
	} while (bSplit);
return 1;
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::QSortMeshTris (int left, int right)
{
	int	l = left,
			r = right,
			m = m_meshTris [(l + r) / 2].nFace;

do {
	while (m_meshTris [l].nFace < m)
		l++;
	while (m_meshTris [r].nFace > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tMeshTri h = m_meshTris [l];
			m_meshTris [l] = m_meshTris [r];
			m_meshTris [r] = h;
			}
		l++;
		r--;
		}	
	} while (l <= r);
if (l < right)
	QSortMeshTris (l, right);
if (left < r)
	QSortMeshTris (left, r);
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::InsertMeshTris (void)
{
	tMeshTri		*mtP = m_meshTris;
	grsTriangle	*triP = gameData.segs.faces.tris;
	grsFace		*m_faceP = NULL;
	vmsVector	vNormal;
	int			h, i, nVertex, nIndex = 0, nTriVertIndex = 0, nFace = -1;

PrintLog ("   inserting new triangles\n");
QSortMeshTris (0, m_nMeshTris - 1);
ResetVertexNormals ();
for (h = 0; h < m_nMeshTris; h++, mtP++, triP++) {
	triP->nFace = mtP->nFace;
	if (triP->nFace == nFace) 
		m_faceP->nTris++;
	else {
		if (m_faceP)
			m_faceP++;
		else
			m_faceP = gameData.segs.faces.faces;
		nFace = triP->nFace;
		if (m_faceP - gameData.segs.faces.faces != nFace)
			return 0;
		m_faceP->nIndex = nIndex;
		m_faceP->nTriIndex = h;
		m_faceP->nTris = 1;
		m_faceP->vertIndex = gameData.segs.faces.vertIndex + nIndex;
		}
	triP->nIndex = nIndex;
#ifdef _DEBUG
	memcpy (triP->index, mtP->index, sizeof (triP->index));
#endif
	for (i = 0; i < 3; i++)
		gameData.segs.faces.vertices [nIndex + i] = gameData.segs.fVertices [mtP->index [i]].v3;
	VmVecNormal (gameData.segs.faces.normals + nIndex,
					 gameData.segs.faces.vertices + nIndex, 
					 gameData.segs.faces.vertices + nIndex + 1, 
					 gameData.segs.faces.vertices + nIndex + 2);
#ifdef _DEBUG
	if (VmVecMag (gameData.segs.faces.normals + nIndex) == 0)
		m_faceP = m_faceP;
#endif
	VmVecFloatToFix (&vNormal, gameData.segs.faces.normals + nIndex);
	for (i = 1; i < 3; i++)
		gameData.segs.faces.normals [nIndex + i] = gameData.segs.faces.normals [nIndex];
	for (i = 0; i < 3; i++) {
		nVertex = mtP->index [i];
#ifdef _DEBUG
		if (nVertex == nDbgVertex)
			nVertex = nVertex;
#endif
		VmVecInc (&gameData.segs.points [nVertex].p3_normal.vNormal.v3, gameData.segs.faces.normals + nIndex);
		gameData.segs.points [nVertex].p3_normal.nFaces++;
		}
	memcpy (gameData.segs.faces.texCoord + nIndex, mtP->texCoord, sizeof (mtP->texCoord));
	memcpy (gameData.segs.faces.ovlTexCoord + nIndex, mtP->ovlTexCoord, sizeof (mtP->ovlTexCoord));
	memcpy (gameData.segs.faces.color + nIndex, mtP->color, sizeof (mtP->color));
	gameData.segs.faces.vertIndex [nIndex] = nIndex++;
	gameData.segs.faces.vertIndex [nIndex] = nIndex++;
	gameData.segs.faces.vertIndex [nIndex] = nIndex++;
	}
ComputeVertexNormals ();
FreeMeshData ();
PrintLog ("   created %d new triangles and %d new vertices\n", 
			 m_nMeshTris - gameData.segs.nTris, gameData.segs.nVertices - m_nVertices);
gameData.segs.nTris = m_nMeshTris;
return 1;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::BuildMesh (void)
{
PrintLog ("creating triangle mesh\n");
if (!CreateMeshTris ())
	return 0;
if (!SplitMeshTris ())
	return 0;
return InsertMeshTris ();
}

//------------------------------------------------------------------------------

int CFaceMeshBuilder::IsBigFace (short *m_sideVerts)
{
for (int i = 0; i < 4; i++) 
	if (VmVecDist (gameData.segs.fVertices + m_sideVerts [i], gameData.segs.fVertices + m_sideVerts [(i + 1) % 4]) > MAX_SIDE_LEN)
		return 1;
return 0;
}

//------------------------------------------------------------------------------

fVector3 *CFaceMeshBuilder::SetTriNormals (grsTriangle *triP, fVector3 *m_normalP)
{
	fVector	vNormalf;

VmVecNormal (&vNormalf, gameData.segs.fVertices + triP->index [0], 
				 gameData.segs.fVertices + triP->index [1], gameData.segs.fVertices + triP->index [2]);
*m_normalP++ = vNormalf.v3;
*m_normalP++ = vNormalf.v3;
*m_normalP++ = vNormalf.v3;
return m_normalP;
}

//------------------------------------------------------------------------------

void CFaceMeshBuilder::InitFace (short nSegment, ubyte nSide)
{
memset (m_faceP, 0, sizeof (*m_faceP));
m_faceP->nSegment = nSegment;
m_faceP->nVerts = 4;
m_faceP->nIndex = m_vertexP - gameData.segs.faces.vertices;
if (gameStates.render.bTriangleMesh)
	m_faceP->nTriIndex = m_triP - gameData.segs.faces.tris;
memcpy (m_faceP->index, m_sideVerts, sizeof (m_faceP->index));
m_faceP->nType = gameStates.render.bTriangleMesh ? m_sideP->nType : -1;
m_faceP->nSegment = nSegment;
m_faceP->nSide = nSide;
m_faceP->nWall = gameStates.app.bD2XLevel ? m_nWall : IS_WALL (m_nWall) ? m_nWall : (ushort) -1;
m_faceP->bAnimation = IsAnimatedTexture (m_faceP->nBaseTex) || IsAnimatedTexture (m_faceP->nOvlTex);
}

//------------------------------------------------------------------------------

void CFaceMeshBuilder::InitTexturedFace (void)
{
m_faceP->nBaseTex = m_sideP->nBaseTex;
if ((m_faceP->nOvlTex = m_sideP->nOvlTex))
	m_nOvlTexCount++;
m_faceP->bSlide = (gameData.pig.tex.pTMapInfo [m_faceP->nBaseTex].slide_u || gameData.pig.tex.pTMapInfo [m_faceP->nBaseTex].slide_v);
m_faceP->bIsLight = IsLight (m_faceP->nBaseTex) || (m_faceP->nOvlTex && IsLight (m_faceP->nOvlTex));
m_faceP->nOvlOrient = (ubyte) m_sideP->nOvlOrient;
m_faceP->bTextured = 1;
m_faceP->bTransparent = 0;
char *pszName = gameData.pig.tex.bitmapFiles [gameStates.app.bD1Mission][gameData.pig.tex.pBmIndex [m_faceP->nBaseTex].index].name;
m_faceP->bSparks = (strstr (pszName, "misc17") != NULL);
if (m_nWallType < 2)
	m_faceP->bAdditive = 0;
else if (WALLS [m_nWall].flags & WALL_RENDER_ADDITIVE)
	m_faceP->bAdditive = 2;
else if (strstr (pszName, "lava"))
	m_faceP->bAdditive = 3;
else
	m_faceP->bAdditive = (strstr (pszName, "force") || m_faceP->bSparks) ? 1 : 0;
}

//------------------------------------------------------------------------------

void CFaceMeshBuilder::InitColoredFace (short nSegment)
{
m_faceP->nBaseTex = -1;
m_faceP->bTransparent = 1;
m_faceP->bAdditive = gameData.segs.segment2s [nSegment].special >= SEGMENT_IS_LAVA;
}

//------------------------------------------------------------------------------

void CFaceMeshBuilder::SetupFace (void)
{
	int			i, j;
	vmsVector	vNormal;
	fVector3		vNormalf;

VmVecAdd (&vNormal, m_sideP->normals, m_sideP->normals + 1);
VmVecScale (&vNormal, F1_0 / 2);
VmVecFixToFloat (&vNormalf, &vNormal);
for (i = 0; i < 4; i++) {
	j = m_sideVerts [i];
	*m_vertexP++ = gameData.segs.fVertices [j].v3;
	*m_normalP++ = vNormalf;
	m_texCoordP->v.u = f2fl (m_sideP->uvls [i].u);
	m_texCoordP->v.v = f2fl (m_sideP->uvls [i].v);
	RotateTexCoord2f (m_ovlTexCoordP, m_texCoordP, (ubyte) m_sideP->nOvlOrient);
	m_texCoordP++;
	m_ovlTexCoordP++;
	*m_faceColorP++ = gameData.render.color.ambient [j].color;
	}
}

//------------------------------------------------------------------------------

void CFaceMeshBuilder::SplitIn2Tris (void)
{
	static short	n2TriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};

	int	h, i, j, k, v;
	short	*triVertP;

h = (m_sideP->nType == SIDE_IS_TRI_13);
for (i = 0; i < 2; i++, m_triP++) {
	gameData.segs.nTris++;
	m_faceP->nTris++;
	m_triP->nFace = m_faceP - gameData.segs.faces.faces;
	m_triP->nIndex = m_vertexP - gameData.segs.faces.vertices;
	triVertP = n2TriVerts [h][i];
	for (j = 0; j < 3; j++) {
		k = triVertP [j];
		v = m_sideVerts [k];
		m_triP->index [j] = v;
		*m_vertexP++ = gameData.segs.fVertices [v].v3;
		m_texCoordP->v.u = f2fl (m_sideP->uvls [k].u);
		m_texCoordP->v.v = f2fl (m_sideP->uvls [k].v);
		RotateTexCoord2f (m_ovlTexCoordP, m_texCoordP, (ubyte) m_sideP->nOvlOrient);
		m_texCoordP++;
		m_ovlTexCoordP++;
		m_colorP = gameData.render.color.ambient + v;
		*m_faceColorP++ = m_colorP->color;
		}
	m_normalP = SetTriNormals (m_triP, m_normalP);
	}
}

//------------------------------------------------------------------------------

void CFaceMeshBuilder::SplitIn4Tris (void)
{
	static short	n4TriVerts [4][3] = {{0,1,4},{1,2,4},{2,3,4},{3,0,4}};

	fVector		vSide [4];
	tRgbaColorf	color;
	tTexCoord2f	texCoord;
	short			*triVertP;
	int			h, i, j, k, v;

texCoord.v.u = texCoord.v.v = 0;
color.red = color.green = color.blue = color.alpha = 0;
for (i = 0; i < 4; i++) {
	j = (i + 1) % 4;
	texCoord.v.u += f2fl (m_sideP->uvls [i].u + m_sideP->uvls [j].u) / 8;
	texCoord.v.v += f2fl (m_sideP->uvls [i].v + m_sideP->uvls [j].v) / 8;
	h = m_sideVerts [i];
	k = m_sideVerts [j];
	color.red += (gameData.render.color.ambient [h].color.red + gameData.render.color.ambient [k].color.red) / 8;
	color.green += (gameData.render.color.ambient [h].color.green + gameData.render.color.ambient [k].color.green) / 8;
	color.blue += (gameData.render.color.ambient [h].color.blue + gameData.render.color.ambient [k].color.blue) / 8;
	color.alpha += (gameData.render.color.ambient [h].color.alpha + gameData.render.color.ambient [k].color.alpha) / 8;
	}
VmLineLineIntersection (VmVecAvg (vSide, gameData.segs.fVertices + m_sideVerts [0], gameData.segs.fVertices + m_sideVerts [1]),
								VmVecAvg (vSide + 2, gameData.segs.fVertices + m_sideVerts [2], gameData.segs.fVertices + m_sideVerts [3]),
								VmVecAvg (vSide + 1, gameData.segs.fVertices + m_sideVerts [1], gameData.segs.fVertices + m_sideVerts [2]),
								VmVecAvg (vSide + 3, gameData.segs.fVertices + m_sideVerts [3], gameData.segs.fVertices + m_sideVerts [0]),
								gameData.segs.fVertices + gameData.segs.nVertices,
								gameData.segs.fVertices + gameData.segs.nVertices);
VmVecFloatToFix (gameData.segs.vertices + gameData.segs.nVertices, gameData.segs.fVertices + gameData.segs.nVertices);
m_sideVerts [4] = gameData.segs.nVertices++;
m_faceP->nVerts++;
for (i = 0; i < 4; i++, m_triP++) {
	gameData.segs.nTris++;
	m_faceP->nTris++;
	m_triP->nFace = m_faceP - gameData.segs.faces.faces;
	m_triP->nIndex = m_vertexP - gameData.segs.faces.vertices;
	triVertP = n4TriVerts [i];
	for (j = 0; j < 3; j++) {
		k = triVertP [j];
		v = m_sideVerts [k];
		m_triP->index [j] = v;
		*m_vertexP++ = gameData.segs.fVertices [v].v3;
		if (j == 2) {
			m_texCoordP [2] = texCoord;
			m_faceColorP [2] = color;
			}
		else {
			m_texCoordP [j].v.u = f2fl (m_sideP->uvls [k].u);
			m_texCoordP [j].v.v = f2fl (m_sideP->uvls [k].v);
			m_colorP = gameData.render.color.ambient + v;
			m_faceColorP [j] = m_colorP->color;
			}
		RotateTexCoord2f (m_ovlTexCoordP, m_texCoordP + j, (ubyte) m_sideP->nOvlOrient);
		m_ovlTexCoordP++;
		}
	m_normalP = SetTriNormals (m_triP, m_normalP);
	m_texCoordP += 3;
	m_faceColorP += 3;
	}
}

//------------------------------------------------------------------------------

#define FACE_VERTS	6

void CFaceMeshBuilder::BuildMesh (void)
{
m_faceP = gameData.segs.faces.faces;
m_triP = gameData.segs.faces.tris;
m_vertexP = gameData.segs.faces.vertices;
m_normalP = gameData.segs.faces.normals;
m_texCoordP = gameData.segs.faces.texCoord;
m_ovlTexCoordP = gameData.segs.faces.ovlTexCoord;
m_faceColorP = gameData.segs.faces.color;
m_colorP = gameData.render.color.ambient;
m_segP = SEGMENTS;
m_segFaceP = SEGFACES;

	short			nSegment, i;
	ubyte			nSide;
	
gameStates.render.bTriangleMesh = !gameOpts->ogl.bPerPixelLighting && gameOpts->render.nMeshQuality;
gameStates.render.nFacePrimitive = gameStates.render.bTriangleMesh ? GL_TRIANGLES : GL_TRIANGLE_FAN;
if (gameStates.render.bSplitPolys)
	gameStates.render.bSplitPolys = (gameOpts->ogl.bPerPixelLighting || !gameOpts->render.nMeshQuality) ? 1 : -1;

PrintLog ("   Creating face list\n");
gameData.segs.nFaces = 0;
gameData.segs.nTris = 0;
for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, m_segP++, m_segFaceP++) {
	m_bColoredSeg = ((gameData.segs.segment2s [nSegment].special >= SEGMENT_IS_WATER) &&
					   (gameData.segs.segment2s [nSegment].special <= SEGMENT_IS_TEAM_RED)) ||
					   (gameData.segs.xSegments [nSegment].group >= 0);
#ifdef _DEBUG
	if (nSegment == nDbgSeg)
		m_faceP = m_faceP;
#endif
	m_faceP->nSegment = nSegment;
	m_nOvlTexCount = 0;
	m_segFaceP->nFaces = 0;
	for (nSide = 0, m_sideP = m_segP->sides; nSide < 6; nSide++, m_sideP++) {
		m_nWall = WallNumI (nSegment, nSide);
		m_nWallType = IS_WALL (m_nWall) ? 2 : (m_segP->children [nSide] == -1) ? 1 : 0;
		if (m_bColoredSeg || m_nWallType) {
#ifdef _DEBUG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#endif
			GetSideVertIndex (m_sideVerts, nSegment, nSide);
			InitFace (nSegment, nSide);
			if (m_nWallType)
				InitTexturedFace ();
			else if (m_bColoredSeg)
				InitColoredFace (nSegment);
			if (gameStates.render.bTriangleMesh) {
				// split in four triangles, using the quad's center of gravity as additional vertex
				if (!gameOpts->ogl.bPerPixelLighting && (m_sideP->nType == SIDE_IS_QUAD) && IsBigFace (m_sideVerts))
					SplitIn4Tris ();
				else // split in two triangles, regarding any non-planarity
					SplitIn2Tris ();
				}
			else
				SetupFace ();
#ifdef _DEBUG
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
	if (!gameStates.render.bTriangleMesh && gameStates.ogl.bGlTexMerge && m_nOvlTexCount) { //allow for splitting multi-textured faces into two single textured ones
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
for (m_colorP = gameData.render.color.ambient, i = gameData.segs.nVertices; i; i--, m_colorP++)
	if (m_colorP->color.alpha > 1) {
		m_colorP->color.red /= m_colorP->color.alpha;
		m_colorP->color.green /= m_colorP->color.alpha;
		m_colorP->color.blue /= m_colorP->color.alpha;
		m_colorP->color.alpha = 1;
		}
#ifdef _DEBUG
if (!gameOpts->ogl.bPerPixelLighting && gameOpts->render.nMeshQuality)
	m_triMeshBuilder.BuildMesh ();
#endif
}

//------------------------------------------------------------------------------
//eof
