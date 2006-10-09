/* $Id: gauges.c,v 1.10 2003/10/11 09:28:38 btb Exp $ */
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
 * Inferno gauge drivers
 *
 * Old Log:
 * Revision 1.15  1995/10/31  10:22:37  allender
 * shareware stuff
 *
 * Revision 1.14  1995/10/26  14:11:05  allender
 * do copy of weapon boxes in cockpit correctly
 *
 * Revision 1.13  1995/10/21  22:54:56  allender
 * fixed up player names on hud
 *
 * Revision 1.12  1995/10/12  17:39:27  allender
 * fixed status bar lives display
 *
 * Revision 1.11  1995/09/22  15:21:46  allender
 * fixed hud problems (reticle and kill lists) for
 * non pixel doubled mode
 *
 * Revision 1.10  1995/09/13  11:38:47  allender
 * show KB left in heap instead of piggy cache
 *
 * Revision 1.9  1995/09/04  15:52:28  allender
 * fix vulcan ammo count to update without overwritting itself
 *
 * Revision 1.8  1995/08/31  14:11:20  allender
 * worked on hud kill list for non pixel doubled mode
 *
 * Revision 1.7  1995/08/24  16:05:05  allender
 * more gauge placement -- still not done!
 *
 * Revision 1.6  1995/08/18  15:44:56  allender
 * put in PC gauges for keys, lives, and reticle when pixel doubling
 *
 * Revision 1.5  1995/08/18  10:24:47  allender
 * added proper support for cockpit mode -- still needs
 *
 * Revision 1.4  1995/07/26  16:56:34  allender
 * more gauge stuff for status bar.  still problem
 * with ship
 *
 * Revision 1.3  1995/07/17  08:55:57  allender
 * fix up for large status bar.  Still needs some work though
 *
 * Revision 1.2  1995/06/20  09:54:29  allender
 * stopgap measure to get status bar "working" until real mac
 * status bar gets added
 *
 * Revision 1.1  1995/05/16  15:26:05  allender
 * Initial revision
 *
 * Revision 2.7  1995/12/19  16:18:33  john
 * Made weapon info align with canvas width, not 315.
 *
 * Revision 2.6  1995/03/21  14:39:25  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.5  1995/03/14  12:31:25  john
 * Prevent negative shields from printing.
 *
 * Revision 2.4  1995/03/10  12:57:58  allender
 * move rear view text up four pixels up when playing back demo
 *
 * Revision 2.3  1995/03/09  11:47:51  john
 * Added HUD for VR helmets.
 *
 * Revision 2.2  1995/03/06  15:23:26  john
 * New screen techniques.
 *
 * Revision 2.1  1995/02/27  13:13:45  john
 * Removed floating point.
 *
 * Revision 2.0  1995/02/27  11:29:06  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.203  1995/02/11  01:56:45  mike
 * move up weapons text on fullscreen hud, missiles was offscreen.
 *
 * Revision 1.202  1995/02/09  13:23:34  rob
 * Added reticle names in demo playback.
 *
 * Revision 1.201  1995/02/08  19:20:46  rob
 * Show cloaked teammates on H
 * UD.  Get rid of show ID's in anarchy option.
 *
 * Revision 1.200  1995/02/07  21:09:00  mike
 * add flashing to invulnerability and cloak on fullscreen.
 *
 * Revision 1.199  1995/02/02  21:55:57  matt
 * Added new colored key icons for fullscreen
 *
 * Revision 1.198  1995/01/30  17:17:07  rob
 * Fixed teammate names on hud.
 *
 * Revision 1.197  1995/01/28  17:40:49  mike
 * fix gauge fontcolor.
 *
 * Revision 1.196  1995/01/27  17:03:14  mike
 * fix placement of weapon info in multiplayer fullscreen, as per AP request.
 *
 * Revision 1.195  1995/01/27  11:51:23  rob
 * Put deaths tally into cooperative mode
 *
 * Revision 1.194  1995/01/27  11:43:24  adam
 * fiddled with key display
 *
 * Revision 1.193  1995/01/25  23:38:35  mike
 * fix keys on fullscreen.
 *
 * Revision 1.192  1995/01/24  22:03:28  mike
 * Lotsa hud stuff, put a lot of messages up.
 *
 * Revision 1.191  1995/01/23  16:47:21  rob
 * Fixed problem with playing extra life noise in coop.
 *
 * Revision 1.190  1995/01/22  16:00:46  mike
 * remove unneeded string.
 *
 * Revision 1.189  1995/01/22  15:58:22  mike
 * localization
 *
 * Revision 1.188  1995/01/20  17:19:45  rob
 * Fixing colors of hud kill list players.
 *
 * Revision 1.187  1995/01/20  09:19:18  allender
 * record player flags when in CM_FULL_SCREEN
 *
 * Revision 1.186  1995/01/19  16:29:09  allender
 * made demo recording of weapon change be in this file for shareware only
 *
 * Revision 1.185  1995/01/19  15:00:33  allender
 * code to record shield, energy, and ammo in fullscreen
 *
 * Revision 1.184  1995/01/19  13:43:13  matt
 * Fixed "cheater" message on HUD
 *
 * Revision 1.183  1995/01/18  16:11:58  mike
 * Don't show added scores of 0.
 *
 * Revision 1.182  1995/01/17  17:42:39  allender
 * do ammo counts in demo recording
 *
 * Revision 1.181  1995/01/16  17:26:25  rob
 * Fixed problem with coloration of team kill list.
 *
 * Revision 1.180  1995/01/16  17:22:39  john
 * Made so that KB and framerate don't collide.
 *
 * Revision 1.179  1995/01/16  14:58:31  matt
 * Changed score_added display to print "Cheater!" when cheats enabled
 *
 * Revision 1.178  1995/01/15  19:42:07  matt
 * Ripped out hostage faces for registered version
 *
 * Revision 1.177  1995/01/15  19:25:07  mike
 * show vulcan ammo and secondary ammo in fullscreen view.
 *
 * Revision 1.176  1995/01/15  13:16:12  john
 * Made so that paging always happens, lowmem just loads less.
 * Also, make KB load print to hud.
 *
 * Revision 1.175  1995/01/14  19:17:32  john
 * First version of piggy paging.
 *
 * Revision 1.174  1995/01/05  21:25:23  rob
 * Re-did some changes lost due to RCS weirdness.
 *
 * Revision 1.173  1995/01/05  12:22:34  rob
 * Don't show player names for cloaked players.
 *
 * Revision 1.172  1995/01/04  17:14:50  allender
 * make InitGauges work properly on demo playback
 *
 * Revision 1.171  1995/01/04  15:04:42  allender
 * new demo calls for registered version
 *
 * Revision 1.167  1995/01/03  13:03:57  allender
 * pass score points instead of total points.   Added ifdef for
 * MultiSendScore
 *
 * Revision 1.166  1995/01/03  11:45:02  allender
 * add hook to record player score
 *
 * Revision 1.165  1995/01/03  11:25:19  allender
 * remove newdemo stuff around score display
 *
 * Revision 1.163  1995/01/02  21:03:53  rob
 * Fixing up the hud-score-list for coop games.
 *
 * Revision 1.162  1994/12/31  20:54:40  rob
 * Added coop mode HUD score list.
 * Added more generic system for player names on HUD.
 *
 * Revision 1.161  1994/12/30  20:13:01  rob
 * Ifdef reticle names on shareware.
 * Added robot reticle naming.
 *
 * Revision 1.160  1994/12/29  17:53:51  mike
 * move up energy/shield in fullscreen to get out of way of kill list.
	 *
 * Revision 1.159  1994/12/29  16:44:05  mike
 * add energy and shield showing.
 *
 * Revision 1.158  1994/12/28  16:34:29  mike
 * make warning beep go away on gameStates.app.bPlayerIsDead.
 *
 * Revision 1.157  1994/12/28  10:00:43  allender
 * change in InitGauges to for multiplayer demo playbacks
 *
 * Revision 1.156  1994/12/27  11:06:46  allender
 * removed some previous code to for demo playback stuff
 *
 * Revision 1.155  1994/12/23  14:23:06  john
 * Added floating reticle for VR helments.
 *
 * Revision 1.154  1994/12/21  12:56:41  allender
 * on multiplayer demo playback, show kills and deaths
 *
 * Revision 1.153  1994/12/19  20:28:42  rob
 * Get rid of kill list in coop games.
 *
 * Revision 1.152  1994/12/14  18:06:44  matt
 * Removed compile warnings
 *
 * Revision 1.151  1994/12/14  15:21:28  rob
 * Made gauges align in status_bar net game.
 *
 * Revision 1.150  1994/12/12  17:20:33  matt
 * Don't get bonus points when cheating
 *
 * Revision 1.149  1994/12/12  16:47:00  matt
 * When cheating, get no score.  Change level cheat to prompt for and
 * jump to new level.
 *
 * Revision 1.148  1994/12/12  12:05:45  rob
 * Grey out players who are disconnected.
 *
 * Revision 1.147  1994/12/09  16:19:48  yuan
 * kill matrix stuff.
 *
 * Revision 1.146  1994/12/09  16:12:34  rob
 * Fixed up the status bar kills gauges for net play.
 *
 * Revision 1.145  1994/12/09  01:55:34  rob
 * Added kills list to HUD/status bar.
 * Added something for Mark.
 *
 * Revision 1.144  1994/12/08  21:03:30  allender
 * pass old player flags to record_player_flags
 *
 * Revision 1.143  1994/12/07  22:49:33  mike
 * no homing missile warning during endlevel sequence.
 *
 * Revision 1.142  1994/12/06  13:55:31  matt
 * Use new rounding func, f2ir()
 *
 * Revision 1.141  1994/12/03  19:03:37  matt
 * Fixed vulcan ammo HUD message
 *
 * Revision 1.140  1994/12/03  18:43:18  matt
 * Fixed (hopefully) claok gauge
 *
 * Revision 1.139  1994/12/03  14:26:21  yuan
 * Fixed dumb bug
 *
 * Revision 1.138  1994/12/03  14:17:30  yuan
 * Localization 320
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "hudmsg.h"

#include "pa_enabl.h"                   //$$POLY_ACC
#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "error.h"

#include "menu.h"			// For the font.
#include "mono.h"
#include "collide.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"
#include "bm.h"
#include "text.h"
#include "powerup.h"
#include "sounds.h"
#ifdef NETWORK
#include "multi.h"
#include "network.h"
#endif
#include "endlevel.h"
#include "cntrlcen.h"
#include "controls.h"
#include "weapon.h"
#include "globvars.h"

#include "wall.h"
#include "text.h"
#include "render.h"
#include "piggy.h"
#include "laser.h"
#include "grdef.h"
#include "gr.h"
#include "gamepal.h"
#include "kconfig.h"
#include "object.h"

#ifdef OGL
#include "ogl_init.h"
#endif

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

void DrawAmmoInfo(int x,int y,int ammo_count,int primary);
extern void DrawGuidedCrosshair(void);
extern void DoAutomap( int key_code, int bRadar );

double cmScaleX, cmScaleY;

bitmap_index Gauges[MAX_GAUGE_BMS];   // Array of all gauge bitmaps.
bitmap_index Gauges_hires[MAX_GAUGE_BMS];   // hires gauges

grs_canvas *Canv_LeftEnergyGauge;
grs_canvas *Canv_AfterburnerGauge;
grs_canvas *Canv_SBEnergyGauge;
grs_canvas *Canv_SBAfterburnerGauge;
grs_canvas *Canv_RightEnergyGauge;
grs_canvas *Canv_NumericalGauge;

#define NUM_INV_ITEMS			8
#define INV_ITEM_QUADLASERS	5
#define INV_ITEM_CLOAK			6
#define INV_ITEM_INVUL			7

grs_bitmap	*bmpInventory = NULL;
grs_bitmap	bmInvItems [NUM_INV_ITEMS];
int bHaveInvBms = -1;

//Flags for gauges/hud stuff
//bitmap numbers for gauges

#define GAUGE_SHIELDS			0		//0..9, in decreasing order (100%,90%...0%)

#define GAUGE_INVULNERABLE		10		//10..19
#define N_INVULNERABLE_FRAMES	10

#define GAUGE_AFTERBURNER   	20
#define GAUGE_ENERGY_LEFT		21
#define GAUGE_ENERGY_RIGHT		22
#define GAUGE_NUMERICAL			23

#define GAUGE_BLUE_KEY			24
#define GAUGE_GOLD_KEY			25
#define GAUGE_RED_KEY			26
#define GAUGE_BLUE_KEY_OFF		27
#define GAUGE_GOLD_KEY_OFF		28
#define GAUGE_RED_KEY_OFF		29

#define SB_GAUGE_BLUE_KEY		30
#define SB_GAUGE_GOLD_KEY		31
#define SB_GAUGE_RED_KEY		32
#define SB_GAUGE_BLUE_KEY_OFF	33
#define SB_GAUGE_GOLD_KEY_OFF	34
#define SB_GAUGE_RED_KEY_OFF	35

#define SB_GAUGE_ENERGY			36

#define GAUGE_LIVES				37	

#define GAUGE_SHIPS				38
#define GAUGE_SHIPS_LAST		45

#define RETICLE_CROSS			46
#define RETICLE_PRIMARY			48
#define RETICLE_SECONDARY		51
#define RETICLE_LAST				55

#define GAUGE_HOMING_WARNING_ON	56
#define GAUGE_HOMING_WARNING_OFF	57

#define SML_RETICLE_CROSS		58
#define SML_RETICLE_PRIMARY	60
#define SML_RETICLE_SECONDARY	63
#define SML_RETICLE_LAST		67

#define KEY_ICON_BLUE			68
#define KEY_ICON_YELLOW			69
#define KEY_ICON_RED				70

#define SB_GAUGE_AFTERBURNER	71

#define FLAG_ICON_RED			72
#define FLAG_ICON_BLUE			73


/* Use static inline function under GCC to avoid CR/LF issues */
#ifdef __GNUC__
#define PAGE_IN_GAUGE(x) _page_in_gauge(x)
static inline void _page_in_gauge(int x)
{
PIGGY_PAGE_IN(gameStates.render.fonts.bHires ? Gauges_hires[x] : Gauges[x], 0);
}

#else
#define PAGE_IN_GAUGE(x)	PIGGY_PAGE_IN(gameStates.render.fonts.bHires ? Gauges_hires[x] : Gauges[x], 0);	

#endif

#define GET_GAUGE_INDEX(x)	(gameStates.render.fonts.bHires?Gauges_hires[x].index:Gauges[x].index)

//change MAX_GAUGE_BMS when adding gauges

//Coordinats for gauges

#define GAUGE_BLUE_KEY_X_L		272
#define GAUGE_BLUE_KEY_Y_L		152
#define GAUGE_BLUE_KEY_X_H		535
#define GAUGE_BLUE_KEY_Y_H		374
#define GAUGE_BLUE_KEY_X		(gameStates.video.nDisplayMode?GAUGE_BLUE_KEY_X_H:GAUGE_BLUE_KEY_X_L)
#define GAUGE_BLUE_KEY_Y		(gameStates.video.nDisplayMode?GAUGE_BLUE_KEY_Y_H:GAUGE_BLUE_KEY_Y_L)

#define GAUGE_GOLD_KEY_X_L		273
#define GAUGE_GOLD_KEY_Y_L		162
#define GAUGE_GOLD_KEY_X_H		537
#define GAUGE_GOLD_KEY_Y_H		395
#define GAUGE_GOLD_KEY_X		(gameStates.video.nDisplayMode?GAUGE_GOLD_KEY_X_H:GAUGE_GOLD_KEY_X_L)
#define GAUGE_GOLD_KEY_Y		(gameStates.video.nDisplayMode?GAUGE_GOLD_KEY_Y_H:GAUGE_GOLD_KEY_Y_L)

#define GAUGE_RED_KEY_X_L		274
#define GAUGE_RED_KEY_Y_L		172
#define GAUGE_RED_KEY_X_H		539
#define GAUGE_RED_KEY_Y_H		416
#define GAUGE_RED_KEY_X			(gameStates.video.nDisplayMode?GAUGE_RED_KEY_X_H:GAUGE_RED_KEY_X_L)
#define GAUGE_RED_KEY_Y			(gameStates.video.nDisplayMode?GAUGE_RED_KEY_Y_H:GAUGE_RED_KEY_Y_L)

// status bar keys

#define SB_GAUGE_KEYS_X_L	   11
#define SB_GAUGE_KEYS_X_H		26
#define SB_GAUGE_KEYS_X			(gameStates.video.nDisplayMode?SB_GAUGE_KEYS_X_H:SB_GAUGE_KEYS_X_L)

#define SB_GAUGE_BLUE_KEY_Y_L		153
#define SB_GAUGE_GOLD_KEY_Y_L		169
#define SB_GAUGE_RED_KEY_Y_L		185

#define SB_GAUGE_BLUE_KEY_Y_H		390
#define SB_GAUGE_GOLD_KEY_Y_H		422
#define SB_GAUGE_RED_KEY_Y_H		454

#define SB_GAUGE_BLUE_KEY_Y	(gameStates.video.nDisplayMode?SB_GAUGE_BLUE_KEY_Y_H:SB_GAUGE_BLUE_KEY_Y_L)
#define SB_GAUGE_GOLD_KEY_Y	(gameStates.video.nDisplayMode?SB_GAUGE_GOLD_KEY_Y_H:SB_GAUGE_GOLD_KEY_Y_L)
#define SB_GAUGE_RED_KEY_Y		(gameStates.video.nDisplayMode?SB_GAUGE_RED_KEY_Y_H:SB_GAUGE_RED_KEY_Y_L)

// cockpit enery gauges

#define LEFT_ENERGY_GAUGE_X_L 	70
#define LEFT_ENERGY_GAUGE_Y_L 	131
#define LEFT_ENERGY_GAUGE_W_L 	64
#define LEFT_ENERGY_GAUGE_H_L 	8

#define LEFT_ENERGY_GAUGE_X_H 	137
#define LEFT_ENERGY_GAUGE_Y_H 	314
#define LEFT_ENERGY_GAUGE_W_H 	133
#define LEFT_ENERGY_GAUGE_H_H 	21

#define LEFT_ENERGY_GAUGE_X 	(gameStates.video.nDisplayMode?LEFT_ENERGY_GAUGE_X_H:LEFT_ENERGY_GAUGE_X_L)
#define LEFT_ENERGY_GAUGE_Y 	(gameStates.video.nDisplayMode?LEFT_ENERGY_GAUGE_Y_H:LEFT_ENERGY_GAUGE_Y_L)
#define LEFT_ENERGY_GAUGE_W 	(gameStates.video.nDisplayMode?LEFT_ENERGY_GAUGE_W_H:LEFT_ENERGY_GAUGE_W_L)
#define LEFT_ENERGY_GAUGE_H 	(gameStates.video.nDisplayMode?LEFT_ENERGY_GAUGE_H_H:LEFT_ENERGY_GAUGE_H_L)

#define RIGHT_ENERGY_GAUGE_X 	(gameStates.video.nDisplayMode?380:190)
#define RIGHT_ENERGY_GAUGE_Y 	(gameStates.video.nDisplayMode?314:131)
#define RIGHT_ENERGY_GAUGE_W 	(gameStates.video.nDisplayMode?133:64)
#define RIGHT_ENERGY_GAUGE_H 	(gameStates.video.nDisplayMode?21:8)

// cockpit afterburner gauge

#define AFTERBURNER_GAUGE_X_L	45-1
#define AFTERBURNER_GAUGE_Y_L	158
#define AFTERBURNER_GAUGE_W_L	12
#define AFTERBURNER_GAUGE_H_L	32

#define AFTERBURNER_GAUGE_X_H	88
#define AFTERBURNER_GAUGE_Y_H	377
#define AFTERBURNER_GAUGE_W_H	21
#define AFTERBURNER_GAUGE_H_H	65

#define AFTERBURNER_GAUGE_X	(gameStates.video.nDisplayMode?AFTERBURNER_GAUGE_X_H:AFTERBURNER_GAUGE_X_L)
#define AFTERBURNER_GAUGE_Y	(gameStates.video.nDisplayMode?AFTERBURNER_GAUGE_Y_H:AFTERBURNER_GAUGE_Y_L)
#define AFTERBURNER_GAUGE_W	(gameStates.video.nDisplayMode?AFTERBURNER_GAUGE_W_H:AFTERBURNER_GAUGE_W_L)
#define AFTERBURNER_GAUGE_H	(gameStates.video.nDisplayMode?AFTERBURNER_GAUGE_H_H:AFTERBURNER_GAUGE_H_L)

// sb afterburner gauge

#define SB_AFTERBURNER_GAUGE_X 		(gameStates.video.nDisplayMode?196:98)
#define SB_AFTERBURNER_GAUGE_Y 		(gameStates.video.nDisplayMode?446:184)
#define SB_AFTERBURNER_GAUGE_W 		(gameStates.video.nDisplayMode?33:16)
#define SB_AFTERBURNER_GAUGE_H 		(gameStates.video.nDisplayMode?29:13)

// sb energy gauge

#define SB_ENERGY_GAUGE_X 		(gameStates.video.nDisplayMode?196:98)
#define SB_ENERGY_GAUGE_Y 		(gameStates.video.nDisplayMode?382:(155-2))
#define SB_ENERGY_GAUGE_W 		(gameStates.video.nDisplayMode?32:16)
#define SB_ENERGY_GAUGE_H 		((gameStates.video.nDisplayMode?60:29) + (gameStates.app.bD1Mission ? SB_AFTERBURNER_GAUGE_H : 0))

#define SB_ENERGY_NUM_X 		(SB_ENERGY_GAUGE_X+(gameStates.video.nDisplayMode?4:2))
#define SB_ENERGY_NUM_Y 		(gameStates.video.nDisplayMode?457:175)

#define SHIELD_GAUGE_X 			(gameStates.video.nDisplayMode?292:146)
#define SHIELD_GAUGE_Y			(gameStates.video.nDisplayMode?374:155)
#define SHIELD_GAUGE_W 			(gameStates.video.nDisplayMode?70:35)
#define SHIELD_GAUGE_H			(gameStates.video.nDisplayMode?77:32)

#define SHIP_GAUGE_X 			(SHIELD_GAUGE_X+(gameStates.video.nDisplayMode?11:5))
#define SHIP_GAUGE_Y				(SHIELD_GAUGE_Y+(gameStates.video.nDisplayMode?10:5))

#define SB_SHIELD_GAUGE_X 		(gameStates.video.nDisplayMode?247:123)		//139
#define SB_SHIELD_GAUGE_Y 		(gameStates.video.nDisplayMode?395:163)

#define SB_SHIP_GAUGE_X 		(SB_SHIELD_GAUGE_X+(gameStates.video.nDisplayMode?11:5))
#define SB_SHIP_GAUGE_Y 		(SB_SHIELD_GAUGE_Y+(gameStates.video.nDisplayMode?10:5))

#define SB_SHIELD_NUM_X 		(SB_SHIELD_GAUGE_X+(gameStates.video.nDisplayMode?21:12))	//151
#define SB_SHIELD_NUM_Y 		(SB_SHIELD_GAUGE_Y-(gameStates.video.nDisplayMode?16:8))			//156 -- MWA used to be hard coded to 156

#define NUMERICAL_GAUGE_X		(gameStates.video.nDisplayMode?308:154)
#define NUMERICAL_GAUGE_Y		(gameStates.video.nDisplayMode?316:130)
#define NUMERICAL_GAUGE_W		(gameStates.video.nDisplayMode?38:19)
#define NUMERICAL_GAUGE_H		(gameStates.video.nDisplayMode?55:22)

#define PRIMARY_W_PIC_X			(gameStates.video.nDisplayMode?(135-10):64)
#define PRIMARY_W_PIC_Y			(gameStates.video.nDisplayMode?370:154)
#define PRIMARY_W_TEXT_X		(gameStates.video.nDisplayMode?182:87)
#define PRIMARY_W_TEXT_Y		(gameStates.video.nDisplayMode?400:157)
#define PRIMARY_AMMO_X			(gameStates.video.nDisplayMode?186:(96-3))
#define PRIMARY_AMMO_Y			(gameStates.video.nDisplayMode?420:171)

#define SECONDARY_W_PIC_X		(gameStates.video.nDisplayMode?466:234)
#define SECONDARY_W_PIC_Y		(gameStates.video.nDisplayMode?374:154)
#define SECONDARY_W_TEXT_X		(gameStates.video.nDisplayMode?413:207)
#define SECONDARY_W_TEXT_Y		(gameStates.video.nDisplayMode?378:157)
#define SECONDARY_AMMO_X		(gameStates.video.nDisplayMode?428:213)
#define SECONDARY_AMMO_Y		(gameStates.video.nDisplayMode?407:171)

#define SB_LIVES_X				(gameStates.video.nDisplayMode?(550-10-3):266)
#define SB_LIVES_Y				(gameStates.video.nDisplayMode?450-3:185)
#define SB_LIVES_LABEL_X		(gameStates.video.nDisplayMode?475:237)
#define SB_LIVES_LABEL_Y		(SB_LIVES_Y+1)

#define SB_SCORE_RIGHT_L		301
#define SB_SCORE_RIGHT_H		(605+8)
#define SB_SCORE_RIGHT			(gameStates.video.nDisplayMode?SB_SCORE_RIGHT_H:SB_SCORE_RIGHT_L)

#define SB_SCORE_Y				(gameStates.video.nDisplayMode?398:158)
#define SB_SCORE_LABEL_X		(gameStates.video.nDisplayMode?475:237)

#define SB_SCORE_ADDED_RIGHT	(gameStates.video.nDisplayMode?SB_SCORE_RIGHT_H:SB_SCORE_RIGHT_L)
#define SB_SCORE_ADDED_Y		(gameStates.video.nDisplayMode?413:165)

#define HOMING_WARNING_X		(gameStates.video.nDisplayMode?14:7)
#define HOMING_WARNING_Y		(gameStates.video.nDisplayMode?415:171)

#define BOMB_COUNT_X			(gameStates.video.nDisplayMode?546:275)
#define BOMB_COUNT_Y			(gameStates.video.nDisplayMode?445:186)

#define SB_BOMB_COUNT_X			(gameStates.video.nDisplayMode?342:171)
#define SB_BOMB_COUNT_Y			(gameStates.video.nDisplayMode?458:191)

#ifdef WINDOWS
#define LHX(x)      (gameStates.video.nDisplayMode?2*(x):x)
#define LHY(y)      (gameStates.video.nDisplayMode?(24*(y))/10:y)
#else
#define LHX(x)      (gameStates.menus.bHires?2*(x):x)
#define LHY(y)      (gameStates.menus.bHires?(24*(y))/10:y)
#endif

