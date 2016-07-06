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
**	zoom.c : Functions to change the screen magnification
**
**	External Functions:
**	void
**	Zoom_Dialog_Func(Widget, XtPointer, XtPointer);
**	Puts up the dialog box for changing the magnification.
*/

#include <sced.h>
#include <View.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>

#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif


/* Define the longest any valid magnification string will be. */
#define MAX_DIM_STRING_LENGTH 10


static void 	Create_Zoom_Dialog();

static Widget	zoom_dialog_shell = NULL;
static Widget	zoom_dialog;
static WindowInfoPtr	current_target;
static char		magnification_string[MAX_DIM_STRING_LENGTH];


void
Zoom_Dialog_Func(Widget w, XtPointer client_data, XtPointer call_data)
{
	int		current_mag;

	if ( ! zoom_dialog_shell )
		Create_Zoom_Dialog();

	if ( client_data )
		current_target = (WindowInfoPtr)client_data;
	else
		current_target = Get_Active_Window();

	/* Get the magnification from the view widget. */
	XtVaGetValues(current_target->view_widget,
				XtNmagnification, &current_mag, NULL);

	/* Set the display string. */
	sprintf(magnification_string, "%0d", current_mag);
	XtVaSetValues(zoom_dialog, XtNvalue, magnification_string, NULL);

	/* Set the position of the popup. */
	SFpositionWidget(zoom_dialog_shell);

	XtPopup(zoom_dialog_shell, XtGrabExclusive);
}


static void
Magnification_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	char			*entered_string;
	int	new_mag;

	entered_string = XawDialogGetValueString(zoom_dialog);

	sscanf(entered_string, "%d", &new_mag);

	XtVaSetValues(current_target->view_widget, XtNmagnification, new_mag, NULL);

	View_Update(current_target, current_target->all_instances, CalcScreen);
	Update_Projection_Extents(current_target->all_instances);
	
}


static void
Zoom_Done_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	XtPopdown(zoom_dialog_shell);
}

static void
Magnify_To_Fit(Widget w, XtPointer cl_data, XtPointer ca_data) 
{
	InstanceList	elmt;
	int			old_mag;
	int			new_mag;
	Dimension	view_width;
	Dimension	view_height;
	Dimension	actual_width;
	Dimension	actual_height;
	int			screen_width;
	int			screen_height;
	int			object_width;
	int			object_height;
	int			center_x, center_y;
	Boolean		changed = FALSE;

	/* Get the desired width and height of the viewport. */
	/* Use desired values because they are guaranteed to always
	** be less than or equal to the actual values.
	** We still want the actual dimensions to calculate the centre.
	*/
	/* Also need the instances. */
	XtVaGetValues(current_target->view_widget,
				XtNdesiredWidth, &view_width,
				XtNdesiredHeight, &view_height,
				XtNwidth, &actual_width,
				XtNheight, &actual_height,
				XtNmagnification, &old_mag, NULL);

	if ( current_target == &main_window && (int)view_width && (int)view_height )
	{
		screen_width = (int)view_width / 2;
		screen_height = (int)view_height / 2;
	}
	else
	{
		screen_width = (int)actual_width / 2;
		screen_height = (int)actual_height / 2;
	}
	center_x = (int)actual_width / 2;
	center_y = (int)actual_height / 2;

	/* Get the maximum absolute extremity of any object. */
#define int_abs(a) ( (a) < 0 ? -(a) : (a) )
	object_width = object_height = 0;
	for ( elmt = current_target->all_instances ; elmt ; elmt = elmt->next )
	{
		if ( ! ( elmt->the_instance->o_flags & ObjVisible ) )
			continue;

		if ( int_abs(elmt->the_instance->o_proj_extent.min.x - center_x) >
			object_width )
			object_width =
				int_abs(elmt->the_instance->o_proj_extent.min.x - center_x);
		if ( int_abs(elmt->the_instance->o_proj_extent.max.x - center_x) >
			object_width )
			object_width =
				int_abs(elmt->the_instance->o_proj_extent.max.x - center_x);
		if ( int_abs(elmt->the_instance->o_proj_extent.min.y - center_y) >
			object_height )
			object_height =
				int_abs(elmt->the_instance->o_proj_extent.min.y - center_y);
		if ( int_abs(elmt->the_instance->o_proj_extent.max.y - center_y) >
			object_height )
			object_height =
				int_abs(elmt->the_instance->o_proj_extent.max.y - center_y);

		changed = TRUE;
	}
#undef int_abs

	if ( ! changed ) return;

	new_mag = (int)( min ( screen_width / (double)object_width,
						   screen_height / (double)object_height) * old_mag );

	if ( ! new_mag )
		new_mag = 1;

	/* Set the display string. */
	sprintf(magnification_string, "%0d", new_mag);
	XtVaSetValues(zoom_dialog, XtNvalue, magnification_string, NULL);

	Magnification_Func(NULL, NULL, NULL);
}


/*	void
**	Create_Zoom_Dialog()
**	Creates the popup shell to change the magnification.
*/
static void
Create_Zoom_Dialog()
{
	Arg		args[5];
	int		n;


	zoom_dialog_shell = XtCreatePopupShell("Zoom",
						transientShellWidgetClass, main_window.shell, NULL, 0);

	/* Create the dialog widget to go inside the shell. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Zoom:");	n++;
	XtSetArg(args[n], XtNvalue, "");		n++;
	zoom_dialog = XtCreateManagedWidget("zoomDialog", dialogWidgetClass,
						zoom_dialog_shell, args, n);

	/* Add the button at the bottom of the dialog. */
	XawDialogAddButton(zoom_dialog, "Apply", Magnification_Func, NULL);
	XawDialogAddButton(zoom_dialog, "To Fit", Magnify_To_Fit, NULL);
	XawDialogAddButton(zoom_dialog, "Done", Zoom_Done_Func, NULL);

	XtOverrideTranslations(XtNameToWidget(zoom_dialog, "value"),
		XtParseTranslationTable(":<Key>Return: Zoom_Action()"));

	XtVaSetValues(XtNameToWidget(zoom_dialog, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(zoom_dialog, "Apply"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(zoom_dialog, "To Fit"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(zoom_dialog, "Done"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(zoom_dialog_shell);
}


void
Zoom_Action_Func(Widget w, XEvent *e, String *s, Cardinal *c)
{
	Magnification_Func(NULL, NULL, NULL);
	XtPopdown(zoom_dialog_shell);
}

