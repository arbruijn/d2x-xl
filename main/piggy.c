/* $Id: piggy.c,v 1.51 2004/01/08 19:02:53 schaffner Exp $ */
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

#ifdef RCS
static char rcsid [] = "$Id: piggy.c,v 1.51 2004/01/08 19:02:53 schaffner Exp $";
#endif

#ifdef _WIN32
#	include <windows.h>
#endif

#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "strutil.h"
#include "inferno.h"
#include "gr.h"
#include "u_mem.h"
#include "iff.h"
#include "mono.h"
#include "error.h"
#include "sounds.h"
#include "songs.h"
#include "bm.h"
#include "bmread.h"
#include "hash.h"
#include "args.h"
#include "palette.h"
#include "gamefont.h"
#include "rle.h"
#include "screens.h"
#include "piggy.h"
#include "gamemine.h"
#include "textures.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "cfile.h"
#include "newmenu.h"
#include "byteswap.h"
#include "findfile.h"
#include "makesig.h"
#include "effects.h"
#include "wall.h"
#include "weapon.h"
#include "error.h"
#include "grdef.h"
#include "gamepal.h"

//#define NO_DUMP_SOUNDS        1   //if set, dump bitmaps but not sounds

#ifdef _DEBUG
#	define PIGGY_MEM_QUOTA	4
#else
#	define PIGGY_MEM_QUOTA	8
#endif

#define DEFAULT_PIGFILE_REGISTERED      "groupa.pig"
#define DEFAULT_PIGFILE_SHAREWARE       "d2demo.pig"
#define DEFAULT_HAMFILE_REGISTERED      "descent2.ham"
#define DEFAULT_HAMFILE_SHAREWARE       "d2demo.ham"

#define DEFAULT_PIGFILE \
		  (gameStates.app.bDemoData ? DEFAULT_PIGFILE_SHAREWARE : DEFAULT_PIGFILE_REGISTERED)
#define DEFAULT_HAMFILE \
		  (gameStates.app.bDemoData ? DEFAULT_HAMFILE_SHAREWARE : DEFAULT_HAMFILE_REGISTERED)
#define DEFAULT_SNDFILE \
		  ((gameData.pig.tex.nHamFileVersion < 3) ? DEFAULT_HAMFILE_SHAREWARE : \
			(gameOpts->sound.digiSampleRate == SAMPLE_RATE_22K) ? "descent2.s22" : "descent2.s11")

#define MAC_ALIEN1_PIGSIZE      5013035
#define MAC_ALIEN2_PIGSIZE      4909916
#define MAC_FIRE_PIGSIZE        4969035
#define MAC_GROUPA_PIGSIZE      4929684 // also used for mac shareware
#define MAC_ICE_PIGSIZE         4923425
#define MAC_WATER_PIGSIZE       4832403

ubyte *SoundBits [2] = {NULL, NULL};

short *d1_tmap_nums = NULL;

static short d2OpaqueDoors [] = {
	440, 451, 463, 477, /*483,*/ 488, 
	500, 508, 523, 550, 556, 564, 572, 579, 585, 593, 536, 
	600, 608, 615, 628, 635, 642, 649, 664, /*672,*/ 687, 
	702, 717, 725, 731, 738, 745, 754, 763, 772, 780, 790,
	806, 817, 827, 838, 849, /*858,*/ 863, 871, 886,
	901,
	-1};

ubyte bogus_data [4096*4096];
grsBitmap bogus_bitmap;
ubyte bogus_bitmap_initialized=0;
tDigiSound bogusSound;

#define RLE_REMAP_MAX_INCREASE 132 /* is enough for d1 pc registered */

static int bLowMemory = 0;
static int bMustWriteHamFile = 0;
static int nBitmapFilesNew = 0;
static int nSoundFilesNew = 0;

static hashtable soundNames [2];
static SoundFile sounds [2][MAX_SOUND_FILES];
static int soundOffset [2][MAX_SOUND_FILES];

static hashtable bitmapNames [2];
unsigned int bitmapCacheUsed = 0;
unsigned int bitmapCacheSize = 0;
static int bitmapCacheNext [2] = {0, 0};
static int bitmapOffsets [2][MAX_BITMAP_FILES];

static ubyte *bitmapBits [2] = {NULL, NULL};

static sbyte bitmapFlags [2][MAX_BITMAP_FILES];

ubyte d1ColorMap [256];
ubyte *d1Palette = NULL;

#ifdef _DEBUG
#	define PIGGY_BUFFER_SIZE ((unsigned int) (512*1024*1024))
#else
#	define PIGGY_BUFFER_SIZE ((unsigned int) 0x7fffffff)
#endif
#define PIGGY_SMALL_BUFFER_SIZE (16*1024*1024)		// size of buffer when bLowMemory is set

#define DBM_FLAG_ABM    64 // animated bitmap
#define DBM_NUM_FRAMES  63

#define BM_FLAGS_TO_COPY (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT \
                         | BM_FLAG_NO_LIGHTING | BM_FLAG_RLE | BM_FLAG_RLE_BIG)

typedef struct DiskBitmapHeader {
	char name [8];
	ubyte dflags;           // bits 0-5 anim frame num, bit 6 abm flag
	ubyte width;            // low 8 bits here, 4 more bits in wh_extra
	ubyte height;           // low 8 bits here, 4 more bits in wh_extra
	ubyte wh_extra;         // bits 0-3 width, bits 4-7 height
	ubyte flags;
	ubyte bm_avgColor;
	int offset;
} __pack__ DiskBitmapHeader;

#define DISKBITMAPHEADER_D1_SIZE 17 // no wh_extra

typedef struct DiskSoundHeader {
	char name [8];
	int length;
	int data_length;
	int offset;
} __pack__ DiskSoundHeader;

int ReadHamFile ();
int ReadSoundFile ();

//---------------------------------------------------------------

int IsOpaqueDoor (int i)
{
	short	*p;

for (p = d2OpaqueDoors; *p >= 0; p++)
	if (i == gameData.pig.tex.pBmIndex [*p].index)
		return 1;
return 0;
}

//------------------------------------------------------------------------------

#if TEXTURE_COMPRESSION

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

int SaveS3TC (grsBitmap *bmP, char *pszFolder, char *pszFilename)
{
	CFILE		*fp;
	char		szFilename [FILENAME_LEN], szFolder [FILENAME_LEN];

if (!bmP->bm_compressed)
	return 0;
CFSplitPath (pszFilename, szFolder, szFilename + 1, NULL);
strcat (szFilename + 1, ".s3tc");
*szFilename = '\x02';	//don't search lib (hog) files
if (*szFolder)
	pszFolder = szFolder;
if (CFExist (szFilename, pszFolder, 0))
	return 1;
if (!(fp = CFOpen (szFilename + 1, pszFolder, "wb", 0)))
	return 0;
if ((CFWrite (&bmP->bm_props.w, sizeof (bmP->bm_props.w), 1, fp) != 1) ||
	 (CFWrite (&bmP->bm_props.h, sizeof (bmP->bm_props.h), 1, fp) != 1) ||
	 (CFWrite (&bmP->bm_format, sizeof (bmP->bm_format), 1, fp) != 1) ||
    (CFWrite (&bmP->bm_bufSize, sizeof (bmP->bm_bufSize), 1, fp) != 1) ||
    (CFWrite (bmP->bm_texBuf, bmP->bm_bufSize, 1, fp) != 1)) {
	CFClose (fp);
	return 0;
	}
return !CFClose (fp);
}

//------------------------------------------------------------------------------

int ReadS3TC (grsBitmap *bmP, char *pszFolder, char *pszFilename)
{
	CFILE		*fp;
	char		szFilename [FILENAME_LEN], szFolder [FILENAME_LEN];

if (!gameStates.ogl.bHaveTexCompression)
	return 0;
CFSplitPath (pszFilename, szFolder, szFilename, NULL);
if (!*szFilename)
	CFSplitPath (pszFilename, szFolder, szFilename, NULL);
strcat (szFilename, ".s3tc");
if (*szFolder)
	pszFolder = szFolder;
if (!(fp = CFOpen (szFilename, pszFolder, "rb", 0)))
	return 0;
if ((CFRead (&bmP->bm_props.w, sizeof (bmP->bm_props.w), 1, fp) != 1) ||
	 (CFRead (&bmP->bm_props.h, sizeof (bmP->bm_props.h), 1, fp) != 1) ||
	 (CFRead (&bmP->bm_format, sizeof (bmP->bm_format), 1, fp) != 1) ||
	 (CFRead (&bmP->bm_bufSize, sizeof (bmP->bm_bufSize), 1, fp) != 1)) {
	CFClose (fp);
	return 0;
	}
if (!(bmP->bm_texBuf = (ubyte *) D2_ALLOC (bmP->bm_bufSize))) {
	CFClose (fp);
	return 0;
	}
if (CFRead (bmP->bm_texBuf, bmP->bm_bufSize, 1, fp) != 1) {
	CFClose (fp);
	return 0;
	}
CFClose (fp);
bmP->bm_compressed = 1;
return 1;
}

#endif

//---------------------------------------------------------------

int ReadTGAImage (CFILE *fp, tTgaHeader *ph, grsBitmap *bmP, int alpha, 
						double brightness, int bGrayScale, int bReverse)
{
	int				i, j, n, nFrames, nBytes = ph->bits / 8;
	int				h = bmP->bm_props.h;
	int				w = bmP->bm_props.w;

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
			//p->alpha = (alpha < 0) ? 255 : alpha;
			}
		p -= 2 * w;
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
				if (0 && bShaderMerge) { 
					p->red =
					p->green =
					p->blue = 0;
					p->alpha = 1;
					}
				else
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
				if (0 && bShaderMerge) { 
					p->red =
					p->green =
					p->blue = 0;
					p->alpha = 1;
					}
				else
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
			}
		p -= 2 * w;
		}
	}	
return 1;
}

//---------------------------------------------------------------

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

//---------------------------------------------------------------

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

//---------------------------------------------------------------

