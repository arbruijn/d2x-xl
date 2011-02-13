#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(__unix__) || defined(__macosx__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __macosx__
#	include "SDL/SDL_main.h"
#	include "SDL/SDL_keyboard.h"
#	include "FolderDetector.h"
#else
#	include "SDL_main.h"
#	include "SDL_keyboard.h"
#endif
#include "descent.h"
#include "text.h"
#include "menu.h"

// ----------------------------------------------------------------------------

#if defined(__unix__)

#	define FILEEXT		"rar"
#	define FILETYPE		"src"

#include <curl/curl.h>
#include <cstdio>

// link with libcurl (-lcurl)

int DownloadFile (const char* pszSrc, const char* pszDest)
{
CURL* hCurl;
if (!(hCurl = curl_easy_init ()))
	return 1;
if (curl_easy_setopt (hCurl, CURLOPT_URL, pszSrc)) {
	curl_easy_cleanup (hCurl);
	return 1;
	}
std::FILE* file;
if (!(file = std::fopen (pszDest, "w"))) {
	curl_easy_cleanup (hCurl);
	return 1;
	}
#if DBG
curl_easy_setopt (hCurl, CURLOPT_VERBOSE, 1);
#endif
if (curl_easy_setopt (hCurl, CURLOPT_WRITEDATA, file)) {
	curl_easy_cleanup (hCurl);
	return 1;
	}
if (curl_easy_perform (hCurl)) {
	curl_easy_cleanup (hCurl);
	std::fclose (file);
	unlink (pszDest);
	return 1;
	}
curl_easy_cleanup (hCurl);
std::fclose (file);
return 0;
}

// ----------------------------------------------------------------------------

#elif defined(_WIN32)

#	include "urlmon.h"
#	include <process.h>
#	include "errno.h"

#	define DownloadFile(_src,_dest)	URLDownloadToFile (NULL, _src, _dest, NULL, NULL)

#	define FILEEXT		"exe"
#	define FILETYPE		"win"

#endif

#if defined(_WIN32) || defined(__unix__)

int CheckForUpdate (void)
{
	char		szSrc [FILENAME_LEN], szDest [FILENAME_LEN];
	CFile		cf;
	int		nVersion [3], nLocation;
	char		szMsg [1000];

	static const char* pszSource [2] = {
		"http://www.descent2.de/files", 
		"http://sourceforge.net/projects/d2x-xl/files"
	};

sprintf (szDest, "%s/d2x-xl-version.txt", gameFolders.szDownloadDir);
if (!DownloadFile ("http://www.descent2.de/files/d2x-xl-version.txt", szDest))
	nLocation = 0;
else if (!DownloadFile ("http://sourceforge.net/projects/d2x-xl/files/d2x-xl-version.txt/download", szDest))
	nLocation = 1;
else {
	MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, "Download failed.");
	return -1;
	}
if (!cf.Open ("d2x-xl-version.txt", gameFolders.szDownloadDir, "rt", -1)) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, "Download failed.");
	return -1;
	}
if (3 != fscanf (cf.File (), "%d.%d.%d", &nVersion [0], &nVersion [1], &nVersion [2])) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, "Download failed.");
	return -1;
	}

#if !DBG
if (D2X_IVER >= nVersion [0] * 100000 + nVersion [1] * 1000 + nVersion [2]) {
	MsgBox (NULL, NULL, 1, TXT_CLOSE, "No updates were found.");
	return 0;
	}
#endif

if (MsgBox (NULL, NULL, 2, TXT_YES, TXT_NO, "An update has been found. Download it?"))
	return 0;
sprintf (szDest, "%s/d2x-xl-%s-%d.%d.%d.%s", gameFolders.szDownloadDir,
			FILETYPE, nVersion [0], nVersion [1], nVersion [2], FILEEXT);
#if 1
messageBox.Show ("Downloading...");
sprintf (szSrc, "%s/d2x-xl-%s-%d.%d.%d.%s", pszSource [nLocation], FILETYPE, nVersion [0], nVersion [1], nVersion [2], FILEEXT);
if (DownloadFile (szSrc, szDest)) {
	messageBox.Clear ();
	MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, "Download failed.");
	return -1;
	}
messageBox.Clear ();
if (!cf.Exist (szDest, "", 0)) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, "Download failed.");
	return -1;
	}
#endif
#if defined(__unix__)
sprintf (szMsg, "\nThe file\n\n%s\n\nwas sucessfully downloaded.", szDest);
MsgBox (NULL, NULL, 1, TXT_CLOSE, szMsg);
#else
#	if 1
#	include "shellapi.h"
#	include "objbase.h"
#if !defined(_M_IA64) && !defined(_M_AMD64) && !defined(_OPENMP) && !defined(__INTEL_COMPILER)
CoInitializeEx (NULL, COINIT_MULTITHREADED);
#	else
CoInitialize (NULL);
#	endif
if (HINSTANCE (32) < ShellExecute (NULL, NULL, szDest, NULL, NULL, SW_SHOW))
	exit (1);
#else
char*	args [2] = {szDest, NULL};
if (0 <= _execv (szDest, args))
	exit (1);
#endif
sprintf (szMsg, "\nThe file\n\n%s\n\nwas sucessfully downloaded, but couldn't be excuted.\nPlease leave D2X-XL and start the installer manually.", szDest);
//Warning (szMsg);
MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, szMsg);
#endif
return -1;
}

#endif

// ----------------------------------------------------------------------------
