/* $Id: newmenu.c, v 1.25 2003/11/27 04:52:18 btb Exp $ */
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
 * Routines for menus.
 *
 * Old Log:
 * Revision 1.26  1996/04/14  21:07:35  allender
 * some change for update
 *
 * Revision 1.25  1995/10/31  10:19:25  allender
 * shareware stuff
 *
 * Revision 1.24  1995/10/27  11:09:04  allender
 * always cd to descent dir before any menu processing --
 * sanity check because of way mac does volume mounting and
 * other file stuff
 *
 * Revision 1.23  1995/10/24  18:11:19  allender
 * do_appl_quit saves and restores background -- don't
 * do any special processing.  fixed ExecMenu3 to pass width
 * and height to ...do_4
 *
 * Revision 1.22  1995/10/21  23:26:25  allender
 * can't do cmd-q in multiplayer games
 *
 * Revision 1.21  1995/10/21  22:53:25  allender
 * menus need to be 8 byte multiple wide -- printscreen stuff
 *
 * Revision 1.20  1995/10/20  00:50:15  allender
 * redbook stuff and make esc leave menu always
 *
 * Revision 1.19  1995/10/17  13:14:52  allender
 * mouse support for other menus now operational -- dragging
 * mouse will also move sliders
 *
 * Revision 1.18  1995/10/12  17:35:19  allender
 * mouse support for pilot and demos
 *
 * Revision 1.17  1995/10/11  12:17:47  allender
 * removed event loop processing
 *
 * Revision 1.16  1995/10/10  11:51:29  allender
 * align menus to 8 byte boundry
 *
 * Revision 1.15  1995/10/05  14:09:36  allender
 * lowered starting x location on menus to fixup network
 * status screen chopping off characters
 *
 * Revision 1.14  1995/09/24  10:52:27  allender
 * made close boxes work
 * maybe some other minor stuff?
 *
 * Revision 1.13  1995/09/15  09:02:04  allender
 * started putting mouse support in listboxes
 *
 * Revision 1.12  1995/09/13  08:50:14  allender
 * close box in this rev?  Worked on getting list box better
 * looking
 *
 * Revision 1.11  1995/09/01  10:52:37  allender
 * fixed up list boxes
 *
 * Revision 1.10  1995/08/24  16:23:08  allender
 * mouse support for most menus
 *
 * Revision 1.9  1995/08/08  13:46:56  allender
 * added macsys header file
 *
 * Revision 1.8  1995/08/01  16:03:54  allender
 * added file list with Desalvo's directory stuff
 *
 * Revision 1.7  1995/07/26  17:03:37  allender
 * show then hide cursor during newmenu operations -- not quite
 * right yet
 *
 * Revision 1.6  1995/07/17  08:56:26  allender
 * fixed up menus to look better with new high res fonts
 *
 * Revision 1.5  1995/07/12  10:35:52  allender
 * removed old debug thing
 *
 * Revision 1.4  1995/06/23  10:23:21  allender
 * menus should work in any resolution now
 *
 * Revision 1.3  1995/06/20  09:11:19  allender
 * changed menu system to support 640x480 instead of 320x200
 *
 * Revision 1.2  1995/06/13  13:07:30  allender
 * added special key to move 320x200 window to upper left corner of screen
 *
 * Revision 1.1  1995/05/16  15:29:13  allender
 * Initial revision
 *
 * Revision 2.8  1995/05/26  16:16:28  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 *
 * Revision 2.7  1995/04/23  14:54:17  john
 * Fixed bug with background breaking in first menu.
 *
 * Revision 2.6  1995/03/21  14:38:46  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.5  1995/03/15  14:33:29  john
 * Added code to force the Descent CD-rom in the drive.
 *
 * Revision 2.4  1995/03/14  18:24:28  john
 * Force Destination Saturn to use CD-ROM drive.
 *
 * Revision 2.3  1995/03/14  16:22:23  john
 * Added cdrom alternate directory stuff.
 *
 * Revision 2.2  1995/03/06  18:30:51  john
 * Fixed bug with newmenu trashing editor font.
 *
 * Revision 2.1  1995/03/06  15:23:17  john
 * New screen techniques.
 *
 * Revision 2.0  1995/02/27  11:27:55  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.129  1995/02/11  16:19:56  john
 * Added code to make the default mission be the one last played.
 *
 * Revision 1.128  1995/02/02  19:41:33  john
 * Added 10 save game slots.
 *
 * Revision 1.127  1995/02/01  18:13:52  john
 * Fixed some constants.
 *
 * Revision 1.126  1995/02/01  18:04:01  yuan
 * Added 50 characters to list.
 *
 * Revision 1.125  1995/02/01  13:39:35  john
 * Made menu text that changes not overwrite.
 *
 * Revision 1.124  1995/01/31  10:47:57  john
 * Added menu that you can specify the width of the menu.
 *
 * Revision 1.123  1995/01/28  17:18:12  john
 * Added file list box.
 *
 * Revision 1.122  1995/01/27  17:15:55  john
 * Made prev comment actually work.
 *
 * Revision 1.121  1995/01/27  16:49:03  john
 * *** empty log message ***
 *
 * Revision 1.120  1995/01/27  16:46:40  john
 * Made so that input_menu only clears text if -empty-.
 *
 * Revision 1.119  1995/01/27  15:25:04  john
 * Delete saved game when deleteing a pilot file.
 *
 * Revision 1.118  1995/01/25  16:36:09  john
 * Made so that when you hit enter during
 * game save, -empty- goes away.
 *
 * Revision 1.117  1995/01/24  16:59:30  john
 * took out mono debugging text.
 *
 * Revision 1.116  1995/01/24  16:23:16  john
 * Fixed some bugs with listboxes.
 *
 * Revision 1.115  1995/01/23  23:47:36  matt
 * Made keypad enter work in menus
 *
 * Revision 1.114  1995/01/23  19:29:59  john
 * Added scrolling listbox menus.
 *
 * Revision 1.113  1995/01/19  12:33:34  john
 * Made keys jump to sliders, etc in menus.
 *
 * Revision 1.112  1995/01/15  14:33:08  rob
 * Fixed problem with nested nm_messageboxes.
 *
 * Revision 1.111  1995/01/03  17:33:40  john
 * Made scrolling textbox. Used it for scores cool saying.
 *
 * Revision 1.110  1994/12/28  10:42:58  john
 * More VFX tweaking.
 *
 * Revision 1.109  1994/12/28  10:26:39  john
 * Fixed some VFX problems.
 *
 * Revision 1.108  1994/12/15  23:18:10  john
 * Added fix so that VFX mode doesn't hang.
 *
 * Revision 1.107  1994/12/15  12:19:55  john
 * Made menu use clipped bitblt functions.
 *
 * Revision 1.106  1994/12/09  00:41:30  mike
 * fix hang in automap print screen
 *
 * Revision 1.105  1994/12/08  10:01:34  john
 * Changed the way the player callsign stuff works.
 *
 * Revision 1.104  1994/12/04  15:34:30  john
 * Fixed bug with newmenu not restoring font properly.
 *
 * Revision 1.103  1994/12/03  17:47:09  john
 * Fixed bug that didn't d_free filename mem when not .plr files found.
 *
 * Revision 1.102  1994/12/03  15:06:15  john
 * If no pilot exists, bring up box asking for name.
 *
 * Revision 1.101  1994/12/03  11:04:02  john
 * Changed newmenu code a bit to fix bug with bogus
 * backgrounds occcasionally.
 *
 * Revision 1.100  1994/12/01  20:15:48  yuan
 * Localization.
 *
 * Revision 1.99  1994/12/01  10:33:28  john
 * Fixed bug with large menu backgrounds not drawing correctly.
 *
 * Revision 1.98  1994/12/01  02:41:56  john
 * Fixed warnining.
 *
 * Revision 1.97  1994/11/30  22:52:43  john
 * Fixed bug in code that made the backgrounds behind menus.
 *
 * Revision 1.96  1994/11/30  19:47:42  john
 * Added a check for out o' memory when d_mallocing background.
 *
 * Revision 1.95  1994/11/30  19:39:10  john
 * ..
 *
 * Revision 1.94  1994/11/30  19:38:27  john
 * fixed bug with previous.
 *
 * Revision 1.93  1994/11/30  19:36:47  john
 * Made Gravis Ultrasound work again.  Made the scores blink
 * at a constant rate.  Revamped the newmenu background storage, 
 * which hopefully fixed some bugs.  Made menus in ame not pause
 * sound, except for the pause key.               ^== Game!
 *
 * Revision 1.92  1994/11/30  18:06:05  matt
 * When player types space in callsign, comes up as underscore
 *
 * Revision 1.91  1994/11/30  12:28:22  adam
 * added PCX support
 *
 * Revision 1.90  1994/11/30  12:10:59  adam
 * added support for PCX titles/brief screens
 *
 * Revision 1.89  1994/11/29  00:59:12  allender
 * change ExecMenuFileSelector so demo files can be deleted too
 *
 * Revision 1.88  1994/11/27  21:16:18  allender
 * made some return values in ExecMenuFileSelector 0 instead of -1
 *
 * Revision 1.87  1994/11/27  16:58:17  matt
 * Made printscreen work all the time (not just when no ndebug) and made it
 * work when getting a filename.
 *
 * Revision 1.86  1994/11/27  16:47:51  john
 * Made the call to fade in palette only happen if it needs to be, just
 * because I thought it might reduce code paging with vm in menus.
 *
 * Revision 1.85  1994/11/26  15:30:16  matt
 * Allow escape out of change pilot menu
 *
 * Revision 1.84  1994/11/26  14:17:26  matt
 * Player can now only enter valid chars for his name
 *
 * Revision 1.83  1994/11/23  14:13:17  allender
 * if no demo files, displays less "techy" message
 *
 * Revision 1.82  1994/11/21  11:55:52  john
 * Fixed some sound pausing in menus bugs.
 *
 * Revision 1.81  1994/11/19  15:14:58  mike
 * remove unused code and data
 *
 * Revision 1.80  1994/11/18  23:37:54  john
 * Changed some shorts to ints.
 *
 * Revision 1.79  1994/11/15  09:29:21  john
 * Made it so that pressing a letter when selecting players moves to
 * a matching choice.
 *
 * Revision 1.78  1994/11/14  17:12:28  adam
 * *** empty log message ***
 *
 * Revision 1.77  1994/11/14  16:58:31  rob
 * Tried to fix a problem with save demo dialog.
 *
 * Revision 1.76  1994/11/14  16:13:46  matt
 * Fixed handling of players with DOS device names
 *
 * Revision 1.75  1994/11/13  18:12:53  matt
 * Fixed handling of filenames that are the same as DOS devices
 *
 * Revision 1.74  1994/11/13  17:20:44  john
 * Fixed text a bit.
 *
 * Revision 1.73  1994/11/13  17:18:22  john
 * Changed wording of new pilot.
 *
 * Revision 1.72  1994/11/13  17:14:21  john
 * Fixed bug with player list box.
 *
 * Revision 1.71  1994/11/13  17:12:48  john
 * Fixed broken demo file list.
 *
 * Revision 1.70  1994/11/13  17:04:49  john
 * Made the callsign entry be a list box and gave the ability
 * to delete players.
 *
 * Revision 1.69  1994/11/13  15:38:03  john
 * Added critical error handler to game.  Took out -editor command line
 * option because it didn't work anymore and wasn't worth fixing.  Made scores
 * not use MINER enviroment variable on release version, and made scores
 * not print an error if there is no descent.hi.
 *
 * Revision 1.68  1994/11/11  18:17:03  rob
 * Made MultiMenuPoll return a value to exit menus.
 *
 * Revision 1.67  1994/11/11  11:07:06  rob
 * Added include of multi.h
 *
 * Revision 1.66  1994/11/10  20:25:16  rob
 * John's stuff to make network menus work.
 *
 * Revision 1.65  1994/11/08  14:51:39  john
 * Added ExecMessageBox1, (like the original, only you can pass a function).
 *
 * Revision 1.64  1994/11/08  08:30:39  john
 * Fixed bug with centering titles.
 *
 * Revision 1.63  1994/11/08  08:27:00  john
 * Made titles and subtitles center.
 *
 * Revision 1.62  1994/11/07  09:40:48  john
 * Neatend file list box some.
 *
 * Revision 1.61  1994/11/05  17:22:41  john
 * Fixed lots of sequencing problems with newdemo stuff.
 *
 * Revision 1.60  1994/11/05  15:04:08  john
 * Added non-popup menu for the main menu, so that scores and credits don't have to save
 * the background.
 *
 * Revision 1.59  1994/11/05  14:03:52  john
 * Fixed fade transitions between all screens by making GrPaletteFadeIn and out keep
 * track of whether the palette is faded in or not.  Then, wherever the code needs to fade out, 
 * it just calls GrPaletteFadeOut and it will fade out if it isn't already.  The same with fade_in.
 * This eliminates the need for all the flags like Menu_fade_out, game_fade_in palette, etc.
 *
 * Revision 1.58  1994/11/04  20:11:50  john
 * Neatening up palette stuff with demos.
 *
 * Revision 1.57  1994/11/04  13:49:24  allender
 * fixed ExecMenuFileSelector to work with less than 10 files
 *
 * Revision 1.56  1994/11/03  19:37:44  john
 * Added scrolling file list box
 *
 * Revision 1.55  1994/10/31  18:16:42  john
 * Made Pad arrows work with menus.
 *
 * Revision 1.54  1994/10/28  14:54:25  john
 * Added forward dec. for NewMenuClose.
 * .\
 *
 * Revision 1.53  1994/10/28  14:53:00  john
 * Fixed hideous bug that would bomb if you called newmenu_draw_background
 * before any menus were ever displayed.
 *
 * Revision 1.52  1994/10/24  19:56:53  john
 * Made the new user setup prompt for config options.
 *
 * Revision 1.51  1994/10/24  15:15:49  john
 * Made Esc exit ExecMessageBox's, 
 * , 
 *
 * Revision 1.50  1994/10/21  15:20:20  john
 * Made PrtScr do screen dump, not F2.
 *
 * Revision 1.49  1994/10/18  12:33:38  john
 * Only used copy the item text into the saved_text field
 * if it is an inputbox or inputbox_menu.
 *
 * Revision 1.48  1994/10/17  11:04:01  john
 * Made backtab work also.
 *
 * Revision 1.47  1994/10/17  10:47:49  john
 * MAde Tab work like down arrow.
 *
 * Revision 1.46  1994/10/17  10:45:10  john
 * Made the player able to abort death by pressing any button or key.
 *
 * Revision 1.45  1994/10/13  21:52:02  john
 * Made it so that if a messagebox has 1 choice, then
 * Esc will return -1.
 *
 * Revision 1.44  1994/10/13  11:35:38  john
 * Made Thrustmaster FCS Hat work.  Put a background behind the
 * keyboard configure.  Took out turn_sensitivity.  Changed sound/config
 * menu to new menu. Made F6 be calibrate joystick.
 *
 * Revision 1.43  1994/10/11  17:18:52  john
 * Fixed bug with sliders always starting at -1.
 *
 * Revision 1.42  1994/10/11  17:08:29  john
 * Added sliders for volume controls.
 *
 * Revision 1.41  1994/10/06  16:04:40  john
 * Made text items color differently than others. Adam
 * is gonna make a diff colored font for these.
 *
 * Revision 1.40  1994/10/06  15:08:23  rob
 * Allowed any negative key value to abort the menu and return.
 *
 * Revision 1.39  1994/10/04  10:26:06  matt
 * Changed fade in to happen every time a global var is set
 *
 * Revision 1.38  1994/10/04  09:16:08  john
 * If you pass -1 as choice in ExecMenu1, then
 * no item is highlighted until you press up or
 * down arrows.
 *
 * Revision 1.37  1994/10/03  23:44:37  matt
 * Save & restore palette effect around menus & pause message
 *
 * Revision 1.36  1994/10/03  22:59:40  matt
 * Re-enabled backspace to generate Int3 ()
 *
 * Revision 1.35  1994/10/03  19:11:21  matt
 * Changed string input cursor to blinking underscore
 *
 * Revision 1.34  1994/10/03  14:44:15  john
 * Added ExecMenu1, which allows you to pass the starting
 * item to the menu system
 *
 * Revision 1.33  1994/09/30  11:51:21  john
 * Added Matt's NM_TYPE_INPUT_MENU
 *
 * Revision 1.32  1994/09/28  17:22:56  matt
 * Added extra space between subtitle and menu items
 * Made shortcut key check ignore leading spaces in text
 *
 * Revision 1.31  1994/09/15  16:11:22  john
 * Added support for VFX1 head tracking. Fixed bug with memory over-
 * write when using stereo mode.
 *
 * Revision 1.30  1994/09/12  09:52:59  john
 * Made global flush function that flushes keyboard, mouse, and joystick.
 *
 * Revision 1.29  1994/09/10  19:10:54  matt
 * Fixed a few things (like arrow key handling) for menus with all
 * text items, such as the key help message.
 *
 * Revision 1.28  1994/09/01  18:55:38  john
 * freed scores.lbm
 *
 * Revision 1.27  1994/09/01  18:03:50  john
 * Neatened up scores a bit.
 *
 * Revision 1.26  1994/08/30  20:38:13  john
 * Passed cItem in newmenu sub.
 *
 * Revision 1.25  1994/08/30  11:13:01  john
 * Added beveled edges to menus.
 *
 * Revision 1.24  1994/08/26  13:01:58  john
 * Put high score system in.
 *
 * Revision 1.23  1994/08/16  00:18:44  john
 * Made pressing the first letter of a menu
 * item move to it.
 *
 * Revision 1.22  1994/08/15  23:17:43  john
 * *** empty log message ***
 *
 * Revision 1.21  1994/08/15  23:15:28  john
 * Made 1 menu/checkbox return with any keypress.
 *
 * Revision 1.20  1994/08/12  10:18:23  john
 * *** empty log message ***
 *
 * Revision 1.19  1994/08/12  10:09:11  john
 * Made borders better.
 *
 * Revision 1.18  1994/08/12  03:11:16  john
 * Made network be default off; Moved network options into
 * main menu.  Made starting net game check that mines are the
 * same.
 *
 * Revision 1.17  1994/08/11  22:14:43  john
 * Free'd up some memory that I forgot to d_free.
 *
 * Revision 1.16  1994/08/11  19:27:35  john
 * Made the Backspace drop into the debugger only
 * if you're not in an inputbox.
 *
 * Revision 1.15  1994/08/11  18:01:49  matt
 * Added F2 and BACKSPACE keys to new menu system
 *
 * Revision 1.14  1994/08/11  14:25:58  john
 * *** empty log message ***
 *
 * Revision 1.13  1994/08/11  14:25:40  john
 * *** empty log message ***
 *
 * Revision 1.12  1994/08/11  13:47:02  john
 * Made newmenu have subtitles, passed key through to
 * the newmenu subfunctions.
 *
 * Revision 1.11  1994/08/11  12:45:08  john
 * *** empty log message ***
 *
 * Revision 1.10  1994/08/11  12:25:45  john
 * Made right_offset spacing different
 *
 * Revision 1.9  1994/08/11  12:09:49  john
 * Made work with bitmapped fonts.
 *
 * Revision 1.8  1994/08/10  19:56:16  john
 * Changed font stuff; Took out old menu; messed up lots of
 * other stuff like game sequencing messages, etc.
 *
 * Revision 1.7  1994/07/27  16:12:23  john
 * Changed newmenu system to have a callback function.
 * /.
 *
 * Revision 1.6  1994/07/25  15:10:23  john
 * *** empty log message ***
 *
 * Revision 1.5  1994/07/25  12:33:35  john
 * Network "pinging" in.
 *
 * Revision 1.4  1994/07/24  18:21:27  john
 * Took out first time stuff.
 *
 * Revision 1.3  1994/07/24  17:32:47  john
 * Added percent item.  Also neatend up a bit.
 *
 * Revision 1.2  1994/07/22  17:48:13  john
 * Added new menuing system.
 *
 * Revision 1.1  1994/07/22  13:55:38  john
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif
#ifdef __macosx__
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <cType.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <limits.h>

#include "pa_enabl.h"                   //$$POLY_ACC
#include "error.h"
#include "inferno.h"
#include "gr.h"
#include "mono.h"
#include "songs.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "text.h"
#include "findfile.h"

#include "menu.h"
#include "newmenu.h"
#include "gamefont.h"
#include "gamepal.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "mouse.h"
#include "joy.h"
#include "digi.h"

#include "multi.h"
#include "endlevel.h"
#include "screens.h"
#include "config.h"
#include "player.h"
#include "newdemo.h"
#include "kconfig.h"
#include "strutil.h"
#include "ogl_init.h"
#include "render.h"
#include "input.h"
#include "gameseq.h"

