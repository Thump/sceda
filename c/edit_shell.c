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
**	edit_shell.c : Functions controlling the edit shell, the one with all
**					the constraints and stuff in it.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <select_point.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>

extern void	Apply_Viewfrom_Text(WindowInfoPtr);
extern void	Cancel_Change_Look_Event();

extern void	Edit_Clear_Cursor();

static void	Edit_Shell_Set_Constraints();
static void	Edit_Create_Edit_Shell();

extern Pixmap	menu_bitmap;

#define NO_LABEL "No Drag in Progress"

FeatureData	origin_defaults[] = {
	{ plane_feature, "X-Y Plane", FALSE, { 0, 0, 1 }, { 0, 0, 0 }, 0 },
	{ plane_feature, "Y-Z Plane", FALSE, { 1, 0, 0 }, { 0, 0, 0 }, 0 },
	{ plane_feature, "Z-X Plane", FALSE, { 0, 1, 0 }, { 0, 0, 0 }, 0 }};
FeatureData	scale_defaults[] = {
	{ plane_feature, "Major-Minor Pl", FALSE },
	{ plane_feature, "Major-Other Pl", FALSE },
	{ plane_feature, "Minor-Other Pl", FALSE }};
FeatureData	rotate_defaults[] = {
	{ line_feature, "Major Axis", FALSE },
	{ line_feature, "Minor Axis", FALSE },
	{ line_feature, "Other Axis", FALSE }};

static Widget	edit_shell = NULL;
Widget			edit_form;
/*static Widget	drag_label;*/
static Widget	ref_button;
static Widget	major_a_button;
static Widget	minor_a_button;

/* changed this from being static so I can tell whether buttons are
** sensitive or not.  Also changed it to edit_buttons[] from just
** buttons[], for clarity.  No offense Steve!  :)
*/
Widget	edit_buttons[12];

static EditInfoPtr	info;

ConstraintBox	origin_constraints;
ConstraintBox	scale_constraints;
ConstraintBox	rotate_constraints;

static Cursor	delete_cursor;


void
Edit_Initialize_Shell(EditInfoPtr new_info)
{
	/*Position	x, y;
	Dimension	height;*/
	Vector		dummy_vect;

	info = new_info;

	if ( ! edit_shell )
		Edit_Create_Edit_Shell();

	Edit_Shell_Set_Constraints();
	Edit_Scale_Force_Constraints(info, TRUE);
	Edit_Force_Rotation_Constraints(info, TRUE);

	Edit_Set_Drag_Label(NO_DRAG, dummy_vect, 0.0);
	Edit_Set_Major_Align_Label(info->obj->o_major_align.f_type==line_feature);
	Edit_Set_Minor_Align_Label(info->obj->o_minor_align.f_type==line_feature);

	Edit_Match_Widths();

	/*
	XtVaGetValues(info->window->shell, XtNx, &x, XtNy, &y,
				  XtNheight, &height, NULL);
	XtVaSetValues(edit_shell, XtNx, x, XtNy, (int)y + (int)height, NULL);

	XtPopup(edit_shell, XtGrabNone);
	*/

	Edit_Sensitize_Buttons(TRUE, TRUE);
	XMapRaised(XtDisplay(edit_shell), XtWindow(edit_shell));
}



static void
Edit_Shell_Set_Constraints()
{
	int	i;

	/* Set the defaults to correspond to the current origin and reference.*/
	Edit_Set_Origin_Defaults(origin_constraints.options, info->origin);
	Edit_Set_Scale_Defaults(scale_constraints.options, info->reference,
							info->axes);
	Edit_Set_Rotate_Defaults(rotate_constraints.options, info->axes);

	/* Add any constraints specific to this object, and set all the states. */
	origin_constraints.num_options = 3;
	for ( i = 0 ; i < info->obj->o_origin_num ; i++ )
		Edit_Add_Constraint(&origin_constraints, i + 3);

	scale_constraints.num_options = 3;
	for ( i = 0 ; i < info->obj->o_scale_num ; i++ )
		Edit_Add_Constraint(&scale_constraints, i + 3);

	rotate_constraints.num_options = 3;
	for ( i = 0 ; i < info->obj->o_rotate_num ; i++ )
		Edit_Add_Constraint(&rotate_constraints, i + 3);

	Edit_Set_Constraint_States(&origin_constraints, info->obj->o_origin_active);
	Edit_Set_Constraint_States(&scale_constraints, info->obj->o_scale_active);
	Edit_Set_Constraint_States(&rotate_constraints, info->obj->o_rotate_active);

	Constraint_Solve_System(origin_constraints.options,
							origin_constraints.num_options,
							&(info->origin_resulting));
	Constraint_Solve_System(scale_constraints.options,
							scale_constraints.num_options,
							&(info->reference_resulting));
	Constraint_Solve_System(rotate_constraints.options,
							rotate_constraints.num_options,
							&(info->rotate_resulting));

	if ( info->obj->o_major_align.f_type == line_feature )
		info->obj->o_major_align.f_status = TRUE;
	if ( info->obj->o_minor_align.f_type == line_feature )
		info->obj->o_minor_align.f_status = TRUE;
}


