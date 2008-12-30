/* $Id: digi.c,v 1.12 2005/02/25 10:49:48 btb Exp $ */
#define DIGI_SOUND
#define MIDI_SOUND

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include <math.h>

#include "error.h"
#include "mono.h"
#include "fix.h"
#include "vecmat.h"
#include "gr.h" // needed for piggy.h
#include "piggy.h"
#include "digi.h"
#include "sounds.h"
#include "wall.h"
#include "newdemo.h"
#include "kconfig.h"
#include "hmpfile.h"

//#include "altsound.h"

hmp_file *hmp = NULL;

#ifdef DIGI_SOUND
#define MAX_SOUND_SLOTS 32
#define MIN_VOLUME 10


//added/changed on 980905 by adb to make sfx volume work
#define SOUND_MAX_VOLUME I2X (1)
int gameStates.sound.digi.nVolume = SOUND_MAX_VOLUME;
//end edit by adb

LPDIRECTSOUND lpds;
WAVEFORMATEX waveformat;
DSBUFFERDESC dsbd;

extern HWND g_hWnd;

struct sound_slot {
 int soundno;
 int playing;   // Is there a sample playing on this channel?
 int looped;    // Play this sample looped?
 fix pan;       // 0 = far left, 1 = far right
 fix volume;    // 0 = nothing, 1 = fully on
 //changed on 980905 by adb from char * to ubyte * 
 ubyte *samples;
 //end changes by adb
 uint length; // Length of the sample
 uint position; // Position we are at at the moment.
 LPDIRECTSOUNDBUFFER lpsb;
} SoundSlots[MAX_SOUND_SLOTS];


int midiVolume = 255;
int gameData.songs.bPlaying = 0;
int digi_last_midi_song = 0;
int digi_last_midi_song_loop = 0;

static int gameStates.sound.digi.bInitialized = 0;
static int digi_atexit_initialised=0;

//added on 980905 by adb to add rotating/volume based sound kill system
static int gameStates.sound.digi.nMaxChannels = 16;
static int next_handle = 0;
int SampleHandles[32];
void resetSounds_on_channel(int channel);
//end edit by adb

void digi_reset_digiSounds(void);

void DigiReset() { }

void DigiClose(void)
{
	if(gameStates.sound.digi.bInitialized)
	{
		digi_reset_digiSounds();
		IDirectSound_Release(lpds);
	}
	gameStates.sound.digi.bInitialized = 0;
}

/* Initialise audio devices. */
int DigiInit()
{
 HRESULT hr;
 
 if (!gameStates.sound.digi.bInitialized && g_hWnd){

	 memset(&waveformat, 0, sizeof(waveformat));
	 waveformat.wFormatTag=WAVE_FORMAT_PCM;
	 waveformat.wBitsPerSample=8;
	 waveformat.nChannels = 1;
	 waveformat.nSamplesPerSec = gameOpts->sound.digiSampleRate; //11025;
	 waveformat.nBlockAlign = waveformat.nChannels * (waveformat.wBitsPerSample / 8);
	 waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;

	  if ((hr = DirectSoundCreate(NULL, &lpds, NULL)) != DS_OK)
	   return -1;

	  if ((hr = IDirectSound_SetCooperativeLevel(lpds, g_hWnd, DSSCL_PRIORITY)) //hWndMain
	       != DS_OK)
	   {
	    IDirectSound_Release(lpds);
	    return -1;
	   }

	 memset(&dsbd, 0, sizeof(dsbd));
	 dsbd.dwSize = sizeof(dsbd);
	 dsbd.dwFlags = DSBCAPS_CTRLDEFAULT | DSBCAPS_GETCURRENTPOSITION2;
	 dsbd.dwBufferBytes = 8192;
	 dsbd.dwReserved=0;
	 dsbd.lpwfxFormat = &waveformat;

	 gameStates.sound.digi.bInitialized = 1;
}

	if (!digi_atexit_initialised){
		//atexit(DigiClose);
		digi_atexit_initialised=1;
	}
 return 0;
}

