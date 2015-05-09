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

#include <stdio.h>		// for printf()
#include <stdlib.h>		// for rand() and qsort()
#include <string.h>		// for memset()

#include "descent.h"
#include "error.h"
#include "text.h"
#include "network.h"
#include "findpath.h"
#include "segmath.h"
#include "cockpit.h"
#include "dropobject.h"

extern void MultiSendStolenItems();

#define	THIEF_ATTACK_TIME		(I2X (10))

//	-------------------------------------------------------------------------------------------------

#define	THIEF_DEPTH	20

//	-----------------------------------------------------------------------------
void _CDECL_ ThiefMessage (const char * format, ... )
{
	char	szMsg [128];
	va_list	args;

va_start (args, format);
vsprintf (szMsg + 15, format, args);
va_end (args);

szMsg [0] = char (1);
szMsg [1] = char (127 + 128);
szMsg [2] = char (128);
szMsg [3] = char (128);
memcpy (szMsg + 4, "THIEF: ", 7);
szMsg [11] = char (1);
szMsg [12] = char (128);
szMsg [13] = char (127 + 128);
szMsg [14] = char (128);
HUDInitMessage (szMsg);
}

//	------------------------------------------------------------------------------------------------------
//	Choose CSegment to recreate thief in.
int32_t ChooseThiefRecreationSegment (void)
{
	static int32_t	nSegment = -1;
	int32_t	nDepth, nDropDepth;

nDepth = THIEF_DEPTH;
while (nSegment == -1) {
	nSegment = PickConnectedSegment (OBJECT (LOCALPLAYER.nObject), nDepth, &nDropDepth);
	if (nDropDepth < THIEF_DEPTH / 2)
		return (RandShort () * gameData.segData.nLastSegment) >> 15;
	if ((nSegment >= 0) && (SEGMENT (nSegment)->m_function == SEGMENT_FUNC_REACTOR))
		nSegment = -1;
	nDepth--;
	}

if (nSegment >= 0)
	return nSegment;
#if TRACE
console.printf (1, "Warning: Unable to find a connected CSegment for thief recreation.\n");
#endif
return (RandShort () * gameData.segData.nLastSegment) >> 15;
}

//	----------------------------------------------------------------------

void RecreateThief (CObject *pObj)
{
	int32_t			nSegment;
	CFixVector	vCenter;
	CObject*		pNewObj;

nSegment = ChooseThiefRecreationSegment ();
vCenter = SEGMENT (nSegment)->Center ();
pNewObj = CreateMorphRobot( SEGMENT (nSegment), &vCenter, pObj->info.nId);
InitAIObject (OBJ_IDX (pNewObj), AIB_SNIPE, -1);
gameData.thief.xReInitTime = gameData.time.xGame + I2X (10);		//	In 10 seconds, re-initialize thief.
}

//	----------------------------------------------------------------------------

