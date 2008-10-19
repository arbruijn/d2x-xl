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

char copyright[] = "DESCENT II  COPYRIGHT (C) 1994-1996 PARALLAX SOFTWARE CORPORATION";

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(__unix__) || defined(__macosx__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __macosx__
#	include "SDL/SDL_main.h"
#	include "SDL/SDL_keyboard.h"
#	include "FolderDetector.h"
#else
#	include "SDL_main.h"
#	include "SDL_keyboard.h"
#endif
#include "inferno.h"
#include "u_mem.h"
#include "strutil.h"
#include "key.h"
#include "timer.h"
#include "error.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "iff.h"
#include "pcx.h"
#include "args.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "sdlgl.h"
#include "text.h"
#include "newdemo.h"
#include "objrender.h"
#include "renderthreads.h"
#include "network.h"
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "joy.h"
#include "desc_id.h"
#include "joydefs.h"
#include "gamepal.h"
#include "movie.h"
#include "compbit.h"
#include "playsave.h"
#include "tracker.h"
#include "render.h"
#include "sphere.h"
#include "endlevel.h"
#include "interp.h"
#include "autodl.h"
#include "hiresmodels.h"
#include "soundthreads.h"

//#  include "3dfx_des.h"

//added on 9/30/98 by Matt Mueller for selectable automap modes
#include "automap.h"
//end addition -MM

#include "../texmap/scanline.h" //for select_tmap -MM

extern int SDL_HandleSpecialKeys;

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#include "ui.h"
#endif

#ifdef __macosx__
#	include <SDL/SDL.h>
#	if USE_SDL_MIXER
#		include <SDL/SDL_mixer.h>
#	endif
#else
#	include <SDL.h>
#	if USE_SDL_MIXER
#		include <SDL_mixer.h>
#	endif
#endif
#include "vers_id.h"

tGameOptions	gameOptions [2];
tGameStates		gameStates;
tGameData		gameData;

//static const char desc_id_checksum_str[] = DESC_ID_CHKSUM_TAG "0000"; // 4-byte checksum
char desc_id_exit_num = 0;

//--unused-- grsBitmap Inferno_bitmap_title;

int WVIDEO_running=0;		//debugger can set to 1 if running

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
#endif

void check_joystick_calibration (void);

void ShowOrderForm (void);
void quit_request ();

tGameOptions *gameOpts = gameOptions;

//--------------------------------------------------------------------------

char szAutoMission [255];
char szAutoHogFile [255];

extern void SetMaxPitch (int nMinTurnRate);

int nDescentCriticalError = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

#define LINE_LEN	100

// ----------------------------------------------------------------------------

#if defined (__unix__) || defined (__macosx__)
void D2SignalHandler (int nSignal)
#else
void __cdecl D2SignalHandler (int nSignal)
#endif
{
if (nSignal == SIGABRT)
	PrintLog ("Abnormal program termination\n");
else if (nSignal == SIGFPE)
	PrintLog ("Floating point error\n");
else if (nSignal == SIGILL)
	PrintLog ("Illegal instruction\n");
else if (nSignal == SIGINT)
	PrintLog ("Ctrl+C signal\n");
else if (nSignal == SIGSEGV)
	PrintLog ("Memory access violation\n");
else if (nSignal == SIGTERM)
	PrintLog ("Termination request\n");
else
	PrintLog ("Unknown signal\n");
#if !DBG
//exit (1);
#endif
}

// ----------------------------------------------------------------------------

void D2SetCaption (void)
{
#if USE_IRRLICHT
	char		szCaption [200];
	wchar_t	wszCaption [200];
	int		i;

strcpy (szCaption, DESCENT_VERSION);
if (*LOCALPLAYER.callsign) {
	strcat (szCaption, " [");
	strcat (szCaption, LOCALPLAYER.callsign);
	strcat (szCaption, "]");
	strupr (szCaption);
	}
if (*gameData.missions.szCurrentLevel) {
	strcat (szCaption, " - ");
	strcat (szCaption, gameData.missions.szCurrentLevel);
	}
for (i = 0; szCaption [i]; i++)
	wszCaption [i] = (wchar_t) szCaption [i];
wszCaption [i] = (wchar_t) 0;
IRRDEVICE->setWindowCaption (wszCaption);
#else
	char	szCaption [200];
	strcpy (szCaption, DESCENT_VERSION);
if (*LOCALPLAYER.callsign) {
	strcat (szCaption, " [");
	strcat (szCaption, LOCALPLAYER.callsign);
	strcat (szCaption, "]");
	strupr (szCaption);
	}
if (*gameData.missions.szCurrentLevel) {
	strcat (szCaption, " - ");
	strcat (szCaption, gameData.missions.szCurrentLevel);
	}
SDL_WM_SetCaption (szCaption, "Descent II");
#endif
}

// ----------------------------------------------------------------------------

#define SUBVER_XOFFS	 (gameStates.menus.bHires ? 45 : 25)

void PrintVersionInfo (void)
{
	static int bVertigo = -1;

	int y, w, ws, h, hs, aw;

if (gameStates.app.bGameRunning || gameStates.app.bBetweenLevels)
	return;
if (gameStates.menus.bHires) {
	if (gameOpts->menus.altBg.bHave > 0)
		y = 8; //102
	else {
		y = (88 * (gameStates.render.vr.nScreenSize % 65536)) / 480;
		if (y < 88)
			y = 88;
		}
	}
else
	y = 37;

gameStates.menus.bDrawCopyright = 0;
GrSetCurrentCanvas (NULL);
GrSetCurFont (GAME_FONT);
GrGetStringSize ("V2.2", &w, &h, &aw);
GrSetFontColorRGBi (RGBA_PAL (63, 47, 0), 1, 0, 0);
GrPrintF (NULL, 0x8000, grdCurCanv->cvBitmap.bmProps.h - GAME_FONT->ftHeight - 2, "visit www.descent2.de");
GrSetFontColorRGBi (RGBA_PAL (23, 23, 23), 1, 0, 0);
GrPrintF (NULL, 0x8000, grdCurCanv->cvBitmap.bmProps.h - GAME_FONT->ftHeight - h - 6, TXT_COPYRIGHT);
GrPrintF (NULL, grdCurCanv->cvBitmap.bmProps.w - w - 2, 
			 grdCurCanv->cvBitmap.bmProps.h - GAME_FONT->ftHeight - h - 6, "V%d.%d", D2X_MAJOR, D2X_MINOR);
if (bVertigo < 0)
	bVertigo = CFExist ("d2x.hog", gameFolders.szMissionDir, 0);
if (bVertigo) {
	GrSetCurFont (MEDIUM2_FONT);
	GrGetStringSize (TXT_VERTIGO, &w, &h, &aw);
	GrPrintF (NULL, grdCurCanv->cvBitmap.bmProps.w - w - SUBVER_XOFFS, 
				 y + (gameOpts->menus.altBg.bHave ? h + 2 : 0), TXT_VERTIGO);
	}
GrSetCurFont (MEDIUM2_FONT);
GrGetStringSize (D2X_NAME, &w, &h, &aw);
GrPrintF (NULL, grdCurCanv->cvBitmap.bmProps.w - w - SUBVER_XOFFS, 
			 y + ((bVertigo && !gameOpts->menus.altBg.bHave) ? h + 2 : 0), D2X_NAME);
GrSetCurFont (SMALL_FONT);
GrGetStringSize (VERSION, &ws, &hs, &aw);
GrSetFontColorRGBi (D2BLUE_RGBA, 1, 0, 0);
GrPrintF (NULL, grdCurCanv->cvBitmap.bmProps.w - ws - 1, 
			 y + ((bVertigo && !gameOpts->menus.altBg.bHave) ? h + 2 : 0) + (h - hs) / 2, VERSION);
GrSetFontColorRGBi (RGBA_PAL (6, 6, 6), 1, 0, 0);
}

// ----------------------------------------------------------------------------
//read help from a file & print to screen
void PrintCmdLineHelp ()
{
	CFILE cf;
	int bHaveBinary = 0;
	char line[LINE_LEN];

	if (!(CFOpen (&cf, "help.tex", gameFolders.szDataDir, "rb", 0) || 
			CFOpen (&cf, "help.txb", gameFolders.szDataDir, "rb", 0))) {
			Warning (TXT_NO_HELP);
		bHaveBinary = 1;
	}
	if (cf.file) {
		while (CFGetS (line, LINE_LEN, &cf)) {
			if (bHaveBinary) {
				int i;
				for (i = 0; i < (int) strlen (line) - 1; i++) {
					line[i] = EncodeRotateLeft ((char) (EncodeRotateLeft (line [i]) ^ BITMAP_TBL_XOR));
				}
			}
			if (line[0] == ';')
				continue;		//don't show comments
		}
		CFClose (&cf);
	}

	con_printf ((int) con_threshold.value, " D2X Options:\n\n");
	con_printf ((int) con_threshold.value, "  -noredundancy   %s\n", "Do not send messages when picking up redundant items in multi");
	con_printf ((int) con_threshold.value, "  -shortpackets   %s\n", "Set shortpackets to default as on");
	con_printf ((int) con_threshold.value, "  -gameOpts->render.nMaxFPS <n>     %s\n", "Set maximum framerate (1-100)");
	con_printf ((int) con_threshold.value, "  -notitles       %s\n", "Do not show titlescreens on startup");
	con_printf ((int) con_threshold.value, "  -hogdir <dir>   %s\n", "set shared data directory to <dir>");
#if defined(__unix__) || defined(__macosx__)
	con_printf ((int) con_threshold.value, "  -nohogdir       %s\n", "don't try to use shared data directory");
	con_printf ((int) con_threshold.value, "  -userdir <dir>  %s\n", "set user dir to <dir> instead of $HOME/.d2x");
#endif
	con_printf ((int) con_threshold.value, "  -ini <file>     %s\n", "option file (alternate to command line), defaults to d2x.ini");
	con_printf ((int) con_threshold.value, "  -autodemo       %s\n", "Start in demo mode");
	con_printf ((int) con_threshold.value, "  -bigpig         %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -bspgen         %s\n", "FIXME: Undocumented");
//	con_printf ((int) con_threshold.value, "  -cdproxy        %s\n", "FIXME: Undocumented");
#if DBG
	con_printf ((int) con_threshold.value, "  -checktime      %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -showmeminfo    %s\n", "FIXME: Undocumented");
#endif
	con_printf ((int) con_threshold.value, "  -debug          %s\n", "Enable very verbose output");
#ifdef SDL_INPUT
	con_printf ((int) con_threshold.value, "  -grabmouse      %s\n", "Keeps the mouse from wandering out of the window");
#endif
#if DBG
	con_printf ((int) con_threshold.value, "  -invulnerability %s\n", "Make yourself invulnerable");
#endif
	con_printf ((int) con_threshold.value, "  -ipxnetwork <num> %s\n", "Use IPX network number <num>");
	con_printf ((int) con_threshold.value, "  -jasen          %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -joyslow        %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -kali           %s\n", "use Kali for networking");
	con_printf ((int) con_threshold.value, "  -lowresmovies   %s\n", "Play low resolution movies if available (for slow machines)");
#if defined (EDITOR) || !defined (MACDATA)
	con_printf ((int) con_threshold.value, "  -macdata        %s\n", "Read (and, for editor, write) mac data files (swap colors)");
#endif
	con_printf ((int) con_threshold.value, "  -nocdrom        %s\n", "FIXME: Undocumented");
#ifdef __DJGPP__
	con_printf ((int) con_threshold.value, "  -nocyberman     %s\n", "FIXME: Undocumented");
#endif
#if DBG
	con_printf ((int) con_threshold.value, "  -nofade         %s\n", "Disable fades");
#endif
	con_printf ((int) con_threshold.value, "  -nomatrixcheat  %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -norankings     %s\n", "Disable multiplayer ranking system");
	con_printf ((int) con_threshold.value, "  -packets <num>  %s\n", "Specifies the number of packets per second\n");
	con_printf ((int) con_threshold.value, "  -socket         %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -nomixer        %s\n", "Don't crank music volume");
	con_printf ((int) con_threshold.value, "  -sound22k       %s\n", "Use 22 KHz sound sample rate");
	con_printf ((int) con_threshold.value, "  -sound11k       %s\n", "Use 11 KHz sound sample rate");
#if DBG
	con_printf ((int) con_threshold.value, "  -nomovies       %s\n", "Don't play movies");
	con_printf ((int) con_threshold.value, "  -noscreens      %s\n", "Skip briefing screens");
#endif
	con_printf ((int) con_threshold.value, "  -noredbook      %s\n", "Disable redbook audio");
	con_printf ((int) con_threshold.value, "  -norun          %s\n", "Bail out after initialization");
#ifdef TACTILE
	con_printf ((int) con_threshold.value, "  -stickmag       %s\n", "FIXME: Undocumented");
#endif
	con_printf ((int) con_threshold.value, "  -subtitles      %s\n", "Turn on movie subtitles (English-only)");
	con_printf ((int) con_threshold.value, "  -text <file>    %s\n", "Specify alternate .tex file");
	con_printf ((int) con_threshold.value, "  -udp            %s\n", "Use TCP/UDP for networking");
	con_printf ((int) con_threshold.value, "  -xcontrol       %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -xname          %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -xver           %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -tmap <t>       %s\n", "select texmapper to use (c, fp, i386, pent, ppro)");
	con_printf ((int) con_threshold.value, "  -<X>x<Y>        %s\n", "Set screen resolution to <X> by <Y>");
	con_printf ((int) con_threshold.value, "  -automap<X>x<Y> %s\n", "Set automap resolution to <X> by <Y>");
	con_printf ((int) con_threshold.value, "  -automap_gameres %s\n", "Set automap to use the same resolution as in game");
	con_printf ((int) con_threshold.value, "\n");

	con_printf ((int) con_threshold.value, "D2X System Options:\n\n");
	con_printf ((int) con_threshold.value, "  -fullscreen     %s\n", "Use fullscreen mode if available");
	con_printf ((int) con_threshold.value, "  -gl_texmagfilt <f> %s\n", "set GL_TEXTURE_MAG_FILTER (see readme.d1x)");
	con_printf ((int) con_threshold.value, "  -gl_texminfilt <f> %s\n", "set GL_TEXTURE_MIN_FILTER (see readme.d1x)");
	con_printf ((int) con_threshold.value, "  -gl_mipmap      %s\n", "set gl texture filters to \"standard\" options for mipmapping");
	con_printf ((int) con_threshold.value, "  -gl_trilinear   %s\n", "set gl texture filters to trilinear mipmapping");
	con_printf ((int) con_threshold.value, "  -gl_simple      %s\n", "set gl texture filters to gl_nearest for \"original\" look. (default)");
	con_printf ((int) con_threshold.value, "  -gl_alttexmerge %s\n", "use new texmerge, usually uses less ram (default)");
	con_printf ((int) con_threshold.value, "  -gl_stdtexmerge %s\n", "use old texmerge, uses more ram, but _might_ be a bit faster");
	con_printf ((int) con_threshold.value, "  -gl_16bittextures %s\n", "attempt to use 16bit textures");
	con_printf ((int) con_threshold.value, "  -ogl_reticle <r> %s\n", "use OGL reticle 0=never 1=above 320x* 2=always");
	con_printf ((int) con_threshold.value, "  -gl_intensity4_ok %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -gl_luminance4_alpha4_ok %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -gl_readpixels_ok %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -gl_rgba2_ok    %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -gl_test2       %s\n", "FIXME: Undocumented");
	con_printf ((int) con_threshold.value, "  -gl_vidmem      %s\n", "FIXME: Undocumented");
#ifdef OGL_RUNTIME_LOAD
	con_printf ((int) con_threshold.value, "  -gl_library <l> %s\n", "use alternate opengl library");
#endif
#ifdef SDL_VIDEO
	con_printf ((int) con_threshold.value, "  -nosdlvidmodecheck %s\n", "Some X servers don't like checking vidmode first, so just switch");
	con_printf ((int) con_threshold.value, "  -hwsurface      %s\n", "FIXME: Undocumented");
#endif
#if defined(__unix__) || defined(__macosx__)
	con_printf ((int) con_threshold.value, "  -serialdevice <s> %s\n", "Set serial/modem device to <s>");
	con_printf ((int) con_threshold.value, "  -serialread <r> %s\n\n", "Set serial/modem to read from <r>");
#endif
	con_printf ((int) con_threshold.value, "  -legacyfuelcens   %s\n", "Turn off repair centers");
	con_printf ((int) con_threshold.value, "  -legacyhomers     %s\n", "Turn off frame-rate independant homing missile turn rate");
	con_printf ((int) con_threshold.value, "  -legacyinput      %s\n", "Turn off enhanced input handling");
	con_printf ((int) con_threshold.value, "  -legacymouse      %s\n", "Turn off frame-rate independant mouse sensitivity");
	con_printf ((int) con_threshold.value, "  -legacyrender     %s\n", "Turn off colored tSegment rendering");
	con_printf ((int) con_threshold.value, "  -legacyzbuf       %s\n", "Turn off OpenGL depth buffer");
	con_printf ((int) con_threshold.value, "  -legacyswitches   %s\n", "Turn off fault-tolerant switch handling");
	con_printf ((int) con_threshold.value, "  -legacywalls      %s\n", "Turn off fault-tolerant tWall handling");
	con_printf ((int) con_threshold.value, "  -legacymode       %s\n", "Turn off all of the above non-legacy behaviour\n");
	con_printf ((int) con_threshold.value, "  -joydeadzone <n>  %s\n", "Set joystick deadzone to <n> percent (0 <= n <= 100)\n");
	con_printf ((int) con_threshold.value, "  -limitturnrate    %s\n", "Limit max. turn speed in single player mode like in multiplayer\n");
	con_printf ((int) con_threshold.value, "  -friendlyfire <n> %s\n", "Turn friendly fire on (1) or off (0)\n");
	con_printf ((int) con_threshold.value, "  -fixedrespawns    %s\n", "Have OBJECTS always respawn at their initial location\n");
	con_printf ((int) con_threshold.value, "  -respawndelay <n> %s\n", "Delay respawning of OBJECTS for <n> secs (0 <= n <= 60)\n");
	con_printf ((int) con_threshold.value, "\n Help:\n\n");
	con_printf ((int) con_threshold.value, "  -help, -h, -?, ? %s\n", "View this help screen");
	con_printf ((int) con_threshold.value, "\n");
}

// ----------------------------------------------------------------------------

void CheckEndian (void)
{
	union {
		short	s;
		unsigned char	b [2];
	} h;

h.b [0] = 0;
h.b [1] = 1;
gameStates.app.bLittleEndian = (h.s == 256);
}

// ----------------------------------------------------------------------------

void DoJoystickInit ()
{

if (!FindArg ("-nojoystick"))	{
	JoyInit ();
	if (FindArg ("-joyslow"))
		JoySetSlowReading (JOY_SLOW_READINGS);
	if (FindArg ("-joypolled"))
		JoySetSlowReading (JOY_POLLED_READINGS);
	if (FindArg ("-joybios"))
		JoySetSlowReading (JOY_BIOS_READINGS);
	}
}

// ----------------------------------------------------------------------------
//set this to force game to run in low res
void DoSelectPlayer (void)
{
LOCALPLAYER.callsign[0] = '\0';
if (!gameData.demo.bAuto) 	{
	KeyFlush ();
	//now, before we bring up the register player menu, we need to
	//do some stuff to make sure the palette is ok.  First, we need to
	//get our current palette into the 2d's array, so the remapping will
	//work.  Second, we need to remap the fonts.  Third, we need to fill
	//in part of the fade tables so the darkening of the menu edges works
	RemapFontsAndMenus (1);
	SelectPlayer ();		//get player's name
	}
}

// ----------------------------------------------------------------------------

#define MENU_HIRES_MODE SM (640, 480)

//	DESCENT II by Parallax Software
//		Descent Main

//extern ubyte gr_current_pal[];

#ifdef	EDITOR
int	Auto_exit = 0;
char	Auto_file[128] = "";
#endif

// ----------------------------------------------------------------------------

void PrintVersion (void)
{
	FILE	*f;
	char	fn [FILENAME_LEN];

sprintf (fn, "%s%sd2x-xl-version.txt", gameFolders.szDataDir, *gameFolders.szDataDir ? "/" : "");
if ((f = fopen (fn, "wa"))) {
	fprintf (f, "%s\n", VERSION);
	fclose (f);
	}
exit (0);
}

// ----------------------------------------------------------------------------

#ifdef WIN32
#	define	STD_GAMEDIR		""
#	define	D2X_APPNAME		"d2x-xl.exe"
#elif defined(__macosx__)
#	define	STD_GAMEDIR		"/Applications/Games/D2X-XL"
#	define	D2X_APPNAME		"d2x-xl"
#else
#	define	STD_GAMEDIR		"/usr/local/games/d2x-xl"
#	define	D2X_APPNAME		"d2x-xl"
#endif

#if defined(__macosx__)
#	define	DATADIR			"Data"
#	define	SHADERDIR		"Shaders"
#	define	MODELDIR			"Models"
#	define	ROBOTDIR			"Models/Robots"
#	define	SOUNDDIR1		"Sounds1"
#	define	SOUNDDIR2		"Sounds2"
#	define	CONFIGDIR		"Config"
#	define	PROFDIR			"Profiles"
#	define	SCRSHOTDIR		"Screenshots"
#	define	MOVIEDIR			"Movies"
#	define	SAVEDIR			"Savegames"
#	define	DEMODIR			"Demos"
#	define	TEXTUREDIR_D2	"Textures"
#	define	TEXTUREDIR_D1	"Textures/D1"
#	define	CACHEDIR			"Cache"
#else
#	define	DATADIR			"data"
#	define	SHADERDIR		"shaders"
#	define	MODELDIR			"models"
#	define	ROBOTDIR			"models/robots"
#	define	SOUNDDIR1		"sounds1"
#	define	SOUNDDIR2		"sounds2"
#	define	CONFIGDIR		"config"
#	define	PROFDIR			"profiles"
#	define	SCRSHOTDIR		"screenshots"
#	define	MOVIEDIR			"movies"
#	define	SAVEDIR			"savegames"
#	define	DEMODIR			"demos"
#	define	TEXTUREDIR_D2	"textures"
#	define	TEXTUREDIR_D1	"textures/d1"
#	define	CACHEDIR			"cache"
#endif

void GetAppFolders (void)
{
	int	i, j;
	char	szDataRootDir [FILENAME_LEN], szTemp [FILENAME_LEN];
	char	*psz;
#ifdef _WIN32
	char	c;
#endif
  
*gameFolders.szHomeDir =
*gameFolders.szGameDir =
*gameFolders.szDataDir =
*szDataRootDir = '\0';
if ((i = FindArg ("-userdir")) && GetAppFolder ("", gameFolders.szGameDir, pszArgList [i + 1], D2X_APPNAME))
	*gameFolders.szGameDir = '\0';
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, getenv ("DESCENT2"), D2X_APPNAME))
	*gameFolders.szGameDir = '\0';
