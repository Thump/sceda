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
**	csg_intersect.c : Functions to intersect to polygons for csg wireframing.
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

static Boolean	Calculate_Sign(double*, short*, short);

static void		CSG_Calculate_Vertex_Distances(CSGVertexPtr, int*, double*,
											   short, CSGPlanePtr);
static Vector	CSG_Intersection_Find_Edge_Point(CSGVertexPtr, CSGVertexPtr,
												 CSGPlanePtr);
static double	CSG_Intersection_Determine_Distance(CSGPlanePtr, Vector);

static void	CSG_Intersection_Segments(CSGFacePtr, CSGVertexPtr, CSGFacePtr,
									  CSGVertexPtr, short*, short*,
									  CSGSegmentPtr, CSGSegmentPtr);
static Boolean	CSG_Determine_Segment_Intersection(CSGSegmentPtr,CSGSegmentPtr);
static void		CSG_Validate_Segment(CSGSegmentPtr, CSGSegmentPtr);

/*	Boolean
**	CSG_Intersect_Polygons(CSGFacePtr a, CSGFacePtr b, CSGSegmentPtr res_seg)
**	Intersects 2 polygons for CSG combination purposes.
**	Returns True if they are non-coplanar and intersect, FALSE otherwise.
**	In the case of intersection, res_seg is filled in as appropriate.
*/
Boolean
CSG_Intersect_Polygons(CSGFacePtr a, CSGVertexPtr a_verts,
					   CSGFacePtr b, CSGVertexPtr b_verts,
					   CSGSegmentPtr res_seg)
{
	CSGSegment	a_seg;
	CSGSegment	b_seg;
	double		*a_to_b_dists;
	double		*b_to_a_dists;
	short		*a_signs;
	short		*b_signs;

	if ( ! ( Extents_Intersect(&(a->face_extent), &(b->face_extent)) ) )
		return FALSE;

	a_to_b_dists = New(double, a->face_num_vertices);
	a_signs = New(short, a->face_num_vertices);
	CSG_Calculate_Vertex_Distances(a_verts, a->face_vertices, a_to_b_dists,
								   a->face_num_vertices, &(b->face_plane));
	if ( ! Calculate_Sign(a_to_b_dists, a_signs, a->face_num_vertices) )
	{
		free(a_to_b_dists);
		free(a_signs);
		return FALSE;
	}

	b_to_a_dists = New(double, b->face_num_vertices);
	b_signs = New(short, b->face_num_vertices);
	CSG_Calculate_Vertex_Distances(b_verts, b->face_vertices, b_to_a_dists,
								   b->face_num_vertices, &(a->face_plane));
	if ( ! Calculate_Sign(b_to_a_dists, b_signs, b->face_num_vertices) )
	{
		free(a_to_b_dists);
		free(b_to_a_dists);
		free(a_signs);
		free(b_signs);
		return FALSE;
	}

	CSG_Intersection_Segments(a, a_verts, b, b_verts, a_signs, b_signs,
							  &a_seg, &b_seg);

	free(a_to_b_dists);
	free(b_to_a_dists);
	free(a_signs);
	free(b_signs);

	*res_seg = a_seg;
	if ( ! CSG_Determine_Segment_Intersection(res_seg, &b_seg) )
		return FALSE;

	CSG_Validate_Segment(&a_seg, res_seg);

	return TRUE;
}