void DoThiefFrame (CObject *pObj)
{
	int32_t			nObject = pObj->Index ();
	tAILocalInfo*	pLocalInfo = gameData.ai.localInfo + nObject;
	fix				connectedDistance;

if ((missionManager.nCurrentLevel < 0) && (gameData.thief.xReInitTime < gameData.time.xGame)) {
	if (gameData.thief.xReInitTime > gameData.time.xGame - I2X (2))
		InitThiefForLevel();
	gameData.thief.xReInitTime = 0x3f000000;
	}

if ((gameData.ai.target.xDist > I2X (500)) && (pLocalInfo->nextActionTime > 0))
	return;
if (pObj->Disarmed () || pObj->Reprogrammed ())
	return;

gameData.ai.target.pObj = gameData.objData.pConsole;
if (gameStates.app.bPlayerIsDead)
	pLocalInfo->mode = AIM_THIEF_RETREAT;

switch (pLocalInfo->mode) {
	case AIM_THIEF_WAIT:
		if (pLocalInfo->targetAwarenessType >= PA_PLAYER_COLLISION) {
			pLocalInfo->targetAwarenessType = 0;
			CreatePathToTarget (pObj, 30, 1);
			pLocalInfo->mode = AIM_THIEF_ATTACK;
			pLocalInfo->nextActionTime = THIEF_ATTACK_TIME/2;
			return;
			}
		if (gameData.ai.nTargetVisibility) {
			CreateNSegmentPath (pObj, 15, gameData.objData.pConsole->info.nSegment);
			pLocalInfo->mode = AIM_THIEF_RETREAT;
			return;
			}
		if ((gameData.ai.target.xDist > I2X (50)) && (pLocalInfo->nextActionTime > 0))
			return;
		pLocalInfo->nextActionTime = gameData.thief.xWaitTimes [gameStates.app.nDifficultyLevel] / 2;
		connectedDistance = simpleRouter [0].PathLength (pObj->info.position.vPos, pObj->info.nSegment, 
																		 gameData.ai.target.vBelievedPos, gameData.ai.target.nBelievedSeg, 
																		 30, WID_PASSABLE_FLAG, 1);
		if (connectedDistance < I2X (500)) {
			CreatePathToTarget (pObj, 30, 1);
			pLocalInfo->mode = AIM_THIEF_ATTACK;
			pLocalInfo->nextActionTime = THIEF_ATTACK_TIME;	//	have up to 10 seconds to find player.
		}
		break;

	case AIM_THIEF_RETREAT:
		if (pLocalInfo->nextActionTime < 0) {
			pLocalInfo->mode = AIM_THIEF_WAIT;
			pLocalInfo->nextActionTime = gameData.thief.xWaitTimes [gameStates.app.nDifficultyLevel];
			}
		else if ((gameData.ai.target.xDist < I2X (100)) || gameData.ai.nTargetVisibility || (pLocalInfo->targetAwarenessType >= PA_PLAYER_COLLISION)) {
			AIFollowPath (pObj, gameData.ai.nTargetVisibility, gameData.ai.nTargetVisibility, &gameData.ai.target.vDir);
			if ((gameData.ai.target.xDist < I2X (100)) || (pLocalInfo->targetAwarenessType >= PA_PLAYER_COLLISION)) {
				tAIStaticInfo* pStaticInfo = &pObj->cType.aiInfo;
				if (((pStaticInfo->nCurPathIndex <= 1) && (pStaticInfo->PATH_DIR == -1)) || ((pStaticInfo->nCurPathIndex >= pStaticInfo->nPathLength-1) && (pStaticInfo->PATH_DIR == 1))) {
					pLocalInfo->targetAwarenessType = 0;
					CreateNSegmentPath (pObj, 10, gameData.objData.pConsole->info.nSegment);

					//	If path is real int16_t, try again, allowing to go through player's CSegment
					if (pStaticInfo->nPathLength < 4) {
						CreateNSegmentPath (pObj, 10, -1);
						}
					else if (ROBOTINFO (pObj) && (pObj->info.xShield * 4 < ROBOTINFO (pObj)->strength)) {
						//	If robot really low on hits, will run through player with even longer path
						if (pStaticInfo->nPathLength < 8) {
							CreateNSegmentPath (pObj, 10, -1);
						}
					}
				pLocalInfo->mode = AIM_THIEF_RETREAT;
				}
			} 
		else
			pLocalInfo->mode = AIM_THIEF_RETREAT;
		}

		break;

	//	This means the thief goes from wherever he is to the player.
	//	Note: When thief successfully steals something, his action time is forced negative and his mode is changed
	//			to retreat to get him out of attack mode.
	case AIM_THIEF_ATTACK:
		if (pLocalInfo->targetAwarenessType >= PA_PLAYER_COLLISION) {
			pLocalInfo->targetAwarenessType = 0;
			if (RandShort () > 8192) {
				CreateNSegmentPath (pObj, 10, gameData.objData.pConsole->info.nSegment);
				gameData.ai.localInfo [pObj->Index ()].nextActionTime = gameData.thief.xWaitTimes [gameStates.app.nDifficultyLevel] / 2;
				gameData.ai.localInfo [pObj->Index ()].mode = AIM_THIEF_RETREAT;
				}
			} 
		else if (pLocalInfo->nextActionTime < 0) {
			//	This forces him to create a new path every second.
			pLocalInfo->nextActionTime = I2X (1);
			CreatePathToTarget(pObj, 100, 0);
			pLocalInfo->mode = AIM_THIEF_ATTACK;
			}
		else {
			if (gameData.ai.nTargetVisibility && (gameData.ai.target.xDist < I2X (100))) {
				//	If the player is close to looking at the thief, thief shall run away.
				//	No more stupid thief trying to sneak up on you when you're looking right at him!
				if (gameData.ai.target.xDist > I2X (60)) {
					fix dot = CFixVector::Dot (gameData.ai.target.vDir, OBJPOS (gameData.objData.pConsole)->mOrient.m.dir.f);
					if (dot < -I2X (1)/2) {	//	Looking at least towards thief, so thief will run!
						CreateNSegmentPath (pObj, 10, gameData.objData.pConsole->info.nSegment);
						gameData.ai.localInfo [pObj->Index ()].nextActionTime = gameData.thief.xWaitTimes [gameStates.app.nDifficultyLevel] / 2;
						gameData.ai.localInfo [pObj->Index ()].mode = AIM_THIEF_RETREAT;
						}
					}
				AITurnTowardsVector (&gameData.ai.target.vDir, pObj, I2X (1)/4);
				MoveTowardsPlayer (pObj, &gameData.ai.target.vDir);
				}
			else {
				tAIStaticInfo	*pStaticInfo = &pObj->cType.aiInfo;
				//	If path length == 0, then he will keep trying to create path, but he is probably stuck in his closet.
				if ((pStaticInfo->nPathLength > 1) || ((gameData.app.nFrameCount & 0x0f) == 0)) {
					AIFollowPath (pObj, gameData.ai.nTargetVisibility, gameData.ai.nTargetVisibility, &gameData.ai.target.vDir);
					pLocalInfo->mode = AIM_THIEF_ATTACK;
					}
				}
			}
		break;

	default:
#if TRACE
		console.printf (CON_DBG,"Thief mode (broken) = %d\n",pLocalInfo->mode);
#endif
		// -- Int3();	//	Oops, illegal mode for thief behavior.
		pLocalInfo->mode = AIM_THIEF_ATTACK;
		pLocalInfo->nextActionTime = I2X (1);
		break;
	}

}

