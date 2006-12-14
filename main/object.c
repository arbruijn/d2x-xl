/* $Id: tObject.c, v 1.9 2003/10/04 03:14:47 btb Exp $ */
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

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
//#include "error.h"
#include "mono.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"
#include "textures.h"

#include "object.h"
#include "objsmoke.h"
#include "physics.h"
#include "slew.h"		
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "error.h"
#include "pa_enabl.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "fuelcen.h"
#include "endlevel.h"

#include "sounds.h"
#include "collide.h"

#include "lighting.h"
#include "newdemo.h"
#include "player.h"
#include "weapon.h"
#include "network.h"
#include "newmenu.h"
#include "gauges.h"
#include "multi.h"
#include "menu.h"
#include "args.h"
#include "text.h"
#include "piggy.h"
#include "switch.h"
#include "gameseq.h"
#include "vecmat.h"
#include "particles.h"
#include "hudmsg.h"
#include "oof.h"
#include "sphere.h"
#include "globvars.h"
#ifdef TACTILE
#include "tactile.h"
#endif
#include "ogl_init.h"
#include "kconfig.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#ifdef _3DFX
#include "3dfx_des.h"
#endif

extern vmsVector player_thrust;
extern int bSpeedBost;

void DetachAllObjects (tObject *parent);
void DetachOneObject (tObject *sub);
int FreeObjectSlots (int num_used);

/*
 *  Global variables
 */

ubyte CollisionResult [MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];
ubyte bIsMissile [100];
ubyte bIsWeapon [100];
short idToOOF [100];

//Data for gameData.objs.objects

// -- Object stuff

//info on the various types of gameData.objs.objects
#ifdef _DEBUG
tObject	Object_minus_one;
#endif

//------------------------------------------------------------------------------
// grsBitmap *robot_bms [MAX_ROBOT_BITMAPS];	//all bitmaps for all robots

// int robot_bm_nums [MAX_ROBOT_TYPES];		//starting bitmap num for each robot
// int robot_n_bitmaps [MAX_ROBOT_TYPES];		//how many bitmaps for each robot

// char *gameData.bots.names [MAX_ROBOT_TYPES];		//name of each robot

//--unused-- int NumRobotTypes = 0;

int bPrintObjectInfo = 0;
//@@int Object_viewer = 0;

//tObject * SlewObject = NULL;	// Object containing slew tObject info.

//--unused-- int Player_controllerType = 0;

tWindowRenderedData windowRenderedData [MAX_RENDERED_WINDOWS];

#ifdef _DEBUG
char	szObjectTypeNames [MAX_OBJECT_TYPES][9] = {
	"WALL    ", 
	"FIREBALL", 
	"ROBOT   ", 
	"HOSTAGE ", 
	"PLAYER  ", 
	"WEAPON  ", 
	"CAMERA  ", 
	"POWERUP ", 
	"DEBRIS  ", 
	"CNTRLCEN", 
	"FLARE   ", 
	"CLUTTER ", 
	"GHOST   ", 
	"LIGHT   ", 
	"COOP    ", 
	"MARKER  ", 
	"CAMBOT  ",
	"M-BALL  "
};
#endif

#ifndef RELEASE
//set viewer tObject to next tObject in array
void ObjectGotoNextViewer ()
{
	int i, nStartObj = 0;

	nStartObj = OBJ_IDX (gameData.objs.viewer);		//get viewer tObject number
	for (i = 0;i<=gameData.objs.nLastObject;i++) {
		nStartObj++;
		if (nStartObj > gameData.objs.nLastObject) nStartObj = 0;

		if (gameData.objs.objects [nStartObj].nType != OBJ_NONE)	{
			gameData.objs.viewer = gameData.objs.objects + nStartObj;
			return;
		}
	}
	Error ("Couldn't find a viewer tObject!");
}

//------------------------------------------------------------------------------
//set viewer tObject to next tObject in array
void ObjectGotoPrevViewer ()
{
	int i, nStartObj = 0;

	nStartObj = OBJ_IDX (gameData.objs.viewer);		//get viewer tObject number
	for (i = 0; i<=gameData.objs.nLastObject; i++) {
		nStartObj--;
		if (nStartObj < 0) nStartObj = gameData.objs.nLastObject;
		if (gameData.objs.objects [nStartObj].nType != OBJ_NONE)	{
			gameData.objs.viewer = gameData.objs.objects + nStartObj;
			return;
		}
	}
	Error ("Couldn't find a viewer tObject!");
}
#endif

//------------------------------------------------------------------------------

tObject *ObjFindFirstOfType (int nType)
 {
  int i;
  tObject	*objP = gameData.objs.objects;

  for (i=gameData.objs.nLastObject+1;i;i--, objP++)
	if (objP->nType==nType)
	 return (objP);
  return ((tObject *)NULL);
 }


int obj_return_num_ofType (int nType)
 {
  int i, count = 0;
	tObject *objP = gameData.objs.objects;

  for (i=gameData.objs.nLastObject+1;i;i--, objP++)
	if (objP->nType==nType)
	 count++;
  return (count);
 }


int obj_return_num_ofTypeid (int nType, int id)
 {
  int i, count = 0;
	tObject	*objP = gameData.objs.objects;

  for (i=gameData.objs.nLastObject+1;i;i--, objP++)
	if (objP->nType==nType && objP->id==id)
	 count++;
  return (count);
 }

int global_orientation = 0;

//------------------------------------------------------------------------------
//draw an tObject that has one bitmap & doesn't rotate

extern tRgbColorf bitmapColors [MAX_BITMAP_FILES];

void DrawObjectBlob (tObject *objP, tBitmapIndex bmi0, tBitmapIndex bmi, int iFrame, tRgbColorf *color, float alpha)
{
	grsBitmap	*bmP;
	int			id, orientation = 0;
	int			transp = (objP->nType == OBJ_FIREBALL) && (objP->renderType != RT_THRUSTER);
	int			bDepthInfo = 1; // (objP->nType != OBJ_FIREBALL);
	fix			xSize;

if (gameOpts->render.bTransparentEffects) {
	if (transp)
		alpha = 1.0;
	else if (!alpha) {
		if (objP->nType == OBJ_POWERUP) {
			id = objP->id;
			if ((id == POW_EXTRA_LIFE) ||
				 (id == POW_ENERGY) ||
				 (id == POW_SHIELD_BOOST) ||
				 (id == POW_HOARD_ORB) ||
				 (id == POW_CLOAK) ||
				 (id == POW_INVULNERABILITY))
				alpha = 2.0f / 3.0f;
			else
				alpha = 1.0f;
			orientation = OBJ_IDX (objP) & 7;
			}
		else if (objP->nType != OBJ_FIREBALL)
			alpha = 1.0f;
		else {
			orientation = (OBJ_IDX (objP)) & 7;
			alpha = 2.0f / 3.0f;
			}
		}
	}
else {
	transp = 0;
	alpha = 1.0f;
	}
orientation = global_orientation;
PIGGY_PAGE_IN (bmi, 0);
bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
if ((bmP->bmType == BM_TYPE_STD) && BM_OVERRIDE (bmP)) {
	bmP = BM_OVERRIDE (bmP);
	if (BM_FRAMES (bmP))
		bmP = BM_FRAMES (bmP) + iFrame;
	}

xSize = objP->size;
if (bmP->bm_props.w > bmP->bm_props.h)
	G3DrawBitMap (&objP->position.vPos,  xSize, FixMulDiv (xSize, bmP->bm_props.h, bmP->bm_props.w), bmP, 
					  orientation, alpha, transp, bDepthInfo);
else
	G3DrawBitMap (&objP->position.vPos, FixMulDiv (xSize, bmP->bm_props.w, bmP->bm_props.h), xSize, bmP, 
					  orientation, alpha, transp, bDepthInfo);
if (color) {
#if 1
	memcpy (color, bitmapColors + bmi.index, sizeof (*color));
#else
#	if 0
	ubyte *p = bmP->bm_texBuf;
	int c, h, i, j = 0, r = 0, g = 0, b = 0;
	for (h = i = bmP->bm_props.w * bmP->bm_props.h; i; i--, p++) {
		if (c = *p) {
			c *= 3;
			r += grPalette [c++];
			g += grPalette [c++];
			b += grPalette [c];
			j++;
			}
		}
	j *= 63;
	color->red = (double) r / (double) j;
	color->green = (double) g / (double) j;
	color->blue = (double) b / (double) j;
#	else
	unsigned char c = bmP->bm_avgColor;
	color->red = CPAL2Tr (bmP->bm_palette, c);
	color->green = CPAL2Tg (bmP->bm_palette, c);
	color->blue = CPAL2Tb (bmP->bm_palette, c);
#	endif
#endif
	}
}

//------------------------------------------------------------------------------
//draw an tObject that is a texture-mapped rod
void DrawObjectRodTexPoly (tObject *objP, tBitmapIndex bmi, int lighted)
{
	grsBitmap *bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
	fix light;
	vmsVector delta, top_v, bot_v;
	g3sPoint top_p, bot_p;

PIGGY_PAGE_IN (bmi, 0);
bmP = BmOverride (bmP);
//bmP->bm_handle = bmi.index;
VmVecCopyScale (&delta, &objP->position.mOrient.uVec, objP->size);
VmVecAdd (&top_v, &objP->position.vPos, &delta);
VmVecSub (&bot_v, &objP->position.vPos, &delta);
G3TransformAndEncodePoint (&top_p, &top_v);
G3TransformAndEncodePoint (&bot_p, &bot_v);
if (lighted)
	light = ComputeObjectLight (objP, &top_p.p3_vec);
else
	light = f1_0;
#ifdef _3DFX
_3dfx_rendering_poly_obj = 1;
#endif
#ifdef PA_3DFX_VOODOO
light = f1_0;
#endif
G3DrawRodTexPoly (bmP, &bot_p, objP->size, &top_p, objP->size, light);
#ifdef _3DFX
_3dfx_rendering_poly_obj = 0;
#endif
}

//------------------------------------------------------------------------------

int	bLinearTMapPolyObjs = 1;

extern fix Max_thrust;

//used for robot engine glow
//function that takes the same parms as draw_tmap, but renders as flat poly
//we need this to do the cloaked effect

//what darkening level to use when cloaked
#define CLOAKED_FADE_LEVEL		28

#define	CLOAK_FADEIN_DURATION_PLAYER	F2_0
#define	CLOAK_FADEOUT_DURATION_PLAYER	F2_0

#define	CLOAK_FADEIN_DURATION_ROBOT	F1_0
#define	CLOAK_FADEOUT_DURATION_ROBOT	F1_0

//------------------------------------------------------------------------------
//do special cloaked render
void DrawCloakedObject (tObject *objP, fix light, fix *glow, fix xCloakStartTime, fix xCloakEndTime)
{
	fix	xCloakDeltaTime, xTotalCloakedTime;
	fix	xLightScale = F1_0;
	int	nCloakValue = 0;
	int	bFading = 0;		//if true, bFading, else cloaking
	fix	xCloakFadeinDuration = F1_0;
	fix	xCloakFadeoutDuration = F1_0;


	xTotalCloakedTime = xCloakEndTime - xCloakStartTime;
	switch (objP->nType) {
		case OBJ_PLAYER:
			xCloakFadeinDuration = CLOAK_FADEIN_DURATION_PLAYER;
			xCloakFadeoutDuration = CLOAK_FADEOUT_DURATION_PLAYER;
			break;
		case OBJ_ROBOT:
			xCloakFadeinDuration = CLOAK_FADEIN_DURATION_ROBOT;
			xCloakFadeoutDuration = CLOAK_FADEOUT_DURATION_ROBOT;
			break;
		default:
			Int3 ();		//	Contact Mike: Unexpected tObject nType in DrawCloakedObject.
	}

	xCloakDeltaTime = gameData.time.xGame - xCloakStartTime;
	if (xCloakDeltaTime < xCloakFadeinDuration / 2) {

		xLightScale = FixDiv (xCloakFadeinDuration / 2 - xCloakDeltaTime, xCloakFadeinDuration / 2);
		bFading = 1;
	}
	else if (xCloakDeltaTime < xCloakFadeinDuration) {
		nCloakValue = f2i (FixDiv (xCloakDeltaTime - xCloakFadeinDuration / 2, xCloakFadeinDuration / 2) * CLOAKED_FADE_LEVEL);
	} else if (gameData.time.xGame < xCloakEndTime-xCloakFadeoutDuration) {
		static int nCloakDelta = 0, nCloakDir = 1;
		static fix xCloakTimer = 0;

		//note, if more than one cloaked tObject is visible at once, the
		//pulse rate will change!
		xCloakTimer -= gameData.time.xFrame;
		while (xCloakTimer < 0) {
			xCloakTimer += xCloakFadeoutDuration/12;
			nCloakDelta += nCloakDir;
			if (nCloakDelta == 0 || nCloakDelta == 4)
				nCloakDir = -nCloakDir;
		}
		nCloakValue = CLOAKED_FADE_LEVEL - nCloakDelta;
	} else if (gameData.time.xGame < xCloakEndTime - xCloakFadeoutDuration / 2) {
		nCloakValue = f2i (FixDiv (xTotalCloakedTime - xCloakFadeoutDuration / 2 - xCloakDeltaTime, xCloakFadeoutDuration / 2) * CLOAKED_FADE_LEVEL);
	} else {
		xLightScale = FixDiv (xCloakFadeoutDuration / 2 - (xTotalCloakedTime - xCloakDeltaTime), xCloakFadeoutDuration / 2);
		bFading = 1;
	}

	if (bFading) {
		fix xNewLight, xSaveGlow;
		tBitmapIndex * nAltTextures = NULL;

		if (objP->rType.polyObjInfo.nAltTextures > 0)
			nAltTextures = multi_player_textures [objP->rType.polyObjInfo.nAltTextures-1];
		xNewLight = FixMul (light, xLightScale);
		xSaveGlow = glow [0];
		glow [0] = FixMul (glow [0], xLightScale);
		DrawPolygonModel (objP, &objP->position.vPos, 
				   &objP->position.mOrient, 
				   (vmsAngVec *)&objP->rType.polyObjInfo.animAngles, 
				   objP->rType.polyObjInfo.nModel, objP->rType.polyObjInfo.nSubObjFlags, 
				   xNewLight, 
				   glow, 
				   nAltTextures, 
					NULL);
		glow [0] = xSaveGlow;
	}
	else {
		gameStates.render.grAlpha = (float) nCloakValue;
		GrSetColorRGB (0, 0, 0, 255);	//set to black (matters for s3)
		G3SetSpecialRender (DrawTexPolyFlat, NULL, NULL);		//use special flat drawer
		DrawPolygonModel (objP, &objP->position.vPos, 
				   &objP->position.mOrient, 
				   (vmsAngVec *)&objP->rType.polyObjInfo.animAngles, 
				   objP->rType.polyObjInfo.nModel, objP->rType.polyObjInfo.nSubObjFlags, 
				   light, 
				   glow, 
				   NULL, 
					NULL);
		G3SetSpecialRender (NULL, NULL, NULL);
		gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
	}
}

