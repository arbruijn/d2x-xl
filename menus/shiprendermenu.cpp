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
	int	nWeapons;
	int	nColor;
} shipRenderOpts;

static int fpsTable [16] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 125, 150, 175, 200, 250};

//------------------------------------------------------------------------------

static const char *pszShipColors [8];

int ShipRenderOptionsCallback (CMenu& menu, int& key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menu + shipRenderOpts.nWeapons;
v = m->m_value;
if (v != extraGameInfo [0].bShowWeapons) {
	extraGameInfo [0].bShowWeapons = v;
	key = -2;
	}
if (shipRenderOpts.nColor >= 0) {
	m = menu + shipRenderOpts.nColor;
	v = m->m_value;
	if (v != gameOpts->render.ship.nColor) {
		gameOpts->render.ship.nColor = v;
		sprintf (m->m_text, TXT_SHIPCOLOR, pszShipColors [v]);
		m->m_bRebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void ShipRenderOptionsMenu (void)
{
	CMenu m;
	int	i, j, choice = 0;
	int	optBullets, optWingtips;
	char	szShipColor [50];

pszShipColors [0] = TXT_SHIP_WHITE;
pszShipColors [1] = TXT_SHIP_BLUE;
pszShipColors [2] = TXT_RED;
pszShipColors [3] = TXT_SHIP_GREEN;
pszShipColors [4] = TXT_SHIP_YELLOW;
pszShipColors [5] = TXT_SHIP_PURPLE;
pszShipColors [6] = TXT_SHIP_ORANGE;
pszShipColors [7] = TXT_SHIP_CYAN;
do {
	m.Destroy ();
	m.Create (10);
	shipRenderOpts.nWeapons = m.AddCheck (TXT_SHIP_WEAPONS, extraGameInfo [0].bShowWeapons, KEY_W, HTX_SHIP_WEAPONS);
	if (extraGameInfo [0].bShowWeapons) {
		optBullets = m.AddCheck (TXT_SHIP_BULLETS, gameOpts->render.ship.bBullets, KEY_B, HTX_SHIP_BULLETS);
		m.AddText ("", 0);
		optWingtips = m.AddRadio (TXT_SHIP_WINGTIP_LASER, 0, KEY_A, 1, HTX_SHIP_WINGTIPS);
		m.AddRadio (TXT_SHIP_WINGTIP_SUPLAS, 0, KEY_U, 1, HTX_SHIP_WINGTIPS);
		m.AddRadio (TXT_SHIP_WINGTIP_SHORT, 0, KEY_S, 1, HTX_SHIP_WINGTIPS);
		m.AddRadio (TXT_SHIP_WINGTIP_LONG, 0, KEY_L, 1, HTX_SHIP_WINGTIPS);
		m [optWingtips + gameOpts->render.ship.nWingtip].m_value = 1;
		m.AddText ("", 0);
		m.AddText (TXT_SHIPCOLOR_HEADER, 0);
		sprintf (szShipColor + 1, TXT_SHIPCOLOR, pszShipColors [gameOpts->render.ship.nColor]);
		*szShipColor = 0;
		shipRenderOpts.nColor = m.AddSlider (szShipColor + 1, gameOpts->render.ship.nColor, 0, 7, KEY_C, HTX_SHIPCOLOR);
		}
	else
		optBullets =
		optWingtips =
		shipRenderOpts.nColor = -1;
	for (;;) {
		i = m.Menu (NULL, TXT_SHIP_RENDERMENU, ShipRenderOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if ((extraGameInfo [0].bShowWeapons = m [shipRenderOpts.nWeapons].m_value)) {
		gameOpts->render.ship.bBullets = m [optBullets].m_value;
		for (j = 0; j < 4; j++)
			if (m [optWingtips + j].m_value) {
				gameOpts->render.ship.nWingtip = j;
				break;
				}
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------
//eof
