#ifndef __OOF_H
#define __OOF_H

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#if DBG
#	define OOF_TEST_CUBE 0
#endif

//------------------------------------------------------------------------------
// Subobject flags

#define OOF_PMF_LIGHTMAP_RES	1
#define OOF_PMF_TIMED			2			// Uses new timed animation
#define OOF_PMF_ALPHA			4			// Has alpha per vertex qualities
#define OOF_PMF_FACING			8			// Has a submodel that is always facing
#define OOF_PMF_NOT_RESIDENT	16			// This CPolyModel is not in memory
#define OOF_PMF_SIZE_COMPUTED	32			// This CPolyModel's size is computed

#define OOF_SOF_ROTATE		0x01			// This subobject is a rotator
#define OOF_SOF_TURRET		0x02			//	This subobject is a turret that tracks
#define OOF_SOF_SHELL		0x04			// This subobject is a door housing
#define OOF_SOF_FRONTFACE	0x08			// This subobject contains the front face for the door
#define OOF_SOF_MONITOR1	0x010			// This subobject contains it's first monitor
#define OOF_SOF_MONITOR2	0x020			// This subobject contains it's second monitor
#define OOF_SOF_MONITOR3	0x040			// This subobject contains it's third monitor
#define OOF_SOF_MONITOR4	0x080			// This subobject contains it's fourth monitor
#define OOF_SOF_MONITOR5	0x0100
#define OOF_SOF_MONITOR6	0x0200
#define OOF_SOF_MONITOR7	0x0400
#define OOF_SOF_MONITOR8	0x0800
#define OOF_SOF_FACING		0x01000		// This subobject always faces you
#define OOF_SOF_VIEWER		0x02000		// This subobject is marked as a 'viewer'.
#define OOF_SOF_LAYER		0x04000		// This subobject is marked as part of possible secondary model rendering.
#define OOF_SOF_WB			0x08000		// This subobject is part of a weapon battery
#define OOF_SOF_GLOW			0x0200000	// This subobject glows
#define OOF_SOF_CUSTOM		0x0400000	// This subobject has textures/colors that are customizable
#define OOF_SOF_THRUSTER	0x0800000   // This is a thruster subobject
#define OOF_SOF_JITTER		0x01000000  // This object jitters by itself
#define OOF_SOF_HEADLIGHT	0x02000000	// This subobject is a headlight

#define OOF_MAX_GUNS_PER_MODEL	      64
#define OOF_MAX_SUBOBJECTS		         30
#define OOF_MAX_POINTS_PER_SUBOBJECT	300

#define OOF_WB_INDEX_SHIFT		16       // bits to shift over to get the weapon battery index (after masking out flags)
#define OOF_SOF_WB_MASKS		0x01F0000// Room for 32 weapon batteries (currently we only use 21 slots)

#define OOF_SOF_MONITOR_MASK	0x0ff0	// mask for monitors

//------------------------------------------------------------------------------

#define OOF_PAGENAME_LEN	35

typedef char tChunkType [4];

namespace OOF {

class CSubModel;
class CModel;

class CTriangle {
	public:
		CFloatVector	p [3];
		CFloatVector	c;
	};

class CQuad {
	public:
		CFloatVector	p [4];
		CFloatVector c;
	};

class CChunkHeader {
	public:
		tChunkType		m_nType;
		int32_t				m_nLength;
	};

class CFaceVert {
	public:
		int32_t		m_nIndex;
		float		m_fu;
		float		m_fv;

	public:
		int32_t Read (CFile& cf, int32_t bFlipV);
	};

class CFace {
	public:
		CFloatVector		m_vNormal;
		CFloatVector		m_vRotNormal;
		CFloatVector		m_vCenter;
		CFloatVector		m_vRotCenter;
		int32_t					m_nVerts;
		int32_t					m_bTextured;
		union {
			int32_t				nTexId;
			CRGBColor		color;
			} m_texProps;
		CFaceVert*			m_vertices;
		float					m_fBoundingLength;
		float					m_fBoundingWidth;
		CFloatVector		m_vMin;
		CFloatVector		m_vMax;
		uint8_t					m_bFacingLight : 1;
		uint8_t					m_bFacingViewer : 1;
		uint8_t					m_bReverse : 1;

	public:
		int32_t Read (CFile& cf, CSubModel *pso, CFaceVert *pfv, int32_t bFlipV);
		inline CFloatVector* CalcCenter (CSubModel *pso);
		inline CFloatVector *CalcNormal (CSubModel *pso);
};

class CFaceList {
	public:
		int32_t					m_nFaces;
		CArray<CFace>		m_list;
		CArray<CFaceVert>	m_vertices;

