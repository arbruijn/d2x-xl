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

#include "descent.h"
#include "error.h"
#include "gamefont.h"
#include "texmap.h"
#include "rendermine.h"
#include "fastrender.h"
#include "rendershadows.h"
#include "transprender.h"
#include "renderthreads.h"
#include "glare.h"
#include "cockpit.h"
#include "radar.h"
#include "objrender.h"
#include "textures.h"
#include "screens.h"
#include "segpoint.h"
#include "texmerge.h"
#include "physics.h"
#include "segmath.h"
#include "light.h"
#include "dynlight.h"
#include "headlight.h"
#include "newdemo.h"
#include "automap.h"
#include "key.h"
#include "u_mem.h"
#include "kconfig.h"
#include "mouse.h"
#include "interp.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_render.h"
#include "ogl_fastrender.h"
#include "cockpit.h"
#include "input.h"
#include "shadows.h"
#include "textdata.h"
#include "sparkeffect.h"
#include "createmesh.h"

//------------------------------------------------------------------------------

#define bPreDrawSegs			0

#define VIS_CULLING			1
#define OLD_SEGLIST			1
#define SORT_RENDER_SEGS	0

//------------------------------------------------------------------------------

void SortSidesByDist (double *sideDists, char *sideNums, int32_t left, int32_t right)
{
	int32_t		l = left;
	int32_t		r = right;
	int32_t		h, m = (l + r) / 2;
	double	hd, md = sideDists [m];

do {
	while (sideDists [l] < md)
		l++;
	while (sideDists [r] > md)
		r--;
	if (l <= r) {
		if (l < r) {
			h = sideNums [l];
			sideNums [l] = sideNums [r];
			sideNums [r] = h;
			hd = sideDists [l];
			sideDists [l] = sideDists [r];
			sideDists [r] = hd;
			}
		++l;
		--r;
		}
	} while (l <= r);
if (right > l)
   SortSidesByDist (sideDists, sideNums, l, right);
if (r > left)
   SortSidesByDist (sideDists, sideNums, left, r);
}

//------------------------------------------------------------------------------

uint8_t CodePortalPoint (fix x, fix y, tPortal& curPortal)
{
	uint8_t code = 0;

if (x <= curPortal.left)
	code |= 1;
else if (x >= curPortal.right)
	code |= 2;
if (y <= curPortal.top)
	code |= 4;
else if (y >= curPortal.bot)
	code |= 8;
return code;
}

//------------------------------------------------------------------------------

uint8_t CodePortal (tPortal& facePortal, tPortal& curPortal)
{
	uint8_t code = 0;

if (facePortal.right < curPortal.left)
	code |= 1;
else if (facePortal.left > curPortal.right)
	code |= 2;
if (facePortal.bot < curPortal.top)
	code |= 4;
else if (facePortal.top > curPortal.bot)
	code |= 8;
return code;
}

//------------------------------------------------------------------------------

//tPortal renderPortals [MAX_SEGMENTS_D2X];
#if !OLD_SEGLIST
tPortal sidePortals [MAX_SEGMENTS_D2X * 6];
#endif
//uint8_t bVisible [MAX_SEGMENTS_D2X];

//Given two sides of CSegment, tell the two verts which form the
//edge between them
int16_t edgeBetweenTwoSides [6][6][2] = {
	{{-1, -1}, {3, 7}, {-1, -1}, {2, 6}, {6, 7}, {2, 3}},
	{{3, 7}, {-1, -1}, {0, 4}, {-1, -1}, {4, 7}, {0, 3}},
	{{-1, -1}, {0, 4}, {-1, -1}, {1, 5}, {4, 5}, {0, 1}},
	{{2, 6}, {-1, -1}, {1, 5}, {-1, -1}, {5, 6}, {1, 2}},
	{{6, 7}, {4, 7}, {4, 5}, {5, 6}, {-1, -1}, {-1, -1}},
	{{2, 3}, {0, 3}, {0, 1}, {1, 2}, {-1, -1}, {-1, -1}}
	};

