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
**	quit.c : The quit dialog box function.
**
**	void Quit_Dialog_Func(Widget, XtPointer, XtPointer);
**	Puts up a quit dialog if necessary, otherwise just dies.
*/

#include <unistd.h>
#include <sced.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Command.h>

static void 	Create_Quit_Dialog();
void	Remove_Temporaries();

static Widget	quit_dialog_shell;

void
Quit_Dialog_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	if (!changed_scene)
	{
		Remove_Temporaries();
		exit(0);
	}

	if (!quit_dialog_shell)
		Create_Quit_Dialog();

	/* Set the position of the popup. */
	SFpositionWidget(quit_dialog_shell);

	XtPopup(quit_dialog_shell, XtGrabExclusive);
}

void
Quit_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	Remove_Temporaries();

	exit(0);
}

static void
Cancel_Quit_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	XtPopdown(quit_dialog_shell);
}

static void
Save_Quit_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	XtPopdown(quit_dialog_shell);

	Save_Dialog_Func(NULL, (void*)SAVE_QUIT, NULL);
}


/*	void
**	Create_Quit_Dialog()
**	Creates the popup shell to for use when quitting.
*/
static void
Create_Quit_Dialog()
{
	Widget	dialog_widget;
	Arg		args[5];
	int		n;


	quit_dialog_shell = XtCreatePopupShell("Quit",
						transientShellWidgetClass, main_window.shell, NULL, 0);

	/* Create the dialog widget to go inside the shell. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "The scene has changed:");	n++;
	dialog_widget = XtCreateManagedWidget("quitDialog", dialogWidgetClass,
						quit_dialog_shell, args, n);

	/* Add the button at the bottom of the dialog. */
	XawDialogAddButton(dialog_widget, "Save", Save_Quit_Func, NULL);
	XawDialogAddButton(dialog_widget, "Quit", Quit_Func, NULL);
	XawDialogAddButton(dialog_widget, "Cancel", Cancel_Quit_Func, NULL);

	XtVaSetValues(XtNameToWidget(dialog_widget, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(dialog_widget, "Save"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(dialog_widget, "Quit"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(dialog_widget, "Cancel"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(quit_dialog_shell);
}

void
Remove_Temporaries()
{
	int	i;

	/* Delete the temporary files. */
	for ( i = 0 ; i < num_temp_files ; i++ )
		unlink(temp_filenames[i]);
}
