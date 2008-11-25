
//particle.h
//simple particle system handler

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "pstypes.h"
#include "inferno.h"
#include "error.h"
#include "u_mem.h"
#include "vecmat.h"
#include "hudmsg.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_fastrender.h"
#include "piggy.h"
#include "globvars.h"
#include "gameseg.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "render.h"
#include "transprender.h"
#include "objsmoke.h"
#include "glare.h"
#include "particles.h"
#include "renderthreads.h"

#ifdef __macosx__
#	include <OpenGL/gl.h>
#	include <OpenGL/glu.h>
#	include <OpenGL/glext.h>
#else
#	include <GL/gl.h>
#	include <GL/glu.h>
#	ifdef _WIN32
#		include <glext.h>
#		include <wglext.h>
#	else
#		include <GL/glext.h>
#		include <GL/glx.h>
#		include <GL/glxext.h>
#	endif
#endif

//------------------------------------------------------------------------------

#define MAKE_SMOKE_IMAGE 0

#define MT_PARTICLES	0

#define BLOWUP_PARTICLES 0		//blow particles up at "birth"

#define SMOKE_SLOWMO 0

#define SORT_CLOUDS 1

#define SORT_CLOUD_PARTS 0

#define OGL_VERTEX_ARRAYS	1

#define PARTICLE_TYPES	4

#define PARTICLE_FPS	30

#define PARTICLE_RAD	(F1_0)

#define PART_DEPTHBUFFER_SIZE 100000
#define PARTLIST_SIZE 1000000

static int bHavePartImg [2][PARTICLE_TYPES] = {{0,0,0,0},{0,0,0,0}};

static grsBitmap *bmpParticle [2][PARTICLE_TYPES] = {{NULL, NULL, NULL, NULL},{NULL, NULL, NULL, NULL}};
#if 0
static grsBitmap *bmpBumpMaps [2] = {NULL, NULL};
#endif

static const char *szParticleImg [2][PARTICLE_TYPES] = {
	{"particleSystem.tga", "bubble.tga", "bullcase.tga", "corona.tga"},
	{"particleSystem.tga", "bubble.tga", "bullcase.tga", "corona.tga"}
	};

static int nParticleFrames [2][PARTICLE_TYPES] = {{1,1,1,1},{1,1,1,1}};
static int iParticleFrames [2][PARTICLE_TYPES] = {{0,0,0,0},{0,0,0,0}};
#if 0
static int iPartFrameIncr  [2][PARTICLE_TYPES] = {{1,1,1,1},{1,1,1,1}};
#endif
static float alphaScale [5] = {5.0f / 5.0f, 5.0f / 5.0f, 3.0f / 5.0f, 2.0f / 5.0f, 1.0f / 5.0f};

#define PART_BUF_SIZE	4096
#define VERT_BUF_SIZE	(PART_BUF_SIZE * 4)

typedef struct tParticleVertex {
	tTexCoord2f	texCoord;
	tRgbaColorf	color;
	fVector3		vertex;
	} tParticleVertex;

static tParticleVertex particleBuffer [VERT_BUF_SIZE];
static float bufferBrightness = -1;
static char bBufferEmissive = 0;
static int iBuffer = 0;

#define SMOKE_START_ALPHA		(gameOpts->render.particles.bDisperse ? 96 : 128)

//	-----------------------------------------------------------------------------

void InitParticleSystems (void)
{
	int i, j;
#if OGL_VERTEX_BUFFERS
	GLfloat	pf = colorBuffer;

for (i = 0; i < VERT_BUFFER_SIZE; i++, pf++) {
	*pf++ = 1.0f;
	*pf++ = 1.0f;
	*pf++ = 1.0f;
	}
#endif
for (i = 0, j = 1; j < MAX_SMOKE; i++, j++) {
	gameData.particles.buffer [i].nNext = j;
	gameData.particles.buffer [i].nObject = -1;
	gameData.particles.buffer [i].nObjType = -1;
	gameData.particles.buffer [i].nObjId = -1;
	gameData.particles.buffer [i].nSignature = -1;
	}
gameData.particles.buffer [i].nNext = -1;
gameData.particles.iFree = 0;
gameData.particles.iUsed = -1;
}

//------------------------------------------------------------------------------

static void RebuildParticleSystemList (void)
{
gameData.particles.iUsed =
gameData.particles.iFree = -1;
tParticleSystem *systemP = gameData.particles.buffer;
for (int i = 0; i < MAX_SMOKE; i++, systemP++) {
	if (systemP->emitterP) {
		systemP->nNext = gameData.particles.iUsed;
		gameData.particles.iUsed = i;
		}
	if (systemP->emitterP) {
		systemP->nNext = gameData.particles.iFree;
		gameData.particles.iFree = i;
		}
	}
}

//------------------------------------------------------------------------------

void ResetDepthBuf (void)
{
memset (gameData.particles.depthBuf.pDepthBuffer, 0, PART_DEPTHBUFFER_SIZE * sizeof (struct tPartList **));
memset (gameData.particles.depthBuf.pPartList, 0, (PARTLIST_SIZE - gameData.particles.depthBuf.nFreeParts) * sizeof (struct tPartList));
gameData.particles.depthBuf.nFreeParts = PARTLIST_SIZE;
}

//	-----------------------------------------------------------------------------

int AllocPartList (void)
{
if (gameData.particles.depthBuf.pDepthBuffer)
	return 1;
if (!(gameData.particles.depthBuf.pDepthBuffer = (struct tPartList **) D2_ALLOC (PART_DEPTHBUFFER_SIZE * sizeof (struct tPartList *))))
	return 0;
if (!(gameData.particles.depthBuf.pPartList = (struct tPartList *) D2_ALLOC (PARTLIST_SIZE * sizeof (struct tPartList)))) {
	D2_FREE (gameData.particles.depthBuf.pDepthBuffer);
	return 0;
	}
gameData.particles.depthBuf.nFreeParts = 0;
ResetDepthBuf ();
return 1;
}

//	-----------------------------------------------------------------------------

void FreePartList (void)
{
D2_FREE (gameData.particles.depthBuf.pPartList);
D2_FREE (gameData.particles.depthBuf.pDepthBuffer);
}

//	-----------------------------------------------------------------------------
// 4: gatling projectile trail
// 3: air bubbles
// 2: bullet casings
// 1: light trails
// 0: particleSystem

static inline int ParticleImageType (int nType)
{
if (nType == SMOKE_PARTICLES)
	return SMOKE_PARTICLES;
if (nType == BULLET_PARTICLES)
	return BULLET_PARTICLES;
if ((nType == LIGHT_PARTICLES) || (nType == GATLING_PARTICLES))
	return LIGHT_PARTICLES;
if (nType == BUBBLE_PARTICLES)
	return BUBBLE_PARTICLES;
return -1;
}

//	-----------------------------------------------------------------------------

void AnimateParticle (int nType)
{
	int	bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.particles.bSort,
			nFrames = nParticleFrames [bPointSprites][nType];

if (nFrames > 1) {
	static time_t t0 [PARTICLE_TYPES] = {0, 0, 0, 0};

	time_t		t = gameStates.app.nSDLTicks;
	int			iFrame = iParticleFrames [bPointSprites][nType];
#if 0
	int			iFrameIncr = iPartFrameIncr [bPointSprites][nType];
#endif
	int			bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.particles.bSort;
	grsBitmap	*bmP = bmpParticle [gameStates.render.bPointSprites && !gameOpts->render.particles.bSort][ParticleImageType (nType)];

	if (!BM_FRAMES (bmP))
		return;
	BM_CURFRAME (bmP) = BM_FRAMES (bmP) + iFrame;
#if 1
	if (t - t0 [nType] > 150)
#endif
		{
		t0 [nType] = t;
#if 1
		iParticleFrames [bPointSprites][nType] = (iFrame + 1) % nFrames;
#else
		iFrame += iFrameIncr;
		if ((iFrame < 0) || (iFrame >= nFrames)) {
			iPartFrameIncr [bPointSprites][nType] = -iFrameIncr;
			iFrame += -2 * iFrameIncr;
			}
		iParticleFrames [bPointSprites][nType] = iFrame;
#endif
		}
	}
}

//	-----------------------------------------------------------------------------

void AdjustParticleBrightness (grsBitmap *bmP)
{
	grsBitmap	*bmfP;
	int			i, j = bmP->bmData.alt.bmFrameCount;
	float			*fFrameBright, fAvgBright = 0, fMaxBright = 0;

if (j < 2)
	return;
if (!(fFrameBright = (float *) D2_ALLOC (j * sizeof (float))))
	return;
for (i = 0, bmfP = BM_FRAMES (bmP); i < j; i++, bmfP++) {
	fAvgBright += (fFrameBright [i] = (float) TGABrightness (bmfP));
	if (fMaxBright < fFrameBright [i])
		fMaxBright = fFrameBright [i];
	}
fAvgBright /= j;
for (i = 0, bmfP = BM_FRAMES (bmP); i < j; i++, bmfP++) {
	TGAChangeBrightness (bmfP, 0, 1, 2 * (int) (255 * fFrameBright [i] * (fAvgBright - fFrameBright [i])), 0);
	}
D2_FREE (fFrameBright);
}

//	-----------------------------------------------------------------------------

int LoadParticleImage (int nType)
{
	int			h,
					bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.particles.bSort,
					*flagP;
	grsBitmap	*bmP = NULL;

nType = ParticleImageType (nType);
flagP = bHavePartImg [bPointSprites] + nType;
if (*flagP < 0)
	return 0;
if (*flagP > 0)
	return 1;
bmP = CreateAndReadTGA (szParticleImg [bPointSprites][nType]);
*flagP = bmP ? 1 : -1;
if (*flagP < 0)
	return 0;
bmpParticle [bPointSprites][nType] = bmP;
#if MAKE_SMOKE_IMAGE
{
	tTgaHeader h;

TGAInterpolate (bmP, 2);
if (TGAMakeSquare (bmP)) {
	memset (&h, 0, sizeof (h));
	SaveTGA (szParticleImg [bPointSprites][nType], gameFolders.szDataDir, &h, bmP);
	}
}
#endif
BM_FRAMECOUNT (bmP) = bmP->bmProps.h / bmP->bmProps.w;
#if 0
if (OglSetupBmFrames (BmOverride (bmP), 0, 0, 0)) {
	AdjustParticleBrightness (bmP);
	D2_FREE (BM_FRAMES (bmP));	// make sure frames get loaded to OpenGL in OglLoadBmTexture ()
	BM_CURFRAME (bmP) = NULL;
	}
#endif
OglLoadBmTexture (bmP, 0, 3, 1);
if (nType == SMOKE_PARTICLES)
	h = 8;
else if (nType == BUBBLE_PARTICLES)
	h = 4;
else
	h = BM_FRAMECOUNT (bmP);
nParticleFrames [bPointSprites][nType] = h;
return *flagP > 0;
}

//	-----------------------------------------------------------------------------

int LoadParticleImages (void)
{
	int	i;

for (i = 0; i < PARTICLE_TYPES; i++) {
	if (!LoadParticleImage (i))
		return 0;
	AnimateParticle (i);
	}
return 1;
}

//	-----------------------------------------------------------------------------

