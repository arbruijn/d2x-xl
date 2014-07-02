/*
 *
 * SDL digital audio support
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "text.h"
#include "songs.h"
#include "midi.h"
#include "audio.h"
#include "lightning.h"
#include "soundthreads.h"

//end changes by adb
#define SOUND_BUFFER_SIZE (512)

/* This table is used to add two sound values together and pin
 * the value to avoid overflow.  (used with permission from ARDI)
 * DPH: Taken from SDL/src/SDL_mixer.c.
 */
static const uint8_t mix8[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
  0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
  0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
  0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
  0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
  0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
  0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,
  0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
  0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71,
  0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C,
  0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92,
  0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
  0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
  0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3,
  0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE,
  0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
  0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4,
  0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
  0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5,
  0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

#ifndef _WIN32
void*					sndDevHandle;
pthread_t			threadId;
pthread_mutex_t	mutex;
#endif

#define MAKE_WAV	1
#if MAKE_WAV
#	define	WAVINFO_SIZE	sizeof (tWAVInfo)
#else
#	define	WAVINFO_SIZE	0
#endif

//------------------------------------------------------------------------------

CAudio audio;

static SDL_AudioSpec waveSpec;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/* Audio mixing callback */
//changed on 980905 by adb to cleanup, add nPan support and optimize mixer
void _CDECL_ CAudio::MixCallback (void* userdata, Uint8* stream, int32_t len)
{
if (!audio.Available ())
	return;
memset (stream, 0x80, len); // fix "static" sound bug on Mac OS X

CAudioChannel*	channelP = audio.m_channels.Buffer ();

for (int32_t i = audio.MaxChannels (); i; i--, channelP++)
	channelP->Mix (reinterpret_cast<uint8_t*> (stream), len);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void SDLCALL Mix_FinishChannel (int32_t nChannel)
{
audio.Channel (nChannel)->SetPlaying (0);
}

//------------------------------------------------------------------------------

void Mix_VolPan (int32_t nChannel, int32_t nVolume, int32_t nPan)
{
#if USE_SDL_MIXER
if (!audio.Available ()) 
	return;
if (gameOpts->sound.bUseSDLMixer && (nChannel >= 0)) {
	nVolume = (fix) FRound (X2F (2 * nVolume) /** X2F (audio.Volume ()*/ * MIX_MAX_VOLUME);
	Mix_Volume (nChannel, nVolume);
	if (nPan >= 0) {
		nPan = (fix) FRound (X2F (nPan) * 254);
		Mix_SetPanning (nChannel, uint8_t (nPan), uint8_t (254 - nPan));
		}
	}
#endif
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CAudioChannel::Init (void)
{
m_info.nSound = -1;
m_info.nIndex = -1;
m_info.nPan = 0;				
m_info.nVolume = 0;			
m_info.nLength = 0;			
m_info.nPosition = 0;		
m_info.nSoundObj = -1;		
m_info.nSoundClass = 0;
m_info.bPlaying = 0;		
m_info.bLooped = 0;			
m_info.bPersistent = 0;	
m_info.bResampled = 0;
m_info.bBuiltIn = 0;
#if USE_SDL_MIXER
m_info.mixChunkP = NULL;
m_info.nChannel = 0;
#endif
#if USE_OPENAL
m_info.source = 0;
m_info.loops = 0;
#endif
}

//------------------------------------------------------------------------------

void CAudioChannel::Destroy (void)
{
m_info.sample.Destroy ();
m_info.bResampled = 0;
}

//------------------------------------------------------------------------------

void CAudioChannel::SetVolume (int32_t nVolume)
{
if (m_info.bPlaying) {
	nVolume = FixMulDiv (nVolume, audio.Volume (m_info.bAmbient), I2X (1));
	if (m_info.nVolume != nVolume) {
		m_info.nVolume = nVolume;
#if DBG
		if (nVolume == 0)
			BRP;
#endif
#if USE_SDL_MIXER
		if (gameOpts->sound.bUseSDLMixer)
			Mix_VolPan (int32_t (this - audio.Channel ()), m_info.nVolume, -1);
#endif
		}
	}
}

//------------------------------------------------------------------------------

void CAudioChannel::SetPan (int32_t nPan)
{
if (m_info.bPlaying) {
	m_info.nPan = nPan;
#if USE_SDL_MIXER
	if (gameOpts->sound.bUseSDLMixer) {
		nPan /= (32767 / 127);
		Mix_SetPanning (m_info.nChannel, (uint8_t) nPan, (uint8_t) (254 - nPan));
		}
#endif
	}
}

//------------------------------------------------------------------------------

void CAudioChannel::Stop (void)
{
audio.UnregisterChannel (m_info.nIndex);
#if DBG
if (m_info.nChannel == nDbgChannel)
	BRP;
#endif
m_info.nIndex = -1;
m_info.bPlaying = 0;
m_info.nSoundObj = -1;
m_info.bPersistent = 0;
#if USE_OPENAL
if (gameOpts->sound.bUseOpenAL) {
	if (m_info.source != 0xFFFFFFFF)
		alSourceStop (m_info.source);
	}
#endif
#if USE_SDL_MIXER
if (audio.Available () && gameOpts->sound.bUseSDLMixer) {
	if (m_info.mixChunkP) {
		Mix_HaltChannel (m_info.nChannel);
		if (m_info.bBuiltIn)
			m_info.bBuiltIn = 0;
		else
			Mix_FreeChunk (m_info.mixChunkP);
		m_info.mixChunkP = NULL;
		}
	//Mix_FadeOutChannel (nChannel, 500);
	}
#endif
if (m_info.bResampled) {
	m_info.sample.Destroy ();
	m_info.bResampled = 0;
	}
}

//------------------------------------------------------------------------------

#if MAKE_WAV

void SetupWAVInfo (uint8_t* buffer, int32_t nLength)
{
	tWAVInfo*	infoP = reinterpret_cast<tWAVInfo*> (buffer);

memcpy (infoP->header.chunkID, "RIFF", 4);
infoP->header.chunkSize = nLength + sizeof (tWAVInfo) - 8;
memcpy (infoP->header.riffType, "WAVE", 4);

memcpy (infoP->format.chunkID, "fmt ", 4);
infoP->format.chunkSize = sizeof (tWAVFormat) - sizeof (infoP->format.chunkID) - sizeof (infoP->format.chunkSize);
infoP->format.format = 1; //PCM
infoP->format.channels = 2;
infoP->format.sampleRate = SAMPLE_RATE_22K;
infoP->format.bitsPerSample = (audio.Format () == AUDIO_U8) ? 8 : 16;
infoP->format.blockAlign = infoP->format.channels * ((infoP->format.bitsPerSample + 7) / 8);
infoP->format.avgBytesPerSec = infoP->format.sampleRate * infoP->format.blockAlign;

memcpy (infoP->data.chunkID, "data", 4);
infoP->data.chunkSize = nLength;
}

#endif

//------------------------------------------------------------------------------

static int32_t ResampleFormat (uint16_t *destP, uint8_t* srcP, int32_t nSrcLen)
{
for (int32_t i = nSrcLen; i; i--)
#if 1 
	*destP++ = uint16_t (*srcP++) * 128;
#else
	*destP++ = uint16_t (float (*srcP++) * (32767.0f / 255.0f));
#endif
return nSrcLen;
}

//------------------------------------------------------------------------------

static int32_t ResampleRate (uint16_t* destP, uint16_t* srcP, int32_t nSrcLen)
{
	uint16_t	nSound, nPrevSound;

srcP += nSrcLen;
destP += nSrcLen * 2;

for (int32_t i = 0; i < nSrcLen; i++) {
	nSound = *(--srcP);
	*(--destP) = i ? (nSound + nPrevSound) / 2 : nSound;
	*(--destP) = nSound;
	nPrevSound = nSound;
	}
return nSrcLen * 2;
}

//------------------------------------------------------------------------------

static int32_t ResampleRate (uint8_t* destP, uint8_t* srcP, int32_t nSrcLen)
{
	uint8_t	nSound, nPrevSound;

srcP += nSrcLen;
destP += nSrcLen * 2;

for (int32_t i = 0; i < nSrcLen; i++) {
	nSound = *(--srcP);
	*(--destP) = i ? (nSound + nPrevSound) / 2 : nSound;
	*(--destP) = nSound;
	nPrevSound = nSound;
	}
return nSrcLen * 2;
}

//------------------------------------------------------------------------------

template<typename _T> 
int32_t ResampleChannels (_T* destP, _T* srcP, int32_t nSrcLen)
{
	_T			nSound;
	float		fFade;

srcP += nSrcLen;
destP += nSrcLen * 2;

for (int32_t i = nSrcLen; i; i--) {
	nSound = *(--srcP);
	fFade = float (i) / 500.0f;
	if (fFade > 1.0f)
		fFade = float (nSrcLen - i) / 500.0f;
	if (fFade < 1.0f)
		nSound = _T (float (nSound) * fFade);
	*(--destP) = nSound;
	*(--destP) = nSound;
	}
return nSrcLen * 2;
}

//------------------------------------------------------------------------------
// resample to 16 bit stereo

int32_t CAudioChannel::Resample (CSoundSample *soundP, int32_t bD1Sound, int32_t bMP3)
{
	int32_t	h, i, l;

#if DBG
if (soundP->bCustom)
	BRP;
#endif
l = soundP->nLength [soundP->bCustom];
i = gameOpts->sound.audioSampleRate / gameOpts->sound.soundSampleRate;
h = l * 2 * i;
if (audio.Format () == AUDIO_S16SYS)
	h *= 2;

if (!m_info.sample.Create (h + WAVINFO_SIZE))
	return -1;
m_info.bResampled = 1;

if (audio.Format () == AUDIO_S16SYS) {
	uint16_t* bufP = reinterpret_cast<uint16_t*> (m_info.sample.Buffer () + WAVINFO_SIZE);
	l = ResampleFormat (bufP, soundP->data [soundP->bCustom].Buffer (), l);
	for (; i > 1; i >>= 1)
		l = ResampleRate (bufP, bufP, l);
	l = ResampleChannels (bufP, bufP, l);
	}
else {
	uint8_t* srcP = soundP->data [soundP->bCustom].Buffer ();
	uint8_t* destP = reinterpret_cast<uint8_t*> (m_info.sample.Buffer () + WAVINFO_SIZE);
	for (; i > 1; i >>= 1) {
		l = ResampleRate (destP, srcP, l);
		srcP = destP;
		}
	l = ResampleChannels (destP, srcP, l);
	}

#if MAKE_WAV
SetupWAVInfo (m_info.sample.Buffer (), h);
#endif
return m_info.nLength = h;
}

//------------------------------------------------------------------------------

int32_t CAudioChannel::ReadWAV (void)
{
	CFile	cf;
	int32_t	l;

if (!cf.Open ("d2x-temp.wav", gameFolders.game.szData [0], "rb", 0))
	return 0;
if (0 >= (l = (int32_t) cf.Length ()))
	l = -1;
else if (!m_info.sample.Create (l))
	l = -1;
else if (m_info.sample.Read (cf, l) != (size_t) l)
	l = -1;
cf.Close ();
if ((l < 0) && m_info.sample.Buffer ()) {
	m_info.sample.Destroy ();
	}
return l > 0;
}

//------------------------------------------------------------------------------

int32_t CAudioChannel::Speedup (CSoundSample *soundP, int32_t speed)
{
	int32_t	h, i, j, l;
	uint8_t	*pDest, *pSrc;

l = FixMulDiv (m_info.bResampled ? m_info.nLength : soundP->nLength [soundP->bCustom], speed, I2X (1));
if (!(pDest = new uint8_t [l]))
	return -1;
pSrc = m_info.bResampled ? m_info.sample.Buffer () : soundP->data [soundP->bCustom].Buffer ();
for (h = i = j = 0; i < l; i++) {
	pDest [j] = pSrc [i];
	h += speed;
	while (h >= I2X (1)) {
		j++;
		h -= I2X (1);
		}
	}
if (m_info.bResampled)
	m_info.sample.Destroy ();
else
	m_info.bResampled = 1;
m_info.sample.SetBuffer (pDest, 0, j);
return m_info.nLength = j;
}

//------------------------------------------------------------------------------

#if USE_OPENAL

inline int32_t CAudio::ALError (void)
{
return alcGetError (gameData.pig.sound.openAL.device) != AL_NO_ERROR;
}

#endif

//------------------------------------------------------------------------------

// Volume 0-I2X (1)
void CAudioChannel::SetPlaying (int32_t bPlaying)
{ 
if ((m_info.bPlaying != bPlaying) && !(m_info.bPlaying = bPlaying)) {
	audio.UnregisterChannel (m_info.nIndex);
	m_info.nIndex = -1;
	if (m_info.nSoundObj >= 0)
		audio.EndSoundObject (m_info.nSoundObj);
	}
}

//------------------------------------------------------------------------------

fix CAudioChannel::Duration (void)
{
if (!m_info.mixChunkP)
	return 0;
return F2X (float (m_info.mixChunkP->alen) / float (gameOpts->sound.audioSampleRate * ((audio.Format () == AUDIO_U8) ? 2 : 4)) + 0.5f);
}

//------------------------------------------------------------------------------

// Volume 0-I2X (1)
int32_t CAudioChannel::Start (int16_t nSound, int32_t nSoundClass, fix nVolume, int32_t nPan, int32_t bLooping, 
								  int32_t nLoopStart, int32_t nLoopEnd, int32_t nSoundObj, int32_t nSpeed, 
								  const char *pszWAV, CFixVector* vPos)
{
	CSoundSample*	soundP = NULL;
	int32_t				bPersistent = (nSoundObj > -1) || bLooping || (nVolume > I2X (1));

if (!(pszWAV && *pszWAV && gameOpts->sound.bUseSDLMixer)) {
	if (nSound < 0)
		return -1;
	if (!gameData.pig.sound.nSoundFiles [gameStates.sound.bD1Sound])
		return -1;
	soundP = gameData.pig.sound.sounds [gameStates.sound.bD1Sound] + nSound % gameData.pig.sound.nSoundFiles [gameStates.sound.bD1Sound];
	if (!(soundP->data [soundP->bCustom].Buffer () && soundP->nLength [soundP->bCustom]))
		return -1;
	}
#if DBG
if ((nDbgSound >= 0) && (nSound == nDbgSound))
	BRP;
#endif
if (m_info.bPlaying) {
	SetPlaying (0);
	if (soundQueue.Channel () == audio.FreeChannel ())
		soundQueue.End ();
	}
#if USE_OPENAL
if (m_info.source == 0xFFFFFFFF) {
	CFloatVector	fPos;

	DigiALError ();
	alGenSources (1, &m_info.source);
	if (DigiALError ())
		return -1;
	alSourcei (m_info.source, AL_BUFFER, soundP->buffer);
	if (DigiALError ())
		return -1;
	alSourcef (m_info.source, AL_GAIN, ((nVolume < I2X (1)) ? X2F (nVolume) : 1) * 2 * X2F (m_info.nVolume));
	alSourcei (m_info.source, AL_LOOPING, (ALuint) ((nSoundObj > -1) || bLooping || (nVolume > I2X (1))));
	fPos.Assign (vPos ? *vPos : OBJECTS [LOCALPLAYER.nObject].nPosition.vPos);
	alSourcefv (m_info.source, AL_POSITION, reinterpret_cast<ALfloat*> (fPos));
	alSource3f (m_info.source, AL_VELOCITY, 0, 0, 0);
	alSource3f (m_info.source, AL_DIRECTION, 0, 0, 0);
	if (DigiALError ())
		return -1;
	alSourcePlay (m_info.source);
	if (DigiALError ())
		return -1;
	}
#endif
#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer) {
	if (m_info.mixChunkP) {
		Mix_HaltChannel (m_info.nChannel);
		if (m_info.bBuiltIn)
			m_info.bBuiltIn = 0;
		else
			Mix_FreeChunk (m_info.mixChunkP);
		m_info.mixChunkP = NULL;
		}
	}
#endif
if (m_info.bResampled) {
	m_info.sample.Destroy ();
	m_info.bResampled = 0;
	}

m_info.nVolume = FixMul (audio.Volume (), nVolume);
m_info.nPan = nPan;
m_info.nPosition = 0;
m_info.nSoundObj = nSoundObj;
m_info.nSoundClass = nSoundClass;
m_info.bLooped = bLooping;
#if USE_OPENAL
m_info.loops = bLooping ? -1 : nLoopEnd - nLoopStart + 1;
#endif

#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer) {
	//resample to two channels
	m_info.nChannel = audio.FreeChannel ();
	if (pszWAV && *pszWAV) {
		if (!(m_info.mixChunkP = LoadAddonSound (pszWAV, &m_info.bBuiltIn)))
			return -1;
		}
	else {
		if (!gameOpts->sound.bHires [0]) 
			m_info.mixChunkP = Mix_QuickLoad_RAW (reinterpret_cast<Uint8*> (soundP->data [soundP->bCustom].Buffer ()), soundP->nLength [soundP->bCustom]);
		else if (soundP->bHires) {
			int32_t l = soundP->nLength [soundP->bCustom];
			m_info.sample.SetBuffer (soundP->data [soundP->bCustom].Buffer (), 1, l);
			m_info.mixChunkP = Mix_QuickLoad_WAV (reinterpret_cast<Uint8*> (m_info.sample.Buffer ()));
			}
		else {
			int32_t l = Resample (soundP, (gameStates.sound.bD1Sound || gameStates.app.bDemoData) && (gameOpts->sound.audioSampleRate != SAMPLE_RATE_11K), 0);
			if (l <= 0)
				return -1;
			if (nSpeed < I2X (1))
				l = Speedup (soundP, nSpeed);
#if MAKE_WAV
			m_info.mixChunkP = Mix_QuickLoad_WAV (reinterpret_cast<Uint8*> (m_info.sample.Buffer ()));
#else
			m_info.mixChunkP = Mix_QuickLoad_RAW (reinterpret_cast<Uint8*> (m_info.sample.Buffer ()), l);
#endif
			}
		}
	Mix_VolPan (m_info.nChannel, m_info.nVolume, nPan);
	try {
		Mix_PlayChannel (m_info.nChannel, m_info.mixChunkP, bLooping ? -1 : nLoopEnd - nLoopStart);
		}
	catch (...) {
		PrintLog (0, "Error in Mix_PlayChannel\n");
		}
	}
else 
#else
if (pszWAV && *pszWAV)
	return -1;
#endif
	{
	if ((gameStates.sound.bD1Sound || gameStates.app.bDemoData) && (gameOpts->sound.audioSampleRate != SAMPLE_RATE_11K)) {
		int32_t l = Resample (soundP, 0, 0);
		if (l <= 0)
			return -1;
		m_info.nLength = l;
		}
	else {
		m_info.sample.SetBuffer (soundP->data [soundP->bCustom].Buffer (), 1, m_info.nLength = soundP->nLength [soundP->bCustom]);
		}
	if (nSpeed < I2X (1))
		Speedup (soundP, nSpeed);
	}
m_info.nSound = nSound;
m_info.bAmbient = (nSoundClass == SOUNDCLASS_AMBIENT);
m_info.bPlaying = 1;
m_info.bPersistent = bPersistent;
if (m_info.nIndex < 0)
	m_info.nIndex = audio.RegisterChannel (this);
return audio.FreeChannel ();
}

//------------------------------------------------------------------------------

void CAudioChannel::Mix (uint8_t* stream, int32_t len)
{
if (m_info.bPlaying && m_info.sample.Buffer () && m_info.nLength) {
	uint8_t* streamEnd = stream + len;
	uint8_t* channelData = reinterpret_cast<uint8_t*> (m_info.sample.Buffer () + m_info.nPosition);
	uint8_t* channelEnd = reinterpret_cast<uint8_t*> (m_info.sample.Buffer () + m_info.nLength);
	uint8_t* streamPos = stream, s;
	signed char v;
	fix vl, vr;
	int32_t x;

	if ((x = m_info.nPan) & 0x8000) {
		vl = 0x20000 - x * 2;
		vr = 0x10000;
		}
	else {
		vl = 0x10000;
		vr = x * 2;
		}
	x = m_info.nVolume;
	vl = FixMul (vl, x);
	vr = FixMul (vr, x);
	while (streamPos < streamEnd) {
		if (channelData == channelEnd) {
			if (!m_info.bLooped) {
				SetPlaying (0);
				break;
				}
			channelData = m_info.sample.Buffer ();
			}
		v = *(channelData++) - 0x80;
		s = *streamPos;
		*streamPos++ = mix8 [s + FixMul (v, vl) + 0x80];
		s = *streamPos;
		*streamPos++ = mix8 [s + FixMul (v, vr) + 0x80];
		}
	m_info.nPosition = int32_t (channelData - m_info.sample.Buffer ());
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CAudio::CAudio ()
{
memset (&m_info, 0, sizeof (m_info));
#if 0
m_info.nFormat = AUDIO_S16SYS; 
#else
m_info.nFormat = AUDIO_U8;
#endif
m_info.nVolume [0] = 
m_info.nVolume [1] = SOUND_MAX_VOLUME;
m_info.nMaxChannels = MAX_SOUND_CHANNELS;
Init ();
}

//------------------------------------------------------------------------------

void CAudio::Init (void)
{
m_info.nFreeChannel = 0;
m_info.bInitialized = 0;
m_info.nNextSignature = 0;
m_info.nActiveObjects = 0;
m_info.nLoopingSound = -1;
m_info.nLoopingVolume = 0;
m_info.nLoopingStart = -1;
m_info.nLoopingEnd = -1;
m_info.nLoopingChannel = -1;
m_info.fSlowDown = 1.0f;
m_channels.Resize (m_info.nMaxChannels);
m_usedChannels.Resize (m_info.nMaxChannels);
m_objects.Resize (MAX_SOUND_OBJECTS);
m_bSDLInitialized = false;
m_nListenerSeg = -1;
InitSounds ();
}

//------------------------------------------------------------------------------

void CAudio::Prepare (void)
{
for (int32_t i = 0; i < gameStates.app.nThreads; i++)
	uniDacsRouter [i].Create (gameData.segs.nSegments);
}

//------------------------------------------------------------------------------

void CAudio::Destroy (void)
{
Close ();
m_channels.Destroy ();
m_objects.Destroy ();
}

//------------------------------------------------------------------------------

int32_t CAudio::AudioError (void)
{
SDL_QuitSubSystem (SDL_INIT_AUDIO);
Destroy ();
m_bSDLInitialized = false;
return 1;
}

//------------------------------------------------------------------------------
/* Initialise audio devices. */
int32_t CAudio::InternalSetup (float fSlowDown, int32_t nFormat, const char* driver)
{
if (!gameStates.app.bUseSound)
	return 1;
if (!gameStates.sound.bDynamic && m_info.bAvailable)
	return 1;

if (!m_bSDLInitialized) {
	if (driver && *driver) {
		char szEnvVar [100];
		sprintf (szEnvVar, "SDL_AUDIODRIVER=%s", driver);
		SDL_putenv (szEnvVar);
		}
	if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0)
		return AudioError ();
	m_bSDLInitialized = true;
	}

Init ();
if (nFormat >= 0)
	m_info.nFormat = nFormat;
if (gameStates.app.bNostalgia)
	gameOpts->sound.bHires [0] = 0;
if (gameStates.app.bDemoData)
	gameOpts->sound.audioSampleRate = SAMPLE_RATE_11K;
#if USE_OPENAL
if (gameOpts->sound.bUseOpenAL) {
	gameData.pig.sound.openAL.device = alcOpenDevice (NULL);
	if (!gameData.pig.sound.openAL.device)
		gameOpts->sound.bUseOpenAL = 0;
	else {
		gameData.pig.sound.openAL.context = alcCreateContext (gameData.pig.sound.openAL.device, NULL);
		alcMakeContextCurrent (gameData.pig.sound.openAL.context);
		}
	alcGetError (gameData.pig.sound.openAL.device);
	}
if (!gameOpts->sound.bUseOpenAL)
#endif
#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer) {
	int32_t h;
	if (fSlowDown <= 0)
		fSlowDown = 1.0f;
	m_info.fSlowDown = fSlowDown;
#if 0
	if (gameStates.app.bDemoData)
		h = Mix_OpenAudio (int32_t (gameOpts->sound.audioSampleRate / fSlowDown), m_info.nFormat = AUDIO_U8, 2, SOUND_BUFFER_SIZE);
	else 
#endif
	if (gameOpts->UseHiresSound ())
		h = Mix_OpenAudio (int32_t ((gameOpts->sound.audioSampleRate = SAMPLE_RATE_44K) / fSlowDown), m_info.nFormat = AUDIO_S16SYS, 2, SOUND_BUFFER_SIZE);
	else 
		h = Mix_OpenAudio (int32_t ((gameOpts->sound.audioSampleRate = SAMPLE_RATE_22K) / fSlowDown), m_info.nFormat = AUDIO_U8, 1, SOUND_BUFFER_SIZE);
	if (h < 0)
		return 1;
#if 1
	Mix_Resume (-1);
	Mix_ResumeMusic ();
#endif
	Mix_AllocateChannels (m_info.nMaxChannels);
	Mix_ChannelFinished (Mix_FinishChannel);
	}
else 
#endif
 {
	waveSpec.freq = (int32_t) (gameOpts->sound.audioSampleRate / fSlowDown);
	waveSpec.format = AUDIO_U8;
	waveSpec.channels = 2;
	waveSpec.samples = SOUND_BUFFER_SIZE * (gameOpts->sound.audioSampleRate / SAMPLE_RATE_11K);
	waveSpec.callback = CAudio::MixCallback;
	if (SDL_OpenAudio (&waveSpec, NULL) < 0) {
		SDL_QuitSubSystem (SDL_INIT_AUDIO);
		return 1;
		}
	SDL_PauseAudio (0);
	}
SetFxVolume (Volume (1), 1);
SetFxVolume (Volume ());
m_info.bInitialized =
m_info.bAvailable = 1;
return 0;
}

//------------------------------------------------------------------------------

int32_t CAudio::Setup (float fSlowDown, int32_t nFormat)
{
if (!InternalSetup (fSlowDown, nFormat, NULL))
	return 0;
SDL_QuitSubSystem (SDL_INIT_AUDIO);
if (!InternalSetup (fSlowDown, nFormat, "alsa"))
	return 0;
SDL_QuitSubSystem (SDL_INIT_AUDIO);
PrintLog (0, TXT_SDL_OPEN_AUDIO, SDL_GetError ()); 
PrintLog (0, "\n");
Warning (TXT_SDL_OPEN_AUDIO, SDL_GetError ());
return 1;
}

//------------------------------------------------------------------------------

void CAudio::SetupRouter (void)
{
m_nListenerSeg = -1;
for (int32_t i = 0; i < MAX_THREADS; i++)
	m_router [i].Create (gameData.segs.nSegments) /*&& m_segDists.Create (gameData.segs.nSegments)*/;
}

//------------------------------------------------------------------------------
/* Shut down audio */
void CAudio::Close (void)
{
if (!(gameStates.sound.bDynamic && m_info.bAvailable))
	return;
m_info.bInitialized =
m_info.bAvailable = 0;
#if defined (__MINGW32__) || defined (__macosx__)
SDL_Delay (500); // CloseAudio hangs if it's called too soon after opening?
#endif
#if USE_OPENAL
if (gameOpts->sound.bUseOpenAL) {
	alcMakeContextCurrent (NULL);
	alcDestroyContext (gameData.pig.sound.openAL.context);
	alcCloseDevice (gameData.pig.sound.openAL.device);
	gameData.pig.sound.openAL.device = NULL;
	}
else
#endif
StopAll ();
songManager.StopAll ();
#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer) {
	Mix_CloseAudio ();
	}
else
#endif
	SDL_CloseAudio ();
}

