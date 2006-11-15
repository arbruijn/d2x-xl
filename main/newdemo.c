/* $Id: newdemo.c, v 1.15 2003/11/26 12:26:31 btb Exp $ */
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
 * Code to make a complete demo playback system.
 *
 * Old Log:
 * Revision 1.12  1995/10/31  10:19:43  allender
 * shareware stuff
 *
 * Revision 1.11  1995/10/17  13:19:16  allender
 * close boxes for demo save
 *
 * Revision 1.10  1995/10/05  10:36:07  allender
 * fixed calls to DigiPlaySample to account for differing volume and angle calculations
 *
 * Revision 1.9  1995/09/12  15:49:13  allender
 * define __useAppleExts__ if not already defined
 *
 * Revision 1.8  1995/09/05  14:28:32  allender
 * fixed previous gameData.multi.nPlayers bug in NDGotoEnd
 *
 * Revision 1.7  1995/09/05  14:16:51  allender
 * added space to allowable demo filename characters
 * and fixed bug with netgame demos gameData.multi.nPlayers got getting
 * set correctly
 *
 * Revision 1.6  1995/09/01  16:10:47  allender
 * fixed bug with reading in gameData.multi.nPlayers variable on
 * netgame demo playback
 *
 * Revision 1.5  1995/08/24  16:04:11  allender
 * fix signed byte problem
 *
 * Revision 1.4  1995/08/12  12:21:59  allender
 * made call to CreateShortPos not swap the shortpos
 * elements
 *
 * Revision 1.3  1995/08/01  16:04:19  allender
 * made random picking of demo work
 *
 * Revision 1.2  1995/08/01  13:56:56  allender
 * demo system working on the mac
 *
 * Revision 1.1  1995/05/16  15:28:59  allender
 * Initial revision
 *
 * Revision 2.7  1995/05/26  16:16:06  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 *
 * Revision 2.6  1995/03/21  14:39:38  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.5  1995/03/14  18:24:31  john
 * Force Destination Saturn to use CD-ROM drive.
 *
 * Revision 2.4  1995/03/14  16:22:29  john
 * Added cdrom alternate directory stuff.
 *
 * Revision 2.3  1995/03/10  12:58:33  allender
 * only display rear view cockpit when cockpit mode was CM_FULL_COCKPIT.
 *
 * Revision 2.2  1995/03/08  16:12:15  allender
 * changes for Destination Saturn
 *
 * Revision 2.1  1995/03/08  12:11:26  allender
 * fix shortpos reading
 *
 * Revision 2.0  1995/02/27  11:29:40  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.189  1995/02/22  14:53:42  allender
 * missed some anonymous stuff
 *
 * Revision 1.188  1995/02/22  13:24:53  john
 * Removed the vecmat anonymous unions.
 *
 * Revision 1.187  1995/02/22  13:13:54  allender
 * remove anonymous unions from tObject structure
 *
 * Revision 1.186  1995/02/14  15:36:41  allender
 * fix fix for morph effect
 *
 * Revision 1.185  1995/02/14  11:25:48  allender
 * save cockpit mode and restore after playback.  get orientation for morph
 * effect when tObject is morph tVideoClip
 *
 * Revision 1.184  1995/02/13  12:18:14  allender
 * change to decide about interpolating or not
 *
 * Revision 1.183  1995/02/12  00:46:23  adam
 * don't decide to skip frames until after at least 10 frames have
 * passed -- allender
 *
 * Revision 1.182  1995/02/11  22:34:01  john
 * Made textures page in for newdemos before playback time.
 *
 * Revision 1.181  1995/02/11  17:28:32  allender
 * strip frames from end of demo
 *
 * Revision 1.180  1995/02/11  16:40:35  allender
 * start of frame stripping debug code
 *
 * Revision 1.179  1995/02/10  17:40:06  allender
 * put back in WallHitProcess code to fix door problem
 *
 * Revision 1.178  1995/02/10  17:17:24  adam
 * allender } fix
 *
 * Revision 1.177  1995/02/10  17:16:24  allender
 * fix possible tmap problems
 *
 * Revision 1.176  1995/02/10  15:54:37  allender
 * changes for out of space on device.
 *
 * Revision 1.175  1995/02/09  19:55:00  allender
 * fix bug with morph recording -- force rendertype to RT_POLYOBJ when
 * playing back since it won't render until fully morphed otherwise
 *
 * Revision 1.174  1995/02/07  17:15:35  allender
 * DOH!!!!!
 *
 * Revision 1.173  1995/02/07  17:14:21  allender
 * immediately return when loading bogus level stuff when reading a frame
 *
 * Revision 1.172  1995/02/02  11:15:03  allender
 * after loading new level, read next frame (forward or back) always because
 * of co-op ships showing up when level is loaded
 *
 * Revision 1.171  1995/02/02  10:24:16  allender
 * removed cfile stuff.  Use standard FILE functions for demo playback
 *
 * Revision 1.170  1995/01/30  13:54:32  allender
 * support for missions
 *
 * Revision 1.169  1995/01/27  16:27:35  allender
 * put game mode to demo_game_mode when sorting multiplayer kill (and score)
 * list
 *
 * Revision 1.168  1995/01/27  09:52:25  allender
 * minor changes because of tObject/tSegment linking problems
 *
 * Revision 1.167  1995/01/27  09:22:28  allender
 * changed way multi-player score is recorded.  Record difference, not
 * actual
 *
 * Revision 1.166  1995/01/25  14:32:44  allender
 * changed with recorded player flags.  More checks for paused state
 * during interpolation reading of gameData.objs.objects
 *
 * Revision 1.165  1995/01/25  11:23:32  allender
 * found bug with out of disk space problem
 *
 * Revision 1.164  1995/01/25  11:11:33  allender
 * coupla' things.  Fix problem with gameData.objs.objects apparently being linked in
 * the wrong tSegment.  Put an Int3 in to check why demos will write to
 * end of space on drive. close demo file if demo doens't start playing
 * back.  Put objP->nType == OBJ_ROBOT around checking for boss cloaking
 *
 * Revision 1.163  1995/01/24  19:44:30  allender
 * fix obscure problem with rewinding and having the wrong tObject linked
 * to the wrong segments.  will investigate further.
 *
 * Revision 1.162  1995/01/23  09:31:28  allender
 * add team score in team mode playback
 *
 * Revision 1.161  1995/01/20  22:47:39  matt
 * Mission system implemented, though imcompletely
 *
 * Revision 1.160  1995/01/20  09:30:37  allender
 * don't call LoadLevel with bogus data
 *
 * Revision 1.159  1995/01/20  09:13:23  allender
 * *&^%&*%$ typo
 *
 * Revision 1.158  1995/01/20  09:12:04  allender
 * record team names during demo recoring in GM_TEAM
 *
 * Revision 1.157  1995/01/19  16:31:09  allender
 * forgot to bump demo version for new weapon change stuff
 *
 * Revision 1.156  1995/01/19  16:29:33  allender
 * added new byte for weapon change (old weapon) so rewinding works
 * correctly for weapon changes in registered
 *
 * Revision 1.155  1995/01/19  15:00:05  allender
 * remove code to take away blastable walls in multiplayer demo playbacks
 *
 * Revision 1.154  1995/01/19  11:07:05  allender
 * put in psuedo cloaking for boss robots.  Problem is that cloaking is
 * time based, and that don't get bDone in demos, so bosses just disappear.
 * oh well
 *
 * Revision 1.153  1995/01/19  09:42:29  allender
 * record laser levels in demos
 *
 * Revision 1.152  1995/01/18  20:43:12  allender
 * fix laser level stuff on goto-beginning and goto-end
 *
 * Revision 1.151  1995/01/18  20:28:18  allender
 * cloak robots now cloak (except maybe for boss........)  Put in function
 * to deal with control center triggers
 *
 * Revision 1.150  1995/01/18  18:55:07  allender
 * bug fix
 *
 * Revision 1.149  1995/01/18  18:49:03  allender
 * lots 'o stuff....record laser level.  Record beginning of door opening
 * sequence.  Fix some problems with control center stuff.  Control center
 * triggers now work in reverse
 *
 * Revision 1.148  1995/01/18  08:51:40  allender
 * forgot to record ammo counts at beginning of demo
 *
 * Revision 1.147  1995/01/17  17:42:07  allender
 * added primary and secondary ammo counts.  Changed goto_end routine
 * to be more efficient
 *
 * Revision 1.146  1995/01/17  13:46:35  allender
 * fix problem with destroyed control center and rewinding a demo.
 * Save callsign and restore after demo playback
 *
 * Revision 1.145  1995/01/12  10:21:53  allender
 * fixes for 1.0 to 1.1 demo incompatibility
 *
 * Revision 1.144  1995/01/05  13:51:43  allender
 * fixed nType of player num variable
 *
 * Revision 1.143  1995/01/04  16:58:28  allender
 * bumped up demo version number
 *
 * Revision 1.142  1995/01/04  14:59:02  allender
 * added more information to end of demo for registered.  Forced game mode
 * to be GM_NORMAL on demo playback
 *
 * Revision 1.141  1995/01/03  17:30:47  allender
 * fixed logic problem with cloak stuf
 *
 * Revision 1.140  1995/01/03  17:12:23  allender
 * fix for getting cloak stuff at end of demo for shareware
 *
 * Revision 1.139  1995/01/03  15:20:24  allender
 * fix goto_end for shareware -- changes to goto_end for registered
 *
 * Revision 1.138  1995/01/03  13:13:26  allender
 * add } I forgot
 *
 * Revision 1.137  1995/01/03  13:10:29  allender
 * make score work forwards and backwards
 *
 * Revision 1.136  1995/01/03  11:45:20  allender
 * added code to record players scores
 *
 * Revision 1.135  1994/12/30  10:03:57  allender
 * put cloak stuff at end of demo for fast forward to the end
 *
 * Revision 1.134  1994/12/29  17:02:55  allender
 * spelling fix on SHAREWARE
 *
 * Revision 1.133  1994/12/29  16:43:41  allender
 * lots of new multiplayer stuff.  wrapped much code with SHAREWARE defines
 *
 * Revision 1.132  1994/12/28  14:15:01  allender
 * added routines to deal with connecting and reconnecting players when
 * recording multiplayer demos
 *
 * Revision 1.131  1994/12/21  12:57:59  allender
 * bug fix
 *
 * Revision 1.130  1994/12/21  12:46:53  allender
 * record multi player deaths and kills
 *
 * Revision 1.129  1994/12/19  16:37:27  allender
 * pick good filename when trying to save in network play and player
 * gets bumped out of menu by multi-player code
 *
 * Revision 1.128  1994/12/14  10:49:01  allender
 * reset bad_read variable when starting demo playback
 *
 * Revision 1.127  1994/12/14  08:53:06  allender
 * lowered watermark for out of space
 *
 * Revision 1.126  1994/12/14  08:49:52  allender
 * put up warning when starting demo recording if not enough space and
 * not let them record
 *
 * Revision 1.125  1994/12/13  00:01:37  allender
 * CLOAK FIX -- (I'm tempted to take cloak out of game because I can't
 * seem to get it right in demo playback)
 *
 * Revision 1.124  1994/12/12  14:51:21  allender
 * more fixed to multiplayer cloak stuff
 *
 * Revision 1.123  1994/12/12  11:33:11  allender
 * fixed rearview mode to work again
 *
 * Revision 1.122  1994/12/12  11:00:16  matt
 * Added code to handle confusion with attached gameData.objs.objects
 *
 * Revision 1.121  1994/12/12  00:31:29  allender
 * give better warning when out of space when recording.  Don't record
 * until no space left.  We have 500K watermark when we now stop recording
 *
 * Revision 1.120  1994/12/10  16:44:54  matt
 * Added debugging code to track down door that turns into rock
 *
 * Revision 1.119  1994/12/09  18:46:15  matt
 * Added code to handle odd error condition
 *
 * Revision 1.118  1994/12/09  17:27:37  allender
 * force playernum to 0 when demo is bDone playing
 *
 * Revision 1.117  1994/12/09  16:40:39  allender
 * yet more cloak stuff.  Assign cloak/invuln time when starting demo
 * if flags are set.  Check cloak and invuln time when demo
 * even when paused
 *
 * Revision 1.116  1994/12/09  14:59:22  matt
 * Added system to attach a fireball to another tObject for rendering purposes, 
 * so the fireball always renders on top of (after) the tObject.
 *
 * Revision 1.115  1994/12/09  12:21:45  allender
 * only allow valid chars when typing in demo filename
 *
 * Revision 1.114  1994/12/08  23:19:02  allender
 * final (?) fix for getting cloak gauge to work on demo playback
 * with forward and reverse
 *
 * Revision 1.113  1994/12/08  21:34:38  allender
 * record old and new player flags to accuratedly record cloaking and
 * decloaking
 * ./
 *
 * Revision 1.112  1994/12/08  18:04:47  allender
 * bashed playernum right after reading it in demo header so shields
 * and energy are put in right place
 *
 * Revision 1.111  1994/12/08  17:10:07  allender
 * encode playernum in demo header.  Bash viewer tSegment to 0 if in
 * bogus nSegment.  Don't link render objs for same reason
 *
 * Revision 1.110  1994/12/08  15:36:12  allender
 * cloak stuff works forwards and backwards
 *
 * Revision 1.109  1994/12/08  13:46:03  allender
 * don't record rearview anymore, but leave in case statement for playback
 * purposes.  change the way letterbox <--> cockpit transitions happen
 *
 * Revision 1.108  1994/12/08  12:36:06  matt
 * Added new tObject allocation & deallocation functions so other code
 * could stop messing around with internal tObject data structures.
 *
 * Revision 1.107  1994/12/08  11:19:04  allender
 * handle out of space (more) gracefully then before
 *
 * Revision 1.106  1994/12/08  00:29:49  allender
 * fixed bug that didn't load level on goto_beginning
 *
 * Revision 1.105  1994/12/08  00:11:51  mike
 * change matrix interpolation.
 *
 * Revision 1.104  1994/12/07  23:46:37  allender
 * changed invulnerability and cloak to work (almost) correctly both
 * in single and multi player
 *
 * Revision 1.103  1994/12/07  11:48:49  adam
 * BY ALLENDER -- added dampening of interpolation factor to 1 if greater
 * than 1 (although I have not seen this happen).  this is attempt to
 * get wobbling problem solved
 *
 * Revision 1.102  1994/12/07  11:23:56  allender
 * attempt at getting rid of wobbling on demo playback
 *
 * Revision 1.101  1994/12/06  19:31:17  allender
 * moved blastable wall stuff code to where we load level during demo
 * playback
 *
 * Revision 1.100  1994/12/06  19:21:51  allender
 * multi games, destroy blastable walls.  Do wall toggle when control center
 * destroyed
 *
 * Revision 1.99  1994/12/06  16:54:48  allender
 * fixed code so if demo automatically started from menu, don't bring up
 * message if demo is too old
 *
 * Revision 1.98  1994/12/06  13:55:15  matt
 * Use new rounding func, f2ir ()
 *
 * Revision 1.97  1994/12/06  13:44:45  allender
 * suppressed compiler warnings
 *
 * Revision 1.96  1994/12/06  13:38:03  allender
 * removed recording of wall hit process.  I think that all bases are covered
 * elsewhere
 *
 * Revision 1.95  1994/12/06  12:57:35  allender
 * added recording of multi_decloaking.  Fixed some other cloaking code so
 * that cloak should last as long as player was cloaked.  We will lose the
 * guage effect, but the time is probably more important on playback
 *
 * Revision 1.94  1994/12/05  23:37:17  matt
 * Took out calls to warning () function
 *
 * Revision 1.93  1994/12/03  17:52:04  yuan
 * Localization 380ish
 *
 * Revision 1.92  1994/12/02  12:53:39  allender
 * fixed goto_beginning and goto_end on demo playback
 *
 * Revision 1.91  1994/12/01  12:01:49  allender
 * added multi player cloak stuff
 *
 * Revision 1.90  1994/11/30  09:33:58  allender
 * added field in header to tell what version (shareware or registered)
 * demo was recorded with.  Don't allow demo recorded on one to playback
 * on the other
 *
 * Revision 1.89  1994/11/29  00:31:01  allender
 * major changes -- added level recording feature which records level
 * advancement.  Changes to internal code to handle this.
 *
 * Revision 1.88  1994/11/27  23:13:54  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.87  1994/11/27  23:07:35  allender
 * starting on code to get all level transitions recorded. not bDone yet
 *
 * Revision 1.86  1994/11/27  17:39:47  matt
 * Don't xlate tmap numbers when editor compiled out
 *
 * Revision 1.85  1994/11/23  09:27:21  allender
 * put up info box with message if demo version is too old or level
 * cannot be loaded
 *
 * Revision 1.84  1994/11/22  19:37:39  allender
 * fix array mistake
 *
 * Revision 1.83  1994/11/22  19:35:09  allender
 * record player ship colors in multiplayer demo recordings
 *
 * Revision 1.82  1994/11/19  15:36:42  mike
 * fix fix.
 *
 * Revision 1.81  1994/11/19  15:23:21  mike
 * rip out unused code
 *
 * Revision 1.80  1994/11/16  14:51:49  rob
 * Fixed network/demo incompatibility.
 *
 * Revision 1.79  1994/11/15  10:55:48  allender
 * made start of demo playback read initial demo information so
 * level will get loaded.  Made demo record to single file which
 * will get renamed.  Added numerics after old filename so
 * sequential filenames would be defaulted to
 *
 * Revision 1.78  1994/11/15  09:46:06  allender
 * added versioning.  Fixed problems with trying to interpolating a completely
 * 0 orientation matrix
 *
 * Revision 1.77  1994/11/14  14:34:31  matt
 * Fixed up handling when textures can't be found during remap
 *
 * Revision 1.76  1994/11/14  09:15:29  allender
 * make ESC from file save menu exit w/o saving.  Fix letterbox, rear view, 
 * to normal cockpit mode transition to work correctly when skipping and
 * interpolating frames
 *
 * Revision 1.75  1994/11/11  16:22:07  allender
 * made morphing gameData.objs.objects record only the tObject being morphed.
 *
 * Revision 1.74  1994/11/08  14:59:19  john
 * Added code to respond to network while in menus.
 *
 * Revision 1.73  1994/11/08  14:52:20  adam
 * *** empty log message ***
 *
 * Revision 1.72  1994/11/07  15:47:04  allender
 * prompt for filename when bDone recording demo
 *
 * Revision 1.71  1994/11/07  11:47:19  allender
 * when interpolating frames, delete weapon, fireball, and debris gameData.objs.objects
 * from an inpolated frame if they don't appear in the next recorded
 * frame
 *
 * Revision 1.70  1994/11/07  11:02:41  allender
 * more with interpolation. I believe that I have it right now
 *
 * Revision 1.69  1994/11/07  08:47:40  john
 * Made wall state record.
 *
 * Revision 1.68  1994/11/05  17:22:51  john
 * Fixed lots of sequencing problems with newdemo stuff.
 *
 * Revision 1.67  1994/11/04  20:11:52  john
 * Neatening up palette stuff with demos.
 *
 * Revision 1.66  1994/11/04  16:49:44  allender
 * changed newdemo_do_interpolate to default to on
 *
 * Revision 1.65  1994/11/04  16:44:51  allender
 * added filename support for demo recording.  more auto demo stuff
 *
 * Revision 1.64  1994/11/04  13:05:31  allender
 * fixing the lifeleft variable again.  (I think I got it right this time)
 *
 * Revision 1.63  1994/11/04  11:37:37  allender
 * commented out fprintfs and fixed compiler warning
 *
 * Revision 1.62  1994/11/04  11:33:50  allender
 * added OBJ_FLARE and OBJ_LIGHT to objP->lifeleft recording
 *
 * Revision 1.61  1994/11/04  11:29:21  allender
 * more interpolation stuff -- not bDone yet.  Fixed so hostage vclips
 * render correctly.  Changed lifeleft to full precision, but only
 * write it when tObject is fireball or weapon nType of tObject
 *
 * Revision 1.60  1994/11/03  10:00:11  allender
 * fixed divide by zero in calculating render time.  more interpolation
 * stuff which isn't quite bDone
 *
 * Revision 1.59  1994/11/02  17:10:59  allender
 * never play recorded frames when interpolation is occuring
 *
 * Revision 1.58  1994/11/02  14:28:58  allender
 * profile total playback time and average frame render time
 *
 * Revision 1.57  1994/11/02  14:09:03  allender
 * record rear view.  start of playback interpolation code -- this
 * is not yet bDone
 *
 * Revision 1.56  1994/11/01  13:25:30  allender
 * drop frames if playing back demo on slower machine
 *
 * Revision 1.55  1994/10/31  16:10:40  allender
 * record letterbox mode on death seq, and then restore
 *
 * Revision 1.54  1994/10/29  16:01:38  allender
 * added ND_STATE_NODEMOS to indicate that there are no demos currently
 * available for playback
 *
 * Revision 1.53  1994/10/29  15:38:42  allender
 * in NDStartPlayback, make gameData.demo.bEof = 0
 *
 * Revision 1.52  1994/10/28  14:45:28  john
 * fixed typo from last checkin.
 *
 * Revision 1.51  1994/10/28  14:42:55  john
 * Added sound volumes to all sound calls.
 *
 * Revision 1.50  1994/10/28  14:31:57  allender
 * homing missle and autodemo stuff
 *
 * Revision 1.49  1994/10/28  12:42:14  allender
 * record homing distance
 *
 * Revision 1.48  1994/10/27  16:57:54  allender
 * changed demo vcr to be able to play any number of frames by storing
 * frame length (in bytes) in the demo file.  Added blowing up monitors
 *
 * Revision 1.47  1994/10/26  16:50:50  allender
 * put two functions inside of VCR_MODE ifdef
 *
 * Revision 1.46  1994/10/26  15:20:32  allender
 * added CT_REMOTE as valid control nType for recording
 *
 * Revision 1.45  1994/10/26  14:45:35  allender
 * completed hacked in vcr demo playback stuff
 *
 * Revision 1.44  1994/10/26  13:40:52  allender
 * vcr playback of demo stuff
 *
 * Revision 1.43  1994/10/26  08:51:57  allender
 * record player weapon change
 *
 * Revision 1.42  1994/10/25  15:48:01  allender
 * add shields, energy, and player flags to demo recording.
 * , 
 *
 * Revision 1.41  1994/10/24  08:19:35  allender
 * fixed compilation errors
 *
 * Revision 1.40  1994/10/23  19:17:08  matt
 * Fixed bug with "no key" messages
 *
 * Revision 1.39  1994/10/22  14:15:08  mike
 * Suppress compiler warnings.
 *
 * Revision 1.38  1994/10/21  15:24:55  allender
 * compressed writing of tObject structures with specialized code
 * to write out only pertinent tObject structures.
 *
 * Revision 1.37  1994/10/20  13:03:17  matt
 * Replaced old save files (MIN/SAV/HOT) with new LVL files
 *
 * Revision 1.36  1994/09/28  23:13:10  matt
 * Macroized palette flash system
 *
 * Revision 1.35  1994/09/26  17:28:32  matt
 * Made new multiple-tObject morph code work with the demo system
 *
 * Revision 1.34  1994/09/10  13:31:54  matt
 * Made exploding walls a nType of blastable walls.
 * Cleaned up blastable walls, making them tmap2 bitmaps.
 *
 * Revision 1.33  1994/08/15  18:05:28  john
 * *** empty log message ***
 *
 * Revision 1.32  1994/08/15  17:56:38  john
 * , 
 *
 * Revision 1.31  1994/08/10  09:44:54  john
 * *** empty log message ***
 *
 * Revision 1.30  1994/07/22  12:35:48  matt
 * Cleaned up editor/game interactions some more.
 *
 * Revision 1.29  1994/07/21  13:06:45  matt
 * Ripped out remants of old demo system, and added demo only system that
 * disables tObject movement and game options from menu.
 *
 * Revision 1.28  1994/07/18  16:22:44  john
 * Made all file read/writes call the same routine.
 *
 * Revision 1.27  1994/07/14  22:38:27  matt
 * Added exploding doors
 *
 * Revision 1.26  1994/07/05  12:49:04  john
 * Put functionality of New Hostage spec into code.
 *
 * Revision 1.25  1994/06/29  11:05:38  john
 * Made demos read in compressed.
 *
 * Revision 1.24  1994/06/29  09:14:06  john
 * Made files write out uncompressed and read in compressed.
 *
 * Revision 1.23  1994/06/28  11:55:28  john
 * Made newdemo system record/play directly to/from disk, so
 * we don't need the 4 MB buffer anymore.
 *
 * Revision 1.22  1994/06/27  15:52:38  john
 * #define'd out the newdemo stuff
 *
 *
 * Revision 1.21  1994/06/22  00:29:04  john
 * Fixed bug with playing demo then playing game without
 * loading new mine.
 *
 * Revision 1.20  1994/06/22  00:14:23  john
 * Attempted to fix sign.
 *
 * Revision 1.19  1994/06/21  23:57:54  john
 * Hopefully fixed bug with negative countdowns.
 *
 * Revision 1.18  1994/06/21  23:47:44  john
 * MAde Malloc always 4*1024*1024.
 *
 * Revision 1.17  1994/06/21  22:58:47  john
 * Added error if out of memory.
 *
 * Revision 1.16  1994/06/21  22:15:48  john
 * Added  % bDone to demo recording.
 *
 *
 * Revision 1.15  1994/06/21  19:45:55  john
 * Added palette effects to demo recording.
 *
 * Revision 1.14  1994/06/21  15:08:54  john
 * Made demo record HUD message and cleaned up the HUD code.
 *
 * Revision 1.13  1994/06/21  14:20:08  john
 * Put in hooks to record HUD messages.
 *
 * Revision 1.12  1994/06/20  11:50:15  john
 * Made demo record flash effect, and control center triggers.
 *
 * Revision 1.11  1994/06/17  18:01:33  john
 * A bunch of new stuff by John
 *
 * Revision 1.10  1994/06/17  12:13:31  john
 * More newdemo stuff; made editor->game transition start in slew mode.
 *
 * Revision 1.9  1994/06/16  13:14:36  matt
 * Fixed typo
 *
 * Revision 1.8  1994/06/16  13:02:07  john
 * Added morph hooks.
 *
 * Revision 1.7  1994/06/15  19:01:33  john
 * Added the capability to make 3d sounds play just once for the
 * laser hit wall effects.
 *
 * Revision 1.6  1994/06/15  14:56:59  john
 * Added triggers to demo recording.
 *
 * Revision 1.5  1994/06/14  20:42:15  john
 * Made robot matztn cntr not work until no robots or player are
 * in the tSegment.
 *
 * Revision 1.4  1994/06/14  14:43:27  john
 * Made doors work with newdemo system.
 *
 * Revision 1.3  1994/06/14  11:32:29  john
 * Made Newdemo record & restore the current mine.
 *
 * Revision 1.2  1994/06/13  21:02:43  john
 * Initial version of new demo recording system.
 *
 * Revision 1.1  1994/06/13  11:09:00  john
 * Initial revision
 *
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h> // for memset
#ifndef _WIN32_WCE
#include <errno.h>
#endif
#include <ctype.h>      /* for isdigit */
#include <limits.h>
#if defined (__unix__) || defined (__macosx__)
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "u_mem.h"
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
//#include "error.h"
#include "mono.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"

