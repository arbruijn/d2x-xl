
#include <string.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif 

#include "descent.h"
#include "strutil.h"
#include "u_mem.h"
#include "cfile.h"
#include "banlist.h"

CBanList banList;

//-----------------------------------------------------------------------------

bool CBanList::Add (const char* szPlayer)
{
if (!szPlayer [0])
	return 1;
if (!Buffer ()) {
	Create ();
	}
CBanListEntry e;
return Push (e = szPlayer);
}

//-----------------------------------------------------------------------------

int32_t CBanList::Find (const char* szPlayer)
{
	uint32_t	i;

for (i = 0; i < ToS (); i++)
	if ((*this) [i] == szPlayer)
		return 1;
return 0;
}

//-----------------------------------------------------------------------------

int32_t CBanList::Load (void)
{
	CFile	cf;
	tBanListEntry	szPlayer;
	uint32_t i;

if (!cf.Open ("banlist.txt", gameFolders.game.szData [0], "rb", 0))
	return 0;
while (!feof (cf.File ())) {
	fgets (reinterpret_cast<char*> (szPlayer), sizeof (szPlayer), cf.File ());
	for (i = 0; i < sizeof (tBanListEntry); i++)
		if (szPlayer [i] == '\n')
			szPlayer [i] = '\0';
	szPlayer [CALLSIGN_LEN] = '\0';
	if (!Add (szPlayer))
		break;
	}
cf.Close ();
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBanList::Save (void)
{
if (Buffer () && ToS ()) {
	CFile	cf;
	uint32_t i;

	if (!cf.Open ("banlist.txt", gameFolders.game.szData [0], "wt", 0))
		return 0;
	for (i = 0; i < ToS (); i++)
		fputs (const_cast<char*> (m_data.buffer [i].m_entry), cf.File ());
	cf.Close ();
	}
return 1;
}
//-----------------------------------------------------------------------------

bool CBanList::Create (void)
{

if (!CStack<CBanListEntry>::Create (100, "BanList"))
	return false;
SetGrowth (100);
return true;
}

//-----------------------------------------------------------------------------
