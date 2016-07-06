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
**	maintain.c: constraint maintenance functions.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <instance_list.h>
#include <X11/Xaw/Toggle.h>

void	Edit_Update_Object_Dependents(ObjectInstancePtr obj);

static void	Maintain_Align_Major(ObjectInstancePtr);
static void	Maintain_Align_Minor(ObjectInstancePtr);

Boolean	do_maintenance = TRUE;

static FeatureData	m_origin_defaults[3];
static FeatureData	m_scale_defaults[3];

static FeaturePtr 	*options;
static int			max_num_options = 0;

InstanceList	topological_list = NULL;

#define Check_Option_Num(num) \
	if ( num + 3 > max_num_options ) \
	{ \
		if ( max_num_options ) \
		{ \
			max_num_options += 10; \
			options = More(options, FeaturePtr, max_num_options); \
		} \
		else \
		{ \
			max_num_options = 10; \
			options = New(FeaturePtr, 10); \
		} \
	}

void
Edit_Maintain_All_Constraints(EditInfoPtr info)
{
	InstanceList	temp;

	if ( ! topological_list )
		DFS(info->obj, NULL, 0, TRUE, &topological_list);

	for ( temp = topological_list->next ; temp ; temp = temp->next )
		Edit_Update_Object_Dependents(temp->the_instance);

	View_Update(info->window, topological_list, CalcView );
}


void
Edit_Maintain_Free_List()
{
	InstanceList	elmt;

	for ( elmt = topological_list ; elmt ; elmt = elmt->next )
		elmt->the_instance->o_flags &= ( ObjAll ^ ObjDepends );
	Free_Selection_List(topological_list);
	topological_list = NULL;
}


void
Edit_Update_Object_Dependents(ObjectInstancePtr obj)
{
	FeatureData		resulting;
	Transformation	transform;
	Vector			origin;
	Vector			reference;
	Boolean			changed = FALSE;
	Boolean			new_cons;
	int				i;

	/* Update all the origin constraints. */

	debug(FUNC_NAME,fprintf(stderr,"Edit_Update_Object_Dependents()\n"));

	Check_Option_Num(obj->o_origin_num)

	options[0] = m_origin_defaults;
	options[1] = m_origin_defaults + 1;
	options[2] = m_origin_defaults + 2;

	for ( i = 0 ; i < obj->o_origin_num ; i++ )
		options[ i + 3 ] = obj->o_origin_cons + i;
	for ( i = 0 ; i < obj->o_origin_num + 3 ; i++ )
		options[i]->f_status = obj->o_origin_active[i];

	VAdd(obj->o_origin, obj->o_transform.displacement, origin);

	new_cons = FALSE;
	Edit_Set_Origin_Defaults(options, origin);
	for ( i = 0 ; i < obj->o_origin_num ; i++ )
		new_cons = Edit_Update_Feature_Specs(options[i+3],
					obj->o_transform.displacement,
					obj->o_reference, obj->o_origin) || new_cons;

	/* Resolve for them. */
	if ( new_cons )
	{
		Constraint_Solve_System(options, obj->o_origin_num + 3, &resulting);

		if ( ! Point_Satisfies_Constraint(origin, &resulting) )
		{
			NewIdentityMatrix(transform.matrix);
			transform.displacement = Find_Required_Motion(origin, &resulting);
			Transform_Instance(obj, &transform, FALSE);
			VAdd(obj->o_origin, obj->o_transform.displacement, origin);
			changed = TRUE;
		}
	}

	/* Redo the alignment. Do it before scaling because scaling depends
	** on alignment.
	*/
	if ( obj->o_major_align.f_type != null_feature &&
		 ( Edit_Update_Feature_Specs(&(obj->o_major_align),
									 obj->o_transform.displacement,
									 obj->o_reference, obj->o_origin) ||
		   changed ) )
	{
		Maintain_Align_Major(obj);
		changed = TRUE;
	}
	if ( obj->o_minor_align.f_type != null_feature &&
		 ( Edit_Update_Feature_Specs(&(obj->o_minor_align),
									 obj->o_transform.displacement,
									 obj->o_reference, obj->o_origin) ||
		   changed ) )
	{
		Maintain_Align_Minor(obj);
		changed = TRUE;
	}


	/* Now do scaling. */

	Check_Option_Num(obj->o_scale_num)

	options[0] = m_scale_defaults;
	options[1] = m_scale_defaults + 1;
	options[2] = m_scale_defaults + 2;

	for ( i = 0 ; i < obj->o_scale_num ; i++ )
		options[ i + 3 ] = obj->o_scale_cons + i;
	for ( i = 0 ; i < obj->o_scale_num + 3 ; i++ )
		options[i]->f_status = obj->o_scale_active[i];

	VAdd(obj->o_reference, obj->o_transform.displacement, reference);

	Edit_Set_Scale_Defaults(options, reference, obj->o_axes);
	new_cons = FALSE;
	for ( i = 0 ; i < obj->o_scale_num ; i++ )
		new_cons = Edit_Update_Feature_Specs(options[i+3],
					obj->o_transform.displacement,
					obj->o_reference, obj->o_origin) || new_cons;

	if ( new_cons || changed )
	{
		Vector	orig_ref;
		Vector	disp;

		VSub(reference, origin, disp);
		MVMul(obj->o_axes, disp, orig_ref);

		/* Do any forcing that's necessary. */
		options[0]->f_status = options[0]->f_status || IsZero(orig_ref.z);
		options[1]->f_status = options[1]->f_status || IsZero(orig_ref.y);
		options[2]->f_status = options[2]->f_status || IsZero(orig_ref.x);

		/* Resolve for the scaling resultant. */
		Constraint_Solve_System(options, obj->o_scale_num + 3, &resulting);

		if ( ! Point_Satisfies_Constraint(reference, &resulting) )
		{
			Matrix	axes_inverse = MInvert(&(obj->o_axes));

			disp = Find_Required_Motion(reference, &resulting);
			VAdd(disp, reference, reference);

			Scale_Calculate_Transform(&transform, reference, orig_ref,
									  obj->o_origin, origin,
									  &(obj->o_axes), &axes_inverse, FALSE, 
										&(obj->o_scale));

			Transform_Instance(obj, &transform, FALSE);

			VSub(origin, obj->o_transform.displacement, obj->o_origin);
			VSub(reference, obj->o_transform.displacement, obj->o_reference);
			for ( i = 0 ; i < obj->o_scale_num ; i++ )
				Edit_Update_Feature_Specs(options[i+3],
									  obj->o_transform.displacement,
									  obj->o_reference, obj->o_origin);
			changed = TRUE;
		}
	}

	/* Finally, update rotation constraints. */
	for ( i = 0 ; i < obj->o_rotate_num ; i++ )
		Edit_Update_Feature_Specs(obj->o_rotate_cons + i,
								  obj->o_transform.displacement,
								  obj->o_reference, obj->o_origin);

}