//given an edge specified by two verts, give the two sides on that edge
int32_t edgeToSides [8][8][2] = {
	{{-1, -1}, {2, 5}, {-1, -1}, {1, 5}, {1, 2}, {-1, -1}, {-1, -1}, {-1, -1}},
	{{2, 5}, {-1, -1}, {3, 5}, {-1, -1}, {-1, -1}, {2, 3}, {-1, -1}, {-1, -1}},
	{{-1, -1}, {3, 5}, {-1, -1}, {0, 5}, {-1, -1}, {-1, -1}, {0, 3}, {-1, -1}},
	{{1, 5}, {-1, -1}, {0, 5}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {0, 1}},
	{{1, 2}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {2, 4}, {-1, -1}, {1, 4}},
	{{-1, -1}, {2, 3}, {-1, -1}, {-1, -1}, {2, 4}, {-1, -1}, {3, 4}, {-1, -1}},
	{{-1, -1}, {-1, -1}, {0, 3}, {-1, -1}, {-1, -1}, {3, 4}, {-1, -1}, {0, 4}},
	{{-1, -1}, {-1, -1}, {-1, -1}, {0, 1}, {1, 4}, {-1, -1}, {0, 4}, {-1, -1}},
	};

//------------------------------------------------------------------------------
//given an edge and one side adjacent to that edge, return the other adjacent CSide

#if SORT_RENDER_SEGS

int32_t FindOtherSideOnEdge (CSegment *pSeg, int16_t *verts, int32_t oppSide)
{
	int32_t	i;
	int32_t	i0 = -1, i1 = -1;
	int32_t	side0, side1;
	int32_t	*eptr;
	int32_t	sv, v0, v1;
	int16_t	*vp;

//@@	check_check();

v0 = verts [0];
v1 = verts [1];
vp = seg->verts;
for (i = 0; i < 8; i++) {
	sv = *vp++;	// seg->verts [i];
	if (sv == v0) {
		i0 = i;
		if (i1 != -1)
			break;
		}
	if (sv == v1) {
		i1 = i;
		if (i0 != -1)
			break;
		}
	}
eptr = edgeToSides [i0][i1];
side0 = eptr [0];
side1 = eptr [1];
return (side0 == oppSide) ? side1 : side0;
}

//------------------------------------------------------------------------------
//find the two segments that join a given seg through two sides, and
//the sides of those segments the abut.

typedef struct tSideNormData {
	CFixVector	n [2];
	CFixVector	*facePortal;
	int16_t			t;
} tSideNormData;

int32_t FindAdjacentSideNorms (CSegment *pSeg, int16_t s0, int16_t s1, tSideNormData *s)
{
	CSegment	*seg0, *seg1;
	CSide		*side0, *side1;
	int16_t		edgeVerts [2];
	int32_t		oppSide0, oppSide1;
	int32_t		otherSide0, otherSide1;

Assert(s0 != -1 && s1 != -1);
seg0 = SEGMENT (pSeg->m_children [s0]);
seg1 = SEGMENT (pSeg->m_children [s1]);
edgeVerts [0] = pSeg->m_vertices [edgeBetweenTwoSides [s0][s1][0]];
edgeVerts [1] = pSeg->m_vertices [edgeBetweenTwoSides [s0][s1][1]];
Assert(edgeVerts [0] != -1 && edgeVerts [1] != -1);
oppSide0 = pSeg->ConnectedSide (seg0);
Assert (oppSide0 != -1);
oppSide1 = pSeg->ConnectedSide (seg1);
Assert (oppSide1 != -1);
otherSide0 = FindOtherSideOnEdge (seg0, edgeVerts, oppSide0);
otherSide1 = FindOtherSideOnEdge (seg1, edgeVerts, oppSide1);
side0 = seg0->m_sides + otherSide0;
side1 = seg1->m_sides + otherSide1;
memcpy (s [0].n, side0->m_normals, 2 * sizeof (CFixVector));
memcpy (s [1].n, side1->m_normals, 2 * sizeof (CFixVector));
s [0].facePortal = gameData.segData.vertices + seg0->m_vertices [sideVertIndex [otherSide0][(s [0].t = side0->m_nType) == 3]];
s [1].facePortal = gameData.segData.vertices + seg1->m_vertices [sideVertIndex [otherSide1][(s [1].t = side1->m_nType) == 3]];
return 1;
}

