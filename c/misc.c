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
**	misc.c : miscellaneous functions.
**
**	External Functions:
**	void
**	Reset_Dialog_Func(Widget, XtPointer, XtPointer);
**	Puts up the dialog box for reseting the world.
**
**	void
**	Image_Size_Callback(Widget w, XtPointer client_data, XtPointer call_data)
**	The callback invoked from the imagesize button in the view menu.
**
**	void
**	Target_Callback(Widget, XtPointer, XtPointer)
**	The callback invoked to change the target raytracer.
*/

#include <sced.h>
#include <instance_list.h>
#include <View.h>
#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Toggle.h>
#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif



/* Define the longest any valid width/height dimension string will be. */
#define MAX_DIM_STRING_LENGTH 10


extern void		Define_Camera_Callback(Widget, XtPointer, XtPointer);
extern void		Export_Callback(Widget, XtPointer, XtPointer);


static void 	Create_Reset_Dialog();
static void 	Create_Image_Dialog();
static void		Create_Target_Dialog();
static void		Copyright_Create_Dialog();

static void		Help_Create_Dialog();

static Widget	reset_dialog_shell = NULL;
static Widget	image_size_shell = NULL;
static Widget	target_dialog_shell = NULL;
static Widget	copyright_dialog_shell = NULL;
static Widget	help_dialog_shell = NULL;
static WindowInfoPtr	window;
static Boolean	clear;
static char		image_width_string[MAX_DIM_STRING_LENGTH];
static char		image_height_string[MAX_DIM_STRING_LENGTH];
static Widget	radio_toggle;
static Widget	width_text;
static Widget	height_text;
static Boolean	camera_when_finished;
static Boolean	export_when_finished;
static Boolean	export_animation_when_finished;

static void		Reset_Func(Widget, XtPointer, XtPointer);
static void		Set_Image_Size(Widget, XtPointer, XtPointer);
static void		Fit_Image_Size(Widget, XtPointer, XtPointer);


void
Reset_Dialog_Func(Widget w, XtPointer client_data, XtPointer call_data)
{
	window = (WindowInfoPtr)client_data;

	clear = FALSE;

	if ( ! changed_scene )
	{
		Reset_Func(NULL, NULL, NULL);
		return;
	}

	if ( ! reset_dialog_shell )
		Create_Reset_Dialog();

	SFpositionWidget(reset_dialog_shell);
	XtPopup(reset_dialog_shell, XtGrabExclusive);
}


void
Clear_Dialog_Func(Widget w, XtPointer client_data, XtPointer call_data)
{
	window = (WindowInfoPtr)client_data;

	clear = TRUE;

	if ( ! changed_scene )
	{
		Reset_Func(NULL, NULL, NULL);
		return;
	}

	if ( ! reset_dialog_shell )
		Create_Reset_Dialog();

	SFpositionWidget(reset_dialog_shell);
	XtPopup(reset_dialog_shell, XtGrabExclusive);
}


static void
Reset_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	if (w)
		XtPopdown(reset_dialog_shell);

	if ( window == &csg_window )
		CSG_Reset();
	else
		Destroy_World(!clear);
}


static void
Cancel_Reset_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	XtPopdown(reset_dialog_shell);
}

static void
Save_Reset_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	XtPopdown(reset_dialog_shell);

	if ( clear )
		Save_Dialog_Func(NULL, (void*)SAVE_CLEAR, NULL);
	else
		Save_Dialog_Func(NULL, (void*)SAVE_RESET, NULL);
}


