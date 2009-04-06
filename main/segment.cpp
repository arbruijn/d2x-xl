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
#include "text.h"

// Number of vertices in current mine (ie, gameData.segs.vertices, pointed to by Vp)
//	Translate table to get opposite CSide of a face on a CSegment.
char	sideOpposite [MAX_SIDES_PER_SEGMENT] = {WRIGHT, WBOTTOM, WLEFT, WTOP, WFRONT, WBACK};

//	Note, this MUST be the same as sideVertIndex, it is an int for speed reasons.
int sideVertIndex [MAX_SIDES_PER_SEGMENT][4] = {
		 {7,6,2,3},			// left
		 {0,4,7,3},			// top
		 {0,1,5,4},			// right
		 {2,6,5,1},			// bottom
		 {4,5,6,7},			// back
		 {3,2,1,0}			// front
	};

extern bool bNewFileFormat;

//------------------------------------------------------------------------------

int CSegment::Index (void)
{
return this - SEGMENTS;
}

//------------------------------------------------------------------------------

void CSegment::ReadType (CFile& cf, ubyte flags)
{
if (flags & (1 << MAX_SIDES_PER_SEGMENT)) {
	m_nType = cf.ReadByte ();
	m_nMatCen = cf.ReadByte ();
	m_value = char (cf.ReadShort ());
	}
else {
	m_nType = 0;
	m_nMatCen = -1;
	m_value = 0;
	}
}

//------------------------------------------------------------------------------

