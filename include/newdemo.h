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
void NDRecordStartFrame (int32_t frame_number, fix frameTime);
void NDRecordRenderObject (CObject* objP);
void NDRecordViewerObject (CObject* objP);
void NDRecordSound3D (int32_t nSound, int32_t angle, int32_t volume);
void NDRecordSound3DOnce (int32_t nSound, int32_t angle, int32_t volume);
void newdemo_recordSound_once (int32_t nSound);
void NDRecordSound (int32_t nSound);
void NDRecordWallHitProcess (int32_t nSegment, int32_t nSide, int32_t damage, int32_t playernum);
void NDRecordTrigger (int32_t nSegment, int32_t nSide, int32_t nObject, int32_t shot);
void NDRecordHostageRescued (int32_t nHostage);
void NDRecordMorphFrame (tMorphInfo* md);
void NDRecordPlayerStats (int32_t shield, int32_t energy, int32_t score);
void NDRecordPlayerAfterburner (fix old_afterburner, fix afterburner);
void NDRecordWallToggle (int32_t nSegment, int32_t nSide);
void NDRecordControlCenterDestroyed (void);
void NDRecordHUDMessage (char* s);
void NDRecordPaletteEffect (int16_t r, int16_t g, int16_t b);
void NDRecordPlayerEnergy (int32_t, int32_t);
void NDRecordPlayerShield (int32_t, int32_t);
void NDRecordPlayerFlags (uint32_t, uint32_t);
void NDRecordPlayerWeapon (int32_t, int32_t);
void NDRecordEffectBlowup (int16_t, int32_t, CFixVector&);
void NDRecordHomingDistance (fix);
void NDRecordLetterbox (void);
void NDRecordRearView (void);
void NDRecordRestoreCockpit (void);
void NDRecordWallSetTMapNum1 (int16_t nSegment, uint8_t nSide, int16_t nConnSeg, uint8_t nConnSide, int16_t tmap, int32_t nAnim, int32_t nFrame);
void NDRecordWallSetTMapNum2 (int16_t nSegment, uint8_t nSide, int16_t nConnSeg, uint8_t nConnSide, int16_t tmap, int32_t nAnim, int32_t nFrame);
void NDRecordMultiCloak (int32_t nPlayer);
void NDRecordMultiDeCloak (int32_t nPlayer);
void NDSetNewLevel (int32_t level_num);
void NDRecordRestoreRearView (void);

void NDRecordMultiDeath (int32_t nPlayer);
void NDRecordMultiKill (int32_t nPlayer, int8_t kill);
void NDRecordMultiConnect (int32_t nPlayer, int32_t new_player, char* new_callsign);
void NDRecordMultiReconnect (int32_t nPlayer);
void NDRecordMultiDisconnect (int32_t nPlayer);
void NDRecordPlayerScore (int32_t score);
void NDRecordMultiScore (int32_t nPlayer, int32_t score);
void NDRecordPrimaryAmmo (int32_t old_ammo, int32_t new_ammo);
void NDRecordSecondaryAmmo (int32_t old_ammo, int32_t new_ammo);
void NDRecordDoorOpening (int32_t nSegment, int32_t nSide);
void NDRecordLaserLevel (int8_t oldLevel, int8_t newLevel);
void NDRecordCloakingWall (int32_t nFrontWall, int32_t nBackWall, uint8_t nType, uint8_t state, fix cloakValue, fix l0, fix l1, fix l2, fix l3);
void NDRecordSecretExitBlown (int32_t truth);
void NDRecordGuidedEnd (void);

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

float NDGetPercentDone (void);

void NDRecordCreateObjectSound (int32_t nSound, int16_t nObject, fix maxVolume, fix maxDistance, int32_t loop_start, int32_t loop_end);
void NDRecordCockpitChange (int32_t mode);
int32_t NDFindObject (int32_t nSignature);
void NDRecordDestroySoundObject (int32_t nObject);
void NDStripFrames (char* outname, int32_t bytes_to_strip);

extern CObject demoRightExtra, demoLeftExtra;
extern uint8_t nDemoDoRight, nDemoDoLeft;
extern uint8_t nDemoDoingRight, nDemoDoingLeft;
extern char nDemoWBUType [];
extern char bDemoRearCheck [];
extern const char *szDemoExtraMessage [];

#endif // _NEWDEMO_H
