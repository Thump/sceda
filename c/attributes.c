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
**	attributes.c : attribute definition dialog box functions.
**
**	Created: 09/06/94
**
*/

#include <math.h>
#include <sced.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Toggle.h>

/* we need this for the definition of intptr_t */
#include <stdint.h>

/* From save.c */
extern int	Save_String(FILE*, char*);

static void Update_Attribute_Strings(AttributePtr new_vals);

static void Create_Attributes_Dialog();
static void	Create_Declare_Dialog();

static void	Target_Attributes_Callback();
static void Default_Attributes_Callback(Widget, XtPointer, XtPointer);
static void	No_Attributes_Callback(Widget, XtPointer, XtPointer);
static void Attributes_Cancel_Callback(Widget, XtPointer, XtPointer);
static void Apply_Attributes_Callback(Widget, XtPointer, XtPointer);
static void	Declares_Callback(Widget, XtPointer, XtPointer);
static void	Create_Specific_Attributes_Dialog();
static void	Radiance_Attributes_Callback();
static void	Create_Radiance_Attributes_Dialog();


static Widget	attributes_dialog_shell = NULL;
static Widget	attributes_form;

static WindowInfoPtr	window;

#define MAX_TEXT_LENGTH 10
#define NUM_TEXT_STRINGS 7

static char	red_text[MAX_TEXT_LENGTH] = {'\0'};
static char	green_text[MAX_TEXT_LENGTH] = {'\0'};
static char	blue_text[MAX_TEXT_LENGTH] = {'\0'};
static char	diffuse_text[MAX_TEXT_LENGTH] = {'\0'};
static char	specular_text[MAX_TEXT_LENGTH] = {'\0'};
static char	spec_power_text[MAX_TEXT_LENGTH] = {'\0'};
static char	reflect_text[MAX_TEXT_LENGTH] = {'\0'};
static char	transparency_text[MAX_TEXT_LENGTH] = {'\0'};
static char	refract_text[MAX_TEXT_LENGTH] = {'\0'};

static Widget	attribute_text[NUM_TEXT_STRINGS];
static Widget	rgb_text[3];
static Widget	none_button;
static Widget	target_button;

/* Declaration handling. */
char		*declarations = NULL;
static Widget	declare_text;
static Widget	declare_shell = NULL;

/* Specialised attribute handling. */
static Widget	specific_attributes_shell = NULL;
static Widget	specific_attributes_text;
static Widget	specific_transform_toggle;
static Widget	specific_open_toggle;

/* Radiance attributes handling. */
static Widget	radiance_attributes_shell = NULL;
static Widget	radiance_attributes_text;
static Widget	radiance_invert_toggle;
static Widget	radiance_open_toggle;

/* Functions for controlling whether light dialogs appear. */
Boolean	attr_have_light;
Boolean	attr_have_spotlight;
Boolean	attr_have_arealight;

void
Set_Attributes_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	Boolean			have_objs = FALSE;
	InstanceList	elmt;

	if ( cl_data )
		window = (WindowInfoPtr)cl_data;

	if ( ! attributes_dialog_shell )
		Create_Attributes_Dialog();

	attr_have_light = attr_have_spotlight = attr_have_arealight = FALSE;
	for ( elmt = window->selected_instances ; elmt ; elmt = elmt->next )
	{
		if ( elmt->the_instance->o_parent->b_class == light_obj )
			attr_have_light = TRUE;
		else if ( elmt->the_instance->o_parent->b_class == spotlight_obj )
			attr_have_spotlight = TRUE;
		else if ( elmt->the_instance->o_parent->b_class == arealight_obj )
			attr_have_arealight = TRUE;
		else
			have_objs = TRUE;
		
	}

	if ( have_objs )
	{
		for ( elmt = window->selected_instances ;
			  Obj_Is_Light(elmt->the_instance) ;
			  elmt = elmt->next );

		Update_Attribute_Strings((AttributePtr)(elmt->the_instance->o_attribs));

		if ( ((AttributePtr)elmt->the_instance->o_attribs)->use_extension )
		{
			Target_Attributes_Callback();
			return;
		}

		SFpositionWidget(attributes_dialog_shell);
		XtPopup(attributes_dialog_shell, XtGrabExclusive);
	}
	else if ( attr_have_light )
		Set_Light_Attributes(window->selected_instances,
							 attr_have_spotlight, attr_have_arealight);
	else if ( attr_have_spotlight )
		Set_Spotlight_Attributes(window->selected_instances,
								 attr_have_arealight);
	else if ( attr_have_arealight )
		Set_Arealight_Attributes(window->selected_instances);
}


static void
Target_Attributes_Callback()
{
	switch ( camera.type )
	{
		case POVray:
		case Rayshade:
			Specific_Attributes_Callback();
			break;

		case Radiance:
		case Renderman:
			Radiance_Attributes_Callback();
			break;

		default:;
	}
}



