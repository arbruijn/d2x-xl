#ifndef _RENDERMODEL_H
#define _RENDERMODEL_H

#include <math.h>

#include "descent.h"
#include "carray.h"
#include "ase.h"
#include "oof.h"
#include "pof.h"

namespace RenderModel {

class CModel;

//	-----------------------------------------------------------------------------

#define G3_BUFFER_OFFSET(_i)	(GLvoid *) ((char *) NULL + (_i))

class CRenderVertex {
	public:
		CFloatVector3			m_vertex;
		CFloatVector3			m_normal;
		CFloatVector			m_color;
		tTexCoord2f				m_texCoord;
		};

class CVertex {
	public:
		tTexCoord2f				m_texCoord;
		CFloatVector			m_renderColor;
		CFloatVector3			m_vertex;
		CFloatVector3			m_normal;
		CFloatVector			m_baseColor;
		uint16_t					m_nIndex;
		uint8_t					m_bTextured;
	};

inline int32_t operator- (RenderModel::CVertex* f, CArray<RenderModel::CVertex>& a) { return a.Index (f); }

class CContourInfo {
	public:
		CFloatVector3			m_vCenterf [2];
		CFloatVector3			m_vNormalf [2];
		GLenum					m_faceWinding;
		uint16_t					m_bFrontFace :1;
		uint16_t					m_bFacingLight :1;
		uint16_t					m_bFacingViewer :1;
		uint16_t					m_bTransformed :1;

	public:
		int32_t IsFacingPoint (CFloatVector3& v);
		int32_t IsFacingViewer (void);
		int32_t IsFront (void);
		int32_t IsLit (CFloatVector3& vLight);
		void Transform (void);

	private:
		void RotateNormal (void);
		void TransformCenter (void);
};

class CFace : public CContourInfo {
	public:
		CFixVector				m_vNormal;
		uint16_t					m_nVerts;
		int16_t					m_nBitmap;
		CBitmap*					m_textureP;
		uint16_t					m_nIndex;
		uint16_t					m_nId;
		uint8_t					m_nSubModel;
		uint8_t					m_bThruster;
		uint8_t					m_bGlow :2;
		uint8_t					m_bBillboard;

	public:
		void SetTexture (CBitmap* textureP);
		int32_t GatherVertices (CArray<CVertex>&, CArray<CVertex>&, int32_t nIndex);
		
		static int32_t Compare (const CFace* pf, const CFace* pm);

		inline const bool operator< (CFace& other) { return m_nSubModel < other.m_nSubModel; }
		inline const bool operator> (CFace& other) { return m_nSubModel > other.m_nSubModel; }
		const bool operator!= (CFace& other);
	};

inline int32_t operator- (RenderModel::CFace* f, CArray<RenderModel::CFace>& a) { return a.Index (f); }

class CModelEdge : public CMeshEdge {
	public:
		void Transform (void);
		int32_t IsFacingViewer (int16_t nFace);
		int32_t IsContour (void);

	protected:
		virtual int32_t Visibility (void);
		virtual int32_t Type (void);
		virtual float PartialAngle (void);
		virtual float PlanarAngle (void);
		virtual CFloatVector& Normal (int32_t i);
		virtual CFloatVector& Vertex (int32_t i, int32_t bTransformed = 0);
	};


class CSubModel {
	public:
		int16_t					m_nSubModel;
#if DBG
		char						m_szName [256];
#endif
		CFixVector				m_vOffset;
		CFixVector				m_vCenter;
		CFloatVector3			m_vMin;
		CFloatVector3			m_vMax;
		CFace*					m_faces;
		int16_t					m_nParent;
		uint16_t					m_nFaces;
		uint16_t					m_nVertices;
		uint16_t					m_nVertexIndex [2];
		int16_t					m_nBitmap;
		int16_t					m_nHitbox;
		int32_t					m_nRad;
		uint16_t					m_nAngles;
		uint8_t					m_nType :2;
		uint8_t					m_bRender :1;
		uint8_t					m_bFlare :1;
		uint8_t					m_bWeapon :1;
		uint8_t					m_bHeadlight :1;
		uint8_t					m_bBullets :1;
		uint8_t					m_bBombMount :1;
		uint8_t					m_bGlow :2;
		uint8_t					m_bBillboard :1;
		uint8_t					m_bThruster;
		char						m_nGunPoint;
		char						m_nGun;
		char						m_nBomb;
		char						m_nMissile;
		char						m_nWeaponPos;
		uint8_t					m_nFrames;
		uint8_t					m_iFrame;
		time_t					m_tFrame;
		CArray<CModelEdge>	m_edges;
		uint16_t					m_nEdges;

	public:
		CSubModel () { Init (); }
		~CSubModel () { Destroy (); }
		void Init (void) { memset (this, 0, sizeof (*this)); }
		bool Create (void);
		void Destroy (void) { Init (); }
		void InitMinMax (void);
		void SetMinMax (CFloatVector3 *vertexP);
		void SortFaces (CBitmap* textureP);
		void GatherVertices (CArray<CVertex>& source, CArray<CVertex>& dest);
		void Size (CModel* pm, CObject* objP, CFixVector* vOffset);

		uint16_t CountEdges (void);
		int32_t FindEdge (CModel* modelP, uint16_t v1, uint16_t v2);
		int32_t AddEdge (CModel *modelP, CFace* faceP, uint16_t v1, uint16_t v2);
		bool BuildEdgeList (CModel* modelP);

