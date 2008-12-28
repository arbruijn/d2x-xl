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
	int	nQuality;
	int	nStyle;
	int	nOmega;
} lightningOpts;

//------------------------------------------------------------------------------

static const char* pszLightningQuality [2];
static const char* pszLightningStyle [3];

int LightningOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + lightningOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseLightnings) {
	if (!(extraGameInfo [0].bUseLightnings = v))
		lightningManager.Shutdown (0);
	*key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bUseLightnings) {
	m = menus + lightningOpts.nQuality;
	v = m->value;
	if (gameOpts->render.lightnings.nQuality != v) {
		gameOpts->render.lightnings.nQuality = v;
		sprintf (m->text, TXT_LIGHTNING_QUALITY, pszLightningQuality [v]);
		m->rebuild = 1;
		lightningManager.Shutdown (0);
		}
	m = menus + lightningOpts.nStyle;
	v = m->value;
	if (gameOpts->render.lightnings.nStyle != v) {
		gameOpts->render.lightnings.nStyle = v;
		sprintf (m->text, TXT_LIGHTNING_STYLE, pszLightningStyle [v]);
		m->rebuild = 1;
		}
	m = menus + lightningOpts.nOmega;
	v = m->value;
	if (gameOpts->render.lightnings.bOmega != v) {
		gameOpts->render.lightnings.bOmega = v;
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void LightningOptionsMenu (void)
{
	CMenuItem m [15];
	int	i, choice = 0;
	int	nOptions;
	int	optDamage, optExplosions, optPlayers, optRobots, optStatic, optRobotOmega, optPlasma, optAuxViews, optMonitors;
	char	szQuality [50], szStyle [100];

	pszLightningQuality [0] = TXT_LOW;
	pszLightningQuality [1] = TXT_HIGH;
	pszLightningStyle [0] = TXT_LIGHTNING_ERRATIC;
	pszLightningStyle [1] = TXT_LIGHTNING_JAGGY;
	pszLightningStyle [2] = TXT_LIGHTNING_SMOOTH;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	lightningOpts.nQuality = 
	optDamage = 
	optExplosions = 
	optPlayers = 
	optRobots = 
	optStatic = 
	optRobotOmega = 
	optPlasma = 
	optAuxViews = 
	optMonitors = -1;

	m.AddCheck (nOptions, TXT_LIGHTNING_ENABLE, extraGameInfo [0].bUseLightnings, KEY_U, HTX_LIGHTNING_ENABLE);
	lightningOpts.nUse = nOptions++;
	if (extraGameInfo [0].bUseLightnings) {
		sprintf (szQuality + 1, TXT_LIGHTNING_QUALITY, pszLightningQuality [gameOpts->render.lightnings.nQuality]);
		*szQuality = *(TXT_LIGHTNING_QUALITY - 1);
		m.AddSlider (nOptions, szQuality + 1, gameOpts->render.lightnings.nQuality, 0, 1, KEY_R, HTX_LIGHTNING_QUALITY);
		lightningOpts.nQuality = nOptions++;
		sprintf (szStyle + 1, TXT_LIGHTNING_STYLE, pszLightningStyle [gameOpts->render.lightnings.nStyle]);
		*szStyle = *(TXT_LIGHTNING_STYLE - 1);
		m.AddSlider (nOptions, szStyle + 1, gameOpts->render.lightnings.nStyle, 0, 2, KEY_S, HTX_LIGHTNING_STYLE);
		lightningOpts.nStyle = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_PLASMA, gameOpts->render.lightnings.bPlasma, KEY_L, HTX_LIGHTNING_PLASMA);
		optPlasma = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_DAMAGE, gameOpts->render.lightnings.bDamage, KEY_D, HTX_LIGHTNING_DAMAGE);
		optDamage = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_EXPLOSIONS, gameOpts->render.lightnings.bExplosions, KEY_E, HTX_LIGHTNING_EXPLOSIONS);
		optExplosions = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_PLAYERS, gameOpts->render.lightnings.bPlayers, KEY_P, HTX_LIGHTNING_PLAYERS);
		optPlayers = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_ROBOTS, gameOpts->render.lightnings.bRobots, KEY_R, HTX_LIGHTNING_ROBOTS);
		optRobots = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_STATIC, gameOpts->render.lightnings.bStatic, KEY_T, HTX_LIGHTNING_STATIC);
		optStatic = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_OMEGA, gameOpts->render.lightnings.bOmega, KEY_O, HTX_LIGHTNING_OMEGA);
		lightningOpts.nOmega = nOptions++;
		if (gameOpts->render.lightnings.bOmega) {
			m.AddCheck (nOptions, TXT_LIGHTNING_ROBOT_OMEGA, gameOpts->render.lightnings.bRobotOmega, KEY_B, HTX_LIGHTNING_ROBOT_OMEGA);
			optRobotOmega = nOptions++;
			}
		m.AddCheck (nOptions, TXT_LIGHTNING_AUXVIEWS, gameOpts->render.lightnings.bAuxViews, KEY_D, HTX_LIGHTNING_AUXVIEWS);
		optAuxViews = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_MONITORS, gameOpts->render.lightnings.bMonitors, KEY_M, HTX_LIGHTNING_MONITORS);
		optMonitors = nOptions++;
		}
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_LIGHTNING_MENUTITLE, nOptions, m, &LightningOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (extraGameInfo [0].bUseLightnings) {
		GET_VAL (gameOpts->render.lightnings.bPlasma, optPlasma);
		GET_VAL (gameOpts->render.lightnings.bDamage, optDamage);
		GET_VAL (gameOpts->render.lightnings.bExplosions, optExplosions);
		GET_VAL (gameOpts->render.lightnings.bPlayers, optPlayers);
		GET_VAL (gameOpts->render.lightnings.bRobots, optRobots);
		GET_VAL (gameOpts->render.lightnings.bStatic, optStatic);
		GET_VAL (gameOpts->render.lightnings.bRobotOmega, optRobotOmega);
		GET_VAL (gameOpts->render.lightnings.bAuxViews, optAuxViews);
		GET_VAL (gameOpts->render.lightnings.bMonitors, optMonitors);
		}
	} while (i == -2);
if (!gameOpts->render.lightnings.bPlayers)
	lightningManager.DestroyForPlayers ();
if (!gameOpts->render.lightnings.bRobots)
	lightningManager.DestroyForRobots ();
if (!gameOpts->render.lightnings.bStatic)
	lightningManager.DestroyStatic ();
}

//------------------------------------------------------------------------------
//eof
