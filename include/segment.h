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

//------------------------------------------------------------------------------
// Version 1 - Initial version
// Version 2 - Mike changed some shorts to bytes in segments, so incompatible!

#define SIDE_IS_QUAD				1   // render CSide as quadrilateral
#define SIDE_IS_TRI_02			2   // render CSide as two triangles, triangulated along edge from 0 to 2
#define SIDE_IS_TRI_13			3   // render CSide as two triangles, triangulated along edge from 1 to 3

#define SEGMENT_SHAPE_CUBE    0
#define SEGMENT_SHAPE_WEDGE   1
#define SEGMENT_SHAPE_PYRAMID 2

#define SIDE_SHAPE_RECTANGLE  0
#define SIDE_SHAPE_TRIANGLE   1
#define SIDE_SHAPE_LINE       2
#define SIDE_SHAPE_POINT      3

//------------------------------------------------------------------------------
// Set maximum values for tSegment and face data structures.
#define SEGMENT_VERTEX_COUNT			8
#define SEGMENT_SIDE_COUNT				6
#define MAX_VERTICES_PER_POLY       4
#define WLEFT                       0
#define WTOP                        1
#define WRIGHT                      2
#define WBOTTOM                     3
#define WBACK                       4
#define WFRONT                      5

#define MAX_SEGMENTS_D1					800
#define MAX_SEGMENTS_D2					900
#define MAX_SEGMENTS_D2X				20000
#define MAX_SEGMENTS						gameData.segs.nMaxSegments
#define MAX_SEGMENT_VERTICES			(8 * MAX_SEGMENTS) 
#define MAX_SEGMENT_VERTICES_D2X		(3 * MAX_SEGMENTS_D2X)

#define MAX_SIDES			(MAX_SEGMENTS * 6)
#define MAX_FACES			(MAX_SIDES * 2)
#define MAX_TRIANGLES	(MAX_FACES * 16)

//------------------------------------------------------------------------------
//normal everyday vertices

#define DEFAULT_LIGHTING        0   // (I2X (1)/2)

# define MAX_VERTICES           (MAX_SEGMENT_VERTICES)
# define MAX_VERTICES_D2X       (MAX_SEGMENT_VERTICES_D2X)

//------------------------------------------------------------------------------
//values for function field
#define SEGMENT_TYPE_NONE				0
#define SEGMENT_TYPE_FUELCEN			1
#define SEGMENT_TYPE_REPAIRCEN		2
#define SEGMENT_TYPE_CONTROLCEN		3
#define SEGMENT_TYPE_ROBOTMAKER		4
#define SEGMENT_TYPE_GOAL_BLUE		5
#define SEGMENT_TYPE_GOAL_RED			6
#define SEGMENT_TYPE_WATER				7
#define SEGMENT_TYPE_LAVA				8
#define SEGMENT_TYPE_TEAM_BLUE		9
#define SEGMENT_TYPE_TEAM_RED			10
#define SEGMENT_TYPE_SPEEDBOOST		11
#define SEGMENT_TYPE_BLOCKED			12
#define SEGMENT_TYPE_NODAMAGE			13
#define SEGMENT_TYPE_SKYBOX			14
#define SEGMENT_TYPE_EQUIPMAKER		15
#define SEGMENT_TYPE_LIGHT_SELF		16
#define MAX_SEGMENT_TYPES				17

#define SEGMENT_FUNC_NONE				0
#define SEGMENT_FUNC_FUELCEN			1
#define SEGMENT_FUNC_REPAIRCEN		2
#define SEGMENT_FUNC_CONTROLCEN		3
#define SEGMENT_FUNC_ROBOTMAKER		4
#define SEGMENT_FUNC_VIRUSMAKER		SEGMENT_FUNC_ROBOTMAKER
#define SEGMENT_FUNC_GOAL_BLUE		5
#define SEGMENT_FUNC_GOAL_RED			6
#define SEGMENT_FUNC_TEAM_BLUE		7
#define SEGMENT_FUNC_TEAM_RED			8
#define SEGMENT_FUNC_SPEEDBOOST		9
#define SEGMENT_FUNC_SKYBOX			10
#define SEGMENT_FUNC_EQUIPMAKER		11
#define MAX_SEGMENT_FUNCTIONS			12

