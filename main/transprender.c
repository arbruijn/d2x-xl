/* $Id: render.c, v 1.18 2003/10/10 09:36:35 btb Exp $ */
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "inferno.h"
#include "u_mem.h"
#include "ogl_lib.h"
#include "ogl_fastrender.h"
#include "render.h"
#include "renderlib.h"
#include "objrender.h"
#include "lightning.h"
#include "sphere.h"
#include "network.h"
#include "transprender.h"
#include "renderthreads.h"

#define RI_SPLIT_POLYS 1
#define RI_POLY_OFFSET 0
#define RI_POLY_CENTER 1

//------------------------------------------------------------------------------

tRenderItemBuffer	renderItems;

static tTexCoord2f tcDefault [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

inline int AllocRenderItems (void)
{
if (renderItems.pDepthBuffer)
	return 1;
if (!(renderItems.pDepthBuffer = (struct tRenderItem **) D2_ALLOC (ITEM_DEPTHBUFFER_SIZE * sizeof (struct tRenderItem *))))
	return 0;
if (!(renderItems.pItemList = (struct tRenderItem *) D2_ALLOC (ITEM_BUFFER_SIZE * sizeof (struct tRenderItem)))) {
	D2_FREE (renderItems.pDepthBuffer);
	return 0;
	}
renderItems.nFreeItems = 0;
ResetRenderItemBuffer ();
return 1;
}

//------------------------------------------------------------------------------

void FreeRenderItems (void)
{
D2_FREE (renderItems.pItemList);
D2_FREE (renderItems.pDepthBuffer);
}

//------------------------------------------------------------------------------

void ResetRenderItemBuffer (void)
{
memset (renderItems.pDepthBuffer, 0, ITEM_DEPTHBUFFER_SIZE * sizeof (struct tRenderItem **));
memset (renderItems.pItemList, 0, (ITEM_BUFFER_SIZE - renderItems.nFreeItems) * sizeof (struct tRenderItem));
renderItems.nFreeItems = ITEM_BUFFER_SIZE;
}


//------------------------------------------------------------------------------

void InitRenderItemBuffer (int zMin, int zMax)
{
renderItems.zMin = 0;
renderItems.zMax = zMax - renderItems.zMin;
renderItems.zScale = (double) (ITEM_DEPTHBUFFER_SIZE - 1) / (double) (zMax - renderItems.zMin);
if (renderItems.zScale < 0)
	renderItems.zScale = 1;
else if (renderItems.zScale > 1)
	renderItems.zScale = 1;
}

//------------------------------------------------------------------------------

int AddRenderItem (tRenderItemType nType, void *itemData, int itemSize, int nDepth, int nIndex)
{
	tRenderItem *ph, *pi, *pj, **pd;
	int			nOffset;

if ((nDepth < renderItems.zMin) || (nDepth > renderItems.zMax))
	return renderItems.nFreeItems;
AllocRenderItems ();
if (!renderItems.nFreeItems)
	return 0;
#if 1
	nOffset = (int) ((double) (nDepth - renderItems.zMin) * renderItems.zScale);
#else
if (nIndex < renderItems.zMin)
	nOffset = 0;
else
	nOffset = (int) ((double) (nIndex - renderItems.zMin) * renderItems.zScale);
#endif
if (nOffset >= ITEM_DEPTHBUFFER_SIZE)
	return 0;
pd = renderItems.pDepthBuffer + nOffset;
// find the first particle to insert the new one *before* and place in pj; pi will be it's predecessor (NULL if to insert at list start)
ph = renderItems.pItemList + --renderItems.nFreeItems;
ph->nType = nType;
ph->z = nDepth;
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
return renderItems.nFreeItems;
}

//------------------------------------------------------------------------------

int AddRenderItemMT (tRenderItemType nType, void *itemData, int itemSize, int nDepth, int nIndex, int nThread)
{
if (!gameStates.app.bMultiThreaded || (nThread < 0))
	return AddRenderItem (nType, itemData, itemSize, nDepth, nIndex);
while (tiRenderItems.ti [nThread].bExec)
	G3_SLEEP (0);
tiRenderItems.itemData [nThread].nType = nType;
tiRenderItems.itemData [nThread].nSize = itemSize;
tiRenderItems.itemData [nThread].nDepth = nDepth;
tiRenderItems.itemData [nThread].nIndex = nIndex;
memcpy (&tiRenderItems.itemData [nThread].item, itemData, itemSize);
tiRenderItems.ti [nThread].bExec = 1;
return 1;
}

//------------------------------------------------------------------------------

int RISplitPoly (tRIPoly *item, int nDepth)
{
	tRIPoly		split [2];
	fVector		vSplit;
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
		z = split [0].vertices [i].p.z;
		if (zMax < z)
			zMax = z;
		if (zMin > z)
			zMin = z;
		}
#if RI_POLY_CENTER
	return AddRenderItem (item->bmP ? riTexPoly : riFlatPoly, item, sizeof (*item), fl2f (zMax), fl2f ((zMax + zMin) / 2));
#else
	return AddRenderItem (item->bmP ? riTexPoly : riFlatPoly, item, sizeof (*item), fl2f (zMax), fl2f (zMin));
#endif
	}
