/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTTIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DETIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEATING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYTIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL TIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "inferno.h"
#include "u_mem.h"
#include "error.h"
#include "ogl_lib.h"
#include "ogl_fastrender.h"
#include "render.h"
#include "gameseg.h"
#include "objrender.h"
#include "lightmap.h"
#include "lightning.h"
#include "sphere.h"
#include "glare.h"
#include "transprender.h"
#include "renderthreads.h"

#define RENDER_TRANSPARENCY 1
#define RENDER_TRANSP_DECALS 1

#define TI_SPLIT_POLYS 0
#define TI_POLY_OFFSET 0
#define TI_POLY_CENTER 0

#if DBG
static int nDbgPoly = -1, nDbgItem = -1;
#endif

//------------------------------------------------------------------------------

tTranspItemBuffer	transpItems;

static tTexCoord2f tcDefault [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

inline int AllocTranspItems (void)
{
if (transpItems.depthBufP)
	return 1;
if (!(transpItems.depthBufP = new tTranspItem* [ITEM_DEPTHBUFFER_SIZE]))
	return 0;
if (!(transpItems.itemListP = new tTranspItem [ITEM_BUFFER_SIZE])) {
	delete[] transpItems.depthBufP;
	transpItems.depthBufP = NULL;
	return 0;
	}
transpItems.nFreeItems = 0;
ResetTranspItemBuffer ();
return 1;
}

//------------------------------------------------------------------------------

void FreeTranspItems (void)
{
delete[] transpItems.itemListP;
transpItems.itemListP = NULL;
delete[] transpItems.depthBufP;
transpItems.depthBufP = NULL;
}

//------------------------------------------------------------------------------

void ResetTranspItemBuffer (void)
{
memset (transpItems.depthBufP, 0, ITEM_DEPTHBUFFER_SIZE * sizeof (struct tTranspItem **));
memset (transpItems.itemListP, 0, (ITEM_BUFFER_SIZE - transpItems.nFreeItems) * sizeof (struct tTranspItem));
transpItems.nFreeItems = ITEM_BUFFER_SIZE;
}


//------------------------------------------------------------------------------

void InitTranspItemBuffer (int zMin, int zMax)
{
transpItems.zMin = 0;
transpItems.zMax = zMax - transpItems.zMin;
transpItems.zScale = (double) (ITEM_DEPTHBUFFER_SIZE - 1) / (double) (zMax - transpItems.zMin);
if (transpItems.zScale < 0)
	transpItems.zScale = 1;
else if (transpItems.zScale > 1)
	transpItems.zScale = 1;
}

//------------------------------------------------------------------------------

int AddTranspItem (tTranspItemType nType, void *itemData, int itemSize, int nDepth, int nIndex)
{
#if RENDER_TRANSPARENCY
	tTranspItem *ph, *pi, *pj, **pd;
	int			nOffset;

#if DBG
if (nDepth < transpItems.zMin)
	return transpItems.nFreeItems;
if (nDepth > transpItems.zMax) {
	//if (nType != riParticle)
		return transpItems.nFreeItems;
	nDepth = transpItems.zMax;
	}
#else
if ((nDepth < transpItems.zMin) || (nDepth > transpItems.zMax))
	return transpItems.nFreeItems;
#endif
AllocTranspItems ();
if (!transpItems.nFreeItems)
	return 0;
#if 1
	nOffset = (int) ((double) (nDepth - transpItems.zMin) * transpItems.zScale);
#else
if (nIndex < transpItems.zMin)
	nOffset = 0;
else
	nOffset = (int) ((double) (nIndex - transpItems.zMin) * transpItems.zScale);
#endif
if (nOffset >= ITEM_DEPTHBUFFER_SIZE)
	return 0;
pd = transpItems.depthBufP + nOffset;
// find the first particle to insert the new one *before* and place in pj; pi will be it's predecessor (NULL if to insert at list start)
ph = transpItems.itemListP + --transpItems.nFreeItems;
ph->nItem = transpItems.nItems++;
ph->nType = nType;
ph->bRendered = 0;
ph->parentP = NULL;
ph->z = nDepth;
ph->bValid = true;
memcpy (&ph->item, itemData, itemSize);
if (*pd)
	pj = *pd;
for (pi = NULL, pj = *pd; pj && ((pj->z < nDepth) || ((pj->z == nDepth) && (pj->nType < nType))); pj = pj->pNextItem)
	pi = pj;
if (pi) {
	ph->pNextItem = pi->pNextItem;
	pi->pNextItem = ph;
	}
else {
	ph->pNextItem = *pd;
	*pd = ph;
	}
if (transpItems.nMinOffs > nOffset)
	transpItems.nMinOffs = nOffset;
if (transpItems.nMaxOffs < nOffset)
	transpItems.nMaxOffs = nOffset;
return transpItems.nFreeItems;
#else
return 0;
#endif
}

//------------------------------------------------------------------------------

int AddTranspItemMT (tTranspItemType nType, void *itemData, int itemSize, int nDepth, int nIndex, int nThread)
{
if (!gameStates.app.bMultiThreaded || (nThread < 0) || !gameData.app.bUseMultiThreading [rtTransparency])
	return AddTranspItem (nType, itemData, itemSize, nDepth, nIndex);
while (tiTranspItems.ti [nThread].bExec)
	G3_SLEEP (0);
tiTranspItems.itemData [nThread].nType = nType;
tiTranspItems.itemData [nThread].nSize = itemSize;
tiTranspItems.itemData [nThread].nDepth = nDepth;
tiTranspItems.itemData [nThread].nIndex = nIndex;
memcpy (&tiTranspItems.itemData [nThread].item, itemData, itemSize);
tiTranspItems.ti [nThread].bExec = 1;
return 1;
}

//------------------------------------------------------------------------------

#if TI_SPLIT_POLYS

int TISplitPoly (tTranspPoly *item, int nDepth)
{
	tTranspPoly		split [2];
	CFloatVector		vSplit;
	tRgbaColorf	color;
	int			i, l, i0, i1, i2, i3, nMinLen = 0x7fff, nMaxLen = 0;
	float			z, zMin, zMax, *c, *c0, *c1;

split [0] = split [1] = *item;
for (i = i0 = 0; i < split [0].nVertices; i++) {
	l = split [0].sideLength [i];
	if (nMaxLen < l) {
		nMaxLen = l;
		i0 = i;
		}
	if (nMinLen > l)
		nMinLen = l;
	}
if ((nDepth > 1) || !nMaxLen || (nMaxLen < 10) || ((nMaxLen <= 30) && ((split [0].nVertices == 3) || (nMaxLen <= nMinLen / 2 * 3)))) {
	for (i = 0, zMax = 0, zMin = 1e30f; i < split [0].nVertices; i++) {
		z = split [0].vertices [i][Z];
		if (zMax < z)
			zMax = z;
		if (zMin > z)
			zMin = z;
		}
#if TI_POLY_CENTER
	return AddTranspItem (item->bmP ? riTexPoly : riFlatPoly, item, sizeof (*item), F2X (zMax), F2X ((zMax + zMin) / 2));
#else
	return AddTranspItem (item->bmP ? riTexPoly : riFlatPoly, item, sizeof (*item), F2X (zMax), F2X (zMin));
#endif
	}
if (split [0].nVertices == 3) {
	i1 = (i0 + 1) % 3;
	vSplit = CFloatVector::Avg(split [0].vertices [i0], split [0].vertices [i1]);
	split [0].vertices [i0] =
	split [1].vertices [i1] = vSplit;
	split [0].sideLength [i0] =
	split [1].sideLength [i0] = nMaxLen / 2;
	if (split [0].bmP) {
		split [0].texCoord [i0].v.u =
		split [1].texCoord [i1].v.u = (split [0].texCoord [i1].v.u + split [0].texCoord [i0].v.u) / 2;
		split [0].texCoord [i0].v.v =
		split [1].texCoord [i1].v.v = (split [0].texCoord [i1].v.v + split [0].texCoord [i0].v.v) / 2;
		}
	if (split [0].nColors == 3) {
		for (i = 4, c = reinterpret_cast<float*> (&color, c0 = reinterpret_cast<float*> (split [0].color + i0), c1 = reinterpret_cast<float*> (split [0].color + i1); i; i--)
			*c++ = (*c0++ + *c1++) / 2;
		split [0].color [i0] =
		split [1].color [i1] = color;
		}
	}
else {
	i1 = (i0 + 1) % 4;
	i2 = (i0 + 2) % 4;
	i3 = (i1 + 2) % 4;
	vSplit = CFloatVector::Avg(split [0].vertices [i0], split [0].vertices [i1]);
	split [0].vertices [i1] =
	split [1].vertices [i0] = vSplit;
	vSplit = CFloatVector::Avg(split [0].vertices [i2], split [0].vertices [i3]);
	split [0].vertices [i2] =
	split [1].vertices [i3] = vSplit;
	if (split [0].bmP) {
		split [0].texCoord [i1].v.u =
		split [1].texCoord [i0].v.u = (split [0].texCoord [i1].v.u + split [0].texCoord [i0].v.u) / 2;
		split [0].texCoord [i1].v.v =
		split [1].texCoord [i0].v.v = (split [0].texCoord [i1].v.v + split [0].texCoord [i0].v.v) / 2;
		split [0].texCoord [i2].v.u =
		split [1].texCoord [i3].v.u = (split [0].texCoord [i3].v.u + split [0].texCoord [i2].v.u) / 2;
		split [0].texCoord [i2].v.v =
		split [1].texCoord [i3].v.v = (split [0].texCoord [i3].v.v + split [0].texCoord [i2].v.v) / 2;
		}
	if (split [0].nColors == 4) {
		for (i = 4, c = reinterpret_cast<float*> (&color, c0 = reinterpret_cast<float*> (split [0].color + i0), c1 = reinterpret_cast<float*> (split [0].color + i1); i; i--)
			*c++ = (*c0++ + *c1++) / 2;
		split [0].color [i1] =
		split [1].color [i0] = color;
		for (i = 4, c = reinterpret_cast<float*> (&color, c0 = reinterpret_cast<float*> (split [0].color + i2), c1 = reinterpret_cast<float*> (split [0].color + i3); i; i--)
			*c++ = (*c0++ + *c1++) / 2;
		split [0].color [i2] =
		split [1].color [i3] = color;
		}
	split [0].sideLength [i0] =
	split [1].sideLength [i0] =
	split [0].sideLength [i2] =
	split [1].sideLength [i2] = nMaxLen / 2;
	}
return TISplitPoly (split, nDepth + 1) && TISplitPoly (split + 1, nDepth + 1);
}

#endif

//------------------------------------------------------------------------------

int TIAddObject (CObject *objP)
{
	tTranspObject	item;
	CFixVector		vPos;

if (objP->info.nType == 255)
	return 0;
item.objP = objP;
G3TransformPoint(vPos, OBJPOS (objP)->vPos, 0);
return AddTranspItem (tiObject, &item, sizeof (item), vPos [Z], vPos [Z]);
}

//------------------------------------------------------------------------------

int TIAddPoly (tFace *faceP, grsTriangle *triP, CBitmap *bmP,
					CFloatVector *vertices, char nVertices, tTexCoord2f *texCoord, tRgbaColorf *color,
					tFaceColor *altColor, char nColors, char bDepthMask, int nPrimitive, int nWrap, int bAdditive,
					short nSegment)
{
	tTranspPoly	item;
	int			i;
	float			z, zMinf, zMaxf, s = GrAlpha ();
	fix			zCenter;

item.faceP = faceP;
item.triP = triP;
item.bmP = bmP;
item.nVertices = nVertices;
item.nPrimitive = nPrimitive;
item.nWrap = nWrap;
item.bDepthMask = bDepthMask;
item.bAdditive = bAdditive;
item.nSegment = nSegment;
memcpy (item.texCoord, texCoord ? texCoord : tcDefault, nVertices * sizeof (tTexCoord2f));
if ((item.nColors = nColors)) {
	if (nColors < nVertices)
		nColors = 1;
	if (color) {
		memcpy (item.color, color, nColors * sizeof (tRgbaColorf));
		for (i = 0; i < nColors; i++)
			if (bAdditive)
				item.color [i].alpha = 1;
			else
				item.color [i].alpha *= s;
		}
	else if (altColor) {
		for (i = 0; i < nColors; i++) {
			item.color [i] = altColor [i].color;
			if (bAdditive)
				item.color [i].alpha = 1;
			else
				item.color [i].alpha *= s;
			}
		}
	else
		item.nColors = 0;
	}
memcpy (item.vertices, vertices, nVertices * sizeof (CFloatVector));
#if TI_SPLIT_POLYS
if (bDepthMask && transpItems.bSplitPolys) {
	for (i = 0; i < nVertices; i++)
		item.sideLength [i] = (short) (CFloatVector::Dist(vertices [i], vertices [(i + 1) % nVertices]) + 0.5f);
	return TISplitPoly (&item, 0);
	}
else
#endif
	{
#if TI_POLY_CENTER
	zCenter = 0;
#endif
	zMinf = 1e30f;
	zMaxf = -1e30f;
	for (i = 0; i < item.nVertices; i++) {
		z = item.vertices [i][Z];
		if (zMinf > z)
			zMinf = z;
		if (zMaxf < z)
			zMaxf = z;
		}
	if ((F2X (zMaxf) < transpItems.zMin) || (F2X (zMinf) > transpItems.zMax))
		return -1;
#if TI_POLY_CENTER
	zCenter = F2X ((zMinf + zMaxf) / 2);
#else
	zCenter = F2X (zMaxf);
#endif
	if (zCenter < transpItems.zMin)
		return AddTranspItem (item.bmP ? tiTexPoly : tiFlatPoly, &item, sizeof (item), transpItems.zMin, transpItems.zMin);
	if (zCenter > transpItems.zMax)
		return AddTranspItem (item.bmP ? tiTexPoly : tiFlatPoly, &item, sizeof (item), transpItems.zMax, transpItems.zMax);
	return AddTranspItem (item.bmP ? tiTexPoly : tiFlatPoly, &item, sizeof (item), zCenter, zCenter);
	}
}

//------------------------------------------------------------------------------

int TIAddFaceTris (tFace *faceP)
{
	grsTriangle	*triP;
	CFloatVector		vertices [3];
	int			h, i, j, bAdditive = FaceIsAdditive (faceP);
	CBitmap	*bmP = faceP->bTextured ? /*faceP->bmTop ? faceP->bmTop :*/ faceP->bmBot : NULL;

if (bmP)
	bmP = bmP->Override (-1);
#if DBG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	faceP = faceP;
#endif
triP = gameData.segs.faces.tris + faceP->nTriIndex;
for (h = faceP->nTris; h; h--, triP++) {
	for (i = 0, j = triP->nIndex; i < 3; i++, j++) {
#if 1
		G3TransformPoint (vertices [i], *(reinterpret_cast<CFloatVector*> (gameData.segs.faces.vertices + j)), 0);
#else
		if (gameStates.render.automap.bDisplay)
			G3TransformPoint (vertices + i, gameData.segs.fVertices + triP->index [i], 0);
		else
			VmVecFixToFloat (vertices + i, &gameData.segs.points [triP->index [i]].p3_vec);
#endif
		}
	if (!TIAddPoly (faceP, triP, bmP, vertices, 3, gameData.segs.faces.texCoord + triP->nIndex,
						 gameData.segs.faces.color + triP->nIndex,
						 NULL, 3, 1, GL_TRIANGLES, GL_REPEAT,
						 bAdditive, faceP->nSegment))
		return 0;
	}
return 1;
}

//------------------------------------------------------------------------------

int TIAddFace (tFace *faceP)
{
if (gameStates.render.bTriangleMesh)
	return TIAddFaceTris (faceP);

	CFloatVector		vertices [4];
	int			i, j;
	CBitmap	*bmP = faceP->bTextured ? /*faceP->bmTop ? faceP->bmTop :*/ faceP->bmBot : NULL;

if (bmP)
	bmP = bmP->Override (-1);
#if DBG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	faceP = faceP;
#endif
for (i = 0, j = faceP->nIndex; i < 4; i++, j++) {
#if 1
	G3TransformPoint (vertices [i], *(reinterpret_cast<CFloatVector*> (gameData.segs.faces.vertices + j)), 0);
#else
	if (gameStates.render.automap.bDisplay)
		G3TransformPoint(vertices [i], gameData.segs.fVertices [faceP->index [i]], 0);
	else
		vertices [i] = gameData.segs.points [faceP->index [i]].p3_vec.ToFloat();
#endif
	}
return TIAddPoly (faceP, NULL, bmP,
						vertices, 4, gameData.segs.faces.texCoord + faceP->nIndex,
						gameData.segs.faces.color + faceP->nIndex,
						NULL, 4, 1, GL_TRIANGLE_FAN, GL_REPEAT,
						FaceIsAdditive (faceP), faceP->nSegment) > 0;
}

//------------------------------------------------------------------------------

int TIAddSprite (CBitmap *bmP, const CFixVector& position, tRgbaColorf *color,
					  int nWidth, int nHeight, char nFrame, char bAdditive, float fSoftRad)
{
	tTranspSprite	item;
	CFixVector		vPos;

item.bmP = bmP;
if ((item.bColor = (color != NULL)))
	item.color = *color;
item.nWidth = nWidth;
item.nHeight = nHeight;
item.nFrame = nFrame;
item.bAdditive = bAdditive;
item.fSoftRad = fSoftRad;
G3TransformPoint (vPos, position, 0);
item.position = vPos.ToFloat();
return AddTranspItem (tiSprite, &item, sizeof (item), vPos [Z], vPos [Z]);
}

//------------------------------------------------------------------------------

int TIAddSpark (const CFixVector& position, char nType, int nSize, char nFrame)
{
	tTranspSpark	item;
	CFixVector		vPos;

item.nSize = nSize;
item.nFrame = nFrame;
item.nType = nType;
G3TransformPoint (vPos, position, 0);
item.position = vPos.ToFloat();
return AddTranspItem (tiSpark, &item, sizeof (item), vPos [Z], vPos [Z]);
}

//------------------------------------------------------------------------------

int TIAddSphere (tTranspSphereType nType, float red, float green, float blue, float alpha, CObject *objP)
{
	tTranspSphere	item;
	CFixVector		vPos;

item.nType = nType;
item.color.red = red;
item.color.green = green;
item.color.blue = blue;
item.color.alpha = alpha;
item.objP = objP;
G3TransformPoint(vPos, objP->info.position.vPos, 0);
return AddTranspItem (tiSphere, &item, sizeof (item), vPos [Z], vPos [Z]);
}

//------------------------------------------------------------------------------

int TIAddParticle (CParticle *particle, float fBrightness, int nThread)
{
	tTranspParticle	item;
	fix					z;

item.particle = particle;
item.fBrightness = fBrightness;
z = particle->Transform (gameStates.render.bPerPixelLighting == 2);
if (gameStates.app.bMultiThreaded && gameData.app.bUseMultiThreading [rtTransparency])
	return AddTranspItemMT (tiParticle, &item, sizeof (item), z, z, nThread);
else
	return AddTranspItem (tiParticle, &item, sizeof (item), z, z);
}

//------------------------------------------------------------------------------

int TIAddLightning (CLightning *lightningP, short nDepth)
{
	tTranspLightning	item;
	CFixVector			vPos;
	int					z;

item.lightning = lightningP;
item.nDepth = nDepth;
G3TransformPoint (vPos, lightningP->m_vPos, 0);
z = vPos [Z];
G3TransformPoint (vPos, lightningP->m_vEnd, 0);
if (z < vPos [Z])
	z = vPos [Z];
if (!AddTranspItem (tiLightning, &item, sizeof (item), z, z))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int TIAddLightTrail (CBitmap *bmP, CFloatVector *vThruster, tTexCoord2f *tcThruster, CFloatVector *vFlame, tTexCoord2f *tcFlame, tRgbaColorf *colorP)
{
	tTranspLightTrail	item;
	int					i, j;
	float					z = 0;

item.bmP = bmP;
memcpy (item.vertices, vThruster, 4 * sizeof (CFloatVector));
memcpy (item.texCoord, tcThruster, 4 * sizeof (tTexCoord2f));
item.color = *colorP;
if ((item.bTrail = (vFlame != NULL))) {
	memcpy (item.vertices + 4, vFlame, 3 * sizeof (CFloatVector));
	memcpy (item.texCoord + 4, tcFlame, 3 * sizeof (tTexCoord2f));
	j = 7;
	}
else
	j = 4;
for (i = 0; i < j; i++)
	if (z < item.vertices [i][Z])
		z = item.vertices [i][Z];
return AddTranspItem (tiThruster, &item, sizeof (item), F2X (z), F2X (z));
}

//------------------------------------------------------------------------------

void TIEnableClientState (char bClientState, char bTexCoord, char bColor, char bDecal, int nTMU)
{
glActiveTexture (nTMU);
glClientActiveTexture (nTMU);
if (!bDecal && (bColor != transpItems.bClientColor)) {
	if ((transpItems.bClientColor = bColor))
		glEnableClientState (GL_COLOR_ARRAY);
	else
		glDisableClientState (GL_COLOR_ARRAY);
	}
if (bDecal || (bTexCoord != transpItems.bClientTexCoord)) {
	if ((transpItems.bClientTexCoord = bTexCoord))
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	else
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}
if (!transpItems.bLightmaps)
	glEnableClientState (GL_NORMAL_ARRAY);
glEnableClientState (GL_VERTEX_ARRAY);
}

//------------------------------------------------------------------------------

void TIDisableClientState (int nTMU, char bDecal, char bFull)
{
#if DBG
if (nTMU == GL_TEXTURE0)
	nTMU = nTMU;
#endif
glActiveTexture (nTMU);
glClientActiveTexture (nTMU);
OglClearError (0);
if (bFull) {
	if (bDecal) {
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
		glDisableClientState (GL_COLOR_ARRAY);
			transpItems.bClientTexCoord = 0;
			transpItems.bClientColor = 0;
		}
	else {
		transpItems.bClientState = 0;
		if (transpItems.bClientTexCoord) {
			glDisableClientState (GL_TEXTURE_COORD_ARRAY);
			transpItems.bClientTexCoord = 0;
			}
		if (bDecal || transpItems.bClientColor) {
			glDisableClientState (GL_COLOR_ARRAY);
			transpItems.bClientColor = 0;
			}
		}
	glDisableClientState (GL_VERTEX_ARRAY);
	}
else {
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	}
}

//------------------------------------------------------------------------------

inline void TIResetBitmaps (void)
{
transpItems.bmP [0] =
transpItems.bmP [1] =
transpItems.bmP [2] = NULL;
transpItems.bDecal = 0;
transpItems.bTextured = 0;
transpItems.nFrame = -1;
transpItems.bUseLightmaps = 0;
}

//------------------------------------------------------------------------------

int TISetClientState (char bClientState, char bTexCoord, char bColor, char bUseLightmaps, char bDecal)
{
PROF_START
#if 1
if (transpItems.bUseLightmaps != bUseLightmaps) {
	glActiveTexture (GL_TEXTURE0);
	glClientActiveTexture (GL_TEXTURE0);
	OglClearError (0);
	if (bUseLightmaps) {
		glEnable (GL_TEXTURE_2D);
		glEnableClientState (GL_NORMAL_ARRAY);
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
		glEnableClientState (GL_COLOR_ARRAY);
		glEnableClientState (GL_VERTEX_ARRAY);
		transpItems.bClientTexCoord =
		transpItems.bClientColor = 0;
		}
	else {
		glDisableClientState (GL_NORMAL_ARRAY);
		TIDisableClientState (GL_TEXTURE1, 0, 0);
		if (transpItems.bDecal) {
			TIDisableClientState (GL_TEXTURE2, 1, 0);
			if (transpItems.bDecal == 2)
				TIDisableClientState (GL_TEXTURE3, 1, 0);
			transpItems.bDecal = 0;
			}
		transpItems.bClientTexCoord =
		transpItems.bClientColor = 1;
		}
	TIResetBitmaps ();
	transpItems.bUseLightmaps = bUseLightmaps;
	}
#endif
#if 0
if (transpItems.bClientState == bClientState) {
	if (bClientState) {
		glActiveTexture (GL_TEXTURE0 + bUseLightmaps);
		glClientActiveTexture (GL_TEXTURE0 + bUseLightmaps);
		if (transpItems.bClientColor != bColor) {
			if ((transpItems.bClientColor = bColor))
				glEnableClientState (GL_COLOR_ARRAY);
			else
				glDisableClientState (GL_COLOR_ARRAY);
			}
		if (transpItems.bClientTexCoord != bTexCoord) {
			if ((transpItems.bClientTexCoord = bTexCoord))
				glEnableClientState (GL_TEXTURE_COORD_ARRAY);
			else
				glDisableClientState (GL_TEXTURE_COORD_ARRAY);
			}
		}
	else
		glActiveTexture (GL_TEXTURE0 + bUseLightmaps);
	return 1;
	}
else
#endif
if (bClientState) {
	transpItems.bClientState = 1;
#if RENDER_TRANSP_DECALS
	if (bDecal) {
		if (bDecal == 2) {
			TIEnableClientState (bClientState, bTexCoord, 0, 1, GL_TEXTURE2 + bUseLightmaps);
			}
		TIEnableClientState (bClientState, bTexCoord, bColor, 1, GL_TEXTURE1 + bUseLightmaps);
		transpItems.bDecal = bDecal;
		}
	else /*if (transpItems.bDecal)*/ {
		if (transpItems.bDecal == 2) {
			TIDisableClientState (GL_TEXTURE2 + bUseLightmaps, 1, 1);
			OGL_BINDTEX (0);
			glDisable (GL_TEXTURE_2D);
			transpItems.bmP [2] = NULL;
			}
		TIDisableClientState (GL_TEXTURE1 + bUseLightmaps, 1, 1);
		OGL_BINDTEX (0);
		glDisable (GL_TEXTURE_2D);
		transpItems.bmP [1] = NULL;
		transpItems.bDecal = 0;
		}
#endif
	TIEnableClientState (bClientState, bTexCoord, bColor, 0, GL_TEXTURE0 + bUseLightmaps);
	}
else {
#if RENDER_TRANSP_DECALS
	if (transpItems.bDecal) {
		if (transpItems.bDecal == 2)
			TIDisableClientState (GL_TEXTURE2 + bUseLightmaps, 1, 1);
		TIDisableClientState (GL_TEXTURE1 + bUseLightmaps, 1, 1);
		transpItems.bDecal = 0;
		}
#endif
	TIDisableClientState (GL_TEXTURE0 + bUseLightmaps, 0, 1);
	glActiveTexture (GL_TEXTURE0);
	}
//transpItems.bmP = NULL;
PROF_END(ptRenderStates)
return 1;
}

//------------------------------------------------------------------------------

void TIResetShader (void)
{
if (gameStates.ogl.bShadersOk && (gameStates.render.history.nShader >= 0)) {
	gameData.render.nShaderChanges++;
	if (gameStates.render.history.nShader == 999)
		UnloadGlareShader ();
	else
		glUseProgramObject (0);
	gameStates.render.history.nShader = -1;
	}
}

//------------------------------------------------------------------------------

int LoadTranspItemImage (CBitmap *bmP, char nColors, char nFrame, int nWrap,
								 int bClientState, int nTransp, int bShader, int bUseLightmaps,
								 int bHaveDecal, int bDecal)
{
if (bmP) {
	glEnable (GL_TEXTURE_2D);
	if (bDecal || TISetClientState (bClientState, 1, nColors > 1, bUseLightmaps, bHaveDecal) || (transpItems.bTextured < 1)) {
		glActiveTexture (GL_TEXTURE0 + bUseLightmaps + bDecal);
		glClientActiveTexture (GL_TEXTURE0 + bUseLightmaps + bDecal);
		//glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		transpItems.bTextured = 1;
		}
	if (bDecal == 1)
		bmP = bmP->Override (-1);
	if ((bmP != transpItems.bmP [bDecal]) || (nFrame != transpItems.nFrame) || (nWrap != transpItems.nWrap)) {
		gameData.render.nStateChanges++;
		if (bmP) {
			if (bmP->Bind (1, nTransp)) {
				transpItems.bmP [bDecal] = NULL;
				return 0;
				}
			if (bDecal != 2)
				bmP = bmP->Override (nFrame);
			bmP->Texture ()->Wrap (nWrap);
			transpItems.nWrap = nWrap;
			transpItems.nFrame = nFrame;
			}
		else
			OGL_BINDTEX (0);
		transpItems.bmP [bDecal] = bmP;
		}
	}
else if (TISetClientState (bClientState, 0, /*!transpItems.bLightmaps &&*/ (nColors > 1), bUseLightmaps, 0) || transpItems.bTextured) {
	if (transpItems.bTextured) {
		OGL_BINDTEX (0);
		glDisable (GL_TEXTURE_2D);
		TIResetBitmaps ();
		}
	}
if (!bShader)
	TIResetShader ();
return (transpItems.bClientState == bClientState);
}

//------------------------------------------------------------------------------

inline void TISetRenderPointers (int nTMU, int nIndex, int bDecal)
{
glActiveTexture (nTMU);
glClientActiveTexture (nTMU);
if (transpItems.bTextured)
	glTexCoordPointer (2, GL_FLOAT, 0, (bDecal ? gameData.segs.faces.ovlTexCoord : gameData.segs.faces.texCoord) + nIndex);
glVertexPointer (3, GL_FLOAT, 0, gameData.segs.faces.vertices + nIndex);
}

//------------------------------------------------------------------------------

void TIRenderPoly (tTranspPoly *item)
{
PROF_START
	tFace		*faceP;
	grsTriangle	*triP;
	CBitmap		*bmBot = item->bmP, *bmTop = NULL, *mask;
	int			i, j, nIndex, bLightmaps, bDecal, bSoftBlend = 0;

#if TI_POLY_OFFSET
if (!bmBot) {
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1,1);
	glPolygonMode (GL_FRONT, GL_FILL);
	}
#endif
#if DBG
if (bmBot && strstr (bmBot->Name (), "glare.tga"))
	item = item;
#endif
#if 1
faceP = item->faceP;
triP = item->triP;
bLightmaps = transpItems.bLightmaps && (faceP != NULL);
#if DBG
if (!bLightmaps)
	bLightmaps = bLightmaps;
if (faceP) {
	if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide))) {
#if 0
		if (triP) {
			if ((triP->nIndex - faceP->nIndex) / 3 < 22)
				return;
			if ((triP->nIndex - faceP->nIndex) / 3 > 23)
				return;
			}
#endif
		nDbgSeg = nDbgSeg;
		}
	}
else
	bSoftBlend = (gameOpts->render.effects.bSoftParticles & 1) != 0;
//transpItems.bUseLightmaps = 0;
#endif
#if 0
if (transpItems.bDepthMask)
	glDepthMask (transpItems.bDepthMask = 0);
#else
if (transpItems.bDepthMask != item->bDepthMask)
	glDepthMask (transpItems.bDepthMask = item->bDepthMask);
#endif
bmTop = faceP ? faceP->bmTop->Override (-1) : NULL;
if (bmTop && !(bmTop->Flags () & (BM_FLAG_SUPER_TRANSPARENT | BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU))) {
	bmBot = bmTop;
	bmTop = mask = NULL;
	bDecal = -1;
	}
#if RENDER_TRANSP_DECALS
else {
	bDecal = bmTop != NULL;
	mask = (bDecal && ((bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0) && gameStates.render.textures.bHaveMaskShader) ? bmTop->Mask () : NULL;
	}
#else
bDecal = 0;
mask = NULL;
#endif
if (LoadTranspItemImage (bmBot, bLightmaps ? 0 : item->nColors, 0, item->nWrap, 1, 3,
	 (faceP != NULL) || bSoftBlend, bLightmaps, mask ? 2 : bDecal > 0, 0) &&
	 ((bDecal < 1) || LoadTranspItemImage (bmTop, 0, 0, item->nWrap, 1, 3, 1, bLightmaps, 0, 1)) &&
	 (!mask || LoadTranspItemImage (mask, 0, 0, item->nWrap, 1, 3, 1, bLightmaps, 0, 2))) {
	nIndex = triP ? triP->nIndex : faceP ? faceP->nIndex : 0;
	if (triP || faceP) {
		TISetRenderPointers (GL_TEXTURE0 + bLightmaps, nIndex, bDecal < 0);
		if (!bLightmaps)
			glNormalPointer (GL_FLOAT, 0, gameData.segs.faces.normals + nIndex);
		if (bDecal > 0) {
			TISetRenderPointers (GL_TEXTURE1 + bLightmaps, nIndex, 1);
			if (mask)
				TISetRenderPointers (GL_TEXTURE2 + bLightmaps, nIndex, 1);
			}
		}
	else {
		glActiveTexture (GL_TEXTURE0);
		glClientActiveTexture (GL_TEXTURE0);
		if (transpItems.bTextured)
			glTexCoordPointer (2, GL_FLOAT, 0, item->texCoord);
		glVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), item->vertices);
		}
	OglSetupTransform (faceP != NULL);
	if (item->nColors > 1) {
		glActiveTexture (GL_TEXTURE0);
		glClientActiveTexture (GL_TEXTURE0);
		glEnableClientState (GL_COLOR_ARRAY);
		if (faceP || triP)
			glColorPointer (4, GL_FLOAT, 0, gameData.segs.faces.color + nIndex);
		else
			glColorPointer (4, GL_FLOAT, 0, item->color);
		if (bLightmaps) {
			glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.lMapTexCoord + nIndex);
			glNormalPointer (GL_FLOAT, 0, gameData.segs.faces.normals + nIndex);
			glVertexPointer (3, GL_FLOAT, 0, gameData.segs.faces.vertices + nIndex);
			}
		}
	else if (item->nColors == 1)
		glColor4fv (reinterpret_cast<GLfloat*> (item->color));
	else
		glColor3d (1, 1, 1);
	i = item->bAdditive;
	glEnable (GL_BLEND);
	if (i == 1)
		glBlendFunc (GL_ONE, GL_ONE);
	else if (i == 2)
		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
	else if (i == 3)
		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if DBG
	if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (faceP && gameStates.render.bPerPixelLighting) {
		if (!faceP->bColored) {
			G3SetupGrayScaleShader ((int) faceP->nRenderType, &faceP->color);
			glDrawArrays (item->nPrimitive, 0, item->nVertices);
			}
		else {
			bool bAdditive = false;
#if 0
			if (gameData.render.lights.dynamic.headlights.nLights && !gameStates.render.automap.bDisplay) {
				G3SetupHeadlightShader (transpItems.bTextured, 1, transpItems.bTextured ? NULL : &faceP->color);
				glDrawArrays (item->nPrimitive, 0, item->nVertices);
				bAdditive = true;
				glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
				glDepthFunc (GL_LEQUAL);
				}
#endif
#if 1
#	if 0
			if (faceP)
				SetNearestFaceLights (faceP, transpItems.bTextured);
#	endif
			if (gameStates.render.bPerPixelLighting == 1) {
#	if RENDER_TRANSP_DECALS
				G3SetupLightmapShader (faceP, 0, (int) faceP->nRenderType, false);
#	else
				G3SetupLightmapShader (faceP, 0, (int) faceP->nRenderType != 0, false);
#	endif
				glDrawArrays (item->nPrimitive, 0, item->nVertices);
				}
			else {
				gameStates.ogl.iLight = 0;
				gameData.render.lights.dynamic.shader.index [0][0].nActive = -1;
				for (;;) {
					G3SetupPerPixelShader (faceP, 0, faceP->nRenderType, false);
					glDrawArrays (item->nPrimitive, 0, item->nVertices);
					if ((gameStates.ogl.iLight >= gameStates.ogl.nLights) ||
						 (gameStates.ogl.iLight >= gameStates.render.nMaxLightsPerFace))
						break;
					if (!bAdditive) {
						bAdditive = true;
						if (i)
							glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
						glDepthFunc (GL_LEQUAL);
						}
					}
				}
#endif
#if 1
			if (gameStates.render.bHeadlights) {
				if (faceP) {
					if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
						nDbgSeg = nDbgSeg;
					}
				G3SetupHeadlightShader (transpItems.bTextured, 1, transpItems.bTextured ? NULL : &faceP->color);
				if (!bAdditive) {
					bAdditive = true;
					//if (i)
						glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
					glDepthFunc (GL_LEQUAL);
					}
				glDrawArrays (item->nPrimitive, 0, item->nVertices);
				}
#endif
			if (bAdditive)
				glDepthFunc (GL_LESS);
			}
		}
	else {
		if (i && !gameStates.render.automap.bDisplay) {
			if (bSoftBlend)
				LoadGlareShader (5);
			else
				TIResetShader ();
			}
		else
			G3SetupShader (faceP, 0, 0, bDecal > 0, bmBot != NULL,
								(item->nSegment < 0) || !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [item->nSegment],
								transpItems.bTextured ? NULL : faceP ? &faceP->color : item->color);
#if 0
		if (triP)
			glNormal3fv (reinterpret_cast<GLfloat*> (gameData.segs.faces.normals + triP->nIndex));
		else if (faceP)
			glNormal3fv (reinterpret_cast<GLfloat*> (gameData.segs.faces.normals + faceP->nIndex));
#endif
		glDrawArrays (item->nPrimitive, 0, item->nVertices);
		}
	OglResetTransform (faceP != NULL);
	if (faceP)
		gameData.render.nTotalFaces++;
	}
