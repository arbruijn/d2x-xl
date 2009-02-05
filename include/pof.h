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
		short					m_nVerts;
		short*				m_verts;
		CFixVector			m_vCenter;
		CFixVector			m_vNorm;
		CFixVector			m_vRotNorm;
		CFloatVector		m_vNormf;
		CFloatVector		m_vCenterf;
		float					m_fClipDist;
		ubyte					m_bFacingLight :1;
		ubyte					m_bFrontFace :1;
		ubyte					m_bGlow :1;
		ubyte					m_bTest :1;
		ubyte					m_bIgnore :1;
		short					m_nAdjFaces;

	public:
		CFace () { Init (); }
		void Init (void);
		inline int IsLit (void);
		inline int IsFront (void);
		inline int IsFacingLight (void);
		inline int IsFacingViewer (void);
		inline CFloatVector* RotateNormal (void);
		inline CFloatVector* TransformCenter (void);
		void CalcNormal (CModel* po);
		CFloatVector CalcCenterf (CModel* po);
		CFixVector CalcCenter (CModel* po);
		int RenderShadowVolume (CModel* po, int bCullFront);
		inline float ClipDist (CObject *objP);
};

inline int operator- (CFace* f, CArray<CFace>& a) { return a.Index (f); }

class CSubModel {
	public:
		CFace*				m_faces;		//points at submodel's first face in model's face list (which is sorted by submodels)
		short					m_nFaces;
		CFace**				m_litFaces;	//submodel faces facing the current light source
		short					m_nLitFaces;
		CFixVector			m_vPos;
		CAngleVector		m_vAngles;
		float					m_fClipDist;
		short					m_nParent;
		short*				m_adjFaces;
		short					m_nRenderFlipFlop;
		short					m_bCalcClipDist;

	public:
		CSubModel () { Init (); }
		void Init (void);
		int GatherLitFaces (CModel* po);
		void RotateNormals (void);
		int FindEdge (CFace* pf0, int v0, int v1);
		int CalcFacing (CModel* po);
		int RenderShadowCaps (CObject *objP, CModel* po, int bCullFront);
		int RenderShadowVolume (CModel* po, int bCullFront);
		int RenderShadow (CObject *objP, CModel* po);

	private:
		float ClipDistByFaceCenters (CObject *objP, CModel* po, int i, int incr);
		float ClipDistByFaceVerts (CObject *objP, CModel* po, float fMaxDist, int i, int incr);
		float ClipDist (CObject *objP, CModel* po);
};

inline int operator- (CSubModel* o, CArray<CSubModel>& a) { return a.Index (o); }

class CModel {
	public:
		CArray<CSubModel>		m_subModels;
		short						m_nSubModels;
		short						m_nVerts;
		short						m_nFaces;
		short						m_nFaceVerts;
		short						m_nLitFaces;
		short						m_nAdjFaces;
		CArray<CFixVector>	m_verts;
		CArray<CFloatVector>	m_vertsf;
		CArray<float>			m_fClipDist;
		CArray<ubyte>			m_vertFlags;
		CArray<g3sNormal>		m_vertNorms;
		CFixVector				m_vCenter;
		CArray<CFixVector>	m_rotVerts;
		CArray<CFace>			m_faces;
		CStack<CFace*>			m_litFaces;
		CArray<short>			m_adjFaces;
		CArray<short>			m_faceVerts;
		CArray<short>			m_vertMap;
		short						m_iSubObj;
		short						m_iVert;
		short						m_iFace;
		short						m_iFaceVert;
		char						m_nState;
		ubyte						m_nVertFlag;

	public:
		CModel () { Init (); }
		~CModel () { Destroy (); }
		void Init (void);
		void Destroy (void);
		inline void AddTriangle (CFace *pf, short v0, short v1, short v2);
		int FindFace (short *p, int nVerts);
		CFace* AddFace (CSubModel* pso, CFace* pf, CFixVector *pn, ubyte *p, int bShadowData);
		CFloatVector* VertsToFloat (void);
		int GatherAdjFaces (void);
		void CalcCenters (void);
		int CountItems (void *modelDataP);
		int GatherItems (void *modelDataP, CAngleVector *animAngleP, int bInitModel, int bShadowData, int nThis, int nParent);
		int Create (void *modelDataP, int bShadowData);
};

} //namespace POF

#endif //_POF_H
//eof