void CSegment::ReadVerts (CFile& cf)
{
for (int i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
	m_verts [i] = cf.ReadShort ();
}

//------------------------------------------------------------------------------

void CSegment::ReadChildren (CFile& cf, ubyte flags)
{
for (int i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
	m_children [i] = (flags & (1 << i)) ? cf.ReadShort () : -1;
}

//------------------------------------------------------------------------------

void CSegment::ReadExtras (CFile& cf)
{
m_nType = cf.ReadByte ();
m_nMatCen = cf.ReadByte ();
m_value = cf.ReadByte ();
m_flags = cf.ReadByte ();
m_xAvgSegLight = cf.ReadFix ();
}

//------------------------------------------------------------------------------

void CSegment::Read (CFile& cf)
{
#if DBG
if (Index () == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
if (gameStates.app.bD2XLevel) {
	m_owner = cf.ReadByte ();
	m_group = cf.ReadByte ();
	}
else {
	m_owner = -1;
	m_group = -1;
	}

ubyte flags = bNewFileFormat ? cf.ReadByte () : 0x7f;

if (gameData.segs.nLevelVersion == 5) { // d2 SHAREWARE level
	ReadType (cf, flags);
	ReadVerts (cf);
	ReadChildren (cf, flags);
	}
else {
	ReadChildren (cf, flags);
	ReadVerts (cf);
	if (gameData.segs.nLevelVersion <= 1) { // descent 1 level
		ReadType (cf, flags);
		}
	}
m_objects = -1;

if (gameData.segs.nLevelVersion <= 5) // descent 1 thru d2 SHAREWARE level
	m_xAvgSegLight	= fix (cf.ReadShort ()) << 4;

// Read the walls as a 6 byte array
flags = bNewFileFormat ? cf.ReadByte () : 0x3f;

int i;

for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
	m_sides [i].ReadWallNum (cf, (flags & (1 << i)) != 0);

ushort sideVerts [4];

for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
	::GetCorners (Index (), i, sideVerts);
	m_sides [i].Read (cf, sideVerts, m_children [i] == -1);
	}
}

//------------------------------------------------------------------------------
// reads a CSegment structure from a CFile

void CSegment::SaveState (CFile& cf)
{
for (int i = 0; i < 6; i++)
	m_sides [i].SaveState (cf);
}

//------------------------------------------------------------------------------
// reads a CSegment structure from a CFile

void CSegment::LoadState (CFile& cf)
{
for (int i = 0; i < 6; i++)
	m_sides [i].LoadState (cf);
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
ComputeSideRads ();
m_extents [0] = vMin;
m_extents [1] = vMax;
}

// -----------------------------------------------------------------------------
//	Given two segments, return the side index in the connecting segment which connects to the base segment

int CSegment::ConnectedSide (CSegment* other)
{
	short		nSegment = SEG_IDX (this);
	short*	childP = other->m_children;

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
	short			nSide, faceBit;
	CSegMasks	masks;

//check refPoint against each CSide of CSegment. return bitmask
masks.m_valid = 1;
for (nSide = 0, faceBit = 1; nSide < 6; nSide++)
	masks |= m_sides [nSide].Masks (refP, xRad, 1 << nSide, faceBit);
return masks;
}

// -------------------------------------------------------------------------------
//returns 3 different bitmasks with info telling if this sphere is in
//this CSegment.  See CSegMasks structure for info on fields
CSegMasks CSegment::SideMasks (int nSide, const CFixVector& refP, fix xRad, bool bCheckPoke)
{
	short faceBit = 1;

return m_sides [nSide].Masks (refP, xRad, 1, faceBit, bCheckPoke);
}

// -------------------------------------------------------------------------------
//	Make a just-modified CSegment valid.
//		check all sides to see how many faces they each should have (0, 1, 2)
//		create new vector normals
void CSegment::Setup (void)
{
for (int i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
#if DBG
	if ((SEG_IDX (this) == nDbgSeg) && ((nDbgSide < 0) || (i == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	m_sides [i].Setup (m_verts, sideVertIndex [i], m_children [i] < 0);
	}
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

int CSegment::HasOpenableDoor (void)
{
for (int i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
	if (m_sides [i].IsOpenableDoor ())
		return i;
return -1;
}
//-----------------------------------------------------------------

int CSegment::IsDoorWay (int nSide, CObject *objP)
{
	int	nChild = m_children [nSide];

if (nChild == -1)
	return WID_RENDER_FLAG;
if (nChild == -2)
	return WID_EXTERNAL_FLAG;

CWall* wallP = m_sides [nSide].Wall ();
CSegment* childP = SEGMENTS + nChild;

#if DBG
if (objP && (objP->Index () == nDbgObj))
	nDbgObj = nDbgObj;
if ((SEG_IDX (this) == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if ((objP == gameData.objs.consoleP) &&
	 gameData.objs.speedBoost [objP->Index ()].bBoosted &&
	 (m_nType == SEGMENT_IS_SPEEDBOOST) && (childP->m_nType != SEGMENT_IS_SPEEDBOOST) &&
	 (!wallP || (TRIGGERS [wallP->nTrigger].nType != TT_SPEEDBOOST)))
	return objP ? WID_RENDER_FLAG : wallP ? wallP->IsDoorWay (objP) : WID_RENDPAST_FLAG;

if ((childP->m_nType == SEGMENT_IS_BLOCKED) || (childP->m_nType == SEGMENT_IS_SKYBOX))
	return (objP && ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT)))
			 ? WID_RENDER_FLAG
			 : wallP ? wallP->IsDoorWay (objP) : WID_FLY_FLAG | WID_RENDPAST_FLAG;

if (!wallP)
	return WID_FLY_FLAG | WID_RENDPAST_FLAG;

return wallP->IsDoorWay (objP);
}

//------------------------------------------------------------------------------

bool CSegment::IsVertex (int nVertex)
{
for (int i = 0; i < 8; i++)
	if (nVertex == m_verts [i])
		return true;
return false;
}

//------------------------------------------------------------------------------

int CSegment::Physics (fix& xDamage)
{
if (m_nType == SEGMENT_IS_LAVA) {
	xDamage = gameData.pig.tex.tMapInfo [0][404].damage / 2;
	return 1;
	}
if (m_nType == SEGMENT_IS_WATER) {
	xDamage = 0;
	return 2;
	}
return 0;
}

//-----------------------------------------------------------------
//set the nBaseTex or nOvlTex field for a CWall/door
void CSegment::SetTexture (int nSide, CSegment *connSegP, short nConnSide, int nAnim, int nFrame)
{
	tWallClip*	animP = gameData.walls.animP + nAnim;
	short			nTexture = animP->frames [(animP->flags & WCF_ALTFMT) ? 0 : nFrame];
	CBitmap*		bmP;
	int			nFrames;

//if (gameData.demo.nState == ND_STATE_PLAYBACK)
//	return;
if (nConnSide < 0)
	connSegP = NULL;
if (animP->flags & WCF_ALTFMT) {
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordWallSetTMapNum1 (SEG_IDX (this), (ubyte) nSide, (short) (connSegP ? SEG_IDX (connSegP) : -1), (ubyte) nConnSide, nTexture, nAnim, nFrame);
	nFrames = animP->nFrameCount;
	bmP = SetupHiresAnim (reinterpret_cast<short*> (animP->frames), nFrames, -1, 1, 0, &nFrames);
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
		NDRecordWallSetTMapNum1 (SEG_IDX (this), (ubyte) nSide, (short) (connSegP ? SEG_IDX (connSegP) : -1), (ubyte) nConnSide, nTexture, nAnim, nFrame);
	}
else {
	m_sides [nSide].m_nOvlTex = nTexture;
	if (connSegP)
		connSegP->m_sides [nConnSide].m_nOvlTex = nTexture;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordWallSetTMapNum2 (SEG_IDX (this), (ubyte) nSide, (short) (connSegP ? SEG_IDX (connSegP) : -1), (ubyte) nConnSide, nTexture, nAnim, nFrame);
	}
m_sides [nSide].m_nFrame = -nFrame;
if (connSegP)
	connSegP->m_sides [nConnSide].m_nFrame = -nFrame;
}

//-----------------------------------------------------------------

int CSegment::PokesThrough (int nObject, int nSide)
{
	CObject *objP = OBJECTS + nObject;

return (objP->info.xSize && SideMasks (nSide, objP->info.position.vPos, objP->info.xSize, true).m_side);
}

//-----------------------------------------------------------------
//returns true of door in unobjstructed (& thus can close)
int CSegment::DoorIsBlocked (int nSide)
{
	short		nConnSide;
	CSegment *connSegP;
	short		nObject, nType;

connSegP = SEGMENTS + m_children [nSide];
nConnSide = ConnectedSide (connSegP);
Assert(nConnSide != -1);

//go through each CObject in each of two segments, and see if
//it pokes into the connecting segP

for (nObject = m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg) {
	nType = OBJECTS [nObject].info.nType;
	if ((nType == OBJ_WEAPON) || (nType == OBJ_FIREBALL) || (nType == OBJ_EXPLOSION) || (nType == OBJ_EFFECT))
		continue;
	if (PokesThrough (nObject, nSide) || connSegP->PokesThrough (nObject, nConnSide))
		return 1;	//not free
		}
return 0; 	//doorway is free!
}

// -------------------------------------------------------------------------------
//when the CWall has used all its hitpoints, this will destroy it
void CSegment::BlastWall (int nSide)
{
	short			nConnSide;
	CSegment*	connSegP;
	int			a, n;
	short			nConnWall;
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
	connSegP = SEGMENTS + m_children [nSide];
	nConnSide = ConnectedSide (connSegP);
	Assert (nConnSide != -1);
	nConnWall = connSegP->WallNum (nConnSide);
	KillStuckObjects (nConnWall);
	}
KillStuckObjects (WallNum (nSide));

//if this is an exploding wall, explode it
if ((gameData.walls.animP [wallP->nClip].flags & WCF_EXPLODES) && !(wallP->flags & WALL_BLASTED))
	ExplodeWall (Index (), nSide);
else {
	//if not exploding, set final frame, and make door passable
	a = wallP->nClip;
	n = AnimFrameCount (gameData.walls.animP + a);
	SetTexture (nSide, connSegP, nConnSide, a, n - 1);
	wallP->flags |= WALL_BLASTED;
	if (IS_WALL (nConnWall))
		WALLS [nConnWall].flags |= WALL_BLASTED;
	}
}

//-----------------------------------------------------------------
// Destroys a blastable CWall.
void CSegment::DestroyWall (int nSide)
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
void CSegment::DamageWall (int nSide, fix damage)
{
	int		a, i, n;
	short		nConnSide, nConnWall;
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
	connSegP = SEGMENTS + m_children [nSide];
	nConnSide = ConnectedSide (connSegP);
	Assert(nConnSide != -1);
	nConnWall = connSegP->WallNum (nConnSide);
	}
wallP->hps -= damage;
if (IS_WALL (nConnWall))
	WALLS [nConnWall].hps -= damage;
a = wallP->nClip;
n = AnimFrameCount (gameData.walls.animP + a);
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
void CSegment::OpenDoor (int nSide)
{
CWall* wallP;
if (!(wallP = Wall (nSide)))
	return;

CActiveDoor* doorP;
if (!(doorP = wallP->OpenDoor ()))
	return;

	short			nConnSide, nConnWall = NO_WALL;
	CSegment		*connSegP;
// So that door can't be shot while opening
if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_OPEN_SINGLE, this - SEGMENTS, nSide, WallNum (nSide));
	connSegP = NULL;
	nConnSide = -1;
	nConnWall = NO_WALL;
	}
else {
	connSegP = SEGMENTS + m_children [nSide];
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
	CWall *linkedWallP = WALLS + wallP->nLinkedWall;
	CSegment *linkedSegP = SEGMENTS + linkedWallP->nSegment;
	linkedWallP->state = WALL_DOOR_OPENING;
	connSegP = SEGMENTS + linkedSegP->m_children [linkedWallP->nSide];
	if (IS_WALL (nConnWall))
		WALLS [nConnWall].state = WALL_DOOR_OPENING;
	doorP->nPartCount = 2;
	doorP->nFrontWall [1] = wallP->nLinkedWall;
	doorP->nBackWall [1] = linkedSegP->ConnectedSide (connSegP);
	}
else
	doorP->nPartCount = 1;
if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	if (gameData.walls.animP [wallP->nClip].openSound > -1)
		CreateSound (gameData.walls.animP [wallP->nClip].openSound, nSide);
	}
}

//-----------------------------------------------------------------
// Closes a door
void CSegment::CloseDoor (int nSide)
{
	CWall*	wallP;

if (!(wallP = Wall (nSide)))
	return;
if ((wallP->state == WALL_DOOR_CLOSING) ||		//already closing
	 (wallP->state == WALL_DOOR_WAITING) ||		//open, waiting to close
	 (wallP->state == WALL_DOOR_CLOSED))			//closed
	return;
if (DoorIsBlocked (nSide))
	return;

	CActiveDoor*	doorP;

if (!(doorP = wallP->CloseDoor ()))
	return;

	CSegment*		connSegP;
	short				nConnSide, nConnWall;

connSegP = SEGMENTS + m_children [nSide];
nConnSide = ConnectedSide (connSegP);
nConnWall = connSegP->WallNum (nConnSide);
if (IS_WALL (nConnWall))
	WALLS [nConnWall].state = WALL_DOOR_CLOSING;
doorP->nFrontWall [0] = WallNum (nSide);
doorP->nBackWall [0] = nConnWall;
Assert(SEG_IDX (this) != -1);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordDoorOpening (SEG_IDX (this), nSide);
if (IS_WALL (wallP->nLinkedWall))
	Int3();		//don't think we ever used linked walls
else
	doorP->nPartCount = 1;
if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	if (gameData.walls.animP [wallP->nClip].openSound > -1)
		CreateSound (gameData.walls.animP [wallP->nClip].openSound, nSide);
	}
}

//-----------------------------------------------------------------

// start the transition from closed -> open CWall
void CSegment::StartCloak (int nSide)
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
	short				nConnSide;
	int				i;
	short				nConnWall;


connSegP = SEGMENTS + m_children [nSide];
nConnSide = ConnectedSide (connSegP);

if (!(cloakWallP = wallP->StartCloak ())) {
	wallP->nType = WALL_OPEN;
	if ((wallP = connSegP->Wall (nConnSide)))
		wallP->nType = WALL_OPEN;
	return;
	}

nConnWall = connSegP->WallNum (nConnSide);
if (IS_WALL (nConnWall))
	WALLS [nConnWall].state = WALL_DOOR_CLOAKING;
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
void CSegment::StartDecloak (int nSide)
{
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return;
	CWall				*wallP;

if (!(wallP = Wall (nSide)))
	return;
if (wallP->nType == WALL_CLOSED || wallP->state == WALL_DOOR_DECLOAKING)		//already closed or decloaking
	return;

	CCloakingWall	*cloakWallP;
	short				nConnSide;
	CSegment			*connSegP;
	int				i;

connSegP = SEGMENTS + m_children [nSide];
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

int CSegment::AnimateOpeningDoor (int nSide, fix xElapsedTime)
{
CWall	*wallP = Wall (nSide);
if (!wallP)
	return 3;
return wallP->AnimateOpeningDoor (xElapsedTime);
}

//-----------------------------------------------------------------

int CSegment::AnimateClosingDoor (int nSide, fix xElapsedTime)
{
	CWall	*wallP = Wall (nSide);

if (!wallP)
	return 3;
return wallP->AnimateClosingDoor (xElapsedTime);
}

//-----------------------------------------------------------------
// Turns on an illusionary CWall (This will be used primarily for
//  CWall switches or triggers that can turn on/off illusionary walls.)
void CSegment::IllusionOn (int nSide)
{
	CWall* wallP = Wall (nSide);

if (!wallP)
	return;

	CSegment*	connSegP;
	short			nConnSide;

wallP->flags &= ~WALL_ILLUSION_OFF;
if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_ILLUS_SINGLE, this - SEGMENTS, nSide);
	connSegP = NULL;
	nConnSide = -1;
	}
else {
	connSegP = SEGMENTS + m_children [nSide];
	nConnSide = ConnectedSide (connSegP);
	if ((wallP = connSegP->Wall (nConnSide)))
		wallP->flags &= ~WALL_ILLUSION_OFF;
	}
}

//-----------------------------------------------------------------
// Turns off an illusionary CWall (This will be used primarily for
//  CWall switches or triggers that can turn on/off illusionary walls.)
void CSegment::IllusionOff (int nSide)
{

	CWall*	wallP = Wall (nSide);

if (!wallP)
	return;

	CSegment *connSegP;
	short		nConnSide;

wallP->flags |= WALL_ILLUSION_OFF;
KillStuckObjects (wallP - WALLS);

if (m_children [nSide] < 0) {
	if (gameOpts->legacy.bWalls)
		Warning (TXT_ILLUS_SINGLE,	this - SEGMENTS, nSide);
	connSegP = NULL;
	nConnSide = -1;
	}
else {
	connSegP = SEGMENTS + m_children [nSide];
	nConnSide = ConnectedSide (connSegP);
	if ((wallP = connSegP->Wall (nSide))) {
		wallP->flags |= WALL_ILLUSION_OFF;
		KillStuckObjects (wallP - WALLS);
		}
	}
}

//-----------------------------------------------------------------
// Opens doors/destroys CWall/shuts off triggers.
void CSegment::ToggleWall (int nSide)
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
//obj is the CObject that hit...either a weapon or the CPlayerData himself
//nPlayer is the number the CPlayerData who hit the CWall or fired the weapon,
//or -1 if a robot fired the weapon

int CSegment::ProcessWallHit (int nSide, fix damage, int nPlayer, CObject *objP)
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

if (nPlayer != gameData.multiplayer.nLocalPlayer)	//return if was robot fire
	return WHP_NOT_SPECIAL;

Assert(nPlayer > -1);

//	Determine whether CPlayerData is moving forward.  If not, don't say negative
//	messages because he probably didn't intentionally hit the door.
return wallP->ProcessHit (nPlayer, objP);
}

