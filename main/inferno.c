/* $Id: inferno.c,v 1.71 2004/04/15 07:34:28 btb Exp $ */
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
 * inferno.c: Entry point of program (main procedure)
 *
 * After main initializes everything, most of the time is spent in the loop
 * while (gameStates.app.nFunctionMode != FMODE_EXIT)
 * In this loop, the main menu is brought up first.
 *
 * main () for Inferno
 *
 * Old Log:
 * Revision 1.1  1995/12/05  15:57:49  allender
 * Initial revision
 *
 * Revision 1.19  1995/11/07  17:03:12  allender
 * added splash screen for descent contest
 *
 * Revision 1.18  1995/10/31  10:22:22  allender
 * shareware stuff
 *
 * Revision 1.17  1995/10/18  01:53:07  allender
 * mouse click to leave do not distribute screen
 *
 * Revision 1.16  1995/10/17  12:00:12  allender
 * mouse click gets past endgame screen
 *
 * Revision 1.15  1995/10/12  17:40:12  allender
 * read config file after digi initialized
 *
 * Revision 1.14  1995/10/05  10:38:22  allender
 * changed key_getch at exit to be KeyInKey and moved
 * mouse init until after macintosh windowing init call
 *
 * Revision 1.13  1995/09/18  17:01:04  allender
 * put GrInit call before render buffer stuff
 *
 * Revision 1.12  1995/08/31  15:50:53  allender
 * call init for appletalk, and change name of intro screens
 *
 * Revision 1.11  1995/08/26  16:26:19  allender
 * whole bunch 'o stuff!!
 *
 * Revision 1.10  1995/07/12  21:48:18  allender
 * removed Int3 from beginning of program
 *
 * Revision 1.9  1995/07/05  16:45:48  allender
 * removed hide_cursor call
 *
 * Revision 1.8  1995/06/20  16:44:57  allender
 * game now renders in 640x480 at all times.  Changed code
 * to call GameInitRenderBuffers with right params
 *
 * Revision 1.7  1995/06/13  13:07:55  allender
 * change macintosh initialzation.  Mac windows now init'ed through GrInit.
 *
 * Revision 1.6  1995/06/08  16:36:53  allender
 * "ifdef" profile include
 *
 * Revision 1.5  1995/06/07  08:08:18  allender
 * dont' make memory info show at end of program
 *
 * Revision 1.4  1995/06/02  07:47:40  allender
 * removed network initialzation for now
 *
 * Revision 1.3  1995/05/26  06:54:52  allender
 * put DigiInit after timer and key stuff since I was testing stuff
 * that needed the keyboard handler installed
 *
 * Revision 1.2  1995/05/19  11:28:09  allender
 * removed //printf
 *
 * Revision 1.1  1995/05/16  15:26:39  allender
 * Initial revision
 *
 * Revision 2.36  1996/01/05  16:52:16  john
 * Improved 3d stuff.
 *
 * Revision 2.35  1995/10/07  13:20:06  john
 * Added new modes for LCDBIOS, also added support for -JoyNice,
 * and added Shift+F1-F4 to controls various stereoscopic params.
 *
 * Revision 2.34  1995/06/26  11:30:57  john
 * Made registration/copyright screen go away after 5 minutes.
 *
 * Revision 2.33  1995/05/31  14:26:55  unknown
 * Fixed ugly spacing.
 *
 * Revision 2.32  1995/05/26  16:15:28  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 *
 * Revision 2.31  1995/05/11  13:30:01  john
 * Changed 3dbios detection to work like Didde Kim wanted it to.
 *
 * Revision 2.30  1995/05/08  13:53:50  john
 * Added code to read vipport environemnt variable.
 *
 * Revision 2.29  1995/05/08  11:26:18  john
 * Reversed eyes in 3dmax mode.
 *
 * Revision 2.28  1995/05/08  11:24:06  john
 * Made 3dmax work like Kasan wants it to.
 *
 * Revision 2.27  1995/04/23  16:06:25  john
 * Moved rinvul into modem/null modem menu.
 *
 * Revision 2.26  1995/04/12  13:39:26  john
 * Fixed bug with -lowmem not working.
 *
 * Revision 2.25  1995/04/09  14:43:00  john
 * Made Dynamic sockets not print Msockets for help.
 *
 * Revision 2.24  1995/04/07  16:11:33  john
 * Fixed problem with VFX display when using setup.
 *
 * Revision 2.23  1995/04/06  15:40:51  john
 * Synced VFX with setup#'s.
 *
 * Revision 2.22  1995/04/06  12:12:53  john
 * Fixed some bugs with 3dmax.
 *
 * Revision 2.21  1995/03/30  16:36:51  mike
 * text localization.
 *
 * Revision 2.20  1995/03/29  15:33:52  john
 * Added code to parse descent.net file.
 *
 * Revision 2.19  1995/03/28  20:08:21  john
 * Took away alternate server thing.
 *
 * Revision 2.18  1995/03/27  09:43:08  john
 * Added VR Settings in config file.
 *
 * Revision 2.17  1995/03/23  19:02:21  john
 * Added descent.net file use.
 *
 * Revision 2.16  1995/03/23  12:25:11  john
 * Moved IPX stuff into BIOS lib.
 *
 * Revision 2.15  1995/03/21  16:52:34  john
 * Added 320x100.
 *
 * Revision 2.14  1995/03/21  14:40:33  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.13  1995/03/16  23:13:35  john
 * Fixed bug with piggy paging in bitmap not checking for disk
 * error, hence bogifying textures if you pull the CD out.
 *
 * Revision 2.12  1995/03/16  21:45:22  john
 * Made all paged modes have incompatible menus!
 *
 * Revision 2.11  1995/03/15  15:19:34  john
 * Took out code that changes to exe dir.
 *
 * Revision 2.10  1995/03/15  14:33:37  john
 * Added code to force the Descent CD-rom in the drive.
 *
 * Revision 2.9  1995/03/15  11:41:27  john
 * Better Saturn CD-ROM support.
 *
 * Revision 2.8  1995/03/14  18:24:46  john
 * Force Destination Saturn to use CD-ROM drive.
 *
 * Revision 2.7  1995/03/14  16:22:35  john
 * Added cdrom alternate directory stuff.
 *
 * Revision 2.6  1995/03/13  15:17:19  john
 * Added alternate hogfile directory.
 *
 * Revision 2.5  1995/03/10  13:05:35  john
 * Added code so that palette is correct for VFX1 helmets.
 *
 * Revision 2.4  1995/03/07  15:12:43  john
 * Fixed VFX,3dmax support.
 *
 * Revision 2.3  1995/03/07  14:19:35  mike
 * More destination saturn stuff.
 *
 * Revision 2.2  1995/03/06  16:47:34  mike
 * destination saturn
 *
 * Revision 2.1  1995/03/06  15:24:06  john
 * New screen techniques.
 *
 * Revision 2.0  1995/02/27  11:31:29  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.295  1995/02/23  12:02:14  john
 * Made mono  windows smaller.
 *
 * Revision 1.294  1995/02/16  17:35:00  john
 * Added code to allow dynamic socket changing.
 *
 * Revision 1.293  1995/02/14  19:29:29  john
 * Locked down critical error handler.
 *
 * Revision 1.292  1995/02/14  15:29:20  john
 * Added CR-LF to last line of menu help text.
 *
 * Revision 1.291  1995/02/14  11:39:01  john
 * Added polled/bios joystick readers.
 *
 * Revision 1.290  1995/02/13  20:35:03  john
 * Lintized
 *
 * Revision 1.289  1995/02/11  16:20:02  john
 * Added code to make the default mission be the one last nPlayed.
 *
 * Revision 1.288  1995/02/11  15:54:13  rob
 * changed cinvul to rinvul.
 *
 * Revision 1.287  1995/02/11  14:48:43  rob
 * Added max of 314 seconds to control invul. times
 *
 * Revision 1.286  1995/02/11  12:42:01  john
 * Added new song method, with FM bank switching..
 *
 * Revision 1.285  1995/02/11  11:36:11  rob
 * Added cinvul option.
 *
 * Revision 1.284  1995/02/10  16:07:45  matt
 * Took 'registered' out of printed info at startup
 *
 * Revision 1.283  1995/02/09  22:00:59  john
 * Added i-glasses tracking.
 *
 * Revision 1.282  1995/02/02  11:11:27  john
 * Added -nocyberman switch.
 *
 * Revision 1.281  1995/02/01  16:35:14  john
 * Linted.
 *
 * Revision 1.280  1995/01/31  02:04:25  matt
 * Fixed up cmdline help
 *
 * Revision 1.279  1995/01/30  16:25:55  john
 * Put back in graphical screen at program end.
 *
 * Revision 1.278  1995/01/28  17:05:50  matt
 * Changed imbedded copyright to use comma instead of hyphen
 *
 * Revision 1.277  1995/01/28  15:57:26  john
 * Made joystick calibration be only when wrong detected in
 * menu or joystick axis changed.
 *
 * Revision 1.276  1995/01/25  14:37:49  john
 * Made joystick only prompt for calibration once...
 *
 * Revision 1.275  1995/01/24  18:21:00  john
 * Added Adam's text warning.
 *
 * Revision 1.274  1995/01/22  15:57:20  john
 * Took out code that printed warning out as game exited.
 *
 * Revision 1.273  1995/01/22  13:31:35  matt
 * Added load of mission 0, so there's always a default mission
 *
 * Revision 1.272  1995/01/19  17:00:41  john
 * Made save game work between levels.
 *
 * Revision 1.271  1995/01/18  11:47:57  adam
 * changed copyright notice
 *
 * Revision 1.270  1995/01/15  13:42:42  john
 * Moved low_mem cutoff higher.
 *
 * Revision 1.269  1995/01/12  18:53:50  john
 * Put ifdef EDITOR around the code that checked for
 * a 800x600 mode, because this trashed some people's
 * computers (maybe) causing the mem allocation error in
 * mouse.c that many users reported.
 *
 * Revision 1.268  1995/01/12  11:41:42  john
 * Added external control reading.
 *
 * Revision 1.267  1995/01/06  10:26:55  john
 * Added -nodoscheck command line switch.
 *
 * Revision 1.266  1995/01/05  16:59:30  yuan
 * Don't show orderform in editor version.
 *
 * Revision 1.265  1994/12/28  15:33:51  john
 * Added -slowjoy option.
 *
 * Revision 1.264  1994/12/15  16:44:15  matt
 * Added trademark notice
 *
 * Revision 1.263  1994/12/14  20:13:59  john
 * Reduced physical mem requments to 2 MB.
 *
 * Revision 1.262  1994/12/14  19:06:17  john
 * Lowered physical memory requments to 2 MB.
 *
 * Revision 1.261  1994/12/14  09:41:29  allender
 * change to drive and directory (if needed) of command line invocation
 * so descent can be started from anywhere
 *
 * Revision 1.260  1994/12/13  19:08:59  john
 * Updated memory requirements.
 *
 * Revision 1.259  1994/12/13  17:30:33  john
 * Made the timer rate be changed right after initializing it.
 *
 * Revision 1.258  1994/12/13  02:46:25  matt
 * Added imbedded copyright
 *
 * Revision 1.257  1994/12/13  02:06:46  john
 * Added code to check stack used by descent... initial
 * check showed stack used 35k/50k, so we decided it wasn't
 * worth pursuing any more.
 *
 * Revision 1.256  1994/12/11  23:17:54  john
 * Added -nomusic.
 * Added gameData.time.xRealFrame.
 * Put in a pause when sound initialization error.
 * Made controlcen countdown and framerate use gameData.time.xRealFrame.
 *
 * Revision 1.255  1994/12/10  00:56:51  matt
 * Added -nomusic to command-line help
 *
 * Revision 1.254  1994/12/08  11:55:11  john
 * Took out low memory print.
 *
 * Revision 1.253  1994/12/08  11:51:00  john
 * Made strcpy only copy corect number of chars,.
 *
 * Revision 1.252  1994/12/08  00:38:29  matt
 * Cleaned up banner messages
 *
 * Revision 1.251  1994/12/07  19:14:52  matt
 * Cleaned up command-line options and command-line help message
 *
 * Revision 1.250  1994/12/06  19:33:28  john
 * Fixed text of message to make more sense.
 *
 * Revision 1.249  1994/12/06  16:30:55  john
 * Neatend mem message,..
 *
 * Revision 1.248  1994/12/06  16:17:35  john
 * Added better mem checking/printing.
 *
 * Revision 1.247  1994/12/06  14:14:37  john
 * Added code to set low mem based on memory.
 *
 * Revision 1.246  1994/12/05  12:29:09  allender
 * removed ifdefs around -norun option
 *
 * Revision 1.245  1994/12/05  00:03:30  matt
 * Added -norun option to exit after writing pig
 *
 * Revision 1.244  1994/12/04  14:47:01  john
 * MAde the intro and menu be the same song.
 *
 * Revision 1.243  1994/12/04  14:36:42  john
 * Added menu music.
 *
 * Revision 1.242  1994/12/02  13:50:17  yuan
 * Localization.
 *
 * Revision 1.241  1994/12/01  17:28:30  adam
 * added end-shareware stuff
 *
 * Revision 1.240  1994/11/30  12:10:57  adam
 * added support for PCX titles/brief screens
 *
 * Revision 1.239  1994/11/29  15:47:33  matt
 * Moved error_init to start of game, so error message prints last
 *
 * Revision 1.238  1994/11/29  14:19:22  jasen
 * reduced dos mem requirments.
 *
 * Revision 1.237  1994/11/29  03:46:35  john
 * Added joystick sensitivity; Added sound channels to detail menu.  Removed -maxchannels
 * command line arg.
 *
 * Revision 1.236  1994/11/29  02:50:18  john
 * Increased the amount a joystick has to be off before
 * asking if they want to recalibrate their joystick.
 *
 * Revision 1.235  1994/11/29  02:01:29  john
 * Corrected some of the Descent command line help items.
 *
 * Revision 1.234  1994/11/29  01:39:56  john
 * Fixed minor bug with vfx_light help not wrapping correctly.
 *
 * Revision 1.233  1994/11/28  21:34:17  john
 * Reduced dos mem rqment to 70k.
 *
 * Revision 1.232  1994/11/28  21:20:38  john
 * First version with memory checking.
 *
 * Revision 1.231  1994/11/28  20:06:21  rob
 * Removed old serial param command line options.
 * Added -noserial and -nonetwork to help listing.
 *
 * Revision 1.230  1994/11/27  23:15:24  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.229  1994/11/27  20:50:51  matt
 * Don't set mem stuff if no debug
 *
 * Revision 1.228  1994/11/27  18:46:21  matt
 * Cleaned up command-line switches a little
 *
 * Revision 1.227  1994/11/21  17:48:00  matt
 * Added text to specifiy whether shareware or registered version
 *
 * Revision 1.226  1994/11/21  14:44:20  john
 * Fixed some bugs with setting volumes even when -nosound was used. Duh!
 *
 * Revision 1.225  1994/11/21  13:53:42  matt
 * Took out dos extender copyright
 *
 * Revision 1.224  1994/11/21  09:46:54  john
 * Added -showmeminfo parameter.
 *
 * Revision 1.223  1994/11/20  22:12:05  mike
 * Make some stuff dependent on SHAREWARE.
 *
 * Revision 1.222  1994/11/20  21:14:09  john
 * Changed -serial to -noserial.  MAde a 1 sec delay
 * before leaving title screen.  Clear keyboard buffer
 * before asking for player name.
 *
 * Revision 1.221  1994/11/19  15:20:20  mike
 * rip out unused code and data
 *
 * Revision 1.220  1994/11/17  19:14:29  adam
 * prevented order screen from coming up when -notitles is used
 *
 * Revision 1.219  1994/11/16  11:34:39  john
 * Added -nottitle switch.
 *
 * Revision 1.218  1994/11/16  10:05:53  john
 * Added verbose messages.
 *
 * Revision 1.217  1994/11/15  20:12:34  john
 * Added back in inferno and parallax screens.
 *
 * Revision 1.216  1994/11/15  18:35:30  john
 * Added verbose setting.
 *
 * Revision 1.215  1994/11/15  17:47:44  john
 * Added ordering info screen.
 *
 * Revision 1.214  1994/11/15  08:57:44  john
 * Added MS-DOS version checking and -nonetwork option.
 *
 * Revision 1.213  1994/11/15  08:34:32  john
 * Added better error messages for IPX init.
 *
 * Revision 1.212  1994/11/14  20:14:18  john
 * Fixed some warnings.
 *
 * Revision 1.211  1994/11/14  19:50:49  john
 * Added joystick cal values to descent.cfg.
 *
 * Revision 1.210  1994/11/14  17:56:44  allender
 * make call to ReadConfigFile at startup
 *
 * Revision 1.209  1994/11/14  11:41:55  john
 * Fixed bug with editor/game sequencing.
 *
 * Revision 1.208  1994/11/13  17:05:11  john
 * Made the callsign entry be a list box and gave the ability
 * to delete players.
 *
 * Revision 1.207  1994/11/13  15:39:22  john
 * Added critical error handler to game.  Took out -editor command line
 * option because it didn't work anymore and wasn't worth fixing.  Made scores
 * not use MINER enviroment variable on release version, and made scores
 * not print an error if there is no descent.hi.
 *
 * Revision 1.206  1994/11/10  20:53:29  john
 * Used new sound install parameters.
 *
 * Revision 1.205  1994/11/10  11:07:52  mike
 * Set default detail level.
 *
 * Revision 1.204  1994/11/09  13:45:43  matt
 * Made -? work again for help
 *
 * Revision 1.203  1994/11/09  10:55:58  matt
 * Cleaned up initialization for editor -> game transitions
 *
 * Revision 1.202  1994/11/07  21:35:47  matt
 * Use new function iff_read_into_bitmap ()
 *
 * Revision 1.201  1994/11/05  17:22:16  john
 * Fixed lots of sequencing problems with newdemo stuff.
 *
 * Revision 1.200  1994/11/05  14:05:44  john
 * Fixed fade transitions between all screens by making
 * GrPaletteFadeIn and out keep track of whether the palette is
 * faded in or not.  Then, wherever the code needs to fade out, it
 * just calls GrPaletteFadeOut and it will fade out if it isn't
 * already.  The same with fade_in.
 * This eliminates the need for all the flags like Menu_fade_out,
 * game_fade_in palette, etc.
 *
 * Revision 1.199  1994/11/04  14:36:30  allender
 * change gameData.demo.bAuto meaning to mean autostart from menu only.  Use
 * FindArgs when searching for AutoDemo from command line.  also,
 * set N_Players to 1 when starting in editor mode.
 *
 * Revision 1.198  1994/11/02  11:59:49  john
 * Moved menu out of game into inferno main loop.
 *
 * Revision 1.197  1994/11/01  17:57:39  mike
 * -noscreens option to bypass all screens.
 *
 * Revision 1.196  1994/10/28  15:42:34  allender
 * don't register player if Autodemo is on
 *
 * Revision 1.195  1994/10/28  10:58:01  matt
 * Added copyright notice for DOS4GW
 *
 * Revision 1.194  1994/10/20  21:26:48  matt
 * Took out old serial name/number code, and put up message if this
 * is a marked version.
 *
 * Revision 1.193  1994/10/19  09:52:14  allender
 * Print out who descent.exe belongs to if descent.exe is stamped.
 *
 * Revision 1.192  1994/10/18  16:43:05  allender
 * Added check for identifier stamp and time after which descent will
 * no longer run.
 *
 * Revision 1.191  1994/10/17  13:07:17  john
 * Moved the descent.cfg info into the player config file.
 *
 * Revision 1.190  1994/10/04  10:26:31  matt
 * Support new menu fade in
 *
 * Revision 1.189  1994/10/03  22:58:46  matt
 * Changed some values of game_mode
 *
 * Revision 1.188  1994/10/03  18:55:39  rob
 * Changed defaults for com port settings.
 *
 * Revision 1.187  1994/10/03  13:34:47  matt
 * Added new (and hopefully better) game sequencing functions
 *
 * Revision 1.186  1994/09/30  12:37:28  john
 * Added midi,digi volume to configuration.
 *
 * Revision 1.185  1994/09/30  10:08:48  john
 * Changed sound stuff... made it so the reseting card doesn't hang,
 * made volume change only if sound is installed.
 *
 * Revision 1.184  1994/09/28  17:25:00  matt
 * Added first draft of game save/load system
 *
 * Revision 1.183  1994/09/28  16:18:23  john
 * Added capability to play midi song.
 *
 * Revision 1.182  1994/09/28  11:31:18  john
 * Made text output unbuffered.
 *
 * Revision 1.181  1994/09/27  19:23:44  john
 * Added -nojoystick and -nomouse
 *
 * Revision 1.180  1994/09/24  16:55:29  rob
 * No longer open COM port immediately upon program start.
 * No longer set Network_active is serial_active is set.
 *
 * Revision 1.179  1994/09/24  14:16:30  mike
 * Support new game mode constants.
 *
 * Revision 1.178  1994/09/22  17:52:31  rob
 * Added Findargs hooks for -serial, -speed, and -com.
 *
 * Revision 1.177  1994/09/22  16:14:11  john
 * Redid intro sequecing.
 *
 * Revision 1.176  1994/09/21  16:32:58  john
 * Made mouse and keyboard init after BMInit. Why?
 * Because it seems to work better under virtual
 * memory.
 *
 * Revision 1.175  1994/09/21  16:27:52  john
 * Added mouse_init
 *
 * Revision 1.174  1994/09/20  15:14:10  matt
 * New message for new VFX switches
 *
 * Revision 1.173  1994/09/16  16:14:27  john
 * Added acrade sequencing.
 *
 * Revision 1.172  1994/09/16  11:49:52  john
 * Added first version of arcade joystick support;
 * Also fixed some bugs in KConfig.c, such as reading non-present
 * joysticks, which killed frame rate, and not reading KeyDownTime
 * when in slide mode or bank mode.
 *
 * Revision 1.171  1994/09/15  16:11:35  john
 * Added support for VFX1 head tracking. Fixed bug with memory over-
 * write when using stereo mode.
 *
 * Revision 1.170  1994/09/12  19:38:23  john
 * Made some stuff that prints to the DOS screen go to the
 * mono instead, since it really is debugging info.
 *
 * Revision 1.169  1994/08/29  21:18:28  john
 * First version of new keyboard/oystick remapping stuff.
 *
 * Revision 1.168  1994/08/26  13:02:00  john
 * Put high score system in.
 *
 * Revision 1.167  1994/08/24  19:00:23  john
 * Changed KeyDownTime to return fixed seconds instead of
 * milliseconds.
 *
 * Revision 1.166  1994/08/18  16:24:20  john
 * changed socket to channel in text.
 *
 * Revision 1.165  1994/08/18  16:16:51  john
 * Added support for different sockets.
 *
 * Revision 1.164  1994/08/18  10:47:53  john
 * *** empty log message ***
 *
 * Revision 1.163  1994/08/12  09:15:54  john
 * *** empty log message ***
 *
 * Revision 1.162  1994/08/12  03:11:19  john
 * Made network be default off; Moved network options into
 * main menu.  Made starting net game check that mines are the
 * same.
 *
 * Revision 1.161  1994/08/10  19:57:05  john
 * Changed font stuff; Took out old menu; messed up lots of
 * other stuff like game sequencing messages, etc.
 *
 * Revision 1.160  1994/08/05  16:30:23  john
 * Added capability to turn off network.
 *
 * Revision 1.159  1994/08/04  19:42:51  matt
 * Moved serial number & name (and version name) from inferno.c to inferno.ini
 *
 * Revision 1.158  1994/08/03  10:30:23  matt
 * Change cybermaxx switches, updated command-line help, and added serial number system
 *
 * Revision 1.157  1994/07/29  18:30:10  matt
 * New parms (lack of parms, actually) for g3_init ()
 *
 * Revision 1.156  1994/07/24  00:39:25  matt
 * Added more text to TEX file; make NewGame () take a start level; made game
 * load/save menus use open/close window funcs.
 *
 * Revision 1.155  1994/07/21  21:31:27  john
 * First cheapo version of VictorMaxx tracking.
 *
 * Revision 1.154  1994/07/21  18:15:34  matt
 * Ripped out a bunch of unused stuff
 *
 * Revision 1.153  1994/07/21  17:59:10  matt
 * Cleaned up initial mode game/editor code
 *
 * Revision 1.152  1994/07/21  13:11:19  matt
 * Ripped out remants of old demo system, and added demo only system that
 * disables object movement and game options from menu.
 *
 * Revision 1.151  1994/07/20  15:58:27  john
 * First installment of ipx stuff.
 *
 * Revision 1.150  1994/07/15  16:04:24  matt
 * Changed comment for milestone 3 version
 *
 * Revision 1.149  1994/07/15  13:59:24  matt
 * Fixed stupid mistake I make in the last revision
 *
 * Revision 1.148  1994/07/15  13:20:15  matt
 * Updated comand-line help
 *
 * Revision 1.147  1994/07/14  23:29:43  matt
 * Open two mono debug messages, one for errors & one for spew
 *
 * Revision 1.146  1994/07/09  22:48:05  matt
 * Added localizable text
 *
 * Revision 1.145  1994/07/02  13:49:47  matt
 * Cleaned up includes
 *
 * Revision 1.144  1994/06/30  20:04:43  john
 * Added -joydef support.
 *
 * Revision 1.143  1994/06/24  17:01:44  john
 * Add VFX support; Took Game Sequencing, like EndGame and stuff and
 * took it out of game.c and into gameseq.c
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

