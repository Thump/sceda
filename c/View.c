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
**	View.c : C file for the View widget class.
**
**	Created: 19/03/94
*/

#include <X11/IntrinsicP.h>
#include <sced.h>
#include <ViewP.h>

static XtResource resources[] = {
#define offset(field) XtOffsetOf(ViewRec, view.field)
    /* {name, class, type, size, offset, default_type, default_addr}, */
	{ XtNexposeCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	  offset(expose_callback), XtRCallback, NULL },
	{ XtNdesiredWidth, XtCWidth, XtRDimension, sizeof(Dimension),
	  offset(desired_width), XtRDimension, 0 },
	{ XtNdesiredHeight, XtCHeight, XtRDimension, sizeof(Dimension),
	  offset(desired_height), XtRDimension, 0 },
	{ XtNmaintainSize, XtCBoolean, XtRBoolean, sizeof(Boolean),
	  offset(maintain_size), XtRBoolean, FALSE },
	{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	  offset(foreground_pixel), XtRString, XtDefaultForeground },
	{ XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
	  offset(background_pixel), XtRString, XtDefaultBackground },
	{ XtNmagnification, XtCValue, XtRInt, sizeof(int),
	  offset(magnification), XtRString, "1" }
#undef offset
};


/* Prototypes for various procedures. */
static void InitView(Widget, Widget, ArgList, Cardinal*);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal*);
static XtGeometryResult
		QueryGeometry(Widget, XtWidgetGeometry*, XtWidgetGeometry*);
static void Redisplay(Widget w, XEvent *event, Region region);


ViewClassRec viewClassRec = {
  { /* core fields */
    /* superclass		*/	(WidgetClass) &widgetClassRec,
    /* class_name		*/	"View",
    /* widget_size		*/	sizeof(ViewRec),
    /* class_initialize		*/	NULL,
    /* class_part_initialize	*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	InitView,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	NULL,
    /* resize			*/	NULL,
    /* expose			*/	Redisplay,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	NULL,
    /* query_geometry		*/	QueryGeometry,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
  { /* view fields */
    /* empty			*/	0
  }
};

WidgetClass viewWidgetClass = (WidgetClass)&viewClassRec;




/*	static void InitView(Widget, Widget, ArgList, Cardinal*)
**	Initialization function.  Just sets the width and height if desired.
*/
static void
InitView(Widget req_w, Widget new_w, ArgList args, Cardinal *num)
{
	ViewWidget	new = (ViewWidget)new_w;

	if ( new->view.maintain_size )
	{
		new->core.width = new->view.desired_width;
		new->core.height = new->view.desired_height;
	}
}

/*	static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal*)
**	This set values function only reacts to changes in the desired
**	geometry fields and the maintainSize field.
**	If either changes, it checks against the current width and height
**	and tries to change them if they don't match what is desired.
*/
static Boolean
SetValues(Widget old_w, Widget request_w, Widget new_w, ArgList args,
			Cardinal *num)
{
	ViewWidget	old = (ViewWidget)old_w;
	ViewWidget	new = (ViewWidget)old_w;
	Boolean		size_change = FALSE;

	if ( old->view.desired_width != new->view.desired_width )
		size_change = TRUE;

	if ( old->view.desired_height != new->view.desired_height )
		size_change = TRUE;

	if ( new->view.maintain_size &&( size_change || ! old->view.maintain_size ))
	{
		/* They want the geometry to change.  So reset the width and height
		** fields.
		*/
		new->core.width = new->view.desired_width;
		new->core.height = new->view.desired_height;

		return TRUE;
	}

	return FALSE;

}


/*	static XtGeometryResult
**	QueryGeometry(Widget w, XtWidgetGeometry *prop, XtWidgetGeometry *pref)
**	The query geometry procedure.  Basically always tries to get its
**	preferred size.
*/
static XtGeometryResult
QueryGeometry(Widget w, XtWidgetGeometry *prop, XtWidgetGeometry *pref)
{
	ViewWidget	widg = (ViewWidget)w;
#define Set(b) (prop->request_mode & b)

	if ( widg->view.maintain_size )
	{
		pref->width = widg->view.desired_width;
		pref->height = widg->view.desired_height;
		pref->request_mode = CWWidth | CWHeight;
	}
	else
	{
		pref->width = prop->width;
		pref->height = prop->height;
	}

	if (Set(CWWidth) && (prop->width == pref->width) &&
		Set(CWHeight) && (prop->height == pref->height))
		return XtGeometryYes;

	if ((pref->width == widg->core.width) &&
		(pref->height == widg->core.height))
		return XtGeometryNo;

	return XtGeometryAlmost;
#undef Set
}


/*	void Redisplay(Widget w, XEvent *event, Region region);
**	The expose procedure.
**	Calls the Callback for expose.
*/
static void
Redisplay(Widget w, XEvent *event, Region region)
{
	XtCallCallbacks(w, XtNexposeCallback, (XtPointer)&region);
}

