#ifndef __LIGHTNING_H
#define __LIGHTNING_H

#include "descent.h"
#include "u_mem.h"
#include "maths.h"

#define MAX_LIGHTNING_SYSTEMS	1000
#define MAX_LIGHTNING_NODES	1000
#define NOISE_TYPE				0

//------------------------------------------------------------------------------

class CLightning;

typedef struct tLightningNode {
	CLightning			*m_child;
	CFixVector			m_vPos;
	CFixVector			m_vNewPos;
	CFixVector			m_vOffs;
	CFixVector			m_vBase;
	CFixVector			m_vDelta [2];
	int32_t				m_nDelta [2];
} tLightningNode;

class CLightningNode : public tLightningNode {
	public:
		CLightningNode () { m_child = NULL; };
		~CLightningNode () { Destroy (); };
		CFixVector *Create (CFixVector *vOffs, CFixVector *vAttract, int32_t nDist, int32_t nAmplitude);
		void Destroy (void);
		void Setup (bool bInit, CFixVector *vPos, CFixVector *vDelta);
		void Animate (bool bInit, int16_t nSegment, int32_t nDepth, int32_t nThread);
		bool CreateChild (CFixVector *vEnd, CFixVector *vDelta,
							   int32_t nLife, int32_t nLength, int32_t nAmplitude,
							   char nAngle, int16_t nNodes, int16_t nChildren, char nDepth, int16_t nSteps,
							   int16_t nSmoothe, char bClamp, char bGlow, char bLight,
							   char nStyle, float nWidth, CFloatVector *pColor, CLightning *pParent, int16_t nNode,
								int32_t nThread);
		void ComputeOffset (int32_t nSteps);
		int32_t ComputeAttractor (CFixVector *vAttract, CFixVector *vDest, CFixVector *vPos, int32_t nMinDist, int32_t i);
		int32_t Clamp (CFixVector *vPos, CFixVector *vBase, int32_t nAmplitude);
		void Rotate (CFloatVector &v0, float len0, CFloatVector &v1, float len1, CFloatVector& vBase, int32_t nSteps);
		void Scale (CFloatVector vStart, CFloatVector vEnd, float scale, int32_t nSteps);
		CFixVector *Smoothe (CFixVector *vOffs, CFixVector *vPrevOffs, int32_t nDist, int32_t nSmoothe);
		CFixVector *Attract (CFixVector *vOffs, CFixVector *vAttract, CFixVector *vPos, int32_t nDist, int32_t i, int32_t bJoinPaths);
		CFixVector CreateJaggy (CFixVector *vPos, CFixVector *vDest, CFixVector *vBase, CFixVector *vPrevOffs,
									  int32_t nSteps, int32_t nAmplitude, int32_t nMinDist, int32_t i, int32_t nSmoothe, int32_t bClamp);
		CFixVector CreateErratic (CFixVector *vPos, CFixVector *vBase, int32_t nSteps, int32_t nAmplitude,
									    int32_t bInPlane, int32_t bFromEnd, int32_t bRandom, int32_t i, int32_t nNodes, int32_t nSmoothe, int32_t bClamp);
		void CreatePerlin (double l, double i, int32_t nThread);
		void Move (const CFixVector& vOffset, int16_t nSegment);
		void Move (const CFixVector& vOldPos, const CFixVector& vOldEnd, 
					  const CFixVector& vNewPos, const CFixVector& vNewEnd, 
					  float fScale, int16_t nSegment);
		bool SetLight (int16_t nSegment, CFloatVector *pColor);
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
	int32_t						m_nNext;
	int32_t						m_nLife;
	int32_t						m_nTTL;
	int32_t						m_nDelay;
	int32_t						m_nLength;
	int32_t						m_nOffset;
	int32_t						m_nAmplitude;
	int16_t						m_nSegment;
	int16_t						m_nSmoothe;
	int16_t						m_nFrames;
	int16_t						m_iStep;
	int16_t						m_nNodes;
	int16_t						m_nChildren;
	int16_t						m_nObject;
	int16_t						m_nNode;
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
		float							m_fAvgDist;
		float							m_fDistScale;

	public:
		CLightning () { m_parent = NULL, m_nodes = NULL, m_nNodes = 0; };
		~CLightning () { Destroy (); };
		bool Create (char nDepth, int32_t nThread);
		void Init (CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
					  int16_t nObject, int32_t nLife, int32_t nDelay, int32_t nLength, int32_t nAmplitude,
					  char nAngle, int32_t nOffset, int16_t nNodes, int16_t nChildren, int16_t nSteps,
					  int16_t nSmoothe, char bClamp, char bGlow, char bLight,
					  char nStyle, float nWidth, CFloatVector *pColor, CLightning *pParent, int16_t nNode);
		void Setup (bool bInit);
		void Destroy (void);
		void DestroyNodes (void);
		void Smoothe (void);
		void ComputeOffsets (void);
		void Bump (void);
		int32_t SetLife (void);
		void Animate (int32_t nDepth, int32_t nThread);
		int32_t Update (int32_t nDepth, int32_t nThread);
		void Move (CFixVector vNewPos, int16_t nSegment);
		void Move (CFixVector vNewPos, CFixVector vNewEnd, int16_t nSegment);
		void Render (int32_t nDepth, int32_t nThread);
		int32_t SetLight (void);
		inline int32_t MayBeVisible (int32_t nThread);
		CLightning& operator= (CLightning& source) { 
			memcpy (this, &source, sizeof (CLightning)); 
			return *this;
			}