/*	void
**	Create_Reset_Dialog()
**	Creates the popup shell to for use when quitting.
*/
static void
Create_Reset_Dialog()
{
	Widget	dialog_widget;
	Arg		args[5];
	int		n;


	reset_dialog_shell = XtCreatePopupShell("Reset / Clear",
						transientShellWidgetClass, main_window.shell, NULL, 0);

	/* Create the dialog widget to go inside the shell. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "The scene has changed:");	n++;
	dialog_widget = XtCreateManagedWidget("resetDialog", dialogWidgetClass,
						reset_dialog_shell, args, n);

	/* Add the button at the bottom of the dialog. */
	XawDialogAddButton(dialog_widget, "Save", Save_Reset_Func, NULL);
	XawDialogAddButton(dialog_widget, "Reset / Clear", Reset_Func, NULL);
	XawDialogAddButton(dialog_widget, "Cancel", Cancel_Reset_Func, NULL);

	XtVaSetValues(XtNameToWidget(dialog_widget, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(dialog_widget, "Save"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(dialog_widget, "Reset / Clear"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(dialog_widget, "Cancel"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(reset_dialog_shell);
}


/*	void
**	Image_Size_Callback(Widget w, XtPointer client_data, XtPointer call_data)
**	The callback invoked from the imagesize button in the view menu.
*/
void
Image_Size_Callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	XawTextBlock	text_block;
	int			old_length;
	Dimension	width, height;

	window = (WindowInfoPtr)client_data;
	if ( ! image_size_shell )
		Create_Image_Dialog();
	else
	{
		text_block.firstPos = 0;
		text_block.format = FMT8BIT;

		XtVaGetValues(window->view_widget,
					XtNdesiredWidth, &width,
					XtNdesiredHeight, &height, NULL);

		/* Set the strings. */
		old_length = strlen(image_width_string);
		sprintf(image_width_string, "%d", (int)width);
		text_block.length = strlen(image_width_string);
		text_block.ptr = image_width_string;
		XawTextReplace(width_text, 0, old_length + 1, &text_block);

		old_length = strlen(image_height_string);
		sprintf(image_height_string, "%d", (int)height);
		text_block.length = strlen(image_height_string);
		text_block.ptr = image_height_string;
		XawTextReplace(height_text, 0, old_length + 1, &text_block);
	}

	/* Set the position of the popup. */
	SFpositionWidget(image_size_shell);

	XtPopup(image_size_shell, XtGrabExclusive);
}


static void
Set_Image_Size(Widget widg, XtPointer cl_data, XtPointer ca_data)
{
	int			w, h;
	Dimension	width, height;
	Arg	args[4];
	int	n;


	XtPopdown(image_size_shell);

	n = 0;

	if (sscanf(image_width_string, "%d", &w) == 1)
	{
		width = (Dimension)w;
		XtSetArg(args[n], XtNdesiredWidth, width);	n++;
		XtSetArg(args[n], XtNwidth, width);			n++;
	}

	if (sscanf(image_height_string, "%d", &h) == 1)
	{
		height = (Dimension)h;
		XtSetArg(args[n], XtNdesiredHeight, height);	n++;
		XtSetArg(args[n], XtNheight, height);			n++;
	}

	XtSetValues(window->view_widget, args, n);

}


static void
Fit_Image_Size(Widget widg, XtPointer cl_data, XtPointer ca_data)
{
	Dimension	width, height;

	XtPopdown(image_size_shell);

	XtVaGetValues(XtParent(window->view_widget),
				XtNwidth, &width, XtNheight, &height, NULL);

	/* To force a geometry change we need to reset the width and height also. */
	XtVaSetValues(window->view_widget,
				XtNwidth, width,
				XtNheight, height,
				XtNdesiredWidth, width,
				XtNdesiredHeight, height, NULL);
}


static void
Cancel_Image_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	XtPopdown(image_size_shell);
}


/*	void
**	Create_Image_Dialog()
**	Creates the popup shell used to change the image size.
*/
static void
Create_Image_Dialog()
{
	Widget		dialog_form;
	Widget		top_label;
	Widget		width_label;
	Widget		height_label;
	Widget		buttons[3];
	Dimension	width, height;
	Dimension	label_height;
	Arg			args[15];
	int			n;


	n = 0;
	XtSetArg(args[n], XtNtitle, "Image Size");	n++;
	image_size_shell = XtCreatePopupShell("imageSize",
						transientShellWidgetClass, main_window.shell, args, n);

	/* Create the form to go inside the shell. */
	n = 0;
	dialog_form = XtCreateManagedWidget("imageSizeDialogForm", formWidgetClass,
					image_size_shell, args, n);

	/* Add the main label. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Image Size");	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);		n++;
	top_label = XtCreateManagedWidget("imageSizeLabel", labelWidgetClass,
				dialog_form, args, n);

	/* Need some information first. */
	n = 0;
	XtSetArg(args[n], XtNheight, &label_height);	n++;
	XtGetValues(top_label, args, n);
	n = 0;
	XtSetArg(args[n], XtNdesiredWidth, &width);		n++;
	XtSetArg(args[n], XtNdesiredHeight, &height);	n++;
	XtGetValues(main_window.view_widget, args, n);

	/* Set up the strings. */
	sprintf(image_width_string, "%0d", (int)width);
	sprintf(image_height_string, "%0d", (int)height);

	/* Common arguments. */
	n = 0;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);	n++;

	/* The width label. */
	n = 4;
	XtSetArg(args[n], XtNlabel, "Width ");		n++;
	XtSetArg(args[n], XtNfromVert, top_label);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);		n++;
	width_label = XtCreateManagedWidget("imageWidthLabel", labelWidgetClass,
					dialog_form, args, n);

	n = 0;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainLeft);		n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);		n++;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_DIM_STRING_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, image_width_string);	n++;
	XtSetArg(args[n], XtNfromVert, top_label);		n++;
	XtSetArg(args[n], XtNfromHoriz, width_label);	n++;
	width_text = XtCreateManagedWidget("imageSizeWidthText",
					asciiTextWidgetClass, dialog_form, args, n);
	XtOverrideTranslations(width_text,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	/* The height label. */
	n = 4;
	XtSetArg(args[n], XtNlabel, "Height");			n++;
	XtSetArg(args[n], XtNfromVert, width_text);		n++;
	XtSetArg(args[n], XtNborderWidth, 0);			n++;
	height_label = XtCreateManagedWidget("imageHeightLabel", labelWidgetClass,
					dialog_form, args, n);

	n = 0;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainLeft);		n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);		n++;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_DIM_STRING_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, image_height_string);	n++;
	XtSetArg(args[n], XtNfromVert, width_text);		n++;
	XtSetArg(args[n], XtNfromHoriz, height_label);	n++;
	height_text = XtCreateManagedWidget("imageSizeHeightText",
					asciiTextWidgetClass, dialog_form, args, n);
	XtOverrideTranslations(height_text,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	n = 0;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);	n++;

	/* The Done button.*/
	n = 4;
	XtSetArg(args[n], XtNlabel, "Done");			n++;
	XtSetArg(args[n], XtNfromVert, height_text);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[0] = XtCreateManagedWidget("imageSizeDoneButton",
					commandWidgetClass, dialog_form, args, n);
	XtAddCallback(buttons[0], XtNcallback, Set_Image_Size, NULL);

	/* The To Window button. */
	n = 4;
	XtSetArg(args[n], XtNlabel, "To Fit");			n++;
	XtSetArg(args[n], XtNfromVert, height_text);	n++;
	XtSetArg(args[n], XtNfromHoriz, buttons[0]);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[1] = XtCreateManagedWidget("imageSizeFitButton",
					commandWidgetClass, dialog_form, args, n);
	XtAddCallback(buttons[1], XtNcallback, Fit_Image_Size, NULL);

	/* The Cancel button.*/
	n = 4;
	XtSetArg(args[n], XtNlabel, "Cancel");			n++;
	XtSetArg(args[n], XtNfromVert, height_text);	n++;
	XtSetArg(args[n], XtNfromHoriz, buttons[1]);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[2] = XtCreateManagedWidget("imageSizeCancelButton",
					commandWidgetClass, dialog_form, args, n);
	XtAddCallback(buttons[2], XtNcallback, Cancel_Image_Func, NULL);

	XtRealizeWidget(image_size_shell);
}



