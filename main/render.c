/* $Id: render.c, v 1.18 2003/10/10 09:36:35 btb Exp $ */
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "inferno.h"
#include "segment.h"
#include "error.h"
#include "bm.h"
#include "texmap.h"
#include "mono.h"
#include "render.h"
#include "game.h"
#include "object.h"
#include "laser.h"
#include "textures.h"
#include "screens.h"
#include "segpoint.h"
#include "wall.h"
#include "texmerge.h"
#include "physics.h"
#include "3d.h"
#include "gameseg.h"
#include "vclip.h"
#include "lighting.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "automap.h"
#include "endlevel.h"
#include "key.h"
#include "newmenu.h"
#include "u_mem.h"
#include "piggy.h"
#include "network.h"
#include "switch.h"
#include "hudmsg.h"
#include "cameras.h"
#include "kconfig.h"
#include "mouse.h"
#include "particles.h"
#include "globvars.h"
#include "interp.h"
#include "oof.h"
#include "ogl_init.h"
#include "lightmap.h"
#include "gauges.h"
#include "sphere.h"

//------------------------------------------------------------------------------

#define SOFT_SHADOWS		0

#if DBG_SHADOWS
extern int bShadowTest;
extern int bFrontCap;
extern int bRearCap;
extern int bShadowVolume;
extern int bFrontFaces;
extern int bBackFaces;
extern int bSWCulling;
extern int bWallShadows;
#endif
#if SHADOWS
extern int bZPass;
#endif

grsBitmap *bmpCorona = NULL;
int bHaveCorona = 0;

int _CDECL_ D2X_RenderThread (void *p);
int _CDECL_ D2X_OpenGLThread (void *p);

extern tFaceColor tMapColor, lightColor, vertColors [4];
char bUseGlobalColor = 0;

#if DBG_SHADOWS
extern int bShadowTest;
#endif

#define INITIAL_LOCAL_LIGHT (F1_0/4)    // local light value in tSegment of occurence (of light emission)

#ifdef EDITOR
#include "editor/editor.h"
#endif

//used for checking if points have been bRotated
unsigned int	nClearWindowColor = 0;
int				nClearWindow = 2;	// 1 = Clear whole background window, 2 = clear view portals into rest of world, 0 = no clear

int nRotatedLast [MAX_VERTICES_D2X];

// When any render function needs to know what's looking at it, it should 
// access gameData.objs.viewer members.


ubyte bObjectRendered [MAX_OBJECTS_D2X];

#ifdef EDITOR
int	Render_only_bottom = 0;
int	Bottom_bitmap_num = 9;
#endif

fix	Face_reflectivity = (F1_0/2);

#ifdef _DEBUG
short nDbgSeg = -1;
short nDbgSide = -1;
int nDbgVertex = -1;
#endif

//------------------------------------------------------------------------------

#ifdef EDITOR
int bSearchMode = 0;			//true if looking for curseg, tSide, face
short _search_x, _search_y;	//pixel we're looking at
int found_seg, found_side, found_face, found_poly;
#else
#define bSearchMode 0
#endif

int	bOutLineMode = 0, 
		bShowOnlyCurSide = 0;

//------------------------------------------------------------------------------

int ToggleOutlineMode (void)
{
return bOutLineMode = !bOutLineMode;
}

//------------------------------------------------------------------------------

int ToggleShowOnlyCurSide (void)
{
return bShowOnlyCurSide = !bShowOnlyCurSide;
}

//------------------------------------------------------------------------------

int LoadCorona (void)
{
if (!bHaveCorona) {
	bmpCorona = CreateAndReadTGA ("corona.tga");
	bHaveCorona = bmpCorona ? 1 : -1;
	}
return bHaveCorona > 0;
}

//------------------------------------------------------------------------------

void FreeCorona (void)
{
if (bmpCorona) {
	GrFreeBitmap (bmpCorona);
	bmpCorona = NULL;
	bHaveCorona = 0;
	}
}

//------------------------------------------------------------------------------

void DrawOutline (int nv, g3sPoint **pointList)
{
	int i;
	GLint depthFunc; 
	g3sPoint center, normal;
	vmsVector n;
	fVector *nf;

#if 1 //def RELEASE
if (gameStates.render.bQueryOcclusion) {
	G3DrawPolyAlpha (nv, pointList, 1, 1, 0, -1);
	return;
	}
#endif

glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
glDepthFunc (GL_ALWAYS);
GrSetColorRGB (255, 255, 255, 255);
VmVecZero (&center.p3_vec);
for (i = 0; i < nv; i++) {
	G3DrawLine (pointList [i], pointList [(i + 1) % nv]);
	VmVecInc (&center.p3_vec, &pointList [i]->p3_vec);
	nf = &pointList [i]->p3_normal.vNormal;
	n.p.x = (fix) (nf->p.x * 65536.0f);
	n.p.y = (fix) (nf->p.y * 65536.0f);
	n.p.z = (fix) (nf->p.z * 65536.0f);
	G3RotatePoint (&n, &n, 0);
	VmVecScaleAdd (&normal.p3_vec, &pointList [i]->p3_vec, &n, F1_0 * 10);
	G3DrawLine (pointList [i], &normal);
	}
#if 0
VmVecNormal (&normal.p3_vec, 
				 &pointList [0]->p3_vec, 
				 &pointList [1]->p3_vec, 
				 &pointList [2]->p3_vec);
VmVecInc (&normal.p3_vec, &center.p3_vec);
VmVecScale (&normal.p3_vec, F1_0 * 10);
G3DrawLine (&center, &normal);
#endif
glDepthFunc (depthFunc);
}

//------------------------------------------------------------------------------

grs_canvas * reticle_canvas = NULL;

void _CDECL_ FreeReticleCanvas (void)
{
if (reticle_canvas)	{
	LogErr ("unloading reticle data\n");
	D2_FREE( reticle_canvas->cv_bitmap.bm_texBuf);
	D2_FREE( reticle_canvas);
	reticle_canvas	= NULL;
	}
}

extern void ShowReticle(int force_big);

//------------------------------------------------------------------------------

// Draw the reticle in 3D for head tracking
void Draw3DReticle (fix nEyeOffset)
{
	g3sPoint 	reticlePoints [4];
	tUVL			tUVL [4];
	g3sPoint	*pointList [4];
	int 			i;
	vmsVector	v1, v2;
	grs_canvas	*saved_canvas;
	int			saved_interp_method;

//	if (!viewInfo.bUsePlayerHeadAngles) return;
	
for (i = 0; i < 4; i++) {
	reticlePoints [i].p3_index = -1;
	pointList [i] = reticlePoints + i;
	tUVL [i].l = MAX_LIGHT;
	}
tUVL [0].u =
tUVL [0].v =
tUVL [1].v =
tUVL [3].u = 0; 
tUVL [1].u =
tUVL [2].u =
tUVL [2].v =
tUVL [3].v = F1_0;

VmVecScaleAdd( &v1, &gameData.objs.viewer->position.vPos, &gameData.objs.viewer->position.mOrient.fVec, F1_0*4);
VmVecScaleInc(&v1, &gameData.objs.viewer->position.mOrient.rVec, nEyeOffset);

VmVecScaleAdd( &v2, &v1, &gameData.objs.viewer->position.mOrient.rVec, -F1_0*1);
VmVecScaleInc( &v2, &gameData.objs.viewer->position.mOrient.uVec, F1_0*1);
G3TransformAndEncodePoint(reticlePoints, &v2);

VmVecScaleAdd( &v2, &v1, &gameData.objs.viewer->position.mOrient.rVec, +F1_0*1);
VmVecScaleInc( &v2, &gameData.objs.viewer->position.mOrient.uVec, F1_0*1);
G3TransformAndEncodePoint(reticlePoints + 1, &v2);

VmVecScaleAdd( &v2, &v1, &gameData.objs.viewer->position.mOrient.rVec, +F1_0*1);
VmVecScaleInc( &v2, &gameData.objs.viewer->position.mOrient.uVec, -F1_0*1);
G3TransformAndEncodePoint(reticlePoints + 2, &v2);

VmVecScaleAdd( &v2, &v1, &gameData.objs.viewer->position.mOrient.rVec, -F1_0*1);
VmVecScaleInc( &v2, &gameData.objs.viewer->position.mOrient.uVec, -F1_0*1);
G3TransformAndEncodePoint(reticlePoints + 3, &v2);

if ( reticle_canvas == NULL)	{
	reticle_canvas = GrCreateCanvas(64, 64);
	if ( !reticle_canvas)
		Error( "Couldn't D2_ALLOC reticle_canvas");
	atexit( FreeReticleCanvas);
	//reticle_canvas->cv_bitmap.bm_handle = 0;
	reticle_canvas->cv_bitmap.bm_props.flags = BM_FLAG_TRANSPARENT;
	}

saved_canvas = grdCurCanv;
GrSetCurrentCanvas(reticle_canvas);
GrClearCanvas(0);		// Clear to Xparent
ShowReticle(1);
GrSetCurrentCanvas(saved_canvas);

saved_interp_method=gameStates.render.nInterpolationMethod;
gameStates.render.nInterpolationMethod	= 3;		// The best, albiet slowest.
G3DrawTexPoly (4, pointList, tUVL, &reticle_canvas->cv_bitmap, NULL, 1);
gameStates.render.nInterpolationMethod	= saved_interp_method;
}


//------------------------------------------------------------------------------

//cycle the flashing light for when mine destroyed
void FlashFrame()
{
	static fixang flash_ang=0;

if (!gameData.reactor.bDestroyed && !gameStates.gameplay.seismic.nMagnitude)
	return;
if (gameStates.app.bEndLevelSequence)
	return;
if (gameStates.ogl.palAdd.blue > 10)		//whiting out
	return;
//	flash_ang += FixMul(FLASH_CYCLE_RATE, gameData.time.xFrame);
if (gameStates.gameplay.seismic.nMagnitude) {
	fix	added_flash;

	added_flash = abs(gameStates.gameplay.seismic.nMagnitude);
	if (added_flash < F1_0)
		added_flash *= 16;

	flash_ang += (fixang) FixMul (gameStates.render.nFlashRate, FixMul(gameData.time.xFrame, added_flash+F1_0));
	FixFastSinCos (flash_ang, &gameStates.render.nFlashScale, NULL);
	gameStates.render.nFlashScale = (gameStates.render.nFlashScale + F1_0*3)/4;	//	gets in range 0.5 to 1.0
	}
else {
	flash_ang += (fixang) FixMul (gameStates.render.nFlashRate, gameData.time.xFrame);
	FixFastSinCos (flash_ang, &gameStates.render.nFlashScale, NULL);
	gameStates.render.nFlashScale = (gameStates.render.nFlashScale + f1_0)/2;
	if (gameStates.app.nDifficultyLevel == 0)
		gameStates.render.nFlashScale = (gameStates.render.nFlashScale+F1_0*3)/4;
	}
}

// ----------------------------------------------------------------------------
//	Render a face.
//	It would be nice to not have to pass in nSegment and nSide, but
//	they are used for our hideously hacked in headlight system.
//	vp is a pointer to vertex ids.
//	tmap1, tmap2 are texture map ids.  tmap2 is the pasty one.

int RenderColoredSegment (int nSegment, int nSide, int nv, g3sPoint **pointList)
{
	short csegnum = gameData.segs.segments [nSegment].children [nSide];
	int	funcRes = 1;
	int	owner = gameData.segs.xSegments [nSegment].owner;
	int	special = gameData.segs.segment2s [nSegment].special;

gameStates.render.grAlpha = 6;
if ((gameData.app.nGameMode & GM_ENTROPY) && (extraGameInfo [1].entropy.nOverrideTextures == 2) && (owner > 0)) {
	if ((csegnum < 0) || (gameData.segs.xSegments [csegnum].owner != owner)) {
		if (owner == 1)
			G3DrawPolyAlpha (nv, pointList, 0, 0, 0.5, -1);		//draw as flat poly
		else
			G3DrawPolyAlpha (nv, pointList, 0.5, 0, 0, -1);		//draw as flat poly
		}
	}
else if (special == SEGMENT_IS_WATER) {
	if ((csegnum < 0) || (gameData.segs.segment2s [csegnum].special != SEGMENT_IS_WATER))
		G3DrawPolyAlpha(nv, pointList, 0, 1.0 / 16.0, 0.5, -1);		//draw as flat poly
	}
else if (special == SEGMENT_IS_LAVA) {
	if ((csegnum < 0) || (gameData.segs.segment2s [csegnum].special != SEGMENT_IS_LAVA))
		G3DrawPolyAlpha (nv, pointList, 0.5, 0, 0, -1);		//draw as flat poly
	}
else
	funcRes = 0;
gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
return funcRes;
}

//------------------------------------------------------------------------------

typedef struct tFaceProps {
	short			segNum, sideNum;
	short			nBaseTex, nOvlTex, nOvlOrient;
	tUVL			uvls [4];
	short			vp [4];
#if LIGHTMAPS
	tUVL			uvl_lMaps [4];
#endif
	vmsVector	vNormal;
	ubyte			nv;
	ubyte			widFlags;
	char			nType;
} tFaceProps;

typedef struct tFaceListEntry {
	short			nextFace;
	tFaceProps	props;
} tFaceListEntry;

#if APPEND_LAYERED_TEXTURES
static tFaceListEntry	faceList [6 * 2 * MAX_SEGMENTS_D2X];
static short				faceListRoots [MAX_TEXTURES];
static short				faceListTails [MAX_TEXTURES];
static short				nFaceListSize;
#endif

//------------------------------------------------------------------------------
// If any color component > 1, scale all components down so that the greatest == 1.

static inline void ScaleColor (tFaceColor *color, float l)
{
	float m = color->color.red;

if (m < color->color.green)
	m = color->color.green;
if (m < color->color.blue)
	m = color->color.blue;
m = l / m;
color->color.red *= m;
color->color.green *= m;
color->color.blue *= m;
}

//------------------------------------------------------------------------------

int SetVertexColors (tFaceProps *propsP)
{
if (SHOW_DYN_LIGHT) {
	// set material properties specific for certain textures here
	SetDynLightMaterial (propsP->segNum, propsP->sideNum, -1);
	return 0;
	}
if (gameOpts->render.color.bAmbientLight && !USE_LIGHTMAPS) { 
#if VERTEX_LIGHTING
	int i, j = propsP->nv;
	for (i = 0; i < j; i++)
		vertColors [i] = gameData.render.color.vertices [propsP->vp [i]];
	}
else
	memset (vertColors, 0, sizeof (vertColors));
#else
	tFaceColor *colorP = gameData.render.color.sides + nSegment * 6 + nSide;
	if (colorP->index)
		lightColor = *colorP;
	else
		lightColor.color.red =
		lightColor.color.blue =
		lightColor.color.green = 1.0;
		}
else
	lightColor.index = 0;
#endif
return 1;
}

//------------------------------------------------------------------------------

int SetFaceLight (tFaceProps *propsP)
{
	int			nVertex, i;
	tFaceColor	*pvc = vertColors;
	tRgbColorf	*pdc;
	fix			dynLight;
	float			l, dl, hl;

if (SHOW_DYN_LIGHT)
	return 0;
for (i = 0; i < propsP->nv; i++, pvc++) {
	//the tUVL struct has static light already in it
	//scale static light for destruction effect
	if (EGI_FLAG (bDarkness, 0, 0, 0))
		propsP->uvls [i].l = 0;
	else {
#if LMAP_LIGHTADJUST
		if (USE_LIGHTMAPS) {
			else {
				propsP->uvls [i].l = F1_0 / 2 + gameData.render.lights.segDeltas [propsP->segNum * 6 + propsP->sideNum];
				if (propsP->uvls [i].l < 0)
					propsP->uvls [i].l = 0;
				}
			}
#endif
		if (gameData.reactor.bDestroyed || gameStates.gameplay.seismic.nMagnitude)	//make lights flash
			propsP->uvls [i].l = FixMul (gameStates.render.nFlashScale, propsP->uvls [i].l);
		}
	//add in dynamic light (from explosions, etc.)
	dynLight = gameData.render.lights.dynamicLight [nVertex = propsP->vp [i]];
#ifdef _DEBUG
	if (nVertex == nDbgVertex)
		nVertex = nVertex;
#endif
	l = f2fl (propsP->uvls [i].l);
	dl = f2fl (dynLight);
	propsP->uvls [i].l += dynLight;
#if 0
	if (gameData.app.nGameMode & GM_ENTROPY) {
		if (segP->owner == 1) {
			tMapColor.index = 1;
			tMapColor.color.red = 
			tMapColor.color.green = 5.0 / 63.0;
			tMapColor.color.blue = 1.0;
			}
		else if (segP->owner == 3) {
			tMapColor.index = 1;
			tMapColor.color.red = 1.0;
			tMapColor.color.green = 
			tMapColor.color.blue = 5.0 / 63.0;
			}
		}
#endif
	if (gameStates.app.bHaveExtraGameInfo [IsMultiGame] /*&& gameOpts->render.color.bGunLight*/) {
		if (bUseGlobalColor) {
			if (gameData.render.lights.bGotGlobalDynColor) {
				tMapColor.index = 1;
				memcpy (&tMapColor.color, &gameData.render.lights.globalDynColor, sizeof (tRgbColorf));
				}
			}
		else if (gameData.render.lights.bGotDynColor [nVertex]) {
#ifdef _DEBUG //convenient place for a debug breakpoint
			if (propsP->segNum == nDbgSeg && propsP->sideNum == nDbgSide)
				propsP->segNum = propsP->segNum;
#endif
			pdc = gameData.render.lights.dynamicColor + nVertex;
			//pvc->index = -1;
			if (gameOpts->render.color.bMix) {
#if 0
				pvc->color.red = (pvc->color.red + gameData.render.lights.dynamicColor [nVertex].red * gameOpts->render.color.bMix) / (float) (gameOpts->render.color.bMix + 1);
				pvc->color.green = (pvc->color.green + pdc->green * gameOpts->render.color.bMix) / (float) (gameOpts->render.color.bMix + 1);
				pvc->color.blue = (pvc->color.blue + pdc->blue * gameOpts->render.color.bMix) / (float) (gameOpts->render.color.bMix + 1);
#else
				if (gameOpts->render.color.bGunLight) {
					if (/*gameStates.app.bD2XLevel && */
						gameOpts->render.color.bAmbientLight && 
						!gameOpts->render.color.bUseLightMaps && 
						pvc->index) {
						if (l && gameData.render.color.vertBright [nVertex]) {
							hl = l / gameData.render.color.vertBright [nVertex];
							pvc->color.red = pvc->color.red * hl + pdc->red * dl;
							pvc->color.green = pvc->color.green * hl + pdc->green * dl;
							pvc->color.blue = pvc->color.blue * hl + pdc->blue * dl;
							ScaleColor (pvc, l + dl);
							}
						else {
							pvc->color.red = pdc->red * dl;
							pvc->color.green = pdc->green * dl;
							pvc->color.blue = pdc->blue * dl;
							ScaleColor (pvc, dl);
							}
						pvc->index = -1;
						}
					else {
						pvc->color.red = l + pdc->red * dl;
						pvc->color.green = l + pdc->green * dl;
						pvc->color.blue = l + pdc->blue * dl;
						pvc->index = -1;
						}
					}
				else {
					pvc->color.red =
					pvc->color.green =
					pvc->color.blue = l + dl;
					}	
				if (gameOpts->render.color.bCap) {
					if (pvc->color.red > 1.0)
						pvc->color.red = 1.0;
					if (pvc->color.green > 1.0)
						pvc->color.green = 1.0;
					if (pvc->color.blue > 1.0)
						pvc->color.blue = 1.0;
					}
#endif
				}
			else {
				float dl = f2fl (propsP->uvls [i].l);
				dl = (float) pow (dl, 1.0 / 3.0); //sqrt (dl);
				pvc->color.red = pdc->red * dl;
				pvc->color.green = pdc->green * dl;
				pvc->color.blue = pdc->blue * dl;
				}
			}
		else {
			if (pvc->index)
				ScaleColor (pvc, l);
			}
		}
	else {
		if (pvc->index)
			ScaleColor (pvc, l);
		}
	//add in light from tPlayer's headlight
	// -- Using new headlight system...propsP->uvls [i].l += compute_headlight_light(&gameData.segs.points [propsP->vp [i]].p3_vec, face_light);
	//saturate at max value
	if (propsP->uvls [i].l > MAX_LIGHT)
		propsP->uvls [i].l = MAX_LIGHT;
	}
return 1;
}

//------------------------------------------------------------------------------

#if 0

static inline int IsLava (tFaceProps *propsP)
{
	short	nTexture = gameData.segs.segments [propsP->segNum].sides [propsP->sideNum].nBaseTex;

return (nTexture == 378) || ((nTexture >= 404) && (nTexture <= 409));
}

//------------------------------------------------------------------------------

static inline int IsWater (tFaceProps *propsP)
{
	short	nTexture = gameData.segs.segments [propsP->segNum].sides [propsP->sideNum].nBaseTex;

return ((nTexture >= 399) && (nTexture <= 403));
}

//------------------------------------------------------------------------------

