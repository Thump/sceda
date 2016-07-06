#define PATCHLEVEL 0
/*
**    ScEd: A Constraint Based Scene Editor
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
**	align.c : Functions for axis alignment operations.
*/


#include <math.h>
#include <sced.h>
#include <edit.h>
#include <select_point.h>
#include <X11/Xaw/Toggle.h>


static long			which_axis;
static EditInfoPtr	info;

Boolean	rotate_forced[2];

static void
Add_Do_Alignment(FeaturePtr feat)
{
	Edit_Cleanup_Selection(TRUE);

	if ( which_axis == MAJOR_AXIS )
	{
		feat->f_status = TRUE;
		feat->f_label = Strdup("Major Align");
		Edit_Undo_Register_State(edit_major_op, ROTATE, 0);
		Edit_Major_Align(feat->f_vector, TRUE);
		Edit_Set_Major_Align_Label(TRUE);
	}
	else
	{
		feat->f_status = TRUE;
		feat->f_label = Strdup("Minor Align");
		Edit_Undo_Register_State(edit_minor_op, ROTATE, 0);
		Edit_Minor_Align(feat->f_vector, TRUE);
		Edit_Set_Minor_Align_Label(TRUE);
	}

	Edit_Force_Rotation_Constraints(info, FALSE);
}


static void
Add_Point_Alignment(int *index, FeatureSpecType *spec)
{
	FeaturePtr	feat;
	Vector	endpt;
	double	temp_d;

	if ( which_axis == MAJOR_AXIS )
		feat = &(info->obj->o_major_align);
	else
		feat = &(info->obj->o_minor_align);

	if ( feat->f_type != null_feature )
		Constraint_Manipulate_Specs(feat, info->obj, NULL, 1,
									Edit_Remove_Obj_From_Dependencies);

	feat->f_type = line_feature;
	feat->f_spec_type = axis_feature;

	Add_Spec(feat->f_specs, info->origin,
			 info->obj->o_world_verts[info->obj->o_num_vertices - 1],
			 origin_spec, info->obj, 0);

	endpt =select_verts[*index].obj->o_world_verts[select_verts[*index].offset];
	Add_Spec(feat->f_specs + 1, endpt,
			 info->obj->o_world_verts[info->obj->o_num_vertices - 1], *spec,
			 select_verts[*index].obj, select_verts[*index].offset);

	VSub(endpt, info->origin, feat->f_vector);

	if ( VZero(feat->f_vector) )
	{
		feat->f_type = null_feature;
		feat->f_spec_type = null_feature;
		XBell(XtDisplay(main_window.shell), 0);
		return;
	}

	VUnit(feat->f_vector, temp_d, feat->f_vector);
	feat->f_point = info->origin;

	Add_Do_Alignment(feat);
}


static void
Add_Line_Alignment(int *indices, FeatureSpecType *specs)
{
	Vector		endpoints[2];
	FeaturePtr	feat;
	double		temp_d;
	int			i;

	if ( which_axis == MAJOR_AXIS )
		feat = &(info->obj->o_major_align);
	else
		feat = &(info->obj->o_minor_align);

	if ( feat->f_type != null_feature )
		Constraint_Manipulate_Specs(feat, info->obj, NULL, 1,
									Edit_Remove_Obj_From_Dependencies);

	feat->f_type = line_feature;
	feat->f_spec_type = axis_feature;
	for ( i = 0 ; i < 2 ; i ++ )
	{
		endpoints[i] = select_verts[indices[i]].
						obj->o_world_verts[select_verts[indices[i]].offset];
		Add_Spec(feat->f_specs + i, endpoints[i],
			info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[i],
			select_verts[indices[i]].obj, select_verts[indices[i]].offset);
	}

	VSub(endpoints[1], endpoints[0], feat->f_vector);
	VUnit(feat->f_vector, temp_d, feat->f_vector);
	feat->f_point = endpoints[0];

	Add_Do_Alignment(feat);
}


