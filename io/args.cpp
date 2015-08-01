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
for (int32_t i = Count (); i > 0; ) {
	if (m_properties [--i])
		delete[] m_properties [i];
	}
m_properties.Destroy ();
}

//------------------------------------------------------------------------------

int32_t CConfigManager::Find (const char * s)
{
for (int32_t i = 0, j = Count (); i < j; i++) 
	if (m_properties [i] && *m_properties [i] && !stricmp (m_properties [i], s))
		return i + 1;
return 0;
}

//------------------------------------------------------------------------------

char* CConfigManager::Filename (int32_t bDebug)
{
	int32_t	i;
	CFile	cf;

if ((i = Find ("-ini")))
	strncpy (m_filename, m_properties [i], sizeof (m_filename) - 1);
else {
#if defined(__unix__)
	FFS		ffs;
	if (FFF (gameFolders.user.szConfig, &ffs, 1) <= 0) {
#endif
	sprintf (m_filename, "%s%s", gameFolders.user.szConfig, bDebug ? "d2xdebug.ini" : "d2x.ini");
	if (!cf.Exist (m_filename, "", false)) 
		sprintf (m_filename, "%s%s", gameFolders.user.szConfig, bDebug ? "d2xdebug-default.ini" : "d2x-default.ini");
#if defined(__unix__)
   }
#endif //!__unix__
	}
return m_filename;
}

//------------------------------------------------------------------------------

char* Trim (char* s);

int32_t CConfigManager::Parse (CFile* pFile)
{
	char 		lineBuf [1024], *token;

if (pFile == NULL)
	pFile = &m_cf;
while (!pFile->EoF ()) {
	pFile->GetS (lineBuf, sizeof (lineBuf));
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
::PrintLog (1, "");
for (int32_t i = 0, j = 0; i < Count (); i++, j++) {
	if (!m_properties [i]) 
		continue;
	if ((m_properties [i][0] == '-') && (isalpha (m_properties [i][1]) || (j == 2))) {
		::PrintLog (0, "\n   ");
		j = 0;
		}
	::PrintLog (0, m_properties [i]);
	::PrintLog (0, "");
	}
::PrintLog (-1, "\n");
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

void CConfigManager::Load (int32_t argC, char **argV)
{
for (int32_t i = 0; i < argC; i++) {
	m_properties.Push (StrDup (argV [i]));
	if (*m_properties [i] == '-')
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

int32_t CConfigManager::Int (int32_t t, int32_t nDefault)
{
	char *psz = m_properties [t];

if (!psz || !*psz)
	return nDefault;
if (isdigit (*psz))
	return atoi (psz);
if (((*psz == '-') || (*psz == '+')) && isdigit (*(psz + 1)))
	return atoi (psz);
return nDefault;
}

// ----------------------------------------------------------------------------

int32_t CConfigManager::Int (const char* szArg, int32_t nDefault)
{
	int32_t t = Find (szArg);

return t ? Int (t, nDefault) : nDefault;
}

// ----------------------------------------------------------------------------

const char* CConfigManager::Text (const char* szArg, const char* pszDefault)
{
	int32_t t = Find (szArg);

return t ? m_properties [t] : pszDefault;
}

//------------------------------------------------------------------------------
//eof
