#ifndef __findfile_h
#define __findfile_h

#ifdef _WIN32

#include <windows.h>

// Empty file
typedef struct FILEFINDSTRUCT {
	unsigned long size;
	char name[256];
} FILEFINDSTRUCT;

int FileFindFirst(char *search_str, FILEFINDSTRUCT *ffstruct, int bFindDirs);
int FileFindNext(FILEFINDSTRUCT *ffstruct, int bFindDirs);
int FileFindClose(void);

typedef struct FILETIMESTRUCT {
	unsigned short date,time;
} FILETIMESTRUCT;

#else //!defined (_WIN32)

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <regex.h>

typedef struct FILEFINDSTRUCT {
	char 		name [256];
	char		szDir [256];
	char 		nType;
	DIR		*dirP;
	regex_t	filter;
} FILEFINDSTRUCT;

#define	FF_NORMAL		0
#define	FF_DIRECTORY	1

int FileFindNext (FILEFINDSTRUCT *ffsP, int nFlags);
int FileFindFirst (char *pszFilter, FILEFINDSTRUCT *ffsP, int nFlags);
void FileFindClose(FILEFINDSTRUCT *ffsP);

#endif //_WIN32

#	define	FFS					FILEFINDSTRUCT
#	define	FFF(_fn,_fs,_fi)	FileFindFirst (_fn, _fs, _fi)
#	define	FFN(_fs,_fi)		FileFindNext (_fs, _fi)
#ifdef _WIN32
#	define	FFC(_fs)				FileFindClose ()
#else
#	define	FFC(_fs)				FileFindClose (_fs)
#endif

#endif //__findfile_h