static int score_display[2];
static fix score_time;

static int old_score[2]				= { -1, -1 };
static int old_energy[2]			= { -1, -1 };
static int old_shields[2]			= { -1, -1 };
static uint old_flags[2]			= { (uint) -1, (uint) -1 };
static int old_weapon[2][2]		= {{ -1, -1 },{-1,-1}};
static int old_ammo_count[2][2]	= {{ -1, -1 },{-1,-1}};
static int Old_Omega_charge[2]	= { -1, -1 };
static int old_laser_level[2]		= { -1, -1 };
static int old_cloak[2]				= { 0, 0 };
static int old_lives[2]				= { -1, -1 };
static fix old_afterburner[2]		= { -1, -1 };
static int old_bombcount[2]		= { 0, 0 };

static int invulnerable_frame = 0;

static int nCloakFadeState;		//0=steady, -1 fading out, 1 fading in 

#define WS_SET				0		//in correct state
#define WS_FADING_OUT	1
#define WS_FADING_IN		2

int weapon_box_user[2]={WBU_WEAPON,WBU_WEAPON};		//see WBU_ constants in gauges.h
int weapon_box_states[2];
fix weapon_box_fade_values[2];

#define FADE_SCALE	(2*i2f(GR_ACTUAL_FADE_LEVELS)/REARM_TIME)		// fade out and back in REARM_TIME, in fade levels per seconds (int)

typedef struct span {
	sbyte l,r;
} span;

//store delta x values from left of box
span weapon_window_left[] = {		//first span 67,151
		{8,51},
		{6,53},
		{5,54},
		{4-1,53+2},
		{4-1,53+3},
		{4-1,53+3},
		{4-2,53+3},
		{4-2,53+3},
		{3-1,53+3},
		{3-1,53+3},
		{3-1,53+3},
		{3-1,53+3},
		{3-1,53+3},
		{3-1,53+3},
		{3-1,53+3},
		{3-2,53+3},
		{2-1,53+3},
		{2-1,53+3},
		{2-1,53+3},
		{2-1,53+3},
		{2-1,53+3},
		{2-1,53+3},
		{2-1,53+3},
		{2-1,53+3},
		{1-1,53+3},
		{1-1,53+2},
		{1-1,53+2},
		{1-1,53+2},
		{1-1,53+2},
		{1-1,53+2},
		{1-1,53+2},
		{1-1,53+2},
		{0,53+2},
		{0,53+2},
		{0,53+2},
		{0,53+2},
		{0,52+3},
		{1-1,52+2},
		{2-2,51+3},
		{3-2,51+2},
		{4-2,50+2},
		{5-2,50},
		{5-2+2,50-2},
	};


//store delta x values from left of box
span weapon_window_right[] = {		//first span 207,154
		{208-202,255-202},
		{206-202,257-202},
		{205-202,258-202},
		{204-202,259-202},
		{203-202,260-202},
		{203-202,260-202},
		{203-202,260-202},
		{203-202,260-202},
		{203-202,260-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,261-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{203-202,262-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{204-202,263-202},
		{205-202,263-202},
		{206-202,262-202},
		{207-202,261-202},
		{208-202,260-202},
		{211-202,255-202},
	};

//store delta x values from left of box
span weapon_window_left_hires[] = {		//first span 67,154
	{20,110},
	{18,113},
	{16,114},
	{15,116},
	{14,117},
	{13,118},
	{12,119},
	{11,119},
	{10,120},
	{10,120},
	{9,121},
	{8,121},
	{8,121},
	{8,122},
	{7,122},
	{7,122},
	{7,122},
	{7,122},
	{7,122},
	{6,122},
	{6,122},
	{6,122},
	{6,122},
	{6,122},
	{6,122},
	{6,122},
	{6,122},
	{6,122},
	{6,122},
	{5,122},
	{5,122},
	{5,122},
	{5,122},
	{5,121},
	{5,121},
	{5,121},
	{5,121},
	{5,121},
	{5,121},
	{4,121},
	{4,121},
	{4,121},
	{4,121},
	{4,121},
	{4,121},
	{4,121},
	{4,121},
	{4,121},
	{4,121},
	{3,121},
	{3,121},
	{3,120},
	{3,120},
	{3,120},
	{3,120},
	{3,120},
	{3,120},
	{3,120},
	{3,120},
	{3,120},
	{2,120},
	{2,120},
	{2,120},
	{2,120},
	{2,120},
	{2,120},
	{2,120},
	{2,120},
	{2,120},
	{2,120},
	{2,120},
	{2,120},
	{1,120},
	{1,120},
	{1,119},
	{1,119},
	{1,119},
	{1,119},
	{1,119},
	{1,119},
	{1,119},
	{1,119},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,118},
	{0,118},
	{0,118},
	{0,117},
	{0,117},
	{0,117},
	{1,116},
	{1,116},
	{2,115},
	{2,114},
	{3,113},
	{4,112},
	{5,111},
	{5,110},
	{7,109},
	{9,107},
	{10,105},
	{12,102},
};


//store delta x values from left of box
span weapon_window_right_hires[] = {		//first span 207,154
	{12,105},
	{9,107},
	{8,109},
	{6,110},
	{5,111},
	{4,112},
	{3,113},
	{3,114},
	{2,115},
	{2,115},
	{1,116},
	{1,117},
	{1,117},
	{0,117},
	{0,118},
	{0,118},
	{0,118},
	{0,118},
	{0,118},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,119},
	{0,120},
	{0,120},
	{0,120},
	{0,120},
	{1,120},
	{1,120},
	{1,120},
	{1,120},
	{1,120},
	{1,120},
	{1,121},
	{1,121},
	{1,121},
	{1,121},
	{1,121},
	{1,121},
	{1,121},
	{1,121},
	{1,121},
	{1,121},
	{1,122},
	{1,122},
	{2,122},
	{2,122},
	{2,122},
	{2,122},
	{2,122},
	{2,122},
	{2,122},
	{2,122},
	{2,123},
	{2,123},
	{2,123},
	{2,123},
	{2,123},
	{2,123},
	{2,123},
	{2,123},
	{2,123},
	{2,123},
	{2,123},
	{2,123},
	{2,123},
	{2,124},
	{2,124},
	{3,124},
	{3,124},
	{3,124},
	{3,124},
	{3,124},
	{3,124},
	{3,124},
	{3,125},
	{3,125},
	{3,125},
	{3,125},
	{3,125},
	{3,125},
	{3,125},
	{3,125},
	{4,125},
	{4,125},
	{4,125},
	{5,125},
	{5,125},
	{5,125},
	{6,125},
	{6,124},
	{7,123},
	{8,123},
	{9,122},
	{10,121},
	{11,120},
	{12,120},
	{13,118},
	{15,117},
	{18,115},
	{20,114},
};

											
#define N_LEFT_WINDOW_SPANS  (sizeof(weapon_window_left)/sizeof(*weapon_window_left))
#define N_RIGHT_WINDOW_SPANS (sizeof(weapon_window_right)/sizeof(*weapon_window_right))

#define N_LEFT_WINDOW_SPANS_H  (sizeof(weapon_window_left_hires)/sizeof(*weapon_window_left_hires))
#define N_RIGHT_WINDOW_SPANS_H (sizeof(weapon_window_right_hires)/sizeof(*weapon_window_right_hires))

// defining box boundries for weapon pictures

#define PRIMARY_W_BOX_LEFT_L		63
#define PRIMARY_W_BOX_TOP_L		151		//154
#define PRIMARY_W_BOX_RIGHT_L		(PRIMARY_W_BOX_LEFT_L+58)
#define PRIMARY_W_BOX_BOT_L		(PRIMARY_W_BOX_TOP_L+N_LEFT_WINDOW_SPANS-1)
											
#define PRIMARY_W_BOX_LEFT_H		121
#define PRIMARY_W_BOX_TOP_H		364
#define PRIMARY_W_BOX_RIGHT_H		242
#define PRIMARY_W_BOX_BOT_H		(PRIMARY_W_BOX_TOP_H+N_LEFT_WINDOW_SPANS_H-1)		//470
											
#define PRIMARY_W_BOX_LEFT		(gameStates.video.nDisplayMode?PRIMARY_W_BOX_LEFT_H:PRIMARY_W_BOX_LEFT_L)
#define PRIMARY_W_BOX_TOP		(gameStates.video.nDisplayMode?PRIMARY_W_BOX_TOP_H:PRIMARY_W_BOX_TOP_L)
#define PRIMARY_W_BOX_RIGHT	(gameStates.video.nDisplayMode?PRIMARY_W_BOX_RIGHT_H:PRIMARY_W_BOX_RIGHT_L)
#define PRIMARY_W_BOX_BOT		(gameStates.video.nDisplayMode?PRIMARY_W_BOX_BOT_H:PRIMARY_W_BOX_BOT_L)

#define SECONDARY_W_BOX_LEFT_L	202	//207
#define SECONDARY_W_BOX_TOP_L		151
#define SECONDARY_W_BOX_RIGHT_L	263	//(SECONDARY_W_BOX_LEFT+54)
#define SECONDARY_W_BOX_BOT_L		(SECONDARY_W_BOX_TOP_L+N_RIGHT_WINDOW_SPANS-1)

#define SECONDARY_W_BOX_LEFT_H	404
#define SECONDARY_W_BOX_TOP_H		363
#define SECONDARY_W_BOX_RIGHT_H	529
#define SECONDARY_W_BOX_BOT_H		(SECONDARY_W_BOX_TOP_H+N_RIGHT_WINDOW_SPANS_H-1)		//470

#define SECONDARY_W_BOX_LEFT	(gameStates.video.nDisplayMode?SECONDARY_W_BOX_LEFT_H:SECONDARY_W_BOX_LEFT_L)
#define SECONDARY_W_BOX_TOP	(gameStates.video.nDisplayMode?SECONDARY_W_BOX_TOP_H:SECONDARY_W_BOX_TOP_L)
#define SECONDARY_W_BOX_RIGHT	(gameStates.video.nDisplayMode?SECONDARY_W_BOX_RIGHT_H:SECONDARY_W_BOX_RIGHT_L)
#define SECONDARY_W_BOX_BOT	(gameStates.video.nDisplayMode?SECONDARY_W_BOX_BOT_H:SECONDARY_W_BOX_BOT_L)

#define SB_PRIMARY_W_BOX_LEFT_L		34		//50
#define SB_PRIMARY_W_BOX_TOP_L		153
#define SB_PRIMARY_W_BOX_RIGHT_L		(SB_PRIMARY_W_BOX_LEFT_L+53+2)
#define SB_PRIMARY_W_BOX_BOT_L		(195+1)
											
#define SB_PRIMARY_W_BOX_LEFT_H		68
#define SB_PRIMARY_W_BOX_TOP_H		381
#define SB_PRIMARY_W_BOX_RIGHT_H		179
#define SB_PRIMARY_W_BOX_BOT_H		473
											
#define SB_PRIMARY_W_BOX_LEFT		(gameStates.video.nDisplayMode?SB_PRIMARY_W_BOX_LEFT_H:SB_PRIMARY_W_BOX_LEFT_L)
#define SB_PRIMARY_W_BOX_TOP		(gameStates.video.nDisplayMode?SB_PRIMARY_W_BOX_TOP_H:SB_PRIMARY_W_BOX_TOP_L)
#define SB_PRIMARY_W_BOX_RIGHT	(gameStates.video.nDisplayMode?SB_PRIMARY_W_BOX_RIGHT_H:SB_PRIMARY_W_BOX_RIGHT_L)
#define SB_PRIMARY_W_BOX_BOT		(gameStates.video.nDisplayMode?SB_PRIMARY_W_BOX_BOT_H:SB_PRIMARY_W_BOX_BOT_L)
											
#define SB_SECONDARY_W_BOX_LEFT_L	169
#define SB_SECONDARY_W_BOX_TOP_L		153
#define SB_SECONDARY_W_BOX_RIGHT_L	(SB_SECONDARY_W_BOX_LEFT_L+54+1)
#define SB_SECONDARY_W_BOX_BOT_L		(153+43)

#define SB_SECONDARY_W_BOX_LEFT_H	338
#define SB_SECONDARY_W_BOX_TOP_H		381
#define SB_SECONDARY_W_BOX_RIGHT_H	449
#define SB_SECONDARY_W_BOX_BOT_H		473

#define SB_SECONDARY_W_BOX_LEFT	(gameStates.video.nDisplayMode?SB_SECONDARY_W_BOX_LEFT_H:SB_SECONDARY_W_BOX_LEFT_L)	//210
#define SB_SECONDARY_W_BOX_TOP	(gameStates.video.nDisplayMode?SB_SECONDARY_W_BOX_TOP_H:SB_SECONDARY_W_BOX_TOP_L)
#define SB_SECONDARY_W_BOX_RIGHT	(gameStates.video.nDisplayMode?SB_SECONDARY_W_BOX_RIGHT_H:SB_SECONDARY_W_BOX_RIGHT_L)
#define SB_SECONDARY_W_BOX_BOT	(gameStates.video.nDisplayMode?SB_SECONDARY_W_BOX_BOT_H:SB_SECONDARY_W_BOX_BOT_L)

#define SB_PRIMARY_W_PIC_X			(SB_PRIMARY_W_BOX_LEFT+1)	//51
#define SB_PRIMARY_W_PIC_Y			(gameStates.video.nDisplayMode?382:154)
#define SB_PRIMARY_W_TEXT_X		(SB_PRIMARY_W_BOX_LEFT+(gameStates.video.nDisplayMode?50:24))	//(51+23)
#define SB_PRIMARY_W_TEXT_Y		(gameStates.video.nDisplayMode?390:157)
#define SB_PRIMARY_AMMO_X			(SB_PRIMARY_W_BOX_LEFT+(gameStates.video.nDisplayMode?(38+20):30))	//((SB_PRIMARY_W_BOX_LEFT+33)-3)	//(51+32)
#define SB_PRIMARY_AMMO_Y			(gameStates.video.nDisplayMode?410:171)

#define SB_SECONDARY_W_PIC_X		(gameStates.video.nDisplayMode?385:(SB_SECONDARY_W_BOX_LEFT+29))	//(212+27)
#define SB_SECONDARY_W_PIC_Y		(gameStates.video.nDisplayMode?382:154)
#define SB_SECONDARY_W_TEXT_X		(SB_SECONDARY_W_BOX_LEFT+2)	//212
#define SB_SECONDARY_W_TEXT_Y		(gameStates.video.nDisplayMode?389:157)
#define SB_SECONDARY_AMMO_X		(SB_SECONDARY_W_BOX_LEFT+(gameStates.video.nDisplayMode?(14-4):11))	//(212+9)
#define SB_SECONDARY_AMMO_Y		(gameStates.video.nDisplayMode?414:171)

typedef struct gauge_box {
	int left,top;
	int right,bot;		//maximal box
	span *spanlist;	//list of left,right spans for copy
} gauge_box;

gauge_box gauge_boxes[] = {

// primary left/right low res
		{PRIMARY_W_BOX_LEFT_L,PRIMARY_W_BOX_TOP_L,PRIMARY_W_BOX_RIGHT_L,PRIMARY_W_BOX_BOT_L,weapon_window_left},
		{SECONDARY_W_BOX_LEFT_L,SECONDARY_W_BOX_TOP_L,SECONDARY_W_BOX_RIGHT_L,SECONDARY_W_BOX_BOT_L,weapon_window_right},

//sb left/right low res
		{SB_PRIMARY_W_BOX_LEFT_L,SB_PRIMARY_W_BOX_TOP_L,SB_PRIMARY_W_BOX_RIGHT_L,SB_PRIMARY_W_BOX_BOT_L,NULL},
		{SB_SECONDARY_W_BOX_LEFT_L,SB_SECONDARY_W_BOX_TOP_L,SB_SECONDARY_W_BOX_RIGHT_L,SB_SECONDARY_W_BOX_BOT_L,NULL},

// primary left/right hires
		{PRIMARY_W_BOX_LEFT_H,PRIMARY_W_BOX_TOP_H,PRIMARY_W_BOX_RIGHT_H,PRIMARY_W_BOX_BOT_H,weapon_window_left_hires},
		{SECONDARY_W_BOX_LEFT_H,SECONDARY_W_BOX_TOP_H,SECONDARY_W_BOX_RIGHT_H,SECONDARY_W_BOX_BOT_H,weapon_window_right_hires},

// sb left/right hires
		{SB_PRIMARY_W_BOX_LEFT_H,SB_PRIMARY_W_BOX_TOP_H,SB_PRIMARY_W_BOX_RIGHT_H,SB_PRIMARY_W_BOX_BOT_H,NULL},
		{SB_SECONDARY_W_BOX_LEFT_H,SB_SECONDARY_W_BOX_TOP_H,SB_SECONDARY_W_BOX_RIGHT_H,SB_SECONDARY_W_BOX_BOT_H,NULL},
	};

// these macros refer to arrays above

#define COCKPIT_PRIMARY_BOX		(!gameStates.video.nDisplayMode?0:4)
#define COCKPIT_SECONDARY_BOX		(!gameStates.video.nDisplayMode?1:5)
#define SB_PRIMARY_BOX				(!gameStates.video.nDisplayMode?2:6)
#define SB_SECONDARY_BOX			(!gameStates.video.nDisplayMode?3:7)

static double xScale, yScale;

//	-----------------------------------------------------------------------------

#define HUD_SCALE(v,s)	((int) ((double) (v) * (s) + 0.5))
#define HUD_SCALE_X(v)	HUD_SCALE(v,cmScaleX)
#define HUD_SCALE_Y(v)	HUD_SCALE(v,cmScaleY)

//	-----------------------------------------------------------------------------

inline void HUDBitBlt (int x, int y, grs_bitmap *bmP, int scale, int orient)
{
OglUBitMapMC (
	(x < 0) ? -x : HUD_SCALE_X (x), 
	(y < 0) ? -y : HUD_SCALE_Y (y), 
	HUD_SCALE_X (bmP->bm_props.w), 
	HUD_SCALE_Y (bmP->bm_props.h), 
	bmP, 
	NULL,
	scale,
	orient
	);
}

//	-----------------------------------------------------------------------------

inline void HUDStretchBlt (int x, int y, grs_bitmap *bmP, int scale, int orient, double xScale, double yScale)
{
OglUBitMapMC (
	(x < 0) ? -x : HUD_SCALE_X (x), 
	(y < 0) ? -y : HUD_SCALE_Y (y), 
	HUD_SCALE_X ((int) (bmP->bm_props.w * xScale + 0.5)), 
	HUD_SCALE_Y ((int) (bmP->bm_props.h * yScale + 0.5)), 
	bmP, 
	NULL,
	scale,
	orient
	);
}

//	-----------------------------------------------------------------------------

inline void HUDRect (int left, int top, int width, int height)
{
GrRect (HUD_SCALE_X (left), HUD_SCALE_Y (top), HUD_SCALE_X (width), HUD_SCALE_Y (height));
}

//	-----------------------------------------------------------------------------

inline void HUDUScanLine (int left, int right, int y)
{
gr_uscanline (HUD_SCALE_X (left), HUD_SCALE_X (right), HUD_SCALE_Y (y));
}

//	-----------------------------------------------------------------------------

void _CDECL_ HUDPrintF (int x, int y, char *pszFmt, ...)
{
	static char szBuf [1000];
	va_list args;

	va_start(args, pszFmt);
	vsprintf (szBuf, pszFmt, args);
	GrString (HUD_SCALE_X (x), HUD_SCALE_Y (y), szBuf);
}

//	-----------------------------------------------------------------------------

//copy a box from the off-screen buffer to the visible page
#ifdef WINDOWS
void CopyGaugeBox(gauge_box *box,dd_grs_canvas *cv)
{
//	This is kind of funny.  If we are in a full cockpit mode
//	we have a system offscreen buffer for our canvas.
//	Since this is true of cockpit mode only, we should do a 
//	direct copy from system to video memory without blting.

	if (box->spanlist) {
		int n_spans = box->bot-box->top+1;
		int cnt,y;

		if (gameStates.render.cockpit.nMode==CM_FULL_COCKPIT && cv->sram) {
			grs_bitmap *bm;

			Assert(cv->sram);
		DDGRLOCK(cv);
		DDGRLOCK(dd_grd_curcanv);
			bm = &cv->canvas.cv_bitmap;		
	
			for (cnt=0,y=box->top;cnt<n_spans;cnt++,y++)
			{
				GrBmUBitBlt(box->spanlist[cnt].r-box->spanlist[cnt].l+1,1,
							box->left+box->spanlist[cnt].l,y,box->left+box->spanlist[cnt].l,y,bm,&grdCurCanv->cv_bitmap);
		  	}
		DDGRUNLOCK(dd_grd_curcanv);
		DDGRUNLOCK(cv);
		}
		else {
			for (cnt=0,y=box->top;cnt<n_spans;cnt++,y++)
			{
				dd_gr_blt_notrans(cv, 
								box->left+box->spanlist[cnt].l,y,
								box->spanlist[cnt].r-box->spanlist[cnt].l+1,1,
								dd_grd_curcanv,
								box->left+box->spanlist[cnt].l,y,
								box->spanlist[cnt].r-box->spanlist[cnt].l+1,1);
			}
		}
	}
	else {
		dd_gr_blt_notrans(cv, box->left, box->top, 
						box->right-box->left+1, box->bot-box->top+1,
						dd_grd_curcanv, box->left, box->top,
						box->right-box->left+1, box->bot-box->top+1);
	}
}

#else

//	-----------------------------------------------------------------------------

void CopyGaugeBox(gauge_box *box,grs_bitmap *bm)
{
	return;
	if (box->spanlist) {
		int n_spans = box->bot-box->top+1;
		int cnt,y;

//GrSetColorRGBi (RGBA_PAL (31,0,0);

		for (cnt=0,y=box->top;cnt<n_spans;cnt++,y++) {
			PA_DFX (pa_set_frontbuffer_current());
			PA_DFX (pa_set_back_to_read());

			GrBmUBitBlt(box->spanlist[cnt].r-box->spanlist[cnt].l+1,1,
						box->left+box->spanlist[cnt].l,y,box->left+box->spanlist[cnt].l,y,bm,&grdCurCanv->cv_bitmap);

			//gr_scanline(box->left+box->spanlist[cnt].l,box->left+box->spanlist[cnt].r,y);
			PA_DFX (pa_set_backbuffer_current());
			PA_DFX (pa_set_front_to_read());

	 	}
 	}
	else
	 {
		PA_DFX (pa_set_frontbuffer_current());
		PA_DFX (pa_set_back_to_read());
	
		GrBmUBitBlt(box->right-box->left+1,box->bot-box->top+1,
						box->left,box->top,box->left,box->top,
						bm,&grdCurCanv->cv_bitmap);
		PA_DFX (pa_set_backbuffer_current());
		PA_DFX (pa_set_front_to_read());
	 }
}
#endif

//	-----------------------------------------------------------------------------

//	-----------------------------------------------------------------------------
//fills in the coords of the hostage video window
void GetHostageWindowCoords(int *x,int *y,int *w,int *h)
{
	if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
		*x = SB_SECONDARY_W_BOX_LEFT;
		*y = SB_SECONDARY_W_BOX_TOP;
		*w = SB_SECONDARY_W_BOX_RIGHT - SB_SECONDARY_W_BOX_LEFT + 1;
		*h = SB_SECONDARY_W_BOX_BOT - SB_SECONDARY_W_BOX_TOP + 1;
	}
	else {
		*x = SECONDARY_W_BOX_LEFT;
		*y = SECONDARY_W_BOX_TOP;
		*w = SECONDARY_W_BOX_RIGHT - SECONDARY_W_BOX_LEFT + 1;
		*h = SECONDARY_W_BOX_BOT - SECONDARY_W_BOX_TOP + 1;
	}

}

//	-----------------------------------------------------------------------------
//these should be in gr.h 
#define cv_w  cv_bitmap.bm_props.w
#define cv_h  cv_bitmap.bm_props.h

extern fix ThisLevelTime;
extern fix xOmegaCharge;

void HUDShowScore()
{
	char	score_str[20];
	int	w, h, aw;

if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
	return;
if ((gameData.hud.msgs [0].nMessages > 0) && (strlen(gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;
GrSetCurFont( GAME_FONT );
if (((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP)))
	sprintf(score_str, "%s: %5d", TXT_KILLS, gameData.multi.players[gameData.multi.nLocalPlayer].net_kills_total);
else
	sprintf(score_str, "%s: %5d", TXT_SCORE, gameData.multi.players[gameData.multi.nLocalPlayer].score);
GrGetStringSize(score_str, &w, &h, &aw );
GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
GrPrintF(grdCurCanv->cv_w-w-LHX(2), 3, score_str);
 }

//	-----------------------------------------------------------------------------

void HUDShowTimerCount()
 {
#ifdef NETWORK
	char	score_str[20];
	int	w, h, aw,i;
	fix timevar=0;
#endif

	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
	if ((gameData.hud.msgs [0].nMessages > 0) && (strlen(gameData.hud.msgs [0].szMsgs[gameData.hud.msgs [0].nFirst]) > 38))
		return;

#ifdef NETWORK
   if ((gameData.app.nGameMode & GM_NETWORK) && netGame.PlayTimeAllowed)
    {
     timevar=i2f (netGame.PlayTimeAllowed*5*60);
     i=f2i(timevar-ThisLevelTime);
     i++;
    
     sprintf(score_str, "T - %5d", i);
																								 
     GrGetStringSize(score_str, &w, &h, &aw );
     GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);

     if (i>-1 && !gameData.reactor.bDestroyed)
	     GrPrintF(grdCurCanv->cv_w-w-LHX(10), LHX(11), score_str);
    }
#endif
 }

//	-----------------------------------------------------------------------------
//y offset between lines on HUD
int Line_spacing;

void HUDShowScoreAdded()
{
	int	color;
	int	w, h, aw;
	char	score_str[20];

	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
	if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP)) 
		return;

	if (score_display[0] == 0)
		return;

	GrSetCurFont( GAME_FONT );

	score_time -= gameData.app.xFrameTime;
	if (score_time > 0) {
		color = f2i(score_time * 20) + 12;
		if (color < 10) 
			color = 12;
		else if (color > 31) 
			color = 30;
		color = color - (color % 4);	//	Only allowing colors 12,16,20,24,28 speeds up gr_getcolor, improves caching

		if (gameStates.app.cheats.bEnabled)
			sprintf(score_str, "%s", TXT_CHEATER);
		else
			sprintf(score_str, "%5d", score_display[0]);

		GrGetStringSize(score_str, &w, &h, &aw );
		GrSetFontColorRGBi (RGBA_PAL2 (0, color, 0), 1, 0, 0);
		GrPrintF(grdCurCanv->cv_w-w-LHX(2+10), Line_spacing+4, score_str);
	} else {
		score_time = 0;
		score_display[0] = 0;
	}
	
}

