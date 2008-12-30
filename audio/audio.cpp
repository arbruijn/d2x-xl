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

#include "digi.h"

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
# if USE_SDL_MIXER
#  include <SDL_mixer.h>
# endif
#endif

#include "inferno.h"
#include "u_mem.h"
#include "error.h"
#include "text.h"

#define SDL_MIXER_CHANNELS	2

//changed on 980905 by adb to increase number of concurrent sounds
#define MAX_SOUND_SLOTS 128
//end changes by adb
#define SOUND_BUFFER_SIZE 512

#define D2_SOUND_FORMAT	AUDIO_U8	//AUDIO_S16LSB

/* This table is used to add two sound values together and pin
 * the value to avoid overflow.  (used with permission from ARDI)
 * DPH: Taken from SDL/src/SDL_mixer.c.
 */
static const ubyte mix8[] =
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

//------------------------------------------------------------------------------

static SDL_AudioSpec waveSpec;

class CAudioChannel {
	public:
		int				nSound;
		fix				nPan;				// 0 = far left, 1 = far right
		fix				nVolume;			// 0 = nothing, 1 = fully on
		CByteArray		sample;
		uint				nLength;			// Length of the sample
		uint				nPosition;		// Position we are at at the moment.
		int				nSoundObj;		// Which soundobject is on this nChannel
		int				nSoundClass;
		ubyte				bPlaying;		// Is there a sample playing on this nChannel?
		ubyte				bLooped;			// Play this sample looped?
		ubyte				bPersistent;	// This can't be pre-empted
		ubyte				bResampled;
		ubyte				bBuiltIn;
	#if USE_SDL_MIXER
		Mix_Chunk*		mixChunkP;
		int				nChannel;
	#endif
	#if USE_OPENAL
		ALuint			source;
		int				loops;
	#endif
	};


class CAudioChannel {
	private:
		tChannelInfo	m_info;

	public:
		CAudioChannel () { Init (); }
		~CAudioChannel () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void SetVolume (int nVolume);
		void SetPan (int nPan);
		int Start (short nSound, int nSoundClass, fix nVolume, int nPan, int bLooping, 
					  int nLoopStart, int nLoopEnd, int nSoundObj, int nSpeed, 
					  const char *pszWAV, CFixVector* vPos);
		inline int IsPlaying (void) { return m_info.bPlaying; }
		inline int Sound (void) { return m_info.nSound; }

	private:
		int Resample (CDigiSound *soundP, int bD1Sound, int bMP3);
		int Speedup (CDigiSound *soundP, int speed);
		int ReadWAV (CAudioChannel *channelP);
	};

//------------------------------------------------------------------------------

class CAudioInfo {
	public:
		int		bInitialized;
		int		bAvailable;
		int		bSoundsInitialized;
		int		bLoMem;
		int		nMaxChannels;
		int		nFreeChannel;
		int		nVolume;
		int		nNextSignature;
		int		nActiveObjects;
		short		nLoopingSound;
		short		nLoopingVolume;
		short		nLoopingStart;
		short		nLoopingEnd;
		short		nLoopingChannel;
	};


class CAudio {
	private:
		CAudioInfo	m_info;

	public:
		CArray<CAudioChannel>	m_channels;

	public:
		CAudio () { Init (); }
		~CAudio () { Destroy (); }
		int Setup (float fSlowDown);
		void Shutdown (void);
		void Close (void);
		void Reset (void);
		void FadeoutMusic (void);
		void StopAllSounds (void);
		void StopSound (int nChannel);
		void StopActiveSound (int nChannel);
		void StopCurrentSong (void);
		void DestroyBuffers (void);
#if DBG
		void Debug (void);
#endif
		int SoundClassCount (int nSoundClass);
		void SetVolume (int nChannel, int nVolume);
		void SetPan (int nChannel, int nPan);
		int ChannelIsPlaying (int nChannel);
		int SoundIsPlaying (short nSound);
		void SetMaxChannels (int nChannels);
		int GetMaxChannels (void);
		void SetFxVolume (int nVolume);
		void SetVolumes (int fxVolume, int midiVolume);

		int StartSound (short nSound, int nSoundClass, fix nVolume, int nPan, int bLooping, 
							 int nLoopStart, int nLoopEnd, int nSoundObj, int nSpeed, 
							 const char *pszWAV, CFixVector* vPos);

