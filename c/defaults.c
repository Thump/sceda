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
**	defaults.c : Loads a default specification file, if it exists.
*/

#include <sced.h>
#include <load.h>
#include <View.h>


extern char	*rayshade_path;
extern char	*rayshade_options;
extern char	*povray_path;
extern char	*povray_options;
extern char	*radiance_path;
extern char	*radiance_options;
extern char	*renderman_path;
extern char	*renderman_options;
extern char	*genray_path;
extern char	*genray_options;
extern char	*genscan_path;
extern char	*genscan_options;

static void	Load_Defaults(FILE*);
static int	Load_Default_View(Viewport*, char*);
static int	Load_Raytracer_Defs(char**, char**);
static int	Load_Default_Attributes();
static int	Load_Default_Includes();

#define Error fprintf(stderr, "Error in defaults file line %d\n", line_num)
#define Load_Float(f) \
	{ \
		if ((token = yylex()) == INT) \
			f = (double)lex_int; \
		else if (token == FLOAT) \
			f = lex_float; \
		else { \
			Error; \
			return token; \
		} \
	}
#define Load_Vector(v) \
	{ Load_Float((v).x); Load_Float((v).y); Load_Float((v).z); }

void
Load_Defaults_File(char *filename)
{
	char	*home_dir;
	FILE	*defs_file;

	if ( ! filename )
	{
		/* Look for a file called .scenerc in the users home directory. */
		home_dir = getenv("HOME");
		if ( ! home_dir ) return;
		filename = New(char, strlen(home_dir) + 12);
		sprintf(filename, "%s/.scenerc", home_dir);

		if ( ! ( defs_file = fopen(filename, "r") ) )
			return;
	}
	else
	{
		if ( ! ( defs_file = fopen(filename, "r") ) )
		{
			fprintf(stderr, "Couldn't open defaults file %s\n", filename);
			return;
		}
	}

	Load_Defaults(defs_file);

	fclose(defs_file);
}


static void
Load_Defaults(FILE *file)
{
	int	token;

#if FLEX
	if ( yyin ) yyrestart(yyin);
#endif /* FLEX */
	yyin = file;

	token = yylex();
	while ( token != EOF_TOKEN )
	{
		switch ( token )
		{
			case MAINVIEW:
				token = Load_Default_View(&(main_window.viewport),
										  "Main_Default");
				if ( main_window.view_widget && main_window.viewport.scr_width )
					XtVaSetValues(main_window.view_widget,
							XtNdesiredWidth, main_window.viewport.scr_width,
							XtNwidth, main_window.viewport.scr_width,
							XtNdesiredHeight, main_window.viewport.scr_height,
							XtNheight, main_window.viewport.scr_height,
							NULL);
				if ( main_window.view_widget )
					XtVaSetValues(main_window.view_widget,
						XtNmagnification, main_window.viewport.magnify, NULL);

				break;

			case CSGVIEW:
				token = Load_Default_View(&(csg_window.viewport),
										  "CSG_Default");
				break;

			case VIEWPORT:
				if ( ( token = yylex() ) != STRING )
				{
					Error;
					break;
				}
				token = Load_Default_View(NULL, lex_string);
				break;

			case RAYSHADE:
				token = Load_Raytracer_Defs(&rayshade_path, &rayshade_options);
				break;

			case POVRAY:
				token = Load_Raytracer_Defs(&povray_path, &povray_options);
				break;

			case RADIANCE:
				token = Load_Raytracer_Defs(&radiance_path, &radiance_options);
				break;

			case RENDERMAN:
				token = Load_Raytracer_Defs(&renderman_path,
											&renderman_options);
				break;

			case GENRAY:
				token = Load_Raytracer_Defs(&genray_path, &genray_options);
				break;

			case GENSCAN:
				token = Load_Raytracer_Defs(&genscan_path, &genscan_options);
				break;

			case INCLUDES:
				token = Load_Default_Includes();
				break;

			case ATTRIBUTES:
				token = Load_Default_Attributes();
				break;

			case COMPRESS:
				compress_output = TRUE;
				token = yylex();
				break;

			case WIREFRAME:
				if ( ( token = yylex() ) == FULL )
				{
					save_simple_wires = TRUE;
					token = yylex();
				}
				break;

			case SCENEDIR:
				if ((token = yylex()) != STRING)
					Error;
				else
					scene_path = lex_string;
				token = yylex();
				break;

			case TARGET:
				if ( ( token = yylex() ) == POVRAY )
					camera.type = POVray;
				else if ( token == RAYSHADE )
					camera.type = Rayshade;
				else if ( token == RADIANCE )
					camera.type = Radiance;
				else if ( token == RENDERMAN )
					camera.type = Renderman;
				else if ( token == GENRAY )
					camera.type = Genray;
				else
					Error;
				token = yylex();
				break;

			default:
				Error;
				token = yylex();
		}
	}
}


