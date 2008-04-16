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
	if (!((meshLines = (tMeshLine *) D2_REALLOC (meshLines, nMaxMeshLines * sizeof (tMeshLine))) &&
			(meshTris = (tMeshTri *) D2_REALLOC (meshTris, nMaxMeshTris * sizeof (tMeshTri))))) {
		FreeMeshData ();
		return 0;
		}
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
tMeshLine *mlP = FindMeshLine (nVert1, nVert2);
if (mlP) {
	if (mlP->tris [0] < 0)
		mlP->tris [0] = nTri;
	else
		mlP->tris [1] = nTri;
	return mlP - meshLines;
	}
if ((nMeshLines == nMaxMeshLines - 1) && !AllocMeshData ())
	return -1;
mlP = meshLines + nMeshLines++;
mlP->tris [0] = nTri;
mlP->verts [0] = nVert1;
mlP->verts [1] = nVert2;
return nMeshLines;
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
for (i = 0; i < 3; i++) {
	if (0 > (h = AddMeshLine (nMeshTris - 1, index [i], index [(i + 1) % 3])))
		return NULL;
	mtP->lines [i] = h;
	}
mtP->nFace = nFace;
memcpy (mtP->index, index, sizeof (mtP->index));
return mtP;
}

//------------------------------------------------------------------------------

static int AddMeshTri (tMeshTri *mtP, ushort index [], grsTriangle *triP)
{
	int		h, i;
	
for (h = i = 0; i < 3; i++)
	if (VmVecDistf ((fVector *) (gameData.segs.faces.vertices + index [i]), 
						 (fVector *) (gameData.segs.faces.vertices + index [(i + 1) % 3])) > 30.0f)
		return CreateMeshTri (mtP, index, triP->nFace, triP - gameData.segs.faces.tris) ? nMeshTris : 0;
return nMeshTris;
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
	else
		mlP->tris [1] = -1;
	}
}

//------------------------------------------------------------------------------

static int CreateMeshTris (void)
{
nMeshLines = 0;
nMeshTris = 0;
nMaxMeshLines = 0;
nMaxMeshTris = 0;
if (!AllocMeshData ())
	return 0;

grsTriangle *triP;
int i;

for (i = gameData.segs.nFaces * 2, triP = gameData.segs.faces.tris; i; i--, triP++)
	if (!AddMeshTri (NULL, triP->index, triP)) {
		FreeMeshData ();
		return 0;
		}
return nMeshTris;
}

//------------------------------------------------------------------------------

static int SplitMeshTriByLine (int nTri, int nVert1, int nVert2)
{
	tMeshTri		*mtP = meshTris + nTri;
	int			h, i, j, nIndex = mtP->nIndex;
	ushort		nFace = mtP->nFace, *indexP = mtP->index, index [4];
	tTexCoord2f	texCoord [4], ovlTexCoord [4];
	tRgbaColorf	color [4];

// find insertion point for the new vertex
for (i = 0; i < 3; i++) {
	h = indexP [i];
	if ((h == nVert1) || (h == nVert2))
		break;
	}

h = indexP [(i + 1) % 3]; //check next vertex index
if ((h == nVert1) || (h == nVert2))
	h = (i + 1) % 3; //insert before i+1
else
	h = i; //insert before i

// build new quad index containing the new vertex
// the two triangle indices will be derived from it (indices 0,1,2 and 1,2,3)
for (j = i = 0; i < 3; i++, j++) {
	if (i == h)
		index [j++] = gameData.segs.nVertices;
	index [j] = indexP [i];
	texCoord [j] = mtP->texCoord [i];
	ovlTexCoord [j] = mtP->ovlTexCoord [i];
	color [j] = mtP->color [i];
	}

// interpolate texture coordinates and color for the new vertex
i = h ? h - 1 : 2;
j = h + 1;
texCoord [h].v.v = (texCoord [i].v.v + texCoord [j].v.v) / 2;
texCoord [h].v.u = (texCoord [i].v.u + texCoord [j].v.u) / 2;
ovlTexCoord [h].v.v = (ovlTexCoord [i].v.v + ovlTexCoord [j].v.v) / 2;
ovlTexCoord [h].v.u = (ovlTexCoord [i].v.u + ovlTexCoord [j].v.u) / 2;
color [h].red = (color [i].red + color [j].red) / 2;
color [h].green = (color [i].green + color [j].green) / 2;
color [h].blue = (color [i].blue + color [j].blue) / 2;
color [h].alpha = (color [i].alpha + color [j].alpha) / 2;

DeleteMeshTri (mtP); //remove any references to this triangle
if (!(mtP = CreateMeshTri (mtP, index, nFace, nIndex))) //create a new triangle at this location (insert)
	return 0;
memcpy (mtP->color, color, sizeof (mtP->color));
memcpy (mtP->texCoord, texCoord, sizeof (mtP->texCoord));
memcpy (mtP->ovlTexCoord, ovlTexCoord, sizeof (mtP->ovlTexCoord));

index [1] = index [0];
if (!(mtP = CreateMeshTri (NULL, index + 1, nFace, -1))) //create a new triangle (append)
	return 0;
texCoord [1] = texCoord [0];
ovlTexCoord [1] = ovlTexCoord [0];
color [1] = color [0];
memcpy (mtP->color, color, sizeof (mtP->color));
memcpy (mtP->texCoord, texCoord, sizeof (mtP->texCoord));
memcpy (mtP->ovlTexCoord, ovlTexCoord, sizeof (mtP->ovlTexCoord));
return 1;
}