//	-----------------------------------------------------------------------------

void SBShowScore()
{	                                                                                                                                                                                                                                                             
	char	score_str[20];
	int x,y;
	int	w, h, aw;
	static int last_x[4]={SB_SCORE_RIGHT_L,SB_SCORE_RIGHT_L,SB_SCORE_RIGHT_H,SB_SCORE_RIGHT_H};
	int redraw_score;

WIN(DDGRLOCK(dd_grd_curcanv));
	if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP)) 
		redraw_score = -99;
	else
		redraw_score = -1;

	if (old_score[VR_current_page]==redraw_score) {
		GrSetCurFont( GAME_FONT );
		GrSetFontColorRGBi (MEDGREEN_RGBA, 1, 0, 0);

		if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP)) {
			 PA_DFX(pa_set_frontbuffer_current()); 
          PA_DFX(HUDPrintF(SB_SCORE_LABEL_X,SB_SCORE_Y,"%s:", TXT_KILLS)); 
          PA_DFX(pa_set_backbuffer_current()); 
			 HUDPrintF(SB_SCORE_LABEL_X,SB_SCORE_Y,"%s:", TXT_KILLS);
			}
		else {
		   PA_DFX (pa_set_frontbuffer_current());
			PA_DFX (HUDPrintF(SB_SCORE_LABEL_X,SB_SCORE_Y,"%s:", TXT_SCORE));
         PA_DFX(pa_set_backbuffer_current()); 
			HUDPrintF(SB_SCORE_LABEL_X,SB_SCORE_Y,"%s:", TXT_SCORE);
		  }
	}

	GrSetCurFont( GAME_FONT );
	if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP)) 
		sprintf(score_str, "%5d", gameData.multi.players[gameData.multi.nLocalPlayer].net_kills_total);
	else
		sprintf(score_str, "%5d", gameData.multi.players[gameData.multi.nLocalPlayer].score);
	GrGetStringSize(score_str, &w, &h, &aw );

	x = SB_SCORE_RIGHT-w-LHX(2);
	y = SB_SCORE_Y;
	
	//erase old score
	GrSetColorRGBi (RGBA_PAL (0,0,0));
   PA_DFX (pa_set_frontbuffer_current());
	PA_DFX (HUDRect(last_x[(gameStates.video.nDisplayMode?2:0)+VR_current_page],y,SB_SCORE_RIGHT,y+GAME_FONT->ft_h));
   PA_DFX(pa_set_backbuffer_current()); 
	HUDRect(last_x[(gameStates.video.nDisplayMode?2:0)+VR_current_page],y,SB_SCORE_RIGHT,y+GAME_FONT->ft_h);

	if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP)) 
		GrSetFontColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
	else
		GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);	
 
   PA_DFX (pa_set_frontbuffer_current());
	PA_DFX (HUDPrintF(x,y,score_str));
   PA_DFX(pa_set_backbuffer_current()); 
 	HUDPrintF(x,y,score_str);

	last_x[(gameStates.video.nDisplayMode?2:0)+VR_current_page] = x;
WIN(DDGRUNLOCK(dd_grd_curcanv));
}

//	-----------------------------------------------------------------------------

void SBShowScoreAdded()
{
	int	color;
	int w, h, aw;
	char	score_str[32];
	int x;
	static int last_x[4]={SB_SCORE_RIGHT_L,SB_SCORE_RIGHT_L,SB_SCORE_RIGHT_H,SB_SCORE_RIGHT_H};
	static	int last_score_display[2] = { -1, -1};
   int frc=0;
	PA_DFX (frc=0);
	
	if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP)) 
		return;

	if (score_display[VR_current_page] == 0)
		return;

WIN(DDGRLOCK(dd_grd_curcanv));
	GrSetCurFont( GAME_FONT );

	score_time -= gameData.app.xFrameTime;
	if (score_time > 0) {
		if (score_display[VR_current_page] != last_score_display[VR_current_page] || frc) {
			GrSetColorRGBi (RGBA_PAL (0,0,0));
			PA_DFX (pa_set_frontbuffer_current());
			PA_DFX (HUDRect(last_x[(gameStates.video.nDisplayMode?2:0)+VR_current_page],SB_SCORE_ADDED_Y,SB_SCORE_ADDED_RIGHT,SB_SCORE_ADDED_Y+GAME_FONT->ft_h));
		   PA_DFX(pa_set_backbuffer_current()); 
			GrRect(last_x[(gameStates.video.nDisplayMode?2:0)+VR_current_page],SB_SCORE_ADDED_Y,SB_SCORE_ADDED_RIGHT,SB_SCORE_ADDED_Y+GAME_FONT->ft_h);

			last_score_display[VR_current_page] = score_display[VR_current_page];
		}

		color = f2i(score_time * 20) + 10;

		if (color < 10) 
			color = 10;
		else if (color > 31) 
			color = 31;

		if (gameStates.app.cheats.bEnabled)
			sprintf(score_str, "%s", TXT_CHEATER);
		else
			sprintf(score_str, "%5d", score_display[VR_current_page]);
		GrGetStringSize(score_str, &w, &h, &aw );
		x = SB_SCORE_ADDED_RIGHT-w-LHX(2);
		GrSetFontColorRGBi (RGBA_PAL2 (0, color, 0), 1, 0, 0);
		PA_DFX (pa_set_frontbuffer_current());
		PA_DFX (GrPrintF(x, SB_SCORE_ADDED_Y, score_str));
      PA_DFX(pa_set_backbuffer_current()); 
		GrPrintF(x, SB_SCORE_ADDED_Y, score_str);
		last_x[(gameStates.video.nDisplayMode?2:0)+VR_current_page] = x;
	} else {
		//erase old score
		GrSetColorRGBi (RGBA_PAL (0,0,0));
		PA_DFX (pa_set_frontbuffer_current());
		PA_DFX (HUDRect(last_x[(gameStates.video.nDisplayMode?2:0)+VR_current_page],SB_SCORE_ADDED_Y,SB_SCORE_ADDED_RIGHT,SB_SCORE_ADDED_Y+GAME_FONT->ft_h));
	   PA_DFX(pa_set_backbuffer_current()); 
		GrRect(last_x[(gameStates.video.nDisplayMode?2:0)+VR_current_page],SB_SCORE_ADDED_Y,SB_SCORE_ADDED_RIGHT,SB_SCORE_ADDED_Y+GAME_FONT->ft_h);
		score_time = 0;
		score_display[VR_current_page] = 0;
	}
WIN(DDGRUNLOCK(dd_grd_curcanv));	
}

fix	Last_warning_beep_time[2] = {0,0};		//	Time we last played homing missile warning beep.

//	-----------------------------------------------------------------------------

void PlayHomingWarning(void)
{
	fix	beep_delay;

	if (gameStates.app.bEndLevelSequence || gameStates.app.bPlayerIsDead)
		return;

	if (gameData.multi.players[gameData.multi.nLocalPlayer].homing_object_dist >= 0) {
		beep_delay = gameData.multi.players[gameData.multi.nLocalPlayer].homing_object_dist/128;
		if (beep_delay > F1_0)
			beep_delay = F1_0;
		else if (beep_delay < F1_0/8)
			beep_delay = F1_0/8;

		if (Last_warning_beep_time[VR_current_page] > gameData.app.xGameTime)
			Last_warning_beep_time[VR_current_page] = 0;

		if (gameData.app.xGameTime - Last_warning_beep_time[VR_current_page] > beep_delay/2) {
			DigiPlaySample( SOUND_HOMING_WARNING, F1_0 );
			Last_warning_beep_time[VR_current_page] = gameData.app.xGameTime;
		}
	}
}

int	Last_homing_warning_shown[2]={-1,-1};

//	-----------------------------------------------------------------------------

void ShowHomingWarning(void)
{
	if ((gameStates.render.cockpit.nMode == CM_STATUS_BAR) || (gameStates.app.bEndLevelSequence)) {
		if (Last_homing_warning_shown[VR_current_page] == 1) {
			PAGE_IN_GAUGE( GAUGE_HOMING_WARNING_OFF );
			WIN(DDGRLOCK(dd_grd_curcanv));
				/*GrUBitmapM*/HUDBitBlt( 
					HOMING_WARNING_X, HOMING_WARNING_Y, 
					gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_HOMING_WARNING_OFF), F1_0, 0);
			WIN(DDGRUNLOCK(dd_grd_curcanv));
			Last_homing_warning_shown[VR_current_page] = 0;
		}
		return;
	}

	WINDOS(
	  	DDGrSetCurrentCanvas( GetCurrentGameScreen()),
		GrSetCurrentCanvas( GetCurrentGameScreen())
	);


WIN(DDGRLOCK(dd_grd_curcanv))
{
	if (gameData.multi.players[gameData.multi.nLocalPlayer].homing_object_dist >= 0) {
		if (gameData.app.xGameTime & 0x4000) {
			//if (Last_homing_warning_shown[VR_current_page] != 1) 
			{
				PAGE_IN_GAUGE( GAUGE_HOMING_WARNING_ON );
				/*GrUBitmapM*/HUDBitBlt( 
					HOMING_WARNING_X, HOMING_WARNING_Y, 
					gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_HOMING_WARNING_ON), F1_0, 0);
				Last_homing_warning_shown[VR_current_page] = 1;
			}
		} 
		else {
			//if (Last_homing_warning_shown[VR_current_page] != 0) 
			{
				PAGE_IN_GAUGE( GAUGE_HOMING_WARNING_OFF );
				/*GrUBitmapM*/HUDBitBlt( 
					HOMING_WARNING_X, HOMING_WARNING_Y, 
					gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_HOMING_WARNING_OFF), F1_0, 0);
				Last_homing_warning_shown[VR_current_page] = 0;
			}
		}
	}
	else 
		//if (Last_homing_warning_shown[VR_current_page] != 0) 
	{
		PAGE_IN_GAUGE( GAUGE_HOMING_WARNING_OFF );
		/*GrUBitmapM*/HUDBitBlt( 
			HOMING_WARNING_X, HOMING_WARNING_Y, 
			gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_HOMING_WARNING_OFF), F1_0, 0);
		Last_homing_warning_shown[VR_current_page] = 0;
	}
}
WIN(DDGRUNLOCK(dd_grd_curcanv));

}

//	-----------------------------------------------------------------------------

#define MAX_SHOWN_LIVES 4

extern int Game_window_y;
extern int SW_y[2];

void HUDShowHomingWarning(void)
{
	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
	if (gameData.multi.players[gameData.multi.nLocalPlayer].homing_object_dist >= 0) {
		if (gameData.app.xGameTime & 0x4000) {
			int x=0x8000, y=grdCurCanv->cv_h-Line_spacing;

			if (weapon_box_user[0] != WBU_WEAPON || weapon_box_user[1] != WBU_WEAPON) {
				int wy = (weapon_box_user[0] != WBU_WEAPON)?SW_y[0]:SW_y[1];
				y = min(y,(wy - Line_spacing - Game_window_y));
			}

			GrSetCurFont( GAME_FONT );
			GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
			GrPrintF(x,y,TXT_LOCK);
		}
	}
}

//	-----------------------------------------------------------------------------

void HUDShowKeys(void)
{
	int y = 3*Line_spacing;
	int dx = GAME_FONT->ft_w+GAME_FONT->ft_w/2;

	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
	if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP))
		return;

	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_BLUE_KEY) {
		PAGE_IN_GAUGE( KEY_ICON_BLUE );
		GrUBitmapM(2,y,&gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(KEY_ICON_BLUE)] );

	}

	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_GOLD_KEY) {
		PAGE_IN_GAUGE( KEY_ICON_YELLOW );
		GrUBitmapM(2+dx,y,&gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(KEY_ICON_YELLOW)] );
	}

	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_RED_KEY) {
		PAGE_IN_GAUGE( KEY_ICON_RED );
		GrUBitmapM(2+2*dx,y,&gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(KEY_ICON_RED)] );
	}

}

//	-----------------------------------------------------------------------------

#ifdef NETWORK

void HUDShowOrbs (void)
{
	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
	if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) {
		int x,y;
		grs_bitmap *bmP;

		x=y=0;

		if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
			y = 2*Line_spacing;
			x = 4*GAME_FONT->ft_w;
		}
		else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
			y = Line_spacing;
			x = GAME_FONT->ft_w;
		}
		else if ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)) {
//			y = 5*Line_spacing;
			y = Line_spacing;
			x = GAME_FONT->ft_w;
			if (gameStates.render.fonts.bHires)
				y += Line_spacing;
		}
		else
			Int3();		//what sort of cockpit?

		bmP = &gameData.hoard.icon [gameStates.render.fonts.bHires].bm;
		GrUBitmapM(x,y,bmP);

		x+=bmP->bm_props.w+bmP->bm_props.w/2;
		y+=(gameStates.render.fonts.bHires?2:1);
		if (gameData.app.nGameMode & GM_ENTROPY) {
			char	szInfo [20];
			int	w, h, aw;
			if (gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo [PROXIMITY_INDEX] >= 
				 extraGameInfo [1].entropy.nCaptureVirusLimit)
				GrSetFontColorRGBi (ORANGE_RGBA, 1, 0, 0);
			else
				GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
			sprintf (szInfo, 
				"x %d [%d]", 
				gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[PROXIMITY_INDEX],
				gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[SMART_MINE_INDEX]);
			GrPrintF(x, y, szInfo);
#if 0
			GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
			GrGetStringSize(szInfo, &w, &h, &aw );
			y += h + (gameStates.render.fonts.bHires?2:1);
			sprintf (szInfo, "[%d ", gameData.entropy.nTeamRooms [0]);
			GrPrintF(x, y, szInfo);
			GrGetStringSize(szInfo, &w, &h, &aw );
			x += w;
			GrSetFontColorRGBi (BLUE_RGBA, 1, 0, 0);
			sprintf (szInfo, "%d ", gameData.entropy.nTeamRooms [1]);
			GrPrintF(x, y, szInfo);
			GrGetStringSize(szInfo, &w, &h, &aw );
			x += w;
			GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);
			sprintf (szInfo, "%d", gameData.entropy.nTeamRooms [2]);
			GrPrintF(x, y, szInfo);
			GrGetStringSize(szInfo, &w, &h, &aw );
			x += w;
			GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
			GrPrintF (x, y, "]");
#endif
			if (gameStates.entropy.bConquering) {
				int t = (extraGameInfo [1].entropy.nCaptureTimeLimit * 1000) - 
					     (gameStates.app.nSDLTicks - gameStates.entropy.nTimeLastMoved);

				if (t < 0)
					t = 0;
				GrGetStringSize(szInfo, &w, &h, &aw );
				x += w;
				GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);
				sprintf (szInfo, " %d.%d", t / 1000, (t % 1000) / 100);
				GrPrintF(x, y, szInfo);
				}
			}
		else
			GrPrintF(x, y, "x %d", gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[PROXIMITY_INDEX]);
	}
}

//	-----------------------------------------------------------------------------

void HUDShowFlag (void)
{
if (!gameOpts->render.cockpit.bHUD && 
	 ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
	return;
if ((gameData.app.nGameMode & GM_CAPTURE) && (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_FLAG)) {
	int x = 0, y = 0, icon;

	if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
		y = 2*Line_spacing;
		x = 4*GAME_FONT->ft_w;
		}
	else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
		y = Line_spacing;
		x = GAME_FONT->ft_w;
		}
	else if ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || 
				(gameStates.render.cockpit.nMode == CM_LETTERBOX)) {
		y = 5*Line_spacing;
		x = GAME_FONT->ft_w;
		if (gameStates.render.fonts.bHires)
			y += Line_spacing;
		}
	else
		Int3();		//what sort of cockpit?
	icon = (GetTeam (gameData.multi.nLocalPlayer) == TEAM_BLUE) ? FLAG_ICON_RED : FLAG_ICON_BLUE;
	PAGE_IN_GAUGE (icon);
	GrUBitmapM (x, y, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(icon));
	}
}
#endif

//	-----------------------------------------------------------------------------

inline int HUDShowFlashGauge (int h, int *bFlash, int tToggle)
{
	time_t t = gameStates.app.nSDLTicks;
	int b = *bFlash;

if (gameStates.app.bPlayerIsDead || gameStates.app.bPlayerExploded)
	b = 0;
else if (b == 2) {
	if (h > 20)
		b = 0;
	else if (h > 10)
		b = 1;
	}
else if (b == 1) {
	if (h > 20)
		b = 0;
	else if (h <= 10)
		b = 2;
	}
else {
	if (h <= 10)
		b = 2;
	else if (h <= 20)
		b = 1;
	else
		b = 0;
	tToggle = -1;
	}
*bFlash = b;
return (int) ((b && (tToggle <= t)) ? t + 300 / b : 0);
}

//	-----------------------------------------------------------------------------

void HUDShowEnergy(void)
{
	int h, y;

	//GrSetCurrentCanvas(&VR_render_sub_buffer[0]);	//render off-screen
if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
	return;

h = gameData.multi.players[gameData.multi.nLocalPlayer].energy ? f2ir (gameData.multi.players[gameData.multi.nLocalPlayer].energy) : 0;
if (gameOpts->render.cockpit.bTextGauges) {
	y = grdCurCanv->cv_h - ((gameData.app.nGameMode & GM_MULTI) ? 5 : 1) * Line_spacing;
	GrSetCurFont( GAME_FONT );
	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
	GrPrintF(2, y, "%s: %i", TXT_ENERGY, h);
	}
else {
	static int		bFlash = 0, bShow = 1;
	static time_t	tToggle;
	time_t			t;
	ubyte				c;

	if (t = HUDShowFlashGauge (h, &bFlash, (int) tToggle)) {
		tToggle = t;
		bShow = !bShow;
		}
	y = grdCurCanv->cv_h - (int) ((((gameData.app.nGameMode & GM_MULTI) ? 5 : 1) * Line_spacing - 1) * yScale);
	GrSetColorRGB (255, 255, (ubyte) ((h > 100) ? 255 : 0), 255);
	GrUBox (6, y, 6 + (int) (100 * xScale), y + (int) (9 * yScale));
	if (bFlash) {
		if (!bShow)
			goto skipGauge;
		h = 100;
		}
	c = (h > 100) ? 224 : 224;
	GrSetColorRGB (c, c, (ubyte) ((h > 100) ? c : 0), 128);
	GrURect (6, y, 6 + (int) (((h > 100) ? h - 100 : h) * xScale), y + (int) (9 * yScale));
	}

skipGauge:

if (gameData.demo.nState==ND_STATE_RECORDING ) {
	int energy = f2ir(gameData.multi.players[gameData.multi.nLocalPlayer].energy);

	if (energy != old_energy[VR_current_page]) {
		NDRecordPlayerEnergy(old_energy[VR_current_page], energy);
		old_energy[VR_current_page] = energy;
	 	}
	}
}

//	-----------------------------------------------------------------------------

void HUDShowAfterburner(void)
{
	int h, y;

	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
	if (! (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER))
		return;		//don't draw if don't have


	h = FixMul(xAfterburnerCharge, 100);
	if (gameOpts->render.cockpit.bTextGauges) {
		y = grdCurCanv->cv_h - ((gameData.app.nGameMode & GM_MULTI) ? 8 : 3) * Line_spacing;
		GrSetCurFont( GAME_FONT );
		GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
		GrPrintF(2, y, TXT_HUD_BURN, h);
		}
	else {
		y = grdCurCanv->cv_h - (int) ((((gameData.app.nGameMode & GM_MULTI) ? 8 : 3) * Line_spacing - 1) * yScale);
		GrSetColorRGB (255, 0, 0, 255);
		GrUBox (6, y, 6 + (int) (100 * xScale), y + (int) (9 * yScale));
		GrSetColorRGB (224, 0, 0, 128);
		GrURect (6, y, 6 + (int) (h * xScale), y + (int) (9 * yScale));
		}
	if (gameData.demo.nState==ND_STATE_RECORDING ) {
		if (xAfterburnerCharge != old_afterburner[VR_current_page]) {
			NDRecordPlayerAfterburner(old_afterburner[VR_current_page], xAfterburnerCharge);
			old_afterburner[VR_current_page] = xAfterburnerCharge;
	 	}
	}
}
#if 0
char *d2_very_short_secondary_weapon_names[] =
		{"Flash","Guided","SmrtMine","Mercury","Shaker"};
#endif
#define SECONDARY_WEAPON_NAMES_VERY_SHORT(weapon_num) 				\
	((weapon_num <= MEGA_INDEX)?GAMETEXT (541 + weapon_num):	\
	 GT (636+weapon_num-SMISSILE1_INDEX))

//return which bomb will be dropped next time the bomb key is pressed
extern int which_bomb();

//	-----------------------------------------------------------------------------

void ShowBombCount (int x,int y, int bg_color, int always_show)
{
	int bomb,count,countx;
	char txt[5],*t;

if (gameData.app.nGameMode & GM_ENTROPY)
	return;
bomb = which_bomb();
if (!bomb)
	return;
if ((gameData.app.nGameMode & GM_HOARD) && (bomb == PROXIMITY_INDEX))
	return;
count = gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[bomb];
#ifndef RELEASE
count = min(count,99);	//only have room for 2 digits - cheating give 200
#endif
countx = (bomb==PROXIMITY_INDEX)?count:-count;
if (always_show && count == 0)		//no bombs, draw nothing on HUD
	return;
if (!always_show && countx == old_bombcount[VR_current_page])
	return;
WIN(DDGRLOCK(dd_grd_curcanv));
// I hate doing this off of hard coded coords!!!!
if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {		//draw background
	GrSetColorRGBi (bg_color);
	if (!gameStates.video.nDisplayMode) {
		HUDRect(169,189,189,196);
		GrSetColorRGBi (RGBA_PAL (0,0,0));
		gr_scanline(168,189,189);
	} else {
		PA_DFX (pa_set_frontbuffer_current());
		PA_DFX (GrRect(HUD_SCALE_X (338), HUD_SCALE_Y (453), HUD_SCALE_X (378), HUD_SCALE_Y (470)));
      PA_DFX(pa_set_backbuffer_current()); 
		GrRect(HUD_SCALE_X (338), HUD_SCALE_Y (453), HUD_SCALE_X (378), HUD_SCALE_Y (470));

		GrSetColorRGBi (RGBA_PAL (0,0,0));
		PA_DFX (pa_set_frontbuffer_current());
		PA_DFX (gr_scanline(HUD_SCALE_X (336), HUD_SCALE_X (378), HUD_SCALE_Y (453)));
      PA_DFX(pa_set_backbuffer_current()); 
		gr_scanline(HUD_SCALE_X (336), HUD_SCALE_X (378), HUD_SCALE_Y (453));
		}
	}
if (count)
	GrSetFontColorRGBi (
		(bomb == PROXIMITY_INDEX) ? RGBA_PAL2 (55, 0, 0) : RGBA_PAL2 (59, 59, 21), 1,
		bg_color, bg_color != -1);
else if (bg_color != -1)
	GrSetFontColorRGBi (bg_color, 1, bg_color, 1);	//erase by drawing in background color
sprintf(txt,"B:%02d",count);
while ((t=strchr(txt,'1')) != NULL)
	*t = '\x84';	//convert to wide '1'
PA_DFX (pa_set_frontbuffer_current());
PA_DFX (GrString(x,y,txt));
PA_DFX(pa_set_backbuffer_current()); 
if ((gameStates.render.cockpit.nMode == CM_STATUS_BAR) || 
	 (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT))
	HUDPrintF (x,y,txt);
else
	GrString(x,y,txt);
WIN(DDGRUNLOCK(dd_grd_curcanv));
old_bombcount[VR_current_page] = countx;
}

//	-----------------------------------------------------------------------------

void DrawPrimaryAmmoInfo(int ammo_count)
{
	if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
		DrawAmmoInfo(SB_PRIMARY_AMMO_X,SB_PRIMARY_AMMO_Y,ammo_count,1);
	else
		DrawAmmoInfo(PRIMARY_AMMO_X,PRIMARY_AMMO_Y,ammo_count,1);
}

//	-----------------------------------------------------------------------------

void HUDToggleWeaponIcons (void)
{
	int	i;

for (i = 0; i < Controls.toggle_icons_count; i++)
	if (gameStates.app.bNostalgia)
		extraGameInfo [0].nWeaponIcons = 0;
	else {
		extraGameInfo [0].nWeaponIcons = (extraGameInfo [0].nWeaponIcons + 1) % 5;
		if (!gameOpts->render.weaponIcons.bEquipment && (extraGameInfo [0].nWeaponIcons == 3))
			extraGameInfo [0].nWeaponIcons = 4;
		}
}

//	-----------------------------------------------------------------------------

#define ICON_SCALE	3

void HUDShowWeaponIcons (void)
{
	grs_bitmap	*bmP;
	int	nWeaponIcons = (gameStates.render.cockpit.nMode == CM_STATUS_BAR) ? 3 : extraGameInfo [0].nWeaponIcons;
	int	nIconScale = (gameOpts->render.weaponIcons.bSmall || (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)) ? 4 : 3;
	int	nIconPos = nWeaponIcons - 1;
	int	nMaxAutoSelect;
	int	fw, fh, faw,
			i, j, ll, n,
			ox = 6,
			oy = 6,
			x, dx, y, dy;
	ubyte	alpha = gameOpts->render.weaponIcons.alpha;
	char	szAmmo [5];
	int	nLvlMap [2][10] = {{9,4,8,3,7,2,6,1,5,0},{4,3,2,1,0,4,3,2,1,0}};
	static int	wIcon = 0, 
					hIcon = 0;
	static int	w = -1,
					h = -1;
	static ubyte ammoType [2][10] = {{0,1,0,0,0,0,1,0,0,0},{1,1,1,1,1,1,1,1,1,1}};
	static int bInitIcons = 1;

ll = gameData.multi.players [gameData.multi.nLocalPlayer].laser_level;
if (gameOpts->render.weaponIcons.bShowAmmo) {
	GrSetCurFont (SMALL_FONT);
	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
	}
dx = (int) (10 * cmScaleX);
if (nWeaponIcons < 3) {
	if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT) {
		dy = (grdCurScreen->sc_h - grdCurCanv->cv_bitmap.bm_props.h);
		y = nIconPos ? grdCurScreen->sc_h - dy - oy : oy + hIcon + 12;
		}
	else {
		y = (2 - gameStates.app.bD1Mission) * (oy + hIcon) + 12;
		nIconPos = 1;
		}
	}