#define SEGMENT_PROP_NONE				0
#define SEGMENT_PROP_WATER				1
#define SEGMENT_PROP_LAVA				2
#define SEGMENT_PROP_BLOCKED			4
#define SEGMENT_PROP_NODAMAGE			8
#define SEGMENT_PROP_OUTDOORS			16

#define OBJ_IS_WEAPON					1
#define OBJ_IS_ENERGY_WEAPON			2
#define OBJ_IS_SPLASHDMG_WEAPON		4
#define OBJ_HAS_LIGHT_TRAIL			8
#define OBJ_IS_PLAYER_MINE				16
#define OBJ_IS_ROBOT_MINE				32
#define OBJ_IS_MISSILE					64
#define OBJ_IS_GATLING_ROUND			128
#define OBJ_BOUNCES						256

// Returns true if nSegment references a child, else returns false.
// Note that -1 means no connection, -2 means a connection to the outside world.
#define IS_CHILD(nSegment) (nSegment > -1)

extern ushort sideVertIndex [SEGMENT_SIDE_COUNT][4];
extern char oppSideTable [SEGMENT_SIDE_COUNT];

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

typedef struct tUVL {
	fix u,v,l;
} __pack__ tUVL;

//------------------------------------------------------------------------------

static inline int IsWaterTexture (short nTexture)
{
return ((nTexture >= 399) && (nTexture <= 403));
}

//------------------------------------------------------------------------------

static inline int IsLavaTexture (short nTexture)
{
return (nTexture == 378) || ((nTexture >= 404) && (nTexture <= 409));
}

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
		CFixVector		m_normals [3];  // 2 normals, if quadrilateral, both the same.
		CFloatVector	m_fNormals [3];
		CFixVector		m_rotNorms [2];
		CFixVector		m_vCenter;
		fix				m_rads [2];
		ushort			m_vertices [6];
		ushort			m_faceVerts [6]; // vertex indices of the side's two triangles
		ushort			m_nMinVertex [2];
		ushort			m_corners [4];
		ubyte				m_nFaces;
		ubyte				m_nShape;
		ubyte				m_nCorners;
		ubyte				m_bIsQuad;
		short				m_nSegment;

	public:
		inline void Init (void) { memset (this, 0, sizeof (*this)); }
		int Read (CFile& cf, ushort* sideVerts, bool bSolid);
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
		inline ubyte IsQuad (void) { return m_bIsQuad; }
		inline ubyte CornerCount (void) { return m_nCorners; }
		void CheckSum (uint& sum1, uint& sum2);

		int CheckTransparency (void);
		int CheckLineToFaceSpecial (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, short iFace, CFixVector vNormal);
		int CheckLineToFaceRegular (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, short iFace, CFixVector vNormal);
		int SphereToFaceRelation (CFixVector& intersection, fix rad, short iFace, CFixVector vNormal);
		uint PointToFaceRelation (CFixVector& intersection, short iFace, CFixVector vNormal);
		int SeesPoint (CFixVector& vPoint, short nDestSeg, int nLevel = 0, int nThread = 0);

		void GetNormals (CFixVector& n1, CFixVector& n2);

		void ComputeCenter (void);
		void ComputeRads (void);
		CFixVector& Center (void) { return m_vCenter; }
		fix MinRad (void) { return m_rads [0]; }
		fix MaxRad (void) { return m_rads [1]; }
		fix AvgRad (void) { return (m_rads [0] + m_rads [1]) / 2; }

		void Setup (short nSegment, ushort* verts, ushort* index, bool bSolid);

		void SetTextures (int nBaseTex, int nOvlTex);

		inline ubyte GetVertices (ushort*& vertices) { 
			vertices = m_vertices;
			return m_nFaces;
			}
		CFixVector* GetCornerVertices (CFixVector* vertices);
		inline ushort* Corners (void) { return m_corners; }
		inline int HasVertex (ushort nVertex) { 
			for (int i = 0; i < m_nCorners; i++)
				if (m_corners [i] == nVertex)
					return 1;
			return 0;
			}
		CFixVector& Vertex (int nVertex);
		CFixVector& MinVertex (void);
		CFixVector& Normal (int nFace);
		fix Height (void);
		bool IsPlanar (void);
		ubyte Dist (const CFixVector& point, fix& xSideDist, int bBehind, short sideBit);
		ubyte Distf (const CFloatVector& point, float& fSideDist, int bBehind, short sideBit);
		fix DistToPoint (CFixVector v);
		float DistToPointf (CFloatVector v);
		CSegMasks Masks (const CFixVector& refP, fix xRad, short sideBit, short& faceBit, bool bCheckPoke = false);
		void HitPointUV (fix *u, fix *v, fix *l, CFixVector& intersection, int iFace);
		int CheckForTranspPixel (CFixVector& intersection, short iFace);
		int Physics (fix& damage, bool bSolid);

		bool IsOpenableDoor (void);
		bool IsTextured (void);

		inline int IsLava (void) { return IsLavaTexture (m_nBaseTex) || (m_nOvlTex && (IsLavaTexture (m_nOvlTex))); }
		inline int IsWater (void) { return IsWaterTexture (m_nBaseTex) || (m_nOvlTex && (IsWaterTexture (m_nOvlTex))); }

		inline ubyte Shape (void) { return m_nShape; }
		inline ubyte FaceCount (void) { return m_nFaces; }

	private:
		void SetupCorners (ushort* verts, ushort* index);
		void SetupVertexList (ushort* verts, ushort* index);
		void SetupFaceVertIndex (void);
		void SetupAsQuad (CFixVector& vNormal, CFloatVector& vNormalf, ushort* verts, ushort* index);
		void SetupAsTriangles (bool bSolid, ushort* verts, ushort* index);
	};

