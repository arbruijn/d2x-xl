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
#include "segmath.h"
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

CFloatVector defaultParticleColor = { 1.0f, 1.0f, 1.0f, 1.0f };

CFloatVector CParticle::vRot [PARTICLE_POSITIONS];
CFixMatrix CParticle::mRot [2][PARTICLE_POSITIONS];

static int smokeStartAlpha [2][5] = {{160, 128, 96, 64, 32}, {128, 96, 64, 32, 16}};

//------------------------------------------------------------------------------

static inline int RandN (int n) 
{
return n ? int (RandFloat () * float (n)) : 0;
}

//------------------------------------------------------------------------------

inline float sqr (float n) 
{
return n * n;
}

//------------------------------------------------------------------------------

static inline float SmokeStartAlpha (char bBlowUp, char nClass)
{
int alpha = smokeStartAlpha [int (bBlowUp)][gameOpts->render.particles.nAlpha [gameOpts->app.bExpertMode ? int (nClass) : 0]];
return float (3 * alpha / 4 + RandN (alpha / 2)) / 255.0f;
}

//------------------------------------------------------------------------------

inline float ParticleBrightness (CFloatVector *colorP) 
{
#if 0
return (colorP->Red () + colorP->Green () + colorP->Blue ()) / 3.0f;
#else
return colorP ? (colorP->Red () * 3 + colorP->Green () * 5 + colorP->Blue () * 2) / 10.0f : 1.0f;
#endif
}

//------------------------------------------------------------------------------

CFixVector* RandomPointOnTriangle (CFixVector *triangle, CFixVector *vPos) 
{
fix a = 2 * RandShort ();
fix b = 2 * RandShort ();
if (a + b > I2X (1)) {
	a = I2X (1) - a;
	b = I2X (1) - b;
	}
fix c = I2X (1) - a - b;
*vPos = triangle [0] * a;
*vPos += triangle [1] * b;
*vPos += triangle [2] * c;
return vPos;
}

//------------------------------------------------------------------------------

CFixVector* RandomPointOnQuad (CFixVector *quad, CFixVector *vPos) 
{
for (int i = 0; i < 4; i++)
	if ((quad [i].v.coord.x == 0x7fffffff) && (quad [i].v.coord.y == 0x7fffffff) && (quad [i].v.coord.z == 0x7fffffff))
		break;
if (i < 3)
	return null;
if (i == 3)
	return RandomPointOnTriangle (quad, vPos);
if (rand () % 2)
	return RandomPointOnTriangle (quad, vPos);
quad [1] = quad [0];
return RandomPointOnTriangle (quad + 1, vPos);
}

//------------------------------------------------------------------------------

void CParticle::InitRotation (void)
{
CAngleVector vRotAngs;
vRotAngs.SetZero ();
for (int i = 0; i < PARTICLE_POSITIONS; i++) {
	vRotAngs.v.coord.b = i * (I2X (1) / PARTICLE_POSITIONS);
	CParticle::mRot [0][i] = CFixMatrix::Create (vRotAngs);
	}
}

//------------------------------------------------------------------------------

void CParticle::SetupRotation (void)
{
for (int i = 0; i < PARTICLE_POSITIONS; i++)
	mRot [1][i] = gameData.render.mine.viewer.mOrient * mRot [0][i];
}

//------------------------------------------------------------------------------

#define RANDOM_FADE	 (0.95f + RandFloat (20.0f))

static int brightFlags [PARTICLE_TYPES] = {1,1,0,0,0,1,1,0,1,1};

void CParticle::InitColor (CFloatVector* colorP, float fBrightness, char nParticleSystemType)
{
	CFloatVector color;

m_bEmissive = (nParticleSystemType == LIGHT_PARTICLES) 
					? 1
					: (nParticleSystemType == FIRE_PARTICLES) 
						? 2
						//: ((nParticleSystemType <= SMOKE_PARTICLES) || (nParticleSystemType <= WATERFALL_PARTICLES))
						//	? 3
						: 0;
m_color [0] = m_color [1] = color = (colorP && (m_bEmissive != 2)) ? *colorP : defaultParticleColor;

if (!brightFlags [(int) m_nType]) {
	m_bBright = 0;
	m_nFadeTime = -1;
	if (colorP && (colorP->Alpha () < 0)) {
		ubyte a = ubyte (-colorP->Alpha () * 255.0f * 0.25f + 0.5f);
		m_color [1].Alpha () = float (3 * a + RandN (2 * a)) / 255.0f;
		}
	}
else {
	m_bBright = (m_nType <= SMOKE_PARTICLES) ? (rand () % 50) == 0 : 0;
	if (colorP) {
		if (!m_bEmissive /*|| (m_bEmissive == 3)*/) {
			m_color [0].Red () *= RANDOM_FADE;
			m_color [0].Green () *= RANDOM_FADE;
			m_color [0].Blue () *= RANDOM_FADE;
			}
		m_nFadeTime = 0;
		} 
	else {
		colorP = &m_color [0];
		m_color [0].Alpha () = 2.0f;
		}
	if (m_bEmissive /*&& (m_bEmissive < 3)*/)
		; // m_color [0].Alpha () = float (SMOKE_START_ALPHA + 64) / 255.0f;
	else if (nParticleSystemType != GATLING_PARTICLES) {
		if (!colorP)
			m_color [1].Alpha () = SmokeStartAlpha (m_bBlowUp, m_nClass);
		else if (colorP->Alpha () < 0) {
			ubyte a = ubyte (-colorP->Alpha () * 255.0f * 0.25f + 0.5f);
			m_color [1].Alpha () = float (3 * a + RandN (2 * a)) / 255.0f;
			} 
		else if (char (colorP->Alpha ()) != 2.0f) 
			m_color [1].Alpha () = SmokeStartAlpha (m_bBlowUp, m_nClass);
		else {
			if ((m_bEmissive = (gameOpts->render.particles.nQuality > 2))) {
				m_color [0].Red () = 0.5f + RandFloat (2.0f);
				m_color [0].Green () = m_color [0].Red () * (0.5f + RandFloat (2.0f));
				}
			else {
#if 1
				m_color [0].Red () = 
				m_color [0].Green () = 1.0;
#else
				m_color [0].Red () = 0.9f + RandFloat (10.0f);
				m_color [0].Green () = m_color [0].Red () * (0.9f + RandFloat (10.0f));
#endif
				}
			m_color [0].Blue () = 0.0f;
			m_nFadeTime = 50 + rand () % 150;
			m_color [1].Red () *= RANDOM_FADE;
			m_color [1].Green () *= RANDOM_FADE;
			m_color [1].Blue () *= RANDOM_FADE;
			m_nWidth *= 0.75;
			m_nHeight *= 0.75;
			m_color [1].Alpha () = SmokeStartAlpha (m_bBlowUp, m_nClass);
			}
		if (m_bBlowUp && !m_bBright) {
			fBrightness = 1.0f - fBrightness;
			if (m_nFadeTime <= 0)
				m_color [1].Alpha () += fBrightness * fBrightness / 8.0f;
			}
		}
	}

if (nParticleSystemType == SIMPLE_SMOKE_PARTICLES)
	m_color [1].Alpha () /= 3.5f - float (1 + int (gameOpts->render.particles.nQuality > 1)) / 2.0f; //colorP ? 2.0f + (color.Red () + color.Green () + color.Blue ()) / 3.0f : 2.0f;
else if (nParticleSystemType == SMOKE_PARTICLES)
	m_color [1].Alpha () /= colorP ? 3.0f - (color.Red () + color.Green () + color.Blue ()) / 3.0f : 2.5f;
else if ((nParticleSystemType == BUBBLE_PARTICLES) || (nParticleSystemType == RAIN_PARTICLES) || (nParticleSystemType == SNOW_PARTICLES))
	m_color [1].Alpha () /= 2.0f;
else if (nParticleSystemType == GATLING_PARTICLES)
	m_color [1].Alpha () /= 4.0f;
if (m_bEmissive)
	m_color [0].Alpha () = (m_nFadeTime > 0) ? 0.8f + RandFloat (5.0f) : 1.0f;
else
	m_color [0].Alpha () = m_color [1].Alpha ();
}

