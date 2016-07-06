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
**	SimpleWire.c : C file for the SimpleWire widget class.
**
**	Created: 19/03/94
*/

#include <X11/IntrinsicP.h>
#include <math.h>
#include <sced.h>
#include <SimpleWireP.h>

static XtResource resources[] = {
#define offset(field) XtOffsetOf(SimpleWireRec, simpleWire.field)
    /* {name, class, type, size, offset, default_type, default_addr}, */
    { XtNbasePtr, XtCValue, XtRPointer, sizeof(XtPointer),
      offset(base_ptr), XtRPointer, NULL },
	{ XtNforeground, XtCColor, XtRPixel, sizeof(Pixel), offset(drawing_color),
	  XtRString, XtDefaultForeground },
	{ XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct*), offset(font),
	  XtRString, XtDefaultFont },
	{ XtNcallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	  offset(select_callback), XtRCallback, NULL }
#undef offset
};


/* Prototypes for various procedures. */
static void InitializeSimpleWire(Widget request, Widget new, ArgList args,
								 Cardinal *num_args);
static void DestroySimpleWire(Widget w);
static Boolean SetSimpleWire(Widget old, Widget request, Widget new,
							 ArgList args, Cardinal *num_args);
static void Resize(Widget w);
static void Redisplay(Widget w, XEvent *event, Region region);
static void Select_Wireframe_Action(Widget w, XEvent *event, String *params,
			Cardinal* num_params);


static XtActionsRec actions[] =
{
  /* {name, procedure}, */
    {"selectWireframe",	Select_Wireframe_Action}
};


static char translations[] = "<BtnUp> : selectWireframe()";


