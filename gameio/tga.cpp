#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "u_mem.h"
#include "gr.h"
#include "tga.h"

//------------------------------------------------------------------------------

int CompressTGA (CBitmap *bmP)
{
if (!(gameStates.ogl.bTextureCompression && gameStates.ogl.bHaveTexCompression))
	return 0;
#if 0
if (bmP->Height () / bmP->Width () > 1)
	return 0;	//don't compress animations
if (bmP->Flags () & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
	return 0;	//don't compress textures containing some form of transparency
#endif
if (bmP->LoadTexture (0, 0, 0))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int ReadTGAImage (CFile& cf, tTgaHeader *ph, CBitmap *bmP, int alpha, 
						double brightness, int bGrayScale, int bReverse)
{
	int				i, j, n, nAlpha = 0, nVisible = 0, nFrames, nBytes = ph->bits / 8;
	int				h = bmP->Height ();
	int				w = bmP->Width ();
	tRgbColorf		avgColor;
	tRgbColorb		avgColorb;
	float				a, avgAlpha = 0;

bmP->SetBPP (nBytes);
if (!(bmP->Buffer () || bmP->CreateBuffer ()))
	 return 0;
bmP->SetTranspType (-1);
memset (bmP->TransparentFrames (), 0, 4 * sizeof (int));
memset (bmP->SuperTranspFrames (), 0, 4 * sizeof (int));
avgColor.red = avgColor.green = avgColor.blue = 0;
//bReverse = (ph->descriptor & (1 << 5)) != 0;
if (ph->bits == 24) {
	tBGRA	c;
	tRgbColorb *p = reinterpret_cast<tRgbColorb*> (bmP->Buffer ()) + w * (h - 1);

	for (i = h; i; i--) {
		for (j = w; j; j--, p++) {
			if (cf.Read (&c, 1, 3) != (size_t) 3)
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
	bmP->AddFlags (BM_FLAG_SEE_THRU);
	if (bReverse) {
		tRGBA	c;
		tRgbaColorb	*p = reinterpret_cast<tRgbaColorb*> (bmP->Buffer ());
		int nSuperTransp;

		nFrames = h / w - 1;
		for (i = 0; i < h; i++) {
			n = nFrames - i / w;
			nSuperTransp = 0;
			for (j = w; j; j--, p++) {
				if (cf.Read (&c, 1, 4) != (size_t) 4)
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
						bmP->AddFlags (BM_FLAG_TRANSPARENT);
						if (p->alpha)
							bmP->DelFlags (BM_FLAG_SEE_THRU);
						}
					if (bmP)
						bmP->TransparentFrames () [n / 32] |= (1 << (n % 32));
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
					bmP->AddFlags (BM_FLAG_SUPER_TRANSPARENT);
				bmP->AddFlags (BM_FLAG_SEE_THRU);
				bmP->SuperTranspFrames () [n / 32] |= (1 << (n % 32));
				}
			}
		}
	else {
		tBGRA	c;
		tRgbaColorb *p = reinterpret_cast<tRgbaColorb*> (bmP->Buffer ()) + w * (h - 1);
		int nSuperTransp;

		nFrames = h / w - 1;
		for (i = 0; i < h; i++) {
			n = nFrames - i / w;
			nSuperTransp = 0;
			for (j = w; j; j--, p++) {
				if (cf.Read (&c, 1, 4) != (size_t) 4)
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
						bmP->AddFlags (BM_FLAG_TRANSPARENT);
						if (p->alpha)
							bmP->DelFlags (BM_FLAG_SEE_THRU);
						}
					bmP->TransparentFrames () [n / 32] |= (1 << (n % 32));
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
					bmP->AddFlags (BM_FLAG_SUPER_TRANSPARENT);
				bmP->AddFlags (BM_FLAG_SEE_THRU);
				bmP->SuperTranspFrames () [n / 32] |= (1 << (n % 32));
				}
			p -= 2 * w;
			}
		}
	}
a = (float) nVisible / 255.0f;
avgColorb.red = (ubyte) (avgColor.red / a);
avgColorb.green = (ubyte) (avgColor.green / a);
avgColorb.blue = (ubyte) (avgColor.blue / a);
bmP->SetAvgColor (avgColorb);
if (!nAlpha)
	bmP->DelFlags (BM_FLAG_SEE_THRU);
#if 0
if (nAlpha && ((ubyte) (avgAlpha / nAlpha) < 2))
	bmP->Flags |= BM_FLAG_SEE_THRU;
#endif
return 1;
}

//	-----------------------------------------------------------------------------

int WriteTGAImage (CFile& cf, tTgaHeader *ph, CBitmap *bmP)
{
	int				i, j, n, nFrames;
	int				h = bmP->Height ();
	int				w = bmP->Width ();

if (ph->bits == 24) {
	if (bmP->BPP () == 3) {
		tBGR	c;
		tRgbColorb *p = reinterpret_cast<tRgbColorb*> (bmP->Buffer ()) + w * (bmP->Height () - 1);
		for (i = bmP->Height (); i; i--) {
			for (j = w; j; j--, p++) {
				c.r = p->red;
				c.g = p->green;
				c.b = p->blue;
				if (cf.Write (&c, 1, 3) != (size_t) 3)
					return 0;
				}
			p -= 2 * w;
			}
		}
	else {
		tBGR	c;
		tRgbaColorb *p = reinterpret_cast<tRgbaColorb*> (bmP->Buffer ()) + w * (bmP->Height () - 1);
		for (i = bmP->Height (); i; i--) {
			for (j = w; j; j--, p++) {
				c.r = p->red;
				c.g = p->green;
				c.b = p->blue;
				if (cf.Write (&c, 1, 3) != (size_t) 3)
					return 0;
				}
			p -= 2 * w;
			}
		}
	}
else {
	tBGRA	c;
	tRgbaColorb *p = reinterpret_cast<tRgbaColorb*> (bmP->Buffer ()) + w * (bmP->Height () - 1);
	int bShaderMerge = gameOpts->ogl.bGlTexMerge;
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
			if (cf.Write (&c, 1, 4) != (size_t) 4)
				return 0;
			}
		p -= 2 * w;
		}
	}
return 1;
}