//------------------------------------------------------------------------------

int CParticle::InitDrift (CFixVector* vDir, int nSpeed)
{
#if 0
if (nType == FIRE_PARTICLES)
nSpeed = int (sqrt (double (nSpeed)) * float (I2X (1)));
else
#endif
nSpeed *= I2X (1);
if (!vDir) {
	m_vDrift.v.coord.x = nSpeed - RandN (2 * nSpeed);
	m_vDrift.v.coord.y = nSpeed - RandN (2 * nSpeed);
	m_vDrift.v.coord.z = nSpeed - RandN (2 * nSpeed);
	m_vDir.SetZero ();
	m_bHaveDir = 1;
	}
else {
	m_vDir = *vDir;

	if (m_nType == RAIN_PARTICLES)
		m_vDrift = m_vDir;
	else {
#if 1
		m_vDrift.v.coord.x = RandN (I2X (1) / 4) - I2X (1) / 8;
		m_vDrift.v.coord.y = RandN (I2X (1) / 4) - I2X (1) / 8;
		m_vDrift.v.coord.z = RandN (I2X (1) / 4) - I2X (1) / 8;
		m_vDrift += m_vDir;
#else
		CAngleVector a;
		CFixMatrix m;
		a.v.coord.p = RandN (I2X (1) / 4) - I2X (1) / 8;
		a.v.coord.b = RandN (I2X (1) / 4) - I2X (1) / 8;
		a.v.coord.h = RandN (I2X (1) / 4) - I2X (1) / 8;
		m = CFixMatrix::Create (a);
		if (m_nType == WATERFALL_PARTICLES)
			CFixVector::Normalize (m_vDir);
		m_vDrift = m * m_vDir;
#endif
		CFixVector::Normalize (m_vDrift);
		if (m_nType == WATERFALL_PARTICLES) {
			fix dot = CFixVector::Dot (m_vDir, m_vDrift);
			if (dot < I2X (1) / 2)
				return 0;
			}
		float d = float (CFixVector::DeltaAngle (m_vDrift, m_vDir, NULL));
		if (d) {
			d = (float) exp ((I2X (1) / 8) / d);
			nSpeed = (fix) ((float) nSpeed / d);
			}
		}
	m_vDrift *= nSpeed;
	if (m_nType <= FIRE_PARTICLES)
		m_vDir *= (I2X (3) / 4 + I2X (RandN (16)) / 64);
	m_bHaveDir = 1;
	}
return 1;
}

//------------------------------------------------------------------------------

void CParticle::InitPosition (CFixVector* vPos, CFixVector* vEmittingFace, CFixMatrix *mOrient)
{
if (vEmittingFace)
	m_vPos = *RandomPointOnQuad (vEmittingFace, vPos);
else if ((m_nType != BUBBLE_PARTICLES) && (m_nType != RAIN_PARTICLES) && (m_nType != SNOW_PARTICLES))
	m_vPos = *vPos + m_vDrift * (I2X (1) / 64);
else {
	//m_vPos = *vPos + vDrift * (I2X (1) / 32);
	int nSpeed = m_vDrift.Mag () / 16;
	CFixVector v = CFixVector::Avg ((*mOrient).m.dir.r * (nSpeed - RandN (2 * nSpeed)), (*mOrient).m.dir.u * (nSpeed - RandN (2 * nSpeed)));
	m_vPos = *vPos + v + (*mOrient).m.dir.f * (I2X (1) / 2 - RandN (I2X (1)));
	}
m_vStartPos = m_vPos;
return m_vPos != null;
}

//------------------------------------------------------------------------------

void CParticle::InitSize (float nScale, CFixMatrix *mOrient)
{
if (nScale < 0)
	m_nRad = float (-nScale);
else if (gameOpts->render.particles.bSyncSizes)
	m_nRad = float (PARTICLE_SIZE (gameOpts->render.particles.nSize [0], nScale, m_bBlowUp));
else
	m_nRad = float (nScale);
if (!m_nRad)
	m_nRad = 1.0f;

if ((m_nType == BUBBLE_PARTICLES) || (m_nType == SNOW_PARTICLES))
	m_nRad = m_nRad / 20 + float (RandN (int (9 * m_nRad / 20)));
else {
	if (m_nType <= SMOKE_PARTICLES) {
		if (m_bBlowUp)
			m_nLife = 2 * m_nLife / 3;
		m_nLife = 4 * m_nLife / 5 + RandN (2 * m_nLife / 5);
		m_nRad += float (RandN (int (m_nRad)));
		}
	else if (m_nType == FIRE_PARTICLES) {
		m_nLife = 3 * m_nLife / 4 + RandN (m_nLife / 4);
		m_nRad += float (RandN (int (m_nRad)));
		}
	else
		m_nRad *= 2;
	if (mOrient) {
		static CFixMatrix mRot [9 * 9];
		static char bInit = 1;

		if (bInit) {
			bInit = 0;
			for (int p = 0; p < 9; p++) { 
				for (int h = 0; h < 9; h++) {
					CAngleVector vRot;
					vRot.v.coord.b = 0;
					vRot.v.coord.p = 2048 - (p * 512);
					vRot.v.coord.h = 2048 - (h * 512);
					mRot [p * 9 + h] = CFixMatrix::Create (vRot);
					}
				}
			}
		m_mOrient = *mOrient * mRot [(RandShort () % 9) * 9 + (RandShort () % 9)];
		}
	}

if (m_bBlowUp) {
	m_nWidth = (m_nType == WATERFALL_PARTICLES) 
				  ? m_nRad * 0.6666667f
				  : m_nRad;
	m_nHeight = m_nRad;
	}
else {
	m_nWidth = (m_nType == WATERFALL_PARTICLES) 
				  ? m_nRad * 0.3333333f
			     : m_nRad * 2;
	m_nHeight = m_nRad * 2;
	}
m_nWidth /= 65536.0f;
m_nHeight /= 65536.0f;
m_nRad /= 65536.0f;
}

