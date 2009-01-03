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

#include "inferno.h"
#include "fix.h"
#include "mono.h"
#include "timer.h"
#include "joy.h"
#include "key.h"
#include "newdemo.h"
#include "error.h"
#include "text.h"
#include "kconfig.h"
#include "gameseg.h"
#include "midi.h"
#include "audio.h"

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
// only use up to 1/4 the sound channels for "permanent" sounts
if ((m_flags & SOF_PERMANENT) &&
	 (audio.ActiveObjects () >= max (1, audio.GetMaxChannels () / 4)))
	return false;
// start the sample playing
m_channel =
	audio.StartSound (m_nSound, m_soundClass, m_volume, m_pan, m_flags & SOF_PLAY_FOREVER, m_nLoopStart, m_nLoopEnd, 
							this - audio.Objects ().Buffer (), I2X (1), m_szSound,
							(m_flags & SOF_LINK_TO_OBJ) ? &OBJECTS [m_linkType.obj.nObject].info.position.vPos : &m_linkType.pos.position);
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
return (nSound == 255) ? -1 : CAudio::UnXlatSound (nSound);
}

//------------------------------------------------------------------------------
// determine nVolume and panning of a sound created at location nSoundSeg,vSoundPos
// as heard by nListenerSeg,vListenerPos

void CAudio::GetVolPan (CFixMatrix& mListener, CFixVector& vListenerPos, short nListenerSeg, 
								CFixVector& vSoundPos, short nSoundSeg, 
								fix maxVolume, int *nVolume, int *pan, fix maxDistance, int nDecay)
{
	CFixVector	vecToSound;
	fix 			angleFromEar, cosang, sinang;
	fix			distance, pathDistance;
	float			fDecay;

*nVolume = 0;
*pan = 0;
if (nDecay)
	maxDistance *= 2;
else
	maxDistance = (5 * maxDistance) / 4;	// Make all sounds travel 1.25 times as far.
distance = CFixVector::NormalizedDir (vecToSound, vSoundPos, vListenerPos);
if (distance < maxDistance) {
	int nSearchSegs = X2I (maxDistance / 10);
	if (nSearchSegs < 1)
		nSearchSegs = 1;
	pathDistance = FindConnectedDistance (vListenerPos, nListenerSeg, vSoundPos, nSoundSeg, nSearchSegs, WID_RENDPAST_FLAG | WID_FLY_FLAG, 0);
	if (pathDistance > -1) {
		if (!nDecay)
			*nVolume = maxVolume - FixDiv (pathDistance, maxDistance);
		else if (nDecay == 1) {
			fDecay = (float) exp (-log (2.0f) * 4.0f * X2F (pathDistance) / X2F (maxDistance / 2));
			*nVolume = (int) (maxVolume * fDecay);
			}
		else {
			fDecay = 1.0f - X2F (pathDistance) / X2F (maxDistance);
			*nVolume = (int) (maxVolume * fDecay * fDecay * fDecay);
			}

		if (*nVolume <= 0)
			*nVolume = 0;
		else {
			angleFromEar = CFixVector::DeltaAngleNorm (mListener.RVec (), vecToSound, &mListener.UVec ());
			FixSinCos (angleFromEar, &sinang, &cosang);
			if (gameConfig.bReverseChannels || gameOpts->sound.bHires)
				cosang = -cosang;
			*pan = (cosang + I2X (1)) / 2;
			}
		}
	}
}

//------------------------------------------------------------------------------

int CAudio::PlaySound (short nSound, int nSoundClass, fix nVolume, int nPan, int bNoDups, int nLoops, const char* pszWAV, CFixVector* vPos)
{
#if 0
if (vPos && (nVolume < 10))
	return;
#endif
if (!pszWAV) {
#ifdef NEWDEMO
	if (gameData.demo.nState == ND_STATE_RECORDING)
		if (bNoDups)
			NDRecordSound3DOnce (nSound, nPan, nVolume);
		else
			NDRecordSound3D (nSound, nPan, nVolume);
#endif
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
StopAllSounds ();
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
//sounds longer than this get their 3d aspects updated
//#define SOUND_3D_THRESHHOLD  (gameOpts->sound.digiSampleRate * 3 / 2)	//1.5 seconds

int CAudio::CreateObjectSound (
	short nOrgSound, int nSoundClass, short nObject, int bForever, fix maxVolume, fix maxDistance,
	int nLoopStart, int nLoopEnd, const char* pszSound, int nDecay)
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
	PlaySound (nOrgSound, SOUNDCLASS_GENERIC, nVolume, nPan, 0, -1, pszSound, &objP->info.position.vPos);
	return -1;
	}
#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordCreateObjectSound (nOrgSound, nObject, maxVolume, maxDistance, nLoopStart, nLoopEnd);
#endif
if (!m_objects.Push ())
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
	if ((soundObjP->m_channel < 0) && (! (soundObjP->m_flags & SOF_PLAY_FOREVER))) {
		m_objects.Pop ();
		return -1;
		}
	m_info.nActiveObjects++;
	}
