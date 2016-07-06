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
**	viewport.c: Functions dealing with viewport specifications.
**
**	Created: 06/03/94
**
**	External functions:
**
**	int
**	Build_Viewport_Transformation(Viewport *result);
**	Creates a complete viewport specification structure from the given info.
**
*/

#include <math.h>
#include <sced.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Toggle.h>

/* we need this for the definition of intptr_t */
#include <stdint.h>

static WindowInfoPtr	draw_window;
static Widget			draw_mode_shell = NULL;
static Widget			draw_toggles;

static void	Create_Draw_Mode_Shell();


/*	void
**	Viewport_Init(ViewportPtr view)
**	Initialises a viewport structure.
*/
void
Viewport_Init(ViewportPtr view)
{
	VNew(INIT_LOOK_FROM_X, INIT_LOOK_FROM_Y, INIT_LOOK_FROM_Z, view->view_from);
	VNew(INIT_LOOK_AT_X, INIT_LOOK_AT_Y, INIT_LOOK_AT_Z, view->view_at);
	VNew(INIT_LOOK_UP_X, INIT_LOOK_UP_Y, INIT_LOOK_UP_Z, view->view_up);
	view->view_distance = INIT_DISTANCE;
 	view->eye_distance = INIT_EYE_DIST;
	view->scr_width = view->scr_height = 0;
	view->magnify = INIT_SCALE;
	view->draw_mode = DRAW_DASHED;

	Build_Viewport_Transformation(view);
}


/*	int
**	Build_Viewport_Transformation(Viewport *result);
**	Creates a complete viewport specification structure using the given data:
**	Assumes the view_from, view_at, view_up, view_distance and eyedist fields
**	have been filled by the caller.
*/
Boolean
Build_Viewport_Transformation(Viewport *result)
{
	Vector	u, v, n;
	double	temp_d, temp_d2;
	Vector	temp_v;

	/* n = (norm)( - view_from) */
	VUnit(result->view_from, temp_d, result->view_from);
	VScalarMul(result->view_from, -1.0, n);

	/* eye_position is:
	**	view_at + (view_distance + eye_distance) * result->view_from */
	VScalarMul(result->view_from, result->view_distance + result->eye_distance,
				temp_v);
	VAdd(temp_v, result->view_at, result->eye_position);

	/* v = (norm)(view_up - (view_up.n)n) */
	temp_d = VDot(result->view_up, n);
	VScalarMul(n, temp_d, temp_v);
	VSub(result->view_up, temp_v, v);

	VUnit(result->view_up, temp_d, result->view_up);

	if (((temp_d2 = VMod(v)) < EPSILON) && (temp_d2 > -EPSILON))
		return FALSE;

	VUnit(v, temp_d, v);

	/* u = n x v */
	VCross(n, v, u);

	if (((temp_d2 = VMod(u)) < EPSILON) && (temp_d2 > -EPSILON))
		return FALSE;

	/* The matrix required to transform from world to view is:	*/
	/*		[	u	]											*/
	/*		[	v	]											*/
	/*		[	n	]											*/
	MNew(u, v, n, result->world_to_view.matrix);

	/* The displacement vector is that required to take the
	** transformed look at point to 0, 0, -dist
	*/
	MVMul(result->world_to_view.matrix, result->view_at,
		  result->world_to_view.displacement);
	VScalarMul(result->world_to_view.displacement, -1,
			   result->world_to_view.displacement);
	result->world_to_view.displacement.z += result->view_distance;

	/* The matrix required to transform from view to world is :		*/
	/*	[ u v n]													*/
	/* Note this is the transpose of that used to go the other way.	*/
	/* The creation of this matrix is a little messy, since we		*/
	/*  already have the column ( as opposed to row) vectors.		*/
	result->view_to_world.matrix.x.x = u.x;
	result->view_to_world.matrix.x.y = v.x;
	result->view_to_world.matrix.x.z = n.x;
	result->view_to_world.matrix.y.x = u.y;
	result->view_to_world.matrix.y.y = v.y;
	result->view_to_world.matrix.y.z = n.y;
	result->view_to_world.matrix.z.x = u.z;
	result->view_to_world.matrix.z.y = v.z;
	result->view_to_world.matrix.z.z = n.z;

	VScalarMul(n, result->view_distance, temp_v);
	VSub(result->view_at, temp_v, result->view_to_world.displacement);

	return True;

}




static void
Viewdist_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	Initiate_Distance_Change((WindowInfoPtr)cl_data, TRUE);
}

