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

static int nOpt3D;

int PowerupOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem * m;
	int			v;

m = menus + nOpt3D;
v = m->value;
if (v != gameOpts->render.powerups.b3D) {
	if ((gameOpts->render.powerups.b3D = v))
		ConvertAllPowerupsToWeapons ();
	*key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void PowerupOptionsMenu (void)
{
	CMenuItem m [10];
	int	i, j, choice = 0;
	int	nOptions;
	int	optSpin;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	m.AddCheck (nOptions, TXT_3D_POWERUPS, gameOpts->render.powerups.b3D, KEY_D, HTX_3D_POWERUPS);
	nOpt3D = nOptions++;
	if (!gameOpts->render.powerups.b3D)
		optSpin = -1;
	else {
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddRadio (nOptions, TXT_SPIN_OFF, 0, KEY_O, 1, NULL);
		optSpin = nOptions++;
		m.AddRadio (nOptions, TXT_SPIN_SLOW, 0, KEY_S, 1, NULL);
		nOptions++;
		m.AddRadio (nOptions, TXT_SPIN_MEDIUM, 0, KEY_M, 1, NULL);
		nOptions++;
		m.AddRadio (nOptions, TXT_SPIN_FAST, 0, KEY_F, 1, NULL);
		nOptions++;
		m [optSpin + gameOpts->render.powerups.nSpin].value = 1;
		}
	for (;;) {
		i = ExecMenu1 (NULL, TXT_POWERUP_MENUTITLE, nOptions, m, PowerupOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (gameOpts->render.powerups.b3D && (optSpin >= 0))
		for (j = 0; j < 4; j++)
			if (m [optSpin + j].value) {
				gameOpts->render.powerups.nSpin = j;
				break;
			}
	} while (i == -2);
}

//------------------------------------------------------------------------------

//eof
