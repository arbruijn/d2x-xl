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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#if defined (_WIN32_WCE) || defined (_WIN32)
#	include <windows.h>
#	include <sys/stat.h>
#else
#	include <sys/stat.h>
#	include <errno.h>
#endif

#include "inferno.h"
#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "d_io.h"
#include "error.h"
#include "cfile.h"
#include "hogfile.h"
#include "byteswap.h"
#include "mission.h"
#include "console.h"
#include "findfile.h"
#include "text.h"

#define SORT_HOGFILES 1

int nCFileError = 0;

tGameFolders gameFolders;

// ----------------------------------------------------------------------------

int GetAppFolder (const char *szRootDir, char *szFolder, const char *szName, const char *szFilter)
{
	FFS	ffs;
	char	szDir [FILENAME_LEN];
	int	i, bAddSlash;

if (! (szName && *szName))
	return 1;
i = (int) strlen (szRootDir);
bAddSlash = i && (szRootDir [i-1] != '\\') && (szRootDir [i-1] != '/');
PrintLog ("GetAppFolder ('%s', '%s', '%s', '%s')\n", szRootDir, szFolder, szName, szFilter);
sprintf (szDir, "%s%s%s%s%s", szRootDir, bAddSlash ? "/" : "", szName, *szFilter ? "/" : "", szFilter);
if (! (i = FFF (szDir, &ffs, *szFilter == '\0'))) {
	if (szFolder != szName)
		sprintf (szFolder, "%s%s%s", szRootDir, bAddSlash ? "/" : "", szName);
	}
else if (*szRootDir)
	strcpy (szFolder, szRootDir);
PrintLog ("GetAppFolder (%s) = '%s' (%d)\n", szName, szFolder, i);
FFC (&ffs);
return i;
}

// ----------------------------------------------------------------------------

void SplitPath (const char *szFullPath, char *szFolder, char *szFile, char *szExt)
{
	int	h = 0, i, j, l = (int) strlen (szFullPath) - 1;

i = l;
#ifdef _WIN32
while ((i >= 0) && (szFullPath [i] != '/') && (szFullPath [i] != '\\') && (szFullPath [i] != ':'))
#else
while ((i >= 0) && (szFullPath [i] != '/'))
#endif
	i--;
i++;
j = l - 1;
while ((j > i) && (szFullPath [j] != '.'))
	j--;
if (szFolder) {
	if (i > 0)
		strncpy (szFolder, szFullPath, i);
	szFolder [i] = '\0';
	}
if (szFile) {
	if (!j || (j > i))
		strncpy (szFile, szFullPath + i, h = (j ? j : l + 1) - i);
	szFile [h] = '\0';
	}
if (szExt) {
	if (j && (j < l))
		strncpy (szExt, szFullPath + j, l - j);
	szExt [l - j] = '\0';
	}
}

//------------------------------------------------------------------------------

void CFile::ChangeFilenameExtension (char *dest, const char *src, const char *newExt)
{
	int i;

strcpy (dest, src);
if (newExt [0] == '.')
	newExt++;
for (i = 1; i < (int) strlen (dest); i++)
	if ((dest [i] == '.') || (dest [i] == ' ') || (dest [i] == 0))
		break;
if (i < FILENAME_LEN - 5) {
	dest [i] = '.';
	dest [i+1] = newExt [0];
	dest [i+2] = newExt [1];
	dest [i+3] = newExt [2];
	dest [i+4] = 0;
	}
}

//------------------------------------------------------------------------------

char *GameDataFilename (char *pszFilename, const char *pszExt, int nLevel, int nType)
{
	char	szFilename [FILENAME_LEN];

SplitPath (*hogFileManager.AltFiles ().szName ? hogFileManager.AltFiles ().szName : 
			  gameStates.app.bD1Mission ? hogFileManager.D1Files ().szName : hogFileManager.D2Files ().szName, 
			  NULL, szFilename, NULL);
if (nType < 0) {
	if (nLevel < 0)
		sprintf (pszFilename, "%s-s%d.%s", szFilename, -nLevel, pszExt);
	else
		sprintf (pszFilename, "%s-%d.%s", szFilename, nLevel, pszExt);
	}
else {
	if (nLevel < 0)
		sprintf (pszFilename, "%s-s%d.%s%d", szFilename, -nLevel, pszExt, nType);
	else
		sprintf (pszFilename, "%s-%d.%s%d", szFilename, nLevel, pszExt, nType);
	}
return pszFilename;
}

