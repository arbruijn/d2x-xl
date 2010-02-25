
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
#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "vecmat.h"
#include "hudmsgs.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "ogl_fastrender.h"
#include "piggy.h"
#include "globvars.h"
#include "gameseg.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "renderlib.h"
#include "rendermine.h"
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

#if DBG
#	define ENABLE_RENDER	1
#	define ENABLE_UPDATE	1
#	define ENABLE_FLUSH	1
#else
#	define ENABLE_RENDER	1
#	define ENABLE_UPDATE	1
#	define ENABLE_FLUSH	1
#endif

#define LAZY_RENDER_SETUP 1

#define MAKE_SMOKE_IMAGE 0

#define MT_PARTICLES	0

#define BLOWUP_PARTICLES 0		//blow particles up at "birth"

#define SMOKE_SLOWMO 0

#define SORT_CLOUDS 1

#define PARTICLE_FPS	30

#define PARTICLE_POSITIONS 64

#define PART_DEPTHBUFFER_SIZE 100000
#define PARTLIST_SIZE 1000000

static int bHavePartImg [2][PARTICLE_TYPES] = {{0,0,0,0,0,0},{0,0,0,0,0,0}};

static CBitmap *bmpParticle [2][PARTICLE_TYPES] = {{NULL, NULL, NULL, NULL, NULL, NULL},{NULL, NULL, NULL, NULL, NULL, NULL}};
#if 0
static CBitmap *bmpBumpMaps [2] = {NULL, NULL};
#endif

static const char *szParticleImg [2][PARTICLE_TYPES] = {
 {"smoke.tga", "bubble.tga", "fire.tga", "smoke.tga", "bullcase.tga", "corona.tga"},
 {"smoke.tga", "bubble.tga", "fire.tga", "smoke.tga", "bullcase.tga", "corona.tga"}
	};

static int nParticleFrames [2][PARTICLE_TYPES] = {{1,1,1,1,1,1},{1,1,1,1,1,1}};
static int iParticleFrames [2][PARTICLE_TYPES] = {{0,0,0,0,0,0},{0,0,0,0,0,0}};
#if 0
static int iPartFrameIncr  [2][PARTICLE_TYPES] = {{1,1,1,1},{1,1,1,1}};
static float alphaScale [5] = {5.0f / 5.0f, 4.0f / 5.0f, 3.0f / 5.0f, 2.0f / 5.0f, 1.0f / 5.0f};
#endif

#define PART_BUF_SIZE	10000
#define VERT_BUF_SIZE	(PART_BUF_SIZE * 4)

typedef struct tRenderParticle {
	CParticle*	particle;
	float			fBrightness;
	char			nFrame;
	char			nRotFrame;
} tRenderParticles;


static tRgbaColorf defaultParticleColor = {1.0f, 1.0f, 1.0f, 2.0f * 0.6f};
static tRenderParticle particleBuffer [PART_BUF_SIZE];
static tParticleVertex particleRenderBuffer [VERT_BUF_SIZE];
static float bufferBrightness = -1;
static char bBufferEmissive = 0;
static CFloatVector vRot [PARTICLE_POSITIONS];


#define SMOKE_START_ALPHA		(gameOpts->render.particles.bDisperse ? 64 : 96) //96 : 128)

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
						     float nScale, tRgbaColorf *colorP, int nCurTime, int bBlowUp, char nFadeType,
							  float fBrightness, CFixVector *vEmittingFace)
{

	tRgbaColorf	color;
	CFixVector	vDrift;
	int			nRad, nType = particleImageManager.GetType (nParticleSystemType);

m_bChecked = 0;
if (nScale < 0)
	nRad = (int) -nScale;
else if (gameOpts->render.particles.bSyncSizes)
	nRad = (int) PARTICLE_SIZE (gameOpts->render.particles.nSize [0], nScale);
else
	nRad = (int) nScale;
if (!nRad)
	nRad = I2X (1);
m_nType = nType;
m_bEmissive = (nParticleSystemType == LIGHT_PARTICLES) ? 1 : (nParticleSystemType == FIRE_PARTICLES) ? 2 : 0;
m_nClass = nClass;
m_nFadeType = nFadeType;
m_nSegment = nSegment;
m_nBounce = 0;
m_bReversed = 0;
m_decay = 1.0f;
m_nMoved = nCurTime;
if (nLife < 0)
	nLife = -nLife;
m_nLife =
m_nTTL = nLife;
m_nDelay = 0; //bStart ? randN (nLife) : 0;

m_color [0] =
m_color [1] = 
color = (colorP && (m_bEmissive < 2)) ? *colorP : defaultParticleColor;

if ((nType == BULLET_PARTICLES) || (nType == BUBBLE_PARTICLES)) {
	m_bBright = 0;
	m_nFadeState = -1;
	}
else {
	m_bBright = (nType == SMOKE_PARTICLES) ? (rand () % 50) == 0 : 0;
	if (colorP) {
		if (!m_bEmissive) {
			m_color [0].red *= RANDOM_FADE;
			m_color [0].green *= RANDOM_FADE;
			m_color [0].blue *= RANDOM_FADE;
			}
		m_nFadeState = 0;
		}
	else {
		m_color [0].red = 1.0f;
		m_color [0].green = 0.5f;
		m_color [0].blue = 0.0f;
		m_nFadeState = 2;
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
				if (2 == (m_nFadeState = (char) colorP->alpha)) {
					m_color [0].red = 1.0f;
					m_color [0].green = 0.5f;
					m_color [0].blue = 0.0f;
					}
				m_color [0].alpha = (float) (SMOKE_START_ALPHA + randN (64)) / 255.0f;
				}
			}
		if (gameOpts->render.particles.bDisperse && !m_bBright) {
			fBrightness = 1.0f - fBrightness;
			m_color [0].alpha += fBrightness * fBrightness / 8.0f;
			}
		}
	}

