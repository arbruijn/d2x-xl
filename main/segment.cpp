#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "newdemo.h"
#include "wall.h"
#include "fireball.h"
#include "renderlib.h"
#include "visibility.h"
#include "segmath.h"
#include "text.h"

// Number of vertices in current mine (ie, gameData.segData.vertices, pointed to by Vp)
//	Translate table to get opposite CSide of a face on a CSegment.
char	oppSideTable [SEGMENT_SIDE_COUNT] = {WRIGHT, WBOTTOM, WLEFT, WTOP, WFRONT, WBACK};

//	Note, this MUST be the same as sideVertIndex, it is an int32_t for speed reasons.
uint16_t sideVertIndex [SEGMENT_SIDE_COUNT][4] = {
		 {7,6,2,3},			// left
		 {0,4,7,3},			// top
		 {0,1,5,4},			// right
		 {2,6,5,1},			// bottom
		 {4,5,6,7},			// back
		 {3,2,1,0}			// front
	};

extern bool bNewFileFormat;

// -----------------------------------------------------------------------------------
// Fill in array with four absolute point numbers for a given CSide
void CSegment::GetCornerIndex (int32_t nSide, uint16_t* vertIndex)
{
	uint16_t* s2v = sideVertIndex [nSide];

vertIndex [0] = m_vertices [s2v [0]];
vertIndex [1] = m_vertices [s2v [1]];
vertIndex [2] = m_vertices [s2v [2]];
vertIndex [3] = m_vertices [s2v [3]];
}

//------------------------------------------------------------------------------

int32_t CSegment::Index (void)
{
return this - SEGMENTS;
}

// -------------------------------------------------------------------------------

int32_t CSegment::HasVertex (uint8_t nSide, uint16_t nVertex) 
{ 
if ((gameData.segData.vertexOwners [nVertex].nSegment == Index ()) && 
	 (gameData.segData.vertexOwners [nVertex].nSide == nSide))
	return 1;
return Side (nSide)->HasVertex (nVertex);
}

//------------------------------------------------------------------------------

void CSegment::ReadFunction (CFile& cf, uint8_t flags)
{
if (flags & (1 << SEGMENT_SIDE_COUNT)) {
	m_function = cf.ReadByte ();
	m_nObjProducer = cf.ReadByte ();
	m_value = char (cf.ReadByte ());
	cf.ReadByte (); // skip
	}
else {
	m_function = 0;
	m_nObjProducer = -1;
	m_value = 0;
	}
}

//------------------------------------------------------------------------------

void CSegment::ReadVerts (CFile& cf)
{
m_nShape = 0;
m_nVertices = 8;
for (int32_t i = 0; i < SEGMENT_VERTEX_COUNT; i++)
	if (0xFFF8 <= (m_vertices [i] = cf.ReadShort ()))
		m_nShape++;
m_nVertices = 8 - m_nShape;
}

//------------------------------------------------------------------------------

void CSegment::ReadChildren (CFile& cf, uint8_t flags)
{
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++)
	m_children [i] = (flags & (1 << i)) ? cf.ReadShort () : -1;
}

//------------------------------------------------------------------------------

static uint8_t segFuncFromType [] = {
	SEGMENT_FUNC_NONE,
	SEGMENT_FUNC_FUELCENTER,
	SEGMENT_FUNC_REPAIRCENTER,
	SEGMENT_FUNC_REACTOR,
	SEGMENT_FUNC_ROBOTMAKER,
	SEGMENT_FUNC_GOAL_BLUE,
	SEGMENT_FUNC_GOAL_RED,
	SEGMENT_FUNC_NONE,
	SEGMENT_FUNC_NONE,
	SEGMENT_FUNC_TEAM_BLUE,
	SEGMENT_FUNC_TEAM_RED,
	SEGMENT_FUNC_SPEEDBOOST,
	SEGMENT_FUNC_NONE,
	SEGMENT_FUNC_NONE,
	SEGMENT_FUNC_SKYBOX,
	SEGMENT_FUNC_EQUIPMAKER,
	SEGMENT_FUNC_NONE
	};

static uint8_t segPropsFromType [] = {
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_WATER,
	SEGMENT_PROP_LAVA,
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_BLOCKED,
	SEGMENT_PROP_NODAMAGE,
	SEGMENT_PROP_BLOCKED,
	SEGMENT_PROP_NONE,
	SEGMENT_PROP_OUTDOORS
	};

void CSegment::Upgrade (void)
{
m_props = segPropsFromType [m_function];
m_function = segFuncFromType [m_function];
m_xDamage [0] =
m_xDamage [1] = 0;
}

//------------------------------------------------------------------------------

void CSegment::ReadExtras (CFile& cf)
{
m_function = cf.ReadByte ();
if (gameData.segData.nLevelVersion < 24) {
	m_nObjProducer = cf.ReadByte ();
	m_value = cf.ReadByte ();
	}
else {
	m_nObjProducer = cf.ReadShort ();
	m_value = cf.ReadShort ();
	}
m_flags = cf.ReadByte ();
if (gameData.segData.nLevelVersion <= 20)
	Upgrade ();
else {
	m_props = cf.ReadByte ();
	m_xDamage [0] = I2X (cf.ReadShort ());
	m_xDamage [1] = I2X (cf.ReadShort ());
	}
m_xAvgSegLight = cf.ReadFix ();
if (!gameStates.app.bD2XLevel && (m_function == 2))
	m_function = 0;
}

//------------------------------------------------------------------------------

void CSegment::Read (CFile& cf)
{
#if DBG
if (Index () == nDbgSeg)
	BRP;
#endif
if (gameStates.app.bD2XLevel) {
	m_owner = cf.ReadByte ();
	m_group = cf.ReadByte ();
	}
else {
	m_owner = -1;
	m_group = -1;
	}

uint8_t flags = bNewFileFormat ? cf.ReadByte () : 0x7f;

if (gameData.segData.nLevelVersion == 5) { // d2 SHAREWARE level
	ReadFunction (cf, flags);
	ReadVerts (cf);
	ReadChildren (cf, flags);
	}
else {
	ReadChildren (cf, flags);
	ReadVerts (cf);
	if (gameData.segData.nLevelVersion <= 1) { // descent 1 level
		ReadFunction (cf, flags);
		}
	}
m_objects = -1;

if (gameData.segData.nLevelVersion <= 5) // descent 1 thru d2 SHAREWARE level
	m_xAvgSegLight	= fix (cf.ReadShort ()) << 4;

// Read the walls as a 6 byte array
uint8_t wallFlags = bNewFileFormat ? cf.ReadByte () : 0x3f;

int32_t i;

for (i = 0; i < SEGMENT_SIDE_COUNT; i++)
	m_sides [i].ReadWallNum (cf, (wallFlags & (1 << i)) != 0);

uint16_t sideVerts [4];

for (i = 0; i < SEGMENT_SIDE_COUNT; i++) {
	if (gameData.segData.nLevelVersion < 25)
		GetCornerIndex (i, sideVerts);
	m_sides [i].Read (cf, m_vertices, sideVerts, m_children [i] == -1);
	}
if (gameData.segData.nLevelVersion > 24) 
	RemapVertices ();
}