int ShrinkTGA (grsBitmap *bmP, int xFactor, int yFactor, int bRealloc)
{
	int		xSrc, ySrc, xMax, yMax, xDest, yDest, x, y, w, h, i, nFactor2, nSuperTransp, bSuperTransp;
	int		bShaderMerge = gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk;
	int		bpp = bmP->bm_bpp;
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
	bitmapCacheUsed -= bmP->bm_props.h * bmP->bm_props.rowsize;
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
				if (bShaderMerge)
					bSuperTransp = (pSrc [3] == 1);
				else
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
			if (bShaderMerge) {
				pDest [0] = 
				pDest [1] = 
				pDest [2] = 0;
				pDest [3] = 1;
				}
			else {
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
				if (i == 3)	 {//alpha treatment round to 0 or 255 depending on the average alpha
					if (*pDest < 128)
						*pDest = 0;
					else if (*pDest > 127)
						*pDest = 255;
					}
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
	bitmapCacheUsed += bmP->bm_props.h * bmP->bm_props.rowsize;
return 1;
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

#if 0//def FAST_FILE_IO /*disabled for a reason!*/

#define DiskBitmapHeaderRead(dbh, fp) CFRead (dbh, sizeof (DiskBitmapHeader), 1, fp)
#define DiskSoundHeader_read(dsh, fp) CFRead (dsh, sizeof (DiskSoundHeader), 1, fp)

#else

//------------------------------------------------------------------------------
/*
 * reads a DiskBitmapHeader structure from a CFILE
 */
void DiskBitmapHeaderRead (DiskBitmapHeader *dbh, CFILE *fp)
{
	CFRead (dbh->name, 8, 1, fp);
	dbh->dflags = CFReadByte (fp);
	dbh->width = CFReadByte (fp);
	dbh->height = CFReadByte (fp);
	dbh->wh_extra = CFReadByte (fp);
	dbh->flags = CFReadByte (fp);
	dbh->bm_avgColor = CFReadByte (fp);
	dbh->offset = CFReadInt (fp);
}

//------------------------------------------------------------------------------
/*
 * reads a DiskSoundHeader structure from a CFILE
 */
void DiskSoundHeader_read (DiskSoundHeader *dsh, CFILE *fp)
{
	CFRead (dsh->name, 8, 1, fp);
	dsh->length = CFReadInt (fp);
	dsh->data_length = CFReadInt (fp);
	dsh->offset = CFReadInt (fp);
}
#endif // FAST_FILE_IO

//------------------------------------------------------------------------------
/*
 * reads a descent 1 DiskBitmapHeader structure from a CFILE
 */
void DiskBitmapHeader_d1_read (DiskBitmapHeader *dbh, CFILE *fp)
{
	CFRead (dbh->name, 8, 1, fp);
	dbh->dflags = CFReadByte (fp);
	dbh->width = CFReadByte (fp);
	dbh->height = CFReadByte (fp);
	dbh->wh_extra = 0;
	dbh->flags = CFReadByte (fp);
	dbh->bm_avgColor = CFReadByte (fp);
	dbh->offset = CFReadInt (fp);
}

//------------------------------------------------------------------------------

ubyte BigPig = 0;

int PiggyIsSubstitutableBitmap (char * name, char * subst_name);

#ifdef EDITOR
void PiggyWritePigFile (char *filename);
static void write_int (int i,FILE *file);
#endif

void swap_0_255 (grsBitmap *bmp)
{
	int i;
	ubyte	*p;

	for (i = bmp->bm_props.h * bmp->bm_props.w, p = bmp->bm_texBuf; i; i--, p++) {
		if (!*p)
			*p = 255;
		else if (*p == 255)
			*p = 0;
	}
}

//------------------------------------------------------------------------------

tBitmapIndex PiggyRegisterBitmap (grsBitmap *bmP, char *name, int in_file)
{
	tBitmapIndex temp;
	Assert (gameData.pig.tex.nBitmaps [gameStates.app.bD1Data] < MAX_BITMAP_FILES);

temp.index = gameData.pig.tex.nBitmaps [gameStates.app.bD1Data];
if (!in_file) {
#ifdef EDITOR
	if (FindArg ("-macdata"))
		swap_0_255 (bmP);
#endif
	if (!BigPig)  
		gr_bitmap_rle_compress (bmP);
	nBitmapFilesNew++;
	}
strncpy (gameData.pig.tex.pBitmapFiles [gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]].name, name, 12);
hashtable_insert (bitmapNames + gameStates.app.bD1Mission, gameData.pig.tex.pBitmapFiles[gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]].name, gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]);
gameData.pig.tex.pBitmaps [gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]] = *bmP;
if (!in_file) {
	bitmapOffsets [gameStates.app.bD1Data][gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]] = 0;
	bitmapFlags [gameStates.app.bD1Data][gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]] = bmP->bm_props.flags;
	}
gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]++;
return temp;
}

//------------------------------------------------------------------------------

void PiggyInitSound (void)
{
memset (gameData.pig.snd.sounds, 0, sizeof (gameData.pig.snd.sounds));
}

//------------------------------------------------------------------------------

int PiggyRegisterSound (tDigiSound * snd, char * name, int in_file)
{
	int i;

Assert (gameData.pig.snd.nSoundFiles [gameStates.app.bD1Data] < MAX_SOUND_FILES);

strncpy (sounds [gameStates.app.bD1Data][gameData.pig.snd.nSoundFiles [gameStates.app.bD1Data]].name, name, 12);
hashtable_insert (&soundNames [gameStates.app.bD1Data], 
						sounds [gameStates.app.bD1Data][gameData.pig.snd.nSoundFiles [gameStates.app.bD1Data]].name, 
						gameData.pig.snd.nSoundFiles [gameStates.app.bD1Data]);
gameData.pig.snd.pSounds[gameData.pig.snd.nSoundFiles [gameStates.app.bD1Data]] = *snd;
if (!in_file) {
	soundOffset [gameStates.app.bD1Data][gameData.pig.snd.nSoundFiles [gameStates.app.bD1Data]] = 0;       
}
i = gameData.pig.snd.nSoundFiles [gameStates.app.bD1Data];
if (!in_file)
	nSoundFilesNew++;
gameData.pig.snd.nSoundFiles [gameStates.app.bD1Data]++;
return i;
}

//------------------------------------------------------------------------------

tBitmapIndex PiggyFindBitmap (char * name, int bD1Data)   
{
	tBitmapIndex bmp;
	int i;
	char *t;

	bmp.index = 0;

	if ((t=strchr (name,'#'))!=NULL)
		*t=0;

	for (i=0;i<gameData.pig.tex.nAliases;i++)
		if (stricmp (name,gameData.pig.tex.aliases [i].alias_name)==0) {
			if (t) {                //extra stuff for ABMs
				static char temp [SHORT_FILENAME_LEN];
				_splitpath (gameData.pig.tex.aliases [i].file_name, NULL, NULL, temp, NULL);
				name = temp;
				strcat (name,"#");
				strcat (name,t+1);
			}
			else
				name=gameData.pig.tex.aliases [i].file_name; 
			break;
		}

	if (t)
		*t = '#';

	i = hashtable_search (bitmapNames + bD1Data, name);
	Assert (i != 0);
	if (i < 0)
		return bmp;

	bmp.index = i;
	return bmp;
}

//------------------------------------------------------------------------------

int PiggyFindSound (char * name)     
{
	int i;

	i = hashtable_search (&soundNames [gameStates.app.bD1Data], name);

	if (i < 0)
		return 255;

	return i;
}

//------------------------------------------------------------------------------

CFILE * piggyFP [2] = {NULL, NULL};

char szCurrentPigFile [2][SHORT_FILENAME_LEN] = {"",""};

void PiggyCloseFile (void)
{
	int	i;

for (i = 0; i < 2; i++)
	if (piggyFP [i]) {
		CFClose (piggyFP [i]);
		piggyFP [i] = NULL;
		szCurrentPigFile [i][0] = 0;
		}
}

//------------------------------------------------------------------------------

int bPigFileInitialized=0;

#define PIGFILE_ID              MAKE_SIG ('G','I','P','P') //PPIG
#define PIGFILE_VERSION         2

extern char CDROM_dir [];

int request_cd (void);



//------------------------------------------------------------------------------
//copies a pigfile from the CD to the current dir
//retuns file handle of new pig
CFILE *CopyPigFileFromCD (char *filename)
{
	char name [80];
	FFS ffs;
	int ret;

	return CFOpen (filename, gameFolders.szDataDir, "rb", 0);
	ShowBoxedMessage ("Copying bitmap data from CD...");
	GrPaletteStepLoad (NULL);    //I don't think this line is really needed

	//First, delete all PIG files currently in the directory

	if (!FFF ("*.pig", &ffs, 0)) {
		do {
			CFDelete (ffs.name, "");
		} while (!FFN  (&ffs, 0));
		FFC (&ffs);
	}

	//Now, copy over new pig

	SongsStopRedbook ();           //so we can read off the cd

	//new code to unarj file
	strcpy (name,CDROM_dir);
	strcat (name,"descent2.sow");

	do {
//		ret = unarj_specific_file (name,filename,filename);
// DPH:FIXME

		ret = !EXIT_SUCCESS;

		if (ret != EXIT_SUCCESS) {

			//delete file, so we don't leave partial file
			CFDelete (filename, "");

			if (request_cd () == -1)
				//NOTE LINK TO ABOVE IF
				Error ("Cannot load file <%s> from CD",filename);
		}

	} while (ret != EXIT_SUCCESS);

	return CFOpen (filename, gameFolders.szDataDir, "rb", 0);
}

//------------------------------------------------------------------------------

int PiggyInitMemory (void)
{
	static	int bMemInited = 0;

if (bMemInited)
	return 1;
#ifdef EDITOR
bitmapCacheSize = nDataSize + (nDataSize / 10);   //extra mem for new bitmaps
Assert (bitmapCacheSize > 0);
#elif defined (WIN32)
	{
	MEMORYSTATUS	memStat;
	GlobalMemoryStatus (&memStat);
	bitmapCacheSize = (int) (memStat.dwAvailPhys / 10) * PIGGY_MEM_QUOTA;
	}
#else
bitmapCacheSize = PIGGY_BUFFER_SIZE;
if (bLowMemory)
	bitmapCacheSize = PIGGY_SMALL_BUFFER_SIZE;
// the following loop will scale bitmap memory down by 20% for each iteration
// until memory could be allocated, and then once more to leave enough memory
// for other parts of the program
for (;;) {
	if ((bitmapBits [0] = D2_ALLOC (bitmapCacheSize))) {
		D2_FREE (bitmapBits [0]);
		break;
		}
	bitmapCacheSize = (bitmapCacheSize / 10) * PIGGY_MEM_QUOTA;
	if (bitmapCacheSize < PIGGY_SMALL_BUFFER_SIZE) {
		Error ("Not enough memory to load D2 bitmaps\n");
		return 0;
		break;
		}
	}
#endif
return bMemInited = 1;
}

//------------------------------------------------------------------------------
//initialize a pigfile, reading headers
//returns the size of all the bitmap data
void PiggyInitPigFile (char *filename)
{
	char					temp_name [16];
	char					temp_name_read [16];
	char					szPigName [FILENAME_LEN];
	int					nHeaderSize, nBitmapNum, nDataSize, nDataStart, i;
	grsBitmap			bmTemp;
	DiskBitmapHeader	bmh;

PiggyCloseFile ();             //close old pig if still open
strcpy (szPigName, filename);
//rename pigfile for shareware
if (!stricmp (DEFAULT_PIGFILE, DEFAULT_PIGFILE_SHAREWARE) && 
	 !CFExist (szPigName, gameFolders.szDataDir,0))
	strcpy (szPigName, DEFAULT_PIGFILE_SHAREWARE);
strlwr (szPigName);
piggyFP [gameStates.app.bD1Data] = CFOpen (szPigName, gameFolders.szDataDir, "rb", 0);
if (!piggyFP [gameStates.app.bD1Data]) {
#ifdef EDITOR
	return;         //if editor, ok to not have pig, because we'll build one
#else
	piggyFP [gameStates.app.bD1Data] = CopyPigFileFromCD (szPigName);
#endif
	}
if (piggyFP [gameStates.app.bD1Data]) {                        //make sure pig is valid nType file & is up-to-date
	int pig_id = CFReadInt (piggyFP [gameStates.app.bD1Data]);
	int pig_version = CFReadInt (piggyFP [gameStates.app.bD1Data]);
	if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
		CFClose (piggyFP [gameStates.app.bD1Data]);              //out of date pig
		piggyFP [gameStates.app.bD1Data] = NULL;                 //..so pretend it's not here
		}
	}
if (!piggyFP [gameStates.app.bD1Data]) {
#ifndef EDITOR
	Error ("Cannot load required file <%s>", szPigName);
#endif
	return;
	}
strncpy (szCurrentPigFile [0], szPigName, sizeof (szCurrentPigFile [0]));
nBitmapNum = CFReadInt (piggyFP [gameStates.app.bD1Data]);
nHeaderSize = nBitmapNum * sizeof (DiskBitmapHeader);
nDataStart = nHeaderSize + CFTell (piggyFP [gameStates.app.bD1Data]);
nDataSize = CFLength (piggyFP [gameStates.app.bD1Data], 0) - nDataStart;
gameData.pig.tex.nBitmaps [0] = 1;
for (i = 0; i < nBitmapNum; i++) {
	DiskBitmapHeaderRead (&bmh, piggyFP [gameStates.app.bD1Data]);
	memcpy (temp_name_read, bmh.name, 8);
	temp_name_read [8] = 0;
	if (bmh.dflags & DBM_FLAG_ABM)        
		sprintf (temp_name, "%s#%d", temp_name_read, bmh.dflags & DBM_NUM_FRAMES);
	else
		strcpy (temp_name, temp_name_read);
	memset (&bmTemp, 0, sizeof (grsBitmap));
	bmTemp.bm_props.w = bmTemp.bm_props.rowsize = bmh.width + ((short) (bmh.wh_extra & 0x0f) << 8);
	bmTemp.bm_props.h = bmh.height + ((short) (bmh.wh_extra & 0xf0) << 4);
	bmTemp.bm_props.flags |= BM_FLAG_PAGED_OUT;
	bmTemp.bm_avgColor = bmh.bm_avgColor;
	bitmapFlags [0][i+1] = bmh.flags & BM_FLAGS_TO_COPY;
	bitmapOffsets [0][i+1] = bmh.offset + nDataStart;
	Assert ((i+1) == gameData.pig.tex.nBitmaps [0]);
	PiggyRegisterBitmap (&bmTemp, temp_name, 1);
	}
