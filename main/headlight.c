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
#include "dynlight.h"

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
		dist = VmVecNormalizeQuick (&vecToObj);
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

void TransformOglHeadLight (tDynLight *pl, tShaderLight *psl)
{
	fVector	vPos, vDir;

VmsVecToFloat (&psl->dir, &pl->vDir);
if (pl->bTransform && !gameStates.ogl.bUseTransform)
	G3RotatePointf (&psl->dir, &psl->dir, 0);
psl->spotAngle = pl->spotAngle;
psl->spotExponent = pl->spotExponent;
if (gameStates.ogl.bHeadLight && gameOpts->ogl.bHeadLight) {
#if 1
	if (!gameData.render.lights.dynamic.headLights.nLights) {
		G3StartFrame (0, 0);
		G3SetViewMatrix (&gameData.objs.viewer->position.vPos, 
								&gameData.objs.viewer->position.mOrient, 
								gameStates.render.xZoom, 0);
		}
	G3TransformPointf (&vPos, psl->pos, 0);
	G3TransformPointf (&vDir, VmVecAddf (&vDir, psl->pos, &psl->dir), 0);
	VmVecNormalizef (&vDir, VmVecDecf (&vDir, &vPos));
	//vDir.p.z = -vDir.p.z;
	memcpy (gameData.render.lights.dynamic.headLights.pos + gameData.render.lights.dynamic.headLights.nLights, &vPos, sizeof (fVector3));
	memcpy (gameData.render.lights.dynamic.headLights.dir + gameData.render.lights.dynamic.headLights.nLights, &vDir, sizeof (fVector3));
#else
#	if 0
	VmsVecToFloat (&vPos, &gameData.objs.viewer->position.vPos);
	VmVecNegatef (&vPos);
	VmVecIncf (&vPos, psl->pos);
	memcpy (gameData.render.lights.dynamic.headLights.pos + gameData.render.lights.dynamic.headLights.nLights, &vPos, sizeof (fVector3));
#	else
	memcpy (gameData.render.lights.dynamic.headLights.pos + gameData.render.lights.dynamic.headLights.nLights, psl->pos, sizeof (fVector3));
#	endif
//			VmVecIncf (&vPos, &psl->dir);
	memcpy (gameData.render.lights.dynamic.headLights.dir + gameData.render.lights.dynamic.headLights.nLights, &psl->dir, sizeof (fVector3));
#endif
	gameData.render.lights.dynamic.headLights.brightness [gameData.render.lights.dynamic.headLights.nLights] = 100.0f;
	gameData.render.lights.dynamic.headLights.nLights++;
	}
}

//------------------------------------------------------------------------------

int AddOglHeadLight (tObject *objP)
{
	static float spotExps [] = {12.0f, 5.0f, 0.0f};
	static float spotAngles [] = {0.9f, 0.75f, 0.5f};

if (gameOpts->render.bDynLighting && (gameData.render.lights.dynamic.nHeadLights [objP->id] < 0)) {
		tRgbaColorf	c = {1.0f, 1.0f, 1.0f, 1.0f};
		tDynLight	*pl;
		int			nLight;

	nLight = AddDynLight (&c, F1_0 * 200, -1, -1, -1, NULL);
	if (nLight >= 0) {
		gameData.render.lights.dynamic.nHeadLights [objP->id] = nLight;
		pl = gameData.render.lights.dynamic.lights + nLight;
		pl->nPlayer = (objP->nType == OBJ_PLAYER) ? objP->id : 1;
		pl->bSpot = 1;
		pl->spotAngle = 0.9f; //spotAngles [extraGameInfo [IsMultiGame].nSpotSize];
		pl->spotExponent = 12.0f; //spotExps [extraGameInfo [IsMultiGame].nSpotStrength];
		pl->bTransform = 0;
		}
	return nLight;
	}
return -1;
}

//------------------------------------------------------------------------------