//------------------------------------------------------------------------------
// reads a CSegment structure from a CFile

void CSegment::SaveState (CFile& cf)
{
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++)
	m_sides [i].SaveState (cf);
}

//------------------------------------------------------------------------------
// reads a CSegment structure from a CFile

void CSegment::LoadState (CFile& cf)
{
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++)
	m_sides [i].LoadState (cf);
}

//------------------------------------------------------------------------------

void CSegment::ComputeSideRads (void)
{
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++)
	m_sides [i].ComputeRads ();
}

//------------------------------------------------------------------------------

void CSegment::ComputeCenter (void)
{
#if DBG
if (Index () == nDbgSeg)
	BRP;
#endif
CFloatVector vCenter;
vCenter.SetZero ();
int32_t nVertices = 0;
for (int32_t i = 0; i < 8; i++) {
	if (m_vertices [i] != 0xFFFF) {
		vCenter += FVERTICES [m_vertices [i]];
		nVertices++;
		}
	}
vCenter /= float (nVertices);
m_vCenter.Assign (vCenter);
}

// -----------------------------------------------------------------------------

void CSegment::ComputeChildDists (void)
{
// unscaled distances from the segment's center to each adjacent segment's center
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
	fix& dist = m_childDists [0][i];
	if (0 > m_children [i])
		dist = -1;
	else {
		dist = CFixVector::Dist (Center (), SEGMENT (m_children [i])->Center ());
		if (dist == 0)
			dist = 1;
		}
	}

// scaled distances from the segment's center to each adjacent segment's center
// scaled with 0xFFFF / max (child distance) of all child distances
// this is needed for the DACS router to make sure no edge is longer than 0xFFFF units
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
	fix& dist = m_childDists [1][i];
	if (0 > m_children [i])
		dist = 0xFFFF;
	else {
		dist = m_childDists [0][i] / gameData.segData.xDistScale;
		if (dist == 0)
			dist = 1;
		}
	}
}

// -----------------------------------------------------------------------------

void CSegment::ComputeRads (fix xMinDist)
{
	CFixVector	vMin, vMax, v;
	fix			xDist;

m_rads [0] = xMinDist;
m_rads [1] = 0;
vMin.v.coord.x = vMin.v.coord.y = vMin.v.coord.z = 0x7FFFFFFF;
vMax.v.coord.x = vMax.v.coord.y = vMax.v.coord.z = -0x7FFFFFFF;
for (int32_t i = 0; i < 8; i++) {
	if (m_vertices [i] != 0xFFFF) {
		v = gameData.segData.vertices [m_vertices [i]];
		if (vMin.v.coord.x > v.v.coord.x)
			vMin.v.coord.x = v.v.coord.x;
		if (vMin.v.coord.y > v.v.coord.y)
			vMin.v.coord.y = v.v.coord.y;
		if (vMin.v.coord.z > v.v.coord.z)
			vMin.v.coord.z = v.v.coord.z;
		if (vMax.v.coord.x < v.v.coord.x)
			vMax.v.coord.x = v.v.coord.x;
		if (vMax.v.coord.y < v.v.coord.y)
			vMax.v.coord.y = v.v.coord.y;
		if (vMax.v.coord.z < v.v.coord.z)
			vMax.v.coord.z = v.v.coord.z;
		xDist = CFixVector::Dist (v, m_vCenter);
		if (m_rads [1] < xDist)
			m_rads [1] = xDist;
		}
	}
ComputeSideRads ();
m_extents [0] = vMin;
m_extents [1] = vMax;
}

// -----------------------------------------------------------------------------
//	Given two segments, return the side index in the connecting segment which connects to the base segment

int32_t CSegment::ConnectedSide (CSegment* other)
{
	int16_t		nSegment = SEG_IDX (this);
	int16_t*	pChild = other->m_children;

if (pChild [0] == nSegment)
		return 0;
if (pChild [1] == nSegment)
		return 1;
if (pChild [2] == nSegment)
		return 2;
if (pChild [3] == nSegment)
		return 3;
if (pChild [4] == nSegment)
		return 4;
if (pChild [5] == nSegment)
		return 5;
return -1;
}

// -------------------------------------------------------------------------------
//returns 3 different bitmasks with info telling if this sphere is in
//this CSegment.  See CSegMasks structure for info on fields
CSegMasks CSegment::Masks (const CFixVector& pRef, fix xRad)
{
ENTER (2, 0);
	int16_t		nSide, faceBit;
	CSegMasks	masks;

//check refPoint against each CSide of CSegment. return bitmask
masks.m_valid = 1;
for (nSide = 0, faceBit = 1; nSide < SEGMENT_SIDE_COUNT; nSide++, faceBit <<= 2)
	if (m_sides [nSide].FaceCount ())
		masks |= m_sides [nSide].Masks (pRef, xRad, 1 << nSide, faceBit);
RETVAL (masks)
}

// -------------------------------------------------------------------------------
//returns 3 different bitmasks with info telling if this sphere is in
//this CSegment.  See CSegMasks structure for info on fields
CSegMasks CSegment::SideMasks (int32_t nSide, const CFixVector& pRef, fix xRad, bool bCheckPoke)
{
	int16_t faceBit = 1;

return m_sides [nSide].Masks (pRef, xRad, 1, faceBit, bCheckPoke);
}

// -------------------------------------------------------------------------------
//	Make a just-modified CSegment valid.
//		check all sides to see how many faces they each should have (0, 1, 2)
//		create new vector normals
void CSegment::Setup (void)
{
ComputeCenter ();
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
#if DBG
	if ((SEG_IDX (this) == nDbgSeg) && ((nDbgSide < 0) || (i == nDbgSide)))
		BRP;
#endif
	m_sides [i].Setup (Index (), m_vertices, sideVertIndex [i], m_children [i] < 0);
	if (!m_sides [i].FaceCount () && (m_children [i] >= 0)) {
		PrintLog (0, "Segment %d, side %d is collapsed, but has child %d!\n", SEG_IDX (this), i, m_children [i]);
		SEGMENT (m_children [i])->m_children [(int32_t) oppSideTable [i]] = -1;
		m_children [i] = -1;
		}
	}
}