#include "object.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "error.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"

#include "powerup.h"
#include "fuelcen.h"

#include "sounds.h"
#include "collide.h"

#include "lighting.h"
#include "newdemo.h"
#include "gameseq.h"
#include "gamesave.h"
#include "gamemine.h"
#include "switch.h"
#include "gauges.h"
#include "player.h"
#include "vecmat.h"
#include "newmenu.h"
#include "args.h"
#include "palette.h"
#include "multi.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "text.h"
#include "cntrlcen.h"
#include "aistruct.h"
#include "mission.h"
#include "piggy.h"
#include "controls.h"
#include "d_io.h"
#include "timer.h"
#include "objsmoke.h"

#include "findfile.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#define ND_USE_SHORTPOS 0

extern void SetFunctionMode (int);

void DoJasonInterpolate (fix xRecordedTime);

//#include "nocfile.h"

//Does demo start automatically?

static sbyte	bNDBadRead;

#define	CATCH_BAD_READ				if (bNDBadRead) {bDone = -1; break;}

#define ND_EVENT_EOF                0   // EOF
#define ND_EVENT_START_DEMO         1   // Followed by 16 character, NULL terminated filename of .SAV file to use
#define ND_EVENT_START_FRAME        2   // Followed by integer frame number, then a fix gameData.time.xFrame
#define ND_EVENT_VIEWER_OBJECT      3   // Followed by an tObject structure
#define ND_EVENT_RENDER_OBJECT      4   // Followed by an tObject structure
#define ND_EVENT_SOUND              5   // Followed by int soundum
#define ND_EVENT_SOUND_ONCE         6   // Followed by int soundum
#define ND_EVENT_SOUND_3D           7   // Followed by int soundum, int angle, int volume
#define ND_EVENT_WALL_HIT_PROCESS   8   // Followed by int nSegment, int nSide, fix damage
#define ND_EVENT_TRIGGER            9   // Followed by int nSegment, int nSide, int nObject
#define ND_EVENT_HOSTAGE_RESCUED    10  // Followed by int hostageType
#define ND_EVENT_SOUND_3D_ONCE      11  // Followed by int soundum, int angle, int volume
#define ND_EVENT_MORPH_FRAME        12  // Followed by ? data
#define ND_EVENT_WALL_TOGGLE        13  // Followed by int seg, int nSide
#define ND_EVENT_HUD_MESSAGE        14  // Followed by char size, char * string (+null)
#define ND_EVENT_CONTROL_CENTER_DESTROYED 15 // Just a simple flag
#define ND_EVENT_PALETTE_EFFECT     16  // Followed by short r, g, b
#define ND_EVENT_PLAYER_ENERGY      17  // followed by byte energy
#define ND_EVENT_PLAYER_SHIELD      18  // followed by byte shields
#define ND_EVENT_PLAYER_FLAGS       19  // followed by player flags
#define ND_EVENT_PLAYER_WEAPON      20  // followed by weapon nType and weapon number
#define ND_EVENT_EFFECT_BLOWUP      21  // followed by tSegment, nSide, and pnt
#define ND_EVENT_HOMING_DISTANCE    22  // followed by homing distance
#define ND_EVENT_LETTERBOX          23  // letterbox mode for death seq.
#define ND_EVENT_RESTORE_COCKPIT    24  // restore cockpit after death
#define ND_EVENT_REARVIEW           25  // going to rear view mode
#define ND_EVENT_WALL_SET_TMAP_NUM1 26  // Wall changed
#define ND_EVENT_WALL_SET_TMAP_NUM2 27  // Wall changed
#define ND_EVENT_NEW_LEVEL          28  // followed by level number
#define ND_EVENT_MULTI_CLOAK        29  // followed by player num
#define ND_EVENT_MULTI_DECLOAK      30  // followed by player num
#define ND_EVENT_RESTORE_REARVIEW   31  // restore cockpit after rearview mode
#define ND_EVENT_MULTI_DEATH        32  // with player number
#define ND_EVENT_MULTI_KILL         33  // with player number
#define ND_EVENT_MULTI_CONNECT      34  // with player number
#define ND_EVENT_MULTI_RECONNECT    35  // with player number
#define ND_EVENT_MULTI_DISCONNECT   36  // with player number
#define ND_EVENT_MULTI_SCORE        37  // playernum / score
#define ND_EVENT_PLAYER_SCORE       38  // followed by score
#define ND_EVENT_PRIMARY_AMMO       39  // with old/new ammo count
#define ND_EVENT_SECONDARY_AMMO     40  // with old/new ammo count
#define ND_EVENT_DOOR_OPENING       41  // with tSegment/nSide
#define ND_EVENT_LASER_LEVEL        42  // no data
#define ND_EVENT_PLAYER_AFTERBURNER 43  // followed by byte old ab, current ab
#define ND_EVENT_CLOAKING_WALL      44  // info changing while wall cloaking
#define ND_EVENT_CHANGE_COCKPIT     45  // change the cockpit
#define ND_EVENT_START_GUIDED       46  // switch to guided view
#define ND_EVENT_END_GUIDED         47  // stop guided view/return to ship
#define ND_EVENT_SECRET_THINGY      48  // 0/1 = secret exit functional/non-functional
#define ND_EVENT_LINK_SOUND_TO_OBJ  49  // record DigiLinkSoundToObject3
#define ND_EVENT_KILL_SOUND_TO_OBJ  50  // record DigiKillSoundLinkedToObject


#define NORMAL_PLAYBACK         0
#define SKIP_PLAYBACK           1
#define INTERPOLATE_PLAYBACK    2
#define INTERPOL_FACTOR         (F1_0 + (F1_0/5))

#define DEMO_VERSION            15      // last D1 version was 13
#define DEMO_VERSION_D2X        16      // last D1 version was 13
#define DEMO_GAME_TYPE          3       // 1 was shareware, 2 registered

#define DEMO_FILENAME           "tmpdemo.dem"

#define DEMO_MAX_LEVELS         29

CFILE *infile = NULL;
CFILE *outfile = NULL;

//	-----------------------------------------------------------------------------

void InitDemoData (void)
{
gameData.demo.bAuto = 0;
gameData.demo.nState = 0;
gameData.demo.nVcrState = 0;
gameData.demo.nStartFrame = -1;
gameData.demo.bInterpolate = 0; // 1
gameData.demo.bWarningGiven = 0;
gameData.demo.bCtrlcenDestroyed = 0;
gameData.demo.nFrameBytesWritten = 0;
gameData.demo.bFirstTimePlayback = 1;
gameData.demo.xJasonPlaybackTotal = 0;
}

//	-----------------------------------------------------------------------------

int NDGetPercentDone () 
{
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return (CFTell (infile) * 100) / gameData.demo.nSize;
if (gameData.demo.nState == ND_STATE_RECORDING)
	return CFTell (outfile);
return 0;
}

//	-----------------------------------------------------------------------------

#define VEL_PRECISION 12

void my_extract_shortpos (tObject *objP, shortpos *spp)
{
	int nSegment;
	sbyte *sp;

sp = spp->bytemat;
objP->orient.rVec.x = *sp++ << MATRIX_PRECISION;
objP->orient.uVec.x = *sp++ << MATRIX_PRECISION;
objP->orient.fVec.x = *sp++ << MATRIX_PRECISION;
objP->orient.rVec.y = *sp++ << MATRIX_PRECISION;
objP->orient.uVec.y = *sp++ << MATRIX_PRECISION;
objP->orient.fVec.y = *sp++ << MATRIX_PRECISION;
objP->orient.rVec.z = *sp++ << MATRIX_PRECISION;
objP->orient.uVec.z = *sp++ << MATRIX_PRECISION;
objP->orient.fVec.z = *sp++ << MATRIX_PRECISION;
nSegment = spp->tSegment;
objP->nSegment = nSegment;
objP->pos.x = (spp->xo << RELPOS_PRECISION) + gameData.segs.vertices [gameData.segs.segments [nSegment].verts [0]].x;
objP->pos.y = (spp->yo << RELPOS_PRECISION) + gameData.segs.vertices [gameData.segs.segments [nSegment].verts [0]].y;
objP->pos.z = (spp->zo << RELPOS_PRECISION) + gameData.segs.vertices [gameData.segs.segments [nSegment].verts [0]].z;
objP->mType.physInfo.velocity.x = (spp->velx << VEL_PRECISION);
objP->mType.physInfo.velocity.y = (spp->vely << VEL_PRECISION);
objP->mType.physInfo.velocity.z = (spp->velz << VEL_PRECISION);
}