//------------------------------------------------------------------------------
//see if the order matters for these two children.
//returns 0 if order doesn't matter, 1 if c0 before c1, -1 if c1 before c0
static int32_t CompareChildren (CSegment *pSeg, int16_t c0, int16_t c1)
{
	tSideNormData	s [2];
	CFixVector		temp;
	fix				d0, d1;

if (oppSideTable [c0] == c1)
	return 0;
//find normals of adjoining sides
FindAdjacentSideNorms (pSeg, c0, c1, s);
temp = gameData.render.mine.viewer.vPos - *s [0].facePortal;
d0 = CFixVector::Dot (s [0].n [0], temp);
if (s [0].t != 1)	// triangularized face -> 2 different normals
	d0 |= CFixVector::Dot (s [0].n [1], temp);	// we only need the sign, so a bitwise or does the trick
temp = gameData.render.mine.viewer.vPos - *s [1].facePortal;
d1 = CFixVector::Dot (s [1].n [0], temp);
if (s [1].t != 1)
	d1 |= CFixVector::Dot (s [1].n [1], temp);
if ((d0 & d1) < 0)	// only < 0 if both negative due to bitwise and
	return 0;
if (d0 < 0)
	return 1;
 if (d1 < 0)
	return -1;
return 0;
}

//------------------------------------------------------------------------------