char copyright[] = "DESCENT II  COPYRIGHT (C) 1994-1996 PARALLAX SOFTWARE CORPORATION";

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if defined(__unix__) || defined(__macosx__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __macosx__
#	include "SDL/SDL_main.h"
#	include "FolderDetector.h"
#else
#	include "SDL_main.h"
#endif
#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "console.h"
#include "pa_enabl.h"       //$$POLY_ACC
#include "gr.h"
#include "fix.h"
#include "vecmat.h"
#include "mono.h"
#include "key.h"
#include "timer.h"
#include "3d.h"
#include "bm.h"
#include "inferno.h"
#include "error.h"
#include "game.h"
#include "segment.h"		//for sideToVerts
#include "u_mem.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "wall.h"
#include "polyobj.h"
#include "effects.h"
#include "digi.h"
#include "iff.h"
#include "pcx.h"
#include "palette.h"
#include "args.h"
#include "sounds.h"
#include "titles.h"
#include "player.h"
#include "text.h"
#include "newdemo.h"
#include "object.h"
#ifdef NETWORK
#	include "network.h"
#	include "modem.h"
#endif
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "joy.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "joydefs.h"
#include "multi.h"
#include "songs.h"
#include "cfile.h"
#include "gameseq.h"
#include "gamepal.h"
#include "mission.h"
#include "movie.h"
#include "compbit.h"
#include "playsave.h"
#include "d_io.h"
#include "tracker.h"
#include "findfile.h"
#include "ogl_init.h"
#include "render.h"
#include "sphere.h"
#include "endlevel.h"
#include "banlist.h"
#include "collide.h"

//#  include "3dfx_des.h"

//added on 9/30/98 by Matt Mueller for selectable automap modes
#include "automap.h"
//end addition -MM

#include "../texmap/scanline.h" //for select_tmap -MM

#if defined (POLY_ACC)
#include "poly_acc.h"
#endif

extern void SetFunctionMode (int);
extern void ShowInGameWarning (char *s);
extern int cfile_use_d2x_hogfile(char * name);
extern int cfile_use_XL_hogfile(char * name);

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#include "ui.h"
#endif

#ifdef __macosx__
#	include <SDL/SDL.h>
#	if USE_SDL_MIXER
#		include <SDL/SDL_mixer.h>
#	endif
#else
#	include <SDL.h>
#	if USE_SDL_MIXER
#		include <SDL_mixer.h>
#	endif
#endif
#include "vers_id.h"

tGameOptions	gameOptions [2];
tGameStates		gameStates;
tGameData		gameData;

void arch_init (void);
void arch_init_start (void);

//static const char desc_id_checksum_str[] = DESC_ID_CHKSUM_TAG "0000"; // 4-byte checksum
char desc_id_exit_num = 0;

//--unused-- grs_bitmap Inferno_bitmap_title;

int WVIDEO_running=0;		//debugger can set to 1 if running

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
#endif

//--unused-- int Cyberman_installed=0;			// SWIFT device present
int __far descent_critical_error_handler (unsigned deverr, unsigned errcode, unsigned __far * devhdr);

void check_joystick_calibration (void);

void show_order_form (void);
void quit_request ();

tGameOptions *gameOpts = gameOptions;

//--------------------------------------------------------------------------

char szAutoMission [255];
char szAutoHogFile [255];

extern void SetMaxPitch (int nMinTurnRate);

int nDescentCriticalError = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

extern int Network_allow_socket_changes;

extern void vfx_set_palette_sub (ubyte *);

extern int VR_low_res;

#define LINE_LEN	100

// ----------------------------------------------------------------------------

void D2SetCaption (void)
{
	char	szCaption [200];

strcpy (szCaption, DESCENT_VERSION);
if (*gameData.multi.players [gameData.multi.nLocalPlayer].callsign) {
	strcat (szCaption, " [");
	strcat (szCaption, gameData.multi.players [gameData.multi.nLocalPlayer].callsign);
	strcat (szCaption, "]");
	strupr (szCaption);
	}
if (*gameData.missions.szCurrentLevel) {
	strcat (szCaption, " - ");
	strcat (szCaption, gameData.missions.szCurrentLevel);
	}
SDL_WM_SetCaption (szCaption, "Descent II");
}

// ----------------------------------------------------------------------------
//read help from a file & print to screen
void print_commandline_help ()
{
	CFILE *ifile;
	int have_binary=0;
	char line[LINE_LEN];

	ifile = CFOpen ("help.tex", gameFolders.szDataDir,"rb",0);
	if (!ifile) {
		ifile = CFOpen ("help.txb", gameFolders.szDataDir,"rb",0);
		if (!ifile)
			Warning (TXT_NO_HELP);
		have_binary = 1;
	}
	if (ifile)
	{
		while (CFGetS (line,LINE_LEN,ifile)) {
			if (have_binary) {
				int i;
				for (i = 0; i < (int) strlen (line) - 1; i++) {
					line[i] = EncodeRotateLeft ((char) (EncodeRotateLeft (line [i]) ^ BITMAP_TBL_XOR));
				}
			}
			if (line[0] == ';')
				continue;		//don't show comments
			//printf ("%s",line);
		}
		CFClose (ifile);
	}

//	//printf (" Diagnostic:\n\n");
//	//printf ("  -emul           %s\n", "Certain video cards need this option in order to run game");
//	//printf (	"  -ddemul         %s\n", "If -emul doesn't work, use this option");
//	//printf ("\n");
#ifdef EDITOR
	//printf (" Editor Options:\n\n");
	//printf ("  -autoload <file>%s\n", "Autoload a level in the editor");
	//printf ("  -hoarddata      %s\n","FIXME: Undocumented");
//	//printf ("  -nobm           %s\n","FIXME: Undocumented");
	//printf ("\n");
#endif
	con_printf ((int) con_threshold.value, " D2X Options:\n\n");
	con_printf ((int) con_threshold.value, "  -noredundancy   %s\n", "Do not send messages when picking up redundant items in multi");
	con_printf ((int) con_threshold.value, "  -shortpackets   %s\n", "Set shortpackets to default as on");
	con_printf ((int) con_threshold.value, "  -gameOpts->render.nMaxFPS <n>     %s\n", "Set maximum framerate (1-100)");
	con_printf ((int) con_threshold.value, "  -notitles       %s\n", "Do not show titlescreens on startup");
	con_printf ((int) con_threshold.value, "  -hogdir <dir>   %s\n", "set shared data directory to <dir>");
#if defined(__unix__) || defined(__macosx__)
	con_printf ((int) con_threshold.value, "  -nohogdir       %s\n", "don't try to use shared data directory");
	con_printf ((int) con_threshold.value, "  -userdir <dir>  %s\n", "set user dir to <dir> instead of $HOME/.d2x");
#endif
	con_printf ((int) con_threshold.value, "  -ini <file>     %s\n", "option file (alternate to command line), defaults to d2x.ini");
	con_printf ((int) con_threshold.value, "  -autodemo       %s\n", "Start in demo mode");
	con_printf ((int) con_threshold.value, "  -bigpig         %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -bspgen         %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -cdproxy        %s\n","FIXME: Undocumented");
#ifndef NDEBUG
	con_printf ((int) con_threshold.value, "  -checktime      %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -showmeminfo    %s\n","FIXME: Undocumented");
#endif
//	con_printf ((int) con_threshold.value, "  -codereadonly   %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -cyberimpact    %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -debug          %s\n","Enable very verbose output");
//	con_printf ((int) con_threshold.value, "  -debugmode      %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -disallowgfx    %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -disallowreboot %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -dynamicsockets %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -forcegfx       %s\n","FIXME: Undocumented");
#ifdef SDL_INPUT
	con_printf ((int) con_threshold.value, "  -grabmouse      %s\n","Keeps the mouse from wandering out of the window");
#endif
//	con_printf ((int) con_threshold.value, "  -hw_3dacc       %s\n","FIXME: Undocumented");
#ifndef RELEASE
	con_printf ((int) con_threshold.value, "  -invulnerability %s\n","Make yourself invulnerable");
#endif
	con_printf ((int) con_threshold.value, "  -ipxnetwork <num> %s\n","Use IPX network number <num>");
	con_printf ((int) con_threshold.value, "  -jasen          %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -joyslow        %s\n","FIXME: Undocumented");
#ifdef NETWORK
	con_printf ((int) con_threshold.value, "  -kali           %s\n","use Kali for networking");
#endif
//	con_printf ((int) con_threshold.value, "  -logfile        %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -lowresmovies   %s\n","Play low resolution movies if available (for slow machines)");
#if defined (EDITOR) || !defined (MACDATA)
	con_printf ((int) con_threshold.value, "  -macdata        %s\n","Read (and, for editor, write) mac data files (swap colors)");
#endif
//	con_printf ((int) con_threshold.value, "  -memdbg         %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -monodebug      %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -nocdrom        %s\n","FIXME: Undocumented");
#ifdef __DJGPP__
	con_printf ((int) con_threshold.value, "  -nocyberman     %s\n","FIXME: Undocumented");
#endif
#ifndef NDEBUG
	con_printf ((int) con_threshold.value, "  -nofade         %s\n","Disable fades");
#endif
#ifdef NETWORK
	con_printf ((int) con_threshold.value, "  -nomatrixcheat  %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -norankings     %s\n","Disable multiplayer ranking system");
	con_printf ((int) con_threshold.value, "  -packets <num>  %s\n","Specifies the number of packets per second\n");
//	con_printf ((int) con_threshold.value, "  -showaddress    %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -socket         %s\n","FIXME: Undocumented");
#endif
#ifndef WINDOWS
	con_printf ((int) con_threshold.value, "  -nomixer        %s\n","Don't crank music volume");
	con_printf ((int) con_threshold.value, "  -sound22k       %s\n","Use 22 KHz sound sample rate");
	con_printf ((int) con_threshold.value, "  -sound11k       %s\n","Use 11 KHz sound sample rate");
//	con_printf ((int) con_threshold.value, "  -superhires     %s\n","Allow higher-resolution modes");
#endif
//	con_printf ((int) con_threshold.value, "  -nomodex        %s\n","FIXME: Undocumented");
#ifndef RELEASE
	con_printf ((int) con_threshold.value, "  -nomovies       %s\n","Don't play movies");
	con_printf ((int) con_threshold.value, "  -noscreens      %s\n","Skip briefing screens");
#endif
#if !defined (SHAREWARE) || (defined (SHAREWARE) && defined (APPLE_DEMO))
	con_printf ((int) con_threshold.value, "  -noredbook      %s\n","Disable redbook audio");
#endif
	con_printf ((int) con_threshold.value, "  -norun          %s\n","Bail out after initialization");
//	con_printf ((int) con_threshold.value, "  -ordinaljoy     %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -rtscts         %s\n","Same as -ctsrts");
//	con_printf ((int) con_threshold.value, "  -semiwin        %s\n","Use non-fullscreen mode");
//	con_printf ((int) con_threshold.value, "  -specialdevice  %s\n","FIXME: Undocumented");
#ifdef TACTILE
	con_printf ((int) con_threshold.value, "  -stickmag       %s\n","FIXME: Undocumented");
#endif
//	con_printf ((int) con_threshold.value, "  -stopwatch      %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -subtitles      %s\n","Turn on movie subtitles (English-only)");
//	con_printf ((int) con_threshold.value, "  -sysram         %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -text <file>    %s\n","Specify alternate .tex file");
//	con_printf ((int) con_threshold.value, "  -tsengdebug1    %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -tsengdebug2    %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -tsengdebug3    %s\n","FIXME: Undocumented");
#ifdef NETWORK
	con_printf ((int) con_threshold.value, "  -udp            %s\n","Use TCP/UDP for networking");
#endif
//	con_printf ((int) con_threshold.value, "  -vidram         %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -xcontrol       %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -xname          %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -xver           %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -tmap <t>       %s\n","select texmapper to use (c,fp,i386,pent,ppro)");
	con_printf ((int) con_threshold.value, "  -<X>x<Y>        %s\n","Set screen resolution to <X> by <Y>");
	con_printf ((int) con_threshold.value, "  -automap<X>x<Y> %s\n","Set automap resolution to <X> by <Y>");
	con_printf ((int) con_threshold.value, "  -automap_gameres %s\n","Set automap to use the same resolution as in game");
//	con_printf ((int) con_threshold.value, "  -menu<X>x<Y>    %s\n","Set menu resolution to <X> by <Y>");
//	con_printf ((int) con_threshold.value, "  -menu_gameres   %s\n","Set menus to use the same resolution as in game");
	con_printf ((int) con_threshold.value, "\n");

	con_printf ((int) con_threshold.value, "D2X System Options:\n\n");
#ifdef __MSDOS__
	con_printf ((int) con_threshold.value, "  -joy209         %s\n", "Use alternate port 209 for joystick");
#endif
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	con_printf ((int) con_threshold.value, "  -fullscreen     %s\n", "Use fullscreen mode if available");
#endif
#ifdef OGL
	con_printf ((int) con_threshold.value, "  -gl_texmagfilt <f> %s\n","set GL_TEXTURE_MAG_FILTER (see readme.d1x)");
	con_printf ((int) con_threshold.value, "  -gl_texminfilt <f> %s\n","set GL_TEXTURE_MIN_FILTER (see readme.d1x)");
	con_printf ((int) con_threshold.value, "  -gl_mipmap      %s\n","set gl texture filters to \"standard\" options for mipmapping");
	con_printf ((int) con_threshold.value, "  -gl_trilinear   %s\n","set gl texture filters to trilinear mipmapping");
	con_printf ((int) con_threshold.value, "  -gl_simple      %s\n","set gl texture filters to gl_nearest for \"original\" look. (default)");
	con_printf ((int) con_threshold.value, "  -gl_alttexmerge %s\n","use new texmerge, usually uses less ram (default)");
	con_printf ((int) con_threshold.value, "  -gl_stdtexmerge %s\n","use old texmerge, uses more ram, but _might_ be a bit faster");
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	con_printf ((int) con_threshold.value, "  -gl_voodoo      %s\n","force fullscreen mode only");
#endif
	con_printf ((int) con_threshold.value, "  -gl_16bittextures %s\n","attempt to use 16bit textures");
	con_printf ((int) con_threshold.value, "  -gameStates.ogl.nReticle <r> %s\n","use OGL reticle 0=never 1=above 320x* 2=always");
	con_printf ((int) con_threshold.value, "  -gl_intensity4_ok %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -gl_luminance4_alpha4_ok %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -gl_readpixels_ok %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -gl_rgba2_ok    %s\n","FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -gl_test1       %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -gl_test2       %s\n","FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -gl_vidmem      %s\n","FIXME: Undocumented");
#ifdef OGL_RUNTIME_LOAD
	con_printf ((int) con_threshold.value, "  -gl_library <l> %s\n","use alternate opengl library");
#endif
#endif
#ifdef SDL_VIDEO
	con_printf ((int) con_threshold.value, "  -nosdlvidmodecheck %s\n", "Some X servers don't like checking vidmode first, so just switch");
	con_printf ((int) con_threshold.value, "  -hwsurface      %s\n","FIXME: Undocumented");
#endif
#if defined(__unix__) || defined(__macosx__)
	con_printf ((int) con_threshold.value, "  -serialdevice <s> %s\n", "Set serial/modem device to <s>");
	con_printf ((int) con_threshold.value, "  -serialread <r> %s\n\n", "Set serial/modem to read from <r>");
#endif
	con_printf ((int) con_threshold.value, "  -legacyfuelcens   %s\n", "Turn off repair centers");
	con_printf ((int) con_threshold.value, "  -legacyhomers     %s\n", "Turn off frame-rate independant homing missile turn rate");
	con_printf ((int) con_threshold.value, "  -legacyinput      %s\n", "Turn off enhanced input handling");
	con_printf ((int) con_threshold.value, "  -legacymouse      %s\n", "Turn off frame-rate independant mouse sensitivity");
	con_printf ((int) con_threshold.value, "  -legacyrender     %s\n", "Turn off colored segment rendering");
	con_printf ((int) con_threshold.value, "  -legacyzbuf       %s\n", "Turn off OpenGL depth buffer");
	con_printf ((int) con_threshold.value, "  -legacyswitches   %s\n", "Turn off fault-tolerant switch handling");
	con_printf ((int) con_threshold.value, "  -legacywalls      %s\n", "Turn off fault-tolerant wall handling");
	con_printf ((int) con_threshold.value, "  -legacymode       %s\n", "Turn off all of the above non-legacy behaviour\n");
	con_printf ((int) con_threshold.value, "  -joydeadzone <n>  %s\n", "Set joystick deadzone to <n> percent (0 <= n <= 100)\n");
	con_printf ((int) con_threshold.value, "  -limitturnrate    %s\n", "Limit max. turn speed in single player mode like in multiplayer\n");
	con_printf ((int) con_threshold.value, "  -friendlyfire <n> %s\n", "Turn friendly fire on (1) or off (0)\n");
	con_printf ((int) con_threshold.value, "  -fixedrespawns    %s\n", "Have gameData.objs.objects always respawn at their initial location\n");
	con_printf ((int) con_threshold.value, "  -respawndelay <n> %s\n", "Delay respawning of gameData.objs.objects for <n> secs (0 <= n <= 60)\n");
	con_printf ((int) con_threshold.value, "\n Help:\n\n");
	con_printf ((int) con_threshold.value, "  -help, -h, -?, ? %s\n", "View this help screen");
	con_printf ((int) con_threshold.value, "\n");
}

// ----------------------------------------------------------------------------

void CheckEndian (void)
{
	union {
		short	s;
		unsigned char	b [2];
	} h;

h.b [0] = 0;
h.b [1] = 1;
gameStates.app.bLittleEndian = (h.s == 256);
}

// ----------------------------------------------------------------------------

void do_joystick_init ()
{

if (!FindArg ("-nojoystick"))	{
	joy_init ();
	if (FindArg ("-joyslow"))
		joy_set_slow_reading (JOY_SLOW_READINGS);
	if (FindArg ("-joypolled"))
		joy_set_slow_reading (JOY_POLLED_READINGS);
	if (FindArg ("-joybios"))
		joy_set_slow_reading (JOY_BIOS_READINGS);
	}
}

// ----------------------------------------------------------------------------
//set this to force game to run in low res
int disable_high_res=0;

void DoSelectPlayer (void)
{
	gameData.multi.players[gameData.multi.nLocalPlayer].callsign[0] = '\0';

if (!gameData.demo.bAuto) 	{
	KeyFlush ();
	//now, before we bring up the register player menu, we need to
	//do some stuff to make sure the palette is ok.  First, we need to
	//get our current palette into the 2d's array, so the remapping will
	//work.  Second, we need to remap the fonts.  Third, we need to fill
	//in part of the fade tables so the darkening of the menu edges works
	RemapFontsAndMenus (1);
	SelectPlayer ();		//get player's name
	}
}

// ----------------------------------------------------------------------------

#define PROGNAME argv[0]

extern char Language[];

//can we do highres menus?
int Inferno_verbose = 0;

//added on 11/18/98 by Victor Rachels to add -mission and -startgame
int start_net_immediately = 0;
//int start_with_mission = 0;
//char *start_with_mission_name;
//end this section addition

// ----------------------------------------------------------------------------

int OpenMovieFile (char *filename,int must_have);

#if defined (POLY_ACC)
#define MENU_HIRES_MODE SM_640x480x15xPA
#else
#define MENU_HIRES_MODE SM (640,480)
#endif

//	DESCENT II by Parallax Software
//		Descent Main

//extern ubyte gr_current_pal[];

#ifdef	EDITOR
int	Auto_exit = 0;
char	Auto_file[128] = "";
#endif

// ----------------------------------------------------------------------------

void PrintVersion (void)
{
	FILE	*f;
	char	fn [FILENAME_LEN];

sprintf (fn, "%s%sd2x-xl-version.txt", gameFolders.szDataDir, *gameFolders.szDataDir ? "/" : "");
if (f = fopen (fn, "wa")) {
	fprintf (f, "%s\n", VERSION);
	fclose (f);
	}
exit (0);
}

// ----------------------------------------------------------------------------

#ifdef WIN32
#	define	STD_GAMEDIR		""
#	define	D2X_APPNAME		"d2x-xl.exe"
#elif defined(__macosx__)
#	define	STD_GAMEDIR		"/Applications/Games/D2X-XL"
#	define	D2X_APPNAME		"d2x-xl"
#else
#	define	STD_GAMEDIR		"/usr/local/games/d2x-xl"
#	define	D2X_APPNAME		"d2x-xl"
#endif

#if defined(__macosx__)
#	define	DATADIR			"Data"
#	define	SHADERDIR		"Shaders"
#	define	MODELDIR			"Models"
#	define	CONFIGDIR		"Config"
#	define	PROFDIR			"Profiles"
#	define	SCRSHOTDIR		"Screenshots"
#	define	MOVIEDIR			"Movies"
#	define	SAVEDIR			"Savegames"
#	define	DEMODIR			"Demos"
#	define	TEXTUREDIR_D2	"Textures"
#	define	TEXTUREDIR_D1	"Textures/d1"
#else
#	define	DATADIR			"data"
#	define	SHADERDIR		"shaders"
#	define	MODELDIR			"models"
#	define	CONFIGDIR		"config"
#	define	PROFDIR			"profiles"
#	define	SCRSHOTDIR		"screenshots"
#	define	MOVIEDIR			"movies"
#	define	SAVEDIR			"savegames"
#	define	DEMODIR			"demos"
#	define	TEXTUREDIR_D2	"textures"
#	define	TEXTUREDIR_D1	"textures/d1"
#endif

void GetAppFolders (void)
{
#ifndef _WIN32
	FFS	ffs;
#endif
	int	i, j;
	char	szDataRootDir [FILENAME_LEN];
	char	*psz, c;
  
*gameFolders.szHomeDir =
*gameFolders.szGameDir =
*gameFolders.szDataDir =
*szDataRootDir = '\0';
if ((i = FindArg ("-userdir")) && GetAppFolder ("", gameFolders.szGameDir, Args [i + 1], D2X_APPNAME))
	*gameFolders.szGameDir = '\0';
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, getenv ("DESCENT2"), D2X_APPNAME))
	*gameFolders.szGameDir = '\0';
