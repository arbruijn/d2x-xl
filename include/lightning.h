#ifndef __LIGHTNING_H
#define __LIGHTNING_H

#include "inferno.h"

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
		~CLightningNode () {};
		vmsVector *Create (vmsVector *vOffs, vmsVector *vAttract, int nDist);
		void Destroy (void);
		void Setup (bool bInit, vmsVector *vPos, vmsVector *vDelta);
		void Animate (bool bInit, short nSegment, int nDepth);
		bool CreateChild (vmsVector *vEnd, vmsVector *vDelta,
								short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
								short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
								char bClamp, char bPlasma, char bSound, char bLight, char nStyle, tRgbaColorf *colorP);
		void ComputeOffset (int nSteps);
		int ComputeAttractor (vmsVector *vAttract, vmsVector *vDest, vmsVector *vPos, int nMinDist, int i);
		int Clamp (vmsVector *vPos, vmsVector *vBase, int nAmplitude);
		vmsVector *Create (vmsVector *vOffs, vmsVector *vAttract, int nDist);
		vmsVector *Smoothe (vmsVector *vOffs, vmsVector *vPrevOffs, int nDist, int nSmoothe);
		vmsVector *Attract (vmsVector *vOffs, vmsVector *vAttract, vmsVector *vPos, int nDist, int i, int bJoinPaths);
		vmsVector CreateJaggy (vmsVector *vPos, vmsVector *vDest, vmsVector *vBase, vmsVector *vPrevOffs,
									  int nSteps, int nAmplitude, int nMinDist, int i, int nSmoothe, int bClamp);
		vmsVector CreateErratic (vmsVector *vPos, vmsVector *vBase, int nSteps, int nAmplitude,
									    int bInPlane, int bFromEnd, int bRandom, int i, int nNodes, int nSmoothe, int bClamp);
		vmsVector CreatePerlin (int nSteps, int nAmplitude, int *nSeed, double phi, double i);
		void Move (vmsVector& vDelta, short nSegment);
		bool SetLight (short nSegment);
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
	tLightningNode		*m_nodes;
	tRgbaColorf			m_color;
	int					m_nIndex;
	int					m_nNext;
	int					m_nLife;
	int					m_nTTL;
	int					m_nDelay;
	int					m_nLength;
	int					m_nOffset;
	int					m_nAmplitude;
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
		~CLightning () {};
		bool Create (vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
						 short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
						 short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
						 char bClamp, char bPlasma, char bSound, char bLight, char nStyle, tRgbaColorf *colorP,
						 tLightning *parentP, short nNode);
		void Destroy (void);
		void Smoothe (void);
		void ComputeOffsets (void);
		void Bump (void);
		int SetLife (void);
		void Animate (int nDepth);
		void Move (vmsVector *vNewPos, short nSegment, int bStretch, int bFromEnd);
		int SetLight (void);
		inline int MayBeVisible (void);
	private:
		void Setup (bool bInit);
		int ComputeChildEnd (vmsVector *vPos, vmsVector *vEnd, vmsVector *vDir, vmsVector *vParentDir, int nLength);
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
		CLightningSystem () { m_lightnings = NULL, m_nLightnings = 0; };
		~CLightningSystem () {};
		bool Create (int m_nId, int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
						 short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
						 short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
						 char bClamp, char bPlasma, char bSound, char bSound, char bLight, char nStyle, tRgbaColorf *colorP);
		void Destroy (void);
		void Animate (int nStart, int nLightnings);
		int SetLife (void);
		int Update (void);
		void Move (vmsVector *vNewPos, short nSegment, int bStretch, int bFromEnd);
		int SetLight (void);
		inline int GetNext (void) { return m_nNext; }
		inline int SetNext (int nNext) { m_nNext = nNext; }
	private:
		void CreateSound (int bSound);
		void DestroySound (void);
		void UpdateSound (void);

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

//------------------------------------------------------------------------------

