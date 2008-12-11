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
#include "gameargs.h"

// ----------------------------------------------------------------------------
//read help from a file & print to screen
#define LINE_LEN	100

void PrintCmdLineHelp (void)
{
	CFile cf;
	int bHaveBinary = 0;
	char line[LINE_LEN];

	if (!(cf.Open ("help.tex", gameFolders.szDataDir, "rb", 0) || 
			cf.Open ("help.txb", gameFolders.szDataDir, "rb", 0))) {
			Warning (TXT_NO_HELP);
		bHaveBinary = 1;
	}
	if (cf.File ()) {
		while (cf.GetS (line, LINE_LEN)) {
			if (bHaveBinary) {
				int i;
				for (i = 0; i < (int) strlen (line) - 1; i++) {
					line[i] = EncodeRotateLeft ((char) (EncodeRotateLeft (line [i]) ^ BITMAP_TBL_XOR));
				}
			}
			if (line[0] == ';')
				continue;		//don't show comments
		}
		cf.Close ();
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
	con_printf ((int) con_threshold.value, "  -legacyrender     %s\n", "Turn off colored CSegment rendering");
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
	gameOptions [1].render.particles.bAuxViews = 0;
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
	gameOptions [1].render.particles.nDens [0] =
	gameOptions [1].render.particles.nDens [1] =
	gameOptions [1].render.particles.nDens [2] =
	gameOptions [1].render.particles.nDens [3] =
	gameOptions [1].render.particles.nDens [4] = 0;
	gameOptions [1].render.particles.nSize [0] =
	gameOptions [1].render.particles.nSize [1] =
	gameOptions [1].render.particles.nSize [2] =
	gameOptions [1].render.particles.nSize [3] =
	gameOptions [1].render.particles.nSize [4] = 0;
	gameOptions [1].render.particles.nLife [0] =
	gameOptions [1].render.particles.nLife [1] =
	gameOptions [1].render.particles.nLife [2] = 0;
	gameOptions [1].render.particles.nLife [3] = 1;
	gameOptions [1].render.particles.nLife [4] = 0;
	gameOptions [1].render.particles.nAlpha [0] =
	gameOptions [1].render.particles.nAlpha [1] =
	gameOptions [1].render.particles.nAlpha [2] =
	gameOptions [1].render.particles.nAlpha [3] =
	gameOptions [1].render.particles.nAlpha [4] = 0;
	gameOptions [1].render.particles.bPlayers = 0;
	gameOptions [1].render.particles.bRobots = 0;
	gameOptions [1].render.particles.bMissiles = 0;
	gameOptions [1].render.particles.bDebris = 0;
	gameOptions [1].render.particles.bStatic = 0;
	gameOptions [1].render.particles.bBubbles = 0;
	gameOptions [1].render.particles.bWobbleBubbles = 1;
	gameOptions [1].render.particles.bWiggleBubbles = 1;
	gameOptions [1].render.particles.bCollisions = 0;
	gameOptions [1].render.particles.bDisperse = 0;
	gameOptions [1].render.particles.bRotate = 0;
	gameOptions [1].render.particles.bSort = 0;
	gameOptions [1].render.particles.bDecreaseLag = 0;
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
	gameOptions [1].render.automap.bParticles = 0;
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
	gameOptions [0].render.particles.bAuxViews = 0;
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
	gameOptions [0].render.particles.nDens [0] =
	gameOptions [0].render.particles.nDens [0] =
	gameOptions [0].render.particles.nDens [2] =
	gameOptions [0].render.particles.nDens [3] =
	gameOptions [0].render.particles.nDens [4] = 2;
	gameOptions [0].render.particles.nSize [0] =
	gameOptions [0].render.particles.nSize [0] =
	gameOptions [0].render.particles.nSize [2] =
	gameOptions [0].render.particles.nSize [3] =
	gameOptions [0].render.particles.nSize [4] = 1;
	gameOptions [0].render.particles.nLife [0] =
	gameOptions [0].render.particles.nLife [0] =
	gameOptions [0].render.particles.nLife [2] = 0;
	gameOptions [0].render.particles.nLife [3] = 1;
	gameOptions [0].render.particles.nLife [4] = 0;
	gameOptions [0].render.particles.nAlpha [0] =
	gameOptions [0].render.particles.nAlpha [0] =
	gameOptions [0].render.particles.nAlpha [2] =
	gameOptions [0].render.particles.nAlpha [3] =
	gameOptions [0].render.particles.nAlpha [4] = 0;
	gameOptions [0].render.particles.bPlayers = 1;
	gameOptions [0].render.particles.bRobots = 1;
	gameOptions [0].render.particles.bMissiles = 1;
	gameOptions [0].render.particles.bDebris = 1;
	gameOptions [0].render.particles.bStatic = 1;
	gameOptions [0].render.particles.bBubbles = 1;
	gameOptions [0].render.particles.bWobbleBubbles = 1;
	gameOptions [0].render.particles.bWiggleBubbles = 1;
	gameOptions [0].render.particles.bCollisions = 0;
	gameOptions [0].render.particles.bDisperse = 1;
	gameOptions [0].render.particles.bRotate = 1;
	gameOptions [0].render.particles.bSort = 1;
	gameOptions [0].render.particles.bDecreaseLag = 1;
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
	gameOptions [0].render.automap.bParticles = 0;
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
