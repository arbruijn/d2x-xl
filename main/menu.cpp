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

#if DBG

const char *menuBgNames [4][2] = {
 {"menu.pcx", "menub.pcx"},
 {"menuo.pcx", "menuob.pcx"},
 {"menud.pcx", "menud.pcx"},
 {"menub.pcx", "menub.pcx"}
	};

#else

const char *menuBgNames [4][2] = {
 {"\x01menu.pcx", "\x01menub.pcx"},
 {"\x01menuo.pcx", "\x01menuob.pcx"},
 {"\x01menud.pcx", "\x01menud.pcx"},
 {"\x01menub.pcx", "\x01menub.pcx"}
	};
#endif

//------------------------------------------------------------------------------

int QuitSaveLoadMenu (void)
{
	CMenu m (5);
	int	i, choice = 0, optQuit, optOptions, optLoad, optSave;

optQuit = m.AddMenu (TXT_QUIT_GAME, KEY_Q, HTX_QUIT_GAME);
optOptions = m.AddMenu (TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF);
optLoad = m.AddMenu (TXT_LOAD_GAME2, KEY_L, HTX_LOAD_GAME);
optSave = m.AddMenu (TXT_SAVE_GAME2, KEY_S, HTX_SAVE_GAME);
i = m.Menu (NULL, TXT_ABORT_GAME, NULL, &choice);
if (!i)
	return 0;
if (i == optOptions)
	ConfigMenu ();
else if (i == optLoad)
	saveGameHandler.Load (1, 0, 0, NULL);
else if (i == optSave)
	saveGameHandler.Save (0, 0, 0, NULL);
return 1;
}

//------------------------------------------------------------------------------
/*
 * IpxSetDriver was called do_network_init and located in main/inferno
 * before the change which allows the user to choose the network driver
 * from the game menu instead of having to supply command line args.
 */
void IpxSetDriver (int ipx_driver)
{
	IpxClose ();

if (!FindArg ("-nonetwork")) {
	int nIpxError;
	int socket = 0, t;

	if ((t = FindArg ("-socket")))
		socket = atoi (pszArgList [t + 1]);
	ArchIpxSetDriver (ipx_driver);
	if ((nIpxError = IpxInit (IPX_DEFAULT_SOCKET + socket)) == IPX_INIT_OK) {
		networkData.bActive = 1;
		} 
	else {
#if 1 //TRACE
	switch (nIpxError) {
		case IPX_NOT_INSTALLED: 
			console.printf (CON_VERBOSE, "%s\n", TXT_NO_NETWORK); 
			break;
		case IPX_SOCKET_TABLE_FULL: 
			console.printf (CON_VERBOSE, "%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET + socket); 
			break;
		case IPX_NO_LOW_DOS_MEM: 
			console.printf (CON_VERBOSE, "%s\n", TXT_MEMORY_IPX); 
			break;
		default: 
			console.printf (CON_VERBOSE, "%s %d", TXT_ERROR_IPX, nIpxError);
		}
		console.printf (CON_VERBOSE, "%s\n", TXT_NETWORK_DISABLED);
#endif
		networkData.bActive = 0;		// Assume no network
	}
	IpxReadUserFile ("descent.usr");
	IpxReadNetworkFile ("descent.net");
	} 
else {
#if 1 //TRACE
	console.printf (CON_VERBOSE, "%s\n", TXT_NETWORK_DISABLED);
#endif
	networkData.bActive = 0;		// Assume no network
	}
}

//------------------------------------------------------------------------------

void DoNewIPAddress (void)
{
  CMenu	m (2);
  char	szIP [30];
  int		choice;

m.AddText (0, "Enter an address or hostname:", 0);
m.AddInput (1, szIP, 50, NULL);
choice = m.Menu (NULL, TXT_JOIN_TCP);
if ((choice == -1) || !*m [1].text)
	return;
ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_INV_ADDRESS);
}

//------------------------------------------------------------------------------
//eof
