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

extern void NDRecordLinkSoundToObject3( int soundno, short nObject, fix maxVolume, fix  maxDistance, int loop_start, int loop_end );
extern int NDFindObject( int nSignature );
extern void NDRecordKillSoundLinkedToObject( int nObject );

#endif // NEWDEMO

#endif // _NEWDEMO_H