//-----------------------------------------------------------------
// Checks for a CTrigger whenever an CObject hits a CTrigger nSide.
void CSegment::OperateTrigger (int nSide, CObject *objP, int bShot)
{
CTrigger* trigP = Trigger (nSide);

if (!trigP)
	return;

if (trigP->Operate (objP->Index (), (objP->info.nType == OBJ_PLAYER) ? objP->info.nId : -1, bShot, 0))
	return;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordTrigger (Index (), nSide, objP->Index (), bShot);
if (IsMultiGame)
	MultiSendTrigger (Index (), objP->Index ());
}

//	-----------------------------------------------------------------------------
//if an effect is hit, and it can blow up, then blow it up
//returns true if it blew up
int CSegment::CheckEffectBlowup (int nSide, CFixVector& vHit, CObject* blowerP, int bForceBlowup)
{
	int				tm, tmf, ec, nBitmap = 0;
	int				bOkToBlow = 0, nSwitchType = -1;
	short				nSound, bPermaTrigger;
	ubyte				vc;
	fix				u, v;
	fix				xDestSize;
	tEffectClip*	ecP = NULL;
	CBitmap*			bmP;
	CWall*			wallP;
	CTrigger*		trigP;
	CObject*			parentP = (!blowerP || (blowerP->cType.laserInfo.parent.nObject < 0)) ? NULL : OBJECTS + blowerP->cType.laserInfo.parent.nObject;
	//	If this CWall has a CTrigger and the blowerP-upper is not the CPlayerData or the buddy, abort!

if (parentP) {
	if ((parentP->info.nType == OBJ_ROBOT) && ROBOTINFO (parentP->info.nId).companion)
		bOkToBlow = 1;
	if (!(bOkToBlow || (parentP->info.nType == OBJ_PLAYER)) &&
		 ((wallP = Wall (nSide)) && (wallP->nTrigger < gameData.trigs.m_nTriggers)))
		return 0;
	}

if (!(tm = m_sides [nSide].m_nOvlTex))
	return 0;

tmf = m_sides [nSide].m_nOvlOrient;		//tm flags
ec = gameData.pig.tex.tMapInfoP [tm].nEffectClip;
if (ec < 0) {
	if (gameData.pig.tex.tMapInfoP [tm].destroyed == -1)
		return 0;
	nBitmap = -1;
	nSwitchType = 0;
	}
else {
	ecP = gameData.eff.effectP + ec;
	if (ecP->flags & EF_ONE_SHOT)
		return 0;
	nBitmap = ecP->nDestBm;
	if (nBitmap < 0)
		return 0;
	nSwitchType = 1;
	}
//check if it's an animation (monitor) or casts light
bmP = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [tm].index;
LoadBitmap (gameData.pig.tex.bmIndexP [tm].index, gameStates.app.bD1Data);
//this can be blown up...did we hit it?
if (!bForceBlowup) {
	HitPointUV (nSide, &u, &v, NULL, vHit, 0);	//evil: always say face zero
	bForceBlowup = !PixelTranspType (tm, tmf,  m_sides [nSide].m_nFrame, u, v);
	}
if (!bForceBlowup)
	return 0;

if (IsMultiGame && netGame.bIndestructibleLights && !nSwitchType)
	return 0;
//note: this must get called before the texture changes,
//because we use the light value of the texture to change
//the static light in the CSegment
wallP = Wall (nSide);
bPermaTrigger = (trigP = Trigger (nSide)) && (trigP->flags & TF_PERMANENT);
if (!bPermaTrigger)
	SubtractLight (Index (), nSide);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordEffectBlowup (Index (), nSide, vHit);
if (nSwitchType) {
	xDestSize = ecP->xDestSize;
	vc = ecP->nDestVClip;
	}
else {
	xDestSize = I2X (20);
	vc = 3;
	}
/*Object*/CreateExplosion (short (Index ()), vHit, xDestSize, vc);
if (nSwitchType) {
	if ((nSound = gameData.eff.vClipP [vc].nSound) != -1)
		audio.CreateSegmentSound (nSound, Index (), 0, vHit);
	if ((nSound = ecP->nSound) != -1)		//kill sound
		audio.DestroySegmentSound (Index (), nSide, nSound);
	if (!bPermaTrigger && (ecP->nDestEClip != -1) && (gameData.eff.effectP [ecP->nDestEClip].nSegment == -1)) {
		tEffectClip	*newEcP = gameData.eff.effectP + ecP->nDestEClip;
		int nNewBm = newEcP->changingWallTexture;
		if (ChangeTextures (-1, nNewBm)) {
			newEcP->xTimeLeft = EffectFrameTime (newEcP);
			newEcP->nCurFrame = 0;
			newEcP->nSegment = Index ();
			newEcP->nSide = nSide;
			newEcP->flags |= EF_ONE_SHOT | ecP->flags;
			newEcP->flags &= ~EF_INITIALIZED;
			newEcP->nDestBm = ecP->nDestBm;

			Assert ((nNewBm != 0) && (m_sides [nSide].m_nOvlTex != 0));
			m_sides [nSide].m_nOvlTex = nNewBm;		//replace with destoyed
			}
		}
	else {
		Assert ((nBitmap != 0) && (m_sides [nSide].m_nOvlTex != 0));
		if (!bPermaTrigger)
			m_sides [nSide].m_nOvlTex = nBitmap;		//replace with destoyed
		}
	}
else {
	if (!bPermaTrigger)
		m_sides [nSide].m_nOvlTex = gameData.pig.tex.tMapInfoP [tm].destroyed;
	//assume this is a light, and play light sound
	audio.CreateSegmentSound (SOUND_LIGHT_BLOWNUP, Index (), 0, vHit);
	}
return 1;		//blew up!
}

