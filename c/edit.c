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
**	edit.c : functions to edit the shape of an object.
**
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <instance_list.h>
#include <select_point.h>
#include <View.h>
#include <X11/cursorfont.h>

static void	Edit_Initialize_Info(WindowInfoPtr, InstanceList);
static void	Edit_Prepare_Instances();
static void	Edit_Initialize_Axes();

static Cursor	origin_cursor = 0;
static Cursor	reference_cursor = 0;
static Cursor	rotate_cursor = 0;
static Cursor	no_motion_cursor = 0;
static int		cursor_state = ROTATE;
static Cursor	old_cursor = 0;

static EditInfo	edit_info;


void
Edit_Instance(WindowInfoPtr window, InstanceList inst)
{
	/* Mark all the dependents. */
	DFS(inst->the_instance, NULL, 0, TRUE, &topological_list);

	Edit_Initialize_Info(window, inst);

	Edit_Initialize_Shell(&edit_info);

	Initiate_Object_Edit(window);

	View_Update(window, NULL, CalcView );
}


static void
Edit_Initialize_Info(WindowInfoPtr window, InstanceList inst)
{
	edit_info.window = window;
	edit_info.inst = inst;
	edit_info.obj = inst->the_instance;

	edit_info.axes = edit_info.obj->o_axes;
	edit_info.axes_inverse = MInvert(&(edit_info.axes));
	edit_info.axes_obj = New(ObjectInstance, 1);

	VAdd(edit_info.obj->o_origin, edit_info.obj->o_transform.displacement,
		 edit_info.origin);

	VAdd(edit_info.obj->o_reference, edit_info.obj->o_transform.displacement,
		 edit_info.reference);

	edit_info.selecting = edit_info.deleting = FALSE;

	edit_info.num_origin_pts = 0;
	edit_info.num_reference_pts = 0;
	edit_info.num_align_pts = 0;

	/* set some values for the edited rotation and scale values */
	edit_info.rotation = edit_info.obj->o_rot;
	edit_info.scale = edit_info.obj->o_scale;
	QNew(1,0,0,0,edit_info.workrot);
	VNew(1,1,1,edit_info.workscale);

	Edit_Prepare_Instances();

	Edit_Initialize_Axes();

	Edit_Calculate_Extras();
}


void
Edit_Clear_Info()
{
	free(edit_info.axes_obj->o_world_verts);
	free(edit_info.axes_obj->o_main_verts);
	free(edit_info.axes_obj);

	if ( edit_info.num_origin_pts ) free(edit_info.origin_def_pts);
	if ( edit_info.num_reference_pts ) free(edit_info.reference_def_pts);
	if ( edit_info.num_align_pts ) free(edit_info.align_def_pts);

	free(edit_info.reference_available);
	Free_Selection_List(edit_info.all_available);
}


static void
Edit_Prepare_Instances()
{
	InstanceList	temp_list;
	InstanceList	axis_list;

	/* Sets up the instance lists for point selection. */

	/*edit_info.obj->o_flags &= ( ObjAll ^ ObjVisible );*/

	/* The reference_available list is the object itself, but with new
	** vertices.
	*/
	temp_list = New(InstanceListElmt, 1);
	temp_list->next = temp_list->prev = NULL;
	temp_list->the_instance = edit_info.obj;
	edit_info.reference_available = temp_list;


	/* The all_available list is a list of all visible objects + the
	** axes object.
	*/
	axis_list = New(InstanceListElmt, 1);
	axis_list->the_instance = &(edit_info.window->axes);
	axis_list->next = axis_list->prev = NULL;
	edit_info.other_available = Merge_Selection_Lists(axis_list,
								edit_info.window->all_instances);

	/* The all list currently has the old object in it, which we
	** don't want.
	*/
	if ( ( temp_list =
		   Find_Object_In_Instances(edit_info.obj, edit_info.other_available)))
	{
		if ( temp_list == edit_info.other_available )
			edit_info.other_available = edit_info.other_available->next;
		Delete_Element(temp_list);
		free(temp_list);
	}

	/* We wish to add the obj itself at the front of the list. */
	temp_list = New(InstanceListElmt, 1);
	temp_list->the_instance = edit_info.obj;
	temp_list->next = edit_info.other_available;
	temp_list->prev = NULL;
	edit_info.all_available = temp_list;
}





