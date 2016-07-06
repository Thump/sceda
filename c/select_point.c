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
**	select_point.c : Functions to select individual vertices.
*/

#include <math.h>
#include <sced.h>
#include <select_point.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>


static void	Select_Calc_Highlight(XPoint);
static void Select_Apply_Callback(Widget, XtPointer, XtPointer);
static void	Select_Point_Apply_Absolute(Widget, XtPointer, XtPointer);
static void	Select_Apply_Create_Dialog();


/* The window to do it in. */
WindowInfoPtr	select_window;

/* The vertices to select from. */
int					num_select_verts;
SelectPointStruct	*select_verts;

/* The number required.  When this number reaches 0, the callback will be
** invoked.  If 0 already the function will exit immediately.
*/
int		num_verts_required;
Boolean	specs_allowed[reference_spec + 1];
Vector	select_center;

/* The callback to invoke when all the points have been selected.	*/
SelectPointCallbackProc	select_finished_callback;

/* Whether to allow text entry of points. */
Boolean	allow_text_entry;

/* Whether to perform highlights and reference cycling. */
Boolean	select_highlight;

static int				chosen_pts[3];
static FeatureSpecType	chosen_specs[3];
static int				num_chosen = 0;

static int		highlight_pt;
static double	closest_dist;

/* GCs for selected and unselected points. */
static Boolean	initialized = FALSE;
static GC		selected_gc;
static GC		ready_gc;
static GC		highlight_gc;
static Cursor	select_cursor;

static char	orig_string[ENTRY_STRING_LENGTH];
static char	orig_label[ENTRY_STRING_LENGTH];

static ObjectInstancePtr	extra_obj;
static int					extra_vert_index;

#define stipple_width 2
#define stipple_height 2
static char stipple_bits[] = {
   0x01, 0x02};
static Pixmap	stippled_pixmap = 0;

static Widget	select_apply_dialog = NULL;

/*	void
**	Select_Point_Action(Widget w, XEvent *e, String *s, Cardinal *n)
**	An action procedure designed to be called for button events.  It simply
**	invokes Select_Point().
*/
void
Select_Point_Action(Widget w, XEvent *e, String *s, Cardinal *n)
{
	XPoint	pt;
	FeatureSpecType type = absolute_spec;

	pt.x = e->xbutton.x;
	pt.y = e->xbutton.y;

	switch ( e->xbutton.button )
	{
		case Button1:
			type = reference_spec;
			break;
		case Button2:
			type = offset_spec;
			break;
		case Button3:
			type = absolute_spec;
			break;
	}

	Select_Point(pt, type);
}


static Boolean
Select_Cycle_References(Boolean ignore)
{
	static short	event_count = 0;
	Boolean	found;
	int		new_highlight = 0; 	/* Initialise to stop compiler warnings. */
	int		i;

	if ( ! select_highlight ) return FALSE;

	/* to smooth things out a bit, only process
	** every fifth event.
	*/
	if ( ignore && event_count++ < 3 ) return FALSE;
	event_count = 0;

	/* Find the next common point. */
	for ( i = highlight_pt + 1, found = FALSE ;
		  ! found && i != highlight_pt ;
		  i = ( i < num_select_verts - 1 ) ? i + 1 : 0 )
	{
		if ( select_verts[highlight_pt].vert.x == select_verts[i].vert.x &&
			 select_verts[highlight_pt].vert.y == select_verts[i].vert.y &&
			 ! ( select_verts[i].obj->o_flags & ObjDepends ) )
		{
			new_highlight = i;
			found = TRUE;
		}
	}

	if ( ! found ) return FALSE;

	/* Unhighlight the currently highlighted. */
	Draw_All_Edges_XOR(XtDisplay(select_window->shell),
					   XtWindow(select_window->view_widget),
					   highlight_gc,
					   select_verts[highlight_pt].obj->o_wireframe,
					   select_verts[highlight_pt].obj->o_main_verts,
					   &(select_window->viewport));

	highlight_pt = new_highlight;

	/* Highlight it. */
	Draw_All_Edges_XOR(XtDisplay(select_window->shell),
					   XtWindow(select_window->view_widget),
					   highlight_gc,
					   select_verts[highlight_pt].obj->o_wireframe,
					   select_verts[highlight_pt].obj->o_main_verts,
					   &(select_window->viewport));

	return TRUE;
}


