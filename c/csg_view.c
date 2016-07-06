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
**	csg_view.c : Functions relating to the csg viewing window.
*/

#include <ctype.h>
#include <sced.h>
#include <base_objects.h>
#include <csg.h>
#include <layers.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Tree.h>
#include <X11/Xaw/Viewport.h>
#include <View.h>

#if ELK_SUPPORT
#include <elk.h>
#endif


extern void Rename_Callback(Widget, XtPointer, XtPointer);

/* The translation table for object selection.  Defined in events.c. */
extern String selection_translations;
extern String csg_translations;

extern Pixmap	menu_bitmap;

/*
**	Function prototypes.
*/
static Widget	Create_CSG_Buttons(int);
static void	Create_CSG_Menu();
static void	Create_Object_Menu();
static void	Create_CSG_Tree_Menus();
static void	CSG_View_Initialize();
static void	Axes_Initialize();

void	Set_CSG_Related_Sensitivity(Boolean);

/* Widgets. */
Widget	csg_tree_widget;
Widget	csg_display_label[2];
static Widget	csg_window_form_widget;
static Widget	csg_view_viewport;
static Widget	csg_tree_viewport;
#if ELK_SUPPORT
static Widget	csg_elk_widget;
String	elk_translations = "Shift <Key>Return: elk_eval_string()";
static Cursor	time_cursor = 0;
#endif /* ELK_SUPPORT */
static Widget	csg_paned_widget;

/* I changed these two from being static, and changed their names
** to csg_* from being just *.  This is to allow me to access them
** outside of this source file, to determine sensitivity, without
** colliding with the option_buttons[] and option_count in main_view.c.
** I've done a similar thing in main_view.c.
*/
Widget	csg_option_buttons[10];
int		csg_option_count = 0;

/* Translations for the tree. */
static String	tree_translations =
				"<BtnDown>: csg_tree_notify() \n\
				 <Motion>:	csg_motion()";

static Widget	csg_modify_button;
static Widget	csg_delete_button;
static Widget	csg_copy_button;
static Widget	csg_save_button;
extern Widget	new_csg_button;

extern Wireframe	axes_wireframe;

#if ELK_SUPPORT
WindowInfoPtr	elk_window;	/* The current elk window. */
int elk_is_csg_window;		/* are we in the csg or main_window */
int	elk_use_csg = FALSE;	/* should we always use the csg window. */
int elk_use_main = FALSE;	/* should we always use the main window. */

/***********************************************************************
 *
 * Description: Elk_Eval_String() is the callback for when the users presses
 *		Shift-Return in the scheme command window. This function
 *		scans backward from the current insertion point to either
 *		the beginning of the previous word or the beginning of
 *		the previous scheme expression (i.e. matching parenthesis).
 *		The found text is then pased to Elk_Eval() for evaluation
 *		by the scheme interpreter. NOTE: This function is used
 *		by both the csg_window and the main_window.
 *
 * Return value: None
 *
 ***********************************************************************/
