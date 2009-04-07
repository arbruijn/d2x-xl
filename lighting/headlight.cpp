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
#include <conf.h>
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>	// for memset ()

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "fix.h"
#include "vecmat.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "rendermine.h"
#include "dynlight.h"
#include "lightmap.h"
#include "headlight.h"
#include "automap.h"

//------------------------------------------------------------------------------

void CHeadlightManager::Init (void)
{
memset (this, 0, sizeof (*this)); 
memset (lightIds, 0xff, sizeof (lightIds));
}

//------------------------------------------------------------------------------

void CHeadlightManager::Toggle (void)
{
if (PlayerHasHeadlight (-1)) {
	LOCALPLAYER.flags ^= PLAYER_FLAGS_HEADLIGHT_ON;
	if (IsMultiGame)
		MultiSendFlags ((char) gameData.multiplayer.nLocalPlayer);
	}
}

//------------------------------------------------------------------------------

//	Flag array of OBJECTS lit last frame.  Guaranteed to process this frame if lit last frame.
fix	xBeamBrightness = (I2X (1)/2);	//global saying how bright the light beam is

fix CHeadlightManager::ComputeLightOnObject (CObject *objP)
{
	//	Let's just illuminate players and robots for speed reasons, ok?
if ((objP->info.nType != OBJ_ROBOT) && (objP->info.nType	!= OBJ_PLAYER))
	return 0;

	fix			dot, dist, light = 0;
	CFixVector	vecToObj;
	CObject*		lightObjP;

for (int nPlayer = 0; nPlayer < MAX_PLAYERS; nPlayer++) {
	if ((lightObjP = objects [nPlayer])) {
		vecToObj = objP->info.position.vPos - lightObjP->info.position.vPos;
		dist = CFixVector::Normalize (vecToObj);
		if (dist > 0) {
			dot = CFixVector::Dot (lightObjP->info.position.mOrient.FVec (), vecToObj);
			if (dot < I2X (1)/2)
				light += FixDiv (HEADLIGHT_SCALE, FixMul (HEADLIGHT_SCALE, dist));	//	Do the Normal thing, but darken around headlight.
			else
				light += FixMul (FixMul (dot, dot), HEADLIGHT_SCALE)/8;
			}
		}
	}
return light;
}

//------------------------------------------------------------------------------
// A note about transforming the headlight direction:
// To achive that, the direction is added to the original position and transformed,
// and the transformed headlight position is subtracted from that.

void CHeadlightManager::Transform (void)
{
if (automap.m_bDisplay)
	return;

	CDynLight*	pl;
	bool			bHWHeadlight = (gameStates.render.bPerPixelLighting == 2) || (gameStates.ogl.bHeadlight && gameOpts->ogl.bHeadlight);

for (int i = 0; i < MAX_PLAYERS; i++) {
	if (lightIds [i] >= 0) {
		pl = lightManager.Lights () + lightIds [i];
		pl->info.bSpot = 1;
		pl->info.vDirf.Assign (pl->vDir);
		if (bHWHeadlight) {
			pos [i] = pl->render.vPosf [0];
			dir [i] = *pl->info.vDirf.XYZ ();
			brightness [i] = 100.0f;
			}
		else if (pl->bTransform && !gameStates.ogl.bUseTransform)
			transformation.Rotate (pl->info.vDirf, pl->info.vDirf, 0);

		}
	}
}

//------------------------------------------------------------------------------
// A note about transforming the headlight direction:
// To achive that, the direction is added to the original position and transformed,
// and the transformed headlight position is subtracted from that.

void CHeadlightManager::Prepare (void)
{
nLights = 0;
for (int i = 0; i < MAX_PLAYERS; i++)
	if (lightIds [i] >= 0)
		lights [nLights++] = lightManager.Lights () + lightIds [i];
}

//------------------------------------------------------------------------------

