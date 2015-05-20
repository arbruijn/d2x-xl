#ifndef __SPHERE_H
#define __SPHERE_H

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

// -----------------------------------------------------------------------------

#define SPHERE_TYPE_RINGS		1
#define SPHERE_TYPE_TRIANGLES	2 
#define SPHERE_TYPE_QUADS		3

#define SPHERE_TYPE				SPHERE_TYPE_QUADS

class CSphere;

// -----------------------------------------------------------------------------

class CSphereData {
public:
		CFloatVector			m_color;
		tTexCoord2f				m_texCoord;
		CPulseData				m_pulse;
		CPulseData*				m_pPulse;
		CBitmap*					m_pBitmap;
		int32_t					m_nRings;
		int32_t					m_nFaces;
		int32_t					m_nFrame;

	public:
		CSphereData () : m_pPulse (NULL), m_pBitmap (NULL), m_nRings (0), m_nFaces (0), m_nFrame (0) { Init (); };
		void Init (void);
};

// -----------------------------------------------------------------------------

class CSphereVertex {
	public:
		static float	m_fNormRadScale;

		CFloatVector	m_v;
		tTexCoord2f		m_tc;
		CFloatVector	m_c;

		inline CSphereVertex& operator= (const CSphereVertex& other) {
			m_v = other.m_v;
			m_tc.v.u = other.m_tc.v.u;
			m_tc.v.v = other.m_tc.v.v;
			return *this;
			}

		inline CSphereVertex& operator+= (const CSphereVertex& other) {
			m_v += other.m_v;
			m_tc.v.u += other.m_tc.v.u;
			m_tc.v.v += other.m_tc.v.v;
			return *this;
			}

		inline CSphereVertex& operator-= (const CSphereVertex& other) {
			m_v -= other.m_v;
			m_tc.v.u -= other.m_tc.v.u;
			m_tc.v.v -= other.m_tc.v.v;
			return *this;
			}

		inline const CSphereVertex operator+ (const CSphereVertex& other) const {
			CSphereVertex v = *this;
			v += other;
			return v;
			}

		inline const CSphereVertex operator- (const CSphereVertex& other) const {
			CSphereVertex v = *this;
			v -= other;
			return v;
			}

#if 0 //ndef _WIN32
		inline const CSphereVertex operator+ (const CSphereVertex other) const {
			CSphereVertex v = *this;
			v += other;
			return v;
			}

		inline const CSphereVertex operator- (const CSphereVertex other) const {
			CSphereVertex v = *this;
			v -= other;
			return v;
			}
#endif

		inline CSphereVertex& operator*= (const float s) {
			m_v *= s;
			m_tc.v.u *= s;
			m_tc.v.v *= s;
			return *this;
			}

		inline const CSphereVertex operator* (const float s) const {
			CSphereVertex v = *this;
			v *= s;
			return v;
			}

		void Normalize ();
	};

// -----------------------------------------------------------------------------

class CSphereFace {
	public:	
		CSphereVertex	m_vCenter;
		CSphereVertex	m_vNormal;

		void Normalize (CFloatVector& v);
		void ComputeNormal (void);

		CFloatVector& Center (void) { return m_vCenter.m_v; }
		CFloatVector& Normal (void) { return m_vNormal.m_v; }

		virtual CSphereVertex* Vertices (void) = 0;
		virtual CFloatVector& Vertex (int32_t i) = 0;
		virtual CSphereVertex *ComputeCenter (void) = 0;
	};

// -----------------------------------------------------------------------------

class CSphereTriangle : public CSphereFace {
	public:
		CSphereVertex			m_v [3];

		virtual CSphereVertex *ComputeCenter (void);
		CSphereTriangle *Split (CSphereTriangle *pDest);
		void Transform (CSphereTriangle *pDest);
		virtual CSphereVertex* Vertices (void) { return m_v; }
		virtual CFloatVector& Vertex (int32_t i) { return m_v [i].m_v; }
	};

