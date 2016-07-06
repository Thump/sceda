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
**	bounds.c : Functions dealing with bounding boxes and extents for objects.
**
**	Created: 13/03/94
**
**	External functions:
**
**	Cuboid
**	Calculate_Bounds(Vector *verts, int num_verts)
**	Calculates a bounding box based on verts (there are num_verts verts).
**
**	Extent2D
**	Calculate_Projection_Extents(Vertex *verts, int num_verts)
**	Calculates the projection extent for a set of screen co-ordinates.
**
**	void
**	Update_Projection_Extents(InstanceList instances)
**	Recalculates the projection extents for all the objects in instances.
*/

#include <math.h>
#include <sced.h>
#include <csg.h>


/*	Cuboid
**	Calculate_Bounds(Vector *verts, int num_verts)
**	Calculates a bounding box based on verts (there are num_verts verts).
*/
Cuboid
Calculate_Bounds(Vector *verts, int num_verts)
{
	Cuboid	res;
	int		i;

	if ( num_verts == 0 )
	{
		VNew(0.0, 0.0, 0.0, res.min);
		VNew(0.0, 0.0, 0.0, res.max);
		return res;
	}
	else
	{
		res.min = verts[0];
		res.max = verts[0];
	}

	for ( i = 1 ; i < num_verts ; i++ )
	{
		if ( verts[i].x < res.min.x ) res.min.x = verts[i].x;
		if ( verts[i].y < res.min.y ) res.min.y = verts[i].y;
		if ( verts[i].z < res.min.z ) res.min.z = verts[i].z;
		if ( verts[i].x > res.max.x ) res.max.x = verts[i].x;
		if ( verts[i].y > res.max.y ) res.max.y = verts[i].y;
		if ( verts[i].z > res.max.z ) res.max.z = verts[i].z;
	}

	return res;

}



/*	Extent2D
**	Calculate_Projection_Extents(Vertex *verts, int num_verts)
**	Calculates the projection extent for a set of screen co-ordinates.
*/
Extent2D
Calculate_Projection_Extents(Vertex *verts, int num_verts)
{
	Extent2D	res;
	int			i;

	if (num_verts == 0)
	{
		res.min.x = res.min.y = 0;
		res.max.x = res.max.y = 0;
	}
	else
	{
		res.min = res.max = verts[0].screen;
		for ( i = 1 ; i < num_verts ; i++ )
		{
			if ( verts[i].screen.x < res.min.x ) res.min.x = verts[i].screen.x;
			if ( verts[i].screen.y < res.min.y ) res.min.y = verts[i].screen.y;
			if ( verts[i].screen.x > res.max.x ) res.max.x = verts[i].screen.x;
			if ( verts[i].screen.y > res.max.y ) res.max.y = verts[i].screen.y;
		}
	}

	return res;

}


/*	void
**	Update_Projection_Extents(InstanceList instances)
**	Recalculates the projection extents for all the objects in instances.
*/
void
Update_Projection_Extents(InstanceList instances)
{
	InstanceList	current_inst;

    for ( current_inst = instances ;
		  current_inst != NULL ;
		  current_inst = current_inst->next )
		if ( current_inst->the_instance->o_flags & ObjVisible )
			current_inst->the_instance->o_proj_extent =
				Calculate_Projection_Extents(
					current_inst->the_instance->o_main_verts,
					current_inst->the_instance->o_num_vertices);

}


Cuboid
Transform_Bound(Cuboid *bound, Transformation *transform)
{
	Vector	corners[8];

	VNew(bound->min.x, bound->min.y, bound->min.z, corners[0]);
	VNew(bound->min.x, bound->min.y, bound->max.z, corners[1]);
	VNew(bound->min.x, bound->max.y, bound->min.z, corners[2]);
	VNew(bound->min.x, bound->max.y, bound->max.z, corners[3]);
	VNew(bound->max.x, bound->min.y, bound->min.z, corners[4]);
	VNew(bound->max.x, bound->min.y, bound->max.z, corners[5]);
	VNew(bound->max.x, bound->max.y, bound->min.z, corners[6]);
	VNew(bound->max.x, bound->max.y, bound->max.z, corners[7]);

	Transform_Vertices(*transform, corners, 8)

	return Calculate_Bounds(corners, 8);
}


