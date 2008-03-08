#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"
#include "u_mem.h"
#include "gr.h"
#include "tga.h"

//------------------------------------------------------------------------------

int CompressTGA (grsBitmap *bmP)
{
if (!(gameStates.ogl.bTextureCompression && gameStates.ogl.bHaveTexCompression))
	return 0;
if (bmP->bmProps.h / bmP->bmProps.w > 1)
	return 0;	//don't compress animations
if (bmP->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
	return 0;	//don't compress textures containing some form of transparency
if (OglLoadTexture (bmP, 0, 0, NULL, -1, 0))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int ReadTGAImage (CFILE *cfp, tTgaHeader *ph, grsBitmap *bmP, int alpha, 
						double brightness, int bGrayScale, int bReverse)
{
	int				i, j, n, nAlpha = 0, nVisible = 0, nFrames, nBytes = ph->bits / 8;
	int				h = bmP->bmProps.h;
	int				w = bmP->bmProps.w;
	tRgbColorf		avgColor;
	float				a, avgAlpha = 0;

if (!(bmP->bmTexBuf || (bmP->bmTexBuf = D2_ALLOC (ph->height * w * nBytes))))
	 return 0;
if (!bmP->bmTexBuf) {
	int nSize = ph->width * ph->height * nBytes;
	if (!(bmP->bmTexBuf = D2_ALLOC (nSize)))
		return 0;
	}
bmP->bmBPP = nBytes;
memset (bmP->bmTransparentFrames, 0, sizeof (bmP->bmTransparentFrames));
memset (bmP->bmSupertranspFrames, 0, sizeof (bmP->bmSupertranspFrames));
avgColor.red = avgColor.green = avgColor.blue = 0;
if (ph->bits == 24) {
	tBGRA	c;
	tRgbColorb *p = ((tRgbColorb *) (bmP->bmTexBuf)) + w * (bmP->bmProps.h - 1);

	for (i = bmP->bmProps.h; i; i--) {
		for (j = w; j; j--, p++) {
			if (CFRead (&c, 1, 3, cfp) != (size_t) 3)
				return 0;
			if (bGrayScale) {
				p->red =
				p->green =
				p->blue = (ubyte) (((int) c.r + (int) c.g + (int) c.b) / 3 * brightness);
				}
			else {
				p->red = (ubyte) (c.r * brightness);
				p->green = (ubyte) (c.g * brightness);
				p->blue = (ubyte) (c.b * brightness);
				}
			avgColor.red += p->red;
			avgColor.green += p->green;
			avgColor.blue += p->blue;
			//p->alpha = (alpha < 0) ? 255 : alpha;
			}
		p -= 2 * w;
		nVisible = w * h * 255;
		}
	}
else {
	bmP->bmProps.flags |= BM_FLAG_SEE_THRU;
	if (bReverse) {
		tRGBA	c;
		tRgbaColorb	*p = (tRgbaColorb *) bmP->bmTexBuf;
		int nSuperTransp;

		nFrames = h / w - 1;
		for (i = 0; i < h; i++) {
			n = nFrames - i / w;
			nSuperTransp = 0;
			for (j = w; j; j--, p++) {
				if (CFRead (&c, 1, 4, cfp) != (size_t) 4)
					return 0;
				if (bGrayScale) {
					p->red =
					p->green =
					p->blue = (ubyte) (((int) c.r + (int) c.g + (int) c.b) / 3 * brightness);
					}
				else if (c.a) {
					p->red = (ubyte) (c.r * brightness);
					p->green = (ubyte) (c.g * brightness);
					p->blue = (ubyte) (c.b * brightness);
					}
				if ((c.r == 120) && (c.g == 88) && (c.b == 128)) {
					nSuperTransp++;
					p->alpha = 0;
					}
				else {
					p->alpha = (alpha < 0) ? c.a : alpha;
					if (!p->alpha)
						p->red =		//avoid colored transparent areas interfering with visible image edges
						p->green =
						p->blue = 0;
					}
				if (p->alpha < 196) {
					if (!n) {
						bmP->bmProps.flags |= BM_FLAG_TRANSPARENT;
						if (p->alpha)
							bmP->bmProps.flags &= ~BM_FLAG_SEE_THRU;
						}
					if (bmP)
						bmP->bmTransparentFrames [n / 32] |= (1 << (n % 32));
					avgAlpha += p->alpha;
					nAlpha++;
					}
				nVisible += p->alpha;
				a = (float) p->alpha / 255;
				avgColor.red += p->red * a;
				avgColor.green += p->green * a;
				avgColor.blue += p->blue * a;
				}
			if (nSuperTransp > 50) {
				if (!n)
					bmP->bmProps.flags |= BM_FLAG_SUPER_TRANSPARENT;
				bmP->bmProps.flags |= BM_FLAG_SEE_THRU;
				bmP->bmSupertranspFrames [n / 32] |= (1 << (n % 32));
				}
			}
		}
	else {
		tBGRA	c;
		tRgbaColorb *p = ((tRgbaColorb *) (bmP->bmTexBuf)) + w * (bmP->bmProps.h - 1);
		int nSuperTransp;

		nFrames = h / w - 1;
		for (i = 0; i < h; i++) {
			n = nFrames - i / w;
			nSuperTransp = 0;
			for (j = w; j; j--, p++) {
				if (CFRead (&c, 1, 4, cfp) != (size_t) 4)
					return 0;
				if (bGrayScale) {
					p->red =
					p->green =
					p->blue = (ubyte) (((int) c.r + (int) c.g + (int) c.b) / 3 * brightness);
					}
				else {
					p->red = (ubyte) (c.r * brightness);
					p->green = (ubyte) (c.g * brightness);
					p->blue = (ubyte) (c.b * brightness);
					}
				if ((c.r == 120) && (c.g == 88) && (c.b == 128)) {
					nSuperTransp++;
					p->alpha = 0;
					}
				else {
					p->alpha = (alpha < 0) ? c.a : alpha;
					if (!p->alpha)
						p->red =		//avoid colored transparent areas interfering with visible image edges
						p->green =
						p->blue = 0;
					}
				if (p->alpha < 196) {
					if (!n) {
						bmP->bmProps.flags |= BM_FLAG_TRANSPARENT;
						if (p->alpha)
							bmP->bmProps.flags &= ~BM_FLAG_SEE_THRU;
						}
					bmP->bmTransparentFrames [n / 32] |= (1 << (n % 32));
					avgAlpha += p->alpha;
					nAlpha++;
					}
				nVisible += p->alpha;
				a = (float) p->alpha / 255;
				avgColor.red += p->red * a;
				avgColor.green += p->green * a;
				avgColor.blue += p->blue * a;
				}
			if (nSuperTransp > w * w / 2000) {
				if (!n)
					bmP->bmProps.flags |= BM_FLAG_SUPER_TRANSPARENT;
				bmP->bmProps.flags |= BM_FLAG_SEE_THRU;
				bmP->bmSupertranspFrames [n / 32] |= (1 << (n % 32));
				}
			p -= 2 * w;
			}
		}
	}
a = (float) nVisible / 255.0f;
bmP->bmAvgRGB.red = (ubyte) (avgColor.red / a);
bmP->bmAvgRGB.green = (ubyte) (avgColor.green / a);
bmP->bmAvgRGB.blue = (ubyte) (avgColor.blue / a);
if (!nAlpha)
	bmP->bmProps.flags &= ~BM_FLAG_SEE_THRU;
#if 0
if (nAlpha && ((ubyte) (avgAlpha / nAlpha) < 2))
	bmP->bmProps.flags |= BM_FLAG_SEE_THRU;
#endif
return 1;
}

//	-----------------------------------------------------------------------------

int WriteTGAImage (CFILE *cfp, tTgaHeader *ph, grsBitmap *bmP)
{
	int				i, j, n, nFrames;
	int				h = bmP->bmProps.h;
	int				w = bmP->bmProps.w;

if (ph->bits == 24) {
	if (bmP->bmBPP == 3) {
		tBGR	c;
		tRgbColorb *p = ((tRgbColorb *) (bmP->bmTexBuf)) + w * (bmP->bmProps.h - 1);
		for (i = bmP->bmProps.h; i; i--) {
			for (j = w; j; j--, p++) {
				c.r = p->red;
				c.g = p->green;
				c.b = p->blue;
				if (CFWrite (&c, 1, 3, cfp) != (size_t) 3)
					return 0;
				}
			p -= 2 * w;
			}
		}
	else {
		tBGR	c;
		tRgbaColorb *p = ((tRgbaColorb *) (bmP->bmTexBuf)) + w * (bmP->bmProps.h - 1);
		for (i = bmP->bmProps.h; i; i--) {
			for (j = w; j; j--, p++) {
				c.r = p->red;
				c.g = p->green;
				c.b = p->blue;
				if (CFWrite (&c, 1, 3, cfp) != (size_t) 3)
					return 0;
				}
			p -= 2 * w;
			}
		}
	}
else {
	tBGRA	c;
	tRgbaColorb *p = ((tRgbaColorb *) (bmP->bmTexBuf)) + w * (bmP->bmProps.h - 1);
	int bShaderMerge = gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk;
	nFrames = h / w - 1;
	for (i = 0; i < h; i++) {
		n = nFrames - i / w;
		for (j = w; j; j--, p++) {
			if (bShaderMerge && !(p->red || p->green || p->blue) && (p->alpha == 1)) {
				c.r = 120;
				c.g = 88;
				c.b = 128;
				c.a = 1;
				}
			else {
				c.r = p->red;
				c.g = p->green;
				c.b = p->blue;
				c.a = ((p->red == 120) && (p->green == 88) && (p->blue == 128)) ? 255 : p->alpha;
				}
			if (CFWrite (&c, 1, 4, cfp) != (size_t) 4)
				return 0;
			}
		p -= 2 * w;
		}
	}
return 1;
}

//---------------------------------------------------------------

int ReadTGAHeader (CFILE *cfp, tTgaHeader *ph, grsBitmap *bmP)
{
	tTgaHeader	h;

h.identSize = (char) CFReadByte (cfp);
h.colorMapType = (char) CFReadByte (cfp);
h.imageType = (char) CFReadByte (cfp);
h.colorMapStart = CFReadShort (cfp);
h.colorMapLength = CFReadShort (cfp);
h.colorMapBits = (char) CFReadByte (cfp);
h.xStart = CFReadShort (cfp);
h.yStart = CFReadShort (cfp);
h.width = CFReadShort (cfp);
h.height = CFReadShort (cfp);
h.bits = (char) CFReadByte (cfp);
h.descriptor = (char) CFReadByte (cfp);
if (h.identSize)
	CFSeek (cfp, h.identSize, SEEK_CUR);
if (bmP) {
	GrInitBitmap (bmP, 0, 0, 0, h.width, h.height, h.width, NULL, bmP->bmBPP = h.bits / 8);
	}
if (ph)
	*ph = h;
return 1;
}

//---------------------------------------------------------------

int WriteTGAHeader (CFILE *cfp, tTgaHeader *ph, grsBitmap *bmP)
{
memset (ph, 0, sizeof (*ph));
ph->width = bmP->bmProps.w;
ph->height = bmP->bmProps.h;
ph->bits = bmP->bmBPP * 8;
ph->imageType = 2;
CFWriteByte (ph->identSize, cfp);
CFWriteByte (ph->colorMapType, cfp);
CFWriteByte (ph->imageType, cfp);
CFWriteShort (ph->colorMapStart, cfp);
CFWriteShort (ph->colorMapLength, cfp);
CFWriteByte (ph->colorMapBits, cfp);
CFWriteShort (ph->xStart, cfp);
CFWriteShort (ph->yStart, cfp);
CFWriteShort (ph->width, cfp);
CFWriteShort (ph->height, cfp);
if (!GrBitmapHasTransparency (bmP))
	ph->bits = 24;
CFWriteByte (ph->bits, cfp);
CFWriteByte (ph->descriptor, cfp);
if (ph->identSize)
	CFSeek (cfp, ph->identSize, SEEK_CUR);
return 1;
}

//---------------------------------------------------------------

int LoadTGA (CFILE *cfp, grsBitmap *bmP, int alpha, double brightness, 
				 int bGrayScale, int bReverse)
{
	tTgaHeader	h;

return ReadTGAHeader (cfp, &h, bmP) &&
		 ReadTGAImage (cfp, &h, bmP, alpha, brightness, bGrayScale, bReverse);
}

//---------------------------------------------------------------

int WriteTGA (CFILE *cfp, tTgaHeader *ph, grsBitmap *bmP)
{
return WriteTGAHeader (cfp, ph, bmP) && WriteTGAImage (cfp, ph, bmP);
}

//---------------------------------------------------------------

int ReadTGA (char *pszFile, char *pszFolder, grsBitmap *bmP, int alpha, 
				 double brightness, int bGrayScale, int bReverse)
{
	CFILE	cf = {NULL, 0, 0, 0};
	char	fn [FILENAME_LEN], *psz;
	int	r;

if (!pszFolder)
	pszFolder = gameFolders.szDataDir;
#if TEXTURE_COMPRESSION
if (ReadS3TC (bmP, pszFolder, pszFile))
	return 1;
#endif
if (!CFOpen (&cf, pszFile, pszFolder, "rb", 0) && !(psz = strstr (pszFile, ".tga"))) {
	strcpy (fn, pszFile);
	if ((psz = strchr (fn, '.')))
		*psz = '\0';
	strcat (fn, ".tga");
	pszFile = fn;
	CFOpen (&cf, pszFile, pszFolder, "rb", 0);
	}
r = (cf.file != NULL) && LoadTGA (&cf, bmP, alpha, brightness, bGrayScale, bReverse);
#if TEXTURE_COMPRESSION
if (r && CompressTGA (bmP))
	SaveS3TC (bmP, pszFolder, pszFile);
#endif
if (&cf)
	CFClose (&cf);
#if 1//def _DEBUG
strncpy (bmP->szName, pszFile, sizeof (bmP->szName) - 1);
if ((psz = strrchr (bmP->szName, '.')))
	*psz = '\0';
else
	bmP->szName [sizeof (bmP->szName) - 1] = '\0';
#endif
return r;
}

//	-----------------------------------------------------------------------------

grsBitmap *CreateAndReadTGA (char *szFile)
{
	grsBitmap	*bmP = NULL;

if (!(bmP = GrCreateBitmap (0, 0, 4)))
	return NULL;
if (ReadTGA (szFile, NULL, bmP, -1, 1.0, 0, 0)) {
	bmP->bmType = BM_TYPE_ALT;
	strncpy (bmP->szName, szFile, sizeof (bmP->szName));
	return bmP;
	}
bmP->bmType = BM_TYPE_ALT;
GrFreeBitmap (bmP);
return NULL;
}

//	-----------------------------------------------------------------------------

int SaveTGA (char *pszFile, char *pszFolder, tTgaHeader *ph, grsBitmap *bmP)
{
	CFILE			cf = {NULL, 0, 0, 0};
	char			fn [FILENAME_LEN], fs [5];
	int			r;
	tTgaHeader	h;

if (!ph)
	memset (ph = &h, 0, sizeof (h));
if (!pszFolder)
	pszFolder = gameFolders.szDataDir;
CFSplitPath (pszFile, NULL, fn, NULL);
sprintf (fs, "-%d", bmP->bmProps.w);
strcat (fn, fs);
strcat (fn, ".tga");
r = CFOpen (&cf, fn, pszFolder, "wb", 0) && WriteTGA (&cf, ph, bmP);
if (cf.file)
	CFClose (&cf);
return r;
}

//	-----------------------------------------------------------------------------

int ShrinkTGA (grsBitmap *bmP, int xFactor, int yFactor, int bRealloc)
{
	int		bpp = bmP->bmBPP;
	int		xSrc, ySrc, xMax, yMax, xDest, yDest, x, y, w, h, i, nFactor2, nSuperTransp, bSuperTransp;
	ubyte		*pData, *pSrc, *pDest;
	int		cSum [4];

	static ubyte superTranspKeys [3] = {120,88,128};

if (!bmP->bmTexBuf)
	return 0;
if ((xFactor < 1) || (yFactor < 1))
	return 0;
if ((xFactor == 1) && (yFactor == 1))
	return 0;
w = bmP->bmProps.w;
h = bmP->bmProps.h;
xMax = w / xFactor;
yMax = h / yFactor;
nFactor2 = xFactor * yFactor;
if (!bRealloc)
	pDest = pData = bmP->bmTexBuf;
else {
	if (!(pData = D2_ALLOC (xMax * yMax * bpp)))
		return 0;
	UseBitmapCache (bmP, (int) -bmP->bmProps.h * (int) bmP->bmProps.rowSize);
	pDest = pData;
	}
if (bpp == 3) {
	for (yDest = 0; yDest < yMax; yDest++) {
		for (xDest = 0; xDest < xMax; xDest++) {
			memset (&cSum, 0, sizeof (cSum));
			ySrc = yDest * yFactor;
			for (y = yFactor; y; ySrc++, y--) {
				xSrc = xDest * xFactor;
				pSrc = bmP->bmTexBuf + (ySrc * w + xSrc) * bpp;
				for (x = xFactor; x; xSrc++, x--) {
					for (i = 0; i < bpp; i++)
						cSum [i] += *pSrc++;
					}
				}
			for (i = 0; i < bpp; i++)
				*pDest++ = (ubyte) (cSum [i] / (nFactor2));
			}
		}
	}
else {
	for (yDest = 0; yDest < yMax; yDest++) {
		for (xDest = 0; xDest < xMax; xDest++) {
			memset (&cSum, 0, sizeof (cSum));
			ySrc = yDest * yFactor;
			nSuperTransp = 0;
			for (y = yFactor; y; ySrc++, y--) {
				xSrc = xDest * xFactor;
				pSrc = bmP->bmTexBuf + (ySrc * w + xSrc) * bpp;
				for (x = xFactor; x; xSrc++, x--) {
						bSuperTransp = (pSrc [0] == 120) && (pSrc [1] == 88) && (pSrc [2] == 128);
					if (bSuperTransp) {
						nSuperTransp++;
						pSrc += bpp;
						}
					else
						for (i = 0; i < bpp; i++)
							cSum [i] += *pSrc++;
					}
				}
			if (nSuperTransp >= nFactor2 / 2) {
				pDest [0] = 120;
				pDest [1] = 88;
				pDest [2] = 128;
				pDest [3] = 0;
				pDest += bpp;
				}
			else {
				for (i = 0, bSuperTransp = 1; i < bpp; i++)
					pDest [i] = (ubyte) (cSum [i] / (nFactor2 - nSuperTransp));
				if (!(bmP->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT)) {
					for (i = 0; i < 3; i++) 
						if (pDest [i] != superTranspKeys [i])
							break;
					if (i == 3)
						pDest [0] =
						pDest [1] =
						pDest [2] =
						pDest [3] = 0;
					}
				pDest += bpp;
				}
			}
		}
	}
if (bRealloc) {
	D2_FREE (bmP->bmTexBuf);
	bmP->bmTexBuf = pData;
	}
bmP->bmProps.w = xMax;
bmP->bmProps.h = yMax;
bmP->bmProps.rowSize /= xFactor;
if (bRealloc)
	UseBitmapCache (bmP, (int) bmP->bmProps.h * (int) bmP->bmProps.rowSize);
return 1;
}

//	-----------------------------------------------------------------------------

double TGABrightness (grsBitmap *bmP)
{
if (!bmP) 
	return 0;
else {
		int		bAlpha = bmP->bmBPP == 4, i, j;
		ubyte		*pData;
		double	pixelBright, totalBright, nPixels, alpha;

	if (!(pData = bmP->bmTexBuf))
		return 0;
	totalBright = 0;
	nPixels = 0;
	for (i = bmP->bmProps.w * bmP->bmProps.h; i; i--) {
		for (pixelBright = 0, j = 0; j < 3; j++)
			pixelBright += ((double) (*pData++)) / 255.0;
		pixelBright /= 3;
		if (bAlpha) {
			alpha = ((double) (*pData++)) / 255.0;
			pixelBright *= alpha;
			nPixels += alpha;
			}
		totalBright += pixelBright;
		}
	return totalBright / nPixels;
	}
}

//	-----------------------------------------------------------------------------

void TGAChangeBrightness (grsBitmap *bmP, double dScale, int bInverse, int nOffset, int bSkipAlpha)
{
if (bmP) {
		int		bpp = bmP->bmBPP, h, i, j, c, bAlpha = (bpp == 4);
		ubyte		*pData;

	if ((pData = bmP->bmTexBuf)) {
	if (!bAlpha)
		bSkipAlpha = 1;
	else if (bSkipAlpha)
		bpp = 3;
		if (nOffset) {
			for (i = bmP->bmProps.w * bmP->bmProps.h; i; i--) {
				for (h = 0, j = 3; j; j--, pData++) {
					c = (int) *pData + nOffset;
					h += c;
					*pData = (ubyte) ((c < 0) ? 0 : (c > 255) ? 255 : c);
					}
				if (bSkipAlpha)
					pData++;
				else if (bAlpha) {
					if ((c = *pData)) {
						c += nOffset;
						*pData = (ubyte) ((c < 0) ? 0 : (c > 255) ? 255 : c);
						}
					pData++;
					}
				}
			}
		else if (dScale && (dScale != 1.0)) {
			if (dScale < 0) {
				for (i = bmP->bmProps.w * bmP->bmProps.h; i; i--) {
					for (j = bpp; j; j--, pData++)
						*pData = (ubyte) (*pData * dScale);
					if (bSkipAlpha)
						pData++;
					}
				}
			else if (bInverse) {
				dScale = 1.0 / dScale;
				for (i = bmP->bmProps.w * bmP->bmProps.h; i; i--) {
					for (j = bpp; j; j--, pData++)
						if ((c = 255 - *pData))
							*pData = 255 - (ubyte) (c * dScale);
					if (bSkipAlpha)
						pData++;
					}
				}
			else {
				for (i = bmP->bmProps.w * bmP->bmProps.h; i; i--) {
					for (j = bpp; j; j--, pData++)
						if ((c = 255 - *pData)) {
							c = (int) (*pData * dScale);
							*pData = (ubyte) ((c > 255) ? 255 : c);
							}
					if (bSkipAlpha)
						pData++;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

grsBitmap *CreateSuperTranspMask (grsBitmap *bmP)
{
	grsBitmap	*bmMask;
	int			i = (int) bmP->bmProps.w * (int) bmP->bmProps.h;
	ubyte			*pi;
	ubyte			*pm;

if (!gameStates.render.textures.bHaveMaskShader)
	return NULL;
if (BM_MASK (bmP))
	return BM_MASK (bmP);
bmP->bmBPP = 4;
if (!(bmMask = GrCreateBitmap ((bmP->bmProps.w  + 1) / 2, (bmP->bmProps.h + 1) / 2, 4)))
	return NULL;
BM_MASK (bmP) = bmMask;
#ifdef _DEBUG
sprintf (bmMask->szName, "{%s}", bmP->szName);
#endif
bmMask->bmProps.w = bmP->bmProps.w;
bmMask->bmProps.h = bmP->bmProps.w;
bmMask->bmProps.rowSize = bmMask->bmProps.w;
bmMask->bmBPP = 1;
UseBitmapCache (bmMask, (int) bmMask->bmProps.h * (int) bmMask->bmProps.rowSize);
if (bmP->bmProps.flags & BM_FLAG_TGA) {
	for (pi = bmP->bmTexBuf, pm = bmMask->bmTexBuf; i; i--, pi += 4, pm++)
		if ((pi [0] == 120) && (pi [1] == 88) && (pi [2] == 128))
			*pm = 0;
		else
			*pm = 0xff;
	}
else {
	for (pi = bmP->bmTexBuf, pm = bmMask->bmTexBuf; i; i--, pi++, pm++)
		if (*pi == SUPER_TRANSP_COLOR)
			*pm = 0;
		else
			*pm = 0xff;
	}
return bmP->bmData.std.bmMask;
}

//------------------------------------------------------------------------------

int CreateSuperTranspMasks (grsBitmap *bmP)
{
	int	nMasks, i, nFrames;

if (!gameStates.render.textures.bHaveMaskShader)
	return 0;
if ((bmP->bmType != BM_TYPE_ALT) || !bmP->bmData.alt.bmFrames) {
	if (bmP->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT)
		return CreateSuperTranspMask (bmP) != NULL;
	return 0;
	}
nFrames = BM_FRAMECOUNT (bmP);
for (nMasks = i = 0; i < nFrames; i++)
	if (bmP->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT)
		if (CreateSuperTranspMask (bmP->bmData.alt.bmFrames + i))
			nMasks++;
return nMasks;
}

//------------------------------------------------------------------------------

int TGAInterpolate (grsBitmap *bmP, int nScale)
{
	ubyte	*bufP, *destP, *srcP1, *srcP2;
	int	nSize, nFrameSize, nStride, nFrames, i, j;

if (nScale < 1)
	nScale = 1;
else if (nScale > 3)
	nScale = 3;
nScale = 1 << nScale;
nFrames = bmP->bmProps.h / bmP->bmProps.w;
nFrameSize = bmP->bmProps.w * bmP->bmProps.w * bmP->bmBPP;
nSize = nFrameSize * nFrames * nScale;
if (!(bufP = (ubyte *) D2_ALLOC (nSize)))
	return 0;
bmP->bmProps.h *= nScale;
memset (bufP, 0, nSize);
for (destP = bufP, srcP1 = bmP->bmTexBuf, i = 0; i < nFrames; i++) {
	memcpy (destP, srcP1, nFrameSize);
	destP += nFrameSize * nScale;
	srcP1 += nFrameSize;
	}
#if 1
while (nScale > 1) {
	nStride = nFrameSize * nScale;
	for (i = 0; i < nFrames; i++) {
		srcP1 = bufP + nStride * i;
		srcP2 = bufP + nStride * ((i + 1) % nFrames);
		destP = srcP1 + nStride / 2;
		for (j = nFrameSize; j; j--) {
			*destP++ = (ubyte) (((short) *srcP1++ + (short) *srcP2++) / 2);
			if (destP - bufP > nSize)
				destP = destP;
			}
		}
	nScale >>= 1;
	nFrames <<= 1;
	}
#endif
D2_FREE (bmP->bmTexBuf);
bmP->bmTexBuf = bufP;
return nFrames;
}

//------------------------------------------------------------------------------

int TGAMakeSquare (grsBitmap *bmP)
{
	ubyte	*bufP, *destP, *srcP;
	int	nSize, nFrameSize, nRowSize, nFrames, i, j, w, q;

nFrames = bmP->bmProps.h / bmP->bmProps.w;
if (nFrames < 4)
	return 0;
for (q = nFrames; q * q > nFrames; q >>= 1)
	;
if (q * q != nFrames)
	return 0;
w = bmP->bmProps.w;
nFrameSize = w * w * bmP->bmBPP;
nSize = nFrameSize * nFrames;
if (!(bufP = (ubyte *) D2_ALLOC (nSize)))
	return 0;
srcP = bmP->bmTexBuf;
nRowSize = w * bmP->bmBPP;
for (destP = bufP, i = 0; i < nFrames; i++) {
	for (j = 0; j < w; j++) {
		destP = bufP + (i / q) * q * nFrameSize + j * q * nRowSize + (i % q) * nRowSize;
		memcpy (destP, srcP, nRowSize);
		srcP += nRowSize;
		}
	}
D2_FREE (bmP->bmTexBuf);
bmP->bmTexBuf = bufP;
bmP->bmProps.w =
bmP->bmProps.h = q * w;
return q;
}

//------------------------------------------------------------------------------

int ReadModelTGA (char *pszFile, grsBitmap *bmP, short nType, int bCustom)
{
	char			fn [FILENAME_LEN], fnBase [FILENAME_LEN], fnShrunk [FILENAME_LEN];
	int			nShrinkFactor = 1 << (3 - gameStates.render.nModelQuality);
	time_t		tBase, tShrunk;

if (!pszFile)
	return 1;
CFSplitPath (pszFile + 1, NULL, fn, NULL);
if (!bCustom && (nShrinkFactor > 1)) {
	sprintf (fnBase, "%s.tga", fn);
	sprintf (fnShrunk, "%s-%d.tga", fn, 512 / nShrinkFactor);
	tBase = CFDate (fnBase, gameFolders.szModelDir [nType], 0);
	tShrunk = CFDate (fnShrunk, gameFolders.szModelCacheDir, 0);
	if ((tShrunk > tBase) && ReadTGA (fnShrunk, gameFolders.szModelCacheDir, bmP, -1, 1.0, 0, 0)) {
#ifdef _DEBUG
		strncpy (bmP->szName, fn, sizeof (bmP->szName));
#endif
		UseBitmapCache (bmP, (int) bmP->bmProps.h * (int) bmP->bmProps.rowSize);
		return 1;
		}
	}
if (!ReadTGA (pszFile + !bCustom, gameFolders.szModelDir [nType], bmP, -1, 1.0, 0, 0))
	return 0;
UseBitmapCache (bmP, (int) bmP->bmProps.h * (int) bmP->bmProps.rowSize);
if (gameStates.app.bCacheTextures && !bCustom && (nShrinkFactor > 1) && 
	 (bmP->bmProps.w == 512) && ShrinkTGA (bmP, nShrinkFactor, nShrinkFactor, 1)) {
	tTgaHeader	h;
	CFILE			cf;

	strcat (fn, ".tga");
	if (!CFOpen (&cf, fn, gameFolders.szModelDir [nType], "rb", 0))
		return 1;
	if (ReadTGAHeader (&cf, &h, NULL))
		SaveTGA (fn, gameFolders.szModelCacheDir, &h, bmP);
	CFClose (&cf);
	}
return 1;
}

//------------------------------------------------------------------------------

int ReadModelTextures (tModelTextures *pt, int nType, int bCustom)
{
	grsBitmap	*bmP;
	int			i;

for (i = 0; i < pt->nBitmaps; i++) {
	if (!ReadModelTGA (pt->pszNames [i], bmP = pt->pBitmaps + i, nType, bCustom))
		return 0;
	bmP = BmOverride (bmP, -1);
	if (BM_FRAMES (bmP))
		bmP = BM_CURFRAME (bmP);
	OglBindBmTex (bmP, 1, 3);
	pt->pBitmaps [i].bmTeam = pt->nTeam ? pt->nTeam [i] : 0;
	}
return 1;
}

//------------------------------------------------------------------------------

void ReleaseModelTextures (tModelTextures *pt)
{
	grsBitmap	*bmP;
	int			i;

if ((bmP = pt->pBitmaps))
	for (i = pt->nBitmaps; i; i--, bmP++) {
		UseBitmapCache (bmP, (int) -bmP->bmProps.h * (int) bmP->bmProps.rowSize);
		GrFreeBitmapData (bmP);
		}
}

//------------------------------------------------------------------------------

void FreeModelTextures (tModelTextures *pt)
{
	int	i;

if (pt->pszNames) {
	for (i = 0; i < pt->nBitmaps; i++) {
		D2_FREE (pt->pszNames [i]);
		if (pt->pBitmaps)
			GrFreeBitmapData (pt->pBitmaps + i);
		}
	D2_FREE (pt->nTeam);
	D2_FREE (pt->pszNames);
	D2_FREE (pt->pBitmaps);
	pt->nBitmaps = 0;
	}
}

//------------------------------------------------------------------------------