static void
Eyedist_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	Initiate_Distance_Change((WindowInfoPtr)cl_data, FALSE);
}


/*	void
**	Create_View_Menu(Widget parent, Boolean all_functions, WindowInfoPtr window)
**	Creates a viewport changing menu.  Parent is the parent shell, all_functions
**	is says whether or not to include the full complement of functions and
**	window says what window it's for.
*/
void
Create_View_Menu(Widget parent, Boolean all_functions, WindowInfoPtr window)
{
	Widget	menu_widget;
	Widget	menu_children[10];

	int		count = 0;


	menu_widget = XtCreatePopupShell("ViewMenu", simpleMenuWidgetClass,
										parent, NULL, 0);

	menu_children[count] = XtCreateManagedWidget("Viewpoint", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback,
					Initiate_Viewfrom_Change, window);
	count++;

	menu_children[count] = XtCreateManagedWidget("Pan", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback,
					Initiate_Pan_Change, window);
	count++;

	if ( all_functions )
	{
		menu_children[count] = XtCreateManagedWidget("Lookat",
									smeBSBObjectClass, menu_widget, NULL, 0);
		XtAddCallback(menu_children[count], XtNcallback,
						Change_Lookat_Callback, window);
		count++;

		menu_children[count] = XtCreateManagedWidget("Lookup",
									smeBSBObjectClass, menu_widget, NULL, 0);
		XtAddCallback(menu_children[count], XtNcallback,
						Change_Lookup_Callback, window);
		count++;

		menu_children[count] = XtCreateManagedWidget("Distance",
									smeBSBObjectClass, menu_widget, NULL, 0);
		XtAddCallback(menu_children[count], XtNcallback,
						Viewdist_Callback, window);
		count++;

		menu_children[count] = XtCreateManagedWidget("Eye", smeBSBObjectClass,
														menu_widget, NULL, 0);
		XtAddCallback(menu_children[count], XtNcallback,
						Eyedist_Callback, window);
		count++;
	}
	else
	{
		menu_children[count] = XtCreateManagedWidget("Lookat",
									smeBSBObjectClass, menu_widget, NULL, 0);
		XtAddCallback(menu_children[count], XtNcallback,
												Change_Lookat_Callback, NULL);
		count++;

		menu_children[count] = XtCreateManagedWidget("Zoom", smeBSBObjectClass,
								menu_widget, NULL, 0);
		XtAddCallback(menu_children[count], XtNcallback,
												Zoom_Dialog_Func, NULL);
		count++;

		menu_children[count] = XtCreateManagedWidget("Save", smeBSBObjectClass,
														menu_widget, NULL, 0);
		XtAddCallback(menu_children[count], XtNcallback,
						View_Save_Current_Callback, (XtPointer)window);
		count++;

		menu_children[count] = XtCreateManagedWidget("Recall",
								smeBSBObjectClass, menu_widget, NULL, 0);
		XtAddCallback(menu_children[count], XtNcallback,
						View_Recall_Callback, (XtPointer)window);
		count++;

		menu_children[count] = XtCreateManagedWidget("Delete",
								smeBSBObjectClass, menu_widget, NULL, 0);
		XtAddCallback(menu_children[count], XtNcallback,
						View_Delete_Callback, (XtPointer)window);
		count++;
	}

}


/*	void
**	Create_Window_Menu(Widget parent, WindowInfoPtr window)
**	Creates a viewport changing menu.  Parent is the parent shell, all_functions
**	is says whether or not to include the full complement of functions and
**	window says what window it's for.
*/
void
Create_Window_Menu(Widget parent, WindowInfoPtr window)
{
	Widget	menu_widget;
	Widget	menu_children[6];

	int		count = 0;

	menu_widget = XtCreatePopupShell("WindowMenu", simpleMenuWidgetClass,
										parent, NULL, 0);

	menu_children[count] = XtCreateManagedWidget("Zoom", smeBSBObjectClass,
							menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Zoom_Dialog_Func,
					(XtPointer)window);
	count++;

	if ( window == &main_window )
		menu_children[count] = XtCreateManagedWidget("Image Size",
						smeBSBObjectClass, menu_widget, NULL, 0);
	else
		menu_children[count] = XtCreateManagedWidget("View Size",
						smeBSBObjectClass, menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Image_Size_Callback,
					(XtPointer)window);
	count++;

	menu_children[count] = XtCreateManagedWidget("Draw Mode",
						smeBSBObjectClass, menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Window_Draw_Mode_Callback,
						(XtPointer)window);
	count++;

	menu_children[count] = XtCreateManagedWidget("Save", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback,
					View_Save_Current_Callback, (XtPointer)window);
	count++;

	menu_children[count] = XtCreateManagedWidget("Recall", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback,
					View_Recall_Callback, (XtPointer)window);
	count++;

	menu_children[count] = XtCreateManagedWidget("Delete", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback,
					View_Delete_Callback, (XtPointer)window);
	count++;

}