static int
Load_Default_View(Viewport *viewport, char *label)
{
	Viewport	result;
	int			token;
	Boolean		finished = FALSE;

	/* Initialize the result in case of underspecification. */
	Viewport_Init(&result);

	while ( ! finished )
	{
		switch ( token = yylex() )
		{
			case LOOKFROM:
				Load_Vector(result.view_from);
				break;

			case LOOKAT:
				Load_Vector(result.view_at);
				break;

			case LOOKUP:
				Load_Vector(result.view_up);
				break;

			case VIEWDIST:
				Load_Float(result.view_distance);
				break;

			case EYEDIST:
				Load_Float(result.eye_distance);
				break;

			case MAGNIFY:
				if ( (token = yylex()) != INT )
				{
					Error;
					finished = TRUE;
				}
				else
					result.magnify = lex_int;
				break;

			case MODE:
				if ( (token = yylex()) != INT )
				{
					Error;
					finished = TRUE;
				}
				else
					result.draw_mode = (int)lex_int;
				break;

			case SCREEN:
				if ((token = yylex()) != INT )
				{
					Error;
					return token;
				}
				result.scr_width = (Dimension)lex_int;

				if ( (token = yylex()) != INT )
				{
					Error;
					return token;
				}
				result.scr_height = (Dimension)lex_int;
				break;

			default:
				finished = TRUE;
		}
	}

	result.is_default = TRUE;

	Build_Viewport_Transformation(&result);

	if ( viewport ) *viewport = result;

	View_Save(&result, label);

	return token;
}


static int
Load_Raytracer_Defs(char **path, char **options)
{
	int			token;

	if ( (token = yylex()) != STRING )
	{
		Error;
		return token;
	}
	*path = lex_string;

	if ( (token = yylex()) != STRING )
	{
		Error;
		return token;
	}
	*options = lex_string;

	return yylex();
}


static int
Load_Default_Attributes()
{
	int		token;
	double	r, g, b;

	while (TRUE)
	{
		switch ( token = yylex() )
		{
			case COLOUR:
				Load_Float(r);	Load_Float(g);	Load_Float(b);
				default_attributes.colour.red = (short)(r * MAX_UNSIGNED_SHORT);
				default_attributes.colour.green=(short)(g * MAX_UNSIGNED_SHORT);
				default_attributes.colour.blue =(short)(b * MAX_UNSIGNED_SHORT);
				break;

			case DIFFUSE:
				Load_Float(default_attributes.diff_coef);
				break;

			case SPECULAR:
				Load_Float(default_attributes.spec_coef);
				Load_Float(default_attributes.spec_power);
				break;

			case TRANSPARENCY:
				Load_Float(default_attributes.transparency);
				break;

			case REFLECT:
				Load_Float(default_attributes.reflect_coef);
				break;

			case REFRACT:
				Load_Float(default_attributes.refract_index);
				break;

			default: return token;
		}
	}
	return token;
}


static int
Load_Default_Includes()
{
	int	token;

	while ( ( token = yylex() ) == STRING )
	{
		POV_Add_Include(lex_string);
		free(lex_string);
	}

	return token;
}
