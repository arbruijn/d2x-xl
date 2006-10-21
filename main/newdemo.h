/* $Id: newdemo.h,v 1.4 2003/10/04 02:58:23 btb Exp $ */
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
 * header for demo playback system
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:00:24  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:27:18  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.43  1995/01/19  09:41:43  allender
 * prototype for laser level recording
 *
 * Revision 1.42  1995/01/18  18:48:49  allender
 * added function prototype for door_open
 *
 * Revision 1.41  1995/01/17  17:42:31  allender
 * new prototypes for ammo counts
 *
 * Revision 1.40  1995/01/04  15:04:27  allender
 * added some different prototypes for registered
 *
 * Revision 1.39  1995/01/03  11:45:11  allender
 * extern function definition
 *
 * Revision 1.38  1994/12/29  16:43:31  allender
 * new function prototype
 *
 * Revision 1.37  1994/12/28  14:15:27  allender
 * new function prototypes
 *
 * Revision 1.36  1994/12/21  12:46:41  allender
 * new functions for multiplayer deaths and kills
 *
 * Revision 1.35  1994/12/12  11:32:55  allender
 * added new record function to restore after in rearview mode
 *
 * Revision 1.34  1994/12/08  21:03:15  allender
 * added new param to record_playerFlags
 *
 * Revision 1.33  1994/12/08  13:47:01  allender
 * removed function call to record_rearview
 *
 * Revision 1.32  1994/12/06  12:57:10  allender
 * added new prototype for multi decloaking
 *
 * Revision 1.31  1994/12/01  11:46:34  allender
 * added recording prototype for multi player cloak
 *
 * Revision 1.30  1994/11/27  23:04:22  allender
 * function prototype for recording new levels
 *
 * Revision 1.29  1994/11/07  08:47:43  john
 * Made wall state record.
 *
 * Revision 1.28  1994/11/05  17:22:53  john
 * Fixed lots of sequencing problems with newdemo stuff.
 *
 * Revision 1.27  1994/11/04  16:48:49  allender
 * extern gameData.demo.bInterpolate variable
 *
 * Revision 1.26  1994/11/02  14:08:53  allender
 * record rearview
 *
 * Revision 1.25  1994/10/31  13:35:04  allender
 * added two record functions to save and restore cockpit state on
 * death sequence
 *
 * Revision 1.24  1994/10/29  16:01:11  allender
 * added ND_STATE_NODEMOS to indicate that there are no demos currently
 * available for playback
 *
 * Revision 1.23  1994/10/28  12:41:58  allender
 * add homing distance recording event
 *
 * Revision 1.22  1994/10/27  16:57:32  allender
 * removed VCR_MODE stuff, and added monitor blowup effects
 *
 * Revision 1.21  1994/10/26  14:44:48  allender
 * completed hacked in vcr nType demo playback states
 *
 * Revision 1.20  1994/10/26  13:40:38  allender
 * more vcr demo playback defines
 *
 * Revision 1.19  1994/10/26  08:51:26  allender
 * record player weapon change
 *
 * Revision 1.18  1994/10/25  16:25:31  allender
 * prototypes for shield, energy and flags
 *
 * Revision 1.17  1994/08/15  18:05:30  john
 * *** empty log message ***
 *
 * Revision 1.16  1994/07/21  13:11:26  matt
 * Ripped out remants of old demo system, and added demo only system that
 * disables tObject movement and game options from menu.
 *
 * Revision 1.15  1994/07/05  12:49:02  john
 * Put functionality of New Hostage spec into code.
 *
 * Revision 1.14  1994/06/27  15:53:12  john
 * #define'd out the newdemo stuff
 *
 *
 * Revision 1.13  1994/06/24  17:01:25  john
 * Add VFX support; Took Game Sequencing, like EndGame and stuff and
 * took it out of game.c and into gameseq.c
 *
 * Revision 1.12  1994/06/21  19:46:05  john
 * Added palette effects to demo recording.
 *
 * Revision 1.11  1994/06/21  14:19:58  john
 * Put in hooks to record HUD messages.
 *
 * Revision 1.10  1994/06/20  11:50:42  john
 * Made demo record flash effect, and control center triggers.
 *
 * Revision 1.9  1994/06/17  18:01:29  john
 * A bunch of new stuff by John
 *
 * Revision 1.8  1994/06/17  12:13:34  john
 * More newdemo stuff; made editor->game transition start in slew mode.
 *
 * Revision 1.7  1994/06/16  13:02:02  john
 * Added morph hooks.
 *
 * Revision 1.6  1994/06/15  19:01:42  john
 * Added the capability to make 3d sounds play just once for the
 * laser hit wall effects.
 *
 * Revision 1.5  1994/06/15  14:57:11  john
 * Added triggers to demo recording.
 *
 * Revision 1.4  1994/06/14  20:42:19  john
 * Made robot matztn cntr not work until no robots or player are
 * in the tSegment.
 *
 * Revision 1.3  1994/06/14  14:43:52  john
 * Made doors work with newdemo system.
 *
 * Revision 1.2  1994/06/13  21:02:44  john
 * Initial version of new demo recording system.
 *
 * Revision 1.1  1994/06/13  15:51:09  john
 * Initial revision
 *
 *
 */


