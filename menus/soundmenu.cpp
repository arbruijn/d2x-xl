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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "inferno.h"
#include "ipx.h"
#include "key.h"
#include "iff.h"
#include "u_mem.h"
#include "error.h"
#include "screens.h"
#include "joy.h"
#include "slew.h"
#include "args.h"
#include "hogfile.h"
#include "newdemo.h"
#include "timer.h"
#include "text.h"
#include "gamefont.h"
#include "menu.h"
#include "network.h"
#include "network_lib.h"
#include "netmenu.h"
#include "scores.h"
#include "joydefs.h"
#include "playsave.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "state.h"
#include "movie.h"
#include "gamepal.h"
#include "gauges.h"
#include "strutil.h"
#include "reorder.h"
#include "render.h"
#include "light.h"
#include "lightmap.h"
#include "autodl.h"
#include "tracker.h"
#include "omega.h"
#include "lightning.h"
#include "vers_id.h"
#include "input.h"
#include "collide.h"
#include "objrender.h"
#include "sparkeffect.h"
#include "renderthreads.h"
#include "soundthreads.h"
#include "menubackground.h"
#ifdef EDITOR
#	include "editor/editor.h"
#endif

extern tDetailData detailData;

//------------------------------------------------------------------------------

static struct {
	int	nDigiVol;
	int	nMusicVol;
	int	nRedbook;
	int	nChannels;
	int	nVolume;
	int	nGatling;
} soundOpts;

//------------------------------------------------------------------------------

void SetRedbookVolume (int volume);

//------------------------------------------------------------------------------

int SoundChannelIndex (void)
{
	int	h, i;

for (h = (int) sizeofa (detailData.nSoundChannels), i = 0; i < h; i++)
	if (m_info.nMaxChannels < detailData.nSoundChannels [i])
		break;
return i - 1;
}

//------------------------------------------------------------------------------

int SoundMenuCallback (CMenu& menu, int& nKey, int nCurItem)
{
if (gameOpts->sound.bGatling != menu [soundOpts.nGatling].m_value) {
	gameOpts->sound.bGatling = menu [soundOpts.nGatling].m_value;
	nKey = -2;
	}
if (gameConfig.nDigiVolume != menu [soundOpts.nDigiVol].m_value) {
	gameConfig.nDigiVolume = menu [soundOpts.nDigiVol].m_value;
	audio.SetFxVolume ((gameConfig.nDigiVolume * 32768) / 8);
	audio.PlaySound (SOUND_DROP_BOMB);
	}
if ((soundOpts.nChannels >= 0) && (gameStates.sound.nSoundChannels != menu [soundOpts.nChannels].m_value)) {
	gameStates.sound.nSoundChannels = menu [soundOpts.nChannels].m_value;
	m_info.nMaxChannels = detailData.nSoundChannels [gameStates.sound.nSoundChannels];
	sprintf (menu [soundOpts.nChannels].m_text, TXT_SOUND_CHANNEL_COUNT, m_info.nMaxChannels);
	menu [soundOpts.nChannels].m_bRebuild = 1;
	}
if ((soundOpts.nVolume >= 0) && (gameOpts->sound.xCustomSoundVolume != menu [soundOpts.nVolume].m_value)) {
	if (!gameOpts->sound.xCustomSoundVolume || !menu [soundOpts.nVolume].m_value)
		nKey = -2;
	else
		menu [soundOpts.nVolume].m_bRebuild = 1;
	gameOpts->sound.xCustomSoundVolume = menu [soundOpts.nVolume].m_value;
	sprintf (menu [soundOpts.nVolume].m_text, TXT_CUSTOM_SOUNDVOL, gameOpts->sound.xCustomSoundVolume * 10, '%');
	return nCurItem;
	}
if (menu [soundOpts.nRedbook].m_value != gameStates.sound.bRedbookEnabled) {
	if (menu [soundOpts.nRedbook].m_value && !gameOpts->sound.bUseRedbook) {
		MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_REDBOOK_DISABLED);
		menu [soundOpts.nRedbook].m_value = 0;
		menu [soundOpts.nRedbook].m_bRebuild = 1;
		}
	else if ((gameStates.sound.bRedbookEnabled = menu [soundOpts.nRedbook].m_value)) {
		if (gameStates.app.nFunctionMode == FMODE_MENU)
			songManager.Play (SONG_TITLE, 1);
		else if (gameStates.app.nFunctionMode == FMODE_GAME)
			songManager.PlayLevel (gameData.missions.nCurrentLevel, gameStates.app.bGameRunning);
		else
			Int3 ();

		if (menu [soundOpts.nRedbook].m_value && !gameStates.sound.bRedbookPlaying) {
			gameStates.sound.bRedbookEnabled = 0;
			gameStates.menus.nInMenu = 0;
			MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_MUSIC_NOCD);
			gameStates.menus.nInMenu = 1;
			menu [soundOpts.nRedbook].m_value = 0;
			menu [soundOpts.nRedbook].m_bRebuild = 1;
			}
		}
	menu [soundOpts.nMusicVol].m_text = gameStates.sound.bRedbookEnabled ? const_cast<char*> (TXT_CD_VOLUME) : const_cast<char*> (TXT_MIDI_VOLUME);
	menu [soundOpts.nMusicVol].m_bRebuild = 1;
	}