/*	void
**	Select_Highlight_Object(Widget w, XEvent *e, String *s, Cardinal *n)
**	Highlights the referenced object, if any.
*/
void
Select_Highlight_Object(Widget w, XEvent *e, String *s, Cardinal *n)
{
	if ( ! select_highlight || e->xbutton.button != Button1 )
		return;

	if ( ( select_verts[highlight_pt].obj->o_flags & ObjDepends ) &&
		 ! Select_Cycle_References(FALSE) )
		return;

	/* Simply draw the object referenced by the highlight point in a different
	** colour.
	*/
	Draw_All_Edges_XOR(XtDisplay(select_window->shell),
					   XtWindow(select_window->view_widget),
					   highlight_gc,
					   select_verts[highlight_pt].obj->o_wireframe,
					   select_verts[highlight_pt].obj->o_main_verts,
					   &(select_window->viewport));
}



/*	void
**	Select_Highlight_Action(Widget w, XEvent *e, String *s, Cardinal *n)
**	Highlights the point closest to the pointer.
*/
void
Select_Highlight_Action(Widget w, XEvent *e, String *s, Cardinal *n)
{
	XPoint		pt;
	XRectangle	rect;

	if ( e->xmotion.state & ( Button2Mask | Button3Mask ) ||
		 num_select_verts == 0 )
		return;

	if ( e->xmotion.state & Button1Mask )
	{
		Select_Cycle_References(TRUE);
		return;
	}

	rect.width = rect.height = sced_resources.select_pt_width;

	/* Undraw the old one. */
	rect.x = select_verts[highlight_pt].vert.x -
			 sced_resources.select_pt_width / 2;
	rect.y = select_verts[highlight_pt].vert.y -
			 sced_resources.select_pt_width / 2;
	if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
		XFillRectangles(XtDisplay(select_window->shell),
						XtWindow(select_window->view_widget),
						ready_gc, &rect, 1);
	else
		XDrawRectangles(XtDisplay(select_window->shell),
						XtWindow(select_window->view_widget),
						ready_gc, &rect, 1);

	pt.x = e->xmotion.x;
	pt.y = e->xmotion.y;

	Select_Calc_Highlight(pt);

	/* Draw the new one. */
	rect.x = select_verts[highlight_pt].vert.x -
			 sced_resources.select_pt_width / 2;
	rect.y = select_verts[highlight_pt].vert.y -
			 sced_resources.select_pt_width / 2;
	if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
		XFillRectangles(XtDisplay(select_window->shell),
						XtWindow(select_window->view_widget),
						ready_gc, &rect, 1);
	else
		XDrawRectangles(XtDisplay(select_window->shell),
						XtWindow(select_window->view_widget),
						ready_gc, &rect, 1);
}


/*	Calculates the closest point for highlighting. Doesn't actually draw it.
*/
void
Select_Highlight_Closest(Window window)
{
    int				x, y;
    Window			root, child;
    int				dummyx, dummyy;
    unsigned int	dummymask;
	XPoint			pt;

    XQueryPointer(XtDisplay(select_window->view_widget), window,
				  &root, &child, &x, &y, &dummyx, &dummyy, &dummymask);

	pt.x = x; pt.y = y;

	Select_Calc_Highlight(pt);
}


