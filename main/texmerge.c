/* $Id: texmerge.c,v 1.3 2002/09/04 22:47:25 btb Exp $ */
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

#include <stdio.h>

#include "inferno.h"
#include "gr.h"
#include "error.h"
#include "game.h"
#include "textures.h"
#include "mono.h"
#include "rle.h"
#include "piggy.h"
#include "ogl_init.h"
#include "texmerge.h"

#include "ogl_init.h"
#define MAX_NUM_CACHE_BITMAPS 200

//static grsBitmap * cache_bitmaps [MAX_NUM_CACHE_BITMAPS];                     

typedef struct	{
	grsBitmap * bitmap;
	grsBitmap * bmBot;
	grsBitmap * bmTop;
	int 		nOrient;
	int		last_frame_used;
} TEXTURE_CACHE;

static TEXTURE_CACHE texCache [MAX_NUM_CACHE_BITMAPS];

static int nCacheEntries = 0;

static int nCacheHits = 0;
static int nCacheMisses = 0;

void MergeTextures (int nType, grsBitmap *bmBot, grsBitmap *bmTop, grsBitmap *dest_bmp, int bSuperTransp);
void MergeTexturesNormal (int nType, grsBitmap *bmBot, grsBitmap *bmTop, ubyte *dest_data);

//----------------------------------------------------------------------

int TexMergeInit (int nCacheSize)
{
	int i;
	TEXTURE_CACHE *cacheP = texCache;

nCacheEntries = (nCacheSize <= MAX_NUM_CACHE_BITMAPS) ? nCacheSize  : MAX_NUM_CACHE_BITMAPS;
for (i = 0; i < nCacheEntries; i++, cacheP++) {
	cacheP->last_frame_used = -1;
	cacheP->bmTop =
	cacheP->bmBot =
	cacheP->bitmap = NULL;
	cacheP->nOrient = -1;
	}
atexit(TexMergeClose);
return 1;
}

//----------------------------------------------------------------------

void TexMergeFlush()
{
	int i;

for (i = 0; i < nCacheEntries; i++) {
	texCache [i].last_frame_used = -1;
	texCache [i].bmTop = NULL;
	texCache [i].bmBot = NULL;
	texCache [i].nOrient = -1;
	}
}

//-------------------------------------------------------------------------

void _CDECL_ TexMergeClose(void)
{
	int i;

LogErr ("shutting down merged textures cache\n");
for (i = 0; i < nCacheEntries; i++) {
	if (texCache [i].bitmap) {
		GrFreeBitmap (texCache [i].bitmap);
		texCache [i].bitmap = NULL;
		}
	}
nCacheEntries = 0;
}

//-------------------------------------------------------------------------
//--unused-- int info_printed = 0;
grsBitmap * TexMergeGetCachedBitmap (int tMapBot, int tMapTop, int nOrient)
{
	grsBitmap		*bmTop, *bmBot, *bmP;
	int				i, nLowestFrame, nLRU;
	TEXTURE_CACHE	*cacheP;

nLRU = 0;
nLowestFrame = texCache [0].last_frame_used;
bmTop = BmOverride (gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [tMapTop].index);
bmBot = BmOverride (gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [tMapBot].index);

for (i = 0, cacheP = texCache; i < nCacheEntries; i++,cacheP++) {
#if 1//ndef _DEBUG
	if ((cacheP->last_frame_used > -1) && 
		 (cacheP->bmTop == bmTop) && 
		 (cacheP->bmBot == bmBot) && 
		 (cacheP->nOrient == nOrient) &&
		  cacheP->bitmap) {
		nCacheHits++;
		cacheP->last_frame_used = gameData.app.nFrameCount;
		return cacheP->bitmap;
	}	
#endif
	if (cacheP->last_frame_used < nLowestFrame)	{
		nLowestFrame = cacheP->last_frame_used;
		nLRU = i;
		}
	}
//---- Page out the LRU bitmap;
nCacheMisses++;
// Make sure the bitmaps are paged in...
#ifdef PIGGY_USE_PAGING
gameData.pig.tex.bPageFlushed = 0;
PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tMapTop], gameStates.app.bD1Mission);
PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tMapBot], gameStates.app.bD1Mission);
if (gameData.pig.tex.bPageFlushed)	{	// If cache got flushed, re-read 'em.
	gameData.pig.tex.bPageFlushed = 0;
	PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tMapTop], gameStates.app.bD1Mission);
	PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tMapBot], gameStates.app.bD1Mission);
	}
