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
**	preview.c : Functions to preview the picture.
*/

#include <unistd.h>
#include <sced.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/Toggle.h>
#include <View.h>

extern char   start_directory_name[];


static void	Create_Preview_Dialog();
static void Cancel_Preview(Widget, XtPointer, XtPointer);
static void Perform_Preview_Callback(Widget, XtPointer, XtPointer);

static WindowInfoPtr	current_window;

static Widget	preview_dialog = NULL;
static Widget	target_widget;
static Widget	all_widget;
static Widget	width_widget, height_widget;

static Raytracer	preview_target = NoTarget;

#define TEXT_LENGTH 10
static char	width_string[TEXT_LENGTH];
static char	height_string[TEXT_LENGTH];

char	*rayshade_path = RAYSHADE_PATH;
char	*rayshade_options = RAYSHADE_OPTIONS;
char	*povray_path = POVRAY_PATH;
char	*povray_options = POVRAY_OPTIONS;
char	*radiance_path = RADIANCE_PATH;
char	*radiance_options = RADIANCE_OPTIONS;
char	*renderman_path = RENDERMAN_PATH;
char	*renderman_options = RENDERMAN_OPTIONS;
char	*genray_path = GENRAY_PATH;
char	*genray_options = GENRAY_OPTIONS;
char	*genscan_path = GENSCAN_PATH;
char	*genscan_options = GENSCAN_OPTIONS;

/*	void
**	Preview_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**	Sets a raytracer to previewing the scene.  cl_data is assumed to be
**	a WindowInfoPtr.
*/
void
Preview_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	current_window = (WindowInfoPtr)cl_data;

	if ( ! preview_dialog )
		Create_Preview_Dialog();
	Preview_Sensitize(TRUE);

	SFpositionWidget(preview_dialog);
	XtPopup(preview_dialog, XtGrabExclusive);
}


void
Preview_Sensitize(Boolean state)
{
	if ( ! preview_dialog )
		Create_Preview_Dialog();

	XtSetSensitive(all_widget, state);
}


