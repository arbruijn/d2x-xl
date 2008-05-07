/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
#ifdef RCS
static char rcsid [] = "$Id: lighting.c,v 1.4 2003/10/04 03:14:47 btb Exp $";
#endif
#include <math.h>
#include <stdio.h>
#include <string.h>	// for memset ()

#include "inferno.h"
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

//------------------------------------------------------------------------------

GLhandleARB gsShaderProg [2][3] = {{0,0,0},{0,0,0}};
GLhandleARB gsf [2][3] = {{0,0,0},{0,0,0}};
GLhandleARB gsv [2][3] = {{0,0,0},{0,0,0}};

char *grayScaleFS [2][3] = {{
	"uniform sampler2D baseTex;\r\n" \
	"uniform vec4 faceColor;\r\n" \
	"void main(void){" \
	"float l = (faceColor.r + faceColor.g + faceColor.b) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, faceColor.a);}"
	,
	"uniform sampler2D baseTex;\r\n" \
	"void main(void){" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"float l = (texColor.r + texColor.g + texColor.b) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, texColor.a);}"
	,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"void main(void){" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"vec4 decalColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"float l = (texColor.r + texColor.g + texColor.b) / 4.0;\r\n" \
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
	"float l = (texColor.r + texColor.g + texColor.b) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, texColor.a);}"
	,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"void main(void){" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"vec4 decalColor = texture2D (baseTex, gl_TexCoord [2].xy);\r\n" \
	"texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"float l = (texColor.r + texColor.g + texColor.b) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, texColor.a);}"
	}};

char *grayScaleVS [2][3] = {{
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
for (int i = 0; i < 2; i++) {
	for (int j = 0; j < 3; j++) {
		if (gsShaderProg [i][j]) {
			DeleteShaderProg (&gsShaderProg [i][j]);
			gsShaderProg [i][j] = 0;
			}
		}
	}
}

//-------------------------------------------------------------------------

void InitGrayScaleShader (void)
{
if (!gameStates.ogl.bShadersOk)
	gameOpts->ogl.bGlTexMerge = 0;
else {
	PrintLog ("building grayscale shader programs\n");
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 3; j++) {
			if (gsShaderProg [i][j])
				DeleteShaderProg (&gsShaderProg [i][j]);
			gameStates.render.textures.bHaveGrayScaleShader = 
				CreateShaderProg (&gsShaderProg [i][j]) &&
				CreateShaderFunc (&gsShaderProg [i][j], &gsf [i][j], &gsv [i][j], grayScaleFS [i][j], grayScaleVS [i][j], 1) &&
				LinkShaderProg (&gsShaderProg [i][j]);
			if (!gameStates.render.textures.bHaveGrayScaleShader) {
				DeleteGrayScaleShader ();
				return;
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------------------------
// per pixel lighting, no lightmaps
// 2 - 8 light sources

char *pszPPXLightingFS [] = {
	"#define LIGHTS 8\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = gl_Color * bStaticColor;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a;\r\n" \
	"		float dist = lightDist - lightRad;\r\n" \
	"		float att = 1.0, NdotL = 1.0;\r\n" \
	"		vec4 color;\r\n" \
	"		if (dist <= 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			float NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * max (NdotL, 0.0) + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*specular highlight >>>>>\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}\r\n" \
	"		*/\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (vec4 (1.0, 1.0, 1.0, 1.0), colorSum);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a;\r\n" \
	"		float dist = lightDist - lightRad;\r\n" \
	"		float att = 1.0, NdotL = 1.0;\r\n" \
	"		vec4 color;\r\n" \
	"		if (dist <= 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			float NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * max (NdotL, 0.0) + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*specular highlight >>>>>\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}\r\n" \
	"		*/\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * colorSum);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a;\r\n" \
	"		float dist = lightDist - lightRad;\r\n" \
	"		float att = 1.0, NdotL = 1.0;\r\n" \
	"		vec4 color;\r\n" \
	"		if (dist <= 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			float NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * max (NdotL, 0.0) + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*specular highlight >>>>>\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}\r\n" \
	"		*/\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * colorSum);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 colorSum = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a;\r\n" \
	"		float dist = lightDist - lightRad;\r\n" \
	"		float att = 1.0, NdotL = 1.0;\r\n" \
	"		vec4 color;\r\n" \
	"		if (dist <= 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			float NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * max (NdotL, 0.0) + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*specular highlight >>>>>\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}\r\n" \
	"		*/\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * colorSum);}\r\n" \
	"	}"
	};

// ----------------------------------------------------------------------------------------------
// one light source

char *pszPP1LightingFS [] = {
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color= gl_Color * bStaticColor;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (vec4 (1.0, 1.0, 1.0, 1.0), colorSum * gl_LightSource [0].constantAttenuation);\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * color * gl_LightSource [0].constantAttenuation);\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * color * gl_LightSource [0].constantAttenuation);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		vec3 NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;*/\r\n" \
	"		}\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * color * gl_LightSource [0].constantAttenuation);}\r\n" \
	"	}"
	};