	private:
		void CreatePath (int32_t nDepth, int32_t nThread);
		int32_t ComputeChildEnd (CFixVector *vPos, CFixVector *vEnd, CFixVector *vDir, CFixVector *vParentDir, int32_t nLength);
		void ComputeGlow (int32_t nDepth, int32_t nThread);
		void ComputeCore (void);
		void RenderCore (CFloatVector *pColor, int32_t nDepth, int32_t nThread);
		void RenderSetup (int32_t nDepth, int32_t nThread);
		int32_t SetupGlow (void);
		void RenderGlow (CFloatVector *pColor, int32_t nDepth, int32_t nThread);
		void Draw (int32_t nDepth, int32_t nThread);
		void Rotate (int32_t nSteps);
		void Scale (int32_t nSteps, int32_t nAmplitude);
		void ComputeExtent (CFloatVector* pVertex);
		float ComputeAvgDist (CFloatVector3* pVertex, int32_t nVerts);
		float ComputeDistScale (float zPivot);

};

//------------------------------------------------------------------------------

typedef struct tLightningSystem {
	int32_t					m_nId;
	CArray<CLightning>	m_lightning;
	int32_t					m_nBolts;
	int16_t					m_nSegment [2];
	int16_t					m_nObject;
	int32_t					m_nKey [2];
	time_t					m_tUpdate;
	int32_t					m_nSound;
	char						m_bSound;
	char						m_bForcefield;
	char						m_bDestroy;
	char						m_bValid;
} tLightningSystem;

class CLightningEmitter : public tLightningSystem {
	public:
		CLightningEmitter () { m_bValid = 0, m_lightning = NULL, m_nBolts = 0, m_nObject = -1; };
		~CLightningEmitter () { Destroy (); };
		void Init (int32_t nId);
		bool Create (int32_t nBolts, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
						 int16_t nObject, int32_t nLife, int32_t nDelay, int32_t nLength, int32_t nAmplitude, char nAngle, int32_t nOffset,
						 int16_t nNodeC, int16_t nChildC, char nDepth, int16_t nSteps, int16_t nSmoothe, 
						 char bClamp, char bGlow, char bSound, char bLight, char nStyle, float nWidth, CFloatVector *pColor);
		void Destroy (void);
		void Animate (int32_t nStart, int32_t nBolts, int32_t nThread);
		void Render (int32_t nStart, int32_t nBolts, int32_t nThread);
		int32_t Update (int32_t nThread);
		void Move (CFixVector vNewPos, int16_t nSegment);
		void Move (CFixVector vNewPos, CFixVector vNewEnd, int16_t nSegment);
		void Mute (void);
		int32_t SetLife (void);
		int32_t SetLight (void);
		inline CLightning* Lightning (void) { return m_lightning.Buffer (); }
		inline int32_t Id (void) { return m_nId; }
		inline void SetValid (char bValid) { m_bValid = bValid; }
	private:
		void CreateSound (int32_t bSound, int32_t nThread = 0);
		void DestroySound (void);
		void UpdateSound (int32_t nThread = 0);
		void MoveForObject (void);
		void RenderBuffered (int32_t nStart, int32_t nBolts, int32_t nThread);
};

//------------------------------------------------------------------------------

typedef struct tLightningLight {
	CFixVector		vPos;
	CFloatVector	color;
	int32_t				nNext;
	int32_t				nLights;
	int32_t				nBrightness;
	int32_t				nDynLight;
	int16_t				nSegment;
	int32_t				nFrame;
} tLightningLight;


typedef struct tLightningData {
	CArray<int16_t>					m_objects;
	CArray<tLightningLight>			m_lights;
	CDataPool<CLightningEmitter>	m_emitters; // [MAX_LIGHTNING];
	CArray<CLightningEmitter*>		m_emitterList;
	int32_t								m_bDestroy;
	int32_t								m_nFirstLight;
} tLightningData;

class CLightningManager : public tLightningData {
	public:
		int32_t	m_bDestroy;

