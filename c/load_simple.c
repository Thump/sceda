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
**	load_simple.c: Functions for loading simple format files. These files
**					are designed to be written by humans or other programs.
*/

#include <math.h>
#include <sced.h>
#include <base_objects.h>
#include <csg.h>
#include <csg_wire.h>
#include <hash.h>
#include <instance_list.h>
#include <layers.h>
#include <load.h>
#include <View.h>

#define Load_Float(f) \
	{ \
		if ((token = yylex()) == INT) \
			f = (double)lex_int; \
		else if (token == FLOAT) \
			f = lex_float; \
		else \
		{ \
			fprintf(stderr, "Malformed input file line %d: Float expected\n", \
					line_num); \
			return EOF_TOKEN; \
		} \
	}

#define Load_Vector(v) \
	{ Load_Float((v).x); Load_Float((v).y); Load_Float((v).z); }

#define Load_Matrix(m) \
	{ Load_Vector((m).x); Load_Vector((m).y); Load_Vector((m).z); }

static int	Load_Ambient(Boolean);
static int	Load_Simple_Instance(ObjectInstancePtr*, Boolean);
static int	Load_Simple_Object(ObjectInstancePtr);
static int	Load_Simple_CSG();
static int	Load_Simple_CSG_Tree(int, CSGNodePtr, CSGNodePtr*);
static int	Load_Simple_Wireframe();


void
Load_Simple_File(FILE *file, int merge, int token)
{
	Viewport	camera_viewport;
	ObjectInstancePtr	dummy;

	if ( token == VERS )
	{
		if ((token = yylex()) == INT)
			version = (double)lex_int;
		else if (token == FLOAT)
			version = lex_float;
		else
		{
			fprintf(stderr, "Malformed input file line %d\n", line_num);
			return;
		}
		token = yylex();
	}

	while (token != EOF_TOKEN)
	{
		switch (token)
		{
			case VIEWPORT:
				if ( merge )
					token = Load_View(&(main_window.viewport),NULL,TRUE);
				else
				{
					token = Load_View(&(main_window.viewport), NULL, FALSE);
					if ( main_window.view_widget &&
						 main_window.viewport.scr_width )
						XtVaSetValues(main_window.view_widget,
							XtNwidth, main_window.viewport.scr_width,
							XtNdesiredWidth, main_window.viewport.scr_width,
							XtNheight, main_window.viewport.scr_height,
							XtNdesiredHeight, main_window.viewport.scr_height,
							NULL);
					if ( main_window.view_widget )
						XtVaSetValues(main_window.view_widget,
							XtNmagnification, main_window.viewport.magnify,
							NULL);
				}
				break;

			case CAMERA:
				token = Load_View(&camera_viewport, NULL, merge);
				if ( ! merge )
					Viewport_To_Camera(&camera_viewport,
									   main_window.view_widget,
									   &camera, FALSE);
				break;

			case INCLUDES:
				token = Load_Includes(!merge);
				break;

			case DECLARE:
				token = Load_Declarations(!merge);
				break;

			case OBJECT:
				token = Load_Simple_Instance(&dummy, FALSE);
				break;

			case CSG:
				token = Load_Simple_CSG();
				break;

			case WIREFRAME:
				token = Load_Simple_Wireframe();
				break;

			case AMBIENT:
				token = Load_Ambient(!merge);
				break;

			default:
				if ( merge )
					fprintf(stderr, "Error token %d in file %s line %d\n",
							token, merge_filename, line_num);
				else
					fprintf(stderr, "Error token %d in file %s line %d\n",
							token, io_file_name, line_num);
				token = yylex();
		}
	}

	View_Update(&main_window, main_window.all_instances, CalcView);
	Update_Projection_Extents(main_window.all_instances);
}


static int
Load_Colour(XColor *col)
{
	int		token;
	double	r, g, b;

	Load_Float(r);
	Load_Float(g);
	Load_Float(b);

	col->red = (unsigned short)(r * MAX_UNSIGNED_SHORT);
	col->green = (unsigned short)(g * MAX_UNSIGNED_SHORT);
	col->blue = (unsigned short)(b * MAX_UNSIGNED_SHORT);

	return COLOUR;
}

static int
Load_Ambient(Boolean set)
{
	XColor	colour;

	if ( Load_Colour(&colour) == EOF_TOKEN )
		return EOF_TOKEN;

	if ( set )
		ambient_light = colour;

	return yylex();
}

