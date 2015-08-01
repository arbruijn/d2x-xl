
#include "descent.h"
#include "error.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "glow.h"

CGlowRenderer glowRenderer;

#define USE_VIEWPORT 1
#define BLUR 1
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
if (gameStates.render.cameras.bActive)
	cameraManager.Camera (cameraManager.Current ())->DisableBuffer (false);
ogl.SelectGlowBuffer ();
glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//------------------------------------------------------------------------------

inline int ScreenScale (void)
{
return (!gameStates.render.cameras.bActive || gameOpts->render.cameras.bHires) ? 1 : 2;
}

//------------------------------------------------------------------------------

inline int ScreenWidth (void)
{
return screen.Width () /*/ ScreenScale ()*/;
}

inline int ScreenHeight (void)
{
return screen.Height () /*/ ScreenScale ()*/;
}

//------------------------------------------------------------------------------

void CGlowRenderer::SetExtent (CFloatVector3 v, bool bTransformed)
{
#if USE_VIEWPORT
if (gameOpts->render.effects.bGlow != 1)
	return;
if (!bTransformed)
	transformation.Transform (v, v);
v = transformation.m_info.projection * v;
float z = -v [Z];
tScreenPos s;
float w = (float) screen.Width () / 2.0f;
float h = (float) screen.Height () / 2.0f;
s.x = fix (w + float (v [X]) * w / z);
s.y = fix (h + float (v [Y]) * h / z);
#pragma omp critical
if (m_screenMin.x > s.x)
	m_screenMin.x = s.x;
if (m_screenMin.y > s.y)
	m_screenMin.y = s.y;
if (m_screenMax.x < s.x)
	m_screenMax.x = s.x;
if (m_screenMax.y < s.y)
	m_screenMax.y = s.y;
#endif
m_bViewport = 1;
}

//------------------------------------------------------------------------------

bool CGlowRenderer::Visible (void)
{
#if USE_VIEWPORT
if (gameOpts->render.effects.bGlow != 1)
	return true;
if (m_bViewport != 1)
	return false;
if (m_screenMin.x > m_screenMax.x)
	Swap (m_screenMin.x, m_screenMax.x);
if (m_screenMin.y > m_screenMax.y)
	Swap (m_screenMin.y, m_screenMax.y);

return (m_screenMax.x > 0) && (m_screenMin.x < ScreenWidth () - 1) &&
		 (m_screenMax.y > 0) && (m_screenMin.y < ScreenHeight () - 1) &&
		 (m_screenMax.x > m_screenMin.x) && (m_screenMax.y > m_screenMin.y);
#else
return true;
#endif
}

//------------------------------------------------------------------------------

void CGlowRenderer::InitViewport (void)
{
if (gameOpts->render.effects.bGlow != 1) {
	m_screenMin.x = m_screenMin.y = 0;
	m_screenMax.x = ScreenWidth ();
	m_screenMax.y = ScreenHeight ();
	}	
else if (!m_bViewport) {
	m_bViewport = -1;
	m_screenMin.x = m_screenMin.y = 0x7FFF;
	m_screenMax.x = m_screenMax.y = -0x7FFF;
	}
}

//------------------------------------------------------------------------------

bool CGlowRenderer::SetViewport (int const nType, CFloatVector3* vertexP, int nVerts)
{
if (!Available (nType))
	return true;
if ((GLOW_FLAGS & nType) == 0)
	return false;
#if USE_VIEWPORT
//if (gameStates.render.cameras.bActive)
//	return true;
if (gameOpts->render.effects.bGlow != 1)
	return true;
#pragma omp parallel 
{
#	pragma omp for
for (int i = 0; i < nVerts; i++) {
	SetExtent (vertexP [i]);
	}
}
#endif
return Visible ();
}

//------------------------------------------------------------------------------

bool CGlowRenderer::SetViewport (int const nType, CFloatVector* vertexP, int nVerts)
{
if (!Available (nType))
	return true;
if ((GLOW_FLAGS & nType) == 0)
	return false;
#if USE_VIEWPORT
//if (gameStates.render.cameras.bActive)
//	return true;
if (gameOpts->render.effects.bGlow != 1)
	return true;
#pragma omp parallel 
{
#	pragma omp for
for (int i = 0; i < nVerts; i++) {
	SetExtent (*(vertexP [i].XYZ ()));
	}
}
#endif
return Visible ();
}

//------------------------------------------------------------------------------

bool CGlowRenderer::SetViewport (int const nType, CFloatVector3 v, float width, float height, bool bTransformed)
{
if (!Available (nType))
	return true;
if ((GLOW_FLAGS & nType) == 0)
	return false;
#if USE_VIEWPORT
//if (gameStates.render.cameras.bActive)
//	return true;
if (gameOpts->render.effects.bGlow != 1)
	return true;
if (!bTransformed)
	transformation.Transform (v, v);
CFloatVector3 r;
r.Set (width, height, 0.0f);
SetExtent (v - r, true);
SetExtent (v + r, true);
#endif
return Visible ();
}

//------------------------------------------------------------------------------

bool CGlowRenderer::SetViewport (int const nType, CFixVector pos, float radius)
{
if (!Available (nType))
	return true;
if ((GLOW_FLAGS & nType) == 0)
	return false;
#if USE_VIEWPORT
//if (gameStates.render.cameras.bActive)
//	return true;
if (gameOpts->render.effects.bGlow != 1)
	return true;
CFloatVector3 v;
v.Assign (pos);
return SetViewport (nType, v, radius, radius);
#endif
}

//------------------------------------------------------------------------------

