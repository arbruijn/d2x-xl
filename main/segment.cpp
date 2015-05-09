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
	int16_t*	childP = other->m_children;

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
CSegMasks CSegment::Masks (const CFixVector& refP, fix xRad)
{
	int16_t			nSide, faceBit;
	CSegMasks	masks;

//check refPoint against each CSide of CSegment. return bitmask
masks.m_valid = 1;
for (nSide = 0, faceBit = 1; nSide < SEGMENT_SIDE_COUNT; nSide++, faceBit <<= 2)
	if (m_sides [nSide].FaceCount ())
		masks |= m_sides [nSide].Masks (refP, xRad, 1 << nSide, faceBit);
return masks;
}

// -------------------------------------------------------------------------------
//returns 3 different bitmasks with info telling if this sphere is in
//this CSegment.  See CSegMasks structure for info on fields
CSegMasks CSegment::SideMasks (int32_t nSide, const CFixVector& refP, fix xRad, bool bCheckPoke)
{
	int16_t faceBit = 1;

return m_sides [nSide].Masks (refP, xRad, 1, faceBit, bCheckPoke);
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

int32_t CSegment::IsPassable (int32_t nSide, CObject *objP, bool bIgnoreDoors)
{
	int32_t	nChildSeg = m_children [nSide];

if (nChildSeg == -1)
	return WID_VISIBLE_FLAG;
if (nChildSeg == -2)
	return WID_EXTERNAL_FLAG;
if (!m_sides [nSide].FaceCount ())
	return 0;

CWall* wallP = m_sides [nSide].Wall ();

#if DBG
if (objP && (objP->Index () == nDbgObj))
	BRP;
if ((SEG_IDX (this) == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif

if (!objP) 
	return wallP ? wallP->IsPassable (NULL, bIgnoreDoors) : WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG;

uint8_t nChildType = SEGMENT (nChildSeg)->m_function;
if (SEGMENT (nChildSeg)->HasBlockedProp ()) {
	if ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT) || (objP->info.nType == OBJ_POWERUP))
		return WID_VISIBLE_FLAG;
	}
else if ((m_function == SEGMENT_FUNC_SPEEDBOOST) && (nChildType != SEGMENT_FUNC_SPEEDBOOST)) {
	// handle the player in a speed boost area. The player must only exit speed boost areas through a segment side with a speed boost trigger
	if ((objP == gameData.objData.consoleP) && gameData.objData.speedBoost [objP->Index ()].bBoosted) {
		if (!wallP)
			return WID_VISIBLE_FLAG;
		CTrigger* trigP = wallP->Trigger ();
		if (!trigP || (trigP->m_info.nType != TT_SPEEDBOOST))
			return WID_VISIBLE_FLAG;
		return wallP->IsPassable (objP, bIgnoreDoors);
		}
	}
return wallP ? wallP->IsPassable (objP, bIgnoreDoors) : WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG;
}

#else

int32_t CSegment::IsPassable (int32_t nSide, CObject *objP, bool bIgnoreDoors)
{
	int32_t	nChild = m_children [nSide];

if (nChild == -1)
	return WID_VISIBLE_FLAG;
if (nChild == -2)
	return WID_EXTERNAL_FLAG;

CWall* wallP = m_sides [nSide].Wall ();
CSegment* childP = SEGMENT (nChild);

#if DBG
if ((SEG_IDX (this) == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif

if ((objP == gameData.objData.consoleP) &&
	 gameData.objData.speedBoost [objP->Index ()].bBoosted &&
	 (m_function == SEGMENT_FUNC_SPEEDBOOST) && (childP->m_function != SEGMENT_FUNC_SPEEDBOOST) &&
	 (!wallP || (GEOTRIGGER (wallP->nTrigger)->m_info.nType != TT_SPEEDBOOST)))
	return WID_VISIBLE_FLAG;

if (childP->HasBlockedProp ())
	return (objP && ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT) || (objP->info.nType == OBJ_POWERUP)))
			 ? WID_VISIBLE_FLAG
			 : wallP ? wallP->IsPassable (objP, bIgnoreDoors) : WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG;

if (!wallP)
	return WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG;

return wallP->IsPassable (objP, bIgnoreDoors);
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
	xDamage = gameData.pig.tex.tMapInfo [0][404].damage / 2;
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
void CSegment::SetTexture (int32_t nSide, CSegment *connSegP, int16_t nConnSide, int32_t nAnim, int32_t nFrame)
{
	tWallEffect*	animP = gameData.wallData.animP + nAnim;
	int16_t			nTexture = animP->frames [(animP->flags & WCF_ALTFMT) ? 0 : nFrame];
	CBitmap*			bmP;
	int32_t			nFrames;

//if (gameData.demo.nState == ND_STATE_PLAYBACK)
//	return;
if (nConnSide < 0)
	connSegP = NULL;
if (animP->flags & WCF_ALTFMT) {
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordWallSetTMapNum1 (SEG_IDX (this), (uint8_t) nSide, (int16_t) (connSegP ? SEG_IDX (connSegP) : -1), (uint8_t) nConnSide, nTexture, nAnim, nFrame);
	nFrames = animP->nFrameCount;
	bmP = SetupHiresAnim (reinterpret_cast<int16_t*> (animP->frames), nFrames, -1, 1, 0, &nFrames);
	//if (animP->flags & WCF_TMAP1)
	if (!bmP)
		animP->flags &= ~WCF_ALTFMT;
	else {
		bmP->SetWallAnim (1);
		if (!gameOpts->ogl.bGlTexMerge)
			animP->flags &= ~WCF_ALTFMT;
		else if (!bmP->Frames ())
			animP->flags &= ~WCF_ALTFMT;
		else {
			animP->flags |= WCF_INITIALIZED;
			if (gameData.demo.nState == ND_STATE_RECORDING) {
				bmP->SetTranspType (3);
				bmP->SetupTexture (1, 1);
				}
			bmP->SetCurFrame (bmP->Frames () + nFrame);
			bmP->CurFrame ()->SetTranspType (3);
			bmP->CurFrame ()->SetupTexture (1, 1);
			if (++nFrame > nFrames)
				nFrame = nFrames;
			}
		}
	}
else if ((animP->flags & WCF_TMAP1) || !m_sides [nSide].m_nOvlTex) {
	m_sides [nSide].m_nBaseTex = nTexture;
	if (connSegP)
		connSegP->m_sides [nConnSide].m_nBaseTex = nTexture;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordWallSetTMapNum1 (SEG_IDX (this), (uint8_t) nSide, (int16_t) (connSegP ? SEG_IDX (connSegP) : -1), (uint8_t) nConnSide, nTexture, nAnim, nFrame);
	}
else {
	m_sides [nSide].m_nOvlTex = nTexture;
	if (connSegP)
		connSegP->m_sides [nConnSide].m_nOvlTex = nTexture;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordWallSetTMapNum2 (SEG_IDX (this), (uint8_t) nSide, (int16_t) (connSegP ? SEG_IDX (connSegP) : -1), (uint8_t) nConnSide, nTexture, nAnim, nFrame);
	}
m_sides [nSide].m_nFrame = -nFrame;
if (connSegP)
	connSegP->m_sides [nConnSide].m_nFrame = -nFrame;
}

//-----------------------------------------------------------------

int32_t CSegment::PokesThrough (int32_t nObject, int32_t nSide)
{
	CObject *objP = OBJECT (nObject);

return (objP->info.xSize && SideMasks (nSide, objP->info.position.vPos, objP->info.xSize, true).m_side);
}

//-----------------------------------------------------------------
//returns true of door in unobjstructed (& thus can close)
int32_t CSegment::DoorIsBlocked (int32_t nSide, bool bIgnoreMarker)
{
	int16_t		nConnSide;
	CSegment*	connSegP;
	int16_t		nObject, nType;

connSegP = SEGMENT (m_children [nSide]);
nConnSide = ConnectedSide (connSegP);
Assert(nConnSide != -1);

//go through each CObject in each of two segments, and see if
//it pokes into the connecting segP

for (nObject = m_objects; nObject != -1; nObject = OBJECT (nObject)->info.nNextInSeg) {
	nType = OBJECT (nObject)->info.nType;
	if ((nType == OBJ_WEAPON) || (nType == OBJ_FIREBALL) || (nType == OBJ_EXPLOSION) || (nType == OBJ_EFFECT) || (bIgnoreMarker && (nType == OBJ_MARKER)))
		continue;
	if (PokesThrough (nObject, nSide) || connSegP->PokesThrough (nObject, nConnSide))
		return 1;	//not free
		}
return 0; 	//doorway is free!
}

// -------------------------------------------------------------------------------
//when the CWall has used all its hitpoints, this will destroy it
void CSegment::BlastWall (int32_t nSide)
{
	int16_t			nConnSide;
	CSegment*	connSegP;
	int32_t			a, n;
	int16_t			nConnWall;
	CWall*		wallP = Wall (nSide);

if (!wallP)
	return;
wallP->hps = -1;	//say it's blasted
if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_BLAST_SINGLE, this - SEGMENTS, nSide, wallP - WALLS);
	connSegP = NULL;
	nConnSide = -1;
	nConnWall = NO_WALL;
	}
else {
	connSegP = SEGMENT (m_children [nSide]);
	nConnSide = ConnectedSide (connSegP);
	Assert (nConnSide != -1);
	nConnWall = connSegP->WallNum (nConnSide);
	KillStuckObjects (nConnWall);
	}
KillStuckObjects (WallNum (nSide));

//if this is an exploding wall, explode it
if ((gameData.wallData.animP [wallP->nClip].flags & WCF_EXPLODES) && !(wallP->flags & WALL_BLASTED))
	ExplodeWall (Index (), nSide);
else {
	//if not exploding, set final frame, and make door passable
	a = wallP->nClip;
	n = WallEffectFrameCount (gameData.wallData.animP + a);
	SetTexture (nSide, connSegP, nConnSide, a, n - 1);
	wallP->flags |= WALL_BLASTED;
	if (IS_WALL (nConnWall))
		WALL (nConnWall)->flags |= WALL_BLASTED;
	}
}

