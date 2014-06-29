#ifndef __PARTICLES_H
#define __PARTICLES_H

#include "descent.h"
#include "ogl_shader.h"

#define EXTRA_VERTEX_ARRAYS	1

#define MAX_PARTICLE_SYSTEMS 10000

#define PARTICLE_RAD	(I2X (1))

#define SIMPLE_SMOKE_PARTICLES	0
#define SMOKE_PARTICLES				1
#define BUBBLE_PARTICLES			2
#define RAIN_PARTICLES				3
#define SNOW_PARTICLES				4
#define FIRE_PARTICLES				5
#define WATERFALL_PARTICLES		6
#define BULLET_PARTICLES			7
#define LIGHT_PARTICLES				8
#define GATLING_PARTICLES			9
#define SPARK_PARTICLES				10
#define PARTICLE_TYPES				11

#define MAX_PARTICLE_QUALITY		3

#define MAX_PARTICLES(_nParts,_nDens)				particleManager.MaxParticles (_nParts, _nDens)
#define PARTICLE_SIZE(_nSize,_fScale,_bBlowUp)	particleManager.ParticleSize (_nSize, _fScale, _bBlowUp)

#if DBG
#	define ENABLE_RENDER	1
#	define ENABLE_UPDATE	1
#	define ENABLE_FLUSH	1
#else
#	define ENABLE_RENDER	1
#	define ENABLE_UPDATE	1
#	define ENABLE_FLUSH	1
#endif

#define MAX_SHRAPNEL_LIFE			I2X (2)

#define TRANSFORM_PARTICLE_VERTICES 0

#define LAZY_RENDER_SETUP 1

#define SMOKE_LIGHTING 0

#define MAKE_SMOKE_IMAGE 0

#define MT_PARTICLES	0

#define BLOWUP_PARTICLES 0		//blow particles up at "birth"

#define SMOKE_SLOWMO 0

#define SORT_CLOUDS 1

#define PARTICLE_FPS	30

#define PARTICLE_POSITIONS 64

#define PART_DEPTHBUFFER_SIZE 100000
#define PARTLIST_SIZE 1000000

#define PART_BUF_SIZE	25000
#define VERT_BUF_SIZE	(PART_BUF_SIZE * 4)

#define MAX_PARTICLE_BUFFERS	3

#define HAVE_PARTICLE_SHADER	1

#if HAVE_PARTICLE_SHADER
#	define USE_PARTICLE_SHADER	ogl.m_features.bShaders.Available() //(gameOpts->SoftBlend (SOFT_BLEND_PARTICLES))
#else
#	define USE_PARTICLE_SHADER	0
#endif

//------------------------------------------------------------------------------

extern int nPartSeg [MAX_THREADS];
extern CFloatVector defaultParticleColor;

//------------------------------------------------------------------------------

class CEffectArea {
	public:
		CFloatVector	m_pos;
		float				m_rad;

	CEffectArea() : m_rad (0.0f) { m_pos.SetZero (); }

	inline CEffectArea& operator= (CEffectArea other) { 
		m_pos = other.m_pos, m_rad = other.m_rad;
		return *this;
		}

	CEffectArea& Add (CFloatVector& pos, float rad);

	inline CEffectArea& operator+= (CEffectArea other) { return Add (other.m_pos, other.m_rad); }

	inline bool const Overlap (CFloatVector& pos, float rad) { return CFloatVector::Dist (m_pos, pos) < m_rad + rad; }

	inline bool const Overlap (CFixVector& pos, float rad) { 
		CFloatVector v;
		v.Assign (pos);
		return CFloatVector::Dist (m_pos, v) < m_rad + rad; 
		}

	inline bool const operator&& (CEffectArea other) { return Overlap (other.m_pos, other.m_rad); }

	inline void Reset (void) {
		m_pos = CFloatVector::ZERO;
		m_rad = 0;
		}
};

//------------------------------------------------------------------------------

