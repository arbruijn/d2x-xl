
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

#define PARTICLE_TYPES	5

#define PARTICLE_FPS	30

#define PARTICLE_RAD	(F1_0)

#define PART_DEPTHBUFFER_SIZE 100000
#define PARTLIST_SIZE 1000000

static int bHavePartImg [2][PARTICLE_TYPES] = {{0, 0, 0, 0},{0, 0, 0, 0}};

static grsBitmap *bmpParticle [2][PARTICLE_TYPES] = {{NULL, NULL, NULL, NULL, NULL},{NULL, NULL, NULL, NULL, NULL}};
#if 0
static grsBitmap *bmpBumpMaps [2] = {NULL, NULL};
#endif

static const char *szParticleImg [2][PARTICLE_TYPES] = {
	{"smoke.tga", "corona.tga", "bullcase.tga"},
	{"smoke.tga", "corona.tga", "bullcase.tga"}
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

#define SMOKE_START_ALPHA		(gameOpts->render.smoke.bDisperse ? 96 : 128)

//	-----------------------------------------------------------------------------

void InitSmoke (void)
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
	gameData.smoke.buffer [i].nNext = j;
	gameData.smoke.buffer [i].nObject = -1;
	gameData.smoke.buffer [i].nObjType = -1;
	gameData.smoke.buffer [i].nObjId = -1;
	gameData.smoke.buffer [i].nSignature = -1;
	}
gameData.smoke.buffer [i].nNext = -1;
gameData.smoke.iFree = 0;
gameData.smoke.iUsed = -1;
}

//------------------------------------------------------------------------------

static void RebuildSmokeList (void)
{
gameData.smoke.iUsed =
gameData.smoke.iFree = -1;
tSmoke *pSmoke = gameData.smoke.buffer;
for (int i = 0; i < MAX_SMOKE; i++, pSmoke++) {
	if (pSmoke->pClouds) {
		pSmoke->nNext = gameData.smoke.iUsed;
		gameData.smoke.iUsed = i;
		}
	if (pSmoke->pClouds) {
		pSmoke->nNext = gameData.smoke.iFree;
		gameData.smoke.iFree = i;
		}
	}
}

//------------------------------------------------------------------------------

void ResetDepthBuf (void)
{
memset (gameData.smoke.depthBuf.pDepthBuffer, 0, PART_DEPTHBUFFER_SIZE * sizeof (struct tPartList **));
memset (gameData.smoke.depthBuf.pPartList, 0, (PARTLIST_SIZE - gameData.smoke.depthBuf.nFreeParts) * sizeof (struct tPartList));
gameData.smoke.depthBuf.nFreeParts = PARTLIST_SIZE;
}

//	-----------------------------------------------------------------------------

int AllocPartList (void)
{
if (gameData.smoke.depthBuf.pDepthBuffer)
	return 1;
if (!(gameData.smoke.depthBuf.pDepthBuffer = (struct tPartList **) D2_ALLOC (PART_DEPTHBUFFER_SIZE * sizeof (struct tPartList *))))
	return 0;
if (!(gameData.smoke.depthBuf.pPartList = (struct tPartList *) D2_ALLOC (PARTLIST_SIZE * sizeof (struct tPartList)))) {
	D2_FREE (gameData.smoke.depthBuf.pDepthBuffer);
	return 0;
	}
gameData.smoke.depthBuf.nFreeParts = 0;
ResetDepthBuf ();
return 1;
}

//	-----------------------------------------------------------------------------

void FreePartList (void)
{
D2_FREE (gameData.smoke.depthBuf.pPartList);
D2_FREE (gameData.smoke.depthBuf.pDepthBuffer);
}

//	-----------------------------------------------------------------------------

static inline int ParticleImageType (int nType)
{
return (nType == 5) ? 2 : ((nType == 3) || (nType == 4)) ? 1 : 0;
}

//	-----------------------------------------------------------------------------

