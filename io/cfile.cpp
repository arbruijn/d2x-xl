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

#include "descent.h"
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

#ifdef WIN32
#	define ZLIB_WINAPI
#endif

#include "zlib.h"

#ifdef WIN32
#	undef ZLIB_WINAPI
#endif

#define SORT_HOGFILES 1

int nCFileError = 0;

tGameFolders gameFolders;

// ----------------------------------------------------------------------------

int GetAppFolder (const char *szRootDir, char *szFolder, const char *szName, const char *szFilter)
{
	FFS	ffs;
	char	szDir [FILENAME_LEN];

if (!(szName && *szName))
	return 1;
int i = (int) strlen (szRootDir);
int bAddSlash = i && (szRootDir [i-1] != '\\') && (szRootDir [i-1] != '/');
PrintLog (0, "GetAppFolder ('%s', '%s', '%s', '%s')\n", szRootDir, szFolder, szName, szFilter);
sprintf (szDir, "%s%s%s%s%s", szRootDir, bAddSlash ? "/" : "", szName, *szFilter ? "/" : "", szFilter);
if (!(i = FFF (szDir, &ffs, *szFilter == '\0'))) {
	if (szFolder != szName)
		sprintf (szFolder, "%s%s%s", szRootDir, bAddSlash ? "/" : "", szName);
	}
else if (*szRootDir)
	strcpy (szFolder, szRootDir);
PrintLog (0, "GetAppFolder (%s) = '%s' (%d)\n", szName, szFolder, i);
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

if (!*filename || (strlen (filename) + (folder ? strlen (folder) : 0) >= FILENAME_LEN)) {
	return NULL;
	}
if ((*filename != '/') && (strstr (filename, "./") != filename) && folder && *folder) {
	sprintf (fn, "%s/%s", folder, filename);
   pfn = fn;
	}
 else
 	pfn = filename;
 
if ((fp = fopen (pfn, mode))) {
	if (ferror (fp)) {
		fclose (fp);
		fp = NULL;
		}
	}
else if (gameFolders.bAltHogDirInited && strcmp (folder, gameFolders.szAltHogDir)) {
   sprintf (fn, "%s/%s", gameFolders.szAltHogDir, filename);
   pfn = fn;
   fp = fopen (pfn, mode);
	}
//if (!fp) PrintLog (0, "CFGetFileHandle (): error opening %s\n", pfn);
return fp;
}

// ----------------------------------------------------------------------------

size_t CFile::Size (const char *hogname, const char *folder, int bUseD1Hog)
{
#if ! (defined (_WIN32_WCE) || defined (_WIN32))
	struct stat statbuf;

//	sprintf (fn, "%s/%s", folder, hogname);
if (!Open (hogname, gameFolders.szDataDir [0], "rb", bUseD1Hog))
	return -1;
#ifdef _WIN32
fstat (_fileno (m_info.file), &statbuf);
#else
fstat (fileno (m_info.file), &statbuf);
#endif
Close ();
return statbuf.st_size;
#else
	size_t size;

//sprintf (fn, "%s%s%s", folder, *folder ? "/" : "", hogname);
if (!Open (hogname, gameFolders.szDataDir [0], "rb", bUseD1Hog))
	return -1;
size = m_info.size;
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
if (!m_info.file)
	return 1;
#endif
return (m_info.rawPosition >= m_info.size) && (m_info.bufPos >= m_info.bufLen);
}

// ----------------------------------------------------------------------------

