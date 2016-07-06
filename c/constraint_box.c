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
**	constraint_box.c : Functions top manage a constraint selection box.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <select_point.h>
#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Toggle.h>


static void	Edit_Add_Constraint_Callback(Widget, XtPointer, XtPointer);
static void	Edit_Select_Constraint_Callback(Widget, XtPointer, XtPointer);

static void	Add_Plane_Constraint_Callback(int*, FeatureSpecType*);
static void Add_Line_Constraint_Callback(int*, FeatureSpecType*);
static void Add_Point_Constraint_Callback(int*, FeatureSpecType*);
static void	Add_Midplane_Constraint_Callback(int*, FeatureSpecType*);
static void	Add_Midpoint_Constraint_Callback(int*, FeatureSpecType*);
static void	Add_Axis_Plane_Constraint_Callback(int*, FeatureSpecType*);
static void Add_Axis_Line_Constraint_Callback(int*, FeatureSpecType*);
static void Add_Orig_Line_Constraint_Callback(int*, FeatureSpecType*);
static void Add_Ref_Line_Constraint_Callback(int*, FeatureSpecType*);
static void Add_Orig_Plane_Constraint_Callback(int*, FeatureSpecType*);
static void Add_Ref_Plane_Constraint_Callback(int*, FeatureSpecType*);

extern Boolean	scale_forced[3];
extern Boolean	rotate_forced[2];

extern Pixmap	menu_bitmap;

static int	feature_counts[NUM_CONSTRAINT_TYPES] = { 0 };

static Widget	add_name_shell = NULL;
static Widget	add_name_dialog;

static ConstraintBoxPtr	add_to_box;


static void
Create_Add_Menu(ConstraintBoxPtr box, Widget parent, int type)
{
	Widget	menu;
	Widget	children[MAX_ADD_ITEMS];
	Arg	arg;
	int	count;

	menu = XtCreatePopupShell("AddMenu", simpleMenuWidgetClass,
								parent, NULL, 0);

	XtSetArg(arg, XtNjustify, XtJustifyCenter);

	count = 0;
	children[count] = XtCreateManagedWidget( "Plane",
				smeBSBObjectClass, menu, &arg, 1);
	box->add_info[count].type =
		( type == ROTATE ) ? axis_plane_feature : plane_feature;
	box->add_info[count].box = box;
	XtAddCallback(children[count], XtNcallback,
			Edit_Add_Constraint_Callback, (XtPointer)(box->add_info + count));
	count++;

	children[count] = XtCreateManagedWidget( "Line",
				smeBSBObjectClass, menu, &arg, 1);
	box->add_info[count].type = type == ROTATE ? axis_feature : line_feature;
	box->add_info[count].box = box;
	XtAddCallback(children[count], XtNcallback,
			Edit_Add_Constraint_Callback, (XtPointer)(box->add_info + count));
	count++;

	children[count] = XtCreateManagedWidget( "Point",
				smeBSBObjectClass, menu, &arg, 1);
	box->add_info[count].type = point_feature;
	box->add_info[count].box = box;
	XtAddCallback(children[count], XtNcallback,
		Edit_Add_Constraint_Callback, (XtPointer)(box->add_info + count));
	count++;

	if ( type != ROTATE )
	{
		children[count] = XtCreateManagedWidget("Equi Plane",
					smeBSBObjectClass, menu, &arg, 1);
		box->add_info[count].type = midplane_feature;
		box->add_info[count].box = box;
		XtAddCallback(children[count], XtNcallback,
			Edit_Add_Constraint_Callback, (XtPointer)(box->add_info + count));
		count++;

		children[count] = XtCreateManagedWidget("Midpoint",
					smeBSBObjectClass, menu, &arg, 1);
		box->add_info[count].type = midpoint_feature;
		box->add_info[count].box = box;
		XtAddCallback(children[count], XtNcallback,
			Edit_Add_Constraint_Callback, (XtPointer)(box->add_info + count));
		count++;
	}

	if ( type == ORIGIN )
	{
		children[count] = XtCreateManagedWidget("Origin Line",
					smeBSBObjectClass, menu, &arg, 1);
		box->add_info[count].type = orig_line_feature;
		box->add_info[count].box = box;
		XtAddCallback(children[count], XtNcallback,
			Edit_Add_Constraint_Callback, (XtPointer)(box->add_info + count));
		count++;

		children[count] = XtCreateManagedWidget("Origin Plane",
					smeBSBObjectClass, menu, &arg, 1);
		box->add_info[count].type = orig_plane_feature;
		box->add_info[count].box = box;
		XtAddCallback(children[count], XtNcallback,
			Edit_Add_Constraint_Callback, (XtPointer)(box->add_info + count));
		count++;
	}

	if ( type == SCALE )
	{
		children[count] = XtCreateManagedWidget("Scale Line",
					smeBSBObjectClass, menu, &arg, 1);
		box->add_info[count].type = ref_line_feature;
		box->add_info[count].box = box;
		XtAddCallback(children[count], XtNcallback,
			Edit_Add_Constraint_Callback, (XtPointer)(box->add_info + count));
		count++;

		children[count] = XtCreateManagedWidget("Scale Plane",
					smeBSBObjectClass, menu, &arg, 1);
		box->add_info[count].type = ref_plane_feature;
		box->add_info[count].box = box;
		XtAddCallback(children[count], XtNcallback,
			Edit_Add_Constraint_Callback, (XtPointer)(box->add_info + count));
		count++;
	}
}