//------------------------------------------------------------------------------

void CParticle::InitAnimation (void)
{
m_nFrames = ParticleImageInfo (m_nType).nFrames;
m_deltaUV = 1.0f / float (m_nFrames);
if (m_nType == BULLET_PARTICLES) {
	m_iFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 3;
	}
else if ((m_nType == RAIN_PARTICLES) || (m_nType == SNOW_PARTICLES)) {
	m_iFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 1;
	}
else if (m_nType == BUBBLE_PARTICLES) {
	m_iFrame = rand () % (m_nFrames * m_nFrames);
	m_nRotFrame = 0;
	m_nOrient = 0;
	}
else if ((m_nType == LIGHT_PARTICLES) || (m_nType == GATLING_PARTICLES)) {
	m_iFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 0;
	}
else if (m_nType == FIRE_PARTICLES) {
	m_iFrame = (rand () % 10 < 6) ? 0 : 2; // more fire than smoke (60:40)
	if (m_iFrame < 2)
		m_nLife = 9 * m_nLife / 10;
	else
		m_nLife = 10 * m_nLife / 9;
	m_nRotFrame = rand () % PARTICLE_POSITIONS;
	m_nOrient = rand () % 4;
	}
else {
	m_iFrame = rand () % (m_nFrames * m_nFrames);
	m_nRotFrame = rand () % PARTICLE_POSITIONS;
	m_nOrient = rand () % 4;
	}
m_bAnimate = (m_nType != FIRE_PARTICLES) && (gameOpts->render.particles.nQuality > 1) && (m_nFrames > 1);
m_bRotate = ((m_nRenderType <= SMOKE_PARTICLES) || (m_nRenderType == SNOW_PARTICLES)) ? 1 : (m_nRenderType == FIRE_PARTICLES + PARTICLE_TYPES) ? -1 : 0;
}

//------------------------------------------------------------------------------

static int bounceFlags [PARTICLE_TYPES] = {2,2,1,1,1,2,1,2,2,2};

