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
#include "error.h"
#include "u_mem.h"
#include "textures.h"
#include "rle.h"
#include "ogl_shader.h"
#include "ogl_render.h"

#define MAX_NUM_CACHE_BITMAPS 200

//static CBitmap * cache_bitmaps [MAX_NUM_CACHE_BITMAPS];                     

typedef struct {
	CBitmap*		bitmap;
	CBitmap* 	bmBot;
	CBitmap* 	bmTop;
	int 			nOrient;
	int			last_frame_used;
} TEXTURE_CACHE;

static TEXTURE_CACHE texCache [MAX_NUM_CACHE_BITMAPS];

static int nCacheEntries = 0;

static int nCacheHits = 0;
static int nCacheMisses = 0;

void MergeTextures (int nType, CBitmap *bmBot, CBitmap *bmTop, CBitmap *dest_bmp, int bSuperTransp);
void MergeTexturesNormal (int nType, CBitmap *bmBot, CBitmap *bmTop, ubyte *dest_data);
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

void TexMergeFlush (void)
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

PrintLog ("shutting down merged textures cache\n");
for (i = 0; i < nCacheEntries; i++) {
	if (texCache [i].bitmap) {
		delete texCache [i].bitmap;
		texCache [i].bitmap = NULL;
		}
	}
nCacheEntries = 0;
}

//-------------------------------------------------------------------------
//--unused-- int info_printed = 0;
CBitmap * TexMergeGetCachedBitmap (int tMapBot, int tMapTop, int nOrient)
{
	CBitmap			*bmTop, *bmBot, *bmP;
	int				i, nLowestFrame, nLRU;
	TEXTURE_CACHE	*cacheP;

nLRU = 0;
nLowestFrame = texCache [0].last_frame_used;
bmTop = gameData.pig.tex.bitmapP [gameData.pig.tex.bmIndexP [tMapTop].index].Override (-1);
bmBot = gameData.pig.tex.bitmapP [gameData.pig.tex.bmIndexP [tMapBot].index].Override (-1);

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
	if (cacheP->last_frame_used < nLowestFrame) {
		nLowestFrame = cacheP->last_frame_used;
		nLRU = i;
		}
	}
//---- Page out the LRU bitmap;
nCacheMisses++;
// Make sure the bitmaps are paged in...
#ifdef PIGGY_USE_PAGING
gameData.pig.tex.bPageFlushed = 0;
PIGGY_PAGE_IN (gameData.pig.tex.bmIndexP [tMapTop].index, gameStates.app.bD1Mission);
PIGGY_PAGE_IN (gameData.pig.tex.bmIndexP [tMapBot].index, gameStates.app.bD1Mission);
if (gameData.pig.tex.bPageFlushed) {	// If cache got flushed, re-read 'em.
	gameData.pig.tex.bPageFlushed = 0;
	PIGGY_PAGE_IN (gameData.pig.tex.bmIndexP [tMapTop].index, gameStates.app.bD1Mission);
	PIGGY_PAGE_IN (gameData.pig.tex.bmIndexP [tMapBot].index, gameStates.app.bD1Mission);
	}
Assert (gameData.pig.tex.bPageFlushed == 0);
#endif

bmTop = gameData.pig.tex.bitmapP [gameData.pig.tex.bmIndexP [tMapTop].index].Override (-1);
bmBot = gameData.pig.tex.bitmapP [gameData.pig.tex.bmIndexP [tMapBot].index].Override (-1);
if (!bmTop->Palette ())
	bmTop->SetPalette (paletteManager.Game ());
if (!bmBot->Palette ())
	bmBot->SetPalette (paletteManager.Game ());
cacheP = texCache + nLRU;
bmP = cacheP->bitmap;
if (bmP)
	bmP->ReleaseTexture ();

// if necessary, allocate cache bitmap
// in any case make sure the cache bitmap has the proper size
if (!bmP ||
	(bmP->Width () != bmBot->Width ()) || 
	(bmP->Height () != bmBot->Height ())) {
	if (bmP)
		delete bmP;
	bmP = CBitmap::Create (0, bmBot->Width (), bmBot->Height (), 4);
	if (!bmP)
		return NULL;
	}
else
	bmP->SetFlags ((char) BM_FLAG_TGA);
if (!bmP->Buffer ())
	return NULL;
bmP->SetPalette (paletteManager.Game ());
if (!(gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk)) {
	if (bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) {
//			return bmTop;
		MergeTextures (nOrient, bmBot, bmTop, bmP, 1);
		bmP->AddFlags (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT);
		bmP->SetAvgColorIndex (bmTop->AvgColorIndex ());
		}
	else {
//			MergeTexturesNormal (nOrient, bmBot, bmTop, bmP->Buffer ());
		MergeTextures (nOrient, bmBot, bmTop, bmP, 0);
		bmP->AddFlags (bmBot->Flags () & (~BM_FLAG_RLE));
		bmP->SetAvgColorIndex (bmBot->AvgColorIndex ());
		}
	}
