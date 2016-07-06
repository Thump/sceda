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
**	placement.c : Functions regarding object placement.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <View.h>


static void Edit_Move_Object(EditInfoPtr);

static FeatureData	view_feature;
static Dimension	width, height;
static int			mag;

static XPoint			drag_offset;
static Vector			start_center;
static Transformation	init_obj_transform;


/*	void
**	Edit_Start_Origin_Drag(XEvent *e, EditInfoPtr info)
**	Registers the start of a origin motion drag. Checks that such a drag
**	is possible, ie right constraints and no special conditions, then sets
**	appropriate flags.
*/
void
Edit_Start_Origin_Drag(XEvent *e, EditInfoPtr info)
{
	Vector	vect;
	double	val;

	if ( info->origin_resulting.f_type != plane_feature &&
		 info->origin_resulting.f_type != line_feature )
		return;

	if (info->origin_resulting.f_type == plane_feature)
	{
		/* Test that the constraint plane is not perp to the screen. */
		val = VDot(info->origin_resulting.f_vector,
					info->window->viewport.world_to_view.matrix.z);
		if ( IsZero(val) ) return;

		/* Bring the plane into view coords, to make the mapping simpler. */
		Convert_Plane_World_To_View(&(info->origin_resulting),
									&(info->window->viewport),
									&view_feature);
	}
	else
	{
		/* Test that the constraint line is not perp to the screen. */
		VCross(info->origin_resulting.f_vector,
			   info->window->viewport.world_to_view.matrix.z, vect);
		if ( VZero(vect) ) return;

		/* Bring the line into view coords. */
		Convert_Line_World_To_View(&(info->origin_resulting),
									&(info->window->viewport),
									&view_feature);
	}

	XtVaGetValues(info->window->view_widget, XtNheight, &height,
					XtNwidth, &width, XtNmagnification, &mag, NULL);

	NewIdentityMatrix(info->drag_transform.matrix);
	VNew(0, 0, 0, info->drag_transform.displacement);

	drag_offset.x = info->origin_vert.screen.x - e->xbutton.x; 
	drag_offset.y = info->origin_vert.screen.y - e->xbutton.y;

	/* Drag start is the position before the drag. */
	info->drag_start = info->origin;

	start_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
	init_obj_transform = info->obj->o_transform;

	Edit_Undo_Register_State(edit_drag_op, ORIGIN, 0);

	info->drag_type = ORIGIN_DRAG;

	Edit_Set_Drag_Label(ORIGIN_DRAG, info->drag_transform.displacement, 0);
}


/*	void
**	Continue_Origin_Drag(XEvent *e, EditInfoPtr info)
**	Continues the drag.  Mostly just works out where the object is now and
**	redraws it.
*/
void
Edit_Continue_Origin_Drag(XEvent *e, EditInfoPtr info)
{
	Vector	disp_moved;
	Vector	new_orig;
	XPoint	screen_pt;
	Vector	new_view;

	screen_pt.x = e->xmotion.x + drag_offset.x;
	screen_pt.y = e->xmotion.y + drag_offset.y;

	/* Get the new origin. */
	if (info->origin_resulting.f_type == plane_feature)
		new_view = Map_Point_Onto_Plane(screen_pt, view_feature,
						&(info->window->viewport), (short)width,
						(short)height, mag);
	else
		new_view = Map_Point_Onto_Line(screen_pt, view_feature,
						&(info->window->viewport), (short)width,
						(short)height, mag);

	/* Take the view_pt back into world. */
	Transform_Vector(info->window->viewport.view_to_world, new_view, new_orig);

	/* Find out how far it's moved. */
	VSub(new_orig, info->origin, disp_moved);
	info->origin = new_orig;

	/* Check to see if it's moved. */
	if ( VZero(disp_moved) ) return;

	VAdd(info->drag_transform.displacement, disp_moved,
		 info->drag_transform.displacement);

	Edit_Set_Drag_Label(ORIGIN_DRAG, info->drag_transform.displacement, 0);

	Edit_Move_Object(info);
}



/*	void
**	Finish_Origin_Drag(XEvent *e, EditInfoPtr info)
**	Redraws all the constraints and updates the constraint dependencies.
*/
void
Edit_Finish_Origin_Drag(XEvent *e, EditInfoPtr info)
{
	/* Rework the available options constraints. */
	/* The active ones were updated as the thing moved. */
	Edit_Update_Constraints(info);

	/* Update the current transform. */
	info->obj->o_transform.matrix = MMMul(&(info->drag_transform.matrix),
								 		  &(init_obj_transform.matrix));
	VAdd(init_obj_transform.displacement,
		 info->drag_transform.displacement,
		 info->obj->o_transform.displacement);

	Edit_Set_Drag_Label(NO_DRAG, info->obj->o_transform.displacement, 0);
}


