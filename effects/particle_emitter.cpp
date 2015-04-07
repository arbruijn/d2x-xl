
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

//	-----------------------------------------------------------------------------

char CParticleEmitter::ObjectClass (int32_t nObject)
{
if ((nObject >= 0) && (nObject < 0x70000000)) {
	CObject	*objP = gameData.Object (nObject);
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

inline int32_t CParticleEmitter::MayBeVisible (int32_t nThread)
{
return (m_nSegment < 0) || (gameData.Segment (m_nSegment)->m_function == SEGMENT_FUNC_SKYBOX) || SegmentMayBeVisible (m_nSegment, 5, -1, nThread);
}

//------------------------------------------------------------------------------

float  CParticleEmitter::Brightness (void)
{
	CObject	*objP;

if (m_nObject >= 0x70000000)
	return 0.5f;
if (m_nType > 1)
	return 1.0f;
if (m_nObject < 0)
	return m_fBrightness;
if (m_nObjType == OBJ_EFFECT)
	return (float) m_nDefBrightness / 100.0f;
if (m_nObjType == OBJ_DEBRIS)
	return 0.5f;
if ((m_nObjType == OBJ_WEAPON) && (m_nObjId == PROXMINE_ID))
	return 0.2f;
objP = gameData.Object (m_nObject);
if ((objP->info.nType != m_nObjType) || (objP->info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED | OF_ARMAGEDDON)))
	return m_fBrightness;
return m_fBrightness = (float) objP->Damage () * 0.5f + 0.1f;
}

//------------------------------------------------------------------------------

#if MT_PARTICLES

int32_t RunEmitterThread (tParticleEmitter *emitterP, int32_t nCurTime, tRenderTask nTask)
{
int32_t	i;

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

int32_t CParticleEmitter::Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
											 int16_t nSegment, int32_t nObject, int32_t nMaxParts, float fScale,
											 int32_t nLife, int32_t nSpeed, char nType,
											 CFloatVector *colorP, int32_t nCurTime, int32_t bBlowUpParts, CFixVector *vEmittingFace)
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
//m_nDensity = nDensity;
//m_nPartsPerPos = nPartsPerPos;
m_nSegment = nSegment;
m_nObject = nObject;
if ((nObject >= 0) && (nObject < 0x70000000)) {
	m_nObjType = gameData.Object (nObject)->info.nType;
	m_nObjId = gameData.Object (nObject)->info.nId;
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

int32_t CParticleEmitter::Destroy (void)
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
	int32_t	i, j;

for (i = m_nParts, j = m_nFirstPart; i; i--, j = (j + 1) % m_nPartLimit)
	if (m_particles [j].nType < 0)
		j = j;
}

#endif

//------------------------------------------------------------------------------

int32_t CParticleEmitter::Update (int32_t nCurTime, int32_t nThread)
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
		int32_t			t, h, i, j, nNewParts = 0;
		float				fDist;
		float				fBrightness = Brightness ();
		CFixMatrix		mOrient = m_mOrient;
		CFixVector		vDelta, vPos, *vDir = (m_bHaveDir ? &m_vDir : NULL),
							* vEmittingFace = m_bEmittingFace ? m_vEmittingFace : NULL;
		CFloatVector	vDeltaf, vPosf;

#if SMOKE_SLOWMO
	t = (int32_t) ((nCurTime - m_nMoved) / gameStates.gameplay.slowmo [0].fSpeed);
#else
	t = nCurTime - m_nMoved;
#endif
	nPartSeg [nThread] = -1;
	
	for (i = 0; i < m_nParts; i++)
		m_particles [(m_nFirstPart + i) % m_nPartLimit].Update (nCurTime, fBrightness, nThread);
			
	for (i = 0, j = m_nFirstPart; i < m_nParts; i++) {
		if (m_particles [j].m_nLife <= 0) {
			if (j != m_nFirstPart)
				m_particles [j] = m_particles [m_nFirstPart];
			m_nFirstPart = (m_nFirstPart + 1) % m_nPartLimit;
			m_nParts--;
			}
		j = (j + 1) % m_nPartLimit;
		}

	m_nTicks += t;
	if ((m_nPartsPerPos = (int32_t) (m_fPartsPerTick * m_nTicks)) >= 1) {
		if ((m_nType == BUBBLE_PARTICLES) || (m_nType == RAIN_PARTICLES) || (m_nType == SNOW_PARTICLES)) {
#if 1
			m_nPartsPerPos = Rand (m_nPartsPerPos) + 1;
#else
			if (Rand (4))	// create some irregularity in bubble appearance
				goto funcExit;
#endif
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
				vDeltaf.v.coord.x /= (float) h;
				vDeltaf.v.coord.y /= (float) h;
				vDeltaf.v.coord.z /= (float) h;
				}
			else {
				vPosf.Assign (m_vPrevPos);
				vDeltaf.Assign (vDelta);
				vDeltaf.v.coord.x /= (float) h;
				vDeltaf.v.coord.y /= (float) h;
				vDeltaf.v.coord.z /= (float) h;
				}
			j = (m_nFirstPart + m_nParts) % m_nPartLimit;
			for (i = 0; i < h; i++) {
				vPos.Assign (vPosf + vDeltaf * float (i));
				if (m_particles [(j + i) % m_nPartLimit].Create (&vPos, vDir, &mOrient, m_nSegment, m_nLife,
																				 m_nSpeed, m_nType, m_nClass, m_fScale, m_bHaveColor ? &m_color : NULL,
																				 nCurTime, m_bBlowUpParts, m_nFadeType, fBrightness, vEmittingFace)) {
					nNewParts++;
					}
				if (m_nType == BULLET_PARTICLES)
					break;
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

int32_t CParticleEmitter::Render (int32_t nThread)
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
	int32_t		h, i, j;
	int32_t		bVisible = (m_nObject >= 0x70000000) || MayBeVisible (nThread);

#if DBG
	if (m_nFirstPart >= int32_t (m_particles.Length ()))
		return 0;
	if (m_nPartLimit > int32_t (m_particles.Length ()))
		m_nPartLimit = int32_t (m_particles.Length ());
#endif
	for (h = 0, i = m_nParts, j = m_nFirstPart; i; i--, j = (j + 1) % m_nPartLimit)
		if ((bVisible || m_particles [j].IsVisible (nThread)) && transparencyRenderer.AddParticle (m_particles + j, fBrightness, nThread))
			h++;
	PROF_END(ptParticles)
	return h;
#endif
	}
return 0;
}

//------------------------------------------------------------------------------

void CParticleEmitter::SetPos (CFixVector *vPos, CFixMatrix *mOrient, int16_t nSegment)
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

void CParticleEmitter::SetDir (CFixVector *vDir)
{
if ((m_bHaveDir = (vDir != NULL)))
	m_vDir = *vDir;
}

//------------------------------------------------------------------------------

void CParticleEmitter::SetLife (int32_t nLife)
{
m_nLife = nLife;
m_fPartsPerTick = nLife ? float (m_nMaxParts) / float (abs (nLife) * 1.25f) : 0.0f;
m_nTicks = 0;
}

//------------------------------------------------------------------------------

void CParticleEmitter::SetBrightness (int32_t nBrightness)
{
m_nDefBrightness = nBrightness;
}

//------------------------------------------------------------------------------

void CParticleEmitter::SetFadeType (int32_t nFadeType)
{
m_nFadeType = nFadeType;
}

//------------------------------------------------------------------------------

void CParticleEmitter::SetBlowUp (int32_t bBlowUpParts)
{
m_bBlowUpParts = bBlowUpParts;
}

//------------------------------------------------------------------------------

void CParticleEmitter::SetSpeed (int32_t nSpeed)
{
m_nSpeed = nSpeed;
}

//------------------------------------------------------------------------------

void CParticleEmitter::SetType (int32_t nType)
{
m_nType = nType;
}

//------------------------------------------------------------------------------

int32_t CParticleEmitter::SetDensity (int32_t nMaxParts/*, int32_t nDensity*/)
{
	CParticle	*pp;
	int32_t			h;

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
//m_nDensity = nDensity;
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

//------------------------------------------------------------------------------
//eof
