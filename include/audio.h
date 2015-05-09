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
#include "descent.h"
#include "findpath.h"

#ifdef _MSC_VER
#	if DBG
#		define USE_SDL_MIXER	1
#	else
#		define USE_SDL_MIXER	1
#	endif
#else
#	if HAVE_CONFIG_H
#		include "conf.h"
#	endif
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
		uint8_t		bHires;
		uint8_t		bCustom;
		int32_t		nLength [2];
		int32_t		nOffset [2];
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
#define MAX_SOUND_OBJECTS			500

#define SOUND_MAX_VOLUME			(I2X (1) / 2)

#define DEFAULT_VOLUME				I2X (1)
#define DEFAULT_PAN					(I2X (1) / 2 - 1)


#define SAMPLE_RATE_11K				11025
#define SAMPLE_RATE_22K				22050
#define SAMPLE_RATE_44K				44100

#define SOUNDCLASS_GENERIC			0
#define SOUNDCLASS_AMBIENT			1
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
	int16_t	nSound;
} tSoundQueueEntry;

typedef struct tSoundQueueData {
	int32_t	nHead;
	int32_t	nTail;
	int32_t	nSounds;
	int32_t	nChannel;
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
		void StartSound (int16_t nSound, fix nVolume);
		inline int32_t Channel (void) { return m_data.nChannel; }
	};

extern CSoundQueue soundQueue;

//------------------------------------------------------------------------------

class CAudioChannel {
	private:
		typedef struct tChannelInfo {
			public:
				int32_t			nSound;
				fix				nPan;				// 0 = far left, 1 = far right
				fix				nVolume;			// 0 = nothing, 1 = fully on
				CByteArray		sample;
				uint32_t			nLength;			// Length of the sample
				uint32_t			nPosition;		// Position we are at at the moment.
				int32_t			nSoundObj;		// Which soundobject is on this nChannel
				int32_t			nSoundClass;
				int32_t			nIndex;
				uint8_t			bPlaying;		// Is there a sample playing on this nChannel?
				uint8_t			bLooped;			// Play this sample looped?
				uint8_t			bPersistent;	// This can't be pre-empted
				uint8_t			bResampled;
				uint8_t			bBuiltIn;
				uint8_t			bAmbient;
#if USE_SDL_MIXER
				Mix_Chunk*		pMixChunk;
				int32_t			nChannel;
#endif
#if USE_OPENAL
				ALuint			source;
				int32_t				loops;
#endif
			} tChannelInfo;

		tChannelInfo	m_info;

	public:
		CAudioChannel () { Init (); }
		~CAudioChannel () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void SetVolume (int32_t nVolume);
		void SetPan (int32_t nPan);
		int32_t Start (int16_t nSound, int32_t nSoundClass, fix nVolume, int32_t nPan, int32_t bLooping,
					  int32_t nLoopStart, int32_t nLoopEnd, int32_t nSoundObj, int32_t nSpeed,
					  const char *pszWAV, CFixVector* vPos);
		void Stop (void);
		void Mix (uint8_t* stream, int32_t len);
		inline int32_t Playing (void) { return m_info.bPlaying && m_info.pMixChunk; }
		void SetPlaying (int32_t bPlaying);
		inline int32_t GetIndex (void) { return m_info.nIndex; }
		inline void SetIndex (int32_t nIndex) { m_info.nIndex = nIndex; }
		fix Duration (void);
		inline int32_t SoundObject (void) { return m_info.nSoundObj; }
		inline void SetSoundObj (int32_t nSoundObj) { m_info.nSoundObj = nSoundObj; }
		inline int32_t Sound (void) { return m_info.nSound; }
		inline int32_t SoundClass (void) { return m_info.nSoundClass; }
		inline int32_t Volume (void) { return m_info.nVolume; }
		inline int32_t Persistent (void) { return m_info.bPersistent; }
		inline int32_t Resampled (void) { return m_info.bResampled; }

	private:
		int32_t Resample (CSoundSample *pSound, int32_t bD1Sound, int32_t bMP3);
		int32_t Speedup (CSoundSample *pSound, int32_t speed);
		int32_t ReadWAV (void);
	};

//------------------------------------------------------------------------------