#if defined (TACTILE)
 #include "tactile.h"
#endif

#if defined (POLY_ACC)
#include "poly_acc.h"
#endif

#define MAXDISPLAYABLEITEMS 15

#define LHX(x)      (gameStates.menus.bHires? 2 * (x) : x)
#define LHY(y)      (gameStates.menus.bHires? (24 * (y)) / 10 : y)

#define NORMAL_CHECK_BOX    "Å"
#define CHECKED_CHECK_BOX   "Ç"

#define NORMAL_RADIO_BOX    ""
#define CHECKED_RADIO_BOX   "Ä"
#define CURSOR_STRING       "_"
#define SLIDER_LEFT         "É"  // 131
#define SLIDER_RIGHT        "Ñ"  // 132
#define SLIDER_MIDDLE       "Ö"  // 133
#define SLIDER_MARKER       "Ü"  // 134
#define UP_ARROW_MARKER     "á"  // 135
#define DOWN_ARROW_MARKER   "à"  // 136

ubyte			menuPalette [256*3];
static		char *pszCurBg = NULL;

grs_bitmap	*pAltBg = NULL;

typedef struct nm_control {
	int	sc_w,
			sc_h,
			x, 
			y, 
			xOffs, 
			yOffs,
			width,
			height,
			w, 
			h, 
			aw, 
			tw, 
			th, 
			ty,
			twidth, 
			right_offset, 
			nStringHeight,
			bTinyMode,
			nMenus, 
			nOthers, 
			nMaxNoScroll, 
			nMaxOnMenu,
			nMaxDisplayable,
			nScrollOffset,
			bIsScrollBox,
			nDisplayMode,
			bValid;
} nm_control;

//------------------------------------------------------------------------------

extern int bSaveScreenShot;

void GameLoop (int, int);
void GameRenderFrame ();
void PrintVersionInfo (void);

int ExecMenu4 (char * title, char * subtitle, int nItems, newmenu_item * item, 
					  void (*subfunction) (int nItems, newmenu_item * items, int * last_key, int cItem), 
					  int *cItemP, char * filename, int width, int height, int bTinyMode);
void ShowExtraNetGameInfo (int choice);


int bNewMenuFirstTime = 1;
//--unused-- int Newmenu_fade_in = 1;

grs_bitmap nm_background, nm_background_save;

#define MESSAGEBOX_TEXT_SIZE 10000		// How many characters in messagebox
#define MAX_TEXT_WIDTH 	200				// How many pixels wide a input box can be

char Pauseable_menu=0;
char bAlreadyShowingInfo=0;

void NMInitBackground (char *filename, bkg *bg, int x, int y, int w, int h, int bRedraw);

//------------------------------------------------------------------------------

inline void NMFreeTextBm (newmenu_item *item)
{
	int	i;

for (i = 0; i < 2; i++) {
	GrFreeBitmap (item->text_bm [i]);
	item->text_bm [i] = NULL;
	}
}

//------------------------------------------------------------------------------

void NMFreeAllTextBms (newmenu_item *items, int nItems)
{
for (; nItems; nItems--, items++)
	NMFreeTextBm (items);
}

//------------------------------------------------------------------------------

void GrRemapMonoFonts ();

void NMRemoveBackground (bkg *bg) 
{
if (gameOpts->menus.nStyle) {
	if (bg) {
		if (bg->background) {
			if (bg->background == pAltBg)
				NMFreeAltBg (0);
			else
				GrFreeBitmap (bg->background);
			bg->background = NULL;
			}
		if (bg->pszPrevBg)
			pszCurBg = bg->pszPrevBg;
#if 1
		if (bg->menu_canvas) {
			WINDOS (DDGrFreeSubCanvas (bg->menu_canvas), 
					  GrFreeSubCanvas (bg->menu_canvas));
			WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (NULL));			
			bg->menu_canvas = NULL;
			}
#endif
		}
	}
if (gameStates.app.bGameRunning) {
	GrPaletteStepLoad (NULL);
#if 1
	RemapFontsAndMenus (1);
	GrRemapMonoFonts ();
#endif
	}
}

//------------------------------------------------------------------------------

void _CDECL_ NewMenuClose (void)	
{
LogErr ("unloading menu data\n");
if (nm_background.bm_texBuf)
	d_free (nm_background.bm_texBuf);
if (nm_background_save.bm_texBuf)
	d_free (nm_background_save.bm_texBuf);
bNewMenuFirstTime = 1;
}

//------------------------------------------------------------------------------

ubyte bgPalette [256*3];
//should be called whenever the palette changes
void NMRemapBackground ()
{
if (!bNewMenuFirstTime) {
	if (!nm_background.bm_texBuf)
		nm_background.bm_texBuf = d_malloc (nm_background.bm_props.w * nm_background.bm_props.h);
	memcpy (nm_background.bm_texBuf, nm_background_save.bm_texBuf, nm_background.bm_props.w * nm_background.bm_props.h);
	//GrRemapBitmapGood (&nm_background, bgPalette, -1, -1);
	}
}

//------------------------------------------------------------------------------

void NMBlueBox (int x1, int y1, int x2, int y2)
{
if ((gameOpts->menus.nStyle == 1)) {// && ((gameOpts->menus.altBg.bHave < 1) || gameStates.app.bGameRunning)) {
	if (x1 <= 0)
		x1 = 1;
	if (y1 <= 0)
		y1 = 1;
	if (x2 >= grdCurScreen->sc_w)
		x2 = grdCurScreen->sc_w - 1;
	if (y2 >= grdCurScreen->sc_h)
		y2 = grdCurScreen->sc_h - 1;
#if 0
	if ((gameOpts->menus.altBg.bHave > 0) && !gameStates.app.bGameRunning)
		GrSetColorRGB (PAL2RGBA (15), PAL2RGBA (15), PAL2RGBA (27), 80);
	else
#endif
		GrSetColorRGB (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), 80);
	gameStates.render.grAlpha = (float) GR_ACTUAL_FADE_LEVELS / 3.0f;
	GrURect (x1, y1, x2, y2);
	gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
	GrSetColorRGB (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), 255);
	//GrSetColorRGBi (D2BLUE_RGBA);
	GrUBox (x1, y1, x2, y2);
	}
}

//------------------------------------------------------------------------------

void NMLoadAltBg (void)
{
if (gameStates.app.bNostalgia || !gameOpts->menus.nStyle)
	gameOpts->menus.altBg.bHave = -1;
else if (gameOpts->menus.altBg.bHave > 0)
	gameOpts->menus.altBg.bHave++;
else if (!gameOpts->menus.altBg.bHave) {
	if (pAltBg = GrCreateBitmap (0, 0, 0)) {
		memset (pAltBg, 0, sizeof (*pAltBg));
		gameOpts->menus.altBg.bHave = 
			ReadTGA (gameOpts->menus.altBg.szName, NULL, pAltBg, 
						(gameOpts->menus.altBg.alpha < 0) ? -1 : (int) (gameOpts->menus.altBg.alpha * 255), 
						gameOpts->menus.altBg.brightness, gameOpts->menus.altBg.grayscale, 0) ? 1 : -1;
		}
	else
		gameOpts->menus.altBg.bHave = -1;
	}
}

//------------------------------------------------------------------------------

int NMFreeAltBg (int bForce)
{
if (!pAltBg)
	return 0;
if (bForce || !--gameOpts->menus.altBg.bHave) {
	gameOpts->menus.altBg.bHave = 0;
	GrFreeBitmap (pAltBg);
	pAltBg = NULL;
	}	
return 1;
}

//------------------------------------------------------------------------------

#include <math.h>

extern char szLastPaletteLoaded [];

void NMLoadBackground (char * filename, bkg *bg, int bRedraw)
{
	int			pcx_error;
	int			width, height;
	grs_bitmap	*bmp = bg ? bg->background : NULL;

	//@@//I think this only gets called to fill the whole screen
	//@@Assert (grdCurCanv->cv_bitmap.bm_props.w == 320);
	//@@Assert (grdCurCanv->cv_bitmap.bm_props.h == 200);
#if 0
	if (bRedraw)
		bRedraw = gameOpts->menus.nStyle && bmp && pszCurBg && filename && !strcmp (pszCurBg, filename);
#endif
if (!(bRedraw && gameOpts->menus.nStyle)) {
	if (bmp && (bmp != pAltBg)) {
		GrFreeBitmap (bmp);
		bmp = NULL;
		}
	if (!filename)
		filename = pszCurBg;
	else if (!pszCurBg)
		pszCurBg = filename;
	if (!filename)
		filename = gameOpts->app.bDemoData ? MENU_PCX_SHAREWARE : MENU_PCX_FULL;
	if (!(filename && strcmp (filename, MENU_PCX_FULL) && strcmp (filename, MENU_PCX_SHAREWARE))) {
		NMLoadAltBg ();
		if (gameOpts->menus.altBg.bHave > 0)
			bmp = pAltBg;
		}
	if (gameOpts->menus.altBg.bHave < 1) {
		if (!filename)
			return;
		pcx_error = pcx_get_dimensions (filename, &width, &height);
		if (pcx_error != PCX_ERROR_NONE)
			Error ("Could not open pcx file <%s>\n", filename);
		bmp = GrCreateBitmap (width, height, 0);
		Assert (bmp != NULL);
		}
	pcx_error = pcx_read_bitmap (filename, (gameOpts->menus.altBg.bHave > 0) ? NULL : bmp, bmp->bm_props.nType, 0);
	Assert (pcx_error == PCX_ERROR_NONE);
// Remap stuff. this code is kind of a hack. Before we bring up the menu, we need to
// do some stuff to make sure the palette is ok. First,we need to get our current palette 
// into the 2d's array, so the remapping will work.  Second, we need to remap the fonts.  
// Third, we need to fill in part of the fade tables so the darkening of the menu edges works.
#ifdef OGL
	GrPaletteStepLoad (gameData.render.pal.pCurPal);
#endif
	//GrCopyPalette (grPalette, menuPalette, sizeof (grPalette));
	//RemapFontsAndMenus (1);
	//GrRemapMonoFonts ();
	strcpy (szLastPaletteLoaded, "");		//force palette load next time
	if (bg) {
		if (gameOpts->menus.nStyle && gameOpts->menus.bFastMenus)
			OglLoadBmTexture (bmp, 0, 0);
		bg->pszPrevBg = pszCurBg;
		pszCurBg = filename;
		}
	}
if (!bmp)
	return;
WIN (DDGRLOCK (dd_grd_curcanv));
#if defined (POLY_ACC)
pa_save_clut ();
pa_update_clut (grPalette, 0, 256, 0);
#endif
if (!(gameStates.app.bGameRunning && gameOpts->menus.nStyle))
	show_fullscr (bmp);
#if defined (POLY_ACC)
    pa_restore_clut ();
#endif
WIN (DDGRUNLOCK (dd_grd_curcanv));
if (bg)
	bg->background = bmp;
else
	GrFreeBitmap (bmp);
}

//------------------------------------------------------------------------------

#define MENU_BACKGROUND_BITMAP_HIRES (CFExist ("scoresb.pcx", gameFolders.szDataDir, 0)?"scoresb.pcx":"scores.pcx")
#define MENU_BACKGROUND_BITMAP_LORES (CFExist ("scores.pcx", gameFolders.szDataDir, 0)?"scores.pcx":"scoresb.pcx") // Mac datafiles only have scoresb.pcx

#define MENU_BACKGROUND_BITMAP (gameStates.menus.bHires?MENU_BACKGROUND_BITMAP_HIRES:MENU_BACKGROUND_BITMAP_LORES)

int bHiresBackground;
int bNoDarkening = 0;

void NMDrawBackground (bkg *bg, int x1, int y1, int x2, int y2, int bRedraw)
{
	int w, h;

if (gameOpts->menus.nStyle) {
	if (bg)
		bg->bIgnoreBg = 1;
	NMInitBackground (NULL, bg, x1, y1, x2 - x1 + 1, y2 - y1 + 1, bRedraw);
//	if (!(bg && bg->menu_canvas))
//		NMBlueBox (x1, y1, x2, y2);
	}
else {
	if (bNewMenuFirstTime || gameStates.menus.bHires!=bHiresBackground)	{
		int pcx_error;

		if (bNewMenuFirstTime) {
			atexit (NewMenuClose);
			bNewMenuFirstTime = 0;
			nm_background_save.bm_texBuf = NULL;		
			}
		else {
			if (nm_background_save.bm_texBuf)
				d_free (nm_background_save.bm_texBuf);
			if (nm_background.bm_texBuf)
				d_free (nm_background.bm_texBuf);
			}
		pcx_error = pcx_read_bitmap (MENU_BACKGROUND_BITMAP, &nm_background_save, BM_LINEAR, 0);
		Assert (pcx_error == PCX_ERROR_NONE);
		nm_background = nm_background_save;
		nm_background.bm_texBuf = NULL;		
		NMRemapBackground ();
		bHiresBackground = gameStates.menus.bHires;
		}
	if (x1 < 0) 
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	w = x2-x1+1;
	h = y2-y1+1;
	//if (w > nm_background.bm_props.w) w = nm_background.bm_props.w;
	//if (h > nm_background.bm_props.h) h = nm_background.bm_props.h;
	x2 = x1 + w - 1;
	y2 = y1 + h - 1;
#if 0
	{
		grs_bitmap *tmp = GrCreateBitmap (w, h);
		GrBitmapScaleTo (&nm_background, tmp);
		WIN (DDGRLOCK (dd_grd_curcanv);
		glDisable (GL_BLEND);
		if (bNoDarkening)
			GrBmBitBlt (w, h, x1, y1, LHX (10), LHY (10), tmp, &(grdCurCanv->cv_bitmap));
		else
			GrBmBitBlt (w, h, x1, y1, 0, 0, tmp, &(grdCurCanv->cv_bitmap));
		glEnable (GL_BLEND);
		GrFreeBitmap (tmp);
	}
#else
	WIN (DDGRLOCK (dd_grd_curcanv));
	glDisable (GL_BLEND);
	if (bNoDarkening)
		GrBmBitBlt (w, h, x1, y1, LHX (10), LHY (10), &nm_background, &(grdCurCanv->cv_bitmap));
	else
		GrBmBitBlt (w, h, x1, y1, 0, 0, &nm_background, &(grdCurCanv->cv_bitmap));
	glEnable (GL_BLEND);
#endif
	if (!bNoDarkening) {
		// give the menu background box a 3D look by changing its edges' brightness
		gameStates.render.grAlpha = 2*7;
		GrSetColorRGB (0, 0, 0, 255);
		GrURect (x2-5, y1+5, x2-6, y2-5);
		GrURect (x2-4, y1+4, x2-5, y2-5);
		GrURect (x2-3, y1+3, x2-4, y2-5);
		GrURect (x2-2, y1+2, x2-3, y2-5);
		GrURect (x2-1, y1+1, x2-2, y2-5);
		GrURect (x2+0, y1+0, x2-1, y2-5);
		GrURect (x1+5, y2-5, x2, y2-6);
		GrURect (x1+4, y2-4, x2, y2-5);
		GrURect (x1+3, y2-3, x2, y2-4);
		GrURect (x1+2, y2-2, x2, y2-3);
		GrURect (x1+1, y2-1, x2, y2-2);
		GrURect (x1+0, y2, x2, y2-1);
		}
	GrUpdate (0);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
	}
gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
}

//------------------------------------------------------------------------------

void NMInitBackground (char *filename, bkg *bg, int x, int y, int w, int h, int bRedraw)
{
	static char *pszMenuPcx = NULL;
	int bVerInfo, bBlueBox;

if (!pszMenuPcx)
	pszMenuPcx = Menu_pcx_name;
bVerInfo = filename && !strcmp (filename, pszMenuPcx);
bBlueBox = gameOpts->menus.nStyle && (!bVerInfo || (gameOpts->menus.altBg.bHave > 0));
if (filename || gameOpts->menus.nStyle) {	// background image file present
	NMLoadBackground (filename, bg, bRedraw);
	if (bVerInfo)
		PrintVersionInfo ();
	else if (bBlueBox) {
		if (bg && (bg->menu_canvas || bg->bIgnoreCanv)) {
			if (bg->menu_canvas)
				WINDOS (DDGrSetCurrentCanvas (bg->menu_canvas), 
						  GrSetCurrentCanvas (bg->menu_canvas));
//			NMLoadBackground (filename, bg, bRedraw);
			if (bVerInfo)
				PrintVersionInfo ();
			}
		NMBlueBox (x, y, x + w, y + h);
		}
	if (bg && !bg->bIgnoreBg) {
		GrPaletteStepLoad (NULL);
		bg->saved = NULL;
		if (!gameOpts->menus.nStyle) {
			if (!bg->background) {
#if defined (POLY_ACC)
				bg->background = GrCreateBitmap2 (w, h, grdCurCanv->cv_bitmap.bm_props.nType, NULL);
#else
				bg->background = GrCreateBitmap (w, h, 1);
#endif
				Assert (bg->background != NULL);
				}
			WIN (DDGRLOCK (dd_grd_curcanv));
			GrBmBitBlt (w, h, 0, 0, x, y, &grdCurCanv->cv_bitmap, bg->background);
			WIN (DDGRUNLOCK (dd_grd_curcanv));
			}
		}
	} 
// Save the background of the display
//	Win95 must refer to the screen as a dd_grs_canvas, so...
if (!(gameOpts->menus.nStyle && bRedraw)) {
	if (bg && !(bg->menu_canvas || bg->bIgnoreCanv)) {
		if (gameOpts->menus.nStyle) {
			x = y = 0;
			w = grdCurScreen->sc_w;
			h = grdCurScreen->sc_h;
			}
		WINDOS (bg->menu_canvas = DDGrCreateSubCanvas (dd_grd_screencanv, x, y, w, h), 
				  bg->menu_canvas = GrCreateSubCanvas (&grdCurScreen->sc_canvas, x, y, w, h)
			);
		WINDOS (DDGrSetCurrentCanvas (bg->menu_canvas), 
				  GrSetCurrentCanvas (bg->menu_canvas));
			}
	}
if (bg && !(gameOpts->menus.nStyle || filename)) {
	// Save the background under the menu...
#ifdef TACTILE
	if (TactileStick)
		DisableForces ();
#endif
	if (!(gameOpts->menus.nStyle || bg->saved)) {
#if defined (POLY_ACC)
		bg->saved = GrCreateBitmap2 (w, h, grdCurCanv->cv_bitmap.bm_props.nType, NULL);
#else
		bg->saved = GrCreateBitmap (w, h, 0);
#endif
		Assert (bg->saved != NULL);
		}
	bg->saved->bm_palette = defaultPalette;
	if (!gameOpts->menus.nStyle) {
		WIN (DDGRLOCK (dd_grd_curcanv));
		GrBmBitBlt (w, h, 0, 0, 0, 0, &grdCurCanv->cv_bitmap, bg->saved);
		WIN (DDGRUNLOCK (dd_grd_curcanv));
		}
	WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (NULL));
	NMDrawBackground (bg, x, y, x + w - 1, y + h - 1, bRedraw);
	GrUpdate (0);
	WINDOS (DDGrSetCurrentCanvas (bg->menu_canvas), 
			  GrSetCurrentCanvas (bg->menu_canvas));
	bg->background = GrCreateSubBitmap (&nm_background, 0, 0, w, h);
	GrUpdate (0);
	}
}

//------------------------------------------------------------------------------

