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
 COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "cfile.h"
#include "u_mem.h"
#include "strio.h"
#include "strutil.h"
#include "console.h"
#include "error.h"
#include "findfile.h"
#include "strutil.h"
#include "args.h"

CArgManager appArgs;

//------------------------------------------------------------------------------

void CArgManager::Destroy (void)
{
for (int i = Count (); i > 0; i) {
	if (m_argList [--i])
		delete[] m_argList [i];
	}
m_argList.Destroy ();
}

//------------------------------------------------------------------------------

int CArgManager::Find (const char * s)
{
	int i;
  
for (i = 0; i < Count (); i++)
	if (m_argList [i] && *m_argList [i] && !stricmp (m_argList [i], s))
		return i;
return 0;
}

//------------------------------------------------------------------------------

char* CArgManager::Filename (int bDebug)
{
	int	i;
	char*	p;
	CFile	cf;

if ((i = Find ("-ini")))
	strncpy (m_filename, m_argList [i + 1], sizeof (m_filename) - 1);
else {
#if defined(__unix__)
	FFS		ffs;
	strcpy (m_filename, gameFolders.szHomeDir);
	strcat (m_filename, "/.d2x-xl");
	if (FFF (m_filename, &ffs, 0) <= 0) {
#endif
	strcpy (m_filename, gameFolders.szConfigDir);
	i = int (strlen (m_filename));
	if (i) {
		p = m_filename + i - 1;
		if ((*p == '\\') || (*p == '/'))
			p++;
		else {
			strcat (m_filename, "/");
			p += 2;
			}
		}
	else
		p = m_filename;
	strcpy (p, bDebug ? "d2xdebug.ini" : "d2x.ini");
	if (!cf.Exist (m_filename, "", false)) 
		strcpy (p, bDebug ? "d2xdebug-default.ini" : "d2x-default.ini");
#if defined(__unix__)
   }
#endif //!__unix__
	}
return m_filename;
}

//------------------------------------------------------------------------------

int CArgManager::Parse (CFile* cfP)
{
	char 		*pszLine, *pszToken;

if (cfP == NULL)
	cfP = &m_cf;
while (!cfP->EoF ()) {
	pszLine = fsplitword (*cfP, '\n');
	if (*pszLine && (*pszLine != ';')) {
		pszToken = splitword (pszLine, ' ');
		if (!m_argList.Push (pszToken))
			break;
		if (pszLine) {
			int l;
			for (l = strlen (pszLine); l > 0; l--) {
				if (pszLine [l - 1] != '\r')
					break;
				}
			pszLine [l] = '\0';
			if (!m_argList.Push (*pszLine ? StrDup (pszLine) : NULL)) {
				m_argList.Pop ();
				break;
				}
			}
		}
	delete[] pszLine; 
	}
return Count ();
}

//------------------------------------------------------------------------------

void CArgManager::PrintLog (void)
{
::PrintLog ("   ");
for (int i = 0, j = 0; i < Count (); i++, j++) {
	if (!m_argList [i]) 
		continue;
	if ((m_argList [i][0] == '-') && (isalpha (m_argList [i][1]) || (j == 2))) {
		::PrintLog ("\n   ");
		j = 0;
		}
	::PrintLog (m_argList [i]);
	::PrintLog (" ");
	}
::PrintLog ("\n");
}

//------------------------------------------------------------------------------

void CArgManager::Init (void)
{
m_argList.Create (100);
m_argList.SetGrowth (100);
m_filename [0] = '\0';
m_null [0] = '\0';
}

//------------------------------------------------------------------------------

void CArgManager::Load (int argC, char **argV)
{
for (int i = 0; i < argC; i++) {
	m_argList.Push (StrDup (argV [i]));
	if (*m_argList [i]== '-')
		strlwr (m_argList [i]);  // Convert all args to lowercase
	}
}

//------------------------------------------------------------------------------

void CArgManager::Load (char* filename)
{
if (filename == NULL)
	filename = m_filename;

if (m_cf.Open (filename, "", "rt", 0)) {
	Parse ();
	m_cf.Close ();
	}
}

// ----------------------------------------------------------------------------

int CArgManager::Value (int t, int nDefault)
{
	char *psz = m_argList [t+1];

if (!psz)
	return nDefault;
if (psz && isdigit (*psz))
	return atoi (psz);
if (((*psz == '-') || (*psz == '+')) && isdigit (*psz + 1))
	return atoi (psz);
return nDefault;
}

// ----------------------------------------------------------------------------

int CArgManager::Value (char* szArg, int nDefault)
{
	int t = Find (szArg);

return t ? Value (t, nDefault) : nDefault;
}

//------------------------------------------------------------------------------
//eof
