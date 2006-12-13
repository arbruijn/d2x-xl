/* $Id: controls.c,v 1.5 2003/08/02 20:36:12 btb Exp $ */
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
static char rcsid[] = "$Id: controls.c,v 1.5 2003/08/02 20:36:12 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"
#include "key.h"
#include "joy.h"
#include "timer.h"
#include "error.h"

#include "inferno.h"
#include "game.h"
#include "object.h"
#include "player.h"

#include "controls.h"
#include "joydefs.h"
#include "render.h"
#include "args.h"
#include "palette.h"
#include "mouse.h"
#include "kconfig.h"
#include "laser.h"
#include "network.h"
#include "multi.h"
#include "vclip.h"
#include "fireball.h"
#include "hudmsg.h"
#include "input.h"

// ----------------------------------------------------------------------------

static fix wiggleTime;

void WiggleObject (tObject *objP)
{
if ((gameStates.render.nShadowPass != 2) &&
	 (EGI_FLAG (bWiggle, 0, 1))) {
	fix swiggle;
	int nParent = gameData.objs.parentObjs [OBJ_IDX (objP)];
	tObject *pParent = (nParent < 0) ? NULL : gameData.objs.objects + nParent;
	fix_fastsincos (gameData.time.xGame, &swiggle, NULL);
	if (wiggleTime < F1_0)// Only scale wiggle if getting at least 1 FPS, to avoid causing the opposite problem.
		swiggle = FixMul (swiggle * 20, wiggleTime); //make wiggle fps-independent (based on pre-scaled amount of wiggle at 20 FPS)
	if ((objP->nType == OBJ_PLAYER) || !pParent)
		VmVecScaleInc (&objP->mType.physInfo.velocity,
								 &objP->position.mOrient.uVec,
								 FixMul (swiggle, gameData.pig.ship.player->wiggle));
#if 1
	else {
		VmVecScaleInc (&objP->mType.physInfo.velocity,
								&pParent->position.mOrient.uVec,
								FixMul (swiggle, gameData.pig.ship.player->wiggle));
		VmVecScaleInc (&objP->position.vPos, &objP->mType.physInfo.velocity, wiggleTime);
		}
#endif
	}
}

// ----------------------------------------------------------------------------
//look at keyboard, mouse, joystick, CyberMan, whatever, and set 
//physics vars rotVel, velocity

fix xAfterburnerCharge=f1_0;
vmsVector player_thrust;

#define AFTERBURNER_USE_SECS	3				//use up in 3 seconds
#define DROP_DELTA_TIME			(f1_0/15)	//drop 3 per second

