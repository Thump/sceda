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
**	rotate.c : Functions to perform object rotation.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>


static void	Rotate_Object(EditInfoPtr);
static Vector	Sphere_Point(EditInfoPtr, int, int);

static Matrix	previous_rot;
static Vector	start_center;
static Transformation	init_obj_transform;

static Vector	constraint_axis;

void
Edit_Start_Rotate_Drag(XEvent *e, EditInfoPtr info)
{
	if ( info->rotate_resulting.f_type != line_feature &&
		 info->rotate_resulting.f_type != null_feature )	return;

	if ( ! do_maintenance )
	{
		Edit_Draw(info->window, ViewNone, info, FALSE);
		Edit_Draw(info->window, RemoveHidden, info, TRUE);
	}

	NewIdentityMatrix(info->drag_transform.matrix);
	VNew(0, 0, 0, info->drag_transform.displacement);
	NewIdentityMatrix(previous_rot);
	start_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];

	if ( info->rotate_resulting.f_type == line_feature )
	{
		Matrix	inverse_trans;

		/* Get the axis in view.  It's the same as transforming a normal. */
		MTrans(info->window->viewport.view_to_world.matrix, inverse_trans);
		MVMul(inverse_trans, info->rotate_resulting.f_vector, constraint_axis);
	}

	info->drag_start = Sphere_Point(info, e->xbutton.x, e->xbutton.y);
	init_obj_transform = info->obj->o_transform;

	Edit_Undo_Register_State(edit_drag_op, ROTATE, 0);

	info->drag_type = ROTATE_DRAG;
}


void
Edit_Continue_Rotate_Drag(XEvent *e, EditInfoPtr info)
{
	Vector	current_pt;
	Quaternion	rot_quat;
	Matrix	prev_inverse;
	Matrix	new_rot;
	Vector	temp_v;
	double	rot_angle;
	Vector	rot_axis;
	double	sin_angle;

	current_pt = Sphere_Point(info, e->xbutton.x, e->xbutton.y);

	/* Find the current rotation quaternion. */
	VCross(current_pt, info->drag_start, rot_quat.vect_part);
	rot_quat.real_part = VDot(info->drag_start, current_pt);

	/* Save the working rotation quaternion. */
	info->workrot=rot_quat;
	debug(ROT_RT,Print_Quaternion(&info->workrot));

	rot_angle = acos(rot_quat.real_part);
	if ( ! IsZero(rot_angle) )
	{
		sin_angle = 1 / sin(rot_angle);
		VScalarMul(rot_quat.vect_part, sin_angle, rot_axis);
		rot_angle *= 360 / M_PI;
	}

	debug(ROT_RT,fprintf(stderr,"angle: %f axis: ",rot_angle));
	debug(ROT_RT,Print_Vector(&rot_axis));
	debug(ROT_RT,fprintf(stderr,"\n"));

	Edit_Set_Drag_Label(ROTATE_DRAG, rot_axis, rot_angle);

	prev_inverse = MInvert(&previous_rot);
	previous_rot = Quaternion_To_Matrix(rot_quat);
	new_rot = MMMul(&previous_rot, &prev_inverse);
	info->drag_transform.matrix =
		MMMul(&new_rot, &(info->drag_transform.matrix));

	/* Rotate the reference point. */
	MVMul(new_rot, info->obj->o_reference, temp_v);
	info->obj->o_reference = temp_v;

	Rotate_Object(info);

	if ( do_maintenance )
		Edit_Maintain_All_Constraints(info);
}



void
Edit_Finish_Rotate_Drag(XEvent *e, EditInfoPtr info)
{	Matrix x,y,z;
	double tmp;

	/* Rework the available constraints. */
	Edit_Update_Constraints(info);

	Apply_Transform(init_obj_transform, info->drag_transform,
					info->obj->o_transform);

	info->obj->o_axes = info->axes;
	info->axes_inverse = MInvert(&(info->axes));

	/* save the rotation as a (normalized) value and zero the work area */
	info->rotation=QMul(info->rotation,info->workrot);

	if ( ! do_maintenance )
	{
		/* Redraw the constraints. */
		Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Origin_Constraints(info->window, CalcView, info, FALSE);
		Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Scale_Constraints(info->window, CalcView, info, FALSE);
		Edit_Draw(info->window, RemoveHidden, info, FALSE);
		Edit_Draw(info->window, ViewNone, info, TRUE);
	}

	Edit_Set_Drag_Label(NO_DRAG, info->obj->o_transform.displacement, 0);
}