//------------------------------------------------------------------------------

void CAudio::Shutdown (void)
{
if (gameStates.sound.bDynamic && m_info.bAvailable) {
	int32_t nVolume = m_info.nVolume [0];
	SetFxVolume (0);
	m_info.nVolume [0] = nVolume;
	//m_info.bAvailable = 0;
	Close ();
	}
}

//------------------------------------------------------------------------------
/* Toggle audio */
void CAudio::Reset (void) 
{
#if !USE_OPENAL
if (gameStates.sound.bDynamic && m_info.bAvailable) {
	audio.Shutdown ();
	audio.Setup (1);
	}
#endif
}

//------------------------------------------------------------------------------

int32_t CAudio::RegisterChannel (CAudioChannel* channelP)
{
if (m_usedChannels.Push (int32_t (channelP - m_channels.Buffer ())))
	return m_usedChannels.ToS () - 1;
for (int32_t i = int32_t (m_usedChannels.ToS ()); i; )
	if (!m_channels [m_usedChannels [--i]].Playing ())
		UnregisterChannel (i);
#if DBG
if (m_usedChannels.Push (int32_t (channelP - m_channels.Buffer ())))
	return m_usedChannels.ToS () - 1;
return -1;
#else
return m_usedChannels.Push (int32_t (channelP - m_channels.Buffer ())) ? m_usedChannels.ToS () - 1 : -1;
#endif
}