static void
Create_Preview_Dialog()
{
	Widget	form;
	Widget	buttons[6];
	Widget	width_label, height_label;
	Widget	go, cancel;
	Arg		args[15];
	int		n, m, count;
	Dimension	width, height;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Preview");			n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	preview_dialog = XtCreatePopupShell("previewDialog",
					transientShellWidgetClass, current_window->shell, args, n);

	n = 0;
	form = XtCreateManagedWidget("previewForm", formWidgetClass,
									preview_dialog, args, n);

	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;
	m = n;

	count = 0;
	n = m;
	XtSetArg(args[n], XtNlabel, "POVray");				n++;
	XtSetArg(args[n], XtNfromVert, NULL);				n++;
	XtSetArg(args[n], XtNfromHoriz, NULL);				n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)POVray);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	target_widget =
	buttons[count] = XtCreateManagedWidget("previewPOVray", toggleWidgetClass,
										form, args, n);
	count++;

	n = m;
	XtSetArg(args[n], XtNlabel, "Rayshade");				n++;
	XtSetArg(args[n], XtNfromVert, NULL);					n++;
	XtSetArg(args[n], XtNfromHoriz, buttons[count-1]);		n++;
	XtSetArg(args[n], XtNradioGroup, target_widget);		n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)Rayshade);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = XtCreateManagedWidget("previewRayshade", toggleWidgetClass,
										form, args, n);
	count++;

	n = m;
	XtSetArg(args[n], XtNlabel, "Radiance");				n++;
	XtSetArg(args[n], XtNfromVert, buttons[count-2]);		n++;
	XtSetArg(args[n], XtNfromHoriz, NULL);					n++;
	XtSetArg(args[n], XtNradioGroup, target_widget);		n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)Radiance);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = XtCreateManagedWidget("previewRadiance", toggleWidgetClass,
										form, args, n);
	count++;

	n = m;
	XtSetArg(args[n], XtNlabel, "RenderMan");				n++;
	XtSetArg(args[n], XtNfromVert, buttons[count-2]);		n++;
	XtSetArg(args[n], XtNfromHoriz, buttons[count-1]);		n++;
	XtSetArg(args[n], XtNradioGroup, target_widget);		n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)Renderman);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = XtCreateManagedWidget("previewRenderman",toggleWidgetClass,
										form, args, n);
	count++;

	n = m;
	XtSetArg(args[n], XtNlabel, "Genray");				n++;
	XtSetArg(args[n], XtNfromVert, buttons[count-2]);	n++;
	XtSetArg(args[n], XtNfromHoriz, NULL);				n++;
	XtSetArg(args[n], XtNradioGroup, target_widget);	n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)Genray);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = XtCreateManagedWidget("previewGenray", toggleWidgetClass,
										form, args, n);
	count++;

	n = m;
	XtSetArg(args[n], XtNlabel, "Genscan");				n++;
	XtSetArg(args[n], XtNfromVert, buttons[count-2]);	n++;
	XtSetArg(args[n], XtNfromHoriz, buttons[count-1]);	n++;
	XtSetArg(args[n], XtNradioGroup, target_widget);	n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)Genscan);n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = XtCreateManagedWidget("previewGenscan", toggleWidgetClass,
										form, args, n);
	count++;

	Match_Widths(buttons, count);

	/* Set up labels and strings. */

	/* Set the width/height strings. */
	XtVaGetValues(current_window->view_widget, XtNdesiredWidth, &width,
				  XtNdesiredHeight, &height, NULL);
	if ( ! width )
		XtVaGetValues(current_window->view_widget, XtNwidth, &width,
					  XtNheight, &height, NULL);
	sprintf(width_string, "%d", (int)width);
	sprintf(height_string, "%d", (int)height);

	/* The width label. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Width ");				n++;
	XtSetArg(args[n], XtNfromVert, buttons[count-2]);	n++;
	XtSetArg(args[n], XtNfromHoriz, NULL);				n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	buttons[0] =
	width_label = XtCreateManagedWidget("previewWidthLabel", labelWidgetClass,
					form, args, n);

	/* Width string. */
	n = m;
	XtSetArg(args[n], XtNeditType, XawtextEdit);		n++;
	XtSetArg(args[n], XtNlength, TEXT_LENGTH);			n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);		n++;
	XtSetArg(args[n], XtNstring, width_string);			n++;
	XtSetArg(args[n], XtNfromVert, buttons[count-2]);	n++;
	XtSetArg(args[n], XtNfromHoriz, width_label);		n++;
	width_widget = XtCreateManagedWidget("previewWidthText",
					asciiTextWidgetClass, form, args, n);
	XtOverrideTranslations(width_widget,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	n = m;
	XtSetArg(args[n], XtNlabel, "Height ");			n++;
	XtSetArg(args[n], XtNfromVert, width_widget);	n++;
	XtSetArg(args[n], XtNfromHoriz, NULL);			n++;
	XtSetArg(args[n], XtNborderWidth, 0);			n++;
	buttons[1] =
	height_label = XtCreateManagedWidget("previewHeightLabel", labelWidgetClass,
					form, args, n);

	/* Height string. */
	n = m;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, TEXT_LENGTH);		n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, height_string);	n++;
	XtSetArg(args[n], XtNfromVert, width_widget);	n++;
	XtSetArg(args[n], XtNfromHoriz, height_label);	n++;
	height_widget = XtCreateManagedWidget("previewHeightText",
					asciiTextWidgetClass, form, args, n);
	XtOverrideTranslations(height_widget,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	Match_Widths(buttons, 2);

	/* All toggle. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Preview All");		n++;
	XtSetArg(args[n], XtNfromVert, height_label);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	all_widget = XtCreateManagedWidget("previewGenray", toggleWidgetClass,
										form, args, n);

	n = m;
	XtSetArg(args[n], XtNlabel, "Go !");			n++;
	XtSetArg(args[n], XtNfromVert, all_widget);		n++;
	XtSetArg(args[n], XtNfromHoriz, NULL);			n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[0] =
	go = XtCreateManagedWidget("previewGo", commandWidgetClass,
								form, args, n);
	XtAddCallback(go, XtNcallback, Perform_Preview_Callback, NULL);

	n = m;
	XtSetArg(args[n], XtNlabel, "Cancel");		n++;
	XtSetArg(args[n], XtNfromVert, all_widget);	n++;
	XtSetArg(args[n], XtNfromHoriz, go);		n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[1] =
	cancel = XtCreateManagedWidget("previewCancel", commandWidgetClass,
									form, args, n);
	XtAddCallback(cancel, XtNcallback, Cancel_Preview, NULL);

	Match_Widths(buttons, 2);

	/* Set the target. */
	if ( camera.type != NoTarget )
		XawToggleSetCurrent(target_widget, (XtPointer)camera.type);

	XtRealizeWidget(preview_dialog);
}

