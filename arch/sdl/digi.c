/* $Id: digi.c,v 1.21 2004/11/29 08:01:47 btb Exp $ */
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
#include "mono.h"
#include "fix.h"
#include "vecmat.h"
#include "gr.h" // needed for piggy.h
#include "piggy.h"
#include "sounds.h"
#include "wall.h"
#include "newdemo.h"
#include "kconfig.h"
#include "inferno.h"
#include "text.h"

#define SDL_MIXER_CHANNELS	2

//edited 05/17/99 Matt Mueller - added ifndef NO_ASM
//added on 980905 by adb to add inline FixMul for mixer on i386
#ifndef NO_ASM
#ifdef __i386__
#define do_fixmul (x,y)				\
 ({						\
	int _ax, _dx;				\
	asm ("imull %2\n\tshrdl %3,%1,%0"	\
	    : "=a" (_ax), "=d" (_dx)		\
	    : "rm" (y), "i" (16), "0" (x));	\
	_ax;					\
})
extern inline fix FixMul (fix x, fix y) { return do_fixmul (x,y); }
#endif
#endif
//end edit by adb
//end edit -MM

//changed on 980905 by adb to increase number of concurrent sounds
#define MAX_SOUND_SLOTS 32
//end changes by adb
#define SOUND_BUFFER_SIZE 512

#define MIN_VOLUME 10
#define D2_SOUND_FORMAT	AUDIO_U8	//AUDIO_S16LSB


/* This table is used to add two sound values together and pin
 * the value to avoid overflow.  (used with permission from ARDI)
 * DPH: Taken from SDL/src/SDL_mixer.c.
 */
static const Uint8 mix8[] =
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


//added/changed on 980905 by adb to make sfx volume work, on 990221 by adb changed F1_0 to F1_0 / 2
#define SOUND_MAX_VOLUME (F1_0 / 2)

//end edit by adb

static SDL_AudioSpec WaveSpec;

typedef struct tSoundSlot {
	int				soundno;
	int				playing;		// Is there a sample playing on this channel?
	int				looped;		// Play this sample looped?
	fix				pan;			// 0 = far left, 1 = far right
	fix				volume;		// 0 = nothing, 1 = fully on
	ubyte				*samples;
	unsigned int	length;		// Length of the sample
	unsigned int	position;	// Position we are at at the moment.
	int				soundobj;   // Which soundobject is on this channel
	int				persistent; // This can't be pre-empted
#if USE_SDL_MIXER
	Mix_Chunk		*mixChunk;
	int				channel;
#endif
#if USE_OPENAL
	ALuint			source;
	int				loops;
#endif
	ubyte				bResampled;
} tSoundSlot;

tSoundSlot	SoundSlots [MAX_SOUND_SLOTS];

//------------------------------------------------------------------------------

void SDLCALL Mix_FinishChannel (int nChannel)
{
SoundSlots [nChannel].playing = 0;
}

//------------------------------------------------------------------------------

#ifdef _WIN32

static void MixSoundSlot (tSoundSlot *sl, Uint8 *sldata, Uint8 *stream, int len)
{
	Uint8 *streamend = stream + len;
	Uint8 *slend = sldata - sl->position + sl->length;
	Uint8 *sp = stream, s;
	signed char v;
	fix vl, vr;
	int x;

if ((x = sl->pan) & 0x8000) {
	vl = 0x20000 - x * 2;
	vr = 0x10000;
	}
else {
	vl = 0x10000;
	vr = x * 2;
	}
vl = FixMul (vl, (x = sl->volume));
vr = FixMul (vr, x);
while (sp < streamend) {
	if (sldata == slend) {
		if (!sl->looped) {
			sl->playing = 0;
			break;
			}
		sldata = sl->samples;
		}
	v = * (sldata++) - 0x80;
	s = *sp;
	* (sp++) = mix8 [s + FixMul (v, vl) + 0x80];
	s = *sp;
	* (sp++) = mix8 [s + FixMul (v, vr) + 0x80];
	}
sl->position = (int) (sldata - sl->samples);
}

#endif