void
Elk_Eval_String(Widget w, XEvent *event, String *str, Cardinal *num)
{
	String s;
	int epos, spos, ipos, len;
	char c;
	char *p;
	XawTextBlock block;
	
	elk_is_csg_window = ( ! elk_use_main &&
						  ( elk_use_csg || w == csg_elk_widget ));
	elk_window = ( elk_is_csg_window ? &csg_window : &main_window );

	/*
	 * Here we look at the preceeding character (from the current
	 * insertion point) and if it is a ')' then we look for its
	 * match otherwise just go back as far as the previous word.
	 */
	XtVaGetValues(w,
		      XtNinsertPosition, &epos,
		      XtNstring, &s,
		      NULL);
	ipos = epos;
	if (epos == 0)
		return;
	epos--;
	while ((epos >= 0) && isspace(s[epos]))
		epos--;
	if (epos < 0)
		return;
	if (s[epos] == ')') {
		int count = 1;
		
		spos = epos;
		while((spos >= 0) && count) {
			if ((spos != epos) && (s[spos] == ')'))
				count++;
			else if (s[spos] == '(')
				count--;
			spos--;
		}
	} else {
		spos = epos;
		while((spos >= 0) && !isspace(s[spos]))
			spos--;
	}
	spos++;

	/* Set the cursor. */
	if ( ! time_cursor )
		time_cursor = XCreateFontCursor(XtDisplay(main_window.shell), XC_watch);
	XDefineCursor(XtDisplay(main_window.shell),
				  XtWindow(main_window.shell), time_cursor);
	if ( csg_window.shell )
		XDefineCursor(XtDisplay(csg_window.shell),
					  XtWindow(csg_window.shell), time_cursor);
	/* Flush the display. */
	XSync(XtDisplay(main_window.shell), False);

	len = epos - spos + 1;
	c = s[spos + len];
	s[spos + len] = '\0';
	p = Elk_Eval(&s[spos]);
	s[spos + len] = c;
	/*
	 * Now place the result of the scheme expression in the text widget.
	 */
	c = '\n';
	block.firstPos = 0;
	block.length = 1;
	block.ptr = &c;
	XawTextReplace(w, ipos, ipos, &block);
	ipos++;
	block.firstPos = 0;
	block.length = strlen(p);
	block.ptr = p;
	XawTextReplace(w, ipos, ipos, &block);
	ipos += block.length;
	block.firstPos = 0;
	block.length = 1;
	block.ptr = &c;
	XawTextReplace(w, ipos, ipos, &block);
	ipos++;
	XawTextSetInsertionPoint(w, ipos);

	/* Put the cursor back. */
   	XDefineCursor(XtDisplay(main_window.shell), XtWindow(main_window.shell),
				  None);
	if ( csg_window.shell )
		XDefineCursor(XtDisplay(csg_window.shell), XtWindow(csg_window.shell),
					  None);
	XSync(XtDisplay(main_window.shell), False);


	return;
}
#endif /* ELK_SUPPORT */

/*	void
**	Create_CSG_Display()
**	Creates and initializes all the widgets for the csg display.
*/
void
Create_CSG_Display()
{
	Arg	args[15];
	int	n;

	Widget	button;				/* Any button. */

	String		shell_geometry;
	unsigned	form_width, form_height;	/* The size of the window. */
	Dimension	apply_width, label_width, label_height;	/* To lay out widgets.*/
	int		gap;				/* The gap between widgets in the form.	*/
	int		csg_window_width;	/* Used for sizing widgets.	*/
	int		csg_window_height;
	int		temp_width, temp_height;
	int		junk;

	n = 0;
	XtSetArg(args[n], XtNtitle, "CSG Window");	n++;
	csg_window.shell = XtCreatePopupShell("csgShell",
						topLevelShellWidgetClass, main_window.shell, args, n);

	/* Find out the geometry of the top level shell.	*/
	n = 0;
	XtSetArg(args[n], XtNgeometry, &shell_geometry);	n++;
	XtGetValues(csg_window.shell, args, n);
	XParseGeometry(shell_geometry, &junk, &junk, &form_width, &form_height);

	/* Create csg_window.shell's immediate child, a form widget. */
	n = 0;
	XtSetArg(args[n], XtNwidth, &form_width);		n++;
	XtSetArg(args[n], XtNheight, &form_height);		n++;
	csg_window_form_widget = XtCreateManagedWidget("csgWindowForm",
					formWidgetClass, csg_window.shell, args, n);

	/* Get the spacing of the form.  I need it to lay things out. */
	n = 0;
	XtSetArg(args[n], XtNdefaultDistance, &gap);	n++;
	XtGetValues(csg_window_form_widget, args, n);

	csg_window_width = (int)form_width;
	csg_window_height = (int)form_height;

	/* Create all the buttons.  Any one is returned. */
	button = Create_CSG_Buttons(gap);

	apply_width = Match_Widths(csg_option_buttons, csg_option_count);

	/* Create a paned widget which will in turn hold the view and tree. */
	n = 0;
	temp_width = csg_window_width - (int)apply_width - 3 * gap - 4;
	temp_height = csg_window_height - 40;
	XtSetArg(args[n], XtNwidth, temp_width);				n++;
	XtSetArg(args[n], XtNheight, temp_height);				n++;
	XtSetArg(args[n], XtNorientation, XtorientVertical);	n++;
	XtSetArg(args[n], XtNfromHoriz, button);				n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);				n++;
	XtSetArg(args[n], XtNright, XtChainRight);				n++;
	XtSetArg(args[n], XtNtop, XtChainTop);					n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);			n++;
	csg_paned_widget = XtCreateManagedWidget("csgPaneWidget", panedWidgetClass,
									csg_window_form_widget, args, n);


	/* Create a Viewport widget to hold the image. */
	/* It is a child of the paned widget. */
	n = 0;
	temp_height *= 0.75;
	XtSetArg(args[n], XtNwidth, temp_width);		n++;
	XtSetArg(args[n], XtNheight, temp_height);		n++;
	XtSetArg(args[n], XtNallowHoriz, TRUE);			n++;
	XtSetArg(args[n], XtNallowVert, TRUE);			n++;
	XtSetArg(args[n], XtNuseBottom, TRUE);			n++;
	XtSetArg(args[n], XtNuseRight, TRUE);			n++;
	XtSetArg(args[n], XtNresizeToPreferred, TRUE);	n++;
	XtSetArg(args[n], XtNallowResize, TRUE);		n++;
	csg_view_viewport = XtCreateManagedWidget("csgViewViewport",
						viewportWidgetClass, csg_paned_widget, args, n);

	/* Then the view widget to go inside it. */
	n = 0;
	if ( csg_window.viewport.scr_width == 0 )
		XtSetArg(args[n], XtNwidth, temp_width);
	else
		XtSetArg(args[n], XtNwidth, csg_window.viewport.scr_width);
	n++;
	if ( csg_window.viewport.scr_height == 0 )
		XtSetArg(args[n], XtNheight, temp_height);
	else
		XtSetArg(args[n], XtNheight, csg_window.viewport.scr_height);
	n++;
	XtSetArg(args[n], XtNmaintainSize, FALSE);			n++;
	XtSetArg(args[n], XtNmagnification, INIT_SCALE);	n++;
	csg_window.view_widget = XtCreateManagedWidget("csgViewWindow",
								viewWidgetClass, csg_view_viewport, args, n);


	/* Create a Viewport widget to hold the CSG trees. */
	/* It is a child of the paned widget also. */
	n = 0;
