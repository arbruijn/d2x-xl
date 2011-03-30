#include "descent.h"
#include "cfile.h"
#include "IpToCountry.h"

CStack<CIpToCountry> ipToCountry;

//------------------------------------------------------------------------------

static inline bool ParseIP (char* buffer, uint& ip)
{
#if DBG
char* token = strtok (buffer, ",");
ip = atoi (token + 1);
if (ip == 0x7FFFFFFF)
	ip = ip;
#else
ip = atoi (strtok (buffer, "#") + 1);
#endif
return (ip > 0);
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
int h = cf.Size () - sizeof (int);
if ((h < 0) || (h / sizeof (CIpToCountry) < 1) || (h % sizeof (CIpToCountry) != 0)) {
	cf.Close ();
	return false;
	}

uint nRecords = (uint) cf.ReadInt ();

if (!ipToCountry.Create ((uint) nRecords))
	return false;

bool bSuccess = (ipToCountry.Read (cf, nRecords) == nRecords);
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
cf.WriteInt ((int) ipToCountry.ToS ());
bool bSuccess = (ipToCountry.Write (cf) == ipToCountry.Length ());
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

uint minIP, maxIP;
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
	if (!strncmp (country, "ZZ", 2))
		continue;
	//if (!strcmp (country, "ZZZ"))
	//	continue;
	if ((bufP = strtok (NULL, ",")) && !strcmp (bufP, "Reserved"))
		continue;
	if (minIP > maxIP)
		Swap (minIP, maxIP);
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