/*	void
**	Create_Attributes_Dialog()
**	Creates the attributes dialog box.
*/
static void
Create_Attributes_Dialog()
{
	Dimension	label_height;
	Dimension	max_width;
	Widget	top_label;
	Widget	done_button;
	Widget	cancel_button;
	Widget	default_button;
	Widget	labels[11];
	Widget	rgb_labels[3];
	Arg		args[15];
	int		count, rgb_count;
	int		m, n;

	attributes_dialog_shell = XtCreatePopupShell("Attributes",
						transientShellWidgetClass, main_window.shell, NULL, 0);

	n = 0;
	attributes_form = XtCreateManagedWidget("attributesForm", formWidgetClass,
					attributes_dialog_shell, args, n);

	/* Add the label at the top. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Object Attributes");	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);				n++;
	XtSetArg(args[n], XtNbottom,XtChainTop);			n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	top_label = XtCreateManagedWidget("attributesLabel", labelWidgetClass,
				attributes_form, args, n);

	/* Need the size of the labels to get the string size right. */
	n = 0;
	XtSetArg(args[n], XtNheight, &label_height);	n++;
	XtGetValues(top_label, args, n);

	/* Common args for all the labels. */
	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;
	m = n;

	count = 0;
	/* The label for the colour: */
	XtSetArg(args[n], XtNlabel, "Colour");		n++;
	XtSetArg(args[n], XtNfromVert, top_label);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);		n++;
	labels[count] = XtCreateManagedWidget("colourLabel", labelWidgetClass,
					attributes_form, args, n);
	count++;

	/* The label for the red. */
	rgb_count = 0;
	n = 5;
	XtSetArg(args[n], XtNfromVert, top_label);			n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNlabel, "Red");					n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	rgb_labels[rgb_count] = XtCreateManagedWidget("redLabel", labelWidgetClass,
					attributes_form, args, n);
	/* The text. */
	n = 5;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, red_text);			n++;
	XtSetArg(args[n], XtNfromVert, top_label);		n++;
	XtSetArg(args[n], XtNfromHoriz, rgb_labels[rgb_count]);	n++;
	rgb_text[rgb_count] = XtCreateManagedWidget("redText",
					asciiTextWidgetClass, attributes_form, args, n);
	XtOverrideTranslations(rgb_text[rgb_count],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	rgb_count++;

	/* The label for the green. */
	n = 5;
	XtSetArg(args[n], XtNfromVert, top_label);			n++;
	XtSetArg(args[n], XtNfromHoriz, rgb_text[rgb_count - 1]);	n++;
	XtSetArg(args[n], XtNlabel, "Green");				n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	rgb_labels[1] = XtCreateManagedWidget("greenLabel", labelWidgetClass,
					attributes_form, args, n);
	/* The text. */
	n = 5;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, green_text);		n++;
	XtSetArg(args[n], XtNfromVert, top_label);		n++;
	XtSetArg(args[n], XtNfromHoriz, rgb_labels[rgb_count]);	n++;
	rgb_text[rgb_count] = XtCreateManagedWidget("greenText",
					asciiTextWidgetClass, attributes_form, args, n);
	XtOverrideTranslations(rgb_text[rgb_count],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	rgb_count++;

	/* The label for the blue. */
	n = 5;
	XtSetArg(args[n], XtNfromVert, top_label);			n++;
	XtSetArg(args[n], XtNfromHoriz, rgb_text[rgb_count - 1]);	n++;
	XtSetArg(args[n], XtNlabel, "Blue");				n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	rgb_labels[2] = XtCreateManagedWidget("blueLabel", labelWidgetClass,
					attributes_form, args, n);
	/* The text. */
	n = 5;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, blue_text);		n++;
	XtSetArg(args[n], XtNfromVert, top_label);		n++;
	XtSetArg(args[n], XtNfromHoriz, rgb_labels[rgb_count]);	n++;
	rgb_text[rgb_count] = XtCreateManagedWidget("blueText",
					asciiTextWidgetClass, attributes_form, args, n);
	XtOverrideTranslations(rgb_text[rgb_count],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	rgb_count++;

	/* The label for the diffuse value. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Diffuse Coefficient");		n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);		n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("diffuseLabel", labelWidgetClass,
					attributes_form, args, n);
	/* The text. */
	n = 5;
	XtSetArg(args[n], XtNheight, label_height);				n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);			n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);			n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);			n++;
	XtSetArg(args[n], XtNstring, diffuse_text);				n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);		n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);			n++;
	attribute_text[count] = XtCreateManagedWidget("diffuseText",
					asciiTextWidgetClass, attributes_form, args, n);
	XtOverrideTranslations(attribute_text[count],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	/* The label for the specular value. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Specular Coef");		n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	labels[count] = XtCreateManagedWidget("specularLabel", labelWidgetClass,
					attributes_form, args, n);
	/* The text. */
	n = 5;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, specular_text);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);	n++;
	attribute_text[count] = XtCreateManagedWidget("specularText",
					asciiTextWidgetClass, attributes_form, args, n);
	XtOverrideTranslations(attribute_text[count],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	/* The specular power label and text.*/
	n = 5;
	XtSetArg(args[n], XtNlabel, "Power");	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 2]);		n++;
	XtSetArg(args[n], XtNfromHoriz, attribute_text[count-1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	labels[count] = XtCreateManagedWidget("specPowerLabel", labelWidgetClass,
					attributes_form, args, n);
	/* The text. */
	n = 5;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, spec_power_text);	n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 2]);	n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);	n++;
	attribute_text[count] = XtCreateManagedWidget("specPowerText",
					asciiTextWidgetClass, attributes_form, args, n);
	XtOverrideTranslations(attribute_text[count],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;



	/* The label for the reflection value. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Reflection");			n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 2]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	labels[count] = XtCreateManagedWidget("reflectLabel", labelWidgetClass,
					attributes_form, args, n);
	/* The text. */
	n = 5;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, reflect_text);		n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 2]);	n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);	n++;
	attribute_text[count] = XtCreateManagedWidget("reflectText",
					asciiTextWidgetClass, attributes_form, args, n);
	XtOverrideTranslations(attribute_text[count],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	/* The label for the transparency value. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Transparency");		n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	labels[count] = XtCreateManagedWidget("transpLabel", labelWidgetClass,
					attributes_form, args, n);
	/* The text. */
	n = 5;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, transparency_text);n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);	n++;
	attribute_text[count] = XtCreateManagedWidget("transparencyText",
					asciiTextWidgetClass, attributes_form, args, n);
	XtOverrideTranslations(attribute_text[count],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	/* The label for the refraction value. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Refraction");			n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	labels[count] = XtCreateManagedWidget("refractLabel", labelWidgetClass,
					attributes_form, args, n);
	/* The text. */
	n = 5;
	XtSetArg(args[n], XtNheight, label_height);		n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);	n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);	n++;
	XtSetArg(args[n], XtNstring, refract_text);		n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
	XtSetArg(args[n], XtNfromHoriz, labels[count]);	n++;
	attribute_text[count] = XtCreateManagedWidget("refractText",
					asciiTextWidgetClass, attributes_form, args, n);
	XtOverrideTranslations(attribute_text[count],
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
	count++;

	/* The buttons at the bottom. */

	/* Target specific attributes. */
	n = m;
	XtSetArg(args[n], XtNlabel, "Target Specific");		n++;
	XtSetArg(args[n], XtNfromVert, labels[count - 1]);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	target_button = XtCreateManagedWidget("targetButton",
					commandWidgetClass, attributes_form, args, n);
	XtAddCallback(target_button, XtNcallback, Apply_Attributes_Callback,
				  (XtPointer)TRUE);

	/* Done button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Done");			n++;
	XtSetArg(args[n], XtNfromVert, target_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	done_button = XtCreateManagedWidget("doneButton",
					commandWidgetClass, attributes_form, args, n);
	XtAddCallback(done_button, XtNcallback, Apply_Attributes_Callback, NULL);

	/* None. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "None");			n++;
	XtSetArg(args[n], XtNfromVert, target_button);	n++;
	XtSetArg(args[n], XtNfromHoriz, done_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	none_button = XtCreateManagedWidget("noneButton",
						toggleWidgetClass, attributes_form, args, n);
	XtAddCallback(none_button, XtNcallback, No_Attributes_Callback, NULL);

	/* Default. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Default");			n++;
	XtSetArg(args[n], XtNfromVert, target_button);	n++;
	XtSetArg(args[n], XtNfromHoriz, none_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	default_button = XtCreateManagedWidget("defaultButton",
						commandWidgetClass, attributes_form, args, n);
	XtAddCallback(default_button, XtNcallback, Default_Attributes_Callback,
					NULL);

	/* Cancel button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Cancel");				n++;
	XtSetArg(args[n], XtNfromVert, target_button);		n++;
	XtSetArg(args[n], XtNfromHoriz, default_button);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	cancel_button = XtCreateManagedWidget("cancelButton",
					commandWidgetClass, attributes_form, args, n);
	XtAddCallback(cancel_button, XtNcallback, Attributes_Cancel_Callback,
				  (XtPointer)attributes_dialog_shell);

	/* Set all the widths to the same value. */
	max_width = Match_Widths(labels, count);

	n = 0;
	XtSetArg(args[n], XtNwidth, max_width);	n++;
	XtSetValues(done_button, args, n);
	XtSetValues(default_button, args, n);
	XtSetValues(cancel_button, args, n);
	XtSetValues(none_button, args, n);

	XtRealizeWidget(attributes_dialog_shell);
}


