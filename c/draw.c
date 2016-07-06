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
**	draw.c : The various drawing functions.
**
**	Created: 19/03/94
**
**	External functions:
**	void
**	View_Update(WindowInfoPtr window, int type);
**	Updates the view in the view widget.
**
**	void
**	Draw_Edges(Display *disp, Drawable draw, GC visible_gc, GC hidden_gc,
**		WireframePtr wire, Vertex *vertices, Vector *world_verts,
**		Vector *normals, Viewport *viewport) 
**	Draws the edges associated with the wireframe wire using vertices vertices
**	with face normals normals.
**	visible_gc is used for front edges and hidden_gc for back edges.
**
**	void
**	Draw_Visible_Edges(Display *disp, Drawable draw, GC gc, WireframePtr wire,
**		Vertex *vertices, Vector *world_verts, Vector *normals,
**		Viewport *viewport)
**	Draws only the edges of obj which are visible with respect to viewport.
**
**	void
**	Draw_All_Edges(Display *disp, Drawable draw, GC gc, WireframePtr wire,
**		Vertex *vertices, Boolean);
**	Draws all the edges, regardless of visibility.
*/

#include <math.h>
#include <sced.h>
#include <edit.h>
#include <select_point.h>
#include <View.h>

/* Local functions. */
static void Draw_Axes(Display*, Drawable, ObjectInstance);
static Boolean Is_Visible(WireframePtr, Vector*, Vector*, short, Viewport*);
static void Determine_Visible(WireframePtr, Vector*, Vector*, Viewport*);


static Pixmap		off_screen;
static GC			visible_gc;
static GC			highlight_gc;
static GC			hidden_gc;
static GC			highlight_hidden_gc;
static GC			clear_gc;
static GC			edit_gc;
static GC			edit_axis_gcs[3];
static GC			origin_gc;
static GC			reference_gc;
static GC			circle_gc;
static GC			constraint_gc;
static GC			lights_gc;
static GC			lights_hidden_gc;
static GC			lights_highlight_gc;
static GC			spec_gc[3];
XRectangle			area;
static XArc			point_arc;
static int			magnification = 0;

#define origin_width 2
#define origin_height 2
static char origin_bits[] = { 0x03, 0x01 };
static Pixmap	origin_stipple = 0;

static EditInfoPtr	draw_info;

static WindowInfoPtr	last_window = NULL;


