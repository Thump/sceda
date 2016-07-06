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
**	edit_dfs.c : Functions to perform Depth-First-Search on constraint
**				 dependency graphs.
*/

#include <sced.h>
#include <instance_list.h>

static unsigned long	mark = 0;


/*	Boolean
**	DFS_Main(...)
**	Performs a depth first search on the dependency graph starting from
**	src node. Each of objn are checked for cycles if the corresponding usen
**	flag is set. If make_list is true, returns a topologically sorted
**	list of the nodes (actually reverse topological).
**	Returns TRUE immediately if a cycle is found, else FALSE.
*/
static Boolean
DFS_Main(ObjectInstancePtr src, ObjectInstancePtr *checks, int num_check,
		Boolean make_list, InstanceList *list_ret)
{
	int	i, j;

	src->o_dfs_mark = mark;

	for ( i = 0 ; i < src->o_num_depend ; i++ )
	{
		for ( j = 0 ; j < num_check ; j++ )
			if ( checks[j] == src->o_dependents[i].obj )
				return TRUE;

		if ( src->o_dependents[i].obj->o_dfs_mark < mark &&
			 DFS_Main(src->o_dependents[i].obj, checks, num_check,
					  make_list, list_ret) )
			return TRUE;
	}

	if ( make_list )
	{
		src->o_flags |= ObjDepends;
		Insert_Element(list_ret, src);
	}

	src->o_dfs_mark = mark + 1;

	return FALSE;

}


/* DFS(..)
** The front end function.
*/
Boolean
DFS(ObjectInstancePtr src, ObjectInstancePtr *checks, int num_check,
	Boolean make_list, InstanceList *list_ret)
{ 
	Boolean	res;

	debug(FUNC_NAME,fprintf(stderr,"DFS()\n"));

	mark += 2;

	res = DFS_Main(src, checks, num_check, make_list, list_ret);

	src->o_flags &= ( ObjAll ^ ObjDepends );

	return res;
}