/*	Boolean
**	Edit_Update_Feature_Specs(...)
**	Updates all the specifications for the given constraint, and then updates
**	the constraint. Returns TRUE if the constraint changes.
*/
Boolean
Edit_Update_Feature_Specs(FeaturePtr cons, Vector center, Vector ref,Vector org)
{
	Vector	orig_vect, orig_point;
	Vector	endpoints[3];
	Vector	temp_v1, temp_v2, temp_v3;
	double	temp_d;

#define Update_Spec(spec, i) \
	if ( spec.spec_type == offset_spec ) \
		VAdd(spec.spec_vector, center, endpoints[i]); \
	if ( spec.spec_type == reference_spec ) \
		Transform_Vector(spec.spec_object->o_transform, spec.spec_vector, \
						 endpoints[i]) \
	if ( spec.spec_type == absolute_spec ) \
		endpoints[i] = spec.spec_vector; \
	if ( spec.spec_type == origin_spec ) \
	{ \
		VAdd(center, org, spec.spec_vector); \
		endpoints[i] = spec.spec_vector; \
	} \
	if ( spec.spec_type == ref_point_spec ) \
	{ \
		VAdd(center, ref, spec.spec_vector); \
		endpoints[i] = spec.spec_vector; \
	}


	orig_vect = cons->f_vector;
	orig_point = cons->f_point;

	switch ( cons->f_spec_type )
	{
		case plane_feature:
		case axis_plane_feature:
		case orig_plane_feature:
		case ref_plane_feature:
			Update_Spec(cons->f_specs[2], 2);
		case line_feature:
		case axis_feature:
		case midplane_feature:
		case midpoint_feature:
		case ref_line_feature:
		case orig_line_feature:
			Update_Spec(cons->f_specs[1], 1);
		case point_feature:
			Update_Spec(cons->f_specs[0], 0);
		default:;
	}
	switch ( cons->f_spec_type )
	{
		case plane_feature:
		case orig_plane_feature:
		case ref_plane_feature:
			VSub(endpoints[1], endpoints[0], temp_v1);
			VSub(endpoints[2], endpoints[0], temp_v2);
			VCross(temp_v1, temp_v2, temp_v3);
			if ( VZero(temp_v3) )
				return FALSE;
			VUnit(temp_v3, temp_d, cons->f_vector);
			cons->f_point = endpoints[0];
			cons->f_value = VDot(cons->f_vector, cons->f_point);
			break;
		case line_feature:
		case axis_feature:
		case ref_line_feature:
		case orig_line_feature:
			VSub(endpoints[1], endpoints[0], temp_v1);
			if ( VZero(temp_v1) )
				return FALSE;
			VUnit(temp_v1, temp_d, cons->f_vector);
			cons->f_point = endpoints[0];
			break;
		case point_feature:
			cons->f_point = endpoints[0];
			break;
		case midplane_feature:
			VSub(endpoints[0], endpoints[1], temp_v1);
			if ( VZero(temp_v1) )
				return FALSE;
			VUnit(temp_v1, temp_d, cons->f_vector);
			VNew((endpoints[0].x + endpoints[1].x) / 2,
				 (endpoints[0].y + endpoints[1].y) / 2,
				 (endpoints[0].z + endpoints[1].z) / 2,
				 cons->f_point);
			cons->f_value = VDot(cons->f_vector, cons->f_point);
			break;
		case midpoint_feature:
			VNew((endpoints[0].x + endpoints[1].x) / 2,
				 (endpoints[0].y + endpoints[1].y) / 2,
				 (endpoints[0].z + endpoints[1].z) / 2,
				 cons->f_point);
			break;
		case axis_plane_feature:
			VSub(endpoints[1], endpoints[0], temp_v1);
			VSub(endpoints[2], endpoints[0], temp_v2);
			VCross(temp_v1, temp_v2, temp_v3);
			if ( VZero(temp_v3) )
				return FALSE;
			VUnit(temp_v3, temp_d, cons->f_vector);
			cons->f_point = endpoints[0];
			break;
		default:;
	}

	return ( ! VEqual(orig_vect, cons->f_vector, temp_v1) ||
			 ! VEqual(orig_point, cons->f_point, temp_v1) );
}


