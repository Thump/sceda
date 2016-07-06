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
**	wire_select_box.c : Functions for displaying, modifying and using the
**						select wire object dialog.
*/

/*
**	Imported wireframe files are expected to be in the OFF format.
**	The user selects the header file as the file to load. From this, a
**	geometry file and optional normal and color files are determined.
**	Only ASCII type files are loaded. Binary files can be converted to
**	ASCII format if desired, using the conversion program provided with
**	the off distribution.
*/

#include <math.h>
#include <ctype.h>
#include <sced.h>
#include <load.h>
#include <SelFile.h>
#include <SimpleWire.h>
#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Viewport.h>
#if ( HAVE_STRING_H )
#include <string.h>
#elif ( HAVE_STRINGS_H )
#include <strings.h>
#endif

extern void	Create_New_Object(Widget, XtPointer, XtPointer);

static void			OFF_Load_Header(FILE*);
static WireframePtr	OFF_Load_Wireframe(FILE*);
static void			OFF_Load_Colors(FILE*, WireframePtr);
static void			OFF_Load_Normals(FILE*, WireframePtr);
static void			Wireframe_Select_Callback(Widget, XtPointer, XtPointer);
static void			Create_Wireframe_Select_Shell();
static void			OFF_Save_Header(FILE*, char*, BaseObjectPtr, Boolean);
static void			OFF_Save_Geometry(FILE*, WireframePtr);
static void			OFF_Save_Colours(FILE*, WireframePtr);

static int		wire_select_state;

static Widget	*wire_select_widgets;
static int		num_wire_widgets = 0;
static int		max_num_wire_widgets = 0;

static Widget	wire_select_shell = NULL;
static Widget	wire_select_form;

Widget	wire_delete_button;
extern Widget	new_wire_button;

static char	*wire_file_name = NULL;	/* The header file name. */

static char	*wire_geom_name; /* The geometry file name. */

static char		*wire_pcol_name;	/* The face color file. */
static Boolean	pcol_indexed;	/* Whether the above is indexed. */
static Boolean	pcol_default;

static char		*wire_vnorm_name;	/* Vertex normal lists. */

static Boolean	clockwise;	/* The vertex ordering. */
static AttributePtr	wireframe_attribs;	/* Default attribute information. */

static char		*label;	/* The name of the object. */

static int		off_line_num;

