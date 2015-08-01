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
#include <string.h>	//	for memset ()

#include "u_mem.h"
#include "descent.h"
#include "error.h"
#include "mono.h"
#include "segmath.h"
#include "byteswap.h"
#include "light.h"
#include "segment.h"

// -------------------------------------------------------------------------------

int8_t ConvertToByte (fix f)
{
if (f >= 0x00010000)
	return MATRIX_MAX;
else if (f <= -0x00010000)
	return -MATRIX_MAX;
else
	return (int8_t) (f >> MATRIX_PRECISION);
}

#define VEL_PRECISION 12

// -------------------------------------------------------------------------------

void CreateLongPos (tLongPos* pPos, CObject* pObj)
{
if (!pObj) 
	memset (pPos, 0, sizeof (*pPos));
else {
	pPos->nSegment = pObj->Segment ();
	pPos->pos = pObj->Position ();
	pPos->orient = pObj->Orientation ();
	pPos->vel = pObj->Velocity ();
	pPos->rotVel = pObj->RotVelocity ();
	}
}

// -------------------------------------------------------------------------------

void ExtractLongPos (CObject* pObj, tLongPos* pPos)
{
if (pObj) {
	pObj->SetSegment (pPos->nSegment);
	pObj->Position () = pPos->pos;
	pObj->Orientation () = pPos->orient;
	pObj->Velocity () = pPos->vel;
	pObj->RotVelocity () = pPos->rotVel;
	}
}

// -------------------------------------------------------------------------------
//	Create a tShortPos struct from an CObject.
//	Extract the matrix into byte values.
//	Create a position relative to vertex 0 with 1/256 Normal "fix" precision.
//	Stuff CSegment in a int16_t.
void CreateShortPos (tShortPos *pPos, CObject *pObj, int32_t bSwapBytes)
{
if (!pObj) 
	memset (pPos, 0, sizeof (*pPos));
else {
	// int32_t	nSegment;
	CFixMatrix	orient = pObj->info.position.mOrient;
	int8_t		*pSeg = pPos->orient;
	CFixVector	*pv;

	*pSeg++ = ConvertToByte (orient.m.dir.r.v.coord.x);
	*pSeg++ = ConvertToByte (orient.m.dir.u.v.coord.x);
	*pSeg++ = ConvertToByte (orient.m.dir.f.v.coord.x);
	*pSeg++ = ConvertToByte (orient.m.dir.r.v.coord.y);
	*pSeg++ = ConvertToByte (orient.m.dir.u.v.coord.y);
	*pSeg++ = ConvertToByte (orient.m.dir.f.v.coord.y);
	*pSeg++ = ConvertToByte (orient.m.dir.r.v.coord.z);
	*pSeg++ = ConvertToByte (orient.m.dir.u.v.coord.z);
	*pSeg++ = ConvertToByte (orient.m.dir.f.v.coord.z);

	pv = gameData.segData.vertices + SEGMENT (pObj->info.nSegment)->m_vertices [0];
	pPos->pos [0] = (int16_t) ((pObj->info.position.vPos.v.coord.x - pv->v.coord.x) >> RELPOS_PRECISION);
	pPos->pos [1] = (int16_t) ((pObj->info.position.vPos.v.coord.y - pv->v.coord.y) >> RELPOS_PRECISION);
	pPos->pos [2] = (int16_t) ((pObj->info.position.vPos.v.coord.z - pv->v.coord.z) >> RELPOS_PRECISION);

	pPos->nSegment = pObj->info.nSegment;

	pPos->vel [0] = (int16_t) ((pObj->mType.physInfo.velocity.v.coord.x) >> VEL_PRECISION);
	pPos->vel [1] = (int16_t) ((pObj->mType.physInfo.velocity.v.coord.y) >> VEL_PRECISION);
	pPos->vel [2] = (int16_t) ((pObj->mType.physInfo.velocity.v.coord.z) >> VEL_PRECISION);

	// swap the int16_t values for the big-endian machines.

	if (bSwapBytes) {
		pPos->pos [0] = INTEL_SHORT (pPos->pos [0]);
		pPos->pos [1] = INTEL_SHORT (pPos->pos [1]);
		pPos->pos [2] = INTEL_SHORT (pPos->pos [2]);
		pPos->nSegment = INTEL_SHORT (pPos->nSegment);
		pPos->vel [0] = INTEL_SHORT (pPos->vel [0]);
		pPos->vel [1] = INTEL_SHORT (pPos->vel [1]);
		pPos->vel [2] = INTEL_SHORT (pPos->vel [2]);
		}
	}
}