// added 2000/01/15 Matt Mueller -- remove some duplication (and fix a big memory leak, in the kill=0 one case)
static int DS_release_slot(int slot, int kill)
{
	if (SoundSlots[slot].lpsb)
	{
		DWORD s;

		IDirectSoundBuffer_GetStatus(SoundSlots[slot].lpsb, &s);
		if (s & DSBSTATUS_PLAYING)
		{
			if (kill)
				IDirectSoundBuffer_Stop(SoundSlots[slot].lpsb);
			else
				return 0;
		}
		IDirectSoundBuffer_Release(SoundSlots[slot].lpsb);
		SoundSlots[slot].lpsb = NULL;
	}
	SoundSlots[slot].playing = 0;
	return 1;
}

void DigiStopAllChannels()
{
	int i;

	for (i = 0; i < MAX_SOUND_SLOTS; i++)
		DigiStopSound(i);
}


static int get_free_slot()
{
 int i;

 for (i=0; i<MAX_SOUND_SLOTS; i++)
 {
		if (DS_release_slot(i, 0))
			return i;
 }
 return -1;
}

int D1vol2DSvol(fix d1v){
//multiplying by 1.5 doesn't help.  DirectSound uses dB for volume, rather than a linear scale like d1 wants.
//I had to pull out a math book, but here is the code to fix it :)  -Matt Mueller
//log x=y  <==>  x=a^y  
//   a
	 if (d1v<=0)
		 return -10000;
	 else
//		 return log2(X2F(d1v))*1000;//no log2? hm.
		 return log(X2F(d1v))/log(2)*1000.0;
}

// Volume 0-I2X (1)
int DigiStartSound(short nSound, fix volume, int pan, int looping, int loop_start, int loop_end, int soundobj)
{
 int ntries;
 int slot;
 HRESULT hr;

	if (!gameStates.sound.digi.bInitialized)
		return -1;

	// added on 980905 by adb from original source to add sound kill system
	// play at most digi_max_channel samples, if possible kill sample with low volume
	ntries = 0;

TryNextChannel:
	if ((SampleHandles[next_handle] >= 0) && (SoundSlots[SampleHandles[next_handle]].playing))
	{
		if ((SoundSlots[SampleHandles[next_handle]].volume > gameStates.sound.digi.nVolume) && (ntries < gameStates.sound.digi.nMaxChannels))
		{
			next_handle++;
			if (next_handle >= gameStates.sound.digi.nMaxChannels)
				next_handle = 0;
			ntries++;
			goto TryNextChannel;
		}
		SampleHandles[next_handle] = -1;
	}

	slot = get_free_slot();
	if (slot < 0)
		return -1;

	SoundSlots[slot].soundno = nSound;
	SoundSlots[slot].samples = Sounddat(nSound)->data;
	SoundSlots[slot].length = Sounddat(nSound)->length;
	SoundSlots[slot].volume = FixMul(gameStates.sound.digi.nVolume, volume);
	SoundSlots[slot].pan = pan;
	SoundSlots[slot].info.position.vPosition = 0;
	SoundSlots[slot].looped = 0;
	SoundSlots[slot].playing = 1;

	memset(&waveformat, 0, sizeof(waveformat));
	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.wBitsPerSample = Sounddat(nSound)->bits;
	waveformat.nChannels = 1;
	waveformat.nSamplesPerSec = Sounddat(nSound)->freq; //gameOpts->sound.digiSampleRate;
	waveformat.nBlockAlign = waveformat.nChannels * (waveformat.wBitsPerSample / 8);
	waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;

	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_CTRLDEFAULT | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwReserved=0;
	dsbd.dwBufferBytes = SoundSlots[slot].length;
	dsbd.lpwfxFormat = &waveformat;

	hr = IDirectSound_CreateSoundBuffer(lpds, &dsbd, &SoundSlots[slot].lpsb, NULL);
	if (hr != DS_OK)
	{
		//printf("Createsoundbuffer failed! hr=0x%X\n", (int)hr);
		abort();
	}

	{
		void *ptr1, *ptr2;
		DWORD len1, len2;

		IDirectSoundBuffer_Lock(SoundSlots[slot].lpsb, 0, Sounddat(nSound)->length,
		                        &ptr1, &len1, &ptr2, &len2, 0);
		memcpy(ptr1, Sounddat(nSound)->data, MIN(len1, Sounddat(nSound)->length));
		IDirectSoundBuffer_Unlock(SoundSlots[slot].lpsb, ptr1, len1, ptr2, len2);
	}

	IDirectSoundBuffer_SetPan(SoundSlots[slot].lpsb, ((int)(X2F(pan) * 20000.0)) - 10000);
	IDirectSoundBuffer_SetVolume(SoundSlots[slot].lpsb, D1vol2DSvol(SoundSlots[slot].volume));
	IDirectSoundBuffer_Play(SoundSlots[slot].lpsb, 0, 0, 0);

	// added on 980905 by adb to add sound kill system from original sos digi.c
	resetSounds_on_channel(slot);
	SampleHandles[next_handle] = slot;
	next_handle++;
	if (next_handle >= gameStates.sound.digi.nMaxChannels)
		next_handle = 0;
	// end edit by adb

	return slot;
}