//	----------------------------------------------------------------------------
//	Return true if this item (whose presence is indicated by PLAYER (nPlayer).flags) gets stolen.
int32_t MaybeStealDevice (int32_t nPlayer, int32_t deviceFlag)
{
if (extraGameInfo [IsMultiGame].loadout.nDevice & deviceFlag)
	return 0;

if (PLAYER (nPlayer).flags & deviceFlag) {
	if (RandShort () < THIEF_PROBABILITY) {
		int32_t nPowerup = -1;
		PLAYER (nPlayer).flags &= (~deviceFlag);
		switch (deviceFlag) {
			case PLAYER_FLAGS_INVULNERABLE:
				nPowerup = POW_INVUL;
				ThiefMessage ("Invulnerability stolen!");
				break;
			case PLAYER_FLAGS_CLOAKED:
				nPowerup = POW_CLOAK;
				ThiefMessage ("Cloak stolen!");
				break;
			case PLAYER_FLAGS_FULLMAP:
				nPowerup = POW_FULL_MAP;
				ThiefMessage ("Full map stolen!");
				break;
			case PLAYER_FLAGS_QUAD_LASERS:
				nPowerup = POW_QUADLASER;
				ThiefMessage ("Quad lasers stolen!");
				break;
			case PLAYER_FLAGS_AFTERBURNER:
				nPowerup = POW_AFTERBURNER;
				ThiefMessage ("Afterburner stolen!");
				break;
// --				case PLAYER_FLAGS_AMMO_RACK:
// --					nPowerup = POW_AMMORACK;
// --					ThiefMessage ("Ammo Rack stolen!");
// --					break;
			case PLAYER_FLAGS_CONVERTER:
				nPowerup = POW_CONVERTER;
				ThiefMessage ("Converter stolen!");
				break;
			case PLAYER_FLAGS_HEADLIGHT:
				nPowerup = POW_HEADLIGHT;
				ThiefMessage ("Headlight stolen!");
		   	LOCALPLAYER.flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
				break;
			}
		gameData.thief.stolenItems [gameData.thief.nStolenItem] = nPowerup;
		audio.PlaySound (SOUND_WEAPON_STOLEN);
		return 1;
		}
	}
return 0;
}

//	----------------------------------------------------------------------------