int CParticle::Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
							  short nSegment, int nLife, int nSpeed, char nParticleSystemType,
							  char nClass, float nScale, CFloatVector *colorP, int nCurTime,
							  int bBlowUp, char nFadeType, float fBrightness,
							  CFixVector *vEmittingFace) 
{
	int nType = particleImageManager.GetType (nParticleSystemType);

m_bChecked = 0;
m_bBlowUp = bBlowUp && gameOpts->render.particles.bDisperse;
m_nType = nType;
m_nClass = nClass;
m_nFadeType = nFadeType;
m_nSegment = nSegment;
m_bSkybox = SEGMENTS [nSegment].Function () == SEGMENT_FUNC_SKYBOX;
#if DBG
if (nSegment < 0)
	nSegment = nSegment;
#endif
m_nBounce = bounceFlags [(int) m_nType];
m_bReversed = 0;
m_nUpdated = m_nMoved = nCurTime;
if (nLife < 0)
	nLife = -nLife;
m_nLife = nLife;
m_nDelay = 0; //bStart ? RandN (nLife) : 0;
m_nRenderType = RenderType ();

#if 0

InitColor (colorP, fBrightness, nParticleSystemType);
if (!InitDrift (vDir, nSpeed))
	return 0;
if (!InitPosition (vPos, vEmittingFace, mOrient)) // needs InitDrift() to be executed first!
	return 0;
InitSize (nScale, mOrient);
InitAnimation ();

#else

// init color

	CFloatVector color;

m_bEmissive = (nParticleSystemType == LIGHT_PARTICLES) 
					? 1
					: (nParticleSystemType == FIRE_PARTICLES) 
						? 2
						//: ((nParticleSystemType <= SMOKE_PARTICLES) || (nParticleSystemType <= WATERFALL_PARTICLES))
						//	? 3
						: 0;
m_color [0] = m_color [1] = color = (colorP && (m_bEmissive != 2)) ? *colorP : defaultParticleColor;

if (!brightFlags [(int) m_nType]) {
	m_bBright = 0;
	m_nFadeTime = -1;
	if (colorP && (colorP->Alpha () < 0)) {
		ubyte a = ubyte (-colorP->Alpha () * 255.0f * 0.25f + 0.5f);
		m_color [1].Alpha () = float (3 * a + RandN (2 * a)) / 255.0f;
		}
	}
else {
	m_bBright = (m_nType <= SMOKE_PARTICLES) ? (rand () % 50) == 0 : 0;
	if (colorP) {
		if (!m_bEmissive /*|| (m_bEmissive == 3)*/) {
			m_color [0].Red () *= RANDOM_FADE;
			m_color [0].Green () *= RANDOM_FADE;
			m_color [0].Blue () *= RANDOM_FADE;
			}
		m_nFadeTime = 0;
		} 
	else {
		colorP = &m_color [0];
		m_color [0].Alpha () = 2.0f;
		}
	if (m_bEmissive /*&& (m_bEmissive < 3)*/)
		; // m_color [0].Alpha () = float (SMOKE_START_ALPHA + 64) / 255.0f;
	else if (nParticleSystemType != GATLING_PARTICLES) {
		if (!colorP)
			m_color [1].Alpha () = SmokeStartAlpha (m_bBlowUp, m_nClass);
		else if (colorP->Alpha () < 0) {
			ubyte a = ubyte (-colorP->Alpha () * 255.0f * 0.25f + 0.5f);
			m_color [1].Alpha () = float (3 * a + RandN (2 * a)) / 255.0f;
			} 
		else if (char (colorP->Alpha ()) != 2.0f) 
			m_color [1].Alpha () = SmokeStartAlpha (m_bBlowUp, m_nClass);
		else {
			if ((m_bEmissive = (gameOpts->render.particles.nQuality > 2))) {
				m_color [0].Red () = 0.5f + RandFloat (2.0f);
				m_color [0].Green () = m_color [0].Red () * (0.5f + RandFloat (2.0f));
				}
			else {
#if 1
				m_color [0].Red () = 
				m_color [0].Green () = 1.0;
#else
				m_color [0].Red () = 0.9f + RandFloat (10.0f);
				m_color [0].Green () = m_color [0].Red () * (0.9f + RandFloat (10.0f));
#endif
				}
			m_color [0].Blue () = 0.0f;
			m_nFadeTime = 50 + rand () % 150;
			m_color [1].Red () *= RANDOM_FADE;
			m_color [1].Green () *= RANDOM_FADE;
			m_color [1].Blue () *= RANDOM_FADE;
			m_nWidth *= 0.75;
			m_nHeight *= 0.75;
			m_color [1].Alpha () = SmokeStartAlpha (m_bBlowUp, m_nClass);
			}
		if (m_bBlowUp && !m_bBright) {
			fBrightness = 1.0f - fBrightness;
			if (m_nFadeTime <= 0)
				m_color [1].Alpha () += fBrightness * fBrightness / 8.0f;
			}
		}
	}

if (nParticleSystemType == SIMPLE_SMOKE_PARTICLES)
	m_color [1].Alpha () /= 3.5f - float (1 + int (gameOpts->render.particles.nQuality > 1)) / 2.0f; //colorP ? 2.0f + (color.Red () + color.Green () + color.Blue ()) / 3.0f : 2.0f;
else if (nParticleSystemType == SMOKE_PARTICLES)
	m_color [1].Alpha () /= colorP ? 3.0f - (color.Red () + color.Green () + color.Blue ()) / 3.0f : 2.5f;
else if ((nParticleSystemType == BUBBLE_PARTICLES) || (nParticleSystemType == RAIN_PARTICLES) || (nParticleSystemType == SNOW_PARTICLES))
	m_color [1].Alpha () /= 2.0f;
else if (nParticleSystemType == GATLING_PARTICLES)
	m_color [1].Alpha () /= 4.0f;
if (m_bEmissive)
	m_color [0].Alpha () = (m_nFadeTime > 0) ? 0.8f + RandFloat (5.0f) : 1.0f;
else
	m_color [0].Alpha () = m_color [1].Alpha ();


// init drift

#if 0
if (nType == FIRE_PARTICLES)
nSpeed = int (sqrt (double (nSpeed)) * float (I2X (1)));
else
#endif
nSpeed *= I2X (1);
if (!vDir) {
	m_vDrift.v.coord.x = nSpeed - RandN (2 * nSpeed);
	m_vDrift.v.coord.y = nSpeed - RandN (2 * nSpeed);
	m_vDrift.v.coord.z = nSpeed - RandN (2 * nSpeed);
	m_vDir.SetZero ();
	m_bHaveDir = 1;
	}
else {
	m_vDir = *vDir;

	if (m_nType == RAIN_PARTICLES)
		m_vDrift = m_vDir;
	else {
#if 1
		m_vDrift.v.coord.x = RandN (I2X (1) / 4) - I2X (1) / 8;
		m_vDrift.v.coord.y = RandN (I2X (1) / 4) - I2X (1) / 8;
		m_vDrift.v.coord.z = RandN (I2X (1) / 4) - I2X (1) / 8;
		m_vDrift += m_vDir;
#else
		CAngleVector a;
		CFixMatrix m;
		a.v.coord.p = RandN (I2X (1) / 4) - I2X (1) / 8;
		a.v.coord.b = RandN (I2X (1) / 4) - I2X (1) / 8;
		a.v.coord.h = RandN (I2X (1) / 4) - I2X (1) / 8;
		m = CFixMatrix::Create (a);
		if (m_nType == WATERFALL_PARTICLES)
			CFixVector::Normalize (m_vDir);
		m_vDrift = m * m_vDir;
#endif
		CFixVector::Normalize (m_vDrift);
		if (m_nType == WATERFALL_PARTICLES) {
			fix dot = CFixVector::Dot (m_vDir, m_vDrift);
			if (dot < I2X (1) / 2)
				return 0;
			}
		float d = float (CFixVector::DeltaAngle (m_vDrift, m_vDir, NULL));
		if (d) {
			d = (float) exp ((I2X (1) / 8) / d);
			nSpeed = (fix) ((float) nSpeed / d);
			}
		}
	m_vDrift *= nSpeed;
	if (m_nType <= FIRE_PARTICLES)
		m_vDir *= (I2X (3) / 4 + I2X (RandN (16)) / 64);
	m_bHaveDir = 1;
	}

// init position

if (vEmittingFace)
	m_vPos = *RandomPointOnQuad (vEmittingFace, vPos);
else if ((m_nType != BUBBLE_PARTICLES) && (m_nType != RAIN_PARTICLES) && (m_nType != SNOW_PARTICLES))
	m_vPos = *vPos + m_vDrift * (I2X (1) / 64);
else {
	//m_vPos = *vPos + vDrift * (I2X (1) / 32);
	int nSpeed = m_vDrift.Mag () / 16;
	CFixVector v = CFixVector::Avg ((*mOrient).m.dir.r * (nSpeed - RandN (2 * nSpeed)), (*mOrient).m.dir.u * (nSpeed - RandN (2 * nSpeed)));
	m_vPos = *vPos + v + (*mOrient).m.dir.f * (I2X (1) / 2 - RandN (I2X (1)));
	}
if (!m_vPos)
	return 0;
m_vStartPos = m_vPos;

// init size
if (nScale < 0)
	m_nRad = float (-nScale);
else if (gameOpts->render.particles.bSyncSizes)
	m_nRad = float (PARTICLE_SIZE (gameOpts->render.particles.nSize [0], nScale, m_bBlowUp));
else
	m_nRad = float (nScale);
if (!m_nRad)
	m_nRad = 1.0f;

if ((m_nType == BUBBLE_PARTICLES) || (m_nType == SNOW_PARTICLES))
	m_nRad = m_nRad / 20 + float (RandN (int (9 * m_nRad / 20)));
else {
	if (m_nType <= SMOKE_PARTICLES) {
		if (m_bBlowUp)
			m_nLife = 2 * m_nLife / 3;
		m_nLife = 4 * m_nLife / 5 + RandN (2 * m_nLife / 5);
		m_nRad += float (RandN (int (m_nRad)));
		}
	else if (m_nType == FIRE_PARTICLES) {
		m_nLife = 3 * m_nLife / 4 + RandN (m_nLife / 4);
		m_nRad += float (RandN (int (m_nRad)));
		}
	else
		m_nRad *= 2;
	if (mOrient) {
		static CFixMatrix mRot [9 * 9];
		static char bInit = 1;

		if (bInit) {
			bInit = 0;
			for (int p = 0; p < 9; p++) { 
				for (int h = 0; h < 9; h++) {
					CAngleVector vRot;
					vRot.v.coord.b = 0;
					vRot.v.coord.p = 2048 - (p * 512);
					vRot.v.coord.h = 2048 - (h * 512);
					mRot [p * 9 + h] = CFixMatrix::Create (vRot);
					}
				}
			}
		m_mOrient = *mOrient * mRot [(RandShort () % 9) * 9 + (RandShort () % 9)];
		}
	}

if (m_bBlowUp) {
	m_nWidth = (m_nType == WATERFALL_PARTICLES) 
				  ? m_nRad * 0.6666667f
				  : m_nRad;
	m_nHeight = m_nRad;
	}
else {
	m_nWidth = (m_nType == WATERFALL_PARTICLES) 
				  ? m_nRad * 0.3333333f
			     : m_nRad * 2;
	m_nHeight = m_nRad * 2;
	}
m_nWidth /= 65536.0f;
m_nHeight /= 65536.0f;
m_nRad /= 65536.0f;

// init animation
m_nFrames = ParticleImageInfo (m_nType).nFrames;
m_deltaUV = 1.0f / float (m_nFrames);
if (m_nType == BULLET_PARTICLES) {
	m_iFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 3;
	}
else if ((m_nType == RAIN_PARTICLES) || (m_nType == SNOW_PARTICLES)) {
	m_iFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 1;
	}
else if (m_nType == BUBBLE_PARTICLES) {
	m_iFrame = rand () % (m_nFrames * m_nFrames);
	m_nRotFrame = 0;
	m_nOrient = 0;
	}
else if ((m_nType == LIGHT_PARTICLES) || (m_nType == GATLING_PARTICLES)) {
	m_iFrame = 0;
	m_nRotFrame = 0;
	m_nOrient = 0;
	}
else if (m_nType == FIRE_PARTICLES) {
	m_iFrame = (rand () % 10 < 6) ? 0 : 2; // more fire than smoke (60:40)
	if (m_iFrame < 2)
		m_nLife = 9 * m_nLife / 10;
	else
		m_nLife = 10 * m_nLife / 9;
	m_nRotFrame = rand () % PARTICLE_POSITIONS;
	m_nOrient = rand () % 4;
	}
else {
	m_iFrame = rand () % (m_nFrames * m_nFrames);
	m_nRotFrame = rand () % PARTICLE_POSITIONS;
	m_nOrient = rand () % 4;
	}
m_bAnimate = (m_nType != FIRE_PARTICLES) && (gameOpts->render.particles.nQuality > 1) && (m_nFrames > 1);
m_bRotate = ((m_nRenderType <= SMOKE_PARTICLES) || (m_nRenderType == SNOW_PARTICLES)) ? 1 : (m_nRenderType == FIRE_PARTICLES + PARTICLE_TYPES) ? -1 : 0;

#endif

m_nTTL = m_nLife; // may have been changed during initialization

UpdateDecay ();
UpdateTexCoord ();
SetupColor (fBrightness);
m_nDelayPosUpdate = -2;
return 1;
}

