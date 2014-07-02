#ifndef _CREATEMESH_H
#define _CREATEMESH_H

#include "carray.h"

namespace Mesh {

	typedef struct tEdge {
		int32_t			nNext;
		uint16_t		verts [2];
		int32_t			tris [2];
		float			fLength;
		} __pack__ tEdge;

	typedef struct tTriangle {
		int32_t			nFace;
		int32_t			nIndex;
		int32_t			lines [3];
		uint16_t		index [3];
		int16_t			nPass;
		int16_t			nId;
		tTexCoord2f	texCoord [3];
		tTexCoord2f	ovlTexCoord [3];
		CFloatVector	color [3];
	} __pack__ tTriangle;

class CTriMeshBuilder {
	private:
		CArray<tEdge>		m_edges;
		CArray<tTriangle>	m_triangles;
		int32_t					m_nEdges;
		int32_t					m_nFreeEdges;
		int32_t					m_nTriangles;
		int32_t					m_nMaxTriangles;
		int32_t					m_nMaxEdges;
		int32_t					m_nVertices;
		int32_t					m_nTris;
		int32_t					m_nQuality;

	private:
		void FreeData (void);
		int32_t AllocData (void);
		tEdge *FindEdge (uint16_t nVert1, uint16_t nVert2, int32_t i);
		int32_t AddEdge (int32_t nTri, uint16_t nVert1, uint16_t nVert2);
		tTriangle *CreateTriangle (tTriangle *mtP, uint16_t index [], int32_t nFace, int32_t nIndex);
		tTriangle *AddTriangle (tTriangle *mtP, uint16_t index [], tFaceTriangle *triP);
		void DeleteEdge (tEdge *mlP);
		void DeleteTriangle (tTriangle *mtP);
		int32_t CreateTriangles (void);
		int32_t SplitTriangleByEdge (int32_t nTri, uint16_t nVert1, uint16_t nVert2, int16_t nPass);
		int32_t SplitEdge (CSegFace* faceP, tEdge *mlP, int16_t nPass);
		float NewEdgeLen (int32_t nTri, int32_t nVert1, int32_t nVert2);
		int32_t SplitTriangle (CSegFace* faceP, tTriangle *mtP, int16_t nPass);
		int32_t SplitTriangles (void);
		void QSortTriangles (int32_t left, int32_t right);
		void CreateSegFaceList (void);
		void SetupVertexNormals (void);
		int32_t InsertTriangles (void);
		void CreateFaceVertLists (void);
		void SortFaceVertList (uint16_t *vertList, int32_t left, int32_t right);
		char *DataFilename (char *pszFilename, int32_t nLevel);
		bool Load (int32_t nLevel, bool bForce = false);
		bool Save (int32_t nLevel);

	public:
		CTriMeshBuilder (void) {};
		~CTriMeshBuilder (void) {};
		int32_t Build (int32_t nLevel, int32_t nQuality);
	};

class CQuadMeshBuilder {
	private:
		CSegFace*		m_faceP;
		tFaceTriangle*	m_triP;
		CFloatVector3*	m_vertexP;
		CFloatVector3*	m_normalP;
		tTexCoord2f*	m_texCoordP;
		tTexCoord2f*	m_ovlTexCoordP;
		tTexCoord2f*	m_lMapTexCoordP;
		CFloatVector*	m_faceColorP;
		CFaceColor*		m_colorP;
		CSegment*		m_segP;
		tSegFaces*		m_segFaceP;
		CSide*			m_sideP;

		uint16_t			m_sideVerts [5];
		int16_t				m_nOvlTexCount;
		int16_t				m_nWall;
		int16_t				m_nWallType;
		//bool				m_bColoredSeg;

		CTriMeshBuilder	m_triMeshBuilder;

	private:
		void InitFace (int16_t nSegment, uint8_t nSide, bool bRebuild);
		void SetupLMapTexCoord (tTexCoord2f *texCoordP);
		void SetupFace (void);
		void InitTexturedFace (void);
		void InitColoredFace (int16_t nSegment);
		void SplitIn1or2Tris (void);
		void SplitIn4Tris (void);
		void BuildSlidingFaceList (void);
		int32_t IsBigFace (uint16_t* sideVerts);
		CFloatVector3 *SetTriNormals (tFaceTriangle *triP, CFloatVector3 *m_normalP);

		static int32_t CompareFaceKeys (const CSegFace** pf, const CSegFace** pm);

	public:
		CQuadMeshBuilder (void) {};
		~CQuadMeshBuilder (void) {};
		void RebuildLightmapTexCoord (void);
		int32_t Build (int32_t nLevel, bool bRebuild = false);
		bool BuildVBOs ();
		void DestroyVBOs ();
		void ComputeFaceKeys (void);
	};

}

extern Mesh::CQuadMeshBuilder meshBuilder;

#endif //_CREATEMESH_H
