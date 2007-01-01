/* $Id: lighting.c,v 1.4 2003/10/04 03:14:47 btb Exp $ */
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
#include <string.h>	// for memset()

#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "inferno.h"
#include "segment.h"
#include "error.h"
#include "mono.h"
#include "render.h"
#include "game.h"
#include "vclip.h"
#include "lighting.h"
#include "3d.h"
#include "laser.h"
#include "timer.h"
#include "player.h"
#include "weapon.h"
#include "powerup.h"
#include "object.h"
#include "fvi.h"
#include "robot.h"
#include "multi.h"
#include "hudmsg.h"
#include "gameseg.h"
#include "maths.h"
#include "network.h"
#include "lightmap.h"
#include "gamemine.h"
#include "text.h"

#define FLICKERFIX 0

//int	Use_fvi_lighting = 0;

fix	dynamicLight [MAX_VERTICES];
tRgbColorf dynamicColor [MAX_VERTICES];
char  bGotDynColor [MAX_VERTICES];
//tFaceColor DyngameData.render.vertices [MAX_VERTICES];
char  bGotGlobalDynColor;
char  bStartDynColoring;
char  bInitDynColoring = 1;
tRgbColorf globalDynColor;

#define	LIGHTING_CACHE_SIZE	4096	//	Must be power of 2!
#define	LIGHTING_FRAME_DELTA	256	//	Recompute cache value every 8 frames.
#define	LIGHTING_CACHE_SHIFT	8

int	Lighting_frame_delta = 1;

int	Lighting_cache [LIGHTING_CACHE_SIZE];

int Cache_hits=0, Cache_lookups=1;

extern vmsVector player_thrust;

typedef struct {
  int    nTexture;
  int		nBrightness;
} tTexBright;

#define	NUM_LIGHTS_D1     49
#define	NUM_LIGHTS_D2     85
#define	MAX_BRIGHTNESS		F2_0

tTexBright texBrightD1 [NUM_LIGHTS_D1] = {
	{250, 0x00b333L}, {251, 0x008000L}, {252, 0x008000L}, {253, 0x008000L},
	{264, 0x01547aL}, {265, 0x014666L}, {268, 0x014666L}, {278, 0x014cccL},
	{279, 0x014cccL}, {280, 0x011999L}, {281, 0x014666L}, {282, 0x011999L},
	{283, 0x0107aeL}, {284, 0x0107aeL}, {285, 0x011999L}, {286, 0x014666L},
	{287, 0x014666L}, {288, 0x014666L}, {289, 0x014666L}, {292, 0x010cccL},
	{293, 0x010000L}, {294, 0x013333L}, {328, 0x011333L}, {330, 0x010000L}, 
	{333, 0x010000L}, {341, 0x010000L}, {343, 0x010000L}, {345, 0x010000L}, 
	{347, 0x010000L}, {349, 0x010000L}, {351, 0x010000L}, {352, 0x010000L}, 
	{354, 0x010000L}, {355, 0x010000L}, {356, 0x020000L}, {357, 0x020000L}, 
	{358, 0x020000L}, {359, 0x020000L}, {360, 0x020000L}, {361, 0x020000L}, 
	{362, 0x020000L}, {363, 0x020000L}, {364, 0x020000L}, {365, 0x020000L}, 
	{366, 0x020000L}, {367, 0x020000L}, {368, 0x020000L}, {369, 0x020000L}, 
	{370, 0x020000L}
};

tTexBright texBrightD2 [NUM_LIGHTS_D2] = {
	{235, 0x012666L}, {236, 0x00b5c2L}, {237, 0x00b5c2L}, {243, 0x00b5c2L},
	{244, 0x00b5c2L}, {275, 0x01547aL}, {276, 0x014666L}, {278, 0x014666L},
	{288, 0x014cccL}, {289, 0x014cccL}, {290, 0x011999L}, {291, 0x014666L},
	{293, 0x011999L}, {295, 0x0107aeL}, {296, 0x011999L}, {298, 0x014666L},
	{300, 0x014666L}, {301, 0x014666L}, {302, 0x014666L}, {305, 0x010cccL},
	{306, 0x010000L}, {307, 0x013333L}, {340, 0x00b333L}, {341, 0x00b333L},
	{343, 0x004cccL}, {344, 0x003333L}, {345, 0x00b333L}, {346, 0x004cccL},
	{348, 0x003333L}, {349, 0x003333L}, {353, 0x011333L}, {356, 0x00028fL},
	{357, 0x00028fL}, {358, 0x00028fL}, {359, 0x00028fL}, {364, 0x010000L},
	{366, 0x010000L}, {368, 0x010000L}, {370, 0x010000L}, {372, 0x010000L},
	{374, 0x010000L}, {375, 0x010000L}, {377, 0x010000L}, {378, 0x010000L},
	{380, 0x010000L}, {382, 0x010000L}, {383, 0x020000L}, {384, 0x020000L},
	{385, 0x020000L}, {386, 0x020000L}, {387, 0x020000L}, {388, 0x020000L},
	{389, 0x020000L}, {390, 0x020000L}, {391, 0x020000L}, {392, 0x020000L},
	{393, 0x020000L}, {394, 0x020000L}, {395, 0x020000L}, {396, 0x020000L},
	{397, 0x020000L}, {398, 0x020000L}, {404, 0x010000L}, {405, 0x010000L},
	{406, 0x010000L}, {407, 0x010000L}, {408, 0x010000L}, {409, 0x020000L},
	{410, 0x008000L}, {411, 0x008000L}, {412, 0x008000L}, {419, 0x020000L},
	{420, 0x020000L}, {423, 0x010000L}, {424, 0x010000L}, {425, 0x020000L},
	{426, 0x020000L}, {427, 0x008000L}, {428, 0x008000L}, {429, 0x008000L},
	{430, 0x020000L}, {431, 0x020000L}, {432, 0x00e000L}, {433, 0x020000L},
	{434, 0x020000L}
};

int AddOglHeadLight (tObject *objP);
void RemoveOglHeadLight (tObject *objP);
void UpdateOglHeadLight (void);

//--------------------------------------------------------------------------

void InitTextureBrightness (void)
{
	tTexBright	*ptb = gameStates.app.bD1Mission ? texBrightD1  : texBrightD2;
	int			i, j, h = (gameStates.app.bD1Mission ? sizeof (texBrightD1) : sizeof (texBrightD2)) / sizeof (tTexBright);

memset (gameData.pig.tex.brightness, 0, sizeof (gameData.pig.tex.brightness));
for (i = 0; i < MAX_WALL_TEXTURES; i++) {
	j = gameStates.app.bD1Mission ? ConvertD1Texture (i, 1) : i;
	if (gameData.pig.tex.pTMapInfo [j].lighting)
		gameData.pig.tex.brightness [j] = gameData.pig.tex.pTMapInfo [j].lighting;
	}
for (i = h; --i; ) {
	gameData.pig.tex.brightness [ptb [i].nTexture] = 
		((ptb [i].nBrightness * 100 + MAX_BRIGHTNESS / 2) / MAX_BRIGHTNESS) * (MAX_BRIGHTNESS / 100);
	}
}

// ----------------------------------------------------------------------------------------------
//	Return true if we think vertex vertnum is visible from tSegment nSegment.
//	If some amount of time has gone by, then recompute, else use cached value.
int LightingCacheVisible(int vertnum, int nSegment, int nObject, vmsVector *obj_pos, int obj_seg, vmsVector *vertpos)
{
	int	cache_val, cache_frame, cache_vis;

	cache_val = Lighting_cache [((nSegment << LIGHTING_CACHE_SHIFT) ^ vertnum) & (LIGHTING_CACHE_SIZE-1)];

	cache_frame = cache_val >> 1;
	cache_vis = cache_val & 1;

Cache_lookups++;
	if ((cache_frame == 0) || (cache_frame + Lighting_frame_delta <= gameData.app.nFrameCount)) {
		int			bApplyLight=0;
		fvi_query	fq;
		fvi_info		hit_data;
		int			nSegment, hitType;

		nSegment = -1;

		#ifndef NDEBUG
		nSegment = FindSegByPoint(obj_pos, obj_seg);
		if (nSegment == -1) {
			Int3();		//	Obj_pos is not in obj_seg!
			return 0;		//	Done processing this tObject.
		}
		#endif

		fq.p0					= obj_pos;
		fq.startSeg			= obj_seg;
		fq.p1					= vertpos;
		fq.rad				= 0;
		fq.thisObjNum		= nObject;
		fq.ignoreObjList	= NULL;
		fq.flags				= FQ_TRANSWALL;

		hitType = FindVectorIntersection(&fq, &hit_data);

		// gameData.ai.vHitPos = gameData.ai.hitData.hit.vPoint;
		// gameData.ai.nHitSeg = gameData.ai.hitData.hit_seg;

		if (hitType == HIT_OBJECT)
			Int3();	//	Hey, we're not supposed to be checking gameData.objs.objects!

		if (hitType == HIT_NONE)
			bApplyLight = 1;
		else if (hitType == HIT_WALL) {
			fix	distDist;
			distDist = VmVecDistQuick(&hit_data.hit.vPoint, obj_pos);
			if (distDist < F1_0/4) {
				bApplyLight = 1;
				// -- Int3();	//	Curious, did fvi detect intersection with wall containing vertex?
			}
		}
		Lighting_cache [((nSegment << LIGHTING_CACHE_SHIFT) ^ vertnum) & (LIGHTING_CACHE_SIZE-1)] = bApplyLight + (gameData.app.nFrameCount << 1);
		return bApplyLight;
	} else {
Cache_hits++;
		return cache_vis;
	}	
}

