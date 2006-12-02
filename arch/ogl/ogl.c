/* $Id: ogl.c,v 1.14 2004/05/11 23:15:55 btb Exp $ */
/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
#else
# include <malloc.h>
# include <SDL.h>
#endif

#include "3d.h"
#include "piggy.h"
#include "../../3d/globvars.h"
#include "error.h"
#include "texmap.h"
#include "palette.h"
#include "rle.h"
#include "mono.h"

#include "inferno.h"
#include "textures.h"
#include "texmerge.h"
#include "effects.h"
#include "weapon.h"
#include "powerup.h"
#include "polyobj.h"
#include "gamefont.h"
#include "byteswap.h"
#include "cameras.h"
#include "render.h"
#include "grdef.h"
#include "ogl_init.h"
#include "lighting.h"
#include "lightmap.h"
#include "gamepal.h"
#include "particles.h"
#include "u_mem.h"
#include "menu.h"
#include "newmenu.h"
#include "gauges.h"

//------------------------------------------------------------------------------

#if OGL_MULTI_TEXTURING
#	ifndef GL_VERSION_20
PFNGLACTIVETEXTUREARBPROC			glActiveTexture = NULL;
#		ifdef _WIN32
PFNGLMULTITEXCOORD2DARBPROC		glMultiTexCoord2d = NULL;
PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2f = NULL;
#		endif
PFNGLCLIENTACTIVETEXTUREARBPROC	glClientActiveTexture = NULL;
#	endif
#endif

int bMultiTexturingOk = 0;

#if OGL_QUERY
#	ifndef GL_VERSION_20
PFNGLGENQUERIESARBPROC				glGenQueries = NULL;
PFNGLDELETEQUERIESARBPROC			glDeleteQueries = NULL;
PFNGLISQUERYARBPROC					glIsQuery = NULL;
PFNGLBEGINQUERYARBPROC				glBeginQuery = NULL;
PFNGLENDQUERYARBPROC					glEndQuery = NULL;
PFNGLGETQUERYIVARBPROC				glGetQueryiv = NULL;
PFNGLGETQUERYOBJECTIVARBPROC		glGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUIVARBPROC		glGetQueryObjectuiv = NULL;
#	endif
#endif

#if OGL_POINT_SPRITES
#	ifdef _WIN32
PFNGLPOINTPARAMETERFVARBPROC		glPointParameterfvARB = NULL;
PFNGLPOINTPARAMETERFARBPROC		glPointParameterfARB = NULL;
#	endif
#endif

#ifndef GL_VERSION_20
PFNGLACTIVESTENCILFACEEXTPROC		glActiveStencilFaceEXT = NULL;
#endif

//------------------------------------------------------------------------------

int bOcclusionQuery = 0;

//change to 1 for lots of spew.
#ifndef M_PI
#	define M_PI 3.141592653589793240
#endif

#if defined (_WIN32) || defined (__sun__)
#	define cosf(a) cos (a)
#	define sinf(a) sin (a)
#endif

int r_texcount=0;
int gr_renderstats = 0;
int gr_badtexture = 0;

tRenderQuality renderQualities [] = {
	{GL_NEAREST, GL_NEAREST, 0, 0},	// no smoothing
	{GL_NEAREST, GL_LINEAR, 0, 0},	// smooth close textures, don't smooth distant textures
	{GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 1, 0},	// smooth close textures, use non-smoothed mipmaps distant textures
	{GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 1, 0},	//smooth close textures, use smoothed mipmaps for distant ones
	{GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 1, 1}	//smooth close textures, use smoothed mipmaps for distant ones, anti-aliasing
	};
	
extern GLuint bsphereh;
extern GLuint ssphereh;
extern GLuint circleh5;
extern GLuint circleh10;
extern GLuint cross_lh [2];
extern GLuint primary_lh [3];
extern GLuint secondary_lh [5];
extern GLuint glInitTMU [3]; 
extern GLuint glExitTMU;
extern GLuint mouseIndList;

ogl_texture oglTextureList [OGL_TEXTURE_LIST_SIZE];
int oglTexListCur;

//------------------------------------------------------------------------------

inline void OglInitTextureStats (ogl_texture* t)
{
t->prio = (float) 0.3;//default prio
t->lastrend = 0;
t->numrend = 0;
}

//------------------------------------------------------------------------------

void OglInitTexture (ogl_texture *t, int bMask)
{
t->handle = 0;
t->internalformat = gameOpts->ogl.bRgbaFormat;
t->format = bMask ? GL_ALPHA : GL_RGBA;
t->wrapstate = -1;
t->w =
t->h = 0;
OglInitTextureStats (t);
}

//------------------------------------------------------------------------------

void OglResetTextureStatsInternal (void)
{
	int			i;
	ogl_texture	*t = oglTextureList;

for (i = OGL_TEXTURE_LIST_SIZE; i; i--, t++)
	if (t->handle)
		OglInitTextureStats (t);
}

//------------------------------------------------------------------------------

void OglInitTextureListInternal (void)
{
	int			i;
	ogl_texture	*t = oglTextureList;
	oglTexListCur = 0;

for (i = OGL_TEXTURE_LIST_SIZE; i; i--, t++)
	OglInitTexture (t, 0);
}

//------------------------------------------------------------------------------

void OglDeleteLists (GLuint *lp, int n)
{
for (; n; n--, lp++) {
	if (*lp)
		glDeleteLists (*lp, 1);
		*lp = 0;
	}
}

//------------------------------------------------------------------------------

inline void UnlinkTexture (grsBitmap *bmP)
{
if (bmP->glTexture && (bmP->glTexture->handle == -1)) {
	bmP->glTexture->handle = 0;
	bmP->glTexture = NULL;
	}
}

//------------------------------------------------------------------------------

void OglSmashTextureListInternal (void)
{
	int			h, i, j, k;
	ogl_texture *t;
	grsBitmap	*bmP, *altBmP;

OglDeleteLists (&bsphereh, 1);
OglDeleteLists (&ssphereh, 1);
OglDeleteLists (&circleh5, 1);
OglDeleteLists (&circleh10, 1);
OglDeleteLists (cross_lh, sizeof (cross_lh) / sizeof (*cross_lh));
OglDeleteLists (primary_lh, sizeof (primary_lh) / sizeof (*primary_lh));
OglDeleteLists (secondary_lh, sizeof (secondary_lh) / sizeof (*secondary_lh));
OglDeleteLists (glInitTMU, sizeof (glInitTMU) / sizeof (*glInitTMU));
OglDeleteLists (&glExitTMU, 1);
OglDeleteLists (&mouseIndList, 1);
bsphereh =
ssphereh =
circleh5 = 
circleh10 =
mouseIndList =
cross_lh [0] =
cross_lh [1] =
primary_lh [0] = 
primary_lh [1] = 
primary_lh [2] =
secondary_lh [0] =
secondary_lh [1] =
secondary_lh [2] =
secondary_lh [3] =
secondary_lh [4] =
glInitTMU [0] =
glInitTMU [1] =
glInitTMU [2] =
glExitTMU = 0;
for (i = OGL_TEXTURE_LIST_SIZE, t = oglTextureList; i; i--, t++) {
	if (t->handle > 0) {
		glDeleteTextures (1, &t->handle);
		t->handle = -1;
		}
	t->w =
	t->h = 0;
	t->wrapstate = -1;
	}
#if 1
for (k = 0; k < 2; k++) {
	bmP = gameData.pig.tex.bitmaps [k];
	for (i = gameData.pig.tex.nBitmaps [k] + (k ? 0 : gameData.hoard.nBitmaps); i; i--, bmP++) {
		if (bmP->bmType == BM_TYPE_STD) {
			if (!BM_OVERRIDE (bmP))
				UnlinkTexture (bmP);
			else {
				altBmP = BM_OVERRIDE (bmP);
				UnlinkTexture (altBmP);
				if ((altBmP->bmType == BM_TYPE_ALT) && BM_FRAMES (altBmP)) {
					h = BM_FRAMECOUNT (altBmP);
					if (h > 1)
						for (j = 0; j < h; j++)
							UnlinkTexture (BM_FRAMES (altBmP) + j);
					}
				}
			}
		}
	}
oglTexListCur = 0;
#endif
}

