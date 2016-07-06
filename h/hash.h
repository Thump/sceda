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

#define PATCHLEVEL 0
/*
**	sced: A Constraint Based Object Scene Editor
**
**	hash.h: Hash table functions for storing pointer mappings.
*/

typedef struct _HashEntry {
	long	key;
	void	*value;
	struct _HashEntry	*next;
	} HashEntry, *HashEntryPtr;

typedef HashEntryPtr *HashTable;

extern HashTable	Hash_New_Table();
extern void	Hash_Insert(HashTable, long, void*);
extern void	*Hash_Get_Value(HashTable, long);
extern void	Hash_Free(HashTable);
extern void	Hash_Clear(HashTable);
extern void	*Hash_Delete(HashTable, long);
extern void	*Hash_Traverse(HashTable, Boolean);

