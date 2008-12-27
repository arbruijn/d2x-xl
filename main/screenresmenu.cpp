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
#include "newmenu.h"
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

//------------------------------------------------------------------------------

static struct {
	int	nWideScreen;
	int	nCustom;
} screenResOpts;

static int nDisplayMode;

//------------------------------------------------------------------------------

static int ScreenResMenuItemToMode (int menuItem)
{
	int j;

if ((screenResOpts.nWideScreen >= 0) && (menuItem > screenResOpts.nWideScreen))
	menuItem--;
for (j = 0; j < NUM_DISPLAY_MODES; j++)
	if (displayModeInfo [j].isAvailable)
		if (--menuItem < 0)
			break;
return j;
}

//------------------------------------------------------------------------------

static int ScreenResModeToMenuItem (int mode)
{
	int item = 0;

for (int j = 0; j < mode; j++)
	if (displayModeInfo [j].isAvailable)
		item++;
if ((screenResOpts.nWideScreen >= 0) && (mode >= screenResOpts.nWideScreen))
	item++;
return item;
}

//------------------------------------------------------------------------------

int ScreenResCallback (CMenu& m, int *nLastKey, int nCurItem)
{
	int	i, j;

if (m [screenResOpts.nCustom].value != (nDisplayMode == NUM_DISPLAY_MODES)) 
	*nLastKey = -2;
for (i = 0; i < screenResOpts.nCustom; i++)
	if (m [i].value) {
		j = ScreenResMenuItemToMode(i);
		if ((j < NUM_DISPLAY_MODES) && (j != nDisplayMode)) {
			SetDisplayMode (j, 0);
			nDisplayMode = gameStates.video.nDisplayMode;
			*nLastKey = -2;
			}
		break;
		}
return nCurItem;
}

//------------------------------------------------------------------------------

int SwitchDisplayMode (int dir)
{
	int	i, h = NUM_DISPLAY_MODES;

for (i = 0; i < h; i++)
	displayModeInfo [i].isAvailable =
		 ((i < 2) || gameStates.menus.bHiresAvailable) && GrVideoModeOK (displayModeInfo [i].VGA_mode);
i = gameStates.video.nDisplayMode;
do {
	i += dir;
	if (i < 0)
		i = 0;
	else if (i >= h)
		i = h - 1;
	if (displayModeInfo [i].isAvailable) {
		SetDisplayMode (i, 0);
		return 1;
		}
	} while (i && (i < h - 1) && (i != gameStates.video.nDisplayMode));
return 0;
}

//------------------------------------------------------------------------------

#if VR_NONE
#   undef VR_NONE			//undef if != 0
#endif
#ifndef VR_NONE
#   define VR_NONE 0		//make sure VR_NONE is defined and 0 here
#endif

#define ADD_RES_OPT(_t) {m.AddRadio (nOptions, _t, 0, -1, 0, NULL); nOptions++;}
//{m [nOptions].nType = NM_TYPE_RADIO; m [nOptions].text = (_t); m [nOptions].key = -1; m [nOptions].value = 0; nOptions++;}

void ScreenResMenu (void)
{
#	define N_SCREENRES_ITEMS (NUM_DISPLAY_MODES + 4)

	CMenu		m (N_SCREENRES_ITEMS);
	int		choice;
	int		i, j, key, nOptions = 0, nCustWOpt, nCustHOpt, nCustW, nCustH, bStdRes;
	char		szMode [NUM_DISPLAY_MODES][20];
	char		cShortCut, szCustX [5], szCustY [5];

if ((gameStates.video.nDisplayMode == -1) || (gameStates.render.vr.nRenderMode != VR_NONE)) {				//special VR mode
	ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, 
			"You may not change screen\nresolution when VR modes enabled.");
	return;
	}
nDisplayMode = gameStates.video.nDisplayMode;
do {
	screenResOpts.nWideScreen = -1;
	cShortCut = '1';
	nOptions = 0;
	memset (m, 0, sizeof (m));
	for (i = 0, j = NUM_DISPLAY_MODES; i < j; i++) {
		if (!(displayModeInfo [i].isAvailable = 
				 ((i < 2) || gameStates.menus.bHiresAvailable) && GrVideoModeOK (displayModeInfo [i].VGA_mode)))
				continue;
		if (displayModeInfo [i].isWideScreen && !displayModeInfo [i-1].isWideScreen) {
			m.AddText (nOptions, TXT_WIDESCREEN_RES, 0);
			if (screenResOpts.nWideScreen < 0)
				screenResOpts.nWideScreen = nOptions;
			nOptions++;
			}
		sprintf (szMode [i], "%c. %dx%d", cShortCut, displayModeInfo [i].w, displayModeInfo [i].h);
		if (cShortCut == '9')
			cShortCut = 'A';
		else
			cShortCut++;
		ADD_RES_OPT (szMode [i]);
		}
	m.AddRadio (nOptions, TXT_CUSTOM_SCRRES, 0, KEY_U, 0, HTX_CUSTOM_SCRRES);
	screenResOpts.nCustom = nOptions++;
	*szCustX = *szCustY = '\0';
	if (displayModeInfo [NUM_DISPLAY_MODES].w)
		sprintf (szCustX, "%d", displayModeInfo [NUM_DISPLAY_MODES].w);
	else
		*szCustX = '\0';
	if (displayModeInfo [NUM_DISPLAY_MODES].h)
		sprintf (szCustY, "%d", displayModeInfo [NUM_DISPLAY_MODES].h);
	else
		*szCustY = '\0';
	//if (nDisplayMode == NUM_DISPLAY_MODES) 
	 {
		m.AddInput (nOptions, szCustX, 4, NULL);
		nCustWOpt = nOptions++;
		m.AddInput (nOptions, szCustY, 4, NULL);
		nCustHOpt = nOptions++;
		}
/*
	else
		nCustWOpt =
		nCustHOpt = -1;
*/
	choice = ScreenResModeToMenuItem(nDisplayMode);
	m [choice].value = 1;

	key = ExecMenu1 (NULL, TXT_SELECT_SCRMODE, nOptions, m, ScreenResCallback, &choice);
	if (key == -1)
		return;
	bStdRes = 0;
	if (m [screenResOpts.nCustom].value) {
		key = -2;
		nDisplayMode = NUM_DISPLAY_MODES;
		if ((nCustWOpt > 0) && (nCustHOpt > 0) &&
			 (0 < (nCustW = atoi (szCustX))) && (0 < (nCustH = atoi (szCustY)))) {
			i = NUM_DISPLAY_MODES;
			if (SetCustomDisplayMode (nCustW, nCustH))
				key = 0;
			else
				ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_ERROR_SCRMODE);
			}
		else
			continue;
		}
	else {
		for (i = 0; i <= screenResOpts.nCustom; i++)
			if (m [i].value) {
				bStdRes = 1;
				i = ScreenResMenuItemToMode(i);
				break;
				}
			if (!bStdRes)
				continue;
		}
	if (((i > 1) && !gameStates.menus.bHiresAvailable) || !GrVideoModeOK (displayModeInfo [i].VGA_mode)) {
		ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_ERROR_SCRMODE);
		return;
		}
	if (i == gameStates.video.nDisplayMode) {
		SetDisplayMode (i, 0);
		SetScreenMode (SCREEN_MENU);
		if (bStdRes)
			return;
		}
	}
	while (key == -2);
}

//------------------------------------------------------------------------------
//eof
