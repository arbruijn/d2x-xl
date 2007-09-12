/* $Id: digi.h,v 1.6 2004/11/29 06:09:06 btb Exp $ */
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

#define USE_OPENAL	0

#if USE_OPENAL
#	include "al.h"
#	include "alc.h"
#	include "efx.h"
#endif

#ifdef _WIN32
#	ifdef _DEBUG
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

/*
#ifdef __DJGPP__
#define ALLEGRO
#endif
*/

#ifdef ALLEGRO
#include "allg_snd.h"
typedef SAMPLE tDigiSound;
#else
typedef struct tDigiSound {
	ubyte		bHires;
	ubyte		bDTX;
	int		nLength [2];
	ubyte		*data [2];
#if USE_OPENAL
	ALuint	buffer;
#endif
} tDigiSound;
#endif

#define SOUND_MAX_VOLUME (F1_0 / 2)

#define SAMPLE_RATE_11K         11025
#define SAMPLE_RATE_22K         22050
#define SAMPLE_RATE_44K         44100

#ifdef __DJGPP__
extern int digi_driver_board;
extern int digi_driver_port;
extern int digi_driver_irq;
extern int digi_driver_dma;
extern int digi_midiType;
extern int digi_midi_port;
#endif

extern int digi_sample_rate;
extern int digiVolume;

int digi_get_settings();
int DigiInit (float fSlowDown);
void DigiExit (void);
void DigiReset ();
void _CDECL_ DigiClose (void);

// Volume is max at F1_0.
int DigiPlaySampleSpeed (short soundno, fix maxVolume, int nSpeed, int nLoops, char *pszWAV);
int DigiPlaySample (short nSound, fix maxVolume);
int DigiPlaySampleLooped (short soundno, fix maxVolume, int nLoops);
int DigiPlayWAV (char *pszWAV, fix maxVolume);
int DigiPlayWAVLooped (char *pszWAV, fix maxVolume, int nLoops);
void DigiPlaySampleOnce (short nSound, fix maxVolume);
int DigiLinkSoundToObject (short nSound, short nObject, int forever, fix maxVolume);
int DigiLinkSoundToPos (short nSound, short nSegment, short nSide, vmsVector *pos, int forever, fix maxVolume);
// Same as above, but you pass the max distance sound can be heard.  The old way uses f1_0*256 for maxDistance.
int DigiLinkSoundToObject2 (short nSound, short nObject, int forever, fix maxVolume, fix  maxDistance);
int DigiLinkSoundToPos2 (short nSound, short nSegment, short nSide, vmsVector * pos, int forever, fix maxVolume, fix maxDistance);
int DigiLinkSoundToObject3 (short orgSoundnum, short nObject, int forever, fix maxVolume, fix  maxDistance, int nLoopStart, int nLoopEnd, char *pszSound);
int DigiPlayMidiSong (char * filename, char * melodic_bank, char * drum_bank, int loop, int bD1Song);
void DigiPlaySample3D (short nSound, int angle, int volume, int no_dups, vmsVector *vPos, char *pszSound); // Volume from 0-0x7fff
void DigiInitSounds();
void DigiSyncSounds();
int DigiKillSoundLinkedToSegment (short nSegment, short nSide, short nSound);
int DigiKillSoundLinkedToObject (int nObject);
void DigiSetMidiVolume (int mvolume);
void DigiSetFxVolume (int dvolume);
void DigiMidiVolume (int dvolume, int mvolume);
int DigiGetSoundByName (char *pszSound);
int DigiSetObjectSound (int nObject, int nSound, char *pszSound);

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
extern void DigiStopSound (int channel);

// Returns the channel a sound number is playing on, or
// -1 if none.
extern int DigiFindChannel(short nSound);

// Volume 0-F1_0
extern int DigiStartSound (short nSound, fix volume, int pan, int looping, 
								   int loop_start, int loop_end, int soundobj, int speed, 
								   char *pszWAV, vmsVector *vPos);

// Stops all sounds that are playing
void DigiStopAllChannels();

void DigiEndSound (int channel);
void DigiSetChannelPan (int channel, int pan);
void DigiSetChannelVolume (int channel, int volume);
int DigiIsChannelPlaying (int channel);
void DigiPauseMidi ();
void DigiDebug ();
void DigiStopCurrentSong ();

void DigiPlaySampleLooping (short nSound, fix maxVolume,int loop_start, int loop_end);
void DigiChangeLoopingVolume (fix volume);
void DigiStopLoopingSound ();
void DigiFreeSoundBufs (void);

// Plays a queued voice sound.
extern void DigiStartSoundQueued (short nSound, fix volume);

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

#endif