// -------------------------------------------------------------------------------

void ExtractShortPos (CObject *pObj, tShortPos *spp, int32_t bSwapBytes)
{
if (pObj) {
	int32_t		nSegment;
	int8_t		*pSeg;
	CFixVector *pv;

	pSeg = spp->orient;

	pObj->info.position.mOrient.m.dir.r.v.coord.x = *pSeg++ << MATRIX_PRECISION;
	pObj->info.position.mOrient.m.dir.u.v.coord.x = *pSeg++ << MATRIX_PRECISION;
	pObj->info.position.mOrient.m.dir.f.v.coord.x = *pSeg++ << MATRIX_PRECISION;
	pObj->info.position.mOrient.m.dir.r.v.coord.y = *pSeg++ << MATRIX_PRECISION;
	pObj->info.position.mOrient.m.dir.u.v.coord.y = *pSeg++ << MATRIX_PRECISION;
	pObj->info.position.mOrient.m.dir.f.v.coord.y = *pSeg++ << MATRIX_PRECISION;
	pObj->info.position.mOrient.m.dir.r.v.coord.z = *pSeg++ << MATRIX_PRECISION;
	pObj->info.position.mOrient.m.dir.u.v.coord.z = *pSeg++ << MATRIX_PRECISION;
	pObj->info.position.mOrient.m.dir.f.v.coord.z = *pSeg++ << MATRIX_PRECISION;

	if (bSwapBytes) {
		spp->pos [0] = INTEL_SHORT (spp->pos [0]);
		spp->pos [1] = INTEL_SHORT (spp->pos [1]);
		spp->pos [2] = INTEL_SHORT (spp->pos [2]);
		spp->nSegment = INTEL_SHORT (spp->nSegment);
		spp->vel [0] = INTEL_SHORT (spp->vel [0]);
		spp->vel [1] = INTEL_SHORT (spp->vel [1]);
		spp->vel [2] = INTEL_SHORT (spp->vel [2]);
	}

	nSegment = spp->nSegment;

	Assert ((nSegment >= 0) && (nSegment <= gameData.segData.nLastSegment));

	pv = gameData.segData.vertices + SEGMENT (nSegment)->m_vertices [0];
	pObj->info.position.vPos.v.coord.x = (spp->pos [0] << RELPOS_PRECISION) + pv->v.coord.x;
	pObj->info.position.vPos.v.coord.y = (spp->pos [1] << RELPOS_PRECISION) + pv->v.coord.y;
	pObj->info.position.vPos.v.coord.z = (spp->pos [2] << RELPOS_PRECISION) + pv->v.coord.z;

	pObj->mType.physInfo.velocity.v.coord.x = (spp->vel [0] << VEL_PRECISION);
	pObj->mType.physInfo.velocity.v.coord.y = (spp->vel [1] << VEL_PRECISION);
	pObj->mType.physInfo.velocity.v.coord.z = (spp->vel [2] << VEL_PRECISION);

	pObj->RelinkToSeg (nSegment);
	}
}