#ifdef _WIN32
if (!*gameFolders.szGameDir) {
	psz = Args [0];
	for (j = strlen (psz); j; ) {
		c = psz [--j];
		if ((c == '\\') || (c == '/')) {
			memcpy (gameFolders.szGameDir, psz, ++j);
			gameFolders.szGameDir [j] = '\0';
			break;
			}
		}
	}
strcpy (szDataRootDir, gameFolders.szGameDir);
#else // Linux, OS X
#	ifdef __unix__
if (getenv ("HOME"))
	strcpy (gameFolders.szHomeDir, getenv ("HOME"));
if (!*gameFolders.szGameDir && *gameFolders.szHomeDir && GetAppFolder (gameFolders.szHomeDir, gameFolders.szGameDir, "d2x-xl", "d2x-xl"))
		*gameFolders.szGameDir = '\0';
#	endif //__unix__
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, STD_GAMEDIR, ""))
		*gameFolders.szGameDir = '\0';
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, SHAREPATH, ""))
		*gameFolders.szGameDir = '\0';
#	ifdef __macosx__
	 GetOSXAppFolder(szDataRootDir, gameFolders.szGameDir);
#	else
strcpy (szDataRootDir, gameFolders.szGameDir);
#	endif //__macosx__
if (*gameFolders.szGameDir)
	chdir (gameFolders.szGameDir);
#endif //Linux, OS X
if (!(i = FindArg ("-hogdir")) || GetAppFolder ("", gameFolders.szDataDir, Args [i + 1], "descent2.hog"))
	if (GetAppFolder (szDataRootDir, gameFolders.szDataDir, DATADIR, "descent2.hog"))
		GetAppFolder (szDataRootDir, gameFolders.szDataDir, DATADIR, "d2demo.hog");
psz = strstr (gameFolders.szDataDir, DATADIR);
if (psz && !*(psz + 4)) {
	if (psz > gameFolders.szDataDir) {
		*(psz - 1) = '\0';
		strcpy (szDataRootDir, gameFolders.szDataDir);
		*(psz - 1) = '/';
		}
	}
else 
	strcpy (szDataRootDir, gameFolders.szDataDir);