// ----------------------------------------------------------------------------

FILE *CFile::GetFileHandle (const char *filename, const char *folder, const char *mode) 
{
	FILE	*fp;
	char	fn [FILENAME_LEN];
	const char *pfn;

if (!*filename || (strlen (filename) + strlen (folder) >= FILENAME_LEN)) {
	return NULL;
	}
if ((*filename != '/') && (strstr (filename, "./") != filename) && *folder) {
	sprintf (fn, "%s/%s", folder, filename);
   pfn = fn;
	}
 else
 	pfn = filename;
 
fp = fopen (pfn, mode);
if (!fp && gameFolders.bAltHogDirInited && strcmp (folder, gameFolders.szAltHogDir)) {
   sprintf (fn, "%s/%s", gameFolders.szAltHogDir);
   pfn = fn;
   fp = fopen (pfn, mode);
	}
//if (!fp) PrintLog ("CFGetFileHandle (): error opening %s\n", pfn);
return fp;
}

// ----------------------------------------------------------------------------

int CFile::Size (const char *hogname, const char *folder, int bUseD1Hog)
{
#if ! (defined (_WIN32_WCE) || defined (_WIN32))
	struct stat statbuf;

//	sprintf (fn, "%s/%s", folder, hogname);
if (!Open (hogname, gameFolders.szDataDir, "rb", bUseD1Hog))
	return -1;
#ifdef _WIN32
fstat (_fileno (m_cf.file), &statbuf);
#else
fstat (fileno (m_cf.file), &statbuf);
#endif
Close ();
return statbuf.st_size;
#else
	DWORD size;

//sprintf (fn, "%s%s%s", folder, *folder ? "/" : "", hogname);
if (!Open (hogname, gameFolders.szDataDir, "rb", bUseD1Hog))
	return -1;
size = m_cf.size;
Close ();
return size;
#endif
}

// ----------------------------------------------------------------------------
// CFile::EoF () Tests for end-of-file on a stream
//
// returns a nonzero value after the first read operation that attempts to read
// past the end of the file. It returns 0 if the current position is not end of file.
// There is no error return.

int CFile::EoF (void)
{
#if DBG
if (!m_cf.file)
	return 1;
#endif
return (m_cf.rawPosition >= m_cf.size);
}

// ----------------------------------------------------------------------------

int CFile::Error (void)
{
return ferror (m_cf.file);
}

// ----------------------------------------------------------------------------

int CFile::Exist (const char *filename, const char *folder, int bUseD1Hog) 
{
	int	length, bNoHOG = 0;
	FILE	*fp;
	char	*pfn = const_cast<char*> (filename);

if (*pfn == '\x01') 
	pfn++;
else {
	bNoHOG = (*pfn == '\x02');
	if ((fp = GetFileHandle (pfn + bNoHOG, folder, "rb"))) { // Check for non-hogP file first...
		fclose (fp);
		return 1;
		}
	if (bNoHOG)
		return 0;
	}
if ((fp = hogFileManager.Find (pfn, &length, bUseD1Hog))) {
	fclose (fp);
	return 2;		// file found in hogP
	}
return 0;		// Couldn't find it.
}

// ----------------------------------------------------------------------------
// Deletes a file.
int CFile::Delete (const char *filename, const char* folder)
{
	char	fn [FILENAME_LEN];

sprintf (fn, "%s%s%s", folder, *folder ? "/" : "", filename);
#ifndef _WIN32_WCE
	return remove (fn);
#else
	return !DeleteFile (fn);
#endif
}