for (i = 0; i < 2; i++) {
	n = (gameStates.app.bD1Mission) ? 5 : 10;
	nMaxAutoSelect = 255;
	if (nWeaponIcons > 2) {
		int h;
		if (gameStates.render.cockpit.nMode != CM_STATUS_BAR)
			h = 0;
		else {
#ifdef _DEBUG
			h = gameStates.render.cockpit.nMode + (gameStates.video.nDisplayMode ? Num_cockpits / 2 : 0);
			h = cockpit_bitmap [h].index;
			h = gameData.pig.tex.bitmaps [0][h].bm_props.h;
#else
			h = gameData.pig.tex.bitmaps [0][cockpit_bitmap [gameStates.render.cockpit.nMode + (gameStates.video.nDisplayMode ? Num_cockpits / 2 : 0)].index].bm_props.h;
#endif
			}
		y = (grdCurCanv->cv_bitmap.bm_props.h - h - n * (hIcon + oy)) / 2 + hIcon;
		x = i ? grdCurScreen->sc_w - wIcon - ox : ox;
		}
	else {
		x = grdCurScreen->sc_w / 2;
		if (i)
			x += dx;
		else
			x -= dx + wIcon;
		}
	for (j = 0; j < n; j++) {
		int bArmed, bHave, bLoaded, l, m;

		if (gameOpts->render.weaponIcons.nSort && !gameStates.app.bD1Mission) {
			l = nWeaponOrder [i][j];
			if (l == 255)
				nMaxAutoSelect = j;
			l = nWeaponOrder [i][j + (j >= nMaxAutoSelect)];
			}
		else
			l = nLvlMap [gameStates.app.bD1Mission][j];
		m = i ? secondaryWeaponToWeaponInfo [l] : primaryWeaponToWeaponInfo [l];
		if ((gameData.pig.tex.nHamFileVersion >= 3) && gameStates.video.nDisplayMode) {
			bmP = gameData.pig.tex.bitmaps [0] + gameData.weapons.info [m].hires_picture.index;
			PIGGY_PAGE_IN (gameData.weapons.info [m].hires_picture, 0);
			}
		else {
			bmP = gameData.pig.tex.bitmaps [0] + gameData.weapons.info [m].picture.index;
			PIGGY_PAGE_IN (gameData.weapons.info [m].picture, 0);
			}
		Assert (bmP != NULL);
		if (w < bmP->bm_props.w)
			w = bmP->bm_props.w;
		if (h < bmP->bm_props.h)
			h = bmP->bm_props.h;
		PA_DFX (pa_set_backbuffer_current());
		wIcon = (int) ((w + nIconScale - 1) / nIconScale * cmScaleX);
		hIcon = (int) ((h + nIconScale - 1) / nIconScale * cmScaleY);
		if (bInitIcons)
			continue;
		if (i)
			bHave = (gameData.multi.players [gameData.multi.nLocalPlayer].secondary_weapon_flags & (1 << l));
		else if (!l) {
			bHave = (ll <= MAX_LASER_LEVEL);
			if (!bHave)
				continue;
			}
		else if (l == 5) {
			bHave = (ll > MAX_LASER_LEVEL);
			if (!bHave)
				continue;
			}
		else {
			bHave = (gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags & (1 << l));
			if (bHave && extraGameInfo [0].bSmartWeaponSwitch && ((l == 1) || (l == 2)) &&
				 gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags & (1 << (l + 5)))
				continue;
			}
		HUDBitBlt (nIconScale * -(x + (w - bmP->bm_props.w) / (2 * nIconScale)), 
					  nIconScale * -(y - hIcon), bmP, nIconScale * F1_0, 0);
		*szAmmo = '\0';
		if (bHave && ammoType [i][l]) {
			int nAmmo = (i ? gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [l] : gameData.multi.players [gameData.multi.nLocalPlayer].primary_ammo [(l == 6) ? 1 : l]);
			bLoaded = (nAmmo > 0);
			if (bLoaded && gameOpts->render.weaponIcons.bShowAmmo) {
				if (!i && (l % 5 == 1)) { //Gauss/Vulcan
					nAmmo = f2i (nAmmo * (unsigned) VULCAN_AMMO_SCALE);
#if 0
					sprintf (szAmmo, "%d.%d", nAmmo / 1000, (nAmmo % 1000) / 100);
#else
					if (nAmmo < 1000)
						sprintf (szAmmo, ".%d", nAmmo / 100);
					else
						sprintf (szAmmo, "%d", nAmmo / 1000);
#endif
					}
				else
					sprintf (szAmmo, "%d", nAmmo);
				GrGetStringSize (szAmmo, &fw, &fh, &faw);
				}
			}
		else {
			bLoaded = (gameData.multi.players [gameData.multi.nLocalPlayer].energy > gameData.weapons.info [l].energy_usage);
			if (l == 0) { //Lasers
				sprintf (szAmmo, "%d", (ll > MAX_LASER_LEVEL) ? MAX_LASER_LEVEL + 1 : ll + 1);
				GrGetStringSize (szAmmo, &fw, &fh, &faw);
				}
			else if ((l == 5) && (ll > MAX_LASER_LEVEL)) {
				sprintf (szAmmo, "%d", ll - MAX_LASER_LEVEL);
				GrGetStringSize (szAmmo, &fw, &fh, &faw);
				}
			}
		if (i && !bLoaded)
			bHave = 0;
		if (bHave) {
			//gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS * 2 / 3;
			if (bLoaded)
				GrSetColorRGB (128, 128, 0, (ubyte) (alpha * 16));
			else
				GrSetColorRGB (128, 0, 0, (ubyte) (alpha * 16));
			}
		else {
			//gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS * 2 / 7;
			GrSetColorRGB (64, 64, 64, (ubyte) (159 + alpha * 12));
			}
		GrURect (x - 1, y - hIcon - 1, x + wIcon + 2, y + 2);
		if (i) {
			if (j < 8)
				bArmed = (l == gameData.weapons.nSecondary);
			else
				bArmed = (j == 8) == bLastSecondaryWasSuper [PROXIMITY_INDEX];
			}
		else {
			if (l == 5)
				bArmed = (bHave && (0 == gameData.weapons.nPrimary));
			else if (l)
				bArmed = (l == gameData.weapons.nPrimary);
			else
				bArmed = (bHave && (l == gameData.weapons.nPrimary));
			}
		if (bArmed)
			if (bLoaded)
				GrSetColorRGB (255, 192, 0, 255);
			else
				GrSetColorRGB (160, 0, 0, 255);
		else if (bHave)
			if (bLoaded)
				GrSetColorRGB (0, 160, 0, 255);
			else
				GrSetColorRGB (96, 0, 0, 255);
		else
			GrSetColorRGB (64, 64, 64, 255);
		GrUBox (x - 1, y - hIcon - 1, x + wIcon + 2, y + 2);
		if (*szAmmo)
			GrString (x + wIcon + 2 - fw, y - fh, szAmmo);
		gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
		if (nWeaponIcons > 2)
			y += hIcon + oy;
		else {
			if (i)
				x += wIcon + ox;
			else
				x -= wIcon + ox;
			}
		}
	}
bInitIcons = 0;
}

//	-----------------------------------------------------------------------------

int LoadInventoryIcons (void)
{
	int	h, i;

if (!((bmpInventory = PiggyLoadBitmap ("inventry.bmp")) ||
	   (bmpInventory = PiggyLoadBitmap ("inventory.bmp"))))
	return bHaveInvBms = 0;
memset (bmInvItems, 0, sizeof (bmInvItems));
h = bmpInventory->bm_props.w * bmpInventory->bm_props.w;
for (i = 0; i < NUM_INV_ITEMS; i++) {
	bmInvItems [i] = *bmpInventory;
	bmInvItems [i].bm_props.h = bmInvItems [i].bm_props.w;
	bmInvItems [i].bm_texBuf += h * i;
	bmInvItems [i].bm_palette = gamePalette;
	}
return bHaveInvBms = 1;
}

//	-----------------------------------------------------------------------------

void FreeInventoryIcons (void)
{
if (bmpInventory) {
	GrFreeBitmap (bmpInventory);
	bmpInventory = NULL;
	bHaveInvBms = -1;
	}
}

//	-----------------------------------------------------------------------------

int HUDEquipmentActive (int bFlag)
{
switch (bFlag) {
	case PLAYER_FLAGS_AFTERBURNER:
		return (xAfterburnerCharge && Controls.afterburner_state);
	case PLAYER_FLAGS_CONVERTER:
		return gameStates.app.bUsingConverter;
	case PLAYER_FLAGS_HEADLIGHT:
		return (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON) != 0;
	case PLAYER_FLAGS_MAP_ALL:
		return 0;
	case PLAYER_FLAGS_AMMO_RACK:
		return 0;
	case PLAYER_FLAGS_QUAD_LASERS:
		return 0;
	case PLAYER_FLAGS_CLOAKED:
		return (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) != 0;
	case PLAYER_FLAGS_INVULNERABLE:
		return (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE) != 0;
	}
return 0;
}

//	-----------------------------------------------------------------------------

void HUDShowInventoryIcons (void)
{
	grs_bitmap	*bm;
	char	szCount [4];
	int nIconScale = (gameOpts->render.weaponIcons.bSmall || (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)) ? 3 : 2;
	int nIconPos = extraGameInfo [0].nWeaponIcons & 1;
	int	fw, fh, faw;
	int	j, n, firstItem,
			oy = 6,
			ox = 6,
			x, y, dy;
	int	w = bmpInventory->bm_props.w,
			h = bmpInventory->bm_props.w;
	int	wIcon = (int) ((w + nIconScale - 1) / nIconScale * cmScaleX), 
			hIcon = (int) ((h + nIconScale - 1) / nIconScale * cmScaleY);
	ubyte	alpha = gameOpts->render.weaponIcons.alpha;
	static int nInvFlags [NUM_INV_ITEMS] = {
		PLAYER_FLAGS_AFTERBURNER,
		PLAYER_FLAGS_CONVERTER,
		PLAYER_FLAGS_HEADLIGHT,
		PLAYER_FLAGS_MAP_ALL,
		PLAYER_FLAGS_AMMO_RACK,
		PLAYER_FLAGS_QUAD_LASERS,
		PLAYER_FLAGS_CLOAKED,
		PLAYER_FLAGS_INVULNERABLE
		};
	static int nEnergyType [NUM_INV_ITEMS] = {F1_0, 100 * F1_0, 0, F1_0, 0, F1_0, 0, 0};

dy = (grdCurScreen->sc_h - grdCurCanv->cv_bitmap.bm_props.h);
if ((gameStates.render.cockpit.nMode != CM_FULL_COCKPIT) && 
	 (gameStates.render.cockpit.nMode != CM_STATUS_BAR))
	y = nIconPos ? grdCurScreen->sc_h - dy - oy : oy + hIcon + 12;
else
	y = oy + hIcon + 12;
n = (gameOpts->gameplay.bInventory && !IsMultiGame) ? NUM_INV_ITEMS : NUM_INV_ITEMS - 2;
firstItem = gameStates.app.bD1Mission ? INV_ITEM_QUADLASERS : 0;
x = (grdCurScreen->sc_w - (n - firstItem) * wIcon - (n - 1 - firstItem) * ox) / 2;
for (j = firstItem; j < n; j++) {
	int bHave, bArmed, bActive = HUDEquipmentActive (nInvFlags [j]);
	bm = bmInvItems + j;
	PA_DFX (pa_set_backbuffer_current());
	HUDBitBlt (nIconScale * -(x + (w - bm->bm_props.w) / (2 * nIconScale)), nIconScale * -(y - hIcon), bm, nIconScale * F1_0, 0);
	//m = 9 - j;
	*szCount = '\0';
	if (j == INV_ITEM_INVUL) {
		if (bHave = (gameData.multi.players [gameData.multi.nLocalPlayer].nInvuls > 0))
			sprintf (szCount, "%d", gameData.multi.players [gameData.multi.nLocalPlayer].nInvuls);
		else
			bHave = gameData.multi.players [gameData.multi.nLocalPlayer].flags & nInvFlags [j];
		}
	else if (j == INV_ITEM_CLOAK) {
		if (bHave = (gameData.multi.players [gameData.multi.nLocalPlayer].nCloaks > 0))
			sprintf (szCount, "%d", gameData.multi.players [gameData.multi.nLocalPlayer].nCloaks);
		else
			bHave = gameData.multi.players [gameData.multi.nLocalPlayer].flags & nInvFlags [j];
		}
	else
		bHave = gameData.multi.players [gameData.multi.nLocalPlayer].flags & nInvFlags [j];
	bArmed = (gameData.multi.players [gameData.multi.nLocalPlayer].energy > nEnergyType [j]);
	if (bHave) {
		//gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS * 2 / 3;
		if (bArmed)
			if (bActive)
				GrSetColorRGB (255, 208, 0, (ubyte) (alpha * 16));
			else
				GrSetColorRGB (128, 128, 0, (ubyte) (alpha * 16));
		else
			GrSetColorRGB (128, 0, 0, (ubyte) (alpha * 16));
		}
	else {
		//gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS * 2 / 7;
		GrSetColorRGB (64, 64, 64, (ubyte) (159 + alpha * 12));
		}
	GrURect (x - 1, y - hIcon - 1, x + wIcon + 2, y + 2);
	if (bHave)
		if (bArmed)
			if (bActive)
				GrSetColorRGB (255, 208, 0, 255);
			else
				GrSetColorRGB (0, 160, 0, 255);
		else
			GrSetColorRGB (96, 0, 0, 255);
	else
		GrSetColorRGB (64, 64, 64, 255);
	GrUBox (x - 1, y - hIcon - 1, x + wIcon + 2, y + 2);
	if (*szCount) {
		GrGetStringSize (szCount, &fw, &fh, &faw);
		GrString (x + wIcon + 2 - fw, y - fh, szCount);
		}
	gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
	x += wIcon + ox;
	}
}

//	-----------------------------------------------------------------------------

void HUDShowIcons (void)
{
HUDToggleWeaponIcons ();
if (gameOpts->render.cockpit.bHUD || 
	 ((gameStates.render.cockpit.nMode != CM_FULL_SCREEN) && 
	  (gameStates.render.cockpit.nMode != CM_LETTERBOX)))
	if (EGI_FLAG (nWeaponIcons, 1, 0)) {
		HUDShowWeaponIcons ();
		if (gameOpts->render.weaponIcons.bEquipment) {
			if (bHaveInvBms < 0)
				LoadInventoryIcons ();
			if (bHaveInvBms > 0)
				HUDShowInventoryIcons ();
			}
		}
}

//	-----------------------------------------------------------------------------

//convert '1' characters to special wide ones
#define convert_1s(s) do {char *p=s; while ((p=strchr(p,'1')) != NULL) *p=(char)132;} while(0)

void HUDShowWeapons(void)
{
	int	w, h, aw;
	int	y;
	char	*weapon_name;
	char	weapon_str[32];

	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
#if 0
	HUDShowIcons ();
#endif
//	GrSetCurrentCanvas(&VR_render_sub_buffer[0]);	//render off-screen
	GrSetCurFont( GAME_FONT );
	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);

	y = grdCurCanv->cv_h;

	if (gameData.app.nGameMode & GM_MULTI)
		y -= 4*Line_spacing;

	weapon_name = PRIMARY_WEAPON_NAMES_SHORT(gameData.weapons.nPrimary);

	switch (gameData.weapons.nPrimary) {
		case LASER_INDEX:
			if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_QUAD_LASERS)
				sprintf(weapon_str, "%s %s %i", TXT_QUAD, weapon_name, gameData.multi.players[gameData.multi.nLocalPlayer].laser_level+1);
			else
				sprintf(weapon_str, "%s %i", weapon_name, gameData.multi.players[gameData.multi.nLocalPlayer].laser_level+1);
			break;

		case SUPER_LASER_INDEX:	Int3(); break;	//no such thing as super laser

		case VULCAN_INDEX:		
		case GAUSS_INDEX:			
			sprintf(weapon_str, "%s: %i", weapon_name, f2i((unsigned) gameData.multi.players[gameData.multi.nLocalPlayer].primary_ammo[VULCAN_INDEX] * (unsigned) VULCAN_AMMO_SCALE)); 
			convert_1s(weapon_str);
			break;

		case SPREADFIRE_INDEX:
		case PLASMA_INDEX:
		case FUSION_INDEX:
		case HELIX_INDEX:
		case PHOENIX_INDEX:
			strcpy(weapon_str, weapon_name);
			break;
		case OMEGA_INDEX:
			sprintf(weapon_str, "%s: %03i", weapon_name, xOmegaCharge * 100/MAX_OMEGA_CHARGE);
			convert_1s(weapon_str);
			break;

		default:						Int3();	weapon_str[0] = 0;	break;
	}

	GrGetStringSize(weapon_str, &w, &h, &aw );
	GrPrintF(grdCurCanv->cv_bitmap.bm_props.w-5-w, y-2*Line_spacing, weapon_str);

	if (gameData.weapons.nPrimary == VULCAN_INDEX) {
		if (gameData.multi.players[gameData.multi.nLocalPlayer].primary_ammo[gameData.weapons.nPrimary] != old_ammo_count[0][VR_current_page]) {
			if (gameData.demo.nState == ND_STATE_RECORDING)
				NDRecordPrimaryAmmo(old_ammo_count[0][VR_current_page], gameData.multi.players[gameData.multi.nLocalPlayer].primary_ammo[gameData.weapons.nPrimary]);
			old_ammo_count[0][VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].primary_ammo[gameData.weapons.nPrimary];
		}
	}

	if (gameData.weapons.nPrimary == OMEGA_INDEX) {
		if (xOmegaCharge != Old_Omega_charge[VR_current_page]) {
			if (gameData.demo.nState == ND_STATE_RECORDING)
				NDRecordPrimaryAmmo(Old_Omega_charge[VR_current_page], xOmegaCharge);
			Old_Omega_charge[VR_current_page] = xOmegaCharge;
		}
	}

	weapon_name = SECONDARY_WEAPON_NAMES_VERY_SHORT(gameData.weapons.nSecondary);

	sprintf(weapon_str, "%s %d",weapon_name,gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[gameData.weapons.nSecondary]);
	GrGetStringSize(weapon_str, &w, &h, &aw );
	GrPrintF(grdCurCanv->cv_bitmap.bm_props.w-5-w, y-Line_spacing, weapon_str);

	if (gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[gameData.weapons.nSecondary] != old_ammo_count[1][VR_current_page]) {
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordSecondaryAmmo(old_ammo_count[1][VR_current_page], gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[gameData.weapons.nSecondary]);
		old_ammo_count[1][VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[gameData.weapons.nSecondary];
	}

	ShowBombCount(grdCurCanv->cv_bitmap.bm_props.w-(3*GAME_FONT->ft_w+(gameStates.render.fonts.bHires?0:2)), y-3*Line_spacing,-1,1);
}

//	-----------------------------------------------------------------------------

void HUDShowCloakInvul(void)
{
	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);

	if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
		int	y = grdCurCanv->cv_h;

		if (gameData.app.nGameMode & GM_MULTI)
			y -= 7*Line_spacing;
		else
			y -= 4*Line_spacing;

		if ((gameData.multi.players[gameData.multi.nLocalPlayer].cloak_time+CLOAK_TIME_MAX - gameData.app.xGameTime > F1_0*3 ) || (gameData.app.xGameTime & 0x8000))
			GrPrintF(2, y, "%s", TXT_CLOAKED);
	}

	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE) {
		int	y = grdCurCanv->cv_h;

		if (gameData.app.nGameMode & GM_MULTI)
			y -= 10*Line_spacing;
		else
			y -= 5*Line_spacing;

		if (((gameData.multi.players[gameData.multi.nLocalPlayer].invulnerable_time + INVULNERABLE_TIME_MAX - gameData.app.xGameTime) > F1_0*4) || (gameData.app.xGameTime & 0x8000))
			GrPrintF(2, y, "%s", TXT_INVULNERABLE);
	}

}

//	-----------------------------------------------------------------------------

static int tBeepSpeed [2] = {F1_0 * 3 / 4, F1_0 * 5 / 8};

void HUDShowShield(void)
{
	int	h, y;

if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
	return;
//	GrSetCurrentCanvas(&VR_render_sub_buffer[0]);	//render off-screen

h = (gameData.multi.players[gameData.multi.nLocalPlayer].shields >= 0) ? f2ir(gameData.multi.players[gameData.multi.nLocalPlayer].shields) : 0; 
if (gameOpts->render.cockpit.bTextGauges) {
	y = grdCurCanv->cv_h - ((gameData.app.nGameMode & GM_MULTI) ? 6 : 2) * Line_spacing;
	GrSetCurFont( GAME_FONT );
	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
	GrPrintF(2, y, "%s: %i", TXT_SHIELD, h);
	}
else {
	static int		bShow = 1;
	static time_t	tToggle = 0, tBeep = 0, nBeep = -1;
	time_t			t = gameStates.app.nSDLTicks;
	int				bLastFlash = gameStates.gameplay.nShieldFlash;

	if (t = HUDShowFlashGauge (h, &gameStates.gameplay.nShieldFlash, (int) tToggle)) {
		tToggle = t;
		bShow = !bShow;
		}
	y = grdCurCanv->cv_h - (int) ((((gameData.app.nGameMode & GM_MULTI) ? 6 : 2) * Line_spacing - 1) * yScale);
	GrSetColorRGB (0, (ubyte) ((h > 100) ? 255 : 64), 255, 255);
	GrUBox (6, y, 6 + (int) (100 * xScale), y + (int) (9 * yScale));
	if (gameStates.gameplay.nShieldFlash) {
		if (gameOpts->gameplay.bShieldWarning) {
			if ((nBeep < 0) || (bLastFlash != gameStates.gameplay.nShieldFlash)) {
				if (nBeep >= 0)
					DigiStopSound ((int) nBeep);
				nBeep = DigiStartSound (DigiXlatSound (SOUND_HUD_MESSAGE), F1_0 * 2 / 3, 0xFFFF / 2, 
												  -1, -1, -1, -1, (gameStates.gameplay.nShieldFlash == 1) ? F1_0 * 3 / 4 : F1_0 / 2, NULL);
				}
			}
		else if (nBeep >= 0) {
			DigiStopSound ((int) nBeep);
			nBeep = -1;
			}
		if (!bShow)
			goto skipGauge;
		h = 100;
		}
	else {
		if (nBeep >= 0) {
			DigiStopSound ((int) nBeep);
			nBeep = -1;
			}
		}
	GrSetColorRGB (0, (ubyte) ((h > 100) ? 224 : 64), 224, 128);
	GrURect (6, y, 6 + (int) (((h > 100) ? h - 100 : h) * xScale), y + (int) (9 * yScale));
	}

skipGauge:

if (gameData.demo.nState==ND_STATE_RECORDING ) {
	int shields = f2ir(gameData.multi.players[gameData.multi.nLocalPlayer].shields);

	if (shields != old_shields[VR_current_page]) {		// Draw the shield gauge
		NDRecordPlayerShields(old_shields[VR_current_page], shields);
		old_shields[VR_current_page] = shields;
		}
	}
}

//	-----------------------------------------------------------------------------

//draw the icons for number of lives
void HUDShowLives()
{
	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
	if ((gameData.hud.msgs [0].nMessages > 0) && (strlen(gameData.hud.msgs [0].szMsgs[gameData.hud.msgs [0].nFirst]) > 38))
		return;

	if (gameData.app.nGameMode & GM_MULTI) {
		GrSetCurFont( GAME_FONT );
		GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
		GrPrintF(10, 3, "%s: %d", TXT_DEATHS, gameData.multi.players[gameData.multi.nLocalPlayer].net_killed_total);
	} 
	else if (gameData.multi.players[gameData.multi.nLocalPlayer].lives > 1)  {
		grs_bitmap *bm;
		GrSetCurFont( GAME_FONT );
		GrSetFontColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
		PAGE_IN_GAUGE( GAUGE_LIVES );
		bm = gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_LIVES);
		GrUBitmapM(10,3,bm);
		GrPrintF(10+bm->bm_props.w+bm->bm_props.w/2, 4, "x %d", gameData.multi.players[gameData.multi.nLocalPlayer].lives-1);
	}

}

//	-----------------------------------------------------------------------------

void SBShowLives()
{
	int x,y;
	grs_bitmap * bm = gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_LIVES);
   int frc=0;
	x = SB_LIVES_X;
	y = SB_LIVES_Y;
  
   PA_DFX (frc=0);

	WIN(DDGRLOCK(dd_grd_curcanv));
	if (old_lives[VR_current_page]==-1 || frc) {
		GrSetCurFont( GAME_FONT );
		GrSetFontColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
		if (gameData.app.nGameMode & GM_MULTI)
		 {
		   PA_DFX (pa_set_frontbuffer_current());
			PA_DFX (HUDPrintF(SB_LIVES_LABEL_X,SB_LIVES_LABEL_Y,"%s:", TXT_DEATHS));
		   PA_DFX (pa_set_backbuffer_current());
			HUDPrintF(SB_LIVES_LABEL_X,SB_LIVES_LABEL_Y,"%s:", TXT_DEATHS);

		 }
		else
		  {
		   PA_DFX (pa_set_frontbuffer_current());
			PA_DFX (HUDPrintF(SB_LIVES_LABEL_X,SB_LIVES_LABEL_Y,"%s:", TXT_LIVES));
		   PA_DFX (pa_set_backbuffer_current());
			HUDPrintF(SB_LIVES_LABEL_X,SB_LIVES_LABEL_Y,"%s:", TXT_LIVES);
		 }

	}