static inline int IsWaterOrLava (tFaceProps *propsP)
{
	short	nTexture = gameData.segs.segments [propsP->segNum].sides [propsP->sideNum].nBaseTex;

return (nTexture == 378) || ((nTexture >= 399) && (nTexture <= 409));
}

#endif

//------------------------------------------------------------------------------

int IsTransparentTexture (short nTexture)
{
return !gameStates.app.bD1Mission &&
		 ((nTexture == 378) || 
		  (nTexture == 353) || 
		  (nTexture == 420) || 
		  (nTexture == 432) || 
		  ((nTexture >= 399) && (nTexture <= 409)));
}

//------------------------------------------------------------------------------

static inline int IsTransparentFace (tFaceProps *propsP)
{
return IsTransparentTexture (gameData.segs.segments [propsP->segNum].sides [propsP->sideNum].nBaseTex);
}

//------------------------------------------------------------------------------

int RenderWall (tFaceProps *propsP, g3sPoint **pointList, int bIsMonitor)
{
short c, nWallNum = WallNumI (propsP->segNum, propsP->sideNum);

if (IS_WALL (nWallNum)) {
	if (propsP->widFlags & (WID_CLOAKED_FLAG | WID_TRANSPARENT_FLAG)) {
		if (!bIsMonitor) {
			if (!RenderColoredSegment (propsP->segNum, propsP->sideNum, propsP->nv, pointList)) {
				c = gameData.walls.walls [nWallNum].cloakValue;
				if (propsP->widFlags & WID_CLOAKED_FLAG) {
					if (c < GR_ACTUAL_FADE_LEVELS) {
						gameStates.render.grAlpha = (float) c;
						G3DrawPolyAlpha (propsP->nv, pointList, 0, 0, 0, -1);		//draw as flat poly
						}
					}
				else {
					if (!gameOpts->render.color.bWalls)
						c = 0;
					if (gameData.walls.walls [nWallNum].hps)
						gameStates.render.grAlpha = (float) fabs ((1.0f - (float) gameData.walls.walls [nWallNum].hps / ((float) F1_0 * 100.0f)) * GR_ACTUAL_FADE_LEVELS);
					else if (IsMultiGame && gameStates.app.bHaveExtraGameInfo [1])
						gameStates.render.grAlpha = COMPETITION ? GR_ACTUAL_FADE_LEVELS * 3.0f / 2.0f : (float) (GR_ACTUAL_FADE_LEVELS - extraGameInfo [1].grWallTransparency);
					else
						gameStates.render.grAlpha = (float) (GR_ACTUAL_FADE_LEVELS - extraGameInfo [0].grWallTransparency);
					if (gameStates.render.grAlpha < GR_ACTUAL_FADE_LEVELS)
						G3DrawPolyAlpha (propsP->nv, pointList, CPAL2Tr (gamePalette, c), CPAL2Tg (gamePalette, c), CPAL2Tb (gamePalette, c), -1);	//draw as flat poly
					}
				}
			gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
			return 1;
			}
		}
	else if (gameStates.app.bD2XLevel) {
		c = gameData.walls.walls [nWallNum].cloakValue;
		if (c && (c < GR_ACTUAL_FADE_LEVELS))
			gameStates.render.grAlpha = (float) (GR_ACTUAL_FADE_LEVELS - c);
		}
	else if (gameOpts->render.bAutoTransparency && IsTransparentFace (propsP))
		gameStates.render.grAlpha = (float) GR_ACTUAL_FADE_LEVELS / 10.0f * 8.0f;
	else
		gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
	}
return 0;
}

//------------------------------------------------------------------------------

void RenderFaceShadow (tFaceProps *propsP)
{
	int			i, nv = propsP->nv;
	g3sPoint		*p;
	tOOF_vector	v [9];

for (i = 0; i < nv; i++) {
	p = gameData.segs.points + propsP->vp [i];
	if (p->p3_index < 0)
		OOF_VecVms2Oof (v + i, &p->p3_vec);
	else
		memcpy (v + i, gameData.render.pVerts + p->p3_index, sizeof (tOOF_vector));
	}
v [nv] = v [0];
glEnableClientState (GL_VERTEX_ARRAY);
glVertexPointer (3, GL_FLOAT, 0, v);
#if DBG_SHADOWS
if (bShadowTest) {
	if (bFrontCap)
		glDrawArrays (GL_LINE_LOOP, 0, nv);
	}
else
#endif
glDrawArrays (GL_TRIANGLE_FAN, 0, nv);
#if DBG_SHADOWS
if (!bShadowTest || bShadowVolume)
#endif
for (i = 0; i < nv; i++)
	G3RenderShadowVolumeFace (v + i);
glDisableClientState (GL_VERTEX_ARRAY);
#if DBG_SHADOWS
if (!bShadowTest || bRearCap)
#endif
G3RenderFarShadowCapFace (v, nv);
}

//------------------------------------------------------------------------------

grsBitmap *LoadFaceBitmap (short tMapNum, short nFrameNum);

#if LIGHTMAPS
#	define LMAP_LIGHTADJUST	1
#else
#	define LMAP_LIGHTADJUST	0
#endif

void RenderFace (tFaceProps *propsP, int offs, int bRender)
{
	tFaceProps	props;

if (propsP->nBaseTex < 0)
	return;
if (gameStates.render.nShadowPass == 2) {
#if DBG_SHADOWS
	if (!bWallShadows)
		return;
#endif
	G3SetCullAndStencil (0, 0);
	RenderFaceShadow (propsP);
	G3SetCullAndStencil (1, 0);
	RenderFaceShadow (propsP);
	return;
	}
#if 1
props = *propsP;
memcpy (props.uvls, propsP->uvls + offs, props.nv * sizeof (*props.uvls));
memcpy (props.vp, propsP->vp + offs, props.nv * sizeof (*props.vp));
#else
props.segNum = propsP->segNum;
props.sideNum = propsP->sideNum;
props.nBaseTex = propsP->nBaseTex;
props.nOvlTex = propsP->nOvlTex;
props.nOvlOrient = propsP->nOvlOrient;
props.nv = propsP->nv;
memcpy (props.uvls, propsP->uvls + offs, props.nv * sizeof (*props.uvls));
memcpy (props.vp, propsP->vp + offs, props.nv * sizeof (*props.vp));
#if LIGHTMAPS
memcpy (props.uvl_lMaps, propsP->uvl_lMaps + offs, props.nv * sizeof (*props.uvl_lMaps));
#endif
memcpy (&props.vNormal, &propsP->vNormal, sizeof (props.vNormal));
props.widFlags = propsP->widFlags;
#endif
#ifdef _DEBUG //convenient place for a debug breakpoint
if (props.segNum == nDbgSeg && props.sideNum == nDbgSide)
	props.segNum = props.segNum;
#	if 0
else
	return;
#	endif
#endif

gameData.render.vertexList = gameData.segs.fVertices;
#if APPEND_LAYERED_TEXTURES
if (!gameOpts->render.bOptimize || bRender) 
#else
if (!bRender) 
#endif
{
	// -- Using new headlight system...fix			face_light;
	grsBitmap  *bmBot = NULL;
	grsBitmap  *bmTop = NULL;

	int			i, bIsMonitor, bIsTeleCam, bHaveCamImg, nCamNum, bCamBufAvail;
	g3sPoint		*pointList [8];
	tSegment		*segP = gameData.segs.segments + props.segNum;
	tSide			*sideP = segP->sides + props.sideNum;
	tCamera		*pc = NULL;

	Assert(props.nv <= 4);
	for (i = 0; i < props.nv; i++)
		pointList [i] = gameData.segs.points + props.vp [i];
#if OGL_QUERY
	if (gameStates.render.bQueryOcclusion) {
		DrawOutline (props.nv, pointList);
		gameData.render.vertexList = NULL;
		return;
		}
#endif
	if (!(gameOpts->render.bTextures || IsMultiGame))
		goto drawWireFrame;
#if 1
	if (gameStates.render.nShadowBlurPass == 1) {
		G3DrawWhitePoly (props.nv, pointList);
		gameData.render.vertexList = NULL;
		return;
		}
#endif
	SetVertexColors (&props);
	if (gameStates.render.nType == 2) {
		RenderColoredSegment (props.segNum, props.sideNum, props.nv, pointList);
		gameData.render.vertexList = NULL;
		return;
		}
	nCamNum = gameData.cameras.nSides  ? gameData.cameras.nSides [props.segNum * 6 + props.sideNum] : -1;
	bIsTeleCam = 0;
	bIsMonitor = extraGameInfo [0].bUseCameras && 
					 (!IsMultiGame || (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bUseCameras)) && 
					 !gameStates.render.cameras.bActive && (nCamNum >= 0);
	if (bIsMonitor) {
		pc = gameData.cameras.cameras + nCamNum;
		bIsTeleCam = pc->bTeleport;
#if RENDER2TEXTURE
		bCamBufAvail = OglCamBufAvail (pc, 1) == 1;
#else
		bCamBufAvail = 0;
#endif
		bHaveCamImg = pc->bValid && /*!pc->bShadowMap &&*/ 
						  (pc->texBuf.glTexture || bCamBufAvail) &&
						  (!bIsTeleCam || EGI_FLAG (bTeleporterCams, 0, 1, 0));
		}
	else {
		bHaveCamImg = 0;
		bCamBufAvail = 0;
		}
	//handle cloaked walls
	if (bIsMonitor)
		pc->bVisible = 1;
	if (RenderWall (&props, pointList, bIsMonitor)) {
		gameData.render.vertexList = NULL;
		return;
		}
	// -- Using new headlight system...face_light = -VmVecDot(&gameData.objs.viewer->position.mOrient.fVec, norm);
	if (props.widFlags & WID_RENDER_FLAG) {		//if (WALL_IS_DOORWAY(segP, nSide) == WID_NO_WALL)
		if (props.nBaseTex >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]) {
#if TRACE	
			//con_printf (CONDBG, "Invalid tmap number %d, gameData.pig.tex.nTextures=%d, changing to 0\n", tmap1, gameData.pig.tex.nTextures);
#endif
#ifdef _DEBUG
			Int3();
#endif
		sideP->nBaseTex = 0;
		}
	if (!(bHaveCamImg && gameOpts->render.cameras.bFitToWall)) {
		if (gameStates.render.nType == 3) {
			bmBot = bmpCorona;
			bmTop = NULL;
			props.uvls [0].u =
			props.uvls [0].v =
			props.uvls [1].v =
			props.uvls [3].u = F1_0 / 4;
			props.uvls [1].u =
			props.uvls [2].u =
			props.uvls [2].v =
			props.uvls [3].v = F1_0 / 4 * 3;
			}
		else if (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk) {
			bmBot = LoadFaceBitmap (props.nBaseTex, sideP->nFrame);
			if (props.nOvlTex)
				bmTop = LoadFaceBitmap ((short) (props.nOvlTex), sideP->nFrame);
			}
		else
			// New code for overlapping textures...
			if (props.nOvlTex != 0) {
				bmBot = TexMergeGetCachedBitmap (props.nBaseTex, props.nOvlTex, props.nOvlOrient);
#ifdef _DEBUG
				if (!bmBot)
					bmBot = TexMergeGetCachedBitmap (props.nBaseTex, props.nOvlTex, props.nOvlOrient);
#endif
				}
			else {
				bmBot = gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [props.nBaseTex].index;
				PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [props.nBaseTex], gameStates.app.bD1Mission);
				}
//		Assert(!(bmBot->bm_props.flags & BM_FLAG_PAGED_OUT));
		}
	//else 
	if (bHaveCamImg) {
		GetCameraUVL (pc, props.uvls);
		pc->texBuf.glTexture->wrapstate = -1;
		if (bIsTeleCam) {
#ifdef _DEBUG
			bmBot = &pc->texBuf;
			gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
#else
			bmTop = &pc->texBuf;
			gameStates.render.grAlpha = (GR_ACTUAL_FADE_LEVELS * 7) / 10;
#endif
			}
		else if (gameOpts->render.cameras.bFitToWall || (props.nOvlTex > 0))
			bmBot = &pc->texBuf;
		else
			bmTop = &pc->texBuf;
		}
	SetFaceLight (&props);
#ifdef EDITOR
	if (Render_only_bottom && (nSide == WBOTTOM))
		G3DrawTexPoly (props.nv, pointList, props.uvls, gameData.pig.tex.bitmaps + gameData.pig.tex.bmIndex [Bottom_bitmap_num].index, 1);
	else
#endif
#ifdef _DEBUG //convenient place for a debug breakpoint
if (props.segNum == nDbgSeg && props.sideNum == nDbgSide)
	props.segNum = props.segNum;
#endif
	if (bmTop)
		fpDrawTexPolyMulti (
			props.nv, 
			pointList, 
			props.uvls, 
#if LIGHTMAPS
			props.uvl_lMaps, 
#endif
			bmBot, bmTop, 
#if LIGHTMAPS
			lightMaps + props.segNum * 6 + props.sideNum, 
#endif
			&props.vNormal, 
			props.nOvlOrient, 
			!bIsMonitor || bIsTeleCam); 
	else
#if LIGHTMAPS == 0
		G3DrawTexPoly (
			props.nv, 
			pointList, 
			props.uvls, 
			bmBot, 
			&props.vNormal, 
			!bIsMonitor || bIsTeleCam); 
#else
		fpDrawTexPolyMulti (
			props.nv, 
			pointList, 
			props.uvls, 
			props.uvl_lMaps, 
			bmBot, 
			NULL, 
			lightMaps + props.segNum * 6 + props.sideNum, 
			&props.vNormal, 
			0, 
			!bIsMonitor || bIsTeleCam); //(bIsMonitor && !gameOpts->render.cameras.bFitToWall) || (bmBot->bm_props.flags & BM_FLAG_TGA));
#endif
		}
gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
		// render the tSegment the tPlayer is in with a transparent color if it is a water or lava tSegment
		//if (nSegment == gameData.objs.objects->nSegment) 
#ifdef _DEBUG
	if (bOutLineMode) 
		DrawOutline (props.nv, pointList);
#endif
drawWireFrame:
	if (gameOpts->render.bWireFrame && !IsMultiGame)
		DrawOutline (props.nv, pointList);
	}
else {
#if APPEND_LAYERED_TEXTURES
	tFaceListEntry	*flp = faceList + nFaceListSize;
	short				t = props.nBaseTex;
	if (!props.nOvlTex || (faceListRoots [props.nBaseTex] < 0)) {
		short	t = props.nOvlTex ? props.nOvlTex : props.nBaseTex;
		flp->nextFace = faceListRoots [t];
		if (faceListTails [t] < 0)
			faceListTails [t] = nFaceListSize;
		faceListRoots [t] = nFaceListSize++;
		}
	else {
		tFaceListEntry	*flh = faceList + faceListTails [t];
		flh->nextFace = nFaceListSize;
		flp->nextFace = -1;
		faceListTails [t] = nFaceListSize++;
		}
#endif
	props.nType = (char) gameStates.render.nType;
#if APPEND_LAYERED_TEXTURES
	flp->props = props;
#endif
	}
}

#ifdef EDITOR
// ----------------------------------------------------------------------------
//	Only called if editor active.
//	Used to determine which face was clicked on.
void CheckFace(int nSegment, int nSide, int facenum, int nv, short *vp, int tmap1, int tmap2, tUVL *uvlp)
{
	int	i;

	if (bSearchMode) {
		int save_lighting;
		grsBitmap *bm;
		tUVL uvlCopy [8];
		g3sPoint *pointList [4];

		if (tmap2 > 0)
			bm = TexMergeGetCachedBitmap( tmap1, tmap2, nOrient);
		else
			bm = gameData.pig.tex.bitmaps + gameData.pig.tex.bmIndex [tmap1].index;

		for (i=0; i<nv; i++) {
			uvlCopy [i] = uvlp [i];
			pointList [i] = gameData.segs.points + vp [i];
		}

		GrSetColor(0);
		gr_pixel(_search_x, _search_y);	//set our search pixel to color zero
		GrSetColor(1);					//and render in color one
 save_lighting = gameStates.render.nLighting;
 gameStates.render.nLighting = 2;
		//G3DrawPoly(nv, vp);
		G3DrawTexPoly (nv, pointList, (tUVL *)uvlCopy, bm, 1);
 gameStates.render.nLighting = save_lighting;

		if (gr_ugpixel(&grdCurCanv->cv_bitmap, _search_x, _search_y) == 1) {
			found_seg = nSegment;
			found_side = nSide;
			found_face = facenum;
		}
	}
}
#endif

fix	Tulate_min_dot = (F1_0/4);
//--unused-- fix	Tulate_min_ratio = (2*F1_0);
fix	Min_n0_n1_dot	= (F1_0*15/16);

extern int containsFlare(tSegment *segP, int nSide);
extern fix	Obj_light_xlate [16];

// -----------------------------------------------------------------------------------

void CalcSpriteCoords (fVector *vSprite, fVector *vCenter, fVector *vEye, float dx, float dy, fMatrix *r)
{
	fVector	v, h, vdx, vdy;
	float		d = vCenter->p.x * vCenter->p.x + vCenter->p.y * vCenter->p.y + vCenter->p.z * vCenter->p.z;
	int		i;

if (!vEye) {
	vEye = &h;
	VmVecNormalizef (vEye, vCenter);
	}
v.p.x = v.p.z = 0;
v.p.y = vCenter->p.y ? d / vCenter->p.y : 1;
VmVecDecf (&v, vCenter);
VmVecNormalizef (&v, &v);
VmVecCrossProdf (&vdx, &v, vEye);	//orthogonal vector in plane through face center and perpendicular to viewer
VmVecScalef (&vdx, &vdx, dx);
v.p.y = v.p.z = 0;
v.p.x = vCenter->p.x ? d / vCenter->p.x : 1;
VmVecDecf (&v, vCenter);
VmVecNormalizef (&v, &v);
VmVecCrossProdf (&vdy, &v, vEye);
if (r) {
	if (vCenter->p.x >= 0) {
		VmVecScalef (&vdy, &vdy, dy);
		v.p.x = +vdx.p.x + vdy.p.x;
		v.p.y = +vdx.p.y + vdy.p.y;
		v.p.z = -vdx.p.z - vdy.p.z;
		VmVecRotatef (vSprite, &v, r);
		VmVecIncf (vSprite, vCenter);
		v.p.x = v.p.y = v.p.z = 0;
		v.p.x = -vdx.p.x + vdy.p.x;
		v.p.y = +vdx.p.y + vdy.p.y;
		v.p.z = +vdx.p.z - vdy.p.z;
		VmVecRotatef (vSprite + 1, &v, r);
		VmVecIncf (vSprite + 1, vCenter);
		v.p.x = v.p.y = v.p.z = 0;
		v.p.x = -vdx.p.x - vdy.p.x;
		v.p.y = -vdx.p.y - vdy.p.y;
		v.p.z = +vdx.p.z + vdy.p.z;
		VmVecRotatef (vSprite + 2, &v, r);
		VmVecIncf (vSprite + 2, vCenter);
		v.p.x = +vdx.p.x - vdy.p.x;
		v.p.y = -vdx.p.y - vdy.p.y;
		v.p.z = -vdx.p.z + vdy.p.z;
		VmVecRotatef (vSprite + 3, &v, r);
		VmVecIncf (vSprite + 3, vCenter);
		}
	else {
		VmVecScalef (&vdy, &vdy, dy);
		v.p.x = -vdx.p.x - vdy.p.x;
		v.p.y = -vdx.p.y - vdy.p.y;
		v.p.z = -vdx.p.z - vdy.p.z;
		VmVecRotatef (vSprite, &v, r);
		VmVecIncf (vSprite, vCenter);
		v.p.x = v.p.y = v.p.z = 0;
		v.p.x = +vdx.p.x - vdy.p.x;
		v.p.y = -vdx.p.y - vdy.p.y;
		v.p.z = +vdx.p.z - vdy.p.z;
		VmVecRotatef (vSprite + 1, &v, r);
		VmVecIncf (vSprite + 1, vCenter);
		v.p.x = v.p.y = v.p.z = 0;
		v.p.x = +vdx.p.x + vdy.p.x;
		v.p.y = +vdx.p.y + vdy.p.y;
		v.p.z = +vdx.p.z + vdy.p.z;
		VmVecRotatef (vSprite + 2, &v, r);
		VmVecIncf (vSprite + 2, vCenter);
		v.p.x = -vdx.p.x + vdy.p.x;
		v.p.y = +vdx.p.y + vdy.p.y;
		v.p.z = -vdx.p.z + vdy.p.z;
		VmVecRotatef (vSprite + 3, &v, r);
		VmVecIncf (vSprite + 3, vCenter);
		}
	}
else {
	vSprite [0].p.x = -vdx.p.x - vdy.p.x;
	vSprite [0].p.y = -vdx.p.y - vdy.p.y;
	vSprite [0].p.z = -vdx.p.z - vdy.p.z;
	vSprite [1].p.x = +vdx.p.x - vdy.p.x;
	vSprite [1].p.y = +vdx.p.y - vdy.p.y;
	vSprite [1].p.z = +vdx.p.z - vdy.p.z;
	vSprite [2].p.x = +vdx.p.x + vdy.p.x;
	vSprite [2].p.y = +vdx.p.y + vdy.p.y;
	vSprite [2].p.z = +vdx.p.z + vdy.p.z;
	vSprite [3].p.x = -vdx.p.x + vdy.p.x;
	vSprite [3].p.y = -vdx.p.y + vdy.p.y;
	vSprite [3].p.z = -vdx.p.z + vdy.p.z;
	for (i = 0; i < 4; i++)
		VmVecIncf (vSprite + i, vCenter);
	}
}