static void
Edit_Shell_Suspend(Widget w, XtPointer cl, XtPointer ca)
{	double tmp;

	/*
	XtPopdown(edit_shell);
	*/
	Edit_Sensitize_Buttons(FALSE, TRUE);

	if ( info->deleting )
		Edit_Cancel_Remove();

	info->obj->o_flags |= ObjVisible;

	/* Calculate the inverse transform. */
	info->obj->o_inverse.matrix = MInvert(&(info->obj->o_transform.matrix));
	VScalarMul(info->obj->o_transform.displacement, -1,
			   info->obj->o_inverse.displacement);

	/* at this point we store the editted rotation and scale in the obj */
	info->obj->o_rot=info->rotation;
	QUnit(info->obj->o_rot,tmp,info->obj->o_rot);
	info->obj->o_scale=info->scale;

	/* Do the constraint maintenance bit. Update all objects depending on
	** this one.
	*/
	if ( ! do_maintenance )
		Edit_Maintain_All_Constraints(info);
	Edit_Maintain_Free_List();

	Edit_Undo_Clear();

	Constraint_Box_Clear(&(origin_constraints));
	Constraint_Box_Clear(&(scale_constraints));
	Constraint_Box_Clear(&(rotate_constraints));

	Edit_Clear_Info();

	Cancel_Object_Edit();

	Edit_Clear_Cursor();

	View_Update(info->window, info->window->all_instances, ViewNone );

	Update_Projection_Extents(info->window->all_instances);

	Set_Prompt(info->window, "");

	changed_scene = TRUE;
}


void
Edit_Shell_Finished(Widget w, XtPointer cl, XtPointer ca)
{
	Delete_Edit_Instance(info->window, info->inst);

	Edit_Shell_Suspend(w, cl, ca);
}


static void
Edit_Shell_Undo(Widget w, XtPointer cl, XtPointer ca)
{
	if ( info->window->current_state & change_viewfrom )
	{
		Apply_Viewfrom_Text(info->window);
		return;
	}

	if ( info->selecting )
	{
		Draw_Selection_Points(XtWindow(info->window->view_widget));
		Edit_Cleanup_Selection(TRUE);
		return;
	}

	if ( info->window->current_state & change_look )
	{
		Draw_Selection_Points(XtWindow(info->window->view_widget));
		Cleanup_Selection();
		Cancel_Change_Look_Event();
		return;
	}

	if ( info->deleting )
	{
		Edit_Cancel_Remove();
		return;
	}

	Edit_Undo();
}


static void
Edit_Constraint_Remove(Widget w, XtPointer cl, XtPointer ca)
{
	EditInfoPtr	info = Edit_Get_Info();

	info->deleting = TRUE;

	/* Change the cursor. */
	delete_cursor = XCreateFontCursor(XtDisplay(info->window->shell),
										XC_pirate);
    XDefineCursor(XtDisplay(info->window->shell),
					XtWindow(edit_shell), delete_cursor);

}

void
Edit_Cancel_Remove()
{
	EditInfoPtr	info = Edit_Get_Info();

	if ( ! info->deleting )
		return;

	info->deleting = FALSE;

	/* Change the cursor back. */
	XDefineCursor(XtDisplay(info->window->shell),
					XtWindow(edit_shell), None);
	XFreeCursor(XtDisplay(info->window->shell), delete_cursor);
}


static void
Edit_Temp_Align_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	if ( info->deleting )
		Edit_Cancel_Remove();

	if ( info->rotate_resulting.f_type != line_feature )
		return;

	if ( (long)cl == MAJOR_AXIS )
		Edit_Major_Align(info->rotate_resulting.f_vector, FALSE);
	else
		Edit_Minor_Align(info->rotate_resulting.f_vector, FALSE);
}