void
Wireframe_Load_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	FILE				*wire_file;
	WireframePtr		wireframe;
	BaseObjectPtr		res;
	char				*temp_s;
	char				*path_name;

	if ( ! wire_file_name )
	{
		if ( ( temp_s = getenv("OBJ_PATH") ) )
		{
			wire_file_name = New(char, strlen(temp_s) + 10);
			strcpy(wire_file_name, temp_s);
			if ( wire_file_name[strlen(temp_s) - 1] == '/' )
				strcat(wire_file_name, "*.aoff");
			else
				strcat(wire_file_name, "/*.aoff");
		}
		else
			wire_file_name = Strdup("*.aoff");
	}

	wire_file = XsraSelFile(main_window.shell, "Wireframe File:", "Load",
				"Cancel", NULL, wire_file_name, "r", NULL, &wire_file_name);

	if ( ! wire_file )
		return;

	/* Determine all the parameters from the header file. */
	wire_geom_name =
	wire_pcol_name =
	wire_vnorm_name = NULL;
	wireframe_attribs = NULL;
	clockwise = TRUE;
	OFF_Load_Header(wire_file);
	fclose(wire_file);

	if ( ! wire_geom_name )
		return;

	/* Figure out the label. */
	if ( ! label )
	{
		temp_s = strrchr(wire_file_name, '/');
		if ( ! temp_s )
			temp_s = wire_file_name;
		else
			temp_s++;
		label = Strdup(temp_s);
		temp_s = strrchr(label, '.');
		if ( temp_s )	*temp_s ='\0';
	}

	/* Get a pathname. */
	path_name = Strdup(wire_file_name);
	temp_s = strrchr(path_name, '/');
	if ( ! temp_s )
		path_name[0] = '\0';
	else
		*(temp_s + 1) = '\0';

	/* Open the geom file. */
	temp_s = New(char, strlen(path_name) + strlen(wire_geom_name) + 5);
	strcpy(temp_s, path_name);
	strcat(temp_s, wire_geom_name);
	free(wire_geom_name);
	wire_geom_name = temp_s;
	if ( ( wire_file = fopen(wire_geom_name, "r") ) == NULL )
	{
		fprintf(stderr, "Unable to open geometry file: %s\n", wire_geom_name);
		free(path_name);
		free(wire_geom_name);
		if ( wire_pcol_name )	free(wire_pcol_name);
		if ( wire_vnorm_name )	free(wire_vnorm_name);
		return;
	}

	/* Load the wireframe. */
	wireframe = OFF_Load_Wireframe(wire_file);
	fclose(wire_file);
	
	if ( ! wireframe )
		return;

	if ( wire_pcol_name )
	{
		/* Open the polygon colors file. */
		temp_s = New(char, strlen(path_name) + strlen(wire_pcol_name) + 5);
		strcpy(temp_s, path_name);
		strcat(temp_s, wire_pcol_name);
		free(wire_pcol_name);
		wire_pcol_name = temp_s;
		if ( ( wire_file = fopen(wire_pcol_name, "r") ) == NULL )
		{
			fprintf(stderr, "Unable to open colors file: %s\n", wire_pcol_name);
			free(path_name);
			free(wire_geom_name);
			if ( wire_pcol_name )	free(wire_pcol_name);
			if ( wire_vnorm_name )	free(wire_vnorm_name);
			return;
		}

		OFF_Load_Colors(wire_file, wireframe);
		fclose(wire_file);
	}
	else
	{
		if ( wireframe_attribs )
		{
			wireframe->attribs = &wireframe_attribs;
			wireframe->num_attribs = 1;
		}
		else
		{
			wireframe->attribs = NULL;
			wireframe->num_attribs = 0;
		}
	}

	if ( wire_vnorm_name )
	{
		/* Open the vertex normal file. */
		temp_s = New(char, strlen(path_name) + strlen(wire_vnorm_name) + 5);
		strcpy(temp_s, path_name);
		strcat(temp_s, wire_vnorm_name);
		free(wire_vnorm_name);
		wire_vnorm_name = temp_s;
		if ( ( wire_file = fopen(wire_vnorm_name, "r") ) == NULL )
		{
			fprintf(stderr, "Unable to open normal file: %s\n",wire_vnorm_name);
			free(path_name);
			free(wire_geom_name);
			if ( wire_pcol_name )	free(wire_pcol_name);
			if ( wire_vnorm_name )	free(wire_vnorm_name);
			wireframe->vertex_normals = NULL;
			return;
		}

		OFF_Load_Normals(wire_file, wireframe);
		fclose(wire_file);
	}
	else
		wireframe->vertex_normals = NULL;

	/* Create the base object. */
	res = Add_Wireframe_Base_Object(label, wireframe);

	/* Get a default reference point. */
	Select_Base_Reference(res);

	/* Add the selection option. */
	Wireframe_Add_Select_Option(res);

	free(label);
	fclose(wire_file);
}


static char*
OFF_Read_Word(FILE *file)
{
	static char	word[128];
	int			i = 0;
	int			ch;

	while ( ( ch = fgetc(file) ) != EOF && ( isspace(ch) || ch == '#' ) )
	{
		if ( ch == '#' )
			while ( ( ch = fgetc(file) ) != EOF && ch != '\n' );
		if ( ch == '\n' )
			off_line_num++;
	}
	if ( ch == EOF )
		return NULL;


	word[i++] = (char)ch;
	while ( ( ch = fgetc(file) ) != EOF && ! isspace(ch) && ch != '#' )
	{
		word[i++] = (char)ch;
		if ( i == 128 )
			return word;
	}
	if ( ch == '\n')
		off_line_num++;
	word[i++] = '\0';

	return word;
}

