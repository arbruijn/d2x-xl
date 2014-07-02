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
		uint16_t					m_nVerts [3];	// indices of vertices 
		uint16_t					m_nTexCoord [3];
		int16_t						m_nBitmap;
};

class CSubModel {
	public:
		CSubModel*				m_next;
		char						m_szName [256];
		char						m_szParent [256];
		int16_t						m_nSubModel;
		int16_t						m_nParent;
		int16_t						m_nBitmap;
		uint16_t					m_nFaces;
		uint16_t					m_nVerts;
		uint16_t					m_nTexCoord;
		uint16_t					m_nIndex;
		uint8_t						m_bRender;
		uint8_t						m_bGlow;
		uint8_t						m_bFlare;
		uint8_t						m_bBillboard;
		uint8_t						m_bThruster;
		uint8_t						m_bWeapon;
		uint8_t						m_bHeadlight;
		uint8_t						m_bBombMount;
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
		CArray<CVertex>		m_vertices;
		CArray<tTexCoord2f>	m_texCoord;

	public:
		CSubModel () { Init (); }
		~CSubModel () { Destroy (); }
		void Init (void);
		void Destroy (void);
		int32_t Read (CFile& cf, int32_t& nVerts, int32_t& nFaces);
		int32_t SaveBinary (CFile& cf);
		int32_t ReadBinary (CFile& cf);

	private:
		int32_t ReadNode (CFile& cf);
		int32_t ReadMeshVertexList (CFile& cf);
		int32_t ReadMeshFaceList (CFile& cf);
		int32_t ReadVertexTexCoord (CFile& cf);
		int32_t ReadFaceTexCoord (CFile& cf);
		int32_t ReadMeshNormals (CFile& cf);
		int32_t ReadMesh (CFile& cf, int32_t& nVerts, int32_t& nFaces);
	};

class CModel {
	public:
		CModelTextures			m_textures;
		CSubModel*				m_subModels;
		int32_t						m_nModel;
		int32_t						m_nSubModels;
		int32_t						m_nVerts;
		int32_t						m_nFaces;
		int32_t						m_bCustom;

	public:
		CModel () { Init (); }
		~CModel () { Destroy (); }
		void Init (void);
		bool Create (void);
		void Destroy (void);
		int32_t Read (const char* filename, int16_t nModel, int32_t bCustom);
		int32_t SaveBinary (void);
		int32_t SaveBinary (const char* szFilename);
		int32_t ReadBinary (const char* szFilename, int32_t bCustom, time_t tASE);
		int32_t ReadBinary (int16_t nModel, int32_t bCustom, time_t tASE);
		int32_t ReloadTextures (void);
		int32_t ReleaseTextures (void);
		int32_t FreeTextures (void);

		static int32_t Error (const char *pszMsg);

	private:
		int32_t ReadTexture (CFile& cf, int32_t nBitmap);
		int32_t ReadOpacity (CFile& cf, int32_t nBitmap);
		int32_t ReadMaterial (CFile& cf);
		int32_t ReadMaterialList (CFile& cf);
		int32_t FindSubModel (const char *pszName);
		void LinkSubModels (void);
		int32_t ReadSubModel (CFile& cf);
	};

} //ASEModel

int32_t ASE_ReloadTextures (void);
int32_t ASE_ReleaseTextures (void);

#endif //_ASE_H

