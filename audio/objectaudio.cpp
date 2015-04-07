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
							(m_flags & SOF_LINK_TO_OBJ) ? &gameData.Object (m_linkType.obj.nObject)->info.position.vPos : &m_linkType.pos.position);
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
	for (int32_t i = 0; i < gameData.segs.nSegments; i++) {
		fix pathDistance = m_router [nThread].Distance (i);
		if (pathDistance <= 0)
			continue;
		int16_t l = m_router [nThread].RouteLength (nSoundSeg);
		if (l < 3)
			continue;
		CSegment* segP = gameData.Segment (nListenerSeg);
		int16_t nChild = m_router [nThread].Route (1)->nNode;
		fix corrStart = CFixVector::Dist (vListenerPos, gameData.Segment (nChild)->Center ()) - segP->m_childDists [0][segP->ChildIndex (nChild)];
		segP = gameData.Segment (nSoundSeg);
		nChild = m_router [nThread].Route (l - 2)->nNode;
		fix corrEnd = CFixVector::Dist (vSoundPos, gameData.Segment (nChild)->Center ()) - segP->m_childDists [0][segP->ChildIndex (nChild)];
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
if (gameData.segs.SegVis (nListenerSeg, nSoundSeg))
	distance += fix (distance * 0.6f * float (distance) / float (maxDistance));
else {
	int16_t l = m_router [nThread].RouteLength (nSoundSeg);
	if (l > 2) {
		CSegment* segP = gameData.Segment (nListenerSeg);
		int16_t nChild = m_router [nThread].Route (1)->nNode;
		pathDistance -= segP->m_childDists [0][segP->ChildIndex (nChild)];
		pathDistance += CFixVector::Dist (vListenerPos, gameData.Segment (nChild)->Center ());
		segP = gameData.Segment (nSoundSeg);
		nChild = m_router [nThread].Route (l - 2)->nNode;
		pathDistance -= segP->m_childDists [0][segP->ChildIndex (nChild)];
		pathDistance += CFixVector::Dist (vSoundPos, gameData.Segment (nChild)->Center ());
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
	if (gameData.demo.nState == ND_STATE_RECORDING) {
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
	m_info.nLoopingChannel  =
		StartSound (m_info.nLoopingSound, SOUNDCLASS_GENERIC, m_info.nLoopingVolume, DEFAULT_PAN, 1,
						m_info.nLoopingStart, m_info.nLoopingEnd);
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
	CObject*			objP;
	CSoundObject*	soundObjP;
	int16_t			nSound = 0;

if (maxVolume < 0)
	return -1;
if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0]))
	return -1;
if (!(pszSound && *pszSound)) {
	nSound = XlatSound (nOrgSound);
	if (nSound < 0)
		return -1;
	if (!gameData.pig.sound.sounds [gameStates.sound.bD1Sound][nSound].data) {
		Int3 ();
		return -1;
		}
	}
objP = gameData.Object (nObject);
if (!bForever) { 	// Hack to keep sounds from building up...
	int32_t nVolume, nPan;
	GetVolPan (gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos,
				  gameData.objs.viewerP->info.nSegment, objP->info.position.vPos, objP->info.nSegment, maxVolume, &nVolume, &nPan,
				  maxDistance, nDecay, nThread);
	PlaySound (nOrgSound, nSoundClass, nVolume, nPan, 0, -1, pszSound, &objP->info.position.vPos);
	return -1;
	}
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordCreateObjectSound (nOrgSound, nObject, maxVolume, maxDistance, nLoopStart, nLoopEnd);
if (!m_objects.Grow ())
	return -1;
soundObjP = m_objects.Top ();
soundObjP->m_nSignature = m_info.nNextSignature++;
soundObjP->m_flags = SOF_USED | SOF_LINK_TO_OBJ;
if (bForever)
	soundObjP->m_flags |= SOF_PLAY_FOREVER;
soundObjP->m_linkType.obj.nObject = nObject;
soundObjP->m_linkType.obj.nObjSig = objP->info.nSignature;
soundObjP->m_maxVolume = maxVolume;
soundObjP->m_maxDistance = maxDistance;
soundObjP->m_soundClass = nSoundClass;
soundObjP->m_bAmbient = (nSoundClass == SOUNDCLASS_AMBIENT);
soundObjP->m_volume = 0;
soundObjP->m_pan = 0;
soundObjP->m_nSound = (pszSound && *pszSound) ? -1 : nSound;
soundObjP->m_nDecay = nSoundClass;
if (pszSound)
	strncpy (soundObjP->m_szSound, pszSound, sizeof (soundObjP->m_szSound));
else
	*(soundObjP->m_szSound) = '\0';
soundObjP->m_nLoopStart = nLoopStart;
soundObjP->m_nLoopEnd = nLoopEnd;
if (gameStates.sound.bDontStartObjects) { 		//started at level start
	soundObjP->m_flags |= SOF_PERMANENT;
	soundObjP->m_channel = -1;
	}
else {
	GetVolPan (
		gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos,
		gameData.objs.viewerP->info.nSegment, objP->info.position.vPos, objP->info.nSegment, soundObjP->m_maxVolume,
      &soundObjP->m_volume, &soundObjP->m_pan, soundObjP->m_maxDistance, soundObjP->m_nDecay, nThread);
	soundObjP->Start ();
	// If it's a one-shot sound effect, and it can't start right away, then
	// just cancel it and be done with it.
	if ((soundObjP->m_channel < 0) && (!(soundObjP->m_flags & SOF_PLAY_FOREVER))) {
		m_objects.Pop ();
		return -1;
		}
	}
soundObjP->m_bCustom = bCustom;
#if DBG
if (nOrgSound == SOUND_AFTERBURNER_IGNITE)
	nDbgChannel = soundObjP->m_channel;
#endif
return soundObjP->m_nSignature;
}

