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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

//#define DOORDBGGING

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
#include "network.h"
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

//------------------------------------------------------------------------------
//#define TEST_TIMER    1		//if this is set, do checking on timer

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

int	redbookVolume = 255;


//	External Variables ---------------------------------------------------------

extern char bWaitForRefuseAnswer, bRefuseThisPlayer, bRefuseTeam;

extern int	*Toggle_var;
extern int	Debug_pause;

extern fix	ShowView_textTimer;

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
extern int	CreateSpecialPath(void);
extern void MovePlayerToSegment(tSegment *seg, int tSide);
extern void	kconfig_center_headset(void);
extern void GameRenderFrameMono(void);
extern void NewDemoStripFrames(char *, int);
extern void ToggleCockpit(void);
extern int  dump_used_textures_all(void);
extern void DropMarker();
extern void DropSecondaryWeapon(int nWeapon);
extern void DropCurrentWeapon();

void FinalCheats(int key);

void HandleGameKey(int key);
int HandleSystemKey(int key);
void HandleTestKey(int key);
void HandleVRKey(int key);

void SpeedtestInit(void);
void SpeedtestFrame(void);
void AdvanceSound(void);
void PlayTestSound(void);

#define key_isfunc(k) (((k&0xff)>=KEY_F1 && (k&0xff)<=KEY_F10) || (k&0xff)==KEY_F11 || (k&0xff)==KEY_F12)
#define key_ismod(k)  ((k&0xff)==KEY_LALT || (k&0xff)==KEY_RALT || (k&0xff)==KEY_LSHIFT || (k&0xff)==KEY_RSHIFT || (k&0xff)==KEY_LCTRL || (k&0xff)==KEY_RCTRL)

// Functions ------------------------------------------------------------------

#define CONVERTER_RATE  20		//10 units per second xfer rate
#define CONVERTER_SCALE  2		//2 units energy -> 1 unit shields

#define CONVERTER_SOUND_DELAY (f1_0/2)		//play every half second

void TransferEnergyToShield(fix time)
{
	fix e;		//how much energy gets transfered
	static fix last_playTime=0;

	if (time <= 0)
		return;
	e = min(min(time*CONVERTER_RATE, LOCALPLAYER.energy - INITIAL_ENERGY), 
		         (MAX_SHIELDS-LOCALPLAYER.shields)*CONVERTER_SCALE);
	if (e <= 0) {
		if (LOCALPLAYER.energy <= INITIAL_ENERGY)
			HUDInitMessage(TXT_TRANSFER_ENERGY, f2i(INITIAL_ENERGY));
		else
			HUDInitMessage(TXT_TRANSFER_SHIELDS);
		return;
	}

	LOCALPLAYER.energy  -= e;
	LOCALPLAYER.shields += e/CONVERTER_SCALE;
	MultiSendShields ();
	gameStates.app.bUsingConverter = 1;
	if (last_playTime > gameData.time.xGame)
		last_playTime = 0;

	if (gameData.time.xGame > last_playTime+CONVERTER_SOUND_DELAY) {
		DigiPlaySampleOnce(SOUND_CONVERT_ENERGY, F1_0);
		last_playTime = gameData.time.xGame;
	}

}

//------------------------------------------------------------------------------

void update_vcrState();
void DoWeaponStuff(void);

//------------------------------------------------------------------------------
// Control Functions

fix newdemo_single_frameTime;

void update_vcrState(void)
{
if (gameOpts->demo.bRevertFormat && (gameData.demo.nVersion > DEMO_VERSION))
	return;
if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_RIGHT])
	gameData.demo.nVcrState = ND_STATE_FASTFORWARD;
else if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_LEFT])
	gameData.demo.nVcrState = ND_STATE_REWINDING;
else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_RIGHT] && ((TimerGetFixedSeconds() - newdemo_single_frameTime) >= F1_0))
	gameData.demo.nVcrState = ND_STATE_ONEFRAMEFORWARD;
else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_LEFT] && ((TimerGetFixedSeconds() - newdemo_single_frameTime) >= F1_0))
	gameData.demo.nVcrState = ND_STATE_ONEFRAMEBACKWARD;
#if 0
else if ((gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || (gameData.demo.nVcrState == ND_STATE_REWINDING))
	gameData.demo.nVcrState = ND_STATE_PLAYBACK;
#endif
}

//------------------------------------------------------------------------------
//returns which bomb will be dropped next time the bomb key is pressed
int ArmedBomb()
{
	int bomb, otherBomb;

	//use the last one selected, unless there aren't any, in which case use
	//the other if there are any
   // If hoard game, only let the tPlayer drop smart mines
if (gameData.app.nGameMode & GM_ENTROPY)
   return PROXMINE_INDEX; //allow for dropping orbs
if (gameData.app.nGameMode & GM_HOARD)
	return SMARTMINE_INDEX;

bomb = bLastSecondaryWasSuper [PROXMINE_INDEX] ? SMARTMINE_INDEX : PROXMINE_INDEX;
otherBomb = SMARTMINE_INDEX + PROXMINE_INDEX - bomb;

if (!LOCALPLAYER.secondaryAmmo [bomb] && LOCALPLAYER.secondaryAmmo [otherBomb]) {
	bomb = otherBomb;
	bLastSecondaryWasSuper [bomb % SUPER_WEAPON] = (bomb == SMARTMINE_INDEX);
	}
return bomb;
}

//------------------------------------------------------------------------------

void DoWeaponStuff (void)
{
  int i;

if (Controls [0].useCloakDownCount)
	ApplyCloak (0, -1);
if (Controls [0].useInvulDownCount)
	ApplyInvul (0, -1);
if (Controls [0].fireFlareDownCount)
	if (AllowedToFireFlare ())
		CreateFlare(gameData.objs.console);
if (AllowedToFireMissile ()) {
	i = secondaryWeaponToWeaponInfo [gameData.weapons.nSecondary];
	gameData.missiles.nGlobalFiringCount += WI_fireCount (i) * (Controls [0].fireSecondaryState || Controls [0].fireSecondaryDownCount);
	}
if (gameData.missiles.nGlobalFiringCount) {
	DoMissileFiring (1);			//always enable autoselect for normal missile firing
	gameData.missiles.nGlobalFiringCount--;
	}
if (Controls [0].cyclePrimaryCount) {
	for (i = 0; i < Controls [0].cyclePrimaryCount; i++)
	CyclePrimary ();
	}
if (Controls [0].cycleSecondaryCount) {
	for (i = 0; i < Controls [0].cycleSecondaryCount; i++)
	CycleSecondary ();
	}
if (Controls [0].headlightCount) {
	for (i = 0; i < Controls [0].headlightCount; i++)
	toggle_headlight_active ();
	}
if (gameData.missiles.nGlobalFiringCount < 0)
	gameData.missiles.nGlobalFiringCount = 0;
//	Drop proximity bombs.
if (Controls [0].dropBombDownCount) {
	if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.console->nSegment].special == SEGMENT_IS_NODAMAGE))
		Controls [0].dropBombDownCount = 0;
	else {
		int ssw_save = gameData.weapons.nSecondary;
		while (Controls [0].dropBombDownCount--) {
			int ssw_save2 = gameData.weapons.nSecondary = ArmedBomb();
			if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))
				DropSecondaryWeapon (-1);
			else
				DoMissileFiring (gameData.weapons.nSecondary == ssw_save);	//only allow autoselect if bomb is actually selected
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
extern void show_extraViews();

