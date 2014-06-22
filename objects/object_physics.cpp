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
#include "rendermine.h"
#include "mouse.h"
#include "kconfig.h"
#include "input.h"
#include "player.h"

//------------------------------------------------------------------------------

float CObject::SpeedScale (void)
{
return (info.nType == OBJ_PLAYER) ? shipModifiers [gameData.multiplayer.weaponStates [info.nId].nShip].v.speed : 1.0f;
}

//------------------------------------------------------------------------------

float CObject::ShieldScale (void)
{
return shipModifiers [gameData.multiplayer.weaponStates [info.nId].nShip].v.shield;
}

//------------------------------------------------------------------------------

float CObject::EnergyScale (void)
{
return shipModifiers [gameData.multiplayer.weaponStates [info.nId].nShip].v.energy;
}

// ----------------------------------------------------------------------------

void CObject::Wiggle (void)
{
	fix		xWiggle;
	int		nParent;
	CObject	*parentP;

if (gameStates.render.nShadowPass == 2)
	return;
if (gameOpts->app.bEpilepticFriendly)
	return;
if (!gameStates.app.bNostalgia && (!EGI_FLAG (nDrag, 0, 0, 0) || !EGI_FLAG (bWiggle, 1, 0, 1)))
	return;
nParent = gameData.objs.parentObjs [Index ()];
parentP = (nParent < 0) ? NULL : OBJECTS + nParent;
FixFastSinCos (fix (gameData.time.xGame / gameStates.gameplay.slowmo [1].fSpeed), &xWiggle, NULL);
xWiggle = 100 * xWiggle / (100 + extraGameInfo [0].nSpeedScale * 25);
if (gameData.time.xFrame < I2X (1))// Only scale wiggle if getting at least 1 FPS, to avoid causing the opposite problem.
	xWiggle = FixMul (xWiggle * 20, gameData.time.xFrame); //make wiggle fps-independent (based on pre-scaled amount of wiggle at 20 FPS)
if (SPECTATOR (this))
	OBJPOS (this)->vPos += (OBJPOS (this)->mOrient.m.dir.u * FixMul (xWiggle, gameData.pig.ship.player->wiggle)) * (I2X (1) / 20);
else if ((info.nType == OBJ_PLAYER) || !parentP)
	mType.physInfo.velocity += info.position.mOrient.m.dir.u * FixMul (xWiggle, gameData.pig.ship.player->wiggle);
else {
	mType.physInfo.velocity += parentP->info.position.mOrient.m.dir.u * FixMul (xWiggle, gameData.pig.ship.player->wiggle);
	info.position.vPos += mType.physInfo.velocity * gameData.time.xFrame;
	}
}

// ----------------------------------------------------------------------------
//look at keyboard, mouse, joystick, CyberMan, whatever, and set
//physics vars rotVel, velocity

#define AFTERBURNER_USE_SECS	3				//use up in 3 seconds
#define DROP_DELTA_TIME			(I2X (1)/15)	//drop 3 per second

