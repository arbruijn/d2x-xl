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

#include "inferno.h"
#include "error.h"
#include "objrender.h"
#include "gauges.h"
#include "hostage.h"
#include "newdemo.h"
#include "text.h"


//------------- Globaly used hostage variables --------------------------------

int nHostageTypes = MAX_HOSTAGE_TYPES;		  			// Number of hostage types
int nHostageVClips [MAX_HOSTAGE_TYPES] = {33};	// tVideoClip num for each tpye of hostage


//-------------- Renders a hostage --------------------------------------------

void DrawHostage (tObject *objP)
{
DrawObjectRodTexPoly (objP, gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].frames [objP->rType.vClipInfo.nCurFrame], 
							 1, objP->rType.vClipInfo.nCurFrame);
gameData.render.nTotalSprites++;
}


//------------- Called once when a hostage is rescued -------------------------

void RescueHostage (int nHostage)
{
PALETTE_FLASH_ADD(0, 0, 25);		//small blue flash
LOCALPLAYER.hostages.nOnBoard++;
// Do an audio effect
if (gameData.demo.nState != ND_STATE_PLAYBACK)
	DigiPlaySample (SOUND_HOSTAGE_RESCUED, F1_0);
HUDInitMessage (TXT_HOSTAGE_RESCUED);
}

//------------- Called once when a hostage is rescued -------------------------
