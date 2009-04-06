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

#include "descent.h"
#include "error.h"
#include "text.h"
#include "interp.h"
#include "gameseg.h"
#include "fireball.h"
#include "objeffects.h"
#include "monsterball.h"
#include "dropobject.h"
#include "network.h"

//------------------------------------------------------------------------------

void RemoveMonsterball (void)
{
if (gameData.hoard.monsterballP) {
	ReleaseObject (OBJ_IDX (gameData.hoard.monsterballP));
	gameData.hoard.monsterballP = NULL;
	}
}

//------------------------------------------------------------------------------

int CreateMonsterball (void)
{
	short			nDropSeg, nObject;

RemoveMonsterball ();
if (!IsMultiGame && (gameData.app.nGameMode & GM_MONSTERBALL))
	return 0;
#if 0 //DBG
nDropSeg = gameData.hoard.nMonsterballSeg;
#else
if (gameData.hoard.nMonsterballSeg >= 0)
	nDropSeg = gameData.hoard.nMonsterballSeg;
else {
	nDropSeg = ChooseDropSegment (NULL, NULL, EXEC_DROP);
	gameData.hoard.vMonsterballPos = SEGMENTS [nDropSeg].Center ();
	}
#endif
if (nDropSeg >= 0) {
	ResetMonsterball (false);
	nObject = DropPowerup (OBJ_POWERUP, POW_MONSTERBALL, -1, 1, CFixVector::ZERO, gameData.hoard.vMonsterballPos, nDropSeg);
	if (nObject >= 0) {
		gameData.render.monsterball.SetupPulse (0.005f, 0.9f);
		gameData.render.monsterball.SetPulse (gameData.render.monsterball.Pulse ());
		gameData.hoard.monsterballP = OBJECTS + nObject;
		gameData.hoard.monsterballP->SetType (OBJ_MONSTERBALL);
		gameData.hoard.monsterballP->mType.physInfo.mass = I2X (10);
		gameData.hoard.monsterballP->mType.physInfo.thrust.SetZero ();
		gameData.hoard.monsterballP->mType.physInfo.rotThrust.SetZero ();
		gameData.hoard.monsterballP->mType.physInfo.velocity.SetZero ();
		gameData.hoard.nLastHitter = -1;
		gameData.hoard.monsterballP->CreateAppearanceEffect ();
		return 1;
		}
	}
#if !DBG
Warning (TXT_NO_MONSTERBALL);
#endif
if (NetworkIAmMaster ())
	gameData.app.nGameMode &= ~GM_MONSTERBALL;
return 0;
}

//------------------------------------------------------------------------------

CObject* FindMonsterball (void)
{
if (!gameData.hoard.monsterballP) {
	CObject	*objP;

	FORALL_STATIC_OBJS (objP, i)
		if ((objP->info.nType == OBJ_MONSTERBALL) || ((objP->info.nType == OBJ_POWERUP) && (objP->info.nId == POW_MONSTERBALL))) {
			gameData.hoard.monsterballP = objP;
			break;
			}
	}
return gameData.hoard.monsterballP;
}

//------------------------------------------------------------------------------

int ResetMonsterball (bool bCreate)
{
	//short		i;
	CObject	*objP;

if (gameData.hoard.monsterballP) {
	//ReleaseObject (gameData.hoard.monsterballP->Index ());
	gameData.hoard.monsterballP = NULL;
	}	
gameData.hoard.nMonsterballSeg = -1;
gameData.hoard.nLastHitter = -1;
FORALL_STATIC_OBJS (objP, i)
	if ((objP->info.nType == OBJ_MONSTERBALL) || ((objP->info.nType == OBJ_POWERUP) && (objP->info.nId == POW_MONSTERBALL))) {
		if (gameData.hoard.nMonsterballSeg < 0) {
			gameData.hoard.nMonsterballSeg = objP->info.nSegment;
			gameData.hoard.vMonsterballPos = OBJPOS (objP)->vPos;
			}
		ReleaseObject (objP->Index ());
		}
#if 1 //!DBG
if (!NetworkIAmMaster ())
	return 0;
#endif
if (bCreate) {
	if (!CreateMonsterball ())
		return 0;
	MultiSendMonsterball (1, 1);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CheckMonsterballScore (void)
{
	ubyte	special;

if (!(gameData.app.nGameMode & GM_MONSTERBALL))
	return 0;
if (!gameData.hoard.monsterballP)
	return 0;
if (gameData.hoard.nLastHitter != LOCALPLAYER.nObject)
	return 0;
special = SEGMENTS [gameData.hoard.monsterballP->info.nSegment].m_nType;
if ((special != SEGMENT_IS_GOAL_BLUE) && (special != SEGMENT_IS_GOAL_RED))
	return 0;
if ((GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_RED) == (special == SEGMENT_IS_GOAL_RED))
	MultiSendCaptureBonus (-gameData.multiplayer.nLocalPlayer - 1);
else
	MultiSendCaptureBonus (gameData.multiplayer.nLocalPlayer);
gameData.hoard.monsterballP->CreateAppearanceEffect ();
RemoveMonsterball ();
CreateMonsterball ();
MultiSendMonsterball (1, 1);
return 1;
}

//	-----------------------------------------------------------------------------

short nMonsterballForces [100];

short nMonsterballPyroForce;

void SetMonsterballForces (void)
{
	int	i;
	tMonsterballForce *forceP = extraGameInfo [IsMultiGame].monsterball.forces;

memset (nMonsterballForces, 0, sizeof (nMonsterballForces));
for (i = 0; i < MAX_MONSTERBALL_FORCES - 1; i++, forceP++)
	nMonsterballForces [forceP->nWeaponId] = 	forceP->nForce;
nMonsterballPyroForce = forceP->nForce;
gameData.objs.pwrUp.info [POW_MONSTERBALL].size =
	(gameData.objs.pwrUp.info [POW_SHIELD_BOOST].size * extraGameInfo [IsMultiGame].monsterball.nSizeMod) / 2;
}

//------------------------------------------------------------------------------
//eof