//---------------------------------------------------------------

int ReadTGAHeader (CFile& cf, tTgaHeader *ph, CBitmap *bmP)
{
	tTgaHeader	h;

h.identSize = (char) cf.ReadByte ();
h.colorMapType = (char) cf.ReadByte ();
h.imageType = (char) cf.ReadByte ();
h.colorMapStart = cf.ReadShort ();
h.colorMapLength = cf.ReadShort ();
h.colorMapBits = (char) cf.ReadByte ();
h.xStart = cf.ReadShort ();
h.yStart = cf.ReadShort ();
h.width = cf.ReadShort ();
h.height = cf.ReadShort ();
h.bits = (char) cf.ReadByte ();
h.descriptor = (char) cf.ReadByte ();
if (h.identSize)
	cf.Seek (h.identSize, SEEK_CUR);
if (bmP) {
	bmP->Init (0, 0, 0, h.width, h.height, h.bits / 8, NULL);
	}
if (ph)
	*ph = h;
return 1;
}

//---------------------------------------------------------------

int WriteTGAHeader (CFile& cf, tTgaHeader *ph, CBitmap *bmP)
{
memset (ph, 0, sizeof (*ph));
ph->width = bmP->Width ();
ph->height = bmP->Height ();
ph->bits = bmP->BPP () * 8;
ph->imageType = 2;
cf.WriteByte (ph->identSize);
cf.WriteByte (ph->colorMapType);
cf.WriteByte (ph->imageType);
cf.WriteShort (ph->colorMapStart);
cf.WriteShort (ph->colorMapLength);
cf.WriteByte (ph->colorMapBits);
cf.WriteShort (ph->xStart);
cf.WriteShort (ph->yStart);
cf.WriteShort (ph->width);
cf.WriteShort (ph->height);
if (!bmP->HasTransparency ())
	ph->bits = 24;
cf.WriteByte (ph->bits);
cf.WriteByte (ph->descriptor);
if (ph->identSize)
	cf.Seek (ph->identSize, SEEK_CUR);
return 1;
}

