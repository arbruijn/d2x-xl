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

#define SIDE_IS_QUAD						1   // render CSide as quadrilateral
#define SIDE_IS_TRI_02					2   // render CSide as two triangles, triangulated along edge from 0 to 2
#define SIDE_IS_TRI_13					3   // render CSide as two triangles, triangulated along edge from 1 to 3

#define SEGMENT_SHAPE_CUBE				0
#define SEGMENT_SHAPE_WEDGE			1
#define SEGMENT_SHAPE_PYRAMID			2

#define SIDE_SHAPE_QUAD					0
#define SIDE_SHAPE_TRIANGLE			1
#define SIDE_SHAPE_EDGE					2
#define SIDE_SHAPE_POINT				3

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
#define MAX_SEGMENTS						gameData.segData.nMaxSegments
#define MAX_SEGMENT_VERTICES			(8  *MAX_SEGMENTS) 
#define MAX_SEGMENT_VERTICES_D2X		(3  *MAX_SEGMENTS_D2X)

#define MAX_SIDES							(MAX_SEGMENTS  *6)
#define MAX_FACES							(MAX_SIDES  *2)
#define MAX_TRIANGLES					(MAX_FACES  *16)

//------------------------------------------------------------------------------
//normal everyday vertices

#define DEFAULT_LIGHTING				0   // (I2X (1)/2)

# define MAX_VERTICES					(MAX_SEGMENT_VERTICES)
# define MAX_VERTICES_D2X				(MAX_SEGMENT_VERTICES_D2X)

//------------------------------------------------------------------------------
//values for function field
#define SEGMENT_TYPE_NONE				0
#define SEGMENT_TYPE_PRODUCER			1
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
#define SEGMENT_FUNC_FUELCENTER		1
#define SEGMENT_FUNC_REPAIRCENTER	2
#define SEGMENT_FUNC_REACTOR			3
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

#define OBJ_IS_PROJECTILE				1
#define OBJ_IS_ENERGY_PROJECTILE		2
#define OBJ_IS_SPLASHDMG_WEAPON		4
#define OBJ_HAS_LIGHT_TRAIL			8
#define OBJ_IS_PLAYER_MINE				16
#define OBJ_IS_ROBOT_MINE				32
#define OBJ_IS_MISSILE					64
#define OBJ_IS_GATLING_ROUND			128
#define OBJ_BOUNCES						256
#define OBJ_IS_WEAPON					(OBJ_IS_PROJECTILE | OBJ_IS_MISSILE)

// Returns true if nSegment references a child, else returns false.
// Note that -1 means no connection, -2 means a connection to the outside world.
#define IS_CHILD(nSegment) (nSegment > -1)

extern uint16_t sideVertIndex [SEGMENT_SIDE_COUNT][4];
extern char oppSideTable [SEGMENT_SIDE_COUNT];

//------------------------------------------------------------------------------

class CSegMasks {
	public:
	   int16_t m_face;     //which faces sphere pokes through (12 bits)
		int8_t m_side;     //which sides sphere pokes through (6 bits)
		int8_t m_center;   //which sides center point is on back of (6 bits)
		int8_t m_valid;

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

static inline int32_t IsWaterTexture (int16_t nTexture)
{
return ((nTexture >= 399) && (nTexture <= 403));
}

//------------------------------------------------------------------------------

static inline int32_t IsLavaTexture (int16_t nTexture)
{
return (nTexture == 378) || ((nTexture >= 404) && (nTexture <= 409));
}

//------------------------------------------------------------------------------

class CTrigger;
class CWall;
class CObject;

class CSide {
	public:
		int8_t   		m_nType;       // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
		int8_t   		m_nFrame;      //keep us longword aligned
		uint16_t  		m_nWall;
		int16_t   		m_nBaseTex;
#ifdef WORDS_BIGENDIAN
		uint16_t			m_nOvlOrient : 2;
		uint16_t			m_nOvlTex : 14;
#else
		uint16_t			m_nOvlTex : 14;
		uint16_t			m_nOvlOrient : 2;
#endif
		tUVL     		m_uvls [4];
		CFixVector		m_normals [3];  // 2 normals, if quadrilateral, both the same.
		CFloatVector	m_fNormals [3];
		CFixVector		m_rotNorms [2];
		CFixVector		m_vCenter;
		fix				m_rads [2];
		uint16_t			m_vertices [6];
		uint16_t			m_faceVerts [6]; // vertex indices of the side's two triangles
		uint16_t			m_nMinVertex [2];
		uint16_t			m_corners [4];
		uint8_t			m_nFaces;
		uint8_t			m_nShape;
		uint8_t			m_nCorners;
		uint8_t			m_bIsQuad;
		int16_t			m_nSegment;