//------------------------------------------------------------------------------

static int SplitMeshLine (tMeshLine *mlP)
{
	int			i = 0;
	ushort		tris [2], verts [2];

memcpy (tris, mlP->tris, sizeof (tris));
memcpy (verts, mlP->verts, sizeof (verts));
VmVecAvgf (gameData.segs.fVertices + gameData.segs.nVertices, 
			  gameData.segs.fVertices + verts [0], 
			  gameData.segs.fVertices + verts [1]);
mlP->verts [1] = gameData.segs.nVertices;
if (!SplitMeshTriByLine (tris [0], verts [0], verts [1]))
	return 0;
if (!SplitMeshTriByLine (tris [1], verts [0], verts [1]))
	return 0;
gameData.segs.nVertices++;
return 1;
}

//------------------------------------------------------------------------------

static int SplitMeshTri (tMeshTri *mtP)
{
	tMeshLine	*mlP;
	int			h, i;
	float			l, lMax = 0;

for (i = 0; i < 3; i++) {
	mlP = meshLines + mtP->lines [i];
	l = VmVecDistf (gameData.segs.fVertices + mlP->verts [0], gameData.segs.fVertices + mlP->verts [1]);
	if (lMax < l) {
		lMax = l;
		h = i;
		}
	}
if (lMax <= 30.0f)
	return -1;
return SplitMeshLine (meshLines + mtP->lines [h]);
}

//------------------------------------------------------------------------------

static int SplitMeshTris (void)
{
	int		h, i = 0;

while (i < nMeshTris) {
	h = SplitMeshTri (meshTris + i);
	if (!h)
		return 0;
	if (h < 0)
		i++;
	}
return 1;
}

//------------------------------------------------------------------------------

void QSortTriangleMesh (int left, int right)
{
	int	l = left,
			r = right,
			m = gameData.segs.faces.tris [(l + r) / 2].nFace;

do {
	while (gameData.segs.faces.tris [l].nFace < m)
		l++;
	while (gameData.segs.faces.tris [r].nFace > m)
		r--;
	if (l <= r) {
		if (l < r) {
			grsTriangle	h = gameData.segs.faces.tris [l];
			gameData.segs.faces.tris [l] = gameData.segs.faces.tris [r];
			gameData.segs.faces.tris [r] = h;
			}
		l++;
		r--;
		}	
	} while (l <= r);
if (l < right)
	QSortTriangleMesh (l, right);
if (left < r)
	QSortTriangleMesh (left, r);
}

//------------------------------------------------------------------------------

int AssignTriangleMesh (int nTris)
{
	grsFace		*faceP = gameData.segs.faces.faces;
	grsTriangle	*triP = gameData.segs.faces.tris;
	int			i, nFace = -1;

for (i = 0; i < nTris; i++, triP++) {
	if (triP->nFace != nFace) {
		nFace = triP->nFace;
		if (faceP - gameData.segs.faces.faces != nFace)
			return 0;
		faceP->nTriIndex = i;
		faceP++;
		}
	}
return 1;
}


//------------------------------------------------------------------------------

