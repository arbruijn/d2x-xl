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

//------------------------------------------------------------------------------

void MovieOptionsMenu (void)
{
	CMenu m;
	int	i, choice = 0;
	int	optMovieQual, optMovieSize, optSubTitles;

do {
	m.Destroy ();
	m.Create (5);
	optSubTitles = m.AddCheck (TXT_MOVIE_SUBTTL, gameOpts->movies.bSubTitles, KEY_O, HTX_RENDER_SUBTTL);
	if (gameOpts->app.bExpertMode) {
		optMovieQual = m.AddCheck (TXT_MOVIE_QUAL, gameOpts->movies.nQuality, KEY_Q, HTX_RENDER_MOVIEQUAL);
		optMovieSize = m.AddCheck (TXT_MOVIE_FULLSCR, gameOpts->movies.bResize, KEY_U, HTX_RENDER_MOVIEFULL);
		}
	else
		optMovieQual = 
		optMovieSize = -1;

	for (;;) {
		i = m.Menu (NULL, TXT_MOVIE_OPTIONS, NULL, &choice);
		if (i < 0)
			break;
		} 
	gameOpts->movies.bSubTitles = m [optSubTitles].m_value;
	if (gameOpts->app.bExpertMode) {
		gameOpts->movies.nQuality = m [optMovieQual].m_value;
		gameOpts->movies.bResize = m [optMovieSize].m_value;
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------
//eof