WIN(DDGRUNLOCK(dd_grd_curcanv));

	if (gameData.app.nGameMode & GM_MULTI)
	{
		char killed_str[20];
		int w, h, aw;
		static int last_x[4] = {SB_SCORE_RIGHT_L,SB_SCORE_RIGHT_L,SB_SCORE_RIGHT_H,SB_SCORE_RIGHT_H};
		int x;

	WIN(DDGRLOCK(dd_grd_curcanv));
		sprintf(killed_str, "%5d", gameData.multi.players[gameData.multi.nLocalPlayer].net_killed_total);
		GrGetStringSize(killed_str, &w, &h, &aw );
		GrSetColorRGBi (RGBA_PAL (0,0,0));
		HUDRect(last_x[(gameStates.video.nDisplayMode?2:0)+VR_current_page], y+1, SB_SCORE_RIGHT, y+GAME_FONT->ft_h);
		GrSetFontColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
		x = SB_SCORE_RIGHT-w-2;		
		HUDPrintF(x, y+1, killed_str);
		last_x[(gameStates.video.nDisplayMode?2:0)+VR_current_page] = x;
	WIN(DDGRUNLOCK(dd_grd_curcanv));
		return;
	}

	if (frc || old_lives[VR_current_page]==-1 || gameData.multi.players[gameData.multi.nLocalPlayer].lives != old_lives[VR_current_page]) {
	WIN(DDGRLOCK(dd_grd_curcanv));

		//erase old icons

		GrSetColorRGBi (RGBA_PAL (0,0,0));
      
      PA_DFX (pa_set_frontbuffer_current());
	   HUDRect(x, y, SB_SCORE_RIGHT, y+bm->bm_props.h);
      PA_DFX (pa_set_backbuffer_current());
	   HUDRect(x, y, SB_SCORE_RIGHT, y+bm->bm_props.h);

		if (gameData.multi.players[gameData.multi.nLocalPlayer].lives-1 > 0) {
			GrSetCurFont( GAME_FONT );
			GrSetFontColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
			PAGE_IN_GAUGE( GAUGE_LIVES );
			#ifdef PA_3DFX_VOODOO
		      PA_DFX (pa_set_frontbuffer_current();
				GrUBitmapM(x, y,bm);
				GrPrintF(x+bm->bm_props.w+GAME_FONT->ft_w, y, "x %d", gameData.multi.players[gameData.multi.nLocalPlayer].lives-1);
			#endif

	      PA_DFX (pa_set_backbuffer_current());
			/*GrUBitmapM*/HUDBitBlt (x, y, bm, F1_0, 0);
			HUDPrintF(x+bm->bm_props.w+GAME_FONT->ft_w, y, "x %d", gameData.multi.players[gameData.multi.nLocalPlayer].lives-1);

//			GrPrintF(x+12, y, "x %d", gameData.multi.players[gameData.multi.nLocalPlayer].lives-1);
		}
	WIN(DDGRUNLOCK(dd_grd_curcanv));
	}

//	for (i=0;i<draw_count;i++,x+=bm->bm_props.w+2)
//		GrUBitmapM(x,y,bm);

}

//	-----------------------------------------------------------------------------

#ifdef PIGGY_USE_PAGING
extern int Piggy_bitmap_cache_next;
#endif

void ShowTime()
{
	int secs = f2i(gameData.multi.players[gameData.multi.nLocalPlayer].time_level) % 60;
	int mins = f2i(gameData.multi.players[gameData.multi.nLocalPlayer].time_level) / 60;

GrSetCurFont( GAME_FONT );
GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
GrPrintF(grdCurCanv->cv_w-4*GAME_FONT->ft_w,grdCurCanv->cv_h-4*Line_spacing,"%d:%02d", mins, secs);

//@@#ifdef PIGGY_USE_PAGING
//@@	{
//@@		char text[25];
//@@		int w,h,aw;
//@@		sprintf( text, "%d KB", Piggy_bitmap_cache_next/1024 );
//@@		GrGetStringSize( text, &w, &h, &aw );	
//@@		GrPrintF(grdCurCanv->cv_w-10-w,grdCurCanv->cv_h/2, text );
//@@	}
//@@#endif

}

//	-----------------------------------------------------------------------------

#define EXTRA_SHIP_SCORE	50000		//get new ship every this many points

void AddPointsToScore(int points) 
{
	int prev_score;

	score_time += f1_0*2;
	score_display[0] += points;
	score_display[1] += points;
	if (score_time > f1_0*4) score_time = f1_0*4;

	if (points == 0 || gameStates.app.cheats.bEnabled)
		return;

	if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP))
		return;

	prev_score=gameData.multi.players[gameData.multi.nLocalPlayer].score;

	gameData.multi.players[gameData.multi.nLocalPlayer].score += points;

	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordPlayerScore(points);

#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI_COOP)
		MultiSendScore();

	if (gameData.app.nGameMode & GM_MULTI)
		return;
#endif

	if (gameData.multi.players[gameData.multi.nLocalPlayer].score/EXTRA_SHIP_SCORE != prev_score/EXTRA_SHIP_SCORE) {
		short snd;
		gameData.multi.players[gameData.multi.nLocalPlayer].lives += gameData.multi.players[gameData.multi.nLocalPlayer].score/EXTRA_SHIP_SCORE - prev_score/EXTRA_SHIP_SCORE;
		PowerupBasic(20, 20, 20, 0, TXT_EXTRA_LIFE);
		if ((snd=gameData.objs.pwrUp.info[POW_EXTRA_LIFE].hit_sound) > -1 )
			DigiPlaySample( snd, F1_0 );
	}
}

//	-----------------------------------------------------------------------------

void AddBonusPointsToScore(int points) 
{
	int prev_score;

	if (points == 0 || gameStates.app.cheats.bEnabled)
		return;

	prev_score=gameData.multi.players[gameData.multi.nLocalPlayer].score;

	gameData.multi.players[gameData.multi.nLocalPlayer].score += points;


	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordPlayerScore(points);

	if (gameData.app.nGameMode & GM_MULTI)
		return;

	if (gameData.multi.players[gameData.multi.nLocalPlayer].score/EXTRA_SHIP_SCORE != prev_score/EXTRA_SHIP_SCORE) {
		short snd;
		gameData.multi.players[gameData.multi.nLocalPlayer].lives += gameData.multi.players[gameData.multi.nLocalPlayer].score/EXTRA_SHIP_SCORE - prev_score/EXTRA_SHIP_SCORE;
		if ((snd=gameData.objs.pwrUp.info[POW_EXTRA_LIFE].hit_sound) > -1 )
			DigiPlaySample( snd, F1_0 );
	}
}

//	-----------------------------------------------------------------------------

#include "key.h"

static int bHaveGaugeCanvases = 0;

void InitGaugeCanvases()
{
if (!bHaveGaugeCanvases && gamePalette) {
	bHaveGaugeCanvases = 1;
	PAGE_IN_GAUGE( SB_GAUGE_ENERGY );
	PAGE_IN_GAUGE( GAUGE_AFTERBURNER );
	Canv_LeftEnergyGauge = GrCreateCanvas( LEFT_ENERGY_GAUGE_W, LEFT_ENERGY_GAUGE_H );
	Canv_SBEnergyGauge = GrCreateCanvas( SB_ENERGY_GAUGE_W, SB_ENERGY_GAUGE_H );
	Canv_SBAfterburnerGauge = GrCreateCanvas( SB_AFTERBURNER_GAUGE_W, SB_AFTERBURNER_GAUGE_H );
	Canv_RightEnergyGauge = GrCreateCanvas( RIGHT_ENERGY_GAUGE_W, RIGHT_ENERGY_GAUGE_H );
	Canv_NumericalGauge = GrCreateCanvas( NUMERICAL_GAUGE_W, NUMERICAL_GAUGE_H );
	Canv_AfterburnerGauge = GrCreateCanvas( AFTERBURNER_GAUGE_W, AFTERBURNER_GAUGE_H );
	}
}

//	-----------------------------------------------------------------------------

void CloseGaugeCanvases()
{
if (bHaveGaugeCanvases) {
	GrFreeCanvas( Canv_LeftEnergyGauge );
	GrFreeCanvas( Canv_SBEnergyGauge );
	GrFreeCanvas( Canv_SBAfterburnerGauge );
	GrFreeCanvas( Canv_RightEnergyGauge );
	GrFreeCanvas( Canv_NumericalGauge );
	GrFreeCanvas( Canv_AfterburnerGauge );
	bHaveGaugeCanvases = 0;
	}
}

//	-----------------------------------------------------------------------------

void InitGauges()
{
	int i;

	//draw_gauges_on 	= 1;

	for (i=0; i<2; i++ )	{
		if (((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP)) || ((gameData.demo.nState == ND_STATE_PLAYBACK) && (gameData.demo.nGameMode & GM_MULTI) && !(gameData.demo.nGameMode & GM_MULTI_COOP))) 
			old_score[i] = -99;
		else
			old_score[i]			= -1;
		old_energy[i]			= -1;
		old_shields[i]			= -1;
		old_flags[i]			= -1;
		old_cloak[i]			= -1;
		old_lives[i]			= -1;
		old_afterburner[i]	= -1;
		old_bombcount[i]		= 0;
		old_laser_level[i]	= 0;
	
		old_weapon[0][i] = old_weapon[1][i] = -1;
		old_ammo_count[0][i] = old_ammo_count[1][i] = -1;
		Old_Omega_charge[i] = -1;
	}

	nCloakFadeState = 0;

	weapon_box_user[0] = weapon_box_user[1] = WBU_WEAPON;
}

//	-----------------------------------------------------------------------------

void DrawEnergyBar(int energy)
{
	grs_bitmap *bm;
	int energy0;
	int x1, x2, y, yMax, i;
	int h0 = HUD_SCALE_X (LEFT_ENERGY_GAUGE_H - 1);
	int h1 = HUD_SCALE_Y (LEFT_ENERGY_GAUGE_H / 4);
	int h2 = HUD_SCALE_Y ((LEFT_ENERGY_GAUGE_H * 3) / 4);
	int w1 = HUD_SCALE_X (LEFT_ENERGY_GAUGE_W - 1);
	int w2 = HUD_SCALE_X (LEFT_ENERGY_GAUGE_W - 2);
	int w3 = HUD_SCALE_X (LEFT_ENERGY_GAUGE_W - 3);
	double eBarScale = (100.0 - (double) energy) * cmScaleX * 0.15 / (double) HUD_SCALE_Y (LEFT_ENERGY_GAUGE_H);

	// Draw left energy bar
//	GrSetCurrentCanvas( Canv_LeftEnergyGauge );
	PAGE_IN_GAUGE( GAUGE_ENERGY_LEFT );
	bm = gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_ENERGY_LEFT);
//	GrUBitmapM( 0, 0, bm);
	WIN(DDGRLOCK(dd_grd_curcanv));
//	GrUBitmapM( LEFT_ENERGY_GAUGE_X, LEFT_ENERGY_GAUGE_Y, bm);
	HUDBitBlt (LEFT_ENERGY_GAUGE_X, LEFT_ENERGY_GAUGE_Y, bm, F1_0, 0);
	WIN(DDGRUNLOCK(dd_grd_curcanv));
#ifdef _DEBUG
	GrSetColorRGBi (RGBA_PAL (255,255,255));
#else
	GrSetColorRGBi (RGBA_PAL (0,0,0));
#endif
	//energy0 = (gameStates.video.nDisplayMode ? 125 - (energy * 125) / 100 : 61 - (energy * 61) / 100);
	energy0 = HUD_SCALE_X (112);
	energy0 = energy0 - (energy * energy0) / 100;
	//energy0 = HUD_SCALE_X (energy0);
	if (energy < 100) {
		gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
		for (i = 0; i < LEFT_ENERGY_GAUGE_H; i++) {
			yMax = HUD_SCALE_Y (i + 1);
			for (y = i; y <= yMax; y++) {
				x1 = h0 - y;
				x2 = x1 + energy0 + (int) ((double) y * eBarScale);
				if (y < h1) {
					if (x2 > w1) 
						x2 = w1;
					}
				else if (y < h2) {
					if (x2 > w2)
						x2 = w2;
					}
				else {
					if (x2 > w3) 
						x2 = w3;
					}
				if (x2 > x1)
					gr_uscanline ( 
						HUD_SCALE_X (LEFT_ENERGY_GAUGE_X) + x1, 
						HUD_SCALE_X (LEFT_ENERGY_GAUGE_X) + x2, 
						HUD_SCALE_Y (LEFT_ENERGY_GAUGE_Y) + y); 
				}
			}
		}
	
	WINDOS(
		DDGrSetCurrentCanvas(GetCurrentGameScreen()),
		GrSetCurrentCanvas( GetCurrentGameScreen())
	);
#if 0
	WIN(DDGRLOCK(dd_grd_curcanv));
	GrUBitmapM( LEFT_ENERGY_GAUGE_X, LEFT_ENERGY_GAUGE_Y, &Canv_LeftEnergyGauge->cv_bitmap );
	WIN(DDGRUNLOCK(dd_grd_curcanv));
#endif
	// Draw right energy bar
//	GrSetCurrentCanvas( Canv_RightEnergyGauge );
	PAGE_IN_GAUGE( GAUGE_ENERGY_RIGHT );
	bm = gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_ENERGY_RIGHT);
//	GrUBitmapM( 0, 0, bm);
//	GrUBitmapM( RIGHT_ENERGY_GAUGE_X, RIGHT_ENERGY_GAUGE_Y, bm);
	HUDBitBlt (RIGHT_ENERGY_GAUGE_X, RIGHT_ENERGY_GAUGE_Y, bm, F1_0, 0);
#ifdef _DEBUG
	GrSetColorRGBi (RGBA_PAL (255,255,255));
#else
	GrSetColorRGBi (RGBA_PAL (0,0,0));
#endif
	h0 = HUD_SCALE_X (RIGHT_ENERGY_GAUGE_W - RIGHT_ENERGY_GAUGE_H);
	w1 = HUD_SCALE_X (1);
	w2 = HUD_SCALE_X (2);
	if (energy < 100) {
		yMax = HUD_SCALE_Y (RIGHT_ENERGY_GAUGE_H);
		for (i = 0; i < RIGHT_ENERGY_GAUGE_H; i++) {
			yMax = HUD_SCALE_Y (i + 1);
			for (y = i; y <= yMax; y++) {
				x2 = h0 + y;
				x1 = x2 - energy0 - (int) ((double) y * eBarScale);
				if (y < h1) {
					if (x1 < 0) 
						x1 = 0;
					}
				else if (y < h2) {
					if (x1 < w1) 
						x1 = w1;
					}
				else {
					if (x1 < w2) 
						x1 = w2;
					}
				if (x2 > x1) 
					gr_uscanline( 
						HUD_SCALE_X (RIGHT_ENERGY_GAUGE_X) + x1, 
						HUD_SCALE_X (RIGHT_ENERGY_GAUGE_X) + x2, 
						HUD_SCALE_Y (RIGHT_ENERGY_GAUGE_Y) + y); 
				}
			}
		}

	WINDOS(
		DDGrSetCurrentCanvas(GetCurrentGameScreen()),
		GrSetCurrentCanvas( GetCurrentGameScreen())
	);
#if 0
	WIN(DDGRLOCK(dd_grd_curcanv));
		GrUBitmapM( RIGHT_ENERGY_GAUGE_X, RIGHT_ENERGY_GAUGE_Y, &Canv_RightEnergyGauge->cv_bitmap );
	WIN(DDGRUNLOCK(dd_grd_curcanv));
#endif
}

//	-----------------------------------------------------------------------------

ubyte afterburner_bar_table[AFTERBURNER_GAUGE_H_L*2] = {
			3,11,
			3,11,
			3,11,
			3,11,
			3,11,
			3,11,
			2,11,
			2,10,
			2,10,
			2,10,
			2,10,
			2,10,
			2,10,
			1,10,
			1,10,
			1,10,
			1,9,
			1,9,
			1,9,
			1,9,
			0,9,
			0,9,
			0,8,
			0,8,
			0,8,
			0,8,
			1,8,
			2,8,
			3,8,
			4,8,
			5,8,
			6,7,
};

ubyte afterburner_bar_table_hires[AFTERBURNER_GAUGE_H_H*2] = {
	5,20,
	5,20,
	5,19,
	5,19,
	5,19,
	5,19,
	4,19,
	4,19,
	4,19,
	4,19,

	4,19,
	4,18,
	4,18,
	4,18,
	4,18,
	3,18,
	3,18,
	3,18,
	3,18,
	3,18,

	3,18,
	3,17,
	3,17,
	2,17,
	2,17,
	2,17,
	2,17,
	2,17,
	2,17,
	2,17,

	2,17,
	2,16,
	2,16,
	1,16,
	1,16,
	1,16,
	1,16,
	1,16,
	1,16,
	1,16,

	1,16,
	1,15,
	1,15,
	1,15,
	0,15,
	0,15,
	0,15,
	0,15,
	0,15,
	0,15,

	0,14,
	0,14,
	0,14,
	1,14,
	2,14,
	3,14,
	4,14,
	5,14,
	6,13,
	7,13,

	8,13,
	9,13,
	10,13,
	11,13,
	12,13
};


//	-----------------------------------------------------------------------------

void DrawAfterburnerBar(int afterburner)
{
	int not_afterburner;
	int i, j, y, yMax;
	grs_bitmap *bm;
	ubyte *pabt = gameStates.video.nDisplayMode ? afterburner_bar_table_hires : afterburner_bar_table;

	// Draw afterburner bar
	//GrSetCurrentCanvas( Canv_AfterburnerGauge );
	PAGE_IN_GAUGE( GAUGE_AFTERBURNER );
	bm =  gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_AFTERBURNER);
//	GrUBitmapM( 0, 0, bm);
//	GrUBitmapM( AFTERBURNER_GAUGE_X, AFTERBURNER_GAUGE_Y, bm);
	HUDBitBlt (AFTERBURNER_GAUGE_X, AFTERBURNER_GAUGE_Y, bm, F1_0, 0);
	GrSetColorRGB (0, 0, 0, 255);
	not_afterburner = FixMul(f1_0 - afterburner,AFTERBURNER_GAUGE_H);
	gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
	yMax = HUD_SCALE_Y (not_afterburner);
	for (y = 0; y < not_afterburner; y++) {
		for (i = HUD_SCALE_Y (y), j = HUD_SCALE_Y (y + 1); i < j; i++) {
			gr_uscanline (
				HUD_SCALE_X (AFTERBURNER_GAUGE_X + pabt [y * 2]), 
				HUD_SCALE_X (AFTERBURNER_GAUGE_X + pabt [y * 2 + 1] + 1), 
				HUD_SCALE_Y (AFTERBURNER_GAUGE_Y) + i);
			}
		}
	WINDOS(
		DDGrSetCurrentCanvas(GetCurrentGameScreen()),
		GrSetCurrentCanvas( GetCurrentGameScreen())
	);
#if 0
	WIN(DDGRLOCK(dd_grd_curcanv));
	GrUBitmapM( AFTERBURNER_GAUGE_X, AFTERBURNER_GAUGE_Y, &Canv_AfterburnerGauge->cv_bitmap );
	WIN(DDGRUNLOCK(dd_grd_curcanv));
#endif
}

//	-----------------------------------------------------------------------------

void DrawShieldBar(int shield)
{
	int bm_num = shield>=100?9:(shield / 10);

	PAGE_IN_GAUGE( GAUGE_SHIELDS+9-bm_num	);
	WIN(DDGRLOCK(dd_grd_curcanv));
	PA_DFX (pa_set_frontbuffer_current());
   PA_DFX (GrUBitmapM( SHIELD_GAUGE_X, SHIELD_GAUGE_Y, &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(GAUGE_SHIELDS+9-bm_num)] ));
	PA_DFX (pa_set_backbuffer_current());
//	GrUBitmapM( SHIELD_GAUGE_X, SHIELD_GAUGE_Y, &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(GAUGE_SHIELDS+9-bm_num)] );
	HUDBitBlt (SHIELD_GAUGE_X, SHIELD_GAUGE_Y, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_SHIELDS+9-bm_num), F1_0, 0);

	WIN(DDGRUNLOCK(dd_grd_curcanv));
}

#define CLOAK_FADE_WAIT_TIME  0x400

//	-----------------------------------------------------------------------------

void DrawPlayerShip(int nCloakState,int nOldCloakState,int x, int y)
{
	static fix xCloakFadeTimer=0;
	static int nCloakFadeValue=GR_ACTUAL_FADE_LEVELS-1;
	static int refade = 0;
	grs_bitmap *bm = NULL;

	if (gameData.app.nGameMode & GM_TEAM)	{
		#ifdef NETWORK
		PAGE_IN_GAUGE( GAUGE_SHIPS+GetTeam(gameData.multi.nLocalPlayer));
		bm =gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_SHIPS+GetTeam(gameData.multi.nLocalPlayer));
		#endif
	} else {
		PAGE_IN_GAUGE( GAUGE_SHIPS+gameData.multi.nLocalPlayer );
		bm = gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_SHIPS+gameData.multi.nLocalPlayer);
	}
	

	if (nOldCloakState==-1 && nCloakState)
			nCloakFadeValue=0;

	if (!nCloakState) {
		nCloakFadeValue=GR_ACTUAL_FADE_LEVELS-1;
		nCloakFadeState = 0;
	}

	if (nCloakState==1 && nOldCloakState==0)
		nCloakFadeState = -1;
	//else if (nCloakState==0 && nOldCloakState==1)
	//	nCloakFadeState = 1;

	if (nCloakState==nOldCloakState)		//doing "about-to-uncloak" effect
		if (nCloakFadeState==0)
			nCloakFadeState = 2;
	

	if (nCloakFadeState)
		xCloakFadeTimer -= gameData.app.xFrameTime;

	while (nCloakFadeState && xCloakFadeTimer < 0) {
		xCloakFadeTimer += CLOAK_FADE_WAIT_TIME;
		nCloakFadeValue += nCloakFadeState;
		if (nCloakFadeValue >= GR_ACTUAL_FADE_LEVELS-1) {
			nCloakFadeValue = GR_ACTUAL_FADE_LEVELS-1;
			if (nCloakFadeState == 2 && nCloakState)
				nCloakFadeState = -2;
			else
				nCloakFadeState = 0;
		}
		else if (nCloakFadeValue <= 0) {
			nCloakFadeValue = 0;
			if (nCloakFadeState == -2)
				nCloakFadeState = 2;
			else
				nCloakFadeState = 0;
		}
	}

//	To fade out both pages in a paged mode.
	if (refade) 
		refade = 0;
	else if (nCloakState && nOldCloakState && !nCloakFadeState && !refade) {
		nCloakFadeState = -1;
		refade = 1;
	}

#if defined(POLY_ACC)
	    gameStates.render.grAlpha = nCloakFadeValue;
	    GrSetCurrentCanvas( GetCurrentGameScreen());
	    PA_DFX (pa_set_frontbuffer_current();	
	    PA_DFX (pa_blit_lit(&grdCurCanv->cv_bitmap, HUD_SCALE_X (x), HUD_SCALE_Y (y), bm, 0, 0, HUD_SCALE_X (bm->bm_props.w), HUD_SCALE_Y (bm->bm_props.h));
		 PA_DFX (pa_set_backbuffer_current();	
	    pa_blit_lit(&grdCurCanv->cv_bitmap, HUD_SCALE_X (x), HUD_SCALE_Y (y), bm, 0, 0, HUD_SCALE_X (bm->bm_props.w), HUD_SCALE_Y (bm->bm_props.h));

	    gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
	    return;
//	    }
//	    else
//		    gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
//		 Int3();
#endif

	if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT) {
		WINDOS( 		
			DDGrSetCurrentCanvas(&dd_VR_render_buffer[0]),
			GrSetCurrentCanvas(&VR_render_buffer[0])
		);
	}
	WIN(DDGRLOCK(dd_grd_curcanv));
		if (nCloakFadeValue >= GR_ACTUAL_FADE_LEVELS - 1)
			//gr_ubitmap( x, y, bm);
		HUDBitBlt (x, y, bm, F1_0, 0);

//		gameStates.render.grAlpha = nCloakFadeValue;
//		GrRect(x, y, x+bm->bm_props.w-1, y+bm->bm_props.h-1);
//		gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
	WIN(DDGRUNLOCK(dd_grd_curcanv));
	if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT) {
		WINDOS(
			DDGrSetCurrentCanvas(GetCurrentGameScreen()),
			GrSetCurrentCanvas( GetCurrentGameScreen())
		);
	}

#ifdef WINDOWS
	DDGRLOCK(dd_grd_curcanv);
	if (dd_grd_curcanv->lpdds != dd_VR_render_buffer[0].lpdds) {
		DDGRLOCK(&dd_VR_render_buffer[0]);
	}
	else {
		dd_gr_dup_hack(&dd_VR_render_buffer[0], dd_grd_curcanv);
	}
#endif
	WINDOS(
		GrBmUBitBltM( 
			(int) (bm->bm_props.w * cmScaleX), (int) (bm->bm_props.h * cmScaleY), 
			(int) (x * cmScaleX), (int) (y * cmScaleY), x, y, 
			&dd_VR_render_buffer[0].canvas.cv_bitmap, &grdCurCanv->cv_bitmap),
		GrBmUBitBltM( 
			(int) (bm->bm_props.w * cmScaleX), (int) (bm->bm_props.h * cmScaleY),
			(int) (x * cmScaleX), (int) (y * cmScaleY), x, y, 
			&VR_render_buffer[0].cv_bitmap, &grdCurCanv->cv_bitmap)
	);
#ifdef WINDOWS
	if (dd_grd_curcanv->lpdds != dd_VR_render_buffer[0].lpdds) {
		DDGRUNLOCK(&dd_VR_render_buffer[0]);
	}
	else {
		dd_gr_dup_unhack(&dd_VR_render_buffer[0]);
	}
	DDGRUNLOCK(dd_grd_curcanv);
#endif
}

#define INV_FRAME_TIME	(f1_0/10)		//how long for each frame

//	-----------------------------------------------------------------------------

inline int NumDispX (int val)
{
int x = ((val > 99) ? 7 : (val > 9) ? 11 : 15);
if (!gameStates.video.nDisplayMode)
	x /= 2;
return x + NUMERICAL_GAUGE_X;
}

void DrawNumericalDisplay(int shield, int energy)
{
	int dx = NUMERICAL_GAUGE_X,
		 dy = NUMERICAL_GAUGE_Y;

	if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT)
		GrSetCurrentCanvas( Canv_NumericalGauge );
	PAGE_IN_GAUGE( GAUGE_NUMERICAL );
