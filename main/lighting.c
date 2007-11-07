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
#include "fix.h"
#include "vecmat.h"
#include "ogl_defs.h"
#include "ogl_color.h"
#include "ogl_shader.h"
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
#include "lightning.h"
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
#include "input.h"
#include "renderthreads.h"

#define CACHE_LIGHTS 0
#define FLICKERFIX 0
//int	Use_fvi_lighting = 0;

#define	LIGHTING_CACHE_SIZE	4096	//	Must be power of 2!
#define	LIGHTING_FRAME_DELTA	256	//	Recompute cache value every 8 frames.
#define	LIGHTING_CACHE_SHIFT	8
int	Lighting_frame_delta = 1;
int	Lighting_cache [LIGHTING_CACHE_SIZE];
int Cache_hits=0, Cache_lookups=1;
extern vmsVector playerThrust;
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
//	Return true if we think vertex nVertex is visible from tSegment nSegment.
//	If some amount of time has gone by, then recompute, else use cached value.

int LightingCacheVisible (int nVertex, int nSegment, int nObject, vmsVector *vObjPos, int nObjSeg, vmsVector *vVertPos)
{
	int	cache_val, cache_frame, cache_vis;
	cache_val = Lighting_cache [((nSegment << LIGHTING_CACHE_SHIFT) ^ nVertex) & (LIGHTING_CACHE_SIZE-1)];
	cache_frame = cache_val >> 1;
	cache_vis = cache_val & 1;
	Cache_lookups++;
	if ((cache_frame == 0) || (cache_frame + Lighting_frame_delta <= gameData.app.nFrameCount)) {
		int			bApplyLight = 0;
		tVFIQuery	fq;
		tFVIData		hit_data;
		int			nSegment, hitType;
		nSegment = -1;
		#ifdef _DEBUG
		nSegment = FindSegByPoint (vObjPos, nObjSeg, 1);
		if (nSegment == -1) {
			Int3 ();		//	Obj_pos is not in nObjSeg!
			return 0;		//	Done processing this tObject.
		}
		#endif
		fq.p0					= vObjPos;
		fq.startSeg			= nObjSeg;
		fq.p1					= vVertPos;
		fq.radP0				=
		fq.radP1				= 0;
		fq.thisObjNum		= nObject;
		fq.ignoreObjList	= NULL;
		fq.flags				= FQ_TRANSWALL;
		hitType = FindVectorIntersection (&fq, &hit_data);
		// gameData.ai.vHitPos = gameData.ai.hitData.hit.vPoint;
		// gameData.ai.nHitSeg = gameData.ai.hitData.hit_seg;
		if (hitType == HIT_OBJECT)
			Int3 ();	//	Hey, we're not supposed to be checking gameData.objs.objects!
		if (hitType == HIT_NONE)
			bApplyLight = 1;
		else if (hitType == HIT_WALL) {
			fix	distDist;
			distDist = VmVecDistQuick (&hit_data.hit.vPoint, vObjPos);
			if (distDist < F1_0/4) {
				bApplyLight = 1;
				// -- Int3 ();	//	Curious, did fvi detect intersection with tWall containing vertex?
			}
		}
		Lighting_cache [ ((nSegment << LIGHTING_CACHE_SHIFT) ^ nVertex) & (LIGHTING_CACHE_SIZE-1)] = bApplyLight + (gameData.app.nFrameCount << 1);
		return bApplyLight;
	} else {
Cache_hits++;
		return cache_vis;
	}	
}
#define	HEADLIGHT_CONE_DOT	 (F1_0*9/10)
#define	HEADLIGHT_SCALE		 (F1_0*10)

// ----------------------------------------------------------------------------------------------

void InitDynColoring (void)
{
if (!gameOpts->render.bDynLighting && gameData.render.lights.bInitDynColoring) {
	gameData.render.lights.bInitDynColoring = 0;
	memset (gameData.render.lights.bGotDynColor, 0, sizeof (*gameData.render.lights.bGotDynColor) * MAX_VERTICES);
	}
gameData.render.lights.bGotGlobalDynColor = 0;
gameData.render.lights.bStartDynColoring = 0;
}

// ----------------------------------------------------------------------------------------------