static void
Edit_Create_Axis_Menu(char *name, XtPointer axis)
{
	Widget	menu;
	Widget	children[7];

	menu = XtCreatePopupShell(name, simpleMenuWidgetClass, edit_shell, NULL, 0);

	children[0] = XtCreateManagedWidget("Redefine 1", smeBSBObjectClass, menu,
										NULL, 0);
	XtAddCallback(children[0], XtNcallback, Edit_Change_Axis_1_Callback, axis);

	children[1] = XtCreateManagedWidget("Redefine 2", smeBSBObjectClass, menu,
										NULL, 0);
	XtAddCallback(children[1], XtNcallback, Edit_Change_Axis_2_Callback, axis);

	children[2] = XtCreateManagedWidget("Temp Align", smeBSBObjectClass, menu,
										NULL, 0);
	XtAddCallback(children[2], XtNcallback, Edit_Temp_Align_Callback, axis);

	children[3] = XtCreateManagedWidget("Align Pt", smeBSBObjectClass, menu,
										NULL, 0);
	XtAddCallback(children[3], XtNcallback, Edit_Align_With_Point, axis);

	children[4] = XtCreateManagedWidget("Align Line", smeBSBObjectClass, menu,
										NULL, 0);
	XtAddCallback(children[4], XtNcallback, Edit_Align_With_Line, axis);

	children[5] = XtCreateManagedWidget("Align Plane", smeBSBObjectClass, menu,
										NULL, 0);
	XtAddCallback(children[5], XtNcallback, Edit_Align_With_Plane, axis);

	children[6] = XtCreateManagedWidget("Remove", smeBSBObjectClass, menu,
										NULL, 0);
	XtAddCallback(children[6], XtNcallback, Edit_Remove_Alignment, axis);
}


