#include "descent.h"
#include "cfile.h"
#include "IP2Country.h"

CStack<CIP2Country> ip2country;

//------------------------------------------------------------------------------

int LoadIP2Country (void)
{
	CFile	cf;

	int nRecords = cf.LineCount ("IpToCountry.csv", gameFolders.szDataDir [1], "#");

if (nRecords <= 0)
	return nRecords;

if (!ip2country.Create ((uint) nRecords))
	return -1;

char lineBuf [1024];

if (!cf.Open ("IpToCountry.csv", gameFolders.szDataDir [1], "rt", 0))
	return -1;

int minIP, maxIP;
char country [4];
char* bufP;

nRecords = 0;
country [3] = '\0';
while (cf.GetS (lineBuf, sizeof (lineBuf))) {
	if ((lineBuf [0] == '\0') || (lineBuf [0] == '#'))
		continue;
	if (!(bufP = strtok (lineBuf, ",")))
		continue;
	if (!(minIP = atoi (bufP + 1)))
		continue;
	if (!(bufP = strtok (NULL, ",")))
		continue;
	if (!(maxIP = atoi (bufP + 1)))
		continue;
	// skip 3 fields
	if (!strtok (NULL, ","))
		continue;
	if (!strtok (NULL, ","))
		continue;
	if (!strtok (NULL, ","))
		continue;
	if (!(bufP = strtok (NULL, ",")))
		continue;
	strncpy (country, bufP, sizeof (country) - 1);
	if (!strcmp (country, "ZZZ"))
		continue;
	if ((bufP = strtok (NULL, ",")) && !strcmp (bufP, "Reserved"))
		continue;
	ip2country.Push (CIP2Country (minIP, maxIP, country));
	}
cf.Close ();
if (ip2country.ToS ())
	ip2country.SortAscending ();
return (int) ip2country.ToS ();
}

//------------------------------------------------------------------------------

char* CountryFromIP (int ip)
{
CIP2Country key (ip, ip, "");
int i = ip2country.BinSearch (key);
return (i < 0) ? "n/a" : ip2country [i].m_country;
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