//------------------------------------------------------------------------------

void CAudio::UnregisterChannel (int32_t nIndex)
{
if ((nIndex >= 0) && (nIndex < int32_t (m_usedChannels.ToS ()))) {
	m_usedChannels.Delete ((uint32_t) nIndex);
	if (nIndex < int32_t (m_usedChannels.ToS ())) {
#if 0 //DBG
		if (!m_channels [m_usedChannels [nIndex]].Playing ())
			ArrayError ("error in audio channel registry\n");
		else
#endif
		m_channels [m_usedChannels [nIndex]].SetIndex (nIndex);
		}
	}
}

//------------------------------------------------------------------------------

void CAudio::StopAllChannels (bool bPause)
{
if (!bPause) {
	StopLoopingSound ();
	StopObjectSounds ();
	}
soundQueue.Init ();
lightningManager.Mute ();
for (int32_t i = 0; i < m_info.nMaxChannels; i++)
	audio.StopSound (i);
gameData.multiplayer.bMoving = -1;
gameData.weapons.firing [0].bSound = 0;
}

//------------------------------------------------------------------------------

int32_t CAudio::SoundClassCount (int32_t nSoundClass)
{
	CAudioChannel	*channelP;
	int32_t			h, i;

for (h = 0, i = m_info.nMaxChannels, channelP = audio.m_channels.Buffer (); i; i--, channelP++)
	if (channelP->Playing () && (channelP->SoundClass () == nSoundClass))
		h++;
return h;
}

