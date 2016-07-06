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
**	events.c : event handling functions.
**
*/

#include <sced.h>
#include <select_point.h>
#include <View.h>
#include <X11/cursorfont.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Text.h>

/* The widget which is receiving special attention. */
static WindowInfoPtr	active_window = &main_window;
static Boolean	changing_view;
static Cursor	cursor;

/* Translation tables. */
String	selection_translations =
		"<Btn1Down> : Start_Selection_Drag() \n\
		 <Btn2Down> : Start_Selection_Drag() \n\
		 <Btn1Motion> : Continue_Selection_Drag() \n\
		 <Btn2Motion> : Continue_Selection_Drag() \n\
		 <Btn1Up> : Finish_Selection_Drag(add) \n\
		 <Btn2Up> : Finish_Selection_Drag(delete)";
String	edit_translations =
		"<BtnDown> : Edit_Start_Drag() \n\
		 <PtrMoved> : Edit_Continue_Drag() \n\
		 :<Key>e: unEdit_Accel() \n\
		 <BtnUp> : Edit_Finish_Drag()";
String	change_view_translations =
		"<Btn1Down> : Start_Newview_Rotation(Both) \n"
		"<Btn2Down> : Start_Newview_Rotation(Horiz) \n"
		"<Btn3Down> : Start_Newview_Rotation(Vert) \n"
		"<BtnMotion> : Newview_Rotation() \n"
		"<Key>v: Apply_Accel() \n"
		"<BtnUp> : Stop_Newview_Rotation() ";
String	pan_translations =
		"<Btn1Down> : Start_Newview_Rotation(Both, Pan) \n\
		 <Btn2Down> : Start_Newview_Rotation(Horiz, Pan) \n\
		 <Btn3Down> : Start_Newview_Rotation(Vert, Pan) \n\
		 <BtnMotion> : Newview_Rotation() \n\
		 <Key>p : Apply_Accel() \n\
		 <BtnUp> : Stop_Newview_Rotation() ";
String	change_distance_translations =
		"<Btn1Down> : Start_Distance_Change(Med, View) \n\
		 <Btn2Down> : Start_Distance_Change(Fast, View) \n\
		 <Btn3Down> : Start_Distance_Change(Slow, View) \n\
		 <BtnMotion> : Distance_Change() \n\
		 <BtnUp> : Stop_Distance_Change() ";
String	change_eye_translations =
		"<Btn1Down> : Start_Distance_Change(Med, Eye) \n\
		 <Btn2Down> : Start_Distance_Change(Fast, Eye) \n\
		 <Btn3Down> : Start_Distance_Change(Slow, Eye) \n\
		 <BtnMotion> : Distance_Change() \n\
		 <BtnUp> : Stop_Distance_Change() ";
String	select_translations =
		"<BtnDown> : Highlight_Object() \n\
		 <PtrMoved> : Highlight_Closest() \n\
		 <BtnUp> : Select_Point() ";
String  anim_translations =
		"!<Key>Left: Prev_KeyFrame() \n\
		 !<Key>Right: Next_KeyFrame() \n\
		 !<Key>Tab: Clone_KeyFrame() \n\
		 !Meta<Key>Tab: Remove_KeyFrame() \n\
		 !Meta<Key>Up: PumpUp_All() \n\
		 !Ctrl<Key>Up: PumpUp_Next() \n\
		 !Meta<Key>Down: PumpDown_All() \n\
		 !Ctrl<Key>Down: PumpDown_Next() \n\
		 !<Key>z: Zero_Next() \n\
		 !:<Key>Z: Zero_All() \n\
		 !<Key>f: Pump_All() \n\
		 !<Key>a: Animate() \n\
		 !<Key>x: Export_Animation() \n\
		 !<Key>c: Clone_to_Current() \n\
		 !:<Key>C: Clone_to_Keyframes() \n\
		 !Meta<Key>c: Clone_to_Prev() \n\
		 !Ctrl<Key>c: Clone_to_Next() \n\
		 !<Key>d: Delete_from_Keyframes() \n\
		 !:<Key>A: Clone_Attributes() \n\
		 !<Key>y: Synch_View() \n\
		 !<Key>m: Maintenance() ";
String  main_translations =
		"!<Key>Up: Zoom_Accel(Main 1) \n\
		 !<Key>Down: Zoom_Accel(Main -1) \n\
		 !Shift<Key>Up: Zoom_Accel(Main 25) \n\
		 !Shift<Key>Down: Zoom_Accel(Main -25) \n\
		 !<Key>e: Edit_Accel(Main) \n\
		 !<Key>r: Recall_Accel(Main) \n\
		 !<Key>p: Pan_Accel(Main) \n\
		 !<Key>l: LA_Accel(Main) \n\
		 !<Key>q: Quit_Accel() \n\
		 !:<Key>Q: QQuit_Accel() \n\
		 !<Key>n: New_Accel(Main) \n\
		 !<Key>F1: Help_Accel() \n\
		 !<Key>v: Viewfrom_Accel(Main) ";
