#ifndef __LIGHTNING_H
#define __LIGHTNING_H

#include "descent.h"
#include "u_mem.h"
#include "maths.h"

#define MAX_LIGHTNING_SYSTEMS	1000
#define MAX_LIGHTNING_NODES	1000
#define NOISE_TYPE			0

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
		CFixVector *Create (CFixVector *vOffs, CFixVector *vAttract, int nDist, int nAmplitude);
		void Destroy (void);
		void Setup (bool bInit, CFixVector *vPos, CFixVector *vDelta);
		void Animate (bool bInit, short nSegment, int nDepth, int nThread);
		bool CreateChild (CFixVector *vEnd, CFixVector *vDelta,
							   int nLife, int nLength, int nAmplitude,
							   char nAngle, short nNodes, short nChildren, char nDepth, short nSteps,
							   short nSmoothe, char bClamp, char bGlow, char bLight,
							   char nStyle, float nWidth, CFloatVector *colorP, CLightning *parentP, short nNode,
								int nThread);
		void ComputeOffset (int nSteps);
		int ComputeAttractor (CFixVector *vAttract, CFixVector *vDest, CFixVector *vPos, int nMinDist, int i);
		int Clamp (CFixVector *vPos, CFixVector *vBase, int nAmplitude);
		void Rotate (CFloatVector &v0, float len0, CFloatVector &v1, float len1, CFloatVector& vBase, int nSteps);
		void Scale (CFloatVector vStart, CFloatVector vEnd, float scale, int nSteps);
		CFixVector *Smoothe (CFixVector *vOffs, CFixVector *vPrevOffs, int nDist, int nSmoothe);
		CFixVector *Attract (CFixVector *vOffs, CFixVector *vAttract, CFixVector *vPos, int nDist, int i, int bJoinPaths);
		CFixVector CreateJaggy (CFixVector *vPos, CFixVector *vDest, CFixVector *vBase, CFixVector *vPrevOffs,
									  int nSteps, int nAmplitude, int nMinDist, int i, int nSmoothe, int bClamp);
		CFixVector CreateErratic (CFixVector *vPos, CFixVector *vBase, int nSteps, int nAmplitude,
									    int bInPlane, int bFromEnd, int bRandom, int i, int nNodes, int nSmoothe, int bClamp);
		void CreatePerlin (double l, double i, int nThread);
		void Move (const CFixVector& vOffset, short nSegment, int nThread);
		void Move (const CFixVector& vOldPos, const CFixVector& vOldEnd, 
					  const CFixVector& vNewPos, const CFixVector& vNewEnd, 
					  float fScale, short nSegment, int nThread);
		bool SetLight (short nSegment, CFloatVector *colorP);
		inline CLightning *GetChild (void) { return m_child; }
		inline void GetChild (CLightning * child) { m_child = child; }
		inline fix Offset (CFixVector& vStart, CFixVector& vEnd) {
			return VmLinePointDist (vStart, vEnd, m_vNewPos);
			}
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
	CArray<double>				m_dx;
	CArray<double>				m_dy;
	CFloatVector				m_color;
	int							m_nNext;
	int							m_nLife;
	int							m_nTTL;
	int							m_nDelay;
	int							m_nLength;
	int							m_nOffset;
	int							m_nAmplitude;
	short							m_nSegment;
	short							m_nSmoothe;
	short							m_nFrames;
	short							m_iStep;
	short							m_nNodes;
	short							m_nChildren;
	short							m_nObject;
	short							m_nNode;
	char							m_nStyle;
	char							m_nAngle;
	char							m_nDepth;
	char							m_bClamp;
	char							m_bGlow;
	char							m_bRandom;
	char							m_bLight;
	char							m_bInPlane;
} tLightning;

class CLightning : public tLightning {
	private:
		CArray<CFloatVector>		m_plasmaVerts;
		CArray<tTexCoord2f>		m_plasmaTexCoord;
		CArray<CFloatVector3>	m_coreVerts;
		CFloatVector				m_vMin, m_vMax;
		float							m_width;