Boolean
Edit_Update_Object_Constraints(ConstraintBoxPtr box, EditInfoPtr info)
{
	int	i;
	Boolean result = FALSE;

	for ( i = 3 ; i < box->num_options ; i++ )
		result = Edit_Update_Feature_Specs(box->options[i],
						info->obj->o_world_verts[info->obj->o_num_vertices - 1],
						info->obj->o_reference,
						info->obj->o_origin) || result;

	return result;
}


Boolean
Edit_Update_Active_Object_Cons(ConstraintBoxPtr box, EditInfoPtr info,
							   Vector center)
{
	int 	i;
	Boolean	result = FALSE;

	for ( i = 3 ; i < box->num_options ; i++ )
		if ( box->options[i]->f_status )
		{
			result = Edit_Update_Feature_Specs(box->options[i], center,
											   info->obj->o_reference,
											   info->obj->o_origin) || result;
		}

	return result;
}


Boolean
Edit_Update_Constraints(EditInfoPtr info)
{
	Boolean	res1, res2, res3;

	/* Reset the defaults first. */
	Edit_Set_Origin_Defaults(origin_constraints.options, info->origin);
	Edit_Set_Scale_Defaults(scale_constraints.options, info->reference,
							info->axes);
	Edit_Set_Rotate_Defaults(rotate_constraints.options, info->axes);

	/* Will also need to reset all the other non selected constraints. */
	res1 = Edit_Update_Object_Constraints(&origin_constraints, info);
	res2 = Edit_Update_Object_Constraints(&scale_constraints, info);
	res3 = Edit_Update_Object_Constraints(&rotate_constraints, info);

	return ( res1 || res2 || res3 );
}


EditInfoPtr
Edit_Get_Info()
{
	return &edit_info;
}


void
Edit_Draw_Selection_Points(EditInfoPtr info)
{
	Draw_Origin_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Scale_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Rotate_Constraints(info->window, ViewNone, info, FALSE);
	Draw_Edit_Extras(info->window, ViewNone, info, TRUE);

	Prepare_Selection_Drawing();

	Select_Highlight_Closest(XtWindow(info->window->view_widget));
	Draw_Selection_Points(XtWindow(info->window->view_widget));
}

void
Edit_Cleanup_Selection(Boolean draw)
{
	edit_info.selecting = FALSE;

	Cleanup_Selection();

	Cancel_Select_Operation();

	Edit_Sensitize_Buttons(TRUE, FALSE);
	if ( draw )
	{
		Draw_Origin_Constraints(edit_info.window, ViewNone, &edit_info, FALSE);
		Draw_Scale_Constraints(edit_info.window, ViewNone, &edit_info, FALSE);
		Draw_Rotate_Constraints(edit_info.window, ViewNone, &edit_info, FALSE);
		Draw_Edit_Extras(edit_info.window, ViewNone, &edit_info, TRUE);
	}
}



static void
Edit_Initialize_Axes()
{
	edit_info.axes_obj->o_num_vertices = 7;
	edit_info.axes_obj->o_world_verts = New(Vector, 7);
	edit_info.axes_obj->o_main_verts = New(Vertex, 7);
	edit_info.axes_obj->o_num_faces = 0;
	edit_info.axes_obj->o_flags |= ObjVisible;
}

void
Edit_Transform_Vertices(Transformation *trans, EditInfoPtr info)
{
	Vector	*orig;
	int		i;

	orig = info->obj->o_wireframe->vertices;
	for ( i = 0 ; i < info->obj->o_num_vertices ; i++ )
		Transform_Vector(*trans, orig[i], info->obj->o_world_verts[i]);
}


