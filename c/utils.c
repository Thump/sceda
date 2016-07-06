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
**	utils.c: Utility functions.
**
**	Created: 05/03/94
**
**	External functions:
**
**	char*
**	EMalloc(unsigned int size);
**	Tries to malloc size bytes, exiting on failure.
**
**	char*
**	WMalloc(unsigned int size);
**	Tries to malloc size bytes, warning and returning NULL on failure.
**
**	char*
**	ERealloc(char *ptr, unsigned int size);
**	Tries to realloc size bytes, exiting on failure.
**
**	char*
**	WRealloc(char *ptr, unsigned int size);
**	Tries to realloc size bytes, warning and returning NULL on failure.
**
**	void
**	Popup_Error(char *text, Widget parent, char *title)
**	Pops up an error dialog box.
**
**	Dimension
**	Match_Widths(Widget *widgets, int num)
**	Makes all the widgets the width of the widest.
**	Returns the maximum width.
**
**	void
**	Destroy_World(Boolean)
**	Destroys the world (really!).
**	Actually just frees the instance list and any non-default base types.
**	Also cameras, light sources and the other nitty gritty bits.
**
**	Boolean
**	Check_Rectangle_Intersection(XPoint, XPoint, XPoint, XPoint)
**	Returns TRUE if the 2 rectangles defined by the points intersect.
**
**	Vector
**	Map_Point_Onto_Plane(XPoint pt, FeatureData plane, Viewport *vp,
**						 short width, short height, int mag)
**	Maps a point onto the desired plane, where the point is in screen co-ords
**	and the plane is in view.
**
**	Vector
**	Map_Point_Onto_Line(XPoint pt, FeatureData line, Viewport *vp,
**						 short width, short height, int mag)
**	Maps the point pt onto the line line, specified in screen.  Returns the
**	view coordinates of pt assuming it lies on the line.
**
**	void
**	Set_Main_Prompt(String new)
**	Sets the main prompt string.
**
**	double
**	Round_To_Snap(double orig, double snap)
**	Rounds orig to the nearest multiple of snap.
**
**	Boolean
**	Points_Colinear(Vector p1, Vector p2, Vector p3)
**	Returns TRUE if the three screen points are colinear.
**
**	void
**	Set_WindowInfo(WindowInfo *inf)
**	Initializes all the fields of a WindowInfo structure.
**
**	Vector
**	Extract_Euler_Angles(Matrix)
**	Extracts the three euler angles from a rotation matrix.
**
**	void
**	Copy_Objects_Callback(Widget w, XtPointer cl, XtPointer ca)
**	The callback for the copy function.
*/

#include <stdlib.h>
#include <math.h>
#include <sced.h>
#include <base_objects.h>
#include <instance_list.h>
#include <layers.h>
#include <malloc.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/Command.h>

extern void	Save_Func(FILE*);

/* The popup error message widget. */
static Widget	popup_err_shell;

/* The popup error acknowledge callback. */
static Boolean	error_down;
static void		Acknowledge_Error(Widget, XtPointer, XtPointer);

static void
Sced_Abort()
{
	FILE	*save_file;

	if ( ( save_file = fopen("sced.save", "w") ) == NULL )
	{
		fprintf(stderr, "sced: Could not open emergency save file\n");
		return;
	}

	Save_Func(save_file);
}


/*	char*
**	emalloc(unsigned int size);
**	Tries to malloc size bytes, exiting on failure.
*/
char*
EMalloc(unsigned int size)
{
	char	*res;

	if ( size == 0 )
	{
#ifdef DEBUG_MALLOC
		fprintf(stderr, "Trying to malloc 0 bytes.\n");
		Sced_Abort();
		abort();
#else
		size = 4;
#endif
	}

	if ((res = malloc(size)) == NULL)
	{
		fprintf(stderr, "Unable to malloc %d bytes.  Exiting.\n", size);
		Sced_Abort();
#ifdef DEBUG_MALLOC
		abort();
#else
		exit(1);
#endif
	}

	return res;
}


/*	char*
**	wmalloc(unsigned int size);
**	Tries to malloc size bytes, warning and returning NULL on failure.
*/
char*
WMalloc(unsigned int size)
{
	char	*res;

	if ((res = malloc(size)) == NULL)
		fprintf(stderr, "Unable to malloc %d bytes.\n", size);

	return res;
}