static int
Load_Simple_Instance(ObjectInstancePtr *obj, Boolean csg)
{
	char			*obj_name;
	BaseObjectPtr	base;
	int				token;

	/* Load the name. */
	if ( ( token = yylex() ) != STRING )
	{
		fprintf(stderr, "Malformed input file line %d: Expected object name\n",
				line_num);
		return EOF_TOKEN;
	}
	obj_name = lex_string;

	/* Load the base type. */
	if ( ( token = yylex() ) != STRING )
	{
		fprintf(stderr, "Malformed input file line %d: Expected object type\n",
				line_num);
		return EOF_TOKEN;
	}
	base = Get_Base_Object_From_Label(lex_string);
	free(lex_string);
	if ( ! base )
	{
		fprintf(stderr, "Unable to find base \"%s\", line %d\n",
				lex_string, line_num);
		return EOF_TOKEN;
	}

	/* Create the object. */
	(*obj) = Create_Instance(base, obj_name);
	free((*obj)->o_label);
	(*obj)->o_label = obj_name;
	if ( ! csg )
	{
		InstanceList	new_elmt;

		new_elmt = Append_Element(&(main_window.all_instances), *obj);
		if ( Layer_Is_Visible((*obj)->o_layer) )
			(*obj)->o_flags |= ObjVisible;
	}

	return Load_Simple_Object(*obj);
}


static int
Load_Simple_Object(ObjectInstancePtr obj)
{
	Transformation	transform;
	Matrix			new_matrix;
	Vector			new_vector;
	Attributes		attributes = default_attributes;
	Boolean			done = FALSE;
	int				token;

	NewIdentityMatrix(transform.matrix);
	VNew(0, 0, 0, transform.displacement);
	attributes.defined = FALSE;

	while ( ! done )
	{
		switch ( token = yylex() )
		{
			case MATRIX:
				Load_Matrix(new_matrix);
				transform.matrix = MMMul(&new_matrix, &(transform.matrix));
				break;

			case SCALE:
				Load_Vector(new_vector);
				NewIdentityMatrix(new_matrix);
				new_matrix.x.x = new_vector.x;
				new_matrix.y.y = new_vector.y;
				new_matrix.z.z = new_vector.z;
				transform.matrix = MMMul(&new_matrix, &(transform.matrix));
				break;

			case ROTATE:
				Load_Vector(new_vector);
				Vector_To_Rotation_Matrix(&new_vector, &new_matrix);
				transform.matrix = MMMul(&new_matrix, &(transform.matrix));
				break;

			case POSITION:
				Load_Vector(new_vector);
				VAdd(new_vector, transform.displacement,transform.displacement);
				break;

			case DENSE:
				if ( ( token = yylex() ) != INT )
				{
					fprintf(stderr,
							"Malformed input file line %d: Expected integer\n",
							line_num);
					return EOF_TOKEN;
				}
				Object_Change_Wire_Level(obj, (int)lex_int);
				break;

			case SPECULAR:
				if ( Obj_Is_Light(obj) )
					break;
				attributes.defined = TRUE;
				Load_Float(attributes.spec_coef);
				Load_Float(attributes.spec_power);
				break;

			case DIFFUSE:
				if ( Obj_Is_Light(obj) )
					break;
				attributes.defined = TRUE;
				Load_Float(attributes.diff_coef);
				break;

			case REFLECT:
				if ( Obj_Is_Light(obj) )
					break;
				attributes.defined = TRUE;
				Load_Float(attributes.reflect_coef);
				break;

			case REFRACT:
				if ( Obj_Is_Light(obj) )
					break;
				attributes.defined = TRUE;
				Load_Float(attributes.refract_index);
				break;

			case TRANSPARENCY:
				if ( Obj_Is_Light(obj) )
					break;
				attributes.defined = TRUE;
				Load_Float(attributes.transparency);
				break;

			case COLOUR:
				if ( Obj_Is_Light(obj) )
				{
					Load_Float(((LightInfoPtr)obj->o_attribs)->red);
					Load_Float(((LightInfoPtr)obj->o_attribs)->green);
					Load_Float(((LightInfoPtr)obj->o_attribs)->blue);
				}
				else
				{
					attributes.defined = TRUE;
					if ( Load_Colour(&(attributes.colour)) == EOF_TOKEN )
						return EOF_TOKEN;
				}
				break;

			case EXTEND:
				if ( Obj_Is_Light(obj) )
					break;
				if ( ( token = yylex() ) != STRING )
				{
					fprintf(stderr,
							"Malformed input file line %d: Expected string\n",
							line_num);
					return EOF_TOKEN;
				}
				attributes.defined = TRUE;
				attributes.use_extension = TRUE;
				attributes.extension = lex_string;
				break;

			default: done = TRUE;
		}
	}

	Transform_Instance(obj, &transform, TRUE);
	if ( attributes.defined )
		*(AttributePtr)(obj->o_attribs) = attributes;

	return token;
}


