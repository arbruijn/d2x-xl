/* 
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX"). PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING, AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES. IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES. THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION. ALL RIGHTS RESERVED.
*/

#ifndef _NEWDEMO_H
#define _NEWDEMO_H

#ifdef NEWDEMO

#define DEMO_VERSION						15   // last D1 version was 13

#define ND_STATE_NORMAL					0
#define ND_STATE_RECORDING				1
#define ND_STATE_PLAYBACK				2
#define ND_STATE_PAUSED					3
#define ND_STATE_REWINDING				4
#define ND_STATE_FASTFORWARD			5
#define ND_STATE_ONEFRAMEFORWARD		6
#define ND_STATE_ONEFRAMEBACKWARD	7
#define ND_STATE_PRINTSCREEN			8

#define DEMO_DIR        "demos/"

// Functions called during recording process...
void NDRecordStartDemo (void);
void NDRecordStartFrame (int frame_number, fix frameTime);
void NDRecordRenderObject (CObject* objP);
void NDRecordViewerObject (CObject* objP);
void NDRecordSound3D (int nSound, int angle, int volume);
void NDRecordSound3DOnce (int nSound, int angle, int volume);
void newdemo_recordSound_once (int nSound);
void NDRecordSound (int nSound);
void NDRecordWallHitProcess (int nSegment, int nSide, int damage, int playernum);
void NDRecordTrigger (int nSegment, int nSide, int nObject, int shot);
void NDRecordHostageRescued (int nHostage);
void NDRecordMorphFrame (tMorphInfo* md);
void newdemo_record_player_stats (int shields, int energy, int score);
void NDRecordPlayerAfterburner (fix old_afterburner, fix afterburner);
void NDRecordWallToggle (int nSegment, int nSide);
void NDRecordControlCenterDestroyed (void);
void NDRecordHUDMessage (char* s);
void NDRecordPaletteEffect (short r, short g, short b);
void NDRecordPlayerEnergy (int, int);
void NDRecordPlayerShields (int, int);
void NDRecordPlayerFlags (uint, uint);
void NDRecordPlayerWeapon (int, int);
void NDRecordEffectBlowup (short, int, CFixVector&);
void NDRecordHomingDistance (fix);
void NDRecordLetterbox (void);
void NDRecordRearView (void);
void NDRecordRestoreCockpit (void);
void NDRecordWallSetTMapNum1 (short nSegment, ubyte nSide, short cseg, ubyte nConnSide, short tmap);
void NDRecordWallSetTMapNum2 (short nSegment, ubyte nSide, short cseg, ubyte nConnSide, short tmap);
void NDRecordMultiCloak (int nPlayer);
void NDRecordMultiDeCloak (int nPlayer);
void NDSetNewLevel (int level_num);
void NDRecordRestoreRearView (void);

void NDRecordMultiDeath (int nPlayer);
void NDRecordMultiKill (int nPlayer, sbyte kill);
void NDRecordMultiConnect (int nPlayer, int new_player, char* new_callsign);
void NDRecordMultiReconnect (int nPlayer);
void NDRecordMultiDisconnect (int nPlayer);
void NDRecordPlayerScore (int score);
void NDRecordMultiScore (int nPlayer, int score);
void NDRecordPrimaryAmmo (int old_ammo, int new_ammo);
void NDRecordSecondaryAmmo (int old_ammo, int new_ammo);
void NDRecordDoorOpening (int nSegment, int nSide);
void NDRecordLaserLevel (sbyte oldLevel, sbyte newLevel);
void NDRecordCloakingWall (int nFrontWall, int nBackWall, ubyte nType, ubyte state, fix cloakValue, fix l0, fix l1, fix l2, fix l3);
void NDRecordSecretExitBlown (int truth);


// Functions called during playback process...
void newdemoObject_move_all (void);
void NDPlayBackOneFrame (void);
void NDGotoEnd (void);
void NDGotoBeginning (void);

// Interactive functions to control playback/record;
void NDStartPlayback (char* filename);
void NDStopPlayback (void);
void NDStartRecording (void);
void NDStopRecording (void);

int NDGetPercentDone (void);

void NDRecordLinkSoundToObject3 (int nSound, short nObject, fix maxVolume, fix maxDistance, int loop_start, int loop_end);
int NDFindObject (int nSignature);
void NDRecordKillSoundLinkedToObject (int nObject);
void NDStripFrames (char* outname, int bytes_to_strip);

#endif // NEWDEMO

#endif // _NEWDEMO_H
