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

#define USE_SHADOW2DPROJ 0

//------------------------------------------------------------------------------

extern int shadowMaps [4];

void COGL::FlushShadowMaps (int nEffects)
{
	static const char* szShadowMap [] = {"shadowMap", "shadow1", "shadow2", "shadow3"};

	float screenSize [2] = {float (screen.Width ()), float (screen.Height ())};

if (gameStates.render.textures.bHaveShadowMapShader && (EGI_FLAG (bShadows, 0, 1, 0) != 0)) {
#if 1
	SetDrawBuffer (GL_BACK, 0);
	ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
	ogl.BindTexture (DrawBuffer ((nEffects & 1) ? 1 : (nEffects & 2) ? 2 : 0)->ColorBuffer ());
	//CopyDepthTexture (0, GL_TEXTURE0);
	ogl.EnableClientStates (1, 0, 0, GL_TEXTURE1);
	ogl.BindTexture (DrawBuffer ((nEffects & 1) ? 1 : (nEffects & 2) ? 2 : 0)->DepthBuffer ());
	//ogl.BindTexture (0);
	OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord);
	OglVertexPointer (2, GL_FLOAT, 0, quadVerts);
#if USE_SHADOW2DPROJ
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
	glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY); 				
#endif
#if 1
	GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy (shadowShaderProg));
	if (shaderProg) {
		shaderManager.Rebuild (shaderProg);
		GLint matrixMode;
		glGetIntegerv (GL_MATRIX_MODE, &matrixMode);
		int i;
		for (i = 0; i < lightManager.LightCount (2); i++) {
			glMatrixMode (GL_TEXTURE);
			ogl.SelectTMU (GL_TEXTURE2 + i);
			ogl.SetTexturing (true);
			glLoadMatrixf (lightManager.ShadowTextureMatrix (i).m.vec);
			//glMultMatrixf (lightManager.ShadowTextureMatrix (-1).m.vec);
#if !USE_SHADOW2DPROJ
			glMultMatrixf (lightManager.ShadowTextureMatrix (-2).m.vec); // inverse of camera's modelview * projection
#endif
			glMatrixMode (matrixMode);
#if 1
			//ogl.BindTexture (cameraManager [shadowMaps [i]]->DrawBuffer ());
			ogl.BindTexture (cameraManager [shadowMaps [i]]->FrameBuffer ().DepthBuffer ());
#else
			ogl.BindTexture (cameraManager [shadowMaps [i]]->FrameBuffer ().ColorBuffer ());
#endif
			glUniform1i (glGetUniformLocation (shaderProg, szShadowMap [i]), i + 2);
			}
		glUniform1i (glGetUniformLocation (shaderProg, "sceneColor"), 0);
		glUniform1i (glGetUniformLocation (shaderProg, "sceneDepth"), 1);
#if USE_SHADOW2DPROJ
		glUniformMatrix4fv (glGetUniformLocation (shaderProg, "modelviewProjInverse"), 1, (GLboolean) 0, (GLfloat*) lightManager.ShadowTextureMatrix (-3).m.vec);
#else
		glUniform2fv (glGetUniformLocation (shaderProg, "screenSize"), 1, (GLfloat*) screenSize);
#endif
		//glUniform1i (glGetUniformLocation (shaderProg, "nLights"), i);
		//glUniform1f (glGetUniformLocation (shaderProg, "aspect"), -GLfloat (screen.Height ()) / GLfloat (screen.Width ()));
		//glUniformMatrix4fv (glGetUniformLocation (shaderProg, "invModelViewMat"), 1, (GLboolean) 0, (GLfloat*) lightManager.ShadowTextureMatrix (-1).m.vec);
		//glUniformMatrix4fv (glGetUniformLocation (shaderProg, "projMat"), 1, (GLboolean) 0, (GLfloat*) lightManager.ShadowTextureMatrix (-2).m.vec);
		//glUniformMatrix4fv (glGetUniformLocation (shaderProg, "invProjMat"), 1, (GLboolean) 0, (GLfloat*) lightManager.ShadowTextureMatrix (-2).m.vec);
		ogl.SetBlendMode (0);
		OglDrawArrays (GL_QUADS, 0, 4);
		for (i = 0; i < lightManager.LightCount (2); i++) {
			ogl.SelectTMU (GL_TEXTURE1 + i);
			ogl.SetTexturing (false);
			}
		}
