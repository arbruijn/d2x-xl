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
static char rcsid [] = "$Id: gamemine.c, v 1.26 2003/10/22 15:00:37 schaffner Exp $";
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

typedef struct tMeshLine {
	ushort		verts [2];
	ushort		tris [2];
	} tMeshLine;

typedef struct tMeshTri {
	ushort		nPass;
	ushort		nFace;
	int			nIndex;
	ushort		lines [3];
	ushort		index [3];
	tTexCoord2f	texCoord [3];
	tTexCoord2f	ovlTexCoord [3];
	tRgbaColorf	color [3];
} tMeshTri;

static tMeshLine	*meshLines;
static tMeshTri	*meshTris;
static int nMeshLines = 0;
static int nMeshTris = 0;
static int nMaxMeshLines = 0;
static int nMaxMeshTris = 0;
static int nVertices = 0;

static float fMaxSideLen [] = {1e30f, 40, 30, 20, 10};

#define	MAX_SIDE_LEN	fMaxSideLen [gameOpts->render.nMeshQuality]

//------------------------------------------------------------------------------

static void FreeMeshData (void)
{
D2_FREE (meshTris);
D2_FREE (meshLines);
}

//------------------------------------------------------------------------------

static int AllocMeshData (void)
{
if (nMaxMeshLines && nMaxMeshTris) {
	if (!((meshLines = (tMeshLine *) D2_REALLOC (meshLines, 2 * nMaxMeshLines * sizeof (tMeshLine))) &&
			(meshTris = (tMeshTri *) D2_REALLOC (meshTris, 2 * nMaxMeshTris * sizeof (tMeshTri))))) {
		FreeMeshData ();
		return 0;
		}
	memset (meshLines + nMaxMeshLines, 0xff, nMaxMeshLines * sizeof (tMeshLine));
	memset (meshTris + nMaxMeshTris, 0xff, nMaxMeshTris * sizeof (tMeshLine));
	nMaxMeshLines *= 2;
	nMaxMeshTris *= 2;
	}
else {
	nMaxMeshLines = gameData.segs.nFaces * 4;
	nMaxMeshTris = gameData.segs.nFaces * 4;
	if (!(meshLines = (tMeshLine *) D2_ALLOC (nMaxMeshLines * sizeof (tMeshLine))))
		return 0;
	if (!(meshTris = (tMeshTri *) D2_ALLOC (nMaxMeshTris * sizeof (tMeshTri)))) {
		FreeMeshData ();
		return 0;
		}
	memset (meshLines, 0xff, nMaxMeshLines * sizeof (tMeshLine));
	memset (meshTris, 0xff, nMaxMeshTris * sizeof (tMeshLine));
	}
return 1;
}

//------------------------------------------------------------------------------

static tMeshLine *FindMeshLine (short nVert1, short nVert2)
{
	tMeshLine	*mlP = meshLines;

for (int i = nMeshLines; i; i--, mlP++)
	if ((mlP->verts [0] == nVert1) && (mlP->verts [1] == nVert2))
		return mlP;
return NULL;
}

//------------------------------------------------------------------------------

static int AddMeshLine (short nTri, short nVert1, short nVert2)
{
if (nVert2 < nVert1) {
	short h = nVert1;
	nVert1 = nVert2;
	nVert2 = h;
	}
#ifdef _DEBUG
if ((nTri < 0) || (nTri >= nMeshTris))
	return -1;
#endif
tMeshLine *mlP = FindMeshLine (nVert1, nVert2);
if (mlP) {
	if (mlP->tris [0] == 65535)
		mlP->tris [0] = nTri;
	else
		mlP->tris [1] = nTri;
	return mlP - meshLines;
	}
if ((nMeshLines == nMaxMeshLines - 1) && !AllocMeshData ())
	return -1;
mlP = meshLines + nMeshLines;
mlP->tris [0] = nTri;
mlP->verts [0] = nVert1;
mlP->verts [1] = nVert2;
return nMeshLines++;
}

//------------------------------------------------------------------------------