static void
Rotate_Object(EditInfoPtr info)
{
	Matrix			transp;
	Vector			to_center;
	Vector			new_center;

	if ( ! do_maintenance )
	{
		Edit_Draw(info->window, RemoveHidden, info, FALSE);
		Draw_Edit_Extras(info->window, ViewNone, info, FALSE);
		/* Undraw the constraints. */
		Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);
	}

	/* Need to know how far it's moved.  So rotate the center about the
	** fixed point and see where it ends up.
	*/
	VSub(start_center, info->origin, to_center);
	MVMul(info->drag_transform.matrix, to_center, new_center);
	VAdd(new_center, info->origin, new_center);
	VSub(new_center, start_center, info->drag_transform.displacement);

	/* Rotate the body axes. */
	MTrans(previous_rot, transp);
	info->axes = MMMul(&(info->obj->o_axes), &transp);
	info->axes_inverse = MInvert(&(info->axes));

	/* Work out the reference point. */
	VAdd(info->obj->o_reference, new_center, info->reference);

	/* Rework the origin location. */
	VSub(info->origin, new_center, info->obj->o_origin);

	/* It needs to be transformed to get the scaling constraints right. */
	Apply_Transform(init_obj_transform, info->drag_transform,
					info->obj->o_transform);


	/* Update the constraints. */
	Edit_Set_Scale_Defaults(scale_constraints.options, info->reference,
							info->axes);
	Edit_Update_Active_Object_Cons(&scale_constraints, info, new_center);
	Constraint_Solve_System(scale_constraints.options,
							scale_constraints.num_options,
							&(info->reference_resulting));

	Edit_Dynamic_Scale(info, new_center);

	/* It needs to be transformed. */
	Apply_Transform(init_obj_transform, info->drag_transform,
					info->obj->o_transform);


	Edit_Transform_Vertices(&(info->obj->o_transform), info);
	Edit_Transform_Normals(&(info->obj->o_transform), info);

	/* Update the origin and reference point. */
	VSub(info->origin, info->obj->o_world_verts[info->obj->o_num_vertices-1],
		 info->obj->o_origin);
	VSub(info->reference, info->obj->o_world_verts[info->obj->o_num_vertices-1],
		 info->obj->o_reference);

	Edit_Update_Active_Object_Cons(&origin_constraints, info,
					info->obj->o_world_verts[info->obj->o_num_vertices - 1]);
	Constraint_Solve_System(origin_constraints.options,
							origin_constraints.num_options,
							&(info->origin_resulting));

	Edit_Set_Rotate_Defaults(rotate_constraints.options, info->axes);
	Edit_Update_Active_Object_Cons(&rotate_constraints, info,
					info->obj->o_world_verts[info->obj->o_num_vertices - 1]);
	Constraint_Solve_System(rotate_constraints.options,
							rotate_constraints.num_options,
							&(info->rotate_resulting));

	if ( ! do_maintenance )
	{
		Draw_Origin_Constraints(info->window, CalcView, info, FALSE);
		Draw_Scale_Constraints(info->window, CalcView, info, FALSE);
		Draw_Rotate_Constraints(info->window, CalcView, info, FALSE);

		Edit_Draw(info->window, CalcView | RemoveHidden, info, FALSE);
		Draw_Edit_Extras(info->window, CalcView, info, TRUE);
	}
}



/*	Matrix
**	Quaternion_To_Matrix(Quaternion q)
**	Converts a quaternion to a rotation matrix.
*/
Matrix
Quaternion_To_Matrix(Quaternion q)
{
	Matrix	result;
	double	xx, yy, zz;
	double	xy, xz, yz;
	double	wx, wy, wz;
	Vector	v1, v2, v3;

	xx = q.vect_part.x * q.vect_part.x;
	yy = q.vect_part.y * q.vect_part.y;
	zz = q.vect_part.z * q.vect_part.z;

	xy = q.vect_part.x * q.vect_part.y;
	xz = q.vect_part.x * q.vect_part.z;
	yz = q.vect_part.y * q.vect_part.z;

	wx = q.real_part * q.vect_part.x;
	wy = q.real_part * q.vect_part.y;
	wz = q.real_part * q.vect_part.z;

	VNew(1 - 2 * yy - 2 * zz, 2 * xy + 2 * wz, 2 * xz - 2 * wy, v1);
	VNew(2 * xy - 2 * wz, 1 - 2 * xx - 2 * zz, 2 * yz + 2 * wx, v2);
	VNew(2 * xz + 2 * wy, 2 * yz - 2 * wx, 1 - 2 * xx - 2 * yy, v3);
	MNew(v1, v2, v3, result);

	return result;
}


