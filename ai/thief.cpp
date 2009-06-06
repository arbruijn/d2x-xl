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
#include "gameseg.h"
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
szMsg [2] = char (0 + 128);
szMsg [3] = char (0 + 128);
memcpy (szMsg + 4, "THIEF: ", 6);
szMsg [11] = char (1);
szMsg [12] = char (0 + 128);
szMsg [13] = char (63 + 128);
szMsg [14] = char (0 + 128);
HUDInitMessage (szMsg);
}

//	------------------------------------------------------------------------------------------------------
//	Choose CSegment to recreate thief in.
int ChooseThiefRecreationSegment (void)
{
	static int	nSegment = -1;
	int	nDepth, nDropDepth;

nDepth = THIEF_DEPTH;
while (nSegment == -1) {
	nSegment = PickConnectedSegment (OBJECTS + LOCALPLAYER.nObject, nDepth, &nDropDepth);
	if (nDropDepth < THIEF_DEPTH / 2)
		return (d_rand() * gameData.segs.nLastSegment) >> 15;
	if ((nSegment >= 0) && (SEGMENTS [nSegment].m_nType == SEGMENT_IS_CONTROLCEN))
		nSegment = -1;
	nDepth--;
	}

if (nSegment >= 0)
	return nSegment;
#if TRACE
console.printf (1, "Warning: Unable to find a connected CSegment for thief recreation.\n");
#endif
return (d_rand() * gameData.segs.nLastSegment) >> 15;
}

//	----------------------------------------------------------------------

void RecreateThief (CObject *objP)
{
	int			nSegment;
	CFixVector	vCenter;
	CObject*		newObjP;

nSegment = ChooseThiefRecreationSegment ();
vCenter = SEGMENTS [nSegment].Center ();
newObjP = CreateMorphRobot( &SEGMENTS[nSegment], &vCenter, objP->info.nId);
InitAIObject (OBJ_IDX (newObjP), AIB_SNIPE, -1);
gameData.thief.xReInitTime = gameData.time.xGame + I2X (10);		//	In 10 seconds, re-initialize thief.
}

//	----------------------------------------------------------------------------

