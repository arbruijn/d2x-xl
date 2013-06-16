
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

#if DBG
#include "timeout.h"

CTimeout toFlushed (1000);
int nFlushes [4] = {0, 0, 0, 0};
int nPartsFlushed [4] = {0, 0, 0, 0};
int iFlushed = 0;
#endif

//------------------------------------------------------------------------------

bool CParticleBuffer::AlphaControl (void)
{
#if HAVE_PARTICLE_SHADER
return !gameStates.render.cameras.bActive && (m_nType <= WATERFALL_PARTICLES) && USE_PARTICLE_SHADER && (ogl.m_features.bDepthBlending > -1) && gameOpts->SoftBlend (SOFT_BLEND_PARTICLES);
#else
return false;
#endif
}

//------------------------------------------------------------------------------

void CParticleBuffer::Setup (void)
{
	bool alphaControl = AlphaControl ();

PROF_START
#if USE_OPENMP //> 1
if (gameStates.app.bMultiThreaded) {
#	if (LAZY_RENDER_SETUP < 2)
	if (m_iBuffer <= 1000)
#	endif
	for (int i = 0; i < m_iBuffer; i++) {
		m_particles [i].particle->Setup (alphaControl, m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, 0);
		}
#	if (LAZY_RENDER_SETUP < 2)
else
#	endif
#	pragma omp parallel
		{
		int nThread = omp_get_thread_num();
#	pragma omp for 
		for (int i = 0; i < m_iBuffer; i++) {
			m_particles [i].particle->Setup (alphaControl, m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, nThread);
			}
		}
	}
else
#endif
	{
	if ((m_iBuffer < 100) || !RunRenderThreads (rtParticles))
		for (int i = 0; i < m_iBuffer; i++)
			m_particles [i].particle->Setup (alphaControl, m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, 0);
	}
PROF_END(ptParticles)
}

//------------------------------------------------------------------------------

void CParticleBuffer::Reset (void)
{
ogl.ResetClientStates (1);
ogl.SetDepthTest (true);
ogl.SetAlphaTest (true);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
m_iBuffer = 0;
m_nType = -1;
m_bEmissive = false;
m_dMax = 0.0f;
shaderManager.Deploy (-1);
CEffectArea::Reset ();
}

//------------------------------------------------------------------------------

void CParticleBuffer::Setup (int nThread)
{
int nStep = m_iBuffer / gameStates.app.nThreads;
int nStart = nStep * nThread;
int nEnd = (nThread == gameStates.app.nThreads - 1) ? m_iBuffer : nStart + nStep;
bool alphaControl = AlphaControl ();

for (int i = nStart; i < nEnd; i++)
	m_particles [i].particle->Setup (alphaControl, m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, 0);
}

//------------------------------------------------------------------------------

int CParticleBuffer::bCompatible [2 * PARTICLE_TYPES] = {0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 
																			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

bool CParticleBuffer::Compatible (CParticle* particleP)
{
#if DBG
if (m_nType == FIRE_PARTICLES)
	m_nType = m_nType;
#endif
int nParticleType = particleP->RenderType ();
if ((nParticleType == m_nType) && (particleP->m_bEmissive == m_bEmissive))
	return 1;
return USE_PARTICLE_SHADER && bCompatible [m_nType] && bCompatible [nParticleType];
}

//------------------------------------------------------------------------------

bool CParticleBuffer::Add (CParticle* particleP, float brightness, CFloatVector& pos, float rad)
{
	bool bFlushed = false;

if ((m_iBuffer == PART_BUF_SIZE) || !Compatible (particleP)) 
	bFlushed = Flush (brightness, true);
if (!m_iBuffer) {
	m_nType = particleP->RenderType ();
	m_bEmissive = particleP->m_bEmissive;
	}

	tRenderParticle* pb = m_particles + m_iBuffer++;

pb->particle = particleP;
pb->fBrightness = brightness;
pb->nFrame = particleP->m_iFrame;
pb->nRotFrame = particleP->m_nRotFrame;
CEffectArea::Add (pos, rad);
float d = CFloatVector::Dist (pos, transformation.m_info.posf [0]);
if (m_dMax < d)
	m_dMax = d;
return bFlushed;
}

//------------------------------------------------------------------------------

int CParticleBuffer::Init (void)
{
ogl.ResetClientStates (1);
ogl.EnableClientStates (1, 1, 0, GL_TEXTURE0);
OglTexCoordPointer (3, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].texCoord);
OglColorPointer (4, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].color);
OglVertexPointer (3, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].vertex);
glColor4f (1.0, 1.0, 1.0, 1.0);
return 1;
}

//------------------------------------------------------------------------------

int CParticleBuffer::UseParticleShader (void)
{
if (!USE_PARTICLE_SHADER)
	return -1;
if (!bCompatible [m_nType])
	return -1;
int nShader = 0; //ogl.m_features.bTextureArrays.Available ();
if ((ogl.m_features.bDepthBlending > -1) && gameOpts->SoftBlend (SOFT_BLEND_PARTICLES))
	return nShader + 2;
return nShader;
}

//------------------------------------------------------------------------------

