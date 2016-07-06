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
**	edit_extras.c: Functions for manipulating the extra edit features,
**					such as reference and origin points.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <select_point.h>
#include <View.h>

/* we need this for the definition of intptr_t */
#include <stdint.h>

static void	Edit_Calculate_Axes();
static void	Calculate_Circle(Vertex pt, XArc *arc);

static EditInfoPtr	info;

static int	change_axis;


/*	void
**	Reference_Select_Callback(int *index, FeatureSpecType *specifier)
**	The select_point callback that actually changes the reference point.
*/
static void
Reference_Select_Callback(int *index, FeatureSpecType *specifier)
{
	Edit_Undo_Register_State(edit_reference_op, 0, 0);

	info->reference = select_verts[*index].
					  obj->o_world_verts[select_verts[*index].offset];

	Edit_Cleanup_Selection(FALSE);

	VSub(info->reference, info->obj->o_world_verts[info->obj->o_num_vertices-1],
		 info->obj->o_reference);

	Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);

	Draw_Edit_Extras(info->window, CalcView, info, TRUE);


	Edit_Set_Scale_Defaults(scale_constraints.options, info->reference,
							info->axes);
	Edit_Update_Object_Constraints(&scale_constraints, info);
	Edit_Scale_Force_Constraints(info, FALSE);
	Constraint_Solve_System(scale_constraints.options,
							scale_constraints.num_options,
							&(info->reference_resulting));
	Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Scale_Constraints(info->window, CalcView, info, TRUE);
	Edit_Force_Scale_Satisfaction(info);

	if ( Edit_Update_Object_Constraints(&origin_constraints, info) )
	{
		Constraint_Solve_System(origin_constraints.options,
							origin_constraints.num_options,
							&(info->origin_resulting));
		Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Origin_Constraints(info->window, CalcView, info, TRUE);
	}

	/* Recalc temporary rotation arcs. */
	if ( Edit_Update_Active_Object_Cons(&rotate_constraints, info,
					info->obj->o_world_verts[info->obj->o_num_vertices - 1]) )
	{
		Constraint_Solve_System(rotate_constraints.options,
								rotate_constraints.num_options,
								&(info->rotate_resulting));
		Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);
		Draw_Rotate_Constraints(info->window, CalcView, info, TRUE);
	}
}



/*	void
**	Edit_Reference_Callback(Widget w, XtPointer cl, XtPointer ca)
**	Instigates a change reference callback by invoking the point selection
**	routines.
*/
void
Edit_Reference_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	info = Edit_Get_Info();

	if ( info->deleting )
		Edit_Cancel_Remove();

	/* Prepare for selection. */
	Register_Select_Operation();
	info->selecting = TRUE;
	Edit_Sensitize_Buttons(FALSE, FALSE);
	select_window = info->window;
	select_highlight = FALSE;
	num_verts_required = 1;
	select_finished_callback = Reference_Select_Callback;
	Build_Select_Verts_From_List(info->reference_available);
	specs_allowed[reference_spec] =
	specs_allowed[absolute_spec] =
	specs_allowed[offset_spec] = TRUE;	/* but I'll ignore the spec type. */
	allow_text_entry = FALSE;

	/* Draw all the reference points off. */
	Edit_Draw_Selection_Points(info);
}


/*	void
**	Origin_Select_Callback(int *index, FeatureSpecType *specifier)
**	The select point callback which actually changes the origin point.
*/
static void
Origin_Select_Callback(int *index, FeatureSpecType *specifier)
{
	Edit_Undo_Register_State(edit_origin_op, 0, 0);

	info->origin = select_verts[*index].
				   obj->o_world_verts[select_verts[*index].offset];

	Edit_Cleanup_Selection(FALSE);

	VSub(info->origin, info->obj->o_world_verts[info->obj->o_num_vertices - 1],
		 info->obj->o_origin);

	Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Edit_Extras(info->window, CalcView, info, TRUE);

	Edit_Set_Origin_Defaults(origin_constraints.options, info->origin);
	Edit_Update_Object_Constraints(&origin_constraints, info);
	Constraint_Solve_System(origin_constraints.options,
							origin_constraints.num_options,
							&(info->origin_resulting));
	Draw_Origin_Constraints(info->window, CalcView, info, TRUE);
	Edit_Force_Origin_Satisfaction(info);

	/* Update any alignment constraints. */
	Edit_Force_Alignment_Satisfaction(info);

	/* Force and update any scaling constraints. */
	Edit_Set_Scale_Defaults(scale_constraints.options, info->reference,
							info->axes);
	Edit_Update_Object_Constraints(&scale_constraints, info);
	Edit_Scale_Force_Constraints(info, FALSE);
	Constraint_Solve_System(scale_constraints.options,
							scale_constraints.num_options,
							&(info->reference_resulting));
	Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Scale_Constraints(info->window, CalcView, info, TRUE);
	Edit_Force_Scale_Satisfaction(info);

	/* Recalc temporary rotation arcs. */
	if ( Edit_Update_Active_Object_Cons(&rotate_constraints, info,
					info->obj->o_world_verts[info->obj->o_num_vertices - 1]) )
		Constraint_Solve_System(rotate_constraints.options,
								rotate_constraints.num_options,
								&(info->rotate_resulting));
	Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Rotate_Constraints(info->window, CalcView, info, TRUE);
}



