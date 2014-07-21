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

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "descent.h"
#include "stdlib.h"
#include "texmap.h"
#include "key.h"
#include "segmath.h"
#include "textures.h"
#include "lightning.h"
#include "objsmoke.h"
#include "physics.h"
#include "slew.h"
#include "rendermine.h"
#include "fireball.h"
#include "error.h"
#include "endlevel.h"
#include "timer.h"
#include "segmath.h"
#include "collide.h"
#include "dynlight.h"
#include "interp.h"
#include "newdemo.h"
#include "cockpit.h"
#include "text.h"
#include "sphere.h"
#include "input.h"
#include "automap.h"
#include "u_mem.h"
#include "entropy.h"
#include "objrender.h"
#include "dropobject.h"
#include "marker.h"
#include "hiresmodels.h"
#include "loadgame.h"
#include "multi.h"
#include "playerdeath.h"
#include "cockpit.h"
#include "renderlib.h"
#include "network_lib.h"
#ifdef FORCE_FEEDBACK
#	include "tactile.h"
#endif

//------------------------------------------------------------------------------

#define	DEATH_SEQUENCE_EXPLODE_TIME	 (I2X (2))

CObject*	viewerSaveP;
int32_t	nPlayerFlagsSave;
fix		xCameraToPlayerDistGoal=I2X (4);
uint8_t	nControlTypeSave, nRenderTypeSave;
int32_t	nKilledInFrame = -1;
int16_t	nKilledObjNum = -1;

//------------------------------------------------------------------------------

//	Camera is less than size of player away from
void SetCameraPos (CFixVector *vCameraPos, CObject *objP)
{
	CFixVector	vPlayerCameraOffs = *vCameraPos - objP->info.position.vPos;
	int32_t			count = 0;
	fix			xCameraPlayerDist;
	fix			xFarScale;

xCameraPlayerDist = vPlayerCameraOffs.Mag ();
if (xCameraPlayerDist < xCameraToPlayerDistGoal) { // 2*objP->info.xSize) {
	//	Camera is too close to player CObject, so move it away.
	CHitResult		hitResult;
	CFixVector	local_p1;

	if (vPlayerCameraOffs.IsZero ())
		vPlayerCameraOffs.v.coord.x += I2X (1)/16;

	hitResult.nType = HIT_WALL;
	xFarScale = I2X (1);

	while ((hitResult.nType != HIT_NONE) && (count++ < 6)) {
		CFixVector	closer_p1;
		CFixVector::Normalize (vPlayerCameraOffs);
		vPlayerCameraOffs *= xCameraToPlayerDistGoal;

		closer_p1 = objP->info.position.vPos + vPlayerCameraOffs;	//	This is the actual point we want to put the camera at.
		vPlayerCameraOffs *= xFarScale;				//	...but find a point 50% further away...
		local_p1 = objP->info.position.vPos + vPlayerCameraOffs;		//	...so we won't have to do as many cuts.

		CHitQuery hitQuery (0, &objP->info.position.vPos, &local_p1, objP->info.nSegment, objP->Index ());

		FindHitpoint (hitQuery, hitResult);
		if (hitResult.nType == HIT_NONE)
			*vCameraPos = closer_p1;
		else {
			vPlayerCameraOffs = CFixVector::Random();
			xFarScale = I2X (3) / 2;
			}
		}
	}
}

//------------------------------------------------------------------------------