void ReadFlyingControls(tObject *objP)
{
	fix	forward_thrustTime;
	tObject *gmObjP;
	int	bMulti;
#if 0
	Assert(gameData.time.xFrame > 0); 		//Get MATT if hit this!
#else
	if (gameData.time.xFrame <= 0)
		return;
#endif

	if (gameStates.app.bPlayerIsDead || gameStates.app.bEnterGame) {
		StopPlayerMovement ();
		FlushInput ();
/*
		VmVecZero(&objP->mType.physInfo.rotThrust);
		VmVecZero(&objP->mType.physInfo.thrust);
		VmVecZero(&objP->mType.physInfo.velocity);
*/		
		gameStates.app.bEnterGame--;
		return;
	}

	if ((objP->nType!=OBJ_PLAYER) || (objP->id!=gameData.multi.nLocalPlayer)) 
		return;	//references to tPlayerShip require that this obj be the tPlayer

	gmObjP = gameData.objs.guidedMissile[gameData.multi.nLocalPlayer];
	if (gmObjP && (gmObjP->nSignature == gameData.objs.guidedMissileSig[gameData.multi.nLocalPlayer])) {
		vmsAngVec rotangs;
		vmsMatrix rotmat,tempm;
		fix speed;

		//this is a horrible hack.  guided missile stuff should not be
		//handled in the middle of a routine that is dealing with the tPlayer
		VmVecZero(&objP->mType.physInfo.rotThrust);
		rotangs.p = Controls.pitchTime / 2 + gameStates.gameplay.seismic.nMagnitude/64;
		rotangs.b = Controls.bankTime / 2 + gameStates.gameplay.seismic.nMagnitude/16;
		rotangs.h = Controls.headingTime / 2 + gameStates.gameplay.seismic.nMagnitude/64;
		VmAngles2Matrix(&rotmat,&rotangs);
		VmMatMul(&tempm,&gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->position.mOrient,&rotmat);
		gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->position.mOrient = tempm;
		speed = WI_speed (gmObjP->id,gameStates.app.nDifficultyLevel);
		VmVecCopyScale(&gmObjP->mType.physInfo.velocity, &gmObjP->position.mOrient.fVec,speed);
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendGuidedInfo (gmObjP, 0);
		}
	else {
		objP->mType.physInfo.rotThrust.x = Controls.pitchTime;
		objP->mType.physInfo.rotThrust.y = Controls.headingTime;
		objP->mType.physInfo.rotThrust.z = Controls.bankTime;
	}
	forward_thrustTime = Controls.forward_thrustTime;
	if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER)	{
		if (Controls.afterburner_state) {			//tPlayer has key down
			//if (forward_thrustTime >= 0) { 		//..and isn't moving backward
			{
				fix afterburner_scale;
				int old_count,new_count;
	
				//add in value from 0..1
				afterburner_scale = f1_0 + min (f1_0/2, xAfterburnerCharge) * 2;
				forward_thrustTime = FixMul (gameData.time.xFrame, afterburner_scale);	//based on full thrust
				old_count = (xAfterburnerCharge / (DROP_DELTA_TIME / AFTERBURNER_USE_SECS));
				xAfterburnerCharge -= gameData.time.xFrame / AFTERBURNER_USE_SECS;
				if (xAfterburnerCharge < 0)
					xAfterburnerCharge = 0;
				new_count = (xAfterburnerCharge / (DROP_DELTA_TIME / AFTERBURNER_USE_SECS));
				if (gameStates.app.bNostalgia && (old_count != new_count))
					gameStates.render.bDropAfterburnerBlob = 1;	//drop blob (after physics called)
			}
		}
		else {
			fix cur_energy,charge_up;
	
			//charge up to full
			charge_up = min(gameData.time.xFrame/8,f1_0 - xAfterburnerCharge);	//recharge over 8 seconds
			cur_energy = gameData.multi.players [gameData.multi.nLocalPlayer].energy - i2f (10);
			cur_energy = max(cur_energy, 0);	//don't drop below 10
			//maybe limit charge up by energy
			charge_up = min (charge_up,cur_energy / 10);
			xAfterburnerCharge += charge_up;
			gameData.multi.players [gameData.multi.nLocalPlayer].energy -= charge_up * 100 / 10;	//full charge uses 10% of energy
		}
	}
	// Set tObject's thrust vector for forward/backward
	VmVecCopyScale (&objP->mType.physInfo.thrust, &objP->position.mOrient.fVec, forward_thrustTime);
	// slide left/right
	VmVecScaleInc (&objP->mType.physInfo.thrust, &objP->position.mOrient.rVec, Controls.sideways_thrustTime);
	// slide up/down
	VmVecScaleInc (&objP->mType.physInfo.thrust, &objP->position.mOrient.uVec, Controls.vertical_thrustTime);
	if (!gameStates.input.bSkipControls)
		memcpy (&player_thrust, &objP->mType.physInfo.thrust, sizeof (player_thrust));
	//HUDMessage (0, "%d %d %d", player_thrust.x, player_thrust.y, player_thrust.z);
	bMulti = IsMultiGame;
	if ((objP->mType.physInfo.flags & PF_WIGGLE) && !gameData.objs.speedBoost [OBJ_IDX (objP)].bBoosted) {
#if 1//ndef _DEBUG
		wiggleTime = gameData.time.xFrame;
		WiggleObject (objP);
#endif		
	}
	// As of now, objP->mType.physInfo.thrust & objP->mType.physInfo.rotThrust are 
	// in units of time... In other words, if thrust==gameData.time.xFrame, that
	// means that the user was holding down the Max_thrust key for the
	// whole frame.  So we just scale them up by the max, and divide by
	// gameData.time.xFrame to make them independant of framerate

	//	Prevent divide overflows on high frame rates.
	//	In a signed divide, you get an overflow if num >= div<<15
	{
		fix	ft = gameData.time.xFrame;

		//	Note, you must check for ft < F1_0/2, else you can get an overflow  on the << 15.
		if ((ft < F1_0/2) && ((ft << 15) <= gameData.pig.ship.player->max_thrust)) {
#if TRACE
			con_printf (CON_DEBUG, "Preventing divide overflow in controls.c for Max_thrust!\n");
#endif
			ft = (gameData.pig.ship.player->max_thrust >> 15) + 1;
		}
		VmVecScale( &objP->mType.physInfo.thrust, FixDiv(gameData.pig.ship.player->max_thrust,ft) );

		if ((ft < F1_0/2) && ((ft << 15) <= gameData.pig.ship.player->max_rotthrust)) {
#if TRACE
			con_printf (CON_DEBUG, "Preventing divide overflow in controls.c for max_rotthrust!\n");
#endif
			ft = (gameData.pig.ship.player->max_thrust >> 15) + 1;
		}
		VmVecScale( &objP->mType.physInfo.rotThrust, FixDiv(gameData.pig.ship.player->max_rotthrust,ft) );
	}

}

// ----------------------------------------------------------------------------
//eof
