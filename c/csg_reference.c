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
**	csg_reference.c: Code for the dialog that allows the reference point for
**					a csg object to be selected.
*/

#include <math.h>
#include <sced.h>
#include <csg.h>
#include <csg_wire.h>
#include <select_point.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <View.h>

extern String	select_translations;

static void	Init_CSG_Reference(Boolean);
static void	Cleanup_CSG_Reference();
static void	Create_CSG_Reference_Shell();

BaseObjectPtr	target;

static Widget	csg_reference_shell = NULL;
static Widget	ref_view;
static Widget	full_button;

static Vertex	*ref_verts;
static Vector	*ref_normals;
static GC		ref_gc;
static GC		ref_hidden_gc;
static GC		ref_clear_gc;

static Viewport		csg_ref_viewport;
static WindowInfo	csg_ref_window;

static Boolean	finished;

/*	void
**	Select_CSG_Base_Reference(BaseObjectPtr obj)
**	Instigates the selection of the reference point for a csg base object.
*/
void
Select_Base_Reference(BaseObjectPtr obj)
{
	target = obj;

	if ( ! csg_reference_shell )
		Create_CSG_Reference_Shell();

	Init_CSG_Reference(TRUE);
}


static void
CSG_Reference_Select_Callback(int *index, FeatureSpecType *spec)
{
	target->b_ref_num = *index;

	Cleanup_CSG_Reference();

	XtPopdown(csg_reference_shell);

	finished = TRUE;
}


static void
CSG_Full_Wireframe_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	Cleanup_CSG_Reference();

	CSG_Generate_Full_Wireframe(target);

	Init_CSG_Reference(FALSE);
}


static void
CSG_Reference_Calc_Points()
{
	double	max_x, max_y;
	double		scale;
	Dimension	width, height;
	int		i;

	max_x = max_y = 0.0;
	for ( i = 0 ; i < target->b_wireframe->num_vertices ; i++ )
	{
		if (fabs(ref_verts[i].view.x) > max_x)
			max_x = fabs(ref_verts[i].view.x);
		if (fabs(ref_verts[i].view.y) > max_y)
			max_y = fabs(ref_verts[i].view.y);
	}
	XtVaGetValues(ref_view, XtNwidth, &width, XtNheight, &height, NULL);
	if ( IsZero(max_x) || IsZero(max_y) )
		scale = 100.0;
	else
		scale = min((((int)width - 5 ) / (double)max_x ),
					(((int)height - 5 ) / (double)max_y));

	Convert_View_To_Screen(ref_verts, target->b_wireframe->num_vertices,
						&csg_ref_viewport, (short)width, (short)height, scale);

	for ( i = 0 ; i < num_select_verts ; i++ )
	{
		select_verts[i].vert = ref_verts[i].screen;
		select_verts[i].obj = NULL;
	}
}

static void
CSG_Reference_Draw()
{
	Dimension	width, height;
	XRectangle	area;

	XtVaGetValues(ref_view, XtNwidth, &width, XtNheight, &height, NULL);

	area.x = area.y = 0;
	area.width = (short)width;
	area.height = (short)height;

	XFillRectangles(XtDisplay(ref_view), XtWindow(ref_view), ref_clear_gc,
					&area, 1);

	Draw_Edges(XtDisplay(ref_view), XtWindow(ref_view), ref_gc, ref_hidden_gc,
			   target->b_wireframe, ref_verts, target->b_wireframe->vertices,
			   ref_normals, &csg_ref_viewport);
	
	Select_Highlight_Closest(XtWindow(ref_view));
	Draw_Selection_Points(XtWindow(ref_view));
}

static void
Init_CSG_Reference(Boolean popup)
{
	Cuboid	bound;
	Vector	sum;
	int		i;

	bound = Calculate_Bounds(target->b_wireframe->vertices,
							 target->b_wireframe->num_vertices - 1);
	VAdd(bound.min, bound.max, sum);
	VScalarMul(sum, 0.5, csg_ref_viewport.view_at);
	Build_Viewport_Transformation(&csg_ref_viewport);

	ref_verts = New(Vertex, target->b_wireframe->num_vertices);
	Convert_World_To_View(target->b_wireframe->vertices, ref_verts,
					target->b_wireframe->num_vertices, &csg_ref_viewport);
	ref_normals = New(Vector, target->b_wireframe->num_faces);
	for ( i = 0 ; i < target->b_wireframe->num_faces ; i++ )
		ref_normals[i] = target->b_wireframe->faces[i].normal;

	num_select_verts = target->b_wireframe->num_vertices - 1;
	select_verts = New(SelectPointStruct, num_select_verts);

	if ( target->b_class == csg_obj )
		XtSetSensitive(full_button, TRUE);
	else
		XtSetSensitive(full_button, FALSE);

	if ( popup )
	{
		SFpositionWidget(csg_reference_shell);
		XtPopup(csg_reference_shell, XtGrabExclusive);
	}

	CSG_Reference_Calc_Points();

	select_window = &csg_ref_window;
	select_highlight = FALSE;
	num_verts_required = 1;
	allow_text_entry = FALSE;
	select_finished_callback = CSG_Reference_Select_Callback;
	specs_allowed[reference_spec] =
	specs_allowed[offset_spec] =
	specs_allowed[absolute_spec] = TRUE;

	Prepare_Selection_Drawing();

	CSG_Reference_Draw();

	if ( popup )
	{
		XtAppContext	context;
		XEvent			event;
		
		context = XtWidgetToApplicationContext(main_window.shell);
		while ( ! ( finished ) )
		{
			XtAppNextEvent(context, &event);
			XtDispatchEvent(&event);
		}
	}
}


