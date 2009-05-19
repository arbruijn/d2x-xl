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

#ifndef _AUDIO_H
#define _AUDIO_H

#include "pstypes.h"
#include "vecmat.h"
#include "carray.h"
#include "cfile.h"

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

#ifdef __macosx__
# include <SDL/SDL.h>
# include <SDL_mixer/SDL_mixer.h>
#else
#	include <SDL.h>
#  include <SDL_mixer.h>
#endif

#define USE_OPENAL	0

#if USE_OPENAL
#	include "al.h"
#	include "alc.h"
#	include "efx.h"
#endif

#ifdef ALLEGRO

#include "allg_snd.h"
typedef SAMPLE 
;

#else //!defined ALLEGRO

class CSoundSample {
	public:
		char			szName [9];
		ubyte			bHires;
		ubyte			bCustom;
		int			nLength [2];
		int			nOffset [2];
		CByteArray	data [2];
#if USE_OPENAL
		ALuint		buffer;
#endif

	public:
		CSoundSample () { 
			bHires = bCustom = 0; 
			nLength [0] = nLength [1] = 0; 
		}
	};

#endif //!defined ALLEGRO

//------------------------------------------------------------------------------

#define MIN_SOUND_CHANNELS			32
#define MAX_SOUND_CHANNELS			128
#define MAX_SOUND_OBJECTS			150

#define SOUND_MAX_VOLUME			(I2X (1) / 2)

#define DEFAULT_VOLUME				I2X (1)
#define DEFAULT_PAN					(I2X (1) / 2 - 1)


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

#define SOF_USED				1 		// Set if this sample is used
#define SOF_PLAYING			2		// Set if this sample is playing on a channel
#define SOF_LINK_TO_OBJ		4		// Sound is linked to a moving CObject. If CObject dies, then finishes play and quits.
#define SOF_LINK_TO_POS		8		// Sound is linked to CSegment, pos
#define SOF_PLAY_FOREVER	16		// Play bForever (or until level is stopped), otherwise plays once
#define SOF_PERMANENT		32		// Part of the level, like a waterfall or fan

//------------------------------------------------------------------------------

#define MAX_SOUND_QUEUE 32
#define MAX_LIFE			I2X (30)		// After being queued for 30 seconds, don't play it

typedef struct tSoundQueueEntry {
	fix	timeAdded;
	short	nSound;
} tSoundQueueEntry;

typedef struct tSoundQueueData {
	int	nHead;
	int	nTail;
	int	nSounds;
	int	nChannel;
	CArray<tSoundQueueEntry>	queue; // [MAX_SOUND_QUEUE];
} tSoundQueueData;

class CSoundQueue {
	private:
		tSoundQueueData	m_data;

	public:
		CSoundQueue () { Init (); }
		~CSoundQueue () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void Pause (void);
		void End (void);
		void Process (void);
		void StartSound (short nSound, fix nVolume);
		inline int Channel (void) { return m_data.nChannel; }
	};

extern CSoundQueue soundQueue;

//------------------------------------------------------------------------------

class CAudioChannel {
	private:
		typedef struct tChannelInfo {
			public:
				int				nSound;
				fix				nPan;				// 0 = far left, 1 = far right
				fix				nVolume;			// 0 = nothing, 1 = fully on
				CByteArray		sample;
				uint				nLength;			// Length of the sample
				uint				nPosition;		// Position we are at at the moment.
				int				nSoundObj;		// Which soundobject is on this nChannel
				int				nSoundClass;
				int				nIndex;
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
			} tChannelInfo;

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
		void Stop (void);
		void Mix (ubyte* stream, int len);
		inline int Playing (void) { return m_info.bPlaying; }
		void SetPlaying (int bPlaying);
		inline int SoundObject (void) { return m_info.nSoundObj; }
		inline void SetSoundObj (int nSoundObj) { m_info.nSoundObj = nSoundObj; }
		inline int Sound (void) { return m_info.nSound; }
		inline int SoundClass (void) { return m_info.nSoundClass; }
		inline int Volume (void) { return m_info.nVolume; }
		inline int Persistent (void) { return m_info.bPersistent; }
		inline int Resampled (void) { return m_info.bResampled; }

