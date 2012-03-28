#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "descent.h"
#include "segmath.h"

#include "objsmoke.h"
#include "transprender.h"
#include "renderthreads.h"
#include "rendermine.h"
#include "error.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_lib.h"
#include "automap.h"
#ifdef TACTILE
#	include "tactile.h"
#endif

CLightningManager lightningManager;
//------------------------------------------------------------------------------

CLightningManager::CLightningManager ()
{
m_objects = NULL;
m_lights = NULL;
m_nFirstLight = -1;
}

//------------------------------------------------------------------------------

CLightningManager::~CLightningManager ()
{
Shutdown (true);
}

//------------------------------------------------------------------------------

void CLightningManager::Init (void)
{
if (!(m_objects.Buffer () || m_objects.Create (LEVEL_OBJECTS))) {
	Shutdown (1);
	extraGameInfo [0].bUseLightning = 0;
	return;
	}
m_objects.Clear (0xff);
if (!(m_lights.Buffer () || m_lights.Create (2 * LEVEL_SEGMENTS))) {
	Shutdown (1);
	extraGameInfo [0].bUseLightning = 0;
	return;
	}
if (!m_emitters.Create (MAX_LIGHTNING_SYSTEMS)) {
	Shutdown (1);
	extraGameInfo [0].bUseLightning = 0;
	return;
	}
m_emitterList.Create (MAX_LIGHTNING_SYSTEMS);
int i = 0;
int nCurrent = m_emitters.FreeList ();
for (CLightningEmitter* emitterP = m_emitters.GetFirst (nCurrent); emitterP; emitterP = m_emitters.GetNext (nCurrent))
	emitterP->Init (i++);
m_nFirstLight = -1;
m_bDestroy = 0;
}

//------------------------------------------------------------------------------

class CLightningSettings {
	public:
		CFixVector	vPos;
		CFixVector	vEnd;
		CFixVector	vDelta;
		int			nBolts;
		int			nLife;
		int			nFrames;
	};

//------------------------------------------------------------------------------

int CLightningManager::Create (int nBolts, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
										 short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
										 char nAngle, int nOffset, short nNodes, short nChildren, char nDepth, short nFrames,
										 short nSmoothe, char bClamp, char bGlow, char bSound, char bLight,
										 char nStyle, float nWidth, CFloatVector *colorP)
{
if (!(SHOW_LIGHTNING (1) && colorP))
	return -1;
if (!nBolts)
	return -1;
SEM_ENTER (SEM_LIGHTNING)
CLightningEmitter* emitterP = m_emitters.Pop ();
if (!emitterP)
	return -1;
if (!(emitterP->Create (nBolts, vPos, vEnd, vDelta, nObject, nLife, nDelay, nLength, nAmplitude,
							   nAngle, nOffset, nNodes, nChildren, nDepth, nFrames, nSmoothe, bClamp, bGlow, bSound, bLight,
							   nStyle, nWidth, colorP))) {
	m_emitters.Push (emitterP->Id ());
	emitterP->Destroy ();
	SEM_LEAVE (SEM_LIGHTNING)
	return -1;
	}
emitterP->m_bValid = 1;
SEM_LEAVE (SEM_LIGHTNING)
return emitterP->Id ();
}

//------------------------------------------------------------------------------

int CLightningManager::Create (tLightningInfo& li, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta, short nObject)
{
CFloatVector color;

color.Red () = float (li.color.Red ()) / 255.0f;
color.Green () = float (li.color.Green ()) / 255.0f;
color.Blue () = float (li.color.Blue ()) / 255.0f;
color.Alpha () = float (li.color.Alpha ()) / 255.0f;

return Create (li.nBolts, 
					vPos, vEnd, vDelta, 
					nObject, 
					-abs (li.nLife), 
					li.nDelay, 
					(li.nLength > 0) ? I2X (li.nLength) : CFixVector::Dist (*vPos, *vEnd),
					(li.nAmplitude > 0) ? I2X (li.nAmplitude) : -I2X (li.nAmplitude) * gameOpts->render.lightning.nStyle, 
					li.nAngle, 
					I2X (li.nOffset), 
					(li.nNodes > 0) ? li.nNodes : -li.nNodes * gameOpts->render.lightning.nStyle, 
					li.nChildren, 
					li.nChildren > 0, 
					li.nFrames,
				   li.nSmoothe, 
					li.bClamp, 
					li.bGlow, 
					li.bSound, 
					1, 
					li.nStyle, 
					(float) li.nWidth, 
					&color);
}