else
#endif
if (LoadTranspItemImage (bmBot, item->nColors, 0, item->nWrap, 0, 3, 1, lightmapManager.HaveLightmaps () && (faceP != NULL), 0, 0)) {
	if (item->bAdditive == 1) {
		TIResetShader ();
		glBlendFunc (GL_ONE, GL_ONE);
		}
	else if (item->bAdditive == 2) {
		TIResetShader ();
		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
	else {
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		G3SetupShader (faceP, 0, 0, 0, bmBot != NULL,
							(item->nSegment < 0) || !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [item->nSegment],
							bmBot ? NULL : item->color);
		}
	j = item->nVertices;
	glBegin (item->nPrimitive);
	if (item->nColors > 1) {
		if (bmBot) {
			for (i = 0; i < j; i++) {
				glColor4fv (reinterpret_cast<GLfloat*> (item->color + i));
				glTexCoord2fv (reinterpret_cast<GLfloat*> (item->texCoord + i));
				glVertex3fv (reinterpret_cast<GLfloat*> (item->vertices + i));
				}
			}
		else {
			for (i = 0; i < j; i++) {
				glColor4fv (reinterpret_cast<GLfloat*> (item->color + i));
				glVertex3fv (reinterpret_cast<GLfloat*> (item->vertices + i));
				}
			}
		}
	else {
		if (item->nColors)
			glColor4fv (reinterpret_cast<GLfloat*> (item->color));
		else
			glColor3d (1, 1, 1);
		if (bmBot) {
			for (i = 0; i < j; i++) {
				glTexCoord2fv (reinterpret_cast<GLfloat*> (item->texCoord + i));
				glVertex3fv (reinterpret_cast<GLfloat*> (item->vertices + i));
				}
			}
		else {
			for (i = 0; i < j; i++) {
				glVertex3fv (reinterpret_cast<GLfloat*> (item->vertices + i));
				}
			}
		}
	glEnd ();
	}
#if TI_POLY_OFFSET
if (!bmBot) {
	glPolygonOffset (0,0);
	glDisable (GL_POLYGON_OFFSET_FILL);
	}
#endif
if (item->bAdditive)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
if (bSoftBlend)
	glEnable (GL_DEPTH_TEST);
PROF_END(ptRenderFaces)
}