typedef struct tPartPos {
	float		x, y, z;
} tPartPos;

typedef struct tParticleVertex {
	CFloatVector3	vertex;
	tTexCoord3f		texCoord;
	CFloatVector		color;
	} tParticleVertex;

typedef struct tParticle {
#if !EXTRA_VERTEX_ARRAYS
	tPartPos		m_glPos;
#endif
	CFixMatrix	m_mOrient;
	CFixVector	m_vPos;				//position
	CFloatVector m_vPosf;
	CFixVector	m_vStartPos;		//initial position
	CFixVector	m_vTransPos;		//transformed position
	CFixVector	m_vDir;				//movement direction
	CFixVector	m_vDrift;
	int			m_nTTL;				//time to live
	int			m_nLife;				//remaining life time
	int			m_nFadeTime;
	float			m_decay;
	int			m_nDelay;			//time between creation and appearance
	int			m_nUpdated;
	int			m_nMoved;			//time last moved
	float			m_nWidth;
	float			m_nHeight;
	float			m_nRad;
	short			m_nSegment;
	tTexCoord2f	m_texCoord;
	float			m_deltaUV;
	CFloatVector	m_color [2];		//well ... the color, ya know =)
	CFloatVector	m_renderColor;
	char			m_nType;				//black or white
	char			m_nRenderType;
	char			m_nBounce;
	char			m_bHaveDir;
	char			m_bBlowUp;
	char			m_bBright;
	char			m_bEmissive;
	char			m_bReversed;
	char			m_nFadeType;
	char			m_nClass;
	char			m_iFrame;
	char			m_nFrames;
	char			m_nRotFrame;
	char			m_nOrient;
	char			m_bChecked;
	char			m_bSkybox;
	char			m_bAnimate;
	char			m_bRotate;
	char			m_nDelayPosUpdate;
} tParticle;

class CParticle : public tParticle {
	public:
		static CFloatVector vRot [PARTICLE_POSITIONS];
		static CFixMatrix mRot [2][PARTICLE_POSITIONS];

		static void InitRotation (void);
		static void SetupRotation (void);

	public:
		int Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
					   short nSegment, int nLife, int nSpeed, char nParticleSystemType, char nClass,
				      float fScale, CFloatVector *colorP, int nCurTime, int bBlowUp, char nFadeType,
					   float fBrightness, CFixVector *vEmittingFace);
		int Render (float brightness);
		void Setup (bool alphaControl, float brightness, char nFrame, char nRotFrame, tParticleVertex* pb, int nThread);
		int Update (int nCurTime, float brightness, int nThread);
		bool IsVisible (int nThread);
		inline fix Transform (bool bUnscaled) {
			transformation.Transform (m_vTransPos, m_vPos, bUnscaled);
			return m_vTransPos.v.coord.z;
			}
#ifdef _WIN32
		inline float Rad (void) { return (float) _hypot (m_nWidth, m_nHeight); }
#else
		inline float Rad (void) { return (float) hypot (m_nWidth, m_nHeight); }
#endif
		inline CFloatVector& Posf (void) {
			m_vPosf.Assign (m_vPos);
			return m_vPosf;
			}
		int RenderType (void);

	private:
		inline int ChangeDir (int d);
		int CollideWithWall (int nThread);
		void UpdateDecay (void);
		int UpdateDrift (int t, int nThread);
		void UpdateTexCoord (void);
		void UpdateColor (float brightness, int nThread);
		int SetupColor (float brightness);
		fix Drag (void);
		int Bounce (int nThread);
#ifdef _WIN32
		inline void InitColor (CFloatVector* colorP, float fBrightness, char nParticleSystemType);
		inline int InitDrift (CFixVector* vDir, int nSpeed);
		inline bool InitPosition (CFixVector* vPos, CFixVector* vEmittingFace, CFixMatrix *mOrient);
		inline void InitSize (float nScale, CFixMatrix *mOrient);
		inline void InitAnimation (void);