#define	HEADLIGHT_CONE_DOT	(F1_0*9/10)
#define	HEADLIGHT_SCALE		(F1_0*10)

// ----------------------------------------------------------------------------------------------

void InitDynColoring (void)
{
if (!gameOpts->render.bDynLighting && bInitDynColoring) {
	bInitDynColoring = 0;
	memset (bGotDynColor, 0, sizeof (bGotDynColor));
	}
bGotGlobalDynColor = 0;
bStartDynColoring = 0;
}

// ----------------------------------------------------------------------------------------------

void SetDynColor (tRgbColorf *color, tRgbColorf *pDynColor, int vertnum, char *pbGotDynColor, int bForce)
{
if (gameOpts->render.bDynLighting)
	return;
if (!color)
	return;
if (!bForce && (color->red == 1.0) && (color->green == 1.0) && (color->blue == 1.0))
	return;
if (bStartDynColoring) {
	InitDynColoring ();
	}
if (!pDynColor) {
	SetDynColor (color, &globalDynColor, 0, &bGotGlobalDynColor, bForce);
	pDynColor = dynamicColor + vertnum;
	pbGotDynColor = bGotDynColor + vertnum;
	}
if (*pbGotDynColor) {
	pDynColor->red = (pDynColor->red + color->red) / 2;
	pDynColor->green = (pDynColor->green + color->green) / 2;
	pDynColor->blue = (pDynColor->blue + color->blue) / 2;
	}
else {
	memcpy (pDynColor, color, sizeof (tRgbColorf));
	*pbGotDynColor = 1;
	}
}

// ----------------------------------------------------------------------------------------------

