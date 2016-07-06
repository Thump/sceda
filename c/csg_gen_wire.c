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
**	csg_gen_wire.c: Generating functions for generic CSG wireframes.
*/

#include <math.h>
#include <config.h>
#if __SCEDCON__
#include <convert.h>
#else
#include <sced.h>
#include <csg.h>
#include <csg_wire.h>
#endif


CSGWireframePtr
CSG_Generic_Wireframe(ObjectInstancePtr src)
{
	CSGWireframePtr	res;
	int				temp_p;
	Matrix			transp;
	Vector			temp, v1, v2;
	double			temp_d;
	int				i, j, k;

	switch ( src->o_parent->b_class )
	{
		case cube_obj:
		case sphere_obj:
		case cylinder_obj:
		case cone_obj:
#if __SCEDCON__
			res = Wireframe_To_CSG(src->o_new_wire, FALSE);
#else
			res = Wireframe_To_CSG(src->o_wireframe, FALSE);
#endif
			break;
		case csg_obj:
#if ! __SCEDCON__
		case wireframe_obj:
#endif
			res = Wireframe_To_CSG(src->o_parent->b_major_wire, FALSE);
			break;
		default:
			fprintf(stderr, "Not supported\n");
			exit(1);
	}

	/* Transform all the points. */
	for ( i = 0 ; i < res->num_vertices ; i++ )
	{
		MVMul(src->o_transform.matrix, res->vertices[i].location, temp);
		VAdd(src->o_transform.displacement, temp, res->vertices[i].location);
	}

#define res_vert(i, j)	res->vertices[res->faces[i].face_vertices[j]].location

	/* Perform operations on faces. */
	MTrans(src->o_inverse.matrix, transp);
	for ( i = 0 ; i < res->num_faces ; i++ )
	{
		/* Transform all the planes. */
		MVMul(transp, res->faces[i].face_plane.f_vector, temp);
		VUnit(temp, temp_d, res->faces[i].face_plane.f_vector);
		res->faces[i].face_plane.f_point = res_vert(i, 0);

		/* Check the ordering of vertices around the normal. Some scaling
		** transformations can reverse the ordering.
		*/
		VSub(res_vert(i, 0), res_vert(i, res->faces[i].face_num_vertices - 1),
			 v1);
		VSub(res_vert(i, 1), res_vert(i, res->faces[i].face_num_vertices - 1),
			 v2);
		VCross(v2, v1, temp);
		if ( VDot(temp, res->faces[i].face_plane.f_vector) < 0 )
		{
			/* Reorder the vertices. */
			for ( j = 0, k = res->faces[i].face_num_vertices - 1 ;
				  j < k ;
				  j++, k-- )
			{
				temp_p = res->faces[i].face_vertices[j];
				res->faces[i].face_vertices[j] = res->faces[i].face_vertices[k];
				res->faces[i].face_vertices[k] = temp_p;
			}
		}

		/* Generate the bounding boxes. */
		CSG_Face_Bounding_Box(res->vertices,
							  res->faces[i].face_vertices,
							  res->faces[i].face_num_vertices,
							  &(res->faces[i].face_extent));

		/* Set face pointers. These are used in exporting attributes to
		** polygon based renderers such as Radiance. Each face must have
		** attributes attached. If the object currently under consideration
		** is not a csg object, then its attributes derive from the object
		** itself. If however it is a csg object, then the attributes derive
		** from the original face if that face had any, or the current object
		** if it didn't.
		*/
#if __SCEDCON__
		if ( src->o_parent->b_class != csg_obj ||
			 ( res->faces[i].face_attribs &&
			   ! ( res->faces[i].face_attribs->defined ) ) )
			res->faces[i].face_attribs = &(src->o_attribs);
#else
		if ( ( src->o_parent->b_class != wireframe_obj &&
			   src->o_parent->b_class != csg_obj ) ||
			 ( res->faces[i].face_attribs &&
			   ! ( res->faces[i].face_attribs->defined ) ) )
			res->faces[i].face_attribs = src->o_attribs;
#endif
	}

#undef res_vert


	/* Generate the overall bounding box. */
	CSG_Bounding_Box(res->vertices, res->num_vertices, &(res->obj_extent));

	return res;
}



CSGWireframePtr
CSG_Copy_Wireframe(CSGWireframePtr src)
{
	CSGWireframePtr	ret = New(CSGWireframe, 1);
	int				i, j;

	ret->num_vertices = src->num_vertices;
	ret->max_vertices = src->num_vertices;
	ret->vertices = New(CSGVertex, ret->num_vertices);
	for ( i = 0 ; i < ret->num_vertices ; i++ )
	{
		ret->vertices[i].location = src->vertices[i].location;
		ret->vertices[i].num_adjacent = ret->vertices[i].max_num_adjacent = 0;
		ret->vertices[i].adjacent = NULL;
		ret->vertices[i].status = vertex_unknown;
	}

	ret->num_faces = src->num_faces;
	ret->faces = New(CSGFace, ret->num_faces);
	for ( i = 0 ; i < ret->num_faces ; i++ )
	{
		ret->faces[i].face_num_vertices = src->faces[i].face_num_vertices;
		ret->faces[i].face_vertices = New(int, ret->faces[i].face_num_vertices);
		for ( j = 0 ; j < ret->faces[i].face_num_vertices ; j++ )
			ret->faces[i].face_vertices[j] = src->faces[i].face_vertices[j];
		ret->faces[i].face_plane = src->faces[i].face_plane;
		ret->faces[i].face_attribs = src->faces[i].face_attribs;
	}

	return ret;
}