#if ELK_SUPPORT
	temp_height /= 5;
#else /* ELK_SUPPORT */
	temp_height /= 3;
#endif /* ELK_SUPPORT */
	XtSetArg(args[n], XtNwidth, temp_width);		n++;
	XtSetArg(args[n], XtNheight, temp_height);		n++;
	XtSetArg(args[n], XtNallowHoriz, TRUE);			n++;
	XtSetArg(args[n], XtNallowVert, TRUE);			n++;
	XtSetArg(args[n], XtNuseBottom, TRUE);			n++;
	XtSetArg(args[n], XtNuseRight, TRUE);			n++;
	XtSetArg(args[n], XtNpreferredPaneSize, temp_height);	n++;
	XtSetArg(args[n], XtNresizeToPreferred, TRUE);	n++;
	/*XtSetArg(args[n], XtNallowResize, TRUE);		n++;*/
	csg_tree_viewport = XtCreateManagedWidget("csgTreeViewport",
						viewportWidgetClass, csg_paned_widget, args, n);

	/* Then the tree widget to go inside it. */
	n = 0;
	XtSetArg(args[n], XtNwidth, temp_width);		n++;
	XtSetArg(args[n], XtNheight, temp_height);		n++;
	XtSetArg(args[n], XtNgravity, NorthGravity);	n++;
	XtSetArg(args[n], XtNautoReconfigure, FALSE);	n++;
	XtSetArg(args[n], XtNhSpace, 8);				n++;
	XtSetArg(args[n], XtNvSpace, 8);				n++;
	csg_tree_widget = XtCreateManagedWidget("csgTreeWindow",
								treeWidgetClass, csg_tree_viewport, args, n);
	XtOverrideTranslations(csg_tree_widget,
							XtParseTranslationTable(tree_translations));
	XawTreeForceLayout(csg_tree_widget);

