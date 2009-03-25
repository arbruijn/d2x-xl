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
#include "u_mem.h"
#include "ogl_color.h"
#include "error.h"
#include "timer.h"
#include "gameseg.h"
#include "lightmap.h"
#include "text.h"
#include "dynlight.h"
#include "headlight.h"
#include "light.h"
#include "lightcluster.h"
#include "lightning.h"
#include "network.h"

#define CACHE_LIGHTS 0
#define FLICKERFIX 0
//int	Use_fvi_lighting = 0;

#define	LIGHTING_CACHE_SIZE	4096	//	Must be power of 2!
#define	LIGHTING_FRAME_DELTA	256	//	Recompute cache value every 8 frames.
#define	LIGHTING_CACHE_SHIFT	8

int	nLightingFrameDelta = 1;
int	lightingCache [LIGHTING_CACHE_SIZE];
int	nCacheHits = 0, nCacheLookups = 1;

typedef struct {
  int    nTexture;
  int		nBrightness;
} tTexBright;

#define	NUM_LIGHTS_D1     49
#define	NUM_LIGHTS_D2     85
#define	MAX_BRIGHTNESS		I2X (2)
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

//--------------------------------------------------------------------------

int LightingMethod (void)
{
if (gameStates.render.nLightingMethod == 1)
	return 2 + gameStates.render.bAmbientColor;
else if (gameStates.render.nLightingMethod == 2)
	return 4;
return gameStates.render.bAmbientColor;
}

// ----------------------------------------------------------------------------------------------
//	Return true if we think vertex nVertex is visible from CSegment nSegment.
//	If some amount of time has gone by, then recompute, else use cached value.