bPigFileInitialized = 1;
}

//------------------------------------------------------------------------------

#define SHORT_FILENAME_LEN 13
#define MAX_BITMAPS_PER_BRUSH 30

extern int ComputeAvgPixel (grsBitmap *new);

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile (char *pigname)
{
	int i;
	char temp_name [16];
	char temp_name_read [16];
	grsBitmap bmTemp;
	DiskBitmapHeader bmh;
	int nHeaderSize, nBitmapNum, nDataSize, nDataStart;
	int must_rewrite_pig = 0;

	strlwr (pigname);

	//rename pigfile for shareware
	if (stricmp (DEFAULT_PIGFILE, DEFAULT_PIGFILE_SHAREWARE) == 0 && !CFExist (pigname, gameFolders.szDataDir,0))
		pigname = DEFAULT_PIGFILE_SHAREWARE;

	if (strnicmp (szCurrentPigFile [0], pigname, sizeof (szCurrentPigFile)) == 0) // no need to reload: no bitmaps were altered
		return;

	if (!bPigFileInitialized) {                     //have we ever opened a pigfile?
		PiggyInitPigFile (pigname);            //..no, so do initialization stuff
		return;
	}
	else
		PiggyCloseFile ();             //close old pig if still open
	bitmapCacheNext [0] = 0;            //D2_FREE up cache
	strncpy (szCurrentPigFile[0],pigname,sizeof (szCurrentPigFile));
	piggyFP [0] = CFOpen (pigname, gameFolders.szDataDir, "rb", 0);

	#ifndef EDITOR
	if (!piggyFP [0])
		piggyFP [0] = CopyPigFileFromCD (pigname);
	#endif
	if (piggyFP [0]) {  //make sure pig is valid nType file & is up-to-date
		int pig_id,pig_version;

		pig_id = CFReadInt (piggyFP [0]);
		pig_version = CFReadInt (piggyFP [0]);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
			CFClose (piggyFP [0]);              //out of date pig
			piggyFP [0] = NULL;                        //..so pretend it's not here
		}
	}

#ifndef EDITOR
	if (!piggyFP [0])
		Error ("Cannot open correct version of <%s>", pigname);
#endif
	if (piggyFP [0]) {
		nBitmapNum = CFReadInt (piggyFP [0]);
		nHeaderSize = nBitmapNum * sizeof (DiskBitmapHeader);
		nDataStart = nHeaderSize + CFTell (piggyFP [0]);
		nDataSize = CFLength (piggyFP [0],0) - nDataStart;
		for (i = 1; i <= nBitmapNum; i++) {
			DiskBitmapHeaderRead (&bmh, piggyFP [0]);
			memcpy (temp_name_read, bmh.name, 8);
			temp_name_read [8] = 0;
			if (bmh.dflags & DBM_FLAG_ABM)        
				sprintf (temp_name, "%s#%d", temp_name_read, bmh.dflags & DBM_NUM_FRAMES);
			else
				strcpy (temp_name, temp_name_read);
			//Make sure name matches
			if (strcmp (temp_name,gameData.pig.tex.bitmapFiles [0][i].name)) {
				//Int3 ();       //this pig is out of date.  Delete it
				must_rewrite_pig=1;
			}
			strcpy (gameData.pig.tex.bitmapFiles [0][i].name,temp_name);
			memset (&bmTemp, 0, sizeof (grsBitmap));
			bmTemp.bm_props.w = bmTemp.bm_props.rowsize = bmh.width + ((short) (bmh.wh_extra&0x0f)<<8);
			bmTemp.bm_props.h = bmh.height + ((short) (bmh.wh_extra & 0xf0)<<4);
			bmTemp.bm_props.flags |= BM_FLAG_PAGED_OUT;
			bmTemp.bm_avgColor = bmh.bm_avgColor;
			bmTemp.bm_texBuf = D2_ALLOC (bmTemp.bm_props.w * bmTemp.bm_props.h);
			bitmapCacheUsed += bmTemp.bm_props.w * bmTemp.bm_props.h;
			bitmapFlags [0][i] = bmh.flags & BM_FLAGS_TO_COPY;
			bitmapOffsets [0][i] = bmh.offset + nDataStart;
			gameData.pig.tex.bitmaps [0][i] = bmTemp;
		}
	}
	else
		nBitmapNum = 0;          //no pigfile, so no bitmaps

#ifndef EDITOR
	Assert (nBitmapNum == gameData.pig.tex.nBitmaps [0]-1);