//	gr_ubitmap(dx, dy, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_NUMERICAL));
	HUDBitBlt (dx, dy, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_NUMERICAL), F1_0, 0);
	GrSetCurFont( GAME_FONT );
	GrSetFontColorRGBi (RGBA_PAL2 (14, 14, 23), 1, 0, 0);
	HUDPrintF(NumDispX (shield), dy + (gameStates.video.nDisplayMode ? 36 : 16),"%d",shield);
	GrSetFontColorRGBi (RGBA_PAL2 (25, 18, 6), 1, 0, 0);
	HUDPrintF(NumDispX (energy), dy + (gameStates.video.nDisplayMode ? 5 : 2),"%d",energy);
	
	if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT) {
		WINDOS(
			DDGrSetCurrentCanvas(GetCurrentGameScreen()),
			GrSetCurrentCanvas( GetCurrentGameScreen())
		);
		WIN(DDGRLOCK(dd_grd_curcanv));
		//	GrUBitmapM( NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y, &Canv_NumericalGauge->cv_bitmap );
			HUDBitBlt (NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y, &Canv_NumericalGauge->cv_bitmap, F1_0, 0);
		WIN(DDGRUNLOCK(dd_grd_curcanv));
	}
}


//	-----------------------------------------------------------------------------

void DrawKeys()
{
WINDOS(
	DDGrSetCurrentCanvas( GetCurrentGameScreen()),
	GrSetCurrentCanvas( GetCurrentGameScreen())
);

WIN(DDGRLOCK(dd_grd_curcanv));
	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_BLUE_KEY )	{
		PAGE_IN_GAUGE( GAUGE_BLUE_KEY );
		/*GrUBitmapM*/HUDBitBlt( GAUGE_BLUE_KEY_X, GAUGE_BLUE_KEY_Y, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_BLUE_KEY), F1_0, 0);
	} else {
		PAGE_IN_GAUGE( GAUGE_BLUE_KEY_OFF );
		/*GrUBitmapM*/HUDBitBlt( GAUGE_BLUE_KEY_X, GAUGE_BLUE_KEY_Y, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_BLUE_KEY_OFF), F1_0, 0);
	}

	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_GOLD_KEY)	{
		PAGE_IN_GAUGE( GAUGE_GOLD_KEY );
		/*GrUBitmapM*/HUDBitBlt( GAUGE_GOLD_KEY_X, GAUGE_GOLD_KEY_Y, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_GOLD_KEY), F1_0, 0);
	} else {
		PAGE_IN_GAUGE( GAUGE_GOLD_KEY_OFF );
		/*GrUBitmapM*/HUDBitBlt( GAUGE_GOLD_KEY_X, GAUGE_GOLD_KEY_Y, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_GOLD_KEY_OFF), F1_0, 0);
	}

	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_RED_KEY)	{
		PAGE_IN_GAUGE( GAUGE_RED_KEY );
		/*GrUBitmapM*/HUDBitBlt( GAUGE_RED_KEY_X,  GAUGE_RED_KEY_Y, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_RED_KEY), F1_0, 0);
	} else {
		PAGE_IN_GAUGE( GAUGE_RED_KEY_OFF );
		/*GrUBitmapM*/HUDBitBlt( GAUGE_RED_KEY_X,  GAUGE_RED_KEY_Y,gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(GAUGE_RED_KEY_OFF), F1_0, 0);
	}
WIN(DDGRUNLOCK(dd_grd_curcanv));
}

//	-----------------------------------------------------------------------------

void DrawWeaponInfoSub(int info_index,gauge_box *box,int pic_x,int pic_y,char *name,int text_x,
								  int text_y, int orient)
{
	grs_bitmap *bmP;
	char *p;

	//clear the window
// PA_DFX (pa_set_frontbuffer_current();
//	PA_DFX (GrRect(box->left,box->top,box->right,box->bot);
	PA_DFX (pa_set_backbuffer_current());
	if (gameStates.render.cockpit.nMode != CM_FULL_SCREEN) {
		GrSetColorRGBi (RGBA_PAL (0,0,0));
		GrRect((int) (box->left * cmScaleX), (int) (box->top * cmScaleY), 
				  (int) (box->right * cmScaleX), (int) (box->bot * cmScaleY));
		}
	if ((gameData.pig.tex.nHamFileVersion >= 3) // !SHAREWARE
		&& gameStates.video.nDisplayMode) {
		bmP=gameData.pig.tex.bitmaps [0] + gameData.weapons.info [info_index].hires_picture.index;
		PIGGY_PAGE_IN(gameData.weapons.info[info_index].hires_picture, 0);
		}
	else {
		bmP=gameData.pig.tex.bitmaps [0] + gameData.weapons.info [info_index].picture.index;
		PIGGY_PAGE_IN(gameData.weapons.info[info_index].picture, 0);
		}
	Assert(bmP != NULL);

//   PA_DFX (pa_set_frontbuffer_current();
//	PA_DFX (GrUBitmapM(pic_x,pic_y,bmP);
   PA_DFX (pa_set_backbuffer_current());
//	GrUBitmapM (pic_x, pic_y, bmP);
	HUDBitBlt (pic_x, pic_y, bmP, (gameStates.render.cockpit.nMode == CM_FULL_SCREEN) ? 2 * F1_0 : F1_0, orient);
	if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)
		return;
	GrSetFontColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
	if ((p=strchr(name,'\n'))!=NULL) {
		*p=0;
	   #ifdef PA_3DFX_VOODOO
  //			pa_set_frontbuffer_current();
//			GrPrintF(text_x,text_y,name);
//			GrPrintF(text_x,text_y+grdCurCanv->cv_font->ft_h+1,p+1);
		#endif
		PA_DFX (pa_set_backbuffer_current());
		HUDPrintF(text_x,text_y,name);
		HUDPrintF(text_x,text_y+grdCurCanv->cv_font->ft_h+1,p+1);
		*p='\n';
	} else
	 {
  //		PA_DFX(pa_set_frontbuffer_current();
//		PA_DFX (GrPrintF(text_x,text_y,name);
		PA_DFX(pa_set_backbuffer_current());
		HUDPrintF(text_x,text_y,name);
	 }	

	//	For laser, show level and quadness
	if (info_index == LASER_ID || info_index == SUPER_LASER_ID) {
		char	temp_str[7];

		sprintf(temp_str, "%s: 0", TXT_LVL);

		temp_str[5] = gameData.multi.players[gameData.multi.nLocalPlayer].laser_level+1 + '0';

//		PA_DFX(pa_set_frontbuffer_current();
//		PA_DFX (GrPrintF(text_x,text_y+Line_spacing, temp_str);
		PA_DFX(pa_set_backbuffer_current());
		NO_DFX (HUDPrintF(text_x,text_y+Line_spacing, temp_str);)
		PA_DFX (HUDPrintF(text_x,text_y+12, temp_str));

		if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_QUAD_LASERS) {
			strcpy(temp_str, TXT_QUAD);
//			PA_DFX(pa_set_frontbuffer_current();
//			PA_DFX (GrPrintF(text_x,text_y+2*Line_spacing, temp_str);
			PA_DFX(pa_set_backbuffer_current());
			HUDPrintF(text_x,text_y+2*Line_spacing, temp_str);
		}
	}
}

//	-----------------------------------------------------------------------------

void DrawWeaponInfo(int weapon_type,int weapon_num,int laser_level)
{
	int info_index;

	if (weapon_type == 0) {
		info_index = primaryWeaponToWeaponInfo[weapon_num];

		if (info_index == LASER_ID && laser_level > MAX_LASER_LEVEL)
			info_index = SUPER_LASER_ID;

		if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
			DrawWeaponInfoSub(info_index,
				gauge_boxes + SB_PRIMARY_BOX,
				SB_PRIMARY_W_PIC_X,SB_PRIMARY_W_PIC_Y,
				PRIMARY_WEAPON_NAMES_SHORT(weapon_num),
				SB_PRIMARY_W_TEXT_X,SB_PRIMARY_W_TEXT_Y, 0);
#if 0
		else if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)
			DrawWeaponInfoSub(info_index,
				gauge_boxes + SB_PRIMARY_BOX,
				SB_PRIMARY_W_PIC_X,SB_PRIMARY_W_PIC_Y,
				PRIMARY_WEAPON_NAMES_SHORT(weapon_num),
				SB_PRIMARY_W_TEXT_X,SB_PRIMARY_W_TEXT_Y, 3);
#endif
		else
			DrawWeaponInfoSub(info_index,
				gauge_boxes + COCKPIT_PRIMARY_BOX,
				PRIMARY_W_PIC_X,PRIMARY_W_PIC_Y,
				PRIMARY_WEAPON_NAMES_SHORT(weapon_num),
				PRIMARY_W_TEXT_X,PRIMARY_W_TEXT_Y, 0);

		}
	else {
		info_index = secondaryWeaponToWeaponInfo[weapon_num];

		if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
			DrawWeaponInfoSub(info_index,
				gauge_boxes + SB_SECONDARY_BOX,
				SB_SECONDARY_W_PIC_X,SB_SECONDARY_W_PIC_Y,
				SECONDARY_WEAPON_NAMES_SHORT(weapon_num),
				SB_SECONDARY_W_TEXT_X,SB_SECONDARY_W_TEXT_Y, 0);
#if 0
		else if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)
			DrawWeaponInfoSub(info_index,
				gauge_boxes + COCKPIT_SECONDARY_BOX,
				SECONDARY_W_PIC_X,SECONDARY_W_PIC_Y,
				SECONDARY_WEAPON_NAMES_SHORT(weapon_num),
				SECONDARY_W_TEXT_X,SECONDARY_W_TEXT_Y, 1);
#endif
		else
			DrawWeaponInfoSub(info_index,
				gauge_boxes + COCKPIT_SECONDARY_BOX,
				SECONDARY_W_PIC_X,SECONDARY_W_PIC_Y,
				SECONDARY_WEAPON_NAMES_SHORT(weapon_num),
				SECONDARY_W_TEXT_X,SECONDARY_W_TEXT_Y, 0);
	}
}

//	-----------------------------------------------------------------------------

void DrawAmmoInfo(int x,int y,int ammo_count,int primary)
{
	int w;
	char str[16];

	if (primary)
		w = (grdCurCanv->cv_font->ft_w*7)/2;
	else
		w = (grdCurCanv->cv_font->ft_w*5)/2;

WIN(DDGRLOCK(dd_grd_curcanv));
{

	PA_DFX (pa_set_frontbuffer_current());

	GrSetColorRGBi (RGBA_PAL (0,0,0));
	GrRect(HUD_SCALE_X (x), HUD_SCALE_Y (y), HUD_SCALE_X (x+w), HUD_SCALE_Y (y + grdCurCanv->cv_font->ft_h));
	GrSetFontColorRGBi (MEDRED_RGBA, 1, 0, 0);
	sprintf(str,"%03d",ammo_count);
	convert_1s(str);
	HUDPrintF(x,y,str);

	PA_DFX (pa_set_backbuffer_current());
	GrRect(HUD_SCALE_X (x), HUD_SCALE_Y (y), HUD_SCALE_X (x+w), HUD_SCALE_Y (y + grdCurCanv->cv_font->ft_h));
	HUDPrintF(x,y,str);
}

WIN(DDGRUNLOCK(dd_grd_curcanv));
}

//	-----------------------------------------------------------------------------

void DrawSecondaryAmmoInfo(int ammo_count)
{
	if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
		DrawAmmoInfo(SB_SECONDARY_AMMO_X,SB_SECONDARY_AMMO_Y,ammo_count,0);
	else
		DrawAmmoInfo(SECONDARY_AMMO_X,SECONDARY_AMMO_Y,ammo_count,0);
}

//	-----------------------------------------------------------------------------

//returns true if drew picture
int DrawWeaponBox(int weapon_type,int weapon_num)
{
	int drew_flag=0;
	int laser_level_changed;

WINDOS(
	DDGrSetCurrentCanvas(&dd_VR_render_buffer[0]),
	GrSetCurrentCanvas(&VR_render_buffer[0])
);

   PA_DFX (pa_set_backbuffer_current());
 
WIN(DDGRLOCK(dd_grd_curcanv));
	GrSetCurFont( GAME_FONT );

	laser_level_changed = (weapon_type==0 && weapon_num==LASER_INDEX && (gameData.multi.players[gameData.multi.nLocalPlayer].laser_level != old_laser_level[VR_current_page]));

	if ((weapon_num != old_weapon[weapon_type][VR_current_page] || laser_level_changed) && weapon_box_states[weapon_type] == WS_SET) {
		weapon_box_states[weapon_type] = WS_FADING_OUT;
		weapon_box_fade_values[weapon_type]=i2f(GR_ACTUAL_FADE_LEVELS-1);
		}
		
	if ((old_weapon[weapon_type][VR_current_page] == -1) || 1/*(gameStates.render.cockpit.nMode == CM_FULL_COCKPIT)*/) {
		DrawWeaponInfo(weapon_type,weapon_num,gameData.multi.players[gameData.multi.nLocalPlayer].laser_level);
		old_weapon[weapon_type][VR_current_page] = weapon_num;
		old_ammo_count[weapon_type][VR_current_page]=-1;
		Old_Omega_charge[VR_current_page]=-1;
		old_laser_level[VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].laser_level;
		drew_flag=1;
		weapon_box_states[weapon_type] = WS_SET;
		}

	if (weapon_box_states[weapon_type] == WS_FADING_OUT) {
		DrawWeaponInfo(weapon_type,old_weapon[weapon_type][VR_current_page],old_laser_level[VR_current_page]);
		old_ammo_count[weapon_type][VR_current_page]=-1;
		Old_Omega_charge[VR_current_page]=-1;
		drew_flag=1;
		weapon_box_fade_values[weapon_type] -= gameData.app.xFrameTime * FADE_SCALE;
		if (weapon_box_fade_values[weapon_type] <= 0) {
			weapon_box_states[weapon_type] = WS_FADING_IN;
			old_weapon[weapon_type][VR_current_page] = weapon_num;
			old_weapon[weapon_type][!VR_current_page] = weapon_num;
			old_laser_level[VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].laser_level;
			old_laser_level[!VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].laser_level;
			weapon_box_fade_values[weapon_type] = 0;
			}
		}
	else if (weapon_box_states[weapon_type] == WS_FADING_IN) {
		if (weapon_num != old_weapon[weapon_type][VR_current_page]) {
			weapon_box_states[weapon_type] = WS_FADING_OUT;
			}
		else {
			DrawWeaponInfo(weapon_type,weapon_num,gameData.multi.players[gameData.multi.nLocalPlayer].laser_level);
			old_ammo_count[weapon_type][VR_current_page]=-1;
			Old_Omega_charge[VR_current_page]=-1;
			drew_flag=1;
			weapon_box_fade_values[weapon_type] += gameData.app.xFrameTime * FADE_SCALE;
			if (weapon_box_fade_values[weapon_type] >= i2f(GR_ACTUAL_FADE_LEVELS-1)) {
				weapon_box_states[weapon_type] = WS_SET;
				old_weapon[weapon_type][!VR_current_page] = -1;		//force redraw (at full fade-in) of other page
				}
			}
		}

	if (weapon_box_states[weapon_type] != WS_SET) {		//fade gauge
		int fade_value = f2i(weapon_box_fade_values[weapon_type]);
		int boxofs = (gameStates.render.cockpit.nMode==CM_STATUS_BAR)?SB_PRIMARY_BOX:COCKPIT_PRIMARY_BOX;
		
		gameStates.render.grAlpha = (float) fade_value;
//	   PA_DFX (pa_set_frontbuffer_current();
//		PA_DFX (GrRect(gauge_boxes[boxofs+weapon_type].left,gauge_boxes[boxofs+weapon_type].top,gauge_boxes[boxofs+weapon_type].right,gauge_boxes[boxofs+weapon_type].bot);
	   PA_DFX (pa_set_backbuffer_current());
		GrRect(gauge_boxes[boxofs+weapon_type].left,gauge_boxes[boxofs+weapon_type].top,gauge_boxes[boxofs+weapon_type].right,gauge_boxes[boxofs+weapon_type].bot);

		gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
		}
WIN(DDGRUNLOCK(dd_grd_curcanv));

WINDOS(
	DDGrSetCurrentCanvas(GetCurrentGameScreen()),
	GrSetCurrentCanvas(GetCurrentGameScreen())
);
	return drew_flag;
}

//	-----------------------------------------------------------------------------

fix static_time[2];

void DrawStatic(int win)
{
	vclip *vc = gameData.eff.vClips [0] + VCLIP_MONITOR_STATIC;
	grs_bitmap *bmp;
	int framenum;
	int boxofs = (gameStates.render.cockpit.nMode==CM_STATUS_BAR)?SB_PRIMARY_BOX:COCKPIT_PRIMARY_BOX;
	int x,y;

	static_time[win] += gameData.app.xFrameTime;
	if (static_time[win] >= vc->xTotalTime) {
		weapon_box_user[win] = WBU_WEAPON;
		return;
	}
	framenum = static_time[win] * vc->nFrameCount / vc->xTotalTime;
	PIGGY_PAGE_IN(vc->frames[framenum], 0);
	bmp = gameData.pig.tex.bitmaps [0] + vc->frames[framenum].index;
	WINDOS(
	DDGrSetCurrentCanvas(&dd_VR_render_buffer[0]),
	GrSetCurrentCanvas(&VR_render_buffer[0])
	);
	WIN(DDGRLOCK(dd_grd_curcanv));
   PA_DFX (pa_set_backbuffer_current());
 	PA_DFX (pa_bypass_mode (0));
	PA_DFX (pa_clip_window (gauge_boxes[boxofs+win].left,gauge_boxes[boxofs+win].top,
									gauge_boxes[boxofs+win].right,gauge_boxes[boxofs+win].bot));
   
	for (x=gauge_boxes[boxofs+win].left;x<gauge_boxes[boxofs+win].right;x+=bmp->bm_props.w)
		for (y=gauge_boxes[boxofs+win].top;y<gauge_boxes[boxofs+win].bot;y+=bmp->bm_props.h)
			/*GrBitmap*/HUDBitBlt(x,y,bmp, F1_0, 0);

 	PA_DFX (pa_bypass_mode(1));
	PA_DFX (pa_clip_window (0,0,640,480));

	WIN(DDGRUNLOCK(dd_grd_curcanv));

	WINDOS(
	DDGrSetCurrentCanvas(GetCurrentGameScreen()),
	GrSetCurrentCanvas(GetCurrentGameScreen())
	);

//   PA_DFX (return);
  
	WINDOS(
	CopyGaugeBox(&gauge_boxes[boxofs+win],&dd_VR_render_buffer[0]),
	CopyGaugeBox(&gauge_boxes[boxofs+win],&VR_render_buffer[0].cv_bitmap)
	);
}

//	-----------------------------------------------------------------------------

void DrawWeaponBoxes()
{
	int boxofs = (gameStates.render.cockpit.nMode==CM_STATUS_BAR)?SB_PRIMARY_BOX:COCKPIT_PRIMARY_BOX;
	int drew;

	if (weapon_box_user[0] == WBU_WEAPON) {
		drew = DrawWeaponBox(0,gameData.weapons.nPrimary);
		if (drew) 
			WINDOS(
				CopyGaugeBox(gauge_boxes+boxofs,dd_VR_render_buffer),
				CopyGaugeBox(gauge_boxes+boxofs,&VR_render_buffer[0].cv_bitmap)
			);

		if (weapon_box_states[0] == WS_SET) {
			if ((gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX)) {
				if (gameData.multi.players[gameData.multi.nLocalPlayer].primary_ammo[VULCAN_INDEX] != old_ammo_count[0][VR_current_page]) {
					if (gameData.demo.nState == ND_STATE_RECORDING)
						NDRecordPrimaryAmmo(old_ammo_count[0][VR_current_page], gameData.multi.players[gameData.multi.nLocalPlayer].primary_ammo[VULCAN_INDEX]);
					DrawPrimaryAmmoInfo(f2i((unsigned) VULCAN_AMMO_SCALE * (unsigned) gameData.multi.players[gameData.multi.nLocalPlayer].primary_ammo[VULCAN_INDEX]));
					old_ammo_count[0][VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].primary_ammo[VULCAN_INDEX];
				}
			}

			if (gameData.weapons.nPrimary == OMEGA_INDEX) {
				if (xOmegaCharge != Old_Omega_charge[VR_current_page]) {
					if (gameData.demo.nState == ND_STATE_RECORDING)
						NDRecordPrimaryAmmo(Old_Omega_charge[VR_current_page], xOmegaCharge);
					DrawPrimaryAmmoInfo(xOmegaCharge * 100/MAX_OMEGA_CHARGE);
					Old_Omega_charge[VR_current_page] = xOmegaCharge;
				}
			}
		}
	}
	else if (weapon_box_user[0] == WBU_STATIC)
		DrawStatic(0);

	if (weapon_box_user[1] == WBU_WEAPON) {
		drew = DrawWeaponBox(1,gameData.weapons.nSecondary);
		if (drew)
			WINDOS(
				CopyGaugeBox(&gauge_boxes[boxofs+1],&dd_VR_render_buffer[0]),
				CopyGaugeBox(&gauge_boxes[boxofs+1],&VR_render_buffer[0].cv_bitmap)
			);

		if (weapon_box_states[1] == WS_SET)
			if (gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[gameData.weapons.nSecondary] != old_ammo_count[1][VR_current_page]) {
				old_bombcount[VR_current_page] = 0x7fff;	//force redraw
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordSecondaryAmmo(old_ammo_count[1][VR_current_page], gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[gameData.weapons.nSecondary]);
				DrawSecondaryAmmoInfo(gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[gameData.weapons.nSecondary]);
				old_ammo_count[1][VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[gameData.weapons.nSecondary];
			}
	}
	else if (weapon_box_user[1] == WBU_STATIC)
		DrawStatic(1);
}

//	-----------------------------------------------------------------------------

void SBDrawEnergyBar(energy)
{
	int erase_height, w, h, aw;
	char energy_str[20];

//	GrSetCurrentCanvas( Canv_SBEnergyGauge );

	PAGE_IN_GAUGE( SB_GAUGE_ENERGY );
	if (gameStates.app.bD1Mission)
	HUDStretchBlt(SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, 
			         gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(SB_GAUGE_ENERGY), F1_0, 0, 
						1.0, (double) SB_ENERGY_GAUGE_H / (double) (SB_ENERGY_GAUGE_H - SB_AFTERBURNER_GAUGE_H));
	else
	/*GrUBitmapM*/HUDBitBlt(SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, 
									  gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(SB_GAUGE_ENERGY), F1_0, 0);
	erase_height = (100 - energy) * SB_ENERGY_GAUGE_H / 100;
	if (erase_height > 0) {
		GrSetColorRGBi (BLACK_RGBA);
		glDisable (GL_BLEND);
		HUDRect(
			SB_ENERGY_GAUGE_X,
			SB_ENERGY_GAUGE_Y,
			SB_ENERGY_GAUGE_X+SB_ENERGY_GAUGE_W,
			SB_ENERGY_GAUGE_Y+erase_height);
		glEnable (GL_BLEND);
	}
	WINDOS(
	DDGrSetCurrentCanvas(GetCurrentGameScreen()),
	GrSetCurrentCanvas(GetCurrentGameScreen())
	);
#if 0
	WIN(DDGRLOCK(dd_grd_curcanv));
   PA_DFX (pa_set_frontbuffer_current());
   PA_DFX (/*GrUBitmapM*/HUDBitBlt( SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, &Canv_SBEnergyGauge->cv_bitmap), F1_0, 0);
   PA_DFX (pa_set_backbuffer_current());
	/*GrUBitmapM*/HUDBitBlt( SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, &Canv_SBEnergyGauge->cv_bitmap, F1_0, 0 );
#endif
	//draw numbers
	sprintf(energy_str, "%d", energy);
	GrGetStringSize(energy_str, &w, &h, &aw );
	GrSetFontColorRGBi (RGBA_PAL2 (25, 18, 6), 1, 0, 0);
   PA_DFX (pa_set_frontbuffer_current());
	PA_DFX (HUDPrintF(SB_ENERGY_GAUGE_X + ((SB_ENERGY_GAUGE_W - w)/2), SB_ENERGY_GAUGE_Y + SB_ENERGY_GAUGE_H - GAME_FONT->ft_h - (GAME_FONT->ft_h / 4), "%d", energy));
   PA_DFX (pa_set_backbuffer_current());
	HUDPrintF(SB_ENERGY_GAUGE_X + ((SB_ENERGY_GAUGE_W - w)/2), SB_ENERGY_GAUGE_Y + SB_ENERGY_GAUGE_H - GAME_FONT->ft_h - (GAME_FONT->ft_h / 4), "%d", energy);
	WIN(DDGRUNLOCK(dd_grd_curcanv));					  
	OglFreeBmTexture(&Canv_SBEnergyGauge->cv_bitmap );
}

//	-----------------------------------------------------------------------------

void SBDrawAfterburner()
{
	int erase_height, w, h, aw;
	char ab_str[3] = "AB";
//	static int b = 1;

	if (gameStates.app.bD1Mission)
		return;
//	GrSetCurrentCanvas( Canv_SBAfterburnerGauge );
	PAGE_IN_GAUGE( SB_GAUGE_AFTERBURNER );
	/*GrUBitmapM*/HUDBitBlt (SB_AFTERBURNER_GAUGE_X, SB_AFTERBURNER_GAUGE_Y, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(SB_GAUGE_AFTERBURNER), F1_0, 0);
	erase_height = FixMul((f1_0 - xAfterburnerCharge),SB_AFTERBURNER_GAUGE_H);
//	HUDMessage (0, "AB: %d", erase_height);

	if (erase_height > 0) {
		GrSetColorRGBi (BLACK_RGBA);
		glDisable (GL_BLEND);
		HUDRect(
			SB_AFTERBURNER_GAUGE_X,
			SB_AFTERBURNER_GAUGE_Y,
			SB_AFTERBURNER_GAUGE_X+SB_AFTERBURNER_GAUGE_W-1,
			SB_AFTERBURNER_GAUGE_Y+erase_height-1);
		glEnable (GL_BLEND);
	}

WINDOS(
	DDGrSetCurrentCanvas(GetCurrentGameScreen()),
	GrSetCurrentCanvas(GetCurrentGameScreen())
);
WIN(DDGRLOCK(dd_grd_curcanv));
#if 0
   PA_DFX (pa_set_frontbuffer_current());
	PA_DFX (/*GrUBitmapM*/HUDBitBlt( SB_AFTERBURNER_GAUGE_X, SB_AFTERBURNER_GAUGE_Y, &Canv_SBAfterburnerGauge->cv_bitmap, F1_0, 0 ));
   PA_DFX (pa_set_backbuffer_current());
	/*GrUBitmapM*/HUDBitBlt( SB_AFTERBURNER_GAUGE_X, SB_AFTERBURNER_GAUGE_Y, &Canv_SBAfterburnerGauge->cv_bitmap, F1_0, 0 );
#endif
	//draw legend
	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER)
		GrSetFontColorRGBi (RGBA_PAL2 (45, 0, 0), 1, 0, 0);
	else 
		GrSetFontColorRGBi (RGBA_PAL2 (12, 12, 12), 1, 0, 0);

	GrGetStringSize(ab_str, &w, &h, &aw );
   PA_DFX (pa_set_frontbuffer_current());
	PA_DFX (HUDPrintF(SB_AFTERBURNER_GAUGE_X + ((SB_AFTERBURNER_GAUGE_W - w)/2),SB_AFTERBURNER_GAUGE_Y+SB_AFTERBURNER_GAUGE_H-GAME_FONT->ft_h - (GAME_FONT->ft_h / 4),"AB"));
   PA_DFX (pa_set_backbuffer_current());
	HUDPrintF(SB_AFTERBURNER_GAUGE_X + ((SB_AFTERBURNER_GAUGE_W - w)/2),SB_AFTERBURNER_GAUGE_Y+SB_AFTERBURNER_GAUGE_H-GAME_FONT->ft_h - (GAME_FONT->ft_h / 4),"AB");

WIN(DDGRUNLOCK(dd_grd_curcanv));					  
OglFreeBmTexture(&Canv_SBAfterburnerGauge->cv_bitmap );
}

