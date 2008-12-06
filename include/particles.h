#ifndef __PARTICLES_H
#define __PARTICLES_H

#include "inferno.h"

#define EXTRA_VERTEX_ARRAYS	1

#define MAX_PARTICLE_SYSTEMS 10000

#define PARTICLE_RAD	(F1_0)

#define SMOKE_PARTICLES		0
#define BUBBLE_PARTICLES	1
#define BULLET_PARTICLES	2
#define LIGHT_PARTICLES		3
#define GATLING_PARTICLES	4

#define MAX_PARTICLES(_nParts,_nDens)	particleManager.MaxParticles (_nParts, _nDens)
#define PARTICLE_SIZE(_nSize,_fScale)	particleManager.ParticleSize (_nSize, _fScale)

//------------------------------------------------------------------------------

typedef struct tPartPos {
	float		x, y, z;
} tPartPos;

typedef struct tParticle {
#if !EXTRA_VERTEX_ARRAYS
	tPartPos		m_glPos;
#endif
	vmsMatrix	m_mOrient;
	vmsVector	m_vPos;				//position
	vmsVector	m_vTransPos;		//transformed position
	vmsVector	m_vDir;				//movement direction
	vmsVector	m_vDrift;
	int			m_nTTL;				//time to live
	int			m_nLife;			//remaining life time
	int			m_nDelay;			//time between creation and appearance
	int			m_nMoved;			//time last moved
	int			m_nWidth;
	int			m_nHeight;
	int			m_nRad;
	short			m_nSegment;
	tRgbaColorf	m_color [2];		//well ... the color, ya know =)
	char			m_nType;			//black or white
	char			m_nRotDir;
	char			m_nBounce;
	char			m_bHaveDir;
	char			m_bBlowUp;
	char			m_bBright;
	char			m_bEmissive;
	char			m_nFade;
	char			m_nClass;
	char			m_nFrame;
	char			m_nRotFrame;
	char			m_nOrient;
} tParticle;

class CParticle : public tParticle {
	public:
		int Create (vmsVector *vPos, vmsVector *vDir, vmsMatrix *mOrient,
					   short nSegment, int nLife, int nSpeed, char nParticleSystemType, char nClass,
				      float fScale, tRgbaColorf *colorP, int nCurTime, int bBlowUp,
					   float fBrightness, vmsVector *vEmittingFace);
		int Render (float brightness);
		int Update (int nCurTime);
		inline bool IsVisible (void);
		inline fix Transform (bool bUnscaled) { 
			G3TransformPoint (m_vTransPos, m_vPos, bUnscaled); 
			return m_vTransPos [Z];
			}

	private:
		inline int ChangeDir (int d);
		int CollideWithWall (void);

};

//------------------------------------------------------------------------------

typedef struct tParticleEmitter {
	char				m_nType;				//smoke/light trail (corona)
	char				m_nClass;
	int				m_nLife;				//max. particle life time
	int				m_nBirth;			//time of creation
	int				m_nSpeed;			//initial particle speed
	int				m_nParts;			//curent no. of particles
	int				m_nFirstPart;
	int				m_nMaxParts;		//max. no. of particles
	int				m_nDensity;			//density (opaqueness) of particle emitter
	float				m_fPartsPerTick;
	int				m_nTicks;
	int				m_nPartsPerPos;	//particles per interpolated position mutiplier of moving objects
	int				m_nPartLimit;		//highest max. part. no ever set for this emitter
	float				m_fScale;
	int				m_nDefBrightness;
	float				m_fBrightness;
	int				m_nMoved;			//time last moved
	short				m_nSegment;
	int				m_nObject;
	short				m_nObjType;
	short				m_nObjId;
	vmsMatrix		m_mOrient;
	vmsVector		m_vDir;
	vmsVector		m_vPos;				//initial particle position
	vmsVector		m_vPrevPos;			//initial particle position
	vmsVector		m_vEmittingFace [4];
	ubyte				m_bHaveDir;			//movement direction given?
	ubyte				m_bHavePrevPos;	//valid previous position set?
	CParticle*		m_particles;		//list of active particles
	tRgbaColorf		m_color;
	char				m_bHaveColor;
	char				m_bBlowUpParts;	//blow particles up at their "birth"
	char				m_bEmittingFace;
} tParticleEmitter;