/*	char*
**	erealloc(char *ptr, unsigned int size);
**	Tries to realloc size bytes, exiting on failure.
*/
char*
ERealloc(char *ptr, unsigned int size)
{
	char	*res;

	if ( size == 0 )
	{
#ifdef DEBUG_MALLOC
		fprintf(stderr, "Trying to realloc 0 bytes.\n");
		Sced_Abort();
		abort();
#else
		size = 4;
#endif
	}

	if ((res = realloc(ptr, size)) == NULL)
	{
		fprintf(stderr, "Unable to realloc %d bytes.  Exiting.\n", size);
		Sced_Abort();
#ifdef DEBUG_MALLOC
		abort();
#else
		exit(1);
#endif
	}

	return res;
}


/*	char*
**	wrealloc(char *ptr, unsigned int size);
**	Tries to realloc size bytes, warning and returning NULL on failure.
*/
char*
WRealloc(char *ptr, unsigned int size)
{
	char	*res;

	if ((res = realloc(ptr, size)) == NULL)
		fprintf(stderr, "Unable to realloc %d bytes.\n", size);

	return res;
}



/*	void
**	Popup_Error(char *text, Widget parent, char *title)
**	Pops up an error dialog box.
*/
void
Popup_Error(char *text, Widget parent, char *title)
{
	XtAppContext	context;
	XEvent			event;
	static Widget	err_dialog = NULL;
	Arg		arg;

	popup_err_shell = XtCreatePopupShell(title, transientShellWidgetClass,
										main_window.shell, NULL, 0);

	XtSetArg(arg, XtNlabel, text);
	err_dialog = XtCreateManagedWidget("errDialog", dialogWidgetClass,
										popup_err_shell, &arg, 1);

	XawDialogAddButton(err_dialog, "Acknowledge", Acknowledge_Error, NULL);

	XtVaSetValues(XtNameToWidget(err_dialog, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(err_dialog, "Acknowledge"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(popup_err_shell);

	SFpositionWidget(popup_err_shell);

	XtPopup(popup_err_shell, XtGrabExclusive);

	error_down = FALSE;
	context = XtWidgetToApplicationContext(main_window.shell);
	while ( ! error_down )
	{
		XtAppNextEvent(context, &event);
		XtDispatchEvent(&event);
	}
}


static void
Acknowledge_Error(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(popup_err_shell);

	XtDestroyWidget(popup_err_shell);

	error_down = TRUE;
}



/*	Dimension
**	Match_Widths(Widget *widgets, int num)
**	Makes all the widgets the width of the widest.
**	Returns the maximum width.
*/
Dimension
Match_Widths(Widget *widgets, int num)
{
	Dimension	max_width;
	Dimension	width;
	Arg			arg;
	int			i;

	max_width = 0;
	XtSetArg(arg, XtNwidth, &width);
	for ( i = 0 ; i < num ; i++ )
	{
		XtGetValues(widgets[i], &arg, 1);
		if ( width > max_width ) max_width = width;
	}

	XtSetArg(arg, XtNwidth, max_width);
	for ( i = 0 ; i < num ; i++ )
		XtSetValues(widgets[i], &arg, 1);

	return max_width;
}


/*	void
**	Destroy_World(Boolean destroy_bases)
**	Destroys the world and all that's in it.
*/
void
Destroy_World(Boolean destroy_bases)
{	int 	numkf,i;
	KeyFrame *kfptr;
	InstanceList	inst, temp_inst;

	if (destroy_bases)
	{	numkf=Count_KeyFrames(key_frames);

		Update_to_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames));

		/* Remove all the keyframes except the first. */
		for (i=numkf; i>1; i--)
		{	Update_from_KeyFrame(Nth_KeyFrame(i,key_frames),TRUE);
	
			Remove_KeyFrame(NULL,NULL,NULL,NULL);
		}

		/* ...and now that we only have one keyframe, away we go... */
		Update_from_KeyFrame(Nth_KeyFrame(1,key_frames),TRUE);
	}

	Free_Selection_List(main_window.selected_instances);
	main_window.selected_instances = NULL;

	for ( inst = main_window.edit_instances ; inst ; inst = temp_inst )
	{
		temp_inst = inst->next;
		Delete_Edit_Instance(&main_window, inst);
	}
	main_window.edit_instances = NULL;

	/* Destroy all the instances first. */
	for ( inst = main_window.all_instances ; inst != NULL ; inst = temp_inst )
	{
		temp_inst = inst->next;
		Destroy_Instance(Delete_Instance(inst));
	}
	main_window.all_instances = NULL;

	if ( csg_window.shell )
		CSG_Reset();

	if ( destroy_bases )
	{
		Destroy_All_Base_Objects();
		Reset_Layers();
		View_Reset();
	}

	changed_scene = FALSE;

	/* save the result back to the keyframes */
	Update_to_KeyFrame(Nth_KeyFrame(1,key_frames));

	/* Redraw an empty world. */
	View_Update(&main_window, main_window.all_instances, ViewNone);

	XFlush(XtDisplay(main_window.shell));
}


/*	Boolean
**	Check_Rectangle_Intersection(XPoint min1, XPoint max1,
**								 XPoint min2, XPoint max2)
**	Returns TRUE if the 2 rectangles defined by the points intersect.
*/
Boolean
Check_Rectangle_Intersection(XPoint min1, XPoint max1, XPoint min2, XPoint max2)
{
	XPoint	min_int, max_int;

	/* Intersect them, and see if the result is valid. */
	min_int.x = max(min1.x, min2.x);
	max_int.x = min(max1.x, max2.x);

	if ( max_int.x < min_int.x )
		return FALSE;

	min_int.y = max(min1.y, min2.y);
	max_int.y = min(max1.y, max2.y);

	if ( max_int.y < min_int.y )
		return FALSE;

	return TRUE;
}


/*	Vector
**	Map_Point_Onto_Plane(XPoint pt, FeatureData plane, Viewport *vp,
**						 short width, short height, int mag)
**	Maps the point pt onto the plane plane, specified in view.  Returns the
**	view coordinates of pt assuming it lies in the plane plane.
*/
Vector
Map_Point_Onto_Plane(XPoint pt, FeatureData plane, Viewport *vp, short width,
					 short height, int mag)
{
	Vector	result;
	short	mid_x = width / 2;
	short	mid_y = height / 2;
	double	temp_d;

	result.x = (double)(pt.x - mid_x) / (double)mag;
	result.y = (double)(mid_y - pt.y) / (double)mag;

	temp_d = result.x * plane.f_vector.x + result.y * plane.f_vector.y;
	result.z = vp->eye_distance *
				( VDot(plane.f_vector, plane.f_point ) - temp_d ) /
				(temp_d + vp->eye_distance * plane.f_vector.z );
	temp_d = (vp->eye_distance + result.z) / vp->eye_distance;
	result.x *= temp_d;
	result.y *= temp_d;

	return result;
}


/*	Vector
**	Map_Point_Onto_Line(XPoint pt, FeatureData line, Viewport *vp,
**						 short width, short height, int mag)
**	Maps the point pt onto the line line, specified in screen.  Returns the
**	view coordinates of pt assuming it lies on the line.
**	The mapping drops a perpendicular from the point onto the line in
**	screen, and returns the point of intersection of the perpendicular
**	with the line converted back into view.
*/
Vector
Map_Point_Onto_Line(XPoint pt, FeatureData line, Viewport *vp, short width,
					 short height, int mag)
{
	Vector	scr_line_dir;
	Vector	scr_line_pt;
	Vertex	line_pt_1;
	Vertex	line_pt_2;
	Vector	result;
	double	temp_d1;
	Vector	temp_v1;

	/* Convert the line in view into screen.*/
	VScalarMul(line.f_vector, 10, temp_v1);
	VAdd(line.f_point, temp_v1, line_pt_1.view);
	line_pt_2.view = line.f_point;
	Convert_View_To_Screen(&line_pt_1, 1, vp, width, height, (double)mag);
	Convert_View_To_Screen(&line_pt_2, 1, vp, width, height, (double)mag);
	scr_line_dir.x = line_pt_1.screen.x - line_pt_2.screen.x;
	scr_line_dir.y = line_pt_1.screen.y - line_pt_2.screen.y;
	scr_line_dir.z = 0;
	VNew(line_pt_2.screen.x, line_pt_2.screen.y, 0, scr_line_pt);

	/* Project the mouse point onto the line, in screen. */
	temp_v1.x = pt.x - scr_line_pt.x;
	temp_v1.y = pt.y - scr_line_pt.y;
	temp_d1 = ( temp_v1.x * scr_line_dir.x + temp_v1.y * scr_line_dir.y ) /
		( scr_line_dir.x * scr_line_dir.x + scr_line_dir.y * scr_line_dir.y );
	temp_d1 *= 10;

	/* temp_d1 is the distance along the line in screen.
	** Use the same param in view. */
	VScalarMul(line.f_vector, temp_d1, result);
	VAdd(result, line.f_point, result);

	return result;
}


/*	void
**	Set_Prompt(WindowInfoPtr window, String new)
**	Sets the main prompt string.
*/
void
Set_Prompt(WindowInfoPtr window, String new)  
{
	XawTextBlock	text_block;
	int	old_length;

	old_length = strlen(window->text_string);
	text_block.format = FMT8BIT;
	text_block.firstPos = 0;
	strcpy(window->text_string, new);
	text_block.length = strlen(window->text_string);
	text_block.ptr = window->text_string;
	XawTextReplace(window->text_widget, 0, old_length + 1, &text_block);
}



/*	Boolean
**	Points_Colinear(Vector p1, Vector p2, Vector p3)
**	Returns TRUE if the three screen points are colinear.
*/
Boolean
Points_Colinear(Vector p1, Vector p2, Vector p3)
{
	Vector temp1, temp2, temp3;

	VSub(p1, p2, temp1);
	VSub(p1, p3, temp2);
	VCross(temp1, temp2, temp3);
	return VZero(temp3);
}


/*	void
**	Set_WindowInfo(WindowInfo *inf)
**	Initializes all the fields of a window info structure.
*/
void
Set_WindowInfo(WindowInfo *inf)
{
	inf->shell = NULL;
	inf->view_widget = NULL;
	inf->text_string = New(char, ENTRY_STRING_LENGTH);
	inf->all_instances = NULL;
	inf->edit_instances = NULL;
	inf->current_state = wait;
	inf->off_screen = 0;
	inf->width = inf->height = 0;
	inf->kfnumber = 1;
	inf->fnumber = 1;
}



void
Copy_Objects_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	InstanceList	elmt;
	InstanceList	new_insts = NULL;

	for ( elmt = ((WindowInfoPtr)cl)->selected_instances ;
		  elmt ;
		  elmt = elmt->next )
		Insert_Element(&new_insts, Copy_Object_Instance(elmt->the_instance));

	Add_Instance_To_Edit(((WindowInfoPtr)cl), new_insts, FALSE);

	Free_Selection_List(new_insts);
}