int LightingCacheVisible (int nVertex, int nSegment, int nObject, CFixVector *vObjPos, int nObjSeg, CFixVector *vVertPos)
{
	int	cache_val = lightingCache [((nSegment << LIGHTING_CACHE_SHIFT) ^ nVertex) & (LIGHTING_CACHE_SIZE-1)];
	int	cache_frame = cache_val >> 1;
	int	cache_vis = cache_val & 1;

nCacheLookups++;
if ((cache_frame == 0) || (cache_frame + nLightingFrameDelta <= gameData.app.nFrameCount)) {
	int			bApplyLight = 0;
	tFVIQuery	fq;
	tFVIData		hit_data;
	int			nSegment, hitType;
	nSegment = -1;
	#if DBG
	nSegment = FindSegByPos (*vObjPos, nObjSeg, 1, 0);
	if (nSegment == -1) {
		Int3 ();		//	Obj_pos is not in nObjSeg!
		return 0;		//	Done processing this CObject.
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
		return bApplyLight;	//	Hey, we're not supposed to be checking objects!
	if (hitType == HIT_NONE)
		bApplyLight = 1;
	else if (hitType == HIT_WALL) {
		fix distDist = CFixVector::Dist(hit_data.hit.vPoint, *vObjPos);
		if (distDist < I2X (1)/4) {
			bApplyLight = 1;
			// -- Int3 ();	//	Curious, did fvi detect intersection with CWall containing vertex?
			}
		}
	lightingCache [((nSegment << LIGHTING_CACHE_SHIFT) ^ nVertex) & (LIGHTING_CACHE_SIZE-1)] = bApplyLight + (gameData.app.nFrameCount << 1);
	return bApplyLight;
	}
nCacheHits++;
return cache_vis;
}

// ----------------------------------------------------------------------------------------------

void InitDynColoring (void)
{
if (!gameStates.render.nLightingMethod && gameData.render.lights.bInitDynColoring) {
	gameData.render.lights.bInitDynColoring = 0;
	gameData.render.lights.bGotDynColor.Clear ();
	}
gameData.render.lights.bGotGlobalDynColor = 0;
gameData.render.lights.bStartDynColoring = 0;
}

// ----------------------------------------------------------------------------------------------

void SetDynColor (tRgbaColorf *color, tRgbColorf *dynColorP, int nVertex, ubyte *bGotDynColorP, int bForce)
{
if (!color)
	return;
#if 1
if (!bForce && (color->red == 1.0) && (color->green == 1.0) && (color->blue == 1.0))
	return;
#endif
if (gameData.render.lights.bStartDynColoring) {
	InitDynColoring ();
	}
if (!dynColorP) {
	SetDynColor (color, &gameData.render.lights.globalDynColor, 0, &gameData.render.lights.bGotGlobalDynColor, bForce);
	dynColorP = gameData.render.lights.dynamicColor + nVertex;
	bGotDynColorP = gameData.render.lights.bGotDynColor + nVertex;
	}
if (*bGotDynColorP) {
	dynColorP->red = (dynColorP->red + color->red) / 2;
	dynColorP->green = (dynColorP->green + color->green) / 2;
	dynColorP->blue = (dynColorP->blue + color->blue) / 2;
	}
else {
	memcpy (dynColorP, color, sizeof (tRgbColorf));
	*bGotDynColorP = 1;
	}
}

// ----------------------------------------------------------------------------------------------

bool SkipPowerup (CObject *objP)
{
if (objP->info.nType != OBJ_POWERUP)
	return false;
if (!EGI_FLAG (bPowerupLights, 0, 0, 0))
	return true;
if (gameStates.render.bPerPixelLighting == 2) {
	int id = objP->info.nId;
	if ((id != POW_EXTRA_LIFE) && (id != POW_ENERGY) && (id != POW_SHIELD_BOOST) &&
		 (id != POW_HOARD_ORB) && (id != POW_MONSTERBALL) && (id != POW_INVUL)) {
		return true;
		}
	}
return false;
}

// ----------------------------------------------------------------------------------------------

void ApplyLight (fix xObjIntensity, int nObjSeg, CFixVector *vObjPos, int nRenderVertices,
					  CArray<short>& renderVertices,	int nObject, tRgbaColorf *color)
{
	int				iVertex, bUseColor, bForceColor;
	int				nVertex;
	int				bApplyLight;
	short				nLightObj;
	ubyte				nObjType;
	CFixVector		*vVertPos;
	fix				dist, xOrigIntensity = xObjIntensity;
	CObject*			lightObjP, *objP = (nObject < 0) ? NULL : OBJECTS + nObject;
	CPlayerData*	playerP = objP ? gameData.multiplayer.players + objP->info.nId : NULL;

nObjType = objP ? objP->info.nType : OBJ_NONE;
if (objP && SHOW_DYN_LIGHT) {
	if (nObjType == OBJ_PLAYER) {
		if (EGI_FLAG (headlight.bAvailable, 0, 0, 0)) {
			if (!HeadlightIsOn (objP->info.nId))
				lightManager.Headlights ().Remove (objP);
			else
				lightManager.Headlights ().Add (objP);
			}
		else {
			if (HeadlightIsOn (objP->info.nId)) {
				playerP->flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
				HUDInitMessage (TXT_NO_HEADLIGHTS);
				}
			}
		if (gameData.render.vertColor.bDarkness)
			return;
		xObjIntensity /= 4;
		}
	else if (nObjType == OBJ_POWERUP) {
		xObjIntensity /= 4;
		}
	else if (nObjType == OBJ_ROBOT)
		xObjIntensity /= 4;
	else if ((nObjType == OBJ_FIREBALL) || (nObjType == OBJ_EXPLOSION)) {
		xObjIntensity /= 2;
		}
#if DBG
	if (nObject == nDbgObj)
		nDbgObj = nDbgObj;
#endif
	if (0 > (nLightObj = lightClusterManager.Object (nObject).nObject))
		lightObjP = NULL;
	else
		lightObjP = OBJECTS + nLightObj;
	if (!lightClusterManager.Add (nObject, color, xObjIntensity))
		lightManager.Add (NULL, color, xObjIntensity, -1, -1, nObject, -1, NULL);
	return;
	}

if (xObjIntensity) {
	fix	obji_64 = xObjIntensity * 64;

	if (gameData.render.vertColor.bDarkness) {
		if (nObjType == OBJ_PLAYER)
			xObjIntensity = 0;
		}
	if (objP && (nObjType == OBJ_POWERUP) && !EGI_FLAG (bPowerupLights, 0, 0, 0))
		return;
	bUseColor = (color != NULL); //&& (color->red < 1.0 || color->green < 1.0 || color->blue < 1.0);
	bForceColor = objP && ((nObjType == OBJ_WEAPON) || (nObjType == OBJ_FIREBALL) || (nObjType == OBJ_EXPLOSION));
	// for pretty dim sources, only process vertices in CObject's own CSegment.
	//	12/04/95, MK, markers only cast light in own CSegment.
	if (objP && ((abs (obji_64) <= I2X (8)) || (nObjType == OBJ_MARKER))) {
		short *vp = SEGMENTS [nObjSeg].m_verts;
		for (iVertex = 0; iVertex < MAX_VERTICES_PER_SEGMENT; iVertex++) {
			nVertex = vp [iVertex];
#if !FLICKERFIX
			if ((nVertex ^ gameData.app.nFrameCount) & 1)
#endif
		 {
				vVertPos = gameData.segs.vertices + nVertex;
				dist = CFixVector::Dist (*vObjPos, *vVertPos) / 4;
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
		fix	maxHeadlightDist = I2X (200);
		if (objP && (nObjType == OBJ_PLAYER))
			if ((gameStates.render.bHeadlightOn = HeadlightIsOn (objP->info.nId))) {
				headlightShift = 3;
				if (color) {
					bUseColor = bForceColor = 1;
					color->red = color->green = color->blue = 1.0;
					}
				if (objP->info.nId != gameData.multiplayer.nLocalPlayer) {
					CFixVector	tvec;
					tFVIQuery	fq;
					tFVIData		hit_data;
					int			fate;
					tvec = *vObjPos + objP->info.position.mOrient.FVec () * I2X (200);
					fq.startSeg			= objP->info.nSegment;
					fq.p0					= vObjPos;
					fq.p1					= &tvec;
					fq.radP0				=
					fq.radP1				= 0;
					fq.thisObjNum		= nObject;
					fq.ignoreObjList	= NULL;
					fq.flags				= FQ_TRANSWALL;
					fate = FindVectorIntersection (&fq, &hit_data);
					if (fate != HIT_NONE) {
						tvec = hit_data.hit.vPoint - *vObjPos;
						maxHeadlightDist = tvec.Mag() + I2X (4);
					}
				}
			}
		// -- for (iVertex=gameData.app.nFrameCount&1; iVertex<nRenderVertices; iVertex+=2) {
		for (iVertex = 0; iVertex < nRenderVertices; iVertex++) {
			nVertex = renderVertices [iVertex];
#if DBG
			if (nVertex == nDbgVertex)
				nVertex = nVertex;
#endif
#if FLICKERFIX == 0
			if ((nVertex ^ gameData.app.nFrameCount) & 1)
#endif
		 {
				vVertPos = gameData.segs.vertices + nVertex;
				dist = CFixVector::Dist (*vObjPos, *vVertPos);
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
							CFixVector	vecToPoint;
							vecToPoint = *vVertPos - *vObjPos;
							CFixVector::Normalize(vecToPoint);		//	MK, Optimization note: You compute distance about 15 lines up, this is partially redundant
							dot = CFixVector::Dot (vecToPoint, objP->info.position.mOrient.FVec ());
							if (gameData.render.vertColor.bDarkness)
								maxDot = I2X (1) / spotSize;
							else
								maxDot = I2X (1) / 2;
							if (dot < maxDot)
								gameData.render.lights.dynamicLight [nVertex] += FixDiv (xOrigIntensity, FixMul (HEADLIGHT_SCALE, dist));	//	Do the Normal thing, but darken around headlight.
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

#define	FLASH_LEN_FIXED_SECONDS	 (I2X (1)/3)
#define	FLASH_SCALE					 (I2X (3)/FLASH_LEN_FIXED_SECONDS)

void CastMuzzleFlashLight (int nRenderVertices, CArray<short>& renderVertices)
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
							nRenderVertices, renderVertices, -1, NULL);
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

fix ComputeLightIntensity (int nObject, tRgbaColorf *colorP, char *pbGotColor)
{
	CObject		*objP = OBJECTS + nObject;
	int			nObjType = objP->info.nType;
   fix			hoardlight, s;
	static tRgbaColorf powerupColors [9] = {
	 {0,1,0,1},{1,0.8f,0,1},{0,0,1,1},{1,1,1,1},{0,0,1,1},{1,0,0,1},{1,0.8f,0,1},{0,1,0,1},{1,0.8f,0,1}
	};
	static tRgbaColorf missileColor = {1,0.3f,0,1};

colorP->red =
colorP->green =
colorP->blue = 1.0;
*pbGotColor = 0;
switch (nObjType) {
	case OBJ_PLAYER:
		*pbGotColor = 1;
		if (!gameStates.render.nLightingMethod && HeadlightIsOn (objP->info.nId)) {
			lightManager.Headlights ().Add (objP);
			return HEADLIGHT_SCALE;
			}
		 else if ((gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) && gameData.multiplayer.players [objP->info.nId].secondaryAmmo [PROXMINE_INDEX]) {

		// If hoard game and CPlayerData, add extra light based on how many orbs you have
		// Pulse as well.
		  	hoardlight = I2X (gameData.multiplayer.players [objP->info.nId].secondaryAmmo [PROXMINE_INDEX])/2; //I2X (12);
			hoardlight++;
		   FixSinCos ((gameData.time.xGame/2) & 0xFFFF,&s,NULL); // probably a bad way to do it
			s+=I2X (1);
			s>>=1;
			hoardlight = FixMul (s,hoardlight);
		   return (hoardlight);
		  }
		else if (objP->info.nId == gameData.multiplayer.nLocalPlayer) {
			return max (gameData.physics.playerThrust.Mag()/4, I2X (2)) + I2X (1)/2;
			}
		else {
			return max (objP->mType.physInfo.thrust.Mag()/4, I2X (2)) + I2X (1)/2;
			}
		break;

	case OBJ_FIREBALL:
	case OBJ_EXPLOSION:
		if (objP->info.nId == 0xff)
			return 0;
		if ((objP->info.renderType == RT_THRUSTER) || (objP->info.renderType == RT_EXPLBLAST) || (objP->info.renderType == RT_SHRAPNELS))
			return 0;
		else {
			tVideoClip *vcP = gameData.eff.vClips [0] + objP->info.nId;
			fix		xLight = vcP->lightValue;
			int		i, j;
			CBitmap	*bmoP, *bmP; // = gameData.pig.tex.bitmapP [vcP->frames [0].index].Override (-1);
#if 0
			if (bmP) {
				bmP->GetAvgColor (colorP);
				*pbGotColor = 1;
				}
			else
#endif
				{
				colorP->red =
				colorP->green =
				colorP->blue = 0.0f;
				for (i = j = 0; i < vcP->nFrameCount; i++) {
					bmP = gameData.pig.tex.bitmaps [0] + vcP->frames [i].index;
					if ((bmoP = bmP->HasOverride ()))
						bmP = bmoP;
					tRgbaColorf avgRGB;
					bmP->GetAvgColor (&avgRGB);
					if (avgRGB.red + avgRGB.green + avgRGB.blue == 0) {
						if (0 > bmP->AvgColor ())
							continue;
						bmP->GetAvgColor (&avgRGB);
						}
					colorP->red += (float) avgRGB.red;
					colorP->green += (float) avgRGB.green;
					colorP->blue += (float) avgRGB.blue;
					j++;
					}
				if (j) {
					colorP->red /= j;
					colorP->green /= j;
					colorP->blue /= j;
					*pbGotColor = 1;
					}
				}
#if 0
			if (objP->info.renderType != RT_THRUSTER)
				xLight /= 8;
#endif
			float maxColor = colorP->red;
			if (maxColor < colorP->green)
				maxColor = colorP->green;
			if (maxColor < colorP->blue)
				maxColor = colorP->blue;
			//if (maxColor > 1)
				{
				colorP->red /= maxColor;
				colorP->green /= maxColor;
				colorP->blue /= maxColor;
				}
			if (objP->info.xLifeLeft < I2X (4))
				return FixMul (FixDiv (objP->info.xLifeLeft, gameData.eff.vClips [0][objP->info.nId].xTotalTime), xLight);
			else
				return xLight;
			}
		break;

	case OBJ_ROBOT:
		*pbGotColor = 1;
#if 0//def _DEBUG
		return ROBOTINFO (objP->info.nId).lighting;
#else
		return ROBOTINFO (objP->info.nId).lightcast ? ROBOTINFO (objP->info.nId).lighting ? ROBOTINFO (objP->info.nId).lighting : I2X (1) : 0;
#endif
		break;

	case OBJ_WEAPON: {
		fix tval = gameData.weapons.info [objP->info.nId].light;
		if (gameData.objs.bIsMissile [objP->info.nId])
			*colorP = missileColor;
		else if (gameOpts->render.color.bGunLight)
			*colorP = gameData.weapons.color [objP->info.nId];
		*pbGotColor = 1;
		if (IsMultiGame)
			if (objP->info.nId == OMEGA_ID)
				if (d_rand () > 8192)
					return 0;		//	3/4 of time, omega blobs will cast 0 light!
		if (objP->info.nId == FLARE_ID) {
			return 2 * (min (tval, objP->info.xLifeLeft) + ((gameData.time.xGame ^ objLightXlat [nObject & 0x0f]) & 0x3fff));
			}
		else
			return tval;
		}

	case OBJ_MARKER: {
		fix	lightval = objP->info.xLifeLeft;
		lightval &= 0xffff;
		lightval = 8 * abs (I2X (1)/2 - lightval);
		if (objP->info.xLifeLeft < I2X (1000))
			objP->info.xLifeLeft += I2X (1);	//	Make sure this CObject doesn't go out.
		colorP->red = 0.1f;
		colorP->green = 1.0f;
		colorP->blue = 0.1f;
		*pbGotColor = 1;
		return lightval;
		}

	case OBJ_POWERUP:
		if (objP->info.nId < 9)
			*colorP = powerupColors [objP->info.nId];
		*pbGotColor = 1;
		return gameData.objs.pwrUp.info [objP->info.nId].light;
		break;

	case OBJ_DEBRIS:
		return I2X (1)/4;
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
	CObject		*objP;
	CFixVector	*objPos;
	fix			xObjIntensity;
	tRgbaColorf	color;

if (!gameOpts->render.debug.bDynamicLight)
	return;
gameData.render.lights.vertexFlags.Clear ();
gameData.render.vertColor.bDarkness = IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [IsMultiGame].bDarkness;
gameData.render.lights.bStartDynColoring = 1;
if (gameData.render.lights.bInitDynColoring) {
	InitDynColoring ();
	}
lightClusterManager.Reset ();
//	Create list of vertices that need to be looked at for setting of ambient light.
nRenderVertices = 0;
if (!gameStates.render.nLightingMethod) {
	for (iRenderSeg = 0; iRenderSeg < gameData.render.mine.nRenderSegs; iRenderSeg++) {
		nSegment = gameData.render.mine.nSegRenderList [iRenderSeg];
		if (nSegment != -1) {
			short* vp = SEGMENTS [nSegment].m_verts;
			for (v = 0; v < MAX_VERTICES_PER_SEGMENT; v++) {
				nv = vp [v];
				if ((nv < 0) || (nv > gameData.segs.nLastVertex)) {
					Int3 ();		//invalid vertex number
					continue;	//ignore it, and go on to next one
					}
				if (!gameData.render.lights.vertexFlags [nv]) {
					Assert (nRenderVertices < LEVEL_VERTICES);
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
			memset (gameData.render.lights.dynamicColor + nVertex, 0, sizeof (gameData.render.lights.dynamicColor [0]));
			}
		}
	}
CastMuzzleFlashLight (nRenderVertices, gameData.render.lights.vertices);
gameData.render.lights.newObjects.Clear ();
if (EGI_FLAG (bUseLightning, 0, 0, 1) && !gameStates.render.nLightingMethod) {
	tLightningLight	*pll;
	for (iRenderSeg = 0; iRenderSeg < gameData.render.mine.nRenderSegs; iRenderSeg++) {
		nSegment = gameData.render.mine.nSegRenderList [iRenderSeg];
		pll = lightningManager.GetLight (nSegment);
		if (pll->nFrame == gameData.app.nFrameCount)
			ApplyLight (pll->nBrightness, nSegment, &pll->vPos, nRenderVertices, gameData.render.lights.vertices, -1, &pll->color);
		}
	}
//	July 5, 1995: New faster dynamic lighting code.  About 5% faster on the PC (un-optimized).
//	Only objects which are in rendered segments cast dynamic light.  We might want to extend this
//	one or two segments if we notice light changing as OBJECTS go offscreen.  I couldn't see any
//	serious visual degradation.  In fact, I could see no humorous degradation, either. --MK
FORALL_OBJS (objP, nObject) {
	if (objP->info.nType == OBJ_NONE)
		continue;
	if (SkipPowerup (objP))
		continue;
	nObject = objP->Index ();
	objPos = &objP->info.position.vPos;
	xObjIntensity = ComputeLightIntensity (nObject, &color, &bGotColor);
	if (bGotColor)
		bKeepDynColoring = 1;
	if (xObjIntensity) {
		ApplyLight (xObjIntensity, objP->info.nSegment, objPos, nRenderVertices, gameData.render.lights.vertices, nObject, bGotColor ? &color : NULL);
		gameData.render.lights.newObjects [nObject] = 1;
		}
	}
//	Now, process all lights from last frame which haven't been processed this frame.
FORALL_OBJS (objP, nObject) {
	nObject = objP->Index ();
	//	In multiplayer games, process even unprocessed OBJECTS every 4th frame, else don't know about CPlayerData sneaking up.
	if ((gameData.render.lights.objects [nObject]) ||
		 (IsMultiGame && (((nObject ^ gameData.app.nFrameCount) & 3) == 0))) {
		if (gameData.render.lights.newObjects [nObject])
			//	Not lit last frame, so we don't need to light it.  (Already lit if casting light this frame.)
			//	But copy value from gameData.render.lights.newObjects to update gameData.render.lights.objects array.
			gameData.render.lights.objects [nObject] = gameData.render.lights.newObjects [nObject];
		else {
			//	Lit last frame, but not this frame.  Get intensity...
			objPos = &objP->info.position.vPos;
			xObjIntensity = ComputeLightIntensity (nObject, &color, &bGotColor);
			if (bGotColor)
				bKeepDynColoring = 1;
			if (xObjIntensity) {
				ApplyLight (xObjIntensity, objP->info.nSegment, objPos, nRenderVertices, gameData.render.lights.vertices, nObject,
								bGotColor ? &color : NULL);
				gameData.render.lights.objects [nObject] = 1;
				}
			else
				gameData.render.lights.objects [nObject] = 0;
			}
		}
	}
lightClusterManager.Set ();
if (!bKeepDynColoring)
	InitDynColoring ();
}

// ----------------------------------------------------------------------------------------------
//compute the average dynamic light in a CSegment.  Takes the CSegment number

fix ComputeSegDynamicLight (int nSegment)
{
short *verts = SEGMENTS [nSegment].m_verts;
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

CObject *oldViewer;
int bResetLightingHack;

#define LIGHT_RATE I2X (4)		//how fast the light ramps up

void StartLightingFrame (CObject *viewer)
{
bResetLightingHack = (viewer != oldViewer);
oldViewer = viewer;
}

// ----------------------------------------------------------------------------------------------
//compute the lighting for an CObject.  Takes a pointer to the CObject,
//and possibly a rotated 3d point.  If the point isn't specified, the
//object's center point is rotated.
fix ComputeObjectLight (CObject *objP, CFixVector *vRotated)
{
#if DBG
   if (!objP)
      return 0;
#endif
	if (!OBJECTS.IsElement (objP))
		return I2X (1);
	int nObject = objP->Index ();
	if (nObject < 0)
		return I2X (1);
	if (nObject >= gameData.objs.nLastObject [0])
		return 0;
	//First, get static light for this CSegment
fix light;
if (gameStates.render.nLightingMethod && !((gameOpts->ogl.bObjLighting) || gameOpts->ogl.bLightObjects)) {
	gameData.objs.color = *lightManager.AvgSgmColor (objP->info.nSegment, &objP->info.position.vPos);
	light = I2X (1);
	}
else
	light = SEGMENTS [objP->info.nSegment].m_xAvgSegLight;
//return light;
//Now, maybe return different value to smooth transitions
if (!bResetLightingHack && (gameData.objs.nLightSig [nObject] == objP->info.nSignature)) {
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
else {		//new CObject, initialize
	gameData.objs.nLightSig [nObject] = objP->info.nSignature;
	gameData.objs.xLight [nObject] = light;
	}
//Next, add in headlight on this CObject
// -- Matt code: light += ComputeHeadlight (vRotated,I2X (1));
light += lightManager.Headlights ().ComputeLightOnObject (objP);
//Finally, add in dynamic light for this CSegment
light += ComputeSegDynamicLight (objP->info.nSegment);
return light;
}

// ----------------------------------------------------------------------------------------------

void ComputeEngineGlow (CObject *objP, fix *xEngineGlowValue)
{
xEngineGlowValue [0] = I2X (1)/5;
if (objP->info.movementType == MT_PHYSICS) {
	if ((objP->info.nType == OBJ_PLAYER) && (objP->mType.physInfo.flags & PF_USES_THRUST) && (objP->info.nId == gameData.multiplayer.nLocalPlayer)) {
		fix thrust_mag = objP->mType.physInfo.thrust.Mag();
		xEngineGlowValue [0] += (FixDiv (thrust_mag,gameData.pig.ship.player->maxThrust)*4)/5;
	}
	else {
		fix speed = objP->mType.physInfo.velocity.Mag();
		xEngineGlowValue [0] += (FixDiv (speed, MAX_VELOCITY) * 3) / 5;
		}
	}
//set value for CPlayerData headlight
if (objP->info.nType == OBJ_PLAYER) {
	if (PlayerHasHeadlight (objP->info.nId) &&  !gameStates.app.bEndLevelSequence)
		xEngineGlowValue [1] = HeadlightIsOn (objP->info.nId) ? -2 : -1;
	else
		xEngineGlowValue [1] = -3;			//don't draw
	}
}

//-----------------------------------------------------------------------------

void FlickerLights (void)
{
	CVariableLight	*flP;

if (!(flP = gameData.render.lights.flicker.Buffer ()))
	return;

	int				l;
	CSide				*sideP;
	short				nSegment, nSide;

for (l = gameData.render.lights.flicker.Length (); l; l--, flP++) {
	//make sure this is actually a light
	nSegment = flP->m_nSegment;
	nSide = flP->m_nSide;
	if (!(SEGMENTS [nSegment].IsDoorWay (nSide, NULL) & WID_RENDER_FLAG))
		continue;
	sideP = SEGMENTS [nSegment].m_sides + nSide;
	if (!(gameData.pig.tex.brightness [sideP->m_nBaseTex] ||
			gameData.pig.tex.brightness [sideP->m_nOvlTex]))
		continue;
	if (flP->m_timer == (fix) 0x80000000)		//disabled
		continue;
	if ((flP->m_timer -= gameData.time.xFrame) < 0) {
		while (flP->m_timer < 0)
			flP->m_timer += flP->m_delay;
		flP->m_mask = ((flP->m_mask & 0x80000000) ? 1 : 0) + (flP->m_mask << 1);
		if (flP->m_mask & 1)
			AddLight (nSegment, nSide);
		else if (!gameOpts->app.bEpilepticFriendly /*EGI_FLAG (bFlickerLights, 1, 0, 1)*/)
			SubtractLight (nSegment, nSide);
		}
	}
}
//-----------------------------------------------------------------------------
//returns ptr to flickering light structure, or NULL if can't find
CVariableLight *FindVariableLight (int nSegment,int nSide)
{
	CVariableLight	*flP;

if ((flP = gameData.render.lights.flicker.Buffer ()))
	for (int l = gameData.render.lights.flicker.Length (); l; l--, flP++)
		if ((flP->m_nSegment == nSegment) && (flP->m_nSide == nSide))	//found it!
			return flP;
return NULL;
}
//-----------------------------------------------------------------------------
//turn flickering off (because light has been turned off)
void DisableVariableLight (int nSegment,int nSide)
{
CVariableLight *flP = FindVariableLight (nSegment, nSide);

if (flP)
	flP->m_timer = 0x80000000;
}

//-----------------------------------------------------------------------------
//turn flickering off (because light has been turned on)
void EnableVariableLight (int nSegment,int nSide)
{
	CVariableLight *flP = FindVariableLight (nSegment, nSide);

if (flP)
	flP->m_timer = 0;
}

//------------------------------------------------------------------------------

int IsLight (int tMapNum)
{
return (tMapNum < MAX_WALL_TEXTURES) ? gameData.pig.tex.brightness [tMapNum] : 0;
}

//	------------------------------------------------------------------------------------------
//cast static light from a CSegment to nearby segments
//these constants should match the ones in seguvs
#define	LIGHT_DISTANCE_THRESHOLD	 (I2X (80))
#define	MAGIC_LIGHT_CONSTANT			 (I2X (16))

#define MAX_CHANGED_SEGS 30
short changedSegs [MAX_CHANGED_SEGS];
int nChangedSegs;

void ApplyLightToSegment (CSegment *segP, CFixVector *vSegCenter, fix xBrightness, int nCallDepth)
{
	CFixVector	rSegmentCenter;
	fix			xDistToRSeg;
	int 			i;
	short			nSide,
					nSegment = segP->Index ();

for (i = 0; i <nChangedSegs; i++)
	if (changedSegs [i] == nSegment)
		break;
if (i == nChangedSegs) {
	rSegmentCenter = segP->Center ();
	xDistToRSeg = CFixVector::Dist(rSegmentCenter, *vSegCenter);

	if (xDistToRSeg <= LIGHT_DISTANCE_THRESHOLD) {
		fix	xLightAtPoint = (xDistToRSeg > I2X (1)) ?
									 FixDiv (MAGIC_LIGHT_CONSTANT, xDistToRSeg) :
									 MAGIC_LIGHT_CONSTANT;

		if (xLightAtPoint >= 0) {
			xLightAtPoint = FixMul (xLightAtPoint, xBrightness);
			if (xLightAtPoint >= I2X (1))
				xLightAtPoint = I2X (1)-1;
			else if (xLightAtPoint <= -I2X (1))
				xLightAtPoint = - (I2X (1)-1);
			segP->m_xAvgSegLight += xLightAtPoint;
			if (segP->m_xAvgSegLight < 0)	// if it went negative, saturate
				segP->m_xAvgSegLight = 0;
			}	//	end if (xLightAtPoint...
		}	//	end if (xDistToRSeg...
	changedSegs [nChangedSegs++] = nSegment;
	}

if (nCallDepth < 2)
	for (nSide=0; nSide<6; nSide++) {
		if (segP->IsDoorWay (nSide, NULL) & WID_RENDPAST_FLAG)
			ApplyLightToSegment (&SEGMENTS [segP->m_children [nSide]], vSegCenter, xBrightness, nCallDepth+1);
		}
}

extern CObject *oldViewer;

//	------------------------------------------------------------------------------------------
//update the xAvgSegLight field in a CSegment, which is used for CObject lighting
//this code is copied from the editor routine calim_process_all_lights ()
void ChangeSegmentLight (short nSegment, short nSide, int dir)
{
	CSegment *segP = SEGMENTS + nSegment;

if (segP->IsDoorWay (nSide, NULL) & WID_RENDER_FLAG) {
	CSide	*sideP = segP->m_sides+nSide;
	fix	xBrightness;
	xBrightness = gameData.pig.tex.tMapInfoP [sideP->m_nBaseTex].lighting + gameData.pig.tex.tMapInfoP [sideP->m_nOvlTex].lighting;
	xBrightness *= dir;
	nChangedSegs = 0;
	if (xBrightness) {
		CFixVector	vSegCenter;
		vSegCenter = segP->Center ();
		ApplyLightToSegment (segP, &vSegCenter, xBrightness, 0);
		}
	}
//this is a horrible hack to get around the horrible hack used to
//smooth lighting values when an CObject moves between segments
oldViewer = NULL;
}

//	------------------------------------------------------------------------------------------

int FindDLIndex (short nSegment, short nSide)
{
if (!gameData.render.lights.deltaIndices.Buffer ())
	return 0;

int	m,
		l = 0,
		r = gameData.render.lights.nStatic - 1;

CLightDeltaIndex* p;
do {
	m = (l + r) / 2;
	p = gameData.render.lights.deltaIndices + m;
	if ((nSegment < p->nSegment) || ((nSegment == p->nSegment) && (nSide < p->nSide)))
		r = m - 1;
	else if ((nSegment > p->nSegment) || ((nSegment == p->nSegment) && (nSide > p->nSide)))
		l = m + 1;
	else {
		if (!m)
			return 0;
		while ((p->nSegment == nSegment) && (p->nSide == nSide)) {
			p--;
			m--;
			}
		return m + 1;
		}
	} while (l <= r);
return 0;
}

//	------------------------------------------------------------------------------------------
//	dir = +1 -> add light
//	dir = -1 -> subtract light
//	dir = 17 -> add 17x light
//	dir =  0 -> you are dumb
void ChangeLight (short nSegment, short nSide, int dir)
{
	int					i, j, k;
	fix					dl, lNew, *pSegLightDelta;
	tUVL*					uvlP;
	CLightDeltaIndex*	dliP;
	CLightDelta*		dlP;
	short					iSeg, iSide;

if ((dir < 0) && lightManager.Delete (nSegment, nSide, -1))
	return;
if (lightManager.Toggle (nSegment, nSide, -1, dir >= 0) >= 0)
	return;
if (gameData.render.lights.deltaIndices.Buffer ()) {
	if (0 > (i = FindDLIndex (nSegment, nSide))) {
#if DBG
		FindDLIndex (nSegment, nSide);
#endif
		return;
		}
	if (!(dliP = gameData.render.lights.deltaIndices + i))
		return;
	for (; i < gameData.render.lights.nStatic; i++, dliP++) {
		iSeg = dliP->nSegment;
		iSide = dliP->nSide;
#if !DBG
		if ((iSeg > nSegment) || ((iSeg == nSegment) && (iSide > nSide)))
			return;
#endif
		if ((iSeg == nSegment) && (iSide == nSide)) {
			if (dliP->index >= gameData.render.lights.deltas.Length ())
				continue;	//ouch - bogus data!
			dlP = gameData.render.lights.deltas + dliP->index;
			for (j = dliP->count; j; j--, dlP++) {
				if (!dlP->bValid)
					continue;	//bogus data!
				uvlP = SEGMENTS [dlP->nSegment].m_sides [dlP->nSide].m_uvls;
				pSegLightDelta = gameData.render.lights.segDeltas + dlP->nSegment * 6 + dlP->nSide;
				for (k = 0; k < 4; k++, uvlP++) {
					dl = dir * dlP->vertLight [k] * DL_SCALE;
					lNew = (uvlP->l += dl);
					if (lNew < 0)
						uvlP->l = 0;
					*pSegLightDelta += dl;
					}
				}
			}
		}
	}
//recompute static light for CSegment
ChangeSegmentLight (nSegment, nSide, dir);
}

//	-----------------------------------------------------------------------------
//	Subtract light cast by a light source from all surfaces to which it applies light.
//	This is precomputed data, stored at static light application time in the editor (the slow lighting function).
// returns 1 if lights actually subtracted, else 0
int SubtractLight (short nSegment, int nSide)
{
if (gameData.render.lights.subtracted [nSegment] & (1 << nSide))
	return 0;
gameData.render.lights.subtracted [nSegment] |= (1 << nSide);
ChangeLight (nSegment, nSide, -1);
return 1;
}

//	-----------------------------------------------------------------------------
//	Add light cast by a light source from all surfaces to which it applies light.
//	This is precomputed data, stored at static light application time in the editor (the slow lighting function).
//	You probably only want to call this after light has been subtracted.
// returns 1 if lights actually added, else 0
int AddLight (short nSegment, int nSide)
{
if (!(gameData.render.lights.subtracted [nSegment] & (1 << nSide)))
	return 0;
gameData.render.lights.subtracted [nSegment] &= ~ (1 << nSide);
ChangeLight (nSegment, nSide, 1);
return 1;
}

//	-----------------------------------------------------------------------------
//	Parse the gameData.render.lights.subtracted array, turning on or off all lights.
void ApplyAllChangedLight (void)
{
	short	i, j;
	ubyte	h;

for (i=0; i <= gameData.segs.nLastSegment; i++) {
	h = gameData.render.lights.subtracted [i];
	for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++)
		if (h & (1 << j))
			ChangeLight (i, j, -1);
	}
}

//	-----------------------------------------------------------------------------
//	Should call this whenever a new mine gets loaded.
//	More specifically, should call this whenever something global happens
//	to change the status of static light in the mine.
void ClearLightSubtracted (void)
{
gameData.render.lights.subtracted.Clear ();
}

//------------------------------------------------------------------------------
//	When loading a saved game, segp->xAvgSegLight is bogus.
//	This is because ApplyAllChangedLight, which is supposed to properly update this value,
//	cannot do so because it needs the original light cast from a light which is no longer there.
//	That is, a light has been blown out, so the texture remaining casts 0 light, but the static light
//	which is present in the xAvgSegLight field contains the light cast from that light.
void ComputeAllStaticLight (void)
{
	int		h, i, j, k;
	CSegment	*segP;
	CSide		*sideP;
	fix		xTotal;

for (i = 0, segP = SEGMENTS.Buffer (); i <= gameData.segs.nLastSegment; i++, segP++) {
	xTotal = 0;
	for (h = j = 0, sideP = segP->m_sides; j < MAX_SIDES_PER_SEGMENT; j++, sideP++) {
		if ((segP->m_children [j] < 0) || sideP->IsWall ()) {
			h++;
			for (k = 0; k < 4; k++)
				xTotal += sideP->m_uvls [k].l;
			}
		}
	SEGMENTS [i].m_xAvgSegLight = h ? xTotal / (h * 4) : 0;
	}
}

//------------------------------------------------------------------------------
// reads a CLightDelta structure from a CFile

void CLightDelta::Read (CFile& cf)
{
nSegment = cf.ReadShort ();
nSide = cf.ReadByte ();
cf.ReadByte ();
if (!(bValid = (nSegment >= 0) && (nSegment < gameData.segs.nSegments) && (nSide >= 0) && (nSide < 6)))
	PrintLog ("Invalid delta light data %d (%d,%d)\n", this - gameData.render.lights.deltas, nSegment, nSide);
cf.Read (vertLight, sizeof (vertLight [0]), sizeofa (vertLight));
}


//------------------------------------------------------------------------------
// reads a CLightDeltaIndex structure from a CFile

void CLightDeltaIndex::Read (CFile& cf)
{
if (gameStates.render.bD2XLights) {
	nSegment = cf.ReadShort ();
	short i = (short) cf.ReadByte ();	// these two bytes contain the side in the lower 3 and the count in the upper 13 bits
	short j = (short) cf.ReadByte ();
	nSide = i & 7;
	count = (j << 5) + ((i >> 3) & 63);
	index = cf.ReadShort ();
	}
else {
	nSegment = cf.ReadShort ();
	nSide = cf.ReadByte ();
	count = cf.ReadByte ();
	index = cf.ReadShort ();
	}
}

//-----------------------------------------------------------------------------

void CVariableLight::Read (CFile& cf)
{
m_nSegment = cf.ReadShort ();
m_nSide = cf.ReadShort ();
m_mask = cf.ReadInt ();
m_timer = cf.ReadFix ();
m_delay = cf.ReadFix ();
}

// ----------------------------------------------------------------------------------------------
//eof
