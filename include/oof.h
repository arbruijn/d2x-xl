#ifndef __OOF_H
#define __OOF_H

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#if defined (_DEBUG) && SHADOWS
#	define OOF_TEST_CUBE 0
#else
#	define OOF_TEST_CUBE 0
#endif

//------------------------------------------------------------------------------
// Subobject flags

#define OOF_PMF_LIGHTMAP_RES	1
#define OOF_PMF_TIMED			2			// Uses new timed animation
#define OOF_PMF_ALPHA			4			// Has alpha per vertex qualities
#define OOF_PMF_FACING			8			// Has a submodel that is always facing
#define OOF_PMF_NOT_RESIDENT	16			// This tPolyModel is not in memory
#define OOF_PMF_SIZE_COMPUTED	32			// This tPolyModel's size is computed

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

namespace OOFModel {

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
		int				m_nLength;
	};

class CFaceVert {
	public:
		int		m_nIndex;
		float		m_fu;
		float		m_fv;

	public:
		int Read (CFile& cf, int bFlipV);
	};

class CFace {
	public:
		CFloatVector		m_vNormal;
		CFloatVector		m_vRotNormal;
		CFloatVector		m_vCenter;
		CFloatVector		m_vRotCenter;
		int					m_nVerts;
		int					m_bTextured;
		union {
			int				nTexId;
			tRgbColorb		color;
			} m_texProps;
		CFaceVert*			m_verts;
		float					m_fBoundingLength;
		float					m_fBoundingWidth;
		CFloatVector		m_vMin;
		CFloatVector		m_vMax;
		ubyte					m_bFacingLight : 1;
		ubyte					m_bFacingViewer : 1;
		ubyte					m_bReverse : 1;

	public:
		int Read (CFile& cf, CSubModel *pso, CFaceVert *pfv, int bFlipV);
		inline CFloatVector* CalcCenter (CSubModel *pso);
		inline CFloatVector *CalcNormal (CSubModel *pso);
};

class CFaceList {
	public:
		int					m_nFaces;
		CArray<CFace>		m_list;
		CArray<CFaceVert>	m_verts;

	public:
		CFaceList () { Init (); }
		~CFaceList () { Destroy (); }
		void Init (void);
		void Destroy (void);
};

class CGlowInfo {
	public:
		tRgbColorf		m_color;
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
		int Read (CFile& cf);

};

class CSpecialList : public CArray<CSpecialPoint> {
	public:
		int Read (CFile& cf);
	};

class CPoint {
	public:
		int				m_nParent;
	   CFloatVector	m_vPos;
		CFloatVector	m_vDir;

	public:
		int Read (CFile& cf, int bParent);
	};

class CPointList : public CArray<CPoint> {
	public:
		int Read (CFile& cf, int bParent, int nSize);
	};

class CAttachPoint : public CPoint {
	public:
		CFloatVector	m_vu;
		char				m_bu;

	public:
		int Read (CFile& cf);
};

class CAttachList : public CArray<CAttachPoint> {
	public:
		int Read (CFile& cf);
		int ReadNormals (CFile& cf);
	};

class CBattery {
	public:
		int				m_nVerts;
		CArray<int>		m_vertIndex;
		int				m_nTurrets;
		CArray<int>		m_turretIndex;

	public:
		CBattery () { Init (); }
		void Init (void);
		void Destroy (void);
		int Read (CFile& cf);
};

class CArmament : public CArray<CBattery> {
	public:
		int Read (CFile& cf);
	};

class CFrameInfo {
	public:
		int		m_nFrames;
		int		m_nFirstFrame;
		int		m_nLastFrame;

	public:
		CFrameInfo () { Init (); }
		void Init (void);
		int Read (CFile& cf, CModel* po, int bTimed);
};

class CPosFrame {
	public:
		int				m_iKeyFrame;
		CFloatVector	m_vPos;
		int				m_nStartTime;

	public:
		int Read (CFile& cf, int bTimed);

};

class CAnim : public CFrameInfo {
	public:
		int				m_nTicks;
		CArray<ubyte>	m_remapTicks;

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
		int Read (CFile& cf, CModel* po, int bTimed);
	};

class CRotFrame {
	public:
		int					m_iKeyFrame;
		CFloatVector		m_vAxis;
		int					m_nAngle;
		CFloatMatrix		m_mMat;
		int					m_nStartTime;

	public:
		int Read (CFile& cf, int bTimed);
	};

class CRotAnim : public CAnim {
	public:
		CArray<CRotFrame>	m_frames;

	public:
		CRotAnim () { Init (); }
		void Init (void);
		void Destroy (void);
		int Read (CFile& cf, CModel* po, int bTimed);
		void BuildMatrices (void);

