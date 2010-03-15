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

//-------------------------------------------------------------------------
// per pixel lighting
// 2 - 8 light sources

const char *multipleLightFS = {
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D lMapTex;\r\n" \
	"uniform float fScale;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = (texture2D (lMapTex, gl_TexCoord [0].xy) + gl_Color) / fScale;\r\n" \
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
	"	   lightDist -= 100.0 * min (0.0, min (NdotL, lightAngle));\r\n" \
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
	"				NdotL += (1.0 - NdotL) / sqrt (att);\r\n" \
	"			color = (gl_LightSource [i].diffuse * NdotL + gl_LightSource [i].ambient) / att;\r\n" \
	"			}\r\n" \
	"		colorSum += color * gl_LightSource [i].constantAttenuation;\r\n" \
	"		}\r\n" \
	"	gl_FragColor = colorSum;\r\n" \
	"	}"
	};

//-------------------------------------------------------------------------
// one light source

const char *singleLightFS = {
	"uniform sampler2D lMapTex;\r\n" \
	"uniform int nLights;\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"	vec4 colorSum = (texture2D (lMapTex, gl_TexCoord [0].xy) + gl_Color) / fScale;\r\n" \
	"	vec3 vertNorm = normalize (normal);\r\n" \
	"	vec3 lightVec = vec3 (gl_LightSource [0].position) - vertPos;\r\n" \
	"	float lightDist = length (lightVec);\r\n" \
	"	vec3 lightNorm = lightVec / lightDist;\r\n" \
	"  float NdotL = (lightDist > 0.1) ? dot (lightNorm, vertNorm) : 1.0;\r\n" \
	"	float lightAngle = min (NdotL, -dot (lightNorm, gl_LightSource [0].spotDirection));\r\n" \
	"	float lightRad = gl_LightSource [0].specular.a * (1.0 - abs (lightAngle));\r\n" \
	"	lightDist -= 100.0 * min (0.0, min (NdotL, lightAngle)); //bDirected\r\n" \
	"	float att = 1.0, dist = max (lightDist - lightRad, 0.0);\r\n" \
	"	if (dist == 0.0)\r\n" \
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
	"	gl_FragColor = colorSum;\r\n" \
	"	}"
	};

//-------------------------------------------------------------------------

const char *lightVS = {
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main() {\r\n" \
	"  normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"	vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"  gl_TexCoord [0] = gl_MultiTexCoord0;"\
	"	gl_Position = ftransform();\r\n" \
   "	gl_FrontColor = gl_Color;\r\n" \
	"	}"
	};

//-------------------------------------------------------------------------

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
PrintLog (" \n\nShader program:\n");
PrintLog (pszFS);
PrintLog (" \n");
#endif
return pszFS;
}

//-------------------------------------------------------------------------

int lightShaderProgs [9] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};

int CreatePerPixelLightingShader (int nLights)
{
	int	bOk;
	char	*pszFS, *pszVS;
	const char	*fsP, *vsP;

if (!gameStates.render.bPerPixelLighting)
	return -1;
if (!(ogl.m_states.bShadersOk && gameStates.render.bUsePerPixelLighting && (ogl.m_states.bPerPixelLightingOk == 2))) {
	ogl.m_states.bPerPixelLightingOk =
	gameStates.render.bPerPixelLighting = 0;
	return -1;
	}
if (lightShaderProgs [nLights] >= 0)
	return nLights;
fsP = (nLights == 1) ? singleLightFS : multipleLightFS;
vsP = lightVS;
PrintLog ("building lighting shader programs\n");
pszFS = BuildLightingShader (fsP, nLights);
pszVS = BuildLightingShader (vsP, nLights);
bOk = (pszFS != NULL) && (pszVS != NULL) && shaderManager.Build (lightShaderProgs [nLights], pszFS, pszVS);
delete[] pszFS;
delete[] pszVS;
if (!bOk) {
	ogl.m_states.bPerPixelLightingOk = 1;
	gameStates.render.bPerPixelLighting = 1;
	for (int i = 0; i <= MAX_LIGHTS_PER_PIXEL; i++)
		shaderManager.Delete (lightShaderProgs [i]);
	nLights = 0;
	return -1;
	}
return ogl.m_data.nPerPixelLights [nLights] = nLights;
}

//------------------------------------------------------------------------------

void ResetPerPixelLightingShaders (void)
{
//memset (lightShaderProgs, 0xFF, sizeof (lightShaderProgs));
}

//------------------------------------------------------------------------------

void InitPerPixelLightingShaders (void)
{
for (int nLights = 1; nLights <= gameStates.render.nMaxLightsPerPass; nLights++)
	CreatePerPixelLightingShader (nLights);
}

//------------------------------------------------------------------------------

#if DBG
int CheckUsedLights2 (void);
#endif