	public:
		inline void Init (void) { memset (this, 0, sizeof (*this)); }
		int32_t Read (CFile& cf, uint16_t *segVerts, uint16_t *sideVerts, bool bSolid);
		void ReadWallNum (CFile& cf, bool bWall);
		void SaveState (CFile& cf);
		void LoadState (CFile& cf);
		void LoadTextures (void);
		inline uint16_t WallNum (void) { return m_nWall; }
		inline int8_t Type (void) { return m_nType; }
		inline void SetType (int8_t nType) { m_nType = nType; }
		bool IsWall (void);
		CWall *Wall (void);
		CTrigger *Trigger (void);
		bool IsVolatile (void);
		inline uint8_t IsQuad (void) { return m_bIsQuad; }
		inline uint8_t CornerCount (void) { return m_nCorners; }
		bool IsSolid (void);

		void CheckSum (uint32_t& sum1, uint32_t& sum2);

		int32_t CheckTransparency (void);
		int32_t CheckLineToFaceEdges (CFixVector& vIntersection, CFixVector *p0, CFixVector *p1, fix rad, int16_t iFace, CFixVector vNormal);
		int32_t CheckLineToFaceRegular (CFixVector& vIntersection, CFixVector *p0, CFixVector *p1, fix rad, int16_t iFace, CFixVector vNormal);
		int32_t SphereToFaceRelation (CFixVector& vIntersection, fix rad, int16_t iFace, CFixVector vNormal);
		uint32_t PointToFaceRelation (CFixVector& vIntersection, int16_t iFace, CFixVector vNormal);
		int32_t IsConnected (int16_t nSegment, int16_t nSide);
		int32_t SeesSide (int16_t nSegment, int16_t nSide);
		int32_t SeesConnectedSide (int16_t nSegment, int16_t nSide);
		int32_t SeesPoint (CFixVector& vPoint, int16_t nDestSeg, int8_t nDestSide, int32_t nLevel = 0, int32_t nThread = 0);

		int32_t CheckLineToFaceEdgesf (CFloatVector& vIntersection, CFloatVector& p0, CFloatVector& p1, float rad, int16_t iFace, CFloatVector& vNormal);
		int32_t CheckLineToFaceRegularf (CFloatVector& vIntersection, CFloatVector *p0, CFloatVector *p1, float rad, int16_t iFace, CFloatVector& vNormal);
		int32_t SphereToFaceRelationf (CFloatVector& vIntersection, float rad, int16_t iFace, CFloatVector& vNormal);
		uint32_t PointToFaceRelationf (CFloatVector& vIntersection, int16_t iFace, CFloatVector& vNormal);

		void GetNormals (CFixVector& n1, CFixVector& n2);

		void ComputeCenter (void);
		void ComputeRads (void);
		CFixVector& Center (void) { return m_vCenter; }
		fix MinRad (void) { return m_rads [0]; }
		fix MaxRad (void) { return m_rads [1]; }
		fix AvgRad (void) { return (m_rads [0] + m_rads [1]) / 2; }

		void Setup (int16_t nSegment, uint16_t *verts, uint16_t *index, bool bSolid);
		void FixNormals (void);

		void SetTextures (int32_t nBaseTex, int32_t nOvlTex);

		inline uint8_t GetVertices (uint16_t*& vertices) { 
			vertices = m_vertices;
			return m_nFaces;
			}
		CFixVector *GetCornerVertices (CFixVector *vertices);
		inline uint16_t *Corners (void) { return m_corners; }
		int32_t HasVertex (uint16_t nVertex);
		CFixVector& Vertex (int32_t nVertex);
		CFixVector& MinVertex (void);
		CFixVector& Normal (int32_t nFace);
		CFloatVector& Normalf (int32_t nFace);
		fix Height (void);
		bool IsPlanar (void);
		uint8_t Dist (const CFixVector& point, fix& xSideDist, int32_t bBehind, int16_t sideBit);
		uint8_t Distf (const CFloatVector& point, float& fSideDist, int32_t bBehind, int16_t sideBit);
		fix DistToPoint (CFixVector v);
		float DistToPointf (CFloatVector v);
		CSegMasks Masks (const CFixVector& pRef, fix xRad, int16_t sideBit, int16_t faceBit, bool bCheckPoke = false);
		void HitPointUV (fix *u, fix *v, fix *l, CFixVector& intersection, int32_t iFace);
		int32_t CheckForTranspPixel (CFixVector& intersection, int16_t iFace);
		int32_t Physics (fix& damage, bool bSolid);

