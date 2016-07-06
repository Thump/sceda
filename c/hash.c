#define PATCHLEVEL 0
/*
**    ScEd: A Constraint Based Scene Editor.
**    Copyright (C) 1994-1995  Stephen Chenney (stephen@cs.su.oz.au)
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program; if not, write to the Free Software
**    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
**	sced: A Constraint Based Object Scene Editor
**
**	hash.c: Hash table functions for storing pointer mappings.
*/

#include <sced.h>
#include <hash.h>


#define HASH_PRIME 197
#define HASH_SIZE ( 197 / 2 + 1 )
#define HASH_INDEX ( ( key % HASH_PRIME ) >> 1 )

HashTable
Hash_New_Table()
{
	HashTable	new_hash = New(HashEntryPtr, HASH_SIZE);
	int	i;

	for ( i = 0 ; i < HASH_SIZE ; i++ )
		new_hash[i] = NULL;
	return new_hash;
}


void
Hash_Insert(HashTable table, long key, void *value)
{
	int				index = HASH_INDEX;
	HashEntryPtr	entry = table[index];
	HashEntryPtr	new_entry;

	while ( entry )
	{
		if ( entry->key == key )
			return;
		entry = entry->next;
	}
	new_entry = New(HashEntry, 1);
	new_entry->next = table[index];
	new_entry->key = key;
	new_entry->value = value;
	table[index] = new_entry;
}


void*
Hash_Get_Value(HashTable table, long key)
{
	HashEntryPtr	entry = table[HASH_INDEX];

	while ( entry && entry->key != key )
		entry = entry->next;
	if ( entry )
		return entry->value;
	else
		return (void*)-1;
}


void
Hash_Free_List(HashEntryPtr victim)
{
	if ( victim )
	{
		Hash_Free_List(victim->next);
		free(victim);
	}
}


void
Hash_Clear(HashTable table)
{
	int	i;

	for ( i = 0 ; i < HASH_SIZE ; i++ )
		if ( table[i] )	Hash_Free_List(table[i]);
}

void
Hash_Free(HashTable table)
{
	Hash_Clear(table);
	free(table);
}


void*
Hash_Delete(HashTable table, long key)
{
	HashEntryPtr	entry = table[HASH_INDEX];
	void			*val;

	if ( ! entry )
		return NULL;

	if ( entry->key == key )
	{
		table[HASH_INDEX] = entry->next;
		val = entry->value;
		free(entry);
		return val;
	}

	while ( entry->next && entry->next->key != key )
		entry = entry->next;

	if ( entry->next && entry->next->key == key )
	{
		HashEntryPtr	temp = entry->next;

		val = entry->next->value;
		entry->next = entry->next->next;
		free(temp);
		return val;
	}

	return NULL;
}


void*
Hash_Traverse(HashTable table, Boolean starting)
{
	static HashEntryPtr	next;
	static int			index;
	void				*result;

	if ( starting )
	{
		for ( index = 0 ; index < HASH_SIZE && ! table[index] ; index++ );
		if ( index < HASH_SIZE )
			next = table[index];
		else
			next = NULL;
	}

	result = next ? next->value : NULL;

	if ( next && next->next )
		next = next->next;
	else
	{
		for ( index++ ; index < HASH_SIZE && ! table[index] ; index++ );
		if ( index < HASH_SIZE )
			next = table[index];
		else
			next = NULL;
	}

	return result;
}


