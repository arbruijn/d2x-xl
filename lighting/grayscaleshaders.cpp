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
#include "gameseg.h"
#include "endlevel.h"
#include "renderthreads.h"
#include "light.h"
#include "lightmap.h"
#include "headlight.h"
#include "dynlight.h"

#define ONLY_LIGHTMAPS 0

#define PPL_AMBIENT_LIGHT	0.3f
#define PPL_DIFFUSE_LIGHT	0.7f

#if 1
#define GEO_LIN_ATT	(/*0.0*/ gameData.render.fAttScale [0])
#define GEO_QUAD_ATT	(/*0.003333f*/ gameData.render.fAttScale [1])
#define OBJ_LIN_ATT	(/*0.0*/ gameData.render.fAttScale [0])
#define OBJ_QUAD_ATT	(/*0.003333f*/ gameData.render.fAttScale [1])
#else
#define GEO_LIN_ATT	0.05f
#define GEO_QUAD_ATT	0.005f
#define OBJ_LIN_ATT	0.05f
#define OBJ_QUAD_ATT	0.005f
#endif

//------------------------------------------------------------------------------

int grayscaleShaderProgs [3] = {-1,-1,-1};

const char *grayScaleFS [3] = {
	"uniform sampler2D baseTex;\r\n" \
	"uniform vec4 faceColor;\r\n" \
	"void main(void){" \
	"float l = (faceColor.r + faceColor.g + faceColor.b) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, faceColor.a);}"
	,
	"uniform sampler2D baseTex;\r\n" \
	"void main(void){" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"float l = (texColor.r * 0.3 + texColor.g * 0.59 + texColor.b * 0.11) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, texColor.a);}"
	,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"void main(void){" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"vec4 decalColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"float l = (texColor.r * 0.3 + texColor.g * 0.59 + texColor.b * 0.11) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, texColor.a);}"
};

const char *grayScaleVS [3] = {
	"void main(void){" \
	"gl_Position=ftransform();"\
	"gl_FrontColor=gl_Color;}"
	,
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_Position=ftransform();"\
	"gl_FrontColor=gl_Color;}"
	,
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_Position=ftransform();"\
	"gl_FrontColor=gl_Color;}"
};

//-------------------------------------------------------------------------

void DeleteGrayScaleShader (void)
{
for (int i = 0; i < 3; i++) 
	shaderManager.Delete (grayscaleShaderProgs [i]);
}

//-------------------------------------------------------------------------

void InitGrayScaleShader (void)
{
if (!(gameOpts->render.bUseShaders && ogl.m_states.bShadersOk))
	gameOpts->ogl.bGlTexMerge = 0;
else {
	PrintLog ("building grayscale shader programs\n");
	for (int i = 0; i < 3; i++) {
		if (!(gameStates.render.textures.bHaveGrayScaleShader = shaderManager.Build (grayscaleShaderProgs [i], grayScaleFS [i], grayScaleVS [i]))) {
			DeleteGrayScaleShader ();
			return;
			}
		}
	}
if (!(ogl.m_states.bGlTexMerge = gameOpts->ogl.bGlTexMerge)) {
	ogl.m_states.bLowMemory = 0;
	ogl.m_states.bHaveTexCompression = 0;
	PrintLog ("+++++ OpenGL shader texture merging has been disabled! +++++\n");
	}
}

//------------------------------------------------------------------------------

int SetupGrayScaleShader (int nType, tRgbaColorf *colorP)
{
if (!gameStates.render.textures.bHaveGrayScaleShader)
	return -1;

if (nType > 2)
	nType = 2;
GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy (grayscaleShaderProgs [nType]));
if (!shaderProg)
	return -1;
shaderManager.Rebuild (shaderProg);
if (!nType)
	glUniform4fv (glGetUniformLocation (shaderProg, "faceColor"), 1, reinterpret_cast<GLfloat*> (colorP));
else {
	glUniform1i (glGetUniformLocation (shaderProg, "baseTex"), 0);
	if (nType > 1)
		glUniform1i (glGetUniformLocation (shaderProg, "decalTex"), 1);
	}
ogl.ClearError (0);
return grayscaleShaderProgs [nType];
}

// ----------------------------------------------------------------------------------------------
//eof
