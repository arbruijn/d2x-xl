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
//#include "descent.h"
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
#define MAX_SEGMENT_VERTICES			(8 * MAX_SEGMENTS) 
#define MAX_SEGMENT_VERTICES_D2X		(8 * MAX_SEGMENTS_D2X)

#define MAX_SIDES			(MAX_SEGMENTS * 6)
#define MAX_FACES			(MAX_SIDES * 2)
#define MAX_TRIANGLES	(MAX_FACES * 16)

//normal everyday vertices

#define DEFAULT_LIGHTING        0   // (I2X (1)/2)

# define MAX_VERTICES           (MAX_SEGMENT_VERTICES)
# define MAX_VERTICES_D2X       (MAX_SEGMENT_VERTICES_D2X)

// Returns true if nSegment references a child, else returns false.
// Note that -1 means no connection, -2 means a connection to the outside world.
#define IS_CHILD(nSegment) (nSegment > -1)

extern int sideVertIndex [MAX_SIDES_PER_SEGMENT][4];
extern char sideOpposite [MAX_SIDES_PER_SEGMENT];

//------------------------------------------------------------------------------

class CSegMasks {
	public:
	   short m_face;     //which faces sphere pokes through (12 bits)
		sbyte m_side;     //which sides sphere pokes through (6 bits)
		sbyte m_center;   //which sides center point is on back of (6 bits)
		sbyte m_valid;

	public:
		CSegMasks () { Init (); }
		void Init (void) {
			m_face = 0;
			m_side = m_center = 0;
			m_valid = 0;
			}

		inline CSegMasks& operator|= (CSegMasks other) {
			if (other.m_valid) {
				m_center |= other.m_center;
				m_face |= other.m_face;
				m_side |= other.m_side;
				}
			return *this;
			}
	};

//------------------------------------------------------------------------------

class CTrigger;
class CWall;
class CObject;

class CSide {
	public:
		sbyte   			m_nType;       // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
		sbyte   			m_nFrame;      //keep us longword aligned
		ushort  			m_nWall;
		short   			m_nBaseTex;
#ifdef WORDS_BIGENDIAN
		ushort			m_nOvlOrient : 2;
		ushort			m_nOvlTex : 14;
#else
		ushort			m_nOvlTex : 14;
		ushort			m_nOvlOrient : 2;
#endif
		tUVL     		m_uvls [4];
		CFixVector		m_normals [2];  // 2 normals, if quadrilateral, both the same.
		CFloatVector	m_fNormals [2];
		CFixVector		m_rotNorms [2];
		CFixVector		m_vCenter;
		fix				m_rads [2];
		short				m_vertices [6];
		short				m_faceVerts [6];
		short				m_corners [4];
		short				m_nMinVertex [2];
		ubyte				m_nFaces;

	public:
		inline void Init (void) { memset (this, 0, sizeof (*this)); }
		void Read (CFile& cf, ushort* sideVerts, bool bSolid);
		void ReadWallNum (CFile& cf, bool bWall);
		void SaveState (CFile& cf);
		void LoadState (CFile& cf);
		void LoadTextures (void);
		inline ushort WallNum (void) { return m_nWall; }
		inline sbyte Type (void) { return m_nType; }
		inline void SetType (sbyte nType) { m_nType = nType; }
		bool IsWall (void);
		CWall* Wall (void);
		CTrigger* Trigger (void);
		bool IsVolatile (void);
		int FaceCount (void);
		void CheckSum (uint& sum1, uint& sum2);

		int CheckTransparency (void);
		int SpecialCheckLineToFace (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, short iFace, CFixVector vNormal);
		int CheckLineToFace (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, short iFace, CFixVector vNormal);
		int CheckSphereToFace (CFixVector& intersection, fix rad, short iFace, CFixVector vNormal);
		uint CheckPointToFace (CFixVector& intersection, short iFace, CFixVector vNormal);

		void GetNormals (CFixVector& n1, CFixVector& n2);

		void ComputeCenter (void);
		void ComputeRads (void);
		CFixVector& Center (void) { return m_vCenter; }
		fix MinRad (void) { return m_rads [0]; }
		fix MaxRad (void) { return m_rads [1]; }

		void Setup (short* verts, int* index, bool bSolid);

		void SetTextures (int nBaseTex, int nOvlTex);

		inline ubyte GetVertices (short*& vertices) { 
			vertices = m_vertices;
			return m_nFaces;
			}
		CFixVector* GetCorners (CFixVector* vertices);
		inline short* Corners (void) { return m_corners; }
		CFixVector& Vertex (int nVertex);
		CFixVector& MinVertex (void);
		CFixVector& Normal (int nFace);
		fix Height (void);
		bool IsPlanar (void);
		ubyte Dist (const CFixVector& point, fix& xSideDist, int bBehind, short sideBit);
		CSegMasks Masks (const CFixVector& refP, fix xRad, short sideBit, short& faceBit, bool bCheckPoke = false);
		void HitPointUV (fix *u, fix *v, fix *l, CFixVector& intersection, int iFace);
		int CheckForTranspPixel (CFixVector& intersection, short iFace);
		int Physics (fix& damage, bool bSolid);