#else
		void InitColor (CFloatVector* colorP, float fBrightness, char nParticleSystemType);
		int InitDrift (CFixVector* vDir, int nSpeed);
		bool InitPosition (CFixVector* vPos, CFixVector* vEmittingFace, CFixMatrix *mOrient);
		void InitSize (float nScale, CFixMatrix *mOrient);
		void InitAnimation (void);
#endif
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
	//int					m_nDensity;			//density (opaqueness) of particle emitter
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
	CFloatVector			m_color;
	char					m_bHaveColor;
	char					m_bBlowUpParts;	//blow particles up at their "birth"
	char					m_bEmittingFace;
	char					m_nFadeType;
} tParticleEmitter;

class CParticleEmitter : public tParticleEmitter {
	public:
		CParticleEmitter () { m_particles = NULL; };
		~CParticleEmitter () { Destroy (); };
		int Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
						short nSegment, int nObject, int nMaxParts, float fScale,
						/*int nDensity, int nPartsPerPos,*/ int nLife, int nSpeed, char nType,
						CFloatVector *colorP, int nCurTime, int bBlowUpParts, CFixVector *vEmittingFace);
		int Destroy (void);
		int Update (int nCurTime, int nThread);
		int Render (int nThread);
		void SetPos (CFixVector *vPos, CFixMatrix *mOrient, short nSegment);
		void SetDir (CFixVector *vDir);
		void SetLife (int nLife);
		void SetBlowUp (int bBlowUpParts);
		void SetBrightness (int nBrightness);
		void SetFadeType (int nFadeType);
		void SetSpeed (int nSpeed);
		void SetType (int nType);
		int SetDensity (int nMaxParts/*, int nDensity*/);
		void SetScale (float fScale);
		inline bool IsAlive (int nCurTime)
		 { return (m_nLife < 0) || (m_nBirth + m_nLife > nCurTime); }
		inline bool IsDead (int nCurTime)
		 { return !(IsAlive (nCurTime) || m_nParts); }

	private:
		char ObjectClass (int nObject);
		float Brightness (void);
		inline int MayBeVisible (int nThread);
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
	bool								m_bDestroy;
	char								m_bValid;
	char								m_nType;				//black or white
	char								m_nFadeType;
} tParticleSystem;

class CParticleSystem : public tParticleSystem {
	public:
		CParticleSystem () { m_bValid = m_bDestroy = false; }
		~CParticleSystem () { Destroy (); };
		void Init (int nId);
		int Create (CFixVector *pPos, CFixVector *pDir, CFixMatrix *pOrient,
					   short nSegment, int nMaxEmitters, int nMaxParts,
						float fScale, /*int nDensity, int nPartsPerPos,*/
						int nLife, int nSpeed, char nType, int nObject,
						CFloatVector *pColor, int bBlowUpParts, char nFace);
		void Destroy (void);
		int Render (int nThread);
		int Update (int nThread);
		int RemoveEmitter (int i);
		void SetDensity (int nMaxParts/*, int nDensity*/);
		void SetPartScale (float fScale);
		void SetPos (CFixVector *vPos, CFixMatrix *mOrient, short nSegment);
		void SetDir (CFixVector *vDir);
		void SetBlowUp (int bBlowUp);
		void SetLife (int nLife);
		void SetScale (float fScale);
		void SetType (int nType);
		void SetSpeed (int nSpeed);
		void SetBrightness (int nBrightness);
		void SetFadeType (int nFadeType);

		inline bool HasEmitters (void) { return m_emitters.Buffer () != NULL; }
		inline CParticleEmitter* GetEmitter (int i)
		 { return m_emitters + i; }
		inline int GetType (void) { return m_nType; }
		inline int Id (void) { return m_nId; }
};

//------------------------------------------------------------------------------

typedef struct tRenderParticle {
	CParticle*	particle;
	float			fBrightness;
	char			nFrame;
	char			nRotFrame;
} tRenderParticles;


