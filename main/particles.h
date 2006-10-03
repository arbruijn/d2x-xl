#ifndef __PARTICLES_H
#define __PARTICLES_H

#define EXTRA_VERTEX_ARRAYS	1

typedef struct tPartColor {
	double		r, g, b, a;
} tPartColor;

typedef struct tPartPos {
	double		x, y, z;
} tPartPos;

typedef struct tParticle {
	tPartColor	glColor;			//well ... the color, ya know =)
#if !EXTRA_VERTEX_ARRAYS
	tPartPos		glPos;
#endif
	vms_vector	pos;				//position
	vms_vector	dir;				//movement direction
	char			nType;			//black or white
	char			nOrient;
	char			nRotDir;
	char			nBounce;
	int			nTTL;				//time to live
	int			nLife;			//remaining life time
	int			nDelay;			//time between creation and appearance
	int			nMoved;			//time last moved
	int			nWidth;
	int			nHeight;
	short			nSegment;
} tParticle;

typedef struct tCloud {
	int			nType;			//black or white
	int			nLife;			//max. particle life time
	int			nBirth;			//time of creation
	int			nSpeed;			//initial particle speed
	int			nParts;			//curent no. of particles
	int			nMaxParts;		//max. no. of particles
	int			nPartLimit;		//highest max. part. no ever set for this cloud
	float			nPartScale;
	int			nMoved;			//time last moved
	short			nSegment;
	vms_vector	pos;				//initial particle position
	vms_vector	prevPos;			//initial particle position
	int			bHavePrevPos;	//valid previous position set?
	tParticle	*pParticles;	//list of active particles
} tCloud;

typedef struct tSmoke {
	int			nNext;
	int			nObject;
	int			nType;			//black or white
	int			nLife;			//max. particle life time
	int			nSpeed;			//initial particle speed
	int			nClouds;			//number of separate particle clouds
	int			nMaxClouds;		//max. no. of clouds
	tCloud		*pClouds;		//list of active clouds
} tSmoke;

int CreateSmoke (vms_vector *pPos, short nSegment, int nMaxClouds, int nMaxParts, 
					  float nPartScale, int nLife, int nSpeed, int nType, int nObject);
int DestroySmoke (int iSmoke);
int MoveSmoke ();
int RenderSmoke ();
int DestroyAllSmoke (void);
void SetSmokeDensity (int i, int nMaxParts);
void SetSmokePartScale (int i, float nPartScale);
void SetSmokePos (int i, vms_vector *pos);
void SetSmokeLife (int i, int nLife);
void SetSmokeType (int i, int nType);
tCloud *GetCloud (int i, int j);
int GetSmokeType (int i);
void FreeParticleImages (void);
void SetCloudPos (tCloud *pCloud, vms_vector *pos);
void InitSmoke (void);

extern int bUseSmoke;
extern int nSmokeDensScale;

#endif //__PARTICLES_H
//eof
