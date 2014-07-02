#ifndef _CFILE_H
#define _CFILE_H

#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifndef _WIN32_WCE
#include <errno.h>
#endif

#include "maths.h"
#include "vecmat.h"

// the maximum length of a filename
#define FILENAME_LEN			255
#define SHORT_FILENAME_LEN 13
#define MAX_HOGFILES			300

class CFilename {
	public:
		char m_buffer [FILENAME_LEN + 1];
	public:
		CFilename () { m_buffer [0] = '\0'; }
		inline CFilename& operator= (CFilename& other) { 
			memcpy (m_buffer, other.m_buffer, sizeof (m_buffer)); 
			return *this;
			}
		inline CFilename& operator= (const char* other) { 
			strncpy (m_buffer, other, sizeof (m_buffer)); 
			return *this;
			}
		inline bool operator== (CFilename& other)
			{ return !strcmp (m_buffer, other.m_buffer); }
		inline bool operator< (CFilename& other)
			{ return strcmp (m_buffer, other.m_buffer) < 0; }
		inline bool operator> (CFilename& other)
			{ return strcmp (m_buffer, other.m_buffer) > 0; }
		inline operator const char*()
			{ return reinterpret_cast<char*> (&m_buffer [0]); }
	};


typedef struct CFILE {
	FILE		*file;
	char		*filename;
	size_t	size;
	size_t	libOffset;
	size_t	rawPosition;
	char		buffer [16384];
	int32_t		bufLen;
	int32_t		bufPos;
} CFILE;

class CFile {
	private:
		CFILE	m_info;
#ifdef _WIN32
		inline // g++ can be such a PITA
#endif
		int32_t FillBuffer (void);

	public:
		CFile () { Init (); }
		~CFile () { Close (); };
		void Init (void);
		int32_t Open (const char *filename, const char *folder, const char *mode, int32_t bUseD1Hog);
		size_t Length (void);							// Returns actual size of file...
		size_t Read (void *buf, size_t elsize, size_t nelem, int32_t bCompressed = 0);
		int32_t Close (void);
		size_t Size (const char *hogname, const char *folder, int32_t bUseD1Hog);
		size_t Seek (long offset, int32_t where);
		size_t Tell (void);
		char *GetS (char *buf, size_t n);
		int32_t EoF (void);
		int32_t Error (void);
		size_t Write (const void *buf, int32_t elsize, int32_t nelem, int32_t bCompressed = 0);
		inline int32_t GetC (void) { return (FillBuffer () == EOF) ? EOF : m_info.buffer [m_info.bufPos++]; }

		size_t ReadCompressed (const void* buf, uint32_t bufLen);
		size_t WriteCompressed (const void* buf, uint32_t bufLen);

		int32_t PutC (int32_t c);
		int32_t PutS (const char *str);

		inline size_t Size (void) { return m_info.size; }

		// prototypes for reading basic types from fp
		int32_t ReadInt (void);
		uint32_t ReadUInt (void);
		int16_t ReadShort (void);
		uint16_t ReadUShort (void);
		int8_t ReadByte (void);
		uint8_t ReadUByte (void);
		fix ReadFix (void);
		fixang ReadFixAng (void);
		void ReadVector (CFixVector& v);
		void ReadVector (CFloatVector3& v);
		void ReadAngVec (CAngleVector& v);
		void ReadMatrix (CFixMatrix& v);
		float ReadFloat (void);
		double ReadDouble (void);
		void ReadString (char *buf, int32_t n);
		char *ReadData (const char *filename, const char *folder, int32_t bUseD1Hog);

		int32_t WriteFix (fix x);
		int32_t WriteInt (int32_t i);
		int32_t WriteShort (int16_t s);
		int32_t WriteByte (int8_t u);
		int32_t WriteFixAng (fixang a);
		int32_t WriteFloat (float f);
		int32_t WriteDouble (double d);
		void WriteAngVec (const CAngleVector& v);
		void WriteVector (const CFixVector& v);
		void WriteVector (const CFloatVector3& v);
		void WriteMatrix (const CFixMatrix& m);
		int32_t WriteString (const char *buf);

		int32_t Copy (const char *pszSrc, const char *pszDest);
		int32_t Extract (const char *filename, const char *folder, int32_t bUseD1Hog, const char *szDest);
		time_t Date (const char *filename, const char *folder, int32_t bUseD1Hog);