void
Edit_Transform_Normals(Transformation *trans, EditInfoPtr info)
{
	Matrix		inverse, transp;
	Face		*face;
	double		temp_d;
	int			i;

	inverse = MInvert(&(trans->matrix));
	MTrans(inverse, transp);
	face = info->obj->o_wireframe->faces;
	for ( i = 0 ; i < info->obj->o_num_faces ; i++ )
	{
		MVMul(transp, face[i].normal, info->obj->o_normals[i]);
		if ( ! VZero(info->obj->o_normals[i]) )
			VUnit(info->obj->o_normals[i], temp_d, info->obj->o_normals[i]);
	}
}


static void
Edit_Calc_Line_Points(Vector *verts, Vector point, Vector dir)
{
	VScalarMul(dir, sced_resources.line_con_length / 2.0, dir);
	VAdd(point, dir, verts[0]);
	verts[2] = verts[0];
	VSub(point, dir, verts[1]);
	verts[3] = verts[1];
}


static void
Edit_Calc_Plane_Points(Vector *verts, Vector point, Vector norm)
{
	Vector	axis;
	Vector	dir1, dir2;
	double	temp_d;

	/* Need 2 vectors perp to the normal and each other. */
	VNew(1, 0, 0, axis);
	VCross(norm, axis, dir1);
	if ( VZero(dir1) )
	{
		VNew(0, 1, 0, axis);
		VCross(norm, axis, dir1);
	}
	VCross(dir1, norm, dir2);
	VUnit(dir1, temp_d, dir1);
	VUnit(dir2, temp_d, dir2);
	VScalarMul(dir1, sced_resources.plane_con_length / 2.0, dir1);
	VScalarMul(dir2, sced_resources.plane_con_length / 2.0, dir2);

	VAdd(point, dir1, verts[0]);
	VAdd(point, dir2, verts[1]);
	VSub(point, dir1, verts[2]);
	VSub(point, dir2, verts[3]);
}


void
Edit_Calculate_Origin_Cons_Points()
{
	Vector	worlds[4];
	Vertex	views[4];
	Dimension	width, height;
	int			mag;
	int			i;

	/* Calculate the 4 screen points that define an origin constraint. */
	/* If the resulting constraint is a line, then 0 & 2 and 1 & 3 will be
	** identical. */

	if ( edit_info.origin_resulting.f_type == line_feature )
		Edit_Calc_Line_Points(worlds, edit_info.origin,
							  edit_info.origin_resulting.f_vector);
	else if ( edit_info.origin_resulting.f_type == plane_feature )
		Edit_Calc_Plane_Points(worlds, edit_info.origin,
							   edit_info.origin_resulting.f_vector);
	else if ( edit_info.origin_resulting.f_type == inconsistent_feature )
	{
		edit_info.origin_pts[0].x = edit_info.origin_vert.screen.x +
									sced_resources.incon_con_length;
		edit_info.origin_pts[0].y = edit_info.origin_vert.screen.y +
									sced_resources.incon_con_length;
		edit_info.origin_pts[1].x = edit_info.origin_vert.screen.x -
									sced_resources.incon_con_length;
		edit_info.origin_pts[1].y = edit_info.origin_vert.screen.y -
									sced_resources.incon_con_length;
		edit_info.origin_pts[2].x = edit_info.origin_vert.screen.x +
									sced_resources.incon_con_length;
		edit_info.origin_pts[2].y = edit_info.origin_vert.screen.y -
									sced_resources.incon_con_length;
		edit_info.origin_pts[3].x = edit_info.origin_vert.screen.x -
									sced_resources.incon_con_length;
		edit_info.origin_pts[3].y = edit_info.origin_vert.screen.y +
									sced_resources.incon_con_length;
	}

	if ( edit_info.origin_resulting.f_type == line_feature ||
		 edit_info.origin_resulting.f_type == plane_feature )
	{
		XtVaGetValues(edit_info.window->view_widget, XtNwidth, &width,
					 XtNheight, &height,
					 XtNmagnification, &mag, NULL);

		Convert_World_To_View(worlds, views, 4, &(edit_info.window->viewport));
		Convert_View_To_Screen(views, 4, &(edit_info.window->viewport),
							   (short)width, (short)height, (double)mag);

		for ( i = 0 ; i < 4 ; i++ )
			edit_info.origin_pts[i] = views[i].screen;
	}

	/* Build all the constraint description points. */
	Edit_Calculate_Spec_Points(origin_constraints.options + 3,
							   origin_constraints.num_options - 3,
							   &(edit_info.origin_def_pts),
							   &(edit_info.num_origin_pts));
}