#if ELK_SUPPORT
	/*
	 * Here we add a text widget for the user to enter
	 * scheme commands.
	 */
	temp_height *= 0.6666;
	n = 0;
	XtSetArg(args[n], XtNwidth, temp_width); n++;
	XtSetArg(args[n], XtNheight, temp_height); n++;
	XtSetArg(args[n], XtNgravity, NorthGravity); n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit); n++;
	XtSetArg(args[n], XtNscrollHorizontal, XawtextScrollWhenNeeded); n++;
	XtSetArg(args[n], XtNscrollVertical, XawtextScrollWhenNeeded); n++;
	XtSetArg(args[n], XtNpreferredPaneSize, temp_height);	n++;
	XtSetArg(args[n], XtNresizeToPreferred, TRUE);	n++;
	XtSetArg(args[n], XtNallowResize, TRUE);		n++;
	csg_elk_widget = XtCreateManagedWidget("csgELKWindow",
					       asciiTextWidgetClass,
					       csg_paned_widget,
					       args, n);
	XtOverrideTranslations(csg_elk_widget,
			       XtParseTranslationTable(elk_translations));
#endif /* ELK_SUPPORT */

	/* Add the Apply button. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Apply");				n++;
	XtSetArg(args[n], XtNwidth, apply_width);			n++;
	XtSetArg(args[n], XtNfromVert, csg_paned_widget);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);			n++;
	XtSetArg(args[n], XtNright, XtChainLeft);			n++;
	XtSetArg(args[n], XtNtop, XtChainBottom);			n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);		n++;
	XtSetArg(args[n], XtNresizable, TRUE);				n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	csg_window.apply_button = XtCreateManagedWidget("applyButton",
						commandWidgetClass, csg_window_form_widget, args, n);
	XtAddCallback(csg_window.apply_button, XtNcallback,
												Apply_Button_Callback, NULL);

	/* Add the text label. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Ready");						n++;
	XtSetArg(args[n], XtNwidth, apply_width);					n++;
	XtSetArg(args[n], XtNfromVert, csg_paned_widget);			n++;
	XtSetArg(args[n], XtNfromHoriz, csg_window.apply_button);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);					n++;
	XtSetArg(args[n], XtNright, XtChainLeft);					n++;
	XtSetArg(args[n], XtNtop, XtChainBottom);					n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);				n++;
	XtSetArg(args[n], XtNresizable, TRUE);						n++;
	XtSetArg(args[n], XtNborderWidth, 0);						n++;
	csg_window.text_label = XtCreateManagedWidget("textLabel",
						labelWidgetClass, csg_window_form_widget, args, n);

	/*
	**	Add the text widget.
	*/
	/* Need to figure out how big to make it. */
	n = 0;
	XtSetArg(args[n], XtNwidth, &label_width);	n++;
	XtSetArg(args[n], XtNheight, &label_height);n++;
	XtGetValues(csg_window.text_label, args, n);

	n = 0;
	csg_window.text_string[0] = '\0';
	XtSetArg(args[n], XtNwidth,
			csg_window_width - (int)apply_width - (int)label_width -
			4 * gap - 6);										n++;
	XtSetArg(args[n], XtNheight, label_height);					n++;
	XtSetArg(args[n], XtNeditType, XawtextEdit);				n++;
	XtSetArg(args[n], XtNlength, ENTRY_STRING_LENGTH);			n++;
	XtSetArg(args[n], XtNuseStringInPlace, TRUE);				n++;
	XtSetArg(args[n], XtNstring, csg_window.text_string);		n++;
	XtSetArg(args[n], XtNresize, XawtextResizeWidth);			n++;
	XtSetArg(args[n], XtNfromVert, csg_paned_widget);			n++;
	XtSetArg(args[n], XtNfromHoriz, csg_window.text_label);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);					n++;
	XtSetArg(args[n], XtNright, XtChainRight);					n++;
	XtSetArg(args[n], XtNtop, XtChainBottom);					n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);				n++;
	XtSetArg(args[n], XtNresizable, TRUE);						n++;
	csg_window.text_widget = XtCreateManagedWidget("textEntry",
						asciiTextWidgetClass, csg_window_form_widget, args, n);
	/* Add translations. */
	XtOverrideTranslations(csg_window.text_widget,
		XtParseTranslationTable(":<Key>Return: Apply_Button()"));

	/* Initialize the main viewing window. */
	CSG_View_Initialize();

	/* Add the expose callback for the view widget.	*/
	XtAddCallback(csg_window.view_widget, XtNexposeCallback,
					Redraw_Main_View, &csg_window);

	/* Create all the menus for editing the tree. */
	Create_CSG_Tree_Menus();

	/* Set CSG button sensitivity. */
	if ( num_base_objects == NUM_GENERIC_OBJS )
		Set_CSG_Related_Sensitivity(FALSE);

	/* Realize it. */
	XtRealizeWidget(csg_window.shell);
}