bool CGlowRenderer::Available (int const nType, bool bForce)
{
if ((GLOW_FLAGS & nType) == 0)
	return false;
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

bool CGlowRenderer::Begin (int const nType, int const nStrength, bool const bReplace, float const brightness)
{
	static int nCalls = 0;

if (!Available (nType))
	return false;
if (++nCalls > 1)
	PrintLog ("nested glow renderer call!\n");
if ((m_bReplace != bReplace) || (m_nStrength != nStrength) || (m_brightness != brightness)) {
	End ();
	m_bReplace = bReplace;
	m_nStrength = nStrength;
	m_brightness = brightness;
	m_bViewport = 0;
	InitViewport ();
	Activate ();
	}
--nCalls;
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

void CGlowRenderer::Render (int const source, int const direction, float const radius, float const scale)
{
#if USE_VIEWPORT //DBG

float r = radius * 3.25f; // scale with a bit more than the max. offset from the blur shader
float w = (float) screen.Width ();
float h = (float) screen.Height ();
float verts [4][2] = {
	{ScreenCoord ((float) m_screenMin.x - r, (float) w),
	 ScreenCoord ((float) m_screenMin.y - r, (float) h)},
	{ScreenCoord ((float) m_screenMin.x - r, (float) w),
	 ScreenCoord ((float) m_screenMax.y + r, (float) h) /** scale*/},
	{ScreenCoord ((float) m_screenMax.x + r, (float) w) /** scale*/,
	 ScreenCoord ((float) m_screenMax.y + r, (float) h) /** scale*/},
	{ScreenCoord ((float) m_screenMax.x + r, (float) w) /** scale*/,
	 ScreenCoord ((float) m_screenMin.y - r, (float) h)}
	};
r += 3.25f;
float texCoord [4][2] = {
	{ScreenCoord ((float) m_screenMin.x - r, (float) w),
	 ScreenCoord ((float) m_screenMin.y - r, (float) h)},
	{ScreenCoord ((float) m_screenMin.x - r, (float) w),
	 ScreenCoord ((float) m_screenMax.y + r, (float) h)},
	{ScreenCoord ((float) m_screenMax.x + r, (float) w),
	 ScreenCoord ((float) m_screenMax.y + r, (float) h)},
	{ScreenCoord ((float) m_screenMax.x + r, (float) w),
	 ScreenCoord ((float) m_screenMin.y - r, (float) h)}
	};

#else

float verts [4][2] = {{0,0},{0,1},{1,1},{1,0}};
float texCoord [4][2] = {{0,0},{0,1},{1,1},{1,0}};

#endif

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

void CGlowRenderer::ClearViewport (float const radius)
{
#if 1
ogl.SaveViewport ();
float r = radius * 3.25f * m_nStrength; // scale with a bit more than the max. offset from the blur shader
glViewport ((GLsizei) max (m_screenMin.x - r, 0), 
				(GLsizei) max (m_screenMin.y - r, 0), 
				(GLint) min (m_screenMax.x - m_screenMin.x + 1 + 2 * r, ScreenWidth ()), 
				(GLint) min (m_screenMax.y - m_screenMin.y + 1 + 2 * r, ScreenHeight ()));
glClear (GL_COLOR_BUFFER_BIT);
ogl.RestoreViewport ();
#else
glClear (GL_COLOR_BUFFER_BIT);
#endif
}

//------------------------------------------------------------------------------

void CGlowRenderer::ChooseDrawBuffer (void)
{
//if (gameStates.render.cameras.bActive)
//	cameraManager.Camera (cameraManager.Current ())->EnableBuffer ();
ogl.ChooseDrawBuffer ();
//if (gameStates.render.cameras.bActive)
//	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//------------------------------------------------------------------------------

bool CGlowRenderer::End (void)
{
if (m_nStrength < 0)
	return false;

#if USE_VIEWPORT
if (!Visible ())
	ChooseDrawBuffer ();
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
	//glClearColor (0.0, 0.5, 1.0, 0.5);
	//glClear (GL_COLOR_BUFFER_BIT);

	float radius = 0.0f;
#if BLUR
	ogl.SetDepthMode (GL_ALWAYS);
	//ogl.SetFaceCulling (false);

	radius += RAD_INCR;
	ogl.SetBlendMode (GL_ONE, GL_ZERO);
	ogl.SelectBlurBuffer (0); 
	ClearViewport (radius);
	Render (-1, 0, radius); // Glow -> Blur 0
	ogl.SelectBlurBuffer (1); 
	ClearViewport (radius);
	Render (0, 1, radius); // Blur 0 -> Blur 1
	ogl.SetBlendMode (GL_ONE, GL_ONE);
#	if BLUR > 1
	for (int i = 1; i < m_nStrength; i++) {
		radius += RAD_INCR;
		ogl.SelectBlurBuffer (0); 
		Render (1, 0, radius); // Blur 1 -> Blur 0
		ogl.SelectBlurBuffer (1); 
		Render (0, 1, radius); // Blur 0 -> Blur 1
		}
	radius += RAD_INCR;
#	endif
#endif

	ChooseDrawBuffer ();
	ogl.SetDepthMode (GL_LEQUAL);
	ogl.SetBlendMode (2);
#if BLUR
	Render (1, 0, radius, (float) ScreenScale ()); // Glow -> back buffer
	if (!m_bReplace)
#endif
		Render (-1, 0, radius, (float) ScreenScale ()); // render the unblurred stuff on top of the blur

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
m_bViewport = 0;
return true;
}

//------------------------------------------------------------------------------