int32_t MaybeStealSecondaryWeapon (int32_t nPlayer, int32_t nWeapon)
{
if ((PLAYER (nPlayer).secondaryWeaponFlags & HAS_FLAG(nWeapon)) && PLAYER (nPlayer).secondaryAmmo [nWeapon])
	if (RandShort () < THIEF_PROBABILITY) {
		if (nWeapon == PROXMINE_INDEX)
			if (RandShort () > 8192)		//	Come in groups of 4, only add 1/4 of time.
				return 0;
		PLAYER (nPlayer).secondaryAmmo [nWeapon]--;
		//	Smart mines and proxbombs don't get dropped because they only come in 4 packs.
		if ((nWeapon != PROXMINE_INDEX) && (nWeapon != SMARTMINE_INDEX)) {
			gameData.thief.stolenItems [gameData.thief.nStolenItem] = secondaryWeaponToPowerup [0][nWeapon];
			}
		ThiefMessage (TXT_WPN_STOLEN, baseGameTexts [114+nWeapon][0]);		//	Danger! Danger! Use of literal!  Danger!
		if (LOCALPLAYER.secondaryAmmo [nWeapon] == 0)
			AutoSelectWeapon (1, 0);
		// -- compress_stolen_items();
		audio.PlaySound (SOUND_WEAPON_STOLEN);
		return 1;
		}
return 0;
}

//	----------------------------------------------------------------------------

int32_t MaybeStealPrimaryWeapon (int32_t nPlayer, int32_t nWeapon)
{
if ((PLAYER (nPlayer).primaryWeaponFlags & HAS_FLAG (nWeapon)) && 
	 PLAYER (nPlayer).primaryAmmo [nWeapon] &&
	 !(extraGameInfo [IsMultiGame].loadout.nGuns & HAS_FLAG (nWeapon))) {
	if (RandShort () < THIEF_PROBABILITY) {
		if (nWeapon == 0) {
			if (PLAYER (nPlayer).laserLevel > 0) {
				if (PLAYER (nPlayer).laserLevel > 3) {
					gameData.thief.stolenItems [gameData.thief.nStolenItem] = POW_SUPERLASER;
				} 
				else {
					gameData.thief.stolenItems [gameData.thief.nStolenItem] = primaryWeaponToPowerup [nWeapon];
					}
				ThiefMessage (TXT_LVL_DECREASED, baseGameTexts [104+nWeapon][0]);		//	Danger! Danger! Use of literal!  Danger!
				PLAYER (nPlayer).laserLevel--;
				audio.PlaySound(SOUND_WEAPON_STOLEN);
				return 1;
				}
			} 
		else if (PLAYER (nPlayer).primaryWeaponFlags & (1 << nWeapon)) {
			PLAYER (nPlayer).primaryWeaponFlags &= ~(1 << nWeapon);
			gameData.thief.stolenItems [gameData.thief.nStolenItem] = primaryWeaponToPowerup [nWeapon];
			ThiefMessage (TXT_WPN_STOLEN, baseGameTexts [104+nWeapon][0]);		//	Danger! Danger! Use of literal!  Danger!
			AutoSelectWeapon (0, 0);
			audio.PlaySound(SOUND_WEAPON_STOLEN);
			return 1;
			}
		}
	}
return 0;
}

//	----------------------------------------------------------------------------
//	Called for a thief-nType robot.
//	If a item successfully stolen, returns true, else returns false.
//	If a wapon successfully stolen, do everything, removing it from player,
//	updating gameData.thief.stolenItems information, deselecting, etc.
int32_t AttemptToStealItem3(CObject *pObj, int32_t nPlayer)
{
	int32_t i;
	static int32_t nDevices [] = {PLAYER_FLAGS_INVULNERABLE, PLAYER_FLAGS_CLOAKED, PLAYER_FLAGS_QUAD_LASERS, PLAYER_FLAGS_AFTERBURNER, 
											PLAYER_FLAGS_CONVERTER, PLAYER_FLAGS_AMMO_RACK, PLAYER_FLAGS_HEADLIGHT, PLAYER_FLAGS_FULLMAP, -1};

if (gameData.ai.localInfo [pObj->Index ()].mode != AIM_THIEF_ATTACK)
	return 0;
//	First, try to steal equipped items.
if (MaybeStealDevice (nPlayer, PLAYER_FLAGS_INVULNERABLE))
	return 1;
//	If primary weapon = laser, first try to rip away those nasty quad lasers!
if (gameData.weapons.nPrimary == 0)
	if (MaybeStealDevice (nPlayer, PLAYER_FLAGS_QUAD_LASERS))
		return 1;
//	Makes it more likely to steal primary than secondary.
for (i = 0; i < 2; i++)
	if (MaybeStealPrimaryWeapon (nPlayer, gameData.weapons.nPrimary))
		return 1;
if (MaybeStealSecondaryWeapon (nPlayer, gameData.weapons.nSecondary))
	return 1;
//	See what the player has and try to snag something.
//	Try best things first.

for (i = 0; nDevices [i] > 0; i++) {
	if (nDevices [i] == PLAYER_FLAGS_AMMO_RACK) {
		if (!gameStates.app.bD2XLevel || (gameOpts->gameplay.nShip [0] == 2)) // Wolf, has the ammo rack built in
			continue;
		}
	if (MaybeStealDevice (nPlayer, nDevices [i])) {
		if (nDevices [i] == PLAYER_FLAGS_AMMO_RACK) 
			DropExcessAmmo ();
		return 1;
		}
	}
// --	if (MaybeStealDevice (nPlayer, PLAYER_FLAGS_AMMO_RACK))	//	Can't steal because what if have too many items, say 15 homing missiles?
// --		return 1;

for (i = MAX_SECONDARY_WEAPONS - 1; i >= 0; i--) {
	if (MaybeStealPrimaryWeapon (nPlayer, i))
		return 1;
	if (MaybeStealSecondaryWeapon (nPlayer, i))
		return 1;
	}
return 0;
}

