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
# include <SDL.h>
#endif

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "rle.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texture.h"
#include "texmerge.h"
#include "fbuffer.h"

#if DBG
CStaticArray< uint8_t, 65536 > usedHandles;

CStack< char* > texIds;
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#if 0

int32_t XGradient (tRGBA* image, int32_t x, int32_t y, int32_t w, int32_t h)
{
    return image.at<uchar>(y-1, x-1) +
                2*image.at<uchar>(y, x-1) +
                 image.at<uchar>(y+1, x-1) -
                  image.at<uchar>(y-1, x+1) -
                   2*image.at<uchar>(y, x+1) -
                    image.at<uchar>(y+1, x+1);
}

// Computes the y component of the gradient vector
// at a given point in a image
// returns gradient in the y direction

int32_t yGradient(Mat image, int32_t x, int32_t y, int32_t w, int32_t h)
{
    return image.at<uchar>(y-1, x-1) +
                2*image.at<uchar>(y-1, x) +
                 image.at<uchar>(y-1, x+1) -
                  image.at<uchar>(y+1, x-1) -
                   2*image.at<uchar>(y+1, x) -
                    image.at<uchar>(y+1, x+1);
}

int32_t SobelFilterRGBA (tRGBA *dest, tRGBA *src, int32_t w, int32_t h, int32_t tw, int32_t th)
{
for (int32_t y = 0; y < h; y++) {
   for (int32_t x = 0; x < w; x++) {
      gx = XGradient (src, x, y, w);
      gy = YGradient (src, x, y, h);
      sum = abs(gx) + abs(gy);
      sum = sum > 255 ? 255:sum;
      sum = sum < 0 ? 0 : sum;
      dst.at<uchar>(y,x) = sum;
		}
   }
}
		  
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#if DBG

static int32_t bWrapBlur = 1;
static int32_t nBlurStrength = 1;
static int32_t nColorSteps = 15;

#else

static int32_t bWrapBlur = 1;
static int32_t nBlurStrength = 3;
static int32_t nColorSteps = 15;

#endif

static inline int32_t Wrap (int32_t i, int32_t l)
{
if (i < 0)
	return i + l;
return i % l;
}

//------------------------------------------------------------------------------

static void HBoxBlurRGBAWrapped (tRGBA *dest, tRGBA *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
int32_t i = nStart * tw;
for (int32_t y = nStart; y < h; y += nStep) {
	int32_t l = 2 * r + 1;
	int32_t acc [3] = { 0, 0, 0 };
	int32_t hits = 0;

	for (int32_t x = -r; x < w + r; x++) {
		if (x > r) {
			int32_t j = Wrap (x - l, w);
			tRGBA& color = src [i + j];
			if (color.a && (color.r | color.g | color.b)) {
				acc [0] -= (int32_t) color.r;
				acc [1] -= (int32_t) color.g;
				acc [2] -= (int32_t) color.b;
				}
			hits--;
			}
 
		int32_t j = Wrap (x, w);
		tRGBA& color = src [i + j];
		if (color.a && (color.r | color.g | color.b)) {
			acc [0] += (int32_t) color.r;
			acc [1] += (int32_t) color.g;
			acc [2] += (int32_t) color.b;
			}
 
		if (x >= r) {
			tRGBA& color = dest [i + x - r];
			color.r = uint8_t (acc [0] / l);
			color.g = uint8_t (acc [1] / l);
			color.b = uint8_t (acc [2] / l);
			color.a = src [i + x - r].a;
			}
		}
	i += nStep * tw;
	}
}

//------------------------------------------------------------------------------

static void VBoxBlurRGBAWrapped (tRGBA *dest, tRGBA *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
	int32_t l = 2 * r + 1;

for (int32_t x = nStart; x < w; x += nStep) {
	int32_t acc [3] = { 0, 0, 0 };

	for (int32_t y = -r; y < h + r; y++) {
		if (y > r) {
			int32_t j = Wrap (y - l, h);
			tRGBA& color = src [j * tw + x];
			if (color.a && (color.r | color.g | color.b)) {
				acc [0] -= (int32_t) color.r;
				acc [1] -= (int32_t) color.g;
				acc [2] -= (int32_t) color.b;
				}
			}
 
		int32_t j = Wrap (y, h);
		tRGBA& color = src [j * tw + x];
		if (color.a && (color.r | color.g | color.b)) {
			acc [0] += (int32_t) color.r;
			acc [1] += (int32_t) color.g;
			acc [2] += (int32_t) color.b;
			}
 
		if (y >= r) {
			tRGBA& color = dest [(y - r) * tw + x];
			color.r = uint8_t (acc [0] / l);
			color.g = uint8_t (acc [1] / l);
			color.b = uint8_t (acc [2] / l);
			color.a = src [(y - r) * tw + x].a;
			}
		}
	}
}

//------------------------------------------------------------------------------

static void HBoxBlurRGBWrapped (tRGB *dest, tRGB *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
int32_t i = nStart * tw;
for (int32_t y = nStart; y < h; y += nStep) {
	int32_t l = 2 * r + 1;
	int32_t acc [3] = { 0, 0, 0 };
	for (int32_t x = -r; x < w + r; x++) {
		if (x > r) {
			int32_t j = Wrap (x - l, w);
			tRGB& color = src [i + j];
			acc [0] -= (int32_t) color.r;
			acc [1] -= (int32_t) color.g;
			acc [2] -= (int32_t) color.b;
			}
 
		int32_t j = Wrap (x, w);
		tRGB& color = src [i + j];
		acc [0] += (int32_t) color.r;
		acc [1] += (int32_t) color.g;
		acc [2] += (int32_t) color.b;
 
		if (x >= r) {
			tRGB& color = dest [i + x - r];
			color.r = uint8_t (acc [0] / l);
			color.g = uint8_t (acc [1] / l);
			color.b = uint8_t (acc [2] / l);
			}
		}
	i += nStep * tw;
	}
}

//------------------------------------------------------------------------------

static void VBoxBlurRGBWrapped (tRGB *dest, tRGB *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
	int32_t l = 2 * r + 1;

for (int32_t x = nStart; x < w; x += nStep) {
	int32_t acc [3] = { 0, 0, 0 };
	int32_t hits = 0;

	for (int32_t y = -r; y < h + r; y++) {
		if (y > r) {
			int32_t j = Wrap (y - l, h);
			tRGB& color = src [j * tw + x];
			acc [0] -= (int32_t) color.r;
			acc [1] -= (int32_t) color.g;
			acc [2] -= (int32_t) color.b;
			}
 
		int32_t j = Wrap (y, h);
		tRGB& color = src [j * tw + x];
		acc [0] += (int32_t) color.r;
		acc [1] += (int32_t) color.g;
		acc [2] += (int32_t) color.b;
 
		if (y >= r) {
			tRGB& color = dest [(y - r) * tw + x];
			color.r = uint8_t (acc [0] / l);
			color.g = uint8_t (acc [1] / l);
			color.b = uint8_t (acc [2] / l);
			}
		}
	}
}

//------------------------------------------------------------------------------