//------------------------------------------------------------------------------

void CLightningManager::Destroy (CLightningEmitter* emitterP, CLightning *lightningP)
{
if (lightningP)
	lightningP->Destroy ();
else
	emitterP->m_bDestroy = 1;
}

//------------------------------------------------------------------------------

void CLightningManager::Cleanup (void)
{
SEM_ENTER (SEM_LIGHTNING)
int nCurrent = -1;
for (CLightningEmitter* emitterP = m_emitters.GetFirst (nCurrent), * nextP = NULL; emitterP; emitterP = nextP) {
	nextP = m_emitters.GetNext (nCurrent);
	if (emitterP->m_bDestroy) {
		emitterP->Destroy ();
		m_emitters.Push (emitterP->Id ());
		}
	}
SEM_LEAVE (SEM_LIGHTNING)
}

//------------------------------------------------------------------------------

int CLightningManager::Shutdown (bool bForce)
{
if (!bForce && (m_bDestroy >= 0))
	m_bDestroy = 1;
else {
	uint bSem = gameData.app.semaphores [SEM_LIGHTNING];
	if (!bSem)
		SEM_ENTER (SEM_LIGHTNING)
	int nCurrent = -1;
	for (CLightningEmitter* emitterP = m_emitters.GetFirst (nCurrent), * nextP = NULL; emitterP; emitterP = nextP) {
		nextP = m_emitters.GetNext (nCurrent);
		Destroy (emitterP, NULL);
		m_emitters.Push (emitterP->Id ());
		}
	ResetLights (1);
	m_emitters.Destroy ();
	m_emitterList.Destroy ();
	m_objects.Destroy ();
	m_lights.Destroy ();
	if (!bSem)
		SEM_LEAVE (SEM_LIGHTNING)
	}
return 1;
}

//------------------------------------------------------------------------------

void CLightningManager::Move (int i, CFixVector vNewPos, CFixVector vNewEnd, short nSegment)
{
if (nSegment < 0)
	return;
if (SHOW_LIGHTNING (1))
	m_emitters [i].Move (vNewPos, vNewEnd, nSegment, 0);
}

//------------------------------------------------------------------------------

void CLightningManager::Move (int i, CFixVector vNewPos, short nSegment)
{
if (nSegment < 0)
	return;
if (SHOW_LIGHTNING (1))
	m_emitters [i].Move (vNewPos, nSegment, 0);
}

//------------------------------------------------------------------------------

void CLightningManager::MoveForObject (CObject* objP)
{
	int i = objP->Index ();

if (m_objects [i] >= 0)
	Move (m_objects [i], OBJPOS (objP)->vPos, OBJSEG (objP));
}

//------------------------------------------------------------------------------

CFloatVector *CLightningManager::LightningColor (CObject* objP)
{
if (objP->info.nType == OBJ_ROBOT) {
	if (ROBOTINFO (objP->info.nId).energyDrain) {
		static CFloatVector color = {1.0f, 0.8f, 0.3f, 0.2f};
		return &color;
		}
	}
else if ((objP->info.nType == OBJ_PLAYER) && gameOpts->render.lightning.bPlayers) {
	if (gameData.FusionCharge (objP->info.nId) > I2X (2)) {
		static CFloatVector color = {0.666f, 0.0f, 0.75f, 0.2f};
		return &color;
		}
	int f = SEGMENTS [objP->info.nSegment].m_function;
	if (f == SEGMENT_FUNC_FUELCEN) {
		static CFloatVector color = {1.0f, 0.8f, 0.3f, 0.2f};
		return &color;
		}
	if (f == SEGMENT_FUNC_REPAIRCEN) {
		static CFloatVector color = {0.1f, 0.3f, 1.0f, 0.2f};
		return &color;
		}
	}
return NULL;
}

//------------------------------------------------------------------------------

