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

int HashTableInit( tHashTable *ht, int size )
{
	int i;

ht->size=0;
for (i=1; i<13; i++ )	{
	if ( (1<<i) >= size )	{
		ht->bitsize = i;
		ht->size = 1<<i;
		break;
		}
	}
size = ht->size;
ht->and_mask = ht->size - 1;
if (ht->size==0)
	Error( "Hashtable has size of 0" );
if (!(ht->key = new const char * [size]))
	Error( "Not enough memory to create a hash table of size %d", size );
for (i = 0; i < size; i++ )
	ht->key [i] = NULL;
// Use calloc cause we want zero'd array.
ht->value = new int [size];
if (ht->value==NULL)	{
	D2_FREE(ht->key);
	Error( "Not enough memory to create a hash table of size %d\n", size );
	}
ht->nitems = 0;
return 0;
}


void HashTableFree( tHashTable *ht )
{
if (ht->key != NULL)
	D2_FREE( ht->key);
if (ht->value != NULL)
	D2_FREE( ht->value);
ht->size = 0;
}


int HashTableGetKey( const char *key )
{
	int k = 0, i=0;
	char c;

while ((c = *key++)) {
	k ^= ((int)(tolower (c))) << i;
	i++;
	}
return k;
}


int HashTableSearch( tHashTable *ht, const char *key )
{
	int i,j,k;

k = HashTableGetKey( key );
i = 0;
while(i < ht->size )	{
	j = (k+(i++)) & ht->and_mask;
	if ( ht->key[j] == NULL )
		return -1;
	if (!stricmp(ht->key[j], key ))
		return ht->value[j];
	}
return -1;
}


void HashTableInsert( tHashTable *ht, const char *key, int value )
{
	int i,j,k;

k = HashTableGetKey(key);
i = 0;
while(i < ht->size) {
	j = (k+(i++)) & ht->and_mask;
	if (ht->key [j] == NULL) {
		ht->nitems++;
		ht->key [j] = key;
		ht->value [j] = value;
		return;
		} 
	else if (!stricmp (key, ht->key[j])) {
		return;
		}
	}
Error( "Out of hash slots\n" );
}
