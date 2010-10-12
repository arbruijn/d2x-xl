
#include "descent.h"
#include "error.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "glow.h"

CGlowRenderer glowRenderer;

#define USE_VIEWPORT 1
#define BLUR 2
#define START_RAD 3.0f
#define RAD_INCR 3.0f

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

void CGlowRenderer::Activate (void)
{
ogl.SelectGlowBuffer ();
m_x = ogl.m_states.nLastX;
m_y = ogl.m_states.nLastY;
m_w = ogl.m_states.nLastW;
m_h = ogl.m_states.nLastH;
//ogl.Viewport ((GLint) (m_vMin [X]), (GLint) (m_vMin [Y]), (GLint) (m_vMax [X] - m_vMin [X]) + 1, (GLint) (m_vMax [Y] - m_vMin [Y]) + 1);
glClear (GL_COLOR_BUFFER_BIT);
//ogl.Viewport (m_x, m_y, m_w, m_h);
}

//------------------------------------------------------------------------------

void CGlowRenderer::SetExtent (CFloatVector3 v, bool bTransformed)
{
#if USE_VIEWPORT
if (!bTransformed)
	transformation.Transform (v, v);
CFloatVector w;
w.Assign (v);
w [3] = 1.0f;
w = m_projection * w;
tScreenPos s;

s.x = fix (fxCanvW2 + float (v [X]) * fxCanvW2 / v [Z]);
s.y = fix (fxCanvH2 + float (v [Y]) * fxCanvH2 / v [Z]);
tScreenPos t;
t.x = fix (fxCanvW2 + float (w [X]) * fxCanvW2 / -w [Z]);
t.y = fix (fxCanvH2 + float (w [Y]) * fxCanvH2 / -w [Z]);
//s.y = screen.Height () - s.y;
#if DBG == 0
#pragma omp critical (glowRender)
#endif
	{
	if (m_screenMin.x > t.x)
		m_screenMin.x = t.x;
	if (m_screenMin.y > t.y)
		m_screenMin.y = t.y;
	if (m_screenMax.x < t.x)
		m_screenMax.x = t.x;
	if (m_screenMax.y < t.y)
		m_screenMax.y = t.y;
	}
#endif
}

//------------------------------------------------------------------------------

void CGlowRenderer::InitViewPort (void)
{
if (!m_bViewPort) {
	m_bViewPort = true;
	m_screenMin.x = m_screenMin.y = 0x7FFF;
	m_screenMax.x = m_screenMax.y = -0x7FFF;
	}
}

//------------------------------------------------------------------------------

void CGlowRenderer::ViewPort (CFloatVector3* vertexP, int nVerts)
{
#if USE_VIEWPORT
SetupProjection ();
for (; nVerts > 0; nVerts--)
	SetExtent (*vertexP++);
#endif
}

//------------------------------------------------------------------------------

void CGlowRenderer::ViewPort (CFloatVector* vertexP, int nVerts)
{
#if USE_VIEWPORT
SetupProjection ();
#if DBG == 0
#pragma omp parallel 
#endif
{
#if DBG == 0
#	pragma omp for
#endif
for (int i = 0; i < nVerts; i++)
	SetExtent (*vertexP [i].XYZ ());
}
#endif
}

//------------------------------------------------------------------------------

void CGlowRenderer::ViewPort (CFloatVector3 v, float width, float height)
{
#if USE_VIEWPORT
transformation.Transform (v, v);
CFloatVector3 r;
r.Set (width, height, 0.0f);
SetExtent (v - r, true);
SetExtent (v + r, true);
#endif
}

//------------------------------------------------------------------------------

void CGlowRenderer::ViewPort (CFixVector pos, float radius)
{
#if USE_VIEWPORT
CFloatVector3 v;
v.Assign (pos);
ViewPort (v, radius, radius);
#endif
}

//------------------------------------------------------------------------------

bool CGlowRenderer::Available (bool bForce)
{
if (!ogl.m_states.bGlowRendering)
	return false;
if (!gameOpts->render.effects.bEnabled)
	return false;
if (!(bForce || gameOpts->render.effects.bGlow))
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
	m_bViewPort = false;
	InitViewPort ();
	Activate ();
	}
return true;
}

//------------------------------------------------------------------------------

inline float ScreenCoord (float v, float m)
{
if (v < 0.0f)
	return 0.0f;
if (v > m)
	return 1.0f;
return v / m;
}

//------------------------------------------------------------------------------

