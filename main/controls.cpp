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

#include "descent.h"
#include "key.h"
#include "joy.h"
#include "timer.h"
#include "error.h"
#include "controls.h"
#include "rendermine.h"
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
if (gameOpts->app.bEpilepticFriendly)
	return;
if (!gameStates.app.bNostalgia && (!EGI_FLAG (nDrag, 0, 0, 0) || !EGI_FLAG (bWiggle, 1, 0, 1)))
	return;
nParent = gameData.objs.parentObjs [objP->Index ()];
pParent = (nParent < 0) ? NULL : OBJECTS + nParent;
FixFastSinCos ((fix) (gameData.time.xGame / gameStates.gameplay.slowmo [1].fSpeed), &xWiggle, NULL);
if (wiggleTime < I2X (1))// Only scale wiggle if getting at least 1 FPS, to avoid causing the opposite problem.
	xWiggle = FixMul (xWiggle * 20, wiggleTime); //make wiggle fps-independent (based on pre-scaled amount of wiggle at 20 FPS)
if (SPECTATOR (objP))
	OBJPOS (objP)->vPos += (OBJPOS (objP)->mOrient.UVec () * FixMul (xWiggle, gameData.pig.ship.player->wiggle)) * (I2X (1) / 20);
else if ((objP->info.nType == OBJ_PLAYER) || !pParent)
	objP->mType.physInfo.velocity += objP->info.position.mOrient.UVec () * FixMul (xWiggle, gameData.pig.ship.player->wiggle);
else {
	objP->mType.physInfo.velocity += pParent->info.position.mOrient.UVec () * FixMul (xWiggle, gameData.pig.ship.player->wiggle);
	objP->info.position.vPos += objP->mType.physInfo.velocity * wiggleTime;
	}
}

// ----------------------------------------------------------------------------
//look at keyboard, mouse, joystick, CyberMan, whatever, and set
//physics vars rotVel, velocity

#define AFTERBURNER_USE_SECS	3				//use up in 3 seconds
#define DROP_DELTA_TIME			(I2X (1)/15)	//drop 3 per second

void ReadFlyingControls (CObject *objP)
{
	fix		forwardThrustTime;
	CObject*	gmObjP;
	int		bMulti;

if (gameData.time.xFrame <= 0)
	return;

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
	CAngleVector	vRotAngs;
	CFixMatrix		mRot, mOrient;
	fix				speed;

	//this is a horrible hack.  guided missile stuff should not be
	//handled in the middle of a routine that is dealing with the CPlayerData
	objP->mType.physInfo.rotThrust.SetZero ();
	vRotAngs [PA] = Controls [0].pitchTime / 2 + gameStates.gameplay.seismic.nMagnitude / 64;
	vRotAngs [BA] = Controls [0].bankTime / 2 + gameStates.gameplay.seismic.nMagnitude / 16;
	vRotAngs [HA] = Controls [0].headingTime / 2 + gameStates.gameplay.seismic.nMagnitude / 64;
	mRot = CFixMatrix::Create (vRotAngs);
	mOrient = gmObjP->info.position.mOrient * mRot;
	gmObjP->info.position.mOrient = mOrient;
	speed = WI_speed (gmObjP->info.nId, gameStates.app.nDifficultyLevel);
	gmObjP->mType.physInfo.velocity = gmObjP->info.position.mOrient.FVec () * speed;
	if(IsMultiGame)
		MultiSendGuidedInfo (gmObjP, 0);
	}
else {
#if DBG
	if (Controls [0].headingTime)
		Controls [0].headingTime = Controls [0].headingTime;
#endif
	objP->mType.physInfo.rotThrust = CFixVector::Create (Controls [0].pitchTime,
	                                                     Controls [0].headingTime, //Controls [0].headingTime ? I2X (1) / 4 : 0; //Controls [0].headingTime;
	                                                     Controls [0].bankTime);
	}
forwardThrustTime = Controls [0].forwardThrustTime;
if ((LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) && (d_rand () < OBJECTS [gameData.multiplayer.nLocalPlayer].DriveDamage ())) {
	if (Controls [0].afterburnerState) {			//CPlayerData has key down
		fix afterburner_scale;
		int oldCount,newCount;

		//add in value from 0..1
		afterburner_scale = I2X (1) + min (I2X (1) / 2, gameData.physics.xAfterburnerCharge) * 2;
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
	else {
		fix xChargeUp = min (gameData.time.xFrame / 8, I2X (1) - gameData.physics.xAfterburnerCharge);	//recharge over 8 seconds
		if (xChargeUp > 0) {
			fix xCurEnergy = LOCALPLAYER.energy - I2X (10);
			xCurEnergy = max (xCurEnergy, 0) / 10;	//don't drop below 10
			if (xCurEnergy > 0) {	//maybe limit charge up by energy
				xChargeUp = min (xChargeUp, xCurEnergy / 10);
				if (xChargeUp > 0) {
					gameData.physics.xAfterburnerCharge += xChargeUp;
					LOCALPLAYER.energy -= xChargeUp * 100 / 10;	//full charge uses 10% of energy
					}
				}
			}
		}
	}
// Set CObject's thrust vector for forward/backward
objP->mType.physInfo.thrust = objP->info.position.mOrient.FVec () * forwardThrustTime;
// slide left/right
objP->mType.physInfo.thrust += objP->info.position.mOrient.RVec () * Controls [0].sidewaysThrustTime;
// slide up/down
objP->mType.physInfo.thrust += objP->info.position.mOrient.UVec () * Controls [0].verticalThrustTime;
objP->mType.physInfo.thrust *= 2 * objP->DriveDamage ();
if (!gameStates.input.bSkipControls)
	memcpy (&gameData.physics.playerThrust, &objP->mType.physInfo.thrust, sizeof (gameData.physics.playerThrust));
bMulti = IsMultiGame;
if ((objP->mType.physInfo.flags & PF_WIGGLE) && !gameData.objs.speedBoost [objP->Index ()].bBoosted) {
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
fix	ft = gameData.time.xFrame;

//	Note, you must check for ft < I2X (1)/2, else you can get an overflow  on the << 15.
if ((ft < I2X (1)/2) && ((ft << 15) <= gameData.pig.ship.player->maxThrust))
	ft = (gameData.pig.ship.player->maxThrust >> 15) + 1;
objP->mType.physInfo.thrust *= FixDiv (gameData.pig.ship.player->maxThrust, ft);
if ((ft < I2X (1)/2) && ((ft << 15) <= gameData.pig.ship.player->maxRotThrust))
	ft = (gameData.pig.ship.player->maxThrust >> 15) + 1;
objP->mType.physInfo.rotThrust *= FixDiv (gameData.pig.ship.player->maxRotThrust, ft);
}

// ----------------------------------------------------------------------------
//eof
