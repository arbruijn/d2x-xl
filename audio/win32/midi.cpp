#include <stdio.h>

#include "descent.h"
#include "error.h"
#include "hmpfile.h"
#include "audio.h"
#include "songs.h"
#include "config.h"
#include "midi.h"

CMidi midi;

//------------------------------------------------------------------------------

void CMidi::Init (void)
{
m_nVolume = 255;
m_nPaused = 0;
m_music = NULL;
m_hmp = NULL;
}

//------------------------------------------------------------------------------

void CMidi::Shutdown (void)
{
if (gameStates.sound.audio.bNoMusic)
	return;

#if (defined (_WIN32) || USE_SDL_MIXER)
if (m_hmp) {
	hmp_close (m_hmp);
	m_hmp = NULL;
	songManager.SetPlaying (0);
	}
#endif
Fadeout ();
}

//------------------------------------------------------------------------------

void CMidi::Fadeout (void)
{
if (gameStates.sound.audio.bNoMusic)
	return;

#if USE_SDL_MIXER
if (!audio.Available ()) 
	return;
if (gameOpts->sound.bUseSDLMixer) {
	if (gameOpts->sound.bFadeMusic) {
		Mix_FadeOutMusic (300);
		SDL_Delay (330);
#if 0
		while (Mix_PlayingMusic ())
			SDL_Delay (1);		
#endif
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

int CMidi::SetVolume (int nVolume)
{
if (gameStates.sound.audio.bNoMusic)
	return 0;

#if (defined (_WIN32) || USE_SDL_MIXER)
	int nLastVolume = m_nVolume;

if (nVolume < 0)
	m_nVolume = 0;
else if (nVolume > 127)
	m_nVolume = 127;
else
	m_nVolume = nVolume;

#	if USE_SDL_MIXER
if (gameOpts->sound.bUseSDLMixer)
	Mix_VolumeMusic (m_nVolume);
#	endif
#	if defined (_WIN32)
#		if USE_SDL_MIXER
else 
#		endif
if (m_hmp) {
	// scale up from 0-127 to 0-0xffff
	nVolume = 65535 * m_nVolume / 128;
	midiOutSetVolume ((HMIDIOUT) m_hmp->hmidi, nVolume | (nVolume << 16));
	}
if ((songManager.Playing () < 0) || !nVolume)
	FixVolume (m_nVolume);
#	endif
return nLastVolume;
#else
return 0;
#endif
}

//------------------------------------------------------------------------------

void CMidi::FixVolume (int nVolume)
{
#ifdef _WIN32
if (gameStates.sound.bMidiFix && (songManager.Playing () <= 0)) {
	HMIDIOUT hMIDI;
	midiOutOpen (&hMIDI, -1, NULL, NULL, CALLBACK_NULL);
	nVolume = 65535; // * nVolume / 128;
	midiOutSetVolume (hMIDI, nVolume | (nVolume << 16));
	midiOutClose (hMIDI);
	}
#endif
}

//------------------------------------------------------------------------------

int CMidi::PlaySong (char* pszSong, char* melodicBank, char* drumBank, int bLoop, int bD1Song)
{
if (gameStates.sound.audio.bNoMusic)
	return 0;

#if (defined (_WIN32) || USE_SDL_MIXER)
	int	bCustom;

PrintLog (1, "DigiPlayMidiSong (%s)\n", pszSong);
audio.StopCurrentSong ();
if (!(pszSong && *pszSong)) {
	PrintLog (-1);
	return 0;
	}
if (m_nVolume < 1) {
	PrintLog (-1);
	return 0;
	}

bCustom = ((strstr (pszSong, ".mp3") != NULL) || (strstr (pszSong, ".ogg") != NULL) || strstr (pszSong, ".flac"));
if (bCustom) {
	if (audio.Format () != AUDIO_S16SYS) {
		audio.Shutdown ();
		audio.Setup (1, AUDIO_S16SYS);
		}
	}
else if (!(m_hmp = hmp_open (pszSong, bD1Song)))
	return 0;

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
		sprintf (fnSong, "%sd2x-temp.mid", *gameFolders.var.szCache ? gameFolders.var.szCache : gameFolders.user.szCache);
#else
		sprintf (fnSong, "%sd2x-temp.mid", *gameFolders.var.szCache ? gameFolders.var.szCache : gameFolders.user.szCache);
#endif
		if (!hmp_to_midi (m_hmp, fnSong)) {
			PrintLog (0, "SDL_mixer failed to load %s\n(%s)\n", fnSong, Mix_GetError ());
			return 0;
			}
		pfnSong = fnSong;
		}
	try {
		m_music = Mix_LoadMUS (pfnSong);
		}
	catch (...) {	// critical problem in midi playback -> turn it off
		SetVolume (gameConfig.nMidiVolume = 0);
		}
	if (!m_music) {
		PrintLog (0, "SDL_mixer failed to load %s\n(%s)\n", fnSong, Mix_GetError ());
		return 0;
		}
	if (-1 == Mix_FadeInMusicPos (m_music, bLoop ? -1 : 1, !gameOpts->sound.bFadeMusic ? 0 : songManager.Pos () ? 1000 : 1500, (double) songManager.Pos () / 1000.0)) {
		PrintLog (0, "SDL_mixer cannot play %s\n(%s)\n", pszSong, Mix_GetError ());
		songManager.SetPos (0);
		return 0;
		}
	PrintLog (0, "SDL_mixer playing %s\n", pszSong);
	if (songManager.Pos ())
		songManager.SetPos (0);
	else
		songManager.SetStart (SDL_GetTicks ());
	
	songManager.SetPlaying (bCustom ? -1 : 1);
	SetVolume (m_nVolume);
	return songManager.Playing ();
	}
#	endif
#	if defined (_WIN32)
if (bCustom) {
	PrintLog (0, "Cannot play %s - enable SDL_mixer\n", pszSong);
	return 0;
	}
hmp_play (m_hmp, bLoop);
songManager.SetPlaying (1);
SetVolume (m_nVolume);
#	endif
#endif
PrintLog (-1);
return 1;
}

//------------------------------------------------------------------------------

void CMidi::Pause (void)
{
if (gameStates.sound.audio.bNoMusic)
	return;

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
if (gameStates.sound.audio.bNoMusic)
	return;

if (m_nPaused == 1) {
#if USE_SDL_MIXER
	if (gameOpts->sound.bUseSDLMixer)
		Mix_ResumeMusic ();
#endif
	}
m_nPaused--;
}

//------------------------------------------------------------------------------
