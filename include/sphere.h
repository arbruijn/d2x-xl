#ifndef __SPHERE_H
#define __SPHERE_H

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "oof.h"

void InitSpheres (void);
void ResetSphereShaders (void);

#define RINGED_SPHERE	1

class CSphereData {
public:
		int32_t					m_nRings;
		int32_t					m_nTiles;
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

class CSphere : private CSphereData {
	private:

		typedef struct tSphereVertex {
			CFloatVector	vPos;
			tTexCoord2f		uv;
			} tSphereVertex;

		CArray<tSphereVertex>	m_vertices;
		int32_t						m_nVertices;

	public:
		CSphere () {}
		~CSphere () {}
		void Init () { CSphereData::Init (); }
		void Destroy ();
		int32_t Create (int32_t nRings = 32, int32_t nTiles = 1);
		int32_t Render (CObject* pObj, CFloatVector *pPos, float xScale, float yScale, float zScale,
						float red, float green, float blue, float alpha, CBitmap *pBm, int32_t nTiles, char bAdditive);
		inline CPulseData* Pulse (void) { return m_pPulse ? m_pPulse : &m_pulse; }
		void SetPulse (CPulseData* pPulse);
		void SetupPulse (float fSpeed, float fMin);

	private:
		void Pulsate (void);
		void Animate (CBitmap* pBm);
		int32_t InitSurface (float red, float green, float blue, float alpha, CBitmap *pBm, float fScale);
		void RenderRing (CFloatVector *pVertex, tTexCoord2f *pTexCoord, int32_t nItems, int32_t bTextured, int32_t nPrimitive);
		void RenderRing (int32_t nOffset, int32_t nItems, int32_t bTextured, int32_t nPrimitive);
		void RenderRings (float fRadius, int32_t nRings, float red, float green, float blue, float alpha, int32_t bTextured, int32_t nTiles);
};

void SetupSpherePulse (CPulseData *pPulse, float fSpeed, float fMin);

int32_t CreateShieldSphere (void);
int32_t DrawShieldSphere (CObject *pObj, float red, float green, float blue, float alpha, char bAdditive, fix nSize = 0);
void DrawMonsterball (CObject *pObj, float red, float green, float blue, float alpha);

#endif //__SPHERE_H

//eof