//-----------------------------------------------------------------
// Destroys a blastable CWall.
void CSegment::DestroyWall (int32_t nSide)
{
	CWall* wallP = Wall (nSide);

if (wallP) {
	if (wallP->nType == WALL_BLASTABLE)
		BlastWall (nSide);
	else
		Error (TXT_WALL_INDESTRUCTIBLE);
	}
}

//-----------------------------------------------------------------
// Deteriorate appearance of CWall. (Changes bitmap (paste-ons))
void CSegment::DamageWall (int32_t nSide, fix damage)
{
	int32_t		a, i, n;
	int16_t		nConnSide, nConnWall;
	CWall		*wallP = Wall (nSide);
	CSegment *connSegP;

if (!wallP) {
#if TRACE
	console.printf (CON_DBG, "Damaging illegal CWall\n");
#endif
	return;
	}
if (wallP->nType != WALL_BLASTABLE)
	return;
if ((wallP->flags & WALL_BLASTED) || (wallP->hps < 0))
	return;

if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_DMG_SINGLE, this - SEGMENTS, nSide, wallP - WALLS);
	connSegP = NULL;
	nConnSide = -1;
	nConnWall = NO_WALL;
	}
else {
	connSegP = SEGMENT (m_children [nSide]);
	nConnSide = ConnectedSide (connSegP);
	Assert(nConnSide != -1);
	nConnWall = connSegP->WallNum (nConnSide);
	}