static void
OFF_Load_Header(FILE *header_file)
{
	char	*word;
	double	temp_d;

#define Input_Error \
		fprintf(stderr, "Malformed off header line %d\n", off_line_num)

	off_line_num = 1;
	word = OFF_Read_Word(header_file);
	while ( word )
	{
		/* Below are the only options I am interested in. */
		/* Others will be ignored. */
		if ( ! strcmp(word, "name") )
		{
			/* Read the name. */
			word = OFF_Read_Word(header_file);
			if ( ! word )
			{
				Input_Error;
				return;
			}
			label = Strdup(word);
		}
		else if ( ! strcmp(word, "type") )
		{
			/* Check that it's a polygon. */
			word = OFF_Read_Word(header_file);
			if ( ! word || strcmp(word, "polygon") )
			{
				fprintf(stderr, "OFF file must be polygon format\n");
				return;
			}
		}
		else if ( ! strcmp(word, "geometry") )
		{
			word = OFF_Read_Word(header_file);
			if ( ! word || strcmp(word, "indexed_poly") )
			{
				Input_Error;
				return;
			}
			word = OFF_Read_Word(header_file);
			if ( ! word || strcmp(word, "fff") )
			{
				Input_Error;
				return;
			}
			word = OFF_Read_Word(header_file);
			if ( ! word )
			{
				Input_Error;
				return;
			}
			else
				wire_geom_name = Strdup(word);
		}
		else if ( ! strcmp(word, "polygon_colors") )
		{
			word = OFF_Read_Word(header_file);
			if ( ! word )
			{
				Input_Error;
				return;
			}
			pcol_indexed = ! strcmp(word, "indexed");
			pcol_default = ! strcmp(word, "default");
			word = OFF_Read_Word(header_file);
			if ( ! word || strcmp(word, "fff") )
			{
				Input_Error;
				return;
			}
			if ( pcol_default )
			{
				/* Read a default colour. */
				if ( ! wireframe_attribs )
				{
					wireframe_attribs = New(Attributes, 1);
					*wireframe_attribs = default_attributes;
				}
				if ( ! ( word = OFF_Read_Word(header_file) ) )
				{
					Input_Error;
					return;
				}
				temp_d = atof(word);
				wireframe_attribs->colour.red = MAX_UNSIGNED_SHORT * temp_d;
				if ( ! ( word = OFF_Read_Word(header_file) ) )
				{
					Input_Error;
					return;
				}
				temp_d = atof(word);
				wireframe_attribs->colour.green = MAX_UNSIGNED_SHORT * temp_d;
				if ( ! ( word = OFF_Read_Word(header_file) ) )
				{
					Input_Error;
					return;
				}
				temp_d = atof(word);
				wireframe_attribs->colour.blue = MAX_UNSIGNED_SHORT * temp_d;
			}
			else
			{
				word = OFF_Read_Word(header_file);
				if ( ! word )
				{
					Input_Error;
					return;
				}
				else
					wire_pcol_name = Strdup(word);
			}
		}
		else if ( ! strcmp(word, "vertex_normals") )
		{
			if ( ! ( word = OFF_Read_Word(header_file) ) )
			{
				Input_Error;
				return;
			}
			if ( ! ( word = OFF_Read_Word(header_file) ) || strcmp(word, "fff"))
			{
				Input_Error;
				return;
			}
			if ( ! ( word = OFF_Read_Word(header_file) ) )
			{
				Input_Error;
				return;
			}
			else
				wire_vnorm_name = Strdup(word);
		}
		else if ( ! strcmp(word, "vertex_order") )
		{
			word = OFF_Read_Word(header_file);
			if ( ! ( word = OFF_Read_Word(header_file) ) ||
				 ! ( word = OFF_Read_Word(header_file) ) )
			{
				Input_Error;
				return;
			}
			clockwise = ! strcmp(word, "clockwise");
		}
		else if ( ! strcmp(word, "diffuse_coef") )
		{
			if ( ! wireframe_attribs )
			{
				wireframe_attribs = (void*)New(Attributes, 1);
				*wireframe_attribs = default_attributes;
			}
			/* Read 2 words, default and f. */
			if ( ! ( word = OFF_Read_Word(header_file) ) ||
				 strcmp(word, "default") ||
				 ! ( word = OFF_Read_Word(header_file) ) ||
				 strcmp(word, "f") )
			{
				Input_Error;
				return;
			}
			if ( ! ( word = OFF_Read_Word(header_file) ) )
			{
				Input_Error;
				return;
			}
			wireframe_attribs->diff_coef =atof(word);
		}
		else if ( ! strcmp(word, "specular_coef") )
		{
			if ( ! wireframe_attribs )
			{
				wireframe_attribs = (void*)New(Attributes, 1);
				*wireframe_attribs = default_attributes;
			}
			/* Read 2 words, default and f. */
			if ( ! ( word = OFF_Read_Word(header_file) ) ||
				 strcmp(word, "default") ||
				 ! ( word = OFF_Read_Word(header_file) ) ||
				 strcmp(word, "f") )
			{
				Input_Error;
				return;
			}
			if ( ! ( word = OFF_Read_Word(header_file) ) )
			{
				Input_Error;
				return;
			}
			wireframe_attribs->spec_coef =atof(word);
		}
		else if ( ! strcmp(word, "specular_power") )
		{
			if ( ! wireframe_attribs )
			{
				wireframe_attribs = (void*)New(Attributes, 1);
				*wireframe_attribs = default_attributes;
			}
			/* Read 2 words, default and f. */
			if ( ! ( word = OFF_Read_Word(header_file) ) ||
				 strcmp(word, "default") ||
				 ! ( word = OFF_Read_Word(header_file) ) ||
				 strcmp(word, "f") )
			{
				Input_Error;
				return;
			}
			if ( ! ( word = OFF_Read_Word(header_file) ) )
			{
				Input_Error;
				return;
			}
			wireframe_attribs->spec_power=atof(word);
		}
		word = OFF_Read_Word(header_file);
	}

#undef Input_Error

}