if (split [0].nVertices == 3) {
	i1 = (i0 + 1) % 3;
	split [0].vertices [i0] =
	split [1].vertices [i1] = *VmVecAvgf (&vSplit, split [0].vertices + i0, split [0].vertices + i1);
	split [0].sideLength [i0] =
	split [1].sideLength [i0] = nMaxLen / 2;
	if (split [0].bmP) {
		split [0].texCoord [i0].v.u =
		split [1].texCoord [i1].v.u = (split [0].texCoord [i1].v.u + split [0].texCoord [i0].v.u) / 2;
		split [0].texCoord [i0].v.v =
		split [1].texCoord [i1].v.v = (split [0].texCoord [i1].v.v + split [0].texCoord [i0].v.v) / 2;
		}
	if (split [0].nColors == 3) {
		for (i = 4, c = (float *) &color, c0 = (float *) (split [0].color + i0), c1 = (float *) (split [0].color + i1); i; i--)
			*c++ = (*c0++ + *c1++) / 2;
		split [0].color [i0] =
		split [1].color [i1] = color;
		}
	}
else {
	i1 = (i0 + 1) % 4;
	i2 = (i0 + 2) % 4;
	i3 = (i1 + 2) % 4;
	split [0].vertices [i1] =
	split [1].vertices [i0] = *VmVecAvgf (&vSplit, split [0].vertices + i0, split [0].vertices + i1);
	split [0].vertices [i2] =
	split [1].vertices [i3] = *VmVecAvgf (&vSplit, split [0].vertices + i2, split [0].vertices + i3);
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
		for (i = 4, c = (float *) &color, c0 = (float *) (split [0].color + i0), c1 = (float *) (split [0].color + i1); i; i--)
			*c++ = (*c0++ + *c1++) / 2;
		split [0].color [i1] =
		split [1].color [i0] = color;
		for (i = 4, c = (float *) &color, c0 = (float *) (split [0].color + i2), c1 = (float *) (split [0].color + i3); i; i--)
			*c++ = (*c0++ + *c1++) / 2;
		split [0].color [i2] =
		split [1].color [i3] = color;
		}
	split [0].sideLength [i0] =
	split [1].sideLength [i0] =
	split [0].sideLength [i2] =
	split [1].sideLength [i2] = nMaxLen / 2;
	}
return RISplitPoly (split, nDepth + 1) && RISplitPoly (split + 1, nDepth + 1);
}

//------------------------------------------------------------------------------

int RIAddObject (tObject *objP)
{
	tRIObject	item;
	vmsVector	vPos;

if (objP->nType == 255)
	return 0;
item.objP = objP;
G3TransformPoint (&vPos, &OBJPOS (objP)->vPos, 0);
return AddRenderItem (riObject, &item, sizeof (item), vPos.p.z, vPos.p.z);
}

//------------------------------------------------------------------------------