//------------------------------------------------------------------------------

void TIRenderObject (tTranspObject *item)
{
//SEM_LEAVE (SEM_LIGHTNINGS)	//might lockup otherwise when creating damage lightnings on cloaked objects
//SEM_LEAVE (SEM_SPARKS)
DrawPolygonObject (item->objP, 0, 1);
glDisable (GL_TEXTURE_2D);
transpItems.bTextured = 0;
transpItems.bClientState = 0;
transpItems.bClientTexCoord = 0;
transpItems.bClientColor = 0;
//SEM_ENTER (SEM_LIGHTNINGS)
//SEM_ENTER (SEM_SPARKS)
}

//------------------------------------------------------------------------------

void TIRenderSprite (tTranspSprite *item)
{
	int bSoftBlend = ((gameOpts->render.effects.bSoftParticles & 1) != 0) && (item->fSoftRad > 0);

if (LoadTranspItemImage (item->bmP, item->bColor, item->nFrame, GL_CLAMP, 0, 1,
								 bSoftBlend, 0, 0, 0)) {
	float		h, w, u, v;
	CFloatVector	fPos = item->position;

	w = (float) X2F (item->nWidth);
	h = (float) X2F (item->nHeight);
	u = item->bmP->Texture ()->U ();
	v = item->bmP->Texture ()->V ();
	if (item->bColor)
		glColor4fv (reinterpret_cast<GLfloat*> (&item->color));
	else
		glColor3f (1, 1, 1);
	if (item->bAdditive == 2)
		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	else if (item->bAdditive == 1)
		glBlendFunc (GL_ONE, GL_ONE);
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (bSoftBlend)
		LoadGlareShader (item->fSoftRad);
	else if (transpItems.bDepthMask)
		glDepthMask (transpItems.bDepthMask = 0); 
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	fPos [X] -= w;
	fPos [Y] += h;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	glTexCoord2f (u, 0);
	fPos [X] += 2 * w;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	glTexCoord2f (u, v);
	fPos [Y] -= 2 * h;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	glTexCoord2f (0, v);
	fPos [X] -= 2 * w;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	glEnd ();
	if (item->bAdditive)
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (bSoftBlend)
		glEnable (GL_DEPTH_TEST);
	}
}