//------------------------------------------------------------------------------

ogl_texture *OglGetFreeTextureInternal (void)
{
	int i;
	ogl_texture *t = oglTextureList + oglTexListCur;

for (i = 0; i < OGL_TEXTURE_LIST_SIZE; i++) {
	if (!(t->handle || t->w))
		return t;
	if (++oglTexListCur < OGL_TEXTURE_LIST_SIZE)
		t++;
	else {
		oglTexListCur = 0;
		t = oglTextureList;
		}
	}
return NULL;
}

//------------------------------------------------------------------------------

ogl_texture *OglGetFreeTexture (void)
{
ogl_texture *t = OglGetFreeTextureInternal ();
if (!t) {
#ifdef _DEBUG
	Warning ("OGL: texture list full!\n");
#endif
	// try to recover: flush all textures, reload fonts and this level's textures
	RebuildGfxFx (gameStates.app.bGameRunning, 1);
	t = OglGetFreeTextureInternal ();
	}
return t;
}

//------------------------------------------------------------------------------

int ogl_texture_stats (void)
{
	int used=0,usedl4a4=0,usedrgba=0,databytes=0,truebytes=0,datatexel=0,truetexel=0,i;
	int prio0=0,prio1=0,prio2=0,prio3=0,prioh=0;
//	int grabbed=0;
	ogl_texture* t;
for (i = 0, t=oglTextureList; i < OGL_TEXTURE_LIST_SIZE; i++, t++) {
	if (t->handle) {
		used++;
		datatexel += t->w * t->h;
		truetexel += t->tw * t->th;
		databytes += t->bytesu;
		truebytes += t->bytes;
		if (t->prio < 0.299) prio0++;
		else if (t->prio < 0.399) prio1++;
		else if (t->prio < 0.499) prio2++;
		else if (t->prio < 0.599) prio3++;
		else prioh++;
	}
//		else if (t->w!=0)
//			grabbed++;
	}
if (gr_renderstats && Gamefonts) {
	GrPrintF (5,GAME_FONT->ft_h*14+3*14,"%i (%i,%i) %iK (%iK wasted)",used,usedrgba,usedl4a4,truebytes/1024, (truebytes-databytes)/1024);
	}
return truebytes;
}

//------------------------------------------------------------------------------

int ogl_mem_target = -1;

void ogl_clean_texture_cache (void)
{
	ogl_texture* t;
	int i,bytes;
	int time=120;
	
if (ogl_mem_target < 0) {
	if (gr_renderstats)
		ogl_texture_stats ();
	return;
	}
	
bytes=ogl_texture_stats ();
while (bytes>ogl_mem_target){
	for (i = 0, t = oglTextureList; i < OGL_TEXTURE_LIST_SIZE; i++, t++) {
		if (t->handle > 0) {
			if (t->lastrend + f1_0 * time < gameData.time.xGame) {
				OglFreeTexture (t);
				bytes -= t->bytes;
				if (bytes < ogl_mem_target)
					return;
				}
			}
		}
	if (time == 0)
		Error ("not enough mem?");
	time /= 2;
	}
}

//------------------------------------------------------------------------------

int ogl_bindteximage (ogl_texture *tex)
{
#if RENDER2TEXTURE == 1
#	if 1
OGL_BINDTEX (tex->pbuffer.texId);
#		if 1
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR /*GL_LINEAR_MIPMAP_LINEAR*/);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#		else
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#		endif
OGL_BINDTEX (tex->pbuffer.texId);
#	endif
#	ifdef _WIN32
#		ifdef _DEBUG
if (!tex->pbuffer.bBound) {
	tex->pbuffer.bBound = wglBindTexImageARB (tex->pbuffer.hBuf, WGL_FRONT_LEFT_ARB);
	if (!tex->pbuffer.bBound) {
		char *psz = (char *) gluErrorString (glGetError ());
		return 1;
		}
	}
#		else
if (!(tex->pbuffer.bBound ||
	 (tex->pbuffer.bBound = wglBindTexImageARB (tex->pbuffer.hBuf, WGL_FRONT_LEFT_ARB))))
	return 1;
#		endif
#	endif
#elif RENDER2TEXTURE == 2
OGL_BINDTEX (tex->fbuffer.texId);
#endif
return 0;
}

//------------------------------------------------------------------------------

int OglBindBmTex (grsBitmap *bmP, int nTransp)
{
	ogl_texture	*tex;
#if RENDER2TEXTURE
	int			bPBuffer;
#endif

bmP = BmOverride (bmP);
tex = bmP->glTexture;
#if RENDER2TEXTURE
if (bPBuffer = tex && (tex->handle < 0)) {
	if (ogl_bindteximage (tex))
		return 1;
	}
else
#endif
	{
	if (!(tex && (tex->handle > 0))) {
		if (OglLoadBmTexture (bmP, 1, nTransp))
			return 1;
		bmP = BmOverride (bmP);
		tex = bmP->glTexture;
#if 1//def _DEBUG
		if (!tex)
			OglLoadBmTexture (bmP, 1, nTransp);
#endif
		}
	OGL_BINDTEX (tex->handle);
	}
bmP->glTexture->lastrend = gameData.time.xGame;
bmP->glTexture->numrend++;
return 0;
}

//------------------------------------------------------------------------------

tRgbColorf bitmapColors [MAX_BITMAP_FILES];

tRgbColorf *BitmapColor (grsBitmap *bmP, ubyte *bufP)
{
	int c, h, i, j = 0, r = 0, g = 0, b = 0;
	tRgbColorf *color;
	ubyte	*palette;
		
h = (int) (bmP - gameData.pig.tex.pBitmaps);
if (!bufP || (h < 0) || (h >= MAX_BITMAP_FILES))
	return NULL;
color = bitmapColors + h;
if (color->red || color->green || color->blue)
	return color;
palette = bmP->bm_palette;
if (!palette)
	palette = defaultPalette;
for (h = i = bmP->bm_props.w * bmP->bm_props.h; i; i--, bufP++) {
	if ((c = *bufP) && (c != TRANSPARENCY_COLOR) && (c != SUPER_TRANSP_COLOR)) {
		c *= 3;
		r += palette [c];
		g += palette [c+1];
		b += palette [c+2];
		j++;
		}
	}
if (j) {
	bmP->bm_avgRGB.red = 4 * (ubyte) (r / j);
	bmP->bm_avgRGB.green = 4 * (ubyte) (g / j);
	bmP->bm_avgRGB.blue = 4 * (ubyte) (b / j);
	j *= 63;	//palette entries are all /4, so do not divide by 256
	color->red = (float) r / (float) j;
	color->green = (float) g / (float) j;
	color->blue = (float) b / (float) j;
	}
else {
	bmP->bm_avgRGB.red =
	bmP->bm_avgRGB.green =
	bmP->bm_avgRGB.blue = 0;
	color->red =
	color->green =
	color->blue = 0.0f;
	}
if (color->red > 1.0 || color->green > 1.0 || color->blue > 1.0)
	c = c;
return color;
}

//------------------------------------------------------------------------------

int GrAvgColor (grsBitmap *bmP)
{
if (bmP->bm_texBuf && !bmP->bm_avgColor) {
		int c, h, i, j = 0, r = 0, g = 0, b = 0;
		ubyte *p = bmP->bm_texBuf;
		ubyte *palette = bmP->bm_palette;

	for (h = i = bmP->bm_props.w * bmP->bm_props.h; i; i--, p++) {
		if ((c = *p) && (c != TRANSPARENCY_COLOR) && (c != SUPER_TRANSP_COLOR)) {
			c *= 3;
			r += palette [c++];
			g += palette [c++];
			b += palette [c];
			j++;
			}
		}
	return j ? GrFindClosestColor (bmP->bm_palette, r / j, g / j, b / j) : 0;
	}
return 0;
}

//------------------------------------------------------------------------------
//crude texture precaching
//handles: powerups, walls, weapons, polymodels, etc.
//it is done with the horrid DoSpecialEffects kludge so that sides that have to be texmerged and have animated textures will be correctly cached.
//similarly, with the gameData.objs.objects (esp weapons), we could just go through and cache em all instead, but that would get ones that might not even be on the level
//TODO: doors