static Boolean
Calculate_Sign(double *dists, short *signs, short num)
{
	Boolean	been_pos_once = FALSE;
	Boolean	been_neg_once = FALSE;
	Boolean	been_zero_once = FALSE;
	Boolean	been_pos_twice = FALSE;
	Boolean	been_neg_twice = FALSE;
	Boolean	been_zero_twice = FALSE;
	int	i;

	/* Perform a consistency check. */
	signs[0] = CSGIsZero(dists[0]) ? 0 : ( dists[0] > 0 ? 1 : -1 );
	for ( i = 1 ; i < num ; i++ )
	{
		signs[i] = CSGIsZero(dists[i]) ? 0 : ( dists[i] > 0 ? 1 : -1 );

		if ( ! signs[i] )
		{
			if ( ! been_zero_once )
				been_zero_once = TRUE;
			else if ( been_zero_twice /* && signs[i - 1] != 0 */ )
				return FALSE;
			else if ( signs[i - 1] != 0 )
				been_zero_twice = TRUE;
		}
		if ( signs[i] > 0 )
		{
			if ( ! been_pos_once )
				been_pos_once = TRUE;
			else if ( been_pos_twice && signs[i - 1] <= 0 )
			{
				fprintf(stderr, "Inconsistent vertices.\n");
				for ( i = 0 ; i < num ; i++ )
					fprintf(stderr, "%d ", signs[i]);
				fprintf(stderr, "\n");
				abort();
			}
			else if ( signs[i - 1] <= 0 )
				been_pos_twice = TRUE;
		}
		if ( signs[i] < 0 )
		{
			if ( ! been_neg_once )
				been_neg_once = TRUE;
			else if ( been_neg_twice && signs[i - 1] >= 0 )
			{
				fprintf(stderr, "Inconsistent vertices.\n");
				for ( i = 0 ; i < num ; i++ )
					fprintf(stderr, "%d ", signs[i]);
				fprintf(stderr, "\n");
				abort();
			}
			else if ( signs[i - 1] >= 0 )
				been_neg_twice = TRUE;
		}
	}
	if ( ! signs[0] )
	{
		if ( ! been_zero_once )
			been_zero_once = TRUE;
		else if ( been_zero_twice /* && signs[num - 1] != 0 */ )
			return FALSE;
		else if ( signs[num - 1] )
			been_zero_twice = TRUE;
	}
	if ( signs[0] > 0 )
	{
		if ( ! been_pos_once )
			been_pos_once = TRUE;
		else if ( been_pos_twice && signs[num - 1] <= 0 )
		{
			fprintf(stderr, "Inconsistent vertices.\n");
			for ( i = 0 ; i < num ; i++ )
				fprintf(stderr, "%d ", signs[i]);
			fprintf(stderr, "\n");
			abort();
		}
		else if ( signs[num - 1] <= 0 )
			been_pos_twice = TRUE;
	}
	if ( signs[0] < 0 )
	{
		if ( ! been_neg_once )
			been_neg_once = TRUE;
		else if ( been_neg_twice && signs[num - 1] >= 0 )
		{
			fprintf(stderr, "Inconsistent vertices.\n");
			for ( i = 0 ; i < num ; i++ )
				fprintf(stderr, "%d ", signs[i]);
			fprintf(stderr, "\n");
			abort();
		}
		else if ( signs[num - 1] >= 0 )
			been_neg_twice = TRUE;
	}
	
	return ( ( been_pos_once && ( been_neg_once || been_zero_once ) ) ||
			 ( been_neg_once && ( been_pos_once || been_zero_once ) ) );
}


static void
CSG_Calculate_Vertex_Distances(CSGVertexPtr verts, int *indices, double *dists,
							   short num, CSGPlanePtr plane)
{
	Vector	temp_v;
	int		i;

	for ( i = 0 ; i < num ; i++ )
	{
		VSub(verts[indices[i]].location, plane->f_point, temp_v);
		dists[i] = VDot(temp_v, plane->f_vector);
	}
}


static void
Project_Vect_Onto_Line(Vector *orig, CSGPlanePtr line, Vector *result)
{
	Vector	temp_v;
	double	project;

	VSub(*orig, line->f_point, temp_v);
	project = VDot(line->f_vector, temp_v);
	VScalarMul(line->f_vector, project, temp_v);
	VAdd(temp_v, line->f_point, *result);
}


Boolean
Extents_Intersect(Cuboid *a, Cuboid *b)
{
	Vector	int_min, int_max;

	int_min.x = max(a->min.x, b->min.x);
	int_min.y = max(a->min.y, b->min.y);
	int_min.z = max(a->min.z, b->min.z);
	int_max.x = min(a->max.x, b->max.x);
	int_max.y = min(a->max.y, b->max.y);
	int_max.z = min(a->max.z, b->max.z);

	return ( int_min.x < int_max.x + EPSILON &&
			 int_min.y < int_max.y + EPSILON &&
			 int_min.z < int_max.z + EPSILON );
}