int CHeadlightManager::Add (CObject *objP)
{
#if 0
	static float spotExps [] = {12.0f, 5.0f, 0.0f};
	static float spotAngles [] = {0.9f, 0.5f, 0.5f};
#endif

objects [objP->info.nId] = objP;
if (gameStates.render.nLightingMethod) {
	if (lightIds [objP->info.nId] < 0) {
			static tRgbaColorf c = {1.0f, 1.0f, 1.0f, 1.0f};
			int nLight = lightManager.Add (NULL, &c, I2X (200), -1, -1, -1, -1, NULL);

		if (0 <= nLight) {
			CDynLight* pl = lightManager.Lights () + nLight;
			pl->info.nPlayer = (objP->info.nType == OBJ_PLAYER) ? objP->info.nId : 1;
			pl->info.fRad = 0;
			pl->info.bSpot = 1;
			pl->info.fSpotAngle = 0.9f; //spotAngles [extraGameInfo [IsMultiGame].nSpotSize];
			pl->info.fSpotExponent = 12.0f; //spotExps [extraGameInfo [IsMultiGame].nSpotStrength];
			pl->bTransform = 0;
			lightIds [objP->info.nId] = nLight;
			}
		return lightIds [objP->info.nId];
		}
	}
return -1;
}

//------------------------------------------------------------------------------

void CHeadlightManager::Remove (CObject *objP)
{
objects [objP->info.nId] = NULL;
if (gameStates.render.nLightingMethod && (lightIds [objP->info.nId] >= 0)) {
	lightManager.Delete (lightIds [objP->info.nId]);
	lightIds [objP->info.nId] = -1;
	nLights--;
	}
}

//------------------------------------------------------------------------------

void CHeadlightManager::Update (void)
{
	CDynLight	*pl;
	CObject		*objP;
	short			nPlayer;

for (nPlayer = 0; nPlayer < MAX_PLAYERS; nPlayer++) {
	if (lightIds [nPlayer] >= 0) {
		pl = lightManager.Lights () + lightIds [nPlayer];
		objP = OBJECTS + gameData.multiplayer.players [nPlayer].nObject;
		pl->info.vPos = OBJPOS (objP)->vPos;
		pl->vDir = OBJPOS (objP)->mOrient.FVec ();
		pl->info.vPos += pl->vDir * (objP->info.xSize / 4);
		}
	}
}

//------------------------------------------------------------------------------

#if DBG_SHADERS


#else