static void
Add_Plane_Alignment(int *indices, FeatureSpecType *specs)
{
	Vector		endpoints[3];
	FeaturePtr	feat;
	Vector		temp_v1, temp_v2;
	double		temp_d;
	int			i;

	if ( which_axis == MAJOR_AXIS )
		feat = &(info->obj->o_major_align);
	else
		feat = &(info->obj->o_minor_align);

	if ( feat->f_type != null_feature )
		Constraint_Manipulate_Specs(feat, info->obj, NULL, 1,
									Edit_Remove_Obj_From_Dependencies);

	feat->f_type = line_feature;
	feat->f_spec_type = axis_plane_feature;
	feat->f_status = FALSE;
	for ( i = 0 ; i < 3 ; i++ )
	{
		endpoints[i] = select_verts[indices[i]].
						obj->o_world_verts[select_verts[indices[i]].offset];
		Add_Spec(feat->f_specs + i, endpoints[i],
			info->obj->o_world_verts[info->obj->o_num_vertices - 1], specs[i],
			select_verts[indices[i]].obj, select_verts[indices[i]].offset);
	}

	VSub(endpoints[1], endpoints[0], temp_v1);
	VSub(endpoints[2], endpoints[0], temp_v2);
	VCross(temp_v1, temp_v2, feat->f_vector);
	VUnit(feat->f_vector, temp_d, feat->f_vector);
	feat->f_point = endpoints[0];

	Add_Do_Alignment(feat);
}


static void
Edit_Align_Prelude(long axis)
{
	info = Edit_Get_Info();

	which_axis = axis;

	Register_Select_Operation();

	/* Prepare for point selection. */
	info->selecting = TRUE;

	Edit_Sensitize_Buttons(FALSE, FALSE);

	select_window = info->window;
	select_highlight = TRUE;
}


static void
Edit_Align_Epilogue()
{
	Build_Select_Verts_From_List(info->other_available);
	specs_allowed[reference_spec] =
	specs_allowed[absolute_spec] = TRUE;
	specs_allowed[offset_spec] = FALSE;

	allow_text_entry = TRUE;
	if ( num_select_verts < num_verts_required ) return;

	/* Draw all the reference points. */
	Edit_Draw_Selection_Points(info);
}


void
Edit_Align_With_Point(Widget w, XtPointer cl, XtPointer ca)
{
	Edit_Align_Prelude((long)cl);
	num_verts_required = 1;
	select_finished_callback = Add_Point_Alignment;
	Edit_Align_Epilogue();
}


void
Edit_Align_With_Line(Widget w, XtPointer cl, XtPointer ca)
{
	Edit_Align_Prelude((long)cl);
	num_verts_required = 2;
	select_finished_callback = Add_Line_Alignment;
	Edit_Align_Epilogue();
}


void
Edit_Align_With_Plane(Widget w, XtPointer cl, XtPointer ca)
{
	Edit_Align_Prelude((long)cl);
	num_verts_required = 3;
	select_finished_callback = Add_Plane_Alignment;
	Edit_Align_Epilogue();
}


void
Edit_Remove_Alignment(Widget w, XtPointer cl, XtPointer ca)
{
	info = Edit_Get_Info();

	if ( (long)cl == MAJOR_AXIS )
	{
		Edit_Undo_Register_State(edit_major_remove_op, ROTATE, 0);
		info->obj->o_major_align.f_type = null_feature;
		info->obj->o_major_align.f_spec_type = null_feature;
		Edit_Set_Major_Align_Label(FALSE);
	}
	else
	{
		Edit_Undo_Register_State(edit_minor_remove_op, ROTATE, 0);
		info->obj->o_minor_align.f_type = null_feature;
		info->obj->o_minor_align.f_spec_type = null_feature;
		Edit_Set_Minor_Align_Label(FALSE);
	}

	Edit_Force_Rotation_Constraints(info, FALSE);
}

/* I added the save parameter to this function: used to save the computed
** quaternion so that we can slerp it when animating.
*/
Matrix
Major_Align_Matrix(Vector new_dir, Matrix *old_axes, Quaternion *save)
{
	Vector	axis, unit_axis;
	double	angle;
	double	cos_angle;
	double	sin_half_angle;
	double	cos_half_angle;
	double	temp_d;
	Quaternion		rot_quat;
	Matrix	identity;
	Vector	diff;

	NewIdentityMatrix(identity);

	/* Build a quaternion for the rotation. */
	VCross(new_dir, old_axes->x, axis);

	if ( VZero(axis) )
	{
		if ( VEqual(new_dir, old_axes->x, diff) )
			return identity;	/* Already aligned. */
		else
			axis = old_axes->y;
	}

	VUnit(axis, temp_d, unit_axis);
	cos_angle = VDot(new_dir, old_axes->x);

	/* The quaternion requires half angles. */
	if ( cos_angle > 1.0 ) cos_angle = 1.0;
	if ( cos_angle < -1.0 ) cos_angle = -1.0;
	angle = acos(cos_angle);
	sin_half_angle = sin(angle / 2);
	cos_half_angle = cos(angle / 2);

	VScalarMul(unit_axis, sin_half_angle, rot_quat.vect_part);
	rot_quat.real_part = cos_half_angle;

	/* We save the value to use in Edit_Finish_Rotate_Drag() (for slerp) */
	*save=QMul(*save,rot_quat);

	return Quaternion_To_Matrix(rot_quat);
}


