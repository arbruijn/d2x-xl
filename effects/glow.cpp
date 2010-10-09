
#include "descent.h"
#include "error.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "glow.h"

CGlowRenderer glowRenderer;

//------------------------------------------------------------------------------

int hBlurShader [2] = {-1, -1};

const char *blurFS [2] = { 
	"uniform sampler2D glowSource;\r\n" \
	"uniform float scale; // render target width/height\r\n" \
	"//float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);\r\n" \
	"//float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);\r\n" \
	"float offset[5] = float[](0.0, 1.0, 2.0, 3.0, 4.0);\r\n" \
	"float weight[5] = float[](0.18, 0.15, 0.12, 0.09, 0.05);\r\n" \
	"void main() {\r\n" \
	"vec2 uv = gl_TexCoord[0].xy;\r\n" \
	"vec3 tc = texture2D(glowSource, uv).rgb * weight[0];\r\n" \
	"vec2 v = vec2 (0.0, offset[1]*scale);\r\n" \
	"tc += texture2D(glowSource, uv + v).rgb * weight[1];\r\n" \
	"tc += texture2D(glowSource, uv - v).rgb * weight[1];\r\n" \
	"v = vec2 (0.0, offset[2]*scale);\r\n" \
	"tc += texture2D(glowSource, uv + v).rgb * weight[2];\r\n" \
	"tc += texture2D(glowSource, uv - v).rgb * weight[2];\r\n" \
	"v = vec2 (0.0, offset[3]*scale);\r\n" \
	"tc += texture2D(glowSource, uv + v).rgb * weight[3];\r\n" \
	"tc += texture2D(glowSource, uv - v).rgb * weight[3];\r\n" \
	"v = vec2 (0.0, offset[4]*scale);\r\n" \
	"tc += texture2D(glowSource, uv + v).rgb * weight[4];\r\n" \
	"tc += texture2D(glowSource, uv - v).rgb * weight[4];\r\n" \
	"//if (length (tc) > 0.0) tc = vec3 (1.0, 0.5, 0.0) * length (tc);\r\n" \
	"gl_FragColor = vec4(tc, 1.0) * 1.3 /*/ sqrt (weight [0] + weight [1] + weight [2])*/;\r\n" \
	"}\r\n"
	,
	"uniform sampler2D glowSource;\r\n" \
	"uniform float scale; // render target width/height\r\n" \
	"//float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);\r\n" \
	"//float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);\r\n" \
	"float offset[5] = float[](0.0, 1.0, 2.0, 3.0, 4.0);\r\n" \
	"float weight[5] = float[](0.18, 0.15, 0.12, 0.09, 0.05);\r\n" \
	"void main() {\r\n" \
	"vec2 uv = gl_TexCoord[0].xy;\r\n" \
	"vec3 tc = texture2D(glowSource, uv).rgb * weight[0];\r\n" \
	"vec2 v = vec2 (offset[1]*scale, 0.0);\r\n" \
	"tc += texture2D(glowSource, uv + v).rgb * weight[1];\r\n" \
	"tc += texture2D(glowSource, uv - v).rgb * weight[1];\r\n" \
	"v = vec2 (offset[2]*scale, 0.0);\r\n" \
	"tc += texture2D(glowSource, uv + v).rgb * weight[2];\r\n" \
	"tc += texture2D(glowSource, uv - v).rgb * weight[2];\r\n" \
	"v = vec2 (offset[3]*scale, 0.0);\r\n" \
	"tc += texture2D(glowSource, uv + v).rgb * weight[3];\r\n" \
	"tc += texture2D(glowSource, uv - v).rgb * weight[3];\r\n" \
	"v = vec2 (offset[4]*scale, 0.0);\r\n" \
	"tc += texture2D(glowSource, uv + v).rgb * weight[4];\r\n" \
	"tc += texture2D(glowSource, uv - v).rgb * weight[4];\r\n" \
	"//if (length (tc) > 0.0) tc = vec3 (1.0, 0.5, 0.0) * length (tc);\r\n" \
	"gl_FragColor = vec4(tc, 1.0) * 1.3 /*/ sqrt (weight [0] + weight [1] + weight [2])*/;\r\n" \
	"}\r\n"
	};

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