//	--------------------------------------------------------------------------------
//	Picks a Random point in a CSegment like so:
//		From center, go up to 50% of way towards any of the 8 vertices.
CFixVector CSegment::RandomPoint (void)
{
int32_t nVertex;
do {
	nVertex = Rand (m_nVertices);
} while (m_vertices [nVertex] == 0xFFFF);
CFixVector v = gameData.segData.vertices [m_vertices [nVertex]] - m_vCenter;
v *= (RandShort ());
return v + m_vCenter;
}

//------------------------------------------------------------------------------

int32_t CSegment::HasOpenableDoor (void)
{
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++)
	if (m_sides [i].FaceCount () && m_sides [i].IsOpenableDoor ())
		return i;
return -1;
}
//-----------------------------------------------------------------

#if 1

int32_t CSegment::IsPassable (int32_t nSide, CObject *pObj, bool bIgnoreDoors)
{
	int32_t	nChildSeg = m_children [nSide];

if (nChildSeg == -1)
	return WID_VISIBLE_FLAG;
if (nChildSeg == -2)
	return WID_EXTERNAL_FLAG;
if (!m_sides [nSide].FaceCount ())
	return 0;

CWall* pWall = m_sides [nSide].Wall ();

#if DBG
if (pObj && (pObj->Index () == nDbgObj))
	BRP;
if ((SEG_IDX (this) == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif

if (!pObj) 
	return pWall ? pWall->IsPassable (NULL, bIgnoreDoors) : WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG;

uint8_t nChildType = SEGMENT (nChildSeg)->m_function;
if (SEGMENT (nChildSeg)->HasBlockedProp ()) {
	if ((pObj->info.nType == OBJ_PLAYER) || (pObj->info.nType == OBJ_ROBOT) || (pObj->info.nType == OBJ_POWERUP))
		return WID_VISIBLE_FLAG;
	}
else if ((m_function == SEGMENT_FUNC_SPEEDBOOST) && (nChildType != SEGMENT_FUNC_SPEEDBOOST)) {
	// handle the player in a speed boost area. The player must only exit speed boost areas through a segment side with a speed boost trigger
	if ((pObj == gameData.objData.pConsole) && (gameData.objData.speedBoost [pObj->Index ()].bBoosted > 0)) {
		if (!pWall)
			return WID_VISIBLE_FLAG;
		CTrigger* pTrigger = pWall->Trigger ();
		if (!pTrigger || (pTrigger->m_info.nType != TT_SPEEDBOOST))
			return WID_VISIBLE_FLAG;
		return pWall->IsPassable (pObj, bIgnoreDoors);
		}
	}
return pWall ? pWall->IsPassable (pObj, bIgnoreDoors) : WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG;
}

#else

int32_t CSegment::IsPassable (int32_t nSide, CObject *pObj, bool bIgnoreDoors)
{
	int32_t	nChild = m_children [nSide];

if (nChild == -1)
	return WID_VISIBLE_FLAG;
if (nChild == -2)
	return WID_EXTERNAL_FLAG;

CWall* pWall = m_sides [nSide].Wall ();
CSegment* pChild = SEGMENT (nChild);

#if DBG
if ((SEG_IDX (this) == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif

if ((pObj == gameData.objData.pConsole) &&
	 (gameData.objData.speedBoost [pObj->Index ()].bBoosted > 0) &&
	 (m_function == SEGMENT_FUNC_SPEEDBOOST) && (pChild->m_function != SEGMENT_FUNC_SPEEDBOOST) &&
	 (!pWall || (GEOTRIGGER (pWall->nTrigger)->m_info.nType != TT_SPEEDBOOST)))
	return WID_VISIBLE_FLAG;

if (pChild->HasBlockedProp ())
	return (pObj && ((pObj->info.nType == OBJ_PLAYER) || (pObj->info.nType == OBJ_ROBOT) || (pObj->info.nType == OBJ_POWERUP)))
			 ? WID_VISIBLE_FLAG
			 : pWall ? pWall->IsPassable (pObj, bIgnoreDoors) : WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG;

if (!pWall)
	return WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG;

return pWall->IsPassable (pObj, bIgnoreDoors);
}

#endif

//------------------------------------------------------------------------------

bool CSegment::IsVertex (int32_t nVertex)
{
for (int32_t i = 0; i < 8; i++)
	if (nVertex == m_vertices [i])
		return true;
return false;
}

//------------------------------------------------------------------------------

int32_t CSegment::Physics (fix& xDamage)
{
if (HasLavaProp ()) {
	xDamage = gameData.pigData.tex.tMapInfo [0][404].damage / 2;
	return 1;
	}
if (HasWaterProp ()) {
	xDamage = 0;
	return 2;
	}
return 0;
}

//-----------------------------------------------------------------
//set the nBaseTex or nOvlTex field for a CWall/door
void CSegment::SetTexture (int32_t nSide, CSegment *pConnSeg, int16_t nConnSide, int32_t nAnim, int32_t nFrame)
{
ENTER (0, 0);
	tWallEffect	*pAnim = gameData.wallData.pAnim + nAnim;
	int16_t		nTexture = pAnim->frames [(pAnim->flags & WCF_ALTFMT) ? 0 : nFrame];
	CBitmap*		pBm;
	int32_t		nFrames;

//if (gameData.demoData.nState == ND_STATE_PLAYBACK)
//	RETURN
if (nConnSide < 0)
	pConnSeg = NULL;
if (pAnim->flags & WCF_ALTFMT) {
	if (gameData.demoData.nState == ND_STATE_RECORDING)
		NDRecordWallSetTMapNum1 (SEG_IDX (this), (uint8_t) nSide, (int16_t) (pConnSeg ? SEG_IDX (pConnSeg) : -1), (uint8_t) nConnSide, nTexture, nAnim, nFrame);
	nFrames = pAnim->nFrameCount;
	pBm = SetupHiresAnim (reinterpret_cast<int16_t*> (pAnim->frames), nFrames, -1, 1, 0, &nFrames);
	//if (pAnim->flags & WCF_TMAP1)
	if (!pBm)
		pAnim->flags &= ~WCF_ALTFMT;
	else {
		pBm->SetWallAnim (1);
		if (!gameOpts->ogl.bGlTexMerge)
			pAnim->flags &= ~WCF_ALTFMT;
		else if (!pBm->Frames ())
			pAnim->flags &= ~WCF_ALTFMT;
		else {
			pAnim->flags |= WCF_INITIALIZED;
			if (gameData.demoData.nState == ND_STATE_RECORDING) {
				pBm->SetTranspType (3);
				pBm->SetupTexture (1, 1);
				}
			pBm->SetCurFrame (pBm->Frames () + nFrame);
			pBm->CurFrame ()->SetTranspType (3);
			pBm->CurFrame ()->SetupTexture (1, 1);
			if (++nFrame > nFrames)
				nFrame = nFrames;
			}
		}
	}
else if ((pAnim->flags & WCF_TMAP1) || !m_sides [nSide].m_nOvlTex) {
	m_sides [nSide].m_nBaseTex = nTexture;
	if (pConnSeg)
		pConnSeg->m_sides [nConnSide].m_nBaseTex = nTexture;
	if (gameData.demoData.nState == ND_STATE_RECORDING)
		NDRecordWallSetTMapNum1 (SEG_IDX (this), (uint8_t) nSide, (int16_t) (pConnSeg ? SEG_IDX (pConnSeg) : -1), (uint8_t) nConnSide, nTexture, nAnim, nFrame);
	}
else {
	m_sides [nSide].m_nOvlTex = nTexture;
	if (pConnSeg)
		pConnSeg->m_sides [nConnSide].m_nOvlTex = nTexture;
	if (gameData.demoData.nState == ND_STATE_RECORDING)
		NDRecordWallSetTMapNum2 (SEG_IDX (this), (uint8_t) nSide, (int16_t) (pConnSeg ? SEG_IDX (pConnSeg) : -1), (uint8_t) nConnSide, nTexture, nAnim, nFrame);
	}
m_sides [nSide].m_nFrame = -nFrame;
if (pConnSeg)
	pConnSeg->m_sides [nConnSide].m_nFrame = -nFrame;
RETURN
}

//-----------------------------------------------------------------

int32_t CSegment::PokesThrough (int32_t nObject, int32_t nSide)
{
	CObject *pObj = OBJECT (nObject);

return (pObj->info.xSize && SideMasks (nSide, pObj->info.position.vPos, pObj->info.xSize, true).m_side);
}

//-----------------------------------------------------------------
//returns true of door in unobjstructed (& thus can close)
int32_t CSegment::DoorIsBlocked (int32_t nSide, bool bIgnoreMarker)
{
	int16_t		nConnSide;
	CSegment*	pConnSeg;
	int16_t		nObject, nType;

pConnSeg = SEGMENT (m_children [nSide]);
nConnSide = ConnectedSide (pConnSeg);
Assert(nConnSide != -1);

//go through each CObject in each of two segments, and see if
//it pokes into the connecting pSeg

for (nObject = m_objects; nObject != -1; nObject = OBJECT (nObject)->info.nNextInSeg) {
	nType = OBJECT (nObject)->info.nType;
	if ((nType == OBJ_WEAPON) || (nType == OBJ_FIREBALL) || (nType == OBJ_EXPLOSION) || (nType == OBJ_EFFECT) || (bIgnoreMarker && (nType == OBJ_MARKER)))
		continue;
	if (PokesThrough (nObject, nSide) || pConnSeg->PokesThrough (nObject, nConnSide))
		return 1;	//not free
		}
return 0; 	//doorway is free!
}

// -------------------------------------------------------------------------------
//when the CWall has used all its hitpoints, this will destroy it
void CSegment::BlastWall (int32_t nSide)
{
ENTER (0, 0);
	CSegment	*pConnSeg = NULL;
	int16_t	nConnSide = -1;
	int16_t	nConnWall = NO_WALL;
	int32_t	a, n;
	CWall		*pWall = Wall (nSide);

if (!pWall)
	return;
pWall->hps = -1;	//say it's blasted
if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_BLAST_SINGLE, this - SEGMENTS, nSide, pWall - WALLS);
	}
else {
	pConnSeg = SEGMENT (m_children [nSide]);
	if (pConnSeg) {
		nConnSide = ConnectedSide (pConnSeg);
		nConnWall = pConnSeg->WallNum (nConnSide);
		KillStuckObjects (nConnWall);
		}
	}
KillStuckObjects (WallNum (nSide));

//if this is an exploding wall, explode it
if ((gameData.wallData.pAnim [pWall->nClip].flags & WCF_EXPLODES) && !(pWall->flags & WALL_BLASTED))
	ExplodeWall (Index (), nSide);
else {
	//if not exploding, set final frame, and make door passable
	a = pWall->nClip;
	n = WallEffectFrameCount (gameData.wallData.pAnim + a);
	SetTexture (nSide, pConnSeg, nConnSide, a, n - 1);
	pWall->flags |= WALL_BLASTED;
	if (IS_WALL (nConnWall))
		WALL (nConnWall)->flags |= WALL_BLASTED;
	}
RETURN
}

//-----------------------------------------------------------------
// Destroys a blastable CWall.
void CSegment::DestroyWall (int32_t nSide)
{
CWall* pWall = Wall (nSide);
if (pWall) {
	if (pWall->nType == WALL_BLASTABLE)
		BlastWall (nSide);
	else
		Error (TXT_WALL_INDESTRUCTIBLE);
	}
}

//-----------------------------------------------------------------
// Deteriorate appearance of CWall. (Changes bitmap (paste-ons))
void CSegment::DamageWall (int32_t nSide, fix damage)
{
ENTER (0, 0);
	int32_t	a, i, n;
	int16_t	nConnSide, nConnWall;
	CWall		*pWall = Wall (nSide);
	CSegment *pConnSeg;

if (!pWall) {
#if TRACE
	console.printf (CON_DBG, "Damaging illegal CWall\n");
#endif
	RETURN
	}
if (pWall->nType != WALL_BLASTABLE)
	RETURN
if ((pWall->flags & WALL_BLASTED) || (pWall->hps < 0))
	RETURN

if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_DMG_SINGLE, this - SEGMENTS, nSide, pWall - WALLS);
	pConnSeg = NULL;
	nConnSide = -1;
	nConnWall = NO_WALL;
	}
else {
	pConnSeg = SEGMENT (m_children [nSide]);
	nConnSide = ConnectedSide (pConnSeg);
	Assert(nConnSide != -1);
	nConnWall = pConnSeg->WallNum (nConnSide);
	}
pWall->hps -= damage;
if (IS_WALL (nConnWall))
	WALL (nConnWall)->hps -= damage;
a = pWall->nClip;
n = WallEffectFrameCount (gameData.wallData.pAnim + a);
if (pWall->hps < WALL_HPS * 1 / n) {
	BlastWall (nSide);
	if (IsMultiGame)
		MultiSendDoorOpen (SEG_IDX (this), nSide, pWall->flags);
	}
else {
	for (i = 0; i < n; i++)
		if (pWall->hps < WALL_HPS * (n - i) / n)
			SetTexture (nSide, pConnSeg, nConnSide, a, i);
	}
RETURN
}

