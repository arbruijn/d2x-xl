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
#include "ogl_init.h"
#include "piggy.h"
#include "globvars.h"
#include "gameseg.h"
#include "network.h"
#include "lighting.h"
#include "objsmoke.h"
#include "particles.h"

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

#define OGL_VERTEX_ARRAYS	1

#define PARTICLE_FPS	30

#define PARTICLE_RAD	(F1_0)

static int bHavePartImg [2][3] = {{0, 0, 0},{0, 0, 0}};

static grsBitmap *bmpParticle [2][3] = {{NULL, NULL, NULL},{NULL, NULL, NULL}};
static grsBitmap *bmpBumpMaps [2] = {NULL, NULL};

static char *szParticleImg [2][3] = {
#if 1
	{"blksmoke.tga", "grysmoke.tga", "whtsmoke.tga"},
	{"blksmoke.tga", "grysmoke.tga", "whtsmoke.tga"}
#else
	{"expl1.tga", "expl1.tga", "expl1.tga"},
	{"expl1.tga", "expl1.tga", "expl1.tga"}
#	if 0
	{"blakpart.tga", "graypart.tga", "whitpart.tga"},
	{"blakpart.tga", "graypart.tga", "whitpart.tga"}
#	endif
#endif
	};

static int nParticleFrames [2][3] = {{1,1,1},{1,1,1}};
static int iParticleFrames [2][3] = {{0,0,0},{0,0,0}};

#if EXTRA_VERTEX_ARRAYS

#define VERT_BUF_SIZE	512 //8192

static GLdouble vertexBuffer [VERT_BUF_SIZE][3];
static GLdouble texCoordBuffer [VERT_BUF_SIZE][2];
static GLdouble colorBuffer [VERT_BUF_SIZE][4];
static int iBuffer = 0;
#else
static int iBuffer = 0;
static int nBuffer = 0;
#endif

#define SMOKE_START_ALPHA		(gameOpts->render.smoke.bDisperse ? 32 : 64)

//	-----------------------------------------------------------------------------

void InitSmoke (void)
{
	int i, j;
#if OGL_VERTEX_BUFFERS
	GLdouble	pf = colorBuffer;

for (i = 0; i < VERT_BUFFER_SIZE; i++, pf++) {
	*pf++ = 1.0;	
	*pf++ = 1.0;	
	*pf++ = 1.0;	
	}
#endif
for (i = 0, j = 1; j < MAX_SMOKE; i++, j++) 
	gameData.smoke.smoke [i].nNext = j;
gameData.smoke.smoke [i].nNext = -1;
gameData.smoke.iFreeSmoke = 0;
gameData.smoke.iUsedSmoke = -1;
}

//	-----------------------------------------------------------------------------

grsBitmap *ReadParticleImage (char *szFile)
{
	grsBitmap	*bmP = NULL;

if (!(bmP = GrCreateBitmap (0, 0, 0)))
	return NULL;
if (ReadTGA (szFile, NULL, bmP, -1, 1.0, 0, 0)) {
	bmP->bmType = BM_TYPE_ALT;
	return bmP;
	}
bmP->bmType = BM_TYPE_ALT;
GrFreeBitmap (bmP);
return NULL;
}

//	-----------------------------------------------------------------------------

int LoadParticleImage (int nType)
{
	int			*flagP = bHavePartImg [gameStates.render.bPointSprites] + nType;
	grsBitmap	*bmP = NULL;

if (*flagP < 0)
	return 0;
if (*flagP > 0)
	return 1;
nType %= 3;
bmP = ReadParticleImage (szParticleImg [gameStates.render.bPointSprites][nType]);
*flagP = bmP ? 1 : -1;
if (*flagP < 0)
	return 0;
bmpParticle [gameStates.render.bPointSprites][nType] = bmP;
BM_FRAMECOUNT (bmP) = bmP->bm_props.h / bmP->bm_props.w;
OglLoadBmTexture (bmP, 0, 1);
nParticleFrames [gameStates.render.bPointSprites][nType] = BM_FRAMECOUNT ((bmP));
return *flagP > 0;
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
return (int) ((double) rand () * (double) n / (double) RAND_MAX);
}

//------------------------------------------------------------------------------

inline double sqr (double n)
{
return n * n;
}

//------------------------------------------------------------------------------