void CLightningManager::Update (void)
{
if (SHOW_LIGHTNING (1)) {

		CObject	*objP;
		ubyte		h;
		int		i;

#if LIMIT_LIGHTNING_FPS
#	if LIGHTNING_SLOWMO
		static int	t0 = 0;
		int t = gameStates.app.nSDLTicks [0] - t0;

	if (t / gameStates.gameplay.slowmo [0].fSpeed < 25)
		return;
	t0 = gameStates.app.nSDLTicks [0] + 25 - int (gameStates.gameplay.slowmo [0].fSpeed * 25);
#	else
	if (!gameStates.app.tick40fps.bTick)
		return 0;
#	endif
#endif
	for (i = 0, objP = OBJECTS.Buffer (); i <= gameData.objs.nLastObject [1]; i++, objP++) {
		if (gameData.objs.bWantEffect [i] & DESTROY_LIGHTNING) {
			gameData.objs.bWantEffect [i] &= ~DESTROY_LIGHTNING;
			DestroyForObject (objP);
			}
		}
	int nCurrent = -1;
#if USE_OPENMP // > 1
	if (gameStates.app.bMultiThreaded && m_emitterList.Buffer ()) {
		int nSystems = 0;
		CLightningEmitter* emitterP, * nextP;
		for (emitterP = m_emitters.GetFirst (nCurrent), nextP = NULL; emitterP; emitterP = nextP) {
			m_emitterList [nSystems++] = emitterP;
			nextP = m_emitters.GetNext (nCurrent);
			}
		if (nSystems > 0)
#	pragma omp parallel
			{
			int nThread = omp_get_thread_num ();
#		pragma omp for private(emitterP)
			for (int i = 0; i < nSystems; i++) {
				emitterP = m_emitterList [i];
				if (0 > emitterP->Update (nThread))
					Destroy (emitterP, NULL);
				}
			}
		}
	else 
#endif
		{
		for (CLightningEmitter* emitterP = m_emitters.GetFirst (nCurrent), * nextP = NULL; emitterP; emitterP = nextP) {
			nextP = m_emitters.GetNext (nCurrent);
			if (0 > emitterP->Update (0))
				Destroy (emitterP, NULL);
			}
		}

	FORALL_OBJS (objP, i) {
		i = objP->Index ();
		h = gameData.objs.bWantEffect [i];
		if (h & EXPL_LIGHTNING) {
			if ((objP->info.nType == OBJ_ROBOT) || (objP->info.nType == OBJ_DEBRIS) || (objP->info.nType == OBJ_REACTOR))
				CreateForBlowup (objP);
#if DBG
			else if (objP->info.nType != 255)
				PrintLog (1, "invalid effect requested\n");
#endif
			}
		else if (h & MISSILE_LIGHTNING) {
			if (objP->IsMissile ())
				CreateForMissile (objP);
#if DBG
			else if (objP->info.nType != 255)
				PrintLog (1, "invalid effect requested\n");
#endif
			}
		else if (h & ROBOT_LIGHTNING) {
			if (objP->info.nType == OBJ_ROBOT)
				CreateForRobot (objP, LightningColor (objP));
#if DBG
			else if (objP->info.nType != 255)
				PrintLog (1, "invalid effect requested\n");
#endif
			}
		else if (h & PLAYER_LIGHTNING) {
			if (objP->info.nType == OBJ_PLAYER)
				CreateForPlayer (objP, LightningColor (objP));
#if DBG
			else if (objP->info.nType != 255)
				PrintLog (1, "invalid effect requested\n");
#endif
			}
		else if (h & MOVE_LIGHTNING) {
			if ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT))
				MoveForObject (objP);
			}
		gameData.objs.bWantEffect [i] &= ~(PLAYER_LIGHTNING | ROBOT_LIGHTNING | MISSILE_LIGHTNING | EXPL_LIGHTNING | MOVE_LIGHTNING);
		}
	}
}

//------------------------------------------------------------------------------
#if 0
void MoveForObject (CObject* objP)
{
SEM_ENTER (SEM_LIGHTNING)
MoveForObjectInternal (objP);
SEM_LEAVE (SEM_LIGHTNING)
}
#endif
//------------------------------------------------------------------------------

void CLightningManager::DestroyForObject (CObject* objP)
{
	int i = objP->Index ();

if (m_objects [i] >= 0) {
	Destroy (m_emitters + m_objects [i], NULL);
	m_objects [i] = -1;
	}
int nCurrent = -1, nObject = objP->Index ();
for (CLightningEmitter* emitterP = m_emitters.GetFirst (nCurrent); emitterP; emitterP = m_emitters.GetNext (nCurrent))
	if (emitterP->m_nObject == nObject)
		Destroy (emitterP, NULL);
}

