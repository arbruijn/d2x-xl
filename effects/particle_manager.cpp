
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

CParticleManager particleManager;

tRenderParticle CParticleManager::particleBuffer [PART_BUF_SIZE];
tParticleVertex CParticleManager::particleRenderBuffer [VERT_BUF_SIZE];

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

#if 0

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

#else

CAngleVector vRotAngs;
vRotAngs.SetZero ();
for (int i = 0; i < PARTICLE_POSITIONS; i++) {
	vRotAngs [BA] = i * (I2X (1) / PARTICLE_POSITIONS);
	mRot [i] = CFixMatrix::Create (vRotAngs);
	}

#endif

m_bufferBrightness = -1;
m_bBufferEmissive = 0;
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
m_systems.Destroy ();
m_systemList.Destroy ();
m_objectSystems.Destroy ();
m_objExplTime.Destroy ();
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
if (!gameOpts->render.particles.nQuality)
	return -1;
#if 0
if (!(EGI_FLAG (bUseParticleSystem, 0, 1, 0)))
	return 0;
else
#endif
CParticleSystem *systemP;
#if USE_OPENMP > 1
#	pragma omp critical
#endif
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

	int nCurrent = -1;

#if USE_OPENMP > 1
if (m_systemList.Buffer ()) {
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
	for (CParticleSystem* systemP = GetFirst (nCurrent); systemP; systemP = GetNext (nCurrent))
		h += systemP->Update (0);
	}
return h;
}

//------------------------------------------------------------------------------

void CParticleManager::Render (void)
{
if (!gameOpts->render.particles.nQuality)
	return;
int nCurrent = -1;

#if USE_OPENMP > 1
if (m_systemList.Buffer ()) {
	int h = 0;
	for (CParticleSystem* systemP = GetFirst (nCurrent); systemP; systemP = GetNext (nCurrent))
		m_systemList [h++] = systemP;
#	pragma omp parallel
		{
		int nThread = omp_get_thread_num();
#	pragma omp for
		for (int i = 0; i < h; i++)
			m_systemList [i]->Render (nThread);
		}
	}
else 
#endif
	{
	for (CParticleSystem* systemP = GetFirst (nCurrent); systemP; systemP = GetNext (nCurrent))
		systemP->Render (0);
	}
}

//------------------------------------------------------------------------------

int CParticleManager::InitBuffer (void)
{
ogl.ResetClientStates (1);
ogl.EnableClientStates (1, 1, 0, GL_TEXTURE0);
OglTexCoordPointer (2, GL_FLOAT, sizeof (tParticleVertex), &particleRenderBuffer [0].texCoord);
OglColorPointer (4, GL_FLOAT, sizeof (tParticleVertex), &particleRenderBuffer [0].color);
OglVertexPointer (3, GL_FLOAT, sizeof (tParticleVertex), &particleRenderBuffer [0].vertex);
return 1;
}

//------------------------------------------------------------------------------

void CParticleManager::SetupParticles (int nThread)
{
int nStep = m_iBuffer / gameStates.app.nThreads;
int nStart = nStep * nThread;
int nEnd = (nThread == gameStates.app.nThreads - 1) ? m_iBuffer : nStart + nStep;

for (int i = nStart; i < nEnd; i++)
	particleBuffer [i].particle->Setup (particleBuffer [i].fBrightness, particleBuffer [i].nFrame, particleBuffer [i].nRotFrame, particleRenderBuffer + 4 * i, 0);
}

//------------------------------------------------------------------------------

void CParticleManager::SetupRenderBuffer (void)
{
PROF_START
#if USE_OPENMP > 1
#	if (LAZY_RENDER_SETUP < 2)
if (m_iBuffer <= 1000)
#	endif
for (int i = 0; i < m_iBuffer; i++)
	particleBuffer [i].particle->Setup (particleBuffer [i].fBrightness, particleBuffer [i].nFrame, particleBuffer [i].nRotFrame, particleRenderBuffer + 4 * i, 0);
#	if (LAZY_RENDER_SETUP < 2)
else
#	endif
#	pragma omp parallel
	{
	int nThread = omp_get_thread_num();
#	pragma omp for 
	for (int i = 0; i < m_iBuffer; i++)
		particleBuffer [i].particle->Setup (particleBuffer [i].fBrightness, particleBuffer [i].nFrame, particleBuffer [i].nRotFrame, particleRenderBuffer + 4 * i, nThread);
	}
#else
if ((m_iBuffer < 100) || !RunRenderThreads (rtParticles))
	for (int i = 0; i < m_iBuffer; i++)
		particleBuffer [i].particle->Setup (particleBuffer [i].fBrightness, particleBuffer [i].nFrame, particleBuffer [i].nRotFrame, particleRenderBuffer + 4 * i, 0);
#endif
PROF_END(ptParticles)
}

//------------------------------------------------------------------------------

bool CParticleManager::FlushBuffer (float fBrightness, bool bForce)
{
int nType = particleManager.LastType ();

if (!m_iBuffer)
	return false;
if (!gameOpts->render.particles.nQuality) {
	m_iBuffer = 0;
	return false;
	}
if ((nType < 0) && !bForce) {
	m_iBuffer = 0;
	return false;
	}

#if ENABLE_FLUSH
PROF_START
if (InitBuffer ()) {
	CBitmap* bmP = ParticleImageInfo (nType % PARTICLE_TYPES).bmP;
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

#if LAZY_RENDER_SETUP
	SetupRenderBuffer ();
#endif

	ogl.SetBlendMode ((nType < PARTICLE_TYPES) ? m_bBufferEmissive : -1);

	if (ogl.m_states.bShadersOk) {
#if SMOKE_LIGHTING	// smoke is currently always rendered fully bright
		if (nType <= SMOKE_PARTICLES) {
			if ((gameOpts->render.particles.nQuality == 3) && !automap.Display () && lightManager.Headlights ().nLights) {
				tRgbaColorf color = {1.0f, 1.0f, 1.0f, 1.0f};
				lightManager.Headlights ().SetupShader (1, 0, &color);
				}
			else 
				shaderManager.Deploy (-1);
			}
		else
#endif
		if ((nType <= WATERFALL_PARTICLES) || (nType >= PARTICLE_TYPES)) {
			if (gameStates.render.cameras.bActive || 
				 !((gameOpts->render.effects.bSoftParticles & 4) && glareRenderer.LoadShader (5, (nType < PARTICLE_TYPES) ? m_bBufferEmissive : -1)))
				shaderManager.Deploy (-1);
			}
		else
			shaderManager.Deploy (-1);
		}
	glNormal3f (0, 0, -1);
	ogl.SetupTransform (1);
	OglDrawArrays (GL_QUADS, 0, m_iBuffer * 4);
	ogl.ResetTransform (1);
	glNormal3f (1, 1, 1);
	}
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
return 1;
}

//------------------------------------------------------------------------------

CParticleManager::~CParticleManager ()
{
Shutdown ();
}

//------------------------------------------------------------------------------
//eof
