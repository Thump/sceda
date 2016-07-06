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
**	scale.c : functions to scale the object.
**
*/

#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <View.h>
#include <X11/Xaw/Toggle.h>


static void Scale_Object(EditInfoPtr);

static FeatureData	view_feature;
static Dimension	width, height;
static int			mag;

static Vector			orig_pt;
static XPoint			drag_offset;
static Transformation	init_obj_transform;

Boolean	scale_forced[3];


void
Edit_Start_Scale_Drag(XEvent *e, EditInfoPtr info)
{
	Vector	vect;
	double	val;

	if ( info->reference_resulting.f_type != plane_feature &&
		 info->reference_resulting.f_type != line_feature )
		return;

	if (info->reference_resulting.f_type == plane_feature)
	{
		/* Test that the constraint plane is not perp to the screen. */
		val = VDot(info->reference_resulting.f_vector,
					info->window->viewport.world_to_view.matrix.z);
		if ( IsZero(val) ) return;

		Convert_Plane_World_To_View(&(info->reference_resulting),
									&(info->window->viewport),
									&view_feature);
	}
	else
	{
		/* Test that the constraint line is not perp to the screen. */
		VCross(info->reference_resulting.f_vector,
			   info->window->viewport.world_to_view.matrix.z, vect);
		if ( VZero(vect) ) return;

		Convert_Line_World_To_View(&(info->reference_resulting),
									&(info->window->viewport),
									&view_feature);
	}

	XtVaGetValues(info->window->view_widget, XtNheight, &height,
					XtNwidth, &width, XtNmagnification, &mag, NULL);

	drag_offset.x = info->reference_vert.screen.x - e->xbutton.x;
	drag_offset.y = info->reference_vert.screen.y - e->xbutton.y;

	VSub(info->reference, info->origin, vect);
	MVMul(info->axes, vect, info->drag_start);
	orig_pt = info->drag_start;
	init_obj_transform = info->obj->o_transform;

	Edit_Undo_Register_State(edit_drag_op, SCALE, 0);

	info->drag_type = SCALE_DRAG;

	VNew(1, 1, 1, vect);
	Edit_Set_Drag_Label(SCALE_DRAG, vect, 0);
}



void
Edit_Continue_Scale_Drag(XEvent *e, EditInfoPtr info)
{
	Vector	new_view;
	XPoint	screen_pt;

	screen_pt.x = e->xmotion.x + drag_offset.x;
	screen_pt.y = e->xmotion.y + drag_offset.y;

	if (info->reference_resulting.f_type == plane_feature)
		new_view = Map_Point_Onto_Plane(screen_pt, view_feature,
						&(info->window->viewport), (short)width,
						(short)height, mag);
	else
		new_view = Map_Point_Onto_Line(screen_pt, view_feature,
						&(info->window->viewport), (short)width,
						(short)height, mag);

	/* Take the view_pt back into world. */
	MVMul(info->window->viewport.view_to_world.matrix, new_view,
		  info->reference);
	VAdd(info->window->viewport.view_to_world.displacement, info->reference,
		 info->reference);

	/* Work out what the complete scaling is. */
	Scale_Calculate_Transform(&(info->drag_transform), info->reference, orig_pt,
							 info->obj->o_origin, info->origin,
							 &(info->axes), &(info->axes_inverse), TRUE,
							&(info->workscale));

	Scale_Object(info);

	if ( do_maintenance )
		Edit_Maintain_All_Constraints(info);

}


void
Edit_Finish_Scale_Drag(XEvent *e, EditInfoPtr info)
{
	Apply_Transform(init_obj_transform, info->drag_transform,
					info->obj->o_transform);

	VSub(info->reference,
		 info->obj->o_world_verts[info->obj->o_num_vertices - 1],
		 info->obj->o_reference);
	VSub(info->origin, info->obj->o_world_verts[info->obj->o_num_vertices - 1],
		 info->obj->o_origin);

	/* Rework the available constraints. */
	Edit_Update_Constraints(info);
	Edit_Scale_Force_Constraints(info, FALSE);

	Edit_Set_Drag_Label(NO_DRAG, info->obj->o_reference, 0);
	VNew(info->scale.x*info->workscale.x,
		info->scale.y*info->workscale.y,
		info->scale.z*info->workscale.z,info->scale);
}


