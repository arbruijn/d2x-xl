#ifndef THRUSTERFLAMES_H
#define THRUSTERFLAMES_H

#include "descent.h"

// -----------------------------------------------------------------------------

#define	THRUSTER_SEGS	14	// number of rings the thruster flame is composed of
#define	RING_SEGS		16	// number of segments each ring is composed of

#define	FLAME_VERT_COUNT	((THRUSTER_SEGS - 1) * (RING_SEGS + 1) * 2)

typedef struct tThrusterInfo {
	CFixVector			vPos [2];
	CFixVector			vDir [2];
	float					fSize;
	float					fLength;
	tPathPoint*			pp;
	CModelThrusters*	mtP;
}  tThrusterInfo;


class CThrusterFlames {
	private:
		CFloatVector	m_vFlame [THRUSTER_SEGS][RING_SEGS];
		CFloatVector	m_flameVerts [FLAME_VERT_COUNT];
		tTexCoord2f		m_flameTexCoord [FLAME_VERT_COUNT];
		CThrusterData*	m_pt;
		tThrusterInfo	m_ti;
		int				m_nVerts;
		int				m_nThrusters;
		int				m_nStyle;
		bool				m_bHaveFlame;
		bool				m_bPlayer;
		bool				m_bSpectate;

	public:
		CThrusterFlames () { m_bHaveFlame = false; }
		void Render (CObject *objP);
		void Render2D (CFixVector& vPos, CFixVector &vDir, float fSize, float fLength, CBitmap *bmP, tRgbaColorf *colorP);
		int CalcPos (CObject *objP, tThrusterInfo* tiP = NULL, int bAfterburnerBlob = 0);

	private:
		void Create (void);
		void CalcPosOnShip (CObject *objP, CFixVector *vPos);
		void RenderCap (void);
		void Render3D (void);
		bool Setup (CObject *objP);
};

extern CThrusterFlames thrusterFlames;

//------------------------------------------------------------------------------

#endif //THRUSTERFLAMES_H

//eof
