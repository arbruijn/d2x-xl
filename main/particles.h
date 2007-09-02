#ifndef __PARTICLES_H
#define __PARTICLES_H

#define EXTRA_VERTEX_ARRAYS	1

#define MAX_SMOKE 10000

#define MAX_PARTICLES(_nParts,_nDens)	MaxParticles (_nParts, _nDens)
#define PARTICLE_SIZE(_nSize,_nScale)	ParticleSize (_nSize, _nScale)

typedef struct tPartPos {
	double		x, y, z;
} tPartPos;

typedef struct tParticle {
#if !EXTRA_VERTEX_ARRAYS
	tPartPos		glPos;
#endif
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
	tRgbaColord	color;			//well ... the color, ya know =)
	char			nType;			//black or white
	char			nOrient;
	char			nRotDir;
	char			nBounce;
	char			bHaveDir;
	char			nFade;
	char			nClass;
} tParticle;

typedef struct tPartIdx {
	int			i;
	int			z;
} tPartIdx;

typedef struct tCloud {
	char			nType;			//black or white
	char			nClass;
	int			nLife;			//max. particle life time
	int			nBirth;			//time of creation
	int			nSpeed;			//initial particle speed
	int			nParts;			//curent no. of particles
	int			nFirstPart;
	int			nMaxParts;		//max. no. of particles
	int			nDensity;		//density (opaqueness) of smoke cloud
	float			fPartsPerTick;
	int			nTicks;
	int			nPartsPerPos;	//particles per interpolated position mutiplier of moving objects
	int			nPartLimit;		//highest max. part. no ever set for this cloud
	float			nPartScale;
	int			nDefBrightness;
	double		brightness;
	int			nMoved;			//time last moved
	short			nSegment;
	short			nObject;
	short			nObjType;
	short			nObjId;
	vmsVector	dir;
	vmsVector	pos;				//initial particle position
	vmsVector	prevPos;			//initial particle position
	ubyte			bHaveDir;		//movement direction given?
	ubyte			bHavePrevPos;	//valid previous position set?
	tParticle	*pParticles;	//list of active particles
	tPartIdx		*pPartIdx;
	tRgbaColord	color;
	int			bHaveColor;
} tCloud;

typedef struct tSmoke {
	int			nNext;
	int			nObject;
	short			nObjType;
	short			nObjId;
	int			nSignature;
	char			nType;			//black or white
	int			nBirth;			//time of creation
	int			nLife;			//max. particle life time
	int			nSpeed;			//initial particle speed
	int			nClouds;			//number of separate particle clouds
	int			nMaxClouds;		//max. no. of clouds
	tCloud		*pClouds;		//list of active clouds
} tSmoke;

int CreateSmoke (vmsVector *pPos, vmsVector *pDir,
					  short nSegment, int nMaxClouds, int nMaxParts, 
					  float nPartScale, int nDensity, int nPartsPerPos, 
					  int nLife, int nSpeed, char nType, int nObject,
					  tRgbaColord *pColor);
int DestroySmoke (int iSmoke);
int MoveSmoke ();
int RenderSmoke ();
int DestroyAllSmoke (void);
void SetSmokeDensity (int i, int nMaxParts, int nDensity);
void SetSmokePartScale (int i, float nPartScale);
void SetSmokePos (int i, vmsVector *pos);
void SetSmokeDir (int i, vmsVector *pDir);
void SetSmokeLife (int i, int nLife);
void SetSmokeType (int i, int nType);
void SetSmokeSpeed (int i, int nSpeed);
void SetSmokeBrightness (int i, int nBrightness);
tCloud *GetCloud (int i, int j);
int GetSmokeType (int i);
void FreeParticleImages (void);
void SetCloudPos (tCloud *pCloud, vmsVector *pos);
void InitSmoke (void);
int MaxParticles (int nParts, int nDens);
float ParticleSize (int nSize, float nScale);
int AllocPartList (void);
void FreePartList (void);

extern int bUseSmoke;
extern int nSmokeDensScale;

#endif //__PARTICLES_H
//eof