void OglCachePolyModelTextures (int nModel)
{
	tPolyModel		*po = gameData.models.polyModels + nModel;
	int				h, i, j;
	tBitmapIndex	bmi;

for (i = po->nTextures, j = po->nFirstTexture; i; i--, j++) {
//		gameData.models.textureIndex [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [po->nFirstTexture+i]];
	h = gameData.pig.tex.pObjBmIndex [j];
	bmi = gameData.pig.tex.objBmIndex [h];
	PIGGY_PAGE_IN (bmi, 0);
	OglLoadBmTexture (gameData.pig.tex.bitmaps [0] + bmi.index, 1, 0);
	}
}

//------------------------------------------------------------------------------

void OglCacheVClipTextures (tVideoClip *vc, int nTransp)
{
	int i;

for (i = 0; i < vc->nFrameCount; i++) {
	PIGGY_PAGE_IN (vc->frames [i], 0);
	OglLoadBmTexture (gameData.pig.tex.bitmaps [0] + vc->frames [i].index, 1, nTransp);
	}
}

//------------------------------------------------------------------------------

#define OglCacheVClipTexturesN(i,t) OglCacheVClipTextures (gameData.eff.vClips [0] + i, t)

void OglCacheWeaponTextures (tWeaponInfo *w)
{
	OglCacheVClipTexturesN (w->flash_vclip, 1);
	OglCacheVClipTexturesN (w->robot_hit_vclip, 1);
	OglCacheVClipTexturesN (w->wall_hit_vclip, 1);
	if (w->renderType == WEAPON_RENDER_VCLIP)
		OglCacheVClipTexturesN (w->weapon_vclip, 0);
	else if (w->renderType == WEAPON_RENDER_POLYMODEL)
		OglCachePolyModelTextures (w->nModel);
}

//------------------------------------------------------------------------------

void OglResetFrames (grsBitmap *bmP)
{
OglFreeBmTexture (bmP);
}

//------------------------------------------------------------------------------

grsBitmap *LoadFaceBitmap (short tMapNum, short nFrameNum)
{
	grsBitmap	*bmP;
	int			nFrames;

PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tMapNum], gameStates.app.bD1Mission);
bmP = gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [tMapNum].index;
if (BM_OVERRIDE (bmP)) {
	bmP = BM_OVERRIDE (bmP);
	if (bmP->bm_wallAnim) {
		nFrames = BM_FRAMECOUNT (bmP);
		if (nFrames > 1) {
			OglLoadBmTexture (bmP, 1, 0);
			if (BM_FRAMES (bmP)) {
				if ((nFrameNum >= 0) || (-nFrames > nFrameNum))
					BM_CURFRAME (bmP) = BM_FRAMES (bmP);
				else
					BM_CURFRAME (bmP) = BM_FRAMES (bmP) - nFrameNum - 1;
				OglLoadBmTexture (BM_CURFRAME (bmP), 1, 0);
				}
			}
		}
	}
return bmP;
}

//------------------------------------------------------------------------------

void CacheSideTextures (int nSeg)
{
	short			nSide, tMap1, tMap2;
	grsBitmap	*bmP, *bm2, *bmm;
	struct tSide	*sideP;

for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	sideP = gameData.segs.segments [nSeg].sides + nSide;
	tMap1 = sideP->nBaseTex;
	if ((tMap1 < 0) || (tMap1 >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
		continue;
	bmP = LoadFaceBitmap (tMap1, sideP->nFrame);
	if (tMap2 = sideP->nOvlTex) {
		bm2 = LoadFaceBitmap (tMap1, sideP->nFrame);
		if (!(bm2->bm_props.flags & BM_FLAG_SUPER_TRANSPARENT) ||
			 (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk))
			OglLoadBmTexture (bm2, 1, 0);
		else if (bmm = TexMergeGetCachedBitmap (tMap1, tMap2, sideP->nOvlOrient))
			bmP = bmm;
		else
			OglLoadBmTexture (bm2, 1, 0);
		}
	OglLoadBmTexture (bmP, 1, 0);
	}
}

//------------------------------------------------------------------------------

void CacheObjTextures (int nObj)
{
if (nObj == -3) {
	ResetSpecialEffects ();
	InitSpecialEffects ();
	}
else if (nObj == -2) 
	OglCacheWeaponTextures (gameData.weapons.info + primaryWeaponToWeaponInfo [LASER_INDEX]);
else if (nObj == -1)
	OglCacheWeaponTextures (gameData.weapons.info + secondaryWeaponToWeaponInfo [CONCUSSION_INDEX]);
else if (gameData.objs.objects [nObj].renderType == RT_POWERUP) {
	OglCacheVClipTexturesN (gameData.objs.objects [nObj].rType.vClipInfo.nClipIndex, 0);
	switch (gameData.objs.objects [nObj].id){
/*					case POW_LASER:
			OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [LASER_INDEX]]);
//						if (laserlev<4)
//							laserlev++;
			break;*/
		case POW_VULCAN_WEAPON:
			OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [VULCAN_INDEX]]);
			break;
		case POW_SPREADFIRE_WEAPON:
			OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [SPREADFIRE_INDEX]]);
			break;
		case POW_PLASMA_WEAPON:
			OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [PLASMA_INDEX]]);
			break;
		case POW_FUSION_WEAPON:
			OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [FUSION_INDEX]]);
			break;
/*					case POW_MISSILE_1:
		case POW_MISSILE_4:
			OglCacheWeaponTextures (&gameData.weapons.info [secondaryWeaponToWeaponInfo [CONCUSSION_INDEX]]);
			break;*/
		case POW_PROXIMITY_WEAPON:
			OglCacheWeaponTextures (&gameData.weapons.info [secondaryWeaponToWeaponInfo [PROXIMITY_INDEX]]);
			break;
		case POW_HOMING_AMMO_1:
		case POW_HOMING_AMMO_4:
			OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [HOMING_INDEX]]);
			break;
		case POW_SMARTBOMB_WEAPON:
			OglCacheWeaponTextures (&gameData.weapons.info [secondaryWeaponToWeaponInfo [SMART_INDEX]]);
			break;
		case POW_MEGA_WEAPON:
			OglCacheWeaponTextures (&gameData.weapons.info [secondaryWeaponToWeaponInfo [MEGA_INDEX]]);
			break;
		}
	}
else if (gameData.objs.objects [nObj].renderType == RT_POLYOBJ)
	OglCachePolyModelTextures (gameData.objs.objects [nObj].rType.polyObjInfo.nModel);
}

//------------------------------------------------------------------------------

static int nCacheSeg = 0;
static int nCacheObj = -3;

static void TexCachePoll (int nItems, tMenuItem *m, int *key, int cItem)
{
if (nCacheSeg < gameData.segs.nSegments)
	CacheSideTextures (nCacheSeg++);
else if (nCacheObj <= gameData.objs.nLastObject) 
	CacheSideTextures (nCacheObj++);
else {
	*key = -2;
	return;
	}
m [0].value++;
m [0].rebuild = 1;
*key = 0;
return;
}

//------------------------------------------------------------------------------

int OglCacheTextures (void)
{
	tMenuItem	m [3];
	int i;

memset (m, 0, sizeof (m));
ADD_GAUGE (0, "                    ", 0, gameData.segs.nSegments + gameData.objs.nLastObject + 4); 
m [2].centered = 1;
nCacheSeg = 0;
nCacheObj = -3;
do {
	i = ExecMenu2 (NULL, "Loading textures", 1, m, (void (*)) TexCachePoll, 0, NULL);
	} while (i >= 0);
return 1;
}

//------------------------------------------------------------------------------