//------------------------------------------------------------------------------
//draw an tObject which renders as a polygon model
void DrawPolygonObject (tObject *objP)
{
	fix xLight;
	int imSave;
	fix xEngineGlow [2];		//element 0 is for engine glow, 1 for headlight
	int bBlendPolys = 0;
	int bBrightPolys = 0;
	//tRgbColorf color;

#if SHADOWS
if (gameOpts->render.shadows.bFast && (gameStates.render.nShadowPass == 3))
	return;
#endif
//	If option set for bright players in netgame, brighten them!
if ((gameData.app.nGameMode & GM_MULTI) && netGame.BrightPlayers && (objP->nType == OBJ_PLAYER))
	xLight = F1_0;
else
xLight = ComputeObjectLight (objP, NULL);
//make robots brighter according to robot glow field
if (objP->nType == OBJ_ROBOT) {
#ifdef _DEBUG
	xLight = ComputeObjectLight (objP, NULL);
#endif
	xLight += (gameData.bots.pInfo [objP->id].glow << 12);		//convert 4:4 to 16:16
	}
else if (objP->nType == OBJ_WEAPON) {
	if (objP->id == FLARE_ID)
		xLight += F1_0 * 2;
	}
else if (objP->nType == OBJ_MARKER)
 	xLight += F1_0 * 2;
imSave = gameStates.render.nInterpolationMethod;
if (bLinearTMapPolyObjs)
	gameStates.render.nInterpolationMethod = 1;
//set engine glow value
ComputeEngineGlow (objP, xEngineGlow);
if (objP->rType.polyObjInfo.nTexOverride != -1) {
#ifdef _DEBUG
	tPolyModel *pm = gameData.models.polyModels + objP->rType.polyObjInfo.nModel;
#endif
	tBitmapIndex bmiP [12];
	int i;

	Assert (pm->nTextures <= 12);
	for (i = 0; i < 12; i++)		//fill whole array, in case simple model needs more
		bmiP [i] = gameData.pig.tex.bmIndex [0][objP->rType.polyObjInfo.nTexOverride];

	DrawPolygonModel (objP, &objP->position.vPos, 
				&objP->position.mOrient, 
				(vmsAngVec *) &objP->rType.polyObjInfo.animAngles, 
				objP->rType.polyObjInfo.nModel, 
				objP->rType.polyObjInfo.nSubObjFlags, 
				xLight, 
				xEngineGlow, 
				bmiP, 
				NULL);
}
else {
	if ((objP->nType == OBJ_PLAYER) && (gameData.multi.players [objP->id].flags & PLAYER_FLAGS_CLOAKED))
		DrawCloakedObject (objP, xLight, xEngineGlow, gameData.multi.players [objP->id].cloakTime, gameData.multi.players [objP->id].cloakTime+CLOAK_TIME_MAX);
	else if ((objP->nType == OBJ_ROBOT) && (objP->cType.aiInfo.CLOAKED)) {
		if (gameData.bots.pInfo [objP->id].bossFlag)
			DrawCloakedObject (objP, xLight, xEngineGlow, gameData.boss.nCloakStartTime, gameData.boss.nCloakEndTime);
		else
			DrawCloakedObject (objP, xLight, xEngineGlow, gameData.time.xGame-F1_0*10, gameData.time.xGame+F1_0*10);
		}
	else {
		tBitmapIndex *bmiAltTex = NULL;
		if (objP->rType.polyObjInfo.nAltTextures > 0)
			bmiAltTex = multi_player_textures [objP->rType.polyObjInfo.nAltTextures-1];

		//	Snipers get bright when they fire.
		if (gameData.ai.localInfo [OBJ_IDX (objP)].nextPrimaryFire < F1_0/8) {
			if (objP->cType.aiInfo.behavior == AIB_SNIPE)
				xLight = 2*xLight + F1_0;
		}
		bBlendPolys = (objP->nType == OBJ_WEAPON) && (gameData.weapons.info [objP->id].model_num_inner > -1);
		bBrightPolys = bBlendPolys && WI_energy_usage (objP->id);
		if (bBlendPolys) {
			fix xDistToEye = VmVecDistQuick (&gameData.objs.viewer->position.vPos, &objP->position.vPos);
			if (!gameOpts->legacy.bRender) {
				gameStates.render.grAlpha = 2.0f;
				OglBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				}
			if (xDistToEye < gameData.models.nSimpleModelThresholdScale * F1_0*2)
				DrawPolygonModel (
					objP, &objP->position.vPos, &objP->position.mOrient, 
					(vmsAngVec *) &objP->rType.polyObjInfo.animAngles, 
					gameData.weapons.info [objP->id].model_num_inner, 
					objP->rType.polyObjInfo.nSubObjFlags, 
					bBrightPolys ? F1_0 : xLight, 
					xEngineGlow, 
					bmiAltTex, 
					NULL /*gameData.weapons.color + objP->id*/);
			}
		DrawPolygonModel (
			objP, &objP->position.vPos, &objP->position.mOrient, 
			(vmsAngVec *)&objP->rType.polyObjInfo.animAngles, 
			objP->rType.polyObjInfo.nModel, 
			objP->rType.polyObjInfo.nSubObjFlags, 
			bBrightPolys ? F1_0 : xLight, 
			xEngineGlow, 
			bmiAltTex, 
			(objP->nType == OBJ_WEAPON) ? gameData.weapons.color + objP->id : NULL);
		if (bBlendPolys && !gameOpts->legacy.bRender) {
			gameStates.render.grAlpha = (float) GR_ACTUAL_FADE_LEVELS;
			OglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
		}
	}
gameStates.render.nInterpolationMethod = imSave;
}

//------------------------------------------------------------------------------
// These variables are used to keep a list of the 3 closest robots to the viewer.
// The code works like this: Every time render tObject is called with a polygon model, 
// it finds the distance of that robot to the viewer.  If this distance if within 10
// segments of the viewer, it does the following: If there aren't already 3 robots in
// the closet-robots list, it just sticks that tObject into the list along with its distance.
// If the list already contains 3 robots, then it finds the robot in that list that is
// farthest from the viewer. If that tObject is farther than the tObject currently being
// rendered, then the new tObject takes over that far tObject's slot.  *Then* after all
// gameData.objs.objects are rendered, object_render_targets is called an it draws a target on top
// of all the gameData.objs.objects.

//091494: #define MAX_CLOSE_ROBOTS 3
//--unused-- static int Object_draw_lock_boxes = 0;
//091494: static int Object_num_close = 0;
//091494: static tObject * Object_close_ones [MAX_CLOSE_ROBOTS];
//091494: static fix Object_closeDistance [MAX_CLOSE_ROBOTS];

//091494: set_closeObjects (tObject *objP)
//091494: {
//091494: 	fix dist;
//091494:
//091494: 	if ((objP->nType != OBJ_ROBOT) || (Object_draw_lock_boxes == 0))	
//091494: 		return;
//091494:
//091494: 	// The following code keeps a list of the 10 closest robots to the
//091494: 	// viewer.  See comments in front of this function for how this works.
//091494: 	dist = VmVecDist (&objP->position.vPos, &gameData.objs.viewer->position.vPos);
//091494: 	if (dist < i2f (20*10))	{				
//091494: 		if (Object_num_close < MAX_CLOSE_ROBOTS)	{
//091494: 			Object_close_ones [Object_num_close] = obj;
//091494: 			Object_closeDistance [Object_num_close] = dist;
//091494: 			Object_num_close++;
//091494: 		} else {
//091494: 			int i, farthestRobot;
//091494: 			fix farthestDistance;
//091494: 			// Find the farthest robot in the list
//091494: 			farthestRobot = 0;
//091494: 			farthestDistance = Object_closeDistance [0];
//091494: 			for (i=1; i<Object_num_close; i++)	{
//091494: 				if (Object_closeDistance [i] > farthestDistance)	{
//091494: 					farthestDistance = Object_closeDistance [i];
//091494: 					farthestRobot = i;
//091494: 				}
//091494: 			}
//091494: 			// If this tObject is closer to the viewer than
//091494: 			// the farthest in the list, replace the farthest with this tObject.
//091494: 			if (farthestDistance > dist)	{
//091494: 				Object_close_ones [farthestRobot] = obj;
//091494: 				Object_closeDistance [farthestRobot] = dist;
//091494: 			}
//091494: 		}
//091494: 	}
//091494: }


// -----------------------------------------------------------------------------
//this routine checks to see if an robot rendered near the middle of
//the screen, and if so and the tPlayer had fired, "warns" the robot
void SetRobotLocationInfo (tObject *objP)
{
if (gameStates.app.bPlayerFiredLaserThisFrame != -1) {
	g3sPoint temp;

	G3TransformAndEncodePoint (&temp, &objP->position.vPos);
	if (temp.p3_codes & CC_BEHIND)		//robot behind the screen
		return;
	//the code below to check for tObject near the center of the screen
	//completely ignores z, which may not be good
	if ((abs (temp.p3_x) < F1_0*4) && (abs (temp.p3_y) < F1_0*4)) {
		objP->cType.aiInfo.nDangerLaser = gameStates.app.bPlayerFiredLaserThisFrame;
		objP->cType.aiInfo.nDangerLaserSig = gameData.objs.objects [gameStates.app.bPlayerFiredLaserThisFrame].nSignature;
		}
	}
}

//	------------------------------------------------------------------------------------------------------------------

void CreateSmallFireballOnObject (tObject *objP, fix size_scale, int bSound)
{
	fix			size;
	vmsVector	pos, rand_vec;
	short			nSegment;

pos = objP->position.vPos;
MakeRandomVector (&rand_vec);
VmVecScale (&rand_vec, objP->size / 2);
VmVecInc (&pos, &rand_vec);
size = FixMul (size_scale, F1_0 / 2 + d_rand ()*4 / 2);
nSegment = FindSegByPoint (&pos, objP->nSegment);
if (nSegment != -1) {
	tObject *explObjP = ObjectCreateExplosion (nSegment, &pos, size, VCLIP_SMALL_EXPLOSION);
	if (!explObjP)
		return;
	AttachObject (objP, explObjP);
	if (d_rand () < 8192) {
		fix	vol = F1_0 / 2;
		if (objP->nType == OBJ_ROBOT)
			vol *= 2;
		else if (bSound)
			DigiLinkSoundToObject (SOUND_EXPLODING_WALL, OBJ_IDX (objP), 0, vol);
		}
	}
}

//	------------------------------------------------------------------------------------------------------------------
void CreateVClipOnObject (tObject *objP, fix size_scale, ubyte vclip_num)
{
	fix			size;
	vmsVector	pos, rand_vec;
	short			nSegment;

pos = objP->position.vPos;
MakeRandomVector (&rand_vec);
VmVecScale (&rand_vec, objP->size / 2);
VmVecInc (&pos, &rand_vec);
size = FixMul (size_scale, F1_0 + d_rand ()*4);
nSegment = FindSegByPoint (&pos, objP->nSegment);
if (nSegment != -1) {
	tObject *explObjP = ObjectCreateExplosion (nSegment, &pos, size, vclip_num);
	if (!explObjP)
		return;

	explObjP->movementType = MT_PHYSICS;
	explObjP->mType.physInfo.velocity.x = objP->mType.physInfo.velocity.x / 2;
	explObjP->mType.physInfo.velocity.y = objP->mType.physInfo.velocity.y / 2;
	explObjP->mType.physInfo.velocity.z = objP->mType.physInfo.velocity.z / 2;
	}
}

// -- mk, 02/05/95 -- #define	VCLIP_INVULNERABILITY_EFFECT	VCLIP_SMALL_EXPLOSION
// -- mk, 02/05/95 --
// -- mk, 02/05/95 -- // -----------------------------------------------------------------------------
// -- mk, 02/05/95 -- void do_player_invulnerability_effect (tObject *objP)
// -- mk, 02/05/95 -- {
// -- mk, 02/05/95 -- 	if (d_rand () < gameData.time.xFrame*8) {
// -- mk, 02/05/95 -- 		CreateVClipOnObject (objP, F1_0, VCLIP_INVULNERABILITY_EFFECT);
// -- mk, 02/05/95 -- 	}
// -- mk, 02/05/95 -- }

// -----------------------------------------------------------------------------
//	Render an tObject.  Calls one of several routines based on nType

// -----------------------------------------------------------------------------

void RenderPlayerShield (tObject *objP)
{
	int i = objP->id;

#if SHADOWS
if (EGI_FLAG (bShadows, 0, 0) &&
	 (gameOpts->render.shadows.bFast ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (EGI_FLAG (bRenderShield, 0, 0) &&
	 !(gameData.multi.players [i].flags & PLAYER_FLAGS_CLOAKED)) {
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glDisable (GL_STENCIL_TEST);
	UseSpherePulse (&gameData.render.shield, gameData.multi.spherePulse + i);
	if (gameData.multi.players [i].flags & PLAYER_FLAGS_INVULNERABLE)
		DrawShieldSphere (objP, 1.0f, 0.8f, 0.6f, 0.6f);
	else if (gameData.multi.bWasHit [i]) {
		if (gameData.multi.bWasHit [i] < 0) {
			gameData.multi.bWasHit [i] = 1;
			gameData.multi.nLastHitTime [i] = gameStates.app.nSDLTicks;
			SetSpherePulse (gameData.multi.spherePulse + i, 0.1f, 0.5f);
			}
		else if (gameStates.app.nSDLTicks - gameData.multi.nLastHitTime [i] >= 300) {
			gameData.multi.bWasHit [i] = 0;
			SetSpherePulse (gameData.multi.spherePulse + i, 0.02f, 0.4f);
			}
		}
	if (gameData.multi.bWasHit [i])
		DrawShieldSphere (objP, 1.0f, 0.5f, 0.0f, 0.5f);
	else {
		if (gameData.multi.spherePulse [i].fSpeed == 0.0f)
			SetSpherePulse (gameData.multi.spherePulse + i, 0.02f, 0.5f);
		DrawShieldSphere (objP, 0.0f, 0.5f, 1.0f, (float) f2ir (gameData.multi.players [i].shields) / 400.0f);
		}
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glEnable (GL_STENCIL_TEST);
	}
}

// -----------------------------------------------------------------------------

static inline tRgbColorf *ObjectFrameColor (tObject *objP, tRgbColorf *pc)
{
	static tRgbColorf	defaultColor = {0, 1.0f, 0};
	static tRgbColorf	botDefColor = {1.0f, 0, 0};
	static tRgbColorf	reactorDefColor = {0.5f, 0, 0.5f};
	static tRgbColorf	playerDefColors [] = {{0,1.0f,0},{0,0,1.0f},{1.0f,0,0}};

if (pc)
	return pc;
if (objP)
	if (objP->nType == OBJ_CNTRLCEN)
		return &reactorDefColor;
	else if (objP->nType == OBJ_ROBOT) {
		if (!gameData.bots.pInfo [objP->id].companion)
			return &botDefColor;
		}
	else if (objP->nType == OBJ_PLAYER) {
		if (IsTeamGame)
			return playerDefColors + GetTeam (objP->id) + 1;
		return playerDefColors;
		}
return &defaultColor;
}

// -----------------------------------------------------------------------------

static inline float ObjectDamage (tObject *objP)
{
	float	fDmg;
	fix	xMaxShields;

if (objP->nType == OBJ_PLAYER)
	fDmg = f2fl (gameData.multi.players [objP->id].shields) / 100;
else if (objP->nType == OBJ_ROBOT) {
	xMaxShields = gameData.bots.info [gameStates.app.bD1Mission][objP->id].strength;
	if (gameData.bots.info [gameStates.app.bD1Mission][objP->id].bossFlag)
		xMaxShields /= (NDL - gameStates.app.nDifficultyLevel);
	fDmg = f2fl (objP->shields) / f2fl (xMaxShields);
	if (gameData.bots.pInfo [objP->id].companion)
		fDmg /= 2;
	}
else if (objP->nType == OBJ_CNTRLCEN)
	fDmg = f2fl (objP->shields) / f2fl (ReactorStrength ());
else
	fDmg = 1.0f;
return (fDmg > 1.0f) ? 1.0f : (fDmg < 0.0f) ? 0.0f : fDmg;
}

// -----------------------------------------------------------------------------

void RenderDamageIndicator (tObject *objP, tRgbColorf *pc)
{
	fVector		fPos, fVerts [4];
	float			r, r2, w;

#if SHADOWS
if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass != 1))
//	 (gameOpts->render.shadows.bFast ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (EGI_FLAG (bDamageIndicators, 0, 0) &&
	 (extraGameInfo [IsMultiGame].bTargetIndicators < 2)) {
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glDisable (GL_STENCIL_TEST);
	pc = ObjectFrameColor (objP, pc);
	VmsVecToFloat (&fPos, &objP->position.vPos);
	G3TransformPointf (&fPos, &fPos, 0);
	r = f2fl (objP->size);
	r2 = r / 10;
	r = r2 * 9;
	w = 2 * r;
	fPos.p.x -= r;
	fPos.p.y += r;
	w *= ObjectDamage (objP);
	fVerts [0].p.x = fVerts [3].p.x = fPos.p.x;
	fVerts [1].p.x = fVerts [2].p.x = fPos.p.x + w;
	fVerts [0].p.y = fVerts [1].p.y = fPos.p.y;
	fVerts [2].p.y = fVerts [3].p.y = fPos.p.y - r2;
	fVerts [0].p.z = fVerts [1].p.z = fVerts [2].p.z = fVerts [3].p.z = fPos.p.z;
	fVerts [0].p.w = fVerts [1].p.w = fVerts [2].p.w = fVerts [3].p.w = 1;
	glColor4f (pc->red, pc->green, pc->blue, 2.0f / 3.0f);
	glDisable (GL_TEXTURE_2D);
#if 1
#	if 1
	glEnableClientState (GL_VERTEX_ARRAY);
	glVertexPointer (4, GL_FLOAT, 0, fVerts);
	glDrawArrays (GL_QUADS, 0, 4);
	//glDisableClientState (GL_VERTEX_ARRAY);
#	else
	glBegin (GL_QUADS);
	glVertex3fv ((GLfloat *) fVerts);
	glVertex3fv ((GLfloat *) (fVerts + 1));
	glVertex3fv ((GLfloat *) (fVerts + 2));
	glVertex3fv ((GLfloat *) (fVerts + 3));
	glEnd ();
#	endif
#else
	glBegin (GL_QUADS);
	glVertex3f (fPos.p.x, fPos.p.y, fPos.p.z);
	glVertex3f (fPos.p.x + w, fPos.p.y, fPos.p.z);
	glVertex3f (fPos.p.x + w, fPos.p.y - r2, fPos.p.z);
	glVertex3f (fPos.p.x, fPos.p.y - r2, fPos.p.z);
	glEnd ();
#endif
	w = 2 * r;
	fVerts [1].p.x = fVerts [2].p.x = fPos.p.x + w;
	glColor3fv ((GLfloat *) pc);
#if 1
	//glEnableClientState (GL_VERTEX_ARRAY);
	glVertexPointer (4, GL_FLOAT, 0, fVerts);
	glDrawArrays (GL_LINE_LOOP, 0, 4);
	glDisableClientState (GL_VERTEX_ARRAY);
#else
	glBegin (GL_LINE_LOOP);
	glVertex3fv ((GLfloat *) fVerts);
	glVertex3fv ((GLfloat *) (fVerts + 1));
	glVertex3fv ((GLfloat *) (fVerts + 2));
	glVertex3fv ((GLfloat *) (fVerts + 3));
	glEnd ();
#endif
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glEnable (GL_STENCIL_TEST);
	}
}

// -----------------------------------------------------------------------------

void RenderTargetIndicator (tObject *objP, tRgbColorf *pc)
{
	fVector		fPos, fVerts [4];
	float			r, r2, r3;
	int			nPlayer = (objP->nType == OBJ_PLAYER) ? objP->id : -1;

#if SHADOWS
if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass != 1))
//	 (gameOpts->render.shadows.bFast ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
#if 0
if (!CanSeeObject (OBJ_IDX (objP), 1))
	return;
#endif
if (gameStates.app.bNostalgia || !EGI_FLAG (bCloakedIndicators, 0, 0)) {
	if (nPlayer >= 0) {
		if (gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_CLOAKED)
			return;
		}
	else if (objP->nType == OBJ_ROBOT) {
		if (objP->cType.aiInfo.CLOAKED)
			return;
		}
	}