/*	I lied. This one actually does the calculation. */
static void
Select_Calc_Highlight(XPoint pt)
{
	int		x_dif, y_dif;
	double	temp_dist;
	int		i;

	x_dif = pt.x - select_verts[0].vert.x;
	y_dif = pt.y - select_verts[0].vert.y;
	closest_dist = x_dif * x_dif + y_dif * y_dif;
	highlight_pt = 0;

	for ( i = 1 ; i < num_select_verts ; i++ )
	{
		x_dif = pt.x - select_verts[i].vert.x;
		y_dif = pt.y - select_verts[i].vert.y;
		temp_dist = x_dif * x_dif + y_dif * y_dif;
		if ( temp_dist < closest_dist )
		{
			closest_dist = temp_dist;
			highlight_pt = i;
		}
	}
}


/*	void
**	Select_Point(XPoint pt, FeatureSpecType nature)
**	Called to select a point.  Checks pt against all the available point
**	rectangles and if it matches, adds that point to those selected.
**	It won't add certain points, such as three co-linear pts.
*/
void
Select_Point(XPoint pt, FeatureSpecType nature)
{
	XPoint		min, max;
	XRectangle	rect;
	int			i, j;
	Boolean		chosen;
	int			temp_i = ( sced_resources.select_pt_width +
						   sced_resources.select_pt_line_width ) / 2;

	if ( nature == reference_spec &&
		 select_verts[highlight_pt].obj &&
		 select_verts[highlight_pt].obj->o_flags & ObjDepends )
		return;

	if ( select_highlight && nature == reference_spec )
		Draw_All_Edges_XOR(XtDisplay(select_window->shell),
						   XtWindow(select_window->view_widget),
						   highlight_gc,
						   select_verts[highlight_pt].obj->o_wireframe,
						   select_verts[highlight_pt].obj->o_main_verts,
						   &(select_window->viewport));

	if ( ! specs_allowed[nature] )
		return;

	/* Search for a match. Take first found. */
	/* Check the click was inside the highlight pt. */
	min.x = select_verts[highlight_pt].vert.x - temp_i;
	max.x = select_verts[highlight_pt].vert.x + temp_i;
	min.y = select_verts[highlight_pt].vert.y - temp_i;
	max.y = select_verts[highlight_pt].vert.y + temp_i;
	if ( ! Point_In_Rect( pt, min, max ))
		return;

	/* If there are three points, check if the third is co-linear. */
	if ( num_chosen == 2 &&
		 Points_Colinear(select_verts[chosen_pts[0]].
				obj->o_world_verts[select_verts[chosen_pts[0]].offset],
		 		select_verts[chosen_pts[1]].
				obj->o_world_verts[select_verts[chosen_pts[1]].offset],
		 		select_verts[highlight_pt].obj->
					o_world_verts[select_verts[highlight_pt].offset] ) )
		return;

	/* Look for the point already chosen. */
	for ( i = 0, chosen = FALSE ; ! chosen && i < num_chosen ; i++ )
		chosen = ( chosen_pts[i] == highlight_pt );

	if ( chosen )
	{
		for ( j = i ; j < num_chosen ; j++ )
		{
			chosen_pts[j-1] = chosen_pts[j];
			chosen_specs[j-1] = chosen_specs[j];
		}
		num_chosen--;
	}
	else
	{
		chosen_pts[num_chosen] = highlight_pt;
		chosen_specs[num_chosen++] = nature;
	}

	rect.width = rect.height = sced_resources.select_pt_width;
	rect.x = select_verts[highlight_pt].vert.x -
			 sced_resources.select_pt_width / 2;
	rect.y = select_verts[highlight_pt].vert.y -
			 sced_resources.select_pt_width / 2;

	/* Change the appearance of the selected vertex. */
	if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
	{
		XFillRectangles(XtDisplay(select_window->shell),
						XtWindow(select_window->view_widget),
						ready_gc, &rect, 1);
		XFillRectangles(XtDisplay(select_window->shell),
						XtWindow(select_window->view_widget),
						selected_gc, &rect, 1);
	}
	else
	{
		XDrawRectangles(XtDisplay(select_window->shell),
						XtWindow(select_window->view_widget),
						selected_gc, &rect, 1);
	}

	if ( num_chosen == num_verts_required )
	{
		/* Undraw all the reference points. */
		Draw_Selection_Points(XtWindow(select_window->view_widget));

		num_chosen = 0;

		/* Call the function. */
		select_finished_callback(chosen_pts, chosen_specs);

	}

}



