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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Include file for sound hardware.
 *
 */

#ifndef _DIGI_H
#define _DIGI_H

#include "pstypes.h"
#include "vecmat.h"
#include "carray.h"

#define USE_OPENAL	0

#if USE_OPENAL
#	include "al.h"
#	include "alc.h"
#	include "efx.h"
#endif

#ifdef _MSC_VER
#	if DBG
#		define USE_SDL_MIXER	1
#	else
#		define USE_SDL_MIXER	1
#	endif
#else
#	include "conf.h"
#	if !defined (USE_SDL_MIXER)
#		define USE_SDL_MIXER	0
#	endif
#endif

#ifdef ALLEGRO
#include "allg_snd.h"
typedef SAMPLE 
;
#else
class CDigiSound {
	public:
		ubyte			bHires;
		ubyte			bDTX;
		int			nLength [2];
		CByteArray	data [2];
#if USE_OPENAL
		ALuint		buffer;
#endif

	public:
		CDigiSound () { 
			bHires = bDTX = 0; 
			nLength [0] = nLength [1] = 0; 
		}
	};
#endif

#define SOUND_MAX_VOLUME (F1_0 / 2)

#define SAMPLE_RATE_11K				11025
#define SAMPLE_RATE_22K				22050
#define SAMPLE_RATE_44K				44100

#define SOUNDCLASS_GENERIC			0
#define SOUNDCLASS_PERSISTENT		1
#define SOUNDCLASS_PLAYER			2
#define SOUNDCLASS_ROBOT			3
#define SOUNDCLASS_LASER			4
#define SOUNDCLASS_MISSILE			5
#define SOUNDCLASS_EXPLOSION		6

extern int digi_sample_rate;
extern int digiVolume;

void audio.FadeoutMusic (void);
void _CDECL_ audio.Shutdown (void);

// Volume is max at F1_0.
int DigiPlaySampleSpeed (short soundno, fix maxVolume, int nSpeed, int nLoops, const char *pszWAV, int nSoundClass);
void DigiPlaySampleOnce (short nSound, fix maxVolume);
int DigiLinkSoundToObject (short nSound, short nObject, int forever, fix maxVolume, int nSoundClass);
int DigiLinkSoundToPos (short nSound, short nSegment, short nSide, CFixVector& vPos, int forever = 0, fix maxVolume = F1_0);
// Same as above, but you pass the max distance sound can be heard.  The old way uses f1_0*256 for maxDistance.
int DigiLinkSoundToObject2 (short nSound, short nObject, int forever, fix maxVolume, fix  maxDistance, int nSoundClass);
int DigiLinkSoundToPos2 (short nSound, short nSegment, short nSide, CFixVector& vPos, int forever, fix maxVolume, fix maxDistance, const char *pszSound);
int DigiLinkSoundToObject3 (short orgSoundnum, short nObject, int forever, fix maxVolume, fix maxDistance, 
									 int nLoopStart, int nLoopEnd, const char *pszSound, int nDecay, int nSoundClass);
int DigiPlayMidiSong (char * filename, char * melodic_bank, char * drum_bank, int loop, int bD1Song);
void DigiPlaySample3D (short nSound, int angle, int volume, int no_dups, CFixVector *vPos, const char *pszSound); // Volume from 0-0x7fff
void DigiInitSounds();
void DigiSyncSounds();
int DigiKillSoundLinkedToSegment (short nSegment, short nSide, short nSound);
int DigiKillSoundLinkedToObject (int nObject);
int DigiChangeSoundLinkedToObject (int nObject, fix volume);
void DigiSetMidiVolume (int mvolume);
void DigiSetFxVolume (int dvolume);
void DigiMidiVolume (int dvolume, int mvolume);
int DigiGetSoundByName (const char *pszSound);
int DigiSetObjectSound (int nObject, int nSound, const char *pszSound, const fix xVolume = F1_0);

int DigiIsSoundPlaying (short nSound);

void DigiPauseAll ();
void DigiResumeAll ();
void DigiPauseDigiSounds ();
void DigiResumeDigiSounds ();
void DigiStopAll ();

void DigiSetMaxChannels (int n);
int DigiGetMaxChannels ();

extern int digi_lomem;

short DigiXlatSound (short nSound);
int DigiUnXlatSound (int nSound);
extern void audio.StopSound (int channel);

// Returns the channel a sound number is playing on, or
// -1 if none.
extern int DigiFindChannel(short nSound);

// Volume 0-F1_0
int DigiStartSound (short nSound, fix xVolume, int xPan, int bLooping, 
					     int nLoopStart, int nLoopEnd, int nSoundObj, int nSpeed, 
						  const char *pszWAV, CFixVector *vPos, int nSoundClass);

// Stops all sounds that are playing
void audio.StopAllSounds();

void audio.StopActiveSound (int channel);
void DigiSetChannelPan (int channel, int pan);
void DigiSetChannelVolume (int channel, int volume);
int DigiIsChannelPlaying (int channel);
void DigiPauseMidi ();
void audio.Debug ();
void DigiStopCurrentSong ();

void DigiPlaySampleLooping (short nSound, fix maxVolume,int loop_start, int loop_end);
void DigiChangeLoopingVolume (fix volume);
void DigiStopLoopingSound ();
void DigiFreeSoundBufs (void);

void DigiStopDigiSounds (void);

// Plays a queued voice sound.
void DigiStartSoundQueued (short nSound, fix volume);

//------------------------------------------------------------------------------

typedef struct tSoundQueueEntry {
	fix			time_added;
	short			nSound;
} tSoundQueueEntry;

#define MAX_SOUND_QUEUE 32
#define MAX_LIFE			F1_0*30		// After being queued for 30 seconds, don't play it

typedef struct tSoundQueue {
	int	nHead;
	int	nTail;
	int	nSounds;
	int	nChannel;
	tSoundQueueEntry	queue [MAX_SOUND_QUEUE];
} tSoundQueue;

extern tSoundQueue soundQueue;

//------------------------------------------------------------------------------

static inline int DigiPlaySampleClass (short nSound, const char *pszSound, fix maxVolume, int nSoundClass)
{
return DigiPlaySampleSpeed (nSound, maxVolume, F1_0, 0, pszSound, nSoundClass);
}

//------------------------------------------------------------------------------

static inline int DigiPlaySample (short nSound, fix maxVolume)
{
return DigiPlaySampleSpeed (nSound, maxVolume, F1_0, 0, NULL, SOUNDCLASS_GENERIC);
}

//------------------------------------------------------------------------------

static inline int DigiPlaySampleLooped (short nSound, fix maxVolume, int nLoops)
{
return DigiPlaySampleSpeed (nSound, maxVolume, F1_0, nLoops, NULL, SOUNDCLASS_GENERIC);
}

//------------------------------------------------------------------------------

static inline int DigiPlayWAV (const char *pszWAV, fix maxVolume)
{
return DigiPlaySampleSpeed (-1, maxVolume, F1_0, 0, pszWAV, SOUNDCLASS_GENERIC);
}

//------------------------------------------------------------------------------

static inline int DigiPlayWAVLooped (const char *pszWAV, fix maxVolume, int nLoops)
{
return DigiPlaySampleSpeed (-1, maxVolume, F1_0, nLoops, pszWAV, SOUNDCLASS_GENERIC);
}

//------------------------------------------------------------------------------

#endif