class CParticleEmitter : public tParticleEmitter {
	public:
		CParticleEmitter () { m_particles = NULL; };
		~CParticleEmitter () { Destroy (); };
		int Create (vmsVector *vPos, vmsVector *vDir, vmsMatrix *mOrient,
						short nSegment, int nObject, int nMaxParts, float fScale,
						int nDensity, int nPartsPerPos, int nLife, int nSpeed, char nType,
						tRgbaColorf *colorP, int nCurTime, int bBlowUpParts, vmsVector *vEmittingFace);
		int Destroy (void);
		int Update (int nCurTime, int nThread);
		int Render (int nThread);
		void SetPos (vmsVector *vPos, vmsMatrix *mOrient, short nSegment);
		inline void SetDir (vmsVector *vDir);
		inline void SetLife (int nLife);
		inline void SetBrightness (int nBrightness);
		inline void SetSpeed (int nSpeed);
		inline void SetType (int nType);
		inline int SetDensity (int nMaxParts, int nDensity);
		inline void SetScale (float fScale);
		inline bool IsAlive (int nCurTime)
			{ return (m_nLife < 0) || (m_nBirth + m_nLife > nCurTime); }
		inline bool IsDead (int nCurTime)
			{ return !(IsAlive (nCurTime) || m_nParts); }

	private:
		char ObjectClass (int nObject);
		float  Brightness (void);
		inline int MayBeVisible (void);
};

//------------------------------------------------------------------------------

typedef struct tParticleSystem {
	CParticleEmitter	*m_emitters;		//list of active emitters
	int					m_nId;
	int					m_nNext;
	int					m_nSignature;
	int					m_nBirth;			//time of creation
	int					m_nLife;				//max. particle life time
	int					m_nSpeed;			//initial particle speed
	int					m_nEmitters;		//number of separate particle emitters
	int					m_nMaxEmitters;	//max. no. of emitters
	int					m_nObject;
	short					m_nObjType;
	short					m_nObjId;
	char					m_nType;				//black or white
} tParticleSystem;

class CParticleSystem : public tParticleSystem {
	public:
		CParticleSystem () { m_emitters = NULL; };
		~CParticleSystem () { Destroy (); };
		void Init (int nId, int nNext);
		int Create (vmsVector *pPos, vmsVector *pDir, vmsMatrix *pOrient,
					   short nSegment, int nMaxEmitters, int nMaxParts, 
						float fScale, int nDensity, int nPartsPerPos, 
						int nLife, int nSpeed, char nType, int nObject,
						tRgbaColorf *pColor, int bBlowUpParts, char nFace);
		void Destroy (void);
		int Render (void);
		int Update (void);
		int RemoveEmitter (int i);
		void SetDensity (int nMaxParts, int nDensity);
		void SetPartScale (float fScale);
		void SetPos (vmsVector *vPos, vmsMatrix *mOrient, short nSegment);
		void SetDir (vmsVector *vDir);
		void SetLife (int nLife);
		void SetScale (float fScale);
		void SetType (int nType);
		void SetSpeed (int nSpeed);
		void SetBrightness (int nBrightness);

		inline bool HasEmitters (void) { return m_emitters != NULL; }
		inline CParticleEmitter* GetEmitter (int i)
			{ return (m_emitters && (i < m_nEmitters)) ? m_emitters + i : NULL; }
		inline int GetType (void) { return m_nType; }
		inline int GetNext (void) { return m_nNext; }
		inline void SetNext (int i) { m_nNext = i; }
};

//------------------------------------------------------------------------------

class CParticleManager {
	private:
		CParticleSystem	m_systems [MAX_PARTICLE_SYSTEMS];
		CArray<short>		m_objectSystems;
		int					m_nFree;
		int					m_nUsed;

	public:
		CParticleManager () {}
		~CParticleManager ();
		void Init (void);
		inline void InitObjects (void)
			{ m_objectSystems.Clear (0xff); }
		int Update (void);
		void Render (void);
		int Create (vmsVector *vPos, vmsVector *vDir, vmsMatrix *mOrient,
						short nSegment, int nMaxEmitters, int nMaxParts,
						float fScale, int nDensity, int nPartsPerPos, int nLife, int nSpeed, char nType,
						int nObject, tRgbaColorf *colorP, int bBlowUpParts, char nSide);
		int Destroy (int iParticleSystem);
		int Shutdown (void);
		int AllocPartList (void);
		void FreePartList (void);

