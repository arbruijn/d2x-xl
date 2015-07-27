#ifndef THRUSTERFLAMES_H
#define THRUSTERFLAMES_H

#include "descent.h"

// -----------------------------------------------------------------------------

#define	THRUSTER_SEGS	14	// number of rings the thruster flame is composed of
#define	RING_SEGS		16	// number of segments each ring is composed of

#define	FLAME_VERT_COUNT	((THRUSTER_SEGS - 1) * (RING_SEGS + 1) * 2)

typedef struct tThrusterInfo {
	CFixVector			vPos [MAX_THRUSTERS];
	CFixVector			vDir [MAX_THRUSTERS];
	CFixMatrix			mRot [MAX_THRUSTERS];
	float					fSize [MAX_THRUSTERS];
	float					fLength [MAX_THRUSTERS];
	ubyte					nType [MAX_THRUSTERS];
	float					fScale;
	tPathPoint*			pp;
	CModelThrusters*	mtP;
	CFixMatrix			mOrient;
}  tThrusterInfo;


class CThrusterFlames {
	private:
		CFloatVector	m_vFlame [THRUSTER_SEGS][RING_SEGS];
		CFloatVector	m_flameVerts [FLAME_VERT_COUNT];
		tTexCoord2f		m_flameTexCoord [2][FLAME_VERT_COUNT];
		CThrusterData*	m_pt;
		tThrusterInfo	m_ti;
		int				m_nVerts;
		int				m_nThrusters;
		int				m_nStyle;
		bool				m_bHaveFlame [2];
		bool				m_bPlayer;
		bool				m_bSpectate;

	public:
		CThrusterFlames () { m_bHaveFlame [0] = m_bHaveFlame [1] = false; }
		void Render (CObject *objP, tThrusterInfo* infoP = NULL, int nThruster = -1);
		void Render2D (CFixVector& vPos, CFixVector &vDir, float fSize, float fLength, CFloatVector *colorP);
		int CalcPos (CObject *objP, tThrusterInfo* tiP = NULL, int bAfterburnerBlob = 0);

	private:
		void Create (void);
		void CalcPosOnShip (CObject *objP, CFixVector *vPos);
		void RenderCap (int i);
		void Render3D (int i);
		bool Setup (CObject *objP, int nStages = 3);
		bool IsFiring (CWeaponState* ws, int i);
};

extern CThrusterFlames thrusterFlames;

//------------------------------------------------------------------------------

#endif //THRUSTERFLAMES_H

//eof