if (IsTeamGame && EGI_FLAG (bFriendlyIndicators, 0, 0)) {
	if (GetTeam (nPlayer) != GetTeam (gameData.multi.nLocalPlayer)) {
		if (!(gameData.multi.players [nPlayer].flags & PLAYER_FLAGS_FLAG))
			return;
		pc = ObjectFrameColor (NULL, NULL);
		}
	}
if (EGI_FLAG (bHitIndicators, 0, 0) && (ObjectDamage (objP) >= 1.0f))
	return;
if (EGI_FLAG (bTargetIndicators, 0, 0)) {
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glDisable (GL_STENCIL_TEST);
	pc = ObjectFrameColor (objP, pc);
	VmsVecToFloat (&fPos, &objP->position.vPos);
	G3TransformPointf (&fPos, &fPos, 0);
	r = f2fl (objP->size);
	glDisable (GL_TEXTURE_2D);
	glColor3fv ((GLfloat *) pc);
	fVerts [0].p.w = fVerts [1].p.w = fVerts [2].p.w = fVerts [3].p.w = 1;
	if (extraGameInfo [IsMultiGame].bTargetIndicators == 1) {	//square brackets
		r2 = r * 2 / 3;
		fVerts [0].p.x = fVerts [3].p.x = fPos.p.x - r2;
		fVerts [1].p.x = fVerts [2].p.x = fPos.p.x - r;
		fVerts [0].p.y = fVerts [1].p.y = fPos.p.y - r;
		fVerts [2].p.y = fVerts [3].p.y = fPos.p.y + r;
		fVerts [0].p.z =
		fVerts [1].p.z =
		fVerts [2].p.z =
		fVerts [3].p.z = fPos.p.z;

#if 1
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer (4, GL_FLOAT, 0, fVerts);
		glDrawArrays (GL_LINE_STRIP, 0, 4);
		//glDisableClientState (GL_VERTEX_ARRAY);
#else
		glBegin (GL_LINE_STRIP);
		glVertex3fv ((GLfloat *) fVerts);
		glVertex3fv ((GLfloat *) (fVerts + 1));
		glVertex3fv ((GLfloat *) (fVerts + 2));
		glVertex3fv ((GLfloat *) (fVerts + 3));
		glEnd ();
#endif
		fVerts [0].p.x = fVerts [3].p.x = fPos.p.x + r2;
		fVerts [1].p.x = fVerts [2].p.x = fPos.p.x + r;
#if 1
		//glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer (4, GL_FLOAT, 0, fVerts);
		glDrawArrays (GL_LINE_STRIP, 0, 4);
		glDisableClientState (GL_VERTEX_ARRAY);
#else
		glBegin (GL_LINE_STRIP);
		glVertex3fv ((GLfloat *) fVerts);
		glVertex3fv ((GLfloat *) (fVerts + 1));
		glVertex3fv ((GLfloat *) (fVerts + 2));
		glVertex3fv ((GLfloat *) (fVerts + 3));
		glEnd ();
#endif
		}
	else {	//triangle
		r2 = r / 3;
		fVerts [0].p.x = fPos.p.x - r2;
		fVerts [1].p.x = fPos.p.x + r2;
		fVerts [2].p.x = fPos.p.x;
		fVerts [0].p.y = fVerts [1].p.y = fPos.p.y + r;
		fVerts [2].p.y = fPos.p.y + r - r2;
		fVerts [0].p.z =
		fVerts [1].p.z =
		fVerts [2].p.z = fPos.p.z;
#if 1
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer (4, GL_FLOAT, 0, fVerts);
		glDrawArrays (GL_LINE_LOOP, 0, 3);
		//glDisableClientState (GL_VERTEX_ARRAY);
#else
		glBegin (GL_LINE_LOOP);
		glVertex3fv ((GLfloat *) fVerts);
		glVertex3fv ((GLfloat *) (fVerts + 1));
		glVertex3fv ((GLfloat *) (fVerts + 2));
		glEnd ();
#endif
		if (EGI_FLAG (bDamageIndicators, 0, 0)) {
			r3 = ObjectDamage (objP);
			if (r3 < 1.0f) {
				if (r3 < 0.0f)
					r3 = 0.0f;
				fVerts [0].p.x = fPos.p.x - r2 * r3;
				fVerts [1].p.x = fPos.p.x + r2 * r3;
				fVerts [2].p.x = fPos.p.x;
				fVerts [0].p.y = fVerts [1].p.y = fPos.p.y + r - r2 * (1.0f - r3);
				//fVerts [2].p.y = fPos.p.y + r - r2;
				}
			}
		glColor4f (pc->red, pc->green, pc->blue, 2.0f / 3.0f);
#if 1
		//glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer (4, GL_FLOAT, 0, fVerts);
		glDrawArrays (GL_TRIANGLES, 0, 3);
		glDisableClientState (GL_VERTEX_ARRAY);
#else
		glBegin (GL_TRIANGLES);
		glVertex3fv ((GLfloat *) fVerts);
		glVertex3fv ((GLfloat *) (fVerts + 1));
		glVertex3fv ((GLfloat *) (fVerts + 2));
		glEnd ();
#endif
		}
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glEnable (GL_STENCIL_TEST);
	}
RenderDamageIndicator (objP, pc);
}

// -----------------------------------------------------------------------------

void RenderTowedFlag (tObject *objP)
{
	static fVector fVerts [4] = {
		{0.0f, 2.0f / 3.0f, 0.0f, 1.0f},
		{0.0f, 2.0f / 3.0f, -1.0f, 1.0f},
		{0.0f, -(1.0f / 3.0f), -1.0f, 1.0f},
		{0.0f, -(1.0f / 3.0f), 0.0f, 1.0f}
	};

	typedef struct uv {
		float	u, v;
	} uv;

	static uv uvList [4] = {{0.0f, -0.3f}, {1.0f, -0.3f}, {1.0f, 0.7f}, {0.0f, 0.7f}};

#if SHADOWS
if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass != 1))
//	 (gameOpts->render.shadows.bFast ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (!gameStates.app.bNostalgia && IsTeamGame && (gameData.multi.players [objP->id].flags & PLAYER_FLAGS_FLAG)) {
		vmsVector		vPos = objP->position.vPos;
		fVector			vPosf;
		tFlagData		*pf = gameData.pig.flags + !GetTeam (objP->id);
		tPathPoint		*pp = GetPathPoint (&pf->path);
		int				i;
		float				r;
		grsBitmap		*bmP;

	if (pp) {
		if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
			glDisable (GL_STENCIL_TEST);
		OglActiveTexture (GL_TEXTURE0_ARB);
		glEnable (GL_TEXTURE_2D);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		PIGGY_PAGE_IN (pf->bmi, 0);
		bmP = gameData.pig.tex.pBitmaps + pf->vcP->frames [pf->vci.nCurFrame].index;
		if (OglBindBmTex (bmP, 0))
			return;
		bmP = BmCurFrame (bmP);
		OglTexWrap (bmP->glTexture, GL_REPEAT);
		VmVecScaleInc (&vPos, &objP->position.mOrient.fVec, -objP->size);
		r = f2fl (objP->size);
		G3StartInstanceMatrix (&vPos, &pp->mOrient);
		glBegin (GL_QUADS);
		glColor3f (1.0f, 1.0f, 1.0f);
		for (i = 0; i < 4; i++) {
			vPosf.p.x = 0;
			vPosf.p.y = fVerts [i].p.y * r;
			vPosf.p.z = fVerts [i].p.z * r;
			G3TransformPointf (&vPosf, &vPosf, 0);
			glTexCoord2fv ((GLfloat *) (uvList + i));
			glVertex3fv ((GLfloat *) &vPosf);
			}
		for (i = 3; i >= 0; i--) {
			vPosf.p.x = 0;
			vPosf.p.y = fVerts [i].p.y * r;
			vPosf.p.z = fVerts [i].p.z * r;
			G3TransformPointf (&vPosf, &vPosf, 0);
			glTexCoord2fv ((GLfloat *) (uvList + i));
			glVertex3fv ((GLfloat *) &vPosf);
			}
		glEnd ();
		G3DoneInstance ();
		OGL_BINDTEX (0);
		if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
			glEnable (GL_STENCIL_TEST);
		}
	}
}

// -----------------------------------------------------------------------------

#define	RING_SIZE		16
#define	THRUSTER_SEGS	14

static fVector	vFlame [THRUSTER_SEGS][RING_SIZE];
static int			bHaveFlame = 0;

static fVector	vRing [RING_SIZE] = {
	{-0.5f, -0.5f, 0.0f, 1.0f},
	{-0.6533f, -0.2706f, 0.0f, 1.0f},
	{-0.7071f, 0.0f, 0.0f, 1.0f},
	{-0.6533f, 0.2706f, 0.0f, 1.0f},
	{-0.5f, 0.5f, 0.0f, 1.0f},
	{-0.2706f, 0.6533f, 0.0f, 1.0f},
	{0.0f, 0.7071f, 0.0f, 1.0f},
	{0.2706f, 0.6533f, 0.0f, 1.0f},
	{0.5f, 0.5f, 0.0f, 1.0f},
	{0.6533f, 0.2706f, 0.0f, 1.0f},
	{0.7071f, 0.0f, 0.0f, 1.0f},
	{0.6533f, -0.2706f, 0.0f, 1.0f},
	{0.5f, -0.5f, 0.0f, 1.0f},
	{0.2706f, -0.6533f, 0.0f, 1.0f},
	{0.0f, -0.7071f, 0.0f, 1.0f},
	{-0.2706f, -0.6533f, 0.0f, 1.0f}
};

static int		nStripIdx [] = {0,15,1,14,2,13,3,12,4,11,5,10,6,9,7,8};

void CreateThrusterFlame (void)
{
if (!bHaveFlame) {
		fVector		*pv;
		int			i, j, m, n;
		double		phi, sinPhi;
		float			z = 0, 
						fScale = 2.0f / 3.0f, 
						fStep [2] = {1.0f / 4.0f, 1.0f / 3.0f};

	pv = &vFlame [0][0];
	for (i = 0, phi = 0; i < 5; i++, phi += Pi / 8, z -= fStep [0]) {
		sinPhi = (1 + sin (phi) / 2) * fScale;
		for (j = 0; j < RING_SIZE; j++, pv++) {
			pv->p.x = vRing [j].p.x * (float) sinPhi;
			pv->p.y = vRing [j].p.y * (float) sinPhi;
			pv->p.z = z;
			}
		}
	m = n = THRUSTER_SEGS - i + 1;
	for (phi = Pi / 2; i < THRUSTER_SEGS; i++, phi += Pi / 8, z -= fStep [1], m--) {
		sinPhi = (1 + sin (phi) / 2) * fScale * m / n;
		for (j = 0; j < RING_SIZE; j++, pv++) {
			pv->p.x = vRing [j].p.x * (float) sinPhi;
			pv->p.y = vRing [j].p.y * (float) sinPhi;
			pv->p.z = z;
			}
		}
	bHaveFlame = 1;
	}
}

// -----------------------------------------------------------------------------

void CalcShipThrusterPos (tObject *objP, vmsVector *vPos)
{
if (gameOpts->render.bHiresModels) {
	VmVecScaleAdd (vPos, &objP->position.vPos, &objP->position.mOrient.fVec, -objP->size);
	VmVecScaleInc (vPos, &objP->position.mOrient.rVec, -(8 * objP->size / 44));
	VmVecScaleAdd (vPos + 1, vPos, &objP->position.mOrient.rVec, 8 * objP->size / 22);
	}
else {
	VmVecScaleAdd (vPos, &objP->position.vPos, &objP->position.mOrient.fVec, -objP->size / 10 * 9);
	VmVecScaleInc (vPos, &objP->position.mOrient.rVec, -(8 * objP->size / 50));
	if (!gameStates.app.bFixModels)
		VmVecScaleInc (vPos, &objP->position.mOrient.uVec, -(objP->size / 20));
	VmVecScaleAdd (vPos + 1, vPos, &objP->position.mOrient.rVec, 8 * objP->size / 25);
	}
}

// -----------------------------------------------------------------------------

void RenderThrusterFlames (tObject *objP)
{
	int				h, i, j, k, l, nThrusters;
	tRgbaColorf		c [2];
	vmsVector		vPos [2];
	fVector			v;
	float				fSize, fLength, fSpeed, fPulse, fFade [4];
	tThrusterData	*pt = NULL;
	tPathPoint		*pp = NULL;
	
	static time_t	tPulse = 0;
	static int		nPulse = 10;

#if SHADOWS
if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass != 1))
//	 (gameOpts->render.shadows.bFast ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
#if 1//ndef _DEBUG
if (gameStates.app.bNostalgia || !EGI_FLAG (bThrusterFlames, 0, 0))
	return;
#endif
if ((objP->nType == OBJ_PLAYER) && (gameData.multi.players [objP->id].flags & PLAYER_FLAGS_CLOAKED))
	return;
fSpeed = f2fl (VmVecMag (&objP->mType.physInfo.velocity));
fLength = fSpeed / 60.0f;
fLength += 0.2f;
if (!pt || (fSpeed >= pt->fSpeed)) {
	fFade [0] = 0.95f;
	fFade [1] = 0.85f;
	fFade [2] = 0.75f;
	fFade [3] = 0.65f;
	}
else {
	fFade [0] = 0.9f;
	fFade [1] = 0.8f;
	fFade [2] = 0.7f;
	fFade [3] = 0.6f;
	}
if (pt)
	pt->fSpeed = fSpeed;
if (objP->nType == OBJ_PLAYER) {
		pt = gameData.render.thrusters + objP->id;
		pp = GetPathPoint (&pt->path);

	if (gameStates.app.nSDLTicks - pt->tPulse > 10) {
		pt->tPulse = gameStates.app.nSDLTicks;
		pt->nPulse = d_rand () % 11;
		}
	fPulse = (float) pt->nPulse / 10.0f;
	fSize = 0.5f + fLength * 0.5f;
	nThrusters = 2;
	CalcShipThrusterPos (objP, vPos);
	}
else {
	if (!bIsMissile [objP->id])
		return;
	if (gameStates.app.nSDLTicks - tPulse > 10) {
		tPulse = gameStates.app.nSDLTicks;
		nPulse = d_rand () % 11;
		}
	fPulse = (float) nPulse / 10.0f;
	if (objP->id == EARTHSHAKER_ID)
		fSize = 1.0f;
	else if ((objP->id == MEGA_ID) || (objP->id == EARTHSHAKER_MEGA_ID))
		fSize = 0.8f;
	else if (objP->id == SMART_ID)
		fSize = 0.6f;
	else
		fSize = 0.5f;
	nThrusters = 1;
	VmVecScaleAdd (vPos, &objP->position.vPos, &objP->position.mOrient.fVec, -objP->size);
	}
CreateThrusterFlame ();
glLineWidth (3);

if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
	glDisable (GL_STENCIL_TEST);
for (h = 0; h < nThrusters; h++) {
	c [1].red = 0.5f + 0.05f * fPulse;
	c [1].green = 0.45f + 0.045f * fPulse;
	c [1].blue = 0.0f;
	c [1].alpha = 0.9f;
	G3StartInstanceMatrix (vPos + h, pp ? &pp->mOrient : &objP->position.mOrient);
	glDisable (GL_TEXTURE_2D);
	glDepthMask (0);
	glCullFace (GL_FRONT);
	for (i = 0; i < THRUSTER_SEGS - 1; i++) {
#if 1
		c [0] = c [1];
		c [1].red *= 0.975f;
		c [1].green *= 0.8f;
		c [1].alpha *= fFade [i / 4];
		glBegin (GL_QUAD_STRIP);
		for (j = 0; j < RING_SIZE + 1; j++) {
			k = j % RING_SIZE;
			for (l = 0; l < 2; l++) {
				v = vFlame [i + l][k];
				v.p.x *= fSize;
				v.p.y *= fSize;
				v.p.z *= fLength;
				G3TransformPointf (&v, &v, 0);
				glColor4f (c [l].red, c [l].green, c [l].blue, c [l].alpha);
				glVertex3fv ((GLfloat *) &v);
				}
			}
		glEnd ();
#else
		glBegin (GL_LINE_LOOP);
		glColor4f (c [1].red, c [1].green, c [1].blue, c [1].alpha);
		for (j = 0; j < RING_SIZE; j++) {
			G3TransformPointf (&v, vFlame [i] + j);
			glVertex3fv ((GLfloat *) &v);
			}
		glEnd ();
#endif
		}
	glBegin (GL_TRIANGLE_STRIP);
	for (j = 0; j < RING_SIZE; j++) {
		G3TransformPointf (&v, vFlame [0] + nStripIdx [j], 0);
		glVertex3fv ((GLfloat *) &v);
		}
	glEnd ();
	glLineWidth (1);
	glCullFace (GL_BACK);
	glDepthMask (1);
	G3DoneInstance ();
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glEnable (GL_STENCIL_TEST);
	}
}

// -----------------------------------------------------------------------------