/*	void
**	Edit_Origin_Callback(Widget w, XtPointer cl, XtPointer ca)
**	The callback invoked to change the origin point. All it does is prepare
**	the view for point selection.
*/
void
Edit_Origin_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	info = Edit_Get_Info();

	if ( info->deleting )
		Edit_Cancel_Remove();

	/* Prepare for selection. */
	Register_Select_Operation();
	info->selecting = TRUE;
	Edit_Sensitize_Buttons(FALSE, FALSE);
	select_window = info->window;
	select_highlight = FALSE;
	select_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
	num_verts_required = 1;
	select_finished_callback = Origin_Select_Callback;
	Build_Select_Verts_From_List(info->all_available);
	specs_allowed[reference_spec] =
	specs_allowed[absolute_spec] =
	specs_allowed[offset_spec] = TRUE;	/* but I'll ignore the spec type. */
	allow_text_entry = TRUE;

	/* Draw all the selection points on. */
	Edit_Draw_Selection_Points(info);
}


static void
Edit_Change_Axis(Vector new_dir)
{
	double	temp_d;
	Vector	cross1, cross2;

	Edit_Undo_Register_State(edit_axis_op, 0, 0);

	Edit_Cleanup_Selection(FALSE);

	switch ( change_axis )
	{
		case MAJOR_AXIS:
			VUnit(new_dir, temp_d, new_dir);
			VCross(new_dir, info->axes.y, cross1);
			if ( VZero(cross1) )
				cross1 = info->axes.z;
			VCross(new_dir, cross1, cross2);
			info->axes.x = new_dir;
			VUnit(cross1, temp_d, info->axes.z);
			VUnit(cross2, temp_d, info->axes.y);
			break;
		case MINOR_AXIS:
			VCross(info->axes.x, new_dir, cross1);
			if ( VZero(cross1) )
				cross1 = info->axes.z;
			VCross(cross1, info->axes.x, cross2);
			VUnit(cross1, temp_d, info->axes.z);
			VUnit(cross2, temp_d, info->axes.y);
			break;
	}

	/* Change the body axes. */
	info->obj->o_axes = info->axes;

	/* Recalc the inverse. */
	info->axes_inverse = MInvert(&(info->axes));

	Draw_Scale_Constraints(info->window, CalcView, info, FALSE);
	Draw_Rotate_Constraints(info->window, CalcView, info, FALSE);
	Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Edit_Extras(info->window, CalcView, info, TRUE);

	/* Redo alignment. */
	Edit_Force_Alignment_Satisfaction(info);

	/* Update the constraints. */
	Edit_Set_Scale_Defaults(scale_constraints.options, info->reference,
							info->axes);
	Edit_Scale_Force_Constraints(info, FALSE);
	Constraint_Solve_System(scale_constraints.options,
							scale_constraints.num_options,
							&(info->reference_resulting));
	Edit_Set_Rotate_Defaults(rotate_constraints.options, info->axes);
	Constraint_Solve_System(rotate_constraints.options,
							rotate_constraints.num_options,
							&(info->rotate_resulting));
}


static void
Edit_Select_1_Axis_Callback(int *ind, FeatureSpecType *spec)
{
	Vector	temp_v;
	Vector	new_dir;

	/* Work out the new direction. */
	temp_v = select_verts[*ind].obj->o_world_verts[select_verts[*ind].offset];
	VSub(temp_v, info->origin, new_dir);

	if ( VZero(new_dir) )
	{
		XBell(XtDisplay(main_window.shell), 0);
		return;
	}

	Edit_Change_Axis(new_dir);
}


void
Edit_Change_Axis_1_Callback(Widget widg, XtPointer cl, XtPointer ca)
{
	change_axis = (intptr_t)cl;
	info = Edit_Get_Info();

	if ( info->deleting )
		Edit_Cancel_Remove();

	/* Ask for 1. */
	/* Prepare for selection. */
	Register_Select_Operation();
	info->selecting = TRUE;
	Edit_Sensitize_Buttons(FALSE, FALSE);
	select_window = info->window;
	select_highlight = FALSE;
	select_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
	num_verts_required = 1;
	select_finished_callback = Edit_Select_1_Axis_Callback;
	Build_Select_Verts_From_List(info->all_available);
	specs_allowed[reference_spec] =
	specs_allowed[absolute_spec] =
	specs_allowed[offset_spec] = TRUE;
	allow_text_entry = TRUE;

	/* Draw all the selection points on. */
	Edit_Draw_Selection_Points(info);
}