// -----------------------------------------------------------------------------------

int radarRanges [] = {100, 150, 200};

#define RADAR_RANGE	radarRanges [gameOpts->render.automap.nRange]
#define RADAR_SLICES	40
#define BLIP_SLICES	40

static vmsAngVec	aRadar = {F1_0 / 4, 0, 0};
static vmsMatrix	mRadar;
static double		yRadar = 20;

void RenderRadarBlip (tObject *objP, double r, double g, double b, double a)
{
	vmsVector	n, v [2];
	fix			m;
	double		h, s;

	static tSinCosd sinCosRadar [RADAR_SLICES];
	static tSinCosd sinCosBlip [BLIP_SLICES];
	static int bInitSinCos = 1;
	
if (bInitSinCos) {
	OglComputeSinCos (sizeofa (sinCosRadar), sinCosRadar);
	OglComputeSinCos (sizeofa (sinCosBlip), sinCosBlip);
	bInitSinCos = 0;
	}
n = objP->position.vPos;
G3TransformPoint (&n, &n, 0);
if ((m = VmVecMag (&n)) > RADAR_RANGE * F1_0)
	return;
if (m) {
	//HUDMessage (0, "%1.2f", f2fl (m));
	v [0].p.x = FixDiv (n.p.x, m) * 15; // /= RADAR_RANGE;
	v [0].p.y = FixDiv (n.p.y, m) * 20; // /= RADAR_RANGE;
	v [0].p.z = n.p.x / RADAR_RANGE;
	//VmVecNormalize (&n);
	}
else {
	glPushMatrix ();
	glColor4d (r, g, b, a);
	glLineWidth (1);
	glTranslated (0, yRadar, 50);
#if 0
	glColor4d (r, g, b, a / 2);
 	OglDrawEllipse (RADAR_SLICES, GL_POLYGON, 10, 0, 7.5, 0, sinCosRadar);
#endif
	glColor4d (r, g, b, a);
 	OglDrawEllipse (RADAR_SLICES, GL_POLYGON, 10, 0, 10.0 / 3.0, 0, sinCosRadar);
	glColor4d (0.5, 0.5, 0.5, 0.8);
	glEnable (GL_LINE_SMOOTH);
 	OglDrawEllipse (RADAR_SLICES, GL_LINE_LOOP, 10, 0, 10.0 / 3.0, 0, sinCosRadar);
 	OglDrawEllipse (RADAR_SLICES, GL_LINE_LOOP, 20.0 / 3.0, 0, 20.0 / 9.0, 0, sinCosRadar);
 	OglDrawEllipse (RADAR_SLICES, GL_LINE_LOOP, 10.0 / 3.0, 0, 10.0 / 9.0, 0, sinCosRadar);
	glBegin (GL_LINES);
	glVertex2d (0, 10.0 / 3.0);
	glVertex2d (0, -10.0 / 3.0);
	glVertex2d (10, 0);
	glVertex2d (-10, 0);
	glEnd ();
	glDisable (GL_LINE_SMOOTH);
	glLineWidth (2);
	glPopMatrix ();
	return;
	}
VmVecScaleFrac (v, 1, 3);
h = f2fl (n.p.z) / RADAR_RANGE;
glPushMatrix ();
glTranslated (0, yRadar + h * 10.0 / 3.0, 50);
glPushMatrix ();
s = 1.0 - fabs ((double) f2fl (m) / RADAR_RANGE);
h = 3 * s;
a += a * h;
glColor4d (r + r * h, g + g * h, b + b * h, sqrt (a));
glTranslatef (f2fl (v [0].p.x), f2fl (v [0].p.y), f2fl (v [0].p.z));
OglDrawEllipse (BLIP_SLICES, GL_POLYGON, 0.33 + 0.33 * s, 0, 0.33 + 0.33 * s, 0, sinCosBlip);
glPopMatrix ();
#if 1
v [1] = v [0];
v [1].p.y = 0;
glBegin (GL_LINES);
OglVertex3x (v [0].p.x, v [0].p.y, v [0].p.z);
OglVertex3x (v [1].p.x, v [1].p.y, v [1].p.z);
glEnd ();
#endif
glPopMatrix ();
}

// -----------------------------------------------------------------------------------

static tRgbColord shipColors [8];
static tRgbColord guidebotColor = {0, 0.75 / 4, 0.25};
static tRgbColord robotColor = {0.75 / 4, 0, 0.25};
static tRgbColord powerupColor = {0.25, 0.5 / 4, 0};
static tRgbColord radarColor [2] = {{1, 1, 1}, {0, 0, 0}};
static int bHaveShipColors = 0;

void InitShipColors (void)
{
if (!bHaveShipColors) {
	int	i;

	for (i = 0; i < 8; i++) {
		shipColors [i].red = 2 * playerColors [i].r / 255.0;
		shipColors [i].green = 2 * playerColors [i].g / 255.0;
		shipColors [i].blue = 2 * playerColors [i].b / 255.0;
		}
	bHaveShipColors = 1;
	}
}

// -----------------------------------------------------------------------------------

void RenderRadar (void)
{
	int			i;
	tObject		*objP;
	GLint			depthFunc;
	tRgbColord	*pc;

if (HIDE_HUD)
	return;
if (gameStates.render.automap.bDisplay)
	return;
if (!(i = EGI_FLAG (nRadar, 0, 1, 0)))
	return;
InitShipColors ();
yRadar = (i == 1) ? 20 : -20;
VmAngles2Matrix (&mRadar, &aRadar);
glDisable (GL_CULL_FACE);		
glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
glDepthFunc (GL_ALWAYS);
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDisable (GL_TEXTURE_2D);
glLineWidth (3);
pc = radarColor + gameOpts->render.automap.nColor;
RenderRadarBlip (gameData.objs.console, pc->red, pc->green, pc->blue, 2.0 / 3.0); //0.5, 0.75, 0.5, 2.0 / 3.0);
for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject; i++, objP++) {
	if ((objP->nType == OBJ_PLAYER) && (objP != gameData.objs.console)) {
		if (AM_SHOW_PLAYERS && AM_SHOW_PLAYER (objP->id)) {
			pc = shipColors + (IsTeamGame ? GetTeam (objP->id) : objP->id);
			RenderRadarBlip (objP, pc->red, pc->green, pc->blue, 0.9 / 4);
			}
		}
	else if (objP->nType == OBJ_ROBOT) {
		if (AM_SHOW_ROBOTS)
			if (ROBOTINFO (objP->id).companion)
				RenderRadarBlip (objP, guidebotColor.red, guidebotColor.green, guidebotColor.blue, 0.9 / 4);
			else
				RenderRadarBlip (objP, robotColor.red, robotColor.green, robotColor.blue, 0.9 / 4);
		}
	else if (objP->nType == OBJ_POWERUP) {
		if (AM_SHOW_POWERUPS (2))
			RenderRadarBlip (objP, powerupColor.red, powerupColor.green, powerupColor.blue, 0.9 / 4);
		}
	}
glLineWidth (1);
glDepthFunc (depthFunc);
glEnable (GL_CULL_FACE);		
}

// -----------------------------------------------------------------------------------

int CalcFaceDimensions (short nSegment, short nSide, fix *w, fix *h, short *pSideVerts)
{
	short			sideVerts [4];
	fix			d, d1, d2, dMax = 0;
	int			i, j;

if (!pSideVerts) {
	GetSideVerts (sideVerts, nSegment, nSide);
	pSideVerts = sideVerts;
	}
for (j = 0; j < 4; j++) {
	d = VmVecDist (gameData.segs.vertices + pSideVerts [j], gameData.segs.vertices + pSideVerts [(j + 1) % 4]);
	if (dMax < d) {
		dMax = d;
		i = j;
		}
	}
if (w)
	*w = dMax;
if (i > 2)
	i--;
j = i + 1;
d1 = VmLinePointDist (gameData.segs.vertices + pSideVerts [i], 
							 gameData.segs.vertices + pSideVerts [j], 
							 gameData.segs.vertices + pSideVerts [(j + 1) % 4]);
d = VmVecDist (gameData.segs.vertices + pSideVerts [i], gameData.segs.vertices + pSideVerts [(j + 1) % 4]);
d = VmVecDist (gameData.segs.vertices + pSideVerts [j], gameData.segs.vertices + pSideVerts [(j + 1) % 4]);
d2 = VmLinePointDist (gameData.segs.vertices + pSideVerts [i], 
							 gameData.segs.vertices + pSideVerts [j], 
							 gameData.segs.vertices + pSideVerts [(j + 2) % 4]);
d = VmVecDist (gameData.segs.vertices + pSideVerts [i], gameData.segs.vertices + pSideVerts [(j + 2) % 4]);
d = VmVecDist (gameData.segs.vertices + pSideVerts [j], gameData.segs.vertices + pSideVerts [(j + 2) % 4]);
if (h)
	*h = d1 > d2 ? d1 : d2;
return i;
}

// -----------------------------------------------------------------------------------
// The following code takes a face and renders a corona over it.
// The corona is rendered as a billboard, i.e. always facing the viewer.
// To do that, the center point of the corona's face is computed. Next, orthogonal
// vectors from the vector (eye,center) pointing up and left are computed. Finally,
// vectors orthogonal to the (eye,up) and (eye,left) vectors are computed. These
// all lie in a plane orthogonal to the viewer eye are used to compute the billboard's 
// coordinates.
// To avoid the corona partially disappearing in the face it is rendered upon, it is 
// moved forward to the foremost z coordinate of that face.
// The corona's x and y dimensions are adjusted to the face's dimensions. To properly
// do that, the face coordinates are rotated so that the face normal has a 90 degree 
// angle towards the vertical (points up/down). Now the actual (unrotated) x and y 
// dimensions of the face are determined.
// The rotation matrix is determined by the face normal; the z axis is disregarded
// during rotation (rotation around z axis).
// With these x and y values and the orthogonal vectors the corona coordinates are computed.
// To make it match with the actual orientation of its face, it is rotated with another
// rotation matrix derived from the face's normal.

#define ROTATE_CORONA	0

float coronaIntensities [] = {0.5f, 0.75f, 1};

void RenderCorona (short nSegment, short nSide)
{
	fVector		vertList [4], sprite [4];
	short			sideVerts [4];
	ushort		nWall;
	tUVLf			uvlList [4] = {{{0,0,1}},{{1,0,1}},{{1,1,1}},{{0,1,1}}};
	fVector		d, n, o, v, vCenter = {{0,0,0}}, vEye;
#if ROTATE_CORONA
	fVector		vDeltaX, vDeltaY;
#endif
	fMatrix		r;
	int			i, j, t;
	float			zMin = 1000000000.0f, zMax = -1000000000.0f;
	float			a, h, m = 0, l = 0, dx = 0, dy = 0, dim = coronaIntensities [gameOpts->render.nCoronaIntensity];
	tFaceColor	*pf;
	tSide			*sideP = gameData.segs.segments [nSegment].sides + nSide;

#if 0//def _DEBUG
if (nSegment != 22)
	return;
#	if 0
if  (nSide != 0)
	return;
#	endif
#endif
if (gameOpts->render.bDynamicLight) {
	i = FindDynLight (nSegment, nSide, -1);
	if ((i >= 0) && !gameData.render.lights.dynamic.lights [i].bOn)
		return;
	}
nWall = gameData.segs.segments [nSegment].sides [nSide].nWall;
if (IS_WALL (nWall)) {
	tWall *wallP = gameData.walls.walls + nWall;
	ubyte nType = wallP->nType;

	if ((nType == WALL_BLASTABLE) || 
		 (nType == WALL_DOOR) ||
		 (nType == WALL_OPEN) ||
		 (nType == WALL_CLOAKED))
		return;
	if (wallP->flags & (WALL_BLASTED | WALL_ILLUSION_OFF))
		return;
	}
// get and check the corona emitting texture
#if 0
t = sideP->nOvlTex;
#else
if (sideP->nOvlTex && IsLight (sideP->nOvlTex))
	t = sideP->nOvlTex;
else {
	t = sideP->nBaseTex;
	dim /= 2;
	}
#endif
if (gameStates.app.bD1Mission) {
	switch (t) {
		case 289:	//empty light
		case 328:	//energy sparks
		case 334:	//reactor
		case 335:
		case 336:
		case 337:
		case 338:	//robot generators
		case 339:
			return;
		default:
			break;
		}
	}
else {
	switch (t) {
		case 302:	//empty light
		case 348:	//sliding walls
		case 349:
		case 353:	//energy sparks
		case 356:	//reactor
		case 357:
		case 358:
		case 359:
		case 360:	//robot generators
		case 361:
		case 420:	//force field
		case 426:	//teleport
		case 432:	//force field
		case 433:	//goals
		case 434:
			return;
		case 404:
		case 405:
		case 406:
		case 407:
		case 408:
		case 409:
			dim *= 2;
			break;
		default:
			break;
		}
	}

GetSideVerts (sideVerts, nSegment, nSide);
// get the transformed face coordinates and compute their center
for (i = 0; i < 4; i++) {
	vertList [i] = gameData.segs.fVertices [sideVerts [i]];	//already transformed
	VmVecIncf (&vCenter, vertList + i);
	l += f2fl (sideP->uvls [i].l);
	}
VmVecScalef (&vCenter, &vCenter, 0.25);
VmVecNormalf (&n, vertList, vertList + 1, vertList + 2);
VmVecNormalizef (&vEye, &vCenter);
if ((a = VmVecDotf (&n, &vEye)) > 0) {
	if (a > 0.25f)
		return;
	dim = 1 - a / 0.25f;
	}
#if 0
else	//brighten if facing directly
	dim = 1.0f + 0.5f * -a / 1.0f;
#endif
// o serves to slightly displace the corona from its face to avoid z fighting
VmVecScalef (&o, &n, 1.0f / 10.0f);
VmVecIncf (&n, &vEye);
#if 1	//might remove z from normal 
n.p.z = 0;
#endif
VmVecNormalizef (&n, &n);
// compute rotation matrix to align transformed face
//n.p.x = 1 - n.p.y;
#if 0
i = CalcFaceDimensions (nSegment, nSide, NULL, NULL, sideVerts);
m = VmVecDistf (vertList + i, vertList + i + 1);
n.p.x = (vertList [i].p.y - vertList [i + 1].p.y) / m;
n.p.y = (vertList [i].p.x - vertList [i + 1].p.x) / m;
#endif
r.rVec.p.x =
r.uVec.p.y = n.p.y;
r.uVec.p.x = n.p.x;
r.rVec.p.y = -n.p.x;
r.rVec.p.z =
r.uVec.p.z =
r.fVec.p.x = 
r.fVec.p.y = 0;
r.fVec.p.z = 1;
for (i = 0; i < 4; i++) {
	VmVecSubf (&d, vertList + i, &vCenter);	//compute face coordinate relative to pivot
	VmVecRotatef (&v, &d, &r);	//align face coordinate
	sprite [i] = v;
	VmVecIncf (vertList + i, &d);
	VmVecIncf (vertList + i, &o);
	//determine depth extension of the face
	v = vertList [i];
	if (zMin > v.p.z)
		zMin = v.p.z;
	if (zMax < v.p.z)
		zMax = v.p.z;
	m += VmVecMagf (&d);	//accumulate face dimensions
	}

m /= 4;	//compute average face dimension
// compute x and y dimensions of aligned face
for (i = 0; i < 4; i++) {
	j = (i + 1) % 4;
	h = (float) fabs (sprite [i].p.x - sprite [j].p.x);
	if (h > dx)
		dx = h;
	h = (float) fabs (sprite [i].p.y - sprite [j].p.y);
	if (h > dy)
		dy = h;
	}
m = (float) sqrt (dx * dx + dy * dy) / 4;	//basic corona size to make it visible regardless of dx and dy

a = VmVecMagf (&vCenter);
// determine whether viewer has passed foremost z coordinate of corona's face
// if so, push corona back
h = (zMax - zMin) / 2;
if (a < h)
	return;
if (a < 2 * h) {
	dim *= (a - h) / h;
	if (a < h)
		h = a * 0.9f;
	}
if (h) {
	VmVecScalef (&v, &vEye, h);
	VmVecDecf (&vCenter, &v);
	a -= h;
	}
if (m > a)
	m = a;

//create rotation matrix to match corona with face
r.rVec.p.x =
r.uVec.p.y = n.p.x;
r.rVec.p.y = -n.p.y;
r.uVec.p.x = n.p.y;
r.rVec.p.z =
r.uVec.p.z = 0;

#if !ROTATE_CORONA
CalcSpriteCoords (sprite, &vCenter, &vEye, m + dy / 2, m + dx / 2, &r);
#else
// compute orthogonal vectors for calculation of billboard coordinates
h = vCenter.p.x * vCenter.p.x + vCenter.p.y * vCenter.p.y + vCenter.p.z * vCenter.p.z;
v.p.x = v.p.z = 0;
v.p.y = vCenter.p.y ? h / vCenter.p.y : 1;
VmVecDecf (&v, &vCenter);
VmVecNormalizef (&v, &v);
VmVecCrossProdf (&vDeltaX, &v, &vEye);	//orthogonal vector in plane through face center and perpendicular to viewer
VmVecScalef (&vDeltaX, &vDeltaX, dx / 2);
v.p.y = v.p.z = 0;
v.p.x = vCenter.p.x ? h / vCenter.p.x : 1;
VmVecDecf (&v, &vCenter);
VmVecNormalizef (&v, &v);
VmVecCrossProdf (&vDeltaY, &v, &vEye);
VmVecScalef (&vDeltaY, &vDeltaY, dy / 2);

//compute corona coordinates
v.p.x = -vDeltaX.p.x - vDeltaY.p.x;
v.p.y = +vDeltaX.p.y + vDeltaY.p.y;
v.p.z = -vDeltaX.p.z - vDeltaY.p.z;
#if 0
VmVecRotatef (sprite, &v, &r);
VmVecIncf (sprite, &vCenter);
#else
VmVecAddf (sprite, &v, &vCenter);
#endif
v.p.x = v.p.y = v.p.z = 0;
v.p.x = +vDeltaX.p.x - vDeltaY.p.x;
v.p.y = +vDeltaX.p.y + vDeltaY.p.y;
v.p.z = +vDeltaX.p.z - vDeltaY.p.z;
#if 0
VmVecRotatef (sprite + 1, &v, &r);
VmVecIncf (sprite + 1, &vCenter);
#else
VmVecAddf (sprite + 1, &v, &vCenter);
#endif
v.p.x = v.p.y = v.p.z = 0;
v.p.x = +vDeltaX.p.x + vDeltaY.p.x;
v.p.y = -vDeltaX.p.y - vDeltaY.p.y;
v.p.z = +vDeltaX.p.z + vDeltaY.p.z;
#if 0
VmVecRotatef (sprite + 2, &v, &r);
VmVecIncf (sprite + 2, &vCenter);
#else
VmVecAddf (sprite + 2, &v, &vCenter);
#endif
v.p.x = -vDeltaX.p.x + vDeltaY.p.x;
v.p.y = -vDeltaX.p.y - vDeltaY.p.y;
v.p.z = -vDeltaX.p.z + vDeltaY.p.z;
#if 0
VmVecRotatef (sprite + 3, &v, &r);
VmVecIncf (sprite + 3, &vCenter);
#else
VmVecAddf (sprite + 3, &v, &vCenter);
#endif
#endif

VmVecNormalf (&o, sprite, sprite + 1, sprite + 2);
VmVecScalef (&o, &o, 0.1f);
for (i = 0; i < 4; i++)
	VmVecIncf (sprite + i, &o);
pf = gameData.render.color.textures + t;
a = (float) (pf->color.red * 3 + pf->color.green * 5 + pf->color.blue * 2) / 30 * 2;
a *= dim;
if (a < 0.01)
	return;
l /= 4;
if (l < 0.01)
	return;
glColor4f (pf->color.red * l, pf->color.green * l, pf->color.blue * l, a);
//render the corona
VmVecNormalf (&n, sprite, sprite + 1, sprite + 2);
glEnable (GL_TEXTURE_2D);
if (OglBindBmTex (bmpCorona, 1, -1)) 
	return;
OglTexWrap (bmpCorona->glTexture, GL_CLAMP);
glDisable (GL_CULL_FACE);
glBegin (GL_QUADS);
for (i = 0; i < 4; i++) {
	glTexCoord2fv ((GLfloat *) (uvlList + i));
	glVertex3fv ((GLfloat *) (sprite + i));
	}
glEnd ();
glEnable (GL_CULL_FACE);
#if 0//def _DEBUG
glDisable (GL_TEXTURE_2D);
glColor4d (1,1,1,1);
glLineWidth (2);
glBegin (GL_LINE_LOOP);
for (i = 0; i < 4; i++)
	glVertex3fv ((GLfloat *) (sprite + i));
glEnd ();
glLineWidth (1);
#endif
}