// ----------------------------------------------------------------------------------------------

char *pszPPLightingVS [] = {
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	};
	
//-------------------------------------------------------------------------

char *pszLightingFS [] = {
	"void main() {\r\n" \
	"gl_FragColor = gl_Color;\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex;\r\n" \
	"void main() {\r\n" \
	"	gl_FragColor = texture2D (baseTex, gl_TexCoord [0].xy) * gl_Color;\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"void main() {\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	gl_FragColor = texColor * gl_Color;\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	gl_FragColor = texColor * gl_Color;}\r\n" \
	"	}"
	};


char *pszLightingVS [] = {
	"void main() {\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
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
	};
	
// ----------------------------------------------------------------------------------------------

char *pszPPLMXLightingFS [] = {
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = gl_Color;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		float att = 1.0;\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a;\r\n" \
	"		float dist = lightDist - lightRad;\r\n" \
	"		float NdotL = 1.0;\r\n" \
	"		vec4 color;\r\n" \
	"		if (dist <= 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			float NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * max (NdotL, 0.0) + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*specular highlight >>>>>\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}\r\n" \
	"		<<<<< specular highlight*/\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * colorSum);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a;\r\n" \
	"		float dist = lightDist - lightRad;\r\n" \
	"		float att = 1.0, NdotL = 1.0;\r\n" \
	"		vec4 color;\r\n" \
	"		if (dist <= 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * max (NdotL, 0.0) + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*specular highlight >>>>>\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}\r\n" \
	"		<<<<< specular highlight*/\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * colorSum * bStaticColor);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a;\r\n" \
	"		float dist = lightDist - lightRad;\r\n" \
	"		float att = 1.0, NdotL = 1.0;\r\n" \
	"		vec4 color;\r\n" \
	"		if (dist <= 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * max (NdotL, 0.0) + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*specular highlight >>>>>\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}\r\n" \
	"		<<<<< specular highlight*/\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * colorSum);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex, decalTex, maskTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [3].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a;\r\n" \
	"		float dist = lightDist - lightRad;\r\n" \
	"		float att = 1.0, NdotL = 1.0;\r\n" \
	"		vec4 color;\r\n" \
	"		if (dist <= 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * max (NdotL, 0.0) + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		/*specular highlight >>>>>\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}\r\n" \
	"		<<<<< specular highlight*/\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * colorSum);}\r\n" \
	"	}"
	};


// ----------------------------------------------------------------------------------------------

