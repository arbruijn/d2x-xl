#include "descent.h"
#include "cfile.h"
#include "IpToCountry.h"

CStack<CIpToCountry> ipToCountry;

//------------------------------------------------------------------------------

static inline bool ParseIP (char* buffer, int& ip)
{
#if DBG
char* token = strtok (buffer, ",");
ip = atoi (token);
if (ip == 0x7FFFFFFF)
	ip = ip;
#else
ip = atoi (strtok (buffer, "#"));
#endif
return (ip > 0) && (ip < 0x7FFFFFFF);
}

//------------------------------------------------------------------------------

int LoadIpToCountry (void)
{
	CFile	cf;

	int nRecords = cf.LineCount ("IpToCountry.csv", gameFolders.szDataDir [1], "#");

if (nRecords <= 0)
	return nRecords;

if (!ipToCountry.Create ((uint) nRecords))
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
	++nRecords;
	if ((lineBuf [0] == '\0') || (lineBuf [0] == '#'))
		continue;
	if (!(ParseIP (lineBuf, minIP) && ParseIP (NULL, maxIP)))
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
	ipToCountry.Push (CIpToCountry (minIP, maxIP, country));
	}
cf.Close ();
if (ipToCountry.ToS ())
	ipToCountry.SortAscending ();
return (int) ipToCountry.ToS ();
}

//------------------------------------------------------------------------------

char* CountryFromIP (int ip)
{
CIpToCountry key (ip, ip, "");
int i = ipToCountry.BinSearch (key);
return (i < 0) ? "n/a" : ipToCountry [i].m_country;
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