void NMRestoreBackground (int sx, int sy, int dx, int dy, int w, int h)
{
	int x1, x2, y1, y2;

	x1 = sx; 
	x2 = sx+w-1;
	y1 = sy; 
	y2 = sy+h-1;

	if (x1 < 0) 
		x1 = 0;
	if (y1 < 0) 
		y1 = 0;

	if (x2 >= nm_background.bm_props.w) 
		x2=nm_background.bm_props.w-1;
	if (y2 >= nm_background.bm_props.h) 
		y2=nm_background.bm_props.h-1;

	w = x2 - x1 + 1;
	h = y2 - y1 + 1;

	WIN (DDGRLOCK (dd_grd_curcanv));
	GrBmBitBlt (w, h, dx, dy, x1, y1, &nm_background, &(grdCurCanv->cv_bitmap));
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

short NMSetItemColor (newmenu_item *item, int bIsCurrent, int bTiny) 
{
if (bTiny) {
	if (!gameData.menu.bValid) {
		gameData.menu.warnColor = RED_RGBA;
		gameData.menu.keyColor = RGBA_PAL (57, 49, 20);
		gameData.menu.tabbedColor = WHITE_RGBA;
		gameData.menu.tinyColors [0][0] = RGBA_PAL (29, 29, 47);	
		gameData.menu.tinyColors [0][1] = RED_RGBA;
		gameData.menu.tinyColors [1][0] = RGBA_PAL (57, 49, 20);
		gameData.menu.tinyColors [1][1] = ORANGE_RGBA;
		gameData.menu.bValid = 1;
		}
	if (gameData.menu.colorOverride)
		GrSetFontColorRGBi (gameData.menu.colorOverride, 1, 0, 0);
	else if (item->text [0] == '\t')
		GrSetFontColorRGBi (gameData.menu.tabbedColor, 1, 0, 0);
	else 
		GrSetFontColorRGBi (gameData.menu.tinyColors [bIsCurrent][item->unavailable], 1, 0, 0);
	}
else {
	if (bIsCurrent)
		grdCurCanv->cv_font = SELECTED_FONT;
	else
		grdCurCanv->cv_font = NORMAL_FONT;
#ifdef WINDOWS
	if (bIsCurrent && item->nType == NM_TYPE_TEXT) 
		grdCurCanv->cv_font = NORMAL_FONT;
#endif
//	if (!bIsCurrent && (gameOpts->menus.altBg.bHave > 0))
//		GrSetFontColor (GrFindClosestColorCurrent (29, 29, 47), -1);
	}
return grdCurCanv->cv_font_fg_color.index;
}

//------------------------------------------------------------------------------

int nTabIndex = -1;
int nTabs [] = {15, 87, 124, 162, 228, 253};

void NMHotKeyString (newmenu_item *item, int bIsCurrent, int bTiny, int bCreateTextBms, int nDepth)
{
#if 1
	grs_bitmap	*bm = item->text_bm [bIsCurrent];

if (!*item->text)
	return;
if (item->color)
	GrSetFontColorRGBi (item->color, 1, 0, 0);
else
	NMSetItemColor (item, bIsCurrent, bTiny);
if (/*!strchr (item->text, '\t') &&*/ bCreateTextBms && gameOpts->menus.bFastMenus &&
	 (bm || (bm = CreateStringBitmap (item->text, MENU_KEY (item->key, -1), 
												 gameData.menu.keyColor,
												 nTabs, item->centered, item->w)))) {
	OglUBitBltI (bm->bm_props.w, bm->bm_props.h, item->x, item->y, bm->bm_props.w, bm->bm_props.h, 0, 0, 
						bm, &grdCurCanv->cv_bitmap, 0);
	item->text_bm [bIsCurrent] = bm;
	}
else {
#endif
	int	w, h, aw, l, i, 
			x = item->x, 
			y = item->y;
	char	*t, *ps = item->text, s [256], ch = 0, ch2;

if (!nDepth)
	item->textSave = item->text;
if (t = strchr (ps, '\n')) {
	strncpy (s, ps, sizeof (s));
	item->text = s;
	GrGetStringSize (s, &w, &h, &aw);
	do {
		if (t = strchr (item->text, '\n'))
			*t = '\0';
		NMHotKeyString (item, 0, bTiny, 0, nDepth + 1);
		if (!t)
			break;
		item->text = t + 1;
		item->y += h / 2;
		item->x = item->xSave;
		nTabIndex = -1;
		} while (*(item->text));
	}
else if (t = strchr (ps, '\t')) {
	strncpy (s, ps, sizeof (s));
	item->text = s;
	GrGetStringSize (s, &w, &h, &aw);
	do {
		if (t = strchr (item->text, '\t'))
			*t = '\0';
		NMHotKeyString (item, 0, bTiny, 0, nDepth + 1);
		if (!t)
			break;
		item->text = t + 1;
		nTabIndex++;
	} while (*(item->text));
	nTabIndex = -1;
	item->text = ps;
	item->y = y;
	}
else {
	l = (int) strlen (item->text);
	if (bIsCurrent || !item->key)
		i = l;
	else {
		ch = MENU_KEY (item->key, *ps);
		for (i = 0; ps [i]; i++)
			if (ps [i] == ch)
				break;
		}
	strncpy (s, ps, sizeof (s));
	s [i] = '\0';
	GrGetStringSize (s, &w, &h, &aw);
	if (nTabIndex >= 0) {
		x += LHX (nTabs [nTabIndex]);
		if (!gameStates.multi.bSurfingNet)
			x += item->w - w;
		}
	//item->x = x + w;
	if (i) {
		GrString (x, y, s);
#ifdef _DEBUG
		//GrUpdate (0);
#endif
		}
	if (i < l) {	// print the hotkey
		x += w;
		s [i] = ch;
		ch2 = s [++i];
		s [i] = '\0';
		NMSetItemColor (item, 1, bTiny);
		GrString (x, y, s + i - 1);
#ifdef _DEBUG
		//GrUpdate (0);
#endif
		NMSetItemColor (item, 0, bTiny);
		if (i < l) { // print text following the hotkey
			GrGetStringSize (s + i - 1, &w, &h, &aw);
			x += w;
			s [i] = ch2;
			GrString (x, y, s + i);
			}
		}
	}
#if 0
if (!nDepth) {
	item->text = item->textSave;
	item->x = item->xSave;
	item->y = item->ySave;
	}
#endif
}
}

//------------------------------------------------------------------------------
// Draw a left justfied string
void NMString (newmenu_item *item, bkg * b, int bIsCurrent, int bTiny)
{
	int w1 = item->w, x = item->x, y = item->y;
	int l, w, h, aw, tx=0, t=0, i;
	char *p, *s1, measure [2] , *s=item->text;
	int XTabs [6];
	static char s2 [1024];

	p=s1=NULL;
	l = (int) strlen (s);
	memcpy (s2, s, l + 1);

	for (i=0;i<6;i++)
		XTabs [i]= (LHX (nTabs [i])) + x;
 
	measure [1]=0;

	if (!gameStates.multi.bSurfingNet) {
		p = strchr (s2, '\t');
		if (p &&(w1>0)) {
			*p = '\0';
			s1 = p+1;
		}
	}
	if (w1 > 0)
		w = w1;
	GrGetStringSize (s2, &w, &h, &aw);
	// CHANGED
	if (curDrawBuffer != GL_BACK)
		GrBmBitBlt (b->background->bm_props.w-15, h+2, 5, y-1, 5, y-1, b->background, &(grdCurCanv->cv_bitmap));
	//GrBmBitBlt (w, h, x, y, x, y, b->background, &(grdCurCanv->cv_bitmap));

	if (0 && gameStates.multi.bSurfingNet) {
		for (i=0;i<l;i++) {
			if (s2 [i]=='\t' && gameStates.multi.bSurfingNet) {
				x=XTabs [t];
				t++;
				continue;
			}
			measure [0]=s2 [i];
			GrGetStringSize (measure, &tx, &h, &aw);
			GrString (x, y, measure);
			x+=tx;
		}
	}
	else {
		NMHotKeyString (item, bIsCurrent, bTiny, 1, 0);
		return;
		}         

	if (!gameStates.multi.bSurfingNet && p &&(w1>0)) {
		GrGetStringSize (s1, &w, &h, &aw);
		GrString (x+w1-w, y, s1);
		*p = '\t';
	}
}

//------------------------------------------------------------------------------
// Draw a slider and it's string
void NMStringSlider (newmenu_item *item, bkg * b, int bIsCurrent, int bTiny)
{
	int	w, h, aw;
	int	w1 = item->w, 
			x = item->x, 
			y = item->y;
	char	*t = item->text, 
			*s = item->saved_text;
	char	*p, *s1;

	s1=NULL;

	p = strchr (s, '\t');
	if (p)	{
		*p = '\0';
		s1 = p+1;
	}

	GrGetStringSize (s, &w, &h, &aw);
	// CHANGED

		if (curDrawBuffer != GL_BACK)
			GrBmBitBlt (b->background->bm_props.w-15, h, 5, y, 5, y, b->background, &(grdCurCanv->cv_bitmap));
		//GrBmBitBlt (w, h, x, y, x, y, b->background, &(grdCurCanv->cv_bitmap));

		item->text = s;
		NMHotKeyString (item, bIsCurrent, bTiny, 1, 0);
		item->text = t;
		//GrString (x, y, s);

		if (p)	{
			GrGetStringSize (s1, &w, &h, &aw);

			// CHANGED
			if (curDrawBuffer != GL_BACK) {
				GrBmBitBlt (w, 1, x+w1-w, y, x+w1-w, y, b->background, &(grdCurCanv->cv_bitmap));
				GrBmBitBlt (w, 1, x+w1-w, y+h-1, x+w1-w, y, b->background, &(grdCurCanv->cv_bitmap));
				}
			GrString (x+w1-w, y, s1);

			*p = '\t';
		}
}


//------------------------------------------------------------------------------
// Draw a left justfied string with black background.
void NMStringBlack (bkg * b, int w1, int x, int y, char * s)
{
	int w, h, aw;
	GrGetStringSize (s, &w, &h, &aw);

	if (w1 == 0) 
		w1 = w;

	WIN (DDGRLOCK (dd_grd_curcanv));
		GrSetColorRGBi (RGBA_PAL2 (2, 2, 2));
  		GrRect (x-1, y-1, x-1, y+h-1);
		GrRect (x-1, y-1, x+w1-1, y-1);

	 
		GrSetColorRGBi (RGBA_PAL2 (5, 5, 5));
		GrRect (x, y+h, x+w1, y+h);
		GrRect (x+w1, y-1, x+w1, y+h);
     
		GrSetColorRGB (0, 0, 0, 255);
		GrRect (x, y, x+w1-1, y+h-1);
	
		GrString (x+1, y+1, s);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------
// Draw a right justfied string
void NMRString (newmenu_item *item, bkg * b, int bIsCurrent, int bTiny, char *s)
{
	int	w, h, aw;
	int	w1 = item->right_offset, 
			x = item->x, 
			y = item->y;
	char	*hs;

	GrGetStringSize (s, &w, &h, &aw);
	x -= 3;

	if (w1 == 0) 
		w1 = w;

	// CHANGED
	WIN (DDGRLOCK (dd_grd_curcanv));
		if (curDrawBuffer != GL_BACK)
			GrBmBitBlt (w1, h, x-w1, y, x-w1, y, b->background, &(grdCurCanv->cv_bitmap));
		hs = item->text;
		item->text = s;
		h = item->x;
		item->x = x - w;
		NMHotKeyString (item, bIsCurrent, bTiny, 0, 0);
		item->text = hs;
		item->x = h;
//		GrString (x-w, y, s);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

void NMRStringWXY (bkg * b, int w1, int x, int y, char *s)
{
	int	w, h, aw;

	GrGetStringSize (s, &w, &h, &aw);
	x -= 3;

	if (w1 == 0) 
		w1 = w;

	// CHANGED
	WIN (DDGRLOCK (dd_grd_curcanv));
		if (curDrawBuffer != GL_BACK)
			GrBmBitBlt (w1, h, x-w1, y, x-w1, y, b->background, &(grdCurCanv->cv_bitmap));
		GrString (x-w, y, s);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

#include "timer.h"

//for text items, constantly redraw cursor (to achieve flash)
void NMUpdateCursor (newmenu_item *item)
{
	int w, h, aw;
	fix time = TimerGetApproxSeconds ();
	int x, y;
	char * text = item->text;

	Assert (item->nType==NM_TYPE_INPUT_MENU || item->nType==NM_TYPE_INPUT);

	while (*text)	{
		GrGetStringSize (text, &w, &h, &aw);
		if (w > item->w-10)
			text++;
		else
			break;
	}
	if (*text==0) 
		w = 0;
	x = item->x+w; y = item->y;

WIN (DDGRLOCK (dd_grd_curcanv));
	if (time & 0x8000)
		GrString (x, y, CURSOR_STRING);
	else {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect (x, y, x+grdCurCanv->cv_font->ft_w-1, y+grdCurCanv->cv_font->ft_h-1);
	}
WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

void NMStringInputBox (bkg *b, int w, int x, int y, char * text, int current)
{
	int w1, h1, aw;

	while (*text)	{
		GrGetStringSize (text, &w1, &h1, &aw);
		if (w1 > w-10)
			text++;
		else
			break;
	}
	if (*text == 0)
		w1 = 0;

   NMStringBlack (b, w, x, y, text);
		
	if (current)	{
		GrString (x+w1+1, y, CURSOR_STRING);
	}
}

//------------------------------------------------------------------------------

void NMGauge (bkg *b, int w, int x, int y, int val, int maxVal, int current)
{
	int w1, h, aw;

GrGetStringSize (" ", &w1, &h, &aw);
if (!w) {
	w = w1 * 30;
	}
w1 = w * val / maxVal;
if (w1 < w) {
	GrSetColorRGB (0, 0, 0, 255);
	GrURect (x + w1 + 1, y, x + w, y + h - 2);
	}
GrSetColorRGB (200, 0, 0, 255);
if (w1) {
	GrURect (x + 1, y, x + w1, y + h - 2);
	}
GrUBox (x, y, x + w - 1, y + h - 1);
}

//------------------------------------------------------------------------------

void NMDrawItem (bkg * b, newmenu_item *item, int bIsCurrent, int bTiny)
{
NMSetItemColor (item, bIsCurrent, bTiny);
if (item->rebuild) {
	NMFreeTextBm (item);
	item->rebuild = 0;
	}
WIN (DDGRLOCK (dd_grd_curcanv));	
	switch (item->nType)	{
	case NM_TYPE_TEXT:
      // grdCurCanv->cv_font=TEXT_FONT;
		// fall through on purpose

	case NM_TYPE_MENU:
		NMString (item, b, bIsCurrent, bTiny);
		break;

	case NM_TYPE_SLIDER:	{
		int h, l;
		char *psz = item->saved_text;

		if (item->value < item->minValue) 
			item->value = item->minValue;
		else if (item->value > item->maxValue) 
			item->value = item->maxValue;
		sprintf (psz, "%s\t%s", item->text, SLIDER_LEFT);
#if 1
		l = (int) strlen (psz);
		h = item->maxValue - item->minValue + 1;
		memset (psz + l, SLIDER_MIDDLE [0], h);
		psz [l + h] = SLIDER_RIGHT [0];
		psz [l + h + 1] = '\0';
#else
		for (j = 0; j < (item->maxValue - item->minValue + 1); j++) {
			sprintf (psz, "%s%s", psz, SLIDER_MIDDLE);
			}
		sprintf (item->saved_text, "%s%s", item->saved_text, SLIDER_RIGHT);
#endif
		psz [item->value + 1 + strlen (item->text) + 1] = SLIDER_MARKER [0];
		NMStringSlider (item, b, bIsCurrent, bTiny);
		}
		break;

	case NM_TYPE_INPUT_MENU:
		if (item->group)	
			NMStringInputBox (b, item->w, item->x, item->y, item->text, bIsCurrent);
		else
			NMString (item, b, bIsCurrent, bTiny);
		break;

	case NM_TYPE_INPUT:
		NMStringInputBox (b, item->w, item->x, item->y, item->text, bIsCurrent);
		break;

	case NM_TYPE_GAUGE:
		NMGauge (b, item->w, item->x, item->y, item->value, item->maxValue, bIsCurrent);
		break;

	case NM_TYPE_CHECK:
		NMString (item, b, bIsCurrent, bTiny);
		if (item->value)
			NMRString (item, b, bIsCurrent, bTiny, CHECKED_CHECK_BOX);
		else														  
			NMRString (item, b, bIsCurrent, bTiny, NORMAL_CHECK_BOX);
		break;

	case NM_TYPE_RADIO:
		NMString (item, b, bIsCurrent, bTiny);
		if (item->value)
			NMRString (item, b, bIsCurrent, bTiny, CHECKED_RADIO_BOX);
		else
			NMRString (item, b, bIsCurrent, bTiny, NORMAL_RADIO_BOX);
		break;

	case NM_TYPE_NUMBER:	
		{
		char text [10];
		if (item->value < item->minValue) 
			item->value=item->minValue;
		if (item->value > item->maxValue) 
			item->value=item->maxValue;
		NMString (item, b, bIsCurrent, bTiny);
		sprintf (text, "%d", item->value);
		NMRString (item, b, bIsCurrent, bTiny, text);
		}
		break;
	}
WIN (DDGRUNLOCK (dd_grd_curcanv));

}

//------------------------------------------------------------------------------

char *nmAllowedChars=NULL;

//returns true if char is bAllowed
int NMCharAllowed (char c)
{
	char *p = nmAllowedChars;

	if (!p)
		return 1;

	while (*p) {
		Assert (p [1]);

		if (c>=p [0] && c<=p [1])
			return 1;

		p += 2;
	}

	return 0;
}

//------------------------------------------------------------------------------

void NMTrimWhitespace (char * text)
{
	int i, l = (int) strlen (text);
	for (i=l-1; i>=0; i--)	{
		if (isspace (text [i]))
			text [i] = 0;
		else
			return;
	}
}

//------------------------------------------------------------------------------

int ExecMenu (char * title, char * subtitle, int nItems, newmenu_item * item, 
					void (*subfunction) (int nItems, newmenu_item * items, int * last_key, int cItem),
					char *filename)
{
	return ExecMenu3 (title, subtitle, nItems, item, subfunction, NULL, filename, -1, -1);
}
int ExecMenutiny (char * title, char * subtitle, int nItems, newmenu_item * item, 
						 void (*subfunction) (int nItems, newmenu_item * items, int * last_key, int cItem))
{
        return ExecMenu4 (title, subtitle, nItems, item, subfunction, NULL, NULL, LHX (310), -1, 1);
}

//------------------------------------------------------------------------------

int ExecMenutiny2 (char * title, char * subtitle, int nItems, newmenu_item * item, 
						  void (*subfunction) (int nItems, newmenu_item * items, int * last_key, int cItem))
{
        return ExecMenu4 (title, subtitle, nItems, item, subfunction, 0, NULL, -1, -1, 1);
}

//------------------------------------------------------------------------------

int ExecMenu1 (char * title, char * subtitle, int nItems, newmenu_item * item, 
					 void (*subfunction) (int nItems, newmenu_item * items, int * last_key, int cItem), 
					 int *cItemP)
{
	return ExecMenu3 (title, subtitle, nItems, item, subfunction, cItemP, NULL, -1, -1);
}

//------------------------------------------------------------------------------

int ExecMenu2 (char * title, char * subtitle, int nItems, newmenu_item * item, 
					 void (*subfunction) (int nItems, newmenu_item * items, int * last_key, int cItem), 
					 int *cItemP, char * filename)
{
	return ExecMenu3 (title, subtitle, nItems, item, subfunction, cItemP, filename, -1, -1);
}

//------------------------------------------------------------------------------

int ExecMenu3 (char * title, char * subtitle, int nItems, newmenu_item * item, 
					 void (*subfunction) (int nItems, newmenu_item * items, int * last_key, int cItem), 
					 int *cItemP, char * filename, int width, int height)
 {
  return ExecMenu4 (title, subtitle, nItems, item, subfunction, cItemP, filename, width, height, 0);
 }

//------------------------------------------------------------------------------

int ExecMenuFixedFont (char * title, char * subtitle, int nItems, newmenu_item * item, 
							  void (*subfunction) (int nItems, newmenu_item * items, int * last_key, int cItem), 
							  int *cItemP, char * filename, int width, int height){
	SetScreenMode (SCREEN_MENU);//hafta set the screen mode before calling or fonts might get changed/freed up if screen res changes
//	return ExecMenu3_real (title, subtitle, nItems, item, subfunction, cItem, filename, width, height, GAME_FONT, GAME_FONT, GAME_FONT, GAME_FONT);
	return ExecMenu4 (title, subtitle, nItems, item, subfunction, cItemP, filename, width, height, 0);
}

//------------------------------------------------------------------------------

//returns 1 if a control device button has been pressed
int NMCheckButtonPress ()
{
	int i;

	switch (gameConfig.nControlType) {
	case	CONTROL_JOYSTICK:
	case	CONTROL_FLIGHTSTICK_PRO:
	case	CONTROL_THRUSTMASTER_FCS:
	case	CONTROL_GRAVIS_GAMEPAD:
		for (i=0; i<4; i++)	
	 		if (joy_get_button_down_cnt (i)>0) return 1;
		break;
	case	CONTROL_MOUSE:
	case	CONTROL_CYBERMAN:
#ifndef NEWMENU_MOUSE   // don't allow mouse to continue from menu
		for (i=0; i<3; i++)	
			if (MouseButtonDownCount (i)>0) return 1;
		break;
#endif
	case	CONTROL_WINJOYSTICK:
	#ifdef WINDOWS	
		for (i=0; i<4; i++)	
	 		if (joy_get_button_down_cnt (i)>0) return 1;
	#endif	
		break;
	case	CONTROL_NONE:		//keyboard only
		#ifdef APPLE_DEMO
			if (key_checkch ())	return 1;			
		#endif

		break;
	default:
		Error ("Bad control nType (gameConfig.nControlType):%i", gameConfig.nControlType);
	}

	return 0;
}

//------------------------------------------------------------------------------

extern int NetworkRequestPlayerNames (int);
extern int RestoringMenu;

#ifdef NEWMENU_MOUSE
ubyte Hack_DblClick_MenuMode=0;
#endif

# define JOYDEFS_CALIBRATING 0

#define CLOSE_X     (gameStates.menus.bHires?15:7)
#define CLOSE_Y     (gameStates.menus.bHires?15:7)
#define CLOSE_SIZE  (gameStates.menus.bHires?10:5)

void NMDrawCloseBox (int x, int y)
{
	WIN (DDGRLOCK (dd_grd_curcanv));
	GrSetColorRGB (0, 0, 0, 255);
	GrRect (x + CLOSE_X, y + CLOSE_Y, x + CLOSE_X + CLOSE_SIZE, y + CLOSE_Y + CLOSE_SIZE);
	GrSetColorRGBi (RGBA_PAL2 (21, 21, 21));
	GrRect (x + CLOSE_X + LHX (1), y + CLOSE_Y + LHX (1), x + CLOSE_X + CLOSE_SIZE - LHX (1), y + CLOSE_Y + CLOSE_SIZE - LHX (1));
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

int NMDrawTitle (char *title, grs_font *font, unsigned int color, int ty)
{
if (title && *title)	{
		int string_width, nStringHeight, average_width, tw, th;

	grdCurCanv->cv_font = font;
	GrSetFontColorRGBi (color, 1, 0, 0);
	GrGetStringSize (title, &string_width, &nStringHeight, &average_width);
	tw = string_width;
	th = nStringHeight;
	WIN (DDGRLOCK (dd_grd_curcanv));
	GrPrintF (0x8000, ty, title);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
	ty += nStringHeight;
	}
return ty;
}

//------------------------------------------------------------------------------

void NMGetTitleSize (char *title, grs_font *font, int *tw, int *th)
{
if (title && *title)	{
		int string_width, nStringHeight, average_width;

	grdCurCanv->cv_font = font;
	GrGetStringSize (title, &string_width, &nStringHeight, &average_width);
	if (string_width > *tw)
		*tw = string_width;
	*th += nStringHeight;
	}
}

//------------------------------------------------------------------------------

int NMGetMenuSize (newmenu_item *item, int nItems, int *w, int *h, int *aw, int *nMenus, int *nOthers)
{
	int	string_width, nStringHeight, average_width;
	int	i, j;

#if 0
if ((gameOpts->menus.nHotKeys > 0) && !gameStates.app.bEnglish)
	gameOpts->menus.nHotKeys = -1;
#endif
for (i = 0; i < nItems; i++) {
	if (!gameStates.app.bEnglish)
		item [i].key = *(item [i].text - 1);
	if (item [i].key) {
		if (gameOpts->menus.nHotKeys < 0) {
			if ((item [i].key < KEY_1) || (item [i].key > KEY_0))
				item [i].key = -1;
			}
		else if (gameOpts->menus.nHotKeys == 0)
			item [i].key = 0;
		}
	item [i].redraw = 1;
	item [i].y = *h;
	GrGetStringSize (item [i].text, &string_width, &nStringHeight, &average_width);
	item [i].right_offset = 0;
	
	if (gameStates.multi.bSurfingNet)
		nStringHeight+=LHY (3);

	item [i].saved_text [0] = '\0';
	if (item [i].nType == NM_TYPE_SLIDER) {
		int w1, h1, aw1;
		(*nOthers)++;
		sprintf (item [i].saved_text, "%s", SLIDER_LEFT);
		for (j=0; j< (item [i].maxValue-item [i].minValue+1); j++)	{
			sprintf (item [i].saved_text, "%s%s", item [i].saved_text, SLIDER_MIDDLE);
			}
		sprintf (item [i].saved_text, "%s%s", item [i].saved_text, SLIDER_RIGHT);
		GrGetStringSize (item [i].saved_text, &w1, &h1, &aw1);
		string_width += w1 + *aw;
		}
	else if (item [i].nType == NM_TYPE_MENU)	{
		(*nMenus)++;
		}
	else if (item [i].nType == NM_TYPE_CHECK)	{
		int w1, h1, aw1;
		(*nOthers)++;
		GrGetStringSize (NORMAL_CHECK_BOX, &w1, &h1, &aw1);
		item [i].right_offset = w1;
		GrGetStringSize (CHECKED_CHECK_BOX, &w1, &h1, &aw1);
		if (w1 > item [i].right_offset)
			item [i].right_offset = w1;
		}
	else if (item [i].nType == NM_TYPE_RADIO) {
		int w1, h1, aw1;
		(*nOthers)++;
		GrGetStringSize (NORMAL_RADIO_BOX, &w1, &h1, &aw1);
		item [i].right_offset = w1;
		GrGetStringSize (CHECKED_RADIO_BOX, &w1, &h1, &aw1);
		if (w1 > item [i].right_offset)
			item [i].right_offset = w1;
		}
	else if  (item [i].nType==NM_TYPE_NUMBER)	{
		int w1, h1, aw1;
		char test_text [20];
		(*nOthers)++;
		sprintf (test_text, "%d", item [i].maxValue);
		GrGetStringSize (test_text, &w1, &h1, &aw1);
		item [i].right_offset = w1;
		sprintf (test_text, "%d", item [i].minValue);
		GrGetStringSize (test_text, &w1, &h1, &aw1);
		if (w1 > item [i].right_offset)
			item [i].right_offset = w1;
		}
	else if (item [i].nType == NM_TYPE_INPUT)	{
		Assert (strlen (item [i].text) < NM_MAX_TEXT_LEN);
		strncpy (item [i].saved_text, item [i].text, NM_MAX_TEXT_LEN);
		(*nOthers)++;
		string_width = item [i].text_len*grdCurCanv->cv_font->ft_w+ ((gameStates.menus.bHires?3:1)*item [i].text_len);
		if (string_width > MAX_TEXT_WIDTH) 
			string_width = MAX_TEXT_WIDTH;
		item [i].value = -1;
		}
	else if (item [i].nType == NM_TYPE_INPUT_MENU)	{
		Assert (strlen (item [i].text) < NM_MAX_TEXT_LEN);
		strncpy (item [i].saved_text, item [i].text, NM_MAX_TEXT_LEN);
		(*nMenus)++;
		string_width = item [i].text_len*grdCurCanv->cv_font->ft_w+ ((gameStates.menus.bHires?3:1)*item [i].text_len);
		item [i].value = -1;
		item [i].group = 0;
		}
	item [i].w = string_width;
	item [i].h = nStringHeight;

	if (string_width > *w)
		*w = string_width;		// Save maximum width
	if (average_width > *aw)
		*aw = average_width;
	*h += nStringHeight + 1;		// Find the height of all strings
	}
return nStringHeight;
}

//------------------------------------------------------------------------------

void NMSetItemPos (newmenu_item *item, int nItems, int twidth, int xOffs, int yOffs, int right_offset)
{
	int	i, j;

for (i = 0; i < nItems; i++)	{
	if ((item [i].x == (short) 0x8000) || item [i].centered) {
		item [i].centered = 1;
		item [i].x = GetCenteredX (item [i].text);
		}
	else
		item [i].x = xOffs + twidth + right_offset;
	item [i].y += yOffs;
	item [i].xSave = item [i].x;
	item [i].ySave = item [i].y;
	if (item [i].nType == NM_TYPE_RADIO)	{
		int fm = -1;	// ffs first marked one
		for (j = 0; j < nItems; j++)	{
			if ((item [j].nType == NM_TYPE_RADIO) && (item [j].group == item [i].group)) {
				if ((fm == -1) && item [j].value)
					fm = j;
				item [j].value = 0;
				}
			}
		if ( fm >= 0)	
			item [fm].value = 1;
		else
			item [i].value = 1;
		}
	}
}

//------------------------------------------------------------------------------

int NMInitCtrl (nm_control *ctrlP, char *title, char *subtitle, int nItems, newmenu_item *item)
{
if ((ctrlP->sc_w != grdCurScreen->sc_w) || (ctrlP->sc_h != grdCurScreen->sc_h)) {
		nm_control	ctrl = *ctrlP;
		int			i, gap, haveTitle;

	ctrl.sc_w = grdCurScreen->sc_w;
	ctrl.sc_h = grdCurScreen->sc_h;
	ctrl.nDisplayMode = gameStates.video.nDisplayMode;
	NMGetTitleSize (title, TITLE_FONT, &ctrl.tw, &ctrl.th);
	NMGetTitleSize (subtitle, SUBTITLE_FONT, &ctrl.tw, &ctrl.th);

	haveTitle = ((title && *title) || (subtitle && *subtitle));
	gap = haveTitle ? (int) LHY (8) : 0;
	ctrl.th += gap;		//put some space between titles & body
	grdCurCanv->cv_font = ctrl.bTinyMode ? SMALL_FONT : NORMAL_FONT;

	ctrl.h = ctrl.th;
	ctrl.nMenus = ctrl.nOthers = 0;
	ctrl.nStringHeight = NMGetMenuSize (item, nItems, &ctrl.w, &ctrl.h, &ctrl.aw, &ctrl.nMenus, &ctrl.nOthers);
	ctrl.nMaxOnMenu = (((gameOpts->menus.nStyle && (ctrl.sc_h > 480)) ? ctrl.sc_h * 4 / 5 : 480) - ctrl.th - LHY (8)) / ctrl.nStringHeight - 2;
	if (/*!ctrl.bTinyMode &&*/ (ctrl.h > (ctrl.nMaxOnMenu * (ctrl.nStringHeight+1)) + gap)) {
	  ctrl.bIsScrollBox = 1;
	  ctrl.h = (ctrl.nMaxOnMenu * (ctrl.nStringHeight + haveTitle) + haveTitle * LHY (ctrl.bTinyMode ? 12 : 8));
	 }
	else
		ctrl.bIsScrollBox = 0;
	ctrl.nMaxDisplayable = (ctrl.nMaxOnMenu < nItems) ? ctrl.nMaxOnMenu : nItems;
	ctrl.right_offset = 0;
	if (ctrl.width > -1)
		ctrl.w = ctrl.width;
	if (ctrl.height > -1)
		ctrl.h = ctrl.height;

	for (i = 0; i < nItems; i++) {
		item [i].w = ctrl.w;
		if (item [i].right_offset > ctrl.right_offset)
			ctrl.right_offset = item [i].right_offset;
		if (item [i].noscroll && (i == ctrl.nMaxNoScroll))
			ctrl.nMaxNoScroll++;
		}
	ctrl.nScrollOffset = ctrl.nMaxNoScroll;
	if (ctrl.right_offset > 0)
		ctrl.right_offset += 3;

	ctrl.w += ctrl.right_offset;
	ctrl.twidth = 0;
	if (ctrl.tw > ctrl.w)	{
		ctrl.twidth = (ctrl.tw - ctrl.w) / 2;
		ctrl.w = ctrl.tw;
	}

	if (RestoringMenu) { 
		ctrl.right_offset = 0; 
		ctrl.twidth = 0;
		}

	if (ctrl.w > ctrl.sc_w)
		ctrl.w = ctrl.sc_w;
	if (ctrl.h > ctrl.sc_h)
		ctrl.h = ctrl.sc_h;

	ctrl.xOffs = (gameStates.menus.bHires ? 30 : 15);
	i = (ctrl.sc_w - ctrl.w) / 2;
	if (i < ctrl.xOffs)
		ctrl.xOffs = i / 2;
	ctrl.yOffs = (gameStates.menus.bHires ? 30 : 15);
	if (ctrl.sc_h - ctrl.h < 2 * ctrl.yOffs)
		ctrl.h = ctrl.sc_h - 2 * ctrl.yOffs;
	ctrl.w += 2 * ctrl.xOffs;
	ctrl.h += 2 * ctrl.yOffs;
	ctrl.x = (ctrl.sc_w - ctrl.w) / 2;
	ctrl.y = (ctrl.sc_h - ctrl.h) / 2;
	if (gameOpts->menus.nStyle) {
		ctrl.xOffs += ctrl.x;
		ctrl.yOffs += ctrl.y;
		}
	ctrl.bValid = 1;
	*ctrlP = ctrl;
	NMSetItemPos (item, nItems, ctrl.twidth, ctrl.xOffs, ctrl.yOffs, ctrl.right_offset);
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

void NMSaveScreen (grs_canvas **save_canvas, grs_canvas **game_canvas, grs_font **saveFont)
{
WINDOS (*game_canvas = dd_grd_curcanv, *game_canvas = grdCurCanv);
WINDOS (*save_canvas = dd_grd_curcanv, *save_canvas = grdCurCanv);
*saveFont = grdCurCanv->cv_font;
WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (NULL));
}

//------------------------------------------------------------------------------

void NMRestoreScreen (char *filename, bkg *bg, grs_canvas *save_canvas, grs_font *saveFont, int bDontRestore)
{
WINDOS (DDGrSetCurrentCanvas (bg->menu_canvas), GrSetCurrentCanvas (bg->menu_canvas));
if (gameOpts->menus.nStyle)
	NMRemoveBackground (bg);
else {
	if (!filename) {
		// Save the background under the menu...
		WIN (DDGRLOCK (dd_grd_curcanv));
		GrBitmap (0, 0, bg->saved); 	
		WIN (DDGRUNLOCK (dd_grd_curcanv));
		GrFreeBitmap (bg->saved);
		d_free (bg->background);
		} 
	else {
		if (!bDontRestore) {	//info passed back from subfunction
			WIN (DDGRLOCK (dd_grd_curcanv));
			GrBitmap (0, 0, bg->background);
			WIN (DDGRUNLOCK (dd_grd_curcanv)); 	
			}
		GrFreeBitmap (bg->background);
		}
	GrUpdate (0);
	}
WINDOS (DDGrFreeSubCanvas (bg->menu_canvas), 
		  GrFreeSubCanvas (bg->menu_canvas));
WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (NULL));			
grdCurCanv->cv_font	= saveFont;
WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (save_canvas));
memset (bg, 0, sizeof (*bg));
GrabMouse (1, 0);
}

//------------------------------------------------------------------------------

void ShowMenuHelp (char *szHelp)
{
if (szHelp && *szHelp) {
	int nInMenu = gameStates.menus.nInMenu;
	int bFastMenus = gameOpts->menus.bFastMenus;
	gameStates.menus.nInMenu = 0;
	//gameOpts->menus.bFastMenus = 0;
	gameData.menu.helpColor = RGBA_PAL (47, 47, 47);
	gameData.menu.colorOverride = gameData.menu.helpColor;
	ExecMessageBox ("D2X-XL online help", NULL, -3, szHelp, " ", TXT_CLOSE);
	gameData.menu.colorOverride = 0;
	gameOpts->menus.bFastMenus = bFastMenus;
	gameStates.menus.nInMenu = nInMenu;
	}
}

//------------------------------------------------------------------------------

extern void OglDoFullScreenInternal (int bForce);

#define REDRAW_ALL	for (i = 0; i < nItems; i++) item [i].redraw = 1; bRedrawAll = 1

int ExecMenu4 (char *title, char *subtitle, int nItems, newmenu_item *item, 
					 void (*subfunction) (int nItems, newmenu_item *items, int *last_key, int cItem), 
					 int *cItemP, char *filename, int width, int height, int bTinyMode)
{
	int			old_keyd_repeat, done, cItem = cItemP ? *cItemP : 0;
	int			choice, old_choice, i;
	nm_control	ctrl;
	int			k, nLastScrollCheck=-1, sx, sy;
	grs_font		*saveFont;
	bkg			bg;
	int			bAllText=0;		//set true if all text items
	int			sound_stopped=0, time_stopped=0;
   int			topChoice;   // Is this a scrolling box? Set to 0 at init
   char			*Temp, TempVal;
	int			bDontRestore = 0;
	int			bRedraw = 0, bRedrawAll = 0, bStart = 1;
	WINDOS (dd_grs_canvas *save_canvas, grs_canvas *save_canvas);	
	WINDOS (dd_grs_canvas *game_canvas, grs_canvas *game_canvas);	
#ifdef NEWMENU_MOUSE
	int			nMouseState, nOldMouseState, bDblClick=0;
	int			mx=0, my=0, x1 = 0, x2, y1, y2;
#endif
	int			bLaunchOption = 0;
	int			bCloseBox = 0;

if (!grdCurScreen)
	return -1;
if (gameStates.menus.nInMenu)
	return -1;
memset (&bg, 0, sizeof (bg));
memset (&ctrl, 0, sizeof (ctrl));
ctrl.width = width;
ctrl.height = height;
ctrl.bTinyMode = bTinyMode;
FlushInput ();
PA_DFX (pa_set_frontbuffer_current ());
PA_DFX (pa_set_front_to_read ());

WIN (if (!_AppActive) {
	return -1}
	);		// Don't draw message if minimized!
newmenu_hide_cursor ();
if (nItems < 1)
	return -1;
SDL_EnableKeyRepeat(60, 30);
gameStates.menus.nInMenu++;
WIN (mouse_set_mode (0));		//disable centering mode
if (!gameOpts->menus.nStyle && (gameStates.app.nFunctionMode == FMODE_GAME) && !(gameData.app.nGameMode & GM_MULTI)) {
	DigiPauseDigiSounds ();
	sound_stopped = 1;
}

if (!(gameOpts->menus.nStyle || ((gameData.app.nGameMode & GM_MULTI) && (gameStates.app.nFunctionMode == FMODE_GAME) &&(!gameStates.app.bEndLevelSequence)))) {
	time_stopped = 1;
	StopTime ();
	#ifdef TACTILE 
	  if (TactileStick)	
		  DisableForces ();	
	#endif
}

if (gameStates.app.bGameRunning && (gameData.app.nGameMode && GM_MULTI))
	nTypingTimeout = 0;

#ifdef WINDOWS
RePaintNewmenu4:
#endif

SetPopupScreenMode ();
NMSaveScreen (&save_canvas, &game_canvas, &saveFont);
old_keyd_repeat = keyd_repeat;
keyd_repeat = 1;

if (cItem == -1)
	choice = -1;
else {
	if (cItem < 0) 
		cItem = 0;
	else 
		cItem %= nItems;
	choice = cItem;
#ifdef NEWMENU_MOUSE
	bDblClick = 1;
#endif
	while (item [choice].nType == NM_TYPE_TEXT) {
		choice++;
		if (choice >= nItems)
			choice = 0; 
		if (choice == cItem) {
			choice = 0; 
			bAllText = 1;
			break; 
			}
		}
	} 

done = 0;
topChoice = 0;
while (item [topChoice].nType == NM_TYPE_TEXT) {
	topChoice++;
	if (topChoice >= nItems)
		topChoice = 0; 
	if (topChoice == cItem) {
		topChoice = 0; 
		break; 
		}
	}

//GrUpdate (0);
// Clear mouse, joystick to clear button presses.
GameFlushInputs ();
#ifdef NEWMENU_MOUSE
nMouseState = nOldMouseState = 0;
bCloseBox = !(filename || gameStates.menus.bReordering);

if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING) {
	newmenu_show_cursor ();
# ifdef WINDOWS
	SetCursor (LoadCursor (NULL, IDC_ARROW);
# endif
	}
#endif

GrabMouse (0, 0);

while (!done) {
	if (cItemP)
		*cItemP = choice;
	if (gameStates.app.bGameRunning && (gameData.app.nGameMode && GM_MULTI)) {
		gameStates.multi.bPlayerIsTyping [gameData.multi.nLocalPlayer] = 1;
		MultiSendTyping ();
		}
#ifdef WINDOWS
		MSG msg;

	DoMessageStuff(&msg);
	if (_RedrawScreen) {
		_RedrawScreen = FALSE;
	if (!filename) {
		GrFreeBitmap(bg.saved);
		d_free(bg.background);
		}
	else 	
		GrFreeBitmap(bg.background);
	DDGrFreeSubCanvas(bg.menu_canvas);
	grdCurCanv->cv_font = saveFont;
	dd_grd_curcanv = save_canvas;
	goto RePaintNewmenu4;
	}
	DDGRRESTORE;
#endif
#ifdef NEWMENU_MOUSE
	if (!JOYDEFS_CALIBRATING)
		newmenu_show_cursor ();      // possibly hidden
	nOldMouseState = nMouseState;
	if (!gameStates.menus.bReordering) {
		int b = gameOpts->legacy.bInput;
		gameOpts->legacy.bInput = 1;
		nMouseState = MouseButtonState (0);
		gameOpts->legacy.bInput = b;
		}
#endif
	//see if redbook song needs to be restarted
	songs_check_redbook_repeat ();
	//NetworkListen ();
	k = KeyInKey ();
	if ((ctrl.sc_w != grdCurScreen->sc_w) || (ctrl.sc_h != grdCurScreen->sc_h)) {
		memset (&ctrl, 0, sizeof (ctrl));
		ctrl.width = width;
		ctrl.height = height;
		ctrl.bTinyMode = bTinyMode;
		}
#if 1
	if (ctrl.bValid && (ctrl.nDisplayMode != gameStates.video.nDisplayMode)) {
		NMFreeAllTextBms (item, nItems);
		NMRestoreScreen (filename, &bg, save_canvas, saveFont, bDontRestore);
		SetScreenMode (SCREEN_MENU);
		NMSaveScreen (&save_canvas, &game_canvas, &saveFont);
//		RemapFontsAndMenus (1);
		memset (&ctrl, 0, sizeof (ctrl));
		ctrl.width = width;
		ctrl.height = height;
		ctrl.bTinyMode = bTinyMode;
		}
#endif
	if (NMInitCtrl (&ctrl, title, subtitle, nItems, item)) {
		bRedraw = 0;
		ctrl.ty = ctrl.yOffs;
		if (choice > ctrl.nScrollOffset + ctrl.nMaxOnMenu - 1)
			ctrl.nScrollOffset = choice - ctrl.nMaxOnMenu + 1;
		}
	if (!gameOpts->menus.nStyle) {
		if (subfunction)
			(*subfunction) (nItems, item, &k, choice);
		}
	else {
		if (gameStates.app.bGameRunning) {
			WINDOS (dd_grs_canvas *save_canvas, grs_canvas *save_canvas);	
			WINDOS (save_canvas = dd_grd_curcanv, save_canvas = grdCurCanv);
			WINDOS (DDGrSetCurrentCanvas (game_canvas), GrSetCurrentCanvas (game_canvas));
			//GrPaletteStepLoad (gamePalette);
			//GrCopyPalette (grPalette, gamePalette, sizeof (grPalette));
			if (gameData.app.bGamePaused /*|| timer_paused*/)
				GameRenderFrame ();
			else {
				GameLoop (1, 0);
				}
			WINDOS (DDGrSetCurrentCanvas (save_canvas), GrSetCurrentCanvas (save_canvas));
			GrPaletteStepLoad (menuPalette);
			//RemapFontsAndMenus (1);
			}
		}

	if (!bRedraw || gameOpts->menus.nStyle) {
		int t;
		NMInitBackground (filename, &bg, ctrl.x, ctrl.y, ctrl.w, ctrl.h, bRedraw);
		if (!gameStates.app.bGameRunning)
			con_update();
		if (subfunction)
     		(*subfunction) (nItems, item, &k, choice);
		t = NMDrawTitle (title, TITLE_FONT, RGBA_PAL (31, 31, 31), ctrl.yOffs);
		NMDrawTitle (subtitle, SUBTITLE_FONT, RGBA_PAL (21, 21, 21), t);
		if (!bRedraw)
			ctrl.ty = t;
		grdCurCanv->cv_font = bTinyMode ? SMALL_FONT : NORMAL_FONT;
		}
#ifdef NETWORK
	if (!time_stopped){
		// Save current menu box
		if (MultiMenuPoll () == -1)
			k = -2;
		}
#endif

	if (k < -1) {
		bDontRestore = (k == -3);		//-3 means don't restore
		choice = k;
		k = -1;
		done = 1;
		}
#ifndef WINDOWS
	if (NMCheckButtonPress ())
		done = 1;
#endif

old_choice = choice;
if (k && con_events(k))
	switch (k)	{

		case KEY_SHIFTED + KEY_ESC:
			con_show();
			k = -1;
			break;
#ifdef NETWORK
		case KEY_I:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 ShowExtraNetGameInfo (choice - 2 - gameStates.multi.bUseTracker);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo)
			{
			 done=1;
			 choice=-1;
			}
		 break;
		case KEY_U:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 NetworkRequestPlayerNames (choice - 2 - gameStates.multi.bUseTracker);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo) {
			 done=1;
			 choice=-1;
			}
		 break;
#endif
		case KEY_CTRLED+KEY_F1:
			SwitchDisplayMode (-1);
			break;
		case KEY_CTRLED+KEY_F2:
			SwitchDisplayMode (1);
			break;
		case KEY_COMMAND + KEY_P:
		case KEY_CTRLED + KEY_P:
		case KEY_PAUSE:
		 if (Pauseable_menu) {	
			 Pauseable_menu=0;
			 done=1;
		 	 choice=-1;
			}
		 else
			 gameData.app.bGamePaused = !gameData.app.bGamePaused;
		 break;
		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAD8:
			if (bAllText) 
				break;
			do {
				choice--;
      		if (ctrl.bIsScrollBox) {
           		nLastScrollCheck=-1;
              	if (choice < topChoice) { 
						choice = nItems - 1; 
						ctrl.nScrollOffset = nItems - ctrl.nMaxDisplayable + ctrl.nMaxNoScroll;
						if (ctrl.nScrollOffset < ctrl.nMaxNoScroll)
							ctrl.nScrollOffset = ctrl.nMaxNoScroll;
						REDRAW_ALL;
						break; 
						}
             	else if (choice < ctrl.nScrollOffset) {
						REDRAW_ALL;
                  ctrl.nScrollOffset--;
						if (ctrl.nScrollOffset < ctrl.nMaxNoScroll)
							ctrl.nScrollOffset = ctrl.nMaxNoScroll;
					   }
	           	}
           	else {
           		if (choice >= nItems) 
						choice=0;
            	else if (choice < 0) 
						choice = nItems - 1;
           		}
				} while (item [choice].nType == NM_TYPE_TEXT);
			if ((item [choice].nType==NM_TYPE_INPUT) &&(choice!=old_choice))	
				item [choice].value = -1;
			if ((old_choice>-1) &&(item [old_choice].nType==NM_TYPE_INPUT_MENU) &&(old_choice!=choice))	{
				item [old_choice].group=0;
				strcpy (item [old_choice].text, item [old_choice].saved_text);
				item [old_choice].value = -1;
			}
			if (old_choice>-1) 
				item [old_choice].redraw = 1;
			item [choice].redraw=1;
			break;

		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAD2:
      	// ((0, "Pressing down! ctrl.bIsScrollBox=%d", ctrl.bIsScrollBox);
     		if (bAllText) 
				break;
			do {
				choice++;
        		if (ctrl.bIsScrollBox) {
            	nLastScrollCheck = -1;
					if (choice == nItems) {
						choice = 0;
						if (ctrl.nScrollOffset) {
							REDRAW_ALL;
			            ctrl.nScrollOffset = ctrl.nMaxNoScroll;
							}
						}
              	if (choice >= ctrl.nMaxOnMenu + ctrl.nScrollOffset - ctrl.nMaxNoScroll) {
						REDRAW_ALL;
                  ctrl.nScrollOffset++;
						}
           		}
            else {
         		if (choice < 0) 
						choice=nItems-1;
           		else if (choice >= nItems) 
						choice=0;
           		}
				} while (item [choice].nType==NM_TYPE_TEXT);
                                                      
			if ((item [choice].nType == NM_TYPE_INPUT) && (choice != old_choice))	
				item [choice].value = -1;
			if ((old_choice>-1) &&(item [old_choice].nType==NM_TYPE_INPUT_MENU) &&(old_choice!=choice))	{
				item [old_choice].group=0;
				strcpy (item [old_choice].text, item [old_choice].saved_text);	
				item [old_choice].value = -1;
			}
			if (old_choice>-1)
				item [old_choice].redraw=1;
			item [choice].redraw=1;
			break;
		case KEY_SPACEBAR:
checkOption:
radioOption:
			if (choice > -1)	{
				switch (item [choice].nType)	{
				case NM_TYPE_MENU:
				case NM_TYPE_INPUT:
				case NM_TYPE_INPUT_MENU:
					break;
				case NM_TYPE_CHECK:
					if (item [choice].value)
						item [choice].value = 0;
					else
						item [choice].value = 1;
					if (ctrl.bIsScrollBox) {
						if (choice== (ctrl.nMaxOnMenu+ctrl.nScrollOffset-1) || choice==ctrl.nScrollOffset)
							nLastScrollCheck=-1;					
						 }
				
					item [choice].redraw=1;
					if (bLaunchOption)
						goto launchOption;
					break;
				case NM_TYPE_RADIO:
					for (i=0; i<nItems; i++)	{
						if ((i!=choice) &&(item [i].nType==NM_TYPE_RADIO) &&(item [i].group==item [choice].group) &&(item [i].value))	{
							item [i].value = 0;
							item [i].redraw = 1;
						}
					}
					item [choice].value = 1;
					item [choice].redraw = 1;
					if (bLaunchOption)
						goto launchOption;
					break;
				}	
			}
			break;

      case KEY_SHIFTED+KEY_UP:
         if (gameStates.menus.bReordering && choice!=topChoice)
         {
            Temp=item [choice].text;
            TempVal=item [choice].value;
            item [choice].text=item [choice-1].text;
            item [choice].value=item [choice-1].value;
            item [choice-1].text=Temp;
            item [choice-1].value=TempVal;
            item [choice].rebuild=1;
            item [choice-1].rebuild=1;
            choice--;
         }
         break;
      case KEY_SHIFTED+KEY_DOWN:
         if (gameStates.menus.bReordering && choice!= (nItems-1))
         {
            Temp=item [choice].text;
            TempVal=item [choice].value;
            item [choice].text=item [choice+1].text;
            item [choice].value=item [choice+1].value;
            item [choice+1].text=Temp;
            item [choice+1].value=TempVal;
            item [choice].rebuild=1;
            item [choice+1].rebuild=1;
            choice++;
         }
         break;
                
		case KEY_ALTED + KEY_ENTER:
			{
			//int bLoadAltBg = NMFreeAltBg ();
			NMFreeAllTextBms (item, nItems);
			NMRestoreScreen (filename, &bg, save_canvas, saveFont, bDontRestore);
			GrToggleFullScreenGame ();
			GrabMouse (0, 0);
			SetScreenMode (SCREEN_MENU);
			NMSaveScreen (&save_canvas, &game_canvas, &saveFont);
			RemapFontsAndMenus (1);
			memset (&ctrl, 0, sizeof (ctrl));
			ctrl.width = width;
			ctrl.height = height;
			ctrl.bTinyMode = bTinyMode;
			continue;
			}

		case KEY_ENTER:
		case KEY_PADENTER:
launchOption:
			if ((choice>-1) &&(item [choice].nType==NM_TYPE_INPUT_MENU) &&(item [choice].group==0))	{
				item [choice].group = 1;
				item [choice].redraw = 1;
				if (!strnicmp (item [choice].saved_text, TXT_EMPTY, strlen (TXT_EMPTY)))	{
					item [choice].text [0] = 0;
					item [choice].value = -1;
					}
				else {	
					NMTrimWhitespace (item [choice].text);
					}
				}
			else {
				if (cItemP)
					*cItemP = choice;
				done = 1;
				}
			break;

		case KEY_ESC:
			if ((choice>-1) &&(item [choice].nType==NM_TYPE_INPUT_MENU) &&(item [choice].group==1))	{
				item [choice].group=0;
				strcpy (item [choice].text, item [choice].saved_text);	
				item [choice].redraw=1;
				item [choice].value = -1;
				}
			else {
				done = 1;
				if (cItemP)
					*cItemP = choice;
				choice = -1;
			}
			break;

		case KEY_COMMAND+KEY_SHIFTED+KEY_P:
		case KEY_ALTED + KEY_F9:
			bSaveScreenShot = 1;
			SaveScreenShot (NULL, 0);
			PA_DFX (pa_set_frontbuffer_current ());
			PA_DFX (pa_set_front_to_read ());
			for (i=0;i<nItems;i++)
				item [i].redraw=1;
			
			break;

		#ifndef NDEBUG
		case KEY_BACKSP:	
			if ((choice>-1) &&(item [choice].nType!=NM_TYPE_INPUT)&&(item [choice].nType!=NM_TYPE_INPUT_MENU))
				Int3 (); 
			break;
		#endif

		case KEY_F1:
			ShowMenuHelp (item [choice].szHelp);
			break;
		}

#ifdef NEWMENU_MOUSE // for mouse selection of menu's etc.
		WIN (Sleep (100));
		if (!done && nMouseState && !nOldMouseState && !bAllText) {
			mouse_get_pos (&mx, &my);
			for (i=0; i<nItems; i++)	{
				x1 = grdCurCanv->cv_bitmap.bm_props.x + item [i].x - item [i].right_offset - 6;
				x2 = x1 + item [i].w;
				y1 = grdCurCanv->cv_bitmap.bm_props.y + item [i].y;
				y2 = y1 + item [i].h;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
					if (i + ctrl.nScrollOffset - ctrl.nMaxNoScroll != choice) {
						if (Hack_DblClick_MenuMode) bDblClick = 0; 
					}
					
					choice = i + ctrl.nScrollOffset - ctrl.nMaxNoScroll;
					if (choice >= 0 && choice < nItems)
						switch (item [choice].nType)	{
							case NM_TYPE_CHECK:
								item [choice].value = !item [choice].value;
								item [choice].redraw=1;
								if (ctrl.bIsScrollBox)
									nLastScrollCheck=-1;
								break;
							case NM_TYPE_RADIO:
								for (i=0; i<nItems; i++)	{
									if ((i!=choice) &&(item [i].nType==NM_TYPE_RADIO) &&(item [i].group==item [choice].group) &&(item [i].value))	{
										item [i].value = 0;
										item [i].redraw = 1;
									}
								}
								item [choice].value = 1;
								item [choice].redraw = 1;
								break;
							}
					item [old_choice].redraw=1;
					break;
				}
			}
		}

		if (nMouseState && bAllText) {
			if (cItemP)
				*cItemP = choice;
			done = 1;
			}
		
		if (!done && nMouseState && !bAllText) {
			mouse_get_pos (&mx, &my);
			
			// check possible scrollbar stuff first
			if (ctrl.bIsScrollBox) {
				int i, arrow_width, arrow_height, aw;
				
				if (ctrl.nScrollOffset > ctrl.nMaxNoScroll) {
					GrGetStringSize (UP_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
					x2 = grdCurCanv->cv_bitmap.bm_props.x + item [ctrl.nScrollOffset].x- (gameStates.menus.bHires?24:12);
		         y1 = grdCurCanv->cv_bitmap.bm_props.y + item [ctrl.nScrollOffset].y- ((ctrl.nStringHeight+1)*(ctrl.nScrollOffset - ctrl.nMaxNoScroll));
					x1 = x2 - arrow_width;
					y2 = y1 + arrow_height;
					if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
						choice--;
		           	nLastScrollCheck=-1;
		            if (choice < ctrl.nScrollOffset) {
							REDRAW_ALL;
		               ctrl.nScrollOffset--;
			            }
						}
					}
				if ((i = ctrl.nScrollOffset + ctrl.nMaxDisplayable - ctrl.nMaxNoScroll) < nItems) {
					GrGetStringSize (DOWN_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
					x2 = grdCurCanv->cv_bitmap.bm_props.x + item [i-1].x - (gameStates.menus.bHires?24:12);
					y1 = grdCurCanv->cv_bitmap.bm_props.y + item [i-1].y - ((ctrl.nStringHeight+1)*(ctrl.nScrollOffset - ctrl.nMaxNoScroll));
					x1 = x2 - arrow_width;
					y2 = y1 + arrow_height;
					if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
						choice++;
		            nLastScrollCheck=-1;
		            if (choice >= ctrl.nMaxOnMenu + ctrl.nScrollOffset) {
							REDRAW_ALL;
		               ctrl.nScrollOffset++;
							}
						}
					}
				}
			
			for (i = ctrl.nScrollOffset; i < nItems; i++)	{
				x1 = grdCurCanv->cv_bitmap.bm_props.x + item [i].x - item [i].right_offset - 6;
				x2 = x1 + item [i].w;
				y1 = grdCurCanv->cv_bitmap.bm_props.y + item [i].y;
				y1 -= ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
				y2 = y1 + item [i].h;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2)) &&(item [i].nType != NM_TYPE_TEXT)) {
					if (i /*+ ctrl.nScrollOffset - ctrl.nMaxNoScroll*/ != choice) {
						if (Hack_DblClick_MenuMode) 
							bDblClick = 0; 
					}

					choice = i/* + ctrl.nScrollOffset - ctrl.nMaxNoScroll*/;

					if (item [choice].nType == NM_TYPE_SLIDER) {
						char slider_text [NM_MAX_TEXT_LEN+1], *p, *s1;
						int slider_width, height, aw, sleft_width, sright_width, smiddle_width;
						
						strcpy (slider_text, item [choice].saved_text);
						p = strchr (slider_text, '\t');
						if (p) {
							*p = '\0';
							s1 = p+1;
						}
						if (p) {
							GrGetStringSize (s1, &slider_width, &height, &aw);
							GrGetStringSize (SLIDER_LEFT, &sleft_width, &height, &aw);
							GrGetStringSize (SLIDER_RIGHT, &sright_width, &height, &aw);
							GrGetStringSize (SLIDER_MIDDLE, &smiddle_width, &height, &aw);

							x1 = grdCurCanv->cv_bitmap.bm_props.x + item [choice].x + item [choice].w - slider_width;
							x2 = x1 + slider_width + sright_width;
							if ((mx > x1) &&(mx < (x1 + sleft_width)) &&(item [choice].value != item [choice].minValue)) {
								item [choice].value = item [choice].minValue;
								item [choice].redraw = 2;
							} else if ((mx < x2) &&(mx > (x2 - sright_width)) &&(item [choice].value != item [choice].maxValue)) {
								item [choice].value = item [choice].maxValue;
								item [choice].redraw = 2;
							} else if ((mx > (x1 + sleft_width)) &&(mx < (x2 - sright_width))) {
								int numValues, value_width, newValue;
								
								numValues = item [choice].maxValue - item [choice].minValue + 1;
								value_width = (slider_width - sleft_width - sright_width) / numValues;
								newValue = (mx - x1 - sleft_width) / value_width;
								if (item [choice].value != newValue) {
									item [choice].value = newValue;
									item [choice].redraw = 2;
								}
							}
							*p = '\t';
						}
					}
					if (choice == old_choice)
						break;
					if ((item [choice].nType==NM_TYPE_INPUT) &&(choice!=old_choice))	
						item [choice].value = -1;
					if ((old_choice>-1) &&(item [old_choice].nType==NM_TYPE_INPUT_MENU) &&(old_choice!=choice))	{
						item [old_choice].group=0;
						strcpy (item [old_choice].text, item [old_choice].saved_text);
						item [old_choice].value = -1;
					}
					if (old_choice>-1) 
						item [old_choice].redraw = 1;
					item [choice].redraw=1;
					break;
				}
			}
		}
		
		if (!done && !nMouseState && nOldMouseState && !bAllText &&(choice != -1) &&(item [choice].nType == NM_TYPE_MENU)) {
			mouse_get_pos (&mx, &my);
			x1 = grdCurCanv->cv_bitmap.bm_props.x + item [choice].x;
			x2 = x1 + item [choice].w;
			y1 = grdCurCanv->cv_bitmap.bm_props.y + item [choice].y;
			if (choice >= ctrl.nScrollOffset)
				y1 -= ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
			y2 = y1 + item [choice].h;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				if (Hack_DblClick_MenuMode) {
					if (bDblClick) {
						if (cItemP)
							*cItemP = choice;
						done = 1;
						}
					else 
						bDblClick = 1;
				}
				else {
					done = 1;
					if (cItemP)
						*cItemP = choice;
				}
			}
		}
		
		if (!done && !nMouseState && nOldMouseState &&(choice>-1) &&(item [choice].nType==NM_TYPE_INPUT_MENU) &&(item [choice].group==0))	{
			item [choice].group = 1;
			item [choice].redraw = 1;
			if (!strnicmp (item [choice].saved_text, TXT_EMPTY, strlen (TXT_EMPTY)))	{
				item [choice].text [0] = 0;
				item [choice].value = -1;
			} else {	
				NMTrimWhitespace (item [choice].text);
			}
		}
		
		if (!done && !nMouseState && nOldMouseState && bCloseBox) {
			mouse_get_pos (&mx, &my);
			x1 = (gameOpts->menus.nStyle ? ctrl.x : grdCurCanv->cv_bitmap.bm_props.x) + CLOSE_X;
			x2 = x1 + CLOSE_SIZE;
			y1 = (gameOpts->menus.nStyle ? ctrl.y : grdCurCanv->cv_bitmap.bm_props.y) + CLOSE_Y;
			y2 = y1 + CLOSE_SIZE;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				if (cItemP)
					*cItemP = choice;
				choice = -1;
				done = 1;
				}
			}

