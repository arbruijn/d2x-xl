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

#include "inferno.h"
#include "error.h"
#include "u_mem.h"
#include "rle.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texture.h"
#include "texmerge.h"

//------------------------------------------------------------------------------

tTextureInfo oglTextureList [OGL_TEXTURE_LIST_SIZE];
int oglTexListCur;

//------------------------------------------------------------------------------

inline void OglInitTextureStats (tTextureInfo* t)
{
t->prio = (float) 0.3;//default prio
t->lastrend = 0;
t->numrend = 0;
}

//------------------------------------------------------------------------------

void OglResetTextureStatsInternal (void)
{
	int			i;
	tTextureInfo	*t = oglTextureList;

for (i = OGL_TEXTURE_LIST_SIZE; i; i--, t++)
	if (t->handle)
		OglInitTextureStats (t);
}

//------------------------------------------------------------------------------

void OglInitTexture (tTextureInfo *t, int bMask, CBitmap *bmP)
{
t->handle = 0;
t->internalformat = bMask ? 1 : gameStates.ogl.bpp / 8;
t->format = bMask ? GL_RED : gameStates.ogl.nRGBAFormat;
t->wrapstate = -1;
t->w =
t->h = 0;
t->bFrameBuffer = 0;
t->bmP = bmP;
OglInitTextureStats (t);
}

//------------------------------------------------------------------------------

void OglInitTextureListInternal (void)
{
	tTextureInfo	*t = oglTextureList;
	
oglTexListCur = 0;
for (int i = OGL_TEXTURE_LIST_SIZE; i; i--, t++)
	OglInitTexture (t, 0, NULL);
}

//------------------------------------------------------------------------------

inline void UnlinkTexture (CBitmap *bmP)
{
if (bmP->texInfo && (bmP->texInfo->handle == (GLuint) -1)) {
	bmP->texInfo->handle = 0;
	bmP->texInfo = NULL;
	}
}

//------------------------------------------------------------------------------