static void
Attributes_Cancel_Callback(Widget w, XtPointer a, XtPointer b)
{
	XtPopdown((Widget)a);
}


/*	void
**	Update_Attribute_Strings()
**	Resets the strings displayed by the dialog box.  Redraws them too.
*/
static void
Update_Attribute_Strings(AttributePtr new_vals)
{
	XawTextBlock	text_block;
	int				old_length;
	int				count;

	/* For this to be called the relevant text widgets must have been
	** created.  You can consider this a statement of fact or an
	** excuse for crashing otherwise.
	*/

	text_block.firstPos = 0;
	text_block.format = FMT8BIT;

	/* Colour. */
	old_length = strlen(red_text);
	sprintf(red_text, "%1.2g", ((double)new_vals->colour.red) /
								(unsigned short)MAX_UNSIGNED_SHORT);
	text_block.length = strlen(red_text);
	text_block.ptr = red_text;
	XawTextReplace(rgb_text[0], 0, old_length + 1, &text_block);

	old_length = strlen(green_text);
	sprintf(green_text, "%1.2g", ((double)new_vals->colour.green) /
								(unsigned short)MAX_UNSIGNED_SHORT);
	text_block.length = strlen(green_text);
	text_block.ptr = green_text;
	XawTextReplace(rgb_text[1], 0, old_length + 1, &text_block);

	old_length = strlen(blue_text);
	sprintf(blue_text, "%1.2g", ((double)new_vals->colour.blue) /
								(unsigned short)MAX_UNSIGNED_SHORT);
	text_block.length = strlen(blue_text);
	text_block.ptr = blue_text;
	XawTextReplace(rgb_text[2], 0, old_length + 1, &text_block);


	count = 1;
	/* diffuse. */
	old_length = strlen(diffuse_text);
	sprintf(diffuse_text, "%1.2g", new_vals->diff_coef);
	text_block.length = strlen(diffuse_text);
	text_block.ptr = diffuse_text;
	XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

	/* Specular. */
	old_length = strlen(specular_text);
	sprintf(specular_text, "%1.2g", new_vals->spec_coef);
	text_block.length = strlen(specular_text);
	text_block.ptr = specular_text;
	XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

	old_length = strlen(spec_power_text);
	sprintf(spec_power_text, "%1.2g", new_vals->spec_power);
	text_block.length = strlen(spec_power_text);
	text_block.ptr = spec_power_text;
	XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

	/* Reflection. */
	old_length = strlen(reflect_text);
	sprintf(reflect_text, "%1.2g", new_vals->reflect_coef);
	text_block.length = strlen(reflect_text);
	text_block.ptr = reflect_text;
	XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

	/* Transparency. */
	old_length = strlen(transparency_text);
	sprintf(transparency_text, "%1.2g", new_vals->transparency);
	text_block.length = strlen(transparency_text);
	text_block.ptr = transparency_text;
	XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

	/* Refraction. */
	old_length = strlen(refract_text);
	sprintf(refract_text, "%1.2g", new_vals->refract_index);
	text_block.length = strlen(refract_text);
	text_block.ptr = refract_text;
	XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

	/* Set the None toggle. */
	XtVaSetValues(none_button, XtNstate, ! new_vals->defined, NULL);
	No_Attributes_Callback(NULL, NULL, (XtPointer)(intptr_t)
        (! new_vals->defined));
}