//------------------------------------------------------------------------------

CAudioChannel* CAudio::FindFreeChannel (int32_t nSoundClass)
{
	CAudioChannel*	channelP, * channelMinVolP [2] = {NULL, NULL};
	int32_t				nStartChannel;
	int32_t				bUseClass = SoundClassCount (nSoundClass) >= m_info.nMaxChannels / 2;

nStartChannel = m_info.nFreeChannel;
DestroyObjectSound (-1); // destroy all 
do {
	channelP = audio.m_channels + m_info.nFreeChannel;
	if (!channelP->Playing ())
		return channelP;
	if ((!bUseClass || (channelP->SoundClass () == nSoundClass)) &&
	    (!channelMinVolP [channelP->Persistent ()] || (channelMinVolP [channelP->Persistent ()]->Volume () > channelP->Volume ())))
		channelMinVolP [channelP->Persistent ()] = channelP;
	m_info.nFreeChannel = (m_info.nFreeChannel + 1) % m_info.nMaxChannels;
	} while (m_info.nFreeChannel != nStartChannel);
m_info.nFreeChannel = audio.m_channels.Index (channelMinVolP [0] ? channelMinVolP [0] : channelMinVolP [1]);
return channelMinVolP [0] ? channelMinVolP [0] : channelMinVolP [1];
}

