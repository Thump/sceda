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
**	view_recall.c : Functions for saving and recalling views.
**
**	Created: Sept 94 (Very late one night).
*/


#include <sced.h>
#include <edit.h>
#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/List.h>
#include <View.h>


static void	View_Save_Create_Shell();
static void	View_Recall_Create_Shell();


Viewport	*view_list;			/* The saved viewports. */
String		*label_list;		/* The list of strings representing them. */
int			num_views = 0;		/* The current number saved. */
static int		max_num_views = 0;	/* The number there's space for. */

static WindowInfoPtr	current_window;

static Boolean	deleting = FALSE;

static Widget	recall_shell = NULL;
static Widget	recall_list;

static Widget	save_name_shell = NULL;
static Widget	save_dialog;

void
View_Save_Current_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	char	default_name[20];

	if ( cl )
		current_window = (WindowInfoPtr)cl;
	else
		current_window = Edit_Get_Info()->window;

	if ( ! save_name_shell )
		View_Save_Create_Shell();

	sprintf(default_name, "Viewport_%d", num_views);
	XtVaSetValues(save_dialog, XtNvalue, default_name, NULL);

	SFpositionWidget(save_name_shell);
	XtPopup(save_name_shell, XtGrabExclusive);
}


static void
View_Name_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	char	*entered_name;

	XtPopdown(save_name_shell);

	entered_name = XawDialogGetValueString(save_dialog);

	XtVaGetValues(current_window->view_widget,
				  XtNwidth, &(current_window->viewport.scr_width),
				  XtNheight, &(current_window->viewport.scr_height),
				  XtNmagnification, &(current_window->viewport.magnify),
				  NULL);

	current_window->viewport.is_default = FALSE;

	View_Save(&(current_window->viewport), entered_name);
}

void
View_Save(Viewport *view, char *name)
{
	if ( ! recall_shell )
		View_Recall_Create_Shell();

	if ( num_views >= max_num_views )
	{
		if ( max_num_views )
		{
			max_num_views += 5;
			view_list = More(view_list, Viewport, max_num_views);
		}
		else
		{
			max_num_views = 5;
			view_list = New(Viewport, max_num_views);
		}
		label_list = More(label_list, String, max_num_views + 1);
	}

	view_list[num_views] = *view;
	num_views++;
	label_list[num_views] = Strdup(name);
	XawListChange(recall_list, label_list, num_views + 1, 0, TRUE);
}


void
View_Name_Action_Func(Widget w, XEvent *e, String *s, Cardinal *n)
{
	View_Name_Callback(w, NULL, NULL);
}

static void
View_Name_Cancel(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(save_name_shell);
}


static void
View_Save_Create_Shell()
{
	Arg	args[5];
	int	n;

	save_name_shell = XtCreatePopupShell("viewSaveShell",
						transientShellWidgetClass, main_window.shell, NULL, 0);

	/* Create the dialog widget to go inside the shell. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Save Name:");	n++;
	XtSetArg(args[n], XtNvalue, "");			n++;
	save_dialog = XtCreateManagedWidget("viewSaveDialog", dialogWidgetClass,
						save_name_shell, args, n);

	/* Add the button at the bottom of the dialog. */
	XawDialogAddButton(save_dialog, "Save", View_Name_Callback, NULL);
	XawDialogAddButton(save_dialog, "Cancel", View_Name_Cancel, NULL);

	XtOverrideTranslations(XtNameToWidget(save_dialog, "value"),
		XtParseTranslationTable(":<Key>Return: View_Name_Action()"));

	XtVaSetValues(XtNameToWidget(save_dialog, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(save_dialog, "Save"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(save_dialog, "Cancel"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(save_name_shell);
}


void
View_Recall_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	if ( cl )
		current_window = (WindowInfoPtr)cl;
	else
		current_window = Edit_Get_Info()->window;

	if ( ! recall_shell )
		View_Recall_Create_Shell();

	SFpositionWidget(recall_shell);
	XtPopup(recall_shell, XtGrabExclusive);
}

void
View_Delete_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	deleting = TRUE;
	View_Recall_Callback(w, cl, ca);
}


static void
View_Delete(int index)
{
	int	i;

	deleting = FALSE;

	if ( index < 0 )
	{
		XBell(XtDisplay(main_window.shell), 0);
		return;
	}

	for ( i = index + 1 ; i < num_views ; i++ )
	{
		view_list[ i - 1 ] = view_list[ i ];
		label_list[ i ] = label_list[ i + 1 ];
	}

	num_views--;
	XawListChange(recall_list, label_list, num_views + 1, 0, TRUE);
}


void
View_Reset()
{
	int	i;

	for ( i = num_views - 1 ; i >= 0 ; i-- )
		if ( ! view_list[i].is_default )
			View_Delete(i);

}


static void
View_Recall(Widget w, XtPointer cl, XtPointer ca)
{
	XawListReturnStruct	*data = (XawListReturnStruct*)ca;

	XtPopdown(recall_shell);

	if ( deleting )
	{
		View_Delete(data->list_index - 1);
		return;
	}

	if ( data->list_index == 0 )
		Camera_To_Window(current_window);
	else
	{
		current_window->viewport = view_list[data->list_index - 1];

		if ( view_list[data->list_index - 1].scr_width )
 		{
			XtVaSetValues(current_window->view_widget,
					XtNwidth, view_list[data->list_index - 1].scr_width,
 					XtNdesiredWidth, view_list[data->list_index - 1].scr_width,
					XtNheight, view_list[data->list_index - 1].scr_height,
 					XtNdesiredHeight,view_list[data->list_index - 1].scr_height,
					XtNmagnification, view_list[data->list_index - 1].magnify,
					NULL);
 			current_window->width = view_list[data->list_index - 1].scr_width;
 			current_window->height = view_list[data->list_index - 1].scr_height;
 		}
		else
			XtVaSetValues(current_window->view_widget,
					  XtNmagnification, view_list[data->list_index - 1].magnify,
					  NULL);

		View_Update(current_window, current_window->all_instances, CalcView);
		Update_Projection_Extents(current_window->all_instances);
	}
}


static void
Cancel_Recall_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(recall_shell);
}


/*	void
**	View_Recall_Create_Shell()
**	Creates the shell and the list in it. It also installs the Camera
**	viewport as the first entry.
*/
static void
View_Recall_Create_Shell()
{
	Widget	box;
	Widget	cancel;
	Arg		args[8];
	int		n;

	label_list = New(String, 1);
	label_list[0] = Strdup("Camera");

	n = 0;
	XtSetArg(args[n], XtNtitle, "View Recall");		n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	recall_shell = XtCreatePopupShell("recallShell",
					transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	box = XtCreateManagedWidget("viewRecallBox", boxWidgetClass, recall_shell,
								NULL, 0);

	n = 0;
	XtSetArg(args[n], XtNdefaultColumns, 1);	n++;
	XtSetArg(args[n], XtNforceColumns, TRUE);	n++;
	XtSetArg(args[n], XtNverticalList, TRUE);	n++;
	recall_list = XtCreateManagedWidget("recallList", listWidgetClass,
					box, args, n);
	XtAddCallback(recall_list, XtNcallback, View_Recall, NULL);

	XawListChange(recall_list, label_list, 1, 0, TRUE);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Cancel");		n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	cancel = XtCreateManagedWidget("viewrecallCancel", commandWidgetClass,
									box, args, n);
	XtAddCallback(cancel, XtNcallback, Cancel_Recall_Callback, NULL);

	XtRealizeWidget(recall_shell);
}