static WireframePtr
OFF_Load_Wireframe(FILE *file)
{
	WireframePtr	result = New(Wireframe, 1);
	int				token;
	int				i, j, k, temp_i, prev;
	int				num_verts;
	Vector			v1, v2;
	double			temp_d;

#define Input_Error \
		fprintf(stderr, "Malformed wireframe file line %d\n", line_num)

#define Load_Float(f) \
	{ \
		if ((token = yylex()) == INT) \
			f = (double)lex_int; \
		else if (token == FLOAT) \
			f = lex_float; \
		else { \
			Input_Error; \
			free(result); \
			return NULL; \
		} \
	}

#define Load_Int(val) \
	if ( ( token = yylex() ) != INT ) \
	{ \
		Input_Error; \
		free(result); \
		return NULL; \
	} \
	val = (int)lex_int;

#define Load_Vector(v) \
	{ Load_Float((v).x); Load_Float((v).y); Load_Float((v).z); }

#if FLEX
	if ( yyin ) yyrestart(yyin);
#endif /* FLEX */
	yyin = file;
	line_num = 1;

	if ( ( token = yylex() ) != INT )
	{
		Input_Error;
		free(result);
		return NULL;
	}
	result->num_vertices = (int)lex_int + 1;

	if ( ( token = yylex() ) != INT )
	{
		Input_Error;
		free(result);
		return NULL;
	}
	result->num_faces = (int)lex_int;

	if ( ( token = yylex() ) != INT )
	{
		Input_Error;
		free(result);
		return NULL;
	}

	result->vertices = New(Vector, result->num_vertices);
	result->faces = New(Face, result->num_faces);

	for ( i = 0 ; i < result->num_vertices - 1 ; i++ )
		Load_Vector(result->vertices[i]);
	/* The center vertex. */
	VNew(0, 0, 0, result->vertices[i]);

	for ( i = 0 ; i < result->num_faces ; i++ )
	{
		Load_Int(result->faces[i].num_vertices);
		result->faces[i].vertices = New(int, result->faces[i].num_vertices);
		prev = -1;
		num_verts = 0;
		for ( j = 0 ; j < result->faces[i].num_vertices ; j++ )
		{
			Load_Int(temp_i);
			if ( ( prev != -1 && temp_i == prev ) ||
				 ( j == result->faces[i].num_vertices - 1 &&
				   temp_i == result->faces[i].vertices[0] + 1 ) )
				continue;
			result->faces[i].vertices[num_verts++] = temp_i - 1;
			prev = temp_i;
		}
		result->faces[i].num_vertices = num_verts;

		if ( ! clockwise )
			/* Reverse the vertex order. */
			for ( j = 0, k = num_verts - 1 ; j < k ; j++, k-- )
			{
				temp_i = result->faces[i].vertices[j];
				result->faces[i].vertices[j] = result->faces[i].vertices[k];
				result->faces[i].vertices[k] = temp_i;
			}

		result->faces[i].face_attribs = wireframe_attribs;

		if ( num_verts < 3 )
			continue;

		/* Calculate the face normal. */
		VSub(result->vertices[result->faces[i].vertices[0]],
			 result->vertices[result->faces[i].vertices[1]], v1);
		VSub(result->vertices[result->faces[i].vertices[2]],
			 result->vertices[result->faces[i].vertices[1]], v2);
		VCross(v1, v2, result->faces[i].normal);
		VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);
	}

	return result;