//------------------------------------------------------------------------------

// Volume 0-I2X (1)
int32_t CAudio::StartSound (int16_t nSound, int32_t nSoundClass, fix nVolume, int32_t nPan, int32_t bLooping, 
								int32_t nLoopStart, int32_t nLoopEnd, int32_t nSoundObj, int32_t nSpeed, 
								const char *pszWAV, CFixVector* vPos)
{
	CAudioChannel*	channelP;

if (!gameStates.app.bUseSound)
	return -1;
if (!m_info.bAvailable) 
	return -1;
if (((nSoundObj > -1) || bLooping || (nVolume > I2X (1))) && !nSoundClass)
	nSoundClass = -1;
if (!(channelP = FindFreeChannel (nSoundClass)))
	return -1;
if (0 > channelP->Start (nSound, nSoundClass, nVolume, nPan, bLooping, nLoopStart, nLoopEnd, nSoundObj, nSpeed, pszWAV, vPos)) {
	return -1;
	}
int32_t i = m_info.nFreeChannel;
if (++m_info.nFreeChannel >= m_info.nMaxChannels)
	m_info.nFreeChannel = 0;
return i;
}

//------------------------------------------------------------------------------
// Returns the nChannel a sound number is bPlaying on, or -1 if none.

int32_t CAudio::FindChannel (int16_t nSound)
{
if (!gameStates.app.bUseSound)
	return -1;
if (!m_info.bAvailable) 
	return -1;
if (nSound < 0)
	return -1;
CSoundSample *soundP = &gameData.pig.sound.sounds [gameStates.sound.bD1Sound][nSound];
if (!soundP->data [soundP->bCustom].Buffer ()) {
	Int3 ();
	return -1;
	}
return -1;
}

