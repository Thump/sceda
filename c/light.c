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
**	light.c : Functions for creating and managing lights.
**
**	void
**	Popup_New_Light_Shell();
**	Pops up the shell needed to create a new light.
*/

#include <sced.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Toggle.h>

static void	Create_Ambient_Dialog();
static void	Set_Ambient_Callback(Widget, XtPointer, XtPointer);
static void	Cancel_Light_Callback(Widget, XtPointer, XtPointer);

static void	Create_Light_Dialog();
static void	Light_Intensity_Callback(Widget, XtPointer, XtPointer);

static void	Create_Spotlight_Dialog();

static void	Create_Arealight_Dialog();

static Widget	ambient_light_shell = NULL;
static Widget	ambient_dialog;

/* The ambient light in the scene. */
XColor		ambient_light = { 0, 0, 0 };

static Boolean	spotlight_after;	/* Whether to popup the spotlight dialog. */
static Boolean	arealight_after;	/* Whether to popup the arealight dialog. */

static InstanceList	instances;

#define MAX_STRING_LENGTH 24

static char		intensity_string[MAX_STRING_LENGTH];

static Widget	light_shell = NULL;
static Widget	light_dialog;

static Widget	spotlight_shell = NULL;
static Widget	spotlight_intensity_text;
static Widget	radius_text;
static char		radius_string[MAX_STRING_LENGTH];
static Widget	tightness_text;
static char		tightness_string[MAX_STRING_LENGTH];
static Widget	invert_toggle;

static Widget	arealight_shell = NULL;
static Widget	arealight_intensity_text;
static Widget	xnum_text;
static char		xnum_string[MAX_STRING_LENGTH];
static Widget	ynum_text;
static char		ynum_string[MAX_STRING_LENGTH];
static Widget	jitter_toggle;

void
Create_Light_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	Create_New_Object_From_Base(&main_window,
								Get_Base_Object_From_Label("light"), FALSE);
}


void
Create_Spotlight_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	Create_New_Object_From_Base(&main_window,
								Get_Base_Object_From_Label("spotlight"),
								FALSE);
}

void
Create_Arealight_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	Create_New_Object_From_Base(&main_window,
								Get_Base_Object_From_Label("arealight"),
								FALSE);
}

/*	void
**	Ambient_Light_Callback(Widget w, XtPointer cl, XtPointer ca)
**	Pops up the ambient light dialog.
*/
void
Ambient_Light_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	char	ambient_string[20];

	if ( ! ambient_light_shell )
		Create_Ambient_Dialog();

	sprintf(ambient_string, "%1.2g %1.2g %1.2g",
			(double)ambient_light.red / (double)MAX_UNSIGNED_SHORT,
			(double)ambient_light.green / (double)MAX_UNSIGNED_SHORT,
			(double)ambient_light.blue / (double)MAX_UNSIGNED_SHORT);

	XtVaSetValues(ambient_dialog, XtNvalue, ambient_string, NULL);

	SFpositionWidget(ambient_light_shell);

	XtPopup(ambient_light_shell, XtGrabExclusive);
}

