
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

void CParticleBuffer::Setup (void)
{
PROF_START
#if USE_OPENMP > 1
#	if (LAZY_RENDER_SETUP < 2)
if (m_iBuffer <= 1000)
#	endif
for (int i = 0; i < m_iBuffer; i++)
	m_particles [i].particle->Setup (m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, 0);
#	if (LAZY_RENDER_SETUP < 2)
else
#	endif
#	pragma omp parallel
	{
	int nThread = omp_get_thread_num();
#	pragma omp for 
	for (int i = 0; i < m_iBuffer; i++)
		m_particles [i].particle->Setup (m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, nThread);
	}
#else
if ((m_iBuffer < 100) || !RunRenderThreads (rtParticles))
	for (int i = 0; i < m_iBuffer; i++)
		m_particles [i].particle->Setup (m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, 0);
#endif
PROF_END(ptParticles)
}

//------------------------------------------------------------------------------

void CParticleBuffer::Setup (int nThread)
{
int nStep = m_iBuffer / gameStates.app.nThreads;
int nStart = nStep * nThread;
int nEnd = (nThread == gameStates.app.nThreads - 1) ? m_iBuffer : nStart + nStep;

for (int i = nStart; i < nEnd; i++)
	m_particles [i].particle->Setup (m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, 0);
}

//------------------------------------------------------------------------------

bool CParticleBuffer::Add (CParticle* particleP, float brightness, CFloatVector& pos, float rad)
{
	bool bFlushed = false;

if (particleP->RenderType () != m_nType) {
	bFlushed = Flush (brightness, true);
	m_nType = particleP->RenderType ();
	}

	tRenderParticle* pb = m_particles + m_iBuffer++;

pb->particle = particleP;
pb->fBrightness = brightness;
pb->nFrame = particleP->m_iFrame;
pb->nRotFrame = particleP->m_nRotFrame;
if (m_iBuffer == PART_BUF_SIZE)
	bFlushed = Flush (brightness, true);
return bFlushed;
}

//------------------------------------------------------------------------------

int CParticleBuffer::Init (void)
{
ogl.ResetClientStates (1);
ogl.EnableClientStates (1, 1, 0, GL_TEXTURE0);
OglTexCoordPointer (2, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].texCoord);
OglColorPointer (4, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].color);
OglVertexPointer (3, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].vertex);
return 1;
}

//------------------------------------------------------------------------------

bool CParticleBuffer::Flush (float fBrightness, bool bForce)
{
if (!m_iBuffer)
	return false;
if (!gameOpts->render.particles.nQuality) {
	m_iBuffer = 0;
	return false;
	}
if ((m_nType < 0) && !bForce) {
	m_iBuffer = 0;
	return false;
	}

#if ENABLE_FLUSH
PROF_START
if (Init ()) {
	CBitmap* bmP = ParticleImageInfo (m_nType % PARTICLE_TYPES).bmP;
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
	Setup ();
#endif

	ogl.SetBlendMode ((m_nType < PARTICLE_TYPES) ? m_bEmissive : -1);

	if (ogl.m_states.bShadersOk) {
#if SMOKE_LIGHTING	// smoke is currently always rendered fully bright
		if (m_nType <= SMOKE_PARTICLES) {
			if ((gameOpts->render.particles.nQuality == 3) && !automap.Display () && lightManager.Headlights ().nLights) {
				tRgbaColorf color = {1.0f, 1.0f, 1.0f, 1.0f};
				lightManager.Headlights ().SetupShader (1, 0, &color);
				}
			else 
				shaderManager.Deploy (-1);
			}
		else
#endif
		if ((m_nType <= WATERFALL_PARTICLES) || (m_nType >= PARTICLE_TYPES)) {
			if (gameStates.render.cameras.bActive || 
				 !((gameOpts->render.effects.bSoftParticles & 4) && glareRenderer.LoadShader (5, (m_nType < PARTICLE_TYPES) ? m_bEmissive : -1)))
				shaderManager.Deploy (-1);
			}
		else
			shaderManager.Deploy (-1);
		}
	glNormal3f (0, 0, -1);
#if !TRANSFORM_PARTICLE_VERTICES
	ogl.SetupTransform (1);
#endif
	ogl.SetFaceCulling (false);
	OglDrawArrays (GL_QUADS, 0, m_iBuffer * 4);
	ogl.SetFaceCulling (true);
#if !TRANSFORM_PARTICLE_VERTICES
	ogl.ResetTransform (1);
#endif
	glNormal3f (1, 1, 1);
	}
PROF_END(ptParticles)
#endif
m_iBuffer = 0;
m_nType = -1;
Reset ();
if ((ogl.m_states.bShadersOk && !particleManager.LastType ()) && !glareRenderer.ShaderActive ())
	shaderManager.Deploy (-1);
m_nType = -1;
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
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
particleBuffer [0].Reset ();
particleBuffer [1].Reset ();

#if TRANSFORM_PARTICLE_VERTICES

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
	int t = gameStates.app.nSDLTicks [0] - t0;

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

void CParticleManager::SetupParticles (int nThread)
{
particleBuffer [0].Setup (nThread);
particleBuffer [1].Setup (nThread);
}

//------------------------------------------------------------------------------

int CParticleManager::CloseBuffer (void)
{
Flush (-1);
ogl.DisableClientStates (1, 1, 0, GL_TEXTURE0 + lightmapManager.HaveLightmaps ());
return 1;
}

//------------------------------------------------------------------------------

bool CParticleManager::Flush (float fBrightness, bool bForce)
{
	bool bFlushed = false;

for (int i = 0; i < 2; i++)
	if (particleBuffer [i].Flush (fBrightness, bForce))
		bFlushed = true;
return bFlushed;
}

//------------------------------------------------------------------------------

short CParticleManager::Add (CParticle* particleP, float brightness, int nBuffer, bool& bFlushed)
{
if (particleP->RenderType () != particleBuffer [nBuffer].GetType ())
	return -1;
CFloatVector pos;
pos.Assign (particleP->m_vPos);
float rad = particleP->Rad ();
for (int i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
	if ((i != nBuffer) && particleBuffer [i].Overlap (pos, rad) && particleBuffer [!nBuffer].Flush (brightness, true))
		bFlushed = true;
	}
particleBuffer [nBuffer].Add (particleP, brightness, pos, rad);
return nBuffer;
}

//------------------------------------------------------------------------------

bool CParticleManager::Add (CParticle* particleP, float brightness)
{
	bool	bFlushed = false;
	int	i, j;

for (i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
	if (Add (particleP, brightness, 0, bFlushed) >= 0)
		return bFlushed;
	}
// flush and reuse the buffer with the most entries
for (i = 1, j = 0; i < MAX_PARTICLE_BUFFERS; i++) {
	if (particleBuffer [i].m_iBuffer > particleBuffer [j].m_iBuffer)
		j = i;
return particleBuffer [j].Add (particleP, brightness, particleP->Posf (), particleP->Rad ());
}

//------------------------------------------------------------------------------

int CParticleManager::BeginRender (int nType, float nScale)
{
	int				bLightmaps = lightmapManager.HaveLightmaps ();
	static time_t	t0 = 0;

particleManager.SetLastType (-1);
if ((gameStates.app.nSDLTicks [0] - t0 < 33) || (ogl.StereoSeparation () < 0))
	particleManager.m_bAnimate = 0;
else {
	t0 = gameStates.app.nSDLTicks [0];
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