//	-----------------------------------------------------------------------------

int NDRead (void *buffer, int elsize, int nelem)
{
int nRead = (int) CFRead (buffer, elsize, nelem, infile);
if (CFError (infile) || CFEoF (infile))
	bNDBadRead = -1;
return nRead;
}

//	-----------------------------------------------------------------------------

int NDFindObject (int nSignature)
{
	int i;
	tObject * objP = gameData.objs.objects;

for (i = 0; i <= gameData.objs.nLastObject; i++, objP++) {
if ((objP->nType != OBJ_NONE) && (objP->nSignature == nSignature))
	return i;
	}
return -1;
}

//	-----------------------------------------------------------------------------

int NDWrite (void *buffer, int elsize, int nelem)
{
	int nWritten, nTotalSize = elsize * nelem;

gameData.demo.nFrameBytesWritten += nTotalSize;
gameData.demo.nWritten += nTotalSize;
Assert (outfile != NULL);
nWritten = CFWrite (buffer, elsize, nelem, outfile);
if ((gameData.demo.nWritten > gameData.demo.nSize) && !gameData.demo.bNoSpace)
	gameData.demo.bNoSpace = 1;
if ((nWritten == nelem) && !gameData.demo.bNoSpace)
	return nWritten;
gameData.demo.bNoSpace = 2;
NDStopRecording ();
return -1;
}

/*
 *  The next bunch of files taken from Matt's gamesave.c.  We have to modify
 *  these since the demo must save more information about gameData.objs.objects that
 *  just a gamesave
*/

//	-----------------------------------------------------------------------------

