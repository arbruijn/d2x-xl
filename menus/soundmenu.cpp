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
#include "playerprofile.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "savegame.h"
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
	int   nAmbientVol;
	int	nMusicVol;
	int	nLinkVols;
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

CMenuItem*	m;
int			v;

if ((m = menu ["channels"])) {
	v = m->Value ();
	if (gameStates.sound.nSoundChannels != v + 2) {
		gameStates.sound.nSoundChannels = v + 2;
		sprintf (m->Text (), TXT_SOUND_CHANNEL_COUNT, pszLowMediumHigh [gameStates.sound.nSoundChannels - 2]);
		m->Rebuild ();
		}
	}

if ((m = menu ["gatling sound"])) {
	v = m->Value ();
	if (gameOpts->sound.bGatling != v) {
		gameOpts->sound.bGatling = v;
		nKey = -2;
		}
	}

if ((m = menu ["fx volume"])) {
	v = m->Value ();
	if (gameConfig.nAudioVolume [0] != v) {
		gameConfig.nAudioVolume [0] = v;
		if (gameOpts->sound.bLinkVolumes) {
			gameConfig.nAudioVolume [1] = gameConfig.nAudioVolume [0];
			audio.SetFxVolume ((gameConfig.nAudioVolume [1] * 32767) / 8, 1);
			}
		audio.SetFxVolume ((gameConfig.nAudioVolume [0] * 32767) / 8);
		audio.PlaySound (SOUND_DROP_BOMB);
		}
	}

if (!gameOpts->sound.bLinkVolumes && (m = menu ["ambient volume"])) {
	v = m->Value ();
	if (gameConfig.nAudioVolume [1] != v) {
		gameConfig.nAudioVolume [1] = v;
		audio.SetFxVolume ((gameConfig.nAudioVolume [1] * 32767) / 8, 1);
		}
	}

if ((m = menu ["link volumes"])) {
	v = m->Value ();
	if (gameOpts->sound.bLinkVolumes != v) {
		if (gameOpts->sound.bLinkVolumes = v) {
			gameConfig.nAudioVolume [1] = gameConfig.nAudioVolume [0];
			audio.SetFxVolume ((gameConfig.nAudioVolume [1] * 32767) / 8, 1);
			}
		nKey = -2;
		}
	}

if ((m = menu ["redbook sound"])) {
	v = m->Value ();
	if (redbook.Enabled () != v) {
		if (v && !gameOpts->sound.bUseRedbook) {
			MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_REDBOOK_DISABLED);
			menu [soundOpts.nRedbook].Value () = 0;
			menu [soundOpts.nRedbook].Rebuild ();
			}
		else {
			redbook.Enable (v);
			if (redbook.Enabled ()) {
				if (gameStates.app.nFunctionMode == FMODE_MENU)
					songManager.Play (SONG_TITLE, 1);
				else if (gameStates.app.nFunctionMode == FMODE_GAME)
					songManager.PlayLevelSong (missionManager.nCurrentLevel, gameStates.app.bGameRunning);
				else
					Int3 ();

				if (v && !redbook.Playing ()) {
					redbook.Enable (0);
					gameStates.menus.nInMenu = 0;
					MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_MUSIC_NOCD);
					gameStates.menus.nInMenu = 1;
					m->Value () = 0;
					m->Rebuild ();
					}
				}
			}
		if ((m = menu ["music volume"])) {
			m->SetText (redbook.Enabled () ? const_cast<char*> (TXT_CD_VOLUME) : const_cast<char*> (TXT_MIDI_VOLUME));
			m->Value () = redbook.Enabled () ? gameConfig.nRedbookVolume : gameConfig.nMidiVolume;
			m->Rebuild ();
			}
		}
	}	

if ((m = menu ["music volume"])) {
	v = m->Value ();
	if (redbook.Enabled ()) {
		if (gameConfig.nRedbookVolume != v) {
			gameConfig.nRedbookVolume = v;
			redbook.SetVolume (gameConfig.nRedbookVolume);
			}
		}
	else {
		if (gameConfig.nMidiVolume != v) {
			int bSongPlaying = (gameConfig.nMidiVolume > 0);

			if (gameConfig.nMidiVolume * v == 0) //=> midi gets either turned on or off
				nKey = -2;
			gameConfig.nMidiVolume = v;
			if (gameConfig.nMidiVolume < 1) {
				midi.PlaySong (NULL, NULL, NULL, 1, 0);	// fade out first
				midi.SetVolume (128 * gameConfig.nMidiVolume / 8);
				}	
			else {
				midi.SetVolume (128 * gameConfig.nMidiVolume / 8);
				if (!bSongPlaying) {
				//audio.StopAllChannels ();
					if (gameStates.app.bGameRunning)
						songManager.PlayLevelSong (missionManager.nCurrentLevel ? missionManager.nCurrentLevel : 1, 1);
					else
						songManager.Play (SONG_TITLE, 1);
					}
				}
			}
		}
	}
// don't enable redbook for a non-apple demo version of the shareware demo
return nCurItem;		//kill warning
}

//------------------------------------------------------------------------------