if (gameStates.sound.bRedbookEnabled) {
	if (gameConfig.nRedbookVolume != menu [soundOpts.nMusicVol].m_value)   {
		gameConfig.nRedbookVolume = menu [soundOpts.nMusicVol].m_value;
		SetRedbookVolume (gameConfig.nRedbookVolume);
		}
	}
else {
	if (gameConfig.nMidiVolume != menu [soundOpts.nMusicVol].m_value) {
		int bSongPlaying = (gameConfig.nMidiVolume > 0);

		if (gameConfig.nMidiVolume * menu [soundOpts.nMusicVol].m_value == 0) //=> midi gets either turned on or off
			nKey = -2;
 		gameConfig.nMidiVolume = menu [soundOpts.nMusicVol].m_value;
		audio.SetMidiVolume (128 * gameConfig.nMidiVolume / 8);
		if (gameConfig.nMidiVolume < 1)
			audio.PlayMidiSong (NULL, NULL, NULL, 1, 0);
		else if (!bSongPlaying) {
			//audio.StopAllSounds ();
			if (gameStates.app.bGameRunning)
				songManager.PlayLevel (gameData.missions.nCurrentLevel ? gameData.missions.nCurrentLevel : 1, 1);
			else
				songManager.Play (SONG_TITLE, 1);
			}
		}
	}
// don't enable redbook for a non-apple demo version of the shareware demo
return nCurItem;		//kill warning
}

//------------------------------------------------------------------------------