/*	void
**	Select_Calc_Screen()
**	Recalculates the screen vertices. Actually just copies them again.
*/
void
Select_Calc_Screen()
{
	int	i;

	for ( i = 0 ; i < num_select_verts ; i++ )
		select_verts[i].vert =
			select_verts[i].obj->o_main_verts[select_verts[i].offset].screen;
}



/*	void
**	Draw_Selection_Points(Drawable draw)
**	Draws all the selection points.
*/
void
Draw_Selection_Points(Drawable draw)
{
	XRectangle	rect;
	int			i;

	rect.width = rect.height = sced_resources.select_pt_width;

	for ( i = 0 ; i < num_chosen ; i++ )
	{
		if ( chosen_pts[i] >= num_select_verts ) continue;

		rect.x = select_verts[chosen_pts[i]].vert.x -
				 sced_resources.select_pt_width / 2;
		rect.y = select_verts[chosen_pts[i]].vert.y -
				 sced_resources.select_pt_width / 2;

		if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
			XFillRectangles(XtDisplay(select_window->shell), draw,
							selected_gc, &rect, 1);
		else
			XDrawRectangles(XtDisplay(select_window->shell), draw,
							selected_gc, &rect, 1);
	}

	rect.x = select_verts[highlight_pt].vert.x -
			 sced_resources.select_pt_width / 2;
	rect.y = select_verts[highlight_pt].vert.y -
			 sced_resources.select_pt_width / 2;
	if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
		XFillRectangles(XtDisplay(select_window->shell), draw,
						ready_gc, &rect, 1);
	else
		XDrawRectangles(XtDisplay(select_window->shell), draw,
						ready_gc, &rect, 1);
}

/*
static int
Select_Vert_Comparison_Function(const void *a, const void *b)
{
	SelectPointStruct *v1 = (SelectPointStruct*)a;
	SelectPointStruct *v2 = (SelectPointStruct*)b;

	if ( v1->vert.x < v2->vert.x )
		return -1;

	if ( v1->vert.x > v2->vert.x )
		return 1;

	if ( v1->vert.y < v2->vert.y )
		return -1;

	if ( v1->vert.y > v2->vert.y )
		return 1;

	return 0;
}
*/


/*	void
**	Build_Select_Verts_From_List(InstanceList list)
**	Builds up the list of select vertices from the given instance list.
*/
void
Build_Select_Verts_From_List(InstanceList list)
{
	InstanceList	elmt;
	int				j;

	num_select_verts = 0;
	for ( elmt = list ; elmt ; elmt = elmt->next )
	{
		if ( ! ( elmt->the_instance->o_flags & ObjVisible ) )
			continue;
		num_select_verts += elmt->the_instance->o_num_vertices;
	}

	select_verts = New(SelectPointStruct,  num_select_verts); 
	for ( num_select_verts = 0, elmt = list ; elmt ; elmt = elmt->next )
	{
		if ( ! ( elmt->the_instance->o_flags & ObjVisible ) )
			continue;
		for ( j = 0 ; j < elmt->the_instance->o_num_vertices ; j++ )
		{
			if ( elmt->the_instance->o_main_verts[j].view.z < 0 )
				continue;
			select_verts[num_select_verts].vert =
				elmt->the_instance->o_main_verts[j].screen;
			select_verts[num_select_verts].obj = elmt->the_instance;
			select_verts[num_select_verts].offset = j;
			num_select_verts++;
		}
	}
}