void DeadPlayerEnd (void)
{
if (!gameStates.app.bPlayerIsDead)
	return;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordRestoreCockpit ();
if (gameData.objs.deadPlayerCamera) {
	ReleaseObject (OBJ_IDX (gameData.objs.deadPlayerCamera));
	gameData.objs.deadPlayerCamera = NULL;
	}
CGenericCockpit::Rewind ();
gameStates.app.bPlayerIsDead = 0;
LOCALPLAYER.m_bExploded = 0;
gameData.objs.viewerP = viewerSaveP;
gameData.objs.consoleP->SetType (OBJ_PLAYER);
gameData.objs.consoleP->info.nFlags = nPlayerFlagsSave;
Assert ((nControlTypeSave == CT_FLYING) || (nControlTypeSave == CT_SLEW));
gameData.objs.consoleP->info.controlType = nControlTypeSave;
gameData.objs.consoleP->info.renderType = nRenderTypeSave;
LOCALPLAYER.flags &= ~PLAYER_FLAGS_INVULNERABLE;
gameStates.app.bPlayerEggsDropped = 0;
}

//------------------------------------------------------------------------------

void DestroyPlayerShip (void)
{
if (!LOCALPLAYER.m_bExploded) {
	LOCALPLAYER.m_bExploded = 1;
	if (gameStates.app.bDeathSequenceAborted) { //xTimeDead > DEATH_SEQUENCE_LENGTH) {
		StopPlayerMovement ();
		gameStates.app.bEnterGame = 2;
		}
	else {
		if (LOCALPLAYER.hostages.nOnBoard > 1)
			HUDInitMessage (TXT_SHIP_DESTROYED_2, LOCALPLAYER.hostages.nOnBoard);
		else if (LOCALPLAYER.hostages.nOnBoard == 1)
			HUDInitMessage (TXT_SHIP_DESTROYED_1);
		else
			HUDInitMessage (TXT_SHIP_DESTROYED_0);
		}
	if (IsNetworkGame) {
		AdjustMineSpawn ();
		MultiCapObjects ();
		}
	if (IsMultiGame)
		gameStates.app.SRand ();
	DropPlayerEggs (gameData.objs.consoleP);
	if (IsMultiGame)
		MultiSendPlayerExplode (MULTI_PLAYER_EXPLODE);
	gameData.objs.consoleP->ExplodeSplashDamagePlayer ();
	//is this next line needed, given the splash damage call above?
	gameData.objs.consoleP->Explode (0);
	gameData.objs.consoleP->info.nFlags &= ~OF_SHOULD_BE_DEAD;	//don't really kill player
	gameData.objs.consoleP->info.renderType = RT_NONE;				//..just make him disappear
	gameData.objs.consoleP->SetType (OBJ_GHOST);						//..and kill intersections
	LOCALPLAYER.flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
	}
}

//------------------------------------------------------------------------------

void DeadPlayerFrame (void)
{
	fix			xTimeDead, h;
	CFixVector	fVec;

if (gameStates.app.bPlayerIsDead) {
	xTimeDead = gameData.time.xGame - gameStates.app.nPlayerTimeOfDeath;

	//	If unable to create camera at time of death, create now.
	if (!gameData.objs.deadPlayerCamera) {
		CObject *playerP = OBJECTS + LOCALPLAYER.nObject;
		int32_t nObject = CreateCamera (playerP);
		if (nObject != -1)
			gameData.objs.viewerP = gameData.objs.deadPlayerCamera = OBJECTS + nObject;
		else
			Int3 ();
		}
	h = DEATH_SEQUENCE_EXPLODE_TIME - xTimeDead;
	h = Max (0, h);
	gameData.objs.consoleP->mType.physInfo.rotVel = CFixVector::Create(h / 4, h / 2, h / 3);
	xCameraToPlayerDistGoal = Min (xTimeDead * 8, I2X (20)) + gameData.objs.consoleP->info.xSize;
	SetCameraPos (&gameData.objs.deadPlayerCamera->info.position.vPos, gameData.objs.consoleP);
	fVec = gameData.objs.consoleP->info.position.vPos - gameData.objs.deadPlayerCamera->info.position.vPos;
	gameData.objs.deadPlayerCamera->info.position.mOrient = CFixMatrix::CreateF(fVec);

	if (xTimeDead > DEATH_SEQUENCE_EXPLODE_TIME) {
		if (!LOCALPLAYER.m_bExploded) {
			if (LOCALPLAYER.hostages.nOnBoard > 1)
				HUDInitMessage (TXT_SHIP_DESTROYED_2, LOCALPLAYER.hostages.nOnBoard);
			else if (LOCALPLAYER.hostages.nOnBoard == 1)
				HUDInitMessage (TXT_SHIP_DESTROYED_1);
			else
				HUDInitMessage (TXT_SHIP_DESTROYED_0);

#ifdef FORCE_FEEDBACK
			if (TactileStick)
				ClearForces ();
#endif
			if (gameOpts->gameplay.bObserve)
				gameStates.app.bDeathSequenceAborted = 1;
			else
				DestroyPlayerShip ();
			}
		}
	else {
		if (RandShort () < gameData.time.xFrame * 4) {
			if (IsMultiGame)
				MultiSendCreateExplosion (N_LOCALPLAYER);
			CreateSmallFireballOnObject (gameData.objs.consoleP, I2X (1), 1);
			}
		}
	if (gameStates.app.bDeathSequenceAborted) {
		DestroyPlayerShip ();
		DoPlayerDead ();
		}
	}
}

