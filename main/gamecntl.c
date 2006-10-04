/* $Id: gamecntl.c, v 1.23 2003/11/07 06:30:06 btb Exp $ */
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

/*
 *
 * Game Controls Stuff
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

//#define DOOR_DEBUGGING

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "pstypes.h"
#include "console.h"
#include "inferno.h"
#include "game.h"
#include "player.h"
#include "key.h"
#include "object.h"
#include "menu.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "digi.h"
#include "ibitblt.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "newmenu.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "gamefont.h"
#include "gamepal.h"
#include "endlevel.h"
#include "joydefs.h"
#include "kconfig.h"
#include "mouse.h"
#include "titles.h"
#include "gr.h"
#include "playsave.h"
#include "movie.h"
#include "scores.h"

#if defined (TACTILE)
#include "tactile.h"
#endif

#include "pa_enabl.h"
#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "ai.h"
#include "rbaudio.h"
#include "switch.h"
#include "escort.h"
#include "collide.h"
#include "ogl_init.h"
#include "object.h"
#include "sphere.h"
#include "cheats.h"
#include "input.h"

#ifdef POLY_ACC
# include "poly_acc.h"
#endif

//------------------------------------------------------------------------------
//#define TEST_TIMER    1		//if this is set, do checking on timer

#define SHOW_EXIT_PATH  1

#define Arcade_mode 0

#ifdef EDITOR
#include "editor/editor.h"
#endif

//#define _MARK_ON 1
#ifdef __WATCOMC__
#if __WATCOMC__ < 1000
#include <wsample.h>		//should come after inferno.h to get mark setting
#endif
#endif

#ifdef SDL_INPUT
#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif
#endif

extern void FullPaletteSave(void);

extern void SetFunctionMode (int);

// Global Variables -----------------------------------------------------------

int	redbook_volume = 255;


//	External Variables ---------------------------------------------------------

extern int	Speedtest_on;			 // Speedtest global adapted from game.c
extern char WaitForRefuseAnswer, RefuseThisPlayer, RefuseTeam;

#ifndef NDEBUG
extern int	Mark_count;
extern int	Speedtest_start_time;
extern int	Speedtest_segnum;
extern int	Speedtest_sidenum;
extern int	Speedtest_frame_start;
extern int	Speedtest_count;
#endif

extern int	*Toggle_var;
extern int	last_drawn_cockpit[2];
extern int	Debug_pause;

extern fix	Show_view_text_timer;

//	Function prototypes --------------------------------------------------------


extern void CyclePrimary();
extern void CycleSecondary();
extern void InitMarkerInput();
extern void MarkerInputMessage (int);
extern void grow_window(void);
extern void shrink_window(void);
extern int	AllowedToFireMissile(void);
extern int	AllowedToFireFlare(void);
extern void	CheckRearView(void);
extern int	create_special_path(void);
extern void MovePlayerToSegment(segment *seg, int side);
extern void	kconfig_center_headset(void);
extern void GameRenderFrameMono(void);
extern void NewDemoStripFrames(char *, int);
extern void ToggleCockpit(void);
extern int  dump_used_textures_all(void);
extern void DropMarker();
extern void DropSecondaryWeapon(int nWeapon);
extern void DropCurrentWeapon();

void FinalCheats(int key);

#ifndef RELEASE
void DoCheatMenu(void);
#endif

void HandleGameKey(int key);
int HandleSystemKey(int key);
void HandleTestKey(int key);
void HandleVRKey(int key);

void speedtest_init(void);
void speedtest_frame(void);
void advance_sound(void);
void play_test_sound(void);

#define key_isfunc(k) (((k&0xff)>=KEY_F1 && (k&0xff)<=KEY_F10) || (k&0xff)==KEY_F11 || (k&0xff)==KEY_F12)
#define key_ismod(k)  ((k&0xff)==KEY_LALT || (k&0xff)==KEY_RALT || (k&0xff)==KEY_LSHIFT || (k&0xff)==KEY_RSHIFT || (k&0xff)==KEY_LCTRL || (k&0xff)==KEY_RCTRL)

// Functions ------------------------------------------------------------------

#define CONVERTER_RATE  20		//10 units per second xfer rate
#define CONVERTER_SCALE  2		//2 units energy -> 1 unit shields

#define CONVERTER_SOUND_DELAY (f1_0/2)		//play every half second

void TransferEnergyToShield(fix time)
{
	fix e;		//how much energy gets transfered
	static fix last_play_time=0;

	if (time <= 0)
		return;
	e = min(min(time*CONVERTER_RATE, gameData.multi.players [gameData.multi.nLocalPlayer].energy - INITIAL_ENERGY), 
		         (MAX_SHIELDS-gameData.multi.players [gameData.multi.nLocalPlayer].shields)*CONVERTER_SCALE);
	if (e <= 0) {
		if (gameData.multi.players [gameData.multi.nLocalPlayer].energy <= INITIAL_ENERGY)
			HUDInitMessage(TXT_TRANSFER_ENERGY, f2i(INITIAL_ENERGY));
		else
			HUDInitMessage(TXT_TRANSFER_SHIELDS);
		return;
	}

	gameData.multi.players [gameData.multi.nLocalPlayer].energy  -= e;
	gameData.multi.players [gameData.multi.nLocalPlayer].shields += e/CONVERTER_SCALE;
	MultiSendShields ();
	gameStates.app.bUsingConverter = 1;
	if (last_play_time > gameData.app.xGameTime)
		last_play_time = 0;

	if (gameData.app.xGameTime > last_play_time+CONVERTER_SOUND_DELAY) {
		DigiPlaySampleOnce(SOUND_CONVERT_ENERGY, F1_0);
		last_play_time = gameData.app.xGameTime;
	}

}

//------------------------------------------------------------------------------

void update_vcr_state();
void do_weapon_stuff(void);

//------------------------------------------------------------------------------
// Control Functions

fix newdemo_single_frame_time;

void update_vcr_state(void)
{
	if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_RIGHT])
		gameData.demo.nVcrState = ND_STATE_FASTFORWARD;
	else if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_LEFT])
		gameData.demo.nVcrState = ND_STATE_REWINDING;
	else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_RIGHT] && ((TimerGetFixedSeconds() - newdemo_single_frame_time) >= F1_0))
		gameData.demo.nVcrState = ND_STATE_ONEFRAMEFORWARD;
	else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_LEFT] && ((TimerGetFixedSeconds() - newdemo_single_frame_time) >= F1_0))
		gameData.demo.nVcrState = ND_STATE_ONEFRAMEBACKWARD;
	else if ((gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || (gameData.demo.nVcrState == ND_STATE_REWINDING))
		gameData.demo.nVcrState = ND_STATE_PLAYBACK;
}

//------------------------------------------------------------------------------
//returns which bomb will be dropped next time the bomb key is pressed
int which_bomb()
{
	int bomb, otherBomb;

	//use the last one selected, unless there aren't any, in which case use
	//the other if there are any
   // If hoard game, only let the player drop smart mines
if (gameData.app.nGameMode & GM_ENTROPY)
   return PROXIMITY_INDEX; //allow for dropping orbs
if (gameData.app.nGameMode & GM_HOARD)
	return SMART_MINE_INDEX;

bomb = bLastSecondaryWasSuper [PROXIMITY_INDEX] ? SMART_MINE_INDEX : PROXIMITY_INDEX;
otherBomb = SMART_MINE_INDEX + PROXIMITY_INDEX - bomb;

if (!gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [bomb] &&
	 gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [otherBomb]) {
	bomb = otherBomb;
	bLastSecondaryWasSuper [bomb % SUPER_WEAPON] = (bomb == SMART_MINE_INDEX);
	}
return bomb;
}

//------------------------------------------------------------------------------

void do_weapon_stuff(void)
{
  int i;

if (Controls.useCloakDownCount)
	ApplyCloak (0, -1);
if (Controls.useInvulDownCount)
	ApplyInvul (0, -1);
if (Controls.fire_flareDownCount)
	if (AllowedToFireFlare())
		CreateFlare(gameData.objs.console);
if (AllowedToFireMissile()) {
	i = secondaryWeaponToWeaponInfo[gameData.weapons.nSecondary];
	gameData.app.nGlobalMissileFiringCount += WI_fire_count (i) * (Controls.fire_secondary_state || Controls.fire_secondaryDownCount);
	}
if (gameData.app.nGlobalMissileFiringCount) {
	DoMissileFiring (1);			//always enable autoselect for normal missile firing
	gameData.app.nGlobalMissileFiringCount--;
	}
if (Controls.cycle_primary_count) {
	for (i = 0; i < Controls.cycle_primary_count; i++)
	CyclePrimary ();
	}
if (Controls.cycle_secondary_count) {
	for (i = 0; i < Controls.cycle_secondary_count; i++)
	CycleSecondary ();
	}
if (Controls.headlight_count) {
	for (i=0;i<Controls.headlight_count;i++)
	toggle_headlight_active ();
	}
if (gameData.app.nGlobalMissileFiringCount < 0)
	gameData.app.nGlobalMissileFiringCount = 0;
//	Drop proximity bombs.
if (Controls.drop_bombDownCount) {
	if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.console->segnum].special == SEGMENT_IS_NODAMAGE))
		Controls.drop_bombDownCount = 0;
	else {
		int ssw_save = gameData.weapons.nSecondary;
		while (Controls.drop_bombDownCount--) {
			int ssw_save2 = gameData.weapons.nSecondary = which_bomb();
			if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))
				DropSecondaryWeapon (-1);
			else
				DoMissileFiring(gameData.weapons.nSecondary == ssw_save);	//only allow autoselect if bomb is actually selected
			if (gameData.weapons.nSecondary != ssw_save2 && ssw_save == ssw_save2)
				ssw_save = gameData.weapons.nSecondary;    //if bomb was selected, and we ran out & autoselect, then stick with new selection
			}
		gameData.weapons.nSecondary = ssw_save;
	}
}
}

//------------------------------------------------------------------------------

char *Pause_msg;

extern void GameRenderFrame();
extern void show_extra_views();

void apply_modified_palette(void)
{
//@@    int				k, x, y;
//@@    grs_bitmap	*sbp;
//@@    grs_canvas	*save_canv;
//@@    int				color_xlate[256];
//@@
//@@
//@@    if (!gameData.render.xFlashEffect && ((gameStates.ogl.palAdd.red < 10) || (gameStates.ogl.palAdd.red < (gameStates.ogl.palAdd.green + gameStates.ogl.palAdd.blue))))
//@@		return;
//@@
//@@    ResetCockpit();
//@@
//@@    save_canv = grdCurCanv;
//@@    GrSetCurrentCanvas(&grdCurScreen->sc_canvas);
//@@
//@@    sbp = &grdCurScreen->sc_canvas.cv_bitmap;
//@@
//@@    for (x=0; x<256; x++)
//@@		color_xlate[x] = -1;
//@@
//@@    for (k=0; k<4; k++) {
//@@		for (y=0; y<grdCurScreen->sc_h; y+= 4) {
//@@			  for (x=0; x<grdCurScreen->sc_w; x++) {
//@@					int	color, new_color;
//@@					int	r, g, b;
//@@					int	xcrd, ycrd;
//@@
//@@					ycrd = y+k;
//@@					xcrd = x;
//@@
//@@					color = gr_ugpixel(sbp, xcrd, ycrd);
//@@
//@@					if (color_xlate[color] == -1) {
//@@						r = grPalette[color*3+0];
//@@						g = grPalette[color*3+1];
//@@						b = grPalette[color*3+2];
//@@
//@@						r += gameStates.ogl.palAdd.red;		 if (r > 63) r = 63;
//@@						g += gameStates.ogl.palAdd.green;   if (g > 63) g = 63;
//@@						b += gameStates.ogl.palAdd.blue;		if (b > 63) b = 63;
//@@
//@@						color_xlate[color] = GrFindClosestColorCurrent(r, g, b);
//@@
//@@					}
//@@
//@@					new_color = color_xlate[color];
//@@
//@@					GrSetColor(new_color);
//@@					gr_upixel(xcrd, ycrd);
//@@			  }
//@@		}
//@@    }
}

//------------------------------------------------------------------------------

void format_time(char *str, int secs_int)
{
	int h, m, s;

	h = secs_int/3600;
	s = secs_int%3600;
	m = s / 60;
	s = s % 60;
	sprintf(str, "%1d:%02d:%02d", h, m, s );
}

//------------------------------------------------------------------------------

void do_show_netgame_help();

//Process selected keys until game unpaused. returns key that left pause (p or esc)
int do_game_pause()
{
	int key;
	char msg[1000];
	char total_time[9], level_time[9];

	key=0;

	if (gameData.app.bGamePaused) {		//unpause!
		gameData.app.bGamePaused=0;
		gameStates.app.bEnterGame = 1;
#if defined (TACTILE)
			if (TactileStick)
			  EnableForces();
#endif
		return KEY_PAUSE;
	}

#ifdef NETWORK
	if (gameData.app.nGameMode & GM_NETWORK)
	{
	 do_show_netgame_help();
    return (KEY_PAUSE);
	}
	else if (gameData.app.nGameMode & GM_MULTI)
	 {
	  HUDInitMessage (TXT_MODEM_PAUSE);
	  return (KEY_PAUSE);
	 }
#endif

	DigiPauseAll();
	RBAPause();
	StopTime();

	PaletteSave();
	apply_modified_palette();
	ResetPaletteAdd();

// -- Matt: This is a hacked-in test for the stupid menu/flash problem.
//	We need a new brightening primitive if we want to make this not horribly ugly.
//		  gameStates.render.grAlpha = 2;
//		  GrRect(0, 0, 319, 199);

	GameFlushInputs();

	gameData.app.bGamePaused=1;

#if defined (TACTILE)
	if (TactileStick)
		  DisableForces();
	#endif

//	SetScreenMode( SCREEN_MENU );
	SetPopupScreenMode();
	GrPaletteStepLoad (NULL);

	format_time(total_time, f2i(gameData.multi.players [gameData.multi.nLocalPlayer].time_total) + gameData.multi.players [gameData.multi.nLocalPlayer].hours_total*3600);
	format_time(level_time, f2i(gameData.multi.players [gameData.multi.nLocalPlayer].time_level) + gameData.multi.players [gameData.multi.nLocalPlayer].hours_level*3600);

   if (gameData.demo.nState!=ND_STATE_PLAYBACK)
		sprintf(msg, TXT_PAUSE_MSG1, GAMETEXT (332 + gameStates.app.nDifficultyLevel), 
				  gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board, level_time, total_time);
   else
	  	sprintf(msg, TXT_PAUSE_MSG2, GAMETEXT (332 +  gameStates.app.nDifficultyLevel), 
				  gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board);

	if (!gameOpts->menus.nStyle) {
		gameStates.menus.nInMenu++;
		GameRenderFrame();
		gameStates.menus.nInMenu--;
		ShowBoxedMessage(Pause_msg=msg);		  //TXT_PAUSE);
		}
	GrabMouse (0, 0);
	while (gameData.app.bGamePaused) 
	{
		int screen_changed;

#if defined (WINDOWS)

		if (!(VR_screen_flags & VRF_COMPATIBLE_MENUS)) {
			ShowBoxedMessage(msg);
		}

	SkipPauseStuff:

		while (!(key = KeyInKey()))
		{
			MSG wmsg;
			DoMessageStuff(&wmsg);
			if (_RedrawScreen) {
#if TRACE
				con_printf (CON_DEBUG, "Redrawing paused screen.\n");
#endif
				_RedrawScreen = FALSE;
				if (VR_screen_flags & VRF_COMPATIBLE_MENUS) 
					GameRenderFrame();
				gameStates.video.nScreenMode = -1;
				SetPopupScreenMode();
				GrPaletteStepLoad (NULL);
				ShowBoxedMessage(msg);
#if 0
				if (gameStates.render.cockpit.nMode==CM_FULL_COCKPIT || gameStates.render.cockpit.nMode==CM_STATUS_BAR)
					if (!GRMODEINFO(modex)) RenderGauges();
#endif
			}
		}

#else
	if (!(gameOpts->menus.nStyle && gameStates.app.bGameRunning))
		key = key_getch();
	else {
		gameStates.menus.nInMenu++;
		while (!(key = KeyInKey ())) {
			GameRenderFrame ();
			GrPaletteStepLoad(NULL);
			RemapFontsAndMenus (1);
			ShowBoxedMessage(msg);
			}
		gameStates.menus.nInMenu--;
		}
#endif

#ifndef RELEASE
		HandleTestKey(key);
#endif
		
		screen_changed = HandleSystemKey(key);

	#ifdef WINDOWS
		if (screen_changed == -1) {
			ExecMessageBox(NULL, 1, TXT_OK, "Unable to do this\noperation while paused under\n320x200 mode"); 
			goto SkipPauseStuff;
		}
	#endif

		HandleVRKey(key);

		if (screen_changed) {
			GameRenderFrame();
			WIN(SetPopupScreenMode());
			ShowBoxedMessage(msg);
#if 0			
			show_extra_views();
			if (gameStates.render.cockpit.nMode==CM_FULL_COCKPIT || gameStates.render.cockpit.nMode==CM_STATUS_BAR)
				RenderGauges();
#endif				
		}
	}
	GrabMouse (1, 0);
	if (VR_screen_flags & VRF_COMPATIBLE_MENUS) {
		ClearBoxedMessage();
	}

	GameFlushInputs();
	ResetCockpit();
	PaletteRestore();
	StartTime();
	if (gameStates.sound.bRedbookPlaying)
		RBAResume();
	DigiResumeAll();
	return key;
}

//------------------------------------------------------------------------------

extern int NetworkWhoIsMaster(), NetworkHowManyConnected(), GetMyNetRanking();
extern char Pauseable_menu;
//char *NetworkModeNames[]={"Anarchy", "Team Anarchy", "Robo Anarchy", "Cooperative", "Capture the Flag", "Hoard", "Team Hoard", "Unknown"};
extern int PhallicLimit, PhallicMan;

#ifdef NETWORK
void do_show_netgame_help()
 {
	newmenu_item m[30];
   char mtext[30][60];
	int i, num=0, eff;
#ifndef RELEASE
	int pl;
#endif
	//char *eff_strings[]={"trashing", "really hurting", "seriously affecting", "hurting", "affecting", "tarnishing"};

	memset (m, 0, sizeof (m));
   for (i=0;i<30;i++)
	{
	 m[i].text=(char *)&mtext[i];
    m[i].type=NM_TYPE_TEXT;
	}

   sprintf (mtext[num], TXT_INFO_GAME, netGame.game_name); num++;
   sprintf (mtext[num], TXT_INFO_MISSION, netGame.mission_title); num++;
	sprintf (mtext[num], TXT_INFO_LEVEL, netGame.levelnum); num++;
	sprintf (mtext[num], TXT_INFO_SKILL, MENU_DIFFICULTY_TEXT(netGame.difficulty)); num++;
	sprintf (mtext[num], TXT_INFO_MODE, GT(537+netGame.gamemode)); num++;
	sprintf (mtext[num], TXT_INFO_SERVER, gameData.multi.players [NetworkWhoIsMaster()].callsign); num++;
   sprintf (mtext[num], TXT_INFO_PLRNUM, NetworkHowManyConnected(), netGame.max_numplayers); num++;
   sprintf (mtext[num], TXT_INFO_PPS, netGame.nPacketsPerSec); num++;
   sprintf (mtext[num], TXT_INFO_SHORTPKT, netGame.bShortPackets?"Yes":"No"); num++;

#ifndef RELEASE
		pl=(int)(((double)networkData.nTotalMissedPackets/(double)networkData.nTotalPacketsGot)*100.0);
		if (pl<0)
		  pl=0;
		sprintf (mtext[num], TXT_INFO_LOSTPKT, networkData.nTotalMissedPackets, pl); num++;
#endif

   if (netGame.KillGoal)
     { sprintf (mtext[num], TXT_INFO_KILLGOAL, netGame.KillGoal*5); num++; }

   sprintf (mtext[num], " "); num++;
   sprintf (mtext[num], TXT_INFO_PLRSCONN); num++;

   netPlayers.players [gameData.multi.nLocalPlayer].rank=GetMyNetRanking();

   for (i=0;i<gameData.multi.nPlayers;i++)
     if (gameData.multi.players [i].connected)
	  {		  
      if (!gameOpts->multi.bNoRankings)
		 {
			if (i==gameData.multi.nLocalPlayer)
				sprintf (mtext[num], "%s%s (%d/%d)", 
							pszRankStrings[netPlayers.players [i].rank], 
							gameData.multi.players [i].callsign, 
							networkData.nNetLifeKills, 
							networkData.nNetLifeKilled); 
			else
				sprintf (mtext[num], "%s%s %d/%d", 
							pszRankStrings[netPlayers.players [i].rank], 
							gameData.multi.players [i].callsign, 
							multiData.kills.matrix[gameData.multi.nLocalPlayer][i], 
							multiData.kills.matrix[i][gameData.multi.nLocalPlayer]); 
			num++;
		 }
	   else
  		 sprintf (mtext[num++], "%s", gameData.multi.players [i].callsign); 
	  }

	
  sprintf (mtext[num], " "); num++;

  eff=(int)((double)((double)networkData.nNetLifeKills/((double)networkData.nNetLifeKilled+(double)networkData.nNetLifeKills))*100.0);

  if (eff<0)
	eff=0;
  
  if (gameData.app.nGameMode & GM_HOARD)
	{
	 if (PhallicMan==-1)
		 sprintf (mtext[num], TXT_NO_RECORD2); 
	 else
		 sprintf (mtext[num], TXT_RECORD3, gameData.multi.players [PhallicMan].callsign, PhallicLimit); 
	num++;
	}
  else if (!gameOpts->multi.bNoRankings)
	{
	 sprintf (mtext[num], TXT_EFF_LIFETIME, eff); num++;
	  if (eff<60)
	   {
		 sprintf (mtext[num], TXT_EFF_INFLUENCE, GT(546 + eff / 10)); num++;
		}
	  else
	   {
		 sprintf (mtext[num], TXT_EFF_SERVEWELL); num++;
	   }
	}  
	

  	FullPaletteSave();

   Pauseable_menu=1;
	ExecMenutiny2( NULL, "netGame Information", num, m, NULL);

	PaletteRestore();
}
#endif

//------------------------------------------------------------------------------

void HandleEndlevelKey(int key)
{

	if ( key == (KEY_COMMAND + KEY_SHIFTED + KEY_P) )
		SaveScreenShot (NULL, 0);

	if (key==KEY_PRINT_SCREEN)
		SaveScreenShot (NULL, 0);

	if ( key == (KEY_COMMAND+KEY_P) )
		key = do_game_pause();

	if (key == KEY_PAUSE)
		key = do_game_pause();		//so esc from pause will end level

	if (key == KEY_ESC) {
		StopEndLevelSequence();
		last_drawn_cockpit[0]=-1;
		last_drawn_cockpit[1]=-1;
		return;
	}

	if (key == KEY_BACKSP)
		Int3();
}

//------------------------------------------------------------------------------

void HandleDeathKey(int key)
{
/*
	Commented out redundant calls because the key used here typically
	will be passed to HandleSystemKey later.  Note that I do this to pause
	which is supposed to pass the ESC key to leave the level.  This
	doesn't work in the DOS version anyway.   -Samir 
*/

	if (gameStates.app.bPlayerExploded && !key_isfunc(key) && !key_ismod(key))
		gameStates.app.bDeathSequenceAborted  = 1;		//Any key but func or modifier aborts

	if ( key == (KEY_COMMAND + KEY_SHIFTED + KEY_P) ) {
//		SaveScreenShot (NULL, 0);
		gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
	}

	if (key==KEY_PRINT_SCREEN) {
//		SaveScreenShot (NULL, 0);
		gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
	}

	if ( key == (KEY_COMMAND+KEY_P) ) {
//		key = do_game_pause();
		gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
	}

	if (key == KEY_PAUSE)   {
//		key = do_game_pause();		//so esc from pause will end level
		gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
	}

	if (key == KEY_ESC) {
		if (gameData.objs.console->flags & OF_EXPLODING)
			gameStates.app.bDeathSequenceAborted = 1;
	}

	if (key == KEY_BACKSP)  {
		gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
		Int3();
	}

	//don't abort death sequence for netgame join/refuse keys
	if (	(key == KEY_ALTED + KEY_1) ||
			(key == KEY_ALTED + KEY_2))
		gameStates.app.bDeathSequenceAborted  = 0;

	if (gameStates.app.bDeathSequenceAborted)
		GameFlushInputs();

}