/*	void
**	View_Update(WindowInfoPtr window, InstanceList instances, int type);
**	Updates the view in the view widget.  This can involve many things,
**	so a group of flags is used.
**	- ViewNone		Just do the drawing, and nothing else.
**	- CalcView		Recalculate the view coords.  This implies CalcScreen.
**	- CalcScreen	Recalculate the screen coords for the objects.
**	- RemoveHidden	Draw with hidden lines removed for each object.
**	- NewWindow		This is a new window.  Allocate new GCs etc.
*/
void
View_Update(WindowInfoPtr window, InstanceList instances, int flags)
{
	InstanceList		current_inst;
	Vertex				*view_verts;
	GC					foreground_gc, hide_gc;
	Dimension			new_width, new_height;

	if ( ( flags & JustExpose ) && last_window == window )
	{
		/* Copy the off screen on. */
		XCopyArea(XtDisplay(window->shell), window->off_screen,
				  XtWindow(window->view_widget), visible_gc,
				  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
				  area.x, area.y);

		if ( window->current_state & window_select )
		{
			Select_Highlight_Closest(XtWindow(window->view_widget));
			Draw_Selection_Points(XtWindow(window->view_widget));
		}
		return;
	}

	last_window = window;

	XtVaGetValues(window->view_widget,
				XtNwidth, &new_width,
				XtNheight, &new_height,
				XtNmagnification, &magnification, NULL);

	if ( new_width != window->width ||
		 new_height != window->height )
	{
		window->width = new_width;
		window->height = new_height;

		/* Clean up the old stuff. */
		if ( window->off_screen )
			XFreePixmap(XtDisplay(window->shell), window->off_screen);

		/* Allocate a new off_screen pixmap. */
		window->off_screen = XCreatePixmap(XtDisplay(window->shell),
								XtWindow(window->shell),
								(unsigned)new_width, (unsigned)new_height,
								DefaultDepthOfScreen(XtScreen(window->shell)));
	}

	off_screen = window->off_screen;
	area.x = area.y = 0;
	area.width = (short)window->width;
	area.height = (short)window->height;

	/* Clear the window. */
	XFillRectangles(XtDisplay(window->shell), off_screen, clear_gc, &area, 1);

	/* Deal with the axes. */
	/* Recalculate anyway.  It's little effort and saves worrying
	**	about which window it's in.
	*/
	Convert_World_To_View(window->axes.o_world_verts,
			window->axes.o_main_verts, 7, &(window->viewport));
	Convert_View_To_Screen(window->axes.o_main_verts, 7,
			&(window->viewport), area.width, area.height,(double)magnification);

	Draw_Axes(XtDisplay(window->shell), off_screen, window->axes);

	if ( flags & CalcView )
		for ( current_inst = instances ;
			  current_inst != NULL ; current_inst = current_inst->next )
		{
			if ( ! ( current_inst->the_instance->o_flags & ObjVisible ) )
				continue;

			Convert_World_To_View(current_inst->the_instance->o_world_verts,
				current_inst->the_instance->o_main_verts,
				current_inst->the_instance->o_num_vertices,
				&(window->viewport));
		}

	if ( flags & ( CalcView | CalcScreen ) )
		for ( current_inst = instances ;
			  current_inst != NULL ; current_inst = current_inst->next )
		{
			if ( ! ( current_inst->the_instance->o_flags & ObjVisible ) )
				continue;

			Convert_View_To_Screen(current_inst->the_instance->o_main_verts,
				current_inst->the_instance->o_num_vertices,
				&(window->viewport), area.width, area.height,
				(double)magnification);
		}

	for ( current_inst = window->all_instances ;
		  current_inst != NULL ; current_inst = current_inst->next )
	{
		if ( ! ( current_inst->the_instance->o_flags & ObjVisible ) )
			continue;

		view_verts = current_inst->the_instance->o_main_verts;

		foreground_gc = ( current_inst->the_instance->o_flags & ObjSelected ) &&
						( ! ( window->current_state & edit_object ) ) ?
						highlight_gc : visible_gc;
		if ( ( current_inst->the_instance->o_flags & ObjSelected ) &&
			 ! ( window->current_state & edit_object ) )
		{
			foreground_gc = highlight_gc;
			hide_gc = highlight_hidden_gc;
		}
		else
		{
			foreground_gc = visible_gc;
			hide_gc = hidden_gc;
		}

		if ( window->current_state & edit_object )
		{
			if ( current_inst->the_instance == draw_info->obj )
				continue;
			if ( current_inst->the_instance->o_flags & ObjDepends )
			{
				foreground_gc = hidden_gc;
				hide_gc = hidden_gc;
			}
		}

		if ( Obj_Is_Light(current_inst->the_instance) )
		{
			if ( foreground_gc == visible_gc )
				foreground_gc = lights_gc;
			else if ( foreground_gc == hidden_gc )
				foreground_gc = lights_hidden_gc;
			else
				foreground_gc = lights_highlight_gc;

			hide_gc = lights_hidden_gc;
		}

		if ( ( flags & RemoveHidden ) ||
			 window->viewport.draw_mode == DRAW_CULLED )
			Draw_Visible_Edges(XtDisplay(window->shell),
				off_screen, foreground_gc,
	 			current_inst->the_instance->o_wireframe,
				view_verts, current_inst->the_instance->o_world_verts,
				current_inst->the_instance->o_normals, &(window->viewport));
		else if ( window->viewport.draw_mode == DRAW_DASHED )
			Draw_Edges(XtDisplay(window->shell),
				off_screen, foreground_gc, hide_gc,
	 			current_inst->the_instance->o_wireframe,
				view_verts, current_inst->the_instance->o_world_verts,
				current_inst->the_instance->o_normals, &(window->viewport));
		else
			Draw_All_Edges(XtDisplay(window->shell),
				off_screen, foreground_gc,
				current_inst->the_instance->o_wireframe, view_verts,
				&(window->viewport));

		if ( current_inst->the_instance->o_parent->b_class == light_obj )
		{
			XPoint	center;

			center =
			view_verts[current_inst->the_instance->o_num_vertices - 1].screen;

			XFillArc(XtDisplay(window->shell), off_screen, lights_gc,
				center.x - sced_resources.light_pt_rad,
				center.y - sced_resources.light_pt_rad,
				sced_resources.light_pt_rad << 1,
				sced_resources.light_pt_rad << 1, 0, 23040);
		}
	}

	if ( window->current_state & edit_object )
	{
		Edit_Draw(window, flags, draw_info, FALSE);
		Draw_Edit_Extras(window, flags, draw_info, FALSE);
		Draw_Origin_Constraints(window, flags, draw_info, FALSE);
		Draw_Scale_Constraints(window, flags, draw_info, FALSE);
		Draw_Rotate_Constraints(window, flags, draw_info, FALSE);

		if ( draw_info->selecting )
			Draw_Edit_Extras(window, flags, draw_info, FALSE);
	}

	/* Copy the off screen on. */
	XCopyArea(XtDisplay(window->shell), off_screen,
			  XtWindow(window->view_widget), visible_gc,
			  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
			  area.x, area.y);

	if ( window->current_state & window_select )
	{
		if ( flags & ( CalcView | CalcScreen ) )
			Select_Calc_Screen();
		Select_Highlight_Closest(XtWindow(window->view_widget));
		Draw_Selection_Points(XtWindow(window->view_widget));
	}

}