/*	void
**	Scale_Calculate_Transform()
**	Works out the transformation implied by the reference motion with
**	respect to the fixed point.
*/
void
Scale_Calculate_Transform(Transformation *result, Vector reference, Vector orig,
						 Vector center, Vector origin, Matrix *axes,
						 Matrix *axes_inverse, Boolean prompt, 
						Vector *scale)
{
	Vector	orig_to_ref;
	Vector	new_pt, to_center;
	Vector	sc_vect;

	NewIdentityMatrix(result->matrix);

	/* Take the vector from world to object space. */
	VSub(reference, origin, orig_to_ref);
	MVMul(*axes, orig_to_ref, new_pt);

	/* Do each dimension seperately. */
	if ( IsZero(orig.x) )
		result->matrix.x.x = 1;
	else
		result->matrix.x.x = new_pt.x / orig.x;

	if ( IsZero(orig.y) )
		result->matrix.y.y = 1;
	else
		result->matrix.y.y = new_pt.y / orig.y;

	if ( IsZero(orig.z) )
		result->matrix.z.z = 1;
	else
		result->matrix.z.z = new_pt.z / orig.z;

	/* Used to be we only computed the scaling vector if we were 
	** in prompt mode, that is, when the function had been called
	** on the object we were directly editing, rather than a dependent
	** object.  However, the animation stuff always needs to know the
	** scaling vector, so it can be saved either in the edited object
	** or in one of its dependents.
	**
	** It's more complicated than that though: this function is called
	** just once when it's being called on a dependent object, and in
	** that case, we have to multiply scaled vector by the existing
	** scale vector for that object to get the final scaling.  For the
	** directly edited object, on the other, this function is called
	** every time Edit_Continue_Scale_Drag() is called, which is lots
	** of times.  In that case, we only want to save the scaling vector,
	** and leave off multiplying this into the edited object's scale
	** vector until the object has finished being scaled.
	**
	** Yeah, well, most of the time I don't understand either.  It works,
	** though.  I think.
	*/ 
		VNew(result->matrix.x.x,result->matrix.y.y,result->matrix.z.z, sc_vect);
	if ( prompt )
	{	Edit_Set_Drag_Label(SCALE_DRAG, sc_vect, 0);
		VNew(sc_vect.x,sc_vect.y,sc_vect.z,*scale);
	}
	else
	{	VNew(sc_vect.x*scale->x,sc_vect.y*scale->y,sc_vect.z*scale->z,*scale);
	}


	/* Take the transformation back into world space. */
	result->matrix = MMMul(&(result->matrix), axes);
	result->matrix = MMMul(axes_inverse, &(result->matrix));

	/* Correct for 0's, which give singular matrices. */
	if ( IsZero(result->matrix.x.x) )
		result->matrix.x.x = SMALL_NUM;
	if ( IsZero(result->matrix.y.y) )
		result->matrix.y.y = SMALL_NUM;
	if ( IsZero(result->matrix.z.z) )
		result->matrix.z.z = SMALL_NUM;

	/* Scale the center by the appropriate amount. */
	VScalarMul(center, -1, to_center);
	MVMul(result->matrix, to_center, new_pt);

	/* The difference in center points is the displacement. */
	VSub(new_pt, to_center, result->displacement);
}

/*	void
**	Scale_Object()
**	Scales the object from its old position as specified by new_verts to
**	a new one based on the current_scaling.  new_verts is updated
**	in the process.
*/
static void
Scale_Object(EditInfoPtr info)
{
	/* Undraw the object. */
	if ( ! do_maintenance )
	{
		Edit_Draw(info->window, ViewNone, info, FALSE);
		Draw_Edit_Extras(info->window, ViewNone, info, FALSE);
		Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
	}

	Apply_Transform(init_obj_transform, info->drag_transform,
					info->obj->o_transform);

	Edit_Transform_Vertices(&(info->obj->o_transform), info);
	Edit_Transform_Normals(&(info->obj->o_transform), info);

	if ( ! do_maintenance )
		Draw_Scale_Constraints(info->window, CalcView, info, FALSE);

	/* Update the constraints. */
	/* Origin. */
	if ( Edit_Update_Active_Object_Cons(&origin_constraints, info,
					info->obj->o_world_verts[info->obj->o_num_vertices - 1]) )
	{
		Constraint_Solve_System(origin_constraints.options,
								origin_constraints.num_options,
								&(info->origin_resulting));
		if ( ! do_maintenance )
		{
			Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
			Draw_Origin_Constraints(info->window, CalcView, info, FALSE);
		}
	}

	/* Rotation. */
	if ( Edit_Update_Active_Object_Cons(&rotate_constraints, info,
					info->obj->o_world_verts[info->obj->o_num_vertices - 1]) )
	{
		Constraint_Solve_System(rotate_constraints.options,
								rotate_constraints.num_options,
								&(info->rotate_resulting));
		if ( ! do_maintenance )
		{
			Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);
			Draw_Rotate_Constraints(info->window, CalcView, info, FALSE);
		}
	}

	/* Redraw the object. */
	if ( ! do_maintenance )
	{
		Edit_Draw(info->window, CalcView, info, FALSE);
		Draw_Edit_Extras(info->window, CalcView, info, TRUE);
	}

}