// ----------------------------------------------------------------------------
// Rename a file.
int CFile::Rename (const char *oldname, const char *newname, const char *folder)
{
	char	fno [FILENAME_LEN], fnn [FILENAME_LEN];

sprintf (fno, "%s%s%s", folder, *folder ? "/" : "", oldname);
sprintf (fnn, "%s%s%s", folder, *folder ? "/" : "", newname);
#ifndef _WIN32_WCE
	return rename (fno, fnn);
#else
	return !MoveFile (fno, fnn);
#endif
}

// ----------------------------------------------------------------------------
// Make a directory.
int CFile::MkDir (const char *pathname)
{
#if defined (_WIN32_WCE) || defined (_WIN32)
return !CreateDirectory (pathname, NULL);
#else
return mkdir (pathname, 0755);
#endif
}

// ----------------------------------------------------------------------------

int CFile::Open (const char *filename, const char *folder, const char *mode, int bUseD1Hog) 
{
	int	length = -1;
	FILE	*fp = NULL;
	const char	*pszHogExt, *pszFileExt;

m_cf.file = NULL;
if (!(filename && *filename))
	return 0;
if ((*filename != '\x01') /*&& !bUseD1Hog*/) {
	fp = GetFileHandle (filename, folder, mode);		// Check for non-hogP file first...
	if (!fp && 
		 ((pszFileExt = strstr (filename, ".rdl")) || (pszFileExt = strstr (filename, ".rl2"))) &&
		 (pszHogExt = strchr (hogFileManager.AltHogFile (), '.')) &&
		 !stricmp (pszFileExt, pszHogExt))
		fp = GetFileHandle (hogFileManager.AltHogFile (), folder, mode);		// Check for non-hogP file first...
	}
else {
	fp = NULL;		//don't look in dir, only in tHogFile
	filename++;
	}

if (!fp) {
	if ((fp = hogFileManager.Find (filename, &length, bUseD1Hog)))
		if (stricmp (mode, "rb")) {
			::Error ("Cannot read hogP file\n (wrong file io mode).\n");
			return 0;
			}
	}
if (!fp) 
	return 0;
m_cf.file = fp;
m_cf.rawPosition = 0;
m_cf.size = (length < 0) ? ffilelength (fp) : length;
m_cf.libOffset = (length < 0) ? 0 : ftell (fp);
m_cf.filename = const_cast<char*> (filename);
return 1;
}

// ----------------------------------------------------------------------------

void CFile::Init (void) 
{
memset (&m_cf, 0, sizeof (m_cf)); 
m_cf.rawPosition = -1; 
}

// ----------------------------------------------------------------------------

int CFile::Length (void) 
{
return m_cf.size;
}

// ----------------------------------------------------------------------------
// Write () writes to the file
//
// returns:   number of full elements actually written
//
//
int CFile::Write (const void *buf, int nElemSize, int nElemCount)
{
	int nWritten;

if (!m_cf.file) {
	return 0;
	}
nWritten = (int) fwrite (buf, nElemSize, nElemCount, m_cf.file);
m_cf.rawPosition = ftell (m_cf.file);
return nWritten;
}

// ----------------------------------------------------------------------------
// CFile::PutC () writes a character to a file
//
// returns:   success ==> returns character written
//            error   ==> EOF
//
int CFile::PutC (int c)
{
	int char_written;

char_written = fputc (c, m_cf.file);
m_cf.rawPosition = ftell (m_cf.file);
return char_written;
}

// ----------------------------------------------------------------------------

int CFile::GetC (void) 
{
	int c;

if (m_cf.rawPosition >= m_cf.size) 
	return EOF;
c = getc (m_cf.file);
if (c != EOF)
	m_cf.rawPosition = ftell (m_cf.file) - m_cf.libOffset;
return c;
}

// ----------------------------------------------------------------------------
// CFile::PutS () writes a string to a file
//
// returns:   success ==> non-negative value
//            error   ==> EOF
//
int CFile::PutS (const char *str)
{
	int ret;

ret = fputs (str, m_cf.file);
m_cf.rawPosition = ftell (m_cf.file);
return ret;
}