void FreeParticleImages (void)
{
	int	i, j;

for (i = 0; i < 2; i++)
	for (j = 0; j < PARTICLE_TYPES; j++)
		if (bmpParticle [i][j]) {
			GrFreeBitmap (bmpParticle [i][j]);
			bmpParticle [i][j] = NULL;
			bHavePartImg [i][j] = 0;
			}
}

//------------------------------------------------------------------------------

static inline int randN (int n)
{
if (!n)
	return 0;
return (int) ((float) rand () * (float) n / (float) RAND_MAX);
}

//------------------------------------------------------------------------------

inline float sqr (float n)
{
return n * n;
}

//------------------------------------------------------------------------------

inline float ParticleBrightness (tRgbaColorf *colorP)
{
#if 0
return (colorP->red + colorP->green + colorP->blue) / 3.0f;
#else
return colorP ? (colorP->red * 3 + colorP->green * 5 + colorP->blue * 2) / 10.0f : 1.0f;
#endif
}

//------------------------------------------------------------------------------

vmsVector *RandomPointOnQuad (vmsVector *quad, vmsVector *vPos)
{
	vmsVector	vOffs;
	int			i;

i = rand () % 2;
vOffs = quad[i+1] - quad[i];
vOffs *= (2 * d_rand ());
vOffs += quad[i];
i += 2;
*vPos = quad[(i+1)%4] - quad[i];
*vPos *= (2 * d_rand ());
*vPos += quad[i];
*vPos -= vOffs;
*vPos *= (2 * d_rand ());
*vPos += vOffs;
return vPos;
}

//------------------------------------------------------------------------------

#define RANDOM_FADE	(0.95f + (float) rand () / (float) RAND_MAX / 20.0f)

int CreateParticle (tParticle *particleP, vmsVector *pPos, vmsVector *pDir, vmsMatrix *pOrient,
						  short nSegment, int nLife, int nSpeed, char nParticleSystemType, char nClass,
						  float nScale, tRgbaColorf *colorP, int nCurTime, int bBlowUp,
						  float fBrightness, vmsVector *vEmittingFace)
{

	static tRgbaColorf	defaultColor = {1,1,1,1};

	tRgbaColorf	color;
	vmsVector	vDrift, vPos;
	int			nRad, nFrames, nType = ParticleImageType (nParticleSystemType);

if (vEmittingFace)
	pPos = RandomPointOnQuad (vEmittingFace, &vPos);
if (nScale < 0)
	nRad = (int) -nScale;
else if (gameOpts->render.particles.bSyncSizes)
	nRad = (int) PARTICLE_SIZE (gameOpts->render.particles.nSize [0], nScale);
else
	nRad = (int) nScale;
if (!nRad)
	nRad = F1_0;
particleP->nType = nType;
particleP->bEmissive = (nParticleSystemType == LIGHT_PARTICLES);
particleP->nClass = nClass;
particleP->nSegment = nSegment;
particleP->nBounce = 0;
color = colorP ? *colorP : defaultColor;
particleP->color [0] = 
particleP->color [1] = color;
if ((nType == BULLET_PARTICLES) || (nType == BUBBLE_PARTICLES)) {
	particleP->bBright = 0;
	particleP->nFade = -1;
	}
else {
	particleP->bBright = (nType == SMOKE_PARTICLES) ? (rand () % 50) == 0 : 0;
	if (colorP) {
		if (nType != LIGHT_PARTICLES) {
			particleP->color [0].red *= RANDOM_FADE;
			particleP->color [0].green *= RANDOM_FADE;
			particleP->color [0].blue *= RANDOM_FADE;
			}
		particleP->nFade = 0;
		}
	else {
		particleP->color [0].red = 1.0f;
		particleP->color [0].green = 0.5f;
		particleP->color [0].blue = 0.0f;
		particleP->nFade = 2;
		}
	if (particleP->bEmissive)
		particleP->color [0].alpha = (float) (SMOKE_START_ALPHA + 64) / 255.0f;
	else if (nParticleSystemType != GATLING_PARTICLES) {
		if (!colorP) 
			particleP->color [0].alpha = (float) (SMOKE_START_ALPHA + randN (64)) / 255.0f;
		else {
			if (colorP->alpha < 0)
				particleP->color [0].alpha = -colorP->alpha;
			else {
				if (2 == (particleP->nFade = (char) colorP->alpha)) {
					particleP->color [0].red = 1.0f;
					particleP->color [0].green = 0.5f;
					particleP->color [0].blue = 0.0f;
					}
				particleP->color [0].alpha = (float) (SMOKE_START_ALPHA + randN (64)) / 255.0f;
				}
			}
		}
#if 1
	if (gameOpts->render.particles.bDisperse && !particleP->bBright) {
		fBrightness = 1.0f - fBrightness;
		particleP->color [0].alpha += fBrightness * fBrightness / 8.0f;
		}
#endif
	}
//nSpeed = (int) (sqrt (nSpeed) * (float) F1_0);
nSpeed *= F1_0;
if (pDir) {
	vmsAngVec	a;
	vmsMatrix	m;
	float			d;
	a [PA] = randN (F1_0 / 4) - F1_0 / 8;
	a [BA] = randN (F1_0 / 4) - F1_0 / 8;
	a [HA] = randN (F1_0 / 4) - F1_0 / 8;
	m = vmsMatrix::Create (a);
	vDrift = m * (*pDir);
	vmsVector::Normalize (vDrift);
	d = (float) vmsVector::DeltaAngle (vDrift, *pDir, NULL);
	if (d) {
		d = (float) exp ((F1_0 / 8) / d);
		nSpeed = (fix) ((float) nSpeed / d);
		}
#if 0
	if (!colorP)	// hack for static particleSystem w/o user defined color
		particleP->color [0].green =
		particleP->color [0].blue = 1.0f;
#endif
	vDrift *= nSpeed;
	if ((nType == SMOKE_PARTICLES) || (nType == BUBBLE_PARTICLES))
		particleP->dir = *pDir * (3 * F1_0 / 4 + randN (16) * F1_0 / 64);
	else
		particleP->dir = *pDir;
	particleP->bHaveDir = 1;
	}
else {
	vmsVector	vOffs;
	vDrift [X] = nSpeed - randN (2 * nSpeed);
	vDrift [Y] = nSpeed - randN (2 * nSpeed);
	vDrift [Z] = nSpeed - randN (2 * nSpeed);
	vOffs = vDrift;
	particleP->dir.SetZero ();
	particleP->bHaveDir = 1;
	}
particleP->drift = vDrift;
if (vEmittingFace)
	particleP->pos = *pPos;
else if (nType != BUBBLE_PARTICLES)
	particleP->pos = *pPos + vDrift * (F1_0 / 64);
else {
	//particleP->pos = *pPos + vDrift * (F1_0 / 32);
	nSpeed = vDrift.Mag () / 16;
	vDrift = vmsVector::Avg ((*pOrient) [RVEC] * (nSpeed - randN (2 * nSpeed)), (*pOrient) [UVEC] * (nSpeed - randN (2 * nSpeed)));
	particleP->pos = *pPos + vDrift + (*pOrient) [FVEC] * (F1_0 / 2 - randN (F1_0));
#if 1
	particleP->drift.SetZero ();
#else
	vmsVector::Normalize (particleP->drift);
	particleP->drift *= F1_0 * 32;
#endif
	}
if ((nType != BUBBLE_PARTICLES) && pOrient) {
		vmsAngVec	vRot;
		vmsMatrix	mRot;

	vRot [BA] = 0;
	vRot [PA] = 2048 - ((d_rand () % 9) * 512);
	vRot [HA] = 2048 - ((d_rand () % 9) * 512);
	mRot = vmsMatrix::Create (vRot);
	//TODO: MM
	particleP->orient = *pOrient * mRot;
	//particleP->orient = *pOrient;
	}
if (nLife < 0)
	nLife = -nLife;
if (nType == SMOKE_PARTICLES) {
	if (gameOpts->render.particles.bDisperse)
		nLife = (nLife * 2) / 3;
	nLife = nLife / 2 + randN (nLife / 2);
	}
particleP->nLife =
particleP->nTTL = nLife;
particleP->nMoved = nCurTime;
particleP->nDelay = 0; //bStart ? randN (nLife) : 0;
if (nType == SMOKE_PARTICLES)
	nRad += randN (nRad);
else if (nType == BUBBLE_PARTICLES)
	nRad = nRad / 10 + randN (9 * nRad / 10);
else
	nRad *= 2;
if ((particleP->bBlowUp = bBlowUp)) {
	particleP->nRad = nRad;
	particleP->nWidth =
	particleP->nHeight = nRad / 2;
	}
else {
	particleP->nWidth =
	particleP->nHeight = nRad;
	particleP->nRad = nRad / 2;
	}
nFrames = nParticleFrames [gameStates.render.bPointSprites && !gameOpts->render.particles.bSort && (gameOpts->render.bDepthSort <= 0)][nType];
if (nType == BULLET_PARTICLES) {
	particleP->nFrame = 0;
	particleP->nRotFrame = 0;
	particleP->nOrient = 3;
	}
else if (nType == BUBBLE_PARTICLES) {
	particleP->nFrame = rand () % (nFrames * nFrames);
	particleP->nRotFrame = 0;
	particleP->nOrient = 0;
	}
else if (nType == LIGHT_PARTICLES) {
	particleP->nFrame = 0;
	particleP->nRotFrame = 0;
	particleP->nOrient = 0;
	}
else {
	particleP->nFrame = rand () % (nFrames * nFrames);
	particleP->nRotFrame = particleP->nFrame / 2;
	particleP->nOrient = rand () % 4;
	}
#if 1
if (particleP->bEmissive)
	particleP->color [0].alpha *= ParticleBrightness (colorP);
else if (nParticleSystemType == SMOKE_PARTICLES)
	particleP->color [0].alpha /= colorP ? color.red + color.green + color.blue + 2 : 2;
else if (nParticleSystemType == BUBBLE_PARTICLES)
	particleP->color [0].alpha /= 2;
else if (nParticleSystemType == LIGHT_PARTICLES)
	particleP->color [0].alpha /= 5;
else if (nParticleSystemType == GATLING_PARTICLES)
	;//particleP->color [0].alpha /= 6;
#endif
return 1;
}

//------------------------------------------------------------------------------

int DestroyParticle (tParticle *particleP)
{
#if DBG
memset (particleP, 0, sizeof (tParticle));
#endif
return 1;
}

//------------------------------------------------------------------------------

inline int ChangeDir (int d)
{
	int	h = d;

if (h)
	h = h / 2 - randN (h);
return (d * 10 + h) / 10;
}

//------------------------------------------------------------------------------

static int nPartSeg = -1;
static int nFaces [6];
static int vertexList [6][6];
static int bSidePokesOut [6];
//static int nVert [6];
static vmsVector	*wallNorm;

