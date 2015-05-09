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
#include "segmath.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "network.h"
#include "gr.h"
#include "glow.h"
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
#include "postprocessing.h"

int32_t shadowShaderProg = -1;

extern tTexCoord2f quadTexCoord [4];
extern float quadVerts [4][2];

#define USE_SHADOW2DPROJ 1

//------------------------------------------------------------------------------

#if MAX_SHADOWMAPS

void COGL::FlushShadowMaps (int32_t nEffects)
{
	static const char* szShadowMap [] = {"shadowMap", "shadow1", "shadow2", "shadow3"};

	float screenSize [2] = { float (gameData.render.screen.Width ()), float (gameData.render.screen.Height ()) };
	float windowScale [2] = { 1.0f / float (gameData.render.screen.Width ()), 1.0f / float (gameData.render.screen.Height ()) };
	float zScale = 5000.0f / 4999.0f;

if (gameStates.render.textures.bHaveShadowMapShader && (EGI_FLAG (bShadows, 0, 1, 0) != 0)) {
	SetDrawBuffer (GL_BACK, 0);
	EnableClientStates (1, 0, 0, GL_TEXTURE0);
#if MAX_SHADOWMAPS < 0
	BindTexture (cameraManager.ShadowMap (0)->FrameBuffer ().ColorBuffer ());
	OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord);
	OglVertexPointer (2, GL_FLOAT, 0, quadVerts);
	OglDrawArrays (GL_QUADS, 0, 4);
#else
	BindTexture (DrawBuffer ((nEffects & 1) ? 1 : (nEffects & 2) ? 2 : 0)->ColorBuffer ());
	EnableClientStates (1, 0, 0, GL_TEXTURE1);
	BindTexture (DrawBuffer ((nEffects & 1) ? 1 : (nEffects & 2) ? 2 : 0)->DepthBuffer ());
	OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord);
	OglVertexPointer (2, GL_FLOAT, 0, quadVerts);
	GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy (shadowShaderProg));
	if (shaderProg) {
		shaderManager.Rebuild (shaderProg);
		GLint matrixMode;
		glGetIntegerv (GL_MATRIX_MODE, &matrixMode);
		int32_t i;
		for (i = 0; i < lightManager.LightCount (2); i++) {
			SelectTMU (GL_TEXTURE2 + i);
			SetTexturing (true);
			glMatrixMode (GL_TEXTURE);
			transformation.SystemMatrix (i).Set ();  // light's projection * modelview
#if 1
			transformation.SystemMatrix (-3).Mul (); // inverse of camera's modelview * inverse of camera's projection
#else
			transformation.SystemMatrix (-1).Mul (); // inverse of camera's modelview
			transformation.SystemMatrix (-2).Mul (); // inverse of camera's projection
#endif
			glMatrixMode (matrixMode);
			BindTexture (cameraManager.ShadowMap (i)->FrameBuffer ().DepthBuffer ());
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //NEAREST);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //NEAREST);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
			glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
			shaderManager.Set (szShadowMap [i], i + 2);
			CDynLight* pLight = cameraManager.ShadowLightSource (i);
			if (!shaderManager.Set ("pLightos", pLight->render.vPosf [0]))
				return;
			if (!shaderManager.Set ("lightRange", fabs (pLight->info.fRange) * 400.0f))
				return;
			}
		if (!shaderManager.Set ("sceneColor", 0))
			return;
		if (!shaderManager.Set ("sceneDepth", 1))
			return;
		if (!shaderManager.Set ("modelpViewrojInverse", transformation.SystemMatrix (-3)))
			return;
		if (!shaderManager.Set ("windowScale", windowScale))
			return;
		SetBlendMode (OGL_BLEND_ALPHA);
		SetDepthTest (false);
		OglDrawArrays (GL_QUADS, 0, 4);
		for (i = 0; i < lightManager.LightCount (2); i++) {
			SelectTMU (GL_TEXTURE1 + i);
			SetTexturing (false);
			}
		SetDepthTest (true);
		}
#endif
	}
}

#endif

//------------------------------------------------------------------------------

const char* shadowMapVS = 
	"void main() {\r\n" \
	"gl_TexCoord[0] = gl_MultiTexCoord0;\r\n" \
	"gl_Position = ftransform();\r\n" \
	"gl_FrontColor = gl_Color;\r\n" \
	"}\r\n";

