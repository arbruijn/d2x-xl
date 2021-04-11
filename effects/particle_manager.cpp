
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

CParticleManager particleManager;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CParticleManager::RebuildSystemList (void)
{
#if 0
m_nUsed =
m_nFree = -1;
CParticleSystem *pSystem = m_systems;
for (int32_t i = 0; i < MAX_PARTICLE_SYSTEMS; i++, pSystem++) {
	if (pSystem->HasEmitters ()) {
		pSystem->SetNext (m_nUsed);
		m_nUsed = i;
		}
	else {
		pSystem->Destroy ();
		pSystem->SetNext (m_nFree);
		m_nFree = i;
		}
	}
#endif
}

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
for (int32_t i = 0; i < 2; i++) {
	if (!m_objectSystems [i].Buffer ()) {
		char szLabel [40];
		sprintf (szLabel, "CParticleManager::m_objectSystems [%d]", i);
		if (!m_objectSystems [i].Create (LEVEL_OBJECTS, szLabel)) {
			Shutdown ();
			extraGameInfo [0].bUseParticles = 0;
			return;
			}
		m_objectSystems [i].Clear (0xff);
		}
	}
if (!m_objExplTime.Buffer ()) {
	if (!m_objExplTime.Create (LEVEL_OBJECTS, "CParticleManager::m_objExplTime")) {
		Shutdown ();
		extraGameInfo [0].bUseParticles = 0;
		return;
		}
	m_objExplTime.Clear (0);
	}
if (!m_systems.Create (MAX_PARTICLE_SYSTEMS, "CParticleManager::m_systems")) {
	Shutdown ();
	extraGameInfo [0].bUseParticles = 0;
	return;
	}
m_systemList.Create (MAX_PARTICLE_SYSTEMS, "CParticleManager::m_systemList");
int32_t i = 0;
int32_t nCurrent = m_systems.FreeList ();
for (CParticleSystem* pSystem = m_systems.GetFirst (nCurrent); pSystem; pSystem = GetNext (nCurrent))
	pSystem->Init (i++);
for (i = 0; i < MAX_PARTICLE_BUFFERS; i++)
	particleBuffer [i].Reset ();

#if TRANSFORM_PARTICLE_VERTICES

tSinCosf sinCosPart [PARTICLE_POSITIONS];
ComputeSinCosTable (sizeofa (sinCosPart), sinCosPart);
CFloatMatrix mat;
mat.m.dir.r.v.coord.z =
mat.m.dir.u.v.coord.z =
mat.m.dir.f.v.coord.x =
mat.m.dir.f.v.coord.y = 0;
mat.m.dir.f.v.coord.z = 1;
CFloatVector dir;
dir.Set (1.0f, 1.0f, 0.0f, 1.0f);
for (int32_t i = 0; i < PARTICLE_POSITIONS; i++) {
	mat.m.dir.r.v.coord.x =
	mat.m.dir.u.v.coord.y = sinCosPart [i].fCos;
	mat.m.dir.u.v.coord.x = sinCosPart [i].fSin;
	mat.m.dir.r.v.coord.y = -mat.m.dir.u.v.coord.x;
	CParticle::vRot [i] = mat * dir;
	}

#else

CParticle::InitRotation ();

#endif
}

//------------------------------------------------------------------------------

int32_t CParticleManager::Destroy (int32_t i)
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
int32_t nCurrent = -1;
for (CParticleSystem* pSystem = GetFirst (nCurrent), * pNext = NULL; pSystem; pSystem = pNext) {
	pNext = GetNext (nCurrent);
	if (pSystem->m_bDestroy) {
		pSystem->Destroy ();
		m_systems.Push (pSystem->m_nId);
		}
	}
}

//------------------------------------------------------------------------------

