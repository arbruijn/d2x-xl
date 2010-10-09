
#include "descent.h"
#include "error.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "glow.h"

CGlowRenderer glowRenderer;

//------------------------------------------------------------------------------

int hBlurShader = -1;

const char *blurFS = 
	"uniform sampler2D glowSource;\r\n" \
	"uniform float scale; // render target width/height\r\n" \
	"uniform int direction;\r\n" \
	"float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);\r\n" \
	"float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);\r\n" \
	"void main() {\r\n" \
	"vec2 uv = gl_TexCoord[0].xy;\r\n" \
	"vec3 tc = texture2D(glowSource, uv).rgb * weight[0];\r\n" \
	"if (direction == 0) {\r\n" \
	"   vec2 v = vec2 (0.0, offset[1]*scale);\r\n" \
	"   tc += texture2D(glowSource, uv + v).rgb * weight[1];\r\n" \
	"   tc += texture2D(glowSource, uv - v).rgb * weight[1];\r\n" \
	"   v = vec2 (0.0, offset[2]*scale);\r\n" \
	"   tc += texture2D(glowSource, uv + v).rgb * weight[2];\r\n" \
	"   tc += texture2D(glowSource, uv - v).rgb * weight[2];\r\n" \
	"  }\r\n" \
	"else {\r\n" \
	"   vec2 v = vec2 (offset[1]*scale, 0.0);\r\n" \
	"   tc += texture2D(glowSource, uv + v).rgb * weight[1];\r\n" \
	"   tc += texture2D(glowSource, uv - v).rgb * weight[1];\r\n" \
	"   v = vec2 (offset[2]*scale, 0.0);\r\n" \
	"   tc += texture2D(glowSource, uv + v).rgb * weight[2];\r\n" \
	"   tc += texture2D(glowSource, uv - v).rgb * weight[2];\r\n" \
	"   }\r\n" \
	"//if (length (tc) > 0.0) tc = vec3 (1.0, 0.5, 0.0) * length (tc);\r\n" \
	"gl_FragColor = vec4(tc, 1.0) / (weight [0] + weight [1] + weight [2]);\r\n" \
	"}\r\n"
	;

const char *blurVS =
	"void main (void){\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"gl_Position = ftransform (); //gl_ModelViewProjectionMatrix * gl_Vertex;\r\n" \
	"gl_FrontColor = gl_Color;}\r\n"
	;

//------------------------------------------------------------------------------

bool CGlowRenderer::LoadShader (int const direction, float const radius)
{
	float fScale [2] = {ogl.m_data.screenScale.y * radius, ogl.m_data.screenScale.x * radius};

m_shaderProg = GLhandleARB (shaderManager.Deploy (hBlurShader));
if (!m_shaderProg)
	return false;
if (shaderManager.Rebuild (m_shaderProg))
	;
glUniform1i (glGetUniformLocation (m_shaderProg, "glowSource"), 0);
glUniform1i (glGetUniformLocation (m_shaderProg, "direction"), direction);
glUniform1f (glGetUniformLocation (m_shaderProg, "scale"), GLfloat (fScale [direction]));
return true;
}

//-------------------------------------------------------------------------

void CGlowRenderer::InitShader (void)
{
ogl.m_states.bDepthBlending = 0;
PrintLog ("building glow shader program\n");
//DeleteShaderProg (NULL);
if (ogl.m_states.bRender2TextureOk && ogl.m_states.bShadersOk) {
	m_shaderProg = 0;
	if (!shaderManager.Build (hBlurShader, blurFS, blurVS)) {
		ogl.ClearError (0);
		ogl.m_states.bDepthBlending = 0;
		}
	}
}

//-------------------------------------------------------------------------

bool CGlowRenderer::ShaderActive (void)
{
return (hBlurShader >= 0) && (shaderManager.Current () == hBlurShader);
}

//------------------------------------------------------------------------------

void CGlowRenderer::Render (int const source, int const direction, float const radius)
{
	static tTexCoord2f texCoord [4] = {{{0,0}},{{0,1}},{{1,1}},{{1,0}}};
	static float verts [4][2] = {{0,0},{0,1},{1,1},{1,0}};

ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
if (direction >= 0)
	LoadShader (direction, radius);
else
	shaderManager.Deploy (-1);
ogl.BindTexture (ogl.BlurBuffer (source)->ColorBuffer ());
OglTexCoordPointer (2, GL_FLOAT, 0, texCoord);
OglVertexPointer (2, GL_FLOAT, 0, verts);
glColor3f (1,1,1);
OglDrawArrays (GL_QUADS, 0, 4);
ogl.BindTexture (0);
}

//------------------------------------------------------------------------------

#define BLUR 3

void CGlowRenderer::Flush (void)
{
glMatrixMode (GL_PROJECTION);
glPushMatrix ();
glLoadIdentity ();//clear matrix
glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
glLoadIdentity ();//clear matrix

ogl.SetDepthMode (GL_ALWAYS);
ogl.SetDepthWrite (false);
#if BLUR
ogl.SetBlendMode (GL_ONE, GL_ZERO);
ogl.SelectBlurBuffer (0); 
float radius = 2.0f;
Render (-1, 0, radius); // Glow -> Blur 0
ogl.SelectBlurBuffer (1); 
Render (0, 1, radius); // Blur 0 -> Blur 1
#	if BLUR > 1
radius += 1.0f;
ogl.SelectBlurBuffer (0); 
Render (1, 0, radius); // Blur 1 -> Blur 0
ogl.SelectBlurBuffer (1); 
Render (0, 1, radius); // Blur 0 -> Blur 1
#		if BLUR > 2
radius += 1.0f;
ogl.SelectBlurBuffer (0); 
Render (1, 0, radius); // Blur 1 -> Blur 0
ogl.SelectBlurBuffer (1); 
Render (0, 1, radius); // Blur 0 -> Blur 1
#		endif
#	endif
#endif
ogl.ChooseDrawBuffer ();
ogl.SetDepthMode (GL_LEQUAL);
//ogl.SetBlendMode (GL_ONE, GL_ZERO);
//Render (-1); // Blur 0 -> back buffer
#if BLUR
ogl.SetBlendMode (2);
Render (1); // Glow -> back buffer
#endif

glPopMatrix ();
glMatrixMode (GL_PROJECTION);
glPopMatrix ();
}

//------------------------------------------------------------------------------

