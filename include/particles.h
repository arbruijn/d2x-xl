#ifndef __PARTICLES_H
#define __PARTICLES_H

#define EXTRA_VERTEX_ARRAYS	1

#define MAX_SMOKE 10000

#define SMOKE_PARTICLES		0
#define BUBBLE_PARTICLES	1
#define BULLET_PARTICLES	2
#define LIGHT_PARTICLES		3
#define GATLING_PARTICLES	4

#define MAX_PARTICLES(_nParts,_nDens)	MaxParticles (_nParts, _nDens)
#define PARTICLE_SIZE(_nSize,_nScale)	ParticleSize (_nSize, _nScale)

typedef struct tPartPos {
	float		x, y, z;
} tPartPos;

typedef struct tParticle {
#if !EXTRA_VERTEX_ARRAYS
	tPartPos		glPos;
#endif
	vmsMatrix	orient;
	vmsVector	pos;				//position
	vmsVector	transPos;		//transformed position
	vmsVector	dir;				//movement direction
	vmsVector	drift;
	int			nTTL;				//time to live
	int			nLife;			//remaining life time
	int			nDelay;			//time between creation and appearance
	int			nMoved;			//time last moved
	int			nWidth;
	int			nHeight;
	int			nRad;
	short			nSegment;
	tRgbaColorf	color [2];		//well ... the color, ya know =)
	char			nType;			//black or white
	char			nRotDir;
	char			nBounce;
	char			bHaveDir;
	char			bBlowUp;
	char			bBright;
	char			bEmissive;
	char			nFade;
	char			nClass;
	char			nFrame;
	char			nRotFrame;
	char			nOrient;
} tParticle;

typedef struct tPartIdx {
	int			i;
	int			z;
} tPartIdx;

typedef struct tParticleEmitter {
	char			nType;			//smoke/light trail (corona)
	char			nClass;
	int			nLife;			//max. particle life time
	int			nBirth;			//time of creation
	int			nSpeed;			//initial particle speed
	int			nParts;			//curent no. of particles
	int			nFirstPart;
	int			nMaxParts;		//max. no. of particles
	int			nDensity;		//density (opaqueness) of particle emitter
	float			fPartsPerTick;
	int			nTicks;
	int			nPartsPerPos;	//particles per interpolated position mutiplier of moving objects
	int			nPartLimit;		//highest max. part. no ever set for this emitter
	float			nPartScale;
	int			nDefBrightness;
	float			fBrightness;
	int			nMoved;			//time last moved
	short			nSegment;
	int			nObject;
	short			nObjType;
	short			nObjId;
	vmsMatrix	orient;
	vmsVector	dir;
	vmsVector	pos;				//initial particle position
	vmsVector	prevPos;			//initial particle position
	vmsVector	vEmittingFace [4];
	ubyte			bHaveDir;		//movement direction given?
	ubyte			bHavePrevPos;	//valid previous position set?
	tParticle	*pParticles;	//list of active particles
	tPartIdx		*pPartIdx;
	tRgbaColorf	color;
	char			bHaveColor;
	char			bBlowUpParts;	//blow particles up at their "birth"
	char			bEmittingFace;
} tParticleEmitter;

typedef struct tParticleSystem {
	int					nNext;
	int					nObject;
	short					nObjType;
	short					nObjId;
	int					nSignature;
	char					nType;			//black or white
	int					nBirth;			//time of creation
	int					nLife;			//max. particle life time
	int					nSpeed;			//initial particle speed
	int					nEmitters;		//number of separate particle emitters
	int					nMaxEmitters;	//max. no. of emitters
	tParticleEmitter	*emitterP;		//list of active emitters
} tParticleSystem;

int CreateParticleSystem (vmsVector *pPos, vmsVector *pDir, vmsMatrix *pOrient,
								  short nSegment, int nMaxEmitters, int nMaxParts, 
								  float nPartScale, int nDensity, int nPartsPerPos, 
								  int nLife, int nSpeed, char nType, int nObject,
								  tRgbaColorf *pColor, int bBlowUpParts, char nFace);
int DestroyParticleSystem (int iParticleSystem);
int UpdateParticleSystems ();
int RenderParticleSystems ();
int DestroyAllParticleSystems (void);
void SetParticleSystemDensity (int i, int nMaxParts, int nDensity);
void SetParticleSystemPartScale (int i, float nPartScale);
void SetParticleSystemPos (int i, vmsVector *pos, vmsMatrix *orient, short nSegment);
void SetParticleSystemDir (int i, vmsVector *pDir);
void SetParticleSystemLife (int i, int nLife);
void SetParticleSystemType (int i, int nType);
void SetParticleSystemSpeed (int i, int nSpeed);
void SetParticleSystemBrightness (int i, int nBrightness);
tParticleEmitter *GetParticleEmitter (int i, int j);
int GetParticleSystemType (int i);
void FreeParticleImages (void);
void SetParticleEmitterPos (tParticleEmitter *pEmitter, vmsVector *pos, vmsMatrix *orient, short nSegment);
void InitParticleSystems (void);
int MaxParticles (int nParts, int nDens);
float ParticleSize (int nSize, float nScale);
int AllocPartList (void);
void FreePartList (void);
int LoadParticleImages (void);
int BeginRenderParticleSystems (int nType, float nScale);
int EndRenderParticleSystems (tParticleEmitter *pEmitter);
int RenderParticle (tParticle *pParticle, float brightness);
int SetParticleSystemObject (int nObject, int nParticleSystem);
void FlushParticleBuffer (float brightness);
int InitParticleBuffer (int bLightmaps);
int CloseParticleBuffer (void);
int UpdateParticleEmitter (tParticleEmitter *pEmitter, int nCurTime, int nThread);
int RenderParticleEmitter (tParticleEmitter *pEmitter, int nThread);

extern int bUseParticleSystem;
extern int nParticleSystemDensScale;

#endif //__PARTICLES_H
//eof
