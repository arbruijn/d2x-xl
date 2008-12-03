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

#define TEXTURE_LIST_SIZE 5000

CTextureManager textureManager;

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

void CTextureManager::Init (void)
{
m_textures.Create (TEXTURE_LIST_SIZE);
for (int i = 0; i < TEXTURE_LIST_SIZE; i++)
	m_textures [i].SetIndex (i);
}

//------------------------------------------------------------------------------

void CTextureManager::Destroy (void)
{
	CBitmap	*bmP;
	CTexture	*texP;
	int		i, j;

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

if (!(texP = GetFirst ()))
	return;
do {
	Push (texP);
	texP->SetHandle ((GLuint) -1);
	} while ((texP = GetNext ()));

for (i = 0; i < MAX_ADDON_BITMAP_FILES; i++)
	gameData.pig.tex.addonBitmaps [i].Unlink (1);

#if DBG
// Make sure all textures (bitmaps) from the game texture lists that had an OpenGL handle get the handle reset
for (i = 0; i < 2; i++) {
	bmP = gameData.pig.tex.bitmaps [i];
	for (j = gameData.pig.tex.nBitmaps [i] + (i ? 0 : gameData.hoard.nBitmaps); j; j--, bmP++)
		bmP->Unlink (0);
	}
#endif
}

//------------------------------------------------------------------------------