void SetDynColor (tRgbaColorf *color, tRgbColorf *pDynColor, int nVertex, char *pbGotDynColor, int bForce)
{
if (0 && gameOpts->render.bDynLighting)
	return;
if (!color)
	return;
#if 1
if (!bForce && (color->red == 1.0) && (color->green == 1.0) && (color->blue == 1.0))
	return;
#endif
if (gameData.render.lights.bStartDynColoring) {
	InitDynColoring ();
	}
if (!pDynColor) {
	SetDynColor (color, &gameData.render.lights.globalDynColor, 0, &gameData.render.lights.bGotGlobalDynColor, bForce);
	pDynColor = gameData.render.lights.dynamicColor + nVertex;
	pbGotDynColor = gameData.render.lights.bGotDynColor + nVertex;
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

void ApplyLight (
	fix			xObjIntensity, 
	int			nObjSeg, 
	vmsVector	*vObjPos, 
	int			nRenderVertices, 
	short			*renderVertexP, 
	int			nObject,
	tRgbaColorf	*color)
{
	int			iVertex, bUseColor, bForceColor;
	int			nVertex;
	int			bApplyLight;
	vmsVector	*vVertPos;
	fix			dist, xOrigIntensity = xObjIntensity;
	tObject		*objP = (nObject < 0) ? NULL : gameData.objs.objects + nObject;
	tPlayer		*playerP = objP ? gameData.multiplayer.players + objP->id : NULL;

if (objP && SHOW_DYN_LIGHT) {
	if (objP->nType == OBJ_PLAYER) {
		if (!gameData.render.vertColor.bDarkness || EGI_FLAG (bHeadLights, 0, 0, 0)) {
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
		if (gameData.render.vertColor.bDarkness)
			return;
		xObjIntensity /= 4;
		}
	else if (objP->nType == OBJ_POWERUP) {
		if (!EGI_FLAG (bPowerupLights, 0, 0, 0)) {
			RemoveDynLight (-1, -1, nObject);
			return;
			}
		xObjIntensity /= 4;
		}
	else if (objP->nType == OBJ_ROBOT)
		xObjIntensity /= 4;
	else if ((objP->nType == OBJ_FIREBALL) || (objP->nType == OBJ_EXPLOSION))
		xObjIntensity /= 2; //10;
	AddDynLight (color, xObjIntensity, -1, -1, nObject, NULL);
	return;
	}
if (xObjIntensity) {
	fix	obji_64 = xObjIntensity * 64;
	
	if (gameData.render.vertColor.bDarkness) {
		if (objP->nType == OBJ_PLAYER)
			xObjIntensity = 0;
		}
	if (objP && (objP->nType == OBJ_POWERUP) && !EGI_FLAG (bPowerupLights, 0, 0, 0)) 
		return;
	bUseColor = (color != NULL); //&& (color->red < 1.0 || color->green < 1.0 || color->blue < 1.0);
	bForceColor = objP && ((objP->nType == OBJ_WEAPON) || (objP->nType == OBJ_FIREBALL) || (objP->nType == OBJ_EXPLOSION));
	// for pretty dim sources, only process vertices in tObject's own tSegment.
	//	12/04/95, MK, markers only cast light in own tSegment.
	if (objP && ((abs (obji_64) <= F1_0 * 8) || (objP->nType == OBJ_MARKER))) {
		short *vp = gameData.segs.segments [nObjSeg].verts;
		for (iVertex = 0; iVertex < MAX_VERTICES_PER_SEGMENT; iVertex++) {
			nVertex = vp [iVertex];
#if !FLICKERFIX
			if ((nVertex ^ gameData.app.nFrameCount) & 1)
#endif
			{
				vVertPos = gameData.segs.vertices + nVertex;
				dist = VmVecDistQuick (vObjPos, vVertPos) / 4;
				dist = FixMul (dist, dist);
				if (dist < abs (obji_64)) {
					if (dist < MIN_LIGHT_DIST)
						dist = MIN_LIGHT_DIST;
					gameData.render.lights.dynamicLight [nVertex] += FixDiv (xObjIntensity, dist);
					if (bUseColor)
						SetDynColor (color, NULL, nVertex, NULL, 0);
					}
				}
			}
		}
	else {
		int	headlightShift = 0;
		fix	maxHeadlightDist = F1_0 * 200;
		if (objP && (objP->nType == OBJ_PLAYER))
			if ((gameStates.render.bHeadlightOn = (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT_ON))) {
				headlightShift = 3;
				if (color) {
					bUseColor = bForceColor = 1;
					color->red = color->green = color->blue = 1.0;
					}
				if (objP->id != gameData.multiplayer.nLocalPlayer) {
					vmsVector	tvec;
					tVFIQuery	fq;
					tFVIData		hit_data;
					int			fate;
					VmVecScaleAdd (&tvec, vObjPos, &objP->position.mOrient.fVec, F1_0*200);
					fq.startSeg			= objP->nSegment;
					fq.p0					= vObjPos;
					fq.p1					= &tvec;
					fq.radP0				=
					fq.radP1				= 0;
					fq.thisObjNum		= nObject;
					fq.ignoreObjList	= NULL;
					fq.flags				= FQ_TRANSWALL;
					fate = FindVectorIntersection (&fq, &hit_data);
					if (fate != HIT_NONE) {
						VmVecSub (&tvec, &hit_data.hit.vPoint, vObjPos);
						maxHeadlightDist = VmVecMagQuick (&tvec) + F1_0*4;
					}
				}
			}
		// -- for (iVertex=gameData.app.nFrameCount&1; iVertex<nRenderVertices; iVertex+=2) {
		for (iVertex = 0; iVertex < nRenderVertices; iVertex++) {
			nVertex = renderVertexP [iVertex];
#ifdef _DEBUG
			if (nVertex == nDbgVertex)
				nVertex = nVertex;
#endif
#if FLICKERFIX == 0
			if ((nVertex ^ gameData.app.nFrameCount) & 1)
#endif
			{
				vVertPos = gameData.segs.vertices + nVertex;
				dist = VmVecDistQuick (vObjPos, vVertPos);
				bApplyLight = 0;
				if ((dist >> headlightShift) < abs (obji_64)) {
					if (dist < MIN_LIGHT_DIST)
						dist = MIN_LIGHT_DIST;
#if 0
					bApplyLight = 1;
					if (bApplyLight) 
#endif
					{
						if (bUseColor)
							SetDynColor (color, NULL, nVertex, NULL, bForceColor);
						if (!headlightShift) 
							gameData.render.lights.dynamicLight [nVertex] += FixDiv (xObjIntensity, dist);
						else {
							fix			dot, maxDot;
							int			spotSize = gameData.render.vertColor.bDarkness ? 2 << (3 - extraGameInfo [1].nSpotSize) : 1;
							vmsVector	vecToPoint;
							VmVecSub (&vecToPoint, vVertPos, vObjPos);
							VmVecNormalizeQuick (&vecToPoint);		//	MK, Optimization note: You compute distance about 15 lines up, this is partially redundant
							dot = VmVecDot (&vecToPoint, &objP->position.mOrient.fVec);
							if (gameData.render.vertColor.bDarkness)
								maxDot = F1_0 / spotSize;
							else
								maxDot = F1_0 / 2;
							if (dot < maxDot)
								gameData.render.lights.dynamicLight [nVertex] += FixDiv (xOrigIntensity, FixMul (HEADLIGHT_SCALE, dist));	//	Do the normal thing, but darken around headlight.
							else if (!IsMultiGame || (dist < maxHeadlightDist))
								gameData.render.lights.dynamicLight [nVertex] += FixMul (FixMul (dot, dot), xOrigIntensity) / 8;//(8 * spotSize);
							}
						}
					}
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------------------------

#define	FLASH_LEN_FIXED_SECONDS	 (F1_0/3)
#define	FLASH_SCALE					 (3*F1_0/FLASH_LEN_FIXED_SECONDS)

void CastMuzzleFlashLight (int nRenderVertices, short *renderVertexP)
{
	int	i;
	short	time_since_flash;
	fix currentTime = TimerGetFixedSeconds ();

for (i = 0; i < MUZZLE_QUEUE_MAX; i++) {
	if (gameData.muzzle.info [i].createTime) {
		time_since_flash = (short) (currentTime - gameData.muzzle.info [i].createTime);
		if (time_since_flash < FLASH_LEN_FIXED_SECONDS)
			ApplyLight ((FLASH_LEN_FIXED_SECONDS - time_since_flash) * FLASH_SCALE, 
							gameData.muzzle.info [i].nSegment, &gameData.muzzle.info [i].pos, 
							nRenderVertices, renderVertexP, -1, NULL);
		else
			gameData.muzzle.info [i].createTime = 0;		// turn off this muzzle flash
		}
	}
}

// ---------------------------------------------------------
//	Translation table to make flares flicker at different rates
fix	objLightXlat [16] =
	{0x1234, 0x3321, 0x2468, 0x1735,
	 0x0123, 0x19af, 0x3f03, 0x232a,
	 0x2123, 0x39af, 0x0f03, 0x132a,
	 0x3123, 0x29af, 0x1f03, 0x032a};
//	Flag array of gameData.objs.objects lit last frame.  Guaranteed to process this frame if lit last frame.
#define	MAX_HEADLIGHTS	8
tObject	*Headlights [MAX_HEADLIGHTS];
int		nHeadLights;

fix ComputeLightIntensity (int nObject, tRgbaColorf *color, char *pbGotColor)
{
	tObject		*objP = gameData.objs.objects + nObject;
	int			nObjType = objP->nType;
   fix			hoardlight, s;
	static tRgbaColorf powerupColors [9] = {
		{0,1,0,1},{1,0.8f,0,1},{0,0,1,1},{1,1,1,1},{0,0,1,1},{1,0,0,1},{1,0.8f,0,1},{0,1,0,1},{1,0.8f,0,1}
	};
	
color->red =
color->green =
color->blue = 1.0;
*pbGotColor = 0;
switch (nObjType) {
	case OBJ_PLAYER:
		*pbGotColor = 1;
		 if (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT_ON) {
			if (nHeadLights < MAX_HEADLIGHTS)
				Headlights [nHeadLights++] = objP;
			return HEADLIGHT_SCALE;
			}
		 else if ((gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) && gameData.multiplayer.players [objP->id].secondaryAmmo [PROXMINE_INDEX]) {
		
		// If hoard game and tPlayer, add extra light based on how many orbs you have
		// Pulse as well.
		  	hoardlight = i2f (gameData.multiplayer.players [objP->id].secondaryAmmo [PROXMINE_INDEX])/2; //i2f (12);
			hoardlight++;
		   FixSinCos ((gameData.time.xGame/2) & 0xFFFF,&s,NULL); // probably a bad way to do it
			s+=F1_0; 
			s>>=1;
			hoardlight = FixMul (s,hoardlight);
		   return (hoardlight);
		  }
		else if (objP->id == gameData.multiplayer.nLocalPlayer) {
			return max (VmVecMagQuick (&playerThrust)/4, F1_0*2) + F1_0/2;
			}
		else {
			return max (VmVecMagQuick (&objP->mType.physInfo.thrust)/4, F1_0*2) + F1_0/2;
			}
		break;

	case OBJ_FIREBALL:
	case OBJ_EXPLOSION:
		if (objP->id == 0xff)
			return 0;
		if ((objP->renderType == RT_THRUSTER) || (objP->renderType == RT_EXPLBLAST) || (objP->renderType == RT_SHRAPNELS)) 
			return 0;
		else {
			tVideoClip *vcP = gameData.eff.vClips [0] + objP->id;
			fix xLight = vcP->lightValue;
			int i, j;
			grsBitmap *bmP;
			if (bmP = BM_OVERRIDE (gameData.pig.tex.pBitmaps + vcP->frames [0].index)) {
				color->red = (float) bmP->bmAvgRGB.red;
				color->green = (float) bmP->bmAvgRGB.green;
				color->blue = (float) bmP->bmAvgRGB.blue;
				*pbGotColor = 1;
				}
			else {
				color->red =
				color->green =
				color->blue = 0.0f;
				for (i = j = 0; i < vcP->nFrameCount; i++) {
					bmP = gameData.pig.tex.pBitmaps + vcP->frames [i].index;
					if (bmP->bmAvgRGB.red + bmP->bmAvgRGB.green + bmP->bmAvgRGB.blue == 0)
						if (!BitmapColor (bmP, bmP->bmTexBuf))
							continue;
					color->red += (float) bmP->bmAvgRGB.red / 255.0f;
					color->green += (float) bmP->bmAvgRGB.green / 255.0f;
					color->blue += (float) bmP->bmAvgRGB.blue / 255.0f;
					j++;
					}
				if (j) {
					color->red /= j;
					color->green /= j;
					color->blue /= j;
					*pbGotColor = 1;
					}
				}
#if 0
			if (objP->renderType != RT_THRUSTER)
				xLight /= 8;
#endif
			if (objP->lifeleft < F1_0*4)
				return FixMul (FixDiv (objP->lifeleft, 
								   gameData.eff.vClips [0][objP->id].xTotalTime), xLight);
			else
				return xLight;
			}
		break;

	case OBJ_ROBOT:
		*pbGotColor = 1;
#if 0//def _DEBUG
		return ROBOTINFO (objP->id).lighting;
#else
		return ROBOTINFO (objP->id).lightcast ? ROBOTINFO (objP->id).lighting ? ROBOTINFO (objP->id).lighting : F1_0 : 0;
#endif
		break;

	case OBJ_WEAPON: {
		fix tval = gameData.weapons.info [objP->id].light;
		if (gameOpts->render.color.bGunLight)
			*color = gameData.weapons.color [objP->id];
		*pbGotColor = 1;
		if (gameData.app.nGameMode & GM_MULTI)
			if (objP->id == OMEGA_ID)
				if (d_rand () > 8192)
					return 0;		//	3/4 of time, omega blobs will cast 0 light!
		if (objP->id == FLARE_ID)
			return 2* (min (tval, objP->lifeleft) + ((gameData.time.xGame ^ objLightXlat [nObject & 0x0f]) & 0x3fff));
		else
			return tval;
		}

	case OBJ_MARKER: {
		fix	lightval = objP->lifeleft;
		lightval &= 0xffff;
		lightval = 8 * abs (F1_0/2 - lightval);
		if (objP->lifeleft < F1_0*1000)
			objP->lifeleft += F1_0;	//	Make sure this tObject doesn't go out.
		color->red = 0.1f;
		color->green = 1.0f;
		color->blue = 0.1f;
		*pbGotColor = 1;
		return lightval;
		}

	case OBJ_POWERUP:
		if (objP->id < 9)
			*color = powerupColors [objP->id];
		*pbGotColor = 1;
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
	int			iVertex, nv;
	int			nObject, nVertex, nSegment;
	int			nRenderVertices;
	int			iRenderSeg, v;
	char			bGotColor, bKeepDynColoring = 0;
	tObject		*objP;
	vmsVector	*objPos;
	fix			xObjIntensity;
	tRgbaColorf	color;
	nHeadLights = 0;

#if 0
if (SHOW_DYN_LIGHT)
	return;
#endif
if (!gameOpts->render.bDynamicLight)
	return;
memset (gameData.render.lights.vertexFlags, 0, gameData.segs.nLastVertex + 1);
gameData.render.vertColor.bDarkness = IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [IsMultiGame].bDarkness;
gameData.render.lights.bStartDynColoring = 1;
if (gameData.render.lights.bInitDynColoring) {
	InitDynColoring ();
	}
//	Create list of vertices that need to be looked at for setting of ambient light.
nRenderVertices = 0;
if (!gameOpts->render.bDynLighting) {
	for (iRenderSeg = 0; iRenderSeg < gameData.render.mine.nRenderSegs; iRenderSeg++) {
		nSegment = gameData.render.mine.nSegRenderList [iRenderSeg];
		if (nSegment != -1) {
			short	*vp = gameData.segs.segments [nSegment].verts;
			for (v = 0; v < MAX_VERTICES_PER_SEGMENT; v++) {
				nv = vp [v];
				if ((nv < 0) || (nv > gameData.segs.nLastVertex)) {
					Int3 ();		//invalid vertex number
					continue;	//ignore it, and go on to next one
					}
				if (!gameData.render.lights.vertexFlags [nv]) {
					Assert (nRenderVertices < MAX_VERTICES);
					gameData.render.lights.vertexFlags [nv] = 1;
					gameData.render.lights.vertices [nRenderVertices++] = nv;
					}
				}
			}
		}
	for (iVertex = 0; iVertex < nRenderVertices; iVertex++) {
		nVertex = gameData.render.lights.vertices [iVertex];
		Assert (nVertex >= 0 && nVertex <= gameData.segs.nLastVertex);
#if FLICKERFIX == 0
		if ((nVertex ^ gameData.app.nFrameCount) & 1)
#endif
			{
			gameData.render.lights.dynamicLight [nVertex] = 0;
			gameData.render.lights.bGotDynColor [nVertex] = 0;
			memset (gameData.render.lights.dynamicColor + nVertex, 0, sizeof (*gameData.render.lights.dynamicColor));
			}
		}
	}
CastMuzzleFlashLight (nRenderVertices, gameData.render.lights.vertices);
memset (gameData.render.lights.newObjects, 0, sizeof (gameData.render.lights.newObjects));
if (EGI_FLAG (bUseLightnings, 0, 0, 1) && !gameOpts->render.bDynLighting) {
	tLightningLight	*pll;
	for (iRenderSeg = 0; iRenderSeg < gameData.render.mine.nRenderSegs; iRenderSeg++) {
		nSegment = gameData.render.mine.nSegRenderList [iRenderSeg];
		pll = gameData.lightnings.lights + nSegment;
		if (pll->nFrameFlipFlop == gameStates.render.nFrameFlipFlop)
			ApplyLight (pll->nBrightness, nSegment, &pll->vPos, nRenderVertices, gameData.render.lights.vertices, -1, &pll->color);
		}
	}
//	July 5, 1995: New faster dynamic lighting code.  About 5% faster on the PC (un-optimized).
//	Only objects which are in rendered segments cast dynamic light.  We might want to extend this
//	one or two segments if we notice light changing as gameData.objs.objects go offscreen.  I couldn't see any
//	serious visual degradation.  In fact, I could see no humorous degradation, either. --MK
for (nObject = 0, objP = gameData.objs.objects; nObject <= gameData.objs.nLastObject; nObject++, objP++) {
	if (objP->nType == OBJ_NONE)
		continue;
	if (objP && (objP->nType == OBJ_POWERUP) && !EGI_FLAG (bPowerupLights, 0, 0, 0)) 
		continue;
	objPos = &objP->position.vPos;
	xObjIntensity = ComputeLightIntensity (nObject, &color, &bGotColor);
	if (bGotColor)
		bKeepDynColoring = 1;
	if (xObjIntensity) {
		ApplyLight (xObjIntensity, objP->nSegment, objPos, nRenderVertices, gameData.render.lights.vertices, OBJ_IDX (objP), bGotColor ? &color : NULL);
		gameData.render.lights.newObjects [nObject] = 1;
		}
	}
//	Now, process all lights from last frame which haven't been processed this frame.
for (nObject = 0; nObject <= gameData.objs.nLastObject; nObject++) {
	//	In multiplayer games, process even unprocessed gameData.objs.objects every 4th frame, else don't know about tPlayer sneaking up.
	if ((gameData.render.lights.objects [nObject]) || 
		 (IsMultiGame && (((nObject ^ gameData.app.nFrameCount) & 3) == 0))) {
		if (gameData.render.lights.newObjects [nObject])
			//	Not lit last frame, so we don't need to light it.  (Already lit if casting light this frame.)
			//	But copy value from gameData.render.lights.newObjects to update gameData.render.lights.objects array.
			gameData.render.lights.objects [nObject] = gameData.render.lights.newObjects [nObject];
		else {
			//	Lit last frame, but not this frame.  Get intensity...
			objP = gameData.objs.objects + nObject;
			objPos = &objP->position.vPos;
			xObjIntensity = ComputeLightIntensity (nObject, &color, &bGotColor);
			if (bGotColor)
				bKeepDynColoring = 1;
			if (xObjIntensity) {
				ApplyLight (xObjIntensity, objP->nSegment, objPos, nRenderVertices, gameData.render.lights.vertices, nObject, 
								bGotColor ? &color : NULL);
				gameData.render.lights.objects [nObject] = 1;
				} 
			else
				gameData.render.lights.objects [nObject] = 0;
			}
		} 
	}
if (!bKeepDynColoring)
	InitDynColoring ();
}

// ---------------------------------------------------------

void toggle_headlight_active ()
{
	if (LOCALPLAYER.flags & PLAYER_FLAGS_HEADLIGHT) {
		LOCALPLAYER.flags ^= PLAYER_FLAGS_HEADLIGHT_ON;			
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendFlags ((char) gameData.multiplayer.nLocalPlayer);		
	}
}

// ---------------------------------------------------------

#define HEADLIGHT_BOOST_SCALE 8		//how much to scale light when have headlight boost
fix	Beam_brightness = (F1_0/2);	//global saying how bright the light beam is
#define MAX_DIST_LOG	6							//log (MAX_DIST-expressed-as-integer)
#define MAX_DIST		 (f1_0<<MAX_DIST_LOG)	//no light beyond this dist

fix ComputeHeadlightLightOnObject (tObject *objP)
{
	int	i;
	fix	light;
	//	Let's just illuminate players and robots for speed reasons, ok?
	if ((objP->nType != OBJ_ROBOT) && (objP->nType	!= OBJ_PLAYER))
		return 0;
	light = 0;
	for (i=0; i<nHeadLights; i++) {
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

// -- Unused -- //Compute the lighting from the headlight for a given vertex on a face.
// -- Unused -- //Takes:
// -- Unused -- //  point - the 3d coords of the point
// -- Unused -- //  face_light - a scale factor derived from the surface normal of the face
// -- Unused -- //If no surface normal effect is wanted, pass F1_0 for face_light
// -- Unused -- fix compute_headlight_light (vmsVector *point,fix face_light)
// -- Unused -- {
// -- Unused -- 	fix light;
// -- Unused -- 	int use_beam = 0;		//flag for beam effect
// -- Unused --
// -- Unused -- 	light = Beam_brightness;
// -- Unused --
// -- Unused -- 	if ((LOCALPLAYER.flags & PLAYER_FLAGS_HEADLIGHT) && (LOCALPLAYER.flags & PLAYER_FLAGS_HEADLIGHT_ON) && gameData.objs.viewer==&gameData.objs.objects [LOCALPLAYER.nObject] && LOCALPLAYER.energy > 0) {
// -- Unused -- 		light *= HEADLIGHT_BOOST_SCALE;
// -- Unused -- 		use_beam = 1;	//give us beam effect
// -- Unused -- 	}
// -- Unused --
// -- Unused -- 	if (light) {				//if no beam, don't bother with the rest of this
// -- Unused -- 		fix pointDist;
// -- Unused --
// -- Unused -- 		pointDist = VmVecMagQuick (point);
// -- Unused --
// -- Unused -- 		if (pointDist >= MAX_DIST)
// -- Unused --
// -- Unused -- 			light = 0;
// -- Unused --
// -- Unused -- 		else {
// -- Unused -- 			fix dist_scale,face_scale;
// -- Unused --
// -- Unused -- 			dist_scale = (MAX_DIST - pointDist) >> MAX_DIST_LOG;
// -- Unused -- 			light = FixMul (light,dist_scale);
// -- Unused --
// -- Unused -- 			if (face_light < 0)
// -- Unused -- 				face_light = 0;
// -- Unused --
// -- Unused -- 			face_scale = f1_0/4 + face_light/2;
// -- Unused -- 			light = FixMul (light,face_scale);
// -- Unused --
// -- Unused -- 			if (use_beam) {
// -- Unused -- 				fix beam_scale;
// -- Unused --
// -- Unused -- 				if (face_light > f1_0*3/4 && point->z > i2f (12)) {
// -- Unused -- 					beam_scale = FixDiv (point->z,pointDist);
// -- Unused -- 					beam_scale = FixMul (beam_scale,beam_scale);	//square it
// -- Unused -- 					light = FixMul (light,beam_scale);
// -- Unused -- 				}
// -- Unused -- 			}
// -- Unused -- 		}
// -- Unused -- 	}
// -- Unused --
// -- Unused -- 	return light;
// -- Unused -- }
// ----------------------------------------------------------------------------------------------
//compute the average dynamic light in a tSegment.  Takes the tSegment number
fix ComputeSegDynamicLight (int nSegment)
{
short *verts = gameData.segs.segments [nSegment].verts;
fix sum = gameData.render.lights.dynamicLight [*verts++];
sum += gameData.render.lights.dynamicLight [*verts++];
sum += gameData.render.lights.dynamicLight [*verts++];
sum += gameData.render.lights.dynamicLight [*verts++];
sum += gameData.render.lights.dynamicLight [*verts++];
sum += gameData.render.lights.dynamicLight [*verts++];
sum += gameData.render.lights.dynamicLight [*verts++];
sum += gameData.render.lights.dynamicLight [*verts];
return sum >> 3;
}
// ----------------------------------------------------------------------------------------------
tObject *oldViewer;
int bResetLightingHack;
#define LIGHT_RATE i2f (4)		//how fast the light ramps up
void StartLightingFrame (tObject *viewer)
{
bResetLightingHack = (viewer != oldViewer);
oldViewer = viewer;
}
// ----------------------------------------------------------------------------------------------
//compute the lighting for an tObject.  Takes a pointer to the tObject,
//and possibly a rotated 3d point.  If the point isn't specified, the
//object's center point is rotated.
fix ComputeObjectLight (tObject *objP, vmsVector *vRotated)
{
	fix light;
	int nObject = OBJ_IDX (objP);

	//First, get static light for this tSegment
if (gameOpts->render.bDynLighting && !((gameOpts->render.nPath && gameOpts->ogl.bObjLighting) || gameOpts->ogl.bLightObjects)) {
	gameData.objs.color = *AvgSgmColor (objP->nSegment, &objP->position.vPos);
	light = F1_0;
	}
else
	light = gameData.segs.segment2s [objP->nSegment].xAvgSegLight;
//return light;
//Now, maybe return different value to smooth transitions
if (!bResetLightingHack && (gameData.objs.nLightSig [nObject] == objP->nSignature)) {
	fix xDeltaLight, xFrameDelta;
	xDeltaLight = light - gameData.objs.xLight [nObject];
	xFrameDelta = FixMul (LIGHT_RATE, gameData.time.xFrame);
	if (abs (xDeltaLight) <= xFrameDelta)
		gameData.objs.xLight [nObject] = light;		//we've hit the goal
	else
		if (xDeltaLight < 0)
			light = gameData.objs.xLight [nObject] -= xFrameDelta;
		else
			light = gameData.objs.xLight [nObject] += xFrameDelta;
	}
else {		//new tObject, initialize
	gameData.objs.nLightSig [nObject] = objP->nSignature;
	gameData.objs.xLight [nObject] = light;
	}
//Next, add in headlight on this tObject
// -- Matt code: light += compute_headlight_light (vRotated,f1_0);
light += ComputeHeadlightLightOnObject (objP);
//Finally, add in dynamic light for this tSegment
light += ComputeSegDynamicLight (objP->nSegment);
return light;
}
// ----------------------------------------------------------------------------------------------

void ComputeEngineGlow (tObject *objP, fix *xEngineGlowValue)
{
xEngineGlowValue [0] = f1_0/5;
if (objP->movementType == MT_PHYSICS) {
	if ((objP->nType == OBJ_PLAYER) && (objP->mType.physInfo.flags & PF_USES_THRUST) && (objP->id == gameData.multiplayer.nLocalPlayer)) {
		fix thrust_mag = VmVecMagQuick (&objP->mType.physInfo.thrust);
		xEngineGlowValue [0] += (FixDiv (thrust_mag,gameData.pig.ship.player->maxThrust)*4)/5;
	}
	else {
		fix speed = VmVecMagQuick (&objP->mType.physInfo.velocity);
		xEngineGlowValue [0] += (FixDiv (speed, MAX_VELOCITY) * 3) / 5;
		}
	}
//set value for tPlayer headlight
if (objP->nType == OBJ_PLAYER) {
	if ((gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT) && 
		 !gameStates.app.bEndLevelSequence)
		xEngineGlowValue [1] =  (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT_ON) ? -2 : -1;
	else
		xEngineGlowValue [1] = -3;			//don't draw
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
	tEffectClip	*ecP = (nClip < 0) ? NULL : gameData.eff.pEffects + nClip;
	short	nDestBM = ecP ? ecP->nDestBm : -1;
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
		else if (EGI_FLAG (bFlickerLights, 1, 0, 1)) 
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
int AddFlicker (int nSegment, int nSide, fix delay, unsigned int mask)
{
	int l;
	tFlickeringLight *flP;
#if TRACE
	//con_printf (CONDBG,"AddFlicker: %d:%d %x %x\n",nSegment,nSide,delay,mask);
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
if (SHOW_DYN_LIGHT) {
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

short UpdateDynLight (tRgbaColorf *pc, float brightness, short nSegment, short nSide, short nObject)
{
	short	nLight = FindDynLight (nSegment, nSide, nObject);

if (nLight >= 0) {
	tDynLight *pl = gameData.render.lights.dynamic.lights + nLight;
	if (!pc)
		pc = &pl->color;
	if ((pl->brightness != brightness) || 
		 (pl->color.red != pc->red) || (pl->color.green != pc->green) || (pl->color.blue != pc->blue)) {
		SetDynLightColor (nLight, pc->red, pc->green, pc->blue, brightness);
		}
	}
return nLight;
}

//------------------------------------------------------------------------------

int LastEnabledDynLight (void)
{
	short	i = gameData.render.lights.dynamic.nLights;
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
		gameData.render.lights.dynamic.owners [pl1->nObject] = 
			(short) (pl1 - gameData.render.lights.dynamic.lights);
	if (pl2->nObject >= 0)
		gameData.render.lights.dynamic.owners [pl2->nObject] = 
			(short) (pl2 - gameData.render.lights.dynamic.lights);
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

int AddDynLight (tRgbaColorf *pc, fix xBrightness, short nSegment, short nSide, short nObject, vmsVector *vPos)
{
	tDynLight	*pl;
	short			h, i;
	fix			rMin, rMax;
#if USE_OGL_LIGHTS
	GLint			nMaxLights;
#endif

#if 0
if (xBrightness > F1_0)
	xBrightness = F1_0;
#endif
#ifdef _DEBUG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nSegment = nSegment;
#endif
if (0 <= (h = UpdateDynLight (pc, f2fl (xBrightness), nSegment, nSide, nObject)))
	return h;
if (!pc)
	return -1;
if ((pc->red == 0.0f) && (pc->green == 0.0f) && (pc->blue == 0.0f)) {
	if (gameStates.app.bD2XLevel)
		return -1;
	pc->red = pc->green = pc->blue = 1.0f;
	}
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
//0: static light
//2: object/lightning
//3: headlight
SetDynLightColor (gameData.render.lights.dynamic.nLights, pc->red, pc->green, pc->blue, f2fl (xBrightness));
if (nObject >= 0) {
	pl->nType = 2;
	pl->vPos = gameData.objs.objects [nObject].position.vPos;
	pl->rad = 0; //f2fl (gameData.objs.objects [nObject].size) / 2;
	gameData.render.lights.dynamic.owners [nObject] = gameData.render.lights.dynamic.nLights;
	}
else if (nSegment >= 0) {
#if 0
	vmsVector	vOffs;
	tSide			*sideP = gameData.segs.segments [nSegment].sides + nSide;
#endif
	if (nSide < 0) {
		pl->nType = 2;
		pl->bVariable = 0;
		pl->rad = 0;
		if (vPos)
			pl->vPos = *vPos;
		else
			COMPUTE_SEGMENT_CENTER_I (&pl->vPos, nSegment);
		}	
	else {
		int	t = gameData.segs.segments [nSegment].sides [nSide].nOvlTex;
		pl->nType = 0;
		ComputeSideRads (nSegment, nSide, &rMin, &rMax);
		pl->rad = f2fl ((rMin + rMax) / 20);
		//RegisterLight (NULL, nSegment, nSide);
		pl->bVariable = IsDestructibleLight (t) || IsFlickeringLight (nSegment, nSide) || IS_WALL (SEGMENTS [nSegment].sides [nSide].nWall);
		COMPUTE_SIDE_CENTER_I (&pl->vPos, nSegment, nSide);
		}
#if 0
	VmVecAdd (&vOffs, sideP->normals, sideP->normals + 1);
	VmVecScaleFrac (&vOffs, 1, 200);
	VmVecInc (&pl->vPos, &vOffs);
#endif
	}
else {
	pl->nType = 3;
	pl->bVariable = 0;
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
	//LogErr ("removing light %d,%d\n", nLight, pl - gameData.render.lights.dynamic.lights);
	// if not removing last light in list, move last light down to the now free list entry
	// and keep the freed light handle thus avoiding gaps in used handles
	if (nLight < --gameData.render.lights.dynamic.nLights) {
		*pl = gameData.render.lights.dynamic.lights [gameData.render.lights.dynamic.nLights];
		if (pl->nObject >= 0)
			gameData.render.lights.dynamic.owners [pl->nObject] = nLight;
		else if (pl->bSpot)
			gameData.render.lights.dynamic.nHeadLights [pl->nPlayer] = nLight;
#if USE_OGL_LIGHTS
		RefreshDynLight (pl);
		pl = gameData.render.lights.dynamic.lights + gameData.render.lights.dynamic.nLights;
#endif
		}
#if USE_OGL_LIGHTS
	glDisable (pl->handle);
#endif
	}
}

//------------------------------------------------------------------------------

void DeleteLightningLights (void)
{
	tDynLight	*pl;
	int			nLight;

for (nLight = gameData.render.lights.dynamic.nLights, pl = gameData.render.lights.dynamic.lights + nLight; nLight;) {
	--nLight;
	--pl;
	if ((pl->nSegment >= 0) && (pl->nSide < 0))
		if (nLight < --gameData.render.lights.dynamic.nLights) {
			*pl = gameData.render.lights.dynamic.lights [gameData.render.lights.dynamic.nLights];
			if (pl->nObject >= 0)
				gameData.render.lights.dynamic.owners [pl->nObject] = nLight;
			}
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

void RemoveDynLightningLights (void)
{
	tDynLight	*pl = gameData.render.lights.dynamic.lights;
	short			i;
for (i = 0; i < gameData.render.lights.dynamic.nLights; )
	if ((pl->nSegment >= 0) && (pl->nSide < 0)) {
		DeleteDynLight (i);
		}
	else {
		i++;
		pl++;
		}
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
	//static float fBlack [4] = {0.0f, 0.0f, 0.0f, 1.0f};
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

static inline int QCmpStaticLights (tDynLight *pl, tDynLight *pm)
{
return (pl->bVariable == pm->bVariable) ? 0 : pl->bVariable ? 1 : -1;
}

//------------------------------------------------------------------------------

void QSortStaticLights (int left, int right)
{
	int	l = left,
			r = right;
			tDynLight m = gameData.render.lights.dynamic.lights [(l + r) / 2];
			
do {
	while (QCmpStaticLights (gameData.render.lights.dynamic.lights + l, &m) < 0)
		l++;
	while (QCmpStaticLights (gameData.render.lights.dynamic.lights + r, &m) > 0)
		r--;
	if (l <= r) {
		if (l < r) {
			tDynLight h = gameData.render.lights.dynamic.lights [l];
			gameData.render.lights.dynamic.lights [l] = gameData.render.lights.dynamic.lights [r];
			gameData.render.lights.dynamic.lights [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortStaticLights (l, right);
if (left < r)
	QSortStaticLights (left, r);
}

//------------------------------------------------------------------------------

void AddDynLights (void)
{
	int			i, j, t;
	tSegment		*segP;
	tSide			*sideP;
	tFaceColor	*pc;
	short			*pSegLights, *pVertLights, *pOwners;

gameStates.render.bHaveDynLights = 1;
//glEnable (GL_LIGHTING);
if (gameOpts->render.bDynLighting)
	memset (gameData.render.color.vertices, 0, sizeof (*gameData.render.color.vertices) * MAX_VERTICES);
//memset (gameData.render.color.ambient, 0, sizeof (*gameData.render.color.ambient) * MAX_VERTICES);
pSegLights = gameData.render.lights.dynamic.nNearestSegLights;
pVertLights = gameData.render.lights.dynamic.nNearestVertLights;
pOwners = gameData.render.lights.dynamic.owners;
memset (&gameData.render.lights.dynamic, 0xff, sizeof (gameData.render.lights.dynamic));
memset (pSegLights, 0xff, sizeof (*pSegLights) * MAX_SEGMENTS * MAX_NEAREST_LIGHTS);
memset (pVertLights, 0xff, sizeof (*pVertLights) * MAX_SEGMENTS * MAX_NEAREST_LIGHTS);
memset (pOwners, 0xff, sizeof (*pOwners) * MAX_OBJECTS);
gameData.render.lights.dynamic.nNearestSegLights = pSegLights;
gameData.render.lights.dynamic.nNearestVertLights = pVertLights;
gameData.render.lights.dynamic.owners = pOwners;
gameData.render.lights.dynamic.nLights = 0;
gameData.render.lights.dynamic.material.bValid = 0;
for (i = 0, segP = gameData.segs.segments; i < gameData.segs.nSegments; i++, segP++) {
	if (gameData.segs.segment2s [i].special == SEGMENT_IS_SKYBOX)
		continue;
	for (j = 0, sideP = segP->sides; j < 6; j++, sideP++) {
		if ((segP->children [j] >= 0) && !IS_WALL (sideP->nWall))
			continue;
		t = sideP->nBaseTex;
		if (t >= MAX_WALL_TEXTURES) 
			continue;
		pc = gameData.render.color.textures + t;
		if (gameData.pig.tex.brightness [t])
			AddDynLight (&pc->color, gameData.pig.tex.brightness [t], (short) i, (short) j, -1, NULL);
		t = sideP->nOvlTex;
		if ((t > 0) && (t < MAX_WALL_TEXTURES) && gameData.pig.tex.brightness [t]) {
			pc = gameData.render.color.textures + t;
			if ((pc->color.red > 0.0f) || (pc->color.green > 0.0f) || (pc->color.blue > 0.0f))
				AddDynLight (&pc->color, gameData.pig.tex.brightness [t], (short) i, (short) j, -1, NULL);
			}
		//if (gameData.render.lights.dynamic.nLights)
		//	return;
		if (!gameStates.render.bHaveDynLights) {
			RemoveDynLights ();
			return;
			}
		}
	}
QSortStaticLights (0, gameData.render.lights.dynamic.nLights);
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
memset (&gameData.render.lights.dynamic.headLights, 0, sizeof (gameData.render.lights.dynamic.headLights));
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
	if ((psl->bSpot = pl->bSpot)) {
		VmsVecToFloat (&psl->dir, &pl->vDir);
		if (pl->bTransform && !gameStates.ogl.bUseTransform)
			G3RotatePointf (&psl->dir, &psl->dir, 0);
		psl->spotAngle = pl->spotAngle;
		psl->spotExponent = pl->spotExponent;
		if (gameStates.ogl.bHeadLight && gameOpts->ogl.bHeadLight) {
			G3TransformPointf ((fVector *) (gameData.render.lights.dynamic.headLights.pos + gameData.render.lights.dynamic.headLights.nLights), psl->pos, 0);
			G3RotatePointf ((fVector *) (gameData.render.lights.dynamic.headLights.dir + gameData.render.lights.dynamic.headLights.nLights), &psl->dir, 0);
			VmVecNegatef ((fVector *) (gameData.render.lights.dynamic.headLights.dir + gameData.render.lights.dynamic.headLights.nLights));
			gameData.render.lights.dynamic.headLights.brightness [gameData.render.lights.dynamic.headLights.nLights] = 200.0f;
			gameData.render.lights.dynamic.headLights.nLights++;
			}
		}
	psl->bState = pl->bState && (pl->color.red + pl->color.green + pl->color.blue > 0.0);
	psl->bVariable = pl->bVariable;
	psl->bOn = pl->bOn;
	psl->nType = pl->nType;
	psl->nSegment = pl->nSegment;
	psl->nObject = pl->nObject;
	psl->bLightning = (pl->nObject < 0) && (pl->nSide < 0);
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
glTexImage2D (GL_TEXTURE_2D, 0, 4, MAX_OGL_LIGHTS / 64, 64, 1, GL_COMPRESSED_RGBA_ARB,
				  GL_FLOAT, gameData.render.lights.dynamic.shader.lights);
#	endif
#endif
}

//------------------------------------------------------------------------------

void SetNearestVertexLights (int nVertex, ubyte nType, int bStatic, int bVariable, int nThread)
{
//if (gameOpts->render.bDynLighting) 
	{
	short				*pnl = gameData.render.lights.dynamic.nNearestVertLights + nVertex * MAX_NEAREST_LIGHTS;
	short				i, j;
	tShaderLight	*psl;

#ifdef _DEBUG
	if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
	gameData.render.lights.dynamic.shader.iVariableLights [nThread] = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
	for (i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
#ifdef _DEBUG
		if (j >= gameData.render.lights.dynamic.nLights)
			break;
#endif
		psl = gameData.render.lights.dynamic.shader.lights + j;
		if (gameData.threads.vertColor.data.bNoShadow && psl->bShadow)
			continue;
		if (psl->bVariable) {
			if (!(bVariable && psl->bOn))
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		psl->nType = nType;
		psl->bState = 1;
		gameData.render.lights.dynamic.shader.activeLights [nThread][gameData.render.lights.dynamic.shader.nActiveLights [nThread]++] = psl;
		}
	}
}

//------------------------------------------------------------------------------

void SetNearestStaticLights (int nSegment, int bStatic, ubyte nType, int nThread)
{
	static int nLastSeg [4] = {-1, -1, -1, -1};
	static int nActiveLights [4] = {-1, -1, -1, -1};
	static int nFrameFlipFlop = -1;
	static ubyte nLastType [4] = {255, 255, 255, 255};

	tShaderLight *psl;
	int nMaxLights = gameOpts->ogl.bObjLighting ? 8 : gameOpts->ogl.nMaxLights;

#if 0
if ((nFrameFlipFlop == gameStates.render.nFrameFlipFlop + 1) && (nLastSeg [nThread] == nSegment) && (nLastType [nThread] == nType)) {
	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = nActiveLights [nThread];
	return;
	}
nFrameFlipFlop = gameStates.render.nFrameFlipFlop + 1;
nLastSeg [nThread] = nSegment;
nLastType [nThread] = nType;
#endif
if (gameOpts->render.bDynLighting) {
	short	*pnl = gameData.render.lights.dynamic.nNearestSegLights + nSegment * MAX_NEAREST_LIGHTS;
	short	i, j;
	gameData.render.lights.dynamic.shader.iStaticLights [nThread] = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
	for (i = nMaxLights; i; i--, pnl++) {
		if ((j = *pnl) < 0)
			break;
		//gameData.render.lights.dynamic.shader.lights [j].nType = nType;
		if (gameData.threads.vertColor.data.bNoShadow && gameData.render.lights.dynamic.shader.lights [j].bShadow)
			continue;
		psl = gameData.render.lights.dynamic.shader.lights + j;
		if (psl->bVariable) {
			if (!psl->bOn)
				continue;
			}
		else {
			if (!bStatic)
				continue;
			}
		gameData.render.lights.dynamic.shader.activeLights [nThread][gameData.render.lights.dynamic.shader.nActiveLights [nThread]++] =
			psl;
		}
	}
nActiveLights [nThread] = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
}

//------------------------------------------------------------------------------

void QSortDynamicLights (int left, int right, int nThread)
{
	int	l = left,
			r = right,
			m = gameData.render.lights.dynamic.shader.activeLights [nThread][(l + r) / 2]->xDistance;
			
do {
	while (gameData.render.lights.dynamic.shader.activeLights [nThread][l]->xDistance < m)
		l++;
	while (gameData.render.lights.dynamic.shader.activeLights [nThread][r]->xDistance > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tShaderLight *h = gameData.render.lights.dynamic.shader.activeLights [nThread][l];
			gameData.render.lights.dynamic.shader.activeLights [nThread][l] = gameData.render.lights.dynamic.shader.activeLights [nThread][r];
			gameData.render.lights.dynamic.shader.activeLights [nThread][r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortDynamicLights (l, right, nThread);
if (left < r)
	QSortDynamicLights (left, r, nThread);
}

//------------------------------------------------------------------------------

#if PROFILING
time_t tSetNearestDynamicLights = 0;
#endif

short SetNearestDynamicLights (int nSegment, int bVariable, int nType, int nThread)
{
#if CACHE_LIGHTS
	static int nLastSeg [4] = {-1, -1, -1, -1};
	static int nActiveLights [4] = {-1, -1, -1, -1};
	static int nFrameFlipFlop = 0;
#endif
	int nMaxLights = (nType && gameOpts->ogl.bObjLighting) ? 8 : gameOpts->ogl.nMaxLights;
#if PROFILING
	time_t t = clock ();
#endif

#if CACHE_LIGHTS
if ((nFrameFlipFlop == gameStates.render.nFrameFlipFlop + 1) && (nLastSeg [nThread] == nSegment))
	return gameData.render.lights.dynamic.shader.nActiveLights [nThread] = nActiveLights [nThread];
nFrameFlipFlop = gameStates.render.nFrameFlipFlop + 1;
nLastSeg [nThread] = nSegment;
#endif
#ifdef _DEBUG
if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
	nDbgSeg = nDbgSeg;
#endif
if (gameOpts->render.bDynLighting) {
	short				i = gameData.render.lights.dynamic.shader.nLights,
						nLightSeg;
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights + i;
	vmsVector		c;

	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = 0;
	COMPUTE_SEGMENT_CENTER_I (&c, nSegment);
	while (i--) {
		if ((--psl)->nType < 2) {
			if (!bVariable)
				break;
			if (!(psl->bVariable && psl->bOn))
				continue;
			}
		if (gameData.threads.vertColor.data.bNoShadow && psl->bShadow)
			continue;
#ifdef _DEBUG
		if ((nDbgSeg >= 0) && (psl->nSegment == nDbgSeg))
			psl = psl;
#endif
		if (psl->nType < 3) {
			nLightSeg = (psl->nSegment < 0) ? (psl->nObject < 0) ? -1 : gameData.objs.objects [psl->nObject].nSegment : psl->nSegment;
			if ((nLightSeg < 0) || !SEGVIS (nLightSeg, nSegment)) 
				continue;
			if ((psl->xDistance = VmVecDist (&c, &psl->vPos)) > MAX_LIGHT_RANGE)
				continue;
			}
		gameData.render.lights.dynamic.shader.activeLights [nThread][gameData.render.lights.dynamic.shader.nActiveLights [nThread]++] = psl;
		}
	}
#if 1
if (nType && (gameData.render.lights.dynamic.shader.nActiveLights [nThread] > nMaxLights)) {
	QSortDynamicLights (0, gameData.render.lights.dynamic.shader.nActiveLights [nThread] - 1, nThread);
	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = nMaxLights;
	}
#endif
#if PROFILING
tSetNearestDynamicLights += clock () - t;
#endif
#if CACHE_LIGHTS
return nActiveLights [nThread] = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
#else
return gameData.render.lights.dynamic.shader.nActiveLights [nThread];
#endif
}

//------------------------------------------------------------------------------

extern float fLightRanges [3];
extern short nDbgSeg;

tFaceColor *AvgSgmColor (int nSegment, vmsVector *pvPos)
{
	tFaceColor	c, *pvc, *psc = gameData.render.color.segments + nSegment;
	short			i, *pv;
	vmsVector	vCenter, vVertex;
	float			d, ds;

#ifdef _DEBUG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
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
	}
psc->color.red = c.color.red / 8.0f;
psc->color.green = c.color.green / 8.0f;
psc->color.blue = c.color.blue / 8.0f;
#if 0
if (SetNearestDynamicLights (nSegment, 1)) {
	short				nLights = gameData.render.lights.dynamic.shader.nLights;
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights + nLights;
	float				fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightRange];
	float				fLightDist, fAttenuation;
	fVector			vPosf;
	if (pvPos)
		VmsVecToFloat (&vPosf, pvPos);
	for (i = 0; i < gameData.render.lights.dynamic.shader.nActiveLights; i++) {
		psl = gameData.render.lights.dynamic.shader.activeLights [i];
#if 1
		if (pvPos) {
			vVertex = gameData.segs.vertices [*pv];
			//G3TransformPoint (&vVertex, &vVertex);
			fLightDist = VmVecDistf (psl->pos, &vPosf) / fLightRange;
			fAttenuation = fLightDist / psl->brightness;
			VmVecScaleAddf ((fVector *) &c.color, (fVector *) &c.color, (fVector *) &psl->color, 1.0f / fAttenuation);
			}
		else 
#endif
			{
			VmVecIncf ((fVector *) &psc->color, (fVector *) &psl->color);
			}
		}
	}
#endif
#if 0
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
#endif
psc->index = gameStates.render.nFrameFlipFlop + 1;
return psc;
}

//------------------------------------------------------------------------------

int AddOglHeadLight (tObject *objP)
{
	static float spotExps [] = {10.0f, 5.0f, 0.0f};
	static float spotAngles [] = {0.75f, 0.5f, 0.25f};

if (gameOpts->render.bDynLighting && (gameData.render.lights.dynamic.nHeadLights [objP->id] < 0)) {
		tRgbaColorf	c = {1.0f, 1.0f, 1.0f, 1.0f};
		tDynLight	*pl;
		int			nLight;

	nLight = AddDynLight (&c, F1_0 * 50, -1, -1, -1, NULL);
	if (nLight >= 0) {
		gameData.render.lights.dynamic.nHeadLights [objP->id] = nLight;
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
if (gameOpts->render.bDynLighting && (gameData.render.lights.dynamic.nHeadLights [objP->id] >= 0)) {
	DeleteDynLight (gameData.render.lights.dynamic.nHeadLights [objP->id]);
	gameData.render.lights.dynamic.nHeadLights [objP->id] = -1;
	gameData.render.lights.dynamic.headLights.nLights = 0;
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
	objP = gameData.objs.objects + gameData.multiplayer.players [nPlayer].nObject;
	pl->vPos = OBJPOS (objP)->vPos;
	pl->vDir = OBJPOS (objP)->mOrient.fVec;
	VmVecScaleInc (&pl->vPos, &pl->vDir, objP->size / 4);
	}
}

//------------------------------------------------------------------------------

void ComputeStaticVertexLights (int nVertex, int nMax, int nThread)
{
	tFaceColor	*pf = gameData.render.color.ambient + nVertex;
	fVector		vVertex;
	int			bColorize = !gameOpts->render.bDynLighting;

for (; nVertex < nMax; nVertex++, pf++) {
#ifdef _DEBUG
	if (nVertex == nDbgVertex)
		nVertex = nVertex;
#endif
	VmsVecToFloat (&vVertex, gameData.segs.vertices + nVertex);
	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = 0;
	SetNearestVertexLights (nVertex, 1, 1, bColorize, nThread);
	gameData.render.color.vertices [nVertex].index = 0;
	G3VertexColor (&gameData.segs.points [nVertex].p3_normal.vNormal, &vVertex, nVertex, pf, NULL, 1, 0, nThread);
	}
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
extern int nDbgVertex;
#endif

void ComputeStaticDynLighting (void)
{
memset (&gameData.render.lights.dynamic.headLights, 0, sizeof (gameData.render.lights.dynamic.headLights));
if (gameStates.app.bNostalgia)
	return;
if (gameOpts->render.bDynLighting || (gameOpts->render.color.bAmbientLight && !gameStates.render.bColored)) {
		int				i, j, bColorize = !gameOpts->render.bDynLighting;
		tFaceColor		*pfh, *pf = gameData.render.color.ambient;
		tSegment2		*seg2P;

	LogErr ("Computing static lighting\n");
	gameData.render.vertColor.bDarkness = IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [IsMultiGame].bDarkness;
	gameStates.render.nState = 0;
	TransformDynLights (1, bColorize);
	memset (pf, 0, gameData.segs.nVertices * sizeof (*pf));
	if (!RunRenderThreads (rtStaticVertLight))
		ComputeStaticVertexLights (0, gameData.segs.nVertices, 0);
	pf = gameData.render.color.ambient;
	for (i = 0, seg2P = gameData.segs.segment2s; i < gameData.segs.nSegments; i++, seg2P++) {
		if (seg2P->special == SEGMENT_IS_SKYBOX) {
			short	*sv = SEGMENTS [i].verts;
			for (j = 8; j; j--, sv++) {
				pfh = pf + *sv;
				pfh->color.red =
				pfh->color.green =
				pfh->color.blue = 
				pfh->color.alpha = 1;
				}
			}
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

//------------------------------------------------------------------------------

int IsLight (int tMapNum) 
{
if (gameStates.app.bD1Mission)
	tMapNum = ConvertD1Texture (tMapNum, 1);
#if 1
if (gameData.pig.tex.brightness [tMapNum] > 0)
	return gameData.pig.tex.brightness [tMapNum];
#else
if (gameData.pig.tex.pTMapInfo [tMapNum].lighting > 0)
	return gameData.pig.tex.pTMapInfo [tMapNum].lighting;
#endif
switch (tMapNum) {
	case 275:
	case 276:
	case 278:
	case 288:
	case 289:
	case 290:
	case 291:
	case 293:
	case 295:
	case 296:
	case 298:
	case 300:
	case 301:
	case 302:
	case 305:
	case 306:
	case 307:
	case 348:
	case 349:
	case 340:
	case 341:
	case 345:
	case 382:
	case 343:
	case 344:
	case 377:
	case 346:
	case 351:
	case 352:
	case 364:
	case 366:
	case 368:
	case 370:
	case 372:
	case 380:
	case 410:
	case 427:
	case 374:
	case 375:
	case 391:
	case 392:
	case 393:
	case 394:
	case 395:
	case 396:
	case 397:
	case 398:
	case 411:
	case 412:
	case 414:
	case 416:
	case 418:
	case 423:
	case 424:
	case 428:
	case 429:
	case 430:
	case 431:
	case 235:
	case 236:
	case 237:
	case 243:
	case 244:
	case 333:
	case 353:
	case 356:
	case 357:
	case 358:
	case 359:
	case 378:
	case 404:
	case 405:
	case 406:
	case 407:
	case 408:
	case 409:
	case 426:
	case 434:
	case 420:
	case 432:
	case 433:
		return F1_0;
	default:
		break;
	}
return 0;
}

//------------------------------------------------------------------------------

#if DBG_SHADERS


#else

char *lightingFS [12] = {
	//single player version - one player
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos, lightDir;" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness;" \
	"void main(void) {" \
	"vec3 lightVec, spotColor;" \
	"float spotEffect, lightDist;" \
	"lightVec = vertPos - lightPos;" \
	"lightDist = length (lightVec);" \
	"spotEffect = dot (lightDir, lightVec / lightDist);" \
	"if (spotEffect < 0.25)" \
	"   gl_FragColor = vec4 (vec3 (matColor) * vec3 (gl_Color), (matColor.a + gl_Color.a) * grAlpha);"  \
	"else {" \
	"	 float attenuation = min (200.0 / lightDist, 1.0);" \
	"   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"   spotColor = vec3 (max (spotEffect, gl_Color.r), max (spotEffect, gl_Color.g), max (spotEffect, gl_Color.b));" \
	"   gl_FragColor = vec4 (vec3 (matColor) * vec3 (spotColor), matColor.a * grAlpha);"  \
	"   }" \
	"}" 
	,
	"uniform sampler2D btmTex;" \
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos, lightDir;" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness;" \
	"void main(void) {" \
	"vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));" \
	"vec3 lightVec, spotColor;" \
	"float spotEffect, lightDist;" \
	"lightVec = vertPos - lightPos;" \
	"lightDist = length (lightVec);" \
	"spotEffect = dot (lightDir, lightVec / lightDist);" \
	"if (spotEffect < 0.25)" \
	"   gl_FragColor = vec4 (vec3 (gl_Color), gl_Color.a * grAlpha);" \
	"else {" \
	"	 float attenuation = min (200.0 / lightDist, 1.0);" \
	"   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"   spotColor = vec3 (max (spotEffect, gl_Color.r), max (spotEffect, gl_Color.g), max (spotEffect, gl_Color.b));" \
	"   gl_FragColor = vec4 (vec3 (btmColor), btmColor.a * grAlpha) * vec4 (spotColor, gl_Color.a);" \
	"	 }" \
	"}" 
	,
	"uniform sampler2D btmTex, topTex;" \
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos, lightDir;" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness;" \
	"void main(void) {" \
	"vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));" \
	"vec4 topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));" \
	"vec3 lightVec, spotColor;" \
	"float spotEffect, lightDist;" \
	"lightVec = vertPos - lightPos;" \
	"lightDist = length (lightVec);" \
	"spotEffect = dot (lightDir, lightVec / lightDist);" \
	"if (spotEffect < 0.25)" \
	"   gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * gl_Color;" \
	"else {" \
	"   float attenuation = min (200.0 / lightDist, 1.0);" \
	"   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"   spotColor = vec3 (max (spotEffect, gl_Color.r), max (spotEffect, gl_Color.g), max (spotEffect, gl_Color.b));" \
	"   gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * vec4 (spotColor, gl_Color.a);" \
	"	 }" \
	"}" 
	,
	"uniform sampler2D btmTex, topTex, maskTex;" \
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos, lightDir;" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness;" \
	"void main (void) {" \
	"float bMask = texture2D (maskTex, vec2 (gl_TexCoord [2])).r;" \
	"if (bMask == 0)" \
	"   discard;" \
	"else {" \
	"   vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));" \
	"   vec4 topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));" \
	"   vec3 lightVec, spotColor;" \
	"   float spotEffect, lightDist;" \
	"   lightVec = vertPos - lightPos;" \
	"   lightDist = length (lightVec);" \
	"   spotEffect = dot (lightDir, lightVec / lightDist);" \
	"   if (spotEffect < 0.25)" \
	"      gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * gl_Color;" \
	"   else {" \
	" 		 float attenuation = min (200.0 / lightDist, 1.0);" \
	"      spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"      spotColor = vec3 (max (spotColor.r, gl_Color.r), max (spotColor.g, gl_Color.g), max (spotColor.b, gl_Color.b));" \
	"      gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * vec4 (spotColor, gl_Color.a);" \
	"   	}" \
	"   }" \
	"}" 
	,
	//coop version - max. 4 players
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos [4], lightDir [4];" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness [4];" \
	"void main(void) {" \
	"vec3 lightVec, spotColor = vec3 (0,0,0);" \
	"float spotEffect, lightDist;" \
	"int i;" \
	"for (i = 0; i < 4; i++) {" \
	"   if (brightness [i] == 0)" \
	"      break;" \
	"   lightVec = vertPos - lightPos [i];" \
	"   lightDist = length (lightVec);" \
	"   spotEffect = dot (lightDir [i], lightVec / lightDist);" \
	"   if (spotEffect >= 0.25) {" \
	"   	float attenuation = min (brightness [i] / lightDist, 1.0);" \
	"	   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"	   spotColor.r += spotEffect;" \
	"	   spotColor.g += spotEffect;" \
	"	   spotColor.b += spotEffect;" \
	"	   }" \
	"	}" \
	"spotColor = vec3 (max (spotColor.r, gl_Color.r), max (spotColor.g, gl_Color.g), max (spotColor.b, gl_Color.b));" \
	"gl_FragColor = vec4 (vec3 (matColor) * vec3 (spotColor), matColor.a * grAlpha);"  \
	"}" 
	,
	"uniform sampler2D btmTex;" \
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos [4], lightDir [4];" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness [4];" \
	"void main(void) {" \
	"vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));" \
	"vec3 lightVec, spotColor = vec3 (0,0,0);" \
	"float spotEffect, lightDist;" \
	"int i;" \
	"for (i = 0; i < 4; i++) {" \
	"   if (brightness [i] == 0)" \
	"      break;" \
	"   lightVec = vertPos - lightPos [i];" \
	"   lightDist = length (lightVec);" \
	"   spotEffect = dot (lightDir [i], lightVec / lightDist);" \
	"   if (spotEffect >= 0.25) {" \
	"   	float attenuation = min (brightness [i] / lightDist, 1.0);" \
	"	   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"	   spotColor.r += spotEffect;" \
	"	   spotColor.g += spotEffect;" \
	"	   spotColor.b += spotEffect;" \
	"	   }" \
	"	}" \
	"spotColor = vec3 (max (spotColor.r, gl_Color.r), max (spotColor.g, gl_Color.g), max (spotColor.b, gl_Color.b));" \
	"gl_FragColor = vec4 (vec3 (btmColor), btmColor.a * grAlpha) * vec4 (spotColor, gl_Color.a);" \
	"}" 
	,
	"uniform sampler2D btmTex, topTex;" \
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos [4], lightDir [4];" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness [4];" \
	"void main(void) {" \
	"vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));" \
	"vec4 topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));" \
	"vec3 lightVec, spotColor = vec3 (0,0,0);" \
	"float spotEffect, lightDist;" \
	"int i;" \
	"for (i = 0; i < 4; i++) {" \
	"   if (brightness [i] == 0)" \
	"      break;" \
	"   lightVec = vertPos - lightPos [i];" \
	"   lightDist = length (lightVec);" \
	"   spotEffect = dot (lightDir [i], lightVec / lightDist);" \
	"   if (spotEffect >= 0.25) {" \
	"   	float attenuation = min (brightness [i] / lightDist, 1.0);" \
	"	   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"	   spotColor.r += spotEffect;" \
	"	   spotColor.g += spotEffect;" \
	"	   spotColor.b += spotEffect;" \
	"	   }" \
	"	}" \
	"spotColor = vec3 (max (spotColor.r, gl_Color.r), max (spotColor.g, gl_Color.g), max (spotColor.b, gl_Color.b));" \
	"gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * vec4 (spotColor, gl_Color.a);" \
	"}" 
	,
	"uniform sampler2D btmTex, topTex, maskTex;" \
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos [4], lightDir [4];" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness [4];" \
	"void main (void) {" \
	"float bMask = texture2D (maskTex, vec2 (gl_TexCoord [2])).r;" \
	"if (bMask == 0)" \
	"   discard;" \
	"else {" \
	"   vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));" \
	"   vec4 topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));" \
	"   vec3 lightVec, spotColor = vec3 (0,0,0);" \
	"   float spotEffect, lightDist;" \
	"   int i;" \
	"   for (i = 0; i < 4; i++) {" \
	"      if (brightness [i] == 0)" \
	"         break;" \
	"      lightVec = vertPos - lightPos [i];" \
	"      lightDist = length (lightVec);" \
	"      spotEffect = dot (lightDir [i], lightVec / lightDist);" \
	"      if (spotEffect >= 0.25) {" \
	"      	float attenuation = min (brightness [i] / lightDist, 1.0);" \
	"   	   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"   	   spotColor.r += spotEffect;" \
	"   	   spotColor.g += spotEffect;" \
	"   	   spotColor.b += spotEffect;" \
	"   	   }" \
	"   	}" \
	"   spotColor = vec3 (max (spotColor.r, gl_Color.r), max (spotColor.g, gl_Color.g), max (spotColor.b, gl_Color.b));" \
	"   gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * vec4 (spotColor, gl_Color.a);" \
	"   }" \
	"}" 
	,
	//multiplayer version - max. 8 players
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos [8], lightDir [8];" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness [8];" \
	"void main(void) {" \
	"vec3 lightVec, spotColor = vec3 (0,0,0);" \
	"float spotEffect, lightDist;" \
	"int i;" \
	"for (i = 0; i < 8; i++) {" \
	"   if (brightness [i] == 0)" \
	"      break;" \
	"   lightVec = vertPos - lightPos [i];" \
	"   lightDist = length (lightVec);" \
	"   spotEffect = dot (lightDir [i], lightVec / lightDist);" \
	"   if (spotEffect >= 0.25) {" \
	"   	float attenuation = min (brightness [i] / lightDist, 1.0);" \
	"	   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"	   spotColor.r += spotEffect;" \
	"	   spotColor.g += spotEffect;" \
	"	   spotColor.b += spotEffect;" \
	"	   }" \
	"	}" \
	"spotColor = vec3 (max (spotColor.r, gl_Color.r), max (spotColor.g, gl_Color.g), max (spotColor.b, gl_Color.b));" \
	"gl_FragColor = vec4 (vec3 (matColor) * vec3 (spotColor), matColor.a * grAlpha);"  \
	"}" 
	,
	"uniform sampler2D btmTex;" \
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos [8], lightDir [8];" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness [8];" \
	"void main(void) {" \
	"vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));" \
	"vec3 lightVec, spotColor = vec3 (0,0,0);" \
	"float spotEffect, lightDist;" \
	"int i;" \
	"for (i = 0; i < 8; i++) {" \
	"   if (brightness [i] == 0)" \
	"      break;" \
	"   lightVec = vertPos - lightPos [i];" \
	"   lightDist = length (lightVec);" \
	"   spotEffect = dot (lightDir [i], lightVec / lightDist);" \
	"   if (spotEffect >= 0.25) {" \
	"   	float attenuation = min (brightness [i] / lightDist, 1.0);" \
	"	   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"	   spotColor.r += spotEffect;" \
	"	   spotColor.g += spotEffect;" \
	"	   spotColor.b += spotEffect;" \
	"	   }" \
	"	}" \
	"spotColor = vec3 (max (spotColor.r, gl_Color.r), max (spotColor.g, gl_Color.g), max (spotColor.b, gl_Color.b));" \
	"gl_FragColor = vec4 (vec3 (btmColor), btmColor.a * grAlpha) * vec4 (spotColor, gl_Color.a);" \
	"}" 
	,
	"uniform sampler2D btmTex, topTex;" \
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos [8], lightDir [8];" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness [8];" \
	"void main(void) {" \
	"vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));" \
	"vec4 topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));" \
	"vec3 lightVec, spotColor = vec3 (0,0,0);" \
	"float spotEffect, lightDist;" \
	"int i;" \
	"for (i = 0; i < 8; i++) {" \
	"   if (brightness [i] == 0)" \
	"      break;" \
	"   lightVec = vertPos - lightPos [i];" \
	"   lightDist = length (lightVec);" \
	"   spotEffect = dot (lightDir [i], lightVec / lightDist);" \
	"   if (spotEffect >= 0.25) {" \
	"   	float attenuation = min (brightness [i] / lightDist, 1.0);" \
	"	   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"	   spotColor.r += spotEffect;" \
	"	   spotColor.g += spotEffect;" \
	"	   spotColor.b += spotEffect;" \
	"	   }" \
	"	}" \
	"spotColor = vec3 (max (spotColor.r, gl_Color.r), max (spotColor.g, gl_Color.g), max (spotColor.b, gl_Color.b));" \
	"gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * vec4 (spotColor, gl_Color.a);" \
	"}" 
	,
	"uniform sampler2D btmTex, topTex, maskTex;" \
	"varying vec3 vertPos;" \
	"uniform vec3 lightPos [8], lightDir [8];" \
	"uniform vec4 matColor;" \
	"uniform float grAlpha, brightness [8];" \
	"void main (void) {" \
	"float bMask = texture2D (maskTex, vec2 (gl_TexCoord [2])).r;" \
	"if (bMask == 0)" \
	"   discard;" \
	"else {" \
	"   vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));" \
	"   vec4 topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));" \
	"   vec3 lightVec, spotColor = vec3 (0,0,0);" \
	"   float spotEffect, lightDist;" \
	"   int i;" \
	"   for (i = 0; i < 8; i++) {" \
	"      if (brightness [i] == 0)" \
	"         break;" \
	"      lightVec = vertPos - lightPos [i];" \
	"      lightDist = length (lightVec);" \
	"      spotEffect = dot (lightDir [i], lightVec / lightDist);" \
	"      if (spotEffect >= 0.25) {" \
	"      	float attenuation = min (brightness [i] / lightDist, 1.0);" \
	"   	   spotEffect = pow (spotEffect, 4.0) * attenuation;" \
	"   	   spotColor.r += spotEffect;" \
	"   	   spotColor.g += spotEffect;" \
	"   	   spotColor.b += spotEffect;" \
	"   	   }" \
	"   	}" \
	"   spotColor = vec3 (max (spotColor.r, gl_Color.r), max (spotColor.g, gl_Color.g), max (spotColor.b, gl_Color.b));" \
	"   gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * vec4 (spotColor, gl_Color.a);" \
	"   }" \
	"}" 
	};

