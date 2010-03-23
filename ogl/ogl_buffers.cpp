/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "config.h"
#include "maths.h"
#include "crypt.h"
#include "strutil.h"
#include "gameseg.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "network.h"
#include "gr.h"
#include "gamefont.h"
#include "screens.h"
#include "interp.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texcache.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "findfile.h"
#include "rendermine.h"
#include "sphere.h"
#include "glare.h"
#include "menu.h"
#include "menubackground.h"
#include "cockpit.h"
#include "renderframe.h"
#include "automap.h"
#include "gpgpu_lighting.h"

//#define _WIN32_WINNT		0x0600

int enhance3DShaderProg [2][2][3] = {{{-1,-1,-1},{-1,-1,-1}},{{-1,-1,-1},{-1,-1,-1}}};
int duboisShaderProg = -1;

int nScreenDists [10] = {1, 2, 5, 10, 15, 20, 30, 50, 70, 100};
float nDeghostThresholds [4][2] = {{1.0f, 1.0f}, {0.8f, 0.8f}, {0.7f, 0.7f}, {0.6f, 0.6f}};

//------------------------------------------------------------------------------

#if DBG_OGL

void COGL::VertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine)
{
m_data.clientBuffers [m_data.nTMU [0]][0].buffer = pointer;
m_data.clientBuffers [m_data.nTMU [0]][0].pszFile = pszFile;
m_data.clientBuffers [m_data.nTMU [0]][0].nLine = nLine;
glVertexPointer (size, type, stride, pointer);
}

//------------------------------------------------------------------------------

void COGL::NormalPointer (GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine)
{
m_data.clientBuffers [m_data.nTMU [0]][1].buffer = pointer;
m_data.clientBuffers [m_data.nTMU [0]][1].pszFile = pszFile;
m_data.clientBuffers [m_data.nTMU [0]][1].nLine = nLine;
glNormalPointer (type, stride, pointer);
}

//------------------------------------------------------------------------------

void COGL::ColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine)
{
m_data.clientBuffers [m_data.nTMU [0]][2].buffer = pointer;
m_data.clientBuffers [m_data.nTMU [0]][2].pszFile = pszFile;
m_data.clientBuffers [m_data.nTMU [0]][2].nLine = nLine;
glColorPointer (size, type, stride, pointer);
}

//------------------------------------------------------------------------------

void COGL::TexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine)
{
m_data.clientBuffers [m_data.nTMU [0]][4].buffer = pointer;
m_data.clientBuffers [m_data.nTMU [0]][4].pszFile = pszFile;
m_data.clientBuffers [m_data.nTMU [0]][4].nLine = nLine;
glTexCoordPointer (size, type, stride, pointer);
}

#endif

//------------------------------------------------------------------------------

void COGL::SwapBuffers (int bForce, int bClear)
{
if (!gameStates.menus.nInMenu || bForce) {
#	if PROFILING
	if (gameStates.render.bShowProfiler && gameStates.app.bGameRunning && !gameStates.menus.nInMenu && fontManager.Current () && SMALL_FONT) {
		static time_t t0 = -1000;
		time_t t1 = clock ();
		static tProfilerData p;
		static float nFrameCount = 1;
		if (t1 - t0 >= 1000) {
			memcpy (&p, &gameData.profiler, sizeof (p));
			nFrameCount = float (gameData.app.nFrameCount);
			t0 = t1;
			}
		int h = SMALL_FONT->Height () + 3, i = 3;
		fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		float t, s = 0;
		GrPrintF (NULL, 5, h * i++, "frame: %1.2f", float (p.t [ptFrame]) / nFrameCount);
		if (p.t [ptRenderFrame]) {
			GrPrintF (NULL, 5, h * i++, "  scene: %1.2f %c (%1.2f)", 100.0f * float (p.t [ptRenderFrame]) / float (p.t [ptFrame]), '%', float (p.t [ptRenderFrame]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "    mine: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptRenderMine]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptRenderMine]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "    light: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptLighting]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptLighting]) / nFrameCount);
			s += t;
			GrPrintF (NULL, 5, h * i++, "    render: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptRenderPass]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptRenderPass]) / nFrameCount);
			s += t;
			GrPrintF (NULL, 5, h * i++, "      face list: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptFaceList]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptFaceList]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      faces: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptRenderFaces]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptRenderFaces]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      objects: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptRenderObjects]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptRenderObjects]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      transparency: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptTranspPolys]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptTranspPolys]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      effects: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptEffects]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptEffects]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "        particles: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptParticles]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptParticles]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      cockpit: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptCockpit]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptCockpit]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      states: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptRenderStates]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptRenderStates]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      shaders: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptShaderStates]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptShaderStates]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "    other: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptAux]) / float (p.t [ptRenderFrame]), '%');
			s += t;
			GrPrintF (NULL, 5, h * i++, "      transform: %1.2f %c (%1.2f) ", t = 100.0f * float (p.t [ptTransform]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptTransform]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      seg list: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptBuildSegList]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptBuildSegList]) / nFrameCount);
			GrPrintF (NULL, 5, h * i++, "      obj list: %1.2f %c (%1.2f)", t = 100.0f * float (p.t [ptBuildObjList]) / float (p.t [ptRenderFrame]), '%', float (p.t [ptBuildObjList]) / nFrameCount);
			}
		GrPrintF (NULL, 5, h * i++, "  total: %1.2f %c", s, '%');
		}