cacheP->bitmap = bmP;
cacheP->bmTop = bmTop;
cacheP->bmBot = bmBot;
cacheP->last_frame_used = gameData.app.nFrameCount;
cacheP->nOrient = nOrient;
return bmP;
}

//-------------------------------------------------------------------------

void MergeTexturesNormal (int nType, CBitmap * bmBot, CBitmap * bmTop, ubyte * dest_data)
{
	ubyte * top_data, *bottom_data;
	int scale;

if (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk)
	return;
if (bmTop->Flags () & BM_FLAG_RLE)
	bmTop = rle_expand_texture(bmTop);
if (bmBot->Flags () & BM_FLAG_RLE)
	bmBot = rle_expand_texture(bmBot);
//	Assert(bmBot != bmTop);
top_data = bmTop->Buffer ();
bottom_data = bmBot->Buffer ();
scale = bmBot->Width () / bmTop->Width ();
if (!scale)
	scale = 1;
if (scale > 1)
	scale = scale;
//	Assert(bottom_data != top_data);
switch(nType) {
	case 0:
		// Normal
		GrMergeTextures(bottom_data, top_data, dest_data, bmBot->Width (), bmBot->Height (), scale);
		break;
	case 1:
		GrMergeTextures1(bottom_data, top_data, dest_data, bmBot->Width (), bmBot->Height (), scale);
		break;
	case 2:
		GrMergeTextures2(bottom_data, top_data, dest_data, bmBot->Width (), bmBot->Height (), scale);
		break;
	case 3:
		GrMergeTextures3(bottom_data, top_data, dest_data, bmBot->Width (), bmBot->Height (), scale);
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
	c = reinterpret_cast<tRGBA*> (b) [i];
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

//#define TOPIDX	TexIdx (x, y, tw, topScale)
#define BTMIDX	TexIdx (x, y, bw, btmScale)


void MergeTextures (
	int nType, CBitmap * bmBot, CBitmap * bmTop, CBitmap *dest_bmp, int bSuperTransp)
{
	tRGBA		*c;
	int		i, x, y, bw, bh, tw, th, dw, dh;
	int		bTopBPP, bBtmBPP, bST = 0;
	frac		topScale, btmScale;
	tRGBA		*dest_data = reinterpret_cast<tRGBA*> (dest_bmp->Buffer ());

	ubyte		*top_data, *bottom_data, *top_pal, *btmPalette;

bmBot = bmBot->Override (-1);
bmTop = bmTop->Override (-1);
if (gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk)
	return;
if (bmTop->Flags () & BM_FLAG_RLE)
	bmTop = rle_expand_texture (bmTop);

if (bmBot->Flags () & BM_FLAG_RLE)
	bmBot = rle_expand_texture (bmBot);

//	Assert(bmBot != bmTop);

top_data = bmTop->Buffer ();
bottom_data = bmBot->Buffer ();
top_pal = bmTop->Palette ()->Raw ();
btmPalette = bmBot->Palette ()->Raw ();

//	Assert(bottom_data != top_data);

//Int3();
bh =
bw = bmBot->Width ();
//h = bmBot->Height ();
th =
tw = bmTop->Width ();
dw =
dh = dest_bmp->Width ();
//th = bmTop->Height ();
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
if (w > bmTop->Width ())
	w = h = bmBot->Width ();
scale.c = scale.d = 1;
#endif
bTopBPP = bmTop->BPP ();
bBtmBPP = bmBot->BPP ();
#if DBG
memset (dest_data, 253, dest_bmp->Width () * dest_bmp->Height () * 4);
#endif
switch(nType) {
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

GLhandleARB tmShaderProgs [6] = {0,0,0,0,0,0};
GLhandleARB tmf [6] = {0,0,0,0,0,0}; 
GLhandleARB tmv [6] = {0,0,0,0,0,0}; 

const char *texMergeFS [6] = {
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float grAlpha;\r\n" \
	"void main(void){" \
	"vec4 decalColor=texture2D(decalTex,gl_TexCoord [1].xy);\r\n" \
	"vec4 texColor=texture2D(baseTex,gl_TexCoord [0].xy);\r\n" \
	"vec4 color = vec4(vec3(mix(texColor,decalColor,decalColor.a)),min (1.0,(texColor.a+decalColor.a))*grAlpha)*gl_Color;\r\n" \
	"float l = (color.r + color.g + color.b) / 3.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, color.a);\r\n" \
   "}"
,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float grAlpha;\r\n" \
	"vec4 decalColor, texColor;\r\n" \
	"void main(void)" \
	"{decalColor=texture2D(decalTex,gl_TexCoord [1].xy);\r\n" \
	"if((abs(decalColor.r-120.0/255.0)<8.0/255.0)&&(abs(decalColor.g-88.0/255.0)<8.0/255.0)&&(abs(decalColor.b-128.0/255.0)<8.0/255.0))discard;\r\n" \
	"texColor=texture2D(baseTex,gl_TexCoord [0].xy);\r\n" \
	"vec4 color = vec4(vec3(mix(texColor,decalColor,decalColor.a)),min (1.0,(texColor.a+decalColor.a))*grAlpha)*gl_Color;\r\n" \
	"float l = (color.r + color.g + color.b) / 3.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, color.a);\r\n" \
   "}"
,
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform float grAlpha;\r\n" \
	"vec4 decalColor, texColor;\r\n" \
	"float bMask;\r\n" \
	"void main(void){" \
	"bMask = texture2D(maskTex,gl_TexCoord [2].xy).r;\r\n" \
	"decalColor=texture2D(decalTex,gl_TexCoord [1].xy);\r\n" \
	"texColor=texture2D(baseTex,gl_TexCoord [0].xy);\r\n" \
	"vec4 color = vec4(vec3(mix(texColor,decalColor,decalColor.a)),min (1.0,(texColor.a+decalColor.a))*grAlpha)*gl_Color;\r\n" \
	"float l = (color.r + color.g + color.b) / 3.0;\r\n" \
	"gl_FragColor = bMask * vec4 (l, l, l, color.a);\r\n" \
	"}"
,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float grAlpha;\r\n" \
	"void main(void){" \
	"vec4 decalColor=texture2D(decalTex,gl_TexCoord [1].xy);\r\n" \
	"vec4 texColor=texture2D(baseTex,gl_TexCoord [0].xy);\r\n" \
	"gl_FragColor=vec4(vec3(mix(texColor,decalColor,decalColor.a)),min (1.0,(texColor.a+decalColor.a))*grAlpha)*gl_Color;\r\n" \
   "}"
,
	"uniform sampler2D baseTex, decalTex;\r\n" \
	"uniform float grAlpha;\r\n" \
	"vec4 decalColor, texColor;\r\n" \
	"void main(void)" \
	"{decalColor=texture2D(decalTex,gl_TexCoord [1].xy);\r\n" \
	"if((abs(decalColor.r-120.0/255.0)<8.0/255.0)&&(abs(decalColor.g-88.0/255.0)<8.0/255.0)&&(abs(decalColor.b-128.0/255.0)<8.0/255.0))discard;\r\n" \
	"texColor=texture2D(baseTex,gl_TexCoord [0].xy);\r\n" \
	"gl_FragColor=vec4(vec3(mix(texColor,decalColor,decalColor.a)),min (1.0,(texColor.a+decalColor.a))*grAlpha)*gl_Color;\r\n" \
   "}"
,
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
	"uniform float grAlpha;\r\n" \
	"uniform float bColored;\r\n" \
	"vec4 decalColor, texColor;\r\n" \
	"float bMask;\r\n" \
	"void main(void){" \
	"bMask = texture2D(maskTex,gl_TexCoord [2].xy).r;\r\n" \
	"decalColor=texture2D(decalTex,gl_TexCoord [1].xy);\r\n" \
	"texColor=texture2D(baseTex,gl_TexCoord [0].xy);\r\n" \
	"gl_FragColor = bMask * vec4(vec3(mix(texColor,decalColor,decalColor.a)),min (1.0,(texColor.a+decalColor.a))*grAlpha)*gl_Color;\r\n" \
	"}"
	};

const char *texMergeVS [3] = {
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

const char *texMergeFSData = 
	"uniform sampler2D baseTex, decalTex, maskTex;\r\n" \
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
	PrintLog ("building texturing shader programs\n");
	for (i = 0; i < 6; i++) {
		if (tmShaderProgs [i])
			DeleteShaderProg (tmShaderProgs + i);
		b = CreateShaderProg (tmShaderProgs + i) &&
			 CreateShaderFunc (tmShaderProgs + i, tmf + i, tmv + i, texMergeFS [i], texMergeVS [i % 3], 1) &&
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

int G3SetupTexMergeShader (int bColorKey, int bColored, int nType)
{
int nShader = nType - 1;
if (nShader != gameStates.render.history.nShader) {
	gameData.render.nShaderChanges++;
	glUseProgramObject (activeShaderProg = tmShaderProgs [nShader + bColored * 3]);
	}
glUniform1i (glGetUniformLocation (activeShaderProg, "baseTex"), 0);
glUniform1i (glGetUniformLocation (activeShaderProg, "decalTex"), 1);
glUniform1i (glGetUniformLocation (activeShaderProg, "maskTex"), 2);
glUniform1f (glGetUniformLocation (activeShaderProg, "grAlpha"), 1.0f);
return gameStates.render.history.nShader = nShader;
}

//------------------------------------------------------------------------------