void
Edit_Draw(WindowInfoPtr window, int flags, EditInfoPtr info, Boolean copy)
{
	off_screen = window->off_screen;
	area.x = area.y = 0;
	area.width = (short)window->width;
	area.height = (short)window->height;


	if (flags & CalcView)
		Convert_World_To_View(info->obj->o_world_verts, info->obj->o_main_verts,
							  info->obj->o_num_vertices, &(window->viewport));

	if (flags & (CalcView | CalcScreen))
		Convert_View_To_Screen(info->obj->o_main_verts,
			info->obj->o_num_vertices,
			&(window->viewport), area.width, area.height,(double)magnification);


	/* Draw the body. */
	if (flags & RemoveHidden)
		Draw_Visible_Edges_XOR(XtDisplay(window->shell), off_screen, edit_gc,
							info->obj->o_wireframe, info->obj->o_main_verts,
							info->obj->o_world_verts, info->obj->o_normals,
							&(window->viewport));
	else
		Draw_All_Edges_XOR(XtDisplay(window->shell), off_screen, edit_gc,
	 					info->obj->o_wireframe, info->obj->o_main_verts,
						&(window->viewport));

	if ( copy )
		/* Copy the off screen on. */
		XCopyArea(XtDisplay(window->shell), off_screen,
				  XtWindow(window->view_widget), visible_gc,
				  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
				  area.x, area.y);
}


void
Draw_Edit_Extras(WindowInfoPtr window, int flags, EditInfoPtr info,Boolean copy)
{
	int	i;

	off_screen = window->off_screen;
	area.x = area.y = 0;
	area.width = (short)window->width;
	area.height = (short)window->height;

	if ( flags & ( CalcView | CalcScreen ) )
		Edit_Calculate_Extras();

	if ( flags & CalcView )
		Convert_World_To_View(info->axes_obj->o_world_verts,
						info->axes_obj->o_main_verts, 7, &(window->viewport));

	if ( flags & ( CalcView | CalcScreen ) )
		Convert_View_To_Screen(info->axes_obj->o_main_verts, 7,
			&(window->viewport), area.width, area.height,(double)magnification);

	/* Draw the axes. */
	for ( i = 0 ; i < 3 ; i++ )
		XDrawLine(XtDisplay(window->shell), off_screen, edit_axis_gcs[i],
				  info->axes_obj->o_main_verts[i*2 + 1].screen.x,
				  info->axes_obj->o_main_verts[i*2 + 1].screen.y,
				  info->axes_obj->o_main_verts[i*2 + 2].screen.x,
				  info->axes_obj->o_main_verts[i*2 + 2].screen.y);

	/* Draw the origin point. */
	XFillArcs(XtDisplay(window->shell), off_screen,
					origin_gc, &(info->origin_circle), 1);

	/* Draw the reference point. */
	XFillArcs(XtDisplay(window->shell), off_screen,
					reference_gc, &(info->reference_circle), 1);

	/* Draw the rotate circle. */
	XDrawArcs(XtDisplay(window->shell), off_screen, circle_gc,
			  &(info->circle_arc), 1);

	if ( copy )
		/* Copy the off screen on. */
		XCopyArea(XtDisplay(window->shell), off_screen,
				  XtWindow(window->view_widget), visible_gc,
				  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
				  area.x, area.y);
}


static void
Draw_Set_Point_Arc(Vector center, WindowInfoPtr window)
{
	Vertex	view;

	Convert_World_To_View(&center, &view, 1, &(window->viewport));
	Convert_View_To_Screen(&view, 1, &(window->viewport),
						   area.width, area.height,(double)magnification);


	point_arc.x = view.screen.x - sced_resources.point_con_rad;
	point_arc.y = view.screen.y - sced_resources.point_con_rad;
	point_arc.width = sced_resources.point_con_rad << 1;
	point_arc.height = sced_resources.point_con_rad << 1;
	point_arc.angle1 = 0;
	point_arc.angle2 = 23040;
}