//------------------------------------------------------------------------------

class CSegment {
	public:
		CSide			m_sides [SEGMENT_SIDE_COUNT];       // 6 sides
		short			m_children [SEGMENT_SIDE_COUNT];    // indices of 6 children segments, front, left, top, right, bottom, back
		fix			m_childDists [2][SEGMENT_SIDE_COUNT];
		ushort		m_vertices [SEGMENT_VERTEX_COUNT];    // vertex ids of 4 front and 4 back vertices
		short			m_nVertices;
		int			m_objects;    // pointer to objects in this tSegment

		ubyte			m_nShape;
		ubyte			m_function;
		ubyte			m_flags;
		ubyte			m_props;
		short			m_value;
		short			m_nMatCen;
		fix			m_xDamage [2];
		fix			m_xAvgSegLight;

		char			m_owner;		  // team owning that tSegment (-1: always neutral, 0: neutral, 1: blue team, 2: red team)
		char			m_group;
		CFixVector	m_vCenter;
		fix			m_rads [2];
		CFixVector	m_extents [2];

	public:
		int Index (void);
		void Read (CFile& cf);
		void ReadFunction (CFile& cf, ubyte flags);
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
		fix Repair (fix xMaxShield);
		fix ShieldDamage (fix xMaxDamage);
		fix EnergyDamage (fix xMaxDamage);
		void CheckForGoal (void);
		void CheckForHoardGoal (void);
		int CheckFlagDrop (int nTeamId, int nFlagId, int nGoalId);
		int ConquerCheck (void);

		void ChangeTexture (int oldOwner);
		void OverrideTextures (short nTexture, short nOldTexture, short nTexture2, int bFullBright, int bForce);

		bool CreateGenerator (int nType);
		bool CreateEquipGen (int nOldFunction);
		bool CreateBotGen (int nOldFunction);
		bool CreateFuelCen (int nOldFunction);
		bool CreateMatCen (int nOldFunction, int nMaxCount);

