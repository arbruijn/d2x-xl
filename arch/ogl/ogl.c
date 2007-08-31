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
#include "hostage.h"
#include "gr.h"

//------------------------------------------------------------------------------

#if TEXTURE_COMPRESSION
#	ifndef GL_VERSION_20
#		ifdef _WIN32
PFNGLGETCOMPRESSEDTEXIMAGEPROC	glGetCompressedTexImage = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC		glCompressedTexImage2D = NULL;
#		endif
#	endif
#endif

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

#ifdef _WIN32
PFNGLACTIVESTENCILFACEEXTPROC		glActiveStencilFaceEXT = NULL;
#endif

//------------------------------------------------------------------------------

tTexPolyMultiDrawer	*fpDrawTexPolyMulti = NULL;

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
	
extern GLuint hBigSphere;
extern GLuint hSmallSphere;
extern GLuint circleh5;
extern GLuint circleh10;
extern GLuint cross_lh [2];
extern GLuint primary_lh [3];
extern GLuint secondary_lh [5];
extern GLuint glInitTMU [3]; 
extern GLuint glExitTMU;
extern GLuint mouseIndList;

tOglTexture oglTextureList [OGL_TEXTURE_LIST_SIZE];
int oglTexListCur;

//------------------------------------------------------------------------------

inline void OglInitTextureStats (tOglTexture* t)
{
t->prio = (float) 0.3;//default prio
t->lastrend = 0;
t->numrend = 0;
}

//------------------------------------------------------------------------------

void OglInitTexture (tOglTexture *t, int bMask)
{
t->handle = 0;
t->internalformat = bMask ? 1 : gameStates.ogl.bpp / 8;
t->format = bMask ? GL_ALPHA : gameStates.ogl.nRGBAFormat;
t->wrapstate = -1;
t->w =
t->h = 0;
t->bFrameBuf = 0;
OglInitTextureStats (t);
}

//------------------------------------------------------------------------------

void OglResetTextureStatsInternal (void)
{
	int			i;
	tOglTexture	*t = oglTextureList;

for (i = OGL_TEXTURE_LIST_SIZE; i; i--, t++)
	if (t->handle)
		OglInitTextureStats (t);
}

//------------------------------------------------------------------------------

void OglInitTextureListInternal (void)
{
	int			i;
	tOglTexture	*t = oglTextureList;
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
if (bmP->glTexture && (bmP->glTexture->handle == (unsigned int) -1)) {
	bmP->glTexture->handle = 0;
	bmP->glTexture = NULL;
	}
}

//------------------------------------------------------------------------------