//-----------------------------------------------------------------
// Opens a door
void CSegment::OpenDoor (int32_t nSide)
{
ENTER (0, 0);
CWall* pWall;
if (!(pWall = Wall (nSide)))
	RETURN

CActiveDoor* pDoor;
if (!(pDoor = pWall->OpenDoor ()))
	RETURN

	int16_t		nConnSide = -1, nConnWall = NO_WALL;
	CSegment*	pConnSeg = NULL;
// So that door can't be shot while opening
if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_OPEN_SINGLE, this - SEGMENTS, nSide, WallNum (nSide));
	}
else {
	pConnSeg = SEGMENT (m_children [nSide]);
	if (pConnSeg) {
		nConnSide = ConnectedSide (pConnSeg);
		if (nConnSide >= 0) {
			CWall* pWall = pConnSeg->Wall (nConnSide);
			if (pWall)
				pWall->state = WALL_DOOR_OPENING;
			}	
		}
	}

//KillStuckObjects(WallNumP (pConnSeg, nConnSide));
pDoor->nFrontWall [0] = WallNum (nSide);
pDoor->nBackWall [0] = nConnWall;
Assert(SEG_IDX (this) != -1);
if (gameData.demoData.nState == ND_STATE_RECORDING)
	NDRecordDoorOpening (SEG_IDX (this), nSide);
