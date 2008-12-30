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
			NDRecordSound3DOnce (nSound, angle, nVolume);
		else
			NDRecordSound3D (nSound, angle, nVolume);
#endif
	nSound = (nSound < 0) ? -nSound : CAudio::XlatSound (nSound);
	if (nSound < 0)
		return;
	int nChannel = FindChannel (nSound);
	if (nChannel > -1)
		StopSound (nChannel);
	}
// start the sample playing
return StartSound (nSound, nSoundClass, nVolume, nPan, 0, (nLoops > 0) ? nLoops - 1 : -1, -1, -1, I2X (1), pszWAV, vPos);
}

//------------------------------------------------------------------------------

void SoundQueue::Init ();
void SoundQueue::Process ();
void SoundQueue::Pause ();

void CAudio::InitSounds (void)
{
	int i;

SoundQInit ();
StopAllSounds ();
DigiStopLoopingSound ();
for (i = 0; i < MAX_SOUND_OBJECTS; i++) {
	m_objects [i].channel = -1;
	m_objects [i].flags = 0;	// Mark as dead, so some other sound can use this sound
	}
m_info.nActiveObjects = 0;
m_info.bSoundsInitialized = 1;
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
							 m_info.nLoopingStart, m_info.nLoopingEnd, -1, I2X (1), NULL);
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
//hack to not start CObject when loading level

void CAudio::StartSoundObject (int i)
{
	CSoundObject	*soP = m_objects + i;
	// start sample structures
soP->channel = -1;
if (soP->nVolume <= 0)
	return;
if (gameStates.sound.bDontStartObjects)
	return;
// -- MK, 2/22/96 -- 	if (gameData.demo.nState == ND_STATE_RECORDING)
// -- MK, 2/22/96 -- 		NDRecordSound3DOnce (DigiUnXlatSound (soP->nSound), soP->pan, soP->nVolume);
// only use up to half the sound channels for "permanent" sounts
if ((soP->flags & SOF_PERMANENT) &&
	 (m_info.nActiveObjects >= max (1, CAudio::GetMaxChannels () / 4)))
	return;
// start the sample playing
soP->channel =
	StartSound (
		soP->nSound,
		soP->soundClass,
		soP->nVolume,
		soP->pan,
		soP->flags & SOF_PLAY_FOREVER,
		soP->nLoopStart,
		soP->nLoopEnd, i, I2X (1),
		soP->szSound,
		(soP->flags & SOF_LINK_TO_OBJ) ? &OBJECTS [soP->linkType.obj.nObject].info.position.vPos : &soP->linkType.pos.position,
		soP->nDecay != 0);
if (soP->channel > -1)
	m_info.nActiveObjects++;
}

//------------------------------------------------------------------------------
//sounds longer than this get their 3d aspects updated
//#define SOUND_3D_THRESHHOLD  (gameOpts->sound.digiSampleRate * 3 / 2)	//1.5 seconds

int CreateObjectSound (
	short nOrgSound, int nSoundClass, short nObject, int bForever, fix maxVolume, fix maxDistance,
	int nLoopStart, int nLoopEnd, const char* pszSound, int nDecay)
{
	CObject*			objP;
	CSoundObject*	soP;
	int				i, nVolume, pan;
	short				nSound = 0;

if (maxVolume < 0)
	return -1;
if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0]))
	return -1;
if (!(pszSound && *pszSound)) {
	nSound = CAudio::XlatSound (nOrgSound);
	if (nSound < 0)
		return -1;
	if (!gameData.pig.sound.sounds [gameStates.sound.bD1Sound][nSound].data) {
		Int3 ();
		return -1;
		}
	}
objP = OBJECTS + nObject;
if (!bForever) { 	// Hack to keep sounds from building up...
	GetVolPan (gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos,
				  gameData.objs.viewerP->info.nSegment, objP->info.position.vPos, objP->info.nSegment, maxVolume, &nVolume, &pan,
				  maxDistance, nDecay);
	PlaySound (nOrgSound, SOUNDCLASS_GENERIC, nVolume, pan, 0, -1, pszSound, &objP->info.position.vPos);
	return -1;
	}
