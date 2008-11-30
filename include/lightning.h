#ifndef __LIGHTNING_H
#define __LIGHTNING_H

#include "inferno.h"
#include "u_mem.h"

#define MAX_LIGHTNINGS	1000
#define MAX_LIGHTNING_NODES	1000

//------------------------------------------------------------------------------

class CLightning;

typedef struct tLightningNode {
	CLightning			*m_child;
	vmsVector			m_vPos;
	vmsVector			m_vNewPos;
	vmsVector			m_vOffs;
	vmsVector			m_vBase;
	vmsVector			m_vDelta [2];
	int					m_nDelta [2];
} tLightningNode;

class CLightningNode : public tLightningNode {
	public:
		CLightningNode () { m_child = NULL; };
		~CLightningNode () { Destroy (); };
		vmsVector *Create (vmsVector *vOffs, vmsVector *vAttract, int nDist);
		void Destroy (void);
		void Setup (bool bInit, vmsVector *vPos, vmsVector *vDelta);
		void Animate (bool bInit, short nSegment, int nDepth);
		bool CreateChild (vmsVector *vEnd, vmsVector *vDelta,
							   int nLife, int nLength, int nAmplitude,
							   char nAngle, short nNodes, short nChildren, char nDepth, short nSteps,
							   short nSmoothe, char bClamp, char bPlasma, char bLight,
							   char nStyle, tRgbaColorf *colorP, CLightning *parentP, short nNode);
		void ComputeOffset (int nSteps);
		int ComputeAttractor (vmsVector *vAttract, vmsVector *vDest, vmsVector *vPos, int nMinDist, int i);
		int Clamp (vmsVector *vPos, vmsVector *vBase, int nAmplitude);
		vmsVector *Smoothe (vmsVector *vOffs, vmsVector *vPrevOffs, int nDist, int nSmoothe);
		vmsVector *Attract (vmsVector *vOffs, vmsVector *vAttract, vmsVector *vPos, int nDist, int i, int bJoinPaths);
		vmsVector CreateJaggy (vmsVector *vPos, vmsVector *vDest, vmsVector *vBase, vmsVector *vPrevOffs,
									  int nSteps, int nAmplitude, int nMinDist, int i, int nSmoothe, int bClamp);
		vmsVector CreateErratic (vmsVector *vPos, vmsVector *vBase, int nSteps, int nAmplitude,
									    int bInPlane, int bFromEnd, int bRandom, int i, int nNodes, int nSmoothe, int bClamp);
		vmsVector CreatePerlin (int nSteps, int nAmplitude, int *nSeed, double phi, double i);
		void Move (vmsVector& vDelta, short nSegment);
		bool SetLight (short nSegment, tRgbaColorf *colorP);
		inline CLightning *GetChild (void) { return m_child; }
		inline void GetChild (CLightning * child) { m_child = child; }
};

//------------------------------------------------------------------------------

typedef struct tLightning {
	CLightning			*m_parent;
	vmsVector			m_vBase;
	vmsVector			m_vPos;
	vmsVector			m_vEnd;
	vmsVector			m_vDir;
	vmsVector			m_vRefEnd;
	vmsVector			m_vDelta;
	CLightningNode		*m_nodes;
	tRgbaColorf			m_color;
	int					m_nNext;
	int					m_nLife;
	int					m_nTTL;
	int					m_nDelay;
	int					m_nLength;
	int					m_nOffset;
	int					m_nAmplitude;
	short					m_nSegment;
	short					m_nSmoothe;
	short					m_nSteps;
	short					m_iStep;
	short					m_nNodes;
	short					m_nChildren;
	short					m_nObject;
	short					nSegment;
	short					m_nNode;
	char					m_nStyle;
	char					m_nAngle;
	char					m_nDepth;
	char					m_bClamp;
	char					m_bPlasma;
	char					m_bRandom;
	char					m_bLight;
	char					m_bInPlane;
} tLightning;