// ----------------------------------------------------------------------------

char * CFile::GetS (char * buf, size_t n) 
{
	char * t = buf;
	size_t i;
	int c;

for (i = 0; i < n - 1; i++) {
	do {
		if (m_cf.rawPosition >= m_cf.size) {
			*buf = 0;
			return (buf == t) ? NULL : t;
			}
		c = GetC ();
		if (c == 0 || c == 10)       // Unix line ending
			break;
		if (c == 13) {      // Mac or DOS line ending
			int c1 = GetC ();
			Seek ( -1, SEEK_CUR);
			if (c1 == 10) // DOS line ending
				continue;
			else            // Mac line ending
				break;
			}
		} while (c == 13);
 	if (c == 0 || c == 10 || c == 13)  // because cr-lf is a bad thing on the mac
 		break;   // and anyway -- 0xod is CR on mac, not 0x0a
	*buf++ = c;
	if (c == '\n')
		break;
	}
*buf++ = 0;
return  t;
}

// ----------------------------------------------------------------------------

size_t CFile::Read (void *buf, size_t elsize, size_t nelem) 
{
uint i, size = (int) (elsize * nelem);

if (!m_cf.file || (m_cf.size < 1)) 
	return 0;
i = (int) fread (buf, 1, size, m_cf.file);
m_cf.rawPosition += i;
return i / elsize;
}


// ----------------------------------------------------------------------------

int CFile::Tell (void) 
{
return m_cf.rawPosition;
}

// ----------------------------------------------------------------------------

int CFile::Seek (long int offset, int where) 
{
if (!m_cf.size)
	return -1;

	int destPos;

switch (where) {
	case SEEK_SET:
		destPos = offset;
		break;
	case SEEK_CUR:
		destPos = m_cf.rawPosition + offset;
		break;
	case SEEK_END:
		destPos = m_cf.size + offset;
		break;
	default:
		return 1;
	}
int c = fseek (m_cf.file, m_cf.libOffset + destPos, SEEK_SET);
m_cf.rawPosition = ftell (m_cf.file) - m_cf.libOffset;
return c;
}

// ----------------------------------------------------------------------------

int CFile::Close (void)
{
	int result;

if (!m_cf.file)
	return 0;
result = fclose (m_cf.file);
m_cf.file = NULL;
m_cf.size = 0;
m_cf.rawPosition = -1;
return result;
}

// ----------------------------------------------------------------------------
// routines to read basic data types from CFile::ILE's.  Put here to
// simplify mac/pc reading from cfiles.

int CFile::ReadInt (void)
{
	int32_t i;

Read (&i, sizeof (i), 1);
//Error ("Error reading int in CFile::ReadInt ()");
return INTEL_INT (i);
}

// ----------------------------------------------------------------------------

short CFile::ReadShort (void)
{
	int16_t s;

Read (&s, sizeof (s), 1);
//Error ("Error reading short in CFile::ReadShort ()");
return INTEL_SHORT (s);
}

// ----------------------------------------------------------------------------

sbyte CFile::ReadByte (void)
{
	sbyte b;

if (Read (&b, sizeof (b), 1) != 1)
	return nCFileError;
//Error ("Error reading byte in CFile::ReadByte ()");
return b;
}

// ----------------------------------------------------------------------------

float CFile::ReadFloat (void)
{
	float f;

Read (&f, sizeof (f), 1) ;
//Error ("Error reading float in CFile::ReadFloat ()");
return INTEL_FLOAT (f);
}

// ----------------------------------------------------------------------------
//Read and return a double (64 bits)
//Throws an exception of nType (nCFileError *) if the OS returns an error on read
double CFile::ReadDouble (void)
{
	double d;

Read (&d, sizeof (d), 1);
return INTEL_DOUBLE (d);
}

// ----------------------------------------------------------------------------

