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
**	instance_list.c : functions for manipulation of instance lists.
**
**	External functions:
**
**	void
**	Insert_Element(InstanceList *dest, ObjectinstancePtr obj)
**	Inserts the new object at the head of the list dest.
**	It will still work even if dest is not really the head of the list.
**
**	void
**	Append_Element(InstanceList *dest, InstanceList *new)
**	Inserts the node new at the end of the list dest.
**
**	InstanceList
**	Delete_Element(InstanceList victim)
**	Removes victim from whatever list it is a member of, and returns victim.
**
**	InstanceList
**	Find_Object_In_Instances(ObjectInstancePtr obj, InstanceList list)
**	Searches for an element containing obj in the list list.
**	Returns a pointer to the InstanceList elmt which contains obj.
**
**	InstanceList
**	Merge_Selection_Lists(InstanceList l1, InstanceList l2)
**	Merges l1 and l2 into a new list containing no duplicates.
**	l1 and l2 are returned unchanged.
**
**  InstanceList
**	Remove_Selection_List(InstanceList src, InstanceList victims)
**	Removes those elements of the victim list which appear in the src list
**	from the src list.  Returns the new src list.  The old one is left
**	unmodified.
**
**	ObjectInstancePtr
**	Delete_Instance(InstanceList *list, InstanceList *victim)
**	Removes the instance victim from list, returning the_instance.
**
**	void
**	Free_Selection_List(InstanceList victim)
**	Frees the nodes of an instance list without freeing the objects.
*/

#include <sced.h>
#include <instance_list.h>


void
Insert_Element(InstanceList *dest, ObjectInstancePtr obj)
{
	InstanceList new;

	new = New(InstanceListElmt, 1);
	new->the_instance = obj;

	if ((*dest) == NULL)
	{
		new->prev = new->next = NULL;
		(*dest) = new;
	}
	else
	{
		new->next = (*dest);
		new->prev = (*dest)->prev;
		(*dest)->prev = new;
		if (new->prev)
			new->prev->next = new;
		else
			(*dest) = new;
	}
}

void
Insert_Instance_List(InstanceList *dest, InstanceList *src)
{
	Append_Instance_List(src, *dest);
}


/*	void
**	Append_Element(InstanceList *dest, ObjectInstancePtr new)
**	Appends the instance new at the end of the list dest.
*/
InstanceList
Append_Element(InstanceList *dest, ObjectInstancePtr new)
{
	InstanceList	tmp;
	InstanceList	new_elmt;

	new_elmt = New(InstanceListElmt, 1);
	new_elmt->the_instance = new;

	if ((*dest) == NULL)
	{
		new_elmt->prev = new_elmt->next = NULL;
		(*dest) = new_elmt;
	}
	else
	{
		
		for ( tmp = *dest ; tmp->next ; tmp = tmp->next );

		new_elmt->next = NULL;
		new_elmt->prev = tmp;
		tmp->next = new_elmt;
	}

	return new_elmt;
}


/*	void
**	Append_Instance_List(InstanceList *dest, InstanceList *new)
**	Appends the list new at the end of the list dest.
*/
void
Append_Instance_List(InstanceList *dest, InstanceList new)
{
	InstanceList	tmp;

	if ((*dest) == NULL)
		(*dest) = new;
	else
	{
		
		for ( tmp = *dest ; tmp->next ; tmp = tmp->next );

		if ( new ) new->prev = tmp;
		tmp->next = new;
	}
}


/*	InstanceList
**	Delete_Element(InstanceList victim)
**	Removes victim from whatever list it is a member of, and returns victim.
*/
InstanceList
Delete_Element(InstanceList victim)
{
	debug(FUNC_NAME,fprintf(stderr,"Delete_Element()\n"));

	if (!victim) return NULL;

	if (victim->prev)
		victim->prev->next = victim->next;

	if (victim->next)
		victim->next->prev = victim->prev;

	return victim;
}


/*	InstanceList
**	Find_Object_In_Instances(ObjectInstancePtr obj, InstanceList list)
**	Searches for an element containing obj in the list list.
**	Returns a pointer to the InstanceList elmt which contains obj.
*/
InstanceList
Find_Object_In_Instances(ObjectInstancePtr obj, InstanceList list)
{
	InstanceList    elmt;

	for ( elmt = list ; elmt != NULL ; elmt = elmt->next )
	{
		if ( elmt->the_instance == obj )
			return elmt;
	}

	return NULL;

}



/*	InstanceList
**	Merge_Selection_Lists(InstanceList l1, InstanceList l2)
**	Merges l1 and l2 into a new list containing no duplicates.
**	l1 and l2 are returned unchanged.
*/
InstanceList
Merge_Selection_Lists(InstanceList l1, InstanceList l2)
{
	InstanceList	src_elmt;
	InstanceList	result = NULL;
	
	/* Insert all the elements from l1. */
	for ( src_elmt = l1 ; src_elmt != NULL ; src_elmt = src_elmt->next )
		Insert_Element(&result, src_elmt->the_instance);

	/* Insert those elements of l2 not already there. */
	for ( src_elmt = l2 ; src_elmt != NULL ; src_elmt = src_elmt->next )
	{
		if (!Find_Object_In_Instances(src_elmt->the_instance, result))
			Insert_Element(&result, src_elmt->the_instance);
	}

	return result;
}



/*  InstanceList
**	Remove_Selection_List(InstanceList src, InstanceList victims)
**	Removes those elements of the victim list which appear in the src list
**	from the src list.  Returns the new src list.  The old one is left
**	unmodified.
*/
InstanceList
Remove_Selection_List(InstanceList src, InstanceList victims)
{
	InstanceList	src_elmt;
	InstanceList	victim_elmt;
	InstanceList	result = NULL;

	/* This uses the simple algorithm:
	**	check for each element of src in victims, and if it's not there
	**	copy it over.
	*/
	for ( src_elmt = src ; src_elmt != NULL ; src_elmt = src_elmt->next )
	{
		if (!(victim_elmt = Find_Object_In_Instances(src_elmt->the_instance,
			victims)))
			Insert_Element(&result, src_elmt->the_instance);
	}

	return result;

}


/*	ObjectInstancePtr
**	Delete_Instance(InstanceList *list, InstanceList *victim)
**	Removes the instance victim from list, returning the_instance.
*/
ObjectInstancePtr
Delete_Instance(InstanceList victim)
{
	ObjectInstancePtr	res;

	res = victim->the_instance;
	Delete_Element(victim);
	free(victim);
	return res;
}


/*	void
**	Free_Selection_List(InstanceList victim)
**	Frees the nodes of an instance list without freeing the objects.
*/
void
Free_Selection_List(InstanceList victim)
{
	InstanceList	elmt;
	InstanceList	tmp;

	for ( elmt = victim ; elmt != NULL ; )
	{
		tmp = elmt->next;
		free(elmt);
		elmt = tmp;
	}
}

