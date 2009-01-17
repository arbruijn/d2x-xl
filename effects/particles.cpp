
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
#include "automap.h"

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

#define PARTICLE_TYPES	4

#define PARTICLE_FPS	30

#define PART_DEPTHBUFFER_SIZE 100000
#define PARTLIST_SIZE 1000000

static int bHavePartImg [2][PARTICLE_TYPES] = {{0,0,0,0},{0,0,0,0}};

static CBitmap *bmpParticle [2][PARTICLE_TYPES] = {{NULL, NULL, NULL, NULL},{NULL, NULL, NULL, NULL}};
#if 0
static CBitmap *bmpBumpMaps [2] = {NULL, NULL};
#endif

static const char *szParticleImg [2][PARTICLE_TYPES] = {
 {"smoke.tga", "bubble.tga", "bullcase.tga", "corona.tga"},
 {"smoke.tga", "bubble.tga", "bullcase.tga", "corona.tga"}
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
	CFloatVector3		vertex;
	} tParticleVertex;

static tParticleVertex particleBuffer [VERT_BUF_SIZE];
static float bufferBrightness = -1;
static char bBufferEmissive = 0;
static int iBuffer = 0;

#define SMOKE_START_ALPHA		(gameOpts->render.particles.bDisperse ? 96 : 128)

CParticleManager particleManager;
CParticleImageManager particleImageManager;

//------------------------------------------------------------------------------

void CParticleManager::RebuildSystemList (void)
{
#if 0
m_nUsed =
m_nFree = -1;
CParticleSystem *systemP = m_systems;
for (int i = 0; i < MAX_PARTICLE_SYSTEMS; i++, systemP++) {
	if (systemP->HasEmitters ()) {
		systemP->SetNext (m_nUsed);
		m_nUsed = i;
		}
	else {
		systemP->Destroy ();
		systemP->SetNext (m_nFree);
		m_nFree = i;
		}
	}
#endif
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

CFixVector *RandomPointOnQuad (CFixVector *quad, CFixVector *vPos)
{
	CFixVector	vOffs;
	int			i;

i = rand () % 2;
vOffs = quad [i + 1] - quad [i];
vOffs *= (2 * d_rand ());
vOffs += quad [i];
i += 2;
*vPos = quad [(i + 1) % 4] - quad [i];
*vPos *= (2 * d_rand ());
*vPos += quad [i];
*vPos -= vOffs;
*vPos *= (2 * d_rand ());
*vPos += vOffs;
return vPos;
}

//------------------------------------------------------------------------------

#define RANDOM_FADE	(0.95f + (float) rand () / (float) RAND_MAX / 20.0f)

int CParticle::Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
							  short nSegment, int nLife, int nSpeed, char nParticleSystemType, char nClass,
						     float nScale, tRgbaColorf *colorP, int nCurTime, int bBlowUp,
							  float fBrightness, CFixVector *vEmittingFace)
{

	static tRgbaColorf	defaultColor = {1,1,1,1};

	tRgbaColorf	color;
	CFixVector	vDrift;
	int			nRad, nFrames, nType = particleImageManager.GetType (nParticleSystemType);

if (nScale < 0)
	nRad = (int) -nScale;
else if (gameOpts->render.particles.bSyncSizes)
	nRad = (int) PARTICLE_SIZE (gameOpts->render.particles.nSize [0], nScale);
else
	nRad = (int) nScale;
if (!nRad)
	nRad = I2X (1);
m_nType = nType;
m_bEmissive = (nParticleSystemType == LIGHT_PARTICLES);
m_nClass = nClass;
m_nSegment = nSegment;
m_nBounce = 0;
color = colorP ? *colorP : defaultColor;
m_color [0] = 
m_color [1] = color;
if ((nType == BULLET_PARTICLES) || (nType == BUBBLE_PARTICLES)) {
	m_bBright = 0;
	m_nFade = -1;
	}
else {
	m_bBright = (nType == SMOKE_PARTICLES) ? (rand () % 50) == 0 : 0;
	if (colorP) {
		if (nType != LIGHT_PARTICLES) {
			m_color [0].red *= RANDOM_FADE;
			m_color [0].green *= RANDOM_FADE;
			m_color [0].blue *= RANDOM_FADE;
			}
		m_nFade = 0;
		}
	else {
		m_color [0].red = 1.0f;
		m_color [0].green = 0.5f;
		m_color [0].blue = 0.0f;
		m_nFade = 2;
		}
	if (m_bEmissive)
		m_color [0].alpha = (float) (SMOKE_START_ALPHA + 64) / 255.0f;
	else if (nParticleSystemType != GATLING_PARTICLES) {
		if (!colorP) 
			m_color [0].alpha = (float) (SMOKE_START_ALPHA + randN (64)) / 255.0f;
		else {
			if (colorP->alpha < 0)
				m_color [0].alpha = -colorP->alpha;
			else {
				if (2 == (m_nFade = (char) colorP->alpha)) {
					m_color [0].red = 1.0f;
					m_color [0].green = 0.5f;
					m_color [0].blue = 0.0f;
					}
				m_color [0].alpha = (float) (SMOKE_START_ALPHA + randN (64)) / 255.0f;
				}
			}
		}
#if 1
	if (gameOpts->render.particles.bDisperse && !m_bBright) {
		fBrightness = 1.0f - fBrightness;
		m_color [0].alpha += fBrightness * fBrightness / 8.0f;
		}
#endif
	}
//nSpeed = (int) (sqrt (nSpeed) * (float) I2X (1));
nSpeed *= I2X (1);
if (vDir) {
	CAngleVector	a;
	CFixMatrix	m;
	float			d;
	a [PA] = randN (I2X (1) / 4) - I2X (1) / 8;
	a [BA] = randN (I2X (1) / 4) - I2X (1) / 8;
	a [HA] = randN (I2X (1) / 4) - I2X (1) / 8;
	m = CFixMatrix::Create (a);
	vDrift = m * (*vDir);
	CFixVector::Normalize (vDrift);
	d = (float) CFixVector::DeltaAngle (vDrift, *vDir, NULL);
	if (d) {
		d = (float) exp ((I2X (1) / 8) / d);
		nSpeed = (fix) ((float) nSpeed / d);
		}
#if 0
	if (!colorP)	// hack for static particleSystem w/o user defined color
		m_color [0].green =
		m_color [0].blue = 1.0f;
#endif
	vDrift *= nSpeed;
	if ((nType == SMOKE_PARTICLES) || (nType == BUBBLE_PARTICLES))
		m_vDir = *vDir * (I2X (3) / 4 + I2X (randN (16)) / 64);
	else
		m_vDir = *vDir;
	m_bHaveDir = 1;
	}
else {
	CFixVector	vOffs;
	vDrift [X] = nSpeed - randN (2 * nSpeed);
	vDrift [Y] = nSpeed - randN (2 * nSpeed);
	vDrift [Z] = nSpeed - randN (2 * nSpeed);
	vOffs = vDrift;
	m_vDir.SetZero ();
	m_bHaveDir = 1;
	}
m_vDrift = vDrift;
if (vEmittingFace)
	m_vPos = *RandomPointOnQuad (vEmittingFace, vPos);
else if (nType != BUBBLE_PARTICLES)
	m_vPos = *vPos + vDrift * (I2X (1) / 64);
else {
	//m_vPos = *vPos + vDrift * (I2X (1) / 32);
	nSpeed = vDrift.Mag () / 16;
	vDrift = CFixVector::Avg ((*mOrient).RVec() * (nSpeed - randN (2 * nSpeed)), (*mOrient).UVec() * (nSpeed - randN (2 * nSpeed)));
	m_vPos = *vPos + vDrift + (*mOrient).FVec() * (I2X (1) / 2 - randN (I2X (1)));
#if 1
	m_vDrift.SetZero ();
#else
	CFixVector::Normalize (m_vDrift);
	m_vDrift *= I2X (32);
#endif
	}
if ((nType != BUBBLE_PARTICLES) && mOrient) {
		CAngleVector	vRot;
		CFixMatrix	mRot;

	vRot [BA] = 0;
	vRot [PA] = 2048 - ((d_rand () % 9) * 512);
	vRot [HA] = 2048 - ((d_rand () % 9) * 512);
	mRot = CFixMatrix::Create (vRot);
	m_mOrient = *mOrient * mRot;
	}
if (nLife < 0)
	nLife = -nLife;
if (nType == SMOKE_PARTICLES) {
	if (gameOpts->render.particles.bDisperse)
		nLife = (nLife * 2) / 3;
	nLife = nLife / 2 + randN (nLife / 2);
	}
m_nLife =
m_nTTL = nLife;
m_nMoved = nCurTime;
m_nDelay = 0; //bStart ? randN (nLife) : 0;
if (nType == SMOKE_PARTICLES)
	nRad += randN (nRad);
else if (nType == BUBBLE_PARTICLES)
	nRad = nRad / 10 + randN (9 * nRad / 10);
else
	nRad *= 2;
if ((m_bBlowUp = bBlowUp)) {
	m_nRad = nRad;
	m_nWidth =
	m_nHeight = nRad / 2;
	}
else {
	m_nWidth =
	m_nHeight = nRad;
	m_nRad = nRad / 2;
	}
nFrames = nParticleFrames [gameStates.render.bPointSprites && !gameOpts->render.particles.bSort && (gameOpts->render.bDepthSort <= 0)][nType];
if (nType == BULLET_PARTICLES) {
	m_nFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 3;
	}
else if (nType == BUBBLE_PARTICLES) {
	m_nFrame = rand () % (nFrames * nFrames);
	m_nRotFrame = 0;
	m_nOrient = 0;
	}
else if (nType == LIGHT_PARTICLES) {
	m_nFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 0;
	}
else {
	m_nFrame = rand () % (nFrames * nFrames);
	m_nRotFrame = m_nFrame / 2;
	m_nOrient = rand () % 4;
	}
#if 1
if (m_bEmissive)
	m_color [0].alpha *= ParticleBrightness (colorP);
else if (nParticleSystemType == SMOKE_PARTICLES)
	m_color [0].alpha /= colorP ? color.red + color.green + color.blue + 2 : 2;
else if (nParticleSystemType == BUBBLE_PARTICLES)
	m_color [0].alpha /= 2;
else if (nParticleSystemType == LIGHT_PARTICLES)
	m_color [0].alpha /= 5;
#	if 0
else if (nParticleSystemType == GATLING_PARTICLES)
	;//m_color [0].alpha /= 6;
#	endif
#endif
return 1;
}