		void ComputeCenter (void);
		void ComputeRads (fix xMinDist);
		void ComputeChildDists (void);
		inline void ComputeSideCenter (int nSide) { m_sides [nSide].ComputeCenter (); }
		inline CSide* Side (int nSide) { return m_sides + nSide; }
		int ChildIndex (int nChild);
		CSide* AdjacentSide (int nSegment);
		CSide* OppositeSide (int nSide);
		inline CWall* Wall (int nSide) { return m_sides [nSide].Wall (); }
		inline CTrigger* Trigger (int nSide) { return m_sides [nSide].Trigger (); }
		inline sbyte Type (int nSide) { return m_sides [nSide].m_nType; }
		inline void SetType (int nSide, sbyte nType) { m_sides [nSide].SetType (nType); }
		inline void SetTextures (int nSide, int nBaseTex, int nOvlTex) { m_sides [nSide].SetTextures (nBaseTex, nOvlTex); }

		void GetCornerIndex (int nSide, ushort* vertIndex);
		void ComputeSideRads (void);
		float FaceSize (ubyte nSide);
		inline bool IsVertex (int nVertex);
		void GetNormals (int nSide, CFixVector& n1, CFixVector& n2) { m_sides [nSide].GetNormals (n1, n2); }
		inline CFixVector& Center (void) { return m_vCenter; }
		inline CFixVector& SideCenter (int nSide) { return m_sides [nSide].Center (); }
		inline ushort* Corners (int nSide) { return m_sides [nSide].Corners (); }
		inline ubyte CornerCount (int nSide) { return m_sides [nSide].CornerCount (); }
		inline CFixVector* GetCornerVertices (int nSide, CFixVector* vertices) { return m_sides [nSide].GetCornerVertices (vertices); }
		ubyte SideDists (const CFixVector& intersection, fix* xSideDists, int bBehind = 1);
		int ConnectedSide (CSegment* other);
		inline CFixVector& Normal (int nSide, int nFace) { return m_sides [nSide].Normal (nFace); }
#if 0
		inline uint PointToFaceRelation (CFixVector& intersection, int nSide, short iFace)
		 { return m_sides [nSide].PointToFaceRelation (intersection, iFace, Normal (nSide, iFace)); }
#endif
		inline int SphereToFaceRelation (CFixVector& intersection, fix rad, int nSide, short iFace)
		 { return m_sides [nSide].SphereToFaceRelation (intersection, rad, iFace, Normal (nSide, iFace)); }
		inline int CheckLineToFaceRegular (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, int nSide, short iFace)
		 { return m_sides [nSide].CheckLineToFaceRegular (intersection, p0, p1, rad, iFace, Normal (nSide, iFace)); }
		inline int CheckLineToFaceSpecial (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, int nSide, int iFace)
		 { return m_sides [nSide].CheckLineToFaceSpecial (intersection, p0, p1, rad, iFace, Normal (nSide, iFace)); }

		inline int FaceCount (int nSide) { return m_sides [nSide].FaceCount (); }
		CSegMasks Masks (const CFixVector& refP, fix xRad);
		CSegMasks SideMasks (int nSide, const CFixVector& refP, fix xRad, bool bCheckPoke = false);
		ubyte GetSideDists (const CFixVector& refP, fix* xSideDists, int bBehind);
		ubyte GetSideDistsf (const CFloatVector& refP, float* fSideDists, int bBehind);
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
		CBitmap* ChangeTextures (short nBaseTex, short nOvlTex, short nSide = -1);

		void OperateTrigger (int nSide, CObject *objP, int bShot);

		inline ubyte Function (void) { return m_function; }

		inline int HasFunction (ubyte function) { return m_function == function; }

		inline int HasProp (ubyte prop) { return (m_props & prop) != 0; }
		inline int HasBlockedProp (void) { return HasProp (SEGMENT_PROP_BLOCKED); }
		inline int HasDamageProp (int i = -1) { return (i < 0) ? (m_xDamage [0] | m_xDamage [1]) != 0 : (m_xDamage [i] != 0); }
		inline int HasNoDamageProp (void) { return HasProp (SEGMENT_PROP_NODAMAGE); }
		inline int HasWaterProp (void) { return HasProp (SEGMENT_PROP_WATER); }
		inline int HasLavaProp (void) { return HasProp (SEGMENT_PROP_LAVA); }
		inline int HasOutdoorsProp (void) { return HasProp (SEGMENT_PROP_OUTDOORS); }