char *lightingVS [4] = {
	"varying vec3 vertPos;" \
	"void main(void){" \
	"vertPos=vec3(gl_ModelViewMatrix * gl_Vertex);" \
	"gl_Position=gl_ModelViewProjectionMatrix * gl_Vertex;" \
   "gl_FrontColor=gl_Color;}"
	,
	"varying vec3 vertPos;" \
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"vertPos=vec3(gl_ModelViewMatrix * gl_Vertex);" \
	"gl_Position=gl_ModelViewProjectionMatrix * gl_Vertex;" \
   "gl_FrontColor=gl_Color;}"
	,
	"varying vec3 vertPos;" \
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"vertPos=vec3(gl_ModelViewMatrix * gl_Vertex);" \
	"gl_Position=gl_ModelViewProjectionMatrix * gl_Vertex;" \
   "gl_FrontColor=gl_Color;}"
	,
	"varying vec3 vertPos;" \
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_TexCoord [2]=gl_MultiTexCoord2;"\
	"vertPos=vec3(gl_ModelViewMatrix * gl_Vertex);" \
	"gl_Position=gl_ModelViewProjectionMatrix * gl_Vertex;" \
   "gl_FrontColor=gl_Color;}"
	};

#endif

//-------------------------------------------------------------------------

GLhandleARB lightingShaderProgs [12] = {0,0,0,0,0,0,0,0,0,0,0,0};
GLhandleARB lvs [12] = {0,0,0,00,0,0,0,0,0,0,0}; 
GLhandleARB lfs [12] = {0,0,0,00,0,0,0,0,0,0,0}; 

void InitLightingShaders (void)
{
	int	i, bOk;

gameStates.render.bHaveDynLights = 1;
LogErr ("building lighting shader programs\n");
DeleteShaderProg (NULL);
gameStates.ogl.bHeadLight = 1;
for (i = 0; i < 12; i++) {
	if (lightingShaderProgs [i])
		DeleteShaderProg (lightingShaderProgs + i);
	bOk = CreateShaderProg (lightingShaderProgs + i) &&
			CreateShaderFunc (lightingShaderProgs + i, lfs + i, lvs + i, lightingFS [i % 4], lightingVS [i % 4], 1) &&
-			LinkShaderProg (lightingShaderProgs + i);
	if (!bOk) {
		gameStates.ogl.bHeadLight = 0;
		while (i)
			DeleteShaderProg (lightingShaderProgs + --i);
		break;
		}
	}
}

// ----------------------------------------------------------------------------------------------
//eof
