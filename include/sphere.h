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
		int32_t					m_nRings;
		int32_t					m_nFaces;
		int32_t					m_nFrame;
		tTexCoord2f				m_texCoord;
		CPulseData				m_pulse;
		CPulseData*				m_pPulse;
		CBitmap*					m_pBm;
		CFloatVector			m_color;

	public:
		CSphereData () { Init (); };
		void Init (void);
};

// -----------------------------------------------------------------------------

class CSphereVertex {
	public:
		static float	m_fNormRadScale;

		CFloatVector	m_v;
		tTexCoord2f		m_tc;

		inline CSphereVertex& operator= (CSphereVertex& other) {
			m_v = other.m_v;
			m_tc.v.u = other.m_tc.v.u;
			m_tc.v.v = other.m_tc.v.v;
			return *this;
			}

		inline CSphereVertex& operator+= (CSphereVertex& other) {
			m_v += other.m_v;
			m_tc.v.u += other.m_tc.v.u;
			m_tc.v.v += other.m_tc.v.v;
			return *this;
			}

		inline CSphereVertex& operator-= (CSphereVertex& other) {
			m_v -= other.m_v;
			m_tc.v.u -= other.m_tc.v.u;
			m_tc.v.v -= other.m_tc.v.v;
			return *this;
			}

		inline CSphereVertex operator+ (CSphereVertex& other) {
			CSphereVertex v = *this;
			v += other;
			return v;
			}

		inline CSphereVertex operator- (CSphereVertex& other) {
			CSphereVertex v = *this;
			v -= other;
			return v;
			}

		inline CSphereVertex& operator*= (float s) {
			m_v *= s;
			m_tc.v.u *= s;
			m_tc.v.v *= s;
			return *this;
			}

		inline CSphereVertex operator* (float s) {
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
		void ComputeNormal (CSphere *pSphere);

		CFloatVector& Center (void) { return m_vCenter.m_v; }
		CFloatVector& Normal (void) { return m_vNormal.m_v; }

		//virtual CFloatVector& Vertex (int32_t i) = 0;
	};

// -----------------------------------------------------------------------------

class CSphereTriangle : public CSphereFace {
	public:
		int16_t			m_v [3];

		CSphereVertex *ComputeCenter (CSphere *pSphere);
		CSphereTriangle *Split (CSphere *pSphere, CSphereTriangle *pDest);
		void Transform (CSphereTriangle *pDest);
		//virtual CFloatVector& Vertex (int32_t i) { return m_v [i].m_v; }
	};

// -----------------------------------------------------------------------------

class CSphereQuad : public CSphereFace {
	public:
		int16_t			m_v [4];

		CSphereVertex *ComputeCenter (CSphere *pSphere);
		CSphereQuad *Split (CSphere *pSphere, CSphereQuad *pDest);
		void Transform (CSphereQuad *pDest);
		//virtual CFloatVector& Vertex (int32_t i) { return m_v [i].m_v; }
	};

// -----------------------------------------------------------------------------

class CSphere : protected CSphereData {
	public:
		CArray<CSphereVertex>	m_worldVerts;
		CArray<int16_t>			m_vertexIndex;
		int32_t						m_nVertices;
		int32_t						m_nFaces;

	public:
		CSphere () : m_nVertices (0), m_nFaces (0) {}
		virtual ~CSphere () { Destroy (); }
		void Init () { CSphereData::Init (); }
		virtual void Destroy ();
		int32_t Render (CObject* pObj, CFloatVector *pPos, float xScale, float yScale, float zScale,
							 float red, float green, float blue, float alpha, CBitmap *pBm, int32_t nTiles, char bAdditive);
		inline CPulseData* Pulse (void) { return m_pPulse ? m_pPulse : &m_pulse; }
		void SetPulse (CPulseData* pPulse);
		void SetupPulse (float fSpeed, float fMin);

		virtual int32_t Create (void) = 0;
		virtual void RenderFaces (float fRadius, float red, float green, float blue, float alpha, int32_t bTextured, int32_t nFaces) = 0;

