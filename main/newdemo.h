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

#define DEMO_VERSION						15      // last D1 version was 13

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
void NDRecordStartDemo();
void NDRecordStartFrame(int frame_number, fix frameTime );
void NDRecordRenderObject(tObject * obj);
void NDRecordViewerObject(tObject * obj);
void NDRecordSound3D( int soundno, int angle, int volume );
void NDRecordSound3DOnce( int soundno, int angle, int volume );
void newdemo_recordSound_once( int soundno );
void NDRecordSound( int soundno );
void NDRecordWallHitProcess( int nSegment, int tSide, int damage, int playernum );
void NDRecordTrigger( int nSegment, int tSide, int nObject,int shot );
void NDRecordHostageRescued( int hostage_num );
void NDRecordMorphFrame();
void newdemo_record_player_stats(int shields, int energy, int score );
void NDRecordPlayerAfterburner(fix old_afterburner, fix afterburner);
void NDRecordWallToggle(int nSegment, int tSide );
void NDRecordControlCenterDestroyed();
void NDRecordHUDMessage(char *s);
void NDRecordPaletteEffect(short r, short g, short b);
void NDRecordPlayerEnergy(int, int);
void NDRecordPlayerShields(int, int);
void NDRecordPlayerFlags(uint, uint);
void NDRecordPlayerWeapon(int, int);
void NDRecordEffectBlowup(short, int, vmsVector *);
void NDRecordHomingDistance(fix);
void NDRecordLetterbox(void);
void NDRecordRearView(void);
void NDRecordRestoreCockpit(void);
void NDRecordWallSetTMapNum1(short seg,ubyte tSide,short cseg,ubyte cside,short tmap);
void NDRecordWallSetTMapNum2(short seg,ubyte tSide,short cseg,ubyte cside,short tmap);
void NDRecordMultiCloak(int pnum);
void NDRecordMultiDeCloak(int pnum);
void NDSetNewLevel(int level_num);
void NDRecordRestoreRearView(void);

void NDRecordMultiDeath(int pnum);
void NDRecordMultiKill(int pnum, sbyte kill);
void NDRecordMultiConnect(int pnum, int new_player, char *new_callsign);
void NDRecordMultiReconnect(int pnum);
void NDRecordMultiDisconnect(int pnum);
void NDRecordPlayerScore(int score);
void NDRecordMultiScore(int pnum, int score);
void NDRecordPrimaryAmmo(int old_ammo, int new_ammo);
void NDRecordSecondaryAmmo(int old_ammo, int new_ammo);
void NDRecordDoorOpening(int nSegment, int tSide);
void NDRecordLaserLevel(sbyte oldLevel, sbyte newLevel);
void NDRecordCloakingWall(int front_wall_num, int back_wall_num, ubyte nType, ubyte state, fix cloakValue, fix l0, fix l1, fix l2, fix l3);
void NDRecordSecretExitBlown(int truth);


// Functions called during playback process...
void newdemoObject_move_all();
void NDPlayBackOneFrame();
void NDGotoEnd();
void NDGotoBeginning();

// Interactive functions to control playback/record;
void NDStartPlayback( char * filename );
void NDStopPlayback();
void NDStartRecording();
void NDStopRecording();

int NDGetPercentDone();

void NDRecordLinkSoundToObject3( int soundno, short nObject, fix maxVolume, fix  maxDistance, int loop_start, int loop_end );
int NDFindObject( int nSignature );
void NDRecordKillSoundLinkedToObject( int nObject );
void NDStripFrames (char *outname, int bytes_to_strip);

#endif // NEWDEMO

#endif // _NEWDEMO_H
