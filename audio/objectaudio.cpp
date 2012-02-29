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
if ((m_flags & SOF_PERMANENT) && (audio.ActiveObjects () >= max (1, 33 * audio.GetMaxChannels () / 100)) && !audio.SuspendObjectSound (m_volume))
	return false;
// start the sample playing
m_channel =
	audio.StartSound (m_nSound, m_soundClass, m_volume, m_pan, (m_flags & SOF_PLAY_FOREVER) != 0, m_nLoopStart, m_nLoopEnd,
							int (this - audio.Objects ().Buffer ()), I2X (1), m_szSound,
							(m_flags & SOF_LINK_TO_OBJ) ? &OBJECTS [m_linkType.obj.nObject].info.position.vPos : &m_linkType.pos.position);
if (m_channel < 0)
	return false;
audio.ActivateObject ();
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/* Find the sound which actually equates to a sound number */
short CAudio::XlatSound (short nSound)
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

int CAudio::UnXlatSound (int nSound)
{
	int i;
	ubyte *table = (m_info.bLoMem ? AltSounds [gameStates.sound.bD1Sound] : Sounds [gameStates.sound.bD1Sound]);

if (nSound < 0)
	return -1;
for (i = 0; i < MAX_SOUNDS; i++)
	if (table [i] == nSound)
		return i;
Int3 ();
return 0;
}

//------------------------------------------------------------------------------