Assert (gameData.pig.tex.bPageFlushed == 0);
#endif

bmTop = BmOverride (gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [tMapTop].index);
bmBot = BmOverride (gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [tMapBot].index);
if (!bmTop->bm_palette)
	bmTop->bm_palette = gamePalette;
if (!bmBot->bm_palette)
	bmBot->bm_palette = gamePalette;
cacheP = texCache + nLRU;
bmP = cacheP->bitmap;
if (bmP)
	OglFreeBmTexture(bmP);

// if necessary, allocate cache bitmap
// in any case make sure the cache bitmap has the proper size
if (!bmP ||
	(bmP->bm_props.w != bmBot->bm_props.w) || 
	(bmP->bm_props.h != bmBot->bm_props.h)) {
	if (bmP)
		GrFreeBitmap (bmP);
	cacheP->bitmap =
	bmP = GrCreateBitmap (bmBot->bm_props.w, bmBot->bm_props.h, 1);
	if (!bmP)
		return NULL;
	}
else
	bmP->bm_props.flags = (char) BM_FLAG_TGA;
if (!bmP->bm_texBuf)
	return NULL;
bmP->bm_palette = gamePalette;
if (!(gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk)) {
	if (bmTop->bm_props.flags & BM_FLAG_SUPER_TRANSPARENT) {
//			return bmTop;
		MergeTextures (nOrient, bmBot, bmTop, bmP, 1);
		bmP->bm_props.flags |= BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT;
		bmP->bm_avgColor = bmTop->bm_avgColor;
		}
	else {
//			MergeTexturesNormal (nOrient, bmBot, bmTop, bmP->bm_texBuf);
		MergeTextures (nOrient, bmBot, bmTop, bmP, 0);
		bmP->bm_props.flags |= bmBot->bm_props.flags & (~BM_FLAG_RLE);
		bmP->bm_avgColor = bmBot->bm_avgColor;
		}
	}
cacheP->bmTop = bmTop;
cacheP->bmBot = bmBot;
cacheP->last_frame_used = gameData.app.nFrameCount;
cacheP->nOrient = nOrient;
return bmP;
}

//-------------------------------------------------------------------------

void MergeTexturesNormal (int nType, grsBitmap * bmBot, grsBitmap * bmTop, ubyte * dest_data)
{
	ubyte * top_data, *bottom_data;
	int scale;

if (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk)
	return;
if (bmTop->bm_props.flags & BM_FLAG_RLE)
	bmTop = rle_expand_texture(bmTop);
if (bmBot->bm_props.flags & BM_FLAG_RLE)
	bmBot = rle_expand_texture(bmBot);
//	Assert(bmBot != bmTop);
top_data = bmTop->bm_texBuf;
bottom_data = bmBot->bm_texBuf;
scale = bmBot->bm_props.w / bmTop->bm_props.w;
if (!scale)
	scale = 1;
if (scale > 1)
	scale = scale;
//	Assert(bottom_data != top_data);
switch(nType)	{
	case 0:
		// Normal
		GrMergeTextures(bottom_data, top_data, dest_data, bmBot->bm_props.w, bmBot->bm_props.h, scale);
		break;
	case 1:
		GrMergeTextures1(bottom_data, top_data, dest_data, bmBot->bm_props.w, bmBot->bm_props.h, scale);
		break;
	case 2:
		GrMergeTextures2(bottom_data, top_data, dest_data, bmBot->bm_props.w, bmBot->bm_props.h, scale);
		break;
	case 3:
		GrMergeTextures3(bottom_data, top_data, dest_data, bmBot->bm_props.w, bmBot->bm_props.h, scale);
		break;
	}
}

//-------------------------------------------------------------------------

typedef struct frac {
	int	c, d;
} frac;

inline tRGBA *C (ubyte *palP, ubyte *b, int i, int bTGA, int *pbST)
{
	static	tRGBA	c;
	int bST;

if (bTGA) {
	c = ((tRGBA *) (b)) [i];
	if (*pbST = ((c.r == 120) && (c.g == 88) && (c.b == 128)))
		c.a = 0;
	return &c;
	}
i = b [i];
bST = (i == SUPER_TRANSP_COLOR);
c.a = (bST || (i == TRANSPARENCY_COLOR)) ? 0 : 255;
if (c.a) {
	i *= 3;
	c.r = palP [i] * 4; 
	c.g = palP [i+1] * 4; 
	c.b = palP [i+2] * 4; 
	}
else if (bST) {
	c.r = 120;
	c.g = 88;
	c.b = 128;
	}
else
	c.r = c.g = c.b = 0;
*pbST = bST;
return &c;
}