static inline void NDWriteByte (sbyte b)
{
gameData.demo.nFrameBytesWritten += sizeof (b);
gameData.demo.nWritten += sizeof (b);
CFWriteByte (b, outfile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteShort (short s)
{
gameData.demo.nFrameBytesWritten += sizeof (s);
gameData.demo.nWritten += sizeof (s);
CFWriteShort (s, outfile);
}

//	-----------------------------------------------------------------------------

static void NDWriteInt (int i)
{
gameData.demo.nFrameBytesWritten += sizeof (i);
gameData.demo.nWritten += sizeof (i);
CFWriteInt (i, outfile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteString (char *str)
{
	sbyte l = (int) strlen (str) + 1;
NDWriteByte ((sbyte) l);
NDWrite (str, (int) l, 1);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteFix (fix f)
{
gameData.demo.nFrameBytesWritten += sizeof (f);
gameData.demo.nWritten += sizeof (f);
CFWriteFix (f, outfile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteFixAng (fixang f)
{
gameData.demo.nFrameBytesWritten += sizeof (f);
gameData.demo.nWritten += sizeof (f);
CFWriteFix (f, outfile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteVector (vmsVector *v)
{
gameData.demo.nFrameBytesWritten += sizeof (*v);
gameData.demo.nWritten += sizeof (*v);
CFWriteVector (v, outfile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteAngVec (vmsAngVec *v)
{
gameData.demo.nFrameBytesWritten += sizeof (*v);
gameData.demo.nWritten += sizeof (*v);
CFWriteAngVec (v, outfile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteMatrix (vmsMatrix *m)
{
gameData.demo.nFrameBytesWritten += sizeof (*m);
gameData.demo.nWritten += sizeof (*m);
CFWriteMatrix (m, outfile);
}

//	-----------------------------------------------------------------------------

void NDWriteShortPos (tObject *objP)
{
	ubyte		renderType = objP->renderType;
#if ND_USE_SHORTPOS
	shortpos sp;
	CreateShortPos (&sp, objP, 0);
#endif
if ((renderType == RT_POLYOBJ) || (renderType == RT_HOSTAGE) || (renderType == RT_MORPH) || 
	 (objP->nType == OBJ_CAMERA)) {
#if ND_USE_SHORTPOS
	int		i;
	for (i = 0; i < 9; i++)
		NDWriteByte (sp.bytemat [i]);
	for (i = 0; i < 9; i++)
		if (sp.bytemat [i] != 0)
			break;
	if (i == 9)
		Int3 ();         // contact Allender about this.
#else
	 NDWriteMatrix (&objP->orient);
#endif
	}
#if ND_USE_SHORTPOS
NDWriteShort (sp.xo);
NDWriteShort (sp.yo);
NDWriteShort (sp.zo);
NDWriteShort (sp.tSegment);
NDWriteShort (sp.velx);
NDWriteShort (sp.vely);
NDWriteShort (sp.velz);
#else
NDWriteVector (&objP->pos);	
NDWriteShort (objP->nSegment);
NDWriteVector (&objP->mType.physInfo.velocity);
#endif
}

//	-----------------------------------------------------------------------------

static inline ubyte NDReadByte (void)
{
return CFReadByte (infile);
}

//	-----------------------------------------------------------------------------

static inline short NDReadShort (void)
{
return CFReadShort (infile);
}

//	-----------------------------------------------------------------------------

static inline int NDReadInt ()
{
return CFReadInt (infile);
}

//	-----------------------------------------------------------------------------

static inline char *NDReadString (char *str)
{
	sbyte len = NDReadByte ();
NDRead (str, len, 1);
return str;
}

//	-----------------------------------------------------------------------------

static inline fix NDReadFix (void)
{
return CFReadFix (infile);
}

//	-----------------------------------------------------------------------------

static inline fixang NDReadFixAng (void)
{
return CFReadFixAng (infile);
}

//	-----------------------------------------------------------------------------

static inline void NDReadVector (vmsVector *v)
{
CFReadVector (v, infile);
}

//	-----------------------------------------------------------------------------

static inline void NDReadAngVec (vmsAngVec *v)
{
CFReadAngVec (v, infile);
}

//	-----------------------------------------------------------------------------

static inline void NDReadMatrix (vmsMatrix *m)
{
CFReadMatrix (m, infile);
}

//	-----------------------------------------------------------------------------

static void NDReadShortPos (tObject *objP)
{
	shortpos sp;
	ubyte renderType;

renderType = objP->renderType;
if ((renderType == RT_POLYOBJ) || (renderType == RT_HOSTAGE) || (renderType == RT_MORPH) || 
		(objP->nType == OBJ_CAMERA)) {
	if (gameData.demo.bUseShortPos) {
		int i;
		for (i = 0; i < 9; i++)
			sp.bytemat [i] = NDReadByte ();
		}
	else {
		CFReadMatrix (&objP->orient, infile);
		}
	}
if (gameData.demo.bUseShortPos) {
	sp.xo = NDReadShort ();
	sp.yo = NDReadShort ();
	sp.zo = NDReadShort ();
	sp.tSegment = NDReadShort ();
	sp.velx = NDReadShort ();
	sp.vely = NDReadShort ();
	sp.velz = NDReadShort ();
	my_extract_shortpos (objP, &sp);
	}
else {
	NDReadVector (&objP->pos);
	objP->nSegment = NDReadShort ();
	NDReadVector (&objP->mType.physInfo.velocity);
	}
if ((objP->id == VCLIP_MORPHING_ROBOT) && 
		 (renderType == RT_FIREBALL) && 
		 (objP->controlType == CT_EXPLOSION))
	ExtractOrientFromSegment (&objP->orient, gameData.segs.segments + objP->nSegment);
}


//	-----------------------------------------------------------------------------

tObject *prevObjP = NULL;      //ptr to last tObject read in

void NDReadObject (tObject *objP)
{
memset (objP, 0, sizeof (tObject));
/*
 * Do render nType first, since with renderType == RT_NONE, we
 * blow by all other tObject information
 */
objP->renderType = NDReadByte ();
objP->nType = NDReadByte ();
if ((objP->renderType == RT_NONE) &&(objP->nType != OBJ_CAMERA))
	return;
objP->id = NDReadByte ();
objP->flags = NDReadByte ();
objP->nSignature = NDReadShort ();
NDReadShortPos (objP);
if ((objP->nType == OBJ_ROBOT) && (objP->id == SPECIAL_REACTOR_ROBOT))
	Int3 ();
objP->attachedObj = -1;
switch (objP->nType) {
	case OBJ_HOSTAGE:
		objP->controlType = CT_POWERUP;
		objP->movementType = MT_NONE;
		objP->size = HOSTAGE_SIZE;
		break;

	case OBJ_ROBOT:
		objP->controlType = CT_AI;
		// (MarkA and MikeK said we should not do the crazy last secret stuff with multiple reactors...
		// This necessary code is our vindication. --MK, 2/15/96)
		if (objP->id != SPECIAL_REACTOR_ROBOT)
			objP->movementType = MT_PHYSICS;
		else
			objP->movementType = MT_NONE;
		objP->size = gameData.models.polyModels [gameData.bots.pInfo [objP->id].nModel].rad;
		objP->rType.polyObjInfo.nModel = gameData.bots.pInfo [objP->id].nModel;
		objP->rType.polyObjInfo.nSubObjFlags = 0;
		objP->cType.aiInfo.CLOAKED = (gameData.bots.pInfo [objP->id].cloakType?1:0);
		break;

	case OBJ_POWERUP:
		objP->controlType = CT_POWERUP;
		objP->movementType = NDReadByte ();        // might have physics movement
		objP->size = gameData.objs.pwrUp.info [objP->id].size;
		break;

	case OBJ_PLAYER:
		objP->controlType = CT_NONE;
		objP->movementType = MT_PHYSICS;
		objP->size = gameData.models.polyModels [gameData.pig.ship.player->nModel].rad;
		objP->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;
		objP->rType.polyObjInfo.nSubObjFlags = 0;
		break;

	case OBJ_CLUTTER:
		objP->controlType = CT_NONE;
		objP->movementType = MT_NONE;
		objP->size = gameData.models.polyModels [objP->id].rad;
		objP->rType.polyObjInfo.nModel = objP->id;
		objP->rType.polyObjInfo.nSubObjFlags = 0;
		break;

	default:
		objP->controlType = NDReadByte ();
		objP->movementType = NDReadByte ();
		objP->size = NDReadFix ();
		break;
	}

NDReadVector (&objP->last_pos);
if ((objP->nType == OBJ_WEAPON) && (objP->renderType == RT_WEAPON_VCLIP))
	objP->lifeleft = NDReadFix ();
else {
	ubyte b;

	b = NDReadByte ();
	objP->lifeleft = (fix)b;
	// MWA old way -- won't work with big endian machines       NDReadByte ((sbyte *) (ubyte *)&(objP->lifeleft);
	objP->lifeleft = (fix) ((int) objP->lifeleft << 12);
	}
if (objP->nType == OBJ_ROBOT) {
	if (gameData.bots.pInfo [objP->id].bossFlag) {
		sbyte cloaked;
		cloaked = NDReadByte ();
		objP->cType.aiInfo.CLOAKED = cloaked;
		}
	}

switch (objP->movementType) {
	case MT_PHYSICS:
		NDReadVector (&objP->mType.physInfo.velocity);
		NDReadVector (&objP->mType.physInfo.thrust);
		break;

	case MT_SPINNING:
		NDReadVector (&objP->mType.spinRate);
		break;

	case MT_NONE:
		break;

	default:
		Int3 ();
	}

switch (objP->controlType) {
	case CT_EXPLOSION:
		objP->cType.explInfo.nSpawnTime = NDReadFix ();
		objP->cType.explInfo.nDeleteTime = NDReadFix ();
		objP->cType.explInfo.nDeleteObj = NDReadShort ();
		objP->cType.explInfo.nNextAttach = 
		objP->cType.explInfo.nPrevAttach = 
		objP->cType.explInfo.nAttachParent = -1;
		if (objP->flags & OF_ATTACHED) {     //attach to previous tObject
			Assert (prevObjP!=NULL);
			if (prevObjP->controlType == CT_EXPLOSION) {
				if ((prevObjP->flags & OF_ATTACHED) &&(prevObjP->cType.explInfo.nAttachParent != -1))
					AttachObject (gameData.objs.objects + prevObjP->cType.explInfo.nAttachParent, objP);
				else
					objP->flags &= ~OF_ATTACHED;
				}
			else
				AttachObject (prevObjP, objP);
			}
		break;

	case CT_LIGHT:
		objP->cType.lightInfo.intensity = NDReadFix ();
		break;

	case CT_AI:
	case CT_WEAPON:
	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
	case CT_POWERUP:
	case CT_SLEW:
	case CT_CNTRLCEN:
	case CT_REMOTE:
	case CT_MORPH:
		break;

	case CT_FLYTHROUGH:
	case CT_REPAIRCEN:
	default:
		Int3 ();
	}

switch (objP->renderType) {
	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: {
		int i, tmo;
		if ((objP->nType != OBJ_ROBOT) &&(objP->nType != OBJ_PLAYER) &&(objP->nType != OBJ_CLUTTER)) {
			objP->rType.polyObjInfo.nModel = NDReadInt ();
			objP->rType.polyObjInfo.nSubObjFlags = NDReadInt ();
			}
		if ((objP->nType != OBJ_PLAYER) &&(objP->nType != OBJ_DEBRIS))
		for (i = 0; i < gameData.models.polyModels [objP->rType.polyObjInfo.nModel].n_models; i++)
			NDReadAngVec (objP->rType.polyObjInfo.animAngles + i);
		tmo = NDReadInt ();
#ifndef EDITOR
		objP->rType.polyObjInfo.nTexOverride = tmo;
#else
		if (tmo==-1)
			objP->rType.polyObjInfo.nTexOverride = -1;
		else {
			int xlated_tmo = tmap_xlate_table [tmo];
			if (xlated_tmo < 0) {
				Int3 ();
				xlated_tmo = 0;
				}
			objP->rType.polyObjInfo.nTexOverride = xlated_tmo;
			}
#endif

		break;
		}

	case RT_POWERUP:
	case RT_WEAPON_VCLIP:
	case RT_FIREBALL:
	case RT_THRUSTER:
	case RT_HOSTAGE:
		objP->rType.vClipInfo.nClipIndex = NDReadInt ();
		objP->rType.vClipInfo.xFrameTime = NDReadFix ();
		objP->rType.vClipInfo.nCurFrame = NDReadByte ();
		break;

	case RT_LASER:
		break;

	default:
		Int3 ();
	}
prevObjP = objP;
}

//	-----------------------------------------------------------------------------

void NDWriteObject (tObject *objP)
{
	int life;

if ((objP->nType == OBJ_ROBOT) &&(objP->id == SPECIAL_REACTOR_ROBOT))
	Int3 ();
// Do renderType first so on read, we can make determination of
// what else to read in
NDWriteByte (objP->renderType);
NDWriteByte (objP->nType);
if ((objP->renderType == RT_NONE) &&(objP->nType != OBJ_CAMERA))
	return;
NDWriteByte (objP->id);
NDWriteByte (objP->flags);
NDWriteShort ((short)objP->nSignature);
NDWriteShortPos (objP);
if ((objP->nType != OBJ_HOSTAGE) &&(objP->nType != OBJ_ROBOT) &&(objP->nType != OBJ_PLAYER) &&(objP->nType != OBJ_POWERUP) &&(objP->nType != OBJ_CLUTTER)) {
	NDWriteByte (objP->controlType);
	NDWriteByte (objP->movementType);
	NDWriteFix (objP->size);
	}
if (objP->nType == OBJ_POWERUP)
	NDWriteByte (objP->movementType);
NDWriteVector (&objP->last_pos);
if ((objP->nType == OBJ_WEAPON) &&(objP->renderType == RT_WEAPON_VCLIP))
	NDWriteFix (objP->lifeleft);
else {
	life = (int)objP->lifeleft;
	life = life >> 12;
	if (life > 255)
		life = 255;
	NDWriteByte ((ubyte)life);
	}
if (objP->nType == OBJ_ROBOT) {
	if (gameData.bots.pInfo [objP->id].bossFlag) {
		if ((gameData.time.xGame > gameData.boss.nCloakStartTime) && 
			 (gameData.time.xGame < gameData.boss.nCloakEndTime))
			NDWriteByte (1);
		else
			NDWriteByte (0);
		}
	}
switch (objP->movementType) {
	case MT_PHYSICS:
		NDWriteVector (&objP->mType.physInfo.velocity);
		NDWriteVector (&objP->mType.physInfo.thrust);
		break;

	case MT_SPINNING:
		NDWriteVector (&objP->mType.spinRate);
		break;

	case MT_NONE:
		break;

	default:
		Int3 ();
	}

switch (objP->controlType) {
	case CT_AI:
		break;

	case CT_EXPLOSION:
		NDWriteFix (objP->cType.explInfo.nSpawnTime);
		NDWriteFix (objP->cType.explInfo.nDeleteTime);
		NDWriteShort (objP->cType.explInfo.nDeleteObj);
		break;

	case CT_WEAPON:
		break;

	case CT_LIGHT:
		NDWriteFix (objP->cType.lightInfo.intensity);
		break;

	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
	case CT_POWERUP:
	case CT_SLEW:       //the player is generally saved as slew
	case CT_CNTRLCEN:
	case CT_REMOTE:
	case CT_MORPH:
		break;

	case CT_REPAIRCEN:
	case CT_FLYTHROUGH:
	default:
		Int3 ();
	}

switch (objP->renderType) {
	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		if ((objP->nType != OBJ_ROBOT) &&(objP->nType != OBJ_PLAYER) &&(objP->nType != OBJ_CLUTTER)) {
			NDWriteInt (objP->rType.polyObjInfo.nModel);
			NDWriteInt (objP->rType.polyObjInfo.nSubObjFlags);
			}
		if ((objP->nType != OBJ_PLAYER) &&(objP->nType != OBJ_DEBRIS))
#if 0
			for (i=0;i<MAX_SUBMODELS;i++)
				NDWriteAngVec (&objP->polyObjInfo.animAngles [i]);
#endif
		for (i = 0; i < gameData.models.polyModels [objP->rType.polyObjInfo.nModel].n_models; i++)
			NDWriteAngVec (&objP->rType.polyObjInfo.animAngles [i]);
		NDWriteInt (objP->rType.polyObjInfo.nTexOverride);
		break;
		}

	case RT_POWERUP:
	case RT_WEAPON_VCLIP:
	case RT_FIREBALL:
	case RT_THRUSTER:
	case RT_HOSTAGE:
		NDWriteInt (objP->rType.vClipInfo.nClipIndex);
		NDWriteFix (objP->rType.vClipInfo.xFrameTime);
		NDWriteByte (objP->rType.vClipInfo.nCurFrame);
		break;

	case RT_LASER:
		break;

	default:
		Int3 ();
	}
}

//	-----------------------------------------------------------------------------

int	bJustStartedRecording = 0, 
		bJustStartedPlayback = 0;

void NDRecordStartDemo ()
{
	int i;

StopTime ();
NDWriteByte (ND_EVENT_START_DEMO);
NDWriteByte (DEMO_VERSION_D2X);
NDWriteByte (DEMO_GAME_TYPE);
NDWriteFix (gameData.time.xGame);
#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI)
	NDWriteInt (gameData.app.nGameMode | (gameData.multi.nLocalPlayer << 16));
else
#endif
	// NOTE LINK TO ABOVE!!!
	NDWriteInt (gameData.app.nGameMode);
#ifdef NETWORK
if (gameData.app.nGameMode & GM_TEAM) {
	NDWriteByte (netGame.team_vector);
	NDWriteString (netGame.team_name [0]);
	NDWriteString (netGame.team_name [1]);
	}
if (gameData.app.nGameMode & GM_MULTI) {
	NDWriteByte ((sbyte)gameData.multi.nPlayers);
	for (i = 0; i < gameData.multi.nPlayers; i++) {
		NDWriteString (gameData.multi.players [i].callsign);
		NDWriteByte (gameData.multi.players [i].connected);
		if (gameData.app.nGameMode & GM_MULTI_COOP)
			NDWriteInt (gameData.multi.players [i].score);
		else {
			NDWriteShort ((short)gameData.multi.players [i].netKilledTotal);
			NDWriteShort ((short)gameData.multi.players [i].netKillsTotal);
			}
		}
	} 
else
#endif
	// NOTE LINK TO ABOVE!!!
	NDWriteInt (gameData.multi.players [gameData.multi.nLocalPlayer].score);
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	NDWriteShort ((short)gameData.multi.players [gameData.multi.nLocalPlayer].primaryAmmo [i]);
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	NDWriteShort ((short)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [i]);
NDWriteByte ((sbyte)gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel);
//  Support for missions added here
NDWriteString (gameStates.app.szCurrentMissionFile);
NDWriteByte ((sbyte) (f2ir (gameData.multi.players [gameData.multi.nLocalPlayer].energy)));
NDWriteByte ((sbyte) (f2ir (gameData.multi.players [gameData.multi.nLocalPlayer].shields)));
NDWriteInt (gameData.multi.players [gameData.multi.nLocalPlayer].flags);        // be sure players flags are set
NDWriteByte ((sbyte)gameData.weapons.nPrimary);
NDWriteByte ((sbyte)gameData.weapons.nSecondary);
gameData.demo.nStartFrame = gameData.app.nFrameCount;
bJustStartedRecording=1;
NDSetNewLevel (gameData.missions.nCurrentLevel);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordStartFrame (int frame_number, fix frameTime)
{
if (gameData.demo.bNoSpace) {
	NDStopPlayback ();
	return;
	}
StopTime ();
memset (gameData.demo.bWasRecorded, 0, sizeof (gameData.demo.bWasRecorded));
memset (gameData.demo.bViewWasRecorded, 0, sizeof (gameData.demo.bViewWasRecorded));
memset (gameData.demo.bRenderingWasRecorded, 0, sizeof (gameData.demo.bRenderingWasRecorded));
frame_number -= gameData.demo.nStartFrame;
Assert (frame_number >= 0);
NDWriteByte (ND_EVENT_START_FRAME);
NDWriteShort ((short) (gameData.demo.nFrameBytesWritten - 1));        // from previous frame
gameData.demo.nFrameBytesWritten=3;
NDWriteInt (frame_number);
NDWriteInt (frameTime);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordRenderObject (tObject * objP)
{
if (gameData.demo.bViewWasRecorded [OBJ_IDX (objP)])
	return;
//if (obj==&gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].nObject] && !gameStates.app.bPlayerIsDead)
//	return;
StopTime ();
NDWriteByte (ND_EVENT_RENDER_OBJECT);
NDWriteObject (objP);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordViewerObject (tObject * objP)
{
	int	i = OBJ_IDX (objP);
	int	h = gameData.demo.bViewWasRecorded [i];
if (h &&(h - 1 == gameStates.render.nRenderingType))
	return;
//if (gameData.demo.bWasRecorded [OBJ_IDX (objP)])
//	return;
if (gameData.demo.bRenderingWasRecorded [gameStates.render.nRenderingType])
	return;
gameData.demo.bViewWasRecorded [i]=gameStates.render.nRenderingType+1;
gameData.demo.bRenderingWasRecorded [gameStates.render.nRenderingType]=1;
StopTime ();
NDWriteByte (ND_EVENT_VIEWER_OBJECT);
NDWriteByte (gameStates.render.nRenderingType);
NDWriteObject (objP);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordSound (int soundno)
{
StopTime ();
NDWriteByte (ND_EVENT_SOUND);
NDWriteInt (soundno);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordCockpitChange (int mode)
{
StopTime ();
NDWriteByte (ND_EVENT_CHANGE_COCKPIT);
NDWriteInt (mode);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordSound3D (int soundno, int angle, int volume)
{
StopTime ();
NDWriteByte (ND_EVENT_SOUND_3D);
NDWriteInt (soundno);
NDWriteInt (angle);
NDWriteInt (volume);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordSound3DOnce (int soundno, int angle, int volume)
{
StopTime ();
NDWriteByte (ND_EVENT_SOUND_3D_ONCE);
NDWriteInt (soundno);
NDWriteInt (angle);
NDWriteInt (volume);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordLinkSoundToObject3 (int soundno, short nObject, fix maxVolume, fix  maxDistance, int loop_start, int loop_end)
{
StopTime ();
NDWriteByte (ND_EVENT_LINK_SOUND_TO_OBJ);
NDWriteInt (soundno);
NDWriteInt (gameData.objs.objects [nObject].nSignature);
NDWriteInt (maxVolume);
NDWriteInt (maxDistance);
NDWriteInt (loop_start);
NDWriteInt (loop_end);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordKillSoundLinkedToObject (int nObject)
{
StopTime ();
NDWriteByte (ND_EVENT_KILL_SOUND_TO_OBJ);
NDWriteInt (gameData.objs.objects [nObject].nSignature);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordWallHitProcess (int nSegment, int nSide, int damage, int playernum)
{
StopTime ();
//nSegment = nSegment;
//nSide = nSide;
//damage = damage;
//playernum = playernum;
NDWriteByte (ND_EVENT_WALL_HIT_PROCESS);
NDWriteInt (nSegment);
NDWriteInt (nSide);
NDWriteInt (damage);
NDWriteInt (playernum);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordGuidedStart ()
{
NDWriteByte (ND_EVENT_START_GUIDED);
}

//	-----------------------------------------------------------------------------

void NDRecordGuidedEnd ()
{
NDWriteByte (ND_EVENT_END_GUIDED);
}

//	-----------------------------------------------------------------------------

void NDRecordSecretExitBlown (int truth)
{
StopTime ();
NDWriteByte (ND_EVENT_SECRET_THINGY);
NDWriteInt (truth);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordTrigger (int nSegment, int nSide, int nObject, int shot)
{
StopTime ();
NDWriteByte (ND_EVENT_TRIGGER);
NDWriteInt (nSegment);
NDWriteInt (nSide);
NDWriteInt (nObject);
NDWriteInt (shot);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordHostageRescued (int hostage_number) 
{
StopTime ();
NDWriteByte (ND_EVENT_HOSTAGE_RESCUED);
NDWriteInt (hostage_number);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMorphFrame (tMorphInfo *md)
{
StopTime ();
NDWriteByte (ND_EVENT_MORPH_FRAME);
NDWriteObject (md->objP);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordWallToggle (int nSegment, int nSide)
{
StopTime ();
NDWriteByte (ND_EVENT_WALL_TOGGLE);
NDWriteInt (nSegment);
NDWriteInt (nSide);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordControlCenterDestroyed ()
{
StopTime ();
NDWriteByte (ND_EVENT_CONTROL_CENTER_DESTROYED);
NDWriteInt (gameData.reactor.countdown.nSecsLeft);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordHUDMessage (char * message)
{
StopTime ();
NDWriteByte (ND_EVENT_HUD_MESSAGE);
NDWriteString (message);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPaletteEffect (short r, short g, short b)
{
StopTime ();
NDWriteByte (ND_EVENT_PALETTE_EFFECT);
NDWriteShort (r);
NDWriteShort (g);
NDWriteShort (b);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerEnergy (int old_energy, int energy)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_ENERGY);
NDWriteByte ((sbyte) old_energy);
NDWriteByte ((sbyte) energy);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerAfterburner (fix old_afterburner, fix afterburner)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_AFTERBURNER);
NDWriteByte ((sbyte) (old_afterburner>>9));
NDWriteByte ((sbyte) (afterburner>>9));
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerShields (int old_shield, int shield)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_SHIELD);
NDWriteByte ((sbyte)old_shield);
NDWriteByte ((sbyte)shield);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerFlags (uint oflags, uint flags)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_FLAGS);
NDWriteInt (( (short)oflags << 16) | (short)flags);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerWeapon (int nWeaponType, int weapon_num)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_WEAPON);
NDWriteByte ((sbyte)nWeaponType);
NDWriteByte ((sbyte)weapon_num);
if (nWeaponType)
	NDWriteByte ((sbyte)gameData.weapons.nSecondary);
else
	NDWriteByte ((sbyte)gameData.weapons.nPrimary);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordEffectBlowup (short tSegment, int nSide, vmsVector *pnt)
{
StopTime ();
NDWriteByte (ND_EVENT_EFFECT_BLOWUP);
NDWriteShort (tSegment);
NDWriteByte ((sbyte)nSide);
NDWriteVector (pnt);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordHomingDistance (fix distance)
{
StopTime ();
NDWriteByte (ND_EVENT_HOMING_DISTANCE);
NDWriteShort ((short) (distance>>16));
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordLetterbox (void)
{
StopTime ();
NDWriteByte (ND_EVENT_LETTERBOX);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordRearView (void)
{
StopTime ();
NDWriteByte (ND_EVENT_REARVIEW);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordRestoreCockpit (void)
{
StopTime ();
NDWriteByte (ND_EVENT_RESTORE_COCKPIT);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordRestoreRearView (void)
{
StopTime ();
NDWriteByte (ND_EVENT_RESTORE_REARVIEW);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordWallSetTMapNum1 (short seg, ubyte nSide, short cseg, ubyte cside, short tmap)
{
StopTime ();
NDWriteByte (ND_EVENT_WALL_SET_TMAP_NUM1);
NDWriteShort (seg);
NDWriteByte (nSide);
NDWriteShort (cseg);
NDWriteByte (cside);
NDWriteShort (tmap);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordWallSetTMapNum2 (short seg, ubyte nSide, short cseg, ubyte cside, short tmap)
{
StopTime ();
NDWriteByte (ND_EVENT_WALL_SET_TMAP_NUM2);
NDWriteShort (seg);
NDWriteByte (nSide);
NDWriteShort (cseg);
NDWriteByte (cside);
NDWriteShort (tmap);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiCloak (int pnum)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_CLOAK);
NDWriteByte ((sbyte)pnum);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiDeCloak (int pnum)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_DECLOAK);
NDWriteByte ((sbyte)pnum);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiDeath (int pnum)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_DEATH);
NDWriteByte ((sbyte)pnum);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiKill (int pnum, sbyte kill)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_KILL);
NDWriteByte ((sbyte)pnum);
NDWriteByte (kill);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiConnect (int pnum, int new_player, char *new_callsign)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_CONNECT);
NDWriteByte ((sbyte)pnum);
NDWriteByte ((sbyte)new_player);
if (!new_player) {
	NDWriteString (gameData.multi.players [pnum].callsign);
	NDWriteInt (gameData.multi.players [pnum].netKilledTotal);
	NDWriteInt (gameData.multi.players [pnum].netKillsTotal);
	}
NDWriteString (new_callsign);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiReconnect (int pnum)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_RECONNECT);
NDWriteByte ((sbyte)pnum);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiDisconnect (int pnum)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_DISCONNECT);
NDWriteByte ((sbyte)pnum);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerScore (int score)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_SCORE);
NDWriteInt (score);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiScore (int pnum, int score)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_SCORE);
NDWriteByte ((sbyte)pnum);
NDWriteInt (score - gameData.multi.players [pnum].score);      // called before score is changed!!!!
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPrimaryAmmo (int old_ammo, int new_ammo)
{
StopTime ();
NDWriteByte (ND_EVENT_PRIMARY_AMMO);
if (old_ammo < 0)
	NDWriteShort ((short)new_ammo);
else
	NDWriteShort ((short)old_ammo);
NDWriteShort ((short)new_ammo);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordSecondaryAmmo (int old_ammo, int new_ammo)
{
StopTime ();
NDWriteByte (ND_EVENT_SECONDARY_AMMO);
if (old_ammo < 0)
	NDWriteShort ((short)new_ammo);
else
	NDWriteShort ((short)old_ammo);
NDWriteShort ((short)new_ammo);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordDoorOpening (int nSegment, int nSide)
{
StopTime ();
NDWriteByte (ND_EVENT_DOOR_OPENING);
NDWriteShort ((short)nSegment);
NDWriteByte ((sbyte)nSide);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordLaserLevel (sbyte oldLevel, sbyte newLevel)
{
StopTime ();
NDWriteByte (ND_EVENT_LASER_LEVEL);
NDWriteByte (oldLevel);
NDWriteByte (newLevel);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordCloakingWall (int front_wall_num, int back_wall_num, ubyte nType, ubyte state, fix cloakValue, fix l0, fix l1, fix l2, fix l3)
{
Assert (front_wall_num <= 255 && back_wall_num <= 255);
StopTime ();
NDWriteByte (ND_EVENT_CLOAKING_WALL);
NDWriteByte ((sbyte) front_wall_num);
NDWriteByte ((sbyte) back_wall_num);
NDWriteByte ((sbyte) nType);
NDWriteByte ((sbyte) state);
NDWriteByte ((sbyte) cloakValue);
NDWriteShort ((short) (l0>>8));
NDWriteShort ((short) (l1>>8));
NDWriteShort ((short) (l2>>8));
NDWriteShort ((short) (l3>>8));
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDSetNewLevel (int level_num)
{
	int i;
	int nSide;
	tSegment *seg;

StopTime ();
NDWriteByte (ND_EVENT_NEW_LEVEL);
NDWriteByte ((sbyte)level_num);
NDWriteByte ((sbyte)gameData.missions.nCurrentLevel);

if (bJustStartedRecording==1) {
	NDWriteInt (gameData.walls.nWalls);
	for (i = 0; i < gameData.walls.nWalls; i++) {
		NDWriteByte (gameData.walls.walls [i].nType);
		NDWriteByte (gameData.walls.walls [i].flags);
		NDWriteByte (gameData.walls.walls [i].state);
		seg = &gameData.segs.segments [gameData.walls.walls [i].nSegment];
		nSide = gameData.walls.walls [i].nSide;
		NDWriteShort (seg->sides [nSide].nBaseTex);
		NDWriteShort (seg->sides [nSide].nOvlTex | (seg->sides [nSide].nOvlOrient << 14));
		bJustStartedRecording=0;
		}
	}
StartTime ();
}

//	-----------------------------------------------------------------------------

int NDReadDemoStart (int rnd_demo)
{
	sbyte i, version, gameType, laserLevel;
	char c, energy, shield;
	char text [128], szCurrentMission [FILENAME_LEN];

c = NDReadByte ();
if ((c != ND_EVENT_START_DEMO) || bNDBadRead) {
	tMenuItem m [1];

	sprintf (text, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_CORRUPT);
	memset (m, 0, sizeof (m));
	m [0].nType = NM_TYPE_TEXT; 
	m [0].text = text;
	ExecMenu (NULL, NULL, sizeof (m)/sizeof (*m), m, NULL, NULL);
	return 1;
	}
version = NDReadByte ();
gameType = NDReadByte ();
if (gameType < DEMO_GAME_TYPE) {
	tMenuItem m [2];

	sprintf (text, "%s %s", TXT_CANT_PLAYBACK, TXT_RECORDED);
	memset (m, 0, sizeof (m));
	m [0].nType = NM_TYPE_TEXT; 
	m [0].text = text;
	m [1].nType = NM_TYPE_TEXT; 
	m [1].text = "    In Descent: First Strike";

	ExecMenu (NULL, NULL, sizeof (m)/sizeof (*m), m, NULL, NULL);
	return 1;
	}
if (gameType != DEMO_GAME_TYPE) {
	tMenuItem m [2];

	sprintf (text, "%s %s", TXT_CANT_PLAYBACK, TXT_RECORDED);
	memset (m, 0, sizeof (m));
	m [0].nType = NM_TYPE_TEXT; 
	m [0].text = text;
	m [1].nType = NM_TYPE_TEXT; 
	m [1].text = "   In Unknown Descent version";

	ExecMenu (NULL, NULL, sizeof (m)/sizeof (*m), m, NULL, NULL);
	return 1;
	}
if (version < DEMO_VERSION) {
	if (!rnd_demo) {
		tMenuItem m [1];
		sprintf (text, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_OLD);
		memset (m, 0, sizeof (m));
		m [0].nType = NM_TYPE_TEXT; 
		m [0].text = text;
		ExecMenu (NULL, NULL, sizeof (m)/sizeof (*m), m, NULL, NULL);
		}
		return 1;
	}
gameData.demo.bUseShortPos = (version == DEMO_VERSION);
gameData.time.xGame = NDReadFix ();
gameData.boss.nCloakStartTime =
gameData.boss.nCloakEndTime = gameData.time.xGame;
gameData.demo.xJasonPlaybackTotal = 0;
gameData.demo.nGameMode = NDReadInt ();
#ifdef NETWORK
ChangePlayerNumTo ((gameData.demo.nGameMode >> 16) & 0x7);
if (gameData.demo.nGameMode & GM_TEAM) {
	netGame.team_vector = NDReadByte ();
	NDReadString (netGame.team_name [0]);
	NDReadString (netGame.team_name [1]);
	}
if (gameData.demo.nGameMode & GM_MULTI) {
	MultiNewGame ();
	c = NDReadByte ();
	gameData.multi.nPlayers = (int)c;
	// changed this to above two lines -- breaks on the mac because of
	// endian issues
	//		NDReadByte ((sbyte *)&gameData.multi.nPlayers);
	for (i = 0 ; i < gameData.multi.nPlayers; i++) {
		gameData.multi.players [i].cloakTime = 0;
		gameData.multi.players [i].invulnerableTime = 0;
		NDReadString (gameData.multi.players [i].callsign);
		gameData.multi.players [i].connected = (sbyte) NDReadByte ();
		if (gameData.demo.nGameMode & GM_MULTI_COOP)
			gameData.multi.players [i].score = NDReadInt ();
		else {
			gameData.multi.players [i].netKilledTotal = NDReadShort ();
			gameData.multi.players [i].netKillsTotal = NDReadShort ();
			}
		}
	gameData.app.nGameMode = gameData.demo.nGameMode;
	MultiSortKillList ();
	gameData.app.nGameMode = GM_NORMAL;
	}
else
#endif
	gameData.multi.players [gameData.multi.nLocalPlayer].score = NDReadInt ();      // Note link to above if!
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	gameData.multi.players [gameData.multi.nLocalPlayer].primaryAmmo [i] = NDReadShort ();
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [i] = NDReadShort ();
laserLevel = NDReadByte ();
if (laserLevel != gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel) {
	gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel = laserLevel;
	UpdateLaserWeaponInfo ();
	}
// Support for missions
NDReadString (szCurrentMission);
if (!LoadMissionByName (szCurrentMission, -1)) {
	if (!rnd_demo) {
		tMenuItem m [1];

		sprintf (text, TXT_NOMISSION4DEMO, szCurrentMission);
		memset (m, 0, sizeof (m));
		m [0].nType = NM_TYPE_TEXT; 
		m [0].text = text;
		ExecMenu (NULL, NULL, sizeof (m)/sizeof (*m), m, NULL, NULL);
		}
	return 1;
	}
gameData.demo.xRecordedTotal = 0;
gameData.demo.xPlaybackTotal = 0;
energy = NDReadByte ();
shield = NDReadByte ();
gameData.multi.players [gameData.multi.nLocalPlayer].flags = NDReadInt ();
if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
	gameData.multi.players [gameData.multi.nLocalPlayer].cloakTime = gameData.time.xGame - (CLOAK_TIME_MAX / 2);
	gameData.demo.bPlayersCloaked |= (1 << gameData.multi.nLocalPlayer);
	}
if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE)
	gameData.multi.players [gameData.multi.nLocalPlayer].invulnerableTime = gameData.time.xGame - (INVULNERABLE_TIME_MAX / 2);
gameData.weapons.nPrimary = NDReadByte ();
gameData.weapons.nSecondary = NDReadByte ();
// Next bit of code to fix problem that I introduced between 1.0 and 1.1
// check the next byte -- it _will_ be a load_newLevel event.  If it is
// not, then we must shift all bytes up by one.
gameData.multi.players [gameData.multi.nLocalPlayer].energy = i2f (energy);
gameData.multi.players [gameData.multi.nLocalPlayer].shields = i2f (shield);
bJustStartedPlayback=1;
return 0;
}

//	-----------------------------------------------------------------------------

void NDPopCtrlCenTriggers ()
{
	short		anim_num, n, i;
	short		side, cside;
	tSegment *seg, *csegp;

for (i = 0; i < gameData.reactor.triggers.nLinks; i++) {
	seg = gameData.segs.segments + gameData.reactor.triggers.nSegment [i];
	side = gameData.reactor.triggers.nSide [i];
	csegp = gameData.segs.segments + seg->children [side];
	cside = FindConnectedSide (seg, csegp);
	anim_num = gameData.walls.walls [WallNumP (seg, side)].nClip;
	n = gameData.walls.pAnims [anim_num].nFrameCount;
	if (gameData.walls.pAnims [anim_num].flags & WCF_TMAP1)
		seg->sides [side].nBaseTex = 
		csegp->sides [cside].nBaseTex = gameData.walls.pAnims [anim_num].frames [n-1];
	else
		seg->sides [side].nOvlTex = 
		csegp->sides [cside].nOvlTex = gameData.walls.pAnims [anim_num].frames [n-1];
	}
}

//	-----------------------------------------------------------------------------

int NDUpdateSmoke (void)
{
if (!EGI_FLAG (bUseSmoke, 0, 0))
	return 0;
else {
		int		i, nObject;
		tSmoke	*pSmoke = gameData.smoke.smoke;

	for (i = gameData.smoke.iUsedSmoke; i >= 0; i = pSmoke->nNext) {
		pSmoke = gameData.smoke.smoke + i;
		nObject = NDFindObject (pSmoke->nSignature);
		if (nObject < 0) {
			gameData.smoke.objects [pSmoke->nObject] = -1;
			SetSmokeLife (i, 0);
			}
		else {
			gameData.smoke.objects [nObject] = i;
			pSmoke->nObject = nObject;
			}
		}
	return 1;
	}
}

//	-----------------------------------------------------------------------------

#define N_PLAYER_SHIP_TEXTURES 6

void NDRenderExtras (ubyte, tObject *); extern void MultiApplyGoalTextures ();

int NDReadFrameInfo ()
{
	int bDone, nSegment, nTexture, nSide, nObject, soundno, angle, volume, i, shot;
	tObject *objP;
	ubyte c, WhichWindow;
	static sbyte saved_letter_cockpit;
	static sbyte saved_rearview_cockpit;
	tObject extraobj;
	static char LastReadValue=101;
	tSegment *seg;

	bDone = 0;

if (gameData.demo.nVcrState != ND_STATE_PAUSED)
	for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++)
		gameData.segs.segments [nSegment].objects = -1;
ResetObjects (1);
gameData.multi.players [gameData.multi.nLocalPlayer].homingObjectDist = -F1_0;
prevObjP = NULL;
while (!bDone) {
	c = NDReadByte ();
	CATCH_BAD_READ
	switch (c) {
		case ND_EVENT_START_FRAME: {        // Followed by an integer frame number, then a fix gameData.time.xFrame
			short last_frame_length;

			bDone=1;
			last_frame_length = NDReadShort ();
			gameData.demo.nFrameCount = NDReadInt ();
			gameData.demo.xRecordedTime = NDReadInt ();
			if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
				gameData.demo.xRecordedTotal += gameData.demo.xRecordedTime;
			gameData.demo.nFrameCount--;
			CATCH_BAD_READ
			break;
			}

		case ND_EVENT_VIEWER_OBJECT:        // Followed by an tObject structure
			WhichWindow = NDReadByte ();
			if (WhichWindow&15) {
				NDReadObject (&extraobj);
				if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
					CATCH_BAD_READ
					NDRenderExtras (WhichWindow, &extraobj);
					}
				}
			else {
				//gameData.objs.viewer=&gameData.objs.objects [0];
				NDReadObject (gameData.objs.viewer);
				if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
					CATCH_BAD_READ
					nSegment = gameData.objs.viewer->nSegment;
					gameData.objs.viewer->next = 
					gameData.objs.viewer->prev = 
					gameData.objs.viewer->nSegment = -1;

					// HACK HACK HACK -- since we have multiple level recording, it can be the case
					// HACK HACK HACK -- that when rewinding the demo, the viewer is in a tSegment
					// HACK HACK HACK -- that is greater than the highest index of segments.  Bash
					// HACK HACK HACK -- the viewer to tSegment 0 for bogus view.

					if (nSegment > gameData.segs.nLastSegment)
						nSegment = 0;
					LinkObject (OBJ_IDX (gameData.objs.viewer), nSegment);
					}
				}
			break;

		case ND_EVENT_RENDER_OBJECT:       // Followed by an tObject structure
			nObject = AllocObject ();
			if (nObject == -1)
				break;
			objP = gameData.objs.objects + nObject;
			NDReadObject (objP);
			CATCH_BAD_READ
			if (objP->controlType == CT_POWERUP)
				DoPowerupFrame (objP);
			if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
				nSegment = objP->nSegment;
				objP->next = objP->prev = objP->nSegment = -1;
				// HACK HACK HACK -- don't render gameData.objs.objects is segments greater than gameData.segs.nLastSegment
				// HACK HACK HACK -- (see above)
				if (nSegment > gameData.segs.nLastSegment)
					break;
				LinkObject (OBJ_IDX (objP), nSegment);
#ifdef NETWORK
				if ((objP->nType == OBJ_PLAYER) &&(gameData.demo.nGameMode & GM_MULTI)) {
					int player = (gameData.demo.nGameMode & GM_TEAM) ? GetTeam (objP->id) : objP->id;
					if (player == 0)
						break;
					player--;
					for (i = 0; i < N_PLAYER_SHIP_TEXTURES; i++)
						multi_player_textures [player] [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [gameData.models.polyModels [objP->rType.polyObjInfo.nModel].first_texture+i]];
					multi_player_textures [player] [4] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [gameData.pig.tex.nFirstMultiBitmap+ (player)*2]];
					multi_player_textures [player] [5] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [gameData.pig.tex.nFirstMultiBitmap+ (player)*2+1]];
					objP->rType.polyObjInfo.nAltTextures = player+1;
					}
#endif
				}
			break;

		case ND_EVENT_SOUND:
			soundno = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
				DigiPlaySample ((short) soundno, F1_0);
			break;

			//--unused		case ND_EVENT_SOUND_ONCE:
			//--unused			NDReadInt (&soundno);
			//--unused			if (bNDBadRead) { bDone = -1; break; }
			//--unused			if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
			//--unused				DigiPlaySampleOnce (soundno, F1_0);
			//--unused			break;

		case ND_EVENT_SOUND_3D:
			soundno = NDReadInt ();
			angle = NDReadInt ();
			volume = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
				DigiPlaySample3D ((short) soundno, angle, volume, 0);
			break;

		case ND_EVENT_SOUND_3D_ONCE:
			soundno = NDReadInt ();
			angle = NDReadInt ();
			volume = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
				DigiPlaySample3D ((short) soundno, angle, volume, 1);
			break;

		case ND_EVENT_LINK_SOUND_TO_OBJ: {
				int soundno, nObject, maxVolume, maxDistance, loop_start, loop_end;
				int nSignature;

			soundno = NDReadInt ();
			nSignature = NDReadInt ();
			maxVolume = NDReadInt ();
			maxDistance = NDReadInt ();
			loop_start = NDReadInt ();
			loop_end = NDReadInt ();
			nObject = NDFindObject (nSignature);
			if (nObject > -1)   //  @mk, 2/22/96, John told me to.
				DigiLinkSoundToObject3 ((short) soundno, (short) nObject, 1, maxVolume, maxDistance, loop_start, loop_end);
			}
			break;

		case ND_EVENT_KILL_SOUND_TO_OBJ: {
				int nObject, nSignature;

			nSignature = NDReadInt ();
			nObject = NDFindObject (nSignature);
			if (nObject > -1)   //  @mk, 2/22/96, John told me to.
				DigiKillSoundLinkedToObject (nObject);
			}
			break;

		case ND_EVENT_WALL_HIT_PROCESS: {
				int player, nSegment;
				fix damage;

			nSegment = NDReadInt ();
			nSide = NDReadInt ();
			damage = NDReadFix ();
			player = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState != ND_STATE_PAUSED)
				WallHitProcess (&gameData.segs.segments [nSegment], (short) nSide, damage, player, &(gameData.objs.objects [0]));
			break;
		}

		case ND_EVENT_TRIGGER:
			nSegment = NDReadInt ();
			nSide = NDReadInt ();
			nObject = NDReadInt ();
			shot = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
				if (gameData.trigs.triggers [gameData.walls.walls [WallNumI ((short) nSegment, (short) nSide)].nTrigger].nType == TT_SECRET_EXIT) {
					int truth;

					c = NDReadByte ();
					Assert (c == ND_EVENT_SECRET_THINGY);
					truth = NDReadInt ();
					if (!truth)
						CheckTrigger (gameData.segs.segments + nSegment, (short) nSide, (short) nObject, shot);
					} 
				else
					CheckTrigger (gameData.segs.segments + nSegment, (short) nSide, (short) nObject, shot);
				}
			break;

		case ND_EVENT_HOSTAGE_RESCUED: {
				int hostage_number;

			hostage_number = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState != ND_STATE_PAUSED)
				hostage_rescue (hostage_number);
			}
			break;

		case ND_EVENT_MORPH_FRAME: {
			nObject = AllocObject ();
			if (nObject==-1)
				break;
			objP = gameData.objs.objects + nObject;
			NDReadObject (objP);
			objP->renderType = RT_POLYOBJ;
			if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
				CATCH_BAD_READ
				if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
					nSegment = objP->nSegment;
					objP->next = objP->prev = objP->nSegment = -1;
					LinkObject (OBJ_IDX (objP), nSegment);
					}
				}
			}
			break;

		case ND_EVENT_WALL_TOGGLE:
			nSegment = NDReadInt ();
			nSide = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState != ND_STATE_PAUSED)
				WallToggle (gameData.segs.segments + nSegment, (short) nSide);
			break;

		case ND_EVENT_CONTROL_CENTER_DESTROYED:
			gameData.reactor.countdown.nSecsLeft = NDReadInt ();
			gameData.reactor.bDestroyed = 1;
			CATCH_BAD_READ
			if (!gameData.demo.bCtrlcenDestroyed) {
				NDPopCtrlCenTriggers ();
				gameData.demo.bCtrlcenDestroyed = 1;
				//DoReactorDestroyedStuff (NULL);
				}
			break;

		case ND_EVENT_HUD_MESSAGE: {
			char hud_msg [60];

			NDReadString (hud_msg);
			CATCH_BAD_READ
			HUDInitMessage (hud_msg);
			}
			break;

		case ND_EVENT_START_GUIDED:
			gameData.demo.bFlyingGuided=1;
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.demo.bFlyingGuided=0;
			break;

		case ND_EVENT_END_GUIDED:
			gameData.demo.bFlyingGuided=0;
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.demo.bFlyingGuided=1;
			break;

		case ND_EVENT_PALETTE_EFFECT: {
			short r, g, b;

			r = NDReadShort ();
			g = NDReadShort ();
			b = NDReadShort ();
			CATCH_BAD_READ
			PALETTE_FLASH_SET (r, g, b);
			}
			break;

		case ND_EVENT_PLAYER_ENERGY: {
			ubyte energy;
			ubyte old_energy;

			old_energy = NDReadByte ();
			energy = NDReadByte ();
			CATCH_BAD_READ
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multi.players [gameData.multi.nLocalPlayer].energy = i2f (energy);
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_energy != 255)
					gameData.multi.players [gameData.multi.nLocalPlayer].energy = i2f (old_energy);
				}
			}
			break;

		case ND_EVENT_PLAYER_AFTERBURNER: {
			ubyte afterburner;
			ubyte old_afterburner;

			old_afterburner = NDReadByte ();
			afterburner = NDReadByte ();
			CATCH_BAD_READ
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				xAfterburnerCharge = afterburner<<9;
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_afterburner != 255)
					xAfterburnerCharge = old_afterburner<<9;
				}
			break;
		}

		case ND_EVENT_PLAYER_SHIELD: {
			ubyte shield;
			ubyte old_shield;

			old_shield = NDReadByte ();
			shield = NDReadByte ();
			CATCH_BAD_READ
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multi.players [gameData.multi.nLocalPlayer].shields = i2f (shield);
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_shield != 255)
					gameData.multi.players [gameData.multi.nLocalPlayer].shields = i2f (old_shield);
				}
			}
			break;

		case ND_EVENT_PLAYER_FLAGS: {
			uint oflags;

			gameData.multi.players [gameData.multi.nLocalPlayer].flags = NDReadInt ();
			CATCH_BAD_READ
			oflags = gameData.multi.players [gameData.multi.nLocalPlayer].flags >> 16;
			gameData.multi.players [gameData.multi.nLocalPlayer].flags &= 0xffff;

			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 ((gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD) && (oflags != 0xffff))) {
				if (!(oflags & PLAYER_FLAGS_CLOAKED) &&(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)) {
					gameData.multi.players [gameData.multi.nLocalPlayer].cloakTime = 0;
					gameData.demo.bPlayersCloaked &= ~ (1 << gameData.multi.nLocalPlayer);
					}
				if ((oflags & PLAYER_FLAGS_CLOAKED) && !(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)) {
					gameData.multi.players [gameData.multi.nLocalPlayer].cloakTime = gameData.time.xGame - (CLOAK_TIME_MAX / 2);
					gameData.demo.bPlayersCloaked |= (1 << gameData.multi.nLocalPlayer);
					}
				if (!(oflags & PLAYER_FLAGS_INVULNERABLE) &&(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE))
					gameData.multi.players [gameData.multi.nLocalPlayer].invulnerableTime = 0;
				if ((oflags & PLAYER_FLAGS_INVULNERABLE) && !(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE))
					gameData.multi.players [gameData.multi.nLocalPlayer].invulnerableTime = gameData.time.xGame - (INVULNERABLE_TIME_MAX / 2);
				gameData.multi.players [gameData.multi.nLocalPlayer].flags = oflags;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				if (!(oflags & PLAYER_FLAGS_CLOAKED) &&(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)) {
					gameData.multi.players [gameData.multi.nLocalPlayer].cloakTime = gameData.time.xGame - (CLOAK_TIME_MAX / 2);
					gameData.demo.bPlayersCloaked |= (1 << gameData.multi.nLocalPlayer);
					}
				if ((oflags & PLAYER_FLAGS_CLOAKED) && !(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)) {
					gameData.multi.players [gameData.multi.nLocalPlayer].cloakTime = 0;
					gameData.demo.bPlayersCloaked &= ~ (1 << gameData.multi.nLocalPlayer);
					}
				if (!(oflags & PLAYER_FLAGS_INVULNERABLE) &&(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE))
					gameData.multi.players [gameData.multi.nLocalPlayer].invulnerableTime = gameData.time.xGame - (INVULNERABLE_TIME_MAX / 2);
				if ((oflags & PLAYER_FLAGS_INVULNERABLE) && !(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE))
					gameData.multi.players [gameData.multi.nLocalPlayer].invulnerableTime = 0;
				}
			UpdateLaserWeaponInfo ();     // in case of quad laser change
			}
			break;

		case ND_EVENT_PLAYER_WEAPON: {
			sbyte nWeaponType, weapon_num;
			sbyte old_weapon;

			nWeaponType = NDReadByte ();
			weapon_num = NDReadByte ();
			old_weapon = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				if (nWeaponType == 0)
					gameData.weapons.nPrimary = (int)weapon_num;
				else
					gameData.weapons.nSecondary = (int)weapon_num;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (nWeaponType == 0)
					gameData.weapons.nPrimary = (int)old_weapon;
				else
					gameData.weapons.nSecondary = (int)old_weapon;
				}
			}
			break;

		case ND_EVENT_EFFECT_BLOWUP: {
			short nSegment;
			sbyte nSide;
			vmsVector pnt;
			tObject dummy;

			//create a dummy tObject which will be the weapon that hits
			//the monitor. the blowup code wants to know who the parent of the
			//laser is, so create a laser whose parent is the player
			dummy.cType.laserInfo.parentType = OBJ_PLAYER;
			nSegment = NDReadShort ();
			nSide = NDReadByte ();
			NDReadVector (&pnt);
			if (gameData.demo.nVcrState != ND_STATE_PAUSED)
				CheckEffectBlowup (gameData.segs.segments + nSegment, nSide, &pnt, &dummy, 0);
			}
			break;

		case ND_EVENT_HOMING_DISTANCE: {
			short distance;

			distance = NDReadShort ();
			gameData.multi.players [gameData.multi.nLocalPlayer].homingObjectDist = 
				i2f ((int) distance << 16);
			}
			break;

		case ND_EVENT_LETTERBOX:
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				saved_letter_cockpit = gameStates.render.cockpit.nMode;
				SelectCockpit (CM_LETTERBOX);
				}
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				SelectCockpit (saved_letter_cockpit);
			break;

		case ND_EVENT_CHANGE_COCKPIT: {
				int dummy;

			dummy = NDReadInt ();
			SelectCockpit (dummy);
			}
			break;

		case ND_EVENT_REARVIEW:
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				saved_rearview_cockpit = gameStates.render.cockpit.nMode;
				if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT)
					SelectCockpit (CM_REAR_VIEW);
				gameStates.render.bRearView=1;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (saved_rearview_cockpit == CM_REAR_VIEW)     // hack to be sure we get a good cockpit on restore
					saved_rearview_cockpit = CM_FULL_COCKPIT;
				SelectCockpit (saved_rearview_cockpit);
				gameStates.render.bRearView = 0;
				}
			break;

		case ND_EVENT_RESTORE_COCKPIT:
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				saved_letter_cockpit = gameStates.render.cockpit.nMode;
				SelectCockpit (CM_LETTERBOX);
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				SelectCockpit (saved_letter_cockpit);
			break;


		case ND_EVENT_RESTORE_REARVIEW:
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				saved_rearview_cockpit = gameStates.render.cockpit.nMode;
				if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT)
					SelectCockpit (CM_REAR_VIEW);
				gameStates.render.bRearView=1;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				if (saved_rearview_cockpit == CM_REAR_VIEW)     // hack to be sure we get a good cockpit on restore
					saved_rearview_cockpit = CM_FULL_COCKPIT;
				SelectCockpit (saved_rearview_cockpit);
				gameStates.render.bRearView=0;
				}
			break;


		case ND_EVENT_WALL_SET_TMAP_NUM1: {
			short seg, cseg, tmap;
			ubyte nSide, cside;

			seg = NDReadShort ();
			nSide = NDReadByte ();
			cseg = NDReadShort ();
			cside = NDReadByte ();
			tmap = NDReadShort ();
			if ((gameData.demo.nVcrState != ND_STATE_PAUSED) && 
				 (gameData.demo.nVcrState != ND_STATE_REWINDING) &&
				 (gameData.demo.nVcrState != ND_STATE_ONEFRAMEBACKWARD))
				gameData.segs.segments [seg].sides [nSide].nBaseTex = 
					gameData.segs.segments [cseg].sides [cside].nBaseTex = tmap;
			}
			break;

		case ND_EVENT_WALL_SET_TMAP_NUM2: {
			short seg, cseg, tmap;
			ubyte nSide, cside;

			seg = NDReadShort ();
			nSide = NDReadByte ();
			cseg = NDReadShort ();
			cside = NDReadByte ();
			tmap = NDReadShort ();
			if ((gameData.demo.nVcrState != ND_STATE_PAUSED) &&
				 (gameData.demo.nVcrState != ND_STATE_REWINDING) &&
				 (gameData.demo.nVcrState != ND_STATE_ONEFRAMEBACKWARD)) {
				Assert (tmap!=0 && gameData.segs.segments [seg].sides [nSide].nOvlTex!=0);
				gameData.segs.segments [seg].sides [nSide].nOvlTex = 
				gameData.segs.segments [cseg].sides [cside].nOvlTex = tmap & 0x3fff;
				gameData.segs.segments [seg].sides [nSide].nOvlOrient = 
				gameData.segs.segments [cseg].sides [cside].nOvlOrient = (tmap >> 14) & 3;
				}
			}
			break;

		case ND_EVENT_MULTI_CLOAK: {
			sbyte pnum = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				gameData.multi.players [pnum].flags &= ~PLAYER_FLAGS_CLOAKED;
				gameData.multi.players [pnum].cloakTime = 0;
				gameData.demo.bPlayersCloaked &= ~ (1 << pnum);
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				gameData.multi.players [pnum].flags |= PLAYER_FLAGS_CLOAKED;
				gameData.multi.players [pnum].cloakTime = gameData.time.xGame  - (CLOAK_TIME_MAX / 2);
				gameData.demo.bPlayersCloaked |= (1 << pnum);
				}
			}
			break;

		case ND_EVENT_MULTI_DECLOAK: {
			sbyte pnum = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				gameData.multi.players [pnum].flags |= PLAYER_FLAGS_CLOAKED;
				gameData.multi.players [pnum].cloakTime = gameData.time.xGame  - (CLOAK_TIME_MAX / 2);
				gameData.demo.bPlayersCloaked |= (1 << pnum);
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				gameData.multi.players [pnum].flags &= ~PLAYER_FLAGS_CLOAKED;
				gameData.multi.players [pnum].cloakTime = 0;
				gameData.demo.bPlayersCloaked &= ~ (1 << pnum);
				}
			}
			break;

		case ND_EVENT_MULTI_DEATH: {
			sbyte pnum = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multi.players [pnum].netKilledTotal--;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multi.players [pnum].netKilledTotal++;
			}
			break;

#ifdef NETWORK
		case ND_EVENT_MULTI_KILL: {
			sbyte pnum = NDReadByte ();
			sbyte kill = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				gameData.multi.players [pnum].netKillsTotal -= kill;
				if (gameData.demo.nGameMode & GM_TEAM)
					multiData.kills.nTeam [GetTeam (pnum)] -= kill;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				gameData.multi.players [pnum].netKillsTotal += kill;
				if (gameData.demo.nGameMode & GM_TEAM)
					multiData.kills.nTeam [GetTeam (pnum)] += kill;
				}
			gameData.app.nGameMode = gameData.demo.nGameMode;
			MultiSortKillList ();
			gameData.app.nGameMode = GM_NORMAL;
			}
			break;

		case ND_EVENT_MULTI_CONNECT: {
			sbyte pnum, new_player;
			int killedTotal, killsTotal;
			char new_callsign [CALLSIGN_LEN+1], old_callsign [CALLSIGN_LEN+1];

			pnum = NDReadByte ();
			new_player = NDReadByte ();
			if (!new_player) {
				NDReadString (old_callsign);
				killedTotal = NDReadInt ();
				killsTotal = NDReadInt ();
				}
			NDReadString (new_callsign);
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				gameData.multi.players [pnum].connected = 0;
				if (!new_player) {
					memcpy (gameData.multi.players [pnum].callsign, old_callsign, CALLSIGN_LEN+1);
					gameData.multi.players [pnum].netKilledTotal = killedTotal;
					gameData.multi.players [pnum].netKillsTotal = killsTotal;
					}
				else
					gameData.multi.nPlayers--;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				gameData.multi.players [pnum].connected = 1;
				gameData.multi.players [pnum].netKillsTotal = 0;
				gameData.multi.players [pnum].netKilledTotal = 0;
				memcpy (gameData.multi.players [pnum].callsign, new_callsign, CALLSIGN_LEN+1);
				if (new_player)
					gameData.multi.nPlayers++;
				}
			}
			break;

		case ND_EVENT_MULTI_RECONNECT: {
			sbyte pnum = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multi.players [pnum].connected = 0;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multi.players [pnum].connected = 1;
			}
			break;

		case ND_EVENT_MULTI_DISCONNECT: {
			sbyte pnum = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multi.players [pnum].connected = 1;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multi.players [pnum].connected = 0;
			}
			break;

		case ND_EVENT_MULTI_SCORE: {
			sbyte pnum = NDReadByte ();
			int score = NDReadInt ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multi.players [pnum].score -= score;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multi.players [pnum].score += score;
			gameData.app.nGameMode = gameData.demo.nGameMode;
			MultiSortKillList ();
			gameData.app.nGameMode = GM_NORMAL;
			}
			break;

#endif // NETWORK
		case ND_EVENT_PLAYER_SCORE: {
			int score = NDReadInt ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multi.players [gameData.multi.nLocalPlayer].score -= score;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multi.players [gameData.multi.nLocalPlayer].score += score;
			}
			break;

		case ND_EVENT_PRIMARY_AMMO: {
			short old_ammo = NDReadShort ();
			short new_ammo = NDReadShort ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multi.players [gameData.multi.nLocalPlayer].primaryAmmo [gameData.weapons.nPrimary] = old_ammo;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multi.players [gameData.multi.nLocalPlayer].primaryAmmo [gameData.weapons.nPrimary] = new_ammo;
			}
			break;

		case ND_EVENT_SECONDARY_AMMO: {
			short old_ammo = NDReadShort ();
			short new_ammo = NDReadShort ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [gameData.weapons.nSecondary] = old_ammo;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [gameData.weapons.nSecondary] = new_ammo;
			}
			break;

		case ND_EVENT_DOOR_OPENING: {
			short nSegment = NDReadShort ();
			ubyte nSide = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				int anim_num;
				int cside;
				tSegment *segp, *csegp;

				segp = gameData.segs.segments + nSegment;
				csegp = gameData.segs.segments + segp->children [nSide];
				cside = FindConnectedSide (segp, csegp);
				anim_num = gameData.walls.walls [WallNumP (segp, nSide)].nClip;
				if (gameData.walls.pAnims [anim_num].flags & WCF_TMAP1)
					segp->sides [nSide].nBaseTex = csegp->sides [cside].nBaseTex =
						gameData.walls.pAnims [anim_num].frames [0];
				else
					segp->sides [nSide].nOvlTex = 
					csegp->sides [cside].nOvlTex = gameData.walls.pAnims [anim_num].frames [0];
				}
			else
				WallOpenDoor (gameData.segs.segments + nSegment, nSide);
			}
			break;

		case ND_EVENT_LASER_LEVEL: {
			sbyte oldLevel, newLevel;

			oldLevel = (sbyte) NDReadByte ();
			newLevel = (sbyte) NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel = oldLevel;
				UpdateLaserWeaponInfo ();
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel = newLevel;
				UpdateLaserWeaponInfo ();
				}
			}
			break;

		case ND_EVENT_CLOAKING_WALL: {
			ubyte back_wall_num, front_wall_num, nType, state, cloakValue;
			short l0, l1, l2, l3;
			tSegment *segp;
			int nSide;

			front_wall_num = NDReadByte ();
			back_wall_num = NDReadByte ();
			nType = NDReadByte ();
			state = NDReadByte ();
			cloakValue = NDReadByte ();
			l0 = NDReadShort ();
			l1 = NDReadShort ();
			l2 = NDReadShort ();
			l3 = NDReadShort ();
			gameData.walls.walls [front_wall_num].nType = nType;
			gameData.walls.walls [front_wall_num].state = state;
			gameData.walls.walls [front_wall_num].cloakValue = cloakValue;
			segp = gameData.segs.segments + gameData.walls.walls [front_wall_num].nSegment;
			nSide = gameData.walls.walls [front_wall_num].nSide;
			segp->sides [nSide].uvls [0].l = ((int) l0) << 8;
			segp->sides [nSide].uvls [1].l = ((int) l1) << 8;
			segp->sides [nSide].uvls [2].l = ((int) l2) << 8;
			segp->sides [nSide].uvls [3].l = ((int) l3) << 8;
			gameData.walls.walls [back_wall_num].nType = nType;
			gameData.walls.walls [back_wall_num].state = state;
			gameData.walls.walls [back_wall_num].cloakValue = cloakValue;
			segp = &gameData.segs.segments [gameData.walls.walls [back_wall_num].nSegment];
			nSide = gameData.walls.walls [back_wall_num].nSide;
			segp->sides [nSide].uvls [0].l = ((int) l0) << 8;
			segp->sides [nSide].uvls [1].l = ((int) l1) << 8;
			segp->sides [nSide].uvls [2].l = ((int) l2) << 8;
			segp->sides [nSide].uvls [3].l = ((int) l3) << 8;
			}
			break;

		case ND_EVENT_NEW_LEVEL: {
			sbyte newLevel, oldLevel, loadedLevel;

			newLevel = NDReadByte ();
			oldLevel = NDReadByte ();
			if (gameData.demo.nVcrState == ND_STATE_PAUSED)
				break;
			StopTime ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				loadedLevel = oldLevel;
			else {
				loadedLevel = newLevel;
				for (i = 0; i < MAX_PLAYERS; i++) {
					gameData.multi.players [i].cloakTime = 0;
					gameData.multi.players [i].flags &= ~PLAYER_FLAGS_CLOAKED;
					}
				}
			if ((loadedLevel < gameData.missions.nLastSecretLevel) || 
				 (loadedLevel > gameData.missions.nLastLevel)) {
				tMenuItem m [3];

				memset (m, 0, sizeof (m));
				m [0].nType = NM_TYPE_TEXT; m [0].text = TXT_CANT_PLAYBACK;
				m [1].nType = NM_TYPE_TEXT; m [1].text = TXT_LEVEL_CANT_LOAD;
				m [2].nType = NM_TYPE_TEXT; m [2].text = TXT_DEMO_OLD_CORRUPT;
				ExecMenu (NULL, NULL, sizeof (m)/sizeof (*m), m, NULL, NULL);
				return -1;
				}
			LoadLevel ((int)loadedLevel, 1);
			gameData.demo.bCtrlcenDestroyed = 0;
			if (bJustStartedPlayback) {
				gameData.walls.nWalls = NDReadInt ();
				for (i = 0; i < gameData.walls.nWalls; i++) {   // restore the walls
					gameData.walls.walls [i].nType = NDReadByte ();
					gameData.walls.walls [i].flags = NDReadByte ();
					gameData.walls.walls [i].state = NDReadByte ();
					seg = &gameData.segs.segments [gameData.walls.walls [i].nSegment];
					nSide = gameData.walls.walls [i].nSide;
					seg->sides [nSide].nBaseTex = NDReadShort ();
					nTexture = NDReadShort ();
					seg->sides [nSide].nOvlTex = nTexture & 0x3fff;
					seg->sides [nSide].nOvlOrient = (nTexture >> 14) & 3;
					}
#ifdef NETWORK
				if (gameData.demo.nGameMode & GM_CAPTURE)
					MultiApplyGoalTextures ();
#endif
				bJustStartedPlayback=0;
				}
			ResetPaletteAdd ();                // get palette back to normal
			StartTime ();
			}
			break;

		case ND_EVENT_EOF: {
			bDone=-1;
			CFSeek (infile, -1, SEEK_CUR);        // get back to the EOF marker
			gameData.demo.bEof = 1;
			gameData.demo.nFrameCount++;
			}
			break;

		default:
			Int3 ();
		}
	}