static void InitStrings (void)
{
	static bool bInitialized = false;

if (bInitialized)
	return;
bInitialized = true;

pszLowMediumHigh [0] = TXT_LOW;
pszLowMediumHigh [1] = TXT_MEDIUM;
pszLowMediumHigh [2] = TXT_HIGH;
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
	int	bSongPlaying = (gameConfig.nMidiVolume > 0);

InitStrings ();

gameStates.sound.nSoundChannels = SoundChannelIndex ();
do {
	m.Destroy ();
	m.Create (20);
	soundOpts.nGatling = -1;
	if (gameOpts->app.bNotebookFriendly || gameOpts->app.bExpertMode) {
		sprintf (szSlider + 1, TXT_SOUND_CHANNEL_COUNT, pszLowMediumHigh [gameStates.sound.nSoundChannels - 2]);
		*szSlider = *(TXT_SOUND_CHANNEL_COUNT - 1);
		m.AddSlider ("channels", szSlider + 1, gameStates.sound.nSoundChannels - 2, 0, 2, KEY_C, HTX_SOUND_CHANNEL_COUNT);  
		}
	m.AddSlider ("fx volume", TXT_FX_VOLUME, gameConfig.nAudioVolume [0], 0, 8, KEY_F, HTX_ONLINE_MANUAL);
	if (!gameOpts->sound.bLinkVolumes)
		m.AddSlider ("ambient volume", TXT_AMBIENT_VOLUME, gameConfig.nAudioVolume [1], 0, 8, KEY_A, HTX_ONLINE_MANUAL);
	m.AddSlider ("music volume", redbook.Enabled () ? TXT_CD_VOLUME : TXT_MIDI_VOLUME, 
					 redbook.Enabled () ? gameConfig.nRedbookVolume : gameConfig.nMidiVolume, 
					 0, 8, KEY_M, HTX_ONLINE_MANUAL);
	m.AddText ("", "");
	m.AddCheck ("link volumes", TXT_LINK_AUDIO_VOLUMES, gameOpts->sound.bLinkVolumes, KEY_L, HTX_ONLINE_MANUAL);
	m.AddCheck ("redbook sound", TXT_REDBOOK_ENABLED, redbook.Enabled (), KEY_C, HTX_ONLINE_MANUAL);
	m.AddCheck ("reverse stereo", TXT_REVERSE_STEREO, gameConfig.bReverseChannels, KEY_R, HTX_ONLINE_MANUAL);
	if (!gameStates.app.bNostalgia) {
#if 1
		if (!redbook.Enabled () && gameConfig.nMidiVolume)
			m.AddCheck ("fade music", TXT_FADE_MUSIC, gameOpts->sound.bFadeMusic, KEY_F, HTX_FADE_MUSIC);
#endif
		m.AddText ("", "");
		m.AddCheck ("ship sound", TXT_SHIP_SOUND, gameOpts->sound.bShip, KEY_S, HTX_SHIP_SOUND);
		m.AddCheck ("missile sound", TXT_MISSILE_SOUND, gameOpts->sound.bMissiles, KEY_M, HTX_MISSILE_SOUND);
		m.AddCheck ("gatling sound", TXT_GATLING_SOUND, gameOpts->sound.bGatling, KEY_G, HTX_GATLING_SOUND);
		if (gameOpts->sound.bGatling)
			m.AddCheck ("spinup sound", TXT_SPINUP_SOUND, extraGameInfo [0].bGatlingSpeedUp, KEY_U, HTX_SPINUP_SOUND);
		if (!gameOpts->render.cockpit.bTextGauges)
			m.AddCheck ("shield warning", TXT_SHIELD_WARNING, gameOpts->gameplay.bShieldWarning, KEY_W, HTX_CPIT_SHIELDWARN);
		}

	i = m.Menu (NULL, TXT_SOUND_OPTS, SoundMenuCallback, &choice);
	redbook.Enable (m ["redbook sound"]->Value ());
	gameConfig.bReverseChannels = m ["reverse stereo"]->Value ();
	if (!gameStates.app.bNostalgia) {
		if (!redbook.Enabled () && gameConfig.nMidiVolume)
			GET_VAL (gameOpts->sound.bFadeMusic, "fade music");
		GET_VAL (gameOpts->sound.bShip, "ship sound");
		GET_VAL (gameOpts->sound.bMissiles, "missile sound");
		GET_VAL (gameOpts->sound.bGatling, "gatling sound");
		GET_VAL (extraGameInfo [0].bGatlingSpeedUp, "spinup sound");
		GET_VAL (gameOpts->gameplay.bShieldWarning, "shield warning");
		if (gameStates.app.bGameRunning && !(gameOpts->sound.bShip && gameOpts->sound.bGatling))
			audio.DestroyObjectSound (LOCALPLAYER.nObject);
		}
	gameOpts->sound.xCustomSoundVolume = fix (float (gameConfig.nAudioVolume [0]) * 10.0f / 8.0f + 0.5f);
} while (i == -2);
if (gameConfig.nMidiVolume < 1)
	midi.PlaySong (NULL, NULL, NULL, 0, 0);
else if (!bSongPlaying)
	songManager.PlayCurrent (1);
audio.SetMaxChannels (32 << (gameStates.sound.nSoundChannels - 2));
}

//------------------------------------------------------------------------------
//eof