		bool IsOpenableDoor (void);
		bool IsTextured (void);

	private:
		void SetupCorners (short* verts, int* index);
		void SetupVertexList (short* verts, int* index);
		void SetupFaceVertIndex (void);
		void SetupAsQuad (CFixVector& vNormal, CFloatVector& vNormalf, short* verts, int* index);
		void SetupAsTriangles (bool bSolid, short* verts, int* index);
	};

//------------------------------------------------------------------------------

class CSegment {
	public:
		CSide			m_sides [MAX_SIDES_PER_SEGMENT];       // 6 sides
		short			m_children [MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
		short			m_verts [MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
		int			m_objects;    // pointer to objects in this tSegment

		ubyte			m_nType;
		sbyte			m_nMatCen;
		sbyte			m_value;
		ubyte			m_flags;
		fix			m_xAvgSegLight;

		char			m_owner;		  // team owning that tSegment (-1: always neutral, 0: neutral, 1: blue team, 2: red team)
		char			m_group;
		CFixVector	m_vCenter;
		fix			m_rads [2];
		CFixVector	m_extents [2];

	public:
		int Index (void);
		void Read (CFile& cf);
		void ReadType (CFile& cf, ubyte flags);
		void ReadVerts (CFile& cf);
		void ReadChildren (CFile& cf, ubyte flags);
		void ReadExtras (CFile& cf);
		void SaveState (CFile& cf);
		void LoadState (CFile& cf);
		void LoadTextures (void);
		void LoadSideTextures (int nSide);
		void LoadBotGenTextures (void);
		void Setup (void);
		inline ushort WallNum (int nSide) { return m_sides [nSide].WallNum (); }
		inline int CheckTransparency (int nSide) { return m_sides [nSide].CheckTransparency (); }
		void CheckSum (uint& sum1, uint& sum2);

		inline bool IsWall (int nSide) { return m_sides [nSide].IsWall (); }
		void SetTexture (int nSide, CSegment *connSegP, short nConnSide, int nAnim, int nFrame);
		void DestroyWall (int nSide);
		void DamageWall (int nSide, fix damage);
		void BlastWall (int nSide);
		void OpenDoor (int nSide);
		void CloseDoor (int nSide, bool bForce = false);
		void StartCloak (int nSide);
		void StartDecloak (int nSide);
		void IllusionOff (int nSide);
		void IllusionOn (int nSide);
		int AnimateOpeningDoor (int nSide, fix xElapsedTime);
		int AnimateClosingDoor (int nSide, fix xElapsedTime);
		void ToggleWall (int nSide);
		int ProcessWallHit (int nSide, fix damage, int nPlayer, CObject *objP);
		int DoorIsBlocked (int nSide, bool bIgnoreMarker = false);
		int CheckEffectBlowup (int nSide, CFixVector& vHit, CObject* blower, int bForceBlowup);
		void CreateSound (short nSound, int nSide);

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
		void CreateFuelCen (int oldType);

		void ComputeCenter (void);
		void ComputeRads (fix xMinDist);
		inline void ComputeSideCenter (int nSide) { m_sides [nSide].ComputeCenter (); }
		inline CSide* Side (int nSide) { return m_sides + nSide; }
		inline CWall* Wall (int nSide) { return m_sides [nSide].Wall (); }
		inline CTrigger* Trigger (int nSide) { return m_sides [nSide].Trigger (); }
		inline sbyte Type (int nSide) { return m_sides [nSide].m_nType; }
		inline void SetType (int nSide, sbyte nType) { m_sides [nSide].SetType (nType); }
		inline void SetTextures (int nSide, int nBaseTex, int nOvlTex) { m_sides [nSide].SetTextures (nBaseTex, nOvlTex); }

		void ComputeSideRads (void);
		inline bool IsVertex (int nVertex);
		void GetNormals (int nSide, CFixVector& n1, CFixVector& n2) { m_sides [nSide].GetNormals (n1, n2); }
		inline CFixVector& Center (void) { return m_vCenter; }
		inline CFixVector& SideCenter (int nSide) { return m_sides [nSide].Center (); }
		inline short* Corners (int nSide) { return m_sides [nSide].Corners (); }
		inline CFixVector* GetCorners (int nSide, CFixVector* vertices) { return m_sides [nSide].GetCorners (vertices); }
		ubyte SideDists (const CFixVector& intersection, fix* xSideDists, int bBehind = 1);
		int ConnectedSide (CSegment* other);
		inline CFixVector& Normal (int nSide, int nFace) { return m_sides [nSide].Normal (nFace); }
#if 0
		inline uint CheckPointToFace (CFixVector& intersection, int nSide, short iFace)
		 { return m_sides [nSide].CheckPointToFace (intersection, iFace, Normal (nSide, iFace)); }
#endif
		inline int CheckSphereToFace (CFixVector& intersection, fix rad, int nSide, short iFace)
		 { return m_sides [nSide].CheckSphereToFace (intersection, rad, iFace, Normal (nSide, iFace)); }
		inline int CheckLineToFace (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, int nSide, short iFace)
		 { return m_sides [nSide].CheckLineToFace (intersection, p0, p1, rad, iFace, Normal (nSide, iFace)); }
		inline int SpecialCheckLineToFace (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, int nSide, int iFace)
		 { return m_sides [nSide].SpecialCheckLineToFace (intersection, p0, p1, rad, iFace, Normal (nSide, iFace)); }

		inline int FaceCount (int nSide) { return m_sides [nSide].FaceCount (); }
		CSegMasks Masks (const CFixVector& refP, fix xRad);
		CSegMasks SideMasks (int nSide, const CFixVector& refP, fix xRad, bool bCheckPoke = false);
		ubyte GetSideDists (const CFixVector& refP, fix* xSideDists, int bBehind);
		void HitPointUV (int nSide, fix *u, fix *v, fix *l, CFixVector& intersection, int iFace)
		 { m_sides [nSide].HitPointUV (u, v, l, intersection, iFace); }

		fix MinRad (void) { return m_rads [0]; }
		fix MaxRad (void) { return m_rads [1]; }
		float MinRadf (void) { return X2F (m_rads [0]); }
		float MaxRadf (void) { return X2F (m_rads [1]); }

		inline fix AvgRad (void) {return (m_rads [0] + m_rads [1]) / 2;}
		inline float AvgRadf (void) {return X2F (m_rads [0] + m_rads [1]) / 2;}
		inline fix Volume (void) {return (fix) (1.25 * Pi * pow (AvgRadf (), 3) + 0.5);}

		CFixVector RandomPoint (void);

		int IsDoorWay (int nSide, CObject* objP, bool bIgnoreDoors = false);
		int HasOpenableDoor (void);

		inline int CheckForTranspPixel (CFixVector& intersection, int nSide, short iFace) 
		 { return m_sides [nSide].CheckForTranspPixel (intersection, iFace); }

		int Physics (int nSide, fix& damage) { return m_sides [nSide].Physics (damage, m_children [nSide] == -1); }
		int Physics (fix& xDamage);

		int TexturedSides (void);
		CBitmap* ChangeTextures (short nBaseTex, short nOvlTex);

		void OperateTrigger (int nSide, CObject *objP, int bShot);

	private:
		inline int PokesThrough (int nObject, int nSide);

	};

inline int operator- (CSegment* s, CArray<CSegment>& a) { return a.Index (s); }

//------------------------------------------------------------------------------

typedef struct tSegFaces {
	CSegFace*	faceP;
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

// Globals from mglobal.c
extern int	sideVertIndex [MAX_SIDES_PER_SEGMENT][4];       // sideVertIndex[my_side] is list of vertices forming CSide my_side.
extern char sideOpposite [];                                // sideOpposite [my_side] returns CSide opposite cube from my_side.

// New stuff, 10/14/95: For shooting out lights and monitors.
// Light cast upon vertLight vertices in nSegment:nSide by some light
class CLightDelta {
	public:
		short   nSegment;
		sbyte   nSide;
		sbyte   bValid;
		ubyte   vertLight [4];

	public:
		void Read (CFile& cf);
	};

class CLightDeltaIndex {
	public:
		short		nSegment;
		ushort	nSide :3;
		ushort	count :13;
		ushort	index;

	public:
		inline bool operator< (CLightDeltaIndex& other) {
			return (nSegment < other.nSegment) || ((nSegment == other.nSegment) && (nSide < other.nSide));
			}
		inline bool operator> (CLightDeltaIndex& other) {
			return (nSegment > other.nSegment) || ((nSegment == other.nSegment) && (nSide > other.nSide));
			}

		void Read (CFile& cf);
	};

#define MAX_DL_INDICES_D2    500
#define MAX_DELTA_LIGHTS_D2  10000

#define MAX_DL_INDICES       (MAX_SEGMENTS / 2)
#define MAX_DELTA_LIGHTS     (MAX_SEGMENTS * 10)

#define DL_SCALE            2048    // Divide light to allow 3 bits integer, 5 bits fraction.

int SubtractLight (short nSegment, int nSide);
int AddLight (short nSegment, int nSide);
void ClearLightSubtracted (void);

int CountSkyBoxSegments (void);
void FreeSkyBoxSegList (void);
int BuildSkyBoxSegList (void);

void GetCorners (int nSegment, int nSide, ushort* vertIndex);

// ----------------------------------------------------------------------------

#endif //_SEGMENT_H

