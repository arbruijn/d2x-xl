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
#include "hogfile.h"
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
#include "automap.h"
#include "banlist.h"

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
CGameData		gameData;

//static const char desc_id_checksum_str[] = DESC_ID_CHKSUM_TAG "0000"; // 4-byte checksum

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
#endif

void check_joystick_calibration (void);

void ShowOrderForm (void);
void quit_request ();

tGameOptions *gameOpts = gameOptions;

void EvalArgs (void);
void GetAppFolders (void);
void InitGameStates (void);

//--------------------------------------------------------------------------

char szAutoMission [255];
char szAutoHogFile [255];

extern void SetMaxPitch (int nMinTurnRate);

int nDescentCriticalError = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

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
CCanvas::SetCurrent (NULL);
fontManager.SetCurrent (GAME_FONT);
fontManager.Current ()->StringSize ("V2.2", w, h, aw);
fontManager.SetColorRGBi (RGBA_PAL (63, 47, 0), 1, 0, 0);
GrPrintF (NULL, 0x8000, CCanvas::Current ()->Height () - GAME_FONT->Height () - 2, "visit www.descent2.de");
fontManager.SetColorRGBi (RGBA_PAL (23, 23, 23), 1, 0, 0);
GrPrintF (NULL, 0x8000, CCanvas::Current ()->Height () - GAME_FONT->Height () - h - 6, TXT_COPYRIGHT);
GrPrintF (NULL, CCanvas::Current ()->Width () - w - 2, 
			 CCanvas::Current ()->Height () - GAME_FONT->Height () - h - 6, "V%d.%d", D2X_MAJOR, D2X_MINOR);
if (bVertigo < 0)
	bVertigo = CFile::Exist ("d2x.hog", gameFolders.szMissionDir, 0);
if (bVertigo) {
	fontManager.SetCurrent (MEDIUM2_FONT);
	fontManager.Current ()->StringSize (TXT_VERTIGO, w, h, aw);
	GrPrintF (NULL, CCanvas::Current ()->Width () - w - SUBVER_XOFFS, 
				 y + (gameOpts->menus.altBg.bHave ? h + 2 : 0), TXT_VERTIGO);
	}
fontManager.SetCurrent (MEDIUM2_FONT);
fontManager.Current ()->StringSize (D2X_NAME, w, h, aw);
GrPrintF (NULL, CCanvas::Current ()->Width () - w - SUBVER_XOFFS, 
			 y + ((bVertigo && !gameOpts->menus.altBg.bHave) ? h + 2 : 0), D2X_NAME);
fontManager.SetCurrent (SMALL_FONT);
fontManager.Current ()->StringSize (VERSION, ws, hs, aw);
fontManager.SetColorRGBi (D2BLUE_RGBA, 1, 0, 0);
GrPrintF (NULL, CCanvas::Current ()->Width () - ws - 1, 
			 y + ((bVertigo && !gameOpts->menus.altBg.bHave) ? h + 2 : 0) + (h - hs) / 2, VERSION);
fontManager.SetColorRGBi (RGBA_PAL (6, 6, 6), 1, 0, 0);
}

// ----------------------------------------------------------------------------