/* I added the save parameter to this function: used to save the computed
** quaternion so that we can slerp it when animating.
*/
Matrix
Minor_Align_Matrix(Vector new_dir, Matrix *old_axes, Quaternion *save)
{
	Vector	new_other, unit_new, axis;
	double	angle;
	double	sin_half_angle;
	double	cos_angle, cos_half_angle;
	double	temp_d;
	Quaternion		rot_quat;
	Matrix	identity;
	Vector	diff;

	NewIdentityMatrix(identity);

	/* Work out where the new other axis is. */
	VCross(old_axes->x, new_dir, new_other);

	if ( VZero(new_other) )
		return identity;

	VUnit(new_other, temp_d, unit_new);

	/* Hmmm, doesn't seem to me that this line is correct: won't this
	** give us the axis of rotation of the z axis?
	**
	** No, talked with Steve about this: he said it's right the way 
	** it was.
	*/
	VCross(unit_new, old_axes->z, axis);
	/* VCross(new_dir, old_axes->y, axis); */

	if ( VZero(axis) )
	{
		if ( VEqual(unit_new, old_axes->z, diff) )
			return identity;	/* Already aligned. */
		else
			axis = old_axes->x;
	}
	VUnit(axis, temp_d, axis);

	/* Similarly here: won't this give us the angle of rotation of the
	** the z axis?
	**
	** And here, too...
	*/
	cos_angle = VDot(unit_new, old_axes->z);
	/* cos_angle = VDot(new_dir, old_axes->y); */

	if ( cos_angle > 1.0 ) cos_angle = 1.0;
	if ( cos_angle < -1.0 ) cos_angle = -1.0;
	angle = acos(cos_angle);

	/* The quaternion requires half angles. */
	sin_half_angle = sin(angle / 2.0);
	cos_half_angle = cos(angle / 2.0);

	VScalarMul(axis, sin_half_angle, rot_quat.vect_part);
	rot_quat.real_part = cos_half_angle;

	/* We save the value to use in Edit_Finish_Rotate_Drag() (for slerp) */
	*save=QMul(*save,rot_quat);

	return Quaternion_To_Matrix(rot_quat);
}



/*	void
**	Edit_Force_Alignment_Satisfaction(EditInfoPtr align_info)
**	Updates any origin dependent constraints, and checks and modifies
**	the axis alignment.
*/
void
Edit_Force_Alignment_Satisfaction(EditInfoPtr align_info)
{
	if ( align_info->obj->o_major_align.f_spec_type == axis_feature &&
		 align_info->obj->o_major_align.f_specs[0].spec_type == origin_spec )
	{
		Edit_Update_Feature_Specs(&(align_info->obj->o_major_align),
			align_info->obj->o_world_verts[align_info->obj->o_num_vertices-1],
			align_info->obj->o_reference,
			align_info->obj->o_origin);
		Edit_Major_Align(align_info->obj->o_major_align.f_vector, TRUE);
	}

	if ( align_info->obj->o_minor_align.f_spec_type == axis_feature &&
		 align_info->obj->o_minor_align.f_specs[0].spec_type == origin_spec )
	{
		Edit_Update_Feature_Specs(&(align_info->obj->o_minor_align),
			align_info->obj->o_world_verts[align_info->obj->o_num_vertices-1],
			align_info->obj->o_reference,
			align_info->obj->o_origin);
		Edit_Minor_Align(align_info->obj->o_minor_align.f_vector, TRUE);
	}
}