void ApplyLight(
	fix			xObjIntensity, 
	int			obj_seg, 
	vmsVector	*obj_pos, 
	int			nRenderVertices, 
	short			*renderVertices, 
	int			nObject,
	tRgbColorf	*color)
{
	int			vv, bUseColor, bForceColor;
	int			vertnum;
	int			bApplyLight;
	int			bDarkness = IsMultiGame && EGI_FLAG (bDarkness, 0, 0);
	vmsVector	*vertpos;
	fix			dist, xOrigIntensity = xObjIntensity;
	tObject		*objP = gameData.objs.objects + nObject;
	tPlayer		*playerP = gameData.multi.players + objP->id;

if (gameStates.render.bHaveDynLights && gameOpts->render.bDynLighting) {
	if (objP->nType == OBJ_PLAYER) {
		if (!bDarkness || EGI_FLAG (bHeadLights, 0, 0)) {
			if (!(playerP->flags & PLAYER_FLAGS_HEADLIGHT_ON)) 
				RemoveOglHeadLight (objP);
			else if (gameData.render.lights.dynamic.nHeadLights [objP->id] < 0)
				gameData.render.lights.dynamic.nHeadLights [objP->id] = AddOglHeadLight (objP);
			}
		else {
			if (playerP->flags & PLAYER_FLAGS_HEADLIGHT_ON) {
				playerP->flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
				HUDInitMessage (TXT_NO_HEADLIGHTS);
				}
			}
		if (bDarkness)
			return;
		}
	else if ((objP->nType == OBJ_POWERUP) && bDarkness && !EGI_FLAG (bPowerupLights, 0, 0))
		return;
	AddDynLight (color, xObjIntensity / 4, -1, -1, nObject);
	return;
	}
if (xObjIntensity) {
	fix	obji_64 = xObjIntensity*64;
	
	if (bDarkness) {
		if (objP->nType == OBJ_PLAYER)
			xObjIntensity = 0;
		else if ((objP->nType == OBJ_POWERUP) && !EGI_FLAG (bPowerupLights, 0, 0)) 
			xObjIntensity = 0;
		}
	bUseColor = (color != NULL); //&& (color->red < 1.0 || color->green < 1.0 || color->blue < 1.0);
	bForceColor = 0;
	// for pretty dim sources, only process vertices in tObject's own tSegment.
	//	12/04/95, MK, markers only cast light in own tSegment.
	if ((abs(obji_64) <= F1_0*8) || (objP->nType == OBJ_MARKER)) {
		short *vp = gameData.segs.segments [obj_seg].verts;

		for (vv = 0; vv < MAX_VERTICES_PER_SEGMENT; vv++) {

			vertnum = vp [vv];
#if !FLICKERFIX
			if (/*(gameOpts->render.color.bAmbientLight && color) ||*/ 
				 ((vertnum ^ gameData.app.nFrameCount) & 1))
#endif
			{
				vertpos = gameData.segs.vertices+vertnum;
				dist = VmVecDistQuick (obj_pos, vertpos) / 4;
				dist = FixMul(dist, dist);
				if (dist < abs (obji_64)) {
					if (dist < MIN_LIGHT_DIST)
						dist = MIN_LIGHT_DIST;

					dynamicLight [vertnum] += FixDiv (xObjIntensity, dist);
					if (bUseColor)
						SetDynColor (color, NULL, vertnum, NULL, 0);
					}
				}
			}
		}
	else {
		int	headlightShift = 0;
		fix	maxHeadlightDist = F1_0*200;

		if (objP->nType == OBJ_PLAYER)
			if (gameStates.render.bHeadlightOn = (gameData.multi.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
				headlightShift = 3;
				if (color) {
					bUseColor = bForceColor = 1;
					color->red = color->green = color->blue = 1.0;
					}
				if (objP->id != gameData.multi.nLocalPlayer) {
					vmsVector	tvec;
					fvi_query	fq;
					fvi_info		hit_data;
					int			fate;

					VmVecScaleAdd(&tvec, obj_pos, &objP->position.mOrient.fVec, F1_0*200);

					fq.startSeg			= objP->position.nSegment;
					fq.p0					= obj_pos;
					fq.p1					= &tvec;
					fq.rad				= 0;
					fq.thisObjNum		= nObject;
					fq.ignoreObjList	= NULL;
					fq.flags				= FQ_TRANSWALL;

					fate = FindVectorIntersection(&fq, &hit_data);
					if (fate != HIT_NONE) {
						VmVecSub(&tvec, &hit_data.hit.vPoint, obj_pos);
						maxHeadlightDist = VmVecMagQuick(&tvec) + F1_0*4;
					}
				}
			}
		// -- for (vv=gameData.app.nFrameCount&1; vv<nRenderVertices; vv+=2) {
		for (vv = 0; vv < nRenderVertices; vv++) {

			vertnum = renderVertices [vv];
#if FLICKERFIX == 0
			if (/*(gameOpts->render.color.bAmbientLight && color) ||*/ ((vertnum ^ gameData.app.nFrameCount) & 1))
#endif
			{
				vertpos = gameData.segs.vertices + vertnum;
				dist = VmVecDistQuick (obj_pos, vertpos);
				bApplyLight = 0;

				if ((dist >> headlightShift) < abs(obji_64)) {

					if (dist < MIN_LIGHT_DIST)
						dist = MIN_LIGHT_DIST;
					bApplyLight = 1;
					if (bApplyLight) {
						if (bUseColor)
							SetDynColor (color, NULL, vertnum, NULL, bForceColor);
						if (!headlightShift) 
							dynamicLight [vertnum] += FixDiv(xObjIntensity, dist);
						else {
							fix			dot, maxDot;
							int			spotSize = bDarkness ? 2 << (3 - extraGameInfo [1].nSpotSize) : 1;
							vmsVector	vecToPoint;

							VmVecSub(&vecToPoint, vertpos, obj_pos);
							VmVecNormalizeQuick(&vecToPoint);		//	MK, Optimization note: You compute distance about 15 lines up, this is partially redundant
							dot = VmVecDot(&vecToPoint, &objP->position.mOrient.fVec);
							if (bDarkness)
								maxDot = F1_0 / spotSize;
							else
								maxDot = F1_0 / 2;
							if (dot < maxDot)
								dynamicLight [vertnum] += FixDiv(xOrigIntensity, FixMul (HEADLIGHT_SCALE, dist));	//	Do the normal thing, but darken around headlight.
							else if (!(gameData.app.nGameMode & GM_MULTI) || (dist < maxHeadlightDist))
								dynamicLight [vertnum] += FixMul(FixMul (dot, dot), xOrigIntensity) / (8 * spotSize);
							}
						}
					}
				}
			}
		}
	}
}

#define	FLASH_LEN_FIXED_SECONDS	(F1_0/3)
#define	FLASH_SCALE					(3*F1_0/FLASH_LEN_FIXED_SECONDS)

// ----------------------------------------------------------------------------------------------

void CastMuzzleFlashLight (int nRenderVertices, short *renderVertices)
{
	int	i;
	short	time_since_flash;
	fix currentTime = TimerGetFixedSeconds();

	for (i=0; i<MUZZLE_QUEUE_MAX; i++) {
		if (gameData.muzzle.info [i].createTime) {
			time_since_flash = currentTime - gameData.muzzle.info [i].createTime;
			if (time_since_flash < FLASH_LEN_FIXED_SECONDS)
				ApplyLight ((FLASH_LEN_FIXED_SECONDS - time_since_flash) * FLASH_SCALE, 
								gameData.muzzle.info [i].nSegment, &gameData.muzzle.info [i].pos, 
								nRenderVertices, renderVertices, -1, NULL);
			else
				gameData.muzzle.info [i].createTime = 0;		// turn off this muzzle flash
		}
	}
}

//	Translation table to make flares flicker at different rates
fix	objLightXlat [16] =
	{0x1234, 0x3321, 0x2468, 0x1735,
	 0x0123, 0x19af, 0x3f03, 0x232a,
	 0x2123, 0x39af, 0x0f03, 0x132a,
	 0x3123, 0x29af, 0x1f03, 0x032a};

//	Flag array of gameData.objs.objects lit last frame.  Guaranteed to process this frame if lit last frame.
sbyte   lightingObjects [MAX_OBJECTS];

#define	MAX_HEADLIGHTS	8
tObject	*Headlights [MAX_HEADLIGHTS];
int		Num_headlights;

// ---------------------------------------------------------

fix ComputeLightIntensity (int nObject, tRgbColorf *color, char *pbGotColor)
{
	tObject		*objP = gameData.objs.objects+nObject;
	int			objtype = objP->nType;
   fix			hoardlight, s;

	static tRgbColorf powerupColors [9] = {
		{0,1,0}, {1, 0.8f, 0}, {0,0,1}, {1,1,1}, {0,0,1}, {1,0,0}, {1, 0.8f, 0}, {0,1,0}, {1, 0.8f, 0}
	};

color->red =
color->green =
color->blue = 1.0;
*pbGotColor = 0;
switch (objtype) {
	case OBJ_PLAYER:
		 if (gameData.multi.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT_ON) {
			if (Num_headlights < MAX_HEADLIGHTS)
				Headlights [Num_headlights++] = objP;
			return HEADLIGHT_SCALE;
			}
		 else if ((gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) && gameData.multi.players [objP->id].secondaryAmmo [PROXIMITY_INDEX]) {
		
		// If hoard game and tPlayer, add extra light based on how many orbs you have
		// Pulse as well.

		  	hoardlight=i2f(gameData.multi.players [objP->id].secondaryAmmo [PROXIMITY_INDEX])/2; //i2f(12);
			hoardlight++;
		   FixSinCos ((gameData.time.xGame/2) & 0xFFFF,&s,NULL); // probably a bad way to do it
			s+=F1_0; 
			s>>=1;
			hoardlight=FixMul (s,hoardlight);
		   return (hoardlight);
		  }
		else if (objP->id == gameData.multi.nLocalPlayer) {
			return max(VmVecMagQuick(&player_thrust)/4, F1_0*2) + F1_0/2;
			}
		else {
			return max(VmVecMagQuick(&objP->mType.physInfo.thrust)/4, F1_0*2) + F1_0/2;
			}
		break;

	case OBJ_FIREBALL:
		if ((objP->id != 0xff) && (objP->renderType != RT_THRUSTER)) {
			tVideoClip *vcP = gameData.eff.vClips [0] + objP->id;
			fix xLight = vcP->lightValue;
#if 0
			int i, j;
			grsBitmap *bmP;
			color->red =
			color->green =
			color->blue = 0.0f;
			for (i = j = 0; i < vcP->nFrameCount; i++) {
				bmP = gameData.pig.tex.pBitmaps + vcP->frames [i].index;
				if (bmP->bm_avgRGB.red + bmP->bm_avgRGB.green + bmP->bm_avgRGB.blue == 0)
					if (!BitmapColor (bmP, bmP->bm_texBuf))
						continue;
				color->red += (float) bmP->bm_avgRGB.red / 255.0f;
				color->green += (float) bmP->bm_avgRGB.green / 255.0f;
				color->blue += (float) bmP->bm_avgRGB.blue / 255.0f;
				j++;
				}
			if (j) {
				color->red /= j;
				color->green /= j;
				color->blue /= j;
				}
			else 
#endif
				{
				color->red = 0.75f;
				color->green = 0.15f;
				color->blue = 0.0f;
				}
			*pbGotColor = 1;
#if 0
			if (objP->renderType != RT_THRUSTER)
				xLight /= 8;
#endif
			if (objP->lifeleft < F1_0*4)
				return FixMul (FixDiv(objP->lifeleft, 
								   gameData.eff.vClips [0][objP->id].xTotalTime), xLight);
			else
				return xLight;
			}
		else
			 return 0;
		break;

	case OBJ_ROBOT:
		return F1_0*gameData.bots.pInfo [objP->id].lightcast;
		break;

	case OBJ_WEAPON: {
		fix tval = gameData.weapons.info [objP->id].light;
		*color = gameData.weapons.color [objP->id];
		*pbGotColor = 1;
		if (gameData.app.nGameMode & GM_MULTI)
			if (objP->id == OMEGA_ID)
				if (d_rand() > 8192)
					return 0;		//	3/4 of time, omega blobs will cast 0 light!
		if (objP->id == FLARE_ID)
			return 2* (min(tval, objP->lifeleft) + ((gameData.time.xGame ^ objLightXlat [nObject & 0x0f]) & 0x3fff));
		else
			return tval;
	}

	case OBJ_MARKER: {
		fix	lightval = objP->lifeleft;
		lightval &= 0xffff;
		lightval = 8 * abs(F1_0/2 - lightval);
		if (objP->lifeleft < F1_0*1000)
			objP->lifeleft += F1_0;	//	Make sure this tObject doesn't go out.
		color->red = 0.1f;
		color->green = 1.0f;
		color->blue = 0.1f;
		*pbGotColor = 1;
		return lightval;
	}

	case OBJ_POWERUP:
		if (objP->id < 9) {
			*color = powerupColors [objP->id];
			*pbGotColor = 1;
			}
		return gameData.objs.pwrUp.info [objP->id].light;
		break;

	case OBJ_DEBRIS:
		return F1_0/4;
		break;

	case OBJ_LIGHT:
		return objP->cType.lightInfo.intensity;
		break;
	default:
		return 0;
		break;
	}
}

// ----------------------------------------------------------------------------------------------

void SetDynamicLight (void)
{
	int			vv;
	int			nObject, vertnum, nSegment;
	int			nRenderVertices;
	short			renderVertices [MAX_VERTICES];
	sbyte			render_vertexFlags [MAX_VERTICES];
	int			render_seg, v;
	sbyte			newLightingObjects [MAX_OBJECTS];
	char			bGotColor, bKeepDynColoring = 0;
	tObject		*objP;
	vmsVector	*objPos;
	fix			xObjIntensity;
	tRgbColorf	color;

	Num_headlights = 0;

if (!gameOpts->render.bDynamicLight)
	return;

memset(render_vertexFlags, 0, gameData.segs.nLastVertex+1);
bStartDynColoring = 1;
if (bInitDynColoring) {
	InitDynColoring ();
	}

//	Create list of vertices that need to be looked at for setting of ambient light.
nRenderVertices = 0;
if (!gameOpts->render.bDynLighting) {
	for (render_seg=0; render_seg<nRenderSegs; render_seg++) {
		nSegment = nRenderList [render_seg];
		if (nSegment != -1) {
			short	*vp = gameData.segs.segments [nSegment].verts;
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++) {
				int	vnum = vp [v];
				if (vnum<0 || vnum>gameData.segs.nLastVertex) {
					Int3();		//invalid vertex number
					continue;	//ignore it, and go on to next one
					}
				if (!render_vertexFlags [vnum]) {
					render_vertexFlags [vnum] = 1;
					renderVertices [nRenderVertices++] = vnum;
					}
				}
			}
		}

	for (vv=0; vv<nRenderVertices; vv++) {
		vertnum = renderVertices [vv];
		Assert(vertnum >= 0 && vertnum <= gameData.segs.nLastVertex);
#if FLICKERFIX == 0
		if ((vertnum ^ gameData.app.nFrameCount) & 1)
#endif
			{
			dynamicLight [vertnum] = 0;
			bGotDynColor [vertnum] = 0;
			memset (dynamicColor + vertnum, 0, sizeof (*dynamicColor));
			}
		}
	}
CastMuzzleFlashLight (nRenderVertices, renderVertices);

memset (newLightingObjects, 0, sizeof (newLightingObjects));

//	July 5, 1995: New faster dynamic lighting code.  About 5% faster on the PC (un-optimized).
//	Only objects which are in rendered segments cast dynamic light.  We might want to extend this
//	one or two segments if we notice light changing as gameData.objs.objects go offscreen.  I couldn't see any
//	serious visual degradation.  In fact, I could see no humorous degradation, either. --MK
for (render_seg=0; render_seg < nRenderSegs; render_seg++) {
	nSegment = nRenderList [render_seg];
	nObject = gameData.segs.segments [nSegment].objects;

	while (nObject != -1) {
		objP = gameData.objs.objects + nObject;
		objPos = &objP->position.vPos;

		if (objP->nType == OBJ_FIREBALL)
			objP = objP;
		xObjIntensity = ComputeLightIntensity (nObject, &color, &bGotColor);
		if (bGotColor)
			bKeepDynColoring = 1;
		if (xObjIntensity) {
			ApplyLight (xObjIntensity, objP->position.nSegment, objPos, nRenderVertices, renderVertices, OBJ_IDX (objP), &color);
			newLightingObjects [nObject] = 1;
			}
		nObject = objP->next;
		}
	}

//	Now, process all lights from last frame which haven't been processed this frame.
for (nObject = 0; nObject <= gameData.objs.nLastObject; nObject++) {
	//	In multiplayer games, process even unprocessed gameData.objs.objects every 4th frame, else don't know about tPlayer sneaking up.
	if ((lightingObjects [nObject]) || 
		 ((gameData.app.nGameMode & GM_MULTI) && (((nObject ^ gameData.app.nFrameCount) & 3) == 0))) {
		if (newLightingObjects [nObject])
			//	Not lit last frame, so we don't need to light it.  (Already lit if casting light this frame.)
			//	But copy value from newLightingObjects to update lightingObjects array.
			lightingObjects [nObject] = newLightingObjects [nObject];
		else {
			//	Lit last frame, but not this frame.  Get intensity...
			objP = gameData.objs.objects + nObject;
			objPos = &objP->position.vPos;

			xObjIntensity = ComputeLightIntensity (nObject, &color, &bGotColor);
			if (bGotColor)
				bKeepDynColoring = 1;
			if (xObjIntensity) {
				ApplyLight (xObjIntensity, objP->position.nSegment, objPos, nRenderVertices, renderVertices, nObject, 
								bGotColor ? &color : NULL);
				lightingObjects [nObject] = 1;
				} 
			else
				lightingObjects [nObject] = 0;
			}
		} 
	}
if (!bKeepDynColoring)
	InitDynColoring ();
}