//------------------------------------------------------------------------------

typedef struct tSparkVertex {
	fVector3		vPos;
	tTexCoord2f	texCoord;
} tSparkVertex;

#define SPARK_BUF_SIZE	1000

typedef struct tSparkBuffer {
	int				nSparks;
	tSparkVertex	info [SPARK_BUF_SIZE * 4];
} tSparkBuffer;

tSparkBuffer sparkBuffer;

//------------------------------------------------------------------------------

void TIFlushSparkBuffer (void)
{
	int bSoftSparks = (gameOpts->render.effects.bSoftParticles & 2) != 0;

if (sparkBuffer.nSparks && LoadTranspItemImage (bmpSparks, 0, 0, GL_CLAMP, 1, 1, bSoftSparks, 0, 0, 0)) {
	G3EnableClientStates (1, 0, 0, GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	bmpSparks->Texture ()->Bind ();
	if (bSoftSparks)
		LoadGlareShader (3);
	else {
		TIResetShader ();
		if (transpItems.bDepthMask)
			glDepthMask (transpItems.bDepthMask = 0);
		}
	glBlendFunc (GL_ONE, GL_ONE);
	glColor3f (1, 1, 1);
	glTexCoordPointer (2, GL_FLOAT, sizeof (tSparkVertex), &sparkBuffer.info [0].texCoord);
	glVertexPointer (3, GL_FLOAT, sizeof (tSparkVertex), &sparkBuffer.info [0].vPos);
	glDrawArrays (GL_QUADS, 0, 4 * sparkBuffer.nSparks);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (bSoftSparks)
		glEnable (GL_DEPTH_TEST);
	transpItems.bClientColor = 0;
	sparkBuffer.nSparks = 0;
	}
}

//------------------------------------------------------------------------------

void TIRenderSpark (tTranspSpark *item)
{
if (sparkBuffer.nSparks >= SPARK_BUF_SIZE)
	TIFlushSparkBuffer ();

	tSparkVertex	*infoP = sparkBuffer.info + 4 * sparkBuffer.nSparks;
	CFloatVector			vPos = item->position;
	float				nSize = X2F (item->nSize);
	float				nCol = (float) (item->nFrame / 8);
	float				nRow = (float) (item->nFrame % 8);

if (!item->nType)
	nCol += 4;
infoP->vPos [X] = vPos [X] - nSize;
infoP->vPos [Y] = vPos [Y] + nSize;
infoP->vPos [Z] = vPos [Z];
infoP->texCoord.v.u = nCol / 8.0f;
infoP->texCoord.v.v = (nRow + 1) / 8.0f;
infoP++;
infoP->vPos [X] = vPos [X] + nSize;
infoP->vPos [Y] = vPos [Y] + nSize;
infoP->vPos [Z] = vPos [Z];
infoP->texCoord.v.u = (nCol + 1) / 8.0f;
infoP->texCoord.v.v = (nRow + 1) / 8.0f;
infoP++;
infoP->vPos [X] = vPos [X] + nSize;
infoP->vPos [Y] = vPos [Y] - nSize;
infoP->vPos [Z] = vPos [Z];
infoP->texCoord.v.u = (nCol + 1) / 8.0f;
infoP->texCoord.v.v = nRow / 8.0f;
infoP++;
infoP->vPos [X] = vPos [X] - nSize;
infoP->vPos [Y] = vPos [Y] - nSize;
infoP->vPos [Z] = vPos [Z];
infoP->texCoord.v.u = nCol / 8.0f;
infoP->texCoord.v.v = nRow / 8.0f;
sparkBuffer.nSparks++;
}

//------------------------------------------------------------------------------

void TIRenderSphere (tTranspSphere *item)
{
	int bDepthSort = gameOpts->render.bDepthSort;

gameOpts->render.bDepthSort = -1;
TISetClientState (0, 0, 0, 0, 0);
TIResetShader ();
if (item->nType == riSphereShield)
	DrawShieldSphere (item->objP, item->color.red, item->color.green, item->color.blue, item->color.alpha);
if (item->nType == riMonsterball)
	DrawMonsterball (item->objP, item->color.red, item->color.green, item->color.blue, item->color.alpha);
TIResetBitmaps ();
glActiveTexture (GL_TEXTURE0 + transpItems.bLightmaps);
glClientActiveTexture (GL_TEXTURE0 + transpItems.bLightmaps);
OGL_BINDTEX (0);
glDisable (GL_TEXTURE_2D);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDepthMask (transpItems.bDepthMask = 0);
glEnable (GL_BLEND);
gameOpts->render.bDepthSort = bDepthSort;
}

//------------------------------------------------------------------------------

void TIRenderBullet (CParticle *pParticle)
{
	CObject	o;

memset (&o, 0, sizeof (o));
o.info.nType = OBJ_POWERUP;
o.info.position.vPos = pParticle->m_vPos;
o.info.position.mOrient = pParticle->m_mOrient;
if (0 <= (o.info.nSegment = FindSegByPos (o.info.position.vPos, pParticle->m_nSegment, 0, 0))) {
	gameData.render.lights.dynamic.shader.index [0][0].nActive = 0;
	o.info.renderType = RT_POLYOBJ;
	o.rType.polyObjInfo.nModel = BULLET_MODEL;
	o.rType.polyObjInfo.nTexOverride = -1;
	DrawPolygonObject (&o, 0, 1);
	glDisable (GL_TEXTURE_2D);
	transpItems.bTextured = 0;
	transpItems.bClientState = 0;
	gameData.models.vScale.SetZero ();
	}
}

//------------------------------------------------------------------------------

void TIRenderParticle (tTranspParticle *item)
{
if (item->particle->m_nType == 2)
	TIRenderBullet (item->particle);
else {
	int bSoftSmoke = (gameOpts->render.effects.bSoftParticles & 4) != 0;

	TISetClientState (0, 0, 0, 0, 0);
	if (!bSoftSmoke || (gameStates.render.history.nShader != 999))
		TIResetShader ();
	if (transpItems.nPrevType != tiParticle) {
		glEnable (GL_TEXTURE_2D);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		particleManager.SetLastType (-1);
		transpItems.bTextured = 1;
		//InitParticleBuffer (transpItems.bLightmaps);
		}
	item->particle->Render (item->fBrightness);
	TIResetBitmaps ();
	transpItems.bDepthMask = 1;
	}
}

//------------------------------------------------------------------------------

void TIRenderLightning (tTranspLightning *item)
{
if (transpItems.nPrevType != transpItems.nCurType) {
	if (transpItems.bDepthMask)
		glDepthMask (transpItems.bDepthMask = 0);
	TIDisableClientState (GL_TEXTURE2, 1, 0);
	TISetClientState (1, 0, 0, 0, 0);
	TIResetShader ();
	}
item->lightning->Render (item->nDepth, 0, 0);
TISetClientState (0, 0, 0, 0, 0);
TIResetBitmaps ();
transpItems.bDepthMask = 1;
}

//------------------------------------------------------------------------------

void TIRenderLightTrail (tTranspLightTrail *item)
{
if (!transpItems.bDepthMask)
	glDepthMask (transpItems.bDepthMask = 1);
glEnable (GL_BLEND);
glBlendFunc (GL_ONE, GL_ONE);
glDisable (GL_CULL_FACE);
glColor4fv (reinterpret_cast<GLfloat*> (&item->color));
#if 1
if (LoadTranspItemImage (item->bmP, 1, 0, GL_CLAMP, 1, 1, 0, 0, 0, 0)) {
	glVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), item->vertices);
	glTexCoordPointer (2, GL_FLOAT, 0, item->texCoord);
	if (item->bTrail)
		glDrawArrays (GL_TRIANGLES, 4, 3);
	glDrawArrays (GL_QUADS, 0, 4);
	}