static void
Maintain_Align(ObjectInstancePtr obj, Quaternion rot_quat)
{
	Transformation	rot_trans;
	Vector	org_to_cent;
	Vector	origin;
	Vector	new_center;
	Vector	temp_v;
	Matrix	transp;

	/* Work out the matrix. */
	rot_trans.matrix = Quaternion_To_Matrix(rot_quat);

	VAdd(obj->o_origin, obj->o_transform.displacement, origin);

	/* Rotate the center to find the displacement. */
	VScalarMul(obj->o_origin, -1, org_to_cent);
	MVMul(rot_trans.matrix, org_to_cent, new_center);
	VAdd(origin, new_center, new_center);
	VSub(new_center, obj->o_transform.displacement, rot_trans.displacement);

	Transform_Instance(obj, &rot_trans, FALSE);

	VSub(origin, obj->o_transform.displacement, obj->o_origin);
	MVMul(rot_trans.matrix, obj->o_reference, temp_v);
	obj->o_reference = temp_v;

	/* Rotate the axes. */
	MTrans(rot_trans.matrix, transp);
	obj->o_axes = MMMul(&(obj->o_axes), &transp);

	/* and finally, we save the quaternion for slerp */
	obj->o_rot=QMul(obj->o_rot,rot_quat);
}


static void
Maintain_Align_Major(ObjectInstancePtr obj)
{
	Vector	axis, unit_axis;
	double	angle;
	double	cos_angle;
	double	sin_half_angle;
	double	cos_half_angle;
	double	temp_d;
	Quaternion	rot_quat;

	/* Build a quaternion for the rotation. */
	VCross(obj->o_major_align.f_vector, obj->o_axes.x, axis);

	if ( VZero(axis) ) return;	/* Already aligned. */

	VUnit(axis, temp_d, unit_axis);
	cos_angle = VDot(obj->o_major_align.f_vector, obj->o_axes.x);

	/* The quaternion requires half angles. */
	if ( cos_angle > 1.0 ) cos_angle = 1.0;
	angle = acos(cos_angle);
	sin_half_angle = sin(angle / 2);
	cos_half_angle = cos(angle / 2);

	VScalarMul(unit_axis, sin_half_angle, rot_quat.vect_part);
	rot_quat.real_part = cos_half_angle;

	Maintain_Align(obj, rot_quat);

	Edit_Update_Feature_Specs(&(obj->o_major_align),
							  obj->o_transform.displacement,
							  obj->o_reference, obj->o_origin);
}


static void
Maintain_Align_Minor(ObjectInstancePtr obj)
{
	Vector	new_other, unit_new, axis;
	double	angle;
	double	sin_half_angle;
	double	cos_angle, cos_half_angle;
	double	temp_d;
	Quaternion	rot_quat;

	VCross(obj->o_axes.x, obj->o_minor_align.f_vector, new_other);

	if ( VZero(new_other) ) return;	/* Can't do it. */

	VUnit(new_other, temp_d, unit_new);

	VCross(unit_new, obj->o_axes.z, axis);
	if ( VZero(axis) ) return;	/* Already aligned. */
	VUnit(axis, temp_d, axis);
	cos_angle = VDot(unit_new, obj->o_axes.z);

	if ( cos_angle > 1.0 ) cos_angle = 1.0;
	angle = acos(cos_angle);

	/* The quaternion requires half angles. */
	sin_half_angle = sin(angle / 2.0);
	cos_half_angle = cos(angle / 2.0);

	VScalarMul(axis, sin_half_angle, rot_quat.vect_part);
	rot_quat.real_part = cos_half_angle;

	Maintain_Align(obj, rot_quat);

	Edit_Update_Feature_Specs(&(obj->o_major_align),
							  obj->o_transform.displacement,
							  obj->o_reference, obj->o_origin);
	Edit_Update_Feature_Specs(&(obj->o_minor_align),
							  obj->o_transform.displacement,
							  obj->o_reference, obj->o_origin);
}


void
Maintain_Toggle_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtVaGetValues(w, XtNstate, &do_maintenance, NULL);

	if ( do_maintenance &&
		 ( ( main_window.current_state & edit_object ) ||
		   ( csg_window.current_state & edit_object ) ) )
		Edit_Maintain_All_Constraints(Edit_Get_Info());
}