		bool IsOpenableDoor (void);
		bool IsTextured (void);

		inline int32_t IsLava (void) { return IsLavaTexture (m_nBaseTex) || (m_nOvlTex && (IsLavaTexture (m_nOvlTex))); }
		inline int32_t IsWater (void) { return IsWaterTexture (m_nBaseTex) || (m_nOvlTex && (IsWaterTexture (m_nOvlTex))); }

		inline uint8_t Shape (void) { return m_nShape; }
		inline uint8_t FaceCount (void) { return m_nFaces; }
		void RemapVertices (uint8_t *map);

	private:
		void SetupCorners (uint16_t *verts, uint16_t *index);
		void SetupVertexList (uint16_t *verts, uint16_t *index);
		void SetupFaceVertIndex (void);
		void SetupAsQuad (CFixVector& vNormal, CFloatVector& vNormalf, uint16_t *verts, uint16_t *index);
		void SetupAsTriangles (bool bSolid, uint16_t *verts, uint16_t *index);
	};

//------------------------------------------------------------------------------

typedef struct tDestructableTextureProps {
	uint16_t			nOvlTex;
	uint16_t			nOvlOrient;
	int32_t			nEffect;
	int32_t			nBitmap;
	int32_t			nSwitchType;
} tDestructableTextureProps;

//------------------------------------------------------------------------------

class CSegment {
	public:
		CSide			m_sides [SEGMENT_SIDE_COUNT];       // 6 sides
		int16_t		m_children [SEGMENT_SIDE_COUNT];    // indices of 6 children segments, front, left, top, right, bottom, back
		fix			m_childDists [2][SEGMENT_SIDE_COUNT];
		uint16_t		m_vertices [SEGMENT_VERTEX_COUNT];    // vertex ids of 4 front and 4 back vertices
		int16_t		m_nVertices;
		int32_t		m_objects;    // pointer to objects in this tSegment

		uint8_t		m_nShape;
		uint8_t		m_function;
		uint8_t		m_flags;
		uint8_t		m_props;
		int16_t		m_value;
		int16_t		m_nObjProducer;
		fix			m_xDamage [2];
		fix			m_xAvgSegLight;

		char			m_owner;		  // team owning that tSegment (-1: always neutral, 0: neutral, 1: blue team, 2: red team)
		char			m_group;
		CFixVector	m_vCenter;
		fix			m_rads [2];
		CFixVector	m_extents [2];

	public:
		int32_t Index (void);
		void Read (CFile& cf);
		void ReadFunction (CFile& cf, uint8_t flags);
		void ReadVerts (CFile& cf);
		void ReadChildren (CFile& cf, uint8_t flags);
		void ReadExtras (CFile& cf);
		void SaveState (CFile& cf);
		void LoadState (CFile& cf);
		void LoadTextures (void);
		void LoadSideTextures (int32_t nSide);
		void LoadBotGenTextures (void);
		void Setup (void);
		inline uint16_t WallNum (int32_t nSide) { return (nSide < 0) ? 0xFFFF : m_sides [nSide].WallNum (); }
		inline int32_t CheckTransparency (int32_t nSide) { return (nSide < 0) ? 0 : m_sides [nSide].CheckTransparency (); }
		void CheckSum (uint32_t& sum1, uint32_t& sum2);