// Returns the channel a sound number is playing on, or
// -1 if none.
int DigiFindChannel(int soundno)
{
	if (!gameStates.sound.digi.bInitialized)
		return -1;

	if (soundno < 0 )
		return -1;

	if (gameData.pig.sound.sounds[soundno].data == NULL)
	{
		Int3();
		return -1;
	}

	//FIXME: not implemented
	return -1;
}

 //added on 980905 by adb to add sound kill system from original sos digi.c
void resetSounds_on_channel( int channel )
{
 int i;

 for (i=0; i<gameStates.sound.digi.nMaxChannels; i++)
  if (SampleHandles[i] == channel)
   SampleHandles[i] = -1;
}
//end edit by adb

//added on 980905 by adb from original source to make sfx volume work
void DigiSetFxVolume( int dvolume )
{
	dvolume = FixMulDiv( dvolume, SOUND_MAX_VOLUME, 0x7fff);
	if ( dvolume > SOUND_MAX_VOLUME )
		gameStates.sound.digi.nVolume = SOUND_MAX_VOLUME;
	else if ( dvolume < 0 )
		gameStates.sound.digi.nVolume = 0;
	else
		gameStates.sound.digi.nVolume = dvolume;

	if ( !gameStates.sound.digi.bInitialized ) return;

	DigiSyncSounds();
}
//end edit by adb

void DigiMidiVolume( int dvolume, int mvolume ) 
{ 
	DigiSetFxVolume(dvolume);
	DigiSetMidiVolume(mvolume);
}

int DigiIsSoundPlaying(int soundno)
{
	int i;

	soundno = DigiXlatSound(soundno);

	for (i = 0; i < MAX_SOUND_SLOTS; i++)
		  //changed on 980905 by adb: added SoundSlots[i].playing &&
		  if (SoundSlots[i].playing && SoundSlots[i].soundno == soundno)
		  //end changes by adb
			return 1;
	return 0;
}


 //added on 980905 by adb to make sound channel setting work
void DigiSetMaxChannels(int n) { 
	gameStates.sound.digi.nMaxChannels	= n;

	if ( gameStates.sound.digi.nMaxChannels < 1 ) 
		gameStates.sound.digi.nMaxChannels = 1;
	if (gameStates.sound.digi.nMaxChannels > MAX_SOUND_SLOTS)
		gameStates.sound.digi.nMaxChannels = MAX_SOUND_SLOTS;

	if ( !gameStates.sound.digi.bInitialized ) return;

	digi_reset_digiSounds();
}

int DigiGetMaxChannels() { 
	return gameStates.sound.digi.nMaxChannels; 
}
// end edit by adb

void digi_reset_digiSounds() {
 int i;

 for (i=0; i< MAX_SOUND_SLOTS; i++) {
		DS_release_slot(i, 1);
 }
 
 //added on 980905 by adb to reset sound kill system
 memset(SampleHandles, 255, sizeof(SampleHandles));
 next_handle = 0;
 //end edit by adb
}