void DoThiefFrame (CObject *objP)
{
	int				nObject = objP->Index ();
	tAILocalInfo*	ailp = gameData.ai.localInfo + nObject;
	fix				connectedDistance;

	if ((gameData.missions.nCurrentLevel < 0) && (gameData.thief.xReInitTime < gameData.time.xGame)) {
		if (gameData.thief.xReInitTime > gameData.time.xGame - I2X (2))
			InitThiefForLevel();
		gameData.thief.xReInitTime = 0x3f000000;
	}

	if ((gameData.ai.target.xDist > I2X (500)) && (ailp->nextActionTime > 0))
		return;

	gameData.ai.target.objP = gameData.objs.consoleP;
	if (gameStates.app.bPlayerIsDead)
		ailp->mode = AIM_THIEF_RETREAT;

	switch (ailp->mode) {
		case AIM_THIEF_WAIT:
			if (ailp->targetAwarenessType >= PA_PLAYER_COLLISION) {
				ailp->targetAwarenessType = 0;
				CreatePathToTarget (objP, 30, 1);
				ailp->mode = AIM_THIEF_ATTACK;
				ailp->nextActionTime = THIEF_ATTACK_TIME/2;
				return;
			} else if (gameData.ai.nTargetVisibility) {
				CreateNSegmentPath (objP, 15, gameData.objs.consoleP->info.nSegment);
				ailp->mode = AIM_THIEF_RETREAT;
				return;
			}

			if ((gameData.ai.target.xDist > I2X (50)) && (ailp->nextActionTime > 0))
				return;

			ailp->nextActionTime = gameData.thief.xWaitTimes[gameStates.app.nDifficultyLevel]/2;

			connectedDistance = FindConnectedDistance (objP->info.position.vPos, objP->info.nSegment, gameData.ai.target.vBelievedPos, 
																	 gameData.ai.target.nBelievedSeg, 30, WID_FLY_FLAG, 0);
			if (connectedDistance < I2X (500)) {
				CreatePathToTarget(objP, 30, 1);
				ailp->mode = AIM_THIEF_ATTACK;
				ailp->nextActionTime = THIEF_ATTACK_TIME;	//	have up to 10 seconds to find player.
			}

			break;

		case AIM_THIEF_RETREAT:
			if (ailp->nextActionTime < 0) {
				ailp->mode = AIM_THIEF_WAIT;
				ailp->nextActionTime = gameData.thief.xWaitTimes[gameStates.app.nDifficultyLevel];
			} else if ((gameData.ai.target.xDist < I2X (100)) || gameData.ai.nTargetVisibility || (ailp->targetAwarenessType >= PA_PLAYER_COLLISION)) {
				AIFollowPath(objP, gameData.ai.nTargetVisibility, gameData.ai.nTargetVisibility, &gameData.ai.target.vDir);
				if ((gameData.ai.target.xDist < I2X (100)) || (ailp->targetAwarenessType >= PA_PLAYER_COLLISION)) {
					tAIStaticInfo	*aip = &objP->cType.aiInfo;
					if (((aip->nCurPathIndex <=1) && (aip->PATH_DIR == -1)) || ((aip->nCurPathIndex >= aip->nPathLength-1) && (aip->PATH_DIR == 1))) {
						ailp->targetAwarenessType = 0;
						CreateNSegmentPath (objP, 10, gameData.objs.consoleP->info.nSegment);

						//	If path is real short, try again, allowing to go through CPlayerData's CSegment
						if (aip->nPathLength < 4) {
							CreateNSegmentPath (objP, 10, -1);
						} else if (objP->info.xShields* 4 < ROBOTINFO (objP->info.nId).strength) {
							//	If robot really low on hits, will run through CPlayerData with even longer path
							if (aip->nPathLength < 8) {
								CreateNSegmentPath(objP, 10, -1);
							}
						}

						ailp->mode = AIM_THIEF_RETREAT;
					}
				} else
					ailp->mode = AIM_THIEF_RETREAT;

			}

			break;

		//	This means the thief goes from wherever he is to the player.
		//	Note: When thief successfully steals something, his action time is forced negative and his mode is changed
		//			to retreat to get him out of attack mode.
		case AIM_THIEF_ATTACK:
			if (ailp->targetAwarenessType >= PA_PLAYER_COLLISION) {
				ailp->targetAwarenessType = 0;
				if (d_rand() > 8192) {
					CreateNSegmentPath (objP, 10, gameData.objs.consoleP->info.nSegment);
					gameData.ai.localInfo[objP->Index ()].nextActionTime = gameData.thief.xWaitTimes[gameStates.app.nDifficultyLevel]/2;
					gameData.ai.localInfo[objP->Index ()].mode = AIM_THIEF_RETREAT;
				}
			} else if (ailp->nextActionTime < 0) {
				//	This forces him to create a new path every second.
				ailp->nextActionTime = I2X (1);
				CreatePathToTarget(objP, 100, 0);
				ailp->mode = AIM_THIEF_ATTACK;
			} else {
				if (gameData.ai.nTargetVisibility && (gameData.ai.target.xDist < I2X (100))) {
					//	If the CPlayerData is close to looking at the thief, thief shall run away.
					//	No more stupid thief trying to sneak up on you when you're looking right at him!
					if (gameData.ai.target.xDist > I2X (60)) {
						fix dot = CFixVector::Dot (gameData.ai.target.vDir, OBJPOS (gameData.objs.consoleP)->mOrient.FVec ());
						if (dot < -I2X (1)/2) {	//	Looking at least towards thief, so thief will run!
							CreateNSegmentPath(objP, 10, gameData.objs.consoleP->info.nSegment);
							gameData.ai.localInfo[objP->Index ()].nextActionTime = gameData.thief.xWaitTimes[gameStates.app.nDifficultyLevel]/2;
							gameData.ai.localInfo[objP->Index ()].mode = AIM_THIEF_RETREAT;
						}
					}
					AITurnTowardsVector(&gameData.ai.target.vDir, objP, I2X (1)/4);
					MoveTowardsPlayer(objP, &gameData.ai.target.vDir);
				} else {
					tAIStaticInfo	*aip = &objP->cType.aiInfo;
					//	If path length == 0, then he will keep trying to create path, but he is probably stuck in his closet.
					if ((aip->nPathLength > 1) || ((gameData.app.nFrameCount & 0x0f) == 0)) {
						AIFollowPath(objP, gameData.ai.nTargetVisibility, gameData.ai.nTargetVisibility, &gameData.ai.target.vDir);
						ailp->mode = AIM_THIEF_ATTACK;
					}
				}
			}
			break;

		default:
#if TRACE
			console.printf (CON_DBG,"Thief mode (broken) = %d\n",ailp->mode);
#endif
			// -- Int3();	//	Oops, illegal mode for thief behavior.
			ailp->mode = AIM_THIEF_ATTACK;
			ailp->nextActionTime = I2X (1);
			break;
	}

}