class CParticleBuffer : public CEffectArea {
	public:
#if 1 //DBG
		CStaticArray< tRenderParticle, PART_BUF_SIZE> m_particles;
		CStaticArray< tParticleVertex, PART_BUF_SIZE * 4> m_vertices;
#else
		tRenderParticle m_particles [PART_BUF_SIZE];
		tParticleVertex m_vertices [PART_BUF_SIZE * 4];
#endif
		int m_iBuffer;
		int m_nType;
		char m_bEmissive;
		float m_dMax; // max. distance from viewer

		inline int GetType (void) { return m_nType; }
		inline void SetType (int nType) { m_nType = nType; }
		inline void Clear (void) {
			m_iBuffer = 0;
			m_nType = -1;
			m_bEmissive = 0;
			}
		void Setup (void);
		void Setup (int nThread);
		bool Flush (float brightness, bool bForce = false);
		bool Add (CParticle* particleP, float brightness, CFloatVector& pos, float rad);
		void Reset (void);
		bool AlphaControl (void);
		bool Compatible (CParticle* particleP);

		CParticleBuffer () : CEffectArea (), m_iBuffer (0), m_nType (-1), m_bEmissive (false), m_dMax (0.0f) {}

	private:
		static int bCompatible [2 * PARTICLE_TYPES];

		int Init (void);
		int UseParticleShader (void);
	};

//------------------------------------------------------------------------------

class CParticleManager {
	private:
		CDataPool<CParticleSystem>	m_systems;
		CArray<CParticleSystem*>	m_systemList;
		CArray<short>					m_objectSystems;
		CArray<time_t>					m_objExplTime;
		int								m_bAnimate;
		int								m_bStencil;
		int								m_iRenderBuffer;
		GLhandleARB						m_shaderProg;

	public:
		CParticleBuffer				particleBuffer [MAX_PARTICLE_BUFFERS];