void ApplyModifiedPalette(void)
{
//@@    int				k, x, y;
//@@    grsBitmap	*sbp;
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

void formatTime(char *str, int secs_int)
{
	int h, m, s;

	h = secs_int/3600;
	s = secs_int%3600;
	m = s / 60;
	s = s % 60;
	sprintf(str, "%1d:%02d:%02d", h, m, s );
}

//------------------------------------------------------------------------------

void PauseGame (void)
{
if (!gameData.app.bGamePaused) {
	gameData.app.bGamePaused = 1;
	DigiPauseAll();
	RBAPause();
	StopTime();
	PaletteSave();
	ApplyModifiedPalette();
	ResetPaletteAdd();
	GameFlushInputs();
#if defined (TACTILE)
	if (TactileStick)
		DisableForces();
#endif
	}
}

//------------------------------------------------------------------------------

void ResumeGame (void)
{
GameFlushInputs();
ResetCockpit();
PaletteRestore();
StartTime();
if (gameStates.sound.bRedbookPlaying)
	RBAResume();
DigiResumeAll();
gameData.app.bGamePaused = 0;
}

//------------------------------------------------------------------------------

void DoShowNetgameHelp();

//Process selected keys until game unpaused. returns key that left pause (p or esc)
int DoGamePause()
{
	int key;
	int bScreenChanged;
	char msg[1000];
	char totalTime[9], xLevelTime[9];

	key=0;

if (gameData.app.bGamePaused) {		//unpause!
	gameData.app.bGamePaused = 0;
	gameStates.app.bEnterGame = 1;
#if defined (TACTILE)
	if (TactileStick)
		EnableForces();
#endif
	return KEY_PAUSE;
	}

if (gameData.app.nGameMode & GM_NETWORK) {
	 DoShowNetgameHelp();
    return (KEY_PAUSE);
	}
else if (gameData.app.nGameMode & GM_MULTI) {
	HUDInitMessage (TXT_MODEM_PAUSE);
	return (KEY_PAUSE);
	}
PauseGame ();
SetPopupScreenMode();
GrPaletteStepLoad (NULL);
formatTime(totalTime, f2i(LOCALPLAYER.timeTotal) + LOCALPLAYER.hoursTotal*3600);
formatTime(xLevelTime, f2i(LOCALPLAYER.timeLevel) + LOCALPLAYER.hoursLevel*3600);
  if (gameData.demo.nState!=ND_STATE_PLAYBACK)
	sprintf(msg, TXT_PAUSE_MSG1, GAMETEXT (332 + gameStates.app.nDifficultyLevel), 
			  LOCALPLAYER.hostages_on_board, xLevelTime, totalTime);
   else
	  	sprintf(msg, TXT_PAUSE_MSG2, GAMETEXT (332 +  gameStates.app.nDifficultyLevel), 
				  LOCALPLAYER.hostages_on_board);

if (!gameOpts->menus.nStyle) {
	gameStates.menus.nInMenu++;
	GameRenderFrame();
	gameStates.menus.nInMenu--;
	ShowBoxedMessage(Pause_msg=msg);		  //TXT_PAUSE);
	}
GrabMouse (0, 0);
while (gameData.app.bGamePaused) {
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
#ifdef _DEBUG
		HandleTestKey(key);
#endif
		bScreenChanged = HandleSystemKey(key);
		HandleVRKey(key);
		if (bScreenChanged) {
			GameRenderFrame();
			WIN(SetPopupScreenMode());
			ShowBoxedMessage(msg);
#if 0			
			show_extraViews();
			if (gameStates.render.cockpit.nMode==CM_FULL_COCKPIT || gameStates.render.cockpit.nMode==CM_STATUS_BAR)
				RenderGauges();
#endif				
			}
		}
	GrabMouse (1, 0);
	if (gameStates.render.vr.nScreenFlags & VRF_COMPATIBLE_MENUS) {
		ClearBoxedMessage();
	}
ResumeGame ();
return key;
}

//------------------------------------------------------------------------------

extern int NetworkWhoIsMaster(), NetworkHowManyConnected(), GetMyNetRanking();
extern char Pauseable_menu;
//char *NetworkModeNames[]={"Anarchy", "Team Anarchy", "Robo Anarchy", "Cooperative", "Capture the Flag", "Hoard", "Team Hoard", "Unknown"};
extern int PhallicLimit, PhallicMan;

void DoShowNetgameHelp()
 {
	tMenuItem m[30];
   char mtext[30][60];
	int i, num=0, eff;
#ifdef _DEBUG
	int pl;
#endif
	//char *eff_strings[]={"trashing", "really hurting", "seriously affecting", "hurting", "affecting", "tarnishing"};

	memset (m, 0, sizeof (m));
   for (i=0;i<30;i++)
	{
	 m[i].text=(char *)&mtext[i];
    m[i].nType=NM_TYPE_TEXT;
	}

   sprintf (mtext[num], TXT_INFO_GAME, netGame.szGameName); num++;
   sprintf (mtext[num], TXT_INFO_MISSION, netGame.szMissionTitle); num++;
	sprintf (mtext[num], TXT_INFO_LEVEL, netGame.nLevel); num++;
	sprintf (mtext[num], TXT_INFO_SKILL, MENU_DIFFICULTY_TEXT (netGame.difficulty)); num++;
	sprintf (mtext[num], TXT_INFO_MODE, GT (537 + netGame.gameMode)); num++;
	sprintf (mtext[num], TXT_INFO_SERVER, gameData.multiplayer.players [NetworkWhoIsMaster()].callsign); num++;
   sprintf (mtext[num], TXT_INFO_PLRNUM, NetworkHowManyConnected(), netGame.nMaxPlayers); num++;
   sprintf (mtext[num], TXT_INFO_PPS, netGame.nPacketsPerSec); num++;
   sprintf (mtext[num], TXT_INFO_SHORTPKT, netGame.bShortPackets ? "Yes" : "No"); num++;
#ifdef _DEBUG
		pl=(int)(((double)networkData.nTotalMissedPackets/(double)networkData.nTotalPacketsGot)*100.0);
		if (pl<0)
		  pl=0;
		sprintf (mtext[num], TXT_INFO_LOSTPKT, networkData.nTotalMissedPackets, pl); num++;
#endif
   if (netGame.KillGoal)
     { sprintf (mtext[num], TXT_INFO_KILLGOAL, netGame.KillGoal*5); num++; }
   sprintf (mtext[num], " "); num++;
   sprintf (mtext[num], TXT_INFO_PLRSCONN); num++;
   netPlayers.players [gameData.multiplayer.nLocalPlayer].rank=GetMyNetRanking();
   for (i=0;i<gameData.multiplayer.nPlayers;i++)
     if (gameData.multiplayer.players [i].connected)
	  {		  
      if (!gameOpts->multi.bNoRankings)
		 {
			if (i==gameData.multiplayer.nLocalPlayer)
				sprintf (mtext[num], "%s%s (%d/%d)", 
							pszRankStrings[netPlayers.players [i].rank], 
							gameData.multiplayer.players [i].callsign, 
							networkData.nNetLifeKills, 
							networkData.nNetLifeKilled); 
			else
				sprintf (mtext[num], "%s%s %d/%d", 
							pszRankStrings[netPlayers.players [i].rank], 
							gameData.multiplayer.players [i].callsign, 
							gameData.multigame.kills.matrix[gameData.multiplayer.nLocalPlayer][i], 
							gameData.multigame.kills.matrix[i][gameData.multiplayer.nLocalPlayer]); 
			num++;
		 }
	   else
  		 sprintf (mtext[num++], "%s", gameData.multiplayer.players [i].callsign); 
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
		 sprintf (mtext[num], TXT_RECORD3, gameData.multiplayer.players [PhallicMan].callsign, PhallicLimit); 
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

//------------------------------------------------------------------------------

void HandleEndlevelKey(int key)
{

	if ( key == (KEY_COMMAND + KEY_SHIFTED + KEY_P) )
		SaveScreenShot (NULL, 0);

	if (key==KEY_PRINT_SCREEN)
		SaveScreenShot (NULL, 0);

	if ( key == (KEY_COMMAND+KEY_P) )
		key = DoGamePause();

	if (key == KEY_PAUSE)
		key = DoGamePause();		//so esc from pause will end level

	if (key == KEY_ESC) {
		StopEndLevelSequence();
		gameStates.render.cockpit.nLastDrawn[0] =
		gameStates.render.cockpit.nLastDrawn[1] = -1;
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
//		key = DoGamePause();
		gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
	}

	if (key == KEY_PAUSE)   {
//		key = DoGamePause();		//so esc from pause will end level
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

void HandleDemoKey(int key)
{
if (gameOpts->demo.bRevertFormat && (gameData.demo.nVersion > DEMO_VERSION))
	return;
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
		gameData.multigame.kills.bShowList = (gameData.multigame.kills.bShowList+1) % ((gameData.demo.nGameMode & GM_TEAM) ? 4 : 3);
		break;
			
	case KEY_CTRLED+KEY_F7:
		if ((gameStates.render.cockpit.bShowPingStats = !gameStates.render.cockpit.bShowPingStats))
			ResetPingStats ();
		break;

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
		newdemo_single_frameTime = TimerGetFixedSeconds();
		gameData.demo.nVcrState = ND_STATE_ONEFRAMEBACKWARD;
		break;
	
	case KEY_RIGHT:
		newdemo_single_frameTime = TimerGetFixedSeconds();
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
		DoGamePause();
		break;

	case KEY_COMMAND + KEY_SHIFTED + KEY_P:
	case KEY_PRINT_SCREEN: {
		int oldState = gameData.demo.nVcrState;
		gameData.demo.nVcrState = ND_STATE_PRINTSCREEN;
		//GameRenderFrameMono();
		bSaveScreenShot = 1;
		SaveScreenShot (NULL, 0);
		gameData.demo.nVcrState = oldState;
		break;
		}

	#ifdef _DEBUG
	case KEY_BACKSP:
		Int3();
		break;
	case KEYDBGGED + KEY_I:
		gameData.demo.bInterpolate = !gameData.demo.bInterpolate;
#if TRACE
		if (gameData.demo.bInterpolate)
			con_printf (CONDBG, "demo playback interpolation now on\n");
		else
			con_printf (CONDBG, "demo playback interpolation now off\n");
#endif
		break;
	case KEYDBGGED + KEY_K: {
		int how_many, c;
		char filename[FILENAME_LEN], num[16];
		tMenuItem m[6];

		filename[0] = '\0';
		memset (m, 0, sizeof (m));
		m[0].nType = NM_TYPE_TEXT; 
		m[0].text = "output file name";
		m[1].nType = NM_TYPE_INPUT;
		m[1].text_len = 8; 
		m[1].text = filename;
		c = ExecMenu( NULL, NULL, 2, m, NULL, NULL );
		if (c == -2)
			break;
		strcat(filename, ".dem");
		num[0] = '\0';
		m[ 0].nType = NM_TYPE_TEXT; m[ 0].text = "strip how many bytes";
		m[ 1].nType = NM_TYPE_INPUT;m[ 1].text_len = 16; m[1].text = num;
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

	switch (gameStates.render.cockpit.n3DView[w]) {
		case CV_NONE:
			gameStates.render.cockpit.n3DView[w] = CV_REAR;
			break;
		case CV_REAR:
			if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
			    (!(gameData.app.nGameMode & GM_MULTI) || (netGame.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
				gameStates.render.cockpit.n3DView[w] = CV_RADAR_TOPDOWN;
				break;
				}
		case CV_RADAR_TOPDOWN:
			if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
			    (!(gameData.app.nGameMode & GM_MULTI) || (netGame.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
				gameStates.render.cockpit.n3DView[w] = CV_RADAR_HEADSUP;
				break;
				}
		case CV_RADAR_HEADSUP:
			if (find_escort()) {
				gameStates.render.cockpit.n3DView[w] = CV_ESCORT;
				break;
			}
			//if no ecort, fall through
		case CV_ESCORT:
			gameStates.render.cockpit.nCoopPlayerView[w] = -1;		//force first tPlayer
			//fall through
		case CV_COOP:
			gameData.marker.viewers[w] = -1;
			if ((gameData.app.nGameMode & GM_MULTI_COOP) || (gameData.app.nGameMode & GM_TEAM)) {
				gameStates.render.cockpit.n3DView[w] = CV_COOP;
				while (1) {
					gameStates.render.cockpit.nCoopPlayerView[w]++;
					if (gameStates.render.cockpit.nCoopPlayerView[w] == gameData.multiplayer.nPlayers) {
						gameStates.render.cockpit.n3DView[w] = CV_MARKER;
						goto case_marker;
					}
					if (gameStates.render.cockpit.nCoopPlayerView[w]==gameData.multiplayer.nLocalPlayer)
						continue;

					if (gameData.app.nGameMode & GM_MULTI_COOP)
						break;
					else if (GetTeam(gameStates.render.cockpit.nCoopPlayerView[w]) == GetTeam(gameData.multiplayer.nLocalPlayer))
						break;
				}
				break;
			}
			//if not multi, fall through
		case CV_MARKER:
		case_marker:;
			if (!(gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_COOP) || netGame.bAllowMarkerView) {	//anarchy only
				gameStates.render.cockpit.n3DView[w] = CV_MARKER;
				if (gameData.marker.viewers [w] == -1)
					gameData.marker.viewers [w] = gameData.multiplayer.nLocalPlayer * 2;
				else if (gameData.marker.viewers [w] == gameData.multiplayer.nLocalPlayer * 2)
					gameData.marker.viewers [w]++;
				else
					gameStates.render.cockpit.n3DView[w] = CV_NONE;
			}
			else
				gameStates.render.cockpit.n3DView[w] = CV_NONE;
			break;
	}
	WritePlayerFile();

	return 1;	 //bScreenChanged
}

//------------------------------------------------------------------------------


void SongsGotoNextSong();
void SongsGotoPrevSong();

#ifdef DOORDBGGING
dump_door_debugging_info()
{
	tObject *objP;
	vmsVector new_pos;
	tVFIQuery fq;
	tFVIData hit_info;
	int fate;
	FILE *dfile;
	int wall_numn;

	obj = gameData.objs.objects + LOCALPLAYER.nObject;
	VmVecScaleAdd(&new_pos, &objP->position.vPos, &objP->position.mOrient.fVec, i2f(100);

	fq.p0						= &objP->position.vPos;
	fq.startseg				= objP->nSegment;
	fq.p1						= &new_pos;
	fq.rad					= 0;
	fq.thisobjnum			= LOCALPLAYER.nObject;
	fq.ignore_obj_list	= NULL;
	fq.flags					= 0;

	fate = FindVectorIntersection(&fq, &hit_info);

	dfile = fopen("door.out", "at");

	fprintf(dfile, "FVI hitType = %d\n", fate);
	fprintf(dfile, "    hit_seg = %d\n", hit_info.hit_seg);
	fprintf(dfile, "    hit_side = %d\n", hit_info.hit_side);
	fprintf(dfile, "    hit_side_seg = %d\n", hit_info.hit_side_seg);
	fprintf(dfile, "\n");

	if (fate == HIT_WALL) {

		nWall = WallNumI (hit_info.hit_seg, hit_info.hit_side);
		fprintf(dfile, "nWall = %d\n", nWall);
	
		if (IS_WALL (nWall)) {
			tWall *tWall = gameData.walls.walls + nWall;
			tActiveDoor *d;
			int i;
	
			fprintf(dfile, "    nSegment = %d\n", tWall->nSegment);
			fprintf(dfile, "    nSide = %d\n", tWall->nSide);
			fprintf(dfile, "    hps = %x\n", tWall->hps);
			fprintf(dfile, "    nLinkedWall = %d\n", tWall->nLinkedWall);
			fprintf(dfile, "    nType = %d\n", tWall->nType);
			fprintf(dfile, "    flags = %x\n", tWall->flags);
			fprintf(dfile, "    state = %d\n", tWall->state);
			fprintf(dfile, "    tTrigger = %d\n", tWall->nTrigger);
			fprintf(dfile, "    nClip = %d\n", tWall->nClip);
			fprintf(dfile, "    keys = %x\n", tWall->keys);
			fprintf(dfile, "    controllingTrigger = %d\n", tWall->controllingTrigger);
			fprintf(dfile, "    cloakValue = %d\n", tWall->cloakValue);
			fprintf(dfile, "\n");
	
	
			for (i=0;i<gameData.walls.nOpenDoors;i++) {		//find door
				d = &gameData.walls.activeDoors[i];
				if (d->nFrontWall[0]==nWall || 
					 d->nBackWall[0]==nWall || 
					 (d->nPartCount==2 && (d->nFrontWall[1]==nWall || d->nBackWall[1]==nWall)))
					break;
			} 
	
			if (i>=gameData.walls.nOpenDoors)
				fprintf(dfile, "No active door.\n");
			else {
				fprintf(dfile, "Active door %d:\n", i);
				fprintf(dfile, "    nPartCount = %d\n", d->nPartCount);
				fprintf(dfile, "    nFrontWall = %d, %d\n", d->nFrontWall[0], d->nFrontWall[1]);
				fprintf(dfile, "    nBackWall = %d, %d\n", d->nBackWall[0], d->nBackWall[1]);
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
	int bScreenChanged = 0;
	int bStopPlayerMovement = 1;

	//if (gameStates.gameplay.bSpeedBoost)
	//	return 0;

if (!gameStates.app.bPlayerIsDead)
	switch (key) {

#ifdef DOORDBGGING
		case KEY_LAPOSTRO+KEY_SHIFTED:
			dump_door_debugging_info();
			break;
#endif

		case KEY_ESC:
			if (gameData.app.bGamePaused) {
				gameData.app.bGamePaused = 0;
				}
			else {
				gameStates.app.bGameAborted = 1;
				SetFunctionMode (FMODE_MENU);
			}
			break;

		case KEY_SHIFTED+KEY_F1:
			bScreenChanged = select_next_window_function(0);
			break;
		case KEY_SHIFTED+KEY_F2:
			bScreenChanged = select_next_window_function(1);
			break;
		case KEY_SHIFTED+KEY_F3:
			gameOpts->render.cockpit.nWindowSize = (gameOpts->render.cockpit.nWindowSize + 1) % 4;
			bScreenChanged = 1; //select_next_window_function(1);
			break;
		case KEY_CTRLED+KEY_F3:
			gameOpts->render.cockpit.nWindowPos = (gameOpts->render.cockpit.nWindowPos + 1) % 6;
			bScreenChanged = 1; //select_next_window_function(1);
			break;
		case KEY_SHIFTED+KEY_CTRLED+KEY_F3:
			gameOpts->render.cockpit.nWindowZoom = (gameOpts->render.cockpit.nWindowZoom + 1) % 4;
			bScreenChanged = 1; //select_next_window_function(1);
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

		gameStates.app.bGameAborted = 1;
		gameStates.app.nFunctionMode = FMODE_EXIT;
		break;
#endif

	case KEY_COMMAND+KEY_P: 
	case KEY_CTRLED+KEY_P: 
	case KEY_PAUSE: 
		DoGamePause();				
		break;

	case KEY_CTRLED + KEY_ALTED + KEY_S:
		if ((IsMultiGame && !IsCoopGame) || !gameStates.app.bEnableFreeCam)
			return 0;
		if ((gameStates.app.bFreeCam = !gameStates.app.bFreeCam))
			gameStates.app.playerPos = gameData.objs.viewer->position;
		else
			gameData.objs.viewer->position = gameStates.app.playerPos;
		break;

	case KEY_COMMAND + KEY_SHIFTED + KEY_P:
	case KEY_PRINT_SCREEN: 
		bSaveScreenShot = 1;
		SaveScreenShot (NULL, 0);		
		break;

	case KEY_F1:					
		DoShowHelp();			
		break;

	case KEY_F2:					//gameStates.app.bConfigMenu = 1; break;
		{
			int bScanlineSave = bScanlineDouble;

			if (!IsMultiGame) {
				PaletteSave(); 
				ApplyModifiedPalette(); 
				ResetPaletteAdd(); 
				GrPaletteStepLoad (NULL); 
				}
			ConfigMenu();
			if (!IsMultiGame) 
				PaletteRestore();
			if (bScanlineSave != bScanlineDouble)   
				InitCockpit();	// reset the cockpit after changing...
	      PA_DFX (InitCockpit());
			break;
		}


	case KEY_F3:
		PA_DFX (HUDInitMessage (TXT_3DFX_COCKPIT));
		PA_DFX (break);

		if (!GuidedInMainView ()) {
			ToggleCockpit();	
			bScreenChanged=1;
		}
		break;

	case KEY_F7+KEY_SHIFTED: 
		PaletteSave(); 
		joydefs_calibrate(); 
		PaletteRestore(); 
		break;

	case KEY_SHIFTED+KEY_MINUS:
	case KEY_MINUS:	
		shrink_window(); 
		bScreenChanged=1; 
		break;

	case KEY_SHIFTED+KEY_EQUAL:
	case KEY_EQUAL:			
		grow_window();  
		bScreenChanged=1; 
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
		gameData.multigame.bShowReticleName = (gameData.multigame.bShowReticleName+1)%2;

	case KEY_F7:
		gameData.multigame.kills.bShowList = (gameData.multigame.kills.bShowList+1) % ((gameData.app.nGameMode & GM_TEAM) ? 4 : 3);
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSortKillList();
		bStopPlayerMovement = 0;
		break;

	case KEY_CTRLED+KEY_F7:
		if ((gameStates.render.cockpit.bShowPingStats = !gameStates.render.cockpit.bShowPingStats))
			ResetPingStats ();
		break;

	case KEY_CTRLED+KEY_F8:
		gameData.stats.nDisplayMode = (gameData.stats.nDisplayMode + 1) % 5;
		gameOpts->render.cockpit.bPlayerStats = gameData.stats.nDisplayMode != 0;
		break;

	case KEY_F8:
		MultiSendMsgStart(-1);
		bStopPlayerMovement = 0;
		break;

	case KEY_F9:
	case KEY_F10:
	case KEY_F11:
	case KEY_F12:
		MultiSendMacro(key);
		bStopPlayerMovement = 0;
		break;		// send taunt macros

	case KEY_CTRLED + KEY_F12:
		gameData.trackIR.x =
		gameData.trackIR.y = 0;
		break;

	case KEY_ALTED + KEY_F12:
#ifndef _DEBUG		
		if (!IsMultiGame || IsCoopGame || EGI_FLAG (bEnableCheats, 0, 0, 0))
#endif			
			gameStates.render.bExternalView = !gameStates.render.bExternalView;
		ResetFlightPath (&externalView, -1, -1);
		break;

	case KEY_SHIFTED + KEY_F9:
	case KEY_SHIFTED + KEY_F10:
	case KEY_SHIFTED + KEY_F11:
	case KEY_SHIFTED + KEY_F12:
		MultiDefineMacro(key);
		bStopPlayerMovement = 0;
		break;		// redefine taunt macros

	case KEY_ALTED+KEY_F2:
		if (!gameStates.app.bPlayerIsDead && !(IsMultiGame && !IsCoopGame)) {
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
		if (!gameStates.app.bPlayerIsDead && !(IsMultiGame && !IsCoopGame)) {
			FullPaletteSave ();
			StateRestoreAll (1, 0, NULL);
			if (gameData.app.bGamePaused)
				DoGamePause();
		}
		break;


	case KEY_F4 + KEY_SHIFTED:
		DoEscortMenu();
		break;


	case KEY_F4 + KEY_SHIFTED + KEY_ALTED:
		ChangeGuidebotName();
		break;

	case KEY_MINUS + KEY_ALTED:     
		SongsGotoPrevSong(); 
		break;

	case KEY_EQUAL + KEY_ALTED:     
		SongsGotoNextSong(); 
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
			return bScreenChanged;
	}	 //switch (key)
if (bStopPlayerMovement) {
	StopPlayerMovement ();
	gameStates.app.bEnterGame = 2;
	}
return bScreenChanged;
}

//------------------------------------------------------------------------------

void HandleVRKey(int key)
{
	switch( key )   {

		case KEY_ALTED+KEY_F5:
			if ( gameStates.render.vr.nRenderMode != VR_NONE )	{
				VRResetParams();
				HUDInitMessage( TXT_VR_RESET );
				HUDInitMessage( TXT_VR_SEPARATION, f2fl(gameStates.render.vr.xEyeWidth) );
				HUDInitMessage( TXT_VR_BALANCE, (double)gameStates.render.vr.nEyeOffset/30.0 );
			}
			break;

		case KEY_ALTED+KEY_F6:
			if ( gameStates.render.vr.nRenderMode != VR_NONE )	{
				gameStates.render.vr.nLowRes++;
				if ( gameStates.render.vr.nLowRes > 3 ) gameStates.render.vr.nLowRes = 0;
				switch( gameStates.render.vr.nLowRes )    {
					case 0: HUDInitMessage( TXT_VR_NORMRES ); break;
					case 1: HUDInitMessage( TXT_VR_LOWVRES ); break;
					case 2: HUDInitMessage( TXT_VR_LOWHRES ); break;
					case 3: HUDInitMessage( TXT_VR_LOWRES ); break;
				}
			}
			break;

		case KEY_ALTED+KEY_F7:
			if ( gameStates.render.vr.nRenderMode != VR_NONE )	{
				gameStates.render.vr.nEyeSwitch = !gameStates.render.vr.nEyeSwitch;
				HUDInitMessage( TXT_VR_TOGGLE );
				if ( gameStates.render.vr.nEyeSwitch )
					HUDInitMessage( TXT_VR_RLEYE );
				else
					HUDInitMessage( TXT_VR_LREYE );
			}
			break;

		case KEY_ALTED+KEY_F8:
			if ( gameStates.render.vr.nRenderMode != VR_NONE )	{
			gameStates.render.vr.nSensitivity++;
			if (gameStates.render.vr.nSensitivity > 2 )
				gameStates.render.vr.nSensitivity = 0;
			HUDInitMessage( TXT_VR_SENSITIVITY, gameStates.render.vr.nSensitivity );
		 }
			break;
		case KEY_ALTED+KEY_F9:
			if ( gameStates.render.vr.nRenderMode != VR_NONE )	{
				gameStates.render.vr.xEyeWidth -= F1_0/10;
				if ( gameStates.render.vr.xEyeWidth < 0 ) gameStates.render.vr.xEyeWidth = 0;
				HUDInitMessage( TXT_VR_SEPARATION, f2fl(gameStates.render.vr.xEyeWidth) );
				HUDInitMessage( TXT_VR_DEFAULT, f2fl(VR_SEPARATION) );
			}
			break;
		case KEY_ALTED+KEY_F10:
			if ( gameStates.render.vr.nRenderMode != VR_NONE )	{
				gameStates.render.vr.xEyeWidth += F1_0/10;
				if ( gameStates.render.vr.xEyeWidth > F1_0*4 )    gameStates.render.vr.xEyeWidth = F1_0*4;
				HUDInitMessage( TXT_VR_SEPARATION, f2fl(gameStates.render.vr.xEyeWidth) );
				HUDInitMessage( TXT_VR_DEFAULT, f2fl(VR_SEPARATION) );
			}
			break;

		case KEY_ALTED+KEY_F11:
			if ( gameStates.render.vr.nRenderMode != VR_NONE )	{
				gameStates.render.vr.nEyeOffset--;
				if ( gameStates.render.vr.nEyeOffset < -30 )	gameStates.render.vr.nEyeOffset = -30;
				HUDInitMessage( TXT_VR_BALANCE, (double)gameStates.render.vr.nEyeOffset/30.0 );
				HUDInitMessage( TXT_VR_DEFAULT, (double)VR_PIXEL_SHIFT/30.0 );
				gameStates.render.vr.bEyeOffsetChanged = 2;
			}
			break;
		case KEY_ALTED+KEY_F12:
			if ( gameStates.render.vr.nRenderMode != VR_NONE )	{
				gameStates.render.vr.nEyeOffset++;
				if ( gameStates.render.vr.nEyeOffset > 30 )	 gameStates.render.vr.nEyeOffset = 30;
				HUDInitMessage( TXT_VR_BALANCE, (double)gameStates.render.vr.nEyeOffset/30.0 );
				HUDInitMessage( TXT_VR_DEFAULT, (double)VR_PIXEL_SHIFT/30.0 );
				gameStates.render.vr.bEyeOffsetChanged = 2;
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

		case KEY_0 + KEY_ALTED:
			DropFlag ();
			break;

		case KEY_F4:
		if (!gameData.marker.nDefiningMsg)
		  InitMarkerInput();
		 break;

		case KEY_ALTED + KEY_CTRLED + KEY_T:
			SwitchTeam (gameData.multiplayer.nLocalPlayer, 0);
			break;
		case KEY_F6:
			if (netGame.bRefusePlayers && bWaitForRefuseAnswer && !(gameData.app.nGameMode & GM_TEAM))
				{
					bRefuseThisPlayer=1;
					HUDInitMessage (TXT_ACCEPT_PLR);
				}
			break;
		case KEY_ALTED + KEY_1:
			if (netGame.bRefusePlayers && bWaitForRefuseAnswer && (gameData.app.nGameMode & GM_TEAM))
				{
					bRefuseThisPlayer=1;
					HUDInitMessage (TXT_ACCEPT_PLR);
					bRefuseTeam=1;
				}
			break;
		case KEY_ALTED + KEY_2:
			if (netGame.bRefusePlayers && bWaitForRefuseAnswer && (gameData.app.nGameMode & GM_TEAM))
				{
					bRefuseThisPlayer=1;
					HUDInitMessage (TXT_ACCEPT_PLR);
					bRefuseTeam=2;
				}
			break;

		default:
			break;

	}	 //switch (key)
}

//	--------------------------------------------------------------------------

void toggle_movie_saving(void);
extern char Language[];

#ifdef _DEBUG

void HandleTestKey(int key)
{
	switch (key) {

		case KEYDBGGED+KEY_0:	
			ShowWeaponStatus();   break;

#ifdef SHOW_EXIT_PATH
		case KEYDBGGED+KEY_1:	
			CreateSpecialPath();  
			break;
#endif

		case KEYDBGGED+KEY_Y:
			DoReactorDestroyedStuff(NULL);
			break;

	case KEYDBGGED+KEY_ALTED+KEY_D:
			networkData.nNetLifeKills=4000; 
			networkData.nNetLifeKilled=5;
			MultiAddLifetimeKills();
			break;

		case KEY_BACKSP:
		case KEY_CTRLED+KEY_BACKSP:
		case KEY_ALTED+KEY_BACKSP:
		case KEY_ALTED+KEY_CTRLED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_ALTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_CTRLED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_CTRLED+KEY_ALTED+KEY_BACKSP:
			Int3(); 
			break;

		case KEY_CTRLED+KEY_ALTED+KEY_ENTER:
			exit (0);
			break;

		case KEYDBGGED+KEY_S:				
			DigiReset(); 
			break;

		case KEYDBGGED+KEY_P:
			if (gameStates.app.bGameSuspended & SUSP_ROBOTS)
				gameStates.app.bGameSuspended &= ~SUSP_ROBOTS;		//robots move
			else
				gameStates.app.bGameSuspended |= SUSP_ROBOTS;		//robots don't move
			break;


		case KEYDBGGED+KEY_K:	
			LOCALPLAYER.shields = 1;	
			MultiSendShields ();
			break;				
						//	a virtual kill
		case KEYDBGGED+KEY_SHIFTED + KEY_K:  
			LOCALPLAYER.shields = -1;	 
			MultiSendShields ();
			break;  //	an actual kill
			
		case KEYDBGGED+KEY_X: 
			LOCALPLAYER.lives++; 
			break; // Extra life cheat key.
			
		case KEYDBGGED+KEY_H:
//				if (!(gameData.app.nGameMode & GM_MULTI) )   {
				LOCALPLAYER.flags ^= PLAYER_FLAGS_CLOAKED;
				if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
					if (gameData.app.nGameMode & GM_MULTI)
						MultiSendCloak();
					AIDoCloakStuff();
					LOCALPLAYER.cloakTime = gameData.time.xGame;
#if TRACE
					con_printf (CONDBG, "You are cloaked!\n");
#endif
				} else
#if TRACE
					con_printf (CONDBG, "You are DE-cloaked!\n");
#endif
//				}
			break;


		case KEYDBGGED+KEY_R:
			gameStates.app.cheats.bRobotsFiring = !gameStates.app.cheats.bRobotsFiring;
			break;

		case KEYDBGGED+KEY_R+KEY_SHIFTED:
			KillAllRobots (1);
			break;

		#ifdef EDITOR		//editor-specific functions

		case KEY_E + KEYDBGGED:
			NetworkLeaveGame();
			SetFunctionMode (FMODE_EDITOR);
			break;
	case KEY_Q + KEY_SHIFTED + KEYDBGGED:
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
		case KEY_C + KEY_SHIFTED + KEYDBGGED:
			if (!( gameData.app.nGameMode & GM_MULTI ))
				MovePlayerToSegment(Cursegp, Curside);
			break;   //move eye to curseg


		case KEYDBGGED+KEY_W:	
			DrawWorldFromGame(); 
			break;

		#endif  //#ifdef EDITOR

		//flythrough keys
		// case KEYDBGGED+KEY_SHIFTED+KEY_F: toggle_flythrough(); break;
		// case KEY_LEFT:		ft_preference=FP_LEFT; break;
		// case KEY_RIGHT:				ft_preference=FP_RIGHT; break;
		// case KEY_UP:		ft_preference=FP_UP; break;
		// case KEY_DOWN:		ft_preference=FP_DOWN; break;

#ifdef _DEBUG
		case KEYDBGGED+KEY_LAPOSTRO: 
			ShowView_textTimer = 0x30000; 
			ObjectGotoNextViewer(); 
			break;
		case KEYDBGGED+KEY_CTRLED+KEY_LAPOSTRO: 
			ShowView_textTimer = 0x30000; 
			ObjectGotoPrevViewer(); 
			break;
#endif
		case KEYDBGGED+KEY_SHIFTED+KEY_LAPOSTRO: 
			gameData.objs.viewer=gameData.objs.console; 
			break;

	#ifdef _DEBUG
		case KEYDBGGED+KEY_O: 
			ToggleOutlineMode(); 
			break;
	#endif
		case KEYDBGGED+KEY_T:
			*Toggle_var = !*Toggle_var;
#if TRACE
			con_printf (CONDBG, "Variable at %08x set to %i\n", Toggle_var, *Toggle_var);
#endif
			break;
		case KEYDBGGED + KEY_L:
			if (++gameStates.render.nLighting >= 2) 
				gameStates.render.nLighting = 0; 
			break;
		case KEYDBGGED + KEY_SHIFTED + KEY_L:
			Beam_brightness=0x38000-Beam_brightness; 
			break;
		case KEY_PAD5: 
			slew_stop(); 
			break;

#ifdef _DEBUG
		case KEYDBGGED + KEY_F11: 
			PlayTestSound(); 
			break;
		case KEYDBGGED + KEY_SHIFTED+KEY_F11: 
			AdvanceSound(); 
			PlayTestSound(); 
			break;
#endif

		case KEYDBGGED +KEY_F4: {
			//tFVIData hit_data;
			//vmsVector p0 = {-0x1d99a7, -0x1b20000, 0x186ab7f};
			//vmsVector p1 = {-0x217865, -0x1b20000, 0x187de3e};
			//FindVectorIntersection(&hit_data, &p0, 0x1b9, &p1, 0x40000, 0x0, NULL, -1);
			break;
		}

		case KEYDBGGED + KEY_M:
			gameStates.app.bDebugSpew = !gameStates.app.bDebugSpew;
			if (gameStates.app.bDebugSpew) {
				mopen( 0, 8, 1, 78, 16, "Debug Spew");
				HUDInitMessage( "Debug Spew: ON" );
			} else {
				mclose( 0 );
				HUDInitMessage( "Debug Spew: OFF" );
			}
			break;

		case KEYDBGGED + KEY_C:

			FullPaletteSave();
			DoCheatMenu();
			PaletteRestore();
			break;
		case KEYDBGGED + KEY_SHIFTED + KEY_A:
			DoMegaWowPowerup(10);
			break;
		case KEYDBGGED + KEY_A:	{
			DoMegaWowPowerup(200);
//								if ( gameData.app.nGameMode & GM_MULTI )     {
//									ExecMessageBox( NULL, 1, "Damn", "CHEATER!\nYou cannot use the\nmega-thing in network mode." );
//									gameData.multigame.msg.nReceiver = 100;		// Send to everyone...
//									sprintf( gameData.multigame.msg.szMsg, "%s cheated!", LOCALPLAYER.callsign);
//								} else {
//									DoMegaWowPowerup();
//								}
						break;
		}

		case KEYDBGGED+KEY_F:	
		gameStates.render.frameRate.value = !gameStates.render.frameRate.value; 
		break;

		case KEYDBGGED+KEY_SPACEBAR:		//KEY_F7:				// Toggle physics flying
			slew_stop();
			GameFlushInputs();
			if ( gameData.objs.console->controlType != CT_FLYING ) {
				FlyInit(gameData.objs.console);
				gameStates.app.bGameSuspended &= ~SUSP_ROBOTS;	//robots move
			} else {
				slew_init(gameData.objs.console);			//start tPlayer slewing
				gameStates.app.bGameSuspended |= SUSP_ROBOTS;	//robots don't move
			}
			break;

		case KEYDBGGED+KEY_COMMA: 
			gameStates.render.xZoom = FixMul(gameStates.render.xZoom, 62259); 
			break;
		case KEYDBGGED+KEY_PERIOD: 
			gameStates.render.xZoom = FixMul(gameStates.render.xZoom, 68985); 
			break;

		case KEYDBGGED+KEY_P+KEY_SHIFTED: 
			Debug_pause = 1; 
			break;

#ifdef _DEBUG
		case KEYDBGGED+KEY_F8: 
			SpeedtestInit(); 
			gameData.speedtest.nCount = 1;	 
			break;
		case KEYDBGGED+KEY_F9: 
			SpeedtestInit(); 
			gameData.speedtest.nCount = 10;	 
			break;

		case KEYDBGGED+KEY_D:
			if ((bGameDoubleBuffer = !bGameDoubleBuffer))
				InitCockpit();
			break;
#endif

		#ifdef EDITOR
		case KEYDBGGED+KEY_Q:
			StopTime();
			dump_used_textures_all();
			StartTime();
			break;
		#endif

		case KEYDBGGED+KEY_B: {
			tMenuItem m;
			char text[FILENAME_LEN]="";
			int item;
			memset (&m, 0, sizeof (m));
			m.nType=NM_TYPE_INPUT; 
			m.text_len = FILENAME_LEN; 
			m.text = text;
			item = ExecMenu( NULL, "Briefing to play?", 1, &m, NULL, NULL );
			if (item != -1) {
				DoBriefingScreens(text, 1);
				ResetCockpit();
			}
			break;
		}

		case KEYDBGGED+KEY_F5:
			toggle_movie_saving();
			break;

		case KEYDBGGED+KEY_SHIFTED+KEY_F5: {
			extern int Movie_fixed_frametime;
			Movie_fixed_frametime = !Movie_fixed_frametime;
			break;
		}

		case KEYDBGGED+KEY_ALTED+KEY_F5:
			gameData.time.xGame = i2f(0x7fff - 840);		//will overflow in 14 minutes
#if TRACE
			con_printf (CONDBG, "gameData.time.xGame bashed to %d secs\n", f2i(gameData.time.xGame));
#endif
			break;

		case KEYDBGGED+KEY_SHIFTED+KEY_B:
			KillEverything (1);
			break;
	}
}
#endif		//#ifdef _DEBUG

//	Cheat functions ------------------------------------------------------------

char old_IntMethod;
char OldHomingState[20];

//	Testing functions ----------------------------------------------------------

#ifdef _DEBUG
void SpeedtestInit(void)
{
	gameData.speedtest.nStartTime = TimerGetFixedSeconds();
	gameData.speedtest.bOn = 1;
	gameData.speedtest.nSegment = 0;
	gameData.speedtest.nSide = 0;
	gameData.speedtest.nFrameStart = gameData.app.nFrameCount;
#if TRACE
	con_printf (CONDBG, "Starting speedtest.  Will be %i frames.  Each . = 10 frames.\n", gameData.segs.nLastSegment+1);
#endif
}

//------------------------------------------------------------------------------

void SpeedtestFrame(void)
{
	vmsVector	view_dir, center_point;

	gameData.speedtest.nSide=gameData.speedtest.nSegment % MAX_SIDES_PER_SEGMENT;

	COMPUTE_SEGMENT_CENTER(&gameData.objs.viewer->position.vPos, &gameData.segs.segments[gameData.speedtest.nSegment]);
	gameData.objs.viewer->position.vPos.p.x += 0x10;		
	gameData.objs.viewer->position.vPos.p.y -= 0x10;		
	gameData.objs.viewer->position.vPos.p.z += 0x17;

	RelinkObject(OBJ_IDX (gameData.objs.viewer), gameData.speedtest.nSegment);
	COMPUTE_SIDE_CENTER(&center_point, &gameData.segs.segments[gameData.speedtest.nSegment], gameData.speedtest.nSide);
	VmVecNormalizedDirQuick(&view_dir, &center_point, &gameData.objs.viewer->position.vPos);
	VmVector2Matrix(&gameData.objs.viewer->position.mOrient, &view_dir, NULL, NULL);

	if (((gameData.app.nFrameCount - gameData.speedtest.nFrameStart) % 10) == 0) {
#if TRACE
		con_printf (CONDBG, ".");
#endif
		}
	gameData.speedtest.nSegment++;

	if (gameData.speedtest.nSegment > gameData.segs.nLastSegment) {
		char    msg[128];

		sprintf(msg, TXT_SPEEDTEST, 
			gameData.app.nFrameCount-gameData.speedtest.nFrameStart, 
			f2fl(TimerGetFixedSeconds() - gameData.speedtest.nStartTime), 
			(double) (gameData.app.nFrameCount-gameData.speedtest.nFrameStart) / f2fl(TimerGetFixedSeconds() - gameData.speedtest.nStartTime));
#if TRACE
		con_printf (CONDBG, "%s", msg);
#endif
		HUDInitMessage(msg);

		gameData.speedtest.nCount--;
		if (gameData.speedtest.nCount == 0)
			gameData.speedtest.bOn = 0;
		else
			SpeedtestInit();
	}

}

//------------------------------------------------------------------------------
//	Sounds [gameStates.app.bD1Data] for testing

int testSound_num = 0;
int sound_nums[] = {10, 11, 20, 21, 30, 31, 32, 33, 40, 41, 50, 51, 60, 61, 62, 70, 80, 81, 82, 83, 90, 91};

#define N_TEST_SOUNDS (sizeof(sound_nums) / sizeof(*sound_nums))

//------------------------------------------------------------------------------

void AdvanceSound()
{
	if (++testSound_num == N_TEST_SOUNDS)
		testSound_num=0;

}

//------------------------------------------------------------------------------

short TestSound = 251;

void PlayTestSound()
{

	// -- DigiPlaySample(sound_nums[testSound_num], F1_0);
	DigiPlaySample(TestSound, F1_0);
}

#endif  //ifndef NDEBUG

//------------------------------------------------------------------------------

int ReadControls()
{
	int key, skipControls = 0;
	fix keyTime;
	static ubyte explodingFlag=0;

gameStates.app.bPlayerFiredLaserThisFrame=-1;
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead) {
		if ( (gameData.demo.nState == ND_STATE_PLAYBACK) || (gameData.marker.nDefiningMsg)
			|| gameData.multigame.msg.bSending || gameData.multigame.msg.bDefining
			)	 // WATCH OUT!!! WEIRD CODE ABOVE!!!
			memset( &Controls, 0, sizeof(tControlInfo) );
		else
			skipControls = ControlsReadAll();		//NOTE LINK TO ABOVE!!!
	CheckRearView();
	//	If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
	if (Controls [0].automapDownCount && 
		 !gameData.objs.speedBoost [OBJ_IDX (gameData.objs.console)].bBoosted && 
		 !((gameData.app.nGameMode & GM_MULTI) && gameData.reactor.bDestroyed && (gameData.reactor.countdown.nSecsLeft < 10)))
		gameStates.render.automap.bDisplay = 1;
	DoWeaponStuff();
	}
if (gameStates.app.bPlayerExploded) { //gameStates.app.bPlayerIsDead && (gameData.objs.console->flags & OF_EXPLODING) ) {
	if (explodingFlag==0)  {
		explodingFlag = 1;			// When tPlayer starts exploding, clear all input devices...
		GameFlushInputs();
		}
	else {
		int i;
		for (i = 0; i < 4; i++ )
			if (JoyGetButtonDownCnt (i) > 0) 
				gameStates.app.bDeathSequenceAborted = 1;
		for (i = 0; i < 3; i++ )
			if (MouseButtonDownCount (i) > 0) 
				gameStates.app.bDeathSequenceAborted = 1;
		if (gameStates.app.bDeathSequenceAborted)
			GameFlushInputs();
		}
	} 
else {
	explodingFlag = 0;
	}
if (gameData.demo.nState == ND_STATE_PLAYBACK )
	update_vcrState();
while ((key=KeyInKeyTime(&keyTime)) != 0)    {
	if (gameData.marker.nDefiningMsg) {
		MarkerInputMessage (key);
			continue;
		}
if ( IsMultiGame && (gameData.multigame.msg.bSending || gameData.multigame.msg.bDefining ))	{
	MultiMsgInputSub (key);
	continue;		//get next key
	}
#ifdef _DEBUG
if ((key&KEYDBGGED) && IsMultiGame) {
	gameData.multigame.msg.nReceiver = 100;		// Send to everyone...
	sprintf( gameData.multigame.msg.szMsg, "%s %s", TXT_I_AM_A, TXT_CHEATER);
	}
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
#ifdef _DEBUG
		HandleTestKey(key);
#endif
		}
	else {
		FinalCheats(key);
		HandleSystemKey(key);
		HandleVRKey(key);
		HandleGameKey(key);
#ifdef _DEBUG
		HandleTestKey(key);
#endif
		}
	}
return skipControls;
}

//------------------------------------------------------------------------------
//eof