		int PlaySample (short nSound, fix nVolume = F1_0, int nPan = F1_0 / 2 - 1, int bNoDups = 0, const char* pszWAV = NULL, CFixVector* vPos = NULL);

		void PlaySample3D (short nSound, int angle, int volume, int bNoDups, CFixVector *vPos, const char *pszSound); // Volume from 0-0x7fff
		int PlaySampleSpeed (short nSound, fix maxVolume, int nSpeed, int nLoops, const char *pszWAV, int nSoundClass);
		void PlaySampleOnce (short nSound, fix maxVolume);

		int SetObjectSound (short nSound, int nSoundClass, short nObject, int bForever = 0, fix maxVolume = F1_0, fix maxDistance = I2X (256),
								  int nLoopStart = -1, int nLoopEnd = -1, const char *pszSound = NULL, int nDecay = 0);
		int SetSegmentSound (short nSound, short nSegment, short nSide, CFixVector& vPos, int forever = 0, 
									fix maxVolume = F1_0, fix maxDistance = X2I (256), const char *pszSound = NULL);
		int PlayMidiSong (char * filename, char * melodic_bank, char * drum_bank, int loop, int bD1Song);
		void InitSounds (void);
		void SyncSounds (void);
		int KillSegmentSound (short nSegment, short nSide, short nSound);
		int KillObjectSound (int nObject);
		int ChangeObjectSound (int nObject, fix nVolume);
		void SetMidiVolume (int nVolume);
		void SetFxVolume (int nVolume);
		void MidiVolume (int fxVolume, int midiVolume);
		int GetSoundByName (const char *pszSound);
		int SetObjectSound (int nObject, int nSound, const char *pszSound, const fix xVolume = F1_0);

		int IsSoundPlaying (short nSound);

		void PauseAll ();
		void ResumeAll ();
		void PauseDigiSounds ();
		void ResumeDigiSounds ();
		void StopAll ();

		void SetMaxChannels (int n);
		int GetMaxChannels ();

		extern int digi_lomem;

		short XlatSound (short nSound);
		int UnXlatSound (int nSound);
		extern void audio.StopSound (int channel);

		static void _CDECL_ CAudio::MixCallback (void* userdata, ubyte* stream, int len);

	private:
		CAudioChannel* FindFreeChannel (int nSoundClass);
	};

CAudio audio;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void SDLCALL Mix_FinishChannel (int nChannel)
{
audio.m_channels [nChannel].bPlaying = 0;
}

//------------------------------------------------------------------------------

void Mix_VolPan (int nChannel, int nVolume, int nPan)
{
#if USE_SDL_MIXER
if (!m_info.bAvailable) 
	return;
if (gameOpts->sound.bUseSDLMixer && (nChannel >= 0)) {
	if (nVolume) {
		nVolume = (FixMul (nVolume, m_info.nVolume) + (SOUND_MAX_VOLUME / MIX_MAX_VOLUME) / 2) / (SOUND_MAX_VOLUME / MIX_MAX_VOLUME);
		if (!nVolume)
			nVolume = 1;
		Mix_Volume (nChannel, nVolume);
		if (nPan >= 0) {
			nPan /= (32767 / 127);
			Mix_SetPanning (nChannel, (ubyte) nPan, (ubyte) (254 - nPan));
			}
		}
	}
#endif
}

//------------------------------------------------------------------------------

#if 0//def _WIN32

static void MixSoundSlot (CAudioChannel *sl, ubyte* sldata, ubyte* stream, int len)
{
	ubyte* streamend = stream + len;
	ubyte* slend = sldata - sl->nPosition + sl->nLength;
	ubyte* sp = stream, s;
	signed char v;
	fix vl, vr;
	int x;

if ((x = sl->nPan) & 0x8000) {
	vl = 0x20000 - x * 2;
	vr = 0x10000;
	}
else {
	vl = 0x10000;
	vr = x * 2;
	}
vl = FixMul (vl, (x = sl->nVolume));
vr = FixMul (vr, x);
while (sp < streamend) {
	if (sldata == slend) {
		if (!sl->bLooped) {
			sl->bPlaying = 0;
			break;
			}
		sldata = sl->sample.Buffer ();
		}
	v = * (sldata++) - 0x80;
	s = *sp;
	* (sp++) = mix8 [s + FixMul (v, vl) + 0x80];
	s = *sp;
	* (sp++) = mix8 [s + FixMul (v, vr) + 0x80];
	}
sl->nPosition = (int) (sldata - sl->sample);
}

