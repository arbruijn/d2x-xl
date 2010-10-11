
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
	"uniform float brightness; // render target width/height\r\n" \
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
	"gl_FragColor = vec4(tc, 1.0) * brightness;\r\n" \
	"}\r\n"
	,
	"uniform sampler2D glowSource;\r\n" \
	"uniform float scale; // render target width/height\r\n" \
	"uniform float brightness; // render target width/height\r\n" \
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
	"gl_FragColor = vec4(tc, 1.0) * brightness;\r\n" \
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
glUniform1f (glGetUniformLocation (m_shaderProg, "brightness"), GLfloat (m_brightness));
return true;
}

//-------------------------------------------------------------------------

void CGlowRenderer::InitShader (void)
{
ogl.m_states.bGlowRendering = 0;
PrintLog ("building glow shader program\n");
//DeleteShaderProg (NULL);
if (ogl.m_states.bRender2TextureOk && ogl.m_states.bShadersOk) {
	ogl.m_states.bGlowRendering = 1;
	m_shaderProg = 0;
	for (int i = 0; i < 2; i++) {
		if (!shaderManager.Build (hBlurShader [i], blurFS [i], blurVS)) {
			ogl.ClearError (0);
			ogl.m_states.bGlowRendering = 0;
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

void CGlowRenderer::Project (CFloatVector3& v)
{
g3sPoint p;
CFixVector h;
h.Assign (v);
p.p3_vec.Assign (v);
p.p3_codes = 0;
G3ProjectPoint (&p);
if (p.p3_screen.x < 0)
	v [X] = 0;
else if (p.p3_screen.x > screen.Width ())
	v [X] = screen.Width ();
else
	v [X] = (float) p.p3_screen.x;

if (p.p3_screen.y < 0)
	v [Y] = 0;
else if (p.p3_screen.y > screen.Width ())
	v [Y] = screen.Width ();
else
	v [Y] = (float) p.p3_screen.y;
}

//------------------------------------------------------------------------------

void CGlowRenderer::Activate (void)
{
ogl.SelectGlowBuffer ();
m_x = ogl.m_states.nLastX;
m_y = ogl.m_states.nLastY;
m_w = ogl.m_states.nLastW;
m_h = ogl.m_states.nLastH;
if (m_vMin [X] > m_vMax [X])
	Swap (m_vMin [X], m_vMax [X]);
if (m_vMin [Y] > m_vMax [Y])
	Swap (m_vMin [Y], m_vMax [Y]);
//ogl.Viewport ((GLint) (m_vMin [X]), (GLint) (m_vMin [Y]), (GLint) (m_vMax [X] - m_vMin [X]) + 1, (GLint) (m_vMax [Y] - m_vMin [Y]) + 1);
glClear (GL_COLOR_BUFFER_BIT);
//ogl.Viewport (m_x, m_y, m_w, m_h);
}

//------------------------------------------------------------------------------

void CGlowRenderer::SetExtent (CFloatVector3& v)
{
CFloatVector3 h;
transformation.Transform (h, v);
if (m_vMin [X] > v [X])
	m_vMin [X] = v [X];
if (m_vMin [Y] > v [Y])
	m_vMin [Y] = v [Y];
if (m_vMin [Z] > v [Z])
	m_vMin [Z] = v [Z];
if (m_vMax [X] < v [X])
	m_vMax [X] = v [X];
if (m_vMax [Y] < v [Y])
	m_vMax [Y] = v [Y];
if (m_vMax [Z] < v [Z])
	m_vMax [Z] = v [Z];
}

//------------------------------------------------------------------------------

void CGlowRenderer::ViewPort (CFloatVector3* vertexP, int nVerts)
{
m_vMin.Set (1e30f, 1e30f, 1e30f);
m_vMax.Set (-1e30f, -1e30f, -1e30f);

for (; nVerts > 0; nVerts--)
	SetExtent (*vertexP++);
Project (m_vMin);
Project (m_vMax);
}

//------------------------------------------------------------------------------

void CGlowRenderer::ViewPort (CFloatVector3 pos, float width, float height)
{
CFloatVector3 v, r;
transformation.Transform (v, pos);
r.Set (width, height, 0.0);
m_vMin = v - r;
m_vMax = v + r;
Project (m_vMin);
Project (m_vMax);
}

//------------------------------------------------------------------------------

void CGlowRenderer::ViewPort (CFixVector pos, float radius)
{
CFloatVector3 v;
v.Assign (pos);
ViewPort (v, radius, radius);
}

//------------------------------------------------------------------------------

bool CGlowRenderer::Available (void)
{
if (!ogl.m_states.bGlowRendering)
	return false;
if (gameOptions [0].render.nQuality < 2)
	return false;
return true;
}

//------------------------------------------------------------------------------

bool CGlowRenderer::Begin (int const nStrength, bool const bReplace, float const brightness)
{
if (!Available ())
	return false;
if ((m_bReplace != bReplace) || (m_nStrength != nStrength) || (m_brightness != brightness)) {
	End ();
	m_bReplace = bReplace;
	m_nStrength = nStrength;
	m_brightness = brightness;
	m_vMin.Set (0, 0, 0);
	m_vMax.Set (screen.Width (), screen.Height (), 0);
	Activate ();
	}
return true;
}

//------------------------------------------------------------------------------

void CGlowRenderer::Render (int const source, int const direction, float const radius)
{
#if 0
	static tTexCoord2f texCoord [4] = {{{0,0}},{{0,1}},{{1,1}},{{1,0}}};
	static float verts [4][2] = {{0,0},{0,1},{1,1},{1,0}};
#else
	float verts [4][2] = {
		{m_vMin [X] / (float) screen.Width (), m_vMin [Y] / (float) screen.Height ()},
		{m_vMin [X] / (float) screen.Width (), m_vMax [Y] / (float) screen.Height ()},
		{m_vMax [X] / (float) screen.Width (), m_vMax [Y] / (float) screen.Height ()},
		{m_vMax [X] / (float) screen.Width (), m_vMin [Y] / (float) screen.Height ()}
	};
#endif

ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
if (direction >= 0)
	LoadShader (direction, radius);
else
	shaderManager.Deploy (-1);
ogl.BindTexture (ogl.BlurBuffer (source)->ColorBuffer ());
OglTexCoordPointer (2, GL_FLOAT, 0, verts);
OglVertexPointer (2, GL_FLOAT, 0, verts);
glColor3f (1,1,1);
OglDrawArrays (GL_QUADS, 0, 4);
ogl.BindTexture (0);
}

//------------------------------------------------------------------------------

#define BLUR 1
#define START_RAD 3.0f
#define RAD_INCR 3.0f

bool CGlowRenderer::End (void)
{
if (m_nStrength < 0)
	return false;
glMatrixMode (GL_PROJECTION);
glPushMatrix ();
glLoadIdentity ();//clear matrix
glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
glLoadIdentity ();//clear matrix

GLenum nBlendModes [2], nDepthMode = ogl.GetDepthMode ();
bool bDepthWrite = ogl.GetDepthWrite ();
ogl.GetBlendMode (nBlendModes [0], nBlendModes [1]);

ogl.SetDepthMode (GL_ALWAYS);
ogl.SetDepthWrite (false);

#if BLUR

ogl.SetBlendMode (GL_ONE, GL_ZERO);
float radius = START_RAD;
ogl.SelectBlurBuffer (0, m_vMin, m_vMax, radius); 
Render (-1, 0, radius); // Glow -> Blur 0
ogl.SelectBlurBuffer (1, m_vMin, m_vMax, radius); 
Render (0, 1, radius); // Blur 0 -> Blur 1
for (int i = 1; i < m_nStrength; i++) {
	ogl.SetBlendMode (GL_ONE, GL_ONE);
	radius += RAD_INCR;
	ogl.SelectBlurBuffer (0, m_vMin, m_vMax, radius); 
	Render (-1, 0, radius); // Glow -> Blur 0
	ogl.SelectBlurBuffer (1, m_vMin, m_vMax, radius); 
	Render (0, 1, radius); // Blur 0 -> Blur 1
	}

#endif

ogl.ChooseDrawBuffer ();
ogl.SetDepthMode (GL_LEQUAL);
//ogl.SetBlendMode (GL_ONE, GL_ZERO);
ogl.SetBlendMode (2);
#if BLUR
Render (1); // Glow -> back buffer
#endif
if (!m_bReplace)
	Render (-1); // render the unblurred stuff on top of the blur

glPopMatrix ();
glMatrixMode (GL_PROJECTION);
glPopMatrix ();

ogl.SetBlendMode (nBlendModes [0], nBlendModes [1]);
ogl.SetDepthWrite (bDepthWrite);
ogl.SetDepthMode (nDepthMode);

m_nStrength = -1;
return true;
}

//------------------------------------------------------------------------------