static void HBoxBlurRGBAClamped (tRGBA *dest, tRGBA *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
int32_t i = nStart * tw;
for (int32_t y = nStart; y < h; y += nStep) {
	int32_t hits = 0;
	int32_t acc [3] = { 0, 0, 0 };
	for (int32_t x = -r; x < w; x++) {
		int32_t j = x - r - 1;
		if (j >= 0) {
			tRGBA& color = src [i + j];
			if (color.a && (color.r | color.g | color.b)) {
				acc [0] -= (int32_t) color.r;
				acc [1] -= (int32_t) color.g;
				acc [2] -= (int32_t) color.b;
				}
			hits--;
			}
 
		j = x + r;
		if (j < w) {
			tRGBA& color = src [i + j];
			if (color.a && (color.r | color.g | color.b)) {
				acc [0] += (int32_t) color.r;
				acc [1] += (int32_t) color.g;
				acc [2] += (int32_t) color.b;
				}
			hits++;
			}
 
		if (x >= 0) {
			tRGBA& color = dest [i + x];
			color.r = uint8_t (acc [0] / hits);
			color.g = uint8_t (acc [1] / hits);
			color.b = uint8_t (acc [2] / hits);
			color.a = src [i + x].a;
			}
		}
	i += nStep * tw;
	}
}

//------------------------------------------------------------------------------

static void VBoxBlurRGBAClamped (tRGBA *dest, tRGBA *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
	int32_t o [2] = { -(r + 1) * tw, r * tw };

for (int32_t x = nStart; x < w; x += nStep) {
	int32_t hits = 0;
	int32_t acc [3] = { 0, 0, 0 };

	int32_t i = -r * tw + x;
	for (int32_t y = -r; y < h; y++) {
		int32_t j = y - r - 1;
		if (j >= 0) {
			tRGBA& color = src [i + o [0]];
			if (color.a && (color.r | color.g | color.b)) {
				acc [0] -= (int32_t) color.r;
				acc [1] -= (int32_t) color.g;
				acc [2] -= (int32_t) color.b;
				}
			hits--;
			}
 
		j = y + r;
		if (j < h) {
			tRGBA& color = src [i + o [1]];
			if (color.a && (color.r | color.g | color.b)) {
				acc [0] += (int32_t) color.r;
				acc [1] += (int32_t) color.g;
				acc [2] += (int32_t) color.b;
				}
			hits++;
			}
 
		if (y >= 0) {
			tRGBA& color = dest [y * tw + x];
			color.r = uint8_t (acc [0] / hits);
			color.g = uint8_t (acc [1] / hits);
			color.b = uint8_t (acc [2] / hits);
			color.a = src [y * tw + x].a;
			}
		i += nStep * tw;
		}
	}
}

//------------------------------------------------------------------------------

static void VBoxBlurRGBClamped (tRGB *dest, tRGB *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
	int32_t o [2] = { -(r + 1) * tw, r * tw };

for (int32_t x = nStart; x < w; x += nStep) {
	int32_t hits = 0;
	int32_t acc [3] = { 0, 0, 0 };

	int32_t i = -r * tw + x;
	for (int32_t y = -r; y < h; y++) {
		int32_t j = y - r - 1;
		if (j >= 0) {
			tRGB& color = src [i + o [0]];
			if (color.r | color.g | color.b) {
				acc [0] -= (int32_t) color.r;
				acc [1] -= (int32_t) color.g;
				acc [2] -= (int32_t) color.b;
				}
			hits--;
			}
 
		j = y + r;
		if (j < h) {
			tRGB& color = src [i + o [1]];
			if (color.r | color.g | color.b) {
				acc [0] += (int32_t) color.r;
				acc [1] += (int32_t) color.g;
				acc [2] += (int32_t) color.b;
				}
			hits++;
			}
 
		if (y >= 0) {
			tRGB& color = dest [y * tw + x];
			color.r = uint8_t (acc [0] / hits);
			color.g = uint8_t (acc [1] / hits);
			color.b = uint8_t (acc [2] / hits);
			}
		i += nStep * tw;
		}
	}
}

//------------------------------------------------------------------------------

static void HBoxBlurRGBClamped (tRGB *dest, tRGB *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
int32_t i = nStart * tw;
for (int32_t y = nStart; y < h; y += nStep) {
	int32_t hits = 0;
	int32_t acc [3] = { 0, 0, 0 };
	for (int32_t x = -r; x < w; x++) {
		int32_t j = x - r - 1;
		if (j >= 0) {
			tRGB& color = src [i + j];
			if (color.r | color.g | color.b) {
				acc [0] -= (int32_t) color.r;
				acc [1] -= (int32_t) color.g;
				acc [2] -= (int32_t) color.b;
				}
			hits--;
			}
 
		j = x + r;
		if (j < w) {
			tRGB& color = src [i + j];
			if (color.r | color.g | color.b) {
				acc [0] += (int32_t) color.r;
				acc [1] += (int32_t) color.g;
				acc [2] += (int32_t) color.b;
				}
			hits++;
			}
 
		if (x >= 0) {
			tRGB& color = dest [i + x];
			color.r = uint8_t (acc [0] / hits);
			color.g = uint8_t (acc [1] / hits);
			color.b = uint8_t (acc [2] / hits);
			}
		}
	i += nStep * tw;
	}
}

//------------------------------------------------------------------------------

void BoxBlurRGBA (tRGBA *dest, tRGBA *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t bWrap) 
{
#if USE_OPENMP
if (gameStates.app.bMultiThreaded) {
	if (bWrap) {
#	pragma omp parallel
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			HBoxBlurRGBAWrapped (src, dest, w, h, tw, th, r, i, gameStates.app.nThreads);
#	pragma omp parallel
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			VBoxBlurRGBAWrapped (dest, src, w, h, tw, th, r, i, gameStates.app.nThreads);
		}
	else {
#	pragma omp parallel
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			HBoxBlurRGBAClamped (src, dest, w, h, tw, th, r, i, gameStates.app.nThreads);
#	pragma omp parallel
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			VBoxBlurRGBAClamped (dest, src, w, h, tw, th, r, i, gameStates.app.nThreads);
		}
	}
else
#endif
if (bWrap) {
	HBoxBlurRGBAWrapped (src, dest, w, h, tw, th, r);
	VBoxBlurRGBAWrapped (dest, src, w, h, tw, th, r);
	}
else {
	HBoxBlurRGBAClamped (src, dest, w, h, tw, th, r);
	VBoxBlurRGBAClamped (dest, src, w, h, tw, th, r);
	}
}

//------------------------------------------------------------------------------