// -----------------------------------------------------------------------------------
//	Render a tSide.
//	Check for normal facing.  If so, render faces on tSide dictated by sideP->nType.

#undef LMAP_LIGHTADJUST
#define LMAP_LIGHTADJUST 0

void RenderSide (tSegment *segP, short nSide)
{
//	short			props.vp [4];
//	short			nSegment = SEG_IDX (segP);
	tSide			*sideP = segP->sides + nSide;
	vmsVector	tvec;
	fix			v_dot_n0, v_dot_n1;
//	tUVL			temp_uvls [4];
	fix			min_dot, max_dot;
	vmsVector  normals [2];
//	ubyte			widFlags = WALL_IS_DOORWAY (segP, nSide, NULL);
	int			bDoLightMaps = gameStates.render.color.bLightMapsOk && 
										gameOpts->render.color.bUseLightMaps && 
										gameOpts->render.color.bAmbientLight && 
										!IsMultiGame;
	tFaceProps	props;

#if LIGHTMAPS
#define	LMAP_SIZE	(1.0 / 16.0)

	static tUVL	uvl_lMaps [4] = {
		{fl2f (LMAP_SIZE), fl2f (LMAP_SIZE), 0}, 
		{fl2f (1.0 - LMAP_SIZE), fl2f (LMAP_SIZE), 0}, 
		{fl2f (1.0 - LMAP_SIZE), fl2f (1.0 - LMAP_SIZE), 0}, 
		{fl2f (LMAP_SIZE), fl2f (1.0 - LMAP_SIZE), 0}
	};
#endif

props.segNum = SEG_IDX (segP);
props.sideNum = nSide;
props.widFlags = WALL_IS_DOORWAY (segP, props.sideNum, NULL);
if (!(gameOpts->render.bWalls || IsMultiGame) && IS_WALL (WallNumP (segP, props.sideNum)))
	return;
switch (gameStates.render.nType) {
	case -1:
		if (!(props.widFlags & WID_RENDER_FLAG) && (gameData.segs.segment2s [props.segNum].special < SEGMENT_IS_WATER))		//if (WALL_IS_DOORWAY(segP, props.sideNum) == WID_NO_WALL)
			return;
		break;
	case 0:
		if (segP->children [props.sideNum] >= 0) //&& IS_WALL (WallNumP (segP, props.sideNum)))
			return;
		break;
	case 1:
		if (!IS_WALL (WallNumP (segP, props.sideNum))) 
			return;
		break;
	case 2:
		if ((gameData.segs.segment2s [props.segNum].special < SEGMENT_IS_WATER) &&
			 (gameData.segs.xSegments [props.segNum].owner < 1))
			return;
		break;
	case 3:
		if ((IsLight (sideP->nBaseTex) || (sideP->nOvlTex && IsLight (sideP->nOvlTex))))
			RenderCorona (props.segNum, props.sideNum);
		return;
	}
normals [0] = sideP->normals [0];
normals [1] = sideP->normals [1];
#if LIGHTMAPS
if (bDoLightMaps) {
		float	Xs = 8;
		float	h = 0.5f / (float) Xs;

	props.uvl_lMaps [0].u =
	props.uvl_lMaps [0].v =
	props.uvl_lMaps [1].v =
	props.uvl_lMaps [3].u = fl2f (h);
	props.uvl_lMaps [1].u =
	props.uvl_lMaps [2].u =
	props.uvl_lMaps [2].v =
	props.uvl_lMaps [3].v = fl2f (1-h);
	}
#endif
props.nBaseTex = sideP->nBaseTex;
props.nOvlTex = sideP->nOvlTex;
props.nOvlOrient = sideP->nOvlOrient;

	//	========== Mark: Here is the change...beginning here: ==========

GetSideVerts (props.vp, props.segNum, props.sideNum);
if (sideP->nType == SIDE_IS_QUAD) {
	VmVecSub (&tvec, &gameData.render.mine.viewerEye, gameData.segs.vertices + segP->verts [sideToVerts [props.sideNum][0]]);
	v_dot_n0 = VmVecDot (&tvec, normals);
	if (v_dot_n0 < 0)
		return;
	memcpy (props.uvls, sideP->uvls, sizeof (tUVL) * 4);
#if LIGHTMAPS
	if (bDoLightMaps) {
		memcpy (props.uvl_lMaps, uvl_lMaps, sizeof (tUVL) * 4);
#if LMAP_LIGHTADJUST
		props.uvls [0].l = props.uvls [1].l = props.uvls [2].l = props.uvls [3].l = F1_0 / 2;
#	endif
		}
#endif
	props.nv = 4;
	props.vNormal = normals [0];
	RenderFace (&props, 0, 0);
#ifdef EDITOR
	CheckFace (props.segNum, props.sideNum, 0, 3, props.vp, sideP->nBaseTex, sideP->nOvlTex, sideP->uvls);
#endif
	} 
else {
	//	Regardless of whether this tSide is comprised of a single quad, or two triangles, we need to know one normal, so
	//	deal with it, get the dot product.
	VmVecNormalizedDirQuick (&tvec, &gameData.render.mine.viewerEye, gameData.segs.vertices + segP->verts [sideToVerts [props.sideNum][sideP->nType == SIDE_IS_TRI_13]]);
	v_dot_n0 = VmVecDot (&tvec, normals);

	//	========== Mark: The change ends here. ==========

	//	Although this tSide has been triangulated, because it is not planar, see if it is acceptable
	//	to render it as a single quadrilateral.  This is a function of how far away the viewer is, how non-planar
	//	the face is, how normal to the surfaces the view is.
	//	Now, if both dot products are close to 1.0, then render two triangles as a single quad.
	v_dot_n1 = VmVecDot(&tvec, normals+1);
#if 1
	if (v_dot_n0 < v_dot_n1) {
		min_dot = v_dot_n0;
		max_dot = v_dot_n1;
		}
	else {
		min_dot = v_dot_n1;
		max_dot = v_dot_n0;
		}
	//	Determine whether to detriangulate tSide: (speed hack, assumes Tulate_min_ratio == F1_0*2, should FixMul(min_dot, Tulate_min_ratio))
	if (gameStates.render.bDetriangulation && ((min_dot+F1_0/256 > max_dot) || ((gameData.objs.viewer->nSegment != props.segNum) &&  (min_dot > Tulate_min_dot) && (max_dot < min_dot*2)))) {
		//	The other detriangulation code doesn't deal well with badly non-planar sides.
		fix	n0_dot_n1 = VmVecDot(normals, normals + 1);
		if (n0_dot_n1 < Min_n0_n1_dot)
			goto im_so_ashamed;
		if (min_dot >= 0) {
			memcpy (props.uvls, sideP->uvls, sizeof (props.uvls));
#if LIGHTMAPS
			if (bDoLightMaps) {
				memcpy (props.uvl_lMaps, uvl_lMaps, sizeof (tUVL) * 4);
#	if LMAP_LIGHTADJUST
				props.uvls [0].l = props.uvls [1].l = props.uvls [2].l = props.uvls [3].l = F1_0 / 2;
#	endif
				}
#endif
			props.nv = 4;
			props.vNormal = normals [0];
			RenderFace (&props, 0, 0);
#ifdef EDITOR
			CheckFace (props.segNum, props.sideNum, 0, 3, props.vp, sideP->nBaseTex, sideP->nOvlTex, sideP->uvls);
#endif
			}
		}
	else 
#endif
		{
im_so_ashamed: ;
		props.nv = 3;
		if (sideP->nType == SIDE_IS_TRI_02) {
			if (v_dot_n0 >= 0) {
				memcpy (props.uvls, sideP->uvls, sizeof (tUVL) * 3);
#if LIGHTMAPS
				if (bDoLightMaps) {
					memcpy (props.uvl_lMaps, uvl_lMaps, sizeof (tUVL) * 3);
#	if LMAP_LIGHTADJUST
					props.uvls [0].l = props.uvls [1].l = props.uvls [2].l = F1_0 / 2;
#	endif
					}
#endif
				props.vNormal = normals [0];
				RenderFace (&props, 0, 0);
#ifdef EDITOR
				CheckFace (props.segNum, props.sideNum, 0, 3, props.vp, sideP->nBaseTex, sideP->nOvlTex, sideP->uvls);
#endif
				}

			if (v_dot_n1 >= 0) {
				props.uvls [0] = sideP->uvls [0];
				memcpy (props.uvls + 1, sideP->uvls + 2, sizeof (tUVL) * 2);
#if LIGHTMAPS
				if (bDoLightMaps) {
					props.uvl_lMaps [0] = uvl_lMaps [0];
					memcpy (props.uvl_lMaps + 1, uvl_lMaps + 2, sizeof (tUVL) * 2);
#if LMAP_LIGHTADJUST
					props.uvls [0].l = props.uvls [1].l = props.uvls [2].l = F1_0 / 2;
#endif
					}
#endif
				props.vp [1] = props.vp [2];	
				props.vp [2] = props.vp [3];	// want to render from vertices 0, 2, 3 on tSide
				props.vNormal = normals [1];
				RenderFace (&props, 0, 0);
#ifdef EDITOR
				CheckFace (props.segNum, props.sideNum, 0, 3, props.vp, sideP->nBaseTex, sideP->nOvlTex, sideP->uvls);
#endif
				}
			}
		else if (sideP->nType == SIDE_IS_TRI_13) {
			if (v_dot_n1 >= 0) {
				memcpy (props.uvls + 1, sideP->uvls + 1, sizeof (tUVL) * 3);
#if LIGHTMAPS
				if (bDoLightMaps) {
					memcpy (props.uvl_lMaps + 1, uvl_lMaps + 1, sizeof (tUVL) * 3);
#	if LMAP_LIGHTADJUST
					props.uvls [1].l = props.uvls [2].l = props.uvls [3].l = F1_0 / 2;
#	endif
					}
#endif
				props.vNormal = normals [0];
				RenderFace (&props, 1, 0);
#ifdef EDITOR
				CheckFace (props.segNum, props.sideNum, 0, 3, props.vp, sideP->nBaseTex, sideP->nOvlTex, sideP->uvls);
#endif
				}

			if (v_dot_n0 >= 0) {
				props.uvls [0] = sideP->uvls [0];		
				props.uvls [1] = sideP->uvls [1];		
				props.uvls [2] = sideP->uvls [3];
				props.vp [2] = props.vp [3];		// want to render from vertices 0, 1, 3
#if LIGHTMAPS
				if (bDoLightMaps) {
					props.uvl_lMaps [0] = uvl_lMaps [0];
					props.uvl_lMaps [1] = uvl_lMaps [1];
					props.uvl_lMaps [2] = uvl_lMaps [3];
#if LMAP_LIGHTADJUST
					props.uvls [0].l = props.uvls [1].l = props.uvls [2].l = F1_0 / 2;
#endif
					}
#endif
				props.vNormal = normals [1];
				RenderFace (&props, 0, 0);
#ifdef EDITOR
				CheckFace (props.segNum, props.sideNum, 0, 3, props.vp, sideP->nBaseTex, sideP->nOvlTex, sideP->uvls);
#endif
				}
			}
		else {
			Error("Illegal tSide nType in RenderSide, nType = %i, tSegment # = %i, tSide # = %i\n", sideP->nType, SEG_IDX (segP), props.sideNum);
			return;
			}
		}
	}
}

//------------------------------------------------------------------------------

#ifdef EDITOR
void renderObject_search(tObject *objP)
{
	int changed=0;

	//note that we draw each pixel tObject twice, since we cannot control
	//what color the tObject draws in, so we try color 0, then color 1, 
	//in case the tObject itself is rendering color 0

	GrSetColor(0);
	gr_pixel(_search_x, _search_y);	//set our search pixel to color zero
	RenderObject (objP, 0, 0);
	if (gr_ugpixel(&grdCurCanv->cv_bitmap, _search_x, _search_y) != 0)
		changed=1;

	GrSetColor(1);
	gr_pixel(_search_x, _search_y);	//set our search pixel to color zero
	RenderObject (objP, 0, 0);
	if (gr_ugpixel(&grdCurCanv->cv_bitmap, _search_x, _search_y) != 1)
		changed=1;

	if (changed) {
		if (objP->nSegment != -1)
			Cursegp = gameData.segs.segments+objP->nSegment;
		found_seg = -(OBJ_IDX (objP)+1);
	}
}
#endif

//------------------------------------------------------------------------------

extern ubyte nDemoDoingRight, nDemoDoingLeft;
void DoRenderObject(int nObject, int nWindow)
{
#ifdef EDITOR
	int save_3d_outline=0;
#endif
	tObject *objP = gameData.objs.objects+nObject, *hObj;
	tWindowRenderedData *wrd = windowRenderedData + nWindow;
	int count = 0;
	int n;

	if (!(IsMultiGame || gameOpts->render.bObjects))
		return;
#if 0
	if (obj == gameData.objs.console)
		return;
#endif
	Assert(nObject < MAX_OBJECTS);
#ifdef _DEBUG
	if (bObjectRendered [nObject]) {		//already rendered this...
		Int3();		//get Matt!!!
		return;
	}
#endif
   if (gameData.demo.nState == ND_STATE_PLAYBACK) {
	  if ((nDemoDoingLeft == 6 || nDemoDoingRight == 6) && objP->nType == OBJ_PLAYER) {
			// A nice fat hack: keeps the tPlayer ship from showing up in the
			// small extra view when guiding a missile in the big window
#if TRACE	
			//con_printf (CONDBG, "Returning from RenderObject prematurely...\n");
#endif
  			return; 
			}
		}
	//	Added by MK on 09/07/94 (at about 5:28 pm, CDT, on a beautiful, sunny late summer day!) so
	//	that the guided missile system will know what gameData.objs.objects to look at.
	//	I didn't know we had guided missiles before the release of D1. --MK
	if ((objP->nType == OBJ_ROBOT) || (objP->nType == OBJ_PLAYER) ||
		 ((objP->nType == OBJ_WEAPON) && ((objP->id == PROXMINE_ID) || (objP->id == SMARTMINE_ID)))) {
		//Assert(windowRenderedData [nWindow].renderedObjects < MAX_RENDERED_OBJECTS);
		//	This peculiar piece of code makes us keep track of the most recently rendered gameData.objs.objects, which
		//	are probably the higher priority gameData.objs.objects, without overflowing the buffer
		if (wrd->numObjects >= MAX_RENDERED_OBJECTS) {
			Int3();
			wrd->numObjects /= 2;
		}
		wrd->renderedObjects [wrd->numObjects++] = nObject;
	}
	if ((count++ > MAX_OBJECTS) || (objP->next == nObject)) {
		Int3();					// infinite loop detected
		objP->next = -1;		// won't this clean things up?
		return;					// get out of this infinite loop!
	}
	//g3_drawObject(objP->class_id, &objP->position.vPos, &objP->position.mOrient, objP->size);
	//check for editor tObject
#ifdef EDITOR
	if (gameStates.app.nFunctionMode==FMODE_EDITOR && nObject==CurObject_index) {
		save_3d_outline = g3d_interp_outline;
		g3d_interp_outline=1;
	}
	if (bSearchMode)
		renderObject_search(objP);
	else
#endif
		//NOTE LINK TO ABOVE
	bObjectRendered [nObject] = RenderObject (objP, nWindow, 0);
	for (n = objP->attachedObj; n != -1; n = hObj->cType.explInfo.nNextAttach) {
		hObj = gameData.objs.objects + n;
		Assert(hObj->nType == OBJ_FIREBALL);
		Assert(hObj->controlType == CT_EXPLOSION);
		Assert(hObj->flags & OF_ATTACHED);
		RenderObject (hObj, nWindow, 1);
	}
#ifdef EDITOR
	if (gameStates.app.nFunctionMode==FMODE_EDITOR && nObject==CurObject_index)
		g3d_interp_outline = save_3d_outline;
#endif
}

// -----------------------------------------------------------------------------------

#ifdef _DEBUG
int bDrawBoxes=0;
int bWindowCheck = 1, bDrawEdges = 0, bNewSegSorting = 1, bPreDrawSegs = 0;
int no_migrate_segs=1, migrateObjects=1;
int check_bWindowCheck=0;
#else
#define bDrawBoxes			0
#define bWindowCheck			1
#define bDrawEdges			0
#define bNewSegSorting		1
#define bPreDrawSegs			0
#define no_migrate_segs		1
#define migrateObjects		1
#define check_bWindowCheck	0
#endif

// -----------------------------------------------------------------------------------
//increment counter for checking if points bRotated
//This must be called at the start of the frame if RotateVertexList() will be used
void RenderStartFrame()
{
if (!++gameStates.render.nFrameCount) {		//wrap!
	memset(nRotatedLast, 0, sizeof (nRotatedLast));		//clear all to zero
	gameStates.render.nFrameCount = 1;											//and set this frame to 1
	}
}

// -----------------------------------------------------------------------------------
//Given a list of point numbers, rotate any that haven't been bRotated this frame
//cc.and and cc.or will contain the position/orientation of the face that is determined 
//by the vertices passed relative to the viewer
g3s_codes RotateVertexList (int nv, short *vertexIndexP)
{
	int			i, j;
	g3sPoint		*pnt;
	g3s_codes	cc;

cc.and = 0xff;  
cc.or = 0;
for (i = 0; i < nv; i++) {
	j = vertexIndexP [i];
	pnt = gameData.segs.points + j;
	if (nRotatedLast [j] != gameStates.render.nFrameCount) {
		G3TransformAndEncodePoint (pnt, gameData.segs.vertices + j);
		if (!gameStates.ogl.bUseTransform) {
			gameData.segs.fVertices [j].p.x = f2fl (pnt->p3_vec.p.x);
			gameData.segs.fVertices [j].p.y = f2fl (pnt->p3_vec.p.y);
			gameData.segs.fVertices [j].p.z = f2fl (pnt->p3_vec.p.z);
			}
		nRotatedLast [j] = gameStates.render.nFrameCount;
		}
	cc.and &= pnt->p3_codes;
	cc.or |= pnt->p3_codes;
	pnt->p3_index = j;
	}
return cc;
}

// -----------------------------------------------------------------------------------
//Given a lit of point numbers, project any that haven't been projected
void ProjectVertexList (int nv, short *vertexIndexP)
{
	int i, j;

for (i = 0; i < nv; i++) {
	j = vertexIndexP [i];
	if (!(gameData.segs.points [j].p3_flags & PF_PROJECTED))
		G3ProjectPoint (gameData.segs.points + j);
	}
}

// -----------------------------------------------------------------------------------

void SortSidesByDist (double *sideDists, char *sideNums, char left, char right)
{
	char		l = left;
	char		r = right;
	char		h, m = (l + r) / 2;
	double	hd, md = sideDists [m];

do {
	while (sideDists [l] < md)
		l++;
	while (sideDists [r] > md)
		r--;
	if (l <= r) {
		if (l < r) {
			h = sideNums [l];
			sideNums [l] = sideNums [r];
			sideNums [r] = h;
			hd = sideDists [l];
			sideDists [l] = sideDists [r];
			sideDists [r] = hd;
			}
		}
	++l;
	--r;
	} while (l <= r);
if (right > l)
   SortSidesByDist (sideDists, sideNums, l, right);
if (r > left)
   SortSidesByDist (sideDists, sideNums, left, r);
}

// -----------------------------------------------------------------------------------

void RenderSegment (short nSegment, int nWindow)
{
	tSegment		*segP = gameData.segs.segments + nSegment;
	g3s_codes 	cc;
	short			sn;

#if SHADOWS
if (EGI_FLAG (bShadows, 0, 1, 0) && 
	 FAST_SHADOWS && 
	 !gameOpts->render.shadows.bSoft && 
	 (gameStates.render.nShadowPass >= 2))
	return;
#	if 0
if (gameStates.render.nShadowPass == 2)
	return;
#	endif
#endif
Assert(nSegment!=-1 && nSegment <= gameData.segs.nLastSegment);
if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment))
	return;
OglSetupTransform ();
cc = RotateVertexList (8, segP->verts);
gameData.render.pVerts = gameData.segs.fVertices;
//	return;
if (!cc.and || gameStates.render.automap.bDisplay) {		//all off screen?
	gameStates.render.nState = 0;
#if OGL_QUERY
	if (gameStates.render.bQueryOcclusion) {
			short		sideVerts [4];
			double	sideDists [6];
			char		sideNums [6];
			int		i; 
			double	d, dMin, dx, dy, dz;
			tObject	*objP = gameData.objs.objects + LOCALPLAYER.nObject;

		for (sn = 0; sn < MAX_SIDES_PER_SEGMENT; sn++) {
			sideNums [sn] = (char) sn;
			GetSideVerts (sideVerts, nSegment, sn);
			dMin = 1e300;
			for (i = 0; i < 4; i++) {
				dx = objP->position.vPos.p.x - gameData.segs.vertices [sideVerts [i]].p.x;
				dy = objP->position.vPos.p.y - gameData.segs.vertices [sideVerts [i]].p.y;
				dz = objP->position.vPos.p.z - gameData.segs.vertices [sideVerts [i]].p.z;
				d = dx * dx + dy * dy + dz * dz;
				if (dMin > d)
					dMin = d;
				}
			sideDists [sn] = dMin;
			}
		SortSidesByDist (sideDists, sideNums, 0, 5);
		for (sn = 0; sn < MAX_SIDES_PER_SEGMENT; sn++)
			RenderSide (segP, sideNums [sn]);
		}
	else 
#endif
		{
		for (sn = 0; sn < MAX_SIDES_PER_SEGMENT; sn++)
			RenderSide (segP, sn);
		}
	}
