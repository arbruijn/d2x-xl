/*
 *
 * Code for controlling the console
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef _WIN32
#	include <fcntl.h>
#endif
#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "inferno.h"
#include "error.h"
#include "strutil.h"

CCvar* CCvar::m_list = NULL;

//------------------------------------------------------------------------------

void CCvar::Init (void)
{
m_next = NULL;
m_name = NULL;
m_text = NULL;
m_value = 0;
}

//------------------------------------------------------------------------------

void CCvar::Destroy (void)
{
if (m_name) {
	delete[] m_name;
	m_name = NULL;
	}
if (m_text) {
	delete[] m_text;
	m_text = NULL;
	}
delete this;
}

//------------------------------------------------------------------------------

CCvar* CCvar::Register (const char* name, char* value)
{
	CCvar*	ptr;

if (!(ptr = Find (name))) {
	if (!(ptr = new CCvar))
		return NULL;
	int l = strlen (name) + 1;
	if (!(ptr->m_name = StrDup (name))) {
		delete ptr;
		return NULL;
		}
	ptr->m_next = m_list;
	m_list = ptr;
	}
ptr->Set (value);
return ptr;
}

//------------------------------------------------------------------------------

CCvar* CCvar::Register (const char* name, double value)
{
	char	szValue [100];

sprintf (szValue, "%f", value);
return Register (name, szValue);
}

//------------------------------------------------------------------------------

CCvar* CCvar::Find (const char* name)
{
if (!(name && *name))
	return NULL;

	CCvar *ptr;

for (ptr = m_list; ptr != NULL; ptr = ptr->m_next)
	if (!strcmp (name, ptr->m_name)) 
		return ptr;
return NULL; // If we didn't find the cvar, give up
}

//------------------------------------------------------------------------------

void CCvar::Set (char* value)
{
	char*	text;

if (!(text = StrDup (value)))
	return;
if (m_text)
	delete[] m_text;
m_text = text;
m_value = strtod (value, reinterpret_cast<char **> (NULL));
}

//------------------------------------------------------------------------------

void CCvar::Set (const char* name, char* value)
{
	CCvar *ptr;

if ((ptr = Find (name)))
	ptr->Set (value);
}

//------------------------------------------------------------------------------

double CCvar::Value (const char* name)
{
	CCvar *ptr;

return ((ptr = Find (name))) ? ptr->m_value : 0.0;
}

//------------------------------------------------------------------------------

char* CCvar::Text (const char* name)
{
	CCvar *ptr;

return ((ptr = Find (name))) ? ptr->m_text : "0.0";
}

//------------------------------------------------------------------------------
//eof