static void
Cleanup_CSG_Reference()
{
	Cleanup_Selection();
	free(ref_verts);
	free(ref_normals);
}


static void
CSG_Reference_Redraw(Widget w, XtPointer cl, XtPointer ca)
{
	CSG_Reference_Calc_Points();
	CSG_Reference_Draw();
}


static void
CSG_Reference_GCs()
{
	Pixel		foreground, background;
	XGCValues	gc_vals; 

	XtVaGetValues(ref_view, XtNforeground, &foreground,
							XtNbackground, &background, NULL);

	gc_vals.function = GXcopy;
	gc_vals.foreground = foreground;
	ref_gc = XtGetGC(csg_reference_shell, GCFunction|GCForeground|GCBackground,
					&gc_vals);

	gc_vals.line_style = LineOnOffDash;
	ref_hidden_gc = XtGetGC(csg_reference_shell,
					GCFunction|GCForeground|GCBackground|GCLineStyle, &gc_vals);

	gc_vals.function = GXcopy;
	gc_vals.foreground = background;
	ref_clear_gc = XtGetGC(csg_reference_shell,
					GCForeground | GCFunction, &gc_vals);
}


static void
Create_CSG_Reference_Shell()
{
	Widget	form;
	Widget	label;
	Arg		args[10];
	int		n;
	Dimension	width, form_height, label_height;
	int			gap;

	/* Create the shell. */
	n = 0;
	XtSetArg(args[n], XtNtitle, "CSG Reference Shell");	n++;
	csg_reference_shell = XtCreatePopupShell("csgReferenceShell",
						transientShellWidgetClass, main_window.shell, args, n);

	XtRealizeWidget(csg_reference_shell);

	n = 0;
	form = XtCreateManagedWidget("csgReferenceForm", formWidgetClass,
								 csg_reference_shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Select The Scaling Point");	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);						n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);					n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);					n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	label = XtCreateManagedWidget("csgReferenceLabel", labelWidgetClass,
								  form, args, n);

	n = 0;
	XtSetArg(args[n], XtNwidth, &width);			n++;
	XtSetArg(args[n], XtNheight, &form_height);		n++;
	XtSetArg(args[n], XtNdefaultDistance, &gap);	n++;
	XtGetValues(form, args, n);

	n = 0;
	XtSetArg(args[n], XtNheight, &label_height);	n++;
	XtGetValues(label, args, n);

	n = 0;
	XtSetArg(args[n], XtNwidth, (int)width - 2 * gap);	n++;
	XtSetArg(args[n], XtNheight, (int)form_height - (int)label_height -
								 (int)label_height - 6 * gap);		n++;
	XtSetArg(args[n], XtNtop, XtChainTop);				n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);		n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);			n++;
	XtSetArg(args[n], XtNright, XtChainRight);			n++;
	XtSetArg(args[n], XtNfromVert, label);				n++;
	ref_view = XtCreateManagedWidget("csgReferenceView", viewWidgetClass,
									 form, args, n);
	XtAddCallback(ref_view, XtNexposeCallback, CSG_Reference_Redraw, NULL);
	XtOverrideTranslations(ref_view,
					XtParseTranslationTable(select_translations));

	/* The full wireframe command button. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Full Wireframe");	n++;
	XtSetArg(args[n], XtNtop, XtChainBottom);		n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);	n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);	n++;
#endif
	XtSetArg(args[n], XtNfromVert, ref_view);		n++;
	full_button = XtCreateManagedWidget("csgRefButton", commandWidgetClass,
									form, args, n);
	XtAddCallback(full_button, XtNcallback, CSG_Full_Wireframe_Callback, NULL);

	/* Set up the viewport. */
	VNew(5, 4, 3, csg_ref_viewport.view_from);
	VNew(0, 0, 1, csg_ref_viewport.view_up);
	csg_ref_viewport.view_distance = 500;
	csg_ref_viewport.eye_distance = 500;

	csg_ref_window.shell = csg_reference_shell;
	csg_ref_window.view_widget = ref_view;

	CSG_Reference_GCs();
}