//------------------------------------------------------------------------------

inline int GuidedInMainView (void)
{
	object *gmP;

return gameOpts->render.cockpit.bGuidedInMainView &&
		 (gmP = gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]) && 
		 (gmP->type == OBJ_WEAPON) && 
		 (gmP->id == GUIDEDMISS_ID) && 
		 (gmP->signature == gameData.objs.guidedMissileSig [gameData.multi.nLocalPlayer]);
}

//------------------------------------------------------------------------------

void HandleDemoKey(int key)
{
	switch (key) {

		case KEY_F3:
			 PA_DFX (HUDInitMessage (TXT_3DFX_COCKPIT));
			 PA_DFX (break);
			 if (!GuidedInMainView ())
				ToggleCockpit();
			 break;

		case KEY_SHIFTED+KEY_MINUS:
		case KEY_MINUS:		
			shrink_window(); 
			break;

		case KEY_SHIFTED+KEY_EQUAL:
		case KEY_EQUAL:		
			grow_window(); 
			break;

		case KEY_F2:		
			gameStates.app.bConfigMenu = 1; 
			break;

		case KEY_F7:
#ifdef NETWORK
			multiData.kills.bShowList = (multiData.kills.bShowList+1) % ((gameData.demo.nGameMode & GM_TEAM) ? 4 : 3);
#endif
			break;
			
		case KEY_CTRLED+KEY_F7:
#ifdef NETWORK
			if (gameStates.render.cockpit.bShowPingStats = !gameStates.render.cockpit.bShowPingStats)
				ResetPingStats ();
			break;
#endif
		case KEY_ESC:
			SetFunctionMode (FMODE_MENU);
			break;
		case KEY_UP:
			gameData.demo.nVcrState = ND_STATE_PLAYBACK;
			break;
		case KEY_DOWN:
			gameData.demo.nVcrState = ND_STATE_PAUSED;
			break;
		case KEY_LEFT:
			newdemo_single_frame_time = TimerGetFixedSeconds();
			gameData.demo.nVcrState = ND_STATE_ONEFRAMEBACKWARD;
			break;
		case KEY_RIGHT:
			newdemo_single_frame_time = TimerGetFixedSeconds();
			gameData.demo.nVcrState = ND_STATE_ONEFRAMEFORWARD;
			break;
		case KEY_CTRLED + KEY_RIGHT:
			NDGotoEnd();
			break;
		case KEY_CTRLED + KEY_LEFT:
			NDGotoBeginning();
			break;

		case KEY_COMMAND+KEY_P:
		case KEY_CTRLED+KEY_P:
		case KEY_PAUSE:
			do_game_pause();
			break;

		case KEY_COMMAND + KEY_SHIFTED + KEY_P:
		case KEY_PRINT_SCREEN: 
		{
			int old_state;

			old_state = gameData.demo.nVcrState;
			gameData.demo.nVcrState = ND_STATE_PRINTSCREEN;
			//GameRenderFrameMono();
			SaveScreenShot (NULL, 0);
			gameData.demo.nVcrState = old_state;
			break;
		}

		#ifndef NDEBUG
		case KEY_BACKSP:
			Int3();
			break;
		case KEY_DEBUGGED + KEY_I:
			gameData.demo.bInterpolate = !gameData.demo.bInterpolate;
#if TRACE
			if (gameData.demo.bInterpolate)
				con_printf (CON_DEBUG, "demo playback interpolation now on\n");
			else
				con_printf (CON_DEBUG, "demo playback interpolation now off\n");
#endif
			break;
		case KEY_DEBUGGED + KEY_K: {
			int how_many, c;
			char filename[FILENAME_LEN], num[16];
			newmenu_item m[6];

			filename[0] = '\0';
			memset (m, 0, sizeof (m));
			m[0].type = NM_TYPE_TEXT; 
			m[0].text = "output file name";
			m[1].type = NM_TYPE_INPUT;
			m[1].text_len = 8; 
			m[1].text = filename;
			c = ExecMenu( NULL, NULL, 2, m, NULL, NULL );
			if (c == -2)
				break;
			strcat(filename, ".dem");
			num[0] = '\0';
			m[ 0].type = NM_TYPE_TEXT; m[ 0].text = "strip how many bytes";
			m[ 1].type = NM_TYPE_INPUT;m[ 1].text_len = 16; m[1].text = num;
			c = ExecMenu( NULL, NULL, 2, m, NULL, NULL );
			if (c == -2)
				break;
			how_many = atoi(num);
			if (how_many <= 0)
				break;
			NewDemoStripFrames(filename, how_many);

			break;
		}
		#endif

	}
}