// ---------------------------------------------------------

void toggle_headlight_active()
{
	if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT) {
		gameData.multi.players [gameData.multi.nLocalPlayer].flags ^= PLAYER_FLAGS_HEADLIGHT_ON;			
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendFlags((char) gameData.multi.nLocalPlayer);		
	}
}

// ---------------------------------------------------------

#define HEADLIGHT_BOOST_SCALE 8		//how much to scale light when have headlight boost

fix	Beam_brightness = (F1_0/2);	//global saying how bright the light beam is

#define MAX_DIST_LOG	6							//log(MAX_DIST-expressed-as-integer)
#define MAX_DIST		(f1_0<<MAX_DIST_LOG)	//no light beyond this dist

fix ComputeHeadlightLightOnObject(tObject *objP)
{
	int	i;
	fix	light;

	//	Let's just illuminate players and robots for speed reasons, ok?
	if ((objP->nType != OBJ_ROBOT) && (objP->nType	!= OBJ_PLAYER))
		return 0;

	light = 0;

	for (i=0; i<Num_headlights; i++) {
		fix			dot, dist;
		vmsVector	vecToObj;
		tObject		*lightObjP;

		lightObjP = Headlights [i];

		VmVecSub(&vecToObj, &objP->position.vPos, &lightObjP->position.vPos);
		dist = VmVecNormalizeQuick(&vecToObj);
		if (dist > 0) {
			dot = VmVecDot(&lightObjP->position.mOrient.fVec, &vecToObj);

			if (dot < F1_0/2)
				light += FixDiv(HEADLIGHT_SCALE, FixMul(HEADLIGHT_SCALE, dist));	//	Do the normal thing, but darken around headlight.
			else
				light += FixMul(FixMul(dot, dot), HEADLIGHT_SCALE)/8;
		}
	}

	return light;
}


// -- Unused -- //Compute the lighting from the headlight for a given vertex on a face.
// -- Unused -- //Takes:
// -- Unused -- //  point - the 3d coords of the point
// -- Unused -- //  face_light - a scale factor derived from the surface normal of the face
// -- Unused -- //If no surface normal effect is wanted, pass F1_0 for face_light
// -- Unused -- fix compute_headlight_light(vmsVector *point,fix face_light)
// -- Unused -- {
// -- Unused -- 	fix light;
// -- Unused -- 	int use_beam = 0;		//flag for beam effect
// -- Unused --
// -- Unused -- 	light = Beam_brightness;
// -- Unused --
// -- Unused -- 	if ((gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT) && (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON) && gameData.objs.viewer==&gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].nObject] && gameData.multi.players [gameData.multi.nLocalPlayer].energy > 0) {
// -- Unused -- 		light *= HEADLIGHT_BOOST_SCALE;
// -- Unused -- 		use_beam = 1;	//give us beam effect
// -- Unused -- 	}
// -- Unused --
// -- Unused -- 	if (light) {				//if no beam, don't bother with the rest of this
// -- Unused -- 		fix pointDist;
// -- Unused --
// -- Unused -- 		pointDist = VmVecMagQuick(point);
// -- Unused --
// -- Unused -- 		if (pointDist >= MAX_DIST)
// -- Unused --
// -- Unused -- 			light = 0;
// -- Unused --
// -- Unused -- 		else {
// -- Unused -- 			fix dist_scale,face_scale;
// -- Unused --
// -- Unused -- 			dist_scale = (MAX_DIST - pointDist) >> MAX_DIST_LOG;
// -- Unused -- 			light = FixMul(light,dist_scale);
// -- Unused --
// -- Unused -- 			if (face_light < 0)
// -- Unused -- 				face_light = 0;
// -- Unused --
// -- Unused -- 			face_scale = f1_0/4 + face_light/2;
// -- Unused -- 			light = FixMul(light,face_scale);
// -- Unused --
// -- Unused -- 			if (use_beam) {
// -- Unused -- 				fix beam_scale;
// -- Unused --
// -- Unused -- 				if (face_light > f1_0*3/4 && point->z > i2f(12)) {
// -- Unused -- 					beam_scale = FixDiv(point->z,pointDist);
// -- Unused -- 					beam_scale = FixMul(beam_scale,beam_scale);	//square it
// -- Unused -- 					light = FixMul(light,beam_scale);
// -- Unused -- 				}
// -- Unused -- 			}
// -- Unused -- 		}
// -- Unused -- 	}
// -- Unused --
// -- Unused -- 	return light;
// -- Unused -- }

// ----------------------------------------------------------------------------------------------
//compute the average dynamic light in a tSegment.  Takes the tSegment number
fix ComputeSegDynamicLight(int nSegment)
{
short *verts = gameData.segs.segments [nSegment].verts;
fix sum = dynamicLight [*verts++];
sum += dynamicLight [*verts++];
sum += dynamicLight [*verts++];
sum += dynamicLight [*verts++];
sum += dynamicLight [*verts++];
sum += dynamicLight [*verts++];
sum += dynamicLight [*verts++];
sum += dynamicLight [*verts];
return sum >> 3;
}

// ----------------------------------------------------------------------------------------------
fix object_light [MAX_OBJECTS];
int object_sig [MAX_OBJECTS];
tObject *old_viewer;
int reset_lighting_hack;

#define LIGHT_RATE i2f(4)		//how fast the light ramps up

void StartLightingFrame(tObject *viewer)
{
reset_lighting_hack = (viewer != old_viewer);
old_viewer = viewer;
}

