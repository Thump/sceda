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
**	sced.c : The master file.  Actually, it doesn't do too much.  Just sets
**				up a few widgets and lets it run.
**
**	Created: 20/03/94
*/

#include <sced.h>
#include <layers.h>
#include <select_point.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>

#if ELK_SUPPORT
#include <elk.h>
#endif /* ELK_SUPPORT */

#if ( ! defined(MAXPATHLEN) )
#define MAXPATHLEN 1024
#endif


/*
**	Declaration of global variables.
*/
XtAppContext	app_context;	/* The application context for the program.	*/
WindowInfo		main_window;
WindowInfo		csg_window;
ScedResources	sced_resources;	/* Application resources. */

double	version = VERSION_FLOAT;


Boolean	changed_scene = FALSE;	/* Whether or not the scene has been modified.*/

/* Various file names. */
char	start_directory_name[MAXPATHLEN];
char	*io_file_name = NULL;
char	*scene_path = NULL;
Boolean	compress_output = FALSE;
Boolean	save_simple_wires = FALSE;

/* GCs for drawing the axes. */
GC				axis_gcs[3];

/* Default attributes. */
Attributes	default_attributes;

/* Temporary filenames (for removal). */
char	**temp_filenames = NULL;
int		num_temp_files = 0;

/* The global list of key frames. */
KeyFrameList  key_frames;

/* The global debug value, as set from the -d command line switch:
** only available if SCED_DEBUG is defined
*/
int debugv=0;


extern void	Load_Defaults_File(char*);
extern void Create_Main_Display();

static void Set_Defaults();
static void Initialize_Sced(int, char**, char**);

static char	*defaults_filename = NULL;

/*
**	Declaration of local variables.
*/
/* Fallback resources. Geometries and colors. */
/* Colors are needed because the default resources don't work as expected. */
String	fallbacks[] = {
	"Sceda.geometry: 800x600",
	"Sceda.csgShell.geometry: 800x600",
	"Sceda.newObject.geometry: 400x300",
	"Sceda.wireSelectShell.geometry: 400x300",
	"Sceda.csgSelectShell.geometry: 400x300",
	"Sceda.csgReferenceShell.geometry: 400x400",
	"Sceda.specAttributesShell.geometry: 400x300",
	NULL
	};

/* Actions. */
XtActionsRec	actions[] = {
#if ELK_SUPPORT
	{ "elk_eval_string", Elk_Eval_String },
#endif /* ELK_SUPPORT */
	{ "Start_Newview_Rotation", Start_Newview_Rotation },
	{ "Newview_Rotation", Newview_Rotation },
	{ "Stop_Newview_Rotation", Stop_Newview_Rotation },
	{ "Start_Distance_Change", Start_Distance_Change },
	{ "Distance_Change", Distance_Change },
	{ "Stop_Distance_Change", Stop_Distance_Change },
	{ "Start_Selection_Drag", Start_Selection_Drag },
	{ "Continue_Selection_Drag", Continue_Selection_Drag },
	{ "Finish_Selection_Drag", Finish_Selection_Drag },
	{ "Edit_Set_Cursor", Edit_Set_Cursor_Action },
	{ "Edit_Start_Drag", Edit_Start_Drag },
	{ "Edit_Continue_Drag", Edit_Continue_Drag },
	{ "Edit_Finish_Drag", Edit_Finish_Drag },
	{ "Edit_Add_Name", Add_Name_Action_Func },
	{ "Highlight_Object", Select_Highlight_Object },
	{ "Select_Point", Select_Point_Action },
	{ "Highlight_Closest", Select_Highlight_Action },
	{ "Rename_Object", Rename_Action_Func },
	{ "Apply_Button", Apply_Button_Action },
	{ "Zoom_Action", Zoom_Action_Func },
	{ "Ambient_Action", Ambient_Action_Func },
	{ "Light_Action", Light_Action_Func },
	{ "View_Name_Action", View_Name_Action_Func },
	{ "csg_tree_notify", CSG_Tree_Notify_Func },
	{ "menu_notify", CSG_Menu_Button_Up_Func },
	{ "csg_motion", CSG_Tree_Motion_Func },
	{ "csg_complete", CSG_Complete_Action_Func },
	{ "New_Layer_Action", New_Layer_Action_Function },
	{ "Renderman_File_Action", Renderman_Action_Func },
	{ "Next_KeyFrame", Next_KeyFrame },
	{ "Prev_KeyFrame", Prev_KeyFrame },
	{ "Clone_KeyFrame", Clone_KeyFrame },
	{ "PumpUp_All", PumpUp_All },
	{ "PumpUp_Next", PumpUp_Next },
	{ "PumpDown_All", PumpDown_All },
	{ "PumpDown_Next", PumpDown_Next },
	{ "Animate", Animate },
	{ "Export_Animation", Export_Animation },
	{ "Get_String_Return", Get_String_Return },
	{ "Remove_KeyFrame", Remove_KeyFrame },
	{ "Clone_to_Current", Clone_to_Current },
	{ "Clone_to_Next", Clone_to_Next },
	{ "Clone_to_Keyframes", Clone_to_Keyframes },
	{ "Delete_from_Keyframes", Delete_from_Keyframes },
	{ "Zero_All", Zero_All },
	{ "Zero_Next", Zero_Next },
	{ "Pump_All", Pump_All },
	{ "Clone_Attributes", Clone_Attributes },
	{ "Edit_Accel", Edit_Accel },
	{ "Viewfrom_Accel", Viewfrom_Accel },
	{ "Apply_Accel", Apply_Accel },
	{ "Recall_Accel", Recall_Accel },
	{ "Zoom_Accel", Zoom_Accel },
	{ "Synch_View", Synch_View },
	{ "Pan_Accel", Pan_Accel },
	{ "LA_Accel", LA_Accel },
	{ "Quit_Accel", Quit_Accel },
	{ "Close_Accel", Close_Accel },
	{ "QQuit_Accel", QQuit_Accel },
	{ "unEdit_Accel", unEdit_Accel },
	{ "New_Accel", New_Accel },
	{ "Help_Accel", Help_Accel },
	{ "Clone_to_Prev", Clone_to_Prev },
	{ "Maintenance", Maintenance }
	};

