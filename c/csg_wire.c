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

#include <config.h>
#if __SCEDCON__
#include <convert.h>
#else
#include <sced.h>
#include <csg.h>
#include <csg_wire.h>
#endif


static CSGWireframe	*Create_CSG_Wireframe(CSGNodePtr);


CSGWireframePtr
CSG_Generate_CSG_Wireframe(CSGNodePtr src)
{
	return Create_CSG_Wireframe(src);
}


/*	CSGWireframe*
**	Create_CSG_Wireframe(CSGNodePtr src)
**	Initialises all the wireframes at each node of the tree.
**	For internal nodes, sets it to the concatenation of the children.
*/
static CSGWireframe*
Create_CSG_Wireframe(CSGNodePtr src)
{
	CSGWireframePtr	left_wire;
	CSGWireframePtr	right_wire;
	CSGWireframePtr	result;

	if ( ! src ) return NULL;

	if ( src->csg_op == csg_leaf_op )
		return CSG_Generic_Wireframe(src->csg_instance);

	left_wire = Create_CSG_Wireframe(src->csg_left_child);
	right_wire = Create_CSG_Wireframe(src->csg_right_child);

	result = CSG_Combine_Wireframes(left_wire, right_wire, src->csg_op);

	CSG_Destroy_Wireframe(left_wire);
	CSG_Destroy_Wireframe(right_wire);

	return result;
}


void
CSG_Destroy_Wireframe(CSGWireframePtr victim)
{
	int	i;

	/* Free adjacency lists. */
	for ( i = 0 ; i < victim->num_vertices ; i++ )
		if ( victim->vertices[i].num_adjacent )
			free(victim->vertices[i].adjacent);

	/* Free face vertex lists. */
	for ( i = 0 ; i < victim->num_faces ; i++ )
		free(victim->faces[i].face_vertices);

	/* Free the vertices. */
	free(victim->vertices);
	/* Free the faces. */
	free(victim->faces);
	/* Free the wireframe. */
	free(victim);
}


/*	void
**	CSG_Bounding_Box(CSGVertexPtr verts, short num, Cuboid *ret)
**	Determines the extent for the given vertices.
*/
void
CSG_Bounding_Box(CSGVertexPtr verts, short num, Cuboid *ret)
{
	int	i;

	if ( num == 0 )
	{
		VNew(0, 0, 0, ret->min);
		ret->max = ret->min;
		return;
	}

	VNew(verts[0].location.x,verts[0].location.y,verts[0].location.z, ret->min);
	ret->max = ret->min;
	for ( i = 1 ; i < num ; i++ )
	{
		if ( verts[i].location.x > ret->max.x )
			ret->max.x = verts[i].location.x;
		else if ( verts[i].location.x < ret->min.x )
			ret->min.x = verts[i].location.x;
		if ( verts[i].location.y > ret->max.y )
			ret->max.y = verts[i].location.y;
		else if ( verts[i].location.y < ret->min.y )
			ret->min.y = verts[i].location.y;
		if ( verts[i].location.z > ret->max.z )
			ret->max.z = verts[i].location.z;
		else if ( verts[i].location.z < ret->min.z )
			ret->min.z = verts[i].location.z;
	}
}

/*	void
**	CSG_Face_Bounding_Box(CSGVertexPtr *verts, short num, Cuboid *ret)
**	Determines the extent for the given vertex pointer list.
*/
void
CSG_Face_Bounding_Box(CSGVertexPtr verts, int *indices, short num, Cuboid *ret)
{
	int	i;

	VNew(verts[indices[0]].location.x, verts[indices[0]].location.y,
		 verts[indices[0]].location.z, ret->min);
	ret->max = ret->min;
	for ( i = 1 ; i < num ; i++ )
	{
		if ( verts[indices[i]].location.x > ret->max.x )
			ret->max.x = verts[indices[i]].location.x;
		else if ( verts[indices[i]].location.x < ret->min.x )
			ret->min.x = verts[indices[i]].location.x;
		if ( verts[indices[i]].location.y > ret->max.y )
			ret->max.y = verts[indices[i]].location.y;
		else if ( verts[indices[i]].location.y < ret->min.y )
			ret->min.y = verts[indices[i]].location.y;
		if ( verts[indices[i]].location.z > ret->max.z )
			ret->max.z = verts[indices[i]].location.z;
		else if ( verts[indices[i]].location.z < ret->min.z )
			ret->min.z = verts[indices[i]].location.z;
	}
}



