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

static int32_t nShadows, nCoronas, nLightTrails;

static const char* pszExplDebris [5];
static const char* pszExplShrapnels [5];
static const char* pszNoneBasicFull [3];
static const char* pszNoneBasicStdFull [4];
static const char* pszSmokeQuality [MAX_PARTICLE_QUALITY + 1];
static const char* pszNoneBasicAdv [3];
static const char* pszOffFastFull [3];
static const char* pszOffOn [2];
static const char* pszThrusters [3];
static const char* pszShieldEffect [3];

int32_t EffectOptionsCallback (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int32_t		v;

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
	if (gameOpts->render.effects.nDebris != v) {
		gameOpts->render.effects.nDebris = v;
		sprintf (m->m_text, TXT_EXPLOSION_DEBRIS, pszExplDebris [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["shrapnel"])) {
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

if ((m = menu ["shield effect"])) {
	v = m->Value ();
	if (gameOpts->render.effects.bShields != v) {
		extraGameInfo [0].nShieldEffect = 
		gameOpts->render.effects.bShields = v;
		sprintf (m->m_text, TXT_SHIELD_EFFECT, pszShieldEffect [v]);
		m->Rebuild ();
		key = -2;
		}
	}

if ((m = menu ["warp effect"])) {
	v = m->Value ();
	if (gameOpts->render.effects.bWarpAppearance != v) {
		gameOpts->render.effects.bWarpAppearance = v;
		sprintf (m->m_text, TXT_SHIELD_EFFECT, pszNoneBasicFull [v]);
		m->Rebuild ();
		key = -2;
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
		sprintf (m->m_text, TXT_LIGHTNING, pszNoneBasicStdFull [int32_t (extraGameInfo [0].bUseLightning)]);
		m->m_bRebuild = -1;
		}
	}

if ((m = menu ["glow"])) {
	v = m->Value ();
	if (gameOpts->render.effects.bGlow != v) {
		gameOpts->render.effects.bGlow = v;
		sprintf (m->m_text, TXT_EFFECTS_GLOW, pszOffFastFull [gameOpts->render.effects.bGlow]);
		m->m_bRebuild = -1;
		}
	}

if ((m = menu ["fog"])) {
	v = m->Value ();
	if (gameOpts->render.effects.bFog != v) {
		gameOpts->render.effects.bFog  = v;
		sprintf (m->m_text, TXT_EFFECTS_FOG, pszOffOn [gameOpts->render.effects.bFog]);
		m->m_bRebuild = -1;
		}
	}

if ((m = menu ["energy sparks"])) {
	v = m->Value ();
	if (gameOpts->render.effects.bEnergySparks != v) {
		gameOpts->render.effects.bEnergySparks = v;
		sprintf (m->m_text, TXT_RENDER_SPARKS, pszNoneBasicFull [gameOpts->render.effects.bEnergySparks]);
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

pszExplDebris [0] = TXT_STANDARD;
pszExplDebris [1] = TXT_FEW;
pszExplDebris [2] = TXT_MEDIUM;
pszExplDebris [3] = TXT_MANY;
pszExplDebris [4] = TXT_EXTREME;

pszExplShrapnels [0] = TXT_NONE;
pszExplShrapnels [1] = TXT_FEW;
pszExplShrapnels [2] = TXT_MEDIUM;
pszExplShrapnels [3] = TXT_MANY;
pszExplShrapnels [4] = TXT_EXTREME;

pszNoneBasicFull [0] = TXT_NONE;
pszNoneBasicFull [1] = TXT_BASIC;
pszNoneBasicFull [2] = TXT_FULL;

pszNoneBasicStdFull [0] = TXT_NONE;
pszNoneBasicStdFull [1] = TXT_BASIC;
pszNoneBasicStdFull [2] = TXT_HIGH;
pszNoneBasicStdFull [3] = TXT_MAXIMAL;

pszSmokeQuality [0] = TXT_NONE;
pszSmokeQuality [1] = TXT_BASIC;
pszSmokeQuality [2] = TXT_STANDARD;
pszSmokeQuality [3] = TXT_HIGH;

pszNoneBasicAdv [0] = TXT_NONE;
pszNoneBasicAdv [1] = TXT_BASIC;
pszNoneBasicAdv [2] = TXT_ADVANCED;

pszOffFastFull [0] = TXT_NONE;
pszOffFastFull [1] = TXT_FAST;
pszOffFastFull [2] = TXT_FULL;

pszThrusters [0] = TXT_NONE;
pszThrusters [1] = TXT_2D;
pszThrusters [2] = TXT_3D;

pszShieldEffect [0] = TXT_OFF;
pszShieldEffect [1] = TXT_PLAYERS;
pszShieldEffect [2] = TXT_PLAYERS_AND_ROBOTS;

pszOffOn [0] = TXT_OFF;
pszOffOn [1] = TXT_ON;
}

//------------------------------------------------------------------------------

static const char* softParticleIds [] = {"soft sprites", "soft particles"};

static int32_t nSoftRenderFlags [] = {SOFT_BLEND_SPRITES, SOFT_BLEND_PARTICLES};

void EffectOptionsMenu (void)
{
	static int32_t choice = 0;

	CMenu	m;
	int32_t	i, j;
#if 0
	int32_t	optShockwaves;
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
	m.Create (30, "EffectOptionsMenu");

	m.AddCheck ("enable fx", TXT_ENABLE_EFFECTS, gameOpts->render.effects.bEnabled, KEY_E, HTX_ENABLE_EFFECTS);
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

	sprintf (szSlider + 1, TXT_LIGHTNING, pszNoneBasicStdFull [int32_t (extraGameInfo [0].bUseLightning)]);
	*szSlider = *(TXT_LIGHTNING - 1);
	m.AddSlider ("lightning", szSlider + 1, extraGameInfo [0].bUseLightning, 0, 3, KEY_L, HTX_LIGHTNING);
	sprintf (szSlider + 1, TXT_EFFECTS_GLOW, pszOffFastFull [gameOpts->render.effects.bGlow]);
	*szSlider = *(TXT_EFFECTS_GLOW - 1);
	m.AddSlider ("glow", szSlider + 1, gameOpts->render.effects.bGlow, 0, 2, KEY_W, HTX_EFFECTS_GLOW);
	sprintf (szSlider + 1, TXT_EFFECTS_FOG, pszOffOn [gameOpts->render.effects.bFog]);
	*szSlider = *(TXT_EFFECTS_FOG - 1);
	m.AddSlider ("fog", szSlider + 1, gameOpts->render.effects.bFog, 0, 1, KEY_F, HTX_EFFECTS_FOG);
	sprintf (szSlider + 1, TXT_RENDER_SPARKS, pszNoneBasicFull [gameOpts->render.effects.bEnergySparks]);
	*szSlider = *(TXT_RENDER_SPARKS - 1);
	m.AddSlider ("energy sparks", szSlider + 1, gameOpts->render.effects.bEnergySparks, 0, 2, KEY_P, HTX_RENDER_SPARKS);
	m.AddText ("", "");

	sprintf (szSlider + 1, TXT_EXPLOSION_DEBRIS, pszExplDebris [gameOpts->render.effects.nDebris]);
	*szSlider = *(TXT_EXPLOSION_DEBRIS - 1);
	m.AddSlider ("debris", szSlider + 1, gameOpts->render.effects.nDebris, 0, 4, KEY_D, HTX_EXPLOSION_DEBRIS);

	sprintf (szSlider + 1, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [gameOpts->render.effects.nShrapnels]);
	*szSlider = *(TXT_EXPLOSION_SHRAPNELS - 1);
	m.AddSlider ("shrapnel", szSlider + 1, gameOpts->render.effects.nShrapnels, 0, 4, KEY_H, HTX_EXPLOSION_SHRAPNEL);

	sprintf (szSlider + 1, TXT_EXPLOSION_BLASTS, pszNoneBasicFull [gameOpts->render.effects.nShockwaves]);
	*szSlider = *(TXT_EXPLOSION_BLASTS - 1);
	m.AddSlider ("shockwaves", szSlider + 1, gameOpts->render.effects.nShockwaves, 0, 2, KEY_K, HTX_EXPLOSION_BLASTS);

	sprintf (szSlider + 1, TXT_LIGHTTRAIL_QUAL, pszNoneBasicAdv [nLightTrails]);
	*szSlider = *(TXT_LIGHTTRAIL_QUAL - 1);
	m.AddSlider ("light trails", szSlider + 1, nLightTrails, 0, 1 + extraGameInfo [0].bUseParticles, KEY_T, HTX_LIGHTTRAIL_QUAL);

	sprintf (szSlider + 1, TXT_THRUSTER_FLAMES, pszThrusters [int32_t (extraGameInfo [0].bThrusterFlames)]);
	*szSlider = *(TXT_THRUSTER_FLAMES - 1);
	m.AddSlider ("thrusters", szSlider + 1, extraGameInfo [0].bThrusterFlames, 0, 2, KEY_U, HTX_THRUSTER_FLAMES);

	sprintf (szSlider + 1, TXT_SHIELD_EFFECT, pszShieldEffect [int32_t (gameOpts->render.effects.bShields)]);
	*szSlider = *(TXT_SHIELD_EFFECT - 1);
	m.AddSlider ("shield effect", szSlider + 1, gameOpts->render.effects.bShields, 0, 2, KEY_S, HTX_SHIELD_EFFECT);

	sprintf (szSlider + 1, TXT_WARP_EFFECT, pszNoneBasicFull [int32_t (gameOpts->render.effects.bWarpAppearance)]);
	*szSlider = *(TXT_WARP_EFFECT - 1);
	m.AddSlider ("warp effect", szSlider + 1, gameOpts->render.effects.bWarpAppearance, 0, 2, KEY_W, HTX_WARP_EFFECT);

	m.AddText ("", "");
	if (extraGameInfo [0].bUseParticles) {
		m.AddCheck ("static smoke", TXT_SMOKE_STATIC, gameOpts->render.particles.bStatic, KEY_X, HTX_ADVRND_STATICSMOKE);
		m.AddCheck ("gatling trails", TXT_GATLING_TRAILS, extraGameInfo [0].bGatlingTrails, KEY_G, HTX_GATLING_TRAILS);
		}
	if ((gameOptions [0].render.nQuality > 2) && (gameOpts->app.bExpertMode == SUPERUSER)) {
		m.AddCheck (softParticleIds [0], TXT_SOFT_SPRITES, gameOpts->SoftBlend (SOFT_BLEND_SPRITES) != 0, KEY_I, HTX_SOFT_SPRITES);
		m.AddCheck (softParticleIds [1], TXT_SOFT_PARTICLES, gameOpts->SoftBlend (SOFT_BLEND_PARTICLES) != 0, KEY_A, HTX_SOFT_PARTICLES);
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
	for (j = 0; j < 2; j++) {
		if (m.Available (softParticleIds [j])) {
			if (m.Value (softParticleIds [j]))
				gameOpts->render.effects.bSoftParticles |= nSoftRenderFlags [j];
			else
				gameOpts->render.effects.bSoftParticles &= ~nSoftRenderFlags [j];
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