void
Draw_Origin_Constraints(WindowInfoPtr window, int flags, EditInfoPtr info,
						Boolean copy)
{
	int	i;

	off_screen = window->off_screen;
	area.x = area.y = 0;
	area.width = (short)window->width;
	area.height = (short)window->height;

	if ( info->origin_resulting.f_type == null_feature ) return;

	if ( flags & (CalcView | CalcScreen) )
		Edit_Calculate_Origin_Cons_Points();
	if ( info->origin_resulting.f_type == point_feature )
		Draw_Set_Point_Arc(info->origin, window);

	switch ( info->origin_resulting.f_type )
	{
		case plane_feature:
			XDrawLines(XtDisplay(window->shell), off_screen,
					constraint_gc, info->origin_pts, 4, CoordModeOrigin);
			XDrawLine(XtDisplay(window->shell), off_screen,
					constraint_gc, info->origin_pts[3].x, info->origin_pts[3].y,
					info->origin_pts[0].x, info->origin_pts[0].y);
			XDrawLine(XtDisplay(window->shell), off_screen,
					constraint_gc, info->origin_pts[0].x, info->origin_pts[0].y,
					info->origin_pts[2].x, info->origin_pts[2].y);
			XDrawLine(XtDisplay(window->shell), off_screen,
					constraint_gc, info->origin_pts[1].x, info->origin_pts[1].y,
					info->origin_pts[3].x, info->origin_pts[3].y);
			break;

		case line_feature:
			XDrawLine(XtDisplay(window->shell), off_screen,
					constraint_gc, info->origin_pts[0].x, info->origin_pts[0].y,
					info->origin_pts[1].x, info->origin_pts[1].y);
			break;

		case inconsistent_feature:
			XDrawLine(XtDisplay(window->shell), off_screen,
				constraint_gc, info->origin_pts[0].x, info->origin_pts[0].y,
				info->origin_pts[1].x, info->origin_pts[1].y);
			XDrawLine(XtDisplay(window->shell), off_screen,
				constraint_gc, info->origin_pts[2].x, info->origin_pts[2].y,
				info->origin_pts[3].x, info->origin_pts[3].y);
			break;

		case point_feature:
			XDrawArcs(XtDisplay(window->shell), off_screen, constraint_gc,
					  &point_arc, 1);
			break;

		default:;
	}

	for ( i = 0 ; i < info->num_origin_pts ; i++ )
		XDrawRectangle(XtDisplay(window->shell), off_screen,
						spec_gc[info->origin_def_pts[i].type],
						info->origin_def_pts[i].pt.x -
						sced_resources.origin_con_width,
						info->origin_def_pts[i].pt.y -
						sced_resources.origin_con_width,
						sced_resources.origin_con_width << 1,
						sced_resources.origin_con_width << 1);

	if ( copy )
		/* Copy the off screen on. */
		XCopyArea(XtDisplay(window->shell), off_screen,
				  XtWindow(window->view_widget), visible_gc,
				  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
				  area.x, area.y);
}


void
Draw_Scale_Constraints(WindowInfoPtr window, int flags, EditInfoPtr info,
						Boolean copy)
{
	int	i;

	off_screen = window->off_screen;
	area.x = area.y = 0;
	area.width = (short)window->width;
	area.height = (short)window->height;

	if ( info->reference_resulting.f_type == null_feature ) return;

	if ( flags & (CalcView | CalcScreen) )
		Edit_Calculate_Scale_Cons_Points();
	if ( info->reference_resulting.f_type == point_feature )
		Draw_Set_Point_Arc(info->reference, window);

	switch ( info->reference_resulting.f_type )
	{
		case plane_feature:
			XDrawLines(XtDisplay(window->shell), off_screen,
				constraint_gc, info->reference_pts, 4, CoordModeOrigin);
			XDrawLine(XtDisplay(window->shell), off_screen,
			  constraint_gc, info->reference_pts[3].x, info->reference_pts[3].y,
			  info->reference_pts[0].x, info->reference_pts[0].y);
			XDrawLine(XtDisplay(window->shell), off_screen,
			  constraint_gc, info->reference_pts[0].x, info->reference_pts[0].y,
			  info->reference_pts[2].x, info->reference_pts[2].y);
			XDrawLine(XtDisplay(window->shell), off_screen,
			  constraint_gc, info->reference_pts[1].x, info->reference_pts[1].y,
			  info->reference_pts[3].x, info->reference_pts[3].y);
			break;

		case line_feature:
			XDrawLine(XtDisplay(window->shell), off_screen,
			  constraint_gc, info->reference_pts[0].x, info->reference_pts[0].y,
			  info->reference_pts[1].x, info->reference_pts[1].y);
			break;

		case inconsistent_feature:
			XDrawLine(XtDisplay(window->shell), off_screen,
			  constraint_gc, info->reference_pts[0].x, info->reference_pts[0].y,
			  info->reference_pts[1].x, info->reference_pts[1].y);
			XDrawLine(XtDisplay(window->shell), off_screen,
			  constraint_gc, info->reference_pts[2].x, info->reference_pts[2].y,
			  info->reference_pts[3].x, info->reference_pts[3].y);
			break;

		case point_feature:
			XDrawArcs(XtDisplay(window->shell), off_screen, constraint_gc,
					  &point_arc, 1);
			break;

		default:;
	}

	for ( i = 0 ; i < info->num_reference_pts ; i++ )
		XDrawRectangle(XtDisplay(window->shell), off_screen,
						spec_gc[info->reference_def_pts[i].type],
						info->reference_def_pts[i].pt.x -
						sced_resources.scale_con_width,
						info->reference_def_pts[i].pt.y -
						sced_resources.scale_con_width,
						sced_resources.scale_con_width << 1, 
						sced_resources.scale_con_width << 1);

	if ( copy )
		/* Copy the off screen on. */
		XCopyArea(XtDisplay(window->shell), off_screen,
				  XtWindow(window->view_widget), visible_gc,
				  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
				  area.x, area.y);
}


