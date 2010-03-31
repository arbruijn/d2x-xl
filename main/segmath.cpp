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

*segP++ = ConvertToByte(orient.RVec ()[X]);
*segP++ = ConvertToByte(orient.UVec ()[X]);
*segP++ = ConvertToByte(orient.FVec ()[X]);
*segP++ = ConvertToByte(orient.RVec ()[Y]);
*segP++ = ConvertToByte(orient.UVec ()[Y]);
*segP++ = ConvertToByte(orient.FVec ()[Y]);
*segP++ = ConvertToByte(orient.RVec ()[Z]);
*segP++ = ConvertToByte(orient.UVec ()[Z]);
*segP++ = ConvertToByte(orient.FVec ()[Z]);

pv = gameData.segs.vertices + SEGMENTS [objP->info.nSegment].m_verts [0];
spp->pos [X] = (short) ((objP->info.position.vPos [X] - (*pv)[X]) >> RELPOS_PRECISION);
spp->pos [Y] = (short) ((objP->info.position.vPos [Y] - (*pv)[Y]) >> RELPOS_PRECISION);
spp->pos [Z] = (short) ((objP->info.position.vPos [Z] - (*pv)[Z]) >> RELPOS_PRECISION);

spp->nSegment = objP->info.nSegment;

spp->vel [X] = (short) ((objP->mType.physInfo.velocity[X]) >> VEL_PRECISION);
spp->vel [Y] = (short) ((objP->mType.physInfo.velocity[Y]) >> VEL_PRECISION);
spp->vel [Z] = (short) ((objP->mType.physInfo.velocity[Z]) >> VEL_PRECISION);

// swap the short values for the big-endian machines.

if (swap_bytes) {
	spp->pos [X] = INTEL_SHORT (spp->pos [X]);
	spp->pos [Y] = INTEL_SHORT (spp->pos [Y]);
	spp->pos [Z] = INTEL_SHORT (spp->pos [Z]);
	spp->nSegment = INTEL_SHORT (spp->nSegment);
	spp->vel [X] = INTEL_SHORT (spp->vel [X]);
	spp->vel [Y] = INTEL_SHORT (spp->vel [Y]);
	spp->vel [Z] = INTEL_SHORT (spp->vel [Z]);
	}
}

// -------------------------------------------------------------------------------

void ExtractShortPos (CObject *objP, tShortPos *spp, int swap_bytes)
{
	int	nSegment;
	sbyte   *segP;
	CFixVector *pv;

	segP = spp->orient;

	objP->info.position.mOrient.RVec ()[X] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.UVec ()[X] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.FVec ()[X] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.RVec ()[Y] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.UVec ()[Y] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.FVec ()[Y] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.RVec ()[Z] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.UVec ()[Z] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.FVec ()[Z] = *segP++ << MATRIX_PRECISION;

	if (swap_bytes) {
		spp->pos [X] = INTEL_SHORT (spp->pos [X]);
		spp->pos [Y] = INTEL_SHORT (spp->pos [Y]);
		spp->pos [Z] = INTEL_SHORT (spp->pos [Z]);
		spp->nSegment = INTEL_SHORT (spp->nSegment);
		spp->vel [X] = INTEL_SHORT (spp->vel [X]);
		spp->vel [Y] = INTEL_SHORT (spp->vel [Y]);
		spp->vel [Z] = INTEL_SHORT (spp->vel [Z]);
	}

	nSegment = spp->nSegment;

	Assert ((nSegment >= 0) && (nSegment <= gameData.segs.nLastSegment));

	pv = gameData.segs.vertices + SEGMENTS [nSegment].m_verts [0];
	objP->info.position.vPos [X] = (spp->pos [X] << RELPOS_PRECISION) + (*pv)[X];
	objP->info.position.vPos [Y] = (spp->pos [Y] << RELPOS_PRECISION) + (*pv)[Y];
	objP->info.position.vPos [Z] = (spp->pos [Z] << RELPOS_PRECISION) + (*pv)[Z];

	objP->mType.physInfo.velocity[X] = (spp->vel [X] << VEL_PRECISION);
	objP->mType.physInfo.velocity[Y] = (spp->vel [Y] << VEL_PRECISION);
	objP->mType.physInfo.velocity[Z] = (spp->vel [Z] << VEL_PRECISION);

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
for (int i = 0; i < 4; i++) {
	vs += gameData.segs.vertices [segP->m_verts [sideVertIndex [start][i]]];
	ve += gameData.segs.vertices [segP->m_verts [sideVertIndex [end][i]]];
	}
*vp = ve - vs;
*vp *= (I2X (1) / 4);
}

// -------------------------------------------------------------------------------
//create a matrix that describes the orientation of the given CSegment
void ExtractOrientFromSegment (CFixMatrix *m, CSegment *seg)
{
	CFixVector fVec, uVec;

	ExtractVectorFromSegment (seg, &fVec, WFRONT, WBACK);
	ExtractVectorFromSegment (seg, &uVec, WBOTTOM, WTOP);

	//vector to matrix does normalizations and orthogonalizations
	*m = CFixMatrix::CreateFU(fVec, uVec);
//	*m = CFixMatrix::CreateFU(fVec, &uVec, NULL);
}

// -------------------------------------------------------------------------------
//	Return v0, v1, v2 = 3 vertices with smallest numbers.  If *bFlip set, then negate Normal after computation.
//	Note, pos [Y]u cannot just compute the Normal by treating the points in the opposite direction as this introduces
//	small differences between normals which should merely be opposites of each other.
short GetVertsForNormal (short v0, short v1, short v2, short v3, short* vSorted)
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

Assert ((vSorted [0] < vSorted [1]) && (vSorted [1] < vSorted [2]) && (vSorted [2] < vSorted [3]));
//	Now, if for any index [i] & index [i+1]: index [i+1] = (index [i]+3)%4, then must flip Normal
return (((index [0] + 3) % 4) == index [1]) || (((index [1] + 3) % 4) == index [2]);
}

// -------------------------------------------------------------------------------

void AddToVertexNormal (int nVertex, CFixVector& vNormal)
{
	g3sNormal	*pn = &gameData.segs.points [nVertex].p3_normal;

#if DBG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
pn->nFaces++;
pn->vNormal += vNormal;
}

// -------------------------------------------------------------------------------

void ComputeVertexNormals (void)
{
	int		h, i;
	g3sPoint	*pp;

for (i = gameData.segs.nVertices, pp = gameData.segs.points.Buffer (); i; i--, pp++) {
	if (1 < (h = pp->p3_normal.nFaces)) {
		pp->p3_normal.vNormal /= (float) h;
		/*
		pp->p3_normal.vNormal[Y] /= h;
		pp->p3_normal.vNormal[Z] /= h;
		*/
		CFloatVector::Normalize (pp->p3_normal.vNormal);
		}
	pp->p3_normal.nFaces = 1;
	}
}

// -------------------------------------------------------------------------------

void ResetVertexNormals (void)
{
	int		i;
	g3sPoint	*pp;

for (i = gameData.segs.nVertices, pp = gameData.segs.points.Buffer (); i; i--, pp++) {
	pp->p3_normal.vNormal.SetZero ();
	pp->p3_normal.nFaces = 0;
	}
}

//	-----------------------------------------------------------------------------
//eof