int DigiIsChannelPlaying(int channel)
{
	if (!gameStates.sound.digi.bInitialized)
		return 0;

	return SoundSlots[channel].playing;
}

void DigiSetChannelVolume(int channel, int volume)
{
	if (!gameStates.sound.digi.bInitialized)
		return;

	if (!SoundSlots[channel].playing)
		return;

	SoundSlots[channel].volume = FixMulDiv(volume, gameStates.sound.digi.nVolume, I2X (1));
}

void DigiSetChannelPan(int channel, int pan)
{
	if (!gameStates.sound.digi.bInitialized)
		return;

	if (!SoundSlots[channel].playing)
		return;

	SoundSlots[channel].pan = pan;
}

void DigiStopSound(int channel)
{
	SoundSlots[channel].playing=0;
	SoundSlots[channel].soundobj = -1;
	SoundSlots[channel].persistent = 0;
}

void DigiEndSound(int channel)
{
	if (!gameStates.sound.digi.bInitialized)
		return;

	if (!SoundSlots[channel].playing)
		return;

	SoundSlots[channel].soundobj = -1;
	SoundSlots[channel].persistent = 0;
}

#else
int gameData.songs.bPlaying = 0;
static int gameStates.sound.digi.bInitialized = 0;
int midiVolume = 255;

int digi_get_settings() { return 0; }
int DigiInit() { gameStates.sound.digi.bInitialized = 1; return 0; }
void DigiReset() {}
void DigiClose() {}

void DigiSetFxVolume( int dvolume ) {}
void DigiMidiVolume( int dvolume, int mvolume ) {}

int DigiIsSoundPlaying(int soundno) { return 0; }

void DigiSetMaxChannels(int n) {}
int DigiGetMaxChannels() { return 0; }

#endif

#ifdef MIDI_SOUND
// MIDI stuff follows.

void DigiStopCurrentSong()
{
	if ( gameData.songs.bPlaying ) {
	    hmp_close(hmp);
	    hmp = NULL;
	    gameData.songs.bPlaying = 0;
	}
}

void DigiSetMidiVolume( int n )
{
	int mmVolume;

	if (n < 0)
		midiVolume = 0;
	else if (n > 127)
		midiVolume = 127;
	else
		midiVolume = n;

	// scale up from 0-127 to 0-0xffff
	mmVolume = (midiVolume << 1) | (midiVolume & 1);
	mmVolume |= (mmVolume << 8);

	if (hmp)
		midiOutSetVolume((HMIDIOUT)hmp->hmidi, mmVolume | mmVolume << 16);
}

int DigiPlayMidiSong( char * filename, char * melodic_bank, char * drum_bank, int loop )
{       
	if (!gameStates.sound.digi.bInitialized) return;

        DigiStopCurrentSong();

       //added on 5/20/99 by Victor Rachels to fix crash/etc
        if(filename == NULL) return;
        if(midiVolume < 1) return;
       //end this section addition - VR

	if ((hmp = hmp_open(filename))) {
	    hmp_play(hmp);
	    gameData.songs.bPlaying = 1;
	    DigiSetMidiVolume(midiVolume);
	    return 1;
	}
	return 0;
	//else
		//printf("hmp_open failed\n");
}
void DigiPauseMidi() {}
void DigiResumeMidi() {}

#else
int DigiStopCurrentSong() {return 0;}
void DigiSetMidiVolume( int n ) {}
void DigiPlayMidiSong( char * filename, char * melodic_bank, char * drum_bank, int loop ) {}
void DigiPauseMidi() {}
void DigiResumeMidi() {}
#endif

#if DBG
void DigiDebug()
{
	int i;
	int n_voices = 0;

	if (!gameStates.sound.digi.bInitialized)
		return;

	for (i = 0; i < gameStates.sound.digi.nMaxChannels; i++)
	{
		if (DigiIsChannelPlaying(i))
			n_voices++;
	}
}
#endif