//------------------------------------------------------------------------------
//switch a cockpit window to the next function
int select_next_window_function(int w)
{
	Assert(w==0 || w==1);

	switch (Cockpit_3d_view[w]) {
		case CV_NONE:
			Cockpit_3d_view[w] = CV_REAR;
			break;
		case CV_REAR:
			if (!gameStates.app.bNostalgia && EGI_FLAG (bRadarEnabled, 1, 0) &&
			    (!(gameData.app.nGameMode & GM_MULTI) || (netGame.game_flags & NETGAME_FLAG_SHOW_MAP))) {
				Cockpit_3d_view[w] = CV_RADAR_TOPDOWN;
				break;
				}
		case CV_RADAR_TOPDOWN:
			if (!gameStates.app.bNostalgia && EGI_FLAG (bRadarEnabled, 1, 0) &&
			    (!(gameData.app.nGameMode & GM_MULTI) || (netGame.game_flags & NETGAME_FLAG_SHOW_MAP))) {
				Cockpit_3d_view[w] = CV_RADAR_HEADSUP;
				break;
				}
		case CV_RADAR_HEADSUP:
			if (find_escort()) {
				Cockpit_3d_view[w] = CV_ESCORT;
				break;
			}
			//if no ecort, fall through
		case CV_ESCORT:
			Coop_view_player[w] = -1;		//force first player
#ifdef NETWORK
			//fall through
		case CV_COOP:
			Marker_viewer_num[w] = -1;
			if ((gameData.app.nGameMode & GM_MULTI_COOP) || (gameData.app.nGameMode & GM_TEAM)) {
				Cockpit_3d_view[w] = CV_COOP;
				while (1) {
					Coop_view_player[w]++;
					if (Coop_view_player[w] == gameData.multi.nPlayers) {
						Cockpit_3d_view[w] = CV_MARKER;
						goto case_marker;
					}
					if (Coop_view_player[w]==gameData.multi.nLocalPlayer)
						continue;

					if (gameData.app.nGameMode & GM_MULTI_COOP)
						break;
					else if (GetTeam(Coop_view_player[w]) == GetTeam(gameData.multi.nLocalPlayer))
						break;
				}
				break;
			}
			//if not multi, fall through
		case CV_MARKER:
		case_marker:;
			if (!(gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_COOP) || netGame.Allow_marker_view) {	//anarchy only
				Cockpit_3d_view[w] = CV_MARKER;
				if (Marker_viewer_num[w] == -1)
					Marker_viewer_num[w] = gameData.multi.nLocalPlayer * 2;
				else if (Marker_viewer_num[w] == gameData.multi.nLocalPlayer * 2)
					Marker_viewer_num[w]++;
				else
					Cockpit_3d_view[w] = CV_NONE;
			}
			else
#endif
				Cockpit_3d_view[w] = CV_NONE;
			break;
	}
	WritePlayerFile();

	return 1;	 //screen_changed
}

