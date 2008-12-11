// Copyright (c) Dietfrid Mali

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "inferno.h"
#include "error.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "gameseg.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "renderlib.h"
#include "network.h"
#include "glare.h"

GLhandleARB hBlurShader = 0;
GLhandleARB hBlurVS = 0;
GLhandleARB hBlurFS = 0;

const char *blurFS = 
	"uniform sampler2D blurTex;\r\n" \
	"uniform float width;\r\n" \
	"uniform float height;\r\n" \
	"void main(void) {\r\n" \
	"float dx = 1.0/width;\r\n" \
	"float dy = 1.0/height;\r\n" \
	"vec2 tc = gl_TexCoord [0].st;\r\n" \
	"vec4 color = texture2D(blurTex, tc + vec2 (-dx, -dy)) * (1.0/16.0);\r\n" \
	"color += texture2D (blurTex, tc + vec2 (0.0, -dy)) * (2.0/16.0);\r\n" \
	"color += texture2D (blurTex, tc + vec2 (dx, -dy)) * (1.0/16.0);\r\n" \
	"color += texture2D (blurTex, tc + vec2 (-dx, 0.0)) * (2.0/16.0);\r\n" \
	"color += texture2D (blurTex, tc) * (4.0/16.0);\r\n" \
	"color += texture2D (blurTex, tc + vec2 (dx, 0.0)) * (2.0/16.0);\r\n" \
	"color += texture2D (blurTex, tc + vec2 (-dx, dy)) * (1.0/16.0);\r\n" \
	"color += texture2D (blurTex, tc + vec2 (0.0, dy)) * (2.0/16.0);\r\n" \
	"color += texture2D (blurTex, tc + vec2 (dx, dy)) * (1.0/16.0);\r\n" \
	"gl_FragColor = color;}\r\n";

const char *blurVS =
	"void main(void){\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"gl_Position = ftransform();\r\n" \
	"gl_FrontColor = gl_Color;}\r\n";

//-------------------------------------------------------------------------

void InitBlurShader (void)
{
	int	bOk;

PrintLog ("building blur shader program\n");
DeleteShaderProg (NULL);
if (gameStates.ogl.bRender2TextureOk && gameStates.ogl.bShadersOk && RENDERPATH) {
	gameStates.ogl.bHaveBlur = 1;
	if (hBlurShader)
		DeleteShaderProg (&hBlurShader);
	bOk = CreateShaderProg (&hBlurShader) &&
			CreateShaderFunc (&hBlurShader, &hBlurFS, &hBlurVS, blurFS, blurVS, 1) &&
			LinkShaderProg (&hBlurShader);
	if (!bOk) {
		gameStates.ogl.bHaveBlur = 0;
		DeleteShaderProg (&hBlurShader);
		return;
		}
	OglClearError (0);
	}
}

//------------------------------------------------------------------------------
// eof