#else
	if (must_rewrite_pig || (nBitmapNum < gameData.pig.tex.nBitmaps [0]-1)) {
		int size;
		//re-read the bitmaps that aren't in this pig
		for (i = nBitmapNum + 1; i < gameData.pig.tex.nBitmaps [0]; i++) {
			char *p;
			p = strchr (gameData.pig.tex.bitmapFiles [0][i].name, '#');
			if (p) {   // this is an ABM == animated bitmap
				char abmname [SHORT_FILENAME_LEN];
				int fnum;
				grsBitmap * bm [MAX_BITMAPS_PER_BRUSH];
				int iff_error;          //reference parm to avoid warning message
				ubyte newpal [768];
				char basename [SHORT_FILENAME_LEN];
				int nframes;
				strcpy (basename, gameData.pig.tex.bitmapFiles [0][i].name);
				basename [p-gameData.pig.tex.bitmapFiles [0][i].name] = 0;  //cut off "#nn" part
				sprintf (abmname, "%s.abm", basename);
				iff_error = iff_read_animbrush (abmname,bm,MAX_BITMAPS_PER_BRUSH,&nframes,newpal);
				if (iff_error != IFF_NO_ERROR) {
#if TRACE				
					con_printf (1,"File %s - IFF error: %s",abmname,iff_errormsg (iff_error));
#endif
					Error ("File %s - IFF error: %s",abmname,iff_errormsg (iff_error);
				}
				for (fnum=0;fnum<nframes; fnum++) {
					char tempname [20];
					int SuperX;
					sprintf (tempname, "%s#%d", basename, fnum);
					//SuperX = (gameData.pig.tex.bitmaps [i+fnum].bm_props.flags&BM_FLAG_SUPER_TRANSPARENT)?254:-1;
					SuperX = (bitmapFlags [i + fnum] & BM_FLAG_SUPER_TRANSPARENT) ? 254 : -1;
					//above makes assumption that supertransparent color is 254
					if (iff_has_transparency)
						GrRemapBitmapGood (bm [fnum], newpal, iff_transparent_color, SuperX);
					else
						GrRemapBitmapGood (bm [fnum], newpal, -1, SuperX);
					bm [fnum]->bm_avgColor = ComputeAvgPixel (bm [fnum]);

#ifdef EDITOR
					if (FindArg ("-macdata"))
						swap_0_255 (bm [fnum]);
#endif
					if (!BigPig) gr_bitmap_rle_compress (bm [fnum]);

					if (bm [fnum]->bm_props.flags & BM_FLAG_RLE)
						size = * ((int *) bm [fnum]->bm_texBuf);
					else
						size = bm [fnum]->bm_props.w * bm [fnum]->bm_props.h;
					gameData.pig.tex.bitmaps [0][i+fnum] = *bm [fnum];
					D2_FREE (bm [fnum]);
				}
				i += nframes - 1;         //filled in multiple bitmaps
			}
			else {          //this is a BBM
				grsBitmap * newBm;
				ubyte newpal [256*3];
				int iff_error;
				char bbmname [SHORT_FILENAME_LEN];
				int SuperX;
				MALLOC (newBm, grsBitmap, 1);
				sprintf (bbmname, "%s.bbm", gameData.pig.tex.bitmapFiles [0][i].name);
				iff_error = iff_read_bitmap (bbmname,newBm,BM_LINEAR,newpal);
				newBm->bm_handle=0;
				if (iff_error != IFF_NO_ERROR) {
#if TRACE				
					con_printf (1, "File %s - IFF error: %s",bbmname,iff_errormsg (iff_error));
#endif
					Error ("File %s - IFF error: %s",bbmname,iff_errormsg (iff_error);
				}
				SuperX = (bitmapFlags [i]&BM_FLAG_SUPER_TRANSPARENT)?254:-1;
				//above makes assumption that supertransparent color is 254
				if (iff_has_transparency)
					GrRemapBitmapGood (newBm, newpal, iff_transparent_color, SuperX);
				else
					GrRemapBitmapGood (newBm, newpal, -1, SuperX);
				newBm->bm_avgColor = ComputeAvgPixel (newBm);
#ifdef EDITOR
				if (FindArg ("-macdata"))
					swap_0_255 (newBm);
#endif
				if (!BigPig)  gr_bitmap_rle_compress (newBm);
				if (newBm->bm_props.flags & BM_FLAG_RLE)
					size = * ((int *) newBm->bm_texBuf);
				else
					size = newBm->bm_props.w * newBm->bm_props.h;
				gameData.pig.tex.bitmaps [0][i] = *newBm;
				D2_FREE (newBm);
			}
		}

		//@@Dont' do these things which are done when writing
		//@@for (i=0; i < gameData.pig.tex.nBitmaps; i++) {
		//@@    tBitmapIndex bi;
		//@@    bi.index = i;
		//@@    PIGGY_PAGE_IN (bi);
		//@@}
		//@@
		//@@PiggyCloseFile ();
		PiggyWritePigFile (pigname);
		szCurrentPigFile [0] = 0;                 //say no pig, to force reload
		piggy_new_pigfile (pigname);             //read in just-generated pig
	}
	#endif  //ifdef EDITOR
}

//------------------------------------------------------------------------------

int LoadSounds (CFILE *snd_fp, int nSoundNum, int sound_start)
{
	DiskSoundHeader	sndh;
	tDigiSound			tempSound;
	int					i;
	int					sbytes = 0;
	int 					nHeaderSize = nSoundNum * sizeof (DiskSoundHeader);
	char					temp_name_read [16];


/*---*/LogErr ("      Loading sound data (%d sounds)\n", nSoundNum);
CFSeek (snd_fp, sound_start, SEEK_SET);
for (i=0; i<nSoundNum; i++) {
		DiskSoundHeader_read (&sndh, snd_fp);
		//size -= sizeof (DiskSoundHeader);
		tempSound.length = sndh.length;
		tempSound.data = (ubyte *) (size_t) (sndh.offset + nHeaderSize + sound_start);
		soundOffset [gameStates.app.bD1Data][gameData.pig.snd.nSoundFiles [gameStates.app.bD1Data]] = sndh.offset + nHeaderSize + sound_start;
		memcpy (temp_name_read, sndh.name, 8);
		temp_name_read [8] = 0;
		PiggyRegisterSound (&tempSound, temp_name_read, 1);
		sbytes += sndh.length;
	}

	SoundBits [gameStates.app.bD1Data] = D2_ALLOC (sbytes + 16);
	if (SoundBits [gameStates.app.bD1Data] == NULL)
		Error ("Not enough memory to load sounds\n");

#if TRACE				
	con_printf (CON_VERBOSE, "\nBitmapNum: %d KB   Sounds [gameStates.app.bD1Data]: %d KB\n", 
					bitmapCacheSize / 1024, sbytes / 1024);
#endif
return 1;
}

//------------------------------------------------------------------------------

#define HAMFILE_ID              MAKE_SIG ('!','M','A','H') //HAM!
#define HAMFILE_VERSION 3
//version 1 -> 2:  save marker_model_num
//version 2 -> 3:  removed sound files

#define SNDFILE_ID              MAKE_SIG ('D','N','S','D') //DSND
#define SNDFILE_VERSION 1

int ReadHamFile ()
{
	CFILE * ham_fp = NULL;
#if 1
	char D1_piggy_fn [FILENAME_LEN];
#endif
	int ham_id;
	int sound_offset = 0;

	ham_fp = CFOpen (DEFAULT_HAMFILE, gameFolders.szDataDir, "rb", 0);

	if (ham_fp == NULL) {
		bMustWriteHamFile = 1;
		return 0;
	}

	//make sure ham is valid nType file & is up-to-date
	ham_id = CFReadInt (ham_fp);
	gameData.pig.tex.nHamFileVersion = CFReadInt (ham_fp);
	if (ham_id != HAMFILE_ID)
		Error ("Cannot open ham file %s\n", DEFAULT_HAMFILE);
#if 0
	if (ham_id != HAMFILE_ID || gameData.pig.tex.nHamFileVersion != HAMFILE_VERSION) {
		bMustWriteHamFile = 1;
		CFClose (ham_fp);						//out of date ham
		return 0;
	}
#endif

	if (gameData.pig.tex.nHamFileVersion < 3) // hamfile contains sound info
		sound_offset = CFReadInt (ham_fp);

	#ifndef EDITOR
	{
		//int i;
		BMReadAll (ham_fp);
/*---*/LogErr ("      Loading bitmap index translation table\n");
		CFRead (gameData.pig.tex.bitmapXlat, sizeof (ushort)*MAX_BITMAP_FILES, 1, ham_fp);
		// no swap here?
		//for (i = 0; i < MAX_BITMAP_FILES; i++) {
			//gameData.pig.tex.bitmapXlat [i] = INTEL_SHORT (gameData.pig.tex.bitmapXlat [i]);
			////printf ("gameData.pig.tex.bitmapXlat [%d] = %d\n", i, gameData.pig.tex.bitmapXlat [i]);
		//}
	}
	#endif

	if (gameData.pig.tex.nHamFileVersion < 3) {
		int nSoundNum;
		int sound_start;

		CFSeek (ham_fp, sound_offset, SEEK_SET);
		nSoundNum = CFReadInt (ham_fp);
		sound_start = CFTell (ham_fp);
/*---*/LogErr ("      Loading %d sounds\n", nSoundNum);
		LoadSounds (ham_fp, nSoundNum, sound_start);
	}

	CFClose (ham_fp);
#if 1
/*---*/LogErr ("      Looking for Descent 1 data files\n");
		strcpy (D1_piggy_fn, "descent.pig");
		if (!piggyFP [1])
			piggyFP [1] = CFOpen (D1_piggy_fn, gameFolders.szDataDir, "rb", 0);
		if (piggyFP [1]) {
			gameStates.app.bHaveD1Data = 1;
/*---*/LogErr ("      Loading Descent 1 data\n");
			BMReadGameDataD1 (piggyFP [1]);
			//CFClose (piggyFP);
			}
#else
		strcpy (D1_piggy_fn, "descent.pig");
		piggyFP = CFOpen (D1_piggy_fn, gameFolders.szDataDir, "rb", 0);
		if (piggyFP != NULL) {
			BMReadWeaponInfoD1 (piggyFP);
			CFClose (piggyFP);
			}
		else {
			int	i;

			for (i = 0; i < D1_MAX_WEAPON_TYPES; i++)
				memcpy (gameData.weapons.infoD1 [i].strength, 
						  gameData.weapons.info [i].strength, 
						  sizeof (gameData.weapons.infoD1 [i].strength));
			}
#endif

	return 1;

}

//------------------------------------------------------------------------------

int ReadSoundFile ()
{
	CFILE * snd_fp = NULL;
	int snd_id,snd_version;
	int nSoundNum;
	int sound_start;
	int size, length;

	snd_fp = CFOpen (DEFAULT_SNDFILE, gameFolders.szDataDir, "rb", 0);
	
	if (snd_fp == NULL)
		return 0;

	//make sure soundfile is valid nType file & is up-to-date
	snd_id = CFReadInt (snd_fp);
	snd_version = CFReadInt (snd_fp);
	if (snd_id != SNDFILE_ID || snd_version != SNDFILE_VERSION) {
		CFClose (snd_fp);						//out of date sound file
		return 0;
	}

	nSoundNum = CFReadInt (snd_fp);
	sound_start = CFTell (snd_fp);
	size = CFLength (snd_fp,0) - sound_start;
	length = size;
//	PiggyReadSounds (snd_fp);
	LoadSounds (snd_fp, nSoundNum, sound_start);
	CFClose (snd_fp);
	return 1;
}

//------------------------------------------------------------------------------

int PiggyInit (void)
{
	int ham_ok=0,snd_ok=0;
	int i;

/*---*/LogErr ("   Initializing hash tables\n");
	hashtable_init (bitmapNames, MAX_BITMAP_FILES);
	hashtable_init (bitmapNames + 1, D1_MAX_BITMAP_FILES);
	hashtable_init (soundNames, MAX_SOUND_FILES);
	hashtable_init (soundNames + 1, MAX_SOUND_FILES);

/*---*/LogErr ("   Initializing sound data (%d sounds)\n", MAX_SOUND_FILES);
	for (i=0; i<MAX_SOUND_FILES; i++)	{
		gameData.pig.snd.sounds [0][i].length = 0;
		gameData.pig.snd.sounds [0][i].data = NULL;
		soundOffset [0][i] = 0;
	}
/*---*/LogErr ("   Initializing bitmap index (%d indices)\n", MAX_BITMAP_FILES);
	for (i = 0; i < MAX_BITMAP_FILES; i++)     
		gameData.pig.tex.bitmapXlat [i] = i;

	if (!bogus_bitmap_initialized) {
		int i;
		ubyte c;
/*---*/LogErr ("   Initializing placeholder bitmap\n");
		bogus_bitmap_initialized = 1;
		memset (&bogus_bitmap, 0, sizeof (grsBitmap));
		bogus_bitmap.bm_props.w = 
		bogus_bitmap.bm_props.h = 
		bogus_bitmap.bm_props.rowsize = 64;
		bogus_bitmap.bm_texBuf = bogus_data;
		bogus_bitmap.bm_palette = gamePalette;
		c = GrFindClosestColor (gamePalette, 0, 0, 63);
		memset (bogus_data, c, 4096);
		c = GrFindClosestColor (gamePalette, 63, 0, 0);
		// Make a big red X !
		for (i=0; i<1024; i++) {
			bogus_data [i * 1024 + i] = c;
			bogus_data [i * 1024 + (1023 - i)] = c;
			}
		PiggyRegisterBitmap (&bogus_bitmap, "bogus", 1);
		bogusSound.length = 1024*1024;
		bogusSound.data = bogus_data;
		bitmapOffsets [0][0] =
		bitmapOffsets [1][0] = 0;
	}

	if (FindArg ("-bigpig"))
		BigPig = 1;

	if (FindArg ("-lowmem"))
		bLowMemory = 1;

	if (FindArg ("-nolowmem"))
		bLowMemory = 0;

	if (bLowMemory)
		gameStates.sound.digi.bLoMem = 1;
#if 0
	WIN (DDGRLOCK (dd_grd_curcanv));
		GrSetCurFont (SMALL_FONT);
		GrSetFontColorRGBi (DKGRAY_RGBA, 1, 0, 0);
		GrPrintF (0x8000, grdCurCanv->cv_h-20, "%s...", TXT_LOADING_DATA);
		GrUpdate (0);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
#endif
/*---*/LogErr ("   Loading game data\n");
#if 1 //def EDITOR //need for d1 mission briefings
	PiggyInitPigFile (DEFAULT_PIGFILE);
#endif
/*---*/LogErr ("   Loading main ham file\n");
	snd_ok = ham_ok = ReadHamFile ();

	if (gameData.pig.tex.nHamFileVersion >= 3) {
/*---*/LogErr ("   Loading sound file\n");
		snd_ok = ReadSoundFile ();
		}
	if (gameStates.app.bFixModels)
		gameStates.app.bFixModels = gameStates.app.bDemoData ? 0 : LoadRobotReplacements ("d2x-xl", 0, 1) > 0;
	atexit (PiggyClose);
	return (ham_ok && snd_ok);               //read ok
}

//------------------------------------------------------------------------------

int PiggyIsNeeded (int nSound)
{
	int i;

	if (!gameStates.sound.digi.bLoMem) return 1;

	for (i=0; i<MAX_SOUNDS; i++) {
		if ((AltSounds [gameStates.app.bD1Data][i] < 255) && (Sounds [gameStates.app.bD1Data] [AltSounds [gameStates.app.bD1Data][i]] == nSound))
			return 1;
	}
	return 0;
}

//------------------------------------------------------------------------------

void PiggyReadSounds (void)
{
	CFILE * fp = NULL;
	ubyte * ptr;
	int i, sbytes;

	ptr = SoundBits [gameStates.app.bD1Data];
	sbytes = 0;

	fp = CFOpen (gameStates.app.bD1Mission ? "descent.pig" : DEFAULT_SNDFILE, gameFolders.szDataDir, "rb", 0);
	if (fp == NULL)
		return;
	for (i=0; i<gameData.pig.snd.nSoundFiles [gameStates.app.bD1Data]; i++) {
		tDigiSound *snd = gameData.pig.snd.pSounds + i;
		if (soundOffset [gameStates.app.bD1Data][i] > 0) {
			if (PiggyIsNeeded (i)) {
				CFSeek (fp, soundOffset [gameStates.app.bD1Data][i], SEEK_SET);

				// Read in the sound data!!!
				snd->data = ptr;
				ptr += snd->length;
				sbytes += snd->length;
				CFRead (snd->data, snd->length, 1, fp);
			}
			else
				snd->data = (ubyte *) -1;
		}
	}

	CFClose (fp);

#if TRACE				
	con_printf (CON_VERBOSE, "\nActual Sound usage: %d KB\n", sbytes/1024);
#endif
}

//------------------------------------------------------------------------------

extern int nDescentCriticalError;
extern unsigned descent_critical_deverror;
extern unsigned descent_critical_errcode;

char * crit_errors [13] = { "Write Protected", "Unknown Unit", "Drive Not Ready", "Unknown Command", "CRC Error", \
"Bad struct length", "Seek Error", "Unknown media nType", "Sector not found", "Printer out of paper", "Write Fault", \
"Read fault", "General Failure" };

void PiggyCriticalError ()
{
	grs_canvas * save_canv;
	grs_font * save_font;
	int i;
	save_canv = grdCurCanv;
	save_font = grdCurCanv->cv_font;
	GrPaletteStepLoad (NULL);
	i = ExecMessageBox ("Disk Error", NULL, 2, "Retry", "Exit", "%s\non drive %c:", crit_errors [descent_critical_errcode&0xf], (descent_critical_deverror&0xf)+'A');
	if (i == 1)
		exit (1);
	GrSetCurrentCanvas (save_canv);
	grdCurCanv->cv_font = save_font;
}

//------------------------------------------------------------------------------

int FindTextureByIndex (int nIndex)
{
	int	i, j = gameData.pig.tex.nBitmaps [gameStates.app.bD1Mission];

for (i = 0; i < j; i++)
	if (gameData.pig.tex.pBmIndex [i].index == nIndex)
		return i;
return -1;
}

//------------------------------------------------------------------------------

#define	CHANGING_TEXTURE(_eP)	\
			(((_eP)->changingWallTexture >= 0) ? \
			 (_eP)->changingWallTexture : \
			 (_eP)->changingObjectTexture)

tEffectClip *FindEffect (tEffectClip *ecP, int tNum)
{
	int				h, i, j;
	tBitmapIndex	*frameP;

if (ecP)
	i = (int) (++ecP - gameData.eff.pEffects);
else {
	ecP = gameData.eff.pEffects;
	i = 0;
	}
for (h = gameData.eff.nEffects [gameStates.app.bD1Data]; i < h; i++, ecP++) {
	for (j = ecP->vc.nFrameCount, frameP = ecP->vc.frames; j > 0; j--, frameP++)
		if (frameP->index == tNum) {
#if 0
			int t = FindTextureByIndex (tNum);
			if (t >= 0)
				ecP->changingWallTexture = t;
#endif
			return ecP;
			}
	}
return NULL;
}

//------------------------------------------------------------------------------

tVideoClip *FindVClip (int tNum)
{
	int	h, i, j;
	tVideoClip *vcP = gameData.eff.vClips [0];

for (i = gameData.eff.nClips [0]; i; i--, vcP++) {
	for (h = vcP->nFrameCount, j = 0; j < h; j++)
		if (vcP->frames [j].index == tNum) {
			return vcP;
			}
	}
return NULL;
}

//------------------------------------------------------------------------------

tWallClip *FindWallAnim (int tNum)
{
	int	h, i, j;
	tWallClip *wcP = gameData.walls.pAnims;

for (i = gameData.walls.nAnims [gameStates.app.bD1Data]; i; i--, wcP++)
	for (h = wcP->nFrameCount, j = 0; j < h; j++)
		if (gameData.pig.tex.pBmIndex [wcP->frames [j]].index == tNum)
			return wcP;
return NULL;
}

//------------------------------------------------------------------------------

inline void PiggyFreeBitmapData (grsBitmap *bmP)
{
if (bmP->bm_texBuf) {
#ifdef _DEBUG
	unsigned int h = bitmapCacheUsed;
#endif
	D2_FREE (bmP->bm_texBuf);
	bitmapCacheUsed -= bmP->bm_props.h * bmP->bm_props.rowsize;
#ifdef _DEBUG
	if (bitmapCacheUsed > h)
		bitmapCacheUsed = h;
#endif
	}
}

//------------------------------------------------------------------------------

int PiggyFreeHiresFrame (grsBitmap *bmP, int bD1)
{
BM_OVERRIDE (gameData.pig.tex.bitmaps [bD1] + bmP->bm_handle) = NULL;
OglFreeBmTexture (bmP);
bmP->bmType = 0;
bmP->bm_texBuf = NULL;
return 1;
}

//------------------------------------------------------------------------------

int PiggyFreeHiresAnimation (grsBitmap *bmP, int bD1)
{
	grsBitmap	*altBmP, *bmfP;
	int			i;

if (!(altBmP = BM_OVERRIDE (bmP)))
	return 0;
BM_OVERRIDE (bmP) = NULL;
if (altBmP->bmType == BM_TYPE_FRAME)
	if (!(altBmP = BM_PARENT (altBmP)))
		return 1;
if (altBmP->bmType != BM_TYPE_ALT)
	return 0;	//actually this would be an error
if ((bmfP = BM_FRAMES (altBmP)))
	for (i = BM_FRAMECOUNT (altBmP); i; i--, bmfP++)
		PiggyFreeHiresFrame (bmfP, bD1);
OglFreeBmTexture (altBmP);
D2_FREE (BM_FRAMES (altBmP));
altBmP->bm_data.alt.bm_frameCount = 0;
PiggyFreeBitmapData (altBmP);
altBmP->bm_palette = NULL;
altBmP->bmType = 0;
return 1;
}

//------------------------------------------------------------------------------

void PiggyFreeHiresAnimations (void)
{
	int			i, bD1;
	grsBitmap	*bmP;

for (bD1 = 0, bmP = gameData.pig.tex.bitmaps [bD1]; bD1 < 2; bD1++)
	for (i = gameData.pig.tex.nBitmaps [bD1]; i; i--, bmP++) {
		if (bmP - gameData.pig.tex.bitmaps [0] == 1998)
			i = i;
		PiggyFreeHiresAnimation (bmP, bD1);
		}
}

//------------------------------------------------------------------------------

void PiggyFreeBitmap (grsBitmap *bmP, int i, int bD1)
{
if (!bmP)
	bmP = gameData.pig.tex.bitmaps [bD1] + i;
else if (i < 0)
	i = (int) (bmP - gameData.pig.tex.bitmaps [bD1]);
if (!PiggyFreeHiresAnimation (bmP, 0))
	OglFreeBmTexture (bmP);
if (bitmapOffsets [bD1][i] > 0)
	bmP->bm_props.flags |= BM_FLAG_PAGED_OUT;
bmP->bm_fromPog = 0;
bmP->bm_palette = NULL;
PiggyFreeBitmapData (bmP);
}

//------------------------------------------------------------------------------

void PiggyBitmapPageOutAll (int bAll)
{
	int			i, bD1;
	grsBitmap	*bmP;
	
#if TRACE				
con_printf (CON_VERBOSE, "Flushing piggy bitmap cache\n");
#endif
gameData.pig.tex.bPageFlushed++;
TexMergeFlush ();
RLECacheFlush ();
for (bD1 = 0; bD1 < 2; bD1++) {
	bitmapCacheNext [bD1] = 0;
	for (i = 0, bmP = gameData.pig.tex.bitmaps [bD1]; 
		  i < gameData.pig.tex.nBitmaps [bD1]; 
		  i++, bmP++) {
		if (bmP->bm_texBuf && (bitmapOffsets [bD1][i] > 0)) { // only page out bitmaps read from disk
			bmP->bm_props.flags |= BM_FLAG_PAGED_OUT;
			PiggyFreeBitmap (bmP, i, bD1);
			}
		}
	}
}

//------------------------------------------------------------------------------

int IsMacDataFile (CFILE *fp, int bD1)
{
if (fp == piggyFP [bD1])
	switch (CFLength (piggyFP [bD1], 0)) {
		default:
			if (!FindArg ("-macdata"))
				break;
		case MAC_ALIEN1_PIGSIZE:
		case MAC_ALIEN2_PIGSIZE:
		case MAC_FIRE_PIGSIZE:
		case MAC_GROUPA_PIGSIZE:
		case MAC_ICE_PIGSIZE:
		case MAC_WATER_PIGSIZE:
			return !bD1;
		case D1_MAC_PIGSIZE:
		case D1_MAC_SHARE_PIGSIZE:
			return bD1;
		}
return 0;
}

//------------------------------------------------------------------------------

void GetFlagData (char *bmName, tBitmapIndex bmi)
{
	int			i;
	tFlagData	*pf;

if (strstr (bmName, "flag01#0") == bmName)
	i = 0;
else if (strstr (bmName, "flag02#0") == bmName)
	i = 1;
else
	return;
pf = gameData.pig.flags + i;
pf->bmi = bmi;
pf->vcP = FindVClip (bmi.index);
pf->vci.nClipIndex = gameData.objs.pwrUp.info [46 + i].nClipIndex;	//46 is the blue flag powerup
pf->vci.xFrameTime = gameData.eff.vClips [0][pf->vci.nClipIndex].xFrameTime;
pf->vci.nCurFrame = 0;
}

//------------------------------------------------------------------------------

void PiggyBitmapPageIn (tBitmapIndex bmi, int bD1)
{
	grsBitmap		*bmP, *altBmP = NULL;
	int				i, org_i, temp, nSize, nOffset, nFrames, nShrinkFactor, 
						bRedone = 0, bTGA;
	CFILE				*fp = NULL;
	char				fn [FILENAME_LEN], fnShrunk [FILENAME_LEN], bmName [20];
	tTgaHeader		h;

	//bD1 = gameStates.app.bD1Mission;
org_i = 0;
i = bmi.index;
nShrinkFactor = 1 << (3 - gameOpts->render.textures.nQuality);
#if 0
Assert (i >= 0);
Assert (i < MAX_BITMAP_FILES);
Assert (i < gameData.pig.tex.nBitmaps [bD1]);
Assert (bitmapCacheSize > 0);
#endif
if (i < 1) 
	return;
if (i >= MAX_BITMAP_FILES) 
	return;
if (i >= gameData.pig.tex.nBitmaps [bD1]) 
	return;
if (bitmapOffsets [bD1][i] == 0) 
	return;		// A read-from-disk bmi!!!

if (bLowMemory) {
	org_i = i;
	i = gameData.pig.tex.bitmapXlat [i];          // Xlat for low-memory settings!
	}
#if 1
bmP = BmOverride (gameData.pig.tex.bitmaps [bD1] + i);
if (!bmP->bm_texBuf) {
#else
bmP = gameData.pig.tex.bitmaps [bD1] + i;
if (bmP->bm_props.flags & BM_FLAG_PAGED_OUT) {
#endif
	StopTime ();
	nSize = bmP->bm_props.h * bmP->bm_props.rowsize;
	strcpy (bmName, gameData.pig.tex.bitmapFiles [bD1][i].name);
	GetFlagData (bmName, bmi);
#ifdef _DEBUG
	if (strstr (bmName, "rock260")) {
		sprintf (fn, "%s%s%s.tga", gameFolders.szTextureDir [bD1], 
					*gameFolders.szTextureDir [bD1] ? "/" : "", bmName);
		}
	else
#endif
	sprintf (fn, "%s%s%s.tga", gameFolders.szTextureDir [bD1], 
				*gameFolders.szTextureDir [bD1] ? "/" : "", bmName);
	sprintf (fnShrunk, "%s%s%s-%d.tga", gameFolders.szTextureDir [bD1], 
				*gameFolders.szTextureDir [bD1] ? "/" : "", bmName, 512 / nShrinkFactor);
	bTGA = 0;
	bmP->bm_bpp = 1;
	if (gameStates.app.bNostalgia)
		gameOpts->render.textures.bUseHires = 0;
	if (*bmName && gameOpts->render.textures.bUseHires &&
		 (!gameOpts->ogl.bGlTexMerge || gameStates.render.textures.bGlsTexMergeOk)) {
#if 0
		if (ReadS3TC (gameData.pig.tex.altBitmaps [bD1] + i, gameFolders.szTextureDir [bD1], bmName)) {
			altBmP = gameData.pig.tex.altBitmaps [bD1] + i;
			altBmP->bmType = BM_TYPE_ALT;
			BM_OVERRIDE (bmP) = altBmP;
			BM_FRAMECOUNT (altBmP) = 1;
			bitmapFlags [bD1][i] &= ~BM_FLAG_RLE;
			bitmapFlags [bD1][i] |= BM_FLAG_TGA;
			bmP = altBmP;
			altBmP = NULL;
			}
		else 
#endif
		if ((gameStates.app.bCacheTextures && (nShrinkFactor > 1) && (fp = CFOpen (fnShrunk, "", "rb", 0))) || 
			 (fp = CFOpen (fn, "", "rb", 0))) {
			LogErr ("loading hires texture '%s' (quality: %d)\n", fn, gameOpts->render.nTextureQuality);
			bTGA = 1;
			altBmP = gameData.pig.tex.altBitmaps [bD1] + i;
			altBmP->bmType = BM_TYPE_ALT;
			BM_OVERRIDE (bmP) = altBmP;
			bmP = altBmP;
			ReadTGAHeader (fp, &h, bmP);
			nSize = h.width * h.height * 4;
			nFrames = (h.height % h.width) ? 1 : h.height / h.width;
			BM_FRAMECOUNT (bmP) = (ubyte) nFrames;
			nOffset = CFTell (fp);
			bitmapFlags [bD1][i] &= ~(BM_FLAG_RLE | BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT);
			bitmapFlags [bD1][i] |= BM_FLAG_TGA;
			if (bmP->bm_props.h > bmP->bm_props.w) {
				tEffectClip	*ecP = NULL;
				tWallClip *wcP;
				tVideoClip *vcP;
				while ((ecP = FindEffect (ecP, i))) {
					//e->vc.nFrameCount = nFrames;
					ecP->flags |= EF_ALTFMT;
					//ecP->vc.flags |= WCF_ALTFMT;
					}
				if (!ecP) {
					if ((wcP = FindWallAnim (i))) {
					//w->nFrameCount = nFrames;
						wcP->flags |= WCF_ALTFMT;
						}
					else if ((vcP = FindVClip (i))) {
						//v->nFrameCount = nFrames;
						vcP->flags |= WCF_ALTFMT;
						}
					else {
						LogErr ("   couldn't find animation for '%s'\n", bmName);
						}
					}	
				}
			}
		}
	if (!altBmP) {
		fp = piggyFP [bD1];
		nOffset = bitmapOffsets [bD1][i];
		}

reloadTextures:

	if (bRedone) {
		Error ("Not enough memory for textures.\nTry to decrease texture quality\nin the advanced render options menu.");
#ifndef _DEBUG
		return;
#endif
		}
	bRedone = 1;
	if (CFSeek (fp, nOffset, SEEK_SET)) {
		PiggyCriticalError ();
		goto reloadTextures;
		}
#if TEXTURE_COMPRESSION
	if (bmP->bm_compressed)
		bitmapCacheUsed += bmP->bm_bufSize;
	else 
#endif
		{
		bmP->bm_texBuf = D2_ALLOC (nSize);
		if (bmP->bm_texBuf) 
			bitmapCacheUsed += nSize;
		}
	if (!bmP->bm_texBuf || (bitmapCacheUsed > bitmapCacheSize)) {
		Int3 ();
		PiggyBitmapPageOutAll (0);
		goto reloadTextures;
		}
	bmP->bm_props.flags = bitmapFlags [bD1][i];
	bmP->bm_handle = i;
	if (bmP->bm_props.flags & BM_FLAG_RLE) {
		int zSize = 0;
		nDescentCriticalError = 0;
		zSize = CFReadInt (fp);
		if (nDescentCriticalError) {
			PiggyCriticalError ();
			goto reloadTextures;
			}
		temp = (int) CFRead (bmP->bm_texBuf + 4, 1, zSize-4, fp);
		if (nDescentCriticalError) {
			PiggyCriticalError ();
			goto reloadTextures;
			}
		zSize = rle_expand (bmP, NULL, 0);
		if (bD1)
			GrRemapBitmapGood (bmP, d1Palette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
		else
			GrRemapBitmapGood (bmP, gamePalette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
		}
	else 
#if TEXTURE_COMPRESSION
		if (!bmP->bm_compressed) 
#endif
		{
		nDescentCriticalError = 0;
		if (fp == piggyFP [bD1]) {
			temp = (int) CFRead (bmP->bm_texBuf, 1, nSize, fp);
			if (bD1)
				GrRemapBitmapGood (bmP, d1Palette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
			else
				GrRemapBitmapGood (bmP, gamePalette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
			}
		else {
			ReadTGAImage (fp, &h, bmP, -1, 1.0, 0, 0);
			bmP->bmType = BM_TYPE_ALT;
			if (IsOpaqueDoor (i)) {
				bmP->bm_props.flags &= ~BM_FLAG_TRANSPARENT;
				bmP->bm_transparentFrames [0] &= ~1;
				}
#if TEXTURE_COMPRESSION
			if (CompressTGA (bmP))
				SaveS3TC (bmP, gameFolders.szTextureDir [bD1], bmName);
			else 
#endif
			if ((nShrinkFactor > 1) && (bmP->bm_props.w == 512) && ShrinkTGA (bmP, nShrinkFactor, nShrinkFactor, 1)) {
				nSize /= (nShrinkFactor * nShrinkFactor);
				if (gameStates.app.bCacheTextures)
					SaveTGA (bmName, gameFolders.szTextureDir [bD1], &h, bmP);
				}
			}
		if (nDescentCriticalError) {
			PiggyCriticalError ();
			goto reloadTextures;
			}
		}
#ifndef MACDATA
	if (!bTGA && IsMacDataFile (fp, bD1))
		swap_0_255 (bmP);
#endif
	StartTime ();
	}

if (bLowMemory) {
	if (org_i != i) {
		gameData.pig.tex.bitmaps [bD1][org_i] = gameData.pig.tex.bitmaps [bD1][i];
		i = org_i;
		}
	}
gameData.pig.tex.bitmaps [bD1][i].bm_props.flags &= ~BM_FLAG_PAGED_OUT;
if (fp && (fp != piggyFP [bD1]))
	CFClose (fp);
}

//------------------------------------------------------------------------------

void PiggyLoadLevelData ()
{
LogErr ("   loading level textures\n");
PiggyBitmapPageOutAll (0);
PagingTouchAll ();
}

//------------------------------------------------------------------------------

#ifdef EDITOR

void change_filename_ext (char *dest, char *src, char *ext);

void PiggyWritePigFile (char *filename)
{
	FILE *pig_fp;
	int nBmDataOffs, data_offset;
	DiskBitmapHeader bmh;
	int org_offset;
	char subst_name [32];
	int i, j;
	FILE *fp1,*fp2;
	char tname [SHORT_FILENAME_LEN];

	for (i = 0, j = gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]; i < j; i++) {
		tBitmapIndex bi;
		bi.index = i;
		PIGGY_PAGE_IN (bi);
	}

	PiggyCloseFile ();

	pig_fp = fopen (filename, "wb");       //open PIG file
	Assert (pig_fp!=NULL);

	write_int (PIGFILE_ID,pig_fp);
	write_int (PIGFILE_VERSION,pig_fp);

	gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]--;
	fwrite (&gameData.pig.tex.nBitmaps [gameStates.app.bD1Data], sizeof (int), 1, pig_fp);
	gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]++;

	nBmDataOffs = ftell (pig_fp);
	nBmDataOffs += (gameData.pig.tex.nBitmaps [gameStates.app.bD1Data] - 1) * sizeof (DiskBitmapHeader);
	data_offset = nBmDataOffs;

	change_filename_ext (tname,filename,"lst");
	fp1 = fopen (tname, "wt");
	change_filename_ext (tname,filename,"all");
	fp2 = fopen (tname, "wt");

	for (i = 1, j = gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]; i < j; i++) {
		int *size;
		grsBitmap *bmp;

		{               
			char * p, *p1;
			p = strchr (gameData.pig.tex.bitmapFiles [i].name, '#');
			if (p) {   // this is an ABM == animated bitmap
				int n;
				p1 = p; p1++; 
				n = atoi (p1);
				*p = 0;
				if (fp2 && n==0)
					fprintf (fp2, "%s.abm\n", gameData.pig.tex.pBitmapFiles[i].name);
				memcpy (bmh.name, gameData.pig.tex.bitmapFiles [i].name, 8);
				Assert (n <= DBM_NUM_FRAMES);
				bmh.dflags = DBM_FLAG_ABM + n;
				*p = '#';
			} else {
				if (fp2)
					fprintf (fp2, "%s.bbm\n", gameData.pig.tex.pBitmapFiles[i].name);
				memcpy (bmh.name, gameData.pig.tex.pBitmapFiles[i].name, 8);
				bmh.dflags = 0;
			}
		}
		bmp = gameData.pig.tex.bitmaps + i;

		Assert (!(bmp->bm_props.flags & BM_FLAG_PAGED_OUT));

		if (fp1)
			fprintf (fp1, "BMP: %s, size %d bytes", gameData.pig.tex.pBitmapFiles[i].name, bmp->bm_props.rowsize * bmp->bm_props.h);
		org_offset = ftell (pig_fp);
		bmh.offset = data_offset - nBmDataOffs;
		fseek (pig_fp, data_offset, SEEK_SET);

		if (bmp->bm_props.flags & BM_FLAG_RLE) {
			size = (int *)bmp->bm_texBuf;
			fwrite (bmp->bm_texBuf, sizeof (ubyte), *size, pig_fp);
			data_offset += *size;
			if (fp1)
				fprintf (fp1, ", and is already compressed to %d bytes.\n", *size);
		} else {
			fwrite (bmp->bm_texBuf, sizeof (ubyte), bmp->bm_props.rowsize * bmp->bm_props.h, pig_fp);
			data_offset += bmp->bm_props.rowsize * bmp->bm_props.h;
			if (fp1)
				fprintf (fp1, ".\n");
		}
		fseek (pig_fp, org_offset, SEEK_SET);
		Assert (gameData.pig.tex.bitmaps [i].bm_props.w < 4096);
		bmh.width = (gameData.pig.tex.pBitmaps[i].bm_props.w & 0xff);
		bmh.wh_extra = ((gameData.pig.tex.pBitmaps[i].bm_props.w >> 8) & 0x0f);
		Assert (gameData.pig.tex.bitmaps [i].bm_props.h < 4096);
		bmh.height = gameData.pig.tex.bitmaps [i].bm_props.h;
		bmh.wh_extra |= ((gameData.pig.tex.pBitmaps[i].bm_props.h >> 4) & 0xf0);
		bmh.flags = gameData.pig.tex.pBitmaps[i].bm_props.flags;
		if (PiggyIsSubstitutableBitmap (gameData.pig.tex.pBitmapFiles[i].name, subst_name)) {
			tBitmapIndex other_bitmap;
			other_bitmap = PiggyFindBitmap (subst_name, gameStates.app.bD1Data);
			gameData.pig.tex.bitmapXlat [i] = other_bitmap.index;
			bmh.flags |= BM_FLAG_PAGED_OUT;
		else
			bmh.flags &= ~BM_FLAG_PAGED_OUT;
			}
		bmh.bm_avgColor=gameData.pig.tex.bitmaps [i].bm_avgColor;
		fwrite (&bmh, sizeof (DiskBitmapHeader), 1, pig_fp);  // Mark as a bitmap
		}
	fclose (pig_fp);
#if TRACE				
	con_printf (CONDBG, " Dumped %d assorted bitmaps.\n", gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]);
#endif
	fprintf (fp1, " Dumped %d assorted bitmaps.\n", gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]);

	fclose (fp1);
	fclose (fp2);

}