#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CAudioChannel::Init (void)
{
nSound = 0;
nPan = 0;				
nVolume = 0;			
nLength = 0;			
nPosition = 0;		
nSoundObj = 0;		
nSoundClass = 0;
bPlaying = 0;		
bLooped = 0;			
bPersistent = 0;	
bResampled = 0;
bBuiltIn = 0;
#if USE_SDL_MIXER
mixChunkP = NULL;
nChannel = 0;
#endif
#if USE_OPENAL
source = 0;
loops = 0;
#endif
}

//------------------------------------------------------------------------------

void CAudioChannel::Destroy (void)
{
sample.Destroy ();
}

//------------------------------------------------------------------------------

void CAudioChannel::SetVolume (int nVolume)
{
if (m_info.bPlaying) {
	m_info.nVolume = FixMulDiv (nVolume, m_info.nVolume, F1_0);
#if USE_SDL_MIXER
	if (gameOpts->sound.bUseSDLMixer)
		Mix_VolPan (m_nVolume, -1);
#endif
	}
}

//------------------------------------------------------------------------------

void CAudioChannel::SetPan (int nPan)
{
if (m_info.bPlaying) {
	m_info.nPan = nPan;
#if USE_SDL_MIXER
	if (gameOpts->sound.bUseSDLMixer) {
		nPan /= (32767 / 127);
		Mix_SetPanning (nChannel, (ubyte) nPan, (ubyte) (254 - nPan));
		}
#endif
	}
}

//------------------------------------------------------------------------------