CTexture *CTextureManager::Get (CBitmap *bmP)
{
CTexture *texP = Pop ();

if (texP) 
	texP->SetBitmap (bmP);
else {
#if DBG
	Warning ("OGL: texture list full!\n");
#endif
	// try to recover: flush all textures, reload fonts and this level's textures
	RebuildRenderContext (gameStates.app.bGameRunning);
	if ((texP = Pop ()))
		texP->SetBitmap (bmP);
	}
return texP;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CTexture::Init (void)
{
m_info.handle = 0;
m_info.internalFormat = 4;
m_info.format = gameStates.ogl.nRGBAFormat;
m_info.w =
m_info.h = 0;
m_info.bRenderBuffer = 0;
m_info.bmP = NULL;
}

//------------------------------------------------------------------------------

void CTexture::Setup (int w, int h, int lw, int bpp, int bMask, int bMipMap, CBitmap *bmP)
{
m_info.w = w;
m_info.h = h;
m_info.lw = lw;
m_info.tw = Pow2ize (w);
m_info.th = Pow2ize (h);
if (bMask) {
	m_info.format = GL_RED;
	m_info.internalFormat = 1;
	}
else if (bpp == 3) {
	m_info.format = GL_RGB;
	m_info.internalFormat = 3;
	}
else {
	m_info.format = GL_RGBA;
	m_info.internalFormat = 4;
	}
m_info.bMipMaps = bMipMap && !bMask;
m_info.bmP = bmP;
}

//------------------------------------------------------------------------------

#if RENDER2TEXTURE == 1

void CTexture::SetRenderBuffer (CPBO* pbo)
{
if (!pbo)
	m_info.bRenderBuffer = 0;
else {
	m_info.pbo = *pbo;
	m_info.handle = pbo->TexId ();
	m_info.bRenderBuffer = 1;
	}
}

#elif RENDER2TEXTURE == 2

void CTexture::SetRenderBuffer (CFBO* fbo)
{
if (!fbo)
	m_info.bRenderBuffer = 0;
else {
	m_info.fbo = *fbo;
	m_info.handle = fbo->RenderBuffer ();
	m_info.bRenderBuffer = 1;
	}
}

#endif

//------------------------------------------------------------------------------

int CTexture::BindRenderBuffer (void)
{
#if RENDER2TEXTURE == 1
#	if 1
OGL_BINDTEX (m_info.pbuffer.texId);
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
OGL_BINDTEX (m_info.pbuffer.texId);
#	endif
#	ifdef _WIN32
#		if DBG
if (!m_info.pbo.Bind ()) {
	char *psz = (char *) gluErrorString (glGetError ());
	return 1;
	}
#		else
if (!(m_info.pbo.Bind ())
	return 1;
#		endif
#	endif
#elif RENDER2TEXTURE == 2
OGL_BINDTEX (m_info.fbo.RenderBuffer ());
#endif
return 0;
}

//------------------------------------------------------------------------------
//GLubyte gameData.render.ogl.buffer [512*512*4];

ubyte* CTexture::Convert (
//	GLubyte		*buffer,
//	int			width,
//	int			height,
	int			dxo,
	int			dyo,
//	int			tWidth,
//	int			tHeight,
//	int			nFormat,
	CBitmap		*bmP,
	int			nTransp,
	int			bSuperTransp)
{
	ubyte			*rawData = bmP->Buffer ();
	GLubyte		*bufP;
	tRgbColorb	*colorP;
	int			x, y, c, i;
	ushort		r, g, b, a;
	int			bTransp;

#if DBG
if (strstr (bmP->Name (), "phoenix"))
	bmP = bmP;
#endif
paletteManager.SetTexture (bmP->Parent () ? bmP->Parent ()->Palette () : bmP->Palette ());
if (!paletteManager.Texture ())
	return NULL;
if (m_info.tw * m_info.th * 4 > (int) sizeof (gameData.render.ogl.buffer))//shouldn'texP happen, descent never uses textures that big.
	Error ("texture too big %i %i", m_info.tw, m_info.th);
bTransp = (nTransp || bSuperTransp) && bmP->HasTransparency ();
if (!bTransp)
	m_info.format = GL_RGB;
#if DBG
if (!nTransp)
	nTransp = 0;
#endif
colorP = paletteManager.Texture ()->Color ();

restart:

i = 0;
bufP = gameData.render.ogl.buffer;
//bmP->Flags () &= ~(BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT);
for (y = 0; y < m_info.th; y++) {
	i = dxo + m_info.lw *(y + dyo);
	for (x = 0; x < m_info.tw; x++){
		if ((x < m_info.w) && (y < m_info.h))
			c = rawData [i++];
		else
			c = TRANSPARENCY_COLOR;	//fill the pad space with transparancy
		if (nTransp && ((int) c == TRANSPARENCY_COLOR)) {
			//bmP->Flags () |= BM_FLAG_TRANSPARENT;
			switch (m_info.format) {
				case GL_LUMINANCE:
					(*(bufP++)) = 0;
					break;

				case GL_LUMINANCE_ALPHA:
					*((GLushort *) bufP) = 0;
					bufP += 2; 
					break;

				case GL_RGB:
				case GL_RGB5:
					m_info.format = gameStates.ogl.nRGBAFormat;
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
			switch (m_info.format) {
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
						m_info.format = gameStates.ogl.nRGBAFormat;
						goto restart;
						}
					else {
						r = colorP [c].red * 4;
						g = colorP [c].green * 4;
						b = colorP [c].blue * 4;
						if (m_info.format == GL_RGB) {
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
						r = colorP [c].red * 4;
						g = colorP [c].green * 4;
						b = colorP [c].blue * 4;
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
					if (m_info.format == GL_RGBA) {
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
if (m_info.format == GL_RGB)
	m_info.internalFormat = 3;
else if (m_info.format == GL_RGBA)
	m_info.internalFormat = 4;
else if ((m_info.format == GL_RGB5) || (m_info.format == GL_RGBA4))
	m_info.internalFormat = 2;
return gameData.render.ogl.buffer;
}

//------------------------------------------------------------------------------
//create texture buffer from data already in RGBA format

ubyte *CTexture::Copy (int dxo, int dyo, ubyte *data)
{
if (!dxo && !dyo && (m_info.w == m_info.tw) && (m_info.h == m_info.th))
	return data;	//can use data 1:1
else {	//need to reformat
	int		h, w, tw;
	GLubyte	*bufP;

	h = m_info.lw / m_info.w;
	w = (m_info.w - dxo) * h;
	data += m_info.lw * dyo + h * dxo;
	bufP = gameData.render.ogl.buffer;
	tw = m_info.tw * h;
	h = tw - w;
	for (; dyo < m_info.h; dyo++, data += m_info.lw) {
		memcpy (bufP, data, w);
		bufP += w;
		memset (bufP, 0, h);
		bufP += h;
		}
	memset (bufP, 0, m_info.th * tw - (bufP - gameData.render.ogl.buffer));
	return gameData.render.ogl.buffer;
	}
}

//------------------------------------------------------------------------------

int CTexture::FormatSupported (void)
{
	GLint nFormat = 0;

if (!gameStates.ogl.bGetTexLevelParam)
	return 1;

switch (m_info.format) {
	case GL_RGB:
		if (m_info.internalFormat == 3)
			return 1;
		break;

	case GL_RGBA:
		if (m_info.internalFormat == 4)
			return 1;
		break;

	case GL_RGB5:
	case GL_RGBA4:
		if (m_info.internalFormat == 2)
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

glTexImage2D (GL_PROXY_TEXTURE_2D, 0, m_info.internalFormat, m_info.tw, m_info.th, 0,
				  m_info.format, GL_UNSIGNED_BYTE, gameData.render.ogl.buffer);//NULL?
glGetTexLevelParameteriv (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &nFormat);
switch (m_info.format) {
	case GL_RGBA:
#if TEXTURE_COMPRESSION
		if ((nFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ||
			 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) ||
			 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT))
			nFormat = m_info.internalFormat;
		else
#endif
			m_info.internalFormat = 4;
		break;

	case GL_RGB:
			m_info.internalFormat = 3;
		break;

	case GL_RGB5:
	case GL_RGBA4:
		m_info.internalFormat = 2;
		break;
	
	case GL_INTENSITY4:
		gameStates.ogl.bIntensity4 = (nFormat == m_info.internalFormat) ? -1 : 0;
		break;

	case GL_LUMINANCE4_ALPHA4:
		gameStates.ogl.bLuminance4Alpha4 = (nFormat == m_info.internalFormat) ? -1 : 0;
		break;

	default:
		break;
	}
return nFormat == m_info.internalFormat;
}

//------------------------------------------------------------------------------

int CTexture::Verify (void)
{
while (!FormatSupported ()) {
	switch (m_info.format) {
		case GL_INTENSITY4:
			if (gameStates.ogl.bLuminance4Alpha4) {
				m_info.internalFormat = 2;
				m_info.format = GL_LUMINANCE_ALPHA;
				break;
				}

		case GL_LUMINANCE4_ALPHA4:
			m_info.internalFormat = 4;
			m_info.format = GL_RGBA;
			break;

		case GL_RGB:
			m_info.internalFormat = 4;
			m_info.format = GL_RGBA;
			break;

		case GL_RGB5:
			m_info.internalFormat = 3;
			m_info.format = GL_RGB;
			break;
		
		case GL_RGBA4:
			m_info.internalFormat = 4;
			m_info.format = GL_RGBA;
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

void CTexture::SetBufSize (int dbits, int bits, int w, int h)
{
if (bits <= 0) //the beta nvidia GLX server. doesn'texP ever return any bit sizes, so just use some assumptions.
	bits = dbits;
}

//------------------------------------------------------------------------------

void CTexture::SetSize (void)
{
	GLint	w, h;
	int	nBits = 16, a = 0;

if (gameStates.ogl.bGetTexLevelParam) {
		GLint texP;

	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_LUMINANCE_SIZE, &texP);
	a += texP;
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTENSITY_SIZE, &texP);
	a += texP;
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &texP);
	a += texP;
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &texP);
	a += texP;
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_SIZE, &texP);
	a += texP;
	glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &texP);
	a += texP;
	}
else {
	w = m_info.tw;
	h = m_info.th;
	}
switch (m_info.format) {
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
SetBufSize (nBits, a, w, h);
}

//------------------------------------------------------------------------------

void CTexture::Unlink (void)
{
if (m_info.bmP && (m_info.bmP->Texture () == this))
	m_info.bmP->SetTexture (NULL);
}

//------------------------------------------------------------------------------

void CTexture::Destroy (void)
{
if (m_info.handle && (m_info.handle != (GLuint) -1)) {
	OglDeleteTextures (1, (GLuint *) &m_info.handle);
	Unlink ();
	Init ();
	}
}

//------------------------------------------------------------------------------
//Texture () MUST be bound first
void CTexture::Wrap (int state)
{
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state);
}

//------------------------------------------------------------------------------

GLuint CTexture::Create (int w, int h)			
{
	int		nSize = w * h * sizeof (unsigned int); 
	ubyte		*data = new ubyte [nSize]; 

if (!data)
	return 0;
memset (data, 0, nSize); 	
OglGenTextures (1, &m_info.handle); 
OGL_BINDTEX  (m_info.handle); 		
glTexImage2D (GL_TEXTURE_2D, 0, 4, w, h, 0, gameStates.ogl.nRGBAFormat, GL_UNSIGNED_BYTE, data); 			// Build Texture Using Information In data
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
delete[] data; 							
return m_info.handle; 						
}

//------------------------------------------------------------------------------

int CTexture::Prepare (bool bCompressed)
{
#if TEXTURE_COMPRESSION
if (bCompressed) {
	m_info.SetWidth (w);
	m_info.SetHeight (h);
	}
#endif
//calculate u/v values that would make the resulting texture correctly sized
m_info.u = (float) m_info.w / (float) m_info.tw;
m_info.v = (float) m_info.h / (float) m_info.th;
return 0;
}

//------------------------------------------------------------------------------

#if TEXTURE_COMPRESSION

int CTexture::Compress (void)
{
if (m_info.internalformat != GL_COMPRESSED_RGBA)
	return 0;

	GLint		nFormat, nParam;
	ubyte		*data;
	CBitmap	*bmP = m_info.bmP;

glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &nParam);
if (nParam) {
	glGetTexLevelParameteriv (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &nFormat);
	if ((nFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ||
		 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) ||
		 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)) {
		glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &nParam);
		if (nParam && (data = new ubyte [nParam])) {
			bmP->DestroyBuffer ();
			glGetCompressedTexImage (GL_TEXTURE_2D, 0, (GLvoid *) data);
			bmP->SetBuffer (data);
			bmP->SetBufSize (nParam);
			bmP->SetFormat (nFormat);
			bmP->SetCompressed (1);
			}
		}
	}
if (bmP->Compressed ())
	return 1;
if (bmP->BPP () == 3) {
	m_info.format = GL_RGB;
	m_info.internalFormat = 3;
	}
else {
	m_info.format = GL_RGB;
	m_info.internalFormat = 4;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

#if TEXTURE_COMPRESSION
int CTexture::Load (ubyte *buffer, int nBufSize, int nFormat, bool bCompressed)
#else
int CTexture::Load (ubyte *buffer)
#endif
{
if (!buffer)
	return 1;
// Generate OpenGL texture IDs.
OglGenTextures (1, (GLuint *) &m_info.handle);
if (!m_info.handle) {
#if DBG
	int nError = glGetError ();
#endif
	return 1;
	}
Bind ();
glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
if (m_info.bMipMaps) {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gameStates.ogl.texMagFilter);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gameStates.ogl.texMinFilter);
	}
else {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
#if TEXTURE_COMPRESSION
if (bCompressed) {
	glCompressedTexImage2D (
		GL_TEXTURE_2D, 0, nFormat,
		texP->tw, texP->th, 0, nBufSize, buffer);
	}
else 
#endif
	{
	if (m_info.bMipMaps && gameStates.ogl.bNeedMipMaps)
		gluBuild2DMipmaps (GL_TEXTURE_2D, m_info.internalFormat, m_info.tw, m_info.th, m_info.format, GL_UNSIGNED_BYTE, buffer);
	else
		glTexImage2D (GL_TEXTURE_2D, 0, m_info.internalFormat, m_info.tw, m_info.th, 0, m_info.format, GL_UNSIGNED_BYTE, buffer);
#if TEXTURE_COMPRESSION
	Compress ();
#endif
	SetSize ();
	}
return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CBitmap::UnlinkTexture (void)
{
if (Texture () && (Texture ()->Handle () == (GLuint) -1)) {
	Texture ()->SetHandle (0);
	SetTexture (NULL);
	}
}

//------------------------------------------------------------------------------

void CBitmap::Unlink (int bAddon)
{
	CBitmap	*altBmP, *bmfP;
	int			i, j;

if (bAddon || (Type () == BM_TYPE_STD)) {
	if (Mask ())
		Mask ()->UnlinkTexture ();
	if (!(bAddon || Override ()))
		UnlinkTexture ();
	else {
		altBmP = bAddon ? this : Override ();
		if (altBmP->Mask ())
			altBmP->Mask ()->UnlinkTexture ();
		altBmP->UnlinkTexture ();
		if ((altBmP->Type () == BM_TYPE_ALT) && altBmP->Frames ()) {
			i = altBmP->FrameCount ();
			if (i > 1) {
				for (j = i, bmfP = altBmP->Frames (); j; j--, bmfP++) {
					if (bmfP->Mask ())
						bmfP->Mask ()->UnlinkTexture ();
					bmfP->UnlinkTexture ();
					}
				}
			}
		}
	}
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
			bmP->SetupTexture (1, 3, 1);
			if (bmP->Frames ()) {
				if ((nFrameIdx >= 0) || (-nFrames > nFrameIdx))
					bmP->SetCurFrame (bmP->Frames ());
				else
					bmP->SetCurFrame (bmP->Frames () - nFrameIdx - 1);
				bmP->CurFrame ()->SetupTexture (1, 3, 1);
				}
			}
		}
	}
return bmP;
}

//------------------------------------------------------------------------------
//loads a palettized bitmap into a ogl RGBA texture.
//Sizes and pads dimensions to multiples of 2 if necessary.
//In theory this could be a problem for repeating textures, but all real
//textures (not sprites, etc) in descent are 64x64, so we are ok.
//stores OpenGL textured id in *texid and u/v values required to get only the real data in *u/*v
int CBitmap::LoadTexture (int dxo, int dyo, int nTransp, int superTransp)
{
	ubyte			*data = Buffer ();
	GLubyte		*bufP;
	CTexture		texture, *texP;
	bool			bLocal;
	int			funcRes = 1;

texP = Texture ();
if ((bLocal = (texP == NULL))) {
	texture.Setup (m_bm.props.w, m_bm.props.h, m_bm.props.rowSize, m_bm.nBPP);
	texP = &texture;
	}
#if TEXTURE_COMPRESSION
texP->Prepare (m_bm.bCompressed);
#	ifndef __macosx__
if (!(m_bm.bCompressed || superTransp || Parent ())) {
	if (gameStates.ogl.bTextureCompression && gameStates.ogl.bHaveTexCompression &&
		 ((texP->Format () == GL_RGBA) || (texP->Format () == GL_RGB)) && 
		 (TextureP->TW () >= 64) && (texP->TH () >= m_info.tw))
		texP->SetInternalformat (GL_COMPRESSED_RGBA);
	if (texP->Verify ())
		return 1;
	}
#	endif
#else
texP->Prepare ();
#endif

//	if (width!=twidth || height!=theight)
#if RENDER2TEXTURE
if (!texP->IsRenderBuffer ()) 
#endif
	{
#if TEXTURE_COMPRESSION
	if (data && !m_bm.bCompressed)
#else
	if (data) {
#endif
		if (nTransp < 0) 
			bufP = texP->Copy (dxo, dyo, data);
		else
			bufP = texP->Convert (dxo, dyo, this, nTransp, superTransp);
		}
#if TEXTURE_COMPRESSION
	texP->Load (Compressed () ? Buffer () : bufP, BufSize (), Format (), Compressed ());
#else
	funcRes = texP->Load (bufP);
#endif
	if (bLocal)
		texP->Destroy ();
	}
return 0;
}

//------------------------------------------------------------------------------

unsigned char decodebuf [2048*2048];

#if RENDER2TEXTURE == 1
int CBitmap::PrepareTexture (int bMipMap, int nTransp, int bMask, CBO *renderBuffer)
#elif RENDER2TEXTURE == 2
int CBitmap::PrepareTexture (int bMipMap, int nTransp, int bMask, CFBO *renderBuffer)
#else
int CBitmap::PrepareTexture (int bMipMap, int nTransp, int bMask, tPixelBuffer *renderBuffer)
#endif
{
	CTexture	*texP;

if ((m_bm.nType == BM_TYPE_STD) && Parent () && (Parent () != this))
	return Parent ()->PrepareTexture (bMipMap, nTransp, bMask, renderBuffer);

if (!(texP = Texture ())) {
	if (!(texP = textureManager.Get (this)))
		return 1;
	SetTexture (texP);
	texP->Setup (m_bm.props.w, m_bm.props.h, m_bm.props.rowSize, m_bm.nBPP, bMask, bMipMap, this);
	texP->SetRenderBuffer (renderBuffer);
	}
else {
	if (texP->Handle () > 0)
		return 0;
	if (!texP->Width ())
		texP->Setup (m_bm.props.w, m_bm.props.h, m_bm.props.rowSize, m_bm.nBPP, bMask, bMipMap, this);
	}
if (Flags () & BM_FLAG_RLE)
	RLEExpand (NULL, 0);
if (!bMask) {
	tRgbColorf color;
	if (0 <= (AvgColor (&color)))
		SetAvgColorIndex ((ubyte) Palette ()->ClosestColor (&color));
	}
LoadTexture (0, 0, (m_bm.props.flags & BM_FLAG_TGA) ? -1 : nTransp, 
				 (m_bm.props.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT)) != 0);
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
	CBitmap	*bmfP = new CBitmap [nFrames];
	int		i, w = m_bm.props.w;

	memset (bmfP, 0, nFrames * sizeof (CBitmap));
	m_bm.info.alt.frames = 
	m_bm.info.alt.curFrame = bmfP;
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
			bmfP->PrepareTexture (bDoMipMap, nTransp, 0, NULL);
		}
	}