void
Draw_Rotate_Constraints(WindowInfoPtr window, int flags, EditInfoPtr info,
						Boolean copy)
{
	int	i;

	off_screen = window->off_screen;
	area.x = area.y = 0;
	area.width = (short)window->width;
	area.height = (short)window->height;

	if ( ( info->rotate_resulting.f_type != line_feature ) &&
		 ( info->rotate_resulting.f_type != point_feature ) &&
		 ( info->rotate_resulting.f_type != inconsistent_feature ) )
		return;

	if ( flags & (CalcView | CalcScreen) )
	{
		FeaturePtr	temp[2];
		int			num_feats = 0;

		Edit_Calculate_Arc_Points((int)area.width, (int)area.height);
		if ( info->obj->o_major_align.f_type == line_feature)
			temp[num_feats++] = &(info->obj->o_major_align);
		if ( info->obj->o_minor_align.f_type == line_feature)
			temp[num_feats++] = &(info->obj->o_minor_align);
		if ( num_feats )
			Edit_Calculate_Spec_Points(temp, num_feats,
									   &(info->align_def_pts),
									   &(info->num_align_pts));
	}

	/* Draw any rotation constraint. */
	XDrawLines(XtDisplay(window->shell), off_screen,
				constraint_gc, info->arc_points, info->num_arc_pts,
				CoordModeOrigin);

	for ( i = 0 ; i < info->num_align_pts ; i++ )
		XDrawRectangle(XtDisplay(window->shell), off_screen,
						spec_gc[info->align_def_pts[i].type],
						info->align_def_pts[i].pt.x -
						sced_resources.rotate_con_width,
						info->align_def_pts[i].pt.y -
						sced_resources.rotate_con_width,
						sced_resources.rotate_con_width << 1, 
						sced_resources.rotate_con_width << 1);

	if ( copy )
		/* Copy the off screen on. */
		XCopyArea(XtDisplay(window->shell), off_screen,
				  XtWindow(window->view_widget), visible_gc,
				  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
				  area.x, area.y);
}

/*
**	Draws the edges associated with the wireframe wire using vertices vertices
**	with face normals normals.
**	visible_gc is used for front edges and hidden_gc for back edges.
*/
void
Draw_Edges(Display *disp, Drawable draw, GC visible_gc, GC hidden_gc,
		WireframePtr wire, Vertex *vertices, Vector *world_verts,
		Vector *normals, Viewport *viewport) 
{
	int		i, j;
	FacePtr	face;
	GC		gc;
	int		v1, v2, swap;

	Determine_Visible(wire, world_verts, normals, viewport);

	/* Need to ensure that all hidden edges are drawn in the same
	** direction.
	*/
	for ( i = 0 ; i < wire->num_faces ; i++ )
	{
		face = wire->faces + i;
		v1 = face->vertices[0];
		v2 = face->vertices[face->num_vertices - 1];
		if ( wire->faces[i].draw )
			gc = visible_gc;
		else
		{
			gc = hidden_gc;
			if ( v1 > v2 )
				swap = v1, v1 = v2, v2 = swap;
		}
		if ( vertices[v1].view.z > -viewport->eye_distance ||
			 vertices[v2].view.z > -viewport->eye_distance )
			XDrawLine(disp, draw, gc,
					  vertices[v1].screen.x, vertices[v1].screen.y,
					  vertices[v2].screen.x, vertices[v2].screen.y);
		for ( j = 1 ; j < face->num_vertices ; j++ )
		{
			v1 = face->vertices[j];
			v2 = face->vertices[j - 1];
			if ( vertices[v1].view.z < -viewport->eye_distance &&
				 vertices[v2].view.z < -viewport->eye_distance )
				continue;
			if ( ! wire->faces[i].draw && v1 > v2 )
				swap = v1, v1 = v2, v2 = swap;
			XDrawLine(disp, draw, gc,
					  vertices[v1].screen.x, vertices[v1].screen.y,
					  vertices[v2].screen.x, vertices[v2].screen.y);
		}
	}
}


/*
**	Draws only the edges of obj which are visible with respect to viewport.
*/
void
Draw_Visible_Edges(Display *disp, Drawable draw, GC gc, WireframePtr wire,
					Vertex *vertices, Vector *world_verts, Vector *normals,
					Viewport *viewport)
{
	int		i, j;
	FacePtr	face;

	Determine_Visible(wire, world_verts, normals, viewport);

	for ( i = 0 ; i < wire->num_faces ; i++ )
	{
		if ( ! wire->faces[i].draw )
			continue;

		face = wire->faces + i;
		if ( vertices[face->vertices[0]].view.z > -viewport->eye_distance ||
			 vertices[face->vertices[face->num_vertices - 1]].view.z > -viewport->eye_distance )
			XDrawLine(disp, draw, gc,
					  vertices[face->vertices[0]].screen.x,
					  vertices[face->vertices[0]].screen.y,
					  vertices[face->vertices[face->num_vertices-1]].screen.x,
					  vertices[face->vertices[face->num_vertices-1]].screen.y);
		for ( j = 1 ; j < face->num_vertices ; j++ )
		{
			if ( vertices[face->vertices[j]].view.z < -viewport->eye_distance &&
				 vertices[face->vertices[j - 1]].view.z < -viewport->eye_distance )
				continue;
			XDrawLine(disp, draw, gc,
					  vertices[face->vertices[j]].screen.x,
					  vertices[face->vertices[j]].screen.y,
					  vertices[face->vertices[j - 1]].screen.x,
					  vertices[face->vertices[j - 1]].screen.y);
		}
	}
}

