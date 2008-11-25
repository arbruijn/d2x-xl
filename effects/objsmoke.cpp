#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "inferno.h"
#include "error.h"
#include "timer.h"
#include "u_mem.h"
#include "interp.h"
#include "lightning.h"
#include "network.h"
#include "render.h"
#include "objeffects.h"
#include "objrender.h"
#include "objsmoke.h"
#include "automap.h"

static tRgbaColorf smokeColors [3] = {
	{1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 2.0f},
	{2.0f / 3.0f, 2.0f / 3.0f, 2.0f / 3.0f, 2.0f},
	{1.0f, 1.0f, 1.0f, 2.0f}
	};

//------------------------------------------------------------------------------

#if DBG

void KillObjectSmoke (int nObject)
{
if ((nObject >= 0) && (gameData.smoke.objects [nObject] >= 0)) {
	DigiKillSoundLinkedToObject (nObject);
	SetSmokeLife (gameData.smoke.objects [nObject], 0);
	SetSmokeObject (nObject, -1);
	}
}

#endif

//------------------------------------------------------------------------------

void KillPlayerSmoke (int i)
{
KillObjectSmoke (gameData.multiplayer.players [i].nObject);
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
memset (gameData.smoke.objects, 0xff, sizeof (*gameData.smoke.objects) * MAX_OBJECTS);
memset (gameData.smoke.objExplTime, 0xff, sizeof (*gameData.smoke.objExplTime) * MAX_OBJECTS);
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
if (EGI_FLAG (bDamageExplosions, 1, 0, 0) &&
	 (gameStates.app.nSDLTicks - gameData.smoke.objExplTime [i] > 100)) {
	gameData.smoke.objExplTime [i] = gameStates.app.nSDLTicks;
	if (!RandN (11 - h))
		CreateSmallFireballOnObject (OBJECTS + i, F1_0, 1);
	}
}

//------------------------------------------------------------------------------
#if 0
void CreateThrusterFlames (tObject *objP)
{
	static int nThrusters = -1;

	vmsVector	pos, dir = objP->info.position.mOrient[FVEC];
	int			d, j;
	tCloud		*pCloud;

VmVecNegate (&dir);
if (nThrusters < 0) {
	nThrusters =
		CreateSmoke (&objP->info.position.vPos, &dir, objP->info.nSegment, 2, -2000, 20000,
						 gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [1],
						 2, -2000, PLR_PART_SPEED * 50, LIGHT_PARTICLES, OBJ_IDX (objP), NULL, 1, -1);
	SetSmokeObject (OBJ_IDX (objP)) = nThrusters;
	}
else
	SetSmokeDir (nThrusters, &dir);
d = 8 * objP->info.xSize / 40;
for (j = 0; j < 2; j++)
	if (pCloud = GetCloud (nThrusters, j)) {
		VmVecScaleAdd (&pos, &objP->info.position.vPos, &objP->info.position.mOrient[FVEC], -objP->info.xSize);
		VmVecScaleInc (&pos, &objP->info.position.mOrient[RVEC], j ? d : -d);
		VmVecScaleInc (&pos, &objP->info.position.mOrient[UVEC],  -objP->info.xSize / 25);
		SetCloudPos (pCloud, &pos, NULL, objP->info.nSegment);
		}
}
#endif

//------------------------------------------------------------------------------

void KillPlayerBullets (tObject *objP)
{
	int	i = gameData.multiplayer.bulletEmitters [objP->info.nId];

if (i >= 0) {
	SetSmokeLife (i, 0);
	gameData.multiplayer.bulletEmitters [objP->info.nId] = -1;
	}
}

//------------------------------------------------------------------------------

void KillGatlingSmoke (tObject *objP)
{
	int	i = gameData.multiplayer.gatlingSmoke [objP->info.nId];

if (i >= 0) {
	SetSmokeLife (i, 0);
	gameData.multiplayer.gatlingSmoke [objP->info.nId] = -1;
	}
}

//------------------------------------------------------------------------------

#define BULLET_MAX_PARTS	50
#define BULLET_PART_LIFE	-2000
#define BULLET_PART_SPEED	50