static int
Load_Simple_CSG_Tree(int token, CSGNodePtr parent, CSGNodePtr *ret_tree)
{
	*ret_tree = New(CSGNode, 1);
	(*ret_tree)->csg_parent = parent;

	switch ( token )
	{
		case UNION:
			(*ret_tree)->csg_op = csg_union_op;
			return Load_Simple_CSG_Tree(
						Load_Simple_CSG_Tree(yylex(), *ret_tree,
											 &((*ret_tree)->csg_left_child)),
										*ret_tree,
										&((*ret_tree)->csg_right_child));

		case INTERSECTION:
			(*ret_tree)->csg_op = csg_intersection_op;
			return Load_Simple_CSG_Tree(
						Load_Simple_CSG_Tree(yylex(), *ret_tree,
											 &((*ret_tree)->csg_left_child)),
										*ret_tree,
										&((*ret_tree)->csg_right_child));

		case DIFFERENCE:
			(*ret_tree)->csg_op = csg_difference_op;
			return Load_Simple_CSG_Tree(
						Load_Simple_CSG_Tree(yylex(), *ret_tree,
											 &((*ret_tree)->csg_left_child)),
										*ret_tree,
										&((*ret_tree)->csg_right_child));

		case OBJECT:
			(*ret_tree)->csg_op = csg_leaf_op;
			return Load_Simple_Instance(&((*ret_tree)->csg_instance), TRUE);

		default:
			fprintf(stderr,
					"Malformed input file line %d: Expected CSG tree element\n",
					line_num);
			return EOF_TOKEN;
	}
}

static int
Load_Simple_CSG()
{
	char		*csg_name;
	CSGNodePtr	tree;
	int			token;

	if ( ( token = yylex() ) != STRING )
	{
		fprintf(stderr, "Malformed input file line %d: Expected string\n",
				line_num);
		return EOF_TOKEN;
	}
	csg_name = lex_string;

	if ( ( token = Load_Simple_CSG_Tree(yylex(), NULL, &tree) ) == EOF_TOKEN )
		return token;

	Add_CSG_Base_Object(tree, csg_name, NULL, NULL)->b_ref_num = 0;

	return token;
}

static int
Load_Simple_Wireframe()
{
	char			*wire_name;
	WireframePtr	wireframe;
	int				token;
	int				i, j;
	Vector			v1, v2;
	double			temp_d;

	if ( ( token = yylex() ) != STRING )
	{
		fprintf(stderr, "Malformed input file line %d: Expected string\n",
				line_num);
		return EOF_TOKEN;
	}
	wire_name = lex_string;

	/* Get number of vertices. */
	if ( ( token = yylex() ) != INT )
	{
		fprintf(stderr, "Malformed input file line %d: Expected integer\n",
				line_num);
		return EOF_TOKEN;
	}
	wireframe->num_vertices = (int)lex_int;

	/* Get number of faces. */
	if ( ( token = yylex() ) != INT )
	{
		fprintf(stderr, "Malformed input file line %d: Expected integer\n",
				line_num);
		return EOF_TOKEN;
	}
	wireframe->num_faces = (int)lex_int;

	wireframe->vertices = New(Vector, wireframe->num_vertices);
	for ( i = 0 ; i < wireframe->num_vertices ; i++ )
		Load_Vector(wireframe->vertices[i])

	wireframe->faces = New(Face, wireframe->num_faces);
	for ( i = 0 ; i < wireframe->num_faces ; i++ )
	{
		if ( ( token == yylex() ) != INT )
		{
			fprintf(stderr, "Malformed input file line %d: Expected integer\n",
					line_num);
			return EOF_TOKEN;
		}
		wireframe->faces[i].num_vertices = (int)lex_int;
		for ( j = 0 ; j < wireframe->faces[i].num_vertices ; j++ )
		{
			if ( ( token == yylex() ) != INT )
			{
				fprintf(stderr,
						"Malformed input file line %d: Expected integer\n",
						line_num);
				return EOF_TOKEN;
			}
			wireframe->faces[i].vertices[j] = (int)lex_int;
		}

		if ( wireframe->faces[i].num_vertices > 2 )
		{
			VSub(wireframe->vertices[wireframe->faces[i].vertices[2]],
				 wireframe->vertices[wireframe->faces[i].vertices[0]], v1);
			VSub(wireframe->vertices[wireframe->faces[i].vertices[1]],
				 wireframe->vertices[wireframe->faces[i].vertices[0]], v2);
			VCross(v1, v2, wireframe->faces[i].normal);
			VUnit(wireframe->faces[i].normal, temp_d,
				  wireframe->faces[i].normal);
		}

		wireframe->faces[i].face_attribs = NULL;
	}

	Add_Wireframe_Base_Object(wire_name, wireframe)->b_ref_num = 0;

	return yylex();
}

