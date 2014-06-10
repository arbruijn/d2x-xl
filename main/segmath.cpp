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

sbyte ConvertToByte (fix f)
{
if (f >= 0x00010000)
	return MATRIX_MAX;
else if (f <= -0x00010000)
	return -MATRIX_MAX;
else
	return (sbyte) (f >> MATRIX_PRECISION);
}

#define VEL_PRECISION 12

// -------------------------------------------------------------------------------
//	Create a tShortPos struct from an CObject.
//	Extract the matrix into byte values.
//	Create a position relative to vertex 0 with 1/256 Normal "fix" precision.
//	Stuff CSegment in a short.
void CreateShortPos (tShortPos *spp, CObject *objP, int swap_bytes)
{
	// int	nSegment;
	CFixMatrix orient = objP->info.position.mOrient;
	sbyte   *segP = spp->orient;
	CFixVector *pv;

*segP++ = ConvertToByte(orient.m.dir.r.v.coord.x);
*segP++ = ConvertToByte(orient.m.dir.u.v.coord.x);
*segP++ = ConvertToByte(orient.m.dir.f.v.coord.x);
*segP++ = ConvertToByte(orient.m.dir.r.v.coord.y);
*segP++ = ConvertToByte(orient.m.dir.u.v.coord.y);
*segP++ = ConvertToByte(orient.m.dir.f.v.coord.y);
*segP++ = ConvertToByte(orient.m.dir.r.v.coord.z);
*segP++ = ConvertToByte(orient.m.dir.u.v.coord.z);
*segP++ = ConvertToByte(orient.m.dir.f.v.coord.z);

pv = gameData.segs.vertices + SEGMENTS [objP->info.nSegment].m_vertices [0];
spp->pos [0] = (short) ((objP->info.position.vPos.v.coord.x - pv->v.coord.x) >> RELPOS_PRECISION);
spp->pos [1] = (short) ((objP->info.position.vPos.v.coord.y - pv->v.coord.y) >> RELPOS_PRECISION);
spp->pos [2] = (short) ((objP->info.position.vPos.v.coord.z - pv->v.coord.z) >> RELPOS_PRECISION);

spp->nSegment = objP->info.nSegment;

spp->vel [0] = (short) ((objP->mType.physInfo.velocity.v.coord.x) >> VEL_PRECISION);
spp->vel [1] = (short) ((objP->mType.physInfo.velocity.v.coord.y) >> VEL_PRECISION);
spp->vel [2] = (short) ((objP->mType.physInfo.velocity.v.coord.z) >> VEL_PRECISION);

// swap the short values for the big-endian machines.

if (swap_bytes) {
	spp->pos [0] = INTEL_SHORT (spp->pos [0]);
	spp->pos [1] = INTEL_SHORT (spp->pos [1]);
	spp->pos [2] = INTEL_SHORT (spp->pos [2]);
	spp->nSegment = INTEL_SHORT (spp->nSegment);
	spp->vel [0] = INTEL_SHORT (spp->vel [0]);
	spp->vel [1] = INTEL_SHORT (spp->vel [1]);
	spp->vel [2] = INTEL_SHORT (spp->vel [2]);
	}
}

// -------------------------------------------------------------------------------