/*	Widget
**	Create_CSG_Buttons(int gap)
**	Creates all the command buttons for the csg window.
**	Any button (the first) is returned.
*/
static Widget
Create_CSG_Buttons(int gap)
{
	Arg			args[15];
	int			n;


	/* Create the menus for menuButton popups. */
	Create_CSG_Menu();
	Create_Object_Menu();
	Create_View_Menu(csg_window.shell, TRUE, &csg_window);
	Create_Window_Menu(csg_window.shell, &csg_window);
	Create_Edit_Menu(&csg_window);
	Layers_Create_Menu(&csg_window);

	/* All the csg_option_buttons have same chaining, so just set it once. */
	n = 0;
	XtSetArg(args[n], XtNtop, XtChainTop);		n++;
	XtSetArg(args[n], XtNbottom, XtChainTop);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);	n++;
	XtSetArg(args[n], XtNright, XtChainLeft);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);		n++;


	/* A CSG menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "CSG");			n++;
	XtSetArg(args[n], XtNfromVert, NULL);		n++;
	XtSetArg(args[n], XtNmenuName, "CSGMenu");	n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);		n++;
#endif
	csg_option_buttons[csg_option_count] = XtCreateManagedWidget("csgButton",
				menuButtonWidgetClass, csg_window_form_widget, args, n);
	csg_option_count++;

	/* An Object menu button. */
	n = 5;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
	XtSetArg(args[n], XtNlabel, "Object");							n++;
	XtSetArg(args[n], XtNfromVert, csg_option_buttons[csg_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "ObjectMenu");					n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	csg_option_buttons[csg_option_count] = XtCreateManagedWidget("objectButton",
				menuButtonWidgetClass, csg_window_form_widget, args, n);
	csg_option_count++;

	/* A change viewport menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "View");							n++;
	XtSetArg(args[n], XtNfromVert, csg_option_buttons[csg_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "ViewMenu");						n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	csg_option_buttons[csg_option_count] = XtCreateManagedWidget("viewButton",
					menuButtonWidgetClass, csg_window_form_widget, args, n);
	csg_option_count++;

	/* A window menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Window");							n++;
	XtSetArg(args[n], XtNfromVert, csg_option_buttons[csg_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "WindowMenu");					n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	csg_option_buttons[csg_option_count] = XtCreateManagedWidget("windowButton",
					menuButtonWidgetClass, csg_window_form_widget, args, n);
	csg_option_count++;

	/* A layers menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Layers");							n++;
	XtSetArg(args[n], XtNfromVert, csg_option_buttons[csg_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "LayerMenu");					n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	csg_option_buttons[csg_option_count] = XtCreateManagedWidget("layerButton",
					menuButtonWidgetClass, csg_window_form_widget, args, n);
	csg_option_count++;

	/* A reset button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Clear CSG");						n++;
	XtSetArg(args[n], XtNfromVert, csg_option_buttons[csg_option_count-1]);	n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
	csg_option_buttons[csg_option_count] = XtCreateManagedWidget("resetButton",
					commandWidgetClass, csg_window_form_widget, args, n);
	XtAddCallback(csg_option_buttons[csg_option_count], XtNcallback, Clear_Dialog_Func,
					NULL);
	csg_option_count++;

	/* An Edit menu button. */
	n = 5;
	XtSetArg(args[n], XtNlabel, "Edit");							n++;
	XtSetArg(args[n], XtNfromVert, csg_option_buttons[csg_option_count-1]);	n++;
	XtSetArg(args[n], XtNmenuName, "EditMenu");						n++;
	XtSetArg(args[n], XtNvertDistance, 5*gap);						n++;
	XtSetArg(args[n], XtNsensitive, FALSE);							n++;
#if ( XtSpecificationRelease > 4 )
	XtSetArg(args[n], XtNleftBitmap, menu_bitmap);					n++;
#endif
	csg_window.edit_menu->button = csg_option_buttons[csg_option_count] =
		XtCreateManagedWidget("editButton",
				menuButtonWidgetClass, csg_window_form_widget, args, n);
	csg_option_count++;

	return csg_option_buttons[csg_option_count - 1];

}