int32_t CParticleManager::Shutdown (void)
{
SEM_ENTER (SEM_SMOKE)
int32_t nCurrent = -1;
for (CParticleSystem* pSystem = GetFirst (nCurrent); pSystem; pSystem = GetNext (nCurrent))
	pSystem->Destroy ();
Cleanup ();
m_systems.Destroy ();
m_systemList.Destroy ();
m_objectSystems [0].Destroy ();
m_objectSystems [1].Destroy ();
m_objExplTime.Destroy ();
particleImageManager.FreeAll ();
SEM_LEAVE (SEM_SMOKE)
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CParticleManager::Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
											 int16_t nSegment, int32_t nMaxEmitters, int32_t nMaxParts,
											 float fScale, /*int32_t nDensity, int32_t nPartsPerPos,*/ int32_t nLife, int32_t nSpeed, char nType,
											 int32_t nObject, CFloatVector *pColor, int32_t bBlowUpParts, char nSide)
{
if (!gameOpts->render.particles.nQuality)
	return -1;
#if 0
if (!(EGI_FLAG (bUseParticleSystem, 0, 1, 0)))
	return 0;
else
#endif
CParticleSystem *pSystem;
#if USE_OPENMP > 1
#	pragma omp critical (CParticleManagerCreate)
#endif
{
if (!particleImageManager.Load (nType))
	pSystem = NULL;
else
	pSystem = m_systems.Pop ();
}
if (!pSystem)
	return -1;
int32_t i = pSystem->Create (vPos, vDir, mOrient, nSegment, nMaxEmitters, nMaxParts, fScale, /*nDensity, nPartsPerPos,*/ 
									  nLife, nSpeed, nType, nObject, pColor, bBlowUpParts, nSide);
if (i < 1)
	return i;
return pSystem->Id ();
}

//------------------------------------------------------------------------------

int32_t CParticleManager::Update (void)
{
#if SMOKE_SLOWMO
	static int32_t	t0 = 0;
	int32_t t = gameStates.app.nSDLTicks [0] - t0;

if (t / gameStates.gameplay.slowmo [0].fSpeed < 25)
	return 0;
t0 += (int32_t) (gameStates.gameplay.slowmo [0].fSpeed * 25);
#else
if (!gameStates.app.tick40fps.bTick)
	return 0;
#endif
	int32_t	h = 0;

	int32_t nCurrent = -1;

#if USE_OPENMP > 1
if (gameStates.app.bMultiThreaded && m_systemList.Buffer ()) {
	for (CParticleSystem* pSystem = GetFirst (nCurrent); pSystem; pSystem = GetNext (nCurrent))
		m_systemList [h++] = pSystem;
#	pragma omp parallel
		{
		int32_t nThread = omp_get_thread_num();
#	pragma omp for
		for (int32_t i = 0; i < h; i++)
			m_systemList [i]->Update (nThread);
		}
	}
else 
#endif
	{
	for (CParticleSystem* pSystem = GetFirst (nCurrent); pSystem; pSystem = GetNext (nCurrent))
		h += pSystem->Update (0);
	}
return h;
}

//------------------------------------------------------------------------------

void CParticleManager::Render (void)
{
if (!gameOpts->render.particles.nQuality)
	return;
int32_t nCurrent = -1;

#if USE_OPENMP > 1
if (gameStates.app.bMultiThreaded && m_systemList.Buffer ()) {
	int32_t h = 0;
	for (CParticleSystem* pSystem = GetFirst (nCurrent); pSystem; pSystem = GetNext (nCurrent))
		m_systemList [h++] = pSystem;
	if (h == 0)
		return;
	if (h == 1)
		m_systemList [0]->Render (0);
	else 
#	pragma omp parallel
		{
		int32_t nThread = omp_get_thread_num();
#	pragma omp for
		for (int32_t i = 0; i < h; i++)
			m_systemList [i]->Render (nThread);
		}
	}
else 
#endif
	{
	for (CParticleSystem* pSystem = GetFirst (nCurrent); pSystem; pSystem = GetNext (nCurrent))
		pSystem->Render (0);
	}
}

//------------------------------------------------------------------------------

void CParticleManager::SetupParticles (int32_t nThread)
{
for (int32_t i = 0; i < MAX_PARTICLE_BUFFERS; i++)
	particleBuffer [i].Setup (nThread);
}

