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
**	csg_select_box.c : Functions for displaying, modifying and using the
**						select csg object dialog.
*/

#include <sced.h>
#include <csg.h>
#include <SimpleWire.h>
#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Viewport.h>

extern void	Create_New_Object(Widget, XtPointer, XtPointer);

static void	CSG_Select_Callback(Widget, XtPointer, XtPointer);
static void	Create_CSG_Select_Shell();

static int		csg_select_state;

static Widget	*csg_select_widgets;
static int		num_csg_widgets = 0;
static int		max_num_csg_widgets = 0;

static Widget	csg_select_shell = NULL;
static Widget	csg_select_form;




void
CSG_Select_Popup(int state)
{
	csg_select_state = state;

	if ( ! num_csg_widgets ) return;

	SFpositionWidget(csg_select_shell);
	XtPopup(csg_select_shell, XtGrabExclusive);
}

void
CSG_Add_Select_Option(BaseObjectPtr base)
{
	Dimension	form_width, form_height;
	int			gap;
	Arg			args[10];
	int			n;

	if ( ! csg_select_shell )
		Create_CSG_Select_Shell();

	if ( num_csg_widgets == max_num_csg_widgets )
	{
		if ( max_num_csg_widgets )
			csg_select_widgets = More(csg_select_widgets, Widget,
									  max_num_csg_widgets + 5);
		else
			csg_select_widgets = New(Widget, max_num_csg_widgets + 5);
		max_num_csg_widgets += 5;
	}

	XtVaGetValues(csg_select_form, XtNwidth, &form_width,
								   XtNheight, &form_height,
								   XtNdefaultDistance, &gap,
								   NULL);

	n = 0;
	XtSetArg(args[n], XtNbasePtr, base);	n++;
	XtSetArg(args[n], XtNwidth, 118);		n++;
	XtSetArg(args[n], XtNheight, 118);		n++;
	csg_select_widgets[num_csg_widgets] = XtCreateManagedWidget("csgNewObject",
											simpleWireWidgetClass,
											csg_select_form, args, n);
	XtAddCallback(csg_select_widgets[num_csg_widgets], XtNcallback,
				  CSG_Select_Callback, NULL);
	num_csg_widgets++;

}


static void
CSG_Select_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(csg_select_shell);

	switch ( csg_select_state )
	{
		case csg_select_new:
			Create_New_Object(w, NULL, NULL);
			break;
		case csg_select_edit:
			CSG_Destroy_Base_Object(w, NULL, FALSE);
			break;
		case csg_select_delete:
			CSG_Destroy_Base_Object(w, NULL, TRUE);
			break;
		case csg_select_copy:
			CSG_Copy_Base_Object(w, NULL);
			break;
		case csg_select_save:
			OFF_Save_Wireframe(w, NULL);
			break;
		case csg_select_change:
			Base_Change_Select_Callback(w, NULL);
			break;
	}
}

static void
CSG_Select_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(csg_select_shell);
}


void
CSG_Select_Destroy_Widget(int index)
{
	int	i;

	/* Destroy it's widget. */
	XtDestroyWidget(csg_select_widgets[index]);
	for ( i = index + 1 ; i < num_csg_widgets ; i++ )
		csg_select_widgets[i-1] = csg_select_widgets[i];
	num_csg_widgets--;
}


static void
Create_CSG_Select_Shell()
{
	Widget	outer_form;
	Widget	cancel_button;
	Widget	viewport_widget;
	String	shell_geometry;
	unsigned	shell_width, shell_height;
	int		junk;
	int		gap;
	Arg		args[15];
	int		n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "CSG Select");	n++;
	csg_select_shell = XtCreatePopupShell("csgSelectShell",
						transientShellWidgetClass, main_window.shell, args, n);

	XtRealizeWidget(csg_select_shell);

	/* Get the parents size. */
	XtVaGetValues(csg_select_shell, XtNgeometry, &shell_geometry, NULL);
	XParseGeometry(shell_geometry, &junk, &junk, &shell_width, &shell_height);

	/* Create the first level form. */
	n = 0;
	XtSetArg(args[n], XtNwidth, (int)shell_width);		n++;
	XtSetArg(args[n], XtNheight, (int)shell_height);	n++;
	outer_form = XtCreateManagedWidget("csgSelectOuterForm", formWidgetClass,
										csg_select_shell, args, n);
	XtVaGetValues(outer_form, XtNdefaultDistance, &gap, NULL);

	/* Create the viewport. */
	n = 0;
	XtSetArg(args[n], XtNwidth, (int)(shell_width - 2 * gap));		n++;
	XtSetArg(args[n], XtNheight, (int)(shell_height - 35 - 3*gap));	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainRight);		n++;
	XtSetArg(args[n], XtNallowHoriz, TRUE);			n++;
	XtSetArg(args[n], XtNallowVert, TRUE);			n++;
	XtSetArg(args[n], XtNforceBars, TRUE);			n++;
	XtSetArg(args[n], XtNuseBottom, TRUE);			n++;
	XtSetArg(args[n], XtNuseRight, TRUE);			n++;
	viewport_widget = XtCreateManagedWidget("csgSelectViewport",
						viewportWidgetClass, outer_form, args, n);

	/* Create the box inside the viewport. */
	n = 0;
	XtSetArg(args[n], XtNwidth, (int)(shell_width - 2 * gap - 40));		n++;
	XtSetArg(args[n], XtNheight, (int)(shell_height - 35 - 3*gap - 40));n++;
	csg_select_form = XtCreateManagedWidget("csgSelectForm", boxWidgetClass,
											viewport_widget, args, n);

	/* The cancel button. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Cancel");			n++;
	XtSetArg(args[n], XtNfromVert, viewport_widget);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainLeft);		n++;
	XtSetArg(args[n], XtNtop, XtChainBottom);		n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);			n++;
	cancel_button = XtCreateManagedWidget("csgSelectCancel", commandWidgetClass,
										  outer_form, args, n);
	XtAddCallback(cancel_button, XtNcallback, CSG_Select_Cancel_Callback, NULL);
}