static void
Edit_Select_2_Axis_Callback(int *indices, FeatureSpecType *specs)
{
	Vector	temp_v1, temp_v2;
	Vector	new_dir;

	/* Work out the new direction. */
	temp_v1 = select_verts[indices[0]].
				obj->o_world_verts[select_verts[indices[0]].offset];
	temp_v2 = select_verts[indices[1]].
				obj->o_world_verts[select_verts[indices[1]].offset];
	VSub(temp_v2, temp_v1, new_dir);

	if ( VZero(new_dir) )
	{
		XBell(XtDisplay(main_window.shell), 0);
		return;
	}

	Edit_Change_Axis(new_dir);
}


void
Edit_Change_Axis_2_Callback(Widget widg, XtPointer cl, XtPointer ca)
{
	change_axis = (intptr_t)cl;
	info = Edit_Get_Info();

	if ( info->deleting )
		Edit_Cancel_Remove();

	/* Ask for 2. */
	/* Prepare for selection. */
	Register_Select_Operation();
	info->selecting = TRUE;
	Edit_Sensitize_Buttons(FALSE, FALSE);
	select_window = info->window;
	select_highlight = FALSE;
	select_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
	num_verts_required = 2;
	select_finished_callback = Edit_Select_2_Axis_Callback;
	Build_Select_Verts_From_List(info->all_available);
	specs_allowed[reference_spec] =
	specs_allowed[absolute_spec] =
	specs_allowed[offset_spec] = TRUE;
	allow_text_entry = TRUE;

	/* Draw all the selection points on. */
	Edit_Draw_Selection_Points(info);
}

void
Edit_Calculate_Extras()
{
	Dimension	win_width, win_height;
	short		width, height;
	int			mag;

	info = Edit_Get_Info();

	XtVaGetValues(info->window->view_widget,
				  XtNwidth, &win_width,
				  XtNheight, &win_height,
				  XtNmagnification, &mag, NULL);
	width = (short)win_width;	height = (short)win_height;

	/* Calculate the origin information. */
	Convert_World_To_View(&(info->origin), &(info->origin_vert),
						  1, &(info->window->viewport));
	Convert_View_To_Screen(&(info->origin_vert), 1,
						   &(info->window->viewport), width, height,
						   (double)mag);
	Calculate_Circle(info->origin_vert, &(info->origin_circle));

	/* Calculate the reference information. */
	Convert_World_To_View(&(info->reference), &(info->reference_vert),
						  1, &(info->window->viewport));
	Convert_View_To_Screen(&(info->reference_vert), 1,
						   &(info->window->viewport), width, height,
						   (double)mag);
	Calculate_Circle(info->reference_vert, &(info->reference_circle));

	/* Calculate the arcball radius information. */
	info->radius = min( width - info->origin_vert.screen.x,
							info->origin_vert.screen.x );
	info->radius = min( info->radius, info->origin_vert.screen.y );
	info->radius = min( info->radius,
							height - info->origin_vert.screen.y );
	if ( info->radius < 0 ) info->radius = -info->radius;
	info->circle_arc.x = info->origin_vert.screen.x - info->radius;
	info->circle_arc.y = info->origin_vert.screen.y - info->radius;
	info->circle_arc.width = 2 * info->radius;
	info->circle_arc.height = 2 * info->radius;
	info->circle_arc.angle1 = 0;
	info->circle_arc.angle2 = 23040;

	Edit_Calculate_Axes();
}


static void
Edit_Calculate_Axes()
{
	Vector	temp_v;

	/* Work out the endpoints. */

	/* The 0, 2, 4 and 6 vertices are the body origin. */
	info->axes_obj->o_world_verts[0] = info->origin;
	info->axes_obj->o_world_verts[2] = info->origin;
	info->axes_obj->o_world_verts[4] = info->origin;
	info->axes_obj->o_world_verts[6] = info->origin;

	/* The others are the endpoints of the vectors. */
	VScalarMul(info->axes.x,
			   sced_resources.obj_x_axis_length /
			   (double)sced_resources.obj_axis_denom, temp_v);
	VAdd(info->origin, temp_v, info->axes_obj->o_world_verts[1]);
	VScalarMul(info->axes.y,
			   sced_resources.obj_y_axis_length /
			   (double)sced_resources.obj_axis_denom, temp_v);
	VAdd(info->origin, temp_v, info->axes_obj->o_world_verts[3]);
	VScalarMul(info->axes.z,
			   sced_resources.obj_z_axis_length /
			   (double)sced_resources.obj_axis_denom, temp_v);
	VAdd(info->origin, temp_v, info->axes_obj->o_world_verts[5]);

	/* The space conversions are done by the drawing routines. */
}


/*	void
**	Calculate_Circle(Vertex pt, XArc *arc)
**	Calculates the circle that will be used to indicate a drag point.
*/
static void
Calculate_Circle(Vertex pt, XArc *arc)
{
	arc->x = (short)(pt.screen.x - sced_resources.edit_pt_rad);
	arc->y = (short)(pt.screen.y - sced_resources.edit_pt_rad);
	arc->width = sced_resources.edit_pt_rad  << 1;
	arc->height = sced_resources.edit_pt_rad  << 1;
	arc->angle1 = 0;
	arc->angle2 = 23040;
}