static Cuboid
Calculate_Object_Bounds(ObjectInstancePtr obj)
{
	Cuboid	bound;

	switch ( obj->o_parent->b_class )
	{
		case csg_obj:
			bound = obj->o_parent->b_csgptr->csg_bound;
			break;

		case wireframe_obj:
			bound = Calculate_Bounds(obj->o_parent->b_major_wire->vertices,
									 obj->o_parent->b_major_wire->num_vertices);
			break;

		case arealight_obj:
		case square_obj:
			VNew(1, 1, 0, bound.max);
			VNew(-1, -1, 0, bound.min);
			break;

		case plane_obj:
			VNew(HUGE_VAL, HUGE_VAL, 0, bound.max);
			VNew(-HUGE_VAL, -HUGE_VAL, 0, bound.min);
			break;

		case spotlight_obj:
			VNew(1, 1, 0, bound.max);
			VNew(-1, -1, -1, bound.min);
			break;

		default:
			VNew(1, 1, 1, bound.max);
			VNew(-1, -1, -1, bound.min);
			break;
	}

	return Transform_Bound(&bound, &(obj->o_transform));
}


static Cuboid
Union_Bounds(Cuboid *a, Cuboid *b)
{
	Cuboid	result;

	result.min.x = min(a->min.x, b->min.x);
	result.min.y = min(a->min.y, b->min.y);
	result.min.z = min(a->min.z, b->min.z);
	result.max.x = max(a->max.x, b->max.x);
	result.max.y = max(a->max.y, b->max.y);
	result.max.z = max(a->max.z, b->max.z);

	return result;
}


static Cuboid
Intersect_Bounds(Cuboid *a, Cuboid *b)
{
	Cuboid	result;

	result.min.x = max(a->min.x, b->min.x);
	result.min.y = max(a->min.y, b->min.y);
	result.min.z = max(a->min.z, b->min.z);
	result.max.x = min(a->max.x, b->max.x);
	result.max.y = min(a->max.y, b->max.y);
	result.max.z = min(a->max.z, b->max.z);

	return result;
}


/*
**	This function makes an initial pass over the tree, filling in bounds
**	for leaf nodes.
*/
static void
Calculate_Initial_CSG_Bounds(CSGNodePtr tree)
{
	if ( tree->csg_op == csg_leaf_op )
		tree->csg_bound = Calculate_Object_Bounds(tree->csg_instance);
	else
	{
		Calculate_Initial_CSG_Bounds(tree->csg_left_child);
		Calculate_Initial_CSG_Bounds(tree->csg_right_child);
	}
}


/*
**	This function propagates bounds upward, filling in the bounds of
**	internal nodes based on the bounds of their children.
*/
static void
Propagate_Bounds_Upward(CSGNodePtr tree)
{
	if ( tree->csg_op != csg_leaf_op )
	{
		Propagate_Bounds_Upward(tree->csg_left_child);
		Propagate_Bounds_Upward(tree->csg_right_child);
	}

	switch ( tree->csg_op )
	{
		case csg_union_op:
			tree->csg_bound =
				Union_Bounds(&(tree->csg_left_child->csg_bound),
							 &(tree->csg_right_child->csg_bound));
			break;

		case csg_intersection_op:
			tree->csg_bound =
				Intersect_Bounds(&(tree->csg_left_child->csg_bound),
								 &(tree->csg_right_child->csg_bound));
			break;

		case csg_difference_op:
			tree->csg_bound = tree->csg_left_child->csg_bound;
			break;

		default:;
	}
}


static void
Propagate_Bounds_Downward(CSGNodePtr tree)
{
	if ( ! tree ) return;

	if ( tree->csg_parent )
		tree->csg_bound = Intersect_Bounds(&(tree->csg_bound),
										   &(tree->csg_parent->csg_bound));

	if ( tree->csg_op != csg_leaf_op )
	{
		Propagate_Bounds_Downward(tree->csg_left_child);
		Propagate_Bounds_Downward(tree->csg_right_child);
	}
}


/*
**	This function uses Cameron's S-Bounds method to generate reasonably
**	efficient bounds for csg objects.
*/
void
Calculate_CSG_Bounds(CSGNodePtr tree)
{
	int	i;

	Calculate_Initial_CSG_Bounds(tree);
	Propagate_Bounds_Upward(tree);
	for ( i = 0 ; i < 3 ; i++ )
	{
		Propagate_Bounds_Downward(tree);
		Propagate_Bounds_Upward(tree);
	}
}