String  csg_translations =
		"!<Key>Up: Zoom_Accel(CSG 1) \n\
		 !<Key>Down: Zoom_Accel(CSG -1) \n\
		 !Shift<Key>Up: Zoom_Accel(CSG 25) \n\
		 !Shift<Key>Down: Zoom_Accel(CSG -25) \n\
		 !<Key>v: Viewfrom_Accel(CSG) \n\
		 !<Key>r: Recall_Accel(CSG) \n\
		 !<Key>p: Pan_Accel(CSG) \n\
		 !<Key>q: Close_Accel() \n\
		 !<Key>l: LA_Accel(CSG) \n\
		 !<Key>n: New_Accel(CSG) \n\
		 !<Key>F1: Help_Accel() \n\
		 !<Key>e: Edit_Accel(CSG) ";

/* Apply 'callback' function. */
extern void Apply_Viewfrom_Text(WindowInfoPtr);
extern void Apply_Distance_Text(WindowInfoPtr, Boolean);
extern void	Prepare_Change_Look(WindowInfoPtr);

void	Cancel_Change_Look_Event();


/* A function to clear the prompt at the bottom of the screen. */
static void Reset_Prompt();




void
Cancel_Viewport_Change()
{
	if ( active_window->current_state & change_viewfrom )
		active_window->current_state ^= change_viewfrom;
	else
		active_window->current_state ^= change_distance;

	XtUninstallTranslations(active_window->view_widget);
	if ( active_window->current_state & edit_object )
	{
		if (active_window == &main_window)
			XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(main_translations));
		else
			XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(csg_translations));
			
		/* Do nothing. */
		/* Well, we should more properly be doing this nothing after
		** we load the main_translations, so we don't override the 
		** 'e' key translation...
		*/
		XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(edit_translations));

		Edit_Sensitize_Buttons(TRUE, FALSE);
	}
	else
	{
		XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(selection_translations));

		if (active_window == &main_window)
		{	XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(main_translations));
			XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(anim_translations));
		}
		else
			XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(csg_translations));

		Sensitize_Main_Buttons(TRUE);
		Sensitize_CSG_Buttons(TRUE);
	}

	/* Change the cursor back. */
	XDefineCursor(XtDisplay(active_window->shell),
				XtWindow(active_window->view_widget), None);
	XFreeCursor(XtDisplay(active_window->shell), cursor);

	Reset_Prompt();
}



WindowInfoPtr
Get_Active_Window()
{
	return active_window;
}


void
Init_View_Change()
{
	char		prompt_string[ENTRY_STRING_LENGTH];

	/* Change the cursor. */
	cursor = XCreateFontCursor(XtDisplay(active_window->shell), XC_fleur);
    XDefineCursor(XtDisplay(active_window->shell),
					XtWindow(active_window->view_widget), cursor);

	/* Set the label at the bottom of the screen. */
	XtVaSetValues(active_window->text_label,
				XtNlabel, "Viewpoint:", XtNjustify, XtJustifyRight, NULL);

	/* Set the text at the bottom of the screen. */
	sprintf(prompt_string, "%1.3g %1.3g %1.3g",
			active_window->viewport.view_from.x,
			active_window->viewport.view_from.y,
			active_window->viewport.view_from.z);
	Set_Prompt(active_window, prompt_string);

	Sensitize_Main_Buttons(FALSE);
	Sensitize_CSG_Buttons(FALSE);
	if ( active_window->current_state & edit_object )
		Edit_Sensitize_Buttons(FALSE, FALSE);

	active_window->current_state |= change_viewfrom;
}



/*	void
**	Initiate_Viewfrom_Change(Widget w, XtPointer data, XtPointer call_data)
**	Sets up a view change operation by adding translations to the
**	appropriate widget.
*/
void
Initiate_Viewfrom_Change(Widget w, XtPointer cl_data, XtPointer call_data)
{
	if ( cl_data )
		active_window = (WindowInfoPtr)cl_data;

	/* Install the new translations. */
	XtOverrideTranslations(active_window->view_widget,
							XtParseTranslationTable(change_view_translations));

	Init_View_Change();
}


