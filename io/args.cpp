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

CConfigManager appConfig;

//------------------------------------------------------------------------------

void CConfigManager::Destroy (void)
{
for (int i = Count (); i > 0; ) {
	if (m_properties [--i])
		delete[] m_properties [i];
	}
m_properties.Destroy ();
}

//------------------------------------------------------------------------------

int CConfigManager::Find (const char * s)
{
for (int i = 0, j = Count (); i < j; i++) 
	if (m_properties [i] && *m_properties [i] && !stricmp (m_properties [i], s))
		return i + 1;
return 0;
}

//------------------------------------------------------------------------------

char* CConfigManager::Filename (int bDebug)
{
	int	i;
	char*	p;
	CFile	cf;

if ((i = Find ("-ini")))
	strncpy (m_filename, m_properties [i + 1], sizeof (m_filename) - 1);
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

char* Trim (char* s);

int CConfigManager::Parse (CFile* cfP)
{
	char 		lineBuf [1024], *token;

if (cfP == NULL)
	cfP = &m_cf;
while (!cfP->EoF ()) {
	cfP->GetS (lineBuf, sizeof (lineBuf));
	if (*lineBuf && (*lineBuf != ';')) {
		token = strtok (lineBuf, " ");
		if (!m_properties.Push (StrDup (Trim (token))))
			break;
		token = strtok (NULL, " ");
		if (!m_properties.Push ((token && *token) ? StrDup (Trim (token)) : NULL)) {
			m_properties.Pop ();
			break;
			}
		}
	}
return Count ();
}

//------------------------------------------------------------------------------

void CConfigManager::PrintLog (void)
{
::Printlog (1, "");
for (int i = 0, j = 0; i < Count (); i++, j++) {
	if (!m_properties [i]) 
		continue;
	if ((m_properties [i][0] == '-') && (isalpha (m_properties [i][1]) || (j == 2))) {
		::PrintLog (1, "\n   ");
		j = 0;
		}
	::PrintLog (m_properties [i]);
	::Printlog (1, "");
	}
::PrintLog (1, "\n");
}

//------------------------------------------------------------------------------

void CConfigManager::Init (void)
{
m_properties.Create (100);
m_properties.Clear ();
m_properties.SetGrowth (100);
m_filename [0] = '\0';
m_null [0] = '\0';
}

//------------------------------------------------------------------------------

void CConfigManager::Load (int argC, char **argV)
{
for (int i = 0; i < argC; i++) {
	m_properties.Push (StrDup (argV [i]));
	if (*m_properties [i]== '-')
		strlwr (m_properties [i]);  // Convert all args to lowercase
	}
}

//------------------------------------------------------------------------------

void CConfigManager::Load (char* filename)
{
if (filename == NULL)
	filename = m_filename;

if (m_cf.Open (filename, "", "rb", 0)) {
	Parse ();
	m_cf.Close ();
	}
}

// ----------------------------------------------------------------------------

int CConfigManager::Value (int t, int nDefault)
{
	char *psz = m_properties [t];

if (!psz)
	return nDefault;
if (psz && isdigit (*psz))
	return atoi (psz);
if (((*psz == '-') || (*psz == '+')) && isdigit (*psz + 1))
	return atoi (psz);
return nDefault;
}

// ----------------------------------------------------------------------------

int CConfigManager::Value (const char* szArg, int nDefault)
{
	int t = Find (szArg);

return t ? Value (t, nDefault) : nDefault;
}

//------------------------------------------------------------------------------
//eof