		int BeginRender (int nType, float fScale);
		int EndRender (void);
		int InitBuffer (int bLightmaps);
		void FlushBuffer (float brightness);
		int CloseBuffer (void);

		void AdjustBrightness (CBitmap *bmP);

		inline int GetFree (void) { return m_nFree; }
		inline void SetFree (int i) { m_nFree = i; }
		inline int GetUsed (void) { return m_nUsed; }
		inline void SetUsed (int i) { m_nUsed = i; }
		inline CParticleSystem& GetSystem (int i) { return m_systems [i]; }
		inline short GetObjectSystem (short nObject) { return m_objectSystems [nObject]; }

		inline CParticleEmitter* GetEmitter (int i, int j)
			{ return (0 <= IsUsed (i)) ? GetSystem (i).GetEmitter (j) : NULL; }

		inline void SetPos (int i, vmsVector *vPos, vmsMatrix *mOrient, short nSegment) { 
			if (0 <= IsUsed (i)) 
				GetSystem (i).SetPos (vPos, mOrient, nSegment); 
			}

		inline void SetDensity (int i, int nMaxParts, int nDensity) {
			if (0 <= IsUsed (i)) {
				nMaxParts = MaxParticles (nMaxParts, gameOpts->render.particles.nDens [0]);
				GetSystem (i).SetDensity (nMaxParts, nDensity);
				}
			}

		inline void SetScale (int i, float fScale) {
			if (0 <= IsUsed (i))
				GetSystem (i).SetScale (fScale);
			}

		inline void SetLife (int i, int nLife) {
			if (0 <= IsUsed (i))
				GetSystem (i).SetLife (nLife);
			}

		inline void SetBrightness (int i, int nBrightness) {
			if (0 <= IsUsed (i))
				GetSystem (i).SetBrightness (nBrightness);
			}

		inline void SetType (int i, int nType) {
			if (0 <= IsUsed (i))
				GetSystem (i).SetType (nType);
			}

		inline void SetSpeed (int i, int nSpeed) {
			if (0 <= IsUsed (i))
				GetSystem (i).SetSpeed (nSpeed);
			}

		inline void SetDir (int i, vmsVector *vDir) {
			if (0 <= IsUsed (i))
				GetSystem (i).SetDir (vDir);
			}

		inline int SetObjectSystem (int nObject, int i) {
			if ((nObject < 0) || (nObject >= MAX_OBJECTS))
				return -1;
			return m_objectSystems [nObject] = i;
			}

		inline int GetType (int i) {
			return (IsUsed (i)) ? GetSystem (i).GetType () : -1;
			}

		inline int MaxParticles (int nParts, int nDens) {
			nParts = ((nParts < 0) ? -nParts : nParts * (nDens + 1)); //(int) (nParts * pow (1.2, nDens));
			return (nParts < 100000) ? nParts : 100000;
			}

		inline float ParticleSize (int nSize, float fScale) {
			if (gameOpts->render.particles.bDisperse)
				return (float) (PARTICLE_RAD * (nSize + 1)) / fScale + 0.5f;
			return (float) (PARTICLE_RAD * (nSize + 1) * (nSize + 2) / 2) / fScale + 0.5f;
			}

		inline int RemoveEmitter (int i, int j)
			{ return (0 <= IsUsed (i)) ? GetSystem (i).RemoveEmitter (j) : -1; }

	private:
		int IsUsed (int i);
		void RebuildSystemList (void);

};

extern CParticleManager particleManager;

//------------------------------------------------------------------------------

class CParticleImageManager {
	public:
		CParticleImageManager () {};
		~CParticleImageManager () {};
		int Load (int nType);
		int LoadAll (void);
		void FreeAll (void);
		void Animate (int nType);
		void AdjustBrightness (CBitmap *bmP);
		int GetType (int nType);
};

extern CParticleImageManager particleImageManager;

//------------------------------------------------------------------------------

#endif //__PARTICLES_H
//eof