LastReadValue = c;
if (bNDBadRead) {
	tMenuItem m [2];

	memset (m, 0, sizeof (m));
	m [0].nType = NM_TYPE_TEXT; m [0].text = TXT_DEMO_ERR_READING;
	m [1].nType = NM_TYPE_TEXT; m [1].text = TXT_DEMO_OLD_CORRUPT;
	ExecMenu (NULL, NULL, sizeof (m)/sizeof (*m), m, NULL, NULL);
	}
else
	NDUpdateSmoke ();	
return bDone;
}

//	-----------------------------------------------------------------------------

void NDGotoBeginning ()
{
CFSeek (infile, 0, SEEK_SET);
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
if (NDReadDemoStart (0))
	NDStopPlayback ();
if (NDReadFrameInfo () == -1)
	NDStopPlayback ();
if (NDReadFrameInfo () == -1)
	NDStopPlayback ();
gameData.demo.nVcrState = ND_STATE_PAUSED;
gameData.demo.bEof = 0;
}

//	-----------------------------------------------------------------------------

void NDGotoEnd ()
{
	short frame_length, byte_count, bshort;
	sbyte level, bbyte, laserLevel;
	ubyte energy, shield, c;
	int i, loc, bint;

CFSeek (infile, -2, SEEK_END);
level = NDReadByte ();
if ((level < gameData.missions.nLastSecretLevel) || (level > gameData.missions.nLastLevel)) {
	tMenuItem m [3];

	memset (m, 0, sizeof (m));
	m [0].nType = NM_TYPE_TEXT; m [0].text = TXT_CANT_PLAYBACK;
	m [1].nType = NM_TYPE_TEXT; m [1].text = TXT_LEVEL_CANT_LOAD;
	m [2].nType = NM_TYPE_TEXT; m [2].text = TXT_DEMO_OLD_CORRUPT;
	ExecMenu (NULL, NULL, sizeof (m)/sizeof (*m), m, NULL, NULL);
	NDStopPlayback ();
	return;
	}
if (level != gameData.missions.nCurrentLevel)
	LoadLevel (level, 1);
CFSeek (infile, -4, SEEK_END);
byte_count = NDReadShort ();
CFSeek (infile, -2 - byte_count, SEEK_CUR);

frame_length = NDReadShort ();
loc = CFTell (infile);
if (gameData.demo.nGameMode & GM_MULTI)
	gameData.demo.bPlayersCloaked = NDReadByte ();
else
	bbyte = NDReadByte ();
bbyte = NDReadByte ();
bshort = NDReadShort ();
bint = NDReadInt ();
energy = NDReadByte ();
shield = NDReadByte ();
gameData.multi.players [gameData.multi.nLocalPlayer].energy = i2f (energy);
gameData.multi.players [gameData.multi.nLocalPlayer].shields = i2f (shield);
gameData.multi.players [gameData.multi.nLocalPlayer].flags = NDReadInt ();
if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
	gameData.multi.players [gameData.multi.nLocalPlayer].cloakTime = gameData.time.xGame - (CLOAK_TIME_MAX / 2);
	gameData.demo.bPlayersCloaked |= (1 << gameData.multi.nLocalPlayer);
	}
