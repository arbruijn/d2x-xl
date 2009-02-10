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
#include "menubackground.h"

//------------------------------------------------------------------------------

static int nBonusOpt, nSizeModOpt, nPyroForceOpt;

int MonsterballMenuCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v;

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

ubyte nOptionToForce [] = {
	5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 
	60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 175, 200, 250
	};

static const char *szWeaponTexts [] = {
	"Laser 1", 
	"Laser 2", 
	"Laser 3", 
	"Laser 4", 
	"Spreadfire", 
	"Vulcan", 
	"Plasma", 
	"Fusion", 
	"Superlaser 1", 
	"Superlaser 2", 
	"Helix", 
	"Gauss", 
	"Phoenix", 
	"Omega", 
	"Flare", 
	"Concussion", 
	"Homing", 
	"Smart", 
	"Mega", 
	"Flash", 
	"Guided", 
	"Mercury", 
	"Earthshaker", 
	"Shaker Bomblet"
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
	CMenu					m (35);
	int					h, i, j, opt = 0, optDefaultForces;
	char					szBonus [60], szSize [60], szPyroForce [60];
	tMonsterballForce	*pf = extraGameInfo [0].monsterball.forces;

h = (int) sizeofa (optionToWeaponId);
j = (int) sizeofa (nOptionToForce);
for (i = opt = 0; i < h; i++, opt++, pf++) {
	m.AddSlider (szWeaponTexts [i], ForceToOption (pf->nForce), 0, j - 1, 0, NULL);
	if (pf->nWeaponId == FLARE_ID)
		m.AddText ("", 0);
	}
m.AddText ("", 0);
sprintf (szPyroForce + 1, TXT_MBALL_PYROFORCE, pf->nForce);
*szPyroForce = *(TXT_MBALL_PYROFORCE - 1);
nPyroForceOpt = m.AddSlider (szPyroForce + 1, pf->nForce - 1, 0, 9, 0, NULL);
m.AddText ("", 0);
sprintf (szBonus + 1, TXT_GOAL_BONUS, extraGameInfo [0].monsterball.nBonus);
*szBonus = *(TXT_GOAL_BONUS - 1);
nBonusOpt = m.AddSlider (szBonus + 1, extraGameInfo [0].monsterball.nBonus - 1, 0, 9, 0, HTX_GOAL_BONUS);
i = extraGameInfo [0].monsterball.nSizeMod;
sprintf (szSize + 1, TXT_MBALL_SIZE, i / 2, (i & 1) ? 5 : 0);
*szSize = *(TXT_MBALL_SIZE - 1);
nSizeModOpt = m.AddSlider (szSize + 1, extraGameInfo [0].monsterball.nSizeMod - 2, 0, 8, 0, HTX_MBALL_SIZE);
m.AddText ("", 0);
optDefaultForces = m.AddMenu ("Set default values", 0, NULL);

for (;;) {
	i = m.Menu (NULL, "Monsterball Impact Forces", MonsterballMenuCallback, 0);
	if (i == -1)
		break;
	if (i != optDefaultForces)
		break;
	InitMonsterballSettings (&extraGameInfo [0].monsterball);
	pf = extraGameInfo [0].monsterball.forces;
	for (i = 0; i < h + 1; i++, pf++) {
		m [i].m_value = ForceToOption (pf->nForce);
		if (pf->nWeaponId == FLARE_ID)
			i++;
		}
	m [nPyroForceOpt].m_value = NMCLAMP (pf->nForce - 1, 0, 9);
	m [nSizeModOpt].m_value = extraGameInfo [0].monsterball.nSizeMod - 2;
	}
pf = extraGameInfo [0].monsterball.forces;
for (i = 0; i < h; i++, pf++)
	pf->nForce = nOptionToForce [m [i].m_value];
pf->nForce = m [nPyroForceOpt].m_value + 1;
extraGameInfo [0].monsterball.nBonus = m [nBonusOpt].m_value + 1;
extraGameInfo [0].monsterball.nSizeMod = m [nSizeModOpt].m_value + 2;
}

//------------------------------------------------------------------------------