OglResetTransform ();
OGL_BINDTEX (0);
}

#define CROSS_WIDTH  i2f(8)
#define CROSS_HEIGHT i2f(8)

#ifdef _DEBUG

//------------------------------------------------------------------------------
//draw outline for curside
void OutlineSegSide(tSegment *seg, int _side, int edge, int vert)
{
	g3s_codes cc;

cc=RotateVertexList(8, seg->verts);
if (! cc.and) {		//all off screen?
	g3sPoint *pnt;
	//render curedge of curside of curseg in green
	GrSetColorRGB (0, 255, 0, 255);
	G3DrawLine(gameData.segs.points + seg->verts [sideToVerts [_side][edge]], 
						gameData.segs.points + seg->verts [sideToVerts [_side][(edge+1)%4]]);
	//draw a little cross at the current vert
	pnt = gameData.segs.points + seg->verts [sideToVerts [_side][vert]];
	G3ProjectPoint(pnt);		//make sure projected
	GrLine(pnt->p3_screen.x-CROSS_WIDTH, pnt->p3_screen.y, pnt->p3_screen.x, pnt->p3_screen.y-CROSS_HEIGHT);
	GrLine(pnt->p3_screen.x, pnt->p3_screen.y-CROSS_HEIGHT, pnt->p3_screen.x+CROSS_WIDTH, pnt->p3_screen.y);
	GrLine(pnt->p3_screen.x+CROSS_WIDTH, pnt->p3_screen.y, pnt->p3_screen.x, pnt->p3_screen.y+CROSS_HEIGHT);
	GrLine(pnt->p3_screen.x, pnt->p3_screen.y+CROSS_HEIGHT, pnt->p3_screen.x-CROSS_WIDTH, pnt->p3_screen.y);
	}
}

#endif

#if 0		//this stuff could probably just be deleted

//------------------------------------------------------------------------------
#define DEFAULT_PERSPECTIVE_DEPTH 6

int nPerspectiveDepth=DEFAULT_PERSPECTIVE_DEPTH;	//how many levels deep to render in perspective

int IncPerspectiveDepth(void)
{
return ++nPerspectiveDepth;
}

//------------------------------------------------------------------------------

int DecPerspectiveDepth (void)
{
return (nPerspectiveDepth == 1) ? nPerspectiveDepth : --nPerspectiveDepth;
}

//------------------------------------------------------------------------------

int ResetPerspectiveDepth (void)
{
return nPerspectiveDepth = DEFAULT_PERSPECTIVE_DEPTH;
}
#endif

//------------------------------------------------------------------------------

typedef struct window {
	short left, top, right, bot;
} window;

