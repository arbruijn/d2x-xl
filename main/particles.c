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

#define MAX_SMOKE 1000

#define PARTICLE_FPS	30

#define PARTICLE_RAD	(F1_0)

tSmoke	smoke [MAX_SMOKE];

static int iFreeSmoke = -1;
static int iUsedSmoke = -1;
static int bHavePartImg [2][3] = {{0, 0, 0},{0, 0, 0}};

static grs_bitmap *bmpParticle [2][3] = {{NULL, NULL, NULL},{NULL, NULL, NULL}};
static grs_bitmap *bmpBumpMaps [2] = {NULL, NULL};

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
static GLdouble colorBuffer [VERT_BUF_SIZE][4];
static int iBuffer = 0;
#else
static int iBuffer = 0;
static int nBuffer = 0;
#endif

#define SMOKE_START_ALPHA		191 //128 //191

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
	smoke [i].nNext = j;
smoke [i].nNext = -1;
iFreeSmoke = 0;
iUsedSmoke = -1;
}

//	-----------------------------------------------------------------------------

grs_bitmap *ReadParticleImage (char *szFile)
{
	grs_bitmap	*bmP = NULL;

if (!(bmP = GrCreateBitmap (0, 0, 0)))
	return NULL;
if (ReadTGA (szFile, NULL, bmP, -1, 1.0, 0, 0)) {
	bmP->bm_type = BM_TYPE_ALT;
	return bmP;
	}
bmP->bm_type = BM_TYPE_ALT;
GrFreeBitmap (bmP);
return NULL;
}

//	-----------------------------------------------------------------------------