int CFile::Error (void)
{
if ((nCFileError = ferror (m_info.file)))
	PrintLog (0, "error %d during file operation\n", nCFileError);
return nCFileError;
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
	if ((fp = GetFileHandle (pfn + bNoHOG, folder, "rb"))) { // Check for non-hog file first...
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

int CFile::Open (const char *filename, const char *folder, const char *mode, int nHogType) 
{
if (!(filename && *filename))
	return 0;
if ((*mode == 'w') && gameStates.app.bReadOnly)
	return 0;

	int	length = -1;
	FILE	*fp = NULL;
	const char	*pszHogExt, *pszFileExt;

m_info.file = NULL;
if (*filename != '\x01') {
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

if (!fp && (nHogType >= 0) && (fp = hogFileManager.Find (filename, &length, nHogType == 1))) {
	if (stricmp (mode, "rb")) {
		::Error ("Cannot read hog file\n (wrong file io mode).\n");
		return 0;
		}
	}
if (!fp) 
	return 0;
m_info.file = fp;
m_info.rawPosition = 0;
m_info.size = (length < 0) ? ffilelength (fp) : length;
m_info.libOffset = (length < 0) ? 0 : ftell (fp);
m_info.filename = const_cast<char*> (filename);
m_info.bufPos = m_info.bufLen = 0;
return 1;
}

// ----------------------------------------------------------------------------

void CFile::Init (void) 
{
memset (&m_info, 0, sizeof (m_info)); 
m_info.rawPosition = -1; 
}

// ----------------------------------------------------------------------------

size_t CFile::Length (void) 
{
return m_info.size;
}

// ----------------------------------------------------------------------------
// Write () writes to the file
//
// returns:   number of full elements actually written
//
//
size_t CFile::Write (const void *buf, int nElemSize, int nElemCount, int bCompressed)
{
if (!m_info.file)
	return 0;
if (!(nElemSize * nElemCount))
	return 0;

if (bCompressed > 0) {
	size_t i = WriteCompressed (buf, (uint) (nElemSize * nElemCount));
	return (i < 0)? i : i / nElemSize;
	}

//if (bCompressed < 0)
//	PrintLog (0, "Write: %d bytes @ %d\n", nElemSize * nElemCount, (int) m_info.rawPosition);
int nWritten = (int) fwrite (buf, nElemSize, nElemCount, m_info.file);
m_info.rawPosition = ftell (m_info.file);
if (Error ()) {
	PrintLog (0, "file write error!\n");
	return 0;
	}
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

char_written = fputc (c, m_info.file);
m_info.rawPosition = ftell (m_info.file);
return char_written;
}

// ----------------------------------------------------------------------------

#ifdef _WIN32
inline 
#endif
int CFile::FillBuffer (void)
{
if (m_info.bufPos >= m_info.bufLen) {
	if (m_info.rawPosition >= m_info.size) 
		return EOF;
	size_t h = m_info.size - m_info.rawPosition;
	if (h > sizeof (m_info.buffer))
		h = sizeof (m_info.buffer);
	m_info.bufPos = 0;
	m_info.bufLen = int (Read (m_info.buffer, 1, h));
	m_info.rawPosition = ftell (m_info.file) - m_info.libOffset;
	if (m_info.bufLen < (int) h)
		m_info.size = m_info.rawPosition;
	}
return m_info.bufPos;
}

// ----------------------------------------------------------------------------

//int CFile::GetC (void) 
//{
//return (FillBuffer () == EOF) ? EOF : m_info.buffer [m_info.bufPos++];
//}

// ----------------------------------------------------------------------------
// CFile::PutS () writes a string to a file
//
// returns:   success ==> non-negative value
//            error   ==> EOF
//
int CFile::PutS (const char *str)
{
	int ret;

ret = fputs (str, m_info.file);
m_info.rawPosition = ftell (m_info.file);
return ret;
}

// ----------------------------------------------------------------------------

char * CFile::GetS (char * buf, size_t n) 
{
	size_t	i = 0;
	int		c;

--n;
while (i < n) {
	c = GetC ();
	if (c == EOF) {
		buf [i] = 0;
		return i ? buf : NULL;
		}
	if (c == 0)      // Unix line ending
		break;
	if (c == 10)
		break;
	if (c != 13) 
		buf [i++] = c;
	}
buf [i] = 0;
return buf;
}

// ----------------------------------------------------------------------------

size_t CFile::Read (void *buf, size_t elSize, size_t nElems, int bCompressed) 
{
size_t i, size = elSize * nElems;

if (!m_info.file || (m_info.size < 1) || !size) 
	return 0;

if (bCompressed > 0) {
	i = ReadCompressed (buf, (uint) size);
	return (i < 0) ? i : i / elSize;
	}

//if (bCompressed < 0)
//	PrintLog (0, "Read: %d bytes @ %d\n", (int) size, (int) m_info.rawPosition);
i = fread (buf, 1, size, m_info.file);
m_info.rawPosition += i;
return i / elSize;
}

// ----------------------------------------------------------------------------

size_t CFile::ReadCompressed (const void* buf, uint bufLen) 
{
//PrintLog (0, "ReadCompressed: %d bytes @ %d\n", bufLen, (int) m_info.rawPosition);
uLongf nSize, nCompressedSize;
size_t h = Read (&nSize, 1, sizeof (nSize), -1);
h += Read (&nCompressedSize, 1, sizeof (nCompressedSize), -1);
if (h != sizeof (nSize) + sizeof (nCompressedSize)) {
	// PrintLog (0, "ReadCompressed: error reading buffer sizes\n");
	return -1;
	}
if (bufLen < nSize) {
	// PrintLog (0, "ReadCompressed: destination buffer too small (%d < %d)\n", bufLen, nSize);
	return -1;
	}
CByteArray compressedBuffer;
if (!compressedBuffer.Create (nCompressedSize)) {
	// PrintLog (0, "ReadCompressed: couldn't create decompression buffer\n");
	return -1;
	}
if (Read (compressedBuffer.Buffer (), sizeof (ubyte), nCompressedSize, -1) != nCompressedSize) {
	// PrintLog (0, "ReadCompressed: error reading data\n");
	return -1;
	}
int i;
if ((i = uncompress ((ubyte*) buf, &nSize, compressedBuffer.Buffer (), nCompressedSize)) != Z_OK) {
	// PrintLog (0, "ReadCompressed: decompression error #%d\n", i);
	return -1;
	}
return (size_t) nSize;
}

// ----------------------------------------------------------------------------

size_t CFile::WriteCompressed (const void* buf, uint bufLen) 
{
//PrintLog (0, "WriteCompressed: %d bytes @ %d\n", bufLen, (int) m_info.rawPosition);
uLongf nCompressedSize = compressBound (bufLen);
CByteArray compressedBuffer;
if (compressedBuffer.Create (nCompressedSize) && (compress (compressedBuffer.Buffer (), &nCompressedSize, (ubyte*) buf, bufLen) == Z_OK)) {
	size_t h = Write (&bufLen, 1, sizeof (bufLen), -1);
	h += Write (&nCompressedSize, 1, sizeof (nCompressedSize), -1);
	h += Write (compressedBuffer.Buffer (), sizeof (byte), nCompressedSize, -1);
	return (h == sizeof (bufLen) + sizeof (nCompressedSize) + nCompressedSize) ? bufLen : -1;
	}
return -1;
}

// ----------------------------------------------------------------------------

size_t CFile::Tell (void) 
{
return m_info.rawPosition;
}

// ----------------------------------------------------------------------------

size_t CFile::Seek (long offset, int whence) 
{
if (!m_info.size)
	return -1;

	size_t destPos;

switch (whence) {
	case SEEK_SET:
		destPos = offset;
		break;
	case SEEK_CUR:
		destPos = m_info.rawPosition + offset;
		break;
	case SEEK_END:
		destPos = m_info.size + offset;
		break;
	default:
		return 1;
	}
size_t c = fseek (m_info.file, long (m_info.libOffset + destPos), SEEK_SET);
m_info.rawPosition = ftell (m_info.file) - m_info.libOffset;
return c;
}

// ----------------------------------------------------------------------------

int CFile::Close (void)
{
	int result;

if (!m_info.file)
	return 0;
result = fclose (m_info.file);
m_info.file = NULL;
m_info.size = 0;
m_info.rawPosition = -1;
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

uint CFile::ReadUInt (void)
{
	uint32_t i;

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

ushort CFile::ReadUShort (void)
{
	uint16_t s;

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

ubyte CFile::ReadUByte (void)
{
	ubyte b;

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
v.v.coord.x = ReadFix ();
v.v.coord.y = ReadFix ();
v.v.coord.z = ReadFix ();
}

// ----------------------------------------------------------------------------

void CFile::ReadVector (CFloatVector3& v) 
{
v.v.coord.x = ReadFloat ();
v.v.coord.y = ReadFloat ();
v.v.coord.z = ReadFloat ();
}

// ----------------------------------------------------------------------------

void CFile::ReadAngVec (CAngleVector& v)
{
v.v.coord.p = ReadFixAng ();
v.v.coord.b = ReadFixAng ();
v.v.coord.h = ReadFixAng ();
}

// ----------------------------------------------------------------------------

void CFile::ReadMatrix (CFixMatrix& m)
{
ReadVector (m.m.dir.r);
ReadVector (m.m.dir.u);
ReadVector (m.m.dir.f);
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
return (int) Write (&i, sizeof (i), 1);
}

// ----------------------------------------------------------------------------

int CFile::WriteShort (short s)
{
s = INTEL_SHORT (s);
return (int) Write (&s, sizeof (s), 1);
}

// ----------------------------------------------------------------------------

int CFile::WriteByte (sbyte b)
{
return (int) Write (&b, sizeof (b), 1);
}

// ----------------------------------------------------------------------------

int CFile::WriteFloat (float f)
{
f = INTEL_FLOAT (f);
return (int) Write (&f, sizeof (f), 1);
}

// ----------------------------------------------------------------------------
//Read and return a double (64 bits)
//Throws an exception of nType (nCFileError *) if the OS returns an error on read
int CFile::WriteDouble (double d)
{
d = INTEL_DOUBLE (d);
return (int) Write (&d, sizeof (d), 1);
}

// ----------------------------------------------------------------------------

int CFile::WriteFix (fix x)
{
x = INTEL_INT (x);
return (int) Write (&x, sizeof (x), 1);
}

// ----------------------------------------------------------------------------

int CFile::WriteFixAng (fixang a)
{
a = INTEL_SHORT (a);
return (int) Write (&a, sizeof (a), 1);
}

// ----------------------------------------------------------------------------

void CFile::WriteVector (const CFixVector& v)
{
WriteFix (v.v.coord.x);
WriteFix (v.v.coord.y);
WriteFix (v.v.coord.z);
}

// ----------------------------------------------------------------------------

void CFile::WriteVector (const CFloatVector3& v)
{
WriteFloat (v.v.coord.x);
WriteFloat (v.v.coord.y);
WriteFloat (v.v.coord.z);
}

// ----------------------------------------------------------------------------

void CFile::WriteAngVec (const CAngleVector& v)
{
WriteFixAng (v.v.coord.p);
WriteFixAng (v.v.coord.b);
WriteFixAng (v.v.coord.h);
}

// ----------------------------------------------------------------------------

void CFile::WriteMatrix (const CFixMatrix& m)
{
WriteVector (m.m.dir.r);
WriteVector (m.m.dir.u);
WriteVector (m.m.dir.f);
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
	size_t	h, l;

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
for (h = sizeof (buf), l = m_info.size; l; l -= h) {
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
if (!(pData = new char [nSize]))
	return NULL;
if (!Read (pData, nSize, 1)) {
	delete[] pData;
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
while ((i >= 0) && (szFullPath [i] != '/') && (szFullPath [i] != '\\'))
#endif
	i--;
i++;
j = l;
while ((j > i) && (szFullPath [j] != '.'))
	j--;
if (szExt) {
	if (szFullPath [j] != '.')
		*szExt = '\0';
	else {
		strncpy (szExt, szFullPath + j, l - j + 1);
		szExt [l - j + 1] = '\0';
		}
	}
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
}

// ----------------------------------------------------------------------------

time_t CFile::Date (const char *filename, const char *folder, int bUseD1Hog)
{
	struct stat statbuf;

//	sprintf (fn, "%s/%s", folder, hogname);
if (!Open (filename, folder, "rb", bUseD1Hog))
	return -1;
#ifdef _WIN32
fstat (_fileno (m_info.file), &statbuf);
#else
fstat (fileno (m_info.file), &statbuf);
#endif
Close ();
return statbuf.st_mtime;
}

// ----------------------------------------------------------------------------

int CFile::LineCount (const char* filename, const char* folder, char* delims)
{
if (!Open (filename, folder, "rb", 0))
	return -1;

bool bNewl = true;
char buf [16384];
size_t h = m_info.size - m_info.rawPosition, i = h + 1;
int lineC = 0;

while (!EoF () || (i < h)) {
	if (i >= h) {
		h = m_info.size - m_info.rawPosition;
		if (h > sizeof (buf))
			h = sizeof (buf);
		Read (buf, h, 1);
		i = 0;
		}
	if (bNewl && !strchr (delims, buf [i]))
		lineC++;
	bNewl = (buf [i++] == '\n');
	}
Close ();
return lineC;
}

// ----------------------------------------------------------------------------