/*---*/LogErr ("expected game app folder = '%s'\n", gameFolders.szGameDir);
/*---*/LogErr ("expected game data folder = '%s'\n", gameFolders.szDataDir);
GetAppFolder (szDataRootDir, gameFolders.szModelDir, MODELDIR, "*.oof");
GetAppFolder (szDataRootDir, gameFolders.szShaderDir, SHADERDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szTextureDir [0], TEXTUREDIR_D2, "*.tga");
GetAppFolder (szDataRootDir, gameFolders.szTextureDir [1], TEXTUREDIR_D1, "*.tga");
GetAppFolder (szDataRootDir, gameFolders.szMovieDir, MOVIEDIR, "*.mvl");
#ifdef __linux__
if (*gameFolders.szHomeDir) {
	sprintf (szDataRootDir, "%s/.d2x-xl", gameFolders.szHomeDir);
	if (!CFMkDir (szDataRootDir)) {
		char	fn [FILENAME_LEN];
		sprintf (fn, "%s/%s", szDataRootDir, PROFDIR);
		CFMkDir (fn);
		sprintf (fn, "%s/%s", szDataRootDir, SAVEDIR);
		CFMkDir (fn);
		sprintf (fn, "%s/%s", szDataRootDir, SCRSHOTDIR);
		CFMkDir (fn);
		sprintf (fn, "%s/%s", szDataRootDir, DEMODIR);
		CFMkDir (fn);
		sprintf (fn, "%s/%s", szDataRootDir, CONFIGDIR);
		CFMkDir (fn);
		}
	}
#endif
GetAppFolder (szDataRootDir, gameFolders.szProfDir, PROFDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szSaveDir, SAVEDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szScrShotDir, SCRSHOTDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szDemoDir, DEMODIR, "");
if (GetAppFolder (szDataRootDir, gameFolders.szConfigDir, CONFIGDIR, "*.cfg"))
#if 0//def __linux__
	{
	*gameFolders.szConfigDir = '\0';
	if (GetAppFolder (gameFolders.szGameDir, gameFolders.szConfigDir, CONFIGDIR, "*.cfg"))
		strcpy (gameFolders.szConfigDir, gameFolders.szHomeDir);
	}
#else
	strcpy (gameFolders.szConfigDir, gameFolders.szGameDir);
#endif
sprintf (gameFolders.szMissionDir, "%s%s%s", gameFolders.szGameDir, *gameFolders.szGameDir ? "/" : "", BASE_MISSION_DIR);
//if (i = FindArg ("-hogdir"))
//	CFUseAltHogDir (Args [i + 1]);
}

// ----------------------------------------------------------------------------

void EvalLegacyArgs (void)
{
	int bLegacy = FindArg ("-legacymode");

if (bLegacy || FindArg ("-legacyinput"))
	gameOptions [0].legacy.bInput = 1;
if (bLegacy || FindArg ("-legacyzbuf"))
	gameOptions [0].legacy.bZBuf = 1;
if (bLegacy || FindArg ("-legacyfuelcens"))
	gameOptions [0].legacy.bFuelCens = 1;
if (bLegacy || FindArg ("-legacymouse"))
	gameOptions [0].legacy.bMouse = 1;
if (bLegacy || FindArg ("-legacyhomers"))
	gameOptions [0].legacy.bHomers = 1;
if (bLegacy || FindArg ("-legacyrender"))
	gameOptions [0].legacy.bRender = 1;
if (bLegacy || FindArg ("-legacyswitches"))
	gameOptions [0].legacy.bSwitches = 1;
if (bLegacy || FindArg ("-legacywalls"))
	gameOptions [0].legacy.bWalls = 1;
}

// ----------------------------------------------------------------------------

void EvalAutoNetGameArgs (void)
{
	int	t, bHaveIp = 0;
	char	*p;

	static char *pszTypes [] = {"anarchy", "coop", "ctf", "ctf+", "hoard", "entropy", NULL};
	static char	*pszConnect [] = {"ipx", "udp", "", "multicast", NULL};

memset (&gameData.multi.autoNG, 0, sizeof (gameData.multi.autoNG));
if ((t = FindArg ("-ng_player")) && (p = Args [t+1])) {
	strncpy (gameData.multi.autoNG.szPlayer, Args [t+1], 8);
	gameData.multi.autoNG.szPlayer [8] = '\0';
	}
if ((t = FindArg ("-ng_file")) && (p = Args [t+1])) {
	strncpy (gameData.multi.autoNG.szFile, Args [t+1], FILENAME_LEN - 1);
	gameData.multi.autoNG.szFile [FILENAME_LEN - 1] = '\0';
	}
if ((t = FindArg ("-ng_mission")) && (p = Args [t+1])) {
	strncpy (gameData.multi.autoNG.szMission, Args [t+1], 12);
	gameData.multi.autoNG.szMission [12] = '\0';
	}
if (t = FindArg ("-ng_level"))
	gameData.multi.autoNG.nLevel = NumArg (t, 1);
else
	gameData.multi.autoNG.nLevel = 1;
if ((t = FindArg ("-ng_name")) && (p = Args [t+1])) {
	strncpy (gameData.multi.autoNG.szName, Args [t+1], 80);
	gameData.multi.autoNG.szPlayer [80] = '\0';
	}
if ((t = FindArg ("-ng_ipaddr")) && (p = Args [t+1]))
	bHaveIp = stoip (Args [t+1], gameData.multi.autoNG.ipAddr);
if ((t = FindArg ("-ng_connect")) && (p = Args [t+1])) {
	strlwr (p);
	for (t = 0; pszTypes [t]; t++)
		if (*pszConnect [t] && !strcmp (p, pszConnect [t])) {
			gameData.multi.autoNG.uConnect = t;
			break;
			}
	}
if ((t = FindArg ("-ng_join")) && (p = Args [t+1])) {
	strlwr (p);
	gameData.multi.autoNG.bHost = !strcmp (p, "host");
	}
if ((t = FindArg ("-ng_type")) && (p = Args [t+1])) {
	strlwr (p);
	for (t = 0; pszTypes [t]; t++)
		if (!strcmp (p, pszTypes [t])) {
			gameData.multi.autoNG.uType = t;
			break;
			}
	}
if (t = FindArg ("-ng_team"))
	gameData.multi.autoNG.bTeam = NumArg (t, 1);
if (gameData.multi.autoNG.bHost)
	gameData.multi.autoNG.bValid = 
		*gameData.multi.autoNG.szPlayer &&
		*gameData.multi.autoNG.szName &&
		*gameData.multi.autoNG.szFile &&
		*gameData.multi.autoNG.szMission;
else
	gameData.multi.autoNG.bValid = 
		*gameData.multi.autoNG.szPlayer &&
		bHaveIp;
if (gameData.multi.autoNG.bValid)
	gameOptions [0].movies.nLevel = 0;
}

// ----------------------------------------------------------------------------

void EvalMultiplayerArgs (void)
{
	int t;

if (t = FindArg ("-friendlyfire"))
	extraGameInfo [0].bFriendlyFire = NumArg (t, 1);
if (t = FindArg ("-fixedrespawns"))
	extraGameInfo [0].bFixedRespawns = NumArg (t, 0);
if (t = FindArg ("-immortalpowerups"))
	extraGameInfo [0].bImmortalPowerups = NumArg (t, 0);
if (t = FindArg ("-spawndelay")) {
	extraGameInfo [0].nSpawnDelay = NumArg (t, 0);
	if (extraGameInfo [0].nSpawnDelay < 0)
		extraGameInfo [0].nSpawnDelay = 0;
	else if (extraGameInfo [0].nSpawnDelay > 60)
		extraGameInfo [0].nSpawnDelay = 60;
	extraGameInfo [0].nSpawnDelay *= 1000;
	}
if ((t = FindArg("-pps"))) {
	mpParams.nPPS = atoi(Args [t+1]);
	if (mpParams.nPPS < 1)
		mpParams.nPPS = 1;
	else if (mpParams.nPPS > 20)
		mpParams.nPPS = 20;
	}
if (FindArg ("-shortpackets"))
	mpParams.bShortPackets = 1;
if (FindArg ("-norankings"))
	gameOptions [0].multi.bNoRankings = 1;
if (t = FindArg ("-timeout"))
	gameOptions [0].multi.bTimeoutPlayers = NumArg (t, 1);
if (t = FindArg ("-noredundancy"))
	gameOptions [0].multi.bNoRedundancy = NumArg (t, 1);
if (t = FindArg ("-check_ports"))
	gameStates.multi.bCheckPorts = NumArg (t, 1);
EvalAutoNetGameArgs ();
}

// ----------------------------------------------------------------------------

void EvalMovieArgs (void)
{
	int	t;

if (t=FindArg ("-nomovies")) {
	gameOptions [0].movies.nLevel = 2 - NumArg (t, 2);
	if (gameOptions [0].movies.nLevel < 0)
		gameOptions [0].movies.nLevel = 0;
	else if (gameOptions [0].movies.nLevel > 2)
		gameOptions [0].movies.nLevel = 2;
	}
if (FindArg ("-subtitles"))
	gameOptions [0].movies.bSubTitles = 1;
if (t=FindArg ("-movie_quality"))
	gameOptions [0].movies.nQuality = NumArg (t, 0);
if (FindArg ("-lowresmovies"))
	gameOptions [0].movies.bHires = 0;
if (gameData.multi.autoNG.bValid)
	gameOptions [0].movies.nLevel = 0;
}

// ----------------------------------------------------------------------------

void EvalSoundArgs (void)
{
	int	t;

if (FindArg ("-sound22k"))
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_22K;
else if (FindArg ("-sound11k"))
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_11K;
else
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_22K;
#if USE_SDL_MIXER
if (t = FindArg ("-sdl_mixer"))
	gameOptions [0].sound.bUseSDLMixer = NumArg (t, 1);
#endif
if (t = FindArg ("-use_d1sounds"))
	gameOptions [0].sound.bUseD1Sounds = NumArg (t, 1);
if (t = FindArg ("-noredbook"))
	gameOptions [0].sound.bUseRedbook = 0;
}

// ----------------------------------------------------------------------------

void EvalMenuArgs (void)
{
	int	t;

if (!gameStates.app.bNostalgia)
	if (t = FindArg ("-menustyle"))
		gameOptions [0].menus.nStyle = NumArg (t, 1);
if (t = FindArg ("-fastmenus"))
	gameOptions [0].menus.bFastMenus = NumArg (t, 1);
if ((t = FindArg ("-altbg_alpha")) && *Args [t+1]) {
	gameOptions [0].menus.altBg.alpha = atof (Args [t+1]);
	if (gameOptions [0].menus.altBg.alpha < 0)
		gameOptions [0].menus.altBg.alpha = -1.0;
	else if ((gameOptions [0].menus.altBg.alpha == 0) || (gameOptions [0].menus.altBg.alpha > 1.0))
		gameOptions [0].menus.altBg.alpha = 1.0;
	}
if ((t = FindArg ("-altbg_brightness")) && *Args [t+1]) {
	gameOptions [0].menus.altBg.brightness = atof (Args [t+1]);
	if ((gameOptions [0].menus.altBg.brightness <= 0) || (gameOptions [0].menus.altBg.brightness > 1.0))
		gameOptions [0].menus.altBg.brightness = 1.0;
	}
if (t = FindArg ("-altbg_grayscale"))
	gameOptions [0].menus.altBg.grayscale = NumArg (t, 1);
if ((t = FindArg ("-altbg_name")) && *Args [t+1])
	strncpy (gameOptions [0].menus.altBg.szName, Args [t+1], sizeof (gameOptions [0].menus.altBg.szName));
if (t = FindArg ("-menu_hotkeys"))
	gameOptions [0].menus.nHotKeys = NumArg (t, 1);
else if (gameStates.app.bEnglish)
	gameOptions [0].menus.nHotKeys = 1;
}

// ----------------------------------------------------------------------------