void SoundMenu (void)
{
   CMenu	m;
	char	szChannels [50], szVolume [50];
	int	i, choice = 0, 
			optReverse, optShipSound = -1, optMissileSound = -1, optSpeedUpSound = -1, optFadeMusic = -1, 
			bSongPlaying = (gameConfig.nMidiVolume > 0);

gameStates.sound.nSoundChannels = SoundChannelIndex ();
do {
	m.Destroy ();
	m.Create (20);
	soundOpts.nGatling = -1;
	soundOpts.nDigiVol = m.AddSlider (TXT_FX_VOLUME, gameConfig.nDigiVolume, 0, 8, KEY_F, HTX_ONLINE_MANUAL);
	soundOpts.nMusicVol = m.AddSlider (gameStates.sound.bRedbookEnabled ? TXT_CD_VOLUME : TXT_MIDI_VOLUME, 
												  gameStates.sound.bRedbookEnabled ? gameConfig.nRedbookVolume : gameConfig.nMidiVolume, 
												  0, 8, KEY_M, HTX_ONLINE_MANUAL);
	if (gameStates.app.bGameRunning || gameStates.app.bNostalgia)
		soundOpts.nChannels = -1;
	else {
		sprintf (szChannels + 1, TXT_SOUND_CHANNEL_COUNT, m_info.nMaxChannels);
		*szChannels = *(TXT_SOUND_CHANNEL_COUNT - 1);
		soundOpts.nChannels = m.AddSlider (szChannels + 1, gameStates.sound.nSoundChannels, 0, 
													  (int) sizeofa (detailData.nSoundChannels) - 1, KEY_C, HTX_SOUND_CHANNEL_COUNT);  
		}
	if (!gameStates.app.bNostalgia) {
		sprintf (szVolume + 1, TXT_CUSTOM_SOUNDVOL, gameOpts->sound.xCustomSoundVolume * 10, '%');
		*szVolume = *(TXT_CUSTOM_SOUNDVOL - 1);
		soundOpts.nVolume = m.AddSlider (szVolume + 1, gameOpts->sound.xCustomSoundVolume, 0, 10, KEY_C, HTX_CUSTOM_SOUNDVOL);  
		}
	if (gameStates.app.bNostalgia || !gameOpts->sound.xCustomSoundVolume) 
		optShipSound = 
		optMissileSound =
		optSpeedUpSound =
		soundOpts.nGatling = -1;
	else {
		m.AddText ("", 0);
		optShipSound = m.AddCheck (TXT_SHIP_SOUND, gameOpts->sound.bShip, KEY_S, HTX_SHIP_SOUND);
		optMissileSound = m.AddCheck (TXT_MISSILE_SOUND, gameOpts->sound.bMissiles, KEY_M, HTX_MISSILE_SOUND);
		soundOpts.nGatling = m.AddCheck (TXT_GATLING_SOUND, gameOpts->sound.bGatling, KEY_G, HTX_GATLING_SOUND);
		if (gameOpts->sound.bGatling)
			optSpeedUpSound = m.AddCheck (TXT_SPINUP_SOUND, extraGameInfo [0].bGatlingSpeedUp, KEY_U, HTX_SPINUP_SOUND);
		}
	m.AddText ("", 0);
	soundOpts.nRedbook = m.AddCheck (TXT_REDBOOK_ENABLED, gameStates.sound.bRedbookPlaying, KEY_C, HTX_ONLINE_MANUAL);
	optReverse = m.AddCheck (TXT_REVERSE_STEREO, gameConfig.bReverseChannels, KEY_R, HTX_ONLINE_MANUAL);
	if (gameStates.sound.bRedbookEnabled || !gameConfig.nMidiVolume)
		optFadeMusic = -1;
	else
		optFadeMusic = m.AddCheck (TXT_FADE_MUSIC, gameOpts->sound.bFadeMusic, KEY_F, HTX_FADE_MUSIC);
	i = m.Menu (NULL, TXT_SOUND_OPTS, SoundMenuCallback, &choice);
	gameStates.sound.bRedbookEnabled = m [soundOpts.nRedbook].m_value;
	gameConfig.bReverseChannels = m [optReverse].m_value;
} while (i == -2);
if (gameConfig.nMidiVolume < 1)
	audio.PlayMidiSong (NULL, NULL, NULL, 0, 0);
else if (!bSongPlaying)
	songManager.Play (m_info.nCurrent, 1);
if (!gameStates.app.bNostalgia) {
	GET_VAL (gameOpts->sound.bFadeMusic, optFadeMusic);
	GET_VAL (gameOpts->sound.bShip, optShipSound);
	GET_VAL (gameOpts->sound.bMissiles, optMissileSound);
	GET_VAL (gameOpts->sound.bGatling, soundOpts.nGatling);
	GET_VAL (extraGameInfo [0].bGatlingSpeedUp, optSpeedUpSound);
	if (gameStates.app.bGameRunning && !(gameOpts->sound.bShip && gameOpts->sound.bGatling))
		audio.DestroyObjectSound (LOCALPLAYER.nObject);
	}
}

//------------------------------------------------------------------------------
//eof
