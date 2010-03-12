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

//-------------------------------------------------------------------------

const char *lightmapFS =
	"uniform sampler2D lMapTex;\r\n" \
	"void main(void){\r\n" \
	"vec4 texColor = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"gl_FragColor = vec4 (gl_Color.rgb + texColor.rgb, texColor.a);\r\n" \
	"}"
	;

const char *lightmapVS = 
	"void main(void){" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;"\
	"gl_Position = ftransform();"\
	"gl_FrontColor = gl_Color;}"
	;

// ----------------------------------------------------------------------------------------------

int lightmapShaderProg = -1;

int CreateLightmapShader (void)
{
if (!(ogl.m_states.bShadersOk && ogl.m_states.bPerPixelLightingOk)) {
	gameStates.render.bPerPixelLighting = 0;
	return 0;
	}
if (lightmapShaderProg >= 0)
	return 1;
PrintLog ("building light mask shader programs\n");
if (shaderManager.Build (lightmapShaderProg, lightmapFS, lightmapVS))
	return 1;
ogl.m_states.bPerPixelLightingOk = 0;
gameStates.render.bPerPixelLighting = 0;
shaderManager.Delete (lightmapShaderProg);
return -1;
}

// -----------------------------------------------------------------------------

void InitLightmapShader (void)
{
CreateLightmapShader ();
}

// -----------------------------------------------------------------------------

void ResetLightmapShader (void)
{
//memset (lightmapShaderProgs, 0xFF, sizeof (lightmapShaderProgs));
}

//------------------------------------------------------------------------------

int SetupLightmapShader (CSegFace *faceP)
{
PROF_START
#if DBG
if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (!SetupLightmap (faceP))
	return 0;
if ((lightmapShaderProg < 0) && !CreateLightmapShader ())
		return 0;
GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy (lightmapShaderProg));
if (!shaderProg)
	return -1;
if (shaderManager.Rebuild (shaderProg)) {
	glUniform1i (glGetUniformLocation (shaderProg, "lMapTex"), 0);
	ogl.ClearError (0);
	}
PROF_END(ptShaderStates)
return lightmapShaderProg;
}

// ----------------------------------------------------------------------------------------------
//eof
