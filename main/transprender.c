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

#include "inferno.h"
#include "u_mem.h"
#include "render.h"
#include "renderlib.h"
#include "objrender.h"
#include "lightning.h"
#include "sphere.h"
#include "network.h"
#include "transprender.h"

#define RI_SPLIT_POLYS 1

//------------------------------------------------------------------------------

tRenderItemBuffer	renderItems;

static tTexCoord3f defaultTexCoord [4] = {{{0,0,1}},{{1,0,1}},{{1,1,1}},{{0,1,1}}};

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
renderItems.zScale = (double) (ITEM_DEPTHBUFFER_SIZE - 1) / (double) (zMax - F1_0);
if (renderItems.zScale > 1)
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
	if (*pd)
		pd = pd;
	ph->pNextItem = *pd;
	*pd = ph;
	}
return renderItems.nFreeItems;
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
for (i = 0; i < split [0].nVertices; i++) {
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
	return AddRenderItem (riPoly, item, sizeof (*item), fl2f (zMax), fl2f (zMin));
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

int RIAddPoly (grsBitmap *bmP, fVector *vertices, char nVertices, tTexCoord3f *texCoord, tRgbaColorf *color, 
					tFaceColor *altColor, char nColors, char bDepthMask, int nPrimitive, int nWrap, int bAdditive)
{
	tRIPoly	item;
	int		i;
	float		z, s = GrAlpha ();

item.bmP = bmP;
item.nVertices = nVertices;
item.nPrimitive = nPrimitive;
item.nWrap = nWrap;
item.bDepthMask = bDepthMask;
item.bAdditive = bAdditive;
memcpy (item.texCoord, texCoord ? texCoord : defaultTexCoord, nVertices * sizeof (tTexCoord3f));
if (item.nColors = nColors) {
	if (nColors < nVertices)
		nColors = 1;
	if (color) {
		memcpy (item.color, color, nColors * sizeof (tRgbaColorf));
		for (i = 0; i < nColors; i++)
			item.color [i].alpha *= s;
		}
	else if (altColor) {
		for (i = 0; i < nColors; i++) {
			item.color [i] = altColor [i].color;
			item.color [i].alpha *= s;
			}
		}
	else
		item.nColors = 0;
	}
memcpy (item.vertices, vertices, nVertices * sizeof (fVector));
#if RI_SPLIT_POLYS
if (bDepthMask) {
	for (i = 0; i < nVertices; i++)
		item.sideLength [i] = (short) (VmVecDistf (vertices + i, vertices + (i + 1) % nVertices) + 0.5f);
	return RISplitPoly (&item, 0);
	}
else 
#endif
	{
	for (i = 0, z = 0; i < item.nVertices; i++) {
		if (z < item.vertices [i].p.z)
			z = item.vertices [i].p.z;
		}
	if ((z < renderItems.zMin) || (z > renderItems.zMax))
		return 0;
	return AddRenderItem (riPoly, &item, sizeof (item), fl2f (z), fl2f (z));
	}
}

//------------------------------------------------------------------------------

int RIAddFace (grsFace *faceP)
{
	fVector		vertices [4];
	tTexCoord3f	texCoord [4];
	int			i, j;

#ifdef _DEBUG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	faceP = faceP;
#endif
for (i = 0, j = faceP->nIndex; i < 4; i++, j++) {
	G3TransformPointf (vertices + i, gameData.segs.fVertices + faceP->index [i], 0);
	vertices [i].p.w = 0;
	memcpy (texCoord + i, gameData.segs.faces.texCoord + j, sizeof (tTexCoord2f));
	}
return RIAddPoly (faceP->bmBot, vertices, 4, texCoord, gameData.segs.faces.color + faceP->nIndex,
						NULL, 4, 1, GL_TRIANGLE_FAN, GL_REPEAT, 0);
}

//------------------------------------------------------------------------------

int RIAddSprite (grsBitmap *bmP, vmsVector *position, tRgbaColorf *color, int nWidth, int nHeight, char nFrame, char bAdditive)
{
	tRISprite	item;
	vmsVector	vPos;

item.bmP = bmP;
if (item.bColor = (color != NULL))
	item.color = *color;
item.nWidth = nWidth;
item.nHeight = nHeight;
item.nFrame = nFrame;
item.bAdditive = bAdditive;
G3TransformPoint (&vPos, position, 0);
VmsVecToFloat (&item.position, &vPos);
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

int RIAddParticle (tParticle *particle, float fBrightness)
{
	tRIParticle	item;

item.particle = particle;
item.fBrightness = fBrightness;
G3TransformPoint (&particle->transPos, &particle->pos, 0);
return AddRenderItem (riParticle, &item, sizeof (item), particle->transPos.p.z, particle->transPos.p.z);
}

//------------------------------------------------------------------------------

int RIAddLightnings (tLightning *lightnings, short nLightnings, short nDepth)
{
	tRILightning item;
	vmsVector vPos;	
	int z;

for (; nLightnings; nLightnings--, lightnings++) {
	item.lightning = lightnings;
	item.nDepth = nDepth;
	G3TransformPoint (&vPos, &lightnings->vPos, 0);
	z = vPos.p.z;
	G3TransformPoint (&vPos, &lightnings->vEnd, 0);
	if (z < vPos.p.z)
		z = vPos.p.z;
	if (!AddRenderItem (riLightning, &item, sizeof (item), z, z))
		return 0;
	}
return 1;
}

//------------------------------------------------------------------------------

int RIAddLightningSegment (fVector *vLine, fVector *vPlasma, tRgbaColorf *color, char bPlasma, char bStart, char bEnd, short nDepth)
{
	tRILightningSegment	item;
	fix z;

memcpy (item.vLine, vLine, 2 * sizeof (fVector));
if (item.bPlasma = bPlasma)
	memcpy (item.vPlasma, vPlasma, 4 * sizeof (fVector));
memcpy (&item.color, color, sizeof (tRgbaColorf));
item.bStart = bStart;
item.bEnd = bEnd;
item.nDepth = nDepth;
z = fl2f ((item.vLine [0].p.z + item.vLine [1].p.z) / 2);
return AddRenderItem (riLightningSegment, &item, sizeof (item), z, z);
}

//------------------------------------------------------------------------------

int RIAddThruster (grsBitmap *bmP, fVector *vThruster, tTexCoord3f *uvlThruster, fVector *vFlame, tTexCoord3f *uvlFlame)
{
	tRIThruster	item;
	int			i, j;
	float			z = 0;

item.bmP = bmP;
memcpy (item.vertices, vThruster, 4 * sizeof (fVector));
memcpy (item.texCoord, uvlThruster, 4 * sizeof (tTexCoord3f));
if (item.bFlame = (vFlame != NULL)) {
	memcpy (item.vertices + 4, vFlame, 3 * sizeof (fVector));
	memcpy (item.texCoord + 4, uvlFlame, 3 * sizeof (tTexCoord3f));
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
			if (renderItems.bClientTexCoord = bTexCoord)
				glEnableClientState (GL_TEXTURE_COORD_ARRAY);
			else
				glDisableClientState (GL_TEXTURE_COORD_ARRAY);
			}
		if (renderItems.bClientColor != bColor) {
			if (renderItems.bClientColor = bColor)
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
	if (renderItems.bClientTexCoord) {
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
		renderItems.bClientTexCoord = 0;
		}
	if (renderItems.bClientColor) {
		glDisableClientState (GL_COLOR_ARRAY);
		renderItems.bClientColor = 0;
		}
	glActiveTexture (GL_TEXTURE0);
	renderItems.bClientState = 0;
	}
renderItems.bmP = NULL;
return 1;
}

//------------------------------------------------------------------------------

int LoadRenderItemImage (grsBitmap *bmP, char nColors, char nFrame, int nWrap, int bClientState)
{
if (bmP) {
	if (RISetClientState (bClientState, 1, nColors > 1) || (renderItems.bTextured < 1)) {
		glEnable (GL_TEXTURE_2D);
		//glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		renderItems.bTextured = 1;
		}
	if ((bmP != renderItems.bmP) || (nFrame != renderItems.nFrame) || (nWrap != renderItems.nWrap)) {
		if (OglBindBmTex (bmP, 1, 1)) {
			renderItems.bmP = NULL;
			return 0;
			}
		bmP = BmOverride (bmP, nFrame);
		OglTexWrap (bmP->glTexture, nWrap);
		renderItems.bmP = bmP;
		renderItems.nWrap = nWrap;
		}
	}
else if (RISetClientState (bClientState, 0, nColors > 1) || renderItems.bTextured) {
	glDisable (GL_TEXTURE_2D);
	renderItems.bTextured = 0;
	}
return (renderItems.bClientState == bClientState);
}

//------------------------------------------------------------------------------

void RIRenderPoly (tRIPoly *item)
{
	int	i, j;

if (renderItems.bDepthMask != item->bDepthMask)
	glDepthMask (renderItems.bDepthMask = item->bDepthMask);
glEnable (GL_CULL_FACE);
#if 1
if (LoadRenderItemImage (item->bmP, item->nColors, 0, item->nWrap, 1)) {
	if (item->bAdditive)
		glBlendFunc (GL_ONE, GL_ONE);
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), item->vertices);
	if (renderItems.bTextured)
		glTexCoordPointer (2, GL_FLOAT, sizeof (tTexCoord3f), item->texCoord);
	if (item->nColors == 1)
		glColor4fv ((GLfloat *) item->color);
	else if (item->nColors > 1)
		glColorPointer (4, GL_FLOAT, sizeof (tRgbaColorf), item->color);
	else
		glColor3d (1, 1, 1);
	glDrawArrays (item->nPrimitive, 0, item->nVertices);
	}
else 
#endif
if (LoadRenderItemImage (item->bmP, item->nColors, 0, item->nWrap, 0)) {
	if (item->bAdditive)
		glBlendFunc (GL_ONE, GL_ONE);
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
glDisable (GL_CULL_FACE);
if (item->bAdditive)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//------------------------------------------------------------------------------

void RIRenderSprite (tRISprite *item)
{
if (LoadRenderItemImage (item->bmP, item->bColor, item->nFrame, GL_CLAMP, 0)) {
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
	if (item->bAdditive)
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
RISetClientState (0, 0, 0);
if (item->nType == riSphereShield)
	DrawShieldSphere (item->objP, item->color.red, item->color.green, item->color.blue, item->color.alpha);
if (item->nType == riMonsterball)
	DrawMonsterball (item->objP, item->color.red, item->color.green, item->color.blue, item->color.alpha);
glDisable (GL_CULL_FACE);
renderItems.bmP = NULL;
renderItems.bTextured = 1;
}

//------------------------------------------------------------------------------

void RIRenderParticle (tRIParticle *item)
{
RISetClientState (1, 1, 1);
if (renderItems.bTextured < 1) {
	glEnable (GL_TEXTURE_2D);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	gameData.smoke.nLastType = -1;
	renderItems.bTextured = 1;
	}
if (renderItems.bDepthMask)
	glDepthMask (renderItems.bDepthMask = 0);
RenderParticle (item->particle, item->fBrightness);
renderItems.bTextured = 0;
}

//------------------------------------------------------------------------------

void RIRenderLightning (tRILightning *item)
{
if (renderItems.bDepthMask)
	glDepthMask (renderItems.bDepthMask = 0);
RISetClientState (0, 0, 0);
RenderLightning (item->lightning, 1, item->nDepth, 0);
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
#if 1
if (LoadRenderItemImage (item->bmP, 0, 0, GL_CLAMP, 1)) {
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), item->vertices);
	glTexCoordPointer (2, GL_FLOAT, sizeof (tTexCoord3f), item->texCoord);
	glColor3d (1,1,1);
	if (item->bFlame)
		glDrawArrays (GL_TRIANGLES, 4, 3);
	glDrawArrays (GL_QUADS, 0, 4);
	}
else 
#endif
if (LoadRenderItemImage (item->bmP, 0, 0, GL_CLAMP, 0)) {
	int i;
	glColor3d (1,1,1);
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
}

//------------------------------------------------------------------------------

void RIFlushParticleBuffer (int nType)
{
if ((nType != riParticle) && (gameData.smoke.nLastType >= 0)) {
	FlushParticleBuffer ();
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
bStencil = StencilOff ();
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
glDisable (GL_CULL_FACE);
BeginRenderSmoke (-1, 1);
for (pd = renderItems.pDepthBuffer + ITEM_DEPTHBUFFER_SIZE - 1; 
	  pd >= renderItems.pDepthBuffer; 
	  pd--) {
	if (pl = *pd) {
		nDepth = 0;
		do {
			nType = pl->nType;
			RIFlushParticleBuffer (nType);
			if (nType == riPoly) {
				RIRenderPoly (&pl->item.poly);
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
G3DisableClientStates (1, 1, GL_TEXTURE0);
if (EGI_FLAG (bShadows, 0, 1, 0)) 
	glEnable (GL_STENCIL_TEST);
glDepthFunc (GL_LEQUAL);
glEnable (GL_CULL_FACE);
StencilOn (bStencil);
return;
}

//------------------------------------------------------------------------------
//eof