void RenderShockwave (tObject *objP)
{
#if SHADOWS
if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass != 1))
//	 (gameOpts->render.shadows.bFast ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (!gameStates.app.bNostalgia && EGI_FLAG (bShockwaves, 0, 0) && 
	 (objP->nType == OBJ_WEAPON) && bIsWeapon [objP->id]) {
		vmsVector		vPos;
		fVector			vPosf;
		int				h, i, j, k, n;
		float				r [4], l [4], alpha;
		tRgbColorf		*pc = gameData.weapons.color + objP->id;

	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glDisable (GL_STENCIL_TEST);
	VmVecScaleAdd (&vPos, &objP->position.vPos, &objP->position.mOrient.fVec, objP->size / 2);
	G3StartInstanceMatrix (&vPos, &objP->position.mOrient);
	glDepthMask (0);
	glDisable (GL_TEXTURE_2D);
	//glCullFace (GL_FRONT);
	glDisable (GL_CULL_FACE);		
	r [3] = f2fl (objP->size);
	if (r [3] >= 3.0f)
		r [3] /= 1.5f;
	else if (r [3] < 1)
		r [3] *= 2;
	else if (r [3] < 2)
		r [3] *= 1.5f;
	r [2] = r [3];
	r [1] = r [2] / 4.0f * 3.0f;
	r [0] = r [2] / 3;
	l [3] = (r [3] < 1.0f) ? 10.0f : 20.0f;
	l [2] = r [3] / 4;
	l [1] = -r [3] / 6;
	l [0] = -r [3] / 3;
	alpha = 0.15f;
	for (h = 0; h < 3; h++) {
		glBegin (GL_QUAD_STRIP);
		for (i = 0; i < RING_SIZE + 1; i++) {
			j = i % RING_SIZE;
			for (k = 0; k < 2; k++) {
				n = h + k;
				glColor4f (pc->red, pc->green, pc->blue, (n == 3) ? 0.0f : alpha);
				vPosf = vRing [j];
				vPosf.p.x *= r [n];
				vPosf.p.y *= r [n];
				vPosf.p.z = -l [n];
				G3TransformPointf (&vPosf, &vPosf, 0);
				glVertex3fv ((GLfloat *) &vPosf);
				}
			}
		glEnd ();
		}
	glEnable (GL_CULL_FACE);		
	for (h = 0; h < 3; h += 2) {
		glCullFace (h ? GL_FRONT : GL_BACK);
		glColor4f (pc->red, pc->green, pc->blue, h ? 0.1f : alpha);
		glBegin (GL_TRIANGLE_STRIP);
		for (j = 0; j < RING_SIZE; j++) {
			vPosf = vRing [nStripIdx [j]];
			vPosf.p.x *= r [h];
			vPosf.p.y *= r [h];
			vPosf.p.z = -l [h];
			G3TransformPointf (&vPosf, &vPosf, 0);
			glVertex3fv ((GLfloat *) &vPosf);
			}
		glEnd ();
		}
	glDepthMask (1);
	glCullFace (GL_BACK);
	G3DoneInstance ();
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glEnable (GL_STENCIL_TEST);
	}
}

// -----------------------------------------------------------------------------

void RenderTracers (tObject *objP)
{
#if SHADOWS
if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass != 1))
//	 (gameOpts->render.shadows.bFast ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (!gameStates.app.bNostalgia && EGI_FLAG (bTracers, 0, 0) &&
	 (objP->nType == OBJ_WEAPON) && ((objP->id == VULCAN_ID) || (objP->id == GAUSS_ID))) {
		fVector			vPosf;
		short				h;
		static short	patterns [] = {0x0603, 0x0203, 0x0103, 0x0202};

	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glDisable (GL_STENCIL_TEST);
	glDepthMask (0);
	glEnable (GL_LINE_STIPPLE);
	h = d_rand () % 4;
	glLineStipple ((h + 1) * 4, patterns [h]);
	glLineWidth (3);
	glBegin (GL_LINES);
	glColor4d (1, 1, 1, 1.0 / 3.0);
	VmsVecToFloat (&vPosf, &objP->position.vPos);
	G3TransformPointf (&vPosf, &vPosf, 0);
	glVertex3fv ((GLfloat *) &vPosf);
	vPosf.p.x =
	vPosf.p.y =
	vPosf.p.z = -100;
	VmsVecToFloat (&vPosf, &objP->last_pos);
	G3TransformPointf (&vPosf, &vPosf, 0);
	glVertex3fv ((GLfloat *) &vPosf);
	glEnd ();
	glLineWidth (1);
	glDisable (GL_LINE_STIPPLE);
	glDepthMask (1);
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glEnable (GL_STENCIL_TEST);
	}
}

// -----------------------------------------------------------------------------

static fVector vTrailVerts [2][4] = {{{0,0,0},{0,-1,-5},{0,-1,-50},{0,0,-50}},
												  {{0,0,0},{0,1,-5},{0,1,-50},{0,0,-50}}};

void RenderLightTrail (tObject *objP)
{
#if SHADOWS
if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass != 1))
//	 (gameOpts->render.shadows.bFast ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (!gameStates.app.bNostalgia && EGI_FLAG (bLightTrails, 0, 0) && 
	 (objP->nType == OBJ_WEAPON) && bIsWeapon [objP->id]) {
		vmsVector		vPos;
		fVector			vPosf, *vTrail;
		int				i, j;
		float				h, r = f2fl (objP->size);
		tRgbColorf		*pc = gameData.weapons.color + objP->id;

	if (r <= 1)
		h = 1;
	else if (r >= 3)
		h = 2;
	else
		h = 1.5f;
	if (r >= 3.0f)
		r /= 1.5f;
	else if (r < 1)
		r *= 2;
	else if (r < 2)
		r *= 1.5f;
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glDisable (GL_STENCIL_TEST);
	VmVecScaleAdd (&vPos, &objP->position.vPos, &objP->position.mOrient.fVec, objP->size / 2);
	G3StartInstanceMatrix (&vPos, &objP->position.mOrient);
	glDepthMask (0);
	glDisable (GL_TEXTURE_2D);
	glDisable (GL_CULL_FACE);		
	for (j = 0, vTrail = vTrailVerts [0]; j < 2; j++) {
		glBegin (GL_QUADS);
		for (i = 0; i < 4; i++, vTrail++) {
			if (i)
				glColor4f (0,0,0,0);
			else
				glColor4f (pc->red, pc->green, pc->blue, 0.5f);
			vPosf.p.x = 0;
			vPosf.p.y = vTrail->p.y * r;
			vPosf.p.z = vTrail->p.z;
			if (vPosf.p.z == -50)
				vPosf.p.z *= h;
			G3TransformPointf (&vPosf, &vPosf, 0);
			glVertex3fv ((GLfloat *) &vPosf);
			}
		glEnd ();
		}
	for (j = 0, vTrail = vTrailVerts [0]; j < 2; j++) {
		glBegin (GL_QUADS);
		for (i = 0; i < 4; i++, vTrail++) {
			if (i)
				glColor4f (0,0,0,0);
			else
				glColor4f (pc->red, pc->green, pc->blue, 0.5f);
			vPosf.p.y = 0;
			vPosf.p.x = vTrail->p.y * r;
			vPosf.p.z = vTrail->p.z;
			if (vPosf.p.z == -50)
				vPosf.p.z *= h;
			G3TransformPointf (&vPosf, &vPosf, 0);
			glVertex3fv ((GLfloat *) &vPosf);
			}
		glEnd ();
		}
	glDepthMask (1);
	glCullFace (GL_BACK);
	G3DoneInstance ();
	if (EGI_FLAG (bShadows, 0, 0) && (gameStates.render.nShadowPass == 3))
		glEnable (GL_STENCIL_TEST);
	RenderShockwave (objP);
	}
}

// -----------------------------------------------------------------------------

bool G3DrawSphere3D  (g3sPoint *p0, int nSides, int rad);

void RenderObject (tObject *objP, int nWindowNum)
{
	int			mldSave, oofIdx, bSpectate = 0;
	float			fLight [3];
	fix			nGlow [2];
	tPosition	savePos;

if (OBJ_IDX (objP) == gameData.multi.players [gameData.multi.nLocalPlayer].nObject) {
	if (bSpectate = (gameStates.app.bSpectating && !nWindowNum)) {
		savePos = objP->position;
		objP->position = gameStates.app.playerPos;
		}
#ifdef _DEBUG
	 else if ((gameStates.render.nShadowPass != 2) && !gameStates.app.bPlayerIsDead &&
				 (nWindowNum || (!gameStates.render.bExternalView && (gameStates.app.bEndLevelSequence < EL_LOOKBACK)))) { //don't render ship model if neither external view nor main view
#else	 
	 else if ((gameStates.render.nShadowPass != 2) && !gameStates.app.bPlayerIsDead &&
				 (nWindowNum ||
				  ((IsMultiGame && !IsCoopGame && !EGI_FLAG (bEnableCheats, 0, 0)) || 
				  (!gameStates.render.bExternalView && (gameStates.app.bEndLevelSequence < EL_LOOKBACK))))) {
#endif	 	
		if (gameOpts->render.smoke.bPlayers)
			DoPlayerSmoke (objP, -1);
		return;		
		}
	}
if ((objP->nType == OBJ_NONE)/* || (objP->nType==OBJ_CAMBOT)*/){
#if TRACE				
	con_printf (1, "ERROR!!!Bogus obj %d in seg %d is rendering!\n", OBJ_IDX (objP), objP->nSegment);
#endif
	return;
	}
mldSave = gameStates.render.detail.nMaxLinearDepth;
gameStates.render.nState = 1;
gameStates.render.detail.nMaxLinearDepth = gameStates.render.detail.nMaxLinearDepthObjects;
SetNearestStaticLights (objP->nSegment, 1);
SetNearestDynamicLights (objP->nSegment);
switch (objP->renderType) {
	case RT_NONE:	
		RenderTracers (objP);
		break;		//doesn't render, like the tPlayer

	case RT_POLYOBJ:
		if (!(extraGameInfo [0].bShadows && extraGameInfo [IsMultiGame].bShadows) || 
			 (gameStates.render.nShadowPass == 3))
			DoObjectSmoke (objP);
		if (objP->nType == OBJ_PLAYER) {
			if (gameData.models.bHaveHiresModel [0] && 
				 (!gameStates.render.bLoResShadows || (gameStates.render.nShadowPass != 2))) {
				ComputeEngineGlow (objP, nGlow);
				fLight [0] = (float) ComputeObjectLight (objP, NULL) / 65536.0f;
				fLight [1] = (float) nGlow [0] / 65536.0f;				
				fLight [2] = (float) nGlow [1] / 65536.0f;				
				OOF_Render (objP, gameData.models.hiresModels + OOF_PYRO, fLight, 
								(gameData.multi.players [objP->id].flags & PLAYER_FLAGS_CLOAKED) != 0);
				}
			else
				DrawPolygonObject (objP);
			RenderThrusterFlames (objP);
			RenderPlayerShield (objP);
			RenderTargetIndicator (objP, NULL);
			RenderTowedFlag (objP);
			}
		else if (objP->nType == OBJ_ROBOT) {
			DrawPolygonObject (objP);
			RenderTargetIndicator (objP, NULL);
			SetRobotLocationInfo (objP);
			}
		else /*if (gameStates.render.nShadowPass != 2)*/ {
			if (gameData.models.nHiresModels && 
				 (oofIdx = idToOOF [objP->id]) && 
				 gameData.models.bHaveHiresModel [oofIdx] &&
				 (!gameStates.render.bLoResShadows || (gameStates.render.nShadowPass != 2))) {
				ComputeEngineGlow (objP, nGlow);
				fLight [0] = (float) ComputeObjectLight (objP, NULL) / 65536.0f;
				fLight [1] = (float) nGlow [0] / 65536.0f;				
				fLight [2] = (float) nGlow [1] / 65536.0f;				
				OOF_Render (objP, gameData.models.hiresModels + oofIdx, fLight, 0);
				}
			else
				DrawPolygonObject (objP);
			if (objP->nType == OBJ_WEAPON) {
				if (bIsMissile [objP->id])
					RenderThrusterFlames (objP);
				else
					RenderLightTrail (objP);
				}
			else if (objP->nType == OBJ_CNTRLCEN)
				RenderTargetIndicator (objP, NULL);
			}
		break;

	case RT_MORPH:	
		if (gameStates.render.nShadowPass != 2)
			MorphDrawObject (objP); 
		break;

	case RT_THRUSTER: 
		if (nWindowNum && (objP->mType.physInfo.flags & PF_WIGGLE))
			break;
			
	case RT_FIREBALL: 
		if (gameStates.render.nShadowPass != 2) {
			DrawFireball (objP); 
			if (objP->nType == OBJ_WEAPON)
				RenderLightTrail (objP);
			}
		break;

	case RT_WEAPON_VCLIP: 
		if (gameStates.render.nShadowPass != 2) {
			DrawWeaponVClip (objP); 
			if (objP->nType == OBJ_WEAPON)
				RenderLightTrail (objP);
			}
		break;

	case RT_HOSTAGE: 
		if (gameStates.render.nShadowPass != 2)
			DrawHostage (objP); 
		break;

	case RT_POWERUP: 
		if (gameStates.render.nShadowPass != 2)
			DrawPowerup (objP); 
		break;

	case RT_LASER: 
		if (gameStates.render.nShadowPass != 2) {
			RenderLaser (objP); 
			if (objP->nType == OBJ_WEAPON)
				RenderLightTrail (objP);
			}
		break;

	default: 
		Error ("Unknown renderType <%d>", objP->renderType);
	}
SetNearestStaticLights (objP->nSegment, 0);

#ifdef NEWDEMO
if (objP->renderType != RT_NONE)
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		if (!gameData.demo.bWasRecorded [OBJ_IDX (objP)]) {
			NDRecordRenderObject (objP);
			gameData.demo.bWasRecorded [OBJ_IDX (objP)]=1;
		}
	}
#endif
gameStates.render.detail.nMaxLinearDepth = mldSave;
if (bSpectate)
	objP->position = savePos;
}

//------------------------------------------------------------------------------

void CheckAndFixMatrix (vmsMatrix *m);

#define VmAngVecZero(v) (v)->p= (v)->b= (v)->h = 0

void ResetPlayerObject ()
{
	int i;

//Init physics
VmVecZero (&gameData.objs.console->mType.physInfo.velocity);
VmVecZero (&gameData.objs.console->mType.physInfo.thrust);
VmVecZero (&gameData.objs.console->mType.physInfo.rotVel);
VmVecZero (&gameData.objs.console->mType.physInfo.rotThrust);
gameData.objs.console->mType.physInfo.brakes = gameData.objs.console->mType.physInfo.turnRoll = 0;
gameData.objs.console->mType.physInfo.mass = gameData.pig.ship.player->mass;
gameData.objs.console->mType.physInfo.drag = gameData.pig.ship.player->drag;
gameData.objs.console->mType.physInfo.flags |= PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST;
//Init render info
gameData.objs.console->renderType = RT_POLYOBJ;
gameData.objs.console->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;		//what model is this?
gameData.objs.console->rType.polyObjInfo.nSubObjFlags = 0;		//zero the flags
gameData.objs.console->rType.polyObjInfo.nTexOverride = -1;		//no tmap override!
for (i = 0; i < MAX_SUBMODELS; i++)
	VmAngVecZero (gameData.objs.console->rType.polyObjInfo.animAngles + i);
// Clear misc
gameData.objs.console->flags = 0;
}

//------------------------------------------------------------------------------
//make object0 the tPlayer, setting all relevant fields
void InitPlayerObject ()
{
gameData.objs.console->nType = OBJ_PLAYER;
gameData.objs.console->id = 0;					//no sub-types for tPlayer
gameData.objs.console->nSignature = 0;			//tPlayer has zero, others start at 1
gameData.objs.console->size = gameData.models.polyModels [gameData.pig.ship.player->nModel].rad;
gameData.objs.console->controlType = CT_SLEW;			//default is tPlayer slewing
gameData.objs.console->movementType = MT_PHYSICS;		//change this sometime
gameData.objs.console->lifeleft = IMMORTAL_TIME;
gameData.objs.console->attachedObj = -1;
ResetPlayerObject ();
}

//------------------------------------------------------------------------------

void InitIdToOOF (void)
{
memset (idToOOF, 0, sizeof (idToOOF));
idToOOF [MEGA_ID] = OOF_MEGA;
}

//------------------------------------------------------------------------------
//sets up the d_free list & init tPlayer & whatever else
void InitObjects ()
{
	int i;
	tSegment *pSeg;

CollideInit ();
for (i = 0; i < MAX_OBJECTS; i++) {
	gameData.objs.freeList [i] = i;
	gameData.objs.objects [i].nType = OBJ_NONE;
	gameData.objs.objects [i].nSegment =
	gameData.objs.objects [i].cType.explInfo.nNextAttach =
	gameData.objs.objects [i].cType.explInfo.nPrevAttach =
	gameData.objs.objects [i].cType.explInfo.nAttachParent =
	gameData.objs.objects [i].attachedObj = -1;
	gameData.objs.objects [i].flags = 0;
	}
for (i = 0, pSeg = gameData.segs.segments;i<MAX_SEGMENTS;i++, pSeg++)
	pSeg->objects = -1;
gameData.objs.console = 
gameData.objs.viewer = 
gameData.objs.objects;
InitPlayerObject ();
LinkObject (OBJ_IDX (gameData.objs.console), 0);	//put in the world in tSegment 0
gameData.objs.nObjects = 1;						//just the tPlayer
gameData.objs.nLastObject = 0;
InitIdToOOF ();
}

//------------------------------------------------------------------------------
//after calling initObject (), the network code has grabbed specific
//tObject slots without allocating them.  Go though the gameData.objs.objects & build
//the d_free list, then set the apporpriate globals
void SpecialResetObjects (void)
{
	int i;

gameData.objs.nObjects = MAX_OBJECTS;
gameData.objs.nLastObject = 0;
Assert (gameData.objs.objects [0].nType != OBJ_NONE);		//0 should be used
for (i=MAX_OBJECTS;i--;)
	if (gameData.objs.objects [i].nType == OBJ_NONE)
		gameData.objs.freeList [--gameData.objs.nObjects] = i;
	else
		if (i > gameData.objs.nLastObject)
			gameData.objs.nLastObject = i;
}

