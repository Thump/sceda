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
**	Sced: A Constraint Based Object Scene Editor
**
**	dense_wireframe.c : Functions for manipulating and generating dense
**						wireframes.
*/

#include <math.h>
#include <sced.h>
#include <gen_wireframe.h>


int
Wireframe_Density_Level(ObjectInstancePtr inst)
{
	int level = 0;
	int	i;

	/* Work out which level it's currently at. */
	if ( inst->o_wireframe == inst->o_parent->b_wireframe )
		level = 0;
	else
	{
		for ( i = 0 ; i < inst->o_parent->b_max_density ; i++ )
			if ( inst->o_parent->b_dense_wire[i] == inst->o_wireframe )
			{
				level = i + 1;
				break;
			}

		if ( i == inst->o_parent->b_max_density )
		{
			fprintf(stderr,
					"Sced: Existing wireframe not found in parent: Object %s\n",
					inst->o_label);
			return 0;
		}
	}

	return level;
}


void
Wireframe_Denser_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	WindowInfoPtr	window = (WindowInfoPtr)cl;
	InstanceList	elmt;

	for ( elmt = window->selected_instances ; elmt ; elmt = elmt->next )
		Object_Change_Wire_Level(elmt->the_instance,
							Wireframe_Density_Level(elmt->the_instance) + 1);

	View_Update(window, window->selected_instances, CalcView);
	Update_Projection_Extents(window->selected_instances);
}


void
Wireframe_Thinner_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	WindowInfoPtr	window = (WindowInfoPtr)cl;
	InstanceList	elmt;

	for ( elmt = window->selected_instances ; elmt ; elmt = elmt->next )
		Object_Change_Wire_Level(elmt->the_instance,
							Wireframe_Density_Level(elmt->the_instance) - 1);

	View_Update(window, window->selected_instances, CalcView);
	Update_Projection_Extents(window->selected_instances);
}


static WireframePtr
Dense_Get_New_Wireframe(BaseObjectPtr base, int level)
{
	/* level is the new desired level. */

	if ( level > base->b_max_density + 1 )
		Dense_Get_New_Wireframe(base, level - 1);

	if ( level == base->b_max_density + 1 )
	{
		if ( level == 1 )
			base->b_dense_wire = New(WireframePtr, 1);
		else
			base->b_dense_wire = More(base->b_dense_wire, WireframePtr, level);

		switch ( base->b_class )
		{
			case sphere_obj:
				base->b_dense_wire[level - 1] =
					Dense_Sphere_Wireframe(level,
						( level == 1 ? NULL : base->b_dense_wire[level - 2]));
				break;
			case cone_obj:
				base->b_dense_wire[level - 1] = Dense_Cone_Wireframe(level);
				break;
			case cylinder_obj:
				base->b_dense_wire[level - 1] = Dense_Cylinder_Wireframe(level);
				break;
			case spotlight_obj:
				base->b_dense_wire[level - 1] = Dense_Spot_Wireframe(level);
				break;
			default:
				fprintf(stderr, "Dense_Get_New_Wireframe: How the hell???\n");
		}
		base->b_max_density++;
	}

	return base->b_dense_wire[level - 1];
}


void
Object_Change_Wire_Level(ObjectInstancePtr inst, int level)
{
	if ( inst->o_parent->b_class != sphere_obj &&
		 inst->o_parent->b_class != cone_obj &&
		 inst->o_parent->b_class != cylinder_obj &&
		 inst->o_parent->b_class != spotlight_obj )
		return;

	if ( level < 0 )
		level = 0;

	if ( level == 0 )
		inst->o_wireframe = inst->o_parent->b_wireframe;
	else
		inst->o_wireframe = Dense_Get_New_Wireframe(inst->o_parent, level);

	Object_Change_Wire(inst);
}


void
Object_Change_Wire(ObjectInstancePtr inst)
{
	Matrix	transp;
	double	temp_d;
	int		i;

	inst->o_num_vertices = inst->o_wireframe->num_vertices;
	free(inst->o_world_verts);
	inst->o_world_verts = New(Vector, inst->o_num_vertices);
	free(inst->o_main_verts);
	inst->o_main_verts = New(Vertex, inst->o_num_vertices);
	for ( i = 0 ; i < inst->o_num_vertices ; i++ )
	{
		MVMul(inst->o_transform.matrix,
			  inst->o_wireframe->vertices[i],
			  inst->o_world_verts[i]);
		VAdd(inst->o_transform.displacement,
			 inst->o_world_verts[i], inst->o_world_verts[i]);
	}
	inst->o_num_faces = inst->o_wireframe->num_faces;
	free(inst->o_normals);
	inst->o_normals = New(Vector, inst->o_num_faces);
	MTrans(inst->o_inverse.matrix, transp);
	for ( i = 0 ; i < inst->o_num_faces ; i++ )
	{
		MVMul(transp, inst->o_wireframe->faces[i].normal,
			  inst->o_normals[i]);
		VUnit(inst->o_normals[i], temp_d, inst->o_normals[i]);
	}
}