//	-----------------------------------------------------------------------------

void SBDrawShieldNum(int shield)
{
	//draw numbers

	GrSetCurFont( GAME_FONT );
	GrSetFontColorRGBi (RGBA_PAL2 (14, 14, 23), 1, 0, 0);

	//erase old one
	PIGGY_PAGE_IN( cockpit_bitmap[gameStates.render.cockpit.nMode+(gameStates.video.nDisplayMode?(Num_cockpits/2):0)], 0 );

WIN(DDGRLOCK(dd_grd_curcanv));
   PA_DFX (pa_set_back_to_read());
//	GrSetColor(gr_gpixel(&grdCurCanv->cv_bitmap,HUD_SCALE_X (SB_SHIELD_NUM_X)-1,HUD_SCALE_Y (SB_SHIELD_NUM_Y)-1));
   PA_DFX (pa_set_front_to_read());

	PA_DFX (pa_set_frontbuffer_current());

	HUDRect(SB_SHIELD_NUM_X,SB_SHIELD_NUM_Y,SB_SHIELD_NUM_X+(gameStates.video.nDisplayMode?27:13),SB_SHIELD_NUM_Y+GAME_FONT->ft_h);
	HUDPrintF((shield>99)?SB_SHIELD_NUM_X:((shield>9)?SB_SHIELD_NUM_X+2:SB_SHIELD_NUM_X+4),SB_SHIELD_NUM_Y,"%d",shield);

  	PA_DFX (pa_set_backbuffer_current());
	PA_DFX (HUDRect(SB_SHIELD_NUM_X,SB_SHIELD_NUM_Y,SB_SHIELD_NUM_X+(gameStates.video.nDisplayMode?27:13),SB_SHIELD_NUM_Y+GAME_FONT->ft_h));
	PA_DFX (HUDPrintF((shield>99)?SB_SHIELD_NUM_X:((shield>9)?SB_SHIELD_NUM_X+2:SB_SHIELD_NUM_X+4),SB_SHIELD_NUM_Y,"%d",shield));

WIN(DDGRUNLOCK(dd_grd_curcanv));
}

//	-----------------------------------------------------------------------------

void SBDrawShieldBar(int shield)
{
	int bm_num = shield>=100?9:(shield / 10);

WINDOS(
	DDGrSetCurrentCanvas(GetCurrentGameScreen()),
	GrSetCurrentCanvas(GetCurrentGameScreen())
);
WIN(DDGRLOCK(dd_grd_curcanv));
	PAGE_IN_GAUGE( GAUGE_SHIELDS+9-bm_num );
   PA_DFX (pa_set_frontbuffer_current());		
	/*GrUBitmapM*/HUDBitBlt( SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y, &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(GAUGE_SHIELDS+9-bm_num)], F1_0, 0 );
   PA_DFX (pa_set_backbuffer_current());		
	PA_DFX (/*GrUBitmapM*/HUDBitBlt( SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y, &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(GAUGE_SHIELDS+9-bm_num)], F1_0, 0));
	
WIN(DDGRUNLOCK(dd_grd_curcanv));					  
}

//	-----------------------------------------------------------------------------

void SBDrawKeys()
{
	grs_bitmap * bm;
	int flags = gameData.multi.players[gameData.multi.nLocalPlayer].flags;

WINDOS(
	DDGrSetCurrentCanvas(GetCurrentGameScreen()),
	GrSetCurrentCanvas(GetCurrentGameScreen())
);
WIN(DDGRLOCK(dd_grd_curcanv));
   PA_DFX (pa_set_frontbuffer_current());
	bm = &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX((flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF)];
	PAGE_IN_GAUGE((flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF );
	/*GrUBitmapM*/HUDBitBlt( SB_GAUGE_KEYS_X, SB_GAUGE_BLUE_KEY_Y, bm, F1_0, 0 );
	bm = &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX((flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF)];
	PAGE_IN_GAUGE((flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF );
	/*GrUBitmapM*/HUDBitBlt( SB_GAUGE_KEYS_X, SB_GAUGE_GOLD_KEY_Y, bm, F1_0, 0 );
	bm = &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX((flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF)];
	PAGE_IN_GAUGE((flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF );
	/*GrUBitmapM*/HUDBitBlt( SB_GAUGE_KEYS_X, SB_GAUGE_RED_KEY_Y, bm , F1_0, 0 );
	#ifdef PA_3DFX_VOODOO
	   PA_DFX (pa_set_backbuffer_current();
		bm = &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX((flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF)];
		PAGE_IN_GAUGE((flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF );
		GrUBitmapM( SB_GAUGE_KEYS_X, SB_GAUGE_BLUE_KEY_Y, bm );
		bm = &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX((flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF)];
		PAGE_IN_GAUGE((flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF );
		GrUBitmapM( SB_GAUGE_KEYS_X, SB_GAUGE_GOLD_KEY_Y, bm );
		bm = &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX((flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF)];
		PAGE_IN_GAUGE((flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF );
		GrUBitmapM( SB_GAUGE_KEYS_X, SB_GAUGE_RED_KEY_Y, bm  );
	#endif

WIN(DDGRUNLOCK(dd_grd_curcanv));
}

//	-----------------------------------------------------------------------------

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void DrawInvulnerableShip()
{
	static fix time=0;

WINDOS(
	DDGrSetCurrentCanvas(GetCurrentGameScreen()),
	GrSetCurrentCanvas(GetCurrentGameScreen())
);
WIN(DDGRLOCK(dd_grd_curcanv));

	if (((gameData.multi.players[gameData.multi.nLocalPlayer].invulnerable_time + INVULNERABLE_TIME_MAX - gameData.app.xGameTime) > F1_0*4) || (gameData.app.xGameTime & 0x8000)) {

		if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)	{
			PAGE_IN_GAUGE( GAUGE_INVULNERABLE+invulnerable_frame );
			PA_DFX (pa_set_frontbuffer_current());
			/*GrUBitmapM*/HUDBitBlt( SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y, &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(GAUGE_INVULNERABLE+invulnerable_frame)], F1_0, 0 );
			PA_DFX (pa_set_backbuffer_current());
			PA_DFX (/*GrUBitmapM*/HUDBitBlt( SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y, &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(GAUGE_INVULNERABLE+invulnerable_frame)], F1_0, 0 ));
		} else {
			PAGE_IN_GAUGE( GAUGE_INVULNERABLE+invulnerable_frame );
			PA_DFX (pa_set_frontbuffer_current();)
			PA_DFX (/*GrUBitmapM*/HUDBitBlt( SHIELD_GAUGE_X, SHIELD_GAUGE_Y, &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(GAUGE_INVULNERABLE+invulnerable_frame)], F1_0, 0 ));
			PA_DFX (pa_set_backbuffer_current());
			/*GrUBitmapM*/HUDBitBlt( SHIELD_GAUGE_X, SHIELD_GAUGE_Y, &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX(GAUGE_INVULNERABLE+invulnerable_frame)], F1_0, 0 );
		}

		time += gameData.app.xFrameTime;

		while (time > INV_FRAME_TIME) {
			time -= INV_FRAME_TIME;
			if (++invulnerable_frame == N_INVULNERABLE_FRAMES)
				invulnerable_frame=0;
		}
	} else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
		SBDrawShieldBar(f2ir(gameData.multi.players[gameData.multi.nLocalPlayer].shields));
	else
		DrawShieldBar(f2ir(gameData.multi.players[gameData.multi.nLocalPlayer].shields));
WIN(DDGRUNLOCK(dd_grd_curcanv));
}

//	-----------------------------------------------------------------------------

extern int Missile_gun;
extern int AllowedToFireLaser(void);
extern int AllowedToFireMissile(void);

rgb player_rgb[] = {
	{15,15,23},
	{27,0,0},
	{0,23,0},
	{30,11,31},
	{31,16,0},
	{24,17,6},
	{14,21,12},
	{29,29,0}};

extern int max_window_w;

typedef struct {
	sbyte x, y;
} xy;

//offsets for reticle parts: high-big  high-sml  low-big  low-sml
xy cross_offsets[4] = 		{ {-8,-5},	{-4,-2},	{-4,-2}, {-2,-1} };
xy primary_offsets[4] = 	{ {-30,14}, {-16,6},	{-15,6}, {-8, 2} };
xy secondary_offsets[4] =	{ {-24,2},	{-12,0}, {-12,1}, {-6,-2} };

void OglDrawMouseIndicator (void);

//draw the reticle
void ShowReticle(int force_big_one)
{
	int x,y;
	int bLaserReady, bMissileReady, bLaserAmmo, bMissileAmmo;
	int nCrossBm, nPrimaryBm, nSecondaryBm;
	int bHiresReticle, bSmallReticle, ofs, nGaugeIndex;

   if ((gameData.demo.nState==ND_STATE_PLAYBACK) && gameData.demo.bFlyingGuided) {
		WIN(DDGRLOCK(dd_grd_curcanv));
		DrawGuidedCrosshair();
		WIN(DDGRUNLOCK(dd_grd_curcanv);)
		return;
	   }

	x = grdCurCanv->cv_w / 2;
	y = grdCurCanv->cv_h / ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) ? 2 : 2);
	bLaserReady = AllowedToFireLaser();
	bMissileReady = AllowedToFireMissile();
	bLaserAmmo = PlayerHasWeapon(gameData.weapons.nPrimary,0, -1);
	bMissileAmmo = PlayerHasWeapon(gameData.weapons.nSecondary,1, -1);
	nPrimaryBm = (bLaserReady && bLaserAmmo==HAS_ALL);
	nSecondaryBm = (bMissileReady && bMissileAmmo==HAS_ALL);
	if (nPrimaryBm && (gameData.weapons.nPrimary==LASER_INDEX) && 
		 (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_QUAD_LASERS))
		nPrimaryBm++;

	if (secondaryWeaponToGunNum[gameData.weapons.nSecondary]==7)
		nSecondaryBm += 3;		//now value is 0,1 or 3,4
	else if (nSecondaryBm && !(Missile_gun&1))
			nSecondaryBm++;

	nCrossBm = ((nPrimaryBm > 0) || (nSecondaryBm > 0));

	Assert(nPrimaryBm <= 2);
	Assert(nSecondaryBm <= 4);
	Assert(nCrossBm <= 1);
#ifdef _DEBUG
	if (gameStates.render.bExternalView)
#else	
	if (gameStates.render.bExternalView && (!IsMultiGame || EGI_FLAG (bEnableCheats, 0, 0)))
#endif	
		return;
#ifdef OGL
      if (gameStates.ogl.nReticle==2 || (gameStates.ogl.nReticle && grdCurCanv->cv_bitmap.bm_props.w > 320)){                
      	OglDrawReticle(nCrossBm,nPrimaryBm,nSecondaryBm);
       } else {
#endif
	bHiresReticle = (gameStates.render.fonts.bHires != 0);
	WIN(DDGRLOCK(dd_grd_curcanv));
	bSmallReticle = !(grdCurCanv->cv_bitmap.bm_props.w * 3 > max_window_w*2 || force_big_one);
	ofs = (bHiresReticle ? 0 : 2) + bSmallReticle;

	nGaugeIndex = (bSmallReticle ? SML_RETICLE_CROSS : RETICLE_CROSS) + nCrossBm;
	PAGE_IN_GAUGE (nGaugeIndex);
	/*GrUBitmapM*/HUDBitBlt(
		-(x + HUD_SCALE_X (cross_offsets [ofs].x)),
		-(y + HUD_SCALE_Y (cross_offsets [ofs].y)),
		gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX (nGaugeIndex), F1_0, 0);
	nGaugeIndex = (bSmallReticle ? SML_RETICLE_PRIMARY : RETICLE_PRIMARY) + nPrimaryBm;
	PAGE_IN_GAUGE (nGaugeIndex);
	/*GrUBitmapM*/HUDBitBlt(
		-(x + HUD_SCALE_X (primary_offsets [ofs].x)),
		-(y + HUD_SCALE_Y (primary_offsets [ofs].y)),
		gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(nGaugeIndex), F1_0, 0);
	nGaugeIndex = (bSmallReticle ? SML_RETICLE_SECONDARY : RETICLE_SECONDARY) + nSecondaryBm;
	PAGE_IN_GAUGE (nGaugeIndex);
	/*GrUBitmapM*/HUDBitBlt(
		-(x + HUD_SCALE_X (secondary_offsets [ofs].x)),
		-(y + HUD_SCALE_Y (secondary_offsets [ofs].y)),
		gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX(nGaugeIndex), F1_0, 0);
	WIN(DDGRUNLOCK(dd_grd_curcanv));
#ifdef OGL
       }
#endif
if (gameOpts->input.bJoyMouse && gameOpts->render.cockpit.bMouseIndicator)
	OglDrawMouseIndicator ();
}

//	-----------------------------------------------------------------------------

#ifdef NETWORK
void HUDShowKillList()
{
	int n_players, player_list[MAX_NUM_NET_PLAYERS];
	int n_left, i, xo = 0, x0, x1, y, save_y, fth, l;
	int t = gameStates.app.nSDLTicks;
	int bGetPing = gameStates.render.cockpit.bShowPingStats && (!networkData.tLastPingStat || (t - networkData.tLastPingStat >= 1000));
	static int faw = -1;

	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
// ugly hack since placement of netgame players and kills is based off of
// menuhires (which is always 1 for mac).  This throws off placement of
// players in pixel double mode.
if (bGetPing)
	networkData.tLastPingStat = t;
if (multiData.kills.xShowListTimer > 0) {
	multiData.kills.xShowListTimer -= gameData.app.xFrameTime;
	if (multiData.kills.xShowListTimer < 0)
		multiData.kills.bShowList = 0;
	}
GrSetCurFont( GAME_FONT );
n_players = (multiData.kills.bShowList == 3) ? 2 : MultiGetKillList(player_list);
n_left = (n_players <= 4) ? n_players : (n_players+1)/2;
//If font size changes, this code might not work right anymore 
//Assert(GAME_FONT->ft_h==5 && GAME_FONT->ft_w==7);
fth = GAME_FONT->ft_h;
x0 = LHX(1); 
x1 = (gameData.app.nGameMode & GM_MULTI_COOP) ? LHX(31) : LHX(43);
save_y = y = grdCurCanv->cv_h - n_left*(fth+1);
if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
	save_y = y -= LHX(6);
	x1 = (gameData.app.nGameMode & GM_MULTI_COOP) ? LHX(33) : LHX(43);
	}
if (gameStates.render.cockpit.bShowPingStats) {
	if (faw < 0)
		faw = get_font_total_width (GAME_FONT) / (GAME_FONT->ft_maxchar - GAME_FONT->ft_minchar + 1);
		if(multiData.kills.bShowList == 2)
			xo = faw*24;//was +25;
		else if (gameData.app.nGameMode & GM_MULTI_COOP)
			xo = faw*14;//was +30;
		else
			xo = faw*8; //was +20;
	}	
for (i = 0; i < n_players; i++) {
	int player_num;
	char name[80], teamInd [2] = {127, 0};
	int sw,sh, aw, indent =0;

	if (i>=n_left) {
		x0 = grdCurCanv->cv_w - ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) ? LHX(53) : LHX(60));
		x1 = grdCurCanv->cv_w - ((gameData.app.nGameMode & GM_MULTI_COOP) ? LHX(27) : LHX(15));
		if (gameStates.render.cockpit.bShowPingStats) {
			x0 -= xo + 6 * faw;
			x1 -= xo + 6 * faw;
			}
		if (i==n_left)
			y = save_y;
		if (netGame.KillGoal || netGame.PlayTimeAllowed) 
			x1-=LHX(18);
		}
	else if (netGame.KillGoal || netGame.PlayTimeAllowed) 
		 x1 = LHX(43) - LHX(18);
	player_num = (multiData.kills.bShowList == 3) ? i : player_list[i];
	if (multiData.kills.bShowList == 1 || multiData.kills.bShowList==2) {
		int color;

		if (gameData.multi.players[player_num].connected != 1)
			GrSetFontColorRGBi (RGBA_PAL2 (12, 12, 12), 1, 0, 0);
		else {
			if (gameData.app.nGameMode & GM_TEAM)
				color = GetTeam(player_num);
			else
				color = player_num;
			GrSetFontColorRGBi (RGBA_PAL2 (player_rgb [color].r, player_rgb [color].g, player_rgb [color].b), 1, 0, 0);
			}
		}	
	else 
		GrSetFontColorRGBi (RGBA_PAL2 (player_rgb [player_num].r, player_rgb [player_num].g, player_rgb [player_num].b), 1, 0, 0);
	if (multiData.kills.bShowList == 3) {
		if (GetTeam (gameData.multi.nLocalPlayer) == i) {
#if 0//def _DEBUG
			sprintf (name, "%c%-8s %d.%d.%d.%d:%d", 
						teamInd [0], netGame.team_name [i],
						netPlayers.players [i].network.ipx.node [0], 
						netPlayers.players [i].network.ipx.node [1], 
						netPlayers.players [i].network.ipx.node [2], 
						netPlayers.players [i].network.ipx.node [3], 
						netPlayers.players [i].network.ipx.node [5] +
						(unsigned) netPlayers.players [i].network.ipx.node [4] * 256);
#else
			sprintf (name, "%c%s", teamInd [0], netGame.team_name[i]);
#endif
			indent = 0;
			}
		else {
#if 0//def _DEBUG
			sprintf (name, "%-8s %d.%d.%d.%d:%d", 
						netGame.team_name [i],
						netPlayers.players [i].network.ipx.node [0], 
						netPlayers.players [i].network.ipx.node [1], 
						netPlayers.players [i].network.ipx.node [2], 
						netPlayers.players [i].network.ipx.node [3], 
						netPlayers.players [i].network.ipx.node [5] +
						(unsigned) netPlayers.players [i].network.ipx.node [4] * 256);
#else
			strcpy(name, netGame.team_name[i]);
#endif
			GrGetStringSize (teamInd, &indent, &sh, &aw );
			}
		}
	else
#if 0//def _DEBUG
		sprintf (name, "%-8s %d.%d.%d.%d:%d", 
					gameData.multi.players [player_num].callsign,
					netPlayers.players [player_num].network.ipx.node [0], 
					netPlayers.players [player_num].network.ipx.node [1], 
					netPlayers.players [player_num].network.ipx.node [2], 
					netPlayers.players [player_num].network.ipx.node [3], 
					netPlayers.players [player_num].network.ipx.node [5] +
					(unsigned) netPlayers.players [player_num].network.ipx.node [4] * 256);
#else
		strcpy (name, gameData.multi.players [player_num].callsign);	// Note link to above if!!
#endif
#if 0//def _DEBUG
	x1 += LHX (100);
#endif
	for (l = (int) strlen (name); l; ) {
		GrGetStringSize(name,&sw,&sh,&aw );
		if (sw <= x1 - x0 - LHX (2))
			break;
		name [--l] = '\0';
		}
	GrPrintF(x0+indent,y,"%s",name);

	if (multiData.kills.bShowList==2) {
		if (gameData.multi.players[player_num].net_killed_total+gameData.multi.players[player_num].net_kills_total<=0)
			GrPrintF (x1,y,TXT_NOT_AVAIL);
		else
			GrPrintF (x1,y,"%d%%",
				(int)((double)gameData.multi.players[player_num].net_kills_total/
						((double)gameData.multi.players[player_num].net_killed_total+
						 (double)gameData.multi.players[player_num].net_kills_total)*100.0));		
		}
	else if (multiData.kills.bShowList == 3)	
		if (gameData.app.nGameMode & GM_ENTROPY)
			GrPrintF(x1,y,"%3d [%d/%d]",multiData.kills.nTeam [i], gameData.entropy.nTeamRooms [i + 1], gameData.entropy.nTotalRooms);
		else
			GrPrintF(x1,y,"%3d",multiData.kills.nTeam [i]);
	else if (gameData.app.nGameMode & GM_MULTI_COOP)
		GrPrintF(x1,y,"%-6d",gameData.multi.players[player_num].score);
   else if (netGame.PlayTimeAllowed || netGame.KillGoal)
      GrPrintF(x1,y,"%3d(%d)",gameData.multi.players[player_num].net_kills_total,gameData.multi.players[player_num].KillGoalCount);
   else
		GrPrintF(x1,y,"%3d",gameData.multi.players[player_num].net_kills_total);
	if (gameStates.render.cockpit.bShowPingStats && (player_num != gameData.multi.nLocalPlayer)) {
		if (bGetPing)
			PingPlayer (player_num);
		if (pingStats [player_num].sent) {
#if 0//def _DEBUG
			GrPrintF (x1 + xo, y, "%lu %d %d", 
						  pingStats [player_num].ping, 
						  pingStats [player_num].sent,
						  pingStats [player_num].received);
#else
			GrPrintF(x1 + xo, y, "%lu %i%%", pingStats [player_num].ping, 
						 100 - ((pingStats [player_num].received * 100) / pingStats [player_num].sent));
#endif
			}
		}
	y += fth+1;
	}
}
#endif

//	-----------------------------------------------------------------------------

#ifndef RELEASE
extern int Saving_movie_frames;
#else
#define Saving_movie_frames 0
#endif

//returns true if viewer can see object
int CanSeeObject(int objnum, int bCheckObjs)
{
	fvi_query fq;
	int nHitType;
	fvi_info hit_data;

	//see if we can see this player

	fq.p0 = &gameData.objs.viewer->pos;
	fq.p1 = &gameData.objs.objects[objnum].pos;
	fq.rad = 0;
	fq.thisObjNum = gameStates.render.cameras.bActive ? -1 : OBJ_IDX (gameData.objs.viewer);
	fq.flags = bCheckObjs ? FQ_CHECK_OBJS | FQ_TRANSWALL : FQ_TRANSWALL;
	fq.startSeg = gameData.objs.viewer->segnum;
	fq.ignoreObjList = NULL;
	nHitType = FindVectorIntersection(&fq, &hit_data);
	return bCheckObjs ? (nHitType == HIT_OBJECT) && (hit_data.hit.nObject == objnum) : (nHitType != HIT_WALL);
}

//	-----------------------------------------------------------------------------

#ifdef NETWORK
//show names of teammates & players carrying flags
void ShowHUDNames()
{
	int bHasFlag, bShowName, bShowTeamNames, bShowAllNames, bShowFlags, nObject, nTeam;
	int p;
	rgb *colorP;
	static int nColor = 1, tColorChange = 0;
	static rgb typingColors [2] = {
		{63, 0, 0},
		{63, 63, 0}
	};
	char s[CALLSIGN_LEN+10];
	int w, h, aw;
	int x1, y1;
	int color_num;

bShowAllNames = ((gameData.demo.nState == ND_STATE_PLAYBACK) || 
						(netGame.ShowAllNames && multiData.bShowReticleName));
bShowTeamNames = (multiData.bShowReticleName &&
						((gameData.app.nGameMode & GM_MULTI_COOP) || (gameData.app.nGameMode & GM_TEAM)));
bShowFlags = (gameData.app.nGameMode & (GM_CAPTURE | GM_HOARD | GM_ENTROPY));

nTeam = GetTeam(gameData.multi.nLocalPlayer);
for (p = 0; p < gameData.multi.nPlayers; p++) {	//check all players

	bShowName = (gameStates.multi.bPlayerIsTyping [p] || (bShowAllNames && !(gameData.multi.players[p].flags & PLAYER_FLAGS_CLOAKED)) || (bShowTeamNames && GetTeam(p)==nTeam));
	bHasFlag = (gameData.multi.players[p].connected && gameData.multi.players[p].flags & PLAYER_FLAGS_FLAG);

	if (gameData.demo.nState != ND_STATE_PLAYBACK) 
		nObject = gameData.multi.players [p].objnum;
	else {
		//if this is a demo, the nObject in the player struct is wrong,
		//so we search the object list for the nObject
		for (nObject = 0;nObject <= gameData.objs.nLastObject; nObject++)
			if (gameData.objs.objects[nObject].type==OBJ_PLAYER && 
				 gameData.objs.objects[nObject].id == p)
				break;
		if (nObject > gameData.objs.nLastObject)		//not in list, thus not visible
			bShowName = !bHasFlag;				//..so don't show name
		}

	if ((bShowName || bHasFlag) && CanSeeObject (nObject, 1)) {
		g3s_point vPlayerPos;
		G3TransformAndEncodePoint (&vPlayerPos,&gameData.objs.objects[nObject].pos);
		if (vPlayerPos.p3_codes == 0) {	//on screen
			G3ProjectPoint (&vPlayerPos);
			if (!(vPlayerPos.p3_flags & PF_OVERFLOW)) {
				fix x = vPlayerPos.p3_sx;
				fix y = vPlayerPos.p3_sy;
				if (bShowName) {				// Draw callsign on HUD
					if (gameStates.multi.bPlayerIsTyping [p]) {
						int t = gameStates.app.nSDLTicks;

						if (t - tColorChange > 333) {
							tColorChange = t;
							nColor = !nColor;
							}
						colorP = typingColors + nColor;
						}
					else {
						color_num = (gameData.app.nGameMode & GM_TEAM)? GetTeam (p) : p;
						colorP = player_rgb + color_num;
						}

					sprintf(s, "%s", gameStates.multi.bPlayerIsTyping [p] ? TXT_TYPING : gameData.multi.players[p].callsign);
					GrGetStringSize(s, &w, &h, &aw );
					GrSetFontColorRGBi (RGBA_PAL2 (colorP->r, colorP->g, colorP->b), 1, 0, 0);
					x1 = f2i(x)-w/2;
					y1 = f2i(y)-h/2;
					GrString (x1, y1, s);
				}
	
				if (bHasFlag && !(EGI_FLAG (bTargetIndicators, 0, 0) || EGI_FLAG (bTowFlags, 0, 0))) { // Draw box on HUD
					fix dy = -FixMulDiv(FixMul(gameData.objs.objects[nObject].size,viewInfo.scale.y),i2f(grdCurCanv->cv_h)/2,vPlayerPos.p3_z);
					fix dx = FixMul(dy,grdCurScreen->sc_aspect);
					fix w = dx/4;
					fix h = dy/4;
					if (gameData.app.nGameMode & (GM_CAPTURE | GM_ENTROPY))
						GrSetColorRGBi ((GetTeam(p) == TEAM_BLUE) ? MEDRED_RGBA :  MEDBLUE_RGBA);
					else if (gameData.app.nGameMode & GM_HOARD) {
						if (gameData.app.nGameMode & GM_TEAM)
							GrSetColorRGBi ((GetTeam(p) == TEAM_RED) ? MEDRED_RGBA :  MEDBLUE_RGBA);
						else
							GrSetColorRGBi (MEDGREEN_RGBA);
						}
					GrLine (x+dx-w,y-dy,x+dx,y-dy);
					GrLine (x+dx,y-dy,x+dx,y-dy+h);
					GrLine (x-dx,y-dy,x-dx+w,y-dy);
					GrLine (x-dx,y-dy,x-dx,y-dy+h);
					GrLine (x+dx-w,y+dy,x+dx,y+dy);
					GrLine (x+dx,y+dy,x+dx,y+dy-h);
					GrLine (x-dx,y+dy,x-dx+w,y+dy);
					GrLine (x-dx,y+dy,x-dx,y+dy-h);
					}
				}
			}
		}
	}
}
#endif