//------------------------------------------------------------------------------

static void write_int (int i,FILE *file)
{
	if (fwrite (&i, sizeof (i), 1, file) != 1)
		Error ("Error reading int in gamesave.c");

}

//------------------------------------------------------------------------------

void PiggyDumpAll ()
{
	int i, xlatOffset;
	FILE * ham_fp;
	int org_offset,data_offset=0;
	DiskSoundHeader sndh;
	int sound_data_start=0;
	FILE *fp1,*fp2;

	#ifdef NO_DUMP_SOUNDS
	gameData.pig.snd.nSoundFiles [0] = 0;
	gameData.pig.snd.nSoundFiles [1] = 0;
	nSoundFilesNew = 0;
	#endif

	if (!bMustWriteHamFile && (nBitmapFilesNew == 0) && (nSoundFilesNew == 0))
		return;

	fp1 = fopen ("ham.lst", "wt");
	fp2 = fopen ("ham.all", "wt");

	if (bMustWriteHamFile || nBitmapFilesNew) {

#if TRACE				
		con_printf (CONDBG, "Creating %s...",DEFAULT_HAMFILE);
#endif

		ham_fp = fopen (DEFAULT_HAMFILE, "wb");                       //open HAM file
		Assert (ham_fp!=NULL);
	
		write_int (HAMFILE_ID,ham_fp);
		write_int (HAMFILE_VERSION,ham_fp);
	
		bm_write_all (ham_fp);
		xlatOffset = ftell (ham_fp);
		fwrite (gameData.pig.tex.bitmapXlat, sizeof (ushort)*MAX_BITMAP_FILES, 1, ham_fp);
		//Dump bitmaps
	
		if (nBitmapFilesNew)
			PiggyWritePigFile (DEFAULT_PIGFILE);
	
		//D2_FREE up memeory used by new bitmaps
		for (i=gameData.pig.tex.nBitmaps-nBitmapFilesNew;i<gameData.pig.tex.nBitmaps;i++)
			D2_FREE (gameData.pig.tex.bitmaps [i].bm_texBuf);
	
		//next thing must be done after pig written
		fseek (ham_fp, xlatOffset, SEEK_SET);
		fwrite (gameData.pig.tex.bitmapXlat, sizeof (ushort)*MAX_BITMAP_FILES, 1, ham_fp);
	
		fclose (ham_fp);
#if TRACE				
		con_printf (CONDBG, "\n");
#endif
	}
	
	if (nSoundFilesNew) {

#if TRACE				
		con_printf (CONDBG, "Creating %s...",DEFAULT_HAMFILE);
#endif
		// Now dump sound file
		ham_fp = fopen (DEFAULT_SNDFILE, "wb");
		Assert (ham_fp!=NULL);
	
		write_int (SNDFILE_ID,ham_fp);
		write_int (SNDFILE_VERSION,ham_fp);

		fwrite (&gameData.pig.snd.nSoundFiles [0], sizeof (int), 1, ham_fp);
	
#if TRACE				
		con_printf (CONDBG, "\nDumping sounds...");
#endif	
		sound_data_start = ftell (ham_fp);
		sound_data_start += gameData.pig.snd.nSoundFiles [0]*sizeof (DiskSoundHeader);
		data_offset = sound_data_start;
	
		for (i=0; i < gameData.pig.snd.nSoundFiles [0]; i++) {
			tDigiSound *snd;
	
			snd = &gameData.pig.snd.sounds [0][i];
			strcpy (sndh.name, sounds [0][i].name);
			sndh.length = gameData.pig.snd.sounds [0][i].length;
			sndh.offset = data_offset - sound_data_start;
	
			org_offset = ftell (ham_fp);
			fseek (ham_fp, data_offset, SEEK_SET);
	
			sndh.data_length = gameData.pig.snd.sounds [0][i].length;
			fwrite (snd->data, sizeof (ubyte), snd->length, ham_fp);
			data_offset += snd->length;
			fseek (ham_fp, org_offset, SEEK_SET);
			fwrite (&sndh, sizeof (DiskSoundHeader), 1, ham_fp);                    // Mark as a bitmap
	
			fprintf (fp1, "SND: %s, size %d bytes\n", sounds [i].name, snd->length);
			fprintf (fp2, "%s.raw\n", sounds [i].name);
		}

		fclose (ham_fp);
#if TRACE				
		con_printf (CONDBG, "\n");
#endif
	}

	fprintf (fp1, "Total sound size: %d bytes\n", data_offset-sound_data_start);
#if TRACE				
	con_printf (CONDBG, " Dumped %d assorted sounds.\n", gameData.pig.snd.nSoundFiles [0]);
#endif
	fprintf (fp1, " Dumped %d assorted sounds.\n", gameData.pig.snd.nSoundFiles [0]);

	fclose (fp1);
	fclose (fp2);

	// Never allow the game to run after building ham.
	exit (0);
}