//	 HACK! Don't redraw loadgame preview
		if (RestoringMenu) 
			item [0].redraw = 0;
#endif // NEWMENU_MOUSE

		if (choice > -1)	{
			int ascii;

			if (((item [choice].nType==NM_TYPE_INPUT)|| ((item [choice].nType==NM_TYPE_INPUT_MENU)&&(item [choice].group==1)))&&(old_choice==choice))	{
				if (k==KEY_LEFT || k==KEY_BACKSP || k==KEY_PAD4)	{
					if (item [choice].value==-1) 
						item [choice].value = (int) strlen (item [choice].text);
					if (item [choice].value > 0)
						item [choice].value--;
					item [choice].text [item [choice].value] = 0;
					item [choice].redraw = 1;	
					}
				else {
					ascii = KeyToASCII (k);
					if ((ascii < 255) && (item [choice].value < item [choice].text_len)) {
						int bAllowed;

						if (item [choice].value==-1)
							item [choice].value = 0;
						bAllowed = NMCharAllowed ((char) ascii);
						if (!bAllowed && ascii==' ' && NMCharAllowed ('_')) {
							ascii = '_';
							bAllowed=1;
							}
						if (bAllowed) {
							item [choice].text [item [choice].value++] = ascii;
							item [choice].text [item [choice].value] = 0;
							item [choice].redraw=1;	
							}
						}
					}
				}
			else if ((item [choice].nType!=NM_TYPE_INPUT) && (item [choice].nType!=NM_TYPE_INPUT_MENU)) {
				ascii = KeyToASCII (k);
				if (ascii < 255) {
					int choice1 = choice;
					ascii = toupper (ascii);
					do {
						int i, ch = 0, t;

						if (++choice1 >= nItems) 
							choice1=0;
						t = item [choice1].nType;
						if (item [choice1].key > 0)
							ch = MENU_KEY (item [choice1].key, 0);
						else if (item [choice1].key < 0) //skip any leading blanks
							for (i=0; (ch=item [choice1].text [i]) && ch==' ';i++)
								;
						else
							continue;
								;
						if (((t==NM_TYPE_MENU) ||
							  (t==NM_TYPE_CHECK) ||
							  (t==NM_TYPE_RADIO) ||
							  (t==NM_TYPE_NUMBER) ||
							  (t==NM_TYPE_SLIDER))
								&&(ascii==toupper (ch)))	{
							k = 0;
							choice = choice1;
							if (old_choice>-1)
								item [old_choice].redraw=1;
							item [choice].redraw=1;
							if (choice < ctrl.nScrollOffset) {
								ctrl.nScrollOffset = choice;
								REDRAW_ALL;
								}
							else if (choice > ctrl.nScrollOffset + ctrl.nMaxDisplayable - 1) {
								ctrl.nScrollOffset = choice;
								if (ctrl.nScrollOffset + ctrl.nMaxDisplayable >= nItems) {
									ctrl.nScrollOffset = nItems - ctrl.nMaxDisplayable;
									if (ctrl.nScrollOffset < ctrl.nMaxNoScroll)
										ctrl.nScrollOffset = ctrl.nMaxNoScroll;
									}
								REDRAW_ALL;
								}
							if (t==NM_TYPE_CHECK)
								goto checkOption;
							else if (t==NM_TYPE_RADIO)
								goto radioOption;
							else if (item [choice1].key != 0)
								if (gameOpts->menus.nHotKeys > 0)
									goto launchOption;
						}
					} while (choice1 != choice);
				}	
			}

			if ((item [choice].nType==NM_TYPE_NUMBER) || (item [choice].nType==NM_TYPE_SLIDER)) 	{
				int ov=item [choice].value;
				switch (k) {
				case KEY_PAD4:
			  	case KEY_LEFT:
			  	case KEY_MINUS:
				case KEY_MINUS+KEY_SHIFTED:
				case KEY_PADMINUS:
					item [choice].value -= 1;
					break;
			  	case KEY_RIGHT:
				case KEY_PAD6:
			  	case KEY_EQUAL:
				case KEY_EQUAL+KEY_SHIFTED:
				case KEY_PADPLUS:
					item [choice].value++;
					break;
				case KEY_PAGEUP:
				case KEY_PAD9:
				case KEY_SPACEBAR:
					item [choice].value += 10;
					break;
				case KEY_PAGEDOWN:
				case KEY_BACKSP:
				case KEY_PAD3:
					item [choice].value -= 10;
					break;
				}
				if (ov!=item [choice].value)
					item [choice].redraw=1;
			}
	
		}


    	// Redraw everything...