else
#endif
if (LoadTranspItemImage (item->bmP, 0, 0, GL_CLAMP, 0, 1, 0, 0, 0, 0)) {
	int i;
	if (item->bTrail) {
		glBegin (GL_TRIANGLES);
		for (i = 0; i < 3; i++) {
			glTexCoord2fv (reinterpret_cast<GLfloat*> (item->texCoord + 4 + i));
			glVertex3fv (reinterpret_cast<GLfloat*> (item->vertices + 4 + i));
			}
		glEnd ();
		}
	glBegin (GL_QUADS);
	for (i = 0; i < 4; i++) {
		glTexCoord2fv (reinterpret_cast<GLfloat*> (item->texCoord + i));
		glVertex3fv (reinterpret_cast<GLfloat*> (item->vertices + i));
		}
	glEnd ();
	}
glEnable (GL_CULL_FACE);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//------------------------------------------------------------------------------

void TIFlushParticleBuffer (int nType)
{
if ((nType < 0) || ((nType != tiParticle) && (particleManager.LastType () >= 0))) {
	particleManager.FlushBuffer (-1.0f);
	if (nType < 0)
		particleManager.CloseBuffer ();
#if 1
	TIResetBitmaps ();
#endif
	particleManager.SetLastType (-1);
	transpItems.bClientColor = 1;
	transpItems.bUseLightmaps = 0;
	}
}