//------------------------------------------------------------------------------
#ifdef _DEBUG
int IsObjectInSeg (int nSegment, int objn)
{
	int nObject, count = 0;

for (nObject=gameData.segs.segments [nSegment].objects;nObject!=-1;nObject=gameData.objs.objects [nObject].next)	{
	if (count > MAX_OBJECTS) 	{
		Int3 ();
		return count;
		}
	if (nObject==objn) count++;
	}
 return count;
}

//------------------------------------------------------------------------------

int SearchAllSegsForObject (int nObject)
{
	int i;
	int count = 0;

	for (i = 0; i<=gameData.segs.nLastSegment; i++) {
		count += IsObjectInSeg (i, nObject);
	}
	return count;
}

//------------------------------------------------------------------------------

void JohnsObjUnlink (int nSegment, int nObject)
{
	tObject  *objP = gameData.objs.objects+nObject;
	tSegment *seg = gameData.segs.segments+nSegment;

Assert (nObject != -1);
if (objP->prev == -1)
	seg->objects = objP->next;
else
	gameData.objs.objects [objP->prev].next = objP->next;
if (objP->next != -1)
	gameData.objs.objects [objP->next].prev = objP->prev;
}

//------------------------------------------------------------------------------

void RemoveIncorrectObjects ()
{
	int nSegment, nObject, count;

for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++) {
	count = 0;
	for (nObject = gameData.segs.segments [nSegment].objects;
		  nObject != -1;
		  nObject=gameData.objs.objects [nObject].next) {
		count++;
		#ifdef _DEBUG
		if (count > MAX_OBJECTS)	{
#if TRACE				
			con_printf (1, "Object list in tSegment %d is circular.\n", nSegment);
#endif
			Int3 ();
		}
		#endif
		if (gameData.objs.objects [nObject].nSegment != nSegment)	{
			#ifdef _DEBUG
#if TRACE				
			con_printf (CON_DEBUG, "Removing tObject %d from tSegment %d.\n", nObject, nSegment);
#endif
			Int3 ();
			#endif
			JohnsObjUnlink (nSegment, nObject);
			}
		}
	}
}

//------------------------------------------------------------------------------

void RemoveAllObjectsBut (int nSegment, int nObject)
{
	int i;

for (i = 0; i <= gameData.segs.nLastSegment; i++)
	if (nSegment != i)
		if (IsObjectInSeg (i, nObject))
			JohnsObjUnlink (i, nObject);
}

//------------------------------------------------------------------------------

int check_duplicateObjects ()
{
	int i, count = 0;
	
for (i = 0; i <= gameData.objs.nLastObject; i++) {
	if (gameData.objs.objects [i].nType != OBJ_NONE)	{
		count = SearchAllSegsForObject (i);
		if (count > 1)	{
#ifdef _DEBUG
#	if TRACE				
			con_printf (1, "Object %d is in %d segments!\n", i, count);
#	endif
			Int3 ();
#endif
			RemoveAllObjectsBut (gameData.objs.objects [i].nSegment,  i);
			return count;
			}
		}
	}
	return count;
}

//------------------------------------------------------------------------------

void list_segObjects (int nSegment)
{
	int nObject, count = 0;

for (nObject = gameData.segs.segments [nSegment].objects;
	  nObject != -1;
	  nObject = gameData.objs.objects [nObject].next) {
	count++;
	if (count > MAX_OBJECTS) 	{
		Int3 ();
		return;
		}
	}
return;
}
#endif

//------------------------------------------------------------------------------

//link the tObject into the list for its tSegment
void LinkObject (int nObject, int nSegment)
{
	tObject *objP;
	
Assert (nObject != -1);
objP = gameData.objs.objects + nObject;
Assert (objP->nSegment == -1);
Assert (nSegment >= 0 && nSegment <= gameData.segs.nLastSegment);
objP->nSegment = nSegment;
objP->next = gameData.segs.segments [nSegment].objects;
objP->prev = -1;
gameData.segs.segments [nSegment].objects = nObject;
if (objP->next != -1)
		gameData.objs.objects [objP->next].prev = nObject;

//list_segObjects (nSegment);
//check_duplicateObjects ();

Assert (gameData.objs.objects [0].next != 0);
if (gameData.objs.objects [0].next == 0)
	gameData.objs.objects [0].next = -1;

Assert (gameData.objs.objects [0].prev != 0);
if (gameData.objs.objects [0].prev == 0)
	gameData.objs.objects [0].prev = -1;
}

//------------------------------------------------------------------------------

void UnlinkObject (int nObject)
{
	tObject  *objP = gameData.objs.objects+nObject;
	tSegment *segP= gameData.segs.segments+objP->nSegment;

Assert (nObject != -1);
if (objP->prev == -1)
	segP->objects = objP->next;
else
	gameData.objs.objects [objP->prev].next = objP->next;
if (objP->next != -1) 
	gameData.objs.objects [objP->next].prev = objP->prev;
objP->nSegment = -1;
Assert (gameData.objs.objects [0].next != 0);
Assert (gameData.objs.objects [0].prev != 0);
}

//------------------------------------------------------------------------------

int nDebrisObjectCount = 0;
int nUnusedObjectsSlots;

//returns the number of a d_free tObject, updating gameData.objs.nLastObject.
//Generally, CreateObject () should be called to get an tObject, since it
//fills in important fields and does the linking.
//returns -1 if no d_free gameData.objs.objects
int AllocObject (void)
{
	int nObject;
	tObject *objP;

if (gameData.objs.nObjects >= MAX_OBJECTS - 2)
	FreeObjectSlots (MAX_OBJECTS - 10);
if (gameData.objs.nObjects >= MAX_OBJECTS)
	return -1;
nObject = gameData.objs.freeList [gameData.objs.nObjects++];
if (nObject > gameData.objs.nLastObject) {
	gameData.objs.nLastObject = nObject;
	if (gameData.objs.nLastObject > gameData.objs.nObjectLimit)
		gameData.objs.nObjectLimit = gameData.objs.nLastObject;
	}
objP = gameData.objs.objects + nObject;
objP->flags = 0;
objP->attachedObj =
objP->cType.explInfo.nNextAttach =
objP->cType.explInfo.nPrevAttach =
objP->cType.explInfo.nAttachParent = -1;
return nObject;
}

//------------------------------------------------------------------------------
//frees up an tObject.  Generally, ReleaseObject () should be called to get
//rid of an tObject.  This function deallocates the tObject entry after
//the tObject has been unlinked
void FreeObject (int nObject)
{
	int id = gameData.objs.objects [nObject].id;

DelObjChildrenN (nObject);
DelObjChildN (nObject);
KillObjectSmoke (nObject);
RemoveDynLight (-1, -1, nObject);
if (gameData.objs.objects [nObject].nType == OBJ_ROBOT)
	id = gameData.objs.objects [nObject].id;
gameData.objs.freeList [--gameData.objs.nObjects] = nObject;
Assert (gameData.objs.nObjects >= 0);
if (nObject == gameData.objs.nLastObject)
	while (gameData.objs.objects [--gameData.objs.nLastObject].nType == OBJ_NONE);
}

//-----------------------------------------------------------------------------
//	Scan the tObject list, freeing down to num_used gameData.objs.objects
//	Returns number of slots freed.
int FreeObjectSlots (int num_used)
{
	int		i, olind;
	int		objList [MAX_OBJECTS];
	int		nAlreadyFree, nToFree, nOrgNumToFree;
	tObject	*objP;

olind = 0;
nAlreadyFree = MAX_OBJECTS - gameData.objs.nLastObject - 1;

if (MAX_OBJECTS - nAlreadyFree < num_used)
	return 0;

for (i = 0; i<=gameData.objs.nLastObject; i++) {
	if (gameData.objs.objects [i].flags & OF_SHOULD_BE_DEAD) {
		nAlreadyFree++;
		if (MAX_OBJECTS - nAlreadyFree < num_used)
			return nAlreadyFree;
		}
	else
		switch (gameData.objs.objects [i].nType) {
			case OBJ_NONE:
				nAlreadyFree++;
				if (MAX_OBJECTS - nAlreadyFree < num_used)
					return 0;
				break;
			case OBJ_WALL:
			case OBJ_FLARE:
				Int3 ();		//	This is curious.  What is an tObject that is a wall?
				break;
			case OBJ_FIREBALL:
			case OBJ_WEAPON:
			case OBJ_DEBRIS:
				objList [olind++] = i;
				break;
			case OBJ_ROBOT:
			case OBJ_HOSTAGE:
			case OBJ_PLAYER:
			case OBJ_CNTRLCEN:
			case OBJ_CLUTTER:
			case OBJ_GHOST:
			case OBJ_LIGHT:
			case OBJ_CAMERA:
			case OBJ_POWERUP:
			case OBJ_MONSTERBALL:
				break;
			}
	}

nToFree = MAX_OBJECTS - num_used - nAlreadyFree;
nOrgNumToFree = nToFree;
if (nToFree > olind) {
#if TRACE				
	con_printf (1, "Warning: Asked to d_free %i gameData.objs.objects, but can only d_free %i.\n", nToFree, olind);
#endif
	nToFree = olind;
	}
for (i = 0; i<nToFree; i++) {
	objP = gameData.objs.objects + objList [i];
	if (objP->nType == OBJ_DEBRIS) {
		nToFree--;
		objP->flags |= OF_SHOULD_BE_DEAD;
		}
	}
if (!nToFree)
	return nOrgNumToFree;
for (i = 0; i<nToFree; i++) {
	objP = gameData.objs.objects + objList [i];
	if ((objP->nType == OBJ_FIREBALL)  && (objP->cType.explInfo.nDeleteObj==-1)) {
		nToFree--;
		objP->flags |= OF_SHOULD_BE_DEAD;
		}
	}
if (!nToFree)
	return nOrgNumToFree;
for (i = 0; i<nToFree; i++) {
	objP = gameData.objs.objects + objList [i];
	if ((objP->nType == OBJ_WEAPON) && (objP->id == FLARE_ID)) {
		nToFree--;
		objP->flags |= OF_SHOULD_BE_DEAD;
		}
	}
if (!nToFree)
	return nOrgNumToFree;
for (i = 0; i<nToFree; i++) {
	objP = gameData.objs.objects + objList [i];
	if ((objP->nType == OBJ_WEAPON) && (objP->id != FLARE_ID)) {
		nToFree--;
		objP->flags |= OF_SHOULD_BE_DEAD;
		}
	}
return nOrgNumToFree - nToFree;
}

//-----------------------------------------------------------------------------
//initialize a new tObject.  adds to the list for the given tSegment
//note that nSegment is really just a suggestion, since this routine actually
//searches for the correct tSegment
//returns the tObject number
int CreateObject (ubyte nType, ubyte id, short owner, short nSegment, vmsVector *pos, 
					   vmsMatrix *orient, fix size, ubyte cType, ubyte mType, ubyte rType,
						int bIgnoreLimits)
{
	short nObject;
	tObject *objP;

#ifdef _DEBUG
if (nType == OBJ_WEAPON) {
	nType = nType;
	if ((owner >= 0) && (gameData.objs.objects [owner].nType == OBJ_ROBOT))
		nType = nType;
	}	
else if (nType == OBJ_CNTRLCEN)
	nType = nType;
else if (nType == OBJ_DEBRIS)
	nType = nType;
#endif
if ((nType == OBJ_POWERUP) && !bIgnoreLimits) {
	if (TooManyPowerups (id)) {
#ifdef _DEBUG
		HUDInitMessage ("%c%cDiscarding excess powerup!", 1, 
							 (char) GrFindClosestColor (gamePalette, 63, 31, 0));
#endif
		return -2;
		}
	}
Assert (nSegment <= gameData.segs.nLastSegment);
Assert (nSegment >= 0);
if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment))
	return -1;
Assert (cType <= CT_CNTRLCEN);
if ((nType == OBJ_DEBRIS) && (nDebrisObjectCount >= gameStates.render.detail.nMaxDebrisObjects))
	return -1;
if (GetSegMasks (pos, nSegment, 0).centerMask)
	if ((nSegment = FindSegByPoint (pos, nSegment)) == -1) {
#ifdef _DEBUG
#	if TRACE				
		con_printf (CON_DEBUG, "Bad segment in CreateObject (nType=%d)\n", nType);
#	endif
#endif
		return -1;		//don't create this tObject
	}
// Find next d_free tObject
nObject = AllocObject ();
if (nObject == -1)		//no d_free gameData.objs.objects
	return -1;
Assert (gameData.objs.objects [nObject].nType == OBJ_NONE);		//make sure unused
objP = gameData.objs.objects + nObject;
Assert (objP->nSegment == -1);
// Zero out tObject structure to keep weird bugs from happening
// in uninitialized fields.
memset (objP, 0, sizeof (tObject));
objP->nSignature = gameData.objs.nNextSignature++;
objP->nType = nType;
objP->id = id;
objP->last_pos = *pos;
objP->position.vPos = *pos;
objP->size = size;
objP->flags = 0;
objP->matCenCreator = (sbyte) owner;
//@@if (orient != NULL)
//@@	objP->position.mOrient = *orient;
objP->position.mOrient = orient ? *orient : vmdIdentityMatrix;
objP->controlType = cType;
objP->movementType = mType;
objP->renderType = rType;
objP->containsType = -1;
if ((gameData.app.nGameMode & GM_ENTROPY) && (nType == OBJ_POWERUP) && (id == POW_HOARD_ORB))
	objP->lifeleft = (extraGameInfo [1].entropy.nVirusLifespan <= 0) ? 
									IMMORTAL_TIME : i2f (extraGameInfo [1].entropy.nVirusLifespan);
else
	objP->lifeleft = IMMORTAL_TIME;		//assume immortal
objP->attachedObj = -1;

if (objP->controlType == CT_POWERUP)
	objP->cType.powerupInfo.count = 1;

// Init physics info for this tObject
if (objP->movementType == MT_PHYSICS) {
	VmVecZero (&objP->mType.physInfo.velocity);
	VmVecZero (&objP->mType.physInfo.thrust);
	VmVecZero (&objP->mType.physInfo.rotVel);
	VmVecZero (&objP->mType.physInfo.rotThrust);
	objP->mType.physInfo.mass = 0;
	objP->mType.physInfo.drag = 0;
	objP->mType.physInfo.brakes = 0;
	objP->mType.physInfo.turnRoll = 0;
	objP->mType.physInfo.flags = 0;
	}
if (objP->renderType == RT_POLYOBJ)
	objP->rType.polyObjInfo.nTexOverride = -1;
objP->shields = 20 * F1_0;
nSegment = FindSegByPoint (pos, nSegment);		//find correct tSegment
Assert (nSegment != -1);
objP->nSegment = -1;					//set to zero by memset, above
LinkObject (nObject, nSegment);
//	Set (or not) persistent bit in physInfo.
if (objP->nType == OBJ_WEAPON) {
	Assert (objP->controlType == CT_WEAPON);
	objP->mType.physInfo.flags |= WI_persistent (objP->id) * PF_PERSISTENT;
	objP->cType.laserInfo.creationTime = gameData.time.xGame;
	objP->cType.laserInfo.nLastHitObj = -1;
	objP->cType.laserInfo.multiplier = F1_0;
	}
if (objP->controlType == CT_POWERUP)
	objP->cType.powerupInfo.creationTime = gameData.time.xGame;
else if (objP->controlType == CT_EXPLOSION)
	objP->cType.explInfo.nNextAttach = 
	objP->cType.explInfo.nPrevAttach = 
	objP->cType.explInfo.nAttachParent = -1;
#ifdef _DEBUG
#if TRACE				
if (bPrintObjectInfo)	
	con_printf (CON_DEBUG, "Created tObject %d of nType %d\n", nObject, objP->nType);
#endif
#endif
if (objP->nType == OBJ_DEBRIS)
	nDebrisObjectCount++;
if (IsMultiGame && (nType == OBJ_POWERUP) && PowerupClass (id)) {
	gameData.multi.powerupsInMine [(int) id]++;
	if (MultiPowerupIs4Pack (id))
		gameData.multi.powerupsInMine [(int) id - 1] += 4;
	}
return nObject;
}

//------------------------------------------------------------------------------

#ifdef EDITOR
//create a copy of an tObject. returns new tObject number
int CreateObjectCopy (int nObject, vmsVector *new_pos, int newsegnum)
{
	tObject *objP;
	int newObjNum = AllocObject ();

// Find next d_free tObject
if (newObjNum == -1)
	return -1;
obj = gameData.objs.objects + newObjNum;
*objP = gameData.objs.objects [nObject];
objP->position.vPos = objP->last_pos = *new_pos;
objP->next = objP->prev = objP->nSegment = -1;
LinkObject (newObjNum, newsegnum);
objP->nSignature = gameData.objs.nNextSignature++;
//we probably should initialize sub-structures here
return newObjNum;
}
#endif

//------------------------------------------------------------------------------

extern void NDRecordGuidedEnd ();