/*	void
**	Create_Ambient_Dialog() 
**	Creates the shell used to set the ambient light.
*/
static void
Create_Ambient_Dialog()
{
	Arg	args[3];
	int	n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Ambient Light");	n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	ambient_light_shell =  XtCreatePopupShell("ambientShell",
						transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Ambient color:");	n++;
	XtSetArg(args[n], XtNvalue, "");				n++;
	ambient_dialog = XtCreateManagedWidget("ambientDialog", dialogWidgetClass,
							ambient_light_shell, args, n);

	XawDialogAddButton(ambient_dialog, "Done", Set_Ambient_Callback, NULL);
	XawDialogAddButton(ambient_dialog, "Cancel", Cancel_Light_Callback,
						(XtPointer)ambient_light_shell);

	XtOverrideTranslations(XtNameToWidget(ambient_dialog, "value"),
		XtParseTranslationTable(":<Key>Return: Ambient_Action()"));

	XtVaSetValues(XtNameToWidget(ambient_dialog, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(ambient_dialog, "Done"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(ambient_dialog, "Cancel"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(ambient_light_shell);
}



/*	void
**	Set_Ambient_Callback(Widget w, XtPointer cl, XtPointer ca)
**	Sets the ambient light from the dialog shell.
*/
static void
Set_Ambient_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	String	ambient_text;
	double	r, g, b;
	int		num_args;

	XtPopdown(ambient_light_shell);

	ambient_text = XawDialogGetValueString(ambient_dialog);

	num_args = sscanf(ambient_text, "%lf %lf %lf", &r, &g, &b);
	ambient_light.red = (short)(MAX_UNSIGNED_SHORT * r);
	if ( num_args < 2 )
		ambient_light.green = ambient_light.red;
	else
		ambient_light.green = (short)(MAX_UNSIGNED_SHORT * g);
	if ( num_args < 3 )
		ambient_light.blue = ambient_light.red;
	else
		ambient_light.blue = (short)(MAX_UNSIGNED_SHORT * b);

	changed_scene = TRUE;

}


void
Ambient_Action_Func(Widget w, XEvent *e, String *s, Cardinal *n)
{
	Set_Ambient_Callback(w, NULL, NULL);
}


/*	void
**	Cancel_Ambient_Callback(Widget w, XtPointer cl, XtPointer ca)
**	Cancels an ambience session.
*/
static void
Cancel_Light_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown((Widget)cl);

	if ( spotlight_after )
		Set_Spotlight_Attributes(instances, arealight_after);
	else if ( arealight_after )
		Set_Arealight_Attributes(instances);
}


static void
Light_Intensity_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	String	int_string;
	double	r = 0.0;
	double	g = 0.0;
	double	b = 0.0;
	int		num_args;
	InstanceList	elmt;

	XtPopdown(light_shell);

	int_string = XawDialogGetValueString(light_dialog);

	num_args = sscanf(int_string, "%lf %lf %lf", &r, &g, &b);
	if ( num_args < 3 )
		g = b = r;

	for ( elmt = instances ; elmt ; elmt = elmt->next )
		if ( elmt->the_instance->o_parent->b_class == light_obj )
		{
			((LightInfoPtr)elmt->the_instance->o_attribs)->red = r;
			((LightInfoPtr)elmt->the_instance->o_attribs)->green = g;
			((LightInfoPtr)elmt->the_instance->o_attribs)->blue = b;
		}

	changed_scene = TRUE;

	if ( spotlight_after )
		Set_Spotlight_Attributes(instances, arealight_after);
	else if ( arealight_after )
		Set_Arealight_Attributes(instances);

}


void
Light_Action_Func(Widget w, XEvent *e, String *s, Cardinal *n)
{
	Light_Intensity_Callback(w, NULL, NULL);
}


static void
Light_Set(LightInfoPtr light, double red, double green, double blue,
		  double val1, double val2, Boolean flag)
{
	light->red = red;
	light->green = green;
	light->blue = blue;
	light->val1 = val1;
	light->val2 = val2;
	light->flag = flag;
}


static void
Spotlight_Attributes_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	InstanceList	elmt;
	double	radius;
	double	tightness;
	double	red, green, blue;
	Boolean	invert;
	XtPopdown(spotlight_shell);

	if ( sscanf(intensity_string, "%lf %lf %lf", &red, &green, &blue) < 3 )
		blue = green = red;
	sscanf(radius_string, "%lf", &radius);
	sscanf(tightness_string, "%lf", &tightness);
	XtVaGetValues(invert_toggle, XtNstate, &invert, NULL);

	for ( elmt = instances ; elmt ; elmt = elmt->next )
		if ( elmt->the_instance->o_parent->b_class == spotlight_obj )
			Light_Set((LightInfoPtr)elmt->the_instance->o_attribs,
					  red, green, blue, radius, tightness, invert);

	changed_scene = TRUE;

	if ( arealight_after )
		Set_Arealight_Attributes(instances);
}


