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

/*
 *
 * Routines to cache merged textures.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:31:36  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:31:08  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.28  1995/01/14  19:16:56  john
 * First version of new bitmap paging code.
 *
 * Revision 1.27  1994/12/14  18:21:58  yuan
 * *** empty log message ***
 *
 * Revision 1.26  1994/12/13  09:50:08  john
 * Added Asserts to stop if wall looks like door.
 *
 * Revision 1.25  1994/12/07  00:35:24  mike
 * change how flat shading average color is computed for paste-ons.
 *
 * Revision 1.24  1994/11/19  15:20:29  mike
 * rip out unused code and data
 *
 * Revision 1.23  1994/11/12  16:38:51  mike
 * deal with bm_avgColor in texture merging.
 *
 * Revision 1.22  1994/11/09  19:55:39  john
 * Added full rle support with texture rle caching.
 *
 * Revision 1.21  1994/10/20  15:21:16  john
 * Took out the texmerge caching.
 *
 * Revision 1.20  1994/10/10  19:00:57  john
 * Made caching info print every 1000 frames.
 *
 * Revision 1.19  1994/10/10  18:41:21  john
 * Printed out texture caching info.
 *
 * Revision 1.18  1994/08/11  18:59:02  mike
 * Use new assembler version of merge functions.
 *
 * Revision 1.17  1994/06/09  12:13:14  john
 * Changed selectors so that all bitmaps have a selector of
 * 0, but inside the texture mapper they get a selector set.
 *
 * Revision 1.16  1994/05/14  17:15:15  matt
 * Got rid of externs in source (non-header) files
 *
 * Revision 1.15  1994/05/09  17:21:09  john
 * Took out con_printf with cache hits/misses.
 *
 * Revision 1.14  1994/05/05  12:55:07  john
 * Made SuperTransparency work.
 *
 * Revision 1.13  1994/05/04  11:15:37  john
 * Added Super Transparency
 *
 * Revision 1.12  1994/04/28  23:36:04  john
 * Took out a debugging con_printf.
 *
 * Revision 1.11  1994/04/22  17:44:48  john
 * Made top 2 bits of paste-ons pick the
 * orientation of the bitmap.
 *
 * Revision 1.10  1994/03/31  12:05:51  matt
 * Cleaned up includes
 *
 * Revision 1.9  1994/03/15  16:31:52  yuan
 * Cleaned up bm-loading code.
 * (And structures)
 *
 * Revision 1.8  1994/01/24  13:15:19  john
 * Made caching work with pointers, not texture numbers,
 * that way, the animated textures cache.
 *
 * Revision 1.7  1994/01/21  16:38:10  john
 * Took out debug info.
 *
 * Revision 1.6  1994/01/21  16:28:43  john
 * added warning to print cache hit/miss.
 *
 * Revision 1.5  1994/01/21  16:22:30  john
 * Put in caching/
 *
 * Revision 1.4  1994/01/21  15:34:49  john
 * *** empty log message ***
 *
 * Revision 1.3  1994/01/21  15:33:08  john
 * *** empty log message ***
 *
 * Revision 1.2  1994/01/21  15:15:35  john
 * Created new module texmerge, that merges textures together and
 * caches the results.
 *
 * Revision 1.1  1994/01/21  14:55:29  john
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>

#include "pa_enabl.h"                   //$$POLY_ACC
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

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#ifdef OGL
#include "ogl_init.h"
#define MAX_NUM_CACHE_BITMAPS 200
#else
#define MAX_NUM_CACHE_BITMAPS 50
#endif

//static grs_bitmap * cache_bitmaps [MAX_NUM_CACHE_BITMAPS];                     

typedef struct	{
	grs_bitmap * bitmap;
	grs_bitmap * bmBot;
	grs_bitmap * bmTop;
	int 		nOrient;
	int		last_frame_used;
} TEXTURE_CACHE;

static TEXTURE_CACHE texCache [MAX_NUM_CACHE_BITMAPS];

static int nCacheEntries = 0;

static int nCacheHits = 0;
static int nCacheMisses = 0;

void MergeTextures (int type, grs_bitmap *bmBot, grs_bitmap *bmTop, grs_bitmap *dest_bmp, int bSuperTransp);
void MergeTexturesNormal (int type, grs_bitmap *bmBot, grs_bitmap *bmTop, ubyte *dest_data);

#if defined(POLY_ACC)       // useful to all of D2 I think.
extern grs_bitmap * rle_get_id_sub( grs_bitmap * bmp );

//----------------------------------------------------------------------
// Given pointer to a bitmap returns a unique value that describes the bitmap.
// Returns 0xFFFFFFFF if this bitmap isn't a texmerge'd bitmap.
uint texmerge_get_unique_id( grs_bitmap * bmp )
{
    int i,n;
    uint tmp;
    grs_bitmap * tmpbmp;

// Check in texmerge cache
for (i = 0; i < nCacheEntries; i++ ) {
   if ((texCache [i].last_frame_used > -1) && (texCache [i].bitmap == bmp)) {
      tmp = (uint)texCache [i].nOrient<<30;
      tmp |= ((uint)(texCache [i].bmTop - gameData.pig.tex.bitmaps)) << 16;
      tmp |= (uint)(texCache [i].bmBot - gameData.pig.tex.bitmaps);
      return tmp;
      }
   }
// Check in rle cache
tmpbmp = rle_get_id_sub( bmp );
if (tmpbmp)
	return (uint)(tmpbmp-gameData.pig.tex.bitmaps);
// Must be a normal bitmap
return (uint)(bmp-gameData.pig.tex.bitmaps);
}
#endif

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
atexit( TexMergeClose );
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
grs_bitmap * TexMergeGetCachedBitmap (int tMapBot, int tMapTop)
{
	grs_bitmap		*bmTop, *bmBot, *bmP;
	int				i, nOrient, nLowestFrame, nLRU;
	TEXTURE_CACHE	*cacheP;

nOrient = ((tMapTop & 0xC000) >> 14) & 3;
tMapTop &= 0x3fff;
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
#ifdef OGL
if (bmP)
	OglFreeBmTexture(bmP);
#endif

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

void MergeTexturesNormal (
	int type, grs_bitmap * bmBot, grs_bitmap * bmTop, ubyte * dest_data )
{
	ubyte * top_data, *bottom_data;
	int scale;

	if (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk)
		return;
	if ( bmTop->bm_props.flags & BM_FLAG_RLE )
		bmTop = rle_expand_texture(bmTop);

	if ( bmBot->bm_props.flags & BM_FLAG_RLE )
		bmBot = rle_expand_texture(bmBot);

//	Assert( bmBot != bmTop );

	top_data = bmTop->bm_texBuf;
	bottom_data = bmBot->bm_texBuf;
	scale = bmBot->bm_props.w / bmTop->bm_props.w;
	if (!scale)
		scale = 1;
	if (scale > 1)
		scale = scale;

//	Assert( bottom_data != top_data );

	switch( type )	{
	case 0:
		// Normal

		GrMergeTextures( bottom_data, top_data, dest_data, bmBot->bm_props.w, bmBot->bm_props.h, scale );
		break;
	case 1:
		GrMergeTextures1( bottom_data, top_data, dest_data, bmBot->bm_props.w, bmBot->bm_props.h, scale );
		break;
	case 2:
		GrMergeTextures2( bottom_data, top_data, dest_data, bmBot->bm_props.w, bmBot->bm_props.h, scale );
		break;
	case 3:
		GrMergeTextures3( bottom_data, top_data, dest_data, bmBot->bm_props.w, bmBot->bm_props.h, scale );
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
	int type, grs_bitmap * bmBot, grs_bitmap * bmTop, grs_bitmap *dest_bmp, int bSuperTransp)
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

//	Assert( bmBot != bmTop );

top_data = bmTop->bm_texBuf;
bottom_data = bmBot->bm_texBuf;
top_pal = bmTop->bm_palette;
btmPalette = bmBot->bm_palette;

//	Assert( bottom_data != top_data );

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
switch( type )	{
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

#ifdef _DEBUG

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
	"if(abs(topColor.a*255.0-1.0)<0.5)discard;" \
	"else{"
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
}

//------------------------------------------------------------------------------

