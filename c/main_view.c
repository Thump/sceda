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
**	main_view.c : Functions relating to the main viewing window.
**
**	Created: 26/03/94
**
**	External Functions:
**	void	Create_Main_Display();
**	Creates the main display interface.
**
*/


#include <sced.h>
#include <layers.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Viewport.h>
#include <View.h>
#if ELK_SUPPORT
#include <X11/Xaw/Paned.h>
#endif /* ELK_SUPPORT */


extern void Target_Callback(Widget, XtPointer, XtPointer);
extern void Define_Camera_Callback(Widget, XtPointer, XtPointer);
extern void	Export_Callback(Widget, XtPointer, XtPointer);
extern void	Copyright_Popup_Callback(Widget, XtPointer, XtPointer);
extern void	Help_Popup_Callback(Widget, XtPointer, XtPointer);
extern void Rename_Callback(Widget, XtPointer, XtPointer);



/* The translation table for object selection.  Defined in events.c. */
extern String selection_translations;
extern String main_translations;
extern String anim_translations;
extern String csg_translations;


/* Bitmap to appear on menu buttons. */
static unsigned int	menu_bitmap_width = 20;
static unsigned int	menu_bitmap_height = 12;
static char menu_bitmap_bits[] = {
   0xff, 0xff, 0x0f, 0x01, 0x00, 0x08, 0xfd, 0xff, 0x0b, 0xfd, 0xff, 0x0b,
   0x01, 0x00, 0x08, 0xfd, 0xff, 0x0b, 0xfd, 0xff, 0x0b, 0x01, 0x00, 0x08,
   0xfd, 0xff, 0x0b, 0xfd, 0xff, 0x0b, 0x01, 0x00, 0x08, 0xff, 0xff, 0x0f};
Pixmap	menu_bitmap;


/*
**	Function prototypes.
*/
static Widget	Create_Main_Buttons(int);
static void	Create_File_Menu();
static void	Create_Wireframe_Menu();
static void	Create_Object_Menu();
static void	Create_Lights_Menu();
static void Create_Animate_Menu();
static void	Create_Reset_Menu();
static void	Main_View_Initialize();
static void	Axes_Initialize();


/* Widgets. */
static Widget	main_window_form_widget;
static Widget	main_view_viewport;

/* I changed main_options_buttons[] from being static, and changed both 
** names to csg_* from being just *.  This is to allow me to access them
** outside of this source file, to determine sensitivity, without
** colliding with the option_buttons[] and option_count in csg_view.c.
** I've done a similar thing in csg_view.c.
*/
Widget	main_option_buttons[20];
static int		main_option_count = 0;

#if ELK_SUPPORT
static Widget	main_window_paned_widget;
static Widget	main_window_elk_widget;
extern String	elk_translations;
#endif /* ELK_SUPPORT */

extern Widget	wire_delete_button;

/* A wireframe for axes. */
Wireframe	axes_wireframe;