int CreateParticle (tParticle *pParticle, vmsVector *pPos, vmsVector *pDir,
						  short nSegment, int nLife, 
						  int nSpeed, int nType, float nScale, int nCurTime, int bStart)
{
	vmsVector	vDrift;
	int			nRad;

if (nScale < 0)
	nRad = (int) -nScale;
else if (gameOpts->render.smoke.bSyncSizes)
	nRad = (int) PARTICLE_SIZE (gameOpts->render.smoke.nSize [0], nScale);
else
	nRad = (int) nScale;
if (!nRad)
	nRad = F1_0;
pParticle->nType = nType;
pParticle->nSegment = nSegment;
pParticle->nBounce = 0;
#if 0
if (bStart == 2) {
	pParticle->glColor.r = 1.0;
	pParticle->glColor.g = 0.8;
	pParticle->glColor.b = 0.0;//(double) (64 + randN (64)) / 255.0;
	pParticle->glColor.a = (double) (SMOKE_START_ALPHA + randN (64)) / 255.0;
	}
else if (bStart == 1) {
	pParticle->glColor.r = 1.0;
	pParticle->glColor.g = 0.5;
	pParticle->glColor.b = 0.0;//(double) (64 + randN (64)) / 255.0;
	pParticle->glColor.a = (double) (SMOKE_START_ALPHA + randN (64)) / 255.0;
	}
else {
	pParticle->glColor.r = 0.0;
	pParticle->glColor.g = 0.5;
	pParticle->glColor.b = 1.0;//(double) (64 + randN (64)) / 255.0;
	pParticle->glColor.a = (double) (SMOKE_START_ALPHA + randN (64)) / 255.0;
	}
#else
pParticle->glColor.r = 1.0;
pParticle->glColor.g = 0.0;
pParticle->glColor.b = 0.0;//(double) (64 + randN (64)) / 255.0;
pParticle->glColor.a = (double) (SMOKE_START_ALPHA + randN (64)) / 255.0;
#	if 0
pParticle->glColor.r =
pParticle->glColor.g =
pParticle->glColor.b = pParticle->glColor.a - 0.5;
#	endif
#endif
#if 0
h = 1 << (2 - nType);
pParticle->glColor.r /= h;
pParticle->glColor.g /= h;
pParticle->glColor.b /= h;
#endif
//nSpeed = (int) (sqrt (nSpeed) * (double) F1_0);
nSpeed = nSpeed * F1_0;
if (pDir) {
	vmsAngVec	a;
	vmsMatrix	m;
	double		d;
#if 1
	a.p = randN (F1_0 / 4) - F1_0 / 8;
	a.b = randN (F1_0 / 4) - F1_0 / 8;
	a.h = randN (F1_0 / 4) - F1_0 / 8;
	VmAngles2Matrix (&m, &a);
	VmVecNormalize (VmVecRotate (&vDrift, pDir, &m));
	d = (double) VmVecDeltaAng (&vDrift, pDir, NULL);
	if (d) {
		d = exp ((F1_0 / 8) / d);
		nSpeed = (fix) ((double) nSpeed / d);
		}
	pParticle->glColor.g =
	pParticle->glColor.b = 1.0;//(double) (64 + randN (64)) / 255.0;
#else
	vDrift = *pDir;
#endif
	VmVecScaleAdd (&pParticle->pos, pPos, &vDrift, 200);
	VmVecScale (&vDrift, nSpeed);
	pParticle->dir = *pDir;
	}
else {
	vmsVector	vOffs;
	vDrift.p.x = nSpeed - randN (2 * nSpeed);
	vDrift.p.y = nSpeed - randN (2 * nSpeed);
	vDrift.p.z = nSpeed - randN (2 * nSpeed);
	vOffs = vDrift;
	VmVecScaleAdd (&pParticle->pos, pPos, &vDrift, 10);
	VmVecZero (&pParticle->dir);
	}
pParticle->drift = vDrift;
if (nLife < 0)
	nLife = -nLife;
if (gameOpts->render.smoke.bDisperse)
	nLife = (nLife * 2) / 3;
pParticle->nLife = 
pParticle->nTTL = nLife / 2 + randN (nLife / 2);
pParticle->nOrient = randN (4);
pParticle->nMoved = nCurTime;
pParticle->nDelay = 0; //bStart ? randN (nLife) : 0;
pParticle->nRad = nRad + randN (nRad);
pParticle->nWidth =
pParticle->nHeight = pParticle->nRad / 2;
pParticle->glColor.a /= nType + 2; //4 - nType * 0.5; //nType + 2;
return 1;
}

//------------------------------------------------------------------------------

