#ifndef __LIGHTNING_H
#define __LIGHTNING_H

#include "descent.h"
#include "u_mem.h"

#define MAX_LIGHTNING_SYSTEMS	1000
#define MAX_LIGHTNING_NODES	1000

//------------------------------------------------------------------------------

class CLightning;

typedef struct tLightningNode {
	CLightning			*m_child;
	CFixVector			m_vPos;
	CFixVector			m_vNewPos;
	CFixVector			m_vOffs;
	CFixVector			m_vBase;
	CFixVector			m_vDelta [2];
	int					m_nDelta [2];
} tLightningNode;

class CLightningNode : public tLightningNode {
	public:
		CLightningNode () { m_child = NULL; };
		~CLightningNode () { Destroy (); };
		CFixVector *Create (CFixVector *vOffs, CFixVector *vAttract, int nDist);
		void Destroy (void);
		void Setup (bool bInit, CFixVector *vPos, CFixVector *vDelta);
		void Animate (bool bInit, short nSegment, int nDepth);
		bool CreateChild (CFixVector *vEnd, CFixVector *vDelta,
							   int nLife, int nLength, int nAmplitude,
							   char nAngle, short nNodes, short nChildren, char nDepth, short nSteps,
							   short nSmoothe, char bClamp, char bPlasma, char bLight,
							   char nStyle, tRgbaColorf *colorP, CLightning *parentP, short nNode);
		void ComputeOffset (int nSteps);
		int ComputeAttractor (CFixVector *vAttract, CFixVector *vDest, CFixVector *vPos, int nMinDist, int i);
		int Clamp (CFixVector *vPos, CFixVector *vBase, int nAmplitude);
		CFixVector *Smoothe (CFixVector *vOffs, CFixVector *vPrevOffs, int nDist, int nSmoothe);
		CFixVector *Attract (CFixVector *vOffs, CFixVector *vAttract, CFixVector *vPos, int nDist, int i, int bJoinPaths);
		CFixVector CreateJaggy (CFixVector *vPos, CFixVector *vDest, CFixVector *vBase, CFixVector *vPrevOffs,
									  int nSteps, int nAmplitude, int nMinDist, int i, int nSmoothe, int bClamp);
		CFixVector CreateErratic (CFixVector *vPos, CFixVector *vBase, int nSteps, int nAmplitude,
									    int bInPlane, int bFromEnd, int bRandom, int i, int nNodes, int nSmoothe, int bClamp);
		CFixVector CreatePerlin (int nSteps, int nAmplitude, int *nSeed, double phi, double i);
		void Move (CFixVector& vDelta, short nSegment);
		bool SetLight (short nSegment, tRgbaColorf *colorP);
		inline CLightning *GetChild (void) { return m_child; }
		inline void GetChild (CLightning * child) { m_child = child; }
};

//------------------------------------------------------------------------------

typedef struct tLightning {
	CLightning*					m_parent;
	CFixVector					m_vBase;
	CFixVector					m_vPos;
	CFixVector					m_vEnd;
	CFixVector					m_vDir;
	CFixVector					m_vRefEnd;
	CFixVector					m_vDelta;
	CArray<CLightningNode>	m_nodes;
	tRgbaColorf					m_color;
	int							m_nNext;
	int							m_nLife;
	int							m_nTTL;
	int							m_nDelay;
	int							m_nLength;
	int							m_nOffset;
	int							m_nAmplitude;
	short							m_nSegment;
	short							m_nSmoothe;
	short							m_nSteps;
	short							m_iStep;
	short							m_nNodes;
	short							m_nChildren;
	short							m_nObject;
	short							nSegment;
	short							m_nNode;
	char							m_nStyle;
	char							m_nAngle;
	char							m_nDepth;
	char							m_bClamp;
	char							m_bPlasma;
	char							m_bRandom;
	char							m_bLight;
	char							m_bInPlane;
} tLightning;

class CLightning : public tLightning {
	public:
		CLightning () { m_parent = NULL, m_nodes = NULL, m_nNodes = 0; };
		~CLightning () { Destroy (); };
		bool Create (char nDepth);
		void Init (CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
					  short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
					  char nAngle, int nOffset, short nNodes, short nChildren, short nSteps,
					  short nSmoothe, char bClamp, char bPlasma, char bLight,
					  char nStyle, tRgbaColorf *colorP, CLightning *parentP, short nNode);
		void Setup (bool bInit);
		void Destroy (void);
		void DestroyNodes (void);
		void Smoothe (void);
		void ComputeOffsets (void);
		void Bump (void);
		int SetLife (void);
		void Animate (int nDepth);
		int Update (int nDepth);
		void Move (CFixVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd);
		void Render (int nDepth, int bDepthSort, int nThread);
		int SetLight (void);
		inline int MayBeVisible (void);
		CLightning& operator= (CLightning& source) { 
			memcpy (this, &source, sizeof (CLightning)); 
			return *this;
			}
	private:
		void CreatePath (int bSeed, int nDepth);
		int ComputeChildEnd (CFixVector *vPos, CFixVector *vEnd, CFixVector *vDir, CFixVector *vParentDir, int nLength);
		void ComputePlasmaSegment (CFloatVector *vPosf, int bScale, short nSegment, char bStart, char bEnd, int nDepth, int nThread);
		void ComputePlasma (int nDepth, int nThread);
		void RenderCore (tRgbaColorf *colorP, int nDepth, int nThread);
		int SetupPlasma (void);
		void RenderPlasma (tRgbaColorf *colorP, int nThread);
		void RenderBuffered (int nDepth, int nThread);
};

//------------------------------------------------------------------------------

