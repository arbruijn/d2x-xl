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
#include "ogl_defs.h"
#include "ogl_shader.h"

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
void _CDECL_ TexMergeClose (void);

//----------------------------------------------------------------------

int TexMergeInit (int nCacheSize)
{
	int i;
	TEXTURE_CACHE *cacheP = texCache;

nCacheEntries = ((nCacheSize > 0) && (nCacheSize <= MAX_NUM_CACHE_BITMAPS)) ? nCacheSize  : MAX_NUM_CACHE_BITMAPS;
for (i = 0; i < nCacheEntries; i++, cacheP++) {
	cacheP->last_frame_used = -1;
	cacheP->bmTop =
	cacheP->bmBot =
	cacheP->bitmap = NULL;
	cacheP->nOrient = -1;
	}
atexit (TexMergeClose);
return 1;
}

//----------------------------------------------------------------------

void TexMergeFlush()
{
	int i;

for (i = 0; i < nCacheEntries; i++) {
	texCache [i].last_frame_used = -1;
	texCache [i].nOrient = -1;
	texCache [i].bmTop =
	texCache [i].bmBot = NULL;
	}
}

//-------------------------------------------------------------------------

void _CDECL_ TexMergeClose (void)
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
bmTop = BmOverride (gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [tMapTop].index, -1);
bmBot = BmOverride (gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [tMapBot].index, -1);

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
PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tMapTop].index, gameStates.app.bD1Mission);
PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tMapBot].index, gameStates.app.bD1Mission);
if (gameData.pig.tex.bPageFlushed)	{	// If cache got flushed, re-read 'em.
	gameData.pig.tex.bPageFlushed = 0;
	PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tMapTop].index, gameStates.app.bD1Mission);
	PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tMapBot].index, gameStates.app.bD1Mission);
	}
Assert (gameData.pig.tex.bPageFlushed == 0);
#endif

bmTop = BmOverride (gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [tMapTop].index, -1);
bmBot = BmOverride (gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [tMapBot].index, -1);
if (!bmTop->bmPalette)
	bmTop->bmPalette = gamePalette;
if (!bmBot->bmPalette)
	bmBot->bmPalette = gamePalette;
cacheP = texCache + nLRU;
bmP = cacheP->bitmap;
if (bmP)
	OglFreeBmTexture(bmP);

// if necessary, allocate cache bitmap
// in any case make sure the cache bitmap has the proper size
if (!bmP ||
	(bmP->bmProps.w != bmBot->bmProps.w) || 
	(bmP->bmProps.h != bmBot->bmProps.h)) {
	if (bmP)
		GrFreeBitmap (bmP);
	cacheP->bitmap =
	bmP = GrCreateBitmap (bmBot->bmProps.w, bmBot->bmProps.h, 4);
	if (!bmP)
		return NULL;
	}
else
	bmP->bmProps.flags = (char) BM_FLAG_TGA;
if (!bmP->bmTexBuf)
	return NULL;