//------------------------------------------------------------------------------
/* Audio mixing callback */
//changed on 980905 by adb to cleanup, add pan support and optimize mixer
static void _CDECL_ AudioMixCallback (void *userdata, Uint8 *stream, int len)
{
	Uint8 *streamend = stream + len;
	tSoundSlot *sl;

if (!gameStates.sound.digi.bInitialized)
	return;
memset (stream, 0x80, len); // fix "static" sound bug on Mac OS X
for (sl = SoundSlots; sl < SoundSlots + MAX_SOUND_SLOTS; sl++) {
	if (sl->playing && sl->samples && sl->length) {
#if 0
		MixSoundSlot (sl, sl->samples + sl->position, stream, len);
#else
		Uint8 *sldata = sl->samples + sl->position, *slend = sl->samples + sl->length;
		Uint8 *sp = stream, s;
		signed char v;
		fix vl, vr;
		int x;

		if ((x = sl->pan) & 0x8000) {
			vl = 0x20000 - x * 2;
			vr = 0x10000;
			}
		else {
			vl = 0x10000;
			vr = x * 2;
			}
		vl = FixMul (vl, (x = sl->volume));
		vr = FixMul (vr, x);
		while (sp < streamend) {
			if (sldata == slend) {
				if (!sl->looped) {
					sl->playing = 0;
					break;
					}
				sldata = sl->samples;
				}
			v = *(sldata++) - 0x80;
			s = *sp;
			*(sp++) = mix8 [s + FixMul (v, vl) + 0x80];
			s = *sp;
			*(sp++) = mix8 [s + FixMul (v, vr) + 0x80];
			}
		sl->position = (int) (sldata - sl->samples);
#endif
		}
	}
}
//end changes by adb

//------------------------------------------------------------------------------
/* Initialise audio devices. */
int DigiInit (float fSlowDown)
{
if (!gameStates.app.bUseSound)
	return 1;
if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0) {
	LogErr (TXT_SDL_INIT_AUDIO, SDL_GetError ()); LogErr ("\n");
	Error (TXT_SDL_INIT_AUDIO, SDL_GetError ());
	return 1;
	}
memset (SoundSlots, 0, sizeof (SoundSlots));	
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
	if (gameOpts->sound.bHires == 1)
		h = Mix_OpenAudio ((int) (SAMPLE_RATE_22K / fSlowDown), AUDIO_S16LSB, 2, SOUND_BUFFER_SIZE * 8);
	else if (gameOpts->sound.bHires == 2)
		h = Mix_OpenAudio ((int) (SAMPLE_RATE_44K / fSlowDown), AUDIO_S16LSB, 2, SOUND_BUFFER_SIZE * 16);
	else if (gameData.songs.user.bMP3)
		h = Mix_OpenAudio (32000, AUDIO_S16LSB, 2, SOUND_BUFFER_SIZE * 10);
	else 
		h = Mix_OpenAudio ((int) (gameOpts->sound.digiSampleRate / fSlowDown), D2_SOUND_FORMAT, SDL_MIXER_CHANNELS, 
								 2 * SOUND_BUFFER_SIZE * (gameOpts->sound.digiSampleRate / SAMPLE_RATE_11K));
	if (h < 0) {
		LogErr (TXT_SDL_OPEN_AUDIO, SDL_GetError ()); LogErr ("\n");
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
	WaveSpec.freq = (int) (gameOpts->sound.digiSampleRate / fSlowDown);
	//added/changed by Sam Lantinga on 12/01/98 for new SDL version
	WaveSpec.format = AUDIO_U8;
	WaveSpec.channels = 2;
	//end this section addition/change - SL
	WaveSpec.samples = SOUND_BUFFER_SIZE * (gameOpts->sound.digiSampleRate / SAMPLE_RATE_11K);
	WaveSpec.callback = AudioMixCallback;
	if (SDL_OpenAudio (&WaveSpec, NULL) < 0) {
		LogErr (TXT_SDL_OPEN_AUDIO, SDL_GetError ()); LogErr ("\n");
		Warning (TXT_SDL_OPEN_AUDIO, SDL_GetError ());
		return 1;
		}
	SDL_PauseAudio (0);
	}
//atexit (DigiClose);
gameStates.sound.digi.bInitialized = 1;
return 0;
}