/*	void
**	Create_Main_Display()
**	Creates and initializes all the widgets for the main display.
*/
void
Create_Main_Display()
{
	Arg	args[15];
	int	n;

	Widget	button;				/* Any button. */

	String		shell_geometry;
	unsigned	form_width, form_height;	/* The size of the window. */
	Dimension	apply_width, label_width, label_height;	/* To lay out widgets.*/
	int		gap;				/* The gap between widgets in the form.	*/
	int		main_window_width;	/* Used for sizing widgets.	*/
	int		main_window_height;
	int		junk;

	/* Find out the geometry of the top level shell.	*/
	n = 0;
	XtSetArg(args[n], XtNgeometry, &shell_geometry);	n++;
	XtGetValues(main_window.shell, args, n);
	XParseGeometry(shell_geometry, &junk, &junk, &form_width, &form_height);

	/* Create main_window.shell's immediate child, a form widget. */
	n = 0;
	XtSetArg(args[n], XtNwidth, &form_width);		n++;
	XtSetArg(args[n], XtNheight, &form_height);		n++;
	main_window_form_widget = XtCreateManagedWidget("mainWindowForm",
					formWidgetClass, main_window.shell, args, n);

	/* Get the spacing of the form.  I need it to lay things out. */
	n = 0;
	XtSetArg(args[n], XtNdefaultDistance, &gap);	n++;
	XtGetValues(main_window_form_widget, args, n);

	main_window_width = (int)form_width;
	main_window_height = (int)form_height;

	/* Create the bitmap to appear inside menu buttons. */
	menu_bitmap = XCreateBitmapFromData(XtDisplay(main_window.shell),
					RootWindowOfScreen(XtScreen(main_window.shell)),
					menu_bitmap_bits, menu_bitmap_width, menu_bitmap_height);

	/* Create all the buttons.  Any one is returned. */
	button = Create_Main_Buttons(gap);

	apply_width = Match_Widths(main_option_buttons, main_option_count);

#if ELK_SUPPORT
{
	int temp_width, temp_height;
	
	/*
	 * Create pane widget to hold viewport and text widget
	 */
	n = 0;
	temp_width = main_window_width - (int)apply_width - 3 * gap - 4;
	temp_height = main_window_height - 40;
	XtSetArg(args[n], XtNwidth, temp_width); n++;
	XtSetArg(args[n], XtNheight, temp_height); n++;
	XtSetArg(args[n], XtNorientation, XtorientVertical); n++;
	XtSetArg(args[n], XtNfromHoriz, button); n++;
	XtSetArg(args[n], XtNleft, XtChainLeft); n++;
	XtSetArg(args[n], XtNright, XtChainRight); n++;
	XtSetArg(args[n], XtNtop, XtChainTop); n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom); n++;
	main_window_paned_widget=XtCreateManagedWidget("mainwindowPaneWidget",
		       panedWidgetClass, main_window_form_widget, args, n);
	
	/* First create a Viewport widget to hold the image. */
	temp_height *= 0.9;
	n = 0;
	XtSetArg(args[n], XtNwidth, temp_width); n++;
	XtSetArg(args[n], XtNheight, temp_height); n++;
	XtSetArg(args[n], XtNallowHoriz, TRUE); n++;
	XtSetArg(args[n], XtNallowVert, TRUE); n++;
	XtSetArg(args[n], XtNuseBottom, TRUE); n++;
	XtSetArg(args[n], XtNuseRight, TRUE); n++;
	XtSetArg(args[n], XtNresizeToPreferred, TRUE); n++;
	XtSetArg(args[n], XtNallowResize, TRUE); n++;
	main_view_viewport = XtCreateManagedWidget("mainViewViewport",
		   viewportWidgetClass, main_window_paned_widget, args, n);

	/*
	 * Here we add a text widget for the user to enter
	 * scheme commands.
	 */
	temp_height /= 9;
	n = 0;
	XtSetArg(args[n], XtNwidth, temp_width); n++;
	XtSetArg(args[n], XtNheight, temp_height); n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit); n++;
	XtSetArg(args[n], XtNscrollHorizontal, XawtextScrollWhenNeeded); n++;
	XtSetArg(args[n], XtNscrollVertical, XawtextScrollWhenNeeded); n++;
	main_window_elk_widget = XtCreateManagedWidget("mainwindowELKWindow",
					       asciiTextWidgetClass,
					       main_window_paned_widget,
					       args, n);
	XtOverrideTranslations(main_window_elk_widget,
			       XtParseTranslationTable(elk_translations));
}
#else /* ELK_SUPPORT */
	/* First create a Viewport widget to hold the image. */
	n = 0;
	XtSetArg(args[n], XtNwidth,
		 main_window_width - (int)apply_width - 3 * gap - 4);	n++;
	XtSetArg(args[n], XtNheight, main_window_height - 40);		n++;
	XtSetArg(args[n], XtNallowHoriz, TRUE);			n++;
	XtSetArg(args[n], XtNallowVert, TRUE);			n++;
	XtSetArg(args[n], XtNuseBottom, TRUE);			n++;
	XtSetArg(args[n], XtNuseRight, TRUE);			n++;
	XtSetArg(args[n], XtNfromHoriz, button);		n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainRight);		n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);	n++;
	main_view_viewport = XtCreateManagedWidget("mainViewViewport",
						viewportWidgetClass, main_window_form_widget, args, n);
