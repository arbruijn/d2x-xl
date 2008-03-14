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
#include "u_mem.h"
#include "error.h"
#include "gameseg.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "endlevel.h"
#include "ogl_defs.h"
#include "ogl_color.h"
#include "ogl_render.h"
#include "renderlib.h"

//------------------------------------------------------------------------------

#define SW_CULLING 1
#define SOFT_SHADOWS		0
#define RENDER_DEPTHMASK_FIRST 1

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

//------------------------------------------------------------------------------

int _CDECL_ D2X_RenderThread (void *p);
int _CDECL_ D2X_OpenGLThread (void *p);

#if DBG_SHADOWS
extern int bShadowTest;
#endif

#define INITIAL_LOCAL_LIGHT (F1_0/4)    // local light value in tSegment of occurence (of light emission)

#ifdef EDITOR
#include "editor/editor.h"
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

int FaceIsVisible (short nSegment, short nSide)
{
#if SW_CULLING
tSegment *segP = SEGMENTS + nSegment;
tSide *sideP = segP->sides + nSide;
vmsVector v;
VmVecSub (&v, &gameData.render.mine.viewerEye, SIDE_CENTER_I (nSegment, nSide)); //gameData.segs.vertices + segP->verts [sideToVerts [nSide][0]]);
return (sideP->nType == SIDE_IS_QUAD) ?
		 VmVecDot (sideP->normals, &v) >= 0 :
		 (VmVecDot (sideP->normals, &v) >= 0) || (VmVecDot (sideP->normals + 1, &v) >= 0);
#else
return 1;
#endif
}

//------------------------------------------------------------------------------

void RotateTexCoord2f (tTexCoord2f *pDest, tTexCoord2f *pSrc, ubyte nOrient)
{
if (nOrient == 1) {
	pDest->v.u = 1.0f - pSrc->v.v;
	pDest->v.v = pSrc->v.u;
	}
else if (nOrient == 2) {
	pDest->v.u = 1.0f - pSrc->v.u;
	pDest->v.v = 1.0f - pSrc->v.v;
	}
else if (nOrient == 3) {
	pDest->v.u = pSrc->v.v;
	pDest->v.v = 1.0f - pSrc->v.u;
	}
else {
	pDest->v.u = pSrc->v.u;
	pDest->v.v = pSrc->v.v;
	}
}

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

inline int LoadExtraBitmap (grsBitmap **bmPP, char *pszName, int *bHaveP)
{
if (!*bHaveP) {
	grsBitmap *bmP = CreateAndReadTGA (pszName);
	if (!bmP)
		*bHaveP = -1;
	else {
		*bHaveP = 1;
		BM_FRAMECOUNT (bmP) = bmP->bmProps.h / bmP->bmProps.w;
		OglBindBmTex (bmP, 1, 1);
		}	
	*bmPP = bmP;
	}
return *bHaveP > 0;
}

//------------------------------------------------------------------------------

grsBitmap *bmpExplBlast = NULL;
int bHaveExplBlast = 0;

int LoadExplBlast (void)
{
return LoadExtraBitmap (&bmpExplBlast, "blast.tga", &bHaveExplBlast);
}

//------------------------------------------------------------------------------

void FreeExplBlast (void)
{
if (bmpExplBlast) {
	GrFreeBitmap (bmpExplBlast);
	bmpExplBlast = NULL;
	bHaveExplBlast = 0;
	}
}

//------------------------------------------------------------------------------

grsBitmap *bmpCorona = NULL;
int bHaveCorona = 0;