	public:
		CFaceList () { Init (); }
		~CFaceList () { Destroy (); }
		void Init (void);
		void Destroy (void);
};

class CGlowInfo {
	public:
		CFloatVector3		m_color;
		float				m_fSize;
		float				m_fLength;
		CFloatVector	m_vCenter;
		CFloatVector	m_vNormal;

	public:
		void Init (void) { memset (this, 0, sizeof (*this)); }
};

class CSpecialPoint {
	public:
		char*				m_pszName;
		char*				m_pszProps;
		CFloatVector	m_vPos;
		float				m_fRadius;

	public:
		CSpecialPoint () { Init (); }
		~CSpecialPoint () { Destroy (); }
		void Init (void);
		void Destroy (void);
		int32_t Read (CFile& cf);

};

class CSpecialList : public CArray<CSpecialPoint> {
	public:
		int32_t Read (CFile& cf);
	};

class CPoint {
	public:
		int32_t				m_nParent;
	   CFloatVector	m_vPos;
		CFloatVector	m_vDir;

	public:
		int32_t Read (CFile& cf, int32_t bParent);
	};

class CPointList : public CArray<CPoint> {
	public:
		int32_t Read (CFile& cf, int32_t bParent, int32_t nSize);
	};

class CAttachPoint : public CPoint {
	public:
		CFloatVector	m_vu;
		char				m_bu;

	public:
		int32_t Read (CFile& cf);
};

class CAttachList : public CArray<CAttachPoint> {
	public:
		int32_t Read (CFile& cf);
		int32_t ReadNormals (CFile& cf);
	};

class CBattery {
	public:
		int32_t				m_nVerts;
		CArray<int32_t>		m_vertIndex;
		int32_t				m_nTurrets;
		CArray<int32_t>		m_turretIndex;

	public:
		CBattery () { Init (); }
		void Init (void);
		void Destroy (void);
		int32_t Read (CFile& cf);
};

class CArmament : public CArray<CBattery> {
	public:
		int32_t Read (CFile& cf);
	};

class CFrameInfo {
	public:
		int32_t		m_nFrames;
		int32_t		m_nFirstFrame;
		int32_t		m_nLastFrame;

	public:
		CFrameInfo () { Init (); }
		void Init (void);
		int32_t Read (CFile& cf, CModel* po, int32_t bTimed);
};

class CPosFrame {
	public:
		int32_t				m_iKeyFrame;
		CFloatVector	m_vPos;
		int32_t				m_nStartTime;

	public:
		int32_t Read (CFile& cf, int32_t bTimed);

};

class CAnim : public CFrameInfo {
	public:
		int32_t				m_nTicks;
		CArray<uint8_t>	m_remapTicks;

	public:
		CAnim () { Init (); }
		void Init (void);
		void Destroy (void);
	};

class CPosAnim : public CAnim {
	public:
		CArray<CPosFrame>	m_frames;

	public:
		void Destroy (void);
		int32_t Read (CFile& cf, CModel* po, int32_t bTimed);
	};

class CRotFrame {
	public:
		int32_t					m_iKeyFrame;
		CFloatVector		m_vAxis;
		int32_t					m_nAngle;
		CFloatMatrix		m_mMat;
		int32_t					m_nStartTime;

	public:
		int32_t Read (CFile& cf, int32_t bTimed);
	};

class CRotAnim : public CAnim {
	public:
		CArray<CRotFrame>	m_frames;

	public:
		CRotAnim () { Init (); }
		void Init (void);
		void Destroy (void);
		int32_t Read (CFile& cf, CModel* po, int32_t bTimed);
		void BuildMatrices (void);

	private:
		void BuildAngleMatrix (CFloatMatrix *pm, int32_t a, CFloatVector *pAxis);
	};

class CEdge {
	public:
		int32_t		m_v0 [2];
		int32_t		m_v1 [2];
		CFace*	m_faces [2];
		char		m_bContour;
	};

class CEdgeList {
	public:
		int32_t				m_nEdges;
		int32_t				m_nContourEdges;
		CArray<CEdge>	m_list;

