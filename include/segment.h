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

#ifndef _SEGMENT_H
#define _SEGMENT_H

#include "pstypes.h"
#include "fix.h"
#include "vecmat.h"
#include "3d.h"
//#include "inferno.h"
#include "cfile.h"
#include "gr.h"
#include "carray.h"

// Version 1 - Initial version
// Version 2 - Mike changed some shorts to bytes in segments, so incompatible!

#define SIDE_IS_QUAD    1   // render CSide as quadrilateral
#define SIDE_IS_TRI_02  2   // render CSide as two triangles, triangulated along edge from 0 to 2
#define SIDE_IS_TRI_13  3   // render CSide as two triangles, triangulated along edge from 1 to 3

// Set maximum values for tSegment and face data structures.
#define MAX_VERTICES_PER_SEGMENT    8
#define MAX_SIDES_PER_SEGMENT       6
#define MAX_VERTICES_PER_POLY       4
#define WLEFT                       0
#define WTOP                        1
#define WRIGHT                      2
#define WBOTTOM                     3
#define WBACK                       4
#define WFRONT                      5

#define MAX_SEGMENTS_D1					800
#define MAX_SEGMENTS_D2					900
#define MAX_SEGMENTS_D2X				6000
#define MAX_SEGMENTS						gameData.segs.nMaxSegments
#define MAX_SEGMENT_VERTICES			65536 
#define MAX_SEGMENT_VERTICES_D2X		65536

#define MAX_SIDES			(MAX_SEGMENTS * 6)
#define MAX_FACES			(MAX_SIDES * 2)
#define MAX_TRIANGLES	(MAX_FACES * 16)

//normal everyday vertices

#define DEFAULT_LIGHTING        0   // (F1_0/2)

#ifdef EDITOR   //verts for the new tSegment
# define NUM_NEW_SEG_VERTICES   8
# define NEW_SEGMENT_VERTICES   (MAX_SEGMENT_VERTICES)
# define MAX_VERTICES           (MAX_SEGMENT_VERTICES + NUM_NEW_SEG_VERTICES)
# define MAX_VERTICES_D2X       (MAX_SEGMENT_VERTICES_D2X + NUM_NEW_SEG_VERTICES)
#else           //No editor
# define MAX_VERTICES           (MAX_SEGMENT_VERTICES)
# define MAX_VERTICES_D2X       (MAX_SEGMENT_VERTICES_D2X)
#endif

// Returns true if nSegment references a child, else returns false.
// Note that -1 means no connection, -2 means a connection to the outside world.
#define IS_CHILD(nSegment) (nSegment > -1)


extern int sideVertIndex [MAX_SIDES_PER_SEGMENT][4];
extern char sideOpposite [MAX_SIDES_PER_SEGMENT];

//------------------------------------------------------------------------------

class CWall;

class CSide {
	public:
		sbyte   		m_nType;       // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
		sbyte   		m_nFrame;      //keep us longword aligned
		ushort  		m_nWall;
		short   		m_nBaseTex;
#ifdef WORDS_BIGENDIAN
		ushort		m_nOvlOrient : 2;
		ushort		m_nOvlTex : 14;
#else
		ushort		m_nOvlTex : 14;
		ushort		m_nOvlOrient : 2;
#endif
		tUVL     	m_uvls [4];
		CFixVector	m_normals [2];  // 2 normals, if quadrilateral, both the same.

	public:
		void LoadTextures (void);
		inline ushort WallNum (void) { return m_nWall; }
		inline CWall* Wall (void);
		int FaceCount (void);

		int CheckTransparency (void);
		int SpecialCheckLineToFace (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, short iFace, CFixVector vNormal);
		int CheckLineToFace (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, short iFace, CFixVector vNormal);
		int CheckSphereToFace (CFixVector& intersection, short iFace, fix rad, CFixVector vNormal);
		uint CheckPointToFace (CFixVector& intersection, short iFace, CFixVector vNormal);

		void GetNormals (CFixVector& n1, CFixVector& n2);
	};

//------------------------------------------------------------------------------