#ifndef _NEWDEMO_H
#define _NEWDEMO_H

#ifdef NEWDEMO

#define ND_STATE_NORMAL             0
#define ND_STATE_RECORDING          1
#define ND_STATE_PLAYBACK           2
#define ND_STATE_PAUSED             3
#define ND_STATE_REWINDING          4
#define ND_STATE_FASTFORWARD        5
#define ND_STATE_ONEFRAMEFORWARD    6
#define ND_STATE_ONEFRAMEBACKWARD   7
#define ND_STATE_PRINTSCREEN        8

#define DEMO_DIR                "demos/"

// Functions called during recording process...
extern void NDRecordStartDemo();
extern void NDRecordStartFrame(int frame_number, fix frameTime );
extern void NDRecordRenderObject(tObject * obj);
extern void NDRecordViewerObject(tObject * obj);
extern void NDRecordSound3D( int soundno, int angle, int volume );
extern void NDRecordSound3DOnce( int soundno, int angle, int volume );
extern void newdemo_recordSound_once( int soundno );
extern void NDRecordSound( int soundno );
extern void NDRecordWallHitProcess( int nSegment, int tSide, int damage, int playernum );
extern void NDRecordTrigger( int nSegment, int tSide, int nObject,int shot );
extern void NDRecordHostageRescued( int hostage_num );
extern void NDRecordMorphFrame();
extern void newdemo_record_player_stats(int shields, int energy, int score );
extern void NDRecordPlayerAfterburner(fix old_afterburner, fix afterburner);
extern void NDRecordWallToggle(int nSegment, int tSide );
extern void NDRecordControlCenterDestroyed();
extern void NDRecordHUDMessage(char *s);
extern void NDRecordPaletteEffect(short r, short g, short b);
extern void NDRecordPlayerEnergy(int, int);
extern void NDRecordPlayerShields(int, int);
extern void NDRecordPlayerFlags(uint, uint);
extern void NDRecordPlayerWeapon(int, int);
extern void NDRecordEffectBlowup(short, int, vmsVector *);
extern void NDRecordHomingDistance(fix);
extern void NDRecordLetterbox(void);
extern void NDRecordRearView(void);
extern void NDRecordRestoreCockpit(void);
extern void NDRecordWallSetTMapNum1(short seg,ubyte tSide,short cseg,ubyte cside,short tmap);
extern void NDRecordWallSetTMapNum2(short seg,ubyte tSide,short cseg,ubyte cside,short tmap);
extern void NDRecordMultiCloak(int pnum);
extern void NDRecordMultiDeCloak(int pnum);
extern void NDSetNewLevel(int level_num);
extern void NDRecordRestoreRearView(void);

extern void NDRecordMultiDeath(int pnum);
extern void NDRecordMultiKill(int pnum, sbyte kill);
extern void NDRecordMultiConnect(int pnum, int new_player, char *new_callsign);
extern void NDRecordMultiReconnect(int pnum);
extern void NDRecordMultiDisconnect(int pnum);
extern void NDRecordPlayerScore(int score);
extern void NDRecordMultiScore(int pnum, int score);
extern void NDRecordPrimaryAmmo(int old_ammo, int new_ammo);
extern void NDRecordSecondaryAmmo(int old_ammo, int new_ammo);
extern void NDRecordDoorOpening(int nSegment, int tSide);
extern void NDRecordLaserLevel(sbyte oldLevel, sbyte newLevel);
extern void NDRecordCloakingWall(int front_wall_num, int back_wall_num, ubyte nType, ubyte state, fix cloakValue, fix l0, fix l1, fix l2, fix l3);
extern void NDRecordSecretExitBlown(int truth);


// Functions called during playback process...
extern void newdemoObject_move_all();
extern void NDPlayBackOneFrame();
extern void NDGotoEnd();
extern void NDGotoBeginning();

// Interactive functions to control playback/record;
extern void NDStartPlayback( char * filename );
extern void NDStopPlayback();
extern void NDStartRecording();
extern void NDStopRecording();

extern int NDGetPercentDone();

extern void NDRecordLinkSoundToObject3( int soundno, short nObject, fix max_volume, fix  maxDistance, int loop_start, int loop_end );
extern int NDFindObject( int nSignature );
extern void NDRecordKillSoundLinkedToObject( int nObject );

#endif // NEWDEMO

#endif // _NEWDEMO_H
