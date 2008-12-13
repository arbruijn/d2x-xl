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
#include "shrapnel.h"
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
if ((nObject >= 0) && (particleManager.GetObjectSystem (nObject) >= 0)) {
	DigiKillSoundLinkedToObject (nObject);
	particleManager.SetLife (particleManager.GetObjectSystem (nObject), 0);
	particleManager.SetObjectSystem (nObject, -1);
	shrapnelManager.Destroy (OBJECTS + nObject);
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
particleManager.InitObjects ();
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
	 (gameStates.app.nSDLTicks - *particleManager.ObjExplTime (i) > 100)) {
	*particleManager.ObjExplTime (i) = gameStates.app.nSDLTicks;
	if (!RandN (11 - h))
		CreateSmallFireballOnObject (OBJECTS + i, F1_0, 1);
	}
}

//------------------------------------------------------------------------------
#if 0
void CreateThrusterFlames (CObject *objP)
{
	static int nThrusters = -1;

	CFixVector	pos, dir = objP->info.position.mOrient[FVEC];
	int			d, j;
	tParticleEmitter		*emitterP;

VmVecNegate (&dir);
if (nThrusters < 0) {
	nThrusters =
		particleManager.Create (&objP->info.position.vPos, &dir, objP->info.nSegment, 2, -2000, 20000,
										gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [1],
										2, -2000, PLR_PART_SPEED * 50, LIGHT_PARTICLES, OBJ_IDX (objP), NULL, 1, -1);
	particleManager.SetObjectSystem (OBJ_IDX (objP)) = nThrusters;
	}
else
	particleManager.SetDir (nThrusters, &dir);
d = 8 * objP->info.xSize / 40;
for (j = 0; j < 2; j++)
	if (emitterP = GetParticleEmitter (nThrusters, j)) {
		VmVecScaleAdd (&pos, &objP->info.position.vPos, &objP->info.position.mOrient[FVEC], -objP->info.xSize);
		VmVecScaleInc (&pos, &objP->info.position.mOrient[RVEC], j ? d : -d);
		VmVecScaleInc (&pos, &objP->info.position.mOrient[UVEC],  -objP->info.xSize / 25);
		SetParticleEmitterPos (emitterP, &pos, NULL, objP->info.nSegment);
		}
}
#endif

//------------------------------------------------------------------------------

void KillPlayerBullets (CObject *objP)
{
	int	i = gameData.multiplayer.bulletEmitters [objP->info.nId];

if (i >= 0) {
	particleManager.SetLife (i, 0);
	gameData.multiplayer.bulletEmitters [objP->info.nId] = -1;
	}
}

//------------------------------------------------------------------------------

void KillGatlingSmoke (CObject *objP)
{
	int	i = gameData.multiplayer.gatlingSmoke [objP->info.nId];

if (i >= 0) {
	particleManager.SetLife (i, 0);
	gameData.multiplayer.gatlingSmoke [objP->info.nId] = -1;
	}
}

//------------------------------------------------------------------------------

#define BULLET_MAX_PARTS	50
#define BULLET_PART_LIFE	-2000
#define BULLET_PART_SPEED	50