//	----------------------------------------------------------------------------

int32_t AttemptToStealItem2(CObject *pObj, int32_t nPlayer)
{
int32_t rval = AttemptToStealItem3 (pObj, nPlayer);
if (rval) {
	gameData.thief.nStolenItem = (gameData.thief.nStolenItem + 1) % MAX_STOLEN_ITEMS;
	if (RandShort () > 20000)	//	Occasionally, boost the value again
		gameData.thief.nStolenItem = (gameData.thief.nStolenItem + 1) % MAX_STOLEN_ITEMS;
	}
return rval;
}

//	----------------------------------------------------------------------------
//	Called for a thief-nType robot.
//	If a item successfully stolen, returns true, else returns false.
//	If a wapon successfully stolen, do everything, removing it from player,
//	updating gameData.thief.stolenItems information, deselecting, etc.
int32_t AttemptToStealItem (CObject *pObj, int32_t nPlayer)
{
	int32_t	i;
	int32_t	rval = 0;

if (pObj->cType.aiInfo.xDyingStartTime)
	return 0;
rval += AttemptToStealItem2 (pObj, nPlayer);
for (i = 0; i < 3; i++) {
	if (rval && (RandShort () >= 11000)) 
		break;
	//	about 1/3 of time, steal another item
	rval += AttemptToStealItem2 (pObj, nPlayer);
	} 
CreateNSegmentPath (pObj, 10, gameData.objData.pConsole->info.nSegment);
gameData.ai.localInfo [pObj->Index ()].nextActionTime = gameData.thief.xWaitTimes [gameStates.app.nDifficultyLevel] / 2;
gameData.ai.localInfo [pObj->Index ()].mode = AIM_THIEF_RETREAT;
if (rval) {
	paletteManager.BumpEffect (30, 15, -20);
	cockpit->UpdateLaserWeaponInfo ();
   if (IsNetworkGame)
		MultiSendStolenItems ();
	}
return rval;
}

// --------------------------------------------------------------------------------------------------------------
//	Indicate no items have been stolen.
void InitThiefForLevel(void)
{
gameData.thief.stolenItems.Clear (char (0xff));
Assert (MAX_STOLEN_ITEMS >= 3*2);	//	Oops!  Loop below will overwrite memory!
if (!IsMultiGame)
	for (int32_t i = 0; i < 3; i++) {
		gameData.thief.stolenItems [2 * i] = POW_SHIELD_BOOST;
		gameData.thief.stolenItems [2 * i + 1] = POW_ENERGY;
		}
gameData.thief.nStolenItem = 0;
}

// --------------------------------------------------------------------------------------------------------------

void DropStolenItems (CObject *pObj)
{
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return;

for (int32_t i = 0; i < MAX_STOLEN_ITEMS; i++) 
	if (gameData.thief.stolenItems [i] != 255) {
		DropPowerup (OBJ_POWERUP, gameData.thief.stolenItems [i], -1, 0, pObj->mType.physInfo.velocity, pObj->info.position.vPos, pObj->info.nSegment);
		gameData.thief.stolenItems [i] = 255;
		}
}

// --------------------------------------------------------------------------------------------------------------