//------------------------------------------------------------------------------
/* Shut down audio */
void _CDECL_ DigiClose (void)
{
if (!gameStates.sound.digi.bInitialized) 
	return;
gameStates.sound.digi.bInitialized = 0;
#if defined (__MINGW32__) || defined (__macosx__)
SDL_Delay (500); // CloseAudio hangs if it's called too soon after opening?
#endif
#if USE_OPENAL
if (gameOpts->sound.bUseOpenAL) {
	alcMakeContextCurrent (NULL);
	alcDestroyContext (gameData.pig.sound.openAL.context);
	alcCloseDevice(gameData.pig.sound.openAL.device);
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

void DigiExit (void)
{
DigiStopAll ();
SongsStopAll ();
DigiClose ();
}

//------------------------------------------------------------------------------
/* Toggle audio */
void DigiReset () 
{
#if !USE_OPENAL
DigiExit ();
DigiInit (1);
#endif
}

//------------------------------------------------------------------------------

void DigiStopAllChannels ()
{
	int i;

for (i = 0; i < MAX_SOUND_SLOTS; i++)
	DigiStopSound (i);
}

//------------------------------------------------------------------------------
// resample to 16 bit stereo

int DigiResampleSound (tDigiSound *dsP, tSoundSlot *ssP, int bD1Sound, int bMP3)
{
	int		h = 0, i, k, l;
	ushort	*ps, *ph;
	ubyte		*dataP = dsP->data [dsP->bDTX];

i = dsP->nLength [dsP->bDTX];
#if SDL_MIXER_CHANNELS == 2
l = 2 * i;
if (bD1Sound)
	l *= 2;
if (bMP3) 
	l = l * 32 / 11;	//sample up to approx. 32 kHz
#else
if (bMP3) 
	{
	l = 2 * i;
	if (bD1Sound)
		l *= 2;
	l = l * 32 / 11;	//sample up to approx. 32 kHz
	}
else if (bD1Sound)
	l = 2 * i;
else {
	ssP->samples = dataP;
	return i;
	}
#endif
#if D2_SOUND_FORMAT == AUDIO_S16LSB
else
	l *= 2;
#endif
if (!(ssP->samples = (ubyte *) D2_ALLOC (l)))
	return -1;
ssP->bResampled = 1;
ph = (ushort *) ssP->samples;
ps = (ushort *) (ssP->samples + l);
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
return ssP->length = l;
}

//------------------------------------------------------------------------------

int DigiReadWAV (tSoundSlot *ssP)
{
	CFILE	*fp;
	int	l;

if (!(fp = CFOpen ("d2x-temp.wav", gameFolders.szDataDir, "rb", 0)))
	return 0;
if (0 >= (l = CFLength (fp, 0)))
	l = -1;
else if (!(ssP->samples = (ubyte *) D2_ALLOC (l)))
	l = -1;
else if (CFRead (ssP->samples, 1, l, fp) != (size_t) l)
	l = -1;
CFClose (fp);
if ((l < 0) && ssP->samples) {
	D2_FREE (ssP->samples);
	ssP->samples = NULL;
	}
return l > 0;
}

//------------------------------------------------------------------------------

int DigiSpeedupSound (tDigiSound *dsP, tSoundSlot *ssP, int speed)
{
	int	h, i, j, l;
	ubyte	*pDest, *pSrc;

l = FixMulDiv (ssP->bResampled ? ssP->length : dsP->nLength [dsP->bDTX], speed, F1_0);
if (!(pDest = (ubyte *) D2_ALLOC (l)))
	return -1;
pSrc = ssP->bResampled ? ssP->samples : dsP->data [dsP->bDTX];
for (h = i = j = 0; i < l; i++) {
	pDest [j] = pSrc [i];
	h += speed;
	while (h >= F1_0) {
		j++;
		h -= F1_0;
		}
	}
if (ssP->bResampled)
	{D2_FREE (ssP->samples);}
else
	ssP->bResampled = 1;
ssP->samples = pDest;
return ssP->length = j;
}

//------------------------------------------------------------------------------

void Mix_VolPan (int channel, int vol, int pan)
{
#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer && (channel >= 0)) {
	if (vol) {
		vol = (FixMul (vol, gameStates.sound.digi.nVolume) + (SOUND_MAX_VOLUME / MIX_MAX_VOLUME) / 2) / (SOUND_MAX_VOLUME / MIX_MAX_VOLUME);
		if (!vol)
			vol = 1;
		Mix_Volume (channel, vol);
		pan /= (32767 / 127);
		Mix_SetPanning (channel, (ubyte) pan, (ubyte) (254 - pan));
		}
	}
#endif
}

//------------------------------------------------------------------------------

#if USE_OPENAL

inline int DigiALError (void)
{
return alcGetError (gameData.pig.sound.openAL.device) != AL_NO_ERROR;
}

#endif

//------------------------------------------------------------------------------

extern void DigiEndSoundObj (int channel);	
extern void SoundQEnd ();

int VerifySoundChannelFree (int channel);

// Volume 0-F1_0
int DigiStartSound (short nSound, fix volume, int pan, int looping, 
						  int loop_start, int loop_end, int soundobj, int speed, 
						  char *pszWAV, vmsVector *vPos)
{
	int			i, l, nStartChannel;
	tSoundSlot	*ssP;
	tDigiSound	*dsP = NULL;

if (!gameStates.app.bUseSound)
	return -1;
if (!gameStates.sound.digi.bInitialized) 
	return -1;
if (!(pszWAV && *pszWAV && gameOpts->sound.bUseSDLMixer)) {
	if (nSound < 0)
		return -1;
	dsP = gameData.pig.sound.sounds [gameOpts->sound.bD1Sound] + nSound % gameData.pig.sound.nSoundFiles [gameOpts->sound.bD1Sound];
	if (!(dsP->data && dsP->nLength))
		return -1;
	Assert (dsP->data != (void *) -1);
	}
nStartChannel = gameStates.sound.digi.nNextChannel;
for(;;) {
	if (!SoundSlots [gameStates.sound.digi.nNextChannel].playing)
		break;
	if (!SoundSlots [gameStates.sound.digi.nNextChannel].persistent)
		break;	// use this channel!	
	gameStates.sound.digi.nNextChannel++;
	if (gameStates.sound.digi.nNextChannel >= gameStates.sound.digi.nMaxChannels)
		gameStates.sound.digi.nNextChannel = 0;
	if (gameStates.sound.digi.nNextChannel == nStartChannel) {
		//mprintf ((1, "OUT OF SOUND CHANNELS!!!\n"));
		return -1;
		}
	}
ssP = SoundSlots + gameStates.sound.digi.nNextChannel;
if (ssP->playing) {
	ssP->playing = 0;
	if (ssP->soundobj > -1)
		DigiEndSoundObj (ssP->soundobj);
	if (soundQueue.nChannel == gameStates.sound.digi.nNextChannel)
		SoundQEnd ();
	}
#if USE_OPENAL
if (ssP->source == 0xFFFFFFFF) {
	fVector	fPos;

	DigiALError ();
	alGenSources (1, &ssP->source);
	if (DigiALError ())
		return -1;
	alSourcei (ssP->source, AL_BUFFER, dsP->buffer);
	if (DigiALError ())
		return -1;
	alSourcef (ssP->source, AL_GAIN, ((volume < F1_0) ? f2fl (volume) : 1) * 2 * f2fl (gameStates.sound.digi.nVolume));
	alSourcei (ssP->source, AL_LOOPING, (ALuint) ((soundobj > -1) || looping || (volume > F1_0)));
	alSourcefv (ssP->source, AL_POSITION, (ALfloat*) VmsVecToFloat (&fPos, vPos ? vPos : &gameData.objs.objects [LOCALPLAYER.nObject].position.vPos));
	alSource3f (ssP->source, AL_VELOCITY, 0, 0, 0);
	alSource3f (ssP->source, AL_DIRECTION, 0, 0, 0);
	if (DigiALError ())
		return -1;
	alSourcePlay (ssP->source);
	if (DigiALError ())
		return -1;
	}
#endif
#if USE_SDL_MIXER
if (ssP->mixChunk) {
	Mix_HaltChannel (ssP->channel);
	Mix_FreeChunk (ssP->mixChunk);
	ssP->mixChunk = NULL;
	}
#endif
#ifdef _DEBUG
VerifySoundChannelFree (gameStates.sound.digi.nNextChannel);
#endif
if (ssP->bResampled) {
	D2_FREE (ssP->samples);
	ssP->bResampled = 0;
	}
#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer) {
	//resample to two channels
	ssP->channel = gameStates.sound.digi.nNextChannel;
	if (pszWAV && *pszWAV) {
#if 0
		if (!(ssP->samples = CFReadData (pszWAV, gameFolders.szDataDir, 0)))
			return -1;
		ssP->mixChunk = Mix_QuickLoad_WAV ((Uint8 *) ssP->samples);
#else
		char	szWAV [FILENAME_LEN];
		if (!CFExtract (pszWAV, gameFolders.szDataDir, 0, "d2x-temp.wav"))
			return -1;
		sprintf (szWAV, "%s%sd2x-temp.wav", gameFolders.szTempDir, *gameFolders.szTempDir ? "/" : "");
		ssP->mixChunk = Mix_LoadWAV (szWAV);
#endif
		}
	else {
		if (dsP->bHires) {
			l = dsP->nLength [0];
			ssP->samples = dsP->data [0];
			}
		else {
			if (gameOpts->sound.bHires)
				return -1;	//cannot mix hires and standard sounds
			l = DigiResampleSound (dsP, ssP, gameOpts->sound.bD1Sound && (gameOpts->sound.digiSampleRate != SAMPLE_RATE_11K), gameData.songs.user.bMP3);
			if (l <= 0)
				return -1;
			if (speed < F1_0)
				l = DigiSpeedupSound (dsP, ssP, speed);
			}
		ssP->mixChunk = Mix_QuickLoad_RAW (ssP->samples, l);
		}
	Mix_VolPan (gameStates.sound.digi.nNextChannel, volume, pan);
	Mix_PlayChannel (gameStates.sound.digi.nNextChannel, ssP->mixChunk, looping ? -1 : loop_end - loop_start);
	}
else 
#else
if (pszWAV && *pszWAV)
	return -1;
#endif
	{
	if (gameOpts->sound.bD1Sound && (gameOpts->sound.digiSampleRate != SAMPLE_RATE_11K)) {
		int l = DigiResampleSound (dsP, ssP, 0, 0);
		if (l <= 0)
			return -1;
		ssP->length = l;
		}
	else {
		ssP->samples = dsP->data [dsP->bDTX];
		ssP->length = dsP->nLength [dsP->bDTX];
		}
	if (speed < F1_0)
		DigiSpeedupSound (dsP, ssP, speed);
	}
ssP->volume = FixMul (gameStates.sound.digi.nVolume, volume);
ssP->pan = pan;
ssP->position = 0;
ssP->soundobj = soundobj;
ssP->looped = looping;
#if USE_OPENAL
ssP->loops = looping ? -1 : loop_end - loop_start + 1;
#endif
ssP->soundno = nSound;
ssP->persistent = 0;
ssP->playing = 1;
ssP->persistent = (soundobj > -1) || looping || (volume > F1_0);
i = gameStates.sound.digi.nNextChannel;
if (++gameStates.sound.digi.nNextChannel >= gameStates.sound.digi.nMaxChannels)
	gameStates.sound.digi.nNextChannel = 0;
return i;
}