/*	Boolean
**	Edit_Dynamic_Align(EditInfoPtr align_info)
**	Updates and checks alignment constraints in the middle of a drag op.
**	Assumes it is called with updated info->reference and origin points, and
**	the current center of the body.
**	It does not change anything permanent itself, just returns the transform
**	needed to bring about alignment.
*/
Boolean
Edit_Dynamic_Align(EditInfoPtr align_info, Transformation *result, Vector
					new_center)
{
	Vector	new_ref;
	Vector	new_org;
	Vector	start_center;
	Matrix	new_axes;
	Matrix	major_matrix;
	Matrix	minor_matrix;
	Vector	to_center;
	Vector	temp_v;
	Matrix	transp;
	Boolean	did_major = FALSE;
	Boolean	did_minor = FALSE;

	new_axes = align_info->axes;
	start_center = new_center;
	VSub(align_info->reference, start_center, new_ref);
	VSub(align_info->origin, start_center, new_org);

	if ( align_info->obj->o_major_align.f_spec_type == axis_feature &&
		 align_info->obj->o_major_align.f_specs[0].spec_type == origin_spec &&
		 Edit_Update_Feature_Specs(&(align_info->obj->o_major_align),
								   start_center, new_ref, new_org) )
	{
		if ( VZero(align_info->obj->o_major_align.f_vector) )
			NewIdentityMatrix(major_matrix)
		else
			major_matrix =
				Major_Align_Matrix(align_info->obj->o_major_align.f_vector,
									&(align_info->axes),&info->rotation);

		/* Transform the key points. */
		/* The center first. */
		VSub(new_center, align_info->origin, to_center);
		MVMul(major_matrix, to_center, temp_v);
		VAdd(temp_v, align_info->origin, new_center);
		VSub(align_info->origin, new_center, new_org);

		/* The reference point. */
		MVMul(major_matrix, new_ref, temp_v);
		new_ref = temp_v;

		/* The axes. */
		MTrans(major_matrix, transp);
		new_axes = MMMul(&(align_info->axes), &transp);

		did_major = TRUE;
	}

	if ( align_info->obj->o_minor_align.f_spec_type == axis_feature &&
		 align_info->obj->o_minor_align.f_specs[0].spec_type == origin_spec &&
		 Edit_Update_Feature_Specs(&(align_info->obj->o_minor_align),
								   new_center, new_ref, new_org) )
	{
		if ( VZero(align_info->obj->o_minor_align.f_vector) )
			NewIdentityMatrix(minor_matrix)
		else
		{	minor_matrix =
				Minor_Align_Matrix(align_info->obj->o_minor_align.f_vector,
									&(new_axes), &info->rotation);
		}

		/* Transform the key points. */
		/* The center first. */
		VSub(new_center, align_info->origin, to_center);
		MVMul(minor_matrix, to_center, temp_v);
		VAdd(temp_v, align_info->origin, new_center);
		VSub(align_info->origin, new_center, new_org);

		/* The reference point. */
		MVMul(minor_matrix, new_ref, temp_v);
		new_ref = temp_v;

		/* The axes. */
		MTrans(minor_matrix, transp);
		new_axes = MMMul(&new_axes, &transp);

		did_minor = TRUE;
	}

	if ( did_major )
		result->matrix = major_matrix;
	else
		NewIdentityMatrix(result->matrix);
	if ( did_minor )
	{
		result->matrix = MMMul(&minor_matrix, &(result->matrix));
	}
	if ( did_major || did_minor )
		VSub(new_center, start_center, result->displacement);

	return (did_minor || did_major);
}


/*	void
**	Edit_Force_Rotation_Constraints(EditInfoPtr info)
**	Based on the status of the alignment constraints, forces one, two
**	or no rotation axes.
*/
void
Edit_Force_Rotation_Constraints(EditInfoPtr info, Boolean ignore_old)
{
	Boolean	old[2];

	old[0] = rotate_forced[0];
	old[1] = rotate_forced[1];

	rotate_forced[0] = ( info->obj->o_major_align.f_type == line_feature );
	rotate_forced[1] = ( info->obj->o_minor_align.f_type == line_feature );

	if ( ignore_old || old[0] != rotate_forced[0] )
		Edit_Select_Constraint(info, 0, &rotate_constraints, rotate_forced[0],
							   TRUE, TRUE);
	if ( ignore_old || old[1] != rotate_forced[1] )
		Edit_Select_Constraint(info, 1, &rotate_constraints, rotate_forced[1],
							   TRUE, TRUE);
}