return 1;
}

//------------------------------------------------------------------------------

CBitmap *CBitmap::CreateMask (void)
{
	int		i = (int) Width () * (int) Height ();
	ubyte		*pi;
	ubyte		*pm;

if (!gameStates.render.textures.bHaveMaskShader)
	return NULL;
if (m_bm.info.std.mask)
	return m_bm.info.std.mask;
SetBPP (4);
if (!(m_bm.info.std.mask = CBitmap::Create (0, (Width ()  + 1) / 2, (Height () + 1) / 2, 4)))
	return NULL;
#if DBG
sprintf (m_bm.szName, "{%s}", Name ());
#endif
m_bm.info.std.mask->SetWidth (m_bm.props.w);
m_bm.info.std.mask->SetHeight (m_bm.props.w);
m_bm.info.std.mask->SetBPP (1);
UseBitmapCache (m_bm.info.std.mask, (int) m_bm.info.std.mask->Width () * (int) m_bm.info.std.mask->RowSize ());
if (m_bm.props.flags & BM_FLAG_TGA) {
	for (pi = Buffer (), pm = m_bm.info.std.mask->Buffer (); i; i--, pi += 4, pm++)
		if ((pi [0] == 120) && (pi [1] == 88) && (pi [2] == 128))
			*pm = 0;
		else
			*pm = 0xff;
	}
else {
	for (pi = Buffer (), pm = m_bm.info.std.mask->Buffer (); i; i--, pi++, pm++)
		if (*pi == SUPER_TRANSP_COLOR)
			*pm = 0;
		else
			*pm = 0xff;
	}
return m_bm.info.std.mask;
}