//------------------------------------------------------------------------------
//added on 980905 by adb from original source to make sfx nVolume work
void CAudio::SetFxVolume (int32_t fxVolume, int32_t nType)
{
if (!gameStates.app.bUseSound)
	return;
#ifdef _WIN32
int32_t nVolume = FixMulDiv (fxVolume, SOUND_MAX_VOLUME, 0x7fff);
if (nVolume > SOUND_MAX_VOLUME)
	m_info.nVolume [nType] = SOUND_MAX_VOLUME;
else if (nVolume < 0)
	m_info.nVolume [nType] = 0;
else
	m_info.nVolume [nType] = nVolume;
if (!m_info.bAvailable) 
	return;
if (!(nType || songManager.Playing ()))
	midi.FixVolume (FixMulDiv (fxVolume, 128, SOUND_MAX_VOLUME));
#endif
SyncSounds ();
}

//------------------------------------------------------------------------------

void CAudio::SetVolumes (int32_t fxVolume, int32_t midiVolume)
{
if (!gameStates.app.bUseSound)
	return;
SetFxVolume (fxVolume);
midi.SetVolume (midiVolume);
}

//------------------------------------------------------------------------------

int32_t CAudio::SoundIsPlaying (int16_t nSound)
{
nSound = XlatSound (nSound);
for (int32_t i = 0; i < m_info.nMaxChannels; i++)
  if (audio.m_channels [i].Playing () && (audio.m_channels [i].Sound () == nSound)) 
		return 1;
return 0;
}