#if 1
		bRedraw = 0;
		if (bRedrawAll && !gameOpts->menus.nStyle) {
			int t;
			NMInitBackground (filename, &bg, ctrl.x, ctrl.y, ctrl.w, ctrl.h, bRedraw);
			t = NMDrawTitle (title, TITLE_FONT, RGBA_PAL (31, 31, 31), ctrl.yOffs);
			NMDrawTitle (subtitle, SUBTITLE_FONT, RGBA_PAL (21, 21, 21), t);
			bRedrawAll = 0;
			}
		WINDOS (	DDGrSetCurrentCanvas (bg.menu_canvas), 
				GrSetCurrentCanvas (bg.menu_canvas));
		grdCurCanv->cv_font = ctrl.bTinyMode ? SMALL_FONT : NORMAL_FONT;
     	for (i = 0; i < ctrl.nMaxDisplayable + ctrl.nScrollOffset - ctrl.nMaxNoScroll; i++) {
			if ((i >= ctrl.nMaxNoScroll) && (i < ctrl.nScrollOffset))
				continue;
			if (!(gameOpts->menus.nStyle || (item [i].text && *item [i].text)))
				continue;
			if (bStart || (curDrawBuffer == GL_BACK) || item [i].redraw || item [i].rebuild) {// warning! ugly hack below                  
				bRedraw = 1;
				if (item [i].rebuild && item [i].centered)
					item [i].x = GetCenteredX (item [i].text);
				if (i >= ctrl.nScrollOffset)
         		item [i].y -= ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
				if (!gameOpts->menus.nStyle) 
					newmenu_hide_cursor ();
           	NMDrawItem (&bg, item + i, (i == choice) && !bAllText, bTinyMode);
				item [i].redraw = 0;
#ifdef NEWMENU_MOUSE
				if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING)
					newmenu_show_cursor ();
#endif
				if (i >= ctrl.nScrollOffset)
	            item [i].y += ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
	        	}   
         if ((i == choice) && 
				 ((item [i].nType == NM_TYPE_INPUT) || 
				 ((item [i].nType == NM_TYPE_INPUT_MENU) && item [i].group)))
				NMUpdateCursor (&item [i]);
			}
