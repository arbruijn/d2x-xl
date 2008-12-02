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
 * Font declarations for the game,.
 *
 *
 *
 */


#ifndef _GAMEFONT_H
#define _GAMEFONT_H

#include "gr.h"

// When adding a new font, don't forget to change the filename in
// gamefont.c!!!!!!!!!!!!!!!!!!!!!!!!!!!

// We are interleaving low & high resolution fonts, so to access a
// font you say fontnum+flag where flag is 0 for lowres, 1 for hires

#define GFONT_BIG_1     0
#define GFONT_MEDIUM_1  2
#define GFONT_MEDIUM_2  4
#define GFONT_MEDIUM_3  6
#define GFONT_SMALL     8

#define SMALL_FONT      (Gamefonts[GFONT_SMALL + gameStates.render.fonts.bHires])
#define MEDIUM1_FONT    (Gamefonts[GFONT_MEDIUM_1 + gameStates.render.fonts.bHires])
#define MEDIUM2_FONT    (Gamefonts[GFONT_MEDIUM_2 + gameStates.render.fonts.bHires])
#define MEDIUM3_FONT    (Gamefonts[GFONT_MEDIUM_3 + gameStates.render.fonts.bHires])
#define HUGE_FONT       (Gamefonts[GFONT_BIG_1 + gameStates.render.fonts.bHires])

#define GAME_FONT       SMALL_FONT

#define TITLE_FONT      HUGE_FONT
#define NORMAL_FONT     MEDIUM1_FONT    //normal, non-highlighted item
#define SELECTED_FONT   MEDIUM2_FONT    //highlighted item
#define SUBTITLE_FONT   MEDIUM3_FONT

#define MAX_FONTS 10

extern tFont *Gamefonts[MAX_FONTS];

void GameFontInit();
void _CDECL_ GameFontClose(void);
int get_fontTotal_width(tFont * font);

#endif /* _GAMEFONT_H */