//------------------------------------------------------------------------------


void songs_goto_next_song();
void songs_goto_prev_song();

#ifdef DOOR_DEBUGGING
dump_door_debugging_info()
{
	object *objP;
	vms_vector new_pos;
	fvi_query fq;
	fvi_info hit_info;
	int fate;
	FILE *dfile;
	int wall_numn;

	obj = gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].objnum;
	VmVecScaleAdd(&new_pos, &objP->pos, &objP->orient.fvec, i2f(100);

	fq.p0						= &objP->pos;
	fq.startseg				= objP->segnum;
	fq.p1						= &new_pos;
	fq.rad					= 0;
	fq.thisobjnum			= gameData.multi.players [gameData.multi.nLocalPlayer].objnum;
	fq.ignore_obj_list	= NULL;
	fq.flags					= 0;

	fate = FindVectorIntersection(&fq, &hit_info);

	dfile = fopen("door.out", "at");

	fprintf(dfile, "FVI hit_type = %d\n", fate);
	fprintf(dfile, "    hit_seg = %d\n", hit_info.hit_seg);
	fprintf(dfile, "    hit_side = %d\n", hit_info.hit_side);
	fprintf(dfile, "    hit_side_seg = %d\n", hit_info.hit_side_seg);
	fprintf(dfile, "\n");

	if (fate == HIT_WALL) {

		wall_num = WallNumI (hit_info.hit_seg, hit_info.hit_side);
		fprintf(dfile, "wall_num = %d\n", wall_num);
	
		if (IS_WALL (wall_num)) {
			wall *wall = gameData.walls.walls + wall_num;
			active_door *d;
			int i;
	
			fprintf(dfile, "    segnum = %d\n", wall->segnum);
			fprintf(dfile, "    sidenum = %d\n", wall->sidenum);
			fprintf(dfile, "    hps = %x\n", wall->hps);
			fprintf(dfile, "    linked_wall = %d\n", wall->linked_wall);
			fprintf(dfile, "    type = %d\n", wall->type);
			fprintf(dfile, "    flags = %x\n", wall->flags);
			fprintf(dfile, "    state = %d\n", wall->state);
			fprintf(dfile, "    trigger = %d\n", wall->trigger);
			fprintf(dfile, "    clip_num = %d\n", wall->clip_num);
			fprintf(dfile, "    keys = %x\n", wall->keys);
			fprintf(dfile, "    controlling_trigger = %d\n", wall->controlling_trigger);
			fprintf(dfile, "    cloak_value = %d\n", wall->cloak_value);
			fprintf(dfile, "\n");
	
	
			for (i=0;i<gameData.walls.nOpenDoors;i++) {		//find door
				d = &gameData.walls.activeDoors[i];
				if (d->front_wallnum[0]==wall_num || 
					 d->back_wallnum[0]==wall_num || 
					 (d->n_parts==2 && (d->front_wallnum[1]==wall_num || d->back_wallnum[1]==wall_num)))
					break;
			} 
	
			if (i>=gameData.walls.nOpenDoors)
				fprintf(dfile, "No active door.\n");
			else {
				fprintf(dfile, "Active door %d:\n", i);
				fprintf(dfile, "    n_parts = %d\n", d->n_parts);
				fprintf(dfile, "    front_wallnum = %d, %d\n", d->front_wallnum[0], d->front_wallnum[1]);
				fprintf(dfile, "    back_wallnum = %d, %d\n", d->back_wallnum[0], d->back_wallnum[1]);
				fprintf(dfile, "    time = %x\n", d->time);
			}
	
		}
	}

	fprintf(dfile, "\n");
	fprintf(dfile, "\n");

	fclose(dfile);

}
#endif