int OglCacheLevelTextures (void)
{
	int				i, bD1;
	eclip				*ec;
	int				max_efx=0,ef;
#if 1
	int				seg, side;
	short				tmap1,tmap2;
	grsBitmap		*bmP,*bm2, *bmm;
	struct tSide	*sideP;
#endif

OglResetTextureStatsInternal ();//loading a new lev should reset textures
TexMergeClose ();
TexMergeInit (100);

for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	for (i = 0,ec = gameData.eff.effects [bD1]; i < gameData.eff.nEffects [bD1];i++,ec++) {
		if ((ec->changing_wall_texture == -1) && (ec->changingObject_texture == -1))
			continue;
		if (ec->vc.nFrameCount > max_efx)
			max_efx = ec->vc.nFrameCount;
		}
	for (ef = 0; ef < max_efx; ef++)
		for (i = 0,ec = gameData.eff.effects [bD1]; i < gameData.eff.nEffects [bD1]; i++, ec++) {
			if ((ec->changing_wall_texture == -1) && (ec->changingObject_texture == -1))
				continue;
	//			if (ec->vc.nFrameCount>max_efx)
	//				max_efx=ec->vc.nFrameCount;
			ec->time_left = -1;
			}
	}
DoSpecialEffects ();

#if 0
OglCacheTextures ();
#else
for (seg = 0; seg < gameData.segs.nSegments; seg++) {
	for (side = 0; side < MAX_SIDES_PER_SEGMENT; side++) {
		sideP = gameData.segs.segments [seg].sides + side;
		tmap1 = sideP->nBaseTex;
		if ((tmap1 < 0) || (tmap1 >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
			continue;
		bmP = LoadFaceBitmap (tmap1, sideP->nFrame);
		if (tmap2 = sideP->nOvlTex) {
			bm2 = LoadFaceBitmap (tmap1, sideP->nFrame);
			if (!(bm2->bm_props.flags & BM_FLAG_SUPER_TRANSPARENT) ||
				 (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk))
				OglLoadBmTexture (bm2, 1, 0);
			else if (bmm = TexMergeGetCachedBitmap (tmap1, tmap2, sideP->nOvlOrient))
				bmP = bmm;
			else
				OglLoadBmTexture (bm2, 1, 0);
			}
		OglLoadBmTexture (bmP, 1, 0);
		}
	}
ResetSpecialEffects ();
InitSpecialEffects ();
{
//		int laserlev=1;
	//always have lasers and concs
	OglCacheWeaponTextures (gameData.weapons.info + primaryWeaponToWeaponInfo [LASER_INDEX]);
	OglCacheWeaponTextures (gameData.weapons.info + secondaryWeaponToWeaponInfo [CONCUSSION_INDEX]);
	for (i = 0; i <= gameData.objs.nLastObject; i++) {
		if (gameData.objs.objects [i].renderType == RT_POWERUP) {
			OglCacheVClipTexturesN (gameData.objs.objects [i].rType.vClipInfo.nClipIndex, 0);
			switch (gameData.objs.objects [i].id) {
/*					case POW_LASER:
					OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [LASER_INDEX]]);
//						if (laserlev<4)
//							laserlev++;
					break;*/
#if 1//def _DEBUG
				case POW_HOARD_ORB:
					OglCacheVClipTexturesN (gameData.objs.objects [i].rType.vClipInfo.nClipIndex, 0);
					break;
#endif
				case POW_VULCAN_WEAPON:
					OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [VULCAN_INDEX]]);
					break;
				case POW_SPREADFIRE_WEAPON:
					OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [SPREADFIRE_INDEX]]);
					break;
				case POW_PLASMA_WEAPON:
					OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [PLASMA_INDEX]]);
					break;
				case POW_FUSION_WEAPON:
					OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [FUSION_INDEX]]);
					break;
/*					case POW_MISSILE_1:
				case POW_MISSILE_4:
					OglCacheWeaponTextures (&gameData.weapons.info [secondaryWeaponToWeaponInfo [CONCUSSION_INDEX]]);
					break;*/
				case POW_PROXIMITY_WEAPON:
					OglCacheWeaponTextures (&gameData.weapons.info [secondaryWeaponToWeaponInfo [PROXIMITY_INDEX]]);
					break;
				case POW_HOMING_AMMO_1:
				case POW_HOMING_AMMO_4:
					OglCacheWeaponTextures (&gameData.weapons.info [primaryWeaponToWeaponInfo [HOMING_INDEX]]);
					break;
				case POW_SMARTBOMB_WEAPON:
					OglCacheWeaponTextures (&gameData.weapons.info [secondaryWeaponToWeaponInfo [SMART_INDEX]]);
					break;
				case POW_MEGA_WEAPON:
					OglCacheWeaponTextures (&gameData.weapons.info [secondaryWeaponToWeaponInfo [MEGA_INDEX]]);
					break;
				default:
					break;
				}
			}
		else if (gameData.objs.objects [i].renderType == RT_POLYOBJ)
			OglCachePolyModelTextures (gameData.objs.objects [i].rType.polyObjInfo.nModel);
		}
	}
#endif
return 0;
}

//------------------------------------------------------------------------------

int tex_format_supported (int iformat,int format)
{
	switch (iformat){
		case GL_INTENSITY4:
			if (!gameOpts->ogl.bIntensity4) 
				return 0; 
			break;

		case GL_LUMINANCE4_ALPHA4:
			if (!gameOpts->ogl.bLuminance4Alpha4) 
				return 0; 
			break;

		case GL_RGBA2:
			if (!gameOpts->ogl.bRgba2) 
				return 0; 
			break;
		}
	if (gameOpts->ogl.bGetTexLevelParam){
		GLint internalFormat;
		glTexImage2D (GL_PROXY_TEXTURE_2D, 0, iformat, 64, 64, 0,
				format, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);//NULL?
		glGetTexLevelParameteriv (GL_PROXY_TEXTURE_2D, 0,
				GL_TEXTURE_INTERNAL_FORMAT,
				&internalFormat);
		return (internalFormat==iformat);
		}
	else
		return 1;
}


//------------------------------------------------------------------------------
//little hack to find the largest or equal multiple of 2 for a given number
int pow2ize (int x)
{
	int i;
	for (i = 2; i <= 4096; i *= 2)
		if (x <= i) 
			return i;
	return i;
}

//------------------------------------------------------------------------------

int Luminance (int r, int g, int b)
{
	int minColor, maxColor;

if (r < g) {
	minColor = (r < b) ? r : b;
	maxColor = (g > b) ? g : b;
	}
else {
	minColor = (g < b) ? g : b;
	maxColor = (r > b) ? r : b;
	}
return (minColor + maxColor) / 2;
}

