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
**	Sced: A Constraint Based Object Scene Editor
**
**	edit_undo.c: Functions for edit undo and redo.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <X11/Xaw/Toggle.h>

typedef struct _EditStateType {
	EditOpType		edit_op;
	int				op_type;
	Transformation	saved_current;
	Vector			saved_reference;
	Vector			saved_origin;
	Matrix			saved_axes;
	Quaternion		saved_rot;
	Vector			saved_scale;
	int				con_index;
	FeaturePtr		con_feature;
	struct _EditStateType	*next;
	} EditStateType, *EditStatePtr;

extern Boolean	scale_forced[3];

static void	Edit_Undo_Transformation_Op(EditStatePtr);
static void	Edit_Undo_Select_Op(EditStatePtr, Boolean);
static void	Edit_Undo_Add_Op(EditStatePtr);
static void	Edit_Undo_Remove_Op(EditStatePtr);
static void	Edit_Undo_Align(EditStatePtr state_elmt);
static void	Edit_Undo_Remove_Align(EditStatePtr state_elmt);
static void	Free_Stack_Elmt(EditStatePtr);

static EditStatePtr	undo_stack = NULL;
static EditStatePtr	redo_stack = NULL;


void
Edit_Undo()
{
	EditStatePtr	temp;

	if ( ! undo_stack )
	{
		XBell(XtDisplay(main_window.shell), 0);
		return;
	}

	switch ( undo_stack->edit_op )
	{
		case edit_drag_op:
		case edit_reference_op:
		case edit_origin_op:
		case edit_axis_op:
			Edit_Undo_Transformation_Op(undo_stack);
			break;
		case edit_select_op:
			Edit_Undo_Select_Op(undo_stack, FALSE);
			break;
		case edit_deselect_op:
			Edit_Undo_Select_Op(undo_stack, TRUE);
			break;
		case edit_add_op:
			Edit_Undo_Add_Op(undo_stack);
			break;
		case edit_remove_op:
			Edit_Undo_Remove_Op(undo_stack);
			break;
		case edit_major_op:
		case edit_minor_op:
			Edit_Undo_Align(undo_stack);
			break;
		case edit_major_remove_op:
		case edit_minor_remove_op:
			Edit_Undo_Remove_Align(undo_stack);
			break;
	}

	/* Put the first thing on the undo stack onto the redo stack. */
	temp = undo_stack;
	undo_stack = undo_stack->next;
	temp->next = redo_stack;
	redo_stack = temp;
}


void
Edit_Redo(Widget w, XtPointer cl, XtPointer ca)
{
	EditStatePtr	temp;

	if ( ! redo_stack )
	{
		XBell(XtDisplay(main_window.shell), 0);
		return;
	}

	switch ( redo_stack->edit_op )
	{
		case edit_drag_op:
		case edit_reference_op:
		case edit_origin_op:
		case edit_axis_op:
			Edit_Undo_Transformation_Op(redo_stack);
			break;
		case edit_select_op:
			Edit_Undo_Select_Op(redo_stack, FALSE);
			break;
		case edit_deselect_op:
			Edit_Undo_Select_Op(redo_stack, TRUE);
			break;
		case edit_add_op:
			Edit_Undo_Add_Op(redo_stack);
			break;
		case edit_remove_op:
			Edit_Undo_Remove_Op(redo_stack);
			break;
		case edit_major_op:
		case edit_minor_op:
			Edit_Undo_Align(redo_stack);
			break;
		case edit_major_remove_op:
		case edit_minor_remove_op:
			Edit_Undo_Remove_Align(redo_stack);
			break;
	}

	/* Put the first thing on the redo stack onto the undo stack. */
	temp = redo_stack;
	redo_stack = redo_stack->next;
	temp->next = undo_stack;
	undo_stack = temp;
}