if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE)
	gameData.multi.players [gameData.multi.nLocalPlayer].invulnerableTime = gameData.time.xGame - (INVULNERABLE_TIME_MAX / 2);
gameData.weapons.nPrimary = NDReadByte ();
gameData.weapons.nSecondary = NDReadByte ();
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	gameData.multi.players [gameData.multi.nLocalPlayer].primaryAmmo [i] = NDReadShort ();
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [i] = NDReadShort ();
laserLevel = NDReadByte ();
if (laserLevel != gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel) {
	gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel = laserLevel;
	UpdateLaserWeaponInfo ();
	}
if (gameData.demo.nGameMode & GM_MULTI) {
	c = NDReadByte ();
	gameData.multi.nPlayers = (int)c;
	// see newdemo_read_start_demo for explanation of
	// why this is commented out
	//		NDReadByte ((sbyte *)&gameData.multi.nPlayers);
	for (i = 0; i < gameData.multi.nPlayers; i++) {
		NDReadString (gameData.multi.players [i].callsign);
		gameData.multi.players [i].connected = NDReadByte ();
		if (gameData.demo.nGameMode & GM_MULTI_COOP)
			gameData.multi.players [i].score = NDReadInt ();
		else {
			gameData.multi.players [i].netKilledTotal = NDReadShort ();
			gameData.multi.players [i].netKillsTotal = NDReadShort ();
			}
		}
	}
