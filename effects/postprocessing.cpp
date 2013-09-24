/* Image post processing (fullscreen effects)
	Dietfrid Mali
	22.3.2011
*/

#include <math.h>

#include "descent.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_tmu.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "cockpit.h"
#include "transformation.h"
#include "postprocessing.h"

CPostProcessManager postProcessManager;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// shockwave effect

int hShockwaveShader = -1;

const char* shockwaveVS = 
	"void main(void) {\r\n" \
	"gl_Position = ftransform();\r\n" \
	"gl_TexCoord[0] = gl_MultiTexCoord0;\r\n" \
	"}";

#define SHOCKWAVE_STYLE 0

#if SHOCKWAVE_STYLE

const char* shockwaveFS = 
	"uniform sampler2D sceneTex;\r\n" \
	"uniform sampler2D depthTex;\r\n" \
	"uniform int nShockwaves;\r\n" \
	"uniform vec2 screenSize;\r\n" \
	"uniform vec3 effectStrength;\r\n" \
	"//#define LinearDepth(_z) 10000.0 / (5001.0 - (_z) * 4999.0)\r\n" \
	"//#define LinearDepth(_z) (5000.0 / 4999.0) / ((5000.0 / 4999.0) - (_z))\r\n" \
	"#define PI 3.141592653589793240\r\n" \
	"//const float PI = 2.0 * asin (1.0);\r\n" \
	"#define ZNEAR 1.0\r\n" \
	"#define ZFAR 5000.0\r\n" \
	"#define NDC(z) (2.0 * z - 1.0)\r\n" \
	"#define A (ZNEAR + ZFAR)\r\n" \
	"#define B (ZNEAR - ZFAR)\r\n" \
	"#define C (2.0 * ZNEAR * ZFAR)\r\n" \
	"#define D(z) (NDC (z) * B)\r\n" \
	"#define ZEYE(z) (C / (A + D (z)))\r\n" \
	"void main() {\r\n" \
	"vec2 tcSrc = gl_TexCoord [0].xy * screenSize;\r\n" \
	"vec2 tcDest = tcSrc; //vec2 (0.0, 0.0);\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < 8; i++) if (i < nShockwaves) {\r\n" \
	"  vec2 v = tcSrc - gl_LightSource [i].position.xy;\r\n" \
	"  float d = length (v); //distance of screen coordinate to center of effect\r\n" \
	"  float r = gl_LightSource [i].constantAttenuation; //effect radius in screen space\r\n" \
	"  //if (r >= 0) { // effect range from the wavefront\r\n" \
	"     float offset = r - d;\r\n" \
	"     if (abs (offset) <= effectStrength.z) { // effect range from the wavefront\r\n" \
	"       r += effectStrength.z;\r\n" \
	"       float z = sqrt (r * r - d * d) / r * gl_LightSource [i].linearAttenuation;\r\n" \
	"       if (gl_LightSource [i].position.z - z <= ZEYE (texture2D (depthTex, gl_TexCoord [0].xy).r)) {\r\n" \
	"         offset = sign (offset) * (0.33 - cos (2.0 * PI * offset / effectStrength.z) * 0.33);\r\n" \
	"         v *= effectStrength.z / d;\r\n" \
	"         tcDest += v * (offset * gl_LightSource [i].quadraticAttenuation);\r\n" \
	"         }\r\n" \
	"      }\r\n" \
	"    }\r\n" \
	"  //}\r\n" \
	"gl_FragColor = texture2D (sceneTex, tcDest / screenSize);\r\n" \
	"}\r\n";

#else

const char* shockwaveFS = 
	"uniform sampler2D sceneTex;\r\n" \
	"uniform sampler2D depthTex;\r\n" \
	"uniform int nShockwaves;\r\n" \
	"uniform vec2 screenSize;\r\n" \
	"uniform vec3 effectStrength;\r\n" \
	"#define ZNEAR 1.0\r\n" \
	"#define ZFAR 5000.0\r\n" \
	"#define NDC(z) (2.0 * z - 1.0)\r\n" \
	"#define A (ZNEAR + ZFAR)\r\n" \
	"#define B (ZNEAR - ZFAR)\r\n" \
	"#define C (2.0 * ZNEAR * ZFAR)\r\n" \
	"#define D(z) (NDC (z) * B)\r\n" \
	"#define ZEYE(z) (C / (A + D (z)))\r\n" \
	"void main() {\r\n" \
	"vec2 tcSrc = gl_TexCoord [0].xy * screenSize;\r\n" \
	"vec2 tcDest = tcSrc; //vec2 (0.0, 0.0);\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < 8; i++) if (i < nShockwaves) {\r\n" \
	"  vec2 v = tcSrc - gl_LightSource [i].position.xy;\r\n" \
	"  float r = gl_LightSource [i].constantAttenuation;\r\n" \
	"  float d = length (v);\r\n" \
	"  float offset = d - r;\r\n" \
	"  if (abs (offset) <= effectStrength.z) {\r\n" \
	"    r += effectStrength.z;\r\n" \
	"    float z = sqrt (r * r - d * d) / r * gl_LightSource [i].linearAttenuation;\r\n" \
	"    if (gl_LightSource [i].position.z - z <= ZEYE (texture2D (depthTex, gl_TexCoord [0].xy).r)) {\r\n" \
	"      offset /= screenSize.x;\r\n" \
	"      offset *= (1.0 - pow (abs (offset) * effectStrength.x, effectStrength.y));\r\n" \
	"      tcDest -= v * (gl_LightSource [i].quadraticAttenuation * offset / d * screenSize);\r\n" \
	"      }\r\n" \
	"    }\r\n" \
	"  }\r\n" \
	"gl_FragColor = texture2D (sceneTex, tcDest / screenSize);\r\n" \
	"}\r\n";