//------------------------------------------------------------------------------

static inline void TIFlushBuffers (int nType)
{
if (nType != tiSpark)
	TIFlushSparkBuffer ();
TIFlushParticleBuffer (nType);
}

//------------------------------------------------------------------------------

tTranspItem *TISetParent (int nChild, int nParent)
{
if (nChild < 0)
	return NULL;
tTranspItem	*childP = transpItems.itemListP + nChild;
if (!childP->bValid)
	return NULL;
if (nParent < 0)
	return NULL;
tTranspItem	*parentP = transpItems.itemListP + nParent;
if (!parentP->bValid)
	return NULL;
childP->parentP = parentP;
return parentP;
}

//------------------------------------------------------------------------------

int RenderTranspItem (struct tTranspItem *pl)
{
if (!pl->bRendered) {
	pl->bRendered = true;
	transpItems.nPrevType = transpItems.nCurType;
	transpItems.nCurType = pl->nType;
	TIFlushBuffers (transpItems.nCurType);
	if ((transpItems.nCurType == tiTexPoly) || (transpItems.nCurType == tiFlatPoly)) {
		TIRenderPoly (&pl->item.poly);
		}
	else if (transpItems.nCurType == tiObject) {
		TIRenderObject (&pl->item.object);
		}
	else if (transpItems.nCurType == tiSprite) {
		TIRenderSprite (&pl->item.sprite);
		}
	else if (transpItems.nCurType == tiSpark) {
		TIRenderSpark (&pl->item.spark);
		}
	else if (transpItems.nCurType == tiSphere) {
		TIRenderSphere (&pl->item.sphere);
		}
	else if (transpItems.nCurType == tiParticle) {
		if (transpItems.bHaveParticles)
			TIRenderParticle (&pl->item.particle);
		}
	else if (transpItems.nCurType == tiLightning) {
		TIRenderLightning (&pl->item.lightning);
		}
	else if (transpItems.nCurType == tiThruster) {
		TIRenderLightTrail (&pl->item.thruster);
		}
	if (pl->parentP)
		RenderTranspItem (pl->parentP);
	}
return transpItems.nCurType;
}