else
	gameData.multi.players [gameData.multi.nLocalPlayer].score = NDReadInt ();
CFSeek (infile, loc, SEEK_SET);
CFSeek (infile, -frame_length, SEEK_CUR);
gameData.demo.nFrameCount = NDReadInt ();            // get the frame count
gameData.demo.nFrameCount--;
CFSeek (infile, 4, SEEK_CUR);
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
NDReadFrameInfo ();           // then the frame information
gameData.demo.nVcrState = ND_STATE_PAUSED;
return;
}

//	-----------------------------------------------------------------------------

void NDBackFrames (int frames)
{
	short last_frame_length;
	int i;

for (i = 0; i < frames; i++) {
	CFSeek (infile, -10, SEEK_CUR);
	last_frame_length = NDReadShort ();
	CFSeek (infile, 8 - last_frame_length, SEEK_CUR);
	if (!gameData.demo.bEof && NDReadFrameInfo () == -1) {
		NDStopPlayback ();
		return;
		}
	if (gameData.demo.bEof)
		gameData.demo.bEof = 0;
	CFSeek (infile, -10, SEEK_CUR);
	last_frame_length = NDReadShort ();
	CFSeek (infile, 8 - last_frame_length, SEEK_CUR);
	}
}

//	-----------------------------------------------------------------------------

/*
 *  routine to interpolate the viewer position.  the current position is
 *  stored in the gameData.objs.viewer tObject.  Save this position, and read the next
 *  frame to get all gameData.objs.objects read in.  Calculate the delta playback and
 *  the delta recording frame times between the two frames, then intepolate
 *  the viewers position accordingly.  gameData.demo.xRecordedTime is the time that it
 *  took the recording to render the frame that we are currently looking
 *  at.
*/

void NDInterpolateFrame (fix d_play, fix d_recorded)
{
	int			nCurObjs;
	fix			factor;
	vmsVector  fvec1, fvec2, rvec1, rvec2;
	fix         mag1;
	fix			delta_x, delta_y, delta_z;
	ubyte			renderType;
	tObject		*curObjP, *objP, *i, *j;

static tObject curObjs [MAX_OBJECTS];
factor = FixDiv (d_play, d_recorded);
if (factor > F1_0)
	factor = F1_0;
nCurObjs = gameData.objs.nLastObject;
#if 1
memcpy (curObjs, gameData.objs.objects, sizeof (tObject) * (nCurObjs + 1));
#else
for (i = 0; i <= nCurObjs; i++)
	memcpy (&(curObjs [i]), &(gameData.objs.objects [i]), sizeof (tObject));
#endif
gameData.demo.nVcrState = ND_STATE_PAUSED;
if (NDReadFrameInfo () == -1) {
	NDStopPlayback ();
	return;
	}
for (i = curObjs + nCurObjs, curObjP = curObjs; curObjP < i; curObjP++) {
	for (j = gameData.objs.objects + gameData.objs.nLastObject, objP = gameData.objs.objects; objP < j; objP++) {
		if (curObjP->nSignature == objP->nSignature) {
			renderType = curObjP->renderType;
			//fix delta_p, delta_h, delta_b;
			//vmsAngVec cur_angles, dest_angles;
			//  Extract the angles from the tObject orientation matrix.
			//  Some of this code taken from AITurnTowardsVector
			//  Don't do the interpolation on certain render types which don't use an orientation matrix
			if ((renderType != RT_LASER) &&
				 (renderType != RT_FIREBALL) && 
					(renderType != RT_THRUSTER) && 
					(renderType != RT_POWERUP)) {

				fvec1 = curObjP->orient.fVec;
				VmVecScale (&fvec1, F1_0-factor);
				fvec2 = objP->orient.fVec;
				VmVecScale (&fvec2, factor);
				VmVecInc (&fvec1, &fvec2);
				mag1 = VmVecNormalizeQuick (&fvec1);
				if (mag1 > F1_0/256) {
					rvec1 = curObjP->orient.rVec;
					VmVecScale (&rvec1, F1_0-factor);
					rvec2 = objP->orient.rVec;
					VmVecScale (&rvec2, factor);
					VmVecInc (&rvec1, &rvec2);
					VmVecNormalizeQuick (&rvec1); // Note: Doesn't matter if this is null, if null, VmVector2Matrix will just use fvec1
					VmVector2Matrix (&curObjP->orient, &fvec1, NULL, &rvec1);
					}
				}
			// Interpolate the tObject position.  This is just straight linear
			// interpolation.
			delta_x = objP->pos.x - curObjP->pos.x;
			delta_y = objP->pos.y - curObjP->pos.y;
			delta_z = objP->pos.z - curObjP->pos.z;
			delta_x = FixMul (delta_x, factor);
			delta_y = FixMul (delta_y, factor);
			delta_z = FixMul (delta_z, factor);
			curObjP->pos.x += delta_x;
			curObjP->pos.y += delta_y;
			curObjP->pos.z += delta_z;
				// -- old fashioned way --// stuff the new angles back into the tObject structure
				// -- old fashioned way --				VmAngles2Matrix (&(curObjs [i].orient), &cur_angles);
			}
		}
	}

// get back to original position in the demo file.  Reread the current
// frame information again to reset all of the tObject stuff not covered
// with gameData.objs.nLastObject and the tObject array (previously rendered
// gameData.objs.objects, etc....)
NDBackFrames (1);
NDBackFrames (1);
if (NDReadFrameInfo () == -1)
	NDStopPlayback ();
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
#if 1
memcpy (gameData.objs.objects, curObjs, sizeof (tObject) * (nCurObjs + 1));
#else
for (i = 0; i <= nCurObjs; i++)
	memcpy (&(gameData.objs.objects [i]), &(curObjs [i]), sizeof (tObject));
#endif
gameData.objs.nLastObject = nCurObjs;
}

//	-----------------------------------------------------------------------------

void NDPlayBackOneFrame ()
{
	int frames_back, i, level;
	static fix base_interpolTime = 0;
	static fix d_recorded = 0;

for (i = 0; i < MAX_PLAYERS; i++)
	if (gameData.demo.bPlayersCloaked &(1 << i))
		gameData.multi.players [i].cloakTime = gameData.time.xGame - (CLOAK_TIME_MAX / 2);
if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE)
	gameData.multi.players [gameData.multi.nLocalPlayer].invulnerableTime = gameData.time.xGame - (INVULNERABLE_TIME_MAX / 2);
if (gameData.demo.nVcrState == ND_STATE_PAUSED)       // render a frame or not
	return;
if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
	DoJasonInterpolate (gameData.demo.xRecordedTime);
gameData.reactor.bDestroyed = 0;
gameData.reactor.countdown.nSecsLeft = -1;
PALETTE_FLASH_SET (0, 0, 0);       //clear flash
if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
		(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
	level = gameData.missions.nCurrentLevel;
	if (gameData.demo.nFrameCount == 0)
		return;
	else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) &&(gameData.demo.nFrameCount < 10)) {
		NDGotoBeginning ();
		return;
		}
	if (gameData.demo.nVcrState == ND_STATE_REWINDING)
		frames_back = 10;
	else
		frames_back = 1;
	if (gameData.demo.bEof) {
		CFSeek (infile, 11, SEEK_CUR);
		}
	NDBackFrames (frames_back);
	if (level != gameData.missions.nCurrentLevel)
		NDPopCtrlCenTriggers ();
	if (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD) {
		if (level != gameData.missions.nCurrentLevel)
			NDBackFrames (1);
		gameData.demo.nVcrState = ND_STATE_PAUSED;
		}
	}
else if (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) {
	if (!gameData.demo.bEof) {
		for (i = 0; i < 10; i++) {
			if (NDReadFrameInfo () == -1) {
				if (gameData.demo.bEof)
					gameData.demo.nVcrState = ND_STATE_PAUSED;
				else
					NDStopPlayback ();
				break;
				}
			}
		}
	else
		gameData.demo.nVcrState = ND_STATE_PAUSED;
	}
else if (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD) {
	if (!gameData.demo.bEof) {
		level = gameData.missions.nCurrentLevel;
		if (NDReadFrameInfo () == -1) {
			if (!gameData.demo.bEof)
				NDStopPlayback ();
			}
		if (level != gameData.missions.nCurrentLevel) {
			if (NDReadFrameInfo () == -1) {
				if (!gameData.demo.bEof)
					NDStopPlayback ();
				}
			}
		gameData.demo.nVcrState = ND_STATE_PAUSED;
		} 
	else
		gameData.demo.nVcrState = ND_STATE_PAUSED;
	}
else {
	//  First, uptate the total playback time to date.  Then we check to see
	//  if we need to change the playback style to interpolate frames or
	//  skip frames based on where the playback time is relative to the
	//  recorded time.
	if (gameData.demo.nFrameCount <= 0)
		gameData.demo.xPlaybackTotal = gameData.demo.xRecordedTotal;      // baseline total playback time
	else
		gameData.demo.xPlaybackTotal += gameData.time.xFrame;
	if ((gameData.demo.nPlaybackStyle == NORMAL_PLAYBACK) &&(gameData.demo.nFrameCount > 10))
		if ((gameData.demo.xPlaybackTotal * INTERPOL_FACTOR) < gameData.demo.xRecordedTotal) {
			gameData.demo.nPlaybackStyle = INTERPOLATE_PLAYBACK;
			gameData.demo.xPlaybackTotal = gameData.demo.xRecordedTotal + gameData.time.xFrame;  // baseline playback time
			base_interpolTime = gameData.demo.xRecordedTotal;
			d_recorded = gameData.demo.xRecordedTime;                      // baseline delta recorded
			}
	if ((gameData.demo.nPlaybackStyle == NORMAL_PLAYBACK) &&(gameData.demo.nFrameCount > 10))
		if (gameData.demo.xPlaybackTotal > gameData.demo.xRecordedTotal)
			gameData.demo.nPlaybackStyle = SKIP_PLAYBACK;
	if ((gameData.demo.nPlaybackStyle == INTERPOLATE_PLAYBACK) && gameData.demo.bInterpolate) {
		fix d_play = 0;

		if (gameData.demo.xRecordedTotal - gameData.demo.xPlaybackTotal < gameData.time.xFrame) {
			d_recorded = gameData.demo.xRecordedTotal - gameData.demo.xPlaybackTotal;
			while (gameData.demo.xRecordedTotal - gameData.demo.xPlaybackTotal < gameData.time.xFrame) {
				tObject *curObjs, *objP;
				int i, j, nObjects, nLevel, nSig;

				nObjects = gameData.objs.nLastObject;
				curObjs = (tObject *) d_malloc (sizeof (tObject) * (nObjects + 1));
				if (!
					curObjs) {
					Warning (TXT_INTERPOLATE_BOTS, sizeof (tObject) * nObjects);
					break;
					}
				for (i = 0; i <= nObjects; i++)
					memcpy (curObjs, gameData.objs.objects, (nObjects + 1) * sizeof (tObject));
				nLevel = gameData.missions.nCurrentLevel;
				if (NDReadFrameInfo () == -1) {
					d_free (curObjs);
					NDStopPlayback ();
					return;
					}
				if (nLevel != gameData.missions.nCurrentLevel) {
					d_free (curObjs);
					if (NDReadFrameInfo () == -1)
						NDStopPlayback ();
					break;
					}
				//  for each new tObject in the frame just read in, determine if there is
				//  a corresponding tObject that we have been interpolating.  If so, then
				//  copy that interpolated tObject to the new gameData.objs.objects array so that the
				//  interpolated position and orientation can be preserved.
				for (i = 0; i <= nObjects; i++) {
					nSig = curObjs [i].nSignature;
					objP = gameData.objs.objects;
					for (j = 0; j <= gameData.objs.nLastObject; j++, objP++) {
						if (nSig == objP->nSignature) {
							objP->orient = curObjs [i].orient;
							objP->pos = curObjs [i].pos;
							break;
							}
						}
					}
				d_free (curObjs);
				d_recorded += gameData.demo.xRecordedTime;
				base_interpolTime = gameData.demo.xPlaybackTotal - gameData.time.xFrame;
				}
			}
		d_play = gameData.demo.xPlaybackTotal - base_interpolTime;
		NDInterpolateFrame (d_play, d_recorded);
		return;
		}
	else {
		if (NDReadFrameInfo () == -1) {
			NDStopPlayback ();
			return;
			}
		if (gameData.demo.nPlaybackStyle == SKIP_PLAYBACK) {
			while (gameData.demo.xPlaybackTotal > gameData.demo.xRecordedTotal) {
				if (NDReadFrameInfo () == -1) {
					NDStopPlayback ();
					return;
					}
				}
			}
		}
	}
}