static void
Arealight_Attributes_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	InstanceList	elmt;
	int		xnum;
	int		ynum;
	double	red, green, blue;
	Boolean	jitter;

	XtPopdown(arealight_shell);

	if ( sscanf(intensity_string, "%lf %lf %lf", &red, &green, &blue) < 3 )
		blue = green = red;
	sscanf(xnum_string, "%d", &xnum);
	sscanf(ynum_string, "%d", &ynum);
	XtVaGetValues(jitter_toggle, XtNstate, &jitter, NULL);

	for ( elmt = instances ; elmt ; elmt = elmt->next )
		if ( elmt->the_instance->o_parent->b_class == arealight_obj )
			Light_Set((LightInfoPtr)elmt->the_instance->o_attribs,
					  red, green, blue, (double)xnum, (double)ynum, jitter);

	changed_scene = TRUE;
}

void
Set_Light_Attributes(InstanceList objects, Boolean do_spot, Boolean do_area)
{
	InstanceList	elmt;
	char	light_string[20];

	instances = objects;
	spotlight_after = do_spot;
	arealight_after = do_area;

	if ( ! light_shell )
		Create_Light_Dialog();

	for ( elmt = objects ;
		  elmt->the_instance->o_parent->b_class != light_obj ;
		  elmt = elmt->next );

	sprintf(light_string, "%1.2g %1.2g %1.2g",
			((LightInfoPtr)elmt->the_instance->o_attribs)->red,
			((LightInfoPtr)elmt->the_instance->o_attribs)->green,
			((LightInfoPtr)elmt->the_instance->o_attribs)->blue);

	XtVaSetValues(light_dialog, XtNvalue, light_string, NULL);

	SFpositionWidget(light_shell);

	XtPopup(light_shell, XtGrabExclusive);
}


void
Set_Spotlight_Attributes(InstanceList objects, Boolean do_area)
{
	InstanceList	elmt;
	XawTextBlock	text_block;
	int				old_length;

	instances = objects;
	arealight_after = do_area;

	spotlight_after = FALSE;

	if ( ! spotlight_shell )
		Create_Spotlight_Dialog();

	for ( elmt = objects ;
		  elmt->the_instance->o_parent->b_class != spotlight_obj ;
		  elmt = elmt->next );

	/* Set all the text strings. */
	text_block.firstPos = 0;
	text_block.format = FMT8BIT;
	old_length = strlen(intensity_string);
	sprintf(intensity_string, "%0.2g %0.2g %0.2g", 
			((LightInfoPtr)elmt->the_instance->o_attribs)->red,
			((LightInfoPtr)elmt->the_instance->o_attribs)->green,
			((LightInfoPtr)elmt->the_instance->o_attribs)->blue);
	text_block.length = strlen(intensity_string);
	text_block.ptr = intensity_string;
	XawTextReplace(spotlight_intensity_text, 0, old_length + 1, &text_block);

	old_length = strlen(radius_string);
	sprintf(radius_string, "%0.2g",
			((LightInfoPtr)elmt->the_instance->o_attribs)->val1);
	text_block.length = strlen(radius_string);
	text_block.ptr = radius_string;
	XawTextReplace(radius_text, 0, old_length + 1, &text_block);

	old_length = strlen(tightness_string);
	sprintf(tightness_string, "%0.2g",
			((LightInfoPtr)elmt->the_instance->o_attribs)->val2);
	text_block.length = strlen(tightness_string);
	text_block.ptr = tightness_string;
	XawTextReplace(tightness_text, 0, old_length + 1, &text_block);

	/* Set the inverse toggle. */
	XtVaSetValues(invert_toggle, XtNstate,
				  ((LightInfoPtr)elmt->the_instance->o_attribs)->flag, NULL);

	SFpositionWidget(spotlight_shell);

	XtPopup(spotlight_shell, XtGrabExclusive);
}