/*	void
**	Edit_Move_Object(EditInfoPtr info)
**	Moves the object from its old position to a new one based on the
**	drag_displacement.
*/
static void
Edit_Move_Object(EditInfoPtr info)
{
	Vector	new_center;
	int		i;
	Transformation	align_trans;
	Boolean	did_align = FALSE;

	if ( ! do_maintenance )
	{
		/* Undraw the object. */
		Edit_Draw(info->window, ViewNone, info, FALSE);
		Draw_Edit_Extras(info->window, ViewNone, info, FALSE);

		/* Undraw the constraints. */
		Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);
	}

	VAdd(info->drag_transform.displacement, start_center, new_center);

	/* Update the reference point. */
	VAdd(info->obj->o_reference, new_center, info->reference);

	Edit_Set_Origin_Defaults(origin_constraints.options, info->origin);

	/* Check the alignment constraints. */
	if ( ( did_align = Edit_Dynamic_Align(info, &align_trans, new_center) ) )
	{
		Matrix	transp;
		Matrix	inverse;
		Vector	temp_v;
		double	temp_d;

		/* Rotate the axes. */
		MTrans(align_trans.matrix, transp);
		info->axes = MMMul(&(info->axes), &transp);
		info->obj->o_axes = info->axes;
		info->axes_inverse = MInvert(&(info->axes));

		/* Rotate the normals. */
		inverse = MInvert(&(align_trans.matrix));
		MTrans(inverse, transp);
		for ( i = 0 ; i < info->obj->o_num_faces ; i++ )
		{
			MVMul(transp, info->obj->o_normals[i], temp_v);
			VUnit(temp_v, temp_d, info->obj->o_normals[i]);
		}

		/* Move the center. */
		VAdd(align_trans.displacement, new_center, new_center);
		VSub(info->origin, new_center, info->obj->o_origin);

		/* Rotate the reference point. */
		MVMul(align_trans.matrix, info->obj->o_reference, temp_v);
		info->obj->o_reference = temp_v;
		VAdd(temp_v, new_center, info->reference);

		/* Update the drag transform. */
		info->drag_transform.matrix =
			MMMul(&(align_trans.matrix), &(info->drag_transform.matrix));
		VAdd(info->drag_transform.displacement, align_trans.displacement,
			 info->drag_transform.displacement);
	}

	/* Set the new object transform, because reference specifiers rely on it. */
	Apply_Transform(init_obj_transform, info->drag_transform,
					info->obj->o_transform);
	Transform_Vector(info->obj->o_transform,
			info->obj->o_wireframe->vertices[info->obj->o_num_vertices - 1],
			new_center);

	/* Update the scaling constraints. */
	Edit_Set_Scale_Defaults(scale_constraints.options, info->reference,
							info->axes);
	Edit_Update_Active_Object_Cons(&scale_constraints, info, new_center);
	Constraint_Solve_System(scale_constraints.options,
							scale_constraints.num_options,
							&(info->reference_resulting));

	/* Check the scaling constraints. */
	if ( Edit_Dynamic_Scale(info, new_center) )
	{
		Apply_Transform(init_obj_transform, info->drag_transform,
						info->obj->o_transform);
		Transform_Vector(info->obj->o_transform,
				info->obj->o_wireframe->vertices[info->obj->o_num_vertices - 1],
				new_center);

		VSub(info->reference, new_center, info->obj->o_reference);
		VSub(info->origin, new_center, info->obj->o_origin);
	}

	/* Update the rotation constraints, only for drawing purposes. */
	Edit_Update_Active_Object_Cons(&rotate_constraints, info,
					info->obj->o_world_verts[info->obj->o_num_vertices - 1]);
	Constraint_Solve_System(rotate_constraints.options,
							rotate_constraints.num_options,
							&(info->rotate_resulting));

	/* Transform the vertices. */
	Edit_Transform_Vertices(&(info->obj->o_transform), info);

	if ( do_maintenance )
		Edit_Maintain_All_Constraints(info);
	else
	{
		/* Redraw the object, recalculating as we go. */
		Draw_Origin_Constraints(info->window, CalcView, info, FALSE);
		Draw_Scale_Constraints(info->window, CalcView, info, FALSE);
		Draw_Rotate_Constraints(info->window, CalcView, info, FALSE);
		Edit_Draw(info->window, CalcView, info, FALSE);
		Draw_Edit_Extras(info->window, CalcView, info, TRUE);
	}
}



/*
**	Edit_Force_Origin_Satisfaction(EditInfoPtr info)
**	Checks to see whether the origin constraint is satisfied, and if not
**	satisfies it by moving the object.
*/
Boolean
Edit_Force_Origin_Satisfaction(EditInfoPtr info)
{
	if ( ! Point_Satisfies_Constraint(info->origin, &(info->origin_resulting)) )
	{
		NewIdentityMatrix(info->drag_transform.matrix);
		info->drag_start = info->origin;
		info->drag_transform.displacement =
			Find_Required_Motion(info->origin, &(info->origin_resulting));

		VAdd(info->origin, info->drag_transform.displacement, info->origin);

		/* Draw the old constraint on, because the function that called this
		** one undrew it, and the Edit_Move tries to draw it again.
		*/
		if ( ! do_maintenance )
			Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);

		start_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
		init_obj_transform = info->obj->o_transform;
		Edit_Move_Object(info);

		/* And draw the new one off, because the calling function will
		** redraw it when this one finishes.
		*/
		if ( ! do_maintenance )
			Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);

		Edit_Finish_Origin_Drag(NULL, info);

		return TRUE;
	}

	return FALSE;
}

