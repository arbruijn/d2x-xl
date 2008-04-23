#ifndef _CREATEMESH_H
#define _CREATEMESH_H

namespace mesh {

	typedef struct tEdge {
		int			nNext;
		ushort		verts [2];
		int			tris [2];
		float			fLength;
		} tEdge;

	typedef struct tTriangle {
		int			nFace;
		int			nIndex;
		int			lines [3];
		ushort		index [3];
		ushort		nPass;
		tTexCoord2f	texCoord [3];
		tTexCoord2f	ovlTexCoord [3];
		tRgbaColorf	color [3];
	} tTriangle;

class CTriMeshBuilder {
	private:
		tEdge	*m_edges;
		tTriangle	*m_triangles;
		int			m_nEdges;
		int			m_nFreeEdges;
		int			m_nTriangles;
		int			m_nMaxTriangles;
		int			m_nMaxEdges;
		int			m_nVertices;

	private:
		void FreeData (void);
		int AllocData (void);
		tEdge *FindEdge (ushort nVert1, ushort nVert2, int i);
		int AddEdge (int nTri, ushort nVert1, ushort nVert2);
		tTriangle *CreateTriangle (tTriangle *mtP, ushort index [], int nFace, int nIndex);
		int AddTriangle (tTriangle *mtP, ushort index [], grsTriangle *triP);
		void DeleteEdge (tEdge *mlP);
		void DeleteTriangle (tTriangle *mtP);
		int CreateTriangles (void);
		int SplitTriangleByEdge (int nTri, ushort nVert1, ushort nVert2, ushort nPass);
		int SplitEdge (tEdge *mlP, ushort nPass);
		int SplitTriangle (tTriangle *mtP, ushort nPass);
		int SplitTriangles (void);
		void QSortTriangles (int left, int right);
		int InsertTriangles (void);

	public:
		CTriMeshBuilder (void) {};
		~CTriMeshBuilder (void) {};
		int Build (void);
	};

class CFaceMeshBuilder {
	private:
		grsFace		*m_faceP;
		grsTriangle	*m_triP;
		fVector3		*m_vertexP;
		fVector3		*m_normalP;
		tTexCoord2f	*m_texCoordP;
		tTexCoord2f	*m_ovlTexCoordP;
		tRgbaColorf	*m_faceColorP;
		tFaceColor	*m_colorP;
		tSegment		*m_segP;
		tSegFaces	*m_segFaceP;
		tSide			*m_sideP;

		short			m_sideVerts [5];
		short			m_nOvlTexCount;
		short			m_nWall;
		short			m_nWallType;
		bool			m_bColoredSeg;

		CTriMeshBuilder	m_triMeshBuilder;

	private:
		void InitFace (short nSegment, ubyte nSide);
		void SetupFace (void);
		void InitTexturedFace (void);
		void InitColoredFace (short nSegment);
		void SplitIn2Tris (void);
		void SplitIn4Tris (void);
		int IsBigFace (short *m_sideVerts);
		fVector3 *SetTriNormals (grsTriangle *triP, fVector3 *m_normalP);

	public:
		CFaceMeshBuilder (void) {};
		~CFaceMeshBuilder (void) {};
		void Build (void);
	};

};

extern mesh::CFaceMeshBuilder faceMeshBuilder;

#endif //_CREATEMESH_H