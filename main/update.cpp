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
#	include <SDL/SDL_thread.h>
#	include "SDL/SDL_keyboard.h"
#	include "FolderDetector.h"
#else
#	include "SDL_main.h"
#	include "SDL_thread.h"
#	include "SDL_keyboard.h"
#endif
#include "descent.h"
#include "text.h"
#include "menu.h"

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#if defined(WIN32)
class CDownloadCallback : public IBindStatusCallback {
	public:
		virtual HRESULT STDMETHODCALLTYPE OnProgress (ULONG ulProgress, ULONG ulProgressMax, ULONG ulResultCode, LPCWSTR szResultText) { return E_NOTIMPL; } 

		virtual ULONG STDMETHODCALLTYPE AddRef (void) { return E_NOTIMPL; } 

		virtual ULONG STDMETHODCALLTYPE Release (void) { return E_NOTIMPL; } 

		virtual HRESULT STDMETHODCALLTYPE QueryInterface (const IID &,void **) { return E_NOTIMPL; } 

		virtual HRESULT STDMETHODCALLTYPE OnStartBinding (DWORD dwReserved, __RPC__in_opt IBinding *pib) { return E_NOTIMPL; } 

		virtual HRESULT STDMETHODCALLTYPE GetPriority (__RPC__out LONG *pnPriority) { return E_NOTIMPL; } 

		virtual HRESULT STDMETHODCALLTYPE OnLowResource (DWORD reserved) { return E_NOTIMPL; } 

		virtual HRESULT STDMETHODCALLTYPE OnStopBinding (HRESULT hresult,__RPC__in_opt LPCWSTR szError) { return E_NOTIMPL; } 

		virtual HRESULT STDMETHODCALLTYPE GetBindInfo (DWORD *grfBINDF, BINDINFO *pbindinfo) { return E_NOTIMPL; } 

		virtual HRESULT STDMETHODCALLTYPE OnDataAvailable (DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed) { return E_NOTIMPL; } 

		virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable (__RPC__in REFIID riid, __RPC__in_opt IUnknown *punk) { return E_NOTIMPL; } 
	};
#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

class CDownload {
	protected:
		int32_t		m_nState;
		int32_t		m_nResult;
		const char* m_pszSrc;
		const char* m_pszDest;
		bool			m_bProgressBar;

		static CDownload* m_handler;

	private:
		int32_t		m_nProgress;
		int32_t		m_nProgressMax;
		int32_t		m_nPercent;
		SDL_Thread*	m_thread;
		int32_t		m_nOptPercentage;
		int32_t		m_nOptProgress;
		CMenu			m_menu;

	private:
		void Setup (const char* pszSrc, const char* pszDest, bool bProgressBar) {
			m_nState = -1;
			m_nResult = 0;
			m_nPercent = 0;
			m_nProgress = 0;
			m_nProgressMax = 0;
			m_pszSrc = pszSrc;
			m_pszDest = pszDest;
			if ((m_bProgressBar = bProgressBar)) {
				char szProgress [50];
				sprintf (szProgress, "0%c done", '%');
				m_nOptPercentage = m_menu.AddText (szProgress, 0);
				m_menu [m_nOptPercentage].m_x = (int16_t) 0x8000;	//centered
				m_menu [m_nOptPercentage].m_bCentered = 1;
				m_nOptProgress = m_menu.AddGauge ("progress bar", "                    ", -1, 100);
				}
			}

	protected:
		CDownload () : m_nState (-1), m_nResult (0), m_pszSrc (NULL), m_pszDest (NULL), m_bProgressBar (false), m_nProgress (0), m_nProgressMax (0),
							m_nPercent (0), m_thread (NULL), m_nOptPercentage (0), m_nOptProgress (0) {}

		virtual ~CDownload () {}

		CDownload (CDownload const&) : m_nState (-1), m_nResult (0), m_pszSrc (NULL), m_pszDest (NULL), m_bProgressBar (false), m_nProgress (0), m_nProgressMax (0),
												 m_nPercent (0), m_thread (NULL), m_nOptPercentage (0), m_nOptProgress (0) {}

		CDownload& operator= (CDownload const&) { return *this; }

	public:
		static CDownload* Handler (void) {
			return m_handler;
			}