static void
Cancel_Preview(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(preview_dialog);
}


static void
Perform_Preview_Callback(Widget w, XtPointer cl_data, XtPointer ca)
{
	InstanceList	instances;
	int			preview_width = 0;
	int			preview_height = 0;
	Boolean		preview_all;

	/* Check for a target. */
	preview_target = (Raytracer)XawToggleGetCurrent(target_widget);
	if ( preview_target == NoTarget )
	{
		Popup_Error("No target selected", current_window->shell, "Error");
		return;
	}

	XtPopdown(preview_dialog);

	XtVaGetValues(all_widget, XtNstate, &preview_all, NULL);
	if ( XtIsSensitive(all_widget) && preview_all )
		instances = current_window->all_instances;
	else
		instances = current_window->selected_instances;

	if ( ! instances )
	{
		Popup_Error("Nothing To Preview!", current_window->shell, "Error");
		return;
	}

	sscanf(width_string, "%d\n", &preview_width);
	sscanf(height_string, "%d\n", &preview_height);

	Perform_Preview(current_window, preview_target, instances,
					preview_width, preview_height);
}

void
Perform_Preview(WindowInfoPtr window, Raytracer target, InstanceList insts,
				int width, int height)
{
	SceneStruct	preview_scene;
	char		prompt_string[128];
	char		*infilename;
	FILE		*outfile;
	char		*system_string = NULL;
	InstanceList	elmt;
	Boolean			have_light = FALSE;

	if ( ! insts )
		return;

	preview_scene.instances = insts;

	switch ( target )
	{
		case Genray:
			if ( genray_path[0] == '\0' ) {
				Popup_Error("Genray does not exist on your system.",
							window->shell, "Error");
				return;
			}
			break;
		case Genscan:
			if ( genscan_path[0] == '\0' ) {
				Popup_Error("Genscan does not exist on your system.",
							window->shell, "Error");
				return;
			}
			break;
		case POVray:
			if ( povray_path[0] == '\0' )
			{
				Popup_Error("POVRay does not exist on your system.",
							window->shell, "Error");
				return;
			}
			break;
		case Rayshade:
			if ( rayshade_path[0] == '\0' )
			{
				Popup_Error("Rayshade does not exist on your system.",
							window->shell, "Error");
				return;
			}
			break;
		case Radiance:
			if ( radiance_path[0] == '\0' )
			{
				Popup_Error("Radiance does not exist on your system.",
							window->shell, "Error");
				return;
			}
			break;
		case Renderman:
			if ( renderman_path[0] == '\0' )
			{
				Popup_Error("RenderMan does not exist on your system.",
							window->shell, "Error");
				return;
			}
			break;
		default:;
	}

	preview_scene.camera.type = target;
	preview_scene.camera.default_cam = FALSE;
	Viewport_To_Camera(&(window->viewport), window->view_widget,
					   &(preview_scene.camera), TRUE);

	if ( width )
		preview_scene.camera.scr_width = (Dimension)width;
	if ( height )
		preview_scene.camera.scr_height = (Dimension)height;

	/* Try to find a light amongst the instances. */
	for ( elmt = preview_scene.instances ;
		  ! have_light && elmt ;
		  elmt = elmt->next )
		have_light = ( elmt->the_instance->o_parent->b_class == light_obj ) ||
				( elmt->the_instance->o_parent->b_class == spotlight_obj ) ||
				( elmt->the_instance->o_parent->b_class == arealight_obj );

	if ( have_light )
		preview_scene.light = NULL;
	else
	{
		preview_scene.light =
			Create_Instance(Get_Base_Object_From_Label("light"),
							"PreviewLight");

		/* Put a light near the eye position. */
		VScalarMul(preview_scene.camera.location, 1.2,
				   preview_scene.light->o_transform.displacement);
		((LightInfoPtr)preview_scene.light->o_attribs)->red =
		((LightInfoPtr)preview_scene.light->o_attribs)->green =
		((LightInfoPtr)preview_scene.light->o_attribs)->blue =
		1.0;
	}

	infilename = New(char, 24 + strlen(start_directory_name));
	sprintf(infilename, "%s/previewXXXXXX", start_directory_name);
	/* Get a temporary filename to use. */
	if ( ! ( infilename = (char*)mktemp(infilename) ) )
	{
		Popup_Error("Unable to create file name for preview",
					window->shell, "Error");
		return;
	}


	if ( ( outfile = fopen(infilename, "w")) == NULL )
	{
		sprintf(prompt_string,
			"Unable to open file %s for preview", infilename);
		Popup_Error(prompt_string, window->shell, "Error");
		return;
	}

	/* Store the filename for later deletion. */
	Save_Temp_Filename(infilename);

	preview_scene.ambient = ambient_light;

	if ( ! Export_File(outfile, infilename, &preview_scene, TRUE) )
		return;

	if ( preview_scene.light ) Destroy_Instance(preview_scene.light);

	switch ( target )
	{
		case Rayshade:
			system_string = New(char,
							strlen(rayshade_path) + strlen(rayshade_options) +
							strlen(infilename)  + 30);
			sprintf(system_string, "%s %s -O preview.rle < %s",
					rayshade_path, rayshade_options, infilename);
			sprintf(prompt_string,
					"Previewing using Rayshade output to preview.rle");
			break;
		case Radiance:
			system_string = New(char,
							strlen(radiance_path) + strlen(radiance_options) +
							strlen(infilename)  + 30);
			sprintf(system_string, "%s %s %s",
					radiance_path, radiance_options, infilename);
			sprintf(prompt_string,
					"Previewing using \"%s\"", system_string);
			break;
		case Renderman:
			system_string = New(char,
							strlen(renderman_path) + strlen(renderman_options) +
							strlen(infilename)  + 30);
			sprintf(system_string, "%s %s %s",
					renderman_path, renderman_options, infilename);
			sprintf(prompt_string,
					"Previewing using \"%s\" output to preview.tif",
					system_string);
			break;
		case POVray:
			system_string = New(char,
							strlen(povray_path) + strlen(povray_options) +
							strlen(infilename)  + 50);
			sprintf(system_string, "%s -w%d -h%d -I%s -Opreview.tga %s",
					povray_path, preview_scene.camera.scr_width,
					preview_scene.camera.scr_height, infilename,
					povray_options);
			sprintf(prompt_string,
					"Previewing using POVray output to preview.tga");
			break;
		case Genray:
			system_string = New(char,
							strlen(genray_path) + strlen(genray_options) +
							strlen(infilename)  + 30);
			sprintf(system_string, "%s %s %s",
					genray_path, genray_options, infilename);
			sprintf(prompt_string,
					"Previewing using Genray output to %s.tif", infilename);
			break;
		case Genscan:
			system_string = New(char,
							strlen(genscan_path) + strlen(genscan_options) +
							strlen(infilename)  + 30);
			sprintf(system_string, "%s %s %s",
					genscan_path, genscan_options, infilename);
			sprintf(prompt_string,
					"Previewing using Genscan");
			break;
		default: ;
	}

	/* Fork a shell to do the preview. */
	switch ( fork() )
	{
		case 0:	/* The child. */
			execl("/bin/sh", "sh", "-c", system_string, 0);
			/* Here if the exec failed. */
			sprintf(prompt_string, "Couldn't exec previewer");
			exit(1);
		case -1:
			Popup_Error("Couldn't exec previewer", window->shell,
						"Error");
			break;
		default:
			/* Popup_Error(prompt_string, window->shell, "Preview"); */
			break;
	}
	free(system_string);
	free(infilename);
}


