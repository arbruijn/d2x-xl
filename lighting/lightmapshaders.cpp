#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>	// for memset ()

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "fix.h"
#include "vecmat.h"
#include "network.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_tmu.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "segmath.h"
#include "endlevel.h"
#include "renderthreads.h"
#include "light.h"
#include "lightmap.h"
#include "headlight.h"
#include "dynlight.h"

#define ONLY_LIGHTMAPS 0
#define CONST_LIGHT_COUNT 1

//-------------------------------------------------------------------------

const char *pszLMLightingFS [] = {
	"uniform sampler2D lMapTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float fLightScale;\r\n" \
	"void main() {\r\n" \
	"vec4 color = (texture2D (lMapTex, gl_TexCoord [0].xy) + gl_Color) * fLightScale;\r\n" \
	"gl_FragColor = vec4 (matColor.rgb * min (vec3 (1.0, 1.0, 1.0), color.rgb), matColor.a * gl_Color.a);\r\n" \
	"}"
	,
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = (texture2D (lMapTex, gl_TexCoord [0].xy) + gl_Color) * fLightScale;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	gl_FragColor = vec4 (texColor.rgb * min (vec3 (1.0, 1.0, 1.0), color.rgb), texColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = (texture2D (lMapTex, gl_TexCoord [0].xy) + gl_Color) * fLightScale;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), min (texColor.a + decalColor.a, 1.0));\r\n" \
	"	gl_FragColor = vec4 (texColor.rgb * min (vec3 (1.0, 1.0, 1.0), color.rgb), texColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex, maskTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [3].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 color = (texture2D (lMapTex, gl_TexCoord [0].xy) + gl_Color) * fLightScale;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), min (texColor.a + decalColor.a, 1.0));\r\n" \
	"	gl_FragColor =  vec4 (texColor.rgb * min (vec3 (1.0, 1.0, 1.0), color.rgb), texColor.a * gl_Color.a);\r\n" \
	"	}\r\n" \
	"}"
	};

//-------------------------------------------------------------------------

const char *pszLMLightingVS [] = {
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"void main() {\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
	"	gl_TexCoord [3] = gl_MultiTexCoord3;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	};

// ----------------------------------------------------------------------------------------------

int lightmapShaderProgs [4] = {-1,-1,-1,-1};

int CreateLightmapShader (int nType)
{
	int	h, j;

if (!(ogl.m_features.bShaders && ogl.m_features.bPerPixelLighting)) {
	gameStates.render.bPerPixelLighting = 0;
	return 0;
	}
if (lightmapShaderProgs [nType] >= 0)
	return 1;
for (h = 0; h <= 3; h++) {
	if (lightmapShaderProgs [h] >= 0)
		continue;
	PrintLog (0, "building lightmap shader programs\n");
	if (!shaderManager.Build (lightmapShaderProgs [h], pszLMLightingFS [h], pszLMLightingVS [h])) {
		ogl.m_features.bPerPixelLighting = 0;
		gameStates.render.bPerPixelLighting = 0;
		for (j = 0; j < 4; j++)
			shaderManager.Delete (lightmapShaderProgs [j]);
		return -1;
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

void InitLightmapShaders (void)
{
for (int nType = 0; nType < 4; nType++)
	CreateLightmapShader (nType);
}

// -----------------------------------------------------------------------------

void ResetLightmapShaders (void)
{
//memset (lightmapShaderProgs, 0xFF, sizeof (lightmapShaderProgs));
}

//------------------------------------------------------------------------------

int SetupLightmapShader (CSegFace *faceP, int nType, bool bHeadlight)
{
PROF_START
	//static CBitmap	*nullBmP = NULL;

if (!CreateLightmapShader (nType))
	return 0;
#if DBG
if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (!SetupLightmap (faceP))
	return 0;
#if CONST_LIGHT_COUNT
GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy (lightmapShaderProgs [nType]));
#else
GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy (perPixelLightingShaderProgs [nLights][nType]));
#endif
if (!shaderProg)
	return -1;

if (shaderManager.Rebuild (shaderProg))
	;
	{
	ogl.ClearError (0);
	glUniform1f (glGetUniformLocation (shaderProg, "fLightScale"), 
					 ((gameOpts->render.stereo.nGlasses > 0) && (!gameOpts->app.bExpertMode || gameOpts->render.stereo.bBrighten)) 
					 ? 2.0f 
					 : paletteManager.Brightness ());
	glUniform1i (glGetUniformLocation (shaderProg, "lMapTex"), 0);
	if (nType) {
		glUniform1i (glGetUniformLocation (shaderProg, "baseTex"), 1);
		if (nType > 1) {
			glUniform1i (glGetUniformLocation (shaderProg, "decalTex"), 2);
			if (nType > 2)
				glUniform1i (glGetUniformLocation (shaderProg, "maskTex"), 3);
			}
		}
	}
if (!nType) {
	glUniform4fv (glGetUniformLocation (shaderProg, "matColor"), 1, reinterpret_cast<GLfloat*> (&faceP->m_info.color));
	}
ogl.ClearError (0);
PROF_END(ptShaderStates)
#if CONST_LIGHT_COUNT
return lightmapShaderProgs [nType];
#else
return perPixelLightingShaderProgs [nLights][nType];
#endif
}


// ----------------------------------------------------------------------------------------------
//eof