int InsertTriangleMesh (void)
{
	tMeshTri		*mtP = meshTris;
	grsTriangle	*triP;
	int			h, i, j, nTris = gameData.segs.nFaces * 2, nIndex = nTris * 3;

for (h = nMeshTris; h; h--, mtP++) {
	if (0 > (i = mtP->nIndex))
		i = nTris++;
	triP = gameData.segs.faces.tris + i;
	triP->nFace = mtP->nFace;
	if (0 > (i = mtP->nIndex)) {
		i = nIndex;
		nIndex += 3;
		}
	triP->nIndex = i;
	for (j = 0; j < 3; j++)
		gameData.segs.faces.vertices [i + j] = gameData.segs.fVertices [mtP->index [j]].v3;
	if (0 > mtP->nIndex) {
		gameData.segs.faces.faces [mtP->nFace].nTris++;
		VmVecNormalf (gameData.segs.faces.normals + i,
						  gameData.segs.faces.vertices + i, 
						  gameData.segs.faces.vertices + i + 1, 
						  gameData.segs.faces.vertices + i + 2);
		for (j = 1; j < 3; j++)
			gameData.segs.faces.normals [i + j] = gameData.segs.faces.normals [i];
		}
	memcpy (gameData.segs.faces.texCoord, mtP->texCoord, sizeof (mtP->texCoord));
	memcpy (gameData.segs.faces.ovlTexCoord, mtP->ovlTexCoord, sizeof (mtP->ovlTexCoord));
	memcpy (gameData.segs.faces.color, mtP->color, sizeof (mtP->color));
	}
FreeMeshData ();
QSortTriangleMesh (0, nTris - 1);
return AssignTriangleMesh (nTris);
}

//------------------------------------------------------------------------------

int BuildTriangleMesh (void)
{
if (!CreateMeshTris ())
	return 0;
if (!SplitMeshTris ())
	return 0;
return InsertTriangleMesh ();
}

//------------------------------------------------------------------------------

#define FACE_VERTS	6

void CreateFaceList (void)
{
	grsFace		*faceP = gameData.segs.faces.faces;
	grsTriangle	*triP = gameData.segs.faces.tris;
	fVector3		*vertexP = gameData.segs.faces.vertices;
	fVector3		*normalP = gameData.segs.faces.normals, vNormalf;
	tTexCoord2f	*texCoordP = gameData.segs.faces.texCoord;
	tTexCoord2f	*ovlTexCoordP = gameData.segs.faces.ovlTexCoord;
	tRgbaColorf	*faceColorP = gameData.segs.faces.color;
	tFaceColor	*colorP = gameData.render.color.ambient;
	tSegment		*segP = SEGMENTS;
	tSegFaces	*segFaceP = SEGFACES;
	tSide			*sideP;
	vmsVector	vNormal;
	short			nSegment, nWall, nOvlTexCount, h, i, j, k, v, sideVerts [4];
	ubyte			nSide, bColoredSeg, bWall;
	char			*pszName;

	short			nTriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};

PrintLog ("   Creating face list\n");
gameData.segs.nFaces = 0;
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
			memset (faceP, 0, sizeof (*faceP));
			faceP->nSegment = nSegment;
			faceP->nIndex = vertexP - gameData.segs.faces.vertices;
			faceP->nTriIndex = triP - gameData.segs.faces.tris;
			GetSideVertIndex (sideVerts, nSegment, nSide);
			VmVecAdd (&vNormal, sideP->normals, sideP->normals + 1);
			VmVecScale (&vNormal, F1_0 / 2);
			VmVecFixToFloat (&vNormalf, &vNormal);
			h = (sideP->nType == SIDE_IS_TRI_13);
			for (i = 0; i < 2; i++, triP++) {
				faceP->nTris++;
				triP->nFace = faceP - gameData.segs.faces.faces;
				triP->nIndex = vertexP - gameData.segs.faces.vertices;
				for (j = 0; j < 3; j++) {
					k = nTriVerts [h][i][j];
					v = sideVerts [k];
					triP->index [j] = v;
					memcpy (vertexP++, gameData.segs.fVertices + v, sizeof (fVector3));
					*normalP++ = vNormalf;
					texCoordP->v.u = f2fl (sideP->uvls [k].u);
					texCoordP->v.v = f2fl (sideP->uvls [k].v);
					RotateTexCoord2f (ovlTexCoordP, texCoordP, (ubyte) sideP->nOvlOrient);
					texCoordP++;
					ovlTexCoordP++;
					colorP = gameData.render.color.ambient + v;
					*faceColorP++ = colorP->color;
					colorP++;
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
			faceP->nType = -1;
			faceP->nSegment = nSegment;
			faceP->nSide = nSide;
			faceP->nWall = gameStates.app.bD2XLevel ? nWall : IS_WALL (nWall) ? nWall : (ushort) -1;
			faceP->bAnimation = IsAnimatedTexture (faceP->nBaseTex) || IsAnimatedTexture (faceP->nOvlTex);
			memcpy (faceP->index, sideVerts, sizeof (faceP->index));
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
BuildTriangleMesh ();
}

//------------------------------------------------------------------------------
//eof
