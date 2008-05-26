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
#include "ogl_defs.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "render.h"
#include "dynlight.h"
#include "lightmap.h"
#include "headlight.h"

// ---------------------------------------------------------

void ToggleHeadLight ()
{
if (PlayerHasHeadLight (-1)) {
	LOCALPLAYER.flags ^= PLAYER_FLAGS_HEADLIGHT_ON;		
	if (IsMultiGame)
		MultiSendFlags ((char) gameData.multiplayer.nLocalPlayer);	
	}
}

// ---------------------------------------------------------

#define HEADLIGHT_BOOST_SCALE		8		//how much to scale light when have headlight boost
#define	MAX_HEADLIGHTS				8
#define MAX_DIST_LOG					6							//log (MAX_DIST-expressed-as-integer)
#define MAX_DIST						(f1_0<<MAX_DIST_LOG)	//no light beyond this dist
#define	HEADLIGHT_CONE_DOT		(F1_0*9/10)
#define	HEADLIGHT_SCALE			(F1_0*10)

//	Flag array of gameData.objs.objects lit last frame.  Guaranteed to process this frame if lit last frame.
tObject	*Headlights [MAX_HEADLIGHTS];
int		nHeadLights;
fix		xBeamBrightness = (F1_0/2);	//global saying how bright the light beam is

fix ComputeHeadlightLightOnObject (tObject *objP)
{
	int	i;
	fix	light;
	//	Let's just illuminate players and robots for speed reasons, ok?
	if ((objP->nType != OBJ_ROBOT) && (objP->nType	!= OBJ_PLAYER))
		return 0;
	light = 0;
	for (i = 0; i < nHeadLights; i++) {
		fix			dot, dist;
		vmsVector	vecToObj;
		tObject		*lightObjP;
		lightObjP = Headlights [i];
		VmVecSub (&vecToObj, &objP->position.vPos, &lightObjP->position.vPos);
		dist = VmVecNormalize (&vecToObj);
		if (dist > 0) {
			dot = VmVecDot (&lightObjP->position.mOrient.fVec, &vecToObj);
			if (dot < F1_0/2)
				light += FixDiv (HEADLIGHT_SCALE, FixMul (HEADLIGHT_SCALE, dist));	//	Do the normal thing, but darken around headlight.
			else
				light += FixMul (FixMul (dot, dot), HEADLIGHT_SCALE)/8;
		}
	}
	return light;
}

//------------------------------------------------------------------------------
// A note about transforming the headlight direction:
// To achive that, the direction is added to the original position and transformed,
// and the transformed headlight position is subtracted from that.

void TransformHeadLights (void)
{
	tDynLight		*pl;
	tShaderLight	*psl;
	int				i;

if (!gameData.render.lights.dynamic.headLights.nLights || gameStates.render.automap.bDisplay)
	return;
	// method 1: Emulate OpenGL's transformation
#if 0
G3PushMatrix ();
glPushMatrix ();
#endif
#if HEADLIGHT_TRANSFORMATION == 0
SetRenderView (gameStates.render.nEyeOffset, NULL, 0);
#endif
for (i = 0; i < gameData.render.lights.dynamic.headLights.nLights; i++) {
	pl = gameData.render.lights.dynamic.headLights.pl [i];
	psl = gameData.render.lights.dynamic.headLights.psl [i];
	VmVecFixToFloat (&psl->vDirf, &pl->vDir);
	if (pl->bTransform && !gameStates.ogl.bUseTransform)
		G3RotatePoint (&psl->vDirf, &psl->vDirf, 0);
	pl->info.bSpot = 1;
	psl->info.bSpot = 1;
	psl->info.fSpotAngle = pl->info.fSpotAngle;
	psl->info.fSpotExponent = pl->info.fSpotExponent;
	if (gameStates.ogl.bHeadLight && gameOpts->ogl.bHeadLight && !gameStates.render.automap.bDisplay) {
#if HEADLIGHT_TRANSFORMATION == 0
		fVector	vPos, vDir;
		G3TransformPoint (&vPos, psl->vPosf, 0);
		G3TransformPoint (&vDir, VmVecAdd (&vDir, psl->vPosf, &psl->vDirf), 0);
		VmVecNormalize (&vDir, VmVecDec (&vDir, &vPos));
		gameData.render.lights.dynamic.headLights.pos [i] = vPos;
		gameData.render.lights.dynamic.headLights.dir [i] = vDir.v3;
#elif HEADLIGHT_TRANSFORMATION == 1
		// method 2: translate, but let OpenGL do the scaling and rotating
		fVector	vPos, vDir;
		VmVecSub (gameData.render.lights.dynamic.headLights.pos + i, psl->vPosf, &viewInfo.posf);
		VmVecInc (&vPos, &psl->vDirf);
		gameData.render.lights.dynamic.headLights.dir [i] = psl->vDirf.v3;
#else
		// method 3: let OpenGL do the translating, scaling and rotating (pass &viewInfo.posf to the headlight shader's vEye value)
		gameData.render.lights.dynamic.headLights.pos [i] = psl->vPosf [0];
		gameData.render.lights.dynamic.headLights.dir [i] = psl->vDirf.v3;
#endif
		gameData.render.lights.dynamic.headLights.brightness [i] = 100.0f;
		}
	}
#	if 0
G3PopMatrix ();
glPopMatrix ();
#endif
}