int32_t QuickSortSegChildren (CSegment *pSeg, int16_t left, int16_t right, int16_t *childList)
{
	int16_t	h,
			l = left,
			r = right,
			mat = (l + r) / 2,
			median = childList [mat],
			bSwap = 0;

do {
	while ((l < mat) && CompareChildren (pSeg, childList [l], median) >= 0)
		l++;
	while ((r > mat) && CompareChildren (pSeg, childList [r], median) <= 0)
		r--;
	if (l <= r) {
		if (l < r) {
			h = childList [l];
			childList [l] = childList [r];
			childList [r] = h;
			bSwap = 1;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	bSwap |= QuickSortSegChildren (pSeg, l, right, childList);
if (left < r)
	bSwap |= QuickSortSegChildren (pSeg, left, r, childList);
return bSwap;
}

//------------------------------------------------------------------------------

//int16_t the children of CSegment to render in the correct order
//returns non-zero if swaps were made
static inline int32_t SortSegChildren (CSegment *pSeg, int32_t nChildren, int16_t *childList)
{
#if 1

if (nChildren < 2)
	return 0;
if (nChildren == 2) {
	if (CompareChildren (pSeg, childList [0], childList [1]) >= 0)
		return 0;
	int16_t h = childList [0];
	childList [0] = childList [1];
	childList [1] = h;
	return 1;
	}
return QuickSortSegChildren (pSeg, (int16_t) 0, (int16_t) (nChildren - 1), childList);

#else
	int32_t i, j;
	int32_t r;
	int32_t made_swaps, count;

if (nChildren < 2)
	return 0;
	//for each child,  compare with other children and see if order matters
	//if order matters, fix if wrong

count = 0;

do {
	made_swaps = 0;

	for (i=0;i<nChildren-1;i++)
		for (j=i+1;childList [i]!=-1 && j<nChildren;j++)
			if (childList [j]!=-1) {
				r = CompareChildren(pSeg, childList [i], childList [j]);

				if (r == 1) {
					int32_t temp = childList [i];
					childList [i] = childList [j];
					childList [j] = temp;
					made_swaps=1;
				}
			}

} while (made_swaps && ++count<nChildren);
return count;
#endif
}

#endif //SORT_RENDER_SEGS

//------------------------------------------------------------------------------

void UpdateRenderedData (int32_t nWindow, CObject *viewer, int32_t rearViewFlag, int32_t user)
{
Assert(nWindow < MAX_RENDERED_WINDOWS);
windowRenderedData [nWindow].nFrame = gameData.app.nFrameCount;
windowRenderedData [nWindow].pViewer = viewer;
windowRenderedData [nWindow].bRearView = rearViewFlag;
windowRenderedData [nWindow].nUser = user;
}

//------------------------------------------------------------------------------

void AddObjectToSegList (int16_t nObject, int16_t nSegment)
{
	tObjRenderListItem *pi = gameData.render.mine.objRenderList.objs + gameData.render.mine.objRenderList.nUsed;

#if DBG
if (nObject == nDbgObj)
	BRP;
#endif
CObject *pObj = OBJECT (nObject);
if (pObj) {
	pi->nNextItem = gameData.render.mine.objRenderList.ref [nSegment];
	gameData.render.mine.objRenderList.ref [nSegment] = gameData.render.mine.objRenderList.nUsed++;
	pi->nObject = nObject;
	pi->xDist = CFixVector::Dist (pObj->Position (), gameData.render.mine.viewer.vPos);
	pObj->SetFrame (gameData.app.nFrameCount);
	}
}

//------------------------------------------------------------------------------

int16_t CObject::Visible (void)
{
	int16_t	segList [MAX_SEGMENTS_D2X];
	uint8_t bVisited [MAX_SEGMENTS_D2X];
	int16_t	head = 0, tail = 0;

memset (bVisited, 0, sizeof (bVisited [0]) * gameData.segData.nSegments);
segList [tail++] = Segment ();
while (head != tail) {
	CSegment* pSeg = SEGMENT (segList [head++]);
	for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
		int16_t nSegment = pSeg->m_children [i];
		if (nSegment < 0)
			continue;
		if (bVisited [nSegment])
			continue;
		CSegment* pChildSeg = SEGMENT (nSegment);
		// quick check whether object could reach into this child segment
		if (CFixVector::Dist (Position (), pChildSeg->Center ()) >= info.xSize + pChildSeg->MaxRad ())
			continue;
		// check whether the object actually penetrates the side between the current segment and the child segment
		CSegMasks mask = SEGMENT (info.nSegment)->Masks (Position (), info.xSize);
		if (!(mask.m_side & (1 << i)))
			continue;
		if (gameData.render.mine.Visible (nSegment))
			return nSegment;
		segList [tail++] = nSegment;
		bVisited [nSegment] = 1;
		}
	}
return -1;
}

//------------------------------------------------------------------------------

void GatherLeftoutVisibleObjects (void)
{
#if 1
	CObject* pObj;

FORALL_OBJS (pObj) {
	if (pObj->Type () >= MAX_OBJECT_TYPES) 
		continue;
	if (pObj->Segment () < 0)
		continue;
	if (pObj->Frame () == gameData.app.nFrameCount)
		continue;
	int16_t nSegment = pObj->Visible ();
	if (nSegment < 0)
		continue;
	AddObjectToSegList (pObj->Index (), nSegment);
	}
#endif
}

//------------------------------------------------------------------------------

void BuildRenderObjLists (int32_t nSegCount, int32_t nThread)
{
PROF_START
	CObject*		pObj;
	CSegMasks	mask;
	int16_t			nSegment, nNewSeg, nChild, nSide, sideFlag;
	int32_t			nListPos;
	int16_t			nObject;

gameData.render.mine.objRenderList.ref.Clear (char (0xff));
gameData.render.mine.objRenderList.nUsed = 0;

for (nListPos = 0; nListPos < nSegCount; nListPos++) {
	nSegment = gameData.render.mine.visibility [nThread].segments [nListPos];
	if (nSegment == -0x7fff)
		continue;
#if DBG
	if (nSegment == nDbgSeg)
		BRP;
#endif
	for (nObject = SEGMENT (nSegment)->m_objects; nObject != -1; nObject = pObj->info.nNextInSeg) {
#if DBG
		if (nObject == nDbgObj)
			BRP;
#endif
		pObj = OBJECT (nObject);
		//Assert (pObj->info.nSegment == nSegment);
		if (pObj->info.nFlags & OF_ATTACHED)
			continue;		//ignore this CObject
		nNewSeg = nSegment;
		if ((pObj->info.nType != OBJ_REACTOR) && ((pObj->info.nType != OBJ_ROBOT) || (pObj->info.nId == 65))) { //don't migrate controlcen
			CSegment* pNewSeg = SEGMENT (nNewSeg);
			mask = pNewSeg->Masks (OBJPOS (pObj)->vPos, pObj->info.xSize);
			if (mask.m_side) {
				for (nSide = 0, sideFlag = 1; nSide < SEGMENT_SIDE_COUNT; nSide++, sideFlag <<= 1) {
					if (!(mask.m_side & sideFlag))
						continue;
					if (pNewSeg->IsPassable (nSide, NULL) & WID_PASSABLE_FLAG) {	//can explosion migrate through
						nChild = pNewSeg->m_children [nSide];
						if (gameData.render.mine.Visible (nChild, nThread)) {
							nNewSeg = nChild;	// only migrate to segment in render list
#if DBG
							if (nNewSeg == nDbgSeg)
								BRP;
#endif
							}
						}
					}
				}
			}
		AddObjectToSegList (nObject, nNewSeg);
		}
	}
//if (CollisionModel ())
	GatherLeftoutVisibleObjects ();
PROF_END(ptBuildObjList)
}

//------------------------------------------------------------------------------

void CalcRenderDepth (void)
{
CFixVector vCenter;
transformation.Transform (vCenter, SEGMENT (gameData.objData.pViewer->info.nSegment)->Center (), 0);
CFixVector v;
transformation.Transform (v, gameData.segData.vMin, 0);
fix d1 = CFixVector::Dist (v, vCenter);
transformation.Transform (v, gameData.segData.vMax, 0);
fix d2 = CFixVector::Dist (v, vCenter);

if (d1 < d2)
	d1 = d2;
fix r = gameData.segData.segRads [1][gameData.objData.pViewer->info.nSegment];
gameData.render.zMin = 0;
gameData.render.zMax = vCenter.v.coord.z + d1 + r;
}

//------------------------------------------------------------------------------

void GetMaxDepth (void)
{
gameData.render.zMax = 0;
for (int32_t i = 0; i < gameStates.app.nThreads; i++) {
	if (gameData.render.zMax < tiRender.zMax [i])
		gameData.render.zMax = tiRender.zMax [i];
	if (gameData.render.zMin > tiRender.zMin [i])
		gameData.render.zMin = tiRender.zMin [i];
	}
}

//------------------------------------------------------------------------------
//build a list of segments to be rendered
//fills in gameData.render.mine.renderSegList & gameData.render.mine.visibility [0].nSegments

//static tSegZRef zRef [2][MAX_SEGMENTS_D2X];

void CVisibilityData::QSortZRef (int16_t left, int16_t right)
{
	tSegZRef	*ps = &zRef [0][0];
	tSegZRef	m = ps [(left + right) / 2];
	tSegZRef	h;
	int16_t		l = left,
				r = right;
do {
	while ((ps [l].z > m.z))// || ((zRef [l].z == m.z) && (zRef [l].d > m.d)))
		l++;
	while ((ps [r].z < m.z))// || ((zRef [r].z == m.z) && (zRef [r].d < m.d)))
		r--;
	if (l <= r) {
		if (l < r) {
			h = ps [l];
			ps [l] = ps [r];
			ps [r] = h;
			}
		l++;
		r--;
		}
	}
while (l < r);
if (l < right)
	QSortZRef (l, right);
if (left < r)
	QSortZRef (left, r);
}

//------------------------------------------------------------------------------

void CVisibilityData::InitZRef (int32_t i, int32_t j, int32_t nThread)
{
	tSegZRef*		ps = &zRef [0][i];
	CFloatVector	vViewer, vCenter;
	int32_t				r, z, zMin = 0x7fffffff, zMax = -0x7fffffff;
	CSegment*		pSeg;

vViewer.Assign (gameData.render.mine.viewer.vPos);
for (; i < j; i++, ps++) {
	pSeg = SEGMENT (segments [i]);
#if TRANSP_DEPTH_HASH
	vCenter.Assign (pSeg->Center ());
	float d = CFloatVector::Dist (vCenter, vViewer);
	z = F2X (d);
	r = pSeg->MaxRad ();
	if (zMax < z + r)
		zMax = z + r;
	if (zMin > z - r)
		zMin = z - r;
	ps->z = z;
#else
	CFixVector dir = pSeg->Center ();
	transformation.Transform (dir, dir, 0);
	dir.dir.coord.z += pSeg->MaxRad ();
	if (zMin > dir.dir.coord.z)
		zMin = dir.dir.coord.z;
	if (zMax < dir.dir.coord.z)
		zMax = dir.dir.coord.z;
	ps->z = dir.dir.coord.z;
#endif
	ps->nSegment = segments [i];
	}
tiRender.zMin [nThread] = zMin;
tiRender.zMax [nThread] = zMax;
}

//------------------------------------------------------------------------------

void CVisibilityData::MergeZRef (void)
{
	tSegZRef	*ps, *pi, *pj;
	int32_t		h, i, j;

h = nSegments;
for (i = h / 2, j = h - i, ps = &zRef [1][0], pi = &zRef [0][0], pj = pi + h / 2; h; h--) {
	if (i && (!j || (pi->z < pj->z))) {
		*ps++ = *pi++;
		i--;
		}
	else if (j) {
		*ps++ = *pj++;
		j--;
		}
	}
}

//------------------------------------------------------------------------------

void CVisibilityData::Sort (void)
{
if (nSegments < 2)
	return;

#if USE_OPENMP //> 1

	int32_t h, i, j;

if (gameStates.app.nThreads < 2) {
	InitZRef (0, nSegments, 0);
	gameData.render.zMin = tiRender.zMin [0];
	gameData.render.zMax = tiRender.zMax [0];
	QSortZRef (0, nSegments - 1);
	}
else {
#pragma omp parallel for private (i, j)
	for (h = 0; h < gameStates.app.nThreads; h++) {
		ComputeThreadRange (h, nSegments, i, j);
		InitZRef (i, j, h);
		}
	}
GetMaxDepth ();
#else
if (RunRenderThreads (rtInitSegZRef)) 
	GetMaxDepth ();
else {
	InitZRef (0, gameData.render.mine.visibility [0].nSegments, 0);
	gameData.render.zMin = tiRender.zMin [0];
	gameData.render.zMax = tiRender.zMax [0];
	}
#endif
}

//------------------------------------------------------------------------------

int32_t CVisibilityData::BuildAutomapSegList (void)
{
if (!(automap.Active () && gameOpts->render.automap.bTextured && !automap.Radar ()))
	return 0;

	int32_t nSegmentLimit = automap.SegmentLimit ();
	int32_t bUnlimited = nSegmentLimit == automap.MaxSegsAway ();
	int32_t bSkyBox = gameOpts->render.automap.bSkybox;

for (int32_t i = nSegments = 0; i < gameData.segData.nSegments; i++)
	if (automap.Visible (i)
		 && (bSkyBox || (SEGMENT (i)->m_function != SEGMENT_FUNC_SKYBOX)) 
		 && (bUnlimited || (automap.m_visible [i] <= nSegmentLimit))) {
		segments [nSegments++] = i;
		bVisible [i] = nVisible;
		Visit (i);
		}
Sort ();
return 1;
}

//------------------------------------------------------------------------------

void CVisibilityData::BuildSegList (CTransformation& transformation, int16_t nStartSeg, int32_t nWindow, bool bIgnoreDoors)
{
	int32_t			nCurrent, nHead, nTail, nStart, nSide;
	int32_t			l, i;
	int16_t			nChild;
	int16_t			nChildSeg;
	int32_t			nSegment;
	int16_t			childList [SEGMENT_SIDE_COUNT];		//list of ordered sides to process
	int32_t			nChildren, bCullIfBehind;					//how many sides in childList
	CFixVector		viewDir, pViewos;
#if DBG
	CShortArray&	renderSegList = segments;
	CShortArray&	renderPos = position;
	CShortArray&	renderDepth = nDepth;
	CArray<CRenderPoint>& renderPoints = points;
	CByteArray&		processed = bProcessed;
#else
	int16_t*			renderSegList = segments.Buffer ();
	int16_t*			renderPos = position.Buffer ();
	int16_t*			renderDepth = nDepth.Buffer ();
	CRenderPoint*	renderPoints = points.Buffer ();
	uint8_t*			processed = bProcessed.Buffer ();
#endif

viewDir = transformation.m_info.view [0].m.dir.f;
pViewos = transformation.m_info.pos;
gameData.render.zMin = 0x7fffffff;
gameData.render.zMax = -0x7fffffff;
bCullIfBehind = !SHOW_SHADOWS || (gameStates.render.nShadowPass == 1);
position.Clear (char (0xff));
BumpVisitedFlag ();
BumpProcessedFlag ();
BumpVisibleFlag ();

if (BuildAutomapSegList ())
	return;

renderSegList [0] = nStartSeg;
renderDepth [0] = 0;
Visit (nStartSeg);
renderPos [nStartSeg] = 0;
nHead = nTail = nStart = 0;
nCurrent = 1;

portals [0].left =
portals [0].top = 0;
portals [0].right = CCanvas::Current ()->Width () - 1;
portals [0].bot = CCanvas::Current ()->Height () - 1;

#ifdef _OPENMP
#	pragma omp parallel for
#endif
for (i = 0; i < gameData.segData.nVertices; i++) 
	renderPoints [i].Reset ();

#if DBG
if (nStartSeg == nDbgSeg)
	BRP;
int32_t nIterations = 0;
#endif
int32_t nRenderDepth = Min (4 * (int32_t (gameStates.render.detail.nRenderDepth) + 1), gameData.segData.nSegments);

// Starting at the viewer's segment, the following code looks through each open side of a segment
// the projected rectangular area of that side is that side's "portal".
// For each portal, the code looks through the open sides of the portal face's child segment 
// (the child portals). The current portal is then modified with all of the child portals visible through
// it. This goes on until the current portal has dimensions of zero (because only solid geometry is visible through it).
// The end result is a list of all segments that have faces the viewer can see (fully or partially).

for (l = 0; l < nRenderDepth; l++) {
	for (nHead = nStart, nTail = nCurrent; nHead < nTail; nHead++) {
		if (processed [nHead] == nProcessed)
			continue;
		processed [nHead] = nProcessed;
		nSegment = renderSegList [nHead];
		tPortal& curPortal = portals [nHead];
		if (nSegment == -1)
			continue;
#if DBG
		if (nSegment == -32767)
			continue;
		if (nSegment == nDbgSeg)
			BRP;
#endif
#if DBG
		nIterations++;
#endif
		CSegment* pSeg = SEGMENT (nSegment);
		int32_t bRotated = 0;
		//look at all sides of this segment.
		//tricky code to look at sides in correct order follows
		int32_t nSides = SEGMENT_SIDE_COUNT;
		for (nChild = nChildren = 0; nChild < nSides; nChild++) {		//build list of sides
			nChildSeg = pSeg->m_children [nChild];
			if (nChildSeg < 0)
				continue;
			if (!(pSeg->IsPassable (nChild, NULL, bIgnoreDoors) & WID_TRANSPARENT_FLAG))
				continue;
#if DBG
			if (nChildSeg == nDbgSeg)
				nChildSeg = nChildSeg;
#endif
			if (!bRotated) {
				uint16_t* sv = pSeg->m_vertices;
				int32_t j = pSeg->m_nVertices;
				for (int32_t i = 0; i < j; i++) {
#if DBG
					if (sv [i] == nDbgVertex)
						BRP;
#endif
					if (sv [i] != 0xFFFF)
						renderPoints [sv [i]].ProjectAndEncode (transformation, sv [i]);
					}
				bRotated = 1;
				}

			if (bCullIfBehind) {
				uint16_t* s2v = pSeg->Side (nChild)->m_corners;
				int32_t i, j = pSeg->Side (nChild)->m_nCorners;
				for (i = 0; i < j; i++) {
					if (s2v [i] == 0xFFFF)
						continue;
					if (!(renderPoints [s2v [i]].Codes () & CC_BEHIND))
						break;
					}
				if (i == j)
					continue; // all face vertices behind the viewer => face invisible to the viewer
				}
			childList [nChildren++] = nChild;
			}
		// looking from segment nSegment through side nSide at segment nChildSeg
		for (nChild = 0; nChild < nChildren; nChild++) {
			nSide = childList [nChild];
			nChildSeg = pSeg->m_children [nSide];
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				BRP;
			if ((nChildSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nChildSeg = nChildSeg;
#endif
			tPortal facePortal = {32767, -32767, 32767, -32767};
			int32_t bProjected = 1;	//0 when at least one point wasn't projected
			CSide* pSide = pSeg->Side (nSide);
			uint16_t* s2v = pSide->m_corners;
			uint8_t offScreenFlags = 0xff;
			int32_t nCorners = pSide->m_nCorners;
			for (int32_t nCorner = 0; nCorner < nCorners; nCorner++) {
				uint16_t nVertex = s2v [nCorner];
				CRenderPoint& point = renderPoints [nVertex];
#if DBG
				point.Reset ();
#else
				if (!(point.Projected ()))
#endif
					point.ProjectAndEncode (transformation, nVertex);
				if (point.Behind ()) {
					bProjected = 0;
#if 0
					break;
#endif
					}
				//offScreenFlags &= (point.m_codes & ~CC_BEHIND);
				offScreenFlags &= point.Codes ();
				if (facePortal.left > point.X ())
					facePortal.left = point.X ();
				if (facePortal.right < point.X ())
					facePortal.right = point.X ();
				if (facePortal.top > point.Y ())
					facePortal.top = point.Y ();
				if (facePortal.bot < point.Y ())
					facePortal.bot = point.Y ();
				}
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				BRP;
			if ((nChildSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nChildSeg = nChildSeg;
#endif
#if 1
			if ((facePortal.right < 0) || (facePortal.bot < 0) || (facePortal.left >= gameData.render.screen.Width ()) || (facePortal.top >= gameData.render.screen.Height ()))
				continue;
			if (facePortal.left < 0)
				facePortal.left = 0;
			if (facePortal.right >= gameData.render.screen.Width ())
				facePortal.right = gameData.render.screen.Width () - 1;
			if (facePortal.top < 0)
				facePortal.top = 0;
			if (facePortal.bot >= gameData.render.screen.Height ())
				facePortal.bot = gameData.render.screen.Height () - 1;
			if (offScreenFlags)
				continue;
#endif
			if (bProjected) {
				 if (CodePortal (facePortal, curPortal))
					 continue;
				}
#if 1
			else if (gameStates.render.nShadowMap < 0) {
				if (!transformation.Frustum ().Contains (pSide)) {
#	if DBG
					if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
						transformation.Frustum ().Contains (pSide);
					else {
						transformation.Frustum ().Contains (pSide);
#	endif
						continue;
#	if DBG
					}
#	endif
				}
			}
#endif
			//maybe add this segment
			int32_t nPos = renderPos [nChildSeg];
			tPortal& newPortal = portals [nCurrent];

			if (!bProjected)
				newPortal = curPortal;
			else {
				newPortal.left = Max (curPortal.left, facePortal.left);
				newPortal.right = Min (curPortal.right, facePortal.right);
				newPortal.top = Max (curPortal.top, facePortal.top);
				newPortal.bot = Min (curPortal.bot, facePortal.bot);
				}
			//see if this segment has already been visited, and if so, does the current portal expand the old portal?
			if (nPos == -1) {
				renderPos [nChildSeg] = nCurrent;
				renderSegList [nCurrent] = nChildSeg;
				renderDepth [nCurrent++] = l;
				Visit (nChildSeg);
				}
			else {
				tPortal& oldPortal = portals [nPos];
				bool bExpand = false;
				int32_t h;
				h = newPortal.left - oldPortal.left;
				if (h > 0)
					newPortal.left = oldPortal.left;
				else if (h < 0)
					bExpand = true;
				h = newPortal.right - oldPortal.right;
				if (h < 0)
					newPortal.right = oldPortal.right;
				else if (h > 0)
					bExpand = true;
				h = newPortal.top - oldPortal.top;
				if (h > 0)
					newPortal.top = oldPortal.top;
				else if (h < 0)
					bExpand = true;
				h = newPortal.bot - oldPortal.bot;
				if (h < 0)
					newPortal.bot = oldPortal.bot;
				else if (h > 0)
					bExpand = true;
				if (bExpand) {
					if (nCurrent < gameData.segData.nSegments)
						renderSegList [nCurrent] = -0x7fff;
					oldPortal = newPortal;		//get updated tPortal
					processed [nPos] = nProcessed - 1;		//force reprocess
#if 0
					if (!nStart || (nStart > nPos))
						nStart = nPos;
#endif
					}
				}
			}
		}
	}

nSegments = nCurrent;

for (i = 0; i < nSegments; i++) {
#if DBG
	if (renderSegList [i] == nDbgSeg)
		BRP;
#endif
	if (renderSegList [i] >= 0)
		bVisible [renderSegList [i]] = nVisible;
	}

if (gameStates.render.nShadowMap) {
	for (i = 0; i < nSegments; i++) {
		if (renderSegList [i] >= 0) {
			CSegment* pSeg = SEGMENT (renderSegList [i]);
			if (CFixVector::Dist (pViewos, pSeg->Center ()) > I2X (400) + pSeg->MaxRad ()) {
				renderSegList [i] = -renderSegList [i] - 1;
				bVisible [renderSegList [i]] = nVisible - 1;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void BuildRenderSegList (int16_t nStartSeg, int32_t nWindow, bool bIgnoreDoors, int32_t nThread)
{
return gameData.render.mine.visibility [nThread].BuildSegList (transformation, nStartSeg, nWindow, bIgnoreDoors);
}

//------------------------------------------------------------------------------
// eof