fix CFile::ReadFix (void)
{
	fix f;

Read (&f, sizeof (f), 1) ;
//Error ("Error reading fix in CFile::ReadFix ()");
return (fix) INTEL_INT ((int) f);
return f;
}

// ----------------------------------------------------------------------------

fixang CFile::ReadFixAng (void)
{
	fixang f;

Read (&f, 2, 1);
//Error ("Error reading fixang in CFile::ReadFixAng ()");
return (fixang) INTEL_SHORT ((int) f);
}

// ----------------------------------------------------------------------------

void CFile::ReadVector (CFixVector& v) 
{
v [X] = ReadFix ();
v [Y] = ReadFix ();
v [Z] = ReadFix ();
}

// ----------------------------------------------------------------------------

void CFile::ReadAngVec (vmsAngVec& v)
{
v [PA] = ReadFixAng ();
v [BA] = ReadFixAng ();
v [HA] = ReadFixAng ();
}

// ----------------------------------------------------------------------------

void CFile::ReadMatrix (vmsMatrix& m)
{
ReadVector (m [RVEC]);
ReadVector (m [UVEC]);
ReadVector (m [FVEC]);
}


// ----------------------------------------------------------------------------

void CFile::ReadString (char *buf, int n)
{
	char c;

do {
	c = (char) ReadByte ();
	if (n > 0) {
		*buf++ = c;
		n--;
		}
	} while (c != 0);
}

// ----------------------------------------------------------------------------
// equivalent write functions of above read functions follow

int CFile::WriteInt (int i)
{
i = INTEL_INT (i);
return Write (&i, sizeof (i), 1);
}

// ----------------------------------------------------------------------------

int CFile::WriteShort (short s)
{
s = INTEL_SHORT (s);
return Write(&s, sizeof (s), 1);
}

// ----------------------------------------------------------------------------

int CFile::WriteByte (sbyte b)
{
return Write (&b, sizeof (b), 1);
}

// ----------------------------------------------------------------------------

int CFile::WriteFloat (float f)
{
f = INTEL_FLOAT (f);
return Write (&f, sizeof (f), 1);
}

// ----------------------------------------------------------------------------
//Read and return a double (64 bits)
//Throws an exception of nType (nCFileError *) if the OS returns an error on read
int CFile::WriteDouble (double d)
{
d = INTEL_DOUBLE (d);
return Write (&d, sizeof (d), 1);
}

// ----------------------------------------------------------------------------

int CFile::WriteFix (fix x)
{
x = INTEL_INT (x);
return Write (&x, sizeof (x), 1);
}

// ----------------------------------------------------------------------------

int CFile::WriteFixAng (fixang a)
{
a = INTEL_SHORT (a);
return Write (&a, sizeof (a), 1);
}

// ----------------------------------------------------------------------------

void CFile::WriteVector (const CFixVector& v)
{
WriteFix (v [X]);
WriteFix (v [Y]);
WriteFix (v [Z]);
}

// ----------------------------------------------------------------------------

void CFile::WriteAngVec (const vmsAngVec& v)
{
WriteFixAng (v [PA]);
WriteFixAng (v [BA]);
WriteFixAng (v [HA]);
}

// ----------------------------------------------------------------------------

void CFile::WriteMatrix (const vmsMatrix& m)
{
WriteVector (m [RVEC]);
WriteVector (m [UVEC]);
WriteVector (m [FVEC]);
}


// ----------------------------------------------------------------------------

int CFile::WriteString (const char *buf)
{
if (buf && *buf && Write (buf, (int) strlen (buf), 1))
	return (int) WriteByte (0);   // write out NULL termination
return 0;
}

// ----------------------------------------------------------------------------