typedef struct tLightningData {
	short					*m_objects;
	tLightningLight	*m_lights;
	tLightningSystem	m_systems [MAX_LIGHTNINGS];
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
		CLightningManager () {};
		~CLightningManager () {};
		void Init (void);
		int Create (int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
						short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
						short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
						char bClamp, char bPlasma, char bSound, char bLight, char nStyle, tRgbaColorf *colorP);
		void Destroy (int iLightning, CLightning *pl, bool bDestroy);
		int DestroyAll (int bForce);
		void Move (int i, tLightning *pl, vmsVector *vNewPos, short nSegment, int bStretch, int bFromEnd);
		void RenderSegment (fVector *vLine, fVector *vPlasma, tRgbaColorf *colorP, int bPlasma, int bStart, int bEnd, short nDepth);
		void Render (tLightning *pl, int nLightnings, short nDepth, int bDepthSort);
		void RenderBuffered (tLightning *plRoot, int nStart, int nLightnings, int nDepth, int nThread);
		void RenderSystem (void);
		void RenderDamage (tObject *objP, g3sPoint **pointlist, tG3ModelVertex *pVerts, int nVertices);
		void Animate (tLightning *pl, int nStart, int nLightnings, int nDepth);
		void MoveObject (tObject *objP);
		void DestroyObject (tObject *objP);
		void SetLights (void);
		void ResetLights (int bForce);
		void DoFrame (void);
		int CreateForMissile (tObject *objP);
		void CreateForShaker (tObject *objP);
		void CreateForShakerMega (tObject *objP);
		void CreateForMega (tObject *objP);
		void CreateForBlowup (tObject *objP);
		void CreateForDamage (tObject *objP, tRgbaColorf *colorP);
		void CreateForRobot (tObject *objP, tRgbaColorf *colorP);
		void CreateForPlayer (tObject *objP, tRgbaColorf *colorP);
		void DestroyForPlayer (void);
		void DestroyForRobot (void);
		void DestroyStatic (void);
		inline short GetObjectSystem (short nObject) { return m_objects [nObject]; }
		inline void SetObjectSysetm (short nObject, int i) { m_objects [nObject] = i; }
	private:
		int IsUsed (int iLightning);
		CLightningSystem *PrevSystem (int iLightning);

};

//------------------------------------------------------------------------------

int CreateLightning (int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
							short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
							short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
							char bClamp, char bPlasma, char bSound, char bLight, char nStyle, tRgbaColorf *colorP);
void DestroyLightnings (int iLightning, tLightning *pf, int bDestroy);
int DestroyAllLightnings (int bForce);
void MoveLightnings (int i, tLightning *pl, vmsVector *vNewPos, short nSegment, int bStretch, int bFromEnd);
void RenderLightningSegment (fVector *vLine, fVector *vPlasma, tRgbaColorf *colorP, int bPlasma, int bStart, int bEnd, short nDepth);
void RenderLightning (tLightning *pl, int nLightnings, short nDepth, int bDepthSort);
void RenderLightningsBuffered (tLightning *plRoot, int nStart, int nLightnings, int nDepth, int nThread);
void RenderLightnings (void);
void RenderDamageLightnings (tObject *objP, g3sPoint **pointlist, tG3ModelVertex *pVerts, int nVertices);
void AnimateLightning (tLightning *pl, int nStart, int nLightnings, int nDepth);
void MoveObjectLightnings (tObject *objP);
void DestroyObjectLightnings (tObject *objP);
void SetLightningLights (void);
void ResetLightningLights (int bForce);
void DoLightningFrame (void);
int CreateMissileLightnings (tObject *objP);
void CreateShakerLightnings (tObject *objP);
void CreateShakerMegaLightnings (tObject *objP);
void CreateMegaLightnings (tObject *objP);
void CreateBlowupLightnings (tObject *objP);
void CreateDamageLightnings (tObject *objP, tRgbaColorf *colorP);
void CreateRobotLightnings (tObject *objP, tRgbaColorf *colorP);
void CreatePlayerLightnings (tObject *objP, tRgbaColorf *colorP);
void DestroyPlayerLightnings (void);
void DestroyRobotLightnings (void);
void DestroyStaticLightnings (void);

#define	SHOW_LIGHTNINGS \
			(!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bUseLightnings, 1, 1, 0))

#endif //__LIGHTNING_H