//------------------------------------------------------------------------------

extern int bSaveScreenShot;
//this is for system-level keys, such as help, etc.
//returns 1 if screen changed
int HandleSystemKey(int key)
{
	int screen_changed=0;
	int bStopPlayerMovement = 1;

	//if (gameStates.gameplay.bSpeedBoost)
	//	return 0;

	if (!gameStates.app.bPlayerIsDead)
		switch (key) {

			#ifdef DOOR_DEBUGGING
			case KEY_LAPOSTRO+KEY_SHIFTED:
				dump_door_debugging_info();
				break;
			#endif

			case KEY_ESC:
				if (gameData.app.bGamePaused) {
					gameData.app.bGamePaused=0;
					}
				else {
					gameStates.app.bGameAborted=1;
					SetFunctionMode (FMODE_MENU);
				}
				break;

			case KEY_SHIFTED+KEY_F1:
				screen_changed = select_next_window_function(0);
				break;
			case KEY_SHIFTED+KEY_F2:
				screen_changed = select_next_window_function(1);
				break;
			case KEY_SHIFTED+KEY_F3:
				gameOpts->render.cockpit.nWindowSize = (gameOpts->render.cockpit.nWindowSize + 1) % 4;
				screen_changed = 1; //select_next_window_function(1);
				break;
			case KEY_CTRLED+KEY_F3:
				gameOpts->render.cockpit.nWindowPos = (gameOpts->render.cockpit.nWindowPos + 1) % 6;
				screen_changed = 1; //select_next_window_function(1);
				break;
			case KEY_SHIFTED+KEY_CTRLED+KEY_F3:
				gameOpts->render.cockpit.nWindowZoom = (gameOpts->render.cockpit.nWindowZoom + 1) % 4;
				screen_changed = 1; //select_next_window_function(1);
				break;
		}

	switch (key) {

#if 1
		case KEY_SHIFTED + KEY_ESC:
			con_show();
			break;

#else
		case KEY_SHIFTED + KEY_ESC:     //quick exit
			#ifdef EDITOR
				if (! SafetyCheck()) 
				break;
				close_editor_screen();
			#endif

			gameStates.app.bGameAborted=1;
			gameStates.app.nFunctionMode=FMODE_EXIT;
			break;
#endif

		case KEY_COMMAND+KEY_P: 
		case KEY_CTRLED+KEY_P: 
		case KEY_PAUSE: 
			do_game_pause();				
			break;

		case KEY_COMMAND + KEY_SHIFTED + KEY_P:
		case KEY_PRINT_SCREEN: 
			bSaveScreenShot = 1;
			SaveScreenShot (NULL, 0);		
			break;

		case KEY_F1:					
			do_show_help();			
			break;

		case KEY_F2:					//gameStates.app.bConfigMenu = 1; break;
			{
				int scanline_save = Scanline_double;

				if (!(gameData.app.nGameMode&GM_MULTI)) {
					PaletteSave(); 
					apply_modified_palette(); 
					ResetPaletteAdd(); 
					GrPaletteStepLoad (NULL); 
					}
				OptionsMenu();
				if (!(gameData.app.nGameMode&GM_MULTI)) 
					PaletteRestore();
				if (scanline_save != Scanline_double)   
					InitCockpit();	// reset the cockpit after changing...
	         PA_DFX (InitCockpit());
				break;
			}


		case KEY_F3:
			#ifdef WINDOWS		// HACK! these shouldn't work in 320x200 pause or in letterbox.
				if (gameStates.app.bPlayerIsDead) break;
				if (!(VR_screen_flags&VRF_COMPATIBLE_MENUS) && gameData.app.bGamePaused) {
					screen_changed = -1;
					break;
				}
			#endif
	
			PA_DFX (HUDInitMessage (TXT_3DFX_COCKPIT));
			PA_DFX (break);

			if (!GuidedInMainView ()) {
				ToggleCockpit();	
				screen_changed=1;
			}
			break;

		case KEY_F7+KEY_SHIFTED: 
			PaletteSave(); 
			joydefs_calibrate(); 
			PaletteRestore(); 
			break;

		case KEY_SHIFTED+KEY_MINUS:
		case KEY_MINUS:	
		#ifdef WINDOWS
			if (gameStates.app.bPlayerIsDead) 
				break;
			if (!(VR_screen_flags&VRF_COMPATIBLE_MENUS) && gameData.app.bGamePaused) {
				screen_changed = -1;
				break;
			}
		#endif

			shrink_window(); 
			screen_changed=1; 
			break;

		case KEY_SHIFTED+KEY_EQUAL:
		case KEY_EQUAL:			
		#ifdef WINDOWS
			if (gameStates.app.bPlayerIsDead) 
				break;
			if (!(VR_screen_flags&VRF_COMPATIBLE_MENUS) && gameData.app.bGamePaused) {
				screen_changed = -1;
				break;
			}
		#endif

			grow_window();  
			screen_changed=1; 
			break;

#if 1//ndef _DEBUG
		case KEY_F5:
			if ( gameData.demo.nState == ND_STATE_RECORDING ) {
				NDStopRecording();
				}
			else if ( gameData.demo.nState == ND_STATE_NORMAL )
				if (!gameData.app.bGamePaused)		//can't start demo while paused
					NDStartRecording();
			break;
#endif
		case KEY_ALTED+KEY_F4:
			#ifdef NETWORK
			multiData.bShowReticleName = (multiData.bShowReticleName+1)%2;
			#endif
			break;

		case KEY_F7:
#ifdef NETWORK
			multiData.kills.bShowList = (multiData.kills.bShowList+1) % ((gameData.app.nGameMode & GM_TEAM) ? 4 : 3);
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSortKillList();
			bStopPlayerMovement = 0;
#endif
			break;

		case KEY_CTRLED+KEY_F7:
#ifdef NETWORK
			if (gameStates.render.cockpit.bShowPingStats = !gameStates.render.cockpit.bShowPingStats)
				ResetPingStats ();
			break;
#endif

		case KEY_F8:
			#ifdef NETWORK
			MultiSendMsgStart(-1);
			#endif
			bStopPlayerMovement = 0;
			break;

		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
			#ifdef NETWORK
			MultiSendMacro(key);
			#endif
			bStopPlayerMovement = 0;
			break;		// send taunt macros
			
		case KEY_ALTED + KEY_F12:
#ifndef _DEBUG		
			if (!IsMultiGame || IsCoopGame)
#endif			
				gameStates.render.bExternalView = !gameStates.render.bExternalView;
			ResetFlightPath (&externalView, -1);
			break;

		case KEY_SHIFTED + KEY_F9:
		case KEY_SHIFTED + KEY_F10:
		case KEY_SHIFTED + KEY_F11:
		case KEY_SHIFTED + KEY_F12:
			#ifdef NETWORK
			MultiDefineMacro(key);
			#endif
			bStopPlayerMovement = 0;
			break;		// redefine taunt macros

		case KEY_ALTED+KEY_F2:
			if (!gameStates.app.bPlayerIsDead && !((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP))) {
				int     rsave, gsave, bsave;
				rsave = gameStates.ogl.palAdd.red;
				gsave = gameStates.ogl.palAdd.green;
				bsave = gameStates.ogl.palAdd.blue;

				FullPaletteSave();
				gameStates.ogl.palAdd.red = rsave;
				gameStates.ogl.palAdd.green = gsave;
				gameStates.ogl.palAdd.blue = bsave;
				StateSaveAll( 0, 0, NULL );
				PaletteRestore();
			}
			break;  // 0 means not between levels.

		case KEY_ALTED+KEY_F3:
			if (!gameStates.app.bPlayerIsDead && !((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP))) {
				FullPaletteSave();
				StateRestoreAll(1, 0, NULL);
				if (gameData.app.bGamePaused)
					do_game_pause();
			}
			break;


		case KEY_F4 + KEY_SHIFTED:
			DoEscortMenu();
			break;


		case KEY_F4 + KEY_SHIFTED + KEY_ALTED:
			ChangeGuidebotName();
			break;

		case KEY_MINUS + KEY_ALTED:     
			songs_goto_prev_song(); 
			break;

		case KEY_EQUAL + KEY_ALTED:     
			songs_goto_next_song(); 
			break;

//added 8/23/99 by Matt Mueller for hot key res/fullscreen changing, and menu access
		case KEY_CTRLED+KEY_SHIFTED+KEY_PADDIVIDE:
		case KEY_ALTED+KEY_CTRLED+KEY_PADDIVIDE:
		case KEY_ALTED+KEY_SHIFTED+KEY_PADDIVIDE:
			RenderOptionsMenu();
			break;
#if 0
		case KEY_CTRLED+KEY_SHIFTED+KEY_PADMULTIPLY:
		case KEY_ALTED+KEY_CTRLED+KEY_PADMULTIPLY:
		case KEY_ALTED+KEY_SHIFTED+KEY_PADMULTIPLY:
			change_res();
			break;
#endif
		case KEY_CTRLED+KEY_F1:
			SwitchDisplayMode (-1);
			break;
		case KEY_CTRLED+KEY_F2:
			SwitchDisplayMode (1);
			break;

		case KEY_ALTED+KEY_ENTER:
		case KEY_ALTED+KEY_PADENTER:
			GrToggleFullScreenGame();
			break;
//end addition -MM
			
//added 11/01/98 Matt Mueller
#if 0
		case KEY_CTRLED+KEY_ALTED+KEY_LAPOSTRO:
			toggle_hud_log();
			break;
#endif
//end addition -MM

		default:
			return screen_changed;
	}	 //switch (key)
if (bStopPlayerMovement) {
	StopPlayerMovement ();
	gameStates.app.bEnterGame = 2;
	}
return screen_changed;
}

