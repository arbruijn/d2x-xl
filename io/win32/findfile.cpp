#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "cfile.h"
#include "findfile.h"
#include "u_mem.h"

//	Global Variables	----------------------------------------------------------

static HANDLE	_FindFileHandle = INVALID_HANDLE_VALUE;

#ifdef _WIN32_WCE
char *UnicodeToAsc (char *str, const wchar_t *w_str)
{
if (!w_str)
	return NULL;
int32_t len = wcslen (w_str) + 1;
if (WideCharToMultiByte (CP_ACP, 0, w_str, -1, str, len, NULL, NULL)) 
	return str;
return NULL;
}

// ----------------------------------------------------------------------------

wchar_t *AscToUnicode (wchar_t *w_str, const char *str)
{
	bool bAlloc = false;
if (!str)
	return NULL;
int32_t len = (int32_t) strlen (str) + 1;
if (!w_str) {
	bAlloc = true;
	w_str = new wchar_t [len];
	if (!w_str)
		return NULL;
	}
if (MultiByteToWideChar (CP_ACP, 0, str, -1, w_str, len))
	return w_str;
if (bAlloc)
	delete[] w_str;
return NULL;
}
#else
# define UnicodeToAsc(_as,_us) (_us)
# define AscToUnicode(_us,_as) ((LPSTR) _as)
#endif

// ----------------------------------------------------------------------------

int32_t FileFindFirst (const char *pszFilter, FILEFINDSTRUCT *ffstruct, int32_t bFindDirs)
{
	WIN32_FIND_DATA find;
	char		szFilter [FILENAME_LEN];
#ifdef _WIN32_WCE
	wchar_t	w_str [FILENAME_LEN];
	char		str [FILENAME_LEN];
#endif

find.dwFileAttributes = bFindDirs ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
if (bFindDirs) {
	int32_t l = (int32_t) strlen (pszFilter);
	if (!l)
		return 1;
	if ((pszFilter [l - 1] == '/') || (pszFilter [l - 1] == '\\')) {
		memcpy (szFilter, pszFilter, l - 1);
		szFilter [l - 1] = '\0';
		pszFilter = szFilter;
		}
	}
_FindFileHandle = FindFirstFile (AscToUnicode (w_str, pszFilter), &find);
if (_FindFileHandle == INVALID_HANDLE_VALUE) 
	return 1;
if (bFindDirs != ((find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0))
	return FileFindNext (ffstruct, bFindDirs);
ffstruct->size = find.nFileSizeLow;
strcpy (ffstruct->name, (UnicodeToAsc (str, find.cFileName)));
return 0; 
}

// ----------------------------------------------------------------------------

int32_t FileFindNext (FILEFINDSTRUCT *ffstruct, int32_t bFindDirs)
{
	WIN32_FIND_DATA find;
#ifdef _WIN32_WCE
	char		str [FILENAME_LEN];
#endif
	find.dwFileAttributes = bFindDirs ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	while (FindNextFile (_FindFileHandle, &find)) {
		if (bFindDirs == ((find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)) {
		ffstruct->size = find.nFileSizeLow;
		strcpy (ffstruct->name, UnicodeToAsc (str, find.cFileName));
		return 0;
	}
	}
return 1;
}
 
// ----------------------------------------------------------------------------

int32_t FileFindClose(void)
{
return FindClose (_FindFileHandle) ? 0 : 1;
}

// ----------------------------------------------------------------------------

#if 0
int32_t GetFileDateTime(int32_t filehandle, FILETIMESTRUCT *ftstruct)
{
	FILETIME filetime;
	int32_t retval;

	retval = GetFileTime((HANDLE)filehandle, NULL, NULL, &filetime);
	if (retval) {
		FileTimeToDosDateTime(&filetime, &ftstruct->date, &ftstruct->time);
		return 0;
	}
	else return 1;
}

// ----------------------------------------------------------------------------
//returns 0 if no error
int32_t SetFileDateTime(int32_t filehandle, FILETIMESTRUCT *ftstruct)
{
	FILETIME ft;
	int32_t retval;

	DosDateTimeToFileTime(ftstruct->date, ftstruct->time, &ft);
	retval = SetFileTime((HANDLE)filehandle, NULL, NULL, &ft);
	if (retval) return 0;
	else return 1;
}

#endif
// ----------------------------------------------------------------------------