//------------------------------------------------------------------------------
//GLubyte gameData.render.ogl.texBuf [512*512*4];
void OglFillTexBuf (
	grsBitmap *bmP,
	GLubyte *texp,
	int truewidth,
	int width,
	int height,
	int dxo,
	int dyo,
	int twidth,
	int theight,
	int nType,
	int nTransp,
	int superTransp)
{
//	GLushort *tex= (GLushort *)texp;
	unsigned char *data = bmP->bm_texBuf;
	int x, y, c, i;
	int bShaderMerge = gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk;
	gameData.render.ogl.palette = (BM_PARENT (bmP) ? BM_PARENT (bmP)->bm_palette : bmP->bm_palette);

	if (!gameData.render.ogl.palette)
		gameData.render.ogl.palette = defaultPalette;
	if (!gameData.render.ogl.palette)
		return;
	if (twidth * theight * 4 > sizeof (gameData.render.ogl.texBuf))//shouldn't happen, descent never uses textures that big.
		Error ("texture too big %i %i",twidth,theight);

	i = 0;
	for (y = 0; y < theight; y++) {
		i = dxo + truewidth * (y + dyo);
		for (x = 0; x < twidth; x++){
			if ((x < width) && (y < height))
				c = data [i++];
			else
				c = TRANSPARENCY_COLOR;//fill the pad space with transparancy
			if ((int) c == TRANSPARENCY_COLOR) {
				switch (nType) {
					case GL_LUMINANCE:
						 (*(texp++)) = 0;
						break;
					case GL_LUMINANCE_ALPHA:
#if 1
						* ((GLushort *) texp) = 0;
						texp += 2; 
#else
						 (*(texp++)) = 0;
						 (*(texp++)) = 0;
#endif
						break;
					case GL_RGBA:
#if 1
						* ((GLuint *) texp) = 0;
						texp += 4;
#else
						 (*(texp++)) = 0;
						 (*(texp++)) = 0;
						 (*(texp++)) = 0;
						 (*(texp++)) = 0;//transparent pixel
#endif
						break;
					}
//				 (*(tex++)) = 0;
				}
			else {
				switch (nType) {
					case GL_LUMINANCE://these could prolly be done to make the intensity based upon the intensity of the resulting color, but its not needed for anything (yet?) so no point. :)
						 (*(texp++)) = 255;
						break;
					case GL_LUMINANCE_ALPHA:
						 (*(texp++)) = 255;
						 (*(texp++)) = 255;
						break;
					case GL_RGBA:
						if (superTransp && (c == SUPER_TRANSP_COLOR)) {
							if (bShaderMerge) {
								(*(texp++)) = 0;
								(*(texp++)) = 0;
								(*(texp++)) = 0;
								(*(texp++)) = 1;
								}
							else {
								(*(texp++)) = 120;
								(*(texp++)) =  88;
								(*(texp++)) = 128;
								(*(texp++)) = 0;
								}
							}
						else {
							int h = 0, i = 0, j = c * 3;
							int r = gameData.render.ogl.palette [j] * 4;
							int g = gameData.render.ogl.palette [j + 1] * 4;
							int b = gameData.render.ogl.palette [j + 2] * 4;
							(*(texp++)) = r;
							(*(texp++)) = g;
							(*(texp++)) = b;
#if 0
							if (r) {
								h++;
								i += 3;
								}
							if (g) {
								h++;
								i += 5;
								}
							if (b) {
								h++;
								i += 2;
								}
#endif
							if (nTransp == 1) {
#if 0
								 (*(texp++)) =  (r + g + b) / 3;//transparency based on color intensity
#else //chromatic intensity considered
#	if 0 //non-linear formula
								{
								double a = (double) (r * 3 + g * 5 + b * 2) / (10.0 * 255.0);
								a *= a;
								(*(texp++)) = (ubyte) (a * 255.0);
								}
#	else
								(*(texp++)) = (r * 3 + g * 5 + b * 2) / 10;//transparency based on color intensity
#	endif
#endif
								}
#if 1
							else if (nTransp == 2)
								(*(texp++)) = c ? 255 : 0;
#endif
							else
								(*(texp++)) = 255;//not transparent
							}
						//				 (*(tex++)) =  (gameData.render.ogl.palette [c*3]>>1) + ((gameData.render.ogl.palette [c*3+1]>>1)<<5) + ((gameData.render.ogl.palette [c*3+2]>>1)<<10) + (1<<15);
						break;
					}
			}
		}
	}
}

//------------------------------------------------------------------------------

int tex_format_verify (ogl_texture *tex)
{
	while (!tex_format_supported (tex->internalformat,tex->format)){
		switch (tex->internalformat){
			case GL_INTENSITY4:
				if (gameOpts->ogl.bLuminance4Alpha4){
					tex->internalformat=GL_LUMINANCE4_ALPHA4;
					tex->format=GL_LUMINANCE_ALPHA;
					break;
				}//note how it will fall through here if the statement is false
			case GL_LUMINANCE4_ALPHA4:
				if (gameOpts->ogl.bRgba2){
					tex->internalformat=GL_RGBA2;
					tex->format=GL_RGBA;
					break;
				}//note how it will fall through here if the statement is false
			case GL_RGBA2:
				tex->internalformat=gameOpts->ogl.bRgbaFormat;
				tex->format=GL_RGBA;
				break;
			default:
#if TRACE	
				con_printf (CON_DEBUG,"...no tex format to fall back on\n");
#endif
				return 1;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------

void TexSetSizeSub (ogl_texture *tex,int dbits,int bits,int w, int h)
{
	int u;

if (tex->tw != w || tex->th != h)
	u = (int) ((tex->w / (double) tex->tw * w) * (tex->h / (double) tex->th * h));
else
	u = (int) (tex->w * tex->h);
if (bits <= 0) //the beta nvidia GLX server. doesn't ever return any bit sizes, so just use some assumptions.
	bits = dbits;
tex->bytes = (int) (((double) w * h * bits) / 8.0);
tex->bytesu = (int) (((double) u * bits) / 8.0);
}

//------------------------------------------------------------------------------

void TexSetSize (ogl_texture *tex)
{
	GLint	w, h;
	int	bi = 16, a = 0;

if (gameOpts->ogl.bGetTexLevelParam) {
		GLint t;

	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_LUMINANCE_SIZE, &t);
	a += t;
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTENSITY_SIZE, &t);
	a += t;
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &t);
	a += t;
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &t);
	a += t;
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_SIZE, &t);
	a += t;
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &t);
	a += t;
	}
else {
	w = tex->tw;
	h = tex->th;
}
switch (tex->format) {
	case GL_LUMINANCE:
		bi=8;
		break;
	case GL_LUMINANCE_ALPHA:
		bi=8;
		break;
	case GL_RGBA:
		bi=32;
		break;
	case GL_ALPHA:
		bi=8;
		break;
	default:
		Error ("TexSetSize unknown texformat\n");
		break;
	}
TexSetSizeSub (tex, bi , a, w, h);
}

//------------------------------------------------------------------------------
//loads a palettized bitmap into a ogl RGBA texture.
//Sizes and pads dimensions to multiples of 2 if necessary.
//In theory this could be a problem for repeating textures, but all real
//textures (not sprites, etc) in descent are 64x64, so we are ok.
//stores OpenGL textured id in *texid and u/v values required to get only the real data in *u/*v
int OglLoadTexture (grsBitmap *bmP, int dxo, int dyo, ogl_texture *tex, int nTransp, int superTransp)
{
	unsigned char *data = bmP->bm_texBuf;
	GLubyte	*bufP = gameData.render.ogl.texBuf;
//	int internalformat=GL_RGBA;
//	int format=GL_RGBA;
//int filltype=0;
tex->tw = pow2ize (tex->w);
tex->th = pow2ize (tex->h);//calculate smallest texture size that can accomodate us (must be multiples of 2)
//	tex->tw=tex->w;tex->th=tex->h;//feeling lucky?

if (gr_badtexture > 0) 
	return 1;

#ifndef __macosx__
// always fails on OS X, but textures work fine!
//if (nTransp >= 0)
if (tex_format_verify (tex))
	return 1;
#endif

//calculate u/v values that would make the resulting texture correctly sized
tex->u = (float) ((double) tex->w / (double) tex->tw);
tex->v = (float) ((double) tex->h / (double) tex->th);
//	if (width!=twidth || height!=theight)
#if RENDER2TEXTURE
if (tex->handle < 0) 
	{
#if 0
	if (ogl_bindteximage (tex))
		return 1;
#	endif
	}
else
#endif
	{
	if (data)
		if (nTransp >= 0)
			OglFillTexBuf (bmP, gameData.render.ogl.texBuf, tex->lw, tex->w, tex->h, dxo, dyo, tex->tw, tex->th, 
								tex->format, nTransp, superTransp);
		else {
#if 0
			memcpy (bufP = gameData.render.ogl.texBuf, data, tex->w * tex->h * 4);
#else
			if (!dxo && !dyo && (tex->w == tex->tw) && (tex->h == tex->th))
				bufP = data;
			else {
				int h, w, tw;
				
				h = tex->lw / tex->w;
				w = (tex->w - dxo) * h;
				data += tex->lw * dyo + h * dxo;
				bufP = gameData.render.ogl.texBuf;
				tw = tex->tw * h;
				h = tw - w;
				for (; dyo < tex->h; dyo++, data += tex->lw) {
					memcpy (bufP, data, w);
					bufP += w;
					memset (bufP, 0, h);
					bufP += h;
					}
				memset (bufP, 0, tex->th * tw - (bufP - gameData.render.ogl.texBuf));
				bufP = gameData.render.ogl.texBuf;
				}
#endif
			}
	// Generate OpenGL texture IDs.
	glGenTextures (1, &tex->handle);
	//set priority
	glPrioritizeTextures (1, &tex->handle, &tex->prio);
	// Give our data to OpenGL.
	OGL_BINDTEX (tex->handle);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (tex->wantmip) {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gameStates.ogl.texMagFilter);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gameStates.ogl.texMinFilter);
		}
	else {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
//	bMipMap=0;//mipmaps aren't used in GL_NEAREST anyway, and making the mipmaps is pretty slow
//however, if texturing mode becomes an ingame option, they would need to be made regardless, so it could switch to them later.  OTOH, texturing mode could just be made a command line arg.
	if (tex->wantmip && gameStates.ogl.bNeedMipMaps)
		gluBuild2DMipmaps (
				GL_TEXTURE_2D, tex->internalformat, 
				tex->tw, tex->th, tex->format, 
				GL_UNSIGNED_BYTE, 
				bufP);
	else
		glTexImage2D (
			GL_TEXTURE_2D, 0, tex->internalformat,
			tex->tw, tex->th, 0, tex->format, // RGBA textures.
			GL_UNSIGNED_BYTE, // imageData is a GLubyte pointer.
			bufP);
	TexSetSize (tex);
	}
