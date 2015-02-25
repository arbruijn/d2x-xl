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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "descent.h"
#include "error.h"
#include "objrender.h"
#include "cockpit.h"
#include "hostage.h"
#include "newdemo.h"
#include "text.h"


//------------- Globaly used hostage variables --------------------------------

int32_t nHostageTypes = MAX_HOSTAGE_TYPES;		  			// Number of hostage types
int32_t nHostageVClips [MAX_HOSTAGE_TYPES] = {33};	// tAnimationInfo num for each tpye of hostage


//-------------- Renders a hostage --------------------------------------------

void DrawHostage (CObject *objP)
{
DrawObjectRodTexPoly (objP, gameData.effects.vClips [0][objP->rType.animationInfo.nClipIndex].frames [objP->rType.animationInfo.nCurFrame], 
							 1, objP->rType.animationInfo.nCurFrame);
gameData.render.nTotalSprites++;
}


//------------- Called once when a hostage is rescued -------------------------

void RescueHostage (int32_t nHostage)
{
paletteManager.BumpEffect (0, 0, 25);		//small blue flash
LOCALPLAYER.hostages.nOnBoard++;
// Do an audio effect
if (gameData.demo.nState != ND_STATE_PLAYBACK)
	audio.PlaySound (SOUND_HOSTAGE_RESCUED, I2X (1));
HUDInitMessage (TXT_HOSTAGE_RESCUED);
}

//------------- Called once when a hostage is rescued -------------------------