/*	Wireframe*
**	Dense_Cylinder_Wireframe(int level);
**	Returns a pointer to a NEW dense wireframe structure, with
**	2 ^ ( level + 3 ) vertices on each endcap.
*/
WireframePtr
Dense_Cylinder_Wireframe(int level)
{
	WireframePtr	result = New(Wireframe, 1);
	double			angle;
	double			total_angle;
	int				half_num;
	int				i;
	Vector			temp_v, vert_vect;
	double			temp_d;

	result->num_faces = ( 1 << ( level + 3 ) ) + 2;
	result->num_vertices = ( 1 << ( level + 4 ) ) + 3;

	result->vertices = New(Vector, result->num_vertices);

	/* Work out what the angle subtended at the center by the arc joining
	** neighbouring vertices is.
	*/
	angle = M_PI_4 / ( 1 << level );
	total_angle = 0.0;

	half_num = ( 1 << ( level + 3 ) );
	for ( i = 0, total_angle = 0.0 ; i < half_num ; i++, total_angle += angle )
	{
		result->vertices[i].x =
		result->vertices[i + half_num].x = cos(total_angle);
		result->vertices[i].y =
		result->vertices[i + half_num].y = sin(total_angle);
		result->vertices[i].z = 1;
		result->vertices[i + half_num].z = -1;
	}
	VNew(0, 0, 1, result->vertices[result->num_vertices - 3]);
	VNew(0, 0, -1, result->vertices[result->num_vertices - 2]);
	VNew(0, 0, 0, result->vertices[result->num_vertices - 1]);

	result->faces = New(Face, result->num_faces);
	for ( i = 0 ; i < result->num_faces - 2 ; i++ )
	{
		result->faces[i].num_vertices = 4;
		result->faces[i].vertices = New(int, 4);

		result->faces[i].vertices[0] = i;
		result->faces[i].vertices[1] = i + 1;
		result->faces[i].vertices[2] = half_num + 1 + i;
		result->faces[i].vertices[3] = half_num + i;

		result->faces[i].face_attribs = NULL;
	}
	result->faces[result->num_faces - 3].vertices[1] = 0;
	result->faces[result->num_faces - 3].vertices[2] = half_num;
	VNew(0, 0, 1, vert_vect);
	for ( i = 0 ; i < result->num_faces - 2 ; i++ )
	{
		VSub(result->vertices[result->faces[i].vertices[1]],
			 result->vertices[result->faces[i].vertices[0]], temp_v);
		VCross(temp_v, vert_vect, result->faces[i].normal);
		VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);
	}

	for ( i = result->num_faces - 2 ; i < result->num_faces ; i++ )
	{
		result->faces[i].num_vertices = half_num;
		result->faces[i].vertices = New(int, half_num);
		result->faces[i].face_attribs = NULL;
	}
	for ( i = 0 ; i < half_num ; i++ )
		result->faces[result->num_faces - 2].vertices[i] = half_num - 1 - i;
	VNew(0, 0, 1, result->faces[result->num_faces - 2].normal);
	for ( i = 0 ; i < half_num ; i++ )
		result->faces[result->num_faces - 1].vertices[i] = half_num + i;
	VNew(0, 0, -1, result->faces[result->num_faces - 1].normal);

	result->attribs = NULL;
	result->num_attribs = 0;
	result->vertex_normals = NULL;

	return result;
}



