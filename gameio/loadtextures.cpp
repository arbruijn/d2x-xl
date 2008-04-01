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

char *szAddonTextures [MAX_ADDON_BITMAP_FILES] = {
	"slowmotion#0",
	"bullettime#0",
	"repair-spark#0",
	"energy-spark#0"
	};

static short d2OpaqueDoors [] = {
	440, 451, 463, 477, /*483,*/ 488, 
	500, 508, 523, 550, 556, 564, 572, 579, 585, 593, 536, 
	600, 608, 615, 628, 635, 642, 649, 664, /*672,*/ 687, 
	702, 717, 725, 731, 738, 745, 754, 763, 772, 780, 790,
	806, 817, 827, 838, 849, /*858,*/ 863, 871, 886,
	901,
	-1};

#define RLE_REMAP_MAX_INCREASE 132 /* is enough for d1 pc registered */

static int bLowMemory = 0;
static int bMustWriteHamFile = 0;
static int nBitmapFilesNew = 0;

static hashtable bitmapNames [2];

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

void ChangeFilenameExtension (char *dest, char *src, char *new_ext);
extern char szLastPalettePig [];
extern ubyte bBigPig;

//------------------------------------------------------------------------------

#ifdef _DEBUG
typedef struct tTrackedBitmaps {
	grsBitmap	*bmP;
	int			nSize;
} tTrackedBitmaps;

tTrackedBitmaps	trackedBitmaps [1000000];
int					nTrackedBitmaps = 0;
#endif

void UseBitmapCache (grsBitmap *bmP, int nSize)
{
bitmapCacheUsed += nSize;
if (0x7fffffff < bitmapCacheUsed)
	bitmapCacheUsed = 0;
#ifdef _DEBUG
if (nSize < 0) {
	int i;
	for (i = 0; i < nTrackedBitmaps; i++)
		if (trackedBitmaps [i].bmP == bmP) {
			//CBP (trackedBitmaps [i].nSize != -nSize);
			if (i < --nTrackedBitmaps)
				trackedBitmaps [i] = trackedBitmaps [nTrackedBitmaps];
			trackedBitmaps [nTrackedBitmaps].bmP = NULL;
			trackedBitmaps [nTrackedBitmaps].nSize = 0;
			break;
			}
	}
else {
	trackedBitmaps [nTrackedBitmaps].bmP = bmP;
	trackedBitmaps [nTrackedBitmaps++].nSize = nSize;
	}
#endif
}

//------------------------------------------------------------------------------

int IsOpaqueDoor (int i)
{
	short	*p;

if (i >= 0)
	for (p = d2OpaqueDoors; *p >= 0; p++)
		if (i == gameData.pig.tex.pBmIndex [*p].index)
			return 1;
return 0;
}

//------------------------------------------------------------------------------

#if TEXTURE_COMPRESSION

int SaveS3TC (grsBitmap *bmP, char *pszFolder, char *pszFilename)
{
	CFILE		cf;
	char		szFilename [FILENAME_LEN], szFolder [FILENAME_LEN];

if (!bmP->bmCompressed)
	return 0;
CFSplitPath (pszFilename, szFolder, szFilename + 1, NULL);
strcat (szFilename + 1, ".s3tc");
*szFilename = '\x02';	//don't search lib (hog) files
if (*szFolder)
	pszFolder = szFolder;
if (CFExist (szFilename, pszFolder, 0))
	return 1;
if (!CFOpen (&cf, szFilename + 1, pszFolder, "wb", 0))
	return 0;
if ((CFWrite (&bmP->bmProps.w, sizeof (bmP->bmProps.w), 1, &cf) != 1) ||
	 (CFWrite (&bmP->bmProps.h, sizeof (bmP->bmProps.h), 1, &cf) != 1) ||
	 (CFWrite (&bmP->bmFormat, sizeof (bmP->bmFormat), 1, &cf) != 1) ||
    (CFWrite (&bmP->bmBufSize, sizeof (bmP->bmBufSize), 1, &cf) != 1) ||
    (CFWrite (bmP->bmTexBuf, bmP->bmBufSize, 1, &cf) != 1)) {
	CFClose (&cf);
	return 0;
	}
return !CFClose (&cf);
}

//------------------------------------------------------------------------------

