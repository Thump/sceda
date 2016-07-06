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

#define PATCHLEVEL 0
/*
**	sced: A Constraint Based Object Scene Editor
**
**	extern.h : global external variable declarations.
**
**	Created 20/03/94
*/


/*
**	Version info:
*/
extern double	version;

/*
**	Variables containing all the window information - widgets, instances etc.
*/
extern XtAppContext		app_context;
extern WindowInfo		main_window;
extern WindowInfo		csg_window;

/*
**	A record of whether the scene has been changed.
*/
extern Boolean	changed_scene;

/*
**	Assorted file names.
*/
extern char	*io_file_name;
extern char	*scene_path;

/*
**	Whether or not to compress output.
*/
extern Boolean	compress_output;

/*
**	Whether or not to save all wireframes.
*/
extern Boolean	save_simple_wires;

/*
**	The camera.  Declared in camera.c
*/
extern Camera camera;

/*
**	The three world axes.
*/
extern GC	axis_gcs[3];

/*
**	An array of object counts.  For generating default names.
**	Declared in new_object.c
*/
extern int	object_count[csg_obj + 1];

/*
**	Default attributes for objects.
*/
extern Attributes	default_attributes;

/* Declarations (attribute). */
extern char	*declarations;

/* Temporary files that need to be removed. */
extern char	**temp_filenames;
extern int	num_temp_files;

/* The ambient light values. */
extern XColor 	ambient_light;

/* Application resources structure. */
extern ScedResources	sced_resources;

/* The list of all key frames. */
extern KeyFrameList key_frames;

/* This is to control debug output. */
extern int debugv;