#endif
      if (ctrl.bIsScrollBox) {
      	//grdCurCanv->cv_font = NORMAL_FONT;
        	if (bRedraw || (nLastScrollCheck != ctrl.nScrollOffset)) {
          	nLastScrollCheck=ctrl.nScrollOffset;
          	grdCurCanv->cv_font = SELECTED_FONT;
          	sy=item [ctrl.nScrollOffset].y - ((ctrl.nStringHeight+1)*(ctrl.nScrollOffset - ctrl.nMaxNoScroll));
          	sx=item [ctrl.nScrollOffset].x - (gameStates.menus.bHires?24:12);
          	if (ctrl.nScrollOffset > ctrl.nMaxNoScroll)
           		NMRStringWXY (&bg, (gameStates.menus.bHires ? 20 : 10), sx, sy, UP_ARROW_MARKER);
          	else
           		NMRStringWXY (&bg, (gameStates.menus.bHires ? 20 : 10), sx, sy, "  ");
				i = ctrl.nScrollOffset + ctrl.nMaxDisplayable - ctrl.nMaxNoScroll - 1;
          	sy=item [i].y - ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
          	sx=item [i].x - (gameStates.menus.bHires ? 24 : 12);
          	if (ctrl.nScrollOffset + ctrl.nMaxDisplayable - ctrl.nMaxNoScroll<nItems)
           		NMRStringWXY (&bg, (gameStates.menus.bHires ? 20 : 10), sx, sy, DOWN_ARROW_MARKER);
          	else
	           	NMRStringWXY (&bg, (gameStates.menus.bHires ? 20 : 10), sx, sy, "  ");
		    	}
     		}   
		if (bCloseBox && (bStart || gameOpts->menus.nStyle)) {
			if (gameOpts->menus.nStyle)
				NMDrawCloseBox (ctrl.x, ctrl.y);
			else
				NMDrawCloseBox (ctrl.x - grdCurCanv->cv_bitmap.bm_props.x, ctrl.y - grdCurCanv->cv_bitmap.bm_props.y);
			bCloseBox = 1;
			}
		if (bRedraw || !gameOpts->menus.nStyle)
			GrUpdate (0);
		bRedraw = 1;
		bStart = 0;
		if (!bDontRestore && gameStates.render.bPaletteFadedOut) {
			GrPaletteFadeIn (NULL, 32, 0);
		}
	}
newmenu_hide_cursor ();
// Restore everything...
NMRestoreScreen (filename, &bg, save_canvas, saveFont, bDontRestore);
NMFreeAllTextBms (item, nItems);
keyd_repeat = old_keyd_repeat;
GameFlushInputs ();
if (time_stopped) {
	StartTime ();
#ifdef TACTILE
		if (TactileStick)
			EnableForces ();
#endif
  }
if (sound_stopped)
	DigiResumeDigiSounds ();
WIN (mouse_set_mode (1));				//re-enable centering mode
#if 0 //def OGL_ZBUF
glDepthFunc (depthFunc);
#endif
gameStates.menus.nInMenu--;
SDL_EnableKeyRepeat(0, 0);
if (gameStates.app.bGameRunning && (gameData.app.nGameMode && GM_MULTI))
	MultiSendMsgQuit();
return choice;
}

//------------------------------------------------------------------------------

int _CDECL_ ExecMessageBox1 (
					char *title, 
					void (*subfunction) (int nItems, newmenu_item * items, int * last_key, int cItem), 
					char *filename, int nChoices, ...)
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text [MESSAGEBOX_TEXT_SIZE];
	newmenu_item nmMsgItems [5];
#if 0 //def OGL_ZBUF
	GLint depthFunc; 
	if (!gameOpts->legacy.bZBuf) {
		glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
		glDepthFunc (GL_ALWAYS);
		}
#endif

	va_start (args, nChoices);
	Assert (nChoices <= 5);
	memset (nmMsgItems, 0, sizeof (nmMsgItems));
	for (i=0; i<nChoices; i++)	{
      s = va_arg (args, char *);
      nmMsgItems [i].nType = NM_TYPE_MENU; 
		nmMsgItems [i].text = s;
		nmMsgItems [i].key = -1;
	}
	format = va_arg (args, char *);
	strcpy (nm_text, "");
	vsprintf (nm_text, format, args);
	va_end (args);

	Assert (strlen (nm_text) < MESSAGEBOX_TEXT_SIZE);

	return ExecMenu (title, nm_text, nChoices, nmMsgItems, subfunction, filename);
#if 0 //def OGL_ZBUF
	glDepthFunc (depthFunc);
#endif
}

//------------------------------------------------------------------------------

int _CDECL_ ExecMessageBox (char *title, char *filename, int nChoices, ...)
{
	int				h, i, l, bTiny;
	char				*format, *s;
	va_list			args;
	char				nm_text [MESSAGEBOX_TEXT_SIZE];
	newmenu_item	*nmMsgItems = NULL;


if (!nChoices)
	return -1;
if (bTiny = (nChoices < 0))
	nChoices = -nChoices;
va_start (args, nChoices);
nmMsgItems = (newmenu_item *) d_malloc (h = nChoices * sizeof (newmenu_item));
if (!nmMsgItems)
	return -1;
memset (nmMsgItems, 0, h);
for (i = l = 0; i < nChoices; i++) {
	s = va_arg (args, char *);
	h = (int) strlen (s);
	if (l + h > MESSAGEBOX_TEXT_SIZE)
		break;
	l += h;
	if (!bTiny || i) 
		nmMsgItems [i].nType = NM_TYPE_MENU; 
	else {
		nmMsgItems [i].nType = NM_TYPE_TEXT; 
		nmMsgItems [i].unavailable = 1; 
		}
	nmMsgItems [i].text = s;
	nmMsgItems [i].key = (bTiny && !i) ? 0 : -1;
	if (bTiny)
		nmMsgItems [i].centered = (i != 0);
	}
if (!bTiny) {
	format = va_arg (args, char *);
	vsprintf (nm_text, format, args);
	va_end (args);
	}
i = bTiny ? 
	 ExecMenutiny (NULL, title, nChoices, nmMsgItems, NULL) :
	 ExecMenu (title, nm_text, nChoices, nmMsgItems, NULL, filename);
#if 0 //def OGL_ZBUF
glDepthFunc (depthFunc);
#endif
d_free (nmMsgItems);
return i;
}

//------------------------------------------------------------------------------

void NMFileSort (int n, char *list)
{
	int i, j, incr;
	char t [FILENAME_LEN];

	incr = n / 2;
	while (incr > 0)		{
		for (i=incr; i<n; i++)		{
			j = i - incr;
			while (j>=0)			{
				if (strncmp (&list [j* (FILENAME_LEN + 1)], &list [ (j+incr)* (FILENAME_LEN + 1)], FILENAME_LEN - 1) > 0) {
               memcpy (t, &list [j* (FILENAME_LEN + 1)], FILENAME_LEN);
               memcpy (&list [j* (FILENAME_LEN + 1)], &list [ (j+incr)* (FILENAME_LEN + 1)], FILENAME_LEN);
               memcpy (&list [ (j+incr)* (FILENAME_LEN + 1)], t, FILENAME_LEN);
					j -= incr;
				}
				else
					break;
			}
		}
		incr = incr / 2;
	}
}

//------------------------------------------------------------------------------

void DeletePlayerSavedGames (char * name)
{
	int i;
	char filename [16];
	
	for (i=0;i<10; i++)	{
		sprintf (filename, "%s.sg%d", name, i);
		CFDelete (filename, gameFolders.szSaveDir);
	}
}

//------------------------------------------------------------------------------

#define MAX_FILES 300

int MakeNewPlayerFile (int allow_abort);