void
Draw_Visible_Edges_XOR(Display *disp, Drawable draw, GC gc, WireframePtr wire,
					   Vertex *vertices, Vector *world_verts, Vector *normals,
					   Viewport *viewport)
{
	int		i, j;
	FacePtr	face;
	EdgePtr	edge_list = NULL;
	int		edge_count = 0;

	Determine_Visible(wire, world_verts, normals, viewport);

	for ( i = 0 ; i < wire->num_faces ; i++ )
	{
		if ( ! wire->faces[i].draw )
			continue;

		face = wire->faces + i;
		if ( Edge_Add_Edge(&edge_list, face->vertices[0],
						   face->vertices[face->num_vertices - 1], edge_count)
			 == edge_count )
		{
			if ( vertices[face->vertices[0]].view.z > -viewport->eye_distance ||
				 vertices[face->vertices[face->num_vertices - 1]].view.z > -viewport->eye_distance )
				XDrawLine(disp, draw, gc,
					vertices[face->vertices[0]].screen.x,
					vertices[face->vertices[0]].screen.y,
					vertices[face->vertices[face->num_vertices - 1]].screen.x,
					vertices[face->vertices[face->num_vertices - 1]].screen.y);
			edge_count++;
		}
		for ( j = 1 ; j < face->num_vertices ; j++ )
		{
			if ( Edge_Add_Edge(&edge_list, face->vertices[j],
							 face->vertices[j - 1], edge_count) == edge_count )
			{
				if ( vertices[face->vertices[j]].view.z > -viewport->eye_distance ||
					 vertices[face->vertices[j - 1]].view.z > -viewport->eye_distance )
					continue;
				XDrawLine(disp, draw, gc,
						  vertices[face->vertices[j]].screen.x,
						  vertices[face->vertices[j]].screen.y,
						  vertices[face->vertices[j - 1]].screen.x,
						  vertices[face->vertices[j - 1]].screen.y);
				edge_count++;
			}
		}
	}
}


/*
**	Draws all the edges, regardless of visibility.
*/
void
Draw_All_Edges(Display *disp, Drawable draw, GC gc, WireframePtr wire,
				Vertex *vertices, ViewportPtr viewport)
{
	int			i, j;
	FacePtr		face;

	for ( i = 0 ; i < wire->num_faces ; i++ )
	{
		face = wire->faces + i;
		if ( vertices[face->vertices[0]].view.z > 0 ||
			 vertices[face->vertices[face->num_vertices - 1]].view.z > 0 )
			XDrawLine(disp, draw, gc,
				  vertices[face->vertices[0]].screen.x,
				  vertices[face->vertices[0]].screen.y,
				  vertices[face->vertices[face->num_vertices - 1]].screen.x,
				  vertices[face->vertices[face->num_vertices - 1]].screen.y);
		for ( j = 1 ; j < face->num_vertices ; j++ )
		{
			if ( vertices[face->vertices[j]].view.z < 0 &&
				 vertices[face->vertices[j - 1]].view.z < 0 )
				continue;
			XDrawLine(disp, draw, gc,
					  vertices[face->vertices[j]].screen.x,
					  vertices[face->vertices[j]].screen.y,
					  vertices[face->vertices[j - 1]].screen.x,
					  vertices[face->vertices[j - 1]].screen.y);
		}
	}
}

void
Draw_All_Edges_XOR(Display *disp, Drawable draw, GC gc, WireframePtr wire,
					Vertex *vertices, ViewportPtr viewport)
{
	int		i, j;
	FacePtr	face;
	EdgePtr	edge_list = NULL;
	int		edge_count = 0;

	for ( i = 0 ; i < wire->num_faces ; i++ )
	{
		face = wire->faces + i;
		if ( Edge_Add_Edge(&edge_list, face->vertices[0],
						   face->vertices[face->num_vertices - 1], edge_count)
			 == edge_count )
		{
			if ( vertices[face->vertices[0]].view.z > -viewport->eye_distance ||
				 vertices[face->vertices[face->num_vertices - 1]].view.z >
				 -viewport->eye_distance )
				XDrawLine(disp, draw, gc,
					vertices[face->vertices[0]].screen.x,
					vertices[face->vertices[0]].screen.y,
					vertices[face->vertices[face->num_vertices - 1]].screen.x,
					vertices[face->vertices[face->num_vertices - 1]].screen.y);
			edge_count++;
		}
		for ( j = 1 ; j < face->num_vertices ; j++ )
		{
			if ( Edge_Add_Edge(&edge_list, face->vertices[j],
							 face->vertices[j - 1], edge_count) == edge_count )
			{
				if ( vertices[face->vertices[j]].view.z <
					 -viewport->eye_distance &&
					 vertices[face->vertices[j - 1]].view.z <
					 -viewport->eye_distance )
					continue;
				XDrawLine(disp, draw, gc,
						  vertices[face->vertices[j]].screen.x,
						  vertices[face->vertices[j]].screen.y,
						  vertices[face->vertices[j - 1]].screen.x,
						  vertices[face->vertices[j - 1]].screen.y);
				edge_count++;
			}
		}
	}
}


