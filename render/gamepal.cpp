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

#include "descent.h"
#include "rle.h"
#include "gamepal.h"
#include "menu.h"
#include "texmerge.h"
#include "strutil.h"

char szCurrentLevelPalette [SHORT_FILENAME_LEN];

//give a filename a new extension
void LoadBackgroundBitmap(void);

//------------------------------------------------------------------------------

void RemapFontsAndMenus (int bDoFadeTableHack)
{
#if 0
NMRemapBackground();
GrRemapColorFonts();
fontManager.Remap();
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
		c = GrFindClosestColor(gamma[grPalette [i*3]],gamma[grPalette [i*3+1]],gamma[grPalette [i*3+2]]);
		paletteManager.FadeTable ()[14*256+i] = c;
		}
	}
memcpy(last_palette_for_color_fonts,grPalette,sizeof(last_palette_for_color_fonts));
#endif
}

//------------------------------------------------------------------------------
//eof