//------------------------------------------------------------------------------

bool CParticle::IsVisible (int nThread) 
{
#if 0
	return gameData.render.mine.Visible (m_nSegment);
#else
if ((m_nSegment < 0) || (m_nSegment >= gameData.segs.nSegments))
	return false;
if (gameData.render.mine.Visible (m_nSegment))
	return true;
short* childP = SEGMENTS [m_nSegment].m_children;
for (int i = 6; i; i--, childP++) {
	if ((*childP >= 0) && (gameData.render.mine.Visible (*childP)))
		return true;
	}
#if 1
int nSegment = FindSegByPosExhaustive (m_vPos, m_bSkybox, m_nSegment);
#else
int nSegment = FindSegByPos (m_vPos, m_nSegment, 0, 0, 0, nThread);
#endif
if (nSegment < 0)
	return false;
m_nSegment = nSegment;
return gameData.render.mine.Visible (nSegment);
#endif
}

//------------------------------------------------------------------------------

inline int CParticle::ChangeDir (int d) 
{
	int h = d;

if (h)
	h = h / 2 - RandN (h);
return (d * 10 + h) / 10;
}

//------------------------------------------------------------------------------

int nPartSeg [MAX_THREADS] = { -1, -1, -1, -1 }; //, -1, -1, -1, -1};

static int nFaceCount [MAX_THREADS][6];
static int bSidePokesOut [MAX_THREADS][6];
//static int nVert [6];
static CFixVector* wallNorm [MAX_THREADS];

