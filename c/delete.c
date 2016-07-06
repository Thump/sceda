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
**	delete.c : Object deletion functions.
*/


#include <sced.h>
#include <instance_list.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Command.h>

static void Create_Deletion_Dialog();
static void Do_Delete(Widget, XtPointer, XtPointer);
static void Cancel_Delete(Widget, XtPointer, XtPointer);

static Widget	deletion_shell = NULL;

static WindowInfoPtr	current_window;

/*	void
**	Delete_Objects_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**	Warns first, then deletes the objects in the selection list.
*/
void
Delete_Objects_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	current_window = &main_window;

	if ( ! deletion_shell )
		Create_Deletion_Dialog();

	SFpositionWidget(deletion_shell);

	XtPopup(deletion_shell, XtGrabExclusive);
}


/*	void
**	Do_Delete(Widget w, XtPointer cl_data, XtPointer ca_data)
**	Actually does the deletion.
*/
static void
Do_Delete(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	InstanceList		elmt;
	InstanceList		victim;
	ObjectInstancePtr	obj;

	XtPopdown(deletion_shell);

	for ( elmt = current_window->selected_instances ; elmt ; elmt = elmt->next )
	{
		victim = Find_Object_In_Instances(elmt->the_instance,
											current_window->all_instances);
		if ( current_window->all_instances == victim )
			current_window->all_instances = victim->next;
		Delete_Element(victim);

		obj = victim->the_instance;
		free(victim);

		/* Check for the victim in the edit lists. */
		if ( ( victim = Find_Object_In_Instances(obj,
											current_window->edit_instances) ) )
			Delete_Edit_Instance(current_window, victim);

		Destroy_Instance(obj);
	}

	Free_Selection_List(current_window->selected_instances);
	current_window->selected_instances = NULL;

	View_Update(current_window, current_window->all_instances, ViewNone);
}



/*	void
**	Cancel_Delete(Widget w, XtPointer cl_data, XtPointer ca_data)
**	Cancels the deletion.
*/
static void
Cancel_Delete(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	XtPopdown(deletion_shell);
}


/*	void
**	Create_Deletion_Dialog()
**	Creates the dialog for object deletion.
*/
static void
Create_Deletion_Dialog()
{
	Widget	deletion_dialog;
	Arg		args[5];
	int		n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Delete");	n++;
	deletion_shell = XtCreatePopupShell("deleteShell",
					transientShellWidgetClass, current_window->shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Confirm Deletion!");	n++;
	deletion_dialog = XtCreateManagedWidget("deleteDialog", dialogWidgetClass,	
											deletion_shell, args, n);

	XawDialogAddButton(deletion_dialog, "Confirm", Do_Delete, NULL);
	XawDialogAddButton(deletion_dialog, "Cancel", Cancel_Delete, NULL);

	XtVaSetValues(XtNameToWidget(deletion_dialog, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(deletion_dialog, "Confirm"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(deletion_dialog, "Cancel"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(deletion_shell);
}