void RemoveOglHeadLight (tObject *objP)
{
if (gameOpts->render.bDynLighting && (gameData.render.lights.dynamic.nHeadLights [objP->id] >= 0)) {
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
	objP = nPlayer ? OBJECTS + nPlayer : gameData.objs.objects + gameData.multiplayer.players [nPlayer].nObject;
	pl->vPos = OBJPOS (objP)->vPos;
	pl->vDir = OBJPOS (objP)->mOrient.fVec;
	//VmVecScaleInc (&pl->vPos, &pl->vDir, objP->size / 4);
	}
}

//------------------------------------------------------------------------------

#if DBG_SHADERS


#else

char *lightingFS [12] = {
	//----------------------------------------
	//single player version - one player
	//untextured
	"varying vec3 vertPos;\r\n" \
	"uniform vec3 lightPos, lightDir;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float grAlpha, aspect, cutOff, spotExp, brightness;\r\n" \
	"void main(void) {\r\n" \
	"vec3 lightVec, spotColor = vec3 (0.0, 0.0, 0.0), v = vec3 (vertPos.x * aspect, vertPos.y, vertPos.z);\r\n" \
	"float spotEffect, lightDist;\r\n" \
	"lightVec = v - lightPos;\r\n" \
	"lightDist = length (lightVec);\r\n" \
	"spotEffect = dot (lightDir, lightVec / lightDist);\r\n" \
	"if (spotEffect < cutOff)\r\n" \
	"   gl_FragColor = vec4 (vec3 (matColor) * vec3 (gl_Color), (matColor.a + gl_Color.a) * grAlpha);"  \
	"else {\r\n" \
	"	 float attenuation = min (100.0 / lightDist, 1.0);\r\n" \
	"   spotEffect = pow (spotEffect * 1.1, spotExp) * attenuation;\r\n" \
	"   spotColor = vec3 (max (spotEffect, gl_Color.r), max (spotEffect, gl_Color.g), max (spotEffect, gl_Color.b));\r\n" \
	"   gl_FragColor = vec4 (vec3 (matColor) * vec3 (spotColor), matColor.a * grAlpha);"  \
	"   }\r\n" \
	"}" 
	,
	//only base texture
	"uniform sampler2D btmTex;\r\n" \
	"varying vec3 vertPos;\r\n" \
	"uniform vec3 lightPos, lightDir;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float grAlpha, aspect, cutOff, spotExp, brightness;\r\n" \
	"void main(void) {\r\n" \
	"vec4 btmColor = texture2D (btmTex, gl_TexCoord [0].xy);\r\n" \
	"vec3 lightVec, spotColor = vec3 (0.0, 0.0, 0.0), v = vec3 (vertPos.x * aspect, vertPos.y, vertPos.z);\r\n" \
	"float spotEffect, lightDist;\r\n" \
	"lightVec = v - lightPos;\r\n" \
	"lightDist = length (lightVec);\r\n" \
	"spotEffect = dot (lightDir, lightVec / lightDist);\r\n" \
	"if (spotEffect < cutOff)\r\n" \
	"   gl_FragColor = vec4 (vec3 (btmColor), btmColor.a * grAlpha) * gl_Color;\r\n" \
	"else {\r\n" \
	"	 float attenuation = min (100.0 / lightDist, 1.0);\r\n" \
	"   spotEffect = pow (spotEffect * 1.1, spotExp) * attenuation;\r\n" \
	"   spotColor = vec3 (max (spotEffect, gl_Color.r), max (spotEffect, gl_Color.g), max (spotEffect, gl_Color.b));\r\n" \
	"   spotColor = vec3 (min (spotColor.r, matColor.r), min (spotColor.g, matColor.g), min (spotColor.b, matColor.b));\r\n" \
	"   gl_FragColor = vec4 (vec3 (btmColor), btmColor.a * grAlpha) * vec4 (spotColor, gl_Color.a);\r\n" \
	"	 }\r\n" \
	"}" 
	,
	//base texture and decal
	"uniform sampler2D btmTex, topTex;\r\n" \
	"varying vec3 vertPos;\r\n" \
	"uniform vec3 lightPos, lightDir;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float grAlpha, aspect, cutOff, spotExp, brightness;\r\n" \
	"void main(void) {\r\n" \
	"vec4 btmColor = texture2D (btmTex, gl_TexCoord [0].xy);\r\n" \
	"vec4 topColor = texture2D (topTex, gl_TexCoord [1].xy);\r\n" \
	"vec3 lightVec, spotColor = vec3 (0.0, 0.0, 0.0), v = vec3 (vertPos.x * aspect, vertPos.y, vertPos.z);\r\n" \
	"float spotEffect, lightDist;\r\n" \
	"lightVec = v - lightPos;\r\n" \
	"lightDist = length (lightVec);\r\n" \
	"spotEffect = dot (lightDir, lightVec / lightDist);\r\n" \
	"if (spotEffect < cutOff)\r\n" \
	"   gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * gl_Color;\r\n" \
	"else {\r\n" \
	"   float attenuation = min (100.0 / lightDist, 1.0);\r\n" \
	"   spotEffect = pow (spotEffect * 1.1, spotExp) * attenuation;\r\n" \
	"   spotColor = vec3 (max (spotEffect, gl_Color.r), max (spotEffect, gl_Color.g), max (spotEffect, gl_Color.b));\r\n" \
	"   spotColor = vec3 (min (spotColor.r, matColor.r), min (spotColor.g, matColor.g), min (spotColor.b, matColor.b));\r\n" \
	"   gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * vec4 (spotColor, gl_Color.a);\r\n" \
	"	 }\r\n" \
	"}" 
	,
	//base texture and decal with color key
	"uniform sampler2D btmTex, topTex, maskTex;\r\n" \
	"varying vec3 vertPos;\r\n" \
	"uniform vec3 lightPos, lightDir;\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float grAlpha, aspect, cutOff, spotExp, brightness;\r\n" \
	"void main (void) {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"   discard;\r\n" \
	"else {\r\n" \
	"   vec4 btmColor = texture2D (btmTex, gl_TexCoord [0].xy);\r\n" \
	"   vec4 topColor = texture2D (topTex, gl_TexCoord [1].xy);\r\n" \
	"   vec3 lightVec, spotColor = vec3 (0.0, 0.0, 0.0), v = vec3 (vertPos.x * aspect, vertPos.y, vertPos.z);\r\n" \
	"   float spotEffect, lightDist;\r\n" \
	"   lightVec = v - lightPos;\r\n" \
	"   lightDist = length (lightVec);\r\n" \
	"   spotEffect = dot (lightDir, lightVec / lightDist);\r\n" \
	"   if (spotEffect < cutOff)\r\n" \
	"      gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * gl_Color;\r\n" \
	"   else {\r\n" \
	" 		 float attenuation = min (100.0 / lightDist, 1.0);\r\n" \
	"      spotEffect = pow (spotEffect * 1.1, spotExp) * attenuation;\r\n" \
	"      spotColor = vec3 (max (spotEffect, gl_Color.r), max (spotEffect, gl_Color.g), max (spotEffect, gl_Color.b));\r\n" \
	"      spotColor = vec3 (min (spotColor.r, matColor.r), min (spotColor.g, matColor.g), min (spotColor.b, matColor.b));\r\n" \
	"      gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * vec4 (spotColor, gl_Color.a);\r\n" \
	"   	}\r\n" \
	"   }\r\n" \
	"}" 
	,
	// --------------------------------------------------------------------------------
	//coop version - max. 4 players
	"varying vec3 vertPos;\r\n" \
	"uniform vec3 vEye, lightPos [X], lightDir [X];\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float grAlpha, aspect, cutOff, spotExp, brightness [X];\r\n" \
	"void main(void) {\r\n" \
	"vec3 lp, ld, lightVec, spotColor, v = vec3 (vertPos.x * aspect, vertPos.y, vertPos.z);\r\n" \
	"float spotEffect, lightDist, spotBrightness = 0.0;\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < X; i++) {\r\n" \
	"/*   lp = lightPos [i] - vEye;\r\n" \
	"   ld = vec3 (gl_ModelViewMatrix * vec4 (lp + lightDir [i], 1.0));\r\n" \
	"   lp = vec3 (gl_ModelViewMatrix * vec4 (lp, 1.0));\r\n" \
	"   ld = normalize (ld - lp);*/\r\n" \
	"   lightVec = v - lightPos [i];\r\n" \
	"   lightDist = length (lightVec);\r\n" \
	"   spotEffect = dot (lightDir [i], lightVec / lightDist);\r\n" \
	"   if (spotEffect >= cutOff) {\r\n" \
	"   	float attenuation = min (brightness [i] / lightDist, 1.0);\r\n" \
	" 	   spotBrightness += pow (spotEffect * 1.1, spotExp) * attenuation;\r\n" \
	" 	   }\r\n" \
	" 	}\r\n" \
	"spotColor = vec3 (max (spotBrightness, gl_Color.r), max (spotBrightness, gl_Color.g), max (spotBrightness, gl_Color.b));\r\n" \
	"spotColor = vec3 (min (spotColor.r, matColor.r), min (spotColor.g, matColor.g), min (spotColor.b, matColor.b));\r\n" \
	"gl_FragColor = vec4 (vec3 (matColor) * vec3 (spotColor), matColor.a * grAlpha);"  \
	"}" 
	,
	//only base texture
	"uniform sampler2D btmTex;\r\n" \
	"varying vec3 vertPos;\r\n" \
	"uniform vec3 vEye, lightPos [X], lightDir [X];\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float grAlpha, aspect, cutOff, spotExp, brightness [X];\r\n" \
	"void main(void) {\r\n" \
	"vec4 btmColor = texture2D (btmTex, gl_TexCoord [0].xy);\r\n" \
	"vec3 lp, ld, lightVec, spotColor, v = vec3 (vertPos.x * aspect, vertPos.y, vertPos.z);\r\n" \
	"float spotEffect, lightDist, spotBrightness = 0.0;\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < X; i++) {\r\n" \
	"/*   lp = lightPos [i] - vEye;\r\n" \
	"   ld = vec3 (gl_ModelViewMatrix * vec4 (lp + lightDir [i], 1.0));\r\n" \
	"   lp = vec3 (gl_ModelViewMatrix * vec4 (lp, 1.0));\r\n" \
	"   ld = normalize (ld - lp);*/\r\n" \
	"   lightVec = v - lightPos [i];\r\n" \
	"   lightDist = length (lightVec);\r\n" \
	"   spotEffect = dot (lightDir [i], lightVec / lightDist);\r\n" \
	"   if (spotEffect >= cutOff) {\r\n" \
	"   	float attenuation = min (brightness [i] / lightDist, 1.0);\r\n" \
	" 	   spotBrightness += pow (spotEffect * 1.1, spotExp) * attenuation;\r\n" \
	" 	   }\r\n" \
	" 	}\r\n" \
	"spotColor = vec3 (max (spotBrightness, gl_Color.r), max (spotBrightness, gl_Color.g), max (spotBrightness, gl_Color.b));\r\n" \
	"spotColor = vec3 (min (spotColor.r, matColor.r), min (spotColor.g, matColor.g), min (spotColor.b, matColor.b));\r\n" \
	"gl_FragColor = vec4 (vec3 (btmColor), btmColor.a * grAlpha) * vec4 (spotColor, gl_Color.a);\r\n" \
	"}" 
	,
	//base texture and decal
	"uniform sampler2D btmTex, topTex;\r\n" \
	"varying vec3 vertPos;\r\n" \
	"uniform vec3 vEye, lightPos [X], lightDir [X];\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float grAlpha, aspect, cutOff, spotExp, brightness [X];\r\n" \
	"void main(void) {\r\n" \
	"vec4 btmColor = texture2D (btmTex, gl_TexCoord [0].xy);\r\n" \
	"vec4 topColor = texture2D (topTex, gl_TexCoord [1].xy);\r\n" \
	"vec3 lp, ld, lightVec, spotColor, v = vec3 (vertPos.x * aspect, vertPos.y, vertPos.z);\r\n" \
	"float spotEffect, lightDist, spotBrightness = 0.0;\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < X; i++) {\r\n" \
	"/*   lp = lightPos [i] - vEye;\r\n" \
	"   ld = vec3 (gl_ModelViewMatrix * vec4 (lp + lightDir [i], 1.0));\r\n" \
	"   lp = vec3 (gl_ModelViewMatrix * vec4 (lp, 1.0));\r\n" \
	"   ld = normalize (ld - lp);*/\r\n" \
	"   lightVec = v - lightPos [i];\r\n" \
	"   lightDist = length (lightVec);\r\n" \
	"   spotEffect = dot (lightDir [i], lightVec / lightDist);\r\n" \
	"   if (spotEffect >= cutOff) {\r\n" \
	"   	float attenuation = min (brightness [i] / lightDist, 1.0);\r\n" \
	" 	   spotBrightness += pow (spotEffect * 1.1, spotExp) * attenuation;\r\n" \
	" 	   }\r\n" \
	" 	}\r\n" \
	"spotColor = vec3 (max (spotBrightness, gl_Color.r), max (spotBrightness, gl_Color.g), max (spotBrightness, gl_Color.b));\r\n" \
	"spotColor = vec3 (min (spotColor.r, matColor.r), min (spotColor.g, matColor.g), min (spotColor.b, matColor.b));\r\n" \
	"gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * vec4 (spotColor, gl_Color.a);\r\n" \
	"}" 
	,
	//base texture and decal with color key
	"uniform sampler2D btmTex, topTex, maskTex;\r\n" \
	"varying vec3 vertPos;\r\n" \
	"uniform vec3 vEye, lightPos [X], lightDir [X];\r\n" \
	"uniform vec4 matColor;\r\n" \
	"uniform float grAlpha, aspect, cutOff, spotExp, brightness [X];\r\n" \
	"void main (void) {\r\n" \
	"float bMask = texture2D (maskTex, gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.5)\r\n" \
	"   discard;\r\n" \
	"else {\r\n" \
	"   vec4 btmColor = texture2D (btmTex, gl_TexCoord [0].xy);\r\n" \
	"   vec4 topColor = texture2D (topTex, gl_TexCoord [1].xy);\r\n" \
	"   vec3 lp, ld, lightVec, spotColor = vec3 (0.0, 0.0, 0.0), v = vec3 (vertPos.x * aspect, vertPos.y, vertPos.z);\r\n" \
	"float spotEffect, lightDist, spotBrightness = 0.0;\r\n" \
	"int i;\r\n" \
	"for (i = 0; i < X; i++) {\r\n" \
	"/*   lp = lightPos [i] - vEye;\r\n" \
	"   ld = vec3 (gl_ModelViewMatrix * vec4 (lp + lightDir [i], 1.0));\r\n" \
	"   lp = vec3 (gl_ModelViewMatrix * vec4 (lp, 1.0));\r\n" \
	"   ld = normalize (ld - lp);*/\r\n" \
	"   lightVec = v - lightPos [i];\r\n" \
	"   lightDist = length (lightVec);\r\n" \
	"   spotEffect = dot (lightDir [i], lightVec / lightDist);\r\n" \
	"   if (spotEffect >= cutOff) {\r\n" \
	"   	float attenuation = min (brightness [i] / lightDist, 1.0);\r\n" \
	" 	   spotBrightness += pow (spotEffect * 1.1, spotExp) * attenuation;\r\n" \
	" 	   }\r\n" \
	" 	}\r\n" \
	"   spotColor = vec3 (max (spotBrightness, gl_Color.r), max (spotBrightness, gl_Color.g), max (spotBrightness, gl_Color.b));\r\n" \
	"   spotColor = vec3 (min (spotColor.r, matColor.r), min (spotColor.g, matColor.g), min (spotColor.b, matColor.b));\r\n" \
	"   gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * vec4 (spotColor, gl_Color.a);\r\n" \
	"   }\r\n" \
	"}" 
	};