bool CParticleBuffer::Flush (float fBrightness, bool bForce)
{
	static vec3 dMax = {5.0f, 3.0f, 5.0f}; // blend ranges for smoke, sparks, bubbles
	int nShader = 0;
	bool bHaveTexture = false;

if (!m_iBuffer)
	return false;
if (!gameOpts->render.particles.nQuality) {
	Reset ();
	return false;
	}
if ((m_nType < 0) || (m_iBuffer < 2)) {
	Reset ();
	return false;
	}

#if DBG
if (toFlushed.Expired ()) {
	iFlushed = (iFlushed + 1) % 4;
	nFlushes [iFlushed] = nPartsFlushed [iFlushed] = 0;
	}
	++nFlushes [iFlushed];
	nPartsFlushed [iFlushed] += m_iBuffer;
	int p = 0, f = 0;
	for (int i = 0; i < 4; i++)
		if (i != iFlushed)
		p += nPartsFlushed [i], f = nFlushes [i];
if (f)
	HUDMessage (0, "%1.2f particles/flush", float (p) / float (f));

//if (m_nType % PARTICLE_TYPES == FIRE_PARTICLES)
//	m_nType = m_nType;
//else {
//	Reset ();
//	return false;
//	}
#endif
#if ENABLE_FLUSH
PROF_START
#if 1
ogl.ResetClientStates (1);
ogl.EnableClientStates (1, 1, 0, GL_TEXTURE0);
#else
if (!Init ()) {
	Reset ();
	return false;
	}
#endif

#if LAZY_RENDER_SETUP
	Setup ();
#endif

if (ogl.m_features.bShaders) {
#if SMOKE_LIGHTING	// smoke is currently always rendered fully bright
	if (m_nType <= SMOKE_PARTICLES) {
		if ((gameOpts->render.particles.nQuality == 2) && !automap.Display () && lightManager.Headlights ().nLights) {
			CFloatVector color = {1.0f, 1.0f, 1.0f, 1.0f};
			lightManager.Headlights ().SetupShader (1, 0, &color);
			}
		else 
			shaderManager.Deploy (-1);
		}
	else
#endif
	//if (gameStates.render.cameras.bActive /*|| !gameOpts->SoftBlend (SOFT_BLEND_PARTICLES)*/)
	//	shaderManager.Deploy (-1);
	//else 
#if 0 //HAVE_PARTICLE_SHADER
	if (0 <= (nShader = UseParticleShader ())) {
		if (!particleManager.LoadShader (nShader, dMax))
			shaderManager.Deploy (-1);
		else {
			bHaveTexture = true;
			if (nShader & 1) {
				particleImageManager.LoadMultipleTextures (GL_TEXTURE1); // texture arrays don't seem to like being bound to another than GL_TEXTURE0 though - this doesn't work
				}
			else {
				ogl.EnableClientStates (1, 1, 0, GL_TEXTURE2);
				ParticleImageInfo (BUBBLE_PARTICLES).bmP->Bind (0);
				ogl.EnableClientStates (1, 1, 0, GL_TEXTURE1);
				ParticleImageInfo (SPARK_PARTICLES).bmP->Bind (0);
				ogl.EnableClientStates (1, 1, 0, GL_TEXTURE0);
				ParticleImageInfo (SMOKE_PARTICLES).bmP->Bind (0);
				}	
			}
		}
	else 
#endif
	if (gameOpts->SoftBlend (SOFT_BLEND_PARTICLES) && ((m_nType <= WATERFALL_PARTICLES) || (m_nType >= PARTICLE_TYPES))) { // load soft blending shader
		if (!glareRenderer.LoadShader (5, (m_nType < PARTICLE_TYPES) ? m_bEmissive : -1))
			shaderManager.Deploy (-1);
		}
	else
		shaderManager.Deploy (-1);
	}

if (!bHaveTexture) {
	CBitmap* bmP = ParticleImageInfo (m_nType % PARTICLE_TYPES).bmP;
	if (!bmP) {
		PROF_END(ptParticles)
		Reset ();
		return false;
		}
	if (bmP->CurFrame ())
		bmP = bmP->CurFrame ();
	if (bmP->Bind (1) && !particleImageManager.Load (m_nType % PARTICLE_TYPES, 1)) {
		PROF_END(ptParticles)
		Reset ();
		return false;
		}
	ogl.SetBlendMode ((m_nType < PARTICLE_TYPES) ? m_bEmissive : OGL_BLEND_MULTIPLY);
	}

glNormal3f (0, 0, -1);
#if !TRANSFORM_PARTICLE_VERTICES
ogl.SetupTransform (1);
#endif
ogl.SetFaceCulling (false);
ogl.EnableClientStates (1, 1, 0, GL_TEXTURE0);
OglTexCoordPointer (3, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].texCoord);
OglColorPointer (4, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].color);
OglVertexPointer (3, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].vertex);
try {
	OglDrawArrays (GL_QUADS, 0, m_iBuffer * 4);
	}
catch (...) {
	ArrayError ("Particle buffer overflow");
	}
ogl.SetFaceCulling (true);
#if !TRANSFORM_PARTICLE_VERTICES
ogl.ResetTransform (1);
#endif
glNormal3f (1, 1, 1);
PROF_END(ptParticles)
#endif
Reset ();
#if 0
if (ogl.m_features.bShaders && !glareRenderer.ShaderActive ())
	shaderManager.Deploy (-1);
#endif
return true;
}

//------------------------------------------------------------------------------
//eof