//------------------------------------------------------------------------------
// Returns the channel a sound number is playing on, or
// -1 if none.
int DigiFindChannel (short soundno)
{
if (!gameStates.app.bUseSound)
	return -1;
if (!gameStates.sound.digi.bInitialized)
	return -1;
if (soundno < 0)
	return -1;
if (gameData.pig.sound.sounds [gameOpts->sound.bD1Sound][soundno].data == NULL) {
	Int3 ();
	return -1;
	}
//FIXME: not implemented
return -1;
}

//------------------------------------------------------------------------------
//added on 980905 by adb from original source to make sfx volume work
void DigiSetFxVolume (int dvolume)
{
if (!gameStates.app.bUseSound)
	return;
dvolume = FixMulDiv (dvolume, SOUND_MAX_VOLUME, 0x7fff);
if (dvolume > SOUND_MAX_VOLUME)
	gameStates.sound.digi.nVolume = SOUND_MAX_VOLUME;
else if (dvolume < 0)
	gameStates.sound.digi.nVolume = 0;
else
	gameStates.sound.digi.nVolume = dvolume;
if (!gameStates.sound.digi.bInitialized) 
	return;
DigiSyncSounds ();
}
//end edit by adb
//------------------------------------------------------------------------------

void DigiMidiVolume (int dvolume, int mvolume)
{
if (!gameStates.app.bUseSound)
	return;
DigiSetFxVolume (dvolume);
DigiSetMidiVolume (mvolume);
//      //mprintf ((1, "Volume: 0x%x and 0x%x\n", gameStates.sound.digi.nVolume, midiVolume));
}