//------------------------------------------------------------------------------

extern int bLog;

void RenderTranspItems (void)
{
#if RENDER_TRANSPARENCY
	struct tTranspItem	**pd, *pl, *pn;
	int						nDepth, nItems, bStencil;

if (!(gameOpts->render.bDepthSort && transpItems.depthBufP && (transpItems.nFreeItems < ITEM_BUFFER_SIZE))) {
	return;
	}
PROF_START
gameStates.render.nType = 5;
OglSetLibFlags (1);
TIResetShader ();
bStencil = StencilOff ();
transpItems.bTextured = -1;
transpItems.bClientState = -1;
transpItems.bClientTexCoord = 0;
transpItems.bClientColor = 0;
transpItems.bDepthMask = 0;
transpItems.bUseLightmaps = 0;
transpItems.bDecal = 0;
transpItems.bLightmaps = lightmapManager.HaveLightmaps ();
transpItems.bSplitPolys = (gameStates.render.bPerPixelLighting != 2) && (gameStates.render.bSplitPolys > 0);
transpItems.nWrap = 0;
transpItems.nFrame = -1;
transpItems.bmP [0] =
transpItems.bmP [1] = NULL;
sparkBuffer.nSparks = 0;
OglDisableLighting ();
G3DisableClientStates (1, 1, 0, GL_TEXTURE2 + transpItems.bLightmaps);
G3DisableClientStates (1, 1, 0, GL_TEXTURE1 + transpItems.bLightmaps);
G3DisableClientStates (1, 1, 0, GL_TEXTURE0 + transpItems.bLightmaps);
G3DisableClientStates (1, 1, 0, GL_TEXTURE0);
pl = transpItems.itemListP + ITEM_BUFFER_SIZE - 1;
transpItems.bHaveParticles = particleImageManager.LoadAll ();
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDepthFunc (GL_LEQUAL);
glDepthMask (0);
glEnable (GL_CULL_FACE);
particleManager.BeginRender (-1, 1);
transpItems.nCurType = -1;
for (pd = transpItems.depthBufP + transpItems.nMaxOffs /*ITEM_DEPTHBUFFER_SIZE - 1*/, nItems = transpItems.nItems;
	  (pd >= transpItems.depthBufP) && nItems;
	  pd--) {
	if ((pl = *pd)) {
		nDepth = 0;
		do {
#if DBG
			if (pl->nItem == nDbgItem)
				nDbgItem = nDbgItem;
#endif
			nItems--;
			RenderTranspItem (pl);
			pn = pl->pNextItem;
			pl->pNextItem = NULL;
			pl = pn;
			nDepth++;
			} while (pl);
		*pd = NULL;
		}
	}
TIFlushBuffers (-1);
particleManager.EndRender ();
TIResetShader ();
G3DisableClientStates (1, 1, 1, GL_TEXTURE0);
OGL_BINDTEX (0);
G3DisableClientStates (1, 1, 1, GL_TEXTURE1);
OGL_BINDTEX (0);
G3DisableClientStates (1, 1, 1, GL_TEXTURE2);
OGL_BINDTEX (0);
G3DisableClientStates (1, 1, 1, GL_TEXTURE3);
OGL_BINDTEX (0);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDepthFunc (GL_LEQUAL);
glDepthMask (1);
StencilOn (bStencil);
transpItems.nMinOffs = ITEM_DEPTHBUFFER_SIZE;
transpItems.nMaxOffs = 0;
transpItems.nFreeItems = ITEM_BUFFER_SIZE;
PROF_END(ptTranspPolys)
#endif
}

//------------------------------------------------------------------------------
//eof