#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CPostEffectShockwave::m_nShockwaves = 0;
GLhandleARB CPostEffectShockwave::m_shaderProg;

void CPostEffectShockwave::InitShader (void)
{
if (ogl.m_features.bRenderToTexture && ogl.m_features.bShaders) {
	PrintLog (1, "building shockwave shader program\n");
	m_shaderProg = 0;
	if (!shaderManager.Build (hShockwaveShader, shockwaveFS, shockwaveVS)) {
		ogl.ClearError (0);
		}
	PrintLog (-1);
	}
}

//------------------------------------------------------------------------------

bool CPostEffectShockwave::SetupShader (void)
{
	static CFloatVector3 effectStrength = {{{10.0f, 0.8f, gameData.render.frame.Width () * 0.1f}}};

if (m_nShockwaves > 0)
	return true;
if (hShockwaveShader < 0) {
	InitShader ();
	if (hShockwaveShader < 0)
		return false;
	}
#if DBG
//	ogl.m_states.bDepthBuffer [1] = 0;
#endif
if (!ogl.CopyDepthTexture (1))
	return false;
m_shaderProg = GLhandleARB (shaderManager.Deploy (hShockwaveShader /*[direction]*/));
if (!m_shaderProg)
	return false;
if (shaderManager.Rebuild (m_shaderProg))
	;
shaderManager.Set ("sceneTex", 0);
shaderManager.Set ("depthTex", 1);
shaderManager.Set ("effectStrength", effectStrength);
float viewport [4] = {
#if 1
	0, 0, gameData.render.screen.Width (), gameData.render.screen.Height ()
#else
	gameData.render.frame.Left () + gameData.render.scene.Left (), 
	gameData.render.frame.Top () + gameData.render.scene.Top (), 
	gameData.render.scene.Width (), 
	gameData.render.scene.Height () 
#endif
	};
float screenSize [2] = { gameData.render.screen.Width (), gameData.render.screen.Height () };
shaderManager.Set ("screenSize", screenSize);
ogl.SetLighting (true);
return true;
}

//------------------------------------------------------------------------------

float CPostEffectShockwave::Life (void)
{
return float (m_nLife) * gameStates.gameplay.slowmo [0].fSpeed;
}
	 
//------------------------------------------------------------------------------

bool CPostEffectShockwave::Update (void)
{
int i;
CFixVector p [5];

m_bValid = false;

if (m_nObject >= 0)
	m_pos = OBJECTS [m_nObject].FrontPosition ();
if (transformation.TransformAndEncode (p [0], m_pos) & CC_BEHIND)
	return false;

m_ttl = float (SDL_GetTicks () - m_nStart) / Life ();
m_rad = X2F (m_nSize) * m_ttl;

int size = int (float (m_nSize) * m_ttl);
p [1].v.coord.x = 
p [4].v.coord.x = p [0].v.coord.x - size;
p [1].v.coord.y = 
p [2].v.coord.y = p [0].v.coord.y - size;
p [2].v.coord.x = 
p [3].v.coord.x = p [0].v.coord.x + size;
p [3].v.coord.y = 
p [4].v.coord.y = p [0].v.coord.y + size;
p [1].v.coord.z =
p [2].v.coord.z =
p [3].v.coord.z =
p [4].v.coord.z = p [0].v.coord.z;

for (i = 1; i < 5; i++) {
	if (!(transformation.Codes (p [i]) & CC_BEHIND))
		break;
	}	
if (i == 5)
	return false;

tScreenPos s [5];
for (int i = 0; i < 5; i++) {
	ProjectPoint (p [i], s [i], 0, 0);
	if (gameStates.render.cockpit.nType == CM_LETTERBOX)
		s [i].y += (gameData.render.screen.Height () - CCanvas::Current ()->Height ()) / 2;
	else if (gameStates.render.cockpit.nType != CM_FULL_SCREEN)
		s [i].y += gameData.render.screen.Height () - CCanvas::Current ()->Height ();
	}

#if 1
fix xMin = 0x7FFFFFFF, yMin = 0x7FFFFFFF, xMax = -0x7FFFFFFF, yMax = -0x7FFFFFFF;
for (int i = 1; i < 5; i++) {
	if (xMin > s [i].x)
		xMin = s [i].x;
	if (xMax < s [i].x)
		xMax = s [i].x;
	if (yMin > s [i].y)
		yMin = s [i].y;
	if (yMax < s [i].y)
		yMax = s [i].y;
	}
xMax -= xMin;
yMax -= yMin;
#	ifdef _WIN32
m_screenRad = (float) _hypot (double (xMax), double (yMax)) * 0.125f;
#	else
m_screenRad = (float) hypot (double (xMax), double (yMax)) * 0.125f;
#	endif
#else
int d = 0;
int n = 0;
for (int i = 1; i < 5; i++) {
	if ((s [i].x >= 0) && (s [i].x < gameData.render.screen.Width ()) && (s [i].y >= 0) && (s [i].y < gameData.render.screen.Height ())) {
		d += labs (s [0].x - s [i].x) + labs (s [0].y - s [i].y);
		n += 4;
		}
	}
if (n == 0)
	return false;
m_screenRad = /*sqrt*/ (float (d) / float (n));
#endif

m_renderPos.v.coord.x = float (s [0].x);
m_renderPos.v.coord.y = float (s [0].y);
m_renderPos.v.coord.z = X2F (p [0].v.coord.z);
return m_bValid = true;
}