//------------------------------------------------------------------------------

int32_t CParticleManager::CloseBuffer (void)
{
Flush (-1);
ogl.DisableClientStates (1, 1, 0, GL_TEXTURE0 + lightmapManager.HaveLightmaps ());
return 1;
}

//------------------------------------------------------------------------------

bool CParticleManager::Flush (float fBrightness, bool bForce)
{
#if MAX_PARTICLE_BUFFERS  > 1
	bool bFlushed = false;

for (int32_t i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
	float d = 0;
	int32_t h = -1;
	// flush most distant particles first
	for (int32_t j = 0; j < MAX_PARTICLE_BUFFERS; j++) {
		if (particleBuffer [j].m_iBuffer && (d < particleBuffer [j].m_dMax)) {
			d = particleBuffer [j].m_dMax;
			h = j;
			}
		}
	if (h < 0)
		break;
	if (particleBuffer [h].Flush (fBrightness, bForce))
		bFlushed = true;
	}
return bFlushed;
#else
return particleBuffer [0].Flush (fBrightness, bForce);
#endif
}

//------------------------------------------------------------------------------

int16_t CParticleManager::Add (CParticle* pParticle, float brightness, int32_t nBuffer, bool& bFlushed)
{
#if MAX_PARTICLE_BUFFERS  > 1
if (!particleBuffer [nBuffer].Compatible (pParticle))
	return -1;
CFloatVector pos;
pos.Assign (pParticle->m_vPos);
float rad = pParticle->Rad ();
for (int32_t i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
	if (i == nBuffer) 
		continue;
	if (!particleBuffer [i].Overlap (pos, rad))
		continue;
#if 0
	if ((particleBuffer [nBuffer].m_dMax > particleBuffer [i].m_dMax) && particleBuffer [nBuffer].Flush (brightness, true))
		bFlushed = true;
#endif
	if (particleBuffer [i].Flush (brightness, true))
		bFlushed = true;
	}
particleBuffer [nBuffer].Add (pParticle, brightness, pos, rad);
return nBuffer;
#else
CFloatVector pos;
pos.Assign (pParticle->m_vPos);
float rad = pParticle->Rad ();
particleBuffer [0].Add (pParticle, brightness, pos, rad);
return 0;
#endif
}

//------------------------------------------------------------------------------

bool CParticleManager::Add (CParticle* pParticle, float brightness)
{
#if MAX_PARTICLE_BUFFERS  > 1
	bool	bFlushed = false;
	int32_t	i, j;

for (i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
	if (Add (pParticle, brightness, i, bFlushed) >= 0)
		return bFlushed;
	}
// flush and reuse the buffer with the most entries
for (i = 0, j = 0; i < MAX_PARTICLE_BUFFERS; i++) {
	if (particleBuffer [i].m_nType < 0) {
		j = i;
		break;
		}
	if (particleBuffer [i].m_iBuffer > particleBuffer [j].m_iBuffer)
		j = i;
	}
return particleBuffer [j].Add (pParticle, brightness, pParticle->Posf (), pParticle->Rad ());
#else
return particleBuffer [0].Add (pParticle, brightness, pParticle->Posf (), pParticle->Rad ());
#endif
}

//------------------------------------------------------------------------------

int32_t CParticleManager::BeginRender (int32_t nType, float nScale)
{
	static time_t	t0 = 0;

particleManager.SetLastType (-1);
CParticle::SetupRotation ();
if ((gameStates.app.nSDLTicks [0] - t0 < 33) || (ogl.StereoSeparation () < 0))
	particleManager.m_bAnimate = 0;
else {
	t0 = gameStates.app.nSDLTicks [0];
	particleManager.m_bAnimate = 1;
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t CParticleManager::EndRender (void)
{
return 1;
}

//------------------------------------------------------------------------------

CParticleManager::~CParticleManager ()
{
Shutdown ();
}

//------------------------------------------------------------------------------
//eof