void CheckEndian (void)
{
	union {
		short	s;
		ubyte	b [2];
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
char	Auto_file [128] = "";
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

void PrintBanner (void)
{
#if (defined (_WIN32) || defined (__unix__))
console.printf(CON_NORMAL, "\nDESCENT 2 %s v%d.%d.%d\n", VERSION_TYPE, D2X_MAJOR, D2X_MINOR, D2X_MICRO);
#elif defined(__macosx__)
console.printf(CON_NORMAL, "\nDESCENT 2 %s -- %s\n", VERSION_TYPE, DESCENT_VERSION);
#endif
if (hogFileManager.D2XFiles ().bInitialized)
	console.printf ((int) CON_NORMAL, "  Vertigo Enhanced\n");
console.printf(CON_NORMAL, "\nBuilt: %s %s\n", __DATE__, __TIME__);
#ifdef __VERSION__
console.printf(CON_NORMAL, "Compiler: %s\n", __VERSION__);
#endif
console.printf(CON_NORMAL, "\n");
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
	NMInitBackground (NULL, &bg, 0, 0, screen.Width (), screen.Height (), -1);
	GrUpdate (0);
	}
else {
		int pcx_error;
		char filename [14];

#if TRACE	
	console.printf(CON_DBG, "\nShowing loading screen...\n"); fflush (fErr);
#endif
	strcpy (filename, gameStates.menus.bHires ? "descentb.pcx" : "descent.pcx");
	if (!CFile::Exist (filename, gameFolders.szDataDir, 0))
		strcpy (filename, gameStates.menus.bHires ? "descntob.pcx" : "descento.pcx"); // OEM
	if (!CFile::Exist (filename, gameFolders.szDataDir, 0))
		strcpy (filename, "descentd.pcx"); // SHAREWARE
	if (!CFile::Exist (filename, gameFolders.szDataDir, 0))
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
		paletteManager.ClearEffect ();
		paletteManager.FadeIn ();
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
	CBitmap * bm[MAX_BITMAPS_PER_BRUSH];
	CBitmap icon;
	int nframes;
	short nframes_short;
	ubyte palette [256*3];
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
	fwrite (&bm[0]->Width (), sizeof (short), 1, ofile);
	fwrite (&bm[0]->Height (), sizeof (short), 1, ofile);
	fwrite (palette, 3, 256, ofile);
	for (i=0;i<nframes;i++)
		fwrite (bm[i]->buffer, 1, bm[i]->Width ()*bm[i]->Height (), ofile);

	iff_error = iff_read_animbrush ("orbgoal.abm", bm, MAX_BITMAPS_PER_BRUSH, &nframes, palette);
	Assert (iff_error == IFF_NO_ERROR);
	Assert (bm[0]->Width () == 64 && bm[0]->Height () == 64);
	nframes_short = nframes;
	fwrite (&nframes_short, sizeof (nframes_short), 1, ofile);
	fwrite (palette, 3, 256, ofile);
	for (i=0;i<nframes;i++)
		fwrite (bm[i]->buffer, 1, bm[i]->Width ()*bm[i]->Height (), ofile);

	for (i=0;i<2;i++) {
		iff_error = iff_read_bitmap (i?"orbb.bbm":"orb.bbm", &icon, BM_LINEAR, palette);
		Assert (iff_error == IFF_NO_ERROR);
		fwrite (&icon.Width (), sizeof (short), 1, ofile);
		fwrite (&icon.Height (), sizeof (short), 1, ofile);
		fwrite (palette, 3, 256, ofile);
		fwrite (icon.buffer, 1, icon.Width ()*icon.Height (), ofile);
		}

	for (i = 0; i < sizeof (sounds)/sizeof (*sounds);i++) {
		FILE *cf = fopen (sounds[i], "rb");
		Assert (cf != NULL);
		int size = ffilelength (cf);
		ubyte* buf = new ubyte [size];
		fread (buf, 1, size, cf);
		fwrite (&size, sizeof (size), 1, ofile);
		fwrite (buf, 1, size, ofile);
		delete [] buf;
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
					strcpy (reinterpret_cast<char*> (&gameData.missions.szLevelNames [0]), Auto_file);
					LoadLevel (1, 1, 0);
					SetFunctionMode (FMODE_EXIT);
					break;
					}
#endif
				check_joystick_calibration ();
				paletteManager.ClearEffect ();		//I'm not sure why we need this, but we do
				MainMenu ();
#ifdef EDITOR
				if (gameStates.app.nFunctionMode == FMODE_EDITOR)	{
					create_new_mine ();
					SetPlayerFromCurseg ();
					paletteManager.Load (NULL, 1, 0);
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
			paletteManager.FadeIn ();
			paletteManager.ResetEffect ();
			paletteManager.FadeOut ();
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
gameData.render.vertColor.matDiffuse [R] = 
gameData.render.vertColor.matDiffuse [G] = 
gameData.render.vertColor.matDiffuse [B] = DIFFUSE_LIGHT;
gameData.render.vertColor.matDiffuse [A] = 1.0f;
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
paletteManager.SetDefault (paletteManager.Load (D2_DEFAULT_PALETTE, NULL));
CCanvas::Current ()->SetPalette (paletteManager.Default ());	//just need some valid palette here
/*---*/PrintLog ("Initializing game fonts\n");
fontManager.Setup ();	// must load after palette data loaded.
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
paletteManager.LoadEffect  ();
switch (loadOp) {
	case 0:
		/*---*/PrintLog ("Creating default tracker list\n");
		CreateTrackerList ();
		break;
	case 1:
		/*---*/PrintLog ("Loading ban list\n");
		banList.Load ();
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
paletteManager.LoadEffect  ();
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
hogFileManager.Init ("", "");
InitGameStates ();
gameData.Init ();
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
#ifdef D2X_MEM_HANDLER
MemInit ();
#endif
error_init (NULL, NULL);
*szAutoHogFile =
*szAutoMission = '\0';
EvalArgs ();
InitGameOptions (1);
gameOpts->render.nMathFormat = gameOpts->render.nDefMathFormat;
gameData.Create ();
/*---*/PrintLog ("Loading text resources\n");
/*---*/PrintLog ("Loading main hog file\n");
if (!(hogFileManager.Init ("descent2.hog", gameFolders.szDataDir) || 
	  (gameStates.app.bDemoData = hogFileManager.Init ("d2demo.hog", gameFolders.szDataDir)))) {
	/*---*/PrintLog ("Descent 2 data not found\n");
	Error (TXT_NO_HOG2);
	}
LoadGameTexts ();
if (*szAutoHogFile && *szAutoMission) {
	hogFileManager.UseAlt (szAutoHogFile);
	gameStates.app.bAutoRunMission = hogFileManager.AltFiles ().bInitialized;
	}
/*---*/PrintLog ("Reading configuration file\n");
ReadConfigFile ();
if (!InitGraphics ())
	return 1;
console.Setup (SMALL_FONT, &screen, CON_NUM_LINES, 0, 0, screen.Width (), screen.Height () / 2);
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
paletteManager.FadeOut ();
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
	char	exit_screen[16];

	CCanvas::SetCurrent (NULL);
	paletteManager.ClearEffect ();

	KeyFlush ();

	strcpy (exit_screen, gameStates.menus.bHires?"ordrd2ob.pcx":"ordrd2o.pcx"); // OEM
	if (! CFile::Exist (exit_screen, gameFolders.szDataDir, 0))
		strcpy (exit_screen, gameStates.menus.bHires?"orderd2b.pcx":"orderd2.pcx"); // SHAREWARE, prefer mac if hires
	if (! CFile::Exist (exit_screen, gameFolders.szDataDir, 0))
		strcpy (exit_screen, gameStates.menus.bHires?"orderd2.pcx":"orderd2b.pcx"); // SHAREWARE, have to rescale
	if (! CFile::Exist (exit_screen, gameFolders.szDataDir, 0))
		strcpy (exit_screen, gameStates.menus.bHires?"warningb.pcx":"warning.pcx"); // D1
	if (! CFile::Exist (exit_screen, gameFolders.szDataDir, 0))
		return; // D2 registered

	if ((pcx_error=PcxReadFullScrImage (exit_screen, 0))==PCX_ERROR_NONE) {
		paletteManager.FadeIn ();
		GrUpdate (0);
		while (!(KeyInKey () || MouseButtonState (0)))
			;
		paletteManager.FadeOut ();
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
