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
	//LogErr ("FileFindNext (readdir() = %s, %d)\n", deP->d_name, nFlags);
	if ((i = regexec (&ffsP->filter, deP->d_name, 0, 0, 0))) {
		//LogErr ("regexec = %d\n", i);
		continue;
		}
	//LogErr ("   regexec: match\n", deP->d_name);
	sprintf (szFile, "%s/%s", ffsP->szDir, deP->d_name);
	i = stat (szFile, &statBuf);
	//LogErr ("   stat (%s) = %d [%d]\n", szFile, i, (statBuf.st_mode & S_IFDIR) ? FF_DIRECTORY : FF_NORMAL);
	i = (statBuf.st_mode & S_IFDIR) ? FF_DIRECTORY : FF_NORMAL;
	//LogErr ("mode = %d, flags = %d\n", i, nFlags);
	if (nFlags != i)
		continue;
	strcpy (ffsP->name, deP->d_name);
	//LogErr ("FileFindNext (%s [%c])\n", deP->d_name, (i == FF_NORMAL) ? 'f' : 'd');
	return 0;
	}
return -1;
}


int FileFindFirst (char *pszFilter, FILEFINDSTRUCT *ffsP, int nFlags)
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
//LogErr ("FileFindFirst (%s, %s)\n", gameFolders.szGameDir, pszFilter);
memset (ffsP, 0, sizeof (*ffsP));
if (*pszFilter && (*pszFilter != ',') && (*pszFilter != '/'))
	sprintf (ffsP->szDir, "%s/", gameFolders.szGameDir);
else
	*ffsP->szDir = '\0';
strcat (ffsP->szDir, pszFilter);
for (i = (int) strlen (ffsP->szDir), j = (int) strlen (pszFilter); i; j--)
	if (ffsP->szDir [--i] == '/')
		break;
ffsP->szDir [i] = '\0';
//LogErr ("FileFindFirst (%s, %s)\n", ffsP->szDir, pszFilter);
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
//LogErr ("regcomp (%s)\n", szFilter);
if ((i = regcomp (&ffsP->filter, szFilter, flags))) {
	char szError [200];
	regerror (i, &ffsP->filter, szError, 200);
	//LogErr ("regcomp : error %s\n", szError);
	return -1;
	}
//LogErr ("opendir (%s)\n", ffsP->szDir);
if (!(ffsP->dirP = opendir (ffsP->szDir)))
	return -1;
return FileFindNext (ffsP, nFlags);
}


void FileFindClose (FILEFINDSTRUCT *ffsP)
{
if (ffsP->dirP)
	closedir (ffsP->dirP);
regfree (&ffsP->filter);
}

#endif //_WIN32
