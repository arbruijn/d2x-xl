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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "u_mem.h"
#include "strutil.h"
#include "error.h"
#include "hash.h"

//------------------------------------------------------------------------------

void CHashTable::Init (void)
{
m_bitSize = 0;
m_andMask = 0;
m_size = 0;
m_nItems = 0;
}

//------------------------------------------------------------------------------

int CHashTable::Create (int size)
{
	int i;

m_size = 0;
for (i = 1; i < 13; i++) {
	if ( (1 << i) >= size) {
		m_bitSize = i;
		m_size = 1<<i;
		break;
		}
	}
size = m_size;
m_andMask = m_size - 1;
if (m_size == 0)
	Error ("Hashtable has size of 0");
if (!m_key.Create (size))
	Error ("Not enough memory to create a hash table of size %d", size);
for (i = 0; i < size; i++)
	m_key [i] = NULL;
// Use calloc cause we want zero'd array.
if (!m_value.Create (size)) {
	m_key.Destroy ();
	Error ("Not enough memory to create a hash table of size %d\n", size);
	}
m_nItems = 0;
return 0;
}

//------------------------------------------------------------------------------

void CHashTable::Destroy (void)
{
m_key.Destroy ();
m_value.Destroy ();
m_size = 0;
}

//------------------------------------------------------------------------------

int CHashTable::GetKey (const char *key)
{
	int k = 0, i = 0;
	char c;

while ( (c = *key++)) {
	k ^= ( (int) (tolower (c))) << i;
	i++;
	}
return k;
}

//------------------------------------------------------------------------------

int CHashTable::Search (const char *key)
{
	int i, j, k;

k = GetKey (key);
i = 0;
while (i < m_size) {
	j = (k+ (i++)) & m_andMask;
	if (m_key [j] == NULL)
		return -1;
	if (!stricmp (m_key [j], key))
		return m_value [j];
	}
return -1;
}

//------------------------------------------------------------------------------

void CHashTable::Insert (const char *key, int value)
{
	int i,j,k;

k = GetKey (key);
i = 0;
while (i < m_size) {
	j = (k+ (i++)) & m_andMask;
	if (m_key [j] == NULL) {
		m_nItems++;
		m_key [j] = key;
		m_value [j] = value;
		return;
		} 
	else if (!stricmp (key, m_key [j])) {
		return;
		}
	}
Error ("Out of hash slots\n");
}

//------------------------------------------------------------------------------
