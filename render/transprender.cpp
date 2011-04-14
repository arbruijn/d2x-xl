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

#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "ogl_fastrender.h"
#include "rendermine.h"
#include "segmath.h"
#include "objrender.h"
#include "lightmap.h"
#include "lightning.h"
#include "sphere.h"
#include "../effects/glow.h"
#include "glare.h"
#include "thrusterflames.h"
#include "automap.h"
#include "transprender.h"
#include "renderthreads.h"
#include "addon_bitmaps.h"

#define RENDER_TRANSPARENCY 1
#define RENDER_TRANSP_DECALS 1

#define TI_SPLIT_POLYS 0
#define TI_POLY_OFFSET 0
#define TI_POLY_CENTER 1

#if DBG
int nDbgPoly = -1, nDbgItem = -1;
#endif

CTransparencyRenderer transparencyRenderer;

#define LAZY_RESET 1

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CEffectArea& CEffectArea::Add (CFloatVector& pos, float rad) 
{
if (m_rad == 0.0f) {
	m_pos = pos;
	m_rad = rad;
	}
else {
	CFloatVector v = pos - m_pos;
	float d = v.Mag ();
	float h = rad + d - m_rad;
	if (h > 0.0f) {
		h /= 2.0f;
		v /= d;
		v *= h;
		m_pos += v;
		m_rad += h;
		}
	m_pos = CFloatVector::Avg (m_pos, pos);
	}
return *this;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static tTexCoord2f tcDefault [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

inline int CTransparencyRenderer::AllocBuffers (void)
{
if (m_data.depthBuffer.Buffer ())
	return 1;
if (!m_data.depthBuffer.Create (ITEM_DEPTHBUFFER_SIZE))
	return 0;
if (!m_data.itemLists.Create (ITEM_BUFFER_SIZE)) {
	m_data.depthBuffer.Destroy ();
	return 0;
	}
m_data.nFreeItems = 0;
ResetBuffers ();
return 1;
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::FreeBuffers (void)
{
m_data.itemLists.Destroy ();
m_data.depthBuffer.Destroy ();
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::ResetFreeList (void)
{
	tTranspItem* pl, * pn;

for (pl = m_data.freeList.head; pl; pl = pn) {
	pn = pl->pNextItem;
	pl->pNextItem = NULL;
	}
m_data.freeList.head = NULL;
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::ResetBuffers (void)
{
if (m_data.depthBuffer.Buffer ())
	m_data.depthBuffer.Clear ();
if (m_data.itemLists.Buffer ())
	memset (m_data.itemLists.Buffer (), 0, (ITEM_BUFFER_SIZE - m_data.nFreeItems) * sizeof (struct tTranspItem));
m_data.nFreeItems = ITEM_BUFFER_SIZE;
ResetFreeList ();
}


//------------------------------------------------------------------------------

void CTransparencyRenderer::InitBuffer (int zMin, int zMax, int nWindow)
{
#if LAZY_RESET
if (nWindow)
	m_data.bAllowAdd = 1;
else if (gameStates.render.cameras.bActive)
	m_data.bAllowAdd = 1;
else if (gameOpts->render.stereo.nGlasses && (ogl.StereoSeparation () >= 0))
	m_data.bAllowAdd = -1;
else
	m_data.bAllowAdd = 1;
if (m_data.bAllowAdd > 0) 
#endif
	{
	m_data.zMin = 0;
	m_data.zMax = zMax;
#if DBG
	if (zMax < 0)
		zMax = zMax;
#endif
	m_data.zScale = (double) (ITEM_DEPTHBUFFER_SIZE - 1) / (double) (m_data.zMax);
	if (m_data.zScale < 0)
		m_data.zScale = 1;
	else if (m_data.zScale > 1)
		m_data.zScale = 1;
	m_data.vViewer [0] = gameData.render.mine.viewer.vPos;
	transformation.Transform (m_data.vViewer [1], m_data.vViewer [0]);
	m_data.vViewerf [0].Assign (m_data.vViewer [0]);
	m_data.vViewerf [1].Assign (m_data.vViewer [1]);
	}
m_data.bRenderGlow =
m_data.bSoftBlend = 0;
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::Add (tTranspItemType nType, void *itemData, int itemSize, CFixVector vPos, int nOffset, bool bClamp, int bTransformed)
{
if (gameStates.render.nType == RENDER_TYPE_TRANSPARENCY)
	return 0;
#if LAZY_RESET
if (!m_data.bAllowAdd)
	return 0;
if ((bTransformed > 0) && (m_data.bAllowAdd < 0))	// already added and still in buffer
	return 0;
#endif
#if RENDER_TRANSPARENCY
	tTranspItem*		ph, *	pi;
	tTranspItemList*	pd;
	int					nDepth = Depth (vPos, bTransformed > 0);
	
if (nDepth >= I2X (nOffset))
	nDepth -= I2X (nOffset);

if (nDepth < 0) {
	if (!bClamp)
		return m_data.nFreeItems;
	nDepth = m_data.zMin;
	}
else if (nDepth > m_data.zMax) {
	if (!bClamp)
		return m_data.nFreeItems;
	nDepth = m_data.zMax;
	}

// find the first particle to insert the new one *before* and place in pj; pi will be it's predecessor (NULL if to insert at list start)
#if USE_OPENMP > 1
#	pragma omp critical (transpRender)
#elif !USE_OPENMP
if (gameStates.app.bMultiThreaded)
	SDL_mutexP (tiRender.semaphore);
#endif
if (AllocBuffers ()) {
	nOffset = int (double (nDepth) * m_data.zScale);
	if (nOffset < ITEM_DEPTHBUFFER_SIZE) {
		pd = m_data.depthBuffer + nOffset;
		if (m_data.nFreeItems > 0) {
			--m_data.nFreeItems;
			if (!m_data.freeList.head)
				ph = m_data.itemLists + m_data.nFreeItems;
			else {
				ph = m_data.freeList.head;
				m_data.freeList.head = ph->pNextItem;
				}
			ph->nItem = m_data.nItems [0]++;
			ph->nType = nType;
			ph->bRendered = 0;
			ph->bTransformed = bTransformed;
			ph->parentP = NULL;
			ph->z = nDepth;
			ph->bValid = true;
			memcpy (&ph->item, itemData, itemSize);
			for (pi = pd->head; pi; pi = pi->pNextItem) {
				if ((pi->z < nDepth) || ((pi->z == nDepth) && (pi->nType < nType)))
					break;
				}
			if (pi) {
				ph->pNextItem = pi->pNextItem;
				pi->pNextItem = ph;
				}
			else {
				ph->pNextItem = pd->head;
				pd->head = ph;
				}
			if (m_data.nMinOffs > nOffset)
				m_data.nMinOffs = nOffset;
			if (m_data.nMaxOffs < nOffset)
				m_data.nMaxOffs = nOffset;
			}
		}
	}
#if !USE_OPENMP
if (gameStates.app.bMultiThreaded)
	SDL_mutexV (tiRender.semaphore);
#endif

return m_data.nFreeItems;
#else
return 0;
#endif
}

//------------------------------------------------------------------------------

#if TI_SPLIT_POLYS

int CTransparencyRenderer::SplitPoly (tTranspPoly *item, int nDepth)
{
	tTranspPoly		split [2];
	CFloatVector		vSplit;
	tRgbaColorf	color;
	int			i, l, i0, i1, i2, i3, nMinLen = 0x7fff, nMaxLen = 0;
	float			z, zMin, zMax, *coord, *c0, *c1;

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
		z = split [0].vertices [i].dir.coord.z;
		if (zMax < z)
			zMax = z;
		if (zMin > z)
			zMin = z;
		}
#if TI_POLY_CENTER
	return Add (item->bmP ? riTexPoly : riFlatPoly, item, sizeof (*item), F2X (zMax), F2X ((zMax + zMin) / 2));
#else
	return Add (item->bmP ? riTexPoly : riFlatPoly, item, sizeof (*item), F2X (zMax), F2X (zMin));
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
		split [0].texCoord [i0].dir.u =
		split [1].texCoord [i1].dir.u = (split [0].texCoord [i1].dir.u + split [0].texCoord [i0].dir.u) / 2;
		split [0].texCoord [i0].dir.dir =
		split [1].texCoord [i1].dir.dir = (split [0].texCoord [i1].dir.dir + split [0].texCoord [i0].dir.dir) / 2;
		}
	if (split [0].nColors == 3) {
		for (i = 4, coord = reinterpret_cast<float*> (&color, c0 = reinterpret_cast<float*> (split [0].color + i0), c1 = reinterpret_cast<float*> (split [0].color + i1); i; i--)
			*coord++ = (*c0++ + *c1++) / 2;
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
		split [0].texCoord [i1].dir.u =
		split [1].texCoord [i0].dir.u = (split [0].texCoord [i1].dir.u + split [0].texCoord [i0].dir.u) / 2;
		split [0].texCoord [i1].dir.dir =
		split [1].texCoord [i0].dir.dir = (split [0].texCoord [i1].dir.dir + split [0].texCoord [i0].dir.dir) / 2;
		split [0].texCoord [i2].dir.u =
		split [1].texCoord [i3].dir.u = (split [0].texCoord [i3].dir.u + split [0].texCoord [i2].dir.u) / 2;
		split [0].texCoord [i2].dir.dir =
		split [1].texCoord [i3].dir.dir = (split [0].texCoord [i3].dir.dir + split [0].texCoord [i2].dir.dir) / 2;
		}
	if (split [0].nColors == 4) {
		for (i = 4, coord = reinterpret_cast<float*> (&color, c0 = reinterpret_cast<float*> (split [0].color + i0), c1 = reinterpret_cast<float*> (split [0].color + i1); i; i--)
			*coord++ = (*c0++ + *c1++) / 2;
		split [0].color [i1] =
		split [1].color [i0] = color;
		for (i = 4, coord = reinterpret_cast<float*> (&color, c0 = reinterpret_cast<float*> (split [0].color + i2), c1 = reinterpret_cast<float*> (split [0].color + i3); i; i--)
			*coord++ = (*c0++ + *c1++) / 2;
		split [0].color [i2] =
		split [1].color [i3] = color;
		}
	split [0].sideLength [i0] =
	split [1].sideLength [i0] =
	split [0].sideLength [i2] =
	split [1].sideLength [i2] = nMaxLen / 2;
	}
return SplitPoly (split, nDepth + 1) && SplitPoly (split + 1, nDepth + 1);
}

#endif

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddObject (CObject *objP)
{
if (gameStates.render.nShadowMap)
	return 0;

	tTranspObject	item;
//	CFixVector		vPos;

if (objP->info.nType == 255)
	return 0;
item.objP = objP;
item.vScale = gameData.models.vScale;
//transformation.Transform (vPos, OBJPOS (objP)->vPos, 0);
return Add (tiObject, &item, sizeof (item), OBJPOS (objP)->vPos, 0, 0, -1);
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddPoly (CSegFace *faceP, tFaceTriangle *triP, CBitmap *bmP,
												CFloatVector *vertices, char nVertices, tTexCoord2f *texCoord, tRgbaColorf *color,
												tFaceColor *altColor, char nColors, char bDepthMask, int nPrimitive, int nWrap, int bAdditive,
												short nSegment)
{
if (gameStates.render.nShadowMap)
	return 0;

	tTranspPoly	item;
	int			i;
	float			s = gameStates.render.grAlpha;
	//fix			zCenter;

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
if (bDepthMask && m_data.bSplitPolys) {
	for (i = 0; i < nVertices; i++)
		item.sideLength [i] = (short) (CFloatVector::Dist(vertices [i], vertices [(i + 1) % nVertices]) + 0.5f);
	return SplitPoly (&item, 0);
	}
else
#endif
	{
	if (faceP) {
		return Add (tiFace, &item, sizeof (item), 
						SEGMENTS [faceP->m_info.nSegment].Side (faceP->m_info.nSide)->Center (), 0, true, false);
		}
	CFloatVector v = item.vertices [0];
	for (i = 1; i < item.nVertices; i++) 
		v += item.vertices [i];
	v /= item.nVertices;
	CFixVector vPos;
	vPos.Assign (v);
	if (bAdditive & 3)
		m_data.bRenderGlow = 1;
	if ((gameOpts->render.effects.bSoftParticles & 1) != 0)
		m_data.bSoftBlend = 1;
	return Add (triP ? tiFace : tiPoly, &item, sizeof (item), vPos, 0, true);
	}
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddFaceTris (CSegFace *faceP)
{
if (gameStates.render.nShadowMap)
	return 0;

	tFaceTriangle*	triP;
	CFloatVector	vertices [3];
	int				h, i, j, bAdditive = FaceIsAdditive (faceP);
	CBitmap*			bmP = faceP->m_info.bTextured ? /*faceP->bmTop ? faceP->bmTop :*/ faceP->bmBot : NULL;

if (bmP)
	bmP = bmP->Override (-1);
#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	faceP = faceP;
#endif
triP = FACES.tris + faceP->m_info.nTriIndex;
for (h = faceP->m_info.nTris; h; h--, triP++) {
	for (i = 0, j = triP->nIndex; i < 3; i++, j++) {
#if 1
		transformation.Transform (vertices [i], *(reinterpret_cast<CFloatVector*> (FACES.vertices + j)), 0);
#else
		if (automap.Display ())
			transformation.Transform (vertices + i, gameData.segs.fVertices + triP->index [i], 0);
		else
			vertices [i].Assign (gameData.segs.points [triP->index [i]].p3_vec);
#endif
		}
	if (!AddPoly (faceP, triP, bmP, vertices, 3, FACES.texCoord + triP->nIndex,
					  FACES.color + triP->nIndex,
					  NULL, 3, (bmP != NULL) && !bAdditive, GL_TRIANGLES, GL_REPEAT,
					  bAdditive, faceP->m_info.nSegment))
		return 0;
	}
return 1;
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddFaceQuads (CSegFace *faceP)
{
if (gameStates.render.nShadowMap)
	return 0;

	CFloatVector	vertices [4];
	int				i, j, bAdditive = FaceIsAdditive (faceP);
	CBitmap*			bmP = faceP->m_info.bTextured ? /*faceP->bmTop ? faceP->bmTop :*/ faceP->bmBot : NULL;

if (bmP)
	bmP = bmP->Override (-1);
#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	faceP = faceP;
#endif
for (i = 0, j = faceP->m_info.nIndex; i < 4; i++, j++) {
#if 1
	transformation.Transform (vertices [i], *(reinterpret_cast<CFloatVector*> (FACES.vertices + j)), 0);
#else
	if (automap.Display ())
		transformation.Transform(vertices [i], gameData.segs.fVertices [faceP->m_info.index [i]], 0);
	else
		vertices [i].Assign (gameData.segs.points [faceP->m_info.index [i]].p3_vec);
#endif
	}
return AddPoly (faceP, NULL, bmP,
					 vertices, 4, FACES.texCoord + faceP->m_info.nIndex,
					 FACES.color + faceP->m_info.nIndex,
					 NULL, 4, (bmP != NULL) && !bAdditive, GL_TRIANGLE_FAN, GL_REPEAT,
					 bAdditive, faceP->m_info.nSegment) > 0;
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddSprite (CBitmap *bmP, const CFixVector& position, tRgbaColorf *color,
												  int nWidth, int nHeight, char nFrame, char bAdditive, float fSoftRad)
{
if (gameStates.render.nShadowMap)
	return 0;

	tTranspSprite	item;

item.bmP = bmP;
if ((item.bColor = (color != NULL)))
	item.color = *color;
item.nWidth = nWidth;
item.nHeight = nHeight;
item.nFrame = nFrame;
item.bAdditive = bAdditive;
item.fSoftRad = fSoftRad;
item.position.Assign (position);
if (bAdditive & 3)
	m_data.bRenderGlow = 1;
if ((gameOpts->render.effects.bSoftParticles & 1) != 0)
	m_data.bSoftBlend = 1;
return Add (tiSprite, &item, sizeof (item), position);
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddSpark (const CFixVector& position, char nType, int nSize, char nFrame)
{
if (gameStates.render.nShadowMap)
	return 0;

	tTranspSpark	item;
//	CFixVector		vPos;

item.nSize = nSize;
item.nFrame = nFrame;
item.nType = nType;
item.position.Assign (position);
if (gameOpts->render.effects.bSoftParticles & 2)
	m_data.bSoftBlend = 1;
return Add (tiSpark, &item, sizeof (item), position);
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddSphere (tTranspSphereType nType, float red, float green, float blue, float alpha, 
												  CObject *objP, char bAdditive, fix nSize)
{
if (gameStates.render.nShadowMap)
	return 0;

tTranspSphere	item;
	//CFixVector		vPos;
item.nType = nType;
item.color.red = red;
item.color.green = green;
item.color.blue = blue;
item.color.alpha = alpha;
item.nSize = nSize;
item.bAdditive = bAdditive;
item.objP = objP;
if (nType != riMonsterball)
	m_data.bRenderGlow = 1;
//transformation.Transform (vPos, objP->info.position.vPos, 0);
return Add (tiSphere, &item, sizeof (item), objP->info.position.vPos);
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddParticle (CParticle *particle, float fBrightness, int nThread)
{
if (gameStates.render.nShadowMap)
	return 0;

	tTranspParticle	item;
//	fix					z;

if ((particle->m_nType < 0) || (particle->m_nType >= PARTICLE_TYPES))
	return 0;
item.particle = particle;
item.fBrightness = fBrightness;
//particle->Transform (gameStates.render.bPerPixelLighting == 2);
if (gameOpts->render.effects.bSoftParticles & 4)
	m_data.bSoftBlend = 1;
return Add (tiParticle, &item, sizeof (item), particle->m_vPos, 10);
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddLightning (CLightning *lightningP, short nDepth)
{
if (gameStates.render.nShadowMap)
	return 0;

	tTranspLightning	item;
	bool					bSwap;
	//CFixVector			vPos;

item.lightning = lightningP;
item.nDepth = nDepth;
#if 0
transformation.Transform (vPos, lightningP->m_vPos, 0);
z = vPos.dir.coord.z;
transformation.Transform (vPos, lightningP->m_vEnd, 0);
if (z < vPos.dir.coord.z)
	z = vPos.dir.coord.z;
#endif
fix d1 = Depth (lightningP->m_vPos, false);
fix d2 = Depth (lightningP->m_vEnd, false);
if ((bSwap = (d1 < d2)))
	::Swap (d1, d2);
if (d2 > m_data.zMax)
	return 0;
if (d1 < m_data.zMin)
	return 0;
m_data.bRenderGlow = 1;
if (!Add (tiLightning, &item, sizeof (item), bSwap ? lightningP->m_vEnd : lightningP->m_vPos /*CFixVector::Avg (lightningP->m_vPos, lightningP->m_vEnd)*/, 0, true, 0)) // -1))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddLightTrail (CBitmap *bmP, CFloatVector *vThruster, tTexCoord2f *tcThruster, CFloatVector *vFlame, tTexCoord2f *tcFlame, tRgbaColorf *colorP)
{
if (gameStates.render.nShadowMap)
	return 0;

	tTranspLightTrail	item;
	int					i, j, iMin = 0;
	CFixVector			v;
	float					d, dMin = 1e30f;

item.bmP = bmP;
if ((item.bTrail = (vFlame != NULL))) {
	memcpy (item.vertices, vFlame, 4 * sizeof (CFloatVector));
	memcpy (item.texCoord, tcFlame, 4 * sizeof (tTexCoord2f));
	j = 4;
	}
else
	j = 0;
memcpy (item.vertices + j, vThruster, 4 * sizeof (CFloatVector));
memcpy (item.texCoord + j, tcThruster, 4 * sizeof (tTexCoord2f));
item.color = *colorP;
for (i = 0; i < j; i++) {
	d = Depth (item.vertices [i], true);
	if (dMin > d) {
		dMin = d;
		iMin = i;
		}
	}
v.Assign (item.vertices [iMin]);
m_data.bRenderGlow = 1;
return Add (tiLightTrail, &item, sizeof (item), v, 0, false, 0);
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::AddThruster (CObject* objP, tThrusterInfo* infoP, int nThruster)
{
if (gameStates.render.nShadowMap)
	return 0;

	tTranspThruster item;

item.objP = objP;
item.info = *infoP;
item.nThruster = nThruster;
#if DBG
if (Add (tiThruster, &item, sizeof (item), infoP->vPos [nThruster], 0, false, 0))
	return 1;
#endif
m_data.bRenderGlow = 1;
return Add (tiThruster, &item, sizeof (item), infoP->vPos [nThruster], 0, false, 0);
}

//------------------------------------------------------------------------------

inline void CTransparencyRenderer::ResetBitmaps (void)
{
m_data.bmP [0] =
m_data.bmP [1] =
m_data.bmP [2] = NULL;
m_data.bDecal = 0;
m_data.bTextured = 0;
m_data.nFrame = -1;
m_data.bUseLightmaps = 0;
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::LoadTexture (CBitmap *bmP, int nFrame, int bDecal, int bLightmaps, int nWrap)
{
if (bmP) {
#if 0
	ogl.SelectTMU (GL_TEXTURE0 + bLightmaps, true);
	ogl.SetTexturing (true);
#endif
	if ((bmP != m_data.bmP [bDecal]) || (nFrame != m_data.nFrame) || (nWrap != m_data.nWrap)) {
		gameData.render.nStateChanges++;
		if (bmP) {
			if (bmP->Bind (1)) {
				ResetBitmaps ();
				return 0;
				}
			if (bDecal != 2)
				bmP = bmP->Override (nFrame);
			bmP->Texture ()->Wrap (nWrap);
			m_data.nWrap = nWrap;
			m_data.nFrame = nFrame;
			}
		else
			ogl.BindTexture (0);
		m_data.bmP [bDecal] = bmP;
		}
	}
else {
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	ResetBitmaps ();
	}
return 1;
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderPoly (tTranspPoly *item)
{
PROF_START
	CBitmap*		bmP = item->bmP;
	int			bSoftBlend = SoftBlend (1);

ogl.ResetClientStates (1);
m_data.bmP [1] = m_data.bmP [2] = NULL;
if (item->bAdditive & 3)
	glowRenderer.Begin (GLOW_POLYS, 2, false, 1.0f);
ogl.EnableClientStates (bmP != NULL, item->nColors == item->nVertices, 0, GL_TEXTURE0);
if (LoadTexture (bmP, 0, 0, 0, item->nWrap)) {
	if (bmP)
		OglTexCoordPointer (2, GL_FLOAT, 0, item->texCoord);
	if (item->nColors == item->nVertices)
		OglColorPointer (4, GL_FLOAT, 0, item->color);
	else
		glColor4fv (reinterpret_cast<GLfloat*> (item->color));
	OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), item->vertices);
	ogl.SetBlendMode (item->bAdditive);
	if (!(bSoftBlend && glareRenderer.LoadShader (5, item->bAdditive != 0)))
		shaderManager.Deploy (-1);
	//ogl.SetTexturing (false);
	//glLineWidth (5);
	ogl.SetFaceCulling (false);
	ogl.SetupTransform (0);
	OglDrawArrays (item->nPrimitive, 0, item->nVertices);
	ogl.ResetTransform (0);
	ogl.SetFaceCulling (true);
	//glLineWidth (1);
	}
#if DBG
else
	HUDMessage (0, "Couldn't load '%s'", bmP->Name ());
#endif
#if TI_POLY_OFFSET
if (!bmP) {
	glPolygonOffset (0,0);
	glDisable (GL_POLYGON_OFFSET_FILL);
	}
#endif
PROF_END(ptRenderFaces)
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderFace (tTranspPoly *item)
{
PROF_START
	CSegFace*		faceP;
	tFaceTriangle*	triP;
	CBitmap*			bmBot = item->bmP, *bmTop, *bmMask;
	int				bDecal, 
						bLightmaps = m_data.bLightmaps && !gameStates.render.bFullBright,
						bTextured = (bmBot != NULL), 
						bColored = (item->nColors == item->nVertices) && !(bTextured && gameStates.render.bFullBright);

#if TI_POLY_OFFSET
if (!bmBot) {
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1,1);
	glPolygonMode (GL_FRONT, GL_FILL);
	}
#endif

faceP = item->faceP;
triP = item->triP;

#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (bTextured) {
	if ((bmTop = faceP->bmTop))
		bmTop = bmTop->Override (-1);
	if (bmTop && !(bmTop->Flags () & (BM_FLAG_SUPER_TRANSPARENT | BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU))) {
		bmBot = bmTop;
		bmTop = bmMask = NULL;
		bDecal = -1;
		faceP->m_info.nRenderType = gameStates.render.history.nType = 1;
		}
	else {
		bDecal = (bmTop != NULL);
		bmMask = (bDecal && ((bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0) && gameStates.render.textures.bHaveMaskShader) ? bmTop->Mask () : NULL;
		}
	}
else {
	bmTop = bmMask = NULL;
	bDecal = 0;
	}

int bAdditive = item->bAdditive, nIndex = triP ? triP->nIndex : faceP->m_info.nIndex;
if (bAdditive & 3)
	glowRenderer.Begin (GLOW_FACES, 2, false, 1.0f);
#if 0 //DBG
m_data.bmP [0] = NULL;
ogl.ResetClientStates (bLightmaps);
#else
ogl.ResetClientStates (bTextured + bLightmaps + (bmTop != NULL) + (bmMask != NULL));
#endif

if (bmTop) {
	ogl.EnableClientStates (bTextured, 0, 0, GL_TEXTURE1 + bLightmaps);
#if 0 //DBG
	m_data.bmP [1] = NULL;
#endif
	if (!LoadTexture (bmTop, 0, 1, bLightmaps, item->nWrap))
		return;
	if (bTextured)
		OglTexCoordPointer (2, GL_FLOAT, 0, FACES.ovlTexCoord + nIndex);
	OglVertexPointer (3, GL_FLOAT, 0, FACES.vertices + nIndex);
	if (bmMask) {
		ogl.EnableClientStates (bTextured, 0, 0, GL_TEXTURE2 + bLightmaps);
		if (!LoadTexture (bmMask, 0, 2, bLightmaps, item->nWrap))
			return;
		if (bTextured)
			OglTexCoordPointer (2, GL_FLOAT, 0, FACES.ovlTexCoord + nIndex);
		OglVertexPointer (3, GL_FLOAT, 0, FACES.vertices + nIndex);
		}
	else
		m_data.bmP [2] = NULL;
	}
else {
	m_data.bmP [1] = m_data.bmP [2] = NULL;
	}

ogl.EnableClientStates (bTextured, bColored && !bLightmaps, !bLightmaps, GL_TEXTURE0 + bLightmaps);
if (!LoadTexture (bmBot, 0, 0, bLightmaps, item->nWrap))
	return;
if (bTextured)
	OglTexCoordPointer (2, GL_FLOAT, 0, (bDecal < 0) ? FACES.ovlTexCoord + nIndex : FACES.texCoord + nIndex);
if (!bLightmaps) {
	if (bColored)
		OglColorPointer (4, GL_FLOAT, 0, FACES.color + nIndex);
	OglNormalPointer (GL_FLOAT, 0, FACES.normals + nIndex);
	}
OglVertexPointer (3, GL_FLOAT, 0, FACES.vertices + nIndex);

if (bLightmaps) {
	ogl.EnableClientStates (bTextured, bColored, 1, GL_TEXTURE0);
	if (bTextured)
		OglTexCoordPointer (2, GL_FLOAT, 0, FACES.lMapTexCoord + nIndex);
	if (bColored)
		OglColorPointer (4, GL_FLOAT, 0, FACES.color + nIndex);
	OglNormalPointer (GL_FLOAT, 0, FACES.normals + nIndex);
	OglVertexPointer (3, GL_FLOAT, 0, FACES.vertices + nIndex);
	}
#if 1
if (!bTextured) {
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	m_data.bmP [0] = NULL;
	}
#endif
ogl.SetupTransform (1);
if (gameStates.render.bFullBright)
	glColor4f (1,1,1,item->color [0].alpha);
else if (!bColored)
	glColor4fv (reinterpret_cast<GLfloat*> (item->color));
ogl.SetBlendMode (bAdditive);

#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (gameStates.render.bPerPixelLighting && !gameStates.render.bFullBright) {
	if (!faceP->m_info.bColored) {
		SetupGrayScaleShader ((int) faceP->m_info.nRenderType, &faceP->m_info.color);
		OglDrawArrays (item->nPrimitive, 0, item->nVertices);
		}
	else {
		if (gameStates.render.bPerPixelLighting == 1) {
			SetupLightmapShader (faceP, int (faceP->m_info.nRenderType), false);
			OglDrawArrays (item->nPrimitive, 0, item->nVertices);
			}
		else {
			ogl.m_states.iLight = 0;
			lightManager.Index (0,0).nActive = -1;
			for (;;) {
				SetupPerPixelLightingShader (faceP, int (faceP->m_info.nRenderType), false);
				OglDrawArrays (item->nPrimitive, 0, item->nVertices);
				if (ogl.m_states.iLight >= ogl.m_states.nLights) 
					break;
				bAdditive = 1;
				ogl.SetBlendMode (GL_ONE, GL_ONE);
				ogl.SetDepthWrite (false);
				}
			}
		if (gameStates.render.bHeadlights) {
#	if DBG
			if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
			ogl.SetBlending (true);
#	endif
			lightManager.Headlights ().SetupShader (bTextured ? bmMask ? 3 : bDecal ? 2 : 1 : 0, 1, bTextured ? NULL : &faceP->m_info.color);
			if (bAdditive != 2) {
				bAdditive = 2;
				ogl.SetBlendMode (2);
				ogl.SetDepthWrite (false);
				}
			OglDrawArrays (item->nPrimitive, 0, item->nVertices);
			}
		}
	}
else {
	SetupShader (faceP, bmMask != NULL, bDecal > 0, bmBot != NULL,
				    (item->nSegment < 0) || !automap.Display () || automap.m_visited [item->nSegment],
					 bTextured ? NULL : faceP ? &faceP->m_info.color : item->color);
	OglDrawArrays (item->nPrimitive, 0, item->nVertices);
	}
ogl.ResetTransform (faceP != NULL);
gameData.render.nTotalFaces++;

#if TI_POLY_OFFSET
if (!bmBot) {
	glPolygonOffset (0,0);
	glDisable (GL_POLYGON_OFFSET_FILL);
	}
#endif
PROF_END(ptRenderFaces)
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderObject (tTranspObject *item)
{
shaderManager.Deploy (-1);
ogl.ResetClientStates ();
gameData.models.vScale = item->vScale;
DrawPolygonObject (item->objP, 1);
gameData.models.vScale.SetZero ();
ResetBitmaps ();
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderSprite (tTranspSprite *item)
{
#if 1
	int bSoftBlend = (item->fSoftRad > 0) && SoftBlend (1);
	int bGlow = 1;

if (glowRenderer.Available (GLOW_SPRITES)) {
	if (item->bAdditive == 1) 
		glowRenderer.Begin (GLOW_SPRITES, 2, false, 0.85f);
	else if (item->bAdditive == 2)
		glowRenderer.Begin (GLOW_SPRITES, 1, false, 0.95f);
	else {
		bGlow = 0;
		if (glowRenderer.End ())
			ResetBitmaps ();
		}
	}

ogl.ResetClientStates (1);
m_data.bmP [1] = m_data.bmP [2] = NULL;
m_data.bUseLightmaps = 0;
ogl.SelectTMU (GL_TEXTURE0, true);
#if 1
if (!LoadTexture (item->bmP, 0, 0, 0, GL_CLAMP))
	return;
#else
ogl.SetTexturing (true);
ResetBitmaps ();
#endif
if (item->bColor)
	glColor4fv (reinterpret_cast<GLfloat*> (&item->color));
else
	glColor3f (1,1,1);
ogl.SetBlendMode (item->bAdditive);
if (!(bSoftBlend && glareRenderer.LoadShader (item->fSoftRad, item->bAdditive != 0)))
	shaderManager.Deploy (-1);
item->bmP->SetColor ();
CFloatVector vPosf;
transformation.Transform (vPosf, item->position, 0);
if (!bGlow || glowRenderer.SetViewport (GLOW_SPRITES, *vPosf.XYZ (), X2F (item->nWidth), X2F (item->nHeight), true))
	ogl.RenderQuad (item->bmP, vPosf, X2F (item->nWidth), X2F (item->nHeight), 3);
#endif
}

//------------------------------------------------------------------------------

typedef struct tSparkVertex {
	CFloatVector3		vPos;
	tTexCoord2f	texCoord;
} tSparkVertex;

#define SPARK_BUF_SIZE	1000

typedef struct tSparkBuffer {
	int				nSparks;
	tSparkVertex	info [SPARK_BUF_SIZE * 4];
} tSparkBuffer;

tSparkBuffer sparkBuffer;
CEffectArea sparkArea;

//------------------------------------------------------------------------------

void CTransparencyRenderer::FlushSparkBuffer (void)
{
if (!sparkBuffer.nSparks)
	return;

sparkArea.Reset ();

	int bSoftBlend = SoftBlend (2);

ogl.ResetClientStates (1);
m_data.bmP [1] = m_data.bmP [2] = NULL;
m_data.bUseLightmaps = 0;
ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
if (LoadTexture (sparks.Bitmap (), 0, 0, 0, GL_CLAMP)) {
	if (!(bSoftBlend && glareRenderer.LoadShader (3, 1)))
		shaderManager.Deploy (-1);
	ogl.SetBlendMode (1);
	glColor3f (1,1,1);
	OglTexCoordPointer (2, GL_FLOAT, sizeof (tSparkVertex), &sparkBuffer.info [0].texCoord);
	OglVertexPointer (3, GL_FLOAT, sizeof (tSparkVertex), &sparkBuffer.info [0].vPos);
	OglDrawArrays (GL_QUADS, 0, 4 * sparkBuffer.nSparks);
	ogl.SetBlendMode (0);
	ogl.SetDepthTest (true);
	sparkBuffer.nSparks = 0;
	}
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderSpark (tTranspSpark *item)
{
if (sparkBuffer.nSparks >= SPARK_BUF_SIZE)
	FlushSparkBuffer ();


	tSparkVertex	*infoP = sparkBuffer.info + 4 * sparkBuffer.nSparks;
	CFloatVector	vPos;
	float				nSize = X2F (item->nSize);
	float				nCol = (float) (item->nFrame / 8);
	float				nRow = (float) (item->nFrame % 8);

sparkArea.Add (item->position, nSize);

transformation.Transform (vPos, item->position, 0);
if (!item->nType)
	nCol += 4;
infoP->vPos.v.coord.x = vPos.v.coord.x - nSize;
infoP->vPos.v.coord.y = vPos.v.coord.y + nSize;
infoP->vPos.v.coord.z = vPos.v.coord.z;
infoP->texCoord.v.u = nCol / 8.0f;
infoP->texCoord.v.v = (nRow + 1) / 8.0f;
infoP++;
infoP->vPos.v.coord.x = vPos.v.coord.x + nSize;
infoP->vPos.v.coord.y = vPos.v.coord.y + nSize;
infoP->vPos.v.coord.z = vPos.v.coord.z;
infoP->texCoord.v.u = (nCol + 1) / 8.0f;
infoP->texCoord.v.v = (nRow + 1) / 8.0f;
infoP++;
infoP->vPos.v.coord.x = vPos.v.coord.x + nSize;
infoP->vPos.v.coord.y = vPos.v.coord.y - nSize;
infoP->vPos.v.coord.z = vPos.v.coord.z;
infoP->texCoord.v.u = (nCol + 1) / 8.0f;
infoP->texCoord.v.v = nRow / 8.0f;
infoP++;
infoP->vPos.v.coord.x = vPos.v.coord.x - nSize;
infoP->vPos.v.coord.y = vPos.v.coord.y - nSize;
infoP->vPos.v.coord.z = vPos.v.coord.z;
infoP->texCoord.v.u = nCol / 8.0f;
infoP->texCoord.v.v = nRow / 8.0f;
sparkBuffer.nSparks++;
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderSphere (tTranspSphere *item)
{
ogl.ResetClientStates ();
shaderManager.Deploy (-1);
if (item->nType == riSphereShield) {
	DrawShieldSphere (item->objP, item->color.red, item->color.green, item->color.blue, item->color.alpha, item->bAdditive, item->nSize);
	}
else if (item->nType == riMonsterball) {
	if (glowRenderer.End ())
		ResetBitmaps ();
	DrawMonsterball (item->objP, item->color.red, item->color.green, item->color.blue, item->color.alpha);
	}
shaderManager.Deploy (-1);
ResetBitmaps ();
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderBullet (CParticle *pParticle)
{
	CObject	o;

memset (&o, 0, sizeof (o));
o.info.nType = OBJ_POWERUP;
o.info.position.vPos = pParticle->m_vPos;
o.info.position.mOrient = pParticle->m_mOrient;
if (0 <= (o.info.nSegment = FindSegByPos (o.info.position.vPos, pParticle->m_nSegment, 0, 0))) {
	lightManager.Index (0,0).nActive = 0;
	o.info.renderType = RT_POLYOBJ;
	o.rType.polyObjInfo.nModel = BULLET_MODEL;
	o.rType.polyObjInfo.nTexOverride = -1;
	DrawPolygonObject (&o, 1);
	gameData.models.vScale.SetZero ();
	ResetBitmaps ();
	}
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderParticle (tTranspParticle *item)
{
if (item->particle->m_nType == BULLET_PARTICLES)
	CTransparencyRenderer::RenderBullet (item->particle);
else {
	if (item->particle->Render (item->fBrightness) < 0)
		ResetBitmaps ();
	}
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderLightning (tTranspLightning *item)
{
if (m_data.nPrevType != m_data.nCurType) {
	ogl.ResetClientStates ();
	shaderManager.Deploy (-1);
	}
item->lightning->Render (item->nDepth, 0);
ResetBitmaps ();
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderLightTrail (tTranspLightTrail *item)
{
if (m_data.nPrevType != m_data.nCurType) {
	ogl.ResetClientStates (1);
	m_data.bmP [1] = m_data.bmP [2] = NULL;
	m_data.bUseLightmaps = 0;
	shaderManager.Deploy (-1);
	}
glowRenderer.Begin (GLOW_LIGHTTRAILS, 2, false);
ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
if (LoadTexture (item->bmP, 0, 0, 0, GL_CLAMP)) {
	ogl.SetDepthWrite (true);
	ogl.SetFaceCulling (false);
	ogl.SetBlendMode (1);
	glColor4fv (reinterpret_cast<GLfloat*> (&item->color));
	OglTexCoordPointer (2, GL_FLOAT, 0, item->texCoord);
	OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), item->vertices);
	ogl.SetupTransform (1);
	OglDrawArrays (GL_QUADS, 0, item->bTrail ? 8 : 4);
	ogl.ResetTransform (1);
	ogl.SetFaceCulling (true);
	ogl.SetBlendMode (0);
	}
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderThruster (tTranspThruster *item)
{
shaderManager.Deploy (-1);
ogl.ResetClientStates ();
thrusterFlames.Render (item->objP, &item->info, item->nThruster);
gameData.models.vScale.SetZero ();
ResetBitmaps ();
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::FlushParticleBuffer (int nType)
{
if ((nType < 0) || ((nType != tiParticle) && (particleManager.LastType () >= 0))) {
	ResetBitmaps ();
	if (sparkBuffer.nSparks && particleManager.Overlap (sparkArea))
		FlushSparkBuffer ();
	if (particleManager.Flush (-1.0f, true)) {
		if (nType < 0)
			particleManager.CloseBuffer ();
		particleManager.SetLastType (-1);
		m_data.bClientColor = 1;
		ResetBitmaps ();
		}
	}
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::FlushBuffers (int nType, struct tTranspItem *itemP)
{
if (glowRenderer.Available (GLOW_LIGHTNING | GLOW_SHIELDS | GLOW_SPRITES | GLOW_THRUSTERS) && 
	 (nType != tiLightning) && (nType != tiSphere) && (nType != tiSprite) && (nType != tiThruster)) {
	if (glowRenderer.End ())
		ResetBitmaps ();
	}
if (nType == tiSpark) {
	if (!itemP || particleManager.Overlap (itemP->item.spark.position, X2F (itemP->item.spark.nSize)))
		FlushParticleBuffer (nType);
	}
else if (nType == tiParticle) {
	if (!itemP || sparkArea.Overlap (itemP->item.particle.particle->Posf (), itemP->item.particle.particle->Rad ()))
		FlushSparkBuffer ();
	}	
else {
	FlushSparkBuffer ();
	FlushParticleBuffer (nType);
	}
}

//------------------------------------------------------------------------------

tTranspItem* CTransparencyRenderer::SetParent (int nChild, int nParent)
{
if (nChild < 0)
	return NULL;
tTranspItem	*childP = m_data.itemLists + nChild;
if (!childP->bValid)
	return NULL;
if (nParent < 0)
	return NULL;
tTranspItem	*parentP = m_data.itemLists + nParent;
if (!parentP->bValid)
	return NULL;
childP->parentP = parentP;
return parentP;
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::RenderItem (struct tTranspItem *itemP)
{
if (!itemP->bRendered) {
	//itemP->bRendered = true;
	m_data.nPrevType = m_data.nCurType;
	m_data.nCurType = itemP->nType;
#if DBG
	if (gameOpts->render.debug.bTextures && gameOpts->render.debug.bWalls)
#endif
	try {
		FlushBuffers (m_data.nCurType, itemP);
		ogl.SetBlendMode (0);
		ogl.SetDepthWrite (false);
		ogl.SetDepthMode (GL_LEQUAL);
		ogl.SetDepthTest (true);
		if (m_data.nCurType == tiFace) {
			RenderFace (&itemP->item.poly);
			}
		if (m_data.nCurType == tiPoly) {
			RenderPoly (&itemP->item.poly);
			}
		else if (m_data.nCurType == tiObject) {
			RenderObject (&itemP->item.object);
			}
		else if (m_data.nCurType == tiSprite) {
			RenderSprite (&itemP->item.sprite);
			}
		else if (m_data.nCurType == tiSpark) {
			RenderSpark (&itemP->item.spark);
			}
		else if (m_data.nCurType == tiSphere) {
			RenderSphere (&itemP->item.sphere);
			}
		else if (m_data.nCurType == tiParticle) {
			if (m_data.bHaveParticles)
				RenderParticle (&itemP->item.particle);
			}
		else if (m_data.nCurType == tiLightning) {
			RenderLightning (&itemP->item.lightning);
			}
		else if (m_data.nCurType == tiLightTrail) {
			RenderLightTrail (&itemP->item.lightTrail);
			}
		else if (m_data.nCurType == tiThruster) {
			RenderThruster (&itemP->item.thruster);
			}
		}
	catch(...) {
		PrintLog ("invalid transparent render item (type: %d)\n", m_data.nCurType);
		}
	if (itemP->parentP)
		RenderItem (itemP->parentP);
	}
return m_data.nCurType;
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::NeedDepthBuffer (void)
{
if (!gameOpts->render.bUseShaders)
	return 0;
if (!gameOpts->render.effects.bEnabled)
	return 0;
if (!ogl.m_states.bGlowRendering)
	return true;
if (gameStates.render.cameras.bActive)
	return 0;
return int (m_data.bRenderGlow || m_data.bSoftBlend);
}

//------------------------------------------------------------------------------

int CTransparencyRenderer::SoftBlend (int nFlag)
{
return m_data.bHaveDepthBuffer && !gameStates.render.cameras.bActive && ((gameOpts->render.effects.bSoftParticles & nFlag) != 0);
}

//------------------------------------------------------------------------------

extern int bLog;

void CTransparencyRenderer::Render (int nWindow)
{
#if RENDER_TRANSPARENCY
	tTranspItem*		currentP, * nextP, * prevP;
	tTranspItemList*	listP;
	int					nItems, nDepth, bStencil;
	bool					bCleanup = !LAZY_RESET || (ogl.StereoSeparation () >= 0) || nWindow;

if (!(m_data.depthBuffer.Buffer () && (m_data.nFreeItems < ITEM_BUFFER_SIZE))) {
	return;
	}
PROF_START
gameStates.render.nType = RENDER_TYPE_TRANSPARENCY;
shaderManager.Deploy (-1);
bStencil = ogl.StencilOff ();
ResetBitmaps ();
m_data.bTextured = -1;
m_data.bUseLightmaps = 0;
m_data.bDecal = 0;
m_data.bLightmaps = lightmapManager.HaveLightmaps ();
m_data.bSplitPolys = (gameStates.render.bPerPixelLighting != 2) && (gameStates.render.bSplitPolys > 0);
m_data.nWrap = 0;
m_data.nFrame = -1;
m_data.bmP [0] =
m_data.bmP [1] = NULL;
if (!glowRenderer.Available (0xFFFFFFFF))
	m_data.bRenderGlow = 0;
if (gameOptions [0].render.nQuality < 3)
	m_data.bSoftBlend = 0;
sparkBuffer.nSparks = 0;
ogl.DisableLighting ();
ogl.ResetClientStates ();
shaderManager.Deploy (-1);
currentP = &m_data.itemLists [ITEM_BUFFER_SIZE - 1];
m_data.bHaveParticles = particleImageManager.LoadAll ();
ogl.SetBlendMode (0);
ogl.SetDepthMode (GL_LEQUAL);
ogl.SetFaceCulling (true);
m_data.bHaveDepthBuffer = NeedDepthBuffer () && ogl.CopyDepthTexture (1);
particleManager.BeginRender (-1, 1);
m_data.nCurType = -1;

for (listP = m_data.depthBuffer + m_data.nMaxOffs, nItems = m_data.nItems [0]; (listP >= m_data.depthBuffer.Buffer ()) && nItems; listP--) {
	if ((currentP = listP->head)) {
		if (bCleanup)
			listP->head = NULL;
			//listP->tail = NULL;
		nDepth = 0;
		prevP = NULL;
		do {
#if DBG
			if (currentP->nItem == nDbgItem)
				nDbgItem = nDbgItem;
#endif
			nItems--;
			RenderItem (currentP);
			nextP = currentP->pNextItem;
			if (bCleanup)
				currentP->pNextItem = NULL;
			else if (currentP->bTransformed) {	// remove items that have transformed coordinates when stereo rendering since these items will be reentered with different coordinates
				currentP->pNextItem = m_data.freeList.head;
				m_data.freeList.head = currentP;
				m_data.nFreeItems++;
				if (prevP)
					prevP->pNextItem = nextP;
				else
					listP->head = nextP;
				}
			else
				prevP = currentP;
			currentP = nextP;
			nDepth++;
			} while (currentP);
		}
	}

#if 0 //DBG
if (bCleanup) {
	currentP = m_data.itemLists.Buffer ();
	for (int i = m_data.itemLists.Length (); i; i--, currentP++)
		if (currentP->pNextItem)
			currentP->pNextItem = NULL;
	}
#endif
FlushBuffers (-1);
particleManager.EndRender ();
shaderManager.Deploy (-1);
ogl.ResetClientStates ();
ogl.SetTexturing (false);
ogl.SetBlendMode (0);
ogl.SetDepthMode (GL_LEQUAL);
ogl.SetDepthWrite (true);
ogl.StencilOn (bStencil);
if (bCleanup) {
	ResetFreeList ();
	m_data.nItems [1] = 	m_data.nItems [0];
	m_data.nItems [0] = 0;
	m_data.nMinOffs = ITEM_DEPTHBUFFER_SIZE;
	m_data.nMaxOffs = 0;
	m_data.nFreeItems = ITEM_BUFFER_SIZE;
	}
glowRenderer.End ();
PROF_END(ptTranspPolys)
#endif
}

//------------------------------------------------------------------------------
//eof
