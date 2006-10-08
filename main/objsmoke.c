#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>

#include "inferno.h"
#include "particles.h"
#include "laser.h"
#include "fireball.h"
#include "network.h"
#include "newdemo.h"
#include "object.h"
#include "objsmoke.h"

//------------------------------------------------------------------------------

void KillPlayerSmoke (int i)
{
KillObjectSmoke (gameData.multi.players [i].objnum);
}

//------------------------------------------------------------------------------

void ResetPlayerSmoke (void)
{
	int	i;

for (i = 0; i < MAX_PLAYERS; i++)
	KillPlayerSmoke (i);
}

//------------------------------------------------------------------------------

void InitObjectSmoke (void)
{
memset (gameData.smoke.objects, 0xff, sizeof (gameData.smoke.objects));
memset (gameData.smoke.objExplTime, 0xff, sizeof (gameData.smoke.objExplTime));
}

//------------------------------------------------------------------------------

void ResetObjectSmoke (void)
{
	int	i;

for (i = 0; i < MAX_OBJECTS; i++)
	KillObjectSmoke (i);
InitObjectSmoke ();
}

//------------------------------------------------------------------------------

static inline int RandN (int n)
{
if (!n)
	return 0;
return (int) ((double) rand () * (double) n / (double) RAND_MAX);
}

//------------------------------------------------------------------------------

void CreateDamageExplosion (int h, int i)
{
if (EGI_FLAG (bDamageExplosions, 0, 0) && 
	 (gameStates.app.nSDLTicks - gameData.smoke.objExplTime [i] > 100)) {
	gameData.smoke.objExplTime [i] = gameStates.app.nSDLTicks;
	if (!RandN (11 - h))
		CreateSmallFireballOnObject (gameData.objs.objects + i, F1_0, 1);
	}
}

//------------------------------------------------------------------------------

void DoPlayerSmoke (object *objP, int i)
{
	int			h, j, d, nParts, nType;
	float			nScale;
	tCloud		*pCloud;
	vms_vector	pos;

if (i < 0)
	i = objP->id;
if (gameData.multi.players [i].flags & PLAYER_FLAGS_CLOAKED) {
	KillObjectSmoke (i);
	return;
	}
if (EGI_FLAG (bThrusterFlames, 0, 0)) {
	fixang a = VmVecDeltaAng (&objP->orient.fvec, &objP->mtype.phys_info.thrust, NULL);
	if ((a <= F1_0 / 4) && (a || !gameStates.input.bControlsSkipFrame))	//no thruster flames if moving backward
		DropAfterburnerBlobs (objP, 2, i2f (1), -1, gameData.objs.console, 1); //F1_0 / 4);
	}
if ((gameData.app.nGameMode & GM_NETWORK) && !gameData.multi.players [i].connected)
	nParts = 0;
else if (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED))
	nParts = 0;
else if ((i == gameData.multi.nLocalPlayer) && (gameStates.app.bPlayerIsDead || (gameData.multi.players [i].shields < 0)))
	nParts = 0;
else {
	h = f2ir (gameData.multi.players [i].shields);
	nParts = 10 - h / 5;
	nScale = 655360.0f / (float) objP->size * 2;
	if (h <= 25)
		nScale /= 3;
	else if (h <= 50)
		nScale /= 2;
	else
		nScale /= 1.5;
	j = OBJ_IDX (objP);
	if (nParts <= 0) {
		nType = 2;
		//nParts = (gameStates.entropy.nTimeLastMoved < 0) ? 250 : 125;
		}
	else {
		CreateDamageExplosion (nParts, j);
		nType = (h > 25);
		nParts *= 25;
		nParts += 75;
		}
	nParts = (gameStates.entropy.nTimeLastMoved < 0) ? 250 : 125;
	if (SHOW_SMOKE && gameOpts->render.smoke.bPlayers) {
		if (0 > (h = gameData.smoke.objects [j])) {
			//LogErr ("creating player smoke\n");
			h = gameData.smoke.objects [j] = CreateSmoke (&objP->pos, objP->segnum, 2, nParts, nScale,
														  PLR_PART_LIFE / (nType + 1), PLR_PART_SPEED, nType, j);
			}
		else {
			SetSmokeType (h, nType);
			SetSmokePartScale (h, nScale);
			SetSmokeDensity (h, nParts);
			}
		d = 8 * objP->size / 40;
		for (j = 0; j < 2; j++)
			if (pCloud = GetCloud (h, j)) {
				VmVecScaleAdd (&pos, &objP->pos, &objP->orient.fvec, -objP->size);
				VmVecScaleInc (&pos, &objP->orient.rvec, j ? d : -d);
				VmVecScaleInc (&pos, &objP->orient.uvec,  -objP->size / 25);
				SetCloudPos (pCloud, &pos);
				}
		return;
		}
	}
KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoRobotSmoke (object *objP)
{
	int			h = -1, i, nShields, nParts;
	float			nScale;
	vms_vector	pos;

if (!(SHOW_SMOKE && gameOpts->render.smoke.bRobots))
	return;
i = (int) (objP - gameData.objs.objects);
if ((objP->shields < 0) || (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShields = f2ir (gameData.bots.info [gameStates.app.bD1Mission][objP->id].strength);
	h = f2ir (objP->shields) * 100 / nShields;
	}
if (h < 0)
	return;	
nParts = 10 - h / 5;
if (nParts > 0) {
	if (nShields > 4000)
		nShields = 4000;
	else if (nShields < 1000)
		nShields = 1000;
	CreateDamageExplosion (nParts, i);
	//nParts *= nShields / 10;
	nParts = 250;
	nScale = 655360.0f / (float) objP->size * 1.5f;
	if (h <= 25)
		nScale /= 3;
	else if (h <= 50)
		nScale /= 2;
	else
		nScale /= 1.5;
	if (gameData.smoke.objects [i] < 0) {
		//LogErr ("creating robot %d smoke\n", i);
		gameData.smoke.objects [i] = CreateSmoke (&objP->pos, objP->segnum, 1, nParts, nScale,
												OBJ_PART_LIFE, OBJ_PART_SPEED, 0, i);
		}
	else {
		SetSmokePartScale (gameData.smoke.objects [i], nScale);
		SetSmokeDensity (gameData.smoke.objects [i], nParts);
		}
	VmVecScaleAdd (&pos, &objP->pos, &objP->orient.fvec, -objP->size / 2);
	SetSmokePos (gameData.smoke.objects [i], &pos);
	}
else 
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoMissileSmoke (object *objP)
{
	int			nParts, i;
	vms_vector	pos;

if (!(SHOW_SMOKE && gameOpts->render.smoke.bMissiles))
	return;
i = (int) (objP - gameData.objs.objects);
if ((objP->shields < 0) || (objP->flags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else 
	nParts = (objP->id == EARTHSHAKER_ID) ? 3000 : (objP->id == MEGA_ID) ? 2000 : 1000;
if (nParts) {
	if (gameData.smoke.objects [i] < 0) {
		//LogErr ("creating missile %d smoke\n", i);
		gameData.smoke.objects [i] = CreateSmoke (&objP->pos, objP->segnum, 1, nParts / 2, 1.5,
												 MSL_PART_LIFE, MSL_PART_SPEED, 1, i);
		}
	VmVecScaleAdd (&pos, &objP->pos, &objP->orient.fvec, -objP->size);
	SetSmokePos (gameData.smoke.objects [i], &pos);
	}
else 
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoObjectSmoke (object *objP)
{
int t = objP->type;
if (extraGameInfo [0].bShadows && (gameStates.render.nShadowPass < 3))
	return;
if (t == OBJ_PLAYER)
	DoPlayerSmoke (objP, -1);
else if (t == OBJ_ROBOT)
	DoRobotSmoke (objP);
else if (t == OBJ_WEAPON) {
	int id = objP->id;
	if ((id == CONCUSSION_ID) || (id == HOMING_ID) || (id == SMART_ID) || (id == MEGA_ID) ||
		 (id == FLASH_ID) || (id == GUIDEDMISS_ID) || (id == MERCURY_ID) || (id == EARTHSHAKER_ID) ||
		 (id == EARTHSHAKER_MEGA_ID) || (id == REGULAR_MECH_MISS) || (id == SUPER_MECH_MISS) || 
		 (id == ROBOT_MERCURY_ID) || (id == ROBOT_MEGA_ID))
	DoMissileSmoke (objP);
	}
}

//------------------------------------------------------------------------------

void PlayerSmokeFrame (void)
{
	int	i;

if (!gameOpts->render.smoke.bPlayers)
	return;
for (i = 0; i < gameData.multi.nPlayers; i++)
	DoPlayerSmoke (gameData.objs.objects + gameData.multi.players [i].objnum, i);
}

//------------------------------------------------------------------------------

void RobotSmokeFrame (void)
{
	int		i;
	object	*objP;

if (!SHOW_SMOKE)
	return;
if (!gameOpts->render.smoke.bRobots)
	return;
for (i = 0, objP = gameData.objs.objects; i <= gameData.objs.nLastObject; i++, objP++)
	if (objP->type == OBJ_NONE)
		KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoSmokeFrame (void)
{
#ifdef _DEBUG
if (!gameStates.render.bExternalView)
#else
if (!gameStates.render.bExternalView && (!IsMultiGame || IsCoopGame || EGI_FLAG (bEnableCheats, 0, 0)))
#endif
	DoPlayerSmoke (gameData.objs.viewer, gameData.multi.nLocalPlayer);
RobotSmokeFrame ();
if (SHOW_SMOKE)
	MoveSmoke ();
}

//------------------------------------------------------------------------------