m_shaderProg = GLhandleARB (shaderManager.Deploy (hBlurShader [direction]));
if (!m_shaderProg)
	return false;
if (shaderManager.Rebuild (m_shaderProg))
	;
glUniform1i (glGetUniformLocation (m_shaderProg, "glowSource"), 0);
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
	for (int i = 0; i < 2; i++) {
		if (!shaderManager.Build (hBlurShader [i], blurFS [i], blurVS)) {
			ogl.ClearError (0);
			ogl.m_states.bDepthBlending = 0;
			}
		}
	}
}

//-------------------------------------------------------------------------

bool CGlowRenderer::ShaderActive (void)
{
for (int i = 0; i < 2; i++)
	if ((hBlurShader [i] >= 0) && (shaderManager.Current () == hBlurShader [i]))
		return true;
return false;
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

#define BLUR 1
#define START_RAD 3.0f
#define RAD_INCR 3.0f

void CGlowRenderer::Flush (bool bReplace)
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
float radius = START_RAD;
ogl.SelectBlurBuffer (0, m_vMin, m_vMax, radius); 
Render (-1, 0, radius); // Glow -> Blur 0
ogl.SelectBlurBuffer (1, m_vMin, m_vMax, radius); 
Render (0, 1, radius); // Blur 0 -> Blur 1
#	if BLUR > 1
radius += RAD_INCR;
ogl.SelectBlurBuffer (0, m_vMin, m_vMax, radius); 
Render (-1, 0, radius); // Glow -> Blur 0
ogl.SelectBlurBuffer (1, m_vMin, m_vMax, radius); 
Render (0, 1, radius); // Blur 0 -> Blur 1
#		if BLUR > 2
radius += RAD_INCR;
ogl.SelectBlurBuffer (0, m_vMin, m_vMax, radius); 
Render (-1, 0, radius); // Glow -> Blur 0
ogl.SelectBlurBuffer (1, m_vMin, m_vMax, radius); 
Render (0, 1, radius); // Blur 0 -> Blur 1
#		endif
#	endif
#endif
ogl.ChooseDrawBuffer ();
ogl.SetDepthMode (GL_LEQUAL);
//ogl.SetBlendMode (GL_ONE, GL_ZERO);
ogl.SetBlendMode (2);
#if BLUR
Render (1); // Glow -> back buffer
#endif
if (!bReplace)
	Render (-1); // render the unblurred stuff on top of the blur

glPopMatrix ();
glMatrixMode (GL_PROJECTION);
glPopMatrix ();
}

//------------------------------------------------------------------------------

void CGlowRenderer::Project (CFloatVector& v)
{
g3sPoint p;
CFixVector h;
h.Assign (v);
ubyte code = transformation.TransformAndEncode (p.p3_vec, h);
if (code & CC_OFF_LEFT)
	v [X] = 0.0f;
else if (code & CC_OFF_RIGHT)
	v [X] = screen.Width ();
else {
	G3ProjectPoint (&p);
	v [X] = (float) p.p3_screen.x;
	v [Y] = (float) p.p3_screen.y;
	}
}

//------------------------------------------------------------------------------

void CGlowRenderer::SetExtent (CFloatVector* vertexP, int nVerts)
{
CFloatVector vMin, vMax;
vMin.Create (1e30f, 1e30f, 1e30f);
vMax.Create (-1e30f, -1e30f, -1e30f);

for (; nVerts; nVerts--, vertexP++) {
	if (vMin [X] > (*vertexP) [X])
		vMin [X] = (*vertexP) [X];
	if (vMin [Y] > (*vertexP) [Y])
		vMin [Y] = (*vertexP) [Y];
	if (vMin [Z] > (*vertexP) [Z])
		vMin [Z] = (*vertexP) [Z];
	if (vMax [X] < (*vertexP) [X])
		vMax [X] = (*vertexP) [X];
	if (vMax [Y] < (*vertexP) [Y])
		vMax [Y] = (*vertexP) [Y];
	if (vMax [Z] < (*vertexP) [Z])
		vMax [Z] = (*vertexP) [Z];
	}
Project (vMin);
Project (vMax);
}

//------------------------------------------------------------------------------

