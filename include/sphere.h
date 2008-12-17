#ifndef __SPHERE_H
#define __SPHERE_H

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "oof.h"

void InitSpheres (void);

#define RINGED_SPHERE	1

class CSphereData {
public:
		int						m_nRings;
		int						m_nTiles;
		CPulseData				m_pulse;
		CPulseData*				m_pulseP;

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
		int							m_nVertices;

	public:
		CSphere () {}
		~CSphere () {}
		void Init () { CSphereData::Init (); }
		void Destroy ();
		int Create (int nRings = 32, int nTiles = 1);
		int Render (CFloatVector *pPos, float xScale, float yScale, float zScale,
						float red, float green, float blue, float alpha, CBitmap *bmP, int nTiles, int bAdditive);
		inline CPulseData* Pulse (void) { return m_pulseP ? m_pulseP : &m_pulse; }
		void SetPulse (CPulseData* pulseP);
		void SetupPulse (float fSpeed, float fMin);

	private:
		int InitSurface (float red, float green, float blue, float alpha, CBitmap *bmP, float *pfScale);
		void RenderRing (CFloatVector *vertexP, tTexCoord2f *texCoordP, int nItems, int bTextured, int nPrimitive);
		void RenderRing (int nOffset, int nItems, int bTextured, int nPrimitive);
		void RenderRings (float fRadius, int nRings, float red, float green, float blue, float alpha, int bTextured, int nTiles);
};

void SetupSpherePulse (CPulseData *pulseP, float fSpeed, float fMin);

int CreateShieldSphere (void);
void DrawShieldSphere (CObject *objP, float red, float green, float blue, float alpha);
void DrawMonsterball (CObject *objP, float red, float green, float blue, float alpha);

#endif //__SPHERE_H

//eof