if (IS_WALL (pWall->nLinkedWall) && IS_WALL (nConnWall) && (pWall->nLinkedWall == nConnWall)) {
	CWall *pLinkedWall = WALL (pWall->nLinkedWall);
	CSegment *pLinkedSeg = SEGMENT (pLinkedWall->nSegment);
	pLinkedWall->state = WALL_DOOR_OPENING;
	pConnSeg = SEGMENT (pLinkedSeg->m_children [pLinkedWall->nSide]);
	if (IS_WALL (nConnWall))
		WALL (nConnWall)->state = WALL_DOOR_OPENING;
	pDoor->nPartCount = 2;
	pDoor->nFrontWall [1] = pWall->nLinkedWall;
	pDoor->nBackWall [1] = pLinkedSeg->ConnectedSide (pConnSeg);
	}
else
	pDoor->nPartCount = 1;
if (gameData.demoData.nState != ND_STATE_PLAYBACK) {
	if ((pWall->nClip >= 0) && (gameData.wallData.pAnim [pWall->nClip].openSound > -1))
		CreateSound (gameData.wallData.pAnim [pWall->nClip].openSound, nSide);
	}
RETURN
}

//-----------------------------------------------------------------
// Closes a door
void CSegment::CloseDoor (int32_t nSide, bool bForce)
{
ENTER (0, 0);
CWall*	pWall = Wall (nSide);
if (!pWall)
	RETURN
if ((pWall->state == WALL_DOOR_CLOSING) ||		//already closing
	 ((pWall->state == WALL_DOOR_WAITING) && !(bForce && gameStates.app.bD2XLevel)) ||		//open, waiting to close
	 (pWall->state == WALL_DOOR_CLOSED))			//closed
	RETURN
if (DoorIsBlocked (nSide, pWall->IgnoreMarker ()))
	RETURN

CActiveDoor*	pDoor = pWall->CloseDoor (bForce);
if (!pDoor)
	RETURN

CSegment *pConnSeg = SEGMENT (m_children [nSide]);
int16_t nConnSide = pConnSeg ? ConnectedSide (pConnSeg) : -1;
int16_t nConnWall = pConnSeg ? pConnSeg->WallNum (nConnSide) : NO_WALL;
if (IS_WALL (nConnWall))
	WALL (nConnWall)->state = WALL_DOOR_CLOSING;
pDoor->nFrontWall [0] = WallNum (nSide);
pDoor->nBackWall [0] = nConnWall;
if (gameData.demoData.nState == ND_STATE_RECORDING)
	NDRecordDoorOpening (SEG_IDX (this), nSide);
	pDoor->nPartCount = 1;
if (gameData.demoData.nState != ND_STATE_PLAYBACK) {
	if (gameData.wallData.pAnim [pWall->nClip].openSound > -1)
		CreateSound (gameData.wallData.pAnim [pWall->nClip].openSound, nSide);
	}
RETURN
}

//-----------------------------------------------------------------

// start the transition from closed -> open CWall
void CSegment::StartCloak (int32_t nSide)
{
ENTER (0, 0);
if (gameData.demoData.nState == ND_STATE_PLAYBACK)
	RETURN

CWall* pWall = Wall (nSide);
if (!pWall)
	RETURN

if (pWall->nType == WALL_OPEN || pWall->state == WALL_DOOR_CLOAKING)		//already open or cloaking
	RETURN

	CCloakingWall*	pCloakWall;
	CSegment*		pConnSeg;
	int16_t			nConnSide;
	int32_t			i;
	int16_t			nConnWall;


pConnSeg = SEGMENT (m_children [nSide]);
nConnSide = ConnectedSide (pConnSeg);

if (!(pCloakWall = pWall->StartCloak ())) {
	pWall->nType = WALL_OPEN;
	if ((pWall = pConnSeg->Wall (nConnSide)))
		pWall->nType = WALL_OPEN;
	RETURN
	}

nConnWall = pConnSeg->WallNum (nConnSide);
if (IS_WALL (nConnWall))
	WALL (nConnWall)->state = WALL_DOOR_CLOAKING;
pCloakWall->nFrontWall = WallNum (nSide);
pCloakWall->nBackWall = nConnWall;
Assert(SEG_IDX (this) != -1);
//Assert(!IS_WALL (pWall->nLinkedWall));
if (gameData.demoData.nState != ND_STATE_PLAYBACK) {
	CreateSound (SOUND_WALL_CLOAK_ON, nSide);
	}
for (i = 0; i < 4; i++) {
	pCloakWall->front_ls [i] = m_sides [nSide].m_uvls [i].l;
	if (IS_WALL (nConnWall))
		pCloakWall->back_ls [i] = pConnSeg->m_sides [nConnSide].m_uvls [i].l;
	}
RETURN
}