		inline int HasTexture (ubyte nSide) { return (m_children [nSide] >= 0) || m_sides [nSide].IsWall (); }

		inline int IsLava (ubyte nSide) { return HasTexture (nSide) && m_sides [nSide].IsLava (); }
		inline int IsWater (ubyte nSide) { return HasTexture (nSide) && m_sides [nSide].IsWater (); }


	private:
		inline int PokesThrough (int nObject, int nSide);
		void Upgrade (void);
	};

inline int operator- (CSegment* s, CArray<CSegment>& a) { return a.Index (s); }

//------------------------------------------------------------------------------

typedef struct tFaceTriangle {
	ushort				nFace;
	ushort				index [3];
	int					nIndex;
	} tFaceTriangle;

class CSegFaceInfo {
	public:
		ushort				index [4];
		int					nKey;
		int					nIndex;
		int					nTriIndex;
		int					nVerts;
		int					nTris;
		int					nFrame;
		CFloatVector		color;
		float					fRads [2];
		short					nWall;
		short					nBaseTex;
		short					nOvlTex;
		short					nCorona;
		short					nSegment;
		ushort				nLightmap;
		ubyte					nSide;
		ubyte					nOvlOrient :2;
		ubyte					bVisible :1;
		ubyte					bTextured :1;
		ubyte					bOverlay :1;
		ubyte					bSplit :1;
		ubyte					bTransparent :1;
		ubyte					bIsLight :1;
		ubyte					bAdditive :2;
		ubyte					bHaveCameraBg :1;
		ubyte					bAnimation :1;
		ubyte					bTeleport :1;
		ubyte					bSlide :1;
		ubyte					bSolid :1;
		ubyte					bSparks :1;
		ubyte					nRenderType :2;
		ubyte					nTriangles :2;
		ubyte					bColored :1;
		ubyte					bCloaked :1;
		ubyte					bHasColor :1;
		ubyte					bSegColor :1;
		ubyte					widFlags;
		char					nCamera;
		char					nType;
		char					nSegColor;
		char					nShader;
		char					nTransparent;
		char					nColored;
		};

class CSegFace {
	public:
		CSegFaceInfo		m_info;
		ushort*				triIndex;
#if USE_RANGE_ELEMENTS
		uint*					vertIndex;
#endif
		CBitmap*				bmBot;
		CBitmap*				bmTop;
		tTexCoord2f*		texCoordP;	//needed to override default tex coords, e.g. for camera outputs
		CSegFace*			nextSlidingFace;

	public:
		CSegment* Segment (void);
		inline CSide* Side (void) { return Segment ()->Side (m_info.nSide); }
		inline CFloatVector Normal (void) { 
			CSide* sideP = Side ();
			return (sideP->m_nType == 1) ? sideP->m_fNormals [0] : CFloatVector::Avg (sideP->m_fNormals [0], sideP->m_fNormals [1]);
			}
		};

inline int operator- (CSegFace* f, CArray<CSegFace>& a) { return a.Index (f); }

//------------------------------------------------------------------------------

typedef struct tSegFaces {
	CSegFace*	faceP;
	ubyte		nFaces;
	ubyte		bVisible;
} tSegFaces;

#define S2F_AMBIENT_WATER   0x01
#define S2F_AMBIENT_LAVA    0x02

//------------------------------------------------------------------------------

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
extern ushort sideVertIndex [SEGMENT_SIDE_COUNT][4];       // sideVertIndex[my_side] is list of vertices forming side my_side.
extern char oppSideTable [];                                // oppSideTable [my_side] returns CSide opposite cube from my_side.

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

void SetupSegments (fix xPlaneDistTolerance = -1);

// ----------------------------------------------------------------------------

#endif //_SEGMENT_H

