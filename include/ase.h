#ifndef _ASE_H
#define _ASE_H

namespace ASE {

class CVertex {
	public:
		CFloatVector3			m_vertex;
		CFloatVector3			m_normal;
	};

class CFace {
	public:
		CFloatVector3			m_vNormal;
		short						m_nVerts [3];	// indices of vertices 
		short						m_nTexCoord [3];
		short						m_nBitmap;
};

class CSubModel {
	public:
		CSubModel*				m_next;
		char						m_szName [256];
		char						m_szParent [256];
		short						m_nSubModel;
		short						m_nParent;
		short						m_nBitmap;
		short						m_nFaces;
		short						m_nVerts;
		short						m_nTexCoord;
		short						m_nIndex;
		ubyte						m_bRender;
		ubyte						m_bGlow;
		ubyte						m_bThruster;
		ubyte						m_bWeapon;
		char						m_nGun;
		char						m_nBomb;
		char						m_nMissile;
		char						m_nType;
		char						m_nWeaponPos;
		char						m_nGunPoint;
		char						m_nBullets;
		char						m_bBarrel;
		CFloatVector3			m_vOffset;
		CArray<CFace>			m_faces;
		CArray<CVertex>		m_verts;
		CArray<tTexCoord2f>	m_texCoord;

	public:
		CSubModel () { Init (); }
		~CSubModel () { Destroy (); }
		void Init (void);
		void Destroy (void);
		int Read (CFile& cf);

	private:
		int ReadNode (CFile& cf);
		int ReadMeshVertexList (CFile& cf);
		int ReadMeshFaceList (CFile& cf);
		int ReadVertexTexCoord (CFile& cf);
		int ReadFaceTexCoord (CFile& cf);
		int ReadMeshNormals (CFile& cf);
		int ReadMesh (CFile& cf);
	};

class CModel {
	public:
		CModelTextures			m_textures;
		CSubModel*				m_subModels;
		int						m_nModel;
		int						m_nType;
		int						m_nSubModels;
		int						m_nVerts;
		int						m_nFaces;

	public:
		CModel () { Init (); }
		~CModel () { Destroy (); }
		void Init (void);
		bool Create (void);
		void Destroy (void);
		int Read (const char* filename, short nModel, short nType, int bCustom);
		int ReloadTextures (int bCustom);
		int ReleaseTextures (void);
		int FreeTextures (void);

		static int CModel::Error (const char *pszMsg);

	private:
		int ReadTexture (CFile& cf, int nBitmap, int nType, int bCustom);
		int ReadMaterial (CFile& cf, int nType, int bCustom);
		int ReadMaterialList (CFile& cf, int nType, int bCustom);
		int FindSubModel (const char *pszName);
		void LinkSubModels (void);
		int ReadSubModel (CFile& cf);
	};

} //ASEModel

int ASE_ReloadTextures (void);
int ASE_ReleaseTextures (void);

#endif //_ASE_H