//	-----------------------------------------------------------------------------

extern int last_drawn_cockpit[2];

//draw all the things on the HUD
void DrawHUD()
{

	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
#ifdef OGL
   if (gameStates.render.cockpit.nMode==CM_STATUS_BAR){
   //ogl needs to redraw every frame, at least currently.
   //	InitCockpit();
	last_drawn_cockpit[0]=-1;
	last_drawn_cockpit[1]=-1;
   InitGauges();
	//	vr_reset_display();
    }
#endif
	cmScaleX = (grdCurScreen->sc_w <= 640) ? 1 : (double) grdCurScreen->sc_w / 640.0;
	cmScaleY = (grdCurScreen->sc_h <= 480) ? 1 : (double) grdCurScreen->sc_h / 480.0;
                                          
	Line_spacing = GAME_FONT->ft_h + GAME_FONT->ft_h/4;

WIN(DDGRLOCK(dd_grd_curcanv));
	//	Show score so long as not in rearview
	if (!(gameStates.render.bRearView || Saving_movie_frames || (gameStates.render.cockpit.nMode == CM_REAR_VIEW) || (gameStates.render.cockpit.nMode == CM_STATUS_BAR))) {
		HUDShowScore();
		if (score_time)
			HUDShowScoreAdded();
	}

	if (!(gameStates.render.bRearView || Saving_movie_frames || (gameStates.render.cockpit.nMode == CM_REAR_VIEW)))
	 HUDShowTimerCount();

	//	Show other stuff if not in rearview or letterbox.
	if (!(gameStates.render.bRearView || (gameStates.render.cockpit.nMode == CM_REAR_VIEW))) { // && gameStates.render.cockpit.nMode!=CM_LETTERBOX) {
		if (gameStates.render.cockpit.nMode==CM_STATUS_BAR || gameStates.render.cockpit.nMode==CM_FULL_SCREEN)
			HUDShowHomingWarning();

		if ((gameStates.render.cockpit.nMode==CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode==CM_LETTERBOX)) {
			if (!gameOpts->render.cockpit.bTextGauges) {
				if (gameOpts->render.cockpit.bScaleGauges) {
					xScale = (double) grdCurCanv->cv_h / 480.0;
					yScale = (double) grdCurCanv->cv_h / 640.0;
					}
				else
					xScale = yScale = 1;
				}
			HUDShowEnergy();
			HUDShowShield();
			HUDShowAfterburner();
			HUDShowWeapons();
			if (!Saving_movie_frames)
				HUDShowKeys();
			HUDShowCloakInvul();

			if ((gameData.demo.nState==ND_STATE_RECORDING) && (gameData.multi.players[gameData.multi.nLocalPlayer].flags != old_flags[VR_current_page])) {
				NDRecordPlayerFlags(old_flags[VR_current_page], gameData.multi.players[gameData.multi.nLocalPlayer].flags);
				old_flags[VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].flags;
			}
		}
		#ifdef NETWORK
		#ifndef RELEASE
		if (!(gameData.app.nGameMode&GM_MULTI && multiData.kills.bShowList) && !Saving_movie_frames)
			ShowTime();
		#endif
		#endif
		if (gameOpts->render.cockpit.bReticle && !gameStates.app.bPlayerIsDead /*&& gameStates.render.cockpit.nMode != CM_LETTERBOX*/ && (!bUsePlayerHeadAngles))
			ShowReticle(0);

#ifdef NETWORK
		ShowHUDNames();

		if (/*gameStates.render.cockpit.nMode != CM_LETTERBOX &&*/ gameStates.render.cockpit.nMode != CM_REAR_VIEW)
			HUDShowFlag();

		if (/*gameStates.render.cockpit.nMode != CM_LETTERBOX &&*/ gameStates.render.cockpit.nMode != CM_REAR_VIEW)
			HUDShowOrbs();

#endif
		if (!Saving_movie_frames)
			HUDRenderMessageFrame();

		if (gameStates.render.cockpit.nMode!=CM_STATUS_BAR && !Saving_movie_frames)
			HUDShowLives();

		#ifdef NETWORK
		if (gameData.app.nGameMode&GM_MULTI && multiData.kills.bShowList)
			HUDShowKillList();
		#endif
#if 0
		DrawWeaponBoxes();
#endif
		return;
	}

	if (gameStates.render.bRearView && gameStates.render.cockpit.nMode!=CM_REAR_VIEW) {
		HUDRenderMessageFrame();
		GrSetCurFont( GAME_FONT );
		GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
		if (gameData.demo.nState == ND_STATE_PLAYBACK)
			GrPrintF(0x8000,grdCurCanv->cv_h-14,TXT_REAR_VIEW);
		else
			GrPrintF(0x8000,grdCurCanv->cv_h-10,TXT_REAR_VIEW);
	}
WIN(DDGRUNLOCK(dd_grd_curcanv));
}

//	-----------------------------------------------------------------------------

//print out some player statistics
void RenderGauges()
{
	static int old_display_mode = 0;
	int energy = f2ir(gameData.multi.players[gameData.multi.nLocalPlayer].energy);
	int shields = f2ir(gameData.multi.players[gameData.multi.nLocalPlayer].shields);
	int cloak = ((gameData.multi.players[gameData.multi.nLocalPlayer].flags&PLAYER_FLAGS_CLOAKED) != 0);
	int frc=0;

if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
	return;

cmScaleX = (grdCurScreen->sc_w <= 640) ? 1 : (double) grdCurScreen->sc_w / 640.0;
cmScaleY = (grdCurScreen->sc_h <= 480) ? 1 : (double) grdCurScreen->sc_h / 480.0;

PA_DFX (frc=0);
PA_DFX (pa_set_backbuffer_current());

Assert(gameStates.render.cockpit.nMode==CM_FULL_COCKPIT || gameStates.render.cockpit.nMode==CM_STATUS_BAR);

// check to see if our display mode has changed since last render time --
// if so, then we need to make new gauge canvases.


if (old_display_mode != gameStates.video.nDisplayMode) {
	CloseGaugeCanvases();
	InitGaugeCanvases();
	old_display_mode = gameStates.video.nDisplayMode;
	}

if (shields < 0 ) 
	shields = 0;

WINDOS(
	DDGrSetCurrentCanvas(GetCurrentGameScreen()),
	GrSetCurrentCanvas(GetCurrentGameScreen())
	);
GrSetCurFont( GAME_FONT );

if (gameData.demo.nState == ND_STATE_RECORDING)
	if (gameData.multi.players[gameData.multi.nLocalPlayer].homing_object_dist >= 0)
		NDRecordHomingDistance(gameData.multi.players[gameData.multi.nLocalPlayer].homing_object_dist);

DrawWeaponBoxes();
if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
	if (energy != old_energy[VR_current_page]) {
		if (gameData.demo.nState==ND_STATE_RECORDING ) {
			NDRecordPlayerEnergy(old_energy[VR_current_page], energy);
		}
	}
	DrawEnergyBar(energy);
	DrawNumericalDisplay(shields, energy);
	old_energy[VR_current_page] = energy;

	if (xAfterburnerCharge != old_afterburner[VR_current_page]) {
		if (gameData.demo.nState==ND_STATE_RECORDING ) {
			NDRecordPlayerAfterburner(old_afterburner[VR_current_page], xAfterburnerCharge);
		}
	}
	DrawAfterburnerBar(xAfterburnerCharge);
	old_afterburner[VR_current_page] = xAfterburnerCharge;

	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE) {
		DrawNumericalDisplay(shields, energy);
		DrawInvulnerableShip();
		old_shields[VR_current_page] = shields ^ 1;
	} else {
		if (shields != old_shields[VR_current_page]) {		// Draw the shield gauge
			if (gameData.demo.nState==ND_STATE_RECORDING ) {
				NDRecordPlayerShields(old_shields[VR_current_page], shields);
			}
		}
		DrawShieldBar(shields);
		DrawNumericalDisplay(shields, energy);
		old_shields[VR_current_page] = shields;
	}

	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags != old_flags[VR_current_page]) {
		if (gameData.demo.nState==ND_STATE_RECORDING )
			NDRecordPlayerFlags(old_flags[VR_current_page], gameData.multi.players[gameData.multi.nLocalPlayer].flags);
		old_flags[VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].flags;
	}
	DrawKeys();
	ShowHomingWarning();
	ShowBombCount(BOMB_COUNT_X,BOMB_COUNT_Y,BLACK_RGBA,gameStates.render.cockpit.nMode == CM_FULL_COCKPIT);
} else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {

	if (energy != old_energy[VR_current_page] || frc)  {
		if (gameData.demo.nState==ND_STATE_RECORDING ) {
			NDRecordPlayerEnergy(old_energy[VR_current_page], energy);
		}
		SBDrawEnergyBar(energy);
		old_energy[VR_current_page] = energy;
	}

	if (xAfterburnerCharge != old_afterburner[VR_current_page] || frc) {
		if (gameData.demo.nState==ND_STATE_RECORDING ) {
			NDRecordPlayerAfterburner(old_afterburner[VR_current_page], xAfterburnerCharge);
		}
		SBDrawAfterburner();
		old_afterburner[VR_current_page] = xAfterburnerCharge;
	}
	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE) {
		DrawInvulnerableShip();
		old_shields[VR_current_page] = shields ^ 1;
		SBDrawShieldNum(shields);
	} 
	else 
		if (shields != old_shields[VR_current_page] || frc) {		// Draw the shield gauge
			if (gameData.demo.nState==ND_STATE_RECORDING ) {
				NDRecordPlayerShields(old_shields[VR_current_page], shields);
			}
			SBDrawShieldBar(shields);
			old_shields[VR_current_page] = shields;
			SBDrawShieldNum(shields);
		}
	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags != old_flags[VR_current_page] || frc) {
		if (gameData.demo.nState==ND_STATE_RECORDING )
			NDRecordPlayerFlags(old_flags[VR_current_page], gameData.multi.players[gameData.multi.nLocalPlayer].flags);
		SBDrawKeys();
		old_flags[VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].flags;
	}
	if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP))
	{
		if (gameData.multi.players[gameData.multi.nLocalPlayer].net_killed_total != old_lives[VR_current_page] || frc) {
			SBShowLives();
			old_lives[VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].net_killed_total;
		}
	}
	else
	{
		if (gameData.multi.players[gameData.multi.nLocalPlayer].lives != old_lives[VR_current_page] || frc) {
			SBShowLives();
			old_lives[VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].lives;
		}
	}
	if ((gameData.app.nGameMode&GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP)) {
		if (gameData.multi.players[gameData.multi.nLocalPlayer].net_kills_total != old_score[VR_current_page] || frc) {
			SBShowScore();
			old_score[VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].net_kills_total;
		}
	}
	else {
		if (gameData.multi.players[gameData.multi.nLocalPlayer].score != old_score[VR_current_page] || frc) {
			SBShowScore();
			old_score[VR_current_page] = gameData.multi.players[gameData.multi.nLocalPlayer].score;
		}

		//if (score_time)
			SBShowScoreAdded();
	}
	ShowBombCount(SB_BOMB_COUNT_X,SB_BOMB_COUNT_Y,BLACK_RGBA,0);
}
if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT)
	DrawPlayerShip(cloak,old_cloak[VR_current_page],SHIP_GAUGE_X,SHIP_GAUGE_Y);
else if (frc || (cloak != old_cloak[VR_current_page]) || nCloakFadeState || 
	(cloak && gameData.app.xGameTime>gameData.multi.players[gameData.multi.nLocalPlayer].cloak_time+CLOAK_TIME_MAX-i2f(3))) {
	DrawPlayerShip(cloak,old_cloak[VR_current_page],SB_SHIP_GAUGE_X,SB_SHIP_GAUGE_Y);

	old_cloak[VR_current_page]=cloak;
	}
}

//	---------------------------------------------------------------------------------------------------------
//	Call when picked up a laser powerup.
//	If laser is active, set old_weapon[0] to -1 to force redraw.
void UpdateLaserWeaponInfo(void)
{
	if (old_weapon[0][VR_current_page] == 0)
		if (! (gameData.multi.players[gameData.multi.nLocalPlayer].laser_level > MAX_LASER_LEVEL && old_laser_level[VR_current_page] <= MAX_LASER_LEVEL))
			old_weapon[0][VR_current_page] = -1;
}

extern int Game_window_y;
void fill_background(void);

int SW_drawn[2], SW_x[2], SW_y[2], SW_w[2], SW_h[2];

//	---------------------------------------------------------------------------------------------------------
//draws a 3d view into one of the cockpit windows.  win is 0 for left,
//1 for right.  viewer is object.  NULL object means give up window
//user is one of the WBU_ constants.  If rear_view_flag is set, show a
//rear view.  If label is non-NULL, print the label at the top of the
//window.

int cockpitWindowScale [4] = {6,5,4,3};

void DoCockpitWindowView(int win,object *viewer,int rear_view_flag,int user,char *label)
{
	WINDOS(
		dd_grs_canvas window_canv,
		grs_canvas window_canv
	);
	WINDOS(
		static dd_grs_canvas overlap_canv,
		static grs_canvas overlap_canv
	);

#ifdef WINDOWS
	int saved_window_x, saved_window_y;
#endif

	object *viewer_save = gameData.objs.viewer;
	grs_canvas *save_canv = grdCurCanv;
	static int overlap_dirty[2]={0,0};
	int boxnum;
	static int window_x,window_y;
	gauge_box *box;
	int rear_view_save = gameStates.render.bRearView;
	int w,h,dx;
	fix nZoomSave;

	if (!gameOpts->render.cockpit.bHUD && ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)))
		return;
	box = NULL;
	if (viewer == NULL) {								//this user is done
		Assert(user == WBU_WEAPON || user == WBU_STATIC);
		if (user == WBU_STATIC && weapon_box_user[win] != WBU_STATIC)
			static_time[win] = 0;
		if (weapon_box_user[win] == WBU_WEAPON || weapon_box_user[win] == WBU_STATIC)
			return;		//already set
		weapon_box_user[win] = user;
		if (overlap_dirty[win]) {
		WINDOS(
			DDGrSetCurrentCanvas(&dd_VR_screen_pages[VR_current_page]),
			GrSetCurrentCanvas(&VR_screen_pages[VR_current_page])
		);
			fill_background();
			overlap_dirty[win] = 0;
		}

		return;
	}
	UpdateRenderedData(win+1, viewer, rear_view_flag, user);
	weapon_box_user[win] = user;						//say who's using window
	gameData.objs.viewer = viewer;
	gameStates.render.bRearView = rear_view_flag;

	if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)	{

		w = VR_render_buffer[0].cv_bitmap.bm_props.w/cockpitWindowScale [gameOpts->render.cockpit.nWindowSize];			// hmm.  I could probably do the sub_buffer assigment for all macines, but I aint gonna chance it

		h = i2f(w) / grdCurScreen->sc_aspect;

		dx = (win==0)?-(w+(w/10)):(w/10);

		switch (gameOpts->render.cockpit.nWindowPos) {
			case 0:
				window_x = win ? 
					VR_render_buffer[0].cv_bitmap.bm_props.w-w-h/10 :
					h / 10;
				window_y = VR_render_buffer[0].cv_bitmap.bm_props.h-h-(h/10);
				break;
			case 1:
				window_x = win ? 
					VR_render_buffer[0].cv_bitmap.bm_props.w/3*2-w/3 :
					VR_render_buffer[0].cv_bitmap.bm_props.w/3-2*w/3;
				window_y = VR_render_buffer[0].cv_bitmap.bm_props.h-h-(h/10);
				break;
			case 2:	// only makes sense if there's only one cockpit window
				window_x = VR_render_buffer[0].cv_bitmap.bm_props.w/2-w/2;
				window_y = VR_render_buffer[0].cv_bitmap.bm_props.h-h-(h/10);
				break;
			case 3:
				window_x = win ? 
					VR_render_buffer[0].cv_bitmap.bm_props.w-w-h/10 :
					h / 10;
				window_y = h/10;
				break;
			case 4:
				window_x = win ? 
					VR_render_buffer[0].cv_bitmap.bm_props.w/3*2-w/3 :
					VR_render_buffer[0].cv_bitmap.bm_props.w/3-2*w/3;
				window_y = h/10;
				break;
			case 5:	// only makes sense if there's only one cockpit window
				window_x = VR_render_buffer[0].cv_bitmap.bm_props.w/2-w/2;
				window_y = h/10;
				break;
			}
		if ((gameOpts->render.cockpit.nWindowPos < 3) && 
			 extraGameInfo [0].nWeaponIcons &&
			 (extraGameInfo [0].nWeaponIcons - gameOpts->render.weaponIcons.bEquipment < 3))
			 window_y -= (int) ((gameOpts->render.weaponIcons.bSmall ? 20.0 : 30.0) * (double) grdCurCanv->cv_h / 480.0);


#ifdef WINDOWS
		saved_window_x = window_x;
		saved_window_y = window_y;
		window_x = dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_props.w/2+dx;
		window_x = win ? 
			dd_VR_render_sub_buffer[0].cv_bitmap.bm_props.w/3*2-w/2 :
			dd_VR_render_sub_buffer[0].cv_bitmap.bm_props.w/3+w/2;
		window_y = VR_render_buffer[0].cv_bitmap.bm_props.h-h-(h/10)-dd_VR_render_sub_buffer[0].yoff;
#endif

		//copy these vars so stereo code can get at them
		SW_drawn[win]=1; 
		SW_x[win] = window_x; 
		SW_y[win] = window_y; 
		SW_w[win] = w; 
		SW_h[win] = h; 

	WINDOS(
		DDGrInitSubCanvas(&window_canv, &dd_VR_render_buffer[0],window_x,window_y,w,h),
		GrInitSubCanvas(&window_canv,&VR_render_buffer[0],window_x,window_y,w,h)
	);
	}
	else {
		if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT)
			boxnum = (COCKPIT_PRIMARY_BOX)+win;
		else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
			boxnum = (SB_PRIMARY_BOX)+win;
		else
			goto abort;
		box = gauge_boxes + boxnum;
	WINDOS(								  
		DDGrInitSubCanvas(
			&window_canv,dd_VR_render_buffer,
			HUD_SCALE_X (box->left),
			HUD_SCALE_Y (box->top),
			HUD_SCALE_X (box->right-box->left+1),
			HUD_SCALE_Y (box->bot-box->top+1)),
		GrInitSubCanvas(
			&window_canv,VR_render_buffer,
			HUD_SCALE_X (box->left),
			HUD_SCALE_Y (box->top),
			HUD_SCALE_X (box->right-box->left+1),
			HUD_SCALE_Y (box->bot-box->top+1))
	);
	}

WINDOS(
	DDGrSetCurrentCanvas(&window_canv),
	GrSetCurrentCanvas(&window_canv)
);
	
	WIN(DDGRLOCK(dd_grd_curcanv));
	
		nZoomSave = gameStates.render.nZoomFactor;
		gameStates.render.nZoomFactor = F1_0 * (gameOpts->render.cockpit.nWindowZoom + 1);					//the player's zoom factor
		if ((user == WBU_RADAR_TOPDOWN) ||(user == WBU_RADAR_HEADSUP)) {
			gameStates.render.bTopDownRadar = (user == WBU_RADAR_TOPDOWN);
			if (!(gameData.app.nGameMode & GM_MULTI) || (netGame.game_flags & NETGAME_FLAG_SHOW_MAP))
				DoAutomap (0, 1);
			else
				RenderFrame(0, win+1);
			}
		else
			RenderFrame(0, win+1);

		gameStates.render.nZoomFactor = nZoomSave;					//the player's zoom factor
		
	WIN(DDGRUNLOCK(dd_grd_curcanv));

	//	HACK! If guided missile, wake up robots as necessary.
	if (viewer->type == OBJ_WEAPON) {
		// -- Used to require to be GUIDED -- if (viewer->id == GUIDEDMISS_ID)
		WakeupRenderedObjects(viewer, win+1);
	}

	if (label) {
		WIN(DDGRLOCK(dd_grd_curcanv));
			GrSetCurFont( GAME_FONT );
		GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
		GrPrintF(0x8000,2,label);
			WIN(DDGRUNLOCK(dd_grd_curcanv));
		}

	if (user == WBU_GUIDED) {
		WIN(DDGRLOCK(dd_grd_curcanv));
		DrawGuidedCrosshair();
		WIN(DDGRUNLOCK(dd_grd_curcanv));
		}

	if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN) {
		int small_window_bottom,big_window_bottom,extra_part_h;
		
		WIN(DDGRLOCK(dd_grd_curcanv));
		GrSetColorRGBi (RGBA_PAL (0,0,32));
		GrUBox(0,0,grdCurCanv->cv_bitmap.bm_props.w-1,grdCurCanv->cv_bitmap.bm_props.h);
		WIN(DDGRUNLOCK(dd_grd_curcanv));

		//if the window only partially overlaps the big 3d window, copy
		//the extra part to the visible screen

		big_window_bottom = Game_window_y + Game_window_h - 1;

	#ifdef WINDOWS
		window_x = saved_window_x;
		window_y = saved_window_y;
//		DDGrInitSubCanvas(&window_canv, &dd_VR_render_buffer[0],window_x,window_y,
//						VR_render_buffer[0].cv_bitmap.bm_props.w/6,
//						i2f(VR_render_buffer[0].cv_bitmap.bm_props.w/6) / grdCurScreen->sc_aspect);

	#endif

		if (window_y > big_window_bottom) {

			//the small window is completely outside the big 3d window, so
			//copy it to the visible screen

			if (VR_screen_flags & VRF_USE_PAGING)
				WINDOS(
					DDGrSetCurrentCanvas(&dd_VR_screen_pages[!VR_current_page]),
					GrSetCurrentCanvas(&VR_screen_pages[!VR_current_page])
				);
			else
				WINDOS(
					DDGrSetCurrentCanvas(GetCurrentGameScreen()),
					GrSetCurrentCanvas(GetCurrentGameScreen())
				);

			WINDOS(
				dd_gr_blt_notrans(&window_canv, 0,0,0,0,
										dd_grd_curcanv, window_x, window_y, 0,0),
				GrBitmap(window_x,window_y,&window_canv.cv_bitmap)
			);

			overlap_dirty[win] = 1;
		}
		else {

		WINDOS(
			small_window_bottom = window_y + window_canv.canvas.cv_bitmap.bm_props.h - 1,
			small_window_bottom = window_y + window_canv.cv_bitmap.bm_props.h - 1
		);
			
			extra_part_h = small_window_bottom - big_window_bottom;

			if (extra_part_h > 0) {
			
	
				WINDOS(
					DDGrInitSubCanvas(&overlap_canv,&window_canv,0,
						window_canv.canvas.cv_bitmap.bm_props.h-extra_part_h,
						window_canv.canvas.cv_bitmap.bm_props.w,extra_part_h),
					GrInitSubCanvas(&overlap_canv,&window_canv,0,window_canv.cv_bitmap.bm_props.h-extra_part_h,window_canv.cv_bitmap.bm_props.w,extra_part_h)
				);

				if (VR_screen_flags & VRF_USE_PAGING)
					WINDOS(
						DDGrSetCurrentCanvas(&dd_VR_screen_pages[!VR_current_page]),
						GrSetCurrentCanvas(&VR_screen_pages[!VR_current_page])
					);
				else
					WINDOS(
						DDGrSetCurrentCanvas(GetCurrentGameScreen()),
						GrSetCurrentCanvas(GetCurrentGameScreen())
					);

				WINDOS(
					dd_gr_blt_notrans(&overlap_canv, 0,0,0,0,
											dd_grd_curcanv, window_x, big_window_bottom+1, 0,0),
					GrBitmap(window_x,big_window_bottom+1,&overlap_canv.cv_bitmap)
				);
				
				overlap_dirty[win] = 1;
			}
		}
	}
	else {
	PA_DFX (goto skip_this_junk);
	
	WINDOS(
		DDGrSetCurrentCanvas(GetCurrentGameScreen()),
		GrSetCurrentCanvas(GetCurrentGameScreen())
	);
	WINDOS(
		CopyGaugeBox(box,&dd_VR_render_buffer[0]),
		CopyGaugeBox(box,&VR_render_buffer[0].cv_bitmap)
	);
	}

  PA_DFX(skip_this_junk:)
  
	//force redraw when done
	old_weapon[win][VR_current_page] = old_ammo_count[win][VR_current_page] = -1;

abort:;

	gameData.objs.viewer = viewer_save;
WINDOS(
	DDGrSetCurrentCanvas(save_canv),
	GrSetCurrentCanvas(save_canv)
);
	gameStates.render.bRearView = rear_view_save;
}

//	-----------------------------------------------------------------------------