#ifdef _WIN32
if (!*gameFolders.szGameDir) {
	psz = pszArgList [0];
	for (j = (int) strlen (psz); j; ) {
		c = psz [--j];
		if ((c == '\\') || (c == '/')) {
			memcpy (gameFolders.szGameDir, psz, ++j);
			gameFolders.szGameDir [j] = '\0';
			break;
			}
		}
	}
strcpy (szDataRootDir, gameFolders.szGameDir);
strcpy (gameFolders.szHomeDir, *gameFolders.szGameDir ? gameFolders.szGameDir : szDataRootDir);
#else // Linux, OS X
#	if defined (__unix__) || defined (__macosx__)
if (getenv ("HOME"))
	strcpy (gameFolders.szHomeDir, getenv ("HOME"));
#		if 0
if (!*gameFolders.szGameDir && *gameFolders.szHomeDir && GetAppFolder (gameFolders.szHomeDir, gameFolders.szGameDir, "d2x-xl", "d2x-xl"))
		*gameFolders.szGameDir = '\0';
#		endif
#	endif //__unix__
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, STD_GAMEDIR, ""))
		*gameFolders.szGameDir = '\0';
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, SHAREPATH, ""))
		*gameFolders.szGameDir = '\0';
#	ifdef __macosx__
GetOSXAppFolder (szDataRootDir, gameFolders.szGameDir);
#	else
strcpy (szDataRootDir, gameFolders.szGameDir);
#	endif //__macosx__
if (*gameFolders.szGameDir)
	chdir (gameFolders.szGameDir);
#endif //Linux, OS X
if ((i = FindArg ("-hogdir")) && !GetAppFolder ("", gameFolders.szDataDir, pszArgList [i + 1], "descent2.hog"))
	strcpy (szDataRootDir, pszArgList [i + 1]);
else {
	sprintf (gameFolders.szDataDir, "%s%s", gameFolders.szGameDir, DATADIR);
	if (GetAppFolder ("", gameFolders.szDataDir, gameFolders.szDataDir, "descent2.hog") &&
		 GetAppFolder ("", gameFolders.szDataDir, gameFolders.szDataDir, "d2demo.hog") &&
		 GetAppFolder (szDataRootDir, gameFolders.szDataDir, DATADIR, "descent2.hog") &&
		 GetAppFolder (szDataRootDir, gameFolders.szDataDir, DATADIR, "d2demo.hog"))
	Error (TXT_NO_HOG2);
	}
psz = strstr (gameFolders.szDataDir, DATADIR);
if (psz && !*(psz + 4)) {
	if (psz == gameFolders.szDataDir) 
		sprintf (gameFolders.szDataDir, "%s%s", gameFolders.szGameDir, DATADIR);
	else {
		*(psz - 1) = '\0';
		strcpy (szDataRootDir, gameFolders.szDataDir);
		*(psz - 1) = '/';
		}
	}
else 
	strcpy (szDataRootDir, gameFolders.szDataDir);
/*---*/PrintLog ("expected game app folder = '%s'\n", gameFolders.szGameDir);
/*---*/PrintLog ("expected game data folder = '%s'\n", gameFolders.szDataDir);
if (GetAppFolder (szDataRootDir, gameFolders.szModelDir [0], MODELDIR, "*.oof"))
	GetAppFolder (szDataRootDir, gameFolders.szModelDir [0], MODELDIR, "*.ase");
if (GetAppFolder (szDataRootDir, gameFolders.szModelDir [1], ROBOTDIR, "*.oof"))
	GetAppFolder (szDataRootDir, gameFolders.szModelDir [1], ROBOTDIR, "*.ase");
GetAppFolder (szDataRootDir, gameFolders.szSoundDir [0], SOUNDDIR1, "*.wav");
GetAppFolder (szDataRootDir, gameFolders.szSoundDir [1], SOUNDDIR2, "*.wav");
GetAppFolder (szDataRootDir, gameFolders.szShaderDir, SHADERDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szTextureDir [0], TEXTUREDIR_D2, "*.tga");
GetAppFolder (szDataRootDir, gameFolders.szTextureDir [1], TEXTUREDIR_D1, "*.tga");

GetAppFolder (szDataRootDir, gameFolders.szMovieDir, MOVIEDIR, "*.mvl");
#ifdef __unix__
if (*gameFolders.szHomeDir) {
		char	fn [FILENAME_LEN];

	sprintf (szDataRootDir, "%s/.d2x-xl", gameFolders.szHomeDir);
	CFMkDir (szDataRootDir);
	sprintf (fn, "%s/%s", szDataRootDir, PROFDIR);
	CFMkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, SAVEDIR);
	CFMkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, SCRSHOTDIR);
	CFMkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, DEMODIR);
	CFMkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, CONFIGDIR);
	CFMkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, CONFIGDIR);
	CFMkDir (fn);
	}
#endif
if (*gameFolders.szHomeDir) {
#ifdef __macosx__
	char *pszOSXCacheDir = GetMacOSXCacheFolder ();
	sprintf (gameFolders.szTextureCacheDir [0], "%s/%s",pszOSXCacheDir, TEXTUREDIR_D2);
	CFMkDir (gameFolders.szTextureCacheDir [0]);
	sprintf (gameFolders.szTextureCacheDir [1], "%s/%s", pszOSXCacheDir, TEXTUREDIR_D1);
	CFMkDir (gameFolders.szTextureCacheDir [1]);
	sprintf (gameFolders.szModelCacheDir, "%s/%s", pszOSXCacheDir, MODELDIR);
	CFMkDir (gameFolders.szModelCacheDir);
	sprintf (gameFolders.szCacheDir, "%s/%s", pszOSXCacheDir, CACHEDIR);
	CFMkDir (gameFolders.szCacheDir);
	sprintf (gameFolders.szCacheDir, "%s/%s/256", pszOSXCacheDir, CACHEDIR);
	CFMkDir (gameFolders.szCacheDir);
	sprintf (gameFolders.szCacheDir, "%s/%s/128", pszOSXCacheDir, CACHEDIR);
	CFMkDir (gameFolders.szCacheDir);
	sprintf (gameFolders.szCacheDir, "%s/%s/64", pszOSXCacheDir, CACHEDIR);
	CFMkDir (gameFolders.szCacheDir);
	sprintf (gameFolders.szCacheDir, "%s/%s", pszOSXCacheDir, CACHEDIR);
#else
#	if defined (__unix__)
	sprintf (szDataRootDir, "%s/.d2x-xl", gameFolders.szHomeDir);
#	else
	strcpy (szDataRootDir, gameFolders.szHomeDir);
	if (szDataRootDir [i = (int) strlen (szDataRootDir) - 1] == '\\')
		szDataRootDir [i] = '\0';
#	endif // __unix__
	CFMkDir (szDataRootDir);
	sprintf (gameFolders.szTextureCacheDir [0], "%s/%s", szDataRootDir, TEXTUREDIR_D2);
	CFMkDir (gameFolders.szTextureCacheDir [0]);
	sprintf (gameFolders.szTextureCacheDir [1], "%s/%s", szDataRootDir, TEXTUREDIR_D1);
	CFMkDir (gameFolders.szTextureCacheDir [1]);
	sprintf (gameFolders.szModelCacheDir, "%s/%s", szDataRootDir, MODELDIR);
	CFMkDir (gameFolders.szModelCacheDir);
	sprintf (gameFolders.szCacheDir, "%s/%s", szDataRootDir, CACHEDIR);
	CFMkDir (gameFolders.szCacheDir);
#endif // __macosx__
	}
GetAppFolder (szDataRootDir, gameFolders.szProfDir, PROFDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szSaveDir, SAVEDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szScrShotDir, SCRSHOTDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szDemoDir, DEMODIR, "");
if (GetAppFolder (szDataRootDir, gameFolders.szConfigDir, CONFIGDIR, "d2x.ini"))
	strcpy (gameFolders.szConfigDir, gameFolders.szGameDir);
#ifdef WIN32
sprintf (gameFolders.szMissionDir, "%s%s", gameFolders.szGameDir, BASE_MISSION_DIR);
#else
sprintf (gameFolders.szMissionDir, "%s/%s", gameFolders.szGameDir, BASE_MISSION_DIR);
#endif
//if (i = FindArg ("-hogdir"))
//	CFUseAltHogDir (pszArgList [i + 1]);
static char *szTexSubFolders [] = {"256", "128", "64"};

for (i = 0; i < 2; i++) {
	for (j = 0; j < 3; j++) {
		sprintf (szTemp, "%s/%s", gameFolders.szTextureCacheDir [i], szTexSubFolders [j]);
		CFMkDir (szTemp);
		}
	}
for (j = 0; j < 3; j++) {
	sprintf (szTemp, "%s/%s", gameFolders.szModelCacheDir, szTexSubFolders [j]);
	CFMkDir (szTemp);
	}
		
}

// ----------------------------------------------------------------------------

void EvalLegacyArgs (void)
{
#ifdef DEBUG	// it is strongly discouraged to use these switches!
	int bLegacy = FindArg ("-legacymode");

if (bLegacy || FindArg ("-legacyinput"))
	gameOptions [0].legacy.bInput = 1;
if (bLegacy || FindArg ("-legacyfuelcens"))
	gameOptions [0].legacy.bFuelCens = 1;
if (bLegacy || FindArg ("-legacymouse"))
	gameOptions [0].legacy.bMouse = 1;
if (bLegacy || FindArg ("-legacyhomers"))
	gameOptions [0].legacy.bHomers = 1;
if (bLegacy || FindArg ("-legacyrender"))
	gameOptions [0].legacy.bRender = 1;
if (bLegacy || FindArg ("-legacyswitches"))
	gameOptions [0].legacy.bSwitches = 1;
if (bLegacy || FindArg ("-legacywalls"))
	gameOptions [0].legacy.bWalls = 1;
#endif
}

// ----------------------------------------------------------------------------