wallP->hps -= damage;
if (IS_WALL (nConnWall))
	WALL (nConnWall)->hps -= damage;
a = wallP->nClip;
n = WallEffectFrameCount (gameData.wallData.animP + a);
if (wallP->hps < WALL_HPS * 1 / n) {
	BlastWall (nSide);
	if (IsMultiGame)
		MultiSendDoorOpen (SEG_IDX (this), nSide, wallP->flags);
	}
else {
	for (i = 0; i < n; i++)
		if (wallP->hps < WALL_HPS * (n - i) / n)
			SetTexture (nSide, connSegP, nConnSide, a, i);
	}
}

//-----------------------------------------------------------------
// Opens a door
void CSegment::OpenDoor (int32_t nSide)
{
CWall* wallP;
if (!(wallP = Wall (nSide)))
	return;

CActiveDoor* doorP;
if (!(doorP = wallP->OpenDoor ()))
	return;

	int16_t		nConnSide, nConnWall = NO_WALL;
	CSegment*	connSegP;
// So that door can't be shot while opening
if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_OPEN_SINGLE, this - SEGMENTS, nSide, WallNum (nSide));
	connSegP = NULL;
	nConnSide = -1;
	nConnWall = NO_WALL;
	}
else {
	connSegP = SEGMENT (m_children [nSide]);
	nConnSide = ConnectedSide (connSegP);
	if (nConnSide >= 0) {
		CWall* wallP = connSegP->Wall (nConnSide);
		if (wallP)
			wallP->state = WALL_DOOR_OPENING;
		}
	}