void
Edit_Create_Constraint_Box(ConstraintBoxPtr target, FeaturePtr defaults,
			Widget parent, char *label, Widget neighbour, Widget top,
			int type)
{
	Arg		args[10];
	int		i, n;

	/* Set up the defaults. */
	target->num_options = target->max_num_options = 3;
	target->options = New(FeaturePtr, target->max_num_options);
	target->option_widgets = New(Widget, target->max_num_options);

	target->options[0] = defaults;
	target->options[1] = defaults + 1;
	target->options[2] = defaults + 2;

	/* The box. */
	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;
	XtSetArg(args[n], XtNfromVert, top);		n++;
	XtSetArg(args[n], XtNfromHoriz, neighbour);	n++;
	XtSetArg(args[n], XtNorientation, XtorientVertical);	n++;
	target->box_widget = XtCreateManagedWidget(label,
							boxWidgetClass, parent, args, n);

	/* The label. */
	n = 0;
	XtSetArg(args[n], XtNlabel, label);		n++;
	XtSetArg(args[n], XtNborderWidth, 0);	n++;
	target->label_widget = XtCreateManagedWidget("editConstraintBoxLabel",
							labelWidgetClass, target->box_widget, args, n);

	/* The add button, below the label. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Add New");				n++;
	XtSetArg(args[n], XtNmenuName, "AddMenu");			n++;
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);		n++;
	target->add_widget = XtCreateManagedWidget("editAddButton",
							menuButtonWidgetClass, target->box_widget, args, n);
	Create_Add_Menu(target, target->add_widget, type);

	/* The available toggles. */
	for ( i = 0 ; i < target->num_options ; i++ )
	{
		n = 0;
		XtSetArg(args[n], XtNlabel, target->options[i]->f_label);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
		XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
		XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
		XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
		target->option_widgets[i]=XtCreateManagedWidget("editConstraintToggle",
								toggleWidgetClass, target->box_widget, args, n);
		XtAddCallback(target->option_widgets[i], XtNcallback,
					Edit_Select_Constraint_Callback, (XtPointer)target);
	}

}


void
Edit_Constraint_Box_Resize(ConstraintBoxPtr target)
{
	Match_Widths(target->option_widgets, target->num_options);
}


void
Edit_Add_Constraint(ConstraintBoxPtr target, int index)
{
	EditInfoPtr info = Edit_Get_Info();
	int	i;
	Arg	args[10];
	int	n;

	if ( target->num_options == target->max_num_options )
	{
		target->max_num_options += 3;
		target->options =
				More(target->options, FeaturePtr, target->max_num_options);
		target->option_widgets =
				More(target->option_widgets, Widget, target->max_num_options);
	}

	/* Shuffle up to make space. */
	for ( i = target->num_options ; i > index ; i-- )
		target->options[i] = target->options[i-1];
	if ( target == &origin_constraints )
		target->options[index] = info->obj->o_origin_cons + ( index - 3 );
	else if ( target == &scale_constraints )
		target->options[index] = info->obj->o_scale_cons + ( index - 3 );
	else
		target->options[index] = info->obj->o_rotate_cons + ( index - 3 );

	/* Create a new widget. */
	n = 0;
	XtSetArg(args[n], XtNlabel, target->options[target->num_options]->f_label);
	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	target->option_widgets[target->num_options] = XtCreateManagedWidget(
		"editConstraintToggle", toggleWidgetClass, target->box_widget, args, n);
	XtAddCallback(target->option_widgets[target->num_options], XtNcallback,
					Edit_Select_Constraint_Callback, (XtPointer)target);

	target->num_options++;

	/* Reset the names and states of all the widgets. */
	for ( i = index ; i < target->num_options ; i++ )
	{
		n = 0;
		XtSetArg(args[n], XtNlabel, target->options[i]->f_label);	n++;
		XtSetArg(args[n], XtNstate, target->options[i]->f_status);	n++;
		XtSetValues(target->option_widgets[i], args, n);
	}

	XawFormDoLayout(edit_form, TRUE);
}


static void
Edit_Add_Constraint_Callback(Widget w, XtPointer cl_data, XtPointer ca)
{
	EditInfoPtr	info = Edit_Get_Info();
	AddInfoPtr	data = (AddInfoPtr)cl_data;

	debug(FUNC_NAME,fprintf(stderr,"Edit_Add_Constraint_Callback()\n"));

	if ( info->deleting )
		Edit_Cancel_Remove();

	switch ( data->type )
	{
		case plane_feature:
			num_verts_required = 3;
			select_finished_callback = Add_Plane_Constraint_Callback;
			break;

		case line_feature:
			num_verts_required = 2;
			select_finished_callback = Add_Line_Constraint_Callback;
			break;

		case point_feature:
			num_verts_required = 1;
			select_finished_callback = Add_Point_Constraint_Callback;
			break;

		case midplane_feature:
			num_verts_required = 2;
			select_finished_callback = Add_Midplane_Constraint_Callback;
			break;

		case midpoint_feature:
			num_verts_required = 2;
			select_finished_callback = Add_Midpoint_Constraint_Callback;
			break;
		case axis_plane_feature:
			num_verts_required = 3;
			select_finished_callback = Add_Axis_Plane_Constraint_Callback;
			break;

		case axis_feature:
			num_verts_required = 2;
			select_finished_callback = Add_Axis_Line_Constraint_Callback;
			break;

		case orig_line_feature:
			num_verts_required = 1;
			select_finished_callback = Add_Orig_Line_Constraint_Callback;
			break;

		case ref_line_feature:
			num_verts_required = 1;
			select_finished_callback = Add_Ref_Line_Constraint_Callback;
			break;

		case orig_plane_feature:
			num_verts_required = 2;
			select_finished_callback = Add_Orig_Plane_Constraint_Callback;
			break;

		case ref_plane_feature:
			num_verts_required = 2;
			select_finished_callback = Add_Ref_Plane_Constraint_Callback;
			break;

		default:;
	}

	select_window = info->window;
	select_highlight = TRUE;
	allow_text_entry = TRUE;
	select_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];

	Build_Select_Verts_From_List(info->all_available);
	if ( data->box == &origin_constraints &&
		 data->type != orig_line_feature &&
		 data->type != orig_plane_feature )
	{
		specs_allowed[reference_spec] =
		specs_allowed[absolute_spec] = TRUE;
		specs_allowed[offset_spec] = FALSE;
	}
	else
	{
		specs_allowed[reference_spec] =
		specs_allowed[absolute_spec] =
		specs_allowed[offset_spec] = TRUE;
	}

	if ( num_select_verts < num_verts_required ) return;

	/* Prepare for point selection. */
	info->selecting = TRUE;

	Register_Select_Operation();

	Edit_Sensitize_Buttons(FALSE, FALSE);

	add_to_box = data->box;

	/* Draw all the reference points. */
	Edit_Draw_Selection_Points(info);
}