	public:
		CLightningManager ();
		~CLightningManager ();
		void Init (void);
		int32_t Create (int32_t nBolts, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
						int16_t nObject, int32_t nLife, int32_t nDelay, int32_t nLength, int32_t nAmplitude, char nAngle, int32_t nOffset,
						int16_t nNodeC, int16_t nChildC, char nDepth, int16_t nSteps, int16_t nSmoothe, 
						char bClamp, char bGlow, char bSound, char bLight, char nStyle, float nWidth, CFloatVector *pColor);
		int32_t Create (tLightningInfo& li, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta, int16_t nObject = -1);
		void Destroy (CLightningEmitter* pSystem, CLightning *pLightning);
		void Cleanup (void);
		int32_t Shutdown (bool bForce);
		void Render (void);
		void Update (void);
		void Move (int32_t i, CFixVector vNewPos, int16_t nSegment);
		void Move (int32_t i, CFixVector vNewPos, CFixVector vNewEnd, int16_t nSegment);
		void Mute (void);
		void MoveForObject (CObject *pObj);
		void Render (tLightning *pLightning, int32_t nBolts, int16_t nDepth);
		void RenderBuffered (tLightning *lightningRootP, int32_t nStart, int32_t nBolts, int32_t nDepth, int32_t nThread);
		void RenderSystem (void);
		int32_t RenderForDamage (CObject *pObj, CRenderPoint **pointList, RenderModel::CVertex *pVerts, int32_t nVertices);
		void Animate (tLightning *pLightning, int32_t nStart, int32_t nBolts, int32_t nDepth);
		int32_t Enable (CObject* pObj);
		int32_t CreateForMissile (CObject *pObj);
		void CreateForShaker (CObject *pObj);
		void CreateForShakerMega (CObject *pObj);
		void CreateForMega (CObject *pObj);
		void CreateForBlowup (CObject *pObj);
		void CreateForDamage (CObject *pObj, CFloatVector *pColor);
		void CreateForRobot (CObject *pObj, CFloatVector *pColor);
		void CreateForPlayer (CObject *pObj, CFloatVector *pColor);
		void CreateForExplosion (CObject *pObj, CFloatVector *pColor, int32_t nRods, int32_t nRad, int32_t nTTL);
		void CreateForTeleport (CObject* pObj, CFloatVector *pColor, float fRodScale = 1.0f);
		void CreateForPlayerTeleport (CObject *pObj);
		void CreateForRobotTeleport (CObject *pObj);
		void CreateForPowerupTeleport (CObject *pObj);
		void DestroyForObject (CObject *pObj);
		void DestroyForAllObjects (int32_t nType, int32_t nId);
		void DestroyForPlayers (void);
		void DestroyForRobots (void);
		void DestroyStatic (void);
		void SetLights (void);
		void ResetLights (int32_t bForce);
		void DoFrame (void);
		void StaticFrame (void);
		int32_t FindDamageLightning (int16_t nObject, int32_t *pKey);
		void SetSegmentLight (int16_t nSegment, CFixVector *vPosP, CFloatVector *pColor);
		CFloatVector *LightningColor (CObject *pObj);
		inline int16_t GetObjectSystem (int16_t nObject) { return (m_objects.Buffer () && (nObject >= 0)) ? m_objects [nObject] : -1; }
		inline void SetObjectSystem (int16_t nObject, int32_t i) { if (m_objects.Buffer () && (nObject >= 0)) m_objects [nObject] = i; }
		inline tLightningLight* GetLight (int16_t nSegment) { return m_lights + nSegment; }

	private:
		CFixVector *FindTargetPos (CObject *pEmitter, int16_t nTarget);

};

extern CLightningManager lightningManager;

//------------------------------------------------------------------------------

typedef struct tOmegaLightningHandles {
	int32_t		nLightning [2];
	int16_t		nParentObj;
	int16_t		nTargetObj;
} tOmegaLightningHandles;


class COmegaLightning {
	private:
		tOmegaLightningHandles	m_handles [MAX_OBJECTS_D2X];
		int32_t						m_nHandles;

	public:
		COmegaLightning () : m_nHandles (0) { Init (); }
		~COmegaLightning () {};
		void Init (void) { 
			m_nHandles = 0; 
			memset (m_handles, 0xFF, sizeof (m_handles));
			};
		int32_t Create (CFixVector *vTargetPos, CObject *pParentObj, CObject *pTargetObj);
		int32_t Update (CObject *pParentObj, CObject *pTargetObj, CFixVector* vTargetPos = NULL);
		void Destroy (int16_t nObject);
		inline bool Exist (void) { return m_nHandles > 0; }

	private:
		int32_t Find (int16_t nObject);
		void Delete (int16_t nHandle);
		CFixVector *GetGunPoint (CObject *pObj, CFixVector *vMuzzle);
};

extern COmegaLightning	omegaLightning;

//------------------------------------------------------------------------------

#define	SHOW_LIGHTNING(_nQuality) \
			(gameOpts->render.effects.bEnabled && !(gameStates.app.bNostalgia || COMPETITION) && (EGI_FLAG (bUseLightning, 1, 1, 0) >= (_nQuality)))

//------------------------------------------------------------------------------

extern CFixVector *VmRandomVector (CFixVector *vRand);

//------------------------------------------------------------------------------

#endif //__LIGHTNING_H
