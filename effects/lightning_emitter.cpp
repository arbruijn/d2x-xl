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
										 int16_t nSmoothe, char bClamp, char bGlow, char bSound, char bLight,
										 char nStyle, float nWidth, CFloatVector *colorP)
{
m_nObject = nObject;
if (!(nLife && nLength && (nNodes > 4)))
	return false;
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
	return false;
m_lightning.Clear ();
CLightning l;
l.Init (vPos, vEnd, vDelta, nObject, nLife, nDelay, nLength, nAmplitude,
		  nAngle, nOffset, nNodes, nChildren, nSteps,
		  nSmoothe, bClamp, bGlow, bLight, nStyle, nWidth, colorP, NULL, -1);

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
			if (!m_lightning [i].Create (bChildren, nThread))
				bFail = 1;
			}
		}
	if (bFail)
		return false;
	}
else
#endif
for (int32_t i = 0; i < nBolts; i++) {
		m_lightning [i] = l;
		if (!m_lightning [i].Create (bChildren, 0))
			return false;
		}

CreateSound (bSound);
m_nKey [0] =
m_nKey [1] = 0;
m_bDestroy = 0;
m_tUpdate = -1;
return true;
}

//------------------------------------------------------------------------------

void CLightningEmitter::Destroy (void)
{
m_bValid =
m_bDestroy = 0;
DestroySound ();
m_lightning.Destroy ();	//class and d-tors will handle everything gracefully
m_nBolts = 0;
if ((m_nObject >= 0) && (lightningManager.GetObjectSystem (m_nObject) == m_nId))
	lightningManager.SetObjectSystem (m_nObject, -1);
m_nObject = -1;
}

//------------------------------------------------------------------------------

void CLightningEmitter::CreateSound (int32_t bSound, int32_t nThread)
{
if ((m_bSound = bSound)) {
	audio.CreateObjectSound (-1, SOUNDCLASS_GENERIC, m_nObject, 1, I2X (1) / 2, I2X (256), -1, -1, AddonSoundName (SND_ADDON_LIGHTNING), 0, 0, nThread);
	if (m_bForcefield) {
		if (0 <= (m_nSound = audio.GetSoundByName ("ff_amb_1")))
			audio.CreateObjectSound (m_nSound, SOUNDCLASS_GENERIC, m_nObject, 1, I2X (1), I2X (256), -1, -1, NULL, 0, 0, nThread);
		}
	}
else
	m_nSound = -1;
}

//------------------------------------------------------------------------------

void CLightningEmitter::DestroySound (void)
{
if ((m_bSound > 0) & (m_nObject >= 0))
	audio.DestroyObjectSound (m_nObject);
}

//------------------------------------------------------------------------------

void CLightningEmitter::Animate (int32_t nStart, int32_t nBolts, int32_t nThread)
{
if (m_bValid < 1)
	return;
if (nBolts < 0)
	nBolts = m_nBolts;
for (int32_t i = nStart; i < nBolts; i++)
	m_lightning [i].Animate (0, nThread);
}

//------------------------------------------------------------------------------

int32_t CLightningEmitter::SetLife (void)
{
if (!m_bValid)
	return 0;

	CLightning*	lightningP = m_lightning.Buffer ();
	int32_t			i;

for (i = 0; i < m_nBolts; ) {
	if (!lightningP->SetLife ()) {
		lightningP->DestroyNodes ();
		if (!--m_nBolts)
			return 0;
		if (i < m_nBolts) {
			*lightningP = m_lightning [m_nBolts];
			memset (m_lightning + m_nBolts, 0, sizeof (m_lightning [m_nBolts]));
			continue;
			}
		}
	i++, lightningP++;
	}
return m_nBolts;
}

//------------------------------------------------------------------------------

int32_t CLightningEmitter::Update (int32_t nThread)
{
if (m_bDestroy) {
	Destroy ();
	return -1;
	}

if (!m_bValid)
	return 0;

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
return m_nBolts;
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
if (m_bValid < 1) {
	return;
	}
if (!m_bSound)
	return;

	CLightning	*lightningP;
	int32_t			i;

for (i = m_nBolts, lightningP = m_lightning.Buffer (); i > 0; i--, lightningP++)
	if (lightningP->m_nNodes > 0) {
		if (m_bSound < 0)
			CreateSound (1, nThread);
		return;
		}
if (m_bSound < 0)
	return;
DestroySound ();
m_bSound = -1;
}

//------------------------------------------------------------------------------

void CLightningEmitter::Move (CFixVector vNewPos, int16_t nSegment)
{
if (!m_bValid)
	return;
if (nSegment < 0)
	return;
if (!m_lightning.Buffer ())
	return;
if (SHOW_LIGHTNING (1)) {
	for (int32_t i = 0; i < m_nBolts; i++)
		m_lightning [i].Move (vNewPos, nSegment);
	}
}

//------------------------------------------------------------------------------

void CLightningEmitter::Move (CFixVector vNewPos, CFixVector vNewEnd, int16_t nSegment)
{
if (!m_bValid)
	return;
if (nSegment < 0)
	return;
if (!m_lightning.Buffer ())
	return;
if (SHOW_LIGHTNING (1)) {
	for (int32_t i = 0; i < m_nBolts; i++)
		m_lightning [i].Move (vNewPos, vNewEnd, nSegment);
	}
}

//------------------------------------------------------------------------------

void CLightningEmitter::MoveForObject (void)
{
if (!m_bValid)
	return;

	CObject* objP = gameData.Object (m_nObject);

Move (OBJPOS (objP)->vPos, objP->info.nSegment);
}

//------------------------------------------------------------------------------

void CLightningEmitter::Render (int32_t nStart, int32_t nBolts, int32_t nThread)
{
if (m_bValid < 1)
	return;

if (automap.Active () && !(gameStates.render.bAllVisited || automap.m_bFull)) {
	if (m_nObject >= 0) {
		if (!automap.m_visited [gameData.Object (m_nObject)->Segment ()])
			return;
		}
	else if (!automap.m_bFull) {
		if (((m_nSegment [0] >= 0) && !automap.m_visible [m_nSegment [0]]) &&
			 ((m_nSegment [1] >= 0) && !automap.m_visible [m_nSegment [1]]))
			return;
		}
	}

if (nBolts < 0)
	nBolts = m_nBolts;
for (int32_t i = nStart; i < nBolts; i++)
	m_lightning [i].Render (0, nThread);
}

//------------------------------------------------------------------------------

int32_t CLightningEmitter::SetLight (void)
{
if (m_bValid < 1)
	return 0;
if (!m_lightning.Buffer ())
	return 0;

int32_t nLights = 0;
for (int32_t i = 0; i < m_nBolts; i++)
	nLights += m_lightning [i].SetLight ();
return nLights;
}

//------------------------------------------------------------------------------
//eof