//remove tObject from the world
void ReleaseObject (short nObject)
{
	int pnum;
	tObject *objP = gameData.objs.objects + nObject;

Assert (nObject != -1);
Assert (nObject != 0);
Assert (objP->nType != OBJ_NONE);
Assert (objP != gameData.objs.console);
if (objP->nType == OBJ_WEAPON) {
	RespawnDestroyedWeapon (nObject);
	if (objP->id == GUIDEDMISS_ID) {
		pnum=gameData.objs.objects [objP->cType.laserInfo.nParentObj].id;
		if (pnum!=gameData.multi.nLocalPlayer)
			gameData.objs.guidedMissile [pnum]=NULL;
		else if (gameData.demo.nState==ND_STATE_RECORDING)
			NDRecordGuidedEnd ();
		}
	}
if (objP == gameData.objs.viewer)		//deleting the viewer?
	gameData.objs.viewer = gameData.objs.console;						//..make the tPlayer the viewer
if (objP->flags & OF_ATTACHED)		//detach this from tObject
	DetachOneObject (objP);
if (objP->attachedObj != -1)		//detach all gameData.objs.objects from this
	DetachAllObjects (objP);
if (objP->nType == OBJ_DEBRIS)
	nDebrisObjectCount--;
UnlinkObject (nObject);
Assert (gameData.objs.objects [0].next != 0);
if (objP->nType == OBJ_ROBOT)
	ExecObjTriggers (nObject);
objP->nType = OBJ_NONE;		//unused!
objP->nSignature = -1;
objP->nSegment=-1;				// zero it!
FreeObject (nObject);
SpawnLeftoverPowerups (nObject);
}

//------------------------------------------------------------------------------

#define	DEATH_SEQUENCE_LENGTH			 (F1_0*5)
#define	DEATH_SEQUENCE_EXPLODE_TIME	 (F1_0*2)

tObject	*viewerSaveP;
int		nPlayerFlagsSave;
fix		xCameraToPlayerDistGoal=F1_0*4;
ubyte		nControlTypeSave, nRenderTypeSave;

//------------------------------------------------------------------------------

void DeadPlayerEnd (void)
{
if (!gameStates.app.bPlayerIsDead)
	return;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordRestoreCockpit ();
gameStates.app.bPlayerIsDead = 0;
gameStates.app.bPlayerExploded = 0;
ReleaseObject (OBJ_IDX (gameData.objs.deadPlayerCamera));
gameData.objs.deadPlayerCamera = NULL;
SelectCockpit (gameStates.render.cockpit.nModeSave);
gameStates.render.cockpit.nModeSave = -1;
gameData.objs.viewer = viewerSaveP;
gameData.objs.console->nType = OBJ_PLAYER;
gameData.objs.console->flags = nPlayerFlagsSave;
Assert ((nControlTypeSave == CT_FLYING) || (nControlTypeSave == CT_SLEW));
gameData.objs.console->controlType = nControlTypeSave;
gameData.objs.console->renderType = nRenderTypeSave;
gameData.multi.players [gameData.multi.nLocalPlayer].flags &= ~PLAYER_FLAGS_INVULNERABLE;
gameStates.app.bPlayerEggsDropped = 0;
}

//------------------------------------------------------------------------------

//	Camera is less than size of tPlayer away from
void SetCameraPos (vmsVector *camera_pos, tObject *objP)
{
	int	count = 0;
	fix	xCameraPlayerDist;
	fix	xFarScale;

xCameraPlayerDist = VmVecDistQuick (camera_pos, &objP->position.vPos);
if (xCameraPlayerDist < xCameraToPlayerDistGoal) { // 2*objP->size) {
	//	Camera is too close to tPlayer tObject, so move it away.
	vmsVector	player_camera_vec;
	fvi_query	fq;
	fvi_info		hit_data;
	vmsVector	local_p1;

	VmVecSub (&player_camera_vec, camera_pos, &objP->position.vPos);
	if ((player_camera_vec.x == 0) && (player_camera_vec.y == 0) && (player_camera_vec.z == 0))
		player_camera_vec.x += F1_0/16;

	hit_data.hit.nType = HIT_WALL;
	xFarScale = F1_0;

	while ((hit_data.hit.nType != HIT_NONE) && (count++ < 6)) {
		vmsVector	closer_p1;
		VmVecNormalizeQuick (&player_camera_vec);
		VmVecScale (&player_camera_vec, xCameraToPlayerDistGoal);

		fq.p0 = &objP->position.vPos;
		VmVecAdd (&closer_p1, &objP->position.vPos, &player_camera_vec);		//	This is the actual point we want to put the camera at.
		VmVecScale (&player_camera_vec, xFarScale);						//	...but find a point 50% further away...
		VmVecAdd (&local_p1, &objP->position.vPos, &player_camera_vec);		//	...so we won't have to do as many cuts.

		fq.p1					= &local_p1;
		fq.startSeg			= objP->nSegment;
		fq.rad				= 0;
		fq.thisObjNum		= OBJ_IDX (objP);
		fq.ignoreObjList	= NULL;
		fq.flags				= 0;
		FindVectorIntersection (&fq, &hit_data);

		if (hit_data.hit.nType == HIT_NONE)
			*camera_pos = closer_p1;
		else {
			MakeRandomVector (&player_camera_vec);
			xFarScale = 3*F1_0 / 2;
			}
		}
	}
}

//------------------------------------------------------------------------------

extern void DropPlayerEggs (tObject *objP);
//extern int GetExplosionVClip (tObject *objP, int stage);
extern void MultiCapObjects ();
extern int nProximityDropped, nSmartminesDropped;

void DeadPlayerFrame (void)
{
	fix			xTimeDead;
	vmsVector	fVec;

if (gameStates.app.bPlayerIsDead) {
	xTimeDead = gameData.time.xGame - gameStates.app.nPlayerTimeOfDeath;

	//	If unable to create camera at time of death, create now.
	if (gameData.objs.deadPlayerCamera == viewerSaveP) {
		tObject *player = gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].nObject;
		int nObject = CreateObject (OBJ_CAMERA, 0, -1, player->nSegment, &player->position.vPos, 
											 &player->position.mOrient, 0, CT_NONE, MT_NONE, RT_NONE, 1);
		if (nObject != -1)
			gameData.objs.viewer = gameData.objs.deadPlayerCamera = gameData.objs.objects + nObject;
		else
			Int3 ();
		}		
	gameData.objs.console->mType.physInfo.rotVel.x = max (0, DEATH_SEQUENCE_EXPLODE_TIME - xTimeDead)/4;
	gameData.objs.console->mType.physInfo.rotVel.y = max (0, DEATH_SEQUENCE_EXPLODE_TIME - xTimeDead) / 2;
	gameData.objs.console->mType.physInfo.rotVel.z = max (0, DEATH_SEQUENCE_EXPLODE_TIME - xTimeDead)/3;
	xCameraToPlayerDistGoal = min (xTimeDead*8, F1_0*20) + gameData.objs.console->size;
	SetCameraPos (&gameData.objs.deadPlayerCamera->position.vPos, gameData.objs.console);
	VmVecSub (&fVec, &gameData.objs.console->position.vPos, &gameData.objs.deadPlayerCamera->position.vPos);
	VmVector2Matrix (&gameData.objs.deadPlayerCamera->position.mOrient, &fVec, NULL, NULL);
	if (xTimeDead > DEATH_SEQUENCE_EXPLODE_TIME) {
		if (!gameStates.app.bPlayerExploded) {
		if (gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board > 1)
			HUDInitMessage (TXT_SHIP_DESTROYED_2, gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board);
		else if (gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board == 1)
			HUDInitMessage (TXT_SHIP_DESTROYED_1);
		else
			HUDInitMessage (TXT_SHIP_DESTROYED_0);

#ifdef TACTILE
			if (TactileStick)
				ClearForces ();
#endif
			gameStates.app.bPlayerExploded = 1;
			if (gameData.app.nGameMode & GM_NETWORK) {
				AdjustMineSpawn ();
				MultiCapObjects ();
				}
			DropPlayerEggs (gameData.objs.console);
			gameStates.app.bPlayerEggsDropped = 1;
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendPlayerExplode (MULTI_PLAYER_EXPLODE);
			ExplodeBadassPlayer (gameData.objs.console);
			//is this next line needed, given the badass call above?
			ExplodeObject (gameData.objs.console, 0);
			gameData.objs.console->flags &= ~OF_SHOULD_BE_DEAD;		//don't really kill tPlayer
			gameData.objs.console->renderType = RT_NONE;				//..just make him disappear
			gameData.objs.console->nType = OBJ_GHOST;						//..and kill intersections
			gameData.multi.players [gameData.multi.nLocalPlayer].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
#if 0
			if (gameOpts->gameplay.bFastRespawn)
				gameStates.app.bDeathSequenceAborted = 1;
#endif					
			}
		}
	else {
		if (d_rand () < gameData.time.xFrame*4) {
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendCreateExplosion (gameData.multi.nLocalPlayer);
			CreateSmallFireballOnObject (gameData.objs.console, F1_0, 1);
			}
		}
	if (gameStates.app.bDeathSequenceAborted) { //xTimeDead > DEATH_SEQUENCE_LENGTH) {
		StopPlayerMovement ();
		gameStates.app.bEnterGame = 2;
		if (!gameStates.app.bPlayerEggsDropped) {
			if (gameData.app.nGameMode & GM_NETWORK) {
				AdjustMineSpawn ();
				MultiCapObjects ();
				}
			DropPlayerEggs (gameData.objs.console);
			gameStates.app.bPlayerEggsDropped = 1;
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendPlayerExplode (MULTI_PLAYER_EXPLODE);
			}
		DoPlayerDead ();		//kill_player ();
		}
	}
}

//------------------------------------------------------------------------------

void AdjustMineSpawn ()
{
if (!(gameData.app.nGameMode & GM_NETWORK))
	return;  // No need for this function in any other mode
if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)))
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX]+=nProximityDropped;
if (!(gameData.app.nGameMode & GM_ENTROPY))
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [SMART_MINE_INDEX]+=nSmartminesDropped;
nProximityDropped = 0;
nSmartminesDropped = 0;
}



int nKilledInFrame = -1;
short nKilledObjNum = -1;
extern char bMultiSuicide;

//	------------------------------------------------------------------------

void StartPlayerDeathSequence (tObject *player)
{
	int	nObject;
	
Assert (player == gameData.objs.console);
gameData.objs.speedBoost [OBJ_IDX (gameData.objs.console)].bBoosted = 0;
if ((gameStates.app.bPlayerIsDead != 0) || (gameData.objs.deadPlayerCamera != NULL))
	return;
StopConquerWarning ();
//Assert (gameStates.app.bPlayerIsDead == 0);
//Assert (gameData.objs.deadPlayerCamera == NULL);
ResetRearView ();
if (!(gameData.app.nGameMode & GM_MULTI))
	HUDClearMessages ();
nKilledInFrame = gameData.app.nFrameCount;
nKilledObjNum = OBJ_IDX (player);
gameStates.app.bDeathSequenceAborted = 0;
if (gameData.app.nGameMode & GM_MULTI) {
	MultiSendKill (gameData.multi.players [gameData.multi.nLocalPlayer].nObject);
//		If Hoard, increase number of orbs by 1
//    Only if you haven't killed yourself
//		This prevents cheating
	if (gameData.app.nGameMode & GM_HOARD)
		if (!bMultiSuicide)
			if (gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX]<12)
				gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX]++;
	}
gameStates.ogl.palAdd.red = 40;
gameStates.app.bPlayerIsDead = 1;
#ifdef TACTILE
   if (TactileStick)
	Buffeting (70);
#endif
//gameData.multi.players [gameData.multi.nLocalPlayer].flags &= ~ (PLAYER_FLAGS_AFTERBURNER);
VmVecZero (&player->mType.physInfo.rotThrust);
VmVecZero (&player->mType.physInfo.thrust);
gameStates.app.nPlayerTimeOfDeath = gameData.time.xGame;
nObject = CreateObject (OBJ_CAMERA, 0, -1, player->nSegment, &player->position.vPos, 
								&player->position.mOrient, 0, CT_NONE, MT_NONE, RT_NONE, 1);
viewerSaveP = gameData.objs.viewer;
if (nObject != -1)
	gameData.objs.viewer = gameData.objs.deadPlayerCamera = gameData.objs.objects + nObject;
else {
	Int3 ();
	gameData.objs.deadPlayerCamera = gameData.objs.viewer;
	}
if (gameStates.render.cockpit.nModeSave == -1)		//if not already saved
	gameStates.render.cockpit.nModeSave = gameStates.render.cockpit.nMode;
SelectCockpit (CM_LETTERBOX);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordLetterbox ();
nPlayerFlagsSave = player->flags;
nControlTypeSave = player->controlType;
nRenderTypeSave = player->renderType;
player->flags &= ~OF_SHOULD_BE_DEAD;
//	gameData.multi.players [gameData.multi.nLocalPlayer].flags |= PLAYER_FLAGS_INVULNERABLE;
player->controlType = CT_NONE;
if (!gameStates.entropy.bExitSequence) {
	player->shields = F1_0*1000;
	MultiSendShields ();
	}
PALETTE_FLASH_SET (0, 0, 0);
}

//------------------------------------------------------------------------------

void DeleteAllObjsThatShouldBeDead ()
{
	int		i;
	tObject	*objP;
	int		nLocalDeadPlayerObj = -1;

for (i = 0; i <= gameData.objs.nLastObject; i++) {
	objP = gameData.objs.objects + i;
	if ((objP->nType != OBJ_NONE) && (objP->flags & OF_SHOULD_BE_DEAD)) {
		Assert ((objP->nType != OBJ_FIREBALL) || (objP->cType.explInfo.nDeleteTime == -1));
		if (objP->nType != OBJ_PLAYER) 
			ReleaseObject ((short) i);
		else {
			if (objP->id == gameData.multi.nLocalPlayer) {
				if (nLocalDeadPlayerObj == -1) {
					StartPlayerDeathSequence (objP);
					nLocalDeadPlayerObj = OBJ_IDX (objP);
					}
				else
					Int3 ();
				}
			}
		}
	}
}

//--------------------------------------------------------------------
//when an tObject has moved into a new tSegment, this function unlinks it
//from its old tSegment, and links it into the new tSegment
void RelinkObject (int nObject, int newsegnum)
{
Assert ((nObject >= 0) && (nObject <= gameData.objs.nLastObject));
Assert ((newsegnum <= gameData.segs.nLastSegment) && (newsegnum >= 0));
UnlinkObject (nObject);
LinkObject (nObject, newsegnum);
#if 0//def _DEBUG
#if TRACE				
if (GetSegMasks (&gameData.objs.objects [nObject].position.vPos, 
					  gameData.objs.objects [nObject].nSegment, 0).centerMask)
	con_printf (1, "RelinkObject violates seg masks.\n");
#endif
#endif
}

//--------------------------------------------------------------------
//process a continuously-spinning tObject
void SpinObject (tObject *objP)
{
	vmsAngVec rotangs;
	vmsMatrix rotmat, new_pm;

Assert (objP->movementType == MT_SPINNING);
rotangs.p = FixMul (objP->mType.spinRate.x, gameData.time.xFrame);
rotangs.h = FixMul (objP->mType.spinRate.y, gameData.time.xFrame);
rotangs.b = FixMul (objP->mType.spinRate.z, gameData.time.xFrame);
VmAngles2Matrix (&rotmat, &rotangs);
VmMatMul (&new_pm, &objP->position.mOrient, &rotmat);
objP->position.mOrient = new_pm;
CheckAndFixMatrix (&objP->position.mOrient);
}

extern void MultiSendDropBlobs (char);
extern void FuelCenCheckForGoal (tSegment *);

//see if wall is volatile, and if so, cause damage to tPlayer
//returns true if tPlayer is in lava
int CheckVolatileWall (tObject *objP, int nSegment, int nSide, vmsVector *hitpt);
int CheckVolatileSegment (tObject *objP, int nSegment);

//	Time at which this tObject last created afterburner blobs.

//--------------------------------------------------------------------
//reset tObject's movement info
//--------------------------------------------------------------------

void StopObjectMovement (tObject *objP)
{
Controls.headingTime = 0;
Controls.pitchTime = 0;
Controls.bankTime = 0;
Controls.vertical_thrustTime = 0;
Controls.sideways_thrustTime = 0;
Controls.forward_thrustTime = 0;
VmVecZero (&objP->mType.physInfo.rotThrust);
VmVecZero (&objP->mType.physInfo.thrust);
VmVecZero (&objP->mType.physInfo.velocity);
VmVecZero (&objP->mType.physInfo.rotVel);
}

//--------------------------------------------------------------------

void StopPlayerMovement (void)
{
if (!gameData.objs.speedBoost [OBJ_IDX (gameData.objs.console)].bBoosted) {
	StopObjectMovement (gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].nObject);
	memset (&player_thrust, 0, sizeof (player_thrust));
//	gameData.time.xFrame = F1_0;
	gameData.objs.speedBoost [OBJ_IDX (gameData.objs.console)].bBoosted = 0;
	}
}

//--------------------------------------------------------------------

void MoveCamera (tObject *objP)
{

#define	DEG90		 (F1_0 / 4)		
#define	DEG45		 (F1_0 / 8)				
#define	DEG675	 (DEG45 + (F1_0 / 16))
#define	DEG1		 (F1_0 / (4 * 90))	
	
	tCamera	*pc = cameras + gameData.objs.cameraRef [OBJ_IDX (objP)];
	fixang	curAngle = pc->curAngle;
	fixang	curDelta = pc->curDelta;

#if 1
	time_t	t0 = pc->t0;
	time_t	t = gameStates.app.nSDLTicks;

if ((t0 < 0) || (t - t0 >= 1000 / 90)) 
#endif
	if (objP->cType.aiInfo.behavior == AIB_NORMAL) {
		vmsAngVec	a;
		vmsMatrix	r;

		int	h = abs (curDelta);
		int	d = DEG1 / (gameOpts->render.cameras.nSpeed / 1000);
		int	s = h ? curDelta / h : 1;

	if (h != d)
		curDelta = s * d;
#if 1
	objP->mType.physInfo.brakes = (fix) t;
#endif
	if ((curAngle >= DEG45) || (curAngle <= -DEG45)) {
		if (curAngle * s - DEG45 >= curDelta * s)
			curAngle = s * DEG45 + curDelta - s;
		curDelta = -curDelta;
		}
	
	curAngle += curDelta;
	a.h = curAngle;
	a.b =	a.p = 0;
	VmAngles2Matrix (&r, &a);
	VmMatMul (&objP->position.mOrient, &pc->orient, &r);
	pc->curAngle = curAngle;
	pc->curDelta = curDelta;
	}
}