class CLightning : public tLightning {
	public:
		CLightning () { m_parent = NULL, m_nodes = NULL, m_nNodes = 0; };
		~CLightning () { Destroy (); };
		bool Create (char nDepth);
		void Init (vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
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
		void Move (vmsVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd);
		void Render (int nDepth, int bDepthSort, int nThread);
		int SetLight (void);
		inline int MayBeVisible (void);
		CLightning& operator= (CLightning& source) { 
			memcpy (this, &source, sizeof (CLightning)); 
			return *this;
			}
	private:
		void CreatePath (int bSeed, int nDepth);
		int ComputeChildEnd (vmsVector *vPos, vmsVector *vEnd, vmsVector *vDir, vmsVector *vParentDir, int nLength);
		void ComputePlasmaSegment (fVector *vPosf, int bScale, short nSegment, char bStart, char bEnd, int nDepth, int nThread);
		void ComputePlasma (int nDepth, int nThread);
		void RenderCore (tRgbaColorf *colorP, int nDepth, int nThread);
		int SetupPlasma (void);
		void RenderPlasma (tRgbaColorf *colorP, int nThread);
		void RenderBuffered (int nDepth, int nThread);
};

//------------------------------------------------------------------------------

typedef struct tLightningSystem {
	int				m_nId;
	int				m_nNext;
	CLightning		*m_lightnings;
	int				m_nLightnings;
	short				m_nObject;
	int				m_nKey [2];
	time_t			m_tUpdate;
	int				m_nSound;
	char				m_bSound;
	char				m_bForcefield;
	char				m_bDestroy;
} tLightningSystem;

class CLightningSystem : public tLightningSystem {
	public:
		CLightningSystem () { m_lightnings = NULL, m_nLightnings = 0, m_nObject = -1; };
		~CLightningSystem () { Destroy (); };
		bool Create (int m_nId, int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
						 short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
						 short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
						 char bClamp, char bPlasma, char bSound, char bLight, char nStyle, tRgbaColorf *colorP);
		void Destroy (void);
		void Animate (int nStart, int nLightnings);
		void Render (int nStart, int nLightnings, int bDepthSort, int nThread);
		int Update (void);
		void Move (vmsVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd);
		int SetLife (void);
		int SetLight (void);
		inline CLightning* Lightnings (void) { return m_lightnings; }
		inline int GetNext (void) { return m_nNext; }
		inline void SetNext (int nNext) { m_nNext = nNext; }
	private:
		void CreateSound (int bSound);
		void DestroySound (void);
		void UpdateSound (void);
		void MoveForObject (void);
		void RenderBuffered (int nStart, int nLightnings, int nThread);
};

//------------------------------------------------------------------------------

typedef struct tLightningLight {
	vmsVector		vPos;
	tRgbaColorf		color;
	int				nNext;
	int				nLights;
	int				nBrightness;
	int				nDynLight;
	short				nSegment;
	int				nFrame;
} tLightningLight;


typedef struct tLightningData {
	short					*m_objects;
	tLightningLight	*m_lights;
	CLightningSystem	m_systems [MAX_LIGHTNINGS];
	int					m_nFree;
	int					m_nUsed;
	int					m_nNext;
	int					m_bDestroy;
	int					m_nFirstLight;
} tLightningData;

class CLightningManager : public tLightningData {
	public:
		int	m_bDestroy;

	public:
		CLightningManager ();
		~CLightningManager ();
		void Init (void);
		int Create (int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
						short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
						short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
						char bClamp, char bPlasma, char bSound, char bLight, char nStyle, tRgbaColorf *colorP);
		void Destroy (int iLightning, CLightning *pl, bool bDestroy);
		int DestroyAll (bool bForce);
		void Render (void);
		void Update (void);
		void Move (int i, vmsVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd);
		void MoveForObject (tObject *objP);
		void Render (tLightning *pl, int nLightnings, short nDepth, int bDepthSort);
		void RenderBuffered (tLightning *plRoot, int nStart, int nLightnings, int nDepth, int nThread);
		void RenderSystem (void);
		void RenderForDamage (tObject *objP, g3sPoint **pointList, tG3ModelVertex *pVerts, int nVertices);
		void Animate (tLightning *pl, int nStart, int nLightnings, int nDepth);
		int CreateForMissile (tObject *objP);
		void CreateForShaker (tObject *objP);
		void CreateForShakerMega (tObject *objP);
		void CreateForMega (tObject *objP);
		void CreateForBlowup (tObject *objP);
		void CreateForDamage (tObject *objP, tRgbaColorf *colorP);
		void CreateForRobot (tObject *objP, tRgbaColorf *colorP);
		void CreateForPlayer (tObject *objP, tRgbaColorf *colorP);
		void CreateForExplosion (tObject *objP, tRgbaColorf *colorP, int nRods, int nRad, int nTTL);
		void DestroyForObject (tObject *objP);
		void DestroyForAllObjects (int nType, int nId);
		void DestroyForPlayers (void);
		void DestroyForRobots (void);
		void DestroyStatic (void);
		void SetLights (void);
		void ResetLights (int bForce);
		void DoFrame (void);
		void StaticFrame (void);
		int FindDamageLightning (short nObject, int *pKey);
		void SetSegmentLight (short nSegment, vmsVector *vPosP, tRgbaColorf *colorP);
		tRgbaColorf *LightningColor (tObject *objP);
		inline short GetObjectSystem (short nObject) { return (m_objects && (nObject >= 0)) ? m_objects [nObject] : -1; }
		inline void SetObjectSystem (short nObject, int i) { if (m_objects && (nObject >= 0)) m_objects [nObject] = i; }
		inline tLightningLight* GetLight (short nSegment) { return m_lights + nSegment; }
	private:
		int IsUsed (int iLightning);
		CLightningSystem *PrevSystem (int iLightning);
		vmsVector *FindTargetPos (tObject *emitterP, short nTarget);

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
		int Create (vmsVector *vTargetPos, tObject *parentObjP, tObject *targetObjP);
		int Update (tObject *parentObjP, tObject *targetObjP);
		void Destroy (short nObject);
	private:
		int Find (short nObject);
		void Delete (short nHandle);
		vmsVector *GetGunPoint (tObject *objP, vmsVector *vMuzzle);
};

extern COmegaLightnings	omegaLightnings;

//------------------------------------------------------------------------------

#define	SHOW_LIGHTNINGS \
			(!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bUseLightnings, 1, 1, 0))

#endif //__LIGHTNING_H