//KillStuckObjects(WallNumP (connSegP, nConnSide));
doorP->nFrontWall [0] = WallNum (nSide);
doorP->nBackWall [0] = nConnWall;
Assert(SEG_IDX (this) != -1);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordDoorOpening (SEG_IDX (this), nSide);
if (IS_WALL (wallP->nLinkedWall) && IS_WALL (nConnWall) && (wallP->nLinkedWall == nConnWall)) {
	CWall *linkedWallP = WALL (wallP->nLinkedWall);
	CSegment *linkedSegP = SEGMENT (linkedWallP->nSegment);
	linkedWallP->state = WALL_DOOR_OPENING;
	connSegP = SEGMENT (linkedSegP->m_children [linkedWallP->nSide]);
	if (IS_WALL (nConnWall))
		WALL (nConnWall)->state = WALL_DOOR_OPENING;
	doorP->nPartCount = 2;
	doorP->nFrontWall [1] = wallP->nLinkedWall;
	doorP->nBackWall [1] = linkedSegP->ConnectedSide (connSegP);
	}
else
	doorP->nPartCount = 1;
if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	if ((wallP->nClip >= 0) && (gameData.wallData.animP [wallP->nClip].openSound > -1))
		CreateSound (gameData.wallData.animP [wallP->nClip].openSound, nSide);
	}
}

//-----------------------------------------------------------------
// Closes a door
void CSegment::CloseDoor (int32_t nSide, bool bForce)
{
	CWall*	wallP;

if (!(wallP = Wall (nSide)))
	return;
if ((wallP->state == WALL_DOOR_CLOSING) ||		//already closing
	 ((wallP->state == WALL_DOOR_WAITING) && !(bForce && gameStates.app.bD2XLevel)) ||		//open, waiting to close
	 (wallP->state == WALL_DOOR_CLOSED))			//closed
	return;
if (DoorIsBlocked (nSide, wallP->IgnoreMarker ()))
	return;

	CActiveDoor*	doorP;

if (!(doorP = wallP->CloseDoor (bForce)))
	return;

	CSegment*		connSegP;
	int16_t				nConnSide, nConnWall;

connSegP = SEGMENT (m_children [nSide]);
nConnSide = ConnectedSide (connSegP);
nConnWall = connSegP->WallNum (nConnSide);
if (IS_WALL (nConnWall))
	WALL (nConnWall)->state = WALL_DOOR_CLOSING;
doorP->nFrontWall [0] = WallNum (nSide);
doorP->nBackWall [0] = nConnWall;
Assert(SEG_IDX (this) != -1);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordDoorOpening (SEG_IDX (this), nSide);
#if 0
if (IS_WALL (wallP->nLinkedWall))
	Int3();		//don't think we ever used linked walls
else
#endif
	doorP->nPartCount = 1;
if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	if (gameData.wallData.animP [wallP->nClip].openSound > -1)
		CreateSound (gameData.wallData.animP [wallP->nClip].openSound, nSide);
	}
}