//---------------------------------------------------------------

int LoadTGA (CFile& cf, CBitmap *bmP, int alpha, double brightness, 
				 int bGrayScale, int bReverse)
{
	tTgaHeader	h;

return ReadTGAHeader (cf, &h, bmP) &&
		 ReadTGAImage (cf, &h, bmP, alpha, brightness, bGrayScale, bReverse);
}

//---------------------------------------------------------------

int WriteTGA (CFile& cf, tTgaHeader *ph, CBitmap *bmP)
{
return WriteTGAHeader (cf, ph, bmP) && WriteTGAImage (cf, ph, bmP);
}

//---------------------------------------------------------------

int ReadTGA (const char *pszFile, const char *pszFolder, CBitmap *bmP, int alpha, 
				 double brightness, int bGrayScale, int bReverse)
{
	CFile	cf;
	char	fn [FILENAME_LEN], *psz;
	int	r;

if (!pszFolder) {
	cf.SplitPath (pszFile, fn, NULL, NULL);
	if (!*fn)
		pszFolder = gameFolders.szDataDir;
	}
#if TEXTURE_COMPRESSION
if (bmP->ReadS3TC (pszFolder, pszFile))
	return 1;
#endif
if (!cf.Open (pszFile, pszFolder, "rb", 0) && !(psz = const_cast<char*> (strstr (pszFile, ".tga")))) {
	strcpy (fn, pszFile);
	if ((psz = strchr (fn, '.')))
		*psz = '\0';
	strcat (fn, ".tga");
	pszFile = fn;
	cf.Open (pszFile, pszFolder, "rb", 0);
	}
r = (cf.File() != NULL) && LoadTGA (cf, bmP, alpha, brightness, bGrayScale, bReverse);
#if TEXTURE_COMPRESSION
if (r && CompressTGA (bmP))
	bmP->SaveS3TC (pszFolder, pszFile);
#endif
cf.Close ();
#if 1//def _DEBUG
char szName [20];
strncpy (szName, pszFile, sizeof (szName));
if ((psz = strrchr (szName, '.')))
	*psz = '\0';
else
	szName [sizeof (szName) - 1] = '\0';
bmP->SetName (szName);
#endif
return r;
}

//	-----------------------------------------------------------------------------

CBitmap *CreateAndReadTGA (const char *szFile)
{
	CBitmap	*bmP = NULL;

if (!(bmP = CBitmap::Create (0, 0, 0, 4)))
	return NULL;
if (ReadTGA (szFile, NULL, bmP, -1, 1.0, 0, 0)) {
	char	szName [FILENAME_LEN];
	CFile::SplitPath (szFile, NULL, szName, NULL);
	bmP->SetType (BM_TYPE_ALT);
	bmP->SetName (szName);
	return bmP;
	}
bmP->SetType (BM_TYPE_ALT);
delete bmP;
return NULL;
}

//	-----------------------------------------------------------------------------

int SaveTGA (const char *pszFile, const char *pszFolder, tTgaHeader *ph, CBitmap *bmP)
{
	CFile			cf;
	char			szFolder [FILENAME_LEN], fn [FILENAME_LEN];
	int			r;
	tTgaHeader	h;

if (!ph)
	memset (ph = &h, 0, sizeof (h));
if (!pszFolder)
	pszFolder = gameFolders.szDataDir;
CFile::SplitPath (pszFile, NULL, fn, NULL);
sprintf (szFolder, "%s/%d/", pszFolder, bmP->Width ());
strcat (fn, ".tga");
r = cf.Open (fn, szFolder, "wb", 0) && WriteTGA (cf, ph, bmP);
if (cf.File ())
	cf.Close ();
return r;
}