#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordCreateObjectSound (nOrgSound, nObject, maxVolume, maxDistance, nLoopStart, nLoopEnd);
#endif
for (i = 0, soP = m_objects; i < MAX_SOUND_OBJECTS; i++, soP++)
	if (!soP->flags)
		break;
if (i == MAX_SOUND_OBJECTS)
	return -1;
soP->nSignature = m_info.nNextSignature++;
soP->flags = SOF_USED | SOF_LINK_TO_OBJ;
if (bForever)
	soP->flags |= SOF_PLAY_FOREVER;
soP->linkType.obj.nObject = nObject;
soP->linkType.obj.nObjSig = objP->info.nSignature;
soP->maxVolume = maxVolume;
soP->maxDistance = maxDistance;
soP->soundClass = nSoundClass;
soP->nVolume = 0;
soP->pan = 0;
soP->nSound = (pszSound && *pszSound) ? -1 : nSound;
soP->nDecay = nSoundClass;
if (pszSound)
	strncpy (soP->szSound, pszSound, sizeof (soP->szSound));
else
	*(soP->szSound) = '\0';
soP->nLoopStart = nLoopStart;
soP->nLoopEnd = nLoopEnd;
if (gameStates.sound.bDontStartObjects) { 		//started at level start
	soP->flags |= SOF_PERMANENT;
	soP->channel = -1;
	}
else {
	DigiGetVolPan (
		gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos,
		gameData.objs.viewerP->info.nSegment, objP->info.position.vPos, objP->info.nSegment, soP->maxVolume,
      &soP->nVolume, &soP->pan, soP->maxDistance, soP->nDecay);
	DigiStartSoundObject (i);
	// If it's a one-shot sound effect, and it can't start right away, then
	// just cancel it and be done with it.
	if ((soP->channel < 0) && (! (soP->flags & SOF_PLAY_FOREVER))) {
		soP->flags = 0;
		return -1;
		}
	}
return soP->nSignature;
}

//------------------------------------------------------------------------------