//	----------------------------------------------------------------------------
//	Return true if this item (whose presence is indicated by gameData.multiplayer.players[nPlayer].flags) gets stolen.
int MaybeStealFlagItem (int nPlayer, int flagval)
{
	if (gameData.multiplayer.players[nPlayer].flags & flagval) {
		if (d_rand() < THIEF_PROBABILITY) {
			int	powerup_index=-1;
			gameData.multiplayer.players[nPlayer].flags &= (~flagval);
			switch (flagval) {
				case PLAYER_FLAGS_INVULNERABLE:
					powerup_index = POW_INVUL;
					ThiefMessage ("Invulnerability stolen!");
					break;
				case PLAYER_FLAGS_CLOAKED:
					powerup_index = POW_CLOAK;
					ThiefMessage ("Cloak stolen!");
					break;
				case PLAYER_FLAGS_FULLMAP:
					powerup_index = POW_FULL_MAP;
					ThiefMessage ("Full map stolen!");
					break;
				case PLAYER_FLAGS_QUAD_LASERS:
					powerup_index = POW_QUADLASER;
					ThiefMessage ("Quad lasers stolen!");
					break;
				case PLAYER_FLAGS_AFTERBURNER:
					powerup_index = POW_AFTERBURNER;
					ThiefMessage ("Afterburner stolen!");
					break;
// --				case PLAYER_FLAGS_AMMO_RACK:
// --					powerup_index = POW_AMMORACK;
// --					ThiefMessage ("Ammo Rack stolen!");
// --					break;
				case PLAYER_FLAGS_CONVERTER:
					powerup_index = POW_CONVERTER;
					ThiefMessage ("Converter stolen!");
					break;
				case PLAYER_FLAGS_HEADLIGHT:
					powerup_index = POW_HEADLIGHT;
					ThiefMessage ("Headlight stolen!");
					if (!EGI_FLAG (headlight.bBuiltIn, 0, 1, 0))
				   	LOCALPLAYER.flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
					break;
			}
			Assert(powerup_index != -1);
			gameData.thief.stolenItems[gameData.thief.nStolenItem] = powerup_index;

			audio.PlaySound (SOUND_WEAPON_STOLEN);
			return 1;
		}
	}

	return 0;
}

//	----------------------------------------------------------------------------
int MaybeStealSecondaryWeapon (int nPlayer, int nWeapon)
{
if ((gameData.multiplayer.players[nPlayer].secondaryWeaponFlags & HAS_FLAG(nWeapon)) && gameData.multiplayer.players[nPlayer].secondaryAmmo[nWeapon])
	if (d_rand() < THIEF_PROBABILITY) {
		if (nWeapon == PROXMINE_INDEX)
			if (d_rand() > 8192)		//	Come in groups of 4, only add 1/4 of time.
				return 0;
		gameData.multiplayer.players[nPlayer].secondaryAmmo[nWeapon]--;
		//	Smart mines and proxbombs don't get dropped because they only come in 4 packs.
		if ((nWeapon != PROXMINE_INDEX) && (nWeapon != SMARTMINE_INDEX)) {
			gameData.thief.stolenItems[gameData.thief.nStolenItem] = secondaryWeaponToPowerup[nWeapon];
			}
		ThiefMessage (TXT_WPN_STOLEN, baseGameTexts [114+nWeapon][0]);		//	Danger! Danger! Use of literal!  Danger!
		if (LOCALPLAYER.secondaryAmmo[nWeapon] == 0)
			AutoSelectWeapon (1, 0);
		// -- compress_stolen_items();
		audio.PlaySound (SOUND_WEAPON_STOLEN);
		return 1;
		}
return 0;
}

