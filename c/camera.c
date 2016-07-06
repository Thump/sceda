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
**	camera.c : Camera definition, saving and loading functions.
**
**	Created: 01/05/94
**
*/

#include <math.h>
#include <sced.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Toggle.h>
#include <View.h>



/* Define the global camera here.  It starts out undefined, so type None. */
Camera	camera = { NoTarget,
				   FALSE,
				   {0, 0, 0},
				   {0, 0, 0},
				   {0, 0, 0},
				   0.0, 0.0, 0.0,
				   0.0, 0.0, 0, 0 };

extern void Target_Callback(Widget, XtPointer, XtPointer);

static void Update_Camera_Strings(Camera* new_vals, Viewport *vp);
static void	Viewport_To_Camera_Callback(Widget, XtPointer, XtPointer);

static void Create_Rayshade_Shell();
static void Create_POVray_Shell();
static void Create_Radiance_Shell();
static void Create_Genray_Shell();
static void Create_Buttons(Widget, Widget);

static void Default_Camera_Callback(Widget, XtPointer, XtPointer);
static void Cancel_Camera_Callback(Widget, XtPointer, XtPointer);
static void Apply_Camera_Callback(Widget, XtPointer, XtPointer);


static Widget	camera_dialog_shell = NULL;
static Widget	rayshade_shell = NULL;
static Widget	povray_shell = NULL;
static Widget	radiance_shell = NULL;
static Widget	genray_shell = NULL;

static Widget	default_toggle;
static Widget	rayshade_default;
static Widget	povray_default;
static Widget	radiance_default;
static Widget	genray_default;

#define MAX_TEXT_LENGTH 30
/* Declare an enumerated type with a value for each piece of text. */
/* It will make indexing simpler (I hope). */
enum _string_index { location_index, look_at_index, look_up_index,
					 window_up_index, eye_dist_index, window_right_index };

/* We have a string for each piece of text, plus widgets for each shell. */
static char		camera_text[window_right_index + 1][MAX_TEXT_LENGTH];
static Widget	ray_text[window_right_index + 1] = {NULL};
static Widget	pov_text[window_right_index + 1] = {NULL};
static Widget	rad_text[window_right_index + 1] = {NULL};
static Widget	gen_text[window_right_index + 1] = {NULL};


/*	RELEVANT CAMERA FIELDS:
**
**	type		| Rayshade	| POVray	| Genray | Radiance / Renderman
**	location	|	X		|	X		|        |    X
**	look_at		|	X		|	X		|	X    |    X
**	look_up		|	X		|	X		|	X    |    X
**	horiz_fov	|	X		|			|        |    X
**	vert_fov	|	X		|			|        |    X
**	eye_dist	|	X		|			|	X    |
**	window_up	|			|	X		|	X    |
**	window_right|			|	X		|	X    |
*/

void
Define_Camera_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	/* Need to have a target defined so we know which camera bits to display. */
	if ( camera.type == NoTarget )
	{
		Target_Callback(NULL, "Camera", NULL);
		return;
	}

	switch ( camera.type )
	{
		case Rayshade:
			if ( ! rayshade_shell )
				Create_Rayshade_Shell();
			camera_dialog_shell = rayshade_shell;
			default_toggle = rayshade_default;
			break;
		case POVray:
			if ( ! povray_shell )
				Create_POVray_Shell();
			camera_dialog_shell = povray_shell;
			default_toggle = povray_default;
			break;
		case Radiance:
		case Renderman:
			if ( ! radiance_shell )
				Create_Radiance_Shell();
			camera_dialog_shell = radiance_shell;
			default_toggle = radiance_default;
			break;
		case Genray:
		case Genscan:
			if ( ! genray_shell )
				Create_Genray_Shell();
			camera_dialog_shell = genray_shell;
			default_toggle = genray_default;
			break;

		case NoTarget:
			return;
	}

	if ( camera.default_cam )
	{
		XtVaSetValues(default_toggle, XtNstate, TRUE, NULL);
		Default_Camera_Callback(NULL, NULL, NULL);
	}
	else
		Update_Camera_Strings(&camera, &(main_window.viewport));

	if ( ! XtIsRealized(camera_dialog_shell))
		XtRealizeWidget(camera_dialog_shell);

	SFpositionWidget(camera_dialog_shell);
	XtPopup(camera_dialog_shell, XtGrabExclusive);
}




