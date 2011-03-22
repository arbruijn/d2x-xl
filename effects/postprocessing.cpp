/* Image post processing (fullscreen effects)
	Dietfrid Mali
	22.3.2011
*/

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

const char* shockwaveFS = 
	"uniform sampler2D sceneTex;\r\n" \
	"uniform vec2 center; // explosion center\r\n" \
	"uniform float time; // effect elapsed time\r\n" \
	"uniform vec3 shockParams; // 10.0, 0.8, 0.1\r\n" \
	"void main() {\r\n" \
	"vec2 texCoord = gl_TexCoord [0].xy;\r\n" \
	"float distance = distance (texCoord, center);\r\n" \
	"if ((distance <= (time + shockParams.z)) && (distance >= (time - shockParams.z))) {\r\n" \
	"  float diff = (distance - time);\r\n" \
	"  float powDiff = 1.0 - pow (abs (diff * shockParams.x), shockParams.y);\r\n" \
	"  texCoord += normalize (texCoord - center) * diff * powDiff;\r\n" \
	"  }\r\n" \
	"gl_FragColor = texture2D (sceneTex, texCoord);\r\n" \
	"}\r\n";

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


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
e->Link (NULL, m_effects);
if (m_effects)
	m_effects->Link (e, m_effects->Next ());
m_effects = e;
}

//------------------------------------------------------------------------------

void CPostProcessManager::Render (void)
{
}

//------------------------------------------------------------------------------

void CPostProcessManager::Update (void)
{
for (CPostEffect* e = m_effects; e; ) {
	CPostEffect* h = e;
	e = e->Next ();
	if (h->Terminate ())
		h->Unlink ();
	else
		h->Update ();
	}
for (CPostEffect* e = m_effects; e; )
	e->Render ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CPostEffectShockwave::InitShader (void)
{
PrintLog ("building shockwave shader program\n");
if (ogl.m_states.bRender2TextureOk && ogl.m_states.bShadersOk) {
	m_shaderProg = 0;
	if (!shaderManager.Build (hShockwaveShader, shockwaveFS, shockwaveVS)) {
		ogl.ClearError (0);
		}
	}
}

//------------------------------------------------------------------------------

bool CPostEffectShockwave::LoadShader (const CFixVector pos, const int size)
{
if (m_nExplosions == 0) {
	m_shaderProg = GLhandleARB (shaderManager.Deploy (hShockwaveShader /*[direction]*/));
	if (!m_shaderProg)
		return false;
	if (shaderManager.Rebuild (m_shaderProg))
		;
	glUniform1i (glGetUniformLocation (m_shaderProg, "renderSource"), 0);
glUniform1f (glGetUniformLocation (m_shaderProg, "radius"), GLint (radius));
glUniform1f (glGetUniformLocation (m_shaderProg, "scale"), GLfloat (fScale [direction]));
glUniform1f (glGetUniformLocation (m_shaderProg, "brightness"), GLfloat (m_brightness));
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