void
Save_Temp_Filename(char *name)
{
	if ( num_temp_files == 0 )
		temp_filenames = New(char*, 1);
	else
		temp_filenames = More(temp_filenames, char*, num_temp_files + 1);
	temp_filenames[num_temp_files++] = Strdup(name);
}


void
Vector_To_Rotation_Matrix(Vector *vect, Matrix *res)
{
	Matrix	y_rot, z_rot;
	Vector	v1, v2, v3;
	double	cos_a, sin_a;
	double	rads;

	rads = vect->x * M_PI / 180.0;
	cos_a = cos(rads);
	sin_a = sin(rads);
	VNew(1, 0, 0, v1);
	VNew(0, cos_a, -sin_a, v2);
	VNew(0, sin_a, cos_a, v3);
	MNew(v1, v2, v3, *res);

	rads = vect->y * M_PI / 180.0;
	cos_a = cos(rads);
	sin_a = sin(rads);
	VNew(cos_a, 0, sin_a, v1);
	VNew(0, 1, 0, v2);
	VNew(-sin_a, 0, cos_a, v3);
	MNew(v1, v2, v3, y_rot);
	*res = MMMul(&y_rot, res);

	rads = vect->z * M_PI / 180.0;
	cos_a = cos(rads);
	sin_a = sin(rads);
	VNew(cos_a, -sin_a, 0, v1);
	VNew(sin_a, cos_a, 0, v2);
	VNew(0, 0, 1, v3);
	MNew(v1, v2, v3, z_rot);
	*res = MMMul(&z_rot, res);
}