static void
Create_Rayshade_Shell()
{
	Dimension	label_height;
	Widget	top_label;
	Widget	rayshade_form;
	Widget	labels[6];
	Arg		args[15];
	int		count;
	int		m, n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Rayshade Camera");	n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	rayshade_shell = XtCreatePopupShell("rayshadeCameraShell",
						transientShellWidgetClass, main_window.shell, args, n);

	/* Create the form to put all 17 widgets into. */
	n = 0;
	rayshade_form = XtCreateManagedWidget("rayshadeForm", formWidgetClass,
					rayshade_shell, args, n);

	/* Add the label at the top. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Rayshade Camera");	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);		n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	top_label = XtCreateManagedWidget("rayshadeCameraLabel", labelWidgetClass,
				rayshade_form, args, n);

	/* Common args for all the labels. */
	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;
	m = n;

	count = 0;
	/* The label for the eyep vector. */
	XtSetArg(args[n], XtNlabel, "eyep");		n++;
	XtSetArg(args[n], XtNfromVert, top_label);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("eyepLabel", labelWidgetClass,
					rayshade_form, args, n);
	count++;

	/* The label for the lookp vector. */
	n = m;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNlabel, "lookp");				n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("lookpLabel", labelWidgetClass,
					rayshade_form, args, n);
	count++;

	/* The label for the up vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "up");					n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("upLabel", labelWidgetClass,
					rayshade_form, args, n);
	count++;

	/* The label for the fov vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "fov");					n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("fovLabel", labelWidgetClass,
					rayshade_form, args, n);
	count++;

	/* The label for the focaldist vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "focaldist");			n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("focalLabel", labelWidgetClass,
					rayshade_form, args, n);
	count++;

	Match_Widths(labels, count);

	/* Need the height of the labels to get the string size right. */
	n = 0;
	XtSetArg(args[n], XtNheight, &label_height);	n++;
	XtGetValues(top_label, args, n);

	count = 0;
	/* All the string now.  What joy. */
	n = 0;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainLeft);		n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);		n++;
	XtSetArg(args[n], XtNresizable, TRUE);			n++;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNresize, XawtextResizeWidth);	n++;
	m = n;

	XtSetArg(args[n], XtNstring, camera_text[location_index]);	n++;
	XtSetArg(args[n], XtNfromVert, top_label);					n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	ray_text[location_index] = XtCreateManagedWidget("locationText",
					asciiTextWidgetClass, rayshade_form, args, n);
	XtOverrideTranslations(ray_text[location_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[look_at_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	ray_text[look_at_index] = XtCreateManagedWidget("lookatText",
					asciiTextWidgetClass, rayshade_form, args, n);
	XtOverrideTranslations(ray_text[look_at_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[look_up_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	ray_text[look_up_index] = XtCreateManagedWidget("lookupText",
					asciiTextWidgetClass, rayshade_form, args, n);
	XtOverrideTranslations(ray_text[look_up_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[window_up_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	ray_text[window_up_index] = XtCreateManagedWidget("upText",
					asciiTextWidgetClass, rayshade_form, args, n);
	XtOverrideTranslations(ray_text[window_up_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[eye_dist_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	ray_text[eye_dist_index] = XtCreateManagedWidget("eyeText",
					asciiTextWidgetClass, rayshade_form, args, n);
	XtOverrideTranslations(ray_text[eye_dist_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	Create_Buttons(rayshade_form, ray_text[eye_dist_index]);
	rayshade_default = default_toggle;
}


static void
Create_POVray_Shell()
{
	Dimension	label_height;
	Widget	povray_form;
	Widget	top_label;
	Widget	labels[6];
	Arg		args[15];
	int		count;
	int		m, n;


	n = 0;
	XtSetArg(args[n], XtNtitle, "Genray Camera");	n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	povray_shell = XtCreatePopupShell("povrayCameraShell",
						transientShellWidgetClass, main_window.shell, args, n);

	/* Create the form to put all 17 widgets into. */
	n = 0;
	povray_form = XtCreateManagedWidget("povrayForm", formWidgetClass,
					povray_shell, args, n);

	/* Add the label at the top. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "POVray Camera");	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);		n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	top_label = XtCreateManagedWidget("povrayCameraLabel", labelWidgetClass,
				povray_form, args, n);

	/* Common args for all the labels. */
	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;
	m = n;

	count = 0;
	/* The label for the location vector. */
	XtSetArg(args[n], XtNlabel, "location");	n++;
	XtSetArg(args[n], XtNfromVert, top_label);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("locationLabel", labelWidgetClass,
					povray_form, args, n);
	count++;

	/* The label for the look_at vector. */
	n = m;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNlabel, "look_at");				n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("lookatLabel", labelWidgetClass,
					povray_form, args, n);
	count++;

	/* The label for the sky vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "sky");					n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("skyLabel", labelWidgetClass,
					povray_form, args, n);
	count++;

	/* The label for the direction vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "direction");			n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("directionLabel", labelWidgetClass,
					povray_form, args, n);
	count++;

	/* The label for the up vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "up");					n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	labels[count] = XtCreateManagedWidget("upLabel", labelWidgetClass,
					povray_form, args, n);
	count++;

	/* The label for the right vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "right");				n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("rightLabel", labelWidgetClass,
					povray_form, args, n);
	count++;

	Match_Widths(labels, count);

	/* Need the size of the labels to get the string size right. */
	n = 0;
	XtSetArg(args[n], XtNheight, &label_height);	n++;
	XtGetValues(top_label, args, n);

	count = 0;
	/* All the strings now.  What joy. */
	n = 0;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainLeft);		n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);		n++;
	XtSetArg(args[n], XtNresizable, TRUE);			n++;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNresize, XawtextResizeWidth);	n++;
	m = n;

	XtSetArg(args[n], XtNstring, camera_text[location_index]);	n++;
	XtSetArg(args[n], XtNfromVert, top_label);					n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	pov_text[location_index] = XtCreateManagedWidget("locationText",
						asciiTextWidgetClass, povray_form, args, n);
	XtOverrideTranslations(pov_text[location_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[look_at_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	pov_text[look_at_index] = XtCreateManagedWidget("lookatText",
						asciiTextWidgetClass, povray_form, args, n);
	XtOverrideTranslations(pov_text[look_at_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[look_up_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	pov_text[look_up_index] = XtCreateManagedWidget("lookupText",
						asciiTextWidgetClass, povray_form, args, n);
	XtOverrideTranslations(pov_text[look_up_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[eye_dist_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	pov_text[eye_dist_index] = XtCreateManagedWidget("eyeText",
						asciiTextWidgetClass, povray_form, args, n);
	XtOverrideTranslations(pov_text[eye_dist_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[window_up_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	pov_text[window_up_index] = XtCreateManagedWidget("upText",
						asciiTextWidgetClass, povray_form, args, n);
	XtOverrideTranslations(pov_text[window_up_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[window_right_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);				n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);					n++;
	pov_text[window_right_index] = XtCreateManagedWidget("rightText",
						asciiTextWidgetClass, povray_form, args, n);
	XtOverrideTranslations(pov_text[window_right_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	Create_Buttons(povray_form, pov_text[window_right_index]);
	povray_default = default_toggle;
}


static void
Create_Radiance_Shell()
{
	Dimension	label_height;
	Widget	top_label;
	Widget	radiance_form;
	Widget	labels[5];
	Arg		args[15];
	int		count;
	int		m, n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Radiance / Renderman Camera");	n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	radiance_shell = XtCreatePopupShell("radianceCameraShell",
						transientShellWidgetClass, main_window.shell, args, n);

	/* Create the form to put all 17 widgets into. */
	n = 0;
	radiance_form = XtCreateManagedWidget("radianceForm", formWidgetClass,
					radiance_shell, args, n);

	/* Add the label at the top. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Radiance / Renderman Camera");	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);		n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	top_label = XtCreateManagedWidget("radianceCameraLabel", labelWidgetClass,
				radiance_form, args, n);

	/* Common args for all the labels. */
	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;
	m = n;

	count = 0;
	/* The label for the eyep vector. */
	XtSetArg(args[n], XtNlabel, "Viewpoint (vp)");	n++;
	XtSetArg(args[n], XtNfromVert, top_label);		n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("viewpointLabel", labelWidgetClass,
					radiance_form, args, n);
	count++;

	/* The label for the lookp vector. */
	n = m;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNlabel, "Direction (vd)");		n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	labels[count] = XtCreateManagedWidget("directionLabel", labelWidgetClass,
					radiance_form, args, n);
	count++;

	/* The label for the up vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Up (vu)");				n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);					n++;
	labels[count] = XtCreateManagedWidget("upLabel", labelWidgetClass,
					radiance_form, args, n);
	count++;

	/* The label for the fov vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "FOV (vh)");			n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	labels[count] = XtCreateManagedWidget("fovLabel", labelWidgetClass,
					radiance_form, args, n);
	count++;

	Match_Widths(labels, count);

	/* Need the height of the labels to get the string size right. */
	n = 0;
	XtSetArg(args[n], XtNheight, &label_height);	n++;
	XtGetValues(top_label, args, n);

	count = 0;
	n = 0;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainLeft);		n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);		n++;
	XtSetArg(args[n], XtNresizable, TRUE);			n++;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNresize, XawtextResizeWidth);	n++;
	m = n;

	XtSetArg(args[n], XtNstring, camera_text[location_index]);	n++;
	XtSetArg(args[n], XtNfromVert, top_label);					n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	rad_text[location_index] = XtCreateManagedWidget("locationText",
					asciiTextWidgetClass, radiance_form, args, n);
	XtOverrideTranslations(rad_text[location_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[look_at_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	rad_text[look_at_index] = XtCreateManagedWidget("lookatText",
					asciiTextWidgetClass, radiance_form, args, n);
	XtOverrideTranslations(rad_text[look_at_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[look_up_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	rad_text[look_up_index] = XtCreateManagedWidget("lookupText",
					asciiTextWidgetClass, radiance_form, args, n);
	XtOverrideTranslations(rad_text[look_up_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[window_up_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	rad_text[window_up_index] = XtCreateManagedWidget("upText",
					asciiTextWidgetClass, radiance_form, args, n);
	XtOverrideTranslations(rad_text[window_up_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	Create_Buttons(radiance_form, rad_text[window_up_index]);
	radiance_default = default_toggle;
}

static void
Create_Genray_Shell()
{
	Dimension	label_height;
	Widget	top_label;
	Widget	genray_form;
	Widget	labels[6];
	Arg		args[15];
	int		count;
	int		m, n;


	n = 0;
	XtSetArg(args[n], XtNtitle, "Genray Camera");	n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	genray_shell = XtCreatePopupShell("genrayCameraShell",
						transientShellWidgetClass, main_window.shell, args, n);

	/* Create the form to put all 17 widgets into. */
	n = 0;
	genray_form = XtCreateManagedWidget("genrayForm", formWidgetClass,
					genray_shell, args, n);

	/* Add the label at the top. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Genray Camera");	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);		n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	top_label = XtCreateManagedWidget("genrayCameraLabel", labelWidgetClass,
				genray_form, args, n);

	/* Common args for all the labels. */
	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;
	m = n;

	count = 0;
	/* The label for the lookfrom vector. */
	XtSetArg(args[n], XtNlabel, "lookfrom");	n++;
	XtSetArg(args[n], XtNfromVert, top_label);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("lookfromLabel", labelWidgetClass,
					genray_form, args, n);
	count++;

	/* The label for the look_at vector. */
	n = m;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNlabel, "lookat");				n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("lookatLabel", labelWidgetClass,
					genray_form, args, n);
	count++;

	/* The label for the sky vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "lookup");					n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("upLabel", labelWidgetClass,
					genray_form, args, n);
	count++;

	/* The label for the direction vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "eyedist");			n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("eyedistLabel", labelWidgetClass,
					genray_form, args, n);
	count++;

	/* The label for the up vector. */
	n = m;
	XtSetArg(args[n], XtNlabel, "window");				n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("windowLabel", labelWidgetClass,
					genray_form, args, n);
	count++;

	Match_Widths(labels, count);

	/* Need the size of the labels to get the string size right. */
	n = 0;
	XtSetArg(args[n], XtNheight, &label_height);	n++;
	XtGetValues(top_label, args, n);

	count = 0;
	/* All the string now.  What joy. */
	n = 0;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainLeft);		n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);		n++;
	XtSetArg(args[n], XtNresizable, TRUE);			n++;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNresize, XawtextResizeWidth);	n++;
	m = n;

	XtSetArg(args[n], XtNstring, camera_text[location_index]);	n++;
	XtSetArg(args[n], XtNfromVert, top_label);					n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	gen_text[location_index] = XtCreateManagedWidget("locationText",
					asciiTextWidgetClass, genray_form, args, n);
	XtOverrideTranslations(gen_text[location_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[look_at_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	gen_text[look_at_index] = XtCreateManagedWidget("lookatText",
					asciiTextWidgetClass, genray_form, args, n);
	XtOverrideTranslations(gen_text[look_at_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[look_up_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	gen_text[look_up_index] = XtCreateManagedWidget("lookupText",
					asciiTextWidgetClass, genray_form, args, n);
	XtOverrideTranslations(gen_text[look_up_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[eye_dist_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	gen_text[eye_dist_index] = XtCreateManagedWidget("eyeText",
					asciiTextWidgetClass, genray_form, args, n);
	XtOverrideTranslations(gen_text[eye_dist_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	n = m;
	XtSetArg(args[n], XtNstring, camera_text[window_up_index]);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);				n++;
	gen_text[window_up_index] = XtCreateManagedWidget("upText",
					asciiTextWidgetClass, genray_form, args, n);
	XtOverrideTranslations(gen_text[window_up_index],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	Create_Buttons(genray_form, gen_text[window_up_index]);
	genray_default = default_toggle;
}



static void
Create_Buttons(Widget parent, Widget top_right)
{
	Widget	viewport_button;
	Widget	done_button;
	Widget	cancel_button;
	Arg		args[15];
	int		m, n;
	Dimension	max_width, width;

	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;
	m = n;

	/* The buttons at the bottom right. */
	/* Viewport->camera. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Viewport");	n++;
	XtSetArg(args[n], XtNfromVert, top_right);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	viewport_button = XtCreateManagedWidget("viewportButton",
						commandWidgetClass, parent, args, n);
	XtAddCallback(viewport_button, XtNcallback, Viewport_To_Camera_Callback,
					NULL);

	/* Done button. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Done");				n++;
	XtSetArg(args[n], XtNfromVert, viewport_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	done_button = XtCreateManagedWidget("doneButton",
					commandWidgetClass, parent, args, n);
	XtAddCallback(done_button, XtNcallback, Apply_Camera_Callback, NULL);

	/* The buttons at the bottom right. */
	/* Default camera toggle. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Default");				n++;
	XtSetArg(args[n], XtNfromVert, top_right);			n++;
	XtSetArg(args[n], XtNfromHoriz, viewport_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	default_toggle = XtCreateManagedWidget("defaultToggle",
						toggleWidgetClass, parent, args, n);
	XtAddCallback(default_toggle, XtNcallback, Default_Camera_Callback, NULL);

	/* Cancel button. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Cancel");			n++;
	XtSetArg(args[n], XtNfromVert, default_toggle);	n++;
	XtSetArg(args[n], XtNfromHoriz, done_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	cancel_button = XtCreateManagedWidget("cancelButton",
					commandWidgetClass, parent, args, n);
	XtAddCallback(cancel_button, XtNcallback, Cancel_Camera_Callback, NULL);

	/* Make them all the same width. */
	max_width = 0;
	XtSetArg(args[0], XtNwidth, &width);
	XtGetValues(viewport_button, args, 1);
	if ( width > max_width ) max_width = width;
	XtGetValues(done_button, args, 1);
	if ( width > max_width ) max_width = width;
	XtGetValues(default_toggle, args, 1);
	if ( width > max_width ) max_width = width;
	XtGetValues(cancel_button, args, 1);
	if ( width > max_width ) max_width = width;

	XtSetArg(args[0], XtNwidth, max_width);
	XtSetValues(viewport_button, args, 1);
	XtSetValues(done_button, args, 1);
	XtSetValues(default_toggle, args, 1);
	XtSetValues(cancel_button, args, 1);
}


static void
Cancel_Camera_Callback(Widget w, XtPointer a, XtPointer b)
{
	XtPopdown(camera_dialog_shell);
}


static void
Viewport_To_Camera_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	Camera	new_cam;

	new_cam.type = camera.type;

	/* Turn off the default (if on). */
	XtVaSetValues(default_toggle, XtNstate, FALSE, NULL);

	Viewport_To_Camera(&(main_window.viewport), main_window.view_widget,
					   &new_cam, FALSE);

	/* Update the strings if necessary. */
	Update_Camera_Strings(&new_cam, &(main_window.viewport));

}

/*
**	Converts a known Viewport vp into a camera specification.
**	Finds all aspects of the camera, regardless of which raytracer is defined.
*/
void
Viewport_To_Camera(ViewportPtr vp, Widget widget, Camera *cam,
				   Boolean use_true_width)
{
	Dimension	width, height;
	Vector	max_pt;
	int		mag;
	Vector	temp_v;
	Arg		args[5];
	int		n;

	/* Work out the location. */
	VScalarMul(vp->view_from, vp->view_distance + vp->eye_distance, temp_v);
	VAdd(temp_v, vp->view_at, cam->location);

	/* look_at has an equivalent. */
	cam->look_at = vp->view_at;

	/* look_up is also equiv. */
	cam->look_up = vp->view_up;

	/* horiz fov is a bit of a pain. */
	/* It is derived from the maximum point that will appear on the screen.
	*/
	if ( ! use_true_width )
	{
		if ( ! ( width = vp->scr_width ) || ! (height = vp->scr_height ) )
		{
			n = 0;
			XtSetArg(args[n], XtNdesiredWidth, &width);		n++;
			XtSetArg(args[n], XtNdesiredHeight, &height);	n++;
			XtGetValues(widget, args, n);
		}
	}
	n = 0;
	if ( use_true_width || width == 0 || height == 0 )
	{
		XtSetArg(args[n], XtNwidth, &width);	n++;
		XtSetArg(args[n], XtNheight, &height);	n++;
	}
	XtSetArg(args[n], XtNmagnification, &mag);		n++;
	XtGetValues(widget, args, n);

	/* Work out what the max point is (in view coords). */
	max_pt.x = ((double)width / 2 ) / (double)mag;
	max_pt.y = ((double)height / 2 ) / (double)mag;

	/* We now have the following triangle. */
	/*
	**		-max_pt.x      	      max_pt.x	_
	**			\						/	|
	**			 \					   /	|
	**			  \					  /		|
	**			   \				 /		|
	**				\				/		|
	**				 \			   /	eye_dist
	**				  \			  /			|
	**				   \ theta	 /			|
	**					\		/			|
	**					 \	   /			|
	**					location			-
	**
	*/
	cam->horiz_fov = 360 * atan( max_pt.x / vp->eye_distance ) / M_PI;

	/* Vert fov comes from horiz fov and the aspect-ratio of the screen. */
	/* Actually I choose to compute it separately.	*/
	cam->vert_fov = 360 * atan( max_pt.y / vp->eye_distance ) / M_PI;

	/* eye_dist is a copy. */
	cam->eye_dist = vp->eye_distance;

	/* For window up and right we want just the max_pt calculated above. */
	cam->window_up = max_pt.y * 2;
	cam->window_right = max_pt.x * 2;

	/* Screen size is the current desired size. */
	cam->scr_width = width;
	cam->scr_height = height;

}



/*	void
**	Update_Camera_Strings()
**	Resets the strings displayed by the dialog box.  Redraws them too.
*/
static void
Update_Camera_Strings(Camera *new_vals, Viewport *vp)
{
	XawTextBlock	text_block;
	Vector			temp_v;
	int				old_length;


	/* For this to be called the relevant text widgets must have been
	** created.  You can consider this a statement of fact or an
	** excuse for crashing otherwise.
	*/

	/* Set all the strings which are common to each raytracer. */
	text_block.firstPos = 0;
	text_block.format = FMT8BIT;

	/* Location. */
	old_length = strlen(camera_text[location_index]);
	sprintf(camera_text[location_index], "%1.3g %1.3g %1.3g",
			new_vals->location.x, new_vals->location.y, new_vals->location.z);
	text_block.length = strlen(camera_text[location_index]);
	text_block.ptr = camera_text[location_index];
	if ( ray_text[location_index] )
		XawTextReplace(ray_text[location_index], 0, old_length + 1,&text_block);
	if ( pov_text[location_index] )
		XawTextReplace(pov_text[location_index], 0, old_length + 1,&text_block);
	if ( rad_text[location_index] )
		XawTextReplace(rad_text[location_index], 0, old_length + 1,&text_block);
	if ( gen_text[location_index] )
		XawTextReplace(gen_text[location_index], 0, old_length + 1,&text_block);

	/* Look at. */
	old_length = strlen(camera_text[look_at_index]);
	if ( camera.type == Radiance || camera.type == Renderman )
	{
		VSub(new_vals->look_at, new_vals->location, temp_v);
		sprintf(camera_text[look_at_index], "%1.3g %1.3g %1.3g",
				temp_v.x, temp_v.y, temp_v.z);
	}
	else
		sprintf(camera_text[look_at_index], "%1.3g %1.3g %1.3g",
				new_vals->look_at.x, new_vals->look_at.y, new_vals->look_at.z);
	text_block.length = strlen(camera_text[look_at_index]);
	text_block.ptr = camera_text[look_at_index];
	if ( ray_text[look_at_index] )
		XawTextReplace(ray_text[look_at_index], 0, old_length + 1, &text_block);
	if ( pov_text[look_at_index] )
		XawTextReplace(pov_text[look_at_index], 0, old_length + 1, &text_block);
	if ( rad_text[look_at_index] )
		XawTextReplace(rad_text[look_at_index], 0, old_length + 1, &text_block);
	if ( gen_text[look_at_index] )
		XawTextReplace(gen_text[look_at_index], 0, old_length + 1, &text_block);

	/* Look up. */
	old_length = strlen(camera_text[look_up_index]);
	sprintf(camera_text[look_up_index], "%1.3g %1.3g %1.3g",
			new_vals->look_up.x, new_vals->look_up.y, new_vals->look_up.z);
	text_block.length = strlen(camera_text[look_up_index]);
	text_block.ptr = camera_text[look_up_index];
	if ( ray_text[look_up_index] )
		XawTextReplace(ray_text[look_up_index], 0, old_length + 1, &text_block);
	if ( pov_text[look_up_index] )
		XawTextReplace(pov_text[look_up_index], 0, old_length + 1, &text_block);
	if ( rad_text[look_up_index] )
		XawTextReplace(rad_text[look_up_index], 0, old_length + 1, &text_block);
	if ( gen_text[look_up_index] )
		XawTextReplace(gen_text[look_up_index], 0, old_length + 1, &text_block);


	/* Now we get specific. */
	switch ( new_vals->type )
	{
		case Rayshade:
			/* FOV. */
			old_length = strlen(camera_text[window_up_index]);
			sprintf(camera_text[window_up_index], "%1.3g", new_vals->horiz_fov);
			text_block.length = strlen(camera_text[window_up_index]);
			text_block.ptr = camera_text[window_up_index];
			XawTextReplace(ray_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( pov_text[window_up_index] )
				XawTextReplace(pov_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( rad_text[window_up_index] )
				XawTextReplace(rad_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( gen_text[window_up_index] )
				XawTextReplace(gen_text[window_up_index], 0, old_length + 1,
							&text_block);

			/* eye_dist. */
			old_length = strlen(camera_text[eye_dist_index]);
			sprintf(camera_text[eye_dist_index], "%1.3g", new_vals->eye_dist);
			text_block.length = strlen(camera_text[eye_dist_index]);
			text_block.ptr = camera_text[eye_dist_index];
			XawTextReplace(ray_text[eye_dist_index], 0, old_length + 1,
							&text_block);
			if ( pov_text[eye_dist_index] )
				XawTextReplace(pov_text[eye_dist_index], 0, old_length + 1,
							&text_block);
			if ( gen_text[eye_dist_index] )
				XawTextReplace(gen_text[eye_dist_index], 0, old_length + 1,
							&text_block);

			Match_Widths(ray_text, eye_dist_index + 1);

			break;

		case POVray:
			/* Direction. */
			/* Just make it the default direction with the right length. */
			VNew(0, 0, new_vals->eye_dist, temp_v);
			old_length = strlen(camera_text[eye_dist_index]);
			sprintf(camera_text[eye_dist_index], "%1.3g %1.3g %1.3g",
					temp_v.x, temp_v.y, temp_v.z);
			text_block.length = strlen(camera_text[eye_dist_index]);
			text_block.ptr = camera_text[eye_dist_index];
			XawTextReplace(pov_text[eye_dist_index], 0, old_length + 1,
							&text_block);
			if ( ray_text[eye_dist_index] )
				XawTextReplace(ray_text[eye_dist_index], 0, old_length + 1,
							&text_block);
			if ( gen_text[eye_dist_index] )
				XawTextReplace(gen_text[eye_dist_index], 0, old_length + 1,
							&text_block);

			/* Up. */
			VNew(new_vals->window_up, 0, 0, temp_v);
			old_length = strlen(camera_text[window_up_index]);
			sprintf(camera_text[window_up_index], "%1.3g %1.3g %1.3g",
					temp_v.x, temp_v.y, temp_v.z);
			text_block.length = strlen(camera_text[window_up_index]);
			text_block.ptr = camera_text[window_up_index];
			XawTextReplace(pov_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( ray_text[window_up_index] )
				XawTextReplace(ray_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( rad_text[window_up_index] )
				XawTextReplace(rad_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( gen_text[window_up_index] )
				XawTextReplace(gen_text[window_up_index], 0, old_length + 1,
							&text_block);

			/* Right. */
			VNew(0, - new_vals->window_right, 0, temp_v);
			old_length = strlen(camera_text[window_right_index]);
			sprintf(camera_text[window_right_index], "%1.3g %1.3g %1.3g",
					temp_v.x, temp_v.y, temp_v.z);
			text_block.length = strlen(camera_text[window_right_index]);
			text_block.ptr = camera_text[window_right_index];
			XawTextReplace(pov_text[window_right_index], 0, old_length + 1,
							&text_block);

			Match_Widths(pov_text, window_right_index + 1);

			break;

		case Radiance:
		case Renderman:
			/* FOV. */
			old_length = strlen(camera_text[window_up_index]);
			sprintf(camera_text[window_up_index], "%1.3g", new_vals->horiz_fov);
			text_block.length = strlen(camera_text[window_up_index]);
			text_block.ptr = camera_text[window_up_index];
			XawTextReplace(rad_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( ray_text[window_up_index] )
				XawTextReplace(ray_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( pov_text[window_up_index] )
				XawTextReplace(pov_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( gen_text[window_up_index] )
				XawTextReplace(gen_text[window_up_index], 0, old_length + 1,
							&text_block);

			Match_Widths(rad_text, window_up_index + 1);

			break;

		case Genray:
		case Genscan:
			/* Eye distance. */
			old_length = strlen(camera_text[eye_dist_index]);
			sprintf(camera_text[eye_dist_index], "%1.3g", new_vals->eye_dist);
			text_block.length = strlen(camera_text[eye_dist_index]);
			text_block.ptr = camera_text[eye_dist_index];
			XawTextReplace(gen_text[eye_dist_index], 0, old_length + 1,
							&text_block);
			if ( ray_text[eye_dist_index] )
				XawTextReplace(ray_text[eye_dist_index], 0, old_length + 1,
							&text_block);
			if ( pov_text[eye_dist_index] )
				XawTextReplace(pov_text[eye_dist_index], 0, old_length + 1,
							&text_block);

			/* Window. */
			old_length = strlen(camera_text[window_up_index]);
			sprintf(camera_text[window_up_index], "%1.3g %1.3g",
					new_vals->window_right, new_vals->window_up);
			text_block.length = strlen(camera_text[window_up_index]);
			text_block.ptr = camera_text[window_up_index];
			XawTextReplace(gen_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( ray_text[window_up_index] )
				XawTextReplace(ray_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( pov_text[window_up_index] )
				XawTextReplace(pov_text[window_up_index], 0, old_length + 1,
							&text_block);
			if ( rad_text[window_up_index] )
				XawTextReplace(rad_text[window_up_index], 0, old_length + 1,
							&text_block);

			Match_Widths(gen_text, eye_dist_index + 1);

			break;

		case NoTarget:
			break;
	}

}



void
Camera_To_Viewport(Camera *src, ViewportPtr viewport)
{
	/* Set the view_from vector. */
	VSub(src->location, src->look_at, viewport->view_from);

	/* Set view_at and view_up. */
	viewport->view_at = src->look_at;
	viewport->view_up = src->look_up;

	/* view_distance is the length of the view_from vector - eye_dist. */
	viewport->view_distance = VMod(viewport->view_from) - src->eye_dist;
	viewport->eye_distance = src->eye_dist;

	Build_Viewport_Transformation(viewport);
}


/*	void
**	Camera_To_Window(WindowInfoPtr window)
**	Converts the window so that it is aligned with the camera.  It may also
**	adjust the image size to match that which was in force when the camera
**	was defined.
**
**	This is as good a place as any for a speil about image size.
**	Image size is stored with the camera.
**	Image size is set/modified as follows:
**	- Initially it is set to the window size as it is realized.
**	- When a file is loaded it is set to what the file says.
**	- When a camera is defined, it is reset accordingly.
**	- When it is explicitly set it is changed on the screen BUT NOT in
**		the camera structure.
**	- When a viewport is switched to the camera, the image size is reset
**		from what the camera says.
**
**	NOTE: It would be nice if the user had access to the image size when they
**		edit a camera.  But I'm not feeling nice.
*/
void
Camera_To_Window(WindowInfoPtr window)
{
	int			new_magnification;

	/* Get the necessary data from the widget. */

	if ( camera.type == NoTarget )
	{
		Popup_Error("No camera defined!", window->shell, "Error");
		return;
	}

	Camera_To_Viewport(&camera, &(window->viewport));

	/* The magnification may also need changing.
	** It's simplest to work it out from the window up and right values.
	*/
	new_magnification =
			floor((double)camera.scr_width / (double)camera.window_right);

	/* Set the screen size to what the camera thinks. */
	XtVaSetValues(window->view_widget,
				XtNwidth, camera.scr_width,
				XtNheight, camera.scr_height,
				XtNdesiredWidth, camera.scr_width,
				XtNdesiredHeight, camera.scr_height,
				XtNmagnification, new_magnification, NULL);

	/* Redraw the screen. */
	View_Update(window, window->all_instances, CalcView);
	Update_Projection_Extents(window->all_instances);

}


/*	void
**	Default_Camera_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**	The callback invoked to set the camera to its default.
*/
static void
Default_Camera_Callback(Widget w, XtPointer a, XtPointer b)
{
	Boolean		set_state;
	int			mag;
	Vector		max_pt;

	XtVaGetValues(default_toggle, XtNstate, &set_state, NULL);

	if ( ! set_state )
	{
		camera.default_cam = FALSE;
		return;
	}

	XtVaGetValues(main_window.view_widget,
				XtNdesiredWidth, &(camera.scr_width),
				XtNdesiredHeight, &(camera.scr_height),
				XtNmagnification, &mag, NULL);
	if ( camera.scr_width == 0 || camera.scr_height == 0 )
		XtVaGetValues(main_window.view_widget,
					XtNwidth, &(camera.scr_width),
					XtNheight, &(camera.scr_height), NULL);


	max_pt.x = ((double)camera.scr_width / 2 ) / mag;
	max_pt.y = ((double)camera.scr_height / 2 ) / mag;

	camera.type = camera.type;

	camera.window_up = max_pt.y * 2;
	camera.window_right = max_pt.x * 2;

	switch ( camera.type )
	{
		case Rayshade:
			VNew(0, -10.0, 0, camera.location);
			VNew(0, 0, 0, camera.look_at);
			VNew(0, 0, 1.0, camera.look_up);
			camera.horiz_fov = 45;
			camera.vert_fov = 45 * (double)camera.scr_height /
								(double)camera.scr_width;
			camera.eye_dist = 10;
			break;

		case POVray:
			VNew(0, 0, 0, camera.location);
			VNew(0, 0, 1.0, camera.look_at);
			VNew(0, 1.0, 0, camera.look_up);
			camera.horiz_fov = 360 * atan( max_pt.x ) / M_PI;
			camera.vert_fov = 360 * atan( max_pt.y ) / M_PI;
			camera.eye_dist = 1;
			break;

		case Renderman:
			VNew(0, 0, 0, camera.location);
			VNew(0, 0, -1.0, camera.look_at);
			VNew(0, 1.0, 0, camera.look_up);
			camera.horiz_fov = 90;
			camera.vert_fov = 45 * (double)camera.scr_height /
								(double)camera.scr_width;
			break;

		case Radiance:
			/* No well defined defaults. Use Genrays. */
		case Genray:
		case Genscan:
			VNew(10.0, 0, 0, camera.location);
			VNew(0, 0, 0, camera.look_at);
			VNew(0, 0, 1.0, camera.look_up);
			camera.horiz_fov = 360 * atan( max_pt.x ) / M_PI;
			camera.vert_fov = 360 * atan( max_pt.y ) / M_PI;
			camera.eye_dist = 10;
			break;

		case NoTarget:
			break;

	}

	Update_Camera_Strings(&camera, &(main_window.viewport));

}

static void
Apply_Camera_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	Boolean	use_default;
	Vector	temp_v;

	XtPopdown(camera_dialog_shell);

	XtVaGetValues(default_toggle, XtNstate, &use_default, NULL);

	if ( use_default )
	{
		Default_Camera_Callback(NULL, NULL, NULL);
		return;
	}

	XtVaGetValues(main_window.view_widget,
				XtNdesiredWidth, &(camera.scr_width),
				XtNdesiredHeight, &(camera.scr_height), NULL);
	if ( camera.scr_width == 0 || camera.scr_height == 0 )
		XtVaGetValues(main_window.view_widget,
					XtNwidth, &(camera.scr_width),
					XtNheight, &(camera.scr_height), NULL);

	/* Need to parse all the strings. */
	/* The common ones first. */
	sscanf(camera_text[location_index], "%lf %lf %lf", &(camera.location.x),
			&(camera.location.y), &(camera.location.z));
	sscanf(camera_text[look_at_index], "%lf %lf %lf", &(camera.look_at.x),
			&(camera.look_at.y), &(camera.look_at.z));
	if ( camera.type == Radiance || camera.type == Renderman )
		VAdd(camera.location, camera.look_at, camera.look_at);
	sscanf(camera_text[look_up_index], "%lf %lf %lf", &(camera.look_up.x),
			&(camera.look_up.y), &(camera.look_up.z));

	/* Now life gets a little messy. */
	switch ( camera.type )
	{
		case Rayshade:
			sscanf(camera_text[window_up_index], "%lf", &(camera.horiz_fov));
			camera.vert_fov = camera.horiz_fov *
						(double)camera.scr_height / (double)camera.scr_width;
			sscanf(camera_text[eye_dist_index], "%lf", &(camera.eye_dist));
			camera.window_up = camera.eye_dist * 2 *
								tan(camera.vert_fov * M_PI / 360);
			camera.window_right = camera.eye_dist * 2 *
								tan(camera.horiz_fov * M_PI / 360);
			break;

		case POVray:
			sscanf(camera_text[eye_dist_index], "%lf %lf %lf",
					&(temp_v.x), &(temp_v.y), &(temp_v.z));
			camera.eye_dist = VMod(temp_v);
			sscanf(camera_text[window_up_index], "%lf %lf %lf",
					&(temp_v.x), &(temp_v.y), &(temp_v.z));
			camera.window_up = VMod(temp_v);
			sscanf(camera_text[window_right_index], "%lf %lf %lf",
					&(temp_v.x), &(temp_v.y), &(temp_v.z));
			camera.window_right = VMod(temp_v);
			camera.horiz_fov = 360 *
						atan( camera.window_right / camera.eye_dist ) / M_PI;
			camera.vert_fov = 360 *
						atan( camera.window_up / camera.eye_dist ) / M_PI;
			break;

		case Radiance:
		case Renderman:
			sscanf(camera_text[window_up_index], "%lf", &(camera.horiz_fov));
			camera.vert_fov = camera.horiz_fov *
						(double)camera.scr_height / (double)camera.scr_width;
			camera.eye_dist = main_window.viewport.eye_distance;
			camera.window_up = camera.eye_dist * 2 *
								tan(camera.vert_fov * M_PI / 360);
			camera.window_right = camera.eye_dist * 2 *
								tan(camera.horiz_fov * M_PI / 360);
			break;

		case Genray:
		case Genscan:
			sscanf(camera_text[eye_dist_index], "%lf", &(camera.eye_dist));
			sscanf(camera_text[window_up_index], "%lf %lf",
					&(camera.window_right), &(camera.window_up));
			camera.horiz_fov = 360 *
						atan( camera.window_right / 2 / camera.eye_dist ) /M_PI;
			camera.vert_fov = 360 *
						atan( camera.window_up / 2 / camera.eye_dist ) / M_PI;
			break;

		case NoTarget:
			break;
	}

	changed_scene = TRUE;

}