#endif
#if 0
	if (gameStates.app.bGameRunning && !gameStates.menus.nInMenu)
		paletteManager.RenderEffect ();
#endif
	if (gameStates.render.bRenderIndirect) {
		FlushDrawBuffer ();
		//SelectDrawBuffer (0);
		gameStates.render.bRenderIndirect = 0;
		Draw2DFrameElements ();
		gameStates.render.bRenderIndirect = 1;
		//SetDrawBuffer (GL_BACK, 1);
		}
	SDL_GL_SwapBuffers ();
	if (gameStates.app.bSaveScreenshot)
		SaveScreenShot (NULL, 0);
	SetDrawBuffer (GL_BACK, gameStates.render.bRenderIndirect);
#if 1
	//if (gameStates.menus.nInMenu || bClear)
		glClear (GL_COLOR_BUFFER_BIT);
#endif
	}
}

//------------------------------------------------------------------------------

void COGL::CreateDrawBuffer (void)
{
#if FBO_DRAW_BUFFER
if (gameStates.render.bRenderIndirect && m_states.bRender2TextureOk && !DrawBuffer ()->Handle ()) {
	PrintLog ("creating draw buffer\n");
	DrawBuffer ()->Create (m_states.nCurWidth, m_states.nCurHeight, 1);
	}
#endif
}

//------------------------------------------------------------------------------

void COGL::DestroyDrawBuffer (void)
{
#if FBO_DRAW_BUFFER
#	if 1
	static int bSemaphore = 0;

if (bSemaphore)
	return;
bSemaphore++;
#	endif
if (m_states.bRender2TextureOk && DrawBuffer () && DrawBuffer ()->Handle ()) {
	SetDrawBuffer (GL_BACK, 0);
	DrawBuffer ()->Destroy ();
	}
#	if 1
bSemaphore--;
#	endif
#endif
}

//------------------------------------------------------------------------------

void COGL::DestroyDrawBuffers (void)
{
if (m_data.drawBuffers [1].Handle ()) {
	SelectDrawBuffer (1);
	DestroyDrawBuffer ();
	}
SelectDrawBuffer (0);
DestroyDrawBuffer ();
}

//------------------------------------------------------------------------------

void COGL::SetDrawBuffer (int nBuffer, int bFBO)
{
#if 1
	static int bSemaphore = 0;

if (bSemaphore)
	return;
bSemaphore++;
#endif
#if FBO_DRAW_BUFFER
if (bFBO && (nBuffer == GL_BACK) && m_states.bRender2TextureOk && DrawBuffer ()->Handle ()) {
	if (!DrawBuffer ()->Active ()) {
		if (DrawBuffer ()->Enable ()) {
			glDrawBuffer (GL_COLOR_ATTACHMENT0_EXT);
			}
		else {
			DestroyDrawBuffers ();
			SelectDrawBuffer (0);
			glDrawBuffer (GL_BACK);
			}
		}
	}
else {
	if (DrawBuffer ()->Active ()) {
		DrawBuffer ()->Disable ();
		}
	glDrawBuffer (m_states.nDrawBuffer = nBuffer);
	}
#else
glDrawBuffer (m_states.nDrawBuffer = nBuffer);
#endif
#if 1
bSemaphore--;
#endif
}

