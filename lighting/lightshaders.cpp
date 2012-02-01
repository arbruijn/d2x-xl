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

// ----------------------------------------------------------------------------------------------
// per pixel lighting, no lightmaps
// 2 - 8 light sources

const char *pszPPXLightingFS [] = {
	"#define LIGHTS 8\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = gl_Color;\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) if (i < nLights) {\r\n" \
	"		vec4 color;\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		vec3 lightNorm = lightVec / lightDist;\r\n" \
	"     /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"     float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"		float lightAngle = /*bDirected **/ ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	   lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"		float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	   if (dist == 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			/*specular highlight >>>>>*/\r\n" \
	"			if (NdotL >= 0.0) {\r\n" \
	"				vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"				float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"				color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"				}/*<<<<< specular highlight*/\r\n" \
	"			NdotL = max (NdotL, 0.0);\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = vec4 (min (matColor.rgb, matColor.rgb * colorSum.rgb) * fLightScale, matColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = gl_Color;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) if (i < nLights) {\r\n" \
	"		vec4 color;\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		vec3 lightNorm = lightVec / lightDist;\r\n" \
	"     /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"     float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"		float lightAngle = /*bDirected **/ ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	   lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"		float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"		if (dist == 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			/*specular highlight >>>>>*/\r\n" \
	"			if (NdotL >= 0.0) {\r\n" \
	"				vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"				float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"				color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"				}\r\n" \
	"			/*<<<<< */\r\n" \
	"			NdotL = max (NdotL, 0.0);\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, texColor.rgb * colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = gl_Color;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy) ;\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) if (i < nLights) {\r\n" \
	"		vec4 color;\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		vec3 lightNorm = lightVec / lightDist;\r\n" \
	"     /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"     float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"		float lightAngle = /*bDirected **/ ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	   lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"		float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"		if (dist == 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			/*NdotL = 1.0 - ((1.0 - NdotL) * 0.95);*/\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			/*specular highlight >>>>>*/\r\n" \
	"			if (NdotL >= 0.0) {\r\n" \
	"				vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"				float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"				color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"				}/*<<<<< specular highlight*/\r\n" \
	"			NdotL = max (NdotL, 0.0);\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, texColor.rgb * colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nType;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"if ((nType == 3) && (texture2D (maskTex, gl_TexCoord [2].xy).r < 0.5))\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 colorSum = gl_Color;\r\n" \
	"	vec4 texColor = (nType == 0) ? vec4 (0.0, 0.0, 0.0, 0.0) : texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = (nType == 1) ? vec4 (0.0, 0.0, 0.0, 0.0) : texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) if (i < nLights) {\r\n" \
	"		vec4 color;\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		vec3 lightNorm = lightVec / lightDist;\r\n" \
	"     /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"     float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"		float lightAngle = /*bDirected **/ ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	   lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"		float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"		if (dist == 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			/*NdotL = 1.0 - ((1.0 - NdotL) * 0.95);*/\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			/*specular highlight >>>>>*/\r\n" \
	"			if (NdotL >= 0.0) {\r\n" \
	"				vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"				float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"				color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"				}/*<<<<< specular highlight*/\r\n" \
	"			NdotL = max (NdotL, 0.0);\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, texColor.rgb * colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);\r\n" \
	"	}\r\n" \
	"}"
	};

// ----------------------------------------------------------------------------------------------
// one light source

const char *pszPP1LightingFS [] = {
	"uniform vec4 matColor;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = gl_Color;\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	vec3 lightNorm = lightVec / lightDist;\r\n" \
	"  /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"  float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"	float lightAngle = /*bDirected **/ min (NdotL, -dot (lightNorm, gl_LightSource [0].spotDirection));\r\n" \
	"	float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"	float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	if (dist == 0.0)\r\n" \
	"		colorSum = gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		/*specular highlight >>>>>*/\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"			colorSum += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}/*<<<<< specular highlight*/\r\n" \
	"		NdotL = max (NdotL, 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		colorSum = (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"  colorSum.rgb *= gl_LightSource [0].constantAttenuation;\r\n" \
	"	gl_FragColor = vec4 (min (matColor.rgb, matColor.rgb * colorSum.rgb) * fLightScale, matColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = gl_Color;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	vec3 lightNorm = lightVec / lightDist;\r\n" \
	"  /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"  float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"	float lightAngle = /*bDirected **/ min (NdotL, -dot (lightNorm, gl_LightSource [0].spotDirection));\r\n" \
	"	float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"	float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	if (dist == 0.0)\r\n" \
	"		colorSum = gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		/*specular highlight >>>>>*/\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"			colorSum += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}/*<<<<< specular highlight*/\r\n" \
	"		NdotL = max (NdotL, 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		colorSum = (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"  colorSum *= texColor * gl_LightSource [0].constantAttenuation;\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = gl_Color;\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy) ;\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	vec3 lightNorm = lightVec / lightDist;\r\n" \
	"  /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"  float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"	float lightAngle = /*bDirected **/ min (NdotL, -dot (lightNorm, gl_LightSource [0].spotDirection));\r\n" \
	"	float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"	float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	if (dist == 0.0)\r\n" \
	"		colorSum = gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		/*specular highlight >>>>>*/\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"			colorSum += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}/*<<<<< specular highlight*/\r\n" \
	"		NdotL = max (NdotL, 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		colorSum = (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"  colorSum *= texColor * gl_LightSource [0].constantAttenuation;\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nType;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"if ((nType == 3) && (texture2D (maskTex, gl_TexCoord [3].xy).r < 0.5))\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 colorSum = gl_Color;\r\n" \
	"	vec4 texColor = (nType == 0) ? vec4 (0.0, 0.0, 0.0, 0.0) : texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = (nType == 1) ? vec4 (0.0, 0.0, 0.0, 0.0) : texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	vec3 lightNorm = lightVec / lightDist;\r\n" \
	"  /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"  float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"	float lightAngle = /*bDirected **/ min (NdotL, -dot (lightNorm, gl_LightSource [0].spotDirection));\r\n" \
	"	float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"	float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	if (dist == 0.0)\r\n" \
	"		colorSum = gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		/*specular highlight >>>>>*/\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"			colorSum += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}/*<<<<< specular highlight*/\r\n" \
	"		NdotL = max (NdotL, 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		colorSum = (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"  colorSum *= texColor * gl_LightSource [0].constantAttenuation;\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);\r\n" \
	"	}"
	};

// ----------------------------------------------------------------------------------------------

const char *pszPPLightingVS [] = {
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

const char *pszLightingFS [] = {
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
	"	gl_FragColor = vec4 (0.0, 1.0, 0.0, 1.0); /*texColor * gl_Color;*/\r\n" \
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


const char *pszLightingVS [] = {
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
// per pixel lighting with lightmaps, 2 to 8 light sources per pass

const char *pszPPXLMLightingFS [] = {
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) if (i < nLights) {\r\n" \
	"		vec4 color;\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		vec3 lightNorm = lightVec / lightDist;\r\n" \
	"     /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"     float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"		float lightAngle = /*bDirected **/ ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	   lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"		float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"		  if (dist == 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			/*specular highlight >>>>>*/\r\n" \
	"			if (NdotL >= 0.0) {\r\n" \
	"				vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"				float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"				color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"				}/*<<<<< specular highlight*/\r\n" \
	"			NdotL = max (NdotL, 0.0);\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"			}\r\n" \
	"		}\r\n" \
	"	gl_FragColor = vec4 (min (matColor.rgb, matColor.rgb * colorSum.rgb) * fLightScale, matColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"/*	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) if (i < nLights) {\r\n" \
	"		vec4 color;\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		vec3 lightNorm = lightVec / lightDist;\r\n" \
	"     float NdotL = (lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"		float lightAngle = ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	   lightDist -= 100.0 * min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"		float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	   if (dist == 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			//specular highlight >>>>>\r\n" \
	"			if (NdotL >= 0.0) {\r\n" \
	"				vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"				float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"				color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"				}//<<<<< specular highlight\r\n" \
	"			NdotL = max (NdotL, 0.0);\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"  colorSum *= texColor;\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);*/\r\n" \
	"	gl_FragColor = vec4 (1.0, 0.5, 0.0, 1.0);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"/*	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy) ;\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) if (i < nLights) {\r\n" \
	"     vec4 color;\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		vec3 lightNorm = lightVec / lightDist;\r\n" \
	"     float NdotL = (lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"		float lightAngle = ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	   lightDist -= 100.0 * min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"		float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"		  if (dist == 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			//specular highlight >>>>>\r\n" \
	"			if (NdotL >= 0.0) {\r\n" \
	"				vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"				float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"				color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"				}//<<<<< specular highlight\r\n" \
	"			NdotL = max (NdotL, 0.0);\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"  colorSum *= texColor;\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);*/\r\n" \
	"	gl_FragColor = vec4 (1.0, 0.5, 0.0, 1.0);\r\n" \
	"	}"
	,
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex, baseTex, decalTex, maskTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nType;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"if ((nType == 3) && (texture2D (maskTex, gl_TexCoord [3].xy).r < 0.5))\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = (nType == 0) ? vec4 (1.0, 1.0, 1.0, 1.0) : texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = (nType == 1) ? vec4 (0.0, 0.0, 0.0, 0.0) : texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"  texColor.rgb *= gl_Color.rgb;\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	int i;\r\n" \
	"	for (i = 0; i < LIGHTS; i++) if (i < nLights) {\r\n" \
	"		vec4 color;\r\n" \
	"		vec3 lightVec = vec3 (gl_LightSource [i].position) - vertPos;\r\n" \
	"		float lightDist = length (lightVec);\r\n" \
	"		vec3 lightNorm = lightVec / lightDist;\r\n" \
	"     float NdotL = (lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"		float lightAngle = ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"		float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	   lightDist -= 100.0 * min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"		float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	   if (dist == 0.0)\r\n" \
	"			color = gl_LightSource [i].diffuse + gl_LightSource [i].ambient;\r\n" \
	"		else {\r\n" \
	"			att += gl_LightSource [i].linearAttenuation * dist + gl_LightSource [i].quadraticAttenuation * dist * dist;\r\n" \
	"			//specular highlight >>>>>\r\n" \
	"			if (NdotL >= 0.0) {\r\n" \
	"				vec3 halfV = normalize (gl_LightSource [i].halfVector.xyz);\r\n" \
	"				float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"				color += (gl_LightSource [i].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"				}//<<<<< specular highlight\r\n" \
	"			NdotL = max (NdotL, 0.0);\r\n" \
	"			if (lightRad > 0.0)\r\n" \
	"				NdotL += (1.0 - NdotL) / att;\r\n" \
	"			color = (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"  colorSum *= texColor;\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);\r\n" \
	"	}"
	"}"
	};


// ----------------------------------------------------------------------------------------------

const char *pszPP1LMLightingFS [] = {
	"uniform sampler2D lMapTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	vec3 lightNorm = lightVec / lightDist;\r\n" \
	"  /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"  float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"	float lightAngle = /*bDirected **/ ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"	float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"	float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	  if (dist == 0.0)\r\n" \
	"		colorSum = gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		/*specular highlight >>>>>*/\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"			colorSum += (gl_LightSource [0].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}/*<<<<< specular highlight*/\r\n" \
	"		NdotL = max (NdotL, 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		colorSum += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"	colorSum *= matColor * gl_LightSource [0].constantAttenuation;\r\n" \
	"	gl_FragColor = vec4 (min (matColor.rgb, colorSum.rgb) * fLightScale, matColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	vec3 lightNorm = lightVec / lightDist;\r\n" \
	"  /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"  float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"	float lightAngle = /*bDirected **/ ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"	float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"	float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	  if (dist == 0.0)\r\n" \
	"		colorSum += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		/*specular highlight >>>>>*/\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"			colorSum += (gl_LightSource [0].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}/*<<<<< specular highlight*/\r\n" \
	"		NdotL = max (NdotL, 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		colorSum += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"  colorSum *= texColor * gl_LightSource [0].constantAttenuation;\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy) ;\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	vec3 lightNorm = lightVec / lightDist;\r\n" \
	"  \r\n" \
	"  float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"	float lightAngle = /*bDirected **/ ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"	float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"	float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	  if (dist == 0.0)\r\n" \
	"		colorSum += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		/*specular highlight >>>>>*/\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"			colorSum += (gl_LightSource [0].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}/*<<<<< specular highlight*/\r\n" \
	"		NdotL = max (NdotL, 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		colorSum += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"  colorSum *= texColor * gl_LightSource [0].constantAttenuation;\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex, maskTex;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nType;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"if ((nType == 3) && (texture2D (maskTex, gl_TexCoord [3].xy).r < 0.5))\r\n" \
	"  discard;\r\n" \
	"else {\r\n" \
	"	vec4 colorSum = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = (nType == 0) ? vec4 (0.0, 0.0, 0.0, 0.0) : texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = (nType == 1) ? vec4 (0.0, 0.0, 0.0, 0.0) : texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	vec3 lightNorm = lightVec / lightDist;\r\n" \
	"  /*float bDirected = length (gl_LightSource [i].spotDirection);*/\r\n" \
	"  float NdotL = (/*bDirected **/ lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"	float lightAngle = /*bDirected **/ ((lightDist > 0.01) ? -dot (lightNorm, gl_LightSource [i].spotDirection) + 0.01 : 1.0);\r\n" \
	"	float lightRad = gl_LightSource [i].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	lightDist -= 100.0 * /*bDirected **/ min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"	float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	  if (dist == 0.0)\r\n" \
	"		colorSum += gl_LightSource [0].diffuse + gl_LightSource [0].ambient;\r\n" \
	"	else {\r\n" \
	"		att += gl_LightSource [0].linearAttenuation * dist + gl_LightSource [0].quadraticAttenuation * dist * dist;\r\n" \
	"		/*specular highlight >>>>>*/\r\n" \
	"		if (NdotL >= 0.0) {\r\n" \
	"			vec3 halfV = normalize (gl_LightSource [0].halfVector.xyz);\r\n" \
	"			float NdotHV = max (dot (vertNorm, halfV), 0.0);\r\n" \
	"			colorSum += (gl_LightSource [0].specular * pow (NdotHV, 16.0)) / att;\r\n" \
	"			}/*<<<<< specular highlight*/\r\n" \
	"		NdotL = max (NdotL, 0.0);\r\n" \
	"		if (lightRad > 0.0)\r\n" \
	"			NdotL += (1.0 - NdotL) / att;\r\n" \
	"		colorSum += (gl_LightSource [0].diffuse * NdotL + gl_LightSource [0].ambient) / att;\r\n" \
	"		}\r\n" \
	"  colorSum *= texColor * gl_LightSource [0].constantAttenuation;\r\n" \
	"	gl_FragColor = vec4 (min (texColor.rgb, colorSum.rgb) * fLightScale, texColor.a * gl_Color.a);\r\n" \
	"	}"
	};

// ----------------------------------------------------------------------------------------------
// per pixel lighting with lightmaps, 1 light source per pass

const char *pszPPLMLightingVS [] = {
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

const char *pszPP0LMLightingFS [] = {
#if ONLY_LIGHTMAPS
	"uniform sampler2D lMapTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"void main() {\r\n" \
	"gl_FragColor = texture2D (lMapTex, gl_TexCoord [0].xy) * fLightScale;\r\n" \
	"}"
#else
	"uniform sampler2D lMapTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float fLightScale;\r\n" \
	"uniform int nLights;\r\n" \
	"void main() {\r\n" \
	"vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy) * fLightScale;\r\n" \
	"gl_FragColor = /*min (texColor,*/ vec4 (min (matColor.rgb, matColor.rgb * color.rgb), matColor.a * gl_Color.a);\r\n" \
	"}"
#endif
	,
	"uniform sampler2D lMapTex, baseTex;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"	gl_FragColor = vec4 (texColor.rgb * color.rgb, texColor.a * gl_Color.a);\r\n" \
	"	}"
	,
	"uniform sampler2D lMapTex, baseTex, decalTex;\r\n" \
	"void main() {\r\n" \
	"	vec4 color = texture2D (lMapTex, gl_TexCoord [0].xy);\r\n" \
	"	vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"  vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"	texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	gl_FragColor = vec4 (texColor.rgb * color.rgb, texColor.a * gl_Color.a);\r\n" \
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
	"	gl_FragColor = vec4 (texColor.rgb * color.rgb, texColor.a * gl_Color.a);\r\n" \
	"	}\r\n" \
	"}"
	};

//-------------------------------------------------------------------------

int SetIndent (int nIndent);

char *BuildLightingShader (const char *pszTemplate, int nLights)
{
	int	l = (int) strlen (pszTemplate) + 1;
	char	*pszFS, szLights [2];

if (!(pszFS = new char [l]))
	return NULL;
if (nLights > MAX_LIGHTS_PER_PIXEL)
	nLights = MAX_LIGHTS_PER_PIXEL;
memcpy (pszFS, pszTemplate, l);
if (strstr (pszFS, "#define LIGHTS ") == pszFS) {
	sprintf (szLights, "%d", nLights);
	pszFS [15] = *szLights;
	}
#if DBG
int nIndent = SetIndent (0);
PrintLog (0, "\n\nShader program:\n");
PrintLog (0, pszFS);
PrintLog (0, "\n");
SetIndent (nIndent);
#endif
return pszFS;
}

//-------------------------------------------------------------------------

int perPixelLightingShaderProgs [9][4] = {
	{-1,-1,-1,-1},
	{-1,-1,-1,-1},
	{-1,-1,-1,-1},
	{-1,-1,-1,-1},
	{-1,-1,-1,-1},
	{-1,-1,-1,-1},
	{-1,-1,-1,-1},
	{-1,-1,-1,-1},
	{-1,-1,-1,-1}
};

int CreatePerPixelLightingShader (int nType, int nLights)
{
	int	h, i, j, bOk;
	char	*pszFS, *pszVS;
	const char	**fsP, **vsP;

if (!(ogl.m_features.bShaders && gameStates.render.bUsePerPixelLighting && (ogl.m_features.bPerPixelLighting == 2)))
	ogl.m_features.bPerPixelLighting =
	gameStates.render.bPerPixelLighting = 0;
if (!gameStates.render.bPerPixelLighting)
	return -1;
#if CONST_LIGHT_COUNT
nLights = gameStates.render.nMaxLightsPerPass;
#endif
if (perPixelLightingShaderProgs [nLights][nType] >= 0)
	return nLights;
for (h = 0; h <= 3; h++) {
#if CONST_LIGHT_COUNT
	i = gameStates.render.nMaxLightsPerPass;
#else
	for (i = 0; i <= gameStates.render.nMaxLightsPerPass; i++)
#endif
	 {
		if (perPixelLightingShaderProgs [i][h] >= 0)
			continue;
		if (lightmapManager.HaveLightmaps ()) {
			if (i) {
				fsP = (i == 1) ? pszPP1LMLightingFS : pszPPXLMLightingFS;
				vsP = pszPPLMLightingVS;
				}
			else {
				fsP = pszPP0LMLightingFS;
				vsP = pszPPLMLightingVS;
				}
			}
		else {
			if (i) {
				fsP = (i == 1) ? pszPP1LightingFS : pszPPXLightingFS;
				vsP = pszPPLightingVS;
				}
			else {
				fsP = pszLightingFS;
				vsP = pszLightingVS;
				}
			}
		PrintLog (0, "building lighting shader programs\n");
		pszFS = BuildLightingShader (fsP [h], i);
		pszVS = BuildLightingShader (vsP [h], i);
		bOk = (pszFS != NULL) && (pszVS != NULL) && shaderManager.Build (perPixelLightingShaderProgs [i][h], pszFS, pszVS);
		delete[] pszFS;
		delete[] pszVS;
		if (!bOk) {
			ogl.m_features.bPerPixelLighting = 1;
			gameStates.render.bPerPixelLighting = 1;
			for (i = 0; i <= MAX_LIGHTS_PER_PIXEL; i++)
				for (j = 0; j < 4; j++)
					shaderManager.Delete (perPixelLightingShaderProgs [i][j]);
			nLights = 0;
			return -1;
			}
		}
	}
return ogl.m_data.nPerPixelLights [nType] = nLights;
}

// ----------------------------------------------------------------------------------------------

void ResetPerPixelLightingShaders (void)
{
//memset (perPixelLightingShaderProgs, 0xFF, sizeof (perPixelLightingShaderProgs));
}

// ----------------------------------------------------------------------------------------------

void InitPerPixelLightingShaders (void)
{
#if CONST_LIGHT_COUNT
for (int nType = 0; nType < 4; nType++)
	CreatePerPixelLightingShader (nType, gameStates.render.nMaxLightsPerPass);
#else
for (int nType = 0; nType < 4; nType++)
	for (int nLights = 0; nLights <= gameStates.render.nMaxLightsPerPass; nLights++)
		CreatePerPixelLightingShader (nType, nLights);
#endif
}

//------------------------------------------------------------------------------

#if DBG
int CheckUsedLights2 (void);
#endif

int SetupHardwareLighting (CSegFace *faceP, int nType)
{
PROF_START
	int					nLightRange, nLights;
	float					fBrightness;
	CFloatVector			ambient, diffuse;
#if 0
	CFloatVector			black = {0,0,0,0};
#endif
	CFloatVector			specular = {0.5f,0.5f,0.5f,0.5f};
	//CFloatVector			vPos = CFloatVector::Create(0,0,0,1);
	GLenum				hLight;
	CActiveDynLight*	activeLightsP;
	CDynLight*			psl;
	CDynLightIndex&	sli = lightManager.Index (0,0);

#if DBG
if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if (faceP - FACES.faces == nDbgFace)
	nDbgFace = nDbgFace;
#endif
if (!ogl.m_states.iLight) {
#if DBG
	if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
	if (faceP - FACES.faces == nDbgFace)
		nDbgFace = nDbgFace;
#endif
#if ONLY_LIGHTMAPS == 2
	ogl.m_states.nLights = 0;
#else
	ogl.m_states.nLights = lightManager.SetNearestToFace (faceP, nType != 0);
#endif
	if (ogl.m_states.nLights > gameStates.render.nMaxLightsPerFace)
		ogl.m_states.nLights = gameStates.render.nMaxLightsPerFace;
	ogl.m_states.nFirstLight = sli.nFirst;
	gameData.render.nTotalLights += ogl.m_states.nLights;
	if (gameData.render.nMaxLights < ogl.m_states.nLights)
		gameData.render.nMaxLights = ogl.m_states.nLights;
#if DBG
	if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
		HUDMessage (0, "%d lights", ogl.m_states.nLights);
#endif
	}
activeLightsP = lightManager.Active (0) + ogl.m_states.nFirstLight;
nLightRange = sli.nLast - ogl.m_states.nFirstLight + 1;
for (nLights = 0;
	  (ogl.m_states.iLight < ogl.m_states.nLights) & (nLightRange > 0) && (nLights < gameStates.render.nMaxLightsPerPass);
	  activeLightsP++, nLightRange--) {
	if (!(psl = lightManager.GetActive (activeLightsP, 0)))
		continue;
#if 0 //DBG
	if (psl->info.nObject < 1)
		continue;
	if (OBJECTS [psl->info.nObject].info.nType != OBJ_LIGHT)
		continue;
#endif
#if 0//DBG
	if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
	if (faceP - FACES == nDbgFace)
		nDbgFace = nDbgFace;
	if ((psl->nTarget < 0) && (-psl->nTarget - 1 != faceP->m_info.nSegment))
		faceP = faceP;
	else if ((psl->nTarget > 0) && (psl->nTarget != faceP - FACES + 1))
		faceP = faceP;
	if (!psl->nTarget)
		psl = psl;
#endif
	if (psl->render.bUsed [0] == 2) {//nearest vertex light
		lightManager.ResetUsed (psl, 0);
		sli.nActive--;
		}
	hLight = GL_LIGHT0 + nLights++;
	//glEnable (hLight);
	specular.Alpha () = (psl->info.nSegment >= 0) ? psl->info.fRad : psl->info.fRad * psl->info.fBoost; //krasser Missbrauch!
	fBrightness = psl->info.fBrightness;
#if 0//DBG
	if ((psl->info.nObject >= 0) && (OBJECTS [psl->info.nObject].nType == nDbgObjType) &&
		 ((nDbgObjId < 0) || (OBJECTS [psl->info.nObject].id == nDbgObjId)))
		nDbgObjType = nDbgObjType;
#endif
	if (psl->info.nType == 2) {
		glLightf (hLight, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf (hLight, GL_LINEAR_ATTENUATION, OBJ_LIN_ATT / fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, OBJ_QUAD_ATT / fBrightness);
#if 1
		glLightfv (hLight, GL_SPOT_DIRECTION, (GLfloat*) (&CFloatVector3::ZERO));
#else
		if (!faceP)
			glLightfv (hLight, GL_SPOT_DIRECTION, (GLfloat*) (&CFloatVector3::ZERO));
		else {
			CFloatVector vNormal = -(faceP->Normal ());
			glLightfv (hLight, GL_SPOT_DIRECTION, reinterpret_cast<GLfloat*> (&vNormal));
			}
#endif
		ambient.Red () = psl->info.color.Red () * PPL_AMBIENT_LIGHT;
		ambient.Green () = psl->info.color.Green () * PPL_AMBIENT_LIGHT;
		ambient.Blue () = psl->info.color.Blue () * PPL_AMBIENT_LIGHT;
		ambient.Alpha () = 1.0f;
		diffuse.Red () = psl->info.color.Red () * PPL_DIFFUSE_LIGHT;
		diffuse.Green () = psl->info.color.Green () * PPL_DIFFUSE_LIGHT;
		diffuse.Blue () = psl->info.color.Blue () * PPL_DIFFUSE_LIGHT;
		diffuse.Alpha () = 1.0f;
		}
	else {
		glLightf (hLight, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf (hLight, GL_LINEAR_ATTENUATION, GEO_LIN_ATT / fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, GEO_QUAD_ATT / fBrightness);
		glLightfv (hLight, GL_SPOT_DIRECTION, reinterpret_cast<GLfloat*> (&psl->info.vDirf));
		ambient = psl->info.color * PPL_AMBIENT_LIGHT;
		ambient.Alpha () = 1.0f;
		fBrightness = min (fBrightness, 1.0f) * PPL_DIFFUSE_LIGHT;
		diffuse = psl->info.color * fBrightness;
		diffuse.Alpha () = 1.0f;
		}
	glLightfv (hLight, GL_DIFFUSE, reinterpret_cast<GLfloat*> (&diffuse));
	glLightfv (hLight, GL_SPECULAR, reinterpret_cast<GLfloat*> (&specular));
	glLightfv (hLight, GL_AMBIENT, reinterpret_cast<GLfloat*> (&ambient));
	glLightfv (hLight, GL_POSITION, reinterpret_cast<GLfloat*> (psl->render.vPosf));
	ogl.m_states.iLight++;
	}
if (nLightRange <= 0) {
	ogl.m_states.iLight = ogl.m_states.nLights;
	}
ogl.m_states.nFirstLight = int (activeLightsP - lightManager.Active (0));
#if DBG
if ((ogl.m_states.iLight < ogl.m_states.nLights) && !nLightRange)
	nDbgSeg = nDbgSeg;
#endif
ogl.ClearError (0);
PROF_END(ptPerPixelLighting)
return nLights;
}

//------------------------------------------------------------------------------

int SetupPerPixelLightingShader (CSegFace *faceP, int nType, bool bHeadlight)
{
PROF_START
	//static CBitmap	*nullBmP = NULL;
	static int nLastType = -1;

#if DBG
if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

	int	nLights = SetupHardwareLighting (faceP, nType);//, bStart = (ogl.m_states.iLight == 0);
	
if (0 > nLights)
	return 0;
#if ONLY_LIGHTMAPS == 2
nType = 0;
#endif
#if DBG
if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (!SetupLightmap (faceP))
	return 0;
GLhandleARB shaderProg = GLhandleARB (shaderManager.Deploy (perPixelLightingShaderProgs [gameStates.render.nMaxLightsPerPass][3/*nType*/]));
if (!shaderProg) {
	PROF_END(ptShaderStates);
	return -1;
	}

if (shaderManager.Rebuild (shaderProg) || (nType != nLastType));
	{
	nLastType = nType;
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
if (!nType)
	glUniform4fv (glGetUniformLocation (shaderProg, "matColor"), 1, reinterpret_cast<GLfloat*> (&faceP->m_info.color));
glUniform1i (glGetUniformLocation (shaderProg, "nType"), GLint (nType));
glUniform1i (glGetUniformLocation (shaderProg, "nLights"), GLint (nLights));
glUniform1f (glGetUniformLocation (shaderProg, "fLightScale"), (nLights ? GLfloat (nLights) / GLfloat (ogl.m_states.nLights) : 1.0f) * gameData.render.fBrightness);
ogl.ClearError (0);
PROF_END(ptShaderStates)
return perPixelLightingShaderProgs [gameStates.render.nMaxLightsPerPass][nType];
}

// ----------------------------------------------------------------------------------------------
//eof