//------------------------------------------------------------------------------

inline bool CParticle::IsVisible (void)
{
#if 0
return gameData.render.mine.bVisible [m_nSegment] == gameData.render.mine.nVisible;
#else
if (gameData.render.mine.bVisible [m_nSegment] == gameData.render.mine.nVisible)
	return true;
short* childP = SEGMENTS [m_nSegment].m_children;
for (int i = 6; i; i--, childP++)
	if ((*childP >= 0) && (gameData.render.mine.bVisible [*childP] == gameData.render.mine.nVisible))
		return true;
#endif
return false;
}

//------------------------------------------------------------------------------

inline int CParticle::ChangeDir (int d)
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
static CFixVector	*wallNorm;

int CParticle::CollideWithWall (void)
{
	CSegment		*segP;
	CSide			*sideP;
	int			bInit, nSide, nChild, nFace, nInFront;
	fix			nDist;
	int			*vlP;

//redo:

segP = SEGMENTS + m_nSegment;
if ((bInit = (m_nSegment != nPartSeg)))
	nPartSeg = m_nSegment;
for (nSide = 0, sideP = segP->m_sides; nSide < 6; nSide++, sideP++) {
	vlP = vertexList [nSide];
	if (bInit) {
		bSidePokesOut [nSide] = !sideP->IsPlanar ();
		nFaces [nSide] = sideP->m_nFaces;
		}
	for (nFace = nInFront = 0; nFace < nFaces [nSide]; nFace++) {
		nDist = m_vPos.DistToPlane (sideP->m_normals [nFace], gameData.segs.vertices [sideP->m_nMinVertex [0]]);
		if (nDist > -PLANE_DIST_TOLERANCE)
			nInFront++;
		else
			wallNorm = sideP->m_normals + nFace;
		}
	if (!nInFront || (bSidePokesOut [nSide] && (nFaces [nSide] == 2) && (nInFront < 2))) {
		if (0 > (nChild = segP->m_children [nSide]))
			return 1;
		m_nSegment = nChild;
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

int CParticle::Update (int nCurTime)
{
	int			j, nRad;
	short			nSegment;
	fix			t, dot;
	CFixVector	vPos, drift;
	fix			drag = (m_nType == BUBBLE_PARTICLES) ? I2X (1) : F2X ((float) m_nLife / (float) m_nTTL);

if ((m_nLife <= 0) /*|| (m_color [0].alpha < 0.01f)*/)
	return 0;
t = nCurTime - m_nMoved;
if (m_nDelay > 0)
	m_nDelay -= t;
else {
	vPos = m_vPos;
	drift = m_vDrift;
	if ((m_nType == SMOKE_PARTICLES) /*|| (m_nType == BUBBLE_PARTICLES)*/) {
		drift [X] = ChangeDir (drift [X]);
		drift [Y] = ChangeDir (drift [Y]);
		drift [Z] = ChangeDir (drift [Z]);
		}
	for (j = 0; j < 2; j++) {
		if (t < 0)
			t = -t;
		m_vPos = vPos + drift * t; //(I2X (t) / 1000);
		if (m_bHaveDir) {
			CFixVector vi = drift, vj = m_vDir;
			CFixVector::Normalize (vi);
			CFixVector::Normalize (vj);
//				if (CFixVector::Dot (drift, m_vDir) < 0)
			if (CFixVector::Dot (vi, vj) < 0)
				drag = -drag;
//				VmVecScaleInc (&drift, &m_vDir, drag);
			m_vPos += m_vDir * drag;
			}
		if (m_nTTL - m_nLife > I2X (1) / 16) {
			nSegment = FindSegByPos (m_vPos, m_nSegment, 0, 0, 1);
			if (nSegment < 0) {
#if DBG
				if (m_nSegment == nDbgSeg)
					nSegment = FindSegByPos (m_vPos, m_nSegment, 1, 0, 1);
#endif
				nSegment = FindSegByPos (m_vPos, m_nSegment, 0, 1, 1);
				if (nSegment < 0)
					return 0;
				}
			if ((m_nType == BUBBLE_PARTICLES) && (SEGMENTS [nSegment].m_nType != SEGMENT_IS_WATER))
				return 0;
			m_nSegment = nSegment;
			}
		if (gameOpts->render.particles.bCollisions && CollideWithWall ()) {	//Reflect the particle
			if (m_nType == BUBBLE_PARTICLES)
				return 0;
			if (j)
				return 0;
			else if (!(dot = CFixVector::Dot (drift, *wallNorm)))
				return 0;
			else {
				drift = m_vDrift + *wallNorm * (-2 * dot);
				//VmVecScaleAdd (&m_vPos, &vPos, &drift, 2 * t);
				m_nBounce = 3;
				continue;
				}
			}
		else if (m_nBounce)
			m_nBounce--;
		else {
			break;
			}
		}
	m_vDrift = drift;
	if (m_nTTL >= 0) {
#if SMOKE_SLOWMO
		m_nLife -= (int) (t / gameStates.gameplay.slowmo [0].fSpeed);
#else
		m_nLife -= t;
#endif
		if ((m_nType == SMOKE_PARTICLES) && (nRad = m_nRad)) {
			if (m_bBlowUp) {
				if (m_nWidth >= nRad)
					m_nRad = 0;
				else {
					m_nWidth += nRad / 10 / m_bBlowUp;
					m_nHeight += nRad / 10 / m_bBlowUp;
					if (m_nWidth > nRad)
						m_nWidth = nRad;
					if (m_nHeight > nRad)
						m_nHeight = nRad;
					m_color [0].alpha *= (1.0f + 0.0725f / m_bBlowUp);
					if (m_color [0].alpha > 1)
						m_color [0].alpha = 1;
					}
				}
			else {
				if (m_nWidth <= nRad)
					m_nRad = 0;
				else {
					m_nRad += nRad / 5;
					m_color [0].alpha *= 1.0725f;
					if (m_color [0].alpha > 1)
						m_color [0].alpha = 1;
					}
				}
			}
		}
	}
m_nMoved = nCurTime;
return 1;
}

//------------------------------------------------------------------------------

#define PARTICLE_POSITIONS 64

int CParticle::Render (float brightness)
{
	CFixVector			hp;
	GLfloat				d, u, v;
	CBitmap			*bmP;
	tRgbaColorf			pc;
	tTexCoord2f			texCoord [4];
	tParticleVertex	*pb;
	CFloatVector				vOffset, vCenter;
	int					i, nFrame, nType = m_nType, bEmissive = m_bEmissive,
							bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.particles.bSort && (gameOpts->render.bDepthSort <= 0);
	float					decay = (nType == BUBBLE_PARTICLES) ? 1.0f : (float) m_nLife / (float) m_nTTL;

	static int			nFrames = 1;
	static float		deltaUV = 1.0f;
	static tSinCosf	sinCosPart [PARTICLE_POSITIONS];
	static int			bInitSinCos = 1;
	static CFloatMatrix		mRot;

if (m_nDelay > 0)
	return 0;
if (!(bmP = bmpParticle [0][nType]))
	return 0;
if (bmP->CurFrame ())
	bmP = bmP->CurFrame ();
if (gameOpts->render.bDepthSort > 0) {
	hp = m_vTransPos;
	if ((particleManager.LastType () != nType) || (brightness != bufferBrightness) || (bBufferEmissive != bEmissive)) {
		if (gameStates.render.bVertexArrays)
			particleManager.FlushBuffer (brightness);
		particleManager.SetLastType (nType);
		bBufferEmissive = bEmissive;
		glActiveTexture (GL_TEXTURE0);
		glClientActiveTexture (GL_TEXTURE0);
		if (bmP->Bind (0, 1))
			return 0;
		nFrames = nParticleFrames [0][nType];
		deltaUV = 1.0f / (float) nFrames;
		if (m_bEmissive)
			glBlendFunc (GL_ONE, GL_ONE);
		else
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	if (!gameStates.render.bVertexArrays)
		glBegin (GL_QUADS);
	}
else if (gameOpts->render.particles.bSort) {
	hp = m_vTransPos;
	if ((particleManager.LastType () != nType) || (brightness != bufferBrightness)) {
		if (gameStates.render.bVertexArrays)
			particleManager.FlushBuffer (brightness);
		else
			glEnd ();
		particleManager.SetLastType (nType);
		if (bmP->Bind (0, 1))
			return 0;
		nFrames = nParticleFrames [bPointSprites][nType];
		deltaUV = 1.0f / (float) nFrames;
		if (m_bEmissive)
			glBlendFunc (GL_ONE, GL_ONE);
		else
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (!gameStates.render.bVertexArrays)
			glBegin (GL_QUADS);
		}
	}
else
	transformation.Transform(hp, m_vPos, 0);
if (m_bBright)
	brightness = (float) sqrt (brightness);
if (nType == SMOKE_PARTICLES) {
	if (m_nFade > 0) {
		if (m_color [0].green < m_color [1].green) {
#if SMOKE_SLOWMO
			m_color [0].green += 1.0f / 20.0f / (float) gameStates.gameplay.slowmo [0].fSpeed;
#else
			m_color [0].green += 1.0f / 20.0f;
#endif
			if (m_color [0].green > m_color [1].green) {
				m_color [0].green = m_color [1].green;
				m_nFade--;
				}
			}
		if (m_color [0].blue < m_color [1].blue) {
#if SMOKE_SLOWMO
			m_color [0].blue += 1.0f / 10.0f / (float) gameStates.gameplay.slowmo [0].fSpeed;
#else
			m_color [0].blue += 1.0f / 10.0f;
#endif
			if (m_color [0].blue > m_color [1].blue) {
				m_color [0].blue = m_color [1].blue;
				m_nFade--;
				}
			}
		}
	else if (m_nFade == 0) {
		m_color [0].red = m_color [1].red * RANDOM_FADE;
		m_color [0].green = m_color [1].green * RANDOM_FADE;
		m_color [0].blue = m_color [1].blue * RANDOM_FADE;
		m_nFade = -1;
		}
	}
pc = m_color [0];
//pc.alpha *= /*gameOpts->render.particles.bDisperse ? decay2 :*/ decay;
if ((nType == SMOKE_PARTICLES) || (nType == BUBBLE_PARTICLES)) {
	char nFrame = ((nType == BUBBLE_PARTICLES) && !gameOpts->render.particles.bWobbleBubbles) ? 0 : m_nFrame;
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
		tFaceColor *psc = AvgSgmColor (m_nSegment, NULL);
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
vCenter.Assign (hp);
i = m_nOrient;
if (bEmissive) { //scale light trail particle color to reduce saturation
	pc.red /= 50.0f;
	pc.green /= 50.0f;
	pc.blue /= 50.0f;
	}
else if (m_nType != BUBBLE_PARTICLES) {
#if 0
	pc.alpha = (pc.alpha - 0.005f) * decay + 0.005f;
#	if 1
	pc.alpha = (float) pow (pc.alpha * pc.alpha, 1.0 / 3.0);
#	endif
#else
	float fFade = (float) cos ((double) sqr (1 - decay) * Pi) / 2 + 0.5f;
	pc.alpha *= fFade;
#endif
	pc.alpha *= alphaScale [gameOpts->render.particles.nAlpha [gameOpts->render.particles.bSyncSizes ? 0 : m_nClass]];
	}
if (pc.alpha < 1.0 / 255.0) {
	m_nLife = 0;
	return 0;
	}
#if 0
if (!lightManager.Headlights ().nLights && (pc.red + pc.green + pc.blue < 0.001)) {
	m_nLife = 0;
	return 0;
	}
#endif
if ((nType == SMOKE_PARTICLES) && gameOpts->render.particles.bDisperse) {
#if 0
	decay = (float) sqrt (decay);
#else
	decay = (float) pow (decay * decay * decay, 1.0f / 5.0f);
#endif
	vOffset [X] = X2F (m_nWidth) / decay;
	vOffset [Y] = X2F (m_nHeight) / decay;
	}
else {
	vOffset [X] = X2F (m_nWidth) * decay;
	vOffset [Y] = X2F (m_nHeight) * decay;
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
		vCenter [X] += (float) sin (m_nFrame / 4.0f * Pi) / (10 + rand () % 6);
	if (!nType && gameOpts->render.particles.bRotate) {
		if (bInitSinCos) {
			OglComputeSinCos (sizeofa (sinCosPart), sinCosPart);
			bInitSinCos = 0;
			mRot.RVec()[Z] =
			mRot.UVec()[Z] =
			mRot.FVec()[X] =
			mRot.FVec()[Y] = 0;
			mRot.FVec()[Z] = 1;
			}
		nFrame = (m_nOrient & 1) ? 63 - m_nRotFrame : m_nRotFrame;
		mRot.RVec()[X] =
		mRot.UVec()[Y] = sinCosPart [nFrame].fCos;
		mRot.UVec()[X] = sinCosPart [nFrame].fSin;
		mRot.RVec()[Y] = -mRot.UVec()[X];
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
		particleManager.FlushBuffer (brightness);
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
	glColor4fv (reinterpret_cast<GLfloat*> (&pc));
	glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoord + i));
	glVertex3f (vCenter[X] - vOffset[X], vCenter[Y] + vOffset[Y], vCenter[Z]);
	glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoord + (i + 1) % 4));
	glVertex3f (vCenter[X] + vOffset[X], vCenter[Y] + vOffset[Y], vCenter[Z]);
	glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoord + (i + 2) % 4));
	glVertex3f (vCenter[X] + vOffset[X], vCenter[Y] - vOffset[Y], vCenter[Z]);
	glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoord + (i + 3) % 4));
	glVertex3f (vCenter[X] - vOffset[X], vCenter[Y] - vOffset[Y], vCenter[Z]);
	}