void EvalGameplayArgs (void)
{
	int	t;

if (FindArg ("-noscreens"))
	gameOpts->gameplay.bSkipBriefingScreens = 1;
if (t = FindArg ("-secretsave"))
	gameOptions [0].gameplay.bSecretSave = NumArg (t, 1);
if (t = FindArg ("-nobotai"))
	gameStates.gameplay.bNoBotAI = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalInputArgs (void)
{
	int	t, i;

if (t = FindArg ("-mouselook"))
	extraGameInfo [0].bMouseLook = NumArg (t, 1);
if (t = FindArg ("-limitturnrate"))
	gameOptions [0].input.bLimitTurnRate = NumArg (t, 1);
#ifdef _DEBUG
if (t = FindArg ("-minturnrate"))
	gameOptions [0].input.nMinTurnRate = NumArg (t, 20);
#endif
SetMaxPitch (gameOptions [0].input.nMinTurnRate);
if (t = FindArg ("-joydeadzone"))
	for (i = 0; i < 4; i++)
		set_joy_deadzone (NumArg (t, 0), i);
if (t = FindArg ("-grabmouse"))
	gameStates.input.bGrabMouse = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalOglArgs (void)
{
	int	t;

if ((t = FindArg ("-gl_alttexmerge")))
	gameOpts->ogl.bGlTexMerge = NumArg (t, 1);
if ((t = FindArg("-gl_16bittextures")))
	gameOpts->ogl.bRgbaFormat = GL_RGB5_A1;
if ((t=FindArg("-gl_intensity4_ok")))
	gameOpts->ogl.bIntensity4 = NumArg (t, 1);
if ((t = FindArg("-gl_luminance4_alpha4_ok")))
	gameOpts->ogl.bLuminance4Alpha4 = NumArg (t, 1);
if ((t = FindArg("-gl_rgba2_ok")))
	gameOpts->ogl.bRgba2 = NumArg (t, 1);
if ((t = FindArg("-gl_readpixels_ok")))
	gameOpts->ogl.bReadPixels = NumArg (t, 1);
if ((t = FindArg("-gl_gettexlevelparam_ok")))
	gameOpts->ogl.bGetTexLevelParam = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalRenderArgs (void)
{
	int	t;

if ((t = FindArg ("-render_quality")) && *Args [t+1]) {
	gameOpts->render.nQuality = NumArg (t, 3);
	if (gameOpts->render.nQuality < 0)
		gameOpts->render.nQuality = 0;
	else if (gameOpts->render.nQuality > 3)
		gameOpts->render.nQuality = 3;
	}
if (t = FindArg ("-use_shaders"))
	gameOptions [0].render.bUseShaders = NumArg (t, 1);
if (t = FindArg ("-shadows"))
	extraGameInfo [0].bShadows = NumArg (t, 0);
if (t = FindArg ("-hires_textures"))
	gameOptions [0].render.textures.bUseHires = NumArg (t, 1);
if (t = FindArg ("-hires_models"))
	gameOptions [0].render.bHiresModels = NumArg (t, 1);
if (t=FindArg ("-render_all_segs"))
	gameOptions [0].render.bAllSegs = NumArg (t, 1);
if (t = FindArg ("-enable_lightmaps"))
	gameStates.render.color.bLightMapsOk = NumArg (t, 1);
if (t=FindArg ("-blend_background"))
	gameStates.render.bBlendBackground = NumArg (t, 1);
if (FindArg ("-automap_gamesres"))
	gameStates.render.bAutomapUseGameRes = 1;
if ((t=FindArg ("-tmap")))
	select_tmap (Args [t+1]);
else
	select_tmap (NULL);
if ((t = FindArg ("-maxfps"))) {
	t = NumArg (t, 150);
	if (t <= 0)
		t = 1;
	else if (t > 150)
		t = 150;
	gameOpts->render.nMaxFPS = t;
	}
#if RENDER2TEXTURE
if (t = FindArg ("-render2texture"))
	bUseRender2Texture = NumArg (t, 1);
#endif
#if OGL_POINT_SPRITES
if (t = FindArg ("-point_sprites"))
	gameStates.render.bPointSprites = NumArg (t, 1);
#endif
if (t = FindArg ("-vertex_arrays"))
	gameStates.render.bVertexArrays = NumArg (t, 1);
if (t = FindArg ("-nofade"))
	gameStates.render.bDisableFades = NumArg (t, 1);
if (t = FindArg ("-mathformat"))
	gameOpts->render.nDefMathFormat = NumArg (t, 2);
if (t = FindArg ("-render_opt"))
	gameOptions [0].render.bOptimize = NumArg (t, 1);
#ifdef _DEBUG
//if (gameOpts->ogl.bUseLighting)
//	gameStates.ogl.bUseTransform = 1;
//else
if (t = FindArg ("-gl_transform"))
	gameStates.ogl.bUseTransform = NumArg (t, 1);
#endif
}

// ----------------------------------------------------------------------------

void EvalAppArgs (void)
{
	int	t;

if (t = FindArg ("-nostalgia"))
	if (Args [t+1]) {
		gameStates.app.bNostalgia = NumArg (t, 0);
		if (gameStates.app.bNostalgia < 0)
			gameStates.app.bNostalgia = 0;
		else if (gameStates.app.bNostalgia > 3)
			gameStates.app.bNostalgia = 3;
		}
	else
		gameStates.app.bNostalgia = 1;
gameStates.app.iNostalgia = (gameStates.app.bNostalgia > 0);
gameOpts = gameOptions + gameStates.app.iNostalgia;

if (t = FindArg ("-progress_bars"))
	gameStates.app.bProgressBars = NumArg (t, 1);
if (t=FindArg ("-print_version"))
	PrintVersion ();
if (t=FindArg ("-altLanguage"))
	gameStates.app.bEnglish = (NumArg (t, 1) == 0);

if (t=FindArg ("-auto_hogfile")) {
	strcpy (szAutoHogFile, "missions/");
	strcat (szAutoHogFile, Args [t+1]);
	if (*szAutoHogFile && !strchr (szAutoHogFile, '.'))
		strcat (szAutoHogFile, ".hog");
	}
if (t=FindArg ("-auto_mission")) {
	char	c = *Args [++t];
	char	bDelim = ((c == '\'') || (c == '"'));

	strcpy (szAutoMission, &Args [t][bDelim]);
	if (bDelim)
		szAutoMission [strlen (szAutoMission) - 1] = '\0';
	if (*szAutoMission && !strchr (szAutoMission, '.'))
		strcat (szAutoMission, ".rl2");
	}
if (FindArg ("-debug"))
	con_threshold.value = (double)2;
else if (FindArg ("-verbose"))
	con_threshold.value = (double)1;
else
	con_threshold.value = (double)-1;
if (t = FindArg ("-autodemo")) {
	gameData.demo.bAuto = 1;
	strncpy (gameData.demo.fnAuto, *Args [t+1] ? Args [t+1] : "descent.dem", sizeof (gameData.demo.fnAuto));
	}
else
	gameData.demo.bAuto = 0;
gameStates.app.bMacData = FindArg ("-macdata");
}

// ----------------------------------------------------------------------------

void EvalArgs (void)
{
EvalAppArgs ();
EvalGameplayArgs ();
EvalLegacyArgs ();
EvalInputArgs ();
EvalMultiplayerArgs ();
EvalMenuArgs ();
EvalMovieArgs ();
EvalOglArgs ();
EvalRenderArgs ();
EvalSoundArgs ();
}

//------------------------------------------------------------------------------

void InitRenderOptions (int i)
{
if (i) {
	extraGameInfo [0].bPowerUpsOnRadar = 0;
	extraGameInfo [0].bRobotsOnRadar = 0;
	extraGameInfo [0].bUseCameras = 0;
	if (gameStates.app.bNostalgia > 2)
		extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].bWiggle = 1;
	extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].bDamageExplosions = 0;
	extraGameInfo [0].bThrusterFlames = 0;
	extraGameInfo [0].bShadows = 0;
	gameOptions [1].render.bAutomapAlwaysHires = 0;
	gameOptions [1].render.nMaxFPS = 150;
	gameOptions [1].render.bTransparentEffects = 0;
	gameOptions [1].render.bAllSegs = 0;
	gameOptions [1].render.bDynamicLight = 1;
	gameOptions [1].render.textures.bUseHires = 0;
	if (gameStates.app.bNostalgia > 2)
		gameOptions [1].render.nQuality = 0;
	gameOptions [1].render.bWireFrame = 0;
	gameOptions [1].render.bTextures = 1;
	gameOptions [1].render.bObjects = 1;
	gameOptions [1].render.bWalls = 1;
	gameOptions [1].render.bUseShaders = 1;
	gameOptions [1].render.bHiresModels = 0;
	gameOptions [0].render.bAutoTransparency = 0;
	gameOptions [1].render.bOptimize = 0;
	gameOptions [1].render.nMathFormat = 0;
	gameOptions [1].render.nDefMathFormat = 0;
	gameOptions [1].render.nMaxLights = 0;
	gameOptions [1].render.cameras.bFitToWall = 0;
	gameOptions [1].render.cameras.nFPS = 0;
	gameOptions [1].render.cameras.nSpeed = 0;
	gameOptions [1].render.cockpit.bHUD = 1;
	gameOptions [1].render.cockpit.bSplitHUDMsgs = 0;
	gameOptions [1].render.cockpit.bMouseIndicator = 1;
	gameOptions [1].render.cockpit.bTextGauges = 1;
	gameOptions [1].render.cockpit.bScaleGauges = 1;
	gameOptions [1].render.cockpit.bFlashGauges = 1;
	gameOptions [1].render.cockpit.bReticle = 1;
	gameOptions [1].render.cockpit.nWindowSize = 0;
	gameOptions [1].render.cockpit.nWindowZoom = 0;
	gameOptions [1].render.cockpit.nWindowPos = 1;
	gameOptions [1].render.color.bAmbientLight = 0;
	gameOptions [1].render.color.bCap = 0;
	gameOptions [1].render.color.bGunLight = 0;
	gameOptions [1].render.color.bMix = 1;
	gameOptions [1].render.color.bUseLightMaps = 0;
	gameOptions [1].render.color.bWalls = 0;
	gameOptions [1].render.color.nLightMapRange = 5;
	gameOptions [1].render.weaponIcons.bSmall = 0;
	gameOptions [1].render.weaponIcons.bShowAmmo = 0;
	gameOptions [1].render.weaponIcons.bEquipment = 0;
	gameOptions [1].render.weaponIcons.nSort = 0;
	gameOptions [1].render.textures.bUseHires = 0;
	gameOptions [1].render.textures.nQuality = 0;
	gameOptions [1].render.cockpit.bMissileView = 1;
	gameOptions [1].render.cockpit.bGuidedInMainView = 1;
	gameOptions [1].render.smoke.nScale = 0;
	gameOptions [1].render.smoke.nSize = 0;
	gameOptions [1].render.smoke.bPlayers = 0;
	gameOptions [1].render.smoke.bRobots = 0;
	gameOptions [1].render.smoke.bMissiles = 0;
	gameOptions [1].render.smoke.bDebris = 0;
	gameOptions [1].render.smoke.bCollisions = 0;
	gameOptions [1].render.smoke.bSort = 0;
	gameOptions [1].render.smoke.bDecreaseLag = 0;
	}
else {
	extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].bShadows = 0;
	gameOptions [0].render.bAutomapAlwaysHires = 0;
	gameOptions [0].render.nMaxFPS = 150;
	gameOptions [0].render.bTransparentEffects = 1;
	gameOptions [0].render.bAllSegs = 1;
	gameOptions [0].render.bDynamicLight = 1;
	gameOptions [0].render.nQuality = 3;
	gameOptions [0].render.bWireFrame = 0;
	gameOptions [0].render.bTextures = 1;
	gameOptions [0].render.bObjects = 1;
	gameOptions [0].render.bWalls = 1;
	gameOptions [0].render.bUseShaders = 1;
	gameOptions [0].render.bHiresModels = 1;
	gameOptions [0].render.bAutoTransparency = 1;
	gameOptions [0].render.nMathFormat = 0;
	gameOptions [0].render.nDefMathFormat = 0;
#ifdef _DEBUG
	gameOptions [0].render.nMaxLights = 1;
#else
	gameOptions [0].render.nMaxLights = 3;
#endif
	gameOptions [0].render.cameras.bFitToWall = 0;
	gameOptions [0].render.cameras.nFPS = 0;
	gameOptions [0].render.cameras.nSpeed = 5000;
	gameOptions [0].render.cockpit.bHUD = 1;
	gameOptions [0].render.cockpit.bSplitHUDMsgs = 0;
	gameOptions [0].render.cockpit.bMouseIndicator = 0;
	gameOptions [0].render.cockpit.bTextGauges = 1;
	gameOptions [0].render.cockpit.bScaleGauges = 1;
	gameOptions [0].render.cockpit.bFlashGauges = 1;
	gameOptions [0].render.cockpit.bReticle = 1;
	gameOptions [0].render.cockpit.nWindowSize = 0;
	gameOptions [0].render.cockpit.nWindowZoom = 0;
	gameOptions [0].render.cockpit.nWindowPos = 1;
	gameOptions [0].render.color.bAmbientLight = 0;
	gameOptions [0].render.color.bGunLight = 0;
	gameOptions [0].render.color.bMix = 1;
	gameOptions [0].render.color.bUseLightMaps = 0;
	gameOptions [0].render.color.bWalls = 0;
	gameOptions [0].render.color.nLightMapRange = 5;
	gameOptions [0].render.weaponIcons.bSmall = 0;
	gameOptions [0].render.weaponIcons.bShowAmmo = 1;
	gameOptions [0].render.weaponIcons.bEquipment = 1;
	gameOptions [0].render.weaponIcons.nSort = 1;
	gameOptions [0].render.weaponIcons.alpha = 4;
	gameOptions [0].render.textures.bUseHires = 0;
	gameOptions [0].render.textures.nQuality = 3;
#if APPEND_LAYERED_TEXTURES
	gameOptions [0].render.bOptimize = 1;
#else
	gameOptions [0].render.bOptimize = 0;
#endif
	gameOptions [0].render.cockpit.bMissileView = 1;
	gameOptions [0].render.cockpit.bGuidedInMainView = 1;
	gameOptions [0].render.smoke.nScale = 2;
	gameOptions [0].render.smoke.nSize = 1;
	gameOptions [0].render.smoke.bPlayers = 1;
	gameOptions [0].render.smoke.bRobots = 1;
	gameOptions [0].render.smoke.bMissiles = 1;
	gameOptions [0].render.smoke.bDebris = 1;
	gameOptions [0].render.smoke.bCollisions = 1;
	gameOptions [0].render.smoke.bSort = 0;
	gameOptions [0].render.smoke.bDecreaseLag = 1;
	}
}

//------------------------------------------------------------------------------

void InitGameplayOptions (int i)
{
if (i) {
	extraGameInfo [0].nSpawnDelay = 0;
	extraGameInfo [0].nFusionPowerMod = 2;
	extraGameInfo [0].bFixedRespawns = 0;
	extraGameInfo [0].bRobotsHitRobots = 0;
	extraGameInfo [0].bDualMissileLaunch = 0;
	extraGameInfo [0].bDropAllMissiles = 0;
	extraGameInfo [0].bImmortalPowerups = 0;
	extraGameInfo [0].bMultiBosses = 0;
	extraGameInfo [0].bSmartWeaponSwitch = 0;
	extraGameInfo [0].bFluidPhysics = 0;
	extraGameInfo [0].nWeaponDropMode = 0;
	extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].nZoomMode = 0;
	gameOptions [1].gameplay.nAutoSelectWeapon = 2;
	gameOptions [1].gameplay.bSecretSave = 0;
	gameOptions [1].gameplay.bTurboMode = 0;
	gameOptions [1].gameplay.bFastRespawn = 0;
	gameOptions [1].gameplay.bAutoLeveling = 1;
	gameOptions [1].gameplay.bDefaultLeveling = 1;
	gameOptions [1].gameplay.bEscortHotKeys = 1;
	gameOptions [1].gameplay.nPlayerDifficultyLevel = DEFAULT_DIFFICULTY;
	gameOptions [1].gameplay.bSkipBriefingScreens = 0;
	gameOptions [1].gameplay.bHeadlightOn = 1;
	gameOptions [1].gameplay.bShieldWarning = 0;
	gameOptions [1].gameplay.bInventory = 0;
	}
else {
	gameOptions [0].gameplay.nAutoSelectWeapon = 2;
	gameOptions [0].gameplay.bSecretSave = 0;
	gameOptions [0].gameplay.bTurboMode = 0;
	gameOptions [0].gameplay.bFastRespawn = 0;
	gameOptions [0].gameplay.bAutoLeveling = 1;
	gameOptions [0].gameplay.bDefaultLeveling = 1;
	gameOptions [0].gameplay.bEscortHotKeys = 1;
	gameOptions [0].gameplay.nPlayerDifficultyLevel = DEFAULT_DIFFICULTY;
	gameOptions [0].gameplay.bSkipBriefingScreens = 0;
	gameOptions [0].gameplay.bHeadlightOn = 0;
	gameOptions [0].gameplay.bShieldWarning = 0;
	gameOptions [1].gameplay.bInventory = 0;
	}
}

//------------------------------------------------------------------------------

void InitMovieOptions (int i)
{
if (i) {
	gameOptions [1].movies.bHires = 1;
	gameOptions [1].movies.nQuality = 0;
	gameOptions [1].movies.nLevel = 2;
	gameOptions [1].movies.bResize = 0;
	gameOptions [1].movies.bFullScreen = 0;
	gameOptions [1].movies.bSubTitles = 1;
	}
else {
	gameOptions [0].movies.bHires = 1;
	gameOptions [0].movies.nQuality = 0;
	gameOptions [0].movies.nLevel = 2;
	gameOptions [0].movies.bResize = 1;
	gameOptions [1].movies.bFullScreen = 1;
	gameOptions [0].movies.bSubTitles = 1;
	}
}

//------------------------------------------------------------------------------

void InitLegacyOptions (int i)
{
if (i) {
	gameOptions [1].legacy.bInput = 0;
	gameOptions [1].legacy.bZBuf = 0;
	gameOptions [1].legacy.bFuelCens = 0;
	gameOptions [1].legacy.bMouse = 0;
	gameOptions [1].legacy.bHomers = 0;
	gameOptions [1].legacy.bRender = 0;
	gameOptions [1].legacy.bSwitches = 0;
	gameOptions [1].legacy.bWalls = 0;
	}
else {
	gameOptions [0].legacy.bInput = 0;
	gameOptions [0].legacy.bZBuf = 0;
	gameOptions [0].legacy.bFuelCens = 0;
	gameOptions [0].legacy.bMouse = 0;
	gameOptions [0].legacy.bHomers = 0;
	gameOptions [0].legacy.bRender = 0;
	gameOptions [0].legacy.bSwitches = 0;
	gameOptions [0].legacy.bWalls = 0;
	}
}

//------------------------------------------------------------------------------

void InitSoundOptions (int i)
{
if (i) {
	gameOptions [1].sound.bUseRedbook = 1;
	gameOptions [1].sound.digiSampleRate = SAMPLE_RATE_22K;
#if USE_SDLMIXER
	gameOptions [1].sound.bUseSDLMixer = 1;
#else
	gameOptions [1].sound.bUseSDLMixer = 0;
#endif
	}
else {
	gameOptions [0].sound.bUseRedbook = 1;
	gameOptions [1].sound.digiSampleRate = SAMPLE_RATE_22K;
#if USE_SDLMIXER
	gameOptions [0].sound.bUseSDLMixer = 1;
#else
	gameOptions [0].sound.bUseSDLMixer = 0;
#endif
	}
}

//------------------------------------------------------------------------------

void InitInputOptions (int i)
{
if (i) {
	extraGameInfo [0].bMouseLook = 0;
	gameOptions [1].input.bLimitTurnRate = 1;
	gameOptions [1].input.nMinTurnRate = 20;	//turn time for a 360 deg rotation around a single ship axis in 1/10 sec units
	if (gameOptions [1].input.bUseJoystick)
		gameOptions [1].input.bUseMouse = 0;
	gameOptions [1].input.bSyncMouseAxes = 1;
	gameOptions [1].input.bJoyMouse = 0;
	gameOptions [1].input.bSyncJoyAxes = 1;
	gameOptions [1].input.bUseKeyboard = 1;
	gameOptions [1].input.bUseHotKeys = 1;
	gameOptions [1].input.keyRampScale = 100;
	gameOptions [1].input.bRampKeys [0] =
	gameOptions [1].input.bRampKeys [1] =
	gameOptions [1].input.bRampKeys [2] = 0;
	gameOptions [1].input.bLinearJoySens = 1;
	gameOptions [1].input.mouseSensitivity [0] =
	gameOptions [1].input.mouseSensitivity [1] =
	gameOptions [1].input.mouseSensitivity [2] = 8;
	gameOptions [1].input.joySensitivity [0] =
	gameOptions [1].input.joySensitivity [1] =
	gameOptions [1].input.joySensitivity [2] =
	gameOptions [1].input.joySensitivity [3] = 8;
	gameOptions [1].input.joyDeadZones [0] =
	gameOptions [1].input.joyDeadZones [1] =
	gameOptions [1].input.joyDeadZones [2] =
	gameOptions [1].input.joyDeadZones [3] = 10;
	}
else {
	gameOptions [0].input.bLimitTurnRate = 1;
	gameOptions [0].input.nMinTurnRate = 20;	//turn time for a 360 deg rotation around a single ship axis in 1/10 sec units
	gameOptions [0].input.nMaxPitch = 0;
	gameOptions [0].input.bLinearJoySens = 0;
	gameOptions [0].input.keyRampScale = 100;
	gameOptions [0].input.bRampKeys [0] =
	gameOptions [0].input.bRampKeys [1] =
	gameOptions [0].input.bRampKeys [2] = 0;
	gameOptions [0].input.bUseMouse = 1;
	gameOptions [0].input.bUseJoystick = 0;
	gameOptions [0].input.bSyncMouseAxes = 1;
	gameOptions [0].input.bJoyMouse = 0;
	gameOptions [0].input.bSyncJoyAxes = 1;
	gameOptions [0].input.bUseKeyboard = 1;
	gameOptions [0].input.bUseHotKeys = 1;
	gameOptions [0].input.mouseSensitivity [0] =
	gameOptions [0].input.mouseSensitivity [1] =
	gameOptions [0].input.mouseSensitivity [2] = 8;
	gameOptions [0].input.joySensitivity [0] =
	gameOptions [0].input.joySensitivity [1] =
	gameOptions [0].input.joySensitivity [2] =
	gameOptions [0].input.joySensitivity [3] = 8;
	gameOptions [0].input.joyDeadZones [0] =
	gameOptions [0].input.joyDeadZones [1] =
	gameOptions [0].input.joyDeadZones [2] =
	gameOptions [0].input.joyDeadZones [3] = 10;
	}
}