#endif

//------------------------------------------------------------------------------

void _CDECL_ PiggyClose (void)
{
	int	i;

LogErr ("unloading textures\n");	
PiggyCloseFile ();
for (i = 0; i < 2; i++) {
	if (SoundBits [i])
		D2_FREE (SoundBits [i]);
	hashtable_free (bitmapNames + i);
	hashtable_free (soundNames + i);
	}
}

//------------------------------------------------------------------------------

int PiggyBitmapExistsSlow (char * name)
{
	int i, j;

	for (i=0, j=gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]; i<j; i++) {
		if (!strcmp (gameData.pig.tex.pBitmapFiles[i].name, name))
			return 1;
	}
	return 0;
}

//------------------------------------------------------------------------------

#define NUM_GAUGE_BITMAPS 23
char * gauge_bitmap_names [NUM_GAUGE_BITMAPS] = {
	"gauge01", "gauge01b",
	"gauge02", "gauge02b",
	"gauge06", "gauge06b",
	"targ01", "targ01b",
	"targ02", "targ02b", 
	"targ03", "targ03b",
	"targ04", "targ04b",
	"targ05", "targ05b",
	"targ06", "targ06b",
	"gauge18", "gauge18b",
	"gauss1", "helix1",
	"phoenix1"
};

//------------------------------------------------------------------------------