static void
CSG_Intersection_Segments(CSGFacePtr a, CSGVertexPtr a_verts,
						  CSGFacePtr b, CSGVertexPtr b_verts,
						  short *a_signs, short *b_signs,
						  CSGSegmentPtr a_seg, CSGSegmentPtr b_seg)
{
	CSGPlane	line;
	int			i;
	double		temp_d;
	Vector		diff;

	/* Find the direction of the line. */
	VCross(a->face_plane.f_vector, b->face_plane.f_vector, line.f_vector);
	VUnit(line.f_vector, temp_d, line.f_vector);

	/* Find all the edge-line intersections. */

	/* The first edge intersection. */
	for ( i = 0 ; a_signs[i] && a_signs[i] == a_signs[i+1] ; i++ );
	a_seg->start_distance = 0.0;
	if ( a_signs[i] == 0 )
	{
		/* Vertex i is on the line of intersection. */
		line.f_point = a_verts[a->face_vertices[i]].location;
		a_seg->start_status = segment_vertex;
		a_seg->start_vertex = i;
		a_seg->start_point = line.f_point;
	}
	else if ( a_signs[i + 1] )
	{
		/* The edge between i and i+1 crosses the intersection. */
		a_seg->start_point =
			CSG_Intersection_Find_Edge_Point(a_verts + a->face_vertices[i],
											 a_verts + a->face_vertices[i + 1],
											 &(b->face_plane));
		line.f_point = a_seg->start_point;
		a_seg->start_status = segment_edge;
		a_seg->start_vertex = i;
	}
	else
	{
		/* i + 1 is on the line of intersection. */
		line.f_point = a_verts[a->face_vertices[i+1]].location;
		a_seg->start_status = segment_vertex;
		a_seg->start_vertex = i+1;
		a_seg->start_point = line.f_point;
	}

	/* Find the other non-equal pair. */
	for ( i++ ; i < a->face_num_vertices - 1 &&
				( a_signs[i] == a_signs[i+1] ||
				  ( i == a_seg->start_vertex && a_signs[i+1] != a_signs[i-1] ) )
			  ; i++);
	if ( a_signs[i] == 0 )
	{
		a_seg->end_status = segment_vertex;
		a_seg->end_vertex = i;
		a_seg->end_point = a_verts[a->face_vertices[i]].location;
		Project_Vect_Onto_Line(&(a_seg->end_point), &line, &(a_seg->end_point));
		if ( i == a_seg->start_vertex )
			a_seg->end_distance = 0.0;
		else
			a_seg->end_distance = CSG_Intersection_Determine_Distance(&line,
								  a_verts[a->face_vertices[i]].location);
	}
	else if ( i != a->face_num_vertices - 1 && a_signs[i + 1] == 0 )
	{
		a_seg->end_status = segment_vertex;
		a_seg->end_vertex = i + 1;
		a_seg->end_point = a_verts[a->face_vertices[i+1]].location;
		Project_Vect_Onto_Line(&(a_seg->end_point), &line, &(a_seg->end_point));
		a_seg->end_distance = CSG_Intersection_Determine_Distance(&line,
							  a_verts[a->face_vertices[i+1]].location);
	}
	else if ( i == a->face_num_vertices - 1 && a_signs[0] == 0 )
	{
		a_seg->end_status = segment_vertex;
		a_seg->end_vertex = 0;
		a_seg->end_point = a_verts[a->face_vertices[0]].location;
		Project_Vect_Onto_Line(&(a_seg->end_point), &line, &(a_seg->end_point));
		a_seg->end_distance = CSG_Intersection_Determine_Distance(&line,
							  a_verts[a->face_vertices[0]].location);
	}
	else
	{
		if ( i != a->face_num_vertices - 1 )
			a_seg->end_point =
				CSG_Intersection_Find_Edge_Point(
					a_verts + a->face_vertices[i],
					a_verts + a->face_vertices[i+1], &(b->face_plane));
		else
			a_seg->end_point =
				CSG_Intersection_Find_Edge_Point(
					a_verts + a->face_vertices[i],
					a_verts + a->face_vertices[0], &(b->face_plane));
		Project_Vect_Onto_Line(&(a_seg->end_point), &line, &(a_seg->end_point));

		if ( CSGVEqual(a_seg->end_point, a_verts[a->face_vertices[i]].location,
					   diff))
		{
			a_seg->end_status = segment_vertex;
			a_seg->end_vertex = i;
		}
		else if ( i != a->face_num_vertices - 1 &&
			CSGVEqual(a_seg->end_point,
					  a_verts[a->face_vertices[i+1]].location, diff) )
		{
			a_seg->end_status = segment_vertex;
			a_seg->end_vertex = i + 1;
		}
		else if ( i == a->face_num_vertices - 1 &&
			CSGVEqual(a_seg->end_point, a_verts[a->face_vertices[0]].location,
					  diff) )
		{
			a_seg->end_status = segment_vertex;
			a_seg->end_vertex = 0;
		}
		else
		{
			a_seg->end_status = segment_edge;
			a_seg->end_vertex = i;
		}

		if ( CSGVEqual(a_seg->start_point, a_seg->end_point, diff) )
		{
			a_seg->end_point = a_seg->start_point;
			a_seg->end_status = a_seg->start_status;
			a_seg->end_vertex = a_seg->start_vertex;
			a_seg->end_distance = 0;
		}
		else
			a_seg->end_distance = CSG_Intersection_Determine_Distance(&line,
								  a_seg->end_point);
	}
	/* Categorize the center status. */
	if ( a_seg->start_status == segment_vertex &&
		 a_seg->end_status == segment_vertex )
	{
		if ( a_seg->end_vertex == a_seg->start_vertex )
			a_seg->middle_status = segment_vertex;
		else if ( ( a_seg->end_vertex == a_seg->start_vertex + 1 ) ||
				  ( ( a_seg->end_vertex == a->face_num_vertices - 1 ) &&
					a_seg->start_vertex == 0 ) ||
				  ( ( a_seg->start_vertex == a->face_num_vertices - 1 ) &&
					a_seg->end_vertex == 0 ) )
			a_seg->middle_status = segment_edge;
		else
			a_seg->middle_status = segment_face;
	}
	else
		a_seg->middle_status = segment_face;



	/* Go through the whole thing again for the other object. */
	for ( i = 0 ; b_signs[i] && b_signs[i] == b_signs[i+1] ; i++ );
	if ( b_signs[i] == 0 )
	{
		/* Vertex i is on the line of intersection. */
		b_seg->start_point = b_verts[b->face_vertices[i]].location;
		Project_Vect_Onto_Line(&(b_seg->start_point), &line,
							   &(b_seg->start_point));
		b_seg->start_distance = CSG_Intersection_Determine_Distance(&line,
								b_seg->start_point);
		b_seg->start_status = segment_vertex;
		b_seg->start_vertex = i;
	}
	else if ( b_signs[i + 1] )
	{
		/* The edge between i and i+1 crosses the intersection. */
		b_seg->start_point =
			CSG_Intersection_Find_Edge_Point(b_verts + b->face_vertices[i],
											 b_verts + b->face_vertices[i+1],
											 &(a->face_plane));
		Project_Vect_Onto_Line(&(b_seg->start_point), &line,
							   &(b_seg->start_point));

		if ( CSGVEqual(b_seg->start_point,
					   b_verts[b->face_vertices[i]].location, diff) )
		{
			b_seg->start_status = segment_vertex;
			b_seg->start_vertex = i;
		}
		else if ( CSGVEqual(b_seg->start_point,
							b_verts[b->face_vertices[i+1]].location, diff))
		{
			b_seg->start_status = segment_vertex;
			b_seg->start_vertex = i + 1;
		}
		else
		{
			b_seg->start_status = segment_edge;
			b_seg->start_vertex = i;
		}

		b_seg->start_distance = CSG_Intersection_Determine_Distance(&line,
								b_seg->start_point);
	}
	else
	{
		/* i + 1 is on the line of intersection. */
		b_seg->start_point = b_verts[b->face_vertices[i+1]].location;
		Project_Vect_Onto_Line(&(b_seg->start_point), &line,
							   &(b_seg->start_point));
		b_seg->start_distance = CSG_Intersection_Determine_Distance(&line,
								b_seg->start_point);
		b_seg->start_status = segment_vertex;
		b_seg->start_vertex = i+1;
	}

	/* Find the other non-equal pair. */
	for ( i++ ; i < b->face_num_vertices - 1 &&
				( b_signs[i] == b_signs[i+1] ||
				  ( i == b_seg->start_vertex && b_signs[i+1] != b_signs[i-1] ) )
			  ; i++);
	if ( b_signs[i] == 0 )
	{
		b_seg->end_status = segment_vertex;
		b_seg->end_vertex = i;
		b_seg->end_point = b_verts[b->face_vertices[i]].location;
		Project_Vect_Onto_Line(&(b_seg->end_point), &line, &(b_seg->end_point));
		if ( i == b_seg->start_vertex )
			b_seg->end_distance = b_seg->start_distance;
		else
			b_seg->end_distance = CSG_Intersection_Determine_Distance(&line,
								  b_seg->end_point);
	}
	else if ( i != b->face_num_vertices - 1 && b_signs[i + 1] == 0 )
	{
		b_seg->end_status = segment_vertex;
		b_seg->end_vertex = i + 1;
		b_seg->end_point = b_verts[b->face_vertices[i+1]].location;
		Project_Vect_Onto_Line(&(b_seg->end_point), &line, &(b_seg->end_point));
		b_seg->end_distance = CSG_Intersection_Determine_Distance(&line,
							  b_seg->end_point);
	}
	else if ( i == b->face_num_vertices - 1 && b_signs[0] == 0 )
	{
		b_seg->end_status = segment_vertex;
		b_seg->end_vertex = 0;
		b_seg->end_point = b_verts[b->face_vertices[0]].location;
		Project_Vect_Onto_Line(&(b_seg->end_point), &line, &(b_seg->end_point));
		b_seg->end_distance = CSG_Intersection_Determine_Distance(&line,
							  b_seg->end_point);
	}
	else
	{
		if ( i != b->face_num_vertices - 1 )
			b_seg->end_point =
				CSG_Intersection_Find_Edge_Point(b_verts + b->face_vertices[i],
												b_verts + b->face_vertices[i+1],
												&(a->face_plane));
		else
			b_seg->end_point =
				CSG_Intersection_Find_Edge_Point(b_verts + b->face_vertices[i],
												 b_verts + b->face_vertices[0],
												 &(a->face_plane));
		Project_Vect_Onto_Line(&(b_seg->end_point), &line, &(b_seg->end_point));

		if ( CSGVEqual(b_seg->end_point, b_verts[b->face_vertices[i]].location,
					   diff))
		{
			b_seg->end_status = segment_vertex;
			b_seg->end_vertex = i;
		}
		else if ( i != b->face_num_vertices - 1 &&
			CSGVEqual(b_seg->end_point,
					  b_verts[b->face_vertices[i+1]].location, diff) )
		{
			b_seg->end_status = segment_vertex;
			b_seg->end_vertex = i + 1;
		}
		else if ( i == b->face_num_vertices - 1 &&
			CSGVEqual(b_seg->end_point, b_verts[b->face_vertices[0]].location,
					  diff) )
		{
			b_seg->end_status = segment_vertex;
			b_seg->end_vertex = 0;
		}
		else
		{
			b_seg->end_status = segment_edge;
			b_seg->end_vertex = i;
		}

		if ( CSGVEqual(b_seg->start_point, b_seg->end_point, diff) )
		{
			b_seg->end_point = b_seg->start_point;
			b_seg->end_status = b_seg->start_status;
			b_seg->end_vertex = b_seg->start_vertex;
			b_seg->end_distance = b_seg->start_distance;
		}
		else
			b_seg->end_distance = CSG_Intersection_Determine_Distance(&line,
								  b_seg->end_point);
	}
	/* Categorize the center status. */
	if ( b_seg->start_status == segment_vertex &&
		 b_seg->end_status == segment_vertex )
	{
		if ( b_seg->end_vertex == b_seg->start_vertex )
			b_seg->middle_status = segment_vertex;
		else if ( ( b_seg->end_vertex == b_seg->start_vertex + 1 ) ||
				  ( ( b_seg->end_vertex == b->face_num_vertices - 1 ) &&
					b_seg->start_vertex == 0 ) ||
				  ( ( b_seg->start_vertex == b->face_num_vertices - 1 ) &&
					b_seg->end_vertex == 0 ) )
			b_seg->middle_status = segment_edge;
		else
			b_seg->middle_status = segment_face;
	}
	else
		b_seg->middle_status = segment_face;


	/* At this stage, start vert will be less than end_vertex except in
	** one case, that where start = 1 and end = 0. So check for and
	** rectify this case.  We also want it to be circular, so that 0
	** follows max if it's vert-edge-vert.
	*/
	if ( a_seg->start_vertex == 1 && a_seg->end_vertex == 0 )
	{
		SegmentStatus	stat;
		double			dist;
		int				vert;
		Vector			pt;

		vert = a_seg->start_vertex;
		a_seg->start_vertex = a_seg->end_vertex;
		a_seg->end_vertex = vert;
		dist = a_seg->start_distance;
		a_seg->start_distance = a_seg->end_distance;
		a_seg->end_distance = dist;
		pt = a_seg->start_point;
		a_seg->start_point = a_seg->end_point;
		a_seg->end_point = pt;
		stat = a_seg->start_status;
		a_seg->start_status = a_seg->end_status;
		a_seg->end_status = stat;
	}
}