/*	void
**	Default_Attributes_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**	The callback invoked to set the camera to its default.
*/
static void
Default_Attributes_Callback(Widget w, XtPointer a, XtPointer b)
{
	Attributes	new_vals;

	new_vals.defined = TRUE;
	new_vals.colour.red = default_attributes.colour.red;
	new_vals.colour.green = default_attributes.colour.green;
	new_vals.colour.blue = default_attributes.colour.blue;
	new_vals.diff_coef = default_attributes.diff_coef;
	new_vals.spec_coef = default_attributes.spec_coef;
	new_vals.spec_power = default_attributes.spec_power;
	new_vals.reflect_coef = default_attributes.reflect_coef;
	new_vals.refract_index = default_attributes.refract_index;
	new_vals.transparency = default_attributes.transparency;
	new_vals.use_extension = FALSE;
	new_vals.extension = NULL;
	new_vals.use_obj_trans = FALSE;
	new_vals.open = FALSE;

	Update_Attribute_Strings(&new_vals);
}


static void
Apply_Attributes_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	InstanceList	elmt;
	Attributes		new_vals;
	double			temp_d;

	if ( cl )
	{
		switch ( camera.type )
		{
			case Genray:
			case Genscan:
				Popup_Error("No Genray or Genscan attributes.",
							main_window.shell, "Error");
				return;

			case None:
				Popup_Error("No Target renderer.", main_window.shell, "Error");
				return;

			default:;
		}
	}

	XtPopdown(attributes_dialog_shell);

	XtVaGetValues(none_button, XtNstate, &(new_vals.defined), NULL);
	new_vals.defined = ! new_vals.defined;

	/* Need to parse all the strings. */
	sscanf(red_text, "%lf", &temp_d);
	if ( temp_d > 1.0 ) temp_d = 1.0;
	if ( temp_d < 0.0 ) temp_d = 0.0;
	new_vals.colour.red = (unsigned short)(temp_d * MAX_UNSIGNED_SHORT);
	sscanf(green_text, "%lf", &temp_d);
	if ( temp_d > 1.0 ) temp_d = 1.0;
	if ( temp_d < 0.0 ) temp_d = 0.0;
	new_vals.colour.green = (unsigned short)(temp_d * MAX_UNSIGNED_SHORT);
	sscanf(blue_text, "%lf", &temp_d);
	if ( temp_d > 1.0 ) temp_d = 1.0;
	if ( temp_d < 0.0 ) temp_d = 0.0;
	new_vals.colour.blue = (unsigned short)(temp_d * MAX_UNSIGNED_SHORT);

	sscanf(diffuse_text, "%lf", &(new_vals.diff_coef));
	if ( new_vals.diff_coef > 1.0 ) new_vals.diff_coef = 1.0;
	if ( new_vals.diff_coef < 0.0 ) new_vals.diff_coef = 0.0;
	sscanf(specular_text, "%lf", &(new_vals.spec_coef));
	if ( new_vals.spec_coef > 1.0 ) new_vals.spec_coef = 1.0;
	if ( new_vals.spec_coef < 0.0 ) new_vals.spec_coef = 0.0;
	sscanf(spec_power_text, "%lf", &(new_vals.spec_power));
	sscanf(reflect_text, "%lf", &(new_vals.reflect_coef));
	if ( new_vals.reflect_coef > 1.0 ) new_vals.reflect_coef = 1.0;
	if ( new_vals.reflect_coef < 0.0 ) new_vals.reflect_coef = 0.0;
	sscanf(transparency_text, "%lf", &(new_vals.transparency));
	if ( new_vals.transparency > 1.0 ) new_vals.transparency = 1.0;
	if ( new_vals.transparency < 0.0 ) new_vals.transparency = 0.0;
	sscanf(refract_text, "%lf", &(new_vals.refract_index));

	new_vals.use_extension = FALSE;

	for ( elmt = window->selected_instances ;
		  elmt != NULL ;
		  elmt = elmt->next )
	{
		if ( ! Obj_Is_Light(elmt->the_instance) )
			Modify_Instance_Attributes(elmt->the_instance, &new_vals,ModSimple);
	}

	if ( cl )
	{
		Target_Attributes_Callback();
		return;
	}

	if ( attr_have_light )
		Set_Light_Attributes(window->selected_instances,
							 attr_have_spotlight, attr_have_arealight);
	else if ( attr_have_spotlight )
		Set_Spotlight_Attributes(window->selected_instances,
								 attr_have_arealight);
	else if ( attr_have_arealight )
		Set_Arealight_Attributes(window->selected_instances);
}