// -----------------------------------------------------------------------------

class CSphereQuad : public CSphereFace {
	public:
		CSphereVertex			m_v [4];

		virtual CSphereVertex *ComputeCenter (void);
		CSphereQuad *Split (CSphereQuad *pDest);
		void Transform (CSphereQuad *pDest);
		virtual CSphereVertex* Vertices (void) { return m_v; }
		virtual CFloatVector& Vertex (int32_t i) { return m_v [i].m_v; }
	};

// -----------------------------------------------------------------------------

class CSphere : protected CSphereData {
	public:
		CArray<CSphereVertex>	m_worldVerts;
		int32_t						m_nVertices;
		int32_t						m_nFaces;

	public:
		CSphere () : m_nVertices (0), m_nFaces (0) {}
		virtual ~CSphere () { Destroy (); }
		void Init () { CSphereData::Init (); }
		virtual void Destroy ();
		inline CBitmap *SetBitmap (CBitmap *pBitmap) {
			CBitmap *pOldBitmap = m_pBitmap;
			m_pBitmap = pBitmap;
			return pOldBitmap;
			}
		inline CBitmap *GetBitmap (void) { return m_pBitmap; }
		inline CPulseData* Pulse (void) { return &m_pulse; }
		CPulseData *SetPulse (CPulseData* pPulse);
		inline CPulseData *GetPulse (void) { return m_pPulse ? m_pPulse : &m_pulse; }
		void SetupPulse (float fSpeed = 0.02f, float fMin = 0.5f);
		CPulseData *SetupSurface (CPulseData *pPulse, CBitmap *pBitmap);
		
		int32_t Render (CObject* pObj, CFloatVector *pPos, float xScale, float yScale, float zScale, float red, float green, float blue, float alpha, int32_t nTiles, char bAdditive);

		virtual int32_t Create (void) = 0;
		virtual void RenderFaces (float fRadius, int32_t nFaces, int32_t bTextured, int32_t bEffect) = 0;

		inline CSphereVertex& Vertex (int16_t i) { return m_worldVerts [i]; }
		int16_t AddVertex (CSphereVertex& v);

		virtual int32_t Quality (void) = 0;
		virtual void SetQuality (int32_t nQuality) = 0;
		virtual int32_t HasQuality (int32_t nDesiredQuality) = 0;
		virtual int32_t FaceNodes (void) = 0;
		virtual void RenderOutline (CObject *pObj, float fScale = 1.0f) = 0;

	protected:
		void DrawFaces (CFloatVector *pVertex, tTexCoord2f *pTexCoord, int32_t nFaces, int32_t nClientArrays, int32_t nPrimitive, int32_t nState);
		void DrawFaces (int32_t nOffset, int32_t nFaces, int32_t bTextured, int32_t nPrimitive, int32_t nState);

	private:
		void Pulsate (void);
		void Animate (CBitmap* pBm);
		int32_t InitSurface (float red, float green, float blue, float alpha, float fScale);
		int16_t FindVertex (CSphereVertex& v);
};

// -----------------------------------------------------------------------------

class CRingedSphere : public CSphere {
	public:
		virtual void RenderFaces (float fRadius, int32_t nFaces, int32_t bTextured, int32_t bEffect);
		virtual int32_t Create (void);
		virtual int32_t Quality (void) { return 32; }
		virtual void SetQuality (int32_t nQuality) {}
		virtual int32_t HasQuality (int32_t nDesiredQuality) { return 1; }
		virtual int32_t FaceNodes (void) { return 3; }
		virtual void RenderOutline (CObject *pObj, float fScale = 1.0f) {}

	private:
		int32_t CreateVertices (int32_t nRings = 32, int32_t nTiles = 1);

	};

// -----------------------------------------------------------------------------

