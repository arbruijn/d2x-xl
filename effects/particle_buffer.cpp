
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

bool CParticleBuffer::AlphaControl (void)
{
#if HAVE_PARTICLE_SHADER
return (!gameStates.render.cameras.bActive && (m_nType <= WATERFALL_PARTICLES) && USE_PARTICLE_SHADER);
#else
return false;
#endif
}

//------------------------------------------------------------------------------

void CParticleBuffer::Setup (void)
{
	bool alphaControl = AlphaControl ();

PROF_START
#if USE_OPENMP > 1
#	if (LAZY_RENDER_SETUP < 2)
if (m_iBuffer <= 1000)
#	endif
for (int i = 0; i < m_iBuffer; i++)
	m_particles [i].particle->Setup (alphaControl, m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, 0);
#	if (LAZY_RENDER_SETUP < 2)
else
#	endif
#	pragma omp parallel
	{
	int nThread = omp_get_thread_num();
#	pragma omp for 
	for (int i = 0; i < m_iBuffer; i++)
		m_particles [i].particle->Setup (alphaControl, m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, nThread);
	}
#else
if ((m_iBuffer < 100) || !RunRenderThreads (rtParticles))
	for (int i = 0; i < m_iBuffer; i++)
		m_particles [i].particle->Setup (m_particles [i].fBrightness, m_particles [i].nFrame, m_particles [i].nRotFrame, m_vertices + 4 * i, 0);
#endif
PROF_END(ptParticles)
}

//------------------------------------------------------------------------------

void CParticleBuffer::Reset (void)
{
ogl.ResetClientStates (1);
ogl.SetDepthTest (true);
ogl.SetAlphaTest (true);
ogl.SetBlendMode (m_bEmissive);
m_iBuffer = 0;
m_nType = -1;
m_bEmissive = false;
m_dMax = 0.0f;
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

bool CParticleBuffer::Add (CParticle* particleP, float brightness, CFloatVector& pos, float rad)
{
	bool bFlushed = false;

if ((particleP->RenderType () != m_nType) || ((particleP->m_bEmissive != m_bEmissive) && !USE_PARTICLE_SHADER)) {
	bFlushed = Flush (brightness, true);
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
if (m_iBuffer == PART_BUF_SIZE)
	bFlushed = Flush (brightness, true);
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
	return 0;
if ((m_nType == SMOKE_PARTICLES) || (m_nType == SPARK_PARTICLES))
	return ogl.m_features.bTextureArrays.Available () ? 2 : 1;
return (m_nType <= WATERFALL_PARTICLES);
}

//------------------------------------------------------------------------------

bool CParticleBuffer::Flush (float fBrightness, bool bForce)
{
	static float dMax [2] = {20.0f, 3.0f};
	int nShader = 0;

if (m_iBuffer < 2)
	return false;
if (!gameOpts->render.particles.nQuality) {
	Reset ();
	return false;
	}
if ((m_nType < 0) || (m_iBuffer < 2)) {
	Reset ();
	return false;
	}

#if ENABLE_FLUSH
PROF_START
if (Init ()) {
	CBitmap* bmP = ParticleImageInfo (m_nType % PARTICLE_TYPES).bmP;
	if (!bmP) {
		PROF_END(ptParticles)
		Reset ();
		return false;
		}
	if (bmP->CurFrame ())
		bmP = bmP->CurFrame ();
	if (bmP->Bind (0)) {
		PROF_END(ptParticles)
		Reset ();
		return false;
		}

#if LAZY_RENDER_SETUP
	Setup ();
#endif

	ogl.SetBlendMode ((m_nType < PARTICLE_TYPES) ? m_bEmissive : OGL_BLEND_MULTIPLY);

	if (ogl.m_features.bShaders) {
#if SMOKE_LIGHTING	// smoke is currently always rendered fully bright
		if (m_nType <= SMOKE_PARTICLES) {
			if ((gameOpts->render.particles.nQuality == 2) && !automap.Display () && lightManager.Headlights ().nLights) {
				tRgbaColorf color = {1.0f, 1.0f, 1.0f, 1.0f};
				lightManager.Headlights ().SetupShader (1, 0, &color);
				}
			else 
				shaderManager.Deploy (-1);
			}
		else
#endif
		if (gameStates.render.cameras.bActive || !gameOpts->SoftBlend (SOFT_BLEND_PARTICLES))
			shaderManager.Deploy (-1);
#if HAVE_PARTICLE_SHADER
		else if ((nShader = UseParticleShader ())) {
			if (!particleManager.LoadShader (nShader - 1, dMax))
				shaderManager.Deploy (-1);
			else if (nShader == 2) 
				particleImageManager.LoadMultipleTextures (particleImageManager.LoadMultipleTextures (GL_TEXTURE0));
			else {
				ogl.EnableClientStates (1, 1, 0, GL_TEXTURE1);
				ParticleImageInfo (SPARK_PARTICLES).bmP->Bind (0);
				}	
			}
#endif
		else if ((m_nType <= WATERFALL_PARTICLES) || (m_nType >= PARTICLE_TYPES)) { // load soft blending shader
			if (!glareRenderer.LoadShader (5, (m_nType < PARTICLE_TYPES) ? m_bEmissive : -1))
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
Reset ();
if ((ogl.m_features.bShaders && !particleManager.LastType ()) && !glareRenderer.ShaderActive ())
	shaderManager.Deploy (-1);
return true;
}

//------------------------------------------------------------------------------
//eof