char *lightingVS [4] = {
	"varying vec3 vertPos;\r\n" \
	"void main(void){\r\n" \
	"vertPos=vec3(gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"gl_Position = ftransform() /*gl_ModelViewProjectionMatrix * gl_Vertex*/;\r\n" \
   "gl_FrontColor=gl_Color;}"
	,
	"varying vec3 vertPos;\r\n" \
	"void main(void){\r\n" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"vertPos=vec3(gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"gl_Position = ftransform() /*gl_ModelViewProjectionMatrix * gl_Vertex*/;\r\n" \
   "gl_FrontColor=gl_Color;}"
	,
	"varying vec3 vertPos;\r\n" \
	"void main(void){\r\n" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"vertPos=vec3(gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"gl_Position = ftransform() /*gl_ModelViewProjectionMatrix * gl_Vertex*/;\r\n" \
   "gl_FrontColor=gl_Color;}"
	,
	"varying vec3 vertPos;\r\n" \
	"void main(void){\r\n" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_TexCoord [2]=gl_MultiTexCoord2;"\
	"vertPos=vec3(gl_ModelViewMatrix * gl_Vertex);\r\n" \
	"gl_Position = ftransform() /*gl_ModelViewProjectionMatrix * gl_Vertex*/;\r\n" \
   "gl_FrontColor=gl_Color;}"
	};