void BoxBlurRGB (tRGB *dest, tRGB *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t bWrap) 
{
#if USE_OPENMP
if (gameStates.app.bMultiThreaded) {
	if (bWrap) {
#	pragma omp parallel
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			HBoxBlurRGBWrapped (src, dest, w, h, tw, th, r, i, gameStates.app.nThreads);
#	pragma omp parallel
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			VBoxBlurRGBWrapped (dest, src, w, h, tw, th, r, i, gameStates.app.nThreads);
		}
	else {
#	pragma omp parallel
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			HBoxBlurRGBClamped (src, dest, w, h, tw, th, r, i, gameStates.app.nThreads);
#	pragma omp parallel
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			VBoxBlurRGBClamped (dest, src, w, h, tw, th, r, i, gameStates.app.nThreads);
		}
	}
else
#endif
if (bWrap) {
	HBoxBlurRGBWrapped (src, dest, w, h, tw, th, r);
	VBoxBlurRGBWrapped (dest, src, w, h, tw, th, r);
	}
else {
	HBoxBlurRGBClamped (src, dest, w, h, tw, th, r);
	VBoxBlurRGBClamped (dest, src, w, h, tw, th, r);
	}
}

//------------------------------------------------------------------------------

GLubyte *GaussianBlur (GLubyte *dest, GLubyte *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nColors, int32_t bWrap, int32_t nStrength = 1) 
{
r /= 2;
#if DBG
if (nColors < 3)
	return src;
#endif
for (; nStrength > 0; nStrength--) {
	if (nColors == 4) 
		BoxBlurRGBA ((tRGBA*) src, (tRGBA*) dest, w, h, tw, th, r, bWrap);
	else if (nColors == 3) 
		BoxBlurRGB ((tRGB*) src, (tRGB*) dest, w, h, tw, th, r, bWrap);
	}
return src;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

inline uint8_t Posterize (int32_t nColor, int32_t nSteps = 15) {
	return Max (0, ((nColor + nSteps / 2) / nSteps) * nSteps - nSteps);
	}

//------------------------------------------------------------------------------

void PosterizeRGBA (tRGBA *src, int32_t w, int32_t h, int32_t tw, int32_t nStart = 0, int32_t nStep = 1) 
{
for (int32_t y = nStart; y < h; y += nStep) {
	tRGBA *dest = src + y * tw;
	for (int32_t x = 0; x < w; x++) {
		dest->r = Posterize ((int32_t) dest->r);
		dest->g = Posterize ((int32_t) dest->g);
		dest->b = Posterize ((int32_t) dest->b);
		dest++;
		}
	}
}

//------------------------------------------------------------------------------

void PosterizeRGB (tRGB *src, int32_t w, int32_t h, int32_t tw, int32_t nStart = 0, int32_t nStep = 1) 
{
for (int32_t y = nStart; y < h; y += nStep) {
	tRGB *dest = src + y * tw;
	for (int32_t x = 0; x < w; x++) {
		dest->r = Posterize ((int32_t) dest->r);
		dest->g = Posterize ((int32_t) dest->g);
		dest->b = Posterize ((int32_t) dest->b);
		dest++;
		}
	}
}

//------------------------------------------------------------------------------

void Posterize (GLubyte *src, int32_t w, int32_t h, int32_t tw, int32_t nColors) 
{
#if 0 //USE_OPENMP
if (gameStates.app.bMultiThreaded) {
	if (nColors == 3) {
#	pragma omp parallel
		for (int32_t i = 0; i < gameStates.app.nThreads; i++)
			PosterizeRGB ((tRGB*) src, w, h, tw, i, gameStates.app.nThreads);
		}
	else if (nColors == 4) {
#	pragma omp parallel
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			PosterizeRGBA ((tRGBA*) src, w, h, tw, i, gameStates.app.nThreads);
		}
	}
else
#endif
if (nColors == 3)
	PosterizeRGB ((tRGB*) src, w, h, tw);
else if (nColors == 4)
	PosterizeRGBA ((tRGBA*) src, w, h, tw);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define TEXTURE_LIST_SIZE 5000

CTextureManager textureManager;

//------------------------------------------------------------------------------
//little hack to find the largest or equal multiple of 2 for a given number
int32_t Pow2ize (int32_t v, int32_t max)
{
	int32_t i;

for (i = 2; i <= max; i *= 2)
	if (v <= i)
		return i;
return i;
}

//------------------------------------------------------------------------------

int32_t Luminance (int32_t r, int32_t g, int32_t b)
{
	int32_t minColor, maxColor;

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
#if 1
m_textures.Create (1000);
m_textures.SetGrowth (1000);
#if DBG
texIds.Create (1000);
texIds.SetGrowth (1000);
#endif
#else
m_textures.Create (TEXTURE_LIST_SIZE);
for (int32_t i = 0; i < TEXTURE_LIST_SIZE; i++)
	m_textures [i].SetIndex (i);
#endif
#if DBG
usedHandles.Clear ();
#endif
}

//------------------------------------------------------------------------------

void TextureError (void)
{
PrintLog (0, "Error in texture management\n");
}

//------------------------------------------------------------------------------

void CTextureManager::Destroy (void)
{
#if DBG
Check ();
#endif
ogl.DestroyDrawBuffers ();
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
#if DBG
Check ();
#endif

for (uint32_t i = m_textures.ToS (); i > 0; ) {
	CTexture* texP = m_textures [--i];
#if DBG
	if (texP->Registered () != i + 1)
		TextureError ();
#endif
	texP->Destroy ();
#if DBG
	if (texIds [i])
		delete texIds [i];
#endif
	}
m_textures.Reset ();
#if DBG
texIds.Reset ();
#endif
}

//------------------------------------------------------------------------------

uint32_t CTextureManager::Check (CTexture* texP)
{
uint32_t i = texP->Registered ();
return i && (i <= m_textures.ToS ()) && (m_textures [i] != texP) ? i : 0;
}

//------------------------------------------------------------------------------

bool CTextureManager::Check (void)
{
#if DBG
for (uint32_t i = 0, j = m_textures.ToS (); i < j; i++) {
	CTexture* texP = m_textures [i]; 
	if (texP->Registered () != i + 1) {
		TextureError ();
		return false;
		}
	}
#endif
return true;
}

//------------------------------------------------------------------------------

uint32_t CTextureManager::Find (CTexture* texP)
{
for (uint32_t i = 0, j = m_textures.ToS (); i < j; i++) 
	if (m_textures [i] == texP)
		return i + 1;
return 0;
}

//------------------------------------------------------------------------------

uint32_t CTextureManager::Register (CTexture* texP)
{
uint32_t i = Find (texP);
if (i) {
#if DBG
	if (m_textures [i - 1]->Registered () != i)
		TextureError ();
#endif
	return i;
	}
m_textures.Push (texP);
#if DBG
int32_t l = (int32_t) strlen (texP->Bitmap ()->Name ()) + 1;
char* s = new char [l];
if (s)
	strcpy (s, texP->Bitmap ()->Name ());
texIds.Push (s);
s = NULL;
#endif
return m_textures.ToS ();
}

//------------------------------------------------------------------------------

bool CTextureManager::Release (CTexture* texP)
{
#if DBG
Check ();
#endif
uint32_t i = Check (texP);
if (!i)
	i = Find (texP);
if (!i)
	return false;

#if DBG
if (texIds [i - 1])
	delete texIds [i - 1];
#endif

if (i != m_textures.ToS ()) {
	m_textures [i - 1] = *m_textures.Top ();
	m_textures [i - 1]->Register (i);
#if DBG
	texIds [i - 1] = *texIds.Top ();
#endif
	}
#if DBG
*texIds.Top () = NULL;
texIds.Shrink ();
#endif

*m_textures.Top () = 0;
m_textures.Shrink ();
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CTexture::Init (void)
{
#if DBG
if (m_nRegistered)
	Destroy ();
#endif
m_info.handle = 0;
m_info.internalFormat = 4;
m_info.format = ogl.m_states.nRGBAFormat;
m_info.w =
m_info.h = 0;
m_info.tw =
m_info.th = 0;
m_info.lw = 0;
m_info.bMipMaps = 0;
m_info.bSmoothe = 0;
m_info.u =
m_info.v = 0;
m_info.bRenderBuffer = 0;
m_info.prio = 0.3f;
m_info.bmP = NULL;
m_nRegistered = 0;
}

//------------------------------------------------------------------------------

void CTexture::Setup (int32_t w, int32_t h, int32_t lw, int32_t bpp, int32_t bMask, int32_t bMipMap, int32_t bSmoothe, CBitmap *bmP)
{
m_info.w = w;
m_info.h = h;
m_info.lw = lw;
m_info.tw = Pow2ize (w);
m_info.th = Pow2ize (h);
#if DBG
if (m_info.tw == 2 * w)
	m_info.tw = Pow2ize (w);
if (m_info.th == 2 * h)
	m_info.th = Pow2ize (h);
#endif
if (bMask) {
	m_info.format = GL_RED;
	m_info.internalFormat = 1;
	}
else if (bpp == 3) {
	m_info.format = GL_RGB;
#if TEXTURE_COMPRESSION
	if ((w > 64) && (w == h))
		m_info.internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
	else
#endif
	m_info.internalFormat = 3;
	}
else {
	m_info.format = GL_RGBA;
#if TEXTURE_COMPRESSION
	if ((w > 64) && (w == h))
		m_info.internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	else
#endif
	m_info.internalFormat = 4;
	}
m_info.bMipMaps = (bMipMap > 0) && !bMask;
m_info.bSmoothe = (bMipMap < 0) ? -1 : bSmoothe || m_info.bMipMaps;
}

//------------------------------------------------------------------------------

bool CTexture::Register (uint32_t i)
{
if (i)
	m_nRegistered = i;
else {
	i = m_nRegistered;
	m_nRegistered = textureManager.Register (this);
	if (i)
		return false;
	}
return m_nRegistered != 0;
}

//------------------------------------------------------------------------------

void CTexture::Release (void)
{
if (m_info.handle && (m_info.handle != GLuint (-1))) {
#if DBG
	if (usedHandles [m_info.handle])
		usedHandles [m_info.handle] = 0;
	else if (!m_info.bRenderBuffer)
		TextureError ();
#endif
	if (!m_info.bRenderBuffer)
		ogl.DeleteTextures (1, reinterpret_cast<GLuint*> (&m_info.handle));
	m_info.handle = 0;
#if 1
	if (m_info.bmP)
		m_info.bmP->NeedSetup ();
#endif
	}
}

//------------------------------------------------------------------------------

void CTexture::Destroy (void)
{
Release ();
textureManager.Release (this);
m_nRegistered = 0;
//m_info.bmP = NULL;
Init ();
}

//------------------------------------------------------------------------------

bool CTexture::Check (void)
{
#if DBG
return textureManager.Check (this) != 0;
#endif
return true;
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
	m_info.handle = fbo->ColorBuffer ();
	m_info.bRenderBuffer = 1;
	}
}

#endif

//------------------------------------------------------------------------------

void CTexture::Bind (void) 
{ 
#if DBG
if (int32_t (m_info.handle) <= 0)
	return;
#endif
if (m_info.bRenderBuffer)
	BindRenderBuffer ();
else
	ogl.BindTexture (m_info.handle); 
}

//------------------------------------------------------------------------------

bool CTexture::IsBound (void) 
{ 
return m_info.handle && (m_info.handle == ogl.BoundTexture ()); 
}

//------------------------------------------------------------------------------

int32_t CTexture::BindRenderBuffer (void)
{
#if RENDER2TEXTURE == 1
#	if 1
ogl.BindTexture (m_info.pbuffer.texId);
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
ogl.BindTexture (m_info.pbuffer.texId);
#	endif
#	ifdef _WIN32
#		if DBG
if (!m_info.pbo.Bind ()) {
	char *psz = reinterpret_cast<char*> (gluErrorString (glGetError ())));
	return 1;
	}
#		else
if (!(m_info.pbo.Bind ())
	return 1;
#		endif
#	endif
#elif RENDER2TEXTURE == 2
ogl.BindTexture (m_info.fbo.ColorBuffer ());
#endif
return 0;
}

//------------------------------------------------------------------------------
// Create an image buffer for the renderer from a palettized bitmap.
// The image buffer has power of two dimensions; the image will be stored
// in its upper left area and u and v coordinates of lower right image corner
// will be computed so that only the part of the buffer containing the image
// is rendered.

uint8_t* CTexture::Convert (int32_t dxo, int32_t dyo, CBitmap* bmP, int32_t nTranspType, int32_t bSuperTransp, int32_t& bpp)
{
paletteManager.SetTexture (bmP->Parent () ? bmP->Parent ()->Palette () : bmP->Palette ());
if (!paletteManager.Texture ())
	return NULL;

int32_t bTransp = (nTranspType || bSuperTransp) && bmP->HasTransparency ();
if (!bTransp)
	m_info.format = GL_RGB;
#if DBG
if (!nTranspType)
	nTranspType = 0;
#endif

restart:

switch (m_info.format) {
	case GL_LUMINANCE:
		bpp = 1;
		break;

	case GL_LUMINANCE_ALPHA:
	case GL_RGB5:
	case GL_RGBA4:
		bpp = 2;
		break;

	case GL_RGB:
		bpp = 3;
		break;

	default:
		bpp = 4;
		break;
	}

if (m_info.tw * m_info.th * bpp > (int32_t) sizeof (ogl.m_data.buffer [0]))//shouldn'texP happen, descent never uses textures that big.
	Error ("texture too big %i %i", m_info.tw, m_info.th);

	uint8_t*		rawData = bmP->Buffer ();
	GLubyte*		bufP = ogl.m_data.buffer [0];
	CRGBColor*	colorP;
	int32_t		transparencyColor, superTranspColor;
#if 1
	if (bmP) {
		colorP = bmP->Palette ()->Color ();
		transparencyColor = bmP->Palette ()->TransparentColor ();
		superTranspColor =  bmP->Palette ()->SuperTranspColor ();
		}
	else
#endif
		{
		colorP = paletteManager.Texture ()->Color ();
		transparencyColor = paletteManager.Texture ()->TransparentColor ();
		superTranspColor =  paletteManager.Texture ()->SuperTranspColor ();
		}

	uint16_t		r, g, b, a;
	int32_t		x, y, c;
	int32_t		bPosterize = gameOpts->render.bCartoonStyle;


//bmP->Flags () &= ~(BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT);
for (y = 0; y < m_info.h; y++) {
	for (x = 0; x < m_info.w; x++) {
#if DBG
		if (!bmP->IsElement (rawData))
			break;
#endif
		c = *rawData++;
		if (nTranspType && (c == transparencyColor)) {
			//bmP->Flags () |= BM_FLAG_TRANSPARENT;
			switch (m_info.format) {
				case GL_LUMINANCE:
					(*(bufP++)) = 0;
					break;

				case GL_LUMINANCE_ALPHA:
					*reinterpret_cast<GLushort*> (bufP) = 0;
					bufP += 2;
					break;

				case GL_RGB:
				case GL_RGB5:
					m_info.format = ogl.m_states.nRGBAFormat;
					goto restart;
					break;

				case GL_RGBA:
					*reinterpret_cast<GLuint*> (bufP) = (nTranspType ? 0 : 0xffffffff);
					bufP += 4;
					break;

				case GL_RGBA4:
					*reinterpret_cast<GLushort*> ( bufP) = (nTranspType ? 0 : 0xffff);
					bufP += 2;
					break;
				}
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
					if (bSuperTransp && (c == superTranspColor)) {
						//bmP->Flags () |= BM_FLAG_SUPER_TRANSPARENT;
						m_info.format = ogl.m_states.nRGBAFormat;
						goto restart;
						}
					else {
						r = colorP [c].Red () * 4;
						g = colorP [c].Green () * 4;
						b = colorP [c].Blue () * 4;
						if (bPosterize) {
							r = (r / 16) * 16;
							g = (g / 16) * 16;
							b = (b / 16) * 16;
							}
						if ((r < ogl.m_states.nTransparencyLimit) &&
							 (g < ogl.m_states.nTransparencyLimit) &&
							 (b < ogl.m_states.nTransparencyLimit)) {
							m_info.format = ogl.m_states.nRGBAFormat;
							goto restart;
							}
						if (m_info.format == GL_RGB) {
							(*(bufP++)) = (GLubyte) r;
							(*(bufP++)) = (GLubyte) g;
							(*(bufP++)) = (GLubyte) b;
							}
						else {
							r >>= 3;
							g >>= 2;
							b >>= 3;
							*reinterpret_cast<GLushort*> ( bufP) = r + (g << 5) + (b << 11);
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
							vec = 1;
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
						r = colorP [c].Red () * 4;
						g = colorP [c].Green () * 4;
						b = colorP [c].Blue () * 4;
						if (bPosterize) {
							r = (r / 16) * 16;
							g = (g / 16) * 16;
							b = (b / 16) * 16;
							}
						if ((r < ogl.m_states.nTransparencyLimit) &&
							 (g < ogl.m_states.nTransparencyLimit) &&
							 (b < ogl.m_states.nTransparencyLimit))
							a = 0;
						else if (nTranspType == 1) {
#if 0 //non-linear formula
							double da = (double) (r * 3 + g * 5 + b * 2) / (10.0 * 255.0);
							da *= da;
							vec = (uint8_t) (da * 255.0);
#else
							a = (r * 30 + g * 59 + b * 11) / 100;	//transparency based on color intensity
#endif
							}
						else if (nTranspType == 2)	// black is transparent
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
						*reinterpret_cast<GLushort*> (bufP) = (r >> 4) + ((g >> 4) << 4) + ((b >> 4) << 8) + ((a >> 4) << 12);
						bufP += 2;
						}
					break;
					}
				}
			}
		}
#if DBG
	memset (bufP, 0, (m_info.tw - m_info.w) * bpp);	//erase the entire unused rightmost part of the line
#else
	memset (bufP, 0, bpp); //erase one pixel to compensate for rounding problems with the u coordinate
#endif
	bufP += (m_info.tw - m_info.w) * bpp;
	}
#if DBG
memset (bufP, 0, (m_info.th - m_info.h) * m_info.tw * bpp);
#else
memset (bufP, 0, m_info.tw * bpp);
#endif
if (m_info.format == GL_RGB)
	m_info.internalFormat = 3;
else if (m_info.format == GL_RGBA)
	m_info.internalFormat = 4;
else if ((m_info.format == GL_RGB5) || (m_info.format == GL_RGBA4))
	m_info.internalFormat = 2;
return ogl.m_data.buffer [0];
}

//------------------------------------------------------------------------------
//create texture buffer from data already in RGBA format

uint8_t *CTexture::Copy (int32_t dxo, int32_t dyo, uint8_t *data)
{
if ((gameOpts->render.bCartoonStyle >= 0) && !dxo && !dyo && (m_info.w == m_info.tw) && (m_info.h == m_info.th))
	return data;	//can use data 1:1
else {	//need to reformat
	int32_t	h, w, tw;
	GLubyte	*bufP;

	h = m_info.lw / m_info.w;
	w = (m_info.w - dxo) * h;
	data += m_info.lw * dyo + h * dxo;
	bufP = ogl.m_data.buffer [0];
	tw = m_info.tw * h;
	h = tw - w;
	for (; dyo < m_info.h; dyo++, data += m_info.lw) {
		memcpy (bufP, data, w);
		bufP += w;
		memset (bufP, 0, h);
		bufP += h;
		}
	memset (bufP, 0, m_info.th * tw - (bufP - ogl.m_data.buffer [0]));
	return ogl.m_data.buffer [0];
	}
}

//------------------------------------------------------------------------------

int32_t CTexture::FormatSupported (void)
{
	GLint nFormat = 0;

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
		if (ogl.m_states.bIntensity4 == -1)
			return 1;
		if (!ogl.m_states.bIntensity4)
			return 0;
		break;

	case GL_LUMINANCE4_ALPHA4:
		if (ogl.m_states.bLuminance4Alpha4 == -1)
			return 1;
		if (!ogl.m_states.bLuminance4Alpha4)
			return 0;
		break;
	}

glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_PROXY_TEXTURE_2D, 0, m_info.internalFormat, m_info.tw, m_info.th, 0,
				  m_info.format, GL_UNSIGNED_BYTE, ogl.m_data.buffer [0]);//NULL?
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
		ogl.m_states.bIntensity4 = (nFormat == m_info.internalFormat) ? -1 : 0;
		break;

	case GL_LUMINANCE4_ALPHA4:
		ogl.m_states.bLuminance4Alpha4 = (nFormat == m_info.internalFormat) ? -1 : 0;
		break;

	default:
		break;
	}