void DoPlayerBullets (tObject *objP)
{
if (RENDERPATH && gameOpts->render.ship.bBullets) {
		int	nModel = objP->rType.polyObjInfo.nModel;
		int	bHires = G3HaveModel (nModel) - 1;

	if (bHires >= 0) {
			tG3Model	*pm = gameData.models.g3Models [bHires] + nModel;

		if (pm->bBullets) {
				int			nPlayer = objP->info.nId;
				int			nGun = EquippedPlayerGun (objP);
				int			bDoEffect = (bHires >= 0) && ((nGun == VULCAN_INDEX) || (nGun == GAUSS_INDEX)) &&
												(gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration >= GATLING_DELAY);
				int			i = gameData.multiplayer.bulletEmitters [nPlayer];

			if (bDoEffect) {
					int			bSpectate = SPECTATOR (objP);
					tTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
					vmsVector	vEmitter, vDir;
					vmsMatrix	m, *viewP;

				if (bSpectate) {
					viewP = &m;
					m = posP->mOrient.Transpose();
				}
				else
					viewP = ObjectView (objP);
				vEmitter = *viewP * pm->vBullets;
				vEmitter += posP->vPos;
				vDir = posP->mOrient[UVEC];
				vDir.Neg();
				if (i < 0) {
					gameData.multiplayer.bulletEmitters [nPlayer] =
						CreateSmoke (&vEmitter, &vDir, &posP->mOrient, objP->info.nSegment, 1, BULLET_MAX_PARTS, 15.0f, 1,
										 1, BULLET_PART_LIFE, BULLET_PART_SPEED, BULLET_PARTICLES, 0x7fffffff, NULL, 0, -1);
					}
				else {
					SetSmokePos (i, &vEmitter, &posP->mOrient, objP->info.nSegment);
					}
				}
			else {
				if (i >= 0) {
					SetSmokeLife (i, 0);
					gameData.multiplayer.bulletEmitters [nPlayer] = -1;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

#define GATLING_MAX_PARTS	25
#define GATLING_PART_LIFE	-1000
#define GATLING_PART_SPEED	30

void DoGatlingSmoke (tObject *objP)
{
	int	nModel = objP->rType.polyObjInfo.nModel;
	int	bHires = G3HaveModel (nModel) - 1;

if (bHires >= 0) {
		tG3Model	*pm = gameData.models.g3Models [bHires] + nModel;

	if (pm->bBullets) {
			int			nPlayer = objP->info.nId;
			int			nGun = EquippedPlayerGun (objP);
			int			bDoEffect = (bHires >= 0) && ((nGun == VULCAN_INDEX) || (nGun == GAUSS_INDEX)) &&
											(gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration >= GATLING_DELAY);
			int			i = gameData.multiplayer.gatlingSmoke [nPlayer];

		if (bDoEffect) {
				int			bSpectate = SPECTATOR (objP);
				tTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
				vmsVector	*vGunPoints, vEmitter, vDir;
				vmsMatrix	m, *viewP;

			if (!(vGunPoints = GetGunPoints (objP, nGun)))
				return;
			if (bSpectate) {
				viewP = &m;
				*viewP = posP->mOrient.Transpose();
			}

			else
				viewP = ObjectView (objP);
			vEmitter = *viewP * vGunPoints[nGun];
			vEmitter += posP->vPos;
			//vDir = posP->mOrient[FVEC];
			vDir = posP->mOrient[FVEC] * (F1_0 / 8);
			if (i < 0) {
				gameData.multiplayer.gatlingSmoke [nPlayer] =
					CreateSmoke (&vEmitter, &vDir, &posP->mOrient, objP->info.nSegment, 1, GATLING_MAX_PARTS, F1_0 / 2, 1,
									 1, GATLING_PART_LIFE, GATLING_PART_SPEED, SMOKE_PARTICLES, 0x7ffffffe, smokeColors + 1, 0, -1);
				}
			else {
				SetSmokePos (i, &vEmitter, &posP->mOrient, objP->info.nSegment);
				}
			}
		else {
			if (i >= 0) {
				SetSmokeLife (i, 0);
				gameData.multiplayer.gatlingSmoke [nPlayer] = -1;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void DoPlayerSmoke (tObject *objP, int i)
{
	int				h, j, d, nParts, nType;
	float				nScale;
	tCloud			*pCloud;
	vmsVector		fn, mn, vDir, *vDirP;
	tThrusterInfo	ti;

	static int	bForward = 1;

if (i < 0)
	i = objP->info.nId;
if ((gameData.multiplayer.players [i].flags & PLAYER_FLAGS_CLOAKED) ||
	 (gameStates.render.automap.bDisplay && IsMultiGame && !AM_SHOW_PLAYERS)) {
	KillObjectSmoke (i);
	return;
	}
j = OBJ_IDX (objP);
if (gameOpts->render.particles.bDecreaseLag && (i == gameData.multiplayer.nLocalPlayer)) {
	fn = objP->info.position.mOrient[FVEC];
	mn = objP->info.position.vPos - objP->info.vLastPos;
	vmsVector::Normalize(fn);
	vmsVector::Normalize(mn);
	d = vmsVector::Dot(fn, mn);
	if (d >= -F1_0 / 2)
		bForward = 1;
	else {
		if (bForward) {
			if ((h = gameData.smoke.objects [j]) >= 0) {
				KillObjectSmoke (j);
				DestroySmoke (h);
				}
			bForward = 0;
			nScale = 0;
			return;
			}
		}
	}
#if 0
if (EGI_FLAG (bThrusterFlames, 1, 1, 0)) {
	if ((a <= F1_0 / 4) && (a || !gameStates.input.bControlsSkipFrame))	//no thruster flames if moving backward
		DropAfterburnerBlobs (objP, 2, I2X (1), -1, gameData.objs.consoleP, 1); //F1_0 / 4);
	}
#endif
if ((gameData.app.nGameMode & GM_NETWORK) && !gameData.multiplayer.players [i].connected)
	nParts = 0;
else if (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED))
	nParts = 0;
else if ((i == gameData.multiplayer.nLocalPlayer) && (gameStates.app.bPlayerIsDead || (gameData.multiplayer.players [i].shields < 0)))
	nParts = 0;
else {
	h = X2IR (gameData.multiplayer.players [i].shields);
	nParts = 10 - h / 5;
	nScale = X2F (objP->info.xSize);
	if (h <= 25)
		nScale /= 1.5;
	else if (h <= 50)
		nScale /= 2;
	else
		nScale /= 3;
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
	nParts = objP->mType.physInfo.thrust.IsZero() ? SHIP_MAX_PARTS : SHIP_MAX_PARTS / 2;
	if (SHOW_SMOKE && nParts && gameOpts->render.particles.bPlayers) {
		if (gameOpts->render.particles.bSyncSizes) {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [0]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [0], nScale);
			}
		else {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [1]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [1], nScale);
			}
		if (!objP->mType.physInfo.thrust.IsZero ())
			vDirP = NULL;
		else {	// if the ship is standing still, let the thruster smoke move away from it
			nParts /= 2;
			nScale /= 2;
			vDir = OBJPOS (objP)->mOrient [FVEC] * (F1_0 / 8);
			vDir = -vDir;
			vDirP = &vDir;
			}
		if (0 > (h = gameData.smoke.objects [j])) {
			//PrintLog ("creating tPlayer smoke\n");
			h = SetSmokeObject (j,
					CreateSmoke (&objP->info.position.vPos, vDirP, NULL, objP->info.nSegment, 2, nParts, nScale,
									 gameOpts->render.particles.nSize [1],
									 2, PLR_PART_LIFE / (nType + 1) * (vDirP ? 2 : 1), PLR_PART_SPEED, SMOKE_PARTICLES, j, smokeColors + nType, 1, -1));
			}
		else {
			if (vDirP)
				SetSmokeDir (h, vDirP);
			SetSmokeLife (h, PLR_PART_LIFE / (nType + 1) * (vDirP ? 2 : 1));
			SetSmokeType (h, SMOKE_PARTICLES);
			SetSmokePartScale (h, -nScale);
			SetSmokeDensity (h, nParts, gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [1]);
			SetSmokeSpeed (gameData.smoke.objects [i],
								objP->mType.physInfo.thrust.IsZero () ? PLR_PART_SPEED * 2 : PLR_PART_SPEED);
			}
		CalcThrusterPos (objP, &ti, 0);
		for (j = 0; j < 2; j++)
			if ((pCloud = GetCloud (h, j)))
				SetCloudPos (pCloud, ti.vPos + j, NULL, objP->info.nSegment);
		DoGatlingSmoke (objP);
		return;
		}
	}
KillObjectSmoke (i);
KillGatlingSmoke (objP);
}

//------------------------------------------------------------------------------

void DoRobotSmoke (tObject *objP)
{
	int			h = -1, i, nShields = 0, nParts;
	float			nScale;
	vmsVector	pos;

i = OBJ_IDX (objP);
if (!(SHOW_SMOKE && gameOpts->render.particles.bRobots)) {
	if (gameData.smoke.objects [i] >= 0)
		KillObjectSmoke (i);
	return;
	}
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShields = X2IR (RobotDefaultShields (objP));
	h = X2IR (objP->info.xShields) * 100 / nShields;
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
	nParts = BOT_MAX_PARTS;
	nScale = (float) sqrt (8.0 / X2F (objP->info.xSize));
	nScale *= 1.0f + h / 25.0f;
	if (!gameOpts->render.particles.bSyncSizes) {
		nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [2]);
		nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [2], nScale);
		}
	if (gameData.smoke.objects [i] < 0) {
		//PrintLog ("creating robot %d smoke\n", i);
		SetSmokeObject (i, CreateSmoke (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, nScale,
												  gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [2],
												  1, BOT_PART_LIFE, BOT_PART_SPEED, SMOKE_PARTICLES, i, smokeColors, 1, -1));
		}
	else {
		SetSmokePartScale (gameData.smoke.objects [i], nScale);
		SetSmokeDensity (gameData.smoke.objects [i], nParts, gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [2]);
		SetSmokeSpeed (gameData.smoke.objects [i], !objP->mType.physInfo.velocity.IsZero() ?
							BOT_PART_SPEED : BOT_PART_SPEED * 2 / 3);
		}
	pos = objP->info.position.vPos + objP->info.position.mOrient[FVEC] * (-objP->info.xSize / 2);
	SetSmokePos (gameData.smoke.objects [i], &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoReactorSmoke (tObject *objP)
{
	int			h = -1, i, nShields = 0, nParts;
	vmsVector	vDir, vPos;

i = OBJ_IDX (objP);
if (!(SHOW_SMOKE && gameOpts->render.particles.bRobots)) {
	if (gameData.smoke.objects [i] >= 0)
		KillObjectSmoke (i);
	return;
	}
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShields = X2IR (gameData.bots.info [gameStates.app.bD1Mission][objP->info.nId].strength);
	h = nShields ? X2IR (objP->info.xShields) * 100 / nShields : 0;
	}
if (h < 0)
	h = 0;
nParts = 10 - h / 10;
if (nParts > 0) {
	nParts = REACTOR_MAX_PARTS;
	if (gameData.smoke.objects [i] < 0) {
		//PrintLog ("creating robot %d smoke\n", i);
		SetSmokeObject (i, CreateSmoke (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, F2X (-4.0),
												  -1, 1, BOT_PART_LIFE * 2, BOT_PART_SPEED, SMOKE_PARTICLES, i, smokeColors, 1, -1));
		}
	else {
		SetSmokePartScale (gameData.smoke.objects [i], F2X (-4.0));
		SetSmokeDensity (gameData.smoke.objects [i], nParts, -1);
		vDir[X] = d_rand () - F1_0 / 4;
		vDir[Y] = d_rand () - F1_0 / 4;
		vDir[Z] = d_rand () - F1_0 / 4;
		vmsVector::Normalize(vDir);
		vPos = objP->info.position.vPos + vDir * (-objP->info.xSize / 2);
		SetSmokePos (gameData.smoke.objects [i], &vPos, NULL, objP->info.nSegment);
		}
	}
else
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoMissileSmoke (tObject *objP)
{
	int				nParts, nSpeed, nLife, i;
	float				nScale = 1.5f;
	tThrusterInfo	ti;

i = OBJ_IDX (objP);
if (!(SHOW_SMOKE && gameOpts->render.particles.bMissiles)) {
	if (gameData.smoke.objects [i] >= 0)
		KillObjectSmoke (i);
	return;
	}
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nSpeed = WI_speed (objP->info.nId, gameStates.app.nDifficultyLevel);
	nLife = gameOpts->render.particles.nLife [3] + 1;
#if 1
	nParts = (int) (MSL_MAX_PARTS * X2F (nSpeed) / (40.0f * (4 - nLife)));
	if ((objP->info.nId == EARTHSHAKER_MEGA_ID) || (objP->info.nId == ROBOT_SHAKER_MEGA_ID))
		nParts /= 2;

#else
	nParts = (objP->info.nId == EARTHSHAKER_ID) ? 1500 :
				(objP->info.nId == MEGAMSL_ID) ? 1400 :
				(objP->info.nId == SMARTMSL_ID) ? 1300 :
				1200;
#endif
	}
if (nParts) {
	if (gameData.smoke.objects [i] < 0) {
		if (!gameOpts->render.particles.bSyncSizes) {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [3]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [3], nScale);
			}
		SetSmokeObject (i, CreateSmoke (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, nScale,
												  gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [3],
												  1, nLife * MSL_PART_LIFE, MSL_PART_SPEED, SMOKE_PARTICLES, i, smokeColors + 1, 1, -1));
		}
	CalcThrusterPos (objP, &ti, 0);
	SetSmokePos (gameData.smoke.objects [i], ti.vPos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoDebrisSmoke (tObject *objP)
{
	int			nParts, i;
	float			nScale = 2;
	vmsVector	pos;

i = OBJ_IDX (objP);
if (!(SHOW_SMOKE && gameOpts->render.particles.bDebris)) {
	if (gameData.smoke.objects [i] >= 0)
		KillObjectSmoke (i);
	return;
	}
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)) ||
	 (gameOpts->render.nDebrisLife && (nDebrisLife [gameOpts->render.nDebrisLife] * F1_0 - objP->info.xLifeLeft > 10 * F1_0)))
	nParts = 0;
else
	nParts = DEBRIS_MAX_PARTS;
if (nParts) {
	if (!gameOpts->render.particles.bSyncSizes) {
		nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [4]);
		nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [4], nScale);
		}
	if (gameData.smoke.objects [i] < 0) {
		SetSmokeObject (i, CreateSmoke (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts / 2,
												  nScale, -1, 1, DEBRIS_PART_LIFE, DEBRIS_PART_SPEED, SMOKE_PARTICLES, i, smokeColors, 0, -1));
		}
	pos = objP->info.position.vPos + objP->info.position.mOrient[FVEC] * (-objP->info.xSize);
	SetSmokePos (gameData.smoke.objects [i], &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoStaticSmoke (tObject *objP)
{
	int			i, j, bBubbles = objP->rType.smokeInfo.nType == SMOKE_TYPE_BUBBLES;
	vmsVector	pos, offs, dir;

	static tRgbaColorf defaultColors [2] = {{0.5f, 0.5f, 0.5f, 0.0f}, {0.8f, 0.9f, 1.0f, 1.0f}};

i = (int) OBJ_IDX (objP);
if (!(SHOW_SMOKE && (bBubbles ? gameOpts->render.particles.bBubbles : gameOpts->render.particles.bStatic))) {
	if (gameData.smoke.objects [i] >= 0)
		KillObjectSmoke (i);
	return;
	}
if (gameData.smoke.objects [i] < 0) {
		tRgbaColorf color;
		int bColor;

	color.red = (float) objP->rType.smokeInfo.color.red / 255.0f;
	color.green = (float) objP->rType.smokeInfo.color.green / 255.0f;
	color.blue = (float) objP->rType.smokeInfo.color.blue / 255.0f;
	if ((bColor = (color.red + color.green + color.blue > 0)))
		color.alpha = (float) -objP->rType.smokeInfo.color.alpha / 255.0f;
	dir = objP->info.position.mOrient [FVEC] * (objP->rType.smokeInfo.nSpeed * 2 * F1_0 / 55);
	SetSmokeObject (i, CreateSmoke (&objP->info.position.vPos, &dir, &objP->info.position.mOrient,
											  objP->info.nSegment, 1, -objP->rType.smokeInfo.nParts,
											  -PARTICLE_SIZE (objP->rType.smokeInfo.nSize [gameOpts->render.particles.bDisperse], bBubbles ? 4.0f : 2.0f),
											  -1, 3, STATIC_SMOKE_PART_LIFE * objP->rType.smokeInfo.nLife,
											  objP->rType.smokeInfo.nDrift, bBubbles ? BUBBLE_PARTICLES : SMOKE_PARTICLES, 
											  i, bColor ? &color : defaultColors + bBubbles, 1, objP->rType.smokeInfo.nSide - 1));
	if (bBubbles)
		DigiSetObjectSound (i, -1, AddonSoundName (SND_ADDON_AIRBUBBLES), F1_0 / 2);
	else
		SetSmokeBrightness (gameData.smoke.objects [i], objP->rType.smokeInfo.nBrightness);
	}
if (objP->rType.smokeInfo.nSide <= 0) {	//don't vary emitter position for smoke emitting faces
	i = objP->rType.smokeInfo.nDrift >> 4;
	i += objP->rType.smokeInfo.nSize [0] >> 2;
	i /= 2;
	if (!(j = i - i / 2))
		j = 2;
	i /= 2;
	offs [X] = (F1_0 / 4 - d_rand ()) * (d_rand () % j + i);
	offs [Y] = (F1_0 / 4 - d_rand ()) * (d_rand () % j + i);
	offs [Z] = (F1_0 / 4 - d_rand ()) * (d_rand () % j + i);
	pos = objP->info.position.vPos + offs;
	SetSmokePos (gameData.smoke.objects [i], &pos, NULL, objP->info.nSegment);
	}
}

//------------------------------------------------------------------------------

void DoBombSmoke (tObject *objP)
{
	int			nParts, i;
	vmsVector	pos, offs;

if (gameStates.app.bNostalgia || !gameStates.app.bHaveExtraGameInfo [IsMultiGame])
	return;
i = OBJ_IDX (objP);
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else
	nParts = -BOMB_MAX_PARTS;
if (nParts) {
	if (gameData.smoke.objects [i] < 0) {
		SetSmokeObject (i, CreateSmoke (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts,
												  -PARTICLE_SIZE (3, 0.5f), -1, 3, BOMB_PART_LIFE, BOMB_PART_SPEED, SMOKE_PARTICLES, i, NULL, 1, -1));
		}
	offs [X] = (F1_0 / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	offs [Y] = (F1_0 / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	offs [Z] = (F1_0 / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	pos = objP->info.position.vPos + offs;
	SetSmokePos (gameData.smoke.objects [i], &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoParticleTrail (tObject *objP)
{
	int			nParts, i, id = objP->info.nId, bGatling = (id == VULCAN_ID) || (id == GAUSS_ID);
	float			nScale;
	vmsVector	pos;
	tRgbaColorf	c;

if (!(SHOW_OBJ_FX && (bGatling ? EGI_FLAG (bGatlingTrails, 1, 1, 0) : EGI_FLAG (bLightTrails, 1, 1, 0))))
	return;
i = OBJ_IDX (objP);
if (!(bGatling || gameOpts->render.particles.bPlasmaTrails)) {
	if (gameData.smoke.objects [i] >= 0)
		KillObjectSmoke (i);
	return;
	}
#if 1
nParts = 2 * LASER_MAX_PARTS / 3;
#else
nParts = gameData.weapons.info [objP->info.nId].speed [0] / F1_0;
#endif
if (bGatling) {
	c.red = c.green = c.blue = 2.0f / 3.0f;
	c.alpha = 1.0f / 4.0f;
	}
else {
	c.red = (float) gameData.weapons.color [objP->info.nId].red;
	c.green = (float) gameData.weapons.color [objP->info.nId].green;
	c.blue = (float) gameData.weapons.color [objP->info.nId].blue;
	c.alpha = 0.5f;
	}
if (gameData.smoke.objects [i] < 0) {
	if (bGatling)
		nScale = 5.0f;
	else {
		if (((id >= LASER_ID) && (id < LASER_ID + 4)) ||
			 (id == SUPERLASER_ID) || (id == SUPERLASER_ID + 1) ||
			 (id == ROBOT_BLUE_LASER_ID) || (id == ROBOT_GREEN_LASER_ID) || (id == ROBOT_RED_LASER_ID) || (id == ROBOT_WHITE_LASER_ID))
			nScale = 3;
		else if ((id == PHOENIX_ID) || (id == ROBOT_LIGHT_FIREBALL_ID) || (id == ROBOT_FAST_PHOENIX_ID))
			nScale = 1;
		else if ((id == PLASMA_ID) || (id == ROBOT_PLASMA_ID))
			nScale = 1.5;
		else if (id == FUSION_ID)
			nScale = 2;
		else if ((id == SPREADFIRE_ID) || (id == HELIX_ID) || (id == ROBOT_HELIX_ID))
			nScale = 2;
		else if (id == FLARE_ID)
			nScale = 2;
		else if ((id == ROBOT_BLUE_ENERGY_ID) || (id == ROBOT_WHITE_ENERGY_ID) || (id == ROBOT_PHASE_ENERGY_ID))
			nScale = 2;
		else
			nScale = 1;
		c.alpha = 0.1f + nScale / 10;
		}
	SetSmokeObject (i, CreateSmoke (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts << bGatling, -PARTICLE_SIZE (1, nScale),
											  gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [3],
											  1, ((gameOpts->render.particles.nLife [3] + 1) * LASER_PART_LIFE) << bGatling, LASER_PART_SPEED, 
											  bGatling ? GATLING_PARTICLES : LIGHT_PARTICLES, i, &c, 0, -1));
	}
pos = objP->info.position.vPos + objP->info.position.mOrient[FVEC] * (-objP->info.xSize / 2);
SetSmokePos (gameData.smoke.objects [i], &pos, NULL, objP->info.nSegment);
}

// -----------------------------------------------------------------------------

#define SHRAPNEL_MAX_PARTS			500
#define SHRAPNEL_PART_LIFE			-1750
#define SHRAPNEL_PART_SPEED		10

static float fShrapnelScale [5] = {0, 5.0f / 3.0f, 2.5f, 10.0f / 3.0f, 5};

int CreateShrapnels (tObject *parentObjP)
{
if (!SHOW_SMOKE)
	return 0;
if (!gameOpts->render.effects.nShrapnels)
	return 0;
if (parentObjP->info.nFlags & OF_ARMAGEDDON)
	return 0;
if ((parentObjP->info.nType != OBJ_PLAYER) && (parentObjP->info.nType != OBJ_ROBOT))
	return 0;

	tShrapnelData	*sdP;
	tShrapnel		*shrapnelP;
	vmsVector		vDir;
	int				i, h = (int) (X2F (parentObjP->info.xSize) * fShrapnelScale [gameOpts->render.effects.nShrapnels] + 0.5);
	short				nObject;
	tObject			*objP;
	tRgbaColorf		color = {1,1,1,0.5};

nObject = CreateFireball (0, parentObjP->info.nSegment, parentObjP->info.position.vPos, 1, RT_SHRAPNELS);
if (nObject < 0)
	return 0;
objP = OBJECTS + nObject;
objP->info.xLifeLeft = 0;
objP->cType.explInfo.nSpawnTime = -1;
objP->cType.explInfo.nDeleteObj = -1;
objP->cType.explInfo.nDeleteTime = -1;
sdP = gameData.objs.shrapnels + nObject;
h += d_rand () % h;
if (!(sdP->shrapnels = (tShrapnel *) D2_ALLOC (h * sizeof (tShrapnel))))
	return 0;
sdP->nShrapnels = h;
srand (gameStates.app.nSDLTicks);
for (i = 0, shrapnelP = sdP->shrapnels; i < h; i++, shrapnelP++) {
	if (i & 1) {
		vDir[X] = -FixMul (vDir[X], F1_0 / 2 + d_rand ()) | 1;
		vDir[Y] = -FixMul (vDir[Y], F1_0 / 2 + d_rand ());
		vDir[Z] = -FixMul (vDir[Z], F1_0 / 2 + d_rand ());
		vmsVector::Normalize(vDir);
		}
	else
		vDir = vmsVector::Random();
	shrapnelP->vDir = vDir;
	shrapnelP->vPos = parentObjP->info.position.vPos + vDir * (parentObjP->info.xSize / 4 + rand () % (parentObjP->info.xSize / 2));
	shrapnelP->nTurn = 1;
	shrapnelP->xSpeed = 3 * (F1_0 / 20 + rand () % (F1_0 / 20)) / 4;
	shrapnelP->xLife =
	shrapnelP->xTTL = 3 * F1_0 / 2 + rand ();
	shrapnelP->tUpdate = gameStates.app.nSDLTicks;
	if (objP->info.xLifeLeft < shrapnelP->xLife)
		objP->info.xLifeLeft = shrapnelP->xLife;
	shrapnelP->nSmoke = CreateSmoke (&shrapnelP->vPos, NULL, NULL, objP->info.nSegment, 1, -SHRAPNEL_MAX_PARTS,
											   -PARTICLE_SIZE (1, 4), -1, 1, SHRAPNEL_PART_LIFE , SHRAPNEL_PART_SPEED, SMOKE_PARTICLES, 0x7fffffff, &color, 1, -1);
	}
objP->info.xLifeLeft *= 2;
objP->cType.explInfo.nSpawnTime = -1;
objP->cType.explInfo.nDeleteObj = -1;
objP->cType.explInfo.nDeleteTime = -1;
return 1;
}

// -----------------------------------------------------------------------------

void DestroyShrapnels (tObject *objP)
{
	tShrapnelData	*sdP = gameData.objs.shrapnels + OBJ_IDX (objP);

if (sdP->shrapnels) {
	int	i, h = sdP->nShrapnels;

	sdP->nShrapnels = 0;
	for (i = 0; i < h; i++)
		if (sdP->shrapnels [i].nSmoke >= 0)
			SetSmokeLife (sdP->shrapnels [i].nSmoke, 0);
	D2_FREE (sdP->shrapnels);
	sdP->shrapnels = 0;
	}
objP->info.xLifeLeft = -1;
}

// -----------------------------------------------------------------------------

void MoveShrapnel (tShrapnel *shrapnelP)
{
	fix			xSpeed = FixDiv (shrapnelP->xSpeed, 25 * F1_0 / 1000);
	vmsVector	vOffs;
	time_t		nTicks;

if ((nTicks = gameStates.app.nSDLTicks - shrapnelP->tUpdate) < 25)
	return;
xSpeed = (fix) (xSpeed / gameStates.gameplay.slowmo [0].fSpeed);
for (; nTicks >= 25; nTicks -= 25) {
	if (--(shrapnelP->nTurn))
		vOffs = shrapnelP->vOffs;
	else {
		shrapnelP->nTurn = ((shrapnelP->xTTL > F1_0 / 2) ? 2 : 4) + d_rand () % 4;
		vOffs = shrapnelP->vDir;
		vOffs[X] = FixMul (vOffs[X], 2 * d_rand ());
		vOffs[Y] = FixMul (vOffs[Y], 2 * d_rand ());
		vOffs[Z] = FixMul (vOffs[Z], 2 * d_rand ());
		vmsVector::Normalize(vOffs);
		shrapnelP->vOffs = vOffs;
		}
	vOffs *= xSpeed;
	shrapnelP->vPos += vOffs;
	}
SetSmokePos (shrapnelP->nSmoke, &shrapnelP->vPos, NULL, -1);
shrapnelP->tUpdate = gameStates.app.nSDLTicks - nTicks;
}

// -----------------------------------------------------------------------------

void DrawShrapnel (tShrapnel *shrapnelP)
{
if ((shrapnelP->xTTL > 0) && LoadExplBlast ()) {
	fix	xSize = F1_0 / 2 + d_rand () % (F1_0 / 4);
	G3DrawSprite (shrapnelP->vPos, xSize, xSize, bmpExplBlast, NULL, X2F (shrapnelP->xTTL) / X2F (shrapnelP->xLife) / 2, 0, 0);
	}
}

// -----------------------------------------------------------------------------

void DrawShrapnels (tObject *objP)
{
	tShrapnelData	*sdP = gameData.objs.shrapnels + OBJ_IDX (objP);
	tShrapnel		*shrapnelP = sdP->shrapnels;
	int				i;

for (i = sdP->nShrapnels; i; i--, shrapnelP++)
	DrawShrapnel (shrapnelP);
}

// -----------------------------------------------------------------------------

int UpdateShrapnels (tObject *objP)
{
	tShrapnelData	*sdP = gameData.objs.shrapnels + OBJ_IDX (objP);
	tShrapnel		*shrapnelP = sdP->shrapnels;
	int				h, i;

#if 0
if (!gameStates.app.tick40fps.bTick)
	return 0;
#endif
if (objP->info.xLifeLeft > 0) {
	for (i = 0, h = sdP->nShrapnels; i < h; ) {
		if (shrapnelP->xTTL <= 0)
			continue;
		MoveShrapnel (shrapnelP);
		if (0 < (shrapnelP->xTTL -= (fix) (SECS2X (gameStates.app.tick40fps.nTime) / gameStates.gameplay.slowmo [0].fSpeed))) {
			shrapnelP++;
			i++;
			}
		else {
			SetSmokeLife (shrapnelP->nSmoke, 0);
			shrapnelP->nSmoke = -1;
			if (i < --h)
				*shrapnelP = sdP->shrapnels [h];
			}
		}
	if ((sdP->nShrapnels = h))
		return 1;
	}
DestroyShrapnels (objP);
return 0;
}

//------------------------------------------------------------------------------

int DoObjectSmoke (tObject *objP)
{
int t = objP->info.nType;
#if 0
if (extraGameInfo [0].bShadows && (gameStates.render.nShadowPass < 3))
	return;
#endif
if (gameStates.render.bQueryCoronas)
	return 0;
if (t == OBJ_PLAYER)
	DoPlayerSmoke (objP, -1);
else if (t == OBJ_ROBOT)
	DoRobotSmoke (objP);
else if ((t == OBJ_EFFECT) && (objP->info.nId == SMOKE_ID))
	DoStaticSmoke (objP);
else if (t == OBJ_REACTOR)
	DoReactorSmoke (objP);
else if (t == OBJ_WEAPON) {
	if (gameData.objs.bIsMissile [objP->info.nId])
		DoMissileSmoke (objP);
	else if ((objP->info.nId == VULCAN_ID) || (objP->info.nId == GAUSS_ID))
		DoParticleTrail (objP);
	else if (!COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0) && (objP->info.nId == PROXMINE_ID))
		DoBombSmoke (objP);
	else if (gameOpts->render.particles.bPlasmaTrails && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bLightTrails, 0, 0, 0) &&
				gameData.objs.bIsWeapon [objP->info.nId] && !gameData.objs.bIsSlowWeapon [objP->info.nId])
		DoParticleTrail (objP);
	else
		return 0;
	}
else if (t == OBJ_DEBRIS)
	DoDebrisSmoke (objP);
else
	return 0;
return 1;
}

//------------------------------------------------------------------------------

void PlayerBulletFrame (void)
{
	int	i;

if (!gameOpts->render.ship.bBullets)
	return;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	DoPlayerBullets (OBJECTS + gameData.multiplayer.players [i].nObject);
}

//------------------------------------------------------------------------------

void PlayerSmokeFrame (void)
{
	int	i;

if (!gameOpts->render.particles.bPlayers)
	return;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	DoPlayerSmoke (OBJECTS + gameData.multiplayer.players [i].nObject, i);
}

//------------------------------------------------------------------------------

void ObjectSmokeFrame (void)
{
	int		i;
	tObject	*objP;

if (!SHOW_SMOKE)
	return;
for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject [1]; i++, objP++) {
	if (gameData.objs.bWantEffect [i] & DESTROY_SMOKE) {
		gameData.objs.bWantEffect [i] &= ~DESTROY_SMOKE;
		KillObjectSmoke (i);
		}
	else if (objP->info.nType == OBJ_NONE)
		KillObjectSmoke (i);
	else
		DoObjectSmoke (objP);
	}
}

//------------------------------------------------------------------------------

void StaticSmokeFrame (void)
{
	tObject	*objP;
	int		i;

if (!SHOW_SMOKE)
	return;
FORALL_EFFECT_OBJS (objP, i) {
	if (objP->info.nId == SMOKE_ID)
		DoStaticSmoke (objP);
	}
}

//------------------------------------------------------------------------------

void ShrapnelFrame (void)
{
	tObject	*objP;
	int		i;

if (!SHOW_SMOKE)
	return;
FORALL_STATIC_OBJS (objP, i) {
	i = OBJ_IDX (objP);
	if (objP->info.renderType == RT_SHRAPNELS)
		UpdateShrapnels (objP);
	if (gameData.objs.bWantEffect [i] & SHRAPNEL_SMOKE) {
		gameData.objs.bWantEffect [i] &= ~SHRAPNEL_SMOKE;
		CreateShrapnels (objP);
		}
	}
}

//------------------------------------------------------------------------------

void DoSmokeFrame (void)
{
#if SHADOWS
if (gameStates.render.nShadowPass > 1)
	return;
#endif
SEM_ENTER (SEM_SMOKE)
PlayerBulletFrame ();
#if 0
#	if DBG
if (!gameStates.render.bExternalView)
#	else
if (!gameStates.render.bExternalView && (!IsMultiGame || IsCoopGame || EGI_FLAG (bEnableCheats, 0, 0, 0)))
#	endif
	DoPlayerSmoke (gameData.objs.viewerP, gameData.multiplayer.nLocalPlayer);
#endif
ObjectSmokeFrame ();
//StaticSmokeFrame ();
SEM_LEAVE (SEM_SMOKE)
ShrapnelFrame ();
UpdateSmoke ();
SEM_LEAVE (SEM_SMOKE)
}

//------------------------------------------------------------------------------