bmP->bmPalette = gamePalette;
if (!(gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk)) {
	if (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) {
//			return bmTop;
		MergeTextures (nOrient, bmBot, bmTop, bmP, 1);
		bmP->bmProps.flags |= BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT;
		bmP->bmAvgColor = bmTop->bmAvgColor;
		}
	else {
//			MergeTexturesNormal (nOrient, bmBot, bmTop, bmP->bmTexBuf);
		MergeTextures (nOrient, bmBot, bmTop, bmP, 0);
		bmP->bmProps.flags |= bmBot->bmProps.flags & (~BM_FLAG_RLE);
		bmP->bmAvgColor = bmBot->bmAvgColor;
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
if (bmTop->bmProps.flags & BM_FLAG_RLE)
	bmTop = rle_expand_texture(bmTop);
if (bmBot->bmProps.flags & BM_FLAG_RLE)
	bmBot = rle_expand_texture(bmBot);
//	Assert(bmBot != bmTop);
top_data = bmTop->bmTexBuf;
bottom_data = bmBot->bmTexBuf;
scale = bmBot->bmProps.w / bmTop->bmProps.w;
if (!scale)
	scale = 1;
if (scale > 1)
	scale = scale;
//	Assert(bottom_data != top_data);
switch(nType)	{
	case 0:
		// Normal
		GrMergeTextures(bottom_data, top_data, dest_data, bmBot->bmProps.w, bmBot->bmProps.h, scale);
		break;
	case 1:
		GrMergeTextures1(bottom_data, top_data, dest_data, bmBot->bmProps.w, bmBot->bmProps.h, scale);
		break;
	case 2:
		GrMergeTextures2(bottom_data, top_data, dest_data, bmBot->bmProps.w, bmBot->bmProps.h, scale);
		break;
	case 3:
		GrMergeTextures3(bottom_data, top_data, dest_data, bmBot->bmProps.w, bmBot->bmProps.h, scale);
		break;
	}
}

//-------------------------------------------------------------------------

typedef struct frac {
	int	c, d;
} frac;

inline tRGBA *C (ubyte *palP, ubyte *b, int i, int bpp, int *pbST)
{
	static	tRGBA	c;
	int bST;

if (bpp == 4) {
	c = ((tRGBA *) (b)) [i];
	if ((*pbST = ((c.r == 120) && (c.g == 88) && (c.b == 128))))
		c.a = 0;
	return &c;
	}
if (bpp == 3) {
	*((tRGB *) &c) = ((tRGB *) (b)) [i];
	if ((*pbST = ((c.r == 120) && (c.g == 88) && (c.b == 128))))
		c.a = 0;
	else
		c.a = 255;
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
	int		bTopBPP, bBtmBPP, bST = 0;
	frac		topScale, btmScale;
	tRGBA		*dest_data = (tRGBA *) dest_bmp->bmTexBuf;

	ubyte * top_data, *bottom_data, *top_pal, *btmPalette;

bmBot = BmOverride (bmBot, -1);
bmTop = BmOverride (bmTop, -1);
if (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk)
	return;
if (bmTop->bmProps.flags & BM_FLAG_RLE)
	bmTop = rle_expand_texture (bmTop);

if (bmBot->bmProps.flags & BM_FLAG_RLE)
	bmBot = rle_expand_texture (bmBot);

//	Assert(bmBot != bmTop);

top_data = bmTop->bmTexBuf;
bottom_data = bmBot->bmTexBuf;
top_pal = bmTop->bmPalette;
btmPalette = bmBot->bmPalette;

//	Assert(bottom_data != top_data);

//Int3();
bh =
bw = bmBot->bmProps.w;
//h = bmBot->bmProps.h;
th =
tw = bmTop->bmProps.w;
dw =
dh = dest_bmp->bmProps.w;
//th = bmTop->bmProps.h;
#if 1
// square textures assumed here, so no test for h!
if (dw < tw) {
	topScale.c = tw / dw;
	topScale.d = 1;
	}
else  {
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
if (w > bmTop->bmProps.w)
	w = h = bmBot->bmProps.w;
scale.c = scale.d = 1;
#endif
bTopBPP = bmTop->bmBPP;
bBtmBPP = bmBot->bmBPP;
#ifdef _DEBUG
memset (dest_data, 253, dest_bmp->bmProps.w * dest_bmp->bmProps.h * 4);
#endif
switch(nType)	{
	case 0:
		// Normal
		for (i = y = 0; y < dh; y++)
			for (x = 0; x < dw; x++, i++) {
				c = C (top_pal, top_data, tw * TOPSCALE (y) + TOPSCALE (x), bTopBPP, &bST);
				if (!(bST || c->a))
					c = C (btmPalette, bottom_data, BTMIDX, bBtmBPP, &bST);
				dest_data [i] = *c;
			}
		break;
	case 1:
		// 
		for (i = y = 0; y < dh; y++)
			for (x = 0; x < dw; x++, i++) {
				c = C (top_pal, top_data, tw * TOPSCALE (x) + th - 1 - TOPSCALE (y), bTopBPP, &bST);
				if (!(bST || c->a))
					c = C (btmPalette, bottom_data, BTMIDX, bBtmBPP, &bST);
				dest_data [i] = *c;
			}
		break;
	case 2:
		// Normal
		for (i = y = 0; y < dh; y++)
			for (x = 0; x < dw; x++, i++) {
				c = C (top_pal, top_data, tw * (th - 1 - TOPSCALE (y)) + tw - 1 - TOPSCALE (x), bTopBPP, &bST);
				if (!(bST || c->a))
					c = C (btmPalette, bottom_data, BTMIDX, bBtmBPP, &bST);
				dest_data [i] = *c;
			}
		break;
	case 3:
		// Normal
		for (i = y = 0; y < dh; y++)
			for (x = 0; x < dw; x++, i++) {
				c = C (top_pal, top_data, tw * (th - 1 - TOPSCALE (x)) + TOPSCALE (y), bTopBPP, &bST);
				if (!(bST || c->a))
					c = C (btmPalette, bottom_data, BTMIDX, bBtmBPP, &bST);
				dest_data [i] = *c;
			}
		break;
	}
}

//------------------------------------------------------------------------------

GLhandleARB tmShaderProgs [3] = {0,0,0};
GLhandleARB tmf [3] = {0,0,0}; 
GLhandleARB tmv [3] = {0,0,0}; 

#if DBG_SHADERS

char *texMergeFS [3] = {"texmerge1.frag", "texmerge2.frag","texmerge3.frag"};
char *texMergeVS [3] = {"texmerge12.vert", "texmerge12.vert", "texmerge3.vert"};

#else

char *texMergeFS [3] = {
	"uniform sampler2D btmTex, topTex;\r\n" \
	"uniform float grAlpha;\r\n" \
	"uniform float bColored;\r\n" \
	"void main(void){" \
	"vec4 topColor=texture2D(topTex,gl_TexCoord [1].xy);\r\n" \
	"vec4 btmColor=texture2D(btmTex,gl_TexCoord [0].xy);\r\n" \
	"if (bColored != 0.0)\r\n" \
	"   gl_FragColor=vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;\r\n" \
	"else {\r\n" \
	"   vec4 color = vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;\r\n" \
	"   float l = (color.r + color.g + color.b) / 3.0;\r\n" \
	"   gl_FragColor = vec4 (l, l, l, color.a);\r\n" \
   "   }\r\n" \
   "}"
,
	"uniform sampler2D btmTex, topTex;\r\n" \
	"uniform float grAlpha;\r\n" \
	"uniform float bColored;\r\n" \
	"vec4 topColor, btmColor;\r\n" \
	"void main(void)" \
	"{topColor=texture2D(topTex,gl_TexCoord [1].xy);\r\n" \
	"if((abs(topColor.r-120.0/255.0)<8.0/255.0)&&(abs(topColor.g-88.0/255.0)<8.0/255.0)&&(abs(topColor.b-128.0/255.0)<8.0/255.0))discard;\r\n" \
	"btmColor=texture2D(btmTex,gl_TexCoord [0].xy);\r\n" \
	"if (bColored != 0.0)\r\n" \
	"   gl_FragColor=vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;\r\n" \
	"else {\r\n" \
	"   vec4 color = vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;\r\n" \
	"   float l = (color.r + color.g + color.b) / 3.0;\r\n" \
	"   gl_FragColor = vec4 (l, l, l, color.a);\r\n" \
   "   }\r\n" \
   "}"
,
	"uniform sampler2D btmTex, topTex, maskTex;\r\n" \
	"uniform float grAlpha;\r\n" \
	"uniform float bColored;\r\n" \
	"vec4 topColor, btmColor;\r\n" \
	"float bMask;\r\n" \
	"void main(void){" \
	"bMask = texture2D(maskTex,gl_TexCoord [2].xy).r;\r\n" \
	"if (bMask < 0.5)" \
	"   discard;\r\n" \
	"else {\r\n" \
	"   topColor=texture2D(topTex,gl_TexCoord [1].xy);\r\n" \
	"   btmColor=texture2D(btmTex,gl_TexCoord [0].xy);\r\n" \
	"   if (bColored != 0.0)\r\n" \
	"      gl_FragColor = bMask * vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;\r\n" \
	"   else {\r\n" \
	"      vec4 color = vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;\r\n" \
	"      float l = (color.r + color.g + color.b) / 3.0;\r\n" \
	"      gl_FragColor = bMask * vec4 (l, l, l, color.a);\r\n" \
   "      }\r\n" \
   "   }\r\n" \
	"}"
	};

char *texMergeVS [3] = {
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_Position=ftransform() /*gl_ModelViewProjectionMatrix * gl_Vertex*/;"\
	"gl_FrontColor=gl_Color;}"
,
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_Position=ftransform() /*gl_ModelViewProjectionMatrix * gl_Vertex*/;"\
	"gl_FrontColor=gl_Color;}"
,
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_TexCoord [1]=gl_MultiTexCoord1;"\
	"gl_TexCoord [2]=gl_MultiTexCoord2;"\
	"gl_Position=ftransform() /*gl_ModelViewProjectionMatrix * gl_Vertex*/;"\
	"gl_FrontColor=gl_Color;}"
	};

#endif

char *texMergeFSData = 
	"uniform sampler2D btmTex, topTex, maskTex;\r\n" \
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
	for (i = 0; i < 3; i++) {
		if (tmShaderProgs [i])
			DeleteShaderProg (tmShaderProgs + i);
		b = CreateShaderProg (tmShaderProgs + i) &&
			 CreateShaderFunc (tmShaderProgs + i, tmf + i, tmv + i, texMergeFS [i], texMergeVS [i], 1) &&
-			 LinkShaderProg (tmShaderProgs + i);
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

