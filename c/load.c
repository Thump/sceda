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
**	load.c : Code needed to load a scene description file.
**
**	External Functions:
**
**	void Load_Dialog_Func(Widget, XtPointer, XtPointer);
**	Puts up the load dialog box.
**
**	void Load_File(FILE*)
**	Loads the contents of a file.
**
*/

#include <sced.h>
#include <load.h>
#if ( HAVE_STRING_H )
#include <string.h>
#elif ( HAVE_STRINGS_H )
#include <strings.h>
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SelFile.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Command.h>

static void		Create_Changed_Load_Dialog();

static Widget	changed_load_shell = NULL;

char	*merge_filename = NULL;

static Boolean	last_was_pipe;


/*	void
**	Load_Dialog_Func(Widget, XtPointer, XtPointer);
**	If necessary, creates, then pops up the load file dialog box.
*/
void
Load_Dialog_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	FILE	*load_file;
	char	path_name[1024];
	Cursor	time_cursor;

	if ( changed_load_shell && (Widget)cl_data == changed_load_shell )
		XtPopdown(changed_load_shell);
	else if ( changed_scene )
	{
		if (!changed_load_shell)
			Create_Changed_Load_Dialog();

		/* Set the position of the popup. */
		SFpositionWidget(changed_load_shell);
		XtPopup(changed_load_shell, XtGrabExclusive);

		changed_scene = FALSE;
		return;
	}

	if ( ! io_file_name )
	{
		if ( scene_path )
			strcpy(path_name, scene_path);
		else
			getcwd(path_name, 1024);
		strcat(path_name, "/*.scn");
	}
	else
		strcpy(path_name, io_file_name);
	load_file = XsraSelFile(main_window.shell, "Load file:", "Load", "Cancel",
							NULL, path_name, "r", NULL, &io_file_name);

	/* Close it again, and reopen using our function. */
	if ( load_file )
	{
		fclose(load_file);
		load_file = Open_Load_File_Name(&io_file_name);
	}

	if ( load_file )
	{
		time_cursor = XCreateFontCursor(XtDisplay(main_window.shell), XC_watch);
    	XDefineCursor(XtDisplay(main_window.shell),
					  XtWindow(main_window.shell), time_cursor);

		Destroy_World(TRUE);
		Load_File(load_file, FALSE);

    	XDefineCursor(XtDisplay(main_window.shell), XtWindow(main_window.shell),
					  None);
		XFreeCursor(XtDisplay(main_window.shell), time_cursor);
	}

}


/*	void
**	Merge_Dialog_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
**	Merge-loads a file. Like load but doesn't do any destroying.
*/
void
Merge_Dialog_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	FILE	*merge_file;
	char	path_name[1024];
	Cursor	time_cursor;

	if ( ! merge_filename )
	{
		if ( scene_path )
			strcpy(path_name, scene_path);
		else
			getcwd(path_name, 1024);
		strcat(path_name, "/*.base");
		merge_filename = Strdup(path_name);
	}
	else
		strncpy(path_name, merge_filename, 1024);
	merge_file = XsraSelFile(main_window.shell, "Merge file:", "Merge",
						"Cancel", NULL, path_name, "r", NULL, &merge_filename);

	/* Close it again, and reopen using our function. */
	if ( merge_file )
	{
		fclose(merge_file);
		merge_file = Open_Load_File_Name(&merge_filename);
	}

	if ( ! io_file_name )
		io_file_name = Strdup(merge_filename);

	if (merge_file )
	{
		time_cursor = XCreateFontCursor(XtDisplay(main_window.shell), XC_watch);
    	XDefineCursor(XtDisplay(main_window.shell),
					  XtWindow(main_window.shell), time_cursor);

		Load_File(merge_file, TRUE);

    	XDefineCursor(XtDisplay(main_window.shell), XtWindow(main_window.shell),
					  None);
		XFreeCursor(XtDisplay(main_window.shell), time_cursor);
	}
}