//------------------------------------------------------------------------------

void HandleVRKey(int key)
{
	switch( key )   {

		case KEY_ALTED+KEY_F5:
			if ( VR_render_mode != VR_NONE )	{
				VRResetParams();
				HUDInitMessage( TXT_VR_RESET );
				HUDInitMessage( TXT_VR_SEPARATION, f2fl(VR_eye_width) );
				HUDInitMessage( TXT_VR_BALANCE, (double)VR_eye_offset/30.0 );
			}
			break;

		case KEY_ALTED+KEY_F6:
			if ( VR_render_mode != VR_NONE )	{
				VR_low_res++;
				if ( VR_low_res > 3 ) VR_low_res = 0;
				switch( VR_low_res )    {
					case 0: HUDInitMessage( TXT_VR_NORMRES ); break;
					case 1: HUDInitMessage( TXT_VR_LOWVRES ); break;
					case 2: HUDInitMessage( TXT_VR_LOWHRES ); break;
					case 3: HUDInitMessage( TXT_VR_LOWRES ); break;
				}
			}
			break;

		case KEY_ALTED+KEY_F7:
			if ( VR_render_mode != VR_NONE )	{
				VR_eye_switch = !VR_eye_switch;
				HUDInitMessage( TXT_VR_TOGGLE );
				if ( VR_eye_switch )
					HUDInitMessage( TXT_VR_RLEYE );
				else
					HUDInitMessage( TXT_VR_LREYE );
			}
			break;

		case KEY_ALTED+KEY_F8:
			if ( VR_render_mode != VR_NONE )	{
			VR_sensitivity++;
			if (VR_sensitivity > 2 )
				VR_sensitivity = 0;
			HUDInitMessage( TXT_VR_SENSITIVITY, VR_sensitivity );
		 }
			break;
		case KEY_ALTED+KEY_F9:
			if ( VR_render_mode != VR_NONE )	{
				VR_eye_width -= F1_0/10;
				if ( VR_eye_width < 0 ) VR_eye_width = 0;
				HUDInitMessage( TXT_VR_SEPARATION, f2fl(VR_eye_width) );
				HUDInitMessage( TXT_VR_DEFAULT, f2fl(VR_SEPARATION) );
			}
			break;
		case KEY_ALTED+KEY_F10:
			if ( VR_render_mode != VR_NONE )	{
				VR_eye_width += F1_0/10;
				if ( VR_eye_width > F1_0*4 )    VR_eye_width = F1_0*4;
				HUDInitMessage( TXT_VR_SEPARATION, f2fl(VR_eye_width) );
				HUDInitMessage( TXT_VR_DEFAULT, f2fl(VR_SEPARATION) );
			}
			break;

		case KEY_ALTED+KEY_F11:
			if ( VR_render_mode != VR_NONE )	{
				VR_eye_offset--;
				if ( VR_eye_offset < -30 )	VR_eye_offset = -30;
				HUDInitMessage( TXT_VR_BALANCE, (double)VR_eye_offset/30.0 );
				HUDInitMessage( TXT_VR_DEFAULT, (double)VR_PIXEL_SHIFT/30.0 );
				VR_eye_offset_changed = 2;
			}
			break;
		case KEY_ALTED+KEY_F12:
			if ( VR_render_mode != VR_NONE )	{
				VR_eye_offset++;
				if ( VR_eye_offset > 30 )	 VR_eye_offset = 30;
				HUDInitMessage( TXT_VR_BALANCE, (double)VR_eye_offset/30.0 );
				HUDInitMessage( TXT_VR_DEFAULT, (double)VR_PIXEL_SHIFT/30.0 );
				VR_eye_offset_changed = 2;
			}
			break;
	}
}

//------------------------------------------------------------------------------

extern void DropFlag();

