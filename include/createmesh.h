#ifndef _CREATEMESH_H
#define _CREATEMESH_H

typedef struct tMeshLine {
	ushort		verts [2];
	int			tris [2];
	} tMeshLine;

typedef struct tMeshTri {
	ushort		nPass;
	int			nFace;
	int			nIndex;
	int			lines [3];
	ushort		index [3];
	tTexCoord2f	texCoord [3];
	tTexCoord2f	ovlTexCoord [3];
	tRgbaColorf	color [3];
} tMeshTri;

class CTriMeshBuilder {
	private:
		tMeshLine	*m_meshLines;
		tMeshTri		*m_meshTris;
		int			m_nMeshLines;
		int			m_nMeshTris;
		int			m_nMaxMeshTris;
		int			m_nMaxMeshLines;
		int			m_nVertices;

	private:
		void FreeMeshData (void);
		int AllocMeshData (void);
		tMeshLine *FindMeshLine (ushort nVert1, ushort nVert2);
		int AddMeshLine (int nTri, ushort nVert1, ushort nVert2);
		tMeshTri *CreateMeshTri (tMeshTri *mtP, ushort index [], int nFace, int nIndex);
		int AddMeshTri (tMeshTri *mtP, ushort index [], grsTriangle *triP);
		void DeleteMeshLine (tMeshLine *mlP);
		void DeleteMeshTri (tMeshTri *mtP);
		int CreateMeshTris (void);
		int SplitMeshTriByLine (int nTri, ushort nVert1, ushort nVert2, ushort nPass);
		int SplitMeshLine (tMeshLine *mlP, ushort nPass);
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