return nFormat == m_info.internalFormat;
}

//------------------------------------------------------------------------------

int32_t CTexture::Verify (void)
{
while (!FormatSupported ()) {
	switch (m_info.format) {
		case GL_INTENSITY4:
			if (ogl.m_states.bLuminance4Alpha4) {
				m_info.internalFormat = 2;
				m_info.format = GL_LUMINANCE_ALPHA;
				break;
				}

		case GL_LUMINANCE4_ALPHA4:
			m_info.internalFormat = 4;
			m_info.format = GL_RGBA;
			break;

		case GL_RGB:
			m_info.internalFormat = 3;
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
			console.printf (CON_DBG,"...no texP format to fall back on\n");
#endif
			return 1;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void CTexture::SetBufSize (int32_t dbits, int32_t bits, int32_t w, int32_t h)
{
if (bits <= 0) //the beta nvidia GLX server. doesn'texP ever return any bit sizes, so just use some assumptions.
	bits = dbits;
}

//------------------------------------------------------------------------------

void CTexture::SetSize (void)
{
	GLint	w, h;
	int32_t	nBits = 16, a = 0;
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

void CTexture::Wrap (int32_t state)
{
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state);
}

//------------------------------------------------------------------------------

GLuint CTexture::Create (int32_t w, int32_t h)
{
	int32_t		nSize = w * h * sizeof (uint32_t);
	uint8_t		*data = new uint8_t [nSize];

if (!data)
	return 0;
memset (data, 0, nSize);
ogl.GenTextures (1, &m_info.handle);
if (!m_info.handle) {
	ogl.ClearError (0);
	return 0;
	}
#if DBG
usedHandles [m_info.handle] = 1;
#endif
ogl.BindTexture  (m_info.handle);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, 4, w, h, 0, ogl.m_states.nRGBAFormat, GL_UNSIGNED_BYTE, data); 			// Build Texture Using Information In data
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
delete[] data;
return m_info.handle;
}

//------------------------------------------------------------------------------

int32_t CTexture::Prepare (bool bCompressed)
{
#if 0 //TEXTURE_COMPRESSION
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

int32_t CTexture::Compress (void)
{
if (m_info.internalFormat != GL_COMPRESSED_RGBA)
	return 0;

	GLint		nParam, nFormat, nSize;
	CBitmap	*bmP = m_info.bmP;

glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &nParam);
if (nParam) {
	glGetTexLevelParameteriv (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &nFormat);
	if ((nFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) ||
		 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ||
		 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) ||
		 (nFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)) {
		glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &nSize);
		if (bmP->CompressedBuffer ().Resize (nSize)) {
			glGetCompressedTexImage (GL_TEXTURE_2D, 0, reinterpret_cast<GLvoid*> (bmP->CompressedBuffer ().Buffer ()));
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
	m_info.format = GL_RGBA;
	m_info.internalFormat = 4;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

#if TEXTURE_COMPRESSION
int32_t CTexture::Load (uint8_t *buffer, int32_t nBufSize, int32_t nFormat, bool bCompressed)
#else
int32_t CTexture::Load (uint8_t* buffer)
#endif
{
if (!buffer)
	return 1;
ogl.GenTextures (1, reinterpret_cast<GLuint*> (&m_info.handle));
if (!m_info.handle) {
	ogl.ClearError (0);
	return 1;
	}
#if DBG
	if (!usedHandles [m_info.handle])
		usedHandles [m_info.handle] = 1;
	else
		TextureError ();
#endif
#if 0
m_info.prio = m_info.bMipMaps ? (m_info.h == m_info.w) ? 1.0f : 0.5f : 0.1f;
glPrioritizeTextures (1, (GLuint *) &m_info.handle, &m_info.prio);
#endif
Bind ();
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GLint (m_info.bMipMaps && ogl.m_states.bNeedMipMaps));
glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
if (m_info.bSmoothe > 0) {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ogl.m_states.texMagFilter);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ogl.m_states.texMinFilter);
	}
else if (m_info.bSmoothe < 0) {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	}
else {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
#if TEXTURE_COMPRESSION
if (bCompressed) {
	glCompressedTexImage2D (GL_TEXTURE_2D, 0, m_info.format, m_info.tw, m_info.th, 0, nBufSize, buffer);
	}
else
#endif
	{
	try {
		glTexImage2D (GL_TEXTURE_2D, 0, m_info.internalFormat, m_info.tw, m_info.th, 0, m_info.format, GL_UNSIGNED_BYTE, buffer);
	}
	catch (...) {
		Release ();
		}
#if 0
	try {
		if (ogl.m_states.bLowMemory && m_info.bMipMaps && (!m_info.bmP->Static () || (m_info.format == GL_RGB)))
			m_info.bmP->FreeData ();
		}
	catch (...) {
		}
#endif
#if TEXTURE_COMPRESSION
	Compress ();
#endif
	//SetSize ();
	}
return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#if 0

void CBitmap::UnlinkTexture (void)
{
if (Texture () && (Texture ()->Handle () == -1)) {
	Texture ()->SetHandle (0);
	SetTexture (NULL);
	}
}

//------------------------------------------------------------------------------

void CBitmap::Unlink (int32_t bAddon)
{
	CBitmap	*altBmP, *bmfP;
	int32_t			i, j;

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

#endif

//------------------------------------------------------------------------------

int32_t BitmapFrame (CBitmap* bmP, int16_t nTexture, int16_t nSegment, int32_t nFrame)
{
if (nSegment < 0)
	return nFrame;
if (nTexture != 361)
	return nFrame;
tObjectProducerInfo* p = gameData.producers.Find (nSegment);
if (!p)
	return nFrame;
if (gameData.producers.producers [p->nProducer].bEnabled)
	return nFrame;
return 3; // that's the number of the animation's darkest frame
}

//------------------------------------------------------------------------------

CBitmap *LoadFaceBitmap (int16_t nTexture, int16_t nFrameIdx, int32_t bLoadTextures)
{
	CBitmap*	bmP, * bmoP, * bmfP;
	int32_t	nFrames;

#if DBG
if (nTexture == nDbgTexture)
	BRP;
#endif
LoadTexture (gameData.pig.tex.bmIndexP [nTexture].index, 0, gameStates.app.bD1Mission);
bmP = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [nTexture].index;
bmP->SetStatic (1);
if (!(bmoP = bmP->Override ()))
	return bmP;
bmoP->SetStatic (1);
if (!bmoP->WallAnim ())
	return bmoP;
if (2 > (nFrames = bmoP->FrameCount ()))
	return bmoP;
bmoP->SetTranspType (3);
bmoP->SetupTexture (1, bLoadTextures);
if (!(bmfP = bmoP->Frames ()))
	return bmoP;
if ((nFrameIdx < 0) && (nFrames >= -nFrameIdx))
	bmfP -= (nFrameIdx + 1);
bmoP->SetCurFrame (bmfP);
bmfP->SetTranspType (3);
bmfP->SetupTexture (1, bLoadTextures);
bmfP->SetStatic (1);
return bmfP;
}

//------------------------------------------------------------------------------
//loads a palettized bitmap into a ogl RGBA texture.
//Sizes and pads dimensions to multiples of 2 if necessary.
//stores OpenGL textured id in *texid and u/v values required to get only the real data in *u/*v
int32_t CBitmap::LoadTexture (int32_t dxo, int32_t dyo, int32_t superTransp)
{
	uint8_t*		data = Buffer ();

if (!data)
	return 1;

	GLubyte*		bufP = NULL;
	CTexture		texture;
	bool			bLocal;
	int32_t		funcRes = 0;

if ((bLocal = (m_info.texP == NULL))) {
	texture.Setup (m_info.props.w, m_info.props.h, m_info.props.rowSize, m_info.nBPP);
	}
#if TEXTURE_COMPRESSION
m_info.texP->Prepare (m_info.compressed.bCompressed);
#	ifndef __macosx__
if (!(m_info.compressed.bCompressed || Parent ())) {
	if (ogl.m_features.bTextureCompression &&
		 ((m_info.texP->Format () == GL_RGBA) || (m_info.texP->Format () == GL_RGB)) &&
		 (m_info.texP->TW () >= 64) && (m_info.texP->TH () >= 64))
		m_info.texP->SetInternalFormat (GL_COMPRESSED_RGBA);
	if (m_info.texP->Verify ())
		return 1;
	}
#	endif
#else
m_info.texP->Prepare ();
#endif

#if DBG
if (strstr (m_info.szName, "ship"))
	BRP;
if (strstr (m_info.szName, "shield.tga"))
	BRP;
#endif
//	if (width!=twidth || height!=theight)
#if RENDER2TEXTURE
if (!m_info.texP->IsRenderBuffer ())
#endif
 {
	if (data) {
#if TEXTURE_COMPRESSION
		if (m_info.compressed.bCompressed)
			bufP = CompressedBuffer ().Buffer ();
		else
#endif
		int32_t nColors;
		if (m_info.nTranspType < 0) {
			bufP = m_info.texP->Copy (dxo, dyo, data);
			nColors = BPP ();
			}
		else {
			bufP = m_info.texP->Convert (dxo = 0, dyo = 0, this, m_info.nTranspType, superTransp, nColors);
			}
#if DBG
		if (strstr (m_info.szName, "String Bitmap"))
			BRP;
		if (strstr (m_info.szName, "rbot"))
			BRP;
#endif
		if (m_info.bCartoonize && (gameOpts->render.bCartoonStyle < 0)) {
			int32_t w = Width () - dxo;
			int32_t h = Height () - dxo;
			bufP = GaussianBlur (ogl.m_data.buffer [1], bufP, w, h, m_info.texP->TW (), m_info.texP->TH (), (w >= 512) ? 15 : (w >= 256) ? 14 : (w >= 128) ? 9 : 7, nColors, bWrapBlur, nBlurStrength);
			Posterize (bufP, w, h, m_info.texP->TW (), nColors);
			}
		}
#if TEXTURE_COMPRESSION
	m_info.texP->Load (bufP, m_info.compressed.buffer.Size (), m_info.compressed.nFormat, m_info.compressed.bCompressed);
#else
	funcRes = m_info.texP->Load (bufP);
#endif
	if (bLocal)
		m_info.texP->Destroy ();
	}
return funcRes;
}

//------------------------------------------------------------------------------

uint8_t decodebuf [2048*2048];

#if RENDER2TEXTURE == 1
int32_t CBitmap::PrepareTexture (int32_t bMipMap, int32_t bMask, CBO *renderBuffer)
#elif RENDER2TEXTURE == 2
int32_t CBitmap::PrepareTexture (int32_t bMipMap, int32_t bMask, CFBO *renderBuffer)
#else
int32_t CBitmap::PrepareTexture (int32_t bMipMap, int32_t bMask, tPixelBuffer *renderBuffer)
#endif
{
if ((m_info.nType == BM_TYPE_STD) && Parent () && (Parent () != this))
	return Parent ()->PrepareTexture (bMipMap, bMask, renderBuffer);

#if DBG
if ((nDbgTexture >= 0) && (m_info.nId == nDbgTexture))
	BRP;
if (strstr (m_info.szName, "shield.tga"))
	BRP;
#endif

if (!m_info.texP)
	m_info.texP = &m_info.texture;
#if DBG
if (m_info.texP->Bitmap () && (m_info.texP->Bitmap () != this))
	m_info.texP->SetBitmap (this);
else
#endif
//if (!m_info.texP->Bitmap ())
	m_info.texP->SetBitmap (this);
if (m_info.texP->Register ()) {
	m_info.texP->Setup (m_info.props.w, m_info.props.h, m_info.props.rowSize, m_info.nBPP, bMask, bMipMap, 0, this);
	m_info.texP->SetRenderBuffer (renderBuffer);
	}
else {
	if (m_info.texP->Handle () > 0)
		return 0;
	if (!m_info.texP->Width ())
		m_info.texP->Setup (m_info.props.w, m_info.props.h, m_info.props.rowSize, m_info.nBPP, bMask, bMipMap, 0, this);
	}
#if 0
if (Flags () & BM_FLAG_RLE)
	RLEExpand (NULL, 0);
if (!bMask) {
	CFloatVector3 color;
	if (0 <= (AvgColor (&color)))
		SetAvgColorIndex ((uint8_t) Palette ()->ClosestColor (&color));
	}
#endif
#if DBG
if (m_info.nId == nDbgTexture)
	BRP;
#endif
LoadTexture (0, 0, (m_info.props.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT)) != 0);
return m_info.texP->Handle () == 0;
}

//------------------------------------------------------------------------------

int32_t CBitmap::CreateFrames (int32_t bMipMaps, int32_t bLoad)
{
	int32_t	nFrames = (m_info.nType == BM_TYPE_ALT) ? m_info.frames.nCount : 0;
	uint8_t	nFlags;

if (nFrames < 2)
	return 0;
else {
#if DBG
	if (!strcmp (m_info.szName, "sparks.tga"))
		BRP;
	if ((nDbgTexture >= 0) && (m_info.nId == nDbgTexture))
		BRP;
#endif

	m_info.frames.bmP = new CBitmap [nFrames];

	int32_t	i, w = m_info.props.w;
	CBitmap* bmfP = m_info.frames.currentP = m_info.frames.bmP;

	m_info.frames.nCurrent = 0;
	for (i = 0; i < nFrames; i++, bmfP++) {
#if 0 //DBG
		bmfP->InitChild (this, 0, 0, w, w);
#else
		bmfP->InitChild (this, 0, i * w, w, w);
#endif
		bmfP->SetType (BM_TYPE_FRAME);
		bmfP->SetTop (0);
		nFlags = BM_FLAG_TGA;
		if (m_info.transparentFrames [i / 32] & (1 << (i % 32)))
			nFlags |= BM_FLAG_TRANSPARENT;
		if (m_info.supertranspFrames [i / 32] & (1 << (i % 32)))
			nFlags |= BM_FLAG_SUPER_TRANSPARENT;
		bmfP->SetFlags (nFlags);
		bmfP->SetTranspType (m_info.nTranspType);
		bmfP->SetRenderStyle (m_info.bCartoonize);
		bmfP->SetStatic (1);	// don't unload because this is just a child texture
		if (bLoad)
			bmfP->PrepareTexture (bMipMaps, 0, NULL);
		}
	}
return 1;
}

//------------------------------------------------------------------------------

CBitmap *CBitmap::CreateMask (void)
{
	int32_t		i = (int32_t) Width () * (int32_t) Height ();
	uint8_t		*pi;
	uint8_t		*pm;

if (!gameStates.render.textures.bHaveMaskShader)
	return NULL;
if (!Buffer ())
	return NULL;
if (m_info.maskP)
	return m_info.maskP;
//int32_t nTranspType = m_info.nTranspType;
//SetBPP (4);
if (!(m_info.maskP = CBitmap::Create (0, (Width ()  + 1) / 2, (Height () + 1) / 2, 4)))
	return NULL;
#if DBG
sprintf (m_info.maskP->m_info.szName, "{%s}", Name ());
#endif
m_info.maskP->SetWidth (m_info.props.w);
m_info.maskP->SetHeight (m_info.props.w);
m_info.maskP->SetBPP (1);
//m_info.nTranspType = nTranspType;
UseBitmapCache (m_info.maskP, (int32_t) m_info.maskP->Width () * (int32_t) m_info.maskP->RowSize ());
if (m_info.props.flags & BM_FLAG_TGA) {
	for (pi = Buffer (), pm = m_info.maskP->Buffer (); i; i--, pi += 4, pm++)
		if ((pi [0] == 120) && (pi [1] == 88) && (pi [2] == 128))
			*pm = 0;
		else
			*pm = 0xff;
	}
else {
	for (pi = Buffer (), pm = m_info.maskP->Buffer (); i; i--, pi++, pm++)
		if (*pi == SUPER_TRANSP_COLOR)
			*pm = 0;
		else
			*pm = 0xff;
	}
m_info.nMasks = 1;
return m_info.maskP;
}

//------------------------------------------------------------------------------

int32_t CBitmap::CreateMasks (void)
{
	int32_t	nMasks, i, nFrames;

if (!gameStates.render.textures.bHaveMaskShader)
	return 0;
if (m_info.nMasks)
	return m_info.nMasks;
m_info.nMasks = -1;
if ((m_info.nType != BM_TYPE_ALT) || !m_info.frames.bmP) {
	if (m_info.props.flags & BM_FLAG_SUPER_TRANSPARENT)
		return CreateMask () != NULL;
	return 0;
	}
nFrames = FrameCount ();
for (nMasks = i = 0; i < nFrames; i++)
	if (m_info.supertranspFrames [i / 32] & (1 << (i % 32)))
		if (m_info.frames.bmP [i].CreateMask ())
			nMasks++;
if (nMasks > 0)
	m_info.nMasks = nMasks;
return nMasks;
}

//------------------------------------------------------------------------------
// returns 0:Success, 1:Error

int32_t CBitmap::Bind (int32_t bMipMaps)
{
	CBitmap		*bmP;

	static int32_t nDepth = 0;

#if 0
if (nDepth > 1)
	return -1;
nDepth++;

if ((nDepth < 2) && (bmP = HasOverride ()) && (bmP != this)) {
	int32_t i = bmP->Bind (bMipMaps);
	nDepth--;
	return i;
	}

#else

if ((bmP = HasOverride ()) && (bmP != this)) {
	int32_t i = bmP->Bind (bMipMaps);
	return i;
	}

#endif

#if DBG
if (strstr (m_info.szName, "shield.tga"))
	BRP;
#endif

#if RENDER2TEXTURE
if (!(m_info.texP && m_info.texP->IsRenderBuffer ()))
#endif
	{
	if (!Prepared ()) {
		if (!SetupTexture (bMipMaps, 1)) {
#if DBG
			SetupTexture (bMipMaps, 1);
#endif
			nDepth--;
			return 1;
			}
		}
	CBitmap* maskP = Mask ();
	if (maskP && !maskP->Prepared ())
		maskP->SetupTexture (0, 1);
	}
if (!m_info.texP)
	return -1;
m_info.texP->Bind ();
#if 0
nDepth--;
#endif
return 0;
}

//------------------------------------------------------------------------------

bool CBitmap::SetupFrames (int32_t bMipMaps, int32_t bLoad)
{
int32_t h = m_info.props.h;
int32_t w = m_info.props.w;
if (!(h * w))
	return false;
int32_t nFrames = (m_info.nType == BM_TYPE_ALT) ? FrameCount () : 0;
if (!(m_info.props.flags & BM_FLAG_TGA) || (nFrames < 2)) {
	CreateMasks ();
	if (bLoad) {
		if (PrepareTexture (bMipMaps, 0, NULL))
			return false;
		if (Mask () && Mask ()->PrepareTexture (0, 1, NULL))
			return false;
		}
	}
else if (!Frames ()) {
	CreateFrames (bMipMaps, bLoad);
	CreateMasks ();
	if (bLoad) {
		CBitmap*	bmfP = Frames ();
		for (int32_t i = nFrames; i; i--, bmfP++) {
			if (bmfP->PrepareTexture (bMipMaps, 1))
				return false;
			if (bmfP->Mask () && (bmfP->Mask ()->PrepareTexture (0, 1, NULL)))
				return false;
			}
		}
	}
return (m_info.bSetup = true);
}

//------------------------------------------------------------------------------

bool CBitmap::SetupTexture (int32_t bMipMaps, int32_t bLoad)
{
	CBitmap *bmP;

#if DBG
if ((nDbgTexture >= 0) && (m_info.nId == nDbgTexture))
	BRP;
if (bMipMaps < 0)
	BRP;
if (strstr (m_info.szName, "shield.tga"))
	BRP;
#endif

switch (m_info.nType) {
	case BM_TYPE_STD: // primary (low res) texture
		if ((bmP = HasOverride ())) {
			if (bmP == this)
				m_info.overrideP = NULL;
			else
				return bmP->SetupTexture (bMipMaps, bLoad);
			}
		if (m_info.bSetup)
			return Prepared () || !PrepareTexture (bMipMaps, 0);
		return SetupFrames (bMipMaps, bLoad);

	case BM_TYPE_ALT:	// alternative (hires) textures
		if (!(m_info.bSetup || SetupFrames (bMipMaps, bLoad)))
			return false;
		if ((bmP = HasOverride ()))
			return bmP->SetupTexture (bMipMaps, bLoad);
		if (bLoad)
			return Prepared () || !PrepareTexture (bMipMaps, 0);
		return true;

	case BM_TYPE_FRAME:	// hires frame
		if (bLoad)
			return Prepared () || !PrepareTexture (bMipMaps, 0);
		return true;
	}
return false;
}

//------------------------------------------------------------------------------

#if DBG_OGL

void COGL::GenTextures (GLsizei n, GLuint *hTextures)
{
glGenTextures (n, hTextures);
#if 0
if ((*hTextures == DrawBuffer ()->ColorBuffer ()) &&
	 (hTextures != &DrawBuffer ()->ColorBuffer ()))
	DestroyDrawBuffers ();
#endif
}

//------------------------------------------------------------------------------

void COGL::DeleteTextures (GLsizei n, GLuint *hTextures)
{
#if 0
if ((*hTextures == DrawBuffer ()->ColorBuffer ()) &&
	 (hTextures != &DrawBuffer ()->ColorBuffer ()))
	DestroyDrawBuffers ();
#endif
#if DBG
for (int32_t i = 0; i < n;)
	if (int32_t (hTextures [i]) < 0)
		hTextures [i] = hTextures [--n];
	else
		i++;
if (n)
#endif
glDeleteTextures (n, hTextures);
}

#endif

//------------------------------------------------------------------------------