SimpleWireClassRec simpleWireClassRec = {
  { /* core fields */
    /* superclass		*/	(WidgetClass) &widgetClassRec,
    /* class_name		*/	"SimpleWire",
    /* widget_size		*/	sizeof(SimpleWireRec),
    /* class_initialize	*/	NULL,
    /* class_part_initialize	*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	InitializeSimpleWire,
    /* initialize_hook	*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	actions,
    /* num_actions		*/	XtNumber(actions),
    /* resources		*/	resources,
    /* num_resources	*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion	*/	TRUE,
    /* compress_exposure*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest	*/	FALSE,
    /* destroy			*/	DestroySimpleWire,
    /* resize			*/	Resize,
    /* expose			*/	Redisplay,
    /* set_values		*/	SetSimpleWire,
    /* set_values_hook	*/	NULL,
    /* set_values_almost*/	XtInheritSetValuesAlmost,
    /* get_values_hook	*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private	*/	NULL,
    /* tm_table			*/	translations,
    /* query_geometry	*/	XtInheritQueryGeometry,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
  { /* simpleWire fields */
    /* empty			*/	0
  }
};

WidgetClass simpleWireWidgetClass = (WidgetClass)&simpleWireClassRec;


/*	void
**	InitializeSimpleWire( ... )
**	The initialization procedure for SimpleWire widgets.
**	Does anything that can be done with the given args, in particular
**	sets up the wireframe (through its base object).
**	Also sets up the drawing_gc.
*/
static void
InitializeSimpleWire(Widget request, Widget new, ArgList args,
					 Cardinal *num_args)
{
	SimpleWireWidget	sww = (SimpleWireWidget)new;
	XGCValues			gc_vals;

	sww->simpleWire.wireframe = NULL;
	sww->simpleWire.scale = 1;
	if (sww->simpleWire.base_ptr != NULL)
	{
		BaseObjectPtr	base = (BaseObjectPtr)sww->simpleWire.base_ptr;

		/* A base object name has been specified. */
		sww->simpleWire.wireframe = base->b_wireframe;

	}

	/* Allocate a gc for drawing with. */
	gc_vals.foreground = sww->simpleWire.drawing_color;
	gc_vals.background = sww->core.background_pixel;
	gc_vals.font = sww->simpleWire.font->fid;
	sww->simpleWire.drawing_gc =
			XtGetGC((Widget)sww, GCForeground|GCBackground|GCFont, &gc_vals);

	sww->simpleWire.off_screen = 0;
}


/*	void
**	DestroySimpleWire(Widget w)
**	Frees memory and the drawing_gc.
*/
static void
DestroySimpleWire(Widget w)
{
	SimpleWireWidget	sww = (SimpleWireWidget)w;

	if ( sww->simpleWire.off_screen )
		XFreePixmap(XtDisplay(w), sww->simpleWire.off_screen);
	XtReleaseGC((Widget)sww, sww->simpleWire.drawing_gc);
}


/*	Boolean
**	SetSimpleWire( ... )
**	The SetValues procedure for SimpleWire widgets.
*/
static Boolean
SetSimpleWire(Widget old, Widget request, Widget new, ArgList args,
														Cardinal *num_args)
{
	SimpleWireWidget	newsww = (SimpleWireWidget)new;
	SimpleWireWidget	oldsww = (SimpleWireWidget)old;
	XGCValues			gc_vals;
	Boolean				need_redraw = FALSE;


#define NE(field) (newsww->simpleWire.field != oldsww->simpleWire.field)

	/* If drawing_color or foont have changed, need new GC. */
	if (NE(font) || NE(drawing_color))
	{
		XtReleaseGC((Widget)oldsww, oldsww->simpleWire.drawing_gc);
		gc_vals.foreground = newsww->simpleWire.drawing_color;
		gc_vals.background = newsww->core.background_pixel;
		gc_vals.font = newsww->simpleWire.font->fid;
		newsww->simpleWire.drawing_gc =
			XtGetGC((Widget)newsww, GCForeground|GCBackground|GCFont, &gc_vals);

		need_redraw = TRUE;
	}

	return need_redraw;
}



/*	void Resize(Widget w)
**	Returns if the widget is unrealized.
**	Clears the window with exposures TRUE.
*/
static void
Resize(Widget w)
{
	SimpleWireWidget    sww = (SimpleWireWidget)w;
	WireframePtr    	wireframe = (WireframePtr)sww->simpleWire.wireframe;

	double	max_x, max_y;	/* The absolute max distances from the centre. */
	Vertex	*these_vertices;
	Vector	*normals;
	Viewport	vp;
	int		i;
	Vector	sum;
	GC		clear_gc;
	XGCValues	gc_vals;
	Cuboid	bound;

	if ( ! XtIsRealized(w) ) return;
	if ( sww->core.width == 0 ||  sww->core.height == 0 ) return;
	if ( wireframe == NULL ) return;

	/* Need a group of vertices to work with. */
	these_vertices = New(Vertex, wireframe->num_vertices);

	/* Convert them to view. */
	/* Need a viewport to use. */
	/* View at is the centroid. Need to calculate it. */
	bound = Calculate_Bounds(wireframe->vertices, wireframe->num_vertices - 1);
	VAdd(bound.min, bound.max, sum);
	VScalarMul(sum, 0.5, vp.view_at);

	VNew(5, 4, 3, vp.view_from);
	VNew(0, 0, 1, vp.view_up);
	vp.view_distance = 500;
	vp.eye_distance = 250;
	Build_Viewport_Transformation(&vp);
	Convert_World_To_View(wireframe->vertices, these_vertices,
							wireframe->num_vertices, &vp);

	max_x = max_y = 0.0;
	for ( i = 0 ; i < wireframe->num_vertices ; i++ )
	{
		if ( these_vertices[i].view.z < 0 )
			continue;
		if (fabs(these_vertices[i].view.x) > max_x)
			max_x = fabs(these_vertices[i].view.x);
		if (fabs(these_vertices[i].view.y) > max_y)
			max_y = fabs(these_vertices[i].view.y);
	}

	if ( max_x == 0.0 && max_y == 0.0 )
		if ( max_y == 0.0 )
			sww->simpleWire.scale = 50;
		else
			sww->simpleWire.scale = (sww->core.height - 5) / max_y;
	else
		if ( max_y == 0.0 )
			sww->simpleWire.scale = (sww->core.width - 5) / max_x;
		else
			sww->simpleWire.scale =
					(int)min(((sww->core.width - 5) / max_x ),
							 ((sww->core.height - 5) / max_y ));

	/* Recalculate screen co-ords for each vertex. */
	Convert_View_To_Screen(these_vertices, wireframe->num_vertices, &vp,
		(short)sww->core.width, (short)sww->core.height,
		(double)(sww->simpleWire.scale));

	/* Also need a set of normals. */
	/* These come from the wireframe specs. */
	normals = New(Vector, wireframe->num_faces);
	for ( i = 0 ; i < wireframe->num_faces ; i++ )
		normals[i] = wireframe->faces[i].normal;

	if ( sww->simpleWire.off_screen )
		XFreePixmap(XtDisplay(w), sww->simpleWire.off_screen);
	sww->simpleWire.off_screen = XCreatePixmap(XtDisplay(w), XtWindow(w),
									sww->core.width, sww->core.height,
									DefaultDepthOfScreen(XtScreen(w)));
	gc_vals.function = GXcopy;
	gc_vals.foreground = sww->core.background_pixel;
	clear_gc = XtGetGC(w, GCFunction | GCForeground, &gc_vals);
	XFillRectangle(XtDisplay(w), sww->simpleWire.off_screen, clear_gc,
					0, 0, sww->core.width, sww->core.height);
	XtReleaseGC(w, clear_gc);

	Draw_Visible_Edges(XtDisplay(w), sww->simpleWire.off_screen,
				sww->simpleWire.drawing_gc,
				wireframe, these_vertices, wireframe->vertices, normals, &vp);

	free(these_vertices);
}


/*	void Redisplay(Widget w, XEvent *event, Region region);
**	The expose procedure.
**	Converts all the points for the wireframe to screen, then draws the edges.
*/
static void
Redisplay(Widget w, XEvent *event, Region region)
{
	SimpleWireWidget    sww = (SimpleWireWidget)w;

	if ( ! sww->simpleWire.off_screen )
		Resize(w);

	/* Copy the off screen on. */
	XCopyArea(XtDisplay(w), sww->simpleWire.off_screen,
			  XtWindow(w), sww->simpleWire.drawing_gc, 0, 0, sww->core.width,
			  sww->core.height, 0, 0);
}


static void
Select_Wireframe_Action(Widget w, XEvent *event, String *params,
												Cardinal* num_params)
{
	XtCallCallbacks(w, XtNcallback, (XtPointer)event);
}



/*	void
**	Update_SimpleWire_Wireframe(Widget widget, WireframePtr wireframe)
**	Updates the widget the represent the new wireframe.
*/
void
Update_SimpleWire_Wireframe(Widget widget, WireframePtr wireframe)
{
	XtCheckSubclass(widget, SimpleWireClass, "Update_SimpleWire_Wireframe");
	((SimpleWireWidget)widget)->simpleWire.wireframe = wireframe;
	Resize(widget);
}


