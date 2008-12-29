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

#include "inferno.h"
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
#include "menubackground.h"
#include "netmenu.h"

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

static int 
	optCapVirLim, optCapTimLim, optMaxVirCap, optBumpVirCap, optBashVirCap, optVirGenTim, 
	optVirLife, optVirStab, optConqWarn, optRevRooms, optEnergyFill, optShieldFill, optShieldDmg, 
	optOvrTex, optBrRooms;

//------------------------------------------------------------------------------

void NetworkEntropyToggleOptions (void)
{
	CMenu	m (12);
	int	optPlrHand;

optPlrHand = m.AddCheck (TXT_ENT_HANDICAP, extraGameInfo [0].entropy.bPlayerHandicap, KEY_H, HTX_ONLINE_MANUAL);
optConqWarn = m.AddCheck (TXT_ENT_CONQWARN, extraGameInfo [0].entropy.bDoConquerWarning, KEY_W, HTX_ONLINE_MANUAL);
optRevRooms = m.AddCheck (TXT_ENT_REVERT, extraGameInfo [0].entropy.bRevertRooms, KEY_R, HTX_ONLINE_MANUAL);
m.AddText ("");
optVirStab = m.AddText (TXT_ENT_VIRSTAB);
m.AddRadio (TXT_ENT_VIRSTAB_DROP, 0, KEY_D);
m.AddRadio (TXT_ENT_VIRSTAB_ENEMY, 0, KEY_T);
m.AddRadio (TXT_ENT_VIRSTAB_TOUCH, 0, KEY_L);
m.AddRadio (TXT_ENT_VIRSTAB_NEVER, 0, KEY_N);
m [optVirStab + extraGameInfo [0].entropy.nVirusStability].m_value = 1;

m.Menu (NULL, TXT_ENT_TOGGLES, NULL, 0);

extraGameInfo [0].entropy.bRevertRooms = m [optRevRooms].m_value;
extraGameInfo [0].entropy.bDoConquerWarning = m [optConqWarn].m_value;
extraGameInfo [0].entropy.bPlayerHandicap = m [optPlrHand].m_value;
for (extraGameInfo [0].entropy.nVirusStability = 0; 
	  extraGameInfo [0].entropy.nVirusStability < 3; 
	  extraGameInfo [0].entropy.nVirusStability++)
	if (m [optVirStab + extraGameInfo [0].entropy.nVirusStability].m_value)
		break;
}

//------------------------------------------------------------------------------

void NetworkEntropyTextureOptions (void)
{
	CMenu	m (7);

optOvrTex = m.AddRadio (TXT_ENT_TEX_KEEP, 0, KEY_K, HTX_ONLINE_MANUAL);
m.AddRadio (TXT_ENT_TEX_OVERRIDE, 0, KEY_O, HTX_ONLINE_MANUAL);
m.AddRadio (TXT_ENT_TEX_COLOR, 0, KEY_C, HTX_ONLINE_MANUAL);
m [optOvrTex + extraGameInfo [0].entropy.nOverrideTextures].m_value = 1;
m.AddText ("");
optBrRooms = m.AddCheck (TXT_ENT_TEX_BRIGHTEN, extraGameInfo [0].entropy.bBrightenRooms, KEY_B, HTX_ONLINE_MANUAL);

m.Menu (NULL, TXT_ENT_TEXTURES, NULL, 0);

extraGameInfo [0].entropy.bBrightenRooms = m [optBrRooms].m_value;
for (extraGameInfo [0].entropy.nOverrideTextures = 0; 
	  extraGameInfo [0].entropy.nOverrideTextures < 3; 
	  extraGameInfo [0].entropy.nOverrideTextures++)
	if (m [optOvrTex + extraGameInfo [0].entropy.nOverrideTextures].m_value)
		break;
}

//------------------------------------------------------------------------------