static tMeshTri *CreateMeshTri (tMeshTri *mtP, ushort index [], short nFace, short nIndex)
{
	int	h, i;

if (mtP) 
	mtP->nIndex = nIndex;
else {
	if ((nMeshTris == nMaxMeshTris - 1) && !AllocMeshData ())
		return NULL;
	mtP = meshTris + nMeshTris++;
	mtP->nIndex = nIndex;
	if (nIndex >= 0) {
		i = gameData.segs.faces.tris [nIndex].nIndex;
		memcpy (mtP->texCoord, gameData.segs.faces.texCoord + i, sizeof (mtP->texCoord));
		memcpy (mtP->ovlTexCoord, gameData.segs.faces.ovlTexCoord + i, sizeof (mtP->ovlTexCoord));
		memcpy (mtP->color, gameData.segs.faces.color + i, sizeof (mtP->color));
		}
	}
nIndex = mtP - meshTris;
for (i = 0; i < 3; i++) {
	if (0 > (h = AddMeshLine (nIndex, index [i], index [(i + 1) % 3])))
		return NULL;
	mtP = meshTris + nIndex;
	mtP->lines [i] = h;
	}
mtP->nFace = nFace;
memcpy (mtP->index, index, sizeof (mtP->index));
return mtP;
}

//------------------------------------------------------------------------------

static int AddMeshTri (tMeshTri *mtP, ushort index [], grsTriangle *triP)
{
#if 1
return CreateMeshTri (mtP, index, triP->nFace, triP - gameData.segs.faces.tris) ? nMeshTris : 0;
#else
	int		h, i;
	float		l;
	
for (h = i = 0; i < 3; i++)
	if ((l = VmVecDist ((fVector *) (gameData.segs.fVertices + index [i]), 
								(fVector *) (gameData.segs.fVertices + index [(i + 1) % 3]))) >= MAX_SIDE_LEN)
		return CreateMeshTri (mtP, index, triP->nFace, triP - gameData.segs.faces.tris) ? nMeshTris : 0;
return nMeshTris;
#endif
}

//------------------------------------------------------------------------------

static void DeleteMeshLine (tMeshLine *mlP)
{
	tMeshTri	*mtP;
	int		h = mlP - meshLines, i, j;

if (h < --nMeshLines) {
	*mlP = meshLines [nMeshLines];
	for (i = 0; i < 2; i++) {
		if (mlP->tris [i] != 65535) {
			mtP = meshTris + mlP->tris [i];
			for (j = 0; j < 3; j++)
				if (mtP->lines [j] == nMeshLines)
					mtP->lines [j] = h;
			}
		}
	}
}

//------------------------------------------------------------------------------

static void DeleteMeshTri (tMeshTri *mtP)
{
	tMeshLine	*mlP;
	int			i, nTri = mtP - meshTris;

for (i = 0; i < 3; i++) {
	mlP = meshLines + mtP->lines [i];
	if (mlP->tris [0] == nTri)
		mlP->tris [0] = mlP->tris [1];
	mlP->tris [1] = 65535;
	if (mlP->tris [0] == 65535)
		DeleteMeshLine (mlP);
	}
}

//------------------------------------------------------------------------------

static int CreateMeshTris (void)
{
PrintLog ("   adding existing triangles\n");
nMeshLines = 0;
nMeshTris = 0;
nMaxMeshLines = 0;
nMaxMeshTris = 0;
nVertices = gameData.segs.nVertices;
if (!AllocMeshData ())
	return 0;

grsTriangle *triP;
int i;

for (i = gameData.segs.nTris, triP = gameData.segs.faces.tris; i; i--, triP++)
	if (!AddMeshTri (NULL, triP->index, triP)) {
		FreeMeshData ();
		return 0;
		}
return nMeshTris;
}

//------------------------------------------------------------------------------