//	-----------------------------------------------------------------------------

void NDStartRecording ()
{
#ifdef WINDOWS
gameData.demo.nSize=GetFreeDiskSpace ();
#else
gameData.demo.nSize = GetDiskFree ();
#endif
gameData.demo.nSize -= 100000;
if ((gameData.demo.nSize+100000) <  2000000000) {
	if (( (int) (gameData.demo.nSize)) < 500000) {
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_DEMO_NO_SPACE);
		return;
		}
	}
InitDemoData ();
gameData.demo.nWritten = 0;
gameData.demo.bNoSpace=0;
gameData.demo.nState = ND_STATE_RECORDING;
outfile = CFOpen (DEMO_FILENAME, gameFolders.szDemoDir, "wb", 0);
#ifndef _WIN32_WCE
if (!outfile && errno == ENOENT) {   //dir doesn't exist?
#else
if (!outfile) {                      //dir doesn't exist and no errno on mac!
#endif
	CFMkDir (gameFolders.szDemoDir); //try making directory
	outfile = CFOpen (DEMO_FILENAME, gameFolders.szDemoDir, "wb", 0);
	}
if (!outfile) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, "Cannot open demo temp file");
	gameData.demo.nState = ND_STATE_NORMAL;
	}
else
	NDRecordStartDemo ();
}

//	-----------------------------------------------------------------------------

char demoname_allowed_chars [] = "azAZ09__--";
void NDStopRecording ()
{
	tMenuItem m [6];
	int l, exit;
	static char filename [15] = "", *s;
	static ubyte tmpcnt = 0;
	ubyte cloaked = 0;
	char fullname [15+FILENAME_LEN] = "";
	unsigned short byte_count = 0;

	exit = 0;

NDWriteByte (ND_EVENT_EOF);
NDWriteShort ((short) (gameData.demo.nFrameBytesWritten - 1));
if (gameData.app.nGameMode & GM_MULTI) {
	for (l = 0; l < gameData.multi.nPlayers; l++) {
		if (gameData.multi.players [l].flags & PLAYER_FLAGS_CLOAKED)
			cloaked |= (1 << l);
		}
	NDWriteByte (cloaked);
	NDWriteByte (ND_EVENT_EOF);
	}
else {
	NDWriteShort (ND_EVENT_EOF);
	}
NDWriteShort (ND_EVENT_EOF);
NDWriteInt (ND_EVENT_EOF);
byte_count += 10;       // from gameData.demo.nFrameBytesWritten
NDWriteByte ((sbyte) (f2ir (gameData.multi.players [gameData.multi.nLocalPlayer].energy)));
NDWriteByte ((sbyte) (f2ir (gameData.multi.players [gameData.multi.nLocalPlayer].shields)));
NDWriteInt (gameData.multi.players [gameData.multi.nLocalPlayer].flags);        // be sure players flags are set
NDWriteByte ((sbyte)gameData.weapons.nPrimary);
NDWriteByte ((sbyte)gameData.weapons.nSecondary);
byte_count += 8;
for (l = 0; l < MAX_PRIMARY_WEAPONS; l++)
	NDWriteShort ((short)gameData.multi.players [gameData.multi.nLocalPlayer].primaryAmmo [l]);
for (l = 0; l < MAX_SECONDARY_WEAPONS; l++)
	NDWriteShort ((short)gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [l]);
byte_count += (sizeof (short) * (MAX_PRIMARY_WEAPONS + MAX_SECONDARY_WEAPONS));
NDWriteByte (gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel);
byte_count++;
if (gameData.app.nGameMode & GM_MULTI) {
	NDWriteByte ((sbyte)gameData.multi.nPlayers);
	byte_count++;
	for (l = 0; l < gameData.multi.nPlayers; l++) {
		NDWriteString (gameData.multi.players [l].callsign);
		byte_count += ((int) strlen (gameData.multi.players [l].callsign) + 2);
		NDWriteByte ((sbyte) gameData.multi.players [l].connected);
		if (gameData.app.nGameMode & GM_MULTI_COOP) {
			NDWriteInt (gameData.multi.players [l].score);
			byte_count += 5;
			}
		else {
			NDWriteShort ((short)gameData.multi.players [l].netKilledTotal);
			NDWriteShort ((short)gameData.multi.players [l].netKillsTotal);
			byte_count += 5;
			}
		}
	} 
else {
	NDWriteInt (gameData.multi.players [gameData.multi.nLocalPlayer].score);
	byte_count += 4;
	}
NDWriteShort (byte_count);
NDWriteByte ((sbyte) gameData.missions.nCurrentLevel);
NDWriteByte (ND_EVENT_EOF);
l = CFTell (outfile);
CFClose (outfile);
outfile = NULL;
gameData.demo.nState = ND_STATE_NORMAL;
GrPaletteStepLoad (NULL);
if (filename [0] != '\0') {
	int num, i = (int) strlen (filename) - 1;
	char newfile [15];

	while (isdigit (filename [i])) {
		i--;
		if (i == -1)
			break;
		}
	i++;
	num = atoi (&(filename [i]));
	num++;
	filename [i] = '\0';
	sprintf (newfile, "%s%d", filename, num);
	strncpy (filename, newfile, 8);
	filename [8] = '\0';
	}

try_again:
	;

nmAllowedChars = demoname_allowed_chars;
memset (m, 0, sizeof (m));
if (!gameData.demo.bNoSpace) {
	m [0].nType = NM_TYPE_INPUT; 
	m [0].text_len = 8; 
	m [0].text = filename;
	exit = ExecMenu (NULL, TXT_SAVE_DEMO_AS, 1, &(m [0]), NULL, NULL);
	}
else if (gameData.demo.bNoSpace == 1) {
	m [0].nType = NM_TYPE_TEXT; 
	m [0].text = TXT_DEMO_SAVE_BAD;
	m [1].nType = NM_TYPE_INPUT;
	m [1].text_len = 8; 
	m [1].text = filename;
	exit = ExecMenu (NULL, NULL, 2, m, NULL, NULL);
	} 
else if (gameData.demo.bNoSpace == 2) {
	m [0].nType = NM_TYPE_TEXT; 
	m [0].text = TXT_DEMO_SAVE_NOSPACE;
	m [1].nType = NM_TYPE_INPUT;
	m [1].text_len = 8; 
	m [1].text = filename;
	exit = ExecMenu (NULL, NULL, 2, m, NULL, NULL);
	}
nmAllowedChars = NULL;
if (exit == -2) {                   // got bumped out from network menu
	char save_file [7+FILENAME_LEN];

	if (filename [0] != '\0') {
		strcpy (save_file, filename);
		strcat (save_file, ".dem");
	} else
		sprintf (save_file, "tmp%d.dem", tmpcnt++);
	CFDelete (save_file, gameFolders.szDemoDir);
	CFRename (DEMO_FILENAME, save_file, gameFolders.szDemoDir);
	return;
	}
if (exit == -1) {               // pressed ESC
	CFDelete (DEMO_FILENAME, gameFolders.szDemoDir);      // might as well remove the file
	return;                     // return without doing anything
	}
if (filename [0]==0) //null string
	goto try_again;
//check to make sure name is ok
for (s=filename;*s;s++)
	if (!isalnum (*s) && *s!='_') {
		ExecMessageBox (NULL, NULL, 1, TXT_CONTINUE, TXT_DEMO_USE_LETTERS);
		goto try_again;
		}
if (gameData.demo.bNoSpace)
	strcpy (fullname, m [1].text);
else
	strcpy (fullname, m [0].text);
strcat (fullname, ".dem");
CFDelete (fullname, gameFolders.szDemoDir);
CFRename (DEMO_FILENAME, fullname, gameFolders.szDemoDir);
}

//	-----------------------------------------------------------------------------
//returns the number of demo files on the disk
int NDCountDemos ()
{
	FFS	ffs;
	int 	nFiles=0;
	char	searchName [FILENAME_LEN];

sprintf (searchName, "%s%s*.dem", gameFolders.szDemoDir, *gameFolders.szDemoDir ? "/" : "");
if (!FFF (searchName, &ffs, 0)) {
	do {
		nFiles++;
		} while (!FFN (&ffs, 0));
	FFC (&ffs);
	}
if (gameFolders.bAltHogDirInited) {
	sprintf (searchName, "%s/%s%s*.dem", gameFolders.szAltHogDir, gameFolders.szDemoDir, gameFolders.szDemoDir ? "/" : "");
	if (!FFF (searchName, &ffs, 0)) {
		do {
			nFiles++;
			} while (!FFN (&ffs, 0));
		FFC (&ffs);
		}
	}
return nFiles;
}

//	-----------------------------------------------------------------------------

void NDStartPlayback (char * filename)
{
	FFS ffs;
	int rnd_demo = 0;
	char filename2 [PATH_MAX+FILENAME_LEN], searchName [FILENAME_LEN];

#ifdef NETWORK
ChangePlayerNumTo (0);
#endif
*filename2 = '\0';
InitDemoData ();
gameData.demo.bFirstTimePlayback = 1;
gameData.demo.xJasonPlaybackTotal = 0;
if (!filename) {
	// Randomly pick a filename
	int nFiles = 0, nRandFiles;
	rnd_demo = 1;
	nFiles = NDCountDemos ();
	if (nFiles == 0) {
		return;     // No files found!
		}
	nRandFiles = d_rand () % nFiles;
	nFiles = 0;
	sprintf (searchName, "%s%s*.dem", gameFolders.szDemoDir, *gameFolders.szDemoDir ? "/" : "");
	if (!FFF (searchName, &ffs, 0)) {
		do {
			if (nFiles==nRandFiles) {
				filename = (char *)&ffs.name;
				break;
				}
			nFiles++;
			} while (!FFN (&ffs, 0));
		FFC (&ffs);
		}
	if (!filename && gameFolders.bAltHogDirInited) {
		sprintf (searchName, "%s/%s%s*.dem", gameFolders.szAltHogDir, gameFolders.szDemoDir, gameFolders.szDemoDir ? "/" : "");
		if (!FFF (searchName, &ffs, 0)) {
			do {
				if (nFiles==nRandFiles) {
					filename = (char *)&ffs.name;
					break;
					}
				nFiles++;
				} while (!FFN (&ffs, 0));
			FFC (&ffs);
			}
		}
		if (!filename) 
			return;
	}
if (!filename)
	return;
strcpy (filename2, filename);
infile = CFOpen (filename2, gameFolders.szDemoDir, "rb", 0);
if (infile==NULL) {
#if TRACE				
	con_printf (CON_DEBUG, "Error reading '%s'\n", filename);
#endif
	return;
	}
bNDBadRead = 0;
#ifdef NETWORK
ChangePlayerNumTo (0);                 // force playernum to 0
#endif
strncpy (gameData.demo.callSignSave, gameData.multi.players [gameData.multi.nLocalPlayer].callsign, CALLSIGN_LEN);
gameData.objs.viewer = gameData.objs.console = gameData.objs.objects;   // play properly as if console player
if (NDReadDemoStart (rnd_demo)) {
	CFClose (infile);
	return;
	}
gameData.app.nGameMode = GM_NORMAL;
gameData.demo.nState = ND_STATE_PLAYBACK;
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
gameData.demo.nOldCockpit = gameStates.render.cockpit.nMode;
gameData.demo.nSize = CFLength (infile, 0);
bNDBadRead = 0;
gameData.demo.bEof = 0;
gameData.demo.nFrameCount = 0;
gameData.demo.bPlayersCloaked = 0;
gameData.demo.nPlaybackStyle = NORMAL_PLAYBACK;
SetFunctionMode (FMODE_GAME);
Cockpit_3d_view [0] = CV_NONE;       //turn off 3d views on cockpit
Cockpit_3d_view [1] = CV_NONE;       //turn off 3d views on cockpit
NDPlayBackOneFrame ();       // this one loads new level
NDPlayBackOneFrame ();       // get all of the gameData.objs.objects to renderb game
}

//	-----------------------------------------------------------------------------

void NDStopPlayback ()
{
CFClose (infile);
gameData.demo.nState = ND_STATE_NORMAL;
#ifdef NETWORK
ChangePlayerNumTo (0);             //this is reality
#endif
strncpy (gameData.multi.players [gameData.multi.nLocalPlayer].callsign, gameData.demo.callSignSave, CALLSIGN_LEN);
gameStates.render.cockpit.nMode = gameData.demo.nOldCockpit;
gameData.app.nGameMode = GM_GAME_OVER;
SetFunctionMode (FMODE_MENU);
longjmp (gameExitPoint, 0);               // Exit game loop
}

//	-----------------------------------------------------------------------------

#ifndef NDEBUG

#define BUF_SIZE 16384

void NewDemoStripFrames (char *outname, int bytes_to_strip)
{
	CFILE *outfile;
	char *buf;
	int nTotalSize, bytes_done, read_elems, bytes_back;
	int trailer_start, loc1, loc2, stop_loc, bytes_to_read;
	short last_frame_length;

bytes_done = 0;
nTotalSize = CFLength (infile, 0);
outfile = CFOpen (outname, "", "wb", 0);
if (!outfile) {
	tMenuItem m [1];

	memset (m, 0, sizeof (m));
	m [0].nType = NM_TYPE_TEXT; m [0].text = "Can't open output file";
	ExecMenu (NULL, NULL, 1, m, NULL, NULL);
	NDStopPlayback ();
	return;
	}
buf = d_malloc (BUF_SIZE);
if (buf == NULL) {
	tMenuItem m [1];

	m [0].nType = NM_TYPE_TEXT; m [0].text = "Can't d_malloc output buffer";
	ExecMenu (NULL, NULL, 1, m, NULL, NULL);
	CFClose (outfile);
	NDStopPlayback ();
	return;
	}
NDGotoEnd ();
trailer_start = CFTell (infile);
CFSeek (infile, 11, SEEK_CUR);
bytes_back = 0;
while (bytes_back < bytes_to_strip) {
	loc1 = CFTell (infile);
	//CFSeek (infile, -10, SEEK_CUR);
	//NDReadShort (&last_frame_length);
	//CFSeek (infile, 8 - last_frame_length, SEEK_CUR);
	NDBackFrames (1);
	loc2 = CFTell (infile);
	bytes_back += (loc1 - loc2);
	}
CFSeek (infile, -10, SEEK_CUR);
last_frame_length = NDReadShort ();
CFSeek (infile, -3, SEEK_CUR);
stop_loc = CFTell (infile);
CFSeek (infile, 0, SEEK_SET);
while (stop_loc > 0) {
	if (stop_loc < BUF_SIZE)
		bytes_to_read = stop_loc;
	else
		bytes_to_read = BUF_SIZE;
	read_elems = CFRead (buf, 1, bytes_to_read, infile);
	CFWrite (buf, 1, read_elems, outfile);
	stop_loc -= read_elems;
	}
stop_loc = CFTell (outfile);
CFSeek (infile, trailer_start, SEEK_SET);
while ((read_elems = CFRead (buf, 1, BUF_SIZE, infile)) != 0)
	CFWrite (buf, 1, read_elems, outfile);
CFSeek (outfile, stop_loc, SEEK_SET);
CFSeek (outfile, 1, SEEK_CUR);
CFWrite (&last_frame_length, 2, 1, outfile);
CFClose (outfile);
NDStopPlayback ();
DestroyAllSmoke ();
}

#endif

//	-----------------------------------------------------------------------------

tObject DemoRightExtra, DemoLeftExtra;
ubyte DemoDoRight=0, DemoDoLeft=0;

void NDRenderExtras (ubyte which, tObject *objP)
{
	ubyte w=which>>4;
	ubyte nType=which&15;

if (which==255) {
	Int3 (); // how'd we get here?
	DoCockpitWindowView (w, NULL, 0, WBU_WEAPON, NULL);
	return;
	}
if (w) {
	memcpy (&DemoRightExtra, objP, sizeof (tObject));  
	DemoDoRight=nType;
	}
else {
	memcpy (&DemoLeftExtra, objP, sizeof (tObject)); 
	DemoDoLeft=nType;
	}
}

//	-----------------------------------------------------------------------------

void DoJasonInterpolate (fix xRecordedTime)
{
	fix xDelay;

gameData.demo.xJasonPlaybackTotal += gameData.time.xFrame;
if (!gameData.demo.bFirstTimePlayback) {
	// get the difference between the recorded time and the playback time
	xDelay = (xRecordedTime - gameData.time.xFrame);
	if (xDelay >= f0_0) {
		StopTime ();
		timer_delay (xDelay);
		StartTime ();
		}
	else {
		while (gameData.demo.xJasonPlaybackTotal > gameData.demo.xRecordedTotal)
			if (NDReadFrameInfo () == -1) {
				NDStopPlayback ();
				return;
				}
		//xDelay = gameData.demo.xRecordedTotal - gameData.demo.xJasonPlaybackTotal;
		//if (xDelay > f0_0)
		//	timer_delay (xDelay);
		}
	}
gameData.demo.bFirstTimePlayback=0;
}

//	-----------------------------------------------------------------------------
//eof