//	------------------------------------------------------------------------

void StartPlayerDeathSequence (CObject* playerObjP)
{
Assert (playerObjP == gameData.objs.consoleP);
gameData.objs.speedBoost [OBJ_IDX (gameData.objs.consoleP)].bBoosted = 0;
if (gameStates.app.bPlayerIsDead)
	return;
if (gameData.objs.deadPlayerCamera) {
	ReleaseObject (OBJ_IDX (gameData.objs.deadPlayerCamera));
	gameData.objs.deadPlayerCamera = NULL;
	}
StopConquerWarning ();
//Assert (gameStates.app.bPlayerIsDead == 0);
//Assert (gameData.objs.deadPlayerCamera == NULL);
ResetRearView ();
nKilledInFrame = gameData.app.nFrameCount;
nKilledObjNum = OBJ_IDX (playerObjP);
gameStates.app.bDeathSequenceAborted = 0;
if (!IsMultiGame)
	HUDClearMessages ();
else {
	MultiSendKill (LOCALPLAYER.nObject);
//		If Hoard, increase number of orbs by 1
//    Only if you haven't killed yourself
//		This prevents cheating
	if (IsHoardGame)
		if (!gameStates.multi.bSuicide)
			if (LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] < 12)
				LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]++;
	}
paletteManager.SetRedEffect (40);
gameStates.app.bPlayerIsDead = 1;
#ifdef FORCE_FEEDBACK
   if (TactileStick)
	Buffeting (70);
#endif
//LOCALPLAYER.flags &= ~ (PLAYER_FLAGS_AFTERBURNER);
playerObjP->mType.physInfo.rotThrust.SetZero ();
playerObjP->mType.physInfo.thrust.SetZero ();
playerObjP->ResetDamage ();
gameStates.app.nPlayerTimeOfDeath = gameData.time.xGame;
int32_t nObject = CreateCamera (playerObjP);
viewerSaveP = gameData.objs.viewerP;
if (nObject != -1)
	gameData.objs.viewerP = gameData.objs.deadPlayerCamera = OBJECTS + nObject;
else {
	Int3 ();
	gameData.objs.deadPlayerCamera = NULL;
	}
CGenericCockpit::Save (true);
cockpit->Activate (CM_LETTERBOX);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordLetterbox ();
nPlayerFlagsSave = playerObjP->info.nFlags;
nControlTypeSave = playerObjP->info.controlType;
nRenderTypeSave = playerObjP->info.renderType;
playerObjP->info.nFlags &= ~OF_SHOULD_BE_DEAD;
//	LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
playerObjP->info.controlType = CT_NONE;
if (!gameStates.entropy.bExitSequence) {
	playerObjP->SetShield (I2X (1000));
	}
paletteManager.SetEffect (0, 0, 0);
}

//------------------------------------------------------------------------------
//eof