void
Attributes_Change_String(InstanceList insts, char *new_str, Boolean transform,
						 Boolean open)
{
	Attributes		new_attr;
	InstanceList	elmt;

	for ( elmt = insts ; elmt != NULL ; elmt = elmt->next )
	{
		if ( ! Obj_Is_Light(elmt->the_instance) )
		{
			new_attr.defined = TRUE;
			new_attr.use_extension = TRUE;
			new_attr.extension = Strdup(new_str);
			new_attr.use_obj_trans = transform;
			new_attr.open = open;
			Modify_Instance_Attributes(elmt->the_instance, &new_attr,ModExtend);
		}
	}
}

static void
No_Attributes_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	InstanceList	elmt;
	Boolean			result = ( ca ? TRUE : FALSE );
	int				i;

	for ( elmt = window->selected_instances ;
		  elmt != NULL ;
		  elmt = elmt->next )
		if ( ! Obj_Is_Light(elmt->the_instance) )
			((AttributePtr)(elmt->the_instance->o_attribs))->defined = ! result;

	for ( i = 1 ; i < NUM_TEXT_STRINGS ; i++ )
		XtSetSensitive(attribute_text[i], ! result );
	for ( i = 0 ; i < 3 ; i++ )
		XtSetSensitive(rgb_text[i], ! result );
	XtSetSensitive(target_button, ! result );
}



static void
Declares_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	if ( ! declare_shell )
		Create_Declare_Dialog();

	SFpositionWidget(declare_shell);

	XtPopup(declare_shell, XtGrabNonexclusive);
}



/*	Adds an include file to the list of files to include when exporting.
*/
void
Add_Declarations(char *new_stuff)
{
	if ( new_stuff[0] == '\0' )
		return;

	if ( declarations )
	{
		declarations = More(declarations, char,
						strlen(declarations) + strlen(new_stuff) + 5);
		strcat(declarations, new_stuff);
	}
	else
	{
		declarations = New(char, strlen(new_stuff) + 5);
		strcpy(declarations, new_stuff);
	}

	if ( ! declare_shell )
		Create_Declare_Dialog();

	XtVaSetValues(declare_text, XtNstring, declarations, NULL);
}


/*	Clears all the include files.
*/
void
Clear_Declarations()
{
	free(declarations);
	declarations = NULL;
}


/*	Saves includes.
*/
int
Save_Declarations(FILE *outfile)
{
	if ( ! declarations ) return 1;

	fprintf(outfile, "Declare\n");

	Save_String(outfile, declarations);

	return fprintf(outfile, "\n");
}


static void
Add_Declare_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	char	*temp_string;

	XtPopdown(declare_shell);

	/* Free the old string, then take what's in the dialog and
	** make it the new set of declarations
	*/
	Clear_Declarations();
	XtVaGetValues(declare_text, XtNstring, &temp_string, NULL);
	Add_Declarations(temp_string);
}


static void
Create_Declare_Dialog()
{
	Arg		args[15];
	int		n, m;
	Widget	form;
	Widget	label;
	Widget	done, cancel;
	Dimension	height;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Declarations");	n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	declare_shell = XtCreatePopupShell("povDeclareShell",
						topLevelShellWidgetClass, main_window.shell, args, n);

	n = 0;
	form = XtCreateManagedWidget("povDeclareForm", formWidgetClass,
						declare_shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;
	XtSetArg(args[n], XtNlabel, "Declarations:");	n++;
	XtSetArg(args[n], XtNborderWidth, 0);			n++;
	label = XtCreateManagedWidget("povDeclareExistingLabel",
						labelWidgetClass, form, args, n);

	XtVaGetValues(label, XtNheight, &height, NULL);

	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainRight);		n++;
	XtSetArg(args[n], XtNresizable, TRUE);			n++;
	XtSetArg(args[n], XtNheight, (int)height * 10);	n++;
	XtSetArg(args[n], XtNstring, "");				n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNresize, XawtextResizeWidth);				n++;
	XtSetArg(args[n], XtNscrollVertical, XawtextScrollWhenNeeded);	n++;
	XtSetArg(args[n], XtNfromVert, label);			n++;
	declare_text = XtCreateManagedWidget("povDeclareExistingText",
						asciiTextWidgetClass, form, args, n);

	m = 0;
	XtSetArg(args[m], XtNtop, XtChainBottom);		m++;
	XtSetArg(args[m], XtNbottom, XtChainBottom);	m++;
	XtSetArg(args[m], XtNleft, XtChainLeft);		m++;
	XtSetArg(args[m], XtNright, XtChainLeft);		m++;
	XtSetArg(args[m], XtNresizable, TRUE);			m++;

	n = m;
	XtSetArg(args[n], XtNlabel, "Done");				n++;
	XtSetArg(args[n], XtNfromVert, declare_text);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	done = XtCreateManagedWidget("povDeclareDoneButton", commandWidgetClass,
								form, args, n);
	XtAddCallback(done, XtNcallback, Add_Declare_Callback, NULL);

	n = m;
	XtSetArg(args[n], XtNlabel, "Cancel");				n++;
	XtSetArg(args[n], XtNfromVert, declare_text);	n++;
	XtSetArg(args[n], XtNfromHoriz, done);				n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	cancel = XtCreateManagedWidget("povDeclareCancelButton", commandWidgetClass,
								form, args, n);
	XtAddCallback(cancel, XtNcallback, Attributes_Cancel_Callback,
					(XtPointer)declare_shell);

	XtRealizeWidget(declare_shell);
}