#undef Input_Error
#undef Load_Float
#undef Load_Vector

}


static void
OFF_Load_Colors(FILE *file, WireframePtr wireframe)
{
	int		num_indices, num_data;
	Vector	color;
	int		index;
	int		i;

#define Input_Error { \
				fprintf(stderr, "Malformed color file\n"); \
				return; \
				}

#define Vect_To_Col(v, c) \
				(c).red = MAX_UNSIGNED_SHORT * (v).x; \
				(c).green = MAX_UNSIGNED_SHORT * (v).y; \
				(c).blue = MAX_UNSIGNED_SHORT * (v).z;

#define Load_Vector(v) \
	if ( fscanf(file, "%lf %lf %lf", &((v).x), &((v).y), &((v).z)) != 3 ) \
		Input_Error


	if ( pcol_indexed )
	{
		if ( fscanf(file, "%d", &num_data) != 1 )
			Input_Error
	}

	if ( fscanf(file, "%d", &num_indices) != 1 )
		Input_Error

	if ( ! pcol_indexed )
		num_data = num_indices;

	/* Allocate an array for the attributes. */
	if ( wireframe_attribs )
	{
		wireframe->attribs = New(AttributePtr, num_data + 1);
		wireframe->num_attribs = num_data + 1;
		wireframe->attribs[num_data] = wireframe_attribs;
	}
	else
	{
		wireframe->attribs = New(AttributePtr, num_data);
		wireframe->num_attribs = num_data;
	}

	if ( pcol_indexed )
	{
		for ( i = 0 ; i < num_data ; i++ )
		{
			wireframe->attribs[i] = New(Attributes, 1);
			*(wireframe->attribs[i]) = wireframe_attribs ?
				*wireframe_attribs : default_attributes;
			Load_Vector(color);
			Vect_To_Col(color, wireframe->attribs[i]->colour);
		}
		for ( i = 0 ; i < num_indices ; i++ )
		{
			if ( fscanf(file, "%d", &index) != 1 )
				Input_Error
			wireframe->faces[i].face_attribs = wireframe->attribs[index - 1];
		}
	}
	else
	{
		for ( i = 0 ; i < num_indices ; i++ )
		{
			Load_Vector(color);
			wireframe->attribs[i] =
			wireframe->faces[i].face_attribs = New(Attributes, 1);
			*(wireframe->faces[i].face_attribs) = wireframe_attribs ?
				*wireframe_attribs : default_attributes;
			Vect_To_Col(color, wireframe->faces[i].face_attribs->colour);
		}
	}

#undef Load_Vector
#undef Input_Error
#undef Vect_To_Col
}


static void
OFF_Load_Normals(FILE *file, WireframePtr wire)
{
	int	num_indices;
	int	i;

#define Input_Error { \
				fprintf(stderr, "Malformed color file\n"); \
				return; \
				}