/*
**	Edit_Force_Scale_Satisfaction(EditInfoPtr info)
**	Checks to see whether the scale constraint is satisfied, and if not
**	satisfies it by scaling the object.
*/
void
Edit_Force_Scale_Satisfaction(EditInfoPtr info)
{
	if ( ! Point_Satisfies_Constraint(info->reference,
									  &(info->reference_resulting)) )
	{
		Vector	to_center;
		Vector	vect;
		Vector	disp =
			Find_Required_Motion(info->reference, &(info->reference_resulting));

		VSub(info->reference, info->origin, vect);
		MVMul(info->axes, vect, orig_pt);
		VSub(info->origin,
			 info->obj->o_world_verts[info->obj->o_num_vertices - 1],
			 to_center);
		init_obj_transform = info->obj->o_transform;

		VAdd(info->reference, disp, info->reference);

		if ( ! do_maintenance )
			Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);

		/* Work out what the complete scaling is. */
		Scale_Calculate_Transform(&(info->drag_transform), info->reference,
								  orig_pt, to_center, info->origin,
								  &(info->axes), &(info->axes_inverse), 
								FALSE,&info->scale);

		Scale_Object(info);

		Edit_Finish_Scale_Drag(NULL, info);

		if ( do_maintenance )
			Edit_Maintain_All_Constraints(info);
		else
			Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
	}
}


/*	Boolean
**	Edit_Dynamic_Scale(EditInfoPtr info)
**	Checks to see if the scale constraints are satisfied, and if not,
**	updates the drag_transform to give a cumulative scaling that satisfies it.
**	Modifies info->reference and drag_transform only.
*/
Boolean
Edit_Dynamic_Scale(EditInfoPtr info, Vector center)
{
	if ( ! Point_Satisfies_Constraint(info->reference,
									  &(info->reference_resulting)) )
	{
		Transformation	temp_m = info->drag_transform;
		Vector	to_center;
		Vector	vect;
		Vector	disp =
			Find_Required_Motion(info->reference, &(info->reference_resulting));

		VSub(info->reference, info->origin, vect);
		MVMul(info->axes, vect, orig_pt);
		VSub(info->origin, center, to_center);

		VAdd(info->reference, disp, info->reference);

		/* Work out what the complete scaling is. */
		Scale_Calculate_Transform(&(info->drag_transform), info->reference,
								  orig_pt, to_center, info->origin,
								  &(info->axes), &(info->axes_inverse), 
									FALSE,&info->scale);

		info->drag_transform.matrix = MMMul(&(info->drag_transform.matrix),
											&(temp_m.matrix));
		VAdd(info->drag_transform.displacement, temp_m.displacement,
			 info->drag_transform.displacement);

		return TRUE;
	}

	return FALSE;
}



Boolean
Edit_Scale_Force_Constraints(EditInfoPtr info, Boolean ignore_old)
{
	Vector	body_space_ref;
	Vector	temp_v;
	Boolean	old[3];

	/* Convert a vector from the origin to the ref point to body space. */
	VSub(info->reference, info->origin, temp_v);
	MVMul(info->axes, temp_v, body_space_ref);

	old[0] = scale_forced[0];
	old[1] = scale_forced[1];
	old[2] = scale_forced[2];

	/* Find out which ones need forcing. */
	scale_forced[0] = IsZero(body_space_ref.z);
	scale_forced[1] = IsZero(body_space_ref.y);
	scale_forced[2] = IsZero(body_space_ref.x);

	/* Set the status of all the forced options. */
	if ( ignore_old || old[0] != scale_forced[0] )
		Edit_Select_Constraint(info, 0, &scale_constraints, scale_forced[0],
							   TRUE, TRUE);
	if ( ignore_old || old[1] != scale_forced[1] )
		Edit_Select_Constraint(info, 1, &scale_constraints, scale_forced[1],
							   TRUE, TRUE);
	if ( ignore_old || old[2] != scale_forced[2] )
		Edit_Select_Constraint(info, 2, &scale_constraints, scale_forced[2],
							   TRUE, TRUE);

	return ( old[0] != scale_forced[0] ||
			 old[1] != scale_forced[1] ||
			 old[2] != scale_forced[2] );
}