// ----------------------------------------------------------------------------

void InitMultiplayerOptions (int i)
{
if (i) {
	extraGameInfo [0].bFriendlyFire = 1;
	extraGameInfo [0].bInhibitSuicide = 0;
	extraGameInfo [1].bMouseLook = 0;
	extraGameInfo [1].bDualMissileLaunch = 0;
	extraGameInfo [0].bAutoBalanceTeams = 0;
	extraGameInfo [1].bRotateLevels = 0;
	extraGameInfo [1].bDisableReactor = 0;
	gameOptions [1].multi.bNoRankings = 0;
	gameOptions [1].multi.bTimeoutPlayers = 1;
	gameOptions [1].multi.bUseMacros = 0;
	gameOptions [1].multi.bNoRedundancy = 0;
	}
else {
	gameOptions [0].multi.bNoRankings = 0;
	gameOptions [0].multi.bTimeoutPlayers = 1;
	gameOptions [0].multi.bUseMacros = 0;
	gameOptions [0].multi.bNoRedundancy = 0;
	}
}

// ----------------------------------------------------------------------------

void InitMenuOptions (int i)
{
if (i) {
	gameOptions [1].menus.nStyle = 0;
	gameOptions [1].menus.bFastMenus = 0;
	gameOptions [1].menus.bSmartFileSearch = 0;
	gameOptions [1].menus.bShowLevelVersion = 0;
	gameOptions [1].menus.altBg.alpha = 0;
	gameOptions [1].menus.altBg.brightness = 0;
	gameOptions [1].menus.altBg.grayscale = 0;
	gameOptions [1].menus.nHotKeys = 0;
	strcpy (gameOptions [0].menus.altBg.szName, "");
	}
else {
	gameOptions [0].menus.nStyle = 0;
	gameOptions [0].menus.bFastMenus = 0;
	gameOptions [0].menus.bSmartFileSearch = 1;
	gameOptions [0].menus.bShowLevelVersion = 0;
	gameOptions [0].menus.altBg.alpha = 0.75;
	gameOptions [0].menus.altBg.brightness = 0.5;
	gameOptions [0].menus.altBg.grayscale = 0;
	gameOptions [0].menus.nHotKeys = gameStates.app.bEnglish ? 1 : -1;
	strcpy (gameOptions [0].menus.altBg.szName, "menubg.tga");
	}
}

// ----------------------------------------------------------------------------

void InitOglOptions (int i)
{
if (i) {
	gameOptions [1].ogl.bUseLighting = 0;
	gameOptions [1].ogl.bLightObjects = 0;
	gameOptions [1].ogl.nMaxLights = MAX_NEAREST_LIGHTS / 2;
	gameOptions [1].ogl.bSetGammaRamp = 0;
	gameOptions [1].ogl.bRgbaFormat = 4;
	gameOptions [1].ogl.bIntensity4 = 1;
	gameOptions [1].ogl.bLuminance4Alpha4 = 1;
	gameOptions [1].ogl.bRgba2 = 1;
	gameOptions [1].ogl.bReadPixels = 1;
	gameOptions [1].ogl.bGetTexLevelParam = 1;
	gameOptions [1].ogl.bVoodooHack = 0;
#ifdef GL_ARB_multitexture
	gameOptions [1].ogl.bArbMultiTexture = 0;
#endif
#ifdef GL_SGIS_multitexture
	gameOptions [1].ogl.bSgisMultiTexture = 0;
#endif
	gameOptions [1].ogl.bGlTexMerge = 0;
	}
else {
#ifdef _DEBUG
	gameOptions [0].ogl.bUseLighting = 0;
#else
	gameOptions [0].ogl.bUseLighting = 0;
#endif
	gameOptions [0].ogl.bLightObjects = 0;
	gameOptions [0].ogl.nMaxLights = MAX_NEAREST_LIGHTS / 2;
	gameOptions [0].ogl.bSetGammaRamp = 0;
	gameOptions [0].ogl.bRgbaFormat = 4;
	gameOptions [0].ogl.bIntensity4 = 1;
	gameOptions [0].ogl.bLuminance4Alpha4 = 1;
	gameOptions [0].ogl.bRgba2 = 1;
	gameOptions [0].ogl.bReadPixels = 1;
	gameOptions [0].ogl.bGetTexLevelParam = 1;
	gameOptions [0].ogl.bVoodooHack = 0;
#ifdef GL_ARB_multitexture
	gameOptions [0].ogl.bArbMultiTexture = 0;
#endif
#ifdef GL_SGIS_multitexture
	gameOptions [0].ogl.bSgisMultiTexture = 0;
#endif
	gameOptions [0].ogl.bGlTexMerge = 0;
	}
}

// ----------------------------------------------------------------------------

void InitAppOptions (int i)
{
if (i) {
	gameOptions [1].app.nVersionFilter = 2;
	gameOptions [1].app.bDemoData = 0;
	gameOptions [1].app.bSinglePlayer = 0;
	gameOptions [1].app.bExpertMode = 0;
	gameOptions [1].app.nScreenShotInterval = 0;
	}
else {
	gameOptions [0].app.nVersionFilter = 2;
	gameOptions [0].app.bDemoData = 1;
	gameOptions [0].app.bSinglePlayer = 0;
	gameOptions [0].app.bExpertMode = 1;
	gameOptions [0].app.nScreenShotInterval = 0;
	}
}

// ----------------------------------------------------------------------------

void InitGameOptions (int i)
{
if (i)
	if (gameStates.app.bNostalgia)
		gameOptions [1] = gameOptions [0];
	else
		return;
else
	memset (gameOptions, 0, sizeof (gameOptions));
InitInputOptions (i);
InitGameplayOptions (i);
InitRenderOptions (i);
InitMultiplayerOptions (i);
InitMenuOptions (i);
InitSoundOptions (i);
InitMovieOptions (i);
InitOglOptions (i);
InitLegacyOptions (i);
InitGameplayOptions (i);
}

// ----------------------------------------------------------------------------

void InitGameplayStates (void)
{
gameStates.gameplay.bSpeedBoost = 0;
gameStates.gameplay.bFinalBossIsDead = 0;
gameStates.gameplay.bHaveSmartMines = 1;
gameStates.gameplay.bMineDestroyed = 0;
gameStates.gameplay.bNoBotAI = 0;
gameStates.gameplay.nShieldFlash = 0;
gameStates.gameplay.nPlayerSpeed = 0;
gameStates.gameplay.seismic.nMagnitude = 0;
gameStates.gameplay.seismic.nStartTime = 0;
gameStates.gameplay.seismic.nEndTime = 0;
gameStates.gameplay.seismic.nLevel = 0;
gameStates.gameplay.seismic.nShakeFrequency = 0;
gameStates.gameplay.seismic.nShakeDuration = 0;
gameStates.gameplay.seismic.nSound = SOUND_SEISMIC_DISTURBANCE_START;
gameStates.gameplay.seismic.bSound = 0;
gameStates.gameplay.seismic.nVolume = 0;
}

// ----------------------------------------------------------------------------

void InitInputStates (void)
{
gameStates.input.nMouseType = -1;
gameStates.input.nJoyType = -1;
gameStates.input.bCybermouseActive = 0;
#ifdef RELEASE
gameStates.input.bGrabMouse = 1;
#else
gameStates.input.bGrabMouse = 0;
#endif
gameStates.input.bSkipControls = 0;
gameStates.input.bControlsSkipFrame = 0;
gameStates.input.bKeepSlackTime = 0;
gameStates.input.nCruiseSpeed = 0;
}

// ----------------------------------------------------------------------------

void InitMenuStates (void)
{
gameStates.menus.bInitBG = 1;
gameStates.menus.nInMenu = 0;
gameStates.menus.bHires = 1;
gameStates.menus.bHiresAvailable = 1;
gameStates.menus.bDrawCopyright = 0;
gameStates.menus.bFullScreen = 0;
gameStates.menus.bReordering = 0;
}

// ----------------------------------------------------------------------------

void InitMovieStates (void)
{
gameStates.movies.nRobots = 0;
gameStates.movies.bIntroPlayed = 0;
}

// ----------------------------------------------------------------------------

void InitMultiplayerStates (void)
{
gameStates.multi.nGameType = 0;
gameStates.multi.nGameSubType = 0;
gameStates.multi.bUseTracker = 0;
gameStates.multi.bServer = 1;
gameStates.multi.bTryAutoDL = 0;
gameStates.multi.bHaveLocalAddress = 0;
gameStates.multi.bSurfingNet = 0;
gameStates.multi.bCheckPorts = 0;
}

// ----------------------------------------------------------------------------

void InitGfxStates (void)
{
gameStates.gfx.bInstalled = 0;
gameStates.gfx.nStartScrSize = 2;
gameStates.gfx.nStartScrMode = -1;
}

// ----------------------------------------------------------------------------

void InitOglStates (void)
{
gameStates.ogl.bInitialized = 0;
gameStates.ogl.bShadersOk = 0;
gameStates.ogl.bVoodooHack = 0;
gameStates.ogl.bBrightness = 0;
gameStates.ogl.nContrast = 8;
gameStates.ogl.bFullScreen = 0;
gameStates.ogl.bUseTransform = 0;
gameStates.ogl.bDoPalStep = 0;
gameStates.ogl.nColorBits = 32;
gameStates.ogl.nDepthBits = 24;
gameStates.ogl.bEnableTexture2D = -1;
gameStates.ogl.bEnableTexClamp = -1;
gameStates.ogl.bEnableScissor = 0;
gameStates.ogl.bNeedMipMaps = 0;
gameStates.ogl.texMinFilter = GL_NEAREST;
gameStates.ogl.texMinFilter = GL_NEAREST;
gameStates.ogl.nTexMagFilterState = -1,
gameStates.ogl.nTexMinFilterState = -1;
gameStates.ogl.nTexEnvModeState = -1,
gameStates.ogl.nLastX =
gameStates.ogl.nLastY =
gameStates.ogl.nLastW =
gameStates.ogl.nLastH = 1;
gameStates.ogl.bAntiAliasing = 0;
gameStates.ogl.bAntiAliasingOk = 0;
}

// ----------------------------------------------------------------------------

void InitRenderStates (void)
{
gameStates.render.bAltShadows = 0;
gameStates.render.bQueryOcclusion = 0;
gameStates.render.bPointSprites = 0;
gameStates.render.bVertexArrays = 0;
gameStates.render.bExternalView = 0;
gameStates.render.bTopDownRadar = 0;
gameStates.render.bAutomapUseGameRes = 1;
gameStates.render.bDisableFades = 0;
gameStates.render.bDropAfterburnerBlob = 0;
gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
gameStates.render.frameRate.name = "r_framerate";
cvar_registervariable (&gameStates.render.frameRate);
gameStates.render.frameRate.string = "0";
gameStates.render.frameRate.value = 0;
gameStates.render.cameras.bActive = 0;
gameStates.render.color.bLightMapsOk = 1;
gameStates.render.nZoomFactor = F1_0;
gameStates.render.nMinZoomFactor = F1_0;
gameStates.render.nMaxZoomFactor = F1_0 * 5;
gameStates.render.textures.bGlsTexMergeOk = 0;
gameStates.render.textures.bHaveMaskShader = 0;
gameStates.render.bBlendBackground = 0;
gameStates.render.fonts.bHires = 0;
gameStates.render.fonts.bHiresAvailable = 0;
gameStates.render.fonts.bInstalled = 0;
gameStates.render.nFlashScale = 0;
gameStates.render.nFlashRate = FLASH_CYCLE_RATE;
gameStates.render.bOutsideMine = 0;
gameStates.render.bExtExplPlaying = 0;
gameStates.render.bDoAppearanceEffect = 0;
gameStates.render.glFOV = DEFAULT_FOV;
gameStates.render.glAspect = 1.0f;
gameStates.render.bDetriangulation = 0;
gameStates.render.nInterpolationMethod = 0;
gameStates.render.bTMapFlat = 0;
gameStates.render.nLighting = 1;
gameStates.render.bTransparency = 0;
gameStates.render.bHaveStencilBuffer = 0;
gameStates.render.nRenderPass = -1;
gameStates.render.nShadowPass = 0;
gameStates.render.bShadowMaps = 0;
gameStates.render.bHeadlightOn = 0;
gameStates.render.bPaletteFadedOut = 0;
gameStates.render.cockpit.bShowPingStats = 0;
gameStates.render.cockpit.nMode = CM_FULL_COCKPIT;
gameStates.render.cockpit.nNextMode = -1;
gameStates.render.cockpit.nModeSave = -1;
gameStates.render.cockpit.bRedraw = 0;
gameStates.render.cockpit.bBigWindowSwitch = 0;
gameStates.render.detail.nRenderDepth = DEFAULT_RENDER_DEPTH;
gameStates.render.detail.nObjectComplexity = 2; 
gameStates.render.detail.nObjectDetail = 2;
gameStates.render.detail.nWallDetail = 2; 
gameStates.render.detail.nWallRenderDepth = 2; 
gameStates.render.detail.nDebrisAmount = 2; 
}

// ----------------------------------------------------------------------------

void InitSoundStates (void)
{
gameStates.sound.nCurrentSong = 0;
gameStates.sound.bRedbookEnabled = 1;
gameStates.sound.bRedbookPlaying = 0;
gameStates.sound.bWasRecording = 0;
gameStates.sound.bDontStartObjects = 0;
gameStates.sound.nConquerWarningSoundChannel = -1;
gameStates.sound.nMaxSoundChannels = 2;
gameStates.sound.digi.nMaxChannels = 16;
gameStates.sound.digi.nNextChannel = 0;
gameStates.sound.digi.nVolume = SOUND_MAX_VOLUME;
gameStates.sound.digi.bInitialized = 0;
gameStates.sound.digi.nNextSignature=0;
gameStates.sound.digi.nActiveObjects=0;
gameStates.sound.digi.bSoundsInitialized = 0;
gameStates.sound.digi.bLoMem = 0;
gameStates.sound.digi.nLoopingSound = -1;
gameStates.sound.digi.nLoopingVolume = 0;
gameStates.sound.digi.nLoopingStart = -1;
gameStates.sound.digi.nLoopingEnd = -1;
gameStates.sound.digi.nLoopingChannel = -1;
}

// ----------------------------------------------------------------------------

void InitVideoStates (void)
{
gameStates.video.nDisplayMode = 2;
gameStates.video.nDefaultDisplayMode = 2;
gameStates.video.nScreenMode = (u_int32_t) -1;
gameStates.video.nLastScreenMode = (u_int32_t) -1;
gameStates.video.nWidth = -1;
gameStates.video.nHeight = -1;
gameStates.video.bFullScreen = -1;
}

// ----------------------------------------------------------------------------

