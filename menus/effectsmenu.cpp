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
#include "playerprofile.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "savegame.h"
#include "movie.h"
#include "gamepal.h"
#include "cockpit.h"
#include "strutil.h"
#include "reorder.h"
#include "rendermine.h"
#include "light.h"
#include "lightmap.h"
#include "../effects/glow.h"
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

void DefaultEffectSettings (bool bSetup = false);
void SmokeDetailsMenu (void);

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

static int nShadows, nCoronas, nLightTrails;

static const char* pszExplShrapnels [5];
static const char* pszNoneBasicFull [3];
static const char* pszSmokeQuality [MAX_PARTICLE_QUALITY + 1];
static const char* pszNoneBasicAdv [3];
static const char* pszNoneStdHigh [3];
static const char* pszOffOn [2];
static const char* pszThrusters [3];

int EffectOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v;

if ((m = menu ["shockwaves"])) {
	v = m->Value ();
	if (gameOpts->render.effects.nShockwaves != v) {
		gameOpts->render.effects.nShockwaves = v;
		sprintf (m->m_text, TXT_EXPLOSION_BLASTS, pszNoneBasicFull [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["debris"])) {
	v = m->Value ();
	if (gameOpts->render.effects.nShrapnels != v) {
		gameOpts->render.effects.nShrapnels = v;
		sprintf (m->m_text, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["thrusters"])) {
	v = m->Value ();
	if (extraGameInfo [0].bThrusterFlames != v) {
		extraGameInfo [0].bThrusterFlames = v;
		sprintf (m->m_text, TXT_THRUSTER_FLAMES, pszThrusters [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["light trails"])) {
	v = m->Value ();
	if (nLightTrails != v) {
		nLightTrails = v;
		key = -2;
		}
	}

if ((m = menu ["particles"])) {
	v = m->Value ();
	if (gameOpts->render.particles.nQuality != v) {
		if (!gameOpts->render.particles.nQuality || !v)
			key = -2;
		else
			m->m_bRebuild = -1;
		gameOpts->render.particles.nQuality = v;
		sprintf (m->m_text, TXT_SMOKE, pszSmokeQuality [gameOpts->render.particles.nQuality]);
		}
	}

if ((m = menu ["shadows"])) {
	v = m->Value ();
	if (nShadows != v) {
		nShadows = v;
		sprintf (m->m_text, TXT_SHADOWS, pszNoneBasicFull [nShadows]);
		m->m_bRebuild = -1;
		}
	}

if ((m = menu ["lightning"])) {
	v = m->Value ();
	if (extraGameInfo [0].bUseLightning != v) {
		extraGameInfo [0].bUseLightning = v;
		sprintf (m->m_text, TXT_LIGHTNING, pszNoneBasicFull [int (extraGameInfo [0].bUseLightning)]);
		m->m_bRebuild = -1;
		}
	}

if ((m = menu ["glow"])) {
	v = m->Value ();
	if (gameOpts->render.effects.bGlow != v) {
		gameOpts->render.effects.bGlow = v;
		sprintf (m->m_text, TXT_EFFECTS_GLOW, pszOffOn [gameOpts->render.effects.bGlow]);
		m->m_bRebuild = -1;
		}
	}

if ((m = menu ["coronas"])) {
	v = m->Value ();
	if (nCoronas != v) {
		nCoronas = v;
		sprintf (m->m_text, TXT_CORONAS, pszOffOn [nCoronas]);
		m->m_bRebuild = -1;
		}
	}

return nCurItem;
}

//------------------------------------------------------------------------------

static void InitStrings (void)
{
	static bool bInitialized = false;

if (bInitialized)
	return;
bInitialized = true;

pszExplShrapnels [0] = TXT_STANDARD;
pszExplShrapnels [1] = TXT_FEW;
pszExplShrapnels [2] = TXT_MEDIUM;
pszExplShrapnels [3] = TXT_MANY;
pszExplShrapnels [4] = TXT_EXTREME;

pszNoneBasicFull [0] = TXT_NONE;
pszNoneBasicFull [1] = TXT_BASIC;
pszNoneBasicFull [2] = TXT_FULL;

pszSmokeQuality [0] = TXT_NONE;
pszSmokeQuality [1] = TXT_BASIC;
pszSmokeQuality [2] = TXT_STANDARD;
pszSmokeQuality [3] = TXT_HIGH;

pszNoneBasicAdv [0] = TXT_NONE;
pszNoneBasicAdv [1] = TXT_BASIC;
pszNoneBasicAdv [2] = TXT_ADVANCED;

pszNoneStdHigh [0] = TXT_NONE;
pszNoneStdHigh [1] = TXT_STANDARD;
pszNoneStdHigh [2] = TXT_HIGH;

pszThrusters [0] = TXT_NONE;
pszThrusters [1] = TXT_2D;
pszThrusters [2] = TXT_3D;

pszOffOn [0] = TXT_OFF;
pszOffOn [1] = TXT_ON;
}

//------------------------------------------------------------------------------

static const char* softParticleIds [] = {"soft sprites", "soft sparks", "soft smoke"};

void EffectOptionsMenu (void)
{
	static int choice = 0;

	CMenu	m;
	int	i, j;
#if 0
	int	optShockwaves;
#endif
	char	szSlider [50];

InitStrings ();

if (gameOpts->render.coronas.nStyle > 1)
	gameOpts->render.coronas.nStyle = 1;
nCoronas = (ogl.m_features.bDepthBlending > 0) && gameOpts->render.coronas.bUse && gameOpts->render.coronas.nStyle;
nShadows = gameOpts->render.ShadowQuality ();
nLightTrails = extraGameInfo [0].bLightTrails ? gameOpts->render.particles.bPlasmaTrails ? 2 : 1 : 0;
do {
	m.Destroy ();
	m.Create (30);

	m.AddCheck ("enable fx", TXT_ENABLE_EFFECTS, gameOpts->render.effects.bEnabled, KEY_F, HTX_ENABLE_EFFECTS);
	m.AddText ("", "");
	sprintf (szSlider + 1, TXT_SMOKE, pszSmokeQuality [gameOpts->render.particles.nQuality]);
	*szSlider = *(TXT_SMOKE - 1);
	m.AddSlider ("particles", szSlider + 1, gameOpts->render.particles.nQuality, 0, MAX_PARTICLE_QUALITY, KEY_P, HTX_SMOKE);
	if (ogl.m_features.bStencilBuffer) {
		sprintf (szSlider + 1, TXT_SHADOWS, pszNoneBasicFull [nShadows]);
		*szSlider = *(TXT_SHADOWS - 1);
		m.AddSlider ("shadows", szSlider + 1, nShadows, 0, 2, KEY_S, HTX_SHADOWS);
		}
	if (ogl.m_features.bDepthBlending > 0) {
		sprintf (szSlider + 1, TXT_CORONAS, pszOffOn [nCoronas]);
		*szSlider = *(TXT_CORONAS - 1);
		m.AddSlider ("coronas", szSlider + 1, nCoronas, 0, 1, KEY_O, HTX_CORONAS);
		}

	sprintf (szSlider + 1, TXT_LIGHTNING, pszNoneBasicFull [int (extraGameInfo [0].bUseLightning)]);
	*szSlider = *(TXT_LIGHTNING - 1);
	m.AddSlider ("lightning", szSlider + 1, extraGameInfo [0].bUseLightning, 0, 2, KEY_L, HTX_LIGHTNING);
	sprintf (szSlider + 1, TXT_EFFECTS_GLOW, pszOffOn [gameOpts->render.effects.bGlow]);
	*szSlider = *(TXT_EFFECTS_GLOW - 1);
	m.AddSlider ("glow", szSlider + 1, gameOpts->render.effects.bGlow, 0, 1, KEY_W, HTX_EFFECTS_GLOW);
	m.AddText ("", "");

	sprintf (szSlider + 1, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [gameOpts->render.effects.nShrapnels]);
	*szSlider = *(TXT_EXPLOSION_SHRAPNELS - 1);
	m.AddSlider ("debris", szSlider + 1, gameOpts->render.effects.nShrapnels, 0, 4, KEY_A, HTX_EXPLOSION_SHRAPNELS);

	sprintf (szSlider + 1, TXT_EXPLOSION_BLASTS, pszNoneBasicFull [gameOpts->render.effects.nShockwaves]);
	*szSlider = *(TXT_EXPLOSION_BLASTS - 1);
	m.AddSlider ("shockwaves", szSlider + 1, gameOpts->render.effects.nShockwaves, 0, 2, KEY_K, HTX_EXPLOSION_BLASTS);

	sprintf (szSlider + 1, TXT_LIGHTTRAIL_QUAL, pszNoneBasicAdv [nLightTrails]);
	*szSlider = *(TXT_LIGHTTRAIL_QUAL - 1);
	m.AddSlider ("light trails", szSlider + 1, nLightTrails, 0, 1 + extraGameInfo [0].bUseParticles, KEY_T, HTX_LIGHTTRAIL_QUAL);

	sprintf (szSlider + 1, TXT_THRUSTER_FLAMES, pszThrusters [int (extraGameInfo [0].bThrusterFlames)]);
	*szSlider = *(TXT_THRUSTER_FLAMES - 1);
	m.AddSlider ("thrusters", szSlider + 1, extraGameInfo [0].bThrusterFlames, 0, 2, KEY_U, HTX_THRUSTER_FLAMES);

	m.AddText ("", "");
	if (extraGameInfo [0].bUseParticles) {
		m.AddCheck ("static smoke", TXT_SMOKE_STATIC, gameOpts->render.particles.bStatic, KEY_X, HTX_ADVRND_STATICSMOKE);
		m.AddCheck ("gatling trails", TXT_GATLING_TRAILS, extraGameInfo [0].bGatlingTrails, KEY_G, HTX_GATLING_TRAILS);
		}

	if ((gameOptions [0].render.nQuality > 2) && (gameOpts->app.bExpertMode == SUPERUSER)) {
		m.AddText ("", "");
		m.AddCheck (softParticleIds [0], TXT_SOFT_SPRITES, gameOpts->SoftBlend (SOFT_BLEND_SPRITES) != 0, KEY_I, HTX_SOFT_SPRITES);
		m.AddCheck (softParticleIds [1], TXT_SOFT_SPARKS, gameOpts->SoftBlend (SOFT_BLEND_SPARKS) != 0, KEY_A, HTX_SOFT_SPARKS);
		if (extraGameInfo [0].bUseParticles)
			m.AddCheck (softParticleIds [2], TXT_SOFT_SMOKE, gameOpts->SoftBlend (SOFT_BLEND_PARTICLES) != 0, KEY_O, HTX_SOFT_SMOKE);
		}
	if (gameOpts->app.bExpertMode && extraGameInfo [0].bUseParticles) {
		m.AddText ("", "");
		m.AddMenu ("smoke details", TXT_SMOKE_DETAILS, KEY_D, HTX_SMOKE_DETAILS);
		}

	for (;;) {
		i = m.Menu (NULL, TXT_EFFECT_MENUTITLE, EffectOptionsCallback, &choice);
		if (i < 0)
			break;
		if (i == m.IndexOf ("smoke details"))
			SmokeDetailsMenu ();
		}

	extraGameInfo [0].bUseParticles = (gameOpts->render.particles.nQuality != 0);
	if (m.Available ("shadows")) {
		if ((extraGameInfo [0].bShadows = (nShadows != 0)))
			gameOpts->render.shadows.nReach =
			gameOpts->render.shadows.nClip = nShadows;
		}
	if (m.Available ("coronas")) {
		if ((gameOpts->render.coronas.bUse = (nCoronas != 0)))
			gameOpts->render.coronas.nStyle = nCoronas;
		}
	for (j = 0; j < 3; j++) {
		if (m.Available (softParticleIds [j])) {
			if (m.Value (softParticleIds [j]))
				gameOpts->render.effects.bSoftParticles |= 1 << j;
			else
				gameOpts->render.effects.bSoftParticles &= ~(1 << j);
			}
		}
	GET_VAL (gameOpts->render.effects.bEnabled, "enable fx");
	GET_VAL (gameOpts->render.particles.bStatic, "static smoke");
	GET_VAL (extraGameInfo [0].bGatlingTrails, "gatling trails");
	if ((extraGameInfo [0].bLightTrails = (nLightTrails != 0)))
		gameOpts->render.particles.bPlasmaTrails = (nLightTrails == 2);
	} while (i == -2);

particleManager.ClearBuffers ();
SetDebrisCollisions ();
DefaultEffectSettings ();
if (gameStates.app.bGameRunning) {
	particleImageManager.LoadAll ();
	if (gameOpts->render.effects.bEnabled && gameOpts->render.effects.bEnergySparks && !sparkManager.HaveSparks ())
		sparkManager.Setup ();
	}
if (!(gameOpts->render.effects.bEnabled &&  extraGameInfo [0].bUseLightning))
	lightningManager.ResetLights (1);
}

//------------------------------------------------------------------------------
//eof