//	----------------------------------------------------------------------------
int MaybeStealPrimaryWeapon (int nPlayer, int nWeapon)
{
	if ((gameData.multiplayer.players[nPlayer].primaryWeaponFlags & HAS_FLAG(nWeapon)) && gameData.multiplayer.players[nPlayer].primaryAmmo[nWeapon]) {
		if (d_rand() < THIEF_PROBABILITY) {
			if (nWeapon == 0) {
				if (gameData.multiplayer.players[nPlayer].laserLevel > 0) {
					if (gameData.multiplayer.players[nPlayer].laserLevel > 3) {
						gameData.thief.stolenItems[gameData.thief.nStolenItem] = POW_SUPERLASER;
					} else {
						gameData.thief.stolenItems[gameData.thief.nStolenItem] = primaryWeaponToPowerup[nWeapon];
					}
					ThiefMessage (TXT_LVL_DECREASED, baseGameTexts [104+nWeapon][0]);		//	Danger! Danger! Use of literal!  Danger!
					gameData.multiplayer.players[nPlayer].laserLevel--;
					audio.PlaySound(SOUND_WEAPON_STOLEN);
					return 1;
				}
			} else if (gameData.multiplayer.players[nPlayer].primaryWeaponFlags & (1 << nWeapon)) {
				gameData.multiplayer.players[nPlayer].primaryWeaponFlags &= ~(1 << nWeapon);
				gameData.thief.stolenItems[gameData.thief.nStolenItem] = primaryWeaponToPowerup[nWeapon];

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
//	If a wapon successfully stolen, do everything, removing it from CPlayerData,
//	updating gameData.thief.stolenItems information, deselecting, etc.
int AttemptToStealItem3(CObject *objP, int nPlayer)
{
	int	i;

	if (gameData.ai.localInfo[objP->Index ()].mode != AIM_THIEF_ATTACK)
		return 0;

	//	First, try to steal equipped items.

	if (MaybeStealFlagItem (nPlayer, PLAYER_FLAGS_INVULNERABLE))
		return 1;

	//	If primary weapon = laser, first try to rip away those nasty quad lasers!
	if (gameData.weapons.nPrimary == 0)
		if (MaybeStealFlagItem (nPlayer, PLAYER_FLAGS_QUAD_LASERS))
			return 1;

	//	Makes it more likely to steal primary than secondary.
	for (i=0; i<2; i++)
		if (MaybeStealPrimaryWeapon (nPlayer, gameData.weapons.nPrimary))
			return 1;

	if (MaybeStealSecondaryWeapon (nPlayer, gameData.weapons.nSecondary))
		return 1;

	//	See what the CPlayerData has and try to snag something.
	//	Try best things first.
	if (MaybeStealFlagItem (nPlayer, PLAYER_FLAGS_INVULNERABLE))
		return 1;
	if (MaybeStealFlagItem (nPlayer, PLAYER_FLAGS_CLOAKED))
		return 1;
	if (MaybeStealFlagItem (nPlayer, PLAYER_FLAGS_QUAD_LASERS))
		return 1;
	if (MaybeStealFlagItem (nPlayer, PLAYER_FLAGS_AFTERBURNER))
		return 1;
	if (MaybeStealFlagItem (nPlayer, PLAYER_FLAGS_CONVERTER))
		return 1;
// --	if (MaybeStealFlagItem (nPlayer, PLAYER_FLAGS_AMMO_RACK))	//	Can't steal because what if have too many items, say 15 homing missiles?
// --		return 1;
	if (MaybeStealFlagItem (nPlayer, PLAYER_FLAGS_HEADLIGHT))
		return 1;
	if (MaybeStealFlagItem (nPlayer, PLAYER_FLAGS_FULLMAP))
		return 1;

	for (i = MAX_SECONDARY_WEAPONS - 1; i >= 0; i--) {
		if (MaybeStealPrimaryWeapon (nPlayer, i))
			return 1;
		if (MaybeStealSecondaryWeapon (nPlayer, i))
			return 1;
	}

	return 0;
}

//	----------------------------------------------------------------------------
int AttemptToStealItem2(CObject *objP, int nPlayer)
{
	int	rval;

	rval = AttemptToStealItem3(objP, nPlayer);

	if (rval) {
		gameData.thief.nStolenItem = (gameData.thief.nStolenItem+1) % MAX_STOLEN_ITEMS;
		if (d_rand() > 20000)	//	Occasionally, boost the value again
			gameData.thief.nStolenItem = (gameData.thief.nStolenItem+1) % MAX_STOLEN_ITEMS;
	}

	return rval;
}

//	----------------------------------------------------------------------------
//	Called for a thief-nType robot.
//	If a item successfully stolen, returns true, else returns false.
//	If a wapon successfully stolen, do everything, removing it from CPlayerData,
//	updating gameData.thief.stolenItems information, deselecting, etc.
int AttemptToStealItem (CObject *objP, int nPlayer)
{
	int	i;
	int	rval = 0;

	if (objP->cType.aiInfo.xDyingStartTime)
		return 0;

	rval += AttemptToStealItem2(objP, nPlayer);

	for (i=0; i<3; i++) {
		if (!rval || (d_rand() < 11000)) {	//	about 1/3 of time, steal another item
			rval += AttemptToStealItem2(objP, nPlayer);
		} else
			break;
	}
	CreateNSegmentPath(objP, 10, gameData.objs.consoleP->info.nSegment);
	gameData.ai.localInfo[objP->Index ()].nextActionTime = gameData.thief.xWaitTimes[gameStates.app.nDifficultyLevel]/2;
	gameData.ai.localInfo[objP->Index ()].mode = AIM_THIEF_RETREAT;
	if (rval) {
		paletteManager.BumpEffect(30, 15, -20);
		cockpit->UpdateLaserWeaponInfo();
//		audio.CreateSegmentSound( SOUND_NASTY_ROBOT_HIT_1, objP->info.nSegment, 0, &objP->info.position.vPos, 0 , DEFAULT_ROBOT_SOUND_VOLUME);
//	I removed this to make the "steal sound" more obvious -AP
                if (gameData.app.nGameMode & GM_NETWORK)
                 MultiSendStolenItems();
	}
	return rval;
}

// --------------------------------------------------------------------------------------------------------------
//	Indicate no items have been stolen.
void InitThiefForLevel(void)
{
	int	i;

#if 1
gameData.thief.stolenItems.Clear (char (0xff));
#else
	for (i=0; i<MAX_STOLEN_ITEMS; i++)
		gameData.thief.stolenItems[i] = 255;
#endif
	Assert (MAX_STOLEN_ITEMS >= 3*2);	//	Oops!  Loop below will overwrite memory!
   if (!(gameData.app.nGameMode & GM_MULTI))
		for (i=0; i<3; i++) {
			gameData.thief.stolenItems[2*i] = POW_SHIELD_BOOST;
			gameData.thief.stolenItems[2*i+1] = POW_ENERGY;
		}

	gameData.thief.nStolenItem = 0;
}

// --------------------------------------------------------------------------------------------------------------

void DropStolenItems(CObject *objP)
{
	int	i;
#if TRACE
        console.printf (CON_DBG,"Dropping thief items!\n");
#endif
	// -- compress_stolen_items();

	for (i=0; i<MAX_STOLEN_ITEMS; i++) {
		if (gameData.thief.stolenItems[i] != 255)
			DropPowerup(OBJ_POWERUP, gameData.thief.stolenItems[i], -1, 1, objP->mType.physInfo.velocity, objP->info.position.vPos, objP->info.nSegment);
		gameData.thief.stolenItems[i] = 255;
	}

}

// --------------------------------------------------------------------------------------------------------------