class CSoundObject {
	public:
		int16_t			m_nSignature;		// A unique nSignature to this sound
		uint8_t			m_flags;				// Used to tell if this slot is used and/or currently playing, and how long.
		uint8_t			m_bCustom;			//	Keep alignment
		fix			m_maxVolume;		// Max volume that this sound is playing at
		fix			m_maxDistance;		// The max distance that this sound can be heard at...
		int32_t			m_soundClass;
		int32_t			m_bAmbient;
		int16_t			m_nSound;			// The sound number that is playing
		int32_t			m_channel;			// What channel this is playing on, -1 if not playing
		int32_t			m_volume;			// Volume that this sound is playing at
		int32_t			m_audioVolume;		// audio volume set when the sound object was last updated
		int32_t			m_pan;				// Pan value that this sound is playing at
		int32_t			m_nDecay;			// type of decay (0: linear, 1: quadratic, 2: cubic)
		char			m_szSound [FILENAME_LEN];	// file name of custom sound to be played
		int32_t			m_nLoopStart;		// The start point of the loop. -1 means no loop
		int32_t			m_nLoopEnd;			// The end point of the loop
		int32_t			m_nMidiVolume;
		union {
			struct {
				int16_t			nSegment;				// Used if SOF_LINK_TO_POS field is used
				int16_t			nSide;
				CFixVector	position;
			} pos;
			struct {
				int16_t			nObject;				// Used if SOF_LINK_TO_OBJ field is used
				int16_t			nObjSig;
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
				int32_t		bInitialized;
				int32_t		bAvailable;
				int32_t		bNoObjects;
				int32_t		bLoMem;
				int32_t		nFormat;
				int32_t		nMaxChannels;
				int32_t		nFreeChannel;
				int32_t		nVolume [2];
				int32_t		nMidiVolume;
				int32_t		nNextSignature;
				int32_t		nActiveObjects;
				int16_t		nLoopingSound;
				int16_t		nLoopingVolume;
				int16_t		nLoopingStart;
				int16_t		nLoopingEnd;
				int16_t		nLoopingChannel;
				float			fSlowDown;
			};

	private:
		CAudioInfo					m_info;
		CArray<CAudioChannel>	m_channels;
		CStack<CSoundObject>		m_objects;
		CStack<int32_t>			m_usedChannels;
		CDACSUniDirRouter			m_router [MAX_THREADS];
		//CArray<fix>					m_segDists;
		int16_t						m_nListenerSeg;
		bool							m_bSDLInitialized;

	private:
		int32_t AudioError (void);
		int32_t InternalSetup (float fSlowDown, int32_t nFormat, const char* driver);

	public:
		CAudio ();
		~CAudio () { Destroy (); }
		void Init (void);
#ifndef _WIN32
		int32_t InitThread (void);
#endif
		void Destroy (void);
		void Prepare (void);
		int32_t Setup (float fSlowDown, int32_t nFormat = -1);
		void SetupRouter (void);
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
		int32_t SoundClassCount (int32_t nSoundClass);
		void SetVolume (int32_t nChannel, int32_t nVolume);
		void SetPan (int32_t nChannel, int32_t nPan);
		int32_t ChannelIsPlaying (int32_t nChannel);
		int32_t SoundIsPlaying (int16_t nSound);
		void SetMaxChannels (int32_t nChannels);
		int32_t GetMaxChannels (void);
		int32_t FindChannel (int16_t nSound);
		void SetMidiVolume (int32_t nVolume);
		void SetFxVolume (int32_t nVolume, int32_t nType = 0);
		void SetVolumes (int32_t fxVolume, int32_t midiVolume);
		inline int32_t Volume (int32_t nType = 0) { return m_info.nVolume [nType]; }

		int32_t StartSound (int16_t nSound, int32_t nSoundClass = SOUNDCLASS_GENERIC, fix nVolume = DEFAULT_VOLUME, int32_t nPan = DEFAULT_PAN,
							 int32_t bLooping = 0, int32_t nLoopStart = -1, int32_t nLoopEnd = -1, int32_t nSoundObj = -1, int32_t nSpeed = I2X (1),
							 const char *pszWAV = NULL, CFixVector* vPos = NULL);
		void StopSound (int32_t channel);
		void StopActiveSound (int32_t nChannel);
		void StopObjectSounds (void);
		void StopAllChannels (bool bPause = false);

		int32_t PlaySound (int16_t nSound, int32_t nSoundClass = SOUNDCLASS_GENERIC, fix nVolume = I2X (1), int32_t nPan = I2X (1) / 2 - 1,
						   int32_t bNoDups = 0, int32_t nLoops = -1, const char* pszWAV = NULL, CFixVector* vPos = NULL);

		inline int32_t PlayWAV (const char *pszWAV) {
			return PlaySound (-1, SOUNDCLASS_GENERIC, DEFAULT_VOLUME, DEFAULT_PAN, 0, -1, pszWAV);
			}

		int32_t CreateObjectSound (int16_t nSound, int32_t nSoundClass, int16_t nObject, int32_t bForever = 0, fix maxVolume = I2X (1), fix maxDistance = I2X (256),
									  int32_t nLoopStart = -1, int32_t nLoopEnd = -1, const char *pszSound = NULL, int32_t nDecay = 0, uint8_t bCustom = 0, int32_t nThread = 0);
		int32_t ChangeObjectSound (int32_t nObject, fix nVolume);
		int32_t FindObjectSound (int32_t nObject, int16_t nSound);
		int32_t DestroyObjectSound (int32_t nObject, int16_t nSound = -1);
		bool SuspendObjectSound (int32_t nThreshold);
		void DeleteSoundObject (int32_t i);