//------------------------------------------------------------------------------

bool CPostEffectShockwave::Setup (void)
{
if (m_nShockwaves >= 8)
	return true;

if (!SetupShader ())
	return false;

glEnable (GL_LIGHT0 + m_nShockwaves);
CFloatVector3 v = m_renderPos;
v.v.coord.x += gameData.render.frame.Left ();
glLightfv (GL_LIGHT0 + m_nShockwaves, GL_POSITION, reinterpret_cast<GLfloat*> (&v));
glLightf (GL_LIGHT0 + m_nShockwaves, GL_CONSTANT_ATTENUATION, m_screenRad); 
glLightf (GL_LIGHT0 + m_nShockwaves, GL_LINEAR_ATTENUATION, m_rad); // Wellenfront
#if SHOCKWAVE_STYLE
glLightf (GL_LIGHT0 + m_nShockwaves, GL_QUADRATIC_ATTENUATION, (float) pow (0.5f - (float) cos (2.0 * PI * (1.0f - m_ttl)) * 0.5f, 0.25f) * m_nBias); 
#else
glLightf (GL_LIGHT0 + m_nShockwaves, GL_QUADRATIC_ATTENUATION, (float) pow (1.0 - m_ttl, 0.25) * m_nBias);
#endif
//glLighti (GL_LIGHT0 + m_nShockwaves, GL_SPOT_EXPONENT, m_nBias);
shaderManager.Set ("nShockwaves", ++m_nShockwaves);
return true;
}

//------------------------------------------------------------------------------

void CPostEffectShockwave::Render (void)
{
// render current render target
OglDrawArrays (GL_QUADS, 0, 4);
if (m_nShockwaves > 0) {
	for (int i = 0; i < m_nShockwaves; i++)
		glDisable (GL_LIGHT0 + i);
	ogl.SetLighting (false);
	m_nShockwaves = 0;
	}
}

//------------------------------------------------------------------------------

bool CPostEffectShockwave::Enabled (void)
{
return gameOpts->render.effects.bEnabled && (gameOpts->render.effects.nShockwaves > 1);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CPostProcessManager::Destroy (void) 
{
while (m_effects) {
	CPostEffect* e = m_effects;
	m_effects = m_effects->Next ();
	delete e;
	}
m_nEffects = 0;
}

//------------------------------------------------------------------------------

void CPostProcessManager::Add (CPostEffect* e) 
{
if (!(ogl.m_features.bShaders && ogl.m_features.bRenderToTexture && e->Enabled ()))
	delete e;
else {
	e->Link (NULL, m_effects);
	if (m_effects)
		m_effects->Link (e, m_effects->Next ());
	m_effects = e;
	}
}

//------------------------------------------------------------------------------

void CPostProcessManager::Update (void)
{
for (CPostEffect* e = m_effects; e; ) {
	CPostEffect* h = e;
	e = e->Next ();
	if (h->Terminate ()) {
		if (m_effects == h)
			m_effects = m_effects->Next ();
		h->Unlink ();
		delete h;
		}
	else
		h->Update ();
	}
}

//------------------------------------------------------------------------------

bool CPostProcessManager::HaveEffects (void)
{
for (CPostEffect* e = m_effects; e; e = e->Next ()) 
	if (e->Valid ())
		return true;
return false;
}

//------------------------------------------------------------------------------

bool CPostProcessManager::Setup (void)
{
	bool bHaveEffect = false;

for (CPostEffect* e = m_effects; e; e = e->Next ()) 
	if (e->Valid ()) {
		e->Setup ();
		bHaveEffect = true;
	}
return bHaveEffect;
}

//------------------------------------------------------------------------------

void CPostProcessManager::Render (void)
{
for (CPostEffect* e = m_effects; e; e = e->Next ()) 
	if (e->Valid ())
		e->Render ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