//------------------------------------------------------------------------------

int32_t CAudio::ChangeObjectSound (int32_t nObject, fix nVolume)
{

	CSoundObject*	soundObjP = m_objects.Buffer ();

if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordDestroySoundObject (nObject);

for (uint32_t i = 0; i < m_objects.ToS (); i++, soundObjP++) {
	if ((soundObjP->m_flags & (SOF_USED | SOF_LINK_TO_OBJ)) == (SOF_USED | SOF_LINK_TO_OBJ)) {
		if ((soundObjP->m_linkType.obj.nObject == nObject) && (soundObjP->m_channel > -1)) {
			SetVolume (soundObjP->m_channel, soundObjP->m_volume = nVolume);
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
	CAudioChannel* channelP;

h = int32_t (m_objects.ToS ());

for (i = int32_t (m_usedChannels.ToS ()); i; ) {
	channelP = m_channels + m_usedChannels [--i];
	if (channelP->SoundObject () >= h)
		channelP->Stop ();
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

	CSoundObject*	soundObjP = m_objects + i;

soundObjP->Stop ();
soundObjP->m_linkType.obj.nObject = -1;
soundObjP->m_linkType.obj.nObjSig = -1;
soundObjP->m_flags = 0;	// Mark as dead, so some other sound can use this sound
m_objects.Delete (i);
}

//------------------------------------------------------------------------------

int32_t CAudio::FindObjectSound (int32_t nObject, int16_t nSound)
{
	CSoundObject*	soundObjP = m_objects.Buffer ();

if (nSound >= 0)
	nSound = XlatSound (nSound);
for (uint32_t i = m_objects.ToS (); i; i--, soundObjP++) 
	if ((soundObjP->m_linkType.obj.nObject == nObject) && (soundObjP->m_nSound == nSound))
		return int32_t (m_objects.ToS () - i);
return -1;
}

//------------------------------------------------------------------------------

int32_t CAudio::DestroyObjectSound (int32_t nObject, int16_t nSound)
{
	int32_t nKilled = 0;

if ((nObject >= 0) && (gameData.demo.nState == ND_STATE_RECORDING))
	NDRecordDestroySoundObject (nObject);

if (nObject == LOCALPLAYER.nObject)
	gameData.multiplayer.bMoving = -1;

if (nSound >= 0)
	nSound = XlatSound (nSound);

#pragma omp critical
{
	uint32_t i = m_objects.ToS ();
	CSoundObject*	soundObjP = m_objects.Buffer () + i;

while (i) {
	i--;
	soundObjP--;
	if ((soundObjP->m_flags & (SOF_USED | SOF_LINK_TO_OBJ)) != (SOF_USED | SOF_LINK_TO_OBJ))
		continue;
	if (nObject < 0) { // kill all sounds belonging to disappeared objects
		if ((soundObjP->m_linkType.obj.nObject >= 0) && (gameData.Object (soundObjP->m_linkType.obj.nObject)->Signature () == soundObjP->m_linkType.obj.nObjSig))
			continue;
		}
	else {
		if (soundObjP->m_linkType.obj.nObject != nObject)
			continue;
		}
	if ((nObject < 0) && (soundObjP->m_flags & SOF_PLAY_FOREVER))
		soundObjP->Stop ();
	else if ((nSound < 0) || (soundObjP->m_nSound == nSound)) {
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
	CSoundObject*	soundObjP;

nSound = XlatSound (nOrgSound);
if (maxVolume < 0)
	return -1;
//	if (maxVolume > I2X (1)) maxVolume = I2X (1);
if (nSound < 0)
	return -1;
if (!gameData.pig.sound.sounds [gameStates.sound.bD1Sound][nSound].data) {
	Int3 ();
	return -1;
	}
if ((nSegment < 0)|| (nSegment > gameData.segs.nLastSegment))
	return -1;
if (!bForever) { 	//&& gameData.pig.sound.sounds [nSound - SOUND_OFFSET].length < SOUND_3D_THRESHHOLD) {
	// Hack to keep sounds from building up...
	int32_t nVolume, nPan;
	GetVolPan (gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos, gameData.objs.viewerP->info.nSegment,
				  vPos, nSegment, maxVolume, &nVolume, &nPan, maxDistance, 0);
	PlaySound (nOrgSound, SOUNDCLASS_GENERIC, nVolume, nPan, 0, -1, pszSound, &vPos);
	return -1;
	}
if (m_objects.ToS () == MAX_SOUND_OBJECTS)
	return -1;
if (!m_objects.Grow ())
	return -1;
soundObjP = m_objects.Top ();
soundObjP->m_nSignature = m_info.nNextSignature++;
soundObjP->m_flags = SOF_USED | SOF_LINK_TO_POS;
if (bForever)
	soundObjP->m_flags |= SOF_PLAY_FOREVER;
soundObjP->m_linkType.pos.nSegment = nSegment;
soundObjP->m_linkType.pos.nSide = nSide;
soundObjP->m_linkType.pos.position = vPos;
soundObjP->m_nSound = nSound;
soundObjP->m_soundClass = SOUNDCLASS_GENERIC;
soundObjP->m_maxVolume = maxVolume;
soundObjP->m_maxDistance = maxDistance;
if (pszSound)
	strncpy (soundObjP->m_szSound, pszSound, sizeof (soundObjP->m_szSound));
else
	*soundObjP->m_szSound = '\0';
soundObjP->m_volume = 0;
soundObjP->m_pan = 0;
soundObjP->m_nDecay = 0;
soundObjP->m_nLoopStart = soundObjP->m_nLoopEnd = -1;
if (gameStates.sound.bDontStartObjects) {		//started at level start
	soundObjP->m_flags |= SOF_PERMANENT;
	soundObjP->m_channel = -1;
	}
else {
	GetVolPan (
		gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos,
		gameData.objs.viewerP->info.nSegment, soundObjP->m_linkType.pos.position,
		soundObjP->m_linkType.pos.nSegment, soundObjP->m_maxVolume, &soundObjP->m_volume, &soundObjP->m_pan, soundObjP->m_maxDistance, soundObjP->m_nDecay);
	soundObjP->Start ();
	// If it's a one-shot sound effect, and it can't start right away, then
	// just cancel it and be done with it.
	if ((soundObjP->m_channel < 0) && (!(soundObjP->m_flags & SOF_PLAY_FOREVER))) {
		m_objects.Pop ();
		return -1;
		}
	}
return soundObjP->m_nSignature;
}

//------------------------------------------------------------------------------
//if nSound==-1, kill any sound
int32_t CAudio::DestroySegmentSound (int16_t nSegment, int16_t nSide, int16_t nSound)
{
if (nSound != -1)
	nSound = XlatSound (nSound);

	int32_t nKilled = 0;
	uint32_t i = m_objects.ToS ();
	CSoundObject*	soundObjP = m_objects.Buffer () + i;

while (i) {
	i--;
	soundObjP--;
	if ((soundObjP->m_flags & (SOF_USED | SOF_LINK_TO_POS)) == (SOF_USED | SOF_LINK_TO_POS)) {
		if ((soundObjP->m_linkType.pos.nSegment == nSegment) && (soundObjP->m_linkType.pos.nSide == nSide) &&
			 ((nSound == -1) || (soundObjP->m_nSound == nSound))) {
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

	int32_t				nOldVolume, nNewVolume, nOldPan, 
						nAudioVolume [2] = {audio.Volume (0), audio.Volume (1)};
	uint32_t				i;
	CObject*			objP = NULL;
	CFixVector		vListenerPos = gameData.objs.viewerP->info.position.vPos;
	CFixMatrix		mListenerOrient = gameData.objs.viewerP->info.position.mOrient;
	int16_t				nListenerSeg = gameData.objs.viewerP->info.nSegment;

if (gameData.demo.nState == ND_STATE_RECORDING) {
	if (!gameStates.sound.bWasRecording)
		RecordSoundObjects ();
	gameStates.sound.bWasRecording = 1;
	}
else
	gameStates.sound.bWasRecording = 0;

soundQueue.Process ();

	i = m_objects.ToS ();
	CSoundObject*	soundObjP = m_objects.Buffer () + i;

//Update ();
while (i) {
	i--;
	soundObjP--;
	if (soundObjP->m_flags & SOF_USED) {
		nOldVolume = FixMulDiv (soundObjP->m_volume, soundObjP->m_audioVolume, I2X (1));
#if USE_SDL_MIXER
		nOldVolume = (fix) FRound (X2F (2 * nOldVolume) * MIX_MAX_VOLUME);
#endif
#if DBG
		if ((nOldVolume <= 0) && (soundObjP->m_channel >= 0))
			BRP;
#endif
		nOldPan = soundObjP->m_pan;
		// Check if its done.
		if (!(soundObjP->m_flags & SOF_PLAY_FOREVER) && ((soundObjP->m_channel < 0) && !ChannelIsPlaying (soundObjP->m_channel))) {
			DeleteSoundObject (i);
			continue;		// Go on to next sound...
			}
		if (soundObjP->m_flags & SOF_LINK_TO_POS) {
#if DBG
			if (soundObjP->m_linkType.pos.nSegment == nDbgSeg)
				BRP;
#endif
			GetVolPan (
				mListenerOrient, vListenerPos, nListenerSeg,
				soundObjP->m_linkType.pos.position, soundObjP->m_linkType.pos.nSegment, soundObjP->m_maxVolume,
				&soundObjP->m_volume, &soundObjP->m_pan, soundObjP->m_maxDistance, soundObjP->m_nDecay);
			}
		else if (soundObjP->m_flags & SOF_LINK_TO_OBJ) {
			if (gameData.demo.nState == ND_STATE_PLAYBACK) {
				int32_t nObject = NDFindObject (soundObjP->m_linkType.obj.nObjSig);
				objP = gameData.Object (((nObject > -1) ? nObject : 0));
				}
			else
				objP = gameData.Object (soundObjP->m_linkType.obj.nObject);
			if (!objP || (objP->info.nType == OBJ_NONE) || (objP->info.nSignature != soundObjP->m_linkType.obj.nObjSig)) {
				DeleteSoundObject (i);	// The object that this is linked to is dead, so just end this sound if it is looping.
				continue;
				}
			else if ((objP->info.nType == OBJ_EFFECT) && 
						(((objP->info.nId == SOUND_ID) && !objP->rType.soundInfo.bEnabled) || 
						 ((objP->info.nId == LIGHTNING_ID) && !(SHOW_LIGHTNING (1) && objP->rType.lightningInfo.bEnabled)))) {
				soundObjP->Stop ();
				continue;
				}
			GetVolPan (
				mListenerOrient, vListenerPos, nListenerSeg,
				OBJPOS (objP)->vPos, OBJSEG (objP), soundObjP->m_maxVolume,
				&soundObjP->m_volume, &soundObjP->m_pan, soundObjP->m_maxDistance, soundObjP->m_nDecay);
			}
		if (!soundObjP->m_bCustom) {
			nNewVolume = FixMulDiv (soundObjP->m_volume, nAudioVolume [soundObjP->m_bAmbient], I2X (1));
#if USE_SDL_MIXER
			nNewVolume = (fix) FRound (X2F (2 * nNewVolume) * MIX_MAX_VOLUME);
#endif
			if ((nOldVolume != nNewVolume) || ((nNewVolume <= 0) != (soundObjP->m_channel < 0))) {
#if DBG
				if (soundObjP->m_linkType.pos.nSegment == nDbgSeg)
					BRP;
#endif
				soundObjP->m_audioVolume = nAudioVolume [soundObjP->m_bAmbient];
				if (nNewVolume <= 0) {	// sound is too far away or muted, so stop it playing.
					if (soundObjP->m_channel > -1) {
						if (!(soundObjP->m_flags & SOF_PLAY_FOREVER)) {
							DeleteSoundObject (i);
							continue;
							}
						soundObjP->Stop ();
						}
					}
				else {
#if 0 //DBG
					if (*soundObjP->m_szSound)
						soundObjP = soundObjP;
					if (strstr (soundObjP->m_szSound, "dripping-water"))
						soundObjP = soundObjP;
#endif
					if (soundObjP->m_channel < 0)
						soundObjP->Start ();
					else
						SetVolume (soundObjP->m_channel, soundObjP->m_volume);
					}
				}
			if ((nNewVolume > 0) && (nOldPan != soundObjP->m_pan) && (soundObjP->m_channel > -1))
				SetPan (soundObjP->m_channel, soundObjP->m_pan);
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
	CSoundObject*	soundObjP = m_objects.Buffer () + i;

while (i) {
	i--;
	soundObjP--;
	if ((soundObjP->m_flags & SOF_USED) && (soundObjP->m_channel > -1)) {
#if 1
		if ((m_objects [i].m_flags & SOF_PLAY_FOREVER))
			soundObjP->Stop ();
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
SyncSounds ();	//don't think we really need to do this, but can't hurt
ResumeLoopingSound ();
}

//------------------------------------------------------------------------------

void CAudio::ResumeAll (void)
{
PrintLog (1, "restarting sounds\n");
midi.Resume ();
ResumeSounds ();
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
CSegment* segP = gameData.Segment (nSegment);
if (!(segP->IsPassable (nSide, NULL) & WID_VISIBLE_FLAG))
	return -1;
int16_t nOvlTex = segP->m_sides [nSide].m_nOvlTex;
int16_t nEffect = nOvlTex ? gameData.pig.tex.tMapInfoP [nOvlTex].nEffectClip : -1;
if (nEffect < 0)
	nEffect = gameData.pig.tex.tMapInfoP [segP->m_sides [nSide].m_nBaseTex].nEffectClip;
if (nEffect < 0)
	return -1;
int32_t nSound = gameData.effects.effectP [nEffect].nSound;
if (nSound == -1)
	return -1;
int16_t nConnSeg = segP->m_children [nSide];

//check for sound on other CSide of CWall.  Don't add on
//both walls if sound travels through CWall.  If sound
//does travel through CWall, add sound for lower-numbered
//CSegment.

if (IS_CHILD (nConnSeg) && (nConnSeg < nSegment) &&
	 (segP->IsPassable (nSide, NULL) & (WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG))) {
	CSegment* connSegP = gameData.Segment (segP->m_children [nSide]);
	int16_t nConnSide = segP->ConnectedSide (connSegP);
	if (connSegP->m_sides [nConnSide].m_nOvlTex == segP->m_sides [nSide].m_nOvlTex)
		return -1;		//skip this one
	}
return nSound;
}

//------------------------------------------------------------------------------

//go through this level and start any effect sounds
void SetSoundSources (void)
{
	int16_t		nSegment, nSide;
	CSegment*	segP;
	CObject*		objP;
	int32_t		nSegSoundSources, nSideSounds [6];

SetD1Sound ();
audio.InitSounds ();		//clear old sounds
gameStates.sound.bDontStartObjects = 1;
for (segP = SEGMENTS.Buffer (), nSegment = 0; nSegment <= gameData.segs.nLastSegment; segP++, nSegment++) {
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
			audio.CreateSegmentSound (nSideSounds [nSide], nSegment, nSide, segP->SideCenter (nSide), 1, I2X (1) / (2 * nSegSoundSources));
	}

FORALL_EFFECT_OBJS (objP)
	if (objP->info.nId == SOUND_ID) {
		char fn [FILENAME_LEN];
#if 0 //DBG
		if (strcmp (objP->rType.soundInfo.szFilename, "steam"))
			continue;
#endif
		sprintf (fn, "%s.wav", objP->rType.soundInfo.szFilename);
		audio.CreateObjectSound (-1, SOUNDCLASS_AMBIENT, objP->Index (), 1, objP->rType.soundInfo.nVolume, I2X (256), 0, 0, fn);
		}

int16_t nSound = audio.GetSoundByName ("explode2");
if (0 <= nSound) {
	FORALL_STATIC_OBJS (objP)
		if (objP->info.nType == OBJ_EXPLOSION) {
			objP->info.renderType = RT_POWERUP;
			objP->rType.animationInfo.nClipIndex = objP->info.nId;
			audio.CreateObjectSound (nSound, SOUNDCLASS_AMBIENT, objP->Index (), 1);
			}
	}
//gameStates.sound.bD1Sound = 0;
gameStates.sound.bDontStartObjects = 0;
}

//------------------------------------------------------------------------------
//eof