//------------------------------------------------------------------------------

void COGL::SetReadBuffer (int nBuffer, int bFBO)
{
#if FBO_DRAW_BUFFER
if (bFBO && (nBuffer == GL_BACK) && m_states.bRender2TextureOk && DrawBuffer ()->Handle ()) {
	if (DrawBuffer ()->Active () || DrawBuffer ()->Enable ())
		glReadBuffer (GL_COLOR_ATTACHMENT0_EXT);
	else
		glReadBuffer (GL_BACK);
	}
else {
	if (DrawBuffer ()->Active ())
		DrawBuffer ()->Disable ();
	glReadBuffer (nBuffer);
	}
#else
glReadBuffer (nBuffer);
#endif
}

//------------------------------------------------------------------------------

int COGL::SelectDrawBuffer (int nBuffer) 
{ 
int nPrevBuffer = (m_data.drawBufferP && m_data.drawBufferP->Active ()) ? int (m_data.drawBufferP - m_data.drawBuffers) : -1;
if ((nBuffer < 0) || (nBuffer >= sizeof (m_data.drawBuffers)))
	return nPrevBuffer;
if (nBuffer != nPrevBuffer) {
	if (nPrevBuffer >= 0)
		m_data.drawBufferP->Disable (false);
	m_data.drawBufferP = m_data.GetDrawBuffer (nBuffer); 
	}
CreateDrawBuffer ();
m_data.drawBufferP->Enable (false);
return nPrevBuffer;
}

//------------------------------------------------------------------------------

void COGL::FlushDrawBuffer (bool bAdditive)
{
#if FBO_DRAW_BUFFER
if (HaveDrawBuffer ()) {
	static tTexCoord2f texCoord [4] = {{{0,0}},{{0,1}},{{1,1}},{{1,0}}};
	static float verts [4][2] = {{0,0},{0,1},{1,1},{1,0}};

	int bStereo = 0;

	SetDrawBuffer (GL_BACK, 0);
	ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
	ogl.BindTexture (DrawBuffer (0)->ColorBuffer ());
	OglTexCoordPointer (2, GL_FLOAT, 0, texCoord);
	OglVertexPointer (2, GL_FLOAT, 0, verts);

	if (m_data.xStereoSeparation > 0) {
		static float gain [4] = {1.0, 4.0, 2.0, 1.0};
		int h = gameOpts->render.bDeghost;
		int i = (gameOpts->render.bColorGain > 0);
		int j = Enhance3D () - 1;
		if ((bStereo = ((j >= 0) && (j <= 2))) && ((h < 4) || (j == 1))) {
			GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy ((h == 4) ? duboisShaderProg : enhance3DShaderProg [h > 0][i][j]));
			if (shaderProg) {
				shaderManager.Rebuild (shaderProg);
				ogl.EnableClientStates (1, 0, 0, GL_TEXTURE1);
				ogl.BindTexture (DrawBuffer (1)->ColorBuffer ());
				OglTexCoordPointer (2, GL_FLOAT, 0, texCoord);
				OglVertexPointer (2, GL_FLOAT, 0, verts);

				glUniform1i (glGetUniformLocation (shaderProg, "leftFrame"), gameOpts->render.bFlipFrames);
				glUniform1i (glGetUniformLocation (shaderProg, "rightFrame"), !gameOpts->render.bFlipFrames);
				if (h < 4) {
					if (h)
						glUniform2fv (glGetUniformLocation (shaderProg, "strength"), 1, reinterpret_cast<GLfloat*> (&nDeghostThresholds [gameOpts->render.bDeghost]));
					if (i)
						glUniform1f (glGetUniformLocation (shaderProg, "gain"), gain [gameOpts->render.bColorGain]);
					}
				ClearError (0);
				}
			}
		}

	glColor3f (1,1,1);
	OglDrawArrays (GL_QUADS, 0, 4);
	ResetClientStates (0);
	SelectDrawBuffer (0);
	if (bStereo)
		shaderManager.Deploy (-1);
	}
