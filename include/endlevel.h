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

#ifndef _ENDLEVEL_H
#define _ENDLEVEL_H

#include "object.h"

#define EL_OFF				0		//not in endlevel
#define EL_FLYTHROUGH	1		//auto-flythrough in tunnel
#define EL_LOOKBACK		2		//looking back at CPlayerData
#define EL_OUTSIDE		3		//flying outside for a while
#define EL_STOPPED		4		//stopped, watching explosion
#define EL_PANNING		5		//panning around, watching player
#define EL_CHASING		6		//chasing player to station

extern int32_t Endlevel_sequence;
void DoEndLevelFrame();
void StopEndLevelSequence();
void StartEndLevelSequence(int32_t bSecret);
void RenderEndLevelFrame(fix eye_offset, int32_t window_num);

void render_external_scene();
void DrawExitModel();
void InitEndLevel();

//@@extern CFixVector mine_exit_point;
//@@extern CObject external_explosion;
//@@extern int32_t ext_expl_playing;

//called for each level to load & setup the exit sequence
void LoadEndLevelData(int32_t level_num);

void InitEndLevelData (void);

#endif /* _ENDLEVEL_H */