	public:
		CLightning () { m_parent = NULL, m_nodes = NULL, m_nNodes = 0; };
		~CLightning () { Destroy (); };
		bool Create (char nDepth, int nThread);
		void Init (CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
					  short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
					  char nAngle, int nOffset, short nNodes, short nChildren, short nSteps,
					  short nSmoothe, char bClamp, char bGlow, char bLight,
					  char nStyle, float nWidth, CFloatVector *colorP, CLightning *parentP, short nNode);
		void Setup (bool bInit);
		void Destroy (void);
		void DestroyNodes (void);
		void Smoothe (void);
		void ComputeOffsets (void);
		void Bump (void);
		int SetLife (void);
		void Animate (int nDepth, int nThread);
		int Update (int nDepth, int nThread);
		void Move (CFixVector vNewPos, short nSegment, int nThread);
		void Move (CFixVector vNewPos, CFixVector vNewEnd, short nSegment, int nThread);
		void Render (int nDepth, int nThread);
		int SetLight (void);
		inline int MayBeVisible (int nThread);
		CLightning& operator= (CLightning& source) { 
			memcpy (this, &source, sizeof (CLightning)); 
			return *this;
			}

	private:
		void CreatePath (int nDepth, int nThread);
		int ComputeChildEnd (CFixVector *vPos, CFixVector *vEnd, CFixVector *vDir, CFixVector *vParentDir, int nLength);
		void ComputeGlow (int nDepth, int nThread);
		void ComputeCore (void);
		void RenderCore (CFloatVector *colorP, int nDepth, int nThread);
		void RenderSetup (int nDepth, int nThread);
		int SetupGlow (void);
		void RenderGlow (CFloatVector *colorP, int nDepth, int nThread);
		void Draw (int nDepth, int nThread);
		void Rotate (int nSteps);
		void Scale (int nSteps, int nAmplitude);
		void ComputeExtent (CFloatVector* vertexP);
};

//------------------------------------------------------------------------------