int CFile::Extract (const char *filename, const char *folder, int bUseD1Hog, const char *szDestName)
{
	FILE		*fp;
	char		szDest [FILENAME_LEN], fn [FILENAME_LEN];
	static	char buf [4096];
	int		h, l;

if (!Open (filename, folder, "rb", bUseD1Hog))
	return 0;
strcpy (fn, filename);
if (*szDestName) {
	if (*szDestName == '.') {
		char *psz = strchr (fn, '.');
		if (psz)
			strcpy (psz, szDestName);
		else
			strcat (fn, szDestName);
		}
	else
		strcpy (fn, szDestName);
	}
sprintf (szDest, "%s%s%s", gameFolders.szCacheDir, *gameFolders.szCacheDir ? "/" : "", fn);
if (! (fp = fopen (szDest, "wb"))) {
	Close ();
	return 0;
	}
for (h = sizeof (buf), l = m_cf.size; l; l -= h) {
	if (h > l)
		h = l;
	Read (buf, h, 1);
	fwrite (buf, h, 1, fp);
	}
Close ();
fclose (fp);
return 1;
}

//	-----------------------------------------------------------------------------------
//	Imagine if C had a function to copy a file...

#define COPY_BUF_SIZE 65536

int CFile::Copy (const char *pszSrc, const char *pszDest)
{
	sbyte	buf [COPY_BUF_SIZE];
	CFile	cf;

if (!cf.Open (pszDest, gameFolders.szSaveDir, "wb", 0))
	return -1;
if (!Open (pszSrc, gameFolders.szSaveDir, "rb", 0))
	return -2;
while (!EoF ()) {
	int bytes_read = (int) Read (buf, 1, COPY_BUF_SIZE);
	if (Error ())
		::Error (TXT_FILEREAD_ERROR, pszSrc, strerror (errno));
	Assert (bytes_read == COPY_BUF_SIZE || EoF ());
	cf.Write (buf, 1, bytes_read);
	if (cf.Error ())
		::Error (TXT_FILEWRITE_ERROR, pszDest, strerror (errno));
	}
if (Close ()) {
	cf.Close ();
	return -3;
	}
if (cf.Close ())
	return -4;
return 0;
}

// ----------------------------------------------------------------------------

char *CFile::ReadData (const char *filename, const char *folder, int bUseD1Hog)
{
	char		*pData = NULL;
	size_t	nSize;

if (!Open (filename, folder, "rb", bUseD1Hog))
	return NULL;
nSize = Length ();
if (!(pData = reinterpret_cast<char*> (D2_ALLOC ((uint) nSize))))
	return NULL;
if (!Read (pData, nSize, 1)) {
	D2_FREE (pData);
	pData = NULL;
	}
Close ();
return pData;
}

// ----------------------------------------------------------------------------

void CFile::SplitPath (const char *szFullPath, char *szFolder, char *szFile, char *szExt)
{
	int	h = 0, i, j, l = (int) strlen (szFullPath) - 1;

i = l;
#ifdef _WIN32
while ((i >= 0) && (szFullPath [i] != '/') && (szFullPath [i] != '\\') && (szFullPath [i] != ':'))
#else
while ((i >= 0) && (szFullPath [i] != '/'))
#endif
	i--;
i++;
j = l - 1;
while ((j > i) && (szFullPath [j] != '.'))
	j--;
if (szFolder) {
	if (i > 0)
		strncpy (szFolder, szFullPath, i);
	szFolder [i] = '\0';
	}
if (szFile) {
	if (!j || (j > i))
		strncpy (szFile, szFullPath + i, h = (j ? j : l + 1) - i);
	szFile [h] = '\0';
	}
if (szExt) {
	if (j && (j < l))
		strncpy (szExt, szFullPath + j, l - j);
	szExt [l - j] = '\0';
	}
}

// ----------------------------------------------------------------------------

time_t CFile::Date (const char *filename, const char *folder, int bUseD1Hog)
{
	struct stat statbuf;

//	sprintf (fn, "%s/%s", folder, hogname);
if (!Open (filename, folder, "rb", bUseD1Hog))
	return -1;
#ifdef _WIN32
fstat (_fileno (m_cf.file), &statbuf);
#else
fstat (fileno (m_cf.file), &statbuf);
#endif
Close ();
return statbuf.st_mtime;
}

// ----------------------------------------------------------------------------