//-----------------------------------------------------------------
// start the transition from open -> closed CWall
void CSegment::StartDecloak (int32_t nSide)
{
ENTER (0, 0);
if (gameData.demoData.nState == ND_STATE_PLAYBACK)
	RETURN
	
CWall *pWall = Wall (nSide);
if (!pWall)
	RETURN
if ((pWall->nType == WALL_CLOSED) || (pWall->state == WALL_DOOR_DECLOAKING))		//already closed or decloaking
	RETURN

CSegment *pConnSeg = SEGMENT (m_children [nSide]);
int16_t nConnSide = pConnSeg ? ConnectedSide (pConnSeg) : -1;
CCloakingWall *pCloakWall = pWall->StartDecloak ();
if (!pCloakWall) {
	pWall->state = WALL_CLOSED;
	if (pConnSeg && (pWall = pConnSeg->Wall (nConnSide)))
		pWall->state = WALL_CLOSED;
	RETURN
	}
// So that door can't be shot while opening
if (pConnSeg && (pWall = pConnSeg->Wall (nConnSide)))
	pWall->state = WALL_DOOR_DECLOAKING;
pCloakWall->nFrontWall = WallNum (nSide);
pCloakWall->nBackWall = pConnSeg->WallNum (nConnSide);
if (gameData.demoData.nState != ND_STATE_PLAYBACK) {
	CreateSound (SOUND_WALL_CLOAK_OFF, nSide);
	}
for (int32_t i = 0; i < 4; i++) {
	pCloakWall->front_ls [i] = m_sides [nSide].m_uvls [i].l;
	if (pWall)
		pCloakWall->back_ls [i] = pConnSeg->m_sides [nConnSide].m_uvls [i].l;
	}
RETURN
}

//-----------------------------------------------------------------

int32_t CSegment::AnimateOpeningDoor (int32_t nSide, fix xElapsedTime)
{
CWall	*pWall = Wall (nSide);
if (!pWall)
	return 3;
return pWall->AnimateOpeningDoor (xElapsedTime);
}

//-----------------------------------------------------------------

int32_t CSegment::AnimateClosingDoor (int32_t nSide, fix xElapsedTime)
{
CWall	*pWall = Wall (nSide);
if (!pWall)
	return 3;
return pWall->AnimateClosingDoor (xElapsedTime);
}

//-----------------------------------------------------------------
// Turns on an illusionary CWall (This will be used primarily for
//  CWall switches or triggers that can turn on/off illusionary walls.)
void CSegment::IllusionOn (int32_t nSide)
{
CWall* pWall = Wall (nSide);
if (!pWall)
	return;

pWall->flags &= ~WALL_ILLUSION_OFF;
if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_ILLUS_SINGLE, this - SEGMENTS, nSide);
	}
else {
	CSegment *pConnSeg = SEGMENT (m_children [nSide]);
	if (pConnSeg) {
		int16_t nConnSide = ConnectedSide (pConnSeg);
		if ((pWall = pConnSeg->Wall (nConnSide)))
			pWall->flags &= ~WALL_ILLUSION_OFF;
		}
	}
}

//-----------------------------------------------------------------
// Turns off an illusionary CWall (This will be used primarily for
//  CWall switches or triggers that can turn on/off illusionary walls.)
void CSegment::IllusionOff (int32_t nSide)
{
CWall*	pWall = Wall (nSide);
if (!pWall)
	return;

pWall->flags |= WALL_ILLUSION_OFF;
KillStuckObjects (pWall - WALLS);

if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_ILLUS_SINGLE,	this - SEGMENTS, nSide);
	}
else {
	CSegment *pConnSeg = SEGMENT (m_children [nSide]);
	if (pConnSeg && (pWall = pConnSeg->Wall (nSide))) {
		pWall->flags |= WALL_ILLUSION_OFF;
		KillStuckObjects (pWall - WALLS);
		}
	}
}

//-----------------------------------------------------------------
// Opens doors/destroys CWall/shuts off triggers.
void CSegment::ToggleWall (int32_t nSide)
{
CWall	*pWall = Wall (nSide);
if (!pWall)
	return;
if (gameData.demoData.nState == ND_STATE_RECORDING)
	NDRecordWallToggle (SEG_IDX (this), nSide);
if (pWall->nType == WALL_BLASTABLE)
	DestroyWall (nSide);
if ((pWall->nType == WALL_DOOR) && (pWall->state == WALL_DOOR_CLOSED))
	OpenDoor (nSide);
}

//-----------------------------------------------------------------
// Determines what happens when a CWall is shot
//returns info about CWall.  see wall[HA] for codes
//obj is the CObject that hit...either a weapon or the player himself
//nPlayer is the number the player who hit the CWall or fired the weapon,
//or -1 if a robot fired the weapon

int32_t CSegment::ProcessWallHit (int32_t nSide, fix damage, int32_t nPlayer, CObject *pObj)
{
ENTER (0, 0);
	CWall* pWall = Wall (nSide);

if (!pWall)
	RETVAL (WHP_NOT_SPECIAL)

if (gameData.demoData.nState == ND_STATE_RECORDING)
	NDRecordWallHitProcess (SEG_IDX (this), nSide, damage, nPlayer);

if (pWall->nType == WALL_BLASTABLE) {
	if (pObj->cType.laserInfo.parent.nType == OBJ_PLAYER)
		DamageWall (nSide, damage);
	RETVAL (WHP_BLASTABLE)
	}

if (nPlayer != N_LOCALPLAYER)	//return if was robot fire
	RETVAL (WHP_NOT_SPECIAL)

Assert(nPlayer > -1);

//	Determine whether player is moving forward.  If not, don't say negative
//	messages because he probably didn't intentionally hit the door.
RETVAL (pWall->ProcessHit (nPlayer, pObj))
}