if (nType == FIRE_PARTICLES)
	nSpeed = int (sqrt (double (nSpeed)) * float (I2X (1)));
else
	nSpeed *= I2X (1);
if (!vDir /*|| (nType == FIRE_PARTICLES)*/) {
	CFixVector	vOffs;
	vDrift [X] = nSpeed - randN (2 * nSpeed);
	vDrift [Y] = nSpeed - randN (2 * nSpeed);
	vDrift [Z] = nSpeed - randN (2 * nSpeed);
	if (nType == FIRE_PARTICLES)
		vDrift *= nRad / 32;
	vOffs = vDrift;
	m_vDir.SetZero ();
	m_bHaveDir = 1;
	}
else {
	m_vDir = *vDir;
	if (nType == FIRE_PARTICLES)
		vDrift = m_vDir;
	else {
		CAngleVector	a;
		CFixMatrix		m;
		a [PA] = randN (I2X (1) / 4) - I2X (1) / 8;
		a [BA] = randN (I2X (1) / 4) - I2X (1) / 8;
		a [HA] = randN (I2X (1) / 4) - I2X (1) / 8;
		m = CFixMatrix::Create (a);
		if (nType == WATERFALL_PARTICLES)
			CFixVector::Normalize (m_vDir);
		vDrift = m * m_vDir;
		}
	CFixVector::Normalize (vDrift);
	if (nType == WATERFALL_PARTICLES) {
		fix dot = CFixVector::Dot (m_vDir, vDrift);
		if (dot < I2X (1) / 2)
			return 0;
		}
	float d = float (CFixVector::DeltaAngle (vDrift, m_vDir, NULL));
	if (d) {
		d = (float) exp ((I2X (1) / 8) / d);
		nSpeed = (fix) ((float) nSpeed / d);
		}
	vDrift *= nSpeed;
	if (nType <= FIRE_PARTICLES)
		m_vDir *= (I2X (3) / 4 + I2X (randN (16)) / 64);
#if DBG
	if (CFixVector::Dot (vDrift, m_vDir) < 0)
		d = 0;
#endif
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
	vDrift = CFixVector::Avg ((*mOrient).RVec () * (nSpeed - randN (2 * nSpeed)), (*mOrient).UVec () * (nSpeed - randN (2 * nSpeed)));
	m_vPos = *vPos + vDrift + (*mOrient).FVec () * (I2X (1) / 2 - randN (I2X (1)));
	m_vDrift.SetZero ();
	}

if ((nType != BUBBLE_PARTICLES) && mOrient) {
		CAngleVector	vRot;
		CFixMatrix		mRot;

	vRot [BA] = 0;
	vRot [PA] = 2048 - ((d_rand () % 9) * 512);
	vRot [HA] = 2048 - ((d_rand () % 9) * 512);
	mRot = CFixMatrix::Create (vRot);
	m_mOrient = *mOrient * mRot;
	}
if (nType == SMOKE_PARTICLES) {
	if (gameOpts->render.particles.bDisperse)
		nLife = (nLife * 2) / 3;
	nLife = nLife / 2 + randN (nLife / 2);
	nRad += randN (nRad);
	}
else if (nType == FIRE_PARTICLES) {
	nLife = nLife / 2 + randN (nLife / 2);
	nRad += randN (nRad);
	}
else if (nType == BUBBLE_PARTICLES)
	nRad = nRad / 10 + randN (9 * nRad / 10);
else
	nRad *= 2;
m_vStartPos = m_vPos;

if ((m_bBlowUp = bBlowUp)) {
	m_nRad = nRad / 2;
	m_nWidth = (nType == WATERFALL_PARTICLES) ? nRad / 3 : m_nRad;
	m_nHeight = m_nRad;
	m_nRad += m_nRad / bBlowUp;
	}
else {
	m_nWidth = (nType == WATERFALL_PARTICLES) ? nRad / 3 : nRad;
	m_nHeight = nRad;
	m_nRad = nRad / 2;
	}
m_nFrames = nParticleFrames [0][nType];
m_deltaUV = 1.0f / float (m_nFrames);
if (nType == BULLET_PARTICLES) {
	m_nFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 3;
	}
else if (nType == BUBBLE_PARTICLES) {
	m_nFrame = rand () % (m_nFrames * m_nFrames);
	m_nRotFrame = 0;
	m_nOrient = 0;
	}
else if ((nType == LIGHT_PARTICLES) /*|| (nType == WATERFALL_PARTICLES)*/) {
	m_nFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 0;
	}
else {
	m_nFrame = rand () % (m_nFrames * m_nFrames);
	m_nRotFrame = m_nFrame / 2;
	m_nOrient = rand () % 4;
	}
UpdateTexCoord ();
#if 1
if (m_bEmissive)
	m_color [0].alpha *= ParticleBrightness (colorP);
else if (nParticleSystemType == SMOKE_PARTICLES)
	m_color [0].alpha /= colorP ? color.red + color.green + color.blue + 2 : 2;
else if (nParticleSystemType == BUBBLE_PARTICLES)
	m_color [0].alpha /= 2;
else if ((nParticleSystemType == LIGHT_PARTICLES) || (nParticleSystemType == FIRE_PARTICLES))
	m_color [0].alpha = 1.0f;
#	if 0
else if (nParticleSystemType == GATLING_PARTICLES)
	;//m_color [0].alpha /= 6;
#	endif
#endif
SetupColor (fBrightness);
return 1;
}

//------------------------------------------------------------------------------

