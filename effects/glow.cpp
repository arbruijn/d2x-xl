
#include "descent.h"
#include "error.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "glow.h"

CGlowRenderer glowRenderer;

//------------------------------------------------------------------------------
// 7x1 gaussian blur fragment shader

int hBlurShader = -1;

const char *blurFS =
	"varying vec2 vCoordinates;\r\n" \
	"// u_Scale is vec2 (0.0, 1.0/height) for vertical blur and vec2 (1.0/width, 0.0) for horizontal blur.\r\n" \
	"uniform vec2 uScale;\r\n" \
	"uniform sampler2D uTexture0;\r\n" \
	"const vec2 gaussFilter [7] = { \r\n" \
	"	-3.0,	0.015625,\r\n" \
	"	-2.0,	0.09375,\r\n" \
	"	-1.0,	0.234375,\r\n" \
	"	 0.0,	0.3125,\r\n" \
	"	 1.0,	0.234375,\r\n" \
	"	 2.0,	0.09375,\r\n" \
	"	 3.0,	0.015625\r\n" \
	"	};\r\n" \
	"void main() {\r\n" \
	"	vec4 color = 0.0;\r\n" \
	"	for (int i = 0; i < 7; i++)\r\n" \
	"		color += texture2D (uTexture0, vec2 (vCoordinates.x + gaussFilter [i].x * uScale.x, vCoordinates.y + gaussFilter [i].x * uScale.y)) * gaussFilter [i].y;\r\n" \
	"  if (length (color) == 0.0)\r\n" \
	"    discard;\r\n" \
	"  else\r\n" \
	"	  gl_FragColor = color;\r\n" \
	"}"
	;

const char *blurVS =
	"void main (void){\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"gl_Position = ftransform (); //gl_ModelViewProjectionMatrix * gl_Vertex;\r\n" \
	"gl_FrontColor = gl_Color;}\r\n"
	;

//------------------------------------------------------------------------------

bool CGlowRenderer::LoadShader (int const direction)
{
	float uScale [2][2] = {{ogl.m_data.screenScale.x, 0.0f}, {0.0f, ogl.m_data.screenScale.y}};

m_shaderProg = GLhandleARB (shaderManager.Deploy (hBlurShader));
if (!m_shaderProg)
	return false;
if (shaderManager.Rebuild (m_shaderProg)) {
	glUniform1i (glGetUniformLocation (m_shaderProg, "glowTex"), 0);
	glUniform2fv (glGetUniformLocation (m_shaderProg, "uScale"), 1, reinterpret_cast<GLfloat*> (&uScale [direction]));
	}
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

void CGlowRenderer::Draw (int const direction)
{
	static tTexCoord2f texCoord [4] = {{{0,0}},{{0,1}},{{1,1}},{{1,0}}};
	static float verts [4][2] = {{0,0},{0,1},{1,1},{1,0}};

ogl.BindTexture (ogl.DrawBuffer (2 + direction)->ColorBuffer ());
OglTexCoordPointer (2, GL_FLOAT, 0, texCoord);
OglVertexPointer (2, GL_FLOAT, 0, verts);
glColor3f (1,1,1);
OglDrawArrays (GL_QUADS, 0, 4);
}

//------------------------------------------------------------------------------

bool CGlowRenderer::Blur (int const direction)
{
if (!LoadShader (direction)) 
	return false;
ogl.SetDrawBuffer (GL_BACK, 3 - direction);
ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
Draw (direction);
return true;
}

//------------------------------------------------------------------------------

void CGlowRenderer::Render (void)
{
return;
Blur (0);
Blur (1);
shaderManager.Deploy (-1);
ogl.ChooseDrawBuffer ();
Draw (0);
}

//------------------------------------------------------------------------------