//--------------------------------------------------------------------

void CheckObjectInVolatileWall (tObject *objP)
{
	int bChkVolaSeg = 1, nType, sideMask, bUnderLavaFall = 0;
	static int nLavaFallHissPlaying [MAX_PLAYERS]={0};

if (objP->nType != OBJ_PLAYER)
	return;
sideMask = GetSegMasks (&objP->position.vPos, objP->nSegment, objP->size).sideMask;
if (sideMask) {
	short nSide, nWall;
	int bit;
	tSide *pSide = gameData.segs.segments [objP->nSegment].sides;
	for (nSide = 0, bit = 1; nSide < 6; bit <<= 1, nSide++, pSide++) {
		if (!(sideMask & bit))
			continue;
		nWall = pSide->nWall;
		if (!IS_WALL (nWall))
			continue;
		if (gameData.walls.walls [nWall].nType != WALL_ILLUSION)
			continue;
		if (nType = CheckVolatileWall (objP, objP->nSegment, nSide, &objP->position.vPos)) {
			short sound = (nType==1) ? SOUND_LAVAFALL_HISS : SOUND_SHIP_IN_WATERFALL;
			bUnderLavaFall = 1;
			bChkVolaSeg = 0;
			if (!nLavaFallHissPlaying [objP->id]) {
				DigiLinkSoundToObject3 (sound, OBJ_IDX (objP), 1, F1_0, i2f (256), -1, -1);
				nLavaFallHissPlaying [objP->id] = 1;
				}
			}
		}
	}
if (bChkVolaSeg) {
	if (nType=CheckVolatileSegment (objP, objP->nSegment)) {
		short sound = (nType==1) ? SOUND_LAVAFALL_HISS : SOUND_SHIP_IN_WATERFALL;
		bUnderLavaFall = 1;
		if (!nLavaFallHissPlaying [objP->id]) {
			DigiLinkSoundToObject3 (sound, OBJ_IDX (objP), 1, F1_0, i2f (256), -1, -1);
			nLavaFallHissPlaying [objP->id] = 1;
			}
		}
	}
if (!bUnderLavaFall && nLavaFallHissPlaying [objP->id]) {
	DigiKillSoundLinkedToObject (OBJ_IDX (objP));
	nLavaFallHissPlaying [objP->id] = 0;
	}
}

//--------------------------------------------------------------------

void HandleSpecialSegments (tObject *objP)
{
	fix fuel, shields;
	tSegment *segP = gameData.segs.segments + objP->nSegment;
	xsegment *xsegP = gameData.segs.xSegments + objP->nSegment;
	tPlayer *playerP = gameData.multi.players + gameData.multi.nLocalPlayer;

if ((objP->nType == OBJ_PLAYER) && (gameData.multi.nLocalPlayer == objP->id)) {
   if (gameData.app.nGameMode & GM_CAPTURE)
		 FuelCenCheckForGoal (segP);
   else if (gameData.app.nGameMode & GM_HOARD)
		 FuelCenCheckForHoardGoal (segP);
   else if (gameData.app.nGameMode & GM_ENTROPY) {
		if (Controls.forward_thrustTime || 
			 Controls.vertical_thrustTime || 
			 Controls.sideways_thrustTime ||
			 (xsegP->owner < 0) ||
			 (xsegP->owner == GetTeam (gameData.multi.nLocalPlayer) + 1)) {
			StopConquerWarning ();
			gameStates.entropy.nTimeLastMoved = -1;
			}
		else if (gameStates.entropy.nTimeLastMoved < 0)
			gameStates.entropy.nTimeLastMoved = 0;
		}
	shields = HostileRoomDamageShields (segP, playerP->shields + 1);
	if (shields > 0) {
		playerP->shields -= shields;
		MultiSendShields ();
		if (playerP->shields < 0)
			StartPlayerDeathSequence (objP);
		else
			CheckConquerRoom (xsegP);
		}
	else {
		StopConquerWarning ();
		fuel=FuelCenGiveFuel (segP, INITIAL_ENERGY - playerP->energy);
		if (fuel > 0)
			playerP->energy += fuel;
		shields = RepairCenGiveShields (segP, INITIAL_SHIELDS - playerP->shields);
		if (shields > 0) {
			playerP->shields += shields;
			MultiSendShields ();
			}
		if (!xsegP->owner)
			CheckConquerRoom (xsegP);
		}
	}
}

//--------------------------------------------------------------------

int HandleObjectControl (tObject *objP)
{
switch (objP->controlType) {
	case CT_NONE: 
		break;

	case CT_FLYING:
		ReadFlyingControls (objP);
		break;

	case CT_REPAIRCEN: 
		Int3 ();	// -- hey!these are no longer supported!!-- do_repair_sequence (objP); break;

	case CT_POWERUP: 
		DoPowerupFrame (objP); 
		break;

	case CT_MORPH:			//morph implies AI
		DoMorphFrame (objP);
		//NOTE: FALLS INTO AI HERE!!!!

	case CT_AI:
		//NOTE LINK TO CT_MORPH ABOVE!!!
		if (gameStates.gameplay.bNoBotAI || (gameStates.app.bGameSuspended & SUSP_ROBOTS)) {
			DoAnyRobotDyingFrame (objP);
			return 1;
			}
		DoAIFrame (objP);
		break;

	case CT_CAMERA:		
		MoveCamera (objP); 
		break;

	case CT_WEAPON:		
		LaserDoWeaponSequence (objP); 
		break;

	case CT_EXPLOSION:	
		DoExplosionSequence (objP); 
		break;

	case CT_SLEW:
#ifdef _DEBUG
		if (keyd_pressed [KEY_PAD5]) 
			slew_stop ();
		if (keyd_pressed [KEY_NUMLOCK]) {
			slew_reset_orient ();
			* (ubyte *) 0x417 &= ~0x20;		//kill numlock
		}
		slew_frame (0);		// Does velocity addition for us.
#endif
		break;	//ignore

	case CT_DEBRIS: 
		DoDebrisFrame (objP);
		break;

	case CT_LIGHT: 
		break;		//doesn't do anything

	case CT_REMOTE: 
		break;		//movement is handled in com_process_input

	case CT_CNTRLCEN: 
		if (gameStates.gameplay.bNoBotAI)
			return 1;
		DoReactorFrame (objP); 
		break;

	default:
#ifdef __DJGPP__
		Error ("Unknown control nType %d in tObject %li, sig/nType/id = %i/%i/%i", objP->controlType, OBJ_IDX (objP), objP->nSignature, objP->nType, objP->id);
#else
		Error ("Unknown control nType %d in tObject %i, sig/nType/id = %i/%i/%i", objP->controlType, OBJ_IDX (objP), objP->nSignature, objP->nType, objP->id);
#endif
	}
return 0;
}

//--------------------------------------------------------------------

void HandleObjectMovement (tObject *objP)
{
switch (objP->movementType) {
	case MT_NONE:			
		break;								//this doesn't move

	case MT_PHYSICS:	
		DoPhysicsSim (objP);	
		SetDynLightPos (OBJ_IDX (objP));
		break;	//move by physics

	case MT_SPINNING:		
		SpinObject (objP); 
		break;
	}
}

//--------------------------------------------------------------------

int CheckObjectHitTriggers (tObject *objP, short nPrevSegment)
{
		short	nConnSide, i;
		int	nOldLevel;

if (/*(objP->nType != OBJ_PLAYER) ||*/ (objP->movementType != MT_PHYSICS) || (nPrevSegment == objP->nSegment))
	return 0;
nOldLevel = gameData.missions.nCurrentLevel;
for (i = 0; i < nPhysSegs - 1; i++) {
#ifdef _DEBUG
	if (physSegList [i] > gameData.segs.nLastSegment)
		LogErr ("invalid segment in physSegList\n");
#endif
	nConnSide = FindConnectedSide (gameData.segs.segments + physSegList [i+1], gameData.segs.segments + physSegList [i]);
	if (nConnSide != -1)
		CheckTrigger (gameData.segs.segments + physSegList [i], nConnSide, OBJ_IDX (objP), 0);
#ifdef _DEBUG
	else	// segments are not directly connected, so do binary subdivision until you find connected segments.
		LogErr ("UNCONNECTED SEGMENTS %d, %d\n", physSegList [i+1], physSegList [i]);
#endif
	//maybe we've gone on to the next level.  if so, bail!
	if (gameData.missions.nCurrentLevel != nOldLevel)
		return 1;
	}
return 0;
}

//--------------------------------------------------------------------

void CheckGuidedMissileThroughExit (tObject *objP, short nPrevSegment)
{
if ((objP == gameData.objs.guidedMissile [gameData.multi.nLocalPlayer]) && 
	 (objP->nSignature == gameData.objs.guidedMissileSig [gameData.multi.nLocalPlayer])) {
	if (nPrevSegment != objP->nSegment) {
		short	nConnSide = FindConnectedSide (gameData.segs.segments + objP->nSegment, gameData.segs.segments+nPrevSegment);
		if (nConnSide != -1) {
			short nWall, nTrigger;
			nWall = WallNumI (nPrevSegment, nConnSide);
			if (IS_WALL (nWall)) {
				nTrigger = gameData.walls.walls [nWall].nTrigger;
				if ((nTrigger < gameData.trigs.nTriggers) &&
					 (gameData.trigs.triggers [nTrigger].nType == TT_EXIT))
					gameData.objs.guidedMissile [gameData.multi.nLocalPlayer]->lifeleft = 0;
				}
			}
		}
	}
}

//--------------------------------------------------------------------

void CheckAfterburnerBlobDrop (tObject *objP)
{
if (gameStates.render.bDropAfterburnerBlob) {
	Assert (objP == gameData.objs.console);
	DropAfterburnerBlobs (objP, 2, i2f (5) / 2, -1, NULL, 0);	//	-1 means use default lifetime
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendDropBlobs ((char) gameData.multi.nLocalPlayer);
	gameStates.render.bDropAfterburnerBlob = 0;
	}

if ((objP->nType == OBJ_WEAPON) && (gameData.weapons.info [objP->id].afterburner_size)) {
	int	nObject = OBJ_IDX (objP);
	fix	vel = VmVecMagQuick (&objP->mType.physInfo.velocity);
	fix	delay, lifetime;

	if (vel > F1_0*200)
		delay = F1_0/16;
	else if (vel > F1_0*40)
		delay = FixDiv (F1_0*13, vel);
	else
		delay = DEG90;

	lifetime = (delay * 3) / 2;
	if (!(gameData.app.nGameMode & GM_MULTI)) {
		delay /= 2;
		lifetime *= 2;
		}

	if ((objP->nType == OBJ_WEAPON) &&
		 bIsMissile [objP->id] &&
		 ((SHOW_SMOKE && gameOpts->render.smoke.bMissiles)
		  || gameStates.app.bNostalgia
		  || !EGI_FLAG (bThrusterFlames, 0, 0)
		 ))
		return;
	if ((gameData.objs.xLastAfterburnerTime [nObject] + delay < gameData.time.xGame) || 
		 (gameData.objs.xLastAfterburnerTime [nObject] > gameData.time.xGame)) {
		DropAfterburnerBlobs (objP, 1, i2f (gameData.weapons.info [objP->id].afterburner_size)/16, lifetime, NULL, 0);
		gameData.objs.xLastAfterburnerTime [nObject] = gameData.time.xGame;
		}
	}
}

//--------------------------------------------------------------------
//move an tObject for the current frame

int MoveOneObject (tObject * objP)
{
	short	nPrevSegment = (short) objP->nSegment;

//if (gameStates.gameplay.bNoBotAI && (objP->nType != OBJ_PLAYER))
//	return 1;
#ifdef _DEBUG
if ((objP->nType == OBJ_PLAYER) && (gameData.multi.players [objP->id].shields < 1)) {
	if ((gameData.multi.players [objP->id].shields < 0) && !(objP->flags & OF_SHOULD_BE_DEAD))
		HUDInitMessage ("Player should be dead");
	}
if (objP->nType == OBJ_DEBRIS)
	objP = objP;
#endif
objP->last_pos = objP->position.vPos;			// Save the current position
HandleSpecialSegments (objP);
if (objP->lifeleft != IMMORTAL_TIME) {	//if not immortal...
	//	Ok, this is a big hack by MK.
	//	If you want an tObject to last for exactly one frame, then give it a lifeleft of ONE_FRAME_TIME.
	if (objP->lifeleft != ONE_FRAME_TIME)
		if (gameData.time.xFrame != F1_0)
			objP->lifeleft -= gameData.time.xFrame;		//...inevitable countdown towards death
}

gameStates.render.bDropAfterburnerBlob = 0;
if (HandleObjectControl (objP))
	return 1;

if (objP->lifeleft < 0) {		// We died of old age
	objP->flags |= OF_SHOULD_BE_DEAD;
	if ((objP->nType == OBJ_WEAPON) && WI_damage_radius (objP->id))
		ExplodeBadassWeapon (objP, &objP->position.vPos);
	else if (objP->nType == OBJ_ROBOT)	//make robots explode
		ExplodeObject (objP, 0);
	}

if ((objP->nType == OBJ_NONE) || (objP->flags & OF_SHOULD_BE_DEAD)) {
	return 1;			//tObject has been deleted
	}
HandleObjectMovement (objP);
if (CheckObjectHitTriggers (objP, nPrevSegment))
	return 0;
CheckObjectInVolatileWall (objP);
CheckGuidedMissileThroughExit (objP, nPrevSegment);
CheckAfterburnerBlobDrop (objP);
return 1;
}

int	nMaxUsedObjects = MAX_OBJECTS - 20;

//--------------------------------------------------------------------
//move all gameData.objs.objects for the current frame
int MoveAllObjects ()
{
	int i;
	tObject *objP;

//	check_duplicateObjects ();
//	RemoveIncorrectObjects ();

if (gameData.objs.nLastObject > nMaxUsedObjects)
	FreeObjectSlots (nMaxUsedObjects);		//	Free all possible tObject slots.

DeleteAllObjsThatShouldBeDead ();
if (gameOpts->gameplay.bAutoLeveling)
	gameData.objs.console->mType.physInfo.flags |= PF_LEVELLING;
else
	gameData.objs.console->mType.physInfo.flags &= ~PF_LEVELLING;

// Move all gameData.objs.objects
objP = gameData.objs.objects;
#ifndef DEMO_ONLY
gameStates.entropy.bConquering = 0;
UpdatePlayerOrient ();
for (i = 0; i <= gameData.objs.nLastObject; i++) {
	if ((objP->nType != OBJ_NONE) && !(objP->flags & OF_SHOULD_BE_DEAD)) {
		if (!MoveOneObject (objP))
			return 0;
	}
	objP++;
}
#else
	i = 0;	//kill warning
#endif
return 1;
//	check_duplicateObjects ();
//	RemoveIncorrectObjects ();

}


//--unused-- // -----------------------------------------------------------
//--unused-- //	Moved here from eobject.c on 02/09/94 by MK.
//--unused-- int find_last_obj (int i)
//--unused-- {
//--unused-- 	for (i=MAX_OBJECTS;--i >= 0;)
//--unused-- 		if (gameData.objs.objects [i].nType != OBJ_NONE) break;
//--unused--
//--unused-- 	return i;
//--unused--
//--unused-- }


//make tObject array non-sparse
void compressObjects (void)
{
	int start_i;	//, last_i;

	//last_i = find_last_obj (MAX_OBJECTS);

	//	Note: It's proper to do < (rather than <=) gameData.objs.nLastObject here because we
	//	are just removing gaps, and the last tObject can't be a gap.
	for (start_i = 0;start_i<gameData.objs.nLastObject;start_i++)

		if (gameData.objs.objects [start_i].nType == OBJ_NONE) {

			int	segnum_copy;

			segnum_copy = gameData.objs.objects [gameData.objs.nLastObject].nSegment;

			UnlinkObject (gameData.objs.nLastObject);

			gameData.objs.objects [start_i] = gameData.objs.objects [gameData.objs.nLastObject];

			#ifdef EDITOR
			if (CurObject_index == gameData.objs.nLastObject)
				CurObject_index = start_i;
			#endif

			gameData.objs.objects [gameData.objs.nLastObject].nType = OBJ_NONE;

			LinkObject (start_i, segnum_copy);

			while (gameData.objs.objects [--gameData.objs.nLastObject].nType == OBJ_NONE);

			//last_i = find_last_obj (last_i);
			
		}

	ResetObjects (gameData.objs.nObjects);

}

//------------------------------------------------------------------------------
//called after load.  Takes number of gameData.objs.objects,  and gameData.objs.objects should be
//compressed.  resets d_free list, marks unused gameData.objs.objects as unused
void ResetObjects (int n_objs)
{
	int i;

gameData.objs.nObjects = n_objs;
Assert (gameData.objs.nObjects > 0);
for (i = gameData.objs.nObjects; i < MAX_OBJECTS; i++) {
	gameData.objs.freeList [i] = i;
	gameData.objs.objects [i].nType = OBJ_NONE;
	gameData.objs.objects [i].nSegment =
	gameData.objs.objects [i].cType.explInfo.nNextAttach =
	gameData.objs.objects [i].cType.explInfo.nPrevAttach =
	gameData.objs.objects [i].cType.explInfo.nAttachParent =
	gameData.objs.objects [i].attachedObj = -1;
	gameData.objs.objects [i].flags = 0;
	//KillObjectSmoke (i);
	}
gameData.objs.nLastObject = gameData.objs.nObjects - 1;
nDebrisObjectCount = 0;
}