/* Spec is the spec we want removed. Obj is the object that OWNS the spec
** passed, NOT the referenced object.
*/
void
Edit_Remove_Obj_From_Dependencies(FeatureSpecPtr spec, ObjectInstancePtr obj,
								  void *ptr, int check)
{
	ObjectInstancePtr	src;
	int	i, index;

	if ( check && spec->spec_type != reference_spec ) return;

	src = spec->spec_object;

	for ( index = 0 ;
		  index < src->o_num_depend && src->o_dependents[index].obj != obj ;
		  index++ );

	if ( index == src->o_num_depend )
		return;

	if ( src->o_dependents[index].count == 1 )
	{
		for ( i = index + 1 ; i < src->o_num_depend ; i++ )
			src->o_dependents[ i - 1 ] = src->o_dependents[i];
		src->o_num_depend--;
	}
	else
		src->o_dependents[index].count--;

	if ( ! src->o_num_depend ) free(src->o_dependents);
}


Boolean
Edit_Remove_Constraint(EditInfoPtr info, ConstraintBoxPtr box, int index,
					   Boolean save_state)
{
	int		i;

	Edit_Cancel_Remove();

	/* Refuse to remove the defaults or active constraints. */
	if ( index < 3 || box->options[index]->f_status )
	{
		XBell(XtDisplay(info->window->shell), 0);
		return FALSE;
	}

	/* Save the Undo info. */
	if ( save_state )
	{
		if ( box == &origin_constraints )
			Edit_Undo_Register_State(edit_remove_op, ORIGIN, index);
		else if ( box == &scale_constraints )
			Edit_Undo_Register_State(edit_remove_op, SCALE, index);
		else
			Edit_Undo_Register_State(edit_remove_op, ROTATE, index);
	}
		

	/* Destroy the widget. */
	XtDestroyWidget(box->option_widgets[index]);
	for ( i = index + 1 ; i < box->num_options ; i++ )
		box->option_widgets[i-1] = box->option_widgets[i];
	XawFormDoLayout(edit_form, TRUE);

	/* Remove any dependencies. */
	Constraint_Manipulate_Specs(box->options[index], info->obj, NULL, 1,
								Edit_Remove_Obj_From_Dependencies);

	/* Shuffle down the options. */
	for ( i = index + 1 ; i < box->num_options ; i++ )
		box->options[i-1] = box->options[i];
	box->num_options--;

	/* Shuffle down the objects constraints. */
	if ( box == &origin_constraints )
	{
		for ( i = index - 2 ; i < info->obj->o_origin_num ; i++ )
			info->obj->o_origin_cons[i-1] = info->obj->o_origin_cons[i];
		for ( i = index + 1 ; i < info->obj->o_origin_num + 3 ; i++ )
			info->obj->o_origin_active[i-1] = info->obj->o_origin_active[i];

		/* Update the pointers. */
		for ( i = 0 ; i < origin_constraints.num_options - 3 ; i++ )
			origin_constraints.options[ i + 3 ] = info->obj->o_origin_cons + i;

		info->obj->o_origin_num--;
	}
	else if ( box == &scale_constraints )
	{
		for ( i = index - 2 ; i < info->obj->o_scale_num ; i++ )
			info->obj->o_scale_cons[i-1] = info->obj->o_scale_cons[i];
		for ( i = index + 1 ; i < info->obj->o_scale_num + 3 ; i++ )
			info->obj->o_scale_active[i-1] = info->obj->o_scale_active[i];

		/* Update the pointers. */
		for ( i = 0 ; i < scale_constraints.num_options - 3 ; i++ )
			scale_constraints.options[ i + 3 ] = info->obj->o_scale_cons + i;

		info->obj->o_scale_num--;
	}
	else
	{
		for ( i = index - 2 ; i < info->obj->o_rotate_num ; i++ )
			info->obj->o_rotate_cons[i-1] = info->obj->o_rotate_cons[i];
		for ( i = index + 1 ; i < info->obj->o_rotate_num + 3 ; i++ )
			info->obj->o_rotate_active[i-1] = info->obj->o_rotate_active[i];

		/* Update the pointers. */
		for ( i = 0 ; i < rotate_constraints.num_options - 3 ; i++ )
			rotate_constraints.options[ i + 3 ] = info->obj->o_rotate_cons + i;

		info->obj->o_rotate_num--;
	}

	Edit_Match_Widths();

	return TRUE;
}



static void
Edit_Select_Constraint_Callback(Widget widg, XtPointer cl_data, XtPointer ca)
{
	EditInfoPtr			info = Edit_Get_Info();
	ConstraintBoxPtr	box = (ConstraintBoxPtr)cl_data;
	Boolean				new_state = ( ca ? TRUE : FALSE );
	int					index;

	/* Find which widget. */
	for ( index = 0 ; box->option_widgets[index] != widg ; index++ );

	if ( info->deleting && Edit_Remove_Constraint(info, box, index, TRUE) )
		return;

	Edit_Select_Constraint(info, index, box, new_state, FALSE, TRUE);
}