static void
Cancel_Changed_Load_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	XtPopdown(changed_load_shell);
}


static void
Save_Load_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	XtPopdown(changed_load_shell);

	Save_Dialog_Func(NULL, (void*)SAVE_LOAD, NULL);
}


/*	void
**	Create_Changed_Load_Dialog()
**	Creates the popup shell for use with changed files.
*/
static void
Create_Changed_Load_Dialog()
{
	Widget	dialog_widget;
	Arg		args[5];
	int		n;


	changed_load_shell = XtCreatePopupShell("Changed Files",
						transientShellWidgetClass, main_window.shell, NULL, 0);

	/* Create the dialog widget to go inside the shell. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "The scene has changed:");	n++;
	dialog_widget = XtCreateManagedWidget("changedLoadDialog",
						dialogWidgetClass, changed_load_shell, args, n);

	/* Add the button at the bottom of the dialog. */
	XawDialogAddButton(dialog_widget, "Save", Save_Load_Func, NULL);
	XawDialogAddButton(dialog_widget, "Load", Load_Dialog_Func,
						changed_load_shell);
	XawDialogAddButton(dialog_widget, "Cancel", Cancel_Changed_Load_Func, NULL);

	XtVaSetValues(XtNameToWidget(dialog_widget, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(dialog_widget, "Save"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(dialog_widget, "Load"),
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

	XtRealizeWidget(changed_load_shell);
}



/*	FILE *Open_File_Name(char *name)
**	Attempts to open the named file. If it ends in .gz or .Z, uses
**	zcat to uncompress into a pipe.
*/
FILE*
Open_Load_File_Name(char **name)
{
	char	*extension;
	char	*command_string;
	char	*new_name;
	Boolean	new_new_name = FALSE;
	Boolean	is_compressed = FALSE;
	struct stat	stat_struct;
	FILE	*result;

	last_was_pipe = FALSE;
	new_name = *name;

	/* Determine the extension. */
	extension = strrchr(new_name, '.');
	if ( extension &&
		( ! strcmp(extension, ".gz") ||
		  ! strcmp(extension, ".z") ||
		  ! strcmp(extension, ".Z") ) )
		is_compressed = TRUE;

	while ( TRUE )
	{
		if ( is_compressed )
		{
			/* Check its existence. */
			if ( stat(new_name, &stat_struct) == -1 )
				return NULL;

			command_string = New(char, strlen(new_name) + 8);
			strcpy(command_string, "zcat ");
			strcat(command_string, new_name);
			result = popen(command_string, "r");
			free(command_string);

			last_was_pipe = TRUE;
			*name = new_name;

			return result;
		}
		else
		{
			/* Check its existence. */
			if ( stat(new_name, &stat_struct) != -1 )
			{
				result = fopen(new_name, "r");
				if ( result ) return result;
			}

			/* Here if we couldn't open the file. */
			/* Try compressed. */
			new_name = New(char, strlen(new_name) + 5);
			strcpy(new_name, *name);
			new_new_name = TRUE;
#if ( HAVE_GZIP )
			strcat(new_name, ".gz");
#else
			strcat(new_name, ".Z");
#endif
			is_compressed = TRUE;
			/* Now it loops around trying the compressed name. */
		}
	}
}


/*	void Close_File(FILE*)
**	Attempts to close the file. If the last file opened was opened
**	as a pipe, uses pclose, else uses fclose.
*/
void
Close_Load_File(FILE *victim)
{
	if ( last_was_pipe )
		pclose(victim);
	else
		fclose(victim);
}

void
Load_File(FILE *file, Boolean merge)
{
	int	token;

#if FLEX
	if ( yyin ) yyrestart(yyin);
#endif /* FLEX */
	yyin = file;

	line_num = 1;

	if ( ( token = yylex() ) == INTERNAL )
		Load_Internal_File(file, merge);
	else
		Load_Simple_File(file, merge, token);

	Close_Load_File(file);
}