//------------------------------------------------------------------------------
//added on 980905 by adb to make sound nChannel setting work
void CAudio::SetMaxChannels (int32_t nChannels) 
{ 
if (!gameStates.app.bUseSound)
	return;
if (m_info.nMaxChannels	== nChannels)
	return;
if (m_info.bAvailable) 
	audio.StopAllChannels ();
m_info.nMaxChannels = (nChannels < 2) ? 2 : (nChannels > MAX_SOUND_CHANNELS) ? MAX_SOUND_CHANNELS : nChannels;
gameStates.sound.audio.nMaxChannels = m_info.nMaxChannels;
Init ();
}

//------------------------------------------------------------------------------

int32_t CAudio::GetMaxChannels (void) 
{ 
return m_info.nMaxChannels; 
}
// end edit by adb

//------------------------------------------------------------------------------

int32_t CAudio::ChannelIsPlaying (int32_t nChannel)
{
if (!m_info.bAvailable) 
	return 0;
if ((nChannel < 0) || (nChannel >= m_info.nMaxChannels))
	return 0;
#ifdef _WIN32
return audio.m_channels [nChannel].Playing ();
#else
bool b = audio.m_channels [nChannel].Playing ();
return b;
#endif
}

//------------------------------------------------------------------------------

void CAudio::SetVolume (int32_t nChannel, int32_t nVolume)
{
if (!gameStates.app.bUseSound)
	return;
if (!m_info.bAvailable) 
	return;
m_channels [nChannel].SetVolume (nVolume);
}