class CSphereEdge : public CMeshEdge {
	public:
		inline bool operator== (CSphereEdge& other) {
			return ((CFloatVector::Dist (m_vertices [0][0], other.m_vertices [0][0]) < 1e-6f) && (CFloatVector::Dist (m_vertices [1][0], other.m_vertices [1][0]) < 1e-6f)) ||
					 ((CFloatVector::Dist (m_vertices [0][0], other.m_vertices [1][0]) < 1e-6f) && (CFloatVector::Dist (m_vertices [1][0], other.m_vertices [0][0]) < 1e-6f));
			}
		virtual void Transform (float fScale);
		int32_t Prepare (CFloatVector vViewer, int32_t nFilter = 2, float fScale = -1.0f);
	};

// -----------------------------------------------------------------------------

class CTesselatedSphere : public CSphere {
	public:
		CArray<CSphereVertex>	m_viewVerts;
		CArray<CSphereEdge>		m_edges;
		int32_t						m_nEdges;
		int32_t						m_nQuality;
		int32_t						m_nFaceBuffer;

		CTesselatedSphere () : m_nQuality (0) {}
		virtual ~CTesselatedSphere () { Destroy (); }
		virtual void Destroy ();
		virtual int32_t Create (void) = 0;
		virtual void SetQuality (int32_t nQuality) { m_nQuality = nQuality; }
		virtual int32_t HasQuality (int32_t nDesiredQuality) { return m_nQuality == nDesiredQuality; }
		virtual int32_t FaceNodes (void) = 0;
		virtual CSphereFace *Face (int32_t nFace) = 0;
		virtual void RenderOutline (CObject *pObj, float fScale = 1.0f);
		int32_t SetupColor (float fRadius);
		void Transform (float fScale);

	protected:
		int32_t Quality (void);
		virtual int32_t CreateEdgeList (void) = 0;
		int32_t FindEdge (CFloatVector& v1, CFloatVector& v2);
		int32_t AddEdge (CFloatVector& v1, CFloatVector& v2, CFloatVector& vCenter);
	};

// -----------------------------------------------------------------------------

class CTriangleSphere : public CTesselatedSphere {
	public:
		virtual void RenderFaces (float fRadius, int32_t nFaces, int32_t bTextured, int32_t bEffect);
		virtual int32_t Create (void);
		virtual int32_t FaceNodes (void) { return 3; }

	protected:
		CArray< CSphereTriangle >	m_faces [2];

		int32_t Tesselate (CSphereTriangle *pSrc, CSphereTriangle *pDest, int32_t nFaces);
		void SetupFaces (void);
		int32_t CreateBuffers (void);
		int32_t CreateFaces (void);
		virtual int32_t CreateEdgeList (void);
		virtual CSphereFace *Face (int32_t nFace) { return m_faces [m_nFaceBuffer] + nFace; }
	};

// -----------------------------------------------------------------------------

class CQuadSphere : public CTesselatedSphere {
	public:
		virtual void RenderFaces (float fRadius, int32_t nFaces, int32_t bTextured, int32_t bEffect);
		virtual int32_t Create (void);
		virtual int32_t FaceNodes (void) { return 4; }

	protected:
		CArray< CSphereQuad >	m_faces [2];

		int32_t Tesselate (CSphereQuad *pSrc, CSphereQuad *pDest, int32_t nFaces);
		void SetupFaces (void);
		int32_t CreateBuffers (void);
		int32_t CreateFaces (void);
		virtual int32_t CreateEdgeList (void);
		virtual CSphereFace *Face (int32_t nFace) { return m_faces [m_nFaceBuffer] + nFace; }
	};

// -----------------------------------------------------------------------------

void InitSpheres (void);
void ResetSphereShaders (void);
int32_t CreateShieldSphere (void);
int32_t DrawShieldSphere (CObject *pObj, float red, float green, float blue, float alpha, char bAdditive, fix nSize = 0);
void DrawMonsterball (CObject *pObj, float red, float green, float blue, float alpha);

// -----------------------------------------------------------------------------

#endif //__SPHERE_H

//eof
