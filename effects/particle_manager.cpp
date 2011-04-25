
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

#define HAVE_PARTICLE_SHADER	1

#if HAVE_PARTICLE_SHADER
#	define USE_PARTICLE_SHADER	(ogl.m_features.bMultipleRenderTargets && (gameOpts->SoftBlend (SOFT_BLEND_PARTICLES)))
#else
#	define USE_PARTICLE_SHADER	0
#endif

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

int hParticleShader = -1;

bool CParticleManager::LoadShader (float dMax)
{
	static float dMaxPrev = -1;

ogl.ClearError (0);
ogl.m_states.bDepthBlending = 0;
if (!gameOpts->render.bUseShaders)
	return false;
if (ogl.m_states.bDepthBlending < 1)
	return false;
if (!ogl.CopyDepthTexture (0, GL_TEXTURE1))
	return false;
ogl.m_states.bDepthBlending = 1;
if (dMax < 1)
	dMax = 1;
//ogl.DrawBuffer ()->FlipBuffers (0, 1); // color buffer 1 becomes render target, color buffer 0 becomes render source (scene texture)
//ogl.DrawBuffer ()->SetDrawBuffers ();
m_shaderProg = GLhandleARB (shaderManager.Deploy (hParticleShader));
if (!m_shaderProg)
	return false;
if (shaderManager.Rebuild (m_shaderProg)) {
	shaderManager.Set ("particleTex", 0);
	shaderManager.Set ("depthTex", 1);
	shaderManager.Set ("windowScale", ogl.m_data.windowScale.vec);
	shaderManager.Set ("dMax", dMax);
	}
else {
	if (dMaxPrev != dMax)
		shaderManager.Set ("dMax", dMax);
	}
dMaxPrev = dMax;
ogl.SetDepthTest (false);
ogl.SetAlphaTest (false);
ogl.SetBlendMode (OGL_BLEND_ALPHA_CONTROLLED);
ogl.SelectTMU (GL_TEXTURE0);
return true;
}

//-------------------------------------------------------------------------

void CParticleManager::UnloadShader (void)
{
if (ogl.m_states.bDepthBlending) {
	shaderManager.Deploy (-1);
	//DestroyGlareDepthTexture ();
	ogl.SetTexturing (true);
	ogl.SelectTMU (GL_TEXTURE1);
	ogl.BindTexture (0);
	ogl.SelectTMU (GL_TEXTURE2);
	ogl.BindTexture (0);
	ogl.SelectTMU (GL_TEXTURE0);
	ogl.SetDepthTest (true);
	}
}

//------------------------------------------------------------------------------
// The following shader blends a particle into a scene. The blend mode depends
// on the particle color's alpha value: 0.0 => additive, otherwise alpha
// This shader allows to switch between additive and alpha blending without
// having to flush a particle render batch beforehand
// In order to be able to handle blending in a shader, a framebuffer with 
// two color buffers is used and the scene from the one color buffer is rendered
// into the other color buffer with blend mode replace (GL_ONE, GL_ZERO)

const char *particleFS =
	"uniform sampler2D particleTex, depthTex;\r\n" \
	"uniform float dMax;\r\n" \
	"uniform vec2 windowScale;\r\n" \
	"//#define ZNEAR 1.0\r\n" \
	"//#define ZFAR 5000.0\r\n" \
	"//#define A 5001.0 //(ZNEAR + ZFAR)\r\n" \
	"//#define B 4999.0 //(ZNEAR - ZFAR)\r\n" \
	"//#define C 10000.0 //(2.0 * ZNEAR * ZFAR)\r\n" \
	"//#define D (NDC (Z) * B)\r\n" \
	"// compute Normalized Device Coordinates\r\n" \
	"#define NDC(z) (2.0 * z - 1.0)\r\n" \
	"// compute eye space depth value from window depth\r\n" \
	"#define ZEYE(z) (10000.0 / (5001.0 - NDC (z) * 4999.0)) //(C / (A + D))\r\n" \
	"//#define ZEYE(z) -(ZFAR / ((z) * (ZFAR - ZNEAR) - ZFAR))\r\n" \
	"void main (void) {\r\n" \
	"// compute distance from scene fragment to particle fragment and clamp with 0.0 and max. distance\r\n" \
	"// the bigger the result, the further the particle fragment is behind the corresponding scene fragment\r\n" \
	"float dz = clamp (ZEYE (gl_FragCoord.z) - ZEYE (texture2D (depthTex, gl_FragCoord.xy * windowScale).r), 0.0, dMax);\r\n" \
	"// compute scaling factor [0.0 - 1.0] - the closer distance to max distance, the smaller it gets\r\n" \
	"dz = (dMax - dz) / dMax;\r\n" \
	"vec4 particleColor = texture2D (particleTex, gl_TexCoord [0].xy) * gl_Color * dz;\r\n" \
	"if (gl_Color.a == 0.0) //additive\r\n" \
	"   gl_FragColor = vec4 (particleColor.rgb, 1.0);\r\n" \
	"else // alpha\r\n" \
	"   gl_FragColor = vec4 (particleColor.rgb * particleColor.a, 1.0 - particleColor.a);\r\n" \
	"}\r\n"
	;

const char *particleVS =
	"void main (void){\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"gl_Position = ftransform (); //gl_ModelViewProjectionMatrix * gl_Vertex;\r\n" \
	"gl_FrontColor = gl_Color;}\r\n"
	;

//-------------------------------------------------------------------------