//-----------------------------------------------------------------

// start the transition from closed -> open CWall
void CSegment::StartCloak (int32_t nSide)
{
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return;

CWall* wallP = Wall (nSide);
if (!wallP)
	return;

if (wallP->nType == WALL_OPEN || wallP->state == WALL_DOOR_CLOAKING)		//already open or cloaking
	return;

	CCloakingWall*	cloakWallP;
	CSegment*		connSegP;
	int16_t				nConnSide;
	int32_t				i;
	int16_t				nConnWall;


connSegP = SEGMENT (m_children [nSide]);
nConnSide = ConnectedSide (connSegP);

if (!(cloakWallP = wallP->StartCloak ())) {
	wallP->nType = WALL_OPEN;
	if ((wallP = connSegP->Wall (nConnSide)))
		wallP->nType = WALL_OPEN;
	return;
	}

nConnWall = connSegP->WallNum (nConnSide);
if (IS_WALL (nConnWall))
	WALL (nConnWall)->state = WALL_DOOR_CLOAKING;
cloakWallP->nFrontWall = WallNum (nSide);
cloakWallP->nBackWall = nConnWall;
Assert(SEG_IDX (this) != -1);
//Assert(!IS_WALL (wallP->nLinkedWall));
if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	CreateSound (SOUND_WALL_CLOAK_ON, nSide);
	}
for (i = 0; i < 4; i++) {
	cloakWallP->front_ls [i] = m_sides [nSide].m_uvls [i].l;
	if (IS_WALL (nConnWall))
		cloakWallP->back_ls [i] = connSegP->m_sides [nConnSide].m_uvls [i].l;
	}
}

//-----------------------------------------------------------------
// start the transition from open -> closed CWall
void CSegment::StartDecloak (int32_t nSide)
{
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return;
	CWall				*wallP;

if (!(wallP = Wall (nSide)))
	return;
if (wallP->nType == WALL_CLOSED || wallP->state == WALL_DOOR_DECLOAKING)		//already closed or decloaking
	return;

	CCloakingWall	*cloakWallP;
	int16_t				nConnSide;
	CSegment			*connSegP;
	int32_t				i;

connSegP = SEGMENT (m_children [nSide]);
nConnSide = ConnectedSide (connSegP);
if (!(cloakWallP = wallP->StartDecloak ())) {
	wallP->state = WALL_CLOSED;
	if ((wallP = connSegP->Wall (nConnSide)))
		wallP->state = WALL_CLOSED;
	return;
	}
// So that door can't be shot while opening
if ((wallP = connSegP->Wall (nConnSide)))
	wallP->state = WALL_DOOR_DECLOAKING;
cloakWallP->nFrontWall = WallNum (nSide);
cloakWallP->nBackWall = connSegP->WallNum (nConnSide);
if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	CreateSound (SOUND_WALL_CLOAK_OFF, nSide);
	}
for (i = 0; i < 4; i++) {
	cloakWallP->front_ls [i] = m_sides [nSide].m_uvls [i].l;
	if (wallP)
		cloakWallP->back_ls [i] = connSegP->m_sides [nConnSide].m_uvls [i].l;
	}
}

//-----------------------------------------------------------------

