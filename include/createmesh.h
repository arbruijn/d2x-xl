#ifndef _CREATEMESH_H
#define _CREATEMESH_H

typedef struct tMeshEdge {
	int			nNext;
	ushort		verts [2];
	int			tris [2];
	} tMeshEdge;

typedef struct tMeshTri {
	int			nNext;
	int			nFace;
	int			nIndex;
	int			lines [3];
	ushort		index [3];
	ushort		nPass;
	tTexCoord2f	texCoord [3];
	tTexCoord2f	ovlTexCoord [3];
	tRgbaColorf	color [3];
} tMeshTri;

class CTriMeshBuilder {
	private:
		tMeshEdge	*m_meshEdges;
		tMeshTri		*m_meshTris;
		int			m_nMeshEdges;
		int			m_nFreeEdges;
		int			m_nMeshTris;
		int			m_nFreeTris;
		int			m_nMaxMeshTris;
		int			m_nMaxMeshEdges;
		int			m_nVertices;

	private:
		void FreeMeshData (void);
		int AllocMeshData (void);
		tMeshEdge *FindMeshEdge (ushort nVert1, ushort nVert2, int i);
		int AddMeshEdge (int nTri, ushort nVert1, ushort nVert2);
		tMeshTri *CreateMeshTri (tMeshTri *mtP, ushort index [], int nFace, int nIndex);
		int AddMeshTri (tMeshTri *mtP, ushort index [], grsTriangle *triP);
		void DeleteMeshEdge (tMeshEdge *mlP);
		void DeleteMeshTri (tMeshTri *mtP);
		int CreateMeshTris (void);
		int SplitMeshTriByEdge (int nTri, ushort nVert1, ushort nVert2, ushort nPass);
		int SplitMeshEdge (tMeshEdge *mlP, ushort nPass);
		int SplitMeshTri (tMeshTri *mtP, ushort nPass);
		int SplitMeshTris (void);
		void QSortMeshTris (int left, int right);
		int InsertMeshTris (void);

	public:
		CTriMeshBuilder (void) {};
		~CTriMeshBuilder (void) {};
		int BuildMesh (void);
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
		void BuildMesh (void);
	};

extern CFaceMeshBuilder faceMeshBuilder;

#endif //_CREATEMESH_H