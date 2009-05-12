#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#define PATCH12

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#	include <winsock.h>
#else
#	include <sys/socket.h>
#endif

#include "descent.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "ipx.h"
#include "ipx_udp.h"
#include "menu.h"
#include "key.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "menu.h"
#include "text.h"
#include "byteswap.h"
#include "netmisc.h"
#include "kconfig.h"
#include "autodl.h"
#include "tracker.h"
#include "gamefont.h"
#include "netmenu.h"
#include "monsterball.h"
#include "menubackground.h"

//------------------------------------------------------------------------------

static int nBonusOpt, nSizeModOpt, nPyroForceOpt;

static const char *szWeaponTexts [] = {
	"Laser 1: %1.2f", 
	"Laser 2: %1.2f", 
	"Laser 3: %1.2f", 
	"Laser 4: %1.2f", 
	"Spreadfire: %1.2f", 
	"Vulcan: %1.2f", 
	"Plasma: %1.2f", 
	"Fusion: %1.2f", 
	"Superlaser 1: %1.2f", 
	"Superlaser 2: %1.2f", 
	"Helix: %1.2f", 
	"Gauss: %1.2f", 
	"Phoenix: %1.2f", 
	"Omega: %1.2f", 
	"Flare: %1.2f", 
	"Concussion: %1.2f", 
	"Homing: %1.2f", 
	"Smart: %1.2f", 
	"Mega: %1.2f", 
	"Flash: %1.2f", 
	"Guided: %1.2f", 
	"Mercury: %1.2f", 
	"Earthshaker: %1.2f", 
	"Shaker Bomblet: %1.2f"
	};

short nOptionToForce [] = {1, 3, 5, 10, 20, 30, 50, 75, 100, 200, 300, 500, 1000, 2500, 5000, 10000};


int MonsterballMenuCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem			*m;
	int					h,i, j, v;
	tMonsterballForce	*pf = extraGameInfo [0].monsterball.forces;

h = sizeofa (szWeaponTexts);
for (i = j = 0; i <= h; i++, j++, pf++) {
	m = menu + i;
	v = m->m_value;
	if (pf->nForce != nOptionToForce [v]) {
		pf->nForce = nOptionToForce [v];
		sprintf (m->m_text, szWeaponTexts [j], float (pf->nForce) / 100.0f);
		m->m_bRebuild = 1;
		}
	if (pf->nWeaponId == FLARE_ID)
		i++;
	}

m = menu + nPyroForceOpt;
v = m->m_value + 1;
if (v != extraGameInfo [0].monsterball.forces [24].nForce) {
	extraGameInfo [0].monsterball.forces [24].nForce = v;
	sprintf (m->m_text, TXT_MBALL_PYROFORCE, v);
	m->m_bRebuild = 1;
	//key = -2;
	return nCurItem;
	}
m = menu + nBonusOpt;
v = m->m_value + 1;
if (v != extraGameInfo [0].monsterball.nBonus) {
	extraGameInfo [0].monsterball.nBonus = v;
	sprintf (m->m_text, TXT_GOAL_BONUS, v);
	m->m_bRebuild = 1;
	//key = -2;
	return nCurItem;
	}