#define Load_Vector(v) \
	if ( fscanf(file, "%lf %lf %lf", &((v).x), &((v).y), &((v).z)) != 3 ) \
		Input_Error

	if ( fscanf(file, "%d", &num_indices) != 1 )
		Input_Error;

	if ( num_indices != wire->num_vertices - 1 )
	{
	  fprintf(stderr, "Wireframe error: Incorrect number of vertex normals.\n");
	  return;
	}

	wire->vertex_normals = New(Vector, num_indices + 1);

	for ( i = 0 ; i < num_indices - 1 ; i++ )
		Load_Vector(wire->vertex_normals[i]);
	VNew(1, 0, 0, wire->vertex_normals[i]);
}


void
Wireframe_Delete_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	Wireframe_Select_Popup(WIRE_DELETE);
}


void
Wireframe_Select_Popup(int state)
{
	wire_select_state = state;

	if ( ! num_wire_widgets ) return;

	SFpositionWidget(wire_select_shell);
	XtPopup(wire_select_shell, XtGrabExclusive);
}

void
Wireframe_Add_Select_Option(BaseObjectPtr base)
{
	Dimension	form_width, form_height;
	int			gap;
	Arg			args[10];
	int			n;

	if ( ! wire_select_shell )
		Create_Wireframe_Select_Shell();

	if ( num_wire_widgets == max_num_wire_widgets )
	{
		if ( max_num_wire_widgets )
			wire_select_widgets = More(wire_select_widgets, Widget,
									  max_num_wire_widgets + 5);
		else
			wire_select_widgets = New(Widget, max_num_wire_widgets + 5);
		max_num_wire_widgets += 5;
	}

	XtVaGetValues(wire_select_form, XtNwidth, &form_width,
								   XtNheight, &form_height,
								   XtNdefaultDistance, &gap,
								   NULL);

	n = 0;
	XtSetArg(args[n], XtNbasePtr, base);	n++;
	XtSetArg(args[n], XtNwidth, 118);		n++;
	XtSetArg(args[n], XtNheight, 118);		n++;
	wire_select_widgets[num_wire_widgets] =
		XtCreateManagedWidget("wireNewObject", simpleWireWidgetClass,
							  wire_select_form, args, n);
	XtAddCallback(wire_select_widgets[num_wire_widgets], XtNcallback,
				  Wireframe_Select_Callback, NULL);
	num_wire_widgets++;

}


static void
Wireframe_Select_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(wire_select_shell);

	switch ( wire_select_state )
	{
		case WIRE_CREATE:
			Create_New_Object(w, NULL, NULL);
			break;
		case WIRE_DELETE:
			Wireframe_Destroy_Base_Object(w, NULL);
			break;
	}
}

static void
Wireframe_Select_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(wire_select_shell);
}


void
Wireframe_Select_Destroy_Widget(int index)
{
	int	i;

	/* Destroy it's widget. */
	XtDestroyWidget(wire_select_widgets[index]);
	for ( i = index + 1 ; i < num_wire_widgets ; i++ )
		wire_select_widgets[i-1] = wire_select_widgets[i];
	num_wire_widgets--;
}