if (particleManager.Animate ()) {
	m_nFrame = (m_nFrame + 1) % (nFrames * nFrames);
	if (!(nType || (m_nFrame & 1)))
		m_nRotFrame = (m_nRotFrame + 1) % 64;
	}
if (gameOpts->render.bDepthSort > 0)
	glEnd ();
return 1;
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//------------------------------------------------------------------------------

char CParticleEmitter::ObjectClass (int nObject)
{
if ((nObject >= 0) && (nObject < 0x70000000)) {
	CObject	*objP = OBJECTS + nObject;
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

inline int CParticleEmitter::MayBeVisible (void)
{
return (m_nSegment < 0) || SegmentMayBeVisible (m_nSegment, 5, -1); 
}

//------------------------------------------------------------------------------

float  CParticleEmitter::Brightness (void)
{
	CObject	*objP;

if (m_nObject >= 0x70000000)
	return 0.5f;
if (m_nType > 2)
	return 1.0f;
if (m_nObject < 0)
	return m_fBrightness;
if (m_nObjType == OBJ_EFFECT)
	return (float) m_nDefBrightness / 100.0f;
if (m_nObjType == OBJ_DEBRIS)
	return 0.5f;
if ((m_nObjType == OBJ_WEAPON) && (m_nObjId == PROXMINE_ID))
	return 0.2f;
objP = OBJECTS + m_nObject;
if ((objP->info.nType != m_nObjType) || (objP->info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED | OF_ARMAGEDDON)))
	return m_fBrightness;
return m_fBrightness = (float) ObjectDamage (objP) * 0.5f + 0.1f;
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

int CParticleEmitter::Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
										short nSegment, int nObject, int nMaxParts, float fScale,
										int nDensity, int nPartsPerPos, int nLife, int nSpeed, char nType,
										tRgbaColorf *colorP, int nCurTime, int bBlowUpParts, CFixVector *vEmittingFace)
{
if (!m_particles.Create (nMaxParts))
	return 0;
m_nLife = nLife;
m_nBirth = nCurTime;
m_nSpeed = nSpeed;
m_nType = nType;
if ((m_bHaveColor = (colorP != NULL)))
	m_color = *colorP;
if ((m_bHaveDir = (vDir != NULL)))
	m_vDir = *vDir;
m_vPrevPos =
m_vPos = *vPos;
if (mOrient)
	m_mOrient = *mOrient;
else
	m_mOrient = CFixMatrix::IDENTITY;
m_bHavePrevPos = 1;
m_bBlowUpParts = bBlowUpParts;
m_nParts = 0;
m_nMoved = nCurTime;
m_nPartLimit =
m_nMaxParts = nMaxParts;
m_nFirstPart = 0;
m_fScale = fScale;
m_nDensity = nDensity;
m_nPartsPerPos = nPartsPerPos;
m_nSegment = nSegment;
m_nObject = nObject;
if ((nObject >= 0) && (nObject < 0x70000000)) {
	m_nObjType = OBJECTS [nObject].info.nType;
	m_nObjId = OBJECTS [nObject].info.nId;
	}
m_nClass = ObjectClass (nObject);
m_fPartsPerTick = (float) nMaxParts / (float) abs (nLife);
m_nTicks = 0;
m_nDefBrightness = 0;
if ((m_bEmittingFace = (vEmittingFace != NULL)))
	memcpy (m_vEmittingFace, vEmittingFace, sizeof (m_vEmittingFace));
m_fBrightness = (nObject < 0) ? 0.5f :  CParticleEmitter::Brightness ();
return 1;
}

//------------------------------------------------------------------------------

int CParticleEmitter::Destroy (void)
{
m_particles.Destroy ();
m_nParts =
m_nMaxParts = 0;
return 1;
}

//------------------------------------------------------------------------------

#if 0

void CParticleEmitter::Check (void)
{
	int	i, j;

for (i = m_nParts, j = m_nFirstPart; i; i--, j = (j + 1) % m_nPartLimit)
	if (m_particles [j].nType < 0)
		j = j;
}

#endif

//------------------------------------------------------------------------------

int CParticleEmitter::Update (int nCurTime, int nThread)
{
if (!m_particles)
	return 0;
#if MT_PARTICLES
if ((nThread < 0) && RunEmitterThread (emitterP, nCurTime, rtUpdateParticles)) {
	return 0;
	}
else
#endif
 {
		int			t, h, i, j;
		float			fDist;
		float			fBrightness = Brightness ();
		CFixMatrix	mOrient = m_mOrient;
		CFixVector	vDelta, vPos, *vDir = (m_bHaveDir ? &m_vDir : NULL),
						*vEmittingFace = m_bEmittingFace ? m_vEmittingFace : NULL;
		CFloatVector		vDeltaf, vPosf;

#if SMOKE_SLOWMO
	t = (int) ((nCurTime - m_nMoved) / gameStates.gameplay.slowmo [0].fSpeed);
#else
	t = nCurTime - m_nMoved;
#endif
	nPartSeg = -1;
	for (i = m_nParts, j = m_nFirstPart; i; i--, j = (j + 1) % m_nPartLimit)
		if (!m_particles [j].Update (nCurTime)) {
			if (j != m_nFirstPart)
				m_particles [j] = m_particles [m_nFirstPart];
			m_nFirstPart = (m_nFirstPart + 1) % m_nPartLimit;
			m_nParts--;
			}
	m_nTicks += t;
	if ((m_nPartsPerPos = (int) (m_fPartsPerTick * m_nTicks)) >= 1) {
		if (m_nType == BUBBLE_PARTICLES) {
			if (rand () % 4)	// create some irregularity in bubble appearance
				goto funcExit;
			}
		m_nTicks = 0;
		if (IsAlive (nCurTime)) {
			vDelta = m_vPos - m_vPrevPos;
			fDist = X2F (vDelta.Mag());
			h = m_nPartsPerPos;
			if (h > m_nMaxParts - i)
				h = m_nMaxParts - i;
			if (h <= 0)
				goto funcExit;
			if (m_bHavePrevPos && (fDist > 0)) {
				vPosf.Assign (m_vPrevPos);
				vDeltaf.Assign (vDelta);
				vDeltaf [X] /= (float) h;
				vDeltaf [Y] /= (float) h;
				vDeltaf [Z] /= (float) h;
				}
			else {
#if 1
				vPosf.Assign (m_vPrevPos);
				vDeltaf.Assign (vDelta);
				vDeltaf [X] /= (float) h;
				vDeltaf [Y] /= (float) h;
				vDeltaf [Z] /= (float) h;
#else
				vPosf.Assign (m_vPos);
				vDeltaf [X] =
				vDeltaf [Y] =
				vDeltaf [Z] = 0.0f;
				h = 1;
#endif
				}
			m_nParts += h;
			for (; h; h--, j = (j + 1) % m_nPartLimit) {
				vPosf += vDeltaf;
				vPos.Assign (vPosf);
/*
				vPos[Y] = (fix) (vPosf [Y] * 65536.0f);
				vPos[Z] = (fix) (vPosf [Z] * 65536.0f);
*/
				m_particles [j].Create (&vPos, vDir, &mOrient, m_nSegment, m_nLife,
												m_nSpeed, m_nType, m_nClass, m_fScale, m_bHaveColor ? &m_color : NULL,
												nCurTime, m_bBlowUpParts, fBrightness, vEmittingFace);
			if ((m_nType == LIGHT_PARTICLES) || (m_nType == BULLET_PARTICLES))
				goto funcExit;
				}
			}
		}

funcExit:

	m_bHavePrevPos = 1;
	m_nMoved = nCurTime;
	m_vPrevPos = m_vPos;
	m_nTicks = m_nTicks;
	m_nFirstPart = m_nFirstPart;
	return m_nParts = m_nParts;
	}
}

//------------------------------------------------------------------------------

int CParticleEmitter::Render (int nThread)
{
if (!m_particles)
	return 0;
#if MT_PARTICLES
if (((gameOpts->render.bDepthSort > 0) && (nThread < 0)) && RunEmitterThread (emitterP, 0, rtRenderParticles)) {
	return 0;
	}
else
#endif
 {
		float			brightness = Brightness ();
		int			nParts = m_nParts, h, i, j,
						nFirstPart = m_nFirstPart,
						nPartLimit = m_nPartLimit;

	for (h = 0, i = nParts, j = nFirstPart; i; i--, j = (j + 1) % nPartLimit)
		if (m_particles [j].IsVisible () && TIAddParticle (m_particles + j, brightness, nThread))
			h++;
	return h;
	}
return 0;
}

//------------------------------------------------------------------------------

void CParticleEmitter::SetPos (CFixVector *vPos, CFixMatrix *mOrient, short nSegment)
{
if ((nSegment < 0) && gameOpts->render.particles.bCollisions)
	nSegment = FindSegByPos (*vPos, m_nSegment, 1, 0, 1);
m_vPos = *vPos;
if (mOrient)
	m_mOrient = *mOrient;
if (nSegment >= 0)
	m_nSegment = nSegment;
}

//------------------------------------------------------------------------------

inline void CParticleEmitter::SetDir (CFixVector *vDir)
{
if ((m_bHaveDir = (vDir != NULL)))
	m_vDir = *vDir;
}

//------------------------------------------------------------------------------

inline void CParticleEmitter::SetLife (int nLife)
{
m_nLife = nLife;
m_fPartsPerTick = nLife ? (float) m_nMaxParts / (float) abs (nLife) : 0.0f;
m_nTicks = 0;
}

//------------------------------------------------------------------------------

inline void CParticleEmitter::SetBrightness (int nBrightness)
{
m_nDefBrightness = nBrightness;
}

//------------------------------------------------------------------------------

inline void CParticleEmitter::SetSpeed (int nSpeed)
{
m_nSpeed = nSpeed;
}

//------------------------------------------------------------------------------

inline void CParticleEmitter::SetType (int nType)
{
m_nType = nType;
}

//------------------------------------------------------------------------------

int CParticleEmitter::SetDensity (int nMaxParts, int nDensity)
{
	CParticle	*pp;
	int			h;

if (m_nMaxParts == nMaxParts)
	return 1;
if (nMaxParts > m_nPartLimit) {
	if (!(pp = new CParticle [nMaxParts]))
		return 0;
	if (m_particles.Buffer ()) {
		if (m_nParts > nMaxParts)
			m_nParts = nMaxParts;
		h = m_nPartLimit - m_nFirstPart;
		if (h > m_nParts)
			h = m_nParts;
		memcpy (pp, m_particles + m_nFirstPart, h * sizeof (CParticle));
		if (h < m_nParts)
			memcpy (pp + h, m_particles.Buffer (), (m_nParts - h) * sizeof (CParticle));
		m_nFirstPart = 0;
		m_nPartLimit = nMaxParts;
		delete[] m_particles.Buffer ();
		}
	m_particles.SetBuffer (pp);
	}
m_nDensity = nDensity;
m_nMaxParts = nMaxParts;
#if 0
if (m_nParts > nMaxParts)
	m_nParts = nMaxParts;
#endif
m_fPartsPerTick = (float) m_nMaxParts / (float) abs (m_nLife);
return 1;
}

//------------------------------------------------------------------------------

void CParticleEmitter::SetScale (float fScale)
{
m_fScale = fScale;
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CParticleSystem::Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
									  short nSegment, int nMaxEmitters, int nMaxParts,
									  float fScale, int nDensity, int nPartsPerPos, int nLife, int nSpeed, char nType,
									  int nObject, tRgbaColorf *colorP, int bBlowUpParts, char nSide)
{
	int			i;
	CFixVector	vEmittingFace [4];

if (nSide >= 0)
	SEGMENTS [nSegment].GetCorners (nSide, vEmittingFace);
nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.particles.nDens [0]);
if (gameStates.render.bPointSprites)
	nMaxParts *= 2;
if (!m_emitters.Create (nMaxEmitters)) {
	//PrintLog ("cannot create m_systems\n");
	return 0;
	}
if ((m_nObject = nObject) < 0x70000000) {
	m_nSignature = OBJECTS [nObject].info.nSignature;
	m_nObjType = OBJECTS [nObject].info.nType;
	m_nObjId = OBJECTS [nObject].info.nId;
	}
m_nEmitters = 0;
m_nLife = nLife;
m_nSpeed = nSpeed;
m_nBirth = gameStates.app.nSDLTicks;
m_nMaxEmitters = nMaxEmitters;
for (i = 0; i < nMaxEmitters; i++)
	if (m_emitters [i].Create (vPos, vDir, mOrient, nSegment, nObject, nMaxParts, fScale, nDensity,
										nPartsPerPos, nLife, nSpeed, nType, colorP, gameStates.app.nSDLTicks, bBlowUpParts, (nSide < 0) ? NULL : vEmittingFace))
		m_nEmitters++;
	else {
		particleManager.Destroy (m_nId);
		//PrintLog ("cannot create particle systems\n");
		return -1;
		}
m_nType = nType;
return 1;
}

