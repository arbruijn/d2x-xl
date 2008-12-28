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
	int	nUse;
	int	nSpeed;
	int	nFPS;
} camOpts;

//------------------------------------------------------------------------------

int CameraOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + camOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseCameras) {
	extraGameInfo [0].bUseCameras = v;
	*key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bUseCameras) {
	if (camOpts.nFPS >= 0) {
		m = menus + camOpts.nFPS;
		v = m->value * 5;
		if (gameOpts->render.cameras.nFPS != v) {
			gameOpts->render.cameras.nFPS = v;
			sprintf (m->text, TXT_CAM_REFRESH, gameOpts->render.cameras.nFPS);
			m->rebuild = 1;
			}
		}
	if (gameOpts->app.bExpertMode && (camOpts.nSpeed >= 0)) {
		m = menus + camOpts.nSpeed;
		v = (m->value + 1) * 1000;
		if (gameOpts->render.cameras.nSpeed != v) {
			gameOpts->render.cameras.nSpeed = v;
			sprintf (m->text, TXT_CAM_SPEED, v / 1000);
			m->rebuild = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void CameraOptionsMenu (void)
{
	CMenuItem m [10];
	int	i, choice = 0;
	int	nOptions;
	int	bFSCameras = gameOpts->render.cameras.bFitToWall;
	int	optFSCameras, optTeleCams, optHiresCams;
#if 0
	int checks;
#endif

	char szCameraFps [50];
	char szCameraSpeed [50];

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	m.AddCheck (nOptions, TXT_USE_CAMS, extraGameInfo [0].bUseCameras, KEY_C, HTX_ADVRND_USECAMS);
	camOpts.nUse = nOptions++;
	if (extraGameInfo [0].bUseCameras && gameOpts->app.bExpertMode) {
		if (gameStates.app.bGameRunning) 
			optHiresCams = -1;
		else {
			m.AddCheck (nOptions, TXT_HIRES_CAMERAS, gameOpts->render.cameras.bHires, KEY_H, HTX_HIRES_CAMERAS);
			optHiresCams = nOptions++;
			}
		m.AddCheck (nOptions, TXT_TELEPORTER_CAMS, extraGameInfo [0].bTeleporterCams, KEY_U, HTX_TELEPORTER_CAMS);
		optTeleCams = nOptions++;
		m.AddCheck (nOptions, TXT_ADJUST_CAMS, gameOpts->render.cameras.bFitToWall, KEY_A, HTX_ADVRND_ADJUSTCAMS);
		optFSCameras = nOptions++;
		sprintf (szCameraFps + 1, TXT_CAM_REFRESH, gameOpts->render.cameras.nFPS);
		*szCameraFps = *(TXT_CAM_REFRESH - 1);
		m.AddSlider (nOptions, szCameraFps + 1, gameOpts->render.cameras.nFPS / 5, 0, 6, KEY_A, HTX_ADVRND_CAMREFRESH);
		camOpts.nFPS = nOptions++;
		sprintf (szCameraSpeed + 1, TXT_CAM_SPEED, gameOpts->render.cameras.nSpeed / 1000);
		*szCameraSpeed = *(TXT_CAM_SPEED - 1);
		m.AddSlider (nOptions, szCameraSpeed + 1, (gameOpts->render.cameras.nSpeed / 1000) - 1, 0, 9, KEY_D, HTX_ADVRND_CAMSPEED);
		camOpts.nSpeed = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		}
	else {
		optHiresCams = 
		optTeleCams = 
		optFSCameras =
		camOpts.nFPS =
		camOpts.nSpeed = -1;
		}

	do {
		i = ExecMenu1 (NULL, TXT_CAMERA_MENUTITLE, nOptions, m, &CameraOptionsCallback, &choice);
	} while (i >= 0);

	if ((extraGameInfo [0].bUseCameras = m [camOpts.nUse].value)) {
		GET_VAL (extraGameInfo [0].bTeleporterCams, optTeleCams);
		GET_VAL (gameOpts->render.cameras.bFitToWall, optFSCameras);
		if (!gameStates.app.bGameRunning)
			GET_VAL (gameOpts->render.cameras.bHires, optHiresCams);
		}
	if (bFSCameras != gameOpts->render.cameras.bFitToWall) {
		cameraManager.Destroy ();
		cameraManager.Create ();
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------
//eof
