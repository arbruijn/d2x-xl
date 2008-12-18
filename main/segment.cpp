#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"
#include "u_mem.h"
#include "error.h"

// Number of vertices in current mine (ie, gameData.segs.vertices, pointed to by Vp)
//	Translate table to get opposite CSide of a face on a CSegment.
char	sideOpposite[MAX_SIDES_PER_SEGMENT] = {WRIGHT, WBOTTOM, WLEFT, WTOP, WFRONT, WBACK};

//	Note, this MUST be the same as sideVertIndex, it is an int for speed reasons.
int sideVertIndex [MAX_SIDES_PER_SEGMENT][4] = {
			{7,6,2,3},			// left
			{0,4,7,3},			// top
			{0,1,5,4},			// right
			{2,6,5,1},			// bottom
			{4,5,6,7},			// back
			{3,2,1,0}			// front
	};	

//------------------------------------------------------------------------------
// reads a tSegment2 structure from a CFile
 
void CSegment::Read (CFile& cf, bool bExtended)
{
if (bExtended) {
	m_nType = cf.ReadByte ();
	m_nMatCen = cf.ReadByte ();
	m_value = cf.ReadByte ();
	m_flags = cf.ReadByte ();
	m_xAvgSegLight = cf.ReadFix ();
	}
else {
	}
}

//------------------------------------------------------------------------------

void CSegment::ComputeSideRads (void)
{
for (int i = 0; i < 6; i++)
	m_sides [i].ComputeRads ();
}

//------------------------------------------------------------------------------

void CSegment::ComputeCenter (void)
{
m_vCenter = gameData.segs.vertices [m_verts [0]];
m_vCenter += gameData.segs.vertices [m_verts [1]];
m_vCenter += gameData.segs.vertices [m_verts [2]];
m_vCenter += gameData.segs.vertices [m_verts [3]];
m_vCenter += gameData.segs.vertices [m_verts [4]];
m_vCenter += gameData.segs.vertices [m_verts [5]];
m_vCenter += gameData.segs.vertices [m_verts [6]];
m_vCenter += gameData.segs.vertices [m_verts [7]];
m_vCenter [X] /= 8;
m_vCenter [Y] /= 8;
m_vCenter [Z] /= 8;
}

// -----------------------------------------------------------------------------

void CSegment::ComputeRads (fix xMinDist)
{
	CFixVector	vMin, vMax, v;
	fix			xDist;

m_rads [0] = xMinDist;
m_rads [1] = 0;
vMin [X] = vMin [Y] = vMin [Z] = 0x7FFFFFFF;
vMax [X] = vMax [Y] = vMax [Z] = -0x7FFFFFFF;
for (int i = 0; i < 8; i++) {
	v = m_vCenter - gameData.segs.vertices [m_verts [i]];
	if (vMin [X] > v [X])
		vMin [X] = v [X];
	if (vMin [Y] > v [Y])
		vMin [Y] = v [Y];
	if (vMin [Z] > v [Z])
		vMin [Z] = v [Z];
	if (vMax [X] < v [X])
		vMax [X] = v [X];
	if (vMax [Y] < v [Y])
		vMax [Y] = v [Y];
	if (vMax [Z] < v [Z])
		vMax [Z] = v [Z];
	xDist = v.Mag ();
	if (m_rads [1] < xDist)
		m_rads [1] = xDist;
	}
m_extents [0] = vMin;
m_extents [1] = vMax;
}

// -----------------------------------------------------------------------------
//	Given two segments, return the side index in the connecting segment which connects to the base segment

int CSegment::FindConnectedSide (CSegment* other)
{
	short		nSegment = SEG_IDX (this);
	ushort*	childP = other->m_children;

if (childP [0] == nSegment)
		return 0;
if (childP [1] == nSegment)
		return 1;
if (childP [2] == nSegment)
		return 2;
if (childP [3] == nSegment)
		return 3;
if (childP [4] == nSegment)
		return 4;
if (childP [5] == nSegment)
		return 5;
return -1;
}

// -------------------------------------------------------------------------------
//returns 3 different bitmasks with info telling if this sphere is in
//this CSegment.  See CSegMasks structure for info on fields
CSegMasks CSegment::GetSideMasks (const CFixVector& refP, fix xRad)
{
	short			nSide, faceBit;
	CSegMasks	masks;

//check refPoint against each CSide of CSegment. return bitmask
masks.m_valid = 1;
for (nSide = 0, faceBit = 1; nSide < 6; nSide++)
	masks |= m_sides [nSide].Masks (refP, xRad, 1 << nSide, faceBit);
return masks;
}

// -------------------------------------------------------------------------------
//	Make a just-modified CSegment valid.
//		check all sides to see how many faces they each should have (0, 1, 2)
//		create new vector normals
void CSegment::Validate (void)
{
for (int i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
	m_sides [i].Validate (m_children [i] < 0);
}

//	--------------------------------------------------------------------------------
//	Picks a Random point in a CSegment like so:
//		From center, go up to 50% of way towards any of the 8 vertices.
CFixVector CSegment::RandomPoint (void)
{
int nVertex = (d_rand () * MAX_VERTICES_PER_SEGMENT) >> 15;
CFixVector v = gameData.segs.vertices [m_verts [nVertex]] - m_vCenter;
v *= (d_rand ());        
return v;
}

//------------------------------------------------------------------------------
//eof