int DestroyParticle (tParticle *pParticle)
{
#ifdef _DEBUG
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
static int nVert [6];
static vmsVector	*wallNorm;

int CollideParticleAndWall (tParticle *pParticle)
{
	tSegment		*segP;
	tSide			*sideP;
	int			bInit, bRedo = 0, nSide, nVert, nChild, nFace, nInFront,
					nRad = max (pParticle->nWidth, pParticle->nHeight) / 2;
	fix			nDist;
	int			*vlP;
	vmsVector	pos = pParticle->pos;

//redo:

segP = gameData.segs.segments + pParticle->nSegment;
if (bInit = (pParticle->nSegment != nPartSeg)) 
	nPartSeg = pParticle->nSegment;
for (nSide = 0, sideP = segP->sides; nSide < 6; nSide++, sideP++) {
	vlP = vertexList [nSide];
	if (bInit) {
		CreateAbsVertexLists (nFaces + nSide, vlP, nPartSeg, nSide);
		if (nFaces [nSide] == 1) {
			bSidePokesOut [nSide] = 0;
			nVert = vlP [0];
			}
		else {
			nVert = min (vlP [0], vlP [2]);
			if (vlP [4] < vlP [1])
				nDist = VmDistToPlane (gameData.segs.vertices + vlP [4], sideP->normals, gameData.segs.vertices + nVert);
			else
				nDist = VmDistToPlane (gameData.segs.vertices + vlP [1], sideP->normals + 1, gameData.segs.vertices + nVert);
			bSidePokesOut [nSide] = (nDist > PLANE_DIST_TOLERANCE);
			}
		}
	else
		nVert = (nFaces [nSide] == 1) ? vlP [0] : min (vlP [0], vlP [2]);
	for (nFace = nInFront = 0; nFace < nFaces [nSide]; nFace++) {
		nDist = VmDistToPlane (&pos, sideP->normals + nFace, gameData.segs.vertices + nVert);
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

int MoveParticle (tParticle *pParticle, int nCurTime)
{
	int			j;
	fix			t, dot;
	vmsVector	pos, drift;
	fix			drag = fl2f ((float) pParticle->nLife / (float) pParticle->nTTL);


if (pParticle->nLife <= 0)
	return 0;
t = nCurTime - pParticle->nMoved;
//if ((1000 / PARTICLE_FPS) <= ()) 
	{
	if (pParticle->nDelay > 0)
		pParticle->nDelay -= t;
	else {
		pos = pParticle->pos;
		drift = pParticle->drift;
		if (pParticle->nType != 3) {
			drift.p.x = ChangeDir (drift.p.x);
			drift.p.y = ChangeDir (drift.p.y);
			drift.p.z = ChangeDir (drift.p.z);
			}
		for (j = 0; j < 2; j++) {
			VmVecScaleInc (&drift, &pParticle->dir, drag);
			VmVecScaleAdd (&pParticle->pos, &pos, &drift, t);
			if (gameOpts->render.smoke.bCollisions && CollideParticleAndWall (pParticle)) {	//reflect the particle
				if (j)
					return 0;
				else if (!(dot = VmVecDot (&drift, wallNorm)))
					return 0;
				else {
					VmVecScaleAdd (&drift, &pParticle->drift, wallNorm, -2 * dot);
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
			int nRad = pParticle->nRad;
		if (pParticle->nWidth < nRad) {
			pParticle->nWidth += nRad / 10;
			pParticle->nHeight += nRad / 10;
			if (pParticle->nWidth > nRad)
				pParticle->nWidth = nRad;
			if (pParticle->nHeight > nRad)
				pParticle->nHeight = nRad;
			pParticle->glColor.a *= 1.0725;
			}
#if 0
			//VmVecScaleFrac (&pParticle->drift, t - ((t / 10) ? t / 10 : 1), t);
			decay = 1.0 - (double) t / (double) pParticle->nTTL;
			pParticle->glColor.a *= decay;
			HUDMessage (0, "%1.5f", decay);
			if (0 && !gameStates.render.bPointSprites) {
				pParticle->nWidth = (short) ((double) pParticle->nWidth * decay);
				pParticle->nHeight = (short) ((double) pParticle->nHeight * decay);
				}
#endif
			pParticle->nLife -= t;
			}
		}	
	pParticle->nMoved = nCurTime;
	}
return 1;
}

//------------------------------------------------------------------------------

#if EXTRA_VERTEX_ARRAYS
static void FlushVertexArrays (void)
#else
static void FlushVertexArrays (tParticle *pParticles, int iBuffer)
#endif
{
if (iBuffer) {
	GLenum	i;

#if !EXTRA_VERTEX_ARRAYS
	glColorPointer (4, GL_DOUBLE, sizeof (tParticle) - sizeof (tPartColor), &pParticles->glColor);
	glVertexPointer (3, GL_DOUBLE, sizeof (tParticle) - sizeof (tPartPos), &pParticles->glPos);
	if (i = glGetError ()) {
#	ifdef _DEBUG
		HUDMessage (0, "glVertexPointer %d failed (%d)", iBuffer, i);
#	endif
		glVertexPointer (3, GL_DOUBLE, sizeof (tParticle) - sizeof (tPartPos), &pParticles->glPos);
		if (i = glGetError ())
			glVertexPointer (3, GL_DOUBLE, sizeof (tParticle) - sizeof (tPartPos), &pParticles->glPos);
		}
#else
	glVertexPointer (3, GL_DOUBLE, 0, vertexBuffer);
#	ifdef _DEBUG
	if (i = glGetError ())
		HUDMessage (0, "glVertexPointer %d failed (%d)", iBuffer, i);
#	endif
	if (!gameStates.render.bPointSprites) {
		glTexCoordPointer (2, GL_DOUBLE, 0, texCoordBuffer);
#	ifdef _DEBUG
		if (i = glGetError ())
			HUDMessage (0, "glTexCoordPointer %d failed (%d)", iBuffer, i);
#	endif
		}
	glColorPointer (4, GL_DOUBLE, 0, colorBuffer);
#	ifdef _DEBUG
	if (i = glGetError ())
		HUDMessage (0, "glColorPointer %d failed (%d)", iBuffer, i);
#	endif
#endif
	i = glGetError ();
	glDrawArrays (gameStates.render.bPointSprites ? GL_POINTS : GL_QUADS, 0, iBuffer);
	i = glGetError ();
	iBuffer = 0;
	}
}

//------------------------------------------------------------------------------

inline void SetParticleTexCoord (GLdouble u, GLdouble v, char orient)
{
#if OGL_VERTEX_ARRAYS
if (gameStates.render.bVertexArrays) {
	if (orient == 1) {
		texCoordBuffer [iBuffer][0] = 1.0 - v;
		texCoordBuffer [iBuffer][1] = u;
		}
	else if (orient == 2) {
		texCoordBuffer [iBuffer][0] = 1.0 - u;
		texCoordBuffer [iBuffer][1] = 1.0 - v;
		}
	else if (orient == 3) {
		texCoordBuffer [iBuffer][0] = v;
		texCoordBuffer [iBuffer][1] = 1.0 - u;
		}
	else {
		texCoordBuffer [iBuffer][0] = u;
		texCoordBuffer [iBuffer][1] = v;
		}
	}
else 
#endif
{
if (orient == 1)
	glTexCoord2d (1.0 - v, u);
else if (orient == 2)
	glTexCoord2d (1.0 - u, 1.0 - v);
else if (orient == 3)
	glTexCoord2d (v, 1.0 - u);
else 
	glTexCoord2d (u, v);
}
}

//------------------------------------------------------------------------------

int RenderParticle (tParticle *pParticle)
{
	vmsVector			hp;
	GLdouble				u, v, x, y, z, h, w;
	char					o;
	grsBitmap			*bmP;
	tPartColor			pc;
#if OGL_VERTEX_ARRAYS
	double				*pf;
#endif
	double				decay = (double) pParticle->nLife / (double) pParticle->nTTL;

if (pParticle->nDelay > 0)
	return 0;
bmP = bmpParticle [gameStates.render.bPointSprites][pParticle->nType % 3];
if (BM_CURFRAME (bmP))
	bmP = BM_CURFRAME (bmP);
if (gameOpts->render.smoke.bSort)
	hp = pParticle->transPos;
else
	G3TransformPoint (&hp, &pParticle->pos, 0);
//if (pParticle->glColor.r < 1.0)
//	pParticle->glColor.r += 0.1;
if (pParticle->nType > 2) {
	//pParticle->glColor.g *= 0.99;
	//pParticle->glColor.b *= 0.99;
	}
else {
	if (pParticle->glColor.g < 1.0) {
		pParticle->glColor.g += 1.0 / 3.0;
		if (pParticle->glColor.g > 1.0)
			pParticle->glColor.g = 1.0;
		}
	if (pParticle->glColor.b < 1.0) {
		pParticle->glColor.b += 1.0 / 6.0;
		if (pParticle->glColor.b > 1.0)
			pParticle->glColor.b = 1.0;
		}
	}
pc = pParticle->glColor;
pc.a *= /*gameOpts->render.smoke.bDisperse ? decay2 :*/ decay;
if (SHOW_DYN_LIGHT) {
	tFaceColor	*psc = AvgSgmColor (pParticle->nSegment, NULL);
	if (psc->index == gameStates.render.nFrameFlipFlop + 1) {
		pc.r *= (double) psc->color.red;
		pc.g *= (double) psc->color.green;
		pc.b *= (double) psc->color.blue;
		}
	}
#if OGL_POINT_SPRITES
if (gameStates.render.bPointSprites) {
#	if OGL_VERTEX_ARRAYS
	if (gameStates.render.bVertexArrays) {
#		if !EXTRA_VERTEX_ARRAYS
		pParticle->glPos.p.x = f2fl (hp.x);
		pParticle->glPos.p.y = f2fl (hp.y);
		pParticle->glPos.p.z = f2fl (hp.z);
		nBuffer++;
#		else
		memcpy (colorBuffer [iBuffer], &pc, sizeof (pc));
		pf = vertexBuffer [iBuffer];
		pf [0] = f2fl (hp.p.x);
		pf [1] = f2fl (hp.p.y);
		pf [2] = f2fl (hp.p.z);
		memcpy (colorBuffer [iBuffer++], &pc, sizeof (pc));
		if (iBuffer >= VERT_BUF_SIZE)
			FlushVertexArrays ();
#	endif
		}
	else
#endif
	{
	glColor4dv ((GLdouble *) &pc);
	glVertex3d (f2fl (hp.p.x), f2fl (hp.p.y), f2fl (hp.p.z));
	}
	}
else
#endif
	{
	u = bmP->glTexture->u;
	v = bmP->glTexture->v;
	o = pParticle->nOrient;
	if (gameOpts->render.smoke.bDisperse) {
		decay = sqrt (decay);
		w = f2fl (pParticle->nWidth) / decay;
		h = f2fl (pParticle->nHeight) / decay;
		}
	else {
		w = f2fl (pParticle->nWidth) * decay;
		h = f2fl (pParticle->nHeight) * decay;
		}
	x = f2fl (hp.p.x);
	y = f2fl (hp.p.y);
	z = f2fl (hp.p.z);
#if OGL_VERTEX_ARRAYS
	if (gameStates.render.bVertexArrays) {
		pf = vertexBuffer [iBuffer];
		pf [0] =
		pf [9] = x - w; 
		pf [3] =
		pf [6] = x + w; 
		pf [1] = 
		pf [4] = y + h; 
		pf [7] =
		pf [10] = y - h; 
		pf [2] =
		pf [5] =
		pf [8] =
		pf [11] = z;
		SetParticleTexCoord (0, 0, o);
		memcpy (colorBuffer [iBuffer++], &pc, sizeof (pc));
		SetParticleTexCoord (u, 0, o);
		memcpy (colorBuffer [iBuffer++], &pc, sizeof (pc));
		SetParticleTexCoord (u, v, o);
		memcpy (colorBuffer [iBuffer++], &pc, sizeof (pc));
		SetParticleTexCoord (0, v, o);
		memcpy (colorBuffer [iBuffer++], &pc, sizeof (pc));
		if (iBuffer >= VERT_BUF_SIZE)
			FlushVertexArrays ();
		}
	else {
#else
	{
#endif
	glColor4dv ((GLdouble *) &pc);
	SetParticleTexCoord (0, 0, o);
	glVertex3d (x - w, y + h, z);
	SetParticleTexCoord (u, 0, o);
	glVertex3d (x + w, y + h, z);
	SetParticleTexCoord (u, v, o);
	glVertex3d (x + w, y - h, z);
	SetParticleTexCoord (0, v, o);
	glVertex3d (x - w, y - h, z);
	}
	}
return 1;
}

//------------------------------------------------------------------------------

int BeginRenderSmoke (int nType, float nScale)
{
	grsBitmap	*bmP;
	int			nFrames;

nType %= 3;
bmP = bmpParticle [gameStates.render.bPointSprites][nType];
nFrames = nParticleFrames [gameStates.render.bPointSprites][nType];
if (nFrames > 1) {
	static time_t	t0 = 0;
	time_t			t = gameStates.app.nSDLTicks;
	int				iFrames = iParticleFrames [gameStates.render.bPointSprites][nType];
;

	BM_CURFRAME (bmP) = BM_FRAMES (bmP) + iFrames;
	if (t - t0 > 100) {
		t0 = t;
		iParticleFrames [gameStates.render.bPointSprites][nType] = (iFrames + 1) % nFrames;
		}
	}
OglActiveTexture (GL_TEXTURE0_ARB);
glEnable (GL_TEXTURE_2D);
if (EGI_FLAG (bShadows, 0, 1, 0)) // && (gameStates.render.nShadowPass == 3))
	glDisable (GL_STENCIL_TEST);
glDepthFunc (GL_LESS);
glDepthMask (0);
#if 0
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
#if OGL_POINT_SPRITES
if (gameStates.render.bPointSprites) {
	float quadratic [] =  {1.0f, 0.0f, 0.01f};
   float partSize = f2fl ((gameOpts->render.smoke.bSyncSizes ? 
									PARTICLE_SIZE (gameOpts->render.smoke.nSize [0], nScale) : nScale)) * 128; 
	static float maxSize = -1.0f;
	glPointParameterfvARB (GL_POINT_DISTANCE_ATTENUATION_ARB, quadratic);
	glPointParameterfARB (GL_POINT_FADE_THRESHOLD_SIZE_ARB, 60.0f);
	glTexEnvf (GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable (GL_POINT_SPRITE_ARB);
	if (maxSize < 0)
	   glGetFloatv (GL_POINT_SIZE_MAX_ARB, &maxSize);
	glPointSize ((maxSize > partSize) ? partSize : maxSize);
	glPointParameterfARB (GL_POINT_SIZE_MIN_ARB, 1.0f);
	glPointParameterfARB (GL_POINT_SIZE_MAX_ARB, 100 * maxSize);
	if (OglBindBmTex (bmP, 1))
		return 0;
#	if OGL_VERTEX_ARRAYS
	if (gameStates.render.bVertexArrays) {
#	ifdef _DEBUG
		GLenum i;
#endif
		glEnableClientState (GL_VERTEX_ARRAY);
#	ifdef _DEBUG
		if (i = glGetError ()) 
			HUDMessage (0, "glEnableClientState (GL_VERTEX_ARRAY) %d failed (%d)", iBuffer, i);
#	endif
		glEnableClientState (GL_COLOR_ARRAY);
#	ifdef _DEBUG
		if (i = glGetError ()) 
			HUDMessage (0, "glEnableClientState (GL_COLOR_ARRAY) %d failed (%d)", iBuffer, i);
#	endif
		}
	else
#	endif
		glBegin (GL_POINTS);
	}
else
#endif
	{
	if (OglBindBmTex (bmP, 1))
		return 0;
#if OGL_VERTEX_ARRAYS
	if (gameStates.render.bVertexArrays) {
#ifdef _DEBUG
		GLenum i;
#endif
		glDisableClientState (GL_VERTEX_ARRAY);
		glEnableClientState (GL_VERTEX_ARRAY);
#ifdef _DEBUG
		if (i = glGetError ()) 
			HUDMessage (0, "glEnableClientState (GL_VERTEX_ARRAY) failed (%d)", i);
#endif
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
#ifdef _DEBUG
		if (i = glGetError ()) 
			HUDMessage (0, "glEnableClientState (GL_TEX_COORD_ARRAY) failed (%d)", i);
#endif
		glEnableClientState (GL_COLOR_ARRAY);
#ifdef _DEBUG
		if (i = glGetError ()) 
			HUDMessage (0, "glEnableClientState (GL_COLOR_ARRAY) failed (%d)", i);
#endif
		}
	else
#endif
		glBegin (GL_QUADS);
	}
return 1;
}

//------------------------------------------------------------------------------

int EndRenderSmoke (tCloud *pCloud)
{
#if OGL_VERTEX_ARRAYS
if (gameStates.render.bVertexArrays) {
#	if EXTRA_VERTEX_ARRAYS
	FlushVertexArrays ();
#	else
	FlushVertexArrays (pCloud->pParticles + iBuffer, nBuffer - iBuffer);
#	endif
	glDisableClientState (GL_COLOR_ARRAY);
	glDisableClientState (GL_VERTEX_ARRAY);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}
else
	glEnd ();
#else
glEnd ();
#endif
#if OGL_POINT_SPRITES
if (gameStates.render.bPointSprites) {
	glDisable (GL_POINT_SPRITE_ARB);
	}
#endif
OGL_BINDTEX (0);
glDepthMask (1);
if (EGI_FLAG (bShadows, 0, 1, 0))// && (gameStates.render.nShadowPass == 3))
	glEnable (GL_STENCIL_TEST);
//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//glEnable (GL_DEPTH_TEST);
//glDisable (GL_TEXTURE_2D);
return 1;
}

//------------------------------------------------------------------------------

int CreateCloud (tCloud *pCloud, vmsVector *pPos, vmsVector *pDir,
					  short nSegment, short nObject, int nMaxParts, float nPartScale, 
					  int nDensity, int nPartsPerPos, int nLife, int nSpeed, int nType, int nCurTime)
{
if (!(pCloud->pParticles = (tParticle *) d_malloc (nMaxParts * sizeof (tParticle))))
	return 0;
if (gameOpts->render.smoke.bSort) {
	if (!(pCloud->pPartIdx = (tPartIdx *) d_malloc (nMaxParts * sizeof (tPartIdx))))
		gameOpts->render.smoke.bSort = 0;
	}
else
	pCloud->pPartIdx = NULL;
pCloud->nLife = nLife;
pCloud->nBirth = nCurTime;
pCloud->nSpeed = nSpeed;
pCloud->nType = nType;
if (pCloud->bHaveDir = (pDir != NULL))
	pCloud->dir = *pDir;
pCloud->prevPos =
pCloud->pos = *pPos;
pCloud->bHavePrevPos = 1;
pCloud->nParts = 0;
pCloud->nMoved = nCurTime;
pCloud->nPartLimit =
pCloud->nMaxParts = nMaxParts;
pCloud->nPartScale = nPartScale;
pCloud->nDensity = nDensity;
pCloud->nPartsPerPos = nPartsPerPos;
pCloud->nSegment = nSegment;
pCloud->nObject = nObject;
return 1;
}

//------------------------------------------------------------------------------

int DestroyCloud (tCloud *pCloud)
{
if (pCloud->pParticles) {
	d_free (pCloud->pParticles);
	pCloud->pParticles = NULL;
	d_free (pCloud->pPartIdx);
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

int MoveCloud (tCloud *pCloud, int nCurTime)
{
	tCloud		c = *pCloud;
	int			t, h, i, j = c.nParts,
					nDens, nInterpolatePos = 0;
	float			fDist;
	vmsVector	vDelta, vPos, *pDir = (pCloud->bHaveDir ? &pCloud->dir : NULL);
	fVector		vDeltaf, vPosf;

t = nCurTime - c.nMoved;
//if (t < (1000 / PARTICLE_FPS))
//	return 0;
nPartSeg = -1;
for (i = 0; i < j; )
	if (MoveParticle (c.pParticles + i, nCurTime))
		i++;
	else if (i < --j)
		c.pParticles [i] = c.pParticles [j];
if (CloudLives (pCloud, nCurTime)) {
	fDist = f2fl (VmVecMag (VmVecSub (&vDelta, &c.pos, &c.prevPos)));
	nDens = ((pCloud->nDensity < 0) ? gameOpts->render.smoke.nDens [0] : pCloud->nDensity) + 1;
	if (c.bHavePrevPos && (fDist > 0))
		h = (int) (fDist + 0.5) * nDens * c.nPartsPerPos;
	else
		h = (c.nMaxParts * t) / abs (c.nLife);
	if (!h)
		h = 1;
	if (h > c.nMaxParts - i)
		h = c.nMaxParts - i;
	//LogErr ("   creating %vDelta particles\n", h);
	//HUDMessage (0, "have %d/%d particles, creating %d, moved %1.2f\n", i, c.nMaxParts, h, fDist);
	if (!h)
		return c.nMaxParts;
	else if (h == 1)
		h = 1;
#ifdef _DEBUG
	if (c.prevPos.p.x == c.pos.p.x && c.prevPos.p.y == c.pos.p.y && c.prevPos.p.z == c.pos.p.z)
		c.bHavePrevPos = 0;
#endif
	if (c.bHavePrevPos && (fDist > 0)) {
		nInterpolatePos = 2;
		VmsVecToFloat (&vPosf, &c.prevPos);
		VmsVecToFloat (&vDeltaf, &vDelta);
		vDeltaf.p.x /= (float) h;
		vDeltaf.p.y /= (float) h;
		vDeltaf.p.z /= (float) h;
		}
	else {
		nInterpolatePos = 0;
		VmsVecToFloat (&vPosf, &c.pos);
		vDeltaf.p.x =
		vDeltaf.p.y =
		vDeltaf.p.z = 0.0f;
		}
	for (; h; h--, i++) {
		VmVecIncf (&vPosf, &vDeltaf);
		vPos.p.x = (fix) (vPosf.p.x * 65536.0f);
		vPos.p.y = (fix) (vPosf.p.y * 65536.0f);
		vPos.p.z = (fix) (vPosf.p.z * 65536.0f);
		CreateParticle (c.pParticles + i, &vPos, pDir, c.nSegment, c.nLife, 
							 c.nSpeed, c.nType, c.nPartScale, nCurTime, nInterpolatePos);
		nInterpolatePos = (h > 0);
		}
	}
pCloud->nMoved = nCurTime;
pCloud->prevPos = pCloud->pos;
pCloud->bHavePrevPos = 1;
return pCloud->nParts = i;
}

//------------------------------------------------------------------------------

void QSortParticles (tPartIdx *pPartIdx, int left, int right)
{
	int	l = left, 
			r = right, 
			m = pPartIdx [(l + r) / 2].z;

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
if (l < right)
	QSortParticles (pPartIdx, l, right);
if (left < r)
	QSortParticles (pPartIdx, left, r);
}

//------------------------------------------------------------------------------

void TransformParticles (tParticle *pParticles, int nParticles)
{
for (; nParticles; nParticles--, pParticles++)
	G3TransformPoint (&pParticles->transPos, &pParticles->pos, 0);
}

//------------------------------------------------------------------------------

int SortParticles (tParticle *pParticles, tPartIdx *pPartIdx, int nParticles)
{
if (nParticles > 1) {
	int	i, z, nSortedUp = 1, nSortedDown = 1;

	TransformParticles (pParticles, nParticles);
	for (i = 0; i < nParticles; i++) {
		pPartIdx [i].i = i;
		pPartIdx [i].z = pParticles [i].transPos.p.z;
		if (i)
			if (z > pPartIdx [i].z)
				nSortedUp++;
			else if (z < pPartIdx [i].z)
				nSortedDown++;
			else {
				nSortedUp++;
				nSortedDown++;
				}
		z = pPartIdx [i].z;
		}
	if (nSortedDown >= 9 * nParticles / 10)
		return 0;
	if (nSortedUp >= 9 * nParticles / 10)
		return 1;
	QSortParticles (pPartIdx, 0, nParticles - 1);
	}
return 0;
}

//------------------------------------------------------------------------------

int RenderCloud (tCloud *pCloud)
{
	int		i, j = pCloud->nParts, 
				bSorted = gameOpts->render.smoke.bSort && (j > 1),
				bReverse;
	tPartIdx	*pPartIdx;

if (!BeginRenderSmoke (pCloud->nType, pCloud->nPartScale))
	return 0;
if (bSorted) {
	pPartIdx = pCloud->pPartIdx;
	bReverse = SortParticles (pCloud->pParticles, pPartIdx, j);
	if (bReverse)
		for (i = j; i; )
			RenderParticle (pCloud->pParticles + pPartIdx [--i].i);
	else
		for (i = 0; i < j; i++)
			RenderParticle (pCloud->pParticles + pPartIdx [i].i);
	}
for (i = 0; i < j; i++)
#if OGL_VERTEX_ARRAYS && !EXTRA_VERTEX_ARRAYS
	if (!RenderParticle (pCloud->pParticles + i))
		if (gameStates.render.bVertexArrays) {
			FlushVertexArrays (pCloud->pParticles + iBuffer, nBuffer - iBuffer);
			nBuffer = iBuffer;
			}
#else
	RenderParticle (pCloud->pParticles + i);
#endif
return EndRenderSmoke (pCloud);
}

//------------------------------------------------------------------------------

void SetCloudPos (tCloud *pCloud, vmsVector *pos)
{
int nNewSeg = FindSegByPoint (pos, pCloud->nSegment);
pCloud->pos = *pos;
if (nNewSeg >= 0)
	pCloud->nSegment = nNewSeg;
}

//------------------------------------------------------------------------------

void SetCloudDir (tCloud *pCloud, vmsVector *pDir)
{
if (pCloud->bHaveDir = (pDir != NULL))
	pCloud->dir = *pDir;
}

//------------------------------------------------------------------------------

void SetCloudLife (tCloud *pCloud, int nLife)
{
pCloud->nLife = nLife;
}

//------------------------------------------------------------------------------

void SetCloudType (tCloud *pCloud, int nType)
{
pCloud->nType = nType;
}

//------------------------------------------------------------------------------

int SetCloudDensity (tCloud *pCloud, int nMaxParts, int nDensity)
{
	tParticle	*p;

if (pCloud->nMaxParts == nMaxParts)
	return 1;
if (nMaxParts > pCloud->nPartLimit) {
	pCloud->nPartLimit = nMaxParts;
	if (!(p = d_malloc (nMaxParts * sizeof (tParticle))))
		return 0;
	if (pCloud->pParticles) {
		memcpy (p, pCloud->pParticles, pCloud->nParts * sizeof (tParticle));
		d_free (pCloud->pParticles);
		}
	pCloud->pParticles = p;
	}
pCloud->nDensity = nDensity;
pCloud->nMaxParts = nMaxParts;
if (pCloud->nParts > nMaxParts)
	pCloud->nParts = nMaxParts;
return 1;
}

//------------------------------------------------------------------------------

void SetCloudPartScale (tCloud *pCloud, float nPartScale)
{
pCloud->nPartScale = nPartScale;
}

//------------------------------------------------------------------------------

int IsUsedSmoke (int iSmoke)
{
	int	i;

for (i = gameData.smoke.iUsedSmoke; i >= 0; i = gameData.smoke.smoke [i].nNext)
	if (iSmoke == i)
		return 1;
return 0;
}

//------------------------------------------------------------------------------

int RemoveCloud (int iSmoke, int iCloud)
{
	tSmoke	*pSmoke;

if (!IsUsedSmoke (iSmoke))
	return -1;
pSmoke = gameData.smoke.smoke + iSmoke;
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

for (i = gameData.smoke.iUsedSmoke; i >= 0; i = j)
	if ((j = gameData.smoke.smoke [i].nNext) == iSmoke)
		return gameData.smoke.smoke + i;
return NULL;
}

//------------------------------------------------------------------------------

int DestroySmoke (int iSmoke)
{
	int		i;
	tSmoke	*pSmoke;

if (iSmoke < 0)
	iSmoke = -iSmoke - 1;
else if (!IsUsedSmoke (iSmoke))
	return -1;
pSmoke = gameData.smoke.smoke + iSmoke;
if (pSmoke->pClouds) {
	for (i = pSmoke->nClouds; i; )
		DestroyCloud (pSmoke->pClouds + --i);
	d_free (pSmoke->pClouds);
	pSmoke->pClouds = NULL;
	i = pSmoke->nNext;
	if (gameData.smoke.iUsedSmoke == iSmoke)
		gameData.smoke.iUsedSmoke = i;
	pSmoke->nNext = gameData.smoke.iFreeSmoke;
	if (pSmoke = PrevSmoke (iSmoke))
		pSmoke->nNext = i;
	gameData.smoke.iFreeSmoke = iSmoke;
	}
return iSmoke;
}

//------------------------------------------------------------------------------

int DestroyAllSmoke (void)
{
	int	i, j;

for (i = gameData.smoke.iUsedSmoke; i >= 0; i = j) {
	j = gameData.smoke.smoke [i].nNext;
	DestroySmoke (-i - 1);
	}
FreeParticleImages ();
InitSmoke ();
return 1;
}

//------------------------------------------------------------------------------

int CreateSmoke (vmsVector *pPos, vmsVector *pDir, short nSegment, int nMaxClouds, int nMaxParts, 
					  float nPartScale, int nDensity, int nPartsPerPos, 
					  int nLife, int nSpeed, int nType, int nObject)
{
#if 0
if (!(EGI_FLAG (bUseSmoke, 0, 1, 0)))
	return 0;
else 
#endif
if (gameData.smoke.iFreeSmoke < 0)
	return 0;
else if (!LoadParticleImage (nType)) {
	//LogErr ("cannot create gameData.smoke.smoke\n");
	return -1;
	}
else {
		int		i, t = gameStates.app.nSDLTicks;
		tSmoke	*pSmoke;

	nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.smoke.nDens [0]);
	if (gameStates.render.bPointSprites)
		nMaxParts *= 2;
	srand (SDL_GetTicks ());
	pSmoke = gameData.smoke.smoke + gameData.smoke.iFreeSmoke;
	if (!(pSmoke->pClouds = (tCloud *) d_malloc (nMaxClouds * sizeof (tCloud)))) {
		//LogErr ("cannot create gameData.smoke.smoke\n");
		return 0;
		}
	pSmoke->nObject = nObject;
	pSmoke->nSignature = gameData.objs.objects [nObject].nSignature;
	pSmoke->nClouds = 0;
	pSmoke->nMaxClouds = nMaxClouds;
	for (i = 0; i < nMaxClouds; i++)
		if (CreateCloud (pSmoke->pClouds + i, pPos, pDir, nSegment, nObject, nMaxParts, nPartScale, nDensity, 
							  nPartsPerPos, nLife, nSpeed, nType, t))
			pSmoke->nClouds++;
		else {
			DestroySmoke (gameData.smoke.iFreeSmoke);
			//LogErr ("cannot create gameData.smoke.smoke\n");
			return -1;
			}
	pSmoke->nType = nType;
	i = gameData.smoke.iFreeSmoke;
	gameData.smoke.iFreeSmoke = pSmoke->nNext;
	pSmoke->nNext = gameData.smoke.iUsedSmoke;
	gameData.smoke.iUsedSmoke = i;
	//LogErr ("CreateSmoke (%d) = %d,%d (%d)\n", nObject, i, nMaxClouds, nType);
	return gameData.smoke.iUsedSmoke;
	}
}

//------------------------------------------------------------------------------

int MoveSmoke (void)
{
if (!gameStates.app.tick40fps.bTick)
	return 0;
#if 0
if (!EGI_FLAG (bUseSmoke, 0, 1, 0))
	return 0;
else 
#endif
{
		int		h, i, j = 0, t = gameStates.app.nSDLTicks;
		tSmoke	*pSmoke;
		tCloud	*pCloud;

	for (i = gameData.smoke.iUsedSmoke; i >= 0; i = pSmoke->nNext) {
		pSmoke = gameData.smoke.smoke + i;
#ifdef _DEBUG
		if (gameData.objs.objects [pSmoke->nObject].nType == 255)
			i = i;
#endif
		if (pCloud = pSmoke->pClouds)
			for (h = j = 0; j < pSmoke->nClouds; ) {
				if (!pSmoke->pClouds)
					return 0;
				if (CloudIsDead (pCloud, t)) {
					if (!RemoveCloud (i, j)) {
						//LogErr ("killing gameData.smoke.smoke %d (%d)\n", i, pSmoke->nObject);
						DestroySmoke (i);
						return 0;
						}
					}
				else {
					//LogErr ("moving %d (%d)\n", i, pSmoke->nObject);
					if ((pSmoke->nObject < 0) || (gameData.objs.objects [pSmoke->nObject].nType == 255))
						SetCloudLife (pCloud, 0);
					if (MoveCloud (pCloud, t))
						h++;
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
if (l < right)
	QSortClouds (l, right);
if (left < r)
	QSortClouds (left, r);
}

//------------------------------------------------------------------------------

int CloudCount (void)
{
	int		i, j;
	tSmoke	*pSmoke = gameData.smoke.smoke;

for (i = gameData.smoke.iUsedSmoke, j = 0; i >= 0; i = pSmoke->nNext) {
	pSmoke = gameData.smoke.smoke + i;
	if (pSmoke->pClouds) {
		j += pSmoke->nClouds;
		if (gameData.smoke.objects [pSmoke->nObject] < 0)
			SetSmokeLife (i, 0);
		}
	}
return j;
}

//------------------------------------------------------------------------------

int CreateCloudList (void)
{
	int			h, i, j, k;
	vmsVector	vPos;
	tSmoke		*pSmoke = gameData.smoke.smoke;

h = CloudCount ();
if (!h)
	return 0;
if (!(pCloudList = d_malloc (h * sizeof (tCloudList))))
	return -1;
for (i = gameData.smoke.iUsedSmoke, k = 0; i >= 0; i = pSmoke->nNext) {
	pSmoke = gameData.smoke.smoke + i;
	if (!LoadParticleImage (pSmoke->nType)) {
		d_free (pCloudList);
		return 0;
		}
	if (pSmoke->pClouds) {
		for (j = 0; j < pSmoke->nClouds; j++, k++) {
			pCloudList [k].pCloud = pSmoke->pClouds + j;
			G3TransformPoint (&vPos, &pSmoke->pClouds [j].pos, 0);
			pCloudList [k].xDist = VmVecMag (&vPos);
			}
		}
	}
if (h > 1)
	QSortClouds (0, h - 1);
return h;
}

//------------------------------------------------------------------------------

int RenderSmoke (void)
{
	int		h, i, j;

#if !EXTRA_VERTEX_ARRAYS
nBuffer = 0;
#endif
h = CreateCloudList ();
if (!h)
	return 1;
else if (h > 0) {
	do {
		RenderCloud (pCloudList [--h].pCloud);
		} while (h);
	d_free (pCloudList);
	pCloudList = NULL;
	}
else {
	tSmoke *pSmoke = gameData.smoke.smoke;

	for (i = gameData.smoke.iUsedSmoke; i >= 0; i = pSmoke->nNext) {
		pSmoke = gameData.smoke.smoke + i;
		if (pSmoke->pClouds) {
			if (!LoadParticleImage (pSmoke->nType))
				return 0;
			if (gameData.smoke.objects [pSmoke->nObject] < 0)
				SetSmokeLife (i, 0);
			for (j = 0; j < pSmoke->nClouds; j++)
				RenderCloud (pSmoke->pClouds + j);
			}
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void SetSmokePos (int i, vmsVector *pos)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.smoke + i;
	if (pSmoke->pClouds)
		for (i = 0; i < pSmoke->nClouds; i++)
			SetCloudPos (pSmoke->pClouds, pos);
#ifdef _DEBUG
	else if (pSmoke->nObject >= 0) {
		HUDMessage (0, "no smoke in SetSmokePos (%d,%d)\n", i, pSmoke->nObject);
		//LogErr ("no gameData.smoke.smoke in SetSmokePos (%d,%d)\n", i, pSmoke->nObject);
		pSmoke->nObject = -1;
		}
#endif
	}
}

//------------------------------------------------------------------------------

void SetSmokeDensity (int i, int nMaxParts, int nDensity)
{
nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.smoke.nDens [0]);
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.smoke + i;
	if (pSmoke->pClouds)
		for (i = 0; i < pSmoke->nClouds; i++)
			SetCloudDensity (pSmoke->pClouds + i, nMaxParts, nDensity);
	}
}

//------------------------------------------------------------------------------

void SetSmokePartScale (int i, float nPartScale)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.smoke + i;
	if (pSmoke->pClouds)
		for (i = 0; i < pSmoke->nClouds; i++)
			SetCloudPartScale (pSmoke->pClouds + i, nPartScale);
	}
}

//------------------------------------------------------------------------------

void SetSmokeLife (int i, int nLife)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.smoke + i;
	if (pSmoke->pClouds && (pSmoke->pClouds->nLife != nLife)) {
		//LogErr ("SetSmokeLife (%d,%d) = %d\n", i, pSmoke->nObject, nLife);
		for (i = 0; i < pSmoke->nClouds; i++)
			SetCloudLife (pSmoke->pClouds, nLife);
		}
	}
}

//------------------------------------------------------------------------------

void SetSmokeType (int i, int nType)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.smoke + i;
	pSmoke->nType = nType;
	for (i = 0; i < pSmoke->nClouds; i++)
		SetCloudType (pSmoke->pClouds + i, nType);
	}
}

//------------------------------------------------------------------------------

void SetSmokeDir (int i, vmsVector *pDir)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.smoke + i;
	for (i = 0; i < pSmoke->nClouds; i++)
		SetCloudDir (pSmoke->pClouds + i, pDir);
	}
}

//------------------------------------------------------------------------------

int GetSmokeType (int i)
{
return (IsUsedSmoke (i)) ? gameData.smoke.smoke [i].nType : -1;
}

//------------------------------------------------------------------------------

tCloud *GetCloud (int i, int j)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = gameData.smoke.smoke + i;
	return (pSmoke->pClouds && (j < pSmoke->nClouds)) ? pSmoke->pClouds + j : NULL;
	}
else
	return NULL;
}

//------------------------------------------------------------------------------

int MaxParticles (int nParts, int nDens)
{
return (nParts < 0) ? -nParts : nParts * (nDens + 1); //(int) (nParts * pow (1.2, nDens));
}

//------------------------------------------------------------------------------

float ParticleSize (int nSize, float nScale)
{
if (gameOpts->render.smoke.bDisperse)
	return (float) (PARTICLE_RAD * (nSize + 1)) / nScale + 0.5f;
else
	return ((float) (PARTICLE_RAD << nSize)) / nScale + 0.5f;
}

//------------------------------------------------------------------------------
//eof
