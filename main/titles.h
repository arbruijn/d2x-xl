/* $Id: titles.h,v 1.3 2003/10/10 09:36:35 btb Exp $ */
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
 * Header for titles.c
 *
 * Old Log:
 * Revision 2.0  1995/02/27  11:27:40  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.12  1995/02/02  12:23:29  john
 * Made between level saves have picture.
 *
 * Revision 1.11  1995/01/21  16:27:11  matt
 * Made endlevel briefing work with missions
 *
 * Revision 1.10  1995/01/20  22:48:02  matt
 * Mission system implemented, though imcompletely
 *
 * Revision 1.9  1994/12/07  19:14:07  matt
 * Kill briefing screen disable switch for release version
 *
 * Revision 1.8  1994/11/21  16:40:24  matt
 * Added prototype
 *
 * Revision 1.7  1994/11/05  14:05:54  john
 * Fixed fade transitions between all screens by making GrPaletteFadeIn and out keep
 * track of whether the palette is faded in or not.  Then, wherever the code needs to fade out,
 * it just calls GrPaletteFadeOut and it will fade out if it isn't already.  The same with fade_in.
 * This eliminates the need for all the flags like Menu_fade_out, game_fade_in palette, etc.
 *
 * Revision 1.6  1994/11/01  17:57:48  mike
 * Briefing screens.
 *
 * Revision 1.5  1994/10/31  22:28:04  mike
 * briefing screens
 *
 * Revision 1.4  1994/09/16  16:14:31  john
 * Added acrade sequencing.
 *
 * Revision 1.3  1994/06/22  00:26:53  matt
 * Added support for Interplay's animating title sequence
 *
 * Revision 1.2  1994/06/17  12:13:36  john
 * More newdemo stuff; made editor->game transition start in slew mode.
 *
 * Revision 1.1  1994/06/17  10:50:12  john
 * Initial revision
 *
 *
 */

#ifndef _TITLES_H
#define _TITLES_H

extern int	Skip_briefing_screens;

// Descent 1 briefings

extern int ShowTitleScreen (char *filename, int bAllowKeys, int from_hog_only);
extern int ShowBriefingScreen (char *filename, int bAllowKeys);
extern void ShowTitleFlick (char *name, int bAllowKeys);
extern void DoBriefingScreens(char *filename,int nLevel);
extern char *GetBriefingScreen (int nLevel);

extern void show_endgame_briefing(void);

#endif