/*	void
**	Target_Callback(Widget, XtPointer, XtPointer)
**	The callback invoked to change the target raytracer.
*/
void
Target_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	if ( ! target_dialog_shell )
		Create_Target_Dialog();

	/* Make sure that the right toggle is set. */
	if ( camera.type == NoTarget )
		XawToggleUnsetCurrent(radio_toggle);
	else
		XawToggleSetCurrent(radio_toggle, (XtPointer)camera.type);

	if ( cl_data != NULL  && ! strcmp ( cl_data, "Camera" ) )
		camera_when_finished = TRUE;
	else if ( cl_data != NULL  && ! strcmp ( cl_data, "Export Animation" ) )
		export_animation_when_finished = TRUE;
	else if ( cl_data != NULL  && ! strcmp ( cl_data, "Export" ) )
		export_when_finished = TRUE;
	else
	{
		camera_when_finished = FALSE;
		export_animation_when_finished = FALSE;
		export_when_finished = FALSE;
	}

	/* Set the position of the popup. */
	SFpositionWidget(target_dialog_shell);

	XtPopup(target_dialog_shell, XtGrabExclusive);

}


void
Select_Target_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	XtPopdown(target_dialog_shell);

	camera.type = (Raytracer)cl_data;

	if ( camera_when_finished )
		Define_Camera_Callback(NULL, NULL, NULL);

	if ( export_when_finished )
		Export_Callback(NULL, NULL, NULL);

	if ( export_animation_when_finished )
		Export_Animation(NULL, NULL, NULL, NULL);

}