/*	void
**	Draw_Axes(Display *disp, Drawable draw, ObjectInstance axes);
**	Draws the coordinate axes in the appropriate window.
*/
static void
Draw_Axes(Display *disp, Drawable draw, ObjectInstance axes)
{
	int		x, y, i;
	char	str[2];

	str[1] = '\0';
	for ( i = 0 ; i < 3 ; i++ )
	{
		XDrawLine(disp, draw, axis_gcs[i],
				  axes.o_main_verts[i*2 + 1].screen.x,
				  axes.o_main_verts[i*2 + 1].screen.y,
				  axes.o_main_verts[i*2 + 2].screen.x,
				  axes.o_main_verts[i*2 + 2].screen.y);

		/* Add labels. */
		x = axes.o_main_verts[i*2 + 1].screen.x;
		y = axes.o_main_verts[i*2 + 1].screen.y;
		x += ( x == 0 ? 10 : -10);
		y += ( y == 0 ? 10 : -10);
		str[0] = 'X' + i;
		XDrawString(disp, draw, axis_gcs[i], x, y, str, 1);
	}
}



/*
**	Returns TRUE if face index from wireframe wire is visible wrt viewport.
*/
static Boolean
Is_Visible(WireframePtr wire, Vector *vertices, Vector *normals, short index,
			Viewport *viewport)
{
	Vector		temp_v;

	if ( wire->faces[index].num_vertices == 0 )
		return FALSE;

	VSub(viewport->eye_position, vertices[wire->faces[index].vertices[0]],
		temp_v);
	return ( VDot(temp_v, normals[index]) > 0.0 );
}


/*	void
**	Determine_Visible( ... )
**	Determines which of the faces of obj are visible wrt viewport.
**	Works only for convex objects.
*/
static void
Determine_Visible(WireframePtr wire, Vector *vertices, Vector *normals,
					Viewport *viewport)
{
	int	i;

	/* Set all the faces to not drawn. */
	for ( i = 0 ; i < wire->num_faces ; i++ )
		wire->faces[i].draw = FALSE;

	/* For each face, check to see if it's visible and if so set draw.	*/
	for ( i = 0 ; i < wire->num_faces ; i++ )
		wire->faces[i].draw = Is_Visible(wire, vertices, normals, i, viewport);
}



