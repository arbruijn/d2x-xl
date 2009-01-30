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

//------------------------------------------------------------------------------

static struct {
	int	nUse;
	int	nSpeed;
	int	nFPS;
} camOpts;

//------------------------------------------------------------------------------

int CameraOptionsCallback (CMenu& menu, int& key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menu + camOpts.nUse;
v = m->m_value;
if (v != extraGameInfo [0].bUseCameras) {
	extraGameInfo [0].bUseCameras = v;
	key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bUseCameras) {
	if (camOpts.nFPS >= 0) {
		m = menu + camOpts.nFPS;
		v = m->m_value * 5;
		if (gameOpts->render.cameras.nFPS != v) {
			gameOpts->render.cameras.nFPS = v;
			sprintf (m->m_text, TXT_CAM_REFRESH, gameOpts->render.cameras.nFPS);
			m->m_bRebuild = 1;
			}
		}
	if (gameOpts->app.bExpertMode && (camOpts.nSpeed >= 0)) {
		m = menu + camOpts.nSpeed;
		v = (m->m_value + 1) * 1000;
		if (gameOpts->render.cameras.nSpeed != v) {
			gameOpts->render.cameras.nSpeed = v;
			sprintf (m->m_text, TXT_CAM_SPEED, v / 1000);
			m->m_bRebuild = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void CameraOptionsMenu (void)
{
	CMenu	m;
	int	i, choice = 0;
	int	bFSCameras = gameOpts->render.cameras.bFitToWall;
	int	optFSCameras, optTeleCams, optHiresCams;
#if 0
	int checks;
#endif

	char szCameraFps [50];
	char szCameraSpeed [50];

do {
	m.Destroy ();
	m.Create (10);
	camOpts.nUse = m.AddCheck (TXT_USE_CAMS, extraGameInfo [0].bUseCameras, KEY_C, HTX_ADVRND_USECAMS);
	if (extraGameInfo [0].bUseCameras && gameOpts->app.bExpertMode) {
		if (gameStates.app.bGameRunning) 
			optHiresCams = -1;
		else
			optHiresCams = m.AddCheck (TXT_HIRES_CAMERAS, gameOpts->render.cameras.bHires, KEY_H, HTX_HIRES_CAMERAS);
		optTeleCams = m.AddCheck (TXT_TELEPORTER_CAMS, extraGameInfo [0].bTeleporterCams, KEY_U, HTX_TELEPORTER_CAMS);
		optFSCameras = m.AddCheck (TXT_ADJUST_CAMS, gameOpts->render.cameras.bFitToWall, KEY_A, HTX_ADVRND_ADJUSTCAMS);
		sprintf (szCameraFps + 1, TXT_CAM_REFRESH, gameOpts->render.cameras.nFPS);
		*szCameraFps = *(TXT_CAM_REFRESH - 1);
		camOpts.nFPS = m.AddSlider (szCameraFps + 1, gameOpts->render.cameras.nFPS / 5, 0, 6, KEY_A, HTX_ADVRND_CAMREFRESH);
		sprintf (szCameraSpeed + 1, TXT_CAM_SPEED, gameOpts->render.cameras.nSpeed / 1000);
		*szCameraSpeed = *(TXT_CAM_SPEED - 1);
		camOpts.nSpeed = m.AddSlider (szCameraSpeed + 1, (gameOpts->render.cameras.nSpeed / 1000) - 1, 0, 9, KEY_D, HTX_ADVRND_CAMSPEED);
		m.AddText ("", 0);
		}
	else {
		optHiresCams = 
		optTeleCams = 
		optFSCameras =
		camOpts.nFPS =
		camOpts.nSpeed = -1;
		}

	do {
		i = m.Menu (NULL, TXT_CAMERA_MENUTITLE, &CameraOptionsCallback, &choice);
	} while (i >= 0);

	if ((extraGameInfo [0].bUseCameras = m [camOpts.nUse].m_value)) {
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
