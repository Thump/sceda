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
**	selection.c : Functions dealing with object selection.
**
**	External functions:
**
*/


#include <sced.h>
#include <instance_list.h>
#include <View.h>

static InstanceList Select_Objects_By_Area(InstanceList, XPoint,XPoint,Boolean);
static Boolean Object_In_Rectangle(WireframePtr, Vertex*, XPoint, XPoint);
static Boolean Edge_In_Rectangle(XPoint, XPoint, XPoint, XPoint);

void Free_Selection_List(InstanceList);


/* Static variables used to hold values between action procedure calls. */
static WindowInfoPtr	window;
static XPoint		start_pt;
static XPoint		rect_top_left;
static unsigned		rect_width;
static unsigned		rect_height;
static GC			rectangle_gc = NULL;
static Boolean		in_progress = FALSE;



/*	void
**	Start_Selection_Drag(Widget, XEvent*, String*, Cardinal*)
**	Begins an object selection drag sequence.  Basically just sets the
**	start point.
*/
void
Start_Selection_Drag(Widget w, XEvent *event, String *params, Cardinal *num_p)
{
	Pixel		window_foreground;
	Pixel		window_background;
	XGCValues  	gc_vals;
	
	if ( w == main_window.view_widget )
		window = &main_window;
	else
		window = &csg_window;

	/* Get data from the widget. */
	XtVaGetValues(w, XtNforeground, &window_foreground,
					 XtNbackground, &window_background, NULL);

	/*
	**	Get the start point.
	*/
	start_pt.x = event->xbutton.x;
	start_pt.y = event->xbutton.y;

	/* Set the finish point to be the same as the start point.  This makes
	** the drag rectangle drawing uniform.
	*/
	rect_top_left = start_pt;
	rect_width = rect_height = 0;

	if ( ! rectangle_gc )
	{
		/* Set the GC for drawing the rectangle. */
		gc_vals.function = GXxor;
		gc_vals.foreground = window_foreground ^ window_background;  
		rectangle_gc = XtGetGC(window->shell, GCFunction | GCForeground,
							   &gc_vals);
	}

	/* Draw the rectangle. */
	XDrawRectangle(XtDisplay(window->shell), XtWindow(w), rectangle_gc,
					rect_top_left.x, rect_top_left.y,
					rect_width, rect_height);

	in_progress = TRUE;

}



/*	void
**	Continue_Selection_Drag(Widget, XEvent*, String*, Cardinal*)
**	Continues an object selection drag sequence.
**	Undraws the previous rectangle and draws a new one.
*/
void
Continue_Selection_Drag(Widget w, XEvent *event, String *params,
						Cardinal *num_p)
{
	if (!in_progress) return;

	/* Undraw the old rectangle. */
	XDrawRectangle(XtDisplay(window->shell), XtWindow(w), rectangle_gc,
					rect_top_left.x, rect_top_left.y,
					rect_width, rect_height);

	/* Set the rectangle parameters. */
	rect_top_left.x = min(start_pt.x, event->xbutton.x);
	rect_top_left.y = min(start_pt.y, event->xbutton.y);
	rect_width = (unsigned)max(start_pt.x - rect_top_left.x,
								event->xbutton.x - rect_top_left.x);
	rect_height = (unsigned)max(start_pt.y - rect_top_left.y,
								event->xbutton.y - rect_top_left.y);

	/* Draw the new rectangle. */
	XDrawRectangle(XtDisplay(window->shell), XtWindow(w), rectangle_gc,
					rect_top_left.x, rect_top_left.y,
					rect_width, rect_height);


}