static int SplitMeshTriByLine (int nTri, int nVert1, int nVert2, ushort nPass)
{
if (nTri == 65535)
	return 1;

	tMeshTri		*mtP = meshTris + nTri;
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

static int SplitMeshLine (tMeshLine *mlP, ushort nPass)
{
	int			i = 0;
	ushort		tris [2], verts [2];

memcpy (tris, mlP->tris, sizeof (tris));
memcpy (verts, mlP->verts, sizeof (verts));
VmVecAvg (gameData.segs.fVertices + gameData.segs.nVertices, 
			  gameData.segs.fVertices + verts [0], 
			  gameData.segs.fVertices + verts [1]);
if (/*(gameData.segs.faces.faces [meshTris [tris [0]].nFace].nSegment == 12) &&*/
	 (((verts [0] == 13) && (verts [1] == 33)) || ((verts [1] == 13) && (verts [0] == 33))))
	i = 1;
if (!SplitMeshTriByLine (tris [0], verts [0], verts [1], nPass))
	return 0;
if (tris [1] > nMeshTris)
	nMeshTris = nMeshTris;
if (!SplitMeshTriByLine (tris [1], verts [0], verts [1], nPass))
	return 0;
gameData.segs.faces.faces [meshTris [tris [0]].nFace].nVerts++;
if ((tris [0] != tris [1]) && (tris [1] < nMeshTris))
	gameData.segs.faces.faces [meshTris [tris [1]].nFace].nVerts++;
VmVecFloatToFix (gameData.segs.vertices + gameData.segs.nVertices, gameData.segs.fVertices + gameData.segs.nVertices);
gameData.segs.nVertices++;
return 1;
}

//------------------------------------------------------------------------------

static int SplitMeshTri (tMeshTri *mtP, ushort nPass)
{
	tMeshLine	*mlP;
	int			h, i;
	float			l, lMax = 0;

for (i = 0; i < 3; i++) {
	mlP = meshLines + mtP->lines [i];
	l = VmVecDist (gameData.segs.fVertices + mlP->verts [0], gameData.segs.fVertices + mlP->verts [1]);
	if (lMax < l) {
		lMax = l;
		h = i;
		}
	}
if (lMax < MAX_SIDE_LEN)
	return -1;
return SplitMeshLine (meshLines + mtP->lines [h], nPass);
}

//------------------------------------------------------------------------------

static int SplitMeshTris (void)
{
	int		bSplit = 0, h, i, j;
	ushort	nPass = 0;

do {
	bSplit = 0;
	j = nMeshTris;
	nPass++;
	PrintLog ("   splitting triangles (pass %d)\n", nPass);
	for (i = 0; i < j; i++) {
		if (meshTris [i].nPass == nPass)
			continue;
		h = SplitMeshTri (meshTris + i, nPass);
		if ((gameData.segs.nVertices == 65535) || (nMeshTris == 65535))
			return 1;
		if (!h)
			return 0;
		if (h > 0)
			bSplit = 1;
		}
	} while (bSplit);
return 1;
}

//------------------------------------------------------------------------------

void QSortMeshTris (int left, int right)
{
	int	l = left,
			r = right,
			m = meshTris [(l + r) / 2].nFace;

do {
	while (meshTris [l].nFace < m)
		l++;
	while (meshTris [r].nFace > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tMeshTri h = meshTris [l];
			meshTris [l] = meshTris [r];
			meshTris [r] = h;
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

int InsertMeshTris (void)
{
	tMeshTri		*mtP = meshTris;
	grsTriangle	*triP = gameData.segs.faces.tris;
	grsFace		*faceP = NULL;
	vmsVector	vNormal;
	int			h, i, nVertex, nIndex = 0, nVertIndex = 0, nFace = -1;

PrintLog ("   inserting new triangles\n");
QSortMeshTris (0, nMeshTris - 1);
ResetVertexNormals ();
for (h = 0; h < nMeshTris; h++, mtP++, triP++, nIndex += 3) {
	triP->nFace = mtP->nFace;
	if (triP->nFace == nFace) 
		faceP->nTris++;
	else {
		if (faceP)
			faceP++;
		else
			faceP = gameData.segs.faces.faces;
		nFace = triP->nFace;
		if (faceP - gameData.segs.faces.faces != nFace)
			return 0;
		faceP->nIndex = nIndex;
		faceP->nTriIndex = h;
		faceP->nTris = 1;
		faceP->vertIndex = gameData.segs.faces.vertIndex + nVertIndex;
		nVertIndex += faceP->nVerts;
		}
	triP->nIndex = nIndex;
	memcpy (triP->index, mtP->index, sizeof (triP->index));
	for (i = 0; i < 3; i++)
		gameData.segs.faces.vertices [nIndex + i] = gameData.segs.fVertices [mtP->index [i]].v3;
	VmVecNormal (gameData.segs.faces.normals + nIndex,
					 gameData.segs.faces.vertices + nIndex, 
					 gameData.segs.faces.vertices + nIndex + 1, 
					 gameData.segs.faces.vertices + nIndex + 2);
#ifdef _DEBUG
	if (VmVecMag (gameData.segs.faces.normals + nIndex) == 0)
		faceP = faceP;
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
	}
ComputeVertexNormals ();
FreeMeshData ();
PrintLog ("   created %d new triangles and %d new vertices\n", 
			 nMeshTris - gameData.segs.nTris, gameData.segs.nVertices - nVertices);
gameData.segs.nTris = nMeshTris;
return 1;
}

//------------------------------------------------------------------------------

int BuildTriangleMesh (void)
{
PrintLog ("creating triangle mesh\n");
if (!CreateMeshTris ())
	return 0;
if (!SplitMeshTris ())
	return 0;
return InsertMeshTris ();
}

//------------------------------------------------------------------------------

int IsBigFace (short *sideVerts)
{
for (int i = 0; i < 4; i++) 
	if (VmVecDist (gameData.segs.fVertices + sideVerts [i], gameData.segs.fVertices + sideVerts [(i + 1) % 4]) >= MAX_SIDE_LEN)
		return 1;
return 0;
}

//------------------------------------------------------------------------------

fVector3 *SetTriNormals (grsTriangle *triP, fVector3 *normalP)
{
	fVector	vNormalf;

VmVecNormal (&vNormalf, gameData.segs.fVertices + triP->index [0], 
				 gameData.segs.fVertices + triP->index [1], gameData.segs.fVertices + triP->index [2]);
*normalP++ = vNormalf.v3;
*normalP++ = vNormalf.v3;
*normalP++ = vNormalf.v3;
return normalP;
}

//------------------------------------------------------------------------------

#define FACE_VERTS	6

void CreateFaceList (void)
{
	grsFace		*faceP = gameData.segs.faces.faces;
	grsTriangle	*triP = gameData.segs.faces.tris;
	fVector3		*vertexP = gameData.segs.faces.vertices;
	fVector3		*normalP = gameData.segs.faces.normals;
	tTexCoord2f	*texCoordP = gameData.segs.faces.texCoord;
	tTexCoord2f	*ovlTexCoordP = gameData.segs.faces.ovlTexCoord;
	tRgbaColorf	*faceColorP = gameData.segs.faces.color;
	tFaceColor	*colorP = gameData.render.color.ambient;
	tSegment		*segP = SEGMENTS;
	tSegFaces	*segFaceP = SEGFACES;
	tSide			*sideP;
	short			nSegment, nWall, nOvlTexCount, h, i, j, k, v, sideVerts [5];
	ubyte			nSide, bColoredSeg, bWall;
	char			*pszName;
	short			*triVertP;

	short			n2TriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};
	short			n4TriVerts [4][3] = {{0,1,4},{1,2,4},{2,3,4},{3,0,4}};

PrintLog ("   Creating face list\n");
gameData.segs.nFaces = 0;
gameData.segs.nTris = 0;
for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, segP++, segFaceP++) {
	bColoredSeg = ((gameData.segs.segment2s [nSegment].special >= SEGMENT_IS_WATER) &&
					   (gameData.segs.segment2s [nSegment].special <= SEGMENT_IS_TEAM_RED)) ||
					   (gameData.segs.xSegments [nSegment].group >= 0);
#ifdef _DEBUG
	if (nSegment == nDbgSeg)
		faceP = faceP;
#endif
	faceP->nSegment = nSegment;
	nOvlTexCount = 0;
	segFaceP->nFaces = 0;
	for (nSide = 0, sideP = segP->sides; nSide < 6; nSide++, sideP++) {
		nWall = WallNumI (nSegment, nSide);
		bWall = IS_WALL (nWall) ? 2 : (segP->children [nSide] == -1) ? 1 : 0;
		if (bColoredSeg || bWall) {
#ifdef _DEBUG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#endif
			memset (faceP, 0, sizeof (*faceP));
			faceP->nSegment = nSegment;
			faceP->nVerts = 4;
			faceP->nIndex = vertexP - gameData.segs.faces.vertices;
			faceP->nTriIndex = triP - gameData.segs.faces.tris;
			GetSideVertIndex (sideVerts, nSegment, nSide);
			memcpy (faceP->index, sideVerts, sizeof (faceP->index));
			// split in four triangles, using the quad's center of gravity as additional vertex
			if (!gameOpts->ogl.bPerPixelLighting && (sideP->nType == SIDE_IS_QUAD) && IsBigFace (sideVerts)) {
				fVector		vSide [4];
				tRgbaColorf	color;
				tTexCoord2f	texCoord;

				texCoord.v.u = texCoord.v.v = 0;
				color.red = color.green = color.blue = color.alpha = 0;
				for (i = 0; i < 4; i++) {
					j = (i + 1) % 4;
					texCoord.v.u += f2fl (sideP->uvls [i].u + sideP->uvls [j].u) / 8;
					texCoord.v.v += f2fl (sideP->uvls [i].v + sideP->uvls [j].v) / 8;
					h = sideVerts [i];
					k = sideVerts [j];
					color.red += (gameData.render.color.ambient [h].color.red + gameData.render.color.ambient [k].color.red) / 8;
					color.green += (gameData.render.color.ambient [h].color.green + gameData.render.color.ambient [k].color.green) / 8;
					color.blue += (gameData.render.color.ambient [h].color.blue + gameData.render.color.ambient [k].color.blue) / 8;
					color.alpha += (gameData.render.color.ambient [h].color.alpha + gameData.render.color.ambient [k].color.alpha) / 8;
					}
				VmLineLineIntersection (VmVecAvg (vSide, gameData.segs.fVertices + sideVerts [0], gameData.segs.fVertices + sideVerts [1]),
												VmVecAvg (vSide + 2, gameData.segs.fVertices + sideVerts [2], gameData.segs.fVertices + sideVerts [3]),
												VmVecAvg (vSide + 1, gameData.segs.fVertices + sideVerts [1], gameData.segs.fVertices + sideVerts [2]),
												VmVecAvg (vSide + 3, gameData.segs.fVertices + sideVerts [3], gameData.segs.fVertices + sideVerts [0]),
												gameData.segs.fVertices + gameData.segs.nVertices,
												gameData.segs.fVertices + gameData.segs.nVertices);
				VmVecFloatToFix (gameData.segs.vertices + gameData.segs.nVertices, gameData.segs.fVertices + gameData.segs.nVertices);
				sideVerts [4] = gameData.segs.nVertices++;
				faceP->nVerts++;
				for (i = 0; i < 4; i++, triP++) {
					gameData.segs.nTris++;
					faceP->nTris++;
					triP->nFace = faceP - gameData.segs.faces.faces;
					triP->nIndex = vertexP - gameData.segs.faces.vertices;
					triVertP = n4TriVerts [i];
					for (j = 0; j < 3; j++) {
						k = triVertP [j];
						v = sideVerts [k];
						triP->index [j] = v;
						*vertexP++ = gameData.segs.fVertices [v].v3;
						if (j == 2) {
							texCoordP [2] = texCoord;
							faceColorP [2] = color;
							}
						else {
							texCoordP [j].v.u = f2fl (sideP->uvls [k].u);
							texCoordP [j].v.v = f2fl (sideP->uvls [k].v);
							colorP = gameData.render.color.ambient + v;
							faceColorP [j] = colorP->color;
							}
						RotateTexCoord2f (ovlTexCoordP, texCoordP + j, (ubyte) sideP->nOvlOrient);
						ovlTexCoordP++;
						}
					normalP = SetTriNormals (triP, normalP);
					texCoordP += 3;
					faceColorP += 3;
					}
				}
			else { // split in two triangles, regarding any non-planarity
				h = (sideP->nType == SIDE_IS_TRI_13);
				for (i = 0; i < 2; i++, triP++) {
					gameData.segs.nTris++;
					faceP->nTris++;
					triP->nFace = faceP - gameData.segs.faces.faces;
					triP->nIndex = vertexP - gameData.segs.faces.vertices;
					triVertP = n2TriVerts [h][i];
					for (j = 0; j < 3; j++) {
						k = triVertP [j];
						v = sideVerts [k];
						triP->index [j] = v;
						*vertexP++ = gameData.segs.fVertices [v].v3;
						texCoordP->v.u = f2fl (sideP->uvls [k].u);
						texCoordP->v.v = f2fl (sideP->uvls [k].v);
						RotateTexCoord2f (ovlTexCoordP, texCoordP, (ubyte) sideP->nOvlOrient);
						texCoordP++;
						ovlTexCoordP++;
						colorP = gameData.render.color.ambient + v;
						*faceColorP++ = colorP->color;
						}
					normalP = SetTriNormals (triP, normalP);
					}
				}
#ifdef _DEBUG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				faceP = faceP;
#endif
			if (bWall) {
				faceP->nBaseTex = sideP->nBaseTex;
				if ((faceP->nOvlTex = sideP->nOvlTex))
					nOvlTexCount++;
				faceP->bSlide = (gameData.pig.tex.pTMapInfo [faceP->nBaseTex].slide_u || gameData.pig.tex.pTMapInfo [faceP->nBaseTex].slide_v);
				faceP->bIsLight = IsLight (faceP->nBaseTex) || (faceP->nOvlTex && IsLight (faceP->nOvlTex));
				faceP->nOvlOrient = (ubyte) sideP->nOvlOrient;
				faceP->bTextured = 1;
				faceP->bTransparent = 0;
				pszName = gameData.pig.tex.bitmapFiles [gameStates.app.bD1Mission][gameData.pig.tex.pBmIndex [faceP->nBaseTex].index].name;
				faceP->bSparks = (strstr (pszName, "misc17") != NULL);
				if (bWall < 2)
					faceP->bAdditive = 0;
				else if (WALLS [nWall].flags & WALL_RENDER_ADDITIVE)
					faceP->bAdditive = 2;
				else if (strstr (pszName, "lava"))
					faceP->bAdditive = 3;
				else
					faceP->bAdditive = (strstr (pszName, "force") || faceP->bSparks) ? 1 : 0;
				} 
			else if (bColoredSeg) {
				faceP->nBaseTex = -1;
				faceP->bTransparent = 1;
				faceP->bAdditive = gameData.segs.segment2s [nSegment].special >= SEGMENT_IS_LAVA;
				}
			faceP->nType = sideP->nType;
			faceP->nSegment = nSegment;
			faceP->nSide = nSide;
			faceP->nWall = gameStates.app.bD2XLevel ? nWall : IS_WALL (nWall) ? nWall : (ushort) -1;
			faceP->bAnimation = IsAnimatedTexture (faceP->nBaseTex) || IsAnimatedTexture (faceP->nOvlTex);
			if (!segFaceP->nFaces++)
				segFaceP->pFaces = faceP;
			faceP++;
			gameData.segs.nFaces++;
			}	
		else {
			colorP += FACE_VERTS;
			}
		}
#if 0
	if (gameStates.ogl.bGlTexMerge && nOvlTexCount) {	//allow for splitting multi-textured faces into two single textured ones
		gameData.segs.nFaces += nOvlTexCount;
		faceP += nOvlTexCount;
		triP += 2;
		vertexP += nOvlTexCount * FACE_VERTS;
		normalP += nOvlTexCount * FACE_VERTS;
		texCoordP += nOvlTexCount * FACE_VERTS;
		ovlTexCoordP += nOvlTexCount * FACE_VERTS;
		faceColorP += nOvlTexCount * FACE_VERTS;
		}
#endif
	}
for (colorP = gameData.render.color.ambient, i = gameData.segs.nVertices; i; i--, colorP++)
	if (colorP->color.alpha > 1) {
		colorP->color.red /= colorP->color.alpha;
		colorP->color.green /= colorP->color.alpha;
		colorP->color.blue /= colorP->color.alpha;
		colorP->color.alpha = 1;
		}
#ifdef _DEBUG
if (!gameOpts->ogl.bPerPixelLighting)
	BuildTriangleMesh ();
#endif
}

//------------------------------------------------------------------------------
//eof
