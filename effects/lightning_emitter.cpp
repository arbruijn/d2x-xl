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

//------------------------------------------------------------------------------

void CLightningEmitter::Init (int32_t nId)
{
m_bValid = 0;
m_nId = nId;
}

//------------------------------------------------------------------------------

bool CLightningEmitter::Create (int32_t nBolts, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
										 int16_t nObject, int32_t nLife, int32_t nDelay, int32_t nLength, int32_t nAmplitude,
										 char nAngle, int32_t nOffset, int16_t nNodes, int16_t nChildren, char nDepth, int16_t nSteps,
										 int16_t nSmoothe, char bClamp, char bGlow, char bBlur, char bSound, char bLight,
										 char nStyle, float nWidth, CFloatVector *pColor)
{
ENTER (0, 0);
m_nObject = nObject;
if (!(nLife && nLength && (nNodes > 4)))
	RETURN (false)
m_nBolts = nBolts;
if (nObject >= 0)
	m_nSegment [0] =
	m_nSegment [1] = -1;
else {
	m_nSegment [0] = FindSegByPos (*vPos, -1, 1, 0);
	m_nSegment [1] = vEnd ? FindSegByPos (*vEnd, -1, 1, 0) : -1;
	}
m_bForcefield = !nDelay && (vEnd || (nAngle <= 0));
if (!m_lightning.Create (nBolts))
	RETURN (false)
m_lightning.Clear ();
CLightning l;
l.Init (vPos, vEnd, vDelta, nObject, nLife, nDelay, nLength, nAmplitude,
		  nAngle, nOffset, nNodes, nChildren, nSteps,
		  nSmoothe, bClamp, bGlow, bBlur, bLight, nStyle, nWidth, pColor, NULL, -1);

int32_t bChildren = (gameOpts->render.lightning.nStyle > 1);

#if USE_OPENMP // > 1

if (gameStates.app.bMultiThreaded) {
	int32_t bFail = 0;
	#pragma omp parallel 
		{
		int32_t nThread = omp_get_thread_num ();
		#pragma omp for
		for (int32_t i = 0; i < nBolts; i++) {
			if (bFail)
				continue;
			m_lightning [i] = l;
			if (!m_lightning [i].Create (0, nThread))
				bFail = 1;
			}
		}
	if (bFail)
		RETURN (false)
	}
else
#endif
for (int32_t i = 0; i < nBolts; i++) {
		m_lightning [i] = l;
		if (!m_lightning [i].Create (0, 0))
			RETURN (false)
		}

CreateSound (bSound);
m_nKey [0] =
m_nKey [1] = 0;
m_bDestroy = 0;
m_tUpdate = -1;
RETURN (true)
}

//------------------------------------------------------------------------------

void CLightningEmitter::Destroy (void)
{
ENTER (0, 0);
m_bValid =
m_bDestroy = 0;
DestroySound ();
m_lightning.Destroy ();	//class and d-tors will handle everything gracefully
m_nBolts = 0;
if ((m_nObject >= 0) && (lightningManager.GetObjectSystem (m_nObject) == m_nId))
	lightningManager.SetObjectSystem (m_nObject, -1);
m_nObject = -1;
LEAVE
}

//------------------------------------------------------------------------------

void CLightningEmitter::CreateSound (int32_t bSound, int32_t nThread)
{
ENTER (0, nThread);
if ((m_bSound = bSound)) {
	audio.CreateObjectSound (-1, SOUNDCLASS_GENERIC, m_nObject, 1, I2X (1) / 2, I2X (256), -1, -1, AddonSoundName (SND_ADDON_LIGHTNING), 0, 0, nThread);
	if (m_bForcefield) {
		if (0 <= (m_nSound = audio.GetSoundByName ("ff_amb_1")))
			audio.CreateObjectSound (m_nSound, SOUNDCLASS_GENERIC, m_nObject, 1, I2X (1), I2X (256), -1, -1, NULL, 0, 0, nThread);
		}
	}
else
	m_nSound = -1;
LEAVE
}

//------------------------------------------------------------------------------

void CLightningEmitter::DestroySound (void)
{
ENTER (0, 0);
if ((m_bSound > 0) & (m_nObject >= 0))
	audio.DestroyObjectSound (m_nObject);
LEAVE
}

//------------------------------------------------------------------------------

void CLightningEmitter::Animate (int32_t nStart, int32_t nBolts, int32_t nThread)
{
ENTER (0, nThread);
if (m_bValid < 1)
	LEAVE
CObject *pObj = OBJECT (m_nObject);
if (pObj && ((pObj->Type () == OBJ_ROBOT) || (pObj->Type () == OBJ_POWERUP)) && (pObj->Frame () != gameData.appData.nFrameCount - 1))
	LEAVE
if (nBolts < 0)
	nBolts = m_nBolts;
for (int32_t i = nStart; i < nBolts; i++)
	m_lightning [i].Animate (0, nThread);
LEAVE
}

//------------------------------------------------------------------------------