void
Edit_Calculate_Scale_Cons_Points()
{
	Vector	worlds[4];
	Vertex	views[4];
	Dimension	width, height;
	int			mag;
	int			i;

	if ( edit_info.reference_resulting.f_type == line_feature )
		Edit_Calc_Line_Points(worlds, edit_info.reference,
							  edit_info.reference_resulting.f_vector);
	else if ( edit_info.reference_resulting.f_type == plane_feature )
		Edit_Calc_Plane_Points(worlds, edit_info.reference,
							   edit_info.reference_resulting.f_vector);
	else if ( edit_info.reference_resulting.f_type == inconsistent_feature )
	{
		edit_info.reference_pts[0].x = edit_info.reference_vert.screen.x +
									sced_resources.incon_con_length;
		edit_info.reference_pts[0].y = edit_info.reference_vert.screen.y +
									sced_resources.incon_con_length;
		edit_info.reference_pts[1].x = edit_info.reference_vert.screen.x -
									sced_resources.incon_con_length;
		edit_info.reference_pts[1].y = edit_info.reference_vert.screen.y -
									sced_resources.incon_con_length;
		edit_info.reference_pts[2].x = edit_info.reference_vert.screen.x +
									sced_resources.incon_con_length;
		edit_info.reference_pts[2].y = edit_info.reference_vert.screen.y -
									sced_resources.incon_con_length;
		edit_info.reference_pts[3].x = edit_info.reference_vert.screen.x -
									sced_resources.incon_con_length;
		edit_info.reference_pts[3].y = edit_info.reference_vert.screen.y +
									sced_resources.incon_con_length;
	}

	if ( edit_info.reference_resulting.f_type == line_feature ||
		 edit_info.reference_resulting.f_type == plane_feature )
	{
		XtVaGetValues(edit_info.window->view_widget, XtNwidth, &width,
					 XtNheight, &height,
					 XtNmagnification, &mag, NULL);

		Convert_World_To_View(worlds, views, 4, &(edit_info.window->viewport));
		Convert_View_To_Screen(views, 4, &(edit_info.window->viewport),
							   (short)width, (short)height, (double)mag);

		for ( i = 0 ; i < 4 ; i++ )
			edit_info.reference_pts[i] = views[i].screen;
	}

	/* Build all the constraint description points. */
	Edit_Calculate_Spec_Points(scale_constraints.options + 3,
							   scale_constraints.num_options - 3,
							   &(edit_info.reference_def_pts),
							   &(edit_info.num_reference_pts));
}