static Vector
CSG_Intersection_Find_Edge_Point(CSGVertexPtr v1, CSGVertexPtr v2,
								 CSGPlanePtr pl)
{
	double	v1_dist, v2_dist;
	Vector	temp_v;
	double	temp_d;
	Vector	res;

	/* Get the signed distances from the plane (again). */
	VSub(v1->location, pl->f_point, temp_v);
	v1_dist = VDot(temp_v, pl->f_vector);
	VSub(v2->location, pl->f_point, temp_v);
	v2_dist = VDot(temp_v, pl->f_vector);

	temp_d = v1_dist - v2_dist;
	res.x = ( v1_dist * v2->location.x - v2_dist * v1->location.x ) / temp_d;
	res.y = ( v1_dist * v2->location.y - v2_dist * v1->location.y ) / temp_d;
	res.z = ( v1_dist * v2->location.z - v2_dist * v1->location.z ) / temp_d;

	return res;
}


static double
CSG_Intersection_Determine_Distance(CSGPlanePtr line, Vector point)
{
	/* Maximize the denominator. */
	if ( fabs(line->f_vector.x) > fabs(line->f_vector.y) )
		if ( fabs(line->f_vector.x) > fabs(line->f_vector.z) )
			return ( point.x - line->f_point.x ) / line->f_vector.x;
		else
			return ( point.z - line->f_point.z ) / line->f_vector.z;
	else
		if ( fabs(line->f_vector.y) > fabs(line->f_vector.z) )
			return ( point.y - line->f_point.y ) / line->f_vector.y;
		else
			return ( point.z - line->f_point.z ) / line->f_vector.z;
}