int CParticle::CollideWithWall (int nThread) 
{
if (m_nSegment < 0)
	return 0;

	CSegment* segP;
	CSide* sideP;
	int bInit, nSide, nChild, nFace, nFaces, nInFront;
	fix nDist;

	//redo:

segP = SEGMENTS + m_nSegment;
if ((bInit = (m_nSegment != nPartSeg [nThread])))
	nPartSeg [nThread] = m_nSegment;
for (nSide = 0, sideP = segP->m_sides; nSide < SEGMENT_SIDE_COUNT; nSide++, sideP++) {
	if (bInit) {
		bSidePokesOut [nThread][nSide] = !sideP->IsPlanar ();
		nFaceCount [nThread][nSide] = sideP->m_nFaces;
		}
	nFaces = nFaceCount [nThread][nSide];
	for (nFace = nInFront = 0; nFace < nFaces; nFace++) {
		nDist = m_vPos.DistToPlane (sideP->m_normals [nFace],	gameData.segs.vertices [sideP->m_nMinVertex [0]]);
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
	if (m_nFadeTime > 0) {
#if 1
		if (m_nTTL - m_nLife < m_nFadeTime) {
			m_color [0].Red () *= 0.95f;
			m_color [0].Green () *= 0.8f;
			}
		else {
			m_color [0] = m_color [1];
			m_nWidth *= 1.25;
			m_nHeight *= 1.25;
			m_bEmissive = false;
			m_nFadeTime = -1;
			}
#else
		if (m_color [0].Green () < m_color [1].Green ()) {
#if SMOKE_SLOWMO
			m_color [0].Green () += 1.0f / 40.0f / (float) gameStates.gameplay.slowmo [0].fSpeed;
#else
			m_color [0].Green () += 1.0f / 40.0f;
#endif
			if (m_color [0].Green () > m_color [1].Green ()) {
				m_color [0].Green () = m_color [1].Green ();
				m_nFadeTime--;
				}
			}
		if (m_color [0].Blue () < m_color [1].Blue ()) {
#if SMOKE_SLOWMO
			m_color [0].Blue () += 1.0f / 20.0f / (float) gameStates.gameplay.slowmo [0].fSpeed;
#else
			m_color [0].Blue () += 1.0f / 20.0f;
#endif
			if (m_color [0].Blue () > m_color [1].Blue ()) {
				m_color [0].Blue () = m_color [1].Blue ();
				m_nFadeTime--;
				}
			}
#endif
		}
	else if (m_nFadeTime == 0) {
		m_color [0].Red () = m_color [1].Red () * RANDOM_FADE;
		m_color [0].Green () = m_color [1].Green () * RANDOM_FADE;
		m_color [0].Blue () = m_color [1].Blue () * RANDOM_FADE;
		m_nFadeTime = -1;
	}
#if SMOKE_LIGHTING //> 1
	if (gameOpts->render.particles.nQuality > 2) {
		if (0 <= (m_nSegment = FindSegByPos (m_vPos, m_nSegment, 0, 0, 0, nThread))) {
			CFaceColor* colorP = lightManager.AvgSgmColor (m_nSegment, NULL, nThread);
			m_color [0].Red () *= colorP->Red ();
			m_color [0].Green () *= colorP->Green ();
			m_color [0].Blue () *= colorP->Blue ();
			}
		}
#endif
	}
SetupColor (fBrightness);
}

//------------------------------------------------------------------------------

fix CParticle::Drag (void) 
{
if ((m_nType == BUBBLE_PARTICLES) || (m_nType == RAIN_PARTICLES) || (m_nType == SNOW_PARTICLES) || (m_nType == FIRE_PARTICLES))
	return I2X (1); // constant speed
if (m_nType == WATERFALL_PARTICLES) {
	float h = 4.0f - 3.0f * m_decay;
	h *= h;
	return F2X (h);
	} 
return I2X (1); //F2X (m_decay); // decelerate
}

//------------------------------------------------------------------------------

int CParticle::Bounce (int nThread) 
{
if (!gameOpts->render.particles.bCollisions)
	return 1;
if (!CollideWithWall (nThread)) {
	m_nBounce = ((m_nType == BUBBLE_PARTICLES) || (m_nType == RAIN_PARTICLES) || (m_nType == SNOW_PARTICLES) || (m_nType == WATERFALL_PARTICLES)) ? 1 : 2;
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

int CParticle::UpdateDrift (int nCurTime, int nThread) 
{
fix t = nCurTime - m_nUpdated;
m_nUpdated = nCurTime;
m_vPos += m_vDrift * t; // (I2X (t) / 1000);

#if 0 //DBG
CFixVector vDrift = m_vDrift;
CFixVector::Normalize (vDrift);
if (CFixVector::Dot (vDrift, m_vDir) < 0)
	t = t;
#endif

if ((m_nType <= SMOKE_PARTICLES) || (m_nType == FIRE_PARTICLES)) {
	m_vDrift.v.coord.x = ChangeDir (m_vDrift.v.coord.x);
	m_vDrift.v.coord.y = ChangeDir (m_vDrift.v.coord.y);
	m_vDrift.v.coord.z = ChangeDir (m_vDrift.v.coord.z);
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

if (nCurTime - m_nMoved < 250) 
	return 1;
m_nMoved = nCurTime;

int nSegment = m_nSegment;

if (m_nSegment < -1)
	m_nSegment++;
if (m_nSegment >= -1) {
#if 1 
	nSegment = FindSegByPosExhaustive (m_vPos, m_bSkybox, m_nSegment);
#else
	nSegment = FindSegByPos (m_vPos, m_nSegment, 1, -1, ((m_nType == BUBBLE_PARTICLES) || (m_nType == RAIN_PARTICLES) || (m_nType == SNOW_PARTICLES)) ? 0 : fix (m_nRad), nThread);
#endif
	if (nSegment < 0)
		m_nSegment = int (--m_nDelayPosUpdate);
	}

if (nSegment < 0) {
	if (m_nType == WATERFALL_PARTICLES) {
		if (!m_bChecked) {
			CFixVector vDir = m_vPos - m_vStartPos;
			if ((CFixVector::Normalize (vDir) >= I2X (1)) && (CFixVector::Dot (vDir, m_vDir) < I2X (1) / 2)) {
				m_nLife = -1;
				return 0;
				}
			m_bChecked = 1;
			m_nLife = 500;
			}
		}
	if (m_nTTL - m_nLife > 500) {
		m_nLife = -1;
		return 0;
		}
	}
else if (m_nType == BUBBLE_PARTICLES) {
	if (!SEGMENTS [nSegment].HasWaterProp ()) {
		m_nLife = -1;
		return 0;
		}
	}
else if ((m_nType == RAIN_PARTICLES) || (m_nType == SNOW_PARTICLES)) {
	if (SEGMENTS [nSegment].HasWaterProp () || SEGMENTS [nSegment].HasLavaProp ()) {
		m_nLife = -1;
		return 0;
		}
#if 1
	if ((m_nSegment >= 0) && (nSegment != m_nSegment) && SEGMENTS [m_nSegment].HasFunction (SEGMENT_FUNC_SKYBOX)) {
		if (m_nTTL - m_nLife > 500) {
			m_nLife = -1;
			return 0;
			}
		nSegment = m_nSegment;
		}
#endif
	}
m_nSegment = nSegment;

if (!Bounce (nThread))
	return 0;

return 1;
}

//------------------------------------------------------------------------------

void CParticle::UpdateDecay (void)
{
if ((m_nType == BUBBLE_PARTICLES) || (m_nType == RAIN_PARTICLES) || (m_nType == SNOW_PARTICLES) || (m_nType == WATERFALL_PARTICLES))
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
if ((m_nLife <= 0) /*|| (m_color [0].Alpha () < 0.01f)*/)
	return 0;

fix t = nCurTime - m_nUpdated;

#if !ENABLE_UPDATE 
m_nLife -= t;
m_decay = ((m_nType == BUBBLE_PARTICLES) || (m_nType == WATERFALL_PARTICLES)) ? 1.0f : float (m_nLife) / float (m_nTTL);
#else
UpdateColor (fBrightness, nThread);
if (m_nLife < 0)
	return 1;

if (m_nDelay > 0) {
	m_nUpdated = nCurTime;
	m_nDelay -= t;
	return 1;
	}

if (!UpdateDrift (nCurTime, nThread))
	return 0;
if (m_nLife < 0)
	return 1;

#if SMOKE_SLOWMO
m_nLife -= (int) (t / gameStates.gameplay.slowmo [0].fSpeed);
#else
m_nLife -= t;
#	if 0
if ((m_nType == FIRE_PARTICLES) && !m_bReversed && (m_nLife <= m_nTTL / 4 + RandN (m_nTTL / 4))) {
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
			m_color [0].Alpha () *= (1.0f + 0.0725f / m_bBlowUp);
			if (m_color [0].Alpha () > 1)
				m_color [0].Alpha () = 1;
			}
		} 
	else {
		if (m_nWidth <= m_nRad)
			m_nRad = 0;
		else {
			m_nRad *= 1.2f;
			m_color [0].Alpha () *= 1.0725f;
			if (m_color [0].Alpha () > 1)
				m_color [0].Alpha () = 1;
			}
		}
	}
#endif
return 1;
}

//------------------------------------------------------------------------------

int CParticle::RenderType (void) 
{
#if 0
return m_nType;
#else
if ((m_nType != FIRE_PARTICLES) || /* (gameOpts->render.particles.nQuality < 2) ||*/ (m_iFrame < m_nFrames))
	return (m_nType == SNOW_PARTICLES) ? SIMPLE_SMOKE_PARTICLES : m_nType;
return PARTICLE_TYPES + m_nType;
#endif
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
if (m_nType == BUBBLE_PARTICLES)
	return 0;
#endif

#if !ENABLE_RENDER
	return 1;
#else

PROF_START

#if LAZY_RENDER_SETUP
bool bFlushed = particleManager.Add (this, fBrightness);
#else
bool bFlushed = false;
Setup (fBrightness, m_iFrame, m_nRotFrame, particleRenderBuffer + particleManager.BufPtr () * 4, 0);
#endif

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

PROF_END (ptParticles)
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
	m_renderColor.Red () *= fBrightness;
	m_renderColor.Green () *= fBrightness;
	m_renderColor.Blue () *= fBrightness;
	}

float fFade;

if (m_nFadeType == 0) { // default (start fully visible, fade out)
#if 1 
	m_renderColor.Alpha () *= m_decay; // * 0.6f;
#else
	m_renderColor.Alpha () *= float (cos (double (sqr (1.0f - m_decay)) * Pi) * 0.5 + 0.5) * 0.6f;
#endif
	}
else if (m_nFadeType == 1) { // quickly fade in, then gently fade out
	m_renderColor.Alpha () *= float (
			sin (double (sqr (1.0f - m_decay)) * Pi * 1.5) * 0.5 + 0.5);
	if (m_decay >= 0.666f)
		return 1;
} else if (m_nFadeType == 2) { // fade in, then gently fade out
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
	m_renderColor.Alpha () *= fFade;
	if (m_decay >= 0.75f)
		return 1;
	}
else if (m_nFadeType == 3) { // fire (additive, blend in)
#if 0
	if (m_decay > 0.5f)
	fFade = 2.0f * (1.0f - m_decay);
	else
	fFade = m_decay * 2.0f;
	if ((m_decay < 0.5f) && (fFade < 0.00333f)) {
		m_nLife = -1;
		return 0;
	}
	m_renderColor.Red () =
	m_renderColor.Green () =
	m_renderColor.Blue () = fFade;
	m_color [0] = m_renderColor;
#endif
	return 1;
	}
else if (m_nFadeType == 4) { // light trail (additive, constant effect)
	m_renderColor.Red () /= 50.0f;
	m_renderColor.Green () /= 50.0f;
	m_renderColor.Blue () /= 50.0f;
	return 1;
	}
if (m_renderColor.Alpha () >= 0.01f) //1.0 / 255.0
	return 1;
m_nLife = -1;
return 0;
}

//------------------------------------------------------------------------------

#if TRANSFORM_PARTICLE_VERTICES

void CParticle::Setup (float fBrightness, char nFrame, char nRotFrame, tParticleVertex* pb, int nThread)
{
	CFloatVector3 vCenter, vOffset;

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
						: (m_decay > 0.9f) // start from zero size by scaling with pow (m_decay, 44f) which is < 0.01 for m_decay == 0.9f
							? (1.0f - pow (m_decay, 44.0f)) / float (pow (m_decay, 0.3333333f))
							: 1.0f / float (pow (m_decay, 0.3333333f));
#endif
	vOffset.dir.coord.x = m_nWidth * fFade;
	vOffset.dir.coord.y = m_nHeight * fFade;
	}
else {
	vOffset.dir.coord.x = m_nWidth * m_decay;
	vOffset.dir.coord.y = m_nHeight * m_decay;
	}
vOffset.dir.coord.z = 0;

float h = ParticleImageInfo (m_nType).xBorder;
pb [m_nOrient].texCoord.dir.u =
pb [(m_nOrient + 3) % 4].texCoord.dir.u = m_texCoord.dir.u + h;
pb [(m_nOrient + 1) % 4].texCoord.dir.u =
pb [(m_nOrient + 2) % 4].texCoord.dir.u = m_texCoord.dir.u + m_deltaUV - h;
h = ParticleImageInfo (m_nType).yBorder;
pb [m_nOrient].texCoord.dir.dir =
pb [(m_nOrient + 1) % 4].texCoord.dir.dir = m_texCoord.dir.dir + h;
pb [(m_nOrient + 2) % 4].texCoord.dir.dir =
pb [(m_nOrient + 3) % 4].texCoord.dir.dir = m_texCoord.dir.dir + m_deltaUV - h;

pb [0].color =
pb [1].color =
pb [2].color =
pb [3].color = m_renderColor;

if ((m_nType == BUBBLE_PARTICLES) && gameOpts->render.particles.bWiggleBubbles)
vCenter.dir.coord.x += (float) sin (nFrame / 4.0f * Pi) / (10 + rand () % 6);
if (m_bRotate && gameOpts->render.particles.bRotate) {
	int i = (m_nOrient & 1) ? 63 - m_nRotFrame : m_nRotFrame;
	vOffset.dir.coord.x *= vRot [i].dir.coord.x;
	vOffset.dir.coord.y *= vRot [i].dir.coord.y;

	pb [0].vertex.dir.coord.x = vCenter.dir.coord.x - vOffset.dir.coord.x;
	pb [0].vertex.dir.coord.y = vCenter.dir.coord.y + vOffset.dir.coord.y;
	pb [1].vertex.dir.coord.x = vCenter.dir.coord.x + vOffset.dir.coord.y;
	pb [1].vertex.dir.coord.y = vCenter.dir.coord.y + vOffset.dir.coord.x;
	pb [2].vertex.dir.coord.x = vCenter.dir.coord.x + vOffset.dir.coord.x;
	pb [2].vertex.dir.coord.y = vCenter.dir.coord.y - vOffset.dir.coord.y;
	pb [3].vertex.dir.coord.x = vCenter.dir.coord.x - vOffset.dir.coord.y;
	pb [3].vertex.dir.coord.y = vCenter.dir.coord.y - vOffset.dir.coord.x;
	}
else {
	pb [0].vertex.dir.coord.x =
	pb [3].vertex.dir.coord.x = vCenter.dir.coord.x - vOffset.dir.coord.x;
	pb [1].vertex.dir.coord.x =
	pb [2].vertex.dir.coord.x = vCenter.dir.coord.x + vOffset.dir.coord.x;
	pb [0].vertex.dir.coord.y =
	pb [1].vertex.dir.coord.y = vCenter.dir.coord.y + vOffset.dir.coord.y;
	pb [2].vertex.dir.coord.y =
	pb [3].vertex.dir.coord.y = vCenter.dir.coord.y - vOffset.dir.coord.y;
	}
pb [0].vertex.dir.coord.z =
pb [1].vertex.dir.coord.z =
pb [2].vertex.dir.coord.z =
pb [3].vertex.dir.coord.z = vCenter.dir.coord.z;
}