int PiggyIsGaugeBitmap (char * base_name)
{
	int i;
	for (i=0; i<NUM_GAUGE_BITMAPS; i++) {
		if (!stricmp (base_name, gauge_bitmap_names [i]))      
			return 1;
	}

	return 0;       
}

//------------------------------------------------------------------------------

int PiggyIsSubstitutableBitmap (char * name, char * subst_name)
{
	int frame;
	char * p;
	char base_name [ 16 ];
	
	strcpy (subst_name, name);
	p = strchr (subst_name, '#');
	if (p) {
		frame = atoi (&p [1]);
		*p = 0;
		strcpy (base_name, subst_name);
		if (!PiggyIsGaugeBitmap (base_name)) {
			sprintf (subst_name, "%s#%d", base_name, frame+1);
			if (PiggyBitmapExistsSlow (subst_name)) {
				if (frame & 1) {
					sprintf (subst_name, "%s#%d", base_name, frame-1);
					return 1;
				}
			}
		}
	}
	strcpy (subst_name, name);
	return 0;
}

//------------------------------------------------------------------------------
/*
 * Functions for loading replacement textures
 *  1) From .pog files
 *  2) From descent.pig (for loading d1 levels)
 */

extern void ChangeFilenameExtension (char *dest, char *src, char *new_ext);
extern char szLastPalettePig [];

void _CDECL_ FreeBitmapReplacements (void)
{
}

//------------------------------------------------------------------------------