void CParticleManager::InitShader (void)
{
if (ogl.m_features.bMultipleRenderTargets) {
	PrintLog ("building particle blending shader program\n");
	if (ogl.m_features.bRenderToTexture && ogl.m_features.bShaders) {
		ogl.m_states.bDepthBlending = 1;
		m_shaderProg = 0;
		if (!shaderManager.Build (hParticleShader, particleFS, particleVS)) {
			ogl.ClearError (0);
			ogl.m_states.bDepthBlending = -1;
			}
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
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
OglTexCoordPointer (2, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].texCoord);
OglColorPointer (4, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].color);
OglVertexPointer (3, GL_FLOAT, sizeof (tParticleVertex), &m_vertices [0].vertex);
glColor4f (1.0, 1.0, 1.0, 1.0);
return 1;
}

//------------------------------------------------------------------------------

bool CParticleBuffer::Flush (float fBrightness, bool bForce)
{
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
		else if ((m_nType <= WATERFALL_PARTICLES) && USE_PARTICLE_SHADER) {
			if (!particleManager.LoadShader (20.0f))
				shaderManager.Deploy (-1);
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
for (i = 0; i < MAX_PARTICLE_BUFFERS; i++)
	particleBuffer [i].Reset ();

#if TRANSFORM_PARTICLE_VERTICES

tSinCosf sinCosPart [PARTICLE_POSITIONS];
ComputeSinCosTable (sizeofa (sinCosPart), sinCosPart);
CFloatMatrix mat;
mat.mat.dir.r.dir.coord.z =
mat.mat.dir.u.dir.coord.z =
mat.mat.dir.f.dir.coord.x =
mat.mat.dir.f.dir.coord.y = 0;
mat.mat.dir.f.dir.coord.z = 1;
CFloatVector dir;
dir.Set (1.0f, 1.0f, 0.0f, 1.0f);
for (int i = 0; i < PARTICLE_POSITIONS; i++) {
	mat.mat.dir.r.dir.coord.x =
	mat.mat.dir.u.dir.coord.y = sinCosPart [i].fCos;
	mat.mat.dir.u.dir.coord.x = sinCosPart [i].fSin;
	mat.mat.dir.r.dir.coord.y = -mat.mat.dir.u.dir.coord.x;
	vRot [i] = mat * dir;
	}

#else

CAngleVector vRotAngs;
vRotAngs.SetZero ();
for (int i = 0; i < PARTICLE_POSITIONS; i++) {
	vRotAngs.v.coord.b = i * (I2X (1) / PARTICLE_POSITIONS);
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
										float fScale, /*int nDensity, int nPartsPerPos,*/ int nLife, int nSpeed, char nType,
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
int i = systemP->Create (vPos, vDir, mOrient, nSegment, nMaxEmitters, nMaxParts, fScale, /*nDensity, nPartsPerPos,*/ 
								 nLife, nSpeed, nType, nObject, colorP, bBlowUpParts, nSide);
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
for (int i = 0; i < MAX_PARTICLE_BUFFERS; i++)
	particleBuffer [i].Setup (nThread);
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
#if MAX_PARTICLE_BUFFERS  > 1
	bool bFlushed = false;

for (int i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
	float d = 0;
	int h = 0;
	// flush most distant particles first
	for (int j = 0; j < MAX_PARTICLE_BUFFERS; j++) {
		if (d < particleBuffer [j].m_dMax) {
			d = particleBuffer [j].m_dMax;
			h = j;
			}
		}
	if (particleBuffer [h].Flush (fBrightness, bForce))
		bFlushed = true;
	}
return bFlushed;
#else
return particleBuffer [0].Flush (fBrightness, bForce);
#endif
}

//------------------------------------------------------------------------------

short CParticleManager::Add (CParticle* particleP, float brightness, int nBuffer, bool& bFlushed)
{
#if MAX_PARTICLE_BUFFERS  > 1
if ((particleP->RenderType () != particleBuffer [nBuffer].GetType ()) || ((particleP->m_bEmissive != particleBuffer [nBuffer].m_bEmissive) && !USE_PARTICLE_SHADER))
	return -1;
CFloatVector pos;
pos.Assign (particleP->m_vPos);
float rad = particleP->Rad ();
for (int i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
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
particleBuffer [nBuffer].Add (particleP, brightness, pos, rad);
return nBuffer;
#else
CFloatVector pos;
pos.Assign (particleP->m_vPos);
float rad = particleP->Rad ();
particleBuffer [0].Add (particleP, brightness, pos, rad);
return 0;
#endif
}

//------------------------------------------------------------------------------

bool CParticleManager::Add (CParticle* particleP, float brightness)
{
#if MAX_PARTICLE_BUFFERS  > 1
	bool	bFlushed = false;
	int	i, j;

for (i = 0; i < MAX_PARTICLE_BUFFERS; i++) {
	if (Add (particleP, brightness, i, bFlushed) >= 0)
		return bFlushed;
	}
// flush and reuse the buffer with the most entries
for (i = 1, j = 0; i < MAX_PARTICLE_BUFFERS; i++) {
	if (particleBuffer [i].m_nType < 0) {
		j = i;
		break;
		}
	if (particleBuffer [i].m_iBuffer > particleBuffer [j].m_iBuffer)
		j = i;
	}
return particleBuffer [j].Add (particleP, brightness, particleP->Posf (), particleP->Rad ());
#else
return particleBuffer [0].Add (particleP, brightness, particleP->Posf (), particleP->Rad ());
#endif
}

//------------------------------------------------------------------------------

int CParticleManager::BeginRender (int nType, float nScale)
{
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
