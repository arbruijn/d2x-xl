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

#define MAX_BLUR_RADIUS		15

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
	int32_t a [3] = { 0, 0, 0 };
	int32_t n = 0;

	for (int32_t x = -r; x < w + r; x++) {
		if (x > r) {
			tRGBA& color = src [i + Wrap (x - l, w)];
			if (color.a) {
				a [0] -= (int32_t) color.r;
				a [1] -= (int32_t) color.g;
				a [2] -= (int32_t) color.b;
				--n;
				}
			}
 
		tRGBA& color = src [i + Wrap (x, w)];
		if (color.a) {
			a [0] += (int32_t) color.r;
			a [1] += (int32_t) color.g;
			a [2] += (int32_t) color.b;
			++n;
			}
 
		if (x >= r) {
			tRGBA& color = dest [i + x - r];
			if (n) {
				color.r = uint8_t (a [0] / n);
				color.g = uint8_t (a [1] / n);
				color.b = uint8_t (a [2] / n);
				}
			else
				color.r = color.g = color.b = 0;
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
	int32_t a [3] = { 0, 0, 0 };
	int32_t n = 0;

	for (int32_t y = -r; y < h + r; y++) {
		if (y > r) {
			tRGBA& color = src [Wrap (y - l, h) * tw + x];
			if (color.a) {
				a [0] -= (int32_t) color.r;
				a [1] -= (int32_t) color.g;
				a [2] -= (int32_t) color.b;
				--n;
				}
			}
 
		tRGBA& color = src [Wrap (y, h) * tw + x];
		if (color.a) {
			a [0] += (int32_t) color.r;
			a [1] += (int32_t) color.g;
			a [2] += (int32_t) color.b;
			++n;
			}
 
		if (y >= r) {
			tRGBA& color = dest [(y - r) * tw + x];
			if (n) {
				color.r = uint8_t (a [0] / n);
				color.g = uint8_t (a [1] / n);
				color.b = uint8_t (a [2] / n);
				}
			else
				color.r = color.g = color.b = 0;
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
	int32_t a [3] = { 0, 0, 0 };

	for (int32_t x = -r; x < w + r; x++) {
		if (x > r) {
			tRGB& color = src [i + Wrap (x - l, w)];
			a [0] -= (int32_t) color.r;
			a [1] -= (int32_t) color.g;
			a [2] -= (int32_t) color.b;
			}
 
		tRGB& color = src [i + Wrap (x, w)];
		a [0] += (int32_t) color.r;
		a [1] += (int32_t) color.g;
		a [2] += (int32_t) color.b;
 
		if (x >= r) {
			tRGB& color = dest [i + x - r];
			color.r = uint8_t (a [0] / l);
			color.g = uint8_t (a [1] / l);
			color.b = uint8_t (a [2] / l);
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
	int32_t a [3] = { 0, 0, 0 };

	for (int32_t y = -r; y < h + r; y++) {
		if (y > r) {
			tRGB& color = src [Wrap (y - l, h) * tw + x];
			a [0] -= (int32_t) color.r;
			a [1] -= (int32_t) color.g;
			a [2] -= (int32_t) color.b;
			}
 
		tRGB& color = src [Wrap (y, h) * tw + x];
		a [0] += (int32_t) color.r;
		a [1] += (int32_t) color.g;
		a [2] += (int32_t) color.b;
 
		if (y >= r) {
			tRGB& color = dest [(y - r) * tw + x];
			color.r = uint8_t (a [0] / l);
			color.g = uint8_t (a [1] / l);
			color.b = uint8_t (a [2] / l);
			}
		}
	}
}

//------------------------------------------------------------------------------

static void HBoxBlurRGBAClamped (tRGBA *dest, tRGBA *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
int32_t i = nStart * tw;
for (int32_t y = nStart; y < h; y += nStep) {
	int32_t a [3] = { 0, 0, 0 };
	int32_t n = 0;

	for (int32_t x = -r; x < w; x++) {
		int32_t j = x - r - 1;
		if (j >= 0) {
			tRGBA& color = src [i + j];
			if (color.a) {
				a [0] -= (int32_t) color.r;
				a [1] -= (int32_t) color.g;
				a [2] -= (int32_t) color.b;
				--n;
				}
			}
 
		j = x + r;
		if (j < w) {
			tRGBA& color = src [i + j];
			if (color.a) {
				a [0] += (int32_t) color.r;
				a [1] += (int32_t) color.g;
				a [2] += (int32_t) color.b;
				++n;
				}
			}
 
		if (x >= 0) {
			tRGBA& color = dest [i + x];
			if (n) {
				color.r = uint8_t (a [0] / n);
				color.g = uint8_t (a [1] / n);
				color.b = uint8_t (a [2] / n);
				}
			else
				color.r = color.g = color.b = 0;
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
	int32_t a [3] = { 0, 0, 0 };
	int32_t n = 0;

	int32_t i = -r * tw + x;
	for (int32_t y = -r; y < h; y++) {
		int32_t j = y - r - 1;
		if (j >= 0) {
			tRGBA& color = src [i + o [0]];
			if (color.a) {
				a [0] -= (int32_t) color.r;
				a [1] -= (int32_t) color.g;
				a [2] -= (int32_t) color.b;
				--n;
				}
			}
 
		j = y + r;
		if (j < h) {
			tRGBA& color = src [i + o [1]];
			if (color.a) {
				a [0] += (int32_t) color.r;
				a [1] += (int32_t) color.g;
				a [2] += (int32_t) color.b;
				++n;
				}
			}
 
		if (y >= 0) {
			tRGBA& color = dest [y * tw + x];
			if (n) {
				color.r = uint8_t (a [0] / n);
				color.g = uint8_t (a [1] / n);
				color.b = uint8_t (a [2] / n);
				}	
			else
				color.r = color.g = color.b = 0;
			color.a = src [y * tw + x].a;
			}
		i += tw;
		}
	}
}

//------------------------------------------------------------------------------

static void VBoxBlurRGBClamped (tRGB *dest, tRGB *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
	int32_t o [2] = { -(r + 1) * tw, r * tw };

for (int32_t x = nStart; x < w; x += nStep) {
	int32_t n = 0;
	int32_t a [3] = { 0, 0, 0 };

	int32_t i = -r * tw + x;
	for (int32_t y = -r; y < h; y++) {
		int32_t j = y - r - 1;
		if (j >= 0) {
			tRGB& color = src [i + o [0]];
			a [0] -= (int32_t) color.r;
			a [1] -= (int32_t) color.g;
			a [2] -= (int32_t) color.b;
			n--;
			}
 
		j = y + r;
		if (j < h) {
			tRGB& color = src [i + o [1]];
			a [0] += (int32_t) color.r;
			a [1] += (int32_t) color.g;
			a [2] += (int32_t) color.b;
			n++;
			}
 
		if (y >= 0) {
			tRGB& color = dest [y * tw + x];
			color.r = uint8_t (a [0] / n);
			color.g = uint8_t (a [1] / n);
			color.b = uint8_t (a [2] / n);
			}
		i += tw;
		}
	}
}

//------------------------------------------------------------------------------

static void HBoxBlurRGBClamped (tRGB *dest, tRGB *src, int32_t w, int32_t h, int32_t tw, int32_t th, int32_t r, int32_t nStart = 0, int32_t nStep = 1)
{
int32_t i = nStart * tw;
for (int32_t y = nStart; y < h; y += nStep) {
	int32_t n = 0;
	int32_t a [3] = { 0, 0, 0 };
	for (int32_t x = -r; x < w; x++) {
		int32_t j = x - r - 1;
		if (j >= 0) {
			tRGB& color = src [i + j];
			a [0] -= (int32_t) color.r;
			a [1] -= (int32_t) color.g;
			a [2] -= (int32_t) color.b;
			n--;
			}
 
		j = x + r;
		if (j < w) {
			tRGB& color = src [i + j];
			a [0] += (int32_t) color.r;
			a [1] += (int32_t) color.g;
			a [2] += (int32_t) color.b;
			n++;
			}
 
		if (x >= 0) {
			tRGB& color = dest [i + x];
			color.r = uint8_t (a [0] / n);
			color.g = uint8_t (a [1] / n);
			color.b = uint8_t (a [2] / n);
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
#	pragma omp for
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			HBoxBlurRGBAWrapped (src, dest, w, h, tw, th, r, i, gameStates.app.nThreads);
#	pragma omp parallel
#	pragma omp for
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			VBoxBlurRGBAWrapped (dest, src, w, h, tw, th, r, i, gameStates.app.nThreads);
		}
	else {
#	pragma omp parallel
#	pragma omp for
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			HBoxBlurRGBAClamped (src, dest, w, h, tw, th, r, i, gameStates.app.nThreads);
#	pragma omp parallel
#	pragma omp for
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
#	pragma omp for
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			HBoxBlurRGBWrapped (src, dest, w, h, tw, th, r, i, gameStates.app.nThreads);
#	pragma omp parallel
#	pragma omp for
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			VBoxBlurRGBWrapped (dest, src, w, h, tw, th, r, i, gameStates.app.nThreads);
		}
	else {
#	pragma omp parallel
#	pragma omp for
		for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
			HBoxBlurRGBClamped (src, dest, w, h, tw, th, r, i, gameStates.app.nThreads);
#	pragma omp parallel
#	pragma omp for
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
r = Clamp (r, 3, MAX_BLUR_RADIUS) / 2;
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
#if USE_OPENMP
if (gameStates.app.bMultiThreaded) {
	if (nColors == 3) {
#	pragma omp parallel
#	pragma omp for
		for (int32_t i = 0; i < gameStates.app.nThreads; i++)
			PosterizeRGB ((tRGB*) src, w, h, tw, i, gameStates.app.nThreads);
		}
	else if (nColors == 4) {
#	pragma omp parallel
#	pragma omp for
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

void CreateOutline (tRGBA *pTexture, int32_t w, int32_t h, int32_t tw, int32_t nPasses, int32_t nStart = 0, int32_t nStep = 1)
{
	int32_t offsets [9] = { -tw - 1, -tw, -tw + 1, -1, 0, 1, tw - 1, tw, tw + 1};
	int32_t l = (h - 1) * tw + w;
	int32_t nTag = 254, n = 0;

for (int32_t nPass = 0; nPass < nPasses; nPass++, nTag--) {
	for (int32_t y = nStart; y < h; y += nStep) {
		int32_t i = y * tw;
		for (int32_t x = 0; x < w; x++, i++) {
			if (ogl.m_data.outlineFilter [i])
				continue;
			for (int32_t o = 0; o < 9; o++) {
				int32_t j = offsets [o];
				if (j == 0)
					continue;
				j += i;
				if (j < 0)
					continue;
				if (j >= l)
					continue;
				if (ogl.m_data.outlineFilter [j] > nTag) {
					ogl.m_data.outlineFilter [i] = nTag;
					pTexture [i].r = 
					pTexture [i].g = 
					pTexture [i].b = 2;
					pTexture [i].a = 255;
					n++;
					break;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void PrepareOutline (tRGBA *pTexture, int32_t w, int32_t h, int32_t tw, int32_t nStart = 0, int32_t nStep = 1)
{
for (int32_t y = nStart; y < h; y += nStep) {
	tRGBA *pSrc = pTexture + y * tw;
	uint8_t *pDest = ogl.m_data.outlineFilter + y * tw;
	for (int32_t x = 0; x < w; x++)
#if DBG
		{
		if (pSrc->a > 128)
			*pDest = 255;
		else
			*pDest = 0;
		pSrc++;
		pDest++;
		}
#else
		*(pDest++) = ((pSrc++)->a > 128) ? 255 : 0;
#endif
	}
}

//------------------------------------------------------------------------------

void Outline (GLubyte *src, int32_t w, int32_t h, int32_t tw, int32_t nPasses)
{
#if USE_OPENMP
if (gameStates.app.bMultiThreaded) {
#	pragma omp parallel
#	pragma omp for
	for (int32_t i = 0; i < gameStates.app.nThreads; i++)
		PrepareOutline ((tRGBA*) src, w, h, tw, i, gameStates.app.nThreads);
#	pragma omp for
	for (int32_t i = 0; i < gameStates.app.nThreads; i++)
		CreateOutline ((tRGBA*) src, w, h, tw, nPasses, i, gameStates.app.nThreads);
	}
else
#endif
	{
	PrepareOutline ((tRGBA*) src, w, h, tw);
	CreateOutline ((tRGBA*) src, w, h, tw, nPasses);
	}
}

//------------------------------------------------------------------------------

GLubyte *Cartoonize (CBitmap *pBm, GLubyte *pBuffer, int32_t dxo, int32_t dyo, int32_t nColors)
{
if (pBm->m_info.bCartoonizable && (gameStates.render.bCartoonize < 0)) {
	static int32_t blurRads [4][4] = {
		{ 15, 13, 11, 9 },
		{ 13, 11,  9, 7 },
		{ 11,  9,  7, 5 },
		{  9,  7,  5, 3 }
	};

#if DBG
	if (strstr (pBm->Name (), "shield"))
		BRP;
#endif

	int32_t w = pBm->Width () - dxo;
	int32_t h = pBm->Height () - dxo;
	int32_t tw = pBm->m_info.pTexture->TW ();
	int32_t s = (w >= 512) ? 3 : (w >= 256) ? 2 : (w >= 128) ? 1 : 0;
	if (gameStates.render.bBlurTextures)
#if 0
		pBuffer = GaussianBlur (ogl.m_data.buffer [1], pBuffer, w, h, tw, m_info.pTexture->TH (), blurRads [0][s], nColors, !gameStates.render.bClampBlur, 1);
#elif 1
		pBuffer = GaussianBlur (ogl.m_data.buffer [1], pBuffer, w, h, tw, pBm->m_info.pTexture->TH (), blurRads [-gameStates.render.bCartoonize - 1][s], 
										nColors, !gameStates.render.bClampBlur, gameStates.render.bBlurTextures);
#endif
	if (gameStates.render.bPosterizeTextures)
		Posterize (pBuffer, w, h, tw, nColors);
	if (gameStates.render.bOutlineTextures && (nColors == 4) && !strstr (pBm->Name (), "lava") && !strstr (pBm->Name (), "water"))
		Outline (pBuffer, w, h, tw, 1 << s);
	}
return pBuffer;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