void LoadBitmapReplacements (char *level_name)
{
	char			szFilename [SHORT_FILENAME_LEN];
	CFILE			*fp;
	int			i, j;
	grsBitmap	bm;

	//first, D2_FREE up data allocated for old bitmaps
LogErr ("   loading replacement textures\n");
FreeBitmapReplacements ();
ChangeFilenameExtension (szFilename, level_name, ".pog");
fp = CFOpen (szFilename, gameFolders.szDataDir,"rb",0);
if (fp) {
	int					id, version, nBitmapNum, bTGA;
	int					bmDataSize, bmDataOffset, bmOffset;
	ushort				*indices;
	DiskBitmapHeader	*bmh;

	id = CFReadInt (fp);
	version = CFReadInt (fp);
	if (id != MAKE_SIG ('G','O','P','D') || version != 1) {
		CFClose (fp);
		return;
		}
	nBitmapNum = CFReadInt (fp);
	MALLOC (indices, ushort, nBitmapNum);
	MALLOC (bmh, DiskBitmapHeader, nBitmapNum);
#if 0
	CFRead (indices, nBitmapNum * sizeof (ushort), 1, fp);
	CFRead (bmh, nBitmapNum * sizeof (DiskBitmapHeader), 1, fp);
#else
	for (i = 0; i < nBitmapNum; i++)
		indices [i] = CFReadShort (fp);
	for (i = 0; i < nBitmapNum; i++)
		DiskBitmapHeaderRead (bmh + i, fp);
#endif
	bmDataOffset = CFTell (fp);
	bmDataSize = CFLength (fp, 0) - bmDataOffset;

	for (i = 0; i < nBitmapNum; i++) {
		bmOffset = bmh [i].offset;
		memset (&bm, 0, sizeof (grsBitmap));
		bm.bm_props.flags |= bmh [i].flags & (BM_FLAGS_TO_COPY | BM_FLAG_TGA);
		bm.bm_props.w = bm.bm_props.rowsize = bmh [i].width + ((short) (bmh [i].wh_extra & 0x0f) << 8);
		if ((bTGA = (bm.bm_props.flags & BM_FLAG_TGA)) && (bm.bm_props.w > 256))
			bm.bm_props.h = bm.bm_props.w * bmh [i].height;
		else
			bm.bm_props.h = bmh [i].height + ((short) (bmh [i].wh_extra & 0xf0) << 4);
		bm.bm_bpp = bTGA ? 4 : 1;
		if (!(bm.bm_props.w * bm.bm_props.h))
			continue;
		if (bmOffset + bm.bm_props.h * bm.bm_props.rowsize > bmDataSize)
			break;
		bm.bm_avgColor = bmh [i].bm_avgColor;
		bm.bmType = BM_TYPE_ALT;
		if (!(bm.bm_texBuf = GrAllocBitmapData (bm.bm_props.w, bm.bm_props.h, bm.bm_bpp)))
			break;
		CFSeek (fp, bmDataOffset + bmOffset, SEEK_SET);
		if (bTGA) {
			int	nFrames = bm.bm_props.h / bm.bm_props.w;
			tTgaHeader	h;

			h.width = bm.bm_props.w;
			h.height = bm.bm_props.h;
			h.bits = 32;
			if (!ReadTGAImage (fp, &h, &bm, -1, 1.0, 0, 1)) {
				D2_FREE (bm.bm_texBuf);
				break;
				}
			bm.bm_props.rowsize *= bm.bm_bpp;
			bm.bm_data.alt.bm_frameCount = (ubyte) nFrames;
			if (nFrames > 1) {
				tEffectClip	*ecP = NULL;
				tWallClip *wcP;
				tVideoClip *vcP;
				while ((ecP = FindEffect (ecP, indices [i]))) {
					//e->vc.nFrameCount = nFrames;
					ecP->flags |= EF_ALTFMT | EF_FROMPOG;
					}
				if (!ecP) {
					if ((wcP = FindWallAnim (indices [i]))) {
						//w->nFrameCount = nFrames;
						wcP->flags |= WCF_ALTFMT | WCF_FROMPOG;
						}
					else if ((vcP = FindVClip (i))) {
						//v->nFrameCount = nFrames;
						vcP->flags |= WCF_ALTFMT | WCF_FROMPOG;
						}
					}
				}
			j = indices [i];
			}
		else {
			int nSize = bm.bm_props.w * bm.bm_props.h;
			if (nSize != (int) CFRead (bm.bm_texBuf, 1, nSize, fp)) {
				D2_FREE (bm.bm_texBuf);
				break;
				}
			bm.bm_palette = gamePalette;
			j = indices [i];
			rle_expand (&bm, NULL, 0);
			bm.bm_props = gameData.pig.tex.pBitmaps [j].bm_props;
			GrRemapBitmapGood (&bm, gamePalette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
			}
		PiggyFreeBitmap (NULL, j, 0);
		bm.bm_fromPog = 1;
		bm.bm_handle = j;
		gameData.pig.tex.pAltBitmaps [j] = bm;
		BM_OVERRIDE (gameData.pig.tex.pBitmaps + j) = gameData.pig.tex.pAltBitmaps + j;
		bitmapCacheUsed += bm.bm_props.h * bm.bm_props.rowsize;
		}
	D2_FREE (indices);
	D2_FREE (bmh);
	CFClose (fp);
	szLastPalettePig [0] = 0;  //force pig re-load
	TexMergeFlush ();       //for re-merging with new textures
	}
atexit (FreeBitmapReplacements);
}

//------------------------------------------------------------------------------
/* calculate table to translate d1 bitmaps to current palette,
 * return -1 on error
 */
ubyte *LoadD1Palette (void)
{
	tPalette	palette;

CFILE * fp = CFOpen (D1_PALETTE, gameFolders.szDataDir, "rb", 1);
if (!fp || CFLength (fp, 0) != 9472) {
	return NULL;
	}
CFRead (palette, 256, 3, fp);
CFClose (fp);
palette [254] = SUPER_TRANSP_COLOR;
palette [255] = TRANSPARENCY_COLOR;
return d1Palette = AddPalette (palette);
}

//------------------------------------------------------------------------------

void PiggyBitmapReadD1 (
	grsBitmap			*bmP, /* read into this bmP */
   CFILE					*piggyFP, /* read from this file */
	int					nBmDataOffs, /* specific to file */
   DiskBitmapHeader	*bmh, /* header info for bmP */
   ubyte					**pNextBmP, /* where to write it (if 0, use D2_ALLOC) */
	ubyte					*palette, /* what palette the bmP has */
   ubyte					*colorMap) /* how to translate bmP's colors */
{
	int zSize, bSwap0255;

memset (bmP, 0, sizeof (grsBitmap));
bmP->bm_props.w = bmP->bm_props.rowsize = bmh->width + ((short) (bmh->wh_extra&0x0f)<<8);
bmP->bm_props.h = bmh->height + ((short) (bmh->wh_extra&0xf0)<<4);
bmP->bm_avgColor = bmh->bm_avgColor;
bmP->bm_props.flags |= bmh->flags & BM_FLAGS_TO_COPY;

CFSeek (piggyFP , nBmDataOffs + bmh->offset, SEEK_SET);
if (bmh->flags & BM_FLAG_RLE) {
	zSize = CFReadInt (piggyFP);
	CFSeek (piggyFP, -4, SEEK_CUR);
	}
else
	zSize = bmP->bm_props.h * bmP->bm_props.w;

if (pNextBmP) {
	bmP->bm_texBuf = *pNextBmP;
	*pNextBmP += zSize;
	}
else {
	bmP->bm_texBuf = D2_ALLOC (bmP->bm_props.h * bmP->bm_props.rowsize);
	bitmapCacheUsed += bmP->bm_props.h * bmP->bm_props.rowsize;
	}
CFRead (bmP->bm_texBuf, 1, zSize, piggyFP);
bSwap0255 = 0;
switch (CFLength (piggyFP,0)) {
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		if (bmh->flags & BM_FLAG_RLE)
			bSwap0255 = 1;
		else
			swap_0_255 (bmP);
		}
if (bmh->flags & BM_FLAG_RLE)
	rle_expand (bmP, NULL, bSwap0255);
GrRemapBitmapGood (bmP, d1Palette, TRANSPARENCY_COLOR, -1);
}

//------------------------------------------------------------------------------
#define D1_MAX_TEXTURES 800
#define D1_MAX_TMAP_NUM 1630 // 1621 in descent.pig Mac registered

/* the inverse of the gameData.pig.tex.bmIndex array, for descent 1.
 * "gameData.pig.tex.bmIndex" looks up a d2 bitmap index given a d2 nBaseTex
 * "d1_tmap_nums" looks up a d1 nBaseTex given a d1 bitmap. "-1" means "None"
 */
void _CDECL_ FreeD1TMapNums (void) 
{
if (d1_tmap_nums) {
	LogErr ("unloading D1 texture ids\n");
	D2_FREE (d1_tmap_nums);
	d1_tmap_nums = NULL;
	}
}

//------------------------------------------------------------------------------

void BMReadD1TMapNums (CFILE *d1pig)
{
	int i, d1_index;

	FreeD1TMapNums ();
	CFSeek (d1pig, 8, SEEK_SET);
	MALLOC (d1_tmap_nums, short, D1_MAX_TMAP_NUM);
	for (i = 0; i < D1_MAX_TMAP_NUM; i++)
		d1_tmap_nums [i] = -1;
	for (i = 0; i < D1_MAX_TEXTURES; i++) {
		d1_index = CFReadShort (d1pig);
		Assert (d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM);
		d1_tmap_nums [d1_index] = i;
	}
	atexit (FreeD1TMapNums);
}

//------------------------------------------------------------------------------
/* If the given d1_index is the index of a bitmap we have to load
 * (because it is unique to descent 1), then returns the d2_index that
 * the given d1_index replaces.
 * Returns -1 if the given d1_index is not unique to descent 1.
 */
short D2IndexForD1Index (short d1_index)
{
	short d1_tmap_num = d1_tmap_nums ? d1_tmap_nums [d1_index] : -1;

	Assert (d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM);
	if ((d1_tmap_num == -1) || !d1_tmap_num_unique (d1_tmap_num))
  		return -1;
	return gameData.pig.tex.bmIndex [0][ConvertD1Texture (d1_tmap_num, 0)].index;
}

//------------------------------------------------------------------------------

void LoadD1PigHeader (
	CFILE *fp, int *pSoundNum, int *pBmHdrOffs, int *pBmDataOffs, int *pBitmapNum, int bReadTMapNums)
{

#	define D1_PIG_LOAD_FAILED "Failed loading " D1_PIGFILE

	int	nPigDataStart,
			nHeaderSize,
			nBmHdrOffs, 
			nBmDataOffs,
			nSoundNum, 
			nBitmapNum;

switch (CFLength (fp, 0)) {
	case D1_SHARE_BIG_PIGSIZE:
	case D1_SHARE_10_PIGSIZE:
	case D1_SHARE_PIGSIZE:
	case D1_10_BIG_PIGSIZE:
	case D1_10_PIGSIZE:
		nPigDataStart = 0;
		Warning ("%s %s. %s", TXT_LOAD_FAILED, D1_PIGFILE, TXT_D1_SUPPORT);
		return;
	default:
		Warning ("%s %s", TXT_UNKNOWN_SIZE, D1_PIGFILE);
		Int3 ();
		// fall through
	case D1_PIGSIZE:
	case D1_OEM_PIGSIZE:
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		nPigDataStart = CFReadInt (fp);
		if (bReadTMapNums)
			BMReadD1TMapNums (fp); 
		break;
	}
CFSeek (fp, nPigDataStart, SEEK_SET);
nBitmapNum = CFReadInt (fp);
nSoundNum = CFReadInt (fp);
nHeaderSize = nBitmapNum * DISKBITMAPHEADER_D1_SIZE + nSoundNum * sizeof (DiskSoundHeader);
nBmHdrOffs = nPigDataStart + 2 * sizeof (int);
nBmDataOffs = nBmHdrOffs + nHeaderSize;
if (pSoundNum)
	*pSoundNum = nSoundNum;
*pBmHdrOffs = nBmHdrOffs;
*pBmDataOffs = nBmDataOffs;
*pBitmapNum = nBitmapNum;
}

//------------------------------------------------------------------------------

static int bHaveD1Sounds = 0;

grsBitmap bmTemp;

#define D1_BITMAPS_SIZE (128 * 1024 * 1024)

void LoadD1BitmapReplacements ()
{
	DiskBitmapHeader	bmh;
	char					temp_name [16];
	char					temp_name_read [16];
	int					i, nBmHdrOffs, nBmDataOffs, nSoundNum, nBitmapNum;

if (piggyFP [1])
	CFSeek (piggyFP [1], 0, SEEK_SET);
else
	piggyFP [1] = CFOpen (D1_PIGFILE, gameFolders.szDataDir, "rb", 0);

if (!piggyFP [1]) {
	Warning (D1_PIG_LOAD_FAILED);
	return;
	}
//first, free up data allocated for old bitmaps
FreeBitmapReplacements ();
#ifdef _DEBUG
Assert (LoadD1Palette () != NULL);
#else
LoadD1Palette ();
#endif

LoadD1PigHeader (piggyFP [1], &nSoundNum, &nBmHdrOffs, &nBmDataOffs, &nBitmapNum, 1);
if (gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && !gameStates.app.bHaveD1Textures) {
	gameStates.app.bD1Data = 1;
	SetDataVersion (1);
	if (!bHaveD1Sounds) {
		LoadSounds (piggyFP [1], nSoundNum, nBmHdrOffs + nBitmapNum * DISKBITMAPHEADER_D1_SIZE);
		PiggyReadSounds ();
		bHaveD1Sounds = 1;
		}
	CFSeek (piggyFP [1], nBmHdrOffs, SEEK_SET);
	gameData.pig.tex.nBitmaps [1] = 0;
	PiggyRegisterBitmap (&bogus_bitmap, "bogus", 1);
	for (i = 0; i < nBitmapNum; i++) {
		DiskBitmapHeader_d1_read (&bmh, piggyFP [1]);
		memcpy (temp_name_read, bmh.name, 8);
		temp_name_read [8] = 0;
		if (bmh.dflags & DBM_FLAG_ABM)        
			sprintf (temp_name, "%s#%d", temp_name_read, bmh.dflags & DBM_NUM_FRAMES);
		else
			strcpy (temp_name, temp_name_read);
		memset (&bmTemp, 0, sizeof (grsBitmap));
		bmTemp.bm_props.w = bmTemp.bm_props.rowsize = bmh.width + ((short) (bmh.wh_extra&0x0f)<<8);
		bmTemp.bm_props.h = bmh.height + ((short) (bmh.wh_extra&0xf0)<<4);
		bmTemp.bm_props.flags |= BM_FLAG_PAGED_OUT;
		bmTemp.bm_avgColor = bmh.bm_avgColor;
		bmTemp.bm_texBuf = D2_ALLOC (bmTemp.bm_props.w * bmTemp.bm_props.h);
		bitmapCacheUsed += bmTemp.bm_props.h * bmTemp.bm_props.w;
		bitmapFlags [1][i+1] = bmh.flags & BM_FLAGS_TO_COPY;
		bitmapOffsets [1][i+1] = bmh.offset + nBmDataOffs;
		Assert ((i+1) == gameData.pig.tex.nBitmaps [1]);
		PiggyRegisterBitmap (&bmTemp, temp_name, 1);
		}
	gameStates.app.bHaveD1Textures = 1;
	}
//CFClose (piggyFP [1]);
szLastPalettePig [0]= 0;  //force pig re-load
TexMergeFlush ();       //for re-merging with new textures
}

//------------------------------------------------------------------------------

/*
 * Find and load the named bitmap from descent.pig
 * similar to ReadExtraBitmapIFF
 */
tBitmapIndex ReadExtraBitmapD1Pig (char *name)
{
	CFILE					*piggyFP;
	DiskBitmapHeader	bmh;
	int					i, nBmHdrOffs, nBmDataOffs, nBitmapNum;
	tBitmapIndex		bmi;
	grsBitmap			*newBm = gameData.pig.tex.bitmaps [0] + gameData.pig.tex.nExtraBitmaps;

bmi.index = 0;
if (!(piggyFP = CFOpen (D1_PIGFILE, gameFolders.szDataDir, "rb", 0))) {
	Warning (D1_PIG_LOAD_FAILED);
	return bmi;
	}
if (!gameStates.app.bHaveD1Data) {
#ifdef _DEBUG
Assert (LoadD1Palette () != NULL);
#else
LoadD1Palette ();
#endif
}
LoadD1PigHeader (piggyFP, NULL, &nBmHdrOffs, &nBmDataOffs, &nBitmapNum, 0);
for (i = 0; i < nBitmapNum; i++) {
	DiskBitmapHeader_d1_read (&bmh, piggyFP);
	if (!strnicmp (bmh.name, name, 8))
		break;
	}
if (i >= nBitmapNum) {
#if TRACE				
	con_printf (CONDBG, "could not find bitmap %s\n", name);
#endif
	return bmi;
	}
PiggyBitmapReadD1 (newBm, piggyFP, nBmDataOffs, &bmh, 0, d1Palette, d1ColorMap);
CFClose (piggyFP);
newBm->bm_avgColor = 0;	//ComputeAvgPixel (newBm);
bmi.index = gameData.pig.tex.nExtraBitmaps;
gameData.pig.tex.pBitmaps[gameData.pig.tex.nExtraBitmaps++] = *newBm;
return bmi;
}

//------------------------------------------------------------------------------

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads a tBitmapIndex structure from a CFILE
 */
void BitmapIndexRead (tBitmapIndex *bi, CFILE *fp)
{
	bi->index = CFReadShort (fp);
}

//------------------------------------------------------------------------------
/*
 * reads n tBitmapIndex structs from a CFILE
 */
int BitmapIndexReadN (tBitmapIndex *bi, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++)
		bi [i].index = CFReadShort (fp);
	return i;
}
#endif // FAST_FILE_IO

//------------------------------------------------------------------------------

typedef struct tBitmapFileHeader {
	short	bfType;
	unsigned int bfSize;
	short bfReserved1;
	short bfReserved2;
	unsigned int bfOffBits;
} tBitmapFileHeader;

typedef struct tBitmapInfoHeader {
	unsigned int biSize;
	unsigned int biWidth;
	unsigned int biHeight;
	short biPlanes;
	short biBitCount;
	unsigned int biCompression;
	unsigned int biSizeImage;
	unsigned int biXPelsPerMeter;
	unsigned int biYPelsPerMeter;
	unsigned int biClrUsed;
	unsigned int biClrImportant;
} tBitmapInfoHeader;

grsBitmap *PiggyLoadBitmap (char *pszFile)
{
	CFILE					*fp;
	grsBitmap			*bmp;
	tBitmapFileHeader	bfh;
	tBitmapInfoHeader	bih;

if (!(fp = CFOpen (pszFile, gameFolders.szDataDir, "rb", 0)))
	return NULL;

bfh.bfType = CFReadShort (fp);
bfh.bfSize = (unsigned int) CFReadInt (fp);
bfh.bfReserved1 = CFReadShort (fp);
bfh.bfReserved2 = CFReadShort (fp);
bfh.bfOffBits = (unsigned int) CFReadInt (fp);

bih.biSize = (unsigned int) CFReadInt (fp);
bih.biWidth = (unsigned int) CFReadInt (fp);
bih.biHeight = (unsigned int) CFReadInt (fp);
bih.biPlanes = CFReadShort (fp);
bih.biBitCount = CFReadShort (fp);
bih.biCompression = (unsigned int) CFReadInt (fp);
bih.biSizeImage = (unsigned int) CFReadInt (fp);
bih.biXPelsPerMeter = (unsigned int) CFReadInt (fp);
bih.biYPelsPerMeter = (unsigned int) CFReadInt (fp);
bih.biClrUsed = (unsigned int) CFReadInt (fp);
bih.biClrImportant = (unsigned int) CFReadInt (fp);

if (!(bmp = GrCreateBitmap (bih.biWidth, bih.biHeight, 1))) {
	CFClose (fp);
	return NULL;
	}
CFSeek (fp, bfh.bfOffBits, SEEK_SET);
if (CFRead (bmp->bm_texBuf, bih.biWidth * bih.biHeight, 1, fp) != 1) {
	GrFreeBitmap (bmp);
	return NULL;
	}
CFClose (fp);
return bmp;
}

//------------------------------------------------------------------------------
//eof