static Boolean
CSG_Determine_Segment_Intersection(CSGSegmentPtr a_seg, CSGSegmentPtr b_seg)
{
	Vector	diff;

	if ( b_seg->start_distance > b_seg->end_distance )
	{
		SegmentStatus	stat;
		double			dist;
		int				vert;
		Vector			pt;

		vert = b_seg->start_vertex;
		b_seg->start_vertex = b_seg->end_vertex;
		b_seg->end_vertex = vert;
		dist = b_seg->start_distance;
		b_seg->start_distance = b_seg->end_distance;
		b_seg->end_distance = dist;
		pt = b_seg->start_point;
		b_seg->start_point = b_seg->end_point;
		b_seg->end_point = pt;
		stat = b_seg->start_status;
		b_seg->start_status = b_seg->end_status;
		b_seg->end_status = stat;
	}

	a_seg->start_was_vert = a_seg->start_status == segment_vertex;
	a_seg->end_was_vert = a_seg->end_status == segment_vertex;
	if ( a_seg->start_distance < a_seg->end_distance )
	{
		if ( a_seg->start_distance < b_seg->start_distance &&
			 ! CSGVEqual(a_seg->start_point, b_seg->start_point, diff) )
		{
			a_seg->start_distance = b_seg->start_distance;
			a_seg->start_point = b_seg->start_point;
			a_seg->start_status = a_seg->middle_status;
		}
		if ( a_seg->end_distance > b_seg->end_distance &&
			 ! CSGVEqual(a_seg->end_point, b_seg->end_point, diff) )
		{
			a_seg->end_distance = b_seg->end_distance;
			a_seg->end_point = b_seg->end_point;
			a_seg->end_status = a_seg->middle_status;
		}
		return ( a_seg->end_distance >= a_seg->start_distance );
	}
	else
	{
		if ( a_seg->end_distance < b_seg->start_distance &&
			 ! CSGVEqual(a_seg->end_point, b_seg->start_point, diff) )
		{
			a_seg->end_distance = b_seg->start_distance;
			a_seg->end_point = b_seg->start_point;
			a_seg->end_status = a_seg->middle_status;
		}
		if ( a_seg->start_distance > b_seg->end_distance &&
			 ! CSGVEqual(a_seg->start_point, b_seg->end_point, diff) )
		{
			a_seg->start_distance = b_seg->end_distance;
			a_seg->start_point = b_seg->end_point;
			a_seg->start_status = a_seg->middle_status;
		}
		return ( a_seg->end_distance <= a_seg->start_distance );
	}
	
	return FALSE;
}