void CAudioChannel::Stop (void)
{
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
if (m_info.bAvailable && gameOpts->sound.bUseSDLMixer) {
	if (m_info.mixChunkP) {
		Mix_HaltChannel (nChannel);
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
// resample to 16 bit stereo

int CAudioChannel::Resample (CDigiSound *soundP, int bD1Sound, int bMP3)
{
	int		h = 0, i, k, l;
	ushort	*ps, *ph;
	ubyte		*dataP = soundP->data [soundP->bDTX].Buffer ();

i = soundP->nLength [soundP->bDTX];
#if SDL_MIXER_CHANNELS == 2
l = 2 * i;
if (bD1Sound)
	l *= 2;
if (bMP3) 
	l = (l * 32) / 11;	//sample up to approx. 32 kHz
#else
if (bMP3) 
 {
	l = 2 * i;
	if (bD1Sound)
		l *= 2;
	l = (l * 32) / 11;	//sample up to approx. 32 kHz
	}
else if (bD1Sound)
	l = 2 * i;
else {
	m_info.sample.Destroy ();
	m_info.sample.SetBuffer (dataP);
	return i;
	}
#endif
#if D2_SOUND_FORMAT == AUDIO_S16LSB
else
	l *= 2;
#endif
if (!m_info.sample.Create (l))
	return -1;
m_info.bResampled = 1;
ph = reinterpret_cast<ushort*> (m_info.sample.Buffer ());
ps = reinterpret_cast<ushort*> (m_info.sample.Buffer () + l);
k = 0;
for (;;) {
	if (i) 
		h = dataP [--i];
	if (bMP3) { //get as close to 32.000 Hz as possible
		if (k < 700)
			h <<= k / 100;
		else if (i < 700)
			h <<= i / 100;
		else
			h = (h - 1) << 8;
		*(--ps) = (ushort) h;
		if (ps <= ph)
			break;
		*(--ps) = (ushort) h;
		if (ps <= ph)
			break;
		if (++k % 11) {
			*(--ps) = (ushort) h;
			if (ps <= ph)
				break;
			}
		}
	else {
#if D2_SOUND_FORMAT == AUDIO_S16LSB
		h = (((h + 1) << 7) - 1);
		*(--ps) = (ushort) h;
#else
		h |= h << 8;
#endif
		*(--ps) = (ushort) h;
		if (ps <= ph)
			break;
		}
	if (bD1Sound) {
		if (bMP3) {
			*(--ps) = (ushort) h;
			if (ps <= ph)
				break;
			*(--ps) = (ushort) h;
			if (ps <= ph)
				break;
			if (k % 11) {
				*(--ps) = (ushort) h;
				if (ps <= ph)
					break;
				}
			}
		else {
#if D2_SOUND_FORMAT == AUDIO_S16LSB
			*(--ps) = (ushort) h;
#endif
#if SDL_MIXER_CHANNELS == 2
			*(--ps) = (ushort) h;
			if (ps <= ph)
				break;
#endif
			}
		}
	}
Assert (ps == ph);
return m_info.nLength = l;
}

//------------------------------------------------------------------------------

int CAudioChannel::ReadWAV (void)
{
	CFile	cf;
	int	l;

if (!cf.Open ("d2x-temp.wav", gameFolders.szDataDir, "rb", 0))
	return 0;
if (0 >= (l = cf.Length ()))
	l = -1;
else if (!m_info.sample.Create (l))
	l = -1;
else if (cf.Read (m_info.sample.Buffer (), 1, l) != (size_t) l)
	l = -1;
cf.Shutdown ();
if ((l < 0) && m_info.sample.Buffer ()) {
	m_info.sample.Destroy ();
	}
return l > 0;
}

//------------------------------------------------------------------------------

int CAudioChannel::Speedup (CDigiSound *soundP, int speed)
{
	int	h, i, j, l;
	ubyte	*pDest, *pSrc;

l = FixMulDiv (m_info.bResampled ? m_info.nLength : soundP->nLength [soundP->bDTX], speed, F1_0);
if (!(pDest = new ubyte [l]))
	return -1;
pSrc = m_info.bResampled ? m_info.sample.Buffer () : soundP->data [soundP->bDTX].Buffer ();
for (h = i = j = 0; i < l; i++) {
	pDest [j] = pSrc [i];
	h += speed;
	while (h >= F1_0) {
		j++;
		h -= F1_0;
		}
	}
if (m_info.bResampled)
	m_info.sample.Destroy ();
else
	m_info.bResampled = 1;
m_info.sample.SetBuffer (pDest, false, j);
return m_info.nLength = j;
}

//------------------------------------------------------------------------------

#if USE_OPENAL

inline int CAudio::ALError (void)
{
return alcGetError (gameData.pig.sound.openAL.device) != AL_NO_ERROR;
}

#endif

//------------------------------------------------------------------------------

// Volume 0-F1_0
int CAudioChannel::Start (short nSound, int nSoundClass, fix nVolume, int nPan, int bLooping, 
								  int nLoopStart, int nLoopEnd, int nSoundObj, int nSpeed, 
								  const char *pszWAV, CFixVector* vPos)
{
	CDigiSound*		soundP = NULL;
	int				i, bPersistent = (nSoundObj > -1) || bLooping || (nVolume > F1_0);

if (!(pszWAV && *pszWAV && gameOpts->sound.bUseSDLMixer)) {
	if (nSound < 0)
		return -1;
	soundP = gameData.pig.sound.sounds [gameStates.sound.bD1Sound] + nSound % gameData.pig.sound.nSoundFiles [gameStates.sound.bD1Sound];
	if (!(soundP->data && soundP->nLength))
		return -1;
	}
if (m_info.bPlaying) {
	m_info.bPlaying = 0;
	if (m_info.nSoundObj > -1)
		DigiEndSoundObj (m_info.nSoundObj);
	if (soundQueue.nChannel == m_info.nFreeChannel)
		SoundQEnd ();
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
	alSourcef (m_info.source, AL_GAIN, ((nVolume < F1_0) ? X2F (nVolume) : 1) * 2 * X2F (m_info.nVolume));
	alSourcei (m_info.source, AL_LOOPING, (ALuint) ((nSoundObj > -1) || bLooping || (nVolume > F1_0)));
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
if (m_info.mixChunkP) {
	Mix_HaltChannel (m_info.nChannel);
	if (m_info.bBuiltIn)
		m_info.bBuiltIn = 0;
	else
		Mix_FreeChunk (m_info.mixChunkP);
	m_info.mixChunkP = NULL;
	}
#endif
#if DBG
VerifySoundChannelFree (m_info.nFreeChannel);
#endif
if (m_info.bResampled) {
	m_info.sample.Destroy ();
	m_info.bResampled = 0;
	}
#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer) {
	//resample to two channels
	m_info.nChannel = m_info.nFreeChannel;
	if (pszWAV && *pszWAV) {
		if (!(m_info.mixChunkP = LoadAddonSound (pszWAV, &m_info.bBuiltIn)))
			return -1;
		}
	else {
		int l;
		if (soundP->bHires) {
			l = soundP->nLength [0];
			m_info.sample.SetBuffer (soundP->data [0].Buffer (), true, l);
			}
		else {
			if (gameOpts->sound.bHires)
				return -1;	//cannot mix hires and standard sounds
			l = DigiResampleSound (soundP, channelP, gameStates.sound.bD1Sound && (gameOpts->sound.digiSampleRate != SAMPLE_RATE_11K), songManager.MP3 ());
			if (l <= 0)
				return -1;
			if (nSpeed < F1_0)
				l = DigiSpeedupSound (soundP, channelP, nSpeed);
			}
		m_info.mixChunkP = Mix_QuickLoad_RAW (m_info.sample.Buffer (), l);
		}
	Mix_VolPan (m_info.nFreeChannel, nVolume, nPan);
	Mix_PlayChannel (m_info.nFreeChannel, m_info.mixChunkP, bLooping ? -1 : nLoopEnd - nLoopStart);
	}
else 
#else
if (pszWAV && *pszWAV)
	return -1;
#endif
 {
	if (gameStates.sound.bD1Sound && (gameOpts->sound.digiSampleRate != SAMPLE_RATE_11K)) {
		int l = DigiResampleSound (soundP, channelP, 0, 0);
		if (l <= 0)
			return -1;
		m_info.nLength = l;
		}
	else {
		m_info.sample.SetBuffer (soundP->data [soundP->bDTX].Buffer (), true, m_info.nLength = soundP->nLength [soundP->bDTX]);
		}
	if (nSpeed < F1_0)
		DigiSpeedupSound (soundP, channelP, nSpeed);
	}
m_info.nVolume = FixMul (m_info.nVolume, nVolume);
m_info.nPan = nPan;
m_info.nPosition = 0;
m_info.nSoundObj = nSoundObj;
m_info.nSoundClass = nSoundClass;
m_info.bLooped = bLooping;
#if USE_OPENAL
m_info.loops = bLooping ? -1 : nLoopEnd - nLoopStart + 1;
#endif
m_info.nSound = nSound;
m_info.bPersistent = 0;
m_info.bPlaying = 1;
m_info.bPersistent = bPersistent;
i = m_info.nFreeChannel;
if (++m_info.nFreeChannel >= m_info.nMaxChannels)
	m_info.nFreeChannel = 0;
return i;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CAudio::Init (void)
{
memset (&m_info, 0, sizeof (m_info));
m_channels.Create (MAX_SOUND_SLOTS);
}

//------------------------------------------------------------------------------

void CAudio::Destroy (void)
{
Close ();
m_channels.Destroy ();
}

//------------------------------------------------------------------------------
/* Initialise audio devices. */
int CAudio::Setup (float fSlowDown)
{
if (!gameStates.app.bUseSound)
	return 1;
if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0) {
	PrintLog (TXT_SDL_INIT_AUDIO, SDL_GetError ()); PrintLog ("\n");
	Error (TXT_SDL_INIT_AUDIO, SDL_GetError ());
	return 1;
	}
audio.m_channels.Clear ();
if (gameStates.app.bDemoData)
	gameOpts->sound.digiSampleRate = SAMPLE_RATE_11K;
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
	int h;
	if (fSlowDown <= 0)
		fSlowDown = 1.0f;
	if (gameOpts->sound.bHires == 1)
		h = Mix_OpenAudio ((int) (SAMPLE_RATE_22K / fSlowDown), AUDIO_S16LSB, 2, SOUND_BUFFER_SIZE);
	else if (gameOpts->sound.bHires == 2)
		h = Mix_OpenAudio ((int) (SAMPLE_RATE_44K / fSlowDown), AUDIO_S16LSB, 2, SOUND_BUFFER_SIZE);
	else if (songManager.MP3 ())
		h = Mix_OpenAudio (32000, AUDIO_S16LSB, 2, SOUND_BUFFER_SIZE * 10);
	else 
		h = Mix_OpenAudio ((int) (gameOpts->sound.digiSampleRate / fSlowDown), D2_SOUND_FORMAT, SDL_MIXER_CHANNELS, 
								SOUND_BUFFER_SIZE);
	if (h < 0) {
		PrintLog (TXT_SDL_OPEN_AUDIO, SDL_GetError ()); PrintLog ("\n");
		Warning (TXT_SDL_OPEN_AUDIO, SDL_GetError ());
		return 1;
		}
	Mix_Resume (-1);
	Mix_ResumeMusic ();
	Mix_AllocateChannels (MAX_SOUND_SLOTS);
	Mix_ChannelFinished (Mix_FinishChannel);
	}
else 
#endif
 {
	waveSpec.freq = (int) (gameOpts->sound.digiSampleRate / fSlowDown);
	//added/changed by Sam Lantinga on 12/01/98 for new SDL version
	waveSpec.format = AUDIO_U8;
	waveSpec.channels = 2;
	//end this section addition/change - SL
	waveSpec.samples = SOUND_BUFFER_SIZE * (gameOpts->sound.digiSampleRate / SAMPLE_RATE_11K);
	waveSpec.callback = AudioMixCallback;
	if (SDL_OpenAudio (&waveSpec, NULL) < 0) {
		PrintLog (TXT_SDL_OPEN_AUDIO, SDL_GetError ()); PrintLog ("\n");
		Warning (TXT_SDL_OPEN_AUDIO, SDL_GetError ());
		return 1;
		}
	SDL_PauseAudio (0);
	}
m_info.bInitialized =
m_info.bAvailable = 1;
return 0;
}

//------------------------------------------------------------------------------
/* Shut down audio */
void CAudio::Close (void)
{
if (!m_info.bInitialized) 
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
#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer) {
	Mix_CloseAudio ();
	}
else
#endif
	SDL_CloseAudio ();
}

//------------------------------------------------------------------------------

void CAudio::FadeoutMusic (void)
{
#if USE_SDL_MIXER
extern Mix_Music *mixMusic;
if (!m_info.bAvailable) 
	return;
if (gameOpts->sound.bUseSDLMixer) {
	if (gameOpts->sound.bFadeMusic) {
		while (!Mix_FadeOutMusic (250) && Mix_PlayingMusic ())
			SDL_Delay (50);		
		while (Mix_PlayingMusic ())
			SDL_Delay (1);		
		}
	Mix_HaltMusic ();
	Mix_FreeMusic (mixMusic);
	mixMusic = NULL;
	}
#endif
}

//------------------------------------------------------------------------------

void CAudio::Shutdown (void)
{
if (m_info.bAvailable) {
	DigiStopAll ();
	songManager.StopAll ();
	m_info.bAvailable = 0;
	Close ();
	}
}

//------------------------------------------------------------------------------
/* Toggle audio */
void CAudio::Reset (void) 
{
#if !USE_OPENAL
audio.Shutdown ();
audio.Setup (1);
#endif
}

//------------------------------------------------------------------------------

void CAudio::StopAllSounds (void)
{
	int i;

for (i = 0; i < MAX_SOUND_SLOTS; i++)
	audio.StopSound (i);
gameData.multiplayer.bMoving = -1;
gameData.weapons.firing [0].bSound = 0;
}

//------------------------------------------------------------------------------

void DigiEndSoundObj (int nChannel);
void SoundQEnd ();
int VerifySoundChannelFree (int nChannel);

//------------------------------------------------------------------------------

int CAudio::SoundClassCount (int nSoundClass)
{
	CAudioChannel	*channelP;
	int			h, i;

for (h = 0, i = m_info.nMaxChannels, channelP = audio.m_channels; i; i--, channelP++)
	if (channelP->bPlaying && (channelP->nSoundClass == nSoundClass))
		h++;
return h;
}

//------------------------------------------------------------------------------

CAudioChannel* CAudio::FindFreeChannel (int nSoundClass)
{
	CAudioChannel*	channelP, * channelMinVolP [2] = {NULL, NULL};
	int				nStartChannel;
	int				bUseClass = SoundClassCount (nSoundClass) >= m_info.nMaxChannels / 2;

nStartChannel = m_info.nFreeChannel;
do {
	channelP = audio.m_channels + m_info.nFreeChannel;
	if (!channelP->bPlaying)
		return channelP;
	if ((!bUseClass || (channelP->nSoundClass == nSoundClass)) &&
	    (!channelMinVolP [channelP->bPersistent] || (channelMinVolP [channelP->bPersistent]->nVolume > channelP->nVolume)))
		channelMinVolP [channelP->bPersistent] = channelP;
	m_info.nFreeChannel = (m_info.nFreeChannel + 1) % m_info.nMaxChannels;
	} while (m_info.nFreeChannel != nStartChannel);
return channelMinVolP [0] ? channelMinVolP [0] : channelMinVolP [1];
}

//------------------------------------------------------------------------------

// Volume 0-F1_0
int CAudio::StartSound (short nSound, int nSoundClass, fix nVolume, int nPan, int bLooping, 
								int nLoopStart, int nLoopEnd, int nSoundObj, int nSpeed, 
								const char *pszWAV, CFixVector* vPos)
{
	CAudioChannel*	channelP;

if (!gameStates.app.bUseSound)
	return -1;
if (!m_info.bAvailable) 
	return -1;
if (bPersistent && !nSoundClass)
	nSoundClass = -1;
if (!(channelP = FindFreeChannel (nSoundClass)))
	return -1;
if (0 > channelP->Start (nSound, nSoundClass, nVolume, nPan, bLooping, nLoopStart, nLoopEnd, nSoundObj, nSpeed, pszWAV, vPos))
	return -1;
int i = m_info.nFreeChannel;
if (++m_info.nFreeChannel >= m_info.nMaxChannels)
	m_info.nFreeChannel = 0;
return i;
}

//------------------------------------------------------------------------------
// Returns the nChannel a sound number is bPlaying on, or
// -1 if none.
int CAudio::FindChannel (short nSound)
{
if (!gameStates.app.bUseSound)
	return -1;
if (!m_info.bAvailable) 
	return -1;
if (nSound < 0)
	return -1;
if (gameData.pig.sound.sounds [gameStates.sound.bD1Sound][nSound].data == NULL) {
	Int3 ();
	return -1;
	}
//FIXME: not implemented
return -1;
}

//------------------------------------------------------------------------------
//added on 980905 by adb from original source to make sfx nVolume work
void CAudio::SetFxVolume (int nVolume)
{
if (!gameStates.app.bUseSound)
	return;
nVolume = FixMulDiv (nVolume, SOUND_MAX_VOLUME, 0x7fff);
if (nVolume > SOUND_MAX_VOLUME)
	m_info.nVolume = SOUND_MAX_VOLUME;
else if (nVolume < 0)
	m_info.nVolume = 0;
else
	m_info.nVolume = nVolume;
if (!m_info.bAvailable) 
	return;
SyncSounds ();
}
//end edit by adb
//------------------------------------------------------------------------------

void CAudio::SetVolumes (int fxVolume, int midiVolume)
{
if (!gameStates.app.bUseSound)
	return;
SetFxVolume (fxVolume);
SetMidiVolume (midiVolume);
}

//------------------------------------------------------------------------------

int CAudio::SoundIsPlaying (short nSound)
{
	int i;

nSound = XlatSound (nSound);
for (i = 0; i < MAX_SOUND_SLOTS; i++)
  //changed on 980905 by adb: added audio.m_channels[i].bPlaying &&
  if (audio.m_channels [i].Playing () && (audio.m_channels [i].Sound () == nSound))
  //end changes by adb
		return 1;
return 0;
}

//------------------------------------------------------------------------------
//added on 980905 by adb to make sound nChannel setting work
void CAudio::SetMaxChannels (int nChannels) 
{ 
if (!gameStates.app.bUseSound)
	return;
m_info.nMaxChannels	= nChannels;
if (m_info.nMaxChannels < 1) 
	m_info.nMaxChannels = 1;
if (m_info.nMaxChannels > MAX_SOUND_SLOTS) 
	m_info.nMaxChannels = MAX_SOUND_SLOTS;
if (!m_info.bAvailable) 
	return;
audio.StopAllSounds ();
}

//------------------------------------------------------------------------------

int CAudio::GetMaxChannels (void) 
{ 
return m_info.nMaxChannels; 
}
// end edit by adb

//------------------------------------------------------------------------------

int CAudio::ChannelIsPlaying (int nChannel)
{
if (!m_info.bAvailable) 
	return 0;
return audio.m_channels [nChannel].IsPlaying ();
}

//------------------------------------------------------------------------------

void CAudio::SetVolume (int nChannel, int nVolume)
{
if (!gameStates.app.bUseSound)
	return;
if (!m_info.bAvailable) 
	return;
m_channels [nChannel].SetVolume (nVolume);
}

//------------------------------------------------------------------------------

void CAudio::SetPan (int nChannel, int nPan)
{
if (!gameStates.app.bUseSound)
	return;
if (!m_info.bAvailable) 
	return;
m_channels [nChannel].SetPan (nPan);
}

//------------------------------------------------------------------------------

void CAudio::StopSound (int nChannel)
{
if (!gameStates.app.bUseSound)
	return;
m_channels [nChannel].Stop ();
}

//------------------------------------------------------------------------------

void CAudio::StopActiveSound (int nChannel)
{
if (!gameStates.app.bUseSound)
	return;
if (!m_info.bAvailable)
	return;
if (!audio.m_channels [nChannel].bPlaying)
	return;
audio.StopSound (nChannel);
}

//------------------------------------------------------------------------------

#if !(defined (_WIN32) || USE_SDL_MIXER)
// MIDI stuff follows.
void DigiSetMidiVolume (int mvolume) { }

int DigiPlayMidiSong (char * filename, char * melodic_bank, char * drum_bank, int loop, int bD1Song) 
{
return 0;
}

//------------------------------------------------------------------------------

void CAudio::StopCurrentSong (void)
{
#ifdef HMIPLAY
	char buf [10];
    
   sprintf (buf, "s");
   send_ipc (buf);
#endif
}

void DigiPauseMidi () {}
void DigiResumeMidi () {}
#endif

//------------------------------------------------------------------------------

void CAudio::DestroyBuffers (void)
{
	CAudioChannel	*channelP;
	int			i;

for (channelP = audio.m_channels, i = sizeofa (audio.m_channels); i; i--, channelP++)
	if (channelP->bResampled) {
		channelP->sample.Destroy ();
		channelP->bResampled = 0;
		}
}

//------------------------------------------------------------------------------
#if DBG
void CAudio::Debug (void)
{
	int i;
	int n_voices = 0;

if (!m_info.bAvailable)
	return;
for (i = 0; i < m_info.nMaxChannels; i++) {
	if (DigiIsChannelPlaying (i))
		n_voices++;
	}
}
#endif

//------------------------------------------------------------------------------
/* Audio mixing callback */
//changed on 980905 by adb to cleanup, add nPan support and optimize mixer
static void _CDECL_ CAudio::MixCallback (void* userdata, ubyte* stream, int len)
{
	ubyte*			streamend = stream + len;
	CAudioChannel*	sl;

if (!m_info.bAvailable)
	return;
memset (stream, 0x80, len); // fix "static" sound bug on Mac OS X
for (sl = audio.m_channels; sl < audio.m_channels + MAX_SOUND_SLOTS; sl++) {
	if (sl->bPlaying && sl->sample.Buffer () && sl->nLength) {
#if 0
		MixSoundSlot (sl, sl->sample + sl->nPosition, stream, len);
#else
		ubyte* sldata = reinterpret_cast<ubyte*> (sl->sample + sl->nPosition), 
				*slend = reinterpret_cast<ubyte*> (sl->sample + sl->nLength);
		ubyte* sp = stream, s;
		signed char v;
		fix vl, vr;
		int x;

		if ((x = sl->nPan) & 0x8000) {
			vl = 0x20000 - x * 2;
			vr = 0x10000;
			}
		else {
			vl = 0x10000;
			vr = x * 2;
			}
		vl = FixMul (vl, (x = sl->nVolume));
		vr = FixMul (vr, x);
		while (sp < streamend) {
			if (sldata == slend) {
				if (!sl->bLooped) {
					sl->bPlaying = 0;
					break;
					}
				sldata = sl->sample.Buffer ();
				}
			v = *(sldata++) - 0x80;
			s = *sp;
			*(sp++) = mix8 [s + FixMul (v, vl) + 0x80];
			s = *sp;
			*(sp++) = mix8 [s + FixMul (v, vr) + 0x80];
			}
		sl->nPosition = (int) (sldata - sl->sample);
#endif
		}
	}
}
//end changes by adb

//------------------------------------------------------------------------------
//eof
