/* $Id: menu.h,v 1.7 2003/10/10 09:36:35 btb Exp $ */
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
 * Menu prototypes and variables
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:59:14  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/06  16:47:41  mike
 * destination saturn
 *
 * Revision 2.0  1995/02/27  11:29:47  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.34  1994/12/12  00:16:16  john
 * Added auto-leveling flag.
 *
 * Revision 1.33  1994/12/07  20:04:26  mike
 * set Max_debris_objects.
 *
 * Revision 1.32  1994/11/14  17:23:19  rob
 * ADded extern for default difficulty settings.
 *
 * Revision 1.31  1994/11/10  11:08:29  mike
 * detail level stuff.
 *
 * Revision 1.30  1994/11/05  14:05:53  john
 * Fixed fade transitions between all screens by making GrPaletteFadeIn and out keep
 * track of whether the palette is faded in or not.  Then, wherever the code needs to fade out,
 * it just calls GrPaletteFadeOut and it will fade out if it isn't already.  The same with fade_in.
 * This eliminates the need for all the flags like Menu_fade_out, game_fade_in palette, etc.
 *
 * Revision 1.29  1994/11/02  11:59:44  john
 * Moved menu out of game into inferno main loop.
 *
 * Revision 1.28  1994/10/17  20:30:59  john
 * Made the text for the difficulty labels global so that
 * the high score screen can print "rookie" or whatever.
 *
 * Revision 1.27  1994/10/04  10:47:09  matt
 * Made main menu remember selected item
 *
 * Revision 1.26  1994/08/10  19:55:19  john
 * Changed font stuff; Took out old menu; messed up lots of
 * other stuff like game sequencing messages, etc.
 *
 * Revision 1.25  1994/06/23  18:54:09  matt
 * Cleaned up game start/menu interaction, and improved main menu a little
 *
 * Revision 1.24  1994/06/21  12:11:50  yuan
 * Fixed up menus and added HUDisplay messages.
 *
 * Revision 1.23  1994/06/20  23:15:16  yuan
 * Color switching capability for the menus.
 *
 * Revision 1.22  1994/06/20  22:02:25  yuan
 * Made menu GREEN by POPULAR DEMAND!!
 *
 * Revision 1.21  1994/06/20  21:05:48  yuan
 * Fixed up menus.
 *
 * Revision 1.20  1994/06/20  19:19:29  yuan
 * Tidied up the menu and the "message blocks" between levels, etc.
 *
 * Revision 1.19  1994/06/17  18:01:10  john
 * A bunch of new stuff by John
 *
 * Revision 1.18  1994/05/16  09:37:22  matt
 * Got rid of global continue_flag
 *
 * Revision 1.17  1994/05/14  17:14:51  matt
 * Got rid of externs in source (non-header) files
 *
 * Revision 1.16  1994/05/10  12:14:26  yuan
 * Game save/load... Demo levels 1-5 added...
 * High scores fixed...
 *
 * Revision 1.15  1994/05/05  09:21:21  yuan
 * *** empty log message ***
 *
 * Revision 1.14  1994/04/29  14:55:40  mike
 * Change some menu colors.
 *
 * Revision 1.13  1994/04/28  18:04:36  yuan
 * Gamesave added.
 * Trigger problem fixed (seg pointer is replaced by index now.)
 *
 * Revision 1.12  1994/02/18  11:55:01  yuan
 * Fixed menu to be called from game.
 *
 * Revision 1.11  1994/02/10  17:45:39  yuan
 * Integrated some hacks which still need to be fixed.
 *
 * Revision 1.10  1994/02/01  22:50:23  yuan
 * Final menu version for demo
 *
 * Revision 1.9  1994/02/01  11:50:17  yuan
 * Moved quit message down just a tiny bit
 *
 * Revision 1.8  1994/01/31  17:30:16  yuan
 * Fixed quit not disappearing problem
 *
 * Revision 1.7  1994/01/31  12:25:20  yuan
 * New menu stuff
 *
 * Revision 1.6  1994/01/26  13:14:04  john
 * *** empty log message ***
 *
 * Revision 1.5  1993/12/29  16:44:44  yuan
 * Added some function definitions
 *
 * Revision 1.4  1993/12/13  18:53:12  yuan
 * Fixed dependency problem
 *
 * Revision 1.3  1993/12/12  13:53:51  yuan
 * Added menu and -g flag
 *
 * Revision 1.2  1993/12/10  16:07:17  yuan
 * Working on menu system.  Updated the title screen.
 *
 * Revision 1.1  1993/12/10  12:45:27  yuan
 * Initial revision
 *
 *
 */

#ifndef _MENU_H
#define _MENU_H

// returns number of item chosen
int CallMenu();
void OptionsMenu();
void RenderOptionsMenu ();
void gameplay_options_menu();

