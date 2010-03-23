
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

//------------------------------------------------------------------------------

#define ROTATE_VERTICES 0

tRgbaColorf defaultParticleColor = {1.0f, 1.0f, 1.0f, 1.0f};

CFloatVector vRot [PARTICLE_POSITIONS];
CFixMatrix mRot [PARTICLE_POSITIONS];

#define SMOKE_START_ALPHA		(m_bBlowUp ? 64 : 96) //96 : 128)

//------------------------------------------------------------------------------

static inline int randN (int n)
{
if (!n)
	return 0;
return int (float (rand ()) * float (n) / float (RAND_MAX));
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
	int			nType = particleImageManager.GetType (nParticleSystemType);

m_bChecked = 0;
m_bBlowUp = bBlowUp && gameOpts->render.particles.bDisperse;
if (nScale < 0)
	m_nRad = float (-nScale);
else if (gameOpts->render.particles.bSyncSizes)
	m_nRad = float (PARTICLE_SIZE (gameOpts->render.particles.nSize [0], nScale, m_bBlowUp));
else
	m_nRad = float (nScale);
if (!m_nRad)
	m_nRad = 1.0f;
m_nType = nType;
m_bEmissive = (nParticleSystemType == LIGHT_PARTICLES) ? 1 : (nParticleSystemType == FIRE_PARTICLES) ? 2 : 0;
m_nClass = nClass;
m_nFadeType = nFadeType;
m_nSegment = nSegment;
m_nBounce = ((m_nType == BUBBLE_PARTICLES) || (m_nType == WATERFALL_PARTICLES)) ? 1 : 2;
m_bReversed = 0;
m_nMoved = nCurTime;
if (nLife < 0)
	nLife = -nLife;
m_nLife = nLife;
m_nDelay = 0; //bStart ? randN (nLife) : 0;

m_color [0] =
m_color [1] = 
color = (colorP && (m_bEmissive < 2)) ? *colorP : defaultParticleColor;

if ((nType == BULLET_PARTICLES) || (nType == BUBBLE_PARTICLES)) {
	m_bBright = 0;
	m_nFadeState = -1;
	}
else {
	m_bBright = (nType <= SMOKE_PARTICLES) ? (rand () % 50) == 0 : 0;
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
		m_color [0].alpha = float (SMOKE_START_ALPHA + 64) / 255.0f;
	else if (nParticleSystemType != GATLING_PARTICLES) {
		if (!colorP)
			m_color [0].alpha = float (3 * SMOKE_START_ALPHA / 4 + randN (SMOKE_START_ALPHA / 2)) / 255.0f;
		else {
			if (colorP->alpha < 0) {
				ubyte a = ubyte (-colorP->alpha * 255.0f * 0.25f + 0.5f);
				m_color [0].alpha = float (3 * a + randN (2 * a)) / 255.0f;
				}
			else {
				if (2 == (m_nFadeState = char (colorP->alpha))) {
					m_color [0].red = 1.0f;
					m_color [0].green = 0.5f;
					m_color [0].blue = 0.0f;
					}
				m_color [0].alpha = float (3 * SMOKE_START_ALPHA / 4 + randN (SMOKE_START_ALPHA / 2)) / 255.0f;
				}
			}
		if (m_bBlowUp && !m_bBright) {
			fBrightness = 1.0f - fBrightness;
			m_color [0].alpha += fBrightness * fBrightness / 8.0f;
			}
		}
	}
#if 0
if (nType == FIRE_PARTICLES)
	nSpeed = int (sqrt (double (nSpeed)) * float (I2X (1)));
else
#endif
	nSpeed *= I2X (1);
if (!vDir) {
	m_vDrift [X] = nSpeed - randN (2 * nSpeed);
	m_vDrift [Y] = nSpeed - randN (2 * nSpeed);
	m_vDrift [Z] = nSpeed - randN (2 * nSpeed);
	m_vDir.SetZero ();
	m_bHaveDir = 1;
	}
else {
	m_vDir = *vDir;

	CAngleVector	a;
	CFixMatrix		m;
	a [PA] = randN (I2X (1) / 4) - I2X (1) / 8;
	a [BA] = randN (I2X (1) / 4) - I2X (1) / 8;
	a [HA] = randN (I2X (1) / 4) - I2X (1) / 8;
	m = CFixMatrix::Create (a);
	if (nType == WATERFALL_PARTICLES)
		CFixVector::Normalize (m_vDir);
	m_vDrift = m * m_vDir;
	CFixVector::Normalize (m_vDrift);
	if (nType == WATERFALL_PARTICLES) {
		fix dot = CFixVector::Dot (m_vDir, m_vDrift);
		if (dot < I2X (1) / 2)
			return 0;
		}
	float d = float (CFixVector::DeltaAngle (m_vDrift, m_vDir, NULL));
	if (d) {
		d = (float) exp ((I2X (1) / 8) / d);
		nSpeed = (fix) ((float) nSpeed / d);
		}
	m_vDrift *= nSpeed;
	if (nType <= FIRE_PARTICLES)
		m_vDir *= (I2X (3) / 4 + I2X (randN (16)) / 64);
#if DBG
	if (CFixVector::Dot (m_vDrift, m_vDir) < 0)
		d = 0;
#endif
	m_bHaveDir = 1;
	}

if (vEmittingFace)
	m_vPos = *RandomPointOnQuad (vEmittingFace, vPos);
else if (nType != BUBBLE_PARTICLES)
	m_vPos = *vPos + m_vDrift * (I2X (1) / 64);
else {
	//m_vPos = *vPos + vDrift * (I2X (1) / 32);
	nSpeed = m_vDrift.Mag () / 16;
	CFixVector v = CFixVector::Avg ((*mOrient).RVec () * (nSpeed - randN (2 * nSpeed)), (*mOrient).UVec () * (nSpeed - randN (2 * nSpeed)));
	m_vPos = *vPos + v + (*mOrient).FVec () * (I2X (1) / 2 - randN (I2X (1)));
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

if (nType <= SMOKE_PARTICLES) {
	if (m_bBlowUp)
		m_nLife = 2 * m_nLife / 3;
	m_nLife = 4 * m_nLife / 5 + randN (2 * m_nLife / 5);
	m_nRad += float (randN (int (m_nRad)));
	}
else if (nType == FIRE_PARTICLES) {
	m_nLife = 3 * m_nLife / 4 + randN (m_nLife / 4);
	m_nRad += float (randN (int (m_nRad)));
	}
else if (nType == BUBBLE_PARTICLES)
	m_nRad = m_nRad / 10 + float (randN (int (9 * m_nRad / 10)));
else
	m_nRad *= 2;

//m_nRad *= 0.5f;
m_vStartPos = m_vPos;

if (m_bBlowUp) {
	m_nWidth = (nType == WATERFALL_PARTICLES) ? m_nRad * 0.6666667f : m_nRad;
	m_nHeight = m_nRad;
	}
else {
	m_nWidth = (nType == WATERFALL_PARTICLES) ? m_nRad * 0.3333333f : m_nRad * 2;
	m_nHeight = m_nRad * 2;
	}
m_nWidth /= 65536.0f;
m_nHeight /= 65536.0f;
m_nRad /= 65536.0f;

m_nFrames = ParticleImageInfo (nType).nFrames;
m_deltaUV = 1.0f / float (m_nFrames);
if (nType == BULLET_PARTICLES) {
	m_iFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 3;
	}
else if (nType == BUBBLE_PARTICLES) {
	m_iFrame = rand () % (m_nFrames * m_nFrames);
	m_nRotFrame = 0;
	m_nOrient = 0;
	}
else if ((nType == LIGHT_PARTICLES) /*|| (nType == WATERFALL_PARTICLES)*/) {
	m_iFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 0;
	}
else if (nType == FIRE_PARTICLES) {
	m_iFrame = (rand () % 10 < 6) ? 0 : 2;	// more fire than smoke (60:40)
	if (m_iFrame < 2)
		m_nLife  = 9 * m_nLife / 10;
	else
		m_nLife  = 10 * m_nLife / 9;
	m_nRotFrame = rand () % PARTICLE_POSITIONS;
	m_nOrient = rand () % 4;
	}
else {
	m_iFrame = rand () % (m_nFrames * m_nFrames);
	m_nRotFrame = rand () % PARTICLE_POSITIONS;
	m_nOrient = rand () % 4;
#if 1
#endif
	}
m_nTTL = m_nLife;

m_nRenderType = RenderType ();
m_bAnimate = (nType != FIRE_PARTICLES) && (gameOpts->render.particles.nQuality > 1) && (m_nFrames > 1);
m_bRotate = (m_nRenderType <= SMOKE_PARTICLES) ? 1 : (m_nRenderType == FIRE_PARTICLES + PARTICLE_TYPES) ? -1 : 0;

UpdateDecay ();
UpdateTexCoord ();
#if 0
if (colorP && (colorP->alpha < 0))
	m_color [0].alpha /= 2.0f;
else 
#endif
	{
	if (m_bEmissive)
		m_color [0].alpha = 1.0f;
	else if (nParticleSystemType == SIMPLE_SMOKE_PARTICLES)
		m_color [0].alpha /= 3.5f - float (gameOpts->render.particles.nQuality) / 2.0f; //colorP ? 2.0f + (color.red + color.green + color.blue) / 3.0f : 2.0f;
	else if (nParticleSystemType == SMOKE_PARTICLES)
		m_color [0].alpha /= colorP ? 3.0f - (color.red + color.green + color.blue) / 3.0f : 2.5f;
	else if (nParticleSystemType == BUBBLE_PARTICLES)
		m_color [0].alpha /= 2.0f;
	else if (nParticleSystemType == GATLING_PARTICLES)
		m_color [0].alpha /= 4.0f;
#	if 0
	else if (nParticleSystemType == GATLING_PARTICLES)
		;//m_color [0].alpha /= 6;
#	endif
	}
SetupColor (fBrightness);
return 1;
}

//------------------------------------------------------------------------------

bool CParticle::IsVisible (int nThread)
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
int nSegment = FindSegByPos (m_vPos, m_nSegment, 0, 0, 0, nThread);
if (nSegment < 0)
	return false;
m_nSegment = nSegment;
return gameData.render.mine.bVisible [nSegment] == gameData.render.mine.nVisible;
#endif
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

int nPartSeg [MAX_THREADS] = {-1, -1, -1, -1}; //, -1, -1, -1, -1};

static int nFaceCount [MAX_THREADS][6];
static int bSidePokesOut [MAX_THREADS][6];
//static int nVert [6];
static CFixVector* wallNorm [MAX_THREADS];

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
			wallNorm [nThread] = sideP->m_normals + nFace;
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
	m_texCoord.v.u = float (m_iFrame % m_nFrames) * m_deltaUV;
	m_texCoord.v.v = float (m_iFrame / m_nFrames) * m_deltaUV;
	}
}