void ExtractShortPos (CObject *objP, tShortPos *spp, int swap_bytes)
{
	int	nSegment;
	sbyte   *segP;
	CFixVector *pv;

	segP = spp->orient;

	objP->info.position.mOrient.m.dir.r.v.coord.x = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.m.dir.u.v.coord.x = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.m.dir.f.v.coord.x = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.m.dir.r.v.coord.y = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.m.dir.u.v.coord.y = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.m.dir.f.v.coord.y = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.m.dir.r.v.coord.z = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.m.dir.u.v.coord.z = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.m.dir.f.v.coord.z = *segP++ << MATRIX_PRECISION;

	if (swap_bytes) {
		spp->pos [0] = INTEL_SHORT (spp->pos [0]);
		spp->pos [1] = INTEL_SHORT (spp->pos [1]);
		spp->pos [2] = INTEL_SHORT (spp->pos [2]);
		spp->nSegment = INTEL_SHORT (spp->nSegment);
		spp->vel [0] = INTEL_SHORT (spp->vel [0]);
		spp->vel [1] = INTEL_SHORT (spp->vel [1]);
		spp->vel [2] = INTEL_SHORT (spp->vel [2]);
	}

	nSegment = spp->nSegment;

	Assert ((nSegment >= 0) && (nSegment <= gameData.segs.nLastSegment));

	pv = gameData.segs.vertices + SEGMENTS [nSegment].m_vertices [0];
	objP->info.position.vPos.v.coord.x = (spp->pos [0] << RELPOS_PRECISION) + pv->v.coord.x;
	objP->info.position.vPos.v.coord.y = (spp->pos [1] << RELPOS_PRECISION) + pv->v.coord.y;
	objP->info.position.vPos.v.coord.z = (spp->pos [2] << RELPOS_PRECISION) + pv->v.coord.z;

	objP->mType.physInfo.velocity.v.coord.x = (spp->vel [0] << VEL_PRECISION);
	objP->mType.physInfo.velocity.v.coord.y = (spp->vel [1] << VEL_PRECISION);
	objP->mType.physInfo.velocity.v.coord.z = (spp->vel [2] << VEL_PRECISION);

	objP->RelinkToSeg (nSegment);

}

// -----------------------------------------------------------------------------
//	Extract a vector from a CSegment.  The vector goes from the start face to the end face.
//	The point on each face is the average of the four points forming the face.
void ExtractVectorFromSegment (CSegment *segP, CFixVector *vp, int start, int end)
{
	CFixVector	vs, ve;

vs.SetZero ();
ve.SetZero ();
int nVertices = 0;
CSide* sideP = segP->Side (start);
for (int i = 0, j = sideP->CornerCount (); i < j; i++) {
	ushort n = sideP->m_corners [i];
	if (n != 0xFFFF) {
		vs += gameData.segs.vertices [n];
		nVertices++;
		}
	}
sideP = segP->Side (end);
for (int i = 0, j = sideP->CornerCount (); i < j; i++) {
	ushort n = sideP->m_corners [i];
	n = segP->m_vertices [sideVertIndex [end][i]];
	if (n != 0xFFFF) {
		ve += gameData.segs.vertices [n];
		nVertices++;
		}
	}
*vp = ve - vs;
*vp *= nVertices * I2X (2) / nVertices;
}

// -------------------------------------------------------------------------------
//create a matrix that describes the orientation of the given CSegment
void ExtractOrientFromSegment (CFixMatrix *m, CSegment *segP)
{
	CFixVector fVec, uVec;

	ExtractVectorFromSegment (segP, &fVec, WFRONT, WBACK);
	ExtractVectorFromSegment (segP, &uVec, WBOTTOM, WTOP);

	//vector to matrix does normalizations and orthogonalizations
	*m = CFixMatrix::CreateFU(fVec, uVec);
//	*m = CFixMatrix::CreateFU(fVec, &uVec, NULL);
}

// -------------------------------------------------------------------------------
//	Return v0, v1, v2 = 3 vertices with smallest numbers.  If *bFlip set, then negate Normal after computation.
//	Note, pos.v.c.yu cannot just compute the Normal by treating the points in the opposite direction as this introduces
//	small differences between normals which should merely be opposites of each other.
ushort SortVertsForNormal (ushort v0, ushort v1, ushort v2, ushort v3, ushort* vSorted)
{
	int		i, j;
	ushort	index [4] = {0, 1, 2, 3};

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

void AddToVertexNormal (int nVertex, CFixVector& vNormal)
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

for (int i = gameData.segs.nVertices; i; i--, pp++)
	pp->Normal ().Normalize ();
}

// -------------------------------------------------------------------------------

void ResetVertexNormals (void)
{
	CRenderPoint* pp = RENDERPOINTS.Buffer ();

for (int i = gameData.segs.nVertices; i; i--, pp++)
	pp->Normal ().Reset ();
}

//	-----------------------------------------------------------------------------
//eof