void
Edit_Select_Constraint(EditInfoPtr info, int index, ConstraintBoxPtr box,
					   Boolean new_state, Boolean forced, Boolean draw)
{
	Boolean	old_state;

	if ( box == &origin_constraints )
	{
		box->options[index]->f_status = new_state;
		if ( draw )
		{
			Edit_Undo_Register_State(
				( new_state ? edit_select_op : edit_deselect_op),
				ORIGIN, index);
			Draw_Origin_Constraints(info->window, ViewNone, info, TRUE);
		}
		Constraint_Solve_System(box->options, box->num_options,
								&(info->origin_resulting));
		if ( draw )
			Draw_Origin_Constraints(info->window, CalcView, info, TRUE);
		Edit_Force_Origin_Satisfaction(info);
		info->obj->o_origin_active[index] = new_state;
	}
	if ( box == &scale_constraints )
	{
		if ( ! forced )
		{
			if ( info->obj->o_scale_active[index] )
			{
				if ( draw )
					Edit_Undo_Register_State(edit_deselect_op, SCALE, index);
				info->obj->o_scale_active[index] = FALSE;
				if ( index < 3 )
					new_state = scale_forced[index];
			}
			else
			{
				if ( draw )
					Edit_Undo_Register_State(edit_select_op, SCALE, index);
				info->obj->o_scale_active[index] = TRUE;
				new_state = TRUE;
			}
		}
		else
			new_state = new_state || info->obj->o_scale_active[index];

		box->options[index]->f_status = new_state;
		XtVaGetValues(box->option_widgets[index], XtNstate, &old_state, NULL);
		if ( old_state != new_state )
			XtVaSetValues(box->option_widgets[index], XtNstate, new_state,NULL);

		if ( draw )
			Draw_Scale_Constraints(info->window, ViewNone, info, TRUE);
		Constraint_Solve_System(box->options, box->num_options,
								&(info->reference_resulting));
		if ( draw )
			Draw_Scale_Constraints(info->window, CalcView, info, TRUE);
		Edit_Force_Scale_Satisfaction(info);
	}
	if ( box == &rotate_constraints )
	{
		if ( ! forced )
		{
			if ( info->obj->o_rotate_active[index] )
			{
				if ( draw )
					Edit_Undo_Register_State(edit_deselect_op, ROTATE, index);
				info->obj->o_rotate_active[index] = FALSE;
				if ( index < 2 )
					new_state = rotate_forced[index];
			}
			else
			{
				if ( draw )
					Edit_Undo_Register_State(edit_select_op, ROTATE, index);
				info->obj->o_rotate_active[index] = TRUE;
				new_state = TRUE;
			}
		}
		else
			new_state = new_state || info->obj->o_rotate_active[index];

		box->options[index]->f_status = new_state;
		XtVaGetValues(box->option_widgets[index], XtNstate, &old_state, NULL);
		if ( old_state != new_state )
			XtVaSetValues(box->option_widgets[index], XtNstate, new_state,NULL);

		if ( draw )
			Draw_Rotate_Constraints(info->window, ViewNone, info, TRUE);
		Constraint_Solve_System(box->options, box->num_options,
								&(info->rotate_resulting));
		if ( draw )
			Draw_Rotate_Constraints(info->window, CalcView, info, TRUE);
	}
}


void
Edit_Set_Constraint_States(ConstraintBoxPtr target, Boolean *states)
{
	int	i;

	scale_forced[0] = scale_forced[1] = scale_forced[2] = FALSE;
	for ( i = 0 ; i < target->num_options ; i++ )
	{
		target->options[i]->f_status = states[i];
		XtVaSetValues(target->option_widgets[i], XtNstate, states[i], NULL);
	}
}

void
Constraint_Box_Clear(ConstraintBoxPtr target)
{
	int	i;

	/* Free any extra widgets. */
	for ( i = 3 ; i < target->num_options ; i++ )
		XtDestroyWidget(target->option_widgets[i]);

	target->num_options = 3;	/* The defaults. */
}



void
Add_Dependency(ObjectInstancePtr ref, ObjectInstancePtr obj)
{
	int	i;

	if ( ref == obj )
		return;

	/* Add the dependency to the ref_objects list. */
	if ( ref->o_num_depend == 0 )
	{
		ref->o_dependents = New(Dependent, 1);
		ref->o_dependents[0].obj = obj;
		ref->o_dependents[0].count = 1;
		ref->o_num_depend++;
	}
	else
	{
		for ( i = 0 ; i < ref->o_num_depend ; i++ )
			if ( ref->o_dependents[i].obj == obj )
			{
				ref->o_dependents[i].count++;
				return;
			}

		ref->o_dependents = More(ref->o_dependents, Dependent,
								 ref->o_num_depend + 1);
		ref->o_dependents[ref->o_num_depend].obj = obj;
		ref->o_dependents[ref->o_num_depend].count = 1;
		ref->o_num_depend++;
	}
}


void
Add_Spec(FeatureSpecifier *spec, Vector pt, Vector center, FeatureSpecType type,
		 ObjectInstancePtr ref, int index)
{
	ObjectInstancePtr	obj = Edit_Get_Info()->obj;

	spec->spec_type = type;
	switch ( type )
	{
		case absolute_spec:
		case origin_spec:
		case ref_point_spec:
			spec->spec_object = NULL;
			spec->spec_vector = pt;
			break;

		case offset_spec:
			spec->spec_object = NULL;
			VSub(pt, center, spec->spec_vector);
			break;

		case reference_spec:
			spec->spec_object = ref;
			spec->spec_vector = ref->o_wireframe->vertices[index];

			Add_Dependency(ref, obj);

			break;
	}
}