//------------------------------------------------------------------------------

void CLightningManager::DestroyForAllObjects (int nType, int nId)
{
	CObject	*objP;
	//int		i;

FORALL_OBJS (objP, i) {
	if ((objP->info.nType == nType) && ((nId < 0) || (objP->info.nId == nId)))
#if 1
		objP->RequestEffects (DESTROY_LIGHTNING);
#else
		DestroyObjectLightnings (objP);
#endif
	}
}

//------------------------------------------------------------------------------

void CLightningManager::DestroyForPlayers (void)
{
DestroyForAllObjects (OBJ_PLAYER, -1);
}

//------------------------------------------------------------------------------

void CLightningManager::DestroyForRobots (void)
{
DestroyForAllObjects (OBJ_ROBOT, -1);
}

//------------------------------------------------------------------------------

void CLightningManager::DestroyStatic (void)
{
DestroyForAllObjects (OBJ_EFFECT, LIGHTNING_ID);
}

//------------------------------------------------------------------------------

void CLightningManager::Render (void)
{
if (SHOW_LIGHTNING (1)) {
		int bStencil = ogl.StencilOff ();

	int nCurrent = -1;
#if USE_OPENMP > 1
	if (gameStates.app.bMultiThreaded && m_emitterList.Buffer ()) {
		CLightningEmitter* emitterP;
		int nSystems = 0;
		for (emitterP = m_emitters.GetFirst (nCurrent); emitterP; emitterP = m_emitters.GetNext (nCurrent))
			if (!(emitterP->m_nKey [0] | emitterP->m_nKey [1]))
				m_emitterList [nSystems++] = emitterP;
#	pragma omp parallel
			{
			int nThread = omp_get_thread_num ();
#		pragma omp for private(emitterP)
			for (int i = 0; i < nSystems; i++) {
				emitterP = m_emitterList [i];
				emitterP->Render (0, emitterP->m_nBolts, nThread);
				}
			}
		}
	else 
#endif
		{
		for (CLightningEmitter* emitterP = m_emitters.GetFirst (nCurrent); emitterP; emitterP = m_emitters.GetNext (nCurrent))
			if (!(emitterP->m_nKey [0] | emitterP->m_nKey [1]))
				emitterP->Render (0, emitterP->m_nBolts, 0);
		}
	ogl.StencilOn (bStencil);
	}
}

//------------------------------------------------------------------------------

void CLightningManager::Mute (void)
{
if (SHOW_LIGHTNING (1)) {
	int nCurrent = -1;
	for (CLightningEmitter* emitterP = m_emitters.GetFirst (nCurrent); emitterP; emitterP = m_emitters.GetNext (nCurrent))
		emitterP->Mute ();
	}
}

//------------------------------------------------------------------------------

CFixVector *CLightningManager::FindTargetPos (CObject* emitterP, short nTarget)
{
	//int		i;
	CObject*	objP;

if (!nTarget)
	return 0;
FORALL_EFFECT_OBJS (objP, i) {
	if ((objP != emitterP) && (objP->info.nId == LIGHTNING_ID) && (objP->rType.lightningInfo.nId == nTarget))
		return &objP->info.position.vPos;
	}
return NULL;
}

//------------------------------------------------------------------------------

int CLightningManager::Enable (CObject* objP)
{
int h = m_objects [objP->Index ()];
if ((h >= 0) && m_emitters [h].m_bValid)
	m_emitters [h].m_bValid = objP->rType.lightningInfo.bEnabled ? 1 : -1;
return m_emitters [h].m_bValid;
}

//------------------------------------------------------------------------------

void CLightningManager::StaticFrame (void)
{
	CObject*				objP;
	CFixVector*			vEnd, * vDelta, v;

if (!SHOW_LIGHTNING (1))
	return;
if (!gameOpts->render.lightning.bStatic)
	return;
FORALL_EFFECT_OBJS (objP, i) {
	if (objP->info.nId != LIGHTNING_ID)
		continue;
	short nObject = objP->Index ();
	if (m_objects [nObject] >= 0)
		continue;
	tLightningInfo& li = objP->rType.lightningInfo;
	if (li.nBolts <= 0)
		continue;
	if (li.bRandom && !li.nAngle)
		vEnd = NULL;
	else if ((vEnd = FindTargetPos (objP, li.nTarget)))
		li.nLength = CFixVector::Dist (objP->info.position.vPos, *vEnd) / I2X (1);
	else {
		v = objP->info.position.vPos + objP->info.position.mOrient.m.dir.f * I2X (li.nLength);
		vEnd = &v;
		}
	vDelta = li.bInPlane ? &objP->info.position.mOrient.m.dir.r : NULL;
	int nHandle = Create (li, &objP->info.position.vPos, vEnd, vDelta, nObject);
	if (nHandle >= 0) {
		m_objects [nObject] = nHandle;
		if (!objP->rType.lightningInfo.bEnabled)
			m_emitters [nHandle].m_bValid = -1;
		}
	}
}