#endif /* ELK_SUPPORT */


	n = 0;
	XtSetArg(args[n], XtNdesiredWidth,
			main_window_width - (int)apply_width - 3 * gap - 4);	n++;
	XtSetArg(args[n], XtNdesiredHeight, main_window_height - 40);	n++;
	XtSetArg(args[n], XtNmaintainSize, TRUE);					n++;
	XtSetArg(args[n], XtNmagnification, INIT_SCALE);			n++;
	main_window.view_widget = XtCreateManagedWidget("mainViewWindow",
								viewWidgetClass, main_view_viewport, args, n);

	/* Add the Apply button. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Apply");							n++;
	XtSetArg(args[n], XtNwidth, apply_width);						n++;
#if ELK_SUPPORT
	XtSetArg(args[n], XtNfromVert, main_window_paned_widget); n++;
#else /* ELK_SUPPORT */
	XtSetArg(args[n], XtNfromVert, main_view_viewport);				n++;
#endif /* ELK_SUPPORT */
	XtSetArg(args[n], XtNleft, XtChainLeft);						n++;
	XtSetArg(args[n], XtNright, XtChainLeft);						n++;
	XtSetArg(args[n], XtNtop, XtChainBottom);						n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);					n++;
	XtSetArg(args[n], XtNresizable, TRUE);							n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	main_window.apply_button = XtCreateManagedWidget("applyButton",
						commandWidgetClass, main_window_form_widget, args, n);
	XtAddCallback(main_window.apply_button, XtNcallback,
												Apply_Button_Callback, NULL);

	/* Add the text label. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Ready");						n++;
	XtSetArg(args[n], XtNwidth, apply_width);					n++;
#if ELK_SUPPORT
	XtSetArg(args[n], XtNfromVert, main_window_paned_widget); n++;
#else /* ELK_SUPPORT */
	XtSetArg(args[n], XtNfromVert, main_view_viewport);			n++;
#endif /* ELK_SUPPORT */
	XtSetArg(args[n], XtNfromHoriz, main_window.apply_button);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);					n++;
	XtSetArg(args[n], XtNright, XtChainLeft);					n++;
	XtSetArg(args[n], XtNtop, XtChainBottom);					n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);				n++;
	XtSetArg(args[n], XtNresizable, TRUE);						n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	main_window.text_label = XtCreateManagedWidget("textLabel",
						labelWidgetClass, main_window_form_widget, args, n);

	/*
	**	Add the text widget.
	*/
	/* Need to figure out how big to make it. */
	n = 0;
	XtSetArg(args[n], XtNwidth, &label_width);	n++;
	XtSetArg(args[n], XtNheight, &label_height);n++;
	XtGetValues(main_window.text_label, args, n);

	n = 0;
	main_window.text_string[0] = '\0';
	XtSetArg(args[n], XtNwidth,
			main_window_width - (int)apply_width - (int)label_width -
			4 * gap - 6);										n++;
	XtSetArg(args[n], XtNheight, label_height);					n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);				n++;
	XtSetArg(args[n], XtNlength, ENTRY_STRING_LENGTH);			n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);				n++;
	XtSetArg(args[n], XtNstring, main_window.text_string);		n++;
	XtSetArg(args[n], XtNresize, XawtextResizeWidth);			n++;
#if ELK_SUPPORT
	XtSetArg(args[n], XtNfromVert, main_window_paned_widget); n++;
#else /* ELK_SUPPORT */
	XtSetArg(args[n], XtNfromVert, main_view_viewport);			n++;
#endif /* ELK_SUPPORT */
	XtSetArg(args[n], XtNfromHoriz, main_window.text_label);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);					n++;
	XtSetArg(args[n], XtNright, XtChainRight);					n++;
	XtSetArg(args[n], XtNtop, XtChainBottom);					n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);				n++;
	XtSetArg(args[n], XtNresizable, TRUE);						n++;
	main_window.text_widget = XtCreateManagedWidget("textEntry",
						asciiTextWidgetClass, main_window_form_widget, args, n);
	/* Add translations. */
	XtOverrideTranslations(main_window.text_widget,
		XtParseTranslationTable(":<Key>Return: Apply_Button()"));


	/* Initialize the main viewing window. */
	Main_View_Initialize();

	/* Add the expose callback for the view widget.	*/
	XtAddCallback(main_window.view_widget, XtNexposeCallback,
					Redraw_Main_View, &main_window);

	XtRealizeWidget(main_window.shell);
}


