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
#include <string.h>

#include "inferno.h"
#include "key.h"
#include "joy.h"
#include "timer.h"
#include "error.h"
#include "controls.h"
#include "render.h"
#include "mouse.h"
#include "kconfig.h"
#include "input.h"

// ----------------------------------------------------------------------------

static fix wiggleTime;

void WiggleObject (CObject *objP)
{
	fix		xWiggle;
	int		nParent;
	CObject	*pParent;

if (gameStates.render.nShadowPass == 2)
	return;
if (!gameStates.app.bNostalgia && (!EGI_FLAG (nDrag, 0, 0, 0) || !EGI_FLAG (bWiggle, 1, 0, 1)))
	return;
nParent = gameData.objs.parentObjs [OBJ_IDX (objP)];
pParent = (nParent < 0) ? NULL : OBJECTS + nParent;
FixFastSinCos ((fix) (gameData.time.xGame / gameStates.gameplay.slowmo [1].fSpeed), &xWiggle, NULL);
if (wiggleTime < F1_0)// Only scale wiggle if getting at least 1 FPS, to avoid causing the opposite problem.
	xWiggle = FixMul (xWiggle * 20, wiggleTime); //make wiggle fps-independent (based on pre-scaled amount of wiggle at 20 FPS)
if (SPECTATOR (objP))
	OBJPOS (objP)->vPos += (OBJPOS (objP)->mOrient.UVec() * FixMul (xWiggle, gameData.pig.ship.player->wiggle)) * (F1_0 / 20);
else if ((objP->info.nType == OBJ_PLAYER) || !pParent)
	objP->mType.physInfo.velocity += objP->info.position.mOrient.UVec() * FixMul (xWiggle, gameData.pig.ship.player->wiggle);
else {
	objP->mType.physInfo.velocity += pParent->info.position.mOrient.UVec() * FixMul (xWiggle, gameData.pig.ship.player->wiggle);
	objP->info.position.vPos += objP->mType.physInfo.velocity * wiggleTime;
	}
}

// ----------------------------------------------------------------------------
//look at keyboard, mouse, joystick, CyberMan, whatever, and set
//physics vars rotVel, velocity

#define AFTERBURNER_USE_SECS	3				//use up in 3 seconds
#define DROP_DELTA_TIME			(f1_0/15)	//drop 3 per second