r_texcount++; 
return 0;
}

//------------------------------------------------------------------------------

unsigned char decodebuf [2048*2048];

#if RENDER2TEXTURE == 1
int OglLoadBmTextureM (grsBitmap *bmP, int bMipMap, int nTransp, int bMask, ogl_pbuffer *pb)
#elif RENDER2TEXTURE == 2
int OglLoadBmTextureM (grsBitmap *bmP, int bMipMap, int nTransp, int bMask, ogl_fbuffer *fb)
#else
int OglLoadBmTextureM (grsBitmap *bmP, int bMipMap, int nTransp, int bMask, void *pb)
#endif
{
	unsigned char	*buf;
	ogl_texture		*t;
	grsBitmap		*bmParent;

while ((bmP->bmType == BM_TYPE_STD) && (bmParent = BM_PARENT (bmP)) && (bmParent != bmP))
	bmP = bmParent;
buf = bmP->bm_texBuf;
if (!(t = bmP->glTexture)) {
	t = bmP->glTexture = OglGetFreeTexture ();
	if (!t)
		return 1;
	OglInitTexture (t, bMask);
	t->lw = bmP->bm_props.w;
	if (bmP->bm_props.flags & BM_FLAG_TGA)
		t->lw *= 4;
	t->w = bmP->bm_props.w;
	t->h = bmP->bm_props.h;
	t->wantmip = bMipMap && !bMask;
#if RENDER2TEXTURE == 1
	if (pb) {
		t->pbuffer = *pb;
		t->handle = -((GLint) pb->texId);
		}
#elif RENDER2TEXTURE == 2
	if (fb) {
		t->fbuffer = *fb;
		t->handle = -((GLint) fb->texId);
		}
#endif
	}
else {
	if (t->handle > 0)
		return 0;
	if (!t->w) {
		t->lw = bmP->bm_props.w;
		t->w = bmP->bm_props.w;
		t->h = bmP->bm_props.h;
		}
	}
#if 1
rle_expand (bmP, NULL, 0);
buf = bmP->bm_texBuf;
#else
if (bmP->bm_props.flags & BM_FLAG_RLE) {
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset, bBig = bmP->bm_props.flags & BM_FLAG_RLE_BIG;

	data_offset = bBig ? 2 : 1;
	sbits = bmP->bm_texBuf + 4 + (bmP->bm_props.h * data_offset);
	dbits = decodebuf;
	for (i = 0; i < bmP->bm_props.h; i++) {
		gr_rle_decode (sbits, dbits);
		if (bBig)
			sbits += (int) INTEL_SHORT (*((short *) (bmP->bm_texBuf + 4 + (i * data_offset))));
		else
			sbits += (int) bmP->bm_texBuf [4 + i];
		dbits += bmP->bm_props.w;
		}
	buf = decodebuf;
	}
#endif
#ifdef _DEBUG
if (bmP->bm_props.flags & BM_FLAG_TGA)
	bmP->bm_props.flags = bmP->bm_props.flags;
else 
#endif
	{
	tRgbColorf	*c;

	c = BitmapColor (bmP, buf);
	if (c && !bmP->bm_avgColor)
		bmP->bm_avgColor = GrFindClosestColor (bmP->bm_palette, (int) c->red, (int) c->green, (int) c->blue);
	}
OglLoadTexture (bmP, 0, 0, bmP->glTexture, (bmP->bm_props.flags & BM_FLAG_TGA) ? -1 : nTransp, 
					 (bmP->bm_props.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT)) != 0);
return 0;
}

//------------------------------------------------------------------------------

int OglLoadBmTexture (grsBitmap *bmP, int bDoMipMap, int nTransp)
{
	int			i, h, w, nFrames;

bmP = BmOverride (bmP);
h = bmP->bm_props.h;
w = bmP->bm_props.w;
if (!(h * w))
	return 1;
nFrames = (bmP->bmType == BM_TYPE_ALT) ? BM_FRAMECOUNT (bmP) : 0;
if (!(bmP->bm_props.flags & BM_FLAG_TGA) || (nFrames < 2)) {
	if (OglLoadBmTextureM (bmP, bDoMipMap, nTransp, 0, NULL))
		return 1;
	if (CreateSuperTranspMasks (bmP) && OglLoadBmTextureM (BM_MASK (bmP), 0, -1, 1, NULL))
		return 1;
	}
else if (!BM_FRAMES (bmP)) {
	grsBitmap	*bmfP;
	bmfP = (grsBitmap *) d_malloc (nFrames * sizeof (struct _grsBitmap));
	memset (bmfP, 0, nFrames * sizeof (struct _grsBitmap));
	BM_FRAMES (bmP) = bmfP;
	for (i = 0; i < nFrames; i++, bmfP++) {
		BM_CURFRAME (bmP) = bmfP;
		GrInitSubBitmap (bmfP, bmP, 0, i * w, w, w);
		bmfP->bmType = BM_TYPE_FRAME;
		bmfP->bm_props.y = 0;
		BM_PARENT (bmfP) = bmP;
		if (bmP->bm_data.alt.bm_transparentFrames [i / 32] & (1 << (i % 32)))
			bmfP->bm_props.flags |= BM_FLAG_TRANSPARENT;
		if (bmP->bm_data.alt.bm_supertranspFrames [i / 32] & (1 << (i % 32)))
			bmfP->bm_props.flags |= BM_FLAG_SUPER_TRANSPARENT;
		OglLoadBmTextureM (bmfP, bDoMipMap, nTransp, 0, NULL);
		}
	BM_CURFRAME (bmP) = BM_FRAMES (bmP);
	if (CreateSuperTranspMasks (bmP))
		for (i = nFrames, bmfP = BM_FRAMES (bmP); i; i--, bmfP++)
			if (BM_MASK (bmfP))
				OglLoadBmTextureM (BM_MASK (bmfP), 0, -1, 1, NULL);
	}
else
	CBRK (!BM_CURFRAME (bmP));
return 0;
}

//------------------------------------------------------------------------------

void OglFreeTexture (ogl_texture *t)
{
if (t) {
	GLint	h = t->handle;
	if (h) {
		r_texcount--;
		glDeleteTextures (1, &h);
		OglInitTexture (t, 0);
		}
	}
}

//------------------------------------------------------------------------------

void OglFreeBmTexture (grsBitmap *bmP)
{
	ogl_texture	*t;

while ((bmP->bmType != BM_TYPE_ALT) && BM_PARENT (bmP) && (bmP != BM_PARENT (bmP)))
	bmP = BM_PARENT (bmP);
if (BM_FRAMES (bmP)) {
	int i, nFrames = bmP->bm_props.h / bmP->bm_props.w;

	for (i = 0; i < nFrames; i++) {
		OglFreeTexture (BM_FRAMES (bmP) [i].glTexture);
		BM_FRAMES (bmP) [i].glTexture = NULL;
		}
	}
else if (t = bmP->glTexture) {
#if RENDER2TEXTURE == 2
	if (t->handle < 0)
		OGL_BINDTEX (0);
	else
#elif RENDER2TEXTURE == 1
#	ifdef _WIN32
	if (t->handle < 0) {
		if (t->pbuffer.bBound) {
			if (wglReleaseTexImageARB (t->pbuffer.hBuf, WGL_FRONT_LEFT_ARB))
				t->pbuffer.bBound = 0;
#		ifdef _DEBUG
			else {
				char *psz = (char *) gluErrorString (glGetError ());
				}
#		endif
			}
		}
	else
#	endif
#endif
		{
		OglFreeTexture (t);
		bmP->glTexture = NULL;
		}
	}
}