//	-----------------------------------------------------------------------------

void CParticleSystem::Init (int nId)
{
m_nId = nId;
m_nObject = -1;
m_nObjType = -1;
m_nObjId = -1;
m_nSignature = -1;
}

//------------------------------------------------------------------------------

void CParticleSystem::Destroy (void)
{
if (m_emitters.Buffer ()) {
	m_emitters.Destroy ();
	if ((m_nObject >= 0) && (m_nObject < 0x70000000))
		particleManager.SetObjectSystem (m_nObject, -1);
	m_nObject = -1;
	m_nObjType = -1;
	m_nObjId = -1;
	m_nSignature = -1;
	}
}

//------------------------------------------------------------------------------

int CParticleSystem::Render (void)
{
	int	h = 0;

if (m_emitters.Buffer ()) {
	if (!particleImageManager.Load (m_nType))
		return 0;
	if ((m_nObject >= 0) && (m_nObject < 0x70000000) && 
		 ((OBJECTS [m_nObject].info.nType == OBJ_NONE) || 
		  (OBJECTS [m_nObject].info.nSignature != m_nSignature) || 
		  (particleManager.GetObjectSystem (m_nObject) < 0)))
		SetLife (0);
	CParticleEmitter *emitterP = m_emitters.Buffer ();
	for (int i = m_nEmitters; i; i--, emitterP++) 
		h += emitterP->Render (-1);
	}
#if DBG
if (!h)
	return 0;
#endif
return h;
}

