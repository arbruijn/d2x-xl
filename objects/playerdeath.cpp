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
#include "gameseg.h"
#include "textures.h"
#include "lightning.h"
#include "objsmoke.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "fireball.h"
#include "error.h"
#include "endlevel.h"
#include "timer.h"
#include "gameseg.h"
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
#ifdef TACTILE
#	include "tactile.h"
#endif

//------------------------------------------------------------------------------

#define	DEATH_SEQUENCE_EXPLODE_TIME	 (I2X (2))

CObject	*viewerSaveP;
int		nPlayerFlagsSave;
fix		xCameraToPlayerDistGoal=I2X (4);
ubyte		nControlTypeSave, nRenderTypeSave;
int		nKilledInFrame = -1;
short		nKilledObjNum = -1;

extern char bMultiSuicide;

//------------------------------------------------------------------------------

//	Camera is less than size of CPlayerData away from
void SetCameraPos (CFixVector *vCameraPos, CObject *objP)
{
	CFixVector	vPlayerCameraOffs = *vCameraPos - objP->info.position.vPos;
	int			count = 0;
	fix			xCameraPlayerDist;
	fix			xFarScale;

xCameraPlayerDist = vPlayerCameraOffs.Mag();
if (xCameraPlayerDist < xCameraToPlayerDistGoal) { // 2*objP->info.xSize) {
	//	Camera is too close to CPlayerData CObject, so move it away.
	tFVIQuery	fq;
	tFVIData		hit_data;
	CFixVector	local_p1;

	if (vPlayerCameraOffs.IsZero())
		vPlayerCameraOffs[X] += I2X (1)/16;

	hit_data.hit.nType = HIT_WALL;
	xFarScale = I2X (1);

	while ((hit_data.hit.nType != HIT_NONE) && (count++ < 6)) {
		CFixVector	closer_p1;
		CFixVector::Normalize(vPlayerCameraOffs);
		vPlayerCameraOffs *= xCameraToPlayerDistGoal;

		fq.p0 = &objP->info.position.vPos;
		closer_p1 = objP->info.position.vPos + vPlayerCameraOffs;	//	This is the actual point we want to put the camera at.
		vPlayerCameraOffs *= xFarScale;				//	...but find a point 50% further away...
		local_p1 = objP->info.position.vPos + vPlayerCameraOffs;		//	...so we won't have to do as many cuts.

		fq.p1					= &local_p1;
		fq.startSeg			= objP->info.nSegment;
		fq.radP0				=
		fq.radP1				= 0;
		fq.thisObjNum		= objP->Index ();
		fq.ignoreObjList	= NULL;
		fq.flags				= 0;
		FindVectorIntersection (&fq, &hit_data);

		if (hit_data.hit.nType == HIT_NONE)
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
gameStates.app.bPlayerIsDead = 0;
gameStates.app.bPlayerExploded = 0;
if (gameData.objs.deadPlayerCamera) {
	ReleaseObject (OBJ_IDX (gameData.objs.deadPlayerCamera));
	gameData.objs.deadPlayerCamera = NULL;
	}
cockpit->Activate (gameStates.render.cockpit.nTypeSave);
gameStates.render.cockpit.nTypeSave = -1;
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

void DeadPlayerFrame (void)
{
	fix			xTimeDead, h;
	CFixVector	fVec;

if (gameStates.app.bPlayerIsDead) {
	xTimeDead = gameData.time.xGame - gameStates.app.nPlayerTimeOfDeath;

	//	If unable to create camera at time of death, create now.
	if (!gameData.objs.deadPlayerCamera) {
		CObject *playerP = OBJECTS + LOCALPLAYER.nObject;
		int nObject = CreateCamera (playerP);
		if (nObject != -1)
			gameData.objs.viewerP = gameData.objs.deadPlayerCamera = OBJECTS + nObject;
		else
			Int3 ();
		}
	h = DEATH_SEQUENCE_EXPLODE_TIME - xTimeDead;
	h = max (0, h);
	gameData.objs.consoleP->mType.physInfo.rotVel = CFixVector::Create(h / 4, h / 2, h / 3);
	xCameraToPlayerDistGoal = min (xTimeDead * 8, I2X (20)) + gameData.objs.consoleP->info.xSize;
	SetCameraPos (&gameData.objs.deadPlayerCamera->info.position.vPos, gameData.objs.consoleP);
	fVec = gameData.objs.consoleP->info.position.vPos - gameData.objs.deadPlayerCamera->info.position.vPos;
	gameData.objs.deadPlayerCamera->info.position.mOrient = CFixMatrix::CreateF(fVec);

	if (xTimeDead > DEATH_SEQUENCE_EXPLODE_TIME) {
		if (!gameStates.app.bPlayerExploded) {
		if (LOCALPLAYER.hostages.nOnBoard > 1)
			HUDInitMessage (TXT_SHIP_DESTROYED_2, LOCALPLAYER.hostages.nOnBoard);
		else if (LOCALPLAYER.hostages.nOnBoard == 1)
			HUDInitMessage (TXT_SHIP_DESTROYED_1);
		else
			HUDInitMessage (TXT_SHIP_DESTROYED_0);

#ifdef TACTILE
			if (TactileStick)
				ClearForces ();
#endif
			gameStates.app.bPlayerExploded = 1;
			if (gameData.app.nGameMode & GM_NETWORK) {
				AdjustMineSpawn ();
				MultiCapObjects ();
				}
			DropPlayerEggs (gameData.objs.consoleP);
			gameStates.app.bPlayerEggsDropped = 1;
			if (IsMultiGame)
				MultiSendPlayerExplode (MULTI_PLAYER_EXPLODE);
			gameData.objs.consoleP->ExplodeBadassPlayer ();
			//is this next line needed, given the badass call above?
			gameData.objs.consoleP->Explode (0);
			gameData.objs.consoleP->info.nFlags &= ~OF_SHOULD_BE_DEAD;		//don't really kill CPlayerData
			gameData.objs.consoleP->info.renderType = RT_NONE;				//..just make him disappear
			gameData.objs.consoleP->SetType (OBJ_GHOST);						//..and kill intersections
			LOCALPLAYER.flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
#if 0
			if (gameOpts->gameplay.bFastRespawn)
				gameStates.app.bDeathSequenceAborted = 1;
#endif
			}
		}
	else {
		if (d_rand () < gameData.time.xFrame * 4) {
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendCreateExplosion (gameData.multiplayer.nLocalPlayer);
			CreateSmallFireballOnObject (gameData.objs.consoleP, I2X (1), 1);
			}
		}
	if (gameStates.app.bDeathSequenceAborted) { //xTimeDead > DEATH_SEQUENCE_LENGTH) {
		StopPlayerMovement ();
		gameStates.app.bEnterGame = 2;
		if (!gameStates.app.bPlayerEggsDropped) {
			if (gameData.app.nGameMode & GM_NETWORK) {
				AdjustMineSpawn ();
				MultiCapObjects ();
				}
			DropPlayerEggs (gameData.objs.consoleP);
			gameStates.app.bPlayerEggsDropped = 1;
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendPlayerExplode (MULTI_PLAYER_EXPLODE);
			}
		DoPlayerDead ();		//kill_playerP ();
		}
	}
}

//	------------------------------------------------------------------------

void StartPlayerDeathSequence (CObject *playerP)
{
	int	nObject;

Assert (playerP == gameData.objs.consoleP);
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
if (!(gameData.app.nGameMode & GM_MULTI))
	HUDClearMessages ();
nKilledInFrame = gameData.app.nFrameCount;
nKilledObjNum = OBJ_IDX (playerP);
gameStates.app.bDeathSequenceAborted = 0;
if (gameData.app.nGameMode & GM_MULTI) {
	MultiSendKill (LOCALPLAYER.nObject);
//		If Hoard, increase number of orbs by 1
//    Only if you haven't killed yourself
//		This prevents cheating
	if (gameData.app.nGameMode & GM_HOARD)
		if (!bMultiSuicide)
			if (LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]<12)
				LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]++;
	}
paletteManager.SetRedEffect (40);
gameStates.app.bPlayerIsDead = 1;
#ifdef TACTILE
   if (TactileStick)
	Buffeting (70);
#endif
//LOCALPLAYER.flags &= ~ (PLAYER_FLAGS_AFTERBURNER);
playerP->mType.physInfo.rotThrust.SetZero ();
playerP->mType.physInfo.thrust.SetZero ();
gameStates.app.nPlayerTimeOfDeath = gameData.time.xGame;
nObject = CreateCamera (playerP);
viewerSaveP = gameData.objs.viewerP;
if (nObject != -1)
	gameData.objs.viewerP = gameData.objs.deadPlayerCamera = OBJECTS + nObject;
else {
	Int3 ();
	gameData.objs.deadPlayerCamera = NULL;
	}
if (gameStates.render.cockpit.nTypeSave == -1)		//if not already saved
	gameStates.render.cockpit.nTypeSave = gameStates.render.cockpit.nType;
cockpit->Activate (CM_LETTERBOX);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordLetterbox ();
nPlayerFlagsSave = playerP->info.nFlags;
nControlTypeSave = playerP->info.controlType;
nRenderTypeSave = playerP->info.renderType;
playerP->info.nFlags &= ~OF_SHOULD_BE_DEAD;
//	LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
playerP->info.controlType = CT_NONE;
if (!gameStates.entropy.bExitSequence) {
	playerP->info.xShields = I2X (1000);
	MultiSendShields ();
	}
paletteManager.SetEffect (0, 0, 0);
}

//------------------------------------------------------------------------------
//eof