const char *headlightFS [2][8] = {
 {
	//----------------------------------------
   //no lightmaps
	//single player version - one player
	//untextured
	"uniform vec4 matColor;\r\n" \
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"vec4 texColor = matColor;\r\n" \
	"vec3 spotColor = gl_Color.rgb;\r\n" \
	"vec3 lvNorm = normalize (lightVec);\r\n" \
	"if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"   float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"   if (spotEffect >= 0.15) {\r\n" \
	"	    float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	"      spotColor = max (vec3 (spotEffect, spotEffect, spotEffect), spotColor);\r\n" \
	"      }\r\n" \
	"   }\r\n" \
	"gl_FragColor = vec4 (min (texColor.rgb, texColor.rgb * spotColor), texColor.a);\r\n"  \
	"}"
	,
	//only base texture
	"uniform sampler2D baseTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"vec3 spotColor = gl_Color.rgb;\r\n" \
	"vec3 lvNorm = normalize (lightVec);\r\n" \
	"if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"   float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"   if (spotEffect >= 0.15) {\r\n" \
	"	    float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	"      spotColor = max (vec3 (spotEffect, spotEffect, spotEffect), spotColor);\r\n" \
	"      spotColor = min (spotColor, matColor.rgb);\r\n" \
	"	    }\r\n" \
	"	 }\r\n" \
	"gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n"  \
	"}"
	,
	//base texture and decal
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"vec3 spotColor = gl_Color.rgb;\r\n" \
	"vec3 lvNorm = normalize (lightVec);\r\n" \
	"if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"   float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"   if (spotEffect >= 0.15) {\r\n" \
	"      float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	"      spotColor = max (vec3 (spotEffect, spotEffect, spotEffect), spotColor);\r\n" \
	"      spotColor = min (spotColor, matColor.rgb);\r\n" \
	"	    }\r\n" \
	"	 }\r\n" \
	"gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n"  \
	"}"
	,
	//base texture and decal with color key
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.15)\r\n" \
	"   discard;\r\n" \
	"else {\r\n" \
	"   vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"   vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"   texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	 vec3 spotColor = gl_Color.rgb;\r\n" \
	"	 vec3 lvNorm = normalize (lightVec);\r\n" \
	"   float spotBrightness;\r\n" \
	"   if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.15) {\r\n" \
	" 		    float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"         spotEffect = pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	"         spotColor = max (vec3 (spotEffect, spotEffect, spotEffect), spotColor);\r\n" \
	"         spotColor = min (spotColor, matColor.rgb);\r\n" \
	"   	    }\r\n" \
	"   	 }\r\n" \
	"	 gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n"  \
	"   }\r\n" \
	"}"
	,
	// --------------------------------------------------------------------------------
	//multiplayer version - 1 - 8 players
	"#define LIGHTS 8\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 vertPos, normal;\r\n" \
	"void main (void) {\r\n" \
	"float spotBrightness = 0.0;\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < LIGHTS; i++) {\r\n" \
	"	 vec3 lightVec = vertPos - gl_LightSource [i].position.xyz;\r\n" \
	"	 float lightDist = length (lightVec);\r\n" \
	"	 vec3 lvNorm = lightVec / lightDist;\r\n" \
	"   if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [i].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.15) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	" 	      }\r\n" \
	" 	   }\r\n" \
	" 	}\r\n" \
	"vec3 spotColor = max (vec3 (spotBrightness, spotBrightness, spotBrightness), gl_Color.rgb);\r\n" \
	"spotColor = min (spotColor, matColor.rgb);\r\n" \
	"gl_FragColor = vec4 (matColor.rgb * spotColor, matColor.a);"  \
	"}"
	,
	//only base texture
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 vertPos, normal;\r\n" \
	"void main (void) {\r\n" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"float spotBrightness = 0.0, normLen = length (normal);\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < LIGHTS; i++) {\r\n" \
	"	 vec3 lightVec = vertPos - gl_LightSource [i].position.xyz;\r\n" \
	"	 float lightDist = length (lightVec);\r\n" \
	"	 vec3 lvNorm = lightVec / lightDist;\r\n" \
	"   if ((normLen == 0.0) || (dot (normalize (normal), lvNorm) < 0.0)) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [i].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.15) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	" 	      }\r\n" \
	" 	   }\r\n" \
	" 	}\r\n" \
	"vec3 spotColor = max (vec3 (spotBrightness, spotBrightness, spotBrightness), gl_Color.rgb);\r\n" \
	"spotColor = min (spotColor, matColor.rgb);\r\n" \
	"gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n" \
	"}"
	,
	//base texture and decal
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 vertPos, normal;\r\n" \
	"void main (void) {\r\n" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"float spotBrightness = 0.0;\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < LIGHTS; i++) {\r\n" \
	"	 vec3 lightVec = vertPos - gl_LightSource [i].position.xyz;\r\n" \
	"	 float lightDist = length (lightVec);\r\n" \
	"	 vec3 lvNorm = lightVec / lightDist;\r\n" \
	"   if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [i].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.15) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	" 	      }\r\n" \
	" 	   }\r\n" \
	" 	}\r\n" \
	"vec3 spotColor = max (vec3 (spotBrightness, spotBrightness, spotBrightness), gl_Color.rgb);\r\n" \
	"spotColor = min (spotColor, matColor.rgb);\r\n" \
	"gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n" \
	"}"
	,
	//base texture and decal with color key
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 vertPos, normal;\r\n" \
	"void main (void) {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.15)\r\n" \
	"   discard;\r\n" \
	"else {\r\n" \
	"   vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"   vec4 decalColor = texture2D (decalTex, gl_TexCoord [1].xy);\r\n" \
	"   texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	 float spotBrightness = 0.0;\r\n" \
	"	 int i;\r\n" \
	"	 for (i = 0; i < LIGHTS; i++) {\r\n" \
	"	    vec3 lightVec = vertPos - gl_LightSource [i].position.xyz;\r\n" \
	"	    float lightDist = length (lightVec);\r\n" \
	"	    vec3 lvNorm = lightVec / lightDist;\r\n" \
	"	    if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"	       float spotEffect = dot (gl_LightSource [i].spotDirection, lvNorm);\r\n" \
	"         if (spotEffect >= 0.15) {\r\n" \
	"            float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	          spotBrightness += pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	" 		       }\r\n" \
	" 		    }\r\n" \
	" 		 }\r\n" \
	"   vec3 spotColor = max (vec3 (spotBrightness, spotBrightness, spotBrightness), gl_Color.rgb);\r\n" \
	"   spotColor = min (spotColor, matColor.rgb);\r\n" \
	"	 gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n" \
	"   }\r\n" \
	"}"
	},
 {
	//----------------------------------------
   //lightmaps
	//single player version - one player
	//untextured
	"uniform vec4 matColor;\r\n" \
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"vec4 spotColor = vec4 (0.0, 0.0, 0.0, 0.0);\r\n" \
	"vec3 lvNorm = normalize (lightVec);\r\n" \
	"if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"   float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"   if (spotEffect >= 0.15) {\r\n" \
	"	    float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	"      spotColor = vec4 (spotEffect, spotEffect, spotEffect, 1.0);\r\n" \
	"      spotColor.rgb = min (spotColor.rgb, matColor.rgb);\r\n" \
	"      }\r\n" \
	"   }\r\n" \
	"gl_FragColor = matColor * spotColor;\r\n" \
	"}"
	,
	//only base texture
	"uniform sampler2D baseTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"vec4 spotColor = vec4 (0.0, 0.0, 0.0, 0.0);\r\n" \
	"vec3 lvNorm = normalize (lightVec);\r\n" \
	"if ((length (normal) == 0.0) || (dot (normalize (normal), lvNorm) < 0.0)) {\r\n" \
	"   float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"   if (spotEffect >= 0.15) {\r\n" \
	"      float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	"      spotColor = vec4 (spotEffect, spotEffect, spotEffect, 1.0);\r\n" \
	"      spotColor.rgb = min (spotColor.rgb, matColor.rgb);\r\n" \
	"	    }\r\n" \
	"	 }\r\n" \
	"gl_FragColor = texColor * spotColor;\r\n" \
	"}"
	,
	//base texture and decal
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"vec4 spotColor = vec4 (0.0, 0.0, 0.0, 0.0);\r\n" \
	"vec3 lvNorm = normalize (lightVec);\r\n" \
	"if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"   float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"   if (spotEffect >= 0.15) {\r\n" \
	"      float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	"      spotColor = vec4 (spotEffect, spotEffect, spotEffect, 1.0);\r\n" \
	"      spotColor.rgb = min (spotColor.rgb, matColor.rgb);\r\n" \
	"	    }\r\n" \
	"	 }\r\n" \
	"gl_FragColor = texColor * spotColor;\r\n" \
	"}"
	,
	//base texture and decal with color key
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [3].xy).r;\r\n" \
	"if (bMask < 0.15)\r\n" \
	"   discard;\r\n" \
	"else {\r\n" \
	"   vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"   vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"   texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	 vec4 spotColor = vec4 (0.0, 0.0, 0.0, 0.0);\r\n" \
	"	 vec3 lvNorm = normalize (lightVec);\r\n" \
	"   if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.15) {\r\n" \
	" 		    float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"         spotEffect = pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	"			 spotColor = vec4 (spotEffect, spotEffect, spotEffect, 1.0);\r\n" \
	"			 spotColor.rgb = min (spotColor.rgb, matColor.rgb);\r\n" \
	"			 }\r\n" \
	"		 }\r\n" \
	"	 gl_FragColor = texColor * spotColor;\r\n" \
	"   }\r\n" \
	"}"
	,
	// --------------------------------------------------------------------------------
	//multiplayer version - 1 - 8 players
	"#define LIGHTS 8\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 vertPos, normal;\r\n" \
	"void main (void) {\r\n" \
	"float spotBrightness = 0.0;\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < LIGHTS; i++) {\r\n" \
	"	 vec3 lightVec = vertPos - gl_LightSource [i].position.xyz;\r\n" \
	"	 float lightDist = length (lightVec);\r\n" \
	"	 vec3 lvNorm = lightVec / lightDist;\r\n" \
	"   if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [i].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.15) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow (spotEffect * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	" 	      }\r\n" \
	" 	   }\r\n" \
	" 	}\r\n" \
	"vec3 spotColor = min (vec3 (spotBrightness, spotBrightness, spotBrightness), matColor.rgb);\r\n" \
	"gl_FragColor = vec4 (matColor.rgb * spotColor, matColor.a);"  \
	"}"
	,
	//only base texture
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 vertPos, normal;\r\n" \
	"void main (void) {\r\n" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"float spotBrightness = 0.0, normLen = length (normal);\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < LIGHTS; i++) {\r\n" \
	"	 vec3 lightVec = vertPos - gl_LightSource [i].position.xyz;\r\n" \
	"	 float lightDist = length (lightVec);\r\n" \
	"	 vec3 lvNorm = lightVec / lightDist;\r\n" \
	"   if ((normLen == 0.0) || (dot (normalize (normal), lvNorm) < 0.0)) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [i].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.15) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow ((spotEffect + (1.0 - spotEffect) / 2) * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	" 	      }\r\n" \
	" 	   }\r\n" \
	" 	}\r\n" \
	"vec3 spotColor = min (vec3 (spotBrightness, spotBrightness, spotBrightness), matColor.rgb);\r\n" \
	"gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n" \
	"}"
	,
	//base texture and decal
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 vertPos, normal;\r\n" \
	"void main (void) {\r\n" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"float spotBrightness = 0.0;\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < LIGHTS; i++) {\r\n" \
	"	 vec3 lightVec = vertPos - gl_LightSource [i].position.xyz;\r\n" \
	"	 float lightDist = length (lightVec);\r\n" \
	"	 vec3 lvNorm = lightVec / lightDist;\r\n" \
	"   if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [i].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.15) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow ((spotEffect + (1.0 - spotEffect) / 2) * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	" 	      }\r\n" \
	" 	   }\r\n" \
	" 	}\r\n" \
	"vec3 spotColor = min (vec3 (spotBrightness, spotBrightness, spotBrightness), matColor.rgb);\r\n" \
	"gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n" \
	"}"
	,
	//base texture and decal with color key
	"#define LIGHTS 8\r\n" \
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"varying vec3 vertPos, normal;\r\n" \
	"void main (void) {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [3].xy).r;\r\n" \
	"if (bMask < 0.15)\r\n" \
	"   discard;\r\n" \
	"else {\r\n" \
	"   vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"   vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"   texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	 float spotBrightness = 0.0;\r\n" \
	"	 int i;\r\n" \
	"	 for (i = 0; i < LIGHTS; i++) {\r\n" \
	"	    vec3 lightVec = vertPos - gl_LightSource [i].position.xyz;\r\n" \
	"	    float lightDist = length (lightVec);\r\n" \
	"	    vec3 lvNorm = lightVec / lightDist;\r\n" \
	"	    if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"	       float spotEffect = dot (gl_LightSource [i].spotDirection, lvNorm);\r\n" \
	"         if (spotEffect >= 0.15) {\r\n" \
	"            float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	          spotBrightness += pow ((spotEffect + (1.0 - spotEffect) / 2) * 1.05, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	" 		       }\r\n" \
	" 		    }\r\n" \
	" 		 }\r\n" \
	"   vec3 spotColor = min (vec3 (spotBrightness, spotBrightness, spotBrightness), matColor.rgb);\r\n" \
	"   gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n" \
	"   }\r\n" \
	"}"
	}
	};

const char *headlightVS [2][8] = {
 {
	//no lightmaps
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"lightVec = vec3 (gl_ModelViewMatrix * gl_Vertex - gl_LightSource [0].position);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"lightVec = vec3 (gl_ModelViewMatrix * gl_Vertex - gl_LightSource [0].position);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"lightVec = vec3 (gl_ModelViewMatrix * gl_Vertex - gl_LightSource [0].position);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n"\
	"gl_TexCoord [1] = gl_MultiTexCoord1;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"lightVec = vec3 (gl_ModelViewMatrix * gl_Vertex - gl_LightSource [0].position);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n"\
	"gl_TexCoord [1] = gl_MultiTexCoord1;\r\n"\
	"gl_TexCoord [2] = gl_MultiTexCoord2;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main (void) {\r\n" \
	"vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main (void) {\r\n" \
	"vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main (void) {\r\n" \
	"vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n"\
	"gl_TexCoord [1] = gl_MultiTexCoord1;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main (void) {\r\n" \
	"vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [0] = gl_MultiTexCoord0;\r\n"\
	"gl_TexCoord [1] = gl_MultiTexCoord1;\r\n"\
	"gl_TexCoord [2] = gl_MultiTexCoord2;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	},
	//with lightmaps
	//single player
 {
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"lightVec = vec3 (gl_ModelViewMatrix * gl_Vertex - gl_LightSource [0].position);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"lightVec = vec3 (gl_ModelViewMatrix * gl_Vertex - gl_LightSource [0].position);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [1] = gl_MultiTexCoord1;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"lightVec = vec3 (gl_ModelViewMatrix * gl_Vertex - gl_LightSource [0].position);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [1] = gl_MultiTexCoord1;\r\n"\
	"gl_TexCoord [2] = gl_MultiTexCoord2;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"lightVec = vec3 (gl_ModelViewMatrix * gl_Vertex - gl_LightSource [0].position);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [1] = gl_MultiTexCoord1;\r\n"\
	"gl_TexCoord [2] = gl_MultiTexCoord2;\r\n"\
	"gl_TexCoord [3] = gl_MultiTexCoord3;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	//multiplayer
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main (void) {\r\n" \
	"vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main (void) {\r\n" \
	"vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [1] = gl_MultiTexCoord1;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main (void) {\r\n" \
	"vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [1] = gl_MultiTexCoord1;\r\n"\
	"gl_TexCoord [2] = gl_MultiTexCoord2;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	,
	"#define LIGHTS 8\r\n" \
	"varying vec3 normal, vertPos;\r\n" \
	"void main (void) {\r\n" \
	"vertPos = vec3 (gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"normal = normalize (gl_NormalMatrix * gl_Normal);\r\n" \
	"gl_TexCoord [1] = gl_MultiTexCoord1;\r\n"\
	"gl_TexCoord [2] = gl_MultiTexCoord2;\r\n"\
	"gl_TexCoord [3] = gl_MultiTexCoord3;\r\n"\
	"gl_Position = ftransform();\r\n" \
   "gl_FrontColor = gl_Color;}"
	}
	};

#endif

//-------------------------------------------------------------------------

GLhandleARB headlightShaderProgs [2][4] = {{0,0,0,0},{0,0,0,0}};
GLhandleARB lvs [2][4] = {{0,0,0,0},{0,0,0,0}};
GLhandleARB lfs [2][4] = {{0,0,0,0},{0,0,0,0}};

//-------------------------------------------------------------------------

void DeleteHeadlightShader (void)
{
for (int i = 0; i < 2; i++) {
	for (int j = 0; j < 4; j++) {
		if (headlightShaderProgs [i][j]) {
			DeleteShaderProg (&headlightShaderProgs [i][j]);
			headlightShaderProgs [i][j] = 0;
			}
		}
	}
}

//-------------------------------------------------------------------------

void InitHeadlightShaders (int nLights)
{
	int	h, i, j, bOk;
	char	*pszFS;

if (nLights < 0) {
	nLights = gameData.render.ogl.nHeadlights;
	gameData.render.ogl.nHeadlights = 0;
	}
if (nLights == gameData.render.ogl.nHeadlights)
	return;
gameStates.render.bHaveDynLights = 0;
PrintLog ("building lighting shader programs\n");
if ((gameStates.ogl.bHeadlight = (gameStates.ogl.bShadersOk))) {
	gameStates.render.bHaveDynLights = 1;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 4; j++) {
			if (headlightShaderProgs [i][j])
				DeleteShaderProg (&headlightShaderProgs [i][j]);
#if 1//ndef _DEBUG
			if (nLights == 1)
				pszFS = const_cast<char*> (headlightFS [i][h = j]);
			else
#endif
				pszFS = BuildLightingShader (headlightFS [i][h = j + 4], nLights);
			bOk = (pszFS != NULL) &&
					CreateShaderProg (&headlightShaderProgs [i][j]) &&
					CreateShaderFunc (&headlightShaderProgs [i][j], &lfs [i][j], &lvs [i][j], pszFS, headlightVS [i][h], 1) &&
					LinkShaderProg (&headlightShaderProgs [i][j]);
			if (pszFS && (nLights > 1))
				delete[] pszFS;
			if (!bOk) {
				DeleteHeadlightShader ();
				nLights = 0;
				break;
				}
			}
		}
	}
OglClearError (0);
gameData.render.ogl.nHeadlights = nLights;
}

//------------------------------------------------------------------------------

extern int nOglTransformCalls;

int CHeadlightManager::SetupShader (int nType, int bLightmaps, tRgbaColorf *colorP)
{
	int			h, i, nShader, bTransform;
	tRgbaColorf	color;

//headlights
h = IsMultiGame ? nLights : 1;
InitHeadlightShaders (h);
nShader = 10 + bLightmaps * 4 + nType;
if (nShader != gameStates.render.history.nShader) {
	//glUseProgramObject (0);
	gameData.render.nShaderChanges++;
	glUseProgramObject (activeShaderProg = headlightShaderProgs [bLightmaps][nType]);
	if (nType) {
		glUniform1i (glGetUniformLocation (activeShaderProg, "baseTex"), bLightmaps);
		if (nType > 1) {
			glUniform1i (glGetUniformLocation (activeShaderProg, "decalTex"), 1 + bLightmaps);
			if (nType > 2)
				glUniform1i (glGetUniformLocation (activeShaderProg, "maskTex"), 2 + bLightmaps);
			}
		}
	//glUniform1f (glGetUniformLocation (activeShaderProg, "aspect"), (float) screen.Width () / (float) screen.Height ());
	//glUniform1f (glGetUniformLocation (activeShaderProg, "zoom"), 65536.0f / (float) gameStates.render.xZoom);
	if ((bTransform = !nOglTransformCalls))
		OglSetupTransform (1);
	for (h = i = 0; i < MAX_PLAYERS; i++) {
		if (lightIds [i] >= 0) {
			glEnable (GL_LIGHT0 + h);
			glLightfv (GL_LIGHT0 + h, GL_POSITION, reinterpret_cast<GLfloat*> (pos + i));
			glLightfv (GL_LIGHT0 + h, GL_SPOT_DIRECTION, reinterpret_cast<GLfloat*> (dir + i));
			h++;
			}
		}
	if (bTransform)
		OglResetTransform (1);
	if (colorP) {
		color.red = colorP->red * 1.1f;
		color.green = colorP->green * 1.1f;
		color.blue = colorP->blue * 1.1f;
		color.alpha = colorP->alpha;
		}
	else {
		color.red = color.green = color.blue = 2.0f;
		color.alpha = 1;
		}
	glUniform4fv (glGetUniformLocation (activeShaderProg, "matColor"), 1, reinterpret_cast<GLfloat*> (&color));
	OglClearError (0);
	}
return gameStates.render.history.nShader = nShader;
}

//-----------------------------------------------------------------------------

int PlayerHasHeadlight (int nPlayer)
{
return EGI_FLAG (headlight.bAvailable, 0, 0, 0) &&
		 (EGI_FLAG (headlight.bBuiltIn, 0, 1, 0) ||
		  ((gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags & PLAYER_FLAGS_HEADLIGHT) != 0));
}

//-----------------------------------------------------------------------------

int HeadlightIsOn (int nPlayer)
{
#if DBG
if (!PlayerHasHeadlight (nPlayer))
	return 0;
if (!(gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON))
	return 0;
return 1;
#else
return PlayerHasHeadlight (nPlayer) && ((gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON) != 0);
#endif
}

//-----------------------------------------------------------------------------

void SetPlayerHeadlight (int nPlayer, int bOn)
{
if (bOn)
	gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags |= PLAYER_FLAGS_HEADLIGHT_ON;
else
	gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
}

//-----------------------------------------------------------------------------

void DrainHeadlightPower (void)
{
	static int bTurnedOff = 0;

if (!EGI_FLAG (headlight.bDrainPower, 0, 0, 1))
	return;
if (!HeadlightIsOn (-1))
	return;

LOCALPLAYER.energy -= (gameData.time.xFrame * 3 / 8);
if (LOCALPLAYER.energy < I2X (10)) {
	if (!bTurnedOff) {
		LOCALPLAYER.flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
		bTurnedOff = 1;
		if (IsMultiGame)
			MultiSendFlags ((char) gameData.multiplayer.nLocalPlayer);
		}
	}
else
	bTurnedOff = 0;
if (LOCALPLAYER.energy <= 0) {
	LOCALPLAYER.energy = 0;
	LOCALPLAYER.flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
	if (IsMultiGame)
		MultiSendFlags ((char) gameData.multiplayer.nLocalPlayer);
	}
}

// ----------------------------------------------------------------------------------------------
//eof