void CGlowRenderer::Render (int const source, int const direction, float const radius)
{
#if USE_VIEWPORT //DBG

float r = radius + 1.0f;
float verts [4][2] = {
	{ScreenCoord ((float) m_screenMin.x - r, (float) screen.Width ()),
	 ScreenCoord ((float) m_screenMin.y - r, (float) screen.Height ())},
	{ScreenCoord ((float) m_screenMin.x - r, (float) screen.Width ()),
	 ScreenCoord ((float) m_screenMax.y + r, (float) screen.Height ())},
	{ScreenCoord ((float) m_screenMax.x + r, (float) screen.Width ()),
	 ScreenCoord ((float) m_screenMax.y + r, (float) screen.Height ())},
	{ScreenCoord ((float) m_screenMax.x + r, (float) screen.Width ()),
	 ScreenCoord ((float) m_screenMin.y - r, (float) screen.Height ())}
	};

#else

float verts [4][2] = {{0,0},{0,1},{1,1},{1,0}};

#endif

//ogl.Viewport ((GLint) (m_screenMin.x - r), 
//				  (GLint) (m_screenMin.y - r), 
//				  (GLint) (m_screenMax.x - m_screenMin.x + 1 + 2 * r), 
//				  (GLint) ((m_screenMax.y - m_screenMin.y + 1 + 2 * r)));
ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
//if (direction >= 0)
//	LoadShader (direction, radius);
//else
	shaderManager.Deploy (-1);
ogl.BindTexture (ogl.BlurBuffer (source)->ColorBuffer ());
//glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
OglTexCoordPointer (2, GL_FLOAT, 0, verts);
OglVertexPointer (2, GL_FLOAT, 0, verts);
glColor3f (1,1,1);
OglDrawArrays (GL_QUADS, 0, 4);
ogl.BindTexture (0);
}

//------------------------------------------------------------------------------

bool CGlowRenderer::End (void)
{
if (m_nStrength < 0)
	return false;

#if USE_VIEWPORT
if (m_screenMin.x > m_screenMax.x)
	Swap (m_screenMin.x, m_screenMax.x);
if (m_screenMin.y > m_screenMax.y)
	Swap (m_screenMin.y, m_screenMax.y);

if ((m_screenMax.x <= m_screenMin.x) || (m_screenMax.y <= m_screenMin.y)) 
	ogl.ChooseDrawBuffer ();
else
#endif
	{
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();//clear matrix
	glMatrixMode (GL_PROJECTION);
	glPushMatrix ();
	glLoadIdentity ();//clear matrix
	glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

	GLenum nBlendModes [2], nDepthMode = ogl.GetDepthMode ();
	bool bDepthWrite = ogl.GetDepthWrite ();
	ogl.GetBlendMode (nBlendModes [0], nBlendModes [1]);

	ogl.SetDepthWrite (false);
	ogl.ResetClientStates (1);

#if BLUR
	ogl.SetDepthMode (GL_ALWAYS);
	//ogl.SetFaceCulling (false);

	ogl.SetBlendMode (GL_ONE, GL_ZERO);
	float radius = START_RAD;
	ogl.SelectBlurBuffer (0); 
	Render (-1, 0, radius); // Glow -> Blur 0
	ogl.SelectBlurBuffer (1); 
	Render (0, 1, radius); // Blur 0 -> Blur 1
#	if BLUR > 1
	for (int i = 1; i < m_nStrength; i++) {
		//ogl.SetBlendMode (GL_ONE, GL_ONE);
		radius += RAD_INCR;
		ogl.SelectBlurBuffer (0); 
		Render (1, 0, radius); // Blur 1 -> Blur 0
		ogl.SelectBlurBuffer (1); 
		Render (0, 1, radius); // Blur 0 -> Blur 1
		}
#	endif
#endif

	ogl.ChooseDrawBuffer ();
	ogl.SetDepthMode (GL_LEQUAL);
	ogl.SetBlendMode (2);
#if BLUR
	Render (1); // Glow -> back buffer
#endif
	if (!m_bReplace)
		Render (-1); // render the unblurred stuff on top of the blur

	glMatrixMode (GL_PROJECTION);
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();

	ogl.SetBlendMode (nBlendModes [0], nBlendModes [1]);
	ogl.SetDepthWrite (bDepthWrite);
	ogl.SetDepthMode (nDepthMode);
	//ogl.SetFaceCulling (true);
	}

m_nStrength = -1;
m_bViewPort = false;
return true;
}

//------------------------------------------------------------------------------