		inline CSphereVertex& Vertex (int16_t i) { return m_worldVerts [i]; }
		int16_t AddVertex (CSphereVertex& v);

		virtual int32_t Quality (void) = 0;
		virtual void SetQuality (int32_t nQuality) = 0;
		virtual int32_t HasQuality (int32_t nDesiredQuality) = 0;
		virtual int32_t FaceNodes (void) = 0;

	protected:
		void DrawFaces (CFloatVector *pVertex, tTexCoord2f *pTexCoord, int32_t nFaces, int32_t bTextured, int32_t nPrimitive);
		void DrawFaces (int32_t nOffset, int32_t nFaces, int32_t bTextured, int32_t nPrimitive);

	private:
		void Pulsate (void);
		void Animate (CBitmap* pBm);
		int32_t InitSurface (float red, float green, float blue, float alpha, CBitmap *pBm, float fScale);
		int16_t FindVertex (CSphereVertex& v);
};

// -----------------------------------------------------------------------------

class CRingedSphere : public CSphere {
	public:
		virtual void RenderFaces (float fRadius, float red, float green, float blue, float alpha, int32_t bTextured, int32_t nTiles);
		virtual int32_t Create (void);
		virtual int32_t Quality (void) { return 32; }
		virtual void SetQuality (int32_t nQuality) {}
		virtual int32_t HasQuality (int32_t nDesiredQuality) { return 1; }
		virtual int32_t FaceNodes (void) { return 3; }

	private:
		int32_t CreateVertices (int32_t nRings = 32, int32_t nTiles = 1);

	};

// -----------------------------------------------------------------------------

class CTesselatedSphere : public CSphere {
	public:
		CArray<CSphereVertex>	m_viewVerts;
		int32_t						m_nQuality;

		CTesselatedSphere () : m_nQuality (0) {}
		virtual ~CTesselatedSphere () { Destroy (); }
		virtual void Destroy ();
		virtual int32_t Create (void) = 0;
		void Transform (float fScale);
		virtual void SetQuality (int32_t nQuality) { m_nQuality = nQuality; }
		virtual int32_t HasQuality (int32_t nDesiredQuality) { return m_nQuality = nDesiredQuality; }
		virtual int32_t FaceNodes (void) = 0;

	protected:
		int32_t Quality (void);
	};

// -----------------------------------------------------------------------------

class CTriangleSphere : public CTesselatedSphere {
	public:
		virtual void RenderFaces (float fRadius, float red, float green, float blue, float alpha, int32_t bTextured, int32_t nTiles);
		virtual int32_t Create (void);
		virtual int32_t FaceNodes (void) { return 3; }

	protected:
		CArray< CSphereTriangle >	m_faces [2];

		int32_t Tesselate (CSphereTriangle *pSrc, CSphereTriangle *pDest, int32_t nFaces);
		void SetupFaces (void);
		int32_t CreateBuffers (void);
		int32_t CreateFaces (void);
	};

// -----------------------------------------------------------------------------

class CQuadSphere : public CTesselatedSphere {
	public:
		virtual void RenderFaces (float fRadius, float red, float green, float blue, float alpha, int32_t bTextured, int32_t nTiles);
		virtual int32_t Create (void);
		virtual int32_t FaceNodes (void) { return 4; }

	protected:
		CArray< CSphereQuad >	m_faces [2];

		int32_t Tesselate (CSphereQuad *pSrc, CSphereQuad *pDest, int32_t nFaces);
		void SetupFaces (void);
		int32_t CreateBuffers (void);
		int32_t CreateFaces (void);
	};

// -----------------------------------------------------------------------------

void InitSpheres (void);
void ResetSphereShaders (void);
void SetupSpherePulse (CPulseData *pPulse, float fSpeed, float fMin);
int32_t CreateShieldSphere (void);
int32_t DrawShieldSphere (CObject *pObj, float red, float green, float blue, float alpha, char bAdditive, fix nSize = 0);
void DrawMonsterball (CObject *pObj, float red, float green, float blue, float alpha);

// -----------------------------------------------------------------------------

#endif //__SPHERE_H

//eof