//------------------------------------------------------------------------------

int DigiIsSoundPlaying (short soundno)
{
	int i;

soundno = DigiXlatSound (soundno);
for (i = 0; i < MAX_SOUND_SLOTS; i++)
  //changed on 980905 by adb: added SoundSlots[i].playing &&
  if (SoundSlots[i].playing && SoundSlots[i].soundno == soundno)
  //end changes by adb
		return 1;
return 0;
}

//------------------------------------------------------------------------------
//added on 980905 by adb to make sound channel setting work
void DigiSetMaxChannels (int n) 
{ 
if (!gameStates.app.bUseSound)
	return;
gameStates.sound.digi.nMaxChannels	= n;
if (gameStates.sound.digi.nMaxChannels < 1) 
	gameStates.sound.digi.nMaxChannels = 1;
if (gameStates.sound.digi.nMaxChannels > MAX_SOUND_SLOTS) 
	gameStates.sound.digi.nMaxChannels = MAX_SOUND_SLOTS;
if (!gameStates.sound.digi.bInitialized) 
	return;
DigiStopAllChannels ();
}

//------------------------------------------------------------------------------

int DigiGetMaxChannels () 
{ 
return gameStates.sound.digi.nMaxChannels; 
}
// end edit by adb

//------------------------------------------------------------------------------