//------------------------------------------------------------------------------

void CLightningManager::DoFrame (void)
{
if (m_bDestroy) {
	m_bDestroy = -1;
	Shutdown (0);
	}
else {
	Update ();
	omegaLightning.Update (NULL, NULL);
	StaticFrame ();
	Cleanup ();
	}
}

//------------------------------------------------------------------------------

void CLightningManager::SetSegmentLight (short nSegment, CFixVector *vPosP, CFloatVector *colorP)
{
if ((nSegment < 0) || (nSegment >= gameData.segs.nSegments))
	return;
else {
		tLightningLight	*llP = m_lights + nSegment;

#if DBG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	if (llP->nFrame != gameData.app.nFrameCount) {
		memset (llP, 0, sizeof (*llP));
		llP->nFrame = gameData.app.nFrameCount;
		llP->nSegment = nSegment;
		llP->nNext = m_nFirstLight;
		m_nFirstLight = nSegment;
		}
	llP->nLights++;
	llP->vPos += *vPosP;
	llP->color.Red () += colorP->Red ();
	llP->color.Green () += colorP->Green ();
	llP->color.Blue () += colorP->Blue ();
	llP->color.Alpha () += colorP->Alpha ();
	}
}

//------------------------------------------------------------------------------

void CLightningManager::ResetLights (int bForce)
{
if ((SHOW_LIGHTNING (1) || bForce) && m_lights.Buffer ()) {
		tLightningLight	*llP;
		int					i;

	for (i = m_nFirstLight; i >= 0; ) {
		if ((i < 0) || (i >= LEVEL_SEGMENTS))
			break;
		llP = m_lights + i;
		i = llP->nNext;
		llP->nLights = 0;
		llP->nNext = -1;
		llP->vPos.SetZero ();
		llP->color.Red () =
		llP->color.Green () =
		llP->color.Blue () = 0;
		llP->nBrightness = 0;
		if (llP->nDynLight >= 0) {
			llP->nDynLight = -1;
			}
		}
	m_nFirstLight = -1;
	lightManager.DeleteLightning ();
	}
}

//------------------------------------------------------------------------------