/*	WireframePtr
**	Dense_Cone_Wireframe(int level);
**	Returns a pointer to a NEW dense wireframe cone.
**	NULL on failure.
*/
WireframePtr
Dense_Cone_Wireframe(int level)
{
	WireframePtr	result = New(Wireframe, 1);
	double			angle;
	double			total_angle;
	int				i;
	Vector			temp_v1, temp_v2;
	double			temp_d;

	result->num_faces = ( 1 << ( level + 3 ) ) + 1;
	result->num_vertices = ( 1 << ( level + 3 ) ) + 3;

	result->vertices = New(Vector, result->num_vertices);

	/* Work out what the angle subtended at the center by the arc joining
	** neighbouring vertices is.
	*/
	angle = M_PI_4 / ( 1 << level );
	total_angle = 0.0;

	VNew(0, 0, 1, result->vertices[0]);
	for ( i = 1, total_angle = 0.0 ;
		  i < result->num_vertices - 2 ;
		  i++, total_angle += angle )
	{
		result->vertices[i].x = cos(total_angle);
		result->vertices[i].y = sin(total_angle);
		result->vertices[i].z = -1;
	}
	VNew(0, 0, -1, result->vertices[result->num_vertices - 2]);
	VNew(0, 0, 0, result->vertices[result->num_vertices - 1]);

	result->faces = New(Face, result->num_faces);
	for ( i = 0 ; i < result->num_faces - 1 ; i++ )
	{
		result->faces[i].num_vertices = 3;
		result->faces[i].vertices = New(int, 3);

		result->faces[i].vertices[0] = 0;
		result->faces[i].vertices[1] = i + 2;
		result->faces[i].vertices[2] = i + 1;

		result->faces[i].face_attribs = NULL;
	}
	result->faces[result->num_faces - 2].vertices[1] = 1;
	for ( i = 0 ; i < result->num_faces - 1 ; i++ )
	{
		VSub(result->vertices[result->faces[i].vertices[0]],
			 result->vertices[result->faces[i].vertices[1]], temp_v1);
		VSub(result->vertices[result->faces[i].vertices[2]],
			 result->vertices[result->faces[i].vertices[1]], temp_v2);
		VCross(temp_v1, temp_v2, result->faces[i].normal);
		VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);
	}

	result->faces[result->num_faces - 1].num_vertices = result->num_vertices-3;
	result->faces[result->num_faces - 1].vertices =
		New(int, result->num_vertices - 3);
	result->faces[result->num_faces - 1].face_attribs = NULL;
	for ( i = 0 ; i < result->num_vertices - 3 ; i++ )
		result->faces[result->num_faces - 1].vertices[i] = 1 + i;
	VNew(0, 0, -1, result->faces[result->num_faces - 1].normal);

	result->attribs = NULL;
	result->vertex_normals = NULL;
	result->num_attribs = 0;

	return result;
}


static WireframePtr
Sphere_Build_Initial_Wireframe()
{
	WireframePtr	initial = New(Wireframe, 1);
	WireframePtr	result;
	int				i;

	initial->num_faces = 8;
	initial->num_vertices = 7;

	initial->vertices = New(Vector, 7);
	VNew(1, 0, 0, initial->vertices[0]);
	VNew(-1, 0, 0, initial->vertices[1]);
	VNew(0, 1, 0, initial->vertices[2]);
	VNew(0, -1, 0, initial->vertices[3]);
	VNew(0, 0, 1, initial->vertices[4]);
	VNew(0, 0, -1, initial->vertices[5]);
	VNew(0, 0, 0, initial->vertices[6]);

	initial->faces = New(Face, 8);
	for ( i = 0 ; i < 8 ; i++ )
	{
		initial->faces[i].num_vertices = 3;
		initial->faces[i].vertices = New(int, 3);
	}
	initial->faces[0].vertices[0] = 0;
	initial->faces[0].vertices[1] = 4;
	initial->faces[0].vertices[2] = 2;
	initial->faces[1].vertices[0] = 2;
	initial->faces[1].vertices[1] = 4;
	initial->faces[1].vertices[2] = 1;
	initial->faces[2].vertices[0] = 1;
	initial->faces[2].vertices[1] = 4;
	initial->faces[2].vertices[2] = 3;
	initial->faces[3].vertices[0] = 3;
	initial->faces[3].vertices[1] = 4;
	initial->faces[3].vertices[2] = 0;
	initial->faces[4].vertices[0] = 0;
	initial->faces[4].vertices[1] = 2;
	initial->faces[4].vertices[2] = 5;
	initial->faces[5].vertices[0] = 2;
	initial->faces[5].vertices[1] = 1;
	initial->faces[5].vertices[2] = 5;
	initial->faces[6].vertices[0] = 1;
	initial->faces[6].vertices[1] = 3;
	initial->faces[6].vertices[2] = 5;
	initial->faces[7].vertices[0] = 3;
	initial->faces[7].vertices[1] = 0;
	initial->faces[7].vertices[2] = 5;

	result = Dense_Sphere_Wireframe(0, initial);
	free(initial->vertices);
	for ( i = 0 ; i < 8 ; i++ )
		free(initial->faces[i].vertices);
	free(initial->faces);
	free(initial);

	return result;
}