//------------------------------------------------------------------------------

int CBitmap::CreateMasks (void)
{
	int	nMasks, i, nFrames;

if (!gameStates.render.textures.bHaveMaskShader)
	return 0;
if ((m_bm.nType != BM_TYPE_ALT) || !m_bm.info.alt.frames) {
	if (m_bm.props.flags & BM_FLAG_SUPER_TRANSPARENT)
		return CreateMask () != NULL;
	return 0;
	}
nFrames = FrameCount ();
for (nMasks = i = 0; i < nFrames; i++)
	if (m_bm.transparentFrames [i / 32] & (1 << (i % 32)))
		if (m_bm.info.alt.frames [i].CreateMask ())
			nMasks++;
return nMasks;
}

//------------------------------------------------------------------------------
// returns 0:Success, 1:Error

int CBitmap::Bind (int bMipMaps, int nTransp)
{
	CTexture		*texP = Texture ();
	CBitmap		*bmP, *mask;

if (bmP = HasOverride ())
	bmP->Bind (bMipMaps, nTransp);
#if RENDER2TEXTURE
if (texP->IsRenderBuffer ())
	texP->BindRenderBuffer ();
else
#endif
	{
	if (!Prepared ()) {
		if (!(bmP = SetupTexture (bMipMaps, nTransp, 1)))
			return 1;
		texP = bmP->Texture ();
#if 0
		if (!texP)
			bmP->SetupTexture (1, nTransp, 1);
#endif
		}
	if ((mask = Mask ()) && !mask->Prepared ())
		mask->SetupTexture (0, -1, 1);
	}
texP->Bind ();
return 0;
}