void InitAppStates (void)
{
gameStates.app.bHaveExtraData = 0;
gameStates.app.bHaveExtraMovies = 0;
gameStates.app.nExtGameStatus = 1;
gameStates.app.nFunctionMode = FMODE_MENU;
gameStates.app.nLastFuncMode = -1;
gameStates.app.nCriticalError = 0;
gameStates.app.bNostalgia = 0;
gameStates.app.iNostalgia = 0;
gameStates.app.bD2XLevel = 0;
gameStates.app.bGameRunning = 0;
gameStates.app.bGameAborted = 0;
gameStates.app.bGameSuspended = 0;
gameStates.app.bEnterGame = 0;
gameStates.app.bLunacy = 0;
gameStates.app.bHaveExtraGameInfo [0] = 1;
gameStates.app.bHaveExtraGameInfo [1] = 0;
gameStates.app.nSDLTicks = -1;
#ifdef _DEBUG
gameStates.app.bEnglish = 1;
#else
gameStates.app.bEnglish = 1;
#endif
gameStates.app.bMacData = 0;
gameStates.app.bD1Model = 0;
gameStates.app.bD1Data = 0;
gameStates.app.bD1Mission = 0;
gameStates.app.bHaveD1Data = 0;
gameStates.app.bHaveD1Textures = 0;
gameStates.app.bEndLevelDataLoaded = 0;
gameStates.app.bEndLevelSequence = 0;
gameStates.app.bPlayerIsDead = 0;
gameStates.app.bPlayerExploded = 0;
gameStates.app.bPlayerEggsDropped = 0;
gameStates.app.bDeathSequenceAborted = 0;
gameStates.app.bUseDefaults = 0;
gameStates.app.nCompSpeed = 3;
gameStates.app.bPlayerFiredLaserThisFrame = 0;
gameStates.app.bAutoMap = 0;
gameStates.app.bConfigMenu = 0;
gameStates.app.bAutoRunMission = 0;
gameStates.entropy.bConquering = 0;
gameStates.entropy.bConquerWarning = 0;
gameStates.entropy.nTimeLastMoved = -1;
gameStates.app.bFirstSecretVisit = 1;
gameStates.app.nDifficultyLevel = DEFAULT_DIFFICULTY;
gameStates.app.nDetailLevel = NUM_DETAIL_LEVELS - 1;
gameStates.app.nBaseCtrlCenExplTime = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;
gameStates.app.bDebugSpew = 1;
gameStates.app.bProgressBars = 1;
gameStates.app.cheats.bEnabled = 0;
gameStates.app.cheats.bTurboMode = 0;
gameStates.app.cheats.bMonsterMode = 0;
gameStates.app.cheats.bLaserRapidFire = 0;
gameStates.app.cheats.bRobotsKillRobots = 0;
gameStates.app.cheats.bJohnHeadOn = 0;
gameStates.app.cheats.bHomingWeapons = 0;
gameStates.app.cheats.bBouncingWeapons = 0;
gameStates.app.cheats.bMadBuddy = 0;
gameStates.app.cheats.bAcid = 0;
gameStates.app.cheats.bPhysics = 0;
gameStates.app.cheats.bRobotsFiring = 1;
gameStates.app.cheats.bD1CheatsEnabled = 0;
gameStates.app.cheats.nUnlockLevel = 0;
gameStates.limitFPS.bControls = 1;
gameStates.limitFPS.bJoystick = !gameStates.limitFPS.bControls;
gameStates.limitFPS.bFusion = 0;
gameStates.limitFPS.bCountDown = 1;
gameStates.limitFPS.bSeismic = 1;
gameStates.limitFPS.bHomers = 1;
gameStates.limitFPS.bOmega = 1;
}

// ----------------------------------------------------------------------------

void InitGameStates (void)
{
memset (&gameStates, 0, sizeof (gameStates));
CheckEndian ();
InitGameConfig ();
InitAppStates ();
InitGameplayStates ();
InitInputStates ();
InitMenuStates ();
InitMovieStates ();
InitMultiplayerStates ();
InitGfxStates ();
InitOglStates ();
InitRenderStates ();
InitSoundStates ();
InitVideoStates ();
}

//------------------------------------------------------------------------------

void SetDataVersion (int v)
{
gameStates.app.bD1Data = (v < 0) ? gameStates.app.bD1Mission && gameStates.app.bHaveD1Data : v;
gameData.pig.tex.pBitmaps = gameData.pig.tex.bitmaps [gameStates.app.bD1Data];
gameData.pig.tex.pAltBitmaps = gameData.pig.tex.altBitmaps [gameStates.app.bD1Data];
gameData.pig.tex.pBmIndex = gameData.pig.tex.bmIndex [gameStates.app.bD1Data];
gameData.pig.tex.pBitmapFiles = gameData.pig.tex.bitmapFiles [gameStates.app.bD1Data];
gameData.pig.tex.pTMapInfo = gameData.pig.tex.tMapInfo [gameStates.app.bD1Data];
gameData.pig.snd.pSounds = gameData.pig.snd.sounds [gameStates.app.bD1Data];
gameData.eff.pEffects = gameData.eff.effects [gameStates.app.bD1Data];
gameData.eff.pVClips = gameData.eff.vClips [gameStates.app.bD1Data];
gameData.walls.pAnims = gameData.walls.anims [gameStates.app.bD1Data];
gameData.bots.pInfo = gameData.bots.info [gameStates.app.bD1Data];
}

// ----------------------------------------------------------------------------

void InitGameData (void)
{
	int	i;

memset (&gameData, 0, sizeof (gameData));
gameData.reactor.nDeadObj = -1;
gameData.bots.nCamBotId = -1;
gameData.bots.nCamBotModel = -1;
gameData.pig.ship.player = &gameData.pig.ship.only;
gameData.multi.nPlayers = 1;
gameData.multi.nLocalPlayer = 0;
gameData.multi.nMaxPlayers = -1;
gameData.multi.nPlayerPositions = -1;
gameData.missions.nCurrentLevel = 0;
gameData.missions.nLastMission = -1;
gameData.missions.nLastLevel = -1;
gameData.missions.nLastSecretLevel = -1;
gameData.marker.nHighlight = -1;
gameData.missions.nEnteredFromLevel = -1;
gameData.marker.fScale = 2.0f;
gameData.time.xFrame = 0x1000;
gameData.app.nGameMode = GM_GAME_OVER;
gameData.render.nPaletteGamma = -1;
gameData.reactor.bDestroyed = 0;
gameData.reactor.nStrength = 0;
gameData.reactor.countdown.nTimer = 0;
gameData.reactor.countdown.nSecsLeft = 0;
gameData.reactor.countdown.nTotalTime = 0;
gameData.render.transpColor = DEFAULT_TRANSPARENCY_COLOR; //transparency color bitmap index
gameData.render.sphere.nFaceNodes = 4; //tesselate using quads
gameData.boss.nCloakStartTime = 0;
gameData.boss.nCloakEndTime = 0;
gameData.boss.nLastTeleportTime = 0;
gameData.boss.nTeleportInterval = F1_0 * 8;
gameData.boss.nCloakInterval = F1_0 * 10;                    //    Time between cloaks
gameData.boss.nCloakDuration = BOSS_CLOAK_DURATION;
gameData.boss.nLastGateTime = 0;
gameData.boss.nGateInterval = F1_0 * 6;
gameData.ai.bInitialized = 0;
gameData.ai.bEvaded = 0;
gameData.ai.bEnableAnimation = 1;
gameData.ai.bInfoEnabled = 0;
gameData.ai.nAwarenessEvents = 0;
gameData.ai.nDistToLastPlayerPosFiredAt = 0;
gameData.ai.freePointSegs = gameData.ai.pointSegs;
gameData.menu.colorOverride = 0;
gameData.matCens.xFuelRefillSpeed = i2f (1);
gameData.matCens.xFuelGiveAmount = i2f (25);
gameData.matCens.xFuelMaxAmount = i2f (100);
gameData.matCens.xEnergyToCreateOneRobot = i2f (1);
gameData.escort.nMaxLength = 200;
gameData.escort.nKillObject = -1;
gameData.escort.xLastPathCreated = 0;
gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
gameData.escort.nSpecialGoal = -1;
gameData.escort.nGoalIndex = -1;
gameData.escort.bMsgsSuppressed = 0;
gameData.objs.nNextSignature = 1;
memset (gameData.objs.guidedMissileSig, 0xff, sizeof (gameData.objs.guidedMissileSig));
gameData.render.morph.xRate = MORPH_RATE;
gameData.render.ogl.nSrcBlend = GL_SRC_ALPHA;
gameData.render.ogl.nDestBlend = GL_ONE_MINUS_SRC_ALPHA;
memset (&gameData.render.lights.ogl, 0xff, sizeof (gameData.render.lights.ogl));
gameData.render.lights.ogl.nLights = 0;
gameData.models.nSimpleModelThresholdScale = 5;
gameData.pig.tex.nFirstMultiBitmap = -1;
strcpy (gameData.escort.szName, "GUIDE-BOT");
strcpy (gameData.escort.szRealName, "GUIDE-BOT");
gameData.thief.xReInitTime = 0x3f000000;
for (i = 0; i < NDL; i++)
	gameData.thief.xWaitTimes [i] = F1_0 * (30 - i * 5);
gameData.hud.bPlayerMessage = 1;
gameData.hud.msgs [0].nColor = 
gameData.hud.msgs [1].nColor = -1;
gameData.render.terrain.uvlList [0][1].u = f1_0;
gameData.render.terrain.uvlList [0][2].v = f1_0;
gameData.render.terrain.uvlList [1][0].u = f1_0;
gameData.render.terrain.uvlList [1][1].u = f1_0;
gameData.render.terrain.uvlList [1][1].v = f1_0;
gameData.render.terrain.uvlList [1][2].v = f1_0;
SetSpherePulse (&gameData.render.sphere.pulse, 0.02f, 0.5f);
UseSpherePulse (&gameData.render.sphere.pulse);
gameData.smoke.iFreeSmoke = -1;
gameData.smoke.iUsedSmoke = -1;
InitEndLevelData ();
SetDataVersion (-1);
}

// ----------------------------------------------------------------------------

void PrintBanner (void)
{
#if (defined (_WIN32) || defined (__unix__))
con_printf(CON_NORMAL, "\nDESCENT 2 %s v%d.%d.%d\n", VERSION_TYPE, D2X_MAJOR, D2X_MINOR, D2X_MICRO);
#elif defined(__macosx__)
con_printf(CON_NORMAL, "\nDESCENT 2 %s -- %s\n", VERSION_TYPE, DESCENT_VERSION);
#endif
if (gameHogFiles.D2XHogFiles.bInitialized)
	con_printf ((int) CON_NORMAL, "  Vertigo Enhanced\n");
con_printf(CON_NORMAL, "\nBuilt: %s %s\n", __DATE__,__TIME__);
#ifdef __VERSION__
con_printf(CON_NORMAL, "Compiler: %s\n", __VERSION__);
#endif
con_printf(CON_NORMAL, "\n");
if (FindArg ("-?") || FindArg ("-help") || FindArg ("?") || FindArg ("-h")) {
	print_commandline_help ();
   set_exit_message ("");
#ifdef __MINGW32__
	exit (0);  /* mingw hangs on this return.  dunno why */
#endif
	}
}

// ----------------------------------------------------------------------------

void ShowTitleScreens (void)
{
#ifndef RELEASE
if (FindArg ("-notitles"))
	SongsPlaySong (SONG_TITLE, 1);
else
#endif
	{	//NOTE LINK TO ABOVE!
#ifndef SHAREWARE
	int nPlayed = MOVIE_NOT_PLAYED;	//default is not nPlayed
#endif
	int bSongPlaying = 0;

#ifdef D2_OEM
#	define MOVIE_REQUIRED 0
#else
#	define MOVIE_REQUIRED 1
#endif

#ifdef D2_OEM   //$$POLY_ACC, jay.
		{	//show bundler screens
		char filename[FILENAME_LEN];

		nPlayed = PlayMovie ("pre_i.mve",0,0,gameOpts->movies.bResize);
		if (!nPlayed) {
			strcpy (filename,gameStates.menus.bHires?"pre_i1b.pcx":"pre_i1.pcx");
			while (CFExist (filename)) {
				show_title_screen (filename, 1, 0);
				filename[5]++;
				}
			}
		}
#endif

#ifndef SHAREWARE
	if (gameStates.app.bHaveExtraMovies) {
		nPlayed = PlayMovie ("starta.mve",MOVIE_REQUIRED,0,gameOpts->movies.bResize);
		if (nPlayed == MOVIE_ABORTED)
			nPlayed = MOVIE_PLAYED_FULL;
		else
			nPlayed = PlayMovie ("startb.mve",MOVIE_REQUIRED,0,gameOpts->movies.bResize);
		}
	else {
		InitSubTitles ("intro.tex");
		nPlayed = PlayMovie ("intro.mve",MOVIE_REQUIRED,0,gameOpts->movies.bResize);
		}
	CloseSubTitles ();
#endif

#ifdef D2_OEM
	if (nPlayed != MOVIE_NOT_PLAYED)
	gameStates.movies.bIntroPlayed = 1;
	else {						//didn't get intro movie, try titles
		nPlayed = PlayMovie ("titles.mve",MOVIE_REQUIRED,0,gameOpts->movies.bResize);
		if (nPlayed == MOVIE_NOT_PLAYED) {
#if defined (POLY_ACC)
			GrSetMode (SM_640x480x15xPA);
#else
			GrSetMode (gameStates.menus.bHires?SM_640x480V:SM_320x200C);
#endif
			SongsPlaySong (SONG_TITLE, 1);
			bSongPlaying = 1;
#if TRACE		
			con_printf(CON_DEBUG, "\nShowing logo screens..."); fflush (fErr);}
#endif
			show_title_screen (gameStates.menus.bHires?"iplogo1b.pcx":"iplogo1.pcx", 1, 1);
			show_title_screen (gameStates.menus.bHires?"logob.pcx":"logo.pcx", 1, 1);
			}
		}

		{	//show bundler movie or screens
		char filename[FILENAME_LEN];
		int movie_handle;
		nPlayed=MOVIE_NOT_PLAYED;	//default is not nPlayed
		//check if OEM movie exists, so we don't stop the music if it doesn't
		movie_handle = OpenMovieFile ("oem.mve",0);
		if (movie_handle != -1) {
			close (movie_handle);
			nPlayed = PlayMovie ("oem.mve",0,0,1);
			bSongPlaying = 0;		//movie will kill sound
			}
		if (!nPlayed) {
			strcpy (filename,gameStates.menus.bHires?"oem1b.pcx":"oem1.pcx");
			while (CFExist (filename)) {
				show_title_screen (filename, 1, 0);
				filename[3]++;
				}
			}
		}
#endif

	if (!bSongPlaying)
		SongsPlaySong (SONG_TITLE, 1);
	}
PA_DFX (pa_splash ());
}

// ----------------------------------------------------------------------------

void ShowLoadingScreen (void)
{
	int pcx_error;
	char filename[14];

#if TRACE		
con_printf(CON_DEBUG, "\nShowing loading screen...\n"); fflush (fErr);
#endif
strcpy (filename, gameStates.menus.bHires?"descentb.pcx":"descent.pcx");
if (! CFExist (filename,gameFolders.szDataDir,0))
	strcpy (filename, gameStates.menus.bHires?"descntob.pcx":"descento.pcx"); // OEM
if (! CFExist (filename,gameFolders.szDataDir,0))
	strcpy (filename, "descentd.pcx"); // SHAREWARE
if (! CFExist (filename,gameFolders.szDataDir,0))
	strcpy (filename, "descentb.pcx"); // MAC SHAREWARE
#if defined (POLY_ACC)
GrSetMode (SM_640x480x15xPA);
#else
GrSetMode (
	gameStates.menus.bHires ? 
		(gameStates.gfx.nStartScrSize < 0) ? 
			SM (640,480) 
			: SM (scrSizes [gameStates.gfx.nStartScrSize].x, scrSizes [gameStates.gfx.nStartScrSize].y) 
		: SM (320,200));
#endif
#ifdef OGL
SetScreenMode (SCREEN_MENU);
#endif
gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && gameStates.menus.bHires;
if ((pcx_error = pcx_read_fullscr (filename, 0))==PCX_ERROR_NONE)	{
	GrPaletteStepClear ();
	GrPaletteFadeIn (gameData.render.pal.pCurPal, 32, 0);
	} 
else
	Error ("Couldn't load pcx file '%s',\nPCX load error: %s\n",filename, pcx_errormsg (pcx_error));
//the bitmap loading code changes grPalette, so restore it
//memcpy (grPalette,titlePal,sizeof (grPalette));
}

// ----------------------------------------------------------------------------