		int32_t CreateSegmentSound (int16_t nOrgSound, int16_t nSegment, int16_t nSide, CFixVector& vPos, int32_t forever = 0,
										fix maxVolume = I2X (1), fix maxDistance = I2X (256), const char *pszSound = NULL);
		int32_t DestroySegmentSound (int16_t nSegment, int16_t nSide, int16_t nSound);

		void StartLoopingSound (void);
		void PlayLoopingSound (int16_t nSound, fix nVolume, int32_t nLoopStart, int32_t nLoopEnd);
		void ChangeLoopingVolume (fix nVolume);
		void PauseLoopingSound (void);
		void ResumeLoopingSound (void);
		void StopLoopingSound (void);

		void GetVolPan (CFixMatrix& mListener, CFixVector& vListenerPos, int16_t nListenerSeg,
							 CFixVector& vSoundPos, int16_t nSoundSeg,
							 fix maxVolume, int32_t *volume, int32_t *pan, fix maxDistance, int32_t nDecay, 
							 int32_t nThread = 0);

		void InitSounds (void);
		void SyncSounds (void);
		int32_t GetSoundByName (const char *pszSound);

		int32_t IsSoundPlaying (int16_t nSound);

		void EndSoundObject (int32_t i);

		void PauseAll (void);
		void ResumeAll (void);
		void PauseSounds (void);
		void ResumeSounds (void);
		void StopAll (void);

		int16_t XlatSound (int16_t nSound);
		int32_t UnXlatSound (int32_t nSound);

		inline int32_t Format (void) { return m_info.nFormat; }
		inline void SetFormat (int32_t nFormat) { m_info.nFormat = nFormat; }
		inline int32_t Available (void) { return m_info.bAvailable; }
		inline int32_t ActiveObjects (void) { 
			m_info.nActiveObjects = 0;
			for (int32_t i = 0; i < int32_t (m_objects.ToS ()); i++)
				if (m_objects [i].m_channel >= 0)
					m_info.nActiveObjects++;
				return m_info.nActiveObjects; 
			}
		inline int32_t MaxChannels (void) { return m_info.nMaxChannels; }
		inline int32_t FreeChannel (void) { return m_info.nFreeChannel; }
		inline CAudioChannel* Channel (uint32_t i = 0) { return (i < (uint32_t) MaxChannels ()) ? m_channels + i : NULL; }
		inline CStack<CSoundObject>& Objects (void) { return m_objects; }
		inline float SlowDown (void) { return m_info.fSlowDown; }
		inline void ActivateObject (void) { m_info.nActiveObjects++; }
		inline void DeactivateObject (void) { m_info.nActiveObjects--; }
		int32_t RegisterChannel (CAudioChannel* pChannel);
		void UnregisterChannel (int32_t nIndex);

		void RecordSoundObjects (void);
#if DBG
		int32_t VerifyChannelFree (int32_t channel);
#endif
		static void _CDECL_ MixCallback (void* userdata, Uint8* stream, int32_t len);

		inline bool HaveRouter (int32_t nThread) { return m_router [nThread].Size () > 0; }
		int32_t Distance (CFixVector& vListenerPos, int16_t nListenerSeg, CFixVector& vSoundPos, int16_t nSoundSeg, fix maxDistance, int32_t nDecay, CFixVector& vecToSound, int32_t nThread = 0);

		void Update (void) { 
			for (int32_t i = 0; i < MAX_THREADS; i++) {
				m_router [i].SetStartSeg (-1); 
				m_router [i].SetDestSeg (-1);
				}
			}

	private:
		CAudioChannel* FindFreeChannel (int32_t nSoundClass);
	};

extern CAudio audio;

void Mix_VolPan (int32_t nChannel, int32_t nVolume, int32_t nPan);
void SetSoundSources (void);
void SetD1Sound (void);

//------------------------------------------------------------------------------

typedef struct tWAVHeader {
	char		chunkID [4];
	uint32_t		chunkSize;
	char		riffType [4];
} tRIFFChunk;

typedef struct tWAVFormat {
	char		chunkID [4];
	uint32_t		chunkSize;
	uint16_t	format;
	uint16_t	channels;
	uint32_t		sampleRate;
	uint32_t		avgBytesPerSec;
	uint16_t	blockAlign;
	uint16_t	bitsPerSample;
	} tPCMFormatChunk;

typedef struct tWAVData {
	char		chunkID [4];
	uint32_t		chunkSize;
} tWAVData;

typedef struct tWAVInfo {
	tWAVHeader	header;
	tWAVFormat	format;
	tWAVData		data;
} tWAVInfo;

//------------------------------------------------------------------------------

#endif