typedef struct tLightningSystem {
	int						m_nId;
	CArray<CLightning>	m_lightning;
	int						m_nBolts;
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

class CLightningEmitter : public tLightningSystem {
	public:
		CLightningEmitter () { m_bValid = 0, m_lightning = NULL, m_nBolts = 0, m_nObject = -1; };
		~CLightningEmitter () { Destroy (); };
		void Init (int nId);
		bool Create (int nBolts, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
						 short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
						 short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
						 char bClamp, char bGlow, char bSound, char bLight, char nStyle, float nWidth, CFloatVector *colorP);
		void Destroy (void);
		void Animate (int nStart, int nBolts, int nThread);
		void Render (int nStart, int nBolts, int nThread);
		int Update (int nThread);
		void Move (CFixVector vNewPos, short nSegment, int nThread);
		void Move (CFixVector vNewPos, CFixVector vNewEnd, short nSegment, int nThread);
		void Mute (void);
		int SetLife (void);
		int SetLight (void);
		inline CLightning* Lightning (void) { return m_lightning.Buffer (); }
		inline int Id (void) { return m_nId; }
		inline void SetValid (char bValid) { m_bValid = bValid; }
	private:
		void CreateSound (int bSound);
		void DestroySound (void);
		void UpdateSound (void);
		void MoveForObject (int nThread = 0);
		void RenderBuffered (int nStart, int nBolts, int nThread);
};

//------------------------------------------------------------------------------

typedef struct tLightningLight {
	CFixVector		vPos;
	CFloatVector	color;
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
	CDataPool<CLightningEmitter>	m_emitters; // [MAX_LIGHTNING];
	CArray<CLightningEmitter*>		m_emitterList;
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
		int Create (int nBolts, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
						short nObject, int nLife, int nDelay, int nLength, int nAmplitude, char nAngle, int nOffset,
						short nNodeC, short nChildC, char nDepth, short nSteps, short nSmoothe, 
						char bClamp, char bGlow, char bSound, char bLight, char nStyle, float nWidth, CFloatVector *colorP);
		int Create (tLightningInfo& li, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta, short nObject = -1);
		void Destroy (CLightningEmitter* systemP, CLightning *lightningP);
		void Cleanup (void);
		int Shutdown (bool bForce);
		void Render (void);
		void Update (void);
		void Move (int i, CFixVector vNewPos, short nSegment);
		void Move (int i, CFixVector vNewPos, CFixVector vNewEnd, short nSegment);
		void Mute (void);
		void MoveForObject (CObject *objP);
		void Render (tLightning *lightningP, int nBolts, short nDepth);
		void RenderBuffered (tLightning *lightningRootP, int nStart, int nBolts, int nDepth, int nThread);
		void RenderSystem (void);
		int RenderForDamage (CObject *objP, CRenderPoint **pointList, RenderModel::CVertex *pVerts, int nVertices);
		void Animate (tLightning *lightningP, int nStart, int nBolts, int nDepth);
		int Enable (CObject* objP);
		int CreateForMissile (CObject *objP);
		void CreateForShaker (CObject *objP);
		void CreateForShakerMega (CObject *objP);
		void CreateForMega (CObject *objP);
		void CreateForBlowup (CObject *objP);
		void CreateForDamage (CObject *objP, CFloatVector *colorP);
		void CreateForRobot (CObject *objP, CFloatVector *colorP);
		void CreateForPlayer (CObject *objP, CFloatVector *colorP);
		void CreateForExplosion (CObject *objP, CFloatVector *colorP, int nRods, int nRad, int nTTL);
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
		void SetSegmentLight (short nSegment, CFixVector *vPosP, CFloatVector *colorP);
		CFloatVector *LightningColor (CObject *objP);
		inline short GetObjectSystem (short nObject) { return (m_objects.Buffer () && (nObject >= 0)) ? m_objects [nObject] : -1; }
		inline void SetObjectSystem (short nObject, int i) { if (m_objects.Buffer () && (nObject >= 0)) m_objects [nObject] = i; }
		inline tLightningLight* GetLight (short nSegment) { return m_lights + nSegment; }

	private:
		CFixVector *FindTargetPos (CObject *emitterP, short nTarget);

};

extern CLightningManager lightningManager;

//------------------------------------------------------------------------------

typedef struct tOmegaLightningHandles {
	int		nLightning [2];
	short		nParentObj;
	short		nTargetObj;
} tOmegaLightningHandles;


class COmegaLightning {
	private:
		tOmegaLightningHandles	m_handles [MAX_OBJECTS_D2X];
		int							m_nHandles;

	public:
		COmegaLightning () : m_nHandles (0) { Init (); }
		~COmegaLightning () {};
		void Init (void) { 
			m_nHandles = 0; 
			memset (m_handles, 0xFF, sizeof (m_handles));
			};
		int Create (CFixVector *vTargetPos, CObject *parentObjP, CObject *targetObjP);
		int Update (CObject *parentObjP, CObject *targetObjP, CFixVector* vTargetPos = NULL);
		void Destroy (short nObject);
		inline bool Exist (void) { return m_nHandles > 0; }

	private:
		int Find (short nObject);
		void Delete (short nHandle);
		CFixVector *GetGunPoint (CObject *objP, CFixVector *vMuzzle);
};

extern COmegaLightning	omegaLightning;

//------------------------------------------------------------------------------

#define	SHOW_LIGHTNING \
			(gameOpts->render.effects.bEnabled && !(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bUseLightning, 1, 1, 0))

//------------------------------------------------------------------------------

extern CFixVector *VmRandomVector (CFixVector *vRand);

//------------------------------------------------------------------------------

#endif //__LIGHTNING_H