//------------------------------------------------------------------------------
// A note about transforming the headlight direction:
// To achive that, the direction is added to the original position and transformed,
// and the transformed headlight position is subtracted from that.

void SetupHeadLight (tDynLight *pl, tShaderLight *psl)
{
gameData.render.lights.dynamic.headLights.pl [gameData.render.lights.dynamic.headLights.nLights] = pl;
gameData.render.lights.dynamic.headLights.psl [gameData.render.lights.dynamic.headLights.nLights] = psl;
gameData.render.lights.dynamic.headLights.nLights++;
}

//------------------------------------------------------------------------------

int AddOglHeadLight (tObject *objP)
{
#if 0
	static float spotExps [] = {12.0f, 5.0f, 0.0f};
	static float spotAngles [] = {0.9f, 0.5f, 0.5f};
#endif
	
if (gameOpts->render.nLightingMethod && (gameData.render.lights.dynamic.nHeadLights [objP->id] < 0)) {
		tRgbaColorf	c = {1.0f, 1.0f, 1.0f, 1.0f};
		tDynLight	*pl;
		int			nLight;

	nLight = AddDynLight (NULL, &c, F1_0 * 200, -1, -1, -1, -1, NULL);
	if (nLight >= 0) {
		gameData.render.lights.dynamic.nHeadLights [objP->id] = nLight;
		pl = gameData.render.lights.dynamic.lights + nLight;
		pl->info.nPlayer = (objP->nType == OBJ_PLAYER) ? objP->id : 1;
		pl->info.fRad = 0;
		pl->info.bSpot = 1;
		pl->info.fSpotAngle = 0.9f; //spotAngles [extraGameInfo [IsMultiGame].nSpotSize];
		pl->info.fSpotExponent = 12.0f; //spotExps [extraGameInfo [IsMultiGame].nSpotStrength];
		pl->bTransform = 0;
		}
	return nLight;
	}
return -1;
}

//------------------------------------------------------------------------------

void RemoveOglHeadLight (tObject *objP)
{
if (gameOpts->render.nLightingMethod && (gameData.render.lights.dynamic.nHeadLights [objP->id] >= 0)) {
	DeleteDynLight (gameData.render.lights.dynamic.nHeadLights [objP->id]);
	gameData.render.lights.dynamic.nHeadLights [objP->id] = -1;
	gameData.render.lights.dynamic.headLights.nLights--;
	}
}

//------------------------------------------------------------------------------

void UpdateOglHeadLight (void)
{
	tDynLight	*pl;
	tObject		*objP;
	short			nPlayer;

for (nPlayer = 0; nPlayer < MAX_PLAYERS; nPlayer++) {
	if (gameData.render.lights.dynamic.nHeadLights [nPlayer] < 0)
		continue;
	pl = gameData.render.lights.dynamic.lights + gameData.render.lights.dynamic.nHeadLights [nPlayer];
	objP = OBJECTS + gameData.multiplayer.players [nPlayer].nObject;
	pl->info.vPos = OBJPOS (objP)->vPos;
	pl->vDir = OBJPOS (objP)->mOrient.fVec;
	VmVecScaleInc (&pl->info.vPos, &pl->vDir, objP->size / 4);
	}
}

//------------------------------------------------------------------------------

#if DBG_SHADERS


#else

