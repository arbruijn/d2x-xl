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

void DefaultEffectSettings (void);

//------------------------------------------------------------------------------

static struct {
	int	nExplShrapnels;
	int	nLightTrails;
	int	nSparks;
	int	nThrusters;
} effectOpts;

//------------------------------------------------------------------------------

static int nLightTrails;

static const char* pszExplShrapnels [5];
static const char* pszLightTrails [3];
static const char* pszThrusters [3];

int EffectOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v;

if (effectOpts.nExplShrapnels >= 0) {
	m = menu + effectOpts.nExplShrapnels;
	v = m->m_value;
	if (gameOpts->render.effects.nShrapnels != v) {
		gameOpts->render.effects.nShrapnels = v;
		sprintf (m->m_text, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [v]);
		m->m_bRebuild = -1;
		}
	}

m = menu + effectOpts.nThrusters;
v = m->m_value;
if (extraGameInfo [0].bThrusterFlames != v) {
	extraGameInfo [0].bThrusterFlames = v;
	sprintf (m->m_text, TXT_THRUSTER_FLAMES, pszThrusters [v]);
	m->m_bRebuild = 1;
	}

m = menu + effectOpts.nLightTrails;
v = m->m_value;
if (nLightTrails != v) {
	nLightTrails = v;
	key = -2;
	}

return nCurItem;
}

//------------------------------------------------------------------------------

void EffectOptionsMenu (void)
{
	static int choice = 0;

	CMenu	m;
	int	i, j;
	int	optGatlingTrails, optStaticSmoke, optSoftParticles [3];
#if 0
	int	optShockwaves;
#endif
	int	bEnergySparks = gameOpts->render.effects.bEnergySparks;
	char	szSlider [50];

pszExplShrapnels [0] = TXT_NONE;
pszExplShrapnels [1] = TXT_FEW;
pszExplShrapnels [2] = TXT_MEDIUM;
pszExplShrapnels [3] = TXT_MANY;
pszExplShrapnels [4] = TXT_EXTREME;

pszLightTrails [0] = TXT_NONE;
pszLightTrails [1] = TXT_BASIC;
pszLightTrails [2] = TXT_ADVANCED;

pszThrusters [0] = TXT_NONE;
pszThrusters [1] = TXT_2D;
pszThrusters [2] = TXT_3D;

nLightTrails = extraGameInfo [0].bLightTrails ? gameOpts->render.particles.bPlasmaTrails ? 2 : 1 : 0;
do {
	m.Destroy ();
	m.Create (30);

	if (extraGameInfo [0].bUseParticles) {
		sprintf (szSlider + 1, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [gameOpts->render.effects.nShrapnels]);
		*szSlider = *(TXT_EXPLOSION_SHRAPNELS - 1);
		effectOpts.nExplShrapnels = m.AddSlider (szSlider + 1, gameOpts->render.effects.nShrapnels, 0, 4, KEY_P, HTX_EXPLOSION_SHRAPNELS);
		}
	else
		effectOpts.nExplShrapnels = -1;

	sprintf (szSlider + 1, TXT_LIGHTTRAIL_QUAL, pszLightTrails [nLightTrails]);
	*szSlider = *(TXT_LIGHTTRAIL_QUAL - 1);
	effectOpts.nLightTrails = m.AddSlider (szSlider + 1, nLightTrails, 0, 1 + extraGameInfo [0].bUseParticles, KEY_P, HTX_LIGHTTRAIL_QUAL);

	sprintf (szSlider + 1, TXT_THRUSTER_FLAMES, pszThrusters [extraGameInfo [0].bThrusterFlames]);
	*szSlider = *(TXT_THRUSTER_FLAMES - 1);
	effectOpts.nThrusters = m.AddSlider (szSlider + 1, extraGameInfo [0].bThrusterFlames, 0, 2, KEY_T, HTX_THRUSTER_FLAMES);

	m.AddText ("");
	if (extraGameInfo [0].bUseParticles) {
		optStaticSmoke = m.AddCheck (TXT_SMOKE_STATIC, gameOpts->render.particles.bStatic, KEY_S, HTX_ADVRND_STATICSMOKE);
		optGatlingTrails = m.AddCheck (TXT_GATLING_TRAILS, extraGameInfo [0].bGatlingTrails, KEY_G, HTX_GATLING_TRAILS);
		}
	else
		optStaticSmoke =
		optGatlingTrails = -1;
	optSoftParticles [0] = m.AddCheck (TXT_SOFT_SPRITES, (gameOpts->render.effects.bSoftParticles & 1) != 0, KEY_I, HTX_SOFT_SPRITES);
	optSoftParticles [1] = m.AddCheck (TXT_SOFT_SPARKS, (gameOpts->render.effects.bSoftParticles & 2) != 0, KEY_A, HTX_SOFT_SPARKS);
	if (extraGameInfo [0].bUseParticles)
		optSoftParticles [2] = m.AddCheck (TXT_SOFT_SMOKE, (gameOpts->render.effects.bSoftParticles & 4) != 0, KEY_O, HTX_SOFT_SMOKE);
	else
		optSoftParticles [2] = -1;
	for (;;) {
		i = m.Menu (NULL, TXT_EFFECT_MENUTITLE, EffectOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	for (j = 0; j < 3; j++) {
		if (optSoftParticles [j] >= 0) {
			if (m [optSoftParticles [j]].m_value)
				gameOpts->render.effects.bSoftParticles |= 1 << j;
			else
				gameOpts->render.effects.bSoftParticles &= ~(1 << j);
			}
		}
	GET_VAL (gameOpts->render.particles.bStatic, optStaticSmoke);
	GET_VAL (extraGameInfo [0].bGatlingTrails, optGatlingTrails);
	if ((extraGameInfo [0].bLightTrails = (nLightTrails != 0)))
		gameOpts->render.particles.bPlasmaTrails = (nLightTrails == 2);
	} while (i == -2);

SetDebrisCollisions ();
DefaultEffectSettings ();
if (gameOpts->render.effects.bEnergySparks && gameStates.app.bGameRunning && !sparkManager.HaveSparks ())
	sparkManager.Setup ();
}

//------------------------------------------------------------------------------
//eof