static void
Edit_Create_Edit_Shell()
{
	Arg		args[15];
	int		n, m, count;
	Widget	remove_button;
	Widget	origin_button;
	Widget	major_button;
	Widget	minor_button;
	Widget	view_button;
	Widget	finished_button;
	Widget	suspend_button;
	Widget	undo_button;
	Widget	redo_button;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Edit");			n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	edit_shell = XtCreatePopupShell("editShell", topLevelShellWidgetClass,
									main_window.shell, args, n);

	n = 0;
	edit_form = XtCreateManagedWidget("editForm", formWidgetClass, edit_shell,
									  args, n);

	m = 0;
	XtSetArg(args[m], XtNtop, XtChainTop);		m++;
	XtSetArg(args[m], XtNbottom, XtChainTop);	m++;
	XtSetArg(args[m], XtNleft, XtChainLeft);	m++;
	XtSetArg(args[m], XtNright, XtChainLeft);	m++;

	/*
	n = m;
	XtSetArg(args[n], XtNborderWidth, 0);	n++;
	XtSetArg(args[n], XtNlabel, NO_LABEL);	n++;
	XtSetArg(args[n], XtNwidth, 300);		n++;
	drag_label = XtCreateManagedWidget("editDragLabel", labelWidgetClass,
									   edit_form, args, n);
	*/

	count = 0;

	XtSetArg(args[m], XtNresizable, TRUE);		m++;

	/* Finish. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Finish");			n++;
	/*XtSetArg(args[n], XtNfromVert, drag_label);		n++;*/
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	edit_buttons[count++] =
	finished_button = XtCreateManagedWidget("editShellFinished",
						commandWidgetClass, edit_form, args, n);
	XtAddCallback(finished_button, XtNcallback, Edit_Shell_Finished, NULL);

	/* Suspend. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Suspend");				n++;
	/*XtSetArg(args[n], XtNfromVert, drag_label);			n++;*/
	XtSetArg(args[n], XtNfromHoriz, finished_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	edit_buttons[count++] =
	suspend_button = XtCreateManagedWidget("editShellSuspend",
						commandWidgetClass, edit_form, args, n);
	XtAddCallback(suspend_button, XtNcallback, Edit_Shell_Suspend, NULL);

	/* Undo. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Undo");				n++;
	/*XtSetArg(args[n], XtNfromVert, drag_label);			n++;*/
	XtSetArg(args[n], XtNfromHoriz, suspend_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	edit_buttons[count++] =
	undo_button = XtCreateManagedWidget("editShellUndo",
						commandWidgetClass, edit_form, args, n);
	XtAddCallback(undo_button, XtNcallback, Edit_Shell_Undo, NULL);

	/* Redo. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Redo");			n++;
	/*XtSetArg(args[n], XtNfromVert, drag_label);		n++;*/
	XtSetArg(args[n], XtNfromHoriz, undo_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	edit_buttons[count++] =
	redo_button = XtCreateManagedWidget("editShellRedo",
						commandWidgetClass, edit_form, args, n);
	XtAddCallback(redo_button, XtNcallback, Edit_Redo, NULL);

	/* Origin. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Origin");				n++;
	XtSetArg(args[n], XtNfromVert, finished_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	edit_buttons[count++] =
	origin_button = XtCreateManagedWidget("editShellOrigin",
						commandWidgetClass, edit_form, args, n);
	XtAddCallback(origin_button, XtNcallback, Edit_Origin_Callback, NULL);

	/* Reference. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Scaling");			n++;
	XtSetArg(args[n], XtNfromVert, suspend_button);	n++;
	XtSetArg(args[n], XtNfromHoriz, origin_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	edit_buttons[count++] =
	ref_button = XtCreateManagedWidget("editShellReference",
						commandWidgetClass, edit_form, args, n);
	XtAddCallback(ref_button, XtNcallback, Edit_Reference_Callback, NULL);

	/* Major. */
	Edit_Create_Axis_Menu("MajorMenu", (XtPointer)MAJOR_AXIS);
	n = m;
	XtSetArg(args[n], XtNlabel, "Major");			n++;
	XtSetArg(args[n], XtNfromVert, undo_button);	n++;
	XtSetArg(args[n], XtNfromHoriz, ref_button);	n++;
	XtSetArg(args[n], XtNmenuName, "MajorMenu");	n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);	n++;
#endif
	edit_buttons[count++] =
	major_button = XtCreateManagedWidget("editShellMajor",
						menuButtonWidgetClass, edit_form, args, n);

	/* Minor. */
	Edit_Create_Axis_Menu("MinorMenu", (XtPointer)MINOR_AXIS);
	n = m;
	XtSetArg(args[n], XtNlabel, "Minor");			n++;
	XtSetArg(args[n], XtNfromVert, redo_button);	n++;
	XtSetArg(args[n], XtNfromHoriz, major_button);	n++;
	XtSetArg(args[n], XtNmenuName, "MinorMenu");	n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);	n++;
#endif
	edit_buttons[count++] =
	minor_button = XtCreateManagedWidget("editShellMinor",
						menuButtonWidgetClass, edit_form, args, n);

	/* Remove. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Remove");			n++;
	XtSetArg(args[n], XtNfromVert, origin_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	edit_buttons[count++] =
	remove_button = XtCreateManagedWidget("editShellRemove",
						commandWidgetClass, edit_form, args, n);
	XtAddCallback(remove_button, XtNcallback, Edit_Constraint_Remove, NULL);

	/* View. */
	Create_View_Menu(edit_shell, FALSE, NULL);
	n = m;
	XtSetArg(args[n], XtNlabel, "View");			n++;
	XtSetArg(args[n], XtNmenuName, "ViewMenu");		n++;
	XtSetArg(args[n], XtNfromHoriz, remove_button);	n++;
	XtSetArg(args[n], XtNfromVert, ref_button);		n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);	n++;
#endif
	edit_buttons[count++] =
	view_button = XtCreateManagedWidget("editShellView",
						menuButtonWidgetClass, edit_form, args, n);

	/* Major Align. */
	n = m;
	XtSetArg(args[n], XtNlabel, NO_ALLIGN);			n++;
	XtSetArg(args[n], XtNfromVert, major_button);	n++;
	XtSetArg(args[n], XtNfromHoriz, ref_button);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);			n++;
	edit_buttons[count++] =
	major_a_button = XtCreateManagedWidget("editShellMajorA",
						labelWidgetClass, edit_form, args, n);

	/* Minor Align. */
	n = m;
	XtSetArg(args[n], XtNlabel, NO_ALLIGN);			n++;
	XtSetArg(args[n], XtNfromVert, minor_button);	n++;
	XtSetArg(args[n], XtNfromHoriz, major_button);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);			n++;
	edit_buttons[count++] =
	minor_a_button = XtCreateManagedWidget("editShellMinorA",
						labelWidgetClass, edit_form, args, n);

	Edit_Create_Constraint_Box(&origin_constraints, origin_defaults, edit_form,
							   "Position", NULL, remove_button, ORIGIN);
	Edit_Create_Constraint_Box(&scale_constraints, scale_defaults, edit_form,
							   "Scale", origin_constraints.box_widget,
							   remove_button, SCALE);
	Edit_Create_Constraint_Box(&rotate_constraints, rotate_defaults, edit_form,
							   "Rotate", scale_constraints.box_widget,
							   remove_button, ROTATE);

	XtRealizeWidget(edit_shell);
}