		inline bool IsWall (int32_t nSide) { return (nSide < 0) ? false : m_sides [nSide].IsWall (); }
		void SetTexture (int32_t nSide, CSegment *pConnSeg, int16_t nConnSide, int32_t nAnim, int32_t nFrame);
		void DestroyWall (int32_t nSide);
		void DamageWall (int32_t nSide, fix damage);
		void BlastWall (int32_t nSide);
		void OpenDoor (int32_t nSide);
		void CloseDoor (int32_t nSide, bool bForce = false);
		void StartCloak (int32_t nSide);
		void StartDecloak (int32_t nSide);
		void IllusionOff (int32_t nSide);
		void IllusionOn (int32_t nSide);
		int32_t AnimateOpeningDoor (int32_t nSide, fix xElapsedTime);
		int32_t AnimateClosingDoor (int32_t nSide, fix xElapsedTime);
		void ToggleWall (int32_t nSide);
		int32_t ProcessWallHit (int32_t nSide, fix damage, int32_t nPlayer, CObject *pObj);
		int32_t DoorIsBlocked (int32_t nSide, bool bIgnoreMarker = false);
		int32_t TextureIsDestructable (int32_t nSide, tDestructableTextureProps *dtpP = NULL);
		int32_t BlowupTexture (int32_t nSide, CFixVector& vHit, CObject *blower, int32_t bForceBlowup);
		void CreateSound (int16_t nSound, int32_t nSide);

		fix Refuel (fix xMaxFuel);
		fix Repair (fix xMaxShield);
		fix ShieldDamage (fix xMaxDamage);
		fix EnergyDamage (fix xMaxDamage);
		void CheckForGoal (void);
		void CheckForHoardGoal (void);
		int32_t CheckFlagDrop (int32_t nTeamId, int32_t nFlagId, int32_t nGoalId);
		int32_t ConquerCheck (void);

		void ChangeTexture (int32_t oldOwner);
		void OverrideTextures (int16_t nTexture, int16_t nOldTexture, int16_t nTexture2, int32_t bFullBright, int32_t bForce);

		bool CreateGenerator (int32_t nType);
		bool CreateEquipmentMaker (int32_t nOldFunction);
		bool CreateRobotMaker (int32_t nOldFunction);
		bool CreateProducer (int32_t nOldFunction);
		bool CreateObjectProducer (int32_t nOldFunction, int32_t nMaxCount);

		void ComputeCenter (void);
		void ComputeRads (fix xMinDist);
		void ComputeChildDists (void);
		inline void ComputeSideCenter (int32_t nSide) { m_sides [nSide].ComputeCenter (); }
		inline int16_t ChildId (int32_t nSide) { return m_children [nSide]; }
		inline CSide *Side (int32_t nSide) { return m_sides + nSide; }
		int32_t ChildIndex (int32_t nChild);
		CSide *AdjacentSide (int32_t nSegment);
		CSide *OppositeSide (int32_t nSide);
		int32_t SeesConnectedSide (int16_t nSide, int16_t nChildSeg, int16_t nChildSide);
		inline CWall *Wall (int32_t nSide) { return m_sides [nSide].Wall (); }
		inline CTrigger *Trigger (int32_t nSide) { return m_sides [nSide].Trigger (); }
		inline int8_t Type (int32_t nSide) { return m_sides [nSide].m_nType; }
		inline void SetType (int32_t nSide, int8_t nType) { m_sides [nSide].SetType (nType); }
		inline void SetTextures (int32_t nSide, int32_t nBaseTex, int32_t nOvlTex) { m_sides [nSide].SetTextures (nBaseTex, nOvlTex); }

		void GetCornerIndex (int32_t nSide, uint16_t *vertIndex);
		void ComputeSideRads (void);
		float FaceSize (uint8_t nSide);
		inline bool IsVertex (int32_t nVertex);
		void GetNormals (int32_t nSide, CFixVector& n1, CFixVector& n2) { m_sides [nSide].GetNormals (n1, n2); }
		inline CFixVector& Center (void) { return m_vCenter; }
		inline CFixVector& SideCenter (int32_t nSide) { return m_sides [nSide].Center (); }
		inline uint16_t *Corners (int32_t nSide) { return m_sides [nSide].Corners (); }
		inline uint8_t CornerCount (int32_t nSide) { return m_sides [nSide].CornerCount (); }
		inline CFixVector *GetCornerVertices (int32_t nSide, CFixVector *vertices) { return m_sides [nSide].GetCornerVertices (vertices); }
		uint8_t SideDists (const CFixVector& intersection, fix *xSideDists, int32_t bBehind = 1);
		int32_t ConnectedSide (CSegment *other);
		inline CFixVector& Normal (int32_t nSide, int32_t nFace) { return m_sides [nSide].Normal (nFace); }
#if 0
		inline uint32_t PointToFaceRelation (CFixVector& intersection, int32_t nSide, int16_t iFace)
		 { return m_sides [nSide].PointToFaceRelation (intersection, iFace, Normal (nSide, iFace)); }
#endif
		inline int32_t SphereToFaceRelation (CFixVector& intersection, fix rad, int32_t nSide, int16_t iFace)
		 { return m_sides [nSide].SphereToFaceRelation (intersection, rad, iFace, Normal (nSide, iFace)); }
		inline int32_t CheckLineToFaceRegular (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, int32_t nSide, int16_t iFace)
		 { return m_sides [nSide].CheckLineToFaceRegular (intersection, p0, p1, rad, iFace, Normal (nSide, iFace)); }
		inline int32_t CheckLineToFaceEdges (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, int32_t nSide, int32_t iFace)
		 { return m_sides [nSide].CheckLineToFaceEdges (intersection, p0, p1, rad, iFace, Normal (nSide, iFace)); }