/*	void
**	Create_Target_Dialog()
**	Creates the target dialog shell.  It is a label with a radio group
**	to decide which raytracer is the target.
*/
static void
Create_Target_Dialog()
{
	Widget	dialog_form;
	Widget	buttons[7];
	Arg		args[15];
	int		n, m, count;

	target_dialog_shell = XtCreatePopupShell("Target",
						transientShellWidgetClass, main_window.shell, NULL, 0);

	/* Create the form to go inside the shell. */
	n = 0;
	dialog_form = XtCreateManagedWidget("targetDialogForm", formWidgetClass,
					target_dialog_shell, args, n);

	count = 0;

	/* Add the main label. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Target Raytracer");	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);				n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);			n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	buttons[count++] = XtCreateManagedWidget("targetLabel", labelWidgetClass,
				dialog_form, args, n);

	/* Add the three toggles. */

	/* Rayshade's toggle. */
	n = 0;
	XtSetArg(args[n], XtNleft, XtChainLeft);				n++;
	XtSetArg(args[n], XtNright, XtChainLeft);				n++;
	XtSetArg(args[n], XtNtop, XtChainTop);					n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);				n++;
	XtSetArg(args[n], XtNlabel, "Rayshade");				n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)Rayshade);	n++;
	XtSetArg(args[n], XtNfromVert, buttons[count - 1]);		n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = radio_toggle = XtCreateManagedWidget("rayshadeToggle",
					toggleWidgetClass, dialog_form, args, n);
	n = 0;
	XtSetArg(args[n], XtNradioGroup, radio_toggle);	n++;
	XtSetValues(buttons[count], args, n);
	XtAddCallback(buttons[count], XtNcallback, Select_Target_Callback,
					(XtPointer)Rayshade);
	count++;

	/* Common args: */
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);	n++;
	m = n;

	/* POVray  toggle. */
	XtSetArg(args[n], XtNlabel, "POVray");				n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)POVray);	n++;
	XtSetArg(args[n], XtNfromVert, buttons[count - 1]);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = XtCreateManagedWidget("povrayToggle", toggleWidgetClass,
					dialog_form, args, n);
	XtAddCallback(buttons[count], XtNcallback, Select_Target_Callback,
				  (XtPointer)POVray);
	count++;

	n = m;
	XtSetArg(args[n], XtNlabel, "Radiance");				n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)Radiance);	n++;
	XtSetArg(args[n], XtNfromVert, buttons[count - 1]);		n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = XtCreateManagedWidget("radianceToggle", toggleWidgetClass,
					dialog_form, args, n);
	XtAddCallback(buttons[count], XtNcallback, Select_Target_Callback,
				  (XtPointer)Radiance);
	count++;

	n = m;
	XtSetArg(args[n], XtNlabel, "RenderMan");				n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)Renderman);	n++;
	XtSetArg(args[n], XtNfromVert, buttons[count - 1]);		n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = XtCreateManagedWidget("rendermanToggle", toggleWidgetClass,
					dialog_form, args, n);
	XtAddCallback(buttons[count], XtNcallback, Select_Target_Callback,
				  (XtPointer)Renderman);
	count++;

	n = m;
	XtSetArg(args[n], XtNlabel, "Genray");				n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)Genray);	n++;
	XtSetArg(args[n], XtNfromVert, buttons[count - 1]);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = XtCreateManagedWidget("genrayToggle", toggleWidgetClass,
					dialog_form, args, n);
	XtAddCallback(buttons[count], XtNcallback, Select_Target_Callback,
				  (XtPointer)Genray);
	count++;

	n = m;
	XtSetArg(args[n], XtNlabel, "Genscan");					n++;
	XtSetArg(args[n], XtNradioData, (XtPointer)Genscan);	n++;
	XtSetArg(args[n], XtNfromVert, buttons[count - 1]);		n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	buttons[count] = XtCreateManagedWidget("genscanToggle", toggleWidgetClass,
					dialog_form, args, n);
	XtAddCallback(buttons[count], XtNcallback, Select_Target_Callback,
				  (XtPointer)Genscan);
	count++;

	Match_Widths(buttons, count);

	XtRealizeWidget(target_dialog_shell);

}