/*	void
**	Finish_Selection_Drag(Widget, XEvent*, String*, Cardinal*)
**	Begins an object selection drag sequence.
**	Undraws the rectangle, then calls the selecting procedure.
*/
void
Finish_Selection_Drag(Widget w, XEvent *event, String *params, Cardinal *num_p)
{
	InstanceList	selected_objects;
	InstanceList	old_selected_instances;
	XPoint			finish_pt;
	Boolean			adding;

	if (!in_progress) return;

	/* Figure out whether we are adding to or deleting from the list. */
	if (*num_p != 1)
	{
		fprintf(stderr,
			"Obscene: Finish_Selection_Drag called with missing argument\n");
		return;
	}
	else
	{
		if (!strcmp("add", params[0]))
			adding = TRUE;
		else if (!strcmp("delete", params[0]))
			adding = FALSE;
		else
		{
			fprintf(stderr,
			"Obscene: Finish_Selection_Drag called with invalid argument\n");
			return;
		}
	}

	/* Set the finish point. */
	finish_pt.x = max(start_pt.x, event->xbutton.x);
	finish_pt.y = max(start_pt.y, event->xbutton.y);
	start_pt.x = min(start_pt.x, event->xbutton.x);
	start_pt.y = min(start_pt.y, event->xbutton.y);

	/* Check to see whether the start point and finish point coincide. */
	/* At this stage, set the rectangle to be a defined constant. */
	if ((start_pt.x == finish_pt.x) && (start_pt.y == finish_pt.y))
	{
		start_pt.x -= SELECTION_CLICK_WIDTH / 2;
		finish_pt.x += SELECTION_CLICK_WIDTH / 2;
		start_pt.y -= SELECTION_CLICK_WIDTH / 2;
		finish_pt.y += SELECTION_CLICK_WIDTH / 2;
	}

	/* Select from the available objects, returning a list of selected ones. */
	selected_objects =
	Select_Objects_By_Area(window->all_instances, start_pt, finish_pt, adding);

	old_selected_instances = window->selected_instances;
	if (adding)
		window->selected_instances =
			Merge_Selection_Lists(old_selected_instances, selected_objects);
	else
		window->selected_instances =
			Remove_Selection_List(old_selected_instances, selected_objects);
	Free_Selection_List(old_selected_instances);
	Free_Selection_List(selected_objects);

	/* Undraw the old rectangle. */
	/* With the update that follows this is really unnecessary. */
	XDrawRectangle(XtDisplay(window->shell), XtWindow(w), rectangle_gc,
					rect_top_left.x, rect_top_left.y,
					rect_width, rect_height);

	/* Update all the objects on the screen to get/unget highlights. */
	/* Will also clear any junk left after drawing the drag rectangle. */
	View_Update(window, window->all_instances, ViewNone);

	in_progress = FALSE;
}


/*	InstanceList
**	Select_Objects_By_Area(InstanceList available, XPoint min, XPoint max,
**							Boolean select)
**	Selects objects be checking for edges passing through a given rectangular
**	area.  Objects are chosen from the available list.  The region is defined
**	by min and max points.
*/
static InstanceList
Select_Objects_By_Area(InstanceList available, XPoint min, XPoint max,
					   Boolean select)
{
	InstanceList	result = NULL;
	InstanceList	elmt;

	for ( elmt = available ; elmt != NULL ; elmt = elmt->next )
	{
		if ( ! ( elmt->the_instance->o_flags & ObjVisible ) )
			continue;

		if (!Check_Rectangle_Intersection(elmt->the_instance->o_proj_extent.min,
										  elmt->the_instance->o_proj_extent.max,
										  min, max))
			continue;

		if ( Object_In_Rectangle(elmt->the_instance->o_wireframe,
								 elmt->the_instance->o_main_verts, min, max) )
		{
			if ( select )
				elmt->the_instance->o_flags |= ObjSelected;
			else
				elmt->the_instance->o_flags &= ( ObjAll ^ ObjSelected );
			Insert_Element(&result, elmt->the_instance);
		}
	}

	return result;
}