//-----------------------------------------------------------------
// Checks for a CTrigger whenever an CObject hits a CTrigger nSide.
void CSegment::OperateTrigger (int32_t nSide, CObject *pObj, int32_t bShot)
{
CTrigger* pTrigger = Trigger (nSide);

if (!pTrigger)
	return;

if (pObj
	 ? pTrigger->Operate (pObj->Index (), pObj->IsPlayer () ? pObj->info.nId : pObj->IsGuideBot () ? N_LOCALPLAYER : -1, bShot, 0)
	 : pTrigger->Operate (-1, -1, bShot, 0)
	)
	return;
if (gameData.demoData.nState == ND_STATE_RECORDING)
	NDRecordTrigger (Index (), nSide, pObj->Index (), bShot);
if (IsMultiGame && !pTrigger->ClientOnly ())
	MultiSendTrigger (Index (), pObj->Index ());
}

//	-----------------------------------------------------------------------------
//if an effect is hit, and it can blow up, then blow it up
//returns true if it blew up
int32_t CSegment::TextureIsDestructable (int32_t nSide, tDestructableTextureProps* pTexProps)
{
ENTER (0, 0);
	tDestructableTextureProps	dtp;

if (!pTexProps)
	pTexProps = &dtp;
pTexProps->nOvlTex = m_sides [nSide].m_nOvlTex;
if (!pTexProps->nOvlTex)
	RETVAL (0)
pTexProps->nOvlOrient = m_sides [nSide].m_nOvlOrient;	//nOvlTex flags
pTexProps->nEffect = gameData.pigData.tex.pTexMapInfo [pTexProps->nOvlTex].nEffectClip;
if (pTexProps->nEffect < 0) {
	if (IsMultiGame && netGameInfo.m_info.bIndestructibleLights)
		RETVAL (0)
	pTexProps->nBitmap = gameData.pigData.tex.pTexMapInfo [pTexProps->nOvlTex].destroyed;
	if (pTexProps->nBitmap < 0)
		RETVAL (0)
	pTexProps->nSwitchType = 0;
	}
else {
	tEffectInfo* pEffectInfo = gameData.effectData.pEffect + pTexProps->nEffect;
	if (pEffectInfo->flags & EF_ONE_SHOT)
		RETVAL (0)
	pTexProps->nBitmap = pEffectInfo->destroyed.nTexture;
	if (pTexProps->nBitmap < 0)
		RETVAL (0)
	pTexProps->nSwitchType = 1;
	}
RETVAL (1)
}

//	-----------------------------------------------------------------------------
//if an effect is hit, and it can blow up, then blow it up
//returns true if it blew up

int32_t CSegment::BlowupTexture (int32_t nSide, CFixVector& vHit, CObject* pBlower, int32_t bForceBlowup)
{
ENTER (0, 0);
	tDestructableTextureProps	dtp;
	int16_t			nSound, bPermaTrigger;
	uint8_t			vc;
	fix				u, v;
	fix				xDestroyedSize;
	tEffectInfo*	pEffectInfo = NULL;
	CWall*			pWall = Wall (nSide);
	CTrigger*		pTrigger;
	CObject*			pParent = (!pBlower || (pBlower->cType.laserInfo.parent.nObject < 0)) ? NULL : OBJECT (pBlower->cType.laserInfo.parent.nObject);
	//	If this CWall has a CTrigger and the pBlower-upper is not the player or the buddy, abort!

if (pParent) {
	// only the player and the guidebot may blow a texture up if a trigger is attached to it
	if (pParent && (pParent->info.nType != OBJ_PLAYER) && ((pParent->info.nType != OBJ_ROBOT) || !pParent->IsGuideBot ()) && pWall && (pWall->nTrigger < gameData.trigData.m_nTriggers [0]))
		RETVAL (0)
	}

if (!TextureIsDestructable (nSide, &dtp))
	RETVAL (0)
//check if it's an animation (monitor) or casts light
LoadTexture (gameData.pigData.tex.pBmIndex [dtp.nOvlTex].index, 0, gameStates.app.bD1Data);
//this can be blown up...did we hit it?
if (!bForceBlowup) {
	HitPointUV (nSide, &u, &v, NULL, vHit, 0);	//evil: always say face zero
	bForceBlowup = !PixelTranspType (dtp.nOvlTex, dtp.nOvlOrient, m_sides [nSide].m_nFrame, u, v);
	}
if (!bForceBlowup)
	RETVAL (0)

//note: this must get called before the texture changes,
//because we use the light value of the texture to change
//the static light in the CSegment
bPermaTrigger = (pTrigger = Trigger (nSide)) && pTrigger->Flagged (TF_PERMANENT);
if (!bPermaTrigger)
	SubtractLight (Index (), nSide);
if (gameData.demoData.nState == ND_STATE_RECORDING)
	NDRecordEffectBlowup (Index (), nSide, vHit);
if (dtp.nSwitchType) {
	pEffectInfo = gameData.effectData.pEffect + dtp.nEffect;
	xDestroyedSize = pEffectInfo->destroyed.xSize;
	vc = pEffectInfo->destroyed.nAnimation;
	}
else {
	xDestroyedSize = I2X (20);
	vc = 3;
	}

CreateExplosion (int16_t (Index ()), vHit, xDestroyedSize, vc);
if (dtp.nSwitchType) {
	if ((nSound = gameData.effectData.vClipP [vc].nSound) != -1)
		audio.CreateSegmentSound (nSound, Index (), 0, vHit);
	if ((nSound = pEffectInfo->nSound) != -1)		//kill sound
		audio.DestroySegmentSound (Index (), nSide, nSound);
	if (!bPermaTrigger && (pEffectInfo->destroyed.nEffect != -1) && (gameData.effectData.pEffect [pEffectInfo->destroyed.nEffect].nSegment == -1)) {
		tEffectInfo	*pNewEffect = gameData.effectData.pEffect + pEffectInfo->destroyed.nEffect;
		int32_t nNewBm = pNewEffect->changing.nWallTexture;
		if (ChangeTextures (-1, nNewBm, nSide)) {
			pNewEffect->xTimeLeft = EffectFrameTime (pNewEffect);
			pNewEffect->nCurFrame = 0;
			pNewEffect->nSegment = Index ();
			pNewEffect->nSide = nSide;
			pNewEffect->flags |= EF_ONE_SHOT | pEffectInfo->flags;
			pNewEffect->flags &= ~EF_INITIALIZED;
			pNewEffect->destroyed.nTexture = pEffectInfo->destroyed.nTexture;

			Assert ((nNewBm != 0) && (m_sides [nSide].m_nOvlTex != 0));
			m_sides [nSide].m_nOvlTex = nNewBm;		//replace with destoyed
			}
		}
	else {
		if (!bPermaTrigger)
			m_sides [nSide].m_nOvlTex = dtp.nBitmap;		//replace with destoyed
		}
	}
else {
	if (!bPermaTrigger)
		m_sides [nSide].m_nOvlTex = gameData.pigData.tex.pTexMapInfo [dtp.nOvlTex].destroyed;
	//assume this is a light, and play light sound
	audio.CreateSegmentSound (SOUND_LIGHT_BLOWNUP, Index (), 0, vHit);
	}
RETVAL (1)		//blew up!
}

