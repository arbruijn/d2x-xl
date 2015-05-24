#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "descent.h"
#include "error.h"
#include "timer.h"
#include "u_mem.h"
#include "interp.h"
#include "lightning.h"
#include "network.h"
#include "rendermine.h"
#include "objeffects.h"
#include "objrender.h"
#include "objsmoke.h"
#include "shrapnel.h"
#include "automap.h"
#include "thrusterflames.h"
#include "renderthreads.h"

static CFloatVector smokeColors [] = {
	 {{{0.5f, 0.5f, 0.5f, 2.0f}}},	// alpha == 2.0 means that the particles are red in the beginning
	 {{{0.75f, 0.75f, 0.75f, 2.0f}}},
	 {{{1.0f, 1.0f, 1.0f, 2.0f}}},
	 {{{1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, -0.25f}}},
	 {{{1.0f, 1.0f, 1.0f, -0.1f}}}
	};

#define SHIP_MAX_PARTS				50
#define PLR_PART_LIFE				-1400
#define PLR_PART_SPEED				50

#define BOT_MAX_PARTS				250
#define BOT_PART_LIFE				-6000
#define BOT_PART_SPEED				300

#define MSL_MAX_PARTS				750
#define MSL_PART_LIFE				-3000
#define MSL_PART_SPEED				-50

#define LASER_MAX_PARTS				250
#define LASER_PART_LIFE				-500
#define LASER_PART_SPEED			0

#define BOMB_MAX_PARTS				250
#define BOMB_PART_LIFE				-16000
#define BOMB_PART_SPEED				200

#define DEBRIS_MAX_PARTS			250
#define DEBRIS_PART_LIFE			-2000
#define DEBRIS_PART_SPEED			50

#define FIRE_PART_LIFE				-800

#define REACTOR_MAX_PARTS			500

#define PLAYER_SMOKE					1
#define ROBOT_SMOKE					1
#define MISSILE_SMOKE				1
#define REACTOR_SMOKE				1
#define GATLING_SMOKE				1
#define DEBRIS_SMOKE					1
#define BOMB_SMOKE					1
#define STATIC_SMOKE					1
#define PARTICLE_TRAIL				1

//------------------------------------------------------------------------------

#if DBG

void KillObjectSmoke (int32_t nObject)
{
if ((nObject >= 0) && (particleManager.GetObjectSystem (nObject) >= 0)) {
	if (OBJECT (nObject)->Type () == OBJ_EFFECT)
		audio.DestroyObjectSound (nObject);
	particleManager.SetLife (particleManager.GetObjectSystem (nObject), 0);
	particleManager.SetObjectSystem (nObject, -1);
	shrapnelManager.Destroy (OBJECT (nObject));
	}
}

#endif

//------------------------------------------------------------------------------

void KillPlayerSmoke (int32_t i)
{
KillObjectSmoke (PLAYER (i).nObject);
}

//------------------------------------------------------------------------------

void ResetPlayerSmoke (void)
{
	int32_t	i;

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
	int32_t	i;

for (i = 0; i < LEVEL_OBJECTS; i++)
	KillObjectSmoke (i);
InitObjectSmoke ();
}

//------------------------------------------------------------------------------

static inline int32_t RandN (int32_t n)
{
if (!n)
	return 0;
return (int32_t) ((double) rand () * (double) n / (double) RAND_MAX);
}

//------------------------------------------------------------------------------

void CreateDamageExplosion (int32_t h, int32_t i)
{
if (EGI_FLAG (bDamageExplosions, 1, 0, 0) &&
	 (gameStates.app.nSDLTicks [0] - *particleManager.ObjExplTime (i) > 100)) {
	*particleManager.ObjExplTime (i) = gameStates.app.nSDLTicks [0];
	if (!RandN (11 - h))
		CreateSmallFireballOnObject (OBJECT (i), I2X (1), 1);
	}
}

//------------------------------------------------------------------------------
#if 0
void CreateThrusterFlames (CObject *pObj)
{
	static int32_t nThrusters = -1;

	CFixVector	coord, dir = pObj->info.position.mOrient.mat.dir.f;
	int32_t			d, j;
	tParticleEmitter		*pEmitter;

VmVecNegate (&dir);
if (nThrusters < 0) {
	nThrusters =
		particleManager.Create (&pObj->info.position.vPos, &dir, pObj->info.nSegment, 2, -2000, 20000,
										gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [1],
										2, -2000, PLR_PART_SPEED * 50, LIGHT_PARTICLES, pObj->Index (), NULL, 1, -1);
	particleManager.SetObjectSystem (pObj->Index ()) = nThrusters;
	}
else
	particleManager.SetDir (nThrusters, &dir);
d = 8 * pObj->info.xSize / 40;
for (j = 0; j < 2; j++)
	if (pEmitter = GetParticleEmitter (nThrusters, j)) {
		VmVecScaleAdd (&coord, &pObj->info.position.vPos, &pObj->info.position.mOrient.mat.dir.f, -pObj->info.xSize);
		VmVecScaleInc (&coord, &pObj->info.position.mOrient.mat.dir.r, j ? d : -d);
		VmVecScaleInc (&coord, &pObj->info.position.mOrient.mat.dir.u,  -pObj->info.xSize / 25);
		SetParticleEmitterPos (pEmitter, &coord, NULL, pObj->info.nSegment);
		}
}
#endif