void
Set_Arealight_Attributes(InstanceList objects)
{
	InstanceList	elmt;
	XawTextBlock	text_block;
	int				old_length;

	instances = objects;

	arealight_after = FALSE;

	if ( ! arealight_shell )
		Create_Arealight_Dialog();

	for ( elmt = objects ;
		  elmt->the_instance->o_parent->b_class != arealight_obj ;
		  elmt = elmt->next );

	/* Set all the text strings. */
	text_block.firstPos = 0;
	text_block.format = FMT8BIT;
	old_length = strlen(intensity_string);
	sprintf(intensity_string, "%0.2g %0.2g %0.2g", 
			((LightInfoPtr)elmt->the_instance->o_attribs)->red,
			((LightInfoPtr)elmt->the_instance->o_attribs)->green,
			((LightInfoPtr)elmt->the_instance->o_attribs)->blue);
	text_block.length = strlen(intensity_string);
	text_block.ptr = intensity_string;
	XawTextReplace(arealight_intensity_text, 0, old_length + 1, &text_block);

	old_length = strlen(xnum_string);
	sprintf(xnum_string, "%d",
			(int)(((LightInfoPtr)elmt->the_instance->o_attribs)->val1));
	text_block.length = strlen(xnum_string);
	text_block.ptr = xnum_string;
	XawTextReplace(xnum_text, 0, old_length + 1, &text_block);

	old_length = strlen(ynum_string);
	sprintf(ynum_string, "%d",
			(int)(((LightInfoPtr)elmt->the_instance->o_attribs)->val2));
	text_block.length = strlen(ynum_string);
	text_block.ptr = ynum_string;
	XawTextReplace(ynum_text, 0, old_length + 1, &text_block);

	/* Set the inverse toggle. */
	XtVaSetValues(jitter_toggle, XtNstate,
				  ((LightInfoPtr)elmt->the_instance->o_attribs)->flag, NULL);

	SFpositionWidget(arealight_shell);

	XtPopup(arealight_shell, XtGrabExclusive);
}