void HandleGameKey(int key)
{
	switch (key) {

#ifndef D2X_KEYS // weapon selection handled in ControlsReadAll, d1x-style
// MWA changed the weapon select cases to have each case call
// DoSelectWeapon the macintosh keycodes aren't consecutive from 1
// -- 0 on the keyboard -- boy is that STUPID!!!!

		//	Select primary or secondary weapon.
		case KEY_1:
			DoSelectWeapon(0 , 0);
			break;
		case KEY_2:
			DoSelectWeapon(1 , 0);
			break;
		case KEY_3:
			DoSelectWeapon(2 , 0);
			break;
		case KEY_4:
			DoSelectWeapon(3 , 0);
			break;
		case KEY_5:
			DoSelectWeapon(4 , 0);
			break;

		case KEY_6:
			DoSelectWeapon(0 , 1);
			break;
		case KEY_7:
			DoSelectWeapon(1 , 1);
			break;
		case KEY_8:
			DoSelectWeapon(2 , 1);
			break;
		case KEY_9:
			DoSelectWeapon(3 , 1);
			break;
		case KEY_0:
			DoSelectWeapon(4 , 1);
			break;
#endif

		case KEY_1 + KEY_SHIFTED:
		case KEY_2 + KEY_SHIFTED:
		case KEY_3 + KEY_SHIFTED:
		case KEY_4 + KEY_SHIFTED:
		case KEY_5 + KEY_SHIFTED:
		case KEY_6 + KEY_SHIFTED:
		case KEY_7 + KEY_SHIFTED:
		case KEY_8 + KEY_SHIFTED:
		case KEY_9 + KEY_SHIFTED:
		case KEY_0 + KEY_SHIFTED:
		if (gameOpts->gameplay.bEscortHotKeys)
		{
			if (!(gameData.app.nGameMode & GM_MULTI))
				EscortSetSpecialGoal(key);
			else
				HUDInitMessage (TXT_GB_MULTIPLAYER);
			break;
		}
		
		case KEY_ALTED + KEY_R:
			gameStates.render.frameRate.value = !gameStates.render.frameRate.value;
			break;

		case KEY_ALTED + KEY_O:
			gameOpts->render.bOptimize = !gameOpts->render.bOptimize;
			break;

		case KEY_F5 + KEY_SHIFTED:
		   DropCurrentWeapon();
			break;

		case KEY_F6 + KEY_SHIFTED:
	 DropSecondaryWeapon(-1);
	 break;

#ifdef NETWORK
		case KEY_0 + KEY_ALTED:
			DropFlag ();
			break;
#endif

		case KEY_F4:
		if (!gameData.marker.nDefiningMsg)
		  InitMarkerInput();
		 break;

#ifdef NETWORK
		case KEY_ALTED + KEY_CTRLED + KEY_T:
			SwitchTeam (gameData.multi.nLocalPlayer, 0);
			break;
		case KEY_F6:
			if (netGame.RefusePlayers && WaitForRefuseAnswer && !(gameData.app.nGameMode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUDInitMessage (TXT_ACCEPT_PLR);
				}
			break;
		case KEY_ALTED + KEY_1:
			if (netGame.RefusePlayers && WaitForRefuseAnswer && (gameData.app.nGameMode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUDInitMessage (TXT_ACCEPT_PLR);
					RefuseTeam=1;
				}
			break;
		case KEY_ALTED + KEY_2:
			if (netGame.RefusePlayers && WaitForRefuseAnswer && (gameData.app.nGameMode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUDInitMessage (TXT_ACCEPT_PLR);
					RefuseTeam=2;
				}
			break;
#endif

		default:
			break;

	}	 //switch (key)
}

//	--------------------------------------------------------------------------

void toggle_movie_saving(void);
extern char Language[];

#ifndef RELEASE

void HandleTestKey(int key)
{
	switch (key) {

		case KEY_DEBUGGED+KEY_0:	ShowWeaponStatus();   break;

		#ifdef SHOW_EXIT_PATH
		case KEY_DEBUGGED+KEY_1:	create_special_path();  break;
		#endif

		case KEY_DEBUGGED+KEY_Y:
			DoReactorDestroyedStuff(NULL);
			break;

#ifdef NETWORK
	case KEY_DEBUGGED+KEY_ALTED+KEY_D:
			networkData.nNetLifeKills=4000; 
			networkData.nNetLifeKilled=5;
			MultiAddLifetimeKills();
			break;
#endif

		case KEY_BACKSP:
		case KEY_CTRLED+KEY_BACKSP:
		case KEY_ALTED+KEY_BACKSP:
		case KEY_ALTED+KEY_CTRLED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_ALTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_CTRLED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_CTRLED+KEY_ALTED+KEY_BACKSP:

				Int3(); break;

		case KEY_CTRLED+KEY_ALTED+KEY_ENTER:
				exit (0);
				break;

		case KEY_DEBUGGED+KEY_S:				
			DigiReset(); break;

		case KEY_DEBUGGED+KEY_P:
			if (gameStates.app.bGameSuspended & SUSP_ROBOTS)
				gameStates.app.bGameSuspended &= ~SUSP_ROBOTS;		//robots move
			else
				gameStates.app.bGameSuspended |= SUSP_ROBOTS;		//robots don't move
			break;



		case KEY_DEBUGGED+KEY_K:	
			gameData.multi.players [gameData.multi.nLocalPlayer].shields = 1;	
			MultiSendShields ();
			break;							//	a virtual kill
		case KEY_DEBUGGED+KEY_SHIFTED + KEY_K:  
			gameData.multi.players [gameData.multi.nLocalPlayer].shields = -1;	 
			MultiSendShields ();
			break;  //	an actual kill
		case KEY_DEBUGGED+KEY_X: 
			gameData.multi.players [gameData.multi.nLocalPlayer].lives++; 
			break; // Extra life cheat key.
		case KEY_DEBUGGED+KEY_H:
//				if (!(gameData.app.nGameMode & GM_MULTI) )   {
				gameData.multi.players [gameData.multi.nLocalPlayer].flags ^= PLAYER_FLAGS_CLOAKED;
				if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
					#ifdef NETWORK
					if (gameData.app.nGameMode & GM_MULTI)
						MultiSendCloak();
					#endif
					AIDoCloakStuff();
					gameData.multi.players [gameData.multi.nLocalPlayer].cloak_time = gameData.app.xGameTime;
#if TRACE
					con_printf (CON_DEBUG, "You are cloaked!\n");
#endif
				} else
#if TRACE
					con_printf (CON_DEBUG, "You are DE-cloaked!\n");
#endif
//				}
			break;


		case KEY_DEBUGGED+KEY_R:
			gameStates.app.cheats.bRobotsFiring = !gameStates.app.cheats.bRobotsFiring;
			break;

		case KEY_DEBUGGED+KEY_R+KEY_SHIFTED:
			KillAllRobots();
			break;

		#ifdef EDITOR		//editor-specific functions

		case KEY_E + KEY_DEBUGGED:
#ifdef NETWORK
			NetworkLeaveGame();
#endif
			SetFunctionMode (FMODE_EDITOR);
			break;
	case KEY_Q + KEY_SHIFTED + KEY_DEBUGGED:
		{
			char pal_save[768];
			memcpy(pal_save, grPalette, 768);
			InitSubTitles("end.tex");	//ingore errors
			PlayMovie ("end.mve", MOVIE_ABORT_ON, 0, gameOpts->movies.bResize);
			CloseSubTitles();
			gameStates.video.nScreenMode = -1;
			SetScreenMode(SCREEN_GAME);
			ResetCockpit();
			memcpy(grPalette, pal_save, 768);
			GrPaletteStepLoad (NULL);
			break;
		}
		case KEY_C + KEY_SHIFTED + KEY_DEBUGGED:
			if (!( gameData.app.nGameMode & GM_MULTI ))
				MovePlayerToSegment(Cursegp, Curside);
			break;   //move eye to curseg


		case KEY_DEBUGGED+KEY_W:	
			draw_world_from_game(); break;

		#endif  //#ifdef EDITOR

		//flythrough keys
		// case KEY_DEBUGGED+KEY_SHIFTED+KEY_F: toggle_flythrough(); break;
		// case KEY_LEFT:		ft_preference=FP_LEFT; break;
		// case KEY_RIGHT:				ft_preference=FP_RIGHT; break;
		// case KEY_UP:		ft_preference=FP_UP; break;
		// case KEY_DOWN:		ft_preference=FP_DOWN; break;

#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_LAPOSTRO: 
			Show_view_text_timer = 0x30000; 
			ObjectGotoNextViewer(); 
			break;
		case KEY_DEBUGGED+KEY_CTRLED+KEY_LAPOSTRO: 
			Show_view_text_timer = 0x30000; 
			ObjectGotoPrevViewer(); 
			break;
#endif
		case KEY_DEBUGGED+KEY_SHIFTED+KEY_LAPOSTRO: 
			gameData.objs.viewer=gameData.objs.console; 
			break;

	#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_O: 
			ToggleOutlineMode(); 
			break;
	#endif
		case KEY_DEBUGGED+KEY_T:
			*Toggle_var = !*Toggle_var;
#if TRACE
			con_printf (CON_DEBUG, "Variable at %08x set to %i\n", Toggle_var, *Toggle_var);
#endif
			break;
		case KEY_DEBUGGED + KEY_L:
			if (++gameStates.render.nLighting >= 2) 
				gameStates.render.nLighting = 0; 
			break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_L:
			Beam_brightness=0x38000-Beam_brightness; 
			break;
		case KEY_PAD5: 
			slew_stop(); 
			break;

#ifndef NDEBUG
		case KEY_DEBUGGED + KEY_F11: 
			play_test_sound(); 
			break;
		case KEY_DEBUGGED + KEY_SHIFTED+KEY_F11: 
			advance_sound(); 
			play_test_sound(); 
			break;
#endif

		case KEY_DEBUGGED +KEY_F4: {
			//fvi_info hit_data;
			//vms_vector p0 = {-0x1d99a7, -0x1b20000, 0x186ab7f};
			//vms_vector p1 = {-0x217865, -0x1b20000, 0x187de3e};
			//FindVectorIntersection(&hit_data, &p0, 0x1b9, &p1, 0x40000, 0x0, NULL, -1);
			break;
		}

		case KEY_DEBUGGED + KEY_M:
			gameStates.app.bDebugSpew = !gameStates.app.bDebugSpew;
			if (gameStates.app.bDebugSpew) {
				mopen( 0, 8, 1, 78, 16, "Debug Spew");
				HUDInitMessage( "Debug Spew: ON" );
			} else {
				mclose( 0 );
				HUDInitMessage( "Debug Spew: OFF" );
			}
			break;

		case KEY_DEBUGGED + KEY_C:

			FullPaletteSave();
			DoCheatMenu();
			PaletteRestore();
			break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_A:
			DoMegaWowPowerup(10);
			break;
		case KEY_DEBUGGED + KEY_A:	{
			DoMegaWowPowerup(200);
//								if ( gameData.app.nGameMode & GM_MULTI )     {
//									ExecMessageBox( NULL, 1, "Damn", "CHEATER!\nYou cannot use the\nmega-thing in network mode." );
//									multiData.msg.nReceiver = 100;		// Send to everyone...
//									sprintf( multiData.msg.szMsg, "%s cheated!", gameData.multi.players [gameData.multi.nLocalPlayer].callsign);
//								} else {
//									DoMegaWowPowerup();
//								}
						break;
		}

		case KEY_DEBUGGED+KEY_F:	
		gameStates.render.frameRate.value = !gameStates.render.frameRate.value; 
		break;

		case KEY_DEBUGGED+KEY_SPACEBAR:		//KEY_F7:				// Toggle physics flying
			slew_stop();
			GameFlushInputs();
			if ( gameData.objs.console->control_type != CT_FLYING ) {
				FlyInit(gameData.objs.console);
				gameStates.app.bGameSuspended &= ~SUSP_ROBOTS;	//robots move
			} else {
				slew_init(gameData.objs.console);			//start player slewing
				gameStates.app.bGameSuspended |= SUSP_ROBOTS;	//robots don't move
			}
			break;

		case KEY_DEBUGGED+KEY_COMMA: 
			nRenderZoom = FixMul(nRenderZoom, 62259); 
			break;
		case KEY_DEBUGGED+KEY_PERIOD: 
			nRenderZoom = FixMul(nRenderZoom, 68985); 
			break;

		case KEY_DEBUGGED+KEY_P+KEY_SHIFTED: 
			Debug_pause = 1; 
			break;

		//case KEY_F7: {
		//	char mystr[30];
		//	sprintf(mystr, "mark %i start", Mark_count);
		//	_MARK_(mystr);
		//	break;
		//}
		//case KEY_SHIFTED+KEY_F7: {
		//	char mystr[30];
		//	sprintf(mystr, "mark %i end", Mark_count);
		//	Mark_count++;
		//	_MARK_(mystr);
		//	break;
		//}


		#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_F8: 
			speedtest_init(); 
			Speedtest_count = 1;	 
			break;
		case KEY_DEBUGGED+KEY_F9: 
			speedtest_init(); 
			Speedtest_count = 10;	 
			break;

		case KEY_DEBUGGED+KEY_D:
			if ((Game_double_buffer = !Game_double_buffer)!=0)
				InitCockpit();
			break;
		#endif

		#ifdef EDITOR
		case KEY_DEBUGGED+KEY_Q:
			StopTime();
			dump_used_textures_all();
			StartTime();
			break;
		#endif

		case KEY_DEBUGGED+KEY_B: {
			newmenu_item m;
			char text[FILENAME_LEN]="";
			int item;
			memset (&m, 0, sizeof (m));
			m.type=NM_TYPE_INPUT; 
			m.text_len = FILENAME_LEN; 
			m.text = text;
			item = ExecMenu( NULL, "Briefing to play?", 1, &m, NULL, NULL );
			if (item != -1) {
				DoBriefingScreens(text, 1);
				ResetCockpit();
			}
			break;
		}

		case KEY_DEBUGGED+KEY_F5:
			toggle_movie_saving();
			break;

		case KEY_DEBUGGED+KEY_SHIFTED+KEY_F5: {
			extern int Movie_fixed_frametime;
			Movie_fixed_frametime = !Movie_fixed_frametime;
			break;
		}

		case KEY_DEBUGGED+KEY_ALTED+KEY_F5:
			gameData.app.xGameTime = i2f(0x7fff - 840);		//will overflow in 14 minutes
#if TRACE
			con_printf (CON_DEBUG, "gameData.app.xGameTime bashed to %d secs\n", f2i(gameData.app.xGameTime));
#endif
			break;

		case KEY_DEBUGGED+KEY_SHIFTED+KEY_B:
			KillEverything();
			break;
	}
}
#endif		//#ifndef RELEASE

//	Cheat functions ------------------------------------------------------------

char old_IntMethod;
char OldHomingState[20];

//	Testing functions ----------------------------------------------------------

#ifndef NDEBUG
void speedtest_init(void)
{
	Speedtest_start_time = TimerGetFixedSeconds();
	Speedtest_on = 1;
	Speedtest_segnum = 0;
	Speedtest_sidenum = 0;
	Speedtest_frame_start = gameData.app.nFrameCount;
#if TRACE
	con_printf (CON_DEBUG, "Starting speedtest.  Will be %i frames.  Each . = 10 frames.\n", gameData.segs.nLastSegment+1);
#endif
}

//------------------------------------------------------------------------------

void speedtest_frame(void)
{
	vms_vector	view_dir, center_point;

	Speedtest_sidenum=Speedtest_segnum % MAX_SIDES_PER_SEGMENT;

	COMPUTE_SEGMENT_CENTER(&gameData.objs.viewer->pos, &gameData.segs.segments[Speedtest_segnum]);
	gameData.objs.viewer->pos.x += 0x10;		gameData.objs.viewer->pos.y -= 0x10;		gameData.objs.viewer->pos.z += 0x17;

	RelinkObject(OBJ_IDX (gameData.objs.viewer), Speedtest_segnum);
	COMPUTE_SIDE_CENTER(&center_point, &gameData.segs.segments[Speedtest_segnum], Speedtest_sidenum);
	VmVecNormalizedDirQuick(&view_dir, &center_point, &gameData.objs.viewer->pos);
	VmVector2Matrix(&gameData.objs.viewer->orient, &view_dir, NULL, NULL);

	if (((gameData.app.nFrameCount - Speedtest_frame_start) % 10) == 0) {
#if TRACE
		con_printf (CON_DEBUG, ".");
#endif
		}
	Speedtest_segnum++;

	if (Speedtest_segnum > gameData.segs.nLastSegment) {
		char    msg[128];

		sprintf(msg, TXT_SPEEDTEST, 
			gameData.app.nFrameCount-Speedtest_frame_start, 
			f2fl(TimerGetFixedSeconds() - Speedtest_start_time), 
			(double) (gameData.app.nFrameCount-Speedtest_frame_start) / f2fl(TimerGetFixedSeconds() - Speedtest_start_time));
#if TRACE
		con_printf (CON_DEBUG, "%s", msg);
#endif
		HUDInitMessage(msg);

		Speedtest_count--;
		if (Speedtest_count == 0)
			Speedtest_on = 0;
		else
			speedtest_init();
	}

}

//------------------------------------------------------------------------------
//	Sounds [gameStates.app.bD1Data] for testing

int test_sound_num = 0;
int sound_nums[] = {10, 11, 20, 21, 30, 31, 32, 33, 40, 41, 50, 51, 60, 61, 62, 70, 80, 81, 82, 83, 90, 91};

#define N_TEST_SOUNDS (sizeof(sound_nums) / sizeof(*sound_nums))

//------------------------------------------------------------------------------

void advance_sound()
{
	if (++test_sound_num == N_TEST_SOUNDS)
		test_sound_num=0;

}

//------------------------------------------------------------------------------

short Test_sound = 251;

void play_test_sound()
{

	// -- DigiPlaySample(sound_nums[test_sound_num], F1_0);
	DigiPlaySample(Test_sound, F1_0);
}

#endif  //ifndef NDEBUG

//------------------------------------------------------------------------------

int ReadControls()
{
	int key, skipControls = 0;
	fix key_time;
	static ubyte exploding_flag=0;

	gameStates.app.bPlayerFiredLaserThisFrame=-1;

	if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead) {

			if ( (gameData.demo.nState == ND_STATE_PLAYBACK) || (gameData.marker.nDefiningMsg)
				#ifdef NETWORK
				|| multiData.msg.bSending || multiData.msg.bDefining
				#endif
				)	 // WATCH OUT!!! WEIRD CODE ABOVE!!!
				memset( &Controls, 0, sizeof(control_info) );
			else
				#ifdef WINDOWS
					controls_read_all_win();
				#else
					skipControls = ControlsReadAll();		//NOTE LINK TO ABOVE!!!
				#endif

		CheckRearView();

		//	If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
		if ( Controls.automapDownCount && !gameStates.gameplay.bSpeedBoost && !((gameData.app.nGameMode & GM_MULTI) && gameData.reactor.bDestroyed && (gameData.reactor.countdown.nSecsLeft < 10)))
			gameStates.app.bAutoMap = 1;

		do_weapon_stuff();

	}

	if (gameStates.app.bPlayerExploded) { //gameStates.app.bPlayerIsDead && (gameData.objs.console->flags & OF_EXPLODING) ) {

		if (exploding_flag==0)  {
			exploding_flag = 1;			// When player starts exploding, clear all input devices...
			GameFlushInputs();
		} else {
			int i;
			//if (keyDownCount(KEY_BACKSP))
			//	Int3();
			//if (keyDownCount(KEY_PRINT_SCREEN))
			//	SaveScreenShot (NULL, 0);

			for (i=0; i<4; i++ )
				if (joy_get_button_down_cnt(i)>0) 
					gameStates.app.bDeathSequenceAborted = 1;
			for (i=0; i<3; i++ )
				if (MouseButtonDownCount(i)>0) 
					gameStates.app.bDeathSequenceAborted = 1;

			//for (i=0; i<256; i++ )
			//	if (!key_isfunc(i) && !key_ismod(i) && keyDownCount(i)>0) gameStates.app.bDeathSequenceAborted = 1;

			if (gameStates.app.bDeathSequenceAborted)
				GameFlushInputs();
		}
	} else {
		exploding_flag=0;
	}

	if (gameData.demo.nState == ND_STATE_PLAYBACK )
		update_vcr_state();

	while ((key=KeyInKeyTime(&key_time)) != 0)    {

		if (gameData.marker.nDefiningMsg)
		 {
			MarkerInputMessage (key);
			continue;
		 }

		#ifdef NETWORK
		if ( (gameData.app.nGameMode&GM_MULTI) && (multiData.msg.bSending || multiData.msg.bDefining ))	{
			MultiMsgInputSub( key );
			continue;		//get next key
		}
		#endif

		#ifndef RELEASE
		#ifdef NETWORK
		if ((key&KEY_DEBUGGED)&&(gameData.app.nGameMode&GM_MULTI))   {
			multiData.msg.nReceiver = 100;		// Send to everyone...
			sprintf( multiData.msg.szMsg, "%s %s", TXT_I_AM_A, TXT_CHEATER);
		}
		#endif
		#endif

#ifdef CONSOLE
		if(!con_events(key))
			continue;
#endif

		if (gameStates.app.bPlayerIsDead)
			HandleDeathKey(key);

		if (gameStates.app.bEndLevelSequence)
			HandleEndlevelKey(key);
		else if (gameData.demo.nState == ND_STATE_PLAYBACK ) {
			HandleDemoKey(key);

			#ifndef RELEASE
			HandleTestKey(key);
			#endif
		} else {
			FinalCheats(key);

			HandleSystemKey(key);
			HandleVRKey(key);
			HandleGameKey(key);

			#ifndef RELEASE
			HandleTestKey(key);
			#endif
		}
	}


//	if ((gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CONVERTER) && keyd_pressed[KEY_F8] && (keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT]))
  //		TransferEnergyToShield(KeyDownTime(KEY_F8);
return skipControls;
}

//------------------------------------------------------------------------------
//eof