void EvalAutoNetGameArgs (void)
{
	int	t, bHaveIp = 0;
	char	*p;

	static const char *pszTypes [] = {"anarchy", "coop", "ctf", "ctf+", "hoard", "entropy", NULL};
	static const char	*pszConnect [] = {"ipx", "udp", "", "multicast", NULL};

memset (&gameData.multiplayer.autoNG, 0, sizeof (gameData.multiplayer.autoNG));
if ((t = FindArg ("-ng_player")) && (p = pszArgList [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szPlayer, pszArgList [t+1], 8);
	gameData.multiplayer.autoNG.szPlayer [8] = '\0';
	}
if ((t = FindArg ("-ng_file")) && (p = pszArgList [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szFile, pszArgList [t+1], FILENAME_LEN - 1);
	gameData.multiplayer.autoNG.szFile [FILENAME_LEN - 1] = '\0';
	}
if ((t = FindArg ("-ng_mission")) && (p = pszArgList [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szMission, pszArgList [t+1], 12);
	gameData.multiplayer.autoNG.szMission [12] = '\0';
	}
if ((t = FindArg ("-ngLevel")))
	gameData.multiplayer.autoNG.nLevel = NumArg (t, 1);
else
	gameData.multiplayer.autoNG.nLevel = 1;
if ((t = FindArg ("-ng_name")) && (p = pszArgList [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szName, pszArgList [t+1], 80);
	gameData.multiplayer.autoNG.szName [80] = '\0';
	}
if ((t = FindArg ("-ng_ipaddr")) && (p = pszArgList [t+1]))
	bHaveIp = stoip (pszArgList [t+1], gameData.multiplayer.autoNG.ipAddr);
if ((t = FindArg ("-ng_connect")) && (p = pszArgList [t+1])) {
	strlwr (p);
	for (t = 0; pszTypes [t]; t++)
		if (*pszConnect [t] && !strcmp (p, pszConnect [t])) {
			gameData.multiplayer.autoNG.uConnect = t;
			break;
			}
	}
if ((t = FindArg ("-ng_join")) && (p = pszArgList [t+1])) {
	strlwr (p);
	gameData.multiplayer.autoNG.bHost = !strcmp (p, "host");
	}
if ((t = FindArg ("-ngType")) && (p = pszArgList [t+1])) {
	strlwr (p);
	for (t = 0; pszTypes [t]; t++)
		if (!strcmp (p, pszTypes [t])) {
			gameData.multiplayer.autoNG.uType = t;
			break;
			}
	}
if ((t = FindArg ("-ng_team")))
	gameData.multiplayer.autoNG.bTeam = NumArg (t, 1);
if (gameData.multiplayer.autoNG.bHost)
	gameData.multiplayer.autoNG.bValid = 
		*gameData.multiplayer.autoNG.szPlayer &&
		*gameData.multiplayer.autoNG.szName &&
		*gameData.multiplayer.autoNG.szFile &&
		*gameData.multiplayer.autoNG.szMission;
else
	gameData.multiplayer.autoNG.bValid = 
		*gameData.multiplayer.autoNG.szPlayer &&
		bHaveIp;
if (gameData.multiplayer.autoNG.bValid)
	gameOptions [0].movies.nLevel = 0;
}

// ----------------------------------------------------------------------------

void EvalMultiplayerArgs (void)
{
	int t;

if ((t = FindArg ("-friendlyfire")))
	extraGameInfo [0].bFriendlyFire = NumArg (t, 1);
if ((t = FindArg ("-fixedrespawns")))
	extraGameInfo [0].bFixedRespawns = NumArg (t, 0);
if ((t = FindArg ("-immortalpowerups")))
	extraGameInfo [0].bImmortalPowerups = NumArg (t, 0);
if ((t = FindArg ("-spawndelay"))) {
	extraGameInfo [0].nSpawnDelay = NumArg (t, 0);
	if (extraGameInfo [0].nSpawnDelay < 0)
		extraGameInfo [0].nSpawnDelay = 0;
	else if (extraGameInfo [0].nSpawnDelay > 60)
		extraGameInfo [0].nSpawnDelay = 60;
	extraGameInfo [0].nSpawnDelay *= 1000;
	}
if ((t = FindArg ("-pps"))) {
	mpParams.nPPS = atoi(pszArgList [t+1]);
	if (mpParams.nPPS < 1)
		mpParams.nPPS = 1;
	else if (mpParams.nPPS > 20)
		mpParams.nPPS = 20;
	}
if (FindArg ("-shortpackets"))
	mpParams.bShortPackets = 1;
if (FindArg ("-norankings"))
	gameOptions [0].multi.bNoRankings = 1;
if ((t = FindArg ("-timeout")))
	gameOptions [0].multi.bTimeoutPlayers = NumArg (t, 1);
if ((t = FindArg ("-noredundancy")))
	gameOptions [0].multi.bNoRedundancy = NumArg (t, 1);
if ((t = FindArg ("-check_ports")))
	gameStates.multi.bCheckPorts = NumArg (t, 1);
EvalAutoNetGameArgs ();
}

// ----------------------------------------------------------------------------

void EvalMovieArgs (void)
{
	int	t;

if ((t = FindArg ("-nomovies"))) {
	gameOptions [0].movies.nLevel = 2 - NumArg (t, 2);
	if (gameOptions [0].movies.nLevel < 0)
		gameOptions [0].movies.nLevel = 0;
	else if (gameOptions [0].movies.nLevel > 2)
		gameOptions [0].movies.nLevel = 2;
	}
if (FindArg ("-subtitles"))
	gameOptions [0].movies.bSubTitles = 1;
if ((t = FindArg ("-movie_quality")))
	gameOptions [0].movies.nQuality = NumArg (t, 0);
if (FindArg ("-lowresmovies"))
	gameOptions [0].movies.bHires = 0;
if (gameData.multiplayer.autoNG.bValid)
	gameOptions [0].movies.nLevel = 0;
if (FindArg ("-mac_movies"))
	gameStates.movies.bMac = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalSoundArgs (void)
{
	int	t;

if (FindArg ("-sound44k"))
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_44K;
else if (FindArg ("-sound22k"))
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_22K;
else if (FindArg ("-sound11k"))
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_11K;
else
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_22K;
#if USE_SDL_MIXER
if ((t = FindArg ("-sdl_mixer")))
	gameOptions [0].sound.bUseSDLMixer = NumArg (t, 1);
#endif
if ((t = FindArg ("-use_d1sounds")))
	gameOptions [0].sound.bUseD1Sounds = NumArg (t, 1);
if ((t = FindArg ("-noredbook")))
	gameOptions [0].sound.bUseRedbook = 0;
#if USE_SDL_MIXER
if (gameOptions [0].sound.bUseSDLMixer) {
	if ((t = FindArg ("-hires_sound")))
		gameOptions [0].sound.bHires = NumArg (t, 1);
	}
#endif
}

// ----------------------------------------------------------------------------

void EvalMusicArgs (void)
{
	int	t;
	char	*p;

if ((t = FindArg ("-playlist")) && (p = pszArgList [t+1]))
	LoadPlayList (p);
if ((t = FindArg ("-introsong")) && (p = pszArgList [t+1]))
	strncpy (gameData.songs.user.szIntroSong, p, FILENAME_LEN);
if ((t = FindArg ("-briefingsong")) && (p = pszArgList [t+1]))
	strncpy (gameData.songs.user.szBriefingSong, p, FILENAME_LEN);
if ((t = FindArg ("-creditssong")) && (p = pszArgList [t+1]))
	strncpy (gameData.songs.user.szCreditsSong, p, FILENAME_LEN);
if ((t = FindArg ("-menusong")) && (p = pszArgList [t+1]))
	strncpy (gameData.songs.user.szMenuSong, p, FILENAME_LEN);
}

// ----------------------------------------------------------------------------

void EvalMenuArgs (void)
{
	int	t;

if (!gameStates.app.bNostalgia)
	if ((t = FindArg ("-menustyle")))
		gameOptions [0].menus.nStyle = NumArg (t, 1);
if ((t = FindArg ("-fastmenus")))
	gameOptions [0].menus.bFastMenus = NumArg (t, 1);
if ((t = FindArg ("-altbg_alpha")) && *pszArgList [t+1]) {
	gameOptions [0].menus.altBg.alpha = atof (pszArgList [t+1]);
	if (gameOptions [0].menus.altBg.alpha < 0)
		gameOptions [0].menus.altBg.alpha = -1.0;
	else if ((gameOptions [0].menus.altBg.alpha == 0) || (gameOptions [0].menus.altBg.alpha > 1.0))
		gameOptions [0].menus.altBg.alpha = 1.0;
	}
if ((t = FindArg ("-altbg_brightness")) && *pszArgList [t+1]) {
	gameOptions [0].menus.altBg.brightness = atof (pszArgList [t+1]);
	if ((gameOptions [0].menus.altBg.brightness <= 0) || (gameOptions [0].menus.altBg.brightness > 1.0))
		gameOptions [0].menus.altBg.brightness = 1.0;
	}
if ((t = FindArg ("-altbg_grayscale")))
	gameOptions [0].menus.altBg.grayscale = NumArg (t, 1);
if ((t = FindArg ("-altbg_name")) && *pszArgList [t+1])
	strncpy (gameOptions [0].menus.altBg.szName, pszArgList [t+1], sizeof (gameOptions [0].menus.altBg.szName));
if ((t = FindArg ("-menu_hotkeys")))
	gameOptions [0].menus.nHotKeys = NumArg (t, 1);
if ((t = FindArg ("-use_swapfile")))
	gameStates.app.bUseSwapFile = NumArg (t, 1);
else if (gameStates.app.bEnglish)
	gameOptions [0].menus.nHotKeys = 1;
}

// ----------------------------------------------------------------------------

void EvalGameplayArgs (void)
{
	int	t;

if ((t = FindArg ("-noscreens")))
	gameOpts->gameplay.bSkipBriefingScreens = NumArg (t, 1);
if ((t = FindArg ("-secretsave")))
	gameOptions [0].gameplay.bSecretSave = NumArg (t, 1);
if ((t = FindArg ("-nobotai")))
	gameStates.gameplay.bNoBotAI = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalInputArgs (void)
{
	int	t, i;

if ((t = FindArg ("-mouselook")))
	extraGameInfo [0].bMouseLook = NumArg (t, 1);
if ((t = FindArg ("-limitturnrate")))
	gameOptions [0].input.bLimitTurnRate = NumArg (t, 1);
#if DBG
if ((t = FindArg ("-minturnrate")))
	gameOptions [0].input.nMinTurnRate = NumArg (t, 20);
#endif
SetMaxPitch (gameOptions [0].input.nMinTurnRate);
if ((t = FindArg ("-joydeadzone")))
	for (i = 0; i < 4; i++)
		JoySetDeadzone (NumArg (t, 0), i);
if ((t = FindArg ("-grabmouse")))
	gameStates.input.bGrabMouse = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalOglArgs (void)
{
	int	t;

if ((t = FindArg ("-gl_alttexmerge")))
	gameOpts->ogl.bGlTexMerge = NumArg (t, 1);
if ((t = FindArg("-gl_16bittextures"))) {
	gameStates.ogl.bpp = 16;
	gameStates.ogl.nRGBAFormat = GL_RGBA4;
	gameStates.ogl.nRGBFormat = GL_RGB5;
	}
if ((t=FindArg("-gl_intensity4_ok")))
	gameStates.ogl.bIntensity4 = NumArg (t, 1);
if ((t = FindArg("-gl_luminance4_alpha4_ok")))
	gameStates.ogl.bLuminance4Alpha4 = NumArg (t, 1);
if ((t = FindArg("-gl_readpixels_ok")))
	gameStates.ogl.bReadPixels = NumArg (t, 1);
if ((t = FindArg("-gl_gettexlevelparam_ok")))
	gameStates.ogl.bGetTexLevelParam = NumArg (t, 1);
#ifdef GL_ARB_multitexture
if (t = FindArg ("-gl_arb_multitexture_ok")))
	gameStates.ogl.bArbMultiTexture= NumArg (t, 1);
#endif
#ifdef GL_SGIS_multitexture
if (t = FindArg ("-gl_sgis_multitexture_ok")))
	gameStates.ogl.bSgisMultiTexture = NumArg (t, 1);
#endif
}

// ----------------------------------------------------------------------------

void EvalDemoArgs (void)
{
	int	t;

if ((t = FindArg ("-revert_demos")))
	gameOpts->demo.bRevertFormat = NumArg (t, 1);
if ((t = FindArg ("-auto_demos")))
	gameStates.app.bAutoDemos = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalRenderArgs (void)
{
	int	t;

if ((t = FindArg ("-render_quality")) && *pszArgList [t+1]) {
	gameOpts->render.nQuality = NumArg (t, 3);
	if (gameOpts->render.nQuality < 0)
		gameOpts->render.nQuality = 0;
	else if (gameOpts->render.nQuality > 3)
		gameOpts->render.nQuality = 3;
	}
#if FBO_DRAW_BUFFER
if ((t = FindArg ("-render_indirect")))
	gameStates.render.bRenderIndirect = NumArg (t, 1);
#endif
if ((t = FindArg ("-use_shaders")))
	gameOptions [0].render.bUseShaders = NumArg (t, 1);
if ((t = FindArg ("-shadows")))
	extraGameInfo [0].bShadows = NumArg (t, 0);
if ((t = FindArg ("-hires_textures")))
	gameOptions [0].render.textures.bUseHires = NumArg (t, 1);
if ((t = FindArg ("-hires_models")))
	gameOptions [0].render.bHiresModels = NumArg (t, 1);
if ((t = FindArg ("-render_all_segs")))
	gameOptions [0].render.bAllSegs = NumArg (t, 1);
if ((t = FindArg ("-enable_lightmaps")))
	gameStates.render.color.bLightmapsOk = NumArg (t, 1);
if ((t = FindArg ("-blend_background")))
	gameStates.render.bBlendBackground = NumArg (t, 1);
if ((t = FindArg ("-tmap")))
	select_tmap (pszArgList [t+1]);
else
	select_tmap (NULL);
if ((t = FindArg ("-maxfps"))) {
	t = NumArg (t, 250);
	if (t <= 0)
		t = 1;
	else if (t > 250)
		t = 250;
	gameOpts->render.nMaxFPS = t;
	}
if ((t = FindArg ("-usePerPixelLighting")))
	gameStates.render.bUsePerPixelLighting = NumArg (t, 1);
#if RENDER2TEXTURE
if ((t = FindArg ("-render2texture")))
	gameStates.ogl.bUseRender2Texture = NumArg (t, 1);
#endif
#if OGL_POINT_SPRITES
if ((t = FindArg ("-point_sprites")))
	gameStates.render.bPointSprites = NumArg (t, 1);
#endif
if ((t = FindArg ("-nofade")))
	gameStates.render.bDisableFades = NumArg (t, 1);
if ((t = FindArg ("-mathformat")))
	gameOpts->render.nDefMathFormat = NumArg (t, 2);
#if defined (_WIN32) || defined (__unix__)
if ((t = FindArg ("-enable_sse")))
	gameStates.render.bEnableSSE = NumArg (t, 1);
#endif
#if DBG
if ((t = FindArg ("-gl_transform")))
	gameStates.ogl.bUseTransform = NumArg (t, 1);
#endif
if ((t = FindArg ("-cache_textures")))
	gameStates.app.bCacheTextures = NumArg (t, 1);
if ((t = FindArg ("-cache_models")))
	gameStates.app.bCacheModelData = NumArg (t, 1);
if ((t = FindArg ("-model_quality")) && *pszArgList [t+1])
	gameStates.render.nModelQuality = NumArg (t, 3);
#if 0
if ((t = FindArg ("-gl_texcompress")))
	gameStates.ogl.bTextureCompression = NumArg (t, 1);
#endif
if ((t = FindArg ("-cache_meshes")))
	gameStates.app.bCacheMeshes = NumArg (t, 1);
if ((t = FindArg ("-cache_lightmaps")))
	gameStates.app.bCacheLightmaps = NumArg (t, 1);
#if 0
if ((t = FindArg ("-renderpath")))
	gameOptions [0].render.nPath = NumArg (t, 1);
#endif
if ((t = FindArg ("-split_polys")))
	gameStates.render.bSplitPolys = NumArg (t, 1);
if ((t = FindArg ("-cluster_lights")))
	gameStates.render.bClusterLights = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalAppArgs (void)
{
	int	t;

if ((t = FindArg ("-max_segments")) && *pszArgList [t+1]) {
	t = NumArg (t, MAX_SEGMENTS_D2X);
	if (t < MAX_SEGMENTS_D2)
		t = MAX_SEGMENTS_D2;
	else if (t > MAX_SEGMENTS_D2X)
		t = MAX_SEGMENTS_D2X;
	gameData.segs.nMaxSegments = t;
	}
#if 0
if ((t = FindArg ("-gpgpu_lights")))
	gameStates.ogl.bVertexLighting = NumArg (t, 1);
#endif
if ((t = FindArg ("-cache_lights")))
	gameStates.app.bCacheLights = NumArg (t, 1);
if ((t = FindArg ("-enable_shadows")))
	gameStates.app.bEnableShadows = NumArg (t, 1);
if ((t = FindArg ("-enable_freecam")))
	gameStates.app.bEnableFreeCam = NumArg (t, 1);
if ((t = FindArg ("-pured2")))
	gameStates.app.bNostalgia = 3;
else if ((t = FindArg ("-nostalgia"))) {
	if (pszArgList [t+1]) {
		gameStates.app.bNostalgia = NumArg (t, 0);
		if (gameStates.app.bNostalgia < 0)
			gameStates.app.bNostalgia = 0;
		else if (gameStates.app.bNostalgia > 3)
			gameStates.app.bNostalgia = 3;
		}
	else
		gameStates.app.bNostalgia = 1;
	}
gameStates.app.iNostalgia = (gameStates.app.bNostalgia > 0);
gameOpts = gameOptions + gameStates.app.iNostalgia;

#if 1 //MULTI_THREADED
if ((t = FindArg ("-multithreaded")))
	gameStates.app.bMultiThreaded = NumArg (t, 1);
#endif
if ((t = FindArg ("-nosound")))
	gameStates.app.bUseSound = (NumArg (t, 1) == 0);
if ((t = FindArg ("-progress_bars")))
	gameStates.app.bProgressBars = NumArg (t, 1);
if ((t = FindArg ("-fix_models")))
	gameStates.app.bFixModels = NumArg (t, 1);
if ((t = FindArg ("-alt_models")))
	gameStates.app.bAltModels = NumArg (t, 1);
if ((t = FindArg ("-lores_shadows")))
	gameStates.render.bLoResShadows = NumArg (t, 1);
if ((t = FindArg ("-printVersion")))
	PrintVersion ();
if ((t = FindArg ("-altLanguage")))
	gameStates.app.bEnglish = (NumArg (t, 1) == 0);

if ((t = FindArg ("-auto_hogfile"))) {
	strcpy (szAutoHogFile, "missions/");
	strcat (szAutoHogFile, pszArgList [t+1]);
	if (*szAutoHogFile && !strchr (szAutoHogFile, '.'))
		strcat (szAutoHogFile, ".hog");
	}
if ((t = FindArg ("-auto_mission"))) {
	char	c = *pszArgList [++t];
	int		bDelim = ((c == '\'') || (c == '"'));

	strcpy (szAutoMission, &pszArgList [t][bDelim]);
	if (bDelim)
		szAutoMission [strlen (szAutoMission) - 1] = '\0';
	if (*szAutoMission && !strchr (szAutoMission, '.'))
		strcat (szAutoMission, ".rl2");
	}
if (FindArg ("-debug"))
	con_threshold.value = 2.0;
else if (FindArg ("-verbose"))
	con_threshold.value = 1.0;
else
	con_threshold.value = -1.0;
if ((t = FindArg ("-autodemo"))) {
	gameData.demo.bAuto = 1;
	strncpy (gameData.demo.fnAuto, *pszArgList [t+1] ? pszArgList [t+1] : "descent.dem", sizeof (gameData.demo.fnAuto));
	}
else
	gameData.demo.bAuto = 0;
gameStates.app.bMacData = FindArg ("-macdata");
if (gameStates.app.bNostalgia)
	gameData.segs.nMaxSegments = MAX_SEGMENTS_D2;
}

// ----------------------------------------------------------------------------

void EvalArgs (void)
{
EvalAppArgs ();
EvalGameplayArgs ();
EvalLegacyArgs ();
EvalInputArgs ();
EvalMultiplayerArgs ();
EvalMenuArgs ();
EvalMovieArgs ();
EvalOglArgs ();
EvalRenderArgs ();
EvalSoundArgs ();
EvalMusicArgs ();
EvalDemoArgs ();
}

//------------------------------------------------------------------------------

void InitRenderOptions (int i)
{
if (i) {
	extraGameInfo [0].bPowerupsOnRadar = 0;
	extraGameInfo [0].bRobotsOnRadar = 0;
	extraGameInfo [0].bUseCameras = 0;
	if (gameStates.app.bNostalgia > 2)
		extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].bWiggle = 1;
	extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].bDamageExplosions = 0;
	extraGameInfo [0].bThrusterFlames = 0;
	extraGameInfo [0].bShadows = 0;
	extraGameInfo [0].bShowWeapons = 1;
	gameOptions [0].render.nPath = 0;
	gameOptions [1].render.shadows.bPlayers = 0;
	gameOptions [1].render.shadows.bRobots = 0;
	gameOptions [1].render.shadows.bMissiles = 0;
	gameOptions [1].render.shadows.bPowerups = 0;
	gameOptions [1].render.shadows.bReactors = 0;
	gameOptions [1].render.shadows.bFast = 1;
	gameOptions [1].render.shadows.nClip = 1;
	gameOptions [1].render.shadows.nReach = 1;
	gameOptions [1].render.ship.nWingtip = 1;
	gameOptions [1].render.ship.bBullets = 1;
	gameOptions [1].render.nMaxFPS = 150;
	gameOptions [1].render.bDepthSort = 0;
	gameOptions [1].render.effects.bTransparent = 0;
	gameOptions [1].render.bAllSegs = 0;
	gameOptions [1].render.debug.bDynamicLight = 1;
	gameOptions [1].render.textures.bUseHires = 0;
	if (gameStates.app.bNostalgia > 2)
		gameOptions [1].render.nQuality = 0;
	gameOptions [1].render.coronas.bUse = 0;
	gameOptions [1].render.coronas.nStyle = 1;
	gameOptions [1].render.coronas.bShots = 0;
	gameOptions [1].render.coronas.bPowerups = 0;
	gameOptions [1].render.coronas.bWeapons = 0;
	gameOptions [1].render.coronas.bAdditive = 0;
	gameOptions [1].render.coronas.bAdditiveObjs = 0;
	gameOptions [1].render.effects.bRobotShields = 0;
	gameOptions [1].render.effects.bOnlyShieldHits = 0;
	gameOptions [1].render.bBrightObjects = 0;
	gameOptions [1].render.coronas.nIntensity = 2;
	gameOptions [1].render.coronas.nObjIntensity = 1;
	gameOptions [1].render.effects.bExplBlasts = 1;
	gameOptions [1].render.effects.nShrapnels = 1;
	gameOptions [1].render.smoke.bAuxViews = 0;
	gameOptions [1].render.lightnings.bAuxViews = 0;
	gameOptions [1].render.debug.bWireFrame = 0;
	gameOptions [1].render.debug.bTextures = 1;
	gameOptions [1].render.debug.bObjects = 1;
	gameOptions [1].render.debug.bWalls = 1;
	gameOptions [1].render.bUseShaders = 1;
	gameOptions [1].render.bHiresModels = 0;
	gameOptions [1].render.bUseLightmaps = 0;
	gameOptions [1].render.effects.bAutoTransparency = 0;
	gameOptions [1].render.nMeshQuality = 0;
	gameOptions [1].render.nMathFormat = 0;
	gameOptions [1].render.nDefMathFormat = 0;
	gameOptions [1].render.nDebrisLife = 0;
	gameOptions [1].render.shadows.nLights = 0;
	gameOptions [1].render.cameras.bFitToWall = 0;
	gameOptions [1].render.cameras.nFPS = 0;
	gameOptions [1].render.cameras.nSpeed = 0;
	gameOptions [1].render.cockpit.bHUD = 1;
	gameOptions [1].render.cockpit.bHUDMsgs = 1;
	gameOptions [1].render.cockpit.bSplitHUDMsgs = 0;
	gameOptions [1].render.cockpit.bMouseIndicator = 1;
	gameOptions [1].render.cockpit.bTextGauges = 1;
	gameOptions [1].render.cockpit.bObjectTally = 0;
	gameOptions [1].render.cockpit.bScaleGauges = 1;
	gameOptions [1].render.cockpit.bFlashGauges = 1;
	gameOptions [1].render.cockpit.bReticle = 1;
	gameOptions [1].render.cockpit.bRotateMslLockInd = 0;
	gameOptions [1].render.cockpit.nWindowSize = 0;
	gameOptions [1].render.cockpit.nWindowZoom = 0;
	gameOptions [1].render.cockpit.nWindowPos = 1;
	gameOptions [1].render.color.bAmbientLight = 0;
	gameOptions [1].render.color.bCap = 0;
	gameOptions [1].render.color.nSaturation = 0;
	gameOptions [1].render.color.bGunLight = 0;
	gameOptions [1].render.color.bMix = 1;
	gameOptions [1].render.color.bUseLightmaps = 0;
	gameOptions [1].render.color.bWalls = 0;
	gameOptions [1].render.color.nLightmapRange = 5;
	gameOptions [1].render.weaponIcons.bSmall = 0;
	gameOptions [1].render.weaponIcons.bShowAmmo = 0;
	gameOptions [1].render.weaponIcons.bEquipment = 0;
	gameOptions [1].render.weaponIcons.nSort = 0;
	gameOptions [1].render.textures.bUseHires = 0;
	gameOptions [1].render.textures.nQuality = 0;
	gameOptions [1].render.cockpit.bMissileView = 1;
	gameOptions [1].render.cockpit.bGuidedInMainView = 1;
	gameOptions [1].render.smoke.nDens [0] =
	gameOptions [1].render.smoke.nDens [1] =
	gameOptions [1].render.smoke.nDens [2] =
	gameOptions [1].render.smoke.nDens [3] =
	gameOptions [1].render.smoke.nDens [4] = 0;
	gameOptions [1].render.smoke.nSize [0] =
	gameOptions [1].render.smoke.nSize [1] =
	gameOptions [1].render.smoke.nSize [2] =
	gameOptions [1].render.smoke.nSize [3] =
	gameOptions [1].render.smoke.nSize [4] = 0;
	gameOptions [1].render.smoke.nLife [0] =
	gameOptions [1].render.smoke.nLife [1] =
	gameOptions [1].render.smoke.nLife [2] = 0;
	gameOptions [1].render.smoke.nLife [3] = 1;
	gameOptions [1].render.smoke.nLife [4] = 0;
	gameOptions [1].render.smoke.nAlpha [0] =
	gameOptions [1].render.smoke.nAlpha [1] =
	gameOptions [1].render.smoke.nAlpha [2] =
	gameOptions [1].render.smoke.nAlpha [3] =
	gameOptions [1].render.smoke.nAlpha [4] = 0;
	gameOptions [1].render.smoke.bPlayers = 0;
	gameOptions [1].render.smoke.bRobots = 0;
	gameOptions [1].render.smoke.bMissiles = 0;
	gameOptions [1].render.smoke.bDebris = 0;
	gameOptions [1].render.smoke.bStatic = 0;
	gameOptions [1].render.smoke.bCollisions = 0;
	gameOptions [1].render.smoke.bDisperse = 0;
	gameOptions [1].render.smoke.bRotate = 0;
	gameOptions [1].render.smoke.bSort = 0;
	gameOptions [1].render.smoke.bDecreaseLag = 0;
	gameOptions [1].render.lightnings.bOmega = 0;
	gameOptions [1].render.lightnings.bDamage = 0;
	gameOptions [1].render.lightnings.bExplosions = 0;
	gameOptions [1].render.lightnings.bPlayers = 0;
	gameOptions [1].render.lightnings.bRobots = 0;
	gameOptions [1].render.lightnings.bStatic = 0;
	gameOptions [1].render.lightnings.bPlasma = 0;
	gameOptions [1].render.lightnings.nQuality = 0;
	gameOptions [1].render.lightnings.nStyle = 0;
	gameOptions [1].render.powerups.b3D = 0;
	gameOptions [1].render.powerups.nSpin = 0;
	gameOptions [1].render.automap.bTextured = 0;
	gameOptions [1].render.automap.bSmoke = 0;
	gameOptions [1].render.automap.bLightnings = 0;
	gameOptions [1].render.automap.bSkybox = 0;
	gameOptions [1].render.automap.bBright = 1;
	gameOptions [1].render.automap.bCoronas = 0;
	gameOptions [1].render.automap.nColor = 0;
	gameOptions [1].render.automap.nRange = 2;
	}
else {
	extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].bShadows = 0;
	gameOptions [0].render.nPath = 1;
	gameOptions [0].render.shadows.bPlayers = 1;
	gameOptions [0].render.shadows.bRobots = 0;
	gameOptions [0].render.shadows.bMissiles = 0;
	gameOptions [0].render.shadows.bPowerups = 0;
	gameOptions [0].render.shadows.bReactors = 0;
	gameOptions [0].render.shadows.bFast = 1;
	gameOptions [0].render.shadows.nClip = 1;
	gameOptions [0].render.shadows.nReach = 1;
	gameOptions [0].render.nMaxFPS = 150;
	gameOptions [1].render.bDepthSort = 1;
	gameOptions [0].render.effects.bTransparent = 1;
	gameOptions [0].render.bAllSegs = 1;
	gameOptions [0].render.debug.bDynamicLight = 1;
	gameOptions [0].render.nQuality = 3;
	gameOptions [0].render.debug.bWireFrame = 0;
	gameOptions [0].render.debug.bTextures = 1;
	gameOptions [0].render.debug.bObjects = 1;
	gameOptions [0].render.debug.bWalls = 1;
	gameOptions [0].render.bUseShaders = 1;
	gameOptions [0].render.bHiresModels = 1;
	gameOptions [0].render.bUseLightmaps = 0;
	gameOptions [0].render.effects.bAutoTransparency = 1;
	gameOptions [0].render.nMathFormat = 0;
	gameOptions [0].render.nDefMathFormat = 0;
	gameOptions [0].render.nDebrisLife = 0;
	gameOptions [0].render.smoke.bAuxViews = 0;
	gameOptions [0].render.lightnings.bAuxViews = 0;
	gameOptions [0].render.coronas.bUse = 0;
	gameOptions [0].render.coronas.nStyle = 1;
	gameOptions [0].render.coronas.bShots = 0;
	gameOptions [0].render.coronas.bPowerups = 0;
	gameOptions [0].render.coronas.bWeapons = 0;
	gameOptions [0].render.coronas.bAdditive = 0;
	gameOptions [0].render.coronas.bAdditiveObjs = 0;
	gameOptions [0].render.coronas.nIntensity = 2;
	gameOptions [0].render.coronas.nObjIntensity = 1;
	gameOptions [0].render.effects.bRobotShields = 0;
	gameOptions [0].render.effects.bOnlyShieldHits = 0;
	gameOptions [0].render.bBrightObjects = 0;
	gameOptions [0].render.effects.bExplBlasts = 1;
	gameOptions [0].render.effects.nShrapnels = 1;
#if DBG
	gameOptions [0].render.shadows.nLights = 1;
#else
	gameOptions [0].render.shadows.nLights = 3;
#endif
	gameOptions [0].render.cameras.bFitToWall = 0;
	gameOptions [0].render.cameras.nFPS = 0;
	gameOptions [0].render.cameras.nSpeed = 5000;
	gameOptions [0].render.cockpit.bHUD = 1;
	gameOptions [0].render.cockpit.bHUDMsgs = 1;
	gameOptions [0].render.cockpit.bSplitHUDMsgs = 0;
	gameOptions [0].render.cockpit.bMouseIndicator = 0;
	gameOptions [0].render.cockpit.bTextGauges = 1;
	gameOptions [0].render.cockpit.bObjectTally = 1;
	gameOptions [0].render.cockpit.bScaleGauges = 1;
	gameOptions [0].render.cockpit.bFlashGauges = 1;
	gameOptions [0].render.cockpit.bRotateMslLockInd = 0;
	gameOptions [0].render.cockpit.bReticle = 1;
	gameOptions [0].render.cockpit.nWindowSize = 0;
	gameOptions [0].render.cockpit.nWindowZoom = 0;
	gameOptions [0].render.cockpit.nWindowPos = 1;
	gameOptions [0].render.color.bAmbientLight = 0;
	gameOptions [0].render.color.bGunLight = 0;
	gameOptions [0].render.color.bMix = 1;
	gameOptions [0].render.color.nSaturation = 0;
	gameOptions [0].render.color.bUseLightmaps = 0;
	gameOptions [0].render.color.bWalls = 0;
	gameOptions [0].render.color.nLightmapRange = 5;
	gameOptions [0].render.weaponIcons.bSmall = 0;
	gameOptions [0].render.weaponIcons.bShowAmmo = 1;
	gameOptions [0].render.weaponIcons.bEquipment = 1;
	gameOptions [0].render.weaponIcons.nSort = 1;
	gameOptions [0].render.weaponIcons.alpha = 4;
	gameOptions [0].render.textures.bUseHires = 0;
	gameOptions [0].render.textures.nQuality = 3;
	gameOptions [0].render.nMeshQuality = 0;
	gameOptions [0].render.cockpit.bMissileView = 1;
	gameOptions [0].render.cockpit.bGuidedInMainView = 1;
	gameOptions [0].render.smoke.nDens [0] =
	gameOptions [0].render.smoke.nDens [0] =
	gameOptions [0].render.smoke.nDens [2] =
	gameOptions [0].render.smoke.nDens [3] =
	gameOptions [0].render.smoke.nDens [4] = 2;
	gameOptions [0].render.smoke.nSize [0] =
	gameOptions [0].render.smoke.nSize [0] =
	gameOptions [0].render.smoke.nSize [2] =
	gameOptions [0].render.smoke.nSize [3] =
	gameOptions [0].render.smoke.nSize [4] = 1;
	gameOptions [0].render.smoke.nLife [0] =
	gameOptions [0].render.smoke.nLife [0] =
	gameOptions [0].render.smoke.nLife [2] = 0;
	gameOptions [0].render.smoke.nLife [3] = 1;
	gameOptions [0].render.smoke.nLife [4] = 0;
	gameOptions [0].render.smoke.nAlpha [0] =
	gameOptions [0].render.smoke.nAlpha [0] =
	gameOptions [0].render.smoke.nAlpha [2] =
	gameOptions [0].render.smoke.nAlpha [3] =
	gameOptions [0].render.smoke.nAlpha [4] = 0;
	gameOptions [0].render.smoke.bPlayers = 1;
	gameOptions [0].render.smoke.bRobots = 1;
	gameOptions [0].render.smoke.bMissiles = 1;
	gameOptions [0].render.smoke.bDebris = 1;
	gameOptions [0].render.smoke.bStatic = 1;
	gameOptions [0].render.smoke.bCollisions = 0;
	gameOptions [0].render.smoke.bDisperse = 1;
	gameOptions [0].render.smoke.bRotate = 1;
	gameOptions [0].render.smoke.bSort = 1;
	gameOptions [0].render.smoke.bDecreaseLag = 1;
	gameOptions [0].render.lightnings.bOmega = 1;
	gameOptions [0].render.lightnings.bDamage = 1;
	gameOptions [0].render.lightnings.bExplosions = 1;
	gameOptions [0].render.lightnings.bPlayers = 1;
	gameOptions [0].render.lightnings.bRobots = 1;
	gameOptions [0].render.lightnings.bStatic = 1;
	gameOptions [0].render.lightnings.bPlasma = 1;
	gameOptions [0].render.lightnings.nQuality = 0;
	gameOptions [0].render.lightnings.nStyle = 1;
	gameOptions [0].render.powerups.b3D = 0;
	gameOptions [0].render.powerups.nSpin = 0;
	gameOptions [0].render.automap.bTextured = 0;
	gameOptions [0].render.automap.bSmoke = 0;
	gameOptions [0].render.automap.bLightnings = 0;
	gameOptions [0].render.automap.bSkybox = 0;
	gameOptions [0].render.automap.bBright = 1;
	gameOptions [0].render.automap.bCoronas = 0;
	gameOptions [0].render.automap.nColor = 0;
	gameOptions [0].render.automap.nRange = 2;
	}
}

//------------------------------------------------------------------------------

void InitGameplayOptions (int i)
{
if (i) {
	extraGameInfo [0].nSpawnDelay = 0;
	extraGameInfo [0].nFusionRamp = 2;
	extraGameInfo [0].bFixedRespawns = 0;
	extraGameInfo [0].bRobotsHitRobots = 0;
	extraGameInfo [0].bDualMissileLaunch = 0;
	extraGameInfo [0].bDropAllMissiles = 0;
	extraGameInfo [0].bImmortalPowerups = 0;
	extraGameInfo [0].bMultiBosses = 0;
	extraGameInfo [0].bSmartWeaponSwitch = 0;
	extraGameInfo [0].bFluidPhysics = 0;
	extraGameInfo [0].nWeaponDropMode = 0;
	extraGameInfo [0].nWeaponIcons = 0;
	extraGameInfo [0].nZoomMode = 0;
	extraGameInfo [0].nHitboxes = 0;
	extraGameInfo [0].bTripleFusion = 0;
	extraGameInfo [0].bKillMissiles = 0;
	gameOptions [1].gameplay.nAutoSelectWeapon = 2;
	gameOptions [1].gameplay.bSecretSave = 0;
	gameOptions [1].gameplay.bTurboMode = 0;
	gameOptions [1].gameplay.bFastRespawn = 0;
	gameOptions [1].gameplay.nAutoLeveling = 1;
	gameOptions [1].gameplay.bEscortHotKeys = 1;
	gameOptions [1].gameplay.bSkipBriefingScreens = 0;
	gameOptions [1].gameplay.bHeadlightOnWhenPickedUp = 1;
	gameOptions [1].gameplay.bShieldWarning = 0;
	gameOptions [1].gameplay.bInventory = 0;
	gameOptions [1].gameplay.bIdleAnims = 0;
	gameOptions [1].gameplay.nAIAwareness = 0;
	gameOptions [1].gameplay.nAIAggressivity = 0;
	}
else {
	gameOptions [0].gameplay.nAutoSelectWeapon = 2;
	gameOptions [0].gameplay.bSecretSave = 0;
	gameOptions [0].gameplay.bTurboMode = 0;
	gameOptions [0].gameplay.bFastRespawn = 0;
	gameOptions [0].gameplay.nAutoLeveling = 1;
	gameOptions [0].gameplay.bEscortHotKeys = 1;
	gameOptions [0].gameplay.bSkipBriefingScreens = 0;
	gameOptions [0].gameplay.bHeadlightOnWhenPickedUp = 0;
	gameOptions [0].gameplay.bShieldWarning = 0;
	gameOptions [0].gameplay.bInventory = 0;
	gameOptions [0].gameplay.bIdleAnims = 0;
	gameOptions [0].gameplay.nAIAwareness = 0;
	gameOptions [0].gameplay.nAIAggressivity = 0;
	}
}

//------------------------------------------------------------------------------

void InitMovieOptions (int i)
{
if (i) {
	gameOptions [1].movies.bHires = 1;
	gameOptions [1].movies.nQuality = 0;
	gameOptions [1].movies.nLevel = 2;
	gameOptions [1].movies.bResize = 0;
	gameOptions [1].movies.bFullScreen = 0;
	gameOptions [1].movies.bSubTitles = 1;
	}
else {
	gameOptions [0].movies.bHires = 1;
	gameOptions [0].movies.nQuality = 0;
	gameOptions [0].movies.nLevel = 2;
	gameOptions [0].movies.bResize = 1;
	gameOptions [1].movies.bFullScreen = 1;
	gameOptions [0].movies.bSubTitles = 1;
	}
}

//------------------------------------------------------------------------------

void InitLegacyOptions (int i)
{
if (i) {
	gameOptions [1].legacy.bInput = 0;
	gameOptions [1].legacy.bFuelCens = 0;
	gameOptions [1].legacy.bMouse = 0;
	gameOptions [1].legacy.bHomers = 0;
	gameOptions [1].legacy.bRender = 0;
	gameOptions [1].legacy.bSwitches = 0;
	gameOptions [1].legacy.bWalls = 0;
	}
else {
	gameOptions [0].legacy.bInput = 0;
	gameOptions [0].legacy.bFuelCens = 0;
	gameOptions [0].legacy.bMouse = 0;
	gameOptions [0].legacy.bHomers = 0;
	gameOptions [0].legacy.bRender = 0;
	gameOptions [0].legacy.bSwitches = 0;
	gameOptions [0].legacy.bWalls = 0;
	}
}

//------------------------------------------------------------------------------

void InitSoundOptions (int i)
{
if (i) {
	gameOptions [1].sound.bUseRedbook = 1;
	gameOptions [1].sound.digiSampleRate = SAMPLE_RATE_22K;
#if USE_SDL_MIXER
	gameOptions [1].sound.bUseSDLMixer = 1;
#else
	gameOptions [1].sound.bUseSDLMixer = 0;
#endif
	}
else {
	gameOptions [0].sound.bUseRedbook = 1;
	gameOptions [1].sound.digiSampleRate = SAMPLE_RATE_22K;
#if USE_SDL_MIXER
	gameOptions [0].sound.bUseSDLMixer = 1;
#else
	gameOptions [0].sound.bUseSDLMixer = 0;
#endif
	}
gameOptions [i].sound.bHires = 0;
gameOptions [i].sound.bShip = 0;
gameOptions [i].sound.bMissiles = 0;
gameOptions [i].sound.bFadeMusic = 1;
}

//------------------------------------------------------------------------------

void InitInputOptions (int i)
{
if (i) {
	extraGameInfo [0].bMouseLook = 0;
	gameOptions [1].input.bLimitTurnRate = 1;
	gameOptions [1].input.nMinTurnRate = 20;	//turn time for a 360 deg rotation around a single ship axis in 1/10 sec units
	if (gameOptions [1].input.joystick.bUse)
		gameOptions [1].input.mouse.bUse = 0;
	gameOptions [1].input.mouse.bSyncAxes = 1;
	gameOptions [1].input.mouse.bJoystick = 0;
	gameOptions [1].input.mouse.nDeadzone = 0;
	gameOptions [1].input.joystick.bSyncAxes = 1;
	gameOptions [1].input.keyboard.bUse = 1;
	gameOptions [1].input.bUseHotKeys = 1;
	gameOptions [1].input.keyboard.nRamp = 100;
	gameOptions [1].input.keyboard.bRamp [0] =
	gameOptions [1].input.keyboard.bRamp [1] =
	gameOptions [1].input.keyboard.bRamp [2] = 0;
	gameOptions [1].input.joystick.bLinearSens = 1;
	gameOptions [1].input.mouse.sensitivity [0] =
	gameOptions [1].input.mouse.sensitivity [1] =
	gameOptions [1].input.mouse.sensitivity [2] = 8;
	gameOptions [1].input.joystick.sensitivity [0] =
	gameOptions [1].input.joystick.sensitivity [1] =
	gameOptions [1].input.joystick.sensitivity [2] =
	gameOptions [1].input.joystick.sensitivity [3] = 8;
	gameOptions [1].input.joystick.deadzones [0] =
	gameOptions [1].input.joystick.deadzones [1] =
	gameOptions [1].input.joystick.deadzones [2] =
	gameOptions [1].input.joystick.deadzones [3] = 10;
	}
else {
	gameOptions [0].input.bLimitTurnRate = 1;
	gameOptions [0].input.nMinTurnRate = 20;	//turn time for a 360 deg rotation around a single ship axis in 1/10 sec units
	gameOptions [0].input.nMaxPitch = 0;
	gameOptions [0].input.joystick.bLinearSens = 0;
	gameOptions [0].input.keyboard.nRamp = 100;
	gameOptions [0].input.keyboard.bRamp [0] =
	gameOptions [0].input.keyboard.bRamp [1] =
	gameOptions [0].input.keyboard.bRamp [2] = 0;
	gameOptions [0].input.mouse.bUse = 1;
	gameOptions [0].input.joystick.bUse = 0;
	gameOptions [0].input.mouse.bSyncAxes = 1;
	gameOptions [0].input.mouse.nDeadzone = 0;
	gameOptions [0].input.mouse.bJoystick = 0;
	gameOptions [0].input.joystick.bSyncAxes = 1;
	gameOptions [0].input.keyboard.bUse = 1;
	gameOptions [0].input.bUseHotKeys = 1;
	gameOptions [0].input.mouse.nDeadzone = 2;
	gameOptions [0].input.mouse.sensitivity [0] =
	gameOptions [0].input.mouse.sensitivity [1] =
	gameOptions [0].input.mouse.sensitivity [2] = 8;
	gameOptions [0].input.trackIR.nDeadzone = 0;
	gameOptions [0].input.trackIR.bMove [0] =
	gameOptions [0].input.trackIR.bMove [1] = 1;
	gameOptions [0].input.trackIR.bMove [2] = 0;
	gameOptions [0].input.trackIR.sensitivity [0] =
	gameOptions [0].input.trackIR.sensitivity [1] =
	gameOptions [0].input.trackIR.sensitivity [2] = 8;
	gameOptions [0].input.joystick.sensitivity [0] =
	gameOptions [0].input.joystick.sensitivity [1] =
	gameOptions [0].input.joystick.sensitivity [2] =
	gameOptions [0].input.joystick.sensitivity [3] = 8;
	gameOptions [0].input.joystick.deadzones [0] =
	gameOptions [0].input.joystick.deadzones [1] =
	gameOptions [0].input.joystick.deadzones [2] =
	gameOptions [0].input.joystick.deadzones [3] = 10;
	}
}

// ----------------------------------------------------------------------------

void InitMultiplayerOptions (int i)
{
if (i) {
	extraGameInfo [0].bFriendlyFire = 1;
	extraGameInfo [0].bInhibitSuicide = 0;
	extraGameInfo [1].bMouseLook = 0;
	extraGameInfo [1].bDualMissileLaunch = 0;
	extraGameInfo [0].bAutoBalanceTeams = 0;
	extraGameInfo [1].bRotateLevels = 0;
	extraGameInfo [1].bDisableReactor = 0;
	gameOptions [1].multi.bNoRankings = 0;
	gameOptions [1].multi.bTimeoutPlayers = 1;
	gameOptions [1].multi.bUseMacros = 0;
	gameOptions [1].multi.bNoRedundancy = 0;
	}
else {
	gameOptions [0].multi.bNoRankings = 0;
	gameOptions [0].multi.bTimeoutPlayers = 1;
	gameOptions [0].multi.bUseMacros = 0;
	gameOptions [0].multi.bNoRedundancy = 0;
	}
}

// ----------------------------------------------------------------------------

void InitDemoOptions (int i)
{
if (i)
	gameOptions [i].demo.bOldFormat = 0;
else
	gameOptions [i].demo.bOldFormat = 1;
gameOptions [i].demo.bRevertFormat = 0;
}

// ----------------------------------------------------------------------------

void InitMenuOptions (int i)
{
if (i) {
	gameOptions [1].menus.nStyle = 0;
	gameOptions [1].menus.bFastMenus = 0;
	gameOptions [1].menus.bSmartFileSearch = 0;
	gameOptions [1].menus.bShowLevelVersion = 0;
	gameOptions [1].menus.altBg.alpha = 0;
	gameOptions [1].menus.altBg.brightness = 0;
	gameOptions [1].menus.altBg.grayscale = 0;
	gameOptions [1].menus.nHotKeys = 0;
	strcpy (gameOptions [0].menus.altBg.szName, "");
	}
else {
	gameOptions [0].menus.nStyle = 0;
	gameOptions [0].menus.bFastMenus = 0;
	gameOptions [0].menus.bSmartFileSearch = 1;
	gameOptions [0].menus.bShowLevelVersion = 0;
	gameOptions [0].menus.altBg.alpha = 0.75;
	gameOptions [0].menus.altBg.brightness = 0.5;
	gameOptions [0].menus.altBg.grayscale = 0;
	gameOptions [0].menus.nHotKeys = gameStates.app.bEnglish ? 1 : -1;
	strcpy (gameOptions [0].menus.altBg.szName, "menubg.tga");
	}
}

// ----------------------------------------------------------------------------

void InitOglOptions (int i)
{
if (i) {
	gameOptions [1].render.nLightingMethod = 0;
	gameOptions [1].ogl.bLightObjects = 0;
	gameOptions [1].ogl.bHeadlight = 0;
	gameOptions [1].ogl.bLightPowerups = 0;
	gameOptions [1].ogl.bObjLighting = 0;
	gameOptions [1].ogl.bSetGammaRamp = 0;
	gameOptions [1].ogl.bVoodooHack = 0;
	gameOptions [1].ogl.bGlTexMerge = 0;
	}
else {
#if DBG
	gameOptions [0].render.nLightingMethod = 0;
#else
	gameOptions [0].render.nLightingMethod = 0;
#endif
	gameOptions [0].ogl.bLightObjects = 0;
	gameOptions [0].ogl.bHeadlight = 0;
	gameOptions [0].ogl.bLightPowerups = 0;
	gameOptions [0].ogl.bObjLighting = 0;
	gameOptions [0].ogl.bSetGammaRamp = 0;
	gameOptions [0].ogl.bVoodooHack = 0;
	gameOptions [0].ogl.bGlTexMerge = 0;
	}
}

// ----------------------------------------------------------------------------

void InitAppOptions (int i)
{
if (i) {
	gameOptions [1].app.nVersionFilter = 2;
	gameOptions [1].app.bSinglePlayer = 0;
	gameOptions [1].app.bExpertMode = 0;
	gameOptions [1].app.nScreenShotInterval = 0;
	}
else {
	gameOptions [0].app.nVersionFilter = 2;
	gameOptions [0].app.bSinglePlayer = 0;
	gameOptions [0].app.bExpertMode = 1;
	gameOptions [0].app.nScreenShotInterval = 0;
	}
}

// ----------------------------------------------------------------------------

void InitGameOptions (int i)
{
if (i)
	if (gameStates.app.bNostalgia)
		gameOptions [1] = gameOptions [0];
	else
		return;
else
	memset (gameOptions, 0, sizeof (gameOptions));
InitInputOptions (i);
InitGameplayOptions (i);
InitRenderOptions (i);
InitMultiplayerOptions (i);
InitMenuOptions (i);
InitDemoOptions (i);
InitSoundOptions (i);
InitMovieOptions (i);
InitOglOptions (i);
InitLegacyOptions (i);
InitGameplayOptions (i);
}

// ----------------------------------------------------------------------------

void InitGameplayStates (void)
{
gameStates.gameplay.bFinalBossIsDead = 0;
gameStates.gameplay.bHaveSmartMines = 1;
gameStates.gameplay.bMineDestroyed = 0;
gameStates.gameplay.bNoBotAI = 0;
gameStates.gameplay.nPlayerSpeed = 0;
gameStates.gameplay.seismic.nMagnitude = 0;
gameStates.gameplay.seismic.nStartTime = 0;
gameStates.gameplay.seismic.nEndTime = 0;
gameStates.gameplay.seismic.nLevel = 0;
gameStates.gameplay.seismic.nShakeFrequency = 0;
gameStates.gameplay.seismic.nShakeDuration = 0;
gameStates.gameplay.seismic.nSound = SOUND_SEISMIC_DISTURBANCE_START;
gameStates.gameplay.seismic.bSound = 0;
gameStates.gameplay.seismic.nVolume = 0;
gameStates.gameplay.xStartingShields = INITIAL_SHIELDS;
gameStates.gameplay.slowmo [0].nState = 0;
gameStates.gameplay.slowmo [0].fSpeed = 1;
gameStates.gameplay.slowmo [0].tUpdate = 0;
gameStates.gameplay.slowmo [0].bActive = 0;
gameStates.gameplay.slowmo [1].nState = 0;
gameStates.gameplay.slowmo [1].fSpeed = 1;
gameStates.gameplay.slowmo [1].tUpdate = 0;
gameStates.gameplay.slowmo [1].bActive = 0;
gameOpts->gameplay.nSlowMotionSpeedup = 6;	//max slow motion delay
}

// ----------------------------------------------------------------------------

void InitInputStates (void)
{
gameStates.input.nMouseType = -1;
gameStates.input.nJoyType = -1;
gameStates.input.bCybermouseActive = 0;
#if !DBG
gameStates.input.bGrabMouse = 1;
#else
gameStates.input.bGrabMouse = 0;
#endif
gameStates.input.bSkipControls = 0;
gameStates.input.bControlsSkipFrame = 0;
gameStates.input.bKeepSlackTime = 0;
gameStates.input.nCruiseSpeed = 0;
}

// ----------------------------------------------------------------------------

void InitMenuStates (void)
{
gameStates.menus.bInitBG = 1;
gameStates.menus.nInMenu = 0;
gameStates.menus.bHires = 1;
gameStates.menus.bHiresAvailable = 1;
gameStates.menus.bDrawCopyright = 0;
gameStates.menus.bFullScreen = 0;
gameStates.menus.bReordering = 0;
}

// ----------------------------------------------------------------------------

void InitMovieStates (void)
{
gameStates.movies.nRobots = 0;
gameStates.movies.bIntroPlayed = 0;
gameStates.movies.bMac = 0;
}

// ----------------------------------------------------------------------------

void InitMultiplayerStates (void)
{
gameStates.multi.nGameType = 0;
gameStates.multi.nGameSubType = 0;
gameStates.multi.bUseTracker = 0;
gameStates.multi.bServer = 1;
gameStates.multi.bTryAutoDL = 0;
gameStates.multi.bHaveLocalAddress = 0;
gameStates.multi.bSurfingNet = 0;
gameStates.multi.bCheckPorts = 0;
}

// ----------------------------------------------------------------------------

void InitGfxStates (void)
{
gameStates.gfx.bInstalled = 0;
gameStates.gfx.nStartScrMode = 2;
gameStates.gfx.nStartScrSize = 2;
}

// ----------------------------------------------------------------------------

void InitOglStates (void)
{
gameStates.ogl.bInitialized = 0;
gameStates.ogl.bShadersOk = 0;
gameStates.ogl.bRender2TextureOk = 0;
gameStates.ogl.bPerPixelLightingOk = 1;
gameStates.ogl.bUseRender2Texture = 1;
gameStates.ogl.bVoodooHack = 0;
gameStates.ogl.bBrightness = 0;
gameStates.ogl.nContrast = 8;
gameStates.ogl.bFullScreen = 0;
gameStates.ogl.bUseTransform = 0;
gameStates.ogl.bDoPalStep = 0;
gameStates.ogl.nColorBits = 32;
gameStates.ogl.nDepthBits = 24;
gameStates.ogl.bEnableTexture2D = -1;
gameStates.ogl.bEnableTexClamp = -1;
gameStates.ogl.bEnableScissor = 0;
gameStates.ogl.bNeedMipMaps = 0;
gameStates.ogl.texMinFilter = GL_NEAREST;
gameStates.ogl.texMinFilter = GL_NEAREST;
gameStates.ogl.nTexMagFilterState = -1, 
gameStates.ogl.nTexMinFilterState = -1;
gameStates.ogl.nTexEnvModeState = -1, 
gameStates.ogl.nLastX =
gameStates.ogl.nLastY =
gameStates.ogl.nLastW =
gameStates.ogl.nLastH = 1;
gameStates.ogl.nCurWidth =
gameStates.ogl.nCurHeight = -1;
gameStates.ogl.bCurFullScreen = -1;
gameStates.ogl.bAntiAliasing = 0;
gameStates.ogl.bAntiAliasingOk = 0;
gameStates.ogl.bpp = 32;
gameStates.ogl.nRGBAFormat = GL_RGBA;
gameStates.ogl.nRGBFormat = GL_RGB;
gameStates.ogl.bIntensity4 = 1;
gameStates.ogl.bLuminance4Alpha4 = 1;
gameStates.ogl.bReadPixels = 1;
gameStates.ogl.bGetTexLevelParam = 1;
gameStates.ogl.nDrawBuffer = -1;
#ifdef GL_ARB_multitexture
gameStates.ogl.bArbMultiTexture = 0;
#endif
#ifdef GL_SGIS_multitexture
gameStates.ogl.bSgisMultiTexture = 0;
#endif
}

// ----------------------------------------------------------------------------

void InitRenderStates (void)
{
gameStates.render.xZoom = 0x9000;
gameStates.render.xZoomScale = 1;
gameStates.render.nFrameCount = -1;
gameStates.render.bQueryOcclusion = 0;
gameStates.render.bPointSprites = 0;
gameStates.render.bVertexArrays = 1;
gameStates.render.bExternalView = 0;
gameStates.render.bTopDownRadar = 0;
gameStates.render.bDisableFades = 0;
gameStates.render.bDropAfterburnerBlob = 0;
gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
gameStates.render.bShowFrameRate = 0;
gameStates.render.bShowTime = 0;
gameStates.render.cameras.bActive = 0;
gameStates.render.color.bLightmapsOk = 1;
gameStates.render.nZoomFactor = F1_0;
gameStates.render.nMinZoomFactor = F1_0;
gameStates.render.nMaxZoomFactor = F1_0 * 5;
gameStates.render.textures.bGlsTexMergeOk = 0;
gameStates.render.textures.bHaveMaskShader = 0;
gameStates.render.bBlendBackground = 0;
gameStates.render.fonts.bHires = 0;
gameStates.render.fonts.bHiresAvailable = 0;
gameStates.render.fonts.bInstalled = 0;
gameStates.render.nFlashScale = 0;
gameStates.render.nFlashRate = FLASH_CYCLE_RATE;
gameStates.render.bOutsideMine = 0;
gameStates.render.bExtExplPlaying = 0;
gameStates.render.bDoAppearanceEffect = 0;
gameStates.render.glFOV = DEFAULT_FOV;
gameStates.render.glAspect = 1.0f;
gameStates.render.bDetriangulation = 0;
gameStates.render.nInterpolationMethod = 0;
gameStates.render.bTMapFlat = 0;
gameStates.render.nLighting = 1;
gameStates.render.bTransparency = 0;
gameStates.render.bClusterLights = 1;
gameStates.render.bSplitPolys = 1;
gameStates.render.bHaveStencilBuffer = 0;
gameStates.render.nRenderPass = -1;
gameStates.render.nShadowPass = 0;
gameStates.render.bShadowMaps = 0;
gameStates.render.bHeadlightOn = 0;
gameStates.render.bPaletteFadedOut = 0;
gameStates.render.bRenderIndirect = 0;
gameStates.render.nModelQuality = 3;
gameStates.render.nType = -1;
gameStates.render.cockpit.bShowPingStats = 0;
gameStates.render.cockpit.nMode = CM_FULL_COCKPIT;
gameStates.render.cockpit.nNextMode = -1;
gameStates.render.cockpit.nModeSave = -1;
gameStates.render.cockpit.bRedraw = 0;
gameStates.render.cockpit.bBigWindowSwitch = 0;
gameStates.render.cockpit.nShieldFlash = 0;
gameStates.render.cockpit.nLastDrawn [0] =
gameStates.render.cockpit.nLastDrawn [1] = -1;
gameStates.render.cockpit.nCoopPlayerView [0] = 
gameStates.render.cockpit.nCoopPlayerView [1] = -1;
gameStates.render.cockpit.n3DView [0] =
gameStates.render.cockpit.n3DView [1] = CV_NONE;
gameStates.render.vr.xEyeWidth = F1_0;
gameStates.render.vr.nRenderMode	= VR_NONE;
gameStates.render.vr.nLowRes = 3;
gameStates.render.vr.bShowHUD	= 1;
gameStates.render.vr.nSensitivity = 1;
gameStates.render.detail.nRenderDepth = DEFAULT_RENDER_DEPTH;
gameStates.render.detail.nObjectComplexity = 2; 
gameStates.render.detail.nObjectDetail = 2;
gameStates.render.detail.nWallDetail = 2; 
gameStates.render.detail.nWallRenderDepth = 2; 
gameStates.render.detail.nDebrisAmount = 2; 
gameStates.render.bUsePerPixelLighting = 1;
gameStates.render.nMaxLightsPerPass = 8;
gameStates.render.nMaxLightsPerFace = 16;
}

// ----------------------------------------------------------------------------

void InitSoundStates (void)
{
gameStates.sound.nCurrentSong = 0;
gameStates.sound.bRedbookEnabled = 1;
gameStates.sound.bRedbookPlaying = 0;
gameStates.sound.bWasRecording = 0;
gameStates.sound.bDontStartObjects = 0;
gameStates.sound.nConquerWarningSoundChannel = -1;
gameStates.sound.nSoundChannels = 2;
gameStates.sound.digi.nMaxChannels = 16;
gameStates.sound.digi.nFreeChannel = 0;
gameStates.sound.digi.nVolume = SOUND_MAX_VOLUME;
gameStates.sound.digi.bInitialized = 0;
gameStates.sound.digi.nNextSignature = 0;
gameStates.sound.digi.nActiveObjects = 0;
gameStates.sound.digi.bSoundsInitialized = 0;
gameStates.sound.digi.bLoMem = 0;
gameStates.sound.digi.nLoopingSound = -1;
gameStates.sound.digi.nLoopingVolume = 0;
gameStates.sound.digi.nLoopingStart = -1;
gameStates.sound.digi.nLoopingEnd = -1;
gameStates.sound.digi.nLoopingChannel = -1;
}

// ----------------------------------------------------------------------------

void InitVideoStates (void)
{
gameStates.video.nDisplayMode = 2;
gameStates.video.nDefaultDisplayMode = 2;
gameStates.video.nScreenMode = (u_int32_t) -1;
gameStates.video.nLastScreenMode = (u_int32_t) -1;
gameStates.video.nWidth = -1;
gameStates.video.nHeight = -1;
gameStates.video.bFullScreen = -1;
}

// ----------------------------------------------------------------------------

void InitAppStates (void)
{
gameStates.app.bHaveExtraData = 0;
gameStates.app.bHaveExtraMovies = 0;
gameStates.app.nExtGameStatus = 1;
gameStates.app.nFunctionMode = FMODE_MENU;
gameStates.app.nLastFuncMode = -1;
gameStates.app.nCriticalError = 0;
gameStates.app.bNostalgia = 0;
gameStates.app.iNostalgia = 0;
gameStates.app.bD2XLevel = 0;
gameStates.app.bGameRunning = 0;
gameStates.app.bGameAborted = 0;
gameStates.app.bGameSuspended = 0;
gameStates.app.bEnterGame = 0;
gameStates.app.bUseSound = 1;
gameStates.app.bLunacy = 0;
gameStates.app.bEnableShadows = 1;
gameStates.app.bHaveExtraGameInfo [0] = 1;
gameStates.app.bHaveExtraGameInfo [1] = 0;
gameStates.app.nSDLTicks = -1;
#if DBG
gameStates.app.bEnglish = 1;
#else
gameStates.app.bEnglish = 1;
#endif
gameStates.app.bMacData = 0;
gameStates.app.bD1Model = 0;
gameStates.app.bD1Data = 0;
gameStates.app.bD1Mission = 0;
gameStates.app.bHaveD1Data = 0;
gameStates.app.bHaveD1Textures = 0;
gameStates.app.bEndLevelDataLoaded = 0;
gameStates.app.bEndLevelSequence = 0;
gameStates.app.bPlayerIsDead = 0;
gameStates.app.bPlayerExploded = 0;
gameStates.app.bPlayerEggsDropped = 0;
gameStates.app.bDeathSequenceAborted = 0;
gameStates.app.bUseDefaults = 0;
gameStates.app.nCompSpeed = 3;
gameStates.app.bPlayerFiredLaserThisFrame = 0;
gameStates.render.automap.bDisplay = 0;
gameStates.app.bConfigMenu = 0;
gameStates.app.bAutoRunMission = 0;
gameStates.entropy.bConquering = 0;
gameStates.entropy.bConquerWarning = 0;
gameStates.entropy.nTimeLastMoved = -1;
gameStates.app.bFirstSecretVisit = 1;
gameStates.app.nDifficultyLevel = DEFAULT_DIFFICULTY;
gameStates.app.nDetailLevel = NUM_DETAIL_LEVELS - 1;
gameStates.app.nBaseCtrlCenExplTime = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;
gameStates.app.bDebugSpew = 1;
gameStates.app.bProgressBars = 1;
gameStates.app.bFixModels = 0;
gameStates.app.bAltModels = 0;
gameStates.app.cheats.bEnabled = 0;
gameStates.app.cheats.bTurboMode = 0;
gameStates.app.cheats.bMonsterMode = 0;
gameStates.app.cheats.bLaserRapidFire = 0;
gameStates.app.cheats.bRobotsKillRobots = 0;
gameStates.app.cheats.bJohnHeadOn = 0;
gameStates.app.cheats.bHomingWeapons = 0;
gameStates.app.cheats.bBouncingWeapons = 0;
gameStates.app.cheats.bMadBuddy = 0;
gameStates.app.cheats.bAcid = 0;
gameStates.app.cheats.bPhysics = 0;
gameStates.app.cheats.bRobotsFiring = 1;
gameStates.app.cheats.bD1CheatsEnabled = 0;
gameStates.app.cheats.nUnlockLevel = 0;
gameStates.limitFPS.bControls = 1;
gameStates.limitFPS.bJoystick = !gameStates.limitFPS.bControls;
gameStates.limitFPS.bFusion = 0;
gameStates.limitFPS.bCountDown = 1;
gameStates.limitFPS.bSeismic = 1;
gameStates.limitFPS.bHomers = 1;
gameStates.limitFPS.bOmega = 1;
}

// ----------------------------------------------------------------------------

void InitGameStates (void)
{
memset (&gameStates, 0, sizeof (gameStates));
CheckEndian ();
InitGameConfig ();
InitAppStates ();
InitGameplayStates ();
InitInputStates ();
InitMenuStates ();
InitMovieStates ();
InitMultiplayerStates ();
InitGfxStates ();
InitOglStates ();
InitRenderStates ();
InitSoundStates ();
InitVideoStates ();
}

//------------------------------------------------------------------------------

void SetDataVersion (int v)
{
gameStates.app.bD1Data = (v < 0) ? gameStates.app.bD1Mission && gameStates.app.bHaveD1Data : v;
gameData.pig.tex.pBitmaps = gameData.pig.tex.bitmaps [gameStates.app.bD1Data];
gameData.pig.tex.pAltBitmaps = gameData.pig.tex.altBitmaps [gameStates.app.bD1Data];
gameData.pig.tex.pBmIndex = gameData.pig.tex.bmIndex [gameStates.app.bD1Data];
gameData.pig.tex.pBitmapFiles = gameData.pig.tex.bitmapFiles [gameStates.app.bD1Data];
gameData.pig.tex.pTMapInfo = gameData.pig.tex.tMapInfo [gameStates.app.bD1Data];
gameData.pig.sound.pSounds = gameData.pig.sound.sounds [gameStates.app.bD1Data];
gameData.eff.pEffects = gameData.eff.effects [gameStates.app.bD1Data];
gameData.eff.pVClips = gameData.eff.vClips [gameStates.app.bD1Data];
gameData.walls.pAnims = gameData.walls.anims [gameStates.app.bD1Data];
gameData.bots.pInfo = gameData.bots.info [gameStates.app.bD1Data];
}

// ----------------------------------------------------------------------------

void InitGameData (void)
{
	int	i;

memset (&gameData, 0, sizeof (gameData));
#if USE_IRRLICHT
memset (&irrData, 0, sizeof (irrData));
#endif
gameData.segs.nMaxSegments = MAX_SEGMENTS_D2X;
for (i = 0; i < MAX_BOSS_COUNT; i++)
	gameData.reactor.states [i].nDeadObj = -1;
gameData.bots.nCamBotId = -1;
gameData.bots.nCamBotModel = -1;
gameData.pig.ship.player = &gameData.pig.ship.only;
gameData.multiplayer.nPlayers = 1;
gameData.multiplayer.nLocalPlayer = 0;
gameData.multiplayer.nMaxPlayers = -1;
gameData.multiplayer.nPlayerPositions = -1;
gameData.missions.nCurrentLevel = 0;
gameData.missions.nLastMission = -1;
gameData.missions.nLastLevel = -1;
gameData.missions.nLastSecretLevel = -1;
gameData.marker.nHighlight = -1;
gameData.marker.viewers [0] =
gameData.marker.viewers [1] = -1;
gameData.missions.nEnteredFromLevel = -1;
strcpy (gameData.missions.szEndingFilename, "endreg.txt");
strcpy (gameData.missions.szBriefingFilename, "briefing.txt");
gameData.marker.fScale = 2.0f;
gameData.time.xFrame = 0x1000;
gameData.app.nGameMode = GM_GAME_OVER;
gameData.render.nPaletteGamma = -1;
gameData.reactor.bDestroyed = 0;
gameData.reactor.nStrength = 0;
gameData.reactor.countdown.nTimer = 0;
gameData.reactor.countdown.nSecsLeft = 0;
gameData.reactor.countdown.nTotalTime = 0;
gameData.render.transpColor = DEFAULT_TRANSPARENCY_COLOR; //transparency color bitmap index
gameData.render.shield.nFaceNodes = 4; //tesselate using quads
gameData.render.monsterball.nFaceNodes = 4; //tesselate using quads
gameData.render.mine.nVisited = 255;
gameData.render.mine.nProcessed = 255;
gameData.render.mine.nVisible = 255;
for (i = 0; i < MAX_BOSS_COUNT; i++) {
	gameData.boss [i].nCloakStartTime = 0;
	gameData.boss [i].nCloakEndTime = 0;
	gameData.boss [i].nLastTeleportTime = 0;
	gameData.boss [i].nTeleportInterval = F1_0 * 8;
	gameData.boss [i].nCloakInterval = F1_0 * 10;                    //    Time between cloaks
	gameData.boss [i].nCloakDuration = BOSS_CLOAK_DURATION;
	gameData.boss [i].nLastGateTime = 0;
	gameData.boss [i].nGateInterval = F1_0 * 6;
	}
gameData.ai.bInitialized = 0;
gameData.ai.bEvaded = 0;
gameData.ai.bEnableAnimation = 1;
gameData.ai.bInfoEnabled = 0;
gameData.ai.nAwarenessEvents = 0;
gameData.ai.nDistToLastPlayerPosFiredAt = 0;
gameData.ai.freePointSegs = gameData.ai.pointSegs;
gameData.menu.colorOverride = 0;
gameData.menu.alpha = 5 * 16 - 1;	//~ 0.3
gameData.menu.nLineWidth = 1;
gameData.matCens.xFuelRefillSpeed = I2X (1);
gameData.matCens.xFuelGiveAmount = I2X (25);
gameData.matCens.xFuelMaxAmount = I2X (100);
gameData.matCens.xEnergyToCreateOneRobot = I2X (1);
gameData.escort.nMaxLength = 200;
gameData.escort.nKillObject = -1;
gameData.escort.xLastPathCreated = 0;
gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
gameData.escort.nSpecialGoal = -1;
gameData.escort.nGoalIndex = -1;
gameData.escort.bMsgsSuppressed = 0;
gameData.objs.nNextSignature = 1;
gameData.objs.nMaxUsedObjects = MAX_OBJECTS - 20;
memset (gameData.objs.guidedMissile, 0, sizeof (gameData.objs.guidedMissile));
gameData.render.morph.xRate = MORPH_RATE;
gameData.render.ogl.nSrcBlend = GL_SRC_ALPHA;
gameData.render.ogl.nDestBlend = GL_ONE_MINUS_SRC_ALPHA;
memset (&gameData.render.lights.dynamic, 0xff, sizeof (gameData.render.lights.dynamic));
gameData.render.lights.bInitDynColoring = 1;
gameData.render.lights.dynamic.nLights = 0;
gameData.render.lights.dynamic.material.bValid = 0;
gameData.models.nSimpleModelThresholdScale = 5;
gameData.models.nMarkerModel = -1;
gameData.models.nCockpits = 0;
memset (gameData.models.nDyingModels, 0xff, sizeof (gameData.models.nDyingModels));
memset (gameData.models.nDeadModels, 0xff, sizeof (gameData.models.nDeadModels));
gameData.pig.tex.nFirstMultiBitmap = -1;
strcpy (gameData.escort.szName, "GUIDE-BOT");
strcpy (gameData.escort.szRealName, "GUIDE-BOT");
gameData.thief.xReInitTime = 0x3f000000;
for (i = 0; i < NDL; i++)
	gameData.thief.xWaitTimes [i] = F1_0 * (30 - i * 5);
gameData.hud.bPlayerMessage = 1;
gameData.hud.msgs [0].nColor = 
gameData.hud.msgs [1].nColor = -1;
gameData.render.terrain.uvlList [0][1].u = f1_0;
gameData.render.terrain.uvlList [0][2].v = f1_0;
gameData.render.terrain.uvlList [1][0].u = f1_0;
gameData.render.terrain.uvlList [1][1].u = f1_0;
gameData.render.terrain.uvlList [1][1].v = f1_0;
gameData.render.terrain.uvlList [1][2].v = f1_0;
gameData.physics.xAfterburnerCharge = f1_0;
gameData.physics.side.bCache = 1;
SetSpherePulse (&gameData.render.shield.pulse, 0.02f, 0.5f);
UseSpherePulse (&gameData.render.shield, &gameData.render.shield.pulse);
SetSpherePulse (&gameData.render.monsterball.pulse, 0.005f, 0.9f);
UseSpherePulse (&gameData.render.monsterball, &gameData.render.monsterball.pulse);
gameData.smoke.iFree = -1;
gameData.smoke.iUsed = -1;
gameData.smoke.nLastType = -1;
gameData.lightnings.iFree = -1;
gameData.lightnings.iUsed = -1;
gameData.omega.xCharge [0] = 
gameData.omega.xCharge [1] = 
gameData.omega.xMaxCharge = DEFAULT_MAX_OMEGA_CHARGE;
memset (gameData.cockpit.gauges, 0xff, sizeof (gameData.cockpit.gauges));
gameData.render.ogl.zNear = 1.0f;
gameData.render.ogl.zFar = 5000.0f;
gameData.score.nPhallicMan = -1;
InitEndLevelData ();
InitStringPool ();
SetDataVersion (-1);
}

// ----------------------------------------------------------------------------

#define	GETMEM(_t,_p,_s,_f)	(_p) = (_t *) GetMem ((_s) * sizeof (*(_p)), _f)

void *GetMem (size_t size, char filler)
{
	void	*p = D2_ALLOC ((unsigned int) size);

if (p) {
	memset (p, filler, size);
	return p;
	}
Error (TXT_OUT_OF_MEMORY);
exit (1);
}

// ----------------------------------------------------------------------------

void AllocSegmentData (void)
{
GETMEM (vmsVector, gameData.segs.vertices, 65536, 0);
GETMEM (fVector, gameData.segs.fVertices, 65536, 0);
GETMEM (tSegment, gameData.segs.segments, MAX_SEGMENTS, 0);
GETMEM (tSegment2, gameData.segs.segment2s, MAX_SEGMENTS, 0);
GETMEM (xsegment, gameData.segs.xSegments, MAX_SEGMENTS, 0);
GETMEM (g3sPoint, gameData.segs.points, 65536, 0);
GETMEM (short, gameData.segs.objects, MAX_SEGMENTS, 0);
#if CALC_SEGRADS
GETMEM (fix, gameData.segs.segRads [0], MAX_SEGMENTS, 0);
GETMEM (fix, gameData.segs.segRads [1], MAX_SEGMENTS, 0);
GETMEM (tSegExtent, gameData.segs.extent, MAX_SEGMENTS, 0);
#endif
GETMEM (vmsVector, gameData.segs.segCenters [0], MAX_SEGMENTS, 0);
GETMEM (vmsVector, gameData.segs.segCenters [1], MAX_SEGMENTS, 0);
GETMEM (vmsVector, gameData.segs.sideCenters, MAX_SEGMENTS * 6, 0);
GETMEM (ubyte, gameData.segs.bSegVis, MAX_SEGMENTS * MAX_SEGVIS_FLAGS, 0);
GETMEM (tSlideSegs, gameData.segs.slideSegs, MAX_SEGMENTS, 0);
GETMEM (tSegFaces, gameData.segs.segFaces, MAX_SEGMENTS, 0);
GETMEM (grsFace, gameData.segs.faces.faces, MAX_FACES, 0);
GETMEM (grsTriangle, gameData.segs.faces.tris, MAX_TRIANGLES, 0);
GETMEM (fVector3, gameData.segs.faces.vertices, MAX_TRIANGLES * 3, 0);
#if USE_RANGE_ELEMENTS
GETMEM (GLuint, gameData.segs.faces.vertIndex, MAX_TRIANGLES * 3, 0);
#endif
GETMEM (ushort, gameData.segs.faces.faceVerts, MAX_FACES * 16, 0);
GETMEM (fVector3, gameData.segs.faces.normals, MAX_TRIANGLES * 3 * 2, 0);
GETMEM (tRgbaColorf, gameData.segs.faces.color, MAX_TRIANGLES * 3, 0);
GETMEM (tTexCoord2f, gameData.segs.faces.texCoord, MAX_TRIANGLES * 2 * 3, 0);
GETMEM (tTexCoord2f, gameData.segs.faces.lMapTexCoord, MAX_FACES * 2, 0);
gameData.segs.faces.ovlTexCoord = gameData.segs.faces.texCoord + MAX_FACES * 4;
}

// ----------------------------------------------------------------------------

void AllocObjectData (void)
{
GETMEM (tObject, gameData.objs.objects, MAX_OBJECTS, 0);
GETMEM (short, gameData.objs.freeList, MAX_OBJECTS, 0);
GETMEM (tLightObjId, gameData.objs.lightObjs, MAX_OBJECTS, (char) 0xff);
GETMEM (tShotInfo, gameData.objs.shots, MAX_OBJECTS, (char) 0xff);
GETMEM (short, gameData.objs.parentObjs, MAX_OBJECTS, (char) 0xff);
GETMEM (tObjectRef, gameData.objs.childObjs, MAX_OBJECTS, 0);
GETMEM (short, gameData.objs.firstChild, MAX_OBJECTS, (char) 0xff);
GETMEM (tObject, gameData.objs.init, MAX_OBJECTS, 0);
GETMEM (tObjDropInfo, gameData.objs.dropInfo, MAX_OBJECTS, 0);
GETMEM (tSpeedBoostData, gameData.objs.speedBoost, MAX_OBJECTS, 0);
GETMEM (vmsVector, gameData.objs.vRobotGoals, MAX_OBJECTS, 0);
GETMEM (vmsVector, gameData.objs.vStartVel, MAX_OBJECTS, 0);
GETMEM (fix, gameData.objs.xLastAfterburnerTime, MAX_OBJECTS, 0);
GETMEM (fix, gameData.objs.xCreationTime, MAX_OBJECTS, 0);
GETMEM (fix, gameData.objs.xTimeLastHit, MAX_OBJECTS, 0);
GETMEM (fix, gameData.objs.xLight, MAX_OBJECTS, 0);
GETMEM (int, gameData.objs.nLightSig, MAX_OBJECTS, 0);
GETMEM (ubyte, gameData.objs.nTracers, MAX_OBJECTS, 0);
GETMEM (ushort, gameData.objs.cameraRef, MAX_OBJECTS, 0);
GETMEM (short, gameData.objs.nHitObjects, MAX_OBJECTS * MAX_HIT_OBJECTS, 0);
GETMEM (tObjectViewData, gameData.objs.viewData, MAX_OBJECTS, (char) 0xFF);
GETMEM (tShrapnelData, gameData.objs.shrapnels, MAX_OBJECTS, 0);
}

// ----------------------------------------------------------------------------

void AllocSmokeData (void)
{
GETMEM (short, gameData.smoke.objects, MAX_OBJECTS, (char) 0xff);
GETMEM (time_t, gameData.smoke.objExplTime, MAX_OBJECTS, 0);
}

// ----------------------------------------------------------------------------

void AllocLightningData (void)
{
GETMEM (short, gameData.lightnings.objects, MAX_OBJECTS, (char) 0xff);
GETMEM (tLightningLight, gameData.lightnings.lights, MAX_SEGMENTS, (char) 0xff);
}

// ----------------------------------------------------------------------------

void AllocCameraData (void)
{
GETMEM (char, gameData.cameras.nSides, 2 * MAX_SEGMENTS * 6, 0);
}

// ----------------------------------------------------------------------------

void AllocRenderColorData (void)
{
GETMEM (tFaceColor, gameData.render.color.lights, MAX_SEGMENTS * 6, 0);
GETMEM (tFaceColor, gameData.render.color.sides, MAX_SEGMENTS * 6, 0);
GETMEM (tFaceColor, gameData.render.color.segments, MAX_SEGMENTS, 0);
GETMEM (tFaceColor, gameData.render.color.vertices, MAX_VERTICES, 0);
GETMEM (float, gameData.render.color.vertBright, MAX_VERTICES, 0);
GETMEM (tFaceColor, gameData.render.color.ambient, MAX_VERTICES, 0);	//static light values
GETMEM (tLightRef, gameData.render.color.visibleLights, MAX_SEGMENTS * 6, 0);
}

// ----------------------------------------------------------------------------

void AllocRenderLightData (void)
{
if (!gameStates.app.bNostalgia) {
	GETMEM (short, gameData.render.lights.dynamic.nNearestSegLights, MAX_SEGMENTS * MAX_NEAREST_LIGHTS, 0);
	GETMEM (short, gameData.render.lights.dynamic.nNearestVertLights, MAX_VERTICES * MAX_NEAREST_LIGHTS, 0);
	GETMEM (ubyte, gameData.render.lights.dynamic.nVariableVertLights, MAX_VERTICES, 0);
	GETMEM (short, gameData.render.lights.dynamic.owners, MAX_OBJECTS, (char) 0xff);
	}
GETMEM (fix, gameData.render.lights.segDeltas, MAX_SEGMENTS * 6, 0);
GETMEM (tLightDeltaIndex, gameData.render.lights.deltaIndices, MAX_DL_INDICES, 0);
GETMEM (tLightDelta, gameData.render.lights.deltas, MAX_DELTA_LIGHTS, 0);
GETMEM (ubyte, gameData.render.lights.subtracted, MAX_SEGMENTS, 0);
GETMEM (fix, gameData.render.lights.dynamicLight, MAX_VERTICES, 0);
GETMEM (tRgbColorf, gameData.render.lights.dynamicColor, MAX_VERTICES, 0);
GETMEM (char, gameData.render.lights.bGotDynColor, MAX_VERTICES, 0);
GETMEM (short, gameData.render.lights.vertices, MAX_VERTICES, 0);
GETMEM (sbyte, gameData.render.lights.vertexFlags, MAX_VERTICES, 0);
GETMEM (sbyte, gameData.render.lights.newObjects, MAX_OBJECTS, 0);
GETMEM (sbyte, gameData.render.lights.objects, MAX_OBJECTS, 0);
GETMEM (GLuint, gameData.render.lights.coronaQueries, MAX_OGL_LIGHTS, 0);
GETMEM (GLuint, gameData.render.lights.coronaSamples, MAX_OGL_LIGHTS, 0);
}

// ----------------------------------------------------------------------------

void AllocRenderData (void)
{
GETMEM (tFaceListItem, gameData.render.faceList, MAX_FACES, 0);
}

// ----------------------------------------------------------------------------

void AllocPhysicsData (void)
{
GETMEM (short, gameData.physics.ignoreObjs, MAX_OBJECTS, 0);
}

// ----------------------------------------------------------------------------

void AllocShadowData (void)
{
if (!gameStates.app.bNostalgia && gameStates.app.bEnableShadows)
	GETMEM (short, gameData.render.shadows.objLights, MAX_OBJECTS * MAX_SHADOW_LIGHTS, 0);
}

// ----------------------------------------------------------------------------

void AllocWeaponData (void)
{
GETMEM (tRgbaColorf, gameData.weapons.color, MAX_OBJECTS, 0);
}

// ----------------------------------------------------------------------------

void AllocMultiData (void)
{
GETMEM (tLeftoverPowerup, gameData.multiplayer.leftoverPowerups, MAX_OBJECTS, 0);
GETMEM (short, gameData.multigame.remoteToLocal, MAX_NUM_NET_PLAYERS * MAX_OBJECTS, 0);  // Remote tObject number for each local tObject
GETMEM (short, gameData.multigame.localToRemote, MAX_OBJECTS, 0);
GETMEM (sbyte, gameData.multigame.nObjOwner, MAX_OBJECTS, 0);   // Who created each tObject in my universe, -1 = loaded at start
}

// ----------------------------------------------------------------------------

void AllocAIData (void)
{
GETMEM (tAILocalInfo, gameData.ai.localInfo, MAX_OBJECTS, 0);
}

// ----------------------------------------------------------------------------

void AllocDemoData (void)
{
GETMEM (sbyte, gameData.demo.bWasRecorded,  MAX_OBJECTS, 0);
GETMEM (sbyte, gameData.demo.bViewWasRecorded, MAX_OBJECTS, 0);
}

// ----------------------------------------------------------------------------

void AllocGameData (void)
{
AllocSegmentData ();
AllocObjectData ();
AllocSmokeData ();
AllocLightningData ();
AllocCameraData ();
AllocRenderColorData ();
AllocRenderLightData ();
AllocRenderData ();
AllocShadowData ();
AllocPhysicsData ();
AllocWeaponData ();
AllocAIData ();
AllocMultiData ();
AllocDemoData ();
}

// ----------------------------------------------------------------------------

#define FREEMEM(_t,_p,_s) {D2_FREE (_p); (_p) = (_t *) NULL; }

void FreeSegmentData (void)
{
FREEMEM (vmsVector, gameData.segs.vertices, MAX_VERTICES);
FREEMEM (fVector, gameData.segs.fVertices, MAX_VERTICES);
FREEMEM (tSegment, gameData.segs.segments, MAX_SEGMENTS);
FREEMEM (tSegment2, gameData.segs.segment2s, MAX_SEGMENTS);
FREEMEM (xsegment, gameData.segs.xSegments, MAX_SEGMENTS);
FREEMEM (g3sPoint, gameData.segs.points, MAX_VERTICES);
FREEMEM (short, gameData.segs.objects, MAX_SEGMENTS);
#if CALC_SEGRADS
FREEMEM (fix, gameData.segs.segRads [0], MAX_SEGMENTS);
FREEMEM (fix, gameData.segs.segRads [1], MAX_SEGMENTS);
FREEMEM (tSegExtent, gameData.segs.extent, MAX_SEGMENTS);
#endif
FREEMEM (tSegFaces, gameData.segs.segFaces, MAX_SEGMENTS);
FREEMEM (grsFace, gameData.segs.faces.faces, MAX_FACES);
FREEMEM (grsTriangle, gameData.segs.faces.tris, MAX_TRIANGLES);
FREEMEM (fVector3, gameData.segs.faces.vertices, MAX_TRIANGLES * 3);
#if USE_RANGE_ELEMENTS
FREEMEM (GLuint, gameData.segs.faces.vertIndex, MAX_TRIANGLES * 3, 0);
#endif
FREEMEM (ushort, gameData.segs.faces.faceVerts, MAX_FACES * 16);
FREEMEM (fVector3, gameData.segs.faces.normals, MAX_TRIANGLES * 3);
FREEMEM (tRgbaColorf, gameData.segs.faces.color, MAX_TRIANGLES * 3 * 2);
FREEMEM (tTexCoord2f, gameData.segs.faces.texCoord, MAX_TRIANGLES * 3 * 2);
FREEMEM (tTexCoord2f, gameData.segs.faces.lMapTexCoord, MAX_FACES * 2);
FREEMEM (vmsVector, gameData.segs.segCenters [0], MAX_SEGMENTS);
FREEMEM (vmsVector, gameData.segs.segCenters [1], MAX_SEGMENTS);
FREEMEM (vmsVector, gameData.segs.sideCenters, MAX_SEGMENTS * 6);
FREEMEM (ubyte, gameData.segs.bSegVis, MAX_SEGMENTS * MAX_SEGVIS_FLAGS);
FREEMEM (tSlideSegs, gameData.segs.slideSegs, MAX_SEGMENTS);
}

// ----------------------------------------------------------------------------

void FreeObjectData (void)
{
FREEMEM (tObject, gameData.objs.objects, MAX_OBJECTS);
FREEMEM (short, gameData.objs.freeList, MAX_OBJECTS);
FREEMEM (tLightObjId, gameData.objs.lightObjs, MAX_OBJECTS);
FREEMEM (tShotInfo, gameData.objs.shots, MAX_OBJECTS);
FREEMEM (short, gameData.objs.parentObjs, MAX_OBJECTS);
FREEMEM (tObjectRef, gameData.objs.childObjs, MAX_OBJECTS);
FREEMEM (short, gameData.objs.firstChild, MAX_OBJECTS);
FREEMEM (tObject, gameData.objs.init, MAX_OBJECTS);
FREEMEM (tObjDropInfo, gameData.objs.dropInfo, MAX_OBJECTS);
FREEMEM (tSpeedBoostData, gameData.objs.speedBoost, MAX_OBJECTS);
FREEMEM (vmsVector, gameData.objs.vRobotGoals, MAX_OBJECTS);
FREEMEM (vmsVector, gameData.objs.vStartVel, MAX_OBJECTS);
FREEMEM (fix, gameData.objs.xLastAfterburnerTime, MAX_OBJECTS);
FREEMEM (fix, gameData.objs.xCreationTime, MAX_OBJECTS);
FREEMEM (fix, gameData.objs.xTimeLastHit, MAX_OBJECTS);
FREEMEM (fix, gameData.objs.xLight, MAX_OBJECTS);
FREEMEM (int, gameData.objs.nLightSig, MAX_OBJECTS);
FREEMEM (ubyte, gameData.objs.nTracers, MAX_OBJECTS);
FREEMEM (ushort, gameData.objs.cameraRef, MAX_OBJECTS);
FREEMEM (short, gameData.objs.nHitObjects, MAX_OBJECTS * MAX_HIT_OBJECTS);
FREEMEM (tObjectViewData, gameData.objs.viewData, MAX_OBJECTS);
FREEMEM (tShrapnelData, gameData.objs.shrapnels, MAX_OBJECTS);
}

// ----------------------------------------------------------------------------

void FreeSmokeData (void)
{
FREEMEM (short, gameData.smoke.objects, MAX_OBJECTS);
FREEMEM (time_t, gameData.smoke.objExplTime, MAX_OBJECTS);
}

// ----------------------------------------------------------------------------

void FreeLightningData (void)
{
PrintLog ("unloading lightning data\n");
DestroyAllLightnings (1);
FREEMEM (tLightningLight, gameData.lightnings.lights, MAX_SEGMENTS);
FREEMEM (short, gameData.lightnings.objects, MAX_OBJECTS);
}

// ----------------------------------------------------------------------------

void FreeCameraData (void)
{
FREEMEM (char, gameData.cameras.nSides, MAX_SEGMENTS * 6);
}

// ----------------------------------------------------------------------------

void FreeRenderColorData (void)
{
FREEMEM (tFaceColor, gameData.render.color.lights, MAX_SEGMENTS);
FREEMEM (tFaceColor, gameData.render.color.sides, MAX_SEGMENTS);
FREEMEM (tFaceColor, gameData.render.color.segments, MAX_SEGMENTS);
FREEMEM (tFaceColor, gameData.render.color.vertices, MAX_VERTICES);
FREEMEM (float, gameData.render.color.vertBright, MAX_VERTICES);
FREEMEM (tFaceColor, gameData.render.color.ambient, MAX_VERTICES);	//static light values
FREEMEM (tLightRef, gameData.render.color.visibleLights, MAX_SEGMENTS * 6);
}

// ----------------------------------------------------------------------------

void FreeRenderLightData (void)
{
if (!gameStates.app.bNostalgia) {
	FREEMEM (short, gameData.render.lights.dynamic.nNearestSegLights, MAX_SEGMENTS * MAX_NEAREST_LIGHTS);
	FREEMEM (short, gameData.render.lights.dynamic.nNearestVertLights, MAX_SEGMENTS * MAX_NEAREST_LIGHTS);
	FREEMEM (ubyte, gameData.render.lights.dynamic.nVariableVertLights, MAX_SEGMENTS * MAX_NEAREST_LIGHTS);
	FREEMEM (short, gameData.render.lights.dynamic.owners, MAX_OBJECTS);
	}
FREEMEM (fix, gameData.render.lights.segDeltas, MAX_SEGMENTS * 6);
FREEMEM (tLightDeltaIndex, gameData.render.lights.deltaIndices, MAX_DL_INDICES);
FREEMEM (tLightDelta, gameData.render.lights.deltas, MAX_DELTA_LIGHTS);
FREEMEM (ubyte, gameData.render.lights.subtracted, MAX_SEGMENTS);
FREEMEM (fix, gameData.render.lights.dynamicLight, MAX_VERTICES);
FREEMEM (tRgbColorf, gameData.render.lights.dynamicColor, MAX_VERTICES);
FREEMEM (char, gameData.render.lights.bGotDynColor, MAX_VERTICES);
FREEMEM (short, gameData.render.lights.vertices, MAX_VERTICES);
FREEMEM (sbyte, gameData.render.lights.vertexFlags, MAX_VERTICES);
FREEMEM (sbyte, gameData.render.lights.newObjects, MAX_OBJECTS);
FREEMEM (sbyte, gameData.render.lights.objects, MAX_OBJECTS);
FREEMEM (GLuint, gameData.render.lights.coronaQueries, MAX_OGL_LIGHTS);
FREEMEM (GLuint, gameData.render.lights.coronaSamples, MAX_OGL_LIGHTS);
}

// ----------------------------------------------------------------------------

void FreeRenderData (void)
{
FREEMEM (tFaceListItem, gameData.render.faceList, MAX_FACES);
}

// ----------------------------------------------------------------------------

void FreePhysicsData (void)
{
FREEMEM (short, gameData.physics.ignoreObjs, MAX_OBJECTS);
}

// ----------------------------------------------------------------------------

void FreeShadowData (void)
{
if (!gameStates.app.bNostalgia && gameStates.app.bEnableShadows)
	FREEMEM (short, gameData.render.shadows.objLights, MAX_OBJECTS * MAX_SHADOW_LIGHTS);
}

// ----------------------------------------------------------------------------

void FreeWeaponData (void)
{
FREEMEM (tRgbaColorf, gameData.weapons.color, MAX_OBJECTS);
}

// ----------------------------------------------------------------------------

void FreeMultiData (void)
{
FREEMEM (tLeftoverPowerup, gameData.multiplayer.leftoverPowerups, MAX_OBJECTS);
FREEMEM (short, gameData.multigame.remoteToLocal, MAX_NUM_NET_PLAYERS * MAX_OBJECTS);  // Remote tObject number for each local tObject
FREEMEM (short, gameData.multigame.localToRemote, MAX_OBJECTS);
FREEMEM (sbyte, gameData.multigame.nObjOwner, MAX_OBJECTS);   // Who created each tObject in my universe, -1 = loaded at start
}

// ----------------------------------------------------------------------------

void FreeAIData (void)
{
FREEMEM (tAILocalInfo, gameData.ai.localInfo, MAX_OBJECTS);
}

// ----------------------------------------------------------------------------

void FreeDemoData (void)
{
FREEMEM (sbyte, gameData.demo.bWasRecorded,  MAX_OBJECTS);
FREEMEM (sbyte, gameData.demo.bViewWasRecorded, MAX_OBJECTS);
}

// ----------------------------------------------------------------------------

void FreeGameData (void)
{
FreeSegmentData ();
FreeObjectData ();
FreeSmokeData ();
FreeLightningData ();
FreeCameraData ();
FreeRenderColorData ();
FreeRenderLightData ();
FreeRenderData ();
FreeShadowData ();
FreePhysicsData ();
FreeWeaponData ();
FreeAIData ();
FreeMultiData ();
FreeDemoData ();
}

// ----------------------------------------------------------------------------

void PrintBanner (void)
{
#if (defined (_WIN32) || defined (__unix__))
con_printf(CON_NORMAL, "\nDESCENT 2 %s v%d.%d.%d\n", VERSION_TYPE, D2X_MAJOR, D2X_MINOR, D2X_MICRO);
#elif defined(__macosx__)
con_printf(CON_NORMAL, "\nDESCENT 2 %s -- %s\n", VERSION_TYPE, DESCENT_VERSION);
#endif
if (gameHogFiles.D2XHogFiles.bInitialized)
	con_printf ((int) CON_NORMAL, "  Vertigo Enhanced\n");
con_printf(CON_NORMAL, "\nBuilt: %s %s\n", __DATE__, __TIME__);
#ifdef __VERSION__
con_printf(CON_NORMAL, "Compiler: %s\n", __VERSION__);
#endif
con_printf(CON_NORMAL, "\n");
if (FindArg ("-?") || FindArg ("-help") || FindArg ("?") || FindArg ("-h")) {
	PrintCmdLineHelp ();
   set_exit_message ("");
#ifdef __MINGW32__
	exit (0);  /* mingw hangs on this return.  dunno why */
#endif
	}
}

// ----------------------------------------------------------------------------

int ShowTitleScreens (void)
{
	int nPlayed = MOVIE_NOT_PLAYED;	//default is not nPlayed
#if DBG
if (FindArg ("-notitles"))
	SongsPlaySong (SONG_TITLE, 1);
else
#endif
	{	//NOTE LINK TO ABOVE!
	int bSongPlaying = 0;
	if (gameStates.app.bHaveExtraMovies) {
		nPlayed = PlayMovie ("starta.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
		if (nPlayed == MOVIE_ABORTED)
			nPlayed = MOVIE_PLAYED_FULL;
		else
			nPlayed = PlayMovie ("startb.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
		}
	else {
		PlayIntroMovie ();
		}

	if (!bSongPlaying)
		SongsPlaySong (SONG_TITLE, 1);
	}
return (nPlayed != MOVIE_NOT_PLAYED);	//default is not nPlayed
}

// ----------------------------------------------------------------------------

void ShowLoadingScreen (void)
{
if (!gameStates.app.bNostalgia) {
		bkg	bg;

	memset (&bg, 0, sizeof (bg));
	NMInitBackground (NULL, &bg, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight, -1);
	GrUpdate (0);
	}
else {
		int pcx_error;
		char filename [14];

#if TRACE	
	con_printf(CONDBG, "\nShowing loading screen...\n"); fflush (fErr);
#endif
	strcpy (filename, gameStates.menus.bHires ? "descentb.pcx" : "descent.pcx");
	if (!CFExist (filename, gameFolders.szDataDir, 0))
		strcpy (filename, gameStates.menus.bHires ? "descntob.pcx" : "descento.pcx"); // OEM
	if (!CFExist (filename, gameFolders.szDataDir, 0))
		strcpy (filename, "descentd.pcx"); // SHAREWARE
	if (!CFExist (filename, gameFolders.szDataDir, 0))
		strcpy (filename, "descentb.pcx"); // MAC SHAREWARE
	GrSetMode (
		gameStates.menus.bHires ? 
			(gameStates.gfx.nStartScrMode < 0) ? 
				SM (640, 480) 
				: SM (scrSizes [gameStates.gfx.nStartScrMode].x, scrSizes [gameStates.gfx.nStartScrMode].y) 
			: SM (320, 200));
	SetScreenMode (SCREEN_MENU);
	gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && gameStates.menus.bHires;
	if ((pcx_error = PcxReadFullScrImage (filename, 0)) == PCX_ERROR_NONE)	{
		GrPaletteStepClear ();
		GrPaletteFadeIn (gameData.render.pal.pCurPal, 32, 0);
		} 
	else
		Error ("Couldn't load pcx file '%s', \nPCX load error: %s\n", filename, pcx_errormsg (pcx_error));
	}
}

// ----------------------------------------------------------------------------

void LoadHoardData (void)
{
#ifdef EDITOR
if (FindArg ("-hoarddata")) {
#define MAX_BITMAPS_PER_BRUSH 30
	grsBitmap * bm[MAX_BITMAPS_PER_BRUSH];
	grsBitmap icon;
	int nframes;
	short nframes_short;
	ubyte palette[256*3];
	FILE *ofile;
	int iff_error, i;

	char *sounds[] = {"selforb.raw", "selforb.r22", 		//SOUND_YOU_GOT_ORB		
							"teamorb.raw", "teamorb.r22", 		//SOUND_FRIEND_GOT_ORB		
							"enemyorb.raw", "enemyorb.r22", 	//SOUND_OPPONENT_GOT_ORB
							"OPSCORE1.raw", "OPSCORE1.r22"};	//SOUND_OPPONENT_HAS_SCORED

	ofile = fopen ("hoard.ham", "wb");

	iff_error = iff_read_animbrush ("orb.abm", bm, MAX_BITMAPS_PER_BRUSH, &nframes, palette);
	Assert (iff_error == IFF_NO_ERROR);
	nframes_short = nframes;
	fwrite (&nframes_short, sizeof (nframes_short), 1, ofile);
	fwrite (&bm[0]->bmProps.w, sizeof (short), 1, ofile);
	fwrite (&bm[0]->bmProps.h, sizeof (short), 1, ofile);
	fwrite (palette, 3, 256, ofile);
	for (i=0;i<nframes;i++)
		fwrite (bm[i]->bmTexBuf, 1, bm[i]->bmProps.w*bm[i]->bmProps.h, ofile);

	iff_error = iff_read_animbrush ("orbgoal.abm", bm, MAX_BITMAPS_PER_BRUSH, &nframes, palette);
	Assert (iff_error == IFF_NO_ERROR);
	Assert (bm[0]->bmProps.w == 64 && bm[0]->bmProps.h == 64);
	nframes_short = nframes;
	fwrite (&nframes_short, sizeof (nframes_short), 1, ofile);
	fwrite (palette, 3, 256, ofile);
	for (i=0;i<nframes;i++)
		fwrite (bm[i]->bmTexBuf, 1, bm[i]->bmProps.w*bm[i]->bmProps.h, ofile);

	for (i=0;i<2;i++) {
		iff_error = iff_read_bitmap (i?"orbb.bbm":"orb.bbm", &icon, BM_LINEAR, palette);
		Assert (iff_error == IFF_NO_ERROR);
		fwrite (&icon.bmProps.w, sizeof (short), 1, ofile);
		fwrite (&icon.bmProps.h, sizeof (short), 1, ofile);
		fwrite (palette, 3, 256, ofile);
		fwrite (icon.bmTexBuf, 1, icon.bmProps.w*icon.bmProps.h, ofile);
		}

	for (i=0;i<sizeof (sounds)/sizeof (*sounds);i++) {
		FILE *cf;
		int size;
		ubyte *buf;
		cf = fopen (sounds[i], "rb");
		Assert (cf != NULL);
		size = ffilelength (cf);
		buf = D2_ALLOC (size);
		fread (buf, 1, size, cf);
		fwrite (&size, sizeof (size), 1, ofile);
		fwrite (buf, 1, size, ofile);
		D2_FREE (buf);
		fclose (cf);
		}
	fclose (ofile);
	exit (1);
	}
#endif
}

// ----------------------------------------------------------------------------

void GrabMouse (int bGrab, int bForce)
{
#ifdef SDL_INPUT
if (gameStates.input.bGrabMouse && (bForce || gameStates.app.bGameRunning))
	SDL_WM_GrabInput ((bGrab || gameStates.ogl.bFullScreen) ? SDL_GRAB_ON : SDL_GRAB_OFF);
#endif
}

// ----------------------------------------------------------------------------

void MainLoop (void)
{
while (gameStates.app.nFunctionMode != FMODE_EXIT) {
	gameStates.app.bGameRunning = (gameStates.app.nFunctionMode == FMODE_GAME);
	switch (gameStates.app.nFunctionMode)	{
		case FMODE_MENU:
			SetScreenMode (SCREEN_MENU);
			if (gameData.demo.bAuto && !gameOpts->demo.bRevertFormat) {
				NDStartPlayback (NULL);		// Randomly pick a file
				if (gameData.demo.nState != ND_STATE_PLAYBACK)
				Error ("No demo files were found for autodemo mode!");
				}
			else {
#ifdef EDITOR
				if (Auto_exit) {
					strcpy ((char *)&gameData.missions.szLevelNames [0], Auto_file);
					LoadLevel (1, 1, 0);
					SetFunctionMode (FMODE_EXIT);
					break;
					}
#endif
				check_joystick_calibration ();
				GrPaletteStepClear ();		//I'm not sure why we need this, but we do
				MainMenu ();
#ifdef EDITOR
				if (gameStates.app.nFunctionMode == FMODE_EDITOR)	{
					create_new_mine ();
					SetPlayerFromCurseg ();
					LoadPalette (NULL, 1, 0);
					}
#endif
				}
			if (gameData.multiplayer.autoNG.bValid && (gameStates.app.nFunctionMode != FMODE_GAME))
				gameStates.app.nFunctionMode = FMODE_EXIT;
			break;

		case FMODE_GAME:
#ifdef EDITOR
			gameStates.input.keys.bEditorMode = 0;
#endif
			GrabMouse (1, 1);
			RunGame ();
			GrabMouse (0, 1);
			GrPaletteFadeIn (NULL, 0, 0);
			ResetPaletteAdd ();
			GrPaletteFadeOut (NULL, 0, 0);
			gameStates.app.bD1Mission = 0;
			if (gameData.multiplayer.autoNG.bValid)
				gameStates.app.nFunctionMode = FMODE_EXIT;
			if (gameStates.app.nFunctionMode == FMODE_MENU) {
				DigiStopAllChannels ();
				SongsPlaySong (SONG_TITLE, 1);
				}
			RestoreDefaultRobots ();
			break;

#ifdef EDITOR
		case FMODE_EDITOR:
			gameStates.input.keys.bEditorMode = 1;
			editor ();
#ifdef __WATCOMC__
			_harderr ((void*)descent_critical_error_handler);		// Reinstall game error handler
#endif
			if (gameStates.app.nFunctionMode == FMODE_GAME) {
				gameData.app.nGameMode = GM_EDITOR;
				editor_reset_stuff_onLevel ();
				gameData.multiplayer.nPlayers = 1;
				}
			break;
#endif
		default:
			Error ("Invalid function mode %d", gameStates.app.nFunctionMode);
		}
	}
}

// ----------------------------------------------------------------------------

void InitThreads (void)
{
if (gameStates.app.bMultiThreaded) {
	int	i;
	for (i = 0; i < 2; i++) {
#if MULTI_THREADED_LIGHTS
		gameData.threads.vertColor.info [i].done = SDL_CreateSemaphore (0);
		gameData.threads.vertColor.info [i].exec = SDL_CreateSemaphore (0);
		gameData.threads.vertColor.info [i].bDone =
		gameData.threads.vertColor.info [i].bExec =
		gameData.threads.vertColor.info [i].bQuit = 0;
		gameData.threads.vertColor.info [i].nId = i;
		gameData.threads.vertColor.info [i].pThread = SDL_CreateThread (VertexColorThread, &gameData.threads.vertColor.info [i].nId);
#endif
#if MULTI_THREADED_SHADOWS
		gameData.threads.clipDist.info [i].done = SDL_CreateSemaphore (0);
		gameData.threads.clipDist.info [i].exec = SDL_CreateSemaphore (0);
		gameData.threads.clipDist.info [i].nId = i;
		gameData.threads.clipDist.info [i].pThread = SDL_CreateThread (ClipDistThread, &gameData.threads.clipDist.info [i].nId);
#endif
		}
	StartRenderThreads ();
	}
StartSoundThread ();
gameData.render.vertColor.matAmbient[R] = 
gameData.render.vertColor.matAmbient[G] = 
gameData.render.vertColor.matAmbient[B] = AMBIENT_LIGHT;
gameData.render.vertColor.matAmbient[A] = 1.0f;
gameData.render.vertColor.matDiffuse[R] = 
gameData.render.vertColor.matDiffuse[G] = 
gameData.render.vertColor.matDiffuse[B] = DIFFUSE_LIGHT;
gameData.render.vertColor.matDiffuse[A] = 1.0f;
gameData.render.vertColor.matSpecular[R] = 
gameData.render.vertColor.matSpecular[G] = 
gameData.render.vertColor.matSpecular[B] = 0.0f;
gameData.render.vertColor.matSpecular[A] = 1.0f;
}

// ------------------------------------------------------------------------------------------

tpfnTIRInit		pfnTIRInit = NULL;
tpfnTIRExit		pfnTIRExit = NULL;
tpfnTIRStart	pfnTIRStart = NULL;
tpfnTIRStop		pfnTIRStop = NULL;
tpfnTIRCenter	pfnTIRCenter = NULL;
tpfnTIRQuery	pfnTIRQuery = NULL;

#define	LOAD_TIR_FUNC(_t,_f) \
			if (!((pfn ## _f) = (_t) GetProcAddress (hTIRDll, #_f))) \
				return TIRUnload ();

#ifdef _WIN32
static HINSTANCE hTIRDll = 0;
#endif

// ------------------------------------------------------------------------------------------

int TIRUnload (void)
{
#ifdef WIN32
if (hTIRDll)
   FreeLibrary (hTIRDll);
pfnTIRInit = NULL;
pfnTIRExit = NULL;
pfnTIRStart = NULL;
pfnTIRStop = NULL;
pfnTIRCenter = NULL;
pfnTIRQuery = NULL;
hTIRDll = 0;
#endif
return gameStates.input.bHaveTrackIR = 0;
}

// ------------------------------------------------------------------------------------------

int TIRLoad (void)
{
#ifdef WIN32
if (hTIRDll)
	return 1;
hTIRDll = LoadLibrary ("d2x-trackir.dll");
if ((size_t) hTIRDll < HINSTANCE_ERROR) {
	hTIRDll = NULL;
	return gameStates.input.bHaveTrackIR = 0;
	}
LOAD_TIR_FUNC (tpfnTIRInit, TIRInit)
LOAD_TIR_FUNC (tpfnTIRExit, TIRExit)
LOAD_TIR_FUNC (tpfnTIRStart, TIRStart)
LOAD_TIR_FUNC (tpfnTIRStop, TIRStop)
LOAD_TIR_FUNC (tpfnTIRCenter, TIRCenter)
LOAD_TIR_FUNC (tpfnTIRQuery, TIRQuery)
if (pfnTIRInit ((HWND) SDL_GetWindowHandle ()))
	return gameStates.input.bHaveTrackIR = 1;
TIRUnload ();
return gameStates.input.bHaveTrackIR = 0;
#else
return gameStates.input.bHaveTrackIR = 0;
#endif
}

// ----------------------------------------------------------------------------

int InitGraphics (void)
{
	u_int32_t	nScreenSize;
	int			t;

/*---*/PrintLog ("Initializing graphics\n");
if ((t = GrInit ())) {		//doesn't do much
	PrintLog ("Cannot initialize graphics\n");
	Error (TXT_CANT_INIT_GFX, t);
	return 0;
	}
nScreenSize = SM (scrSizes [gameStates.gfx.nStartScrMode].x, scrSizes [gameStates.gfx.nStartScrMode].y);
#ifdef _3DFX
_3dfx_Init ();
#endif

/*---*/PrintLog ("Initializing render buffers\n");
if (!gameStates.render.vr.buffers.offscreen)	//if hasn't been initialied (by headset init)
	SetDisplayMode (gameStates.gfx.nStartScrMode, gameStates.gfx.bOverride);		//..then set default display mode
/*---*/PrintLog ("Loading default palette\n");
defaultPalette = GrUsePaletteTable (D2_DEFAULT_PALETTE, NULL);
grdCurCanv->cvBitmap.bmPalette = defaultPalette;	//just need some valid palette here
/*---*/PrintLog ("Initializing game fonts\n");
GameFontInit ();	// must load after palette data loaded.
/*---*/PrintLog ("Setting screen mode\n");
SetScreenMode (SCREEN_MENU);
/*---*/PrintLog ("Showing loading screen\n");
ShowLoadingScreen ();
return 1;
}

//------------------------------------------------------------------------------

static inline int InitGaugeSize (void)
{
return 21;
}

//------------------------------------------------------------------------------

static int loadOp = 0;

static int InitializePoll (int nItems, tMenuItem *m, int *key, int nCurItem)
{
GrPaletteStepLoad (NULL);
switch (loadOp) {
	case 0:
		/*---*/PrintLog ("Creating default tracker list\n");
		CreateTrackerList ();
		break;
	case 1:
		/*---*/PrintLog ("Loading ban list\n");
		LoadBanList ();
		break;
	case 2:
		/*---*/PrintLog ("Initializing control types\n");
		SetControlType ();
		break;
	case 3:
		/*---*/PrintLog ("Initializing keyboard\n");
		KeyInit ();
		break;
	case 4:
		/*---*/PrintLog ("Initializing joystick\n");
		DoJoystickInit ();
		break;
	case 5:
		int i;
		if ((i = FindArg ("-xcontrol")) > 0)
			KCInitExternalControls (strtol (pszArgList [i+1], NULL, 0), strtol (pszArgList [i+2], NULL, 0));
		break;
	case 6:
		/*---*/PrintLog ("Initializing movies\n");
		if (FindArg ("-nohires") || FindArg ("-nohighres") || !GrVideoModeOK (MENU_HIRES_MODE))
			gameOpts->movies.bHires = 
			gameStates.menus.bHires = 
			gameStates.menus.bHiresAvailable = 0;
		else
			gameStates.menus.bHires = gameStates.menus.bHiresAvailable = 1;
		InitMovies ();		//init movie libraries
		break;
	case 7:
		/*---*/PrintLog ("Initializing game data\n");
		InitWeaponFlags ();
		break;
	case 8:
		BMInit ();
		break;
	case 9:
		/*---*/PrintLog ("Initializing sound\n");
		DigiInit (1);
		break;
	case 10:
		/*---*/PrintLog ("Loading hoard data\n");
		LoadHoardData ();
		break;
	case 11:
		error_init (ShowInGameWarning, NULL);
		break;
	case 12:
		break;
		g3_init ();
	case 13:
		/*---*/PrintLog ("Initializing texture merge buffer\n");
		TexMergeInit (100); // 100 cache bitmaps
		break;
	case 14:
		InitPowerupTables ();
		break;
	case 15:
		InitGame ();
		break;
	case 16:
		InitThreads ();
		break;
	case 17:
		PiggyInitMemory ();
		break;
	case 18:
		if (!FindArg("-nomouse"))
			MouseInit ();
		break;
	case 19:
		/*---*/PrintLog ("Enabling TrackIR support\n");
		TIRLoad ();
		break;
	}
loadOp++;
if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
	if (loadOp == InitGaugeSize ())
		*key = -2;
	else {
		m [0].value++;
		m [0].rebuild = 1;
		*key = 0;
		}
	}
GrPaletteStepLoad (NULL);
return nCurItem;
}

//------------------------------------------------------------------------------

void InitializeGauge (void)
{
loadOp = 0;
NMProgressBar (TXT_INITIALIZING, 0, InitGaugeSize (), InitializePoll); 
}

// ----------------------------------------------------------------------------

int Initialize (int argc, char *argv[])
{
/*---*/PrintLog ("Initializing data\n");
signal (SIGABRT, D2SignalHandler);
signal (SIGFPE, D2SignalHandler);
signal (SIGILL, D2SignalHandler);
signal (SIGINT, D2SignalHandler);
signal (SIGSEGV, D2SignalHandler);
signal (SIGTERM, D2SignalHandler);
#ifdef _WIN32
SDL_SetSpecialKeyHandling (0);
#endif
CFileInit ("", "");
InitGameData ();
InitGameStates ();
InitExtraGameInfo ();
InitNetworkData ();
InitAutoDL ();
InitGameOptions (0);
InitArgs (argc, argv);
GetAppFolders ();
InitArgs (argc, argv);
GetAppFolders ();
if (FindArg ("-debug-printlog") || FindArg ("-printlog")) {
	   char fnErr [FILENAME_LEN];
#ifdef __unix__
	sprintf (fnErr, "%s/d2x.log", getenv ("HOME"));
	fErr = fopen (fnErr, "wt");
#else
	sprintf (fnErr, "%s/d2x.log", gameFolders.szGameDir);
	fErr = fopen (fnErr, "wt");
#endif
	}
PrintLog ("%s\n", DESCENT_VERSION);
con_init ();  // Initialise the console
#ifdef D2X_MEM_HANDLER
MemInit ();
#endif
error_init (NULL, NULL);
*szAutoHogFile =
*szAutoMission = '\0';
EvalArgs ();
InitGameOptions (1);
gameOpts->render.nMathFormat = gameOpts->render.nDefMathFormat;
AllocGameData ();
/*---*/PrintLog ("Loading text resources\n");
CFSetCriticalErrorCounterPtr (&nDescentCriticalError);
/*---*/PrintLog ("Loading main hog file\n");
if (!(CFileInit ("descent2.hog", gameFolders.szDataDir) || 
	  (gameStates.app.bDemoData = CFileInit ("d2demo.hog", gameFolders.szDataDir)))) {
	/*---*/PrintLog ("Descent 2 data not found\n");
	Error (TXT_NO_HOG2);
	}
LoadGameTexts ();
if (*szAutoHogFile && *szAutoMission) {
	CFUseAltHogFile (szAutoHogFile);
	gameStates.app.bAutoRunMission = gameHogFiles.AltHogFiles.bInitialized;
	}
/*---*/PrintLog ("Reading configuration file\n");
ReadConfigFile ();
if (!InitGraphics ())
	return 1;
if (gameStates.app.bProgressBars && gameOpts->menus.nStyle)
	InitializeGauge ();
else {
	ShowBoxedMessage (TXT_INITIALIZING);
	for (loadOp = 0; loadOp < InitGaugeSize (); )
		InitializePoll (0, NULL, NULL, 0);
	}
PrintBanner ();
if (!gameStates.app.bAutoRunMission) {
	/*---*/PrintLog ("Showing title screens\n");
	if (!ShowTitleScreens ())
		ShowLoadingScreen ();
	}
if (FindArg ("-norun"))
	return 0;
/*---*/PrintLog ("Loading hires models\n");
LoadHiresModels (0);
LoadModelData ();
InitShaders ();
return 0;
}

// ----------------------------------------------------------------------------

int CleanUp (void)
{
if (gameStates.input.bHaveTrackIR) {
	pfnTIRExit ();
	TIRUnload ();
	}
SongsStopAll ();
DigiStopCurrentSong ();
SaveModelData ();
/*---*/PrintLog ("Saving configuration file\n");
WriteConfigFile ();
/*---*/PrintLog ("Saving player profile\n");
WritePlayerFile ();
/*---*/PrintLog ("Releasing tracker list\n");
DestroyTrackerList ();
FreeParams ();
#if DBG
if (!FindArg ("-notitles"))
#endif
	//ShowOrderForm ();
OglDestroyDrawBuffer ();
FreeGameData ();
return 0;
}

// ----------------------------------------------------------------------------

int SDLCALL main (int argc, char *argv[])
{
gameStates.app.bInitialized = 0;
if (Initialize (argc, argv))
	return -1;
//	If built with editor, option to auto-load a level and quit game
//	to write certain data.
#ifdef	EDITOR
if ((i = FindArg ("-autoload"))) {
	Auto_exit = 1;
	strcpy (Auto_file, pszArgList[i+1]);
	}
if (Auto_exit) {
	strcpy (gameData.multiplayer.players[0].callsign, "dummy");
	} 
else
#endif
/*---*/PrintLog ("Loading player profile\n");
DoSelectPlayer ();
StartSoundThread (); //needs to be repeated here due to dependency on data read in DoSelectPlayer()
GrPaletteFadeOut (NULL, 32, 0);
// handle direct loading and starting of a mission specified via the command line
if (gameStates.app.bAutoRunMission) {
	gameStates.app.nFunctionMode = StartNewGame (1) ? FMODE_GAME : FMODE_MENU;
	gameStates.app.bAutoRunMission = 0;
	}
// handle automatic launch of a demo playback
if (gameData.demo.bAuto && !gameOpts->demo.bRevertFormat) {
	NDStartPlayback (gameData.demo.fnAuto);	
	if (gameData.demo.nState == ND_STATE_PLAYBACK)
		SetFunctionMode (FMODE_GAME);
	}
//do this here because the demo code can do a __asm int 3; longjmp when trying to
//autostart a demo from the main menu, never having gone into the game
setjmp (gameExitPoint);
/*---*/PrintLog ("Invoking main menu\n");
gameStates.app.bInitialized = 1;
MainLoop ();
CleanUp ();
return 0;		//presumably successful exit
}

// ----------------------------------------------------------------------------

void check_joystick_calibration ()
{
#if 0//ndef _WIN32
	int x1, y1, x2, y2, c;
	fix t1;

	if ((gameConfig.nControlType!=CONTROL_JOYSTICK) &&
		  (gameConfig.nControlType!=CONTROL_FLIGHTSTICK_PRO) &&
		  (gameConfig.nControlType!=CONTROL_THRUSTMASTER_FCS) &&
		  (gameConfig.nControlType!=CONTROL_GRAVIS_GAMEPAD)
		) return;
	JoyGetpos (&x1, &y1);
	t1 = TimerGetFixedSeconds ();
	while (TimerGetFixedSeconds () < t1 + F1_0/100)
		;
	JoyGetpos (&x2, &y2);
	// If joystick hasn't moved...
	if ((abs (x2-x1)<30) &&  (abs (y2-y1)<30))	{
		if ((abs (x1)>30) || (abs (x2)>30) ||  (abs (y1)>30) || (abs (y2)>30))	{
			c = ExecMessageBox (NULL, NULL, 2, TXT_CALIBRATE, TXT_SKIP, TXT_JOYSTICK_NOT_CEN);
			if (c==0)	{
				JoyDefsCalibrate ();
			}
		}
	}
#endif
}

// ----------------------------------------------------------------------------

void ShowOrderForm ()
{
#ifndef EDITOR
	int 	pcx_error;
	ubyte titlePal [768];
	char	exit_screen[16];

	GrSetCurrentCanvas (NULL);
	GrPaletteStepClear ();

	KeyFlush ();

	strcpy (exit_screen, gameStates.menus.bHires?"ordrd2ob.pcx":"ordrd2o.pcx"); // OEM
	if (! CFExist (exit_screen, gameFolders.szDataDir, 0))
		strcpy (exit_screen, gameStates.menus.bHires?"orderd2b.pcx":"orderd2.pcx"); // SHAREWARE, prefer mac if hires
	if (! CFExist (exit_screen, gameFolders.szDataDir, 0))
		strcpy (exit_screen, gameStates.menus.bHires?"orderd2.pcx":"orderd2b.pcx"); // SHAREWARE, have to rescale
	if (! CFExist (exit_screen, gameFolders.szDataDir, 0))
		strcpy (exit_screen, gameStates.menus.bHires?"warningb.pcx":"warning.pcx"); // D1
	if (! CFExist (exit_screen, gameFolders.szDataDir, 0))
		return; // D2 registered

	if ((pcx_error=PcxReadFullScrImage (exit_screen, 0))==PCX_ERROR_NONE) {
		GrPaletteFadeIn (NULL, 32, 0);
		GrUpdate (0);
		while (!(KeyInKey () || MouseButtonState (0)))
			;
		GrPaletteFadeOut (titlePal, 32, 0);
	}
	else
		Int3 ();		//can't load order screen

	KeyFlush ();
#endif
}

// ----------------------------------------------------------------------------

void quit_request ()
{
exit (0);
}

// ----------------------------------------------------------------------------