//------------------------------------------------------------------------------

void CParticle::UpdateColor (float fBrightness, int nThread)
{
if (m_nType <= SMOKE_PARTICLES) {
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
#if SMOKE_LIGHTING //> 1
	if (gameOpts->render.particles.nQuality == 3) {
		if (0 <= (m_nSegment = FindSegByPos (m_vPos, m_nSegment, 0, 0, 0, nThread))) {
			tFaceColor* colorP = lightManager.AvgSgmColor (m_nSegment, NULL, nThread);
			m_color [0].red *= colorP->color.red;
			m_color [0].green *= colorP->color.green;
			m_color [0].blue *= colorP->color.blue;
			}
		}
#endif
	}

SetupColor (fBrightness);
}

//------------------------------------------------------------------------------

fix CParticle::Drag (void)
{
if ((m_nType == BUBBLE_PARTICLES) || (m_nType == FIRE_PARTICLES))
	return I2X (1); // constant speed
else if (m_nType == WATERFALL_PARTICLES) {
	float h = 4.0f - 3.0f * m_decay;
	h *= h;
	return F2X (h);
	}
else 
	return I2X (1); //F2X (m_decay); // decelerate
}

//------------------------------------------------------------------------------

int CParticle::Bounce (int nThread)
{
if (!gameOpts->render.particles.bCollisions)
	return 1;
if (!CollideWithWall (nThread)) {
	m_nBounce = ((m_nType == BUBBLE_PARTICLES) || (m_nType == WATERFALL_PARTICLES)) ? 1 : 2;
	return 1;
	}
if (!--m_nBounce)
	return 0;
fix dot = CFixVector::Dot (m_vDrift, *wallNorm [nThread]);
if (dot < I2X (1) / 100) {
	m_nLife = -1;
	return 0;
	}
m_vDrift += m_vDrift + *wallNorm [nThread] * (-2 * dot);
return 1;
}