void LoadHoardData (void)
{
#ifdef EDITOR
if (FindArg ("-hoarddata")) {
#define MAX_BITMAPS_PER_BRUSH 30
	grs_bitmap * bm[MAX_BITMAPS_PER_BRUSH];
	grs_bitmap icon;
	int nframes;
	short nframes_short;
	ubyte palette[256*3];
	FILE *ofile;
	int iff_error,i;

	char *sounds[] = {"selforb.raw","selforb.r22",		//SOUND_YOU_GOT_ORB			
							"teamorb.raw","teamorb.r22",		//SOUND_FRIEND_GOT_ORB			
							"enemyorb.raw","enemyorb.r22",	//SOUND_OPPONENT_GOT_ORB	
							"OPSCORE1.raw","OPSCORE1.r22"};	//SOUND_OPPONENT_HAS_SCORED

	ofile = fopen ("hoard.ham","wb");

	iff_error = iff_read_animbrush ("orb.abm",bm,MAX_BITMAPS_PER_BRUSH,&nframes,palette);
	Assert (iff_error == IFF_NO_ERROR);
	nframes_short = nframes;
	fwrite (&nframes_short,sizeof (nframes_short),1,ofile);
	fwrite (&bm[0]->bm_props.w,sizeof (short),1,ofile);
	fwrite (&bm[0]->bm_props.h,sizeof (short),1,ofile);
	fwrite (palette,3,256,ofile);
	for (i=0;i<nframes;i++)
		fwrite (bm[i]->bm_texBuf,1,bm[i]->bm_props.w*bm[i]->bm_props.h,ofile);

	iff_error = iff_read_animbrush ("orbgoal.abm",bm,MAX_BITMAPS_PER_BRUSH,&nframes,palette);
	Assert (iff_error == IFF_NO_ERROR);
	Assert (bm[0]->bm_props.w == 64 && bm[0]->bm_props.h == 64);
	nframes_short = nframes;
	fwrite (&nframes_short,sizeof (nframes_short),1,ofile);
	fwrite (palette,3,256,ofile);
	for (i=0;i<nframes;i++)
		fwrite (bm[i]->bm_texBuf,1,bm[i]->bm_props.w*bm[i]->bm_props.h,ofile);

	for (i=0;i<2;i++) {
		iff_error = iff_read_bitmap (i?"orbb.bbm":"orb.bbm",&icon,BM_LINEAR,palette);
		Assert (iff_error == IFF_NO_ERROR);
		fwrite (&icon.bm_props.w,sizeof (short),1,ofile);
		fwrite (&icon.bm_props.h,sizeof (short),1,ofile);
		fwrite (palette,3,256,ofile);
		fwrite (icon.bm_texBuf,1,icon.bm_props.w*icon.bm_props.h,ofile);
		}

	for (i=0;i<sizeof (sounds)/sizeof (*sounds);i++) {
		FILE *ifile;
		int size;
		ubyte *buf;
		ifile = fopen (sounds[i],"rb");
		Assert (ifile != NULL);
		size = ffilelength (ifile);
		buf = d_malloc (size);
		fread (buf,1,size,ifile);
		fwrite (&size,sizeof (size),1,ofile);
		fwrite (buf,1,size,ofile);
		d_free (buf);
		fclose (ifile);
		}
	fclose (ofile);
	exit (1);
	}
#endif
}

// ----------------------------------------------------------------------------

void GrabMouse (int bGrab, int bForce)
{
#ifdef SDL_INPUT
if (gameStates.input.bGrabMouse && (bForce || gameStates.app.bGameRunning))
	SDL_WM_GrabInput ((bGrab || gameStates.ogl.bFullScreen) ? SDL_GRAB_ON : SDL_GRAB_OFF);
#endif
}

// ----------------------------------------------------------------------------

void MainLoop (void)
{
while (gameStates.app.nFunctionMode != FMODE_EXIT) {
	gameStates.app.bGameRunning = (gameStates.app.nFunctionMode == FMODE_GAME);
	switch (gameStates.app.nFunctionMode)	{
		case FMODE_MENU:
			SetScreenMode (SCREEN_MENU);
			if (gameData.demo.bAuto)	{
				NDStartPlayback (NULL);		// Randomly pick a file
				if (gameData.demo.nState != ND_STATE_PLAYBACK)	
				Error ("No demo files were found for autodemo mode!");
				}
			else {
#ifdef EDITOR
				if (Auto_exit) {
					strcpy ((char *)&gameData.missions.szLevelNames[0], Auto_file);
					LoadLevel (1, 1);
					SetFunctionMode (FMODE_EXIT);
					break;
					}
#endif
				check_joystick_calibration ();
				GrPaletteStepClear ();		//I'm not sure why we need this, but we do
				CallMenu ();									 	
#ifdef EDITOR
				if (gameStates.app.nFunctionMode == FMODE_EDITOR)	{
					create_new_mine ();
					SetPlayerFromCurseg ();
					LoadPalette (NULL,1,0);
					}
#endif
				}
			if (gameData.multi.autoNG.bValid && (gameStates.app.nFunctionMode != FMODE_GAME))
				gameStates.app.nFunctionMode = FMODE_EXIT;
			break;

		case FMODE_GAME:
#ifdef EDITOR
			keyd_editor_mode = 0;
#endif
			GrabMouse (1, 1);
			game ();
			GrabMouse (0, 1);
			GrPaletteFadeIn (NULL, 0, 0);
			ResetPaletteAdd ();
			GrPaletteFadeOut (NULL, 0, 0);
			if (gameData.multi.autoNG.bValid)
				gameStates.app.nFunctionMode = FMODE_EXIT;
			if (gameStates.app.nFunctionMode == FMODE_MENU)
				SongsPlaySong (SONG_TITLE, 1);
			RestoreDefaultRobots ();
			break;

#ifdef EDITOR
		case FMODE_EDITOR:
			keyd_editor_mode = 1;
			editor ();
#ifdef __WATCOMC__
			_harderr ((void*)descent_critical_error_handler);		// Reinstall game error handler
#endif
			if (gameStates.app.nFunctionMode == FMODE_GAME) {
				gameData.app.nGameMode = GM_EDITOR;
				editor_reset_stuff_on_level ();
				gameData.multi.nPlayers = 1;
				}
			break;
#endif
		default:
			Error ("Invalid function mode %d",gameStates.app.nFunctionMode);
		}
	}
}

// ----------------------------------------------------------------------------

int Initialize (int argc, char *argv[])
{
	int			h, i, t;
	u_int32_t	nScreenMode;

/*---*/LogErr ("Initializing data\n");
CFileInit ("", "");
InitGameData ();
InitGameStates ();
InitExtraGameInfo ();
InitNetworkData ();
InitGameOptions (0);
InitArgs (argc,argv);
GetAppFolders ();
if (FindArg ("-debug-printlog") || FindArg ("-printlog")) {
	   char fnErr [FILENAME_LEN];
#ifdef __linux__
	sprintf (fnErr, "%s/d2x.log", getenv ("HOME"));
	fErr = fopen (fnErr, "wt");
#else
	sprintf (fnErr, "%s/d2x.log", gameFolders.szGameDir);
	fErr = fopen (fnErr, "wt");
#endif
	}
LogErr ("%s\n", DESCENT_VERSION);
InitArgs (argc, argv);
GetAppFolders ();
con_init ();  // Initialise the console
#ifdef D2X_MEM_HANDLER
mem_init ();
#endif
error_init (NULL, NULL);
*szAutoHogFile =
*szAutoMission = '\0';
EvalArgs ();
InitGameOptions (1);
/*---*/LogErr ("Creating default tracker list\n");
CreateTrackerList ();
CFSetCriticalErrorCounterPtr (&nDescentCriticalError);
/*---*/LogErr ("Loading main hog file\n");
if (!(CFileInit ("descent2.hog", gameFolders.szDataDir) || 
	  (gameOpts->app.bDemoData = CFileInit ("d2demo.hog", gameFolders.szDataDir)))) {
	  char *psz = TXT_NO_HOG2;
	/*---*/LogErr ("Descent 2 data not found\n");
	Error (TXT_NO_HOG2);
	}
if (*szAutoHogFile && *szAutoMission) {
	CFUseAltHogFile (szAutoHogFile);
	gameStates.app.bAutoRunMission = gameHogFiles.AltHogFiles.bInitialized;
	}
/*---*/LogErr ("Loading text resources\n");
LoadGameTexts ();
/*---*/LogErr ("Loading hires models\n");
if (gameOpts->render.bHiresModels) {
#if 0 && defined (_DEBUG) && SHADOWS
	if (OOF_ReadFile ("cube.oof", gameData.models.hiresModels))
#else
	if (OOF_ReadFile ("pyrogl.oof", gameData.models.hiresModels))
#endif
		gameData.models.nHiresModels = 1;
	if (OOF_ReadFile ("MegaMissile.oof", gameData.models.hiresModels + OOF_MEGA))
		gameData.models.nHiresModels++;
	}
/*---*/LogErr ("Loading ban list\n");
LoadBanList ();
PrintBanner ();
/*---*/LogErr ("Initializing i/o\n");
arch_init ();
gameStates.render.nLighting = 1;
#ifdef EDITOR
if (!Inferno_is_800x600_available)	{
	SetFunctionMode (FMODE_MENU);
	}
#endif

/*---*/LogErr ("Reading configuration file\n");
ReadConfigFile ();
/*---*/LogErr ("Initializing control types\n");
SetControlType ();
/*---*/LogErr ("Initializing joystick\n");
do_joystick_init ();
/*---*/LogErr ("Initializing graphics\n");
if (t = GrInit ()) {		//doesn't do much
	LogErr ("Cannot initialize graphics\n");
	Error (TXT_CANT_INIT_GFX,t);
	return 1;
	}
nScreenMode = SM (scrSizes [gameStates.gfx.nStartScrSize].x, scrSizes [gameStates.gfx.nStartScrSize].y);
#ifdef _3DFX
_3dfx_Init ();
#endif

for (i = 0; i < NUM_DISPLAY_MODES; i++)
	if (0 <= (h = SCREENMODE (displayModeInfo [i].w, displayModeInfo [i].h, displayModeInfo [i].w < 640)))
		gameStates.gfx.nStartScrMode = h;
/*---*/LogErr ("Initializing render buffers\n");
#if defined (POLY_ACC)
if (gameStates.gfx.nStartScrMode < 0)
	GameInitRenderBuffers (SM_640x480x15xPA, 640, 480, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT);
else
	GameInitRenderBuffers (nScreenMode, SM_W (nScreenMode), SM_H (nScreenMode), gameStates.gfx.nStartScrMode, screen_compatible);
#else
if (!VR_offscreen_buffer)	//if hasn't been initialied (by headset init)
	SetDisplayMode ((gameStates.gfx.nStartScrMode < 0) ? 0 : gameStates.gfx.nStartScrMode);		//..then set default display mode
#endif
S_MODE (&automap_mode, &gameStates.render.bAutomapUseGameRes);
if ((i = FindArg ("-xcontrol")) > 0)
	KCInitExternalControls (strtol (Args[i+1], NULL, 0), strtol (Args[i+2], NULL, 0));

#if !defined (POLY_ACC)
if (FindArg ("-nohires") || FindArg ("-nohighres") || !GrVideoModeOK (MENU_HIRES_MODE) || disable_high_res)
	gameOpts->movies.bHires = 
	gameStates.menus.bHires = 
	gameStates.menus.bHiresAvailable = 0;
else
#endif
	gameStates.menus.bHires = gameStates.menus.bHiresAvailable = 1;

/*---*/LogErr ("Loading default palette\n");
defaultPalette = GrUsePaletteTable (D2_DEFAULT_PALETTE, NULL);
grdCurCanv->cv_bitmap.bm_palette = defaultPalette;	//just need some valid palette here
/*---*/LogErr ("Initializing game fonts\n");
GameFontInit ();	// must load after palette data loaded.
/*---*/LogErr ("Initializing movies\n");
InitMovies ();		//init movie libraries

#if defined (POLY_ACC)
GrSetMode (SM_640x480x15xPA);
#endif

/*---*/LogErr ("Initializing game data\n");
#ifdef EDITOR
bm_init_use_tbl ();
#else
BMInit ();
#endif

if (!FindArg ("-nosound")) {
	/*---*/LogErr ("Initializing sound\n");
	DigiInit ();
	}
/*---*/LogErr ("Loading hoard data\n");
LoadHoardData ();
error_init (ShowInGameWarning, NULL);
if (FindArg ("-norun"))
	return (0);
if (!gameStates.app.bAutoRunMission) {
	/*---*/LogErr ("Showing title screens\n");
	ShowTitleScreens ();
	}
/*---*/LogErr ("Showing loading screen\n");
ShowLoadingScreen ();
g3_init ();
/*---*/LogErr ("Initializing texture merge buffer\n");
TexMergeInit (100);		// 100 cache bitmaps
/*---*/LogErr ("Setting screen mode\n");
SetScreenMode (SCREEN_MENU);
InitPowerupTables ();
InitMissileFlags ();
InitGame ();
return 0;
}

// ----------------------------------------------------------------------------

int CleanUp (void)
{
SongsStopAll ();
DigiStopCurrentSong ();
/*---*/LogErr ("Saving configuration file\n");
WriteConfigFile ();
/*---*/LogErr ("Saving player profile\n");
WritePlayerFile ();
/*---*/LogErr ("Releasing tracker list\n");
DestroyTrackerList ();
#ifndef RELEASE
if (!FindArg ("-notitles"))
#endif
	//show_order_form ();
#ifndef NDEBUG
if (FindArg ("-showmeminfo"))
	bShowMemInfo = 1;		// Make memory statistics show
#endif
return 0;
}

// ----------------------------------------------------------------------------

int _CDECL_ main (int argc, char *argv[])
{
gameStates.app.bInitialized = 0;
if (Initialize (argc, argv))
	return -1;
//	If built with editor, option to auto-load a level and quit game
//	to write certain data.
#ifdef	EDITOR
if ((i = FindArg ("-autoload"))) {
	Auto_exit = 1;
	strcpy (Auto_file, Args[i+1]);
	}
if (Auto_exit) {
	strcpy (gameData.multi.players[0].callsign, "dummy");
	} 
else
#endif
/*---*/LogErr ("Loading player profile\n");
DoSelectPlayer ();
GrPaletteFadeOut (NULL, 32, 0);
// handle direct loading and starting of a mission specified via the command line
if (gameStates.app.bAutoRunMission) {
	gameStates.app.nFunctionMode = StartNewGame (1) ? FMODE_GAME : FMODE_MENU;
	gameStates.app.bAutoRunMission = 0;
	}
// handle automatic launch of a demo playback
if (gameData.demo.bAuto)	{
	NDStartPlayback (gameData.demo.fnAuto);		
	if (gameData.demo.nState == ND_STATE_PLAYBACK)
		SetFunctionMode (FMODE_GAME);
	}
//do this here because the demo code can do a longjmp when trying to
//autostart a demo from the main menu, never having gone into the game
setjmp (gameExitPoint);
/*---*/LogErr ("Invoking main menu\n");
gameStates.app.bInitialized = 1;
MainLoop ();
CleanUp ();
return 0;		//presumably successful exit
}

// ----------------------------------------------------------------------------

void check_joystick_calibration ()	
{
#if 0//ndef _WIN32	
	int x1, y1, x2, y2, c;
	fix t1;

	if ((gameConfig.nControlType!=CONTROL_JOYSTICK) &&
		  (gameConfig.nControlType!=CONTROL_FLIGHTSTICK_PRO) &&
		  (gameConfig.nControlType!=CONTROL_THRUSTMASTER_FCS) &&
		  (gameConfig.nControlType!=CONTROL_GRAVIS_GAMEPAD)
		) return;
	joy_get_pos (&x1, &y1);
	t1 = TimerGetFixedSeconds ();
	while (TimerGetFixedSeconds () < t1 + F1_0/100)
		;
	joy_get_pos (&x2, &y2);
	// If joystick hasn't moved...
	if ((abs (x2-x1)<30) &&  (abs (y2-y1)<30))	{
		if ((abs (x1)>30) || (abs (x2)>30) ||  (abs (y1)>30) || (abs (y2)>30))	{
			c = ExecMessageBox (NULL, NULL, 2, TXT_CALIBRATE, TXT_SKIP, TXT_JOYSTICK_NOT_CEN);
			if (c==0)	{
				joydefs_calibrate ();
			}
		}
	}
#endif
}

// ----------------------------------------------------------------------------

void show_order_form ()
{
#ifndef EDITOR
	int pcx_error;
	char titlePal[768];
	char	exit_screen[16];

	GrSetCurrentCanvas (NULL);
	GrPaletteStepClear ();

	KeyFlush ();

	strcpy (exit_screen, gameStates.menus.bHires?"ordrd2ob.pcx":"ordrd2o.pcx"); // OEM
	if (! CFExist (exit_screen,gameFolders.szDataDir,0))
		strcpy (exit_screen, gameStates.menus.bHires?"orderd2b.pcx":"orderd2.pcx"); // SHAREWARE, prefer mac if hires
	if (! CFExist (exit_screen,gameFolders.szDataDir,0))
		strcpy (exit_screen, gameStates.menus.bHires?"orderd2.pcx":"orderd2b.pcx"); // SHAREWARE, have to rescale
	if (! CFExist (exit_screen,gameFolders.szDataDir,0))
		strcpy (exit_screen, gameStates.menus.bHires?"warningb.pcx":"warning.pcx"); // D1
	if (! CFExist (exit_screen,gameFolders.szDataDir,0))
		return; // D2 registered

	if ((pcx_error=pcx_read_fullscr (exit_screen, 0))==PCX_ERROR_NONE) {
		//vfx_set_palette_sub (titlePal);
		GrPaletteFadeIn (NULL, 32, 0);
		GrUpdate (0);
		while (!KeyInKey () && !MouseButtonState (0)) {} //key_getch ();
		GrPaletteFadeOut (titlePal, 32, 0);
	}
	else
		Int3 ();		//can't load order screen

	KeyFlush ();
#endif
}

// ----------------------------------------------------------------------------

void quit_request ()
{
#ifdef NETWORK
//	void network_abort_game ();
//	if (Network_status)
//		network_abort_game ();
#endif
	exit (0);
}
