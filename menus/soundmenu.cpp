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
#include "descent.h"
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
#include "cockpit.h"
#include "strutil.h"
#include "reorder.h"
#include "rendermine.h"
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
#include "midi.h"
#include "songs.h"

extern tDetailData detailData;

//------------------------------------------------------------------------------

static struct {
	int	nDigiVol;
	int	nMusicVol;
	int	nRedbook;
	int	nVolume;
	int	nGatling;
	int	nChannels;
} soundOpts;

static const char* pszLowMediumHigh [3];

//------------------------------------------------------------------------------

void SetRedbookVolume (int volume);

//------------------------------------------------------------------------------

int SoundChannelIndex (void)
{
	int	h, i;

for (h = (int) sizeofa (detailData.nSoundChannels), i = 0; i < h; i++)
	if (audio.MaxChannels () < detailData.nSoundChannels [i])
		break;
return i - 1;
}

//------------------------------------------------------------------------------

int SoundMenuCallback (CMenu& menu, int& nKey, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

if (soundOpts.nChannels >= 0) {
	if (gameStates.sound.nSoundChannels != menu [soundOpts.nChannels].m_value + 2) {
		gameStates.sound.nSoundChannels = menu [soundOpts.nChannels].m_value + 2;
		sprintf (menu [soundOpts.nChannels].m_text, TXT_SOUND_CHANNEL_COUNT, pszLowMediumHigh [gameStates.sound.nSoundChannels - 2]);
		menu [soundOpts.nChannels].m_bRebuild = 1;
		}
	}

if (soundOpts.nGatling >= 0) {
	if (gameOpts->sound.bGatling != menu [soundOpts.nGatling].m_value) {
		gameOpts->sound.bGatling = menu [soundOpts.nGatling].m_value;
		nKey = -2;
		}
	}
if (gameConfig.nDigiVolume != menu [soundOpts.nDigiVol].m_value) {
	gameConfig.nDigiVolume = menu [soundOpts.nDigiVol].m_value;
	audio.SetFxVolume ((gameConfig.nDigiVolume * 32768) / 8);
	audio.PlaySound (SOUND_DROP_BOMB);
	}

if (menu [soundOpts.nRedbook].m_value != redbook.Enabled ()) {
	if (menu [soundOpts.nRedbook].m_value && !gameOpts->sound.bUseRedbook) {
		MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_REDBOOK_DISABLED);
		menu [soundOpts.nRedbook].m_value = 0;
		menu [soundOpts.nRedbook].m_bRebuild = 1;
		}
	else {
		redbook.Enable (menu [soundOpts.nRedbook].m_value);
		if (redbook.Enabled ()) {
			if (gameStates.app.nFunctionMode == FMODE_MENU)
				songManager.Play (SONG_TITLE, 1);
			else if (gameStates.app.nFunctionMode == FMODE_GAME)
				songManager.PlayLevelSong (gameData.missions.nCurrentLevel, gameStates.app.bGameRunning);
			else
				Int3 ();

			if (menu [soundOpts.nRedbook].m_value && !redbook.Playing ()) {
				redbook.Enable (0);
				gameStates.menus.nInMenu = 0;
				MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_MUSIC_NOCD);
				gameStates.menus.nInMenu = 1;
				menu [soundOpts.nRedbook].m_value = 0;
				menu [soundOpts.nRedbook].m_bRebuild = 1;
				}
			}
		}
	menu [soundOpts.nMusicVol].SetText (redbook.Enabled () ? const_cast<char*> (TXT_CD_VOLUME) : const_cast<char*> (TXT_MIDI_VOLUME));
	menu [soundOpts.nMusicVol].m_bRebuild = 1;
	}

if (redbook.Enabled ()) {
	if (gameConfig.nRedbookVolume != menu [soundOpts.nMusicVol].m_value)   {
		gameConfig.nRedbookVolume = menu [soundOpts.nMusicVol].m_value;
		redbook.SetVolume (gameConfig.nRedbookVolume);
		}
	}
else {
	if (gameConfig.nMidiVolume != menu [soundOpts.nMusicVol].m_value) {
		int bSongPlaying = (gameConfig.nMidiVolume > 0);

		if (gameConfig.nMidiVolume * menu [soundOpts.nMusicVol].m_value == 0) //=> midi gets either turned on or off
			nKey = -2;
 		gameConfig.nMidiVolume = menu [soundOpts.nMusicVol].m_value;
		if (gameConfig.nMidiVolume < 1) {
			midi.PlaySong (NULL, NULL, NULL, 1, 0);	// fade out first
			midi.SetVolume (128 * gameConfig.nMidiVolume / 8);
			}	
		else {
			midi.SetVolume (128 * gameConfig.nMidiVolume / 8);
			if (!bSongPlaying) {
			//audio.StopAllChannels ();
				if (gameStates.app.bGameRunning)
					songManager.PlayLevelSong (gameData.missions.nCurrentLevel ? gameData.missions.nCurrentLevel : 1, 1);
				else
					songManager.Play (SONG_TITLE, 1);
				}
			}
		}
	}