static void
Create_Wireframe_Select_Shell()
{
	Widget	outer_form;
	Widget	cancel_button;
	Widget	viewport_widget;
	String	shell_geometry;
	unsigned	shell_width, shell_height;
	int		junk;
	int		gap;
	Arg		args[15];
	int		n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Wireframe Select");	n++;
	wire_select_shell = XtCreatePopupShell("wireSelectShell",
						transientShellWidgetClass, main_window.shell, args, n);

	XtRealizeWidget(wire_select_shell);

	/* Get the parents size. */
	XtVaGetValues(wire_select_shell, XtNgeometry, &shell_geometry, NULL);
	XParseGeometry(shell_geometry, &junk, &junk, &shell_width, &shell_height);

	/* Create the first level form. */
	n = 0;
	XtSetArg(args[n], XtNwidth, (int)shell_width);		n++;
	XtSetArg(args[n], XtNheight, (int)shell_height);	n++;
	outer_form = XtCreateManagedWidget("wireSelectOuterForm", formWidgetClass,
										wire_select_shell, args, n);
	XtVaGetValues(outer_form, XtNdefaultDistance, &gap, NULL);

	/* Create the viewport. */
	n = 0;
	XtSetArg(args[n], XtNwidth, (int)(shell_width - 2 * gap));		n++;
	XtSetArg(args[n], XtNheight, (int)(shell_height - 35 - 3*gap));	n++;
	XtSetArg(args[n], XtNtop, XtChainTop);			n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainRight);		n++;
	XtSetArg(args[n], XtNallowHoriz, TRUE);			n++;
	XtSetArg(args[n], XtNallowVert, TRUE);			n++;
	XtSetArg(args[n], XtNforceBars, TRUE);			n++;
	XtSetArg(args[n], XtNuseBottom, TRUE);			n++;
	XtSetArg(args[n], XtNuseRight, TRUE);			n++;
	viewport_widget = XtCreateManagedWidget("wireSelectViewport",
						viewportWidgetClass, outer_form, args, n);

	/* Create the box inside the viewport. */
	n = 0;
	XtSetArg(args[n], XtNwidth, (int)(shell_width - 2 * gap - 40));		n++;
	XtSetArg(args[n], XtNheight, (int)(shell_height - 35 - 3*gap - 40));n++;
	wire_select_form = XtCreateManagedWidget("wireSelectForm", boxWidgetClass,
											viewport_widget, args, n);

	/* The cancel button. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Cancel");			n++;
	XtSetArg(args[n], XtNfromVert, viewport_widget);	n++;
	XtSetArg(args[n], XtNleft, XtChainLeft);		n++;
	XtSetArg(args[n], XtNright, XtChainLeft);		n++;
	XtSetArg(args[n], XtNtop, XtChainBottom);		n++;
	XtSetArg(args[n], XtNbottom, XtChainBottom);	n++;
	XtSetArg(args[n], XtNresizable, TRUE);			n++;
	cancel_button = XtCreateManagedWidget("wireSelectCancel",
						commandWidgetClass, outer_form, args, n);
	XtAddCallback(cancel_button, XtNcallback, Wireframe_Select_Cancel_Callback,
					NULL);
}


void
OFF_Save_Wireframe(Widget w, BaseObjectPtr base)
{
	char	*dir_name;
	char	*file_name;
	char	*base_name;
	char	*temp_s;
	FILE	*header_file;
	FILE	*geom_file;
	FILE	*pcol_file;

	if ( w && ! base )
		XtVaGetValues(w, XtNbasePtr, &base, NULL);

	/* Get the target directory. */
	if ( ( dir_name = getenv("OBJ_PATH") ) )
	{
		dir_name = Strdup(dir_name);
		if ( dir_name[strlen(dir_name) - 1] != '/' )
		{
			dir_name = More(dir_name, char, strlen(dir_name) + 4);
			dir_name[strlen(dir_name)] = '/';
		}
	}
	else
		dir_name = Strdup("");

	/* Build the probable name. */
	file_name = New(char, strlen(dir_name) + strlen(base->b_label) + 10);
	strcpy(file_name, dir_name);
	strcat(file_name, base->b_label);
	strcat(file_name, ".aoff");
	free(dir_name);

	/* Check it. */
	header_file = XsraSelFile(main_window.shell, "Wireframe File:", "Save",
					"Cancel", NULL, file_name, "w", NULL, &file_name);
	if ( ! header_file )
		return;

	/* Infer the dir and base name. */
	dir_name = Strdup(file_name);
	if ( ( temp_s = strrchr(dir_name, '/') ) )
	{
		base_name = Strdup(temp_s + 1);
		temp_s[1] = '\0';
	}
	else
	{
		dir_name[0] = '\0';
		base_name = Strdup(file_name);
	}
	if ( ( temp_s = strrchr(base_name, '.') ) )
		temp_s[0] = '\0';
	free(file_name);

	/* Open geometry and poly colour files. */
	file_name = New(char, strlen(dir_name) + strlen(base_name) + 10);
	strcpy(file_name, dir_name);
	strcat(file_name, base_name);
	strcat(file_name, ".geom");
	if ( ! ( geom_file = fopen(file_name, "w") ) )
	{
		char	*error_string = New(char, strlen(file_name) + 64);
		sprintf(error_string, "Unable to open file: %s", file_name);
		Popup_Error(error_string, main_window.shell, "Error");
		free(error_string);
		return;
	}

	if ( Wireframe_Has_Attributes(base->b_major_wire) )
	{
		strcpy(file_name, dir_name);
		strcat(file_name, base_name);
		strcat(file_name, ".pcol");
		if ( ! ( pcol_file = fopen(file_name, "w") ) )
		{
			char	*error_string = New(char, strlen(file_name) + 64);
			sprintf(error_string, "Unable to open file: %s", file_name);
			Popup_Error(error_string, main_window.shell, "Error");
			free(error_string);
			return;
		}
	}

	OFF_Save_Header(header_file, base_name, base, pcol_file ? TRUE : FALSE);
	OFF_Save_Geometry(geom_file, base->b_major_wire);
	if ( pcol_file )
		OFF_Save_Colours(pcol_file, base->b_major_wire);

	free(file_name);
	free(dir_name);
	free(base_name);

	fclose(header_file);
	fclose(geom_file);
	if ( pcol_file )
		fclose(pcol_file);

	return;
}