ubyte code_window_point (fix x, fix y, window *w)
{
	ubyte code = 0;

if (x <= w->left)  
	code |= 1;
else if (x >= w->right) 
	code |= 2;
if (y <= w->top) 
	code |= 4;
else if (y >= w->bot) 
	code |= 8;
return code;
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
void DrawWindowBox (unsigned int color, short left, short top, short right, short bot)
{
	short l, t, r, b;

GrSetColorRGBi (color);
l=left; 
t=top; 
r=right; 
b=bot;

if ( r<0 || b<0 || l>=grdCurCanv->cv_bitmap.bm_props.w || (t>=grdCurCanv->cv_bitmap.bm_props.h && b>=grdCurCanv->cv_bitmap.bm_props.h))
	return;

if (l<0) 
	l=0;
if (t<0) 
	t=0;
if (r>=grdCurCanv->cv_bitmap.bm_props.w) r=grdCurCanv->cv_bitmap.bm_props.w-1;
if (b>=grdCurCanv->cv_bitmap.bm_props.h) b=grdCurCanv->cv_bitmap.bm_props.h-1;

GrLine(i2f(l), i2f(t), i2f(r), i2f(t));
GrLine(i2f(r), i2f(t), i2f(r), i2f(b));
GrLine(i2f(r), i2f(b), i2f(l), i2f(b));
GrLine(i2f(l), i2f(b), i2f(l), i2f(t));
}
#endif

//------------------------------------------------------------------------------

int MattFindConnectedSide(int seg0, int seg1);

#ifdef _DEBUG
char visited2 [MAX_SEGMENTS_D2X];
#endif

#define VISITED(_ch)	(gameData.render.mine.bVisited [_ch] == gameData.render.mine.nVisited)
#define VISIT(_ch) (gameData.render.mine.bVisited [_ch] = gameData.render.mine.nVisited)
//@@short *persp_ptr;
short nRenderPos [MAX_SEGMENTS_D2X];	//where in render_list does this tSegment appear?
//ubyte no_renderFlag [MAX_RENDER_SEGS];
window renderWindows [MAX_RENDER_SEGS];

short nRenderObjList [MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS][OBJS_PER_SEG];

//for gameData.objs.objects

//Given two sides of tSegment, tell the two verts which form the 
//edge between them
short edgeBetweenTwoSides [6][6][2] = {
	{ {-1, -1}, {3, 7}, {-1, -1}, {2, 6}, {6, 7}, {2, 3} }, 
	{ {3, 7}, {-1, -1}, {0, 4}, {-1, -1}, {4, 7}, {0, 3} }, 
	{ {-1, -1}, {0, 4}, {-1, -1}, {1, 5}, {4, 5}, {0, 1} }, 
	{ {2, 6}, {-1, -1}, {1, 5}, {-1, -1}, {5, 6}, {1, 2} }, 
	{ {6, 7}, {4, 7}, {4, 5}, {5, 6}, {-1, -1}, {-1, -1} }, 
	{ {2, 3}, {0, 3}, {0, 1}, {1, 2}, {-1, -1}, {-1, -1} }
};

//given an edge specified by two verts, give the two sides on that edge
int edgeToSides [8][8][2] = {
	{ {-1, -1}, {2, 5}, {-1, -1}, {1, 5}, {1, 2}, {-1, -1}, {-1, -1}, {-1, -1} }, 
	{ {2, 5}, {-1, -1}, {3, 5}, {-1, -1}, {-1, -1}, {2, 3}, {-1, -1}, {-1, -1} }, 
	{ {-1, -1}, {3, 5}, {-1, -1}, {0, 5}, {-1, -1}, {-1, -1}, {0, 3}, {-1, -1} }, 
	{ {1, 5}, {-1, -1}, {0, 5}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {0, 1} }, 
	{ {1, 2}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {2, 4}, {-1, -1}, {1, 4} }, 
	{ {-1, -1}, {2, 3}, {-1, -1}, {-1, -1}, {2, 4}, {-1, -1}, {3, 4}, {-1, -1} }, 
	{ {-1, -1}, {-1, -1}, {0, 3}, {-1, -1}, {-1, -1}, {3, 4}, {-1, -1}, {0, 4} }, 
	{ {-1, -1}, {-1, -1}, {-1, -1}, {0, 1}, {1, 4}, {-1, -1}, {0, 4}, {-1, -1} }, 
};

//@@//perform simple check on tables
//@@check_check()
//@@{
//@@	int i, j;
//@@
//@@	for (i=0;i<8;i++)
//@@		for (j=0;j<8;j++)
//@@			Assert(edgeToSides [i][j][0] == edgeToSides [j][i][0] && 
//@@					edgeToSides [i][j][1] == edgeToSides [j][i][1]);
//@@
//@@	for (i=0;i<6;i++)
//@@		for (j=0;j<6;j++)
//@@			Assert(edgeBetweenTwoSides [i][j][0] == edgeBetweenTwoSides [j][i][0] && 
//@@					edgeBetweenTwoSides [i][j][1] == edgeBetweenTwoSides [j][i][1]);
//@@
//@@
//@@}


//------------------------------------------------------------------------------
//given an edge and one tSide adjacent to that edge, return the other adjacent tSide

int FindOtherSideOnEdge (tSegment *seg, short *verts, int notside)
{
	int	i;
	int	vv0 = -1, vv1 = -1;
	int	side0, side1;
	int	*eptr;
	int	v0, v1;
	short	*vp;

//@@	check_check();

v0 = verts [0];
v1 = verts [1];
vp = seg->verts;
for (i = 0; i < 8; i++) {
	int svv = *vp++;	// seg->verts [i];
	if (vv0 == -1 && svv == v0) {
		vv0 = i;
		if (vv1 != -1)
			break;
		}
	if (vv1 == -1 && svv == v1) {
		vv1 = i;
		if (vv0 != -1)
			break;
		}
	}
Assert(vv0 != -1 && vv1 != -1);

eptr = edgeToSides [vv0][vv1];
side0 = eptr [0];
side1 = eptr [1];
Assert(side0 != -1 && side1 != -1);
if (side0 != notside) {
	Assert(side1==notside);
	return side0;
}
Assert(side0==notside);
return side1;
}

//------------------------------------------------------------------------------
//find the two segments that join a given seg through two sides, and
//the sides of those segments the abut. 

typedef struct tSideNormData {
	vmsVector	n [2];
	vmsVector	*p;
	short			t;
} tSideNormData;

int FindAdjacentSideNorms (tSegment *seg, short s0, short s1, tSideNormData *s)
{
	tSegment	*seg0, *seg1;
	tSide		*side0, *side1;
	short		edge_verts [2];
	int		notside0, notside1;
	int		edgeside0, edgeside1;

	Assert(s0 != -1 && s1 != -1);

	seg0 = gameData.segs.segments + seg->children [s0];
	seg1 = gameData.segs.segments + seg->children [s1];

	edge_verts [0] = seg->verts [edgeBetweenTwoSides [s0][s1][0]];
	edge_verts [1] = seg->verts [edgeBetweenTwoSides [s0][s1][1]];

	Assert(edge_verts [0] != -1 && edge_verts [1] != -1);

	notside0 = FindConnectedSide (seg, seg0);
	Assert (notside0 != -1);
	notside1 = FindConnectedSide (seg, seg1);
	Assert (notside1 != -1);

	edgeside0 = FindOtherSideOnEdge (seg0, edge_verts, notside0);
	edgeside1 = FindOtherSideOnEdge (seg1, edge_verts, notside1);

	//deal with the case where an edge is shared by more than two segments

//@@	if (IS_CHILD(seg0->children [edgeside0])) {
//@@		tSegment *seg00;
//@@		int notside00;
//@@
//@@		seg00 = &gameData.segs.segments [seg0->children [edgeside0]];
//@@
//@@		if (seg00 != seg1) {
//@@
//@@			notside00 = FindConnectedSide(seg0, seg00);
//@@			Assert(notside00 != -1);
//@@
//@@			edgeside0 = FindOtherSideOnEdge(seg00, edge_verts, notside00);
//@@	 		seg0 = seg00;
//@@		}
//@@
//@@	}
//@@
//@@	if (IS_CHILD(seg1->children [edgeside1])) {
//@@		tSegment *seg11;
//@@		int notside11;
//@@
//@@		seg11 = &gameData.segs.segments [seg1->children [edgeside1]];
//@@
//@@		if (seg11 != seg0) {
//@@			notside11 = FindConnectedSide(seg1, seg11);
//@@			Assert(notside11 != -1);
//@@
//@@			edgeside1 = FindOtherSideOnEdge(seg11, edge_verts, notside11);
//@@ 			seg1 = seg11;
//@@		}
//@@	}

//	if ( IS_CHILD(seg0->children [edgeside0]) ||
//		  IS_CHILD(seg1->children [edgeside1])) 
//		return 0;

side0 = seg0->sides + edgeside0;
side1 = seg1->sides + edgeside1;
memcpy (s [0].n, side0->normals, 2 * sizeof (vmsVector));
memcpy (s [1].n, side1->normals, 2 * sizeof (vmsVector));
s [0].p = gameData.segs.vertices + seg0->verts [sideToVerts [edgeside0][(s [0].t = side0->nType) == 3]];
s [1].p = gameData.segs.vertices + seg1->verts [sideToVerts [edgeside1][(s [1].t = side1->nType) == 3]];
return 1;
}

//------------------------------------------------------------------------------
//see if the order matters for these two children.
//returns 0 if order doesn't matter, 1 if c0 before c1, -1 if c1 before c0
static int CompareChildren (tSegment *seg, short c0, short c1)
{
	tSideNormData	s [2];
	vmsVector		temp;
	fix				d0, d1;

if (sideOpposite [c0] == c1) 
	return 0;

Assert(c0 != -1 && c1 != -1);

//find normals of adjoining sides
FindAdjacentSideNorms (seg, c0, c1, s);

VmVecSub (&temp, &gameData.render.mine.viewerEye, s [0].p);
d0 = VmVecDot (s [0].n, &temp);
if (s [0].t != 1)	// triangularized face -> 2 different normals
	d0 |= VmVecDot (s [0].n + 1, &temp);	// we only need the sign, so a bitwise or does the trick
VmVecSub (&temp, &gameData.render.mine.viewerEye, s [1].p);
d1 = VmVecDot (s [1].n, &temp);
if (s [1].t != 1) 
	d1 |= VmVecDot (s [1].n + 1, &temp);

#if 0
d0 = (d0_0 < 0) || (d0_1 < 0) ? -1 : 1;
d1 = (d1_0 < 0) || (d1_1 < 0) ? -1 : 1;
#endif
if ((d0 & d1) < 0)	// only < 0 if both negative due to bitwise and
	return 0;
if (d0 < 0)
	return 1;
 if (d1 < 0)
	return -1;
return 0;
}

//------------------------------------------------------------------------------

int QuicksortSegChildren (tSegment *seg, short left, short right, short *childList)
{
	short	h, 
			l = left, 
			r = right, 
			m = (l + r) / 2, 
			median = childList [m], 
			bSwap = 0;

do {
	while ((l < m) && CompareChildren (seg, childList [l], median) >= 0)
		l++;
	while ((r > m) && CompareChildren (seg, childList [r], median) <= 0)
		r--;
	if (l <= r) {
		if (l < r) {
			h = childList [l];
			childList [l] = childList [r];
			childList [r] = h;
			bSwap = 1;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	bSwap |= QuicksortSegChildren (seg, l, right, childList);
if (left < r)
	bSwap |= QuicksortSegChildren (seg, left, r, childList);
return bSwap;
}

//------------------------------------------------------------------------------

int sscTotal=0, ssc_swaps=0;

//short the children of tSegment to render in the correct order
//returns non-zero if swaps were made
int SortSegChildren(tSegment *seg, int n_children, short *childList)
{
#if 1
if (n_children < 2) 
	return 0;
return QuicksortSegChildren (seg, (short) 0, (short) (n_children - 1), childList);
#else
	int i, j;
	int r;
	int made_swaps, count;

if (n_children < 2) 
	return 0;
sscTotal++;
	//for each child,  compare with other children and see if order matters
	//if order matters, fix if wrong

count = 0;

do {
	made_swaps = 0;

	for (i=0;i<n_children-1;i++)
		for (j=i+1;childList [i]!=-1 && j<n_children;j++)
			if (childList [j]!=-1) {
				r = CompareChildren(seg, childList [i], childList [j]);

				if (r == 1) {
					int temp = childList [i];
					childList [i] = childList [j];
					childList [j] = temp;
					made_swaps=1;
				}
			}

} while (made_swaps && ++count<n_children);
if (count)
	ssc_swaps++;
return count;
#endif
}

//------------------------------------------------------------------------------

void AddObjectToSegList(int nObject, int listnum)
{
	int i, checkn, marker;

	checkn = listnum;
	//first, find a slot
	do {
		for (i=0;nRenderObjList [checkn][i] >= 0;i++);
		Assert(i < OBJS_PER_SEG);
		marker = nRenderObjList [checkn][i];
		if (marker != -1) {
			checkn = -marker;
			//Assert(checkn < MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS);
			if (checkn >= MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS) {
				Int3();
				return;
			}
		}

	} while (marker != -1);


	//now we have found a slot.  put tObject in it

	if (i != OBJS_PER_SEG-1) {

		nRenderObjList [checkn][i] = nObject;
		nRenderObjList [checkn][i+1] = -1;
	}
	else {				//chain to additional list
		int lookn;

		//find an available sublist

		for (lookn=MAX_RENDER_SEGS;nRenderObjList [lookn][0]!=-1 && lookn<MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS;lookn++);

		//Assert(lookn<MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS);
		if (lookn >= MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS) {
			Int3();
			return;
		}

		nRenderObjList [checkn][i] = -lookn;
		nRenderObjList [lookn][0] = nObject;
		nRenderObjList [lookn][1] = -1;

	}

}
#ifdef __sun__
// the following is a drop-in replacement for the broken libc qsort on solaris
// taken from http://www.snippets.org/snippets/portable/RG_QSORT+C.php3

#define qsort qsort_dropin

/******************************************************************/
/* qsort.c  --  Non-Recursive ANSI Quicksort function             */
/* Public domain by Raymond Gardner, Englewood CO  February 1991  */
/******************************************************************/
#define  _COMP(a, b)  ((*comp)((void *)(a), (void *)(b)))
#define  T 7 // subfiles of <= T elements will be insertion sorteded (T >= 3)
#define  _SWAP(a, b)  (swap_bytes((char *)(a), (char *)(b), size))

static void swap_bytes(char *a, char *b, size_t nbytes)
{
   char tmp;
   do {
      tmp = *a; *a++ = *b; *b++ = tmp;
   } while ( --nbytes);
}

void qsort(void *basep, size_t nelems, size_t size, 
                            int (*comp)(const void *, const void *))
{
   char *stack [40], **sp;       /* stack and stack pointer        */
   char *i, *j, *limit;         /* scan and limit pointers        */
   size_t thresh;               /* size of T elements in bytes    */
   char *base;                  /* base pointer as char *         */
   base = (char *)basep;        /* set up char * base pointer     */
   thresh = T * size;           /* init threshold                 */
   sp = stack;                  /* init stack pointer             */
   limit = base + nelems * size;/* pointer past end of array      */
   for ( ;;) {                 /* repeat until break...          */
      if ( limit - base > thresh) {  /* if more than T elements  */
                                      /*   swap base with middle  */
         _SWAP((((limit-base)/size)/2)*size+base, base);
         i = base + size;             /* i scans left to right    */
         j = limit - size;            /* j scans right to left    */
         if ( _COMP(i, j) > 0)        /* Sedgewick's              */
            _SWAP(i, j);               /*    three-element sort    */
         if ( _COMP(base, j) > 0)     /*        sets things up    */
            _SWAP(base, j);            /*            so that       */
         if ( _COMP(i, base) > 0)     /*      *i <= *base <= *j   */
            _SWAP(i, base);            /* *base is pivot element   */
         for ( ;;) {                 /* loop until break         */
            do                        /* move i right             */
               i += size;             /*        until *i >= pivot */
            while ( _COMP(i, base) < 0);
            do                        /* move j left              */
               j -= size;             /*        until *j <= pivot */
            while ( _COMP(j, base) > 0);
            if ( i > j)              /* if pointers crossed      */
               break;                 /*     break loop           */
            _SWAP(i, j);       /* else swap elements, keep scanning*/
         }
         _SWAP(base, j);         /* move pivot into correct place  */
         if ( j - base > limit - i) {  /* if left subfile larger */
            sp [0] = base;             /* stack left subfile base  */
            sp [1] = j;                /*    and limit             */
            base = i;                 /* sort the right subfile   */
         } else {                     /* else right subfile larger*/
            sp [0] = i;                /* stack right subfile base */
            sp [1] = limit;            /*    and limit             */
            limit = j;                /* sort the left subfile    */
         }
         sp += 2;                     /* increment stack pointer  */
      } else {      /* else subfile is small, use insertion sort  */
         for ( j = base, i = j+size; i < limit; j = i, i += size)
            for ( ; _COMP(j, j+size) > 0; j -= size) {
               _SWAP(j, j+size);
               if ( j == base)
                  break;
            }
         if ( sp != stack) {         /* if any entries on stack  */
            sp -= 2;                  /* pop the base and limit   */
            base = sp [0];
            limit = sp [1];
         } else                       /* else stack empty, done   */
            break;
      }
   }
}
#endif // __sun__ qsort drop-in replacement

//------------------------------------------------------------------------------

#define SORT_LIST_SIZE 100

typedef struct sort_item {
	int nObject;
	fix dist;
} sort_item;

sort_item sortList [SORT_LIST_SIZE];
int nSortItems;

//compare function for tObject sort. 
int _CDECL_ sort_func(const sort_item *a, const sort_item *b)
{
	fix xDeltaDist;
	tObject *objAP, *objBP;

xDeltaDist = a->dist - b->dist;
objAP = gameData.objs.objects + a->nObject;
objBP = gameData.objs.objects + b->nObject;

if (abs (xDeltaDist) < (objAP->size + objBP->size)) {		//same position
	//these two objects are in the same position.  see if one is a fireball
	//or laser or something that should plot on top.  Don't do this for
	//the afterburner blobs, though.

	if ((objAP->nType == OBJ_WEAPON) || 
		 ((objAP->nType == OBJ_FIREBALL) && (objAP->id != VCLIP_AFTERBURNER_BLOB))) {
		if (!((objBP->nType == OBJ_WEAPON) || (objBP->nType == OBJ_FIREBALL)))
			return -1;	//a is weapon, b is not, so say a is closer
		}
	else {
		if (objBP->nType == OBJ_WEAPON || (objBP->nType == OBJ_FIREBALL && objBP->id != VCLIP_AFTERBURNER_BLOB))
			return 1;	//b is weapon, a is not, so say a is farther
		}
	//no special case, fall through to normal return
	}
return xDeltaDist;	//return distance
}

//------------------------------------------------------------------------------

void BuildObjectLists(int n_segs)
{
	int nn;

	for (nn=0;nn<MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS;nn++)
		nRenderObjList [nn][0] = -1;

	for (nn=0;nn<n_segs;nn++) {
		short nSegment;
		nSegment = gameData.render.mine.nRenderList [nn];
		if (nSegment != -1) {
			int nObject;
			tObject *objP;
			for (nObject = gameData.segs.segments [nSegment].objects; nObject != -1; nObject = objP->next) {
				int new_segnum, did_migrate, list_pos;
				objP = gameData.objs.objects+nObject;

				Assert( objP->nSegment == nSegment);

				if (objP->flags & OF_ATTACHED)
					continue;		//ignore this tObject

				new_segnum = nSegment;
				list_pos = nn;

				if (objP->nType != OBJ_CNTRLCEN && !(objP->nType==OBJ_ROBOT && objP->id==65))		//don't migrate controlcen
				do {
					segmasks m;
					did_migrate = 0;
					m = GetSegMasks(&objP->position.vPos, new_segnum, objP->size);
					if (m.sideMask) {
						short sn, sf;
						for (sn=0, sf=1;sn<6;sn++, sf<<=1)
							if (m.sideMask & sf) {
								tSegment *seg = gameData.segs.segments+new_segnum;
		
								if (WALL_IS_DOORWAY (seg, sn, NULL) & WID_FLY_FLAG) {		//can explosion migrate through
									int child = seg->children [sn];
									int checknp;
		
									for (checknp=list_pos;checknp--;)
										if (gameData.render.mine.nRenderList [checknp] == child) {
											new_segnum = child;
											list_pos = checknp;
											did_migrate = 1;
										}
								}
							}
					}
	
				} while (0);	//while (did_migrate);
				AddObjectToSegList(nObject, list_pos);
			}
		}
	}

	//now that there's a list for each tSegment, sort the items in those lists
	for (nn=0;nn<n_segs;nn++) {
		int nSegment;

		nSegment = gameData.render.mine.nRenderList [nn];

		if (nSegment != -1) {
			int t, lookn, i, n;

			//first count the number of gameData.objs.objects & copy into sort list

			lookn = nn;
			i = nSortItems = 0;
			while ((t=nRenderObjList [lookn][i++])!=-1)
				if (t<0)
					{lookn = -t; i=0;}
				else
					if (nSortItems < SORT_LIST_SIZE-1) {		//add if room
						sortList [nSortItems].nObject = t;
						//NOTE: maybe use depth, not dist - quicker computation
						sortList [nSortItems].dist = VmVecDistQuick(&gameData.objs.objects [t].position.vPos, &gameData.render.mine.viewerEye);
						nSortItems++;
					}
					else {			//no room for tObject
						int ii;

						#ifdef _DEBUG
						FILE *tfile=fopen("sortlist.out", "wt");

						//I find this strange, so I'm going to write out
						//some information to look at later
						if (tfile) {
							for (ii=0;ii<SORT_LIST_SIZE;ii++) {
								int nObject = sortList [ii].nObject;

								fprintf(tfile, "Obj %3d  Type = %2d  Id = %2d  Dist = %08x  Segnum = %3d\n", 
									nObject, gameData.objs.objects [nObject].nType, gameData.objs.objects [nObject].id, sortList [ii].dist, gameData.objs.objects [nObject].nSegment);
							}
							fclose(tfile);
						}
						#endif

						Int3();	//Get Matt!!!

						//Now try to find a place for this tObject by getting rid
						//of an tObject we don't care about

						for (ii=0;ii<SORT_LIST_SIZE;ii++) {
							int nObject = sortList [ii].nObject;
							tObject *objP = &gameData.objs.objects [nObject];
							int nType = objP->nType;

							//replace debris & fireballs
							if (nType == OBJ_DEBRIS || nType == OBJ_FIREBALL) {
								fix dist = VmVecDistQuick(&gameData.objs.objects [t].position.vPos, &gameData.render.mine.viewerEye);

								//don't replace same kind of tObject unless new 
								//one is closer

								if (gameData.objs.objects [t].nType != nType || dist < sortList [ii].dist) {
									sortList [ii].nObject = t;
									sortList [ii].dist = dist;
									break;
								}
							}
						}
						Int3();	//still couldn't find a slot
					}


			//now call qsort
		#if defined(__WATCOMC__)
			qsort(sortList, nSortItems, sizeof(*sortList), 
				   sort_func);
		#else
			qsort(sortList, nSortItems, sizeof(*sortList), 
					(int (_CDECL_ *)(const void*, const void*))sort_func);
		#endif	

			//now copy back into list

			lookn = nn;
			i = 0;
			n = nSortItems;
			while ((t=nRenderObjList [lookn][i])!=-1 && n>0)
				if (t<0)
					{lookn = -t; i=0;}
				else
					nRenderObjList [lookn][i++] = sortList [--n].nObject;
			nRenderObjList [lookn][i] = -1;	//mark (possibly new) end
		}
	}
}

//------------------------------------------------------------------------------

#define	PP_DELTAZ	-i2f(30)
#define	PP_DELTAY	i2f(10)

tFlightPath	externalView;

//------------------------------------------------------------------------------

void ResetFlightPath (tFlightPath *pPath, int nSize, int nFPS)
{
pPath->nSize = (nSize < 0) ? MAX_PATH_POINTS : nSize;
pPath->tRefresh = (time_t) (1000 / ((nFPS < 0) ? 40 : nFPS));
pPath->nStart =
pPath->nEnd = 0;
pPath->pPos = NULL;
pPath->tUpdate = -1;
}

//------------------------------------------------------------------------------

void SetPathPoint (tFlightPath *pPath, tObject *objP)
{
	time_t	t = SDL_GetTicks () - pPath->tUpdate;

if (pPath->nSize && ((pPath->tUpdate < 0) || (t >= pPath->tRefresh))) {
	pPath->tUpdate = t;
//	h = pPath->nEnd;
	pPath->nEnd = (pPath->nEnd + 1) % pPath->nSize;
	pPath->path [pPath->nEnd].vOrgPos = objP->position.vPos;
	pPath->path [pPath->nEnd].vPos = objP->position.vPos;
	pPath->path [pPath->nEnd].mOrient = objP->position.mOrient;
	VmVecScaleInc (&pPath->path [pPath->nEnd].vPos, &objP->position.mOrient.fVec, 0);
	VmVecScaleInc (&pPath->path [pPath->nEnd].vPos, &objP->position.mOrient.uVec, 0);
//	if (!memcmp (pPath->path + h, pPath->path + pPath->nEnd, sizeof (tMovementPath)))
//		pPath->nEnd = h;
//	else 
	if (pPath->nEnd == pPath->nStart)
		pPath->nStart = (pPath->nStart + 1) % pPath->nSize;
	}
}

//------------------------------------------------------------------------------

tPathPoint *GetPathPoint (tFlightPath *pPath)
{
	vmsVector		*p = &pPath->path [pPath->nEnd].vPos;
	int				i;

if (pPath->nStart == pPath->nEnd) {
	pPath->pPos = NULL;
	return NULL;
	}
i = pPath->nEnd;
do {
	if (!i)
		i = pPath->nSize;
	i--;
	if (VmVecDist (&pPath->path [i].vPos, p) >= i2f (15))
		break;
	}
while (i != pPath->nStart);
return pPath->pPos = pPath->path + i;
}

//------------------------------------------------------------------------------

void GetViewPoint (void)
{
	tPathPoint		*p = GetPathPoint (&externalView);

if (!p)
	VmVecScaleInc (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient.fVec, PP_DELTAZ);
else {
	gameData.render.mine.viewerEye = p->vPos;
	VmVecScaleInc (&gameData.render.mine.viewerEye, &p->mOrient.fVec, PP_DELTAZ * 2 / 3);
	VmVecScaleInc (&gameData.render.mine.viewerEye, &p->mOrient.uVec, PP_DELTAY * 2 / 3);
	}
}

//------------------------------------------------------------------------------

extern kcItem kcMouse [];

inline int ZoomKeyPressed (void)
{
#if 1
	int	v;

return keyd_pressed [kcKeyboard [52].value] || keyd_pressed [kcKeyboard [53].value] ||
		 (((v = kcMouse [30].value) < 255) && MouseButtonState (v));
#else
return (Controls [0].zoomDownCount > 0);
#endif
}

//------------------------------------------------------------------------------

extern int criticalErrorCounterPtr, nDescentCriticalError;

extern int Num_tmaps_drawn;
extern int nTotalPixels;
//--unused-- int Total_num_tmaps_drawn=0;

void StartLightingFrame (tObject *viewer);

//------------------------------------------------------------------------------

void RenderShadowQuad (int bWhite)
{
	static GLdouble shadowHue [2][4] = {{0, 0, 0, 0.6},{0, 0, 0, 1}};

glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
glLoadIdentity ();
glMatrixMode (GL_PROJECTION);
glPushMatrix ();
glLoadIdentity ();
glOrtho (0, 1, 1, 0, 0, 1);
glDisable (GL_DEPTH_TEST);
glEnable (GL_STENCIL_TEST);
glDepthMask (0);
if (gameStates.render.nShadowBlurPass)
	glDisable (GL_BLEND);
else
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDisable (GL_TEXTURE_2D);
glColor4dv (shadowHue [gameStates.render.nShadowBlurPass]);// / fDist);
glBegin (GL_QUADS);
glVertex2f (0,0);
glVertex2f (1,0);
glVertex2f (1,1);
glVertex2f (0,1);
glEnd ();
glEnable (GL_DEPTH_TEST);
glDisable (GL_STENCIL_TEST);
glDepthMask (1);
glPopMatrix ();
glMatrixMode (GL_MODELVIEW);
glPopMatrix ();
}

//------------------------------------------------------------------------------

#define STB_SIZE_X	2048
#define STB_SIZE_Y	2048

grsBitmap	shadowBuf;
ubyte			shadowTexBuf [STB_SIZE_X * STB_SIZE_Y * 4];
static int	bHaveShadowBuf = 0;

void CreateShadowTexture (void)
{
	GLint	i;

if (!bHaveShadowBuf) {
	memset (&shadowBuf, 0, sizeof (shadowBuf));
	shadowBuf.bm_props.w = STB_SIZE_X;
	shadowBuf.bm_props.h = STB_SIZE_Y;
	shadowBuf.bm_props.flags = (char) BM_FLAG_TGA;
	shadowBuf.bm_texBuf = shadowTexBuf;
	OglLoadBmTextureM (&shadowBuf, 0, -1, 0, NULL);
	bHaveShadowBuf = 1;
	}
#if 1
//glStencilFunc (GL_EQUAL, 0, ~0);
//RenderShadowQuad (1);
#	if 0
glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 0, grdCurCanv->cv_bitmap.bm_props.h - 128, 128, 128, 0);
#	else
glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 0, 0,
						grdCurCanv->cv_bitmap.bm_props.w, 
						grdCurCanv->cv_bitmap.bm_props.h, 0);
#	endif
#else
glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, 128, 128);
#endif
i = glGetError ();
}

//------------------------------------------------------------------------------

GLhandleARB shadowProg = 0;
GLhandleARB shadowFS = 0; 
GLhandleARB shadowVS = 0; 

#if DBG_SHADERS

char *pszShadowFS = "shadows.frag";
char *pszShadowVS = "shadows.vert";

#else

char *pszShadowFS = "shadows.frag";
char *pszShadowVS = "shadows.vert";

#endif

void RenderShadowTexture (void)
{
if (!(shadowProg ||
	   (CreateShaderProg (&shadowProg) &&
	    CreateShaderFunc (&shadowProg, &shadowFS, &shadowVS, pszShadowFS, pszShadowVS, 1) &&
	    LinkShaderProg (&shadowProg))))
	return;
glMatrixMode (GL_MODELVIEW);
glPushMatrix ();
glLoadIdentity ();
glMatrixMode (GL_PROJECTION);
glPushMatrix ();
glLoadIdentity ();
glOrtho (0, 1, 1, 0, 0, 1);
glDisable (GL_DEPTH_TEST);
glDepthMask (0);
#if 1
glDisable (GL_BLEND);
#else
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
glEnable (GL_TEXTURE_2D);
OglActiveTexture (GL_TEXTURE0_ARB, 0);
if (OglBindBmTex (&shadowBuf, 0, 0))
	return;
glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#if 0
glUseProgramObject (shadowProg);
glUniform1i (glGetUniformLocation (shadowProg, "shadowTex"), 0);
#endif
glBegin (GL_QUADS);
glTexCoord2d (0,0);
glVertex2d (0,0);
glTexCoord2d (1,0);
glVertex2d (0.5,0);
glTexCoord2d (1,-1);
glVertex2d (0.5,0.5);
glTexCoord2d (0,-1);
glVertex2d (0,0.5);
glEnd ();
glUseProgramObject (0);
glEnable (GL_DEPTH_TEST);
glDepthMask (1);
glPopMatrix ();
glMatrixMode (GL_MODELVIEW);
glPopMatrix ();
}

//------------------------------------------------------------------------------

int RenderShadowMap (tDynLight *pLight)
{
	tCamera	*pc;

if (pLight->shadow.nFrame == gameData.render.shadows.nFrame)
	return 0;
if (gameData.render.shadows.nShadowMaps == MAX_SHADOW_MAPS)
	return 0;
pLight->shadow.nFrame = !pLight->shadow.nFrame;
gameStates.render.nShadowPass = 2;
pc = gameData.render.shadows.shadowMaps + gameData.render.shadows.nShadowMaps++;
CreateCamera (pc, pLight->nSegment, pLight->nSide, pLight->nSegment, pLight->nSide, NULL, 1, 0);
RenderCamera (pc);
gameStates.render.nShadowPass = 2;
return 1;
}

//------------------------------------------------------------------------------
//The following code is an attempt to find all objects that cast a shadow visible
//to the player. To accomplish that, for each robot the line of sight to each
//tSegment visible to the tPlayer is computed. If there is a los to any of these 
//segments, the tObject's shadow is rendered. Far from perfect solution though. :P

void RenderObjectShadows (void)
{
	tObject		*objP = gameData.objs.objects;
	int			i, j, bSee;
	tObject		fakePlayerPos = *gameData.objs.viewer;

for (i = 0; i <= gameData.objs.nLastObject; i++, objP++)
	if (objP == gameData.objs.console)
		RenderObject (objP, 0, 0);
	else if ((objP->nType == OBJ_PLAYER) || 
				(gameOpts->render.shadows.bRobots && (objP->nType == OBJ_ROBOT))) {
		for (j = gameData.render.mine.nRenderSegs; j--;) {
			fakePlayerPos.nSegment = gameData.render.mine.nRenderList [j];
			COMPUTE_SEGMENT_CENTER_I (&fakePlayerPos.position.vPos, fakePlayerPos.nSegment);
			bSee = ObjectToObjectVisibility (objP, &fakePlayerPos, FQ_TRANSWALL);
			if (bSee) {
				RenderObject (objP, 0, 0);
				break;
				}
			}
		}
}

//------------------------------------------------------------------------------

void DestroyShadowMaps (void)
{
	tCamera	*pc;

for (pc = gameData.render.shadows.shadowMaps; gameData.render.shadows.nShadowMaps; gameData.render.shadows.nShadowMaps--, pc++)
	DestroyCamera (pc);
}

//------------------------------------------------------------------------------

void ApplyShadowMaps (short nStartSeg, fix nEyeOffset, int nWindow)
{	
	static float mTexBiasf [] = {
		0.5f, 0.0f, 0.0f, 0.0f, 
		0.0f, 0.5f, 0.0f, 0.0f, 
		0.0f, 0.0f, 0.5f, 0.0f, 
		0.5f, 0.5f, 0.5f, 1.0f};

	static float mPlanef [] = {
		1.0f, 0.0f, 0.0f, 0.0f, 
		0.0f, 1.0f, 0.0f, 0.0f, 
		0.0f, 0.0f, 1.0f, 0.0f, 
		0.0f, 0.0f, 0.0f, 1.0f};

	static GLenum nTexCoord [] = {GL_S, GL_T, GL_R, GL_Q};

	float mProjectionf [16];
	float mModelViewf [16];

	int			i;
	tCamera		*pc;

#if 1
OglActiveTexture (GL_TEXTURE0_ARB, 0);
glEnable (GL_TEXTURE_2D); 

glEnable (GL_TEXTURE_GEN_S);
glEnable (GL_TEXTURE_GEN_T);
glEnable (GL_TEXTURE_GEN_R);
glEnable (GL_TEXTURE_GEN_Q);

for (i = 0; i < 4; i++)
	glTexGeni (nTexCoord [i], GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
for (i = 0; i < 4; i++)
	glTexGenfv (nTexCoord [i], GL_EYE_PLANE, mPlanef + 4 * i);

glGetFloatv (GL_PROJECTION_MATRIX, mProjectionf);
glMatrixMode (GL_TEXTURE);
for (i = 0, pc = gameData.render.shadows.shadowMaps; i < 1/*gameData.render.shadows.nShadowMaps*/; i++) {
	glBindTexture (GL_TEXTURE_2D, pc->fb.texId);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
	glLoadMatrixf (mTexBiasf);
	glMultMatrixf (mProjectionf);
	glMultMatrixf (OOF_MatVms2Gl (mModelViewf, &pc->objP->position.mOrient));
	}
glMatrixMode (GL_MODELVIEW);
#endif
RenderMine (nStartSeg, nEyeOffset, nWindow);
#if 1
glMatrixMode (GL_TEXTURE);
glLoadIdentity ();
glMatrixMode (GL_MODELVIEW);
glDisable (GL_TEXTURE_GEN_S);
glDisable (GL_TEXTURE_GEN_T);
glDisable (GL_TEXTURE_GEN_R);
glDisable (GL_TEXTURE_GEN_Q);
OglActiveTexture (GL_TEXTURE0_ARB, 0);		
glDisable (GL_TEXTURE_2D);
#endif
DestroyShadowMaps ();
}

//------------------------------------------------------------------------------

int GatherShadowLightSources (void)
{
	tObject			*objP = gameData.objs.objects;
	int				h, i, j, k, n, m = gameOpts->render.shadows.nLights;
	short				*pnl;
//	tDynLight		*pl;
	tShaderLight	*psl;
	vmsVector		vLightDir;

psl = gameData.render.lights.dynamic.shader.lights;
for (h = 0, i = gameData.render.lights.dynamic.nLights; i; i--, psl++)
	psl->bShadow =
	psl->bExclusive = 0;
for (h = 0; h <= gameData.objs.nLastObject + 1; h++, objP++) {
	if (!bObjectRendered [h])
		continue;
	pnl = gameData.render.lights.dynamic.nNearestSegLights + objP->nSegment * MAX_NEAREST_LIGHTS;
	k = h * MAX_SHADOW_LIGHTS;
	for (i = n = 0; (n < m) && (*pnl >= 0); i++, pnl++) {
		psl = gameData.render.lights.dynamic.shader.lights + *pnl;
		if (!psl->bState)
			continue;
		if (!CanSeePoint (objP, &objP->position.vPos, &psl->vPos, objP->nSegment))
			continue;
		VmVecSub (&vLightDir, &objP->position.vPos, &psl->vPos);
		VmVecNormalize (&vLightDir);
		if (n) {
			for (j = 0; j < n; j++)
				if (abs (VmVecDot (&vLightDir, gameData.render.shadows.vLightDir + j)) > 2 * F1_0 / 3) // 60 deg
					break;
			if (j < n)
				continue;
			}
		gameData.render.shadows.vLightDir [n] = vLightDir;
		gameData.render.shadows.objLights [k + n++] = *pnl;
		psl->bShadow = 1;
		}
	gameData.render.shadows.objLights [k + n] = -1;
	}
psl = gameData.render.lights.dynamic.shader.lights;
for (h = 0, i = gameData.render.lights.dynamic.nLights; i; i--, psl++)
	if (psl->bShadow)
		h++;
return h;
}

//------------------------------------------------------------------------------

void SetRenderView (fix nEyeOffset, short *pnStartSeg)
{
	static int bStopZoom;
	short nStartSeg;

gameData.render.mine.viewerEye = gameData.objs.viewer->position.vPos;
if (nEyeOffset) {
	VmVecScaleInc (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient.rVec, nEyeOffset);
	}
#ifdef EDITOR
if (gameStates.app.nFunctionMode == FMODE_EDITOR)
	gameData.render.mine.viewerEye = gameData.objs.viewer->position.vPos;
#endif

externalView.pPos = NULL;
if (gameStates.render.cameras.bActive) {
	nStartSeg = gameData.objs.viewer->nSegment;
	G3SetViewMatrix (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient, gameStates.render.xZoom);
	}
else {
	nStartSeg = FindSegByPoint (&gameData.render.mine.viewerEye, gameData.objs.viewer->nSegment);
	if (nStartSeg == -1)
		nStartSeg = gameData.objs.viewer->nSegment;
	if ((gameData.objs.viewer == gameData.objs.console) && viewInfo.bUsePlayerHeadAngles) {
		vmsMatrix mHead, mView;
		VmAngles2Matrix (&mHead, &viewInfo.playerHeadAngles);
		VmMatMul (&mView, &gameData.objs.viewer->position.mOrient, &mHead);
		G3SetViewMatrix (&gameData.render.mine.viewerEye, &mView, gameStates.render.xZoom);
		}
	else if (gameStates.render.bRearView && (gameData.objs.viewer == gameData.objs.console)) {
		vmsMatrix mHead, mView;
		viewInfo.playerHeadAngles.p = 
		viewInfo.playerHeadAngles.b = 0;
		viewInfo.playerHeadAngles.h = 0x7fff;
		VmAngles2Matrix (&mHead, &viewInfo.playerHeadAngles);
		VmMatMul (&mView, &gameData.objs.viewer->position.mOrient, &mHead);
		G3SetViewMatrix (&gameData.render.mine.viewerEye, &mView, FixDiv(gameStates.render.xZoom, gameStates.render.nZoomFactor));
		} 
	else if (!IsMultiGame || gameStates.app.bHaveExtraGameInfo [1]) {
		gameStates.render.nMinZoomFactor = (fix) (F1_0 * gameStates.render.glAspect); //(((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) ? 2 * F1_0  / 3 : F1_0) * glAspect);
		gameStates.render.nMaxZoomFactor = gameStates.render.nMinZoomFactor * 5;
		if ((gameData.weapons.nPrimary != VULCAN_INDEX) && (gameData.weapons.nPrimary != GAUSS_INDEX))
			gameStates.render.nZoomFactor = gameStates.render.nMinZoomFactor; //(fix) (((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) ? 2 * F1_0  / 3 : F1_0) * glAspect);
		else {
			switch (extraGameInfo [IsMultiGame].nZoomMode) {
				case 0:
					break;
				case 1:
					if (ZoomKeyPressed ()) {
						if (!bStopZoom) {
							gameStates.render.nZoomFactor = (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) ? (gameStates.render.nZoomFactor * 7) / 5 : (gameStates.render.nZoomFactor * 5) / 3;
							if (gameStates.render.nZoomFactor > gameStates.render.nMaxZoomFactor) 
								gameStates.render.nZoomFactor = gameStates.render.nMinZoomFactor;
							bStopZoom = 1;
							} 
						}
					else {
						bStopZoom = 0;
						}
					break;
				case 2:
					if (ZoomKeyPressed ()) {
						gameStates.render.nZoomFactor += gameData.time.xFrame * 4;
						if (gameStates.render.nZoomFactor > gameStates.render.nMaxZoomFactor) 
							gameStates.render.nZoomFactor = gameStates.render.nMaxZoomFactor;
						} 
					else {
						gameStates.render.nZoomFactor -= gameData.time.xFrame * 4;
						if (gameStates.render.nZoomFactor < gameStates.render.nMinZoomFactor) 
							gameStates.render.nZoomFactor = gameStates.render.nMinZoomFactor;
						}
					break;
				}
			}
		if ((gameData.objs.viewer == gameData.objs.console) && 
#ifdef _DEBUG
			 gameStates.render.bExternalView) {
#else		
			 gameStates.render.bExternalView && (!IsMultiGame || IsCoopGame || EGI_FLAG (bEnableCheats, 0, 0, 0))) {
#endif			 	
			SetPathPoint (&externalView, gameData.objs.viewer);
			GetViewPoint ();
			G3SetViewMatrix (&gameData.render.mine.viewerEye,
								  externalView.pPos ? &externalView.pPos->mOrient : &gameData.objs.viewer->position.mOrient, 
								  gameStates.render.xZoom);
			}
		else
			G3SetViewMatrix (&gameData.render.mine.viewerEye, 
								  &gameData.objs.viewer->position.mOrient, FixDiv (gameStates.render.xZoom, 
								  gameStates.render.nZoomFactor));
		}
	else
		G3SetViewMatrix (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient, gameStates.render.xZoom);
	}
if (pnStartSeg)
	*pnStartSeg = nStartSeg;
}

//------------------------------------------------------------------------------

void RenderFastShadows (fix nEyeOffset, int nWindow, short nStartSeg)
{
#if 0//OOF_TEST_CUBE
#	if 1
for (bShadowTest = 1; bShadowTest >= 0; bShadowTest--) 
#	else
for (bShadowTest = 0; bShadowTest < 2; bShadowTest++) 
#	endif
#endif
	{
	gameStates.render.nShadowPass = 2;
	OglStartFrame (0, 0);
	gameData.render.shadows.nFrame = !gameData.render.shadows.nFrame;
	//RenderObjectShadows ();
	RenderMine (nStartSeg, nEyeOffset, nWindow);
	}
#ifdef _DEBUG
if (!bShadowTest) 
#endif
	{
	gameStates.render.nShadowPass = 3;
	OglStartFrame (0, 0);
	if	(gameStates.render.bShadowMaps) {
#ifdef _DEBUG
		if (gameStates.render.bExternalView)
#else		
		if (gameStates.render.bExternalView && (!IsMultiGame || IsCoopGame || EGI_FLAG (bEnableCheats, 0, 0, 0)))
#endif			 	
			G3SetViewMatrix (&gameData.render.mine.viewerEye, externalView.pPos ? &externalView.pPos->mOrient : &gameData.objs.viewer->position.mOrient, gameStates.render.xZoom);
		else
			G3SetViewMatrix (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient, FixDiv (gameStates.render.xZoom, gameStates.render.nZoomFactor));
		ApplyShadowMaps (nStartSeg, nEyeOffset, nWindow);
		}
	else {
		RenderShadowQuad (0);
		}
	}
}

//------------------------------------------------------------------------------

void RenderNeatShadows (fix nEyeOffset, int nWindow, short nStartSeg)
{
	short				i;
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights;

gameData.render.shadows.nLights = GatherShadowLightSources ();
for (i = 0; i < gameData.render.lights.dynamic.nLights; i++, psl++) {
	if (!psl->bShadow)
		continue;
	gameData.render.shadows.pLight = psl;
	psl->bExclusive = 1;
#if 1
	gameStates.render.nShadowPass = 2;
	OglStartFrame (0, 0);
	memcpy (&gameData.render.shadows.vLightPos, psl->pos + 1, sizeof (tOOF_vector));
	gameData.render.shadows.nFrame = !gameData.render.shadows.nFrame;
	RenderMine (nStartSeg, nEyeOffset, nWindow);
#endif
	gameStates.render.nShadowPass = 3;
	OglStartFrame (0, 0);
	gameData.render.shadows.nFrame = !gameData.render.shadows.nFrame;
	RenderMine (nStartSeg, nEyeOffset, nWindow);
	psl->bExclusive = 0;
	}
#if 0
gameStates.render.nShadowPass = 4;
RenderMine (nStartSeg, nEyeOffset, nWindow);
#endif
}

//------------------------------------------------------------------------------

void RenderFrame (fix nEyeOffset, int nWindow)
{
	short nStartSeg;

if (gameStates.app.bEndLevelSequence) {
	RenderEndLevelFrame (nEyeOffset, nWindow);
	gameData.app.nFrameCount++;
	return;
	}
#ifdef NEWDEMO
if ((gameData.demo.nState == ND_STATE_RECORDING) && (nEyeOffset >= 0))	{
   if (!gameStates.render.nRenderingType)
   	NDRecordStartFrame (gameData.app.nFrameCount, gameData.time.xFrame);
   if (gameStates.render.nRenderingType != 255)
   	NDRecordViewerObject (gameData.objs.viewer);
	}
#endif
  
StartLightingFrame (gameData.objs.viewer);		//this is for ugly light-smoothing hack
gameStates.ogl.bEnableScissor = !gameStates.render.cameras.bActive && nWindow;
G3StartFrame (0, !(nWindow || gameStates.render.cameras.bActive));
SetRenderView (nEyeOffset, &nStartSeg);
if (nClearWindow == 1) {
	if (!nClearWindowColor)
		nClearWindowColor = BLACK_RGBA;	//BM_XRGB(31, 15, 7);
	GrClearCanvas (nClearWindowColor);
	}
#ifdef _DEBUG
if (bShowOnlyCurSide)
	GrClearCanvas (nClearWindowColor);
#endif
#if SHADOWS
if (!gameStates.render.bHaveStencilBuffer)
	extraGameInfo [0].bShadows = 
	extraGameInfo [1].bShadows = 0;
if (SHOW_SHADOWS && 
	 !(nWindow || gameStates.render.cameras.bActive || gameStates.render.automap.bDisplay)) {
	if (!gameStates.render.bShadowMaps) {
		gameStates.render.nShadowPass = 1;
#if SOFT_SHADOWS
		if (gameOpts->render.shadows.bSoft = 1)
			gameStates.render.nShadowBlurPass = 1;
#endif
		OglStartFrame (0, 0);
#if SOFT_SHADOWS
		OGL_VIEWPORT (grdCurCanv->cv_bitmap.bm_props.x, grdCurCanv->cv_bitmap.bm_props.y, 128, 128);
#endif
		RenderMine (nStartSeg, nEyeOffset, nWindow);
#ifdef RELEASE
		RenderFastShadows (nEyeOffset, nWindow, nStartSeg);
#else
		if (FAST_SHADOWS)
			RenderFastShadows (nEyeOffset, nWindow, nStartSeg);
		else
			RenderNeatShadows (nEyeOffset, nWindow, nStartSeg);
#endif
#if SOFT_SHADOWS
		if (gameOpts->render.shadows.bSoft) {
			CreateShadowTexture ();
			gameStates.render.nShadowBlurPass = 2;
			gameStates.render.nShadowPass = 0;
#if 1
			OglStartFrame (0, 1);
			SetRenderView (nEyeOffset, &nStartSeg);
			RenderMine (nStartSeg, nEyeOffset, nWindow);
#endif
			RenderShadowTexture ();
			}
#endif
		RenderSmoke ();
		}
	}
else 
#endif
	{
	if (gameStates.render.nRenderPass < 0)
		RenderMine (nStartSeg, nEyeOffset, nWindow);
	else {
		for (gameStates.render.nRenderPass = 0;
			gameStates.render.nRenderPass < 2;
			gameStates.render.nRenderPass++) {
			OglStartFrame (0, 1);
			RenderMine (nStartSeg, nEyeOffset, nWindow);
			}
		}
	if (!nWindow) {
		RenderSmoke ();
		}
	}
gameStates.render.nShadowPass = 0;
if (viewInfo.bUsePlayerHeadAngles) 
	Draw3DReticle (nEyeOffset);
G3EndFrame ();
}

//------------------------------------------------------------------------------

int nFirstTerminalSeg;

void UpdateRenderedData(int nWindow, tObject *viewer, int rearViewFlag, int user)
{
	Assert(nWindow < MAX_RENDERED_WINDOWS);
	windowRenderedData [nWindow].frame = gameData.app.nFrameCount;
	windowRenderedData [nWindow].viewer = viewer;
	windowRenderedData [nWindow].rearView = rearViewFlag;
	windowRenderedData [nWindow].user = user;
}

//------------------------------------------------------------------------------
// sort segments by distance from tPlayer tSegment, where distance is the minimal
// number of segments that have to be traversed to reach a tSegment, starting at
// the tPlayer tSegment. If the entire mine is to be rendered, segments will then
// be rendered in reverse order (furthest first), to achieve proper rendering of
// transparent walls layered in a view axis.

static short segList [MAX_SEGMENTS_D2X];
static short segDist [MAX_SEGMENTS_D2X];
#if 0
static char  bRenderSegObjs [MAX_SEGMENTS_D2X];
static char  bRenderObjs [MAX_OBJECTS_D2X];
static int nRenderObjs;
#endif
static short nSegListSize;

void BuildSegList (void)
{
	int		h, i, j, segNum, childNum, sideNum, nDist = 0;
	tSegment	*segP;

memset (segDist, 0xFF, sizeof (segDist));
segNum = gameData.objs.objects [LOCALPLAYER.nObject].nSegment;
i = j = 0;
segDist [segNum] = 0;
segList [j++] = segNum;
do {
	nDist++;
	for (h = i, i = j; h < i; h++) {
		segNum = segList [h];
		segP = gameData.segs.segments + segNum;
		for (sideNum = 0; sideNum < 6; sideNum++) {
			childNum = segP->children [sideNum];
			if (childNum < 0)
				continue;
			if (segDist [childNum] >= 0)
				continue;
			segDist [childNum] = nDist;
			segList [j++] = childNum;
			}
		}
	} while (i < j);
nSegListSize = j;
}

//------------------------------------------------------------------------------
//build a list of segments to be rendered
//fills in gameData.render.mine.nRenderList & gameData.render.mine.nRenderSegs

typedef struct tSegZRef {
	fix	z;
	//fix	d;
	short	nSegment;
} tSegZRef;

static tSegZRef segZRef [MAX_SEGMENTS_D2X];

void QSortSegZRef (short left, short right)
{
	tSegZRef	m = segZRef  [(left + right) / 2];
	tSegZRef	h;
	short		l = left, 
				r = right;
do {
	while ((segZRef [l].z > m.z))// || ((segZRef [l].z == m.z) && (segZRef [l].d > m.d)))
		l++;
	while ((segZRef [r].z < m.z))// || ((segZRef [r].z == m.z) && (segZRef [r].d < m.d)))
		r--;
	if (l <= r) {
		if (l < r) {
			h = segZRef [l];
			segZRef [l] = segZRef [r];
			segZRef [r] = h;
			}
		l++;
		r--;
		}
	}
while (l < r);
if (l < right)
	QSortSegZRef (l, right);
if (left < r)
	QSortSegZRef (left, r);
}

//------------------------------------------------------------------------------

void SortRenderSegs (void)
{
	int			i;
	vmsVector	v;

if (gameData.render.mine.nRenderSegs < 2)
	return;
for (i = 0; i < gameData.render.mine.nRenderSegs; i++) {
	COMPUTE_SEGMENT_CENTER_I (&v, gameData.render.mine.nRenderList [i]);
	G3TransformPoint (&v, &v, 0);
	segZRef [i].z = v.p.z;
	segZRef [i].nSegment = gameData.render.mine.nRenderList [i];
	}
QSortSegZRef (0, gameData.render.mine.nRenderSegs - 1);
for (i = 0; i < gameData.render.mine.nRenderSegs; i++)
	gameData.render.mine.nRenderList [i] = segZRef [i].nSegment;
}

//------------------------------------------------------------------------------

void BuildSegmentList (short nStartSeg, int nWindow)
{
	int		lCnt, sCnt, eCnt, wid, nSide;
	int		l, i;
	short		c;
	short		nChild;
	short		*sv;
	sbyte		*s2v;
	ubyte		andCodes;
	int		bRotated, nSegment;
	window	*checkWinP;
	short		childList [MAX_SIDES_PER_SEGMENT];		//list of ordered sides to process
	int		nChildren, bCheckBehind;					//how many sides in childList
	tSegment	*segP;

bCheckBehind = !SHOW_SHADOWS || (gameStates.render.nShadowPass == 1);
if (!++gameData.render.mine.nVisited) {
	memset (gameData.render.mine.bVisited, 0, sizeof (gameData.render.mine.bVisited));
	gameData.render.mine.nVisited = 1;
	}
memset (nRenderPos, -1, sizeof (nRenderPos [0]) * (gameData.segs.nSegments));
//memset(no_renderFlag, 0, sizeof(no_renderFlag [0])*(MAX_RENDER_SEGS);
memset (gameData.render.mine.nProcessed, 0, sizeof (gameData.render.mine.nProcessed));
memset (gameData.render.mine.nRenderList, 0xff, sizeof (gameData.render.mine.nRenderList));

#ifdef _DEBUG
memset(visited2, 0, sizeof(visited2 [0])*(gameData.segs.nLastSegment+1));
#endif

if (gameStates.render.automap.bDisplay && gameOpts->render.automap.bTextured && !gameStates.render.automap.bRadar) {
	for (i = gameData.render.mine.nRenderSegs = 0; i < gameData.segs.nSegments; i++)
		if (gameStates.render.automap.bFull || bAutomapVisited [i]) {
			gameData.render.mine.nRenderList [gameData.render.mine.nRenderSegs++] = i;
			VISIT (i);
			}
	SortRenderSegs ();
	return;
	}

gameData.render.mine.nRenderList [0] = nStartSeg; 
gameData.render.mine.nSegDepth [0] = 0;
VISIT (nStartSeg);
nRenderPos [nStartSeg] = 0;
sCnt = 0;
lCnt = eCnt = 1;

#ifdef _DEBUG
if (bPreDrawSegs)
	RenderSegment (nStartSeg, nWindow);
#endif

renderWindows [0].left =
renderWindows [0].top = 0;
renderWindows [0].right = grdCurCanv->cv_bitmap.bm_props.w - 1;
renderWindows [0].bot = grdCurCanv->cv_bitmap.bm_props.h - 1;

//breadth-first renderer

//build list
for (l = 0; l < gameStates.render.detail.nRenderDepth; l++) {
	//while (sCnt < eCnt) {
	for (sCnt = 0; sCnt < eCnt; sCnt++) {
		if (gameData.render.mine.nProcessed [sCnt])
			continue;
		gameData.render.mine.nProcessed [sCnt] = 1;
		nSegment = gameData.render.mine.nRenderList [sCnt];
		checkWinP = renderWindows + sCnt;
#ifdef _DEBUG
		if (bDrawBoxes)
			DrawWindowBox (RED_RGBA, checkWinP->left, checkWinP->top, checkWinP->right, checkWinP->bot);
		if (nSegment == nDbgSeg)
			nSegment = nSegment;
#endif
		if (nSegment == -1) 
			continue;
		segP = gameData.segs.segments + nSegment;
		sv = segP->verts;
		bRotated = 0;
		//look at all sides of this tSegment.
		//tricky code to look at sides in correct order follows
		for (c = nChildren = 0; c < MAX_SIDES_PER_SEGMENT; c++) {		//build list of sides
			wid = WALL_IS_DOORWAY (segP, c, NULL);
			nChild = segP->children [c];
#ifdef _DEBUG
			if (nChild == nDbgSeg)
				nChild = nChild;
#endif
			if ((bWindowCheck || ((nChild > -1) && !VISITED (nChild))) && (wid & WID_RENDPAST_FLAG)) {
				if (bCheckBehind) {
					andCodes = 0xff;
					s2v = sideToVerts [c];
					if (!bRotated) {
						RotateVertexList (8, segP->verts);
						bRotated = 1;
						}
					for (i = 0; i < 4; i++)
						andCodes &= gameData.segs.points [sv [s2v [i]]].p3_codes;
					if (andCodes & CC_BEHIND) 
						continue;
					}
				childList [nChildren++] = c;
				}
			}
		//now order the sides in some magical way
#if 1
		if (bNewSegSorting && (nChildren > 1))
			SortSegChildren (segP, nChildren, childList);
#endif
		for (c = 0; c < nChildren; c++) {
			nSide = childList [c];
			nChild = segP->children [nSide];
#ifdef _DEBUG
			if (nChild == nDbgSeg)
				nChild = nChild;
#endif
			//if ( (bWindowCheck || !gameData.render.mine.bVisited [nChild])&& (WALL_IS_DOORWAY(segP, c))) {
			{
				if (bWindowCheck) {
					int i;
					ubyte codes_and_3d, codes_and_2d;
					short _x, _y, min_x = 32767, max_x = -32767, min_y = 32767, max_y = -32767;
					int bNotProjected = 0;	//a point wasn't projected
					if (bRotated < 2) {
						if (!bRotated)
							RotateVertexList (8, segP->verts);
						ProjectVertexList (8, segP->verts);
						bRotated = 2;
						}
					s2v = sideToVerts [nSide];
					for (i = 0, codes_and_3d = codes_and_2d = 0xff; i < 4; i++) {
						int p = sv [s2v [i]];
						g3sPoint *pnt = gameData.segs.points + p;
						if (!(pnt->p3_flags & PF_PROJECTED)) {
							bNotProjected = 1; 
							break;
							}
						_x = (short) f2i (pnt->p3_screen.x);
						_y = (short) f2i (pnt->p3_screen.y);
						codes_and_3d &= pnt->p3_codes;
						codes_and_2d &= code_window_point (_x, _y, checkWinP);
#ifdef _DEBUG
						if (bDrawEdges) {
							GrSetColorRGB (128, 0, 128, 255);
							gr_uline (pnt->p3_screen.x, pnt->p3_screen.y, 
								gameData.segs.points [segP->verts [sideToVerts [nSide][(i+1)%4]]].p3_screen.x, 
								gameData.segs.points [segP->verts [sideToVerts [nSide][(i+1)%4]]].p3_screen.y);
							}
#endif
						if (_x < min_x) 
							min_x = _x;
						if (_x > max_x) 
							max_x = _x;
						if (_y < min_y) 
							min_y = _y;
						if (_y > max_y) 
							max_y = _y;
						}
#ifdef _DEBUG
					if (bDrawBoxes)
						DrawWindowBox (WHITE_RGBA, min_x, min_y, max_x, max_y);
#endif
					if (bNotProjected || !(codes_and_3d || codes_and_2d)) {	//maybe add this tSegment
						int rp = nRenderPos [nChild];
						window *pNewWin = renderWindows + lCnt;

						if (bNotProjected) 
							*pNewWin = *checkWinP;
						else {
							pNewWin->left  = max (checkWinP->left, min_x);
							pNewWin->right = min (checkWinP->right, max_x);
							pNewWin->top   = max (checkWinP->top, min_y);
							pNewWin->bot   = min (checkWinP->bot, max_y);
						}
						//see if this segP already visited, and if so, does current window
						//expand the old window?
						if (rp != -1) {
							window *rwP = renderWindows + rp;
							if (pNewWin->left < rwP->left ||
								 pNewWin->top < rwP->top ||
								 pNewWin->right > rwP->right ||
								 pNewWin->bot > rwP->bot) {
								pNewWin->left  = min(pNewWin->left, rwP->left);
								pNewWin->right = max(pNewWin->right, rwP->right);
								pNewWin->top   = min(pNewWin->top, rwP->top);
								pNewWin->bot   = max(pNewWin->bot, rwP->bot);
								if (no_migrate_segs) {
									//no_renderFlag [lCnt] = 1;
									gameData.render.mine.nRenderList [lCnt] = -1;
									*rwP = *pNewWin;		//get updated window
									gameData.render.mine.nProcessed [rp] = 0;		//force reprocess
									goto no_add;
								}
								else
									gameData.render.mine.nRenderList [rp]=-1;
							}
							else 
								goto no_add;
						}
#ifdef _DEBUG
						if (bDrawBoxes)
							DrawWindowBox (5, pNewWin->left, pNewWin->top, pNewWin->right, pNewWin->bot);
#endif
						nRenderPos [nChild] = lCnt;
						gameData.render.mine.nRenderList [lCnt] = nChild;
						gameData.render.mine.nSegDepth [lCnt] = l;
						lCnt++;
						if (lCnt >= MAX_RENDER_SEGS) {
#if TRACE								
							//con_printf (CONDBG, "Too many segs in render list!!\n"); 
#endif
							goto done_list;
							}
						VISIT (nChild);

#ifdef _DEBUG
						if (bPreDrawSegs)
							RenderSegment (nChild, nWindow);
#endif
no_add:
;
					}
				}
				else {
					gameData.render.mine.nRenderList [lCnt] = nChild;
					gameData.render.mine.nSegDepth [lCnt] = l;
					lCnt++;
					if (lCnt >= MAX_RENDER_SEGS) {
						LogErr ("Too many segments in tSegment render list!!!\n"); 
						goto done_list;
						}
					VISIT (nChild);
				}
			}
		}
	}
	sCnt = eCnt;
	eCnt = lCnt;
}
done_list:

gameData.render.mine.lCntSave = lCnt;
gameData.render.mine.sCntSave = sCnt;
nFirstTerminalSeg = sCnt;
gameData.render.mine.nRenderSegs = lCnt;
}


/*
void gl_buildObject_list (void)
{
}
*/

//------------------------------------------------------------------------------

int GlRenderSegments (int nStartState, int nEndState, int nnRenderSegs, int nWindow)
{
for (gameStates.render.nType = nStartState; gameStates.render.nType < nEndState; gameStates.render.nType++) {
	//if (gameStates.render.nShadowPass != 3)
		switch (gameStates.render.nType) {
			case 0: //solid sides
				glDepthFunc (GL_LESS);
				break;
			case 1: //walls
				glDepthFunc (GL_LEQUAL);
				break;
			case 2: //transparency (alpha blending)
				glDepthFunc (GL_LEQUAL);
				break;
			}
	while (nnRenderSegs)
		RenderSegment (gameData.render.mine.nRenderList [--nnRenderSegs], nWindow);
	}
glDepthFunc (GL_LESS);
gameStates.render.nType = -1;
return 1;
}

//------------------------------------------------------------------------------

inline void InitFaceList (void)
{
#if APPEND_LAYERED_TEXTURES
if (gameOpts->render.bOptimize) {
	nFaceListSize = 0;
	memset (faceListRoots, 0xFF, sizeof (faceListRoots));
	memset (faceListTails, 0xFF, sizeof (faceListTails));
	}
#endif
}

//------------------------------------------------------------------------------

void RenderObjList (int listnum, int nWindow)
{
if (migrateObjects) {
	int objnp = 0;
	int saveLinDepth = gameStates.render.detail.nMaxLinearDepth;
	gameStates.render.detail.nMaxLinearDepth = gameStates.render.detail.nMaxLinearDepthObjects;
	for (;;) {
		int objNum = nRenderObjList [listnum][objnp];
		if (objNum < 0) {
			if (objNum == -1)
				break;
			listnum = -objNum;
			objnp = 0;
			}					
		else {
#ifdef LASER_HACK
			tObject *objP = gameData.objs.objects + objNum;
			if ((objP->nType == OBJ_WEAPON) && 								//if its a weapon
				 (objP->lifeleft == Laser_maxTime) && 	//  and its in it's first frame
				 (nHackLasers < MAX_HACKED_LASERS) && 									//  and we have space for it
				 (objP->laserInfo.nParentObj > -1) &&					//  and it has a parent
				 ((OBJ_IDX (gameData.objs.viewer)) == objP->laserInfo.nParentObj)	//  and it's parent is the viewer
				) {
				Hack_laser_list [nHackLasers++] = objNum;								//then make it draw after everything else.
				}
			else	
#endif
				DoRenderObject (objNum, nWindow);	// note link to above else
			objnp++;
			}
		}
	gameStates.render.detail.nMaxLinearDepth = saveLinDepth;
	}
}

//------------------------------------------------------------------------------

void RenderVisibleObjects (int nWindow)
{
	int	i, j;

if (gameStates.render.nShadowPass == 2)
	return;
//memset (gameData.render.mine.bVisited, 0, sizeof (gameData.render.mine.bVisited [0]) * (gameData.segs.nLastSegment + 1));
gameData.render.mine.nVisited++;
for (i = gameData.render.mine.nRenderSegs; i--;)  {
	j = gameData.render.mine.nRenderList [i];
	if ((j != -1) && !VISITED (j)) {
		VISIT (j);
		RenderObjList (i, nWindow);
		}
	}
}

//------------------------------------------------------------------------------

void RenderFaceList (int nWindow)
{
#if APPEND_LAYERED_TEXTURES
if (gameOpts->render.bOptimize) {
		int				i, j;
		tSegment			*segP;
		tSide				*sideP;
		tFaceListEntry	*flp = faceList;

	for (gameStates.render.nType = 0; gameStates.render.nType < 5; gameStates.render.nType++) {
#if 0
		switch (gameStates.render.nType) {
			case 1: //walls
				glDepthFunc (GL_LEQUAL);
				break;
			case 2: //transparency (alpha blending)
				glDepthFunc (GL_LEQUAL);
				break;
			default:
				glDepthFunc (GL_LESS);
				break;
			}
#endif
		for (i = 0; i < MAX_TEXTURES; i++) {
			if (0 > faceListRoots [i])
				continue;
			for (flp = faceList + faceListRoots [i]; ; flp = faceList + flp->nextFace) {
				segP = gameData.segs.segments + flp->props.segNum;
				sideP = segP->sides + flp->props.sideNum;
				if (flp->props.nType == gameStates.render.nType)
					RenderFace (&flp->props, 0, 1);
				if (flp->nextFace < 0)
					break;
				}
			}
		}
	RenderVisibleObjects (nWindow);
	}
#endif
}

//------------------------------------------------------------------------------

void AddVisibleLight (short nSegment, short nSide, short nTexture, int bPrimary)
{
if ((bPrimary || nTexture) && gameData.pig.tex.brightness [nTexture]) {
	tLightRef	*plr = gameData.render.color.visibleLights + gameData.render.color.nVisibleLights++;

	plr->nSegment = nSegment;
	plr->nSide = nSide;
	plr->nTexture = nTexture;
	}
}

//------------------------------------------------------------------------------

void GatherVisibleLights (void)
{
	int	i;
	short	nSegment, nSide;
	tSide	*sideP;

gameData.render.color.nVisibleLights = 0;
for (i = 0; i < gameData.render.mine.nRenderSegs;i++) {
	nSegment = gameData.render.mine.nRenderList [i];
	for (nSide = 0, sideP = gameData.segs.segments [nSegment].sides; nSide < 6; nSide++, sideP++) {
		AddVisibleLight (nSegment, nSide, sideP->nBaseTex, 1);
		AddVisibleLight (nSegment, nSide, sideP->nBaseTex, 0);
		}
	}
}

//------------------------------------------------------------------------------

void RotateSideNorms (void)
{
	int			i, j;
	tSegment		*segP = gameData.segs.segments;
	tSegment2	*seg2P = gameData.segs.segment2s;
	tSide			*sideP;
	tSide2		*side2P;

for (i = gameData.segs.nSegments; i; i--, segP++, seg2P++)
	for (j = 6, sideP = segP->sides, side2P = seg2P->sides; j; j--, sideP++, side2P++) {
		G3RotatePoint (side2P->rotNorms, sideP->normals, 0);
		G3RotatePoint (side2P->rotNorms + 1, sideP->normals + 1, 0);
		}
}

//------------------------------------------------------------------------------

#if USE_SEGRADS

void TransformSideCenters (void)
{
	int	i;

for (i = 0; i < gameData.segs.nSegments; i++)
	G3TransformPoint (gameData.segs.segCenters [1] + i, gameData.segs.segCenters [0] + i, 0);
}

#endif

// -----------------------------------------------------------------------------

int AllowedToFireMissile (void);

void GetPlayerMslLock (void)
{
	int			nWeapon, nObject, nGun, h, i, j;
	vmsVector	vGunPos;
	vmsMatrix	m;
	tObject		*objP;

gameData.objs.trackGoals [0] =
gameData.objs.trackGoals [1] = NULL;
if (objP = GuidedInMainView ()) {
	nObject = FindHomingObject (&objP->position.vPos, objP);
	gameData.objs.trackGoals [0] = 
	gameData.objs.trackGoals [1] = (nObject < 0) ? NULL : gameData.objs.objects + nObject;
	return;
	}
if (!AllowedToFireMissile ())
	return;
if (!EGI_FLAG (bMslLockIndicators, 0, 1, 0) || COMPETITION)
	return;
if (gameStates.app.bPlayerIsDead)
	return;
nWeapon = secondaryWeaponToWeaponInfo [gameData.weapons.nSecondary];
if (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] <= 0)
	return;
if (!gameStates.app.cheats.bHomingWeapons &&
	 (nWeapon != HOMINGMSL_ID) && (nWeapon != MEGAMSL_ID) && (nWeapon != GUIDEDMSL_ID))
	return;
//pnt = gameData.pig.ship.player->gunPoints [nGun];
j = !COMPETITION && (EGI_FLAG (bDualMissileLaunch, 0, 1, 0)) ? 2 : 1;
h = gameData.laser.nMissileGun & 1;
VmCopyTransposeMatrix (&m, &gameData.objs.console->position.mOrient);
for (i = 0; i < j; i++, h = !h) {
	nGun = secondaryWeaponToGunNum [gameData.weapons.nSecondary] + h;
	vGunPos = gameData.pig.ship.player->gunPoints [nGun];
	VmVecRotate (&vGunPos, &vGunPos, &m);
	VmVecInc (&vGunPos, &gameData.objs.console->position.vPos);
	nObject = FindHomingObject (&vGunPos, gameData.objs.console);
	gameData.objs.trackGoals [i] = (nObject < 0) ? NULL : gameData.objs.objects + nObject;
	}
}

//------------------------------------------------------------------------------
//renders onto current canvas

int BeginRenderMine (short nStartSeg, fix nEyeOffset, int nWindow)
{
	window	*rwP;
#ifdef _DEBUG
	int		i;
#endif

GetPlayerMslLock ();
windowRenderedData [nWindow].numObjects = 0;
#ifdef LASER_HACK
nHackLasers = 0;
#endif
memset (bObjectRendered, 0, (gameData.objs.nLastObject + 1) * sizeof (bObjectRendered [0]));
	//set up for rendering

if (((gameStates.render.nRenderPass <= 0) && 
	  (gameStates.render.nShadowPass < 2) && 
	  (gameStates.render.nShadowBlurPass < 2)) ||
	 gameStates.render.bShadowMaps) {
	RenderStartFrame ();
	TransformDynLights (1, 1);
#if USE_SEGRADS
	TransformSideCenters ();
#endif
	RotateSideNorms ();
#if defined(EDITOR) && defined (_DEBUG)
	if (bShowOnlyCurSide) {
		RotateVertexList (8, Cursegp->verts);
		RenderSide (Cursegp, Curside);
		goto done_rendering;
		}
#endif
	 }
#ifdef EDITOR
	if (bSearchMode)	{
		}
	else 
#endif

if (((gameStates.render.nRenderPass <= 0) && 
	  (gameStates.render.nShadowPass < 2) && 
	  (gameStates.render.nShadowBlurPass < 2)) || 
	 (gameStates.render.nShadowPass == 2)) {
	BuildSegmentList (nStartSeg, nWindow);		//fills in gameData.render.mine.nRenderList & gameData.render.mine.nRenderSegs
	//GatherVisibleLights ();
#if OGL_QUERY
	if (1 && !nWindow) {
		memset (bRenderSegObjs, 0, sizeof (bRenderSegObjs));
		memset (bRenderObjs, 0, sizeof (bRenderObjs));
		nRenderObjs = 0;
		}
#endif
#ifdef _DEBUG
	if (!bWindowCheck) {
		nWindowClipLeft  = nWindowClipTop = 0;
		nWindowClipRight = grdCurCanv->cv_bitmap.bm_props.w-1;
		nWindowClipBot   = grdCurCanv->cv_bitmap.bm_props.h-1;
		}
	for (i = 0; i < gameData.render.mine.nRenderSegs;i++) {
		short nSegment = gameData.render.mine.nRenderList [i];
		if (nSegment != -1) {
			if (visited2 [nSegment])
				Int3();		//get Matt
			else
				visited2 [nSegment] = 1;
			}
		}
#endif
	if ((gameStates.render.nRenderPass <= 0) && (gameStates.render.nShadowPass < 2)) {
		BuildObjectLists (gameData.render.mine.nRenderSegs);
		if (nEyeOffset <= 0)	// Do for left eye or zero.
			SetDynamicLight();
		}
	}
if (nClearWindow == 2) {
	if (nFirstTerminalSeg < gameData.render.mine.nRenderSegs) {
		int i;

		if (nClearWindowColor == -1)
			nClearWindowColor = BLACK_RGBA;
		GrSetColor (nClearWindowColor);
		for (i=nFirstTerminalSeg, rwP = renderWindows; i < gameData.render.mine.nRenderSegs; i++, rwP++) {
			if (gameData.render.mine.nRenderList [i] != -1) {
#ifdef _DEBUG
				if ((rwP->left == -1) || 
					 (rwP->top == -1) || 
					 (rwP->right == -1) || 
					 (rwP->bot == -1))
					Int3();
				else
#endif
					//NOTE LINK TO ABOVE!
					GrRect(rwP->left, rwP->top, rwP->right, rwP->bot);
				}
			}
		}
	}
InitFaceList ();
gameStates.render.bFullBright =
	gameStates.render.automap.bDisplay && 
	gameOpts->render.automap.bBright;
return !gameStates.render.cameras.bActive && (gameData.objs.viewer->nType != OBJ_ROBOT);
}

//------------------------------------------------------------------------------

void RenderSkyBox (int nWindow)
{
	int	nSegment;

if (gameStates.render.bHaveSkyBox) {
	gameStates.render.bHaveSkyBox = 0;
	gameStates.render.nType = 4;
	for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++)
		if (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SKYBOX) {
			gameStates.render.bHaveSkyBox = 1;
			RenderSegment (nSegment, nWindow);
			}
	}
}