void
Edit_Calculate_Arc_Points(int width, int height)
{
	double	angle_diff;
	Vector	view_axis;
	Matrix	inverse_trans;
	Vector	start_pt;
	Vertex	*arc_views;
	Matrix	rotation;
	Quaternion	rot_quat;
	XPoint	offset;
	double	temp_d;
	int		i;

	if ( edit_info.rotate_resulting.f_type != line_feature )
	{
		edit_info.num_arc_pts = 0;
		return;
	}

	/* Get the axis in view.  It's the same as transforming a normal. */
	MTrans(edit_info.window->viewport.view_to_world.matrix, inverse_trans);
	MVMul(inverse_trans, edit_info.rotate_resulting.f_vector, view_axis);

	VUnit(view_axis, temp_d, view_axis);

	arc_views = New(Vertex, ARC_DIVISIONS + 1);

	/* Find a point on the sphere perp to the axis through the center. */
	if ( DEqual(view_axis.z, 1.0) )
	{
		start_pt.x = 1.0;
		start_pt.y = 0.0;
		start_pt.z = 0.0;
	}
	else
	{
		double	temp = sqrt(1.0 - view_axis.z * view_axis.z);
		start_pt.x = view_axis.y / temp;
		start_pt.y = -view_axis.x / temp;
		start_pt.z = 0.0;
	}

	/* Find all the points. */
	angle_diff = 2 * M_PI / ARC_DIVISIONS;
	rot_quat.real_part = cos(angle_diff / 2);
	rot_quat.vect_part = view_axis;
	VScalarMul(rot_quat.vect_part, sin(angle_diff / 2), rot_quat.vect_part);
	rotation = Quaternion_To_Matrix(rot_quat);

	arc_views[0].view = start_pt;
	for ( i = 1 ; i < ARC_DIVISIONS ; i++ )
		MVMul(rotation, arc_views[i-1].view, arc_views[i].view);
	arc_views[i].view = start_pt;

	Convert_View_To_Screen(arc_views, ARC_DIVISIONS,
				&(edit_info.window->viewport), width, height,
				(double)edit_info.radius);
	edit_info.num_arc_pts = 0;
	if ( arc_views[0].view.z < arc_views[1].view.z )
	{
		for ( i = ARC_DIVISIONS - 1 ; i >= 0 ; i-- )
			if ( arc_views[i].view.z <= 0.0 )
				edit_info.arc_points[edit_info.num_arc_pts++] =
				arc_views[i].screen;
	}
	else
	{
		for ( i = 0 ; i < ARC_DIVISIONS + 1 ; i++ )
			if ( arc_views[i].view.z <= 0.0 )
				edit_info.arc_points[edit_info.num_arc_pts++] =
				arc_views[i].screen;
	}
	edit_info.num_arc_pts--;

	offset.x = edit_info.origin_vert.screen.x - width / 2;
	offset.y = edit_info.origin_vert.screen.y - height / 2;
	for ( i = 0 ; i < edit_info.num_arc_pts ; i++ )
	{
		edit_info.arc_points[i].x += offset.x;
		edit_info.arc_points[i].y += offset.y;
	}

	free(arc_views);
}


void
Edit_Add_Spec_Vert(FeatureSpecPtr spec, Vertex **verts, FeatureSpecType **types,
				   int *num_verts, int *max_num_verts)
{
	Vector	world;

	switch ( spec->spec_type )
	{
		case absolute_spec:
			world = spec->spec_vector;
			break;
		case offset_spec:
			VAdd(spec->spec_vector,
				 edit_info.obj->o_world_verts[edit_info.obj->o_num_vertices-1],
				 world);
			break;
		case reference_spec:
			Transform_Vector(spec->spec_object->o_transform,
							 spec->spec_vector, world);
			break;
		case origin_spec:
			return;
		case ref_point_spec:
			return;
	}

	if ( *num_verts >= *max_num_verts )
	{
		if ( *max_num_verts )
		{
			*max_num_verts += 10;
			*verts = More((*verts), Vertex, (*max_num_verts));
			*types = More((*types), FeatureSpecType, (*max_num_verts));
		}
		else
		{
			*max_num_verts = 10;
			*verts = New(Vertex, (*max_num_verts));
			*types = New(FeatureSpecType, (*max_num_verts));
		}
	}

	Convert_World_To_View(&(world), *verts + *num_verts, 1,
						  &(edit_info.window->viewport));
	(*types)[(*num_verts)++] = spec->spec_type;
}