int32_t CSegment::AnimateOpeningDoor (int32_t nSide, fix xElapsedTime)
{
CWall	*wallP = Wall (nSide);
if (!wallP)
	return 3;
return wallP->AnimateOpeningDoor (xElapsedTime);
}

//-----------------------------------------------------------------

int32_t CSegment::AnimateClosingDoor (int32_t nSide, fix xElapsedTime)
{
	CWall	*wallP = Wall (nSide);

if (!wallP)
	return 3;
return wallP->AnimateClosingDoor (xElapsedTime);
}

//-----------------------------------------------------------------
// Turns on an illusionary CWall (This will be used primarily for
//  CWall switches or triggers that can turn on/off illusionary walls.)
void CSegment::IllusionOn (int32_t nSide)
{
	CWall* wallP = Wall (nSide);

if (!wallP)
	return;

	CSegment*	connSegP;
	int16_t			nConnSide;

wallP->flags &= ~WALL_ILLUSION_OFF;
if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_ILLUS_SINGLE, this - SEGMENTS, nSide);
	connSegP = NULL;
	nConnSide = -1;
	}
else {
	connSegP = SEGMENT (m_children [nSide]);
	nConnSide = ConnectedSide (connSegP);
	if ((wallP = connSegP->Wall (nConnSide)))
		wallP->flags &= ~WALL_ILLUSION_OFF;
	}
}

//-----------------------------------------------------------------
// Turns off an illusionary CWall (This will be used primarily for
//  CWall switches or triggers that can turn on/off illusionary walls.)
void CSegment::IllusionOff (int32_t nSide)
{

	CWall*	wallP = Wall (nSide);

if (!wallP)
	return;

	CSegment *connSegP;

wallP->flags |= WALL_ILLUSION_OFF;
KillStuckObjects (wallP - WALLS);

if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_ILLUS_SINGLE,	this - SEGMENTS, nSide);
	connSegP = NULL;
	}
else {
	connSegP = SEGMENT (m_children [nSide]);
	if ((wallP = connSegP->Wall (nSide))) {
		wallP->flags |= WALL_ILLUSION_OFF;
		KillStuckObjects (wallP - WALLS);
		}
	}
}

//-----------------------------------------------------------------
// Opens doors/destroys CWall/shuts off triggers.
void CSegment::ToggleWall (int32_t nSide)
{
CWall	*wallP = Wall (nSide);
if (!wallP)
	return;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordWallToggle (SEG_IDX (this), nSide);
if (wallP->nType == WALL_BLASTABLE)
	DestroyWall (nSide);
if ((wallP->nType == WALL_DOOR) && (wallP->state == WALL_DOOR_CLOSED))
	OpenDoor (nSide);
}

//-----------------------------------------------------------------
// Determines what happens when a CWall is shot
//returns info about CWall.  see wall[HA] for codes
//obj is the CObject that hit...either a weapon or the player himself
//nPlayer is the number the player who hit the CWall or fired the weapon,
//or -1 if a robot fired the weapon

int32_t CSegment::ProcessWallHit (int32_t nSide, fix damage, int32_t nPlayer, CObject *objP)
{
	CWall* wallP = Wall (nSide);

if (!wallP)
	return WHP_NOT_SPECIAL;

if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordWallHitProcess (SEG_IDX (this), nSide, damage, nPlayer);

if (wallP->nType == WALL_BLASTABLE) {
	if (objP->cType.laserInfo.parent.nType == OBJ_PLAYER)
		DamageWall (nSide, damage);
	return WHP_BLASTABLE;
	}

if (nPlayer != N_LOCALPLAYER)	//return if was robot fire
	return WHP_NOT_SPECIAL;

Assert(nPlayer > -1);

//	Determine whether player is moving forward.  If not, don't say negative
//	messages because he probably didn't intentionally hit the door.
return wallP->ProcessHit (nPlayer, objP);
}

