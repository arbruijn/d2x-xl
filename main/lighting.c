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

/*
 *
 * Lighting functions.
 *
 * Old Log:
 * Revision 1.4  1995/09/20  14:26:12  allender
 * more optimizations(?) ala MK
 *
 * Revision 1.2  1995/07/05  21:27:31  allender
 * new and improved lighting code by MK!
 *
 * Revision 2.1  1995/07/24  13:21:56  john
 * Added new lighting calculation code to speed things up.
 *
 * Revision 2.0  1995/02/27  11:27:33  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.43  1995/02/22  13:57:10  allender
 * remove anonymous union from object structure
 *
 * Revision 1.42  1995/02/13  20:35:07  john
 * Lintized
 *
 * Revision 1.41  1995/02/04  21:43:40  matt
 * Changed an assert() to an int3() and deal with the bad case
 *
 * Revision 1.40  1995/01/15  20:48:27  mike
 * support light field for powerups.
 *
 * Revision 1.39  1994/12/15  13:04:19  mike
 * Replace gameData.multi.players [gameData.multi.nLocalPlayer].time_total references with gameData.app.xGameTime.
 *
 * Revision 1.38  1994/11/28  21:50:41  mike
 * optimizations.
 *
 * Revision 1.37  1994/11/28  01:32:33  mike
 * lighting optimization.
 *
 * Revision 1.36  1994/11/15  12:01:00  john
 * Changed a bunch of code that uses timer_get_milliseconds to
 * timer_get_fixed_Seconds.
 *
 * Revision 1.35  1994/10/31  21:56:07  matt
 * Fixed bug & added error checking
 *
 * Revision 1.34  1994/10/21  11:24:57  mike
 * Trap divide overflows in lighting.
 *
 * Revision 1.33  1994/10/08  14:49:11  matt
 * If viewer changed, don't do smooth lighting hack
 *
 * Revision 1.32  1994/09/25  23:41:07  matt
 * Changed the object load & save code to read/write the structure fields one
 * at a time (rather than the whole structure at once).  This mean that the
 * object structure can be changed without breaking the load/save functions.
 * As a result of this change, the local_object data can be and has been
 * incorporated into the object array.  Also, timeleft is now a property
 * of all gameData.objs.objects, and the object structure has been otherwise cleaned up.
 *
 * Revision 1.31  1994/09/25  15:45:15  matt
 * Added OBJ_LIGHT, a type of object that casts light
 * Added generalized lifeleft, and moved it to local_object
 *
 * Revision 1.30  1994/09/11  15:48:27  mike
 * Use VmVecMagQuick in place of VmVecMag in point_dist computation.
 *
 * Revision 1.29  1994/09/08  21:44:49  matt
 * Made lighting ramp 4x as fast; made only static (ambient) light ramp
 * up, but not headlight & dynamic light
 *
 * Revision 1.28  1994/09/02  14:00:07  matt
 * Simplified ExplodeObject() & mutliple-stage explosions
 *
 * Revision 1.27  1994/08/29  19:06:44  mike
 * Make lighting proportional to square of distance, not linear.
 *
 * Revision 1.26  1994/08/25  18:08:38  matt
 * Made muzzle flash cast 3x as much light
 *
 * Revision 1.25  1994/08/23  16:38:31  mike
 * Key weapon light off bitmaps.tbl.
 *
 * Revision 1.24  1994/08/13  12:20:44  john
 * Made the networking uise the gameData.multi.players array.
 *
 * Revision 1.23  1994/08/12  22:42:18  john
 * Took away Player_stats; added gameData.multi.players array.
 *
 * Revision 1.22  1994/07/06  10:19:22  matt
 * Changed include
 *
 * Revision 1.21  1994/06/28  13:20:22  mike
 * Oops, fixed a dumb typo.
 *
 * Revision 1.20  1994/06/28  12:53:25  mike
 * Change lighting function for flares, make brighter and asynchronously flicker.
 *
 * Revision 1.19  1994/06/27  18:31:15  mike
 * Add flares.
 *
 * Revision 1.18  1994/06/20  13:41:17  matt
 * Added time-based gradual lighting hack for gameData.objs.objects
 * Took out strobing robots
 *
 * Revision 1.17  1994/06/19  16:25:54  mike
 * Optimize lighting.
 *
 * Revision 1.16  1994/06/17  18:08:08  mike
 * Make robots cast more and variable light.
 *
 * Revision 1.15  1994/06/13  15:15:55  mike
 * Fix phantom light, every 64K milliseconds, muzzle flash would flash again.
 *
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

extern vms_vector player_thrust;

typedef struct {
  int    nTexture;
  int		nBrightness;
} tTexBright;

#define	NUM_LIGHTS_D1     48
#define	NUM_LIGHTS_D2     85
#define	MAX_BRIGHTNESS		F2_0

tTexBright texBrightD1 [NUM_LIGHTS_D1] = {
	{250, 0x00b333L}, {251, 0x008000L}, {252, 0x008000L}, {253, 0x008000L},
	{264, 0x01547aL}, {265, 0x014666L}, {268, 0x014666L}, {278, 0x014cccL},
	{279, 0x014cccL}, {280, 0x011999L}, {281, 0x014666L}, {282, 0x011999L},
	{283, 0x0107aeL}, {284, 0x0107aeL}, {285, 0x011999L}, {286, 0x014666L},
	{287, 0x014666L}, {288, 0x014666L}, {289, 0x014666L}, {292, 0x010cccL},
	{293, 0x010000L}, {294, 0x013333L}, {330, 0x010000L}, {333, 0x010000L}, 
	{341, 0x010000L}, {343, 0x010000L}, {345, 0x010000L}, {347, 0x010000L}, 
	{349, 0x010000L}, {351, 0x010000L}, {352, 0x010000L}, {354, 0x010000L}, 
	{355, 0x010000L}, {356, 0x020000L}, {357, 0x020000L}, {358, 0x020000L}, 
	{359, 0x020000L}, {360, 0x020000L}, {361, 0x020000L}, {362, 0x020000L}, 
	{363, 0x020000L}, {364, 0x020000L}, {365, 0x020000L}, {366, 0x020000L}, 
	{367, 0x020000L}, {368, 0x020000L}, {369, 0x020000L}, {370, 0x020000L}
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

int AddOglHeadLight (object *objP);
void RemoveOglHeadLight (object *objP);
void UpdateOglHeadLight (void);

//--------------------------------------------------------------------------

void InitTextureBrightness (void)
{
	tTexBright	*ptb = gameStates.app.bD1Mission ? texBrightD1  : texBrightD2 ;
	int			i = (gameStates.app.bD1Mission ? sizeof (texBrightD1) : sizeof (texBrightD2)) / sizeof (tTexBright);

memset (gameData.pig.tex.brightness, 0, sizeof (gameData.pig.tex.brightness));
while (i) {
	--i;
	gameData.pig.tex.brightness [ptb [i].nTexture] = 
		((ptb [i].nBrightness * 100 + MAX_BRIGHTNESS / 2) / MAX_BRIGHTNESS) * (MAX_BRIGHTNESS / 100);
	}
}

// ----------------------------------------------------------------------------------------------
//	Return true if we think vertex vertnum is visible from segment segnum.
//	If some amount of time has gone by, then recompute, else use cached value.
int LightingCacheVisible(int vertnum, int segnum, int objnum, vms_vector *obj_pos, int obj_seg, vms_vector *vertpos)
{
	int	cache_val, cache_frame, cache_vis;

	cache_val = Lighting_cache [((segnum << LIGHTING_CACHE_SHIFT) ^ vertnum) & (LIGHTING_CACHE_SIZE-1)];

	cache_frame = cache_val >> 1;
	cache_vis = cache_val & 1;

Cache_lookups++;
	if ((cache_frame == 0) || (cache_frame + Lighting_frame_delta <= gameData.app.nFrameCount)) {
		int			bApplyLight=0;
		fvi_query	fq;
		fvi_info		hit_data;
		int			segnum, hit_type;

		segnum = -1;

		#ifndef NDEBUG
		segnum = FindSegByPoint(obj_pos, obj_seg);
		if (segnum == -1) {
			Int3();		//	Obj_pos is not in obj_seg!
			return 0;		//	Done processing this object.
		}
		#endif

		fq.p0						= obj_pos;
		fq.startseg				= obj_seg;
		fq.p1						= vertpos;
		fq.rad					= 0;
		fq.thisobjnum			= objnum;
		fq.ignore_obj_list	= NULL;
		fq.flags					= FQ_TRANSWALL;

		hit_type = FindVectorIntersection(&fq, &hit_data);

		// gameData.ai.vHitPos = gameData.ai.hitData.hit_pnt;
		// gameData.ai.nHitSeg = gameData.ai.hitData.hit_seg;

		if (hit_type == HIT_OBJECT)
			Int3();	//	Hey, we're not supposed to be checking gameData.objs.objects!

		if (hit_type == HIT_NONE)
			bApplyLight = 1;
		else if (hit_type == HIT_WALL) {
			fix	dist_dist;
			dist_dist = VmVecDistQuick(&hit_data.hit_pnt, obj_pos);
			if (dist_dist < F1_0/4) {
				bApplyLight = 1;
				// -- Int3();	//	Curious, did fvi detect intersection with wall containing vertex?
			}
		}
		Lighting_cache [((segnum << LIGHTING_CACHE_SHIFT) ^ vertnum) & (LIGHTING_CACHE_SIZE-1)] = bApplyLight + (gameData.app.nFrameCount << 1);
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
if (!gameOpts->ogl.bUseLighting && bInitDynColoring) {
	bInitDynColoring = 0;
	memset (bGotDynColor, 0, sizeof (bGotDynColor));
	}
bGotGlobalDynColor = 0;
bStartDynColoring = 0;
}

// ----------------------------------------------------------------------------------------------

void SetDynColor (tRgbColorf *color, tRgbColorf *pDynColor, int vertnum, char *pbGotDynColor, int bForce)
{
if (gameOpts->ogl.bUseLighting)
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
	vms_vector	*obj_pos, 
	int			nRenderVertices, 
	short			*renderVertices, 
	int			objnum,
	tRgbColorf	*color)
{
	int	vv, bUseColor, bForceColor;
	object *objP = gameData.objs.objects + objnum;

if (gameStates.ogl.bHaveLights && gameOpts->ogl.bUseLighting) {
	if (objP->type == OBJ_PLAYER) {
		if (EGI_FLAG (bHeadLights, 0, 0)) {
			if (!(gameData.multi.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT_ON)) 
				RemoveOglHeadLight (objP);
			else if (gameData.render.lights.ogl.nHeadLights [objP->id] < 0)
				gameData.render.lights.ogl.nHeadLights [objP->id] = AddOglHeadLight (objP);
			}
		if (IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bDarkness)
			return;
		}
	else if ((objP->type == OBJ_POWERUP) && !EGI_FLAG (bPowerupLights, 0, 0))
		return;
	AddOglLight (color, xObjIntensity, -1, -1, objnum);
	return;
	}
if (xObjIntensity) {
	fix	obji_64 = xObjIntensity*64;

	bUseColor = (color != NULL); //&& (color->red < 1.0 || color->green < 1.0 || color->blue < 1.0);
	bForceColor = 0;
	// for pretty dim sources, only process vertices in object's own segment.
	//	12/04/95, MK, markers only cast light in own segment.
	if ((abs(obji_64) <= F1_0*8) || (objP->type == OBJ_MARKER)) {
		short *vp = gameData.segs.segments [obj_seg].verts;

		for (vv=0; vv<MAX_VERTICES_PER_SEGMENT; vv++) {
			int			vertnum;
			vms_vector	*vertpos;
			fix			dist;

			vertnum = vp [vv];
#if FLICKERFIX == 0
			if (/*(gameOpts->render.color.bAmbientLight && color) ||*/ ((vertnum ^ gameData.app.nFrameCount) & 1))