void OglSmashTextureListInternal (void)
{
	int			h, i, j, k;
	tOglTexture *t;
	grsBitmap	*bmP, *altBmP;

OglDeleteLists (&hBigSphere, 1);
OglDeleteLists (&hSmallSphere, 1);
OglDeleteLists (&circleh5, 1);
OglDeleteLists (&circleh10, 1);
OglDeleteLists (cross_lh, sizeof (cross_lh) / sizeof (*cross_lh));
OglDeleteLists (primary_lh, sizeof (primary_lh) / sizeof (*primary_lh));
OglDeleteLists (secondary_lh, sizeof (secondary_lh) / sizeof (*secondary_lh));
OglDeleteLists (glInitTMU, sizeof (glInitTMU) / sizeof (*glInitTMU));
OglDeleteLists (&glExitTMU, 1);
OglDeleteLists (&mouseIndList, 1);
hBigSphere =
hSmallSphere =
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
	if (!t->bFrameBuf && (t->handle > 0)) {
		glDeleteTextures (1, (GLuint *) &t->handle);
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

tOglTexture *OglGetFreeTextureInternal (void)
{
	int i;
	tOglTexture *t = oglTextureList + oglTexListCur;

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

tOglTexture *OglGetFreeTexture (void)
{
tOglTexture *t = OglGetFreeTextureInternal ();
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
	tOglTexture* t;
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
	tOglTexture* t;
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
		if (!t->bFrameBuf && (t->handle > 0)) {
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

int ogl_bindteximage (tOglTexture *texP)
{
#if RENDER2TEXTURE == 1
#	if 1
OGL_BINDTEX (texP->pbuffer.texId);
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
OGL_BINDTEX (texP->pbuffer.texId);
#	endif
#	ifdef _WIN32
#		ifdef _DEBUG
if (!texP->pbuffer.bBound) {
	texP->pbuffer.bBound = wglBindTexImageARB (texP->pbuffer.hBuf, WGL_FRONT_LEFT_ARB);
	if (!texP->pbuffer.bBound) {
		char *psz = (char *) gluErrorString (glGetError ());
		return 1;
		}
	}
#		else
if (!(texP->pbuffer.bBound ||
	 (texP->pbuffer.bBound = wglBindTexImageARB (texP->pbuffer.hBuf, WGL_FRONT_LEFT_ARB))))
	return 1;
#		endif
#	endif
#elif RENDER2TEXTURE == 2
OGL_BINDTEX (texP->fbuffer.texId);
#endif
return 0;
}

//------------------------------------------------------------------------------

int OglBindBmTex (grsBitmap *bmP, int bMipMaps, int nTransp)
{
	tOglTexture	*texP;
#if RENDER2TEXTURE
	int			bPBuffer;
#endif

if (!bmP)
	return 1;
bmP = BmOverride (bmP);
texP = bmP->glTexture;
#if RENDER2TEXTURE
if ((bPBuffer = texP && texP->bFrameBuf)) {
	if (ogl_bindteximage (texP))
		return 1;
	}
else
#endif
	{
	if (!(texP && (texP->handle > 0))) {
		if (OglLoadBmTexture (bmP, bMipMaps, nTransp))
			return 1;
		bmP = BmOverride (bmP);
		texP = bmP->glTexture;
#if 1//def _DEBUG
		if (!texP)
			OglLoadBmTexture (bmP, 1, nTransp);
#endif
		}
	OGL_BINDTEX (texP->handle);
	}
#ifdef _DEBUG
bmP->glTexture->lastrend = gameData.time.xGame;
bmP->glTexture->numrend++;
#endif
return 0;
}

//------------------------------------------------------------------------------

tRgbColorf *BitmapColor (grsBitmap *bmP, ubyte *bufP)
{
	int c, h, i, j = 0, r = 0, g = 0, b = 0;
	tRgbColorf *color;
	ubyte	*palette;
		
if (!bufP)
	return NULL;
h = (int) (bmP - gameData.pig.tex.pBitmaps);
#ifdef _DEBUG
if (h == 874)
	h = h;
#endif
if ((h < 0) || (h >= MAX_BITMAP_FILES)) {
	h = (int) (bmP - gameData.pig.tex.bitmaps [0]);
	if ((h < 0) || (h >= MAX_BITMAP_FILES))
		return NULL;
	}
color = gameData.pig.tex.bitmapColors + h;
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
	bmP->bm_avgRGB.red = 4 *(ubyte) (r / j);
	bmP->bm_avgRGB.green = 4 *(ubyte) (g / j);
	bmP->bm_avgRGB.blue = 4 *(ubyte) (b / j);
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
	tPolyModel *po;
if ((po = GetPolyModel (NULL, NULL, nModel, 0)))
	LoadModelTextures (po, NULL);
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
	OglCacheVClipTexturesN (w->nVClipIndex, 3);
else if (w->renderType == WEAPON_RENDER_POLYMODEL)
	OglCachePolyModelTextures (w->nModel);
}

//------------------------------------------------------------------------------

void OglResetFrames (grsBitmap *bmP)
{
OglFreeBmTexture (bmP);
}

//------------------------------------------------------------------------------

grsBitmap *LoadFaceBitmap (short nTexture, short nFrameIdx)
{
	grsBitmap	*bmP;
	int			nFrames;

PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [nTexture], gameStates.app.bD1Mission);
bmP = gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [nTexture].index;
if (BM_OVERRIDE (bmP)) {
	bmP = BM_OVERRIDE (bmP);
	if (bmP->bm_wallAnim) {
		nFrames = BM_FRAMECOUNT (bmP);
		if (nFrames > 1) {
			OglLoadBmTexture (bmP, 1, 3);
			if (BM_FRAMES (bmP)) {
				if ((nFrameIdx >= 0) || (-nFrames > nFrameIdx))
					BM_CURFRAME (bmP) = BM_FRAMES (bmP);
				else
					BM_CURFRAME (bmP) = BM_FRAMES (bmP) - nFrameIdx - 1;
				OglLoadBmTexture (BM_CURFRAME (bmP), 1, 3);
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
	if ((tMap2 = sideP->nOvlTex)) {
		bm2 = LoadFaceBitmap (tMap1, sideP->nFrame);
		if (!(bm2->bm_props.flags & BM_FLAG_SUPER_TRANSPARENT) ||
			 (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk))
			OglLoadBmTexture (bm2, 1, 3);
		else if ((bmm = TexMergeGetCachedBitmap (tMap1, tMap2, sideP->nOvlOrient)))
			bmP = bmm;
		else
			OglLoadBmTexture (bm2, 1, 3);
		}
	OglLoadBmTexture (bmP, 1, 3);
	}
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
	int			i, j, bD1;
	tEffectClip	*ec;
	int			max_efx = 0, ef;
	int			nSegment, nSide;
	short			nBaseTex, nOvlTex;
	grsBitmap	*bmBot,*bmTop, *bmm;
	tSegment		*segP;
	tSide			*sideP;
	tObject		*objP;

if (gameStates.render.bBriefing)
	return 0;
OglResetTextureStatsInternal ();//loading a new lev should reset textures
TexMergeClose ();
TexMergeInit (-1);
for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	for (i = 0,ec = gameData.eff.effects [bD1]; i < gameData.eff.nEffects [bD1];i++,ec++) {
		if ((ec->changingWallTexture == -1) && (ec->changingObjectTexture == -1))
			continue;
		if (ec->vc.nFrameCount > max_efx)
			max_efx = ec->vc.nFrameCount;
		}
	for (ef = 0; ef < max_efx; ef++)
		for (i = 0,ec = gameData.eff.effects [bD1]; i < gameData.eff.nEffects [bD1]; i++, ec++) {
			if ((ec->changingWallTexture == -1) && (ec->changingObjectTexture == -1))
				continue;
			ec->time_left = -1;
			}
	}

for (segP = SEGMENTS, nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, segP++) {
	for (nSide = 0, sideP = segP->sides; nSide < MAX_SIDES_PER_SEGMENT; nSide++, sideP++) {
		nBaseTex = sideP->nBaseTex;
		if ((nBaseTex < 0) || (nBaseTex >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
			continue;
		bmBot = LoadFaceBitmap (nBaseTex, sideP->nFrame);
		if ((nOvlTex = sideP->nOvlTex)) {
			bmTop = LoadFaceBitmap (nOvlTex, sideP->nFrame);
			if (!(bmTop->bm_props.flags & BM_FLAG_SUPER_TRANSPARENT) ||
				 (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk))
				OglLoadBmTexture (bmTop, 1, 3);
			else if ((bmm = TexMergeGetCachedBitmap (nBaseTex, nOvlTex, sideP->nOvlOrient)))
				bmBot = bmm;
			else
				OglLoadBmTexture (bmTop, 1, 3);
			}
		OglLoadBmTexture (bmBot, 1, 3);
		}
	}
ResetSpecialEffects ();
InitSpecialEffects ();
DoSpecialEffects ();
CacheObjectEffects ();
// cache all weapon and powerup textures
for (i = 0; i < EXTRA_OBJ_IDS; i++)
	OglCacheWeaponTextures (gameData.weapons.info + i);
for (i = 0; i < MAX_POWERUP_TYPES; i++)
	if (i != 9)
		OglCacheVClipTexturesN (gameData.objs.pwrUp.info [i].nClipIndex, 3);
for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject; i++, objP++)
	if (objP->renderType == RT_POLYOBJ)
		OglCachePolyModelTextures (objP->rType.polyObjInfo.nModel);
// cache the hostage clip frame textures
OglCacheVClipTexturesN (33, 2);    
// cache all clip frame textures incl. explosions and effects
for (i = 0; i < 2; i++)
	for (j = 0; j < MAX_GAUGE_BMS; j++)
		if (gameData.cockpit.gauges [i][j].index != 0xffff)
			PIGGY_PAGE_IN (gameData.cockpit.gauges [i][j], 0);
for (i = 0; i < gameData.eff.nClips [0]; i++)
	OglCacheVClipTexturesN (i, 1);
return 0;
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

int OglFillTexBuf (
	grsBitmap	*bmP,
	GLubyte		*texBuf,
	int			truewidth,
	int			width,
	int			height,
	int			dxo,
	int			dyo,
	int			tWidth,
	int			tHeight,
	int			nFormat,
	int			nTransp,
	int			superTransp)
{
//	GLushort *texP= (GLushort *)texBuf;
	ubyte		*data = bmP->bm_texBuf;
	int		x, y, c, i, j;
	ushort	r, g, b, a;
	int		bShaderMerge = gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk;

gameData.render.ogl.palette = (BM_PARENT (bmP) ? BM_PARENT (bmP)->bm_palette : bmP->bm_palette);
if (!gameData.render.ogl.palette)
	gameData.render.ogl.palette = defaultPalette;
if (!gameData.render.ogl.palette)
	return nFormat;
if (tWidth * tHeight * 4 > sizeof (gameData.render.ogl.texBuf))//shouldn't happen, descent never uses textures that big.
	Error ("texture too big %i %i",tWidth, tHeight);

if ((tWidth <= width) && (tHeight <= height) && !GrBitmapHasTransparency (bmP))
	nFormat = GL_RGB;
#ifdef _DEBUG
if (!nTransp)
	nTransp = 0;
#endif
restart:

i = 0;
for (y = 0; y < tHeight; y++) {
	i = dxo + truewidth *(y + dyo);
	for (x = 0; x < tWidth; x++){
		if ((x < width) && (y < height))
			c = data [i++];
		else
			c = TRANSPARENCY_COLOR;	//fill the pad space with transparancy
		if ((int) c == TRANSPARENCY_COLOR) {
			if (nTransp)
				bmP->bm_props.flags |= BM_FLAG_TRANSPARENT;
			switch (nFormat) {
				case GL_LUMINANCE:
					(*(texBuf++)) = 0;
					break;

				case GL_LUMINANCE_ALPHA:
					*((GLushort *) texBuf) = 0;
					texBuf += 2; 
					break;

				case GL_RGB:
				case GL_RGB5:
					nFormat = gameStates.ogl.nRGBAFormat;
					goto restart;
					break;

				case GL_RGBA:
					*((GLuint *) texBuf) = nTransp ? 0 : 0xffffffff;
					texBuf += 4;
					break;
					
				case GL_RGBA4:
					*((GLushort *) texBuf) = nTransp ? 0 : 0xffff;
					texBuf += 2;
					break;
				}
//				 (*(texP++)) = 0;
			}
		else {
			switch (nFormat) {
				case GL_LUMINANCE://these could prolly be done to make the intensity based upon the intensity of the resulting color, but its not needed for anything (yet?) so no point. :)
						(*(texBuf++)) = 255;
					break;

				case GL_LUMINANCE_ALPHA:
						(*(texBuf++)) = 255;
						(*(texBuf++)) = 255;
					break;

				case GL_RGB:
				case GL_RGB5:
					if (superTransp && (c == SUPER_TRANSP_COLOR)) {
						nFormat = gameStates.ogl.nRGBAFormat;
						goto restart;
						}
					else {
						j = c * 3;
						r = gameData.render.ogl.palette [j] * 4;
						g = gameData.render.ogl.palette [j + 1] * 4;
						b = gameData.render.ogl.palette [j + 2] * 4;
						if (nFormat == GL_RGB) {
							(*(texBuf++)) = (GLubyte) r;
							(*(texBuf++)) = (GLubyte) g;
							(*(texBuf++)) = (GLubyte) b;
							}
						else {
							r >>= 3;
							g >>= 2;
							b >>= 3;
							*((GLushort *) texBuf) = r + (g << 5) + (b << 11);
							texBuf += 2;
							}
						}
					break;

				case GL_RGBA:
				case GL_RGBA4: {
					if (superTransp && (c == SUPER_TRANSP_COLOR)) {
						bmP->bm_props.flags |= BM_FLAG_SUPER_TRANSPARENT;
#if 0
						if (bShaderMerge) {
							r = g = b = 0;
							a = 1;
							}
						else
#endif
							{
							r = 120;
							g = 88;
							b = 128;
							a = 0;
							}
						}
					else {
						j = c * 3;
						r = gameData.render.ogl.palette [j] * 4;
						g = gameData.render.ogl.palette [j + 1] * 4;
						b = gameData.render.ogl.palette [j + 2] * 4;
						if (nTransp == 1) {
#if 0 //non-linear formula
							double da = (double) (r * 3 + g * 5 + b * 2) / (10.0 * 255.0);
							da *= da;
							a = (ubyte) (da * 255.0);
#else
							a = (r * 3 + g * 5 + b * 2) / 10;	//transparency based on color intensity
#endif
							}
						else if (nTransp == 2)	// black is transparent
							a = c ? 255 : 0;
						else
							a = 255;	//not transparent
						}
					if (nFormat == GL_RGBA) {
						(*(texBuf++)) = (GLubyte) r;
						(*(texBuf++)) = (GLubyte) g;
						(*(texBuf++)) = (GLubyte) b;
						(*(texBuf++)) = (GLubyte) a;
						}
					else {
						*((GLushort *) texBuf) = (r >> 4) + ((g >> 4) << 4) + ((b >> 4) << 8) + ((a >> 4) << 12);
						texBuf += 2;
						}
					break;
					}
				}
			}
		}
	}
return nFormat;
}

//------------------------------------------------------------------------------
//create texture buffer from data already in RGBA format

ubyte *OglCopyTexBuf (tOglTexture *texP, int dxo, int dyo, ubyte *data)
{
if (!dxo && !dyo && (texP->w == texP->tw) && (texP->h == texP->th))
	return data;	//can use data 1:1
else {	//need to reformat
	int		h, w, tw;
	GLubyte	*bufP;
	
	h = texP->lw / texP->w;
	w = (texP->w - dxo) * h;
	data += texP->lw * dyo + h * dxo;
	bufP = gameData.render.ogl.texBuf;
	tw = texP->tw * h;
	h = tw - w;
	for (; dyo < texP->h; dyo++, data += texP->lw) {
		memcpy (bufP, data, w);
		bufP += w;
		memset (bufP, 0, h);
		bufP += h;
		}
	memset (bufP, 0, texP->th * tw - (bufP - gameData.render.ogl.texBuf));
	return gameData.render.ogl.texBuf;
	}
}

//------------------------------------------------------------------------------

int TexFormatSupported (tOglTexture *texP)
{
	GLint nFormat = 0;

if (!gameStates.ogl.bGetTexLevelParam)
	return 1;

switch (texP->format) {
	case GL_RGB:
		if (texP->internalformat == 3)
			return 1;
		break;

	case GL_RGBA:
		if (texP->internalformat == 4)
			return 1;
		break;

	case GL_RGB5:
	case GL_RGBA4:
		if (texP->internalformat == 2)
			return 1;
		break;
		
	case GL_INTENSITY4:
		if (gameStates.ogl.bIntensity4 == -1) 
			return 1; 
		if (!gameStates.ogl.bIntensity4) 
			return 0; 
		break;

	case GL_LUMINANCE4_ALPHA4:
		if (gameStates.ogl.bLuminance4Alpha4 == -1) 
			return 1; 
		if (!gameStates.ogl.bLuminance4Alpha4) 
			return 0; 
		break;
	}

glTexImage2D (GL_PROXY_TEXTURE_2D, 0, texP->internalformat, texP->tw, texP->th, 0,
				  texP->format, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);//NULL?
glGetTexLevelParameteriv (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &nFormat);
switch (texP->format) {
	case GL_RGBA:
#if TEXTURE_COMPRESSION
		if ((nFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ||
			 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) ||
			 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT))
			nFormat = texP->internalformat;
		else
#endif
			texP->internalformat = 4;
		break;

	case GL_RGB:
			texP->internalformat = 3;
		break;

	case GL_RGB5:
	case GL_RGBA4:
		texP->internalformat = 2;
		break;
		
	case GL_INTENSITY4:
		gameStates.ogl.bIntensity4 = (nFormat == texP->internalformat) ? -1 : 0;
		break;

	case GL_LUMINANCE4_ALPHA4:
		gameStates.ogl.bLuminance4Alpha4 = (nFormat == texP->internalformat) ? -1 : 0;
		break;

	default:
		break;
	}
return nFormat == texP->internalformat;
}

//------------------------------------------------------------------------------

int TexFormatVerify (tOglTexture *texP)
{
while (!TexFormatSupported (texP)) {
	switch (texP->format) {
		case GL_INTENSITY4:
			if (gameStates.ogl.bLuminance4Alpha4) {
				texP->internalformat = 2;
				texP->format = GL_LUMINANCE_ALPHA;
				break;
				}

		case GL_LUMINANCE4_ALPHA4:
			texP->internalformat = 4;
			texP->format = GL_RGBA;
			break;

		case GL_RGB:
			texP->internalformat = 4;
			texP->format = GL_RGBA;
			break;

		case GL_RGB5:
			texP->internalformat = 3;
			texP->format = GL_RGB;
			break;
			
		case GL_RGBA4:
			texP->internalformat = 4;
			texP->format = GL_RGBA;
			break;

		default:
#if TRACE	
			con_printf (CONDBG,"...no texP format to fall back on\n");
#endif
			return 1;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void TexSetSizeSub (tOglTexture *texP,int dbits,int bits,int w, int h)
{
	int u;

if (texP->tw != w || texP->th != h)
	u = (int) ((texP->w / (double) texP->tw * w) *(texP->h / (double) texP->th * h));
else
	u = (int) (texP->w * texP->h);
if (bits <= 0) //the beta nvidia GLX server. doesn't ever return any bit sizes, so just use some assumptions.
	bits = dbits;
texP->bytes = (int) (((double) w * h * bits) / 8.0);
texP->bytesu = (int) (((double) u * bits) / 8.0);
}

//------------------------------------------------------------------------------

void TexSetSize (tOglTexture *texP)
{
	GLint	w, h;
	int	nBits = 16, a = 0;

if (gameStates.ogl.bGetTexLevelParam) {
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
	w = texP->tw;
	h = texP->th;
	}
switch (texP->format) {
	case GL_LUMINANCE:
		nBits = 8;
		break;

	case GL_LUMINANCE_ALPHA:
		nBits = 8;
		break;

	case GL_RGB:
		nBits = 24;

	case GL_RGBA:
#if TEXTURE_COMPRESSION
	case GL_COMPRESSED_RGBA:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
#endif
		nBits = 32;
		break;

	case GL_RGB5:
	case GL_RGBA4:
		nBits = 16;
		break;

	case GL_ALPHA:
		nBits = 8;
		break;

	default:
		Error ("TexSetSize unknown texformat\n");
		break;
	}
TexSetSizeSub (texP, nBits, a, w, h);
}

//------------------------------------------------------------------------------

#if TEXTURE_COMPRESSION

int OglCompressTexture (grsBitmap *bmP, tOglTexture *texP)
{
	GLint nFormat, nParam;
	ubyte	*data;

if (texP->internalformat != GL_COMPRESSED_RGBA)
	return 0;
glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &nParam);
if (nParam) {
	glGetTexLevelParameteriv (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &nFormat);
	if ((nFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ||
		 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) ||
		 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)) {
		glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &nParam);
		if (nParam && (data = (ubyte *) D2_ALLOC (nParam))) {
			D2_FREE (bmP->bm_texBuf);
			glGetCompressedTexImage (GL_TEXTURE_2D, 0, (GLvoid *) data);
			bmP->bm_texBuf = data;
			bmP->bm_bufSize = nParam;
			bmP->bm_format = nFormat;
			bmP->bm_compressed = 1;
			}
		}
	}
if (bmP->bm_compressed)
	return 1;
if (bmP->bm_bpp == 3) {
	texP->format = GL_RGB;
	texP->internalformat = 3;
	}
else {
	texP->format = GL_RGB;
	texP->internalformat = 4;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------
//loads a palettized bitmap into a ogl RGBA texture.
//Sizes and pads dimensions to multiples of 2 if necessary.
//In theory this could be a problem for repeating textures, but all real
//textures (not sprites, etc) in descent are 64x64, so we are ok.
//stores OpenGL textured id in *texid and u/v values required to get only the real data in *u/*v
int OglLoadTexture (grsBitmap *bmP, int dxo, int dyo, tOglTexture *texP, int nTransp, int superTransp)
{
	ubyte			*data = bmP->bm_texBuf;
	GLubyte		*bufP = gameData.render.ogl.texBuf;
	tOglTexture	tex;
	int			bLocalTexture;

if (!bmP)
	return 1;
#ifdef _DEBUG
if (strstr (bmP->szName, "plasblob"))
	bmP = bmP;
#endif
if (texP) {
	bLocalTexture = 0;
	//calculate smallest texture size that can accomodate us (must be multiples of 2)
#if TEXTURE_COMPRESSION
	if (bmP->bm_compressed) {
		texP->w = bmP->bm_props.w;
		texP->h = bmP->bm_props.h;
		}
#endif
	texP->tw = pow2ize (texP->w);
	texP->th = pow2ize (texP->h);
	}
else {
	bLocalTexture = 1;
	texP = &tex;
	tex.tw = pow2ize (tex.w = bmP->bm_props.w);
	tex.th = pow2ize (tex.h = bmP->bm_props.h);
	if (bmP->bm_bpp == 3) {
		texP->format = GL_RGB;
		texP->internalformat = 3;
		}
	else {
		texP->format = GL_RGB;
		texP->internalformat = 4;
		}
	tex.handle = 0;
	}
if (bmP && (bmP->bm_bpp == 3)) {
	texP->format = GL_RGB;
	texP->internalformat = 3;
	texP->lw = bmP->bm_bpp * texP->w;
	}

if (gr_badtexture > 0) 
	return 1;

#if TEXTURE_COMPRESSION
#	ifndef __macosx__
if (!(bmP->bm_compressed || superTransp || BM_PARENT (bmP))) {
	if (gameStates.ogl.bTextureCompression && gameStates.ogl.bHaveTexCompression &&
		 ((texP->format == GL_RGBA) || (texP->format == GL_RGB)) && 
		 (texP->tw >= 64) && (texP->th >= texP->tw))
		texP->internalformat = GL_COMPRESSED_RGBA;
	if (TexFormatVerify (texP))
		return 1;
	}
#	endif
#endif

//calculate u/v values that would make the resulting texture correctly sized
texP->u = (float) ((double) texP->w / (double) texP->tw);
texP->v = (float) ((double) texP->h / (double) texP->th);
//	if (width!=twidth || height!=theight)
#if RENDER2TEXTURE
if (!texP->bFrameBuf) 
#endif
	{
#if TEXTURE_COMPRESSION
	if (data && !bmP->bm_compressed)
#else
	if (data) {
#endif
		if (nTransp >= 0) {
			texP->format = 
				OglFillTexBuf (bmP, gameData.render.ogl.texBuf, texP->lw, texP->w, texP->h, dxo, dyo, texP->tw, texP->th, 
									texP->format, nTransp, superTransp);
			if (texP->format == GL_RGB)
				texP->internalformat = 3;
			else if (texP->format == GL_RGBA)
				texP->internalformat = 4;
			else if ((texP->format == GL_RGB5) || (texP->format == GL_RGBA4))
				texP->internalformat = 2;
			}
		else
			bufP = OglCopyTexBuf (texP, dxo, dyo, data);
		}
	// Generate OpenGL texture IDs.
	glGenTextures (1, (GLuint *) &texP->handle);
	//set priority
	glPrioritizeTextures (1, (GLuint *) &texP->handle, &texP->prio);
	// Give our data to OpenGL.
	OGL_BINDTEX (texP->handle);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (texP->bMipMaps) {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gameStates.ogl.texMagFilter);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gameStates.ogl.texMinFilter);
		}
	else {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
//	mipmaps aren't used in GL_NEAREST anyway, and making the mipmaps is pretty slow
// however, if texturing mode becomes an ingame option, they would need to be made regardless, so it could switch to them later.  
// OTOH, texturing mode could just be made a command line arg.
#if TEXTURE_COMPRESSION
	if (bmP->bm_compressed) {
		glCompressedTexImage2D (
			GL_TEXTURE_2D, 0, bmP->bm_format,
			texP->tw, texP->th, 0, bmP->bm_bufSize, bmP->bm_texBuf);
		}
	else 
#endif
		{
		if (texP->bMipMaps && gameStates.ogl.bNeedMipMaps)
			gluBuild2DMipmaps (
				GL_TEXTURE_2D, texP->internalformat, 
				texP->tw, texP->th, texP->format, 
				GL_UNSIGNED_BYTE, 
				bufP);
		else
			glTexImage2D (
				GL_TEXTURE_2D, 0, texP->internalformat,
				texP->tw, texP->th, 0, texP->format, // RGBA textures.
				GL_UNSIGNED_BYTE, // imageData is a GLubyte pointer.
				bufP);
#if TEXTURE_COMPRESSION
		OglCompressTexture (bmP, texP);
#endif
		TexSetSize (texP);
		}
	if (bLocalTexture)
		glDeleteTextures (1, (GLuint *) &texP->handle);
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
	tOglTexture		*t;
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
		t->lw *= bmP->bm_bpp;
	t->w = bmP->bm_props.w;
	t->h = bmP->bm_props.h;
	t->bMipMaps = bMipMap && !bMask;
#if RENDER2TEXTURE == 1
	if (pb) {
		t->pbuffer = *pb;
		t->handle = pb->texId;
		t->bFrameBuf = 1;
		}
#elif RENDER2TEXTURE == 2
	if (fb) {
		t->fbuffer = *fb;
		t->handle = fb->texId;
		t->bFrameBuf = 1;
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
	bmfP = (grsBitmap *) D2_ALLOC (nFrames * sizeof (struct _grsBitmap));
	memset (bmfP, 0, nFrames * sizeof (struct _grsBitmap));
	BM_FRAMES (bmP) = bmfP;
	for (i = 0; i < nFrames; i++, bmfP++) {
		BM_CURFRAME (bmP) = bmfP;
		GrInitSubBitmap (bmfP, bmP, 0, i * w, w, w);
		bmfP->bmType = BM_TYPE_FRAME;
		bmfP->bm_props.y = 0;
		BM_PARENT (bmfP) = bmP;
		if (bmP->bm_transparentFrames [i / 32] & (1 << (i % 32)))
			bmfP->bm_props.flags |= BM_FLAG_TRANSPARENT;
		if (bmP->bm_supertranspFrames [i / 32] & (1 << (i % 32)))
			bmfP->bm_props.flags |= BM_FLAG_SUPER_TRANSPARENT;
		OglLoadBmTextureM (bmfP, bDoMipMap, nTransp, 0, NULL);
		}
	BM_CURFRAME (bmP) = BM_FRAMES (bmP);
	if (CreateSuperTranspMasks (bmP))
		for (i = nFrames, bmfP = BM_FRAMES (bmP); i; i--, bmfP++)
			if (BM_MASK (bmfP))
				OglLoadBmTextureM (BM_MASK (bmfP), 0, -1, 1, NULL);
	}
return 0;
}

//------------------------------------------------------------------------------

void OglFreeTexture (tOglTexture *t)
{
if (t) {
	GLuint h = (GLuint) t->handle;
	if ((GLint) h > 0) {
		r_texcount--;
		glDeleteTextures (1, &h);
		OglInitTexture (t, 0);
		}
	}
}

//------------------------------------------------------------------------------

void OglFreeBmTexture (grsBitmap *bmP)
{
	tOglTexture	*t;

while ((bmP->bmType != BM_TYPE_ALT) && BM_PARENT (bmP) && (bmP != BM_PARENT (bmP)))
	bmP = BM_PARENT (bmP);
if (BM_FRAMES (bmP)) {
	int i, nFrames = bmP->bm_props.h / bmP->bm_props.w;

	for (i = 0; i < nFrames; i++) {
		OglFreeTexture (BM_FRAMES (bmP) [i].glTexture);
		BM_FRAMES (bmP) [i].glTexture = NULL;
		}
	}
else if ((t = bmP->glTexture)) {
#if RENDER2TEXTURE == 2
	if (t->bFrameBuf)
		OGL_BINDTEX (0);
	else
#elif RENDER2TEXTURE == 1
#	ifdef _WIN32
	if (t->bFrameBuf) {
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
	unsigned int *data = (unsigned int*) D2_ALLOC (nSize); 

memset (data, 0, nSize); 	// Clear Storage Memory
glGenTextures (1, &texId); 					// Create 1 Texture
OGL_BINDTEX (texId); 			// Bind The Texture
glTexImage2D (GL_TEXTURE_2D, 0, 4, Xsize, Ysize, 0, gameStates.ogl.nRGBAFormat, GL_UNSIGNED_BYTE, data); 			// Build Texture Using Information In data
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
D2_FREE (data); 							// Release data
return texId; 						// Return The Texture ID
}

//------------------------------------------------------------------------------

char *LoadShader (char* fileName) //, char* Shadersource)
{
	FILE	*fp; 
	char	*bufP = NULL; 
	int 	fSize;
#ifdef _WIN32
	int	f;
#endif 
	char 	fn [FILENAME_LEN];

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

if (!(bufP = (char *) D2_ALLOC (sizeof (char) *(fSize + 1)))) {
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
	if ((nLogLen > 0) && (infoLog = (char *) D2_ALLOC (nLogLen))) {
		infoLog = (char *) D2_ALLOC (nLogLen);
		glGetProgramInfoLog (handle, nLogLen, &charsWritten, infoLog);
		if (*infoLog)
			LogErr ("\n%s\n\n", infoLog);
		D2_FREE (infoLog);
		}
	}
else {
	glGetShaderiv (handle, GL_INFO_LOG_LENGTH, &nLogLen);
	if ((nLogLen > 0) && (infoLog = (char *) D2_ALLOC (nLogLen))) {
		glGetShaderInfoLog (handle, nLogLen, &charsWritten, infoLog);
		if (*infoLog)
			LogErr ("\n%s\n\n", infoLog);
		D2_FREE (infoLog);
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
if ((nLogLen > 0) && (infoLog = (char *) D2_ALLOC (nLogLen))) {
	glGetInfoLog (handle, nLogLen, &charsWritten, infoLog);
	if (*infoLog)
		LogErr ("\n%s\n\n", infoLog);
	D2_FREE (infoLog);
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
if (!(vs = glCreateShaderObject (GL_VERTEX_SHADER)))
	return 0;
if (!(fs = glCreateShaderObject (GL_FRAGMENT_SHADER))) {
	glDeleteObject (vs);
	return 0;
	}
#if DBG_SHADERS	
if (bFromFile) {
	vsName = LoadShader (vsName); 
	fsName = LoadShader (fsName); 
	if (!vsName || !fsName) 
		return 0; 
	}
#endif
glShaderSource (vs, 1, (const GLcharARB **) &vsName, NULL); 
glShaderSource (fs, 1, (const GLcharARB **) &fsName, NULL); 
#if DBG_SHADERS	
if (bFromFile) {
	D2_FREE (vsName); 
	D2_FREE (fsName); 
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
	if (gameOpts->render.bDynLighting)
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
	GLint	nTMUs;
	
glGetIntegerv (GL_MAX_TEXTURE_UNITS, &nTMUs);
if (gameStates.ogl.bShadersOk)
	gameStates.ogl.bShadersOk = (nTMUs > 1);
if (gameStates.render.color.bLightMapsOk)
	gameStates.render.color.bLightMapsOk = (nTMUs > 2);
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

#ifndef _DEBUG
#	ifdef _WIN32
glActiveStencilFaceEXT	= (PFNGLACTIVESTENCILFACEEXTPROC) wglGetProcAddress ("glActiveStencilFaceEXT");
#	endif
#endif

#if TEXTURE_COMPRESSION
gameStates.ogl.bHaveTexCompression = 1;
#	ifndef GL_VERSION_20
#		ifdef _WIN32
	glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC) wglGetProcAddress ("glGetCompressedTexImage");
	glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) wglGetProcAddress ("glCompressedTexImage2D");
	gameStates.ogl.bHaveTexCompression = glGetCompressedTexImage && glCompressedTexImage2D;
#		endif
#	endif
#else
gameStates.ogl.bHaveTexCompression = 0;
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
#ifdef _WIN32
	glMultiTexCoord2d &&
#endif 
	glActiveTexture && glClientActiveTexture;
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
if ((gameStates.render.bHaveStencilBuffer = glIsEnabled (GL_STENCIL_TEST)))
	glDisable (GL_STENCIL_TEST);
}

//------------------------------------------------------------------------------