void CLightningManager::SetLights (void)
{
	int nLights = 0;

ResetLights (0);
if (SHOW_LIGHTNING (1)) {
		tLightningLight	*llP = NULL;
		int					i, n, bDynLighting = gameStates.render.nLightingMethod;

	m_nFirstLight = -1;
	int nCurrent = -1;

#if USE_OPENMP // > 1
	if (gameStates.app.bMultiThreaded && m_emitterList.Buffer ()) {
		CLightningEmitter* emitterP;
		int nSystems = 0;
		for (emitterP = m_emitters.GetFirst (nCurrent); emitterP; emitterP = m_emitters.GetNext (nCurrent))
			m_emitterList [nSystems++] = emitterP;
#	pragma omp parallel
			{
			//int nThread = omp_get_thread_num ();
#		pragma omp for private(emitterP) reduction(+: nLights)
			for (int i = 0; i < nSystems; i++) {
				emitterP = m_emitterList [i];
				nLights += emitterP->SetLight ();
				}
			}
		}
	else 
#endif
		for (CLightningEmitter* emitterP = m_emitters.GetFirst (nCurrent); emitterP; emitterP = m_emitters.GetNext (nCurrent))
			nLights += emitterP->SetLight ();
	if (!nLights)
		return;
	nLights = 0;
	for (i = m_nFirstLight; i >= 0; i = llP->nNext) {
		if ((i < 0) || (i >= LEVEL_SEGMENTS))
			continue;
		llP = m_lights + i;
#if DBG
		if (llP->nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		n = llP->nLights;
		llP->vPos.v.coord.x /= n;
		llP->vPos.v.coord.y /= n;
		llP->vPos.v.coord.z /= n;
		llP->color.Red () /= n;
		llP->color.Green () /= n;
		llP->color.Blue () /= n;

#if 1
		llP->nBrightness = F2X (sqrt (10 * (llP->color.Red () + llP->color.Green () + llP->color.Blue ()) * llP->color.Alpha ()));
#else
		if (gameStates.render.bPerPixelLighting == 2)
			llP->nBrightness = F2X (sqrt (10 * (llP->Red () + llP->Green () + llP->Blue ()) * llP->Alpha ()));
		else
			llP->nBrightness = F2X (10 * (llP->Red () + llP->Green () + llP->Blue ()) * llP->Alpha ());
#endif
		if (bDynLighting) {
			llP->nDynLight = lightManager.Add (NULL, &llP->color, llP->nBrightness, llP->nSegment, -1, -1, -1, &llP->vPos);
			nLights++;
			}
		}
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForExplosion (CObject* objP, CFloatVector *colorP, int nRods, int nRad, int nTTL)
{
if (SHOW_LIGHTNING (1) && gameOpts->render.lightning.bExplosions) {
	//m_objects [objP->Index ()] =
		Create (
			nRods, &objP->info.position.vPos, NULL, NULL, objP->Index (), nTTL, 0,
			nRad, I2X (4), 0, I2X (2), 50, 0, 1, 3, 1, 1, 0, 0, 1, -1, 3.0f, colorP);
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForShaker (CObject* objP)
{
static CFloatVector color = {0.1f, 0.1f, 0.8f, 0.2f};

CreateForExplosion (objP, &color, 30, I2X (20), 750);
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForShakerMega (CObject* objP)
{
static CFloatVector color = {0.1f, 0.1f, 0.6f, 0.2f};

CreateForExplosion (objP, &color, 20, I2X (15), 750);
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForMega (CObject* objP)
{
static CFloatVector color = {0.8f, 0.1f, 0.1f, 0.2f};

CreateForExplosion (objP, &color, 30, I2X (15), 750);
}

//------------------------------------------------------------------------------

int CLightningManager::CreateForMissile (CObject* objP)
{
if (objP->IsMissile ()) {
	if ((objP->info.nId == EARTHSHAKER_ID) || (objP->info.nId == EARTHSHAKER_ID))
		CreateForShaker (objP);
	else if ((objP->info.nId == EARTHSHAKER_MEGA_ID) || (objP->info.nId == ROBOT_SHAKER_MEGA_ID))
		CreateForShakerMega (objP);
	else if ((objP->info.nId == MEGAMSL_ID) || (objP->info.nId == ROBOT_MEGAMSL_ID))
		CreateForMega (objP);
	else
		return 0;
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForBlowup (CObject* objP)
{
static CFloatVector color = {0.1f, 0.1f, 0.8f, 0.2f};

int h = X2I (objP->info.xSize) * 2;

CreateForExplosion (objP, &color, h + rand () % h, h * (I2X (1) + I2X (1) / 2), 350);
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForTeleport (CObject* objP, CFloatVector *colorP, int nRods, int nRad, int nTTL)
{
if (SHOW_LIGHTNING (1) && gameOpts->render.lightning.bExplosions) {
	//m_objects [objP->Index ()] =
		Create (
			nRods, &objP->info.position.vPos, NULL, NULL, -1, nTTL, 0,
			nRad, I2X (4), 0, I2X (2), 50, 0, 1, 3, 1, 1, 0, 0, 1, -1, 3.0f, colorP);
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForPlayerTeleport (CObject* objP)
{
static CFloatVector color = {0.0f, 0.5f, 1.0f, 0.2f};

int h = X2I (objP->info.xSize) * 2;

CreateForTeleport (objP, &color, h + rand () % h, h * (I2X (1) + I2X (1) / 2), objP->LifeLeft ());
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForRobotTeleport (CObject* objP)
{
static CFloatVector color = {1.0f, 0.0f, 0.5f, 0.2f};

int h = X2I (objP->info.xSize) * 2;

CreateForTeleport (objP, &color, h + rand () % h, h * (I2X (1) + I2X (1) / 2), objP->LifeLeft ());
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForPowerupTeleport (CObject* objP)
{
static CFloatVector color = {0.0f, 1.0f, 0.5f, 0.2f};

int h = X2I (objP->info.xSize) * 2;

CreateForTeleport (objP, &color, h + rand () % h, h * (I2X (1) + I2X (1) / 2), objP->LifeLeft ());
}

//------------------------------------------------------------------------------

static tLightningInfo robotLightningInfo = {
	-5000, // nLife
	0, // nDelay
	0, // nLength
	-4, // nAmplitude
	0, // nOffset
	-1, // nWayPoint
	5, // nBolts
	-1, // nId
	-1, // nTarget
	100, // nNodes
	0, // nChildren
	3, // nFrames
	3, // nWidth
	0, // nAngle
	-1, // nStyle
	1, // nSmoothe
	1, // bClamp
	-1, // bGlow
	1, // bSound
	0, // bRandom
	0, // bInPlane
	1, // bEnabled
	0, // bDirection
	{(ubyte) (255 * 0.9f), (ubyte) (255 * 0.6f), (ubyte) (255 * 0.6f), (ubyte) (255 * 0.3f)} // color;
	};

void CLightningManager::CreateForRobot (CObject* objP, CFloatVector *colorP)
{
if (SHOW_LIGHTNING (1) && gameOpts->render.lightning.bRobots && OBJECT_EXISTS (objP)) {
		int nObject = objP->Index ();

	if (0 <= m_objects [nObject])
		MoveForObject (objP);
	else {
		robotLightningInfo.color.Set (ubyte (255 * colorP->v.color.r), ubyte (255 * colorP->v.color.g), ubyte (255 * colorP->v.color.b));
		int h = lightningManager.Create (robotLightningInfo, &objP->Position (), &OBJPOS (OBJECTS + LOCALPLAYER.nObject)->vPos, NULL, nObject);
		if (h >= 0)
			m_objects [nObject] = h;
		}
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForPlayer (CObject* objP, CFloatVector *colorP)
{
if (SHOW_LIGHTNING (1) && gameOpts->render.lightning.bPlayers && OBJECT_EXISTS (objP)) {
	int h, nObject = objP->Index ();

	if (0 <= m_objects [nObject])
		MoveForObject (objP);
	else {
		int s = objP->info.xSize;
		int i = X2I (s);
		h = Create (5 * i, &OBJPOS (objP)->vPos, NULL, NULL, objP->Index (), -250, 150,
						4 * s, s, 0, 2 * s, i * 20, (i + 1) / 2, 1, 3, 1, 1, -1, 1, 1, -1, 3.0f, colorP);
		if (h >= 0)
			m_objects [nObject] = h;
		}
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForDamage (CObject* objP, CFloatVector *colorP)
{
if (SHOW_LIGHTNING (1) && gameOpts->render.lightning.bDamage && OBJECT_EXISTS (objP)) {
	int i = objP->Index ();
	int n = X2IR (RobotDefaultShield (objP));
	if (0 >= n)
		return;
	int h = X2IR (objP->info.xShield) * 100 / n;
	if ((h < 0) || (h >= 50))
		return;
	n = (5 - h / 10) * 2;
	if (0 <= (h = m_objects [i])) {
		if (m_emitters [h].m_nBolts == n) {
			MoveForObject (objP);
			return;
			}
		Destroy (m_emitters + h, NULL);
		}
	h = Create (n, &objP->info.position.vPos, NULL, NULL, objP->Index (), -1000, 4000,
					objP->info.xSize, objP->info.xSize / 8, 0, 0, 20, 0, 1, 10, 1, 1, 0, 0, 0, -1, 3.0f, colorP);
	if (h >= 0)
		m_objects [i] = h;
	}
}

//------------------------------------------------------------------------------

int CLightningManager::FindDamageLightning (short nObject, int *pKey)
{
int nCurrent = -1;
for (CLightningEmitter* emitterP = m_emitters.GetFirst (nCurrent); emitterP; emitterP = m_emitters.GetNext (nCurrent))
	if ((emitterP->m_nObject == nObject) && (emitterP->m_nKey [0] == pKey [0]) && (emitterP->m_nKey [1] == pKey [1]))
		return emitterP->Id ();
return -1;
}

//------------------------------------------------------------------------------

typedef union tPolyKey {
	int	i [2];
	short	s [4];
} tPolyKey;

int CLightningManager::RenderForDamage (CObject* objP, CRenderPoint **pointList, RenderModel::CVertex *vertP, int nVertices)
{
	CLightningEmitter*	emitterP;
	CFloatVector			v, vPosf, vEndf, vNormf, vDeltaf;
	CFixVector				vPos, vEnd, vNorm, vDelta;
	int						h, i, j, nLife = -1, bUpdate = 0;
	short						nObject;
	tPolyKey					key;

	static short	nLastObject = -1;
	static float	fDamage;
	static int		nFrameFlipFlop = -1;

	static CFloatVector color = {0.2f, 0.2f, 1.0f, 1.0f};

if (!(SHOW_LIGHTNING (1) && gameOpts->render.lightning.bDamage))
	return -1;
if ((objP->info.nType != OBJ_ROBOT) && (objP->info.nType != OBJ_PLAYER))
	return -1;
if (nVertices < 3)
	return -1;
j = (nVertices > 4) ? 4 : nVertices;
h = (nVertices + 1) / 2;
// create a unique key for the lightning using the vertex key/index
if (pointList) {
	for (i = 0; i < j; i++)
		key.s [i] = pointList [i]->Key ();
	for (; i < 4; i++)
		key.s [i] = 0;
	}
else {
	for (i = 0; i < j; i++)
		key.s [i] = vertP [i].m_nIndex;
	for (; i < 4; i++)
		key.s [i] = 0;
	}
i = FindDamageLightning (nObject = objP->Index (), key.i);
if (i < 0) {
	// create new lightning stroke
	if ((nLastObject != nObject) || (nFrameFlipFlop != gameStates.render.nFrameFlipFlop)) {
		nLastObject = nObject;
		nFrameFlipFlop = gameStates.render.nFrameFlipFlop;
		fDamage = (0.5f - objP->Damage ()) / 500.0f;
		}
#if 1
	if (RandDouble () > fDamage)
		return 0;
#endif
	if (pointList) {
		vPos = pointList [0]->WorldPos ();
		vEnd = pointList [1 + RandShort () % (nVertices - 1)]->WorldPos ();
		vNorm = CFixVector::Normal (vPos, pointList [1]->WorldPos (), vEnd);
		vPos += vNorm * (I2X (1) / 64);
		vEnd += vNorm * (I2X (1) / 64);
		vDelta = CFixVector::Normal (vNorm, vPos, vEnd);
		h = CFixVector::Dist (vPos, vEnd);
		}
	else {
		memcpy (&vPosf, &vertP->m_vertex, sizeof (CFloatVector3));
		memcpy (&vEndf, &vertP [1 + RandShort () % (nVertices - 1)].m_vertex, sizeof (CFloatVector3));
		memcpy (&v, &vertP [1].m_vertex, sizeof (CFloatVector3));
		vNormf = CFloatVector::Normal (vPosf, v, vEndf);
		vPosf += vNormf * (1.0f / 64.0f);
		vEndf += vNormf * (1.0f / 64.0f);
		vDeltaf = CFloatVector::Normal (vNormf, vPosf, vEndf);
		h = F2X (CFloatVector::Dist (vPosf, vEndf));
		vPos.Assign (vPosf);
		vEnd.Assign (vEndf);
		}
	if (CFixVector::Dist (vPos, vEnd) < I2X (1) / 4)
		return -1;
	nLife = 1000 + RandShort () % 2000;
	i = Create (1, &vPos, &vEnd, NULL /*&vDelta*/, nObject, nLife, 0,
					h, I2X (1) / 2, 0, 0, 20, 0, 1, 5, 0, 1, -1, 0, 0, 1, 3.0f, &color);
	bUpdate = 1;
	}
if (i >= 0) {
	emitterP = m_emitters + i;
	if (bUpdate) {
		emitterP->m_nKey [0] = key.i [0];
		emitterP->m_nKey [1] = key.i [1];
		}
	if (emitterP->Lightning () && (emitterP->m_nBolts = emitterP->Lightning ()->Update (0, 0))) {
		if (nLife > 0)
			emitterP->m_tUpdate = gameStates.app.nSDLTicks [0] + nLife;
		ogl.SetFaceCulling (false);
		emitterP->Render (0, -1, -1);
		ogl.SetFaceCulling (true);
		return 1;
		}
	else {
		Destroy (m_emitters + i, NULL);
		}
	}
return 0;
}

//------------------------------------------------------------------------------
//eof