	public:
		CParticleManager () : m_shaderProg (0) {}
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
						float fScale, /*int nDensity, int nPartsPerPos,*/ int nLife, int nSpeed, char nType,
						int nObject, CFloatVector *colorP, int bBlowUpParts, char nSide);
		int Destroy (int iParticleSystem);
		void Cleanup (void);
		int Shutdown (void);
		int AllocPartList (void);
		void FreePartList (void);

		int BeginRender (int nType, float fScale);
		int EndRender (void);
		int InitBuffer (void);
		int CloseBuffer (void);
		void SetupParticles (int nThread);

		void AdjustBrightness (CBitmap *bmP);

		inline time_t* ObjExplTime (int i = 0) { return m_objExplTime + i; }
		inline int Animate (void) { return m_bAnimate; }
		inline void SetAnimate (int bAnimate) { m_bAnimate = bAnimate; }
		inline int Stencil (void) { return m_bStencil; }
		inline void SetStencil (int bStencil) { m_bStencil = bStencil; }
		inline CParticleSystem& GetSystem (int i) { return m_systems [i]; }
		inline CParticleSystem* GetFirst (int& nCurrent) { return m_systems.GetFirst (nCurrent); }
		inline CParticleSystem* GetNext (int& nCurrent) { return m_systems.GetNext (nCurrent); }
		inline short GetObjectSystem (short nObject) { return  m_objectSystems.Buffer () ? m_objectSystems [nObject] : -1; }

		inline CParticleEmitter* GetEmitter (int i, int j)
		 { return GetSystem (i).GetEmitter (j); }

		inline void SetPos (int i, CFixVector *vPos, CFixMatrix *mOrient, short nSegment) {
			GetSystem (i).SetPos (vPos, mOrient, nSegment);
			}

		inline void SetDensity (int i, int nMaxParts, int nType = -1/*, int nDensity*/) {
			nMaxParts = MaxParticles (nMaxParts, gameOpts->render.particles.nDens [(nType < 0) ? 0 : nType]);
			GetSystem (i).SetDensity (nMaxParts/*, nDensity*/);
			}

		inline void SetScale (int i, float fScale) {
			GetSystem (i).SetScale (fScale);
			}

		inline void SetLife (int i, int nLife) {
			GetSystem (i).SetLife (nLife);
			}

		inline void SetBlowUp (int i, int bBlowUpParts) {
			GetSystem (i).SetBlowUp (bBlowUpParts);
			}

		inline void SetBrightness (int i, int nBrightness) {
			GetSystem (i).SetBrightness (nBrightness);
			}

		inline void SetFadeType (int i, int nFadeType) {
			GetSystem (i).SetFadeType (nFadeType);
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
			if ((nObject < 0) || (nObject >= LEVEL_OBJECTS))
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

		inline float ParticleSize (int nSize, float fScale, int bBlowUp = 1) {
			return (bBlowUp && gameOpts->render.particles.bDisperse)
					 ? FRound (float (PARTICLE_RAD * (nSize + 1)) / fScale)
					 : FRound (float (PARTICLE_RAD * (nSize + 1) * (nSize + 2) / 2) / fScale);
			}

		inline int RemoveEmitter (int i, int j)
		 { return GetSystem (i).RemoveEmitter (j); }

		inline int RenderBufPtr (void)
			{ return m_iRenderBuffer; }

		inline void IncRenderBufPtr (int i = 1)
			{ m_iRenderBuffer += i; }

		inline int LastType (void) { 
			for (int i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
				if (particleBuffer [i].GetType () >= 0)
					return particleBuffer [i].GetType (); 
				}
			return -1;
			}

		inline void SetLastType (int nType) { 
			particleBuffer [0].SetType (nType); 
			particleBuffer [1].SetType (nType); 
			}

		inline bool Overlap (CEffectArea& area) { 
			for (int i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
				if (particleBuffer [i] && area) 
					return true;
				}
			return false;
			}

		inline bool Overlap (CFloatVector& pos, float rad) { 
			for (int i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
				if (particleBuffer [i].Overlap (pos, rad)) 
					return true;
				}
			return false;
			}

		bool Add (CParticle* particleP, float brightness);

		bool Flush (float brightness, bool bForce = false);

		inline void ClearBuffers (void) {
			for (int i = 0; i < MAX_PARTICLE_BUFFERS; i++)
				particleBuffer [i].Clear ();
			}	

		bool LoadShader (int nShader, vec3& dMax);

		void UnloadShader (void);

		void InitShader (void);

	private:
		void RebuildSystemList (void);

		short Add (CParticle* particleP, float brightness, int nBuffer, bool& bFlushed);
};

extern CParticleManager particleManager;

//------------------------------------------------------------------------------

typedef struct tParticleImageInfo {
	CBitmap*		bmP;
	const char*	szName;
	int			nFrames;
	int			iFrame;
	int			bHave;
	int			bAnimate;
	float			xBorder;
	float			yBorder;
} tParticleImageInfo;

class CParticleImageManager {
	private:
		GLuint m_textureArray; // holds several particle images as texture array

	public:
		CParticleImageManager () : m_textureArray (0) {};
		~CParticleImageManager () {};
		int Load (int nType, int bForce = 0);
		int LoadAll (void);
		void FreeAll (void);
		bool SetupMultipleTextures (CBitmap* bmP1, CBitmap* bmP2, CBitmap* bmP3);
		bool LoadMultipleTextures (int nTMU);
		void Animate (int nType);
		void AdjustBrightness (CBitmap *bmP);
		int GetType (int nType);
};

extern CParticleImageManager particleImageManager;
extern tParticleImageInfo particleImageInfo [MAX_PARTICLE_QUALITY + 1][PARTICLE_TYPES];

//------------------------------------------------------------------------------

inline tParticleImageInfo& ParticleImageInfo (int nType)
{
return particleImageInfo [gameOpts->render.particles.nQuality][nType];
}

//------------------------------------------------------------------------------

#endif //__PARTICLES_H
//eof