static void
Add_Check_Specifiers(int *indices, FeatureSpecType *specs, int num,
					 EditInfoPtr info)
{
	int		i;
	Boolean	changed = FALSE;

	for ( i = 0 ; i < num ; i++ )
	{
		if ( specs[i] == reference_spec &&
			 select_verts[indices[i]].obj == info->obj )
		{
			specs[i] = ( add_to_box == &origin_constraints ) ?
						 absolute_spec : offset_spec;
			changed = TRUE;
		}
	}

	if ( changed )
	{
		if ( add_to_box == &origin_constraints )
			Popup_Error("Reference specifiers changed to Absolutes",
						info->window->shell, "Error");
		else
			Popup_Error("Reference specifiers changed to Offsets",
						info->window->shell, "Error");
	}
}


/*	void
**	Add_Plane_Constraint_Callback(int *indices, FeatureSpecType *specs)
**	Adds a plane as defined by the three indices into select_verts.
*/
static void
Add_Plane_Constraint_Callback(int *indices, FeatureSpecType *specs)
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new_feature;
	Vector		endpoints[3];
	Vector		temp_v1, temp_v2;
	double		temp_d;
	int			i;

	Add_Check_Specifiers(indices, specs, 3, info);

	new_feature.f_type = plane_feature;
	new_feature.f_spec_type = plane_feature;
	new_feature.f_status = FALSE;
	for ( i = 0 ; i < 3 ; i++ )
	{
		endpoints[i] = select_verts[indices[i]].
						obj->o_world_verts[select_verts[indices[i]].offset];
		Add_Spec(new_feature.f_specs + i, endpoints[i],
			info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[i],
			select_verts[indices[i]].obj, select_verts[indices[i]].offset);
	}

	VSub(endpoints[1], endpoints[0], temp_v1);
	VSub(endpoints[2], endpoints[0], temp_v2);
	VCross(temp_v1, temp_v2, new_feature.f_vector);
	VUnit(new_feature.f_vector, temp_d, new_feature.f_vector);
	new_feature.f_point = endpoints[0];

	new_feature.f_value = VDot(new_feature.f_vector, new_feature.f_point);

	Add_Object_Constraint(&new_feature, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


/*	void
**	Add_Line_Constraint_Callback(int *indices)
**	Adds a line as defined by the two indices into select_verts.
*/
static void
Add_Line_Constraint_Callback(int *indices, FeatureSpecType *specs)
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new;
	Vector		endpoints[2];
	int			i;
	double		temp_d;

	Add_Check_Specifiers(indices, specs, 2, info);

	new.f_type = line_feature;
	new.f_spec_type = line_feature;
	new.f_status = FALSE;
	for ( i = 0 ; i < 2 ; i++ )
	{
		endpoints[i] = select_verts[indices[i]].
						obj->o_world_verts[select_verts[indices[i]].offset];
		Add_Spec(new.f_specs + i, endpoints[i],
			info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[i],
			select_verts[indices[i]].obj, select_verts[indices[i]].offset);
	}

	VSub(endpoints[1], endpoints[0], new.f_vector);
	VUnit(new.f_vector, temp_d, new.f_vector);
	new.f_point = endpoints[0];

	Add_Object_Constraint(&new, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


/*	void
**	Add_Line_Constraint_Callback(int *indices)
**	Adds the point indexed by *indices.
*/
static void
Add_Point_Constraint_Callback(int *indices, FeatureSpecType *specs)
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new;

	debug(FUNC_NAME,fprintf(stderr,"Add_Point_Constraint_Callback()\n"));

	Add_Check_Specifiers(indices, specs, 1, info);

	new.f_type = point_feature;
	new.f_spec_type = point_feature;
	new.f_status = FALSE;
	new.f_point = select_verts[indices[0]].
					obj->o_world_verts[select_verts[indices[0]].offset];
	VNew(0, 0, 0, new.f_vector);
	Add_Spec(new.f_specs, new.f_point,
			 info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[0],
			 select_verts[indices[0]].obj, select_verts[indices[0]].offset);

	Add_Object_Constraint(&new, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


/*	void
**	Add_Midplane_Constraint_Callback(int *indices)
**	Adds a plane constraint that is the equidistant from the 2 indices given.
*/
static void
Add_Midplane_Constraint_Callback(int *indices, FeatureSpecType *specs) 
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new;
	Vector	endpoints[2];
	double	temp_d;
	int		i;

	Add_Check_Specifiers(indices, specs, 2, info);

	new.f_type = plane_feature;
	new.f_spec_type = midplane_feature;
	new.f_status = FALSE;
	for ( i = 0 ; i < 2 ; i++ )
	{
		endpoints[i] = select_verts[indices[i]].
						obj->o_world_verts[select_verts[indices[i]].offset];
		Add_Spec(new.f_specs + i, endpoints[i],
			info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[i],
			select_verts[indices[i]].obj, select_verts[indices[i]].offset);
	}

	VSub(endpoints[0], endpoints[1], new.f_vector);
	VUnit(new.f_vector, temp_d, new.f_vector);

	VNew((endpoints[0].x + endpoints[1].x) / 2,
		 (endpoints[0].y + endpoints[1].y) / 2,
		 (endpoints[0].z + endpoints[1].z) / 2, new.f_point);

	new.f_value = VDot(new.f_vector, new.f_point);

	Add_Object_Constraint(&new, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


/*	void
**	Add_Midpoint_Constraint_Callback(int *indices)
**	Adds a point constraint that is the equidistant from the 2 indices given.
*/
static void
Add_Midpoint_Constraint_Callback(int *indices, FeatureSpecType *specs) 
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new;
	Vector	endpoints[2];
	int		i;

	Add_Check_Specifiers(indices, specs, 2, info);

	new.f_type = point_feature;
	new.f_spec_type = midpoint_feature;
	new.f_status = FALSE;
	for ( i = 0 ; i < 2 ; i++ )
	{
		endpoints[i] = select_verts[indices[i]].
						obj->o_world_verts[select_verts[indices[i]].offset];
		Add_Spec(new.f_specs + i, endpoints[i],
			info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[i],
			select_verts[indices[i]].obj, select_verts[indices[i]].offset);
	}

	VNew((endpoints[0].x + endpoints[1].x) / 2,
		 (endpoints[0].y + endpoints[1].y) / 2,
		 (endpoints[0].z + endpoints[1].z) / 2, new.f_point);
	VNew(0, 0, 0, new.f_vector);

	Add_Object_Constraint(&new, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


static void
Add_Axis_Plane_Constraint_Callback(int *indices, FeatureSpecType *specs)
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new_feature;
	Vector		endpoints[3];
	Vector		temp_v1, temp_v2;
	double		temp_d;
	int			i;

	new_feature.f_type = line_feature;
	new_feature.f_spec_type = axis_plane_feature;
	new_feature.f_status = FALSE;
	for ( i = 0 ; i < 3 ; i++ )
	{
		endpoints[i] = select_verts[indices[i]].
						obj->o_world_verts[select_verts[indices[i]].offset];
		Add_Spec(new_feature.f_specs + i, endpoints[i],
			info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[i],
			select_verts[indices[i]].obj, select_verts[indices[i]].offset);
	}

	VSub(endpoints[1], endpoints[0], temp_v1);
	VSub(endpoints[2], endpoints[0], temp_v2);
	VCross(temp_v1, temp_v2, new_feature.f_vector);
	VUnit(new_feature.f_vector, temp_d, new_feature.f_vector);
	new_feature.f_point = endpoints[0];

	Add_Object_Constraint(&new_feature, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


static void
Add_Axis_Line_Constraint_Callback(int *indices, FeatureSpecType *specs)
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new;
	Vector		endpoints[2];
	int			i;
	double		temp_d;

	new.f_type = line_feature;
	new.f_spec_type = axis_feature;
	new.f_status = FALSE;
	for ( i = 0 ; i < 2 ; i++ )
	{
		endpoints[i] = select_verts[indices[i]].
						obj->o_world_verts[select_verts[indices[i]].offset];
		Add_Spec(new.f_specs + i, endpoints[i],
			info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[i],
			select_verts[indices[i]].obj, select_verts[indices[i]].offset);
	}

	VSub(endpoints[1], endpoints[0], new.f_vector);
	VUnit(new.f_vector, temp_d, new.f_vector);
	new.f_point = endpoints[0];

	Add_Object_Constraint(&new, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


/*	void
**	Add_Line_Constraint_Callback(int *indices)
**	Adds the point indexed by *indices.
*/
static void
Add_Orig_Line_Constraint_Callback(int *indices, FeatureSpecType *specs)
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new;
	double		temp_d;
	Vector		diff;

	new.f_type = line_feature;
	new.f_spec_type = orig_line_feature;
	new.f_status = FALSE;
	new.f_point = select_verts[indices[0]].
					obj->o_world_verts[select_verts[indices[0]].offset];

	if ( VEqual(new.f_point, info->origin, diff) )
	{
		Edit_Cleanup_Selection(TRUE);
		Popup_Error("Origin point and chosen point are the same!",
					info->window->shell, "Error");
		return;
	}

	Add_Spec(new.f_specs, info->origin,
			 info->obj->o_world_verts[info->obj->o_num_vertices - 1],
			 origin_spec, NULL, 0);
	Add_Spec(new.f_specs + 1, new.f_point,
			 info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[0],
			 select_verts[indices[0]].obj, select_verts[indices[0]].offset);

	VSub(new.f_point, info->origin, new.f_vector);
	VUnit(new.f_vector, temp_d, new.f_vector);
	new.f_point = info->origin;

	Add_Object_Constraint(&new, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


/*	void
**	Add_Ref_Line_Constraint_Callback(int *indices)
**	Adds a reference line costraint with point refd by indices[0].
*/
static void
Add_Ref_Line_Constraint_Callback(int *indices, FeatureSpecType *specs)
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new;
	double		temp_d;
	Vector		diff;

	new.f_type = line_feature;
	new.f_spec_type = ref_line_feature;
	new.f_status = FALSE;
	new.f_point = select_verts[indices[0]].
					obj->o_world_verts[select_verts[indices[0]].offset];

	if ( VEqual(new.f_point, info->reference, diff) )
	{
		Edit_Cleanup_Selection(TRUE);
		Popup_Error("Scaling point and chosen point are the same!",
					info->window->shell, "Error");
		return;
	}

	Add_Spec(new.f_specs, info->reference,
			 info->obj->o_world_verts[info->obj->o_num_vertices - 1],
			 ref_point_spec, NULL, 0);
	Add_Spec(new.f_specs + 1, new.f_point,
			 info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[0],
			 select_verts[indices[0]].obj, select_verts[indices[0]].offset);

	VSub(new.f_point, info->reference, new.f_vector);
	VUnit(new.f_vector, temp_d, new.f_vector);
	new.f_point = info->reference;

	Add_Object_Constraint(&new, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


static void
Add_Orig_Plane_Constraint_Callback(int *indices, FeatureSpecType *specs)
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new_feature;
	Vector		endpoints[3];
	Vector		temp_v1, temp_v2;
	double		temp_d;
	int			i;

	new_feature.f_type = plane_feature;
	new_feature.f_spec_type = orig_plane_feature;
	new_feature.f_status = FALSE;
	endpoints[0] = info->origin;
	for ( i = 0 ; i < 2 ; i++ )
		endpoints[i+1] = select_verts[indices[i]].
						obj->o_world_verts[select_verts[indices[i]].offset];

	if ( Points_Colinear(endpoints[0], endpoints[1], endpoints[2]) )
	{
		Edit_Cleanup_Selection(TRUE);
		Popup_Error("Origin point and chosen points are colinear!",
					info->window->shell, "Error");
		return;
	}

	Add_Spec(new_feature.f_specs, info->origin,
			 info->obj->o_world_verts[info->obj->o_num_vertices - 1],
			 origin_spec, NULL, 0);
	Add_Spec(new_feature.f_specs + i, endpoints[i],
		info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[0],
		select_verts[indices[0]].obj, select_verts[indices[0]].offset);
	Add_Spec(new_feature.f_specs + i, endpoints[i],
		info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[1],
		select_verts[indices[1]].obj, select_verts[indices[1]].offset);

	VSub(endpoints[1], endpoints[0], temp_v1);
	VSub(endpoints[2], endpoints[0], temp_v2);
	VCross(temp_v1, temp_v2, new_feature.f_vector);
	VUnit(new_feature.f_vector, temp_d, new_feature.f_vector);
	new_feature.f_point = endpoints[0];

	new_feature.f_value = VDot(new_feature.f_vector, new_feature.f_point);

	Add_Object_Constraint(&new_feature, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


static void
Add_Ref_Plane_Constraint_Callback(int *indices, FeatureSpecType *specs)
{
	EditInfoPtr	info = Edit_Get_Info();
	FeatureData	new_feature;
	Vector		endpoints[3];
	Vector		temp_v1, temp_v2;
	double		temp_d;
	int			i;

	new_feature.f_type = plane_feature;
	new_feature.f_spec_type = ref_plane_feature;
	new_feature.f_status = FALSE;
	endpoints[0] = info->reference;
	for ( i = 0 ; i < 2 ; i++ )
		endpoints[i+1] = select_verts[indices[i]].
						obj->o_world_verts[select_verts[indices[i]].offset];

	if ( Points_Colinear(endpoints[0], endpoints[1], endpoints[2]) )
	{
		Edit_Cleanup_Selection(TRUE);
		Popup_Error("Scaling point and chosen points are colinear!",
					info->window->shell, "Error");
		return;
	}

	Add_Spec(new_feature.f_specs, info->reference,
			 info->obj->o_world_verts[info->obj->o_num_vertices - 1],
			 ref_point_spec, NULL, 0);
	Add_Spec(new_feature.f_specs + 1, endpoints[1],
		info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[0],
		select_verts[indices[0]].obj, select_verts[indices[0]].offset);
	Add_Spec(new_feature.f_specs + 2, endpoints[2],
		info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[1],
		select_verts[indices[1]].obj, select_verts[indices[1]].offset);

	VSub(endpoints[1], endpoints[0], temp_v1);
	VSub(endpoints[2], endpoints[0], temp_v2);
	VCross(temp_v1, temp_v2, new_feature.f_vector);
	VUnit(new_feature.f_vector, temp_d, new_feature.f_vector);
	new_feature.f_point = endpoints[0];

	new_feature.f_value = VDot(new_feature.f_vector, new_feature.f_point);

	Add_Object_Constraint(&new_feature, add_to_box, -1);

	Edit_Cleanup_Selection(TRUE);
}


static void
Add_Name_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	EditInfoPtr	info = Edit_Get_Info();
	char	*name = XawDialogGetValueString(add_name_dialog);

	XtPopdown(add_name_shell);

	if ( add_to_box == &origin_constraints )
	{
		info->obj->o_origin_cons[info->obj->o_origin_num].f_label =Strdup(name);
		Edit_Add_Constraint(&origin_constraints, info->obj->o_origin_num + 3);
		info->obj->o_origin_num++;
	}
	else if ( add_to_box == &scale_constraints )
	{
		info->obj->o_scale_cons[info->obj->o_scale_num].f_label = Strdup(name);
		Edit_Add_Constraint(&scale_constraints, info->obj->o_scale_num + 3);
		info->obj->o_scale_num++;
	}
	else
	{
		info->obj->o_rotate_cons[info->obj->o_rotate_num].f_label =Strdup(name);
		Edit_Add_Constraint(&rotate_constraints, info->obj->o_rotate_num + 3);
		info->obj->o_rotate_num++;
	}

	Edit_Match_Widths();
}

void
Add_Name_Action_Func(Widget w, XEvent *e, String *s, Cardinal *num)
{
	Add_Name_Callback(w, NULL, NULL);
}

static void
Add_Name_Create_Shell()
{
	Arg	args[2];
	int	n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Add Name");		n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	add_name_shell = XtCreatePopupShell("addNameShell",
					 transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Name:");	n++;
	XtSetArg(args[n], XtNvalue, "");		n++;
	add_name_dialog = XtCreateManagedWidget("addNameDialog", dialogWidgetClass,
						add_name_shell, args, n);

	XawDialogAddButton(add_name_dialog, "Add", Add_Name_Callback, NULL);

	XtOverrideTranslations(XtNameToWidget(add_name_dialog, "value"),
		XtParseTranslationTable(":<Key>Return: Edit_Add_Name()"));

	XtRealizeWidget(add_name_shell);
}



void
Add_Object_Constraint(FeaturePtr new, ConstraintBoxPtr box, int index)
{
	EditInfoPtr	info = Edit_Get_Info();
	char		default_name[20];
	int			i;
	Boolean		have_name = ( index != -1 );

	add_to_box = box;

	if ( box == &origin_constraints )
	{
		if ( index == -1 )
		{
			index = info->obj->o_origin_num;
			Edit_Undo_Register_State(edit_add_op, ORIGIN, index);
		}

		if ( info->obj->o_origin_num )
			info->obj->o_origin_cons = More(info->obj->o_origin_cons,
											FeatureData,
											info->obj->o_origin_num + 1);
		else
			info->obj->o_origin_cons = New(FeatureData,
										   info->obj->o_origin_num + 1);
		info->obj->o_origin_active = More(info->obj->o_origin_active, Boolean,
										  info->obj->o_origin_num + 4);

		/* Update the pointers. */
		for ( i = 0 ; i < origin_constraints.num_options - 3 ; i++ )
			origin_constraints.options[ i + 3 ] = info->obj->o_origin_cons + i;

		/* Shuffle things up. */
		for ( i = info->obj->o_origin_num ; i > index ; i-- )
		{
			info->obj->o_origin_cons[i] = info->obj->o_origin_cons[i-1];
			info->obj->o_origin_active[i+3] = info->obj->o_origin_active[i+2];
			origin_constraints.options[i + 2] = info->obj->o_origin_cons + i;
		}

		info->obj->o_origin_cons[index] = *new;
		info->obj->o_origin_active[index + 3] = FALSE;

		if ( have_name )
		{
			Edit_Add_Constraint(&origin_constraints, index + 3);
			info->obj->o_origin_num++;
			return;
		}
	}
	else if ( box == &scale_constraints )
	{
		if ( index == -1 )
		{
			index = info->obj->o_scale_num;
			Edit_Undo_Register_State(edit_add_op, SCALE, index);
		}

		if ( info->obj->o_scale_num )
			info->obj->o_scale_cons = More(info->obj->o_scale_cons, FeatureData,
										   info->obj->o_scale_num + 1);
		else
			info->obj->o_scale_cons = New(FeatureData,
										  info->obj->o_scale_num + 1);
		info->obj->o_scale_active = More(info->obj->o_scale_active, Boolean,
										 info->obj->o_scale_num + 4);

		/* Update the pointers. */
		for ( i = 0 ; i < scale_constraints.num_options - 3 ; i++ )
			scale_constraints.options[ i + 3 ] = info->obj->o_scale_cons + i;

		/* Shuffle things up. */
		for ( i = info->obj->o_scale_num ; i > index ; i-- )
		{
			info->obj->o_scale_cons[i] = info->obj->o_scale_cons[i-1];
			info->obj->o_scale_active[i+3] = info->obj->o_scale_active[i+2];
			scale_constraints.options[i + 2] = info->obj->o_scale_cons + i;
		}

		info->obj->o_scale_cons[index] = *new;
		info->obj->o_scale_active[index + 3] = FALSE;

		if ( have_name )
		{
			Edit_Add_Constraint(&scale_constraints, index + 3);
			info->obj->o_scale_num++;
			return;
		}
	}
	else
	{
		if ( index == -1 )
		{
			index = info->obj->o_rotate_num;
			Edit_Undo_Register_State(edit_add_op, ROTATE, index);
		}

		if ( info->obj->o_rotate_num )
			info->obj->o_rotate_cons = More(info->obj->o_rotate_cons,
											FeatureData,
											info->obj->o_rotate_num + 1);
		else
			info->obj->o_rotate_cons = New(FeatureData,
										   info->obj->o_rotate_num + 1);
		info->obj->o_rotate_active = More(info->obj->o_rotate_active, Boolean,
										  info->obj->o_rotate_num + 4);

		/* Update the pointers. */
		for ( i = 0 ; i < rotate_constraints.num_options - 3 ; i++ )
			rotate_constraints.options[ i + 3 ] = info->obj->o_rotate_cons + i;

		/* Shuffle things up. */
		for ( i = info->obj->o_rotate_num ; i > index ; i-- )
		{
			info->obj->o_rotate_cons[i] = info->obj->o_rotate_cons[i-1];
			info->obj->o_rotate_active[i+3] = info->obj->o_rotate_active[i+2];
			rotate_constraints.options[i + 2] = info->obj->o_rotate_cons + i;
		}

		info->obj->o_rotate_cons[index] = *new;
		info->obj->o_rotate_active[index + 3] = FALSE;

		if ( have_name )
		{
			Edit_Add_Constraint(&rotate_constraints, index + 3);
			info->obj->o_rotate_num++;
			return;
		}
	}

	switch ( new->f_spec_type )
	{
		case plane_feature:
			sprintf(default_name, "Plane %d", feature_counts[new->f_type]);
			break;
		case line_feature:
			sprintf(default_name, "Line %d", feature_counts[new->f_type]);
			break;
		case point_feature:
			sprintf(default_name, "Point %d", feature_counts[new->f_type]);
			break;
		case midplane_feature:
			sprintf(default_name, "Midplane %d", feature_counts[new->f_type]);
			break;
		case midpoint_feature:
			sprintf(default_name, "Midpoint %d", feature_counts[new->f_type]);
			break;
		case axis_plane_feature:
			sprintf(default_name, "Axis Plane %d", feature_counts[new->f_type]);
			break;
		case axis_feature:
			sprintf(default_name, "Axis Line %d", feature_counts[new->f_type]);
			break;
		case orig_line_feature:
			sprintf(default_name, "Orig_Line %d", feature_counts[new->f_type]);
			break;
		case ref_line_feature:
			sprintf(default_name, "Scale_Line %d", feature_counts[new->f_type]);
			break;
		case orig_plane_feature:
			sprintf(default_name, "Orig_Plane %d", feature_counts[new->f_type]);
			break;
		case ref_plane_feature:
			sprintf(default_name, "Scale_Plane %d",feature_counts[new->f_type]);
			break;
		default:;
	}

	feature_counts[new->f_type]++;

	if ( ! add_name_shell )
		Add_Name_Create_Shell();

	XtVaSetValues(add_name_dialog, XtNvalue, default_name, NULL);

	SFpositionWidget(add_name_shell);

	XtPopup(add_name_shell, XtGrabExclusive);
}