//------------------------------------------------------------------------------

void KillPlayerBullets (CObject *pObj)
{
if (pObj) {
	int32_t	i = gameData.multiplayer.bulletEmitters [pObj->info.nId];

	if ((i >= 0) && (i < MAX_PLAYERS)) {
		particleManager.SetLife (i, 0);
		gameData.multiplayer.bulletEmitters [pObj->info.nId] = -1;
		}
	}
}

//------------------------------------------------------------------------------

void KillGatlingSmoke (CObject *pObj)
{
if (pObj) {
	int32_t	i = gameData.multiplayer.gatlingSmoke [pObj->info.nId];

	if ((i >= 0) && (i < MAX_PLAYERS)) {
		particleManager.SetLife (i, 0);
		gameData.multiplayer.gatlingSmoke [pObj->info.nId] = -1;
		}
	}
}

//------------------------------------------------------------------------------

#define BULLET_MAX_PARTS	50
#define BULLET_PART_LIFE	-2000
#define BULLET_PART_SPEED	50

void DoPlayerBullets (CObject *pObj)
{
if (gameOpts->render.ship.bBullets) {
		int32_t	nModel = pObj->ModelId ();
		int32_t	bHires = G3HaveModel (nModel) - 1;

	if (bHires >= 0) {
		RenderModel::CModel	*pm = gameData.models.renderModels [bHires] + nModel;

		if (pm->m_bBullets) {
				int32_t	nPlayer = pObj->info.nId;
				int32_t	nGun = EquippedPlayerGun (pObj);
				int32_t	bDoEffect = (bHires >= 0) && ((nGun == VULCAN_INDEX) || (nGun == GAUSS_INDEX)) &&
											(gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration >= GATLING_DELAY);
				int32_t	i = gameData.multiplayer.bulletEmitters [nPlayer];

			if (bDoEffect) {
					int32_t					bSpectate = SPECTATOR (pObj);
					tObjTransformation*	pPos = bSpectate ? &gameStates.app.playerPos : &pObj->info.position;
					CFixVector				vEmitter, vDir;
					CFixMatrix				m, * pView;

				if (bSpectate) {
					pView = &m;
					m = pPos->mOrient.Transpose ();
				}
				else
					pView = pObj->View (0);
				vEmitter = *pView * pm->m_vBullets;
				vEmitter += pPos->vPos;
				vDir = pPos->mOrient.m.dir.u;
				vDir.Neg ();
				if (i < 0) {
					gameData.multiplayer.bulletEmitters [nPlayer] =
						particleManager.Create (&vEmitter, &vDir, &pPos->mOrient, pObj->info.nSegment, 1, BULLET_MAX_PARTS, 15.0f, 
														/*1, 1,*/ 
														BULLET_PART_LIFE, BULLET_PART_SPEED, BULLET_PARTICLES, 0x7fffffff, NULL, 0, -1);
					}
				else {
					particleManager.SetPos (i, &vEmitter, &pPos->mOrient, pObj->info.nSegment);
					particleManager.SetFadeType (i, -1);
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

#define GATLING_MAX_PARTS	35
#define GATLING_PART_LIFE	-1000
#define GATLING_PART_SPEED	30

void DoGatlingSmoke (CObject *pObj)
{
#if GATLING_SMOKE
	int32_t	nModel = pObj->ModelId ();
	int32_t	bHires = G3HaveModel (nModel) - 1;

if (bHires >= 0) {
	RenderModel::CModel	*pm = gameData.models.renderModels [bHires] + nModel;

	if (pm->m_bBullets) {
			int32_t	nPlayer = pObj->info.nId;
			int32_t	nGun = EquippedPlayerGun (pObj);
			int32_t	bDoEffect = (bHires >= 0) && ((nGun == VULCAN_INDEX) || (nGun == GAUSS_INDEX)) &&
										(gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration >= GATLING_DELAY);
			int32_t		i = gameData.multiplayer.gatlingSmoke [nPlayer];

		if (bDoEffect) {
				int32_t					bSpectate = SPECTATOR (pObj);
				tObjTransformation*	pPos = bSpectate ? &gameStates.app.playerPos : &pObj->info.position;
				CFixVector				* vGunPoints, vEmitter, vDir;
				CFixMatrix				m, * pView;

			if (!(vGunPoints = GetGunPoints (pObj, nGun)))
				return;
			if (bSpectate) {
				pView = &m;
				*pView = pPos->mOrient.Transpose();
			}

			else
				pView = pObj->View (0);
			vEmitter = *pView * vGunPoints[nGun];
			vEmitter += pPos->vPos;
			//vDir = pPos->mOrient.m.v.f;
			vDir = pPos->mOrient.m.dir.f * (I2X (1) / 8);
			if (i < 0) {
				gameData.multiplayer.gatlingSmoke [nPlayer] =
					particleManager.Create (&vEmitter, &vDir, &pPos->mOrient, pObj->info.nSegment, 1, GATLING_MAX_PARTS, I2X (1) / 2, 
													/*1, 1,*/ 
													GATLING_PART_LIFE, GATLING_PART_SPEED, SIMPLE_SMOKE_PARTICLES, 0x7ffffffe, smokeColors + 3, 0, -1);
				}
			else {
				particleManager.SetPos (i, &vEmitter, &pPos->mOrient, pObj->info.nSegment);
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
#endif
}

//------------------------------------------------------------------------------

void DoPlayerSmoke (CObject *pObj, int32_t nPlayer)
{
#if PLAYER_SMOKE
	int32_t				nObject, nSmoke, d, nParts, nType;
	float					nScale;
	CParticleEmitter*	pEmitter;
	CFixVector			fn, mn, vDir;
	tThrusterInfo		ti;

	static int32_t	bForward = 1;

if (nPlayer < 0)
	nPlayer = pObj->info.nId;
if ((PLAYER (nPlayer).flags & PLAYER_FLAGS_CLOAKED) ||
	 (automap.Active () && IsMultiGame && !AM_SHOW_PLAYERS)) {
	KillObjectSmoke (nPlayer);
	return;
	}
nObject = pObj->Index ();
if (gameOpts->render.particles.bDecreaseLag && (nPlayer == N_LOCALPLAYER)) {
	fn = pObj->info.position.mOrient.m.dir.f;
	mn = pObj->info.position.vPos - pObj->info.vLastPos;
	CFixVector::Normalize (fn);
	CFixVector::Normalize (mn);
	d = CFixVector::Dot (fn, mn);
	if (d >= -I2X (1) / 2)
		bForward = 1;
	else {
		if (bForward) {
			if ((nSmoke = particleManager.GetObjectSystem (nObject)) >= 0) {
				KillObjectSmoke (nObject);
				particleManager.Destroy (nSmoke);
				}
			bForward = 0;
			nScale = 0;
			return;
			}
		}
	}
#if 0
if (EGI_FLAG (bThrusterFlames, 1, 1, 0)) {
	if ((vec <= I2X (1) / 4) && (vec || !gameStates.input.bControlsSkipFrame))	//no thruster flames if moving backward
		DropAfterburnerBlobs (pObj, 2, I2X (1), -1, gameData.objData.pConsole, 1); //I2X (1) / 4);
	}
#endif
if (IsNetworkGame && !PLAYER (nPlayer).connected)
	nParts = 0;
else if (pObj->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED))
	nParts = 0;
else if ((nPlayer == N_LOCALPLAYER) && (gameStates.app.bPlayerIsDead || (PLAYER (nPlayer).Shield () < 0)))
	nParts = 0;
else if (SHOW_SMOKE && gameOpts->render.particles.bPlayers) {
	tObjTransformation* pos = OBJPOS (pObj);
	int16_t nSegment = OBJSEG (pObj);
	CFixVector* vDirP = (SPECTATOR (pObj) || pObj->mType.physInfo.thrust.IsZero ()) ? &vDir : NULL;

	nSmoke = X2IR (PLAYER (nPlayer).Shield ());
	nScale = X2F (pObj->info.xSize) / 2.0f;
	nParts = 10 - nSmoke / 5;	// scale with damage, starting at 50% damage
	if (nParts <= 0) {
		nType = 2;	// no damage
		//nScale /= 2;
		nParts = SHIP_MAX_PARTS / 2; //vDirP ? SHIP_MAX_PARTS / 2 : SHIP_MAX_PARTS;
		}
	else {
		CreateDamageExplosion (nParts, nObject);
		nType = (nSmoke > 25);	// 0: severely damaged, 1: damaged
		nScale /= 2.0f - float (nSmoke) / 50.0f;
		nParts *= 25;
		nParts += 75;
		}

	if (nParts && ((nType < 2) || vDirP)) {
		if (gameOpts->render.particles.bSyncSizes) {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [0]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [0], nScale, 1);
			}
		else {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [1]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [1], nScale, 1);
			}
		int32_t nLife = ((nType != 2) || vDirP) ? PLR_PART_LIFE : PLR_PART_LIFE / 7;
		if (vDirP) {	// if the ship is standing still, let the thruster smoke move away from it
			nParts /= 2;
			if (nType != 2)
				nScale /= 2;
			else
				nType = 4;
			vDir = OBJPOS (pObj)->mOrient.m.dir.f * -(I2X (1) / 8);
			}
		else if (nType == 2)
			nParts /= 2;
		if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
			nSmoke = particleManager.Create (&pos->vPos, vDirP, NULL, nSegment, 2, nParts, nScale,
														/*gameOpts->render.particles.nSize [1], 2,*/ 
														nLife, PLR_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors + nType, 1, -1);
			if (nSmoke < 0)
				return;
			particleManager.SetObjectSystem (nObject, nSmoke);
			}
		else {
			particleManager.SetDir (nSmoke, vDirP);
			particleManager.SetFadeType (nSmoke, vDirP != NULL);
			particleManager.SetLife (nSmoke, nLife);
			//particleManager.SetBlowUp (nSmoke, bBlowUp);
			particleManager.SetType (nSmoke, SMOKE_PARTICLES);
			particleManager.SetScale (nSmoke, -nScale);
			particleManager.SetDensity (nSmoke, nParts, 1/*, gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [1]*/);
			particleManager.SetSpeed (particleManager.GetObjectSystem (nObject),
											  pObj->mType.physInfo.thrust.IsZero () ? PLR_PART_SPEED * 2 : PLR_PART_SPEED);
			}
		thrusterFlames.CalcPos (pObj, &ti);
		for (int32_t i = 0; i < 2; i++)
			if ((pEmitter = particleManager.GetEmitter (nSmoke, i)))
				pEmitter->SetPos (ti.vPos + i, NULL, nSegment);
		}
	DoGatlingSmoke (pObj);
	return;
	}
KillObjectSmoke (nObject);
KillGatlingSmoke (pObj);
#endif
}

//------------------------------------------------------------------------------

void DoRobotSmoke (CObject *pObj)
{
#if ROBOT_SMOKE
	int32_t			h = -1, nObject, nSmoke, nShield = 0, nParts;
	float			nScale;
	CFixVector	pos, vDir;

nObject = pObj->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bRobots)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((pObj->info.xShield < 0) || (pObj->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShield = X2IR (RobotDefaultShield (pObj));
	if (0 >= nShield)
		return;
	h = X2IR (pObj->info.xShield) * 100 / nShield;
	}
if (h < 0)
	return;
nParts = 10 - h / 5;
if (nParts > 0) {
	CFixVector* vDirP = pObj->mType.physInfo.velocity.IsZero () ? &vDir : NULL;

	if (vDirP) // if the robot is standing still, let the smoke move away from it
		vDir = OBJPOS (pObj)->mOrient.m.dir.f * -(I2X (1) / 12);

	if (nShield > 4000)
		nShield = 4000;
	else if (nShield < 1000)
		nShield = 1000;
	CreateDamageExplosion (nParts, nObject);
	//nParts *= nShield / 10;
	nParts = BOT_MAX_PARTS;
	nScale = (float) sqrt (8.0 / X2F (pObj->info.xSize));
	nScale *= 1.0f + h / 25.0f;
	if (!gameOpts->render.particles.bSyncSizes) {
		nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [2]);
		nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [2], nScale, 1);
		}
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		nSmoke = particleManager.Create (&pObj->info.position.vPos, NULL, NULL, pObj->info.nSegment, 1, nParts, nScale,
													/*gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [2], 1,*/ 
													gameOpts->render.particles.nSize [3] * BOT_PART_LIFE / 2, BOT_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	else {
		particleManager.SetDir (nSmoke, vDirP);
		particleManager.SetFadeType (nSmoke, vDirP != NULL);
		particleManager.SetScale (nSmoke, nScale);
		particleManager.SetDensity (nSmoke, nParts, 2/*, gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [2]*/);
		particleManager.SetSpeed (nSmoke, !pObj->mType.physInfo.velocity.IsZero () ? BOT_PART_SPEED : BOT_PART_SPEED * 2 / 3);
		}
	pos = pObj->info.position.vPos + pObj->info.position.mOrient.m.dir.f * (-pObj->info.xSize / 2);
	particleManager.SetPos (nSmoke, &pos, NULL, pObj->info.nSegment);
	}
else
	KillObjectSmoke (nObject);
#endif
}

//------------------------------------------------------------------------------

void DoReactorSmoke (CObject *pObj)
{
#if REACTOR_SMOKE
	int32_t		h = -1, nObject, nSmoke, nShield = 0, nParts;
	CFixVector	vDir, vPos;

nObject = pObj->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bRobots)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((pObj->info.xShield < 0) || (pObj->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nShield = X2IR (gameData.botData.info [gameStates.app.bD1Mission][pObj->info.nId].strength);
	h = nShield ? X2IR (pObj->info.xShield) * 100 / nShield : 0;
	}
if (h < 0)
	h = 0;
nParts = 10 - h / 10;
if (nParts > 0) {
	nParts = REACTOR_MAX_PARTS;
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		nSmoke = particleManager.Create (&pObj->info.position.vPos, NULL, NULL, pObj->info.nSegment, 1, nParts, F2X (-4.0),
													/*-1, 1,*/ 
													gameOpts->render.particles.nSize [3] * BOT_PART_LIFE, BOT_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	else {
		particleManager.SetScale (nSmoke, F2X (-4.0));
		particleManager.SetDensity (nSmoke, nParts, 2/*, -1*/);
		vDir.v.coord.x = SRandShort ();
		vDir.v.coord.y = SRandShort ();
		vDir.v.coord.z = SRandShort ();
		CFixVector::Normalize (vDir);
		vPos = pObj->info.position.vPos + vDir * (-pObj->info.xSize / 2);
		particleManager.SetPos (nSmoke, &vPos, NULL, pObj->info.nSegment);
		}
	}
else
	KillObjectSmoke (nObject);
#endif
}

//------------------------------------------------------------------------------

void DoMissileSmoke (CObject *pObj)
{
#if MISSILE_SMOKE
	int32_t	nParts, nSpeed, nObject, nSmoke;
	float		nScale = 1.5f + float (gameOpts->render.particles.nQuality) / 2.0f, 
				nLife;

nObject = pObj->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bMissiles)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((pObj->info.xShield < 0) || (pObj->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else {
	nSpeed = WI_speed (pObj->info.nId, gameStates.app.nDifficultyLevel);
	nLife = float (gameOpts->render.particles.nLife [3]) * sqrt (float (nSpeed) / float (WI_speed (CONCUSSION_ID, gameStates.app.nDifficultyLevel)));
	nParts = int32_t (MSL_MAX_PARTS /** nLife*/); //int32_t (MSL_MAX_PARTS * X2F (nSpeed) / (15.0f * (4 - nLife)));
	}
if (nParts) {
	CFixVector vPos = pObj->Orientation ().m.dir.f;
	tHitbox&	hb = gameData.models.hitboxes [pObj->ModelId (true)].hitboxes [0];
	int32_t l = labs (hb.vMax.v.coord.z - hb.vMin.v.coord.z);
	vPos *= l / -5/*(gameOpts->render.particles.bDisperse ? -2 : -2) * l / 10*/;
	vPos += pObj->Position (); 
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		if (!gameOpts->render.particles.bSyncSizes) {
			nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [3]);
			nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [3], nScale, 1);
			}
		uint8_t nId = pObj->Id ();
		if ((nId == MERCURYMSL_ID) || (nId == ROBOT_MERCURYMSL_ID)) {
			nParts *= 2;
			nLife /= 2;
			}
		else if ((nId == EARTHSHAKER_MEGA_ID) || (nId == ROBOT_SHAKER_MEGA_ID)) {
			//nParts /= 2;
			nLife /= 2;
			}
#if 0
	if ((pObj->info.nId == EARTHSHAKER_MEGA_ID) || (pObj->info.nId == ROBOT_SHAKER_MEGA_ID))
		nParts /= 2;
#endif
		nSmoke = particleManager.Create (&vPos, NULL, NULL, pObj->info.nSegment, 1, int32_t (nParts * nLife * nLife), nScale,
													/*gameOpts->render.particles.bSyncSizes ? -1 : gameOpts->render.particles.nSize [3], 1,*/ 
													int32_t (nLife * MSL_PART_LIFE * 0.5), MSL_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors + 1, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
#if 1
	particleManager.SetPos (nSmoke, &vPos, NULL, pObj->info.nSegment);
#else
	tThrusterInfo	ti;
	thrusterFlames.CalcPos (pObj, &ti);
	particleManager.SetPos (nSmoke, ti.vPos, NULL, pObj->info.nSegment);
#endif
	}
else
	KillObjectSmoke (nObject);
#endif
}

//------------------------------------------------------------------------------

void DoDebrisSmoke (CObject *pObj)
{
#if DEBRIS_SMOKE
	int32_t		nParts, nObject, nSmoke;
	float			nScale = 2;
	CFixVector	pos;

nObject = pObj->Index ();
if (!(SHOW_SMOKE && gameOpts->render.particles.bDebris)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if ((pObj->info.xShield < 0) || (pObj->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)) ||
	 (gameOpts->render.nDebrisLife && (I2X (nDebrisLife [gameOpts->render.nDebrisLife]) - pObj->info.xLifeLeft > I2X (10))))
	nParts = 0;
else
	nParts = DEBRIS_MAX_PARTS;
if (nParts) {
	if (!gameOpts->render.particles.bSyncSizes) {
		nParts = -MAX_PARTICLES (nParts, gameOpts->render.particles.nDens [4]);
		nScale = PARTICLE_SIZE (gameOpts->render.particles.nSize [4], nScale, 1);
		}
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		nSmoke = particleManager.Create (&pObj->info.position.vPos, NULL, NULL, pObj->info.nSegment, 1, nParts / 2, nScale, 
													/*-1, 1,*/ 
													gameOpts->render.particles.nSize [4] * DEBRIS_PART_LIFE / 2, DEBRIS_PART_SPEED, SMOKE_PARTICLES, nObject, smokeColors, 0, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	pos = pObj->info.position.vPos + pObj->info.position.mOrient.m.dir.f * (-pObj->info.xSize);
	particleManager.SetPos (nSmoke, &pos, NULL, pObj->info.nSegment);
	}
else
	KillObjectSmoke (nObject);
#endif
}

//------------------------------------------------------------------------------

void DoStaticParticles (CObject *pObj)
{
#if STATIC_SMOKE
	int32_t			i, j, nObject, nSmoke, 
					nType, nFadeType;
	CFixVector	pos, offs, dir;

	static CFloatVector defaultColors [7] = {{{{0.5f, 0.5f, 0.5f, 0.0f}}}, {{{0.8f, 0.9f, 1.0f, 1.0f}}}, {{{1.0f, 1.0f, 1.0f, 1.0f}}},
														 {{{1.0f, 1.0f, 1.0f, 1.0f}}}, {{{1.0f, 1.0f, 1.0f, 1.0f}}}, {{{1.0f, 1.0f, 1.0f, 1.0f}}},
														 {{{1.0f, 1.0f, 1.0f, 1.0f}}}};

	static int32_t particleTypes [7] = {SMOKE_PARTICLES, BUBBLE_PARTICLES, FIRE_PARTICLES, WATERFALL_PARTICLES, SIMPLE_SMOKE_PARTICLES, RAIN_PARTICLES, SNOW_PARTICLES};

nObject = (int32_t) pObj->Index ();
if (pObj->rType.particleInfo.nType == SMOKE_TYPE_WATERFALL) {
	nType = 3;
	nFadeType = 2;
	}
else if (pObj->rType.particleInfo.nType == SMOKE_TYPE_FIRE) {
	nType = 2;
	nFadeType = 3;
	}
else if (pObj->rType.particleInfo.nType == SMOKE_TYPE_BUBBLES) {
	nType = 1;
	nFadeType = -1;
	}
else if ((pObj->rType.particleInfo.nType == SMOKE_TYPE_RAIN) || (pObj->rType.particleInfo.nType == SMOKE_TYPE_SNOW)) {
	nType = pObj->rType.particleInfo.nType;
	nFadeType = -1;
	}
else {
	nType = 0;
	nFadeType = 1;
	}
if (!(SHOW_SMOKE && pObj->rType.particleInfo.bEnabled && ((nType == 1) ? gameOpts->render.particles.bBubbles : gameOpts->render.particles.bStatic))) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		CFloatVector color;
		int32_t bColor;

	color.Red () = (float) pObj->rType.particleInfo.color.Red () / 255.0f;
	color.Green () = (float) pObj->rType.particleInfo.color.Green () / 255.0f;
	color.Blue () = (float) pObj->rType.particleInfo.color.Blue () / 255.0f;
	if ((bColor = (color.Red () + color.Green () + color.Blue () > 0)))
		color.Alpha () = (float) -pObj->rType.particleInfo.color.Alpha () / 255.0f;
	dir = pObj->info.position.mOrient.m.dir.f * (pObj->rType.particleInfo.nSpeed * I2X (2) / 55);
	nSmoke = particleManager.Create (&pObj->info.position.vPos, &dir, &pObj->info.position.mOrient,
												pObj->info.nSegment, 1,
												-pObj->rType.particleInfo.nParts / ((nType == SMOKE_TYPE_RAIN) ? 10 : ((nType == SMOKE_TYPE_FIRE) || (nType == SMOKE_TYPE_SNOW)) ? 10 : 1),
												-PARTICLE_SIZE (pObj->rType.particleInfo.nSize [gameOpts->render.particles.bDisperse], (nType == 1) ? 4.0f : 2.0f, 1),
												/*-1, 3,*/ 
												(nType == 2) ? FIRE_PART_LIFE * int32_t (sqrt (double (pObj->rType.particleInfo.nLife))) : STATIC_SMOKE_PART_LIFE * pObj->rType.particleInfo.nLife,
												(nType == SMOKE_TYPE_RAIN) ? pObj->rType.particleInfo.nDrift * 64 : pObj->rType.particleInfo.nDrift, 
												particleTypes [nType], 
												nObject, bColor ? &color : defaultColors + nType, nType != 2, pObj->rType.particleInfo.nSide - 1);
	if (nSmoke < 0)
		return;
	particleManager.SetObjectSystem (nObject, nSmoke);
	if (nType == 1)
		audio.CreateObjectSound (-1, SOUNDCLASS_GENERIC, nObject, 1, I2X (1) / 2, I2X (256), -1, -1, AddonSoundName (SND_ADDON_AIRBUBBLES));
	else
		particleManager.SetBrightness (nSmoke, pObj->rType.particleInfo.nBrightness);
	particleManager.SetFadeType (nSmoke, nFadeType);
	}
if (pObj->rType.particleInfo.nSide <= 0) {	//don't vary emitter position for smoke emitting faces
	i = pObj->rType.particleInfo.nDrift >> 4;
	i += pObj->rType.particleInfo.nSize [0] >> 2;
	i /= 2;
	if (!(j = i - i / 2))
		j = 2;
	i /= 2;
	offs.v.coord.x = SRandShort () * (Rand (j) + i);
	offs.v.coord.y = SRandShort () * (Rand (j) + i);
	offs.v.coord.z = SRandShort () * (Rand (j) + i);
	pos = pObj->info.position.vPos + offs;
	particleManager.SetPos (nSmoke, &pos, NULL, pObj->info.nSegment);
	}
#endif
}

//------------------------------------------------------------------------------

void DoBombSmoke (CObject *pObj)
{
#if BOMB_SMOKE
	int32_t			nParts, nObject, nSmoke;
	CFixVector	pos, offs;

if (gameStates.app.bNostalgia || !gameStates.app.bHaveExtraGameInfo [IsMultiGame])
	return;
nObject = pObj->Index ();
if ((pObj->info.xShield < 0) || (pObj->info.nFlags & (OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	nParts = 0;
else
	nParts = -BOMB_MAX_PARTS;
if (nParts) {
	if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
		nSmoke = particleManager.Create (&pObj->info.position.vPos, NULL, NULL, pObj->info.nSegment, 1, nParts, -PARTICLE_SIZE (3, 0.5f, 1), 
													/*-1, 3,*/ 
													BOMB_PART_LIFE, BOMB_PART_SPEED, SMOKE_PARTICLES, nObject, NULL, 1, -1);
		if (nSmoke < 0)
			return;
		particleManager.SetObjectSystem (nObject, nSmoke);
		}
	offs.v.coord.x = SRandShort () * ((RandShort () & 15) + 16);
	offs.v.coord.y = SRandShort () * ((RandShort () & 15) + 16);
	offs.v.coord.z = SRandShort () * ((RandShort () & 15) + 16);
	pos = pObj->info.position.vPos + offs;
	particleManager.SetPos (nSmoke, &pos, NULL, pObj->info.nSegment);
	}
else
	KillObjectSmoke (nObject);
#endif
}

//------------------------------------------------------------------------------

void DoParticleTrail (CObject *pObj)
{
#if PARTICLE_TRAIL
	int32_t			nParts, nObject, nSmoke, id = pObj->info.nId, 
						bGatling = pObj->IsGatlingRound (),
						bOmega = (id == OMEGA_ID);
	float				nScale;
	CFixVector		pos;
	CFloatVector	c;

if (!(SHOW_OBJ_FX && (bGatling ? EGI_FLAG (bUseParticles, 0, 0, 0) && EGI_FLAG (bGatlingTrails, 1, 1, 0) : EGI_FLAG (bLightTrails, 1, 1, 0))))
	return;
nObject = pObj->Index ();
if (!(bGatling || gameOpts->render.particles.bPlasmaTrails)) {
	if (particleManager.GetObjectSystem (nObject) >= 0)
		KillObjectSmoke (nObject);
	return;
	}
#if 1
nParts = bGatling ? LASER_MAX_PARTS : 2 * LASER_MAX_PARTS / 3;
#else
nParts = gameData.weapons.info [pObj->info.nId].speed [0] / I2X (1);
#endif
if (bGatling) {
	c.Red () = c.Green () = c.Blue () = 2.0f / 3.0f;
	c.Alpha () = 1.0f / 4.0f;
	}
else {
	c.Red () = (float) gameData.weapons.color [pObj->info.nId].Red ();
	c.Green () = (float) gameData.weapons.color [pObj->info.nId].Green ();
	c.Blue () = (float) gameData.weapons.color [pObj->info.nId].Blue ();
	c.Alpha () = 0.5f;
	}
if (0 > (nSmoke = particleManager.GetObjectSystem (nObject))) {
	if (bGatling)
		nScale = 5.0f;
	else {
		if (((id >= LASER_ID) && (id <= LASER_ID + 3)) ||
			 (id == SUPERLASER_ID) || (id == SUPERLASER_ID + 1) ||
			 (id == ROBOT_BLUE_LASER_ID) || (id == ROBOT_GREEN_LASER_ID) || 
			 (id == ROBOT_RED_LASER_ID) || (id == ROBOT_WHITE_LASER_ID))
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
		else if (bOmega)
			nScale = omegaLightning.Exist () ? 2.0f : 1.0f;
		else
			nScale = 1;
		c.Alpha () = 0.1f + nScale / 10;
		}
	nSmoke = particleManager.Create (&pObj->info.position.vPos, NULL, NULL, pObj->info.nSegment, 1, nParts << bGatling, -PARTICLE_SIZE (1, nScale, 1),
											   /*gameOpts->render.particles.nSize [3], 1,*/ 
												(gameOpts->render.particles.nLife [3] + 1) * (bGatling ? 3 * LASER_PART_LIFE / 2 : LASER_PART_LIFE >> bOmega) /*<< bGatling*/, LASER_PART_SPEED, 
											   bGatling ? GATLING_PARTICLES : LIGHT_PARTICLES, nObject, &c, 0, -1);
	if (nSmoke < 0)
		return;
	particleManager.SetObjectSystem (nObject, nSmoke);
	particleManager.SetFadeType (nSmoke, bGatling ? 0 : 4);
	}
pos = pObj->RenderPos () + pObj->info.position.mOrient.m.dir.f * (-pObj->info.xSize / 2);
particleManager.SetPos (nSmoke, &pos, NULL, pObj->info.nSegment);
#endif
}

//------------------------------------------------------------------------------

int32_t DoObjectSmoke (CObject *pObj)
{
int32_t t = pObj->info.nType;
	uint8_t nId = pObj->info.nId;

if (t == OBJ_PLAYER)
	DoPlayerSmoke (pObj, -1);
else if (t == OBJ_ROBOT)
	DoRobotSmoke (pObj);
else if ((t == OBJ_EFFECT) && (nId == PARTICLE_ID))
	DoStaticParticles (pObj);
else if (t == OBJ_REACTOR)
	DoReactorSmoke (pObj);
else if (t == OBJ_WEAPON) {
	if (pObj->IsMissile ())
		DoMissileSmoke (pObj);
	else if (pObj->IsGatlingRound ())
		DoParticleTrail (pObj);
	else if (IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0) && (nId == PROXMINE_ID))
		DoBombSmoke (pObj);
	else if (gameOpts->render.particles.bPlasmaTrails && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bLightTrails, 0, 0, 0) &&
				pObj->HasLightTrail (nId))
		DoParticleTrail (pObj);
	else
		return 0;
	}
else if (t == OBJ_DEBRIS)
	DoDebrisSmoke (pObj);
else
	return 0;
return 1;
}

//------------------------------------------------------------------------------

void PlayerBulletFrame (void)
{
if (!gameOpts->render.ship.bBullets)
	return;
for (int32_t i = 0; i < N_PLAYERS; i++)
	DoPlayerBullets (OBJECT (PLAYER (i).nObject));
}

//------------------------------------------------------------------------------

void PlayerParticleFrame (void)
{
if (!gameOpts->render.particles.bPlayers)
	return;
for (int32_t i = 0; i < N_PLAYERS; i++)
	DoPlayerSmoke (OBJECT (PLAYER (i).nObject), i);
}

//------------------------------------------------------------------------------

void ObjectParticleFrame (void)
{
#if 0
if (!SHOW_SMOKE)
	return;
#endif
CObject	*pObj = OBJECTS.Buffer ();

for (int32_t i = 0; i <= gameData.objData.nLastObject [1]; i++, pObj++) {
	if (gameData.objData.bWantEffect [i] & DESTROY_SMOKE) {
		gameData.objData.bWantEffect [i] &= ~DESTROY_SMOKE;
		KillObjectSmoke (i);
		}
	else if (pObj->info.nType == OBJ_NONE)
		KillObjectSmoke (i);
	else
		DoObjectSmoke (pObj);
	}
}

//------------------------------------------------------------------------------

void StaticParticlesFrame (void)
{
	CObject* pObj;

if (!SHOW_SMOKE)
	return;
FORALL_EFFECT_OBJS (pObj) {
	if (pObj->info.nId == PARTICLE_ID)
		DoStaticParticles (pObj);
	}
}

//------------------------------------------------------------------------------

void DoParticleFrame (void)
{
//if (gameStates.render.nShadowPass > 1)
//	return;
PlayerBulletFrame ();
ObjectParticleFrame ();
StaticParticlesFrame ();
shrapnelManager.DoFrame ();
particleManager.Update ();
}

//------------------------------------------------------------------------------