/* Adds an edge to an edge list. If the edge is already there, returns
** the new field of the structure and removes the edge.
** If it is not present, adds the edge and returns the new field passed.
** While this may not look like a very efficient way of doing things,
** the edge list rarely gets very long, and the overhead of a more
** complex structure appears to be unjustified.
*/
int
Edge_Add_Edge(EdgePtr *list, int v1, int v2, int new)
{
	EdgePtr	elmt, victim;
	int		swap;
	int		result;

	if ( v1 > v2 )
		swap = v1, v1 = v2, v2 = swap;
		
	if ( ! (*list) )
	{
		*list = New(EdgeElmt, 1);
		(*list)->v1 = v1;
		(*list)->v2 = v2;
		(*list)->new_v = new;
		(*list)->next = NULL;
		return new;
	}
	else
	{
		/* Look for the edge already there, finding the end of the list
		** in the process.
		*/
		if ( (*list)->v1 == v1 && (*list)->v2 == v2 )
		{
			result = (*list)->new_v;
			victim = *list;
			*list = (*list)->next;
			free(victim);
			return result;
		}
		for ( elmt = *list ; elmt->next ; elmt = elmt->next )
			if ( elmt->next->v1 == v1 && elmt->next->v2 == v2 )
			{
				result = elmt->next->new_v;
				victim = elmt->next;
				elmt->next = elmt->next->next;
				free(victim);
				return result;
			}

		/* The edge isn't already there. */
		elmt->next = New(EdgeElmt, 1);
		elmt->next->v1 = v1;
		elmt->next->v2 = v2;
		elmt->next->new_v = new;
		elmt->next->next = NULL;
	}

	return new;
}


/*	WireframePtr
**	Dense_Sphere_Wireframe(int level, WireframePtr previous);
**	Returns a pointer to a NEW dense wireframe structure for a sphere.
**	The algorithm builds on the previous approximation.
*/
WireframePtr
Dense_Sphere_Wireframe(int level, WireframePtr previous)
{
	WireframePtr	result = New(Wireframe, 1);
	Vector			temp_v1, temp_v2;
	double			temp_d;
	EdgePtr			edge_list = NULL;
	int				vert_count;
	int				face_count;
	int				i;
	int				new1, new2, new3;

#define Midpoint(v1, v2, r) \
	(r).x = ( (v1).x + (v2).x ) * 0.5; \
	(r).y = ( (v1).y + (v2).y ) * 0.5; \
	(r).z = ( (v1).z + (v2).z ) * 0.5;

	if ( ! previous )
		previous = Sphere_Build_Initial_Wireframe();

	result->num_faces = previous->num_faces * 4;
	result->num_vertices = previous->num_vertices +
						   previous->num_faces * 3 * 0.5;

	/* Copy over the common vertices. */
	result->vertices = New(Vector, result->num_vertices);
	for ( i = 0 ; i < previous->num_vertices - 1 ; i++ )
		result->vertices[i] = previous->vertices[i];
	VNew(0, 0, 0, result->vertices[result->num_vertices - 1]);

	result->faces = New(Face, result->num_faces);
	/* Split faces, adding new vertices as we go. */
	face_count = 0;
	vert_count = previous->num_vertices - 1;
	for ( i = 0, face_count = 0 ; i < previous->num_faces ; i++ )
	{
		new1 = Edge_Add_Edge(&edge_list, previous->faces[i].vertices[0],
							   previous->faces[i].vertices[2], vert_count);
		if ( vert_count == new1 )
		{
			Midpoint(previous->vertices[previous->faces[i].vertices[0]],
					 previous->vertices[previous->faces[i].vertices[2]],
					 result->vertices[vert_count]);
			VUnit(result->vertices[vert_count], temp_d,
				  result->vertices[vert_count]);
			vert_count++;
		}
		new2 = Edge_Add_Edge(&edge_list, previous->faces[i].vertices[0],
							   previous->faces[i].vertices[1], vert_count);
		if ( vert_count == new2 )
		{
			Midpoint(previous->vertices[previous->faces[i].vertices[0]],
					 previous->vertices[previous->faces[i].vertices[1]],
					 result->vertices[vert_count]);
			VUnit(result->vertices[vert_count], temp_d,
				  result->vertices[vert_count]);
			vert_count++;
		}
		new3 = Edge_Add_Edge(&edge_list, previous->faces[i].vertices[1],
							   previous->faces[i].vertices[2], vert_count);
		if ( vert_count == new3 )
		{
			Midpoint(previous->vertices[previous->faces[i].vertices[1]],
					 previous->vertices[previous->faces[i].vertices[2]],
					 result->vertices[vert_count]);
			VUnit(result->vertices[vert_count], temp_d,
				  result->vertices[vert_count]);
			vert_count++;
		}

		/* Now have three new vertex indicies. */
		/* Create new faces. */
		result->faces[face_count].num_vertices = 3;
		result->faces[face_count].vertices = New(int, 3);
		result->faces[face_count].vertices[0] = previous->faces[i].vertices[0];
		result->faces[face_count].vertices[1] = new2;
		result->faces[face_count].vertices[2] = new1;
		result->faces[face_count].face_attribs = NULL;
		face_count++;

		result->faces[face_count].num_vertices = 3;
		result->faces[face_count].vertices = New(int, 3);
		result->faces[face_count].vertices[0] = new2;
		result->faces[face_count].vertices[1] = previous->faces[i].vertices[1];
		result->faces[face_count].vertices[2] = new3;
		result->faces[face_count].face_attribs = NULL;
		face_count++;

		result->faces[face_count].num_vertices = 3;
		result->faces[face_count].vertices = New(int, 3);
		result->faces[face_count].vertices[0] = new1;
		result->faces[face_count].vertices[1] = new2;
		result->faces[face_count].vertices[2] = new3;
		result->faces[face_count].face_attribs = NULL;
		face_count++;

		result->faces[face_count].num_vertices = 3;
		result->faces[face_count].vertices = New(int, 3);
		result->faces[face_count].vertices[0] = new1;
		result->faces[face_count].vertices[1] = new3;
		result->faces[face_count].vertices[2] = previous->faces[i].vertices[2];
		result->faces[face_count].face_attribs = NULL;
		face_count++;
	}

	if ( vert_count != result->num_vertices - 1 )
	{
		fprintf(stderr, "ERROR: Incorrect sphere vertex count.\n");
		exit(1);
	}

	/* Set face normals. */
	for ( i = 0 ; i < result->num_faces ; i++ )
	{
		VSub(result->vertices[result->faces[i].vertices[0]],
			 result->vertices[result->faces[i].vertices[1]], temp_v1);
		VSub(result->vertices[result->faces[i].vertices[2]],
			 result->vertices[result->faces[i].vertices[1]], temp_v2);
		VCross(temp_v1, temp_v2, result->faces[i].normal);
		VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);
	}

	result->attribs = NULL;
	result->vertex_normals = NULL;
	result->num_attribs = 0;

	return result;
}


