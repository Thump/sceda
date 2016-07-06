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
**	edit_menu.c : Functions for handling objects on the edit menu.
**					They add, delete etc.
*/


#include <sced.h>
#include <instance_list.h>
#include <layers.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/SimpleMenu.h>

void Edit_Menu_Callback(Widget, XtPointer, XtPointer);

void
Create_Edit_Menu(WindowInfoPtr window)
{
	Arg	args[3];
	int	n;

	/* Create a structure to hold everything. */
	window->edit_menu = New(MenuInfo, 1);

	/* Create the menu itself. */
	window->edit_menu->menu = XtCreatePopupShell("EditMenu",
						simpleMenuWidgetClass, window->shell, NULL, 0);

	/* Put a line at the top, just to give it width and height. */
	n = 0;
	XtSetArg(args[n], XtNlineWidth, 4);	n++;
	XtSetArg(args[n], XtNwidth, 50);	n++;
	XtCreateManagedWidget("menuLine", smeLineObjectClass,
										window->edit_menu->menu, args, n);

	/* Give it an arbitrary number of children for starters. */
	window->edit_menu->num_children = 0;
	window->edit_menu->max_children = 5;
	window->edit_menu->children = New(Widget, 5);

}


/*	void
**	Edit_Objects_Function(Widget w, XtPointer cl_data, XtPointer ca_data)
**	For a WindowInfoPtr passed as cl_data, adds the selected objects
**	to the edit_objects.
*/
void
Edit_Objects_Function(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	WindowInfoPtr	window = (WindowInfoPtr)cl_data;

	Add_Instance_To_Edit(window, window->selected_instances, TRUE);
}


/*	void
**	Add_Instance_To_Edit(WindowInfoPtr window, InstanceList extras)
**	Adds the new elmt to the list of edit objects for window.
**	This also adds a new menu entry for the object.
*/
void
Add_Instance_To_Edit(WindowInfoPtr window, InstanceList extras, Boolean exists)
{
	InstanceList	src_elmt;
	InstanceList	new_elmt = NULL;
	char			name[20];
	Arg				args[3];
	int				n;
	int				count = 0;
	Boolean			have_new = FALSE;

	for ( src_elmt = extras ; src_elmt != NULL ; src_elmt = src_elmt->next )
	{
		count++;

		/* Add it to the place list if it's not already there. */
		if ( ( new_elmt = Find_Object_In_Instances(src_elmt->the_instance,
												 window->edit_instances) ) )
			continue;

		if ( Layer_Is_Visible(src_elmt->the_instance->o_layer) )
		{
			new_elmt = Append_Element(&(window->edit_instances),
									  src_elmt->the_instance);

			/* Add a widget for it. */
			if ( window->edit_menu->num_children ==
				 window->edit_menu->max_children )
			{
				window->edit_menu->max_children += 5;
				window->edit_menu->children = More(window->edit_menu->children,
									Widget, window->edit_menu->max_children);
			}

			sprintf(name, "editEntry%d", window->edit_menu->num_children);
			n = 0;
			XtSetArg(args[n], XtNlabel, src_elmt->the_instance->o_label);	n++;
			window->edit_menu->children[window->edit_menu->num_children] =
				XtCreateManagedWidget(name, smeBSBObjectClass,
										window->edit_menu->menu, args, n);
			XtAddCallback(
				window->edit_menu->children[window->edit_menu->num_children],
				XtNcallback, Edit_Menu_Callback, (XtPointer)window);

			have_new = TRUE;
			window->edit_menu->num_children++;
		}

		if ( ! exists )
		{
			new_elmt = Append_Element(&(window->all_instances),
									  src_elmt->the_instance);
			if ( Layer_Is_Visible(new_elmt->the_instance->o_layer) )
				new_elmt->the_instance->o_flags |= ObjVisible;

			/* Only one can have been added, so do the redraw here. */
			View_Update(window, new_elmt, CalcView);
			Update_Projection_Extents(new_elmt);

			changed_scene = TRUE;
		}
	}

	if ( have_new )
		XtVaSetValues(window->edit_menu->button, XtNsensitive, TRUE, NULL);

	if ( exists && count == 1 )
		Edit_Instance(window, new_elmt);
}



/*	void
**	Delete_Edit_Instance(WindowInfoPtr window, InstanceList victim)
**	Deletes the victim from the place menu.
*/
void
Delete_Edit_Instance(WindowInfo *window, InstanceList victim)
{
	InstanceList	elmt;
	int	i, j;

	/* Find which widget it corresponds to. */
	for ( i = 0, elmt = window->edit_instances ;
		  elmt != victim ;
		  i++, elmt = elmt->next );

	XtDestroyWidget(window->edit_menu->children[i]);

	/* Shuffle the others down. */
	for ( j = i + 1 ; j < window->edit_menu->num_children ; j++ )
		window->edit_menu->children[j-1] = window->edit_menu->children[j];

	window->edit_menu->num_children--;

	if ( ! window->edit_menu->num_children )
		XtVaSetValues(window->edit_menu->button, XtNsensitive, FALSE, NULL);

	if ( window->edit_instances == victim )
		window->edit_instances = window->edit_instances->next;
	Delete_Element(victim);
	free(victim);
}


/*	void
**	Edit_Menu_Callback(Widget w, XtPointer cl_data, XtPointer ca)
**	The callback for all the children in the place menu.
**	Calls the place object function for the appropriate entry.
*/
void
Edit_Menu_Callback(Widget w, XtPointer cl_data, XtPointer ca)
{
	WindowInfo	*window = (WindowInfo*)cl_data;
	InstanceList	elmt;
	int	i;

	/* Find which widget it corresponds to. */
	for ( i = 0, elmt = window->edit_instances ;
		  window->edit_menu->children[i] != w ;
		  i++, elmt = elmt->next );

	Edit_Instance(window, elmt);
}