//-------------------------------------------------------------------------

static inline int TexScale (int v, frac s)
{
return (v * s.c) / s.d;
}


static inline int TexIdx (int x, int y, int w, frac s)
{
return TexScale (y * w + x, s);
}


#define TOPSCALE(v)	TexScale (v, topScale)

#define TOPIDX	TexIdx (x, y, tw, topScale)
#define BTMIDX	TexIdx (x, y, bw, btmScale)


void MergeTextures (
	int nType, grsBitmap * bmBot, grsBitmap * bmTop, grsBitmap *dest_bmp, int bSuperTransp)
{
	tRGBA		*c;
	int		i, x, y, bw, bh, tw, th, dw, dh;
	int		bTopTGA, bBtmTGA, bST = 0;
	frac		topScale, btmScale;
	tRGBA		*dest_data = (tRGBA *) dest_bmp->bm_texBuf;

	ubyte * top_data, *bottom_data, *top_pal, *btmPalette;

bmBot = BmOverride (bmBot);
bmTop = BmOverride (bmTop);
if (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk)
	return;
if (bmTop->bm_props.flags & BM_FLAG_RLE)
	bmTop = rle_expand_texture (bmTop);

if (bmBot->bm_props.flags & BM_FLAG_RLE)
	bmBot = rle_expand_texture (bmBot);

//	Assert(bmBot != bmTop);

top_data = bmTop->bm_texBuf;
bottom_data = bmBot->bm_texBuf;
top_pal = bmTop->bm_palette;
btmPalette = bmBot->bm_palette;

//	Assert(bottom_data != top_data);

//Int3();
bh =
bw = bmBot->bm_props.w;
//h = bmBot->bm_props.h;
th =
tw = bmTop->bm_props.w;
dw =
dh = dest_bmp->bm_props.w;
//th = bmTop->bm_props.h;
#if 1
// square textures assumed here, so no test for h!
if (dw < tw) {
	topScale.c = tw / dw;
	topScale.d = 1;
	}
else  {
//		CBRK (dw > tw);
	topScale.c = 1;
	topScale.d = dw / tw;
	}
if (dw < bw) {
	btmScale.c = bw / dw;
	btmScale.d = 1;
	}
else {
	btmScale.c = dw / bw;
	btmScale.d = 1;
	}
#else
if (w > bmTop->bm_props.w)
	w = h = bmBot->bm_props.w;
scale.c = scale.d = 1;
#endif
bTopTGA = (bmTop->bm_props.flags & BM_FLAG_TGA) != 0;
bBtmTGA = (bmBot->bm_props.flags & BM_FLAG_TGA) != 0;
#ifdef _DEBUG
memset (dest_data, 253, dest_bmp->bm_props.w * dest_bmp->bm_props.h * 4);
#endif
switch(nType)	{
	case 0:
		// Normal
		for (i = y = 0; y < dh; y++)
			for (x = 0; x < dw; x++, i++) {
				c = C (top_pal, top_data, tw * TOPSCALE (y) + TOPSCALE (x), bTopTGA, &bST);
				if (!(bST || c->a))
					c = C (btmPalette, bottom_data, BTMIDX, bBtmTGA, &bST);
				dest_data [i] = *c;
			}
		break;
	case 1:
		// 
		for (i = y = 0; y < dh; y++)
			for (x = 0; x < dw; x++, i++) {
				c = C (top_pal, top_data, tw * TOPSCALE (x) + th - 1 - TOPSCALE (y), bTopTGA, &bST);
				if (!(bST || c->a))
					c = C (btmPalette, bottom_data, BTMIDX, bBtmTGA, &bST);
				dest_data [i] = *c;
			}
		break;
	case 2:
		// Normal
		for (i = y = 0; y < dh; y++)
			for (x = 0; x < dw; x++, i++) {
				c = C (top_pal, top_data, tw * (th - 1 - TOPSCALE (y)) + tw - 1 - TOPSCALE (x), bTopTGA, &bST);
				if (!(bST || c->a))
					c = C (btmPalette, bottom_data, BTMIDX, bBtmTGA, &bST);
				dest_data [i] = *c;
			}
		break;
	case 3:
		// Normal
		for (i = y = 0; y < dh; y++)
			for (x = 0; x < dw; x++, i++) {
				c = C (top_pal, top_data, tw * (th - 1 - TOPSCALE (x)) + TOPSCALE (y), bTopTGA, &bST);
				if (!(bST || c->a))
					c = C (btmPalette, bottom_data, BTMIDX, bBtmTGA, &bST);
				dest_data [i] = *c;
			}
		break;
	}
}