		int32_t Update (void) {
			Start ();
			if (!(m_nProgress && m_nProgressMax))
				return 1;
			int32_t h = int32_t (float (m_nProgress) * 100.0f / float (m_nProgressMax));
			if (h == m_nPercent)
				return 1;
			if (h >= 100)
				return 0;
			m_nPercent = h;
			sprintf (m_menu [m_nOptPercentage].m_text, TXT_PROGRESS, m_nPercent, '%');
			m_menu [m_nOptPercentage].m_bRebuild = 1;
			if (m_menu [m_nOptProgress].Value () != m_nPercent) {
				m_menu [m_nOptProgress].Value () = m_nPercent;
				m_menu [m_nOptProgress].m_bRebuild = 1;
				}
			return 1;
			}

	public:
		inline int32_t State (void) { return m_nState; }

		inline int32_t Result (void) { return m_nResult; }

		static int32_t MenuPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState) {
			if (!nState) {
				CDownload::Handler ()->Update ();
				key = (CDownload::Handler ()->State () == 1) ? -2 : 0;
				}
			return nCurItem;
			}

		virtual int32_t Fetch (void) = 0;

		static int32_t _CDECL_ Download (void* downloadHandler) {
			return CDownload::Handler ()->Fetch ();
			}

		int32_t Execute (const char* pszSrc, const char* pszDest, bool bProgressBar) {
			Setup (pszSrc, pszDest, bProgressBar);
			if (m_bProgressBar)
				for (; m_menu.Menu (NULL, "Downloading...", &CDownload::MenuPoll) >= 0; )
					;
			else
				Start ();
			m_thread = NULL;
			m_menu.Destroy ();
			return Result ();
			}

		void Start (void) {
			if (m_nState < 0) {
				m_nState = 0;
				m_thread = SDL_CreateThread (&CDownload::Download, NULL);
				}
			}

		void SetProgress (int32_t nProgress, int32_t nProgressMax) {
			m_nProgress = nProgress;
			m_nProgressMax = nProgressMax;
			}
	};


// ----------------------------------------------------------------------------

CDownload* CDownload::m_handler = NULL;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#if defined(__unix__)

#	define FILEEXT		"rar"
#	define FILETYPE	"src"

#include <curl/curl.h>
#include <cstdio>

// ----------------------------------------------------------------------------

// link with libcurl (-lcurl)

class CLinuxDownload : public CDownload {
	protected:
		CLinuxDownload () : CDownload () {}

		virtual ~CLinuxDownload () {}

		CLinuxDownload (CDownload const&) {}

		CLinuxDownload& operator= (CLinuxDownload const&) { return *this; }

	public:
		static CDownload* Handler (void) {
			if (!m_handler)
				m_handler = new CLinuxDownload ();
			return m_handler;
			}

		static int32_t OnProgress (void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
			CDownload::Handler ()->SetProgress (int32_t (dlnow), int32_t (dltotal));
			return 0;
			}

		virtual int32_t Fetch (void) {
			CURL* hCurl;
			if (!(hCurl = curl_easy_init ()))
				return m_nResult = 1;
			if (m_bProgressBar) {
				curl_easy_setopt (hCurl, CURLOPT_NOPROGRESS, 0);
				curl_easy_setopt (hCurl, CURLOPT_PROGRESSFUNCTION, &CLinuxDownload::OnProgress);
				}
			else
				curl_easy_setopt (hCurl, CURLOPT_NOPROGRESS, 1);
			if (curl_easy_setopt (hCurl, CURLOPT_URL, m_pszSrc)) {
				curl_easy_cleanup (hCurl);
				return m_nResult = 1;
				}
			std::FILE* file;
			if (!(file = std::fopen (m_pszDest, "w"))) {
				curl_easy_cleanup (hCurl);
				return m_nResult = 1;
				}
			#if DBG
			curl_easy_setopt (hCurl, CURLOPT_VERBOSE, 1);
			#endif
			if (curl_easy_setopt (hCurl, CURLOPT_WRITEDATA, file)) {
				curl_easy_cleanup (hCurl);
				return m_nResult = 1;
				}
			if (curl_easy_perform (hCurl)) {
				curl_easy_cleanup (hCurl);
				std::fclose (file);
				unlink (m_pszDest);
				return m_nResult = 1;
				}
			curl_easy_cleanup (hCurl);
			std::fclose (file);
			m_nState = 1;
			return m_nResult = 0;
			}
	};

// ----------------------------------------------------------------------------

int32_t DownloadFile (const char* pszSrc, const char* pszDest, bool bProgressBar)
{
return CLinuxDownload::Handler ()->Execute (pszSrc, pszDest, bProgressBar);
}

#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#if defined(_WIN32)

#	include "urlmon.h"
#	include <process.h>
#	include "errno.h"

#	define FILEEXT		"exe"
#	define FILETYPE	"win"

// ----------------------------------------------------------------------------