int LoadParticleImage (int nType)
{
	int			*flagP = bHavePartImg [gameStates.render.bPointSprites] + nType;
	grs_bitmap	*bmP = NULL;

if (*flagP < 0)
	return 0;
if (*flagP > 0)
	return 1;
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
			d_free (BM_FRAMES (bmpParticle [i][j]));
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

int CreateParticle (tParticle *pParticle, vms_vector *pPos, short nSegment, int nLife, 
						  int nSpeed, int nType, float nScale, int nCurTime, int bStart)
{
	vms_vector	dir;
	int			nRad = (int) ((float) (PARTICLE_RAD >> (3 - gameOpts->render.smoke.nSize)) / nScale + 0.5);

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
pParticle->glColor.r =
pParticle->glColor.g =
pParticle->glColor.b = 1.0;//(double) (64 + randN (64)) / 255.0;
pParticle->glColor.a = (double) (SMOKE_START_ALPHA + randN (64)) / 255.0;
#	if 0
pParticle->glColor.r =
pParticle->glColor.g =
pParticle->glColor.b = pParticle->glColor.a - 0.5;
#	endif
#endif
//nSpeed = (int) (sqrt (nSpeed) * (double) F1_0);
nSpeed = nSpeed * F1_0;
dir.x = nSpeed - randN (2 * nSpeed);
dir.y = nSpeed - randN (2 * nSpeed);
dir.z = nSpeed - randN (2 * nSpeed);
pParticle->dir = dir;
#if 0
pParticle->pos.x = pPosegP->x + 500 - randN (1000);
pParticle->pos.y = pPosegP->y + 500 - randN (1000);
pParticle->pos.z = pPosegP->z + 500 - randN (1000);
#else
VmVecScaleAdd (&pParticle->pos, pPos, &dir, 200);
#endif
#if 0
if (nLife < 0) {
	pParticle->nTTL = -1;
	pParticle->nLife = 1;
	}
else
#else
if (nLife < 0)
	nLife = -nLife;
#endif
	pParticle->nLife = 
	pParticle->nTTL = nLife / 2 + randN (nLife / 2);
pParticle->nOrient = randN (4);
pParticle->nMoved = nCurTime;
pParticle->nDelay = 0; //bStart ? randN (nLife) : 0;
pParticle->nWidth = //nRad + randN (nRad);
pParticle->nHeight = nRad + randN (nRad);
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
static vms_vector	*wallNorm;

int CollideParticleAndWall (tParticle *pParticle)
{
	segment		*segP;
	side			*sideP;
	int			bInit, bRedo = 0, nSide, nVert, nChild, nFace, nInFront,
					nRad = max (pParticle->nWidth, pParticle->nHeight) / 2;
	fix			nDist;
	int			*vlP;
	vms_vector	pos = pParticle->pos;

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
	vms_vector	pos, dir;

if (pParticle->nLife <= 0)
	return 0;
if ((1000 / PARTICLE_FPS) <= (t = nCurTime - pParticle->nMoved)) {
	if (pParticle->nDelay > 0)
		pParticle->nDelay -= t;
	else {
		pos = pParticle->pos;
		dir = pParticle->dir;
		dir.x = ChangeDir (dir.x);
		dir.y = ChangeDir (dir.y);
		dir.z = ChangeDir (dir.z);
		for (j = 0; j < 2; j++) {
			VmVecScaleAdd (&pParticle->pos, &pos, &dir, t);
			if (gameOpts->render.smoke.bCollisions && CollideParticleAndWall (pParticle)) {	//reflect the particle
				if (j)
					return 0;
				else if (!(dot = VmVecDot (&dir, wallNorm)))
					return 0;
				else {
					VmVecScaleAdd (&dir, &pParticle->dir, wallNorm, -2 * dot);
					//VmVecScaleAdd (&pParticle->pos, &pos, &dir, 2 * t);
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
		pParticle->dir = dir;
		if (pParticle->nTTL >= 0) {
#if 0
			//VmVecScaleFrac (&pParticle->dir, t - ((t / 10) ? t / 10 : 1), t);
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

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);
#if !EXTRA_VERTEX_ARRAYS
	glColorPointer (4, GL_DOUBLE, sizeof (tParticle) - sizeof (tPartColor), &pParticles->glColor);
	glVertexPointer (3, GL_DOUBLE, sizeof (tParticle) - sizeof (tPartPos), &pParticles->glPos);
	if (i = glGetError ()) {
#ifdef _DEBUG
		HUDMessage (0, "glVertexPointer %d failed (%d)", iBuffer, i);
#endif
		glVertexPointer (3, GL_DOUBLE, sizeof (tParticle) - sizeof (tPartPos), &pParticles->glPos);
		if (i = glGetError ())
			glVertexPointer (3, GL_DOUBLE, sizeof (tParticle) - sizeof (tPartPos), &pParticles->glPos);
		}
#else
	glVertexPointer (3, GL_DOUBLE, 0, vertexBuffer);
	if (i = glGetError ()) {
#ifdef _DEBUG
		HUDMessage (0, "glVertexPointer %d failed (%d)", iBuffer, i);
#endif
		glVertexPointer (3, GL_DOUBLE, 0, vertexBuffer);
		if (i = glGetError ())
			glVertexPointer (3, GL_DOUBLE, 0, vertexBuffer);
		}
	glColorPointer (4, GL_DOUBLE, 0, colorBuffer);
#endif
	i = glGetError ();
	glDrawArrays (gameStates.render.bPointSprites ? GL_POINTS : GL_QUADS, 0, iBuffer);
	i = glGetError ();
	glDisableClientState (GL_VERTEX_ARRAY);
	glDisableClientState (GL_COLOR_ARRAY);
	iBuffer = 0;
	}
}

//------------------------------------------------------------------------------

inline void SetParticleTexCoord (GLdouble u, GLdouble v, char orient)
{
if (orient == 1)
	glMultiTexCoord2d (GL_TEXTURE0_ARB, 1.0 - v, u);
else if (orient == 2)
	glMultiTexCoord2d (GL_TEXTURE0_ARB, 1.0 - u, 1.0 - v);
else if (orient == 3)
	glMultiTexCoord2d (GL_TEXTURE0_ARB, v, 1.0 - u);
else 
	glMultiTexCoord2d (GL_TEXTURE0_ARB, u, v);
}

//------------------------------------------------------------------------------

int RenderParticle (tParticle *pParticle)
{
	vms_vector			hp;
	GLdouble				u, v, x, y, z, h, w;
	char					o;
	grs_bitmap			*bmP;
	tPartColor			pc;
#if OGL_VERTEX_ARRAYS
	double				*pf;
#endif
	double				decay = (double) pParticle->nLife / (double) pParticle->nTTL;

if (pParticle->nDelay > 0)
	return 0;
bmP = bmpParticle [gameStates.render.bPointSprites][pParticle->nType];
if (BM_CURFRAME (bmP))
	bmP = BM_CURFRAME (bmP);
if (gameOpts->render.smoke.bSort)
	hp = pParticle->transPos;
else
	G3TransformPoint (&hp, &pParticle->pos);
pc = pParticle->glColor;
if (gameOpts->ogl.bUseLighting) {
	tFaceColor	*psc = AvgSgmColor (pParticle->nSegment, NULL);
	if (psc->index == gameStates.render.nFrameFlipFlop) {
		pc.r *= (double) psc->color.red;
		pc.g *= (double) psc->color.green;
		pc.b *= (double) psc->color.blue;
		}
	}
glColor4d (pc.r, pc.g, pc.b, pc.a * decay);
#if OGL_POINT_SPRITES
if (gameStates.render.bPointSprites) {
#if OGL_VERTEX_ARRAYS
	if (gameStates.render.bVertexArrays) {
#if !EXTRA_VERTEX_ARRAYS
		pParticle->glPos.x = f2fl (hp.x);
		pParticle->glPos.y = f2fl (hp.y);
		pParticle->glPos.z = -f2fl (hp.z);
		nBuffer++;
#else
		colorBuffer [iBuffer][3] = (double) pParticle->glColor.a * decay;
		pf = vertexBuffer [iBuffer];
		*pf++ = f2fl (hp.x);
		*pf++ = f2fl (hp.y);
		*pf++ = -f2fl (hp.z);
		if (++iBuffer >= VERT_BUF_SIZE)
			FlushVertexArrays ();
#endif
		}
	else
#endif
	{
//	glColor4d (1.0, 1.0, 1.0, pParticle->glColor.a);
	glVertex3d (f2fl (hp.x), f2fl (hp.y), -f2fl (hp.z));
	}
	}
else
#endif
	{
	w = f2fl (FixMul (pParticle->nWidth, viewInfo.scale.x)) * decay;
	h = f2fl (FixMul (pParticle->nHeight, viewInfo.scale.y)) * decay;
	x = f2fl (hp.x);
	y = f2fl (hp.y);
	z = -f2fl (hp.z);
#if OGL_VERTEX_ARRAYS
	if (gameStates.render.bVertexArrays) {
		colorBuffer [iBuffer][3] = (double) pParticle->glColor.a * decay;
		pf = vertexBuffer [iBuffer];
		*pf++ = x - w; *pf++ = y + h; *pf++ = z;
		*pf++ = x + w; *pf++ = y + h; *pf++ = z;
		*pf++ = x + w; *pf++ = y - h; *pf++ = z;
		*pf   = x - w; *pf++ = y + h; *pf++ = z;
		iBuffer += 4;
		if (iBuffer >= VERT_BUF_SIZE)
			FlushVertexArrays ();
		}
	else
#endif
	{
	u = bmP->glTexture->u;
	v = bmP->glTexture->v;
	o = pParticle->nOrient;
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

int BeginRenderSmoke (int nType)
{
	grs_bitmap	*bmP = bmpParticle [gameStates.render.bPointSprites][nType];
	int					nFrames = nParticleFrames [gameStates.render.bPointSprites][nType];

if (nFrames > 1) {
	static time_t	t0 = 0;
	time_t			t = gameStates.app.nSDLTicks;
	int				iFrames = iParticleFrames [gameStates.render.bPointSprites][nType];
;

	BM_CURFRAME (bmP) = BM_FRAMES (bmP) + iFrames;
	if (t - t0 > 50) {
		t0 = t;
		iParticleFrames [gameStates.render.bPointSprites][nType] = (iFrames + 1) % nFrames;
		}
	}
OglActiveTexture (GL_TEXTURE0_ARB);
glEnable (GL_TEXTURE_2D);
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//glDisable (GL_DEPTH_TEST);
#if OGL_POINT_SPRITES
if (gameStates.render.bPointSprites) {
	float quadratic [] =  {1.0f, 0.0f, 0.01f};
   float maxSize = 0.0f;
	glPointParameterfvARB (GL_POINT_DISTANCE_ATTENUATION_ARB, quadratic);
	glPointParameterfARB (GL_POINT_FADE_THRESHOLD_SIZE_ARB, 60.0f);
	glTexEnvf (GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable (GL_POINT_SPRITE_ARB);
   glGetFloatv (GL_POINT_SIZE_MAX_ARB, &maxSize);
	if (maxSize > (float) (64 >> (3 - gameOpts->render.smoke.nSize)))
		maxSize = (float) (64 >> (3 - gameOpts->render.smoke.nSize));
   glPointSize (maxSize);
	glPointParameterfARB (GL_POINT_SIZE_MIN_ARB, 1.0f);
	glPointParameterfARB (GL_POINT_SIZE_MAX_ARB, maxSize);
	if (OglBindBmTex (bmP, 1))
		return 0;
#if OGL_VERTEX_ARRAYS
	if (!gameStates.render.bVertexArrays)
#endif
		glBegin (GL_POINTS);
	}
else
#endif
	{
	if (OglBindBmTex (bmP, 1))
		return 0;
#if OGL_VERTEX_ARRAYS
	if (!gameStates.render.bVertexArrays)
#endif
		glBegin (GL_QUADS);
	}
return 1;
}

//------------------------------------------------------------------------------

int EndRenderSmoke (tCloud *pCloud)
{
#if OGL_VERTEX_ARRAYS
if (gameStates.render.bVertexArrays)
#	if EXTRA_VERTEX_ARRAYS
	FlushVertexArrays ();
#else
	FlushVertexArrays (pCloud->pParticles + iBuffer, nBuffer - iBuffer);
#endif
else
	glEnd ();
#else
glEnd ();
#endif
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if OGL_POINT_SPRITES
if (gameStates.render.bPointSprites) {
	glDisable (GL_POINT_SPRITE_ARB);
	}
#endif
//glEnable (GL_DEPTH_TEST);
glDisable (GL_TEXTURE_2D);
return 1;
}

//------------------------------------------------------------------------------

int CreateCloud (tCloud *pCloud, vms_vector *pPos, short nSegment, int nMaxParts, float nPartScale, int nLife, int nSpeed, int nType, int nCurTime)
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
pCloud->prevPos =
pCloud->pos = *pPos;
pCloud->bHavePrevPos = 1;
pCloud->nParts = 0;
pCloud->nMoved = nCurTime;
pCloud->nPartLimit =
pCloud->nMaxParts = nMaxParts;
pCloud->nPartScale = nPartScale;
pCloud->nSegment = nSegment;
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
	int			t, i, j = c.nParts, r = 0,
					nInterpolatePos = 0;
	vms_vector	d, p;

if ((t = nCurTime - c.nMoved) < (1000 / PARTICLE_FPS))
	return 0;
nPartSeg = -1;
for (i = 0; i < j; )
	if (MoveParticle (c.pParticles + i, nCurTime))
		i++;
	else if (i < --j)
		c.pParticles [i] = c.pParticles [j];
if (CloudLives (pCloud, nCurTime)) {
	int h = c.nMaxParts / abs (c.nLife) * t;
	if (!h)
		h = 1;
	if (h > c.nMaxParts - i)
		h = c.nMaxParts - i;
	//LogErr ("   creating %d particles\n", h);
	if (!h)
		return c.nMaxParts;
	else if (h == 1)
		h = 1;
#ifdef _DEBUG
	if (c.prevPos.x == c.pos.x && c.prevPos.y == c.pos.y && c.prevPos.z == c.pos.z)
		c.bHavePrevPos = 0;
#endif
	if (c.bHavePrevPos && (h > 1)) {
		nInterpolatePos = 2;
		p = c.prevPos;
		VmVecSub (&d, &c.pos, &p);
		r = abs (d.x);
		if (d.y && (!r || (r > abs (d.y))))
			r = abs (d.y);
		if (d.z && (!r || (r > abs (d.z))))
			r = abs (d.z);
		if (!r)
			nInterpolatePos = 0;
		else {
			if (r > h)
				r = h;
#if 1
			d.x /= r;
			d.y /= r;
			d.z /= r;
#endif
			r = h / r;
			}
		}
	if (!nInterpolatePos) {
		p = c.pos;
		d.x = d.y = d.z = 0;
		}
	for (j = 0; h; i++, h--, j--) {
		if (!j) {
			j = r;
			if (h > r)
				VmVecInc (&p, &d);
			else
				p = c.pos;
			}
		CreateParticle (c.pParticles + i, &p, c.nSegment, c.nLife, 
							 c.nSpeed, c.nType, c.nPartScale, nCurTime, nInterpolatePos);
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
	G3TransformPoint (&pParticles->transPos, &pParticles->pos);
}

//------------------------------------------------------------------------------

int SortParticles (tParticle *pParticles, tPartIdx *pPartIdx, int nParticles)
{
if (nParticles > 1) {
	int	i, z, nSortedUp = 1, nSortedDown = 1;

	TransformParticles (pParticles, nParticles);
	for (i = 0; i < nParticles; i++) {
		pPartIdx [i].i = i;
		pPartIdx [i].z = pParticles [i].transPos.z;
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

if (!BeginRenderSmoke (pCloud->nType))
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

void SetCloudPos (tCloud *pCloud, vms_vector *pos)
{
int nNewSeg = FindSegByPoint (pos, pCloud->nSegment);
#if 0
if ((pCloud->prevPos.x != pCloud->pos.x) ||
	 (pCloud->prevPos.y != pCloud->pos.y) ||
	 (pCloud->prevPos.z != pCloud->pos.z)) {
	pCloud->prevPos = pCloud->pos;
	pCloud->bHavePrevPos = 1;
	}
#endif
pCloud->pos = *pos;
if (nNewSeg >= 0)
	pCloud->nSegment = nNewSeg;
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

int SetCloudDensity (tCloud *pCloud, int nMaxParts)
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

for (i = iUsedSmoke; i >= 0; i = smoke [i].nNext)
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
pSmoke = smoke + iSmoke;
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

for (i = iUsedSmoke; i >= 0; i = j)
	if ((j = smoke [i].nNext) == iSmoke)
		return smoke + i;
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
pSmoke = smoke + iSmoke;
if (pSmoke->pClouds) {
	for (i = pSmoke->nClouds; i; )
		DestroyCloud (pSmoke->pClouds + --i);
	d_free (pSmoke->pClouds);
	pSmoke->pClouds = NULL;
	i = pSmoke->nNext;
	if (iUsedSmoke == iSmoke)
		iUsedSmoke = i;
	pSmoke->nNext = iFreeSmoke;
	if (pSmoke = PrevSmoke (iSmoke))
		pSmoke->nNext = i;
	iFreeSmoke = iSmoke;
	}
return iSmoke;
}

//------------------------------------------------------------------------------

int DestroyAllSmoke (void)
{
	int	i, j;

for (i = iUsedSmoke; i >= 0; i = j) {
	j = smoke [i].nNext;
	DestroySmoke (-i - 1);
	}
FreeParticleImages ();
InitSmoke ();
return 1;
}

//------------------------------------------------------------------------------

int CreateSmoke (vms_vector *pPos, short nSegment, int nMaxClouds, int nMaxParts, 
					  float nPartScale, int nLife, int nSpeed, int nType, int nObject)
{
if (!(EGI_FLAG (bUseSmoke, 0, 0)))
	return 0;
else if (iFreeSmoke < 0)
	return 0;
else if (!LoadParticleImage (nType)) {
	//LogErr ("cannot create smoke\n");
	return -1;
	}
else {
		int		i, t = gameStates.app.nSDLTicks;
		tSmoke	*pSmoke;

	nMaxParts <<= gameOpts->render.smoke.nScale;
	if (gameStates.render.bPointSprites)
		nMaxParts *= 2;
	srand (SDL_GetTicks ());
	pSmoke = smoke + iFreeSmoke;
	if (!(pSmoke->pClouds = (tCloud *) d_malloc (nMaxClouds * sizeof (tCloud)))) {
		//LogErr ("cannot create smoke\n");
		return 0;
		}
	pSmoke->nObject = nObject;
	pSmoke->nClouds = 0;
	pSmoke->nMaxClouds = nMaxClouds;
	for (i = 0; i < nMaxClouds; i++)
		if (CreateCloud (pSmoke->pClouds + i, pPos, nSegment, nMaxParts, nPartScale, nLife, nSpeed, nType, t))
			pSmoke->nClouds++;
		else {
			DestroySmoke (iFreeSmoke);
			//LogErr ("cannot create smoke\n");
			return -1;
			}
	pSmoke->nType = nType;
	i = iFreeSmoke;
	iFreeSmoke = pSmoke->nNext;
	pSmoke->nNext = iUsedSmoke;
	iUsedSmoke = i;
	//LogErr ("CreateSmoke (%d) = %d,%d (%d)\n", nObject, i, nMaxClouds, nType);
	return iUsedSmoke;
	}
}

//------------------------------------------------------------------------------

int MoveSmoke (void)
{
if (!(gameStates.app.b40fpsTick &&
	   EGI_FLAG (bUseSmoke, 0, 0)))
	return 0;
else {
		int		h, i, j = 0, t = gameStates.app.nSDLTicks;
		tSmoke	*pSmoke;
		tCloud	*pCloud;

	for (i = iUsedSmoke; i >= 0; i = pSmoke->nNext) {
		pSmoke = smoke + i;
		if (gameData.objs.objects [pSmoke->nObject].type == 255)
			i = i;
		if (pCloud = pSmoke->pClouds)
			for (h = j = 0; j < pSmoke->nClouds; ) {
				if (!pSmoke->pClouds)
					return 0;
				if (CloudIsDead (pCloud, t)) {
					if (!RemoveCloud (i, j)) {
						//LogErr ("killing smoke %d (%d)\n", i, pSmoke->nObject);
						DestroySmoke (i);
						return 0;
						}
					}
				else {
					//LogErr ("moving %d (%d)\n", i, pSmoke->nObject);
					if ((pSmoke->nObject < 0) || (gameData.objs.objects [pSmoke->nObject].type == 255))
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

int RenderSmoke (void)
{
if (!EGI_FLAG (bUseSmoke, 0, 0))
	return 0;
else {
		int		i, j;
		tSmoke	*pSmoke = smoke;

#if !EXTRA_VERTEX_ARRAYS
	nBuffer = 0;
#endif
	for (i = iUsedSmoke; i >= 0; i = pSmoke->nNext) {
		pSmoke = smoke + i;
		if (pSmoke->pClouds) {
			if (!LoadParticleImage (pSmoke->nType))
				return 0;
			if (gameData.smoke.objects [pSmoke->nObject] < 0)
				SetSmokeLife (i, 0);
			for (j = 0; j < pSmoke->nClouds; j++)
				RenderCloud (pSmoke->pClouds + j);
			}
		}
	return 1;
	}
}

//------------------------------------------------------------------------------

void SetSmokePos (int i, vms_vector *pos)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = smoke + i;
	if (pSmoke->pClouds)
		for (i = 0; i < pSmoke->nClouds; i++)
			SetCloudPos (pSmoke->pClouds, pos);
#ifdef _DEBUG
	else if (pSmoke->nObject >= 0) {
		HUDMessage (0, "no smoke in SetSmokePos (%d,%d)\n", i, pSmoke->nObject);
		//LogErr ("no smoke in SetSmokePos (%d,%d)\n", i, pSmoke->nObject);
		pSmoke->nObject = -1;
		}
#endif
	}
}

//------------------------------------------------------------------------------

void SetSmokeDensity (int i, int nMaxParts)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = smoke + i;
	if (pSmoke->pClouds)
		for (i = 0; i < pSmoke->nClouds; i++)
			SetCloudDensity (pSmoke->pClouds + i, nMaxParts << gameOpts->render.smoke.nScale);
	}
}

//------------------------------------------------------------------------------

void SetSmokePartScale (int i, float nPartScale)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = smoke + i;
	if (pSmoke->pClouds)
		for (i = 0; i < pSmoke->nClouds; i++)
			SetCloudPartScale (pSmoke->pClouds + i, nPartScale);
	}
}

//------------------------------------------------------------------------------

void SetSmokeLife (int i, int nLife)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = smoke + i;
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
	tSmoke *pSmoke = smoke + i;
	pSmoke->nType = nType;
	for (i = 0; i < pSmoke->nClouds; i++)
		SetCloudType (pSmoke->pClouds + i, nType);
	}
}

//------------------------------------------------------------------------------

int GetSmokeType (int i)
{
return (IsUsedSmoke (i)) ? smoke [i].nType : -1;
}

//------------------------------------------------------------------------------

tCloud *GetCloud (int i, int j)
{
if (IsUsedSmoke (i)) {
	tSmoke *pSmoke = smoke + i;
	return (pSmoke->pClouds && (j < pSmoke->nClouds)) ? pSmoke->pClouds + j : NULL;
	}
else
	return NULL;
}

//------------------------------------------------------------------------------
//eof