//------------------------------------------------------------------------------

int CParticle::UpdateDrift (int t, int nThread)
{
m_vPos += m_vDrift * t; //(I2X (t) / 1000);

#if DBG
CFixVector vDrift = m_vDrift;
CFixVector::Normalize (vDrift);
if (CFixVector::Dot (vDrift, m_vDir) < 0)
	t = t;
#endif

if ((m_nType <= SMOKE_PARTICLES) || (m_nType == FIRE_PARTICLES)) {
	m_vDrift [X] = ChangeDir (m_vDrift [X]);
	m_vDrift [Y] = ChangeDir (m_vDrift [Y]);
	m_vDrift [Z] = ChangeDir (m_vDrift [Z]);
	}

if (m_bHaveDir) {
	CFixVector vi = m_vDrift, vj = m_vDir;
	CFixVector::Normalize (vi);
	CFixVector::Normalize (vj);
	fix drag = Drag ();
	if (CFixVector::Dot (vi, vj) < 0)
		drag = -drag;
	m_vPos += m_vDir * drag;
	}

int nSegment = FindSegByPos (m_vPos, m_nSegment, m_nSegment < 0, 1, (m_nType == BUBBLE_PARTICLES) ? 0 : fix (m_nRad), nThread);
if ((0 > nSegment) && ((m_nType != WATERFALL_PARTICLES) || m_bChecked)) {
	if (m_nType == BUBBLE_PARTICLES) { 
		if (SEGMENTS [nSegment].m_nType != SEGMENT_IS_WATER) {
			m_nLife = -1;
			return 0;
			}
		}
	else if (m_nType == WATERFALL_PARTICLES) {
		CFixVector vDir = m_vPos - m_vStartPos;
		if ((CFixVector::Normalize (vDir) >= I2X (1)) && (CFixVector::Dot (vDir, m_vDir) < I2X (1) / 2)) {
			m_nLife = -1;
			return 0;
			}
		if (SEGMENTS [nSegment].m_nType == SEGMENT_IS_WATER) { 
			m_bChecked = 1;
			m_nLife = 500; 
			}
		}
	else if (m_nTTL - m_nLife > 500) {
		m_nLife = -1;
		return 0;
		}
	}
m_nSegment = nSegment;

if (!Bounce (nThread))
	return 0;

return 1;
}