class CWindowsDownload : public CDownload, public CDownloadCallback {
	protected:
		CWindowsDownload () : CDownload () {}

		CWindowsDownload (CDownload const&) {}

		CWindowsDownload& operator= (CWindowsDownload const&) { return *this; }

	public:
		static CDownload* Handler (void) {
			if (!m_handler)
				m_handler = new CWindowsDownload ();
			return m_handler;
			}

		virtual HRESULT STDMETHODCALLTYPE OnProgress (ULONG ulProgress, ULONG ulProgressMax, ULONG ulResultCode, LPCWSTR szResultText) {
			CDownload::Handler ()->SetProgress (int32_t (ulProgress), int32_t (ulProgressMax));
			return S_OK;
			}

		virtual int32_t Fetch (void) {
			m_nResult = URLDownloadToFile (NULL, m_pszSrc, m_pszDest, NULL, (CWindowsDownload*) Handler ());
			m_nState = 1;
			return m_nResult;
			}
	};

// ----------------------------------------------------------------------------

int32_t DownloadFile (const char* pszSrc, const char* pszDest, bool bProgressBar)
{
return CWindowsDownload::Handler ()->Execute (pszSrc, pszDest, bProgressBar);
}

// ----------------------------------------------------------------------------

#endif

#if defined(_WIN32) || defined(__unix__)

int32_t CheckForUpdate (void)
{
	char		szSrc [FILENAME_LEN], szDest [FILENAME_LEN];
	CFile		cf;
	int32_t		nVersion [3], nLocation;
	char		szMsg [1000];

	static const char* pszSource [2] = {
		"http://www.descent2.de/files", 
#if defined(_WIN32)
		"http://sourceforge.net/projects/d2x-xl/files/Windows"
#else
		"http://sourceforge.net/projects/d2x-xl/files/Linux"
#endif
	};

sprintf (szDest, "%sd2x-xl-version.txt", gameFolders.var.szDownloads);
if (!DownloadFile ("http://www.descent2.de/files/d2x-xl-version.txt", szDest, false))
	nLocation = 0;
else if (!DownloadFile ("http://sourceforge.net/projects/d2x-xl/files/d2x-xl-version.txt/download", szDest, false))
	nLocation = 1;
else {
	InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_CLOSE, TXT_DOWNLOAD_FAILED);
	return -1;
	}
G3_SLEEP (1000);
if (!cf.Open ("d2x-xl-version.txt", gameFolders.var.szDownloads, "rb", -1)) {
	InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_CLOSE, TXT_DOWNLOAD_FAILED);
	return -1;
	}
if (3 != fscanf (cf.File (), "%d.%d.%d", &nVersion [0], &nVersion [1], &nVersion [2])) {
	InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_CLOSE, TXT_DOWNLOAD_FAILED);
	return -1;
	}

#if !DBG
if (D2X_IVER >= nVersion [0] * 100000 + nVersion [1] * 1000 + nVersion [2]) {
	TextBox (NULL, BG_STANDARD, 1, TXT_CLOSE, TXT_NO_UPDATE_FOUND);
	return 0;
	}
#endif

if (InfoBox (NULL,NULL,  BG_STANDARD, 2, TXT_YES, TXT_NO, TXT_UPDATE_FOUND))
	return 0;
sprintf (szDest, "%sd2x-xl-%s-%d.%d.%d.%s", gameFolders.var.szDownloads,
			FILETYPE, nVersion [0], nVersion [1], nVersion [2], FILEEXT);
#if 1
messageBox.Show ("Downloading...");
sprintf (szSrc, "%s/d2x-xl-%s-%d.%d.%d.%s", pszSource [nLocation], FILETYPE, nVersion [0], nVersion [1], nVersion [2], FILEEXT);
if (DownloadFile (szSrc, szDest, true)) {
	messageBox.Clear ();
	InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_CLOSE, TXT_DOWNLOAD_FAILED);
	return -1;
	}
messageBox.Clear ();
if (!cf.Exist (szDest, "", 0)) {
	InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_CLOSE, TXT_DOWNLOAD_FAILED);
	return -1;
	}
#endif
#if defined(__unix__)
sprintf (szMsg, TXT_DOWNLOAD_SUCCEEDED, szDest);
TextBox (NULL, BG_STANDARD, 1, TXT_CLOSE, szMsg);
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
sprintf (szMsg, TXT_PATCH_FAILED, szDest);
//Warning (szMsg);
InfoBox (TXT_ERROR, (pMenuCallback) NULL, BG_STANDARD, 1, TXT_CLOSE, szMsg);
#endif
return -1;
}

#endif

// ----------------------------------------------------------------------------