//-----------------------------------------------------------------
// Checks for a CTrigger whenever an CObject hits a CTrigger nSide.
void CSegment::OperateTrigger (int32_t nSide, CObject *objP, int32_t bShot)
{
CTrigger* trigP = Trigger (nSide);

if (!trigP)
	return;

if (objP
	 ? trigP->Operate (objP->Index (), objP->IsPlayer () ? objP->info.nId : objP->IsGuideBot () ? N_LOCALPLAYER : -1, bShot, 0)
	 : trigP->Operate (-1, -1, bShot, 0)
	)
	return;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordTrigger (Index (), nSide, objP->Index (), bShot);
if (IsMultiGame && !trigP->ClientOnly ())
	MultiSendTrigger (Index (), objP->Index ());
}

//	-----------------------------------------------------------------------------
//if an effect is hit, and it can blow up, then blow it up
//returns true if it blew up
int32_t CSegment::TextureIsDestructable (int32_t nSide, tDestructableTextureProps* dtpP)
{
	tDestructableTextureProps	dtp;

if (!dtpP)
	dtpP = &dtp;
dtpP->nOvlTex = m_sides [nSide].m_nOvlTex;
if (!dtpP->nOvlTex)
	return 0;
dtpP->nOvlOrient = m_sides [nSide].m_nOvlOrient;	//nOvlTex flags
dtpP->nEffect = gameData.pig.tex.tMapInfoP [dtpP->nOvlTex].nEffectClip;
if (dtpP->nEffect < 0) {
	if (IsMultiGame && netGameInfo.m_info.bIndestructibleLights)
		return 0;
	dtpP->nBitmap = gameData.pig.tex.tMapInfoP [dtpP->nOvlTex].destroyed;
	if (dtpP->nBitmap < 0)
		return 0;
	dtpP->nSwitchType = 0;
	}
else {
	tEffectInfo* effectInfoP = gameData.effects.effectP + dtpP->nEffect;
	if (effectInfoP->flags & EF_ONE_SHOT)
		return 0;
	dtpP->nBitmap = effectInfoP->destroyed.nTexture;
	if (dtpP->nBitmap < 0)
		return 0;
	dtpP->nSwitchType = 1;
	}
return 1;
}

//	-----------------------------------------------------------------------------
//if an effect is hit, and it can blow up, then blow it up
//returns true if it blew up