#endif
	//ogl.SetBlendMode (0);
	//glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	//SetDrawBuffer (GL_BACK, 0);
	//ogl.BindTexture (DrawBuffer ((nEffects & 1) ? 1 : (nEffects & 2) ? 2 : 0)->ColorBuffer ());
	//OglDrawArrays (GL_QUADS, 0, 4);
#else
GLfloat sPlane[4] = {1.0f, 0.0f, 0.0f, 0.0f};
GLfloat tPlane[4] = {0.0f, 1.0f, 0.0f, 0.0f};
GLfloat rPlane[4] = {0.0f, 0.0f, 1.0f, 0.0f};
GLfloat qPlane[4] = {0.0f, 0.0f, 0.0f, 1.0f};
// Set up the eye plane for projecting the shadow map on the scene
glEnable (GL_TEXTURE_GEN_S);
glEnable (GL_TEXTURE_GEN_T);
glEnable (GL_TEXTURE_GEN_R);
glEnable (GL_TEXTURE_GEN_Q);
glTexGenfv (GL_S, GL_EYE_PLANE, sPlane);
glTexGenfv (GL_T, GL_EYE_PLANE, tPlane);
glTexGenfv (GL_R, GL_EYE_PLANE, rPlane);
glTexGenfv (GL_Q, GL_EYE_PLANE, qPlane);
glTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
glTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
glTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
glTexGeni (GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

#	if 0
	CCamera* cameraP = cameraManager [shadowMaps [0]];
	if (cameraP) {
		shaderManager.Deploy (-1);
		SetDrawBuffer (GL_BACK, 0);
		ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
		ogl.BindTexture (cameraP->FrameBuffer ().DepthBuffer ());
		OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord);
		OglVertexPointer (2, GL_FLOAT, 0, quadVerts);
		glColor4f (1, 1, 1, 1);
		OglDrawArrays (GL_QUADS, 0, 4);
		}
#	endif
#endif
	}
}

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

#elif USE_SHADOW2DPROJ

	"uniform sampler2D sceneColor;\r\n" \
	"uniform sampler2D sceneDepth;\r\n" \
	"uniform sampler2DShadow shadowMap;\r\n" \
	"uniform mat4 modelviewProjInverse;\r\n" \
	"void main() {\r\n" \
	"float z = texture2D (sceneDepth, gl_TexCoord [0]).r;\r\n" \
	"vec4 ndcPos = vec4 (gl_TexCoord [0].s - 0.5, gl_TexCoord [0].t - 0.5, z - 0.5, 0.5);\r\n" \
	"vec4 wsPos = modelviewProjInverse * ndcPos;\r\n" \
	"vec4 lsPos = gl_TextureMatrix [2] * wsPos;\r\n" \
	"float light = 0.25 + shadow2DProj (shadowMap, lsPos) * 0.75;\r\n" \
	"gl_FragColor = vec4 (texture2D (sceneColor, gl_TexCoord [0].xy).rgb * light, 1.0);\r\n" \
	"}\r\n";
#else

	"uniform sampler2D sceneColor;\r\n" \
	"uniform sampler2D sceneDepth;\r\n" \
	"uniform sampler2D shadowMap;\r\n" \
	"uniform vec2 screenSize;\r\n" \
	"#define EyeZ(_z) (5000.0 / 4999.0) / ((5000.0 / 4999.0) - (_z))\r\n" \
	"//#define EyeZ(_z) 5000.0 / ((_z) * 4999.0 - 5000.0)\r\n" \
	"void main() {\r\n" \
	"float colorDepth = texture2D (sceneDepth, gl_TexCoord [0]).r;\r\n" \
	"float z = EyeZ (colorDepth);\r\n" \
	"vec2 ndc = vec2 ((gl_TexCoord [0].x - 0.5) * 2.0, (gl_TexCoord [0].y - 0.5) * 2.0);\r\n" \
	"vec4 ls = gl_TextureMatrix [2] * vec4 (ndc * -z, z, 1.0);\r\n" \
	"ls.xy /= -ls.z;\r\n" \
	"float shadowDepth = texture2D (shadowMap, ls.xy).r;\r\n" \
	"float light = 0.25 + ((colorDepth < shadowDepth + 0.0005) ? 0.75 : 0.0);\r\n" \
	"gl_FragColor = vec4 (texture2D (sceneColor, gl_TexCoord [0].xy).rgb * light, 1.0);\r\n" \
	"//gl_FragColor = vec4 (texture2D (shadowMap, gl_TexCoord [0].xy).rgb, 1.0);\r\n" \
	"//gl_FragColor = vec4 (vec3 (shadowDepth), 1.0);\r\n" \
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