void NetworkEntropyOptions (void)
{
	CMenu		m (25);
	int		i, optTogglesMenu, optTextureMenu;
	char		szCapVirLim [10], szCapTimLim [10], szMaxVirCap [10], szBumpVirCap [10], 
				szBashVirCap [10], szVirGenTim [10], szVirLife [10], 
				szEnergyFill [10], szShieldFill [10], szShieldDmg [10];

optCapVirLim = m.AddInput (TXT_ENT_VIRLIM, szCapVirLim, extraGameInfo [0].entropy.nCaptureVirusLimit, 3, HTX_ONLINE_MANUAL);
optCapTimLim = m.AddInput (TXT_ENT_CAPTIME, szCapTimLim, extraGameInfo [0].entropy.nCaptureTimeLimit, 3, HTX_ONLINE_MANUAL);
optMaxVirCap = m.AddInput (TXT_ENT_MAXCAP, szMaxVirCap, extraGameInfo [0].entropy.nMaxVirusCapacity, 6, HTX_ONLINE_MANUAL);
optBumpVirCap = m.AddInput (TXT_ENT_BUMPCAP, szBumpVirCap, extraGameInfo [0].entropy.nBumpVirusCapacity, 3, HTX_ONLINE_MANUAL);
optBashVirCap = m.AddInput (TXT_ENT_BASHCAP, szBashVirCap, extraGameInfo [0].entropy.nBashVirusCapacity, 3, HTX_ONLINE_MANUAL);
optVirGenTim = m.AddInput (TXT_ENT_GENTIME, szVirGenTim, extraGameInfo [0].entropy.nVirusGenTime, 3, HTX_ONLINE_MANUAL);
optVirLife = m.AddInput (TXT_ENT_VIRLIFE, szVirLife, extraGameInfo [0].entropy.nVirusLifespan, 3, HTX_ONLINE_MANUAL);
optEnergyFill = m.AddInput (TXT_ENT_EFILL, szEnergyFill, extraGameInfo [0].entropy.nEnergyFillRate, 3, HTX_ONLINE_MANUAL);
optShieldFill = m.AddInput (TXT_ENT_SFILL, szShieldFill, extraGameInfo [0].entropy.nShieldFillRate, 3, HTX_ONLINE_MANUAL);
optShieldDmg = m.AddInput (TXT_ENT_DMGRATE, szShieldDmg, extraGameInfo [0].entropy.nShieldDamageRate, 3, HTX_ONLINE_MANUAL);
m.AddText ("");
optTogglesMenu = m.AddMenu (TXT_ENT_TGLMENU, KEY_E);
optTextureMenu = m.AddMenu (TXT_ENT_TEXMENU, KEY_T);

for (;;) {
	i = m.Menu (NULL, "Entropy Options", NULL, 0);
	if (i == optTogglesMenu)
		NetworkEntropyToggleOptions ();
	else if (i == optTextureMenu)
		NetworkEntropyTextureOptions ();
	else
		break;
	}
extraGameInfo [0].entropy.nCaptureVirusLimit = (char) atol (m [optCapVirLim].m_text);
extraGameInfo [0].entropy.nCaptureTimeLimit = (char) atol (m [optCapTimLim].m_text);
extraGameInfo [0].entropy.nMaxVirusCapacity = (ushort) atol (m [optMaxVirCap].m_text);
extraGameInfo [0].entropy.nBumpVirusCapacity = (char) atol (m [optBumpVirCap].m_text);
extraGameInfo [0].entropy.nBashVirusCapacity = (char) atol (m [optBashVirCap].m_text);
extraGameInfo [0].entropy.nVirusGenTime = (char) atol (m [optVirGenTim].m_text);
extraGameInfo [0].entropy.nVirusLifespan = (char) atol (m [optVirLife].m_text);
extraGameInfo [0].entropy.nEnergyFillRate = (ushort) atol (m [optEnergyFill].m_text);
extraGameInfo [0].entropy.nShieldFillRate = (ushort) atol (m [optShieldFill].m_text);
extraGameInfo [0].entropy.nShieldDamageRate = (ushort) atol (m [optShieldDmg].m_text);
}

//------------------------------------------------------------------------------