// -----------------------------------------------------------------------------
//	Extract a vector from a CSegment.  The vector goes from the start face to the end face.
//	The point on each face is the average of the four points forming the face.
void ExtractVectorFromSegment (CSegment *pSeg, CFixVector *vp, int32_t start, int32_t end)
{
	CFixVector	vs, ve;

vs.SetZero ();
ve.SetZero ();
int32_t nVertices = 0;
CSide* pSide = pSeg->Side (start);
for (int32_t i = 0, j = pSide->CornerCount (); i < j; i++) {
	uint16_t n = pSide->m_corners [i];
	if (n != 0xFFFF) {
		vs += gameData.segData.vertices [n];
		nVertices++;
		}
	}
pSide = pSeg->Side (end);
for (int32_t i = 0, j = pSide->CornerCount (); i < j; i++) {
	uint16_t n = pSide->m_corners [i];
	n = pSeg->m_vertices [sideVertIndex [end][i]];
	if (n != 0xFFFF) {
		ve += gameData.segData.vertices [n];
		nVertices++;
		}
	}
*vp = ve - vs;
*vp *= nVertices * I2X (2) / nVertices;
}

// -------------------------------------------------------------------------------
//create a matrix that describes the orientation of the given CSegment
void ExtractOrientFromSegment (CFixMatrix *m, CSegment *pSeg)
{
	CFixVector fVec, uVec;

	ExtractVectorFromSegment (pSeg, &fVec, WFRONT, WBACK);
	ExtractVectorFromSegment (pSeg, &uVec, WBOTTOM, WTOP);

	//vector to matrix does normalizations and orthogonalizations
	*m = CFixMatrix::CreateFU(fVec, uVec);
//	*m = CFixMatrix::CreateFU(fVec, &uVec, NULL);
}

// -------------------------------------------------------------------------------
//	Return v0, v1, v2 = 3 vertices with smallest numbers.  If *bFlip set, then negate Normal after computation.
//	Note, pos.v.c.yu cannot just compute the Normal by treating the points in the opposite direction as this introduces
//	small differences between normals which should merely be opposites of each other.
uint16_t SortVertsForNormal (uint16_t v0, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t* vSorted)
{
	int32_t		i, j;
	uint16_t	index [4] = {0, 1, 2, 3};

//	index is a list that shows how things got scrambled so we know if our Normal is pointing backwards
vSorted [0] = v0;
vSorted [1] = v1;
vSorted [2] = v2;
vSorted [3] = v3;
// bubble sort vSorted in reverse order (largest first)
for (i = 1; i < 4; i++)
	for (j = 0; j < i; j++)
		if (vSorted [j] > vSorted [i]) {
			Swap (vSorted [i], vSorted [j]);
			Swap (index [i], index [j]);
			}

//Assert ((vSorted [0] < vSorted [1]) && (vSorted [1] < vSorted [2]) && (vSorted [2] < vSorted [3]));
//	Now, if for any index [i] & index [i+1]: index [i+1] = (index [i]+3)%4, then must flip Normal
return (((index [0] + 3) % 4) == index [1]) || (((index [1] + 3) % 4) == index [2]);
}

// -------------------------------------------------------------------------------

void AddToVertexNormal (int32_t nVertex, CFixVector& vNormal)
{
	CRenderNormal& n = RENDERPOINTS [nVertex].Normal ();

#if DBG
if (nVertex == nDbgVertex)
	BRP;
#endif
n += vNormal;
n++;
}

// -------------------------------------------------------------------------------

void ComputeVertexNormals (void)
{
	CRenderPoint* pp = RENDERPOINTS.Buffer ();

for (int32_t i = gameData.segData.nVertices; i; i--, pp++)
	pp->Normal ().Normalize ();
}

// -------------------------------------------------------------------------------

void ResetVertexNormals (void)
{
	CRenderPoint* pp = RENDERPOINTS.Buffer ();

for (int32_t i = gameData.segData.nVertices; i; i--, pp++)
	pp->Normal ().Reset ();
}

//	-----------------------------------------------------------------------------
//eof