//------------------------------------------------------------------------------

void CParticle::UpdateDecay (void)
{
if ((m_nType == BUBBLE_PARTICLES) || (m_nType == WATERFALL_PARTICLES)) 
	m_decay = 1.0f;
else if (m_nType == FIRE_PARTICLES) {
#if 1
	m_decay = float (sin (double (m_nLife) / double (m_nTTL) * Pi));
#else
	m_decay = float (m_nLife) / float (m_nTTL);
	if (m_decay < 0.4)
		m_decay = float (sin (double (m_decay) * Pi * 1.25));
	else if (m_decay > 0.15)
		m_decay = float (sin (double (1.0 - m_decay) * Pi * 0.5 / 0.15));
	else
		m_decay = 1.0f;
#endif
	}	
else
	m_decay = float (m_nLife) / float (m_nTTL);
}

//------------------------------------------------------------------------------

int CParticle::Update (int nCurTime, float fBrightness, int nThread)
{
if ((m_nLife <= 0) /*|| (m_color [0].alpha < 0.01f)*/)
	return 0;

fix t = nCurTime - m_nMoved;
m_nMoved = nCurTime;

#if !ENABLE_UPDATE 
m_nLife -= t;
m_decay = ((m_nType == BUBBLE_PARTICLES) || (m_nType == WATERFALL_PARTICLES)) ? 1.0f : float (m_nLife) / float (m_nTTL);
#else
UpdateColor (fBrightness, nThread);

if (m_nDelay > 0) {
	m_nDelay -= t;
	return 1;
	}

if (!UpdateDrift (t, nThread))
	return 0;

if (m_nTTL < 0) 
	return 1;

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
if (m_nLife < 0)
	m_nLife = 0;
UpdateDecay ();

if ((m_nType <= SMOKE_PARTICLES) && (m_nRad > 0)) {
	if (m_bBlowUp) {
		if (m_nWidth >= m_nRad)
			m_nRad = 0;
		else {
			m_nWidth += m_nRad * 0.1f;
			if (m_nWidth > m_nRad)
				m_nWidth = m_nRad;
			m_nHeight += m_nRad * 0.1f;
			if (m_nHeight > m_nRad)
				m_nHeight = m_nRad;
			m_color [0].alpha *= (1.0f + 0.0725f / m_bBlowUp);
			if (m_color [0].alpha > 1)
				m_color [0].alpha = 1;
			}
		}
	else {
		if (m_nWidth <= m_nRad)
			m_nRad = 0;
		else {
			m_nRad *= 1.2f;
			m_color [0].alpha *= 1.0725f;
			if (m_color [0].alpha > 1)
				m_color [0].alpha = 1;
			}
		}
	}
#endif
return 1;
}