static void
Create_Light_Dialog()
{
	Arg	args[3];
	int	n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Intensity");	n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	light_shell =  XtCreatePopupShell("lightShell",
						transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Light Intensity");	n++;
	XtSetArg(args[n], XtNvalue, "");					n++;
	light_dialog = XtCreateManagedWidget("lightDialog", dialogWidgetClass,
							light_shell, args, n);

	XawDialogAddButton(light_dialog, "Done", Light_Intensity_Callback, NULL);
	XawDialogAddButton(light_dialog, "Cancel", Cancel_Light_Callback,
						(XtPointer)light_shell);

	XtOverrideTranslations(XtNameToWidget(light_dialog, "value"),
		XtParseTranslationTable(":<Key>Return: Light_Action()"));

	XtVaSetValues(XtNameToWidget(light_dialog, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(light_dialog, "Done"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(light_dialog, "Cancel"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(light_shell);
}


static void
Create_Spotlight_Dialog()
{
	Widget	form;
	Widget	labels[3];
	Widget	done, cancel;
	Arg		args[15];
	int		m, n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Spotlight");		n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	spotlight_shell = XtCreatePopupShell("spotlightShell",
						transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	form = XtCreateManagedWidget("spotlightForm", formWidgetClass,
								 spotlight_shell, args, n);

	/* Common args. */
	m = 0;
	XtSetArg(args[m], XtNleft, XtChainLeft);		m++;
	XtSetArg(args[m], XtNright, XtChainRight);		m++;
	XtSetArg(args[m], XtNtop, XtChainTop);			m++;
	XtSetArg(args[m], XtNbottom, XtChainBottom);	m++;

	/* Intensity label. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Intensity");		n++;
	XtSetArg(args[n], XtNborderWidth, 0);			n++;
	labels[0] = XtCreateManagedWidget("spotlightIntensityLabel",
									  labelWidgetClass, form, args, n);

	/* Intensity text. */
	n = m;
	XtSetArg(args[n], XtNeditType, XawtextEdit);		n++;
	XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);		n++;
	XtSetArg(args[n], XtNstring, intensity_string);		n++;
	XtSetArg(args[n], XtNfromHoriz, labels[0]);			n++;
	spotlight_intensity_text = XtCreateManagedWidget("spotlightIntensityText",
					asciiTextWidgetClass, form, args, n);
	XtOverrideTranslations(spotlight_intensity_text,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	/* Outer Radius label. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Outer Radius");				n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	XtSetArg(args[n], XtNfromVert, spotlight_intensity_text);	n++;
	labels[1] = XtCreateManagedWidget("spotlightRadiusLabel",
									  labelWidgetClass, form, args, n);

	/* Radius text. */
	n = m;
	XtSetArg(args[n], XtNeditType, XawtextEdit);		n++;
	XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);		n++;
	XtSetArg(args[n], XtNstring, radius_string);		n++;
	XtSetArg(args[n], XtNfromHoriz, labels[1]);			n++;
	XtSetArg(args[n], XtNfromVert, spotlight_intensity_text);	n++;
	radius_text = XtCreateManagedWidget("spotlightRadiusText",
					asciiTextWidgetClass, form, args, n);
	XtOverrideTranslations(radius_text,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	/* Tightness label. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Tightness");		n++;
	XtSetArg(args[n], XtNborderWidth, 0);			n++;
	XtSetArg(args[n], XtNfromVert, radius_text);	n++;
	labels[2] = XtCreateManagedWidget("spotlightTightnessLabel",
									  labelWidgetClass, form, args, n);

	/* Radius text. */
	n = m;
	XtSetArg(args[n], XtNeditType, XawtextEdit);		n++;
	XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);		n++;
	XtSetArg(args[n], XtNstring, tightness_string);		n++;
	XtSetArg(args[n], XtNfromHoriz, labels[2]);			n++;
	XtSetArg(args[n], XtNfromVert, radius_text);		n++;
	tightness_text = XtCreateManagedWidget("spotlightTightnessText",
					asciiTextWidgetClass, form, args, n);
	XtOverrideTranslations(tightness_text,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	Match_Widths(labels, 3);

	/* The invert toggle. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Invert");			n++;
	XtSetArg(args[n], XtNfromVert, tightness_text);	n++;
	invert_toggle = XtCreateManagedWidget("spotlightInvertToggle",
						toggleWidgetClass, form, args, n);

	/* Done and cancel. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Done");			n++;
	XtSetArg(args[n], XtNfromVert, invert_toggle);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);	n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);	n++;
#endif
	done = XtCreateManagedWidget("spotlightDoneCommand", commandWidgetClass,
								 form, args, n);
	XtAddCallback(done, XtNcallback, Spotlight_Attributes_Callback, NULL);

	n = m;
	XtSetArg(args[n], XtNlabel, "Cancel");			n++;
	XtSetArg(args[n], XtNfromVert, invert_toggle);	n++;
	XtSetArg(args[n], XtNfromHoriz, done);			n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);	n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);	n++;
#endif
	cancel = XtCreateManagedWidget("spotlightCancelCommand", commandWidgetClass,
								   form, args, n);
	XtAddCallback(cancel, XtNcallback, Cancel_Light_Callback,
				  (XtPointer)spotlight_shell);

	XtRealizeWidget(spotlight_shell);
}


static void
Create_Arealight_Dialog()
{
	Widget	form;
	Widget	labels[3];
	Widget	done, cancel;
	Arg		args[15];
	int		m, n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Arealight");		n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	arealight_shell = XtCreatePopupShell("arealightShell",
						transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	form = XtCreateManagedWidget("arealightForm", formWidgetClass,
								 arealight_shell, args, n);

	/* Common args. */
	m = 0;
	XtSetArg(args[m], XtNleft, XtChainLeft);		m++;
	XtSetArg(args[m], XtNright, XtChainRight);		m++;
	XtSetArg(args[m], XtNtop, XtChainTop);			m++;
	XtSetArg(args[m], XtNbottom, XtChainBottom);	m++;

	/* Intensity label. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Intensity");		n++;
	XtSetArg(args[n], XtNborderWidth, 0);			n++;
	labels[0] = XtCreateManagedWidget("arealightIntensityLabel",
									  labelWidgetClass, form, args, n);

	/* Intensity text. */
	n = m;
	XtSetArg(args[n], XtNeditType, XawtextEdit);		n++;
	XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);		n++;
	XtSetArg(args[n], XtNstring, intensity_string);		n++;
	XtSetArg(args[n], XtNfromHoriz, labels[0]);			n++;
	arealight_intensity_text = XtCreateManagedWidget("arealightIntensityText",
					asciiTextWidgetClass, form, args, n);
	XtOverrideTranslations(arealight_intensity_text,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	/* Xnum label. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Xnum");						n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	XtSetArg(args[n], XtNfromVert, arealight_intensity_text);	n++;
	labels[1] = XtCreateManagedWidget("arealightXnumLabel",
									  labelWidgetClass, form, args, n);

	/* Xnum text. */
	n = m;
	XtSetArg(args[n], XtNeditType, XawtextEdit);		n++;
	XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);		n++;
	XtSetArg(args[n], XtNstring, xnum_string);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[1]);			n++;
	XtSetArg(args[n], XtNfromVert, arealight_intensity_text);	n++;
	xnum_text = XtCreateManagedWidget("arealightXnumText",
					asciiTextWidgetClass, form, args, n);
	XtOverrideTranslations(xnum_text,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	/* Ynum label. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Ynum");		n++;
	XtSetArg(args[n], XtNborderWidth, 0);		n++;
	XtSetArg(args[n], XtNfromVert, xnum_text);	n++;
	labels[2] = XtCreateManagedWidget("arealightYnumLabel",
									  labelWidgetClass, form, args, n);

	/* Ynum text. */
	n = m;
	XtSetArg(args[n], XtNeditType, XawtextEdit);		n++;
	XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);		n++;
	XtSetArg(args[n], XtNstring, ynum_string);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[2]);			n++;
	XtSetArg(args[n], XtNfromVert, xnum_text);			n++;
	ynum_text = XtCreateManagedWidget("arealightYnumText",
					asciiTextWidgetClass, form, args, n);
	XtOverrideTranslations(ynum_text,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	Match_Widths(labels, 3);

	/* The jitter toggle. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Jitter");		n++;
	XtSetArg(args[n], XtNfromVert, ynum_text);	n++;
	jitter_toggle = XtCreateManagedWidget("arealightJitterToggle",
						toggleWidgetClass, form, args, n);

	/* Done and cancel. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Done");			n++;
	XtSetArg(args[n], XtNfromVert, jitter_toggle);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);	n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);	n++;
#endif
	done = XtCreateManagedWidget("arealightDoneCommand", commandWidgetClass,
								 form, args, n);
	XtAddCallback(done, XtNcallback, Arealight_Attributes_Callback, NULL);

	n = m;
	XtSetArg(args[n], XtNlabel, "Cancel");			n++;
	XtSetArg(args[n], XtNfromVert, jitter_toggle);	n++;
	XtSetArg(args[n], XtNfromHoriz, done);			n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);	n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);	n++;
#endif
	cancel = XtCreateManagedWidget("arealightCancelCommand", commandWidgetClass,
								   form, args, n);
	XtAddCallback(cancel, XtNcallback, Cancel_Light_Callback,
				  (XtPointer)arealight_shell);

	XtRealizeWidget(arealight_shell);
}

