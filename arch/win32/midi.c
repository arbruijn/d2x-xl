// MIDI stuff follows.
#include <stdio.h>

#include "digi.h"
#include "cfile.h"
#include "error.h"
#include "hmpfile.h"
#include "inferno.h"

#if USE_SDL_MIXER
#	ifdef __macosx__
#		include <SDL/SDL_mixer.h>
#	else
#		include <SDL_mixer.h>
#	endif

Mix_Music *mixMusic = NULL;
#endif

hmp_file *hmp = NULL;

int midiVolume = 255;
int bDigiMidiSongPlaying = 0;

//------------------------------------------------------------------------------

void DigiSetMidiVolume(int n)
{
if (n < 0)
	midiVolume = 0;
else if (n > 127)
	midiVolume = 127;
else
	midiVolume = n;

#if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer)
	Mix_VolumeMusic (midiVolume);
#endif
#if defined (_WIN32)
#	if USE_SDL_MIXER
else 
#	endif
if (hmp) {
	int mmVolume;

	// scale up from 0-127 to 0-0xffff
	mmVolume = (midiVolume << 1) | (midiVolume & 1);
	mmVolume |= (mmVolume << 8);
	n = midiOutSetVolume((HMIDIOUT)hmp->hmidi, mmVolume | (mmVolume << 16));
	}
#endif
}

//------------------------------------------------------------------------------

void DigiStopCurrentSong()
{
if (bDigiMidiSongPlaying) {
	int h = midiVolume;	// preserve it for another song being started
	DigiSetMidiVolume(0);
	midiVolume = h;

#if USE_SDL_MIXER
	if (gameOpts->sound.bUseSDLMixer) {
		Mix_HaltMusic ();
		Mix_FreeMusic (mixMusic);
		mixMusic = NULL;
		}
#endif
#if defined (_WIN32)
#	if USE_SDL_MIXER
else 
#	endif
		{
		hmp_close (hmp);
		hmp = NULL;
		bDigiMidiSongPlaying = 0;
		}
#endif
	}
}

//------------------------------------------------------------------------------

void DigiPlayMidiSong(char *filename, char *melodic_bank, char *drum_bank, int loop, int bD1Song)
{
#if 0
if (!gameStates.sound.digi.bInitialized)
	return;
#endif

DigiStopCurrentSong();
if (filename == NULL)
	return;
if (midiVolume < 1)
	return;
if (hmp = hmp_open (filename, bD1Song)) {
#if USE_SDL_MIXER
	if (gameOpts->sound.bUseSDLMixer) {
		char	fnMusic [FILENAME_LEN];

		sprintf (fnMusic, "%s%sd2x-temp.mid", gameFolders.szDataDir, *gameFolders.szDataDir ? "/" : "");
		if (hmp_to_midi (hmp, fnMusic) && (mixMusic = Mix_LoadMUS (fnMusic))) {
			if (Mix_PlayMusic (mixMusic, loop ? -1 : 1) != -1) {
				bDigiMidiSongPlaying = 1;
				DigiSetMidiVolume (midiVolume);
				}
			}
		}
#endif
#if defined (_WIN32)
#	if USE_SDL_MIXER
else 
#	endif
		{
		hmp_play(hmp, loop);
		bDigiMidiSongPlaying = 1;
		DigiSetMidiVolume(midiVolume);
		}
#endif
	}
}

//------------------------------------------------------------------------------

int sound_paused = 0;

void DigiPauseMidi()
{
#if 0
	if (!gameStates.sound.digi.bInitialized)
		return;
#endif

if (sound_paused == 0) {
#if USE_SDL_MIXER
	if (gameOpts->sound.bUseSDLMixer)
		Mix_PauseMusic ();
#endif
	}
sound_paused++;
}

//------------------------------------------------------------------------------

void DigiResumeMidi()
{
#if 0
	if (!gameStates.sound.digi.bInitialized)
		return;
#endif
	Assert(sound_paused > 0);
if (sound_paused == 1) {
#if USE_SDL_MIXER
	if (gameOpts->sound.bUseSDLMixer)
		Mix_ResumeMusic ();
#endif
	}
sound_paused--;
}

//------------------------------------------------------------------------------