	private:
		int Resample (CSoundSample *soundP, int bD1Sound, int bMP3);
		int Speedup (CSoundSample *soundP, int speed);
		int ReadWAV (void);
	};

//------------------------------------------------------------------------------

class CSoundObject {
	public:
		short			m_nSignature;		// A unique nSignature to this sound
		ubyte			m_flags;				// Used to tell if this slot is used and/or currently playing, and how long.
		ubyte			m_pad;				//	Keep alignment
		fix			m_maxVolume;		// Max volume that this sound is playing at
		fix			m_maxDistance;		// The max distance that this sound can be heard at...
		int			m_soundClass;
		short			m_nSound;			// The sound number that is playing
		int			m_channel;			// What channel this is playing on, -1 if not playing
		int			m_volume;			// Volume that this sound is playing at
		int			m_pan;				// Pan value that this sound is playing at
		int			m_nDecay;			// type of decay (0: linear, 1: quadratic, 2: cubic)
		char			m_szSound [FILENAME_LEN];	// file name of custom sound to be played
		int			m_nLoopStart;		// The start point of the loop. -1 means no loop
		int			m_nLoopEnd;			// The end point of the loop
		int			m_nMidiVolume;
		union {
			struct {
				short			nSegment;				// Used if SOF_LINK_TO_POS field is used
				short			nSide;
				CFixVector	position;
			} pos;
			struct {
				short			nObject;				// Used if SOF_LINK_TO_OBJ field is used
				short			nObjSig;
			} obj;
		} m_linkType;

	public:
		CSoundObject () { Init (); }
		~CSoundObject () {Init (); }
		void Init (void);
		bool Start (void);
		void Stop (void);
	};

//------------------------------------------------------------------------------

class CAudio {
	private:

		class CAudioInfo {
			public:
				int		bInitialized;
				int		bAvailable;
				int		bNoObjects;
				int		bLoMem;
				int		nFormat;
				int		nMaxChannels;
				int		nFreeChannel;
				int		nVolume;
				int		nMidiVolume;
				int		nNextSignature;
				int		nActiveObjects;
				short		nLoopingSound;
				short		nLoopingVolume;
				short		nLoopingStart;
				short		nLoopingEnd;
				short		nLoopingChannel;
				float		fSlowDown;
			};

	private:
		CAudioInfo					m_info;
		CArray<CAudioChannel>	m_channels;
		CStack<CSoundObject>		m_objects;
		CStack<int>					m_usedChannels;

	public:
		CAudio ();
		~CAudio () { Destroy (); }
		void Init (void);
#ifndef _WIN32
		int InitThread (void);
#endif
		void Destroy (void);
		int Setup (float fSlowDown, int nFormat = -1);
		void Cleanup (void);
		void Shutdown (void);
		void Close (void);
		void Reset (void);
		void FadeoutMusic (void);
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
		int FindChannel (short nSound);
		void SetMidiVolume (int nVolume);
		void SetFxVolume (int nVolume);
		void SetVolumes (int fxVolume, int midiVolume);
		inline int Volume (void) { return m_info.nVolume; }

		int StartSound (short nSound, int nSoundClass = SOUNDCLASS_GENERIC, fix nVolume = DEFAULT_VOLUME, int nPan = DEFAULT_PAN, 
							 int bLooping = 0, int nLoopStart = -1, int nLoopEnd = -1, int nSoundObj = -1, int nSpeed = I2X (1), 
							 const char *pszWAV = NULL, CFixVector* vPos = NULL);
		void StopSound (int channel);
		void StopActiveSound (int nChannel);
		void StopObjectSounds (void);
		void StopAllChannels (bool bPause = false);

		int PlaySound (short nSound, int nSoundClass = SOUNDCLASS_GENERIC, fix nVolume = I2X (1), int nPan = I2X (1) / 2 - 1, 
						   int bNoDups = 0, int nLoops = -1, const char* pszWAV = NULL, CFixVector* vPos = NULL);