//	-----------------------------------------------------------------------------

int ShrinkTGA (CBitmap *bmP, int xFactor, int yFactor, int bRealloc)
{
	int		bpp = bmP->BPP ();
	int		xSrc, ySrc, xMax, yMax, xDest, yDest, x, y, w, h, i, nFactor2, nSuperTransp, bSuperTransp;
	ubyte		*dataP, *pSrc, *pDest;
	int		cSum [4];

	static ubyte superTranspKeys [3] = {120,88,128};

if (!bmP->Buffer ())
	return 0;
if ((xFactor < 1) || (yFactor < 1))
	return 0;
if ((xFactor == 1) && (yFactor == 1))
	return 0;
w = bmP->Width ();
h = bmP->Height ();
xMax = w / xFactor;
yMax = h / yFactor;
nFactor2 = xFactor * yFactor;
if (!bRealloc)
	pDest = dataP = bmP->Buffer ();
else {
	if (!(dataP = new ubyte [xMax * yMax * bpp]))
		return 0;
	UseBitmapCache (bmP, (int) -bmP->Height () * (int) bmP->RowSize ());
	pDest = dataP;
	}
if (bpp == 3) {
	for (yDest = 0; yDest < yMax; yDest++) {
		for (xDest = 0; xDest < xMax; xDest++) {
			memset (&cSum, 0, sizeof (cSum));
			ySrc = yDest * yFactor;
			for (y = yFactor; y; ySrc++, y--) {
				xSrc = xDest * xFactor;
				pSrc = bmP->Buffer () + (ySrc * w + xSrc) * bpp;
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
				pSrc = bmP->Buffer () + (ySrc * w + xSrc) * bpp;
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
				if (!(bmP->Flags () & BM_FLAG_SUPER_TRANSPARENT)) {
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
	bmP->DestroyBuffer ();
	bmP->SetBuffer (dataP);
	}
bmP->SetWidth (xMax);
bmP->SetHeight (yMax);
bmP->SetRowSize (bmP->RowSize () / xFactor);
if (bRealloc)
	UseBitmapCache (bmP, (int) bmP->Height () * (int) bmP->RowSize ());
return 1;
}

//	-----------------------------------------------------------------------------

double TGABrightness (CBitmap *bmP)
{
if (!bmP) 
	return 0;
else {
		int		bAlpha = bmP->BPP () == 4, i, j;
		ubyte		*dataP;
		double	pixelBright, totalBright, nPixels, alpha;

	if (!(dataP = bmP->Buffer ()))
		return 0;
	totalBright = 0;
	nPixels = 0;
	for (i = bmP->Width () * bmP->Height (); i; i--) {
		for (pixelBright = 0, j = 0; j < 3; j++)
			pixelBright += ((double) (*dataP++)) / 255.0;
		pixelBright /= 3;
		if (bAlpha) {
			alpha = ((double) (*dataP++)) / 255.0;
			pixelBright *= alpha;
			nPixels += alpha;
			}
		totalBright += pixelBright;
		}
	return totalBright / nPixels;
	}
}

//	-----------------------------------------------------------------------------

void TGAChangeBrightness (CBitmap *bmP, double dScale, int bInverse, int nOffset, int bSkipAlpha)
{
if (bmP) {
		int		bpp = bmP->BPP (), h, i, j, c, bAlpha = (bpp == 4);
		ubyte		*dataP;

	if ((dataP = bmP->Buffer ())) {
	if (!bAlpha)
		bSkipAlpha = 1;
	else if (bSkipAlpha)
		bpp = 3;
		if (nOffset) {
			for (i = bmP->Width () * bmP->Height (); i; i--) {
				for (h = 0, j = 3; j; j--, dataP++) {
					c = (int) *dataP + nOffset;
					h += c;
					*dataP = (ubyte) ((c < 0) ? 0 : (c > 255) ? 255 : c);
					}
				if (bSkipAlpha)
					dataP++;
				else if (bAlpha) {
					if ((c = *dataP)) {
						c += nOffset;
						*dataP = (ubyte) ((c < 0) ? 0 : (c > 255) ? 255 : c);
						}
					dataP++;
					}
				}
			}
		else if (dScale && (dScale != 1.0)) {
			if (dScale < 0) {
				for (i = bmP->Width () * bmP->Height (); i; i--) {
					for (j = bpp; j; j--, dataP++)
						*dataP = (ubyte) (*dataP * dScale);
					if (bSkipAlpha)
						dataP++;
					}
				}
			else if (bInverse) {
				dScale = 1.0 / dScale;
				for (i = bmP->Width () * bmP->Height (); i; i--) {
					for (j = bpp; j; j--, dataP++)
						if ((c = 255 - *dataP))
							*dataP = 255 - (ubyte) (c * dScale);
					if (bSkipAlpha)
						dataP++;
					}
				}
			else {
				for (i = bmP->Width () * bmP->Height (); i; i--) {
					for (j = bpp; j; j--, dataP++)
						if ((c = 255 - *dataP)) {
							c = (int) (*dataP * dScale);
							*dataP = (ubyte) ((c > 255) ? 255 : c);
							}
					if (bSkipAlpha)
						dataP++;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

int TGAInterpolate (CBitmap *bmP, int nScale)
{
	ubyte	*bufP, *destP, *srcP1, *srcP2;
	int	nSize, nFrameSize, nStride, nFrames, i, j;

if (nScale < 1)
	nScale = 1;
else if (nScale > 3)
	nScale = 3;
nScale = 1 << nScale;
nFrames = bmP->Height () / bmP->Width ();
nFrameSize = bmP->Width () * bmP->Width () * bmP->BPP ();
nSize = nFrameSize * nFrames * nScale;
if (!(bufP = new ubyte [nSize]))
	return 0;
bmP->SetHeight (bmP->Height () * nScale);
memset (bufP, 0, nSize);
for (destP = bufP, srcP1 = bmP->Buffer (), i = 0; i < nFrames; i++) {
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
bmP->DestroyBuffer ();
bmP->SetBuffer (bufP);
return nFrames;
}

//------------------------------------------------------------------------------

int TGAMakeSquare (CBitmap *bmP)
{
	ubyte	*bufP, *destP, *srcP;
	int	nSize, nFrameSize, nRowSize, nFrames, i, j, w, q;

nFrames = bmP->Height () / bmP->Width ();
if (nFrames < 4)
	return 0;
for (q = nFrames; q * q > nFrames; q >>= 1)
	;
if (q * q != nFrames)
	return 0;
w = bmP->Width ();
nFrameSize = w * w * bmP->BPP ();
nSize = nFrameSize * nFrames;
if (!(bufP = new ubyte [nSize]))
	return 0;
srcP = bmP->Buffer ();
nRowSize = w * bmP->BPP ();
for (destP = bufP, i = 0; i < nFrames; i++) {
	for (j = 0; j < w; j++) {
		destP = bufP + (i / q) * q * nFrameSize + j * q * nRowSize + (i % q) * nRowSize;
		memcpy (destP, srcP, nRowSize);
		srcP += nRowSize;
		}
	}
bmP->DestroyBuffer ();
bmP->SetBuffer (bufP);
bmP->SetWidth (q * w);
bmP->SetHeight (q * w);
return q;
}

//------------------------------------------------------------------------------

int ReadModelTGA (const char *pszFile, CBitmap *bmP, int bCustom)
{
	char			fn [FILENAME_LEN], fnBase [FILENAME_LEN], szShrunkFolder [FILENAME_LEN];
	int			nShrinkFactor = 1 << (3 - gameStates.render.nModelQuality);
	time_t		tBase, tShrunk;

if (!pszFile)
	return 1;
CFile::SplitPath (pszFile, NULL, fn, NULL);
if (nShrinkFactor > 1) {
	CFile	cf;
	sprintf (fnBase, "%s.tga", fn);
	sprintf (szShrunkFolder, "%s/%d", gameFolders.szModelCacheDir [bCustom], 512 / nShrinkFactor); 
	tBase = cf.Date (fnBase, gameFolders.szModelDir [bCustom], 0);
	tShrunk = cf.Date (fnBase, szShrunkFolder, 0);
	if ((tShrunk > tBase) && ReadTGA (fnBase, szShrunkFolder, bmP, -1, 1.0, 0, 0)) {
#if DBG
		bmP->SetName (fn);
#endif
		UseBitmapCache (bmP, (int) bmP->Height () * (int) bmP->RowSize ());
		return 1;
		}
	}
if (!ReadTGA (pszFile, gameFolders.szModelDir [bCustom], bmP, -1, 1.0, 0, 0))
	return 0;
UseBitmapCache (bmP, (int) bmP->Height () * (int) bmP->RowSize ());
if (gameStates.app.bCacheTextures && (nShrinkFactor > 1) && 
	 (bmP->Width () == 512) && ShrinkTGA (bmP, nShrinkFactor, nShrinkFactor, 1)) {
	tTgaHeader	h;
	CFile			cf;

	strcat (fn, ".tga");
	if (!cf.Open (fn, gameFolders.szModelDir [bCustom], "rb", 0))
		return 1;
	if (ReadTGAHeader (cf, &h, NULL))
		SaveTGA (fn, gameFolders.szModelCacheDir [bCustom], &h, bmP);
	cf.Close ();
	}
return 1;
}

//------------------------------------------------------------------------------

int CModelTextures::Read (int bCustom)
{
	CBitmap	*bmP;
	int		i;

for (i = 0; i < m_nBitmaps; i++) {
	if (!ReadModelTGA (m_names [i].Buffer (), bmP = m_bitmaps + i, bCustom))
		return 0;
	if (bmP->Buffer ()) {
		bmP = bmP->Override (-1);
		if (bmP->Frames ())
			bmP = bmP->CurFrame ();
		bmP->Bind (1);
		m_bitmaps [i].SetTeam (m_nTeam.Buffer () ? m_nTeam [i] : 0);
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void CModelTextures::Release (void)
{
	int		i;

if ((m_bitmaps.Buffer ()))
	for (i = 0; i < m_nBitmaps; i++)
		m_bitmaps [i].ReleaseTexture ();
}

//------------------------------------------------------------------------------

int CModelTextures::Bind (int bCustom)
{
	int		i;

if ((m_bitmaps.Buffer ()))
	for (i = 0; i < m_nBitmaps; i++) {
		if (!(m_bitmaps [i].Buffer () || Read (bCustom)))
			return 0;
		m_bitmaps [i].Bind (1);
		}
return 1;
}

//------------------------------------------------------------------------------

void CModelTextures::Destroy (void)
{
	int	i;

if (m_names.Buffer ()) {
	for (i = 0; i < m_nBitmaps; i++) 
		m_names [i].Destroy ();
if (m_bitmaps.Buffer ())
	for (i = 0; i < m_nBitmaps; i++) {
		UseBitmapCache (&m_bitmaps [i], -static_cast<int> (m_bitmaps [i].Size ()));
		m_bitmaps [i].Destroy ();
		}
	m_nTeam.Destroy ();
	m_names.Destroy ();
	m_bitmaps.Destroy ();
	m_nBitmaps = 0;
	}
}

//------------------------------------------------------------------------------

bool CModelTextures::Create (int nBitmaps)
{
if (nBitmaps <= 0)
	return false;
if (!(m_bitmaps.Create (nBitmaps)))
	return false;
if (!(m_names.Create (nBitmaps)))
	return false;
if (!(m_nTeam.Create (nBitmaps)))
	return false;
m_nBitmaps = nBitmaps;
m_bitmaps.Clear ();
m_names.Clear ();
m_nTeam.Clear ();
return true;
}

//------------------------------------------------------------------------------

