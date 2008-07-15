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

#include "inferno.h"
#include "error.h"
#include "text.h"
#include "interp.h"
#include "gameseg.h"
#include "fireball.h"
#include "objeffects.h"
#include "monsterball.h"
#include "dropobject.h"

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
	vmsVector	vSegCenter;

RemoveMonsterball ();
#ifdef _DEBUG
nDropSeg = gameData.hoard.nMonsterballSeg;
#else
nDropSeg = (gameData.hoard.nMonsterballSeg >= 0) ? 
			  gameData.hoard.nMonsterballSeg : ChooseDropSegment (NULL, NULL, EXEC_DROP);
#endif
if (nDropSeg >= 0) {
	COMPUTE_SEGMENT_CENTER_I (&vSegCenter, nDropSeg);
	nObject = DropPowerup (OBJ_POWERUP, POW_MONSTERBALL, -1, 1, &vZero, &vSegCenter, nDropSeg);
	if (nObject >= 0) {
		gameData.hoard.monsterballP = OBJECTS + nObject;
		gameData.hoard.monsterballP->nType = OBJ_MONSTERBALL;
		gameData.hoard.monsterballP->mType.physInfo.mass = F1_0 * 10;
		gameData.hoard.nLastHitter = -1;
		CreatePlayerAppearanceEffect (gameData.hoard.monsterballP);
		return 1;
		}
	}
#ifndef _DEBUG
Warning (TXT_NO_MONSTERBALL);
#endif
gameData.app.nGameMode &= ~GM_MONSTERBALL;
return 0;
}

//------------------------------------------------------------------------------

int FindMonsterball (void)
{
	short		i;
	tObject	*objP;

gameData.hoard.monsterballP = NULL;
gameData.hoard.nMonsterballSeg = -1;
gameData.hoard.nLastHitter = -1;
for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject [0]; i++, objP++)
	if ((objP->nType == OBJ_MONSTERBALL) || ((objP->nType == OBJ_POWERUP) && (objP->id == POW_MONSTERBALL))) {
		if (gameData.hoard.nMonsterballSeg < 0)
			gameData.hoard.nMonsterballSeg = objP->nSegment;
		ReleaseObject (i);
		}
#ifndef _DEBUG
if (!(NetworkIAmMaster () && IsMultiGame && (gameData.app.nGameMode & GM_MONSTERBALL)))
	return 0;
#endif
if (!CreateMonsterball ())
	return 0;
MultiSendMonsterball (1, 1);
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
special = gameData.segs.segment2s [gameData.hoard.monsterballP->nSegment].special;
if ((special != SEGMENT_IS_GOAL_BLUE) && (special != SEGMENT_IS_GOAL_RED))
	return 0;
if ((GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_RED) == (special == SEGMENT_IS_GOAL_RED))
	MultiSendCaptureBonus (gameData.multiplayer.nLocalPlayer);
else
	MultiSendCaptureBonus (-gameData.multiplayer.nLocalPlayer - 1);
CreatePlayerAppearanceEffect (gameData.hoard.monsterballP);
RemoveMonsterball ();
CreateMonsterball ();
MultiSendMonsterball (1, 1);
return 1;
}

//------------------------------------------------------------------------------
//eof
