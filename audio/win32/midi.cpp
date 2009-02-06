// MIDI stuff follows.
#include <stdio.h>

#include "descent.h"
#include "error.h"
#include "hmpfile.h"
#include "audio.h"
#include "songs.h"
#include "midi.h"

CMidi midi;

//------------------------------------------------------------------------------

void CMidi::Init (void)
{
m_nVolume [0] = 255;
m_nPaused = 0;
m_music = NULL;
m_hmp = NULL;
}

//------------------------------------------------------------------------------

void CMidi::Shutdown (void)
{
if (m_hmp) {
	hmp_close (m_hmp);
	m_hmp = NULL;
	songManager.SetPlaying (0);
	}
Fadeout ();
}

//------------------------------------------------------------------------------

void CMidi::Fadeout (void)
{
#if USE_SDL_MIXER
if (!audio.Available ()) 
	return;
if (gameOpts->sound.bUseSDLMixer) {
	if (gameOpts->sound.bFadeMusic) {
		while (!Mix_FadeOutMusic (300) && Mix_PlayingMusic ())
			SDL_Delay (30);		
		while (Mix_PlayingMusic ())
			SDL_Delay (1);		
		}
	int nVolume = m_nVolume;
	SetVolume (0);
	m_nVolume = nVolume;
	Mix_HaltMusic ();
	Mix_FreeMusic (m_music);
	m_music = NULL;
	}
#endif
}

//------------------------------------------------------------------------------

int CMidi::SetVolume (int nVolume [0])
{
#if (defined (_WIN32) || USE_SDL_MIXER)
	int nLastVolume = m_nVolume [0];

if (nVolume [0] < 0)
	m_nVolume [0] = 0;
else if (nVolume [0] > 127)
	m_nVolume [0] = 127;
else
	m_nVolume [0] = nVolume [0];

#	if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer)
	Mix_VolumeMusic (m_nVolume [0]);
#	endif
#	if defined (_WIN32)
#	if USE_SDL_MIXER
else 
#	endif
if (m_hmp) {
	int mmVolume;

	// scale up from 0-127 to 0-0xffff
	mmVolume = (m_nVolume [0] << 1) | (m_nVolume [0] & 1);
	mmVolume |= (mmVolume << 8);
	nVolume [0] = midiOutSetVolume ((HMIDIOUT)m_hmp->hmidi, mmVolume | (mmVolume << 16));
	}
#	endif
return nLastVolume;
#else
return 0;
#endif
}

//------------------------------------------------------------------------------

int CMidi::PlaySong (char* pszSong, char* melodicBank, char* drumBank, int bLoop, int bD1Song)
{
#if (defined (_WIN32) || USE_SDL_MIXER)
	int	bCustom;

PrintLog ("DigiPlayMidiSong (%s)\n", pszSong);
audio.StopCurrentSong ();
if (!(pszSong && *pszSong))
	return 0;
if (m_nVolume [0] < 1)
	return 0;
bCustom = ((strstr (pszSong, ".mp3") != NULL) || (strstr (pszSong, ".ogg") != NULL));
if (!(bCustom || (m_hmp = hmp_open (pszSong, bD1Song))))
	return 0;
SetVolume (m_nVolume);
#	if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer) {
	char	fnSong [FILENAME_LEN], *pfnSong;

	if (bCustom) {
		pfnSong = pszSong;
		if (strstr (pszSong, ".mp3") && !songManager.MP3 ()) {
			audio.Shutdown ();
			songManager.SetMP3 (1);
			audio.Setup (1);
			}
		}
	else {
		if (!strstr (pszSong, ".mp3") && songManager.MP3 ()) {
			audio.Shutdown ();
			songManager.SetMP3 (0);
			audio.Setup (1);
			}
#if defined (_WIN32)
		sprintf (fnSong, "%s/d2x-temp.mid", *gameFolders.szCacheDir ? gameFolders.szCacheDir : gameFolders.szHomeDir);
#else
		sprintf (fnSong, "%s/d2x-temp.mid", gameFolders.szHomeDir);
#endif
		if (!hmp_to_midi (m_hmp, fnSong)) {
			PrintLog ("SDL_mixer failed to load %s\n(%s)\n", fnSong, Mix_GetError ());
			return 0;
			}
		pfnSong = fnSong;
		}
	if (!(m_music = Mix_LoadMUS (pfnSong))) {
		PrintLog ("SDL_mixer failed to load %s\n(%s)\n", fnSong, Mix_GetError ());
		return 0;
		}
	if (-1 == Mix_FadeInMusicPos (m_music, bLoop ? -1 : 1, songManager.Pos () ? 1000 : 1500, (double) songManager.Pos () / 1000.0)) {
		PrintLog ("SDL_mixer cannot play %s\n(%s)\n", pszSong, Mix_GetError ());
		songManager.SetPos (0);
		return 0;
		}
	PrintLog ("SDL_mixer playing %s\n", pszSong);
	if (songManager.Pos ())
		songManager.SetPos (0);
	else
		songManager.SetStart (SDL_GetTicks ());
	
	songManager.SetPlaying (1);
	SetVolume (m_nVolume [0]);
	return 1;
	}
#	endif
#	if defined (_WIN32)
if (bCustom) {
	PrintLog ("Cannot play %s - enable SDL_mixer\n", pszSong);
	return 0;
	}
hmp_play (m_hmp, bLoop);
songManager.SetPlaying (1);
SetVolume (m_nVolume [0]);
#	endif
#endif
return 1;
}

//------------------------------------------------------------------------------

void CMidi::Pause (void)
{
#if 0
	if (!gameStates.sound.audio.bInitialized)
		return;
#endif

if (!m_nPaused) {
#if USE_SDL_MIXER
	if (gameOpts->sound.bUseSDLMixer)
		Mix_PauseMusic ();
#endif
	}
m_nPaused++;
}

//------------------------------------------------------------------------------

void CMidi::Resume (void)
{
Assert(m_nPaused > 0);
if (m_nPaused == 1) {
#if USE_SDL_MIXER
	if (gameOpts->sound.bUseSDLMixer)
		Mix_ResumeMusic ();
#endif
	}
m_nPaused--;
}

//------------------------------------------------------------------------------