int CAudio::GetSoundByName (const char* pszSound)
{
	char	szSound [FILENAME_LEN];
	int	nSound;

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

int CAudio::Distance (CFixVector& vListenerPos, short nListenerSeg, CFixVector& vSoundPos, short nSoundSeg, fix maxDistance, int nDecay, CFixVector& vecToSound)
{
	static float fCorrFactor = 1.0f;
	static uint  nRouteCount = 1;

if (nDecay)
	maxDistance *= 2;
else
	maxDistance = (5 * maxDistance) / 4;	// Make all sounds travel 1.25 times as far.

fix distance = CFixVector::NormalizedDir (vecToSound, vSoundPos, vListenerPos);
if (distance > maxDistance) 
	return -1;
if (nListenerSeg == nSoundSeg)
	return distance;
if (!HaveRouter ()) {
	int nSearchSegs = X2I (maxDistance / 10);
	if (nSearchSegs < 3)
		nSearchSegs = 3;
	return uniDacsRouter [0].PathLength (vListenerPos, nListenerSeg, vSoundPos, nSoundSeg, nSearchSegs, WID_TRANSPARENT_FLAG | WID_PASSABLE_FLAG, 0);
	}

if (m_nListenerSeg != nListenerSeg) 
	m_nListenerSeg = nListenerSeg;
if ((m_nListenerSeg != m_router.StartSeg ()) || (m_router.DestSeg () > -1)) { // either we had a different start last time, or the last calculation was a 1:1 routing
	m_router.PathLength (CFixVector::ZERO, nListenerSeg, CFixVector::ZERO, -1, /*I2X (5 * 256 / 4)*/maxDistance, WID_TRANSPARENT_FLAG | WID_PASSABLE_FLAG, -1);
#if 0 //DBG
	for (int i = 0; i < gameData.segs.nSegments; i++) {
		fix pathDistance = m_router.Distance (i);
		if (pathDistance <= 0)
			continue;
		short l = m_router.RouteLength (nSoundSeg);
		if (l < 3)
			continue;
		CSegment* segP = &SEGMENTS [nListenerSeg];
		short nChild = m_router.Route (1)->nNode;
		fix corrStart = CFixVector::Dist (vListenerPos, SEGMENTS [nChild].Center ()) - segP->m_childDists [0][segP->ChildIndex (nChild)];
		segP = &SEGMENTS [nSoundSeg];
		nChild = m_router.Route (l - 2)->nNode;
		fix corrEnd = CFixVector::Dist (vSoundPos, SEGMENTS [nChild].Center ()) - segP->m_childDists [0][segP->ChildIndex (nChild)];
		if (corrStart + corrEnd < pathDistance)
		pathDistance += corrStart + corrEnd;
		if (pathDistance <= 0)
			continue;
		fCorrFactor += float (pathDistance) / float (distance);
		++nRouteCount;
		}
#endif
	}

fix pathDistance = m_router.Distance (nSoundSeg);
if (pathDistance < 0)
	return -1;
#if 1
if (gameData.segs.SegVis (nListenerSeg, nSoundSeg))
	return distance + fix (distance * 0.6f * float (distance) / float (maxDistance));
//	return fix (distance); // * fCorrFactor / float (nRouteCount));
#endif
#if 0 //DBG
short l = m_router.RouteLength (nSoundSeg);
if (l > 2) {
	CSegment* segP = &SEGMENTS [nListenerSeg];
	short nChild = m_router.Route (1)->nNode;
	pathDistance -= segP->m_childDists [0][segP->ChildIndex (nChild)];
	pathDistance += CFixVector::Dist (vListenerPos, SEGMENTS [nChild].Center ());
	segP = &SEGMENTS [nSoundSeg];
	nChild = m_router.Route (l - 2)->nNode;
	pathDistance -= segP->m_childDists [0][segP->ChildIndex (nChild)];
	pathDistance += CFixVector::Dist (vSoundPos, SEGMENTS [nChild].Center ());
#if DBG
	if (pathDistance < 0)
		pathDistance = 0;
#endif
	//fCorrFactor += float (pathDistance) / float (distance);
	//++nRouteCount;
	}
distance += fix (distance * (1.0f + (fCorrFactor - 1.0f) * (float (distance) / float (maxDistance))) / float (nRouteCount));
#else
distance += fix (distance * 0.6f * float (distance) / float (maxDistance));
#endif
return (distance < maxDistance) ? distance : -1;
}

//------------------------------------------------------------------------------
// determine nVolume and panning of a sound created at location nSoundSeg,vSoundPos
// as heard by nListenerSeg,vListenerPos

void CAudio::GetVolPan (CFixMatrix& mListener, CFixVector& vListenerPos, short nListenerSeg,
								CFixVector& vSoundPos, short nSoundSeg,
								fix maxVolume, int *nVolume, int *pan, fix maxDistance, int nDecay)
{

*nVolume = 0;
*pan = 0;

CFixVector vecToSound;
fix distance = Distance (vListenerPos, nListenerSeg, vSoundPos, nSoundSeg, maxDistance, nDecay, vecToSound);
if (distance < 0)
	return;

if (!nDecay)
	//*nVolume = FixMulDiv (maxVolume, maxDistance - distance, maxDistance);
	*nVolume = fix (float (maxVolume) * (1.0f - float (distance) / float (maxDistance)));
else if (nDecay == 1) {
	float fDecay = (float) exp (-log (2.0f) * 4.0f * X2F (distance) / X2F (maxDistance / 2));
	*nVolume = (int) (maxVolume * fDecay);
	}
else {
	float fDecay = 1.0f - X2F (distance) / X2F (maxDistance);
	*nVolume = (int) (maxVolume * fDecay * fDecay * fDecay);
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

int CAudio::PlaySound (short nSound, int nSoundClass, fix nVolume, int nPan, int bNoDups, int nLoops, const char* pszWAV, CFixVector* vPos)
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
	int nChannel = FindChannel (nSound);
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
for (uint i = 0; i < m_objects.ToS (); i++) {
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

void CAudio::PlayLoopingSound (short nSound, fix nVolume, int nLoopStart, int nLoopEnd)
{
nSound = CAudio::XlatSound (nSound);
if (nSound < 0)
	return;
if (m_info.nLoopingChannel > -1)
	StopSound (m_info.nLoopingChannel);
m_info.nLoopingSound = (short) nSound;
m_info.nLoopingVolume = (short) nVolume;
m_info.nLoopingStart = (short) nLoopStart;
m_info.nLoopingEnd = (short) nLoopEnd;
StartLoopingSound ();
}

//------------------------------------------------------------------------------

void CAudio::ChangeLoopingVolume (fix nVolume)
{
if (m_info.nLoopingChannel > -1)
	SetVolume (m_info.nLoopingChannel, nVolume);
m_info.nLoopingVolume = (short) nVolume;
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

bool CAudio::SuspendObjectSound (int nThreshold)
{
	int	j = -1, nMinVolume = I2X (1000);

for (int i = 0; i < int (m_objects.ToS ()); i++) {
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

int CAudio::CreateObjectSound (
	short nOrgSound, int nSoundClass, short nObject, int bForever, fix maxVolume, fix maxDistance,
	int nLoopStart, int nLoopEnd, const char* pszSound, int nDecay, ubyte bCustom)
{
	CObject*			objP;
	CSoundObject*	soundObjP;
	short				nSound = 0;

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
objP = OBJECTS + nObject;
if (!bForever) { 	// Hack to keep sounds from building up...
	int nVolume, nPan;
	GetVolPan (gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos,
				  gameData.objs.viewerP->info.nSegment, objP->info.position.vPos, objP->info.nSegment, maxVolume, &nVolume, &nPan,
				  maxDistance, nDecay);
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
      &soundObjP->m_volume, &soundObjP->m_pan, soundObjP->m_maxDistance, soundObjP->m_nDecay);
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

int CAudio::ChangeObjectSound (int nObject, fix nVolume)
{

	CSoundObject*	soundObjP = m_objects.Buffer ();

if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordDestroySoundObject (nObject);

for (uint i = 0; i < m_objects.ToS (); i++, soundObjP++) {
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
	int				h, i;
	CAudioChannel* channelP;
	CSoundObject*	soundObjP;

h = int (m_objects.ToS ());

for (i = int (m_usedChannels.ToS ()); i; ) {
	channelP = m_channels + m_usedChannels [--i];
	if (channelP->SoundObject () >= h)
		channelP->Stop ();
	}

soundObjP = m_objects.Buffer () + h;
while (h) {
	i = m_objects [--h].m_channel;
	if ((i >= 0) && (m_channels [i].SoundObject () != h))
		DeleteSoundObject (h);
	}
}

//------------------------------------------------------------------------------

void CAudio::DeleteSoundObject (int i)
{
	int	h;

#if DBG
	CSoundObject	o = m_objects [i];
#endif

if ((i < int (m_objects.ToS ()) - 1) && ((h = m_objects.Top ()->m_channel) >= 0)) {
#if DBG
	if (m_channels [h].SoundObject () != int (m_objects.ToS ()) - 1)
		i = i;
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

int CAudio::DestroyObjectSound (int nObject)
{

	int				nKilled = 0;

if ((nObject >= 0) && (gameData.demo.nState == ND_STATE_RECORDING))
	NDRecordDestroySoundObject (nObject);

if (nObject == LOCALPLAYER.nObject)
	gameData.multiplayer.bMoving = -1;

#pragma omp critical
{
	uint i = m_objects.ToS ();
	CSoundObject*	soundObjP = m_objects.Buffer () + i;

while (i) {
	i--;
	soundObjP--;
	if ((soundObjP->m_flags & (SOF_USED | SOF_LINK_TO_OBJ)) != (SOF_USED | SOF_LINK_TO_OBJ))
		continue;
	if (nObject < 0) { // kill all sounds belonging to disappeared objects
		if ((soundObjP->m_linkType.obj.nObject >= 0) &&
			 (OBJECTS [soundObjP->m_linkType.obj.nObject].Signature () == soundObjP->m_linkType.obj.nObjSig))
			continue;
		}
	else {
		if (soundObjP->m_linkType.obj.nObject != nObject)
			continue;
		}
	if ((nObject < 0) && (soundObjP->m_flags & SOF_PLAY_FOREVER))
		soundObjP->Stop ();
	else {
		DeleteSoundObject (i);
		nKilled++;
		}
	}
}
return (nKilled > 0);
}

//------------------------------------------------------------------------------

int CAudio::CreateSegmentSound (
	short nOrgSound, short nSegment, short nSide, CFixVector& vPos, int bForever,
	fix maxVolume, fix maxDistance, const char* pszSound)
{

	int				nSound;
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
	int nVolume, nPan;
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
int CAudio::DestroySegmentSound (short nSegment, short nSide, short nSound)
{
if (nSound != -1)
	nSound = XlatSound (nSound);

	int nKilled = 0;
	uint i = m_objects.ToS ();
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
for (uint i = 0; i < m_objects.ToS (); i++) {
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

	int				nOldVolume, nNewVolume, nOldPan, 
						nAudioVolume [2] = {audio.Volume (0), audio.Volume (1)};
	uint				i;
	CObject*			objP = NULL;
	CFixVector		vListenerPos = gameData.objs.viewerP->info.position.vPos;
	CFixMatrix		mListenerOrient = gameData.objs.viewerP->info.position.mOrient;
	short				nListenerSeg = gameData.objs.viewerP->info.nSegment;

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
		nOldVolume = fix (X2F (2 * nOldVolume) * MIX_MAX_VOLUME + 0.5f);
#endif
#if DBG
		if ((nOldVolume <= 0) && (soundObjP->m_channel >= 0))
			nDbgSeg = nDbgSeg;
#endif
		nOldPan = soundObjP->m_pan;
		// Check if its done.
		if (!(soundObjP->m_flags & SOF_PLAY_FOREVER) && ((soundObjP->m_channel < 0) && !ChannelIsPlaying (soundObjP->m_channel))) {
			DeleteSoundObject (i);
			continue;		// Go on to next sound...
			}
		if (soundObjP->m_flags & SOF_LINK_TO_POS) {
			int nVolume = soundObjP->m_volume;
			int nPan = soundObjP->m_pan;
#if DBG
			if (soundObjP->m_linkType.pos.nSegment == nDbgSeg)
				nDbgSeg = nDbgSeg;
#endif
			GetVolPan (
				mListenerOrient, vListenerPos, nListenerSeg,
				soundObjP->m_linkType.pos.position, soundObjP->m_linkType.pos.nSegment, soundObjP->m_maxVolume,
				&soundObjP->m_volume, &soundObjP->m_pan, soundObjP->m_maxDistance, soundObjP->m_nDecay);
			}
		else if (soundObjP->m_flags & SOF_LINK_TO_OBJ) {
			if (gameData.demo.nState == ND_STATE_PLAYBACK) {
				int nObject = NDFindObject (soundObjP->m_linkType.obj.nObjSig);
				objP = OBJECTS + ((nObject > -1) ? nObject : 0);
				}
			else
				objP = OBJECTS + soundObjP->m_linkType.obj.nObject;
			if ((objP->info.nType == OBJ_NONE) || (objP->info.nSignature != soundObjP->m_linkType.obj.nObjSig)) {
				DeleteSoundObject (i);	// The object that this is linked to is dead, so just end this sound if it is looping.
				continue;
				}
			else if ((objP->info.nType == OBJ_EFFECT) && 
						(((objP->info.nId == SOUND_ID) && !objP->rType.soundInfo.bEnabled) || 
						 ((objP->info.nId == LIGHTNING_ID) && !(SHOW_LIGHTNING && objP->rType.lightningInfo.bEnabled)))) {
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
			nNewVolume = fix (X2F (2 * nNewVolume) * MIX_MAX_VOLUME + 0.5f);
#endif
			if ((nOldVolume != nNewVolume) || ((nNewVolume <= 0) != (soundObjP->m_channel < 0))) {
#if DBG
				if (soundObjP->m_linkType.pos.nSegment == nDbgSeg)
					nDbgSeg = nDbgSeg;
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

	uint i = m_objects.ToS ();
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
void CAudio::EndSoundObject (int i)
{
if (m_objects [i].m_flags & SOF_PLAY_FOREVER)
	m_objects [i].m_channel = -1;
else
	DeleteSoundObject (i);
}

//------------------------------------------------------------------------------

void CAudio::StopObjectSounds (void)
{
	uint i = m_objects.ToS ();

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
int CAudio::VerifyChannelFree (int channel)
{
for (int i = 0; i < MAX_SOUND_OBJECTS; i++) {
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

void CSoundQueue::StartSound (short nSound, fix nVolume)
{
	int					i;
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

static int SideIsSoundSource (short nSegment, short nSide)
{
CSegment* segP = &SEGMENTS [nSegment];
if (!(segP->IsDoorWay (nSide, NULL) & WID_VISIBLE_FLAG))
	return -1;
short nOvlTex = segP->m_sides [nSide].m_nOvlTex;
short nEffect = nOvlTex ? gameData.pig.tex.tMapInfoP [nOvlTex].nEffectClip : -1;
if (nEffect < 0)
	nEffect = gameData.pig.tex.tMapInfoP [segP->m_sides [nSide].m_nBaseTex].nEffectClip;
if (nEffect < 0)
	return -1;
int nSound = gameData.eff.effectP [nEffect].nSound;
if (nSound == -1)
	return -1;
short nConnSeg = segP->m_children [nSide];

//check for sound on other CSide of CWall.  Don't add on
//both walls if sound travels through CWall.  If sound
//does travel through CWall, add sound for lower-numbered
//CSegment.

if (IS_CHILD (nConnSeg) && (nConnSeg < nSegment) &&
	 (segP->IsDoorWay (nSide, NULL) & (WID_PASSABLE_FLAG | WID_TRANSPARENT_FLAG))) {
	CSegment* connSegP = SEGMENTS + segP->m_children [nSide];
	short nConnSide = segP->ConnectedSide (connSegP);
	if (connSegP->m_sides [nConnSide].m_nOvlTex == segP->m_sides [nSide].m_nOvlTex)
		return -1;		//skip this one
	}
return nSound;
}

//------------------------------------------------------------------------------

//go through this level and start any effect sounds
void SetSoundSources (void)
{
	short			nSegment, nSide;
	CSegment*	segP;
	CObject*		objP;
	int			nSegSoundSources, nSideSounds [6];
	//int			i;

SetD1Sound ();
audio.InitSounds ();		//clear old sounds
gameStates.sound.bDontStartObjects = 1;
for (segP = SEGMENTS.Buffer (), nSegment = 0; nSegment <= gameData.segs.nLastSegment; segP++, nSegment++) {
#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	for (nSide = 0, nSegSoundSources = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
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
	for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) 
		if (nSideSounds [nSide] >= 0)
			audio.CreateSegmentSound (nSideSounds [nSide], nSegment, nSide, segP->SideCenter (nSide), 1, I2X (1) / (2 * nSegSoundSources));
	}

FORALL_EFFECT_OBJS (objP, i)
	if (objP->info.nId == SOUND_ID) {
		char fn [FILENAME_LEN];
#if 0 //DBG
		if (strcmp (objP->rType.soundInfo.szFilename, "steam"))
			continue;
#endif
		sprintf (fn, "%s.wav", objP->rType.soundInfo.szFilename);
		audio.CreateObjectSound (-1, SOUNDCLASS_AMBIENT, objP->Index (), 1, objP->rType.soundInfo.nVolume, I2X (256), 0, 0, fn);
		}

short nSound = audio.GetSoundByName ("explode2");
if (0 <= nSound) {
	FORALL_STATIC_OBJS (objP, i)
		if (objP->info.nType == OBJ_EXPLOSION) {
			objP->info.renderType = RT_POWERUP;
			objP->rType.vClipInfo.nClipIndex = objP->info.nId;
			audio.CreateObjectSound (nSound, SOUNDCLASS_AMBIENT, objP->Index ());
			}
	}
//gameStates.sound.bD1Sound = 0;
gameStates.sound.bDontStartObjects = 0;
}

//------------------------------------------------------------------------------
//eof