#endif

//-------------------------------------------------------------------------

char *BuildLightingShader (char *pszTemplate, int nLights)
{
	int	l = (int) strlen (pszTemplate) + 1;
	char	*pszFS, *p;

if (!(pszFS = (char *) D2_ALLOC (l)))
	return NULL;
memcpy (pszFS, pszTemplate, l);
while (p = strstr (pszFS, "[X]")) {
	sprintf (p + 1, "%d", nLights);
	p [2] = ']';
	}
if (p = strstr (pszFS, "i < X")) {
	sprintf (p + 4, "%d", nLights);
	p [5] = ';';
	}
return pszFS;
}

//-------------------------------------------------------------------------

GLhandleARB lightingShaderProgs [4] = {0,0,0,0};
GLhandleARB lvs [4] = {0,0,0,0}; 
GLhandleARB lfs [4] = {0,0,0,0}; 

void InitLightingShaders (int nLights)
{
	int	i, bOk;
	char	*pszFS;

if (nLights < 0) {
	nLights = gameData.render.ogl.nHeadLights;
	gameData.render.ogl.nHeadLights = 0;
	}
if (nLights == gameData.render.ogl.nHeadLights)
	return;
gameStates.render.bHaveDynLights = 0;
LogErr ("building lighting shader programs\n");
if ((gameStates.ogl.bHeadLight = (gameStates.ogl.bShadersOk && gameOpts->render.nPath))) {
	gameStates.render.bHaveDynLights = 1;
	for (i = 0; i < 4; i++) {
		if (lightingShaderProgs [i])
			DeleteShaderProg (lightingShaderProgs + i);
#ifndef _DEBUG
		if (nLights == 1)
			pszFS = lightingFS [i];
		else
#endif
			pszFS = BuildLightingShader (lightingFS [i + 4], nLights);
		bOk = (pszFS != NULL) &&
				CreateShaderProg (lightingShaderProgs + i) &&
				CreateShaderFunc (lightingShaderProgs + i, lfs + i, lvs + i, pszFS, lightingVS [i % 4], 1) &&
	-			LinkShaderProg (lightingShaderProgs + i);
		if (pszFS && (nLights > 1))
			D2_FREE (pszFS);
		if (!bOk) {
			gameStates.ogl.bHeadLight = 0;
			while (i)
				DeleteShaderProg (lightingShaderProgs + --i);
			nLights = 0;
			break;
			}
		}
	}
gameData.render.ogl.nHeadLights = nLights;
}

// ----------------------------------------------------------------------------------------------
//eof