void
Edit_Undo_Register_State(EditOpType type, int sub_type, int index)
{
	EditStatePtr	new_elmt = New(EditStateType, 1);
	EditInfoPtr		info = Edit_Get_Info();
	EditStatePtr	temp;

	/* If there's anything on the redo list, trash it. Can't redo
	** once another thing has been done.
	*/
	while ( redo_stack )
	{
		temp = redo_stack;
		redo_stack = redo_stack->next;
		Free_Stack_Elmt(temp);
	}
	redo_stack = NULL;


	new_elmt->edit_op = type;
	new_elmt->op_type = sub_type;

	new_elmt->saved_current = info->obj->o_transform;
	new_elmt->saved_reference = info->reference;
	new_elmt->saved_origin = info->origin;
	new_elmt->saved_axes = info->axes;
	debug(ROT_RT,Print_Quaternion(&info->rotation));
	new_elmt->saved_rot = info->rotation;
	new_elmt->saved_scale = info->scale;

	new_elmt->con_index = index;

	if ( type == edit_remove_op )
	{
		new_elmt->con_feature = New(FeatureData, 1);
		switch ( sub_type )
		{
			case ORIGIN:
				*(new_elmt->con_feature) =
					info->obj->o_origin_cons[index - 3];
				break;
			case SCALE:
				*(new_elmt->con_feature) =
					info->obj->o_scale_cons[index - 3];
				break;
			case ROTATE:
				*(new_elmt->con_feature) =
					info->obj->o_rotate_cons[index - 3];
				break;
		}
	}

	if ( type == edit_major_remove_op )
	{
		new_elmt->con_feature = New(FeatureData, 1);
		*(new_elmt->con_feature) = info->obj->o_major_align;
	}
	if ( type == edit_minor_remove_op )
	{
		new_elmt->con_feature = New(FeatureData, 1);
		*(new_elmt->con_feature) = info->obj->o_minor_align;
	}

	new_elmt->next = undo_stack;
	undo_stack = new_elmt;
}


static void
Edit_Undo_Transformation_Op(EditStatePtr state_elmt)
{
	EditInfoPtr	info = Edit_Get_Info();
	EditStateType	redo_state;

	/* Remember where we are now for redo. */
	redo_state.saved_current = info->obj->o_transform;
	redo_state.saved_reference = info->reference;
	redo_state.saved_origin = info->origin;
	redo_state.saved_axes = info->axes;
	redo_state.saved_rot = info->rotation;
	redo_state.saved_scale = info->scale;


	if ( ! do_maintenance )
	{
		/* Undraw all the old stuff. */
		Edit_Draw(info->window, ViewNone, info, FALSE);
		Draw_Edit_Extras(info->window, ViewNone, info, FALSE);
		Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);
	}

	/* Put everything back the way it was. */
	info->obj->o_transform = state_elmt->saved_current;
	info->origin = state_elmt->saved_origin;
	info->reference = state_elmt->saved_reference;
	info->axes = state_elmt->saved_axes;
	info->axes_inverse = MInvert(&(info->axes));
	debug(ROT_RT,Print_Quaternion(&state_elmt->saved_rot));
	info->rotation = state_elmt->saved_rot;
	info->scale = state_elmt->saved_scale;

	/* Recalc all the vertices and normals. */
	Edit_Transform_Vertices(&(info->obj->o_transform), info);
	Edit_Transform_Normals(&(info->obj->o_transform), info);

	/* Save the body stuff. */
	VSub(info->origin, info->obj->o_world_verts[info->obj->o_num_vertices-1],
		 info->obj->o_origin);
	VSub(info->reference, info->obj->o_world_verts[info->obj->o_num_vertices-1],
		 info->obj->o_reference);
	info->obj->o_axes = info->axes;

	if ( ! do_maintenance )
	{
		Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);
	}

	/* Rework all the constraints. */
	Edit_Update_Constraints(info);
	Edit_Scale_Force_Constraints(info, FALSE);
	Constraint_Solve_System(origin_constraints.options,
							origin_constraints.num_options,
							&(info->origin_resulting));
	Constraint_Solve_System(scale_constraints.options,
							scale_constraints.num_options,
							&(info->reference_resulting));
	Constraint_Solve_System(rotate_constraints.options,
							rotate_constraints.num_options,
							&(info->rotate_resulting));

	if ( ! do_maintenance )
	{
	}

	if ( do_maintenance )
		/* Maintain constraints. */
		Edit_Maintain_All_Constraints(info);
	else
	{
		Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Origin_Constraints(info->window, CalcView, info, FALSE);
		Draw_Scale_Constraints(info->window, CalcView, info, FALSE);
		Draw_Rotate_Constraints(info->window, CalcView, info, FALSE);
		Edit_Draw(info->window, CalcView, info, FALSE);
		Draw_Edit_Extras(info->window, CalcView, info, TRUE);
	}


	/* Change the undo_stack head ready to be put on the redo stack. */
	state_elmt->saved_current = redo_state.saved_current;
	state_elmt->saved_reference = redo_state.saved_reference;
	state_elmt->saved_origin = redo_state.saved_origin;
	state_elmt->saved_axes = redo_state.saved_axes;
	state_elmt->saved_rot = redo_state.saved_rot;
	state_elmt->saved_scale = redo_state.saved_scale;
}