void
Prepare_Selection_Drawing()
{
	Colormap	col_map;
	Pixel		foreground_pixel, background_pixel;
	XGCValues	gc_vals;
	char		*tmp;

	if ( ! initialized )
	{
		/* Get the colormap. */
		XtVaGetValues(select_window->view_widget,
					XtNcolormap, &col_map,
					XtNforeground, &foreground_pixel,
					XtNbackground, &background_pixel, NULL);

		gc_vals.function = GXxor;

		if ( DefaultDepthOfScreen(XtScreen(select_window->shell) ) == 1)
		{
			gc_vals.foreground = foreground_pixel ^ background_pixel;
			ready_gc = XtGetGC(select_window->shell, GCFunction | GCForeground,
								&gc_vals);

			gc_vals.line_width = 3;
			highlight_gc = XtGetGC(select_window->shell,
								   GCFunction | GCForeground | GCLineWidth,
								   &gc_vals);


			if ( ! stippled_pixmap )
				stippled_pixmap =
					XCreateBitmapFromData(XtDisplay(select_window->shell),
							XtWindow(select_window->view_widget), stipple_bits,
							stipple_width, stipple_height);
			gc_vals.fill_style = FillStippled;
			gc_vals.stipple = stippled_pixmap;
			selected_gc = XtGetGC(select_window->shell,
					GCFunction | GCForeground | GCFillStyle | GCStipple,
					&gc_vals);
		}
		else
		{
			/* Allocate the GCs. */
			gc_vals.line_width = sced_resources.select_pt_line_width;
			gc_vals.foreground =
				sced_resources.selected_pt_color ^ background_pixel;
			selected_gc = XtGetGC(select_window->shell,
							GCFunction | GCForeground | GCLineWidth, &gc_vals);

			gc_vals.foreground = sced_resources.active_color ^ background_pixel;
			ready_gc = XtGetGC(select_window->shell,
							GCFunction | GCForeground | GCLineWidth, &gc_vals);

			gc_vals.foreground =
				sced_resources.referenced_color ^ foreground_pixel;
			highlight_gc = XtGetGC(select_window->shell,
								   GCFunction | GCForeground, &gc_vals);
		}

		extra_obj = New(ObjectInstance, 1);
		extra_obj->o_world_verts = New(Vector, 3);
		extra_obj->o_num_vertices = 0;

		select_cursor = XCreateFontCursor(XtDisplay(select_window->shell),
										  XC_hand2);

		initialized = TRUE;
	}

	/* Save some resources: the text label and the string. */
	if ( allow_text_entry )
	{
		XtVaGetValues(select_window->text_label, XtNlabel, &tmp, NULL);
		strcpy(orig_label, tmp);
		strcpy(orig_string, select_window->text_string);

		/* Update the apply and text stuff. */
		Set_Prompt(select_window, "");

		XtVaSetValues(select_window->text_label, XtNlabel, "Point:", NULL);

		XtRemoveCallback(select_window->apply_button, XtNcallback,
							Apply_Button_Callback, NULL);
		XtAddCallback(select_window->apply_button, XtNcallback,
						Select_Apply_Callback, NULL);
	}

	extra_vert_index = 0;

	XDefineCursor(XtDisplay(select_window->shell),
				  XtWindow(select_window->view_widget), select_cursor);
}


void
Cleanup_Selection()
{
	/* Update the apply and text stuff. */
	if ( allow_text_entry )
	{
		Dimension	width;
		Set_Prompt(select_window, orig_string);
		XtVaGetValues(select_window->apply_button, XtNwidth, &width, NULL);
		XtVaSetValues(select_window->text_label,
					  XtNwidth, width, XtNlabel, orig_label, NULL);
		XtRemoveCallback(select_window->apply_button, XtNcallback,
							Select_Apply_Callback, NULL);
		XtAddCallback(select_window->apply_button, XtNcallback,
						Apply_Button_Callback, NULL);
	}

	free(select_verts);
	num_select_verts = 0;
	num_chosen = 0;

	XDefineCursor(XtDisplay(select_window->shell),
				  XtWindow(select_window->view_widget), None);
}