#ifdef RELEASE  // read only from hog file
#define MENU_PCX_MAC_SHARE ("\x01menub.pcx")
#define MENU_PCX_SHAREWARE ("\x01menud.pcx")
#define MENU_PCX_OEM (gameStates.menus.bHires?"\x01menuob.pcx":"\x01menuo.pcx")
#define MENU_PCX_FULL (gameStates.menus.bHires?"\x01menub.pcx":"\x01menu.pcx")
#else
#define MENU_PCX_MAC_SHARE ("menub.pcx")
#define MENU_PCX_SHAREWARE ("menud.pcx")
#define MENU_PCX_OEM (gameStates.menus.bHires?"menuob.pcx":"menuo.pcx")
#define MENU_PCX_FULL (gameStates.menus.bHires?"menub.pcx":"menu.pcx")
#endif

// name of background bitmap
#define Menu_pcx_name \
			(CFExist(MENU_PCX_FULL,gameFolders.szDataDir,0)?MENU_PCX_FULL:\
			(CFExist(MENU_PCX_OEM,gameFolders.szDataDir,0)?MENU_PCX_OEM:\
			CFExist(MENU_PCX_SHAREWARE,gameFolders.szDataDir,0)?MENU_PCX_SHAREWARE:MENU_PCX_MAC_SHARE))

void InitDetailLevels(int detail_level);

int SwitchDisplayMode (int dir);

extern char *menu_difficulty_text[];
extern int Player_default_difficulty;
extern int Max_debris_objects;
extern int Missile_view_enabled;
extern int Escort_view_enabled;
extern int Cockpit_rear_view;

#define EXPMODE_DEFAULTS 0

#define NMCLAMP(_v,_min,_max)	((_v) < (_min) ? (_min) : (_v) > (_max) ? (_max) : (_v))
#define NMBOOL(_v) ((_v) != 0)

#define	ADD_CHECK(_n,_t,_v,_k,_h) \
			m [_n].type = NM_TYPE_CHECK, m [_n].text = _t, m [_n].value = NMBOOL(_v), m [_n].key=_k, m [_n].szHelp = _h
#define	ADD_RADIO(_n,_t,_v,_k,_g,_h) \
			m [_n].type = NM_TYPE_RADIO, m [_n].text = _t, m [_n].value = _v, m [_n].key=_k, m [_n].group=_g, m [_n].szHelp = _h
#define	ADD_MENU(_n,_t,_k,_h) \
			m [_n].type = NM_TYPE_MENU, m [_n].text = _t, m [_n].key = _k, m [_n].szHelp = _h
#define	ADD_TEXT(_n,_t,_k)  \
			m [_n].type = NM_TYPE_TEXT, m [_n].text = _t, m [_n].key = _k
#define	ADD_SLIDER(_n,_t,_v,_min,_max,_k,_h) \
			m [_n].type = NM_TYPE_SLIDER, m [_n].text = _t, m [_n].value = NMCLAMP(_v,_min,_max), m [_n].min_value = _min, m [_n].max_value = _max, m [_n].key = _k, m [_n].szHelp = _h
#define	ADD_INPUT(_n,_t,_l,_h) \
			m [_n].type = NM_TYPE_INPUT, m [_n].text = _t, m [_n].text_len = _l, m [_n].szHelp = _h
#define	ADD_GAUGE(_n,_t,_v,_max) \
			m [_n].type = NM_TYPE_GAUGE, m [_n].text = _t, m [_n].text_len = *_t ? (int) strlen (_t) : 20, m [_n].value = NMCLAMP(_v,0,_max), m [_n].max_value = _max

#define GET_VAL(_v,_n)	if ((_n) >= 0) (_v) = m [_n].value

#define MENU_KEY(_k,_d)	((_k) < 0) ? (_d) : ((_k) == 0) ? 0 : gameStates.app.bEnglish ? toupper (KeyToASCII (_k)) : (_k)

#define PROGRESS_INCR		20
#define PROGRESS_STEPS(_i)	(((_i) + PROGRESS_INCR - 1) / PROGRESS_INCR)

//------------------------------------------------------------------------------

#define INIT_PROGRESS_LOOP(_i,_j,_max) \
	if ((_i) < 0) { \
		(_i) = 0; (_j) = (_max);} \
	else { \
		(_j) = (_i) + PROGRESS_INCR; \
		if ((_j) > (_max)) \
			(_j) = (_max); \
		}


typedef struct {
	int	VGA_mode;
	short	w,h;
	short	render_method;
	short	flags;
	char	isWideScreen;
	char	isAvailable;
} dmi;

#define NUM_DISPLAY_MODES	20

extern dmi displayModeInfo [NUM_DISPLAY_MODES + 1];

int stoip (char *szServerIpAddr, unsigned char *pIpAddr);
int stoport (char *szPort, int *pPort, int *pSign);
int SetCustomDisplayMode (int x, int y);

#endif /* _MENU_H */