const char* shadowMapFS = 
#if 0

	"uniform sampler2D sceneColor;\r\n" \
	"uniform sampler2D sceneDepth;\r\n" \
	"uniform sampler2DShadow shadowMap;\r\n" \
	"uniform mat4 modelpViewrojInverse;\r\n" \
	"uniform vec3 pLightos;\r\n" \
	"uniform float lightRange;\r\n" \
	"#define ZNEAR 1.0\r\n" \
	"#define ZFAR 5000.0\r\n" \
	"#define A (ZNEAR + ZFAR)\r\n" \
	"#define B (ZNEAR - ZFAR)\r\n" \
	"#define C (2.0 * ZNEAR * ZFAR)\r\n" \
	"#define D (cameraNDC.z * B)\r\n" \
	"#define ZEYE -(C / (A + D))\r\n" \
	"void main() {\r\n" \
	"float fragDepth = texture2D (sceneDepth, gl_TexCoord [0].xy).r;\r\n" \
	"vec3 cameraNDC = (vec3 (gl_TexCoord [0].xy, fragDepth) - 0.5) * 2.0;\r\n" \
	"vec4 cameraClipPos;\r\n" \
	"cameraClipPos.w = -ZEYE;\r\n" \
	"cameraClipPos.xyz = cameraNDC * cameraClipPos.w;\r\n" \
	"vec4 lightWinPos = gl_TextureMatrix [2] * cameraClipPos;\r\n" \
	"float w = abs (lightWinPos.w);\r\n" \
	"float lit = ((w < 0.00001) || (abs (lightWinPos.x) > w) || (abs (lightWinPos.y) > w)) ? 1.0 : shadow2DProj (shadowMap, lightWinPos));\r\n" \
	"float light;\r\n" \
	"if (lit == 1)\r\n" \
	"   light = 1.0;\r\n" \
	"else {\r\n" \
	"   vec4 worldPos = modelpViewrojInverse * cameraClipPos;\r\n" \
	"   float lightDist = length (pLightos - worldPos.xyz);\r\n" \
	"   light = 0.25 + 0.75 * sqrt ((lightRange - min (lightRange, lightDist)) / lightRange);\r\n" \
	"   }\r\n" \
	"gl_FragColor = vec4 (texture2D (sceneColor, gl_TexCoord [0].xy).rgb * light, 1.0);\r\n" \
	"}\r\n";

#else

	"uniform sampler2D sceneColor;\r\n" \
	"uniform sampler2D sceneDepth;\r\n" \
	"uniform sampler2DShadow shadowMap;\r\n" \
	"uniform mat4 modelpViewrojInverse;\r\n" \
	"uniform vec2 windowScale;\r\n" \
	"uniform vec3 pLightos;\r\n" \
	"uniform float lightRange;\r\n" \
	"#define ZNEAR 1.0\r\n" \
	"#define ZFAR 5000.0\r\n" \
	"#define A (ZNEAR + ZFAR)\r\n" \
	"#define B (ZNEAR - ZFAR)\r\n" \
	"#define C (2.0 * ZNEAR * ZFAR)\r\n" \
	"#define D (cameraNDC.z * B)\r\n" \
	"#define ZEYE (C / (A + D))\r\n" \
	"#define MAX_OFFSET 2.5\r\n" \
	"#define SAMPLE_RANGE (MAX_OFFSET + MAX_OFFSET + 1.0)\r\n" \
	"#define SAMPLE_COUNT (SAMPLE_RANGE * SAMPLE_RANGE)\r\n" \
	"void main() {\r\n" \
	"float fragDepth = texture2D (sceneDepth, gl_TexCoord [0].xy).r;\r\n" \
	"vec3 cameraNDC = (vec3 (gl_TexCoord [0].xy, fragDepth) - 0.5) * 2.0;\r\n" \
	"vec4 cameraClipPos;\r\n" \
	"cameraClipPos.w = ZEYE;\r\n" \
	"cameraClipPos.xyz = cameraNDC * cameraClipPos.w;\r\n" \
	"vec4 lightWinPos = gl_TextureMatrix [2] * cameraClipPos;\r\n" \
	"float w = abs (lightWinPos.w);\r\n" \
	"vec4 samplePos = lightWinPos;\r\n" \
	"float light = 0.0;\r\n" \
	"vec2 offset;\r\n" \
	"for (offset.y = -MAX_OFFSET; offset.y <= MAX_OFFSET; offset.y += 1.0) {\r\n" \
	"  for (offset.x = -MAX_OFFSET; offset.x <= MAX_OFFSET; offset.x += 1.0) {\r\n" \
	"    samplePos.xy = lightWinPos.xy + offset * windowScale;\r\n" \
	"    light += ((w < 0.00001) || (abs (samplePos.x) > w) || (abs (samplePos.y) > w)) ? 1.0 : shadow2DProj (shadowMap, samplePos).r;\r\n" \
	"    }\r\n" \
	"  }\r\n" \
	"vec4 worldPos = modelpViewrojInverse * cameraClipPos;\r\n" \
	"float lightDist = length (pLightos - worldPos.xyz);\r\n" \
	"light = 1.0 - 0.75 * sqrt ((lightRange - min (lightRange, lightDist)) / lightRange) * (1.0 - light / SAMPLE_COUNT);\r\n" \
	"gl_FragColor = vec4 (texture2D (sceneColor, gl_TexCoord [0].xy).rgb * light, 1.0);\r\n" \
	"}\r\n";

#endif

//-------------------------------------------------------------------------

void COGL::InitShadowMapShader (void)
{
if (gameOpts->render.bUseShaders && m_features.bShaders.Available ()) {
	PrintLog (0, "building shadow mapping program\n");
	gameStates.render.textures.bHaveShadowMapShader = (0 <= shaderManager.Build (shadowShaderProg, shadowMapFS, shadowMapVS));
	if (!gameStates.render.textures.bHaveShadowMapShader) 
		DeleteShadowMapShader ();
	}
}

//-------------------------------------------------------------------------

void COGL::DeleteShadowMapShader (void)
{
if (shadowShaderProg >= 0) {
	shaderManager.Delete (shadowShaderProg);
	shadowShaderProg = -1;
	gameStates.render.textures.bHaveShadowMapShader = 0;
	}
}

//------------------------------------------------------------------------------