static void
Select_Apply_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	Vector	diff;
	int	i;

	if ( ! allow_text_entry ) return;

	if (  sscanf(select_window->text_string, "%lf %lf %lf",
			&(extra_obj->o_world_verts[extra_vert_index].x),
			&(extra_obj->o_world_verts[extra_vert_index].y),
			&(extra_obj->o_world_verts[extra_vert_index].z) )  != 3 )
		return;

	/* Check for coincident points. */
	for ( i = 0 ; i < num_chosen ; i++ )
		if ( VEqual(select_verts[chosen_pts[i]].
					obj->o_world_verts[select_verts[chosen_pts[i]].offset],
					extra_obj->o_world_verts[extra_vert_index], diff) )
			return;

	/* Need to add a new elmt to hold the vertex. */
	select_verts = More(select_verts, SelectPointStruct, num_select_verts +
						extra_vert_index + 1);
	select_verts[num_select_verts + extra_vert_index].obj = extra_obj;
	select_verts[num_select_verts + extra_vert_index].offset = extra_vert_index;

	/* If there are three points, check if the third is co-linear. */
	if ( num_chosen == 2 &&
		 Points_Colinear(select_verts[chosen_pts[0]].
					obj->o_world_verts[select_verts[chosen_pts[0]].offset],
		 			select_verts[chosen_pts[1]].
					obj->o_world_verts[select_verts[chosen_pts[1]].offset],
		 			extra_obj->o_world_verts[extra_vert_index] ))
		return;

	chosen_pts[num_chosen] = num_select_verts + extra_vert_index;
	extra_vert_index++;

	if ( ! specs_allowed[offset_spec] )
	{
		Select_Point_Apply_Absolute(NULL, NULL, NULL);
		return;
	}

	if ( ! select_apply_dialog )
		Select_Apply_Create_Dialog();

	SFpositionWidget(select_apply_dialog);
	XtPopup(select_apply_dialog, XtGrabExclusive);
}


static void
Select_Point_Apply_Absolute(Widget w, XtPointer cl, XtPointer ca)
{
	if ( w )
		XtPopdown(select_apply_dialog);

	chosen_specs[num_chosen++] = absolute_spec;

	Set_Prompt(select_window, "");

	if ( num_chosen == num_verts_required )
	{
		/* Undraw all the reference points. */
		Draw_Selection_Points(XtWindow(select_window->view_widget));

		num_chosen = 0;

		/* Call the function. */
		select_finished_callback(chosen_pts, chosen_specs);
	}

}


static void
Select_Point_Apply_Offset(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(select_apply_dialog);

	chosen_specs[num_chosen++] = offset_spec;

	VAdd(extra_obj->o_world_verts[extra_vert_index-1], select_center,
		 extra_obj->o_world_verts[extra_vert_index-1]);

	Set_Prompt(select_window, "");

	if ( num_chosen == num_verts_required )
	{
		/* Undraw all the reference points. */
		Draw_Selection_Points(XtWindow(select_window->view_widget));

		num_chosen = 0;

		/* Call the function. */
		select_finished_callback(chosen_pts, chosen_specs);
	}

}

static void
Select_Apply_Create_Dialog()
{
	Widget	dialog;
	Arg		args[5];
	int		n;

	n = 0;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	XtSetArg(args[n], XtNtitle, "Point Type");		n++;
	select_apply_dialog = XtCreatePopupShell("pointTypeShell",
							transientShellWidgetClass, select_window->shell,
							args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Point Type?");	n++;
	dialog = XtCreateManagedWidget("pointTypeDialog", dialogWidgetClass,
									select_apply_dialog, args, n);

	XawDialogAddButton(dialog, "Absolute", Select_Point_Apply_Absolute, NULL);
	XawDialogAddButton(dialog, "Offset", Select_Point_Apply_Offset, NULL);

	XtVaSetValues(XtNameToWidget(dialog, "label"), XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(dialog, "Absolute"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(dialog, "Offset"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(select_apply_dialog);
}