		inline int32_t FaceCount (int32_t nSide) { return m_sides [nSide].FaceCount (); }
		CSegMasks Masks (const CFixVector& pRef, fix xRad);
		CSegMasks SideMasks (int32_t nSide, const CFixVector& pRef, fix xRad, bool bCheckPoke = false);
		uint8_t GetSideDists (const CFixVector& pRef, fix *xSideDists, int32_t bBehind);
		uint8_t GetSideDistsf (const CFloatVector& pRef, float *fSideDists, int32_t bBehind);
		void HitPointUV (int32_t nSide, fix *u, fix *v, fix *l, CFixVector& intersection, int32_t iFace)
			{ m_sides [nSide].HitPointUV (u, v, l, intersection, iFace); }

		int32_t HasVertex (uint8_t nSide, uint16_t nVertex);

		fix MinRad (void) { return m_rads [0]; }
		fix MaxRad (void) { return m_rads [1]; }
		float MinRadf (void) { return X2F (m_rads [0]); }
		float MaxRadf (void) { return X2F (m_rads [1]); }

		inline fix AvgRad (void) {return (m_rads [0] + m_rads [1]) / 2;}
		inline float AvgRadf (void) {return X2F (m_rads [0] + m_rads [1]) / 2;}
		inline fix Volume (void) {return (fix) DRound (1.25  *PI  *pow (AvgRadf (), 3));}

		CFixVector RandomPoint (void);

		int32_t IsPassable (int32_t nSide, CObject *pObj, bool bIgnoreDoors = false);
		int32_t HasOpenableDoor (void);

		inline int32_t CheckForTranspPixel (CFixVector& intersection, int16_t nSide, int16_t iFace) 
		 { return m_sides [nSide].CheckForTranspPixel (intersection, iFace); }

		int32_t Physics (int32_t nSide, fix& damage) { return m_sides [nSide].Physics (damage, m_children [nSide] == -1); }
		int32_t Physics (fix& xDamage);

		int32_t TexturedSides (void);
		CBitmap *ChangeTextures (int16_t nBaseTex, int16_t nOvlTex, int16_t nSide = -1);

		void OperateTrigger (int32_t nSide, CObject *pObj, int32_t bShot);

		inline uint8_t Function (void) { return m_function; }

		inline int32_t HasFunction (uint8_t function) { return m_function == function; }

		inline int32_t HasProp (uint8_t prop) { return (m_props & prop) != 0; }
		inline int32_t HasBlockedProp (void) { return HasProp (SEGMENT_PROP_BLOCKED); }
		inline int32_t HasDamageProp (int32_t i = -1) { return (i < 0) ? (m_xDamage [0] | m_xDamage [1]) != 0 : (m_xDamage [i] != 0); }
		inline int32_t HasNoDamageProp (void) { return HasProp (SEGMENT_PROP_NODAMAGE); }
		inline int32_t HasWaterProp (void) { return HasProp (SEGMENT_PROP_WATER); }
		inline int32_t HasLavaProp (void) { return HasProp (SEGMENT_PROP_LAVA); }
		inline int32_t HasOutdoorsProp (void) { return HasProp (SEGMENT_PROP_OUTDOORS); }

		inline int32_t HasTexture (uint8_t nSide) { return (m_children [nSide] >= 0) || m_sides [nSide].IsWall (); }

		inline int32_t IsLava (uint8_t nSide) { return HasTexture (nSide) && m_sides [nSide].IsLava (); }
		inline int32_t IsWater (uint8_t nSide) { return HasTexture (nSide) && m_sides [nSide].IsWater (); }
		bool IsSolid (int32_t nSide);

		void RemapVertices (void);