// ----------------------------------------------------------------------------------------------
//compute the lighting for an tObject.  Takes a pointer to the tObject,
//and possibly a rotated 3d point.  If the point isn't specified, the
//tObject's center point is rotated.
fix ComputeObjectLight(tObject *objP,vmsVector *rotated_pnt)
{
	fix light;
	g3sPoint objpnt;
	int nObject = OBJ_IDX (objP);

	if (!rotated_pnt) {
		G3TransformAndEncodePoint (&objpnt, &objP->position.vPos);
		rotated_pnt = &objpnt.p3_vec;
	}
	//First, get static light for this tSegment
	light = gameData.segs.segment2s [objP->position.nSegment].static_light;
	//return light;
	//Now, maybe return different value to smooth transitions
	if (!reset_lighting_hack && (object_sig [nObject] == objP->nSignature)) {
		fix xDeltaLight, xFrameDelta;

		xDeltaLight = light - object_light [nObject];
		xFrameDelta = FixMul(LIGHT_RATE, gameData.time.xFrame);
		if (abs (xDeltaLight) <= xFrameDelta)
			object_light [nObject] = light;		//we've hit the goal
		else
			if (xDeltaLight < 0)
				light = object_light [nObject] -= xFrameDelta;
			else
				light = object_light [nObject] += xFrameDelta;
	}
	else {		//new tObject, initialize
		object_sig [nObject] = objP->nSignature;
		object_light [nObject] = light;
	}
	//Next, add in headlight on this tObject
	// -- Matt code: light += compute_headlight_light(rotated_pnt,f1_0);
	light += ComputeHeadlightLightOnObject (objP);
	//Finally, add in dynamic light for this tSegment
	light += ComputeSegDynamicLight (objP->position.nSegment);
	return light;
}

// ----------------------------------------------------------------------------------------------

void ComputeEngineGlow (tObject *objP, fix *engine_glowValue)
{
engine_glowValue [0] = f1_0/5;
if (objP->movementType == MT_PHYSICS) {
	if ((objP->nType==OBJ_PLAYER) && (objP->mType.physInfo.flags & PF_USES_THRUST) && (objP->id==gameData.multi.nLocalPlayer)) {
		fix thrust_mag = VmVecMagQuick(&objP->mType.physInfo.thrust);
		engine_glowValue [0] += (FixDiv(thrust_mag,gameData.pig.ship.player->max_thrust)*4)/5;
	}
	else {
		fix speed = VmVecMagQuick(&objP->mType.physInfo.velocity);
		engine_glowValue [0] += (FixDiv(speed,MAX_VELOCITY)*3)/5;
		}
	}
//set value for tPlayer headlight
if (objP->nType == OBJ_PLAYER) {
	if ((gameData.multi.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT) && 
		 !gameStates.app.bEndLevelSequence)
		engine_glowValue [1] =  (gameData.multi.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT_ON) ? -2 : -1;
	else
		engine_glowValue [1] = -3;			//don't draw
	}
}

//-----------------------------------------------------------------------------

static int IsFlickeringLight (short nSegment, short nSide)
{
	tFlickeringLight	*flP = gameData.render.lights.flicker.lights;
	int					l;

for (l = gameData.render.lights.flicker.nLights; l; l--, flP++)
	if ((flP->nSegment == nSegment) && (flP->nSide == nSide))
		return 1;
return 0;
}

//-----------------------------------------------------------------------------

static int IsDestructibleLight (short nTexture)
{
if (!nTexture)
	return 0;
else {
	short nClip = gameData.pig.tex.pTMapInfo [nTexture].eclip_num;
	eclip	*ecP = (nClip < 0) ? NULL : gameData.eff.pEffects + nClip;
	short	nDestBM = ecP ? ecP->dest_bm_num : -1;
	ubyte	bOneShot = ecP ? (ecP->flags & EF_ONE_SHOT) != 0 : 0;

	if (nClip == -1) 
		return gameData.pig.tex.pTMapInfo [nTexture].destroyed != -1;
	return (nDestBM != -1) && !bOneShot;
	}
}

//-----------------------------------------------------------------------------

void FlickerLights ()
{
	tFlickeringLight	*flP = gameData.render.lights.flicker.lights;
	int					l;
	tSide					*sideP;
	short					nSegment, nSide;

for (l = 0; l < gameData.render.lights.flicker.nLights; l++, flP++) {
	//make sure this is actually a light
	if (!(WALL_IS_DOORWAY (gameData.segs.segments + flP->nSegment, flP->nSide, NULL) & WID_RENDER_FLAG))
		continue;
	nSegment = flP->nSegment;
	nSide = flP->nSide;
	sideP = gameData.segs.segments [nSegment].sides + nSide;
	if (!(gameData.pig.tex.brightness [sideP->nBaseTex] || 
		   gameData.pig.tex.brightness [sideP->nOvlTex]))
		continue;
	if (flP->timer == 0x80000000)		//disabled
		continue;
	if ((flP->timer -= gameData.time.xFrame) < 0) {
		while (flP->timer < 0)
			flP->timer += flP->delay;
		flP->mask = ((flP->mask & 0x80000000) ? 1 : 0) + (flP->mask << 1);
		if (flP->mask & 1)
			AddLight (nSegment, nSide);
		else
			SubtractLight (nSegment, nSide);
		}
	}
}

//-----------------------------------------------------------------------------
//returns ptr to flickering light structure, or NULL if can't find
tFlickeringLight *FindFlicker (int nSegment,int nSide)
{
	int l;
	tFlickeringLight *flP = gameData.render.lights.flicker.lights;

for (l = 0; l < gameData.render.lights.flicker.nLights; l++, flP++)
	if ((flP->nSegment == nSegment) && (flP->nSide == nSide))	//found it!
		return flP;
return NULL;
}

//-----------------------------------------------------------------------------
//turn flickering off (because light has been turned off)
void DisableFlicker (int nSegment,int nSide)
{
	tFlickeringLight *flP = FindFlicker (nSegment ,nSide);

if (flP)
	flP->timer = 0x80000000;
}

//-----------------------------------------------------------------------------
//turn flickering off (because light has been turned on)
void EnableFlicker (int nSegment,int nSide)
{
	tFlickeringLight *flP = FindFlicker (nSegment, nSide);

if (flP)
	flP->timer = 0;
}


#ifdef EDITOR

//returns 1 if ok, 0 if error
int AddFlicker (int nSegment, int nSide, fix delay, unsigned long mask)
{
	int l;
	tFlickeringLight *flP;

#if TRACE
	//con_printf (CON_DEBUG,"AddFlicker: %d:%d %x %x\n",nSegment,nSide,delay,mask);
#endif
	//see if there's already an entry for this seg/tSide

	flP = gameData.render.lights.flicker.lights;

	for (l = 0; l < gameData.render.lights.flicker.nLights; l++, flP++)
		if ((flP->nSegment == nSegment) && (flP->nSide == nSide))	//found it!
			break;

	if (mask==0) {		//clearing entry
		if (l == gameData.render.lights.flicker.nLights)
			return 0;
		else {
			int i;
			for (i=l;i<gameData.render.lights.flicker.nLights-1;i++)
				gameData.render.lights.flicker.lights[i] = gameData.render.lights.flicker.lights[i+1];
			gameData.render.lights.flicker.nLights--;
			return 1;
		}
	}

	if (l == gameData.render.lights.flicker.nLights) {
		if (gameData.render.lights.flicker.nLights == MAX_FLICKERING_LIGHTS)
			return 0;
		else
			gameData.render.lights.flicker.nLights++;
	}

	flP->nSegment = nSegment;
	flP->nSide = nSide;
	flP->delay = flP->timer = delay;
	flP->mask = mask;

	return 1;
}

#endif

//------------------------------------------------------------------------------

unsigned GetDynLightHandle (void)
{
#if USE_OGL_LIGHTS
	GLint	nMaxLights;

if (gameData.render.lights.dynamic.nLights >= MAX_SEGMENTS)
	return 0xffffffff;
glGetIntegerv (GL_MAX_LIGHTS, &nMaxLights);
if (gameData.render.lights.dynamic.nLights >= nMaxLights)
	return 0xffffffff;
return (unsigned) (GL_LIGHT0 + gameData.render.lights.dynamic.nLights);
#else
return 0xffffffff;
#endif
}

//------------------------------------------------------------------------------

void SetDynLightColor (short nLight, float red, float green, float blue, float brightness)
{
	tDynLight	*pl = gameData.render.lights.dynamic.lights + nLight;
	int			i;

if (pl->nType ? gameOpts->render.color.bGunLight : gameOpts->render.color.bAmbientLight) {
	pl->color.red = red;
	pl->color.green = green;
	pl->color.blue = blue;
	}
else
	pl->color.red = 
	pl->color.green =
	pl->color.blue = 1.0f;
pl->brightness = brightness;
pl->fSpecular.v [0] = red;
pl->fSpecular.v [1] = green;
pl->fSpecular.v [2] = blue;
for (i = 0; i < 3; i++) {
#if USE_OGL_LIGHTS
	pl->fAmbient.v [i] = pl->fDiffuse [i] * 0.01f;
	pl->fDiffuse.v [i] = 
#endif
	pl->fEmissive.v [i] = pl->fSpecular.v [i] * brightness;
	}
// light alphas
#if USE_OGL_LIGHTS
pl->fAmbient.v [3] = 1.0f;
pl->fDiffuse.v [3] = 1.0f;
pl->fSpecular.v [3] = 1.0f;
glLightfv (pl->handle, GL_AMBIENT, pl->fAmbient);
glLightfv (pl->handle, GL_DIFFUSE, pl->fDiffuse);
glLightfv (pl->handle, GL_SPECULAR, pl->fSpecular);
glLightf (pl->handle, GL_SPOT_EXPONENT, 0.0f);
#endif
}