void
Copyright_Remove_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(copyright_dialog_shell);
	XtDestroyWidget(copyright_dialog_shell);
	copyright_dialog_shell = NULL;
}

void
Copyright_Popup_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	if ( ! copyright_dialog_shell )
		Copyright_Create_Dialog();

	SFpositionWidget(copyright_dialog_shell);

	XtPopup(copyright_dialog_shell, XtGrabExclusive);
}

static void
Copyright_Create_Dialog()
{
	Widget	box;
	Widget	button;
	Arg	args[5];
	int	n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Copyright");		n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	copyright_dialog_shell = XtCreatePopupShell("copyrightShell",
						transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	box = XtCreateManagedWidget("copyrightBox", boxWidgetClass,
								copyright_dialog_shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNborderWidth, 0);	n++;
	XtSetArg(args[n], XtNlabel,
		"ScEd Version "VERSION", Copyright (C) 1994-1995 Stephen Chenney\n"
		"ScEdA patches copyright (C) 1995 Denis McLaughlin\n"
#ifdef SCED_DEBUG
		"Debug version\n"
#endif
		"\nScEd comes with ABSOLUTELY NO WARRANTY.\n"
		"This is free software, and you are welcome to redistribute it\n"
		"under certain conditions; Read the LICENCE document for details.\n"
		"\n"
 		"This program compiled " __DATE__ " " __TIME__ "\n"
		"\n"
		"The SelFile file selection interface is:\n"
		"Copyright 1989 Software Research Associates, Inc., Tokyo, Japan\n"
		"Author: Erik M. van der Poel (erik@sra.co.jp)\n"
		"\n"
		"The RenderMan(r) Interface Procedures and RIB Protocol are:\n"
		"Copyright 1988, 1989, Pixar.\n"
		"All rights reserved.\n"
		"RenderMan(r) is a registered trademark of Pixar\n");
		n++;
	XtCreateManagedWidget("copyrightLabel", labelWidgetClass, box, args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "OK");							n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	button = XtCreateManagedWidget("copyrightButton", commandWidgetClass,
								box, args, n);
	XtAddCallback(button, XtNcallback, Copyright_Remove_Callback, NULL);

	XtRealizeWidget(copyright_dialog_shell);
}