static Vector
Sphere_Point(EditInfoPtr info, int x, int y)
{
	Vector	sphere_pt;
	Vector	res;
	double	pt_rad;
	double	s;
	Vector	temp_v;
	Vector	projection;
	double	norm;

	sphere_pt.x = ( x - info->origin_vert.screen.x ) / (double)info->radius;
	sphere_pt.y = ( info->origin_vert.screen.y - y ) / (double)info->radius;
	pt_rad = sphere_pt.x * sphere_pt.x + sphere_pt.y * sphere_pt.y;
	if ( pt_rad > 1.0 )
	{
		/* Bring it back to the edge. */
		s = 1.0 / sqrt(pt_rad);
		sphere_pt.x *= s;
		sphere_pt.y *= s;
		sphere_pt.z = 0.0;
	}
	else
		sphere_pt.z = -sqrt(1.0 - pt_rad);

	if ( info->rotate_resulting.f_type == null_feature )
	{
		MVMul(info->window->viewport.view_to_world.matrix, sphere_pt, res);
		return res;
	}

	s = VDot(sphere_pt, constraint_axis);
	VScalarMul(constraint_axis, s, temp_v);
	VSub(sphere_pt, temp_v, projection);

	norm = VMod(projection);

	if ( norm > 0 )
	{
		s = 1.0 / norm;
		if ( projection.z > 0 ) s = -s;
		VScalarMul(projection, s, sphere_pt);
	}
	else if ( constraint_axis.z == 1.0 )
		VNew(1, 0, 0, sphere_pt);
	else
	{
		VNew(-constraint_axis.y, constraint_axis.x, 0, temp_v);
		VUnit(temp_v, norm, sphere_pt);
	}

	MVMul(info->window->viewport.view_to_world.matrix, sphere_pt, res);

	return res;
}


void
Edit_Major_Align(Vector direction, Boolean aligning)
{
	EditInfoPtr	info = Edit_Get_Info();
	Vector	temp_v;
	double angle,tmp;
	Vector axis;

	if ( ! aligning )
		Edit_Undo_Register_State(edit_drag_op, ROTATE, 0);

	previous_rot =
	info->drag_transform.matrix = 
		Major_Align_Matrix(direction, &(info->axes), &info->rotation);

	/* Rotate the reference point. */
	MVMul(previous_rot, info->obj->o_reference, temp_v);
	info->obj->o_reference = temp_v;

	start_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
	init_obj_transform = info->obj->o_transform;

	Rotate_Object(info);

	Edit_Maintain_All_Constraints(info);

	Edit_Finish_Rotate_Drag(NULL, info);
	
}


void
Edit_Minor_Align(Vector direction, Boolean aligning)
{
	EditInfoPtr	info = Edit_Get_Info();
	Vector	new_other;
	double tmp,angle;
	Vector axis;
	Vector	temp_v;

	/* Work out where the new other axis is. */
	VCross(info->axes.x, direction, new_other);

	if ( VZero(new_other) )
	{
		XBell(XtDisplay(main_window.shell), 0);
		return;
	}

	if ( ! aligning )
		Edit_Undo_Register_State(edit_drag_op, ROTATE, 0);

	if ( ! do_maintenance )
	{
		Edit_Draw(info->window, ViewNone, info, FALSE);
		Edit_Draw(info->window, RemoveHidden, info, TRUE);
	}

	previous_rot =
	info->drag_transform.matrix = 
		Minor_Align_Matrix(direction, &(info->axes), &info->rotation);

	/* Rotate the reference point. */
	MVMul(previous_rot, info->obj->o_reference, temp_v);
	info->obj->o_reference = temp_v;

	start_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
	init_obj_transform = info->obj->o_transform;

	Rotate_Object(info);

	if ( do_maintenance )
		Edit_Maintain_All_Constraints(info);

	Edit_Finish_Rotate_Drag(NULL, info);
}