//------------------------------------------------------------------------------
//Tries to find a tSegment for an tObject, using FindSegByPoint ()
int FindObjectSeg (tObject * objP)
{
return FindSegByPoint (&objP->position.vPos, objP->nSegment);
}


//------------------------------------------------------------------------------
//If an tObject is in a tSegment, set its nSegment field and make sure it's
//properly linked.  If not in any tSegment, returns 0, else 1.
//callers should generally use FindVectorIntersection ()
int UpdateObjectSeg (tObject * objP)
{
	int newseg;

newseg = FindObjectSeg (objP);
if (newseg == -1)
	return 0;
if (newseg != objP->nSegment)
	RelinkObject (OBJ_IDX (objP), newseg);
return 1;
}

//------------------------------------------------------------------------------
//go through all gameData.objs.objects and make sure they have the correct tSegment numbers
void FixObjectSegs ()
{
	int i;

for (i = 0; i <= gameData.objs.nLastObject; i++)
	if (gameData.objs.objects [i].nType != OBJ_NONE)
		if (UpdateObjectSeg (gameData.objs.objects + i) == 0) {
#if TRACE				
			con_printf (1, "Cannot find tSegment for tObject %d in FixObjectSegs ()\n");
#endif
			Int3 ();
			COMPUTE_SEGMENT_CENTER_I (&gameData.objs.objects [i].position.vPos, gameData.objs.objects [i].nSegment);
			}
}


//--unused-- void object_use_newObject_list (tObject * new_list)
//--unused-- {
//--unused-- 	int i, nSegment;
//--unused-- 	tObject *objP;
//--unused--
//--unused-- 	// First, unlink all the old gameData.objs.objects for the segments array
//--unused-- 	for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++) {
//--unused-- 		gameData.segs.segments [nSegment].objects = -1;
//--unused-- 	}
//--unused-- 	// Then, erase all the gameData.objs.objects
//--unused-- 	ResetObjects (1);
//--unused--
//--unused-- 	// Fill in the tObject array
//--unused-- 	memcpy (gameData.objs.objects, new_list, sizeof (tObject)*MAX_OBJECTS);
//--unused--
//--unused-- 	gameData.objs.nLastObject=-1;
//--unused--
//--unused-- 	// Relink 'em
//--unused-- 	for (i = 0; i<MAX_OBJECTS; i++)	{
//--unused-- 		obj = &gameData.objs.objects [i];
//--unused-- 		if (objP->nType != OBJ_NONE)	{
//--unused-- 			gameData.objs.nObjects++;
//--unused-- 			gameData.objs.nLastObject = i;
//--unused-- 			nSegment = objP->nSegment;
//--unused-- 			objP->next = objP->prev = objP->nSegment = -1;
//--unused-- 			LinkObject (i, nSegment);
//--unused-- 		} else {
//--unused-- 			objP->next = objP->prev = objP->nSegment = -1;
//--unused-- 		}
//--unused-- 	}
//--unused-- 	
//--unused-- }

//delete gameData.objs.objects, such as weapons & explosions, that shouldn't stay between levels
//	Changed by MK on 10/15/94, don't remove proximity bombs.
//if clear_all is set, clear even proximity bombs
void clear_transientObjects (int clear_all)
{
	short nObject;
	tObject *objP;

	for (nObject = 0, objP = gameData.objs.objects; nObject <= gameData.objs.nLastObject; nObject++, objP++)
		if (((objP->nType == OBJ_WEAPON) && !(gameData.weapons.info [objP->id].flags&WIF_PLACABLE) && (clear_all || ((objP->id != PROXIMITY_ID) && (objP->id != SUPERPROX_ID)))) ||
			   objP->nType == OBJ_FIREBALL ||
			   objP->nType == OBJ_DEBRIS ||
			   objP->nType == OBJ_DEBRIS ||
			   ((objP->nType != OBJ_NONE) && (objP->flags & OF_EXPLODING))) {

			#ifdef _DEBUG
#if TRACE				
			if (gameData.objs.objects [nObject].lifeleft > i2f (2))
				con_printf (CON_DEBUG, "Note: Clearing tObject %d (nType=%d, id=%d) with lifeleft=%x\n", nObject, gameData.objs.objects [nObject].nType, gameData.objs.objects [nObject].id, gameData.objs.objects [nObject].lifeleft);
#endif
			#endif
			ReleaseObject (nObject);
		}
		#ifdef _DEBUG
#if TRACE				
		 else if (gameData.objs.objects [nObject].nType!=OBJ_NONE && gameData.objs.objects [nObject].lifeleft < i2f (2))
			con_printf (CON_DEBUG, "Note: NOT clearing tObject %d (nType=%d, id=%d) with lifeleft=%x\n", nObject, gameData.objs.objects [nObject].nType, gameData.objs.objects [nObject].id, gameData.objs.objects [nObject].lifeleft);
#endif
		#endif
}

//------------------------------------------------------------------------------
//attaches an tObject, such as a fireball, to another tObject, such as a robot
void AttachObject (tObject *parent, tObject *sub)
{
	Assert (sub->nType == OBJ_FIREBALL);
	Assert (sub->controlType == CT_EXPLOSION);

	Assert (sub->cType.explInfo.nNextAttach==-1);
	Assert (sub->cType.explInfo.nPrevAttach==-1);

	Assert (parent->attachedObj == -1 || 
			 gameData.objs.objects [parent->attachedObj].cType.explInfo.nPrevAttach==-1);

	sub->cType.explInfo.nNextAttach = parent->attachedObj;

	if (sub->cType.explInfo.nNextAttach != -1)
		gameData.objs.objects [sub->cType.explInfo.nNextAttach].cType.explInfo.nPrevAttach = OBJ_IDX (sub);

	parent->attachedObj = OBJ_IDX (sub);

	sub->cType.explInfo.nAttachParent = OBJ_IDX (parent);
	sub->flags |= OF_ATTACHED;

	Assert (sub->cType.explInfo.nNextAttach != OBJ_IDX (sub));
	Assert (sub->cType.explInfo.nPrevAttach != OBJ_IDX (sub));
}

//------------------------------------------------------------------------------
//dettaches one tObject
void DetachOneObject (tObject *sub)
{
	Assert (sub->flags & OF_ATTACHED);
	Assert (sub->cType.explInfo.nAttachParent != -1);

if ((gameData.objs.objects [sub->cType.explInfo.nAttachParent].nType != OBJ_NONE) &&
	 (gameData.objs.objects [sub->cType.explInfo.nAttachParent].attachedObj != -1)) {
	if (sub->cType.explInfo.nNextAttach != -1) {
		Assert (gameData.objs.objects [sub->cType.explInfo.nNextAttach].cType.explInfo.nPrevAttach=OBJ_IDX (sub));
		gameData.objs.objects [sub->cType.explInfo.nNextAttach].cType.explInfo.nPrevAttach = sub->cType.explInfo.nPrevAttach;
		}
	if (sub->cType.explInfo.nPrevAttach != -1) {
		Assert (gameData.objs.objects [sub->cType.explInfo.nPrevAttach].cType.explInfo.nNextAttach=OBJ_IDX (sub));
		gameData.objs.objects [sub->cType.explInfo.nPrevAttach].cType.explInfo.nNextAttach = 
			sub->cType.explInfo.nNextAttach;
		}
	else {
		Assert (gameData.objs.objects [sub->cType.explInfo.nAttachParent].attachedObj=OBJ_IDX (sub));
		gameData.objs.objects [sub->cType.explInfo.nAttachParent].attachedObj = sub->cType.explInfo.nNextAttach;
		}
	}
sub->cType.explInfo.nNextAttach = 
sub->cType.explInfo.nPrevAttach =
sub->cType.explInfo.nAttachParent = -1;
sub->flags &= ~OF_ATTACHED;
}

//------------------------------------------------------------------------------
//dettaches all gameData.objs.objects from this tObject
void DetachAllObjects (tObject *parent)
{
while (parent->attachedObj != -1)
	DetachOneObject (gameData.objs.objects + parent->attachedObj);
}

//------------------------------------------------------------------------------
//creates a marker tObject in the world.  returns the tObject number
int DropMarkerObject (vmsVector *pos, short nSegment, vmsMatrix *orient, ubyte marker_num)
{
	short nObject;

	Assert (gameData.models.nMarkerModel != -1);
	nObject = CreateObject (OBJ_MARKER, marker_num, -1, nSegment, pos, orient, 
								  gameData.models.polyModels [gameData.models.nMarkerModel].rad, CT_NONE, MT_NONE, RT_POLYOBJ, 1);
	if (nObject >= 0) {
		tObject *objP = &gameData.objs.objects [nObject];
		objP->rType.polyObjInfo.nModel = gameData.models.nMarkerModel;
		VmVecCopyScale (&objP->mType.spinRate, &objP->position.mOrient.uVec, F1_0 / 2);
		//	MK, 10/16/95: Using lifeleft to make it flash, thus able to trim lightlevel from all gameData.objs.objects.
		objP->lifeleft = IMMORTAL_TIME - 1;
	}

	return nObject;	
}

//------------------------------------------------------------------------------

extern int nAiLastMissileCamera;

//	*viewer is a viewer, probably a missile.
//	wake up all robots that were rendered last frame subject to some constraints.
void WakeupRenderedObjects (tObject *viewer, int window_num)
{
	int	i;

	//	Make sure that we are processing current data.
	if (gameData.app.nFrameCount != windowRenderedData [window_num].frame) {
#if TRACE				
		con_printf (1, "Warning: Called WakeupRenderedObjects with a bogus window.\n");
#endif
		return;
	}

	nAiLastMissileCamera = OBJ_IDX (viewer);

	for (i = 0; i<windowRenderedData [window_num].numObjects; i++) {
		int	nObject;
		tObject *objP;
		int	fcval = gameData.app.nFrameCount & 3;

		nObject = windowRenderedData [window_num].renderedObjects [i];
		if ((nObject & 3) == fcval) {
			objP = &gameData.objs.objects [nObject];
	
			if (objP->nType == OBJ_ROBOT) {
				if (VmVecDistQuick (&viewer->position.vPos, &objP->position.vPos) < F1_0*100) {
					tAILocal		*ailp = &gameData.ai.localInfo [nObject];
					if (ailp->playerAwarenessType == 0) {
						objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_CAMERA_AWAKE;
						ailp->playerAwarenessType = PA_WEAPON_ROBOT_COLLISION;
						ailp->playerAwarenessTime = F1_0*3;
						ailp->previousVisibility = 2;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void ResetChildObjects (void)
{
	int	i;

for (i = 0; i < MAX_OBJECTS; i++) {
	gameData.objs.childObjs [i].objIndex = -1;
	gameData.objs.childObjs [i].nextObj = i + 1;
	}
gameData.objs.childObjs [i - 1].nextObj = -1;
gameData.objs.nChildFreeList = 0;
memset (gameData.objs.firstChild, 0xff, sizeof (gameData.objs.firstChild));
memset (gameData.objs.parentObjs, 0xff, sizeof (gameData.objs.parentObjs));
}

//------------------------------------------------------------------------------

int CheckChildList (int nParent)
{
	int h, i, j;

if (gameData.objs.firstChild [nParent] == gameData.objs.nChildFreeList)
	return 0;
for (h = 0, i = gameData.objs.firstChild [nParent]; i >= 0; i = j, h++) {
	j = gameData.objs.childObjs [i].nextObj;
	if (j == i)
		return 0;
	if (h > gameData.objs.nLastObject)
		return 0;
	}
return 1;
}

//------------------------------------------------------------------------------

int AddChildObjectN (int nParent, int nChild)
{
	int	h, i;

if (gameData.objs.nChildFreeList < 0)
	return 0;
h = gameData.objs.firstChild [nParent];
i = gameData.objs.firstChild [nParent] = gameData.objs.nChildFreeList;
gameData.objs.nChildFreeList = gameData.objs.childObjs [gameData.objs.nChildFreeList].nextObj;
gameData.objs.childObjs [i].nextObj = h;
gameData.objs.childObjs [i].objIndex = nChild;
gameData.objs.parentObjs [nChild] = nParent;
CheckChildList (nParent);
return 1;
}

//------------------------------------------------------------------------------

int AddChildObjectP (tObject *pParent, tObject *pChild)
{
return pParent ? AddChildObjectN (OBJ_IDX (pParent), OBJ_IDX (pChild)) : 0;
//return (pParent->nType == OBJ_PLAYER) ? AddChildObjectN (OBJ_IDX (pParent), OBJ_IDX (pChild)) : 0;
}

//------------------------------------------------------------------------------

int DelObjChildrenN (int nParent)
{
	int	i, j;

for (i = gameData.objs.firstChild [nParent]; i >= 0; i = j) {
	j = gameData.objs.childObjs [i].nextObj;
	gameData.objs.childObjs [i].nextObj = gameData.objs.nChildFreeList;
	gameData.objs.childObjs [i].objIndex = -1;
	gameData.objs.nChildFreeList = i;
	}
return 1;
}

//------------------------------------------------------------------------------

int DelObjChildrenP (tObject *pParent)
{
return DelObjChildrenN (OBJ_IDX (pParent));
}

//------------------------------------------------------------------------------

int DelObjChildN (int nChild)
{
	int	nParent, h = -1, i, j;

if (0 > (nParent = gameData.objs.parentObjs [nChild]))
	return 0;
for (i = gameData.objs.firstChild [nParent]; i >= 0; i = j) {
	j = gameData.objs.childObjs [i].nextObj;
	if (gameData.objs.childObjs [i].objIndex == nChild) {
		if (h < 0)
			gameData.objs.firstChild [nParent] = j;
		else
			gameData.objs.childObjs [h].nextObj = j;
		gameData.objs.childObjs [i].nextObj = gameData.objs.nChildFreeList;
		gameData.objs.childObjs [i].objIndex = -1;
		gameData.objs.nChildFreeList = i;
		CheckChildList (nParent);
		return 1;
		}
	h = i;
	}
return 0;
}

//------------------------------------------------------------------------------

int DelObjChildP (tObject *pChild)
{
return DelObjChildN (OBJ_IDX (pChild));
}

//------------------------------------------------------------------------------

tObjectRef *GetChildObjN (short nParent, tObjectRef *pChildRef)
{
	int i = pChildRef ? pChildRef->nextObj : gameData.objs.firstChild [nParent];
	
return (i < 0) ? NULL : gameData.objs.childObjs + i;
}

//------------------------------------------------------------------------------

tObjectRef *GetChildObjP (tObject *pParent, tObjectRef *pChildRef)
{
return GetChildObjN (OBJ_IDX (pParent), pChildRef);
}

//------------------------------------------------------------------------------

void InitWeaponFlags (void)
{
memset (bIsMissile, 0, sizeof (bIsMissile));
bIsMissile [CONCUSSION_ID] =
bIsMissile [HOMING_ID] =
bIsMissile [SMART_ID] =
bIsMissile [MEGA_ID] =
bIsMissile [FLASH_ID] =
bIsMissile [GUIDEDMISS_ID] =
bIsMissile [MERCURY_ID] =
bIsMissile [EARTHSHAKER_ID] =
bIsMissile [EARTHSHAKER_MEGA_ID] =
bIsMissile [REGULAR_MECH_MISS_ID] =
bIsMissile [SUPER_MECH_MISS_ID] =
bIsMissile [ROBOT_FLASHMISS_ID] =
bIsMissile [ROBOT_MERCURY_ID] =
bIsMissile [ROBOT_VERTIGO_FLASHMISS_ID] =
bIsMissile [ROBOT_SMART_ID] =
bIsMissile [ROBOT_MEGA_ID] =
bIsMissile [ROBOT_EARTHSHAKER_ID] =
bIsMissile [ROBOT_SHAKER_MEGA_ID] = 1;

memset (bIsWeapon, 0, sizeof (bIsWeapon));
bIsWeapon [VULCAN_ID] =
bIsWeapon [GAUSS_ID] = 0;
bIsWeapon [LASER_ID] =
bIsWeapon [LASER_ID + 1] =
bIsWeapon [LASER_ID + 2] =
bIsWeapon [LASER_ID + 3] =
bIsWeapon [SPREADFIRE_ID] =
bIsWeapon [PLASMA_ID] =
bIsWeapon [FUSION_ID] =
bIsWeapon [SUPER_LASER_ID] =
bIsWeapon [SUPER_LASER_ID + 1] =
bIsWeapon [HELIX_ID] =
bIsWeapon [PHOENIX_ID] =
bIsWeapon [OMEGA_ID] =
bIsWeapon [ROBOT_PLASMA_ID] =
bIsWeapon [ROBOT_PHOENIX_ID] =
bIsWeapon [ROBOT_FAST_PHOENIX_ID] =
bIsWeapon [ROBOT_VERTIGO_FLASHMISS_ID] =
bIsWeapon [ROBOT_VERTIGO_FLASHMISS_ID + 1] =
bIsWeapon [ROBOT_VERTIGO_FIREBALL_ID] =
bIsWeapon [ROBOT_VERTIGO_PHOENIX_ID] =
bIsWeapon [ROBOT_HELIX_ID] =
bIsWeapon [ROBOT_BLUE_ENERGY_ID] =
bIsWeapon [ROBOT_WHITE_ENERGY_ID] =
bIsWeapon [ROBOT_BLUE_LASER_ID] =
bIsWeapon [ROBOT_GREEN_LASER_ID] =
bIsWeapon [ROBOT_WHITE_LASER_ID] = 1;
}

//------------------------------------------------------------------------------
//eof