// ----------------------------------------------------------------------------------------------

void SetDynLightPos (short nObject)
{
if (gameStates.render.bHaveDynLights && gameOpts->render.bDynLighting) {
	int	nLight = gameData.render.lights.dynamic.owners [nObject];

	if (nLight >= 0)
		gameData.render.lights.dynamic.lights [nLight].vPos = gameData.objs.objects [nObject].position.vPos;
	}
}

//------------------------------------------------------------------------------

short FindDynLight (short nSegment, short nSide, short nObject)
{
if (gameOpts->render.bDynLighting) {
		tDynLight	*pl = gameData.render.lights.dynamic.lights;
		short			i;

	if (nObject >= 0)
		return gameData.render.lights.dynamic.owners [nObject];
	if (nSegment >= 0)
		for (i = 0; i < gameData.render.lights.dynamic.nLights; i++, pl++)
			if ((pl->nSegment == nSegment) && (pl->nSide == nSide))
				return i;
	}
return -1;
}

//------------------------------------------------------------------------------

short UpdateDynLight (tRgbColorf *pc, float brightness, short nSegment, short nSide, short nObject)
{
	short	nLight = FindDynLight (nSegment, nSide, nObject);
	
if (nLight >= 0) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;
	if (!pc)
		pc = &pl->color;
	if ((pl->brightness != brightness) || 
		 (pl->color.red != pc->red) || (pl->color.green != pc->green) || (pl->color.blue != pc->blue))
		SetDynLightColor (nLight, pc->red, pc->green, pc->blue, brightness);
	}
return nLight;
}

//------------------------------------------------------------------------------

int LastEnabledDynLight (void)
{
	int	i = gameData.render.lights.dynamic.nLights;

while (i)
	if (gameData.render.lights.dynamic.lights [--i].bState)
		return i;
return -1;
}

//------------------------------------------------------------------------------

void RefreshDynLight (tDynLight *pl)
{
#if USE_OGL_LIGHTS
glLightfv (pl->handle, GL_AMBIENT, pl->fAmbient);
glLightfv (pl->handle, GL_DIFFUSE, pl->fDiffuse);
glLightfv (pl->handle, GL_SPECULAR, pl->fSpecular);
glLightf (pl->handle, GL_CONSTANT_ATTENUATION, pl->fAttenuation [0]);
glLightf (pl->handle, GL_LINEAR_ATTENUATION, pl->fAttenuation [1]);
glLightf (pl->handle, GL_QUADRATIC_ATTENUATION, pl->fAttenuation [2]);
#endif
}

//------------------------------------------------------------------------------

void SwapDynLights (tDynLight *pl1, tDynLight *pl2)
{
if (pl1 != pl2) {
		tDynLight	h;

	h = *pl1;
	*pl1 = *pl2;
	*pl2 = h;
#if USE_OGL_LIGHTS
	pl1->handle = (unsigned) (GL_LIGHT0 + (pl1 - gameData.render.lights.dynamic.lights));
	pl2->handle = (unsigned) (GL_LIGHT0 + (pl2 - gameData.render.lights.dynamic.lights));
#endif
	if (pl1->nObject >= 0)
		gameData.render.lights.dynamic.owners [pl1->nObject] = pl1 - gameData.render.lights.dynamic.lights;
	if (pl2->nObject >= 0)
		gameData.render.lights.dynamic.owners [pl2->nObject] = pl2 - gameData.render.lights.dynamic.lights;
	}
}

//------------------------------------------------------------------------------

int ToggleDynLight (short nSegment, short nSide, short nObject, int bState)
{
	short nLight = FindDynLight (nSegment, nSide, nObject);
	
if (nLight >= 0) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;
#if 1
	pl->bOn = bState;
#else
	if (pl->bState != bState) {
		int i = LastEnabledDynLight ();
		if (bState) {
			SwapDynLights (pl, gameData.render.lights.dynamic.lights + i + 1);
			pl = gameData.render.lights.dynamic.lights + i + 1;
#if USE_OGL_LIGHTS
			glEnable (pl->handle);
			RefreshDynLight (pl);
#endif
			}
		else {
			SwapDynLights (pl, gameData.render.lights.dynamic.lights + i);
#if USE_OGL_LIGHTS
			RefreshDynLight (pl);
#endif
			pl = gameData.render.lights.dynamic.lights + i;
#if USE_OGL_LIGHTS
			glDisable (pl->handle);
#endif
			}
		pl->bState = bState;
		}
#endif
	}
return nLight;
}

//------------------------------------------------------------------------------

void RegisterLight (tFaceColor *pc, short nSegment, short nSide)
{
#if 0
if (!pc || pc->index) {
	tLightInfo	*pli = gameData.render.shadows.lightInfo + gameData.render.shadows.nLights++;
#ifdef _DEBUG
	vmsAngVec	a;
#endif
	pli->nIndex = (int) nSegment * 6 + nSide;
	COMPUTE_SIDE_CENTER_I (&pli->pos, nSegment, nSide);
	OOF_VecVms2Gl (pli->glPos, &pli->pos);
	pli->nSegNum = nSegment;
	pli->nSideNum = (ubyte) nSide;
#ifdef _DEBUG
	VmExtractAnglesVector (&a, gameData.segs.segments [nSegment].sides [nSide].normals);
	VmAngles2Matrix (&pli->position.mOrient, &a);
#endif
	}
#endif
}

//------------------------------------------------------------------------------

int AddDynLight (tRgbColorf *pc, fix xBrightness, short nSegment, short nSide, short nObject)
{
	tDynLight	*pl;
	short			h, i;
	fix			rMin, rMax;
#if USE_OGL_LIGHTS
	GLint			nMaxLights;
#endif

if (gameData.render.lights.dynamic.nLights == 86)
gameData.render.lights.dynamic.nLights = gameData.render.lights.dynamic.nLights;
if (0 <= (h = UpdateDynLight (pc, f2fl (xBrightness), nSegment, nSide, nObject)))
	return h;
if (!pc)
	return -1;
if ((pc->red == 0.0f) && (pc->green == 0.0f) && (pc->blue == 0.0f))
	return -1;
if (gameData.render.lights.dynamic.nLights >= MAX_OGL_LIGHTS) {
	gameStates.render.bHaveDynLights = 0;
	return -1;	//too many lights
	}
#if USE_OGL_LIGHTS
glGetIntegerv (GL_MAX_LIGHTS, &nMaxLights);
if (gameData.render.lights.dynamic.nLights >= MAX_OGL_LIGHTS) {
	gameStates.render.bHaveDynLights = 0;
	return -1;	//too many lights
	}
#endif
i = gameData.render.lights.dynamic.nLights; //LastEnabledDynLight () + 1;
pl = gameData.render.lights.dynamic.lights + i;
#if USE_OGL_LIGHTS
pl->handle = GetDynLightHandle (); 
if (pl->handle == 0xffffffff)
	return -1;
#endif
#if 0
if (i < gameData.render.lights.dynamic.nLights)
	SwapDynLights (pl, gameData.render.lights.dynamic.lights + gameData.render.lights.dynamic.nLights);
#endif
pl->nSegment = nSegment;
pl->nSide = nSide;
pl->nObject = nObject;
pl->bState = 1;
pl->bSpot = 0;
pl->nType = (nObject < 0) ? (nSegment < 0) ? 3 : 0 : 2;
SetDynLightColor (gameData.render.lights.dynamic.nLights, pc->red, pc->green, pc->blue, f2fl (xBrightness));
if (nObject >= 0)
	pl->vPos = gameData.objs.objects [nObject].position.vPos;
else if (nSegment >= 0) {
#if 0
	vmsVector	vOffs;
	tSide			*sideP = gameData.segs.segments [nSegment].sides + nSide;
#endif
	COMPUTE_SIDE_CENTER_I (&pl->vPos, nSegment, nSide);
#if 0
	VmVecAdd (&vOffs, sideP->normals, sideP->normals + 1);
	VmVecScaleFrac (&vOffs, 1, 200);
	VmVecInc (&pl->vPos, &vOffs);
#endif
	}
#if USE_OGL_LIGHTS
#	if 0
pl->fAttenuation [0] = 1.0f / f2fl (xBrightness); //0.5f;
pl->fAttenuation [1] = 0.0f; //pl->fAttenuation [0] / 25.0f; //0.01f;
pl->fAttenuation [2] = pl->fAttenuation [0] / 1000.0f; //0.004f;
#	else
pl->fAttenuation [0] = 0.0f;
pl->fAttenuation [1] = 0.0f; //f2fl (xBrightness) / 10.0f;
pl->fAttenuation [2] = (1.0f / f2fl (xBrightness)) / 625.0f;
#	endif
glEnable (pl->handle);
if (!glIsEnabled (pl->handle)) {
	gameStates.render.bHaveDynLights = 0;
	return -1;
	}
glLightf (pl->handle, GL_CONSTANT_ATTENUATION, pl->fAttenuation [0]);
glLightf (pl->handle, GL_LINEAR_ATTENUATION, pl->fAttenuation [1]);
glLightf (pl->handle, GL_QUADRATIC_ATTENUATION, pl->fAttenuation [2]);
#endif
#if 0
LogErr ("adding light %d,%d\n", 
		  gameData.render.lights.dynamic.nLights, pl - gameData.render.lights.dynamic.lights);
#endif		  
if (nObject >= 0) {
	pl->rad = 0;
	gameData.render.lights.dynamic.owners [nObject] = gameData.render.lights.dynamic.nLights;
	}
else if (nSegment >= 0) {
	int	t = gameData.segs.segments [nSegment].sides [nSide].nOvlTex;

	ComputeSideRads (nSegment, nSide, &rMin, &rMax);
	pl->rad = f2fl ((rMin + rMax) / 20);
	//RegisterLight (NULL, nSegment, nSide);
	pl->bVariable = IsDestructibleLight (t) || IsFlickeringLight (nSegment, nSide);
	}
else
	pl->bVariable = 0;
pl->bOn = 1;
pl->bTransform = 1;
return gameData.render.lights.dynamic.nLights++;
}