static void
CSG_Validate_Segment(CSGSegmentPtr orig, CSGSegmentPtr new)
{
	Vector	diff;

	/* Check for features where the start and end vertex are identical
	** but shouldn't be.
	*/
	if ( CSGVEqual(new->start_point, new->end_point, diff))
	{
		if ( new->start_status == segment_vertex &&
			 new->middle_status == segment_edge &&
			 new->end_status == segment_edge )
		{
			new->end_status = segment_vertex;
			new->end_vertex = new->start_vertex;
		}
		else if ( new->start_status == segment_edge &&
				  new->middle_status == segment_edge &&
				  new->end_status == segment_vertex )
		{
			new->start_status = segment_vertex;
			new->start_vertex = new->end_vertex;
		}
		else if ( new->start_status == segment_vertex &&
				  new->middle_status == segment_face &&
				  new->end_status == segment_face )
		{
			new->middle_status = segment_vertex;
			new->end_status = segment_vertex;
			new->end_vertex = new->start_vertex;
		}
		else if ( new->start_status == segment_face &&
				  new->middle_status == segment_face &&
				  new->end_status == segment_vertex )  
		{
			new->middle_status = segment_vertex;
			new->start_status = segment_vertex;
			new->start_vertex = new->end_vertex;
		}
		else if ( new->start_status == segment_edge &&
				  new->middle_status == segment_face &&
				  new->end_status == segment_face )
		{
			new->middle_status = segment_edge;
			new->end_status = segment_edge;
			new->end_vertex = new->start_vertex;
		}
		else if ( new->start_status == segment_face &&
				  new->middle_status == segment_face &&
				  new->end_status == segment_edge )
		{
			new->middle_status = segment_edge;
			new->start_status = segment_edge;
			new->start_vertex = new->end_vertex;
		}

		else if ( new->start_status == segment_edge && 
				  new->middle_status == segment_face &&
				  new->end_status == segment_edge )
			/* Attempt a fix. */
			/* Make it edge-edge-edge. */
			new->middle_status = segment_edge;

		return;
	}
}
