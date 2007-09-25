#include "inferno.h"
#include "u_mem.h"
#include "gr.h"

//------------------------------------------------------------------------------

int CompressTGA (grsBitmap *bmP)
{
if (!(gameStates.ogl.bTextureCompression && gameStates.ogl.bHaveTexCompression))
	return 0;
if (bmP->bm_props.h / bmP->bm_props.w > 1)
	return 0;	//don't compress animations
if (bmP->bm_props.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
	return 0;	//don't compress textures containing some form of transparency
if (OglLoadTexture (bmP, 0, 0, NULL, -1, 0))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int ReadTGAImage (CFILE *fp, tTgaHeader *ph, grsBitmap *bmP, int alpha, 
						double brightness, int bGrayScale, int bReverse)
{
	int				i, j, n, nVisible = 0, nFrames, nBytes = ph->bits / 8;
	int				h = bmP->bm_props.h;
	int				w = bmP->bm_props.w;
	tRgbColorf		avgColor;
	float				a;

if (!(bmP->bm_texBuf || (bmP->bm_texBuf = D2_ALLOC (ph->height * w * nBytes))))
	 return 0;
if (!bmP->bm_texBuf) {
	int nSize = ph->width * ph->height * nBytes;
	if (!(bmP->bm_texBuf = D2_ALLOC (nSize)))
		return 0;
	}
bmP->bm_bpp = nBytes;
memset (bmP->bm_transparentFrames, 0, sizeof (bmP->bm_transparentFrames));
memset (bmP->bm_supertranspFrames, 0, sizeof (bmP->bm_supertranspFrames));
avgColor.red = avgColor.green = avgColor.blue = 0;
if (ph->bits == 24) {
	tBGRA	c;
	tRgbColorb *p = ((tRgbColorb *) (bmP->bm_texBuf)) + w * (bmP->bm_props.h - 1);

	for (i = bmP->bm_props.h; i; i--) {
		for (j = w; j; j--, p++) {
			if (CFRead (&c, 1, 3, fp) != (size_t) 3)
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
else if (bReverse) {
	tRGBA	c;
	tRgbaColorb	*p = (tRgbaColorb *) bmP->bm_texBuf;
	int bShaderMerge = gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk;

	nFrames = h / w - 1;
	for (i = 0; i < h; i++) {
		n = nFrames - i / w;
		for (j = w; j; j--, p++) {
			if (CFRead (&c, 1, 4, fp) != (size_t) 4)
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
#if 0
				if (bShaderMerge) { 
					p->red =
					p->green =
					p->blue = 0;
					p->alpha = 1;
					}
				else
#endif
					p->alpha = 0;
				if (!n)
					bmP->bm_props.flags |= BM_FLAG_SUPER_TRANSPARENT;
				if (bmP)
					bmP->bm_supertranspFrames [n / 32] |= (1 << (n % 32));
				}
			else {
				p->alpha = (alpha < 0) ? c.a : alpha;
				if (!p->alpha)
					p->red =		//avoid colored transparent areas interfering with visible image edges
					p->green =
					p->blue = 0;
				}
			if (p->alpha < 128) {
				if (!n)
					bmP->bm_props.flags |= BM_FLAG_TRANSPARENT;
				if (bmP)
					bmP->bm_transparentFrames [n / 32] |= (1 << (n % 32));
				}
			nVisible += p->alpha;
			a = (float) p->alpha / 255;
			avgColor.red += p->red * a;
			avgColor.green += p->green * a;
			avgColor.blue += p->blue * a;
			}
		}
	}	
else {
	tBGRA	c;
	tRgbaColorb *p = ((tRgbaColorb *) (bmP->bm_texBuf)) + w * (bmP->bm_props.h - 1);
	int bShaderMerge = gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk;

	nFrames = h / w - 1;
	for (i = 0; i < h; i++) {
		n = nFrames - i / w;
		for (j = w; j; j--, p++) {
			if (CFRead (&c, 1, 4, fp) != (size_t) 4)
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
#if 0
				if (bShaderMerge) { 
					p->red =
					p->green =
					p->blue = 0;
					p->alpha = 1;
					}
				else
#endif
					p->alpha = 0;
				if (!n)
					bmP->bm_props.flags |= BM_FLAG_SUPER_TRANSPARENT;
				if (bmP)
					bmP->bm_supertranspFrames [n / 32] |= (1 << (n % 32));
				}
			else {
				p->alpha = (alpha < 0) ? c.a : alpha;
				if (!p->alpha)
					p->red =		//avoid colored transparent areas interfering with visible image edges
					p->green =
					p->blue = 0;
				}
			if (p->alpha < 128) {
				if (!n)
					bmP->bm_props.flags |= BM_FLAG_TRANSPARENT;
				bmP->bm_transparentFrames [n / 32] |= (1 << (n % 32));
				}
			nVisible += p->alpha;
			a = (float) p->alpha / 255;
			avgColor.red += p->red * a;
			avgColor.green += p->green * a;
			avgColor.blue += p->blue * a;
			}
		p -= 2 * w;
		}
	}	
a = (float) nVisible / 255.0f;
bmP->bm_avgRGB.red = (ubyte) (avgColor.red / a);
bmP->bm_avgRGB.green = (ubyte) (avgColor.green / a);
bmP->bm_avgRGB.blue = (ubyte) (avgColor.blue / a);
return 1;
}

//	-----------------------------------------------------------------------------

int WriteTGAImage (CFILE *fp, tTgaHeader *ph, grsBitmap *bmP)
{
	int				i, j, n, nFrames;
	int				h = bmP->bm_props.h;
	int				w = bmP->bm_props.w;

if (ph->bits == 24) {
	if (bmP->bm_bpp == 3) {
		tBGR	c;
		tRgbColorb *p = ((tRgbColorb *) (bmP->bm_texBuf)) + w * (bmP->bm_props.h - 1);
		for (i = bmP->bm_props.h; i; i--) {
			for (j = w; j; j--, p++) {
				c.r = p->red;
				c.g = p->green;
				c.b = p->blue;
				if (CFWrite (&c, 1, 3, fp) != (size_t) 3)
					return 0;
				}
			p -= 2 * w;
			}
		}
	else {
		tBGR	c;
		tRgbaColorb *p = ((tRgbaColorb *) (bmP->bm_texBuf)) + w * (bmP->bm_props.h - 1);
		for (i = bmP->bm_props.h; i; i--) {
			for (j = w; j; j--, p++) {
				c.r = p->red;
				c.g = p->green;
				c.b = p->blue;
				if (CFWrite (&c, 1, 3, fp) != (size_t) 3)
					return 0;
				}
			p -= 2 * w;
			}
		}
	}
else {
	tBGRA	c;
	tRgbaColorb *p = ((tRgbaColorb *) (bmP->bm_texBuf)) + w * (bmP->bm_props.h - 1);
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
			if (CFWrite (&c, 1, 4, fp) != (size_t) 4)
				return 0;
			}
		p -= 2 * w;
		}
	}	
return 1;
}

//---------------------------------------------------------------

int ReadTGAHeader (CFILE *fp, tTgaHeader *ph, grsBitmap *bmP)
{
	tTgaHeader	h;

h.identSize = (char) CFReadByte (fp);
h.colorMapType = (char) CFReadByte (fp);
h.imageType = (char) CFReadByte (fp);
h.colorMapStart = CFReadShort (fp);
h.colorMapLength = CFReadShort (fp);
h.colorMapBits = (char) CFReadByte (fp);
h.xStart = CFReadShort (fp);
h.yStart = CFReadShort (fp);
h.width = CFReadShort (fp);
h.height = CFReadShort (fp);
h.bits = (char) CFReadByte (fp);
h.descriptor = (char) CFReadByte (fp);
if (h.identSize)
	CFSeek (fp, h.identSize, SEEK_CUR);
if (bmP) {
	GrInitBitmap (bmP, 0, 0, 0, h.width, h.height, h.width, NULL, bmP->bm_bpp = h.bits / 8);
	}
if (ph)
	*ph = h;
return 1;
}

//---------------------------------------------------------------

int WriteTGAHeader (CFILE *fp, tTgaHeader *ph, grsBitmap *bmP)
{
ph->width = bmP->bm_props.w;
ph->height = bmP->bm_props.h;
ph->bits = bmP->bm_bpp * 8;
ph->imageType = 2;
CFWriteByte (ph->identSize, fp);
CFWriteByte (ph->colorMapType, fp);
CFWriteByte (ph->imageType, fp);
CFWriteShort (ph->colorMapStart, fp);
CFWriteShort (ph->colorMapLength, fp);
CFWriteByte (ph->colorMapBits, fp);
CFWriteShort (ph->xStart, fp);
CFWriteShort (ph->yStart, fp);
CFWriteShort (ph->width, fp);
CFWriteShort (ph->height, fp);
if (!GrBitmapHasTransparency (bmP))
	ph->bits = 24;
CFWriteByte (ph->bits, fp);
CFWriteByte (ph->descriptor, fp);
if (ph->identSize)
	CFSeek (fp, ph->identSize, SEEK_CUR);
return 1;
}

//---------------------------------------------------------------

int LoadTGA (CFILE *fp, grsBitmap *bmP, int alpha, double brightness, 
				 int bGrayScale, int bReverse)
{
	tTgaHeader	h;

return ReadTGAHeader (fp, &h, bmP) &&
		 ReadTGAImage (fp, &h, bmP, alpha, brightness, bGrayScale, bReverse);
}

//---------------------------------------------------------------

int WriteTGA (CFILE *fp, tTgaHeader *ph, grsBitmap *bmP)
{
return WriteTGAHeader (fp, ph, bmP) && WriteTGAImage (fp, ph, bmP);
}

//---------------------------------------------------------------

int ReadTGA (char *pszFile, char *pszFolder, grsBitmap *bmP, int alpha, 
				 double brightness, int bGrayScale, int bReverse)
{
	CFILE	*fp;
	char	fn [FILENAME_LEN], *psz;
	int	r;

if (!pszFolder)
	pszFolder = gameFolders.szDataDir;
#if TEXTURE_COMPRESSION
if (ReadS3TC (bmP, pszFolder, pszFile))
	return 1;
#endif
if (!(psz = strstr (pszFile, ".tga"))) {
	strcpy (fn, pszFile);
	if ((psz = strchr (fn, '.')))
		*psz = '\0';
	strcat (fn, ".tga");
	pszFile = fn;
	}
fp = CFOpen (pszFile, pszFolder, "rb", 0);
r = (fp != NULL) && LoadTGA (fp, bmP, alpha, brightness, bGrayScale, bReverse);
#if TEXTURE_COMPRESSION
if (r && CompressTGA (bmP))
	SaveS3TC (bmP, pszFolder, pszFile);
#endif
if (fp)
	CFClose (fp);
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
	return bmP;
	}
bmP->bmType = BM_TYPE_ALT;
GrFreeBitmap (bmP);
return NULL;
}