int SetupHardwareLighting (CSegFace *faceP)
{
PROF_START
	int					nLightRange, nLights;
	float					fBrightness;
	tRgbaColorf			ambient, diffuse;
#if 0
	tRgbaColorf			black = {0,0,0,0};
#endif
	tRgbaColorf			specular = {0.5f,0.5f,0.5f,0.5f};
	GLenum				hLight;
	CActiveDynLight*	activeLightsP;
	CDynLight*			psl;
	CDynLightIndex*	sliP = &lightManager.Index (0)[0];

#if DBG
if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
if (faceP - FACES.faces == nDbgFace)
	nDbgFace = nDbgFace;
#endif

if (!ogl.m_states.iLight) {
	if (!(ogl.m_states.nLights = lightManager.SetNearestToFace (faceP)))
		return 0;

	if (ogl.m_states.nLights > gameStates.render.nMaxLightsPerFace)
		ogl.m_states.nLights = gameStates.render.nMaxLightsPerFace;
	ogl.m_states.nFirstLight = sliP->nFirst;
	gameData.render.nTotalLights += ogl.m_states.nLights;
	if (gameData.render.nMaxLights < ogl.m_states.nLights)
		gameData.render.nMaxLights = ogl.m_states.nLights;
	}

activeLightsP = lightManager.Active (0) + ogl.m_states.nFirstLight;
nLightRange = sliP->nLast - ogl.m_states.nFirstLight + 1;
for (nLights = 0;
	  (ogl.m_states.iLight < ogl.m_states.nLights) & (nLightRange > 0) && (nLights < gameStates.render.nMaxLightsPerPass);
	  activeLightsP++, nLightRange--) {
	if (!(psl = lightManager.GetActive (activeLightsP, 0)))
		continue;
	if (psl->render.bUsed [0] == 2) {//nearest vertex light
		lightManager.ResetUsed (psl, 0);
		sliP->nActive--;
		}
	hLight = GL_LIGHT0 + nLights++;
	specular.alpha = (psl->info.nSegment >= 0) ? psl->info.fRad : psl->info.fRad * psl->info.fBoost; //krasser Missbrauch!
	fBrightness = psl->info.fBrightness;
	if (psl->info.nType == 2) {
		glLightf (hLight, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf (hLight, GL_LINEAR_ATTENUATION, OBJ_LIN_ATT / fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, OBJ_QUAD_ATT / fBrightness);
		glLightfv (hLight, GL_SPOT_DIRECTION, (GLfloat*) (&CFloatVector3::ZERO));
		ambient.red = psl->info.color.red * PPL_AMBIENT_LIGHT;
		ambient.green = psl->info.color.green * PPL_AMBIENT_LIGHT;
		ambient.blue = psl->info.color.blue * PPL_AMBIENT_LIGHT;
		ambient.alpha = 1.0f;
		diffuse.red = psl->info.color.red * PPL_DIFFUSE_LIGHT;
		diffuse.green = psl->info.color.green * PPL_DIFFUSE_LIGHT;
		diffuse.blue = psl->info.color.blue * PPL_DIFFUSE_LIGHT;
		diffuse.alpha = 1.0f;
		}
	else {
		glLightf (hLight, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf (hLight, GL_LINEAR_ATTENUATION, GEO_LIN_ATT / fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, GEO_QUAD_ATT / fBrightness);
		glLightfv (hLight, GL_SPOT_DIRECTION, reinterpret_cast<GLfloat*> (&psl->info.vDirf));
		ambient.red = psl->info.color.red * PPL_AMBIENT_LIGHT;
		ambient.green = psl->info.color.green * PPL_AMBIENT_LIGHT;
		ambient.blue = psl->info.color.blue * PPL_AMBIENT_LIGHT;
		ambient.alpha = 1.0f;
		fBrightness = min (fBrightness, 1.0f) * PPL_DIFFUSE_LIGHT;
		diffuse.red = psl->info.color.red * fBrightness;
		diffuse.green = psl->info.color.green * fBrightness;
		diffuse.blue = psl->info.color.blue * fBrightness;
		diffuse.alpha = 1.0f;
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
PROF_END(ptPerPixelLighting)
return nLights;
}

//------------------------------------------------------------------------------

int LoadPerPixelLightingShader (void)
{
PROF_START
gameStates.render.shaderProg = GLhandleARB (shaderManager.Deploy (lightShaderProgs [gameStates.render.nMaxLightsPerPass]));
if (!gameStates.render.shaderProg) {
	PROF_END(ptShaderStates);
	return -1;
	}
glUniform1i (glGetUniformLocation (gameStates.render.shaderProg, "lMapTex"), 0);
gameStates.render.nLights = -1;
PROF_END(ptShaderStates);
return lightShaderProgs [gameStates.render.nMaxLightsPerPass];
}

//------------------------------------------------------------------------------

int SetupPerPixelLightingShader (CSegFace *faceP)
{
PROF_START
#if DBG
if (faceP && (faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

int nLights = SetupHardwareLighting (faceP);
if (0 >= nLights) {
	PROF_END(ptShaderStates)
	return 0;
	}
glUniform1i (glGetUniformLocation (gameStates.render.shaderProg, "lMapTex"), 0);
glUniform1i (glGetUniformLocation (gameStates.render.shaderProg, "nLights"), GLint (nLights));
glUniform1f (glGetUniformLocation (gameStates.render.shaderProg, "fScale"),  
				 float ((nLights + gameStates.render.nMaxLightsPerPass - 1) / gameStates.render.nMaxLightsPerPass));
ogl.ClearError (0);
PROF_END(ptShaderStates)
return lightShaderProgs [gameStates.render.nMaxLightsPerPass];
}

// ----------------------------------------------------------------------------------------------
//eof