char *pszPPLM1LightingFS [] = {
	"uniform sampler2D lMapTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = gl_Color;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	float lightRad = gl_LightSource [0].specular.a;\r\n" \
	"	float dist = lightDist - lightRad;\r\n" \
	"  float att = 1.0, NdotL = 1.0;\r\n" \
	"	if (dist <= 0.0)\r\n" \
	"		color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att = 1.0 + gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		color += (gl_LightSource [0].diffuse * max (NdotL, 0.0) + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"	/*specular highlight >>>>>\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"		}\r\n" \
	"	<<<<< specular highlight*/\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * color * gl_LightSource [0].constantAttenuation);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	float lightRad = gl_LightSource [0].specular.a;\r\n" \
	"	float dist = lightDist - lightRad;\r\n" \
	"  float att = 1.0, NdotL = 1.0;\r\n" \
	"	if (dist <= 0.0)\r\n" \
	"		color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att = 1.0 + gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		color += (gl_LightSource [0].diffuse * max (NdotL, 0.0) + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"	/*specular highlight >>>>>\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"		}\r\n" \
	"	<<<<< specular highlight*/\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * color * gl_LightSource [0].constantAttenuation);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	float lightRad = gl_LightSource [0].specular.a;\r\n" \
	"	float dist = lightDist - lightRad;\r\n" \
	"  float att = 1.0, NdotL = 1.0;\r\n" \
	"	if (dist <= 0.0)\r\n" \
	"		color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		color += (gl_LightSource [0].diffuse * max (NdotL, 0.0) + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"	/*specular highlight >>>>>\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"		}\r\n" \
	"	<<<<< specular highlight*/\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * color * gl_LightSource [0].constantAttenuation);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex, maskTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [3].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	float lightRad = gl_LightSource [0].specular.a;\r\n" \
	"	float dist = lightDist - lightRad;\r\n" \
	"  float att = 1.0, NdotL = 1.0;\r\n" \
	"	if (dist <= 0.0)\r\n" \
	"		color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		NdotL = max (dot (n, lightVec / lightDist), 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		color += (gl_LightSource [0].diffuse * max (NdotL, 0.0) + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"	/*specular highlight >>>>>\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"		}\r\n" \
	"	<<<<< specular highlight*/\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * color * gl_LightSource [0].constantAttenuation);}\r\n" \
	"	}"
	};

// ----------------------------------------------------------------------------------------------

char *pszPPLMLightingVS [] = {
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	,
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"	gl_Position = ftransform();\r\n" \
	"	gl_TexCoord [0] = gl_MultiTexCoord0;\r\n" \
	"	gl_TexCoord [1] = gl_MultiTexCoord1;\r\n" \
	"	gl_TexCoord [2] = gl_MultiTexCoord2;\r\n" \
	"	gl_TexCoord [3] = gl_MultiTexCoord3;\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	};
	
//-------------------------------------------------------------------------

char *pszLMLightingFS [] = {
	"uniform sampler2D lMapTex;\r\n" \
	"void main() {\r\n" \
	"gl_FragColor = texture2D (lMapTex, gl_TexCoord [0].xy) * gl_Color;\r\n" \
	"}"
	,
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * color);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * color);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex, maskTex;\r\n" \
	"void main() {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [3].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	gl_FragColor = /*min (texColor, */(texColor * color);}\r\n" \
	"	}"
	};