		static int32_t Exist (const char *filename, const char *folder, int32_t bUseD1Hog);	// Returns true if file exists on disk (1) or in hog (2).
		static int32_t Delete (const char *filename, const char* folder);
		static int32_t Rename (const char *oldname, const char *newname, const char *folder);
		static int32_t MkDir (const char *pathname);
		static FILE *GetFileHandle (const char *filename, const char *folder, const char *mode);
		static void SplitPath (const char *szFullPath, char *szFolder, char *szFile, char *szExt);
		static void ChangeFilenameExtension (char *dest, const char *src, const char *new_ext);

		inline FILE*& File () { return m_info.file; }
		inline char* Name () { return m_info.filename; }
		int32_t LineCount (const char* filename, const char* folder, const char* delims);
	};

typedef struct tStaticFolders {
	char	szRoot [FILENAME_LEN];
	char	szData [2][FILENAME_LEN];
	char	szTextures [3][FILENAME_LEN];
	char	szModels [FILENAME_LEN];
	char	szSounds [5][FILENAME_LEN];
	char	szMusic [2][FILENAME_LEN];
	char	szMovies [FILENAME_LEN];
	char	szShaders [FILENAME_LEN];
	char	szAltHogs [FILENAME_LEN];
	} tStaticFolders;

typedef struct tSharedFolders {
	char	szRoot [FILENAME_LEN];
	char	szCache [FILENAME_LEN];
	char	szTextures [5][FILENAME_LEN];
	char	szModels [3][FILENAME_LEN];
	char	szLightmaps [FILENAME_LEN];
	char	szLightData [FILENAME_LEN];
	char	szMeshes [FILENAME_LEN];
	char	szMods [FILENAME_LEN];
	char	szDownloads [FILENAME_LEN];
	} tSharedFolders;

typedef struct tPrivateFolders {
	char	szRoot [FILENAME_LEN];
	char	szCache [FILENAME_LEN];
	char	szConfig [FILENAME_LEN];
	char	szProfiles [FILENAME_LEN];
	char	szSavegames [FILENAME_LEN];
	char	szScreenshots [FILENAME_LEN];
	char	szDemos [FILENAME_LEN];
	char	szWallpapers [FILENAME_LEN];
	} tPrivateFolders;

typedef struct tModFolders {
	char	szRoot [FILENAME_LEN];
	char	szCache [FILENAME_LEN];
	char	szCurrent [FILENAME_LEN];
	char	szName [FILENAME_LEN];
	char	szTextures [2][FILENAME_LEN];
	char	szSounds [2][FILENAME_LEN];
	char	szModels [2][FILENAME_LEN];
	char	szWallpapers [FILENAME_LEN];
	char	szMusic [FILENAME_LEN];
	char	szLevel [FILENAME_LEN];
	} tModFolders;

typedef struct tMissionFolders {
	char	szRoot [FILENAME_LEN];
	char	szCache [FILENAME_LEN];
	char	szCurrent [2][FILENAME_LEN];
	char	szSubFolder [FILENAME_LEN];
	char	szStates [FILENAME_LEN];
	char	szDownloads [FILENAME_LEN];
	} tMissionFolders;

typedef struct tGameFolders {
	tModFolders			mods;
	tMissionFolders	missions;
	tStaticFolders		game;
	tSharedFolders		var;
	tPrivateFolders	user;
#if defined (__unix__) || defined (__macosx__)
	char szSharePath [FILENAME_LEN];
#endif
	int32_t bAltHogDirInited;
} tGameFolders;

int32_t GetAppFolder (const char *szMainFolder, char *szDestFolder, const char *szSubFolder, const char *szFilter);
char *GameDataFilename (char *pszFilename, const char *pszExt, int32_t nLevel, int32_t nType);
int32_t MakeTexSubFolders (char* pszParentFolder);
void MakeModFolders (const char* pszMission, int32_t nLevel = 0);
void ResetModFolders (void);
char* LevelFolder (int32_t nLevel);
char* FlipBackslash (char* pszFile);
char* FlipSlash (char* pszFile);
char* AppendSlash (char* pszFile);

#ifdef _WIN32
char *UnicodeToAsc (char *str, const wchar_t *w_str);
wchar_t *AscToUnicode (wchar_t *w_str, const char *str);
#endif

extern int32_t nCFileError;
extern tGameFolders	gameFolders;

#ifdef _WIN32_WCE
# define errno -1
# define strerror (x) "Unknown Error"
#endif

#endif