/*
**	Edit_Add_Spec_Point(...)
**	Adds the defining points for the constraint to the list of verts.
*/
void
Edit_Add_Spec_Point(FeaturePtr feat, Vertex **verts, FeatureSpecType **specs,
					int *num_verts, int *max_num_verts)
{
	switch ( feat->f_spec_type )
	{
		case plane_feature:
		case axis_plane_feature:
		case orig_plane_feature:
		case ref_plane_feature:
			Edit_Add_Spec_Vert(feat->f_specs + 2, verts, specs, num_verts,
							   max_num_verts);
		case line_feature:
		case axis_feature:
		case midplane_feature:
		case midpoint_feature:
		case orig_line_feature:
		case ref_line_feature:
			Edit_Add_Spec_Vert(feat->f_specs + 1, verts, specs, num_verts,
							   max_num_verts);
		case point_feature:
			Edit_Add_Spec_Vert(feat->f_specs, verts, specs, num_verts,
							   max_num_verts);
		default:;
	}
}


/*	void
**	Edit_Calculate_Spec_Points(...)
**	Calculates the locations on screen of the points used to specify
**	constraints.
*/
void
Edit_Calculate_Spec_Points(FeaturePtr *feats, int num_feats, ConPoint **pts,
						   int *num_pts)
{
	static Vertex	*verts;
	static FeatureSpecType	*specs;
	static int		num_verts;
	static int		max_num_verts = 0;
	int				i, j;
	Boolean			found;
	Dimension		width, height;
	int				mag;


	if ( *num_pts )
		free(*pts);
	*num_pts = 0;

	num_verts = 0;
	for ( i = 0 ; i < num_feats ; i++ )
	{
		if ( feats[i]->f_status )
			Edit_Add_Spec_Point(feats[i], &verts, &specs, &num_verts,
								&max_num_verts);
	}

	if ( ! num_verts )
		return;

	XtVaGetValues(edit_info.window->view_widget, XtNwidth, &width,
												 XtNheight, &height,
												 XtNmagnification, &mag, NULL);

	Convert_View_To_Screen(verts, num_verts, &(edit_info.window->viewport),
						   (short)width, (short)height, (double)mag);

	*pts = New(ConPoint, num_verts);
	for ( i = 0 ; i < num_verts ; i++ )
	{
		found = FALSE;
		for ( j = 0 ; ! found && j < *num_pts ; j++ )
			if ( (*pts)[j].pt.x == verts[i].screen.x &&
				 (*pts)[j].pt.y == verts[i].screen.y )
				found = TRUE;
		if ( found ) continue;
		(*pts)[(*num_pts)].pt = verts[i].screen;
		(*pts)[(*num_pts)++].type = specs[i];
	}
}



void
Edit_Set_Origin_Defaults(FeaturePtr *options, Vector pt)
{
	int	i;

	/* These all have default vectors which are constant. */
	/* Only the points change. */

	for ( i = 0 ; i < 3 ; i++ )
	{
		options[i]->f_point = pt;
		options[i]->f_value = VDot(options[i]->f_vector, pt);
	}
}


void
Edit_Set_Scale_Defaults(FeaturePtr *options, Vector pt, Matrix axes)
{
	/* The defaults have vectors along the axis lines and pts through pt.	*/

	options[0]->f_vector = axes.z;
	options[1]->f_vector = axes.y;
	options[2]->f_vector = axes.x;

	Edit_Set_Origin_Defaults(options, pt);
}


void
Edit_Set_Rotate_Defaults(FeaturePtr *options, Matrix axes)
{
	/* The defaults have vectors along the axis lines. */
	options[0]->f_vector = axes.x;
	options[1]->f_vector = axes.y;
	options[2]->f_vector = axes.z;
}



void
Edit_Start_Drag(Widget w, XEvent *e, String *s, Cardinal *c)
{
	switch ( cursor_state )
	{
		case ORIGIN:
			Edit_Start_Origin_Drag(e, &edit_info);
			break;
		case SCALE:
			Edit_Start_Scale_Drag(e, &edit_info);
			break;
		case ROTATE:
			Edit_Start_Rotate_Drag(e, &edit_info);
			break;
	}
}