//------------------------------------------------------------------------------

GLhandleARB tmShaderProgs [3] = {0,0,0};
GLhandleARB tmf [3] = {0,0,0}; 
GLhandleARB tmv [3] = {0,0,0}; 

#ifdef DBG_SHADERS

char *texMergeFS [3] = {"texmerge1.frag", "texmerge2.frag","texmerge3.frag"};
char *texMergeVS [3] = {"texmerge12.vert", "texmerge12.vert", "texmerge3.vert"};

#else

char *texMergeFS [3] = {
	"uniform sampler2D btmTex, topTex;" \
	"uniform float grAlpha;" \
	"void main(void){" \
	"vec4 topColor=texture2D(topTex,vec2(gl_TexCoord [1]));" \
	"vec4 btmColor=texture2D(btmTex,vec2(gl_TexCoord [0]));" \
	"gl_FragColor=vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;}"
,
	"uniform sampler2D btmTex, topTex;" \
	"uniform float grAlpha;" \
	"void main(void)" \
	"{vec4 topColor=texture2D(topTex,vec2(gl_TexCoord [1]));" \
	"if(abs(topColor.a*255.0-1.0)<0.1)discard;" \
	"else{" \
	"vec4 btmColor=texture2D(btmTex,vec2(gl_TexCoord [0]));"\
	"if(topColor.a==0.0)gl_FragColor=vec4(vec3(btmColor),btmColor.a*grAlpha)*gl_Color;" \
	"else gl_FragColor=vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;}}"
,
	"uniform sampler2D btmTex, topTex, maskTex;" \
	"uniform float grAlpha;" \
	"void main(void)" \
	"{float bMask=texture2D(maskTex,vec2(gl_TexCoord [2])).a;" \
	"if(bMask<0.5)discard;" \
	"else {vec4 topColor=texture2D(topTex,vec2(gl_TexCoord [1]));" \
	"vec4 btmColor=texture2D(btmTex,vec2(gl_TexCoord [0]));"\
	"if(topColor.a==0.0)gl_FragColor=vec4(vec3(btmColor),btmColor.a*grAlpha)*gl_Color;" \
	"else gl_FragColor=vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;}}"
	};

char *texMergeVS [3] = {
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;"\
	"gl_FrontColor=gl_Color;}"
,
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;"\
	"gl_FrontColor=gl_Color;}"
,
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_TexCoord [2]=gl_MultiTexCoord2;"\
	"gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;"\
	"gl_FrontColor=gl_Color;}"
	};

#endif

char *texMergeFSData = 
	"uniform sampler2D btmTex, topTex, maskTex;" \
	"uniform float grAlpha;";



//-------------------------------------------------------------------------

void InitTexMergeShaders (void)
{
	int	i, b;

if (!gameStates.ogl.bShadersOk)
	gameOpts->ogl.bGlTexMerge = 0;
if (!gameOpts->ogl.bGlTexMerge)
	gameStates.render.textures.bGlsTexMergeOk = 0;
else {
	LogErr ("building texturing shader programs\n");
	for (i = 0; i < 2; i++) {
		if (tmShaderProgs [i])
			DeleteShaderProg (tmShaderProgs + i);
		b = CreateShaderProg (tmShaderProgs + i) &&
			 CreateShaderFunc (tmShaderProgs + i, tmf + i, tmv + i, texMergeFS [i], texMergeVS [i], 1) &&
			 LinkShaderProg (tmShaderProgs + i);
		if (i == 2)
			gameStates.render.textures.bHaveMaskShader = b;
		else
			gameStates.render.textures.bGlsTexMergeOk = b;
		if (!gameStates.render.textures.bGlsTexMergeOk) {
			while (i)
				DeleteShaderProg (tmShaderProgs + --i);
			break;
			}
		}
	if (!gameStates.render.textures.bGlsTexMergeOk)
		gameOpts->ogl.bGlTexMerge = 0;
	}
if (!(gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk))
	gameStates.ogl.bHaveTexCompression = 0;
}

//------------------------------------------------------------------------------