//------------------------------------------------------------------------------

int CSegment::TexturedSides (void)
{
	int nSides = 0;

for (int i = 0; i < 6; i++)
	if ((m_children [i] < 0) || m_sides [i].IsTextured ())
		nSides++;
return nSides;
}

//------------------------------------------------------------------------------

void CSegment::CreateSound (short nSound, int nSide)
{
audio.CreateSegmentSound (nSound, Index (), nSide, SideCenter (nSide));
}

//------------------------------------------------------------------------------

CBitmap* CSegment::ChangeTextures (short nBaseTex, short nOvlTex)
{
	CBitmap*		bmBot = (nBaseTex < 0) ? NULL : LoadFaceBitmap (nBaseTex, 0, 1);
	CBitmap*		bmTop = (nOvlTex <= 0) ? NULL : LoadFaceBitmap (nOvlTex, 0, 1);
	tSegFaces*	segFaceP = SEGFACES + Index ();
	CSegFace	*	faceP = segFaceP->faceP;

for (int i = segFaceP->nFaces; i; i--, faceP++) {
	if (bmBot) 
		faceP->bmBot = bmBot;
		faceP->nBaseTex = nBaseTex;
	if (nOvlTex >= 0) {
		faceP->bmTop = bmTop;
		faceP->nOvlTex = nOvlTex;
		}
	}
return bmBot ? bmBot : bmTop;
}

//------------------------------------------------------------------------------
//eof