int ExecMenuFileSelector (char * title, char * filespec, char * filename, int allow_abortFlag)
{
	int i;
	FFS ffs;
	int NumFiles=0, key, done, cItem, ocitem;
	char * filenames = NULL;
	int NumFiles_displayed = 8;
	int first_item = -1, ofirst_item;
	int old_keyd_repeat = keyd_repeat;
	int player_mode=0;
	int demo_mode=0;
	int demos_deleted=0;
	int initialized = 0;
	int exitValue = 0;
	int w_x, w_y, w_w, w_h, title_height;
	int box_x, box_y, box_w, box_h;
	bkg bg;		// background under listbox
#ifdef NEWMENU_MOUSE
	int mx, my, x1, x2, y1, y2, nMouseState, nOldMouseState;
	int mouse2_state, omouse2_state;
	int bDblClick=0;
# ifdef WINDOWS
	int simukey=0;
	int show_up_arrow=0, show_down_arrow=0;
# endif
	char szPattern [40];
	int nPatternLen = 0;
	char *pszFn;
#endif
WIN (int win_redraw=0);

	w_x = w_y = w_w = w_h = title_height = 0;
	box_x = box_y = box_w = box_h = 0;

	if (! (filenames = d_malloc (MAX_FILES * (FILENAME_LEN + 1))))
		return 0;

	memset (&bg, 0, sizeof (bg));
	bg.bIgnoreBg = 1;
	cItem = 0;
	keyd_repeat = 1;

	WIN (mouse_set_mode (0));				//disable centering mode

	if (strstr (filespec, "*.plr"))
		player_mode = 1;
	else if (strstr (filespec, "*.dem"))
		demo_mode = 1;

ReadFileNames:
	done = 0;
	NumFiles=0;
	
#if !defined (APPLE_DEMO)		// no new pilots for special apple oem version
	if (player_mode)	{
		strncpy (filenames + NumFiles * (FILENAME_LEN + 1), TXT_CREATE_NEW, FILENAME_LEN);
		NumFiles++;
	}
#endif

	if (!FFF (filespec, &ffs, 0))	{
		pszFn = filenames + NumFiles * (FILENAME_LEN + 1);
		do	{
			if (NumFiles < MAX_FILES)	{
            strncpy (pszFn, ffs.name, FILENAME_LEN);
				if (player_mode)	{
					char * p = strchr (pszFn, '.');
					if (p) 
						*p = '\0';
				}
				if (*pszFn) {
					strlwr (pszFn);
					NumFiles++;
					}
			} else {
				break;
			}
		pszFn += (FILENAME_LEN + 1);
		} while (!FFN (&ffs, 0));
		FFC (&ffs);
	}
	if (demo_mode && gameFolders.bAltHogDirInited) {
		char filespec2 [PATH_MAX + FILENAME_LEN];
		sprintf (filespec2, "%s/%s", gameFolders.szAltHogDir, filespec);
		if (!FFF (filespec2, &ffs, 0)) {
			do {
				if (NumFiles<MAX_FILES)	{
					strncpy (filenames+NumFiles* (FILENAME_LEN + 1), ffs.name, FILENAME_LEN);
					NumFiles++;
				} else {
					break;
				}
			} while (!FFN (&ffs, 0));
			FFC (&ffs);
		}
	}

	if ((NumFiles < 1) && demos_deleted)	{
		exitValue = 0;
		goto ExitFileMenu;
	}
	if ((NumFiles < 1) && demo_mode) {
		ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
		exitValue = 0;
		goto ExitFileMenu;
	}

#if 0//ndef APPLE_DEMO
	if ((NumFiles < 2) && player_mode) {
		cItem = 0;
		goto ExitFileMenuEarly;
	}
#endif


	if (NumFiles<1)	{
		#ifndef APPLE_DEMO
			ExecMessageBox (NULL, NULL, 1, "Ok", "%s\n '%s' %s", TXT_NO_FILES_MATCHING, filespec, TXT_WERE_FOUND);
		#endif
		exitValue = 0;
		goto ExitFileMenu;
	}

	if (!initialized) {	
//		SetScreenMode (SCREEN_MENU);
		SetPopupScreenMode ();

        #ifdef WINDOWS
RePaintNewmenuFile:

		DDGrSetCurrentCanvas (NULL);
	#else
		GrSetCurrentCanvas (NULL);
	#endif

		WIN (DDGRLOCK (dd_grd_curcanv))					//mwa put these here -- are these needed Samir???
		{
			grdCurCanv->cv_font = SUBTITLE_FONT;
		}
		WIN (DDGRUNLOCK (dd_grd_curcanv));

		w_w = 0;
		w_h = 0;

		for (i=0; i<NumFiles; i++) {
			int w, h, aw;
			GrGetStringSize (filenames+i* (FILENAME_LEN+1), &w, &h, &aw);		
			if (w > w_w)
				w_w = w;
		}
		if (title) {
			int w, h, aw;
			GrGetStringSize (title, &w, &h, &aw);		
			if (w > w_w)
				w_w = w;
			title_height = h + (grd_curfont->ft_h*2);		// add a little space at the bottom of the title
		}

		box_w = w_w;
		box_h = ((grd_curfont->ft_h + 2) * NumFiles_displayed);

		w_w += (grd_curfont->ft_w * 4);
		w_h = title_height + box_h + (grd_curfont->ft_h * 2);		// more space at bottom

		if (w_w > grdCurCanv->cv_w) 
			w_w = grdCurCanv->cv_w;
		if (w_h > grdCurCanv->cv_h) 
			w_h = grdCurCanv->cv_h;
		if (w_w > 640)
			w_w = 640;
		if (w_h > 480)
			w_h = 480;
	
		w_x = (grdCurCanv->cv_w-w_w)/2;
		w_y = (grdCurCanv->cv_h-w_h)/2;
	
		if (w_x < 0) 
			w_x = 0;
		if (w_y < 0) 
			w_y = 0;

		box_x = w_x + (grd_curfont->ft_w*2);			// must be in sync with w_w!!!
		box_y = w_y + title_height;

// save the screen behind the menu.

		bg.saved = NULL;
		if (!gameOpts->menus.nStyle) {
#if !defined (WINDOWS)
			if ((VR_offscreen_buffer->cv_w >= w_w) &&(VR_offscreen_buffer->cv_h >= w_h)) 
				bg.background = &VR_offscreen_buffer->cv_bitmap;
			else
#endif
#if defined (POLY_ACC)
				bg.background = GrCreateBitmap2 (w_w, w_h, grdCurCanv->cv_bitmap.bm_props.nType, NULL);
#else
				bg.background = GrCreateBitmap (w_w, w_h, 0);
#endif
			Assert (bg.background != NULL);
			WIN (DDGRLOCK (dd_grd_curcanv));
			GrBmBitBlt (w_w, w_h, 0, 0, w_x, w_y, &grdCurCanv->cv_bitmap, bg.background);
			WIN (DDGRUNLOCK (dd_grd_curcanv));
			}
#if 0
		WINDOS (
	 		dd_gr_blt_notrans (dd_grd_curcanv, 0, 0, 
				_DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh, 
				dd_VR_offscreen_buffer, 0, 0, 
				_DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh), 
			GrBmBitBlt (grdCurCanv->cv_w, grdCurCanv->cv_h, 0, 0, 0, 0, &(grdCurCanv->cv_bitmap), &(VR_offscreen_buffer->cv_bitmap))
		);
#endif

		NMDrawBackground (&bg, w_x, w_y, w_x+w_w-1, w_y+w_h-1, 0);
		WIN (DDGRLOCK (dd_grd_curcanv))
		{	
			GrString (0x8000, w_y+10, title);
		}
		WIN (DDGRUNLOCK (dd_grd_curcanv));

		WIN (DDGRRESTORE);
		initialized = 1;
	}

	if (!player_mode)	{
		NMFileSort (NumFiles, filenames);
	} else {
		NMFileSort (NumFiles-1, filenames+ (FILENAME_LEN+1));		// Don't sort first one!
		for (i=0; i<NumFiles; i++)	{
			if (!stricmp (gameData.multi.players [gameData.multi.nLocalPlayer].callsign, filenames+i* (FILENAME_LEN+1)))	{
#ifdef NEWMENU_MOUSE
				bDblClick = 1;
#endif
				cItem = i;
			}
	 	}
	}

#ifdef NEWMENU_MOUSE
	nMouseState = nOldMouseState = 0;
	mouse2_state = omouse2_state = 0;
	NMDrawCloseBox (w_x, w_y);
	newmenu_show_cursor ();
#endif

	SDL_EnableKeyRepeat(60, 30);
	while (!done)	{
#ifdef WINDOWS
		MSG msg;

		DoMessageStuff (&msg);

		if (_RedrawScreen) {
			_RedrawScreen = 0;

			if (bg.background != &VR_offscreen_buffer->cv_bitmap)
				GrFreeBitmap (bg.background);
	
			win_redraw = 1;		
			goto RePaintNewmenuFile;
		}

		DDGRRESTORE
#endif
		ocitem = cItem;
		ofirst_item = first_item;
		GrUpdate (0);
#ifdef NEWMENU_MOUSE
		nOldMouseState = nMouseState;
		omouse2_state = mouse2_state;
		nMouseState = MouseButtonState (0);
		mouse2_state = MouseButtonState (1);
#endif

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat ();

		#ifdef WINDOWS
		if (!mouse2_state && omouse2_state)
			key = KEY_CTRLED+KEY_D;		//fake ctrl-d
		else
		#endif
			//NOTE LINK TO ABOVE ELSE
			key = KeyInKey ();

	#ifdef WINDOWS
		if (simukey==-1)
			key=KEY_UP;
		else if (simukey==1)
		   key=KEY_DOWN;
		simukey=0;
	#endif
			
		switch (key)	{
		case KEY_CTRLED+KEY_F1:
			SwitchDisplayMode (-1);
			break;
		case KEY_CTRLED+KEY_F2:
			SwitchDisplayMode (1);
			break;
		case KEY_COMMAND+KEY_SHIFTED+KEY_P:
		case KEY_ALTED + KEY_F9:
			bSaveScreenShot = 1;
			SaveScreenShot (NULL, 0);
			PA_DFX (pa_set_frontbuffer_current ());
			PA_DFX (pa_set_front_to_read ());
			
			break;

		case KEY_CTRLED+KEY_S:
			if (gameStates.app.bNostalgia)
				gameOpts->menus.bSmartFileSearch = 0;
			else
				gameOpts->menus.bSmartFileSearch = !gameOpts->menus.bSmartFileSearch;
			break;

		case KEY_CTRLED+KEY_D:

			if (((player_mode)&&(cItem>0)) || ((demo_mode)&&(cItem>=0)))	{
				int x = 1;
				#ifdef WINDOWS
				mouse_set_mode (1);				//re-enable centering mode
				#endif
				newmenu_hide_cursor ();
				if (player_mode)
					x = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, &filenames [cItem* (FILENAME_LEN+1)]+ ((player_mode && filenames [cItem* (FILENAME_LEN+1)]=='$')?1:0));
				else if (demo_mode)
					x = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, &filenames [cItem* (FILENAME_LEN+1)]+ ((demo_mode && filenames [cItem* (FILENAME_LEN+1)]=='$')?1:0));
				#ifdef WINDOWS
				mouse_set_mode (0);				//disenable centering mode
				#endif
				newmenu_show_cursor ();
 				if (x==0)	{
					char * p;
					int ret;
					char name [256], dir [256];

					p = &filenames [ (cItem* (FILENAME_LEN+1))+strlen (&filenames [cItem* (FILENAME_LEN+1)])];
					if (player_mode)
						*p = '.';

					_splitpath (filespec, name, dir, NULL, NULL);
					strcat (name, dir);
					strcat (name, &filenames [cItem* (FILENAME_LEN+1)]);
					
					ret = CFDelete (name, player_mode ? "" : gameFolders.szDemoDir);
					if (player_mode)
						*p = 0;

					if ((!ret) && player_mode)	{
						DeletePlayerSavedGames (&filenames [cItem* (FILENAME_LEN+1)]);
					}

					if (ret) {
						if (player_mode)
							ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, &filenames [cItem* (FILENAME_LEN+1)]+ ((player_mode && filenames [cItem* (FILENAME_LEN+1)]=='$')?1:0));
						else if (demo_mode)
							ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, &filenames [cItem* (FILENAME_LEN+1)]+ ((demo_mode && filenames [cItem* (FILENAME_LEN+1)]=='$')?1:0));
					} else if (demo_mode)
						demos_deleted = 1;
					first_item = -1;
					goto ReadFileNames;
				}
			}
			break;
		case KEY_HOME:
		case KEY_PAD7:
			cItem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			cItem = NumFiles-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			cItem--;			
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			cItem++;			
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			cItem += NumFiles_displayed;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			cItem -= NumFiles_displayed;
			break;
		case KEY_ESC:
			if (allow_abortFlag) {
				cItem = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = 1;
			break;

		case KEY_BACKSP:
			if (!gameOpts->menus.bSmartFileSearch)
				break;
			if (nPatternLen > 0)
				szPattern [--nPatternLen] = '\0';
					
		default:	
			if (!gameOpts->menus.bSmartFileSearch || (nPatternLen < sizeof (szPattern) - 1)) {
				int nStart, ascii = KeyToASCII (key);
				if ((key == KEY_BACKSP) || (ascii < 255)) {
					int cc, bFound = 0;
					if (!gameOpts->menus.bSmartFileSearch) {
						cc = cItem + 1;
						nStart = cItem;
						if (cc < 0)  
							cc = 0;
						else if (cc >= NumFiles)  
							cc = 0;
						}
					else {
						cc = 0;
						nStart = 0;
						if (key != KEY_BACKSP) {
							szPattern [nPatternLen++] = tolower (ascii);
							szPattern [nPatternLen] = '\0';
							}
						}
					do {
						pszFn = filenames + cc * (FILENAME_LEN + 1);
						if (gameOpts->menus.bSmartFileSearch ? strstr (pszFn, szPattern) == pszFn : *pszFn == toupper (ascii)) {
							cItem = cc;
							bFound = 1;
							break;
						}
						cc++;
						cc %= NumFiles;
					} while (cc != nStart);
				if (gameOpts->menus.bSmartFileSearch && !bFound)
					szPattern [--nPatternLen] = '\0';
				}
			}
		}
		if (done) break;


		if (cItem<0)
			cItem=0;

		if (cItem>=NumFiles)
			cItem = NumFiles-1;

		if (cItem< first_item)
			first_item = cItem;

		if (cItem>= (first_item+NumFiles_displayed))
		{
			first_item = cItem-NumFiles_displayed+1;
		}

#ifdef WINDOWS
		if (NumFiles>first_item+NumFiles_displayed)
			show_down_arrow=1;
		else 
			show_down_arrow=0;
		if (first_item>0)
			show_up_arrow=1;
		else	
			show_up_arrow=0;
#endif
			

		if (NumFiles <= NumFiles_displayed)
			 first_item = 0;

		if (first_item>NumFiles-NumFiles_displayed)
		{
			first_item = NumFiles-NumFiles_displayed;
		}

		if (first_item < 0) first_item = 0;

#ifdef NEWMENU_MOUSE
		WIN (Sleep (100));
		if (nMouseState || mouse2_state) {
			int w, h, aw;

			mouse_get_pos (&mx, &my);
			for (i=first_item; i<first_item+NumFiles_displayed; i++)	{
				GrGetStringSize (&filenames [i* (FILENAME_LEN+1)], &w, &h, &aw);
				x1 = box_x;
				x2 = box_x + box_w - 1;
				y1 = (i-first_item)* (grd_curfont->ft_h + 2) + box_y;
				y2 = y1+h+1;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
					if (i == cItem && !mouse2_state) {
						break;
					}
					cItem = i;
					bDblClick = 0;
					break;
				}
			}
		}
		
		if (!nMouseState && nOldMouseState) {
			int w, h, aw;

			GrGetStringSize (&filenames [cItem* (FILENAME_LEN+1)], &w, &h, &aw);
			mouse_get_pos (&mx, &my);
			x1 = box_x;
			x2 = box_x + box_w - 1;
			y1 = (cItem-first_item)* (grd_curfont->ft_h + 2) + box_y;
			y2 = y1+h+1;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				if (bDblClick) 
					done = 1;
				else 
					bDblClick = 1;
			}
		}

		if (!nMouseState && nOldMouseState) {
			mouse_get_pos (&mx, &my);
			x1 = w_x + CLOSE_X + 2;
			x2 = x1 + CLOSE_SIZE - 2;
			y1 = w_y + CLOSE_Y + 2;
			y2 = y1 + CLOSE_SIZE - 2;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				cItem = -1;
				done = 1;
			}
		   #ifdef WINDOWS
			x1 = box_x-LHX (10);
			x2 = x1 + LHX (10);
			y1 = box_y;
			y2 = box_y+LHY (7);
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2)) && show_up_arrow) 
				simukey = -1;
			y1 = box_y+box_h-LHY (7);
			y2 = box_y+box_h;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2)) && show_down_arrow) 
				simukey = 1;
		   #endif
		}

#endif
  
	WIN (DDGRLOCK (dd_grd_curcanv));
#if 0
		GrSetColorRGBi (RGBA_PAL2 (2, 2, 2));
		GrURect (box_x - 1, box_y-2, box_x + box_w, box_y-2);
		GrSetColorRGB (0, 0, 0, 255);
#endif
	#ifdef WINDOWS
		if (ofirst_item != first_item || win_redraw)	{
			win_redraw = 0;
	#else
		if ((ofirst_item != first_item) || gameOpts->menus.nStyle) {
	#endif
			if (!gameOpts->menus.nStyle) 
				newmenu_hide_cursor ();
			NMDrawBackground (&bg, w_x, w_y, w_x+w_w-1, w_y+w_h-1,1);
			WIN (DDGRLOCK (dd_grd_curcanv))
			{	
				grdCurCanv->cv_font = NORMAL_FONT;
				GrString (0x8000, w_y+10, title);
			}
			WIN (DDGRUNLOCK (dd_grd_curcanv));
			GrSetColorRGB (0, 0, 0, 255);
			for (i=first_item; i<first_item+NumFiles_displayed; i++)	{
				int w, h, aw, y;
				y = (i-first_item)* (grd_curfont->ft_h + 2) + box_y;
			
				if (i >= NumFiles)	{

					GrSetColorRGBi (RGBA_PAL2 (5, 5, 5));
					GrRect (box_x + box_w, y-1, box_x + box_w, y + grd_curfont->ft_h + 1);
					//GrRect (box_x, y + grd_curfont->ft_h + 2, box_x + box_w, y + grd_curfont->ft_h + 2);
					
					GrSetColorRGBi (RGBA_PAL2 (2, 2, 2));
					GrRect (box_x - 1, y - 1, box_x - 1, y + grd_curfont->ft_h + 2);
					
					GrSetColorRGB (0, 0, 0, 255);
					GrRect (box_x, y - 1, box_x + box_w - 1, y + grd_curfont->ft_h + 1);
					
				} else {
					if (i == cItem)	
						grdCurCanv->cv_font = SELECTED_FONT;
					else	
						grdCurCanv->cv_font = NORMAL_FONT;
					GrGetStringSize (&filenames [i* (FILENAME_LEN+1)], &w, &h, &aw);

					GrSetColorRGBi (RGBA_PAL2 (5, 5, 5));
				  //	GrRect (box_x, y + h + 2, box_x + box_w, y + h + 2);
					GrRect (box_x + box_w, y - 1, box_x + box_w, y + h + 1);
					
					GrSetColorRGBi (RGBA_PAL2 (2, 2, 2));
					GrRect (box_x - 1, y - 1, box_x - 1, y + h + 1);
					GrSetColorRGB (0, 0, 0, 255);
							
					GrRect (box_x, y-1, box_x + box_w - 1, y + h + 1);
					GrString (box_x + 5, y, (&filenames [i* (FILENAME_LEN+1)])+ ((player_mode && filenames [i* (FILENAME_LEN+1)]=='$')?1:0));
				}
			}	 
			newmenu_show_cursor ();
		} else if (cItem != ocitem)	{
			int w, h, aw, y;

			if (!gameOpts->menus.nStyle) 
				newmenu_hide_cursor ();
			i = ocitem;
			if ((i>=0) &&(i<NumFiles))	{
				y = (i-first_item)* (grd_curfont->ft_h+2)+box_y;
				if (i == cItem)	
					grdCurCanv->cv_font = SELECTED_FONT;
				else	
					grdCurCanv->cv_font = NORMAL_FONT;
				GrGetStringSize (&filenames [i* (FILENAME_LEN+1)], &w, &h, &aw);
				GrRect (box_x, y-1, box_x + box_w - 1, y + h + 1);
				GrString (box_x + 5, y, (&filenames [i* (FILENAME_LEN+1)])+ ((player_mode && filenames [i* (FILENAME_LEN+1)]=='$')?1:0));
			}
			i = cItem;
			if ((i>=0) &&(i<NumFiles))	{
				y = (i-first_item)* (grd_curfont->ft_h+2)+box_y;
				if (i == cItem)	
					grdCurCanv->cv_font = SELECTED_FONT;
				else	
					grdCurCanv->cv_font = NORMAL_FONT;
				GrGetStringSize (&filenames [i* (FILENAME_LEN+1)], &w, &h, &aw);
				GrRect (box_x, y-1, box_x + box_w - 1, y + h + 1);
				GrString (box_x + 5, y, (&filenames [i* (FILENAME_LEN+1)])+ ((player_mode && filenames [i* (FILENAME_LEN+1)]=='$')?1:0));
			}
			GrUpdate (0);
			newmenu_show_cursor ();
		}

	#ifdef WINDOWS   
			grdCurCanv->cv_font = NORMAL_FONT;
			if (show_up_arrow)
	 	  		GrString (box_x-LHX (10), box_y , UP_ARROW_MARKER);
			else
			{
				bNoDarkening=1;
				NMDrawBackground (bg, box_x-LHX (10), box_y, box_x-2, box_y+LHY (7);
				bNoDarkening=0;
			}

			if (show_down_arrow)
	     		GrString (box_x-LHX (10), box_y+box_h-LHY (7) , DOWN_ARROW_MARKER);
			else
			{
				bNoDarkening=1;
				NMDrawBackground (bg, box_x-LHX (10), box_y+box_h-LHY (7), box_x-2, box_y+box_h);
				bNoDarkening=0;
			}

	#endif



	WIN (DDGRUNLOCK (dd_grd_curcanv));
	}

//ExitFileMenuEarly:
	if (cItem > -1)	{
		strncpy (filename, (&filenames [cItem* (FILENAME_LEN+1)])+ ((player_mode && filenames [cItem* (FILENAME_LEN+1)]=='$')?1:0), FILENAME_LEN);
		exitValue = 1;
	} else {
		exitValue = 0;
	}											 

ExitFileMenu:
	keyd_repeat = old_keyd_repeat;

	if (initialized) {
		if (gameOpts->menus.nStyle) {
			NMRemoveBackground (&bg);
			}
		else {
			if (gameData.demo.nState != ND_STATE_PLAYBACK)	//horrible hack to prevent restore when screen has been cleared
			{
			WIN (DDGRLOCK (dd_grd_curcanv));
			GrBmBitBlt (w_w, w_h, w_x, w_y, 0, 0, bg.background, &grdCurCanv->cv_bitmap);
			WIN (DDGRUNLOCK (dd_grd_curcanv)); 	
			}
			if (bg.background != &VR_offscreen_buffer->cv_bitmap)
				GrFreeBitmap (bg.background);
#if 0
		WINDOS (
			dd_gr_blt_notrans (dd_VR_offscreen_buffer, 
				0, 0, _DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh, 
				dd_grd_curcanv, 
				0, 0, _DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh), 
			GrBmBitBlt (grdCurCanv->cv_w, grdCurCanv->cv_h, 0, 0, 0, 0, &(VR_offscreen_buffer->cv_bitmap), &(grdCurCanv->cv_bitmap))
		);
#endif
		GrUpdate (0);
		WIN (DDGRRESTORE);
		}
	}

	if (filenames)
		d_free (filenames);

	WIN (mouse_set_mode (1));				//re-enable centering mode
	WIN (newmenu_hide_cursor ());

	SDL_EnableKeyRepeat(0, 0);
	return exitValue;

}

//------------------------------------------------------------------------------
// Example listbox callback function...
// int lb_callback (int * cItem, int *nItems, char * items [], int *keypress)
// {
// 	int i;
// 
// 	if (*keypress = KEY_CTRLED+KEY_D)	{
// 		if (*nItems > 1)	{
// 			CFDelete (items [*cItem]);    // Delete the file
// 			for (i=*cItem; i<*nItems-1; i++)	{
// 				items [i] = items [i+1];
// 			}
// 			*nItems = *nItems - 1;
// 			d_free (items [*nItems]);
// 			items [*nItems] = NULL;
// 			return 1;	// redraw;
// 		}
//			*keypress = 0;
// 	}			
// 	return 0;
// }

//------------------------------------------------------------------------------

#define LB_ITEMS_ON_SCREEN 8

int ExecMenuListBox (char * title, int nItems, char * items [], int allow_abortFlag, int (*listbox_callback) (int * cItem, int *nItems, char * items [], int *keypress))
{
return ExecMenuListBox1 (title, nItems, items, allow_abortFlag, 0, listbox_callback);
}

//------------------------------------------------------------------------------