void ReadFlyingControls (CObject *objP)
{
	fix	forwardThrustTime;
	CObject *gmObjP;
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

	if ((objP->info.nType != OBJ_PLAYER) || (objP->info.nId != gameData.multiplayer.nLocalPlayer))
		return;	//references to CPlayerShip require that this obj be the CPlayerData

   tGuidedMissileInfo *gmiP = gameData.objs.guidedMissile + gameData.multiplayer.nLocalPlayer;
	gmObjP = gmiP->objP;
	if (gmObjP && (gmObjP->info.nSignature == gmiP->nSignature)) {
		CAngleVector rotangs;
		CFixMatrix rotmat,tempm;
		fix speed;

		//this is a horrible hack.  guided missile stuff should not be
		//handled in the middle of a routine that is dealing with the CPlayerData
		objP->mType.physInfo.rotThrust.SetZero();
		rotangs[PA] = Controls [0].pitchTime / 2 + gameStates.gameplay.seismic.nMagnitude/64;
		rotangs[BA] = Controls [0].bankTime / 2 + gameStates.gameplay.seismic.nMagnitude/16;
		rotangs[HA] = Controls [0].headingTime / 2 + gameStates.gameplay.seismic.nMagnitude/64;
		rotmat = CFixMatrix::Create(rotangs);
		tempm = gmObjP->info.position.mOrient * rotmat;
		gmObjP->info.position.mOrient = tempm;
		speed = WI_speed (gmObjP->info.nId, gameStates.app.nDifficultyLevel);
		gmObjP->mType.physInfo.velocity = gmObjP->info.position.mOrient[FVEC] * speed;
		if(IsMultiGame)
			MultiSendGuidedInfo (gmObjP, 0);
		}
	else {
#if DBG
		if (Controls [0].headingTime)
			Controls [0].headingTime = Controls [0].headingTime;
#endif
		objP->mType.physInfo.rotThrust = CFixVector::Create(Controls[0].pitchTime,
		                                                   Controls[0].headingTime, //Controls [0].headingTime ? f1_0 / 4 : 0; //Controls [0].headingTime;
		                                                   Controls[0].bankTime);
		}
	forwardThrustTime = Controls [0].forwardThrustTime;
	if (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER)	{
		if (Controls [0].afterburnerState) {			//CPlayerData has key down
			//if (forwardThrustTime >= 0) { 		//..ccAnd isn't moving backward
			{
				fix afterburner_scale;
				int oldCount,newCount;

				//add in value from 0..1
				afterburner_scale = f1_0 + min(f1_0/2, gameData.physics.xAfterburnerCharge) * 2;
				forwardThrustTime = FixMul (gameData.time.xFrame, afterburner_scale);	//based on full thrust
				oldCount = (gameData.physics.xAfterburnerCharge / (DROP_DELTA_TIME / AFTERBURNER_USE_SECS));
				if (!gameStates.gameplay.bAfterburnerCheat)
					gameData.physics.xAfterburnerCharge -= gameData.time.xFrame / AFTERBURNER_USE_SECS;
				if (gameData.physics.xAfterburnerCharge < 0)
					gameData.physics.xAfterburnerCharge = 0;
				newCount = (gameData.physics.xAfterburnerCharge / (DROP_DELTA_TIME / AFTERBURNER_USE_SECS));
				if (gameStates.app.bNostalgia && (oldCount != newCount))
					gameStates.render.bDropAfterburnerBlob = 1;	//drop blob (after physics called)
			}
		}
		else {
			fix cur_energy,charge_up;

			//charge up to full
			charge_up = min(gameData.time.xFrame/8,f1_0 - gameData.physics.xAfterburnerCharge);	//recharge over 8 seconds
			cur_energy = LOCALPLAYER.energy - I2X (10);
			cur_energy = max(cur_energy, 0);	//don't drop below 10
			//maybe limit charge up by energy
			charge_up = min (charge_up,cur_energy / 10);
			gameData.physics.xAfterburnerCharge += charge_up;
			LOCALPLAYER.energy -= charge_up * 100 / 10;	//full charge uses 10% of energy
		}
	}
	// Set CObject's thrust vector for forward/backward
	objP->mType.physInfo.thrust = objP->info.position.mOrient[FVEC] * forwardThrustTime;
	// slide left/right
	objP->mType.physInfo.thrust += objP->info.position.mOrient[RVEC] * Controls [0].sidewaysThrustTime;
	// slide up/down
	objP->mType.physInfo.thrust += objP->info.position.mOrient[UVEC] * Controls [0].verticalThrustTime;
	if (!gameStates.input.bSkipControls)
		memcpy (&gameData.physics.playerThrust, &objP->mType.physInfo.thrust, sizeof (gameData.physics.playerThrust));
	bMulti = IsMultiGame;
	if ((objP->mType.physInfo.flags & PF_WIGGLE) && !gameData.objs.speedBoost [OBJ_IDX (objP)].bBoosted) {
#if 1//ndef _DEBUG
		wiggleTime = gameData.time.xFrame;
		WiggleObject (objP);
#endif
	}
	// As of now, objP->mType.physInfo.thrust & objP->mType.physInfo.rotThrust are
	// in units of time... In other words, if thrust==gameData.time.xFrame, that
	// means that the user was holding down the MaxThrust key for the
	// whole frame.  So we just scale them up by the max, and divide by
	// gameData.time.xFrame to make them independant of framerate

	//	Prevent divide overflows on high frame rates.
	//	In a signed divide, you get an overflow if num >= div<<15
	{
		fix	ft = gameData.time.xFrame;

		//	Note, you must check for ft < F1_0/2, else you can get an overflow  on the << 15.
		if ((ft < F1_0/2) && ((ft << 15) <= gameData.pig.ship.player->maxThrust))
			ft = (gameData.pig.ship.player->maxThrust >> 15) + 1;
		if (objP->mType.physInfo.thrust.Mag() > 250)
			objP = objP;
		objP->mType.physInfo.thrust *= FixDiv (gameData.pig.ship.player->maxThrust, ft);
		if (objP->mType.physInfo.thrust.Mag() > 250)
			objP = objP;
		if ((ft < F1_0/2) && ((ft << 15) <= gameData.pig.ship.player->maxRotThrust))
			ft = (gameData.pig.ship.player->maxThrust >> 15) + 1;
		objP->mType.physInfo.rotThrust *= FixDiv (gameData.pig.ship.player->maxRotThrust, ft);
	}

}

// ----------------------------------------------------------------------------
//eof
