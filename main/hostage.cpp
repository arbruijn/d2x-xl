/* $Id: hostage.c,v 1.3 2003/10/10 09:36:35 btb Exp $ */
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

#ifdef RCS
static char rcsid[] = "$Id: hostage.c,v 1.3 2003/10/10 09:36:35 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inferno.h"
#include "error.h"
#include "object.h"
#include "objrender.h"
#include "game.h"
#include "player.h"
#include "gauges.h"
#include "hostage.h"
#include "vclip.h"
#include "newdemo.h"
#include "text.h"


//------------- Globaly used hostage variables --------------------------------

int nHostageTypes = MAX_HOSTAGE_TYPES;		  			// Number of hostage types
int nHostageVClips [MAX_HOSTAGE_TYPES] = {33};	// tVideoClip num for each tpye of hostage


//-------------- Renders a hostage --------------------------------------------

void DrawHostage(tObject *objP)
{
DrawObjectRodTexPoly (objP, gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].frames [objP->rType.vClipInfo.nCurFrame], 1, objP->rType.vClipInfo.nCurFrame);
}


//------------- Called once when a hostage is rescued -------------------------
void hostage_rescue(int blah)
{
	PALETTE_FLASH_ADD(0, 0, 25);		//small blue flash

	LOCALPLAYER.hostages.nOnBoard++;

	// Do an audio effect
	if (gameData.demo.nState != ND_STATE_PLAYBACK)
		DigiPlaySample(SOUND_HOSTAGE_RESCUED, F1_0);

	HUDInitMessage(TXT_HOSTAGE_RESCUED);
}