int ExecMenuListBox1 (char * title, int nItems, char * items [], int allow_abortFlag, int default_item, int (*listbox_callback) (int * cItem, int *nItems, char * items [], int *keypress))
{
	int i;
	int done, ocitem, cItem, ofirst_item, first_item, key, redraw;
	int old_keyd_repeat = keyd_repeat;
	int width, height, wx, wy, title_height, border_size;
	int total_width, total_height;
	bkg bg;
#ifdef NEWMENU_MOUSE
	int mx, my, x1, x2, y1, y2, nMouseState, nOldMouseState;	//, bDblClick;
	int close_x, close_y;
# ifdef WINDOWS
   int simukey=0, show_up_arrow=0, show_down_arrow=0;
# endif
#endif
	int	nItemsOnScreen;
	char szPattern [40];
	int nPatternLen = 0;
	char *pszFn;
WIN (int win_redraw=0);

	keyd_repeat = 1;

   PA_DFX (pa_set_frontbuffer_current ());
	PA_DFX (pa_set_front_to_read ());
	WIN (mouse_set_mode (0));				//disable centering mode

//	SetScreenMode (SCREEN_MENU);
	SetPopupScreenMode ();
	memset (&bg, 0, sizeof (bg));
	bg.bIgnoreBg = 1;

#ifdef WINDOWS
RePaintNewmenuListbox:
 
	DDGrSetCurrentCanvas (NULL);
#else
	GrSetCurrentCanvas (NULL);
#endif
	grdCurCanv->cv_font = SUBTITLE_FONT;

	width = 0;
	for (i=0; i<nItems; i++)	{
		int w, h, aw;
		GrGetStringSize (items [i], &w, &h, &aw);		
		if (w > width)
			width = w;
	}
	nItemsOnScreen = LB_ITEMS_ON_SCREEN * grdCurCanv->cv_bitmap.bm_props.h / 480;
	height = (grd_curfont->ft_h + 2) * nItemsOnScreen;

	{
		int w, h, aw;
		GrGetStringSize (title, &w, &h, &aw);		
		if (w > width)
			width = w;
		title_height = h + 5;
	}

	border_size = grd_curfont->ft_w;
   WIN (border_size=grd_curfont->ft_w*2);
		
	width += (grd_curfont->ft_w);
	if (width > grdCurCanv->cv_w - (grd_curfont->ft_w * 3))
		width = grdCurCanv->cv_w - (grd_curfont->ft_w * 3);

	wx = (grdCurCanv->cv_bitmap.bm_props.w-width)/2;
	wy = (grdCurCanv->cv_bitmap.bm_props.h- (height+title_height))/2 + title_height;
	if (wy < title_height)
		wy = title_height;

	total_width = width+2*border_size;
	total_height = height+2*border_size+title_height;

	bg.saved = NULL;

	if (!gameOpts->menus.nStyle) {
#if !defined (WINDOWS)
		if ((VR_offscreen_buffer->cv_w >= total_width) &&(VR_offscreen_buffer->cv_h >= total_height))
			bg.background = &VR_offscreen_buffer->cv_bitmap;
		else
#endif
			//bg.background = GrCreateBitmap (width, (height + title_height));
#if defined (POLY_ACC)
			bg.background = GrCreateBitmap2 (total_width, total_height, grdCurCanv->cv_bitmap.bm_props.nType, NULL);
#else
			bg.background = GrCreateBitmap (total_width, total_height, 0);
#endif
		Assert (bg.background != NULL);
		WIN (DDGRLOCK (dd_grd_curcanv));
			//GrBmBitBlt (wx+width+border_size, wy+height+border_size, 0, 0, wx-border_size, wy-title_height-border_size, &grdCurCanv->cv_bitmap, bg.background);
			GrBmBitBlt (total_width, total_height, 0, 0, wx-border_size, wy-title_height-border_size, &grdCurCanv->cv_bitmap, bg.background);
		WIN (DDGRUNLOCK (dd_grd_curcanv));
		}

#if 0
	WINDOS (
 		dd_gr_blt_notrans (dd_grd_curcanv, 0, 0, 
				_DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh, 
				dd_VR_offscreen_buffer, 0, 0, 
				_DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh), 
		GrBmBitBlt (grdCurCanv->cv_w, grdCurCanv->cv_h, 0, 0, 0, 0, &(grdCurCanv->cv_bitmap), &(VR_offscreen_buffer->cv_bitmap))
	);
#endif

	NMDrawBackground (&bg, wx-border_size, wy-title_height-border_size, wx+width+border_size-1, wy+height+border_size-1,0);
	GrUpdate (0);
	WIN (DDGRLOCK (dd_grd_curcanv));
		GrString (0x8000, wy - title_height, title);
	WIN (DDGRUNLOCK (dd_grd_curcanv));	

	WIN (DDGRRESTORE);

	done = 0;
	cItem = default_item;
	if (cItem < 0) 
		cItem = 0;
	if (cItem >= nItems) 
		cItem = 0;

	first_item = -1;

#ifdef NEWMENU_MOUSE
	nMouseState = nOldMouseState = 0;	//bDblClick = 0;
	close_x = wx-border_size;
	close_y = wy-title_height-border_size;
	NMDrawCloseBox (close_x, close_y);
	newmenu_show_cursor ();
#endif

	SDL_EnableKeyRepeat(60, 30);
	while (!done)	{
#ifdef WINDOWS
		MSG msg;

		DoMessageStuff (&msg);

		if (_RedrawScreen) {
			_RedrawScreen = 0;

			if (bg.background != &VR_offscreen_buffer->cv_bitmap)
				GrFreeBitmap (bg.background);
			win_redraw = 1;			
			goto RePaintNewmenuListbox;
		}

	  	DDGRRESTORE;
#endif
  
		ocitem = cItem;
		ofirst_item = first_item;
#ifdef NEWMENU_MOUSE
		nOldMouseState = nMouseState;
		nMouseState = MouseButtonState (0);
#endif
		//see if redbook song needs to be restarted
		songs_check_redbook_repeat ();

		key = KeyInKey ();

		if (listbox_callback)
			redraw = (*listbox_callback) (&cItem, &nItems, items, &key);
		else
			redraw = 0;

	#ifdef WINDOWS
		if (win_redraw) {
			redraw = 1;
			win_redraw = 0;
		}
	#endif

		if (key<-1) {
			cItem = key;
			key = -1;
			done = 1;
		}


	#ifdef WINDOWS
		if (simukey==-1)
			key=KEY_UP;
		else if (simukey==1)
		   key=KEY_DOWN;
		simukey=0;
	#endif
		
		switch (key)	{
		case KEY_CTRLED+KEY_F1:
			SwitchDisplayMode (-1);
			break;
		case KEY_CTRLED+KEY_F2:
			SwitchDisplayMode (1);
			break;
		case KEY_CTRLED+KEY_S:
			if (gameStates.app.bNostalgia)
				gameOpts->menus.bSmartFileSearch = 0;
			else
				gameOpts->menus.bSmartFileSearch = !gameOpts->menus.bSmartFileSearch;
			break;

		case KEY_COMMAND+KEY_SHIFTED+KEY_P:
		case KEY_PRINT_SCREEN: 		
			SaveScreenShot (NULL, 0); 
			PA_DFX (pa_set_frontbuffer_current ());
			PA_DFX (pa_set_front_to_read ());			
			break;
		case KEY_HOME:
		case KEY_PAD7:
			cItem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			cItem = nItems-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			cItem--;			
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			cItem++;			
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			cItem += nItemsOnScreen;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			cItem -= nItemsOnScreen;
			break;
		case KEY_ESC:
			if (allow_abortFlag) {
				cItem = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = 1;
			break;

		case KEY_BACKSP:
			if (!gameOpts->menus.bSmartFileSearch)
				break;
			if (nPatternLen > 0)
				szPattern [--nPatternLen] = '\0';
					
		default:	
			if (!gameOpts->menus.bSmartFileSearch || (nPatternLen < sizeof (szPattern) - 1)) {
				int nStart,
					 ascii = KeyToASCII (key);
				if ((key == KEY_BACKSP) || (ascii < 255)) {
					int cc, bFound = 0;
					if (!gameOpts->menus.bSmartFileSearch) {
						nStart = cItem;
						cc = cItem + 1;
						if (cc < 0)  
							cc = 0;
						else if (cc >= nItems)  
							cc = 0;
						}
					else {
						nStart = 0;
						cc = 0;
						if (key != KEY_BACKSP) {
							szPattern [nPatternLen++] = tolower (ascii);
							szPattern [nPatternLen] = '\0';
							}
						}
					do {
						pszFn = items [cc];
						if (items [cc][0] == '[')
							if (((items [cc][1] == '1') || (items [cc][1] == '2')) && (items [cc][2] == ']'))
								pszFn += 4;
							else
								pszFn++;
						strlwr (pszFn);
						if (gameOpts->menus.bSmartFileSearch ? strstr (pszFn, szPattern) == pszFn : *pszFn == tolower (ascii)) {
							cItem = cc;
							bFound = 1;
							break;
							}
						cc++;
						cc %= nItems;
					} while (cc != nStart);
				if (gameOpts->menus.bSmartFileSearch && !bFound && (nPatternLen > 0))
					szPattern [--nPatternLen] = '\0';
				}
			}
		}
		if (done) break;

		if (cItem<0)
			cItem=nItems-1;
		else if (cItem>=nItems)
			cItem = 0;
		if (cItem< first_item)
			first_item = cItem;
		else if (cItem>= (first_item+nItemsOnScreen))
			first_item = cItem-nItemsOnScreen+1;
		if (nItems <= nItemsOnScreen)
			 first_item = 0;
		if (first_item>nItems-nItemsOnScreen)
			first_item = nItems-nItemsOnScreen;
		if (first_item < 0) 
			first_item = 0;

#ifdef WINDOWS
		if (nItems>first_item+nItemsOnScreen)
			show_down_arrow=1;
		else 
			show_down_arrow=0;
		if (first_item>0)
			show_up_arrow=1;
		else	
			show_up_arrow=0;
#endif


#ifdef NEWMENU_MOUSE
		WIN (Sleep (100));
		if (nMouseState) {
			int w, h, aw;

			mouse_get_pos (&mx, &my);
			for (i=first_item; i<first_item+nItemsOnScreen; i++)	{
				if (i > nItems)
					break;
				GrGetStringSize (items [i], &w, &h, &aw);
				x1 = wx;
				x2 = wx + width;
				y1 = (i-first_item)* (grd_curfont->ft_h+2)+wy;
				y2 = y1+h+1;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
					//if (i == cItem) {
					//	break;
					//}
					//bDblClick= 0;
					cItem = i;
					done = 1;
					break;
				}
			}
		}

		//no double-click stuff for listbox
		//@@if (!nMouseState && nOldMouseState) {
		//@@	int w, h, aw;
		//@@
		//@@	GrGetStringSize (items [cItem], &w, &h, &aw);
		//@@	mouse_get_pos (&mx, &my);
		//@@	x1 = wx;
		//@@	x2 = wx + width;
		//@@	y1 = (cItem-first_item)* (grd_curfont->ft_h+2)+wy;
		//@@	y2 = y1+h+1;
		//@@	if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
		//@@		if (bDblClick) done = 1;
		//@@	}
		//@@}

		//check for close box clicked
		if (!nMouseState && nOldMouseState) {
			mouse_get_pos (&mx, &my);
			x1 = close_x + CLOSE_X + 2;
			x2 = x1 + CLOSE_SIZE - 2;
			y1 = close_y + CLOSE_Y + 2;
			y2 = y1 + CLOSE_SIZE - 2;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				cItem = -1;
				done = 1;
			}

		   #ifdef WINDOWS
			x1 = wx-LHX (10);
			x2 = x1 + LHX (10);
			y1 = wy;
			y2 = wy+LHY (7);
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2)) && show_up_arrow) 
				simukey = -1;
			y1 = total_height-LHY (7);
			y2 = total_height;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2)) && show_down_arrow) 
				simukey = 1;
		   #endif

		 	
		}
#endif

		if ((ofirst_item != first_item) || redraw || gameOpts->menus.nStyle) {
			if (gameOpts->menus.nStyle) 
				NMDrawBackground (&bg, wx-border_size, wy-title_height-border_size, wx+width+border_size-1, wy+height+border_size-1,1);
			else
				newmenu_hide_cursor ();
			WIN (DDGRLOCK (dd_grd_curcanv));
			if (gameOpts->menus.nStyle) {
				grdCurCanv->cv_font = NORMAL_FONT;
				GrString (0x8000, wy - title_height, title);
				}

			GrSetColorRGB (0, 0, 0, 255);
			for (i=first_item; i<first_item+nItemsOnScreen; i++)	{
				int w, h, aw, y;
				y = (i-first_item)* (grd_curfont->ft_h+2)+wy;
				if (i >= nItems)	{
					GrSetColorRGB (0, 0, 0, 255);
					GrRect (wx, y-1, wx+width-1, y+grd_curfont->ft_h + 1);
				} else {
					if (i == cItem)	
						grdCurCanv->cv_font = SELECTED_FONT;
					else	
						grdCurCanv->cv_font = NORMAL_FONT;
					GrGetStringSize (items [i], &w, &h, &aw);
					GrRect (wx, y-1, wx+width-1, y+h+1);
					GrString (wx+5, y, items [i]);
				}
			}		

				
			// If Win95 port, draw up/down arrows on left tSide of menu
#ifdef WINDOWS   
				grdCurCanv->cv_font = NORMAL_FONT;
			if (show_up_arrow)
	 	  		GrString (wx-LHX (10), wy , UP_ARROW_MARKER);
			else
			{
				bNoDarkening=1;
				NMDrawBackground (&bg, wx-LHX (10), wy, wx-2, wy+LHY (7));
				bNoDarkening=0;
			}

			if (show_down_arrow)
	     		GrString (wx-LHX (10), wy+total_height-LHY (7) , DOWN_ARROW_MARKER);
			else
			{
				bNoDarkening=1;
				NMDrawBackground (&bg, wx-LHX (10), wy+total_height-LHY (7), wx-2, wy+total_height);
				bNoDarkening=0;
			}

#endif


			WIN (DDGRUNLOCK (dd_grd_curcanv));
			newmenu_show_cursor ();
			GrUpdate (0);
		} else if (cItem != ocitem)	{
			int w, h, aw, y;

			if (!gameOpts->menus.nStyle) 
				newmenu_hide_cursor ();

			WIN (DDGRLOCK (dd_grd_curcanv));

			i = ocitem;
			if ((i>=0) &&(i<nItems))	{
				y = (i-first_item)* (grd_curfont->ft_h+2)+wy;
				if (i == cItem)	
					grdCurCanv->cv_font = SELECTED_FONT;
				else	
					grdCurCanv->cv_font = NORMAL_FONT;
				GrGetStringSize (items [i], &w, &h, &aw);
				GrRect (wx, y-1, wx+width-1, y+h+1);
				GrString (wx+5, y, items [i]);

			}
			i = cItem;
			if ((i>=0) &&(i<nItems))	{
				y = (i-first_item)* (grd_curfont->ft_h+2)+wy;
				if (i == cItem)	
					grdCurCanv->cv_font = SELECTED_FONT;
				else	
					grdCurCanv->cv_font = NORMAL_FONT;
				GrGetStringSize (items [i], &w, &h, &aw);
				GrRect (wx, y-1, wx+width-1, y+h);
				GrString (wx+5, y, items [i]);
			}
			WIN (DDGRUNLOCK (dd_grd_curcanv));

			newmenu_show_cursor ();
			GrUpdate (0);
		}
	}

	keyd_repeat = old_keyd_repeat;

	if (gameOpts->menus.nStyle) {
		NMRemoveBackground (&bg);
#if 1
		if (bg.menu_canvas) {
			WINDOS (DDGrFreeSubCanvas (bg.menu_canvas), 
					  GrFreeSubCanvas (bg.menu_canvas));
			WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (NULL));			
			bg.menu_canvas = NULL;
			}
#endif
		}
	else {
		newmenu_hide_cursor ();
		WIN (DDGRLOCK (dd_grd_curcanv));
		GrBmBitBlt (total_width, total_height, wx-border_size, wy-title_height-border_size, 0, 0, bg.background, &grdCurCanv->cv_bitmap);
		WIN (DDGRUNLOCK (dd_grd_curcanv)); 	
		if (bg.background != &VR_offscreen_buffer->cv_bitmap)
			GrFreeBitmap (bg.background);
		GrUpdate (0);
		}


#if 0
	WINDOS (
		dd_gr_blt_notrans (dd_VR_offscreen_buffer, 
				0, 0, _DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh, 
				dd_grd_curcanv, 
				0, 0, _DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh), 
		GrBmBitBlt (grdCurCanv->cv_w, grdCurCanv->cv_h, 0, 0, 0, 0, &(VR_offscreen_buffer->cv_bitmap), &(grdCurCanv->cv_bitmap))
	);
#endif

	WIN (DDGRRESTORE);

	WIN (mouse_set_mode (1));				//re-enable centering mode

	SDL_EnableKeyRepeat(0, 0);
	return cItem;
}

//------------------------------------------------------------------------------

static char FilenameText [MAX_FILES] [ (FILENAME_LEN+1)];

int ExecMenuFileList (char * title, char * filespec, char * filename)
{
	int i, NumFiles;
	char * Filenames [MAX_FILES];
	FFS  ffs;

	NumFiles = 0;
	if (!FFF (filespec, &ffs, 0))	{
		do	{
			if (NumFiles<MAX_FILES)	{
            strncpy (FilenameText [NumFiles], ffs.name, FILENAME_LEN);
				Filenames [NumFiles] = FilenameText [NumFiles];
				NumFiles++;
			} else {
				break;
			}
		} while (!FFN (&ffs, 0));
		FFC (&ffs);
	}

	i = ExecMenuListBox (title, NumFiles, Filenames, 1, NULL);
	if (i > -1)	{
		strcpy (filename, Filenames [i]);
		return 1;
	} 
	return 0;
}

//------------------------------------------------------------------------------
//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
int _CDECL_ NMMsgBoxFixedFont (char *title, int nChoices, ...)
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text [MESSAGEBOX_TEXT_SIZE];
	newmenu_item nmMsgItems [5];

	va_start (args, nChoices);

	Assert (nChoices <= 5);

	memset (nmMsgItems, 0, sizeof (nmMsgItems));
	for (i=0; i<nChoices; i++)	{
		s = va_arg (args, char *);
		nmMsgItems [i].nType = NM_TYPE_MENU; nmMsgItems [i].text = s;
	}
	format = va_arg (args, char *);
	//sprintf (	  nm_text, ""); // adb: ?
	vsprintf (nm_text, format, args);
	va_end (args);

	Assert (strlen (nm_text) < MESSAGEBOX_TEXT_SIZE);

   return ExecMenuFixedFont (title, nm_text, nChoices, nmMsgItems, NULL, 0, NULL, -1, -1);
}
//end this section addition - Victor Rachels

//------------------------------------------------------------------------------

#ifdef NETWORK
extern netgame_info activeNetGames [];

void ShowExtraNetGameInfo (int choice)
 {
	newmenu_item m [5];
   char mtext [5] [50];
	int i, nInMenu, opt = 0;

if (choice >= networkData.nActiveGames)
	return;
memset (m, 0, sizeof (m));
for (i = 0; i < 5; i++) {
	m [i].text = (char *) (mtext + i);
	m [i].nType = NM_TYPE_TEXT;		
	}
sprintf (mtext [opt], TXT_NGI_GAME, activeNetGames [choice].game_name); 
opt++;
sprintf (mtext [opt], TXT_NGI_MISSION, activeNetGames [choice].mission_title); 
opt++;
sprintf (mtext [opt], TXT_NGI_LEVEL, activeNetGames [choice].levelnum); 
opt++;
sprintf (mtext [opt], TXT_NGI_SKILL, MENU_DIFFICULTY_TEXT (activeNetGames [choice].difficulty)); 
opt++;
bAlreadyShowingInfo = 1;	
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
ExecMenutiny2 (NULL, TXT_NETGAME_INFO, opt, m, NULL);
gameStates.menus.nInMenu = nInMenu;
bAlreadyShowingInfo = 0;	
 }

#endif // NETWORK

//------------------------------------------------------------------------------
/* Spiffy word wrap string formatting function */

void NMWrapText (char *dbuf, char *sbuf, int line_length)
{
	int col;
	char *wordptr;
	char *tbuf;

	tbuf = (char *)d_malloc ((int) strlen (sbuf)+1);
	strcpy (tbuf, sbuf);

	wordptr = strtok (tbuf, " ");
	if (!wordptr) return;
	col = 0;
	dbuf [0] = 0;

	while (wordptr)
	{
		col = col + (int) strlen (wordptr)+1;
		if (col >=line_length) {
			col = 0;
			sprintf (dbuf, "%s\n%s ", dbuf, wordptr);
		}
		else {
			sprintf (dbuf, "%s%s ", dbuf, wordptr);
		}
		wordptr = strtok (NULL, " ");
	}

	d_free (tbuf);
}

//------------------------------------------------------------------------------

void NMProgressBar (char *szCaption, int nCurProgress, int nMaxProgress, 
						  void (*doProgress) (int nItems, newmenu_item *items, int *last_key, int cItem))
{
	newmenu_item	m [3];
	int				i, nInMenu;

memset (m, 0, sizeof (m));
ADD_GAUGE (0, "                    ", 0, nMaxProgress); 
m [0].centered = 1;
m [0].value = nCurProgress;
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
gameData.app.bGamePaused = 1;
do {
	i = ExecMenu2 (NULL, szCaption, 1, m, doProgress, 0, NULL);
	} while (i >= 0);
gameData.app.bGamePaused = 0;
gameStates.menus.nInMenu = nInMenu;
}

//------------------------------------------------------------------------------
