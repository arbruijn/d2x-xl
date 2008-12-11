
#include <string.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif 

#include "inferno.h"
#include "strutil.h"
#include "u_mem.h"
#include "cfile.h"
#include "banlist.h"

//-----------------------------------------------------------------------------

typedef char tBanListEntry [CALLSIGN_LEN + 1];
typedef tBanListEntry *pBanListEntry;

CStack<tBanListEntry>	banList;

//-----------------------------------------------------------------------------

int AddPlayerToBanList (char *szPlayer)
{
if (!*szPlayer)
	return 1;
if (!banList) {
	if (!banList.Create (100 * sizeof (tBanListEntry)))
		return 0;
	}
else if (!(++nBanListSize % 100))
	banList.Resize (banList.Length () + 100);
if (!banList) 
	return 0;
banList.Push (szPlayer);
return 1;
}

//-----------------------------------------------------------------------------

int LoadBanList (void)
{
	CFile	cf;
	tBanListEntry	szPlayer;
	uint i;

if (!cf.Open ("banlist.txt", gameFolders.szDataDir, "rt", 0))
	return 0;
while (!feof (cf.File ())) {
	fgets (reinterpret_cast<char*> (szPlayer), sizeof (szPlayer), cf.File ());
	for (i = 0; i < sizeof (tBanListEntry); i++)
		if (szPlayer [i] == '\n')
			szPlayer [i] = '\0';
	szPlayer [CALLSIGN_LEN] = '\0';
	if (!AddPlayerToBanList (szPlayer))
		break;
	}
cf.Close ();
return 1;
}

//-----------------------------------------------------------------------------

int SaveBanList (void)
{
if (banList.Buffer () && banList.ToS ()) {
	CFile	cf;
	int i;

	if (!cf.Open ("banlist.txt", gameFolders.szDataDir, "wt", 0))
		return 0;
	for (i = 0; i < banList.ToS (); i++)
		fputs (reinterpret_cast<const char*> (banList [i]), cf.File ());
	cf.Close ();
	}
return 1;
}
//-----------------------------------------------------------------------------

int FindPlayerInBanList (char *szPlayer)
{
	int	i;

for (i = 0; i < banList.ToS (); i++)
	if (!strnicmp (reinterpret_cast<char*> (banList [i]), szPlayer, sizeof (tBanListEntry)))
		return 1;
return 0;
}

//-----------------------------------------------------------------------------

void FreeBanList (void)
{
banList.Destroy ();
}

//-----------------------------------------------------------------------------