//------------------------------------------------------------------------------

void DeleteDynLight (short nLight)
{
if ((nLight >= 0) && (nLight < gameData.render.lights.dynamic.nLights)) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;

	if (!pl->nType) {
		// do not remove static lights, or the nearest lights to tSegment info will get messed up!
		pl->bState = pl->bOn = 0;
		return;
		}
	LogErr ("removing light %d,%d\n", nLight, pl - gameData.render.lights.dynamic.lights);
	// if not removing last light in list, move last light down to the now free list entry
	// and keep the freed light handle thus avoiding gaps in used handles
	if (nLight < --gameData.render.lights.dynamic.nLights) {
		*pl = gameData.render.lights.dynamic.lights [gameData.render.lights.dynamic.nLights];
		if (pl->nObject >= 0)
			gameData.render.lights.dynamic.owners [pl->nObject] = nLight;
#if USE_OGL_LIGHTS
		RefreshDynLight (pl);
#endif
		pl = gameData.render.lights.dynamic.lights + gameData.render.lights.dynamic.nLights;
		}
#if USE_OGL_LIGHTS
	glDisable (pl->handle);
#endif
	}
}

//------------------------------------------------------------------------------

int RemoveDynLight (short nSegment, short nSide, short nObject)
{
	int	nLight = FindDynLight (nSegment, nSide, nObject);

if (nLight < 0)
	return 0;
DeleteDynLight (nLight);
if (nObject >= 0)
	gameData.render.lights.dynamic.owners [nObject] = -1;
return 1;
}

//------------------------------------------------------------------------------

void RemoveDynLights (void)
{
	short	i;

for (i = 0; i < gameData.render.lights.dynamic.nLights; i++)
	DeleteDynLight (i);
}

//------------------------------------------------------------------------------

void SetDynLightMaterial (short nSegment, short nSide, short nObject)
{
	static float fBlack [4] = {0.0f, 0.0f, 0.0f, 1.0f};

	int nLight = FindDynLight (nSegment, nSide, nObject);

if (nLight >= 0) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;
	if (pl->bState) {
		gameData.render.lights.dynamic.material.emissive = *((fVector *) &pl->fEmissive);
		gameData.render.lights.dynamic.material.specular = *((fVector *) &pl->fEmissive);
		gameData.render.lights.dynamic.material.shininess = 96;
		gameData.render.lights.dynamic.material.bValid = 1;
		gameData.render.lights.dynamic.material.nLight = nLight;
		return;
		}
	}	
gameData.render.lights.dynamic.material.bValid = 0;
}

//------------------------------------------------------------------------------

void AddDynLights (void)
{
	int			i, j, t;
	tSegment		*segP;
	tSide			*sideP;
	tFaceColor	*pc;

gameStates.render.bHaveDynLights = 1;
//glEnable (GL_LIGHTING);
if (gameOpts->render.bDynLighting)
	memset (&gameData.render.color.vertices, 0, sizeof (gameData.render.color.vertices));
memset (&gameData.render.color.ambient, 0, sizeof (gameData.render.color.ambient));
memset (&gameData.render.lights.dynamic, 0xff, sizeof (gameData.render.lights.dynamic));
gameData.render.lights.dynamic.nLights = 0;
gameData.render.lights.dynamic.material.bValid = 0;
for (i = 0, segP = gameData.segs.segments; i < gameData.segs.nSegments; i++, segP++) {
	for (j = 0, sideP = segP->sides; j < 6; j++, sideP++) {
#ifdef _DEBUG
		if (i == 218 && j == 2)
			i = i;
#endif
		if ((segP->children [j] >= 0) && !IS_WALL (sideP->nWall))
			continue;
		t = sideP->nBaseTex;
		if (t >= MAX_WALL_TEXTURES) 
			continue;
		pc = gameData.render.color.textures + t;
		if (gameData.pig.tex.brightness [t])
			AddDynLight (&pc->color, gameData.pig.tex.brightness [t], (short) i, (short) j, -1);
		t = sideP->nOvlTex;
		if ((t > 0) && (t < MAX_WALL_TEXTURES) && gameData.pig.tex.brightness [t]) {
			pc = gameData.render.color.textures + t;
			if ((pc->color.red > 0.0f) || (pc->color.green > 0.0f) || (pc->color.blue > 0.0f))
				AddDynLight (&pc->color, gameData.pig.tex.brightness [t], (short) i, (short) j, -1);
			}
		//if (gameData.render.lights.dynamic.nLights)
		//	return;
		if (!gameStates.render.bHaveDynLights) {
			RemoveDynLights ();
			return;
			}
		}
	}
}

//------------------------------------------------------------------------------

void TransformDynLights (int bStatic, int bVariable)
{
	int			i;
	tDynLight	*pl = gameData.render.lights.dynamic.lights;

#if USE_OGL_LIGHTS
OglSetupTransform ();
for (i = 0; i < gameData.render.lights.dynamic.nLights; i++, pl++) {
	if (gameStates.ogl.bUseTransform)
		vPos = pl->vPos;
	else
		G3TransformPoint (&vPos, &pl->vPos);
	fPos [0] = f2fl (vPos.x);
	fPos [1] = f2fl (vPos.y);
	fPos [2] = f2fl (vPos.z);
	glLightfv (pl->handle, GL_POSITION, fPos);
	}
OglResetTransform ();
#else
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights;

gameData.render.lights.dynamic.shader.nLights = 0;
UpdateOglHeadLight ();
for (i = 0; i < gameData.render.lights.dynamic.nLights; i++, pl++) {
	memcpy (&psl->color, &pl->color, sizeof (pl->color));
	psl->vPos = pl->vPos;
	VmsVecToFloat (psl->pos, &pl->vPos);
	if (gameStates.ogl.bUseTransform)
		psl->pos [1] = psl->pos [0];
	else
		G3TransformPointf (psl->pos + 1, psl->pos, 0);
	psl->brightness = pl->brightness;
	if (psl->bSpot = pl->bSpot) {
		VmsVecToFloat (&psl->dir, &pl->vDir);
		if (pl->bTransform && !gameStates.ogl.bUseTransform)
			G3RotatePointf (&psl->dir, &psl->dir, 0);
		psl->spotAngle = pl->spotAngle;
		psl->spotExponent = pl->spotExponent;
		}
	psl->bVariable = pl->bVariable;
	psl->bState = pl->bState && (pl->color.red + pl->color.green + pl->color.blue > 0.0);
	psl->bOn = pl->bOn;
	psl->nType = pl->nType;
	psl->bShadow =
	psl->bExclusive = 0;
	if (psl->bState) {
		if (!bStatic && (pl->nType == 1) && !pl->bVariable)
			psl->bState = 0;
		if (!bVariable && ((pl->nType > 1) || pl->bVariable))
			psl->bState = 0;
		}
	psl->rad = pl->rad;
	gameData.render.lights.dynamic.shader.nLights++;
	psl++;
	}
#	if 0
if (gameData.render.lights.dynamic.shader.nTexHandle)
	glDeleteTextures (1, &gameData.render.lights.dynamic.shader.nTexHandle);
gameData.render.lights.dynamic.shader.nTexHandle = 0;
glGenTextures (1, &gameData.render.lights.dynamic.shader.nTexHandle);
glBindTexture (GL_TEXTURE_2D, gameData.render.lights.dynamic.shader.nTexHandle);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
glTexImage2D (GL_TEXTURE_2D, 0, 4, MAX_OGL_LIGHTS / 64, 64, 1, GL_RGBA,
				  GL_FLOAT, gameData.render.lights.dynamic.shader.lights);
#	endif
#endif
}