//------------------------------------------------------------------------------

static ubyte bSetAutomapVisited;
extern ubyte bAutomapVisited [];

void RenderMineSegment (int nn)
{
	int nSegment = (nn < 0) ? -nn - 1 : gameData.render.mine.nRenderList [nn];

if (nSegment == -1)
	return;
if (gameStates.render.automap.bDisplay) {
	if (!(gameStates.render.automap.bFull || bAutomapVisited [nSegment]))
		return;
	if (!gameOpts->render.automap.bSkybox && (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SKYBOX))
		return;
	}
else {
	if (VISITED (nSegment))
		return;
	}
#ifdef _DEBUG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
SetNearestDynamicLights (nSegment);
RenderSegment (nSegment, gameStates.render.nWindow);
VISIT (nSegment);
if ((gameStates.render.nType == 0) && !gameStates.render.automap.bDisplay)
	bAutomapVisited [nSegment] = bSetAutomapVisited;
else if (gameStates.render.nType == 1) {
	SetNearestStaticLights (nSegment, 1);
	RenderObjList (nn, gameStates.render.nWindow);
	SetNearestStaticLights (nSegment, 0);
	}	
else if (gameStates.render.nType == 2)	// render objects containing transparency, like explosions
	RenderObjList (nn, gameStates.render.nWindow);
}