	private:
		void BuildAngleMatrix (CFloatMatrix *pm, int a, CFloatVector *pAxis);
	};

class CEdge {
	public:
		int		m_v0 [2];
		int		m_v1 [2];
		CFace*	m_faces [2];
		char		m_bContour;
	};

class CEdgeList {
	public:
		int				m_nEdges;
		int				m_nContourEdges;
		CArray<CEdge>	m_list;

	public:
		CEdgeList () { Init (); }
		~CEdgeList () { Destroy (); }
		void Init (void);
		void Destroy (void);
	};

class CSubModel {
	public:
		int							m_nIndex;
		int							m_nParent;
		int							m_nFlags;
		CFloatVector				m_vNormal;
		float							m_fd;
		CFloatVector				m_vPlaneVert;
		CFloatVector				m_vOffset;
		float							m_fRadius;
		int							m_nTreeOffset;
		int							m_nDataOffset;
		CFloatVector				m_vCenter;
		char*							m_pszName;
		char*							m_pszProps;
		int							m_nMovementType;
		int							m_nMovementAxis;
		int							m_nFSLists;
		CArray<int>					m_fsLists;
		int							m_nVerts;
		CArray<CFloatVector>		m_verts;
		CArray<CFloatVector>		m_rotVerts;
		CArray<CFloatVector>		m_normals;
		CArray<tFaceColor>		m_vertColors;
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
		int							m_nChildren;
		int							m_children [OOF_MAX_SUBOBJECTS];
		CAngleVector				m_aMod;
		CFloatMatrix				m_mMod;				// The angles from parent.  Stuffed by model_set_instance
		CFloatVector				m_vMod;

	public:
		CSubModel () { Init (); }
		~CSubModel () { Destroy (); }
		void Init (void);
		void Destroy (void);
		int Read (CFile& cf, CModel* po, int bFlipV);
		int AddEdge (CFace *pf, int i0, int i1);
		int Render (CObject *objP, CModel *po, CFloatVector vo, int nIndex, float *fLight);

	private:
		int FindVertex (int i);
		int FindEdge (int i0, int i1);
		void SetProps (char *pszProps);

		void Transform (CFloatVector vo);
		inline void TransformVertex (CFloatVector *prv, CFloatVector *pv, CFloatVector *vo);
		int Draw (CObject *objP, CModel *po, float *fLight);
};	

class CModel {
	public:
		short					m_nModel;
		short					m_nType;
		int					m_nVersion;
		int					m_nFlags;
		float					m_fMaxRadius;
		CFloatVector		m_vMin;
		CFloatVector		m_vMax;
		int					m_nDetailLevels;
		int					m_nSubModels;
		CArray<CSubModel>	m_subModels;
		CPointList			m_gunPoints;
		CAttachList			m_attachPoints;
		CSpecialList		m_specialPoints;
		CArmament			m_armament;
		CModelTextures		m_textures;
		CFrameInfo			m_frameInfo;
		int					m_bCloaked;
		int					m_nCloakPulse;
		int					m_nCloakChangedTime;
		float					m_fAlpha;

	public:
		CModel () { Init (); }
		~CModel () { Destroy (); }
		void Init (void);
		bool Create (void);
		void Destroy (void);
		int Read (char *filename, short nModel, short nType, int bFlipV, int bCustom);
		int ReleaseTextures (void);
		int ReloadTextures (int bCustom);
		int FreeTextures (void);
		int Render (CObject *objP, float *fLight, int bCloaked);
		int RenderShadow (CObject *objP, float *fLight);

	private:
		int ReadInfo (CFile& cf);
		int ReadTextures (CFile& cf, short nType, int bCustom);
		void BuildAnimMatrices (void);
		void AssignChildren (void);
		inline void LinkSubModelBatteries (int iObject, int iBatt);
		void LinkBatteries (void);
		void BuildPosTickRemapList (void);
		void BuildRotTickRemapList (void);
		void ConfigureSubObjects (void);
		void GetSubModelBounds (CSubModel *pso, CFloatVector vo);
		void GetBounds (void);
		void ConfigureSubModels (void);
		int Draw (CObject *objP, float *fLight);

	};

} //OOFModel

//------------------------------------------------------------------------------

float OOF_Centroid (CFloatVector *pvCentroid, CFloatVector *pvSrc, int nv);
float *OOF_GlIdent (float *pm);
float *OOF_GlTranspose (float *pDest, float *pSrc);
int OOF_ReleaseTextures (void);
int OOF_ReloadTextures (void);
int OOF_FreeTextures (void);

//------------------------------------------------------------------------------

#endif //__OOF_H
//eof
