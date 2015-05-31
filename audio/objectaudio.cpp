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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "fix.h"
#include "mono.h"
#include "timer.h"
#include "joy.h"
#include "key.h"
#include "newdemo.h"
#include "error.h"
#include "text.h"
#include "kconfig.h"
#include "segmath.h"
#include "midi.h"
#include "network.h"
#include "lightning.h"
#include "audio.h"
#include "automap.h"

//------------------------------------------------------------------------------

void CSoundObject::Init (void)
{
memset (this, 0, sizeof (*this));
m_audioVolume =
m_channel = -1;
}

//------------------------------------------------------------------------------

void CSoundObject::Stop (void)
{
if (m_channel > -1) {
	audio.StopSound (m_channel);
	m_channel = -1;
	audio.DeactivateObject ();
	}
}

//------------------------------------------------------------------------------
//hack to not start CObject when loading level

bool CSoundObject::Start (void)
{
	// start sample structures
m_channel = -1;
if (m_volume <= 0)
	return false;
if (gameStates.sound.bDontStartObjects)
	return false;
if ((m_nSound < 0) && !*m_szSound)
	return false;
// only use up to 1/4 the sound channels for "permanent" sounts
if ((m_flags & SOF_PERMANENT) && (audio.ActiveObjects () >= Max (1, 33 * audio.GetMaxChannels () / 100)) && !audio.SuspendObjectSound (m_volume))
	return false;
// start the sample playing
m_channel =
	audio.StartSound (m_nSound, m_soundClass, m_volume, m_pan, (m_flags & SOF_PLAY_FOREVER) != 0, m_nLoopStart, m_nLoopEnd,
							int32_t (this - audio.Objects ().Buffer ()), I2X (1), m_szSound,
							(m_flags & SOF_LINK_TO_OBJ) ? &OBJECT (m_linkType.obj.nObject)->info.position.vPos : &m_linkType.pos.position);
if (m_channel < 0)
	return false;
audio.ActivateObject ();
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/* Find the sound which actually equates to a sound number */
int16_t CAudio::XlatSound (int16_t nSound)
{
if (nSound < 0)
	return -1;
if (m_info.bLoMem) {
	nSound = AltSounds [gameStates.sound.bD1Sound][nSound];
	if (nSound == 255)
		return -1;
	}
//Assert (Sounds [gameStates.sound.bD1Sound][nSound] != 255);	//if hit this, probably using undefined sound
if (Sounds [gameStates.sound.bD1Sound][nSound] == 255)
	return -1;
return Sounds [gameStates.sound.bD1Sound][nSound];
}

//------------------------------------------------------------------------------

int32_t CAudio::UnXlatSound (int32_t nSound)
{
	int32_t i;
	uint8_t *table = (m_info.bLoMem ? AltSounds [gameStates.sound.bD1Sound] : Sounds [gameStates.sound.bD1Sound]);

if (nSound < 0)
	return -1;
for (i = 0; i < MAX_SOUNDS; i++)
	if (table [i] == nSound)
		return i;
Int3 ();
return 0;
}

//------------------------------------------------------------------------------

int32_t CAudio::GetSoundByName (const char* pszSound)
{
	char	szSound [FILENAME_LEN];
	int32_t	nSound;

strcpy (szSound, pszSound);
nSound = PiggyFindSound (szSound);
return (nSound < 0) ? -1 : CAudio::UnXlatSound (nSound);
}

//------------------------------------------------------------------------------
// distances will be scaled by a factor of 1.0 + 0.6 * distance / maxDistance
// 1.6 is an empirically determined factor reflecting that sound cannot usually travel on a direct line to the listener
// The fractional part is scaled with the relation of the distance to the possible max. distance
// to reflect that on shorter distances sound may be able to travel along a more direct path than on longer distances
// This method leads to a uniform, plausible effect audio experience during gameplay

int32_t CAudio::Distance (CFixVector& vListenerPos, int16_t nListenerSeg, CFixVector& vSoundPos, int16_t nSoundSeg, fix maxDistance, int32_t nDecay, CFixVector& vecToSound, int32_t nThread)
{
#if 0 //DBG
	static float fCorrFactor = 1.0f;
	static uint32_t  nRouteCount = 1;
#endif

if (nDecay)
	maxDistance *= 2;
else
	maxDistance = (5 * maxDistance) / 4;	// Make all sounds travel 1.25 times as far.

fix distance = CFixVector::NormalizedDir (vecToSound, vSoundPos, vListenerPos);
if (distance > maxDistance) 
	return -1;
if (nListenerSeg == nSoundSeg)
	return distance;
if (!HaveRouter (nThread)) {
	int32_t nSearchSegs = X2I (maxDistance / 10);
	if (nSearchSegs < 3)
		nSearchSegs = 3;
	return uniDacsRouter [nThread].PathLength (vListenerPos, nListenerSeg, vSoundPos, nSoundSeg, nSearchSegs, WID_TRANSPARENT_FLAG | WID_PASSABLE_FLAG, 0);
	}

if (m_nListenerSeg != nListenerSeg) 
	m_nListenerSeg = nListenerSeg;
if ((m_nListenerSeg != m_router [nThread].StartSeg ()) || (m_router [nThread].DestSeg () > -1)) { // either we had a different start last time, or the last calculation was a 1:1 routing
	m_router [nThread].PathLength (CFixVector::ZERO, nListenerSeg, CFixVector::ZERO, -1, /*I2X (5 * 256 / 4)*/maxDistance, WID_TRANSPARENT_FLAG | WID_PASSABLE_FLAG, -1);
#if 0 //DBG
	for (int32_t i = 0; i < gameData.segData.nSegments; i++) {
		fix pathDistance = m_router [nThread].Distance (i);
		if (pathDistance <= 0)
			continue;
		int16_t l = m_router [nThread].RouteLength (nSoundSeg);
		if (l < 3)
			continue;
		CSegment* pSeg = SEGMENT (nListenerSeg);
		int16_t nChild = m_router [nThread].Route (1)->nNode;
		fix corrStart = CFixVector::Dist (vListenerPos, SEGMENT (nChild)->Center ()) - pSeg->m_childDists [0][pSeg->ChildIndex (nChild)];
		pSeg = SEGMENT (nSoundSeg);
		nChild = m_router [nThread].Route (l - 2)->nNode;
		fix corrEnd = CFixVector::Dist (vSoundPos, SEGMENT (nChild)->Center ()) - pSeg->m_childDists [0][pSeg->ChildIndex (nChild)];
		if (corrStart + corrEnd < pathDistance)
		pathDistance += corrStart + corrEnd;
		if (pathDistance <= 0)
			continue;
		fCorrFactor += float (pathDistance) / float (distance);
		++nRouteCount;
		}
#endif
	}

fix pathDistance = m_router [nThread].Distance (nSoundSeg);
if (pathDistance < 0)
	return -1;
if (gameData.segData.SegVis (nListenerSeg, nSoundSeg))
	distance += fix (distance * 0.6f * float (distance) / float (maxDistance));
else {
	int16_t l = m_router [nThread].RouteLength (nSoundSeg);
	if (l > 2) {
		CSegment* pSeg = SEGMENT (nListenerSeg);
		int16_t nChild = m_router [nThread].Route (1)->nNode;
		pathDistance -= pSeg->m_childDists [0][pSeg->ChildIndex (nChild)];
		pathDistance += CFixVector::Dist (vListenerPos, SEGMENT (nChild)->Center ());
		pSeg = SEGMENT (nSoundSeg);
		nChild = m_router [nThread].Route (l - 2)->nNode;
		pathDistance -= pSeg->m_childDists [0][pSeg->ChildIndex (nChild)];
		pathDistance += CFixVector::Dist (vSoundPos, SEGMENT (nChild)->Center ());
		if (pathDistance > 0)
			distance = pathDistance;
		//fCorrFactor += float (pathDistance) / float (distance);
		//++nRouteCount;
		}
	}
return (distance < maxDistance) ? distance : -1;
}

//------------------------------------------------------------------------------
// determine nVolume and panning of a sound created at location nSoundSeg,vSoundPos
// as heard by nListenerSeg,vListenerPos

void CAudio::GetVolPan (CFixMatrix& mListener, CFixVector& vListenerPos, int16_t nListenerSeg,
								CFixVector& vSoundPos, int16_t nSoundSeg,
								fix maxVolume, int32_t *nVolume, int32_t *pan, fix maxDistance, int32_t nDecay,
								int32_t nThread)
{

*nVolume = 0;
*pan = 0;

CFixVector vecToSound;
fix distance = Distance (vListenerPos, nListenerSeg, vSoundPos, nSoundSeg, maxDistance, nDecay, vecToSound, nThread);
if (distance < 0)
	return;

if (!nDecay)
	//*nVolume = FixMulDiv (maxVolume, maxDistance - distance, maxDistance);
	*nVolume = fix (float (maxVolume) * (1.0f - float (distance) / float (maxDistance)));
else if (nDecay == 1) {
	float fDecay = (float) exp (-log (2.0f) * 4.0f * X2F (distance) / X2F (maxDistance / 2));
	*nVolume = (int32_t) (maxVolume * fDecay);
	}
else {
	float fDecay = 1.0f - X2F (distance) / X2F (maxDistance);
	*nVolume = (int32_t) (maxVolume * fDecay * fDecay * fDecay);
	}

if (*nVolume <= 0)
	*nVolume = 0;
else {
	fix angleFromEar = CFixVector::DeltaAngleNorm (mListener.m.dir.r, vecToSound, &mListener.m.dir.u);
	fix cosAng, sinAng;
	FixSinCos (angleFromEar, &sinAng, &cosAng);
	if (gameConfig.bReverseChannels != gameOpts->UseHiresSound ())
		cosAng = -cosAng;
	*pan = (cosAng + I2X (1)) / 2;
	}
}

//------------------------------------------------------------------------------

int32_t CAudio::PlaySound (int16_t nSound, int32_t nSoundClass, fix nVolume, int32_t nPan, int32_t bNoDups, int32_t nLoops, const char* pszWAV, CFixVector* vPos)
{
#if 0
if (vPos && (nVolume < 10))
	return;
#endif
if (!nVolume)
	return -1;
if (!pszWAV) {
	if (gameData.demoData.nState == ND_STATE_RECORDING) {
		if (bNoDups)
			NDRecordSound3DOnce (nSound, nPan, nVolume);
		else
			NDRecordSound3D (nSound, nPan, nVolume);
		}
	nSound = (nSound < 0) ? -nSound : CAudio::XlatSound (nSound);
	if (nSound < 0)
		return -1;
	int32_t nChannel = FindChannel (nSound);
	if (nChannel > -1)
		StopSound (nChannel);
	}
// start the sample playing
return StartSound (nSound, nSoundClass, nVolume, nPan, 0, (nLoops > 0) ? nLoops - 1 : -1, -1, -1, I2X (1), pszWAV, vPos);
}

//------------------------------------------------------------------------------

void CAudio::InitSounds (void)
{
soundQueue.Init ();
StopAllChannels ();
for (uint32_t i = 0; i < m_objects.ToS (); i++) {
	m_objects [i].m_channel = -1;
	m_objects [i].m_flags = 0;	// Mark as dead, so some other sound can use this sound
	}
m_info.nActiveObjects = 0;
m_info.bInitialized = 1;
}

//------------------------------------------------------------------------------

// plays a sample that loops bForever.
// Call digi_stop_channe (channel) to stop it.
// Call CAudio::SetChannelVolume (channel, nVolume) to change nVolume.
// if nLoopStart is -1, entire sample loops
// Returns the channel that sound is playing on, or -1 if can't play.
// This could happen because of no sound drivers loaded or not enough channels.

void CAudio::StartLoopingSound (void)
{
if (m_info.nLoopingSound > -1)
	m_info.nLoopingChannel  = StartSound (m_info.nLoopingSound, SOUNDCLASS_GENERIC, m_info.nLoopingVolume, DEFAULT_PAN, 1, m_info.nLoopingStart, m_info.nLoopingEnd);
}

//------------------------------------------------------------------------------

void CAudio::PlayLoopingSound (int16_t nSound, fix nVolume, int32_t nLoopStart, int32_t nLoopEnd)
{
nSound = CAudio::XlatSound (nSound);
if (nSound < 0)
	return;
if (m_info.nLoopingChannel > -1)
	StopSound (m_info.nLoopingChannel);
m_info.nLoopingSound = (int16_t) nSound;
m_info.nLoopingVolume = (int16_t) nVolume;
m_info.nLoopingStart = (int16_t) nLoopStart;
m_info.nLoopingEnd = (int16_t) nLoopEnd;
StartLoopingSound ();
}

//------------------------------------------------------------------------------

void CAudio::ChangeLoopingVolume (fix nVolume)
{
if (m_info.nLoopingChannel > -1)
	SetVolume (m_info.nLoopingChannel, nVolume);
m_info.nLoopingVolume = (int16_t) nVolume;
}

//------------------------------------------------------------------------------

void CAudio::StopLoopingSound (void)
{
if (m_info.nLoopingChannel > -1)
	StopSound (m_info.nLoopingChannel);
m_info.nLoopingChannel = -1;
m_info.nLoopingSound = -1;
}

//------------------------------------------------------------------------------

void CAudio::PauseLoopingSound (void)
{
if (m_info.nLoopingChannel > -1)
	StopSound (m_info.nLoopingChannel);
m_info.nLoopingChannel = -1;
}

//------------------------------------------------------------------------------

void CAudio::ResumeLoopingSound (void)
{
StartLoopingSound ();
}

//------------------------------------------------------------------------------
// find sound object with lowest volume. If found object's volume <= nThreshold,
// stop sound object to free up a sound channel for another (louder) one.

bool CAudio::SuspendObjectSound (int32_t nThreshold)
{
	int32_t	j = -1, nMinVolume = I2X (1000);

for (int32_t i = 0; i < int32_t (m_objects.ToS ()); i++) {
	if (m_objects [i].m_channel < 0)
		continue;
	if (nMinVolume > m_objects [i].m_volume) {
		nMinVolume = m_objects [i].m_volume;
		j = i;
		}
	}
if (nMinVolume > nThreshold)
	return false;
m_objects [j].Stop ();
return true;
}

//------------------------------------------------------------------------------
//sounds longer than this get their 3d aspects updated
//#define SOUND_3D_THRESHHOLD  (gameOpts->sound.audioSampleRate * 3 / 2)	//1.5 seconds

int32_t CAudio::CreateObjectSound (
	int16_t nOrgSound, int32_t nSoundClass, int16_t nObject, int32_t bForever, fix maxVolume, fix maxDistance,
	int32_t nLoopStart, int32_t nLoopEnd, const char* pszSound, int32_t nDecay, uint8_t bCustom, int32_t nThread)
{
	CObject*			pObj;
	CSoundObject*	pSoundObj;
	int16_t			nSound = 0;

if (maxVolume < 0)
	return -1;
if ((nObject < 0) || (nObject > gameData.objData.nLastObject [0]))
	return -1;
if (!(pszSound && *pszSound)) {
	nSound = XlatSound (nOrgSound);
	if (nSound < 0)
		return -1;
	if (!gameData.pigData.sound.sounds [gameStates.sound.bD1Sound][nSound].data) {
		Int3 ();
		return -1;
		}
	}
pObj = OBJECT (nObject);
if (!bForever) { 	// Hack to keep sounds from building up...
	int32_t nVolume, nPan;
	GetVolPan (gameData.objData.pViewer->info.position.mOrient, gameData.objData.pViewer->info.position.vPos,
				  gameData.objData.pViewer->info.nSegment, pObj->info.position.vPos, pObj->info.nSegment, maxVolume, &nVolume, &nPan,
				  maxDistance, nDecay, nThread);
	PlaySound (nOrgSound, nSoundClass, nVolume, nPan, 0, -1, pszSound, &pObj->info.position.vPos);
	return -1;
	}
if (gameData.demoData.nState == ND_STATE_RECORDING)
	NDRecordCreateObjectSound (nOrgSound, nObject, maxVolume, maxDistance, nLoopStart, nLoopEnd);
if (!m_objects.Grow ())
	return -1;
pSoundObj = m_objects.Top ();
pSoundObj->m_nSignature = m_info.nNextSignature++;
pSoundObj->m_flags = SOF_USED | SOF_LINK_TO_OBJ;
if (bForever)
	pSoundObj->m_flags |= SOF_PLAY_FOREVER;
pSoundObj->m_linkType.obj.nObject = nObject;
pSoundObj->m_linkType.obj.nObjSig = pObj->info.nSignature;
pSoundObj->m_maxVolume = maxVolume;
pSoundObj->m_maxDistance = maxDistance;
pSoundObj->m_soundClass = nSoundClass;
pSoundObj->m_bAmbient = (nSoundClass == SOUNDCLASS_AMBIENT);
pSoundObj->m_volume = 0;
pSoundObj->m_pan = 0;
pSoundObj->m_nSound = (pszSound && *pszSound) ? -1 : nSound;
pSoundObj->m_nDecay = nSoundClass;
if (pszSound)
	strncpy (pSoundObj->m_szSound, pszSound, sizeof (pSoundObj->m_szSound));
else
	*(pSoundObj->m_szSound) = '\0';
pSoundObj->m_nLoopStart = nLoopStart;
pSoundObj->m_nLoopEnd = nLoopEnd;
if (gameStates.sound.bDontStartObjects) { 		//started at level start
	pSoundObj->m_flags |= SOF_PERMANENT;
	pSoundObj->m_channel = -1;
	}
else {
	GetVolPan (
		gameData.objData.pViewer->info.position.mOrient, gameData.objData.pViewer->info.position.vPos,
		gameData.objData.pViewer->info.nSegment, pObj->info.position.vPos, pObj->info.nSegment, pSoundObj->m_maxVolume,
      &pSoundObj->m_volume, &pSoundObj->m_pan, pSoundObj->m_maxDistance, pSoundObj->m_nDecay, nThread);
	pSoundObj->Start ();
	// If it's a one-shot sound effect, and it can't start right away, then
	// just cancel it and be done with it.
	if ((pSoundObj->m_channel < 0) && (!(pSoundObj->m_flags & SOF_PLAY_FOREVER))) {
		m_objects.Pop ();
		return -1;
		}
	}
pSoundObj->m_bCustom = bCustom;
#if DBG
if (nOrgSound == SOUND_AFTERBURNER_IGNITE)
	nDbgChannel = pSoundObj->m_channel;
#endif
return pSoundObj->m_nSignature;
}

//------------------------------------------------------------------------------

int32_t CAudio::ChangeObjectSound (int32_t nObject, fix nVolume)
{

	CSoundObject*	pSoundObj = m_objects.Buffer ();

if (gameData.demoData.nState == ND_STATE_RECORDING)
	NDRecordDestroySoundObject (nObject);

for (uint32_t i = 0; i < m_objects.ToS (); i++, pSoundObj++) {
	if ((pSoundObj->m_flags & (SOF_USED | SOF_LINK_TO_OBJ)) == (SOF_USED | SOF_LINK_TO_OBJ)) {
		if ((pSoundObj->m_linkType.obj.nObject == nObject) && (pSoundObj->m_channel > -1)) {
			SetVolume (pSoundObj->m_channel, pSoundObj->m_volume = nVolume);
			return 1;
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void CAudio::Cleanup (void)
{
	int32_t				h, i;
	CAudioChannel* pChannel;

h = int32_t (m_objects.ToS ());

for (i = int32_t (m_usedChannels.ToS ()); i; ) {
	pChannel = m_channels + m_usedChannels [--i];
	if (pChannel->SoundObject () >= h)
		pChannel->Stop ();
	}

while (h) {
	i = m_objects [--h].m_channel;
	if ((i >= 0) && (m_channels [i].SoundObject () != h))
		DeleteSoundObject (h);
	}
}

//------------------------------------------------------------------------------

void CAudio::DeleteSoundObject (int32_t i)
{
	int32_t	h;

#if DBG
	CSoundObject	o = m_objects [i];
#endif

if ((i < int32_t (m_objects.ToS ()) - 1) && ((h = m_objects.Top ()->m_channel) >= 0)) {
#if DBG
	if (m_channels [h].SoundObject () != int32_t (m_objects.ToS ()) - 1)
		BRP;
	else
#endif
	m_channels [h].SetSoundObj (i);
	}

	CSoundObject*	pSoundObj = m_objects + i;

pSoundObj->Stop ();
pSoundObj->m_linkType.obj.nObject = -1;
pSoundObj->m_linkType.obj.nObjSig = -1;
pSoundObj->m_flags = 0;	// Mark as dead, so some other sound can use this sound
m_objects.Delete (i);
}

//------------------------------------------------------------------------------

int32_t CAudio::FindObjectSound (int32_t nObject, int16_t nSound)
{
	CSoundObject*	pSoundObj = m_objects.Buffer ();

if (nSound >= 0)
	nSound = XlatSound (nSound);
for (uint32_t i = m_objects.ToS (); i; i--, pSoundObj++) 
	if ((pSoundObj->m_linkType.obj.nObject == nObject) && (pSoundObj->m_nSound == nSound))
		return int32_t (m_objects.ToS () - i);
return -1;
}

//------------------------------------------------------------------------------

int32_t CAudio::DestroyObjectSound (int32_t nObject, int16_t nSound)
{
	int32_t nKilled = 0;

if ((nObject >= 0) && (gameData.demoData.nState == ND_STATE_RECORDING))
	NDRecordDestroySoundObject (nObject);

if (nObject == LOCALPLAYER.nObject)
	gameData.multiplayer.bMoving = -1;

if (nSound >= 0)
	nSound = XlatSound (nSound);

#pragma omp critical
{
	uint32_t i = m_objects.ToS ();
	CSoundObject*	pSoundObj = m_objects.Buffer () + i;

while (i) {
	i--;
	pSoundObj--;
	if ((pSoundObj->m_flags & (SOF_USED | SOF_LINK_TO_OBJ)) != (SOF_USED | SOF_LINK_TO_OBJ))
		continue;
	if (nObject < 0) { // kill all sounds belonging to disappeared objects
		CObject* pParent = OBJECT (pSoundObj->m_linkType.obj.nObject);
		if (pParent && (pParent->Signature () == pSoundObj->m_linkType.obj.nObjSig))
			continue;
		}
	else {
		if (pSoundObj->m_linkType.obj.nObject != nObject)
			continue;
		}
	if ((nObject < 0) && (pSoundObj->m_flags & SOF_PLAY_FOREVER))
		pSoundObj->Stop ();
	else if ((nSound < 0) || (pSoundObj->m_nSound == nSound)) {
		DeleteSoundObject (i);
		nKilled++;
		}
	}
}
return (nKilled > 0);
}

//------------------------------------------------------------------------------

int32_t CAudio::CreateSegmentSound (
	int16_t nOrgSound, int16_t nSegment, int16_t nSide, CFixVector& vPos, int32_t bForever,
	fix maxVolume, fix maxDistance, const char* pszSound)
{

	int32_t			nSound;
	CSoundObject*	pSoundObj;

nSound = XlatSound (nOrgSound);
if (maxVolume < 0)
	return -1;
//	if (maxVolume > I2X (1)) maxVolume = I2X (1);
if (nSound < 0)
	return -1;
if (!gameData.pigData.sound.sounds [gameStates.sound.bD1Sound][nSound].data) {
	Int3 ();
	return -1;
	}
if ((nSegment < 0)|| (nSegment > gameData.segData.nLastSegment))
	return -1;
if (!bForever) { 	//&& gameData.pigData.sound.sounds [nSound - SOUND_OFFSET].length < SOUND_3D_THRESHHOLD) {
	// Hack to keep sounds from building up...
	int32_t nVolume, nPan;
	GetVolPan (gameData.objData.pViewer->info.position.mOrient, gameData.objData.pViewer->info.position.vPos, gameData.objData.pViewer->info.nSegment,
				  vPos, nSegment, maxVolume, &nVolume, &nPan, maxDistance, 0);
	PlaySound (nOrgSound, SOUNDCLASS_GENERIC, nVolume, nPan, 0, -1, pszSound, &vPos);
	return -1;
	}
if (m_objects.ToS () == MAX_SOUND_OBJECTS)
	return -1;
if (!m_objects.Grow ())
	return -1;
pSoundObj = m_objects.Top ();
pSoundObj->m_nSignature = m_info.nNextSignature++;
pSoundObj->m_flags = SOF_USED | SOF_LINK_TO_POS;
if (bForever)
	pSoundObj->m_flags |= SOF_PLAY_FOREVER;
pSoundObj->m_linkType.pos.nSegment = nSegment;
pSoundObj->m_linkType.pos.nSide = nSide;
pSoundObj->m_linkType.pos.position = vPos;
pSoundObj->m_nSound = nSound;
pSoundObj->m_soundClass = SOUNDCLASS_GENERIC;
pSoundObj->m_maxVolume = maxVolume;
pSoundObj->m_maxDistance = maxDistance;
if (pszSound)
	strncpy (pSoundObj->m_szSound, pszSound, sizeof (pSoundObj->m_szSound));
else
	*pSoundObj->m_szSound = '\0';
pSoundObj->m_volume = 0;
pSoundObj->m_pan = 0;
pSoundObj->m_nDecay = 0;
pSoundObj->m_nLoopStart = pSoundObj->m_nLoopEnd = -1;
if (gameStates.sound.bDontStartObjects) {		//started at level start
	pSoundObj->m_flags |= SOF_PERMANENT;
	pSoundObj->m_channel = -1;
	}
else {
	GetVolPan (
		gameData.objData.pViewer->info.position.mOrient, gameData.objData.pViewer->info.position.vPos,
		gameData.objData.pViewer->info.nSegment, pSoundObj->m_linkType.pos.position,
		pSoundObj->m_linkType.pos.nSegment, pSoundObj->m_maxVolume, &pSoundObj->m_volume, &pSoundObj->m_pan, pSoundObj->m_maxDistance, pSoundObj->m_nDecay);
	pSoundObj->Start ();
	// If it's a one-shot sound effect, and it can't start right away, then
	// just cancel it and be done with it.
	if ((pSoundObj->m_channel < 0) && (!(pSoundObj->m_flags & SOF_PLAY_FOREVER))) {
		m_objects.Pop ();
		return -1;
		}
	}
return pSoundObj->m_nSignature;
}

//------------------------------------------------------------------------------
//if nSound==-1, kill any sound
int32_t CAudio::DestroySegmentSound (int16_t nSegment, int16_t nSide, int16_t nSound)
{
if (nSound != -1)
	nSound = XlatSound (nSound);

	int32_t nKilled = 0;
	uint32_t i = m_objects.ToS ();
	CSoundObject*	pSoundObj = m_objects.Buffer () + i;

while (i) {
	i--;
	pSoundObj--;
	if ((pSoundObj->m_flags & (SOF_USED | SOF_LINK_TO_POS)) == (SOF_USED | SOF_LINK_TO_POS)) {
		if ((pSoundObj->m_linkType.pos.nSegment == nSegment) && (pSoundObj->m_linkType.pos.nSide == nSide) &&
			 ((nSound == -1) || (pSoundObj->m_nSound == nSound))) {
			DeleteSoundObject (i);
			nKilled++;
			}
		}
	}
return (nKilled > 0);
}

//------------------------------------------------------------------------------
//	John's new function, 2/22/96.
void CAudio::RecordSoundObjects (void)
{
for (uint32_t i = 0; i < m_objects.ToS (); i++) {
	if ((m_objects [i].m_flags & (SOF_USED | SOF_LINK_TO_OBJ | SOF_PLAY_FOREVER)) == (SOF_USED | SOF_LINK_TO_OBJ | SOF_PLAY_FOREVER)) {
		NDRecordCreateObjectSound (UnXlatSound (m_objects [i].m_nSound), m_objects [i].m_linkType.obj.nObject,
											m_objects [i].m_maxVolume, m_objects [i].m_maxDistance, m_objects [i].m_nLoopStart,
											m_objects [i].m_nLoopEnd);
		}
	}
}

//------------------------------------------------------------------------------

void CAudio::SyncSounds (void)
{
if (!OBJECTS.Buffer ())
	return;

	int32_t			nOldVolume, nNewVolume, nOldPan, 
						nAudioVolume [2] = {audio.Volume (0), audio.Volume (1)};
	CObject*			pObj = NULL;
	CFixVector		vListenerPos = gameData.objData.pViewer->info.position.vPos;
	CFixMatrix		mListenerOrient = gameData.objData.pViewer->info.position.mOrient;
	int16_t			nListenerSeg = gameData.objData.pViewer->info.nSegment;

if (gameData.demoData.nState == ND_STATE_RECORDING) {
	if (!gameStates.sound.bWasRecording)
		RecordSoundObjects ();
	gameStates.sound.bWasRecording = 1;
	}
else
	gameStates.sound.bWasRecording = 0;

soundQueue.Process ();

uint32_t i = m_objects.ToS ();

//Update ();
while (i) {
	CSoundObject& soundObj = m_objects [--i];
	if (soundObj.m_flags & SOF_USED) {
		nOldVolume = FixMulDiv (soundObj.m_volume, soundObj.m_audioVolume, I2X (1));
#if USE_SDL_MIXER
		nOldVolume = (fix) FRound (X2F (2 * nOldVolume) * MIX_MAX_VOLUME);
#endif
#if DBG
		if ((nOldVolume <= 0) && (soundObj.m_channel >= 0))
			BRP;
#endif
		nOldPan = soundObj.m_pan;
		// Check if its done.
		if (!(soundObj.m_flags & SOF_PLAY_FOREVER) && ((soundObj.m_channel < 0) && !ChannelIsPlaying (soundObj.m_channel))) {
			DeleteSoundObject (i);
			continue;		// Go on to next sound...
			}
		if (soundObj.m_flags & SOF_LINK_TO_POS) {
#if DBG
			if (soundObj.m_linkType.pos.nSegment == nDbgSeg)
				BRP;
#endif
			GetVolPan (
				mListenerOrient, vListenerPos, nListenerSeg,
				soundObj.m_linkType.pos.position, soundObj.m_linkType.pos.nSegment, soundObj.m_maxVolume,
				&soundObj.m_volume, &soundObj.m_pan, soundObj.m_maxDistance, soundObj.m_nDecay);
			}
		else if (soundObj.m_flags & SOF_LINK_TO_OBJ) {
			if (gameData.demoData.nState == ND_STATE_PLAYBACK) {
				int32_t nObject = NDFindObject (soundObj.m_linkType.obj.nObjSig);
				pObj = OBJECT (nObject);
				}
			else
				pObj = OBJECT (soundObj.m_linkType.obj.nObject);
			if (!pObj || (pObj->info.nType == OBJ_NONE) || (pObj->info.nSignature != soundObj.m_linkType.obj.nObjSig)) {
				DeleteSoundObject (i);	// The object that this is linked to is dead, so just end this sound if it is looping.
				continue;
				}
			else if ((pObj->info.nType == OBJ_EFFECT) && 
						(((pObj->info.nId == SOUND_ID) && !pObj->rType.soundInfo.bEnabled) || 
						 ((pObj->info.nId == LIGHTNING_ID) && !(SHOW_LIGHTNING (1) && pObj->rType.lightningInfo.bEnabled)))) {
				soundObj.Stop ();
				continue;
				}
			GetVolPan (
				mListenerOrient, vListenerPos, nListenerSeg,
				OBJPOS (pObj)->vPos, OBJSEG (pObj), soundObj.m_maxVolume,
				&soundObj.m_volume, &soundObj.m_pan, soundObj.m_maxDistance, soundObj.m_nDecay);
			}
		if (!soundObj.m_bCustom) {
			nNewVolume = FixMulDiv (soundObj.m_volume, nAudioVolume [soundObj.m_bAmbient], I2X (1));
#if USE_SDL_MIXER
			nNewVolume = (fix) FRound (X2F (2 * nNewVolume) * MIX_MAX_VOLUME);
#endif
			if ((nOldVolume != nNewVolume) || ((nNewVolume <= 0) != (soundObj.m_channel < 0))) {
#if DBG
				if (soundObj.m_linkType.pos.nSegment == nDbgSeg)
					BRP;
#endif
				soundObj.m_audioVolume = nAudioVolume [soundObj.m_bAmbient];
				if (nNewVolume <= 0) {	// sound is too far away or muted, so stop it playing.
					if (soundObj.m_channel > -1) {
						if (!(soundObj.m_flags & SOF_PLAY_FOREVER)) {
							DeleteSoundObject (i);
							continue;
							}
						soundObj.Stop ();
						}
					}
				else {
#if 0 //DBG
					if (*soundObj.m_szSound)
						pSoundObj = pSoundObj;
					if (strstr (soundObj.m_szSound, "dripping-water"))
						pSoundObj = pSoundObj;
#endif
					if (soundObj.m_channel < 0)
						soundObj.Start ();
					else
						SetVolume (soundObj.m_channel, soundObj.m_volume);
					}
				}
			if ((nNewVolume > 0) && (nOldPan != soundObj.m_pan) && (soundObj.m_channel > -1))
				SetPan (soundObj.m_channel, soundObj.m_pan);
			}
		}
	}
Cleanup ();
}

//------------------------------------------------------------------------------

void CAudio::PauseSounds (void)
{
PauseLoopingSound ();

	uint32_t i = m_objects.ToS ();
	CSoundObject*	pSoundObj = m_objects.Buffer () + i;

while (i) {
	i--;
	pSoundObj--;
	if ((pSoundObj->m_flags & SOF_USED) && (pSoundObj->m_channel > -1)) {
#if 1
		if ((m_objects [i].m_flags & SOF_PLAY_FOREVER))
			pSoundObj->Stop ();
		else
#endif
			DeleteSoundObject (i);
		}
	}
StopAllChannels (true);
soundQueue.Pause ();
}

//------------------------------------------------------------------------------

void CAudio::PauseAll (void)
{
midi.Pause ();
StopTriggeredSounds ();
PauseSounds ();
}

//------------------------------------------------------------------------------

void CAudio::ResumeSounds (void)
{
//SetSoundSources ();
PrintLog (1, "syncing sounds\n");
SyncSounds ();	//don't think we really need to do this, but can't hurt
PrintLog (0, "resuming looping sounds\n");
ResumeLoopingSound ();
PrintLog (-1);
}

//------------------------------------------------------------------------------

void CAudio::ResumeAll (void)
{
PrintLog (1, "restarting sounds\n");
PrintLog (0, "resuming midi system\n");
midi.Resume ();
PrintLog (0, "resuming sounds\n");
ResumeSounds ();
PrintLog (0, "starting triggered sounds\n");
StartTriggeredSounds ();
PrintLog (-1);
}

//------------------------------------------------------------------------------
// Called by the code in digi.c when another sound takes this sound CObject's
// slot because the sound was done playing.
void CAudio::EndSoundObject (int32_t i)
{
if (m_objects [i].m_flags & SOF_PLAY_FOREVER)
	m_objects [i].m_channel = -1;
else
	DeleteSoundObject (i);
}

//------------------------------------------------------------------------------

void CAudio::StopObjectSounds (void)
{
	uint32_t i = m_objects.ToS ();

while (i)
	DeleteSoundObject (--i);
}

//------------------------------------------------------------------------------

void CAudio::StopAll (void)
{
StopLoopingSound ();
StopObjectSounds ();
StopCurrentSong ();
StopAllChannels ();
}

//------------------------------------------------------------------------------

#if DBG
int32_t CAudio::VerifyChannelFree (int32_t channel)
{
for (int32_t i = 0; i < MAX_SOUND_OBJECTS; i++) {
	if (m_objects [i].m_flags & SOF_USED) {
		if (m_objects [i].m_channel == channel) {
			Int3 ();	// Get John!
			}
		}
	}
return 0;
}
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CSoundQueue soundQueue;

void CSoundQueue::Init (void)
{
m_data.nHead =
m_data.nTail =
m_data.nSounds = 0;
m_data.nChannel = -1;
m_data.queue.Create (MAX_SOUND_QUEUE);
}

//------------------------------------------------------------------------------

void CSoundQueue::Destroy (void)
{
m_data.queue.Destroy ();
}

//------------------------------------------------------------------------------

void CSoundQueue::Pause (void)
{
soundQueue.m_data.nChannel = -1;
}

//------------------------------------------------------------------------------

void CSoundQueue::End (void)
{
	// Current playing sound is stopped, so take it off the Queue
if (++m_data.nHead >= MAX_SOUND_QUEUE)
	m_data.nHead = 0;
m_data.nSounds--;
m_data.nChannel = -1;
}

//------------------------------------------------------------------------------

void CSoundQueue::Process (void)
{
	fix curtime = TimerGetApproxSeconds ();
	tSoundQueueEntry *q;

if (m_data.nChannel > -1) {
	if (audio.ChannelIsPlaying (m_data.nChannel))
		return;
	End ();
	}
while (m_data.nHead != m_data.nTail) {
	q = m_data.queue + m_data.nHead;
	if (q->timeAdded + MAX_LIFE > curtime) {
		m_data.nChannel = audio.StartSound (q->nSound, SOUNDCLASS_GENERIC, I2X (1) + 1);
		return;
		}
	End ();
	}
}

//------------------------------------------------------------------------------

void CSoundQueue::StartSound (int16_t nSound, fix nVolume)
{
	int32_t					i;
	tSoundQueueEntry *q;

nSound = audio.XlatSound (nSound);
if (nSound < 0)
	return;
i = m_data.nTail + 1;
if (i >= MAX_SOUND_QUEUE)
	i = 0;
// Make sure its loud so it doesn't get cancelled!
if (nVolume < I2X (1) + 1)
	nVolume = I2X (1) + 1;
if (i != m_data.nHead) {
	q = m_data.queue + m_data.nTail;
	q->timeAdded = TimerGetApproxSeconds ();
	q->nSound = nSound;
	m_data.nSounds++;
	m_data.nTail = i;
	}
Process ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void SetD1Sound (void)
{
gameStates.sound.bD1Sound = gameStates.app.bD1Mission && gameOpts->sound.bUseD1Sounds && (gameStates.app.bHaveD1Data || gameOpts->UseHiresSound ());
}

//------------------------------------------------------------------------------

static int32_t SideIsSoundSource (int16_t nSegment, int16_t nSide)
{
CSegment* pSeg = SEGMENT (nSegment);
if (!(pSeg->IsPassable (nSide, NULL) & WID_VISIBLE_FLAG))
	return -1;
int16_t nOvlTex = pSeg->m_sides [nSide].m_nOvlTex;
int16_t nEffect = nOvlTex ? gameData.pigData.tex.pTexMapInfo [nOvlTex].nEffectClip : -1;
if (nEffect < 0)
	nEffect = gameData.pigData.tex.pTexMapInfo [pSeg->m_sides [nSide].m_nBaseTex].nEffectClip;
if (nEffect < 0)
	return -1;
int32_t nSound = gameData.effectData.pEffect [nEffect].nSound;
if (nSound == -1)
	return -1;
int16_t nConnSeg = pSeg->m_children [nSide];

//check for sound on other CSide of CWall.  Don't add on
//both walls if sound travels through CWall.  If sound
//does travel through CWall, add sound for lower-numbered
//CSegment.

if (IS_CHILD (nConnSeg) && (nConnSeg < nSegment) &&
	 (pSeg->IsPassable (nSide, NULL) & (WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG))) {
	CSegment* pConnSeg = SEGMENT (pSeg->m_children [nSide]);
	int16_t nConnSide = pSeg->ConnectedSide (pConnSeg);
	if (pConnSeg->m_sides [nConnSide].m_nOvlTex == pSeg->m_sides [nSide].m_nOvlTex)
		return -1;		//skip this one
	}
return nSound;
}

//------------------------------------------------------------------------------

//go through this level and start any effect sounds
void SetSoundSources (void)
{
	int16_t		nSegment, nSide;
	CSegment*	pSeg;
	CObject*		pObj;
	int32_t		nSegSoundSources, nSideSounds [6];

SetD1Sound ();
audio.InitSounds ();		//clear old sounds
gameStates.sound.bDontStartObjects = 1;
for (pSeg = SEGMENTS.Buffer (), nSegment = 0; nSegment <= gameData.segData.nLastSegment; pSeg++, nSegment++) {
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
		BRP;
#endif
	for (nSide = 0, nSegSoundSources = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
#if DBG
		if (nDbgSeg >= 0) {
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				;
			else {
				nSideSounds [nSide] = -1;
				continue;
				}
			}
#endif
		if (0 <= (nSideSounds [nSide] = SideIsSoundSource (nSegment, nSide)))
			nSegSoundSources++;
		}
	for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) 
		if (nSideSounds [nSide] >= 0)
			audio.CreateSegmentSound (nSideSounds [nSide], nSegment, nSide, pSeg->SideCenter (nSide), 1, I2X (1) / (2 * nSegSoundSources));
	}

FORALL_EFFECT_OBJS (pObj)
	if (pObj->info.nId == SOUND_ID) {
		char fn [FILENAME_LEN];
#if 0 //DBG
		if (strcmp (pObj->rType.soundInfo.szFilename, "steam"))
			continue;
#endif
		sprintf (fn, "%s.wav", pObj->rType.soundInfo.szFilename);
		audio.CreateObjectSound (-1, SOUNDCLASS_AMBIENT, pObj->Index (), 1, pObj->rType.soundInfo.nVolume, I2X (256), 0, 0, fn);
		}

int16_t nSound = audio.GetSoundByName ("explode2");
if (0 <= nSound) {
	FORALL_STATIC_OBJS (pObj)
		if (pObj->info.nType == OBJ_EXPLOSION) {
			pObj->info.renderType = RT_POWERUP;
			pObj->rType.animationInfo.nClipIndex = pObj->info.nId;
			audio.CreateObjectSound (nSound, SOUNDCLASS_AMBIENT, pObj->Index (), 1);
			}
	}
//gameStates.sound.bD1Sound = 0;
gameStates.sound.bDontStartObjects = 0;
}

//------------------------------------------------------------------------------
//eof