//------------------------------------------------------------------------------

GLuint EmptyTexture (int Xsize, int Ysize)			// Create An Empty Texture
{
	GLuint texId; 						// Texture ID
	int nSize = Xsize * Ysize * 4 * sizeof (unsigned int); 
	// Create Storage Space For Texture Data (128x128x4)
	unsigned int *data = (unsigned int*) d_malloc (nSize); 

memset (data, 0, nSize); 	// Clear Storage Memory
glGenTextures (1, &texId); 					// Create 1 Texture
OGL_BINDTEX (texId); 			// Bind The Texture
glTexImage2D (GL_TEXTURE_2D, 0, 4, Xsize, Ysize, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); 			// Build Texture Using Information In data
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
d_free (data); 							// Release data
return texId; 						// Return The Texture ID
}

//------------------------------------------------------------------------------

char *LoadShader (char* fileName) //, char* Shadersource)
{
	FILE *fp; 
	char *bufP = NULL; 
	int f, fSize; 
	char fn [FILENAME_LEN];

if (!(fileName && *fileName))
	return NULL;	// no fileName

sprintf (fn, "%s%s%s", gameFolders.szShaderDir, *gameFolders.szShaderDir ? "/" : "", fileName);
#ifdef _WIN32
if (0 > (f = _open (fn, _O_RDONLY)))
	return NULL;	// couldn't open file
fSize = _lseek (f, 0, SEEK_END);
_close (f); 
if (fSize <= 0)
	return NULL;	// empty file or seek error
#endif

if (!(fp = fopen (fn, "rt")))
	return NULL;	// couldn't open file

#ifndef _WIN32
fseek (fp, 0, SEEK_END);
fSize = ftell (fp);
if (fSize <= 0) {
	fclose (fp);
	return NULL;	// empty file or seek error
	}
#endif

if (!(bufP = (char *) d_malloc (sizeof (char) * (fSize + 1)))) {
	fclose (fp);
	return NULL;	// out of memory
	}
fSize = (int) fread (bufP, sizeof (char), fSize, fp); 
bufP [fSize] = '\0'; 
fclose (fp); 
return bufP; 
}

//------------------------------------------------------------------------------

#ifdef GL_VERSION_20

void PrintShaderInfoLog (GLuint handle, int bProgram)
{
   int nLogLen = 0;
   int charsWritten = 0;
   char *infoLog;

if (bProgram) {
	glGetProgramiv (handle, GL_INFO_LOG_LENGTH, &nLogLen);
	if ((nLogLen > 0) && (infoLog = (char *) d_malloc (nLogLen))) {
		infoLog = (char *) d_malloc (nLogLen);
		glGetProgramInfoLog (handle, nLogLen, &charsWritten, infoLog);
		if (*infoLog)
			LogErr ("\n%s\n\n", infoLog);
		d_free (infoLog);
		}
	}
else {
	glGetShaderiv (handle, GL_INFO_LOG_LENGTH, &nLogLen);
	if ((nLogLen > 0) && (infoLog = (char *) d_malloc (nLogLen))) {
		glGetShaderInfoLog (handle, nLogLen, &charsWritten, infoLog);
		if (*infoLog)
			LogErr ("\n%s\n\n", infoLog);
		d_free (infoLog);
		}
	}
}

#else

void PrintShaderInfoLog (GLhandleARB handle, int bProgram)
{
   int nLogLen = 0;
   int charsWritten = 0;
   char *infoLog;

glGetObjectParameteriv (handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &nLogLen);
if ((nLogLen > 0) && (infoLog = (char *) d_malloc (nLogLen))) {
	glGetInfoLog (handle, nLogLen, &charsWritten, infoLog);
	if (*infoLog)
		LogErr ("\n%s\n\n", infoLog);
	d_free (infoLog);
	}
}

#endif

//------------------------------------------------------------------------------

char *progVS [] = {
	"void TexMergeVS ();" \
	"void main (void) {TexMergeVS ();}"
,
	"void LightingVS ();" \
	"void main (void) {LightingVS ();}"
,
	"void LightingVS ();" \
	"void TexMergeVS ();" \
	"void main (void) {TexMergeVS (); LightingVS ();}"
	};

char *progFS [] = {
	"void TexMergeFS ();" \
	"void main (void) {TexMergeFS ();}"
,
	"void LightingFS ();" \
	"void main (void) {LightingFS ();}"
,
	"void LightingFS ();" \
	"void TexMergeFS ();" \
	"void main (void) {TexMergeFS (); LightingFS ();}" 
	};

GLhandleARB mainVS = 0;
GLhandleARB mainFS = 0;

//------------------------------------------------------------------------------

GLhandleARB	genShaderProg = 0;

int CreateShaderProg (GLhandleARB *progP)
{
if (!progP)
	progP = &genShaderProg;
if (*progP)
	return 1;
*progP = glCreateProgramObject (); 
if (*progP)
	return 1;
LogErr ("   Couldn't create shader program tObject\n");
return 0; 
}

//------------------------------------------------------------------------------

void DeleteShaderProg (GLhandleARB *progP)
{
if (!progP)
	progP = &genShaderProg;
if (progP && *progP) {
	glDeleteObject (*progP);
	*progP = 0;
	}
}

//------------------------------------------------------------------------------

int CreateShaderFunc (GLhandleARB *progP, GLhandleARB *fsP, GLhandleARB *vsP, char *fsName, char *vsName, int bFromFile)
{
	GLhandleARB	fs, vs;
	GLint bFragCompiled, bVertCompiled; 

if (!gameStates.ogl.bShadersOk)
	return 0;
if (!CreateShaderProg (progP))
	return 0;
if (*fsP) {
	glDeleteObject (*fsP); 
	*fsP = 0;
	}
if (*vsP) {
	glDeleteObject (*vsP);
	*vsP = 0;
	}
vs = glCreateShaderObject (GL_VERTEX_SHADER_ARB); 
fs = glCreateShaderObject (GL_FRAGMENT_SHADER_ARB); 
#ifdef DBG_SHADERS	
if (bFromFile) {
	vsName = LoadShader (vsName); 
	fsName = LoadShader (fsName); 
	if (!vsName || !fsName) 
		return 0; 
	}
#endif
glShaderSource (vs, 1, (const GLcharARB **) &vsName, NULL); 
glShaderSource (fs, 1, (const GLcharARB **) &fsName, NULL); 
#ifdef DBG_SHADERS	
if (bFromFile) {
	d_free (vsName); 
	d_free (fsName); 
	}
#endif
glCompileShader (vs); 
glCompileShader (fs); 
glGetObjectParameteriv (vs, GL_OBJECT_COMPILE_STATUS_ARB, &bVertCompiled); 
glGetObjectParameteriv (fs, GL_OBJECT_COMPILE_STATUS_ARB, &bFragCompiled); 
if (!bVertCompiled || !bFragCompiled) {
	if (!bVertCompiled) {
		LogErr ("   Couldn't compile vertex shader\n   \"%s\"\n", vsName);
		PrintShaderInfoLog (vs, 0);
		}
	if (!bFragCompiled) {
		LogErr ("   Couldn't compile fragment shader\n   \"%s\"\n", fsName);
		PrintShaderInfoLog (fs, 0);
		}
	return 0; 
	}
glAttachObject (*progP, vs); 
glAttachObject (*progP, fs); 
*fsP = fs;
*vsP = vs;
return 1;
}

//------------------------------------------------------------------------------

int LinkShaderProg (GLhandleARB *progP)
{
	int	i = 0;
	GLint	bLinked;

if (!progP) {
	progP = &genShaderProg;
	if (!*progP)
		return 0;
	if (gameOpts->ogl.bGlTexMerge)
		i |= 1;
	if (gameOpts->ogl.bUseLighting)
		i |= 2;
	if (!i)
		return 0;
	if (!CreateShaderFunc (progP, &mainFS, &mainVS, progFS [i - 1], progVS [i - 1], 0)) {
		DeleteShaderProg (progP);
		return 0;
		}
	}
glLinkProgram (*progP); 
glGetObjectParameteriv (*progP, GL_OBJECT_LINK_STATUS_ARB, &bLinked); 
if (bLinked)
	return 1;
LogErr ("   Couldn't link shader programs\n");
PrintShaderInfoLog (*progP, 1);
DeleteShaderProg (progP);
return 0; 
}

