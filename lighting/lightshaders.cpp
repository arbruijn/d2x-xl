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
#include "ogl_color.h"
#include "ogl_shader.h"
#include "gameseg.h"
#include "endlevel.h"
#include "renderthreads.h"
#include "light.h"
#include "lightmap.h"
#include "headlight.h"
#include "dynlight.h"

// ----------------------------------------------------------------------------------------------
// per pixel lighting, no lightmaps
// 2 - 8 light sources

char *pszPPXLightingFS [] = {
	"#define LIGHTS 8\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (vec4 (1.0, 1.0, 1.0, 1.0), color);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);\r\n" \
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
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			vec3 NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);}\r\n" \
	"	}"
	};

// ----------------------------------------------------------------------------------------------
// one light source

char *pszPP1LightingFS [] = {
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = gl_Color * bStaticColor;\r\n" \
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
	"			color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (vec4 (1.0, 1.0, 1.0, 1.0), color);\r\n" \
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
	"			color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);\r\n" \
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
	"			color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);\r\n" \
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
	"			color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"		vec3 NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);}\r\n" \
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
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (vec4 (1.0, 1.0, 1.0, 1.0), color);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);\r\n" \
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
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) {\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			float att = 1.0;\r\n" \
	"			float dist = length (lightVec) - gl_LightSource [i].specular.a;\r\n" \
	"			if (dist <= 0.0) {\r\n" \
	"				color += gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"				}\r\n" \
	"			else {\r\n" \
	"				att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"				if (gl_LightSource [i].specular.a > 0.0)\r\n" \
	"					NdotL += (1.0 - NdotL) / att;\r\n" \
	"				color += (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"				}\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			vec3 NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"			color += (gl_LightSource [i].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);}\r\n" \
	"	}"
	};


// ----------------------------------------------------------------------------------------------

char *pszPPLM1LightingFS [] = {
	"uniform sampler2D lMapTex;\r\n" \
	"uniform float bStaticColor, fBoost;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * bStaticColor;\r\n" \
	"	vec3 n = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [0].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [0].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"			}\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [0].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (vec4 (1.0, 1.0, 1.0, 1.0), color);\r\n" \
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
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [0].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [0].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"			}\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [0].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);\r\n" \
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
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [0].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [0].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"			}\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [0].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);\r\n" \
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
	"	float NdotL = max (dot (n, normalize (lightVec)), 0.0);\r\n" \
	"	if (NdotL >= 0.0) {\r\n" \
	"		float att = 1.0;\r\n" \
	"		float dist = length (lightVec) - gl_LightSource [0].specular.a;\r\n" \
	"		if (dist <= 0.0) {\r\n" \
	"			color += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"			}\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"			if (gl_LightSource [0].specular.a > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"			}\r\n" \
	"		vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"		float NdotHV = max (dot (n, halfV), 0.0);\r\n" \
	"		color += (gl_LightSource [0].specular * pow (NdotHV, 2.0)) / att;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);}\r\n" \
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
	"gl_FragColor = color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	gl_FragColor = min (texColor, texColor * color);\r\n" \
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
	"	gl_FragColor = min (texColor, texColor * color);}\r\n" \
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
	return 0;
i = nLights;
if (perPixelLightingShaderProgs [i][nType])
	return nLights;
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
		}
	}
return gameData.render.ogl.nPerPixelLights [nType] = nLights;
}

// ----------------------------------------------------------------------------------------------

void ResetPerPixelLightingShaders (void)
{
memset (perPixelLightingShaderProgs, 0, sizeof (perPixelLightingShaderProgs));
}

// ----------------------------------------------------------------------------------------------
//eof