int CAudio::ChangeObjectSound (int nObject, fix nVolume)
{

	int				i, nKilled;
	CSoundObject	*soP;

	nKilled = 0;

#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordDestroyObjectSound (nObject);
#endif

for (i = 0, soP = m_objects; i < MAX_SOUND_OBJECTS; i++, soP++) {
	if ((soP->flags & (SOF_USED | SOF_LINK_TO_OBJ)) == (SOF_USED | SOF_LINK_TO_OBJ)) {
		if (soP->linkType.obj.nObject == nObject) {
			if ((soP->channel > -1) && (soP->nVolume != nVolume)) {
				SetVolume (soP->channel, soP->nVolume = nVolume);
				return 1;
				}
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int CAudio::DestroyObjectSound (int nObject)
{

	int				i, nKilled;
	CSoundObject	*soP;

	nKilled = 0;

#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordDestroyObjectSound (nObject);
#endif

if (nObject == LOCALPLAYER.nObject) {
	gameData.multiplayer.bMoving = -1;
	}
for (i = 0, soP = m_objects; i < MAX_SOUND_OBJECTS; i++, soP++) {
	if ((soP->flags & (SOF_USED | SOF_LINK_TO_OBJ)) == (SOF_USED | SOF_LINK_TO_OBJ)) {
		if (soP->linkType.obj.nObject == nObject) {
			if (soP->channel > -1) {
				StopSound (soP->channel);
				m_info.nActiveObjects--;
				}
			soP->channel = -1;
			soP->flags = 0;	// Mark as dead, so some other sound can use this sound
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

	int i, nVolume, pan;
	int nSound;
	CSoundObject *soP;

nSound = CAudio::XlatSound (nOrgSound);
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
	DigiGetVolPan (gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos, gameData.objs.viewerP->info.nSegment,
					   vPos, nSegment, maxVolume, &nVolume, &pan, maxDistance, 0);
	PlaySound (nOrgSound, SOUNDCLASS_GENERIC, nVolume, pan, 0, -1, pszSound, &vPos);
	return -1;
	}
for (i = 0, soP = m_objects; i < MAX_SOUND_OBJECTS; i++, soP++)
	if (soP->flags == 0)
		break;
if (i == MAX_SOUND_OBJECTS) {
	return -1;
	}
soP->nSignature = m_info.nNextSignature++;
soP->flags = SOF_USED | SOF_LINK_TO_POS;
if (bForever)
	soP->flags |= SOF_PLAY_FOREVER;
soP->linkType.pos.nSegment = nSegment;
soP->linkType.pos.nSide = nSide;
soP->linkType.pos.position = vPos;
soP->nSound = nSound;
soP->soundClass = SOUNDCLASS_GENERIC;
soP->maxVolume = maxVolume;
soP->maxDistance = maxDistance;
if (pszSound)
	strncpy (soP->szSound, pszSound, sizeof (soP->szSound));
else
	*soP->szSound = '\0';
soP->nVolume = 0;
soP->pan = 0;
soP->nDecay = 0;
soP->nLoopStart = soP->nLoopEnd = -1;
if (gameStates.sound.bDontStartObjects) {		//started at level start
	soP->flags |= SOF_PERMANENT;
	soP->channel = -1;
	}
else {
	DigiGetVolPan (
		gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos,
		gameData.objs.viewerP->info.nSegment, soP->linkType.pos.position,
		soP->linkType.pos.nSegment, soP->maxVolume, &soP->nVolume, &soP->pan, soP->maxDistance, soP->nDecay);
	DigiStartSoundObject (i);
	// If it's a one-shot sound effect, and it can't start right away, then
	// just cancel it and be done with it.
	if ((soP->channel < 0) && (! (soP->flags & SOF_PLAY_FOREVER))) {
		soP->flags = 0;
		return -1;
		}
	}
return soP->nSignature;
}

//------------------------------------------------------------------------------
//if nSound==-1, kill any sound
int CAudio::DestroySegmentSound (short nSegment, short nSide, short nSound)
{
	int				i, nKilled;
	CSoundObject	*soP;

if (nSound != -1)
	nSound = XlatSound (nSound);
nKilled = 0;
for (i = 0, soP = m_objects; i < MAX_SOUND_OBJECTS; i++, soP++) {
	if ((soP->flags & (SOF_USED | SOF_LINK_TO_POS)) == (SOF_USED | SOF_LINK_TO_POS)) {
		if ((soP->linkType.pos.nSegment == nSegment) && (soP->linkType.pos.nSide==nSide) &&
			 ((nSound == -1) || (soP->nSound == nSound))) {
			if (soP->channel > -1) {
				StopSound (soP->channel);
				m_info.nActiveObjects--;
				}
			soP->channel = -1;
			soP->flags = 0;	// Mark as dead, so some other sound can use this sound
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
	int i;

for (i = 0; i < MAX_SOUND_OBJECTS; i++) {
	if ((m_objects [i].flags & (SOF_USED | SOF_LINK_TO_OBJ | SOF_PLAY_FOREVER))
		 == (SOF_USED | SOF_LINK_TO_OBJ | SOF_PLAY_FOREVER)) {
		NDRecordCreateObjectSound (DigiUnXlatSound (m_objects [i].nSound), m_objects [i].linkType.obj.nObject,
											 m_objects [i].maxVolume, m_objects [i].maxDistance, m_objects [i].nLoopStart,
											 m_objects [i].nLoopEnd);
		}
	}
}

//------------------------------------------------------------------------------

void Mix_VolPan (int channel, int vol, int pan);

void CAudio::SyncSounds (void)
{
	int				i, oldvolume, oldpan;
	CSoundObject	*soP;
	CObject			*objP;

if (gameData.demo.nState == ND_STATE_RECORDING) {
	if (!gameStates.sound.bWasRecording)
		DigiRecordsoundObjects ();
	gameStates.sound.bWasRecording = 1;
	}
else
	gameStates.sound.bWasRecording = 0;
SoundQProcess ();
for (i = 0, soP = m_objects; i < MAX_SOUND_OBJECTS; i++, soP++) {
	if (soP->flags & SOF_USED) {
		oldvolume = soP->nVolume;
		oldpan = soP->pan;
		// Check if its done.
		if (!(soP->flags & SOF_PLAY_FOREVER) && (soP->channel > -1) && !DigiIsChannelPlaying (soP->channel)) {
			StopActiveSound (soP->channel);
			soP->flags = 0;	// Mark as dead, so some other sound can use this sound
			m_info.nActiveObjects--;
			continue;		// Go on to next sound...
			}
		if (soP->flags & SOF_LINK_TO_POS) {
			DigiGetVolPan (
				gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos, gameData.objs.viewerP->info.nSegment,
				soP->linkType.pos.position, soP->linkType.pos.nSegment, soP->maxVolume,
				&soP->nVolume, &soP->pan, soP->maxDistance, soP->nDecay);
#if USE_SDL_MIXER
			if (gameOpts->sound.bUseSDLMixer)
				Mix_VolPan (soP->channel, soP->nVolume, soP->pan);
#endif
			}
		else if (soP->flags & SOF_LINK_TO_OBJ) {
			if (gameData.demo.nState == ND_STATE_PLAYBACK) {
				int nObject = NDFindObject (soP->linkType.obj.nObjSig);
				objP = OBJECTS + ((nObject > -1) ? nObject : 0);
				}
			else
				objP = OBJECTS + soP->linkType.obj.nObject;
			if ((objP->info.nType == OBJ_NONE) || (objP->info.nSignature != soP->linkType.obj.nObjSig)) {
			// The CObject that this is linked to is dead, so just end this sound if it is looping.
				if (soP->channel > -1) {
					if (soP->flags & SOF_PLAY_FOREVER)
						StopSound (soP->channel);
					else
						StopActiveSound (soP->channel);
					m_info.nActiveObjects--;
					}
				soP->flags = 0;	// Mark as dead, so some other sound can use this sound
				continue;		// Go on to next sound...
				}
			else {
				DigiGetVolPan (
					gameData.objs.viewerP->info.position.mOrient, gameData.objs.viewerP->info.position.vPos,
					gameData.objs.viewerP->info.nSegment, objP->info.position.vPos, objP->info.nSegment, soP->maxVolume,
					&soP->nVolume, &soP->pan, soP->maxDistance, soP->nDecay);
#if USE_SDL_MIXER
				if (gameOpts->sound.bUseSDLMixer)
					Mix_VolPan (soP->channel, soP->nVolume, soP->pan);
#endif
				}
			}
		if (oldvolume != soP->nVolume) {
			if (soP->nVolume < 1) {
			// Sound is too far away, so stop it from playing.
				if (soP->channel > -1) {
					if (soP->flags & SOF_PLAY_FOREVER)
						StopSound (soP->channel);
					else
						StopActiveSound (soP->channel);
					m_info.nActiveObjects--;
					soP->channel = -1;
					}
				if (!(soP->flags & SOF_PLAY_FOREVER)) {
					soP->flags = 0;	// Mark as dead, so some other sound can use this sound
					continue;
					}
				}
			else {
				if (soP->channel < 0)
					DigiStartSoundObject (i);
				else
					DigiSetChannelVolume (soP->channel, soP->nVolume);
				}
			}
		if ((oldpan != soP->pan) && (soP->channel > -1))
			DigiSetChannelPan (soP->channel, soP->pan);
	}
}

//------------------------------------------------------------------------------

#if DBG
//	DigiSoundDebug ();
#endif
}

void CAudio::PauseSounds (void)
{
	int i;

PauseLoopingSound ();
for (i = 0; i < MAX_SOUND_OBJECTS; i++) {
	if ((m_objects [i].flags & SOF_USED) && (m_objects [i].channel > -1)) {
		StopSound (m_objects [i].channel);
		if (!(m_objects [i].flags & SOF_PLAY_FOREVER))
			m_objects [i].flags = 0;	// Mark as dead, so some other sound can use this sound
		m_info.nActiveObjects--;
		m_objects [i].channel = -1;
		}
	}
StopAllSounds ();
SoundQPause ();
}

//------------------------------------------------------------------------------

void CAudio::PauseAll (void)
{
PauseMidi ();
PauseSounds ();
}

//------------------------------------------------------------------------------

void CAudio::ResumeDigiSounds ()
{
DigiSyncSounds ();	//don't think we really need to do this, but can't hurt
DigiResumeLoopingSound ();
}

//------------------------------------------------------------------------------

extern void CAudio::ResumeMidi ();

void CAudio::ResumeAll ()
{
DigiResumeMidi ();
DigiResumeLoopingSound ();
}

//------------------------------------------------------------------------------
// Called by the code in digi.c when another sound takes this sound CObject's
// slot because the sound was done playing.
void CAudio::EndSoundObject (int i)
{
Assert (m_objects [i].flags & SOF_USED);
Assert (m_objects [i].channel > -1);
m_info.nActiveObjects--;
m_objects [i].channel = -1;
}

//------------------------------------------------------------------------------

void CAudio::StopDigiSounds (void)
{
	int i;

DigiStopLoopingSound ();
for (i = 0; i < MAX_SOUND_OBJECTS; i++) {
	if (m_objects [i].flags & SOF_USED) {
		if (m_objects [i].channel > -1) {
			StopSound (m_objects [i].channel);
			m_info.nActiveObjects--;
			}
		m_objects [i].flags = 0;	// Mark as dead, so some other sound can use this sound
		}
	}
StopAllSounds ();
SoundQInit ();
}

//------------------------------------------------------------------------------

void CAudio::StopAll (void)
{
DigiStopCurrentSong ();
DigiStopDigiSounds ();
}

//------------------------------------------------------------------------------

#if DBG
int VerifySoundChannelFree (int channel)
{
	int i;

for (i = 0; i < MAX_SOUND_OBJECTS; i++) {
	if (m_objects [i].flags & SOF_USED) {
		if (m_objects [i].channel == channel) {
			Int3 ();	// Get John!
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void CAudio::SoundDebug (void)
{
	int i;
	int n_activeSound_objs=0;
	int nSound_objs=0;

for (i = 0; i < MAX_SOUND_OBJECTS; i++) {
	if (m_objects [i].flags & SOF_USED) {
		nSound_objs++;
		if (m_objects [i].channel > -1)
			n_activeSound_objs++;
		}
	}
Debug ();
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

void SoundQueue::Pause (void)
{
soundQueue.nChannel = -1;
}

//------------------------------------------------------------------------------

void SoundQueue::End (void)
{
	// Current playing sound is stopped, so take it off the Queue
if (++m_data.nHead >= MAX_SOUND_QUEUE)
	m_data.nHead = 0;
m_data.nSounds--;
m_data.nChannel = -1;
}

//------------------------------------------------------------------------------

void SoundQueue::Process (void)
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
	if (q->time_added + MAX_LIFE > curtime) {
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

nSound = CAudio::XlatSound (nSound);
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
	q->time_added = TimerGetApproxSeconds ();
	q->nSound = nSound;
	m_data.nSounds++;
	m_data.nTail = i;
	}
Process ();
}

//------------------------------------------------------------------------------
//eof