		void GatherContourEdges (CModel* modelP);
		void GatherLitFaces (CModel* modelP, CFloatVector3& vLight);
	};

inline int32_t operator- (RenderModel::CSubModel* f, CArray<RenderModel::CSubModel>& a) { return a.Index (f); }

class CVertNorm {
	public:
		CFloatVector3	vNormal;
		uint8_t			nVerts;
	};

class CVertexOwner {
	public:
		uint16_t			m_nOwner;
		uint16_t			m_nVertex;

	public:
		inline bool operator< (CVertexOwner& other) { return (m_nOwner < other.m_nOwner) || ((m_nOwner == other.m_nOwner) && (m_nVertex < other.m_nVertex)); }
		inline bool operator> (CVertexOwner& other) { return (m_nOwner > other.m_nOwner) || ((m_nOwner == other.m_nOwner) && (m_nVertex > other.m_nVertex)); }
		inline bool operator!= (CVertexOwner& other) { return (m_nOwner != other.m_nOwner) || (m_nVertex != other.m_nVertex); }
	};

class CModel {
	public:

	public:
		int16_t									m_nModel;
		CArray<CBitmap>						m_textures;
		int32_t									m_teamTextures [8];
		CArray<CFloatVector3>				m_vertices;
		CArray<CVertexOwner>					m_vertexOwner;
		CArray<CFloatVector3>				m_vertNorms;
		CArray<CFaceColor>					m_color;
		CArray<CVertex>						m_faceVerts;
		CArray<CVertex>						m_sortedVerts;
		CArray<uint8_t>						m_vbData;
		CArray<tTexCoord2f>					m_vbTexCoord;
		CArray<CFloatVector>					m_vbColor;
		CArray<CFloatVector3>				m_vbVerts;
		CArray<CFloatVector3>				m_vbNormals;
		CArray<CSubModel>						m_subModels;
		CArray<CFace>							m_faces;
		CArray<CRenderVertex>				m_vertBuf [2];
		CArray<uint16_t>						m_index [2];
		CStack<CFace*>							m_litFaces;
		CStack<uint16_t>						m_contourEdges;
		int16_t									m_nGunSubModels [MAX_GUNS];
		float										m_fScale;
		int16_t									m_nType; //-1: custom mode, 0: default model, 1: alternative model, 2: hires model
		uint16_t									m_nFaces;
		uint16_t									m_iFace;
		uint16_t									m_nVerts;
		uint16_t									m_nFaceVerts;
		uint16_t									m_iFaceVert;
		uint16_t									m_nSubModels;
		uint16_t									m_nTextures;
		uint16_t									m_iSubModel;
		int16_t									m_bHasTransparency;
		int16_t									m_bValid;
		int16_t									m_bRendered;
		uint16_t									m_bBullets;
		CFixVector								m_vBullets;
		GLuint									m_vboDataHandle;
		GLuint									m_vboIndexHandle;
		CArray<CModelEdge>					m_edges;
		uint16_t									m_nEdges;

	public:
		CModel () { Init (); }
		~CModel () { Destroy (); }
		void Init (void);
		void Setup (int32_t bHires, int32_t bSort);
		bool Create (void);
		void Destroy (void);
		uint16_t FilterVertices (CArray<CFloatVector3>& vertices, uint16_t nVertices);
		fix Radius (CObject* objP);
		fix Size (CObject *objP, int32_t bHires);
		int32_t Shift (CObject *objP, int32_t bHires, CFloatVector3 *vOffsetfP);
		int32_t MinMax (tHitbox *phb);
		void SetGunPoints (CObject *objP, int32_t bASE);
		void SetShipGunPoints (OOF::CModel *po);
		void SetRobotGunPoints (OOF::CModel *po);

		static int32_t CmpVerts (const CFloatVector3* pv, const CFloatVector3* pm);

		int32_t BuildFromASE (CObject *objP, int32_t nModel);
		int32_t BuildFromOOF (CObject *objP, int32_t nModel);
		int32_t BuildFromPOF (CObject* objP, int32_t nModel, CPolyModel* pp, CArray<CBitmap*>& modelBitmaps, CFloatVector* objColorP);

	private:
		void CountASEModelItems (ASE::CModel *pa);
		void GetASEModelItems (int32_t nModel, ASE::CModel *pa, float fScale);

		void CountOOFModelItems (OOF::CModel *po);
		void GetOOFModelItems (int32_t nModel, OOF::CModel *po, float fScale);

		void AssignPOFFaces (void);
		int32_t CountPOFModelItems (void* modelDataP, uint16_t* pnSubModels, uint16_t* pnVerts, uint16_t* pnFaces, uint16_t* pnFaceVerts);
		CFace* AddPOFFace (CSubModel* psm, CFace* pmf, CFixVector* pn, uint8_t* p, CArray<CBitmap*>& modelBitmaps, CFloatVector* objColorP, bool bTextured = true);
		int32_t GetPOFModelItems (void *modelDataP, CAngleVector *pAnimAngles, int32_t nThis, int32_t nParent,
										  int32_t bSubObject, CArray<CBitmap*>& modelBitmaps, CFloatVector *objColorP);

	};	

//	-----------------------------------------------------------------------------------------------------------

} //RenderModel

#endif