void
Edit_Continue_Drag(Widget w, XEvent *e, String *s, Cardinal *c)
{
	switch ( edit_info.drag_type )
	{
		case NO_DRAG:
			Edit_Set_Cursor_Action(w, e, s, c);
			break;

		case ORIGIN_DRAG:
			Edit_Continue_Origin_Drag(e, &edit_info);
			break;

		case SCALE_DRAG:
			Edit_Continue_Scale_Drag(e, &edit_info);
			break;

		case ROTATE_DRAG:
			Edit_Continue_Rotate_Drag(e, &edit_info);
			break;
	}
}


void
Edit_Finish_Drag(Widget w, XEvent *e, String *s, Cardinal *c)
{
	switch ( edit_info.drag_type )
	{
		case NO_DRAG: return;

		case ORIGIN_DRAG:
			Edit_Finish_Origin_Drag(e, &edit_info);
			break;

		case SCALE_DRAG:
			Edit_Finish_Scale_Drag(e, &edit_info);
			break;

		case ROTATE_DRAG:
			Edit_Finish_Rotate_Drag(e, &edit_info);
			break;
	}

	edit_info.drag_type = NO_DRAG;
}


static void
Edit_Create_Cursors()
{
	origin_cursor = XCreateFontCursor(XtDisplay(edit_info.window->shell),
									  XC_diamond_cross);
	reference_cursor = XCreateFontCursor(XtDisplay(edit_info.window->shell),
										 XC_sizing);
	rotate_cursor = XCreateFontCursor(XtDisplay(edit_info.window->shell),
									  XC_exchange);
	no_motion_cursor = XCreateFontCursor(XtDisplay(edit_info.window->shell),
										 XC_circle);

}


void
Edit_Set_Cursor_Action(Widget w, XEvent *e, String *s, Cardinal *c)
{
	Cursor	new_cursor;

	if ( edit_info.selecting ) return;

	if ( ! origin_cursor )
		Edit_Create_Cursors();

	/* Set the cursor according to what the resulting action will be */

	/* Origin has preference, then scale then rotate. You can always rotate. */
	if ( ( e->xbutton.x - edit_info.origin_vert.screen.x ) *
		 ( e->xbutton.x - edit_info.origin_vert.screen.x ) +
		 ( e->xbutton.y - edit_info.origin_vert.screen.y ) *
		 ( e->xbutton.y - edit_info.origin_vert.screen.y ) <
		 sced_resources.edit_pt_rad * sced_resources.edit_pt_rad )
	{
		cursor_state = ORIGIN;
		if ( edit_info.origin_resulting.f_type != plane_feature &&
			 edit_info.origin_resulting.f_type != line_feature )
			new_cursor = no_motion_cursor;
		else
			new_cursor = origin_cursor;
	}
	else if ( ( e->xbutton.x - edit_info.reference_vert.screen.x ) *
			  ( e->xbutton.x - edit_info.reference_vert.screen.x ) +
			  ( e->xbutton.y - edit_info.reference_vert.screen.y ) *
			  ( e->xbutton.y - edit_info.reference_vert.screen.y ) <
			  sced_resources.edit_pt_rad * sced_resources.edit_pt_rad )
	{
		cursor_state = SCALE;
		if ( edit_info.reference_resulting.f_type != plane_feature &&
			 edit_info.reference_resulting.f_type != line_feature )
			new_cursor = no_motion_cursor;
		else
			new_cursor = reference_cursor;
	}
	else
	{
		cursor_state = ROTATE;
		if ( edit_info.rotate_resulting.f_type != null_feature &&
			 edit_info.rotate_resulting.f_type != line_feature )
			new_cursor = no_motion_cursor;
		else
			new_cursor = rotate_cursor;
	}

	if ( old_cursor != new_cursor )
	{
		old_cursor = new_cursor;
		XDefineCursor(XtDisplay(edit_info.window->shell),
					XtWindow(edit_info.window->view_widget), new_cursor);
	}
}

void
Edit_Clear_Cursor()
{
	XDefineCursor(XtDisplay(edit_info.window->shell),
				XtWindow(edit_info.window->view_widget), None);
	old_cursor = 0;
}