char *headLightFS [2][8] = {
	{
	//----------------------------------------
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
	"   if (spotEffect >= 0.5) {\r\n" \
	"	    float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"   if (spotEffect >= 0.5) {\r\n" \
	"	    float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"   if (spotEffect >= 0.5) {\r\n" \
	"      float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"if (bMask < 0.5)\r\n" \
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
	"      if (spotEffect >= 0.5) {\r\n" \
	" 		    float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"         spotEffect = pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"      if (spotEffect >= 0.5) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"float spotBrightness = 0.0;\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < LIGHTS; i++) {\r\n" \
	"	 vec3 lightVec = vertPos - gl_LightSource [i].position.xyz;\r\n" \
	"	 float lightDist = length (lightVec);\r\n" \
	"	 vec3 lvNorm = lightVec / lightDist;\r\n" \
	"   if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [i].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.5) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"      if (spotEffect >= 0.5) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
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
	"         if (spotEffect >= 0.5) {\r\n" \
	"            float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	          spotBrightness += pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	" 		       }\r\n" \
	" 		    }\r\n" \
	" 		 }\r\n" \
	"   vec3 spotColor = min (vec3 (spotBrightness, spotBrightness, spotBrightness), matColor.rgb);\r\n" \
	"	 gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n" \
	"   }\r\n" \
	"}" 
	},
	{
	//----------------------------------------
	//single player version - one player
	//untextured
	"uniform vec4 matColor;\r\n" \
	"varying vec3 normal, lightVec;\r\n" \
	"void main (void) {\r\n" \
	"vec4 spotColor = vec4 (0.0, 0.0, 0.0, 0.0);\r\n" \
	"vec3 lvNorm = normalize (lightVec);\r\n" \
	"if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"   float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"   if (spotEffect >= 0.5) {\r\n" \
	"	    float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"   float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"   if (spotEffect >= 0.5) {\r\n" \
	"      float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"   if (spotEffect >= 0.5) {\r\n" \
	"      float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"if (bMask < 0.5)\r\n" \
	"   discard;\r\n" \
	"else {\r\n" \
	"   vec4 texColor = texture2D (baseTex, gl_TexCoord [1].xy);\r\n" \
	"   vec4 decalColor = texture2D (decalTex, gl_TexCoord [2].xy);\r\n" \
	"   texColor = vec4 (vec3 (mix (texColor, decalColor, decalColor.a)), (texColor.a + decalColor.a));\r\n" \
	"	 vec4 spotColor = vec4 (0.0, 0.0, 0.0, 0.0);\r\n" \
	"	 vec3 lvNorm = normalize (lightVec);\r\n" \
	"   if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [0].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.5) {\r\n" \
	" 		    float attenuation = min (400.0 / length (lightVec), 1.0);\r\n" \
	"         spotEffect = pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"      if (spotEffect >= 0.5) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"float spotBrightness = 0.0;\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < LIGHTS; i++) {\r\n" \
	"	 vec3 lightVec = vertPos - gl_LightSource [i].position.xyz;\r\n" \
	"	 float lightDist = length (lightVec);\r\n" \
	"	 vec3 lvNorm = lightVec / lightDist;\r\n" \
	"   if (dot (normalize (normal), lvNorm) < 0.0) {\r\n" \
	"      float spotEffect = dot (gl_LightSource [i].spotDirection, lvNorm);\r\n" \
	"      if (spotEffect >= 0.5) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"      if (spotEffect >= 0.5) {\r\n" \
	"   	   float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	      spotBrightness += pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
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
	"if (bMask < 0.5)\r\n" \
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
	"         if (spotEffect >= 0.5) {\r\n" \
	"            float attenuation = min (400.0 / lightDist, 1.0);\r\n" \
	" 	          spotBrightness += pow (spotEffect * 1.025, 4.0 + 16.0 * spotEffect) * attenuation;\r\n" \
	" 		       }\r\n" \
	" 		    }\r\n" \
	" 		 }\r\n" \
	"   vec3 spotColor = min (vec3 (spotBrightness, spotBrightness, spotBrightness), matColor.rgb);\r\n" \
	"   gl_FragColor = vec4 (texColor.rgb * spotColor, texColor.a * gl_Color.a);\r\n" \
	"   }\r\n" \
	"}" 
	}
	};

char *headLightVS [2][8] = {
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

GLhandleARB headLightShaderProgs [2][4] = {{0,0,0,0},{0,0,0,0}};
GLhandleARB lvs [2][4] = {{0,0,0,0},{0,0,0,0}};
GLhandleARB lfs [2][4] = {{0,0,0,0},{0,0,0,0}}; 

//-------------------------------------------------------------------------

void DeleteHeadLightShader (void)
{
for (int i = 0; i < 2; i++) {
	for (int j = 0; j < 4; j++) {
		if (headLightShaderProgs [i][j]) {
			DeleteShaderProg (&headLightShaderProgs [i][j]);
			headLightShaderProgs [i][j] = 0;
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
	nLights = gameData.render.ogl.nHeadLights;
	gameData.render.ogl.nHeadLights = 0;
	}
if (nLights == gameData.render.ogl.nHeadLights)
	return;
gameStates.render.bHaveDynLights = 0;
PrintLog ("building lighting shader programs\n");
if ((gameStates.ogl.bHeadLight = (gameStates.ogl.bShadersOk && gameOpts->render.nPath))) {
	gameStates.render.bHaveDynLights = 1;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 4; j++) {
			if (headLightShaderProgs [i][j])
				DeleteShaderProg (&headLightShaderProgs [i][j]);
#if 1//ndef _DEBUG
			if (nLights == 1)
				pszFS = headLightFS [i][h = j];
			else
#endif
				pszFS = BuildLightingShader (headLightFS [i][h = j + 4], nLights);
			bOk = (pszFS != NULL) &&
					CreateShaderProg (&headLightShaderProgs [i][j]) &&
					CreateShaderFunc (&headLightShaderProgs [i][j], &lfs [i][j], &lvs [i][j], pszFS, headLightVS [i][h], 1) &&
					LinkShaderProg (&headLightShaderProgs [i][j]);
			if (pszFS && (nLights > 1))
				D2_FREE (pszFS);
			if (!bOk) {
				DeleteHeadLightShader ();
				nLights = 0;
				break;
				}
			}
		}
	}
gameData.render.ogl.nHeadLights = nLights;
}

//------------------------------------------------------------------------------

int G3SetupHeadLightShader (int nType, int bLightMaps, tRgbaColorf *colorP)
{
#ifdef _DEBUG
	int			oglRes;
#endif
	int			nLights, nShader;
	tRgbaColorf	color;

//headlights
nLights = IsMultiGame ? gameData.render.lights.dynamic.headLights.nLights : 1;
InitHeadlightShaders (nLights);
nShader = 10 + bLightMaps * 4 + nType;
if (nShader != gameStates.render.history.nShader) {
	//glUseProgramObject (0);
	gameData.render.nShaderChanges++;
	glUseProgramObject (activeShaderProg = headLightShaderProgs [bLightMaps][nType]);
	if (nType) {
		glUniform1i (glGetUniformLocation (activeShaderProg, "baseTex"), bLightMaps);
		if (nType > 1) {
			glUniform1i (glGetUniformLocation (activeShaderProg, "decalTex"), 1 + bLightMaps);
			if (nType > 2)
				glUniform1i (glGetUniformLocation (activeShaderProg, "maskTex"), 2 + bLightMaps);
			}
		}
#if 1
#	if 0
	glUniform1f (glGetUniformLocation (activeShaderProg, "cutOff"), 0.5f);
	glUniform1f (glGetUniformLocation (activeShaderProg, "spotExp"), 8.0f);
	glUniform1f (glGetUniformLocation (activeShaderProg, "grAlpha"), 1.0f);
	glUniform1fv (glGetUniformLocation (activeShaderProg, "brightness"), nLights, 
					  (GLfloat *) gameData.render.lights.dynamic.headLights.brightness);
#	endif
	//glUniform1f (glGetUniformLocation (activeShaderProg, "aspect"), (float) grdCurScreen->scWidth / (float) grdCurScreen->scHeight);
	//glUniform1f (glGetUniformLocation (activeShaderProg, "zoom"), 65536.0f / (float) gameStates.render.xZoom);
#if 1
	for (int i = 0; i < nLights; i++) {
		glEnable (GL_LIGHT0 + i);
		glLightfv (GL_LIGHT0 + i, GL_POSITION, (GLfloat *) (gameData.render.lights.dynamic.headLights.pos + i));
		glLightfv (GL_LIGHT0 + i, GL_SPOT_DIRECTION, (GLfloat *) (gameData.render.lights.dynamic.headLights.dir + i));
		}
#else
	glUniform3fv (glGetUniformLocation (activeShaderProg, "lightPosWorld"), nLights, 
					  (GLfloat *) gameData.render.lights.dynamic.headLights.pos);
	glUniform3fv (glGetUniformLocation (activeShaderProg, "lightDirWorld"), nLights, 
					  (GLfloat *) gameData.render.lights.dynamic.headLights.dir);
#endif
#endif
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
	glUniform4fv (glGetUniformLocation (activeShaderProg, "matColor"), 1, (GLfloat *) &color);
#ifdef _DEBUG
	oglRes = glGetError ();
#endif
	}
return gameStates.render.history.nShader = nShader;
}

// ----------------------------------------------------------------------------------------------
//eof