int32_t CSegment::BlowupTexture (int32_t nSide, CFixVector& vHit, CObject* blowerP, int32_t bForceBlowup)
{
	tDestructableTextureProps	dtp;
	int16_t			nSound, bPermaTrigger;
	uint8_t			vc;
	fix				u, v;
	fix				xDestroyedSize;
	tEffectInfo*	effectInfoP = NULL;
	CWall*			wallP = Wall (nSide);
	CTrigger*		trigP;
	CObject*			parentP = (!blowerP || (blowerP->cType.laserInfo.parent.nObject < 0)) ? NULL : OBJECT (blowerP->cType.laserInfo.parent.nObject);
	//	If this CWall has a CTrigger and the blowerP-upper is not the player or the buddy, abort!

if (parentP) {
	// only the player and the guidebot may blow a texture up if a trigger is attached to it
	if (parentP && (parentP->info.nType != OBJ_PLAYER) && ((parentP->info.nType != OBJ_ROBOT) || !parentP->IsGuideBot ()) && wallP && (wallP->nTrigger < gameData.trigData.m_nTriggers [0]))
		return 0;
	}

if (!TextureIsDestructable (nSide, &dtp))
	return 0;
//check if it's an animation (monitor) or casts light
LoadTexture (gameData.pig.tex.bmIndexP [dtp.nOvlTex].index, 0, gameStates.app.bD1Data);
//this can be blown up...did we hit it?
if (!bForceBlowup) {
	HitPointUV (nSide, &u, &v, NULL, vHit, 0);	//evil: always say face zero
	bForceBlowup = !PixelTranspType (dtp.nOvlTex, dtp.nOvlOrient, m_sides [nSide].m_nFrame, u, v);
	}
if (!bForceBlowup)
	return 0;

//note: this must get called before the texture changes,
//because we use the light value of the texture to change
//the static light in the CSegment
bPermaTrigger = (trigP = Trigger (nSide)) && trigP->Flagged (TF_PERMANENT);
if (!bPermaTrigger)
	SubtractLight (Index (), nSide);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordEffectBlowup (Index (), nSide, vHit);
if (dtp.nSwitchType) {
	effectInfoP = gameData.effects.effectP + dtp.nEffect;
	xDestroyedSize = effectInfoP->destroyed.xSize;
	vc = effectInfoP->destroyed.nAnimation;
	}
else {
	xDestroyedSize = I2X (20);
	vc = 3;
	}

CreateExplosion (int16_t (Index ()), vHit, xDestroyedSize, vc);
if (dtp.nSwitchType) {
	if ((nSound = gameData.effects.vClipP [vc].nSound) != -1)
		audio.CreateSegmentSound (nSound, Index (), 0, vHit);
	if ((nSound = effectInfoP->nSound) != -1)		//kill sound
		audio.DestroySegmentSound (Index (), nSide, nSound);
	if (!bPermaTrigger && (effectInfoP->destroyed.nEffect != -1) && (gameData.effects.effectP [effectInfoP->destroyed.nEffect].nSegment == -1)) {
		tEffectInfo	*newEcP = gameData.effects.effectP + effectInfoP->destroyed.nEffect;
		int32_t nNewBm = newEcP->changing.nWallTexture;
		if (ChangeTextures (-1, nNewBm, nSide)) {
			newEcP->xTimeLeft = EffectFrameTime (newEcP);
			newEcP->nCurFrame = 0;
			newEcP->nSegment = Index ();
			newEcP->nSide = nSide;
			newEcP->flags |= EF_ONE_SHOT | effectInfoP->flags;
			newEcP->flags &= ~EF_INITIALIZED;
			newEcP->destroyed.nTexture = effectInfoP->destroyed.nTexture;

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
		m_sides [nSide].m_nOvlTex = gameData.pig.tex.tMapInfoP [dtp.nOvlTex].destroyed;
	//assume this is a light, and play light sound
	audio.CreateSegmentSound (SOUND_LIGHT_BLOWNUP, Index (), 0, vHit);
	}
return 1;		//blew up!
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
	CBitmap*		bmBot = (nBaseTex < 0) ? NULL : LoadFaceBitmap (nBaseTex, 0, 1);
	CBitmap*		bmTop = (nOvlTex <= 0) ? NULL : LoadFaceBitmap (nOvlTex, 0, 1);
	tSegFaces*	segFaceP = SEGFACES + Index ();
	CSegFace*	faceP = segFaceP->faceP;

for (int32_t i = segFaceP->nFaces; i; i--, faceP++) {
	if ((nSide < 0) || (faceP->m_info.nSide == nSide)) {
		if (bmBot) {
			faceP->bmBot = bmBot;
			faceP->m_info.nBaseTex = nBaseTex;
			}
		if (nOvlTex >= 0) {
			faceP->bmTop = bmTop;
			faceP->m_info.nOvlTex = nOvlTex;
			}
		}
	}
return bmBot ? bmBot : bmTop;
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

//------------------------------------------------------------------------------

CSegment* CSegFace::Segment (void) 
{ 
return SEGMENT (m_info.nSegment); 
}

//------------------------------------------------------------------------------

bool CSegment::IsSolid (int32_t nSide)
{
CSide* sideP = Side (nSide);
if (sideP->Shape () > SIDE_SHAPE_TRIANGLE)
	return false;
if (m_children [nSide] < 0)
	return true;
return sideP->IsWall () && sideP->Wall ()->IsSolid ();
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