return soundObjP->m_nSignature;
}

//------------------------------------------------------------------------------

int CAudio::ChangeObjectSound (int nObject, fix nVolume)
{

	CSoundObject*	soundObjP = m_objects.Buffer ();
	int				nKilled = 0;

#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordDestroyObjectSound (nObject);
#endif

for (uint i = 0; i < m_objects.ToS (); i++, soundObjP++) {
	if ((soundObjP->m_flags & (SOF_USED | SOF_LINK_TO_OBJ)) == (SOF_USED | SOF_LINK_TO_OBJ)) {
		if ((soundObjP->m_linkType.obj.nObject == nObject) && (soundObjP->m_channel > -1) && (soundObjP->m_volume != nVolume)) {
			SetVolume (soundObjP->m_channel, soundObjP->m_volume = nVolume);
			return 1;
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int CAudio::DestroyObjectSound (int nObject)
{

	CSoundObject*	soundObjP = m_objects.Buffer ();
	int				nKilled = 0;

#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordDestroyObjectSound (nObject);
#endif

if (nObject == LOCALPLAYER.nObject)
	gameData.multiplayer.bMoving = -1;

for (uint i = 0; i < m_objects.ToS (); i++, soundObjP++) {
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
	if (soundObjP->m_channel > -1) {
		StopSound (soundObjP->m_channel);
		m_info.nActiveObjects--;
		}
	soundObjP->m_linkType.obj.nObject = -1;
	soundObjP->m_linkType.obj.nObjSig = -1;
	soundObjP->m_channel = -1;
	soundObjP->m_flags = 0;	// Mark as dead, so some other sound can use this sound
	m_objects.Delete (i);
	nKilled++;
	i--, soundObjP--; // stack top has been moved here, so compensate for loop increment
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
if (!m_objects.Push ())
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
	m_info.nActiveObjects++;
	}
return soundObjP->m_nSignature;
}

//------------------------------------------------------------------------------
//if nSound==-1, kill any sound
int CAudio::DestroySegmentSound (short nSegment, short nSide, short nSound)
{
	int				nKilled;
	CSoundObject*	soundObjP = m_objects.Buffer ();

if (nSound != -1)
	nSound = XlatSound (nSound);
nKilled = 0;
for (uint i = 0; i < m_objects.ToS (); i++, soundObjP++) {
	if ((soundObjP->m_flags & (SOF_USED | SOF_LINK_TO_POS)) == (SOF_USED | SOF_LINK_TO_POS)) {
		if ((soundObjP->m_linkType.pos.nSegment == nSegment) && (soundObjP->m_linkType.pos.nSide == nSide) &&
			 ((nSound == -1) || (soundObjP->m_nSound == nSound))) {
			if (soundObjP->m_channel > -1) {
				StopSound (soundObjP->m_channel);
				m_info.nActiveObjects--;
				}
			soundObjP->m_channel = -1;
			soundObjP->m_flags = 0;	// Mark as dead, so some other sound can use this sound
			nKilled++;
			m_objects.Delete (i);
			i--, soundObjP--;
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
	int				oldvolume, oldpan;
	CSoundObject*	soundObjP = m_objects.Buffer ();
	CObject*			objP;

if (gameData.demo.nState == ND_STATE_RECORDING) {
	if (!gameStates.sound.bWasRecording)
		RecordSoundObjects ();
	gameStates.sound.bWasRecording = 1;
	}
else
	gameStates.sound.bWasRecording = 0;
soundQueue.Process ();
for (uint i = 0; i < m_objects.ToS (); i++, soundObjP++) {
	if (soundObjP->m_flags & SOF_USED) {
		oldvolume = soundObjP->m_volume;
		oldpan = soundObjP->m_pan;
		// Check if its done.
		if (!(soundObjP->m_flags & SOF_PLAY_FOREVER) && (soundObjP->m_channel > -1) && !ChannelIsPlaying (soundObjP->m_channel)) {
			StopActiveSound (soundObjP->m_channel);
			m_objects.Delete (i);
			i--, soundObjP--;
			m_info.nActiveObjects--;
			continue;		// Go on to next sound...
			}
		if (soundObjP->m_flags & SOF_LINK_TO_POS) {
			GetVolPan (
				gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos, gameData.objs.viewerP->info.nSegment,
				soundObjP->m_linkType.pos.position, soundObjP->m_linkType.pos.nSegment, soundObjP->m_maxVolume,
				&soundObjP->m_volume, &soundObjP->m_pan, soundObjP->m_maxDistance, soundObjP->m_nDecay);
#if USE_SDL_MIXER
			if (gameOpts->sound.bUseSDLMixer)
				Mix_VolPan (soundObjP->m_channel, soundObjP->m_volume, soundObjP->m_pan);
#endif
			}
		else if (soundObjP->m_flags & SOF_LINK_TO_OBJ) {
			if (gameData.demo.nState == ND_STATE_PLAYBACK) {
				int nObject = NDFindObject (soundObjP->m_linkType.obj.nObjSig);
				objP = OBJECTS + ((nObject > -1) ? nObject : 0);
				}
			else
				objP = OBJECTS + soundObjP->m_linkType.obj.nObject;
			if ((objP->info.nType == OBJ_NONE) || (objP->info.nSignature != soundObjP->m_linkType.obj.nObjSig)) {
			// The CObject that this is linked to is dead, so just end this sound if it is looping.
				if (soundObjP->m_channel > -1) {
					if (soundObjP->m_flags & SOF_PLAY_FOREVER)
						StopSound (soundObjP->m_channel);
					else
						StopActiveSound (soundObjP->m_channel);
					m_info.nActiveObjects--;
					}
				m_objects.Delete (i);
				i--, soundObjP--;
				continue;		// Go on to next sound...
				}
			else {
				GetVolPan (
					gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos,
					gameData.objs.viewerP->info.nSegment, objP->info.position.vPos, objP->info.nSegment, soundObjP->m_maxVolume,
					&soundObjP->m_volume, &soundObjP->m_pan, soundObjP->m_maxDistance, soundObjP->m_nDecay);
#if USE_SDL_MIXER
				if (gameOpts->sound.bUseSDLMixer)
					Mix_VolPan (soundObjP->m_channel, soundObjP->m_volume, soundObjP->m_pan);
#endif
				}
			}
		if (oldvolume != soundObjP->m_volume) {
			if (soundObjP->m_volume < 1) {
			// Sound is too far away, so stop it from playing.
				if (soundObjP->m_channel > -1) {
					if (soundObjP->m_flags & SOF_PLAY_FOREVER)
						StopSound (soundObjP->m_channel);
					else
						StopActiveSound (soundObjP->m_channel);
					m_info.nActiveObjects--;
					soundObjP->m_channel = -1;
					}
				if (!(soundObjP->m_flags & SOF_PLAY_FOREVER)) {
					m_objects.Delete (i);
					i--, soundObjP--;
					continue;
					}
				}
			else {
				if (soundObjP->m_channel < 0)
					soundObjP->Start ();
				else
					SetVolume (soundObjP->m_channel, soundObjP->m_volume);
				}
			}
		if ((oldpan != soundObjP->m_pan) && (soundObjP->m_channel > -1))
			SetPan (soundObjP->m_channel, soundObjP->m_pan);
	}
}

//------------------------------------------------------------------------------

#if DBG
//	SoundDebug ();
#endif
}

void CAudio::PauseSounds (void)
{
PauseLoopingSound ();
for (uint i = 0; i < m_objects.ToS (); i++) {
	if ((m_objects [i].m_flags & SOF_USED) && (m_objects [i].m_channel > -1)) {
		StopSound (m_objects [i].m_channel);
		if (!(m_objects [i].m_flags & SOF_PLAY_FOREVER)) {
			m_objects.Delete (i);
			i--;
			}
		m_info.nActiveObjects--;
		}
	}
StopAllSounds ();
soundQueue.Pause ();
}

//------------------------------------------------------------------------------

void CAudio::PauseAll (void)
{
midi.Pause ();
PauseSounds ();
}

//------------------------------------------------------------------------------

void CAudio::ResumeSounds (void)
{
SyncSounds ();	//don't think we really need to do this, but can't hurt
ResumeLoopingSound ();
}

//------------------------------------------------------------------------------

void CAudio::ResumeAll (void)
{
midi.Resume ();
ResumeLoopingSound ();
}

//------------------------------------------------------------------------------
// Called by the code in digi.c when another sound takes this sound CObject's
// slot because the sound was done playing.
void CAudio::EndSoundObject (int i)
{
Assert (m_objects [i].m_flags & SOF_USED);
Assert (m_objects [i].m_channel > -1);
m_info.nActiveObjects--;
m_objects [i].m_channel = -1;
}

//------------------------------------------------------------------------------

void CAudio::StopObjectSounds (void)
{
for (int i = 0; i < MAX_SOUND_OBJECTS; i++) {
	if (m_objects [i].m_flags & SOF_USED) {
		if (m_objects [i].m_channel > -1) {
			StopSound (m_objects [i].m_channel);
			m_info.nActiveObjects--;
			}
		m_objects [i].m_flags = 0;	// Mark as dead, so some other sound can use this sound
		}
	}
}

//------------------------------------------------------------------------------

void CAudio::StopAll (void)
{
StopCurrentSong ();
StopAllSounds ();
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
//eof