//------------------------------------------------------------------------------

void CAudio::SetPan (int32_t nChannel, int32_t nPan)
{
if (!gameStates.app.bUseSound)
	return;
if (!m_info.bAvailable) 
	return;
if ((nChannel < 0) || (nChannel >= m_info.nMaxChannels))
	return;
m_channels [nChannel].SetPan (nPan);
}

//------------------------------------------------------------------------------

void CAudio::StopSound (int32_t nChannel)
{
if (!gameStates.app.bUseSound)
	return;
if ((nChannel < 0) || (nChannel >= m_info.nMaxChannels))
	return;
m_channels [nChannel].Stop ();
}

//------------------------------------------------------------------------------

void CAudio::StopActiveSound (int32_t nChannel)
{
if (!gameStates.app.bUseSound)
	return;
if (!m_info.bAvailable)
	return;
if ((nChannel < 0) || (nChannel >= m_info.nMaxChannels))
	return;
if (!audio.m_channels [nChannel].Playing ())
	return;
audio.StopSound (nChannel);
}

//------------------------------------------------------------------------------

void CAudio::StopCurrentSong ()
{
if (songManager.Playing ()) {
	midi.Fadeout ();
#if USE_SDL_MIXER
	if (!gameOpts->sound.bUseSDLMixer)
#endif
	m_info.nMidiVolume = midi.SetVolume (0);
#if defined (_WIN32)
#	if USE_SDL_MIXER
if (!gameOpts->sound.bUseSDLMixer)
#	endif
	midi.Shutdown ();
#endif
	songManager.SetPlaying (0);
	}
}

//------------------------------------------------------------------------------

void CAudio::DestroyBuffers (void)
{
	CAudioChannel	*channelP;
	int32_t			i;

for (channelP = audio.m_channels.Buffer (), i = audio.m_channels.Length (); i; i--, channelP++)
	if (channelP->Resampled ())
		channelP->Destroy ();
}

//------------------------------------------------------------------------------
#if DBG
void CAudio::Debug (void)
{
	int32_t i;
	int32_t n_voices = 0;

if (!m_info.bAvailable)
	return;
for (i = 0; i < m_info.nMaxChannels; i++) {
	if (ChannelIsPlaying (i))
		n_voices++;
	}
}
#endif

//------------------------------------------------------------------------------
//eof
