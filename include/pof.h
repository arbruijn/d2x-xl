#ifndef _POF_H
#define _POF_H

#include "carray.h"
#include "cstack.h"
#include "vecmat.h"
#include "3d.h"

namespace POF {

//------------------------------------------------------------------------------

class CModel;

class CFace {
	public:
		uint16_t				m_nVerts;
		uint16_t*			m_vertices;
		CFixVector			m_vCenter;
		CFixVector			m_vNorm;
		CFixVector			m_vRotNorm;
		CFloatVector		m_vNormf;
		CFloatVector		m_vCenterf;
		float					m_fClipDist;
		uint8_t				m_bFacingLight :1;
		uint8_t				m_bFrontFace :1;
		uint8_t				m_bGlow :1;
		uint8_t				m_bTest :1;
		uint8_t				m_bIgnore :1;
		uint16_t				m_nAdjFaces;

	public:
		CFace () { Init (); }
		void Init (void);
		inline int32_t IsLit (void);
		inline int32_t IsFront (void);
		inline int32_t IsFacingLight (void);
		inline int32_t IsFacingViewer (void);
		inline CFloatVector* RotateNormal (void);
		inline CFloatVector* TransformCenter (void);
		void CalcNormal (CModel* po);
		CFloatVector CalcCenterf (CModel* po);
		CFixVector CalcCenter (CModel* po);
		int32_t RenderShadowVolume (CModel* po, int32_t bCullFront);
		inline float ClipDist (CObject *objP);
};

inline int32_t operator- (CFace* f, CArray<CFace>& a) { return a.Index (f); }

class CSubModel {
	public:
		CFace*				m_faces;		//points at submodel's first face in model's face list (which is sorted by submodels)
		uint16_t				m_nFaces;
		CFace**				m_litFaces;	//submodel faces facing the current light source
		uint16_t				m_nLitFaces;
		CFixVector			m_vPos;
		CAngleVector		m_vAngles;
		float					m_fClipDist;
		int16_t				m_nParent;
		uint16_t*			m_adjFaces;
		int16_t				m_nRenderFlipFlop;
		int16_t				m_bCalcClipDist;

	public:
		CSubModel () { Init (); }
		void Init (void);
		int32_t GatherLitFaces (CModel* po);
		void RotateNormals (void);
		int32_t FindEdge (CFace* pf0, int32_t v0, int32_t v1);
		int32_t CalcFacing (CModel* po);
		int32_t RenderShadowCaps (CObject *objP, CModel* po, int32_t bCullFront);
		int32_t RenderShadowVolume (CModel* po, int32_t bCullFront);
		int32_t RenderShadow (CObject *objP, CModel* po);

	private:
		float ClipDistByFaceCenters (CObject *objP, CModel* po, int32_t i, int32_t incr);
		float ClipDistByFaceVerts (CObject *objP, CModel* po, float fMaxDist, int32_t i, int32_t incr);
		float ClipDist (CObject *objP, CModel* po);
};

inline int32_t operator- (CSubModel* o, CArray<CSubModel>& a) { return a.Index (o); }

class CModel {
	public:
		CArray<CSubModel>			m_subModels;
		int16_t						m_nSubModels;
		uint16_t						m_nVerts;
		uint16_t						m_nFaces;
		uint16_t						m_nFaceVerts;
		uint16_t						m_nLitFaces;
		uint16_t						m_nAdjFaces;
		CArray<CFixVector>		m_vertices;
		CArray<CFloatVector>		m_vertsf;
		CArray<float>				m_fClipDist;
		CArray<uint8_t>			m_vertFlags;
		CArray<CRenderNormal>	m_vertNorms;
		CFixVector					m_vCenter;
		CArray<CFixVector>		m_rotVerts;
		CArray<CFace>				m_faces;
		CStack<CFace*>				m_litFaces;
		CArray<uint16_t>			m_adjFaces;
		CArray<uint16_t>			m_faceVerts;
		CArray<uint16_t>			m_vertMap;
		int16_t						m_iSubObj;
		uint16_t						m_iVert;
		uint16_t						m_iFace;
		uint16_t						m_iFaceVert;
		char							m_nState;
		uint8_t						m_nVertFlag;

	public:
		CModel () { Init (); }
		~CModel () { Destroy (); }
		void Init (void);
		void Destroy (void);
		inline void AddTriangle (CFace *pf, uint16_t v0, uint16_t v1, uint16_t v2);
		int32_t FindFace (uint16_t *p, int32_t nVerts);
		CFace* AddFace (CSubModel* pso, CFace* pf, CFixVector *pn, uint8_t *p, int32_t bShadowData);
		CFloatVector* VertsToFloat (void);
		int32_t GatherAdjFaces (void);
		void CalcCenters (void);
		int32_t CountItems (void *modelDataP);
		int32_t GatherItems (void *modelDataP, CAngleVector *animAngleP, int32_t bInitModel, int32_t bShadowData, int32_t nThis, int32_t nParent);
		int32_t Create (void *modelDataP, int32_t bShadowData);
};

} //namespace POF

#endif //_POF_H
//eof