int RIAddPoly (grsBitmap *bmP, fVector *vertices, char nVertices, tTexCoord2f *texCoord, tRgbaColorf *color, 
					tFaceColor *altColor, char nColors, char bDepthMask, int nPrimitive, int nWrap, int bAdditive)
{
	tRIPoly	item;
	int		i;
	float		z, zMin, zMax, s = GrAlpha ();
#if RI_POLY_CENTER
	float		zCenter;
#endif

item.bmP = bmP;
item.nVertices = nVertices;
item.nPrimitive = nPrimitive;
item.nWrap = nWrap;
item.bDepthMask = bDepthMask;
item.bAdditive = bAdditive;
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
memcpy (item.vertices, vertices, nVertices * sizeof (fVector));
#if RI_SPLIT_POLYS
if (bDepthMask && gameStates.render.bSplitPolys) {
	for (i = 0; i < nVertices; i++)
		item.sideLength [i] = (short) (VmVecDistf (vertices + i, vertices + (i + 1) % nVertices) + 0.5f);
	return RISplitPoly (&item, 0);
	}
else 
#endif
	{
#if RI_POLY_CENTER
	zCenter = 0;
	zMin = 1e30f;
	zMax = -1e30f;
#endif
	for (i = 0; i < item.nVertices; i++) {
		z = item.vertices [i].p.z;
#if RI_POLY_CENTER
		zCenter += z;
#endif
		if (zMin > z)
			zMin = z;
		if (zMax < z)
			zMax = z;
		}
	if ((zMax < renderItems.zMin) || (zMin > renderItems.zMax))
		return 0;
#if RI_POLY_CENTER
	zCenter /= item.nVertices;
	if (zCenter < zMin)
		return AddRenderItem (item.bmP ? riTexPoly : riFlatPoly, &item, sizeof (item), fl2f (zMin), fl2f (zMin));
	if (zCenter < zMax)
		return AddRenderItem (item.bmP ? riTexPoly : riFlatPoly, &item, sizeof (item), fl2f (zMax), fl2f (zMax));
	return AddRenderItem (item.bmP ? riTexPoly : riFlatPoly, &item, sizeof (item), fl2f (zCenter), fl2f (zCenter));
#else
	return AddRenderItem (item.bmP ? riTexPoly : riFlatPoly, &item, sizeof (item), fl2f (zMin), fl2f (zMin));
#endif
	}
}

//------------------------------------------------------------------------------

int RIAddFace (grsFace *faceP)
{
	fVector		vertices [4];
	int			i, j;
	grsBitmap	*bmP = BmOverride (faceP->bmBot, -1);

#ifdef _DEBUG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	faceP = faceP;
#endif
for (i = 0, j = faceP->nIndex; i < 4; i++, j++) {
	if (gameStates.render.automap.bDisplay)
		G3TransformPointf (vertices + i, gameData.segs.fVertices + faceP->index [i], 0);
	else
		VmVecFixToFloat (vertices + i, &gameData.segs.points [faceP->index [i]].p3_vec);
	}
return RIAddPoly (faceP->bTextured ? bmP : NULL, vertices, 4, gameData.segs.faces.texCoord + faceP->nIndex, 
						gameData.segs.faces.color + faceP->nIndex,
						NULL, 4, 1, GL_TRIANGLE_FAN, GL_REPEAT, 
						FaceIsAdditive (faceP));
}

//------------------------------------------------------------------------------

int RIAddSprite (grsBitmap *bmP, vmsVector *position, tRgbaColorf *color, int nWidth, int nHeight, char nFrame, char bAdditive)
{
	tRISprite	item;
	vmsVector	vPos;

item.bmP = bmP;
if ((item.bColor = (color != NULL)))
	item.color = *color;
item.nWidth = nWidth;
item.nHeight = nHeight;
item.nFrame = nFrame;
item.bAdditive = bAdditive;
G3TransformPoint (&vPos, position, 0);
VmVecFixToFloat (&item.position, &vPos);
return AddRenderItem (riSprite, &item, sizeof (item), vPos.p.z, vPos.p.z);
}

//------------------------------------------------------------------------------