/* Resources. */
#define Offset(field)	XtOffsetOf(ScedResources, field)
XtResource	resources[] = {
	{"xAxisColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(x_axis_color),
	 XtRString, "red"},
	{"yAxisColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(y_axis_color),
	 XtRString, "green"},
	{"zAxisColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(z_axis_color),
	 XtRString, "blue"},
	{"axisWidth", XtCWidth, XtRInt, sizeof(int), Offset(axis_width),
	 XtRImmediate, (XtPointer)2},
	{"xAxisLength", XtCLength, XtRInt, sizeof(int), Offset(x_axis_length),
	 XtRImmediate, (XtPointer)2},
	{"yAxisLength", XtCLength, XtRInt, sizeof(int), Offset(y_axis_length),
	 XtRImmediate, (XtPointer)2},
	{"zAxisLength", XtCLength, XtRInt, sizeof(int), Offset(z_axis_length),
	 XtRImmediate, (XtPointer)2},
	{"axisDenom", XtCLength, XtRInt, sizeof(int), Offset(axis_denom),
	 XtRImmediate, (XtPointer)1},
	{"majorAxisColor", XtCColor, XtRPixel, sizeof(Pixel),
	 Offset(obj_x_axis_color), XtRString, "red"},
	{"minorAxisColor", XtCColor, XtRPixel, sizeof(Pixel),
	 Offset(obj_y_axis_color), XtRString, "green"},
	{"otherAxisColor", XtCColor, XtRPixel, sizeof(Pixel),
	 Offset(obj_z_axis_color), XtRString, "blue"},
	{"editAxisWidth", XtCWidth, XtRInt, sizeof(int), Offset(obj_axis_width),
	 XtRImmediate, (XtPointer)2},
	{"majorAxisLength", XtCLength, XtRInt, sizeof(int),
	 Offset(obj_x_axis_length), XtRImmediate, (XtPointer)4},
	{"minorAxisLength", XtCLength, XtRInt, sizeof(int),
	 Offset(obj_y_axis_length), XtRImmediate, (XtPointer)3},
	{"otherAxisLength", XtCLength, XtRInt, sizeof(int),
	 Offset(obj_z_axis_length), XtRImmediate, (XtPointer)2},
	{"editAxisDenom", XtCLength, XtRInt, sizeof(int), Offset(obj_axis_denom),
	 XtRImmediate, (XtPointer)2},
	{"editPointRadius", XtCWidth, XtRInt, sizeof(int), Offset(edit_pt_rad),
	 XtRImmediate, (XtPointer)10},
	{"scalingColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(scaling_color),
	 XtRString, "red"},
	{"originColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(origin_color),
	 XtRString, "green"},
	{"objectColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(object_color),
	 XtRString, "blue"},
	{"selectColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(selected_color),
	 XtRString, "red"},
	{"selectWidth", XtCWidth, XtRInt, sizeof(int), Offset(selected_width),
	 XtRImmediate, (XtPointer)0},
	{"lightColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(light_color),
	 XtRString, "yellow"},
	{"lightPointRadius", XtCWidth, XtRInt, sizeof(int), Offset(light_pt_rad),
	 XtRImmediate, (XtPointer)12},
	{"constraintColor", XtCColor, XtRPixel, sizeof(Pixel),
	 Offset(constraint_color), XtRString, "grey"},
	{"planeConLength", XtCLength, XtRInt, sizeof(int), Offset(plane_con_length),
	 XtRImmediate, (XtPointer)6},
	{"lineConLength", XtCLength, XtRInt, sizeof(int), Offset(line_con_length),
	 XtRImmediate, (XtPointer)6},
	{"pointConWidth", XtCWidth, XtRInt, sizeof(int), Offset(point_con_rad),
	 XtRImmediate, (XtPointer)15},
	{"inconConLength", XtCLength, XtRInt, sizeof(int), Offset(incon_con_length),
	 XtRImmediate, (XtPointer)15},
	{"originConWidth", XtCLength, XtRInt, sizeof(int), Offset(origin_con_width),
	 XtRImmediate, (XtPointer)7},
	{"scaleConWidth", XtCLength, XtRInt, sizeof(int), Offset(scale_con_width),
	 XtRImmediate, (XtPointer)5},
	{"rotateConWidth", XtCLength, XtRInt, sizeof(int), Offset(rotate_con_width),
	 XtRImmediate, (XtPointer)3},
	{"referencedColor", XtCColor, XtRPixel, sizeof(Pixel),
	 Offset(referenced_color), XtRString, "red"},
	{"activeColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(active_color),
	 XtRString, "red"},
	{"selectPointColor", XtCColor, XtRPixel, sizeof(Pixel),
	 Offset(selected_pt_color), XtRString, "blue"},
	{"selectPointWidth", XtCWidth, XtRInt, sizeof(int), Offset(select_pt_width),
	 XtRImmediate, (XtPointer)8},
	{"selectPointLineWidth", XtCWidth, XtRInt, sizeof(int),
	 Offset(select_pt_line_width), XtRImmediate, (XtPointer)2},
	{"absoluteColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(absolute_color),
	 XtRString, "green"},
	{"offsetColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(offset_color),
	 XtRString, "blue"},
	{"referenceColor", XtCColor, XtRPixel, sizeof(Pixel),
	 Offset(reference_color), XtRPixel, "red"},
	{"arcballColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(arcball_color),
	 XtRString, "grey"}
	};