	public:
		CEdgeList () { Init (); }
		~CEdgeList () { Destroy (); }
		void Init (void);
		void Destroy (void);
	};

class CSubModel {
	public:
		int32_t							m_nIndex;
		int32_t							m_nParent;
		int32_t							m_nFlags;
		CFloatVector				m_vNormal;
		float							m_fd;
		CFloatVector				m_vPlaneVert;
		CFloatVector				m_vOffset;
		float							m_fRadius;
		int32_t							m_nTreeOffset;
		int32_t							m_nDataOffset;
		CFloatVector				m_vCenter;
		char*							m_pszName;
		char*							m_pszProps;
		int32_t							m_nMovementType;
		int32_t							m_nMovementAxis;
		int32_t							m_nFSLists;
		CArray<int32_t>					m_fsLists;
		int32_t							m_nVerts;
		CArray<CFloatVector>		m_vertices;
		CArray<CFloatVector>		m_rotVerts;
		CArray<CFloatVector>		m_normals;
		CArray<CFaceColor>		m_vertColors;
		CArray<float>				m_pfAlpha;	// only present if version >= 2300
		CFaceList					m_faces;
		CEdgeList					m_edges;
		CPosAnim						m_posAnim;
		CRotAnim						m_rotAnim;
		CFloatVector				m_vMin;
		CFloatVector				m_vMax;
		CGlowInfo					m_glowInfo;
		float							m_fFOV;
		float							m_fRPS;
		float							m_fUpdate;
		int32_t							m_nChildren;
		int32_t							m_children [OOF_MAX_SUBOBJECTS];
		CAngleVector				m_aMod;
		CFloatMatrix				m_mMod;				// The angles from parent.  Stuffed by model_set_instance
		CFloatVector				m_vMod;

	public:
		CSubModel () { Init (); }
		~CSubModel () { Destroy (); }
		void Init (void);
		void Destroy (void);
		int32_t Read (CFile& cf, CModel* po, int32_t bFlipV);
		int32_t AddEdge (CFace *pf, int32_t i0, int32_t i1);
		int32_t Render (CObject *objP, CModel *po, CFloatVector vo, int32_t nIndex, float *fLight);

	private:
		int32_t FindVertex (int32_t i);
		int32_t FindEdge (int32_t i0, int32_t i1);
		void SetProps (char *pszProps);

		void Transform (CFloatVector vo);
		inline void TransformVertex (CFloatVector *prv, CFloatVector *pv, CFloatVector *vo);
		int32_t Draw (CObject *objP, CModel *po, float *fLight);
};	

class CModel {
	public:
		int16_t					m_nModel;
		int32_t					m_nVersion;
		int32_t					m_bCustom;
		int32_t					m_nFlags;
		float					m_fMaxRadius;
		CFloatVector		m_vMin;
		CFloatVector		m_vMax;
		int32_t					m_nDetailLevels;
		int32_t					m_nSubModels;
		CArray<CSubModel>	m_subModels;
		CPointList			m_gunPoints;
		CAttachList			m_attachPoints;
		CSpecialList		m_specialPoints;
		CArmament			m_armament;
		CModelTextures		m_textures;
		CFrameInfo			m_frameInfo;
		int32_t					m_bCloaked;
		int32_t					m_nCloakPulse;
		int32_t					m_nCloakChangedTime;
		float					m_fAlpha;

	public:
		CModel () { Init (); }
		~CModel () { Destroy (); }
		void Init (void);
		bool Create (void);
		void Destroy (void);
		int32_t Read (char *filename, int16_t nModel, int32_t bFlipV, int32_t bCustom);
		int32_t ReleaseTextures (void);
		int32_t ReloadTextures (void);
		int32_t FreeTextures (void);
		int32_t Render (CObject *objP, float *fLight, int32_t bCloaked);
		int32_t RenderShadow (CObject *objP, float *fLight);

	private:
		int32_t ReadInfo (CFile& cf);
		int32_t ReadTextures (CFile& cf);
		void BuildAnimMatrices (void);
		void AssignChildren (void);
		inline void LinkSubModelBatteries (int32_t iObject, int32_t iBatt);
		void LinkBatteries (void);
		void BuildPosTickRemapList (void);
		void BuildRotTickRemapList (void);
		void ConfigureSubObjects (void);
		void GetSubModelBounds (CSubModel *pso, CFloatVector vo);
		void GetBounds (void);
		void ConfigureSubModels (void);
		int32_t Draw (CObject *objP, float *fLight);

	};

} //OOFModel

//------------------------------------------------------------------------------

float OOF_Centroid (CFloatVector *pvCentroid, CFloatVector *pvSrc, int32_t nv);
float *OOF_GlIdent (float *pm);
float *OOF_GlTranspose (float *pDest, float *pSrc);
int32_t OOF_ReleaseTextures (void);
int32_t OOF_ReloadTextures (void);
int32_t OOF_FreeTextures (void);

//------------------------------------------------------------------------------

#endif //__OOF_H
//eof