void
Draw_Initialize()
{
	Pixel		window_foreground, window_background;
	XGCValues  	gc_vals;

	XtVaGetValues(main_window.view_widget,
				XtNforeground, &window_foreground,
				XtNbackground, &window_background, NULL);

	/* Allocate new GC's. */
	gc_vals.function = GXcopy;
	gc_vals.foreground = window_foreground;
	gc_vals.background = window_background;
	visible_gc = XtGetGC(main_window.shell, GCFunction | GCForeground |
						 GCBackground, &gc_vals);

	gc_vals.line_style = LineOnOffDash;
	hidden_gc = XtGetGC(main_window.shell,
		GCFunction | GCForeground |GCBackground |GCLineStyle, &gc_vals);

	gc_vals.foreground = window_background;
	clear_gc = XtGetGC(main_window.shell, GCFunction | GCForeground, &gc_vals);

	gc_vals.foreground = window_foreground;
	gc_vals.background = window_background;

	if (DefaultDepthOfScreen(XtScreen(main_window.shell)) == 1)
	{
		gc_vals.line_width = 2;
		highlight_gc = XtGetGC(main_window.shell,
			GCFunction | GCForeground | GCBackground | GCLineWidth, &gc_vals);

		gc_vals.line_style = LineOnOffDash;
		highlight_hidden_gc = XtGetGC(main_window.shell,
			GCFunction | GCForeground | GCBackground | GCLineStyle, &gc_vals);

		gc_vals.function = GXxor;
		gc_vals.foreground = window_foreground ^ window_background;
		edit_gc = XtGetGC(main_window.shell, GCFunction | GCForeground,
							&gc_vals);

		gc_vals.line_width = sced_resources.obj_axis_width;
		edit_axis_gcs[0] =
		edit_axis_gcs[1] = 
		edit_axis_gcs[2] = XtGetGC(main_window.shell,
				GCFunction | GCForeground | GCBackground | GCLineWidth,
				&gc_vals);

		reference_gc = XtGetGC(main_window.shell,
							GCFunction | GCForeground  | GCBackground,
							&gc_vals);
		gc_vals.line_style = LineOnOffDash;
		circle_gc = XtGetGC(main_window.shell,
					GCFunction | GCForeground  | GCBackground |
					GCLineStyle, &gc_vals);

		origin_stipple =
			XCreateBitmapFromData(XtDisplay(main_window.shell),
						XtWindow(main_window.view_widget), origin_bits,
						origin_width, origin_height);
		gc_vals.fill_style = FillStippled;
		gc_vals.stipple = origin_stipple;
		spec_gc[0] = spec_gc[1] = spec_gc[2] =
		constraint_gc =
		origin_gc = XtGetGC(main_window.shell,
				GCFunction | GCForeground  | GCBackground | GCFillStyle
				| GCStipple, &gc_vals);

		gc_vals.function = GXcopy;
		gc_vals.foreground = window_foreground;
		gc_vals.background = window_background;
		lights_gc = XtGetGC(main_window.shell, GCFunction | GCForeground |
							 GCBackground, &gc_vals);

		gc_vals.line_width = 2;
		lights_highlight_gc = XtGetGC(main_window.shell,
			GCFunction | GCForeground | GCBackground|GCLineWidth, &gc_vals);

		gc_vals.line_style = LineOnOffDash;
		lights_hidden_gc = XtGetGC(main_window.shell,
			GCFunction | GCForeground |GCBackground |GCLineStyle, &gc_vals);
	}
	else
	{
		gc_vals.foreground = sced_resources.selected_color;
		gc_vals.line_width = sced_resources.selected_width;
		highlight_gc = XtGetGC(main_window.shell,
			GCFunction | GCForeground | GCBackground | GCLineWidth, &gc_vals);

		gc_vals.line_style = LineOnOffDash;
		highlight_hidden_gc = XtGetGC(main_window.shell,
			GCFunction | GCForeground | GCBackground | GCLineStyle, &gc_vals);

		gc_vals.function = GXxor;
		gc_vals.foreground = sced_resources.object_color ^ window_background;
		edit_gc = XtGetGC(main_window.shell,
					GCFunction | GCForeground | GCBackground, &gc_vals);

		gc_vals.foreground = sced_resources.scaling_color ^ window_background;
		reference_gc = XtGetGC(main_window.shell,
				GCFunction | GCForeground | GCBackground, &gc_vals);

		gc_vals.foreground = sced_resources.origin_color ^ window_background;
		origin_gc = XtGetGC(main_window.shell,
				GCFunction | GCForeground | GCBackground, &gc_vals);

		gc_vals.foreground =
			sced_resources.constraint_color ^ window_background;
		constraint_gc = XtGetGC(main_window.shell, GCFunction | GCForeground,
								&gc_vals);

		gc_vals.foreground = sced_resources.arcball_color ^ window_background;
		gc_vals.line_style = LineOnOffDash;
		circle_gc = XtGetGC(main_window.shell,
				GCFunction | GCForeground | GCLineStyle | GCBackground,
				&gc_vals);

		gc_vals.foreground = sced_resources.absolute_color ^ window_background;
		spec_gc[0] = XtGetGC(main_window.shell,
				GCFunction | GCForeground | GCBackground, &gc_vals);
		gc_vals.foreground = sced_resources.offset_color ^ window_background;
		spec_gc[1] = XtGetGC(main_window.shell,
				GCFunction | GCForeground | GCBackground, &gc_vals);
		gc_vals.foreground = sced_resources.reference_color ^ window_background;
		spec_gc[2] = XtGetGC(main_window.shell,
				GCFunction | GCForeground | GCBackground, &gc_vals);

		gc_vals.line_width = sced_resources.obj_axis_width;
		gc_vals.foreground =
			sced_resources.obj_x_axis_color ^ window_background;
		edit_axis_gcs[0] = XtGetGC(main_window.shell,
					GCFunction | GCForeground | GCLineWidth, &gc_vals);

		gc_vals.foreground =
			sced_resources.obj_y_axis_color ^ window_background;
		edit_axis_gcs[1] = XtGetGC(main_window.shell,
					GCFunction | GCForeground | GCLineWidth, &gc_vals);

		gc_vals.foreground =
			sced_resources.obj_z_axis_color ^ window_background;
		edit_axis_gcs[2] = XtGetGC(main_window.shell,
					GCFunction | GCForeground | GCLineWidth, &gc_vals);

		gc_vals.foreground = sced_resources.light_color;
		lights_gc = XtGetGC(main_window.shell, GCForeground, &gc_vals);

		gc_vals.line_style = LineOnOffDash;
		lights_hidden_gc = XtGetGC(main_window.shell,
			GCForeground | GCLineStyle, &gc_vals);

		gc_vals.foreground = sced_resources.selected_color;
		gc_vals.line_width = sced_resources.selected_width;
		lights_highlight_gc = XtGetGC(main_window.shell,
			GCForeground | GCLineWidth, &gc_vals);

	}

	draw_info = Edit_Get_Info();
}