//------------------------------------------------------------------------------

void SetNearestVertexLights (int nVertex, ubyte nType, int bStatic, int bVariable)
{
//if (gameOpts->render.bDynLighting) 
	{
	short	*pnl = gameData.render.lights.dynamic.nNearestVertLights [nVertex];
	short	i, j;

	for (i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
		if (gameData.render.lights.dynamic.lights [j].bVariable) {
			if (!(bVariable && gameData.render.lights.dynamic.lights [j].bOn))
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		gameData.render.lights.dynamic.shader.lights [j].nType = nType;
		gameData.render.lights.dynamic.shader.lights [j].bState = 1;
		}
	}
}

//------------------------------------------------------------------------------

void SetNearestStaticLights (int nSegment, ubyte nType)
{
if (gameOpts->render.bDynLighting) {
	short	*pnl = gameData.render.lights.dynamic.nNearestSegLights [nSegment];
	short	i, j;

	for (i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
		gameData.render.lights.dynamic.shader.lights [j].nType = nType;
		}
	}
}

//------------------------------------------------------------------------------

void SetNearestDynamicLights (int nSegment)
{
if (gameOpts->render.bDynLighting) {
	short				i = gameData.render.lights.dynamic.shader.nLights;
	tDynLight		*pl = gameData.render.lights.dynamic.lights + i;
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights + i;
	vmsVector		d, c;
	fix				m;

	COMPUTE_SEGMENT_CENTER_I (&c, nSegment);
	while (i--) {
		psl--;
		pl--;
		if (psl->nType < 2)
			break;
		if (psl->nType == 3)
			psl->bState = 1;
		else {
			VmVecSub (&d, &c, &pl->vPos);
			m = VmVecMag (&d);
			psl->bState = (m <= F1_0 * 125);
			}
		}
	}
}

//------------------------------------------------------------------------------

tFaceColor *AvgSgmColor (int nSegment, vmsVector *pvPos)
{
	tFaceColor	c, *pvc, *psc = gameData.render.color.segments + nSegment;
	short			i, *pv;
	vmsVector	vCenter, vVertex;
	float			d, ds;

if (!gameOpts->render.bDynLighting) {
	psc->index = !gameStates.render.nFrameFlipFlop + 1;
	return psc;
	}
if (psc->index == gameStates.render.nFrameFlipFlop + 1)
	return psc;
if (pvPos) {
	COMPUTE_SEGMENT_CENTER_I (&vCenter, nSegment);
	//G3TransformPoint (&vCenter, &vCenter);
	ds = 0.0f;
	}
else
	ds = 1.0f;
pv = gameData.segs.segments [nSegment].verts;
c.color.red = c.color.green = c.color.blue = 0.0f;
c.index = 0;
for (i = 0; i < 8; i++, pv++) {
	pvc = gameData.render.color.vertices + *pv;
	if (pvc->index == gameStates.render.nFrameFlipFlop + 1) {
		if (pvPos) {
			vVertex = gameData.segs.vertices [*pv];
			//G3TransformPoint (&vVertex, &vVertex);
			d = 2.0f - f2fl (VmVecDist (&vVertex, pvPos)) / f2fl (VmVecDist (&vCenter, &vVertex));
			c.color.red += pvc->color.red * d;
			c.color.green += pvc->color.green * d;
			c.color.blue += pvc->color.blue * d;
			ds += d;
			}
		else {
			c.color.red += pvc->color.red;
			c.color.green += pvc->color.green;
			c.color.blue += pvc->color.blue;
			}
		c.index++;
		}
	}
if (c.index) {
	ds = (float) c.index;
	psc->color.red = c.color.red / ds;
	psc->color.green = c.color.green / ds;
	psc->color.blue = c.color.blue / ds;
	d = psc->color.red;
	if (d < psc->color.green)
		d = psc->color.green;
	if (d < psc->color.blue)
		d = psc->color.blue;
	if (d > 1.0f) {
		psc->color.red /= d;
		psc->color.green /= d;
		psc->color.blue /= d;
		}
	}
else
	psc->color.red = psc->color.green = psc->color.blue = 0.0f;
psc->index = gameStates.render.nFrameFlipFlop + 1;
return psc;
}


//------------------------------------------------------------------------------

int AddOglHeadLight (tObject *objP)
{
	static float spotExps [] = {3.0f, 5.0f, 10.0f};
	static float spotAngles [] = {0.95f, 0.825f, 0.25f};

if (gameOpts->render.bDynLighting) {
		tRgbColorf c = {1.0f, 1.0f, 1.0f};
		tDynLight	*pl;
		int			nLight;

	nLight = AddDynLight (&c, F1_0 * 50, -1, -1, -1);
	if (nLight >= 0) {
		pl = gameData.render.lights.dynamic.lights + nLight;
		pl->nPlayer = objP->id;
		pl->bSpot = 1;
		pl->spotAngle = spotAngles [extraGameInfo [IsMultiGame].nSpotSize];
		pl->spotExponent = spotExps [extraGameInfo [IsMultiGame].nSpotStrength];
		pl->bTransform = 0;
		}
	return nLight;
	}
return -1;
}

//------------------------------------------------------------------------------

void RemoveOglHeadLight (tObject *objP)
{
if (gameOpts->render.bDynLighting) {
	DeleteDynLight (gameData.render.lights.dynamic.nHeadLights [objP->id]);
	gameData.render.lights.dynamic.nHeadLights [objP->id] = -1;
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
	objP = gameData.objs.objects + gameData.multi.players [nPlayer].nObject;
	pl->vPos = objP->position.vPos;
	pl->vDir = objP->position.mOrient.fVec;
	VmVecScaleInc (&pl->vPos, &pl->vDir, objP->size / 3);
	}
}

//------------------------------------------------------------------------------

void ComputeStaticDynLighting ()
{
if (gameOpts->render.bDynLighting || !gameStates.app.bD2XLevel) {
		int				i, bColorize = !(gameOpts->render.bDynLighting || gameStates.app.bD2XLevel);
		tFaceColor		*pf = bColorize ? gameData.render.color.vertices : gameData.render.color.ambient;
		fVector			vVertex;

	gameStates.render.nState = 0;
	TransformDynLights (1, bColorize);
	for (i = 0; i < gameData.segs.nVertices; i++, pf++) {
		VmsVecToFloat (&vVertex, gameData.segs.vertices + i);
		SetNearestVertexLights (i, 1, 1, bColorize);
		G3VertexColor (&gameData.segs.points [i].p3_normal.vNormal, &vVertex, i, pf);
		SetNearestVertexLights (i, 0, 1, bColorize);
		}
	}
}

//------------------------------------------------------------------------------

void CalcDynLightAttenuation (vmsVector *pv)
{
#if !USE_OGL_LIGHTS
	int				i;
	tDynLight		*pl = gameData.render.lights.dynamic.lights;
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights;
	fVector			v, d;
	float				l;

v.p.x = f2fl (pv->p.x);
v.p.y = f2fl (pv->p.y);
v.p.z = f2fl (pv->p.z);
if (!gameStates.ogl.bUseTransform)
	G3TransformPointf (&v, &v, 0);
for (i = gameData.render.lights.dynamic.nLights; i; i--, pl++, psl++) {
	d.p.x = v.p.x - psl->pos [1].p.x;
	d.p.y = v.p.y - psl->pos [1].p.y;
	d.p.z = v.p.z - psl->pos [1].p.z;
	l = (float) (sqrt (d.p.x * d.p.x + d.p.y * d.p.y + d.p.z * d.p.z) / 625.0);
	psl->brightness = l / pl->brightness;
	}
#endif
}

//-------------------------------------------------------------------------

#ifdef _DEBUG

char *lightingVS = "lighting.vert";
char *lightingFS = "lighting.frag";

#else

char *lightingFS =
	"";

char *lightingVS =
	"void LightingVS(void){gl_FragColor=gl_Color;}"
	;

#endif

//-------------------------------------------------------------------------

GLhandleARB lvs = 0; 
GLhandleARB lfs = 0; 

void InitLightingShaders (void)
{
if (!gameOpts->render.bDynLighting)
	gameStates.render.bHaveDynLights = 0;
else {
#if 1
	gameStates.render.bHaveDynLights = 1;
#else
	LogErr ("building lighting shader programs\n");
	DeleteShaderProg (NULL);
	gameStates.render.bHaveDynLights = CreateShaderFunc (NULL, &lvs, &lfs, lightingFS, lightingVS, 1);
	if (!gameStates.render.bHaveDynLights)
		gameOpts->render.bDynLighting = 0;
#endif
	}
}

// ----------------------------------------------------------------------------------------------
//eof