static void
Edit_Undo_Select_Op(EditStatePtr state_elmt, Boolean state)
{
	EditInfoPtr		info = Edit_Get_Info();
	EditStateType	redo_state;
	int				index = state_elmt->con_index;;

	redo_state.con_index = state_elmt->con_index;
	if ( state_elmt->edit_op == edit_select_op )
		redo_state.edit_op = edit_deselect_op;
	else
		redo_state.edit_op = edit_select_op;

	/* Do the transformation bit. */
	Edit_Undo_Transformation_Op(state_elmt);

	/* Set the state of the constraint. */
	switch ( state_elmt->op_type )
	{
		case ORIGIN:
			XtVaSetValues(origin_constraints.option_widgets[index],
						  XtNstate, state, NULL);
			origin_constraints.options[index]->f_status = state;
			info->obj->o_origin_active[index] = state;
			Draw_Origin_Constraints(info->window, ViewNone, info, TRUE);
			Constraint_Solve_System(origin_constraints.options,
									origin_constraints.num_options,
									&(info->origin_resulting));
			Draw_Origin_Constraints(info->window, CalcView, info, TRUE);
			break;
		case SCALE:
			if ( index < 3 )
				scale_constraints.options[index]->f_status =
					state || scale_forced[index];
			else
				scale_constraints.options[index]->f_status = state;
			XtVaSetValues(scale_constraints.option_widgets[index],
						  XtNstate, scale_constraints.options[index]->f_status,
						  NULL);
			info->obj->o_scale_active[index] = state;
			Draw_Scale_Constraints(info->window, ViewNone, info, TRUE);
			Constraint_Solve_System(scale_constraints.options,
									scale_constraints.num_options,
									&(info->reference_resulting));
			Draw_Scale_Constraints(info->window, CalcView, info, TRUE);
			break;
		case ROTATE:
			XtVaSetValues(rotate_constraints.option_widgets[index],
						  XtNstate, state, NULL);
			rotate_constraints.options[index]->f_status = state;
			info->obj->o_rotate_active[index] = state;
			Draw_Rotate_Constraints(info->window, ViewNone, info, TRUE);
			Constraint_Solve_System(rotate_constraints.options,
									rotate_constraints.num_options,
									&(info->rotate_resulting));
			Draw_Rotate_Constraints(info->window, CalcView, info, TRUE);
			break;
	}

	state_elmt->con_index = redo_state.con_index;
	state_elmt->edit_op = redo_state.edit_op;
}


static void
Edit_Undo_Add_Op(EditStatePtr state_elmt)
{
	/* Remember the inverse. */
	state_elmt->edit_op = edit_remove_op;
	state_elmt->con_index += 3;
	state_elmt->con_feature = New(FeatureData, 1);

	/* Delete the constraint that was added. */
	switch ( state_elmt->op_type )
	{
		case ORIGIN:
			*(state_elmt->con_feature) =
				*(origin_constraints.options[state_elmt->con_index]);
			Edit_Remove_Constraint(Edit_Get_Info(), &origin_constraints,
								   state_elmt->con_index, FALSE);
			break;
		case SCALE:
			*(state_elmt->con_feature) =
				*(scale_constraints.options[state_elmt->con_index]);
			Edit_Remove_Constraint(Edit_Get_Info(), &scale_constraints,
								   state_elmt->con_index, FALSE);
			break;
		case ROTATE:
			*(state_elmt->con_feature) =
				*(rotate_constraints.options[state_elmt->con_index]);
			Edit_Remove_Constraint(Edit_Get_Info(), &rotate_constraints,
								   state_elmt->con_index, FALSE);
			break;
	}
	Edit_Match_Widths();
}


