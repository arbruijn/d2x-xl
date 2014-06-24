#ifndef _WIN32

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <stdio.h>

#include "cfile.h"
#include "findfile.h"
#include "error.h"

int FileFindNext (FILEFINDSTRUCT *ffsP, int nFlags)
{
	struct stat		statBuf;
	struct dirent	*deP;
	char				szFile [FILENAME_LEN];
	int				i;

while ((deP = readdir (ffsP->dirP))) {
	//PrintLog (0, "FileFindNext (readdir() = %s, %d)\n", deP->d_name, nFlags);
	if ((i = regexec (&ffsP->filter, deP->d_name, 0, 0, 0))) {
		//PrintLog (0, "regexec = %d\n", i);
		continue;
		}
	//PrintLog (0, "   regexec: match\n", deP->d_name);
	sprintf (szFile, "%s/%s", ffsP->szDir, deP->d_name);
	i = stat (szFile, &statBuf);
	//PrintLog (0, "   stat (%s) = %d [%d]\n", szFile, i, (statBuf.st_mode & S_IFDIR) ? FF_DIRECTORY : FF_NORMAL);
	i = (statBuf.st_mode & S_IFDIR) ? FF_DIRECTORY : FF_NORMAL;
	//PrintLog (0, "mode = %d, flags = %d\n", i, nFlags);
	if (nFlags != i)
		continue;
	strcpy (ffsP->name, deP->d_name);
	//PrintLog (0, "FileFindNext (%s [%c])\n", deP->d_name, (i == FF_NORMAL) ? 'f' : 'd');
	return 0;
	}
return -1;
}


int FileFindFirst (const char *pszFilter, FILEFINDSTRUCT *ffsP, int nFlags)
{
	char szFilter [FILENAME_LEN];
	int	i, j;
#ifdef __macosx__
  const int flags = REG_EXTENDED | REG_NOSUB | REG_ICASE;
#else
  const int flags = REG_EXTENDED | REG_NOSUB;
#endif
  
if (!pszFilter)
	return -1;
//PrintLog (1, "FileFindFirst (%s)\n", pszFilter);
memset (ffsP, 0, sizeof (*ffsP));
#if 1
if (*pszFilter && (*pszFilter != '.') && (*pszFilter != '/'))
	strcpy (ffsP->szDir, gameFolders.game.szRoot); // if only a file spec given, put standard game folder in front of it
else
	*ffsP->szDir = '\0';
strcat (ffsP->szDir, pszFilter);
#else
strcpy (ffsP->szDir, pszFilter);
#endif
for (i = (int) strlen (ffsP->szDir), j = (int) strlen (pszFilter); i; j--)
	if (ffsP->szDir [--i] == '/') 
		break;
ffsP->szDir [i] = '\0';

*ffsP->name = '\0';
//PrintLog (0, "opendir (%s)\n", ffsP->szDir);
if (!(ffsP->dirP = opendir (ffsP->szDir)))
	return -1;
if (!*pszFilter) // only looking for a folder
	return 0;
//PrintLog (0, "FileFindFirst (%s, %s)\n", ffsP->szDir, pszFilter);
// create regular search expression, "*" -> ".*", "." -> "\."
for (i = 0; pszFilter [j]; i++, j++) {
	if (pszFilter [j] == '*') {
		szFilter [i++] = '.';
		szFilter [i] = '*';
		}
	else if (pszFilter [j] == '.') {
		szFilter [i++] = '\\';
		szFilter [i] = '.';
		}
	else 
		szFilter [i] = pszFilter [j];
	}
szFilter [i] = '\0';
//PrintLog (0, "regcomp (%s)\n", szFilter);
// compile the regular expression szFilter and place the result in ffsP->filter
if ((i = regcomp (&ffsP->filter, szFilter, flags))) {
	char szError [200];
	regerror (i, &ffsP->filter, szError, 200);
	//PrintLog (-1, "regcomp : error %s\n", szError);
	return -1;
	}
return FileFindNext (ffsP, nFlags);
}


void FileFindClose (FILEFINDSTRUCT *ffsP)
{
if (ffsP->dirP)
	closedir (ffsP->dirP);
regfree (&ffsP->filter);
}

#endif //_WIN32