void
Edit_Match_Widths()
{
	Dimension	max_width_3, max_width_4, width;
	Arg			arg;
	int			i;

	XawFormDoLayout(edit_form, FALSE);

	Edit_Constraint_Box_Resize(&origin_constraints);
	Edit_Constraint_Box_Resize(&scale_constraints);
	Edit_Constraint_Box_Resize(&rotate_constraints);

	XtSetArg(arg, XtNwidth, &width);
	XtGetValues(origin_constraints.box_widget, &arg, 1);
	max_width_3 = width;
	XtGetValues(scale_constraints.box_widget, &arg, 1);
	if ( width > max_width_3 )
		max_width_3 = width;
	XtGetValues(rotate_constraints.box_widget, &arg, 1);
	if ( width > max_width_3 )
		max_width_3 = width;
	XtGetValues(major_a_button, &arg, 1);
	max_width_4 = width;

	if ( (double)max_width_4 > ( 3.0 / 4.0 * max_width_3 ) )
		max_width_3 = (Dimension)((double)(max_width_4 + 1) * 4.0 / 3.0);
	else
		max_width_4 = (Dimension)((double)max_width_3 * 3.0 / 4.0 - 1);

	XtSetArg(arg, XtNwidth, max_width_3);
	XtSetValues(origin_constraints.box_widget, &arg, 1);
	XtSetValues(scale_constraints.box_widget, &arg, 1);
	XtSetValues(rotate_constraints.box_widget, &arg, 1);

	XtSetArg(arg, XtNwidth, max_width_4);
	for ( i = 0 ; i < 12 ; i++ )
		XtSetValues(edit_buttons[i], &arg, 1);

	XawFormDoLayout(edit_form, TRUE);
}


void
Edit_Sensitize_Buttons(Boolean state, Boolean all)
{
	int	i;

	if ( XtIsSensitive(edit_buttons[0]) == state )	return;

	XtSetSensitive(origin_constraints.box_widget, state);
	XtSetSensitive(scale_constraints.box_widget, state);
	XtSetSensitive(rotate_constraints.box_widget, state);
	XtSetSensitive(origin_constraints.add_widget, state);
	XtSetSensitive(scale_constraints.add_widget, state);
	XtSetSensitive(rotate_constraints.add_widget, state);
	for ( i = 0 ; i < 2 ; i++ )
		XtSetSensitive(edit_buttons[i], state);
	for ( i = 3 ; i < 12 ; i++ )
		XtSetSensitive(edit_buttons[i], state);

	if ( all )
		XtSetSensitive(edit_buttons[2], state);
}

void
Edit_Set_Drag_Label(int op_type, Vector vect, double val)
{
	char	label_str[64];

	switch ( op_type )
	{
		case NO_DRAG:
			/*
			XtVaSetValues(drag_label, XtNlabel, NO_LABEL, NULL);
			*/
			Set_Prompt(info->window, NO_LABEL);
			break;

		case ORIGIN_DRAG:
			sprintf(label_str, "Trans %1.5g %1.5g %1.5g",
					vect.x, vect.y, vect.z);
			/*
			XtVaSetValues(drag_label, XtNlabel, label_str, NULL);
			*/
			Set_Prompt(info->window, label_str);
			break;

		case SCALE_DRAG:
			sprintf(label_str, "Scale %1.5g %1.5g %1.5g",
					vect.x, vect.y, vect.z);
			/*
			XtVaSetValues(drag_label, XtNlabel, label_str, NULL);
			*/
			Set_Prompt(info->window, label_str);
			break;

		case ROTATE_DRAG:
			sprintf(label_str, "Rot %1.5g @ %1.5g %1.5g %1.5g", val,
					vect.x, vect.y, vect.z);
			/*
			XtVaSetValues(drag_label, XtNlabel, label_str, NULL);
			*/
			Set_Prompt(info->window, label_str);
			break;
	}
}


void
Edit_Set_Major_Align_Label(Boolean state)
{
	if ( state )
		XtVaSetValues(major_a_button, XtNlabel, ALLIGNED, NULL);
	else
		XtVaSetValues(major_a_button, XtNlabel, NO_ALLIGN, NULL);
}

void
Edit_Set_Minor_Align_Label(Boolean state)
{
	if ( state )
		XtVaSetValues(minor_a_button, XtNlabel, ALLIGNED, NULL);
	else
		XtVaSetValues(minor_a_button, XtNlabel, NO_ALLIGN, NULL);
}