//	-----------------------------------------------------------------------------

int SaveTGA (char *pszFile, char *pszFolder, tTgaHeader *ph, grsBitmap *bmP)
{
	CFILE	*fp;
	char	fn [FILENAME_LEN], fs [5];
	int	r;

if (!pszFolder)
	pszFolder = gameFolders.szDataDir;
CFSplitPath (pszFile, NULL, fn, NULL);
sprintf (fs, "-%d", bmP->bm_props.w);
strcat (fn, fs);
strcat (fn, ".tga");
fp = CFOpen (fn, pszFolder, "wb", 0);
r = (fp != NULL) && WriteTGA (fp, ph, bmP);
if (fp)
	CFClose (fp);
return r;
}

//	-----------------------------------------------------------------------------

int ShrinkTGA (grsBitmap *bmP, int xFactor, int yFactor, int bRealloc)
{
	int		bpp = bmP->bm_bpp;
	int		xSrc, ySrc, xMax, yMax, xDest, yDest, x, y, w, h, i, nFactor2, nSuperTransp, bSuperTransp;
	int		bShaderMerge = (bpp == 4) && gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk;
	ubyte		*pData, *pSrc, *pDest;
	int		cSum [4];

if (!bmP->bm_texBuf)
	return 0;
if ((xFactor < 1) || (yFactor < 1))
	return 0;
if ((xFactor == 1) && (yFactor == 1))
	return 0;
w = bmP->bm_props.w;
h = bmP->bm_props.h;
xMax = w / xFactor;
yMax = h / yFactor;
nFactor2 = xFactor * yFactor;
if (!bRealloc)
	pDest = pData = bmP->bm_texBuf;
else {
	if (!(pData = D2_ALLOC (xMax * yMax * bpp)))
		return 0;
	UseBitmapCache (bmP, -bmP->bm_props.h * bmP->bm_props.rowsize);
	pDest = pData;
	}
for (yDest = 0; yDest < yMax; yDest++) {
	for (xDest = 0; xDest < xMax; xDest++) {
		memset (&cSum, 0, sizeof (cSum));
		ySrc = yDest * yFactor;
		nSuperTransp = 0;
		for (y = yFactor; y; ySrc++, y--) {
			xSrc = xDest * xFactor;
			pSrc = bmP->bm_texBuf + (ySrc * w + xSrc) * bpp;
			for (x = xFactor; x; xSrc++, x--) {
#if 0
				if (bShaderMerge)
					bSuperTransp = (pSrc [3] == 1);
				else
#endif
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
#if 0
			if (bShaderMerge) {
				pDest [0] = 
				pDest [1] = 
				pDest [2] = 0;
				pDest [3] = 1;
				}
			else
#endif
				{
				pDest [0] = 120;
				pDest [1] = 88;
				pDest [2] = 128;
				pDest [3] = 0;
				}
			pDest += bpp;
			}
		else {
			for (i = 0; i < bpp; i++, pDest++) {
				*pDest = (ubyte) (cSum [i] / (nFactor2 - nSuperTransp));
#if 0
				if (i == 3)	 {//alpha treatment round to 0 or 255 depending on the average alpha
					if (*pDest < 128)
						*pDest = 0;
					else if (*pDest > 127)
						*pDest = 255;
					}
#endif
				}
			}
		}
	}
if (bRealloc) {
	D2_FREE (bmP->bm_texBuf);
	bmP->bm_texBuf = pData;
	}
bmP->bm_props.w = xMax;
bmP->bm_props.h = yMax;
bmP->bm_props.rowsize /= xFactor;
if (bRealloc)
	UseBitmapCache (bmP, bmP->bm_props.h * bmP->bm_props.rowsize);
return 1;
}

//	-----------------------------------------------------------------------------

double TGABrightness (grsBitmap *bmP)
{
if (!bmP) 
	return 0;
else {
		int		bAlpha = bmP->bm_bpp == 4, i, j;
		ubyte		*pData;
		double	pixelBright, totalBright, nPixels, alpha;

	if (!(pData = bmP->bm_texBuf))
		return 0;
	totalBright = 0;
	nPixels = 0;
	for (i = bmP->bm_props.w * bmP->bm_props.h; i; i--) {
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
	int h = 0;

if (bmP) {
		int		bpp = bmP->bm_bpp, h, i, j, c, bAlpha = (bpp == 4);
		ubyte		*pData;

	if (pData = bmP->bm_texBuf) {
	if (!bAlpha)
		bSkipAlpha = 1;
	else if (bSkipAlpha)
		bpp = 3;
		if (nOffset) {
			for (i = bmP->bm_props.w * bmP->bm_props.h; i; i--) {
				for (h = 0, j = 3; j; j--, pData++) {
					c = (int) *pData + nOffset;
					h += c;
					*pData = (ubyte) ((c < 0) ? 0 : (c > 255) ? 255 : c);
					}
				if (bSkipAlpha)
					pData++;
				else if (bAlpha) {
					if (c = *pData) {
						c += nOffset;
						*pData = (ubyte) ((c < 0) ? 0 : (c > 255) ? 255 : c);
						}
					pData++;
					}
				}
			}
		else if (dScale && (dScale != 1.0)) {
			if (dScale < 0) {
				for (i = bmP->bm_props.w * bmP->bm_props.h; i; i--) {
					for (j = bpp; j; j--, pData++)
						*pData = (ubyte) (*pData * dScale);
					if (bSkipAlpha)
						pData++;
					}
				}
			else if (bInverse) {
				dScale = 1.0 / dScale;
				for (i = bmP->bm_props.w * bmP->bm_props.h; i; i--) {
					for (j = bpp; j; j--, pData++)
						if (c = 255 - *pData)
							*pData = 255 - (ubyte) (c * dScale);
					if (bSkipAlpha)
						pData++;
					}
				}
			else {
				for (i = bmP->bm_props.w * bmP->bm_props.h; i; i--) {
					for (j = bpp; j; j--, pData++)
						if (c = 255 - *pData) {
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
	int	i = bmP->bm_props.w * bmP->bm_props.h;
	ubyte	*pm, *pi;

if (bmP->bm_data.std.bm_mask)
	return bmP->bm_data.std.bm_mask;
if (!(bmP->bm_data.std.bm_mask = GrCreateBitmap (bmP->bm_props.w, bmP->bm_props.h, 1)))
	return NULL;
for (pi = bmP->bm_texBuf, pm = bmP->bm_data.std.bm_mask->bm_texBuf; i; i--, pi += 4, pm++)
	if ((pi [0] == 120) && (pi [1] == 88) && (pi [2] == 128))
		*pm = 0;
	else
		*pm = 1;
return bmP->bm_data.std.bm_mask;
}

//------------------------------------------------------------------------------

int CreateSuperTranspMasks (grsBitmap *bmP)
{
	int	nMasks, i, nFrames;

if (!gameStates.render.textures.bHaveMaskShader)
	return 0;
if (!(bmP->bm_props.flags & BM_FLAG_TGA))
	return 0;
if ((bmP->bmType != BM_TYPE_ALT) || !bmP->bm_data.alt.bm_frames) {
	if (bmP->bm_props.flags & BM_FLAG_SUPER_TRANSPARENT)
		return CreateSuperTranspMask (bmP) != NULL;
	return 0;
	}
nFrames = BM_FRAMECOUNT (bmP);
for (nMasks = i = 0; i < nFrames; i++)
if (bmP->bm_supertranspFrames [i / 32] & (1 << (i % 32)))
	if (CreateSuperTranspMask (bmP->bm_data.alt.bm_frames + i))
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
nFrames = bmP->bm_props.h / bmP->bm_props.w;
nFrameSize = bmP->bm_props.w * bmP->bm_props.w * bmP->bm_bpp;
nSize = nFrameSize * nFrames * nScale;
if (!(bufP = (ubyte *) D2_ALLOC (nSize)))
	return 0;
bmP->bm_props.h *= nScale;
memset (bufP, 0, nSize);
for (destP = bufP, srcP1 = bmP->bm_texBuf, i = 0; i < nFrames; i++) {
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
D2_FREE (bmP->bm_texBuf);
bmP->bm_texBuf = bufP;
return nFrames;
}

//------------------------------------------------------------------------------

int TGAMakeSquare (grsBitmap *bmP)
{
	ubyte	*bufP, *destP, *srcP;
	int	nSize, nFrameSize, nRowSize, nFrames, i, j, w, q;

nFrames = bmP->bm_props.h / bmP->bm_props.w;
if (nFrames < 4)
	return 0;
for (q = nFrames; q * q > nFrames; q >>= 1)
	;
if (q * q != nFrames)
	return 0;
w = bmP->bm_props.w;
nFrameSize = w * w * bmP->bm_bpp;
nSize = nFrameSize * nFrames;
if (!(bufP = (ubyte *) D2_ALLOC (nSize)))
	return 0;
srcP = bmP->bm_texBuf;
nRowSize = w * bmP->bm_bpp;
for (destP = bufP, i = 0; i < nFrames; i++) {
	for (j = 0; j < w; j++) {
		destP = bufP + (i / q) * q * nFrameSize + j * q * nRowSize + (i % q) * nRowSize;
		memcpy (destP, srcP, nRowSize);
		srcP += nRowSize;
		}
	}
D2_FREE (bmP->bm_texBuf);
bmP->bm_texBuf = bufP;
bmP->bm_props.w =
bmP->bm_props.h = q * w;
return q;
}

//------------------------------------------------------------------------------