inline bool CParticle::IsVisible (void)
{
#if 0
return gameData.render.mine.bVisible [m_nSegment] == gameData.render.mine.nVisible;
#else
if ((m_nSegment < 0) || (m_nSegment >= gameData.segs.nSegments))
	return false;
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

static int nPartSeg [MAX_THREADS] = {-1, -1, -1, -1};
static int nFaceCount [MAX_THREADS][6];
static int bSidePokesOut [MAX_THREADS][6];
//static int nVert [6];
static CFixVector	*wallNorm;

int CParticle::CollideWithWall (int nThread)
{
	CSegment*	segP;
	CSide*		sideP;
	int			bInit, nSide, nChild, nFace, nFaces, nInFront;
	fix			nDist;

//redo:

segP = SEGMENTS + m_nSegment;
if ((bInit = (m_nSegment != nPartSeg [nThread])))
	nPartSeg [nThread] = m_nSegment;
for (nSide = 0, sideP = segP->m_sides; nSide < 6; nSide++, sideP++) {
	if (bInit) {
		bSidePokesOut [nThread][nSide] = !sideP->IsPlanar ();
		nFaceCount [nThread][nSide] = sideP->m_nFaces;
		}
	nFaces = nFaceCount [nThread][nSide];
	for (nFace = nInFront = 0; nFace < nFaces; nFace++) {
		nDist = m_vPos.DistToPlane (sideP->m_normals [nFace], gameData.segs.vertices [sideP->m_nMinVertex [0]]);
		if (nDist > -PLANE_DIST_TOLERANCE)
			nInFront++;
		else
			wallNorm = sideP->m_normals + nFace;
		}
	if (!nInFront || (bSidePokesOut [nThread][nSide] && (nFaces == 2) && (nInFront < 2))) {
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

void CParticle::UpdateTexCoord (void)
{
if ((m_nType <= WATERFALL_PARTICLES) && ((m_nType != BUBBLE_PARTICLES) || gameOpts->render.particles.bWobbleBubbles)) {
	m_texCoord.v.u = float (m_nFrame % m_nFrames) * m_deltaUV;
	m_texCoord.v.v = float (m_nFrame / m_nFrames) * m_deltaUV;
	}
}

//------------------------------------------------------------------------------

void CParticle::UpdateColor (float fBrightness)
{
if (m_nType == SMOKE_PARTICLES) {
	if (m_nFadeState > 0) {
		if (m_color [0].green < m_color [1].green) {
#if SMOKE_SLOWMO
			m_color [0].green += 1.0f / 20.0f / (float) gameStates.gameplay.slowmo [0].fSpeed;
#else
			m_color [0].green += 1.0f / 20.0f;
#endif
			if (m_color [0].green > m_color [1].green) {
				m_color [0].green = m_color [1].green;
				m_nFadeState--;
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
				m_nFadeState--;
				}
			}
		}
	else if (m_nFadeState == 0) {
		m_color [0].red = m_color [1].red * RANDOM_FADE;
		m_color [0].green = m_color [1].green * RANDOM_FADE;
		m_color [0].blue = m_color [1].blue * RANDOM_FADE;
		m_nFadeState = -1;
		}
	}
SetupColor (fBrightness);
}

//------------------------------------------------------------------------------

int CParticle::Update (int nCurTime, float fBrightness, int nThread)
{
	int			j, nRad;
	short			nSegment;
	fix			t, dot;
	CFixVector	vPos, drift;
	fix			drag;
	
t = nCurTime - m_nMoved;
#if !ENABLE_UPDATE 
m_nLife -= t;
m_decay = ((m_nType == BUBBLE_PARTICLES) || (m_nType == WATERFALL_PARTICLES)) ? 1.0f : float (m_nLife) / float (m_nTTL);
#else
UpdateColor (fBrightness);
if (m_nType == BUBBLE_PARTICLES) 
	drag = I2X (1); // constant speed
else if (m_nType == WATERFALL_PARTICLES) {
	float h = 4.0f - 3.0f * m_decay;
	h *= h;
	drag = F2X (h);
	}
else 
	drag = F2X (m_decay); // decelerate

if ((m_nLife <= 0) /*|| (m_color [0].alpha < 0.01f)*/)
	return 0;

if (m_nDelay > 0)
	m_nDelay -= t;
else {
	vPos = m_vPos;
#if DBG
	drift = m_vDrift;
	CFixVector::Normalize (drift);
	if (CFixVector::Dot (drift, m_vDir) < 0)
		j = 0;
#endif
	drift = m_vDrift;
	if ((m_nType == SMOKE_PARTICLES) || (m_nType == FIRE_PARTICLES)) {
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
			if (CFixVector::Dot (vi, vj) < 0)
				drag = -drag;
			m_vPos += m_vDir * drag;
			}
		if ((m_nType == WATERFALL_PARTICLES) 
			 ? !m_bChecked 
			 : (m_nType == BUBBLE_PARTICLES) /*|| (m_nTTL - m_nLife > I2X (1) / 16)*/) {
			if (0 > (nSegment = FindSegByPos (m_vPos, m_nSegment, 0, 0, (m_nType == BUBBLE_PARTICLES) ? 0 : fix (m_nRad), nThread))) {
				m_nLife = -1;
				return 0;
				}
			if ((m_nType == BUBBLE_PARTICLES) && (SEGMENTS [nSegment].m_nType != SEGMENT_IS_WATER)) { 
				m_nLife = -1;
				return 0;
				}
			if (m_nType == WATERFALL_PARTICLES) {
				CFixVector vDir = m_vPos - m_vStartPos;
				if ((CFixVector::Normalize (vDir) >= I2X (1)) && (CFixVector::Dot (vDir, m_vDir) < I2X (1) / 2)) {
					m_nLife = -1;
					return 0;
					}
#if 1
				if (SEGMENTS [nSegment].m_nType == SEGMENT_IS_WATER) { 
					m_bChecked = 1;
					m_nLife = 500; 
					break;
					}
#endif
				}
			m_nSegment = nSegment;
			}
		if (gameOpts->render.particles.bCollisions && CollideWithWall (nThread)) {	//Reflect the particle
			if (j || (m_nType == BUBBLE_PARTICLES) || (m_nType == WATERFALL_PARTICLES) || !(dot = CFixVector::Dot (drift, *wallNorm))) {
				m_nLife = -1;
				return 0;
				}
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
#	if 0
		if ((m_nType == FIRE_PARTICLES) && !m_bReversed && (m_nLife <= m_nTTL / 4 + randN (m_nTTL / 4))) {
			m_vDrift = -m_vDrift;
			m_bReversed = 1;
			}
#	endif
#	if DBG
		if ((m_nLife <= 0) && (m_nType == 2))
			m_nLife = -1;
#	endif
#endif
		m_decay = ((m_nType == BUBBLE_PARTICLES) || (m_nType == WATERFALL_PARTICLES)) ? 1.0f : float (m_nLife) / float (m_nTTL);
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
#endif
m_nMoved = nCurTime;
return 1;
}

//------------------------------------------------------------------------------

int CParticle::Render (float fBrightness)
{
if (m_nDelay > 0)
	return 0;
if (m_nLife < 0)
	return 0;
if ((m_nType < 0) || (m_nType >= PARTICLE_TYPES))
	return 0;
#if 0 //DBG
if (m_nType == LIGHT_PARTICLES)
	m_nType = m_nType;
CBitmap* bmP = bmpParticle [0][int (m_nType)];
if (!bmP)
	return 0;
#endif

#if !ENABLE_RENDER
return 1;
#else

PROF_START

bool bFlushed = false;

if ((particleManager.LastType () != m_nType) || (fBrightness != bufferBrightness) || (bBufferEmissive != m_bEmissive)) {
	PROF_END(ptParticles)
	bFlushed = particleManager.FlushBuffer (fBrightness);
	PROF_CONT
	particleManager.SetLastType (m_nType);
	bBufferEmissive = m_bEmissive;
	}
else
	bFlushed = false;

#if LAZY_RENDER_SETUP
tRenderParticle* pb = particleBuffer + particleManager.BufPtr ();
pb->particle = this;
pb->fBrightness = fBrightness;
pb->nFrame = m_nFrame;
pb->nRotFrame = m_nRotFrame;
#else
Setup (fBrightness, m_nFrame, m_nRotFrame, particleRenderBuffer + particleManager.BufPtr () * 4, 0);
#endif
particleManager.IncBufPtr ();
if (particleManager.BufPtr () >= PART_BUF_SIZE)
	particleManager.FlushBuffer (fBrightness);

if (particleManager.Animate ()) {
	if (m_nFrames > 1) {
		m_nFrame = (m_nFrame + 1) % (m_nFrames * m_nFrames);
		UpdateTexCoord ();
		}
	if (!(m_nType || (m_nFrame & 1)))
		m_nRotFrame = (m_nRotFrame + 1) % PARTICLE_POSITIONS;
	}

PROF_END(ptParticles)
return bFlushed ? -1 : 1;
#endif
}

//------------------------------------------------------------------------------

int CParticle::SetupColor (float fBrightness)
{
if (m_bBright)
	fBrightness = float (sqrt (fBrightness));

m_renderColor = m_color [0];
if (m_nType == SMOKE_PARTICLES) {
	fBrightness *= 0.9f + (float) (rand () % 1000) / 5000.0f;
	m_renderColor.red *= fBrightness;
	m_renderColor.green *= fBrightness;
	m_renderColor.blue *= fBrightness;
	}

	float	fFade;

if (m_nFadeType == 0)	// default (start fully visible, fade out)
#if 1 
	m_renderColor.alpha *= m_decay * 0.6f;
#else
	m_renderColor.alpha *= float (cos (double (sqr (1.0f - m_decay)) * Pi) * 0.5 + 0.5) * 0.6f;
#endif
else if (m_nFadeType == 1)	// quickly fade in, then gently fade out
	m_renderColor.alpha *= float (sin (double (sqr (1.0f - m_decay)) * Pi * 1.5) * 0.5 + 0.5);
else if (m_nFadeType == 2) {	// fade in, then gently fade out
	float fPivot = m_nTTL / 4000.0f;
	if (fPivot > 0.25f)
		fPivot = 0.25f;
	float fLived = (m_nTTL - m_nLife) / 1000.0f;
	if (fLived < fPivot)
		fFade = fLived / fPivot;
	else
		fFade = 1.0f - (fLived - fPivot) / (1.0f - fPivot);
	if (fFade < 0.0f)
		fFade = 0.05f;
	else
		fFade = 0.1f + 0.9f * fFade;
	if (fFade > 1.0f)
		fFade = 1.0f;
	m_renderColor.alpha *= fFade;
	}
else if (m_nFadeType == 3) {	// fire (additive, blend in)
	if (m_decay > 0.5f)
		fFade = 2.0f * (1.0f - m_decay);
	else
		fFade = m_decay * 2.0f;
	m_decay = 1.0f;
	m_renderColor.red =
	m_renderColor.green = 
	m_renderColor.blue = fFade;
	m_color [0] = m_renderColor;
	}
else if (m_nFadeType == 4) {	// light trail (additive, constant effect)
	m_renderColor.red /= 50.0f;
	m_renderColor.green /= 50.0f;
	m_renderColor.blue /= 50.0f;
	}
if (m_renderColor.alpha >= 0.04f) //1.0 / 255.0
	return 1;
m_nLife = -1;
return 0;
}

//------------------------------------------------------------------------------

void CParticle::Setup (float fBrightness, char nFrame, char nRotFrame, tParticleVertex* pb, int nThread)
{
	CFloatVector vOffset;

#if 0 //DBG
if (m_nType == LIGHT_PARTICLES)
	m_nType = m_nType;

CBitmap* bmP = bmpParticle [0][int (m_nType)];
if (!bmP)
	return;
#endif
pb [0].color =
pb [1].color =
pb [2].color =
pb [3].color = m_renderColor;

pb [m_nOrient].texCoord.v.u =
pb [(m_nOrient + 3) % 4].texCoord.v.u = m_texCoord.v.u;
pb [m_nOrient].texCoord.v.v =
pb [(m_nOrient + 1) % 4].texCoord.v.v = m_texCoord.v.v;
pb [(m_nOrient + 1) % 4].texCoord.v.u =
pb [(m_nOrient + 2) % 4].texCoord.v.u = m_texCoord.v.u + m_deltaUV;
pb [(m_nOrient + 2) % 4].texCoord.v.v =
pb [(m_nOrient + 3) % 4].texCoord.v.v = m_texCoord.v.v + m_deltaUV;

if ((m_nType == SMOKE_PARTICLES) && gameOpts->render.particles.bDisperse) {
#if 0
	float decay = (float) pow (m_decay * m_decay * m_decay, 1.0f / 5.0f);
#else
	float decay = (float) pow (m_decay, 1.0f / 3.0f);
#endif
	vOffset [X] = X2F (m_nWidth) / decay;
	vOffset [Y] = X2F (m_nHeight) / decay;
	}
else {
	vOffset [X] = X2F (m_nWidth) * m_decay;
	vOffset [Y] = X2F (m_nHeight) * m_decay;
	}
vOffset [Z] = 0;

CFloatVector3	vCenter;

if (!gameOpts->render.n3DGlasses || (ogl.StereoSeparation () < 0))
	transformation.Transform (m_vTransPos, m_vPos, gameStates.render.bPerPixelLighting == 2);
vCenter.Assign (m_vTransPos);

if ((m_nType == BUBBLE_PARTICLES) && gameOpts->render.particles.bWiggleBubbles)
	vCenter [X] += (float) sin (nFrame / 4.0f * Pi) / (10 + rand () % 6);
if ((m_nType == SMOKE_PARTICLES) && gameOpts->render.particles.bRotate)  {
	int i = (m_nOrient & 1) ? 63 - m_nRotFrame : m_nRotFrame;
	vOffset [X] *= vRot [i][X];
	vOffset [Y] *= vRot [i][Y];

	pb [0].vertex [X] = vCenter [X] - vOffset [X];
	pb [0].vertex [Y] = vCenter [Y] + vOffset [Y];
	pb [1].vertex [X] = vCenter [X] + vOffset [Y];
	pb [1].vertex [Y] = vCenter [Y] + vOffset [X];
	pb [2].vertex [X] = vCenter [X] + vOffset [X];
	pb [2].vertex [Y] = vCenter [Y] - vOffset [Y];
	pb [3].vertex [X] = vCenter [X] - vOffset [Y];
	pb [3].vertex [Y] = vCenter [Y] - vOffset [X];
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
	}
pb [0].vertex [Z] =
pb [1].vertex [Z] =
pb [2].vertex [Z] =
pb [3].vertex [Z] = vCenter [Z];
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

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
return m_fBrightness = (float) objP->Damage () * 0.5f + 0.1f;
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
m_particles.Clear ();
m_nLife = nLife;
m_nBirth = nCurTime;
m_nSpeed = nSpeed;
m_nType = nType;
m_nFadeType = 0;
m_nClass = ObjectClass (nObject);
if ((m_bHaveColor = (colorP != NULL)))
	m_color = *colorP;
else
	m_color = defaultParticleColor;
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
m_fPartsPerTick = float (nMaxParts) / float (abs (nLife) * 1.25f);
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
		int				t, h, i, j, bSkip, nNewParts = 0;
		float				fDist;
		float				fBrightness = Brightness ();
		CFixMatrix		mOrient = m_mOrient;
		CFixVector		vDelta, vPos, *vDir = (m_bHaveDir ? &m_vDir : NULL),
							* vEmittingFace = m_bEmittingFace ? m_vEmittingFace : NULL;
		CFloatVector	vDeltaf, vPosf;

#if SMOKE_SLOWMO
	t = (int) ((nCurTime - m_nMoved) / gameStates.gameplay.slowmo [0].fSpeed);
#else
	t = nCurTime - m_nMoved;
#endif
	nPartSeg [nThread] = -1;
	
	//#pragma omp parallel
		{
		//#pragma omp for
		for (i = 0; i < m_nParts; i++)
			m_particles [(m_nFirstPart + i) % m_nPartLimit].Update (nCurTime, fBrightness, nThread);
		}
			
	for (i = 0, j = m_nFirstPart; i < m_nParts; i++) {
		if (m_particles [j].m_nLife <= 0) {
			if (j != m_nFirstPart)
				m_particles [j] = m_particles [m_nFirstPart];
			m_nFirstPart = (m_nFirstPart + 1) % m_nPartLimit;
			m_nParts--;
			}
		j = ++j % m_nPartLimit;
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
			fDist = X2F (vDelta.Mag ());
			h = m_nPartsPerPos;
			if (h > m_nMaxParts - m_nParts)
				h = m_nMaxParts - m_nParts;
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
				vPosf.Assign (m_vPrevPos);
				vDeltaf.Assign (vDelta);
				vDeltaf [X] /= (float) h;
				vDeltaf [Y] /= (float) h;
				vDeltaf [Z] /= (float) h;
				}
			j = (m_nFirstPart + m_nParts) % m_nPartLimit;
			bSkip = 0;
			//#pragma omp parallel
				{
				//#pragma omp for private(vPos) reduction(+: nNewParts)
				for (i = 0; i < h; i++) {
					if (bSkip)
						continue;
					vPos.Assign (vPosf + vDeltaf * float (i));
					if (m_particles [(j + i) % m_nPartLimit].Create (&vPos, vDir, &mOrient, m_nSegment, m_nLife,
																					 m_nSpeed, m_nType, m_nClass, m_fScale, m_bHaveColor ? &m_color : NULL,
																					 nCurTime, m_bBlowUpParts, m_nFadeType, fBrightness, vEmittingFace)) {
						nNewParts++;
						}
					if (/*(m_nType == LIGHT_PARTICLES) ||*/ (m_nType == BULLET_PARTICLES))
						bSkip = 1;
					}
				}
			m_nParts += nNewParts;
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
if (((nThread < 0)) && RunEmitterThread (emitterP, 0, rtRenderParticles)) {
	return 0;
	}
else
#endif
	{
#if 1
	PROF_START

	float		fBrightness = Brightness ();
	int		h, i, j;
	int		bVisible = (m_nObject >= 0x70000000) || MayBeVisible ();

#if DBG
	if (m_nFirstPart >= int (m_particles.Length ()))
		return 0;
	if (m_nPartLimit > int (m_particles.Length ()))
		m_nPartLimit = int (m_particles.Length ());
#endif
	for (h = 0, i = m_nParts, j = m_nFirstPart; i; i--, j = (j + 1) % m_nPartLimit)
		if ((bVisible || m_particles [j].IsVisible ()) && transparencyRenderer.AddParticle (m_particles + j, fBrightness, nThread))
			h++;
	PROF_END(ptParticles)
	return h;
#endif
	}
return 0;
}

//------------------------------------------------------------------------------

void CParticleEmitter::SetPos (CFixVector *vPos, CFixMatrix *mOrient, short nSegment)
{
if ((nSegment < 0) && gameOpts->render.particles.bCollisions)
	nSegment = FindSegByPos (*vPos, m_nSegment, 1, 0, 0, 1);
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
m_fPartsPerTick = nLife ? float (m_nMaxParts) / float (abs (nLife) * 1.25f) : 0.0f;
m_nTicks = 0;
}

//------------------------------------------------------------------------------

inline void CParticleEmitter::SetBrightness (int nBrightness)
{
m_nDefBrightness = nBrightness;
}

//------------------------------------------------------------------------------

inline void CParticleEmitter::SetFadeType (int nFadeType)
{
m_nFadeType = nFadeType;
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
	m_particles.SetBuffer (pp, 0, nMaxParts);
	}
m_nDensity = nDensity;
m_nMaxParts = nMaxParts;
#if 0
if (m_nParts > nMaxParts)
	m_nParts = nMaxParts;
#endif
m_fPartsPerTick = float (m_nMaxParts) / float (abs (m_nLife) * 1.25f);
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
m_bValid = 1;
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
m_bValid = 0;
m_bDestroy = false;
}

//------------------------------------------------------------------------------

void CParticleSystem::Destroy (void)
{
m_bValid = 0;
m_bDestroy = false;
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
if (m_bValid < 1)
	return 0;

	int	h = 0;
	CParticleEmitter* emitterP = m_emitters.Buffer ();

if (emitterP) {
	if (!particleImageManager.Load (m_nType))
		return 0;
	if ((m_nObject >= 0) && (m_nObject < 0x70000000) &&
		 ((OBJECTS [m_nObject].info.nType == OBJ_NONE) ||
		  (OBJECTS [m_nObject].info.nSignature != m_nSignature) ||
		  (particleManager.GetObjectSystem (m_nObject) < 0)))
		SetLife (0);
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
if (m_bValid && m_emitters.Buffer ())
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetPos (vPos, mOrient, nSegment);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetDensity (int nMaxParts, int nDensity)
{
if (m_bValid && m_emitters.Buffer ()) {
	nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.particles.nDens [0]);
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetDensity (nMaxParts, nDensity);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetScale (float fScale)
{
if (m_bValid && m_emitters.Buffer ())
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetScale (fScale);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetLife (int nLife)
{
if (m_bValid && m_emitters.Buffer () && (m_nLife != nLife)) {
	m_nLife = nLife;
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetLife (nLife);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetBrightness (int nBrightness)
{
if (m_bValid && m_emitters.Buffer ())
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetBrightness (nBrightness);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetFadeType (int nFadeType)
{
if (m_bValid && m_emitters.Buffer ())
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetFadeType (nFadeType);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetType (int nType)
{
if (m_bValid && m_emitters.Buffer () && (m_nType != nType)) {
	m_nType = nType;
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetType (nType);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetSpeed (int nSpeed)
{
if (m_bValid && m_emitters.Buffer () && (m_nSpeed != nSpeed)) {
	m_nSpeed = nSpeed;
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetSpeed (nSpeed);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetDir (CFixVector *vDir)
{
if (m_bValid && m_emitters.Buffer ())
	for (int i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetDir (vDir);
}

//------------------------------------------------------------------------------

int CParticleSystem::RemoveEmitter (int i)
{
if (m_bValid && m_emitters.Buffer () && (i < m_nEmitters)) {
	m_emitters [i].Destroy ();
	if (i < --m_nEmitters)
		m_emitters [i] = m_emitters [m_nEmitters];
	}
return m_nEmitters;
}

//------------------------------------------------------------------------------

int CParticleSystem::Update (int nThread)
{
if (m_bValid < 1)
	return 0;

	CParticleEmitter*				emitterP;
	CArray<CParticleEmitter*>	emitters;
	int								nEmitters = 0;

if ((m_nObject == 0x7fffffff) && (m_nType == SMOKE_PARTICLES) &&
	 (gameStates.app.nSDLTicks - m_nBirth > (MAX_SHRAPNEL_LIFE / I2X (1)) * 1000))
	SetLife (0);

if ((emitterP = m_emitters.Buffer ()) && emitters.Create (m_nEmitters)) {
	bool bKill = (m_nObject < 0) || ((m_nObject < 0x70000000) &&
					 ((OBJECTS [m_nObject].info.nSignature != m_nSignature) || (OBJECTS [m_nObject].info.nType == OBJ_NONE)));
	while (nEmitters < m_nEmitters) {
		if (!emitterP)
			return 0;
		if (emitterP->IsDead (gameStates.app.nSDLTicks)) {
			if (!RemoveEmitter (nEmitters)) {
				m_bDestroy = true;
				break;
				}
			}
		else {
			if (bKill)
				emitterP->SetLife (0);
			emitterP->Update (gameStates.app.nSDLTicks, nThread);
			nEmitters++, emitterP++;
			}
		}
	}
return nEmitters;
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
m_systemList.Create (MAX_PARTICLE_SYSTEMS);
int i = 0;
int nCurrent = m_systems.FreeList ();
for (CParticleSystem* systemP = m_systems.GetFirst (nCurrent); systemP; systemP = GetNext (nCurrent))
	systemP->Init (i++);
m_iBuffer = 0;

tSinCosf sinCosPart [PARTICLE_POSITIONS];
ComputeSinCosTable (sizeofa (sinCosPart), sinCosPart);
CFloatMatrix m;
m.RVec ()[Z] =
m.UVec ()[Z] =
m.FVec ()[X] =
m.FVec ()[Y] = 0;
m.FVec ()[Z] = 1;
CFloatVector v;
v.Set (1.0f, 1.0f, 0.0f, 1.0f);
for (int i = 0; i < PARTICLE_POSITIONS; i++) {
	m.RVec ()[X] =
	m.UVec ()[Y] = sinCosPart [i].fCos;
	m.UVec ()[X] = sinCosPart [i].fSin;
	m.RVec ()[Y] = -m.UVec ()[X];
	vRot [i] = m * v;
	}
}

//------------------------------------------------------------------------------

int CParticleManager::Destroy (int i)
{
#if 1
m_systems [i].m_bDestroy = true;
#else
m_systems [i].Destroy ();
m_systems.Push (i);
#endif
return i;
}

//------------------------------------------------------------------------------

void CParticleManager::Cleanup (void)
{
WaitForEffectsThread ();
int nCurrent = -1;
for (CParticleSystem* systemP = GetFirst (nCurrent), * nextP = NULL; systemP; systemP = nextP) {
	nextP = GetNext (nCurrent);
	if (systemP->m_bDestroy) {
		systemP->Destroy ();
		m_systems.Push (systemP->m_nId);
		}
	}
}

//------------------------------------------------------------------------------

int CParticleManager::Shutdown (void)
{
SEM_ENTER (SEM_SMOKE)
int nCurrent = -1;
for (CParticleSystem* systemP = GetFirst (nCurrent); systemP; systemP = GetNext (nCurrent))
	systemP->Destroy ();
Cleanup ();
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
CParticleSystem *systemP;
#pragma omp critical
{
if (!particleImageManager.Load (nType))
	systemP = NULL;
else
	systemP = m_systems.Pop ();
}
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

#ifdef _OPENMP
if (m_systemList.Buffer ()) {
	int nCurrent = -1;
	for (CParticleSystem* systemP = GetFirst (nCurrent); systemP; systemP = GetNext (nCurrent))
		m_systemList [h++] = systemP;
#	pragma omp parallel
		{
		int nThread = omp_get_thread_num();
#	pragma omp for
		for (int i = 0; i < h; i++)
			m_systemList [i]->Update (nThread);
		}
	}
else 
#endif
	{
	int nCurrent = -1;
	for (CParticleSystem* systemP = GetFirst (nCurrent); systemP; systemP = GetNext (nCurrent))
		h += systemP->Update (0);
	}
return h;
}

//------------------------------------------------------------------------------

void CParticleManager::Render (void)
{
int nCurrent = -1;
for (CParticleSystem* systemP = GetFirst (nCurrent); systemP; systemP = GetNext (nCurrent))
	systemP->Render ();
}

//------------------------------------------------------------------------------

int CParticleManager::InitBuffer (int bLightmaps)
{
ogl.DisableClientStates (1, 1, 1, GL_TEXTURE2);
ogl.DisableClientStates (1, 1, 1, GL_TEXTURE1);
if (bLightmaps) {
	ogl.BindTexture (0);
	ogl.SetTextureUsage (false);
	ogl.DisableClientStates (1, 1, 1, GL_TEXTURE3);
	}
ogl.EnableClientStates (1, 1, 0, GL_TEXTURE0/* + bLightmaps*/);
OglTexCoordPointer (2, GL_FLOAT, sizeof (tParticleVertex), &particleRenderBuffer [0].texCoord);
OglColorPointer (4, GL_FLOAT, sizeof (tParticleVertex), &particleRenderBuffer [0].color);
OglVertexPointer (3, GL_FLOAT, sizeof (tParticleVertex), &particleRenderBuffer [0].vertex);
return 1;
}

//------------------------------------------------------------------------------

void CParticleManager::SetupRenderBuffer (void)
{
PROF_START
#if (LAZY_RENDER_SETUP < 2)
if (m_iBuffer <= 100)
#endif
for (int i = 0; i < m_iBuffer; i++)
	particleBuffer [i].particle->Setup (particleBuffer [i].fBrightness, particleBuffer [i].nFrame, particleBuffer [i].nRotFrame, particleRenderBuffer + 4 * i, 0);
#if (LAZY_RENDER_SETUP < 2)
else
#endif
#pragma omp parallel
	{
#	ifdef _OPENMP
	int nThread = omp_get_thread_num();
#	else
	int nThread = 0;
#	endif
#	pragma omp for 
	for (int i = 0; i < m_iBuffer; i++)
		particleBuffer [i].particle->Setup (particleBuffer [i].fBrightness, particleBuffer [i].nFrame, particleBuffer [i].nRotFrame, particleRenderBuffer + 4 * i, nThread);
	}
PROF_END(ptParticles)
}

//------------------------------------------------------------------------------

bool CParticleManager::FlushBuffer (float fBrightness)
{
if (!m_iBuffer)
	return false;

int nType = particleManager.LastType ();
if (nType < 0)
	return false;
#if ENABLE_FLUSH
PROF_START
CBitmap *bmP = bmpParticle [0][nType];
if (!bmP) {
	PROF_END(ptParticles)
	return false;
	}
if (bmP->CurFrame ())
	bmP = bmP->CurFrame ();
if (bmP->Bind (0)) {
	PROF_END(ptParticles)
	return false;
	}

if (bufferBrightness < 0)
	bufferBrightness = fBrightness;

tRgbaColorf	color = {bufferBrightness, bufferBrightness, bufferBrightness, 1};
int bLightmaps = lightmapManager.HaveLightmaps ();
bufferBrightness = fBrightness;
ogl.SetDepthTest (true);
ogl.SetDepthMode (GL_LEQUAL);
ogl.SetDepthWrite (false);
ogl.SetBlendMode (bBufferEmissive ? 2 : 0);
#if LAZY_RENDER_SETUP
SetupRenderBuffer ();
#endif

if (InitBuffer (bLightmaps)) {
	if (ogl.m_states.bShadersOk) {
		if (lightManager.Headlights ().nLights && !(automap.Display () || nType))
			lightManager.Headlights ().SetupShader (1, 0, &color);
		else if (!((nType <= WATERFALL_PARTICLES) && (gameOpts->render.effects.bSoftParticles & 4) && glareRenderer.LoadShader (10, bBufferEmissive)))
			shaderManager.Deploy (-1);
		}
	glNormal3f (0, 0, 0);
	OglDrawArrays (GL_QUADS, 0, m_iBuffer * 4);
	glNormal3f (1, 1, 1);
	}
#if GL_FALLBACK
else {
	tParticleVertex *pb;
	glNormal3f (0, 0, 0);
	glBegin (GL_QUADS);
	for (pb = particleRenderBuffer; m_iBuffer; m_iBuffer--, pb++) {
		glTexCoord2fv (reinterpret_cast<GLfloat*> (&pb->texCoord));
		glColor4fv (reinterpret_cast<GLfloat*> (&pb->color));
		glVertex3fv (reinterpret_cast<GLfloat*> (&pb->vertex));
		}
	glEnd ();
	}
#endif
PROF_END(ptParticles)
#endif
m_iBuffer = 0;
if ((ogl.m_states.bShadersOk && !particleManager.LastType ()) && !glareRenderer.ShaderActive ())
	shaderManager.Deploy (-1);
return true;
}

//------------------------------------------------------------------------------

int CParticleManager::CloseBuffer (void)
{
FlushBuffer (-1);
ogl.DisableClientStates (1, 1, 0, GL_TEXTURE0 + lightmapManager.HaveLightmaps ());
return 1;
}

//------------------------------------------------------------------------------

int CParticleManager::BeginRender (int nType, float nScale)
{
	int				bLightmaps = lightmapManager.HaveLightmaps ();
	static time_t	t0 = 0;

#if 0
if (gameStates.render.bDepthSort <= 0) {
	nType = (nType % PARTICLE_TYPES);
	if ((nType >= 0) && !gameOpts->render.particles.bSort)
		particleImageManager.Animate (nType);
	bmP = bmpParticle [0][nType];
	particleManager.SetStencil (ogl.StencilOff ());
	InitBuffer (bLightmaps);
	ogl.SelectTMU (GL_TEXTURE0, true);
	ogl.SetFaceCulling (false);
	ogl.SetBlending (true);
	ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	ogl.SetTextureUsage (true);
	if ((nType >= 0) && bmP->Bind (0))
		return 0;
	ogl.SetDepthMode (GL_LESS);
	ogl.SetDepthWrite (false);
	m_iBuffer = 0;
	}
#endif
particleManager.SetLastType (-1);
if ((gameStates.app.nSDLTicks - t0 < 33) || (ogl.StereoSeparation () < 0))
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
#if 0
CloseBuffer ();
if (gameStates.render.bDepthSort <= 0) {
	ogl.BindTexture (0);
	ogl.SetTextureUsage (false);
	ogl.SetDepthWrite (true);
	ogl.StencilOn (particleManager.Stencil ());
	ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
#endif
return 1;
}

//------------------------------------------------------------------------------

CParticleManager::~CParticleManager ()
{
Shutdown ();
particleImageManager.FreeAll ();
m_systems.Destroy ();
m_systemList.Destroy ();
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
if (nType == FIRE_PARTICLES)
	return FIRE_PARTICLES;
if (nType == WATERFALL_PARTICLES)
	return WATERFALL_PARTICLES;
return -1;
}

//	-----------------------------------------------------------------------------

void CParticleImageManager::Animate (int nType)
{
	int	nFrames = nParticleFrames [0][nType];

if (nFrames > 1) {
	static time_t t0 [PARTICLE_TYPES] = {0, 0, 0, 0, 0};

	if (gameStates.app.nSDLTicks - t0 [nType] > 150) {
		CBitmap*	bmP = bmpParticle [0][GetType (nType)];
		if (!bmP->Frames ())
			return;
		int iFrame = iParticleFrames [0][nType];
		bmP->SetCurFrame (iFrame);
		t0 [nType] = gameStates.app.nSDLTicks;
		iParticleFrames [0][nType] = (iFrame + 1) % nFrames;
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
	int		h;
	CBitmap	*bmP = NULL;

nType = particleImageManager.GetType (nType);
if (bHavePartImg [0][nType])
	return 1;
if (!LoadAddonBitmap (bmpParticle [0] + nType, szParticleImg [0][nType], bHavePartImg [0] + nType))
	return 0;
#if MAKE_SMOKE_IMAGE
{
	tTgaHeader h;

TGAInterpolate (bmP, 2);
if (TGAMakeSquare (bmP)) {
	memset (&h, 0, sizeof (h));
	SaveTGA (szParticleImg [0][nType], gameFolders.szDataDir, &h, bmP);
	}
}
#endif
bmP = bmpParticle [0][nType];
bmP->SetFrameCount ();
bmP->SetupTexture (0, 1);
if (nType == SMOKE_PARTICLES)
	h = 8;
else if (nType == BUBBLE_PARTICLES)
	h = 4;
else if (nType == WATERFALL_PARTICLES)
	h = 8;
else if (nType == FIRE_PARTICLES)
	h = 4;
else
	h = bmP->FrameCount ();
nParticleFrames [0][nType] = h;
return 1;
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