void CObject::ApplyFlightControls (void)
{
	fix		forwardThrustTime;
	CObject*	gmObjP;

if (gameData.time.xFrame <= 0)
	return;

if (gameStates.app.bPlayerIsDead || gameStates.app.bEnterGame) {
	StopPlayerMovement ();
	controls.FlushInput ();
	gameStates.app.bEnterGame--;
	return;
	}

if ((info.nType != OBJ_PLAYER) || (info.nId != N_LOCALPLAYER))
	return;	//references to CPlayerShip require that this obj be the player

tGuidedMissileInfo *gmiP = gameData.objs.guidedMissile + N_LOCALPLAYER;
gmObjP = gmiP->objP;
if (gmObjP && (gmObjP->info.nSignature == gmiP->nSignature)) {
	CAngleVector	vRotAngs;
	CFixMatrix		mRot, mOrient;
	fix				speed;

	//this is a horrible hack.  guided missile stuff should not be
	//handled in the middle of a routine that is dealing with the player
	mType.physInfo.rotThrust.SetZero ();
	vRotAngs.v.coord.p = controls [0].pitchTime / 2 + gameStates.gameplay.seismic.nMagnitude / 64;
	vRotAngs.v.coord.b = controls [0].bankTime / 2 + gameStates.gameplay.seismic.nMagnitude / 16;
	vRotAngs.v.coord.h = controls [0].headingTime / 2 + gameStates.gameplay.seismic.nMagnitude / 64;
	mRot = CFixMatrix::Create (vRotAngs);
	mOrient = gmObjP->info.position.mOrient * mRot;
	gmObjP->info.position.mOrient = mOrient;
	speed = WI_speed (gmObjP->info.nId, gameStates.app.nDifficultyLevel);
	gmObjP->mType.physInfo.velocity = gmObjP->info.position.mOrient.m.dir.f * speed;
	if(IsMultiGame)
		MultiSendGuidedInfo (gmObjP, 0);
	}
else {
	mType.physInfo.rotThrust = CFixVector::Create (controls [0].pitchTime, controls [0].headingTime, controls [0].bankTime);
	}
forwardThrustTime = controls [0].forwardThrustTime;
if ((LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) && (RandShort () < OBJECTS [N_LOCALPLAYER].DriveDamage ())) {
	if (controls [0].afterburnerState) {			//player has key down
		fix xAfterburnerScale;
		int oldCount, newCount;

		//add in value from 0..1
		xAfterburnerScale = I2X (1) + Min (I2X (1) / 2, gameData.physics.xAfterburnerCharge) * 2;
		forwardThrustTime = FixMul (gameData.time.xFrame, xAfterburnerScale);	//based on full thrust
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
		fix xChargeUp = Min (gameData.time.xFrame / 8, I2X (1) - gameData.physics.xAfterburnerCharge);	//recharge over 8 seconds
		if (xChargeUp > 0) {
			fix xCurEnergy = LOCALPLAYER.Energy () - I2X (10);
			xCurEnergy = Max (xCurEnergy, 0) / 10;	//don't drop below 10
			if (xCurEnergy > 0) {	//maybe limit charge up by energy
				xChargeUp = Min (xChargeUp, xCurEnergy / 10);
				if (xChargeUp > 0) {
					gameData.physics.xAfterburnerCharge += xChargeUp;
					LOCALPLAYER.UpdateEnergy (-100 * xChargeUp / 10);	//full charge uses 10% of energy
					}
				}
			}
		}
	}

float fScale = SpeedScale ();

forwardThrustTime = fix (forwardThrustTime * fScale);
// Set object's thrust vector for forward/backward
mType.physInfo.thrust = info.position.mOrient.m.dir.f * forwardThrustTime;
// slide left/right
mType.physInfo.thrust += info.position.mOrient.m.dir.r * fix (controls [0].sidewaysThrustTime * fScale);
// slide up/down
mType.physInfo.thrust += info.position.mOrient.m.dir.u * fix (controls [0].verticalThrustTime * fScale);
mType.physInfo.thrust *= 2 * DriveDamage ();
if (!gameStates.input.bSkipControls)
	memcpy (&gameData.physics.playerThrust, &mType.physInfo.thrust, sizeof (gameData.physics.playerThrust));
if ((mType.physInfo.flags & PF_WIGGLE) && !gameData.objs.speedBoost [Index ()].bBoosted)
	Wiggle ();

// As of now, mType.physInfo.thrust & mType.physInfo.rotThrust are
// in units of time... In other words, if thrust==gameData.time.xFrame, that
// means that the user was holding down the MaxThrust key for the
// whole frame.  So we just scale them up by the max, and divide by
// gameData.time.xFrame to make them independant of framerate

//	Prevent divide overflows on high frame rates.
//	In a signed divide, you get an overflow if num >= div<<15
#if 0
if (!mType.physInfo.thrust.IsZero ()) {
	float fScale = float (gameData.pig.ship.player->maxThrust) / float (gameData.time.xFrame);
	mType.physInfo.thrust.dir.coord.x = fix (float (mType.physInfo.thrust.dir.coord.x) * fScale);
	mType.physInfo.thrust.dir.coord.y = fix (float (mType.physInfo.thrust.dir.coord.y) * fScale);
	mType.physInfo.thrust.dir.coord.z = fix (float (mType.physInfo.thrust.dir.coord.z) * fScale);
	}
if (!mType.physInfo.rotThrust.IsZero ()) {
	float fScale = float (gameData.pig.ship.player->maxRotThrust) / float (gameData.time.xFrame);
	mType.physInfo.rotThrust.dir.coord.x = fix (float (mType.physInfo.rotThrust.dir.coord.x) * fScale);
	mType.physInfo.rotThrust.dir.coord.y = fix (float (mType.physInfo.rotThrust.dir.coord.y) * fScale);
	mType.physInfo.rotThrust.dir.coord.z = fix (float (mType.physInfo.rotThrust.dir.coord.z) * fScale);
	}
#else
fix ft = gameData.time.xFrame;
fix maxThrust = fix (gameData.pig.ship.player->maxThrust * fScale);

//	Note, you must check for ft < I2X (1)/2, else you can get an overflow  on the << 15.
if ((ft < I2X (1) / 2) && ((ft << 15) <= maxThrust))
	ft = (maxThrust >> 15) + 1;
mType.physInfo.thrust *= FixDiv (maxThrust, ft);
if ((ft < I2X (1) / 2) && ((ft << 15) <= gameData.pig.ship.player->maxRotThrust))
	ft = (gameData.pig.ship.player->maxRotThrust >> 15) + 1;
mType.physInfo.rotThrust *= FixDiv (gameData.pig.ship.player->maxRotThrust, ft);
#endif

CWeaponState& ws = gameData.multiplayer.weaponStates [N_LOCALPLAYER];

ubyte nOldThrusters [5];

memcpy (nOldThrusters, ws.nThrusters, sizeof (nOldThrusters));
memset (ws.nThrusters, 0, sizeof (ws.nThrusters));

if (controls [0].forwardThrustTime < 0)
	ws.nThrusters [0] = FRONT_THRUSTER | FRONTAL_THRUSTER;
else //if (controls [1].forwardThrustTime > 0)
	ws.nThrusters [0] = REAR_THRUSTER | FRONTAL_THRUSTER;

if (controls [0].sidewaysThrustTime > 0)
	ws.nThrusters [1] |= RIGHT_THRUSTER | FR_THRUSTERS | TB_THRUSTERS | LATERAL_THRUSTER;
else if (controls [0].sidewaysThrustTime < 0)
	ws.nThrusters [1] |= LEFT_THRUSTER | FR_THRUSTERS | TB_THRUSTERS | LATERAL_THRUSTER;
if (controls [0].verticalThrustTime > 0)
	ws.nThrusters [1] |= BOTTOM_THRUSTER | FR_THRUSTERS | LR_THRUSTERS | LATERAL_THRUSTER;
else if (controls [0].verticalThrustTime < 0)
	ws.nThrusters [1] |= TOP_THRUSTER | FR_THRUSTERS | LR_THRUSTERS | LATERAL_THRUSTER;

if (controls [0].pitchTime > 0)
	ws.nThrusters [2] |= REAR_THRUSTER | BOTTOM_THRUSTER | LR_THRUSTERS | LATERAL_THRUSTER;
else if (controls [0].pitchTime < 0)
	ws.nThrusters [2] |= REAR_THRUSTER | TOP_THRUSTER | LR_THRUSTERS | LATERAL_THRUSTER;
if (controls [0].headingTime > 0)
	ws.nThrusters [3] |= REAR_THRUSTER | LEFT_THRUSTER | TB_THRUSTERS | LATERAL_THRUSTER;
else if (controls [0].headingTime < 0)
	ws.nThrusters [3] |= REAR_THRUSTER | RIGHT_THRUSTER | TB_THRUSTERS | LATERAL_THRUSTER;
if (controls [0].bankTime > 0)
	ws.nThrusters [4] |= LEFT_THRUSTER | BOTTOM_THRUSTER | FR_THRUSTERS | LATERAL_THRUSTER;
else if (controls [0].bankTime < 0)
	ws.nThrusters [4] |= RIGHT_THRUSTER | BOTTOM_THRUSTER | FR_THRUSTERS | LATERAL_THRUSTER;

if (!(ws.nThrusters [0] | ws.nThrusters [1] | ws.nThrusters [2] | ws.nThrusters [3] | ws.nThrusters [4]))
	ws.nThrusters [0] = REAR_THRUSTER | FRONTAL_THRUSTER;	// always on

if (IsMultiGame && memcmp (nOldThrusters, ws.nThrusters, sizeof (nOldThrusters)))
	MultiSendPlayerThrust ();

#if DBG
//HUDMessage (0, "%d %d", ws.nThrusters [0], ws.nThrusters [1]);
#endif
}

// ----------------------------------------------------------------------------
// For standard collision model, return the point on the objects hit sphere traversed by vector vDir
// as hit location and the sphere radius as distance
// For enhanced collision model, compute intersection of vDir with ellipsoid
// around the object determined by the three axes of its hit box as hit location
// and the distance of that point from the object's center

float CObject::CollisionPoint (CFloatVector* vDir, CFloatVector* vHit)
{
return 0.0f;
}

// ----------------------------------------------------------------------------
//eof