/*
**	Returns True if any part of any edge in edges passes through the
**	rectangle defined by min and max.
*/
static Boolean
Object_In_Rectangle(WireframePtr wire, Vertex *vertices, XPoint min, XPoint max)
{
	int	i, j;

	for ( i = 0 ; i < wire->num_faces ; i++ )
	{
		if ( ( vertices[wire->faces[i].vertices[0]].view.z > 0 ||
			   vertices[wire->faces[i].vertices[1]].view.z > 0 ) &&
			 Edge_In_Rectangle(vertices[wire->faces[i].vertices[0]].screen,
							   vertices[wire->faces[i].vertices[
									wire->faces[i].num_vertices - 1]].screen,
							   min, max) )
			return TRUE;
		for ( j = 1 ; j < wire->faces[i].num_vertices ; j++ )
			if ( ( vertices[wire->faces[i].vertices[j]].view.z > 0 ||
				   vertices[wire->faces[i].vertices[j-1]].view.z > 0 ) &&
				 Edge_In_Rectangle(vertices[wire->faces[i].vertices[j]].screen,
								  vertices[wire->faces[i].vertices[j-1]].screen,
								  min, max) )
			return TRUE;
	}

	return FALSE;
}


/*	Boolean
**	Edge_In_Rectangle(XPoint p, XPoint q, XPoint min, XPoint max)
**	Returns TRUE if any part of edge p-q intersects rectangle min-max.
*/
static Boolean
Edge_In_Rectangle(XPoint p, XPoint q, XPoint min, XPoint max)
{
	register int	p_row, p_colm;
	register int	q_row, q_colm;
	int				x_diff, y_diff, constant;


	/*
	**	The theory goes something like this:
	**
	**	Divide the region around the rectangle into 8 areas, and label them:
	**
	**        1,1    |   1,2   |    1,3
	**               |         |
	**       ----------------------------
	**	             |         |
	**        2,1    |   2,2   |    2,3
	**               |         |
	**       ----------------------------
	**               |         |
	**        3,1    |   3,2   |    3,3
	**               |         |
	**
	**	Then, a line joining 2 points in certain regions cannot intersect the
	**	rectangle.  This allows most cases to be eliminated quickly.
	**	This is all explained as we go. (Yeah sure :-))
	*/

#define Get_Third(pt, b1, b2, res) \
	if (pt < b1)		res = 1; \
	else if (pt <= b2)	res = 2; \
	else				res = 3

	Get_Third(p.x, min.x, max.x, p_colm);
	Get_Third(p.y, min.y, max.y, p_row);
	Get_Third(q.x, min.x, max.x, q_colm);
	Get_Third(q.y, min.y, max.y, q_row);

#undef Get_Third

	/* If either of the points is inside, it intersects. */
	if ((p_row == 2 && p_colm == 2) || (q_row == 2 && q_colm == 2))
		return TRUE;

	/* If both points lie on the same side of the rectangle they're out. */
	if ((p_row == 1 && q_row == 1) || (p_colm == 1 && q_colm == 1) ||
		(p_row == 3 && q_row == 3) || (p_colm == 3 && q_colm == 3))
		return FALSE;

	/* Now get lines lying in the "cross" through the center. */
	if ((p_row == 2 && q_row == 2) || (p_colm == 2 && q_colm == 2))
		return TRUE;

	/*	Anything that's left is a bit harder to catch.  They are point
	**	combinations which may or may not leed to intersections.
	**	I decide by testing whether both corners of the rectangle are on the
	**	same side of the line joining p-q.
	**
	**	I am doing more integer multipies rather than floating point
	**	multiplies and divides in the hope it will be faster.
	*/
	x_diff = p.x - q.x;
	y_diff = p.y - q.y;
	constant = x_diff * q.y - y_diff * q.x;
	
	if (((y_diff * min.x - x_diff * min.y + constant < 0) !=
		 (y_diff * max.x - x_diff * max.y + constant < 0)) ||
		((y_diff * min.x - x_diff * max.y + constant < 0) !=
		 (y_diff * max.x - x_diff * min.y + constant < 0)))
		return TRUE;

	return FALSE;

}