#endif
}

// -----------------------------------------------------------------------------------

#define GL_DEPTH_STENCIL_EXT 0x84F9
#define GL_UNSIGNED_INT_24_8_EXT 0x84FA
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1

GLuint COGL::CreateDepthTexture (int nTMU, int bFBO, int bStencil)
{
	GLuint	hBuffer;

if (nTMU >= GL_TEXTURE0)
	SelectTMU (nTMU);
SetTexturing (true);
GenTextures (1, &hBuffer);
if (glGetError ())
	return hBuffer = 0;
BindTexture (hBuffer);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
if (bStencil)
	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8_EXT, m_states.nCurWidth, m_states.nCurHeight, 0, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, NULL);
else
	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_states.nCurWidth, m_states.nCurHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
if (glGetError ()) {
	DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
if (!bFBO) {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
	}
if (glGetError ()) {
	DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
return hBuffer;
}

// -----------------------------------------------------------------------------------

void COGL::DestroyDepthTexture (void)
{
if (ogl.m_states.hDepthBuffer) {
	ogl.DeleteTextures (1, &ogl.m_states.hDepthBuffer);
	ogl.m_states.hDepthBuffer = 0;
	}
}

// -----------------------------------------------------------------------------------

GLuint COGL::CopyDepthTexture (void)
{
	GLenum nError = glGetError ();

#if DBG
if (nError)
	nError = nError;
#endif
SelectTMU (GL_TEXTURE1);
SetTexturing (true);
if (!m_states.hDepthBuffer)
	m_states.bHaveDepthBuffer = 0;
if (m_states.hDepthBuffer || (m_states.hDepthBuffer = CreateDepthTexture (-1, 0))) {
	BindTexture (m_states.hDepthBuffer);
	if (!m_states.bHaveDepthBuffer) {
		if (ogl.Enhance3D () < 0)
			ogl.SetReadBuffer ((ogl.StereoSeparation () < 0) ? GL_BACK_LEFT : GL_BACK_RIGHT, 0);
		else
			ogl.SetReadBuffer (GL_BACK, gameStates.render.bRenderIndirect);
		glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen.Width (), screen.Height ());
		if ((nError = glGetError ())) {
			DestroyDepthTexture ();
			return m_states.hDepthBuffer = 0;
			}
		m_states.bHaveDepthBuffer = 1;
		gameData.render.nStateChanges++;
		}
	}
return m_states.hDepthBuffer;
}

// -----------------------------------------------------------------------------------

GLuint COGL::CreateColorTexture (int nTMU, int bFBO)
{
	GLuint	hBuffer;

if (nTMU > GL_TEXTURE0)
	SelectTMU (nTMU);
SetTexturing (true);
GenTextures (1, &hBuffer);
if (glGetError ())
	return hBuffer = 0;
BindTexture (hBuffer);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, m_states.nCurWidth, m_states.nCurHeight, 0, GL_RGB, GL_UNSIGNED_SHORT, NULL);
if (glGetError ()) {
	DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
if (!bFBO) {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
	}
if (glGetError ()) {
	DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
return hBuffer;
}

// -----------------------------------------------------------------------------------

void COGL::DestroyColorTexture (void)
{
if (ogl.m_states.hColorBuffer) {
	ogl.DeleteTextures (1, &ogl.m_states.hColorBuffer);
	ogl.m_states.hColorBuffer = 0;
	}
}

// -----------------------------------------------------------------------------------

GLuint COGL::CopyColorTexture (void)
{
	GLenum nError = glGetError ();

#if DBG
if (nError)
	nError = nError;
#endif
SelectTMU (GL_TEXTURE1);
SetTexturing (true);
if (!m_states.hColorBuffer)
	m_states.bHaveColorBuffer = 0;
if (m_states.hColorBuffer || (m_states.hColorBuffer = CreateColorTexture (-1, 0))) {
	BindTexture (m_states.hColorBuffer);
	if (!m_states.bHaveColorBuffer) {
		glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen.Width (), screen.Height ());
		if ((nError = glGetError ())) {
			glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen.Width (), screen.Height ());
			if ((nError = glGetError ())) {
				DestroyColorTexture ();
				return m_states.hColorBuffer = 0;
				}
			}
		m_states.bHaveColorBuffer = 1;
		gameData.render.nStateChanges++;
		}
	}
