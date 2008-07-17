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

/*
 *
 * Functions for loading palettes
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "inferno.h"
#include "rle.h"
#include "gamepal.h"
#include "newmenu.h"
#include "texmerge.h"
#include "strutil.h"

char szCurrentLevelPalette[SHORT_FILENAME_LEN];

//give a filename a new extension
void ChangeFilenameExt( char *dest, char *src, char *ext )
{
	char *p;

	strcpy (dest, src);

	if (ext[0] == '.')
		ext++;

	p = strchr(dest,'.');
	if (!p) {
		p = dest + strlen(dest);
		*p = '.';
	}

	strcpy(p+1,ext);
}

void LoadBackgroundBitmap(void);

char szLastPaletteLoaded [FILENAME_LEN] = "";
char szLastPalettePig [FILENAME_LEN] = "";
ubyte *lastPalette = NULL;

ubyte *gamePalette;

//------------------------------------------------------------------------------

void RemapFontsAndMenus (int bDoFadeTableHack)
{
#if 0
NMRemapBackground();
GrRemapColorFonts();
GrRemapMonoFonts();
if (bDoFadeTableHack) {
	int i;
	double g = 1.0;
	double intensity;
	ubyte gamma[64];

	intensity = (double)(14)/(double)(32);
	for (i=0;i<64;i++)
		gamma[i] = (int)((pow(intensity, 1.0/g)*i) + 0.5);
	for (i=0;i<256;i++) {
		int c;
		c = GrFindClosestColor(gamma[grPalette[i*3]],gamma[grPalette[i*3+1]],gamma[grPalette[i*3+2]]);
		grFadeTable[14*256+i] = c;
		}
	}
memcpy(last_palette_for_color_fonts,grPalette,sizeof(last_palette_for_color_fonts));
#endif
}

//------------------------------------------------------------------------------
//load a palette by name. returns 1 if new palette loaded, else 0
//if nUsedForLevel is set, load pig, etc.
//if bNoScreenChange is set, the current screen does not get remapped,
//and the hardware palette does not get changed
ubyte *LoadPalette (const char *pszPaletteName, const char *pszLevelName, int nUsedForLevel, 
						  int bNoScreenChange, int bForce)
{
	char	szPigName[FILENAME_LEN];
	ubyte *palette = NULL;

	//special hack to tell that palette system about a pig that's been loaded elsewhere
if (nUsedForLevel == -2) {
	strncpy (szLastPalettePig, pszPaletteName, sizeof (szLastPalettePig));
	return lastPalette;
	}
if (!pszPaletteName)
	pszPaletteName = szLastPalettePig;
if (!*pszPaletteName)
	pszPaletteName = "groupa.256";
if (nUsedForLevel && stricmp (szLastPalettePig, pszPaletteName) != 0) {
	if (gameStates.app.bD1Mission)
		strcpy (szPigName, "groupa.pig");
	else {
		_splitpath ((char *) pszPaletteName, NULL, NULL, szPigName, NULL);
		strcat (szPigName, ".pig");
		PiggyInitPigFile (szPigName);
		}
//if not editor, load pig first so small install message can come
//up in old palette.  If editor version, we must load the pig after
//the palette is loaded so we can remap new textures.
#ifdef EDITOR
	piggy_new_pigfile(szPigName);
#endif
	}
if (bForce || pszLevelName || stricmp(szLastPaletteLoaded, pszPaletteName)) {
	strncpy (szLastPaletteLoaded, pszPaletteName, sizeof (szLastPaletteLoaded));
	palette = GrUsePaletteTable (pszPaletteName, pszLevelName);
	if (!gameStates.render.bPaletteFadedOut && !bNoScreenChange)
		GrPaletteStepLoad (NULL);
	//RemapFontsAndMenus (0);
	gameData.hud.msgs [0].nColor = -1;
	LoadBackgroundBitmap();
	}
if (nUsedForLevel && stricmp(szLastPalettePig,pszPaletteName) != 0) {
	strncpy(szLastPalettePig,pszPaletteName,sizeof(szLastPalettePig));
#ifdef EDITOR
	piggy_new_pigfile(szPigName);
#endif
	TexMergeFlush();
	RLECacheFlush();
	}
return palette;
}

//------------------------------------------------------------------------------
//eof