/*
**	Sets the drawing mode for a window, which defines how back edges are
**	drawn.
*/
void
Window_Set_Draw_Mode(WindowInfoPtr window, int mode)
{
	window->viewport.draw_mode = mode;

	if ( window->view_widget )
		View_Update(window, NULL, ViewNone);
}


/*
**	Callback to initiate a change in drawing mode.
*/
void
Window_Draw_Mode_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	draw_window = (WindowInfoPtr)cl_data;

	if ( ! draw_mode_shell )
		Create_Draw_Mode_Shell();

	XawToggleSetCurrent(draw_toggles,
						(XtPointer)(intptr_t)draw_window->viewport.draw_mode);

	SFpositionWidget(draw_mode_shell);
	XtPopup(draw_mode_shell, XtGrabExclusive);
}


static void
Draw_Mode_Toggle_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	if ( ca_data )
	{
		Window_Set_Draw_Mode(draw_window,
							 (intptr_t)XawToggleGetCurrent(draw_toggles));
		XtPopdown(draw_mode_shell);
	}
}


static void
Create_Draw_Mode_Shell()
{
	Widget	form;
	Widget	toggles[3];
	Arg		args[15];
	int		n, m;
	XtTranslations translations =
		XtParseTranslationTable("<EnterWindow>: highlight(Always)\n"
								"<LeaveWindow>:unhighlight()\n"
								"<Btn1Down>,<Btn1Up>: set() notify()");

	draw_mode_shell = XtCreatePopupShell("Draw Mode",
						transientShellWidgetClass, main_window.shell, NULL, 0);

	/* Create the form to go inside the shell. */
	n = 0;
	form = XtCreateManagedWidget("drawModeForm", formWidgetClass,
					draw_mode_shell, args, n);

	/* Add the three toggles. */

	n = 0;
	XtSetArg(args[n], XtNleft, XtChainLeft);				n++;
	XtSetArg(args[n], XtNright, XtChainLeft);				n++;
	XtSetArg(args[n], XtNtop, XtChainTop);					n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);				n++;
	m = n;

	/* All toggle. */
	XtSetArg(args[n], XtNlabel, "Show All");				n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)DRAW_ALL);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	draw_toggles =
	toggles[0] = XtCreateManagedWidget("drawAllToggle",
					toggleWidgetClass, form, args, n);
	n = 0;
	XtSetArg(args[n], XtNradioGroup, draw_toggles);	n++;
	XtVaSetValues(toggles[0], XtNradioGroup, draw_toggles, NULL);
	XtAddCallback(toggles[0], XtNcallback, Draw_Mode_Toggle_Callback, NULL);
	XtOverrideTranslations(toggles[0], translations);

	/* Dashed toggle. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Dashed");					n++;
	XtSetArg(args[n], XtNradioGroup, draw_toggles);			n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)DRAW_DASHED);n++;
	XtSetArg(args[n], XtNfromVert, toggles[0]);				n++;
	XtSetArg(args[n], XtNstate, TRUE);						n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	toggles[1] = XtCreateManagedWidget("drawDashedToggle", toggleWidgetClass,
					form, args, n);
	XtAddCallback(toggles[1], XtNcallback, Draw_Mode_Toggle_Callback, NULL);
	XtOverrideTranslations(toggles[1], translations);

	/* Hidden toggle. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Hidden");					n++;
	XtSetArg(args[n], XtNradioGroup, draw_toggles);			n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)DRAW_CULLED);n++;
	XtSetArg(args[n], XtNfromVert, toggles[1]);				n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	toggles[2] = XtCreateManagedWidget("drawCulledToggle", toggleWidgetClass,
					form, args, n);
	XtAddCallback(toggles[2], XtNcallback, Draw_Mode_Toggle_Callback, NULL);
	XtOverrideTranslations(toggles[2], translations);

	Match_Widths(toggles, 3);

	XtRealizeWidget(draw_mode_shell);
}