static void
OFF_Save_Header(FILE *file, char *base_name, BaseObjectPtr base, Boolean colour)
{
	fprintf(file, "name\t\t%s\n", base_name);
	fprintf(file, "author\t\tSced CSG Object. Sced version "VERSION"\n");
	fprintf(file, "type\t\tpolygon\n");
	fprintf(file, "geometry\t\tindexed_poly\tfff\t%s.geom\n", base_name);
	if ( colour )
		fprintf(file, "polygon_colors\tgeneric\t\tfff\t%s.pcol\n", base_name);
	fprintf(file, "vertex_order\tdefault\t\ts\tclockwise\n");
}


static void
OFF_Save_Geometry(FILE *file, WireframePtr wire)
{
	int	i, j;

	fprintf(file, "%d %d %d\n", wire->num_vertices, wire->num_faces,
			Wireframe_Count_Edges(wire));
	for ( i = 0 ; i < wire->num_vertices ; i++ )
		VPrint(file, wire->vertices[i]);
	for ( i = 0 ; i < wire->num_faces ; i++ )
	{
		fprintf(file, "%d", wire->faces[i].num_vertices);
		for ( j = 0 ; j < wire->faces[i].num_vertices ; j++ )
			fprintf(file, " %d", wire->faces[i].vertices[j] + 1);
		fprintf(file, "\n");
	}
}


static void
OFF_Save_Colours(FILE *file, WireframePtr wire)
{
	int	i;

	fprintf(file, "%d\n", wire->num_faces);
	for ( i = 0 ; i < wire->num_faces ; i++ )
		if ( wire->faces[i].face_attribs &&
			 wire->faces[i].face_attribs->defined )
			fprintf(file, "%5.3g %5.3g %5.3g\n",
					wire->faces[i].face_attribs->colour.red /
					(double)MAX_UNSIGNED_SHORT,
					wire->faces[i].face_attribs->colour.green /
					(double)MAX_UNSIGNED_SHORT,
					wire->faces[i].face_attribs->colour.blue /
					(double)MAX_UNSIGNED_SHORT);
		else
			fprintf(file, "%5.3g %5.3g %5.3g\n", 
					default_attributes.colour.red / (double)MAX_UNSIGNED_SHORT,
					default_attributes.colour.green /(double)MAX_UNSIGNED_SHORT,
					default_attributes.colour.blue /(double)MAX_UNSIGNED_SHORT);

}

void
Set_Wireframe_Related_Sensitivity(Boolean state)
{
	XtSetSensitive(wire_delete_button, state);
	if ( new_wire_button )
		XtSetSensitive(new_wire_button, state);
}