void DoPlayerBullets (CObject *objP)
{
if (RENDERPATH && gameOpts->render.ship.bBullets) {
		int	nModel = objP->rType.polyObjInfo.nModel;
		int	bHires = G3HaveModel (nModel) - 1;

	if (bHires >= 0) {
			CRenderModel	*pm = gameData.models.renderModels [bHires] + nModel;

		if (pm->bBullets) {
				int			nPlayer = objP->info.nId;
				int			nGun = EquippedPlayerGun (objP);
				int			bDoEffect = (bHires >= 0) && ((nGun == VULCAN_INDEX) || (nGun == GAUSS_INDEX)) &&
												(gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration >= GATLING_DELAY);
				int			i = gameData.multiplayer.bulletEmitters [nPlayer];

			if (bDoEffect) {
					int			bSpectate = SPECTATOR (objP);
					tTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
					CFixVector	vEmitter, vDir;
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
						particleManager.Create (&vEmitter, &vDir, &posP->mOrient, objP->info.nSegment, 1, BULLET_MAX_PARTS, 15.0f, 1,
													 1, BULLET_PART_LIFE, BULLET_PART_SPEED, BULLET_PARTICLES, 0x7fffffff, NULL, 0, -1);
					}
				else {
					particleManager.SetPos (i, &vEmitter, &posP->mOrient, objP->info.nSegment);
					}
				}
			else {
				if (i >= 0) {
					particleManager.SetLife (i, 0);
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

void DoGatlingSmoke (CObject *objP)
{
	int	nModel = objP->rType.polyObjInfo.nModel;
	int	bHires = G3HaveModel (nModel) - 1;

if (bHires >= 0) {
		CRenderModel	*pm = gameData.models.renderModels [bHires] + nModel;

	if (pm->bBullets) {
			int			nPlayer = objP->info.nId;
			int			nGun = EquippedPlayerGun (objP);
			int			bDoEffect = (bHires >= 0) && ((nGun == VULCAN_INDEX) || (nGun == GAUSS_INDEX)) &&
											(gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration >= GATLING_DELAY);
			int			i = gameData.multiplayer.gatlingSmoke [nPlayer];

		if (bDoEffect) {
				int			bSpectate = SPECTATOR (objP);
				tTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
				CFixVector	*vGunPoints, vEmitter, vDir;
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
					particleManager.Create (&vEmitter, &vDir, &posP->mOrient, objP->info.nSegment, 1, GATLING_MAX_PARTS, F1_0 / 2, 1,
									 1, GATLING_PART_LIFE, GATLING_PART_SPEED, SMOKE_PARTICLES, 0x7ffffffe, smokeColors + 1, 0, -1);
				}
			else {
				particleManager.SetPos (i, &vEmitter, &posP->mOrient, objP->info.nSegment);
				}
			}
		else {
			if (i >= 0) {
				particleManager.SetLife (i, 0);
				gameData.multiplayer.gatlingSmoke [nPlayer] = -1;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void DoPlayerSmoke (CObject *objP, int i)
{
	int					h, j, d, nParts, nType;
	float					nScale;
	CParticleEmitter	*emitterP;
	CFixVector			fn, mn, vDir, *vDirP;
	tThrusterInfo		ti;

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
	fn = objP->info.position.mOrient [FVEC];
	mn = objP->info.position.vPos - objP->info.vLastPos;
	CFixVector::Normalize (fn);
	CFixVector::Normalize (mn);
	d = CFixVector::Dot(fn, mn);
	if (d >= -F1_0 / 2)
		bForward = 1;
	else {
		if (bForward) {
			if ((h = particleManager.GetObjectSystem (j)) >= 0) {
				KillObjectSmoke (j);
				particleManager.Destroy (h);
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
		if (0 > (h = particleManager.GetObjectSystem (j))) {
			//PrintLog ("creating CPlayerData smoke\n");
			h = particleManager.SetObjectSystem (j,
					particleManager.Create (&objP->info.position.vPos, vDirP, NULL, objP->info.nSegment, 2, nParts, nScale,
									 gameOpts->render.particles.nSize [1],
									 2, PLR_PART_LIFE / (nType + 1) * (vDirP ? 2 : 1), PLR_PART_SPEED, SMOKE_PARTICLES, j, smokeColors + nType, 1, -1));
			}
		else {
			if (vDirP)
				particleManager.SetDir (h, vDirP);
			particleManager.SetLife (h, PLR_PART_LIFE / (nType + 1) * (vDirP ? 2 : 1));
			particleManager.SetType (h, SMOKE_PARTICLES);
			particleManager.SetScale (h, -nScale);
			particleManager.SetDensity (h, nParts, gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [1]);
			particleManager.SetSpeed (particleManager.GetObjectSystem (i),
								objP->mType.physInfo.thrust.IsZero () ? PLR_PART_SPEED * 2 : PLR_PART_SPEED);
			}
		CalcThrusterPos (objP, &ti, 0);
		for (j = 0; j < 2; j++)
			if ((emitterP = particleManager.GetEmitter (h, j)))
				emitterP->SetPos (ti.vPos + j, NULL, objP->info.nSegment);
		DoGatlingSmoke (objP);
		return;
		}
	}
KillObjectSmoke (i);
KillGatlingSmoke (objP);
}

//------------------------------------------------------------------------------

void DoRobotSmoke (CObject *objP)
{
	int			h = -1, i, nShields = 0, nParts;
	float			nScale;
	CFixVector	pos;

i = OBJ_IDX (objP);
if (!(SHOW_SMOKE && gameOpts->render.particles.bRobots)) {
	if (particleManager.GetObjectSystem (i) >= 0)
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
	if ((h = particleManager.GetObjectSystem (i)) < 0) {
		//PrintLog ("creating robot %d smoke\n", i);
		particleManager.SetObjectSystem (i, particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, nScale,
													gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [2],
													1, BOT_PART_LIFE, BOT_PART_SPEED, SMOKE_PARTICLES, i, smokeColors, 1, -1));
		}
	else {
		particleManager.SetScale (h, nScale);
		particleManager.SetDensity (h, nParts, gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [2]);
		particleManager.SetSpeed (h, !objP->mType.physInfo.velocity.IsZero() ?
							BOT_PART_SPEED : BOT_PART_SPEED * 2 / 3);
		}
	pos = objP->info.position.vPos + objP->info.position.mOrient[FVEC] * (-objP->info.xSize / 2);
	particleManager.SetPos (particleManager.GetObjectSystem (i), &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoReactorSmoke (CObject *objP)
{
	int			h = -1, i, nShields = 0, nParts;
	CFixVector	vDir, vPos;

i = OBJ_IDX (objP);
if (!(SHOW_SMOKE && gameOpts->render.particles.bRobots)) {
	if (particleManager.GetObjectSystem (i) >= 0)
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
	if (particleManager.GetObjectSystem (i) < 0) {
		//PrintLog ("creating robot %d smoke\n", i);
		particleManager.SetObjectSystem (i, particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, F2X (-4.0),
												  -1, 1, BOT_PART_LIFE * 2, BOT_PART_SPEED, SMOKE_PARTICLES, i, smokeColors, 1, -1));
		}
	else {
		particleManager.SetScale (particleManager.GetObjectSystem (i), F2X (-4.0));
		particleManager.SetDensity (particleManager.GetObjectSystem (i), nParts, -1);
		vDir[X] = d_rand () - F1_0 / 4;
		vDir[Y] = d_rand () - F1_0 / 4;
		vDir[Z] = d_rand () - F1_0 / 4;
		CFixVector::Normalize(vDir);
		vPos = objP->info.position.vPos + vDir * (-objP->info.xSize / 2);
		particleManager.SetPos (particleManager.GetObjectSystem (i), &vPos, NULL, objP->info.nSegment);
		}
	}
else
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoMissileSmoke (CObject *objP)
{
	int				nParts, nSpeed, nLife, i;
	float				nScale = 1.5f;
	tThrusterInfo	ti;

i = OBJ_IDX (objP);
if (!(SHOW_SMOKE && gameOpts->render.particles.bMissiles)) {
	if (particleManager.GetObjectSystem (i) >= 0)
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
	if (particleManager.GetObjectSystem (i) < 0) {
		if (!gameOpts->render.particles.bSyncSizes) {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [3]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [3], nScale);
			}
		particleManager.SetObjectSystem (i, 
			particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts, nScale,
										   gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [3],
										   1, nLife * MSL_PART_LIFE, MSL_PART_SPEED, SMOKE_PARTICLES, i, smokeColors + 1, 1, -1));
		}
	CalcThrusterPos (objP, &ti, 0);
	particleManager.SetPos (particleManager.GetObjectSystem (i), ti.vPos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoDebrisSmoke (CObject *objP)
{
	int			nParts, i;
	float			nScale = 2;
	CFixVector	pos;

i = OBJ_IDX (objP);
if (!(SHOW_SMOKE && gameOpts->render.particles.bDebris)) {
	if (particleManager.GetObjectSystem (i) >= 0)
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
	if (particleManager.GetObjectSystem (i) < 0) {
		particleManager.SetObjectSystem (i, particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts / 2,
												  nScale, -1, 1, DEBRIS_PART_LIFE, DEBRIS_PART_SPEED, SMOKE_PARTICLES, i, smokeColors, 0, -1));
		}
	pos = objP->info.position.vPos + objP->info.position.mOrient[FVEC] * (-objP->info.xSize);
	particleManager.SetPos (particleManager.GetObjectSystem (i), &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoStaticParticles (CObject *objP)
{
	int			i, j, bBubbles = objP->rType.particleInfo.nType == SMOKE_TYPE_BUBBLES;
	CFixVector	pos, offs, dir;

	static tRgbaColorf defaultColors [2] = {{0.5f, 0.5f, 0.5f, 0.0f}, {0.8f, 0.9f, 1.0f, 1.0f}};

i = (int) OBJ_IDX (objP);
if (!(SHOW_SMOKE && (bBubbles ? gameOpts->render.particles.bBubbles : gameOpts->render.particles.bStatic))) {
	if (particleManager.GetObjectSystem (i) >= 0)
		KillObjectSmoke (i);
	return;
	}
if (particleManager.GetObjectSystem (i) < 0) {
		tRgbaColorf color;
		int bColor;

	color.red = (float) objP->rType.particleInfo.color.red / 255.0f;
	color.green = (float) objP->rType.particleInfo.color.green / 255.0f;
	color.blue = (float) objP->rType.particleInfo.color.blue / 255.0f;
	if ((bColor = (color.red + color.green + color.blue > 0)))
		color.alpha = (float) -objP->rType.particleInfo.color.alpha / 255.0f;
	dir = objP->info.position.mOrient [FVEC] * (objP->rType.particleInfo.nSpeed * 2 * F1_0 / 55);
	particleManager.SetObjectSystem (i, 
		particleManager.Create (&objP->info.position.vPos, &dir, &objP->info.position.mOrient,
									   objP->info.nSegment, 1, -objP->rType.particleInfo.nParts,
									   -PARTICLE_SIZE (objP->rType.particleInfo.nSize [gameOpts->render.particles.bDisperse], bBubbles ? 4.0f : 2.0f),
									   -1, 3, STATIC_SMOKE_PART_LIFE * objP->rType.particleInfo.nLife,
									   objP->rType.particleInfo.nDrift, bBubbles ? BUBBLE_PARTICLES : SMOKE_PARTICLES, 
									   i, bColor ? &color : defaultColors + bBubbles, 1, objP->rType.particleInfo.nSide - 1));
	if (bBubbles)
		DigiSetObjectSound (i, -1, AddonSoundName (SND_ADDON_AIRBUBBLES), F1_0 / 2);
	else
		particleManager.SetBrightness (particleManager.GetObjectSystem (i), objP->rType.particleInfo.nBrightness);
	}
if (objP->rType.particleInfo.nSide <= 0) {	//don't vary emitter position for smoke emitting faces
	i = objP->rType.particleInfo.nDrift >> 4;
	i += objP->rType.particleInfo.nSize [0] >> 2;
	i /= 2;
	if (!(j = i - i / 2))
		j = 2;
	i /= 2;
	offs [X] = (F1_0 / 4 - d_rand ()) * (d_rand () % j + i);
	offs [Y] = (F1_0 / 4 - d_rand ()) * (d_rand () % j + i);
	offs [Z] = (F1_0 / 4 - d_rand ()) * (d_rand () % j + i);
	pos = objP->info.position.vPos + offs;
	particleManager.SetPos (particleManager.GetObjectSystem (i), &pos, NULL, objP->info.nSegment);
	}
}

//------------------------------------------------------------------------------

void DoBombSmoke (CObject *objP)
{
	int			nParts, i;
	CFixVector	pos, offs;

if (gameStates.app.bNostalgia || !gameStates.app.bHaveExtraGameInfo [IsMultiGame])
	return;
i = OBJ_IDX (objP);
if ((objP->info.xShields < 0) || (objP->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else
	nParts = -BOMB_MAX_PARTS;
if (nParts) {
	if (particleManager.GetObjectSystem (i) < 0) {
		particleManager.SetObjectSystem (i, particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts,
												  -PARTICLE_SIZE (3, 0.5f), -1, 3, BOMB_PART_LIFE, BOMB_PART_SPEED, SMOKE_PARTICLES, i, NULL, 1, -1));
		}
	offs [X] = (F1_0 / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	offs [Y] = (F1_0 / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	offs [Z] = (F1_0 / 4 - d_rand ()) * ((d_rand () & 15) + 16);
	pos = objP->info.position.vPos + offs;
	particleManager.SetPos (particleManager.GetObjectSystem (i), &pos, NULL, objP->info.nSegment);
	}
else
	KillObjectSmoke (i);
}

//------------------------------------------------------------------------------

void DoParticleTrail (CObject *objP)
{
	int			nParts, i, id = objP->info.nId, bGatling = (id == VULCAN_ID) || (id == GAUSS_ID);
	float			nScale;
	CFixVector	pos;
	tRgbaColorf	c;

if (!(SHOW_OBJ_FX && (bGatling ? EGI_FLAG (bGatlingTrails, 1, 1, 0) : EGI_FLAG (bLightTrails, 1, 1, 0))))
	return;
i = OBJ_IDX (objP);
if (!(bGatling || gameOpts->render.particles.bPlasmaTrails)) {
	if (particleManager.GetObjectSystem (i) >= 0)
		KillObjectSmoke (i);
	return;
	}
#if 1
nParts = bGatling ? LASER_MAX_PARTS : 2 * LASER_MAX_PARTS / 3;
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
if (particleManager.GetObjectSystem (i) < 0) {
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
	particleManager.SetObjectSystem (i, particleManager.Create (&objP->info.position.vPos, NULL, NULL, objP->info.nSegment, 1, nParts << bGatling, -PARTICLE_SIZE (1, nScale),
											   gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [3],
											   1, ((gameOpts->render.particles.nLife [3] + 1) * LASER_PART_LIFE) << bGatling, LASER_PART_SPEED, 
											   bGatling ? GATLING_PARTICLES : LIGHT_PARTICLES, i, &c, 0, -1));
	}
pos = objP->info.position.vPos + objP->info.position.mOrient[FVEC] * (-objP->info.xSize / 2);
particleManager.SetPos (particleManager.GetObjectSystem (i), &pos, NULL, objP->info.nSegment);
}

//------------------------------------------------------------------------------

int DoObjectSmoke (CObject *objP)
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
	DoStaticParticles (objP);
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

void PlayerParticleFrame (void)
{
	int	i;

if (!gameOpts->render.particles.bPlayers)
	return;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	DoPlayerSmoke (OBJECTS + gameData.multiplayer.players [i].nObject, i);
}

//------------------------------------------------------------------------------

void ObjectParticleFrame (void)
{
	int		i;
	CObject	*objP;

if (!SHOW_SMOKE)
	return;
for (i = 0, objP = OBJECTS.Buffer (); i <= gameData.objs.nLastObject [1]; i++, objP++) {
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

void StaticParticlesFrame (void)
{
	CObject	*objP;

if (!SHOW_SMOKE)
	return;
FORALL_EFFECT_OBJS (objP, i) {
	if (objP->info.nId == SMOKE_ID)
		DoStaticParticles (objP);
	}
}

//------------------------------------------------------------------------------

void DoParticleFrame (void)
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
ObjectParticleFrame ();
//StaticParticlesFrame ();
SEM_LEAVE (SEM_SMOKE)
shrapnelManager.DoFrame ();
particleManager.Update ();
SEM_LEAVE (SEM_SMOKE)
}

//------------------------------------------------------------------------------