static void
CSG_Append_Wireframe(WireframePtr dest, WireframePtr src,
					 Transformation *transform, Transformation *inverse,
					 AttributePtr attribs)
{
	int		i, j, k;
	Matrix	transp;

	dest->vertices = More(dest->vertices, Vector,
						  dest->num_vertices + src->num_vertices);
	for ( i = 0, j = dest->num_vertices ; i < src->num_vertices ; i++, j++)
	{
		MVMul(transform->matrix, src->vertices[i], dest->vertices[j]);
		VAdd(dest->vertices[j], transform->displacement, dest->vertices[j]);
	}

	dest->faces =
		More(dest->faces, Face, dest->num_faces + src->num_faces);
	MTrans(inverse->matrix, transp);
	for ( i = 0, j = dest->num_faces ; i < src->num_faces ; i++, j++ )
	{
		dest->faces[j].num_vertices = src->faces[i].num_vertices;
		dest->faces[j].vertices = New(int, dest->faces[j].num_vertices);
		for ( k = 0 ; k < dest->faces[j].num_vertices ; k++ )
			dest->faces[j].vertices[k] =
				src->faces[i].vertices[k] + dest->num_vertices;
		MVMul(transp, src->faces[i].normal, dest->faces[j].normal);
		dest->faces[j].face_attribs = attribs;
	}

	dest->num_vertices += src->num_vertices;
	dest->num_faces += src->num_faces;
}


static void
CSG_Build_Full_Wireframe(CSGNodePtr tree, WireframePtr wire)
{
	if ( ! tree ) return;

	if ( tree->csg_op == csg_leaf_op )
#if __SCEDCON__
		CSG_Append_Wireframe(wire, tree->csg_instance->o_new_wire,
							 &(tree->csg_instance->o_transform),
							 &(tree->csg_instance->o_inverse),
							 &(tree->csg_instance->o_attribs));
#else
		CSG_Append_Wireframe(wire, tree->csg_instance->o_wireframe,
							 &(tree->csg_instance->o_transform),
							 &(tree->csg_instance->o_inverse),
							 (AttributePtr)(tree->csg_instance->o_attribs));
#endif
	else
	{
		CSG_Build_Full_Wireframe(tree->csg_left_child, wire);
		CSG_Build_Full_Wireframe(tree->csg_right_child, wire);
	}
}


/*	void
**	CSG_Generate_Full_Wireframe(BaseObjectPtr obj)
**	Replaces the object existing wireframe with one that is simply the
**	concatenation of all its component wireframes.
*/
void
CSG_Generate_Full_Wireframe(BaseObjectPtr obj)
{
	/* Free the existing main wireframe. */
	Wireframe_Destroy(obj->b_wireframe);

	/* Start with a new, empty wireframe. */
	obj->b_wireframe = New(Wireframe, 1);

	/* It has no faces, vertices or edges yet. */
	obj->b_wireframe->num_vertices =
	obj->b_wireframe->num_faces = 0;

	/* Allocate arrays to save realloc problems. */
	obj->b_wireframe->faces = New(Face, 1);
	obj->b_wireframe->vertices = New(Vector, 1);

	obj->b_dense_wire = NULL;
	obj->b_max_density = 0;

	obj->b_wireframe->num_attribs = 0;
	obj->b_wireframe->attribs = NULL;

	obj->b_wireframe->vertex_normals = NULL;

	/* Traverse the tree, adding all leaves to the wireframe. */
	CSG_Build_Full_Wireframe(obj->b_csgptr, obj->b_wireframe);

	obj->b_use_full = TRUE;
}