int32_t CLightningEmitter::SetLife (void)
{
ENTER (0, 0);
if (!m_bValid)
	RETURN (0)

	CLightning*	pLightning = m_lightning.Buffer ();
	int32_t			i;

for (i = 0; i < m_nBolts; ) {
	if (!pLightning->SetLife ()) {
		pLightning->DestroyNodes ();
		if (!--m_nBolts)
			RETURN (0)
		if (i < m_nBolts) {
			*pLightning = m_lightning [m_nBolts];
			memset (m_lightning + m_nBolts, 0, sizeof (m_lightning [m_nBolts]));
			continue;
			}
		}
	i++, pLightning++;
	}
RETURN (m_nBolts)
}

//------------------------------------------------------------------------------

int32_t CLightningEmitter::Update (int32_t nThread)
{
ENTER (0, nThread);
if (m_bDestroy) {
	Destroy ();
	RETURN (-1)
	}

if (!m_bValid)
	RETURN (0)

if (gameStates.app.nSDLTicks [0] - m_tUpdate >= 25) {
	if (m_nKey [0] || m_nKey [1])
		m_bDestroy = 1;
	else {
		m_tUpdate = gameStates.app.nSDLTicks [0];
		Animate (0, m_nBolts, nThread);
		if (!(m_nBolts = SetLife ()))
			lightningManager.Destroy (this, NULL);
		else if (m_bValid && (m_nObject >= 0)) {
			UpdateSound (nThread);
			MoveForObject ();
			}
		}
	}
RETURN (m_nBolts)
}

//------------------------------------------------------------------------------

void CLightningEmitter::Mute (void)
{
if (m_bSound)
	m_bSound = -1;
}

//------------------------------------------------------------------------------

void CLightningEmitter::UpdateSound (int32_t nThread)
{
ENTER (0, nThread);
if (m_bValid < 1) {
	LEAVE
	}
if (!m_bSound)
	LEAVE

	CLightning	*pLightning = m_lightning.Buffer ();

for (int32_t i = m_nBolts; i > 0; i--, pLightning++)
	if (pLightning->m_nNodes > 0) {
		if (m_bSound < 0)
			CreateSound (1, nThread);
		LEAVE
		}
if (m_bSound < 0)
	LEAVE
DestroySound ();
m_bSound = -1;
LEAVE
}

//------------------------------------------------------------------------------

void CLightningEmitter::Move (CFixVector vNewPos, int16_t nSegment)
{
ENTER (0, 0);
if (!m_bValid)
	LEAVE
if (nSegment < 0)
	LEAVE
if (!m_lightning.Buffer ())
	LEAVE
if (SHOW_LIGHTNING (1)) {
	for (int32_t i = 0; i < m_nBolts; i++)
		m_lightning [i].Move (vNewPos, nSegment);
	}
LEAVE
}

//------------------------------------------------------------------------------

void CLightningEmitter::Move (CFixVector vNewPos, CFixVector vNewEnd, int16_t nSegment)
{
ENTER (0, 0);
if (!m_bValid)
	LEAVE
if (nSegment < 0)
	LEAVE
if (!m_lightning.Buffer ())
	LEAVE
if (SHOW_LIGHTNING (1)) {
	for (int32_t i = 0; i < m_nBolts; i++)
		m_lightning [i].Move (vNewPos, vNewEnd, nSegment);
	}
LEAVE
}

//------------------------------------------------------------------------------

void CLightningEmitter::MoveForObject (void)
{
ENTER (0, 0);
if (!m_bValid)
	LEAVE
CObject* pObj = OBJECT (m_nObject);
if (pObj)
	Move (OBJPOS (pObj)->vPos, pObj->info.nSegment);
LEAVE
}

//------------------------------------------------------------------------------

void CLightningEmitter::Render (int32_t nStart, int32_t nBolts, int32_t nThread)
{
ENTER (0, nThread);
if (m_bValid < 1)
	LEAVE

CObject *pObj = OBJECT (m_nObject);
if (pObj && ((pObj->Type () == OBJ_ROBOT) || (pObj->Type () == OBJ_POWERUP)) && (pObj->Frame () != gameData.appData.nFrameCount))
	LEAVE

if (automap.Active () && !(gameStates.render.bAllVisited || automap.m_bFull)) {
	if (pObj) {
		if (!automap.m_visited [pObj->Segment ()])
			LEAVE
		}
	else if (!automap.m_bFull) {
		if (((m_nSegment [0] >= 0) && !automap.m_visible [m_nSegment [0]]) &&
			 ((m_nSegment [1] >= 0) && !automap.m_visible [m_nSegment [1]]))
			LEAVE
		}
	}

if (nBolts < 0)
	nBolts = m_nBolts;
for (int32_t i = nStart; i < nBolts; i++)
	m_lightning [i].Render (0, nThread);
LEAVE
}

//------------------------------------------------------------------------------

int32_t CLightningEmitter::SetLight (void)
{
ENTER (0, 0);
if (m_bValid < 1)
	RETURN (0)
if (!m_lightning.Buffer ())
	RETURN (0)

CObject *pObj = OBJECT (m_nObject);
if (!pObj || (pObj->Type () == OBJ_POWERUP))
	RETURN (0)

int32_t nLights = 0;
for (int32_t i = 0; i < m_nBolts; i++)
	nLights += m_lightning [i].SetLight ();
RETURN (nLights)
}

//------------------------------------------------------------------------------
//eof