int RIAddSphere (tRISphereType nType, float red, float green, float blue, float alpha, tObject *objP)
{
	tRISphere	item;
	vmsVector	vPos;

item.nType = nType;
item.color.red = red;
item.color.green = green;
item.color.blue = blue;
item.color.alpha = alpha;
item.objP = objP;
G3TransformPoint (&vPos, &objP->position.vPos, 0);
return AddRenderItem (riSphere, &item, sizeof (item), vPos.p.z, vPos.p.z);
}

//------------------------------------------------------------------------------

int RIAddParticle (tParticle *particle, float fBrightness, int nThread)
{
	tRIParticle	item;

item.particle = particle;
item.fBrightness = fBrightness;
G3TransformPoint (&particle->transPos, &particle->pos, 0);
return AddRenderItemMT (riParticle, &item, sizeof (item), particle->transPos.p.z, particle->transPos.p.z, nThread);
}

//------------------------------------------------------------------------------

int RIAddLightnings (tLightning *lightnings, short nLightnings, short nDepth)
{
	tRILightning item;
	vmsVector vPos;
	int z;

if (nLightnings < 1)
	return 0;
item.lightning = lightnings;
item.nLightnings = nLightnings;
item.nDepth = nDepth;
for (; nLightnings; nLightnings--, lightnings++) {
	G3TransformPoint (&vPos, &lightnings->vPos, 0);
	z = vPos.p.z;
	G3TransformPoint (&vPos, &lightnings->vEnd, 0);
	if (z < vPos.p.z)
		z = vPos.p.z;
	}
if (!AddRenderItem (riLightning, &item, sizeof (item), z, z))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int RIAddLightningSegment (fVector *vLine, fVector *vPlasma, tRgbaColorf *color, char bPlasma, char bStart, char bEnd, short nDepth)
{
	tRILightningSegment	item;
	fix z;

memcpy (item.vLine, vLine, 2 * sizeof (fVector));
if ((item.bPlasma = bPlasma))
	memcpy (item.vPlasma, vPlasma, 4 * sizeof (fVector));
memcpy (&item.color, color, sizeof (tRgbaColorf));
item.bStart = bStart;
item.bEnd = bEnd;
item.nDepth = nDepth;
z = fl2f ((item.vLine [0].p.z + item.vLine [1].p.z) / 2);
return AddRenderItem (riLightningSegment, &item, sizeof (item), z, z);
}

//------------------------------------------------------------------------------

int RIAddThruster (grsBitmap *bmP, fVector *vThruster, tTexCoord2f *tcThruster, fVector *vFlame, tTexCoord2f *tcFlame)
{
	tRIThruster	item;
	int			i, j;
	float			z = 0;

item.bmP = bmP;
memcpy (item.vertices, vThruster, 4 * sizeof (fVector));
memcpy (item.texCoord, tcThruster, 4 * sizeof (tTexCoord2f));
if ((item.bFlame = (vFlame != NULL))) {
	memcpy (item.vertices + 4, vFlame, 3 * sizeof (fVector));
	memcpy (item.texCoord + 4, tcFlame, 3 * sizeof (tTexCoord2f));
	j = 7;
	}
else 
	j = 4;
for (i = 0; i < j; i++)
	if (z < item.vertices [i].p.z)
		z = item.vertices [i].p.z;
return AddRenderItem (riThruster, &item, sizeof (item), fl2f (z), fl2f (z));
}

//------------------------------------------------------------------------------

int RISetClientState (char bClientState, char bTexCoord, char bColor)
{
if (renderItems.bClientState == bClientState) {
	if (bClientState) {
		glClientActiveTexture (GL_TEXTURE0);
		if (renderItems.bClientTexCoord != bTexCoord) {
			if ((renderItems.bClientTexCoord = bTexCoord))
				glEnableClientState (GL_TEXTURE_COORD_ARRAY);
			else
				glDisableClientState (GL_TEXTURE_COORD_ARRAY);
			}
		if (renderItems.bClientColor != bColor) {
			if ((renderItems.bClientColor = bColor))
				glEnableClientState (GL_COLOR_ARRAY);
			else
				glDisableClientState (GL_COLOR_ARRAY);
			}
		}
	return 1;
	}
else if (bClientState) {
	renderItems.bClientState = 1;
	glClientActiveTexture (GL_TEXTURE0);
	if (!G3EnableClientState (GL_VERTEX_ARRAY, -1))
		return 0;
	if (bTexCoord) {
		if (G3EnableClientState (GL_TEXTURE_COORD_ARRAY, -1))
			renderItems.bClientTexCoord = 1;
		else {
			RISetClientState (0, 0, 0);
			return 0;
			}
		}
	else
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	if (bColor) {
		if (G3EnableClientState (GL_COLOR_ARRAY, -1))
			renderItems.bClientColor = 1;
		else {
			RISetClientState (0, 0, 0);
			return 0;
			}
		}
	else
		glDisableClientState (GL_COLOR_ARRAY);
	}
else {
	glActiveTexture (GL_TEXTURE0);
	glClientActiveTexture (GL_TEXTURE0);
	if (renderItems.bClientTexCoord) {
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
		renderItems.bClientTexCoord = 0;
		}
	if (renderItems.bClientColor) {
		glDisableClientState (GL_COLOR_ARRAY);
		renderItems.bClientColor = 0;
		}
	glDisableClientState (GL_VERTEX_ARRAY);
	renderItems.bClientState = 0;
	}
renderItems.bmP = NULL;
return 1;
}

//------------------------------------------------------------------------------

void RIResetShader (void)
{
if (gameStates.ogl.bShadersOk && (gameStates.render.history.nShader >= 0)) {
	glUseProgramObject (0);
	gameStates.render.history.nShader = -1;
	}
}

//------------------------------------------------------------------------------

int LoadRenderItemImage (grsBitmap *bmP, char nColors, char nFrame, int nWrap, int bClientState, int nTransp, int bShader)
{
if (bmP) {
	if (RISetClientState (bClientState, 1, nColors > 1) || (renderItems.bTextured < 1)) {
		glEnable (GL_TEXTURE_2D);
		//glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		renderItems.bTextured = 1;
		}
	if ((bmP != renderItems.bmP) || (nFrame != renderItems.nFrame) || (nWrap != renderItems.nWrap)) {
		if (bmP) {
			if (OglBindBmTex (bmP, 1, nTransp)) {
				renderItems.bmP = NULL;
				return 0;
				}
			bmP = BmOverride (bmP, nFrame);
			OglTexWrap (bmP->glTexture, nWrap);
			renderItems.bmP = bmP;
			renderItems.nWrap = nWrap;
			}
		else
			OGL_BINDTEX (0);
		}
	}
else if (RISetClientState (bClientState, 0, nColors > 1) || renderItems.bTextured) {
	glDisable (GL_TEXTURE_2D);
	renderItems.bTextured = 0;
	}
if (!bShader)
	RIResetShader ();
return (renderItems.bClientState == bClientState);
}

//------------------------------------------------------------------------------

void RIRenderPoly (tRIPoly *item)
{
	int	i, j;

if (renderItems.bDepthMask != item->bDepthMask)
	glDepthMask (renderItems.bDepthMask = item->bDepthMask);
#if RI_POLY_OFFSET
if (!item->bmP) {
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1,1);
	glPolygonMode (GL_FRONT, GL_FILL);
	}
#endif
#ifdef _DEBUG
if (item->bmP && strstr (item->bmP->szName, "door45#5"))
	item = item;
#endif
#if 1
if (LoadRenderItemImage (item->bmP, item->nColors, 0, item->nWrap, 1, 3, 1)) {
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), item->vertices);
	if (renderItems.bTextured)
		glTexCoordPointer (2, GL_FLOAT, 0, item->texCoord);
	if (item->nColors == 1)
		glColor4fv ((GLfloat *) item->color);
	else if (item->nColors > 1)
		glColorPointer (4, GL_FLOAT, 0, item->color);
	else
		glColor3d (1, 1, 1);
	if (item->bAdditive == 1) {
		RIResetShader ();
		glBlendFunc (GL_ONE, GL_ONE);
		}
	else if (item->bAdditive == 2) {
		RIResetShader ();
		glBlendFunc (GL_ONE, GL_ONE_MINUS_DST_COLOR);
		}
	else if (item->bAdditive == 3) {
		RIResetShader ();
		glBlendFunc (GL_ONE, GL_ONE_MINUS_DST_ALPHA);
		}
	else {
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		G3SetupShader (0, 0, item->bmP != NULL, item->bmP ? NULL : item->color);
		}
	glDrawArrays (item->nPrimitive, 0, item->nVertices);
	}
