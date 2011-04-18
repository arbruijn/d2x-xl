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

int shadowShaderProg = -1;

extern tTexCoord2f quadTexCoord [4];
extern float quadVerts [4][2];

#define USE_SHADOW2DPROJ 1

//------------------------------------------------------------------------------

#if MAX_SHADOWMAPS

void COGL::FlushShadowMaps (int nEffects)
{
	static const char* szShadowMap [] = {"shadowMap", "shadow1", "shadow2", "shadow3"};

	float screenSize [2] = { float (screen.Width ()), float (screen.Height ()) };
	float screenScale [2] = { 1.0f / float (screen.Width ()), 1.0f / float (screen.Height ()) };
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
		int i;
		for (i = 0; i < lightManager.LightCount (2); i++) {
			SelectTMU (GL_TEXTURE2 + i);
			SetTexturing (true);
			glMatrixMode (GL_TEXTURE);
			lightManager.ShadowTransformation (i).Set ();  // light's projection * modelview
#if 1
			lightManager.ShadowTransformation (-3).Mul (); // inverse of camera's modelview * inverse of camera's projection
#else
			lightManager.ShadowTransformation (-1).Mul (); // inverse of camera's modelview
			lightManager.ShadowTransformation (-2).Mul (); // inverse of camera's projection
#endif
			glMatrixMode (matrixMode);
			BindTexture (cameraManager.ShadowMap (i)->FrameBuffer ().DepthBuffer ());
			glUniform1i (glGetUniformLocation (shaderProg, szShadowMap [i]), i + 2);
			CDynLight* prl = cameraManager.ShadowLightSource (i);
			glUniform3fv (glGetUniformLocation (shaderProg, "lightPos"), 1, (GLfloat*) prl->render.vPosf [0].v.vec);
			glUniform1f (glGetUniformLocation (shaderProg, "lightRange"), (GLfloat) fabs (prl->info.fRange) * 500.0f);
			}
		glUniform1i (glGetUniformLocation (shaderProg, "sceneColor"), 0);
		glUniform1i (glGetUniformLocation (shaderProg, "sceneDepth"), 1);
		glUniformMatrix4fv (glGetUniformLocation (shaderProg, "modelviewProjInverse"), 1, (GLboolean) 0, lightManager.ShadowTransformation (-3).ToFloat ());
		SetBlendMode (0);
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
#if 0
	"varying vec3 position;\r\n" \
	"varying vec4 vertex;\r\n" \
	"varying vec3 normal;\r\n" \
	"void main() {\r\n" \
	"position = gl_Vertex.xyz;\r\n" \
	"vertex = gl_ModelViewMatrix * gl_Vertex;\r\n" \
	"normal = gl_NormalMatrix * gl_Normal;\r\n" \
	"gl_TexCoord[0] = gl_MultiTexCoord0;\r\n" \
	"//gl_TexCoord[1] = gl_TextureMatrix[2] * gl_Vertex;\r\n" \
	"//gl_TexCoord[2] = gl_TextureMatrix[3] * vertex;\r\n" \
	"//gl_TexCoord[3] = gl_TextureMatrix[4] * vertex;\r\n" \
	"//gl_TexCoord[4] = gl_TextureMatrix[5] * vertex;\r\n" \
	"gl_Position = ftransform();\r\n" \
	"gl_FrontColor = gl_Color;\r\n" \
	"}\r\n";
#else
	"void main() {\r\n" \
	"gl_TexCoord[0] = gl_MultiTexCoord0;\r\n" \
	"gl_Position = ftransform();\r\n" \
	"gl_FrontColor = gl_Color;\r\n" \
	"}\r\n";
#endif

const char* shadowMapFS = 
#if 0

	"uniform sampler2D sceneColor;\r\n" \
	"uniform sampler2D sceneDepth;\r\n" \
	"uniform sampler2D/*Shadow*/ shadow0;\r\n" \
	"//uniform sampler2DShadow shadow1;\r\n" \
	"//uniform sampler2DShadow shadow2;\r\n" \
	"//uniform sampler2DShadow shadow3;\r\n" \
	"uniform int nLights;\r\n" \
	"//uniform sampler2D lightmask;\r\n" \
	"varying vec3 position;\r\n" \
	"varying vec4 vertex;\r\n" \
	"varying vec3 normal;\r\n" \
	"varying vec4 shadowCoord;\r\n" \
	"float Lookup (sampler2DShadow shadowMap, vec2 offset, int i) {\r\n" \
	"return shadow2DProj (shadowMap, shadowCoord + vec4 (offset.x * shadowCoord.w, offset.y * shadowCoord.w, 0.05, 0.0));\r\n" \
	"}\r\n" \
	"float Shadow (sampler2DShadow shadowMap, int i) {\r\n" \
	"if (i >= nLights)\r\n" \
	"   return 0.0;\r\n" \
	"if (vertex.w <= 1.0)\r\n" \
	"   return 0.0;\r\n" \
	"return Lookup (shadowMap, vec2 (0.0, 0.0), i);\r\n" \
	"//float shadow = Lookup (shadowMap, vec2 (-1.5, -1.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (-1.5, -0.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (-1.5, 0.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (-1.5, 1.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (-0.5, -1.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (-0.5, -0.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (-0.5, 0.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (-0.5, 1.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (0.5, -1.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (0.5, -0.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (0.5, 0.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (0.5, 1.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (1.5, -1.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (1.5, -0.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (1.5, 0.5, i));\r\n" \
	"//shadow += Lookup (shadowMap, vec2 (1.5, 1.5, i));\r\n" \
	"//return shadow / 16.0;\r\n" \
	"}\r\n" \
	"void main() {\r\n" \
	"//float shadow = shadow2DProj (shadow0, gl_TexCoord [1]).r;\r\n" \
	"float z = texture2D (sceneDepth, gl_TexCoord [0]).r;\r\n" \
	"float shadowDepth = texture2DProj (shadow0, gl_TexCoord [1]).r;\r\n" \
	"float light = 0.5 + (z > shadowDepth + 0.0005) ? 0.5 : 0.0;\r\n" \
	"gl_FragColor = vec4 (texture2D (sceneColor, gl_TexCoord [0]).rgb * light, 1.0);\r\n" \
	"//gl_FragColor = vec4 (texture2D (shadow0, gl_TexCoord [0]).rgb, 1.0);\r\n" \
	"//shadow += Shadow (shadow1, 1);\r\n" \
	"//shadow += Shadow (shadow2, 2);\r\n" \
	"//shadow += Shadow (shadow3, 3);\r\n" \
	"//shadow = min (1.0, 0.2 + shadow * 0.8 / (float) nLights);\r\n" \
	"//gl_FragColor = vec4 (shadow, shadow, shadow, 1.0);\r\n" \
	"//gl_FragColor = vec4 (texture2D (sceneColor, gl_TexCoord [0]).rgb * shadow, 1.0);\r\n" \
	"//float s = texture2D (shadow0, gl_TexCoord [0].xy).r;\r\n" \
	"//gl_FragColor = vec4 (s, s, s, 1.0);\r\n" \
	"//gl_FragColor = vec4 (vec3 (texture2D (shadow0, gl_TexCoord [0]).r), 1.0);\r\n" \
	"}\r\n";

#else

	"uniform sampler2D sceneColor;\r\n" \
	"uniform sampler2D sceneDepth;\r\n" \
	"uniform sampler2D shadowMap;\r\n" \
	"uniform mat4 modelviewProjInverse;\r\n" \
	"uniform vec3 lightPos;\r\n" \
	"uniform float lightRange;\r\n" \
	"#define ZNEAR 1.0\r\n" \
	"#define ZFAR 5000.0\r\n" \
	"#define A 5001.0 //(ZNEAR + ZFAR)\r\n" \
	"#define B 4999.0 //(ZNEAR - ZFAR)\r\n" \
	"#define C 10000.0 //(2.0 * ZNEAR * ZFAR)\r\n" \
	"#define D (cameraNDC.z * B)\r\n" \
	"//#define ZEYE -10000.0 / (5001.0 + cameraNDC.z * 4999.0) //-(C / (A + D))\r\n" \
	"#define ZEYE(z) -(ZFAR / ((z) * (ZFAR - ZNEAR) - ZFAR))\r\n" \
	"void main() {\r\n" \
	"float fragDepth = texture2D (sceneDepth, gl_TexCoord [0].xy).r;\r\n" \
	"vec3 cameraNDC = (vec3 (gl_TexCoord [0].xy, fragDepth) - 0.5) * 2.0;\r\n" \
	"vec4 cameraClipPos;\r\n" \
	"cameraClipPos.w = -ZEYE(cameraNDC.z);\r\n" \
	"cameraClipPos.xyz = cameraNDC * cameraClipPos.w;\r\n" \
	"vec4 lightWinPos = gl_TextureMatrix [2] * cameraClipPos;\r\n" \
	"float shadowDepth = texture2DProj (shadowMap, lightWinPos).r;\r\n" \
	"//vec4 lightClipPos = gl_TextureMatrix [2] * cameraClipPos;\r\n" \
	"//float w = abs (lightClipPos.w);\r\n" \
	"//float shadowDepth = ((w < 0.00001) || (abs (lightClipPos.x) > w) || (abs (lightClipPos.y) > w)) ? 2.0 : texture2D (shadowMap, lightClipPos.xy / (lightClipPos.w * 2.0) + 0.5).r;\r\n" \
	"float light;\r\n" \
	"//if (lightClipPos.z < (lightClipPos.w * 2.0) * (shadowDepth - 0.5))\r\n" \
	"if (fragDepth < shadowDepth)\r\n" \
	"   light = 1.0;\r\n" \
	"else {\r\n" \
	"   vec4 worldPos = modelviewProjInverse * cameraClipPos;\r\n" \
	"   float lightDist = length (lightPos - worldPos.xyz);\r\n" \
	"   light = sqrt (min (lightDist, lightRange) / lightRange);\r\n" \
	"   }\r\n" \
	"gl_FragColor = vec4 (texture2D (sceneColor, gl_TexCoord [0].xy).rgb * light, 1.0);\r\n" \
	"}\r\n";

#endif

//-------------------------------------------------------------------------

void COGL::InitShadowMapShader (void)
{
if (gameOpts->render.bUseShaders && m_states.bShadersOk) {
	PrintLog ("building shadow mapping program\n");
	gameStates.render.textures.bHaveShadowMapShader = (0 <= shaderManager.Build (shadowShaderProg, shadowMapFS, shadowMapVS));
	if (!gameStates.render.textures.bHaveShadowMapShader) 
		DeleteShadowMapShader ();
	}
}

//-------------------------------------------------------------------------

void COGL::DeleteShadowMapShader (void)
{
if (shadowShaderProg >= 0)
	shaderManager.Delete (shadowShaderProg);
}

//------------------------------------------------------------------------------