/*	Widget
**	Create_Main_Buttons(int gap)
**	Creates all the command buttons for the main window.
**	Any button (the first) is returned.
*/
static Widget
Create_Main_Buttons(int gap)
{
	Arg			args[15];
	int			n;
	Dimension	width;


	/* Create the menus for menuButton popups. */
	Create_File_Menu();
	Create_Wireframe_Menu();
	Create_Object_Menu();
	Create_Lights_Menu();
	Create_Reset_Menu();
	Create_View_Menu(main_window.shell, TRUE, &main_window);
	Create_Window_Menu(main_window.shell, &main_window);
	Create_Edit_Menu(&main_window);
	Create_Animate_Menu();
	Layers_Create_Menu(&main_window);

	/* All the main_option_buttons have same chaining, so just set it once. */
	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;


	/* A File menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "File");			n++;
	XtSetArg(args[n], XtNfromVert, NULL);			n++;
	XtSetArg(args[n], XtNmenuName, "FileMenu");		n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);	n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("fileButton",
				menuButtonWidgetClass, main_window_form_widget, args, n);
	main_option_count++;

	/* A CSG window command button.	*/
	n = 5;
	XtSetArg(args[n], XtNlabel, "CSG Window");						n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("csgWindowButton",
						commandWidgetClass, main_window_form_widget, args, n);
	XtAddCallback(main_option_buttons[main_option_count], XtNcallback, CSG_Window_Popup,
					&(main_window.selected_instances));
	main_option_count++;

	/* A Wireframe menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Wireframe");						n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "WireframeMenu");				n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("wireframeButton",
				menuButtonWidgetClass, main_window_form_widget, args, n);
	main_option_count++;

	/* An Object menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Object");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "ObjectMenu");					n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("objectButton",
				menuButtonWidgetClass, main_window_form_widget, args, n);
	main_option_count++;

	/* A Lights menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Lights");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
	XtSetArg(args[n], XtNmenuName, "LightsMenu");					n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("lightsButton",
				menuButtonWidgetClass, main_window_form_widget, args, n);
	main_option_count++;

	/* A change viewport menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "View");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "ViewMenu");						n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("viewButton",
					menuButtonWidgetClass, main_window_form_widget, args, n);
	main_option_count++;

	/* A window menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Window");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "WindowMenu");					n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("windowButton",
					menuButtonWidgetClass, main_window_form_widget, args, n);
	main_option_count++;

	/* A layers menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Layers");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "LayerMenu");					n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("layerButton",
					menuButtonWidgetClass, main_window_form_widget, args, n);
	main_option_count++;


	/* A target button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Target");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("targetButton",
					commandWidgetClass, main_window_form_widget, args, n);
	XtAddCallback(main_option_buttons[main_option_count], XtNcallback, Target_Callback,
					NULL);
	main_option_count++;

	/* A camera button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Camera");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("cameraButton",
					commandWidgetClass, main_window_form_widget, args, n);
	XtAddCallback(main_option_buttons[main_option_count], XtNcallback,
							Define_Camera_Callback, NULL);
	main_option_count++;

	/* A Preview button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Preview");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("previewButton",
					commandWidgetClass, main_window_form_widget, args, n);
	XtAddCallback(main_option_buttons[main_option_count], XtNcallback,
							Preview_Callback, &main_window);
	main_option_count++;


	/* A clear world button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Clear");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
	XtSetArg(args[n], XtNmenuName, "ResetMenu");					n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("resetButton",
					menuButtonWidgetClass, main_window_form_widget, args, n);
	main_option_count++;

	/* A maintenance toggle. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Maintain");						n++;
	XtSetArg(args[n], XtNstate, TRUE);								n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	main_option_buttons[main_option_count] = XtCreateManagedWidget("maintainButton",
					toggleWidgetClass, main_window_form_widget, args, n);
	XtAddCallback(main_option_buttons[main_option_count], XtNcallback,
				  Maintain_Toggle_Callback, NULL);
	main_option_count++;

	/* An Edit menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Edit");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "EditMenu");						n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
	XtSetArg(args[n], XtNsensitive, FALSE);							n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	main_window.edit_menu->button = main_option_buttons[main_option_count] =
		XtCreateManagedWidget("editButton",
				menuButtonWidgetClass, main_window_form_widget, args, n);
	main_option_count++;

	/*  An animation menu. */
	n=5;
	XtSetArg(args[n], XtNlabel, "Animate");							n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]); n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);                      n++;
	XtSetArg(args[n], XtNmenuName, "AnimateMenu");                  n++;
	main_option_buttons[main_option_count] = XtCreateManagedWidget("animateButton",
		menuButtonWidgetClass, main_window_form_widget, args, n);
	main_option_count++;

	/* We match widths at this point to allow us to use the width in */
	/* in some of the following declarations.						 */
	width = Match_Widths(main_option_buttons, main_option_count-1);

	/* Add the keyframe label. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "KeyFrame");						n++;
	XtSetArg(args[n], XtNwidth, width);								n++;
	XtSetArg(args[n], XtNvertDistance, 3*gap);						n++;
	XtSetArg(args[n], XtNfromVert, main_option_buttons[main_option_count-1]); n++;
	XtSetArg(args[n], XtNborderWidth, 0);							n++;
	XtSetArg(args[n], XtNborderWidth, 0);							n++;
	main_window.kf_label = XtCreateManagedWidget("kfLabel",
		labelWidgetClass, main_window_form_widget, args, n);
	
	/* Add the keyframe number label. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "1 of 1");							n++;
	XtSetArg(args[n], XtNwidth, width);								n++;
	XtSetArg(args[n], XtNvertDistance, 0.5*gap);					n++;
	XtSetArg(args[n], XtNfromVert, main_window.kf_label);			n++;
	XtSetArg(args[n], XtNborderWidth, 0);							n++;
	main_window.kfnum_label = XtCreateManagedWidget("kfnumLabel",
						labelWidgetClass, main_window_form_widget, args, n);
  
	/* Add the frame label. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Frame");						n++;
	XtSetArg(args[n], XtNwidth, width);							n++;
	XtSetArg(args[n], XtNvertDistance, 2*gap);					n++;
	XtSetArg(args[n], XtNfromVert, main_window.kfnum_label);	n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	main_window.f_label = XtCreateManagedWidget("fLabel",
						labelWidgetClass, main_window_form_widget, args, n);

	/* Add the frame number label. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "1 of 1");						n++;
	XtSetArg(args[n], XtNwidth, width);							n++;
	XtSetArg(args[n], XtNvertDistance, 0.5*gap);				n++;
	XtSetArg(args[n], XtNfromVert, main_window.f_label);		n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	main_window.fnum_label = XtCreateManagedWidget("fnumLabel",
						labelWidgetClass, main_window_form_widget, args, n);

	return main_option_buttons[main_option_count - 1];
}


/*	void
**	Create_File_Menu()
**	Creates the menu which pops up from the File button.
**	Functions included: Load, Save, Export, Quit
*/
static void
Create_File_Menu()
{
	Widget	menu_widget;
	Widget	menu_children[7];

	int		count = 0;


	menu_widget = XtCreatePopupShell("FileMenu", simpleMenuWidgetClass,
										main_window.shell, NULL, 0);

	menu_children[count] = XtCreateManagedWidget("Load", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Load_Dialog_Func, NULL);
	count++;

	menu_children[count] = XtCreateManagedWidget("Merge", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Merge_Dialog_Func, NULL);
	count++;

	menu_children[count] = XtCreateManagedWidget("Save", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Save_Dialog_Func,
				  (void*)SAVE_ONLY);
	count++;

	menu_children[count] = XtCreateManagedWidget("Export", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Export_Callback, NULL);
	count++;

	menu_children[count] = XtCreateManagedWidget("Copyright", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Copyright_Popup_Callback,
				  NULL);
	count++;

	menu_children[count] = XtCreateManagedWidget("Help", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Help_Popup_Callback,
				  NULL);
	count++;

	menu_children[count] = XtCreateManagedWidget("Quit", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Quit_Dialog_Func, NULL);
	count++;
}