#else // -----------------------------------------------------------------------

void CParticle::Setup (bool alphaControl, float fBrightness, char nFrame, char nRotFrame, tParticleVertex* pb, int nThread) 
{
	CFloatVector3 vCenter, uVec, rVec;
	float fScale;

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
				: (m_decay > 0.9f) // start from zero size by scaling with pow (m_decay, 44f) which is < 0.01 for m_decay == 0.9f
					? (1.0f - pow (m_decay, 44.0f)) / float (pow (m_decay, 0.3333333f))
					: 1.0f / float (pow (m_decay, 0.3333333f));
#endif
	}
else {
	fScale = m_decay;
	}

pb [0].color = m_renderColor;
if (m_bEmissive && alphaControl)
	pb [0].color.Alpha () = 0.0f;
pb [1].color = pb [2].color = pb [3].color = pb [0].color;

if (m_bEmissive < 0) {
	m_bEmissive = 1;
	m_nType = SPARK_PARTICLES;

	float nRow = m_texCoord.v.u;
	float nCol = m_texCoord.v.v;

	pb [0].texCoord.v.u = 
	pb [1].texCoord.v.u = nCol + 1.0f / 64.0f;
	pb [1].texCoord.v.v = 
	pb [2].texCoord.v.v = nRow;
	pb [2].texCoord.v.u = 
	pb [3].texCoord.v.u = nCol + 7.0f / 64.0f;
	pb [0].texCoord.v.v = 
	pb [3].texCoord.v.v = nRow + 1 / 8.0f;
	pb [0].texCoord.v.l = 
	pb [1].texCoord.v.l = 
	pb [2].texCoord.v.l = 
	pb [3].texCoord.v.l = 0.0f;
	}