		inline int PlayWAV (const char *pszWAV) {
			return PlaySound (-1, SOUNDCLASS_GENERIC, DEFAULT_VOLUME, DEFAULT_PAN, 0, -1, pszWAV);
			}

		int CreateObjectSound (short nSound, int nSoundClass, short nObject, int bForever = 0, fix maxVolume = I2X (1), fix maxDistance = I2X (256),
									  int nLoopStart = -1, int nLoopEnd = -1, const char *pszSound = NULL, int nDecay = 0);
		int ChangeObjectSound (int nObject, fix nVolume);
		int DestroyObjectSound (int nObject);
		void DeleteSoundObject (int i);

		int CreateSegmentSound (short nOrgSound, short nSegment, short nSide, CFixVector& vPos, int forever = 0, 
										fix maxVolume = I2X (1), fix maxDistance = I2X (256), const char *pszSound = NULL);
		int DestroySegmentSound (short nSegment, short nSide, short nSound);

		void StartLoopingSound (void);
		void PlayLoopingSound (short nSound, fix nVolume, int nLoopStart, int nLoopEnd);
		void ChangeLoopingVolume (fix nVolume);
		void PauseLoopingSound (void);
		void ResumeLoopingSound (void);
		void StopLoopingSound (void);

		void GetVolPan (CFixMatrix& mListener, CFixVector& vListenerPos, short nListenerSeg, 
							 CFixVector& vSoundPos, short nSoundSeg, 
							 fix maxVolume, int *volume, int *pan, fix maxDistance, int nDecay);

		void InitSounds (void);
		void SyncSounds (void);
		int GetSoundByName (const char *pszSound);

		int IsSoundPlaying (short nSound);

		void EndSoundObject (int i);

		void PauseAll (void);
		void ResumeAll (void);
		void PauseSounds (void);
		void ResumeSounds (void);
		void StopAll (void);

		short XlatSound (short nSound);
		int UnXlatSound (int nSound);

		inline int Format (void) { return m_info.nFormat; }
		inline void SetFormat (int nFormat) { m_info.nFormat = nFormat; }
		inline int Available (void) { return m_info.bAvailable; }
		inline int ActiveObjects (void) { return m_info.nActiveObjects; }
		inline int MaxChannels (void) { return MAX_SOUND_CHANNELS; }
		inline int FreeChannel (void) { return m_info.nFreeChannel; }
		inline CAudioChannel* Channel (uint i = 0) { return m_channels + i; }
		inline CStack<CSoundObject>& Objects (void) { return m_objects; }
		inline float SlowDown (void) { return m_info.fSlowDown; }
		inline void ActivateObject (void) { m_info.nActiveObjects++; }
		inline void DeactivateObject (void) { m_info.nActiveObjects--; }
		int RegisterChannel (CAudioChannel* channelP);
		void UnregisterChannel (int nIndex);

		void RecordSoundObjects (void);
#if DBG
		int VerifyChannelFree (int channel);
#endif
		static void _CDECL_ MixCallback (void* userdata, Uint8* stream, int len);

	private:
		CAudioChannel* FindFreeChannel (int nSoundClass);
	};

extern CAudio audio;

void Mix_VolPan (int nChannel, int nVolume, int nPan);
void SetSoundSources (void);
void SetD1Sound (void);

//------------------------------------------------------------------------------

typedef struct tWAVHeader {
	char	chunkID [4];
	ulong	chunkSize;
	char	riffType [4];
} tRIFFChunk;

typedef struct tWAVFormat {
	char		chunkID [4];
	ulong		chunkSize;
	ushort	format;
	ushort	channels;
	ulong		sampleRate;
	ulong		avgBytesPerSec;
	ushort	blockAlign;
	ushort	bitsPerSample;
	} tPCMFormatChunk;

typedef struct tWAVData {
	char	chunkID [4];
	ulong	chunkSize;
} tWAVData;

typedef struct tWAVInfo {
	tWAVHeader	header;
	tWAVFormat	format;
	tWAVData		data;
} tWAVInfo;

//------------------------------------------------------------------------------

#endif