void
Help_Remove_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(help_dialog_shell);
	XtDestroyWidget(help_dialog_shell);
	help_dialog_shell = NULL;
}

void
Help_Popup_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	if ( ! help_dialog_shell )
		Help_Create_Dialog();

	SFpositionWidget(help_dialog_shell);

	XtPopup(help_dialog_shell, XtGrabNone);
}

static void
Help_Create_Dialog()
{
	Widget	box;
	Widget	button;
	Arg	args[5];
	int	n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Help");		n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	help_dialog_shell = XtCreatePopupShell("helpShell",
						transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	box = XtCreateManagedWidget("helpBox", boxWidgetClass,
								help_dialog_shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNborderWidth, 0);	n++;
	XtSetArg(args[n], XtNlabel,
		"                     ScEdA KeyBoard Help Dialog\n"
		"\n"
		"    Animation\n"
		"    ---------\n"
		"        a        -    perform wireframe animation\n"
		"        x        -    export animation\n"
		"\n"
		"    Keyframes\n"
		"    ---------\n"
		"       Left      -    move to previous keyframe (non-wrapping)\n"
		"      Right      -    move to next keyframe (wrapping)\n"
		"       Tab       -    clone current keyframe\n"
		"    Meta Tab     -    destroy current keyframe\n"
		"    Meta Up      -    increase all keyframe intervals by 1\n"
		"    Ctrl Up      -    increase next keyframe interval by 1\n"
		"    Meta Down    -    decrease all keyframe intervals by 1\n"
		"    Ctrl Down    -    decrease next keyframe interval by 1\n"
		"        z        -    zero next keyframe interval\n"
		"        Z        -    zero all keyframe intervals\n"
		"        f        -    fill all keyframe intervals to 20\n"
		"        c        -    clone system to current keyframe\n"
		"        C        -    clone system to all following keyframes\n"
		"     Ctrl c      -    clone system to next keyframe\n"
		"     Meta c      -    clone system to previous keyframe\n"
		"        d        -    delete selected from current keyframe\n"
		"        D        -    delete selected from following keyframes\n"
		"        A        -    copy attributes of selected to following keyframes\n"
		"\n"
		"    General\n"
		"    -------\n"
		"        e        -    begin/end edit session on selected object\n"
		"        q        -    conditional quit (main) close CSG window (CSG)\n"
		"        Q        -    unconditional quit\n"
		"        F1       -    this help screen\n"
		"        n        -    new object\n"
		"\n"
		"    Viewport\n"
		"    --------\n"
		"        v        -    begin/end viewpoint change\n"
		"        p        -    begin/end pan\n"
		"        l        -    change lookat point\n"
		"        y        -    synchronize viewports of following keyframes to current\n"
		"        Up       -    zooms in by 1%\n"
		"       Down      -    zooms out by 1%\n"
		"    Shift Up     -    zooms in by 25%\n"
		"    Shift Down   -    zooms out by 25%\n"
		"        r        -    recalls saved viewport\n"
		"\n");
		n++;
	XtCreateManagedWidget("helpLabel", labelWidgetClass, box, args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Dismiss");							n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	button = XtCreateManagedWidget("helpButton", commandWidgetClass,
								box, args, n);
	XtAddCallback(button, XtNcallback, Help_Remove_Callback, NULL);

	XtRealizeWidget(help_dialog_shell);
}
