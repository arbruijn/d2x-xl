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
	CFixMatrix	m_mOrient;
	CFixVector	m_vPos;				//position
	CFixVector	m_vTransPos;		//transformed position
	CFixVector	m_vDir;				//movement direction
	CFixVector	m_vDrift;
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
		int Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
					   short nSegment, int nLife, int nSpeed, char nParticleSystemType, char nClass,
				      float fScale, tRgbaColorf *colorP, int nCurTime, int bBlowUp,
					   float fBrightness, CFixVector *vEmittingFace);
		int Render (float brightness);
		int Update (int nCurTime);
		inline bool IsVisible (void);
		inline fix Transform (bool bUnscaled) { 
			transformation.Transform (m_vTransPos, m_vPos, bUnscaled); 
			return m_vTransPos [Z];
			}

	private:
		inline int ChangeDir (int d);
		int CollideWithWall (void);

};

//------------------------------------------------------------------------------

typedef struct tParticleEmitter {
	char					m_nType;				//smoke/light trail (corona)
	char					m_nClass;
	int					m_nLife;				//max. particle life time
	int					m_nBirth;			//time of creation
	int					m_nSpeed;			//initial particle speed
	int					m_nParts;			//curent no. of particles
	int					m_nFirstPart;
	int					m_nMaxParts;		//max. no. of particles
	int					m_nDensity;			//density (opaqueness) of particle emitter
	float					m_fPartsPerTick;
	int					m_nTicks;
	int					m_nPartsPerPos;	//particles per interpolated position mutiplier of moving objects
	int					m_nPartLimit;		//highest max. part. no ever set for this emitter
	float					m_fScale;
	int					m_nDefBrightness;
	float					m_fBrightness;
	int					m_nMoved;			//time last moved
	short					m_nSegment;
	int					m_nObject;
	short					m_nObjType;
	short					m_nObjId;
	CFixMatrix			m_mOrient;
	CFixVector			m_vDir;
	CFixVector			m_vPos;				//initial particle position
	CFixVector			m_vPrevPos;			//initial particle position
	CFixVector			m_vEmittingFace [4];
	ubyte					m_bHaveDir;			//movement direction given?
	ubyte					m_bHavePrevPos;	//valid previous position set?
	CArray<CParticle>	m_particles;		//list of active particles
	tRgbaColorf			m_color;
	char					m_bHaveColor;
	char					m_bBlowUpParts;	//blow particles up at their "birth"
	char					m_bEmittingFace;
} tParticleEmitter;

class CParticleEmitter : public tParticleEmitter {
	public:
		CParticleEmitter () { m_particles = NULL; };
		~CParticleEmitter () { Destroy (); };
		int Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
						short nSegment, int nObject, int nMaxParts, float fScale,
						int nDensity, int nPartsPerPos, int nLife, int nSpeed, char nType,
						tRgbaColorf *colorP, int nCurTime, int bBlowUpParts, CFixVector *vEmittingFace);
		int Destroy (void);
		int Update (int nCurTime, int nThread);
		int Render (int nThread);
		void SetPos (CFixVector *vPos, CFixMatrix *mOrient, short nSegment);
		inline void SetDir (CFixVector *vDir);
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
	CArray<CParticleEmitter>	m_emitters;		//list of active emitters
	int								m_nId;
	int								m_nSignature;
	int								m_nBirth;			//time of creation
	int								m_nLife;				//max. particle life time
	int								m_nSpeed;			//initial particle speed
	int								m_nEmitters;		//number of separate particle emitters
	int								m_nMaxEmitters;	//max. no. of emitters
	int								m_nObject;
	short								m_nObjType;
	short								m_nObjId;
	char								m_nType;				//black or white
} tParticleSystem;

class CParticleSystem : public tParticleSystem {
	public:
		CParticleSystem () {};
		~CParticleSystem () { Destroy (); };
		void Init (int nId);
		int Create (CFixVector *pPos, CFixVector *pDir, CFixMatrix *pOrient,
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
		void SetPos (CFixVector *vPos, CFixMatrix *mOrient, short nSegment);
		void SetDir (CFixVector *vDir);
		void SetLife (int nLife);
		void SetScale (float fScale);
		void SetType (int nType);
		void SetSpeed (int nSpeed);
		void SetBrightness (int nBrightness);

		inline bool HasEmitters (void) { return m_emitters.Buffer () != NULL; }
		inline CParticleEmitter* GetEmitter (int i)
			{ return m_emitters + i; }
		inline int GetType (void) { return m_nType; }
		inline int Id (void) { return m_nId; }
};

//------------------------------------------------------------------------------

class CParticleManager {
	private:
		CDataPool<CParticleSystem>	m_systems;
		CArray<short>					m_objectSystems;
		CArray<time_t>					m_objExplTime;
		int								m_nLastType;
		int								m_bAnimate;
		int								m_bStencil;

	public:
		CParticleManager () {}
		~CParticleManager ();
		void Init (void);
		inline void InitObjects (void) { 
			m_objectSystems.Clear (0xff); 
			m_objExplTime.Clear (0xff);
			}
		int Update (void);
		void Render (void);
		int Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
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

		inline time_t* ObjExplTime (int i = 0) { return m_objExplTime + i; }
		inline int LastType (void) { return m_nLastType; }
		inline void SetLastType (int nLastType) { m_nLastType = nLastType; }
		inline int Animate (void) { return m_bAnimate; }
		inline void SetAnimate (int bAnimate) { m_bAnimate = bAnimate; }
		inline int Stencil (void) { return m_bStencil; }
		inline void SetStencil (int bStencil) { m_bStencil = bStencil; }
		inline CParticleSystem& GetSystem (int i) { return m_systems [i]; }
		inline CParticleSystem* GetFirst (void) { return m_systems.GetFirst (m_systems.UsedList ()); }
		inline CParticleSystem* GetNext (void) { return m_systems.GetNext (); }
		inline short GetObjectSystem (short nObject) { return m_objectSystems [nObject]; }

		inline CParticleEmitter* GetEmitter (int i, int j)
			{ return GetSystem (i).GetEmitter (j); }

		inline void SetPos (int i, CFixVector *vPos, CFixMatrix *mOrient, short nSegment) { 
			GetSystem (i).SetPos (vPos, mOrient, nSegment); 
			}

		inline void SetDensity (int i, int nMaxParts, int nDensity) {
			nMaxParts = MaxParticles (nMaxParts, gameOpts->render.particles.nDens [0]);
			GetSystem (i).SetDensity (nMaxParts, nDensity);
			}

		inline void SetScale (int i, float fScale) {
			GetSystem (i).SetScale (fScale);
			}

		inline void SetLife (int i, int nLife) {
			GetSystem (i).SetLife (nLife);
			}

		inline void SetBrightness (int i, int nBrightness) {
			GetSystem (i).SetBrightness (nBrightness);
			}

		inline void SetType (int i, int nType) {
			GetSystem (i).SetType (nType);
			}

		inline void SetSpeed (int i, int nSpeed) {
			GetSystem (i).SetSpeed (nSpeed);
			}

		inline void SetDir (int i, CFixVector *vDir) {
			GetSystem (i).SetDir (vDir);
			}

		inline int SetObjectSystem (int nObject, int i) {
			if ((nObject < 0) || (nObject >= MAX_OBJECTS))
				return -1;
			return m_objectSystems [nObject] = i;
			}

		inline int GetType (int i) {
			return GetSystem (i).GetType ();
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
			{ return GetSystem (i).RemoveEmitter (j); }

	private:
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