/*	WireframePtr
**	Dense_Spot_Wireframe(int level);
**	Returns a pointer to a NEW dense spotlight wireframe.
**	NULL on failure.
*/
WireframePtr
Dense_Spot_Wireframe(int level)
{
	WireframePtr	result = New(Wireframe, 1);
	double			angle;
	double			total_angle;
	int				i;
	Vector			temp_v1, temp_v2;
	double			temp_d;

	result->num_faces = ( 1 << ( level + 3 ) );
	result->num_vertices = ( 1 << ( level + 3 ) ) + 2;

	result->vertices = New(Vector, result->num_vertices);

	/* Work out what the angle subtended at the center by the arc joining
	** neighbouring vertices is.
	*/
	angle = M_PI_4 / ( 1 << level );
	total_angle = 0.0;

	VNew(0, 0, 1, result->vertices[0]);
	for ( i = 1, total_angle = 0.0 ;
		  i < result->num_vertices - 1 ;
		  i++, total_angle += angle )
	{
		result->vertices[i].x = cos(total_angle);
		result->vertices[i].y = sin(total_angle);
		result->vertices[i].z = 0;
	}
	VNew(0, 0, 0, result->vertices[result->num_vertices - 1]);

	result->faces = New(Face, result->num_faces);
	for ( i = 0 ; i < result->num_faces ; i++ )
	{
		result->faces[i].num_vertices = 3;
		result->faces[i].vertices = New(int, 3);

		result->faces[i].vertices[0] = 0;
		result->faces[i].vertices[1] = i + 2;
		result->faces[i].vertices[2] = i + 1;

		result->faces[i].face_attribs = NULL;
	}
	result->faces[result->num_faces - 1].vertices[1] = 1;
	for ( i = 0 ; i < result->num_faces ; i++ )
	{
		VSub(result->vertices[result->faces[i].vertices[0]],
			 result->vertices[result->faces[i].vertices[1]], temp_v1);
		VSub(result->vertices[result->faces[i].vertices[2]],
			 result->vertices[result->faces[i].vertices[1]], temp_v2);
		VCross(temp_v1, temp_v2, result->faces[i].normal);
		VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);
	}

	result->attribs = NULL;
	result->vertex_normals = NULL;
	result->num_attribs = 0;

	return result;
}