/*
**	static void
**	Close_CSG_Callback(Widget w, XtPointer cl, XtPointer ca)
**	Just unmaps the window - iconifying it would be just as useful.
*/
/* This used to be static, but I needed to call it from my_misc.c, to
** make a keyboard accelerator out of it, so I changed it.
*/
void
Close_CSG_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XUnmapWindow(XtDisplay(csg_window.shell), XtWindow(csg_window.shell));
}


/*	void
**	Create_CSG_Menu()
**	Creates the menu which pops up from the CSG button.
*/
static void
Create_CSG_Menu()
{
	Widget	menu_widget;
	Widget	close_button;

	menu_widget = XtCreatePopupShell("CSGMenu", simpleMenuWidgetClass,
										csg_window.shell, NULL, 0);

	csg_modify_button = XtCreateManagedWidget("Modify Existing",
							smeBSBObjectClass, menu_widget, NULL, 0);
	XtAddCallback(csg_modify_button, XtNcallback,
				  CSG_Modify_Existing_Callback, NULL);

	csg_copy_button = XtCreateManagedWidget("Copy Existing",
							smeBSBObjectClass, menu_widget, NULL, 0);
	XtAddCallback(csg_copy_button, XtNcallback,
				  CSG_Copy_Existing_Callback, NULL);

	csg_save_button = XtCreateManagedWidget("Save OFF",
							smeBSBObjectClass, menu_widget, NULL, 0);
	XtAddCallback(csg_save_button, XtNcallback, CSG_Save_Callback, NULL);

	csg_delete_button = XtCreateManagedWidget("Delete Existing",
							smeBSBObjectClass, menu_widget, NULL, 0);
	XtAddCallback(csg_delete_button, XtNcallback,
				  CSG_Delete_Existing_Callback, NULL);

	close_button = XtCreateManagedWidget("Close", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(close_button, XtNcallback, Close_CSG_Callback, NULL);
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
	Widget	object_children[8];

	int		count = 0;


	object_widget = XtCreatePopupShell("ObjectMenu", simpleMenuWidgetClass,
										csg_window.shell, NULL, 0);

	object_children[count] = XtCreateManagedWidget("New", smeBSBObjectClass,
													object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback,
					New_Object_Popup_Callback, (XtPointer)&csg_window);
	count++;

	object_children[count] = XtCreateManagedWidget("Edit", smeBSBObjectClass,
													object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback, Edit_Objects_Function,
					(XtPointer)&csg_window);
	count++;

	XtCreateManagedWidget("line1", smeLineObjectClass, object_widget, NULL, 0);

	object_children[count] = XtCreateManagedWidget("Name", smeBSBObjectClass,
													object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback, Rename_Callback,
				  (XtPointer)&csg_window);
	count++;

	object_children[count] = XtCreateManagedWidget("Attributes",
								smeBSBObjectClass, object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback, Set_Attributes_Callback,
					(XtPointer)&csg_window);
	count++;

	XtCreateManagedWidget("line1", smeLineObjectClass, object_widget, NULL, 0);

	object_children[count] = XtCreateManagedWidget("Dense Wire",
								smeBSBObjectClass, object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback,
				  Wireframe_Denser_Callback, (XtPointer)&csg_window);
	count++;

	object_children[count] = XtCreateManagedWidget("Thin Wire",
								smeBSBObjectClass, object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback,
				  Wireframe_Thinner_Callback, (XtPointer)&csg_window);
	count++;

	object_children[count] = XtCreateManagedWidget("Change Base",
								smeBSBObjectClass, object_widget, NULL, 0);
	XtAddCallback(object_children[count], XtNcallback, Base_Change_Callback,
					(XtPointer)&csg_window);
	count++;
}


static void
Create_Common_Menu_Widgets(Widget parent)
{
	Widget	current;

	current = XtCreateManagedWidget("Copy", smeBSBObjectClass, parent,
									NULL, 0);
	XtAddCallback(current, XtNcallback, CSG_Copy_Callback, NULL);
								
	current = XtCreateManagedWidget("Delete", smeBSBObjectClass, parent,
									NULL, 0);
	XtAddCallback(current, XtNcallback, CSG_Delete_Callback, NULL);
}

static Widget
Create_Root_Menu_Widgets(Widget parent)
{
	Widget	result;
	Widget	current;

	result =
	current = XtCreateManagedWidget("Display", smeBSBObjectClass, parent,
									NULL, 0);
	XtAddCallback(current, XtNcallback, CSG_Display_Callback, NULL);
								
	current = XtCreateManagedWidget("Move", smeBSBObjectClass, parent,
									NULL, 0);
	XtAddCallback(current, XtNcallback, Init_Motion_Sequence,
				  (XtPointer)csg_move);
								
	current = XtCreateManagedWidget("Attach", smeBSBObjectClass, parent,
									NULL, 0);
	XtAddCallback(current, XtNcallback, Init_Motion_Sequence,
				  (XtPointer)csg_attach);

	XtCreateManagedWidget("menuLine", smeLineObjectClass, parent, NULL, 0);
								
	current = XtCreateManagedWidget("Complete", smeBSBObjectClass, parent,
									NULL, 0);
	XtAddCallback(current, XtNcallback, CSG_Complete_Callback,(XtPointer)FALSE);

	return result;
}

static void
Create_Internal_Menu_Widgets(Widget parent)
{
	Widget	current;

	current = XtCreateManagedWidget("ReOrder", smeBSBObjectClass, parent,
									NULL, 0);
	XtAddCallback(current, XtNcallback, CSG_Reorder_Callback, NULL);

	current = XtCreateManagedWidget("Preview", smeBSBObjectClass, parent,
									NULL, 0);
	XtAddCallback(current, XtNcallback, CSG_Preview_Callback, NULL);
}


static void
Create_Internal_Root_Menu()
{
	Widget	menu_widg;

	menu_widg = XtCreatePopupShell("InternalRootMenu", simpleMenuWidgetClass,
										csg_window.shell, NULL, 0);
	csg_display_label[0] =
	Create_Root_Menu_Widgets(menu_widg);
	XtCreateManagedWidget("menuLine", smeLineObjectClass, menu_widg, NULL, 0);
	Create_Internal_Menu_Widgets(menu_widg);
	XtCreateManagedWidget("menuLine", smeLineObjectClass, menu_widg, NULL, 0);
	Create_Common_Menu_Widgets(menu_widg);
}

static void
Create_Internal_Menu()
{
	Widget	menu_widg;
	Widget	current;

	menu_widg = XtCreatePopupShell("InternalMenu", simpleMenuWidgetClass,
										csg_window.shell, NULL, 0);
	Create_Internal_Menu_Widgets(menu_widg);
	XtCreateManagedWidget("menuLine", smeLineObjectClass, menu_widg, NULL, 0);
	current = XtCreateManagedWidget("Evaluate", smeBSBObjectClass, menu_widg,
									NULL, 0);
	XtAddCallback(current, XtNcallback, CSG_Complete_Callback, (XtPointer)TRUE);
	current = XtCreateManagedWidget("Break", smeBSBObjectClass, menu_widg,
									NULL, 0);
	XtAddCallback(current, XtNcallback, CSG_Break_Callback, NULL);
	Create_Common_Menu_Widgets(menu_widg);
}

static void
Create_External_Root_Menu()
{
	Widget	menu_widg;

	menu_widg = XtCreatePopupShell("ExternalRootMenu", simpleMenuWidgetClass,
										csg_window.shell, NULL, 0);
	csg_display_label[1] =
	Create_Root_Menu_Widgets(menu_widg);
	XtCreateManagedWidget("menuLine", smeLineObjectClass, menu_widg, NULL, 0);
	Create_Common_Menu_Widgets(menu_widg);
}

static void
Create_External_Menu()
{
	Widget	menu_widg;
	Widget	current;

	menu_widg = XtCreatePopupShell("ExternalMenu", simpleMenuWidgetClass,
										csg_window.shell, NULL, 0);
	current = XtCreateManagedWidget("Break", smeBSBObjectClass, menu_widg,
									NULL, 0);
	XtAddCallback(current, XtNcallback, CSG_Break_Callback, NULL);
	Create_Common_Menu_Widgets(menu_widg);
}

static void
Create_Attach_Menu()
{
	Widget	menu_widg;
	Widget	current;

	menu_widg = XtCreatePopupShell("AttachMenu", simpleMenuWidgetClass,
										csg_window.shell, NULL, 0);

	current = XtCreateManagedWidget("Union", smeBSBObjectClass, menu_widg,
									NULL, 0);
	XtAddCallback(current, XtNcallback, Combine_Menu_Callback,
				  (XtPointer)csg_union_op);

	current = XtCreateManagedWidget("Intersection", smeBSBObjectClass,
									menu_widg, NULL,0);
	XtAddCallback(current, XtNcallback, Combine_Menu_Callback,
				  (XtPointer)csg_intersection_op);

	current = XtCreateManagedWidget("Difference", smeBSBObjectClass,
									menu_widg, NULL, 0);
	XtAddCallback(current, XtNcallback, Combine_Menu_Callback,
				  (XtPointer)csg_difference_op);

	current = XtCreateManagedWidget("Cancel", smeBSBObjectClass, menu_widg,
									NULL, 0);
	XtAddCallback(current, XtNcallback, Cancel_Combine_Menu_Callback, NULL);

}


static void
Create_CSG_Tree_Menus()
{
	/* There are different menus depending on what sort of node it is and
	** where it is in the tree. */
	Create_Internal_Root_Menu();
	Create_Internal_Menu();
	Create_External_Root_Menu();
	Create_External_Menu();
	Create_Attach_Menu();

}


/*	void
**	CSG_View_Initialize()
**	Initializes the csg viewport.
*/
static void
CSG_View_Initialize()
{

	/* Set up the axes. */
	Axes_Initialize();

	/* Install translations. */
	XtOverrideTranslations(csg_window.view_widget,
		XtParseTranslationTable(selection_translations));
	XtOverrideTranslations(csg_window.view_widget,
		XtParseTranslationTable(csg_translations));

}


/*	void
**	Axes_Initialize();
**	Sets up endpoints for each axis, and allocates a drawing GC for each.
*/
static void
Axes_Initialize()
{
	int	i;

	NewIdentityMatrix(csg_window.axes.o_transform.matrix);
	VNew(0, 0, 0, csg_window.axes.o_transform.displacement);
	csg_window.axes.o_num_vertices = 7;
	csg_window.axes.o_world_verts = New(Vector, 7);
	csg_window.axes.o_main_verts = New(Vertex, 7);
	csg_window.axes.o_num_faces = 0;
	csg_window.axes.o_flags |= ObjVisible;
	csg_window.axes.o_wireframe = &axes_wireframe;
	for ( i = 0 ; i < axes_wireframe.num_vertices ; i++ )
		csg_window.axes.o_world_verts[i] = axes_wireframe.vertices[i];
}




/*	void
**	Sensitize_CSG_Buttons(Boolean state)
**	A function to sensitize or desensitize all the buttons.
*/
void
Sensitize_CSG_Buttons(Boolean state) 
{
	Arg	arg;
	int	i;

	if ( ! csg_window.shell ) return;

	XtSetArg(arg, XtNsensitive, state);
	for ( i = 0 ; i < csg_option_count - 1 ; i++ )
		XtSetValues(csg_option_buttons[i], &arg, 1);

	if ( csg_window.edit_menu->num_children != 0 )
		XtSetValues(csg_window.edit_menu->button, &arg, 1);

	XtSetSensitive(csg_tree_widget, state);
}



/*	void
**	Set_CSG_Related_Sensitivity(Boolean state)
**	Sets the state of buttons which are only relevent if CSG objects are
**	defined.
*/
void
Set_CSG_Related_Sensitivity(Boolean state)
{
	Arg	arg;
	XtSetArg(arg, XtNsensitive, state);
	if ( csg_modify_button )
	{
		XtSetValues(csg_modify_button, &arg, 1);
		XtSetValues(csg_copy_button, &arg, 1);
		XtSetValues(csg_delete_button, &arg, 1);
		XtSetValues(csg_save_button, &arg, 1);
	}
	if ( new_csg_button )
		XtSetValues(new_csg_button, &arg, 1);
}