// don't enable redbook for a non-apple demo version of the shareware demo
return nCurItem;		//kill warning
}

//------------------------------------------------------------------------------

void SoundMenu (void)
{
	static int choice = 0;
	char szSlider [50];

	CMenu	m;
#if 0
	char	szVolume [50];
#endif
	int	i;
	int	optReverse, optShipSound = -1, optMissileSound = -1, optSpeedUpSound = -1, optFadeMusic = -1, optShieldWarn = -1;
	int	bSongPlaying = (gameConfig.nMidiVolume > 0);

pszLowMediumHigh [0] = TXT_LOW;
pszLowMediumHigh [1] = TXT_MEDIUM;
pszLowMediumHigh [2] = TXT_HIGH;

gameStates.sound.nSoundChannels = SoundChannelIndex ();
do {
	m.Destroy ();
	m.Create (20);
	soundOpts.nGatling = -1;
	if (gameOpts->app.bNotebookFriendly) {
		sprintf (szSlider + 1, TXT_SOUND_CHANNEL_COUNT, pszLowMediumHigh [gameStates.sound.nSoundChannels - 2]);
		*szSlider = *(TXT_SOUND_CHANNEL_COUNT - 1);
		soundOpts.nChannels = m.AddSlider (szSlider + 1, gameStates.sound.nSoundChannels - 2, 0, 2, KEY_C, HTX_SOUND_CHANNEL_COUNT);  
		}
	else
		soundOpts.nChannels = -1;
	soundOpts.nDigiVol = m.AddSlider (TXT_FX_VOLUME, gameConfig.nDigiVolume, 0, 8, KEY_F, HTX_ONLINE_MANUAL);
	soundOpts.nMusicVol = m.AddSlider (redbook.Enabled () ? TXT_CD_VOLUME : TXT_MIDI_VOLUME, 
												  redbook.Enabled () ? gameConfig.nRedbookVolume : gameConfig.nMidiVolume, 
												  0, 8, KEY_M, HTX_ONLINE_MANUAL);
	if (gameStates.app.bNostalgia) 
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
		optShieldWarn = m.AddCheck (TXT_SHIELD_WARNING, gameOpts->gameplay.bShieldWarning, KEY_W, HTX_CPIT_SHIELDWARN);
		}
	m.AddText ("", 0);
	soundOpts.nRedbook = m.AddCheck (TXT_REDBOOK_ENABLED, redbook.Playing (), KEY_C, HTX_ONLINE_MANUAL);
	optReverse = m.AddCheck (TXT_REVERSE_STEREO, gameConfig.bReverseChannels, KEY_R, HTX_ONLINE_MANUAL);
#if 1
	if (redbook.Enabled () || !gameConfig.nMidiVolume)
		optFadeMusic = -1;
	else
		optFadeMusic = m.AddCheck (TXT_FADE_MUSIC, gameOpts->sound.bFadeMusic, KEY_F, HTX_FADE_MUSIC);
#endif
	i = m.Menu (NULL, TXT_SOUND_OPTS, SoundMenuCallback, &choice);
	redbook.Enable (m [soundOpts.nRedbook].m_value);
	gameConfig.bReverseChannels = m [optReverse].m_value;
} while (i == -2);
if (gameConfig.nMidiVolume < 1)
	midi.PlaySong (NULL, NULL, NULL, 0, 0);
else if (!bSongPlaying)
	songManager.PlayCurrent (1);
if (!gameStates.app.bNostalgia) {
	GET_VAL (gameOpts->sound.bFadeMusic, optFadeMusic);
	GET_VAL (gameOpts->sound.bShip, optShipSound);
	GET_VAL (gameOpts->sound.bMissiles, optMissileSound);
	GET_VAL (gameOpts->sound.bGatling, soundOpts.nGatling);
	GET_VAL (extraGameInfo [0].bGatlingSpeedUp, optSpeedUpSound);
	GET_VAL (gameOpts->gameplay.bShieldWarning, optShieldWarn);
	if (gameStates.app.bGameRunning && !(gameOpts->sound.bShip && gameOpts->sound.bGatling))
		audio.DestroyObjectSound (LOCALPLAYER.nObject);
	}
gameOpts->sound.xCustomSoundVolume = fix (float (gameConfig.nDigiVolume) * 10.0f / 8.0f + 0.5f);
audio.SetMaxChannels (32 << (gameStates.sound.nSoundChannels - 2));
}

//------------------------------------------------------------------------------
//eof
