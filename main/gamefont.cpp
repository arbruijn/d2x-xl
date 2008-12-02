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

#include <stdlib.h>

#include "inferno.h"
#include "error.h"
#include "gr.h"
#include "gamefont.h"

// if 1, use high-res versions of fonts

const char * Gamefont_filenames[] = { 
		"font1-1.fnt",      // Font 0
        "font1-1h.fnt",     // Font 0 High-res
        "font2-1.fnt",      // Font 1
        "font2-1h.fnt",     // Font 1 High-res
        "font2-2.fnt",      // Font 2
        "font2-2h.fnt",     // Font 2 High-res
        "font2-3.fnt",      // Font 3
        "font2-3h.fnt",     // Font 3 High-res
        "font3-1.fnt",      // Font 4
        "font3-1h.fnt",     // Font 4 High-res
        };

tFont *Gamefonts[MAX_FONTS];

void _CDECL_ GameFontClose(void)
{
	int i;

if (!gameStates.render.fonts.bInstalled)
	return;
PrintLog ("unloading game fonts\n");
gameStates.render.fonts.bInstalled = 0;
for (i = 0; i < MAX_FONTS; i++) {
	GrCloseFont(Gamefonts[i]);
	Gamefonts[i] = NULL;
	}
}


void GameFontInit()
{
	int i;

if (gameStates.render.fonts.bInstalled)
	return;
gameStates.render.fonts.bInstalled = 1;
gameStates.render.fonts.bHiresAvailable = 1;
// load lores fonts
for (i = 0; i < MAX_FONTS; i += 2)
	Gamefonts [i] = GrInitFont (Gamefont_filenames [i]);
// load hires fonts
for (i = 1; i < MAX_FONTS; i += 2)
	if (!(Gamefonts [i] = GrInitFont (Gamefont_filenames [i])))
		gameStates.render.fonts.bHiresAvailable = 0;
atexit(GameFontClose);
}


