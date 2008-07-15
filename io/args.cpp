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
 COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "cfile.h"
#include "u_mem.h"
#include "strio.h"
#include "strutil.h"
#include "console.h"
#include "error.h"
#include "findfile.h"

int nArgCount = 0;

#define MAX_ARGS	1000

char * pszArgList [MAX_ARGS], *ps;

//------------------------------------------------------------------------------

int FindArg (const char * s)
{
	int i;
  
for (i = 0; i < nArgCount; i++)
	if (pszArgList [i] && *pszArgList [i] && !stricmp (pszArgList [i], s))
		return i;
return 0;
}

//------------------------------------------------------------------------------

void _CDECL_ args_exit (void)
{
	int i;

PrintLog ("unloading program arguments\n");
for (i = 0; i < nArgCount; i++)
	if (pszArgList [i])
		D2_FREE (pszArgList [i]);
memset (pszArgList, 0, sizeof (pszArgList));
nArgCount = 0;
}

//------------------------------------------------------------------------------

char *GetIniFileName (char *fnIni, int bDebug)
{
	int	i;

if ((i = FindArg ("-ini")))
	strncpy (fnIni, pszArgList [i + 1], sizeof (fnIni) - 1);
else {
#if defined(__unix__)
	FFS		ffs;
	strcpy (fnIni, gameFolders.szHomeDir);
	if (bDebug)
		strcat (fnIni, "/.d2x-xl-dbg");
	else
		strcat (fnIni, "/.d2x-xl");
	if (FFF (fnIni, &ffs, 0) <= 0) {
#endif
	strcpy (fnIni, gameFolders.szConfigDir);
	if (*fnIni)
		strcat (fnIni, "/");
	if (bDebug)
		strcat (fnIni, "d2xdebug.ini");
	else
		strcat (fnIni, "d2x.ini");
#if defined(__unix__)
   }
#endif //!__unix__
	}
return fnIni;
}

//------------------------------------------------------------------------------

void InitArgs (int argc, char **argv)
{
	int 		i, j;
	CFILE 	cf = {NULL, 0, 0, 0};
	char 		*pszLine, *pszToken, fnIni [FILENAME_LEN];
	static	char **pszArgs = NULL;
	static	int  nArgs = 0;

if (argv) {
	pszArgs = argv;
	nArgs = argc;
	}
else if (pszArgs) {
	argv = pszArgs;
	argc = nArgs;
	}
else
	return;
PrintLog ("Loading program arguments\n");
args_exit ();
for (i = 0; i < argc; i++)
	pszArgList [nArgCount++] = D2_STRDUP (argv [i]);

for (i = 0; i < nArgCount; i++)
	if (pszArgList [i] [0] == '-')
		strlwr (pszArgList [i]);  // Convert all args to lowercase

// look for the ini file
// for unix, allow both ~/.d2x-xl and <config dir>/d2x.ini
#ifdef _DEBUG
GetIniFileName (fnIni, 1);
#else
GetIniFileName (fnIni, 0);
#endif
CFOpen (&cf, fnIni, "", "rt", 0);
#ifdef _DEBUG
if (!cf.file) {
	GetIniFileName (fnIni, 0);
	CFOpen (&cf, fnIni, "", "rt", 0);
	}
#endif
if (cf.file) {
	while (!CFEoF (&cf)) {
		pszLine = fsplitword (&cf, '\n');
		if (*pszLine && (*pszLine != ';')) {
			pszToken = splitword (pszLine, ' ');
			if (nArgCount >= MAX_ARGS)
				break;
			pszArgList [nArgCount++] = pszToken;
			if (pszLine) {
				if (nArgCount >= MAX_ARGS) {
					PrintLog ("too many program arguments\n");
					break;
					}
				pszArgList [nArgCount++] = *pszLine ? D2_STRDUP (pszLine) : NULL;
				}
			}
		D2_FREE (pszLine); 
		}
	CFClose (&cf);
	}
PrintLog ("   ");
for (i = j = 0; i < nArgCount; i++, j++) {
	if (!pszArgList [i]) 
		continue;
	if ((pszArgList [i][0] == '-') && (isalpha (pszArgList [i][1]) || (j == 2))) {
		PrintLog ("\n   ");
		j = 0;
		}
	PrintLog (pszArgList [i]);
	PrintLog (" ");
	}
PrintLog ("\n");
atexit (args_exit);
}

// ----------------------------------------------------------------------------

int NumArg (int t, int nDefault)
{
	char *psz = pszArgList [t+1];

if (!psz)
	return nDefault;
if (psz && isdigit (*psz))
	return atoi (psz);
if (((*psz == '-') || (*psz == '+')) && isdigit (*psz + 1))
	return atoi (psz);
return nDefault;
}

//------------------------------------------------------------------------------
//eof
