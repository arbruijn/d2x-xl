#include "descent.h"
#include "cfile.h"
#include "IpToCountry.h"

CStack<CIpToCountry> ipToCountry;

//------------------------------------------------------------------------------

static inline bool ParseIP (char* buffer, int& ip)
{
#if DBG
char* token = strtok (buffer, ",");
ip = atoi (token + 1);
if (ip == 0x7FFFFFFF)
	ip = ip;
#else
ip = atoi (strtok (buffer, "#") + 1);
#endif
return (ip > 0) && (ip < 0x7FFFFFFF);
}

//------------------------------------------------------------------------------

static inline bool Skip (int nFields)
{
while (nFields--)
	if (!strtok (NULL, ","))
		return false;
return true;
}

//------------------------------------------------------------------------------

static bool LoadBinary (void)
{
	CFile	cf;

if (!cf.Exist ("IpToCountry.bin", gameFolders.szCacheDir, 0))
	return false;
if (cf.Date ("IpToCountry.csv", gameFolders.szDataDir [1], 0) > cf.Date ("IpToCountry.bin", gameFolders.szCacheDir, 0))
	return false;
if (!cf.Open ("IpToCountry.bin", gameFolders.szCacheDir, "wb", 0))
	return false;

int nRecords = cf.Size () / sizeof (CIpToCountry);

if (!ipToCountry.Create ((uint) nRecords))
	return false;

bool bSuccess = (ipToCountry.Read (cf, nRecords) == cf.Size ());
if (bSuccess)
	ipToCountry.Grow ((uint) nRecords);
else
	ipToCountry.Destroy ();

cf.Close ();
return bSuccess;
}

//------------------------------------------------------------------------------

static bool SaveBinary (void)
{
	CFile	cf;

if (!cf.Open ("IpToCountry.bin", gameFolders.szCacheDir, "wb", 0))
	return false;
bool bSuccess = (ipToCountry.Write (cf) == ipToCountry.Size ());
if (!cf.Close ())
	bSuccess = false;
return bSuccess;
}

//------------------------------------------------------------------------------

int LoadIpToCountry (void)
{
if (LoadBinary ())
	return ipToCountry.Length ();

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
	if (!Skip (3))
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

SaveBinary ();

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