else {
	float hx = ParticleImageInfo (m_nType).xBorder;
	pb [(int) m_nOrient].texCoord.v.u = pb [int (m_nOrient + 3) % 4].texCoord.v.u = m_texCoord.v.u + hx;
	pb [int (m_nOrient + 1) % 4].texCoord.v.u = pb [(m_nOrient + 2) % 4].texCoord.v.u = m_texCoord.v.u + m_deltaUV - hx;
	float hy = ParticleImageInfo (m_nType).yBorder;
	pb [(int) m_nOrient].texCoord.v.v = pb [int (m_nOrient + 1) % 4].texCoord.v.v = m_texCoord.v.v + hy;
	pb [int (m_nOrient + 2) % 4].texCoord.v.v = pb [(m_nOrient + 3) % 4].texCoord.v.v = m_texCoord.v.v + m_deltaUV - hy;
	pb [0].texCoord.v.l = pb [1].texCoord.v.l = pb [2].texCoord.v.l = pb [3].texCoord.v.l = (((m_nType == SMOKE_PARTICLES) || (m_nType == WATERFALL_PARTICLES)) ? 1.0f : 2.0f);
	}

if (m_nType == RAIN_PARTICLES) {
	uVec.Assign (m_vDir);
	CFloatVector3::Normalize (uVec);
	uVec.Neg ();
	CFloatVector3 v;
	v.Assign (gameData.render.mine.viewer.vPos);
	CFloatVector3 u = uVec - v;
	CFloatVector3 c = vCenter - v;
	CFloatVector3::Normalize (u);
	CFloatVector3::Normalize (c);
	rVec = CFloatVector3::Cross (u, v);
	}
else {
	if ((m_nType == SNOW_PARTICLES) || ((m_nType == BUBBLE_PARTICLES) && gameOpts->render.particles.bWiggleBubbles))
		vCenter.v.coord.x += (float) sin (nFrame / 4.0f * Pi) / (10 + rand () % 6);
	if (m_bRotate && gameOpts->render.particles.bRotate) {
		CFixMatrix& mOrient = mRot [1][(m_nOrient & 1) ? 63 - m_nRotFrame : m_nRotFrame];
		uVec.Assign (mOrient.m.dir.u);
		rVec.Assign (mOrient.m.dir.r);
		}
	else {
		uVec.Assign (gameData.render.mine.viewer.mOrient.m.dir.u);
		rVec.Assign (gameData.render.mine.viewer.mOrient.m.dir.r);
		}
	}
uVec *= m_nHeight * fScale;
rVec *= m_nWidth * fScale;
pb [0].vertex = pb [1].vertex = vCenter - rVec;
pb [0].vertex -= uVec;
pb [1].vertex += uVec;
pb [2].vertex = pb [3].vertex = vCenter + rVec;
pb [2].vertex += uVec;
pb [3].vertex -= uVec;
}

#endif

//------------------------------------------------------------------------------
//eof