#undef Offset


void
main(int argc, char *argv[], char *envp[])
{
	FILE 	*infile = NULL;
	Cursor	time_cursor;

	Set_WindowInfo(&main_window);
	Set_WindowInfo(&csg_window);
	Initialize_KeyFrames(&key_frames);

	/* Create the application shell. */
	/* This gets rid of X command line arguments. */
	main_window.shell = XtVaAppInitialize(&app_context, "Sceda",
				(XrmOptionDescList)NULL, 0, &argc, argv, fallbacks,
				XtNtitle, "Scene Window", NULL);

	XtGetApplicationResources(main_window.shell, (XtPointer)&sced_resources,
							  resources, XtNumber(resources), (Arg*)NULL, 0);

	/* Global initialization routine. */
	Initialize_Sced(argc, argv, envp);

	/* Register actions. */
	XtAppAddActions(app_context, actions, XtNumber(actions));

	/* Create the contents. */
	Create_Main_Display();

	/* Realize it all. */
	XtRealizeWidget(main_window.shell);

	/* Init drawing. */
	Draw_Initialize();

	/* Load the defaults file, if specified. */
	Load_Defaults_File(defaults_filename);

	/* Load the scene file, if specified. */
	if ( io_file_name )
	{
		/* Try to open the file in the current directory. */
		if ((infile = Open_Load_File_Name(&io_file_name)) == NULL && scene_path)
		{
			/* Couldn't open it. Try opening in the scenes directory. */
			char	*new_filename = New(char, strlen(io_file_name) +
											  strlen(scene_path) + 4);

			sprintf(new_filename, "%s/%s", scene_path, io_file_name);
			if ( ( infile = Open_Load_File_Name(&new_filename) ) == NULL )
				fprintf(stderr, "Unable to open file %s\n", io_file_name);
			else
			{
				free(io_file_name);
				io_file_name = new_filename;
			}
		}
		/* Check the first character. We want it to be an absolute pathname,
		** since SelFile screws up the current dir otherwise.
		** This in turn screws previewing.
		*/
		if ( io_file_name[0] != '/' )
		{
			char	current_dir[MAXPATHLEN];
			char	*temp;

            /* We used to support getwd() and getcwd(), but getwd() is a
            ** security risk, and getcwd() has been around forever, so I'm
            ** removing getwd()
            */
			getcwd(current_dir, MAXPATHLEN);
			temp = New(char, strlen(io_file_name) + strlen(current_dir) + 5);
			sprintf(temp, "%s/%s", current_dir, io_file_name);
			free(io_file_name);
			io_file_name = temp;
		}
	}
	if ( infile )
	{
		time_cursor = XCreateFontCursor(XtDisplay(main_window.shell), XC_watch);
    	XDefineCursor(XtDisplay(main_window.shell),
					  XtWindow(main_window.shell), time_cursor);

		Load_File(infile, FALSE);

    	XDefineCursor(XtDisplay(main_window.shell), XtWindow(main_window.shell),
					  None);
		XFreeCursor(XtDisplay(main_window.shell), time_cursor);
	}

#if ELK_SUPPORT
	/*
	 * Fire up scheme interpretor
	 */
	init_elk(argc, argv);
#endif /* ELK_SUPPORT */
	
	/* Loop for events. */
	XtAppMainLoop(app_context);

}