void UnlinkBitmap (CBitmap *bmP, int bAddon)
{
	CBitmap	*altBmP, *bmfP;
	int			i, j;

if (bAddon || (bmP->nType == BM_TYPE_STD)) {
	if (bmP->Mask ())
		UnlinkTexture (bmP->Mask ());
	if (!(bAddon || bmP->Override ()))
		UnlinkTexture (bmP);
	else {
		altBmP = bAddon ? bmP : bmP->Override ();
		if (altBmP->Mask ())
			UnlinkTexture (altBmP->Mask ());
		UnlinkTexture (altBmP);
		if ((altBmP->Type () == BM_TYPE_ALT) && altBmP->Frames ()) {
			i = altBmP->FrameCount ();
			if (i > 1) {
				for (j = i, bmfP = altBmP->Frames (); j; j--, bmfP++) {
					if (bmfP->Mask ())
						UnlinkTexture (bmfP->Mask ());
					UnlinkTexture (bmfP);
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void OglSmashTextureListInternal (void)
{
	CBitmap			*bmP;
	tTextureInfo	*t;
	int				i, j, bUnlink = 0;

OglDestroyDrawBuffer ();
cameraManager.Destroy ();
OglDeleteLists (&hBigSphere, 1);
OglDeleteLists (&hSmallSphere, 1);
OglDeleteLists (&circleh5, 1);
OglDeleteLists (&circleh10, 1);
OglDeleteLists (cross_lh, sizeof (cross_lh) / sizeof (GLuint));
OglDeleteLists (primary_lh, sizeof (primary_lh) / sizeof (GLuint));
OglDeleteLists (secondary_lh, sizeof (secondary_lh) / sizeof (GLuint));
OglDeleteLists (g3InitTMU [0], sizeof (g3InitTMU) / sizeof (GLuint));
OglDeleteLists (g3ExitTMU, sizeof (g3ExitTMU) / sizeof (GLuint));
OglDeleteLists (&mouseIndList, 1);

for (i = OGL_TEXTURE_LIST_SIZE, t = oglTextureList; i; i--, t++) {
	if (!t->bFrameBuffer && t->handle && (t->handle != (GLuint) -1)) {
		OglDeleteTextures (1, (GLuint *) &t->handle);
		t->handle = (GLuint) -1;
		bUnlink = 1;
		}
#if DBG
	else if (t->handle > 0x7ffffff)
		t = t;
#endif
	t->w =
	t->h = 0;
	t->wrapstate = -1;
	}

// Make sure all textures (bitmaps) from the game texture lists that had an OpenGL handle get the handle reset
if (bUnlink) {
	for (i = 0; i < 2; i++) {
		bmP = gameData.pig.tex.bitmaps [i];
		for (j = gameData.pig.tex.nBitmaps [i] + (i ? 0 : gameData.hoard.nBitmaps); j; j--, bmP++)
			UnlinkBitmap (bmP, 0);
		}
	for (i = 0; i < MAX_ADDON_BITMAP_FILES; i++)
		UnlinkBitmap (gameData.pig.tex.addonBitmaps + i, 1);
	}
// Make sure all textures (bitmaps) not in the game texture lists that had an OpenGL handle get the handle reset
// (Can be fonts and other textures, e.g. those used for the menus)
for (i = OGL_TEXTURE_LIST_SIZE, t = oglTextureList; i; i--, t++)
	if (!t->bFrameBuffer && (t->handle == (GLuint) -1)) {
		if (t->bmP) {
			if (t->bmP->texInfo == t)
				t->bmP->texInfo = NULL;
			else
				t->bmP = NULL;	// this would mean the texture list is messed up
			}
		t->handle = 0;
		}
oglTexListCur = 0;
}

//------------------------------------------------------------------------------

tTextureInfo *OglGetFreeTextureInternal (void)
{
	int i;
	tTextureInfo *t = oglTextureList + oglTexListCur;

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

tTextureInfo *OglGetFreeTexture (CBitmap *bmP)
{
tTextureInfo *t = OglGetFreeTextureInternal ();
if (t)
	t->bmP = bmP;
else {
#if DBG
	Warning ("OGL: texture list full!\n");
#endif
	// try to recover: flush all textures, reload fonts and this level's textures
	RebuildRenderContext (gameStates.app.bGameRunning);
	if ((t = OglGetFreeTextureInternal ()))
		t->bmP = bmP;
	}
return t;
}

//------------------------------------------------------------------------------

int OglTextureStats (void)
{
	int used=0,databytes=0,truebytes=0,datatexel=0,truetexel=0,i;
	int prio0=0,prio1=0,prio2=0,prio3=0,prioh=0;
//	int grabbed=0;
	tTextureInfo* t;
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
#if 0
if (gr_renderstats && Gamefonts) {
	GrPrintF (5,GAME_FONT->ftHeight*14+3*14,"%i (%i,%i) %iK (%iK wasted)",used,usedrgba,usedl4a4,truebytes/1024, (truebytes-databytes)/1024);
	}
#endif
return truebytes;
}

//------------------------------------------------------------------------------

int OglBindTexImage (tTextureInfo *texP)
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
#		if DBG
if (!texP->pbo.Bind ()) {
	char *psz = (char *) gluErrorString (glGetError ());
	return 1;
	}
#		else
if (!(texP->pbo.Bind ())
	return 1;
#		endif
#	endif
#elif RENDER2TEXTURE == 2
OGL_BINDTEX (texP->fbo.RenderBuffer ());
#endif
return 0;
}

//------------------------------------------------------------------------------

int OglBindBmTex (CBitmap *bmP, int bMipMaps, int nTransp)
{
	tTextureInfo	*texP;
	CBitmap	*mask;
#if RENDER2TEXTURE
	int			bPBuffer;
#endif

if (!bmP)
	return 1;
bmP = bmP->Override (-1);
texP = bmP->texInfo;
#if RENDER2TEXTURE
if ((bPBuffer = texP && texP->bFrameBuffer)) {
	if (OglBindTexImage (texP))
		return 1;
	}
else
#endif
	{
	if (!(texP && (texP->handle > 0))) {
		if (OglLoadBmTexture (bmP, bMipMaps, nTransp, 1))
			return 1;
		bmP = bmP->Override (-1);
		texP = bmP->texInfo;
#if 1//def _DEBUG
		if (!texP)
			OglLoadBmTexture (bmP, 1, nTransp, 1);
#endif
		}
	OGL_BINDTEX (texP->handle);
	if ((mask = bmP->Mask ())) {
		texP = mask->TexInfo ();
		if (!(texP && (texP->handle > 0)))
			OglLoadBmTexture (mask, 0, -1, 1);
		}
	}
#if DBG
bmP->texInfo->lastrend = gameData.time.xGame;
bmP->texInfo->numrend++;
#endif
return 0;
}

//------------------------------------------------------------------------------

void OglResetFrames (CBitmap *bmP)
{
bmP->FreeTexture ();
}

//------------------------------------------------------------------------------

CBitmap *LoadFaceBitmap (short nTexture, short nFrameIdx)
{
	CBitmap	*bmP;
	int		nFrames;

PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [nTexture].index, gameStates.app.bD1Mission);
bmP = gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [nTexture].index;
if (bmP->Override ()) {
	bmP = bmP->Override ();
	if (bmP->WallAnim ()) {
		nFrames = bmP->FrameCount ();
		if (nFrames > 1) {
			OglLoadBmTexture (bmP, 1, 3, 1);
			if (bmP->Frames ()) {
				if ((nFrameIdx >= 0) || (-nFrames > nFrameIdx))
					bmP->SetCurFrame (bmP->Frames ());
				else
					bmP->SetCurFrame (bmP->Frames () - nFrameIdx - 1);
				OglLoadBmTexture (bmP->CurFrame (), 1, 3, 1);
				}
			}
		}
	}
return bmP;
}

//------------------------------------------------------------------------------
//little hack to find the largest or equal multiple of 2 for a given number
int Pow2ize (int x)
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
	CBitmap	*bmP,
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
	int			bSuperTransp)
{
//	GLushort *texP= (GLushort *)texBuf;
	ubyte			*data = bmP->texBuf;
	GLubyte		*bufP;
	tRgbColorb	*colorP;
	int			x, y, c, i, j;
	ushort		r, g, b, a;
	int			bTransp;

#if DBG
if (strstr (bmP->szName, "phoenix"))
	bmP = bmP;
#endif
paletteManager.SetTexture (bmP->Parent () ? bmP->Parent ()->Palette () : bmP->Palette ());
if (!paletteManager.Texture ()) {
	paletteManager.SetTexture (paletteManager.Default ());
	if (!paletteManager.Texture ())
		return nFormat;
	}
if (tWidth * tHeight * 4 > (int) sizeof (gameData.render.ogl.texBuf))//shouldn't happen, descent never uses textures that big.
	Error ("texture too big %i %i",tWidth, tHeight);
bTransp = (nTransp || bSuperTransp) && bmP->HasTransparency ();
if (!bTransp)
	nFormat = GL_RGB;
#if DBG
if (!nTransp)
	nTransp = 0;
#endif
colorP = paletteManager.Texture ()->Color ();
restart:

i = 0;
bufP = texBuf;
//bmP->Flags () &= ~(BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT);
for (y = 0; y < tHeight; y++) {
	i = dxo + truewidth *(y + dyo);
	for (x = 0; x < tWidth; x++){
		if ((x < width) && (y < height))
			c = data [i++];
		else
			c = TRANSPARENCY_COLOR;	//fill the pad space with transparancy
		if (nTransp && ((int) c == TRANSPARENCY_COLOR)) {
			//bmP->Flags () |= BM_FLAG_TRANSPARENT;
			switch (nFormat) {
				case GL_LUMINANCE:
					(*(bufP++)) = 0;
					break;

				case GL_LUMINANCE_ALPHA:
					*((GLushort *) bufP) = 0;
					bufP += 2; 
					break;

				case GL_RGB:
				case GL_RGB5:
					nFormat = gameStates.ogl.nRGBAFormat;
					goto restart;
					break;

				case GL_RGBA:
					*((GLuint *) bufP) = (nTransp ? 0 : 0xffffffff);
					bufP += 4;
					break;
				
				case GL_RGBA4:
					*((GLushort *) bufP) = (nTransp ? 0 : 0xffff);
					bufP += 2;
					break;
				}
//				 (*(texP++)) = 0;
			}
		else {
			switch (nFormat) {
				case GL_LUMINANCE://these could prolly be done to make the intensity based upon the intensity of the resulting color, but its not needed for anything (yet?) so no point. :)
						(*(bufP++)) = 255;
					break;

				case GL_LUMINANCE_ALPHA:
						(*(bufP++)) = 255;
						(*(bufP++)) = 255;
					break;

				case GL_RGB:
				case GL_RGB5:
					if (bSuperTransp && (c == SUPER_TRANSP_COLOR)) {
						//bmP->Flags () |= BM_FLAG_SUPER_TRANSPARENT;
						nFormat = gameStates.ogl.nRGBAFormat;
						goto restart;
						}
					else {
						r = colorP [j].red * 4;
						g = colorP [j].green * 4;
						b = colorP [j].blue * 4;
						if (nFormat == GL_RGB) {
							(*(bufP++)) = (GLubyte) r;
							(*(bufP++)) = (GLubyte) g;
							(*(bufP++)) = (GLubyte) b;
							}
						else {
							r >>= 3;
							g >>= 2;
							b >>= 3;
							*((GLushort *) bufP) = r + (g << 5) + (b << 11);
							bufP += 2;
							}
						}
					break;

				case GL_RGBA:
				case GL_RGBA4: {
					if (bSuperTransp && (c == SUPER_TRANSP_COLOR)) {
						//bmP->Flags () |= BM_FLAG_SUPER_TRANSPARENT;
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
						r = colorP [j].red * 4;
						g = colorP [j].green * 4;
						b = colorP [j].blue * 4;
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
						(*(bufP++)) = (GLubyte) r;
						(*(bufP++)) = (GLubyte) g;
						(*(bufP++)) = (GLubyte) b;
						(*(bufP++)) = (GLubyte) a;
						}
					else {
						*((GLushort *) bufP) = (r >> 4) + ((g >> 4) << 4) + ((b >> 4) << 8) + ((a >> 4) << 12);
						bufP += 2;
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

ubyte *OglCopyTexBuf (tTextureInfo *texP, int dxo, int dyo, ubyte *data)
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

int TexFormatSupported (tTextureInfo *texP)
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

int TexFormatVerify (tTextureInfo *texP)
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

void TexSetSizeSub (tTextureInfo *texP,int dbits,int bits,int w, int h)
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

void TexSetSize (tTextureInfo *texP)
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

	case GL_RED:
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

int OglCompressTexture (CBitmap *bmP, tTextureInfo *texP)
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
			D2_FREE (bmP->texBuf);
			glGetCompressedTexImage (GL_TEXTURE_2D, 0, (GLvoid *) data);
			bmP->texBuf = data;
			bmP->bmBufSize = nParam;
			bmP->bmFormat = nFormat;
			bmP->bmCompressed = 1;
			}
		}
	}
if (bmP->bmCompressed)
	return 1;
if (bmP->nBPP == 3) {
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
int OglLoadTexture (CBitmap *bmP, int dxo, int dyo, tTextureInfo *texP, int nTransp, int superTransp)
{
	ubyte				*data = bmP->TexBuf ();
	GLubyte			*bufP = gameData.render.ogl.texBuf;
	tTextureInfo	tex;
	int				bLocalTexture;

if (!bmP)
	return 1;
#if DBG
if (strstr (bmP->Name (), "phoenix"))
	bmP = bmP;
#endif
if (texP) {
	bLocalTexture = 0;
	//calculate smallest texture size that can accomodate us (must be multiples of 2)
#if TEXTURE_COMPRESSION
	if (bmP->bmCompressed) {
		texP->w = bmP->props.w;
		texP->h = bmP->props.h;
		}
#endif
	texP->tw = Pow2ize (texP->w);
	texP->th = Pow2ize (texP->h);
	}
else {
	bLocalTexture = 1;
	texP = &tex;
	tex.tw = Pow2ize (tex.w = bmP->Width ());
	tex.th = Pow2ize (tex.h = bmP->Height ());
	if (bmP->BPP () == 3) {
		texP->format = GL_RGB;
		texP->internalformat = 3;
		}
	else {
		texP->format = GL_RGB;
		texP->internalformat = 4;
		}
	tex.handle = 0;
	}
if (bmP && (bmP->BPP () == 3)) {
	texP->format = GL_RGB;
	texP->internalformat = 3;
	texP->lw = bmP->BPP () * texP->w;
	}

if (gr_badtexture > 0) 
	return 1;

#if TEXTURE_COMPRESSION
#	ifndef __macosx__
if (!(bmP->bmCompressed || superTransp || BM_PARENT (bmP))) {
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
if (!texP->bFrameBuffer) 
#endif
	{
#if TEXTURE_COMPRESSION
	if (data && !bmP->bmCompressed)
#else
	if (data) {
#endif
		if (nTransp < 0) 
			bufP = OglCopyTexBuf (texP, dxo, dyo, data);
		else {
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
		}
	// Generate OpenGL texture IDs.
	OglGenTextures (1, (GLuint *) &texP->handle);
	if (!texP->handle) {
#if DBG
		int nError = glGetError ();
#endif
		return 1;
		}
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
	if (bmP->bmCompressed) {
		glCompressedTexImage2D (
			GL_TEXTURE_2D, 0, bmP->bmFormat,
			texP->tw, texP->th, 0, bmP->bmBufSize, bmP->texBuf);
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
				texP->tw, texP->th, 0, texP->format, // RGB(A) textures.
				GL_UNSIGNED_BYTE, // imageData is a GLubyte pointer.
				bufP);
#if TEXTURE_COMPRESSION
		OglCompressTexture (bmP, texP);
#endif
		TexSetSize (texP);
		}
	if (bLocalTexture)
		OglDeleteTextures (1, (GLuint *) &texP->handle);
	}
r_texcount++; 
return 0;
}

//------------------------------------------------------------------------------

unsigned char decodebuf [2048*2048];

#if RENDER2TEXTURE == 1
int OglLoadBmTextureM (CBitmap *bmP, int bMipMap, int nTransp, int bMask, tPixelBuffer *pbo)
#elif RENDER2TEXTURE == 2
int OglLoadBmTextureM (CBitmap *bmP, int bMipMap, int nTransp, int bMask, CFBO *fbo)
#else
int OglLoadBmTextureM (CBitmap *bmP, int bMipMap, int nTransp, int bMask, CPBO *pbo)
#endif
{
	unsigned char	*buf;
	tTextureInfo		*t;
	CBitmap		*parent;

while ((bmP->Type () == BM_TYPE_STD) && (parent = bmP->Parent ()) && (parent != bmP))
	bmP = parent;
buf = bmP->TexBuf ();
if (!(t = bmP->TexInfo ())) {
	t = OglGetFreeTexture (bmP);
	if (!t)
		return 1;
	bmP->SetTexInfo (t);
	OglInitTexture (t, bMask, bmP);
	t->lw = bmP->Width ();
	if (bmP->Flags () & BM_FLAG_TGA)
		t->lw *= bmP->BPP ();
	t->w = bmP->Width ();
	t->h = bmP->Height ();
	t->bMipMaps = bMipMap && !bMask;
#if RENDER2TEXTURE == 1
	if (pbo) {
		t->pbo = *pbo;
		t->handle = pbo->TexId ();
		t->bFrameBuffer = 1;
		}
#elif RENDER2TEXTURE == 2
	if (fbo) {
		t->fbo = *fbo;
		t->handle = fbo->RenderBuffer ();
		t->bFrameBuffer = 1;
		}
#endif
	}
else {
	if (t->handle > 0)
		return 0;
	if (!t->w) {
		t->lw = bmP->Width ();
		t->w = bmP->Width ();
		t->h = bmP->Height ();
		}
	}
#if 1
if (bmP->Flags () & BM_FLAG_RLE)
	rle_expand (bmP, NULL, 0);
buf = bmP->TexBuf ();
#else
if (bmP->Flags () & BM_FLAG_RLE) {
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset, bBig = bmP->Flags () & BM_FLAG_RLE_BIG;

	data_offset = bBig ? 2 : 1;
	sbits = bmP->texBuf + 4 + (bmP->props.h * data_offset);
	dbits = decodebuf;
	for (i = 0; i < bmP->props.h; i++) {
		gr_rle_decode (sbits, dbits);
		if (bBig)
			sbits += (int) INTEL_SHORT (*((short *) (bmP->texBuf + 4 + (i * data_offset))));
		else
			sbits += (int) bmP->texBuf [4 + i];
		dbits += bmP->props.w;
		}
	buf = decodebuf;
	}
#endif
if (!bMask) {
	tRgbColorf	*c;

	c = AverageRGB (buf);
	if (!bmP->AverageColor ())
		bmP->SetAvgColor ((ubyte) bmP->Palette ()->ClosestColor ((int) c->red, (int) c->green, (int) c->blue));
	}
OglLoadTexture (bmP, 0, 0, bmP->TexInfo (), (bmP->Flags () & BM_FLAG_TGA) ? -1 : nTransp, 
					 (bmP->Flags () & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT)) != 0);
return 0;
}

//------------------------------------------------------------------------------

int CBitmap::SetupFrames (int bDoMipMap, int nTransp, int bLoad)
{
	int	nFrames = (m_bm.nType == BM_TYPE_ALT) ? m_bm.info.alt.nFrameCount : 0;
	ubyte	nFlags;

if (nFrames < 2)
	return 0;
else {
	CBitmap	*bmfP = (CBitmap *) D2_ALLOC (nFrames * sizeof (CBitmap));
	int		i, w = m_bm.props.w;

	memset (bmfP, 0, nFrames * sizeof (CBitmap));
	m_bm.info.alt.frames = bmfP;
	for (i = 0; i < nFrames; i++, bmfP++) {
		bmfP = CreateChild (0, i * w, w, w);
		bmfP->SetType (BM_TYPE_FRAME);
		bmfP->SetWidth (0);
		nFlags = bmfP->Flags ();
		if (m_bm.transparentFrames [i / 32] & (1 << (i % 32)))
			nFlags |= BM_FLAG_TRANSPARENT;
		if (m_bm.supertranspFrames [i / 32] & (1 << (i % 32)))
			nFlags |= BM_FLAG_SUPER_TRANSPARENT;
		if (nFlags & BM_FLAG_SEE_THRU)
			nFlags |= BM_FLAG_SEE_THRU;
		bmfP->SetFlags (nFlags);
		if (bLoad)
			OglLoadBmTextureM (bmfP, bDoMipMap, nTransp, 0, NULL);
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int OglLoadBmTexture (CBitmap *bmP, int bDoMipMap, int nTransp, int bLoad)
{
	int	i, h, w, nFrames;

bmP = bmP->Override (-1);
#if DBG
if (strstr (bmP->szName, "phoenix"))
	bmP = bmP;
#endif
h = bmP->props.h;
w = bmP->props.w;
if (!(h * w))
	return 1;
nFrames = (bmP->nType == BM_TYPE_ALT) ? bmP->FrameCount () : 0;
if (!(bmP->Flags () & BM_FLAG_TGA) || (nFrames < 2)) {
	if (bLoad && OglLoadBmTextureM (bmP, bDoMipMap, nTransp, 0, NULL))
		return 1;
	if (CreateSuperTranspMasks (bmP) && OglLoadBmTextureM (bmP->Mask (), 0, -1, 1, NULL))
		return 1;
	}
else if (!bmP->Frames ()) {
	CBitmap	*bmfP;

	OglSetupBmFrames (bmP, bDoMipMap, nTransp, bLoad);
	bmP->SetCurFrame (bmP->Frames ());
	if ((i = CreateSuperTranspMasks (bmP)))
		for (i = nFrames, bmfP = bmP->Frames (); i; i--, bmfP++)
			if (bLoad && bmfP->Mask ())
				OglLoadBmTextureM (bmfP->Mask (), 0, -1, 1, NULL);
	}
return 0;
}

//------------------------------------------------------------------------------

void OglFreeTexture (tTextureInfo *t)
{
if (t) {
	GLuint h = t->handle;
	if (h && (h != (GLuint) -1)) {
		r_texcount--;
		OglDeleteTextures (1, (GLuint *) &h);
		OglInitTexture (t, 0, NULL);
		}
	}
}

//------------------------------------------------------------------------------

void OglFreeBmTexture (CBitmap *bmP)
{
while ((bmP->Type () != BM_TYPE_ALT) && bmP->Parent () && (bmP != bmP->Parent ()))
	bmP = bmP->Parent ();
bmP->FreeTexture ();
}

//------------------------------------------------------------------------------
//TexInfo () MUST be bound first
void OglTexWrap (tTextureInfo *tex, int state)
{
#if 0
if (!tex || (tex->handle < 0))
	state = GL_CLAMP;
if (!tex || (tex->wrapstate != state) || (tex->numrend < 1)) {
#endif
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state);
#if 0
	if (tex)
		tex->wrapstate = state;
	}
#endif
}

//------------------------------------------------------------------------------

GLuint EmptyTexture (int Xsize, int Ysize)			// Create An Empty Texture
{
	GLuint texId; 						// Texture ID
	int nSize = Xsize * Ysize * 4 * sizeof (unsigned int); 
	// Create Storage Space For Texture Data (128x128x4)
	unsigned int *data = (unsigned int*) D2_ALLOC (nSize); 

memset (data, 0, nSize); 	// Clear Storage Memory
OglGenTextures (1, &texId); 					// Create 1 Texture
OGL_BINDTEX (texId); 			// Bind The Texture
glTexImage2D (GL_TEXTURE_2D, 0, 4, Xsize, Ysize, 0, gameStates.ogl.nRGBAFormat, GL_UNSIGNED_BYTE, data); 			// Build Texture Using Information In data
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
D2_FREE (data); 							// Release data
return texId; 						// Return The Texture ID
}

//------------------------------------------------------------------------------

