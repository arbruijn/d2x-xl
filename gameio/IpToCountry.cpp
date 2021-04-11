#include "descent.h"
#include "cfile.h"
#include "text.h"
#include "menu.h"
#include "IpToCountry.h"

CStack<CIpToCountry> ipToCountry;

//------------------------------------------------------------------------------

static inline bool ParseIP (char* buffer, uint32_t& ip)
{
char* token = strtok (buffer, ",") + 1;
if (!token)
	return false;
#if 1 //DBG
for (ip = 0; isdigit (*token); token++)
	ip = 10 * ip + uint32_t (*token - '0');
#else
ip = strtoul (token, NULL, 10);
#endif
return true;
}

//------------------------------------------------------------------------------

static inline bool Skip (int32_t nFields)
{
while (nFields--)
	if (!strtok (NULL, ","))
		return false;
return true;
}

//------------------------------------------------------------------------------

static bool LoadBinary (time_t t0, time_t t1)
{
	CFile		cf;

time_t tBIN = cf.Date ("IpToCountry.bin", gameFolders.var.szCache, 0);
if ((tBIN < 0) || ((t1 > 0) && (tBIN < t1)) || ((t0 > 0) && (tBIN < t0)))
	return false;

if (!cf.Open ("IpToCountry.bin", gameFolders.var.szCache, "rb", 0))
	return false;
int32_t h = (int32_t) cf.Size () - sizeof (int32_t);
if ((h < 0) || (h / sizeof (CIpToCountry) < 1) || (h % sizeof (CIpToCountry) != 0)) {
	cf.Close ();
	return false;
	}

uint32_t nRecords = (uint32_t) cf.ReadInt ();

if (!ipToCountry.Create ((uint32_t) nRecords, "ipToCountry"))
	return false;

bool bSuccess = (ipToCountry.Read (cf, nRecords) == nRecords);
if (bSuccess)
	ipToCountry.Grow ((uint32_t) nRecords);
else
	ipToCountry.Destroy ();

cf.Close ();
return bSuccess;
}

//------------------------------------------------------------------------------

static bool SaveBinary (void)
{
	CFile	cf;

if (!cf.Open ("IpToCountry.bin", gameFolders.var.szCache, "wb", 0))
	return false;
cf.WriteInt ((int32_t) ipToCountry.ToS ());
bool bSuccess = (ipToCountry.Write (cf) == ipToCountry.Length ());
if (cf.Close ())
	bSuccess = false;
return bSuccess;
}

//------------------------------------------------------------------------------

static CFile cf;

int32_t ReadIpToCountryRecord (void)
{
char lineBuf [1024];
char* token;
uint32_t minIP, maxIP;
char country [4];

country [3] = '\0';
if (!cf.GetS (lineBuf, sizeof (lineBuf)))
	return 0;
#if 1
if ((lineBuf [0] == '\0') || (lineBuf [0] == '#'))
	return 1;
if (!(ParseIP (lineBuf, minIP) && ParseIP (NULL, maxIP)))
	return 1;
// skip 3 fields
if (!Skip (3))
	return 1;
if (!(token = strtok (NULL, ",")))
	return 1;
strncpy (country, token + 1, sizeof (country) - 1);
if (!strncmp (country, "ZZ", 2))
	return 1;
//if (!strcmp (country, "ZZZ"))
//	return 1;
if (!(token = strtok (NULL, ",")))
	return 1;
if (!strcmp (token, "\"Reserved\""))
	return 1;
if (minIP > maxIP)
	Swap (minIP, maxIP);
ipToCountry.Push (CIpToCountry (minIP, maxIP, country));
#endif
return 1;
}

//------------------------------------------------------------------------------

#if 0

static int32_t nProgressStep;

static int32_t LoadIpToCountryPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (!ReadIpToCountryRecord ())
	key = -2;
else {
	if (++(menu [0].Value ()) % nProgressStep == 0)
		menu [0].Rebuild ();
	key = 0;
	}
return nCurItem;
}

#endif

//------------------------------------------------------------------------------

int32_t LoadIpToCountry (void)
{
time_t t0 = cf.Date ("IpToCountry-Default.csv", gameFolders.game.szData [1], 0);
time_t t1 = cf.Date ("IpToCountry.csv", gameFolders.game.szData [1], 0);

if (LoadBinary (t0, t1))
	return ipToCountry.Length ();

const char* pszFile;

if ((t0 < 0) && (t1 < 0))
	return -1;
if (t0 < 0) // default file not present
	pszFile = "IpToCountry.csv";
else if (t1 < 0)  // user supplied file not present
	pszFile = "IpToCountry-Default.csv";
else if (t1 > t0) // user supplied file newer than default file
	pszFile = "IpToCountry.csv";
else // default file newer than user supplied file
	pszFile = "IpToCountry-Default.csv";

int32_t nRecords = cf.LineCount (pszFile, gameFolders.game.szData [1], "#");
if (nRecords <= 0)
	return nRecords;

if (!ipToCountry.Create ((uint32_t) nRecords))
	return -1;

if (!cf.Open (pszFile, gameFolders.game.szData [1], "rb", 0))
	return -1;

#if 0 //slows it down too much
if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
	nProgressStep = (nRecords + 99) / 100;
	ProgressBar (TXT_LOADING_IPTOCOUNTRY, 1, 0, nRecords, LoadIpToCountryPoll); 
	}
#endif
else {
	while (ReadIpToCountryRecord ())
		;
	}

cf.Close ();
if (ipToCountry.ToS ())
	ipToCountry.SortAscending ();

SaveBinary ();

return (int32_t) ipToCountry.ToS ();
}

//------------------------------------------------------------------------------

const char* CountryFromIP (uint32_t ip)
{
uint32_t l = 0, r = ipToCountry.ToS () - 1, i;
do {
	i = (l + r) / 2;
	if (ipToCountry [i] < ip)
		l = i + 1;
	else if (ipToCountry [i] > ip)
		r = i - 1;
	else
		break;
	} while (l <= r);

if (l > r)
	return "n/a";

// find first record with equal key
for (; i > 0; i--)
	if (ipToCountry [i - 1] != ip)
		break;

int32_t h = i;
int32_t dMin = ipToCountry [i].Range ();
// Find smallest IP range containing key
while (ipToCountry [++i] == ip) {
	int32_t d = ipToCountry [i].Range ();
	if (dMin > d) {
		dMin = d;
		h = i;
		}
	}
return ipToCountry [h].m_country;
}

//------------------------------------------------------------------------------