else 
#endif
if (LoadRenderItemImage (item->bmP, item->nColors, 0, item->nWrap, 0, 3, 1)) {
	if (item->bAdditive == 1) {
		RIResetShader ();
		glBlendFunc (GL_ONE, GL_ONE);
		}
	else if (item->bAdditive == 2) {
		RIResetShader ();
		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
	else {
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		G3SetupShader (0, 0, item->bmP != NULL, item->bmP ? NULL : item->color);
		}
	j = item->nVertices;
	glBegin (item->nPrimitive);
	if (item->nColors > 1) {
		if (item->bmP) {
			for (i = 0; i < j; i++) {
				glColor4fv ((GLfloat *) (item->color + i));
				glTexCoord2fv ((GLfloat *) (item->texCoord + i));
				glVertex3fv ((GLfloat *) (item->vertices + i));
				}
			}
		else {
			for (i = 0; i < j; i++) {
				glColor4fv ((GLfloat *) (item->color + i));
				glVertex3fv ((GLfloat *) (item->vertices + i));
				}
			}
		}
	else {
		if (item->nColors)
			glColor4fv ((GLfloat *) item->color);
		else
			glColor3d (1, 1, 1);
		if (item->bmP) {
			for (i = 0; i < j; i++) {
				glTexCoord2fv ((GLfloat *) (item->texCoord + i));
				glVertex3fv ((GLfloat *) (item->vertices + i));
				}
			}
		else {
			for (i = 0; i < j; i++) {
				glVertex3fv ((GLfloat *) (item->vertices + i));
				}
			}
		}
	glEnd ();
	}
#if RI_POLY_OFFSET
if (!item->bmP) {
	glPolygonOffset (0,0);
	glDisable (GL_POLYGON_OFFSET_FILL);
	}
#endif
if (item->bAdditive)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//------------------------------------------------------------------------------

void RIRenderObject (tRIObject *item)
{
DrawPolygonObject (item->objP, 0);
glDisable (GL_TEXTURE_2D);
renderItems.bTextured = 0;
renderItems.bClientState = 0;
}

//------------------------------------------------------------------------------

void RIRenderSprite (tRISprite *item)
{
if (LoadRenderItemImage (item->bmP, item->bColor, item->nFrame, GL_CLAMP, 0, 1, 0)) {
	float		h, w, u, v;
	fVector	fPos = item->position;

	if (renderItems.bDepthMask)
		glDepthMask (renderItems.bDepthMask = 0);
	w = (float) f2fl (item->nWidth); 
	h = (float) f2fl (item->nHeight); 
	u = item->bmP->glTexture->u;
	v = item->bmP->glTexture->v;
	if (item->bColor)
		glColor4fv ((GLfloat *) &item->color);
	else
		glColor3d (1, 1, 1);
	if (item->bAdditive == 2)
		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	else if (item->bAdditive == 1)
		glBlendFunc (GL_ONE, GL_ONE);
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	fPos.p.x -= w;
	fPos.p.y += h;
	glVertex3fv ((GLfloat *) &fPos);
	glTexCoord2f (u, 0);
	fPos.p.x += 2 * w;
	glVertex3fv ((GLfloat *) &fPos);
	glTexCoord2f (u, v);
	fPos.p.y -= 2 * h;
	glVertex3fv ((GLfloat *) &fPos);
	glTexCoord2f (0, v);
	fPos.p.x -= 2 * w;
	glVertex3fv ((GLfloat *) &fPos);
	glEnd ();
	if (item->bAdditive)
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//------------------------------------------------------------------------------

void RIRenderSphere (tRISphere *item)
{
	int bDepthSort = gameOpts->render.bDepthSort;

gameOpts->render.bDepthSort = -1;
RISetClientState (0, 0, 0);
RIResetShader ();
if (item->nType == riSphereShield)
	DrawShieldSphere (item->objP, item->color.red, item->color.green, item->color.blue, item->color.alpha);
if (item->nType == riMonsterball)
	DrawMonsterball (item->objP, item->color.red, item->color.green, item->color.blue, item->color.alpha);
renderItems.bmP = NULL;
renderItems.bTextured = 1;
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDepthMask (0);
glEnable (GL_BLEND);
gameOpts->render.bDepthSort = bDepthSort;
}

//------------------------------------------------------------------------------

void RIRenderBullet (tParticle *pParticle)
{
	tObject	o;
	float		fScale = (float) pParticle->nLife / (float) pParticle->nTTL;

memset (&o, 0, sizeof (o));
o.nType = OBJ_POWERUP;
o.position.vPos = pParticle->pos;
o.position.mOrient = pParticle->orient;
o.renderType = RT_POLYOBJ;
o.rType.polyObjInfo.nModel = BULLET_MODEL;
o.rType.polyObjInfo.nTexOverride = -1;
gameData.models.nScale = (fix) (sqrt (fScale) * F1_0);
DrawPolygonObject (&o, 0);
glDisable (GL_TEXTURE_2D);
renderItems.bTextured = 0;
renderItems.bClientState = 0;
gameData.models.nScale = 0;
}

//------------------------------------------------------------------------------

void RIRenderParticle (tRIParticle *item)
{
if (item->particle->nType == 2)
	RIRenderBullet (item->particle);
else {
	RISetClientState (0, 0, 0);
	RIResetShader ();
	if (renderItems.nPrevType != riParticle) {
		glEnable (GL_TEXTURE_2D);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		gameData.smoke.nLastType = -1;
		renderItems.bTextured = 1;
		InitParticleBuffer ();
		}
	if (renderItems.bDepthMask)
		glDepthMask (renderItems.bDepthMask = 0);
	RenderParticle (item->particle, item->fBrightness);
	renderItems.bTextured = 0;
	}
}

//------------------------------------------------------------------------------

void RIRenderLightning (tRILightning *item)
{
if (renderItems.bDepthMask)
	glDepthMask (renderItems.bDepthMask = 0);
RISetClientState (0, 0, 0);
RIResetShader ();
RenderLightning (item->lightning, item->nLightnings, item->nDepth, 0);
renderItems.bmP = NULL;
renderItems.bTextured = 0;
renderItems.bDepthMask = 1;
}

//------------------------------------------------------------------------------

void RIRenderLightningSegment (tRILightningSegment *item)
{
if (renderItems.bDepthMask)
	glDepthMask (renderItems.bDepthMask = 0);
RISetClientState (0, 0, 0);
RIResetShader ();
RenderLightningSegment (item->vLine, item->vPlasma, &item->color, item->bPlasma, item->bStart, item->bEnd, item->nDepth);
if (item->bPlasma) {
	renderItems.bmP = NULL;
	}
renderItems.bTextured = 0;
}

//------------------------------------------------------------------------------

void RIRenderThruster (tRIThruster *item)
{
if (!renderItems.bDepthMask)
	glDepthMask (renderItems.bDepthMask = 1);
glBlendFunc (GL_ONE, GL_ONE);
glColor3f (0.75f, 0.75f, 0.75f);
glDisable (GL_CULL_FACE);
#if 0
if (LoadRenderItemImage (item->bmP, 0, 0, GL_CLAMP, 1, 1, 0)) {
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), item->vertices);
	glTexCoordPointer (2, GL_FLOAT, 0, item->texCoord);
	if (item->bFlame)
		glDrawArrays (GL_TRIANGLES, 4, 3);
	glDrawArrays (GL_QUADS, 0, 4);
	}
else 
#endif
if (LoadRenderItemImage (item->bmP, 0, 0, GL_CLAMP, 0, 1, 0)) {
	int i;
	if (item->bFlame) {
		glBegin (GL_TRIANGLES);
		for (i = 0; i < 3; i++) {
			glTexCoord2fv ((GLfloat *) (item->texCoord + 4 + i));
			glVertex3fv ((GLfloat *) (item->vertices + 4 + i));
			}
		glEnd ();
		}
	glBegin (GL_QUADS);
	for (i = 0; i < 4; i++) {
		glTexCoord2fv ((GLfloat *) (item->texCoord + i));
		glVertex3fv ((GLfloat *) (item->vertices + i));
		}
	glEnd ();
	}
glEnable (GL_CULL_FACE);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//------------------------------------------------------------------------------

void RIFlushParticleBuffer (int nType)
{
if ((nType != riParticle) && (gameData.smoke.nLastType >= 0)) {
	FlushParticleBuffer (-1);
	CloseParticleBuffer ();
	gameData.smoke.nLastType = -1;
	}
}

//------------------------------------------------------------------------------

void RenderItems (void)
{
	struct tRenderItem	**pd, *pl, *pn;
	int						nDepth, nType, bParticles, bStencil;

if (!(gameOpts->render.bDepthSort && renderItems.pDepthBuffer && (renderItems.nFreeItems < ITEM_BUFFER_SIZE))) {
	return;
	}
RIResetShader ();
bStencil = StencilOff ();
G3DisableClientStates (1, 1, 0, GL_TEXTURE2);
G3DisableClientStates (1, 1, 0, GL_TEXTURE1);
G3DisableClientStates (1, 1, 0, GL_TEXTURE0);
renderItems.bTextured = -1;
renderItems.bClientState = -1;
renderItems.bClientTexCoord = 0;
renderItems.bClientColor = 0;
renderItems.bDepthMask = 0;
renderItems.nWrap = 0;
renderItems.nFrame = -1;
renderItems.bmP = NULL;
renderItems.nItems = ITEM_BUFFER_SIZE - renderItems.nFreeItems;
pl = renderItems.pItemList + ITEM_BUFFER_SIZE - 1;
bParticles = LoadParticleImages ();
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDepthFunc (GL_LESS);
glDepthMask (0);
glEnable (GL_CULL_FACE);
BeginRenderSmoke (-1, 1);
nType = -1;
for (pd = renderItems.pDepthBuffer + ITEM_DEPTHBUFFER_SIZE - 1; 
	  pd >= renderItems.pDepthBuffer; 
	  pd--) {
	if ((pl = *pd)) {
		nDepth = 0;
		do {
			renderItems.nPrevType = nType;
			nType = pl->nType;
			RIFlushParticleBuffer (nType);
			if ((nType == riTexPoly) || (nType == riFlatPoly)) {
				RIRenderPoly (&pl->item.poly);
				}
			else if (nType == riObject) {
				RIRenderObject (&pl->item.object);
				}
			else if (nType == riSprite) {
				RIRenderSprite (&pl->item.sprite);
				}
			else if (nType == riSphere) {
				RIRenderSphere (&pl->item.sphere);
				}
			else if (nType == riParticle) {
				if (bParticles)
					RIRenderParticle (&pl->item.particle);
				}
			else if (nType == riLightning) {
				RIRenderLightning (&pl->item.lightning);
				}
			else if (nType == riLightningSegment) {
				RIRenderLightningSegment (&pl->item.lightningSegment);
				}
			else if (nType == riThruster) {
				RIRenderThruster (&pl->item.thruster);
				}
			pn = pl->pNextItem;
			pl->pNextItem = NULL;
			pl = pn;
			nDepth++;
			} while (pl);
		*pd = NULL;
		}
	}
renderItems.nFreeItems = ITEM_BUFFER_SIZE;
RIFlushParticleBuffer (-1);
EndRenderSmoke (NULL);
RIResetShader ();
G3DisableClientStates (1, 1, 0, GL_TEXTURE0);
OGL_BINDTEX (0);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDepthFunc (GL_LEQUAL);
glDepthMask (1);
StencilOn (bStencil);
return;
}

//------------------------------------------------------------------------------
//eof