/*	void
**	Initiate_Pan_Change(Widget w, XtPointer data, XtPointer call_data)
**	Sets up a pan operation by adding translations to the
**	appropriate widget.
*/
void
Initiate_Pan_Change(Widget w, XtPointer cl_data, XtPointer call_data)
{
	if ( cl_data )
		active_window = (WindowInfoPtr)cl_data;

	/* Install the new translations. */
	XtOverrideTranslations(active_window->view_widget,
							XtParseTranslationTable(pan_translations));

	Init_View_Change();
}


/*	void
**	Initiate_Distance_Change(WindowInfoPtr w, Boolean do_view)
**	Sets up a view distance change operation by adding translations to the
**	appropriate widget.
*/
void
Initiate_Distance_Change(WindowInfoPtr w, Boolean do_view)
{
	char	prompt_string[ENTRY_STRING_LENGTH];
	Arg		args[5];
	int		n;

	active_window = w;

	n = 0;
	/* Install the new translations. */
	if (do_view)
	{
		XtOverrideTranslations(w->view_widget,
					XtParseTranslationTable(change_distance_translations));
		cursor = XCreateFontCursor(XtDisplay(w->shell), XC_center_ptr);
		XtSetArg(args[n], XtNlabel, "Distance:");	n++;
		sprintf(prompt_string, "%1.3g", w->viewport.view_distance);
	}
	else
	{
		XtOverrideTranslations(w->view_widget,
					XtParseTranslationTable(change_eye_translations));
		cursor = XCreateFontCursor(XtDisplay(w->shell), XC_target);
		XtSetArg(args[n], XtNlabel, "Eye:");	n++;
		sprintf(prompt_string, "%1.3g", w->viewport.eye_distance);
	}

	/* Change the cursor. */
    XDefineCursor(XtDisplay(w->shell), XtWindow(w->view_widget), cursor);

	/* Set the label at the bottom of the screen. */
	XtSetArg(args[n], XtNjustify, XtJustifyRight);	n++;
	XtSetValues(w->text_label, args, n);

	/* Force an update. */
	Set_Prompt(w, prompt_string);

	active_window->current_state |= change_distance;

	Sensitize_Main_Buttons(FALSE);
	Sensitize_CSG_Buttons(FALSE);

	changing_view = do_view;

}



void
Apply_Button_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	if ( active_window->current_state & change_viewfrom )
		Apply_Viewfrom_Text(active_window);
	else if ( active_window->current_state & change_distance )
		Apply_Distance_Text(active_window, changing_view);
}


void
Apply_Button_Action(Widget w, XEvent *e, String *s, Cardinal *c)
{
	XtCallCallbacks(active_window->apply_button, XtNcallback, NULL);
}




static void
Reset_Prompt()
{
	Dimension	width;

	/* Set the label at the bottom of the screen. */
	XtVaGetValues(active_window->apply_button, XtNwidth, &width, NULL);
	XtVaSetValues(active_window->text_label,
				XtNwidth, width,
				XtNjustify, XtJustifyCenter,
				XtNlabel, "Ready", NULL);

	/* Clear the text entry. */
	Set_Prompt(active_window, "");
}


/*	void
**	Redraw_Main_View(Widget w, XtPointer client_data, XtPointer call_data)
**	The exposure callback for the screens.
*/
void
Redraw_Main_View(Widget w, XtPointer client_data, XtPointer call_data)
{
	WindowInfoPtr		window = (WindowInfoPtr)client_data;
	Dimension			new_width, new_height;

	XtVaGetValues(window->view_widget,
				  XtNwidth, &new_width,
				  XtNheight, &new_height, NULL);

	if ( new_height == window->height && new_width == window->width )
		View_Update(window, window->all_instances, JustExpose);
	else
	{
		View_Update(window, window->all_instances, CalcScreen);
		Update_Projection_Extents(window->all_instances);
	}
}



/*	void
**	Initiate_Object_Edit(WindowInfoPtr window)
**	Prepares the widget for a placement sequence.
**	This means drawing everything in a new way and desensitizing most of the
**	buttons on the screen.
*/
void
Initiate_Object_Edit(WindowInfoPtr window)
{
	active_window = window;

	active_window->current_state |= edit_object;

	Sensitize_Main_Buttons(FALSE);
	Sensitize_CSG_Buttons(FALSE);

	XtUninstallTranslations(window->view_widget);

	if (active_window == &main_window)
		XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(main_translations));
	else
		XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(csg_translations));

	/* Hmmm, 'kay, these two lines used to be above the immediately
	** preceding if() statement.  This broke when I began using the
	** 'e' key to both start and finish edit sessions, since the
	** keymap that made the 'e' key start an edit session (in the
	** main_translations and csg_translations map) were loaded after 
	** the translation that made 'e' finish a translation.  Anyhow,
	** suffice to say that putting it here fixes things, and since the
	** 'e' key is the only overlap between {main,csg}_translations and 
	** edit_translations, I don't think this will have any other effect.
	**
	** Gosh, that's a long comment just to explain moving two lines.
	*/
	XtOverrideTranslations(window->view_widget,
		XtParseTranslationTable(edit_translations));
}