/*	void
**	Initialize_Sced(int argc, char *argv[], char *envp[])
**	Parses the command line, then calls initialization functions.
*/
static void
Initialize_Sced(int argc, char *argv[], char *envp[])
{
	int		i;
	char	*temp_s;
	int		w, h;
	Boolean	have_filename = FALSE;

	/* Any options left on the command line are mine.	*/
	/* Options at the moment are:
	** -I WxH
	** -F filename
	** -D defaults_file
	**
	** -v # to set the debug value: only valid if SCED_DEBUG was defined at
	**      compile time.  Check File/Copyright to see if it was compiled
	**      in. (Can't use -d: X grabs it.)
	*/
	for ( i = 1 ; i < argc ; i++ )
	{
		if ( argv[i][0] != '-' )
		{
			/* Assume it's a filename. */
			if ( have_filename )
				printf("Unexpected argument: %s\n", argv[i]);
			else
			{
				io_file_name = Strdup(argv[i]);
				have_filename = TRUE;
			}
		}
		else
		{
			switch ( argv[i][1] )
			{
				case 'F':
					if ( argv[i][2] != '\0' )
						io_file_name = Strdup(argv[i] + 2);
					else
					{
						i++;
						io_file_name = Strdup(argv[i]);
					}
					have_filename = TRUE;
					break;

				case 'D':
					if ( argv[i][2] != '\0' )
						defaults_filename = Strdup(argv[i] + 2);
					else
					{
						i++;
						defaults_filename = Strdup(argv[i]);
					}
					break;

				case 'I':
					if ( argv[i][2] == '\0' )
					{
						i++;
						temp_s = argv[i];
					}
					else
						temp_s = argv[i] + 2;
					if ( ( sscanf(temp_s, "%dx%d", &w, &h) == 2 ) ||
						 ( sscanf(temp_s, "%dX%d", &w, &h) == 2 ) )
					{
						camera.scr_width = (Dimension)w;
						camera.scr_height = (Dimension)h;
					}
					else
						printf("Invalid geometry string: %s\n", temp_s);
					break;
#ifdef SCED_DEBUG
				case 'v':
					if ( argv[i][2] != '\0' )
						debugv = atoi(Strdup(argv[i] + 2));
					else
					{
						i++;
						debugv = atoi(Strdup(argv[i]));
					}
					fprintf(stderr,"Setting debug level to %d\n",debugv);
					break;
#endif
				default:
					printf("Unknown argument: %s\n", argv[i]);
			}
		}
	}

	/* Set defaults for everything that needs one, such as the viewports. */
	Set_Defaults();

	/* Initialize the default base objects. */
	Initialize_Base_Objects();

	/* Initialize visibility layer info. */
	Layers_Init();

	/* Store the start directory, because SelFile tends to trash it, but
	** preview needs it. */
    /* We used to support getwd() and getcwd(), but getwd() is a
    ** security risk, and getcwd() has been around forever, so I'm
    ** removing getwd()
    */
	getcwd(start_directory_name, MAXPATHLEN);
}


/*	void
**	Set_Defaults()
**	Sets default values for:
**	- main_viewport
**	- csg_viewport
*/
static void
Set_Defaults()
{
	/* Set the default attributes for all new objects. */
	default_attributes.defined = TRUE;
	default_attributes.colour.red = DEFAULT_RED;
	default_attributes.colour.green = DEFAULT_GREEN;
	default_attributes.colour.blue = DEFAULT_BLUE;
	default_attributes.diff_coef = DEFAULT_DIFFUSE;
	default_attributes.spec_coef = DEFAULT_SPECULAR;
	default_attributes.spec_power = DEFAULT_SPECULAR * 100;
	default_attributes.reflect_coef = DEFAULT_REFLECT;
	default_attributes.refract_index = DEFAULT_REFRACT;
	default_attributes.transparency = DEFAULT_TRANSPARENCY;
	default_attributes.use_extension = FALSE;
	default_attributes.extension = NULL;
	default_attributes.use_obj_trans = FALSE;
	default_attributes.open = FALSE;

	/* Build the default viewports. */
	Viewport_Init(&(main_window.viewport));
	Viewport_Init(&(csg_window.viewport));

}