m = menu + nSizeModOpt;
v = m->m_value + 2;
if (v != extraGameInfo [0].monsterball.nSizeMod) {
	extraGameInfo [0].monsterball.nSizeMod = v;
	sprintf (m->m_text, TXT_MBALL_SIZE, v / 2, (v & 1) ? 5 : 0);
	m->m_bRebuild = 1;
	//key = -2;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

static int optionToWeaponId [] = {
	LASER_ID, 
	LASER_ID + 1, 
	LASER_ID + 2, 
	LASER_ID + 3, 
	SPREADFIRE_ID, 
	VULCAN_ID, 
	PLASMA_ID, 
	FUSION_ID, 
	SUPERLASER_ID, 
	SUPERLASER_ID + 1, 
	HELIX_ID, 
	GAUSS_ID, 
	PHOENIX_ID, 
	OMEGA_ID, 
	FLARE_ID, 
	CONCUSSION_ID, 
	HOMINGMSL_ID, 
	SMARTMSL_ID, 
	MEGAMSL_ID, 
	FLASHMSL_ID, 
	GUIDEDMSL_ID, 
	MERCURYMSL_ID, 
	EARTHSHAKER_ID, 
	EARTHSHAKER_MEGA_ID
	};

static inline int ForceToOption (double dForce)
{
	int	i, h = (int) sizeofa (nOptionToForce);

for (i = 0; i < h - 1; i++)
	if ((dForce >= nOptionToForce [i]) && (dForce < nOptionToForce [i + 1]))
		break;
return i;
}

void NetworkMonsterballOptions (void)
{
	static int choice = 0;

	CMenu					m (35);
	int					h, i, j, opt = 0, optDefaultForces;
	char					szSlider [60];
	tMonsterballForce	*pf = extraGameInfo [0].monsterball.forces;

h = (int) sizeofa (optionToWeaponId);
j = (int) sizeofa (nOptionToForce);
for (i = opt = 0; i < h; i++, opt++, pf++) {
	sprintf (szSlider, szWeaponTexts [i], float (pf->nForce) / 100.0f);
	m.AddSlider (szSlider, ForceToOption (pf->nForce), 0, j - 1, 0, NULL);
	if (pf->nWeaponId == FLARE_ID)
		m.AddText ("", 0);
	}
m.AddText ("", 0);
sprintf (szSlider + 1, TXT_MBALL_PYROFORCE, pf->nForce);
*szSlider = *(TXT_MBALL_PYROFORCE - 1);
nPyroForceOpt = m.AddSlider (szSlider + 1, pf->nForce - 1, 0, 9, 0, NULL);
m.AddText ("", 0);
sprintf (szSlider + 1, TXT_GOAL_BONUS, extraGameInfo [0].monsterball.nBonus);
*szSlider = *(TXT_GOAL_BONUS - 1);
nBonusOpt = m.AddSlider (szSlider + 1, extraGameInfo [0].monsterball.nBonus - 1, 0, 9, 0, HTX_GOAL_BONUS);
i = extraGameInfo [0].monsterball.nSizeMod;
sprintf (szSlider + 1, TXT_MBALL_SIZE, i / 2, (i & 1) ? 5 : 0);
*szSlider = *(TXT_MBALL_SIZE - 1);
nSizeModOpt = m.AddSlider (szSlider + 1, extraGameInfo [0].monsterball.nSizeMod - 2, 0, 8, 0, HTX_MBALL_SIZE);
m.AddText ("", 0);
optDefaultForces = m.AddMenu ("Set default values", 0, NULL);

for (;;) {
	i = m.Menu (NULL, "Monsterball Impact Forces", MonsterballMenuCallback, &choice);
	if (i == -1)
		break;
	if (i != optDefaultForces)
		break;
	InitMonsterballSettings (&extraGameInfo [0].monsterball);
	pf = extraGameInfo [0].monsterball.forces;
	for (i = 0; i <= h; i++, pf++) {
		m [i].m_value = ForceToOption (pf->nForce);
		if (pf->nWeaponId == FLARE_ID)
			i++;
		}
	m [nPyroForceOpt].m_value = pf->nForce - 1;
	m [nSizeModOpt].m_value = extraGameInfo [0].monsterball.nSizeMod - 2;
	}
#if 0
pf = extraGameInfo [0].monsterball.forces;
for (i = 0; i < h; i++, pf++)
	pf->nForce = nOptionToForce [m [i].m_value];
#endif
pf->nForce = m [nPyroForceOpt].m_value + 1;
extraGameInfo [0].monsterball.nSizeMod = m [nSizeModOpt].m_value + 2;
extraGameInfo [0].monsterball.nBonus = m [nBonusOpt].m_value + 1;
}

//------------------------------------------------------------------------------