return m_states.hColorBuffer;
}

// -----------------------------------------------------------------------------------

#if 0

GLuint COGL::CreateStencilTexture (int nTMU, int bFBO)
{
	GLuint	hBuffer;

if (nTMU > 0)
	SelectTMU (nTMU);
ogl.SetTexturing (true);
GenTextures (1, &hBuffer);
if (glGetError ())
	return hDepthBuffer = 0;
ogl.BindTexture (hBuffer);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, GL_STENCIL_COMPONENT8, m_states.nCurWidth, m_states.nCurHeight,
				  0, GL_STENCIL_COMPONENT, GL_UNSIGNED_BYTE, NULL);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
if (glGetError ()) {
	DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
return hBuffer;
}

#endif

//------------------------------------------------------------------------------

const char* enhance3DFS [2][2][3] = {
	{
		{
	#if 0
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"void main() {\r\n" \
		"gl_FragColor = vec4 (texture2D (leftFrame, gl_TexCoord [0].xy).xy, dot (texture2D (rightFrame, gl_TexCoord [0].xy).rgb, vec3 (0.15, 0.15, 0.7)), 1.0);\r\n" \
		"}",
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"gl_FragColor = vec4 (clamp (1.0, dot (texture2D (leftFrame, gl_TexCoord [0].xy).rgb, vec3 (1.0, 0.15, 0.15))), texture2D (rightFrame, gl_TexCoord [0].xy).yz, 0.0, 1.0);\r\n" \
		"}",
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"gl_FragColor = vec4 (cl.r, dot (texture2D (rightFrame, gl_TexCoord [0].xy).rgb, vec3 (0.15, 0.7, 0.15)), cl.b, 1.0);\r\n" \
		"}"
	#else
		// amber/blue
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"void main() {\r\n" \
		"vec3 c = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - c.b, 0.3) / max (0.000001, c.r + c.g);\r\n" \
		"gl_FragColor = vec4 (texture2D (leftFrame, gl_TexCoord [0].xy).xy, dot (c, vec3 (c.r * s, c.g * s, 1.0)), 1.0);\r\n" \
		"}",
		// red/cyan
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"void main() {\r\n" \
		"vec3 c = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - c.r, 0.3) / max (0.000001, c.g + c.b);\r\n" \
		"gl_FragColor = vec4 (dot (c, vec3 (1.0, c.g * s, c.b * s)), texture2D (rightFrame, gl_TexCoord [0].xy).yz, 1.0);\r\n" \
		"}",
		// green/magenta
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - cl.g, 0.3) / max (0.000001, cl.r + cl.b);\r\n" \
		"gl_FragColor = vec4 (cr.r, dot (cl, vec3 (cl.r * s, 1.0, cl.b * s)), cr.b, 1.0);\r\n" \
		"}"
#endif
		},
		{
		// amber/blue
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dr = (1.0 - cr.b) /  max (0.000001, cr.r + cr.g) / gain;\r\n" \
		"float dl = 1.0 + cl.b /  max (0.000001, cl.r + cl.g) / gain;\r\n" \
		"gl_FragColor = vec4 (cl.r * dl, cl.g * dl, dot (cr, vec3 (dr * cr.r, dr * cr.g, 1.0)), 1.0);\r\n" \
		"}",
		// red/cyan
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dl = (1.0 - cl.r) / max (0.000001, cl.g + cl.b) / gain;\r\n" \
		"float dr = 1.0 + cr.r /  max (0.000001, cr.g + cr.b) / gain;\r\n" \
		"gl_FragColor = vec4 (dot (cl, vec3 (1.0, dl * cl.g, dl * cl.b)), cr.g * dr, cr.b * dr, 1.0);\r\n" \
		"}",
		// green/magenta
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dl = (1.0 - cl.g) / max (0.000001, cl.r + cl.b) / gain;\r\n" \
		"float dr = 1.0 + cr.g /  max (0.000001, cr.r + cr.b) / gain;\r\n" \
		"gl_FragColor = vec4 (cr.r * dr, dot (cl, vec3 (dl * cl.r, 1.0, dl * cl.b)), cr.b * dr, 1.0);\r\n" \
		"}"
		},
	},
	// with de-ghosting
	{
		// no color adjustment
		{
		// amber/blue
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"void main() {\r\n" \
		"vec3 c = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - c.b, 0.3) / max (0.000001, c.r + c.g);\r\n" \
		"vec3 h = vec3 (texture2D (leftFrame, gl_TexCoord [0].xy).xy, dot (c, vec3 (c.r * s, c.g * s, 1.0)));\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"s = strength [0] / max (strength [0], (l.g + l.r) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.b / t);\r\n" \
		"gl_FragColor = vec4 (h.r * s, h.g * s, h.b * t, 1.0);\r\n" \
		"}",
		// red/cyan
		// de-ghosting by reducing red or cyan brightness if they exceed a certain threshold compared to the pixels total brightness
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"void main() {\r\n" \
		"vec3 c = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - c.r, 0.3) / max (0.000001, c.g + c.b);\r\n" \
		"vec3 h = vec3 (dot (c, vec3 (1.0, c.g * s, c.b * s)), texture2D (rightFrame, gl_TexCoord [0].xy).yz);\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"s = strength [0] / max (strength [0], (l.g + l.b) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.r / t);\r\n" \
		"gl_FragColor = vec4 (h.r * t, h.g * s, h.b * s, 1.0);\r\n" \
		"}",
		// green/magenta
		// de-ghosting by reducing red or cyan brightness if they exceed a certain threshold compared to the pixels total brightness
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float s = min (1.0 - cl.g, 0.3) / max (0.000001, cl.r + cl.b);\r\n" \
		"vec3 h = vec3 (cr.r, dot (cl, vec3 (cl.r * s, 1.0, cl.b * s)), cr.b);\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"s = strength [0] / max (strength [0], (l.r + l.b) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.g / t);\r\n" \
		"gl_FragColor = vec4 (h.r * s, h.g * t, h.b * s, 1.0);\r\n" \
		"}"
		},
		// color adjustment
		{
		// amber/blue
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dr = (1.0 - cr.b) / max (0.000001, cr.r + cr.g) / gain;\r\n" \
		"float dl = 1.0 + cl.b /  max (0.000001, cl.r + cl.g) / gain;\r\n" \
		"vec3 h = vec3 (cl.r * dl, cl.g * dl, dot (cr, vec3 (dr * cr.r, dr * cr.g, 1.0)));\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"float s = strength [0] / max (strength [0], (l.g + l.r) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.b / t);\r\n" \
		"gl_FragColor = vec4 (h.r * s, h.g * s, h.b * t, 1.0);\r\n" \
		"}",
		// red/cyan
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dl = (1.0 - cl.r) / max (0.000001, cl.g + cl.b) / gain;\r\n" \
		"float dr = 1.0 + cr.r /  max (0.000001, cr.g + cr.b) / gain;\r\n" \
		"vec3 h = vec3 (dot (cl, vec3 (1.0, dl * cl.g, dl * cl.b)), cr.g * dr, cr.b * dr);\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"float s = strength [0] / max (strength [0], (l.g + l.b) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.r / t);\r\n" \
		"gl_FragColor = vec4 (h.r * t, h.g * s, h.b * s, 1.0);\r\n" \
		"}",
		// red/cyan
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"uniform vec2 strength;\r\n" \
		"uniform float gain;\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"float dl = (1.0 - cl.g) / max (0.000001, cl.r + cl.b) / gain;\r\n" \
		"float dr = 1.0 + cr.g /  max (0.000001, cr.r + cr.b) / gain;\r\n" \
		"vec3 h = vec3 (cr.r * dr, dot (cl, vec3 (dl * cl.r, 1.0, dl * cl.b)), cr.b * dr);\r\n" \
		"vec3 l = h * vec3 (0.3, 0.59, 0.11);\r\n" \
		"float t = (l.r + l.g + l.b);\r\n" \
		"float s = strength [0] / max (strength [0], (l.r + l.b) / t);\r\n" \
		"t = strength [1] / max (strength [1], l.g / t);\r\n" \
		"gl_FragColor = vec4 (h.r * s, h.g * t, h.b * s, 1.0);\r\n" \
		"}"
		}
	}
};