//------------------------------------------------------------------------------

inline int CParticle::RenderType (void)
{
if ((m_nType != FIRE_PARTICLES) || /*(gameOpts->render.particles.nQuality < 2) ||*/ (m_iFrame < m_nFrames))
	return m_nType;
return PARTICLE_TYPES + m_nType;
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
CBitmap* bmP = ParticleImageInfo (int (m_nType)).bmP;
if (!bmP)
	return 0;
#endif

#if !ENABLE_RENDER
return 1;
#else

PROF_START

bool bFlushed = false;

if (particleManager.LastType () != m_nRenderType) {
	PROF_END(ptParticles)
	bFlushed = particleManager.FlushBuffer (fBrightness);
	PROF_CONT
	particleManager.SetLastType (m_nRenderType);
	particleManager.m_bBufferEmissive = m_bEmissive;
	}
else
	bFlushed = false;

#if LAZY_RENDER_SETUP
tRenderParticle* pb = particleManager.particleBuffer + particleManager.BufPtr ();
pb->particle = this;
pb->fBrightness = fBrightness;
pb->nFrame = m_iFrame;
pb->nRotFrame = m_nRotFrame;
#else
Setup (fBrightness, m_iFrame, m_nRotFrame, particleRenderBuffer + particleManager.BufPtr () * 4, 0);
#endif
particleManager.IncBufPtr ();
if (particleManager.BufPtr () >= PART_BUF_SIZE)
	particleManager.FlushBuffer (fBrightness);

if (particleManager.Animate ()) {
	if (m_bAnimate && (m_nFrames > 1)) {
		m_iFrame = (m_iFrame + 1) % (m_nFrames * m_nFrames);
		UpdateTexCoord ();
		}
	if (m_bRotate) {
		if (m_bRotate < 0)
			m_nRotFrame = (m_nRotFrame + 1) % PARTICLE_POSITIONS;
		else {
			m_bRotate <<= 1;
			if (m_bRotate == 4) {
				m_bRotate = 1;
				m_nRotFrame = (m_nRotFrame + 1) % PARTICLE_POSITIONS;
				}
			}
		}
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
if (m_nType <= SMOKE_PARTICLES) {
	fBrightness *= 0.9f + (float) (rand () % 1000) / 5000.0f;
	m_renderColor.red *= fBrightness;
	m_renderColor.green *= fBrightness;
	m_renderColor.blue *= fBrightness;
	}

	float	fFade;

if (m_nFadeType == 0) {	// default (start fully visible, fade out)
#if 1 
	m_renderColor.alpha *= m_decay; // * 0.6f;
#else
	m_renderColor.alpha *= float (cos (double (sqr (1.0f - m_decay)) * Pi) * 0.5 + 0.5) * 0.6f;
#endif
	}
else if (m_nFadeType == 1)	{ // quickly fade in, then gently fade out
	m_renderColor.alpha *= float (sin (double (sqr (1.0f - m_decay)) * Pi * 1.5) * 0.5 + 0.5);
	if (m_decay >= 0.666f)
		return 1;
	}
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
	if (m_decay >= 0.75f) 
		return 1;
	}
else if (m_nFadeType == 3) {	// fire (additive, blend in)
#if 0
	if (m_decay > 0.5f)
		fFade = 2.0f * (1.0f - m_decay);
	else
		fFade = m_decay * 2.0f;
	if ((m_decay < 0.5f) && (fFade < 0.00333f)) {
		m_nLife = -1;
		return 0;
		}
	m_renderColor.red =
	m_renderColor.green = 
	m_renderColor.blue = fFade;
	m_color [0] = m_renderColor;
#endif
	return 1;
	}
else if (m_nFadeType == 4) {	// light trail (additive, constant effect)
	m_renderColor.red /= 50.0f;
	m_renderColor.green /= 50.0f;
	m_renderColor.blue /= 50.0f;
	return 1;
	}
if (m_renderColor.alpha >= 0.01f) //1.0 / 255.0
	return 1;
m_nLife = -1;
return 0;
}

