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
**	export.c : Functions needed to export a file.  Just the callback really.
**
**	External Function:
**
**	Export_Callback(Widget, XtPointer, XtPointer)
**	The callback function for the export menu selection.
**	Pops up a dialog (with others if necessary) and then calls the appropriate
**	raytracer specific function.
*/

#include <sced.h>
#include <unistd.h>
#include <sys/param.h>
#include <SelFile.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Text.h>

#if ( ! defined(MAXPATHLEN) )
#define MAXPATHLEN 1024
#endif


extern void Target_Callback(Widget, XtPointer, XtPointer);

extern int	Export_Rayshade(FILE*, ScenePtr);
extern int	Export_POVray(FILE*, ScenePtr);
extern int	Export_Renderman(FILE*, char*, ScenePtr, Boolean);
extern int	Export_Radiance(FILE*, char*, ScenePtr, Boolean);
extern int	Export_Genray(FILE*, ScenePtr);
extern int	Export_Genscan(FILE*, ScenePtr);

extern void Print_Raytracer(Raytracer *);

static char		*export_file_name = NULL;


/*	void
**	Export_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**	The callback invoked for the export menu function.
**	Checks for a target, then pops up the export dialog.
*/
void
Export_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	SceneStruct	export_scene;
	FILE		*outfile;
	char		path_name[MAXPATHLEN];

	/* Need to have a target. */
	if ( camera.type == NoTarget )
	{
		Target_Callback(NULL, "Export", NULL);
		return;
	}

	if ( ! export_file_name )
	{
		strcat(path_name, "/");
		if ( io_file_name )
		{
			char	*dot;

			strcpy(path_name, io_file_name);
			dot = strchr(path_name, '.');
			if ( dot )
				*(dot + 1) = '\0';
			else
				strcat(path_name, ".");
		}
		else
		{
			if ( scene_path )
				strcpy(path_name, scene_path);
			else
				getcwd(path_name, MAXPATHLEN);
				strcat(path_name, "*.");
		}
		switch ( camera.type )
		{
			case Genray:
				strcat(path_name, "gen");
				break;
			case Genscan:
				strcat(path_name, "gen");
				break;
			case POVray:
				strcat(path_name, "pov");
				break;
			case Rayshade:
				strcat(path_name, "ray");
				break;
			case Radiance:
				strcat(path_name, "rif");
				break;
			case Renderman:
				strcat(path_name, "rib");
			default:;
		}
	}
	else
		strcpy(path_name, export_file_name);

	outfile = XsraSelFile(main_window.shell, "Export to:", "Export", "Cancel",
							NULL, path_name, "w", NULL, &export_file_name);

	export_scene.camera = camera;
	export_scene.light = NULL;
	export_scene.ambient = ambient_light;
	export_scene.instances = main_window.all_instances;
	if ( outfile )
		Export_File(outfile, export_file_name, &export_scene, FALSE);
}





int
Export_File(FILE *outfile, char *name, ScenePtr scene, Boolean preview)
{
	int	res = 0;

	debug(FUNC_NAME,fprintf(stderr,"Export_File()\n"));
	debug(FUNC_VAL,Print_Raytracer(&(scene->camera.type)));

	switch ( scene->camera.type )
	{
		case Rayshade:
			res = Export_Rayshade(outfile, scene);
			break;
		case POVray:
			res = Export_POVray(outfile, scene);
			break;
		case Renderman:
			res = Export_Renderman(outfile, name, scene, preview);
			break;
		case Radiance:
			res = Export_Radiance(outfile, name, scene, preview);
			break;
		case Genray:
			res = Export_Genray(outfile, scene);
			break;
		case Genscan:
			res = Export_Genscan(outfile, scene);
			break;
		case NoTarget:
			fprintf(stderr,"Ack! scene->camera not defined!\n");
			res = TRUE;
			break;
	}

	fclose(outfile);

	return res;

}