class CBaseSegment {
	public:
		CSide   m_sides [MAX_SIDES_PER_SEGMENT];       // 6 sides
		ushort  m_children [MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
		ushort  m_verts [MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
		int     m_objects;    // pointer to objects in this tSegment
	};

//------------------------------------------------------------------------------

class CExtSide {
	public:
		CFixVector	m_rotNorms [2];
		CFixVector	m_vCenter;
		fix			m_rads [2];
		ushort		m_vertices [6];
		ubyte			m_nFaces;

	public:
		void ComputeCenter (void);
		void ComputeRads (void);
		CFixVector* Center (void) { return &m_vCenter; }
		int CreateVertexList (ushort* verts, int* index);
		inline ubyte GetVertices (ushort*& vertices) { 
			vertices = m_vertices;
			return m_nFaces;
			}
	CFixVector* GetVertices (CFixVector* vertices);
	ubyte Dist (const CFixVector& intersection, fix& xSideDist, int bBehind, short sideBit);
	};

//------------------------------------------------------------------------------

class CExtSegment {
	public:
		ubyte			m_special;
		sbyte			m_nMatCen;
		sbyte			m_value;
		ubyte			m_flags;
		fix			m_xAvgSegLight;
		CExtSide		m_sides [MAX_SIDES_PER_SEGMENT];

	public:
		void Read (CFile& cf);
		inline CExtSide* ExtSide (int nSide) { return m_sides + nSide; }
	};

//------------------------------------------------------------------------------

class CSegment : public CBaseSegment, public CExtSegment {
	public:
		char			m_owner;		  // team owning that tSegment (-1: always neutral, 0: neutral, 1: blue team, 2: red team)
		char			m_group;
		CFixVector	m_vCenter;
		fix			m_rads [2];
		CFixVector	m_extents [2];

	public:
		void LoadTextures (void);
		inline ushort WallNum (short nSide) { return CBaseSegment::m_sides [nSide].WallNum (); }
		inline int CheckTransparency (short nSide) { return CBaseSegment::m_sides [nSide].CheckTransparency (); }
		fix Refuel (fix xMaxFuel);
		fix Repair (fix xMaxShields);
		fix Damage (fix xMaxDamage);
		void CheckForGoal (void);
		void CheckForHoardGoal (void);
		int CheckFlagDrop (int nTeamId, int nFlagId, int nGoalId);
		int ConquerCheck (void);
		void CreateGenerator (int nType);
		void CreateEquipGen (int oldType);
		void CreateBotGen (int oldType);
		void ComputeCenter (void);
		void ComputeRads (fix xMinDist);
		inline void ComputeSideCenter (short nSide) { CExtSegment::m_sides [nSide].ComputeCenter (); }
		inline CSide* Side (int nSide) { return CBaseSegment::m_sides + nSide; }
		void ComputeSideRads (void);
		void GetNormals (short nSide, CFixVector& n1, CFixVector& n2) { CBaseSegment::m_sides [nSide].GetNormals (n1, n2); }
		inline CFixVector* Center (void) { return &m_vCenter; }
		inline int CreateVertexList (int nSide) { return CExtSegment::m_sides [nSide].CreateVertexList (m_verts, sideVertIndex [nSide]); }
		inline ubyte GetVertices (int nSide, ushort*& vertices) { return CExtSegment::m_sides [nSide].GetVertices (vertices); }
		inline CFixVector* GetVertices (int nSide, CFixVector* vertices) { return CExtSegment::m_sides [nSide].GetVertices (vertices); }
		ubyte SideDists (const CFixVector& intersection, fix* xSideDists, int bBehind = 1);
		int FindConnectedSide (CSegment* other);
		inline CFixVector& Normal (int nSide, int nFace);
#if 0
		inline uint CheckPointToFace (CFixVector& intersection, short nSide, short iFace)
			{ return CBaseSegment::m_sides [nSide].CheckPointToFace (intersection, iFace, nVerts, Normal (nSide, iFace)); }
		inline int CheckSphereToFace (CFixVector& intersection, short nSide, short iFace, fix rad)
			{ return CBaseSegment::m_sides [nSide].CheckSphereToFace (intersection, iFace, nVerts, Normal (nSide, iFace)); }
#endif
		inline int CheckLineToFace (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, short nSide, short iFace)
			{ return CBaseSegment::m_sides [nSide].CheckLineToFace (intersection, p0, p1, rad, iFace, Normal (nSide, iFace)); }
		inline int SpecialCheckLineToFace (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, short nSide, int iFace)
			{ return CBaseSegment::m_sides [nSide].SpecialCheckLineToFace (intersection, p0, p1, rad, iFace, Normal (nSide, iFace)); }

		inline int FaceCount (int nSide) { return CBaseSegment::m_sides [nSide].FaceCount (); }
	};

inline int operator- (CSegment* s, CArray<CSegment>& a) { return a.Index (s); }

//------------------------------------------------------------------------------

typedef struct tSegFaces {
	tFace*	pFaces;
	ubyte		nFaces;
	ubyte		bVisible;
} tSegFaces;

#define S2F_AMBIENT_WATER   0x01
#define S2F_AMBIENT_LAVA    0x02

//------------------------------------------------------------------------------

//values for special field
#define SEGMENT_IS_NOTHING			0
#define SEGMENT_IS_FUELCEN			1
#define SEGMENT_IS_REPAIRCEN		2
#define SEGMENT_IS_CONTROLCEN		3
#define SEGMENT_IS_ROBOTMAKER		4
#define SEGMENT_IS_GOAL_BLUE		5
#define SEGMENT_IS_GOAL_RED		6
#define SEGMENT_IS_WATER			7
#define SEGMENT_IS_LAVA				8
#define SEGMENT_IS_TEAM_BLUE		9
#define SEGMENT_IS_TEAM_RED		10
#define SEGMENT_IS_SPEEDBOOST		11
#define SEGMENT_IS_BLOCKED			12
#define SEGMENT_IS_NODAMAGE		13
#define SEGMENT_IS_SKYBOX			14
#define SEGMENT_IS_EQUIPMAKER		15
#define SEGMENT_IS_OUTDOOR			16
#define MAX_CENTER_TYPES			17

// Local tSegment data.
// This is stuff specific to a tSegment that does not need to get
// written to disk.  This is a handy separation because we can add to
// this structure without obsoleting existing data on disk.

#define SS_REPAIR_CENTER    0x01    // Bitmask for this tSegment being part of repair center.

//--repair-- typedef struct {
//--repair-- 	int     specialType;
//--repair-- 	short   special_segment; // if specialType indicates repair center, this is the base of the repair center
//--repair-- } lsegment;

#ifdef EDITOR

typedef struct {
	int     num_segments;
	int     num_vertices;
	short   segments [MAX_SEGMENTS];
	short   vertices [MAX_VERTICES];
} group;

#endif

// Globals from mglobal.c
extern int	sideVertIndex [MAX_SIDES_PER_SEGMENT][4];       // sideVertIndex[my_side] is list of vertices forming CSide my_side.
extern char sideOpposite [];                                // sideOpposite[my_side] returns CSide opposite cube from my_side.

// New stuff, 10/14/95: For shooting out lights and monitors.
// Light cast upon vertLight vertices in nSegment:nSide by some light
typedef struct {
	short   nSegment;
	sbyte   nSide;
	sbyte   bValid;
	ubyte   vertLight [4];
} tLightDelta;

// Light at nSegment:nSide casts light on count sides beginning at index (in array gameData.render.lightDeltas)
typedef struct {
	short   nSegment;
	ubyte   nSide;
	ubyte   count;
	ushort   index;
} tDlIndexD2;

typedef struct {
	short   nSegment;
	ushort nSide :3;
	ushort count :13;
	ushort index;
} tDlIndexD2X;

typedef union {
	tDlIndexD2X		d2x;
	tDlIndexD2		d2;
} tLightDeltaIndex;

#define MAX_DL_INDICES_D2    500
#define MAX_DELTA_LIGHTS_D2  10000

#define MAX_DL_INDICES       (MAX_SEGMENTS / 2)
#define MAX_DELTA_LIGHTS     (MAX_SEGMENTS * 10)

#define DL_SCALE            2048    // Divide light to allow 3 bits integer, 5 bits fraction.

int SubtractLight (short nSegment, short nSide);
int AddLight (short nSegment, short nSide);
void ClearLightSubtracted (void);

// ----------------------------------------------------------------------------
// --------------------- Segment interrogation functions ----------------------
// Do NOT read the tSegment data structure directly.  Use these
// functions instead.  The tSegment data structure is GUARANTEED to
// change MANY TIMES.  If you read the tSegment data structure
// directly, your code will break, I PROMISE IT!

#ifdef EDITOR

// Return a pointer to the list of vertex indices for the current
// tSegment in vp and the number of vertices in *nv.
extern void med_get_vertex_list(tSegment *s,int *nv,short **vp);

// Return a pointer to the list of vertex indices for face facenum in
// vp and the number of vertices in *nv.
extern void med_get_face_vertex_list(tSegment *s,int CSide, int facenum,int *nv,short **vp);

// Set *nf = number of faces in tSegment s.
extern void med_get_num_faces(tSegment *s,int *nf);

void med_validate_segment_side(tSegment *sp, short CSide);

// Delete tSegment function added for curves.c
extern int med_delete_segment(tSegment *sp);

// Delete tSegment from group
extern void DeleteSegmentFromGroup (int nSegment, int nGroup);

// Add tSegment to group
extern void AddSegmentToGroup (int nSegment, int nGroup);

#endif

// Verify that all vertices are legal.
extern void med_check_all_vertices();

/*
 * reads a tLightDelta structure from a CFILE
 */
void ReadlightDelta(tLightDelta *dl, CFile& cf);

/*
 * reads a tLightDeltaIndex structure from a CFILE
 */
void ReadlightDeltaIndex(tLightDeltaIndex *di, CFile& cf);

void FreeSkyBoxSegList (void);
int BuildSkyBoxSegList (void);

#endif