int ReadS3TC (grsBitmap *bmP, char *pszFolder, char *pszFilename)
{
	CFILE		cf;
	char		szFilename [FILENAME_LEN], szFolder [FILENAME_LEN];

if (!gameStates.ogl.bHaveTexCompression)
	return 0;
CFSplitPath (pszFilename, szFolder, szFilename, NULL);
if (!*szFilename)
	CFSplitPath (pszFilename, szFolder, szFilename, NULL);
strcat (szFilename, ".s3tc");
if (*szFolder)
	pszFolder = szFolder;
if (!CFOpen (&cf, szFilename, pszFolder, "rb", 0))
	return 0;
if ((CFRead (&bmP->bmProps.w, sizeof (bmP->bmProps.w), 1, &cf) != 1) ||
	 (CFRead (&bmP->bmProps.h, sizeof (bmP->bmProps.h), 1, &cf) != 1) ||
	 (CFRead (&bmP->bmFormat, sizeof (bmP->bmFormat), 1, &cf) != 1) ||
	 (CFRead (&bmP->bmBufSize, sizeof (bmP->bmBufSize), 1, &cf) != 1)) {
	CFClose (&cf);
	return 0;
	}
if (!(bmP->bmTexBuf = (ubyte *) (ubyte *) D2_ALLOC (bmP->bmBufSize))) {
	CFClose (&cf);
	return 0;
	}
if (CFRead (bmP->bmTexBuf, bmP->bmBufSize, 1, &cf) != 1) {
	CFClose (&cf);
	return 0;
	}
CFClose (&cf);
bmP->bmCompressed = 1;
return 1;
}

#endif

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
if (bmP->bmTexBuf) {
	D2_FREE (bmP->bmTexBuf);
	UseBitmapCache (bmP, (int) -bmP->bmProps.h * (int) bmP->bmProps.rowSize);
	}
}

//------------------------------------------------------------------------------

void PiggyFreeMask (grsBitmap *bmP)
{
	grsBitmap	*bmMask;

if ((bmMask = BM_MASK (bmP))) {
	OglFreeBmTexture (bmMask);
	PiggyFreeBitmapData (bmMask);
	GrFreeBitmap (bmMask);
	BM_MASK (bmP) = NULL;
	}
}

//------------------------------------------------------------------------------

int PiggyFreeHiresFrame (grsBitmap *bmP, int bD1)
{

BM_OVERRIDE (gameData.pig.tex.bitmaps [bD1] + bmP->bmHandle) = NULL;
OglFreeBmTexture (bmP);
PiggyFreeMask (bmP);
bmP->bmType = 0;
bmP->bmTexBuf = NULL;
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
else
	PiggyFreeMask (altBmP);
OglFreeBmTexture (altBmP);
D2_FREE (BM_FRAMES (altBmP));
altBmP->bmData.alt.bmFrameCount = 0;
PiggyFreeBitmapData (altBmP);
altBmP->bmPalette = NULL;
altBmP->bmType = 0;
return 1;
}

//------------------------------------------------------------------------------

void PiggyFreeHiresAnimations (void)
{
	int			i, bD1;
	grsBitmap	*bmP;

for (bD1 = 0, bmP = gameData.pig.tex.bitmaps [bD1]; bD1 < 2; bD1++)
	for (i = gameData.pig.tex.nBitmaps [bD1]; i; i--, bmP++)
		PiggyFreeHiresAnimation (bmP, bD1);
}

//------------------------------------------------------------------------------

void PiggyFreeBitmap (grsBitmap *bmP, int i, int bD1)
{
if (!bmP)
	bmP = gameData.pig.tex.bitmaps [bD1] + i;
else if (i < 0)
	i = (int) (bmP - gameData.pig.tex.bitmaps [bD1]);
PiggyFreeMask (bmP);
if (!PiggyFreeHiresAnimation (bmP, 0))
	OglFreeBmTexture (bmP);
if (bitmapOffsets [bD1][i] > 0)
	bmP->bmProps.flags |= BM_FLAG_PAGED_OUT;
bmP->bmFromPog = 0;
bmP->bmPalette = NULL;
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
		if (bmP->bmTexBuf && (bitmapOffsets [bD1][i] > 0)) { // only page out bitmaps read from disk
			bmP->bmProps.flags |= BM_FLAG_PAGED_OUT;
			PiggyFreeBitmap (bmP, i, bD1);
			}
		}
	}