const char* duboisFS = 
#if 0
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"vec3 rl = vec3 ( 0.4561000,  0.5004840,  0.17638100);\r\n" \
		"vec3 gl = vec3 (-0.0400822, -0.0378246, -0.01575890);\r\n" \
		"vec3 bl = vec3 (-0.0152161, -0.0205971, -0.00546856);\r\n" \
		"vec3 rr = vec3 (-0.0434706, -0.0879388, -0.00155529);\r\n" \
		"vec3 gr = vec3 ( 0.3784760,  0.7336400, -0.01845030);\r\n" \
		"vec3 br = vec3 (-0.0721527, -0.1129610,  1.22640000);\r\n" \
		"void main() {\r\n" \
		"vec3 cl = texture2D (leftFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"vec3 cr = texture2D (rightFrame, gl_TexCoord [0].xy).rgb;\r\n" \
		"gl_FragColor = vec4 (clamp (dot (cl, rl) + dot (cr, rr), 0.0, 1.0),\r\n" \
		"                     clamp (dot (cl, gl) + dot (cr, gr), 0.0, 1.0),\r\n" \
		"                     clamp (dot (cl, bl) + dot (cr, br), 0.0, 1.0), 1.0);\r\n" \
		"}";
#else
		"uniform sampler2D leftFrame, rightFrame;\r\n" \
		"mat3 lScale = mat3 ( 0.4561000,  0.5004840,  0.17638100,\r\n" \
		"                    -0.0400822, -0.0378246, -0.01575890,\r\n" \
		"                    -0.0152161, -0.0205971, -0.00546856);\r\n" \
		"mat3 rScale = mat3 (-0.0434706, -0.0879388, -0.00155529,\r\n" \
		"                     0.3784760,  0.7336400, -0.01845030,\r\n" \
		"                    -0.0721527, -0.1129610,  1.22640000);\r\n" \
		"void main() {\r\n" \
		"gl_FragColor = vec4 (clamp (texture2D (leftFrame, gl_TexCoord [0].xy).rgb * lScale + texture2D (rightFrame, gl_TexCoord [0].xy).rgb * rScale,\r\n" \
		"                            vec3 (0.0, 0.0, 0.0), vec3 (1.0, 1.0, 1.0)), 1.0);\r\n" \
		"}";