	private:
		inline int32_t PokesThrough (int32_t nObject, int32_t nSide);
		void Upgrade (void);
	};

inline int32_t operator- (CSegment *s, CArray<CSegment>& a) { return a.Index (s); }

//------------------------------------------------------------------------------

typedef struct tFaceTriangle {
	uint16_t				nFace;
	uint16_t				index [3];
	int32_t				nIndex;
	} tFaceTriangle;

class CSegFaceInfo {
	public:
		uint16_t				index [4];
		int32_t				nKey;
		int32_t				nIndex;
		int32_t				nTriIndex;
		int32_t				nVerts;
		int32_t				nTris;
		int32_t				nFrame;
		CFloatVector		color;
		float					fRads [2];
		int16_t				nWall;
		int16_t				nBaseTex;
		int16_t				nOvlTex;
		int16_t				nCorona;
		int16_t				nSegment;
		uint16_t				nLightmap;
		uint8_t				nSide;
		uint8_t				nOvlOrient :2;
		uint8_t				bVisible :1;
		uint8_t				bTextured :1;
		uint8_t				bOverlay :1;
		uint8_t				bSplit :1;
		uint8_t				bTransparent :1;
		uint8_t				bIsLight :1;
		uint8_t				bAdditive :2;
		uint8_t				bHaveCameraBg :1;
		uint8_t				bAnimation :1;
		uint8_t				bTeleport :1;
		uint8_t				bSlide :1;
		uint8_t				bSolid :1;
		uint8_t				bSparks :1;
		uint8_t				nRenderType :2;
		uint8_t				nTriangles :2;
		uint8_t				bColored :1;
		uint8_t				bCloaked :1;
		uint8_t				bHasColor :1;
		uint8_t				bSegColor :1;
		uint8_t				widFlags;
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
		uint16_t*			triIndex;
#if USE_RANGE_ELEMENTS
		uint32_t*					vertIndex;
#endif
		CBitmap*				bmBot;
		CBitmap*				bmTop;
		tTexCoord2f*		pTexCoord;	//needed to override default tex coords, e.g. for camera outputs
		CSegFace*			nextSlidingFace;

	public:
		CSegment *Segment (void);
		inline CSide *Side (void) { return Segment ()->Side (m_info.nSide); }
		inline CFloatVector Normal (void) { 
			CSide *pSide = Side ();
			return (pSide->m_nType == 1) ? pSide->m_fNormals [0] : CFloatVector::Avg (pSide->m_fNormals [0], pSide->m_fNormals [1]);
			}
		};

inline int32_t operator- (CSegFace *f, CArray<CSegFace>& a) { return a.Index (f); }

//------------------------------------------------------------------------------

typedef struct tSegFaces {
	CSegFace*	pFace;
	uint8_t		nFaces;
	uint8_t		bVisible;
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
//--repair-- 	int32_t     specialType;
//--repair-- 	int16_t   special_segment; // if specialType indicates repair center, this is the base of the repair center
//--repair-- } lsegment;

// Globals from mglobal.c
extern uint16_t sideVertIndex [SEGMENT_SIDE_COUNT][4];       // sideVertIndex[my_side] is list of vertices forming side my_side.
extern char oppSideTable [];                                // oppSideTable [my_side] returns CSide opposite cube from my_side.

// New stuff, 10/14/95: For shooting out lights and monitors.
// Light cast upon vertLight vertices in nSegment:nSide by some light
class CLightDelta {
	public:
		int16_t   nSegment;
		int8_t   nSide;
		int8_t   bValid;
		uint8_t   vertLight [4];

	public:
		void Read (CFile& cf);
	};

class CLightDeltaIndex {
	public:
		int16_t		nSegment;
		uint16_t	nSide :3;
		uint16_t	count :13;
		uint16_t	index;

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
#define MAX_DELTA_LIGHTS     (MAX_SEGMENTS  *10)

#define DL_SCALE            2048    // Divide light to allow 3 bits integer, 5 bits fraction.

int32_t SubtractLight (int16_t nSegment, int32_t nSide);
int32_t AddLight (int16_t nSegment, int32_t nSide);
void ClearLightSubtracted (void);

int32_t CountSkyBoxSegments (void);
void FreeSkyBoxSegList (void);
int32_t BuildSkyBoxSegList (void);

void SetupSegments (fix xPlaneDistTolerance = -1);

// ----------------------------------------------------------------------------

#endif //_SEGMENT_H