char *pszLMLightingVS [] = {
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
	
//-------------------------------------------------------------------------

char *BuildLightingShader (char *pszTemplate, int nLights)
{
	int	l = (int) strlen (pszTemplate) + 1;
	char	*pszFS, szLights [2];

if (!(pszFS = (char *) D2_ALLOC (l)))
	return NULL;
if (nLights > MAX_LIGHTS_PER_PIXEL)
	nLights = MAX_LIGHTS_PER_PIXEL;
memcpy (pszFS, pszTemplate, l);
if (strstr (pszFS, "#define LIGHTS ") == pszFS) {
	sprintf (szLights, "%d", nLights);
	pszFS [15] = *szLights;
	}
#ifdef _DEBUG
PrintLog (" \n\nShader program:\n");
PrintLog (pszFS);
PrintLog (" \n");
#endif
return pszFS;
}

//-------------------------------------------------------------------------

GLhandleARB perPixelLightingShaderProgs [9][4] = 
	{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};

GLhandleARB ppLvs [9][4] = 
	{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}; 

GLhandleARB ppLfs [9][4] = 
	{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}; 

int InitPerPixelLightingShader (int nType, int nLights)
{
	int	i, j, bOk;
	char	*pszFS, *pszVS;
	char	**fsP, **vsP;

#if !PER_PIXEL_LIGHTING
gameStates.ogl.bPerPixelLightingOk =
gameOpts->ogl.bPerPixelLighting = 0;
#endif
if (!gameOpts->ogl.bPerPixelLighting)
	return -1;
#if 0
i = nLights;
if (perPixelLightingShaderProgs [i][nType])
	return nLights;
	{
#else
for (i = 0; i <= gameStates.render.nMaxLightsPerPass; i++) {
if (perPixelLightingShaderProgs [i][nType])
	continue;
#endif
	if (HaveLightMaps ()) {
		if (nLights) {
			fsP = (nLights == 1) ? pszPPLM1LightingFS : pszPPLMXLightingFS;
			vsP = pszPPLMLightingVS;
			//nLights = MAX_LIGHTS_PER_PIXEL;
			}
		else {
			fsP = pszLMLightingFS;
			vsP = pszLMLightingVS;
			}
		}
	else {
		if (nLights) {
			fsP = (nLights == 1) ? pszPP1LightingFS : pszPPXLightingFS;
			vsP = pszPPLightingVS;
			//nLights = MAX_LIGHTS_PER_PIXEL;
			}
		else {
			fsP = pszLightingFS;
			vsP = pszLightingVS;
			}
		}
	PrintLog ("building lighting shader programs\n");
	if ((gameStates.ogl.bPerPixelLightingOk = (gameStates.ogl.bShadersOk && gameOpts->render.nPath))) {
		pszFS = BuildLightingShader (fsP [nType], nLights);
		pszVS = BuildLightingShader (vsP [nType], nLights);
		bOk = (pszFS != NULL) && (pszVS != NULL) &&
				CreateShaderProg (perPixelLightingShaderProgs [i] + nType) &&
				CreateShaderFunc (perPixelLightingShaderProgs [i] + nType, ppLfs [i] + nType, ppLvs [i] + nType, pszFS, pszVS, 1) &&
				LinkShaderProg (perPixelLightingShaderProgs [i] + nType);
		D2_FREE (pszFS);
		D2_FREE (pszVS);
		if (!bOk) {
			gameStates.ogl.bPerPixelLightingOk =
			gameOpts->ogl.bPerPixelLighting = 0;
			for (i = 0; i <= MAX_LIGHTS_PER_PIXEL; i++)
				for (j = 0; j < 4; j++)
					DeleteShaderProg (perPixelLightingShaderProgs [i] + j);
			nLights = 0;
			return -1;
			}
		}
	}
return gameData.render.ogl.nPerPixelLights [nType] = nLights;
}

// ----------------------------------------------------------------------------------------------

void ResetPerPixelLightingShaders (void)
{
memset (perPixelLightingShaderProgs, 0, sizeof (perPixelLightingShaderProgs));
}

//------------------------------------------------------------------------------

int SetupHardwareLighting (grsFace *faceP, int nType)
{
	int						nLightRange, nLights;
	float						fBrightness;
	tRgbaColorf				ambient, diffuse;
	tRgbaColorf				black = {0,0,0,0};
	tRgbaColorf				specular = {0.5f,0.5f,0.5f,0.5f};
	fVector					vPos = {{0,0,0,1}};
	GLenum					hLight;
	tActiveShaderLight	*activeLightsP;
	tShaderLight			*psl;
	tShaderLightIndex		*sliP = &gameData.render.lights.dynamic.shader.index [0][0];

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if (faceP - FACES == nDbgFace)
	nDbgFace = nDbgFace;
#endif
retry:
if (!gameStates.ogl.iLight) {
#ifdef _DEBUG
	if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
	if (faceP - FACES == nDbgFace)
		nDbgFace = nDbgFace;
#endif
	gameStates.ogl.nLights = SetNearestFaceLights (faceP, nType != 0);
	if (gameStates.ogl.nLights > gameStates.render.nMaxLightsPerFace)
		gameStates.ogl.nLights = gameStates.render.nMaxLightsPerFace;
	gameStates.ogl.nFirstLight = sliP->nFirst;
	}
#if HW_VERTEX_LIGHTING == 0
glDisable (GL_LIGHTING);
#endif
activeLightsP = gameData.render.lights.dynamic.shader.activeLights [0] + gameStates.ogl.nFirstLight;
nLightRange = sliP->nLast - gameStates.ogl.nFirstLight + 1;
for (nLights = 0; 
	  (gameStates.ogl.iLight < gameStates.ogl.nLights) & (nLightRange > 0) && (nLights < gameStates.render.nMaxLightsPerPass); 
	  activeLightsP++, nLightRange--) { 
	if (!(psl = GetActiveShaderLight (activeLightsP, 0)))
		continue;
#ifdef _DEBUG
	if ((psl->nTarget < 0) && (-psl->nTarget - 1 != faceP->nSegment))
		faceP = faceP;
	else if ((psl->nTarget > 0) && (psl->nTarget != faceP - FACES + 1))
		faceP = faceP;
	if (!psl->nTarget)
		psl = psl;
#endif
	if (psl->bUsed == 2)	{//nearest vertex light
		ResetUsedLight (psl);
		sliP->nActive--;
		}
	hLight = GL_LIGHT0 + nLights++;
	glEnable (hLight);
#if HW_VERTEX_LIGHTING == 0
	specular.alpha = (psl->info.nSegment >= 0) ? psl->info.fRad : 0; //krasser Missbrauch!
#endif
	fBrightness = psl->info.fBrightness;
	if (psl->info.nType == 2) {
		glLightf (hLight, GL_CONSTANT_ATTENUATION, max (1.0f, psl->info.fBoost));
		glLightf (hLight, GL_LINEAR_ATTENUATION, 0.1f / fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.01f / fBrightness);
		ambient.red = psl->info.color.red * AMBIENT_LIGHT;
		ambient.green = psl->info.color.green * AMBIENT_LIGHT;
		ambient.blue = psl->info.color.blue * AMBIENT_LIGHT;
		ambient.alpha = 1.0f;
		diffuse.red = psl->info.color.red * DIFFUSE_LIGHT;
		diffuse.green = psl->info.color.green * DIFFUSE_LIGHT;
		diffuse.blue = psl->info.color.blue * DIFFUSE_LIGHT;
		diffuse.alpha = 1.0f;
		}
	else {
#if HW_VERTEX_LIGHTING
		glLightf (hLight, GL_CONSTANT_ATTENUATION, min (1.0f, 1.0f / psl->info.fRad));
#else
		glLightf (hLight, GL_CONSTANT_ATTENUATION, 1.0f);
#endif
		glLightf (hLight, GL_LINEAR_ATTENUATION, 0.1f / fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.01f / fBrightness);
		ambient.red = psl->info.color.red * AMBIENT_LIGHT;
		ambient.green = psl->info.color.green * AMBIENT_LIGHT;
		ambient.blue = psl->info.color.blue * AMBIENT_LIGHT;
		ambient.alpha = 1.0f;
		fBrightness *= DIFFUSE_LIGHT;
		diffuse.red = psl->info.color.red * fBrightness;
		diffuse.green = psl->info.color.green * fBrightness;
		diffuse.blue = psl->info.color.blue * fBrightness;
		diffuse.alpha = 1.0f;
		}
	glLightfv (hLight, GL_DIFFUSE, (GLfloat *) &diffuse);
	glLightfv (hLight, GL_SPECULAR, (GLfloat *) &specular);
	glLightfv (hLight, GL_AMBIENT, (GLfloat *) &ambient);
	glLightfv (hLight, GL_POSITION, (GLfloat *) (psl->vPosf));
	gameStates.ogl.iLight++;
	}
if (!nLightRange)
	gameStates.ogl.iLight = gameStates.ogl.nLights;
gameStates.ogl.nFirstLight = activeLightsP - gameData.render.lights.dynamic.shader.activeLights [0];
#ifdef _DEBUG
if ((gameStates.ogl.iLight < gameStates.ogl.nLights) && !nLightRange)
	nDbgSeg = nDbgSeg;
if ((gameStates.ogl.iLight >= gameStates.ogl.nLights) && (gameStates.ogl.nFirstLight < sliP->nLast)) {
	gameStates.ogl.iLight = 0;
	goto retry;
	}
#endif
#if 0
for (int i = nLights; i < 8; i++)
	glDisable (GL_LIGHT0 + i);
#endif
#if HW_VERTEX_LIGHTING
for (int i = nLights; i < 8; i++)
	glDisable (GL_LIGHT0 + i);
#else
if (InitPerPixelLightingShader (nType, nLights) >= 0)
	return nLights;
OglDisableLighting ();
#endif
return -1;
}

//------------------------------------------------------------------------------

int G3SetupPerPixelShader (grsFace *faceP, int nType)
{
	static grsBitmap	*nullBmP = NULL;

	int	bLightMaps, bStaticColor, nLights, nShader;

bStaticColor = (gameStates.ogl.iLight == 0);
if (0 > (nLights = SetupHardwareLighting (faceP, nType)))
	return 0;
#if HW_VERTEX_LIGHTING
return gameStates.render.history.nShader;
#endif
nShader = 20 + 4 * nLights + nType;
#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (bLightMaps = HaveLightMaps ()) {
	int i = (faceP - gameData.segs.faces.faces) / LIGHTMAP_BUFSIZE;
#if 1//def _DEBUG
	if (OglCreateLightMap (i))
#endif
		{INIT_TMU (InitTMU0, GL_TEXTURE0, nullBmP, lightMapData.buffers + i, 1, 1);}
	}
if (nShader != gameStates.render.history.nShader) {
	glUseProgramObject (0);
	glUseProgramObject (activeShaderProg = perPixelLightingShaderProgs [nLights][nType]);
	if (bLightMaps)
		glUniform1i (glGetUniformLocation (activeShaderProg, "lMapTex"), 0);
	if (nType) {
		glUniform1i (glGetUniformLocation (activeShaderProg, "baseTex"), bLightMaps);
		if (nType > 1) {
			glUniform1i (glGetUniformLocation (activeShaderProg, "decalTex"), 1 + bLightMaps);
			if (nType > 2)
				glUniform1i (glGetUniformLocation (activeShaderProg, "maskTex"), 2 + bLightMaps);
			}
		}
	}
if (nLights)
	glUniform1f (glGetUniformLocation (activeShaderProg, "bStaticColor"), 
					 (float) nLights / (float) gameStates.ogl.nLights);
					 //bStaticColor ? 1.0f : 0.0f);
return gameStates.render.history.nShader = nShader;
}

//------------------------------------------------------------------------------

int G3SetupGrayScaleShader (int nType, tRgbaColorf *colorP)
{
if (gameStates.render.textures.bHaveGrayScaleShader) {
	if (nType > 2)
		nType = 2;
	int bLightMaps = HaveLightMaps ();
	int nShader = 90 + 3 * bLightMaps + nType;
	if (gameStates.render.history.nShader != nShader) {
		glUseProgramObject (activeShaderProg = gsShaderProg [bLightMaps][nType]);
		if (!nType) 
			glUniform4fv (glGetUniformLocation (activeShaderProg, "faceColor"), 1, (GLfloat *) colorP);
		else {
			glUniform1i (glGetUniformLocation (activeShaderProg, "baseTex"), bLightMaps);
			if (nType > 1)
				glUniform1i (glGetUniformLocation (activeShaderProg, "decalTex"), 1 + bLightMaps);
			}
		gameStates.render.history.nShader = nShader;
		}
	}
return gameStates.render.history.nShader;
}

// ----------------------------------------------------------------------------------------------
//eof