//------------------------------------------------------------------------------
//renders onto current canvas

void RenderMine (short nStartSeg, fix nEyeOffset, int nWindow)
{
	int		nn;

gameStates.render.nWindow = nWindow;
bSetAutomapVisited = BeginRenderMine (nStartSeg, nEyeOffset, nWindow);
gameStates.render.nType = 0;	//render solid geometry front to back
gameData.render.mine.nVisited++;
for (nn = 0; nn < gameData.render.mine.nRenderSegs; )
	RenderMineSegment (nn++);
if (!gameStates.render.automap.bDisplay)
	RenderSkyBox (nWindow);
gameStates.render.nType = 1;	//render transparency back to front
gameData.render.mine.nVisited++;
for (nn = gameData.render.mine.nRenderSegs; nn; )
	RenderMineSegment (--nn);
if (FAST_SHADOWS ? (gameStates.render.nShadowPass < 2) : (gameStates.render.nShadowPass != 2)) {
	glDepthFunc (GL_LEQUAL);
	gameStates.render.nType = 2;
	gameData.render.mine.nVisited++;
	for (nn = gameData.render.mine.nRenderSegs; nn;)
		RenderMineSegment (--nn);
	glDepthFunc (GL_LESS);
	if (!gameStates.app.bNostalgia &&
		 (!gameStates.render.automap.bDisplay || gameOpts->render.automap.bCoronas) && 
		 gameOpts->render.bCoronas && LoadCorona ()) {
		gameStates.render.nType = 3;
		gameData.render.mine.nVisited++;
		glEnable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthFunc (GL_LEQUAL);
		glDepthMask (0);
		for (nn = gameData.render.mine.nRenderSegs; nn;)
			RenderMineSegment (--nn);
		glDepthMask (1);
		glDepthFunc (GL_LESS);
		glDisable (GL_TEXTURE_2D);
		}
	glDepthFunc (GL_LESS);
	if (!(nWindow || gameStates.render.cameras.bActive || GuidedInMainView ()))
		RenderRadar ();
	if (gameStates.render.automap.bDisplay) {
		if (gameOpts->render.automap.bSmoke)
			RenderSmoke ();
		}
	}
}

//------------------------------------------------------------------------------

#ifdef EDITOR

extern int render_3d_in_big_window;

//finds what tSegment is at a given x&y -  seg, tSide, face are filled in
//works on last frame rendered. returns true if found
//if seg<0, then an tObject was found, and the tObject number is -seg-1
int find_seg_side_face(short x, short y, int *seg, int *tSide, int *face, int *poly)
{
	bSearchMode = -1;
	_search_x = x; _search_y = y;
	found_seg = -1;
	if (render_3d_in_big_window) {
		grs_canvas temp_canvas;

		GrInitSubCanvas(&temp_canvas, canv_offscreen, 0, 0, 
			LargeView.ev_canv->cv_bitmap.bm_props.w, LargeView.ev_canv->cv_bitmap.bm_props.h);
		GrSetCurrentCanvas(&temp_canvas);
		RenderFrame(0, 0);
	}
	else {
		GrSetCurrentCanvas(&gameStates.render.vr.buffers.subRender [0]);	//render off-screen
		RenderFrame(0, 0);
	}
	bSearchMode = 0;
	*seg = found_seg;
	*tSide = found_side;
	*face = found_face;
	*poly = found_poly;
	return (found_seg!=-1);
}

#endif

//------------------------------------------------------------------------------
//eof
