/* $Id: palette.c,v 1.11 2004/05/12 22:06:02 btb Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "pstypes.h"
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "cfile.h"
#include "error.h"
#include "mono.h"
#include "fix.h"
#include "text.h"
//added/remove by dph on 1/9/99
//#include "key.h"
//end remove

#include "palette.h"

//#define Sqr(x) ((x)*(x))

ubyte *defaultPalette = NULL;
ubyte *fadePalette = NULL;

ubyte grFadeTable[256*GR_FADE_LEVELS];


//------------------------------------------------------------------------------

void GrSetPaletteGamma (int gamma)
{
CLAMP (gamma, 0, 16);
if (gameData.render.nPaletteGamma != gamma) {
	gameData.render.nPaletteGamma = gamma;
#if 0
	if (!gameStates.render.bPaletteFadedOut)
		GrPaletteStepLoad (NULL);
#endif
	}
}

//------------------------------------------------------------------------------

int GrGetPaletteGamma()
{
return gameData.render.nPaletteGamma;
}

//------------------------------------------------------------------------------

void GrCopyPalette (ubyte *pCurPal, ubyte *pal, int size)
{
gameData.render.nComputedColors = 0;
}

//------------------------------------------------------------------------------

ubyte *GrUsePaletteTable (const char *pszFile, const char *pszLevel)
{
	CFILE		cf;
	int		i = 0, fsize;
	tPalette	palette;
#ifdef SWAP_0_255
	ubyte		c;
#endif

if (pszLevel) {
	char ifile_name [FILENAME_LEN];

	ChangeFilenameExtension (ifile_name, pszLevel, ".pal");
	i = CFOpen (&cf, ifile_name, gameFolders.szDataDir, "rb", 0);
	}
if (!i)
	i = CFOpen (&cf, pszFile, gameFolders.szDataDir, "rb", 0);
	// the following is a hack to enable the loading of d2 levels
	// even if only the d2 mac shareware datafiles are present.
	// However, if the pig file is present but the palette file isn't,
	// the textures in the level will look wierd...
if (!i)
	i = CFOpen (&cf, DEFAULT_LEVEL_PALETTE, gameFolders.szDataDir, "rb", 0);
if (!i) {
	Error(TXT_PAL_FILES, pszFile, DEFAULT_LEVEL_PALETTE);
	return NULL;
	}
fsize	= CFLength (&cf, 0);
Assert (fsize == 9472);
CFRead (palette, 256*3, 1, &cf);
CFRead (grFadeTable, 256*34, 1, &cf);
CFClose (&cf);
// This is the TRANSPARENCY COLOR
for (i = 0; i < GR_FADE_LEVELS; i++)
	grFadeTable [i * 256 + 255] = 255;
gameData.render.nComputedColors = 0;	//	Flush palette cache.
// swap colors 0 and 255 of the palette along with fade table entries
#ifdef SWAP_0_255
for (i = 0; i < 3; i++) {
	c = palette [i];
	palette [i] = palette [765 + i];
	palette [765 + i] = c;
	}
for (i = 0; i < GR_FADE_LEVELS * 256; i++)
	if (grFadeTable [i] == 0)
		grFadeTable [i] = 255;
for (i = 0; i < GR_FADE_LEVELS; i++)
	grFadeTable [i * 256] = TRANSPARENCY_COLOR;
#endif
return fadePalette = AddPalette (palette);
}

//	-----------------------------------------------------------------------------
//	Add a computed color (by GrFindClosestColor) to list of computed colors in computedColors.
//	If list wasn't full already, increment gameData.render.nComputedColors.
//	If was full, replace a random one.
void AddComputedColor (tPaletteList *plP, int r, int g, int b, int nColor)
{
	int	i;

if (plP->nComputedColors < MAX_COMPUTED_COLORS) {
	i = plP->nComputedColors;
	plP->nComputedColors++;
	} 
else
	i = (d_rand() * MAX_COMPUTED_COLORS) >> 15;
plP->computedColors [i].r = r;
plP->computedColors [i].g = g;
plP->computedColors [i].b = b;
plP->computedColors [i].nColor = nColor;
}

//	-----------------------------------------------------------------------------

void InitComputedColors (tPaletteList *plP)
{
memset (plP->computedColors, 0xff, sizeof (plP->computedColors));
}

//	-----------------------------------------------------------------------------

inline int Sqr (int i)
{
return i * i;
}

//	-----------------------------------------------------------------------------

inline int ColorDelta (ubyte *palette, int r, int g, int b, int j)
{
return Sqr (r - palette [j]) + Sqr (g - palette [j + 1]) + Sqr (b - palette [j + 2]);
}

//	-----------------------------------------------------------------------------

int GrFindClosestColor (ubyte *palette, int r, int g, int b)
{
	int				i, j, n;
	int				nBestValue, nBestIndex, value;
	color_record	*pci, *pcj;
	tPaletteList	*plP;

if (!palette)
	palette = defaultPalette;
if (!palette)
	return -1;
plP = (tPaletteList *) palette;
n = plP->nComputedColors;
//if (!n)
//	InitComputedColors (plP);

//	If we've already computed this color, return it!
pci = plP->computedColors;
for (i = 0; i < n; i++, pci++)
	if ((r == pci->r) && (g == pci->g) && (b == pci->b)) {
		if (i <= 4)
			return pci->nColor;
		else {
			color_record	trec;
			pcj = pci - 1;
			trec = *pcj;
			*pcj = *pci;
			*pci = trec;
			return pcj->nColor;
			}
		}

if (!(nBestValue = ColorDelta (palette, r, g, b, 0))) {
	AddComputedColor (plP, r, g, b, 0);
	return 0;
	}

nBestIndex = 0;
// only go to 255, 'cause we dont want to check the transparent color.
for (i = 1, j = 0; i < 255; i++) {
	j += 3;
	if (!(value = ColorDelta (palette, r, g, b, j))) {
		AddComputedColor (plP, r, g, b, i);
		return i;
		}
	if (value < nBestValue) {
		nBestValue = value;
		nBestIndex = i;
		}
	}
AddComputedColor (plP, r, g, b, nBestIndex);
return nBestIndex;
}

//	-----------------------------------------------------------------------------

int GrFindClosestColor15bpp (int rgb)
{
return GrFindClosestColor (gamePalette, ((rgb >> 10) & 31) * 2, ((rgb >> 5) & 31) * 2, (rgb & 31) * 2);
}

//	-----------------------------------------------------------------------------

int GrFindClosestColorCurrent(int r, int g, int b)
{
return GrFindClosestColor (gameData.render.pal.pCurPal, r, g, b);
}

//	-----------------------------------------------------------------------------

ubyte *FindPalette (ubyte *palette)
{
	tPaletteList	*plP;
#ifdef _DEBUG
	int				i;
#endif

for (plP = gameData.render.pal.palettes; plP; plP = plP->pNextPal)
#ifdef _DEBUG
	{
	for (i = 0; i < 768; i++)
		if (palette [i] != plP->palette [i])
			break;
	if (i == 768)
		return gameData.render.pal.pCurPal = plP->palette;
	}
#else
	if (!memcmp (palette, plP->palette, sizeof (tPalette)))
		return gameData.render.pal.pCurPal = plP->palette;
#endif
return NULL;
}

//	-----------------------------------------------------------------------------

ubyte *AddPalette (ubyte *palette)
{
	tPaletteList	*plP;

if (!palette)
	return NULL;
if (FindPalette (palette))
	return gameData.render.pal.pCurPal;
if (!(plP = (tPaletteList *) D2_ALLOC (sizeof (tPaletteList))))
	return NULL;
plP->pNextPal = gameData.render.pal.palettes;
gameData.render.pal.palettes = plP;
memcpy (plP->palette, palette, sizeof (tPalette));
gameData.render.pal.nPalettes++;
plP->nComputedColors = 0;
memset (plP->computedColors, 0xff, sizeof (plP->computedColors));
return gameData.render.pal.pCurPal = plP->palette;
}

//	-----------------------------------------------------------------------------

void FreePalettes (void)
{
	tPaletteList	*pi, *pj;

for (pi = gameData.render.pal.palettes; pi; pi = pj) {
	pj = pi->pNextPal;
	D2_FREE (pi);
	}
gameData.render.pal.palettes = NULL;
gameData.render.pal.nPalettes = 0;
}

//------------------------------------------------------------------------------

tRgbColors palAddSave;

void PaletteSave (void)
{
palAddSave = gameStates.ogl.palAdd;
}

//------------------------------------------------------------------------------

void GamePaletteStepUp (int r, int g, int b)
{
if (gameStates.render.vr.bUseRegCode)
	;//GrPaletteStepUpVR (r, g, b, VR_WHITE_INDEX, VR_BLACK_INDEX);
else
	GrPaletteStepUp (r, g, b);
}

//------------------------------------------------------------------------------

void PaletteRestore (void)
{
gameStates.ogl.palAdd = palAddSave; 
GamePaletteStepUp (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);
//	Forces flash effect to fixup palette next frame.
gameData.render.xTimeFlashLastPlayed = 0;
}

void DeadPlayerFrame (void);

//------------------------------------------------------------------------------

void FullPaletteSave (void)
{
PaletteSave ();
ResetPaletteAdd ();
GrPaletteStepLoad (NULL);
}

//	-----------------------------------------------------------------------------
//eof
