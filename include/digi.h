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

#define SOUND_MAX_VOLUME (I2X (1) / 2)

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

#define DEFAULT_VOLUME				I2X (1)
#define DEFAULT_PAN					(I2X (1) / 2 - 1)

extern int digi_sample_rate;
extern int digiVolume;

extern int digi_lomem;

//------------------------------------------------------------------------------

typedef struct tSoundQueueEntry {
	fix			timeAdded;
	short			nSound;
} tSoundQueueEntry;

#define MAX_SOUND_QUEUE 32
#define MAX_LIFE			I2X (1)*30		// After being queued for 30 seconds, don't play it

class CSoundQueue {
	public:
		int	m_nHead;
		int	m_nTail;
		int	m_nSounds;
		int	m_nChannel;
		CArray<tSoundQueueEntry>	m_queue; // [MAX_SOUND_QUEUE];

	public:
		CSoundQueue () { Init (); }
		~CSoundQueue () { Destroy (); }
		void Init (void);
		void Destroy (void);
	};

extern CSoundQueue soundQueue;

//------------------------------------------------------------------------------

#endif
