#ifndef __findfile_h
#define __findfile_h

#ifdef _WIN32

#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)

// Empty file
typedef struct FILEFINDSTRUCT {
	uint32_t size;
	char name [256];
} FILEFINDSTRUCT;

int32_t FileFindFirst (const char *search_str, FILEFINDSTRUCT *ffstruct, int32_t bFindDirs);
int32_t FileFindNext (FILEFINDSTRUCT *ffstruct, int32_t bFindDirs);
int32_t FileFindClose (void);

typedef struct FILETIMESTRUCT {
	uint16_t date, time;
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

int32_t FileFindNext (FILEFINDSTRUCT *ffsP, int32_t nFlags);
int32_t FileFindFirst (const char *pszFilter, FILEFINDSTRUCT *ffsP, int32_t nFlags);
void FileFindClose (FILEFINDSTRUCT *ffsP);

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