typedef struct tLightningSystem {
	int						m_nId;
	CArray<CLightning>	m_lightnings;
	int						m_nLightnings;
	short						m_nSegment [2];
	short						m_nObject;
	int						m_nKey [2];
	time_t					m_tUpdate;
	int						m_nSound;
	char						m_bSound;
	char						m_bForcefield;
	char						m_bDestroy;
	char						m_bValid;
} tLightningSystem;

class CLightningSystem : public tLightningSystem {
	public:
		CLightningSystem () { m_bValid = 0, m_lightnings = NULL, m_nLightnings = 0, m_nObject = -1; };
		~CLightningSystem () { Destroy (); };
		void Init (int nId);
		bool Create (int nLightnings, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
						 short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
						 short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
						 char bClamp, char bPlasma, char bSound, char bLight, char nStyle, tRgbaColorf *colorP);
		void Destroy (void);
		void Animate (int nStart, int nLightnings);
		void Render (int nStart, int nLightnings, int bDepthSort, int nThread);
		int Update (void);
		void Move (CFixVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd);
		int SetLife (void);
		int SetLight (void);
		inline CLightning* Lightnings (void) { return m_lightnings.Buffer (); }
		inline int Id (void) { return m_nId; }
		inline void SetValid (char bValid) { m_bValid = bValid; }
	private:
		void CreateSound (int bSound);
		void DestroySound (void);
		void UpdateSound (void);
		void MoveForObject (void);
		void RenderBuffered (int nStart, int nLightnings, int nThread);
};

//------------------------------------------------------------------------------

typedef struct tLightningLight {
	CFixVector		vPos;
	tRgbaColorf		color;
	int				nNext;
	int				nLights;
	int				nBrightness;
	int				nDynLight;
	short				nSegment;
	int				nFrame;
} tLightningLight;


typedef struct tLightningData {
	CArray<short>						m_objects;
	CArray<tLightningLight>			m_lights;
	CDataPool<CLightningSystem>	m_systems; // [MAX_LIGHTNINGS];
	int									m_bDestroy;
	int									m_nFirstLight;
} tLightningData;

class CLightningManager : public tLightningData {
	public:
		int	m_bDestroy;

	public:
		CLightningManager ();
		~CLightningManager ();
		void Init (void);
		int Create (int nLightnings, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
						short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
						short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
						char bClamp, char bPlasma, char bSound, char bLight, char nStyle, tRgbaColorf *colorP);
		void Destroy (CLightningSystem* systemP, CLightning *lightningP);
		void Cleanup (void);
		int Shutdown (bool bForce);
		void Render (void);
		void Update (void);
		void Move (int i, CFixVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd);
		void MoveForObject (CObject *objP);
		void Render (tLightning *pl, int nLightnings, short nDepth, int bDepthSort);
		void RenderBuffered (tLightning *plRoot, int nStart, int nLightnings, int nDepth, int nThread);
		void RenderSystem (void);
		void RenderForDamage (CObject *objP, g3sPoint **pointList, RenderModel::CVertex *pVerts, int nVertices);
		void Animate (tLightning *pl, int nStart, int nLightnings, int nDepth);
		int CreateForMissile (CObject *objP);
		void CreateForShaker (CObject *objP);
		void CreateForShakerMega (CObject *objP);
		void CreateForMega (CObject *objP);
		void CreateForBlowup (CObject *objP);
		void CreateForDamage (CObject *objP, tRgbaColorf *colorP);
		void CreateForRobot (CObject *objP, tRgbaColorf *colorP);
		void CreateForPlayer (CObject *objP, tRgbaColorf *colorP);
		void CreateForExplosion (CObject *objP, tRgbaColorf *colorP, int nRods, int nRad, int nTTL);
		void DestroyForObject (CObject *objP);
		void DestroyForAllObjects (int nType, int nId);
		void DestroyForPlayers (void);
		void DestroyForRobots (void);
		void DestroyStatic (void);
		void SetLights (void);
		void ResetLights (int bForce);
		void DoFrame (void);
		void StaticFrame (void);
		int FindDamageLightning (short nObject, int *pKey);
		void SetSegmentLight (short nSegment, CFixVector *vPosP, tRgbaColorf *colorP);
		tRgbaColorf *LightningColor (CObject *objP);
		inline short GetObjectSystem (short nObject) { return (m_objects.Buffer () && (nObject >= 0)) ? m_objects [nObject] : -1; }
		inline void SetObjectSystem (short nObject, int i) { if (m_objects.Buffer () && (nObject >= 0)) m_objects [nObject] = i; }
		inline tLightningLight* GetLight (short nSegment) { return m_lights + nSegment; }

	private:
		CFixVector *FindTargetPos (CObject *emitterP, short nTarget);

};

extern CLightningManager lightningManager;

//------------------------------------------------------------------------------

typedef struct tOmegaLightningHandles {
	int		nLightning;
	short		nParentObj;
	short		nTargetObj;
} tOmegaLightningHandles;


class COmegaLightnings {
	private:
		tOmegaLightningHandles	m_handles [MAX_OBJECTS_D2X];
		int							m_nHandles;

	public:
		COmegaLightnings () { m_nHandles = 0; };
		~COmegaLightnings () {};
		void Init (void) { m_nHandles = 0; };
		int Create (CFixVector *vTargetPos, CObject *parentObjP, CObject *targetObjP);
		int Update (CObject *parentObjP, CObject *targetObjP);
		void Destroy (short nObject);
		inline bool Exist (void) { return m_nHandles > 0; }

	private:
		int Find (short nObject);
		void Delete (short nHandle);
		CFixVector *GetGunPoint (CObject *objP, CFixVector *vMuzzle);
};

extern COmegaLightnings	omegaLightnings;

//------------------------------------------------------------------------------

#define	SHOW_LIGHTNINGS \
			(!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bUseLightning, 1, 1, 0))

#endif //__LIGHTNING_H