void
Set_Specific_Attributes_Initial_Text(char *text, Boolean transform,Boolean open)
{
	if ( ! specific_attributes_shell )
		Create_Specific_Attributes_Dialog();
	
	if ( ! text )
		text = "";

	XtVaSetValues(specific_attributes_text, XtNstring, text, NULL);
	XtVaSetValues(specific_transform_toggle, XtNstate, transform, NULL);
	XtVaSetValues(specific_open_toggle, XtNstate, open, NULL);
}


void
Specific_Attributes_Callback()
{
	InstanceList	elmt;

	if ( ! specific_attributes_shell )
		Create_Specific_Attributes_Dialog();

	for ( elmt = window->selected_instances ;
		  Obj_Is_Light(elmt->the_instance) ;
		  elmt = elmt->next );

	Set_Specific_Attributes_Initial_Text(
		((AttributePtr)elmt->the_instance->o_attribs)->extension,
		((AttributePtr)elmt->the_instance->o_attribs)->use_obj_trans,
		((AttributePtr)elmt->the_instance->o_attribs)->open);

	SFpositionWidget(specific_attributes_shell);
	XtPopup(specific_attributes_shell, XtGrabExclusive);
}


static void
Specific_Attributes_Done_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	char	*new_attribs;
	Boolean	transform, open;

	XtPopdown(specific_attributes_shell);

	XtVaGetValues(specific_attributes_text, XtNstring, &new_attribs, NULL);
	XtVaGetValues(specific_transform_toggle, XtNstate, &transform, NULL);
	XtVaGetValues(specific_open_toggle, XtNstate, &open, NULL);
	Attributes_Change_String(window->selected_instances, new_attribs,
							 transform, open);

	if ( attr_have_light )
		Set_Light_Attributes(window->selected_instances, attr_have_spotlight,
							 attr_have_arealight);
	else if ( attr_have_spotlight )
		Set_Spotlight_Attributes(window->selected_instances,
								 attr_have_arealight);
	else if ( attr_have_arealight )
		Set_Arealight_Attributes(window->selected_instances);
}


static void
Attributes_Simple_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	InstanceList	elmt;
	char	*new_attribs;
	Boolean	transform, open;

	XtPopdown((Widget)cl);

	if ( (Widget)cl == radiance_attributes_shell )
	{
		XtVaGetValues(radiance_attributes_text, XtNstring, &new_attribs, NULL);
		XtVaGetValues(radiance_invert_toggle, XtNstate, &transform, NULL);
		XtVaGetValues(radiance_open_toggle, XtNstate, &open, NULL);
	}
	else
	{
		XtVaGetValues(specific_attributes_text, XtNstring, &new_attribs, NULL);
		XtVaGetValues(specific_transform_toggle, XtNstate, &transform, NULL);
		XtVaGetValues(specific_open_toggle, XtNstate, &open, NULL);
	}
	Attributes_Change_String(window->selected_instances, new_attribs,
							 transform, open);

	for ( elmt = window->selected_instances ; elmt ; elmt = elmt->next )
	{
		if ( ! Obj_Is_Light(elmt->the_instance ) )
		  ((AttributePtr)elmt->the_instance->o_attribs)->use_extension = FALSE;
	}

	Set_Attributes_Callback(NULL, NULL, NULL);
}