void AnimateParticle (int nType)
{
	int	bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.smoke.bSort,
			nFrames = nParticleFrames [bPointSprites][nType];

if (nFrames > 1) {
	static time_t t0 [PARTICLE_TYPES] = {0, 0, 0, 0};

	time_t		t = gameStates.app.nSDLTicks;
	int			iFrame = iParticleFrames [bPointSprites][nType];
#if 0
	int			iFrameIncr = iPartFrameIncr [bPointSprites][nType];
#endif
	int			bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.smoke.bSort;
	grsBitmap	*bmP = bmpParticle [gameStates.render.bPointSprites && !gameOpts->render.smoke.bSort][ParticleImageType (nType)];

	if (!BM_FRAMES (bmP))
		return;
	BM_CURFRAME (bmP) = BM_FRAMES (bmP) + iFrame;
#if 1
	if (t - t0 [nType] > 150)
#endif
		{

		t0 [nType] = t;
#if 1
		iFrame = (iFrame + 1) % nFrames;
#else
		iFrame += iFrameIncr;
		if ((iFrame < 0) || (iFrame >= nFrames)) {
			iPartFrameIncr [bPointSprites][nType] = -iFrameIncr;
			iFrame += -2 * iFrameIncr;
			}
#endif
		iParticleFrames [bPointSprites][nType] = iFrame;
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
					bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.smoke.bSort,
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
if (nType == PARTICLE_TYPES - 1)
	nParticleFrames [bPointSprites][nType] = BM_FRAMECOUNT (bmP);
else {
	h = bmP->bmProps.w / 64;
	nParticleFrames [bPointSprites][nType] = h;
	}
return *flagP > 0;
}

//	-----------------------------------------------------------------------------

int LoadParticleImages (void)
{
	int	i;

for (i = 0; i < 3; i++) {
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
	for (j = 0; j < 3; j++)
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

int CreateParticle (tParticle *pParticle, vmsVector *pPos, vmsVector *pDir, vmsMatrix *pOrient,
						  short nSegment, int nLife, int nSpeed, char nSmokeType, char nClass,
						  float nScale, tRgbaColorf *colorP, int nCurTime, int bBlowUp,
						  float fBrightness, vmsVector *vEmittingFace)
{
	vmsVector	vDrift, vPos;
	int			nRad, nFrames, nType = ParticleImageType (nSmokeType);

if (vEmittingFace)
	pPos = RandomPointOnQuad (vEmittingFace, &vPos);
if (nScale < 0)
	nRad = (int) -nScale;
else if (gameOpts->render.smoke.bSyncSizes)
	nRad = (int) PARTICLE_SIZE (gameOpts->render.smoke.nSize [0], nScale);
else
	nRad = (int) nScale;
if (!nRad)
	nRad = F1_0;
pParticle->nType = nType;
pParticle->bEmissive = (nSmokeType == 3);
pParticle->nClass = nClass;
pParticle->nSegment = nSegment;
pParticle->nBounce = 0;
if (nType == 2) {
	pParticle->bBright = 0;
	pParticle->color.red =
	pParticle->color.green =
	pParticle->color.blue =
	pParticle->color.alpha = 1;
	pParticle->nFade = -1;
	}
else {
	pParticle->bBright = nSmokeType ? 0 : (rand () % 50) == 0;
	if ((pParticle->bColored = (colorP != NULL))) {
		if (nType == 1)
			pParticle->color = *colorP;
		else {
			pParticle->color.red = colorP->red * RANDOM_FADE;
			pParticle->color.green = colorP->green * RANDOM_FADE;
			pParticle->color.blue = colorP->blue * RANDOM_FADE;
			}
		pParticle->nFade = 0;
		}
	else {
		pParticle->color.red = 1.0f;
		pParticle->color.green = 0.5;
		pParticle->color.blue = 0.0f;//(float) (64 + randN (64)) / 255.0f;
		pParticle->nFade = 2;
		}
	if (pParticle->bEmissive)
		pParticle->color.alpha = (float) (SMOKE_START_ALPHA + 64) / 255.0f;
	else if (nType != 1) {
		if (colorP && (colorP->alpha < 0))
			pParticle->color.alpha = -colorP->alpha;
		else
			pParticle->color.alpha = (float) (SMOKE_START_ALPHA + randN (64)) / 255.0f;
		}
#if 1
	if (gameOpts->render.smoke.bDisperse && !pParticle->bBright) {
		fBrightness = 1.0f - fBrightness;
		pParticle->color.alpha += fBrightness * fBrightness / 8.0f;
		}
	}
#endif
//nSpeed = (int) (sqrt (nSpeed) * (float) F1_0);
nSpeed *= F1_0;
if (pDir) {
	vmsAngVec	a;
	vmsMatrix	m;
	float		d;
	a[PA] = randN (F1_0 / 4) - F1_0 / 8;
	a[BA] = randN (F1_0 / 4) - F1_0 / 8;
	a[HA] = randN (F1_0 / 4) - F1_0 / 8;
	m = vmsMatrix::Create(a);
	vDrift = m * (*pDir);
	vmsVector::Normalize (vDrift);
	d = (float) vmsVector::DeltaAngle (vDrift, *pDir, NULL);
	if (d) {
		d = (float) exp ((F1_0 / 8) / d);
		nSpeed = (fix) ((float) nSpeed / d);
		}
	if (!pParticle->bColored)
		pParticle->color.green =
		pParticle->color.blue = 1.0f;//(float) (64 + randN (64)) / 255.0f;
	vDrift *= nSpeed;
	pParticle->dir = *pDir;
	pParticle->bHaveDir = 1;
	}
else {
	vmsVector	vOffs;
	vDrift[X] = nSpeed - randN (2 * nSpeed);
	vDrift[Y] = nSpeed - randN (2 * nSpeed);
	vDrift[Z] = nSpeed - randN (2 * nSpeed);
	vOffs = vDrift;
	pParticle->dir.SetZero();
	pParticle->bHaveDir = 1;
	}
if (pOrient) {
		vmsAngVec	vRot;
		vmsMatrix	mRot;

	vRot[BA] = 0;
	vRot[PA] = 2048 - ((d_rand () % 9) * 512);
	vRot[HA] = 2048 - ((d_rand () % 9) * 512);
	mRot = vmsMatrix::Create(vRot);
	//TODO: MM
	pParticle->orient = *pOrient * mRot;
	//pParticle->orient = *pOrient;
	}
if (vEmittingFace)
	pParticle->pos = *pPos;
else
	pParticle->pos = *pPos + vDrift * (F1_0 / 64);
pParticle->drift = vDrift;
if (nLife < 0)
	nLife = -nLife;
if (nType < 3) {
	if (gameOpts->render.smoke.bDisperse)
		nLife = (nLife * 2) / 3;
	nLife = nLife / 2 + randN (nLife / 2);
	}
pParticle->nLife =
pParticle->nTTL = nLife;
pParticle->nMoved = nCurTime;
pParticle->nDelay = 0; //bStart ? randN (nLife) : 0;
nRad += nType ? nRad : randN (nRad);
if ((pParticle->bBlowUp = bBlowUp)) {
	pParticle->nRad = nRad;
	pParticle->nWidth =
	pParticle->nHeight = nRad / 2;
	}
else {
	pParticle->nWidth =
	pParticle->nHeight = nRad;
	pParticle->nRad = nRad / 2;
	}
nFrames = nParticleFrames [gameStates.render.bPointSprites && !gameOpts->render.smoke.bSort && (gameOpts->render.bDepthSort <= 0)][nType];
if (pParticle->nType == 2) {
	pParticle->nFrame = 0;
	pParticle->nRotFrame = 0;
	pParticle->nOrient = 3;
	}
else {
	pParticle->nFrame = rand () % (nFrames * nFrames);
	pParticle->nRotFrame = pParticle->nFrame / 2;
	pParticle->nOrient = rand () % 4;
	}
#if 1
if (pParticle->bEmissive)
	pParticle->color.alpha *= ParticleBrightness (colorP);
else if (nType != 1)
	pParticle->color.alpha /= nSmokeType + 2;
#endif
return 1;
}

//------------------------------------------------------------------------------

int DestroyParticle (tParticle *pParticle)
{
#if DBG
memset (pParticle, 0, sizeof (tParticle));
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

int CollideParticleAndWall (tParticle *pParticle)
{
	tSegment		*segP;
	tSide			*sideP;
	int			bInit, nSide, nVert, nChild, nFace, nInFront;
	fix			nDist;
	int			*vlP;
	vmsVector	pos = pParticle->pos;

//redo:

segP = gameData.segs.segments + pParticle->nSegment;
if ((bInit = (pParticle->nSegment != nPartSeg)))
	nPartSeg = pParticle->nSegment;
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
				nDist = gameData.segs.vertices[vlP[4]].DistToPlane(sideP->normals[0], gameData.segs.vertices[nVert]);
			else
				nDist = gameData.segs.vertices[vlP[1]].DistToPlane(sideP->normals[1], gameData.segs.vertices[nVert]);
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
		pParticle->nSegment = nChild;
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

int UpdateParticle (tParticle *pParticle, int nCurTime)
{
	int			j, nRad;
	fix			t, dot;
	vmsVector	pos, drift;
	fix			drag = F2X ((float) pParticle->nLife / (float) pParticle->nTTL);


if ((pParticle->nLife <= 0) /*|| (pParticle->color.alpha < 0.01f)*/)
	return 0;
t = nCurTime - pParticle->nMoved;
if (pParticle->nDelay > 0)
	pParticle->nDelay -= t;
else {
	pos = pParticle->pos;
	drift = pParticle->drift;
	if (!pParticle->nType) {
		drift[X] = ChangeDir (drift[X]);
		drift[Y] = ChangeDir (drift[Y]);
		drift[Z] = ChangeDir (drift[Z]);
		}
	for (j = 0; j < 2; j++) {
		if (t < 0)
			t = -t;
		pParticle->pos = pos + drift * t;
		if (pParticle->bHaveDir) {
			vmsVector vi = drift, vj = pParticle->dir;
			vmsVector::Normalize(vi);
			vmsVector::Normalize(vj);
//				if (vmsVector::Dot(drift, pParticle->dir) < 0)
			if (vmsVector::Dot(vi, vj) < 0)
				drag = -drag;
//				VmVecScaleInc (&drift, &pParticle->dir, drag);
			pParticle->pos += pParticle->dir * drag;
			}
		if (gameOpts->render.smoke.bCollisions && CollideParticleAndWall (pParticle)) {	//Reflect the particle
			if (j)
				return 0;
			else if (!(dot = vmsVector::Dot(drift, *wallNorm)))
				return 0;
			else {
				drift = pParticle->drift + *wallNorm * (-2 * dot);
				//VmVecScaleAdd (&pParticle->pos, &pos, &drift, 2 * t);
				pParticle->nBounce = 3;
				continue;
				}
			}
		else if (pParticle->nBounce)
			pParticle->nBounce--;
		else {
			break;
			}
		}
	pParticle->drift = drift;
	if (pParticle->nTTL >= 0) {
#if SMOKE_SLOWMO
		pParticle->nLife -= (int) (t / gameStates.gameplay.slowmo [0].fSpeed);
#else
		pParticle->nLife -= t;
#endif
		if ((pParticle->nType < 2) && (nRad = pParticle->nRad)) {
			if (pParticle->bBlowUp) {
				if (pParticle->nWidth >= nRad)
					pParticle->nRad = 0;
				else {
					pParticle->nWidth += nRad / 10 / pParticle->bBlowUp;
					pParticle->nHeight += nRad / 10 / pParticle->bBlowUp;
					if (pParticle->nWidth > nRad)
						pParticle->nWidth = nRad;
					if (pParticle->nHeight > nRad)
						pParticle->nHeight = nRad;
					pParticle->color.alpha *= (1.0f + 0.0725f / pParticle->bBlowUp);
					if (pParticle->color.alpha > 1)
						pParticle->color.alpha = 1;
					}
				}
			else {
				if (pParticle->nWidth <= nRad)
					pParticle->nRad = 0;
				else {
					pParticle->nRad += nRad / 5;
					pParticle->color.alpha *= 1.0725f;
					if (pParticle->color.alpha > 1)
						pParticle->color.alpha = 1;
					}
				}
			}
		}
	}
pParticle->nMoved = nCurTime;
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
			grsBitmap *bmP = bmpParticle [0][gameData.smoke.nLastType];
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
			if (gameData.render.lights.dynamic.headlights.nLights && !(gameStates.render.automap.bDisplay || gameData.smoke.nLastType))
				G3SetupHeadlightShader (1, 0, &color);
			else if ((gameOpts->render.effects.bSoftParticles & 4) && !gameData.smoke.nLastType)
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
	if ((gameStates.ogl.bShadersOk && !gameData.smoke.nLastType) && (gameStates.render.history.nShader != 999)) {
		glUseProgramObject (0);
		gameStates.render.history.nShader = -1;
		}
	}
}

//------------------------------------------------------------------------------

#define PARTICLE_POSITIONS 64

int RenderParticle (tParticle *pParticle, float brightness)
{
	vmsVector			hp;
	GLfloat				d, u, v;
	grsBitmap			*bmP;
	tRgbaColorf			pc;
	tTexCoord2f			texCoord [4];
	tParticleVertex	*pb;
	fVector				vOffset, vCenter;
	float					decay = (float) pParticle->nLife / (float) pParticle->nTTL;
	int					i, nFrame, nType = pParticle->nType, bEmissive = pParticle->bEmissive,
							bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.smoke.bSort && (gameOpts->render.bDepthSort <= 0);

	static int			nFrames = 1;
	static float		deltaUV = 1.0f;
	static tSinCosf	sinCosPart [PARTICLE_POSITIONS];
	static int			bInitSinCos = 1;
	static fMatrix		mRot;

if (pParticle->nDelay > 0)
	return 0;
if (!(bmP = bmpParticle [0][nType]))
	return 0;
if (BM_CURFRAME (bmP))
	bmP = BM_CURFRAME (bmP);
if (gameOpts->render.bDepthSort > 0) {
	hp = pParticle->transPos;
	if ((gameData.smoke.nLastType != nType) || (brightness != bufferBrightness) || (bBufferEmissive != bEmissive)) {
		if (gameStates.render.bVertexArrays)
			FlushParticleBuffer (brightness);
		gameData.smoke.nLastType = nType;
		bBufferEmissive = bEmissive;
		glActiveTexture (GL_TEXTURE0);
		glClientActiveTexture (GL_TEXTURE0);
		if (OglBindBmTex (bmP, 0, 1))
			return 0;
		nFrames = nParticleFrames [0][nType];
		deltaUV = 1.0f / (float) nFrames;
		if (pParticle->bEmissive)
			glBlendFunc (GL_ONE, GL_ONE);
		else
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	if (!gameStates.render.bVertexArrays)
		glBegin (GL_QUADS);
	}
else if (gameOpts->render.smoke.bSort) {
	hp = pParticle->transPos;
	if ((gameData.smoke.nLastType != nType) || (brightness != bufferBrightness)) {
		if (gameStates.render.bVertexArrays)
			FlushParticleBuffer (brightness);
		else
			glEnd ();
		gameData.smoke.nLastType = nType;
		if (OglBindBmTex (bmP, 0, 1))
			return 0;
		nFrames = nParticleFrames [bPointSprites][nType];
		deltaUV = 1.0f / (float) nFrames;
		if (pParticle->bEmissive)
			glBlendFunc (GL_ONE, GL_ONE);
		else
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (!gameStates.render.bVertexArrays)
			glBegin (GL_QUADS);
		}
	}
else
	G3TransformPoint(hp, pParticle->pos, 0);
if (pParticle->bBright)
	brightness = (float) sqrt (brightness);
if (nType) {
	//pParticle->color.green *= 0.99;
	//pParticle->color.blue *= 0.99;
	}
else if (!pParticle->bColored && (pParticle->nFade > 0)) {
	if (pParticle->color.green < 1.0f) {
#if SMOKE_SLOWMO
		pParticle->color.green += 1.0f / 20.0f / (float) gameStates.gameplay.slowmo [0].fSpeed;
#else
		pParticle->color.green += 1.0f / 20.0f;
#endif
		if (pParticle->color.green > 1.0f) {
			pParticle->color.green = 1.0f;
			pParticle->nFade--;
			}
		}
	if (pParticle->color.blue < 1.0f) {
#if SMOKE_SLOWMO
		pParticle->color.blue += 1.0f / 10.0f / (float) gameStates.gameplay.slowmo [0].fSpeed;
#else
		pParticle->color.blue += 1.0f / 10.0f;
#endif
		if (pParticle->color.blue > 1.0f) {
			pParticle->color.blue = 1.0f;
			pParticle->nFade--;
			}
		}
	}
else if (pParticle->nFade == 0) {
	pParticle->color.red = pParticle->color.red * RANDOM_FADE;
	pParticle->color.green = pParticle->color.green * RANDOM_FADE;
	pParticle->color.blue = pParticle->color.blue * RANDOM_FADE;
	pParticle->nFade = -1;
	}
pc = pParticle->color;
//pc.alpha *= /*gameOpts->render.smoke.bDisperse ? decay2 :*/ decay;
if (nType) {
	u = v = 0.0f;
	d = 1.0f;
	}
else {
	u = (float) (pParticle->nFrame % nFrames) * deltaUV;
	v = (float) (pParticle->nFrame / nFrames) * deltaUV;
	d = deltaUV;
	}
if (!nType) {
#if 0
	if (SHOW_DYN_LIGHT) {
		tFaceColor *psc = AvgSgmColor (pParticle->nSegment, NULL);
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
vCenter[X] = X2F (hp[X]);
vCenter[Y] = X2F (hp[Y]);
vCenter[Z] = X2F (hp[Z]);
i = pParticle->nOrient;
if (bEmissive) { //scale light trail particle color to reduce saturation
	pc.red /= 50.0f;
	pc.green /= 50.0f;
	pc.blue /= 50.0f;
	}
else {
#if 0
	pc.alpha = (pc.alpha - 0.005f) * decay + 0.005f;
#	if 1
	pc.alpha = (float) pow (pc.alpha * pc.alpha, 1.0 / 3.0);
#	endif
#else
	float fFade = (float) cos ((double) sqr (1 - decay) * Pi) / 2 + 0.5f;
	pc.alpha *= fFade;
#endif
	pc.alpha *= alphaScale [gameOpts->render.smoke.nAlpha [gameOpts->render.smoke.bSyncSizes ? 0 : pParticle->nClass]];
	}
if (pc.alpha < 1.0 / 255.0) {
	pParticle->nLife = 0;
	return 0;
	}
#if 0
if (!gameData.render.lights.dynamic.headlights.nLights && (pc.red + pc.green + pc.blue < 0.001)) {
	pParticle->nLife = 0;
	return 0;
	}
#endif
if (gameOpts->render.smoke.bDisperse && !nType) {
#if 0
	decay = (float) sqrt (decay);
#else
	decay = (float) pow (decay * decay * decay, 1.0f / 5.0f);
#endif
	vOffset[X] = X2F (pParticle->nWidth) / decay;
	vOffset[Y] = X2F (pParticle->nHeight) / decay;
	}
else {
	vOffset[X] = X2F (pParticle->nWidth) * decay;
	vOffset[Y] = X2F (pParticle->nHeight) * decay;
	}
if (gameStates.render.bVertexArrays) {
	vOffset[Z] = 0;
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
	if (!nType && gameOpts->render.smoke.bRotate) {
		if (bInitSinCos) {
			OglComputeSinCos (sizeofa (sinCosPart), sinCosPart);
			bInitSinCos = 0;
			mRot[RVEC][Z] =
			mRot[UVEC][Z] =
			mRot[FVEC][X] =
			mRot[FVEC][Y] = 0;
			mRot[FVEC][Z] = 1;
			}
		nFrame = (pParticle->nOrient & 1) ? 63 - pParticle->nRotFrame : pParticle->nRotFrame;
		mRot[RVEC][X] =
		mRot[UVEC][Y] = sinCosPart [nFrame].fCos;
		mRot[UVEC][X] = sinCosPart [nFrame].fSin;
		mRot[RVEC][Y] = -mRot[UVEC][X];
		vOffset = mRot * vOffset;
		pb [0].vertex[X] = vCenter[X] - vOffset[X];
		pb [0].vertex[Y] = vCenter[Y] + vOffset[Y];
		pb [1].vertex[X] = vCenter[X] + vOffset[Y];
		pb [1].vertex[Y] = vCenter[Y] + vOffset[X];
		pb [2].vertex[X] = vCenter[X] + vOffset[X];
		pb [2].vertex[Y] = vCenter[Y] - vOffset[Y];
		pb [3].vertex[X] = vCenter[X] - vOffset[Y];
		pb [3].vertex[Y] = vCenter[Y] - vOffset[X];
		pb [0].vertex[Z] =
		pb [1].vertex[Z] =
		pb [2].vertex[Z] =
		pb [3].vertex[Z] = vCenter[Z];
		}
	else {
		pb [0].vertex[X] =
		pb [3].vertex[X] = vCenter[X] - vOffset[X];
		pb [1].vertex[X] =
		pb [2].vertex[X] = vCenter[X] + vOffset[X];
		pb [0].vertex[Y] =
		pb [1].vertex[Y] = vCenter[Y] + vOffset[Y];
		pb [2].vertex[Y] =
		pb [3].vertex[Y] = vCenter[Y] - vOffset[Y];
		pb [0].vertex[Z] =
		pb [1].vertex[Z] =
		pb [2].vertex[Z] =
		pb [3].vertex[Z] = vCenter[Z];
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
if (gameData.smoke.bAnimate) {
	pParticle->nFrame = (pParticle->nFrame + 1) % (nFrames * nFrames);
	if (!(pParticle->nFrame & 1))
		pParticle->nRotFrame = (pParticle->nRotFrame + 1) % 64;
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

int BeginRenderSmoke (int nType, float nScale)
{
	grsBitmap	*bmP;
	int			bLightmaps = HaveLightmaps ();
	static time_t	t0 = 0;

if (gameOpts->render.bDepthSort <= 0) {
	nType = (nType % PARTICLE_TYPES == PARTICLE_TYPES - 1);
	if ((nType >= 0) && !gameOpts->render.smoke.bSort)
		AnimateParticle (nType);
	bmP = bmpParticle [0][nType];
	gameData.smoke.bStencil = StencilOff ();
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
gameData.smoke.nLastType = -1;
if (gameStates.app.nSDLTicks - t0 < 33)
	gameData.smoke.bAnimate = 0;
else {
	t0 = gameStates.app.nSDLTicks;
	gameData.smoke.bAnimate = 1;
	}
return 1;
}

//------------------------------------------------------------------------------

int EndRenderSmoke (tCloud *pCloud)
{
if (gameOpts->render.bDepthSort <= 0) {
	if (!CloseParticleBuffer ())
		glEnd ();
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	glDepthMask (1);
	StencilOn (gameData.smoke.bStencil);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
return 1;
}

//------------------------------------------------------------------------------

char SmokeObjClass (int nObject)
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

float CloudBrightness (tCloud *pCloud)
{
	tObject	*objP;

if (pCloud->nObject >= 0x70000000)
	return 0.5f;
if (pCloud->nType > 2)
	return 1.0f;
if (pCloud->nObject < 0)
	return pCloud->fBrightness;
if (pCloud->nObjType == OBJ_EFFECT)
	return (float) pCloud->nDefBrightness / 100.0f;
if (pCloud->nObjType == OBJ_DEBRIS)
	return 0.5f;
if ((pCloud->nObjType == OBJ_WEAPON) && (pCloud->nObjId == PROXMINE_ID))
	return 0.2f;
objP = OBJECTS + pCloud->nObject;
if ((objP->info.nType != pCloud->nObjType) || (objP->info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED | OF_ARMAGEDDON)))
	return pCloud->fBrightness;
return pCloud->fBrightness = (float) ObjectDamage (objP) * 0.5f + 0.1f;
}

//------------------------------------------------------------------------------

#if MT_PARTICLES

int RunCloudThread (tCloud *pCloud, int nCurTime, tRenderTask nTask)
{
int	i;

if (!gameStates.app.bMultiThreaded)
	return 0;
while (tiRender.ti [0].bExec && tiRender.ti [1].bExec)
	G3_SLEEP (0);
i = tiRender.ti [0].bExec ? 1 : 0;
tiRender.clouds [i] = pCloud;
tiRender.nCurTime [i] = nCurTime;
tiRender.nTask = nTask;
tiRender.ti [i].bExec = 1;
return 1;
}

#endif

//------------------------------------------------------------------------------

int CreateCloud (tCloud *pCloud, vmsVector *pPos, vmsVector *pDir, vmsMatrix *pOrient,
					  short nSegment, int nObject, int nMaxParts, float nPartScale,
					  int nDensity, int nPartsPerPos, int nLife, int nSpeed, char nType,
					  tRgbaColorf *colorP, int nCurTime, int bBlowUpParts, vmsVector *vEmittingFace)
{
if (!(pCloud->pParticles = (tParticle *) D2_ALLOC (nMaxParts * sizeof (tParticle))))
	return 0;
#if SORT_CLOUD_PARTS
if (gameOpts->render.smoke.bSort) {
	if (!(pCloud->pPartIdx = (tPartIdx *) D2_ALLOC (nMaxParts * sizeof (tPartIdx))))
		gameOpts->render.smoke.bSort = 0;
	}
else
#endif
	pCloud->pPartIdx = NULL;
pCloud->nLife = nLife;
pCloud->nBirth = nCurTime;
pCloud->nSpeed = nSpeed;
pCloud->nType = nType;
if ((pCloud->bHaveColor = (colorP != NULL)))
	pCloud->color = *colorP;
if ((pCloud->bHaveDir = (pDir != NULL)))
	pCloud->dir = *pDir;
pCloud->prevPos =
pCloud->pos = *pPos;
if (pOrient)
	pCloud->orient = *pOrient;
pCloud->bHavePrevPos = 1;
pCloud->bBlowUpParts = bBlowUpParts;
pCloud->nParts = 0;
pCloud->nMoved = nCurTime;
pCloud->nPartLimit =
pCloud->nMaxParts = nMaxParts;
pCloud->nFirstPart = 0;
pCloud->nPartScale = nPartScale;
pCloud->nDensity = nDensity;
pCloud->nPartsPerPos = nPartsPerPos;
pCloud->nSegment = nSegment;
pCloud->nObject = nObject;
if ((nObject >= 0) && (nObject < 0x70000000)) {
	pCloud->nObjType = OBJECTS [nObject].info.nType;
	pCloud->nObjId = OBJECTS [nObject].info.nId;
	}
pCloud->nClass = SmokeObjClass (nObject);
pCloud->fPartsPerTick = (float) nMaxParts / (float) abs (nLife);
pCloud->nTicks = 0;
pCloud->nDefBrightness = 0;
if ((pCloud->bEmittingFace = (vEmittingFace != NULL)))
	memcpy (pCloud->vEmittingFace, vEmittingFace, sizeof (pCloud->vEmittingFace));
pCloud->fBrightness = (nObject < 0) ? 0.5f : CloudBrightness (pCloud);
return 1;
}

//------------------------------------------------------------------------------

int DestroyCloud (tCloud *pCloud)
{
if (pCloud->pParticles) {
	D2_FREE (pCloud->pParticles);
	pCloud->pParticles = NULL;
	D2_FREE (pCloud->pPartIdx);
	pCloud->pPartIdx = NULL;
	}
pCloud->nParts =
pCloud->nMaxParts = 0;
return 1;
}

//------------------------------------------------------------------------------

inline int CloudLives (tCloud *pCloud, int nCurTime)
{
return (pCloud->nLife < 0) || (pCloud->nBirth + pCloud->nLife > nCurTime);
}

//------------------------------------------------------------------------------

inline int CloudIsDead (tCloud *pCloud, int nCurTime)
{
return !(CloudLives (pCloud, nCurTime) || pCloud->nParts);
}

//------------------------------------------------------------------------------

#if 0

void CheckCloud (tCloud *pCloud)
{
	int	i, j;

for (i = pCloud->nParts, j = pCloud->nFirstPart; i; i--, j = (j + 1) % pCloud->nPartLimit)
	if (pCloud->pParticles [j].nType < 0)
		j = j;
}

#endif

//------------------------------------------------------------------------------

int UpdateCloud (tCloud *pCloud, int nCurTime, int nThread)
{
#if MT_PARTICLES
if ((nThread < 0) && RunCloudThread (pCloud, nCurTime, rtUpdateParticles)) {
	return 0;
	}
else
#endif
	{
		tCloud		c = *pCloud;
		int			t, h, i, j;
		float			fDist;
		float			fBrightness = CloudBrightness (pCloud);
		vmsMatrix	mOrient = pCloud->orient;
		vmsVector	vDelta, vPos, *pDir = (pCloud->bHaveDir ? &pCloud->dir : NULL),
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
		c.nTicks = 0;
		if (CloudLives (pCloud, nCurTime)) {
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
				vDeltaf[X] /= (float) h;
				vDeltaf[Y] /= (float) h;
				vDeltaf[Z] /= (float) h;
				}
			else if ((c.nType == 3) || (c.nType == 4))
				goto funcExit;
			else {
#if 1
				vPosf = c.prevPos.ToFloat();
				vDeltaf = vDelta.ToFloat();
				vDeltaf[X] /= (float) h;
				vDeltaf[Y] /= (float) h;
				vDeltaf[Z] /= (float) h;
#else
				VmVecFixToFloat (&vPosf, &c.pos);
				vDeltaf[X] =
				vDeltaf[Y] =
				vDeltaf[Z] = 0.0f;
				h = 1;
#endif
				}
			c.nParts += h;
			for (; h; h--, j = (j + 1) % c.nPartLimit) {
				vPosf += vDeltaf;
				vPos = vPosf.ToFix();
/*
				vPos[Y] = (fix) (vPosf[Y] * 65536.0f);
				vPos[Z] = (fix) (vPosf[Z] * 65536.0f);
*/
				CreateParticle (c.pParticles + j, &vPos, pDir, &mOrient, c.nSegment, c.nLife,
									 c.nSpeed, c.nType, c.nClass, c.nPartScale, c.bHaveColor ? &c.color : NULL,
									 nCurTime, c.bBlowUpParts, fBrightness, vEmittingFace);
				}
			}
		}

	funcExit:

	pCloud->bHavePrevPos = 1;
	pCloud->nMoved = nCurTime;
	pCloud->prevPos = pCloud->pos;
	pCloud->nTicks = c.nTicks;
	pCloud->nFirstPart = c.nFirstPart;
	return pCloud->nParts = c.nParts;
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

int SortCloudParticles (tParticle *pParticles, tPartIdx *pPartIdx, int nParts, int nFirstPart, int nPartLimit)
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

int RenderCloud (tCloud *pCloud, int nThread)
{
#if MT_PARTICLES
if (((gameOpts->render.bDepthSort > 0) && (nThread < 0)) && RunCloudThread (pCloud, 0, rtRenderParticles)) {
	return 0;
	}
else
#endif
	{
		float			brightness = CloudBrightness (pCloud);
		int			nParts = pCloud->nParts, h, i, j,
						nFirstPart = pCloud->nFirstPart,
						nPartLimit = pCloud->nPartLimit;
#if SORT_CLOUD_PARTS
		int			bSorted = gameOpts->render.smoke.bSort && (nParts > 1);
		tPartIdx		*pPartIdx;
#endif
		vmsVector	v;

	if (gameOpts->render.bDepthSort > 0) {
		for (h = 0, i = nParts, j = nFirstPart; i; i--, j = (j + 1) % nPartLimit)
			if (TIAddParticle (pCloud->pParticles + j, brightness, nThread))
				h++;
		return h;
		}
	else {
		if (!BeginRenderSmoke (pCloud->nType, pCloud->nPartScale))
			return 0;
#if SORT_CLOUD_PARTS
		if (bSorted) {
			pPartIdx = pCloud->pPartIdx;
			nParts = SortCloudParticles (pCloud->pParticles, pPartIdx, nParts, nFirstPart, nPartLimit);
			for (i = 0; i < nParts; i++)
				RenderParticle (pCloud->pParticles + pPartIdx [i].i, brightness);
			}
		else
#endif //SORT_CLOUD_PARTS
		v = pCloud->prevPos - viewInfo.pos;
		if (pCloud->bHavePrevPos &&
			(vmsVector::Dist(pCloud->pos, viewInfo.pos) >= v.Mag()) &&
			(vmsVector::Dot(v, gameData.objs.viewerP->info.position.mOrient[FVEC]) >= 0)) {	//emitter moving away and facing towards emitter
			for (i = nParts, j = (nFirstPart + nParts) % nPartLimit; i; i--) {
				if (!j)
					j = nPartLimit;
#if OGL_VERTEX_ARRAYS && !EXTRA_VERTEX_ARRAYS
				if (!RenderParticle (pCloud->pParticles + --j))
					if (gameStates.render.bVertexArrays && !gameOpts->render.smoke.bSort) {
						FlushParticleBuffer (pCloud->pParticles + iBuffer, nBuffer - iBuffer);
						nBuffer = iBuffer;
						}
#else
				RenderParticle (pCloud->pParticles + --j, brightness);
#endif
				}
			}
		else {
			for (i = nParts, j = nFirstPart; i; i--, j = (j + 1) % nPartLimit) {
#if OGL_VERTEX_ARRAYS && !EXTRA_VERTEX_ARRAYS
				if (!RenderParticle (pCloud->pParticles + j, brightness))
					if (gameStates.render.bVertexArrays && !gameOpts->render.smoke.bSort) {
						FlushParticleBuffer (pCloud->pParticles + iBuffer, nBuffer - iBuffer);
						nBuffer = iBuffer;
						}
#else
				RenderParticle (pCloud->pParticles + j, brightness);
#endif
				}
			}
		return EndRenderSmoke (pCloud);
		}
	}
}

//------------------------------------------------------------------------------

void SetCloudPos (tCloud *pCloud, vmsVector *pos, vmsMatrix *orient, short nSegment)
{
if (pCloud) {
	if ((nSegment < 0) && gameOpts->render.smoke.bCollisions)
		nSegment = FindSegByPos (*pos, pCloud->nSegment, 1, 0);
	pCloud->pos = *pos;
	if (orient)
		pCloud->orient = *orient;
	if (nSegment >= 0)
		pCloud->nSegment = nSegment;
	}
}

//------------------------------------------------------------------------------

void SetCloudDir (tCloud *pCloud, vmsVector *pDir)
{
if (pCloud && (pCloud->bHaveDir = (pDir != NULL)))
	pCloud->dir = *pDir;
}

//------------------------------------------------------------------------------

void SetCloudLife (tCloud *pCloud, int nLife)
{
if (pCloud) {
	pCloud->nLife = nLife;
	pCloud->fPartsPerTick = nLife ? (float) pCloud->nMaxParts / (float) abs (nLife) : 0.0f;
	pCloud->nTicks = 0;
	}
}

//------------------------------------------------------------------------------

void SetCloudBrightness (tCloud *pCloud, int nBrightness)
{
if (pCloud)
	pCloud->nDefBrightness = nBrightness;
}

//------------------------------------------------------------------------------

void SetCloudSpeed (tCloud *pCloud, int nSpeed)
{
if (pCloud)
	pCloud->nSpeed = nSpeed;
}

//------------------------------------------------------------------------------

void SetCloudType (tCloud *pCloud, int nType)
{
if (pCloud)
	pCloud->nType = nType;
}

//------------------------------------------------------------------------------

int SetCloudDensity (tCloud *pCloud, int nMaxParts, int nDensity)
{
	tParticle	*p;
	int			h;

if (!pCloud)
	return 0;
if (pCloud->nMaxParts == nMaxParts)
	return 1;
if (nMaxParts > pCloud->nPartLimit) {
	if (!(p = (tParticle *) D2_ALLOC (nMaxParts * sizeof (tParticle))))
		return 0;
	if (pCloud->pParticles) {
		if (pCloud->nParts > nMaxParts)
			pCloud->nParts = nMaxParts;
		h = pCloud->nPartLimit - pCloud->nFirstPart;
		if (h > pCloud->nParts)
			h = pCloud->nParts;
		memcpy (p, pCloud->pParticles + pCloud->nFirstPart, h * sizeof (tParticle));
		if (h < pCloud->nParts)
			memcpy (p + h, pCloud->pParticles, (pCloud->nParts - h) * sizeof (tParticle));
		pCloud->nFirstPart = 0;
		pCloud->nPartLimit = nMaxParts;
		D2_FREE (pCloud->pParticles);
		}
	pCloud->pParticles = p;
#if SORT_CLOUD_PARTS
	if (gameOpts->render.smoke.bSort) {
		D2_FREE (pCloud->pPartIdx);
		if (!(pCloud->pPartIdx = (tPartIdx *) D2_ALLOC (nMaxParts * sizeof (tPartIdx))))
			gameOpts->render.smoke.bSort = 0;
		}
#endif
	}
pCloud->nDensity = nDensity;
pCloud->nMaxParts = nMaxParts;
#if 0
if (pCloud->nParts > nMaxParts)
	pCloud->nParts = nMaxParts;
#endif
pCloud->fPartsPerTick = (float) pCloud->nMaxParts / (float) abs (pCloud->nLife);
return 1;
}

//------------------------------------------------------------------------------

void SetCloudPartScale (tCloud *pCloud, float nPartScale)
{
if (pCloud)
	pCloud->nPartScale = nPartScale;
}

//------------------------------------------------------------------------------

int IsUsedSmoke (int iSmoke)
{
	int nPrev = -1;

for (int i = gameData.smoke.iUsed; i >= 0; ) {
	if (iSmoke == i)
		return nPrev + 1;
	nPrev = i;
	i = gameData.smoke.buffer [i].nNext;
	if (i == gameData.smoke.iUsed) {
		RebuildSmokeList ();
		return -1;
		}
	}
return -1;
}

//------------------------------------------------------------------------------

int RemoveCloud (int iSmoke, int iCloud)
{
	tSmoke	*pSmoke;

if (0 > IsUsedSmoke (iSmoke))
	return -1;
pSmoke = gameData.smoke.buffer + iSmoke;
if ((pSmoke->pClouds) && (iCloud < pSmoke->nClouds)) {
	DestroyCloud (pSmoke->pClouds + iCloud);
	if (iCloud < --(pSmoke->nClouds))
		pSmoke->pClouds [iCloud] = pSmoke->pClouds [pSmoke->nClouds];
	}
return pSmoke->nClouds;
}

//------------------------------------------------------------------------------

tSmoke *PrevSmoke (int iSmoke)
{
	int	i, j;

for (i = gameData.smoke.iUsed; i >= 0; i = j)
	if ((j = gameData.smoke.buffer [i].nNext) == iSmoke)
		return gameData.smoke.buffer + i;
return NULL;
}

//------------------------------------------------------------------------------

int DestroySmoke (int iSmoke)
{
	int		i, nPrev;
	tSmoke	*pSmoke;

if (iSmoke < 0)
	iSmoke = -iSmoke - 1;
pSmoke = gameData.smoke.buffer + iSmoke;
if (gameData.smoke.objects && (pSmoke->nObject >= 0))
	SetSmokeObject (pSmoke->nObject, -1);
if (pSmoke->pClouds) {
	for (i = pSmoke->nClouds; i; )
		DestroyCloud (pSmoke->pClouds + --i);
	D2_FREE (pSmoke->pClouds);
	}
if (0 > (nPrev = IsUsedSmoke (iSmoke)))
	return -1;
i = pSmoke->nNext;
if (gameData.smoke.iUsed == iSmoke)
	gameData.smoke.iUsed = i;
pSmoke->nNext = gameData.smoke.iFree;
pSmoke->nObject = -1;
pSmoke->nObjType = -1;
pSmoke->nObjId = -1;
pSmoke->nSignature = -1;
if ((pSmoke = PrevSmoke (iSmoke)))
	gameData.smoke.buffer [nPrev - 1].nNext = i;
gameData.smoke.iFree = iSmoke;
return iSmoke;
}

//------------------------------------------------------------------------------

int DestroyAllSmoke (void)
{
SEM_ENTER (SEM_SMOKE)

	int i, j, bDone = 0;

while (!bDone) {
	bDone = 1;
	for (i = gameData.smoke.iUsed; i >= 0; i = j) {
		if ((j = gameData.smoke.buffer [i].nNext) == gameData.smoke.iUsed) {
			RebuildSmokeList ();
			bDone = 0;
			break;
			}
		DestroySmoke (-i - 1);
		}
	}
FreeParticleImages ();
FreePartList ();
InitSmoke ();
SEM_LEAVE (SEM_SMOKE)
return 1;
}

//------------------------------------------------------------------------------

int CreateSmoke (vmsVector *pPos, vmsVector *pDir, vmsMatrix *pOrient,
					  short nSegment, int nMaxClouds, int nMaxParts,
					  float nPartScale, int nDensity, int nPartsPerPos, int nLife, int nSpeed, char nType,
					  int nObject, tRgbaColorf *colorP, int bBlowUpParts, char nSide)
{
#if 0
if (!(EGI_FLAG (bUseSmoke, 0, 1, 0)))
	return 0;
else
#endif
if (gameData.smoke.iFree < 0)
	return -1;
else if (!LoadParticleImage (nType)) {
	//PrintLog ("cannot create gameData.smoke.buffer\n");
	return -1;
	}
else {
		int			i, t = gameStates.app.nSDLTicks;
		tSmoke		*pSmoke;
		vmsVector	vEmittingFace [4];

	if (nSide >= 0)
		GetSideVerts (vEmittingFace, nSegment, nSide);
	nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.smoke.nDens [0]);
	if (gameStates.render.bPointSprites)
		nMaxParts *= 2;
	srand (SDL_GetTicks ());
	pSmoke = gameData.smoke.buffer + gameData.smoke.iFree;
	if (!(pSmoke->pClouds = (tCloud *) D2_ALLOC (nMaxClouds * sizeof (tCloud)))) {
		//PrintLog ("cannot create gameData.smoke.buffer\n");
		return 0;
		}
	if ((pSmoke->nObject = nObject) < 0x70000000) {
 		pSmoke->nSignature = OBJECTS [nObject].info.nSignature;
		pSmoke->nObjType = OBJECTS [nObject].info.nType;
		pSmoke->nObjId = OBJECTS [nObject].info.nId;
		}
	pSmoke->nClouds = 0;
	pSmoke->nBirth = t;
	pSmoke->nMaxClouds = nMaxClouds;
	for (i = 0; i < nMaxClouds; i++)
		if (CreateCloud (pSmoke->pClouds + i, pPos, pDir, pOrient, nSegment, nObject, nMaxParts, nPartScale, nDensity,
							  nPartsPerPos, nLife, nSpeed, nType, colorP, t, bBlowUpParts, (nSide < 0) ? NULL : vEmittingFace))
			pSmoke->nClouds++;
		else {
			DestroySmoke (gameData.smoke.iFree);
			//PrintLog ("cannot create gameData.smoke.buffer\n");
			return -1;
			}
	pSmoke->nType = nType;
	i = gameData.smoke.iFree;
	gameData.smoke.iFree = pSmoke->nNext;
	pSmoke->nNext = gameData.smoke.iUsed;
	gameData.smoke.iUsed = i;
	//PrintLog ("CreateSmoke (%d) = %d,%d (%d)\n", nObject, i, nMaxClouds, nType);
	return gameData.smoke.iUsed;
	}
}

//------------------------------------------------------------------------------

int UpdateSmoke (void)
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
if (!EGI_FLAG (bUseSmoke, 0, 1, 0))
	return 0;
else
#endif
{
		int		i, n, j = 0, t = gameStates.app.nSDLTicks;
		tSmoke	*pSmoke;
		tCloud	*pCloud;

	for (i = gameData.smoke.iUsed; i >= 0; i = n) {
		pSmoke = gameData.smoke.buffer + i;
		if (i == 11)
			i = i;
		if ((n = pSmoke->nNext) == gameData.smoke.iUsed) {
			RebuildSmokeList ();
			break;
			}
#if 0
		if ((pSmoke->nObject < 0x70000000) && (pSmoke->nSignature != OBJECTS [pSmoke->nObject].nSignature)) {
			SetSmokeLife (i, 0);
			//continue;
			}
#endif
		if ((pSmoke->nObject == 0x7fffffff) && (pSmoke->nType < 3) &&
			 (gameStates.app.nSDLTicks - pSmoke->nBirth > (MAX_SHRAPNEL_LIFE / F1_0) * 1000))
			SetSmokeLife (i, 0);
#if DBG
		if ((pSmoke->nObject < 0x70000000) && (OBJECTS [pSmoke->nObject].info.nType == 255))
			i = i;
#endif
		if ((pCloud = pSmoke->pClouds))
			for (j = 0; j < pSmoke->nClouds; ) {
				if (!pSmoke->pClouds)
					return 0;
				if (CloudIsDead (pCloud, t)) {
					if (!RemoveCloud (i, j)) {
						//PrintLog ("killing gameData.smoke.buffer %d (%d)\n", i, pSmoke->nObject);
						DestroySmoke (i);
						break;
						}
					}
				else {
					//PrintLog ("moving %d (%d)\n", i, pSmoke->nObject);
					if ((pSmoke->nObject < 0) || ((pSmoke->nObject < 0x70000000) && (OBJECTS [pSmoke->nObject].info.nType == 255)))
						SetCloudLife (pCloud, 0);
					UpdateCloud (pCloud, t, -1);
					pCloud++, j++;
					}
				}
			}
	return j;
	}
}

//------------------------------------------------------------------------------

typedef struct tCloudList {
	tCloud		*pCloud;
	fix			xDist;
} tCloudList;

tCloudList *pCloudList = NULL;

//------------------------------------------------------------------------------

void QSortClouds (int left, int right)
{
	int	l = left,
			r = right;
	fix	m = pCloudList [(l + r) / 2].xDist;

do {
	while (pCloudList [l].xDist > m)
		l++;
	while (pCloudList [r].xDist < m)
		r--;
	if (l <= r) {
		if (l < r) {
			tCloudList h = pCloudList [l];
			pCloudList [l] = pCloudList [r];
			pCloudList [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortClouds (l, right);
if (left < r)
	QSortClouds (left, r);
}

//------------------------------------------------------------------------------

int CloudCount (void)
{
	int		i, j;
	tSmoke	*pSmoke = gameData.smoke.buffer;

for (i = gameData.smoke.iUsed, j = 0; i >= 0; i = pSmoke->nNext) {
	pSmoke = gameData.smoke.buffer + i;
	if (pSmoke->pClouds) {
		j += pSmoke->nClouds;
		if ((pSmoke->nObject < 0x70000000) && (gameData.smoke.objects [pSmoke->nObject] < 0))
			SetSmokeLife (i, 0);
		}
	}
return j;
}

//------------------------------------------------------------------------------

inline int CloudMayBeVisible (tCloud *pCloud)
{
return (pCloud->nSegment < 0) || SegmentMayBeVisible (pCloud->nSegment, 5, -1);
}

//------------------------------------------------------------------------------

int CreateCloudList (void)
{
	int			h, i, j, nClouds;
	tSmoke		*pSmoke = gameData.smoke.buffer;
	tCloud		*pCloud;
	float		brightness;

h = CloudCount ();
if (!h)
	return 0;
if (!(pCloudList = (tCloudList *) D2_ALLOC (h * sizeof (tCloudList))))
	return -1;
for (i = gameData.smoke.iUsed, nClouds = 0; i >= 0; i = pSmoke->nNext) {
	pSmoke = gameData.smoke.buffer + i;
	if (!LoadParticleImage (pSmoke->nType)) {
		D2_FREE (pCloudList);
		return 0;
		}
	if (pSmoke->pClouds) {
		for (j = pSmoke->nClouds, pCloud = pSmoke->pClouds; j; j--, pCloud++) {
			if ((pCloud->nParts > 0) && CloudMayBeVisible (pCloud)) {
				brightness = CloudBrightness (pCloud);
				pCloud->fBrightness = pCloud->nDefBrightness ? (float) pCloud->nDefBrightness / 100.0f : brightness;
				pCloudList [nClouds].pCloud = pCloud;
#if SORT_CLOUDS
#	if 1	// use the closest point on a line from first to last particle to the viewer
				pCloudList [nClouds++].xDist = VmLinePointDist(pCloud->pParticles[pCloud->nFirstPart].pos,
																				pCloud->pParticles [(pCloud->nFirstPart + pCloud->nParts - 1) % pCloud->nPartLimit].pos,
																				viewInfo.pos);
#	else	// use distance of the current emitter position to the viewer
				pCloudList [nClouds++].xDist = vmsVector::Dist(pCloud->pos, viewInfo.pos);
#	endif
#endif
				}
			}
		}
	}
#if SORT_CLOUDS
if (nClouds > 1)
	QSortClouds (0, nClouds - 1);
#endif
return nClouds;
}

//------------------------------------------------------------------------------

int ParticleCount (void)
{
	int			i, j, nParts, nFirstPart, nPartLimit, z;
	int			bUnscaled = gameStates.render.bPerPixelLighting == 2;
	tSmoke		*pSmoke = gameData.smoke.buffer;
	tCloud		*pCloud;
	tParticle	*pParticle;

#if 1
gameData.smoke.depthBuf.zMin = gameData.render.zMin;
gameData.smoke.depthBuf.zMax = gameData.render.zMax;
#else
gameData.smoke.depthBuf.zMin = 0x7fffffff;
gameData.smoke.depthBuf.zMax = -0x7fffffff;
#endif
gameData.smoke.depthBuf.nParts = 0;
for (i = gameData.smoke.iUsed; i >= 0; i = pSmoke->nNext) {
	pSmoke = gameData.smoke.buffer + i;
	if (pSmoke->pClouds && (j = pSmoke->nClouds)) {
		for (pCloud = pSmoke->pClouds; j; j--, pCloud++) {
			if ((nParts = pCloud->nParts)) {
				nFirstPart = pCloud->nFirstPart;
				nPartLimit = pCloud->nPartLimit;
				for (pParticle = pCloud->pParticles + nFirstPart; nParts; nParts--, nFirstPart++, pParticle++) {
					if (nFirstPart == nPartLimit) {
						nFirstPart = 0;
						pParticle = pCloud->pParticles;
						}
					G3TransformPoint(pParticle->transPos, pParticle->pos, bUnscaled);
					z = pParticle->transPos[Z];
#if 0
					if ((z < gameData.render.zMin) || (z > gameData.render.zMax))
						continue;
#else
					if (z < 0)
						continue;
					gameData.smoke.depthBuf.nParts++;
					if (gameData.smoke.depthBuf.zMin > z)
						gameData.smoke.depthBuf.zMin = z;
					if (gameData.smoke.depthBuf.zMax < z)
						gameData.smoke.depthBuf.zMax = z;
#endif
					}
				}
			}
		if ((pSmoke->nObject < 0x70000000) && (gameData.smoke.objects [pSmoke->nObject] < 0))
			SetSmokeLife (i, 0);
		}
	}
return gameData.smoke.depthBuf.nParts;
}

//------------------------------------------------------------------------------

void DepthSortParticles (void)
{
	int			i, j, z, nParts, nFirstPart, nPartLimit, bSort;
	tSmoke		*pSmoke = gameData.smoke.buffer;
	tCloud		*pCloud;
	tParticle	*pParticle;
	tPartList	*ph, *pi, *pj, **pd;
	float			fBrightness;
	float		zScale;

bSort = (gameOpts->render.smoke.bSort > 1);
zScale = (float) (PART_DEPTHBUFFER_SIZE - 1) / (float) (gameData.smoke.depthBuf.zMax - gameData.smoke.depthBuf.zMin);
if (zScale > 1)
	zScale = 1;
//ResetDepthBuf ();
for (i = gameData.smoke.iUsed; i >= 0; i = pSmoke->nNext) {
	pSmoke = gameData.smoke.buffer + i;
	if (pSmoke->pClouds && (j = pSmoke->nClouds)) {
		for (pCloud = pSmoke->pClouds; j; j--, pCloud++) {
			if (!CloudMayBeVisible (pCloud))
				continue;
			if ((nParts = pCloud->nParts)) {
				fBrightness = (float) CloudBrightness (pCloud);
				nFirstPart = pCloud->nFirstPart;
				nPartLimit = pCloud->nPartLimit;
				for (pParticle = pCloud->pParticles + nFirstPart; nParts; nParts--, nFirstPart++, pParticle++) {
					if (nFirstPart == nPartLimit) {
						nFirstPart = 0;
						pParticle = pCloud->pParticles;
						}
					z = pParticle->transPos[Z];
#if 1
					if ((z < F1_0) || (z > gameData.render.zMax))
						continue;
#else
					if (z < 0)
						continue;
#endif
					pd = gameData.smoke.depthBuf.pDepthBuffer + (int) ((float) (z - gameData.smoke.depthBuf.zMin) * zScale);
					// find the first particle to insert the new one *before* and place in pj; pi will be it's predecessor (NULL if to insert at list start)
					ph = gameData.smoke.depthBuf.pPartList + --gameData.smoke.depthBuf.nFreeParts;
					ph->pParticle = pParticle;
					ph->fBrightness = fBrightness;
					if (bSort) {
						for (pi = NULL, pj = *pd; pj && (pj->pParticle->transPos[Z] > z); pj = pj->pNextPart)
							pi = pj;
						if (pi) {
							ph->pNextPart = pi->pNextPart;
							pi->pNextPart = ph;
							}
						else {
							ph->pNextPart = *pd;
							*pd = ph;
							}
						}
					else {
						ph->pNextPart = *pd;
						*pd = ph;
						}
					if (!gameData.smoke.depthBuf.nFreeParts)
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
pl = gameData.smoke.depthBuf.pPartList + 999999;
pl->pParticle = NULL;
if (!(h = ParticleCount ()))
	return 0;
pl = gameData.smoke.depthBuf.pPartList + 999999;
pl->pParticle = NULL;
DepthSortParticles ();
if (!LoadParticleImages ())
	return 0;
BeginRenderSmoke (-1, 1);
for (pd = gameData.smoke.depthBuf.pDepthBuffer + PART_DEPTHBUFFER_SIZE - 1;
	  pd >= gameData.smoke.depthBuf.pDepthBuffer;
	  pd--) {
		if ((pl = *pd)) {
		do {
			RenderParticle (pl->pParticle, pl->fBrightness);
			pn = pl->pNextPart;
			pl->pNextPart = NULL;
			pl = pn;
			} while (pl);
		*pd = NULL;
		}
	}
gameData.smoke.depthBuf.nFreeParts = PARTLIST_SIZE;
EndRenderSmoke (NULL);
return 1;
}

//------------------------------------------------------------------------------

int RenderClouds (void)
{
	int		h, i, j;

#if !EXTRA_VERTEX_ARRAYS
nBuffer = 0;
#endif
h = (gameOpts->render.bDepthSort > 0) ? -1 : CreateCloudList ();
if (!h)
	return 1;
if (h > 0) {
	do {
		RenderCloud (pCloudList [--h].pCloud, -1);
		} while (h);
	D2_FREE (pCloudList);
	pCloudList = NULL;
	}
else
	{
	tSmoke *pSmoke = gameData.smoke.buffer;
	tCloud *pCloud;
	for (h = 0, i = gameData.smoke.iUsed; i >= 0; i = pSmoke->nNext) {
		pSmoke = gameData.smoke.buffer + i;
		if (pSmoke->pClouds) {
			if (!LoadParticleImage (pSmoke->nType))
				return 0;
			if ((pSmoke->nObject >= 0) && (pSmoke->nObject < 0x70000000) && (gameData.smoke.objects [pSmoke->nObject] < 0))
				SetSmokeLife (i, 0);
			for (j = pSmoke->nClouds, pCloud = pSmoke->pClouds; j; j--, pCloud++) {
				pCloud->fBrightness = CloudBrightness (pCloud);
				h += RenderCloud (pCloud, -1);
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

int RenderSmoke (void)
{
int h = (gameOpts->render.smoke.bSort && (gameOpts->render.bDepthSort <= 0)) ? RenderParticles () : RenderClouds ();
return h;
}

//------------------------------------------------------------------------------

void SetSmokePos (int i, vmsVector *pos, vmsMatrix *orient, short nSegment)
{
if (0 <= IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.buffer + i;
	if (pSmoke->pClouds)
		for (i = 0; i < pSmoke->nClouds; i++)
			SetCloudPos (pSmoke->pClouds, pos, orient, nSegment);
#if DBG
	else if (pSmoke->nObject >= 0) {
		HUDMessage (0, "no smoke in SetSmokePos (%d,%d)\n", i, pSmoke->nObject);
		//PrintLog ("no gameData.smoke.buffer in SetSmokePos (%d,%d)\n", i, pSmoke->nObject);
		pSmoke->nObject = -1;
		}
#endif
	}
}

//------------------------------------------------------------------------------

void SetSmokeDensity (int i, int nMaxParts, int nDensity)
{
if (0 <= IsUsedSmoke (i)) {
	nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.smoke.nDens [0]);
	tSmoke *pSmoke = gameData.smoke.buffer + i;
	if (pSmoke->pClouds)
		for (i = 0; i < pSmoke->nClouds; i++)
			SetCloudDensity (pSmoke->pClouds + i, nMaxParts, nDensity);
	}
}

//------------------------------------------------------------------------------

void SetSmokePartScale (int i, float nPartScale)
{
if (0 <= IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.buffer + i;
	if (pSmoke->pClouds)
		for (i = 0; i < pSmoke->nClouds; i++)
			SetCloudPartScale (pSmoke->pClouds + i, nPartScale);
	}
}

//------------------------------------------------------------------------------

void SetSmokeLife (int i, int nLife)
{
if (0 <= IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.buffer + i;
	if (pSmoke->pClouds && (pSmoke->pClouds->nLife != nLife)) {
		//PrintLog ("SetSmokeLife (%d,%d) = %d\n", i, pSmoke->nObject, nLife);
		int j;
		for (j = 0; j < pSmoke->nClouds; j++)
			SetCloudLife (pSmoke->pClouds, nLife);
		}
	}
}

//------------------------------------------------------------------------------

void SetSmokeBrightness (int i, int nBrightness)
{
if (0 <= IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.buffer + i;
	if (pSmoke->pClouds && (pSmoke->pClouds->nDefBrightness != nBrightness)) {
		//PrintLog ("SetSmokeLife (%d,%d) = %d\n", i, pSmoke->nObject, nLife);
		int j;
		for (j = 0; j < pSmoke->nClouds; j++)
			SetCloudBrightness (pSmoke->pClouds, nBrightness);
		}
	}
}

//------------------------------------------------------------------------------

void SetSmokeType (int i, int nType)
{
if (0 <= IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.buffer + i;
	pSmoke->nType = nType;
	for (i = 0; i < pSmoke->nClouds; i++)
		SetCloudType (pSmoke->pClouds + i, nType);
	}
}

//------------------------------------------------------------------------------

void SetSmokeSpeed (int i, int nSpeed)
{
if (0 <= IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.buffer + i;
	pSmoke->nSpeed = nSpeed;
	for (i = 0; i < pSmoke->nClouds; i++)
		SetCloudSpeed (pSmoke->pClouds + i, nSpeed);
	}
}

//------------------------------------------------------------------------------

void SetSmokeDir (int i, vmsVector *pDir)
{
if (0 <= IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.buffer + i;
	for (i = 0; i < pSmoke->nClouds; i++)
		SetCloudDir (pSmoke->pClouds + i, pDir);
	}
}

//------------------------------------------------------------------------------

int SetSmokeObject (int nObject, int nSmoke)
{
if ((nObject < 0) || (nObject >= MAX_OBJECTS))
	return -1;
return gameData.smoke.objects [nObject] = nSmoke;
}

//------------------------------------------------------------------------------

int GetSmokeType (int i)
{
return (IsUsedSmoke (i)) ? gameData.smoke.buffer [i].nType : -1;
}

//------------------------------------------------------------------------------

tCloud *GetCloud (int i, int j)
{
if (0 <= IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.buffer + i;
	return (pSmoke->pClouds && (j < pSmoke->nClouds)) ? pSmoke->pClouds + j : NULL;
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
if (gameOpts->render.smoke.bDisperse)
	return (float) (PARTICLE_RAD * (nSize + 1)) / nScale + 0.5f;
else
	return (float) (PARTICLE_RAD * (nSize + 1) * (nSize + 2) / 2) / nScale + 0.5f;
}

//------------------------------------------------------------------------------
//eof