void
Cancel_Object_Edit()
{
	active_window->current_state ^= edit_object;

	Sensitize_Main_Buttons(TRUE);
	Sensitize_CSG_Buttons(TRUE);

	XtUninstallTranslations(active_window->view_widget);
	XtOverrideTranslations(active_window->view_widget,
							XtParseTranslationTable(selection_translations));
	if (active_window == &main_window)
	{	XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(main_translations));
		XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(anim_translations));
	}
	else
		XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(csg_translations));

	active_window->current_state = wait;
}



/*	void
**	Change_Lookat_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**	Initiates a change to the lookat point in the window passed as cl_data.
*/
void
Change_Lookat_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	if ( cl_data )
		active_window = (WindowInfoPtr)cl_data; 

	active_window->current_state |= change_look;

	Sensitize_Main_Buttons(FALSE);
	Sensitize_CSG_Buttons(FALSE);
	if ( active_window->current_state & edit_object )
		Edit_Sensitize_Buttons(FALSE, FALSE);

	num_verts_required = 1;
	select_window = active_window;
	select_highlight = FALSE;
	specs_allowed[reference_spec] =
	specs_allowed[offset_spec] =
	specs_allowed[absolute_spec] = TRUE;
	select_center = active_window->viewport.view_at;
	select_finished_callback = Change_Lookat_Point;
	Prepare_Change_Look(active_window);

	Register_Select_Operation();
}


/*	void
**	Change_Lookup_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**	Initiates a change to the lookup vector in the window passed as cl_data.
*/
void
Change_Lookup_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	active_window = (WindowInfoPtr)cl_data; 

	active_window->current_state |= change_look;

	Sensitize_Main_Buttons(FALSE);
	Sensitize_CSG_Buttons(FALSE);

	num_verts_required = 2;
	select_window = active_window;
	select_highlight = FALSE;
	specs_allowed[reference_spec] =
	specs_allowed[absolute_spec] = TRUE;
	specs_allowed[offset_spec] = FALSE;
	select_center = active_window->viewport.view_at;
	select_finished_callback = Change_Lookup_Vector;
	Prepare_Change_Look(active_window);

	Register_Select_Operation();
}

/*	void
**	Cancel_Change_Look_Event()
**	Cancels a currently active look_ event.
*/
void
Cancel_Change_Look_Event()
{
	Cancel_Select_Operation();

	active_window->current_state ^= change_look;

	if ( active_window->current_state == wait )
	{
		Sensitize_Main_Buttons(TRUE);
		Sensitize_CSG_Buttons(TRUE);
	}
	else if ( active_window->current_state & edit_object )
		Edit_Sensitize_Buttons(TRUE, FALSE);
}


void
Register_Select_Operation()
{
	active_window->current_state |= window_select;

	XtUninstallTranslations(active_window->view_widget);

	if (active_window == &main_window)
	{	XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(main_translations));
		XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(anim_translations));

		/* if we're currently editing an object, load the edit
		** key translations (otherwise the 'e' key screws up...)
		*/
		if ( active_window->current_state & edit_object )
			XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(edit_translations));
	}
	else
	{	XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(csg_translations));

		/* if we're currently editing an object, load the edit
		** key translations (otherwise the 'e' key screws up...)
		*/
		if ( active_window->current_state & edit_object )
			XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(edit_translations));
	}

	/* plus we also drag these down here (they used to be above
	** the if statement, to prevent them being overridden.
	*/
	XtOverrideTranslations(active_window->view_widget,
							XtParseTranslationTable(select_translations));
}


void
Cancel_Select_Operation()
{
	active_window->current_state ^= window_select;

	XtUninstallTranslations(active_window->view_widget);

	if ( active_window->current_state & edit_object )
	{	if (active_window == &main_window)
			XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(main_translations));
		else
			XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(csg_translations));

		/* These two lines used to be the first to be executed in this
		** 'then' clause, but the main_translations were overriding
		** the 'e' translation, so I moved them...
		*/
		XtOverrideTranslations(active_window->view_widget,
						XtParseTranslationTable(edit_translations));
	}
	else
	{	XtOverrideTranslations(active_window->view_widget,
			XtParseTranslationTable(selection_translations));
		if (active_window == &main_window)
		{	XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(main_translations));
			XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(anim_translations));
		}
		else
			XtOverrideTranslations(active_window->view_widget,
				XtParseTranslationTable(csg_translations));
	}
}