//------------------------------------------------------------------------------

int32_t CSegment::TexturedSides (void)
{
	int32_t nSides = 0;

for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++)
	if (m_sides [i].FaceCount () && ((m_children [i] < 0) || m_sides [i].IsTextured ()))
		nSides++;
return nSides;
}

//------------------------------------------------------------------------------

void CSegment::CreateSound (int16_t nSound, int32_t nSide)
{
audio.CreateSegmentSound (nSound, Index (), nSide, SideCenter (nSide));
}

//------------------------------------------------------------------------------

CBitmap* CSegment::ChangeTextures (int16_t nBaseTex, int16_t nOvlTex, int16_t nSide)
{
ENTER (0, 0);
	CBitmap*		bmBot = (nBaseTex < 0) ? NULL : LoadFaceBitmap (nBaseTex, 0, 1);
	CBitmap*		bmTop = (nOvlTex <= 0) ? NULL : LoadFaceBitmap (nOvlTex, 0, 1);
	tSegFaces*	pSegFace = SEGFACES + Index ();
	CSegFace*	pFace = pSegFace->pFace;

for (int32_t i = pSegFace->nFaces; i; i--, pFace++) {
	if ((nSide < 0) || (pFace->m_info.nSide == nSide)) {
		if (bmBot) {
			pFace->bmBot = bmBot;
			pFace->m_info.nBaseTex = nBaseTex;
			}
		if (nOvlTex >= 0) {
			pFace->bmTop = bmTop;
			pFace->m_info.nOvlTex = nOvlTex;
			}
		}
	}
RETVAL (bmBot ? bmBot : bmTop)
}

//------------------------------------------------------------------------------

int32_t CSegment::ChildIndex (int32_t nChild)
{
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++)
	if (m_sides [i].FaceCount () && (m_children [i] == nChild))
		return i;
return -1;
}

//------------------------------------------------------------------------------

CSide* CSegment::AdjacentSide (int32_t nSegment)
{
int32_t i = ChildIndex (nSegment);
return (i < 0) ? NULL : m_sides + i;
}

//------------------------------------------------------------------------------

CSide* CSegment::OppositeSide (int32_t nSide)
{
if (m_children [nSide] < 0)
	return NULL;
return SEGMENT (m_children [nSide])->AdjacentSide (Index ());
}

//	-----------------------------------------------------------------------------
// Check whether side nSide in segment nSegment can be seen from this side.

int32_t CSegment::SeesConnectedSide (int16_t nSide, int16_t nChildSeg, int16_t nChildSide)
{
if ((nSide < 0) || (nChildSide < 0))
	return 0;
CSegment *pSeg = SEGMENT (nChildSeg);
if (!pSeg)
	return 0;
if (nChildSeg == Index ())
	return 1;
if (pSeg->ChildId (nChildSide) == Index ())
	return 0;
CSide *pSide = Side (nSide);
return !pSide->IsConnected (nChildSeg, nChildSide) || pSide->SeesSide (nChildSeg, nChildSide);
}

//------------------------------------------------------------------------------

CSegment* CSegFace::Segment (void) 
{ 
return SEGMENT (m_info.nSegment); 
}

//------------------------------------------------------------------------------

bool CSegment::IsSolid (int32_t nSide)
{
if (nSide < 0)
	return true;
CSide* pSide = Side (nSide);
if (pSide->Shape () > SIDE_SHAPE_TRIANGLE)
	return false;
if (m_children [nSide] < 0)
	return true;
return pSide->IsWall () && pSide->Wall ()->IsSolid ();
}

// -------------------------------------------------------------------------------
//	Set up all segments.
//	gameData.segData.nLastSegment must be set.
//	For all used segments (number <= gameData.segData.nLastSegment), nSegment field must be != -1.

void SetupSegments (fix xPlaneDistTolerance)
{
#if 0
PLANE_DIST_TOLERANCE = DEFAULT_PLANE_DIST_TOLERANCE;
#else
PLANE_DIST_TOLERANCE = (xPlaneDistTolerance < 0) ? DEFAULT_PLANE_DIST_TOLERANCE : xPlaneDistTolerance;
#endif
gameOpts->render.nMathFormat = 0;
RENDERPOINTS.Clear ();
for (int32_t i = 0; i <= gameData.segData.nLastSegment; i++)
	SEGMENT (i)->Setup ();
ComputeVertexNormals ();
gameOpts->render.nMathFormat = gameOpts->render.nDefMathFormat;
}

// -------------------------------------------------------------------------------
// move all deleted vertex ids (>= 0xfff8) up to the end of the vertex list
// and adjust all sides' vertex id indices accordingly
// This is required for level formats supporting triangular segment sides

void CSegment::RemapVertices (void)
{
	uint8_t	map [8];
	uint8_t	i, j;

for (i = 0, j = 0; i < 8; i++) {
	map [i] = j;
	if (m_vertices [i] < 0xfff8)
		m_vertices [j++] = m_vertices [i];
	}
m_nVertices = j;
for (; j < 8; j++)
	m_vertices [j] = 0xffff;
for (int32_t nSide = 0; nSide < 6; nSide++)
	m_sides [nSide].RemapVertices (map);
}

// -------------------------------------------------------------------------------

float CSegment::FaceSize (uint8_t nSide)
{
	uint16_t*	s2v = sideVertIndex [nSide];
	int32_t		nVertices = 0;
	int16_t		v [4];

for (int32_t i = 0; i < 4; i++) {
	uint16_t h = m_vertices [s2v [i]];
	if (h != 0xFFFF) 
		v [nVertices++] = h;
	}
float nSize = TriangleSize (gameData.segData.vertices [v [0]], gameData.segData.vertices [v [1]], gameData.segData.vertices [v [2]]);
if (nVertices == 4)
nSize += TriangleSize (gameData.segData.vertices [v [0]], gameData.segData.vertices [v [2]], gameData.segData.vertices [v [3]]);
return nSize;
}

//------------------------------------------------------------------------------
//eof
