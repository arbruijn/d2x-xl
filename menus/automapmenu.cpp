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
	int	nOptTextured;
	int	nOptRadar;
	int	nOptRadarRange;
} automapOpts;

//------------------------------------------------------------------------------

static const char *pszRadarRange [3];

int AutomapOptionsCallback (CMenu& menu, int& key, int nCurItem)
{
	CMenuItem * m;
	int			v;

m = menu + automapOpts.nOptTextured;
v = m->m_value;
if (v != gameOpts->render.automap.bTextured) {
	gameOpts->render.automap.bTextured = v;
	key = -2;
	return nCurItem;
	}
if (!m [automapOpts.nOptRadar + extraGameInfo [0].nRadar].m_value) {
	key = -2;
	return nCurItem;
	}
if (automapOpts.nOptRadarRange >= 0) {
	m = menu + automapOpts.nOptRadarRange;
	v = m->m_value;
	if (v != gameOpts->render.automap.nRange) {
		gameOpts->render.automap.nRange = v;
		sprintf (m->m_text, TXT_RADAR_RANGE, pszRadarRange [v]);
		m->m_bRebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void AutomapOptionsMenu (void)
{
	CMenu	m;
	int	i, j, choice = 0;
	int	optBright, optGrayOut, optShowRobots, optShowPowerups, optCoronas, optSmoke, optLightnings, optColor, optSkybox, optSparks;
	char	szRadarRange [50];

pszRadarRange [0] = TXT_SHORT;
pszRadarRange [1] = TXT_MEDIUM;
pszRadarRange [2] = TXT_FAR;
*szRadarRange = '\0';
do {
	m.Destroy ();
	m.Create (30);
	automapOpts.nOptTextured = m.AddCheck (TXT_AUTOMAP_TEXTURED, gameOpts->render.automap.bTextured, KEY_T, HTX_AUTOMAP_TEXTURED);
	if (gameOpts->render.automap.bTextured) {
		optBright = m.AddCheck (TXT_AUTOMAP_BRIGHT, gameOpts->render.automap.bBright, KEY_B, HTX_AUTOMAP_BRIGHT);
		optGrayOut = m.AddCheck (TXT_AUTOMAP_GRAYOUT, gameOpts->render.automap.bGrayOut, KEY_Y, HTX_AUTOMAP_GRAYOUT);
		optCoronas = m.AddCheck (TXT_AUTOMAP_CORONAS, gameOpts->render.automap.bCoronas, KEY_C, HTX_AUTOMAP_CORONAS);
		optSparks = m.AddCheck (TXT_RENDER_SPARKS, gameOpts->render.automap.bSparks, KEY_P, HTX_RENDER_SPARKS);
		optSmoke = m.AddCheck (TXT_AUTOMAP_SMOKE, gameOpts->render.automap.bParticles, KEY_S, HTX_AUTOMAP_SMOKE);
		optLightnings = m.AddCheck (TXT_AUTOMAP_LIGHTNINGS, gameOpts->render.automap.bLightnings, KEY_S, HTX_AUTOMAP_LIGHTNINGS);
		optSkybox = m.AddCheck (TXT_AUTOMAP_SKYBOX, gameOpts->render.automap.bSkybox, KEY_K, HTX_AUTOMAP_SKYBOX);
		}
	else
		optGrayOut =
		optSmoke =
		optLightnings =
		optCoronas =
		optSkybox =
		optBright =
		optSparks = -1;
	optShowRobots = m.AddCheck (TXT_AUTOMAP_ROBOTS, extraGameInfo [0].bRobotsOnRadar, KEY_R, HTX_AUTOMAP_ROBOTS);
	optShowPowerups = m.AddRadio (TXT_AUTOMAP_NO_POWERUPS, 0, KEY_D, HTX_AUTOMAP_POWERUPS);
	m.AddRadio (TXT_AUTOMAP_POWERUPS, 0, KEY_P, HTX_AUTOMAP_POWERUPS);
	if (extraGameInfo [0].nRadar)
		m.AddRadio (TXT_RADAR_POWERUPS, 0, KEY_A, HTX_AUTOMAP_POWERUPS);
	m [optShowPowerups + extraGameInfo [0].bPowerupsOnRadar].m_value = 1;
	m.AddText ("", 0);
	automapOpts.nOptRadar = m.AddRadio (TXT_RADAR_OFF, 0, KEY_R, HTX_AUTOMAP_RADAR);
	m.AddRadio (TXT_RADAR_TOP, 0, KEY_T, HTX_AUTOMAP_RADAR);
	m.AddRadio (TXT_RADAR_BOTTOM, 0, KEY_O, HTX_AUTOMAP_RADAR);
	if (extraGameInfo [0].nRadar) {
		m.AddText ("", 0);
		sprintf (szRadarRange + 1, TXT_RADAR_RANGE, pszRadarRange [gameOpts->render.automap.nRange]);
		*szRadarRange = *(TXT_RADAR_RANGE - 1);
		automapOpts.nOptRadarRange = m.AddSlider (szRadarRange + 1, gameOpts->render.automap.nRange, 0, 2, KEY_A, HTX_RADAR_RANGE);
		m.AddText ("", 0);
		optColor = m.AddRadio (TXT_RADAR_WHITE, 0, KEY_W);
		m.AddRadio (TXT_RADAR_BLACK, 0, KEY_L);
		m [optColor + gameOpts->render.automap.nColor].m_value = 1;
		m [automapOpts.nOptRadar + extraGameInfo [0].nRadar].m_value = 1;
		}
	else
		automapOpts.nOptRadarRange =
		optColor = -1;
	for (;;) {
		i = m.Menu (NULL, TXT_AUTOMAP_MENUTITLE, AutomapOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	//gameOpts->render.automap.bTextured = m [automapOpts.nOptTextured].m_value;
	GET_VAL (gameOpts->render.automap.bBright, optBright);
	GET_VAL (gameOpts->render.automap.bGrayOut, optGrayOut);
	GET_VAL (gameOpts->render.automap.bCoronas, optCoronas);
	GET_VAL (gameOpts->render.automap.bSparks, optSparks);
	GET_VAL (gameOpts->render.automap.bParticles, optSmoke);
	GET_VAL (gameOpts->render.automap.bLightnings, optLightnings);
	GET_VAL (gameOpts->render.automap.bSkybox, optSkybox);
	if (automapOpts.nOptRadarRange >= 0)
		gameOpts->render.automap.nRange = m [automapOpts.nOptRadarRange].m_value;
	extraGameInfo [0].bPowerupsOnRadar = m [optShowPowerups].m_value;
	extraGameInfo [0].bRobotsOnRadar = m [optShowRobots].m_value;
	for (j = 0; j < 2 + extraGameInfo [0].nRadar; j++)
		if (m [optShowPowerups + j].m_value) {
			extraGameInfo [0].bPowerupsOnRadar = j;
			break;
			}
	for (j = 0; j < 3; j++)
		if (m [automapOpts.nOptRadar + j].m_value) {
			extraGameInfo [0].nRadar = j;
			break;
			}
	if (optColor >= 0) {
		for (j = 0; j < 2; j++)
			if (m [optColor + j].m_value) {
				gameOpts->render.automap.nColor = j;
				break;
				}
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------
//eof