//------------------------------------------------------------------------------

void CParticleSystem::SetPos (CFixVector *vPos, CFixMatrix *mOrient, short nSegment)
{
if (m_emitters.Buffer ())
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetPos (vPos, mOrient, nSegment);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetDensity (int nMaxParts, int nDensity)
{
if (m_emitters.Buffer ()) {
	nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.particles.nDens [0]);
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetDensity (nMaxParts, nDensity);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetScale (float fScale)
{
if (m_emitters.Buffer ())
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetScale (fScale);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetLife (int nLife)
{
if (m_emitters.Buffer () && (m_nLife != nLife)) {
	m_nLife = nLife;
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetLife (nLife);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetBrightness (int nBrightness)
{
if (m_emitters.Buffer ())
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetBrightness (nBrightness);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetType (int nType)
{
if (m_emitters.Buffer () && (m_nType != nType)) {
	m_nType = nType;
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetType (nType);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetSpeed (int nSpeed)
{
if (m_emitters.Buffer () && (m_nSpeed != nSpeed)) {
	m_nSpeed = nSpeed;
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetSpeed (nSpeed);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetDir (CFixVector *vDir)
{
if (m_emitters.Buffer ())
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetDir (vDir);
}

//------------------------------------------------------------------------------

int CParticleSystem::RemoveEmitter (int i)
{
if (m_emitters.Buffer () && (i < m_nEmitters)) {
	m_emitters [i].Destroy ();
	if (i < --m_nEmitters)
		m_emitters [i] = m_emitters [m_nEmitters];
	}
return m_nEmitters;
}

//------------------------------------------------------------------------------

int CParticleSystem::Update (void)
{
	CParticleEmitter	*emitterP;
	int					i = 0;

if ((m_nObject == 0x7fffffff) && (m_nType < 3) &&
	 (gameStates.app.nSDLTicks - m_nBirth > (MAX_SHRAPNEL_LIFE / I2X (1)) * 1000))
	SetLife (0);
#if DBG
if ((m_nObject < 0x70000000) && (OBJECTS [m_nObject].info.nType == 255))
	i = i;
#endif
if ((emitterP = m_emitters.Buffer ())) {
	bool bKill = (m_nObject < 0) || ((m_nObject < 0x70000000) && 
					 ((OBJECTS [m_nObject].info.nSignature != m_nSignature) || (OBJECTS [m_nObject].info.nType == OBJ_NONE)));
	for (i = 0; i < m_nEmitters; ) {
		if (!m_emitters)
			return 0;
		if (emitterP->IsDead (gameStates.app.nSDLTicks)) {
			if (!RemoveEmitter (i)) {
				particleManager.Destroy (m_nId);
				break;
				}
			}
		else {
			if (bKill)
				emitterP->SetLife (0);
			emitterP->Update (gameStates.app.nSDLTicks, -1);
			emitterP++, i++;
			}
		}
	}
return i;
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

void CParticleManager::Init (void)
{
#if OGL_VERTEX_BUFFERS
	GLfloat	pf = colorBuffer;

for (i = 0; i < VERT_BUFFER_SIZE; i++, pf++) {
	*pf++ = 1.0f;
	*pf++ = 1.0f;
	*pf++ = 1.0f;
	}
#endif
if (!m_objectSystems.Buffer ()) {
	if (!m_objectSystems.Create (LEVEL_OBJECTS)) {
		Shutdown ();
		extraGameInfo [0].bUseParticles = 0;
		return;
		}
	m_objectSystems.Clear (0xff);
	}
if (!m_objExplTime.Buffer ()) {
	if (!m_objExplTime.Create (LEVEL_OBJECTS)) {
		Shutdown ();
		extraGameInfo [0].bUseParticles = 0;
		return;
		}
	m_objExplTime.Clear (0);
	}
if (!m_systems.Create (MAX_PARTICLE_SYSTEMS)) {
	Shutdown ();
	extraGameInfo [0].bUseParticles = 0;
	return;
	}
int i = 0;
for (CParticleSystem* systemP = m_systems.GetFirst (m_systems.FreeList ()); systemP; systemP = GetNext ())
	systemP->Init (i++);
}

//------------------------------------------------------------------------------

int CParticleManager::Destroy (int i)
{
m_systems [i].Destroy ();
m_systems.Push (i);
return i;
}

//------------------------------------------------------------------------------

int CParticleManager::Shutdown (void)
{
SEM_ENTER (SEM_SMOKE)

for (CParticleSystem* systemP = GetFirst (); systemP; systemP = GetNext ()) {
	systemP->Destroy ();
	m_systems.Push (systemP->Id ());
	}
particleImageManager.FreeAll ();
SEM_LEAVE (SEM_SMOKE)
return 1;
}

//	-----------------------------------------------------------------------------

int CParticleManager::Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
										short nSegment, int nMaxEmitters, int nMaxParts,
										float fScale, int nDensity, int nPartsPerPos, int nLife, int nSpeed, char nType,
										int nObject, tRgbaColorf *colorP, int bBlowUpParts, char nSide)
{
#if 0
if (!(EGI_FLAG (bUseParticleSystem, 0, 1, 0)))
	return 0;
else
#endif
if (!particleImageManager.Load (nType))
	return -1;
CParticleSystem *systemP = m_systems.Pop ();
if (!systemP)
	return -1;
int i = systemP->Create (vPos, vDir, mOrient, nSegment, nMaxEmitters, nMaxParts, fScale, nDensity, 
								 nPartsPerPos, nLife, nSpeed, nType, nObject, colorP, bBlowUpParts, nSide);
if (i < 1)
	return i;
return systemP->Id ();
}

//------------------------------------------------------------------------------

int CParticleManager::Update (void)
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
	int	h = 0;

for (CParticleSystem* systemP = GetFirst (); systemP; systemP = GetNext ())
	h += systemP->Update ();
return h;
}

//------------------------------------------------------------------------------

void CParticleManager::Render (void)
{
for (CParticleSystem* systemP = GetFirst (); systemP; systemP = GetNext ())
	systemP->Render ();
}

//------------------------------------------------------------------------------

int CParticleManager::InitBuffer (int bLightmaps)
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

void CParticleManager::FlushBuffer (float brightness)
{
if (bufferBrightness < 0)
	bufferBrightness = brightness;
if (iBuffer) {
	tRgbaColorf	color = {bufferBrightness, bufferBrightness, bufferBrightness, 1};
	int bLightmaps = lightmapManager.HaveLightmaps ();
	bufferBrightness = brightness;
	glEnable (GL_BLEND);
	//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc (GL_LEQUAL);
	glDepthMask (0);
	if (gameStates.ogl.bShadersOk) {
		if (InitBuffer (bLightmaps)) { //gameStates.render.bVertexArrays) {
#if 1
			CBitmap *bmP = bmpParticle [0][particleManager.LastType ()];
			if (!bmP)
				return;
			glActiveTexture (GL_TEXTURE0);
			glClientActiveTexture (GL_TEXTURE0);
			glEnable (GL_TEXTURE_2D);
			if (bmP->CurFrame ())
				bmP = bmP->CurFrame ();
			if (bmP->Bind (0, 1))
				return;
#endif
			if (lightManager.Headlights ().nLights && !(automap.m_bDisplay || particleManager.LastType ()))
				lightManager.Headlights ().SetupShader (1, 0, &color);
			else if ((gameOpts->render.effects.bSoftParticles & 4) && (particleManager.LastType () <= BUBBLE_PARTICLES))
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
			glTexCoord2fv (reinterpret_cast<GLfloat*> (&pb->texCoord));
			glColor4fv (reinterpret_cast<GLfloat*> (&pb->color));
			glVertex3fv (reinterpret_cast<GLfloat*> (&pb->vertex));
			}
		glEnd ();
		}
	iBuffer = 0;
	glEnable (GL_DEPTH_TEST);
	if ((gameStates.ogl.bShadersOk && !particleManager.LastType ()) && (gameStates.render.history.nShader != 999)) {
		glUseProgramObject (0);
		gameStates.render.history.nShader = -1;
		}
	}
}

//------------------------------------------------------------------------------

int CParticleManager::CloseBuffer (void)
{
if (!gameStates.render.bVertexArrays)
	return 0;
FlushBuffer (-1);
G3DisableClientStates (1, 1, 0, GL_TEXTURE0 + lightmapManager.HaveLightmaps ());
return 1;
}

//------------------------------------------------------------------------------

int CParticleManager::BeginRender (int nType, float nScale)
{
	CBitmap	*bmP;
	int			bLightmaps = lightmapManager.HaveLightmaps ();
	static time_t	t0 = 0;

if (gameOpts->render.bDepthSort <= 0) {
	nType = (nType % PARTICLE_TYPES);
	if ((nType >= 0) && !gameOpts->render.particles.bSort)
		particleImageManager.Animate (nType);
	bmP = bmpParticle [0][nType];
	particleManager.SetStencil (StencilOff ());
	InitBuffer (bLightmaps);
	glActiveTexture (GL_TEXTURE0);
	glClientActiveTexture (GL_TEXTURE0);
	glDisable (GL_CULL_FACE);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_TEXTURE_2D);
	if ((nType >= 0) && bmP->Bind (0, 1))
		return 0;
	glDepthFunc (GL_LESS);
	glDepthMask (0);
	iBuffer = 0;
	if (!gameStates.render.bVertexArrays)
		glBegin (GL_QUADS);
	}
particleManager.SetLastType (-1);
if (gameStates.app.nSDLTicks - t0 < 33)
	particleManager.m_bAnimate = 0;
else {
	t0 = gameStates.app.nSDLTicks;
	particleManager.m_bAnimate = 1;
	}
return 1;
}

//------------------------------------------------------------------------------

int CParticleManager::EndRender (void)
{
if (gameOpts->render.bDepthSort <= 0) {
	if (!CloseBuffer ())
		glEnd ();
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	glDepthMask (1);
	StencilOn (particleManager.Stencil ());
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
return 1;
}

//------------------------------------------------------------------------------

CParticleManager::~CParticleManager ()
{
Shutdown ();
particleImageManager.FreeAll ();
m_systems.Destroy ();
m_objectSystems.Destroy ();
m_objExplTime.Destroy ();
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
// 4: gatling projectile trail
// 3: air bubbles
// 2: bullet casings
// 1: light trails
// 0: particleSystem

int CParticleImageManager::GetType (int nType)
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

void CParticleImageManager::Animate (int nType)
{
	int	bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.particles.bSort,
			nFrames = nParticleFrames [bPointSprites][nType];

if (nFrames > 1) {
	static time_t t0 [PARTICLE_TYPES] = {0, 0, 0, 0};

	time_t	t = gameStates.app.nSDLTicks;
	int		iFrame = iParticleFrames [bPointSprites][nType];
#if 0
	int		iFrameIncr = iPartFrameIncr [bPointSprites][nType];
#endif
	int		bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.particles.bSort;
	CBitmap*	bmP = bmpParticle [bPointSprites][GetType (nType)];

	if (!bmP->Frames ())
		return;
	bmP->SetCurFrame (iFrame);
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

void CParticleImageManager::AdjustBrightness (CBitmap *bmP)
{
	CBitmap*	bmfP;
	int		i, j = bmP->FrameCount ();
	float*	fFrameBright, fAvgBright = 0, fMaxBright = 0;

if (j < 2)
	return;
if (!(fFrameBright = new float [j]))
	return;
for (i = 0, bmfP = bmP->Frames (); i < j; i++, bmfP++) {
	fAvgBright += (fFrameBright [i] = (float) TGABrightness (bmfP));
	if (fMaxBright < fFrameBright [i])
		fMaxBright = fFrameBright [i];
	}
fAvgBright /= j;
for (i = 0, bmfP = bmP->Frames (); i < j; i++, bmfP++) {
	TGAChangeBrightness (bmfP, 0, 1, 2 * (int) (255 * fFrameBright [i] * (fAvgBright - fFrameBright [i])), 0);
	}
delete[] fFrameBright;
}

//	-----------------------------------------------------------------------------

int CParticleImageManager::Load (int nType)
{
	int		h,
				bPointSprites = gameStates.render.bPointSprites && !gameOpts->render.particles.bSort,
				*flagP;
	CBitmap	*bmP = NULL;

nType = particleImageManager.GetType (nType);
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
bmP->SetFrameCount ();
bmP->SetupTexture (0, 3, 1);
if (nType == SMOKE_PARTICLES)
	h = 8;
else if (nType == BUBBLE_PARTICLES)
	h = 4;
else
	h = bmP->FrameCount ();
nParticleFrames [bPointSprites][nType] = h;
return *flagP > 0;
}

//	-----------------------------------------------------------------------------

int CParticleImageManager::LoadAll (void)
{
	int	i;

for (i = 0; i < PARTICLE_TYPES; i++) {
	if (!Load (i))
		return 0;
	Animate (i);
	}
return 1;
}

//	-----------------------------------------------------------------------------

void CParticleImageManager::FreeAll (void)
{
	int	i, j;

for (i = 0; i < 2; i++)
	for (j = 0; j < PARTICLE_TYPES; j++)
		if (bmpParticle [i][j]) {
			delete bmpParticle [i][j];
			bmpParticle [i][j] = NULL;
			bHavePartImg [i][j] = 0;
			}
}

//------------------------------------------------------------------------------
//eof