for (i = 0; i < MAX_ADDON_BITMAP_FILES; i++)
	PiggyFreeBitmap (gameData.pig.tex.addonBitmaps + i, i, 0);
}

//------------------------------------------------------------------------------

void GetFlagData (char *bmName, int nIndex)
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
pf->bmi.index = nIndex;
pf->vcP = FindVClip (nIndex);
pf->vci.nClipIndex = gameData.objs.pwrUp.info [46 + i].nClipIndex;	//46 is the blue flag powerup
pf->vci.xFrameTime = gameData.eff.vClips [0][pf->vci.nClipIndex].xFrameTime;
pf->vci.nCurFrame = 0;
}

//------------------------------------------------------------------------------

int IsAnimatedTexture (short nTexture)
{
return (nTexture > 0) && (strchr (gameData.pig.tex.bitmapFiles [gameStates.app.bD1Mission][gameData.pig.tex.pBmIndex [nTexture].index].name, '#') != NULL);
}

//------------------------------------------------------------------------------

void PageInBitmap (grsBitmap *bmP, char *bmName, int nIndex, int bD1)
{
	grsBitmap		*altBmP = NULL;
	int				temp, nSize, nOffset, nFrames, nShrinkFactor, 
						bRedone = 0, bTGA, bDefault = 0;
	time_t			tBase, tShrunk;
	CFILE				cf = {NULL, 0, 0, 0}, *pcf = &cf;
	char				fn [FILENAME_LEN], fnShrunk [FILENAME_LEN];
	tTgaHeader		h;

#ifdef _DEBUG
if (!bmName)
	return;
#endif
if (!bmP->bmTexBuf) {
	StopTime ();
	nShrinkFactor = 1 << (3 - gameOpts->render.textures.nQuality);
	nSize = (int) bmP->bmProps.h * (int) bmP->bmProps.rowSize;
	if (nIndex >= 0)
		GetFlagData (bmName, nIndex);
#ifdef _DEBUG
	if (strstr (bmName, "exp06#0")) {
		sprintf (fn, "%s%s%s.tga", gameFolders.szTextureDir [bD1], 
					*gameFolders.szTextureDir [bD1] ? "/" : "", bmName);
		}
	else
#endif
	sprintf (fn, "%s%s%s.tga", gameFolders.szTextureDir [bD1], 
				*gameFolders.szTextureDir [bD1] ? "/" : "", bmName);
	tBase = CFDate (fn, "", 0);
	if (tBase < 0) 
		*fnShrunk = '\0';
	else {
		sprintf (fnShrunk, "%s%s%s-%d.tga", gameFolders.szTextureCacheDir [bD1], 
					*gameFolders.szTextureDir [bD1] ? "/" : "", bmName, 512 / nShrinkFactor);
		tShrunk = CFDate (fnShrunk, "", 0);
		if (tShrunk < tBase)
			*fnShrunk = '\0';
		}
	bTGA = 0;
	bmP->bmBPP = 1;
	if (gameStates.app.bNostalgia)
		gameOpts->render.textures.bUseHires = 0;
	if (*bmName && ((nIndex < 0) || (gameOpts->render.textures.bUseHires && (!gameOpts->ogl.bGlTexMerge || gameStates.render.textures.bGlsTexMergeOk)))) {
#if 0
		if ((nIndex >= 0) && ReadS3TC (gameData.pig.tex.altBitmaps [bD1] + nIndex, gameFolders.szTextureDir [bD1], bmName)) {
			altBmP = gameData.pig.tex.altBitmaps [bD1] + nIndex;
			altBmP->bmType = BM_TYPE_ALT;
			BM_OVERRIDE (bmP) = altBmP;
			BM_FRAMECOUNT (altBmP) = 1;
			gameData.pig.tex.bitmapFlags [bD1][nIndex] &= ~BM_FLAG_RLE;
			gameData.pig.tex.bitmapFlags [bD1][nIndex] |= BM_FLAG_TGA;
			bmP = altBmP;
			altBmP = NULL;
			}
		else 
#endif
		if ((gameStates.app.bCacheTextures && (nShrinkFactor > 1) && *fnShrunk && CFOpen (pcf, fnShrunk, "", "rb", 0)) || 
			 CFOpen (pcf, fn, "", "rb", 0)) {
			PrintLog ("loading hires texture '%s' (quality: %d)\n", fn, gameOpts->render.nTextureQuality);
			bTGA = 1;
			if (nIndex < 0)
				altBmP = gameData.pig.tex.addonBitmaps - nIndex - 1;
			else
				altBmP = gameData.pig.tex.altBitmaps [bD1] + nIndex;
			altBmP->bmType = BM_TYPE_ALT;
			BM_OVERRIDE (bmP) = altBmP;
			bmP = altBmP;
			ReadTGAHeader (pcf, &h, bmP);
			nSize = (int) h.width * (int) h.height * bmP->bmBPP;
			nFrames = (h.height % h.width) ? 1 : h.height / h.width;
			BM_FRAMECOUNT (bmP) = (ubyte) nFrames;
			nOffset = CFTell (pcf);
			if (nIndex >= 0) {
				gameData.pig.tex.bitmapFlags [bD1][nIndex] &= ~(BM_FLAG_RLE | BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT);
				gameData.pig.tex.bitmapFlags [bD1][nIndex] |= BM_FLAG_TGA;
				if (bmP->bmProps.h > bmP->bmProps.w) {
					tEffectClip	*ecP = NULL;
					tWallClip *wcP;
					tVideoClip *vcP;
					while ((ecP = FindEffect (ecP, nIndex))) {
						//e->vc.nFrameCount = nFrames;
						ecP->flags |= EF_ALTFMT;
						//ecP->vc.flags |= WCF_ALTFMT;
						}
					if (!ecP) {
						if ((wcP = FindWallAnim (nIndex))) {
						//w->nFrameCount = nFrames;
							wcP->flags |= WCF_ALTFMT;
							}
						else if ((vcP = FindVClip (nIndex))) {
							//v->nFrameCount = nFrames;
							vcP->flags |= WCF_ALTFMT;
							}
						else {
							PrintLog ("   couldn't find animation for '%s'\n", bmName);
							}
						}
					}
				}
			}
		}
	if (!altBmP) {
		if (nIndex < 0) {
			StartTime (0);
			return;
			}
		pcf = cfPiggy + bD1;
		nOffset = bitmapOffsets [bD1][nIndex];
		bDefault = 1;
		}

reloadTextures:

	if (bRedone) {
		Error ("Not enough memory for textures.\nTry to decrease texture quality\nin the advanced render options menu.");
#ifndef _DEBUG
		StartTime (0);
		if (!bDefault)
			CFClose (pcf);
		return;
#endif
		}

	bRedone = 1;
	if (CFSeek (pcf, nOffset, SEEK_SET)) {
		PiggyCriticalError ();
		goto reloadTextures;
		}
#if 1//def _DEBUG
	strncpy (bmP->szName, bmName, sizeof (bmP->szName));
#endif
#if TEXTURE_COMPRESSION
	if (bmP->bmCompressed)
		UseBitmapCache (bmP, bmP->bmBufSize);
	else 
#endif
		{
		bmP->bmTexBuf = (ubyte *) D2_ALLOC (nSize);
		if (bmP->bmTexBuf) 
			UseBitmapCache (bmP, nSize);
		}
	if (!bmP->bmTexBuf || (bitmapCacheUsed > bitmapCacheSize)) {
		Int3 ();
		PiggyBitmapPageOutAll (0);
		goto reloadTextures;
		}
	if (nIndex >= 0)
		bmP->bmProps.flags = gameData.pig.tex.bitmapFlags [bD1][nIndex];
	bmP->bmHandle = nIndex;
	if (bmP->bmProps.flags & BM_FLAG_RLE) {
		int zSize = 0;
		nDescentCriticalError = 0;
		zSize = CFReadInt (pcf);
		if (nDescentCriticalError) {
			PiggyCriticalError ();
			goto reloadTextures;
			}
		temp = (int) CFRead (bmP->bmTexBuf + 4, 1, zSize-4, pcf);
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
		if (!bmP->bmCompressed) 
#endif
		{
		nDescentCriticalError = 0;
		if (bDefault) {
			temp = (int) CFRead (bmP->bmTexBuf, 1, nSize, pcf);
			if (bD1)
				GrRemapBitmapGood (bmP, d1Palette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
			else
				GrRemapBitmapGood (bmP, gamePalette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
			}
		else {
			ReadTGAImage (pcf, &h, bmP, -1, 1.0, 0, 0);
			if (bmP->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
				bmP->bmProps.flags |= BM_FLAG_SEE_THRU;
			bmP->bmType = BM_TYPE_ALT;
			if (IsOpaqueDoor (nIndex)) {
				bmP->bmProps.flags &= ~BM_FLAG_TRANSPARENT;
				bmP->bmTransparentFrames [0] &= ~1;
				}
#if TEXTURE_COMPRESSION
			if (CompressTGA (bmP))
				SaveS3TC (bmP, gameFolders.szTextureDir [bD1], bmName);
			else 
#endif
			if ((nShrinkFactor > 1) && (bmP->bmProps.w == 512) && ShrinkTGA (bmP, nShrinkFactor, nShrinkFactor, 1)) {
				nSize /= (nShrinkFactor * nShrinkFactor);
				if (gameStates.app.bCacheTextures)
					SaveTGA (bmName, gameFolders.szTextureCacheDir [bD1], &h, bmP);
				}
			}
		if (nDescentCriticalError) {
			PiggyCriticalError ();
			goto reloadTextures;
			}
		}
#ifndef MACDATA
	if (!bTGA && IsMacDataFile (pcf, bD1))
		swap_0_255 (bmP);
#endif
	StartTime (0);
	}
if (!bDefault)
	CFClose (pcf);
}

//------------------------------------------------------------------------------

void PiggyBitmapPageIn (int bmi, int bD1)
{
	grsBitmap		*bmP;
	int				bmiSave;

	//bD1 = gameStates.app.bD1Mission;
bmiSave = 0;
#if 0
Assert (bmi >= 0);
Assert (bmi < MAX_BITMAP_FILES);
Assert (bmi < gameData.pig.tex.nBitmaps [bD1]);
Assert (bitmapCacheSize > 0);
#endif
if (bmi < 1) 
	return;
if (bmi >= MAX_BITMAP_FILES) 
	return;
if (bmi >= gameData.pig.tex.nBitmaps [bD1]) 
	return;
if (bitmapOffsets [bD1][bmi] == 0) 
	return;		// A read-from-disk bmi!!!

if (bLowMemory) {
	bmiSave = bmi;
	bmi = gameData.pig.tex.bitmapXlat [bmi];          // Xlat for low-memory settings!
	}
bmP = BmOverride (gameData.pig.tex.bitmaps [bD1] + bmi, -1);
PageInBitmap (bmP, gameData.pig.tex.bitmapFiles [bD1][bmi].name, bmi, bD1);

if (bLowMemory) {
	if (bmiSave != bmi) {
		gameData.pig.tex.bitmaps [bD1][bmiSave] = gameData.pig.tex.bitmaps [bD1][bmi];
		bmi = bmiSave;
		}
	}
gameData.pig.tex.bitmaps [bD1][bmi].bmProps.flags &= ~BM_FLAG_PAGED_OUT;
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

void LoadBitmapReplacements (char *pszLevelName)
{
	char			szFilename [SHORT_FILENAME_LEN];
	CFILE			cf;
	int			i, j;
	grsBitmap	bm;

	//first, D2_FREE up data allocated for old bitmaps
PrintLog ("   loading replacement textures\n");
ChangeFilenameExtension (szFilename, pszLevelName, ".pog");
if (CFOpen (&cf, szFilename, gameFolders.szDataDir, "rb", 0)) {
	int					id, version, nBitmapNum, bTGA;
	int					bmDataSize, bmDataOffset, bmOffset;
	ushort				*indices;
	tPIGBitmapHeader	*bmh;

	id = CFReadInt (&cf);
	version = CFReadInt (&cf);
	if (id != MAKE_SIG ('G','O','P','D') || version != 1) {
		CFClose (&cf);
		return;
		}
	nBitmapNum = CFReadInt (&cf);
	MALLOC (indices, ushort, nBitmapNum);
	MALLOC (bmh, tPIGBitmapHeader, nBitmapNum);
#if 0
	CFRead (indices, nBitmapNum * sizeof (ushort), 1, &cf);
	CFRead (bmh, nBitmapNum * sizeof (tPIGBitmapHeader), 1, &cf);
#else
	for (i = 0; i < nBitmapNum; i++)
		indices [i] = CFReadShort (&cf);
	for (i = 0; i < nBitmapNum; i++)
		PIGBitmapHeaderRead (bmh + i, &cf);
#endif
	bmDataOffset = CFTell (&cf);
	bmDataSize = CFLength (&cf, 0) - bmDataOffset;

	for (i = 0; i < nBitmapNum; i++) {
		bmOffset = bmh [i].offset;
		memset (&bm, 0, sizeof (grsBitmap));
		bm.bmProps.flags |= bmh [i].flags & (BM_FLAGS_TO_COPY | BM_FLAG_TGA);
		bm.bmProps.w = bm.bmProps.rowSize = bmh [i].width + ((short) (bmh [i].wh_extra & 0x0f) << 8);
		if ((bTGA = (bm.bmProps.flags & BM_FLAG_TGA)) && (bm.bmProps.w > 256))
			bm.bmProps.h = bm.bmProps.w * bmh [i].height;
		else
			bm.bmProps.h = bmh [i].height + ((short) (bmh [i].wh_extra & 0xf0) << 4);
		bm.bmBPP = bTGA ? 4 : 1;
		if (!(bm.bmProps.w * bm.bmProps.h))
			continue;
		if (bmOffset + bm.bmProps.h * bm.bmProps.rowSize > bmDataSize)
			break;
		bm.bmAvgColor = bmh [i].bmAvgColor;
		bm.bmType = BM_TYPE_ALT;
		if (!(bm.bmTexBuf = (ubyte *) GrAllocBitmapData (bm.bmProps.w, bm.bmProps.h, bm.bmBPP)))
			break;
		CFSeek (&cf, bmDataOffset + bmOffset, SEEK_SET);
		if (bTGA) {
			int			nFrames = bm.bmProps.h / bm.bmProps.w;
			tTgaHeader	h;

			h.width = bm.bmProps.w;
			h.height = bm.bmProps.h;
			h.bits = 32;
			if (!ReadTGAImage (&cf, &h, &bm, -1, 1.0, 0, 1)) {
				D2_FREE (bm.bmTexBuf);
				break;
				}
			bm.bmProps.rowSize *= bm.bmBPP;
			bm.bmData.alt.bmFrameCount = (ubyte) nFrames;
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
			bm.bmHandle = j;
			}
		else {
			int nSize = (int) bm.bmProps.w * (int) bm.bmProps.h;
			if (nSize != (int) CFRead (bm.bmTexBuf, 1, nSize, &cf)) {
				D2_FREE (bm.bmTexBuf);
				break;
				}
			bm.bmPalette = gamePalette;
			j = indices [i];
			bm.bmHandle = j;
			rle_expand (&bm, NULL, 0);
			bm.bmProps = gameData.pig.tex.pBitmaps [j].bmProps;
			GrRemapBitmapGood (&bm, gamePalette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
			}
		PiggyFreeBitmap (NULL, j, 0);
		bm.bmFromPog = 1;
		if (*gameData.pig.tex.pBitmaps [j].szName)
			sprintf (bm.szName, "[%s]", gameData.pig.tex.pBitmaps [j].szName);
		else
			sprintf (bm.szName, "POG#%04d", j);
		gameData.pig.tex.pAltBitmaps [j] = bm;
		BM_OVERRIDE (gameData.pig.tex.pBitmaps + j) = gameData.pig.tex.pAltBitmaps + j;
		{
		tRgbColorf	*c;
		grsBitmap	*bmP = gameData.pig.tex.pAltBitmaps + j;

		c = BitmapColor (bmP, NULL);
		if (c && !bmP->bmAvgColor)
			bmP->bmAvgColor = GrFindClosestColor (bmP->bmPalette, (int) c->red, (int) c->green, (int) c->blue);
		}
		UseBitmapCache (gameData.pig.tex.pAltBitmaps + j, (int) bm.bmProps.h * (int) bm.bmProps.rowSize);
		}
	D2_FREE (indices);
	D2_FREE (bmh);
	CFClose (&cf);
	szLastPalettePig [0] = 0;  //force pig re-load
	TexMergeFlush ();       //for re-merging with new textures
	}
}

//------------------------------------------------------------------------------
//eof
