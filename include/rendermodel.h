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
		ushort					m_nIndex;
		char						m_bTextured;
	};

inline int operator- (RenderModel::CVertex* f, CArray<RenderModel::CVertex>& a) { return a.Index (f); }

class CFace {
	public:
		CFixVector				m_vNormal;
		ushort					m_nVerts;
		short						m_nBitmap;
		CBitmap*					m_textureP;
		ushort					m_nIndex;
		ushort					m_nId;
		ubyte						m_nSubModel;
		ubyte						m_bThruster;
		ubyte						m_bGlow :2;
		ubyte						m_bBillboard;

	public:
		void SetTexture (CBitmap* textureP);
		int GatherVertices (CArray<CVertex>&, CArray<CVertex>&, int nIndex);
		
		static int Compare (const CFace* pf, const CFace* pm);

		inline const bool operator< (CFace& other) { return m_nSubModel < other.m_nSubModel; }
		inline const bool operator> (CFace& other) { return m_nSubModel > other.m_nSubModel; }
		const bool operator!= (CFace& other);
	};

inline int operator- (RenderModel::CFace* f, CArray<RenderModel::CFace>& a) { return a.Index (f); }

class CSubModel {
	public:
		short						m_nSubModel;
#if DBG
		char						m_szName [256];
#endif
		CFixVector				m_vOffset;
		CFixVector				m_vCenter;
		CFloatVector3			m_vMin;
		CFloatVector3			m_vMax;
		CFace*					m_faces;
		short						m_nParent;
		ushort					m_nFaces;
		ushort					m_nIndex;
		short						m_nBitmap;
		short						m_nHitbox;
		int						m_nRad;
		ushort					m_nAngles;
		ubyte						m_nType :2;
		ubyte						m_bRender :1;
		ubyte						m_bFlare :1;
		ubyte						m_bWeapon :1;
		ubyte						m_bHeadlight :1;
		ubyte						m_bBullets :1;
		ubyte						m_bBombMount :1;
		ubyte						m_bGlow :2;
		ubyte						m_bBillboard :1;
		ubyte						m_bThruster;
		char						m_nGunPoint;
		char						m_nGun;
		char						m_nBomb;
		char						m_nMissile;
		char						m_nWeaponPos;
		ubyte						m_nFrames;
		ubyte						m_iFrame;
		time_t					m_tFrame;

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
	};

inline int operator- (RenderModel::CSubModel* f, CArray<RenderModel::CSubModel>& a) { return a.Index (f); }

class CVertNorm {
	public:
		CFloatVector3	vNormal;
		ubyte		nVerts;
	};


class CModel {
	public:

	public:
		short										m_nModel;
		CArray<CBitmap>						m_textures;
		int										m_teamTextures [8];
		CArray<CFloatVector3>				m_vertices;
		CArray<CFloatVector3>				m_vertNorms;
		CArray<CFaceColor>					m_color;
		CArray<CVertex>						m_faceVerts;
		CArray<CVertex>						m_sortedVerts;
		CArray<ubyte>							m_vbData;
		CArray<tTexCoord2f>					m_vbTexCoord;
		CArray<CFloatVector>					m_vbColor;
		CArray<CFloatVector3>				m_vbVerts;
		CArray<CFloatVector3>				m_vbNormals;
		CArray<CSubModel>						m_subModels;
		CArray<CFace>							m_faces;
		CArray<CRenderVertex>				m_vertBuf [2];
		CArray<ushort>							m_index [2];
		short										m_nGunSubModels [MAX_GUNS];
		float										m_fScale;
		short										m_nType; //-1: custom mode, 0: default model, 1: alternative model, 2: hires model
		ushort									m_nFaces;
		ushort									m_iFace;
		ushort									m_nVerts;
		ushort									m_nFaceVerts;
		ushort									m_iFaceVert;
		ushort									m_nSubModels;
		ushort									m_nTextures;
		ushort									m_iSubModel;
		short										m_bHasTransparency;
		short										m_bValid;
		short										m_bRendered;
		ushort									m_bBullets;
		CFixVector								m_vBullets;
		GLuint									m_vboDataHandle;
		GLuint									m_vboIndexHandle;

	public:
		CModel () { Init (); }
		~CModel () { Destroy (); }
		void Init (void);
		void Setup (int bHires, int bSort);
		bool Create (void);
		void Destroy (void);
		ushort FilterVertices (CArray<CFloatVector3>& vertices, ushort nVertices);
		fix Radius (CObject* objP);
		fix Size (CObject *objP, int bHires);
		int Shift (CObject *objP, int bHires, CFloatVector3 *vOffsetfP);
		int MinMax (tHitbox *phb);
		void SetGunPoints (CObject *objP, int bASE);
		void SetShipGunPoints (OOF::CModel *po);
		void SetRobotGunPoints (OOF::CModel *po);

		static int CmpVerts (const CFloatVector3* pv, const CFloatVector3* pm);

		int BuildFromASE (CObject *objP, int nModel);
		int BuildFromOOF (CObject *objP, int nModel);
		int BuildFromPOF (CObject* objP, int nModel, CPolyModel* pp, CArray<CBitmap*>& modelBitmaps, CFloatVector* objColorP);

	private:
		void CountASEModelItems (ASE::CModel *pa);
		void GetASEModelItems (int nModel, ASE::CModel *pa, float fScale);

		void CountOOFModelItems (OOF::CModel *po);
		void GetOOFModelItems (int nModel, OOF::CModel *po, float fScale);

		void AssignPOFFaces (void);
		int CountPOFModelItems (void* modelDataP, ushort* pnSubModels, ushort* pnVerts, ushort* pnFaces, ushort* pnFaceVerts);
		CFace* AddPOFFace (CSubModel* psm, CFace* pmf, CFixVector* pn, ubyte* p, CArray<CBitmap*>& modelBitmaps, CFloatVector* objColorP, bool bTextured = true);
		int GetPOFModelItems (void *modelDataP, CAngleVector *pAnimAngles, int nThis, int nParent,
									 int bSubObject, CArray<CBitmap*>& modelBitmaps, CFloatVector *objColorP);

	};	

//	-----------------------------------------------------------------------------------------------------------

} //RenderModel

#endif