#endif
			{
				vertpos = gameData.segs.vertices+vertnum;
				dist = VmVecDistQuick (obj_pos, vertpos) / 4;
				dist = FixMul(dist, dist);
				if (dist < abs (obji_64)) {
					if (dist < MIN_LIGHT_DIST)
						dist = MIN_LIGHT_DIST;

					dynamicLight [vertnum] += fixdiv(xObjIntensity, dist);
					if (bUseColor)
						SetDynColor (color, NULL, vertnum, NULL, 0);
				}
			}
		}
	} else {
		int	headlight_shift = 0;
		fix	max_headlight_dist = F1_0*200;

		if (objP->type == OBJ_PLAYER)
			if (gameData.multi.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT_ON) {
				gameStates.render.bHeadlightOn = 1;
				headlight_shift = 3;
				if (color) {
					bUseColor = bForceColor = 1;
					color->red = color->green = color->blue = 1.0;
					}
				if (objP->id != gameData.multi.nLocalPlayer) {
					vms_vector	tvec;
					fvi_query	fq;
					fvi_info		hit_data;
					int			fate;

					VmVecScaleAdd(&tvec, obj_pos, &objP->orient.fvec, F1_0*200);

					fq.startseg				= objP->segnum;
					fq.p0						= obj_pos;
					fq.p1						= &tvec;
					fq.rad					= 0;
					fq.thisobjnum			= objnum;
					fq.ignore_obj_list	= NULL;
					fq.flags					= FQ_TRANSWALL;

					fate = FindVectorIntersection(&fq, &hit_data);
					if (fate != HIT_NONE) {
						VmVecSub(&tvec, &hit_data.hit_pnt, obj_pos);
						max_headlight_dist = VmVecMagQuick(&tvec) + F1_0*4;
					}
				}
			}
		// -- for (vv=gameData.app.nFrameCount&1; vv<nRenderVertices; vv+=2) {
		for (vv=0; vv<nRenderVertices; vv++) {
			int			vertnum;
			vms_vector	*vertpos;
			fix			dist;
			int			bApplyLight;

			vertnum = renderVertices [vv];
#if FLICKERFIX == 0
			if (/*(gameOpts->render.color.bAmbientLight && color) ||*/ ((vertnum ^ gameData.app.nFrameCount) & 1))
#endif
			{
				vertpos = gameData.segs.vertices + vertnum;
				dist = VmVecDistQuick(obj_pos, vertpos);
				bApplyLight = 0;

				if ((dist >> headlight_shift) < abs(obji_64)) {

					if (dist < MIN_LIGHT_DIST)
						dist = MIN_LIGHT_DIST;

					//if (Use_fvi_lighting) {
					//	if (LightingCacheVisible(vertnum, obj_seg, objnum, obj_pos, obj_seg, vertpos)) {
					//		ApplyLight = 1;
					//	}
					//} else
						bApplyLight = 1;

					if (bApplyLight) {
						if (bUseColor)
							SetDynColor (color, NULL, vertnum, NULL, bForceColor);
						if (headlight_shift) {
							fix			dot;
							vms_vector	vec_to_point;

							VmVecSub(&vec_to_point, vertpos, obj_pos);
							VmVecNormalizeQuick(&vec_to_point);		//	MK, Optimization note: You compute distance about 15 lines up, this is partially redundant
							dot = VmVecDot(&vec_to_point, &objP->orient.fvec);
							if (dot < F1_0/2)
								dynamicLight [vertnum] += fixdiv(xObjIntensity, FixMul(HEADLIGHT_SCALE, dist));	//	Do the normal thing, but darken around headlight.
							else {
								if (gameData.app.nGameMode & GM_MULTI) {
									if (dist < max_headlight_dist)
										dynamicLight [vertnum] += FixMul(FixMul(dot, dot), xObjIntensity)/8;
									}
								else
									dynamicLight [vertnum] += FixMul(FixMul(dot, dot), xObjIntensity)/8;
								}
							}
						else
							dynamicLight [vertnum] += fixdiv(xObjIntensity, dist);
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
	fix current_time;
	int	i;
	short	time_since_flash;

	current_time = TimerGetFixedSeconds();

	for (i=0; i<MUZZLE_QUEUE_MAX; i++) {
		if (gameData.muzzle.info [i].create_time) {
			time_since_flash = current_time - gameData.muzzle.info [i].create_time;
			if (time_since_flash < FLASH_LEN_FIXED_SECONDS)
				ApplyLight ((FLASH_LEN_FIXED_SECONDS - time_since_flash) * FLASH_SCALE, 
								gameData.muzzle.info [i].segnum, &gameData.muzzle.info [i].pos, 
								nRenderVertices, renderVertices, -1, NULL);
			else
				gameData.muzzle.info [i].create_time = 0;		// turn off this muzzle flash
		}
	}
}

//	Translation table to make flares flicker at different rates
fix	Obj_light_xlate [16] =
	{0x1234, 0x3321, 0x2468, 0x1735,
	 0x0123, 0x19af, 0x3f03, 0x232a,
	 0x2123, 0x39af, 0x0f03, 0x132a,
	 0x3123, 0x29af, 0x1f03, 0x032a};

//	Flag array of gameData.objs.objects lit last frame.  Guaranteed to process this frame if lit last frame.
sbyte   lightingObjects [MAX_OBJECTS];

#define	MAX_HEADLIGHTS	8
object	*Headlights [MAX_HEADLIGHTS];
int		Num_headlights;

// ---------------------------------------------------------

fix ComputeLightIntensity (int objnum, tRgbColorf *color, char *pbGotColor)
{
	object		*objP = gameData.objs.objects+objnum;
	int			objtype = objP->type;
   fix hoardlight,s;

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
		 else if ((gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) && gameData.multi.players [objP->id].secondary_ammo [PROXIMITY_INDEX]) {
		
		// If hoard game and player, add extra light based on how many orbs you have
		// Pulse as well.

		  	hoardlight=i2f(gameData.multi.players [objP->id].secondary_ammo [PROXIMITY_INDEX])/2; //i2f(12);
			hoardlight++;
		   fix_sincos ((gameData.app.xGameTime/2) & 0xFFFF,&s,NULL); // probably a bad way to do it
			s+=F1_0; 
			s>>=1;
			hoardlight=FixMul (s,hoardlight);
		   return (hoardlight);
		  }
		else if (objP->id == gameData.multi.nLocalPlayer) {
			return max(VmVecMagQuick(&player_thrust)/4, F1_0*2) + F1_0/2;
			}
		else {
			return max(VmVecMagQuick(&objP->mtype.phys_info.thrust)/4, F1_0*2) + F1_0/2;
			}
		break;

	case OBJ_FIREBALL:
		if ((objP->id != 0xff) && (objP->render_type != RT_THRUSTER)) {
			vclip *vcP = gameData.eff.vClips [0] + objP->id;
			fix xLight = vcP->light_value;
#if 0
			int i, j;
			grs_bitmap *bmP;
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
			if (objP->render_type != RT_THRUSTER)
				xLight /= 8;
#endif
			if (objP->lifeleft < F1_0*4)
				return FixMul (fixdiv(objP->lifeleft, 
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
			return 2* (min(tval, objP->lifeleft) + ((gameData.app.xGameTime ^ Obj_light_xlate [objnum&0x0f]) & 0x3fff));
		else
			return tval;
	}

	case OBJ_MARKER: {
		fix	lightval = objP->lifeleft;
		lightval &= 0xffff;
		lightval = 8 * abs(F1_0/2 - lightval);
		if (objP->lifeleft < F1_0*1000)
			objP->lifeleft += F1_0;	//	Make sure this object doesn't go out.
		color->red = 0.1f;
		color->green = 1.0f;
		color->blue = 0.25f;
		*pbGotColor = 1;
		return lightval;
	}

	case OBJ_POWERUP:
		return gameData.objs.pwrUp.info [objP->id].light;
		break;
	case OBJ_DEBRIS:
		return F1_0/4;
		break;

	case OBJ_LIGHT:
		return objP->ctype.light_info.intensity;
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
	int			objnum, vertnum, segnum;
	int			nRenderVertices;
	short			renderVertices [MAX_VERTICES];
	sbyte			render_vertex_flags [MAX_VERTICES];
	int			render_seg, v;
	sbyte			newLightingObjects [MAX_OBJECTS];
	char			bGotColor, bKeepDynColoring = 0;
	object		*objP;
	vms_vector	*objPos;
	fix			xObjIntensity;
	tRgbColorf	color;

	Num_headlights = 0;

if (!gameOpts->render.bDynamicLight)
	return;

memset(render_vertex_flags, 0, gameData.segs.nLastVertex+1);
bStartDynColoring = 1;
if (bInitDynColoring) {
	InitDynColoring ();
	}

//	Create list of vertices that need to be looked at for setting of ambient light.
nRenderVertices = 0;
if (!gameOpts->ogl.bUseLighting) {
	for (render_seg=0; render_seg<nRenderSegs; render_seg++) {
		segnum = nRenderList [render_seg];
		if (segnum != -1) {
			short	*vp = gameData.segs.segments [segnum].verts;
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++) {
				int	vnum = vp [v];
				if (vnum<0 || vnum>gameData.segs.nLastVertex) {
					Int3();		//invalid vertex number
					continue;	//ignore it, and go on to next one
					}
				if (!render_vertex_flags [vnum]) {
					render_vertex_flags [vnum] = 1;
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
	segnum = nRenderList [render_seg];
	objnum = gameData.segs.segments [segnum].objects;

	while (objnum != -1) {
		objP = gameData.objs.objects + objnum;
		objPos = &objP->pos;

		if (objP->type == OBJ_FIREBALL)
			objP = objP;
		xObjIntensity = ComputeLightIntensity (objnum, &color, &bGotColor);
		if (bGotColor)
			bKeepDynColoring = 1;
		if (xObjIntensity) {
			ApplyLight (xObjIntensity, objP->segnum, objPos, nRenderVertices, renderVertices, OBJ_IDX (objP), &color);
			newLightingObjects [objnum] = 1;
			}
		objnum = objP->next;
		}
	}

//	Now, process all lights from last frame which haven't been processed this frame.
for (objnum = 0; objnum <= gameData.objs.nLastObject; objnum++) {
	//	In multiplayer games, process even unprocessed gameData.objs.objects every 4th frame, else don't know about player sneaking up.
	if ((lightingObjects [objnum]) || 
		 ((gameData.app.nGameMode & GM_MULTI) && (((objnum ^ gameData.app.nFrameCount) & 3) == 0))) {
		if (newLightingObjects [objnum])
			//	Not lit last frame, so we don't need to light it.  (Already lit if casting light this frame.)
			//	But copy value from newLightingObjects to update lightingObjects array.
			lightingObjects [objnum] = newLightingObjects [objnum];
		else {
			//	Lit last frame, but not this frame.  Get intensity...
			objP = gameData.objs.objects + objnum;
			objPos = &objP->pos;

			xObjIntensity = ComputeLightIntensity (objnum, &color, &bGotColor);
			if (bGotColor)
				bKeepDynColoring = 1;
			if (xObjIntensity) {
				ApplyLight (xObjIntensity, objP->segnum, objPos, nRenderVertices, renderVertices, objnum, 
								bGotColor ? &color : NULL);
				lightingObjects [objnum] = 1;
				} 
			else
				lightingObjects [objnum] = 0;
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
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendFlags((char) gameData.multi.nLocalPlayer);		
#endif
	}
}

// ---------------------------------------------------------

#define HEADLIGHT_BOOST_SCALE 8		//how much to scale light when have headlight boost

fix	Beam_brightness = (F1_0/2);	//global saying how bright the light beam is

#define MAX_DIST_LOG	6							//log(MAX_DIST-expressed-as-integer)
#define MAX_DIST		(f1_0<<MAX_DIST_LOG)	//no light beyond this dist

fix compute_headlight_light_on_object(object *objP)
{
	int	i;
	fix	light;

	//	Let's just illuminate players and robots for speed reasons, ok?
	if ((objP->type != OBJ_ROBOT) && (objP->type	!= OBJ_PLAYER))
		return 0;

	light = 0;

	for (i=0; i<Num_headlights; i++) {
		fix			dot, dist;
		vms_vector	vecToObj;
		object		*lightObjP;

		lightObjP = Headlights [i];

		VmVecSub(&vecToObj, &objP->pos, &lightObjP->pos);
		dist = VmVecNormalizeQuick(&vecToObj);
		if (dist > 0) {
			dot = VmVecDot(&lightObjP->orient.fvec, &vecToObj);

			if (dot < F1_0/2)
				light += fixdiv(HEADLIGHT_SCALE, FixMul(HEADLIGHT_SCALE, dist));	//	Do the normal thing, but darken around headlight.
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
// -- Unused -- fix compute_headlight_light(vms_vector *point,fix face_light)
// -- Unused -- {
// -- Unused -- 	fix light;
// -- Unused -- 	int use_beam = 0;		//flag for beam effect
// -- Unused --
// -- Unused -- 	light = Beam_brightness;
// -- Unused --
// -- Unused -- 	if ((gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT) && (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON) && gameData.objs.viewer==&gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].objnum] && gameData.multi.players [gameData.multi.nLocalPlayer].energy > 0) {
// -- Unused -- 		light *= HEADLIGHT_BOOST_SCALE;
// -- Unused -- 		use_beam = 1;	//give us beam effect
// -- Unused -- 	}
// -- Unused --
// -- Unused -- 	if (light) {				//if no beam, don't bother with the rest of this
// -- Unused -- 		fix point_dist;
// -- Unused --
// -- Unused -- 		point_dist = VmVecMagQuick(point);
// -- Unused --
// -- Unused -- 		if (point_dist >= MAX_DIST)
// -- Unused --
// -- Unused -- 			light = 0;
// -- Unused --
// -- Unused -- 		else {
// -- Unused -- 			fix dist_scale,face_scale;
// -- Unused --
// -- Unused -- 			dist_scale = (MAX_DIST - point_dist) >> MAX_DIST_LOG;
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
// -- Unused -- 					beam_scale = fixdiv(point->z,point_dist);
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
//compute the average dynamic light in a segment.  Takes the segment number
fix compute_seg_dynamic_light(int segnum)
{
	segment *seg = gameData.segs.segments + segnum;
	short *verts = seg->verts;
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
object *old_viewer;
int reset_lighting_hack;

#define LIGHT_RATE i2f(4)		//how fast the light ramps up

void StartLightingFrame(object *viewer)
{
reset_lighting_hack = (viewer != old_viewer);
old_viewer = viewer;
}

// ----------------------------------------------------------------------------------------------
//compute the lighting for an object.  Takes a pointer to the object,
//and possibly a rotated 3d point.  If the point isn't specified, the
//object's center point is rotated.
fix ComputeObjectLight(object *objP,vms_vector *rotated_pnt)
{
	fix light;
	g3s_point objpnt;
	int objnum = OBJ_IDX (objP);

	if (!rotated_pnt) {
		G3TransformAndEncodePoint(&objpnt,&objP->pos);
		rotated_pnt = &objpnt.p3_vec;
	}
	//First, get static light for this segment
	light = gameData.segs.segment2s [objP->segnum].static_light;
	//return light;
	//Now, maybe return different value to smooth transitions
	if (!reset_lighting_hack && (object_sig [objnum] == objP->signature)) {
		fix delta_light,frame_delta;

		delta_light = light - object_light [objnum];
		frame_delta = FixMul(LIGHT_RATE,gameData.app.xFrameTime);
		if (abs(delta_light) <= frame_delta)
			object_light [objnum] = light;		//we've hit the goal
		else
			if (delta_light < 0)
				light = object_light [objnum] -= frame_delta;
			else
				light = object_light [objnum] += frame_delta;
	}
	else {		//new object, initialize
		object_sig [objnum] = objP->signature;
		object_light [objnum] = light;
	}
	//Next, add in headlight on this object
	// -- Matt code: light += compute_headlight_light(rotated_pnt,f1_0);
	light += compute_headlight_light_on_object(objP);
	//Finally, add in dynamic light for this segment
	light += compute_seg_dynamic_light(objP->segnum);
	return light;
}

// ----------------------------------------------------------------------------------------------

void ComputeEngineGlow (object *objP, fix *engine_glow_value)
{
engine_glow_value [0] = f1_0/5;
if (objP->movement_type == MT_PHYSICS) {
	if ((objP->type==OBJ_PLAYER) && (objP->mtype.phys_info.flags & PF_USES_THRUST) && (objP->id==gameData.multi.nLocalPlayer)) {
		fix thrust_mag = VmVecMagQuick(&objP->mtype.phys_info.thrust);
		engine_glow_value [0] += (fixdiv(thrust_mag,gameData.pig.ship.player->max_thrust)*4)/5;
	}
	else {
		fix speed = VmVecMagQuick(&objP->mtype.phys_info.velocity);
		engine_glow_value [0] += (fixdiv(speed,MAX_VELOCITY)*3)/5;
		}
	}
//set value for player headlight
if (objP->type == OBJ_PLAYER) {
	if ((gameData.multi.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT) && 
		 !gameStates.app.bEndLevelSequence)
		engine_glow_value [1] =  (gameData.multi.players [objP->id].flags & PLAYER_FLAGS_HEADLIGHT_ON) ? -2 : -1;
	else
		engine_glow_value [1] = -3;			//don't draw
	}
}

//-----------------------------------------------------------------------------

void FlickerLights ()
{
	flickering_light	*flP = gameData.render.lights.flicker.lights;
	segment				*segP;
	side					*sideP;
	int					l;

for (l = 0; l < gameData.render.lights.flicker.nLights; l++, flP++) {
	segP = gameData.segs.segments + flP->segnum;
	//make sure this is actually a light
	if (!(WALL_IS_DOORWAY (segP, flP->sidenum, NULL) & WID_RENDER_FLAG))
		continue;
	sideP = segP->sides + flP->sidenum;
	if (!(gameData.pig.tex.brightness [sideP->tmap_num] ||
			gameData.pig.tex.brightness [sideP->tmap_num2 & 0x3fff] ||
		   (gameData.pig.tex.pTMapInfo [sideP->tmap_num].lighting | 
			   gameData.pig.tex.pTMapInfo [sideP->tmap_num2 & 0x3fff].lighting)))
		continue;
	if (flP->timer == 0x80000000)		//disabled
		continue;
	if ((flP->timer -= gameData.app.xFrameTime) < 0) {
		while (flP->timer < 0)
			flP->timer += flP->delay;
		flP->mask = ((flP->mask & 0x80000000) ? 1 : 0) + (flP->mask << 1);
		if (flP->mask & 1)
			AddLight (flP->segnum, flP->sidenum);
		else
			SubtractLight (flP->segnum, flP->sidenum);
		}
	}
}

//-----------------------------------------------------------------------------
//returns ptr to flickering light structure, or NULL if can't find
flickering_light *FindFlicker (int segnum,int sidenum)
{
	int l;
	flickering_light *flP = gameData.render.lights.flicker.lights;

for (l = 0; l < gameData.render.lights.flicker.nLights; l++, flP++)
	if ((flP->segnum == segnum) && (flP->sidenum == sidenum))	//found it!
		return flP;
return NULL;
}

//-----------------------------------------------------------------------------
//turn flickering off (because light has been turned off)
void DisableFlicker (int segnum,int sidenum)
{
	flickering_light *flP = FindFlicker (segnum ,sidenum);

if (flP)
	flP->timer = 0x80000000;
}

//-----------------------------------------------------------------------------
//turn flickering off (because light has been turned on)
void EnableFlicker (int segnum,int sidenum)
{
	flickering_light *flP = FindFlicker (segnum, sidenum);

if (flP)
	flP->timer = 0;
}


#ifdef EDITOR

//returns 1 if ok, 0 if error
int AddFlicker (int segnum, int sidenum, fix delay, unsigned long mask)
{
	int l;
	flickering_light *flP;

#if TRACE
	//con_printf (CON_DEBUG,"AddFlicker: %d:%d %x %x\n",segnum,sidenum,delay,mask);
#endif
	//see if there's already an entry for this seg/side

	flP = gameData.render.lights.flicker.lights;

	for (l=0;l<gameData.render.lights.flicker.nLights;l++,flP++)
		if (flP->segnum == segnum && flP->sidenum == sidenum)	//found it!
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

	flP->segnum = segnum;
	flP->sidenum = sidenum;
	flP->delay = flP->timer = delay;
	flP->mask = mask;

	return 1;
}

#endif

//------------------------------------------------------------------------------

unsigned GetOglLightHandle (void)
{
#if USE_OGL_LIGHTS
	GLint	nMaxLights;

if (gameData.render.lights.ogl.nLights >= MAX_SEGMENTS)
	return 0xffffffff;
glGetIntegerv (GL_MAX_LIGHTS, &nMaxLights);
if (gameData.render.lights.ogl.nLights >= nMaxLights)
	return 0xffffffff;
return (unsigned) (GL_LIGHT0 + gameData.render.lights.ogl.nLights);
#else
return 0xffffffff;
#endif
}

//------------------------------------------------------------------------------

void SetOglLightColor (short nLight, float red, float green, float blue, float brightness)
{
	tOglLight	*pl = gameData.render.lights.ogl.lights + gameData.render.lights.ogl.nLights;
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

void SetOglLightPos (short nObject)
{
if (gameStates.ogl.bHaveLights && gameOpts->ogl.bUseLighting) {
	int	nLight = gameData.render.lights.ogl.owners [nObject];

	if (nLight >= 0)
		gameData.render.lights.ogl.lights [nLight].vPos = gameData.objs.objects [nObject].pos;
	}
}

//------------------------------------------------------------------------------

short FindOglLight (short nSegment, short nSide, short nObject)
{
if (gameStates.ogl.bHaveLights && gameOpts->ogl.bUseLighting) {
		tOglLight	*pl = gameData.render.lights.ogl.lights;
		short			i;

	if (nObject >= 0)
		return gameData.render.lights.ogl.owners [nObject];
	if (nSegment >= 0)
		for (i = 0; i < gameData.render.lights.ogl.nLights; i++, pl++)
			if ((pl->nSegment == nSegment) && (pl->nSide == nSide))
				return i;
	}
return -1;
}

//------------------------------------------------------------------------------

short UpdateOglLight (tRgbColorf *pc, float brightness, short nSegment, short nSide, short nObject)
{
	short	nLight = FindOglLight (nSegment, nSide, nObject);
	
if (nLight >= 0) {
	tOglLight *pl = gameData.render.lights.ogl.lights + nLight;
	if (!pc)
		pc = &pl->color;
	if ((pl->brightness != brightness) || 
		 (pl->color.red != pc->red) || (pl->color.green != pc->green) || (pl->color.blue != pc->blue))
		SetOglLightColor (nLight, pc->red, pc->green, pc->blue, brightness);
	}
return nLight;
}

//------------------------------------------------------------------------------

int LastEnabledOglLight (void)
{
	int	i = gameData.render.lights.ogl.nLights;

while (i)
	if (gameData.render.lights.ogl.lights [--i].bState)
		return i;
return -1;
}

//------------------------------------------------------------------------------

void RefreshOglLight (tOglLight *pl)
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

void SwapOglLights (tOglLight *pl1, tOglLight *pl2)
{
if (pl1 != pl2) {
		tOglLight	h;

	h = *pl1;
	*pl1 = *pl2;
	*pl2 = h;
#if USE_OGL_LIGHTS
	pl1->handle = (unsigned) (GL_LIGHT0 + (pl1 - gameData.render.lights.ogl.lights));
	pl2->handle = (unsigned) (GL_LIGHT0 + (pl2 - gameData.render.lights.ogl.lights));
#endif
	if (pl1->nObject >= 0)
		gameData.render.lights.ogl.owners [pl1->nObject] = pl1 - gameData.render.lights.ogl.lights;
	if (pl2->nObject >= 0)
		gameData.render.lights.ogl.owners [pl2->nObject] = pl2 - gameData.render.lights.ogl.lights;
	}
}

//------------------------------------------------------------------------------

int ToggleOglLight (short nSegment, short nSide, short nObject, int bState)
{
	short nLight = FindOglLight (nSegment, nSide, nObject);
	
if (nLight >= 0) {
	tOglLight *pl = gameData.render.lights.ogl.lights + nLight;
#if 1
	pl->bState = bState;
#else
	if (pl->bState != bState) {
		int i = LastEnabledOglLight ();
		if (bState) {
			SwapOglLights (pl, gameData.render.lights.ogl.lights + i + 1);
			pl = gameData.render.lights.ogl.lights + i + 1;
#if USE_OGL_LIGHTS
			glEnable (pl->handle);
			RefreshOglLight (pl);
#endif
			}
		else {
			SwapOglLights (pl, gameData.render.lights.ogl.lights + i);
#if USE_OGL_LIGHTS
			RefreshOglLight (pl);
#endif
			pl = gameData.render.lights.ogl.lights + i;
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

int AddOglLight (tRgbColorf *pc, fix xBrightness, short nSegment, short nSide, short nObject)
{
	tOglLight	*pl;
	short			h, i;
#if USE_OGL_LIGHTS
	GLint			nMaxLights;
#endif

if (0 <= (h = UpdateOglLight (pc, f2fl (xBrightness), nSegment, nSide, nObject)))
	return h;
if (!pc)
	return -1;
if ((pc->red == 0.0f) && (pc->green == 0.0f) && (pc->blue == 0.0f))
	return -1;
if (gameData.render.lights.ogl.nLights >= MAX_OGL_LIGHTS) {
	gameStates.ogl.bHaveLights = 0;
	return -1;	//too many lights
	}
#if USE_OGL_LIGHTS
glGetIntegerv (GL_MAX_LIGHTS, &nMaxLights);
if (gameData.render.lights.ogl.nLights >= MAX_OGL_LIGHTS) {
	gameStates.ogl.bHaveLights = 0;
	return -1;	//too many lights
	}
#endif
i = gameData.render.lights.ogl.nLights; //LastEnabledOglLight () + 1;
pl = gameData.render.lights.ogl.lights + i;
#if USE_OGL_LIGHTS
pl->handle = GetOglLightHandle (); 
if (pl->handle == 0xffffffff)
	return -1;
#endif
#if 0
if (i < gameData.render.lights.ogl.nLights)
	SwapOglLights (pl, gameData.render.lights.ogl.lights + gameData.render.lights.ogl.nLights);
#endif
pl->nSegment = nSegment;
pl->nSide = nSide;
pl->nObject = nObject;
pl->bState = 1;
pl->bSpot = 0;
pl->nType = (nObject < 0) ? (nSegment < 0) ? 3 : 0 : 2;
SetOglLightColor (gameData.render.lights.ogl.nLights, pc->red, pc->green, pc->blue, f2fl (xBrightness));
if (nObject >= 0)
	pl->vPos = gameData.objs.objects [nObject].pos;
else if (nSegment >= 0) {
#if 0
	vms_vector	vOffs;
	side			*sideP = gameData.segs.segments [nSegment].sides + nSide;
#endif
	COMPUTE_SIDE_CENTER_I (&pl->vPos, nSegment, nSide);
#if 0
	VmVecScaleAdd (&vOffs, sideP->normals, sideP->normals + 1, 2);
	VmVecScaleFrac (&vOffs, 1, 100);
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
	gameStates.ogl.bHaveLights = 0;
	return -1;
	}
glLightf (pl->handle, GL_CONSTANT_ATTENUATION, pl->fAttenuation [0]);
glLightf (pl->handle, GL_LINEAR_ATTENUATION, pl->fAttenuation [1]);
glLightf (pl->handle, GL_QUADRATIC_ATTENUATION, pl->fAttenuation [2]);
#endif
LogErr ("adding light %d,%d\n", 
		  gameData.render.lights.ogl.nLights, pl - gameData.render.lights.ogl.lights);
if (nObject >= 0)
	gameData.render.lights.ogl.owners [nObject] = gameData.render.lights.ogl.nLights;
return gameData.render.lights.ogl.nLights++;
}

//------------------------------------------------------------------------------

void DeleteOglLight (short nLight)
{
if ((nLight >= 0) && (nLight < gameData.render.lights.ogl.nLights)) {
	tOglLight *pl = gameData.render.lights.ogl.lights + nLight;

	if (!pl->nType) {
		// do not remove static lights, or the nearest lights to segment info will get messed up!
		pl->bState = 0;
		return;
		}
	LogErr ("removing light %d,%d\n", nLight, pl - gameData.render.lights.ogl.lights);
	// if not removing last light in list, move last light down to the now free list entry
	// and keep the freed light handle thus avoiding gaps in used handles
	if (nLight < --gameData.render.lights.ogl.nLights) {
		*pl = gameData.render.lights.ogl.lights [gameData.render.lights.ogl.nLights];
		if (pl->nObject >= 0)
			gameData.render.lights.ogl.owners [pl->nObject] = nLight;
#if USE_OGL_LIGHTS
		RefreshOglLight (pl);
#endif
		pl = gameData.render.lights.ogl.lights + gameData.render.lights.ogl.nLights;
		}
#if USE_OGL_LIGHTS
	glDisable (pl->handle);
#endif
	}
}

//------------------------------------------------------------------------------

int RemoveOglLight (short nSegment, short nSide, short nObject)
{
	int	nLight = FindOglLight (nSegment, nSide, nObject);

if (nLight < 0)
	return 0;
DeleteOglLight (nLight);
if (nObject >= 0)
	gameData.render.lights.ogl.owners [nObject] = -1;
return 1;
}

//------------------------------------------------------------------------------

void RemoveOglLights (void)
{
	short	i;

for (i = 0; i < gameData.render.lights.ogl.nLights; i++)
	DeleteOglLight (i);
}

//------------------------------------------------------------------------------

void SetOglLightMaterial (short nSegment, short nSide, short nObject)
{
	static float fBlack [4] = {0.0f, 0.0f, 0.0f, 1.0f};

	int nLight = FindOglLight (nSegment, nSide, nObject);

if (nLight >= 0) {
	tOglLight *pl = gameData.render.lights.ogl.lights + nLight;
	if (pl->bState) {
		gameData.render.lights.ogl.material.emissive = *((fVector3 *) &pl->fEmissive);
		gameData.render.lights.ogl.material.specular = *((fVector3 *) &pl->fEmissive);
		gameData.render.lights.ogl.material.shininess = 96;
		gameData.render.lights.ogl.material.bValid = 1;
		gameData.render.lights.ogl.material.nLight = nLight;
		return;
		}
	}	
gameData.render.lights.ogl.material.bValid = 0;
}

//------------------------------------------------------------------------------

void AddOglLights (void)
{
	int			i, j, t;
	segment		*segP;
	side			*sideP;
	tFaceColor	*pc;

gameStates.ogl.bHaveLights = 1;
//glEnable (GL_LIGHTING);
gameData.render.lights.ogl.nLights = 0;
gameData.render.lights.ogl.material.bValid = 0;
InitTextureBrightness ();
for (i = 0, segP = gameData.segs.segments; i < gameData.segs.nSegments; i++, segP++) {
	for (j = 0, sideP = segP->sides; j < 6; j++, sideP++) {
		//if (i != 3 || j != 2) continue;
		if ((segP->children [j] >= 0) && !IS_WALL (sideP->wall_num))
			continue;
		t = sideP->tmap_num;
		if (t >= MAX_WALL_TEXTURES) 
			continue;
		pc = gameData.render.color.textures + t;
		AddOglLight (&pc->color, gameData.pig.tex.brightness [t], (short) i, (short) j, -1);
		t = sideP->tmap_num2 & 0x3fff;
		if ((t > 0) && (t < MAX_WALL_TEXTURES)) {
			pc = gameData.render.color.textures + t;
			AddOglLight (&pc->color, gameData.pig.tex.brightness [t], (short) i, (short) j, -1);
			}
		//if (gameData.render.lights.ogl.nLights)
		//	return;
		if (!gameStates.ogl.bHaveLights) {
			RemoveOglLights ();
			return;
			}
		}
	}
}

//------------------------------------------------------------------------------

void TransformOglLights (void)
{
	int			i;
	tOglLight	*pl = gameData.render.lights.ogl.lights;

#if USE_OGL_LIGHTS
OglSetupTransform ();
for (i = 0; i < gameData.render.lights.ogl.nLights; i++, pl++) {
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
	tShaderLight	*psl = gameData.render.lights.ogl.shader.lights;

gameData.render.lights.ogl.shader.nLights = 0;
UpdateOglHeadLight ();
for (i = 0; i < gameData.render.lights.ogl.nLights; i++, pl++) {
#if 0 //need to add all lights
	if (pl->bState) {
		if (pl->brightness == 0.0)
			continue;
		if (pl->color.red + pl->color.green + pl->color.blue == 0.0)
			continue;
#endif
		memcpy (&psl->color, &pl->color, sizeof (pl->color));
		VmsVecToFloat (&psl->pos, &pl->vPos);
		if (!gameStates.ogl.bUseTransform)
			G3TransformPointf (&psl->pos, &psl->pos);
		psl->brightness = pl->brightness;
		if (psl->bSpot = pl->bSpot) {
			VmsVecToFloat (&psl->dir, &pl->vDir);
			if (!gameStates.ogl.bUseTransform)
				G3RotatePointf (&psl->dir, &psl->dir);
			psl->spotAngle = pl->spotAngle;
			psl->spotExponent = pl->spotExponent;
			}
		psl->bState = pl->bState && (pl->color.red + pl->color.green + pl->color.blue > 0.0);
		psl->nType = pl->nType;
		gameData.render.lights.ogl.shader.nLights++;
		psl++;
#if 0
		}
#endif
	}
#	if 0
if (gameData.render.lights.ogl.shader.nTexHandle)
	glDeleteTextures (1, &gameData.render.lights.ogl.shader.nTexHandle);
gameData.render.lights.ogl.shader.nTexHandle = 0;
glGenTextures (1, &gameData.render.lights.ogl.shader.nTexHandle);
glBindTexture (GL_TEXTURE_2D, gameData.render.lights.ogl.shader.nTexHandle);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
glTexImage2D (GL_TEXTURE_2D, 0, 4, MAX_OGL_LIGHTS / 64, 64, 1, GL_RGBA,
				  GL_FLOAT, gameData.render.lights.ogl.shader.lights);
#	endif
#endif
}

//------------------------------------------------------------------------------

void SetNearestStaticLights (int nSegment, ubyte nType)
{
if (gameOpts->ogl.bUseLighting) {
	short	*pnl = gameData.render.lights.ogl.nNearestLights [nSegment];
	short	i;

	for (i = MAX_NEAREST_LIGHTS; i; i--, pnl++) {
		if (*pnl < 0)
			break;
		gameData.render.lights.ogl.shader.lights [*pnl].nType = nType;
		}
	}
}

//------------------------------------------------------------------------------

void SetNearestDynamicLights (int nSegment)
{
if (gameOpts->ogl.bUseLighting) {
	short				i = gameData.render.lights.ogl.shader.nLights;
	tOglLight		*pl = gameData.render.lights.ogl.lights + i;
	tShaderLight	*psl = gameData.render.lights.ogl.shader.lights + i;
	vms_vector		d, c;
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

tFaceColor *AvgSgmColor (int nSegment, vms_vector *pvPos)
{
	tFaceColor	c, *pvc, *psc = gameData.render.color.segments + nSegment;
	short			i, *pv;
	vms_vector	vCenter, vVertex;
	float			d, ds;

if (!gameOpts->ogl.bUseLighting) {
	psc->index = !gameStates.render.nFrameFlipFlop;
	return psc;
	}
if (psc->index == gameStates.render.nFrameFlipFlop)
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
	if (pvc->index == gameStates.render.nFrameFlipFlop) {
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
psc->index = gameStates.render.nFrameFlipFlop;
return psc;
}


//------------------------------------------------------------------------------

int AddOglHeadLight (object *objP)
{
	static float spotExps [] = {0.01f, 0.1f, 2.0f};
	static float spotAngles [] = {0.8f, 0.5f, 0.25f};

if (gameOpts->ogl.bUseLighting) {
		tRgbColorf c = {1.0f, 1.0f, 1.0f};
		tOglLight	*pl;
		int			nLight;

	nLight = AddOglLight (&c, F1_0 * 50, -1, -1, -1);
	if (nLight >= 0) {
		pl = gameData.render.lights.ogl.lights + nLight;
		pl->nPlayer = objP->id;
		pl->bSpot = 1;
		pl->spotAngle = spotAngles [extraGameInfo [IsMultiGame].nSpotSize];
		pl->spotExponent = spotExps [extraGameInfo [IsMultiGame].nSpotStrength];
		}
	return nLight;
	}
return -1;
}

//------------------------------------------------------------------------------

void RemoveOglHeadLight (object *objP)
{
if (gameOpts->ogl.bUseLighting) {
	DeleteOglLight (gameData.render.lights.ogl.nHeadLights [objP->id]);
	gameData.render.lights.ogl.nHeadLights [objP->id] = -1;
	}
}

//------------------------------------------------------------------------------

void UpdateOglHeadLight (void)
{
	tOglLight	*pl;
	object		*objP;
	short			nPlayer;

for (nPlayer = 0; nPlayer < MAX_PLAYERS; nPlayer++) {
	if (gameData.render.lights.ogl.nHeadLights [nPlayer] < 0)
		continue;
	pl = gameData.render.lights.ogl.lights + gameData.render.lights.ogl.nHeadLights [nPlayer];
	objP = gameData.objs.objects + gameData.multi.players [nPlayer].objnum;
	pl->vPos = objP->pos;
	pl->vDir = objP->orient.fvec;
	VmVecScaleInc (&pl->vPos, &pl->vDir, objP->size / 2);
	}
}

//------------------------------------------------------------------------------

void CalcOglLightAttenuation (vms_vector *pv)
{
#if !USE_OGL_LIGHTS
	int				i;
	tOglLight		*pl = gameData.render.lights.ogl.lights;
	tShaderLight	*psl = gameData.render.lights.ogl.shader.lights;
	fVector3			v, d;
	float				l;

v.p.x = f2fl (pv->x);
v.p.y = f2fl (pv->y);
v.p.z = f2fl (pv->z);
if (!gameStates.ogl.bUseTransform)
	G3TransformPointf (&v, &v);
for (i = gameData.render.lights.ogl.nLights; i; i--, pl++, psl++) {
	d.p.x = v.p.x - psl->pos.p.x;
	d.p.y = v.p.y - psl->pos.p.y;
	d.p.z = v.p.z - psl->pos.p.z;
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
if (!gameOpts->ogl.bUseLighting)
	gameStates.ogl.bHaveLights = 0;
else {
#if 1
	gameStates.ogl.bHaveLights = 1;
#else
	LogErr ("building lighting shader programs\n");
	DeleteShaderProg (NULL);
	gameStates.ogl.bHaveLights = CreateShaderFunc (NULL, &lvs, &lfs, lightingFS, lightingVS, 1);
	if (!gameStates.ogl.bHaveLights)
		gameOpts->ogl.bUseLighting = 0;
#endif
	}
}

// ----------------------------------------------------------------------------------------------
//eof