#endif

const char* enhance3DVS = 
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_Position=ftransform();"\
	"gl_FrontColor=gl_Color;}"
	;

//-------------------------------------------------------------------------

void COGL::InitEnhanced3DShader (void)
{
if (gameOpts->render.bUseShaders && m_states.bShadersOk) {
	PrintLog ("building dubois optimization programs\n");
	gameStates.render.textures.bHaveEnhanced3DShader = (0 <= shaderManager.Build (duboisShaderProg, duboisFS, enhance3DVS));
	if (!gameStates.render.textures.bHaveEnhanced3DShader) {
		DeleteEnhanced3DShader ();
		gameOpts->render.n3DGlasses = GLASSES_NONE;
		return;
		}
	PrintLog ("building enhanced 3D shader programs\n");
	for (int h = 0; h < 2; h++) {
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 3; j++) {
				gameStates.render.textures.bHaveEnhanced3DShader = (0 <= shaderManager.Build (enhance3DShaderProg [h][i][j], enhance3DFS [h][i][j], enhance3DVS));
				if (!gameStates.render.textures.bHaveEnhanced3DShader) {
					DeleteEnhanced3DShader ();
					gameOpts->render.n3DGlasses = GLASSES_NONE;
					return;
					}
				}
			}
		}
	}
}

//-------------------------------------------------------------------------

void COGL::DeleteEnhanced3DShader (void)
{
if (duboisShaderProg >= 0) {
	shaderManager.Delete (duboisShaderProg);
	for (int h = 0; h < 2; h++) {
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 3; j++) {
				shaderManager.Delete (enhance3DShaderProg [h][i][j]);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