//------------------------------------------------------------------------------

#if ROTATE_VERTICES

void CParticle::Setup (float fBrightness, char nFrame, char nRotFrame, tParticleVertex* pb, int nThread)
{
CFloatVector3	vCenter, vOffset;

transformation.Transform (m_vTransPos, m_vPos, gameStates.render.bPerPixelLighting == 2);
vCenter.Assign (m_vTransPos);

if ((m_nType <= SMOKE_PARTICLES) && m_bBlowUp) {
#if DBG
	float fFade;
	if (m_nFadeType == 3)
		fFade = 1.0;
	else if (m_decay > 0.9f)
		fFade = (1.0f - pow (m_decay, 44.0f)) / float (pow (m_decay, 0.25f));
	else
		fFade = 1.0f / float (pow (m_decay, 0.25f));
#else
	float fFade = (m_nFadeType == 3) 
		? 1.0f 
		: (m_decay > 0.9f)	// start from zero size by scaling with pow (m_decay, 44f) which is < 0.01 for m_decay == 0.9f
			? (1.0f - pow (m_decay, 44.0f)) / float (pow (m_decay, 0.3333333f))
			: 1.0f / float (pow (m_decay, 0.3333333f));
#endif
	vOffset [X] = m_nWidth * fFade;
	vOffset [Y] = m_nHeight * fFade;
	}
else {
	vOffset [X] = m_nWidth * m_decay;
	vOffset [Y] = m_nHeight * m_decay;
	}
vOffset [Z] = 0;

float h = ParticleImageInfo (m_nType).xBorder;
pb [m_nOrient].texCoord.v.u =
pb [(m_nOrient + 3) % 4].texCoord.v.u = m_texCoord.v.u + h;
pb [(m_nOrient + 1) % 4].texCoord.v.u =
pb [(m_nOrient + 2) % 4].texCoord.v.u = m_texCoord.v.u + m_deltaUV - h;
h = ParticleImageInfo (m_nType).yBorder;
pb [m_nOrient].texCoord.v.v =
pb [(m_nOrient + 1) % 4].texCoord.v.v = m_texCoord.v.v + h;
pb [(m_nOrient + 2) % 4].texCoord.v.v =
pb [(m_nOrient + 3) % 4].texCoord.v.v = m_texCoord.v.v + m_deltaUV - h;

pb [0].color =
pb [1].color =
pb [2].color =
pb [3].color = m_renderColor;

if ((m_nType == BUBBLE_PARTICLES) && gameOpts->render.particles.bWiggleBubbles)
	vCenter [X] += (float) sin (nFrame / 4.0f * Pi) / (10 + rand () % 6);
if (m_bRotate && gameOpts->render.particles.bRotate)  {
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

#else // -----------------------------------------------------------------------

void CParticle::Setup (float fBrightness, char nFrame, char nRotFrame, tParticleVertex* pb, int nThread)
{
	CFloatVector3	vCenter, uVec, rVec;
	float				fScale;

vCenter.Assign (m_vPos);

if ((m_nType <= SMOKE_PARTICLES) && m_bBlowUp) {
#if DBG
	if (m_nFadeType == 3)
		fScale = 1.0;
	else if (m_decay > 0.9f)
		fScale = (1.0f - pow (m_decay, 44.0f)) / float (pow (m_decay, 0.25f));
	else
		fScale = 1.0f / float (pow (m_decay, 0.25f));
#else
	fScale = (m_nFadeType == 3) 
		? 1.0f 
		: (m_decay > 0.9f)	// start from zero size by scaling with pow (m_decay, 44f) which is < 0.01 for m_decay == 0.9f
			? (1.0f - pow (m_decay, 44.0f)) / float (pow (m_decay, 0.3333333f))
			: 1.0f / float (pow (m_decay, 0.3333333f));
#endif
	}
else {
	fScale = m_decay;
	}

pb [0].color =
pb [1].color =
pb [2].color =
pb [3].color = m_renderColor;

float hx = ParticleImageInfo (m_nType).xBorder;
pb [m_nOrient].texCoord.v.u =
pb [(m_nOrient + 3) % 4].texCoord.v.u = m_texCoord.v.u + hx;
pb [(m_nOrient + 1) % 4].texCoord.v.u =
pb [(m_nOrient + 2) % 4].texCoord.v.u = m_texCoord.v.u + m_deltaUV - hx;
float hy = ParticleImageInfo (m_nType).yBorder;
pb [m_nOrient].texCoord.v.v =
pb [(m_nOrient + 1) % 4].texCoord.v.v = m_texCoord.v.v + hy;
pb [(m_nOrient + 2) % 4].texCoord.v.v =
pb [(m_nOrient + 3) % 4].texCoord.v.v = m_texCoord.v.v + m_deltaUV - hy;

if ((m_nType == BUBBLE_PARTICLES) && gameOpts->render.particles.bWiggleBubbles)
	vCenter [X] += (float) sin (nFrame / 4.0f * Pi) / (10 + rand () % 6);
if (m_bRotate && gameOpts->render.particles.bRotate) {
	int i = (m_nOrient & 1) ? 63 - m_nRotFrame : m_nRotFrame;
	CFixMatrix mOrient = gameData.objs.viewerP->info.position.mOrient * mRot [i];
	uVec.Assign (mOrient.UVec ());
	rVec.Assign (mOrient.RVec ());
	}
else {
	uVec.Assign (gameData.objs.viewerP->info.position.mOrient.UVec ());
	rVec.Assign (gameData.objs.viewerP->info.position.mOrient.RVec ());
	}
uVec *= m_nHeight * fScale;
rVec *= m_nWidth * fScale;
pb [0].vertex = vCenter - rVec - uVec;
pb [1].vertex = vCenter - rVec + uVec;
pb [2].vertex = vCenter + rVec + uVec;
pb [3].vertex = vCenter + rVec - uVec;
}

#endif

//------------------------------------------------------------------------------
//eof
