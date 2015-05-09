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

//------------------------------------------------------------------------------

int32_t grayscaleShaderProgs [2][3] = {{-1,-1,-1},{-1,-1,-1}};

const char *grayScaleFS [2][3] = {{
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
	"float l = (texColor.r * 0.3 + texColor.g * 0.584 + texColor.b * 0.116) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, texColor.a);}"
	},
 {
	"uniform sampler2D baseTex;\r\n" \
	"uniform vec4 faceColor;\r\n" \
	"void main(void){" \
	"float l = (faceColor.r + faceColor.g + faceColor.b) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, faceColor.a);}"
	,
	"uniform sampler2D baseTex;\r\n" \
	"void main(void){" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"float l = (texColor.r * 0.3 + texColor.g * 0.584 + texColor.b * 0.116) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, texColor.a);}"
	,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"void main(void){" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"vec4 decalColor = texture2D (baseTex, gl_TexCoord [2].xy);\r\n" \
	"texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"float l = (texColor.r * 0.3 + texColor.g * 0.584 + texColor.b * 0.116) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, texColor.a);}"
	}};

const char *grayScaleVS [2][3] = {{
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
	},
 {
	"void main(void){" \
	"gl_Position=ftransform();"\
	"gl_FrontColor=gl_Color;}"
	,
	"void main(void){" \
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_Position=ftransform();"\
	"gl_FrontColor=gl_Color;}"
	,
	"void main(void){" \
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_TexCoord [2]=gl_MultiTexCoord2;"\
	"gl_Position=ftransform();"\
	"gl_FrontColor=gl_Color;}"
	}};

//-------------------------------------------------------------------------

void DeleteGrayScaleShader (void)
{
for (int32_t i = 0; i < 2; i++)
	for (int32_t j = 0; j < 3; j++) 
		shaderManager.Delete (grayscaleShaderProgs [i][j]);
}

//-------------------------------------------------------------------------

void InitGrayScaleShader (void)
{
if (!(gameOpts->render.bUseShaders && ogl.m_features.bShaders))
	gameOpts->ogl.bGlTexMerge = 0;
else {
	PrintLog (0, "building grayscale shader programs\n");
	for (int32_t i = 0; i < 2; i++) {
		for (int32_t j = 0; j < 3; j++) {
			if (!(gameStates.render.textures.bHaveGrayScaleShader = shaderManager.Build (grayscaleShaderProgs [i][j], grayScaleFS [i][j], grayScaleVS [i][j]))) {
				DeleteGrayScaleShader ();
				return;
				}
			}
		}
	}
if (!gameOpts->ogl.bGlTexMerge) {
	ogl.m_states.bLowMemory = 0;
	ogl.m_features.bTextureCompression = 0;
	PrintLog (0, "+++++ OpenGL shader texture merging has been disabled! +++++\n");
	}
}

//------------------------------------------------------------------------------

int32_t SetupGrayScaleShader (int32_t nType, CFloatVector *pColor)
{
if (!gameStates.render.textures.bHaveGrayScaleShader)
	return -1;

if (nType > 2)
	nType = 2;
int32_t bLightmaps = lightmapManager.HaveLightmaps ();
GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy (grayscaleShaderProgs [bLightmaps][nType]));
if (!shaderProg)
	return -1;
shaderManager.Rebuild (shaderProg);
if (!nType)
	glUniform4fv (glGetUniformLocation (shaderProg, "faceColor"), 1, reinterpret_cast<GLfloat*> (pColor));
else {
	glUniform1i (glGetUniformLocation (shaderProg, "baseTex"), bLightmaps);
	if (nType > 1)
		glUniform1i (glGetUniformLocation (shaderProg, "decalTex"), 1 + bLightmaps);
	}
ogl.ClearError (0);
return grayscaleShaderProgs [bLightmaps][nType];
}

// ----------------------------------------------------------------------------------------------
//eof