//------------------------------------------------------------------------------

CBitmap *CBitmap::SetupTexture (int bDoMipMap, int nTransp, int bLoad)
{
	CBitmap *bmP;

if ((bmP = HasOverride ()))
	bmP->PrepareTexture (bDoMipMap, nTransp, bLoad);

int	i, h, w, nFrames;

h = m_bm.props.h;
w = m_bm.props.w;
if (!(h * w))
	return NULL;
nFrames = (m_bm.nType == BM_TYPE_ALT) ? FrameCount () : 0;
if (!(m_bm.props.flags & BM_FLAG_TGA) || (nFrames < 2)) {
	if (bLoad && PrepareTexture (bDoMipMap, nTransp, 0, NULL))
		return NULL;
	if (CreateMasks () && Mask ()->PrepareTexture (0, -1, 1, NULL))
		return NULL;
	}
else if (!Frames ()) {
	CBitmap	*bmfP;

	SetupFrames (bDoMipMap, nTransp, bLoad);
	if ((i = bmP->CreateMasks ()))
		for (i = nFrames, bmfP = Frames (); i; i--, bmfP++)
			if (bLoad) {
				if (bmfP->PrepareTexture (bDoMipMap, nTransp, 1))
					return NULL;
				if (bmfP->Mask () && (bmfP->Mask ()->PrepareTexture (0, -1, 1, NULL)))
					return NULL;
				}
	}
return Override (-1);
}

//------------------------------------------------------------------------------

