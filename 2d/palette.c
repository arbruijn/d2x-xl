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

/*
 * Graphical routines for setting the palette
 *
 * Old Log:
 * Revision 1.41  1995/02/02  14:26:31  john
 * Made palette fades work better with gamma thingy..
 *
 * Revision 1.40  1994/12/08  19:03:46  john
 * Made functions use cfile.
 *
 * Revision 1.39  1994/12/01  11:23:27  john
 * Limited Gamma from 0-8.
 *
 * Revision 1.38  1994/11/28  01:31:08  mike
 * optimize color lookup function, caching recently used colors.
 *
 * Revision 1.37  1994/11/18  22:50:18  john
 * Changed shorts to ints in parameters.
 *
 * Revision 1.36  1994/11/15  17:54:59  john
 * Made text palette fade in when game over.
 *
 * Revision 1.35  1994/11/10  19:53:14  matt
 * Fixed error handling is GrUsePaletteTable()
 *
 * Revision 1.34  1994/11/07  13:53:48  john
 * Added better gamma stufff.
 *
 * Revision 1.33  1994/11/07  13:37:56  john
 * Added gamma correction stuff.
 *
 * Revision 1.32  1994/11/05  13:20:14  john
 * Fixed bug with find_closest_color_current not working.
 *
 * Revision 1.31  1994/11/05  13:08:09  john
 * MAde it return 0 when palette already faded out.
 *
 * Revision 1.30  1994/11/05  13:05:34  john
 * Added back in code to allow keys during fade.
 *
 * Revision 1.29  1994/11/05  12:49:50  john
 * Fixed bug with previous comment..
 *
 * Revision 1.28  1994/11/05  12:48:46  john
 * Made palette only fade in / out when its supposed to.
 *
 * Revision 1.27  1994/11/05  12:46:43  john
 * Changed palette stuff a bit.
 *
 * Revision 1.26  1994/11/01  12:59:35  john
 * Reduced palette.256 size.
 *
 * Revision 1.25  1994/10/26  23:55:35  john
 * Took out roller; Took out inverse table.
 *
 * Revision 1.24  1994/10/04  22:03:05  matt
 * Fixed bug: palette wasn't fading all the way out or in
 *
 * Revision 1.23  1994/09/22  16:08:40  john
 * Fixed some palette stuff.
 *
 * Revision 1.22  1994/09/19  11:44:31  john
 * Changed call to allocate selector to the dpmi module.
 *
 * Revision 1.21  1994/09/12  19:28:09  john
 * Fixed bug with unclipped fonts clipping.
 *
 * Revision 1.20  1994/09/12  18:18:39  john
 * Set 254 and 255 to fade to themselves in fadetable
 *
 * Revision 1.19  1994/09/12  14:40:10  john
 * Neatend.
 *
 * Revision 1.18  1994/09/09  09:31:55  john
 * Made find_closest_color not look at superx spot of 254
 *
 * Revision 1.17  1994/08/09  11:27:08  john
 * Add cthru stuff.
 *
 * Revision 1.16  1994/08/01  11:03:51  john
 * MAde it read in old/new palette.256
 *
 * Revision 1.15  1994/07/27  18:30:27  john
 * Took away the blending table.
 *
 * Revision 1.14  1994/06/09  10:39:52  john
 * In fade out.in functions, returned 1 if key was pressed...
 *
 * Revision 1.13  1994/05/31  19:04:16  john
 * Added key to stop fade if desired.
 *
 * Revision 1.12  1994/05/06  12:50:20  john
 * Added supertransparency; neatend things up; took out warnings.
 *
 * Revision 1.11  1994/05/03  19:39:02  john
 * *** empty log message ***
 *
 * Revision 1.10  1994/04/22  11:16:07  john
 * *** empty log message ***
 *
 * Revision 1.9  1994/04/08  16:59:40  john
 * Add fading poly's; Made palette fade 32 instead of 16.
 *
 * Revision 1.8  1994/03/16  17:21:17  john
 * Added slow palette searching options.
 *
 * Revision 1.7  1994/01/07  11:47:33  john
 * made use cflib
 *
 * Revision 1.6  1993/12/21  11:41:04  john
 * *** empty log message ***
 *
 * Revision 1.5  1993/12/09  15:02:47  john
 * Changed palette stuff majorly
 *
 * Revision 1.4  1993/12/07  12:31:41  john
 * moved bmd_palette to gameData.render.pal.pCurPal
 *
 * Revision 1.3  1993/10/15  16:22:23  john
 * *** empty log message ***
 *
 * Revision 1.2  1993/09/26  18:59:46  john
 * fade stuff
 *
 * Revision 1.1  1993/09/08  11:44:03  john
 * Initial revision
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "cfile.h"
#include "error.h"
#include "mono.h"
#include "fix.h"
#include "inferno.h"
#include "text.h"
//added/remove by dph on 1/9/99
//#include "key.h"
//end remove

#include "palette.h"

//#define Sqr(x) ((x)*(x))

ubyte *defaultPalette = NULL;

ubyte grFadeTable[256*GR_FADE_LEVELS];

//------------------------------------------------------------------------------

void GrSetPaletteGamma(int gamma)
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

void GrCopyPalette(ubyte *pCurPal, ubyte *pal, int size)
{
gameData.render.nComputedColors = 0;
}

//------------------------------------------------------------------------------

extern void ChangeFilenameExtension(char *dest, char *src, char *new_ext);

ubyte *GrUsePaletteTable (char *pszFile, char *pszLevel)
{
	CFILE		*fp = NULL;
	int		i, fsize;
	tPalette	palette;
#ifdef SWAP_0_255
	ubyte		c;
#endif

if (pszLevel) {
	char ifile_name [FILENAME_LEN];
	
	ChangeFilenameExtension (ifile_name, pszLevel, ".pal");
	fp = CFOpen (ifile_name, gameFolders.szDataDir, "rb", 0);
	}
if (!fp)
	fp = CFOpen (pszFile, gameFolders.szDataDir, "rb", 0);
	// the following is a hack to enable the loading of d2 levels
	// even if only the d2 mac shareware datafiles are present.
	// However, if the pig file is present but the palette file isn't,
	// the textures in the level will look wierd...
if (!fp)
	fp = CFOpen (DEFAULT_LEVEL_PALETTE, gameFolders.szDataDir, "rb", 0);
if (!fp) {
	Error(TXT_PAL_FILES, pszFile, DEFAULT_LEVEL_PALETTE);
	return NULL;
	}
fsize	= CFLength (fp, 0);
Assert (fsize == 9472);
CFRead (palette, 256*3, 1, fp);
CFRead (grFadeTable, 256*34, 1, fp);
CFClose (fp);
// This is the TRANSPARENCY COLOR
for (i = 0; i < GR_FADE_LEVELS; i++)
	grFadeTable [i * 256 + 255] = 255;
gameData.render.nComputedColors = 0;	//	Flush palette cache.
#if defined(POLY_ACC)
pa_update_clut (palette, 0, 256, 0);
#endif
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
return AddPalette (palette);
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

inline int Sqr (i)
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
if (!(plP = d_malloc (sizeof (tPaletteList))))
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
	d_free (pi);
	}
gameData.render.pal.palettes = NULL;
gameData.render.pal.nPalettes = 0;
}

//	-----------------------------------------------------------------------------
//eof