int LoadCorona (void)
{
return LoadExtraBitmap (&bmpCorona, "corona.tga", &bHaveCorona);
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

grsBitmap *bmpGlare = NULL;
int bHaveGlare = 0;

int LoadGlare (void)
{
return LoadExtraBitmap (&bmpGlare, "glare.tga", &bHaveGlare);
}

//------------------------------------------------------------------------------

void FreeGlare (void)
{
if (bmpGlare) {
	GrFreeBitmap (bmpGlare);
	bmpGlare = NULL;
	bHaveGlare = 0;
	}
}

//------------------------------------------------------------------------------

grsBitmap *bmpHalo = NULL;
int bHaveHalo = 0;

int LoadHalo (void)
{
return LoadExtraBitmap (&bmpHalo, "halo.tga", &bHaveHalo);
}

//------------------------------------------------------------------------------

void FreeHalo (void)
{
if (bmpHalo) {
	GrFreeBitmap (bmpHalo);
	bmpHalo = NULL;
	bHaveHalo = 0;
	}
}

//------------------------------------------------------------------------------

grsBitmap *bmpThruster [2] = {NULL, NULL};
int bHaveThruster [2] = {0, 0};

int LoadThruster (void)
{
	int nStyle = EGI_FLAG (bThrusterFlames, 1, 1, 0) == 2;

return LoadExtraBitmap (&bmpThruster [nStyle], (EGI_FLAG (bThrusterFlames, 1, 1, 0) == 1) ? "thrust2d.tga" : "thrust3d.tga", bHaveThruster + nStyle);
}

//------------------------------------------------------------------------------

void FreeThruster (void)
{
	int	i;

for (i = 0; i < 2; i++)
	if (bmpThruster [i]) {
		GrFreeBitmap (bmpThruster [i]);
		bmpThruster [i] = NULL;
		bHaveThruster [i] = 0;
		}
}

//------------------------------------------------------------------------------

grsBitmap *bmpShield = NULL;
int bHaveShield = 0;

int LoadShield (void)
{
return LoadExtraBitmap (&bmpShield, "shield.tga", &bHaveShield);
}

//------------------------------------------------------------------------------

void FreeShield (void)
{
if (bmpShield) {
	GrFreeBitmap (bmpShield);
	bmpShield = NULL;
	bHaveShield = 0;
	}
}

//------------------------------------------------------------------------------

void FreeDeadzone ();

void LoadExtraImages (void)
{
LogErr ("Loading extra images\n");
LoadCorona ();
LoadGlare ();
LoadHalo ();
LoadThruster ();
LoadShield ();
LoadExplBlast ();
LoadDeadzone ();
}

//------------------------------------------------------------------------------

void FreeExtraImages (void)
{
FreeCorona ();
FreeGlare ();
FreeHalo ();
FreeThruster ();
FreeShield ();
FreeExplBlast ();
FreeDeadzone ();
}

//------------------------------------------------------------------------------

void DrawOutline (int nVertices, g3sPoint **pointList)
{
	int i;
	GLint depthFunc; 
	g3sPoint center, normal;
	vmsVector n;
	fVector *nf;

#if 1 //def RELEASE
if (gameStates.render.bQueryOcclusion) {
	tRgbaColorf outlineColor = {1, 1, 0, -1};
	G3DrawPolyAlpha (nVertices, pointList, &outlineColor, 1, -1);
	return;
	}
#endif

glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
glDepthFunc (GL_ALWAYS);
GrSetColorRGB (255, 255, 255, 255);
VmVecZero (&center.p3_vec);
for (i = 0; i < nVertices; i++) {
	G3DrawLine (pointList [i], pointList [(i + 1) % nVertices]);
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

// ----------------------------------------------------------------------------

char IsColoredSegFace (short nSegment, short nSide)
{
	short nConnSeg;
	int	owner;
	int	special;

if ((gameData.app.nGameMode & GM_ENTROPY) && (extraGameInfo [1].entropy.nOverrideTextures == 2) && 
	 ((owner = gameData.segs.xSegments [nSegment].owner) > 0)) {
	nConnSeg = gameData.segs.segments [nSegment].children [nSide];
	if ((nConnSeg < 0) || (gameData.segs.xSegments [nConnSeg].owner != owner))
		return (owner == 1) ? 2 : 1;
	}
special = gameData.segs.segment2s [nSegment].special;
nConnSeg = gameData.segs.segments [nSegment].children [nSide];
if ((nConnSeg >= 0) && (special == gameData.segs.segment2s [nConnSeg].special))
	return 0;
if (special == SEGMENT_IS_WATER) 
	return 3;
if (special == SEGMENT_IS_LAVA)
	return 4;
return 0;
}

// ----------------------------------------------------------------------------

tRgbaColorf segmentColors [4] = {
		{0.5f, 0, 0, 0.2f},
		{0, 0, 0.5f, 0.2f},
		{0, 1.0f / 16.0f, 0.5f, 0.2f},
		{0.5f, 0, 0, 0.2f}};

tRgbaColorf *ColoredSegmentColor (int nSegment, int nSide, char nColor)
{
	short nConnSeg;
	int	owner;
	int	special;

if (nColor > 0)
	nColor--;
else {
	if ((gameData.app.nGameMode & GM_ENTROPY) && (extraGameInfo [1].entropy.nOverrideTextures == 2) && 
		((owner = gameData.segs.xSegments [nSegment].owner) > 0)) {
		nConnSeg = gameData.segs.segments [nSegment].children [nSide];
		if ((nConnSeg >= 0) && (gameData.segs.xSegments [nConnSeg].owner == owner))
			return NULL;
		nColor = (owner == 1);
		}
	special = gameData.segs.segment2s [nSegment].special;
	if (special == SEGMENT_IS_WATER) 
		nColor = 2;
	else if (special == SEGMENT_IS_LAVA)
		nColor = 3;
	else
		return NULL;
	nConnSeg = gameData.segs.segments [nSegment].children [nSide];
	if (nConnSeg >= 0) {
		if (special == gameData.segs.segment2s [nConnSeg].special)
			return NULL;
		if (IS_WALL (gameData.segs.segments [nSegment].sides [nSide].nWall))
			return NULL;
		}
	}
return segmentColors + nColor;
}

//------------------------------------------------------------------------------
// If any color component > 1, scale all components down so that the greatest == 1.

static inline void ScaleColor (tFaceColor *pc, float l)
{
	float m = pc->color.red;

if (m < pc->color.green)
	m = pc->color.green;
if (m < pc->color.blue)
	m = pc->color.blue;
if (m > l) {
	m = l / m;
	pc->color.red *= m;
	pc->color.green *= m;
	pc->color.blue *= m;
	}
}

//------------------------------------------------------------------------------

int SetVertexColor (int nVertex, tFaceColor *pc)
{
#ifdef _DEBUG
if (nVertex == nDbgVertex)
	nVertex = nVertex;
#endif
if (gameOpts->render.color.bAmbientLight && !USE_LIGHTMAPS) { 
	pc->color.red += gameData.render.color.ambient [nVertex].color.red;
	pc->color.green += gameData.render.color.ambient [nVertex].color.green;
	pc->color.blue += gameData.render.color.ambient [nVertex].color.blue;
	}
#if 0
else
	memset (pc, 0, sizeof (*pc));
#endif
return 1;
}

//------------------------------------------------------------------------------

int SetVertexColors (tFaceProps *propsP)
{
if (SHOW_DYN_LIGHT) {
	// set material properties specific for certain textures here
	SetDynLightMaterial (propsP->segNum, propsP->sideNum, -1);
	return 0;
	}
memset (vertColors, 0, sizeof (vertColors));
if (gameOpts->render.color.bAmbientLight && !USE_LIGHTMAPS) { 
	int i, j = propsP->nVertices;
	for (i = 0; i < j; i++)
		SetVertexColor (propsP->vp [i], vertColors + i);
	}
else
	memset (vertColors, 0, sizeof (vertColors));
return 1;
}

//------------------------------------------------------------------------------

fix SetVertexLight (int nSegment, int nSide, int nVertex, tFaceColor *pc, fix light)
{
	tRgbColorf	*pdc;
	fix			dynLight;
	float			fl, dl, hl;

//the tUVL struct has static light already in it
//scale static light for destruction effect
if (EGI_FLAG (bDarkness, 0, 0, 0))
	light = 0;
else {
#if LMAP_LIGHTADJUST
	if (USE_LIGHTMAPS) {
		else {
			light = F1_0 / 2 + gameData.render.lights.segDeltas [nSegment * 6 + nSide];
			if (light < 0)
				light = 0;
			}
		}
#endif
	if (gameData.reactor.bDestroyed || gameStates.gameplay.seismic.nMagnitude)	//make lights flash
		light = FixMul (gameStates.render.nFlashScale, light);
	}
//add in dynamic light (from explosions, etc.)
dynLight = gameData.render.lights.dynamicLight [nVertex];
fl = f2fl (light);
dl = f2fl (dynLight);
light += dynLight;
#ifdef _DEBUG
if (nVertex == nDbgVertex)
	nVertex = nVertex;
#endif
if (gameStates.app.bHaveExtraGameInfo [IsMultiGame]) {
	if (gameData.render.lights.bGotDynColor [nVertex]) {
		pdc = gameData.render.lights.dynamicColor + nVertex;
		if (gameOpts->render.color.bMix) {
			if (gameOpts->render.color.bGunLight) {
				if (gameOpts->render.color.bAmbientLight && !gameOpts->render.color.bUseLightMaps) {
					if ((fl != 0) && gameData.render.color.vertBright [nVertex]) {
						hl = fl / gameData.render.color.vertBright [nVertex];
						pc->color.red = pc->color.red * hl + pdc->red * dl;
						pc->color.green = pc->color.green * hl + pdc->green * dl;
						pc->color.blue = pc->color.blue * hl + pdc->blue * dl;
						ScaleColor (pc, fl + dl);
						}
					else {
						pc->color.red = pdc->red * dl;
						pc->color.green = pdc->green * dl;
						pc->color.blue = pdc->blue * dl;
						ScaleColor (pc, dl);
						}
					}
				else {
					pc->color.red = fl + pdc->red * dl;
					pc->color.green = fl + pdc->green * dl;
					pc->color.blue = fl + pdc->blue * dl;
					ScaleColor (pc, fl + dl);
					}
				}
			else {
				pc->color.red =
				pc->color.green =
				pc->color.blue = fl + dl;
				}
			if (gameOpts->render.color.bCap) {
				if (pc->color.red > 1.0)
					pc->color.red = 1.0;
				if (pc->color.green > 1.0)
					pc->color.green = 1.0;
				if (pc->color.blue > 1.0)
					pc->color.blue = 1.0;
				}
			}
		else {
			float dl = f2fl (light);
			dl = pow (dl, 1.0f / 3.0f);
			pc->color.red = pdc->red * dl;
			pc->color.green = pdc->green * dl;
			pc->color.blue = pdc->blue * dl;
			}
		}
	else {
		ScaleColor (pc, fl + dl);
		}
	}
else {
	ScaleColor (pc, fl + dl);
	}
//saturate at max value
if (light > MAX_LIGHT)
	light = MAX_LIGHT;
return light;
}

//------------------------------------------------------------------------------

int SetFaceLight (tFaceProps *propsP)
{
	int	i;

if (SHOW_DYN_LIGHT)
	return 0;
for (i = 0; i < propsP->nVertices; i++) {
	propsP->uvls [i].l = SetVertexLight (propsP->segNum, propsP->sideNum, propsP->vp [i], vertColors + i, propsP->uvls [i].l);
	vertColors [i].index = -1;
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

float WallAlpha (short nSegment, short nSide, short nWall, ubyte widFlags, int bIsMonitor, tRgbaColorf *colorP, int *nColor, ubyte *bTextured)
{
	static tRgbaColorf cloakColor = {0, 0, 0, 0};

	tWall	*wallP;
	float fAlpha;
	short	c;
	int	bCloaked;

if (!IS_WALL (nWall))
	return 1;
#ifdef _DEBUG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
wallP = gameData.walls.walls + nWall;
bCloaked = (wallP->state == WALL_DOOR_CLOAKING) || (wallP->state == WALL_DOOR_DECLOAKING) || ((widFlags & WID_CLOAKED_FLAG) != 0);
if (bCloaked || (widFlags & WID_TRANSPARENT_FLAG)) {
	if (bIsMonitor)
		return 1;
	c = wallP->cloakValue;
	if (bCloaked) {
		*colorP = cloakColor;
		*nColor = 1;
		*bTextured = 0;
		return colorP->alpha = (c >= GR_ACTUAL_FADE_LEVELS) ? 0 : 1.0f - (float) c / (float) GR_ACTUAL_FADE_LEVELS;
		}
	if (!gameOpts->render.color.bWalls)
		c = 0;
	if (gameData.walls.walls [nWall].hps)
		fAlpha = (float) fabs ((1.0f - (float) gameData.walls.walls [nWall].hps / ((float) F1_0 * 100.0f)));
	else if (IsMultiGame && gameStates.app.bHaveExtraGameInfo [1])
		fAlpha = COMPETITION ? 0.5f : (float) (GR_ACTUAL_FADE_LEVELS - extraGameInfo [1].grWallTransparency) / (float) GR_ACTUAL_FADE_LEVELS;
	else
		fAlpha = 1.0f - extraGameInfo [0].grWallTransparency / (float) GR_ACTUAL_FADE_LEVELS;
	if (fAlpha < 1) {
		//fAlpha = (float) sqrt (fAlpha);
		colorP->red = (float) CPAL2Tr (gamePalette, c) / fAlpha;
		colorP->green = (float) CPAL2Tg (gamePalette, c) / fAlpha;
		colorP->blue = (float) CPAL2Tb (gamePalette, c) / fAlpha;
		*bTextured = 0;
		*nColor = 1;
		}
	return colorP->alpha = fAlpha;
	}
if (gameStates.app.bD2XLevel) {
	c = wallP->cloakValue;
	return colorP->alpha = (c && (c < GR_ACTUAL_FADE_LEVELS)) ? (float) (GR_ACTUAL_FADE_LEVELS - c) / (float) GR_ACTUAL_FADE_LEVELS : 1;
	}
if (gameOpts->render.bAutoTransparency && IsTransparentTexture (gameData.segs.segments [nSegment].sides [nSide].nBaseTex))
	return colorP->alpha = 0.8f;
return colorP->alpha = 1;
}

//------------------------------------------------------------------------------

int IsMonitorFace (short nSegment, short nSide)
{
return (gameStates.render.bDoCameras && gameData.cameras.nSides) ? gameData.cameras.nSides [nSegment * 6 + nSide] : -1;
}

//------------------------------------------------------------------------------

int SetupMonitorFace (short nSegment, short nSide, short nCamera, grsFace *faceP)
{
	tCamera		*pc = gameData.cameras.cameras + nCamera;
	int			bHaveMonitorBg, bIsTeleCam = pc->bTeleport;
#ifdef RELEASE
	int			i;
#endif
#if RENDER2TEXTURE
	int			bCamBufAvail = OglCamBufAvail (pc, 1) == 1;
#else
	int			bCamBufAvail = 0;
#endif

if (!gameStates.render.bDoCameras)
	return 0;
bHaveMonitorBg = pc->bValid && /*!pc->bShadowMap &&*/ 
					  (pc->texBuf.glTexture || bCamBufAvail) &&
					  (!bIsTeleCam || EGI_FLAG (bTeleporterCams, 0, 1, 0));
if (bHaveMonitorBg) {
	GetCameraUVL (pc, NULL, gameData.segs.faces.texCoord + faceP->nIndex, gameData.segs.faces.vertices + faceP->nIndex);
	pc->texBuf.glTexture->wrapstate = -1;
	if (bIsTeleCam) {
#ifdef _DEBUG
		faceP->bmBot = &pc->texBuf;
		gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
#else
		faceP->bmTop = &pc->texBuf;
		for (i = 0; i < 4; i++)
			gameData.render.color.vertices [faceP->index [i]].color.alpha = 0.7f;
#endif
		}
	else if (gameOpts->render.cameras.bFitToWall || (faceP->nOvlTex > 0) || !faceP->bmBot)
		faceP->bmBot = &pc->texBuf;
	else
		faceP->bmTop = &pc->texBuf;
	faceP->pTexCoord = pc->texCoord;
	}
faceP->bTeleport = bIsTeleCam;
pc->bVisible = 1;
return bHaveMonitorBg || gameOpts->render.cameras.bFitToWall;
}

//------------------------------------------------------------------------------
//draw outline for curside
#ifdef _DEBUG

#define CROSS_WIDTH  i2f(8)
#define CROSS_HEIGHT i2f(8)

void OutlineSegSide (tSegment *seg, int _side, int edge, int vert)
{
	g3sCodes cc;

cc = RotateVertexList (8, seg->verts);
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

//------------------------------------------------------------------------------

void AdjustVertexColor (grsBitmap *bmP, tFaceColor *pc, fix xLight)
{
	float l = (bmP && (bmP->bmProps.flags & BM_FLAG_NO_LIGHTING)) ? 1.0f : f2fl (xLight);
	float s = 1.0f;

#if SHADOWS
if (gameStates.ogl.bScaleLight)
	s *= gameStates.render.bHeadLightOn ? 0.4f : 0.3f;
#endif
if (!pc->index || !gameOpts->render.color.bAmbientLight || (gameStates.app.bEndLevelSequence >= EL_OUTSIDE)) {
	pc->color.red =
	pc->color.green =
	pc->color.blue = l * s;
	}
else if (s != 1) {
	pc->color.red *= s;
	pc->color.green *= s;
	pc->color.blue *= s;
	}
pc->color.alpha = 1;
}

// -----------------------------------------------------------------------------------
//Given a list of point numbers, rotate any that haven't been bRotated this frame
//cc.and and cc.or will contain the position/orientation of the face that is determined 
//by the vertices passed relative to the viewer

g3sCodes RotateVertexList (int nVertices, short *vertexIndexP)
{
	int			i, j;
	g3sPoint		*pnt;
	g3sCodes		cc;

cc.and = 0xff;  
cc.or = 0;
for (i = 0; i < nVertices; i++) {
	j = vertexIndexP [i];
	pnt = gameData.segs.points + j;
	if (gameData.render.mine.nRotatedLast [j] != gameStates.render.nFrameCount) {
		G3TransformAndEncodePoint (pnt, gameData.segs.vertices + j);
		if (gameData.render.zMax < pnt->p3_vec.p.z)
			gameData.render.zMax = pnt->p3_vec.p.z;
		if (gameStates.ogl.bUseTransform) {
			gameData.segs.fVertices [j].p.x = f2fl (gameData.segs.vertices [j].p.x);
			gameData.segs.fVertices [j].p.y = f2fl (gameData.segs.vertices [j].p.y);
			gameData.segs.fVertices [j].p.z = f2fl (gameData.segs.vertices [j].p.z);
			}
		else {
			gameData.segs.fVertices [j].p.x = f2fl (pnt->p3_vec.p.x);
			gameData.segs.fVertices [j].p.y = f2fl (pnt->p3_vec.p.y);
			gameData.segs.fVertices [j].p.z = f2fl (pnt->p3_vec.p.z);
			}
		gameData.render.mine.nRotatedLast [j] = gameStates.render.nFrameCount;
		}
	cc.and &= pnt->p3_codes;
	cc.or |= pnt->p3_codes;
	pnt->p3_index = j;
	}
return cc;
}

// -----------------------------------------------------------------------------------
//Given a lit of point numbers, project any that haven't been projected
void ProjectVertexList (int nVertices, short *vertexIndexP)
{
	int i, j;

for (i = 0; i < nVertices; i++) {
	j = vertexIndexP [i];
	if (!(gameData.segs.points [j].p3_flags & PF_PROJECTED))
		G3ProjectPoint (gameData.segs.points + j);
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

//------------------------------------------------------------------------------

void BumpVisitedFlag (void)
{
if (!++gameData.render.mine.nVisited) {
	memset (gameData.render.mine.bVisited, 0, sizeof (gameData.render.mine.bVisited));
	gameData.render.mine.nVisited = 1;
	}
}

//------------------------------------------------------------------------------

void BumpProcessedFlag (void)
{
if (!++gameData.render.mine.nProcessed) {
	memset (gameData.render.mine.bProcessed, 0, sizeof (gameData.render.mine.bProcessed));
	gameData.render.mine.nProcessed = 1;
	}
}

//------------------------------------------------------------------------------

void BumpVisibleFlag (void)
{
if (!++gameData.render.mine.nVisible) {
	memset (gameData.render.mine.bVisible, 0, sizeof (gameData.render.mine.bVisible));
	gameData.render.mine.nVisible = 1;
	}
}

// ----------------------------------------------------------------------------

int SegmentMayBeVisible (short nStartSeg, short nRadius, int nMaxDist)
{
	tSegment	*segP;
	int		nSegment, nChildSeg, nChild, h, i, j;

if (gameData.render.mine.bVisible [nStartSeg] == gameData.render.mine.nVisible)
	return 1;
BumpProcessedFlag ();
gameData.render.mine.nSegRenderList [0] = nStartSeg;
gameData.render.mine.bProcessed [nStartSeg] = gameData.render.mine.nProcessed;
if (nMaxDist < 0)
	nMaxDist = nRadius * 20 * F1_0;
for (i = 0, j = 1; nRadius; nRadius--) {
	for (h = i, i = j; h < i; h++) {
		nSegment = gameData.render.mine.nSegRenderList [h];
		if ((gameData.render.mine.bVisible [nSegment] == gameData.render.mine.nVisible) &&
			 (!nMaxDist || (VmVecDist (SEGMENT_CENTER_I (nStartSeg), SEGMENT_CENTER_I (nSegment)) <= nMaxDist)))
			return 1;
		segP = SEGMENTS + nSegment;
		for (nChild = 0; nChild < 6; nChild++) {
			nChildSeg = segP->children [nChild];
			if ((nChildSeg >= 0) && (WALL_IS_DOORWAY (segP, nChild, NULL) == WID_RENDPAST_FLAG))
				gameData.render.mine.nSegRenderList [j++] = nChildSeg;
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------
// eof