static void
Create_Wireframe_Menu()
{
	Widget	menu_widget;
	Widget	menu_children[2];

	int		count = 0;

	menu_widget = XtCreatePopupShell("WireframeMenu", simpleMenuWidgetClass,
										main_window.shell, NULL, 0);

	menu_children[count] = XtCreateManagedWidget("Load",
								smeBSBObjectClass, menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Wireframe_Load_Callback,
					NULL);
	count++;

	wire_delete_button =
	menu_children[count] = XtCreateManagedWidget("Delete", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtSetSensitive(wire_delete_button, FALSE);
	XtAddCallback(menu_children[count], XtNcallback, Wireframe_Delete_Callback,
					NULL);
	count++;
}

/*	void
**	Create_Object_Menu()
**	Creates the menu which pops up from the Edit button.
**	Functions included: Shape, Move, Copy, Name, Attributes
*/
static void
Create_Object_Menu()
{
	Widget	object_widget;
	Widget	object_children[10];

	int		count = 0;


	object_widget = XtCreatePopupShell("ObjectMenu", simpleMenuWidgetClass,
										main_window.shell, NULL, 0);

	object_children[count] = XtCreateManagedWidget("New", smeBSBObjectClass,
													object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback,
					New_Object_Popup_Callback, (XtPointer)&main_window);
	count++;

	object_children[count] = XtCreateManagedWidget("Edit", smeBSBObjectClass,
													object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback, Edit_Objects_Function,
					(XtPointer)&main_window);
	count++;

	object_children[count] = XtCreateManagedWidget("Copy", smeBSBObjectClass,
													object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback, Copy_Objects_Callback,
					(XtPointer)&main_window);
	count++;

	object_children[count] = XtCreateManagedWidget("Delete", smeBSBObjectClass,
													object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback, Delete_Objects_Callback,
				(XtPointer)&main_window);
	count++;

	XtCreateManagedWidget("line1", smeLineObjectClass, object_widget, NULL, 0);

	object_children[count] = XtCreateManagedWidget("Name", smeBSBObjectClass,
													object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback, Rename_Callback,
				  (XtPointer)&main_window);
	count++;

	object_children[count] = XtCreateManagedWidget("Attributes",
									smeBSBObjectClass, object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback, Set_Attributes_Callback,
					(XtPointer)&main_window);
	count++;

	XtCreateManagedWidget("line1", smeLineObjectClass, object_widget, NULL, 0);

	object_children[count] = XtCreateManagedWidget("Dense Wire",
								smeBSBObjectClass, object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback,
				  Wireframe_Denser_Callback, (XtPointer)&main_window);
	count++;

	object_children[count] = XtCreateManagedWidget("Thin Wire",
								smeBSBObjectClass, object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback,
				  Wireframe_Thinner_Callback, (XtPointer)&main_window);
	count++;

	object_children[count] = XtCreateManagedWidget("Change Base",
								smeBSBObjectClass, object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback, Base_Change_Callback,
					(XtPointer)&main_window);
	count++;
}


/*	void
**	Create_Lights_Menu()
**	Creates the menu which pops up from the Lights button.
**	Functions included: Create, Edit, Ambient
*/
static void
Create_Lights_Menu()
{
	Widget	lights_widget;
	Widget	lights_children[4];

	int		count = 0;


	lights_widget = XtCreatePopupShell("LightsMenu", simpleMenuWidgetClass,
										main_window.shell, NULL, 0);

	lights_children[count] = XtCreateManagedWidget("Point", smeBSBObjectClass,
													lights_widget, NULL, 0);
	XtAddCallback(lights_children[count], XtNcallback, Create_Light_Callback,
					NULL);
	count++;

	lights_children[count] = XtCreateManagedWidget("Spot", smeBSBObjectClass,
													lights_widget, NULL, 0);
	XtAddCallback(lights_children[count], XtNcallback,
					Create_Spotlight_Callback, NULL);
	count++;

	lights_children[count] = XtCreateManagedWidget("Area", smeBSBObjectClass,
													lights_widget, NULL, 0);
	XtAddCallback(lights_children[count], XtNcallback,
					Create_Arealight_Callback, NULL);
	count++;

	lights_children[count] = XtCreateManagedWidget("Ambient", smeBSBObjectClass,
													lights_widget, NULL, 0);
	XtAddCallback(lights_children[count], XtNcallback, Ambient_Light_Callback,
					NULL);
	count++;

}


static void
Create_Reset_Menu()
{
	Widget	reset_widget;
	Widget	reset_children[2];

	int		count = 0;


	reset_widget = XtCreatePopupShell("ResetMenu", simpleMenuWidgetClass,
										main_window.shell, NULL, 0);

	reset_children[count] = XtCreateManagedWidget("Clear Objects",
									smeBSBObjectClass, reset_widget, NULL, 0);
	XtAddCallback(reset_children[count], XtNcallback, Clear_Dialog_Func,
					(XtPointer)&main_window);
	count++;

	reset_children[count] = XtCreateManagedWidget("Reset", smeBSBObjectClass,
													reset_widget, NULL, 0);
	XtAddCallback(reset_children[count], XtNcallback, Reset_Dialog_Func,
					(XtPointer)&main_window);
	count++;
}


/*	void
**	Create_Animate_Menu()
**	Creates the menu which pops up from the Animate button.
**	Functions included: 
*/
static void
Create_Animate_Menu()
{
	Widget	animate_widget;
	Widget	animate_children[10];

	int		count = 0;


	animate_widget = XtCreatePopupShell
		("AnimateMenu",simpleMenuWidgetClass,main_window.shell,NULL,0);

	animate_children[count] = XtCreateManagedWidget
		("Next Key Frame",smeBSBObjectClass,animate_widget,NULL,0);
	XtAddCallback
		(animate_children[count],XtNcallback,(XtPointer)Next_KeyFrame,NULL);
	count++;

	animate_children[count] = XtCreateManagedWidget
		("Prev Key Frame",smeBSBObjectClass,animate_widget,NULL,0);
	XtAddCallback
		(animate_children[count],XtNcallback,(XtPointer)Prev_KeyFrame,NULL);
	count++;

	animate_children[count] = XtCreateManagedWidget
		("Clone Key Frame",smeBSBObjectClass,animate_widget,NULL,0);
	XtAddCallback
		(animate_children[count],XtNcallback,(XtPointer)Clone_KeyFrame,NULL);
	count++;

	animate_children[count] = XtCreateManagedWidget
		("Remove Key Frame",smeBSBObjectClass,animate_widget,NULL,0);
	XtAddCallback
		(animate_children[count],XtNcallback,(XtPointer)Remove_KeyFrame,NULL);
	count++;

	animate_children[count] = XtCreateManagedWidget
		("Pump All Keyframes",smeBSBObjectClass,animate_widget,NULL,0);
	XtAddCallback
		(animate_children[count],XtNcallback,(XtPointer)Pump_All,NULL);
	count++;

	animate_children[count] = XtCreateManagedWidget
		("Zero All Keyframes",smeBSBObjectClass,animate_widget,NULL,0);
	XtAddCallback
		(animate_children[count],XtNcallback,(XtPointer)Zero_All,NULL);
	count++;

	animate_children[count] = XtCreateManagedWidget
		("Animate",smeBSBObjectClass,animate_widget,NULL,0);
	XtAddCallback
		(animate_children[count],XtNcallback,(XtPointer)Animate,NULL);
	count++;

	animate_children[count] = XtCreateManagedWidget
		("Export Animation",smeBSBObjectClass,animate_widget,NULL,0);
	XtAddCallback
		(animate_children[count],XtNcallback,(XtPointer)Export_Animation,NULL);
	count++;

	animate_children[count] = XtCreateManagedWidget
		("Clone to All",smeBSBObjectClass,animate_widget,NULL,0);
	XtAddCallback
		(animate_children[count],XtNcallback,(XtPointer)Clone_to_Keyframes,NULL);
	count++;
}


/*	void
**	Main_View_Initialize()
**	Initializes the main viewport.
*/
static void
Main_View_Initialize()
{

	/* Set up the axes. */
	Axes_Initialize();

	/* Install translations. */
	XtOverrideTranslations(main_window.view_widget,
		XtParseTranslationTable(selection_translations));
	XtOverrideTranslations(main_window.view_widget,
		XtParseTranslationTable(main_translations));
	XtOverrideTranslations(main_window.view_widget,
		XtParseTranslationTable(anim_translations));
}


/*	void
**	Axes_Initialize();
**	Sets up endpoints for each axis, and allocates a drawing GC for each.
*/
static void
Axes_Initialize()
{
	XGCValues	gc_vals;
	int			i;

	NewIdentityMatrix(main_window.axes.o_transform.matrix);
	VNew(0, 0, 0, main_window.axes.o_transform.displacement);
	main_window.axes.o_num_vertices = 7;
	main_window.axes.o_world_verts = New(Vector, 7);
	main_window.axes.o_main_verts = New(Vertex, 7);
	main_window.axes.o_num_faces = 0;
	main_window.axes.o_flags |= ObjVisible;
	main_window.axes.o_label="main_window.axes";

	/* Allocate endpoints. */
	VNew(0, 0, 0, main_window.axes.o_world_verts[0]);
	VNew(sced_resources.x_axis_length / (double)sced_resources.axis_denom,
		 0.0, 0.0, main_window.axes.o_world_verts[1]);
	VNew(-sced_resources.x_axis_length / (double)sced_resources.axis_denom,
		 0.0, 0.0, main_window.axes.o_world_verts[2]);
	VNew(0.0, sced_resources.y_axis_length / (double)sced_resources.axis_denom,
		 0.0, main_window.axes.o_world_verts[3]);
	VNew(0.0, -sced_resources.y_axis_length / (double)sced_resources.axis_denom,
		 0.0, main_window.axes.o_world_verts[4]);
	VNew(0.0, 0.0,
		 sced_resources.z_axis_length / (double)sced_resources.axis_denom,
		 main_window.axes.o_world_verts[5]);
	VNew(0.0, 0.0,
		 -sced_resources.z_axis_length / (double)sced_resources.axis_denom,
		 main_window.axes.o_world_verts[6]);

	/* Create a dodgy wireframe. */
	axes_wireframe.num_faces = 0;
	axes_wireframe.num_vertices = 7;
	axes_wireframe.vertices = New(Vector, 7);
	for ( i = 0 ; i < 7 ; i++ )
		axes_wireframe.vertices[i] = main_window.axes.o_world_verts[i];
	main_window.axes.o_wireframe = &axes_wireframe;

	/* Allocate a GC for each axis. */
	gc_vals.line_width = sced_resources.axis_width;

	if ( DefaultDepthOfScreen(XtScreen(main_window.shell)) == 1 )
	{
		XtVaGetValues(main_window.view_widget,
					  XtNforeground, &(gc_vals.foreground), NULL);
		axis_gcs[0] = axis_gcs[1] = axis_gcs[2] =
			XtGetGC(main_window.shell, GCForeground | GCLineWidth, &gc_vals);
	}
	else
	{
		gc_vals.foreground = sced_resources.x_axis_color;
		axis_gcs[0] = XtGetGC(main_window.shell, GCForeground | GCLineWidth,
							  &gc_vals);
		gc_vals.foreground = sced_resources.y_axis_color;
		axis_gcs[1] = XtGetGC(main_window.shell, GCForeground | GCLineWidth,
							  &gc_vals);
		gc_vals.foreground = sced_resources.z_axis_color;
		axis_gcs[2] = XtGetGC(main_window.shell, GCForeground | GCLineWidth,
							  &gc_vals);
	}

}




/*	void
**	Sensitize_Main_Buttons(Boolean state)
**	A function to sensitize or desensitize all the buttons.
*/
void
Sensitize_Main_Buttons(Boolean state) 
{
	Arg	arg;
	int	i;

	XtSetArg(arg, XtNsensitive, state);

	/* The reason this function runs up to (main_option_count - 2) is
	** because this is not intended to alter the state of the Edit
	** button, and the Edit button is second from the end.  The sensitivity
	** of the Edit button is determined separately in Add_Instance_To_Edit()
	** and Delete_Edit_Instance().
	*/
	for ( i = 0 ; i < main_option_count - 2 ; i++ )
		XtSetValues(main_option_buttons[i], &arg, 1);

	/* But, although we have to stop at main_option_count-2 in order to
	** not fuck with the sensitivity of the edit button, we do want to
	** make the Animate button follow the rest of the buttons in sensitivity.
	** So, we have this line...
	*/
	XtSetValues(main_option_buttons[main_option_count-1], &arg, 1);

	if ( main_window.edit_menu->num_children != 0 )
		XtSetValues(main_window.edit_menu->button, &arg, 1);
}