//------------------------------------------------------------------------------

void InitShaders ()
{
DeleteShaderProg (NULL);
#if LIGHTMAPS
InitLightmapShaders ();
#endif
InitTexMergeShaders ();
InitLightingShaders ();
LinkShaderProg (NULL);
}

//------------------------------------------------------------------------------

void RebuildGfxFx (int bGame, int bCameras)
{
ResetTextures (1, bGame);
InitShaders ();
#if LIGHTMAPS
CreateLightMaps ();
#endif
if (bCameras) {
	DestroyCameras ();
	CreateCameras ();		
	}
}

//------------------------------------------------------------------------------

void OglInitExtensions (void)
{
char *pszOglExtensions = (char*) glGetString (GL_EXTENSIONS);

#if OGL_QUERY
if (!(pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_occlusion_query")))
	bOcclusionQuery = 0;
else {
#	ifndef GL_VERSION_20
	glGenQueriesARB        = (PFNGLGENQUERIESARBPROC) wglGetProcAddress ("glGenQueriesARB");
	glDeleteQueriesARB     = (PFNGLDELETEQUERIESARBPROC) wglGetProcAddress ("glDeleteQueriesARB");
	glIsQueryARB           = (PFNGLISQUERYARBPROC) wglGetProcAddress ("glIsQueryARB");
	glBeginQueryARB        = (PFNGLBEGINQUERYARBPROC) wglGetProcAddress ("glBeginQueryARB");
	glEndQueryARB          = (PFNGLENDQUERYARBPROC) wglGetProcAddress ("glEndQueryARB");
	glGetQueryivARB        = (PFNGLGETQUERYIVARBPROC) wglGetProcAddress ("glGetQueryivARB");
	glGetQueryObjectivARB  = (PFNGLGETQUERYOBJECTIVARBPROC) wglGetProcAddress ("glGetQueryObjectivARB");
	glGetQueryObjectuivARB = (PFNGLGETQUERYOBJECTUIVARBPROC) wglGetProcAddress ("glGetQueryObjectuivARB");
	bOcclusionQuery =
		glGenQueries && glDeleteQueries && glIsQuery && 
		glBeginQuery && glEndQuery && glGetQueryiv && 
		glGetQueryObjectiv && glGetQueryObjectuivARB;
#	else
	bOcclusionQuery = 1;
#	endif
	}
#endif

#if OGL_POINT_SPRITES
#	ifdef _WIN32
glPointParameterfvARB	= (PFNGLPOINTPARAMETERFVARBPROC) wglGetProcAddress ("glPointParameterfvARB");
glPointParameterfARB		= (PFNGLPOINTPARAMETERFARBPROC) wglGetProcAddress ("glPointParameterfARB");
#	endif
#endif

#ifdef RELEASE
#	ifdef _WIN32
glActiveStencilFaceEXT	= (PFNGLACTIVESTENCILFACEEXTPROC) wglGetProcAddress ("glActiveStencilFaceEXT");
#	endif
#endif

//This function initializes the multitexturing stuff.  Pixel Shader stuff should be put here eventually.
#if OGL_MULTI_TEXTURING
#	ifndef GL_VERSION_20
#		ifdef _WIN32
glMultiTexCoord2d			= (PFNGLMULTITEXCOORD2DARBPROC) wglGetProcAddress ("glMultiTexCoord2dARB");
glMultiTexCoord2f			= (PFNGLMULTITEXCOORD2FARBPROC) wglGetProcAddress ("glMultiTexCoord2fARB");
#		endif
glActiveTexture			= (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress ("glActiveTextureARB");		
glClientActiveTexture	= (PFNGLCLIENTACTIVETEXTUREARBPROC) wglGetProcAddress ("glClientActiveTextureARB");
bMultiTexturingOk =
	glMultiTexCoord2d && glActiveTexture && glClientActiveTexture;
#	else
bMultiTexturingOk = 1;
#	endif
#endif

if (!gameOpts->render.bUseShaders ||
	 !bMultiTexturingOk || 
	 !pszOglExtensions ||
	 !strstr (pszOglExtensions, "GL_ARB_shading_language_100") || 
	 !strstr (pszOglExtensions, "GL_ARB_shader_objects")) 
	gameStates.ogl.bShadersOk = 0;
else {
#ifndef GL_VERSION_20
	glCreateProgramObject		= (PFNGLCREATEPROGRAMOBJECTARBPROC) wglGetProcAddress ("glCreateProgramObjectARB");
	glDeleteObject					= (PFNGLDELETEOBJECTARBPROC) wglGetProcAddress ("glDeleteObjectARB");
	glUseProgramObject			= (PFNGLUSEPROGRAMOBJECTARBPROC) wglGetProcAddress ("glUseProgramObjectARB");
	glCreateShaderObject			= (PFNGLCREATESHADEROBJECTARBPROC) wglGetProcAddress ("glCreateShaderObjectARB");
	glShaderSource					= (PFNGLSHADERSOURCEARBPROC) wglGetProcAddress ("glShaderSourceARB");
	glCompileShader				= (PFNGLCOMPILESHADERARBPROC) wglGetProcAddress ("glCompileShaderARB");
	glGetObjectParameteriv		= (PFNGLGETOBJECTPARAMETERIVARBPROC) wglGetProcAddress ("glGetObjectParameterivARB");
	glAttachObject					= (PFNGLATTACHOBJECTARBPROC) wglGetProcAddress ("glAttachObjectARB");
	glGetInfoLog					= (PFNGLGETINFOLOGARBPROC) wglGetProcAddress ("glGetInfoLogARB");
	glLinkProgram					= (PFNGLLINKPROGRAMARBPROC) wglGetProcAddress ("glLinkProgramARB");
	glGetUniformLocation			= (PFNGLGETUNIFORMLOCATIONARBPROC) wglGetProcAddress ("glGetUniformLocationARB");
	glUniform4f						= (PFNGLUNIFORM4FARBPROC) wglGetProcAddress ("glUniform4fARB");
	glUniform3f						= (PFNGLUNIFORM3FARBPROC) wglGetProcAddress ("glUniform3fARB");
	glUniform1f						= (PFNGLUNIFORM1FARBPROC) wglGetProcAddress ("glUniform1fARB");
	glUniform4fv					= (PFNGLUNIFORM4FVARBPROC) wglGetProcAddress ("glUniform4fvARB");
	glUniform3fv					= (PFNGLUNIFORM3FVARBPROC) wglGetProcAddress ("glUniform3fvARB");
	glUniform1fv					= (PFNGLUNIFORM1FVARBPROC) wglGetProcAddress ("glUniform1fvARB");
	glUniform1i						= (PFNGLUNIFORM1IARBPROC) wglGetProcAddress ("glUniform1iARB");
	gameStates.ogl.bShadersOk =
		glCreateProgramObject && glDeleteObject && glUseProgramObject &&
		glCreateShaderObject && glCreateShaderObject && glCompileShader && 
		glGetObjectParameteriv && glAttachObject && glGetInfoLog && 
		glLinkProgram && glGetUniformLocation && 
		glUniform4f && glUniform3f &&glUniform1f && glUniform4fv && glUniform3fv &&glUniform1fv && glUniform1i;
#else
	gameStates.ogl.bShadersOk = 1;
#endif
	}
LogErr (gameStates.ogl.bShadersOk ? "Shaders are available\n" : "No shaders available\n");
gameStates.ogl.bAntiAliasingOk = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_multisample"));
#if RENDER2TEXTURE == 1
OglInitPBuffer ();
#elif RENDER2TEXTURE == 2
OglInitFBuffer ();
#endif
InitShaders ();
glEnable (GL_STENCIL_TEST);
if (gameStates.render.bHaveStencilBuffer = glIsEnabled (GL_STENCIL_TEST))
	glDisable (GL_STENCIL_TEST);
}

//------------------------------------------------------------------------------