int DigiIsChannelPlaying (int channel)
{
if (!gameStates.sound.digi.bInitialized)
	return 0;
return SoundSlots[channel].playing;
}

//------------------------------------------------------------------------------

void DigiSetChannelVolume (int channel, int volume)
{
if (!gameStates.app.bUseSound)
	return;
if (!gameStates.sound.digi.bInitialized)
	return;
if (!SoundSlots[channel].playing)
	return;
SoundSlots[channel].volume = FixMulDiv (volume, gameStates.sound.digi.nVolume, F1_0);
#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer)
	Mix_Volume (channel, (volume * gameStates.sound.digi.nVolume / F1_0) / (SOUND_MAX_VOLUME / MIX_MAX_VOLUME));
#endif
}

//------------------------------------------------------------------------------

void DigiSetChannelPan (int channel, int pan)
{
if (!gameStates.app.bUseSound)
	return;
if (!gameStates.sound.digi.bInitialized)
	return;
if (!SoundSlots [channel].playing)
	return;
SoundSlots [channel].pan = pan;
#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer) {
	pan /= (32767 / 127);
	Mix_SetPanning (channel, (ubyte) pan, (ubyte) (254 - pan));
	}
#endif
}

//------------------------------------------------------------------------------

void DigiStopSound (int channel)
{
	tSoundSlot *ssP = SoundSlots + channel;
		
if (!gameStates.app.bUseSound)
	return;
ssP->playing = 0;
ssP->soundobj = -1;
ssP->persistent = 0;
#if USE_OPENAL
if (gameOpts->sound.bUseOpenAL) {
	if (ssP->source != 0xFFFFFFFF)
		alSourceStop (ssP->source);
	}
#endif
#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer) {
	if (ssP->mixChunk) {
		Mix_HaltChannel (channel);
		Mix_FreeChunk (ssP->mixChunk);
		ssP->mixChunk = NULL;
		}
	//Mix_FadeOutChannel (channel, 500);
	}
#endif
if (ssP->bResampled) {
	D2_FREE (ssP->samples);
	ssP->samples = NULL;
	ssP->bResampled = 0;
	}
}

//------------------------------------------------------------------------------

void DigiEndSound (int channel)
{
if (!gameStates.app.bUseSound)
	return;
if (!gameStates.sound.digi.bInitialized)
	return;
if (!SoundSlots[channel].playing)
	return;
DigiStopSound (channel);
}

//------------------------------------------------------------------------------

#if !(defined (_WIN32) || USE_SDL_MIXER)
// MIDI stuff follows.
void DigiSetMidiVolume (int mvolume) { }

int DigiPlayMidiSong (char * filename, char * melodic_bank, char * drum_bank, int loop, int bD1Song) {return 0;}

void DigiStopCurrentSong ()
{
#ifdef HMIPLAY
        char buf[10];
    
        sprintf (buf,"s");
        send_ipc (buf);
#endif
}
void DigiPauseMidi () {}
void DigiResumeMidi () {}
#endif

//------------------------------------------------------------------------------

void DigiFreeSoundBufs (void)
{
	tSoundSlot	*ssP;
	int			i;

for (ssP = SoundSlots, i = sizeofa (SoundSlots); i; i--, ssP++)
	if (ssP->bResampled) {
		D2_FREE (ssP->samples);
		ssP->bResampled = 0;
		}
}

//------------------------------------------------------------------------------
#ifdef _DEBUG
void DigiDebug ()
{
	int i;
	int n_voices = 0;

if (!gameStates.sound.digi.bInitialized)
	return;
for (i = 0; i < gameStates.sound.digi.nMaxChannels; i++) {
	if (DigiIsChannelPlaying (i))
		n_voices++;
	}
//mprintf_at ((0, 2, 0, "DIGI: Active Sound Channels: %d/%d (HMI says %d/32)      ", n_voices, gameStates.sound.digi.nMaxChannels, -1));
////mprintf_at ((0, 3, 0, "DIGI: Number locked sounds:  %d                          ", digiTotal_locks));
}
#endif

//------------------------------------------------------------------------------