static void
Create_Specific_Attributes_Dialog()
{
	Arg			args[15];
	int			n, m;
	Widget		form;
	Widget		label;
	Widget		done, cancel, simplified;
	Widget		include_widget, declare_widget;
	Dimension	height;
	String		shell_geometry;
	unsigned	shell_width, shell_height;
	int			gap;
	int			junk;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Specific Attributes");		n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);			n++;
	specific_attributes_shell = XtCreatePopupShell("specAttributesShell",
						transientShellWidgetClass, main_window.shell, args, n);

	XtVaGetValues(specific_attributes_shell, XtNgeometry, &shell_geometry,NULL);
	XParseGeometry(shell_geometry, &junk, &junk, &shell_width, &shell_height);

	n = 0;
	form = XtCreateManagedWidget("specAttributesForm", formWidgetClass,
									specific_attributes_shell, args, n);

	XtVaGetValues(form, XtNdefaultDistance, &gap, NULL);

	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);				n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);			n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);			n++;
	XtSetArg(args[n], XtNright, XtChainLeft);			n++;
	XtSetArg(args[n], XtNresizable, TRUE);				n++;
	XtSetArg(args[n], XtNlabel, "Texture String:");		n++;
	XtSetArg(args[n], XtNborderWidth, 0);				n++;
	label = XtCreateManagedWidget("specAttributesLabel",
									labelWidgetClass, form, args, n);

	XtVaGetValues(label, XtNheight, &height, NULL);

	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainRight);		n++;
	XtSetArg(args[n], XtNresizable, TRUE);			n++;
	XtSetArg(args[n], XtNwidth, (int)shell_height - 2 - 2  * (int)gap);	n++;
	XtSetArg(args[n], XtNheight,
			 (int)shell_height - (int)height * 4 - 8 - 6 * (int)gap);	n++;
	XtSetArg(args[n], XtNstring, "");				n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNresize, TRUE);				n++;
	XtSetArg(args[n], XtNscrollVertical, XawtextScrollWhenNeeded);	n++;
	XtSetArg(args[n], XtNfromVert, label);			n++;
	specific_attributes_text = XtCreateManagedWidget("specAttributesText",
						asciiTextWidgetClass, form, args, n);

	m = 0;
	XtSetArg(args[m], XtNtop, XtChainBottom);		m++;
	XtSetArg(args[m], XtNbottom, XtChainBottom);	m++;
	XtSetArg(args[m], XtNleft, XtChainLeft);		m++;
	XtSetArg(args[m], XtNright, XtChainLeft);		m++;
	XtSetArg(args[m], XtNresizable, TRUE);			m++;

	n = m;
	XtSetArg(args[n], XtNlabel, "Transform Texture");			n++;
	XtSetArg(args[n], XtNfromVert, specific_attributes_text);	n++;
	specific_transform_toggle = XtCreateManagedWidget(
		"specAttributesTransformToggle", toggleWidgetClass, form, args, n);

	n = m;
	XtSetArg(args[n], XtNlabel, "Open");					n++;
	XtSetArg(args[n], XtNfromVert, specific_attributes_text);	n++;
	XtSetArg(args[n], XtNfromHoriz, specific_transform_toggle);	n++;
	specific_open_toggle = XtCreateManagedWidget("specAttributesOpenToggle",
						toggleWidgetClass, form, args, n);

	n = m;
	XtSetArg(args[n], XtNlabel, "Includes");					n++;
	XtSetArg(args[n], XtNfromVert, specific_transform_toggle);	n++;
#if ( USE_ROUNDED_BUTTONS )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	include_widget = XtCreateManagedWidget("specAttributesIncludeButton",
						commandWidgetClass, form, args, n);
	XtAddCallback(include_widget, XtNcallback, POV_Includes_Callback, NULL);

	n = m;
	XtSetArg(args[n], XtNlabel, "Declarations");				n++;
	XtSetArg(args[n], XtNfromVert, specific_transform_toggle);	n++;
	XtSetArg(args[n], XtNfromHoriz, include_widget);		n++;
#if ( USE_ROUNDED_BUTTONS )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	declare_widget = XtCreateManagedWidget("specAttributesDeclareButton",
						commandWidgetClass, form, args, n);
	XtAddCallback(declare_widget, XtNcallback, Declares_Callback, NULL);

	n = m;
	XtSetArg(args[n], XtNlabel, "Done");			n++;
	XtSetArg(args[n], XtNfromVert, include_widget);	n++;
#if ( USE_ROUNDED_BUTTONS )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	done = XtCreateManagedWidget("specAttributesDoneButton", commandWidgetClass,
									form, args, n);
	XtAddCallback(done, XtNcallback, Specific_Attributes_Done_Callback, NULL);

	n = m;
	XtSetArg(args[n], XtNlabel, "Simplified");		n++;
	XtSetArg(args[n], XtNfromVert, include_widget);	n++;
	XtSetArg(args[n], XtNfromHoriz, done);			n++;
#if ( USE_ROUNDED_BUTTONS )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	simplified = XtCreateManagedWidget("specAttributesSimpleButton",
					commandWidgetClass, form, args, n);
	XtAddCallback(simplified, XtNcallback, Attributes_Simple_Callback,
				  (XtPointer)specific_attributes_shell);
	
	n = m;
	XtSetArg(args[n], XtNlabel, "Cancel");			n++;
	XtSetArg(args[n], XtNfromVert, include_widget);	n++;
	XtSetArg(args[n], XtNfromHoriz, simplified);	n++;
#if ( USE_ROUNDED_BUTTONS )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	cancel = XtCreateManagedWidget("specAttributesCancelButton",
								commandWidgetClass, form, args, n);
	XtAddCallback(cancel, XtNcallback, Attributes_Cancel_Callback,
					(XtPointer)specific_attributes_shell);
	
	XtRealizeWidget(specific_attributes_shell);
}

void
Set_Radiance_Attributes_Initial_Text(char *text, Boolean invert, Boolean open)
{
	if ( ! radiance_attributes_shell )
		Create_Radiance_Attributes_Dialog();
	
	if ( ! text )
		text = "";

	XtVaSetValues(radiance_attributes_text, XtNstring, text, NULL);
	XtVaSetValues(radiance_invert_toggle, XtNstate, invert, NULL);
	XtVaSetValues(radiance_open_toggle, XtNstate, open, NULL);
}



static void
Radiance_Attributes_Callback()
{
	InstanceList	elmt;

	if ( ! radiance_attributes_shell )
		Create_Radiance_Attributes_Dialog();

	for ( elmt = window->selected_instances ;
		  Obj_Is_Light(elmt->the_instance) ;
		  elmt = elmt->next );
	Set_Radiance_Attributes_Initial_Text(
		((AttributePtr)elmt->the_instance->o_attribs)->extension,
		((AttributePtr)elmt->the_instance->o_attribs)->use_obj_trans,
		((AttributePtr)elmt->the_instance->o_attribs)->open);

	SFpositionWidget(radiance_attributes_shell);
	XtPopup(radiance_attributes_shell, XtGrabExclusive);
}