static void
Edit_Undo_Align(EditStatePtr state_elmt)
{
	EditInfoPtr	info = Edit_Get_Info();

	state_elmt->con_feature = New(FeatureData, 1);

	if ( state_elmt->edit_op == edit_major_op )
	{
		state_elmt->edit_op = edit_major_remove_op;
		*(state_elmt->con_feature) = info->obj->o_major_align;
		Constraint_Manipulate_Specs(&(info->obj->o_major_align), info->obj,
									NULL, 1, Edit_Remove_Obj_From_Dependencies);
		info->obj->o_major_align.f_type = null_feature;
		info->obj->o_major_align.f_spec_type = null_feature;
		Edit_Set_Major_Align_Label(FALSE);
	}
	else
	{
		state_elmt->edit_op = edit_minor_remove_op;
		*(state_elmt->con_feature) = info->obj->o_minor_align;
		Constraint_Manipulate_Specs(&(info->obj->o_major_align), info->obj,
									NULL, 1, Edit_Remove_Obj_From_Dependencies);
		info->obj->o_minor_align.f_type = null_feature;
		info->obj->o_minor_align.f_spec_type = null_feature;
		Edit_Set_Minor_Align_Label(FALSE);
	}

	Edit_Force_Rotation_Constraints(info, FALSE);
	Edit_Undo_Transformation_Op(state_elmt);
}


static void
Add_Dependencies(FeatureSpecPtr spec, ObjectInstancePtr obj, void *ptr, int i)
{
	if ( spec->spec_type == reference_spec )
		Add_Dependency(spec->spec_object, obj);
}


static void
Edit_Undo_Remove_Op(EditStatePtr state_elmt)
{
	EditInfoPtr	info = Edit_Get_Info();

	/* The inverse is an add op. */
	state_elmt->edit_op = edit_add_op;
	state_elmt->con_index -= 3;

	/* Need to re-add the dependencies, since they were removed with the
	** constraint.
	*/
	Constraint_Manipulate_Specs(state_elmt->con_feature, info->obj, NULL, 0,
								Add_Dependencies);

	/* Reinstate the deleted constraint. */
	switch ( state_elmt->op_type )
	{
		case ORIGIN:
			Add_Object_Constraint(state_elmt->con_feature, &origin_constraints,
								  state_elmt->con_index);
			break;
		case SCALE:
			Add_Object_Constraint(state_elmt->con_feature, &scale_constraints,
								  state_elmt->con_index);
			break;
		case ROTATE:
			Add_Object_Constraint(state_elmt->con_feature, &rotate_constraints,
								  state_elmt->con_index);
			break;
	}
	free(state_elmt->con_feature);

	Edit_Match_Widths();
}


static void
Edit_Undo_Remove_Align(EditStatePtr state_elmt)
{
	EditInfoPtr	info = Edit_Get_Info();

	if ( state_elmt->edit_op == edit_major_remove_op )
	{
		state_elmt->edit_op = edit_major_op;
		info->obj->o_major_align = *(state_elmt->con_feature);
		info->obj->o_major_align.f_status = TRUE;
		free(state_elmt->con_feature);
		Constraint_Manipulate_Specs(&(info->obj->o_major_align), info->obj,
									NULL, 0, Add_Dependencies);
		Edit_Set_Major_Align_Label(TRUE);
	}
	else
	{
		state_elmt->edit_op = edit_minor_op;
		info->obj->o_minor_align = *(state_elmt->con_feature);
		info->obj->o_major_align.f_status = TRUE;
		free(state_elmt->con_feature);
		Constraint_Manipulate_Specs(&(info->obj->o_minor_align), info->obj,
									NULL, 0, Add_Dependencies);
		Edit_Set_Minor_Align_Label(TRUE);
	}

	Edit_Force_Rotation_Constraints(info, TRUE);
}


static void
Free_Stack_Elmt(EditStatePtr elmt)
{
	if ( elmt->edit_op == edit_remove_op )
		free(elmt->con_feature);
	free(elmt);
}


/*	void
**	Edit_Undo_Clear()
**	Frees the undo and redo stacks.
*/
void
Edit_Undo_Clear()
{
	EditStatePtr temp;

	while ( undo_stack )
	{
		temp = undo_stack;
		undo_stack = undo_stack->next;
		Free_Stack_Elmt(temp);
	}

	while ( redo_stack )
	{
		temp = redo_stack;
		redo_stack = redo_stack->next;
		Free_Stack_Elmt(temp);
	}
}