int CollideParticleAndWall (tParticle *particleP)
{
	tSegment		*segP;
	tSide			*sideP;
	int			bInit, nSide, nVert, nChild, nFace, nInFront;
	fix			nDist;
	int			*vlP;
	vmsVector	pos = particleP->pos;

//redo:

segP = gameData.segs.segments + particleP->nSegment;
if ((bInit = (particleP->nSegment != nPartSeg)))
	nPartSeg = particleP->nSegment;
for (nSide = 0, sideP = segP->sides; nSide < 6; nSide++, sideP++) {
	vlP = vertexList [nSide];
	if (bInit) {
		nFaces [nSide] = CreateAbsVertexLists (vlP, nPartSeg, nSide);
		if (nFaces [nSide] == 1) {
			bSidePokesOut [nSide] = 0;
			nVert = vlP [0];
			}
		else {
			nVert = min(vlP [0], vlP [2]);
			if (vlP [4] < vlP [1])
				nDist = gameData.segs.vertices[vlP[4]].DistToPlane (sideP->normals[0], gameData.segs.vertices[nVert]);
			else
				nDist = gameData.segs.vertices[vlP[1]].DistToPlane (sideP->normals[1], gameData.segs.vertices[nVert]);
			bSidePokesOut [nSide] = (nDist > PLANE_DIST_TOLERANCE);
			}
		}
	else
		nVert = (nFaces [nSide] == 1) ? vlP [0] : min(vlP [0], vlP [2]);
	for (nFace = nInFront = 0; nFace < nFaces [nSide]; nFace++) {
		nDist = pos.DistToPlane(sideP->normals[nFace], gameData.segs.vertices[nVert]);
		if (nDist > -PLANE_DIST_TOLERANCE)
			nInFront++;
		else
			wallNorm = sideP->normals + nFace;
		}
	if (!nInFront || (bSidePokesOut [nSide] && (nFaces [nSide] == 2) && (nInFront < 2))) {
		if (0 > (nChild = segP->children [nSide]))
			return 1;
		particleP->nSegment = nChild;
		break;
#if 0
		if (bRedo)
			break;
		bRedo = 1;
		goto redo;
#endif
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int UpdateParticle (tParticle *particleP, int nCurTime)
{
	int			j, nRad;
	short			nSegment;
	fix			t, dot;
	vmsVector	pos, drift;
	fix			drag = (particleP->nType == BUBBLE_PARTICLES) ? F1_0 : F2X ((float) particleP->nLife / (float) particleP->nTTL);

if ((particleP->nLife <= 0) /*|| (particleP->color [0].alpha < 0.01f)*/)
	return 0;
t = nCurTime - particleP->nMoved;
if (particleP->nDelay > 0)
	particleP->nDelay -= t;
else {
	pos = particleP->pos;
	drift = particleP->drift;
	if ((particleP->nType == SMOKE_PARTICLES) /*|| (particleP->nType == BUBBLE_PARTICLES)*/) {
		drift [X] = ChangeDir (drift [X]);
		drift [Y] = ChangeDir (drift [Y]);
		drift [Z] = ChangeDir (drift [Z]);
		}
	for (j = 0; j < 2; j++) {
		if (t < 0)
			t = -t;
		particleP->pos = pos + drift * t; //(t * F1_0 / 1000);
		if (particleP->bHaveDir) {
			vmsVector vi = drift, vj = particleP->dir;
			vmsVector::Normalize (vi);
			vmsVector::Normalize (vj);
//				if (vmsVector::Dot(drift, particleP->dir) < 0)
			if (vmsVector::Dot (vi, vj) < 0)
				drag = -drag;
//				VmVecScaleInc (&drift, &particleP->dir, drag);
			particleP->pos += particleP->dir * drag;
			}
		if (particleP->nTTL - particleP->nLife > F1_0 / 16) {
			nSegment = FindSegByPos (particleP->pos, particleP->nSegment, 0, 0, 1);
			if (nSegment < 0) {
#if DBG
				if (particleP->nSegment == nDbgSeg)
					nSegment = FindSegByPos (particleP->pos, particleP->nSegment, 1, 0, 1);
#endif
				nSegment = FindSegByPos (particleP->pos, particleP->nSegment, 0, 1, 1);
				if (nSegment < 0)
					return 0;
				}
			if ((particleP->nType == BUBBLE_PARTICLES) && (SEGMENT2S [nSegment].special != SEGMENT_IS_WATER))
				return 0;
			particleP->nSegment = nSegment;
			}
		if (gameOpts->render.particles.bCollisions && CollideParticleAndWall (particleP)) {	//Reflect the particle
			if (particleP->nType == BUBBLE_PARTICLES)
				return 0;
			if (j)
				return 0;
			else if (!(dot = vmsVector::Dot(drift, *wallNorm)))
				return 0;
			else {
				drift = particleP->drift + *wallNorm * (-2 * dot);
				//VmVecScaleAdd (&particleP->pos, &pos, &drift, 2 * t);
				particleP->nBounce = 3;
				continue;
				}
			}
		else if (particleP->nBounce)
			particleP->nBounce--;
		else {
			break;
			}
		}
	particleP->drift = drift;
	if (particleP->nTTL >= 0) {
#if SMOKE_SLOWMO
		particleP->nLife -= (int) (t / gameStates.gameplay.slowmo [0].fSpeed);
#else
		particleP->nLife -= t;
#endif
		if ((particleP->nType == SMOKE_PARTICLES) && (nRad = particleP->nRad)) {
			if (particleP->bBlowUp) {
				if (particleP->nWidth >= nRad)
					particleP->nRad = 0;
				else {
					particleP->nWidth += nRad / 10 / particleP->bBlowUp;
					particleP->nHeight += nRad / 10 / particleP->bBlowUp;
					if (particleP->nWidth > nRad)
						particleP->nWidth = nRad;
					if (particleP->nHeight > nRad)
						particleP->nHeight = nRad;
					particleP->color [0].alpha *= (1.0f + 0.0725f / particleP->bBlowUp);
					if (particleP->color [0].alpha > 1)
						particleP->color [0].alpha = 1;
					}
				}
			else {
				if (particleP->nWidth <= nRad)
					particleP->nRad = 0;
				else {
					particleP->nRad += nRad / 5;
					particleP->color [0].alpha *= 1.0725f;
					if (particleP->color [0].alpha > 1)
						particleP->color [0].alpha = 1;
					}
				}
			}
		}
	}
particleP->nMoved = nCurTime;
return 1;
}

//------------------------------------------------------------------------------

void FlushParticleBuffer (float brightness)
{
if (bufferBrightness < 0)
	bufferBrightness = brightness;
if (iBuffer) {
	tRgbaColorf	color = {bufferBrightness, bufferBrightness, bufferBrightness, 1};
	int bLightmaps = HaveLightmaps ();
	bufferBrightness = brightness;
	glEnable (GL_BLEND);
	//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc (GL_LEQUAL);
	glDepthMask (0);
	if (gameStates.ogl.bShadersOk) {
		if (InitParticleBuffer (bLightmaps)) { //gameStates.render.bVertexArrays) {
#if 1
			grsBitmap *bmP = bmpParticle [0][gameData.particles.nLastType];
			if (!bmP)
				return;
			glActiveTexture (GL_TEXTURE0);
			glClientActiveTexture (GL_TEXTURE0);
			glEnable (GL_TEXTURE_2D);
			if (BM_CURFRAME (bmP))
				bmP = BM_CURFRAME (bmP);
			if (OglBindBmTex (bmP, 0, 1))
				return;
#endif
			if (gameData.render.lights.dynamic.headlights.nLights && !(gameStates.render.automap.bDisplay || gameData.particles.nLastType))
				G3SetupHeadlightShader (1, 0, &color);
			else if ((gameOpts->render.effects.bSoftParticles & 4) && (gameData.particles.nLastType <= BUBBLE_PARTICLES))
				LoadGlareShader (10);
			else if (gameStates.render.history.nShader >= 0) {
				glUseProgramObject (0);
				gameStates.render.history.nShader = -1;
				}
#if 0
			else if (!bLightmaps)
				G3SetupShader (NULL, 0, 0, 1, 1, &color);
#endif
			}
		glNormal3f (0, 0, 0);
		glDrawArrays (GL_QUADS, 0, iBuffer);
		}
	else {
		tParticleVertex *pb;
		glEnd ();
		glNormal3f (0, 0, 0);
		glBegin (GL_QUADS);
		for (pb = particleBuffer; iBuffer; iBuffer--, pb++) {
			glTexCoord2fv ((GLfloat *) &pb->texCoord);
			glColor4fv ((GLfloat *) &pb->color);
			glVertex3fv ((GLfloat *) &pb->vertex);
			}
		glEnd ();
		}
	iBuffer = 0;
	glEnable (GL_DEPTH_TEST);
	if ((gameStates.ogl.bShadersOk && !gameData.particles.nLastType) && (gameStates.render.history.nShader != 999)) {
		glUseProgramObject (0);
		gameStates.render.history.nShader = -1;
		}
	}
}

//------------------------------------------------------------------------------

#define PARTICLE_POSITIONS 64

int RenderParticle (tParticle *particleP, float brightness)
{
	vmsVector			hp;
	GLfloat				d, u, v;
	grsBitmap			*bmP;
	tRgbaColorf			pc;
	tTexCoord2f			texCoord [4];
	tParticleVertex	*pb;
	fVector				vOffset, vCenter;
	int					i, nFrame, nType = particleP->nType, bEmissive = particleP->bEmissive,
							bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.particles.bSort && (gameOpts->render.bDepthSort <= 0);
	float					decay = (nType == BUBBLE_PARTICLES) ? 1.0f : (float) particleP->nLife / (float) particleP->nTTL;

	static int			nFrames = 1;
	static float		deltaUV = 1.0f;
	static tSinCosf	sinCosPart [PARTICLE_POSITIONS];
	static int			bInitSinCos = 1;
	static fMatrix		mRot;

if (particleP->nDelay > 0)
	return 0;
if (!(bmP = bmpParticle [0][nType]))
	return 0;
if (BM_CURFRAME (bmP))
	bmP = BM_CURFRAME (bmP);
if (gameOpts->render.bDepthSort > 0) {
	hp = particleP->transPos;
	if ((gameData.particles.nLastType != nType) || (brightness != bufferBrightness) || (bBufferEmissive != bEmissive)) {
		if (gameStates.render.bVertexArrays)
			FlushParticleBuffer (brightness);
		gameData.particles.nLastType = nType;
		bBufferEmissive = bEmissive;
		glActiveTexture (GL_TEXTURE0);
		glClientActiveTexture (GL_TEXTURE0);
		if (OglBindBmTex (bmP, 0, 1))
			return 0;
		nFrames = nParticleFrames [0][nType];
		deltaUV = 1.0f / (float) nFrames;
		if (particleP->bEmissive)
			glBlendFunc (GL_ONE, GL_ONE);
		else
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	if (!gameStates.render.bVertexArrays)
		glBegin (GL_QUADS);
	}
else if (gameOpts->render.particles.bSort) {
	hp = particleP->transPos;
	if ((gameData.particles.nLastType != nType) || (brightness != bufferBrightness)) {
		if (gameStates.render.bVertexArrays)
			FlushParticleBuffer (brightness);
		else
			glEnd ();
		gameData.particles.nLastType = nType;
		if (OglBindBmTex (bmP, 0, 1))
			return 0;
		nFrames = nParticleFrames [bPointSprites][nType];
		deltaUV = 1.0f / (float) nFrames;
		if (particleP->bEmissive)
			glBlendFunc (GL_ONE, GL_ONE);
		else
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (!gameStates.render.bVertexArrays)
			glBegin (GL_QUADS);
		}
	}
else
	G3TransformPoint(hp, particleP->pos, 0);
if (particleP->bBright)
	brightness = (float) sqrt (brightness);
if (nType == SMOKE_PARTICLES) {
	if (particleP->nFade > 0) {
		if (particleP->color [0].green < particleP->color [1].green) {
#if SMOKE_SLOWMO
			particleP->color [0].green += 1.0f / 20.0f / (float) gameStates.gameplay.slowmo [0].fSpeed;
#else
			particleP->color [0].green += 1.0f / 20.0f;
#endif
			if (particleP->color [0].green > particleP->color [1].green) {
				particleP->color [0].green = particleP->color [1].green;
				particleP->nFade--;
				}
			}
		if (particleP->color [0].blue < particleP->color [1].blue) {
#if SMOKE_SLOWMO
			particleP->color [0].blue += 1.0f / 10.0f / (float) gameStates.gameplay.slowmo [0].fSpeed;
#else
			particleP->color [0].blue += 1.0f / 10.0f;
#endif
			if (particleP->color [0].blue > particleP->color [1].blue) {
				particleP->color [0].blue = particleP->color [1].blue;
				particleP->nFade--;
				}
			}
		}
	else if (particleP->nFade == 0) {
		particleP->color [0].red = particleP->color [1].red * RANDOM_FADE;
		particleP->color [0].green = particleP->color [1].green * RANDOM_FADE;
		particleP->color [0].blue = particleP->color [1].blue * RANDOM_FADE;
		particleP->nFade = -1;
		}
	}
pc = particleP->color [0];
//pc.alpha *= /*gameOpts->render.particles.bDisperse ? decay2 :*/ decay;
if ((nType == SMOKE_PARTICLES) || (nType == BUBBLE_PARTICLES)) {
	char nFrame = ((nType == BUBBLE_PARTICLES) && !gameOpts->render.particles.bWobbleBubbles) ? 0 : particleP->nFrame;
	u = (float) (nFrame % nFrames) * deltaUV;
	v = (float) (nFrame / nFrames) * deltaUV;
	d = deltaUV;
	}
else {
	u = v = 0.0f;
	d = 1.0f;
	}
if (nType == SMOKE_PARTICLES) {
#if 0
	if (SHOW_DYN_LIGHT) {
		tFaceColor *psc = AvgSgmColor (particleP->nSegment, NULL);
		pc.red *= (float) psc->color.red;
		pc.green *= (float) psc->color.green;
		pc.blue *= (float) psc->color.blue;
		}
#endif
	brightness *= 0.9f + (float) (rand () % 1000) / 5000.0f;
	pc.red *= brightness;
	pc.green *= brightness;
	pc.blue *= brightness;
	}
vCenter = hp.ToFloat ();
i = particleP->nOrient;
if (bEmissive) { //scale light trail particle color to reduce saturation
	pc.red /= 50.0f;
	pc.green /= 50.0f;
	pc.blue /= 50.0f;
	}
else if (particleP->nType != BUBBLE_PARTICLES) {
#if 0
	pc.alpha = (pc.alpha - 0.005f) * decay + 0.005f;
#	if 1
	pc.alpha = (float) pow (pc.alpha * pc.alpha, 1.0 / 3.0);
#	endif
#else
	float fFade = (float) cos ((double) sqr (1 - decay) * Pi) / 2 + 0.5f;
	pc.alpha *= fFade;
#endif
	pc.alpha *= alphaScale [gameOpts->render.particles.nAlpha [gameOpts->render.particles.bSyncSizes ? 0 : particleP->nClass]];
	}
if (pc.alpha < 1.0 / 255.0) {
	particleP->nLife = 0;
	return 0;
	}
#if 0
if (!gameData.render.lights.dynamic.headlights.nLights && (pc.red + pc.green + pc.blue < 0.001)) {
	particleP->nLife = 0;
	return 0;
	}
#endif
if ((nType == SMOKE_PARTICLES) && gameOpts->render.particles.bDisperse) {
#if 0
	decay = (float) sqrt (decay);
#else
	decay = (float) pow (decay * decay * decay, 1.0f / 5.0f);
#endif
	vOffset [X] = X2F (particleP->nWidth) / decay;
	vOffset [Y] = X2F (particleP->nHeight) / decay;
	}
else {
	vOffset [X] = X2F (particleP->nWidth) * decay;
	vOffset [Y] = X2F (particleP->nHeight) * decay;
	}
if (gameStates.render.bVertexArrays) {
	vOffset [Z] = 0;
	pb = particleBuffer + iBuffer;
	pb [i].texCoord.v.u =
	pb [(i + 3) % 4].texCoord.v.u = u;
	pb [i].texCoord.v.v =
	pb [(i + 1) % 4].texCoord.v.v = v;
	pb [(i + 1) % 4].texCoord.v.u =
	pb [(i + 2) % 4].texCoord.v.u = u + d;
	pb [(i + 2) % 4].texCoord.v.v =
	pb [(i + 3) % 4].texCoord.v.v = v + d;
	pb [0].color =
	pb [1].color =
	pb [2].color =
	pb [3].color = pc;
	if ((nType == BUBBLE_PARTICLES) && gameOpts->render.particles.bWiggleBubbles)
		vCenter [X] += (float) sin (particleP->nFrame / 4.0f * Pi) / (10 + rand () % 6);
	if (!nType && gameOpts->render.particles.bRotate) {
		if (bInitSinCos) {
			OglComputeSinCos (sizeofa (sinCosPart), sinCosPart);
			bInitSinCos = 0;
			mRot [RVEC][Z] =
			mRot [UVEC][Z] =
			mRot [FVEC][X] =
			mRot [FVEC][Y] = 0;
			mRot [FVEC][Z] = 1;
			}
		nFrame = (particleP->nOrient & 1) ? 63 - particleP->nRotFrame : particleP->nRotFrame;
		mRot [RVEC][X] =
		mRot [UVEC][Y] = sinCosPart [nFrame].fCos;
		mRot [UVEC][X] = sinCosPart [nFrame].fSin;
		mRot [RVEC][Y] = -mRot [UVEC][X];
		vOffset = mRot * vOffset;
		pb [0].vertex [X] = vCenter [X] - vOffset [X];
		pb [0].vertex [Y] = vCenter [Y] + vOffset [Y];
		pb [1].vertex [X] = vCenter [X] + vOffset [Y];
		pb [1].vertex [Y] = vCenter [Y] + vOffset [X];
		pb [2].vertex [X] = vCenter [X] + vOffset [X];
		pb [2].vertex [Y] = vCenter [Y] - vOffset [Y];
		pb [3].vertex [X] = vCenter [X] - vOffset [Y];
		pb [3].vertex [Y] = vCenter [Y] - vOffset [X];
		pb [0].vertex [Z] =
		pb [1].vertex [Z] =
		pb [2].vertex [Z] =
		pb [3].vertex [Z] = vCenter [Z];
		}
	else {
		pb [0].vertex [X] =
		pb [3].vertex [X] = vCenter [X] - vOffset [X];
		pb [1].vertex [X] =
		pb [2].vertex [X] = vCenter [X] + vOffset [X];
		pb [0].vertex [Y] =
		pb [1].vertex [Y] = vCenter [Y] + vOffset [Y];
		pb [2].vertex [Y] =
		pb [3].vertex [Y] = vCenter [Y] - vOffset [Y];
		pb [0].vertex [Z] =
		pb [1].vertex [Z] =
		pb [2].vertex [Z] =
		pb [3].vertex [Z] = vCenter [Z];
		}
	iBuffer += 4;
	if (iBuffer >= VERT_BUF_SIZE)
		FlushParticleBuffer (brightness);
	}
else {
	texCoord [0].v.u =
	texCoord [3].v.u = u;
	texCoord [0].v.v =
	texCoord [1].v.v = v;
	texCoord [1].v.u =
	texCoord [2].v.u = u + d;
	texCoord [2].v.v =
	texCoord [3].v.v = v + d;
	glColor4fv ((GLfloat *) &pc);
	glTexCoord2fv ((GLfloat *) (texCoord + i));
	glVertex3f (vCenter[X] - vOffset[X], vCenter[Y] + vOffset[Y], vCenter[Z]);
	glTexCoord2fv ((GLfloat *) (texCoord + (i + 1) % 4));
	glVertex3f (vCenter[X] + vOffset[X], vCenter[Y] + vOffset[Y], vCenter[Z]);
	glTexCoord2fv ((GLfloat *) (texCoord + (i + 2) % 4));
	glVertex3f (vCenter[X] + vOffset[X], vCenter[Y] - vOffset[Y], vCenter[Z]);
	glTexCoord2fv ((GLfloat *) (texCoord + (i + 3) % 4));
	glVertex3f (vCenter[X] - vOffset[X], vCenter[Y] - vOffset[Y], vCenter[Z]);
	}
if (gameData.particles.bAnimate) {
	particleP->nFrame = (particleP->nFrame + 1) % (nFrames * nFrames);
	if (!(nType || (particleP->nFrame & 1)))
		particleP->nRotFrame = (particleP->nRotFrame + 1) % 64;
	}
if (gameOpts->render.bDepthSort > 0)
	glEnd ();
return 1;
}

//------------------------------------------------------------------------------

int InitParticleBuffer (int bLightmaps)
{
if (gameStates.render.bVertexArrays) {
	G3DisableClientStates (1, 1, 1, GL_TEXTURE2);
	G3DisableClientStates (1, 1, 1, GL_TEXTURE1);
	if (bLightmaps) {
		OGL_BINDTEX (0);
		glDisable (GL_TEXTURE_2D);
		G3DisableClientStates (1, 1, 1, GL_TEXTURE3);
		}
	G3EnableClientStates (1, 1, 0, GL_TEXTURE0/* + bLightmaps*/);
	}
if (gameStates.render.bVertexArrays) {
	glTexCoordPointer (2, GL_FLOAT, sizeof (tParticleVertex), &particleBuffer [0].texCoord);
	glColorPointer (4, GL_FLOAT, sizeof (tParticleVertex), &particleBuffer [0].color);
	glVertexPointer (3, GL_FLOAT, sizeof (tParticleVertex), &particleBuffer [0].vertex);
	}
return gameStates.render.bVertexArrays;
}

//------------------------------------------------------------------------------

int CloseParticleBuffer (void)
{
if (!gameStates.render.bVertexArrays)
	return 0;
FlushParticleBuffer (-1);
G3DisableClientStates (1, 1, 0, GL_TEXTURE0 + HaveLightmaps ());
return 1;
}

//------------------------------------------------------------------------------

int BeginRenderParticleSystems (int nType, float nScale)
{
	grsBitmap	*bmP;
	int			bLightmaps = HaveLightmaps ();
	static time_t	t0 = 0;

if (gameOpts->render.bDepthSort <= 0) {
	nType = (nType % PARTICLE_TYPES);
	if ((nType >= 0) && !gameOpts->render.particles.bSort)
		AnimateParticle (nType);
	bmP = bmpParticle [0][nType];
	gameData.particles.bStencil = StencilOff ();
	InitParticleBuffer (bLightmaps);
	glActiveTexture (GL_TEXTURE0);
	glClientActiveTexture (GL_TEXTURE0);
	glDisable (GL_CULL_FACE);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_TEXTURE_2D);
	if ((nType >= 0) && OglBindBmTex (bmP, 0, 1))
		return 0;
	glDepthFunc (GL_LESS);
	glDepthMask (0);
	iBuffer = 0;
	if (!gameStates.render.bVertexArrays)
		glBegin (GL_QUADS);
	}
gameData.particles.nLastType = -1;
if (gameStates.app.nSDLTicks - t0 < 33)
	gameData.particles.bAnimate = 0;
else {
	t0 = gameStates.app.nSDLTicks;
	gameData.particles.bAnimate = 1;
	}
return 1;
}

//------------------------------------------------------------------------------

int EndRenderParticleSystems (tParticleEmitter *emitterP)
{
if (gameOpts->render.bDepthSort <= 0) {
	if (!CloseParticleBuffer ())
		glEnd ();
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	glDepthMask (1);
	StencilOn (gameData.particles.bStencil);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
return 1;
}

//------------------------------------------------------------------------------

char ParticleSystemObjClass (int nObject)
{
if ((nObject >= 0) && (nObject < 0x70000000)) {
	tObject	*objP = OBJECTS + nObject;
	if (objP->info.nType == OBJ_PLAYER)
		return 1;
	if (objP->info.nType == OBJ_ROBOT)
		return 2;
	if (objP->info.nType == OBJ_WEAPON)
		return 3;
	if (objP->info.nType == OBJ_DEBRIS)
		return 4;
	}
return 0;
}

//------------------------------------------------------------------------------

float EmitterBrightness (tParticleEmitter *emitterP)
{
	tObject	*objP;

if (emitterP->nObject >= 0x70000000)
	return 0.5f;
if (emitterP->nType > 2)
	return 1.0f;
if (emitterP->nObject < 0)
	return emitterP->fBrightness;
if (emitterP->nObjType == OBJ_EFFECT)
	return (float) emitterP->nDefBrightness / 100.0f;
if (emitterP->nObjType == OBJ_DEBRIS)
	return 0.5f;
if ((emitterP->nObjType == OBJ_WEAPON) && (emitterP->nObjId == PROXMINE_ID))
	return 0.2f;
objP = OBJECTS + emitterP->nObject;
if ((objP->info.nType != emitterP->nObjType) || (objP->info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED | OF_ARMAGEDDON)))
	return emitterP->fBrightness;
return emitterP->fBrightness = (float) ObjectDamage (objP) * 0.5f + 0.1f;
}

//------------------------------------------------------------------------------

#if MT_PARTICLES

int RunEmitterThread (tParticleEmitter *emitterP, int nCurTime, tRenderTask nTask)
{
int	i;

if (!gameStates.app.bMultiThreaded)
	return 0;
while (tiRender.ti [0].bExec && tiRender.ti [1].bExec)
	G3_SLEEP (0);
i = tiRender.ti [0].bExec ? 1 : 0;
tiRender.emitters [i] = emitterP;
tiRender.nCurTime [i] = nCurTime;
tiRender.nTask = nTask;
tiRender.ti [i].bExec = 1;
return 1;
}

#endif

//------------------------------------------------------------------------------

int CreateEmitter (tParticleEmitter *emitterP, vmsVector *pPos, vmsVector *pDir, vmsMatrix *pOrient,
					  short nSegment, int nObject, int nMaxParts, float nPartScale,
					  int nDensity, int nPartsPerPos, int nLife, int nSpeed, char nType,
					  tRgbaColorf *colorP, int nCurTime, int bBlowUpParts, vmsVector *vEmittingFace)
{
if (!(emitterP->pParticles = (tParticle *) D2_ALLOC (nMaxParts * sizeof (tParticle))))
	return 0;
#if SORT_CLOUD_PARTS
if (gameOpts->render.particles.bSort) {
	if (!(emitterP->pPartIdx = (tPartIdx *) D2_ALLOC (nMaxParts * sizeof (tPartIdx))))
		gameOpts->render.particles.bSort = 0;
	}
else
#endif
	emitterP->pPartIdx = NULL;
emitterP->nLife = nLife;
emitterP->nBirth = nCurTime;
emitterP->nSpeed = nSpeed;
emitterP->nType = nType;
if ((emitterP->bHaveColor = (colorP != NULL)))
	emitterP->color = *colorP;
if ((emitterP->bHaveDir = (pDir != NULL)))
	emitterP->dir = *pDir;
emitterP->prevPos =
emitterP->pos = *pPos;
if (pOrient)
	emitterP->orient = *pOrient;
else
	emitterP->orient = vmsMatrix::IDENTITY;
emitterP->bHavePrevPos = 1;
emitterP->bBlowUpParts = bBlowUpParts;
emitterP->nParts = 0;
emitterP->nMoved = nCurTime;
emitterP->nPartLimit =
emitterP->nMaxParts = nMaxParts;
emitterP->nFirstPart = 0;
emitterP->nPartScale = nPartScale;
emitterP->nDensity = nDensity;
emitterP->nPartsPerPos = nPartsPerPos;
emitterP->nSegment = nSegment;
emitterP->nObject = nObject;
if ((nObject >= 0) && (nObject < 0x70000000)) {
	emitterP->nObjType = OBJECTS [nObject].info.nType;
	emitterP->nObjId = OBJECTS [nObject].info.nId;
	}
emitterP->nClass = ParticleSystemObjClass (nObject);
emitterP->fPartsPerTick = (float) nMaxParts / (float) abs (nLife);
emitterP->nTicks = 0;
emitterP->nDefBrightness = 0;
if ((emitterP->bEmittingFace = (vEmittingFace != NULL)))
	memcpy (emitterP->vEmittingFace, vEmittingFace, sizeof (emitterP->vEmittingFace));
emitterP->fBrightness = (nObject < 0) ? 0.5f : EmitterBrightness (emitterP);
return 1;
}

//------------------------------------------------------------------------------

int DestroyEmitter (tParticleEmitter *emitterP)
{
if (emitterP->pParticles) {
	D2_FREE (emitterP->pParticles);
	emitterP->pParticles = NULL;
	D2_FREE (emitterP->pPartIdx);
	emitterP->pPartIdx = NULL;
	}
emitterP->nParts =
emitterP->nMaxParts = 0;
return 1;
}

//------------------------------------------------------------------------------

inline int EmitterLives (tParticleEmitter *emitterP, int nCurTime)
{
return (emitterP->nLife < 0) || (emitterP->nBirth + emitterP->nLife > nCurTime);
}

//------------------------------------------------------------------------------

inline int EmitterIsDead (tParticleEmitter *emitterP, int nCurTime)
{
return !(EmitterLives (emitterP, nCurTime) || emitterP->nParts);
}

//------------------------------------------------------------------------------

#if 0

void CheckEmitter (tParticleEmitter *emitterP)
{
	int	i, j;

for (i = emitterP->nParts, j = emitterP->nFirstPart; i; i--, j = (j + 1) % emitterP->nPartLimit)
	if (emitterP->pParticles [j].nType < 0)
		j = j;
}

#endif

//------------------------------------------------------------------------------

int UpdateParticleEmitter (tParticleEmitter *emitterP, int nCurTime, int nThread)
{
#if MT_PARTICLES
if ((nThread < 0) && RunEmitterThread (emitterP, nCurTime, rtUpdateParticles)) {
	return 0;
	}
else
#endif
	{
		tParticleEmitter		c = *emitterP;
		int			t, h, i, j;
		float			fDist;
		float			fBrightness = EmitterBrightness (emitterP);
		vmsMatrix	mOrient = emitterP->orient;
		vmsVector	vDelta, vPos, *pDir = (emitterP->bHaveDir ? &emitterP->dir : NULL),
						*vEmittingFace = c.bEmittingFace ? c.vEmittingFace : NULL;
		fVector		vDeltaf, vPosf;

#if SMOKE_SLOWMO
	t = (int) ((nCurTime - c.nMoved) / gameStates.gameplay.slowmo [0].fSpeed);
#else
	t = nCurTime - c.nMoved;
#endif
	nPartSeg = -1;
	for (i = c.nParts, j = c.nFirstPart; i; i--, j = (j + 1) % c.nPartLimit)
		if (!UpdateParticle (c.pParticles + j, nCurTime)) {
			if (j != c.nFirstPart)
				c.pParticles [j] = c.pParticles [c.nFirstPart];
			c.nFirstPart = (c.nFirstPart + 1) % c.nPartLimit;
			c.nParts--;
			}
	c.nTicks += t;
	if ((c.nPartsPerPos = (int) (c.fPartsPerTick * c.nTicks)) >= 1) {
		if (c.nType == BUBBLE_PARTICLES) {
			if (rand () % 4)	// create some irregularity in bubble appearance
				goto funcExit;
			}
		c.nTicks = 0;
		if (EmitterLives (emitterP, nCurTime)) {
			vDelta = c.pos - c.prevPos;
			fDist = X2F (vDelta.Mag());
			h = c.nPartsPerPos;
			if (h > c.nMaxParts - i)
				h = c.nMaxParts - i;
			if (h <= 0)
				goto funcExit;
			if (c.bHavePrevPos && (fDist > 0)) {
				vPosf = c.prevPos.ToFloat();
				vDeltaf = vDelta.ToFloat();
				vDeltaf [X] /= (float) h;
				vDeltaf [Y] /= (float) h;
				vDeltaf [Z] /= (float) h;
				}
			else if (/*(c.nType == LIGHT_PARTICLES) ||*/ (c.nType == BULLET_PARTICLES))
				goto funcExit;
			else {
#if 1
				vPosf = c.prevPos.ToFloat();
				vDeltaf = vDelta.ToFloat();
				vDeltaf [X] /= (float) h;
				vDeltaf [Y] /= (float) h;
				vDeltaf [Z] /= (float) h;
#else
				VmVecFixToFloat (&vPosf, &c.pos);
				vDeltaf [X] =
				vDeltaf [Y] =
				vDeltaf [Z] = 0.0f;
				h = 1;
#endif
				}
			c.nParts += h;
			for (; h; h--, j = (j + 1) % c.nPartLimit) {
				vPosf += vDeltaf;
				vPos = vPosf.ToFix();
/*
				vPos[Y] = (fix) (vPosf [Y] * 65536.0f);
				vPos[Z] = (fix) (vPosf [Z] * 65536.0f);
*/
				CreateParticle (c.pParticles + j, &vPos, pDir, &mOrient, c.nSegment, c.nLife,
									 c.nSpeed, c.nType, c.nClass, c.nPartScale, c.bHaveColor ? &c.color : NULL,
									 nCurTime, c.bBlowUpParts, fBrightness, vEmittingFace);
				}
			}
		}

funcExit:

	emitterP->bHavePrevPos = 1;
	emitterP->nMoved = nCurTime;
	emitterP->prevPos = emitterP->pos;
	emitterP->nTicks = c.nTicks;
	emitterP->nFirstPart = c.nFirstPart;
	return emitterP->nParts = c.nParts;
	}
}

//------------------------------------------------------------------------------

#if SORT_CLOUD_PARTS

void QSortParticles (tPartIdx *pPartIdx, int left, int right)
{
	int	l = left,
			r = right,
			m = pPartIdx [(l + r) / 2].z;

do {
	while (pPartIdx [l].z > m)
		l++;
	while (pPartIdx [r].z < m)
		r--;
	if (l <= r) {
		if (l < r) {
			tPartIdx h = pPartIdx [l];
			pPartIdx [l] = pPartIdx [r];
			pPartIdx [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortParticles (pPartIdx, l, right);
if (left < r)
	QSortParticles (pPartIdx, left, r);
}

//------------------------------------------------------------------------------

int TransformParticles (tParticle *pParticles, tPartIdx *pPartIdx, int nParts, int nFirstPart, int nPartLimit)
{
	tParticle	*pp;
	tPartIdx		*pi;

for (pp = pParticles + nFirstPart, pi = pPartIdx; nParts; nParts--, pParticles++, nFirstPart++, pp++) {
	if (nFirstPart == nPartLimit) {
		pp = pParticles;
		nFirstPart = 0;
		}
#if 1
	G3TransformPoint (&pp->transPos, &pp->pos, 0);
	if ((pi->z = pp->transPos[Z]) > 0) {
		pi->i = nFirstPart;
		pi++;
		}
#else
	pi->z = vmsVector::Dist(pp->pos, viewInfo.pos);
	pi->i = nFirstPart;
	pi++;
#endif
	}
return (int) (pi - pPartIdx);
}

//------------------------------------------------------------------------------

int SortParticleEmitterParticles (tParticle *pParticles, tPartIdx *pPartIdx, int nParts, int nFirstPart, int nPartLimit)
{
if (nParts > 1) {
#if 0
	int	h, i, z, nSortedUp = 1, nSortedDown = 1;
#endif
	nParts = TransformParticles (pParticles, pPartIdx, nParts, nFirstPart, nPartLimit);
#if 0
	for (h = nParts, i = nFirstPart; h; h--, i = (i + 1) % nPartLimit) {
		pPartIdx [i].i = i;
		pPartIdx [i].z = pParticles [i].transPos[Z];
		if (i) {
			if (z > pPartIdx [i].z)
				nSortedUp++;
			else if (z < pPartIdx [i].z)
				nSortedDown++;
			else {
				nSortedUp++;
				nSortedDown++;
				}
			}
		z = pPartIdx [i].z;
		}
	if (nSortedDown >= 9 * nParts / 10)
		return 0;
	if (nSortedUp >= 9 * nParts / 10)
		return 1;
#endif
	if (nParts > 1)
		QSortParticles (pPartIdx, 0, nParts - 1);
	}
return nParts;
}

#endif

//------------------------------------------------------------------------------

int RenderParticleEmitter (tParticleEmitter *emitterP, int nThread)
{
#if MT_PARTICLES
if (((gameOpts->render.bDepthSort > 0) && (nThread < 0)) && RunEmitterThread (emitterP, 0, rtRenderParticles)) {
	return 0;
	}
else
#endif
	{
		float			brightness = EmitterBrightness (emitterP);
		int			nParts = emitterP->nParts, h, i, j,
						nFirstPart = emitterP->nFirstPart,
						nPartLimit = emitterP->nPartLimit;
#if SORT_CLOUD_PARTS
		int			bSorted = gameOpts->render.particles.bSort && (nParts > 1);
		tPartIdx		*pPartIdx;
#endif
		vmsVector	v;

	if (gameOpts->render.bDepthSort > 0) {
		for (h = 0, i = nParts, j = nFirstPart; i; i--, j = (j + 1) % nPartLimit)
			if (TIAddParticle (emitterP->pParticles + j, brightness, nThread))
				h++;
		return h;
		}
	else {
		if (!BeginRenderParticleSystems (emitterP->nType, emitterP->nPartScale))
			return 0;
#if SORT_CLOUD_PARTS
		if (bSorted) {
			pPartIdx = emitterP->pPartIdx;
			nParts = SortParticleEmitterParticles (emitterP->pParticles, pPartIdx, nParts, nFirstPart, nPartLimit);
			for (i = 0; i < nParts; i++)
				RenderParticle (emitterP->pParticles + pPartIdx [i].i, brightness);
			}
		else
#endif //SORT_CLOUD_PARTS
		v = emitterP->prevPos - viewInfo.pos;
		if (emitterP->bHavePrevPos &&
			(vmsVector::Dist(emitterP->pos, viewInfo.pos) >= v.Mag()) &&
			(vmsVector::Dot(v, gameData.objs.viewerP->info.position.mOrient[FVEC]) >= 0)) {	//emitter moving away and facing towards emitter
			for (i = nParts, j = (nFirstPart + nParts) % nPartLimit; i; i--) {
				if (!j)
					j = nPartLimit;
#if OGL_VERTEX_ARRAYS && !EXTRA_VERTEX_ARRAYS
				if (!RenderParticle (emitterP->pParticles + --j))
					if (gameStates.render.bVertexArrays && !gameOpts->render.particles.bSort) {
						FlushParticleBuffer (emitterP->pParticles + iBuffer, nBuffer - iBuffer);
						nBuffer = iBuffer;
						}
#else
				RenderParticle (emitterP->pParticles + --j, brightness);
#endif
				}
			}
		else {
			for (i = nParts, j = nFirstPart; i; i--, j = (j + 1) % nPartLimit) {
#if OGL_VERTEX_ARRAYS && !EXTRA_VERTEX_ARRAYS
				if (!RenderParticle (emitterP->pParticles + j, brightness))
					if (gameStates.render.bVertexArrays && !gameOpts->render.particles.bSort) {
						FlushParticleBuffer (emitterP->pParticles + iBuffer, nBuffer - iBuffer);
						nBuffer = iBuffer;
						}
#else
				RenderParticle (emitterP->pParticles + j, brightness);
#endif
				}
			}
		return EndRenderParticleSystems (emitterP);
		}
	}
}

//------------------------------------------------------------------------------

void SetParticleEmitterPos (tParticleEmitter *emitterP, vmsVector *pos, vmsMatrix *orient, short nSegment)
{
if (emitterP) {
	if ((nSegment < 0) && gameOpts->render.particles.bCollisions)
		nSegment = FindSegByPos (*pos, emitterP->nSegment, 1, 0, 1);
	emitterP->pos = *pos;
	if (orient)
		emitterP->orient = *orient;
	if (nSegment >= 0)
		emitterP->nSegment = nSegment;
	}
}

//------------------------------------------------------------------------------

void SetParticleEmitterDir (tParticleEmitter *emitterP, vmsVector *pDir)
{
if (emitterP && (emitterP->bHaveDir = (pDir != NULL)))
	emitterP->dir = *pDir;
}

//------------------------------------------------------------------------------

void SetParticleEmitterLife (tParticleEmitter *emitterP, int nLife)
{
if (emitterP) {
	emitterP->nLife = nLife;
	emitterP->fPartsPerTick = nLife ? (float) emitterP->nMaxParts / (float) abs (nLife) : 0.0f;
	emitterP->nTicks = 0;
	}
}

//------------------------------------------------------------------------------

void SetParticleEmitterBrightness (tParticleEmitter *emitterP, int nBrightness)
{
if (emitterP)
	emitterP->nDefBrightness = nBrightness;
}

//------------------------------------------------------------------------------

void SetParticleEmitterSpeed (tParticleEmitter *emitterP, int nSpeed)
{
if (emitterP)
	emitterP->nSpeed = nSpeed;
}

//------------------------------------------------------------------------------

void SetParticleEmitterType (tParticleEmitter *emitterP, int nType)
{
if (emitterP)
	emitterP->nType = nType;
}

//------------------------------------------------------------------------------

int SetParticleEmitterDensity (tParticleEmitter *emitterP, int nMaxParts, int nDensity)
{
	tParticle	*p;
	int			h;

if (!emitterP)
	return 0;
if (emitterP->nMaxParts == nMaxParts)
	return 1;
if (nMaxParts > emitterP->nPartLimit) {
	if (!(p = (tParticle *) D2_ALLOC (nMaxParts * sizeof (tParticle))))
		return 0;
	if (emitterP->pParticles) {
		if (emitterP->nParts > nMaxParts)
			emitterP->nParts = nMaxParts;
		h = emitterP->nPartLimit - emitterP->nFirstPart;
		if (h > emitterP->nParts)
			h = emitterP->nParts;
		memcpy (p, emitterP->pParticles + emitterP->nFirstPart, h * sizeof (tParticle));
		if (h < emitterP->nParts)
			memcpy (p + h, emitterP->pParticles, (emitterP->nParts - h) * sizeof (tParticle));
		emitterP->nFirstPart = 0;
		emitterP->nPartLimit = nMaxParts;
		D2_FREE (emitterP->pParticles);
		}
	emitterP->pParticles = p;
#if SORT_CLOUD_PARTS
	if (gameOpts->render.particles.bSort) {
		D2_FREE (emitterP->pPartIdx);
		if (!(emitterP->pPartIdx = (tPartIdx *) D2_ALLOC (nMaxParts * sizeof (tPartIdx))))
			gameOpts->render.particles.bSort = 0;
		}
#endif
	}
emitterP->nDensity = nDensity;
emitterP->nMaxParts = nMaxParts;
#if 0
if (emitterP->nParts > nMaxParts)
	emitterP->nParts = nMaxParts;
#endif
emitterP->fPartsPerTick = (float) emitterP->nMaxParts / (float) abs (emitterP->nLife);
return 1;
}

//------------------------------------------------------------------------------

void SetParticleEmitterPartScale (tParticleEmitter *emitterP, float nPartScale)
{
if (emitterP)
	emitterP->nPartScale = nPartScale;
}

//------------------------------------------------------------------------------

int IsUsedParticleSystem (int iParticleSystem)
{
	int nPrev = -1;

for (int i = gameData.particles.iUsed; i >= 0; ) {
	if (iParticleSystem == i)
		return nPrev + 1;
	nPrev = i;
	i = gameData.particles.buffer [i].nNext;
	if (i == gameData.particles.iUsed) {
		RebuildParticleSystemList ();
		return -1;
		}
	}
return -1;
}

//------------------------------------------------------------------------------

int RemoveEmitter (int iParticleSystem, int iEmitter)
{
	tParticleSystem	*systemP;

if (0 > IsUsedParticleSystem (iParticleSystem))
	return -1;
systemP = gameData.particles.buffer + iParticleSystem;
if ((systemP->emitterP) && (iEmitter < systemP->nEmitters)) {
	DestroyEmitter (systemP->emitterP + iEmitter);
	if (iEmitter < --(systemP->nEmitters))
		systemP->emitterP [iEmitter] = systemP->emitterP [systemP->nEmitters];
	}
return systemP->nEmitters;
}

//------------------------------------------------------------------------------

tParticleSystem *PrevParticleSystem (int iParticleSystem)
{
	int	i, j;

for (i = gameData.particles.iUsed; i >= 0; i = j)
	if ((j = gameData.particles.buffer [i].nNext) == iParticleSystem)
		return gameData.particles.buffer + i;
return NULL;
}

//------------------------------------------------------------------------------

int DestroyParticleSystem (int iParticleSystem)
{
	int		i, nPrev;
	tParticleSystem	*systemP;

if (iParticleSystem < 0)
	iParticleSystem = -iParticleSystem - 1;
systemP = gameData.particles.buffer + iParticleSystem;
if (gameData.particles.objects && (systemP->nObject >= 0))
	SetParticleSystemObject (systemP->nObject, -1);
if (systemP->emitterP) {
	for (i = systemP->nEmitters; i; )
		DestroyEmitter (systemP->emitterP + --i);
	D2_FREE (systemP->emitterP);
	}
if (0 > (nPrev = IsUsedParticleSystem (iParticleSystem)))
	return -1;
i = systemP->nNext;
if (gameData.particles.iUsed == iParticleSystem)
	gameData.particles.iUsed = i;
systemP->nNext = gameData.particles.iFree;
systemP->nObject = -1;
systemP->nObjType = -1;
systemP->nObjId = -1;
systemP->nSignature = -1;
if ((systemP = PrevParticleSystem (iParticleSystem)))
	gameData.particles.buffer [nPrev - 1].nNext = i;
gameData.particles.iFree = iParticleSystem;
return iParticleSystem;
}

//------------------------------------------------------------------------------

int DestroyAllParticleSystems (void)
{
SEM_ENTER (SEM_SMOKE)

	int i, j, bDone = 0;

while (!bDone) {
	bDone = 1;
	for (i = gameData.particles.iUsed; i >= 0; i = j) {
		if ((j = gameData.particles.buffer [i].nNext) == gameData.particles.iUsed) {
			RebuildParticleSystemList ();
			bDone = 0;
			break;
			}
		DestroyParticleSystem (-i - 1);
		}
	}
FreeParticleImages ();
FreePartList ();
InitParticleSystems ();
SEM_LEAVE (SEM_SMOKE)
return 1;
}

//------------------------------------------------------------------------------

int CreateParticleSystem (vmsVector *pPos, vmsVector *pDir, vmsMatrix *pOrient,
					  short nSegment, int nMaxEmitters, int nMaxParts,
					  float nPartScale, int nDensity, int nPartsPerPos, int nLife, int nSpeed, char nType,
					  int nObject, tRgbaColorf *colorP, int bBlowUpParts, char nSide)
{
#if 0
if (!(EGI_FLAG (bUseParticleSystem, 0, 1, 0)))
	return 0;
else
#endif
if (gameData.particles.iFree < 0)
	return -1;
else if (!LoadParticleImage (nType)) {
	//PrintLog ("cannot create gameData.particles.buffer\n");
	return -1;
	}
else {
		int			i, t = gameStates.app.nSDLTicks;
		tParticleSystem		*systemP;
		vmsVector	vEmittingFace [4];

	if (nSide >= 0)
		GetSideVerts (vEmittingFace, nSegment, nSide);
	nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.particles.nDens [0]);
	if (gameStates.render.bPointSprites)
		nMaxParts *= 2;
	srand (SDL_GetTicks ());
	systemP = gameData.particles.buffer + gameData.particles.iFree;
	if (!(systemP->emitterP = (tParticleEmitter *) D2_ALLOC (nMaxEmitters * sizeof (tParticleEmitter)))) {
		//PrintLog ("cannot create gameData.particles.buffer\n");
		return 0;
		}
	if ((systemP->nObject = nObject) < 0x70000000) {
 		systemP->nSignature = OBJECTS [nObject].info.nSignature;
		systemP->nObjType = OBJECTS [nObject].info.nType;
		systemP->nObjId = OBJECTS [nObject].info.nId;
		}
	systemP->nEmitters = 0;
	systemP->nBirth = t;
	systemP->nMaxEmitters = nMaxEmitters;
	for (i = 0; i < nMaxEmitters; i++)
		if (CreateEmitter (systemP->emitterP + i, pPos, pDir, pOrient, nSegment, nObject, nMaxParts, nPartScale, nDensity,
							  nPartsPerPos, nLife, nSpeed, nType, colorP, t, bBlowUpParts, (nSide < 0) ? NULL : vEmittingFace))
			systemP->nEmitters++;
		else {
			DestroyParticleSystem (gameData.particles.iFree);
			//PrintLog ("cannot create gameData.particles.buffer\n");
			return -1;
			}
	systemP->nType = nType;
	i = gameData.particles.iFree;
	gameData.particles.iFree = systemP->nNext;
	systemP->nNext = gameData.particles.iUsed;
	gameData.particles.iUsed = i;
	//PrintLog ("CreateParticleSystem (%d) = %d,%d (%d)\n", nObject, i, nMaxEmitters, nType);
	return gameData.particles.iUsed;
	}
}

//------------------------------------------------------------------------------

int UpdateParticleSystems (void)
{
#if SMOKE_SLOWMO
	static int	t0 = 0;
	int t = gameStates.app.nSDLTicks - t0;

if (t / gameStates.gameplay.slowmo [0].fSpeed < 25)
	return 0;
t0 += (int) (gameStates.gameplay.slowmo [0].fSpeed * 25);
#else
if (!gameStates.app.tick40fps.bTick)
	return 0;
#endif
#if 0
if (!EGI_FLAG (bUseParticleSystem, 0, 1, 0))
	return 0;
else
#endif
{
		int		i, n, j = 0, t = gameStates.app.nSDLTicks;
		tParticleSystem	*systemP;
		tParticleEmitter	*emitterP;

	for (i = gameData.particles.iUsed; i >= 0; i = n) {
		systemP = gameData.particles.buffer + i;
		if ((n = systemP->nNext) == gameData.particles.iUsed) {
			RebuildParticleSystemList ();
			break;
			}
#if 0
		if ((systemP->nObject < 0x70000000) && (systemP->nSignature != OBJECTS [systemP->nObject].nSignature)) {
			SetParticleSystemLife (i, 0);
			//continue;
			}
#endif
		if ((systemP->nObject == 0x7fffffff) && (systemP->nType < 3) &&
			 (gameStates.app.nSDLTicks - systemP->nBirth > (MAX_SHRAPNEL_LIFE / F1_0) * 1000))
			SetParticleSystemLife (i, 0);
#if DBG
		if ((systemP->nObject < 0x70000000) && (OBJECTS [systemP->nObject].info.nType == 255))
			i = i;
#endif
		if ((emitterP = systemP->emitterP))
			for (j = 0; j < systemP->nEmitters; ) {
				if (!systemP->emitterP)
					return 0;
				if (EmitterIsDead (emitterP, t)) {
					if (!RemoveEmitter (i, j)) {
						//PrintLog ("killing gameData.particles.buffer %d (%d)\n", i, systemP->nObject);
						DestroyParticleSystem (i);
						break;
						}
					}
				else {
					//PrintLog ("moving %d (%d)\n", i, systemP->nObject);
					if ((systemP->nObject < 0) || ((systemP->nObject < 0x70000000) && (OBJECTS [systemP->nObject].info.nType == 255)))
						SetParticleEmitterLife (emitterP, 0);
					UpdateParticleEmitter (emitterP, t, -1);
					emitterP++, j++;
					}
				}
			}
	return j;
	}
}

//------------------------------------------------------------------------------

typedef struct tParticleEmitterList {
	tParticleEmitter		*emitterP;
	fix						xDist;
} tParticleEmitterList;

tParticleEmitterList *emitterListP = NULL;

//------------------------------------------------------------------------------

void QSortParticleEmitters (int left, int right)
{
	int	l = left,
			r = right;
	fix	m = emitterListP [(l + r) / 2].xDist;

do {
	while (emitterListP [l].xDist > m)
		l++;
	while (emitterListP [r].xDist < m)
		r--;
	if (l <= r) {
		if (l < r) {
			tParticleEmitterList h = emitterListP [l];
			emitterListP [l] = emitterListP [r];
			emitterListP [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortParticleEmitters (l, right);
if (left < r)
	QSortParticleEmitters (left, r);
}

//------------------------------------------------------------------------------

int ParticleEmitterCount (void)
{
	int					i, j;
	tParticleSystem	*systemP = gameData.particles.buffer;

for (i = gameData.particles.iUsed, j = 0; i >= 0; i = systemP->nNext) {
	systemP = gameData.particles.buffer + i;
	if (systemP->emitterP) {
		j += systemP->nEmitters;
		if ((systemP->nObject < 0x70000000) && (gameData.particles.objects [systemP->nObject] < 0))
			SetParticleSystemLife (i, 0);
		}
	}
return j;
}

//------------------------------------------------------------------------------

inline int ParticleEmitterMayBeVisible (tParticleEmitter *emitterP)
{
return (emitterP->nSegment < 0) || SegmentMayBeVisible (emitterP->nSegment, 5, -1);
}

//------------------------------------------------------------------------------

int CreateParticleEmitterList (void)
{
	int					h, i, j, nEmitters;
	tParticleSystem	*systemP = gameData.particles.buffer;
	tParticleEmitter	*emitterP;
	float					brightness;

h = ParticleEmitterCount ();
if (!h)
	return 0;
if (!(emitterListP = (tParticleEmitterList *) D2_ALLOC (h * sizeof (tParticleEmitterList))))
	return -1;
for (i = gameData.particles.iUsed, nEmitters = 0; i >= 0; i = systemP->nNext) {
	systemP = gameData.particles.buffer + i;
	if (!LoadParticleImage (systemP->nType)) {
		D2_FREE (emitterListP);
		return 0;
		}
	if (systemP->emitterP) {
		for (j = systemP->nEmitters, emitterP = systemP->emitterP; j; j--, emitterP++) {
			if ((emitterP->nParts > 0) && ParticleEmitterMayBeVisible (emitterP)) {
				brightness = EmitterBrightness (emitterP);
				emitterP->fBrightness = emitterP->nDefBrightness ? (float) emitterP->nDefBrightness / 100.0f : brightness;
				emitterListP [nEmitters].emitterP = emitterP;
#if SORT_CLOUDS
#	if 1	// use the closest point on a line from first to last particle to the viewer
				emitterListP [nEmitters++].xDist = VmLinePointDist(emitterP->pParticles[emitterP->nFirstPart].pos,
																				emitterP->pParticles [(emitterP->nFirstPart + emitterP->nParts - 1) % emitterP->nPartLimit].pos,
																				viewInfo.pos);
#	else	// use distance of the current emitter position to the viewer
				emitterListP [nEmitters++].xDist = vmsVector::Dist(emitterP->pos, viewInfo.pos);
#	endif
#endif
				}
			}
		}
	}
#if SORT_CLOUDS
if (nEmitters > 1)
	QSortParticleEmitters (0, nEmitters - 1);
#endif
return nEmitters;
}

//------------------------------------------------------------------------------

int ParticleCount (void)
{
	int					i, j, nParts, nFirstPart, nPartLimit, z;
	int					bUnscaled = gameStates.render.bPerPixelLighting == 2;
	tParticleSystem	*systemP = gameData.particles.buffer;
	tParticleEmitter	*emitterP;
	tParticle			*particleP;

#if 1
gameData.particles.depthBuf.zMin = gameData.render.zMin;
gameData.particles.depthBuf.zMax = gameData.render.zMax;
#else
gameData.particles.depthBuf.zMin = 0x7fffffff;
gameData.particles.depthBuf.zMax = -0x7fffffff;
#endif
gameData.particles.depthBuf.nParts = 0;
for (i = gameData.particles.iUsed; i >= 0; i = systemP->nNext) {
	systemP = gameData.particles.buffer + i;
	if (systemP->emitterP && (j = systemP->nEmitters)) {
		for (emitterP = systemP->emitterP; j; j--, emitterP++) {
			if ((nParts = emitterP->nParts)) {
				nFirstPart = emitterP->nFirstPart;
				nPartLimit = emitterP->nPartLimit;
				for (particleP = emitterP->pParticles + nFirstPart; nParts; nParts--, nFirstPart++, particleP++) {
					if (nFirstPart == nPartLimit) {
						nFirstPart = 0;
						particleP = emitterP->pParticles;
						}
					G3TransformPoint(particleP->transPos, particleP->pos, bUnscaled);
					z = particleP->transPos[Z];
#if 0
					if ((z < gameData.render.zMin) || (z > gameData.render.zMax))
						continue;
#else
					if (z < 0)
						continue;
					gameData.particles.depthBuf.nParts++;
					if (gameData.particles.depthBuf.zMin > z)
						gameData.particles.depthBuf.zMin = z;
					if (gameData.particles.depthBuf.zMax < z)
						gameData.particles.depthBuf.zMax = z;
#endif
					}
				}
			}
		if ((systemP->nObject < 0x70000000) && (gameData.particles.objects [systemP->nObject] < 0))
			SetParticleSystemLife (i, 0);
		}
	}
return gameData.particles.depthBuf.nParts;
}

//------------------------------------------------------------------------------

void DepthSortParticles (void)
{
	int					i, j, z, nParts, nFirstPart, nPartLimit, bSort;
	tParticleSystem	*systemP = gameData.particles.buffer;
	tParticleEmitter	*emitterP;
	tParticle			*particleP;
	tPartList			*ph, *pi, *pj, **pd;
	float					fBrightness;
	float					zScale;

bSort = (gameOpts->render.particles.bSort > 1);
zScale = (float) (PART_DEPTHBUFFER_SIZE - 1) / (float) (gameData.particles.depthBuf.zMax - gameData.particles.depthBuf.zMin);
if (zScale > 1)
	zScale = 1;
//ResetDepthBuf ();
for (i = gameData.particles.iUsed; i >= 0; i = systemP->nNext) {
	systemP = gameData.particles.buffer + i;
	if (systemP->emitterP && (j = systemP->nEmitters)) {
		for (emitterP = systemP->emitterP; j; j--, emitterP++) {
			if (!ParticleEmitterMayBeVisible (emitterP))
				continue;
			if ((nParts = emitterP->nParts)) {
				fBrightness = (float) EmitterBrightness (emitterP);
				nFirstPart = emitterP->nFirstPart;
				nPartLimit = emitterP->nPartLimit;
				for (particleP = emitterP->pParticles + nFirstPart; nParts; nParts--, nFirstPart++, particleP++) {
					if (nFirstPart == nPartLimit) {
						nFirstPart = 0;
						particleP = emitterP->pParticles;
						}
					z = particleP->transPos[Z];
#if 1
					if ((z < F1_0) || (z > gameData.render.zMax))
						continue;
#else
					if (z < 0)
						continue;
#endif
					pd = gameData.particles.depthBuf.pDepthBuffer + (int) ((float) (z - gameData.particles.depthBuf.zMin) * zScale);
					// find the first particle to insert the new one *before* and place in pj; pi will be it's predecessor (NULL if to insert at list start)
					ph = gameData.particles.depthBuf.pPartList + --gameData.particles.depthBuf.nFreeParts;
					ph->particleP = particleP;
					ph->fBrightness = fBrightness;
					if (bSort) {
						for (pi = NULL, pj = *pd; pj && (pj->particleP->transPos[Z] > z); pj = pj->nextPartP)
							pi = pj;
						if (pi) {
							ph->nextPartP = pi->nextPartP;
							pi->nextPartP = ph;
							}
						else {
							ph->nextPartP = *pd;
							*pd = ph;
							}
						}
					else {
						ph->nextPartP = *pd;
						*pd = ph;
						}
					if (!gameData.particles.depthBuf.nFreeParts)
						return;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

int RenderParticles (void)
{
	int	h;
	struct tPartList	**pd, *pl, *pn;

if (!AllocPartList ())
	return 0;
pl = gameData.particles.depthBuf.pPartList + 999999;
pl->particleP = NULL;
if (!(h = ParticleCount ()))
	return 0;
pl = gameData.particles.depthBuf.pPartList + 999999;
pl->particleP = NULL;
DepthSortParticles ();
if (!LoadParticleImages ())
	return 0;
BeginRenderParticleSystems (-1, 1);
for (pd = gameData.particles.depthBuf.pDepthBuffer + PART_DEPTHBUFFER_SIZE - 1;
	  pd >= gameData.particles.depthBuf.pDepthBuffer;
	  pd--) {
		if ((pl = *pd)) {
		do {
			RenderParticle (pl->particleP, pl->fBrightness);
			pn = pl->nextPartP;
			pl->nextPartP = NULL;
			pl = pn;
			} while (pl);
		*pd = NULL;
		}
	}
gameData.particles.depthBuf.nFreeParts = PARTLIST_SIZE;
EndRenderParticleSystems (NULL);
return 1;
}

//------------------------------------------------------------------------------

int RenderParticleEmitters (void)
{
	int		h, i, j;

#if !EXTRA_VERTEX_ARRAYS
nBuffer = 0;
#endif
h = (gameOpts->render.bDepthSort > 0) ? -1 : CreateParticleEmitterList ();
if (!h)
	return 1;
if (h > 0) {
	do {
		RenderParticleEmitter (emitterListP [--h].emitterP, -1);
		} while (h);
	D2_FREE (emitterListP);
	emitterListP = NULL;
	}
else
	{
	tParticleSystem *systemP = gameData.particles.buffer;
	tParticleEmitter *emitterP;
	for (h = 0, i = gameData.particles.iUsed; i >= 0; i = systemP->nNext) {
		systemP = gameData.particles.buffer + i;
		if (systemP->emitterP) {
			if (!LoadParticleImage (systemP->nType))
				return 0;
			if ((systemP->nObject >= 0) && (systemP->nObject < 0x70000000) && (gameData.particles.objects [systemP->nObject] < 0))
				SetParticleSystemLife (i, 0);
			for (j = systemP->nEmitters, emitterP = systemP->emitterP; j; j--, emitterP++) {
				emitterP->fBrightness = EmitterBrightness (emitterP);
				h += RenderParticleEmitter (emitterP, -1);
				}
			}
		}
	}
#if DBG
if (!h)
	return 0;
#endif
return 1;
}

//------------------------------------------------------------------------------

int RenderParticleSystems (void)
{
int h = (gameOpts->render.particles.bSort && (gameOpts->render.bDepthSort <= 0)) ? RenderParticles () : RenderParticleEmitters ();
return h;
}

//------------------------------------------------------------------------------

void SetParticleSystemPos (int i, vmsVector *pos, vmsMatrix *orient, short nSegment)
{
if (0 <= IsUsedParticleSystem (i)) {
	tParticleSystem *systemP = gameData.particles.buffer + i;
	if (systemP->emitterP)
		for (i = 0; i < systemP->nEmitters; i++)
			SetParticleEmitterPos (systemP->emitterP, pos, orient, nSegment);
#if DBG
	else if (systemP->nObject >= 0) {
		HUDMessage (0, "no particleSystem in SetParticleSystemPos (%d,%d)\n", i, systemP->nObject);
		//PrintLog ("no gameData.particles.buffer in SetParticleSystemPos (%d,%d)\n", i, systemP->nObject);
		systemP->nObject = -1;
		}
#endif
	}
}

//------------------------------------------------------------------------------

void SetParticleSystemDensity (int i, int nMaxParts, int nDensity)
{
if (0 <= IsUsedParticleSystem (i)) {
	nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.particles.nDens [0]);
	tParticleSystem *systemP = gameData.particles.buffer + i;
	if (systemP->emitterP)
		for (i = 0; i < systemP->nEmitters; i++)
			SetParticleEmitterDensity (systemP->emitterP + i, nMaxParts, nDensity);
	}
}

//------------------------------------------------------------------------------

void SetParticleSystemPartScale (int i, float nPartScale)
{
if (0 <= IsUsedParticleSystem (i)) {
	tParticleSystem *systemP = gameData.particles.buffer + i;
	if (systemP->emitterP)
		for (i = 0; i < systemP->nEmitters; i++)
			SetParticleEmitterPartScale (systemP->emitterP + i, nPartScale);
	}
}

//------------------------------------------------------------------------------

void SetParticleSystemLife (int i, int nLife)
{
if (0 <= IsUsedParticleSystem (i)) {
	tParticleSystem *systemP = gameData.particles.buffer + i;
	if (systemP->emitterP && (systemP->emitterP->nLife != nLife)) {
		//PrintLog ("SetParticleSystemLife (%d,%d) = %d\n", i, systemP->nObject, nLife);
		int j;
		for (j = 0; j < systemP->nEmitters; j++)
			SetParticleEmitterLife (systemP->emitterP, nLife);
		}
	}
}

//------------------------------------------------------------------------------

void SetParticleSystemBrightness (int i, int nBrightness)
{
if (0 <= IsUsedParticleSystem (i)) {
	tParticleSystem *systemP = gameData.particles.buffer + i;
	if (systemP->emitterP && (systemP->emitterP->nDefBrightness != nBrightness)) {
		//PrintLog ("SetParticleSystemLife (%d,%d) = %d\n", i, systemP->nObject, nLife);
		int j;
		for (j = 0; j < systemP->nEmitters; j++)
			SetParticleEmitterBrightness (systemP->emitterP, nBrightness);
		}
	}
}

//------------------------------------------------------------------------------

void SetParticleSystemType (int i, int nType)
{
if (0 <= IsUsedParticleSystem (i)) {
	tParticleSystem *systemP = gameData.particles.buffer + i;
	systemP->nType = nType;
	for (i = 0; i < systemP->nEmitters; i++)
		SetParticleEmitterType (systemP->emitterP + i, nType);
	}
}

//------------------------------------------------------------------------------

void SetParticleSystemSpeed (int i, int nSpeed)
{
if (0 <= IsUsedParticleSystem (i)) {
	tParticleSystem *systemP = gameData.particles.buffer + i;
	systemP->nSpeed = nSpeed;
	for (i = 0; i < systemP->nEmitters; i++)
		SetParticleEmitterSpeed (systemP->emitterP + i, nSpeed);
	}
}

//------------------------------------------------------------------------------

void SetParticleSystemDir (int i, vmsVector *pDir)
{
if (0 <= IsUsedParticleSystem (i)) {
	tParticleSystem *systemP = gameData.particles.buffer + i;
	for (i = 0; i < systemP->nEmitters; i++)
		SetParticleEmitterDir (systemP->emitterP + i, pDir);
	}
}

//------------------------------------------------------------------------------

int SetParticleSystemObject (int nObject, int nParticleSystem)
{
if ((nObject < 0) || (nObject >= MAX_OBJECTS))
	return -1;
return gameData.particles.objects [nObject] = nParticleSystem;
}

//------------------------------------------------------------------------------

int GetParticleSystemType (int i)
{
return (IsUsedParticleSystem (i)) ? gameData.particles.buffer [i].nType : -1;
}

//------------------------------------------------------------------------------

tParticleEmitter *GetParticleEmitter (int i, int j)
{
if (0 <= IsUsedParticleSystem (i)) {
	tParticleSystem *systemP = gameData.particles.buffer + i;
	return (systemP->emitterP && (j < systemP->nEmitters)) ? systemP->emitterP + j : NULL;
	}
else
	return NULL;
}

//------------------------------------------------------------------------------

int MaxParticles (int nParts, int nDens)
{
nParts = ((nParts < 0) ? -nParts : nParts * (nDens + 1)); //(int) (nParts * pow (1.2, nDens));
return (nParts < 100000) ? nParts : 100000;
}

//------------------------------------------------------------------------------

float ParticleSize (int nSize, float nScale)
{
if (gameOpts->render.particles.bDisperse)
	return (float) (PARTICLE_RAD * (nSize + 1)) / nScale + 0.5f;
else
	return (float) (PARTICLE_RAD * (nSize + 1) * (nSize + 2) / 2) / nScale + 0.5f;
}

//------------------------------------------------------------------------------
//eof
