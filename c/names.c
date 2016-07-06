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
**	names.c : code for changing names.
**
**	External Functions:
**	Rename_Callback(Widget, XtPointer, XtPointer)
**	Pops up a rename dialog for each of the selected objects in turn.
*/

#include <sced.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Command.h>

extern void	CSG_Rename_Instance(ObjectInstancePtr, char*);

static void	Create_Name_Dialog();
static void	Popup_Name_Shell();

static Widget		name_dialog_shell = NULL;
static Widget		name_dialog;
static InstanceList	current_elmt;
static WindowInfoPtr	current_window;


/*	void
**	Rename_Callback(Widget widg, XtPointer ca, XtPointer cl)
**	Pops up a rename dialog for each of the selected objects in turn.
*/
void
Rename_Callback(Widget widg, XtPointer ca, XtPointer cl)
{
	if ( ! name_dialog_shell )
		Create_Name_Dialog();

	current_window = (WindowInfoPtr)ca;

	current_elmt = current_window->selected_instances;

	Popup_Name_Shell();
}



static void
Popup_Name_Shell()
{
	if ( ! current_elmt )
		return;

	/* Set the string up. */
	XtVaSetValues(name_dialog, XtNvalue, current_elmt->the_instance->o_label,
				NULL);

	/* Pop the dialog up. */
	SFpositionWidget(name_dialog_shell);
	XtPopup(name_dialog_shell, XtGrabExclusive);
}


static void
Rename_Func(Widget w, XtPointer ca, XtPointer cl)
{
	char	*new_name;

	XtPopdown(name_dialog_shell);

	new_name = XawDialogGetValueString(name_dialog);

	if ( current_window == &csg_window )
		CSG_Rename_Instance(current_elmt->the_instance, new_name);
	else
		Rename_Instance(current_elmt->the_instance, new_name);

	current_elmt = current_elmt->next;

	Popup_Name_Shell();
}


static void
Cancel_Rename_Func(Widget w, XtPointer ca, XtPointer cl)
{
	XtPopdown(name_dialog_shell);

	current_elmt = current_elmt->next;

	Popup_Name_Shell();
}



static void
Cancel_All_Func(Widget w, XtPointer ca, XtPointer cl)
{
	XtPopdown(name_dialog_shell);
}



static void
Create_Name_Dialog()
{
	Arg	args[2];
	int	n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Rename");	n++;
	name_dialog_shell = XtCreatePopupShell("nameShell",
			transientShellWidgetClass, main_window.shell, args, n);

	/* Create the contents, with a suitable label. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Object's name:");	n++;
	XtSetArg(args[n], XtNvalue, "");				n++;
	name_dialog = XtCreateManagedWidget("nameDialog", dialogWidgetClass,
										name_dialog_shell, args, n);

	/* Add command buttons to the bottom. */
	XawDialogAddButton(name_dialog, "Rename", Rename_Func, NULL);
	XawDialogAddButton(name_dialog, "Cancel", Cancel_Rename_Func, NULL);
	XawDialogAddButton(name_dialog, "Cancel All", Cancel_All_Func, NULL);

	/* Add translations. */
	XtOverrideTranslations(XtNameToWidget(name_dialog, "value"),
		XtParseTranslationTable(":<Key>Return: Rename_Object()"));

	XtVaSetValues(XtNameToWidget(name_dialog, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(name_dialog, "Rename"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(name_dialog, "Cancel"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(name_dialog, "Cancel All"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(name_dialog_shell);
}


void
Rename_Action_Func(Widget w, XEvent *e, String *s, Cardinal *c)
{
	Rename_Func(NULL, NULL, NULL);
}