static void
Radiance_Attributes_Done_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	char	*new_attribs;
	Boolean	invert;
	Boolean	open;

	XtPopdown(radiance_attributes_shell);

	XtVaGetValues(radiance_attributes_text, XtNstring, &new_attribs, NULL);
	XtVaGetValues(radiance_invert_toggle, XtNstate, &invert, NULL);
	XtVaGetValues(radiance_open_toggle, XtNstate, &open, NULL);
	Attributes_Change_String(window->selected_instances, new_attribs,
							 invert, open);

	if ( attr_have_light )
		Set_Light_Attributes(window->selected_instances, attr_have_spotlight,
							 attr_have_arealight);
	else if ( attr_have_spotlight )
		Set_Spotlight_Attributes(window->selected_instances,
								 attr_have_arealight);
	else if ( attr_have_arealight )
		Set_Arealight_Attributes(window->selected_instances);
}


static void
Create_Radiance_Attributes_Dialog()
{
	Arg		args[15];
	int		n, m;
	Widget	form;
	Widget	label;
	Widget	done, cancel, simplified;
	Widget	declare_widget;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Radiance / Renderman Attribs");	n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	radiance_attributes_shell = XtCreatePopupShell("radianceAttributesShell",
						transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	form = XtCreateManagedWidget("radianceAttributesForm", formWidgetClass,
									radiance_attributes_shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);						n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);					n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);					n++;
	XtSetArg(args[n], XtNright, XtChainLeft);					n++;
	XtSetArg(args[n], XtNresizable, TRUE);						n++;
	XtSetArg(args[n], XtNlabel, "Modifier / Surface Name:");	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	label = XtCreateManagedWidget("radianceAttributesLabel",
									labelWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainRight);		n++;
	XtSetArg(args[n], XtNresizable, TRUE);			n++;
	XtSetArg(args[n], XtNstring, "");				n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);	n++;
	XtSetArg(args[n], XtNresize, TRUE);				n++;
	XtSetArg(args[n], XtNfromVert, label);			n++;
	radiance_attributes_text = XtCreateManagedWidget("radianceAttributesText",
						asciiTextWidgetClass, form, args, n);
	XtOverrideTranslations(radiance_attributes_text,
		XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

	m = 0;
	XtSetArg(args[m], XtNtop, XtChainBottom);		m++;
	XtSetArg(args[m], XtNbottom, XtChainBottom);	m++;
	XtSetArg(args[m], XtNleft, XtChainLeft);		m++;
	XtSetArg(args[m], XtNright, XtChainLeft);		m++;
	XtSetArg(args[m], XtNresizable, TRUE);			m++;

	n = m;
	XtSetArg(args[n], XtNlabel, "Invert Normals");				n++;
	XtSetArg(args[n], XtNfromVert, radiance_attributes_text);	n++;
	radiance_invert_toggle = XtCreateManagedWidget(
		"radianceAttributesInvertToggle", toggleWidgetClass, form, args, n);

	n = m;
	XtSetArg(args[n], XtNlabel, "Open");						n++;
	XtSetArg(args[n], XtNfromVert, radiance_attributes_text);	n++;
	XtSetArg(args[n], XtNfromHoriz, radiance_invert_toggle);	n++;
	radiance_open_toggle = XtCreateManagedWidget(
		"radianceAttributesOpenToggle", toggleWidgetClass, form, args, n);

	n = m;
	XtSetArg(args[n], XtNlabel, "Declarations");			n++;
	XtSetArg(args[n], XtNfromVert, radiance_invert_toggle);	n++;
#if ( USE_ROUNDED_BUTTONS )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	declare_widget = XtCreateManagedWidget("radianceAttributesDeclareButton",
						commandWidgetClass, form, args, n);
	XtAddCallback(declare_widget, XtNcallback, Declares_Callback, NULL);

	n = m;
	XtSetArg(args[n], XtNlabel, "Done");			n++;
	XtSetArg(args[n], XtNfromVert, declare_widget);	n++;
#if ( USE_ROUNDED_BUTTONS )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	done = XtCreateManagedWidget("radianceAttributesDoneButton", commandWidgetClass,
									form, args, n);
	XtAddCallback(done, XtNcallback, Radiance_Attributes_Done_Callback, NULL);

	n = m;
	XtSetArg(args[n], XtNlabel, "Simplified");		n++;
	XtSetArg(args[n], XtNfromVert, declare_widget);	n++;
	XtSetArg(args[n], XtNfromHoriz, done);			n++;
#if ( USE_ROUNDED_BUTTONS )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	simplified = XtCreateManagedWidget("radianceAttributesSimpleButton",
					commandWidgetClass, form, args, n);
	XtAddCallback(simplified, XtNcallback, Attributes_Simple_Callback,
				  (XtPointer)radiance_attributes_shell);
	
	n = m;
	XtSetArg(args[n], XtNlabel, "Cancel");			n++;
	XtSetArg(args[n], XtNfromVert, declare_widget);	n++;
	XtSetArg(args[n], XtNfromHoriz, simplified);	n++;
#if ( USE_ROUNDED_BUTTONS )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	cancel = XtCreateManagedWidget("radianceAttributesCancelButton",
								commandWidgetClass, form, args, n);
	XtAddCallback(cancel, XtNcallback, Attributes_Cancel_Callback,
					(XtPointer)radiance_attributes_shell);
	
	XtRealizeWidget(radiance_attributes_shell);
}
