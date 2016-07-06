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
**	csg_split.c : Functions for splitting CSG polygons.
*/

#include <config.h>
#if __SCEDCON__
#include <convert.h>
#else
#include <sced.h>
#include <csg.h>
#include <csg_wire.h>
#endif

static void	CSG_Split_Vert_Face_Vert(CSGWireframePtr, short, CSGSegmentPtr);
static void	CSG_Split_Vert_Edge_Edge(CSGWireframePtr, short, CSGSegmentPtr);
static void	CSG_Split_Edge_Edge_Vert(CSGWireframePtr, short, CSGSegmentPtr);
static void	CSG_Split_Face_Face_Vertex(CSGWireframePtr, short, CSGSegmentPtr);
static void	CSG_Split_Vertex_Face_Face(CSGWireframePtr, short, CSGSegmentPtr);

static int	CSG_Split_Add_Vertex(CSGWireframePtr, Vector);
static int	CSG_Split_Insert_Vertex(CSGWireframePtr, int, short,Vector);
static int	CSG_Split_Add_Face_Vertex(CSGWireframePtr, int, Vector);

#define vertex(f, i)	( w->vertices[w->faces[f].face_vertices[i]] )

static Vector	diff;

Boolean
CSG_Vert_Vert_Vert_Split(CSGWireframePtr w, short index, CSGSegmentPtr seg)
{
#if ( CSG_WIRE_DEBUG_1 )
	fprintf(stderr,"V-V-V face %d vertex %d\n", index, seg->start_vertex);
#endif

	vertex(index, seg->start_vertex).status = vertex_boundary;

	return TRUE;
}


Boolean
CSG_Vert_Edge_Vert_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
#if ( CSG_WIRE_DEBUG_1 )
	fprintf(stderr,"V-E-V face %d vertices %d %d\n", index, seg->start_vertex,
		seg->end_vertex);
#endif

	vertex(index, seg->start_vertex).status = vertex_boundary;
	vertex(index, seg->end_vertex).status = vertex_boundary;

	return TRUE;
}


Boolean
CSG_Vert_Edge_Edge_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
	if ( seg->start_status == segment_edge )
	{
		vertex(index, seg->end_vertex).status = vertex_boundary;
		if ( seg->start_vertex == 0 &&
			 seg->end_vertex == w->faces[index].face_num_vertices - 1 )
		{
			seg->start_vertex = seg->end_vertex;
			seg->start_status = segment_vertex;
			seg->end_vertex = 0;
			seg->end_point = seg->start_point;
			seg->end_status = segment_edge;
			CSG_Split_Vert_Edge_Edge(w, index, seg);
		}
		else
			CSG_Split_Edge_Edge_Vert(w, index, seg);
	}
	else
	{
		vertex(index, seg->start_vertex).status = vertex_boundary;
		if ( seg->start_vertex == 0 &&
			 seg->end_vertex == w->faces[index].face_num_vertices - 1 )
		{
			seg->start_vertex = seg->end_vertex;
			seg->start_point = seg->end_point;
			seg->start_status = segment_edge;
			seg->end_vertex = 0;
			seg->end_status = segment_vertex;
			CSG_Split_Edge_Edge_Vert(w, index, seg);
		}
		else
			CSG_Split_Vert_Edge_Edge(w, index, seg);
	}

	return TRUE;
}


static void
CSG_Split_Vert_Edge_Edge(CSGWireframePtr w, short index, CSGSegment *seg)
{

#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"V-E-E face %i vertex %d edge %d\n", index, seg->start_vertex,
		seg->end_vertex);
#endif

	CSG_Split_Insert_Vertex(w, index, seg->start_vertex, seg->end_point);

	seg->end_vertex = seg->start_vertex + 1;
	seg->end_status = segment_vertex;
	if ( seg->start_vertex == 0 )
		seg->start_vertex = w->faces[index].face_num_vertices - 1;
	else
		seg->start_vertex--;
	CSG_Split_Vert_Face_Vert(w, index, seg);
}


static void
CSG_Split_Edge_Edge_Vert(CSGWireframePtr w, short index, CSGSegment *seg)
{

#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"E-E-V face %i edge %d vertex %d\n", index, seg->start_vertex,
		seg->end_vertex);
#endif

	CSG_Split_Insert_Vertex(w, index, seg->start_vertex, seg->start_point);

	/* Modify the segment and pass it on to CSG_Vert_Face_Vert. */
	seg->start_status = segment_vertex;
	if ( seg->end_vertex > seg->start_vertex )
		seg->end_vertex++;
	seg->start_vertex++;
	if ( seg->end_vertex == w->faces[index].face_num_vertices - 1 )
		seg->end_vertex = 0;
	else
		seg->end_vertex++;
	CSG_Split_Vert_Face_Vert(w, index, seg);
}


Boolean
CSG_Vert_Face_Vert_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"V-F-V face %d vertices %d %d\n", index, seg->start_vertex,
		seg->end_vertex);
	fprintf(stderr,"Marking %d %d\n",
			w->faces[index].face_vertices[seg->start_vertex],
			w->faces[index].face_vertices[seg->end_vertex]);
#endif

	vertex(index, seg->start_vertex).status = vertex_boundary;
	vertex(index, seg->end_vertex).status = vertex_boundary;

	CSG_Split_Vert_Face_Vert(w, index, seg);

	return TRUE;
}


static void
CSG_Split_Vert_Face_Vert(CSGWireframePtr w, short index, CSGSegment *seg)
{
	short	start, end;
	int		i, j;

	/* Split the polygon in 2 across the vertices. */

	/* Order the vertices. */
	if ( seg->start_vertex > seg->end_vertex )
	{
		start = seg->end_vertex;
		end = seg->start_vertex;
	}
	else
	{
		start = seg->start_vertex;
		end = seg->end_vertex;
	}

	/* Check that they aren't neighbouring vertices. */
	if ( end == start ||
		 end == start + 1 ||
		 ( end == w->faces[index].face_num_vertices - 1 && start == 0 ) )
		return;

	/* Create a new face. */
	w->faces = More(w->faces, CSGFace, w->num_faces + 1);

	/* Store it's vertices. */
	w->faces[w->num_faces].face_num_vertices = end - start + 1;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
	w->faces[w->num_faces].face_vertices = New(int,
									w->faces[w->num_faces].face_num_vertices);
	for ( i = 0 ; i < w->faces[w->num_faces].face_num_vertices ; i++ )
		w->faces[w->num_faces].face_vertices[i] =
			w->faces[index].face_vertices[i + start];
	/* Calculate or store the other stuff. */
	CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
						  w->faces[w->num_faces].face_num_vertices,
						  &(w->faces[w->num_faces].face_extent));
	w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
	w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
	w->num_faces++;

	/* Truncate the old face. */
	for ( j = start + 1, i = end ;
		  i < w->faces[index].face_num_vertices ;
		  i++, j++ )
		w->faces[index].face_vertices[j] = w->faces[index].face_vertices[i];
	w->faces[index].face_num_vertices = j;
	CSG_Face_Bounding_Box(w->vertices, w->faces[index].face_vertices, 
						  w->faces[index].face_num_vertices,
						  &(w->faces[index].face_extent));
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Truncating face %d, verts %d\n", index,
			w->faces[index].face_num_vertices);
#endif

#if ( CSG_WIRE_DEBUG )
	/* Do a redundancy test. */
	if ( w->faces[index].face_vertices[0] ==
		 w->faces[index].face_vertices[w->faces[index].face_num_vertices-1] )
	{
		fprintf(stderr,"V-F-V: redundant vertex original face %d\n", index);
		for ( i = 0 ; i < w->faces[index].face_num_vertices ; i++ )
			fprintf(stderr, "%d ", w->faces[index].face_vertices[i]);
		fprintf(stderr, "\nstart %d end %d\n", start, end);
	}
	for ( i = 1 ; i < w->faces[index].face_num_vertices ; i++ )
		if ( w->faces[index].face_vertices[i] ==
			 w->faces[index].face_vertices[i-1])
		{
			fprintf(stderr, "V-F-V: redundant vertex original face %d\n",index);
			for ( i = 0 ; i < w->faces[index].face_num_vertices ; i++ )
				fprintf(stderr, "%d ", w->faces[index].face_vertices[i]);
			fprintf(stderr, "\nstart %d end %d\n", start, end);
		}

	if ( w->faces[w->num_faces - 1].face_vertices[0] ==
		 w->faces[w->num_faces - 1].
		 face_vertices[w->faces[w->num_faces-1].face_num_vertices-1] )
	{
		fprintf(stderr, "V-F-V: redundant vertex new face %d\n",w->num_faces-1);
		for ( i = 0 ; i < w->faces[w->num_faces-1].face_num_vertices ; i++ )
			fprintf(stderr, "%d ", w->faces[w->num_faces-1].face_vertices[i]);
		fprintf(stderr, "\nstart %d end %d\n", start, end);
	}
	for ( i = 1 ; i < w->faces[w->num_faces -1].face_num_vertices ; i++ )
		if ( w->faces[w->num_faces -1].face_vertices[i] ==
			 w->faces[w->num_faces-1].face_vertices[i-1])
		{
			fprintf(stderr, "V-F-V: redundant vertex new face %d\n",
				w->num_faces - 1);
			for ( i = 0 ; i < w->faces[w->num_faces-1].face_num_vertices ; i++ )
				fprintf(stderr,"%d ",w->faces[w->num_faces-1].face_vertices[i]);
			fprintf(stderr, "\nstart %d end %d\n", start, end);
		}
#endif
}


Boolean
CSG_Vert_Face_Edge_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{

#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"V-F-E face %d vert %d edge %d\n", index,
		seg->end_status == segment_vertex ? seg->end_vertex : seg->start_vertex,
		seg->end_status == segment_vertex ? seg->start_vertex :seg->end_vertex);
#endif


	/* Need a new vertex. */
	/* Insert it into the face's vertex list in the appropriate space. */
	if ( seg->start_status == segment_edge )
	{
		vertex(index, seg->end_vertex).status = vertex_boundary;
		CSG_Split_Insert_Vertex(w, index, seg->start_vertex, seg->start_point);

		seg->start_status = segment_vertex;
		if ( seg->end_vertex > seg->start_vertex )
			seg->end_vertex++;
		seg->start_vertex++;
		CSG_Split_Vert_Face_Vert(w, index, seg);
	}
	else
	{
		vertex(index, seg->start_vertex).status = vertex_boundary;
		CSG_Split_Insert_Vertex(w, index, seg->end_vertex, seg->end_point);

		seg->end_status = segment_vertex;
		if ( seg->start_vertex > seg->end_vertex )
			seg->start_vertex++;
		seg->end_vertex++;
		CSG_Split_Vert_Face_Vert(w, index, seg);
	}

	return TRUE;
}


Boolean
CSG_Vert_Face_Face_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
	if ( seg->start_status == segment_vertex )
	{
		if ( CSGVEqual(seg->end_point, vertex(index, seg->end_vertex).location,
					   diff))
		{
			seg->end_status = segment_vertex;
			CSG_Vert_Face_Vert_Split(w, index, seg);
			return TRUE;
		}

#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Marking %d\n",
			w->faces[index].face_vertices[seg->start_vertex]);
#endif
		vertex(index, seg->start_vertex).status = vertex_boundary;
		CSG_Split_Vertex_Face_Face(w, index, seg);
	}
	else
	{
		if ( CSGVEqual(seg->start_point,
					   vertex(index, seg->start_vertex).location, diff))
		{
			seg->start_status = segment_vertex;
			CSG_Vert_Face_Vert_Split(w, index, seg);
			return TRUE;
		}

#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Marking %d\n", w->faces[index].face_vertices[seg->end_vertex] - w->vertices);
#endif
		vertex(index, seg->end_vertex).status = vertex_boundary;
		CSG_Split_Face_Face_Vertex(w, index, seg);
	}

	return TRUE;
}


static void
CSG_Split_Vertex_Face_Face(CSGWireframePtr w, short index, CSGSegment *seg)
{
	int	new_index;
	int	start;
	int	i, j;

#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"V-F-F face %d vert %d end %d\n", index, seg->start_vertex,
		seg->end_vertex);
#endif


	new_index = CSG_Split_Add_Face_Vertex(w, index, seg->end_point);

	if ( seg->end_was_vert )
	{
		/* 4 way split. */
		int	end_next =
			seg->end_vertex == w->faces[index].face_num_vertices - 1 ?
			0 : seg->end_vertex + 1;
		int	end_pred = seg->end_vertex - 1;

		w->faces = More(w->faces, CSGFace, w->num_faces + 3);

		/* The first new face contains the new point, the end point and
		** the end's predecessor. */
		w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, 3);
		w->faces[w->num_faces].face_vertices[0] = new_index;
		w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[end_pred];
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[seg->end_vertex];
		/* Calculate or store the other stuff. */
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/*The second new face contains the new point, the end point and its
		** successor. */
		w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, 3);
		w->faces[w->num_faces].face_vertices[0] = new_index;
		w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[seg->end_vertex];
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[end_next];
		/* Calculate or store the other stuff. */
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/* The third new face contains all the original faces vertices from
		** end_pred to new through start and back to pred. */
		w->faces[w->num_faces].face_num_vertices =
			seg->end_vertex - seg->start_vertex + 1;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices =
			New(int, w->faces[w->num_faces].face_num_vertices);
		w->faces[w->num_faces].face_vertices[0] =
			w->faces[index].face_vertices[end_pred];
		w->faces[w->num_faces].face_vertices[1] = new_index;
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[seg->start_vertex];
		i = seg->start_vertex == w->faces[index].face_num_vertices - 1 ?
			0 : seg->start_vertex + 1;
		for (  j = 3 ; i != end_pred ;
				i = ( i == w->faces[index].face_num_vertices - 1 ? 0 : i + 1 ),
				j++ )
			w->faces[w->num_faces].face_vertices[j] =
				w->faces[index].face_vertices[i];
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/* The original face is modified so that it no longer contains
		** all the vertices from start through to end. */
		start = seg->start_vertex + 1;
		w->faces[index].face_vertices[start] = new_index;
		for ( j = start + 1, i = end_next ;
			  i > 0 && i < w->faces[index].face_num_vertices ;
			  i++, j++ )
			w->faces[index].face_vertices[j] = w->faces[index].face_vertices[i];
		w->faces[index].face_num_vertices = j;
		CSG_Face_Bounding_Box(w->vertices, w->faces[index].face_vertices, 
							  w->faces[index].face_num_vertices,
							  &(w->faces[index].face_extent));
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Truncating face %d, verts %d\n", index,
			w->faces[index].face_num_vertices);
#endif

	}
	else
	{
		/* 3 way split. */
		int	end_next =
			seg->end_vertex == w->faces[index].face_num_vertices - 1 ?
			0 : seg->end_vertex + 1;

		w->faces = More(w->faces, CSGFace, w->num_faces + 2);

		/* The first new face contains the new point, the end point and
		** the end's next. */
		w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, 3);
		w->faces[w->num_faces].face_vertices[0] = new_index;
		w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[seg->end_vertex];
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[end_next];
		/* Calculate or store the other stuff. */
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/* The second new face contains all the original faces vertices from
		** end through new through start and back to end. */
		w->faces[w->num_faces].face_num_vertices =
			seg->end_vertex - seg->start_vertex + 2;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices =
			New(int, w->faces[w->num_faces].face_num_vertices);
		w->faces[w->num_faces].face_vertices[0] =
			w->faces[index].face_vertices[seg->end_vertex];
		w->faces[w->num_faces].face_vertices[1] = new_index;
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[seg->start_vertex];
		i = seg->start_vertex + 1;
		for (  j = 3 ; i != seg->end_vertex ; i++, j++ )
			w->faces[w->num_faces].face_vertices[j] =
				w->faces[index].face_vertices[i];
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/* The original face is modified so that it no longer contains
		** all the vertices from start through to end. */
		start = seg->start_vertex + 1;
		w->faces[index].face_vertices[start] = new_index;
		for ( j = start + 1, i = end_next ;
			  i > 0 && i < w->faces[index].face_num_vertices ;
			  i++, j++ )
			w->faces[index].face_vertices[j] = w->faces[index].face_vertices[i];
		w->faces[index].face_num_vertices = j;
		CSG_Face_Bounding_Box(w->vertices, w->faces[index].face_vertices, 
							  w->faces[index].face_num_vertices,
							  &(w->faces[index].face_extent));
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Truncating face %d, verts %d\n", index,
			w->faces[index].face_num_vertices);
#endif

	}
}


static void
CSG_Split_Face_Face_Vertex(CSGWireframePtr w, short index, CSGSegment *seg)
{
	int	new_index;
	int	start_prev;
	int	start_next = seg->start_vertex + 1;
	int	i, j;

#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"F-F-V face %d end %d vert %d\n", index, seg->start_vertex,
		seg->end_vertex);
#endif


	new_index = CSG_Split_Add_Face_Vertex(w, index, seg->start_point);

	if ( seg->start_was_vert )
	{
		/* 4 way split. */
		start_prev = seg->start_vertex == 0 ?
					 w->faces[index].face_num_vertices - 1 :
					 seg->start_vertex - 1;

		w->faces = More(w->faces, CSGFace, w->num_faces + 3);

		/* The first new face contains start_prev, start, and new. */
		w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, 3);
		w->faces[w->num_faces].face_vertices[0] =
			w->faces[index].face_vertices[start_prev];
		w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[seg->start_vertex];
		w->faces[w->num_faces].face_vertices[2] = new_index;
		/* Calculate or store the other stuff. */
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/* The next contains all the vertices from new, start and start_next.*/
		w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, 3);
		w->faces[w->num_faces].face_vertices[0] = new_index;
		w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[seg->start_vertex];
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[start_next];
		/* Calculate or store the other stuff. */
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/* The third new face contains all the vertices from end through
		** new then start_next and back to end.
		*/
		w->faces[w->num_faces].face_num_vertices =
			seg->end_vertex - seg->start_vertex + 1;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices =
			New(int, w->faces[w->num_faces].face_num_vertices);
		w->faces[w->num_faces].face_vertices[0] =
			w->faces[index].face_vertices[seg->end_vertex];
		w->faces[w->num_faces].face_vertices[1] = new_index;
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[start_next];
		for ( j = 3, i = start_next + 1 ; i != seg->end_vertex ; i++, j++ )
			w->faces[w->num_faces].face_vertices[j] =
				w->faces[index].face_vertices[i];
		/* Calculate or store the other stuff. */
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/* Remove all the surplus vertices from the original face. */
		w->faces[index].face_vertices[seg->start_vertex] = new_index;
		for ( j = seg->start_vertex + 1, i = seg->end_vertex ;
			  i < w->faces[index].face_num_vertices ; i++, j++ )
			w->faces[index].face_vertices[j] =
				w->faces[index].face_vertices[i];
		w->faces[index].face_num_vertices = j;
		CSG_Face_Bounding_Box(w->vertices, w->faces[index].face_vertices,
							  w->faces[index].face_num_vertices,
							  &(w->faces[index].face_extent));
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Truncating face %d, verts %d\n", index,
			w->faces[index].face_num_vertices);
#endif

	}
	else
	{
		/* 3 way split. */
		w->faces = More(w->faces, CSGFace, w->num_faces + 2);

		/* The first new face contains start, start_next and new. */
		w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, 3);
		w->faces[w->num_faces].face_vertices[0] =
			w->faces[index].face_vertices[seg->start_vertex];
		w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[start_next];
		w->faces[w->num_faces].face_vertices[2] = new_index;
		/* Calculate or store the other stuff. */
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/* The next contains all the vertices from new to start_next and
		** around to end. */
		w->faces[w->num_faces].face_num_vertices =
			seg->end_vertex - seg->start_vertex + 1;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices =
			New(int, w->faces[w->num_faces].face_num_vertices);
		w->faces[w->num_faces].face_vertices[0] =
			w->faces[index].face_vertices[seg->end_vertex];
		w->faces[w->num_faces].face_vertices[1] = new_index;
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[start_next];
		for ( j = 3, i = start_next + 1 ; i != seg->end_vertex ; i++, j++ )
			w->faces[w->num_faces].face_vertices[j] =
				w->faces[index].face_vertices[i];
		/* Calculate or store the other stuff. */
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/* Remove all the surplus vertices from the original face. */
		w->faces[index].face_vertices[start_next] = new_index;
		for ( j = start_next + 1, i = seg->end_vertex ;
			  i < w->faces[index].face_num_vertices ; i++, j++ )
			w->faces[index].face_vertices[j] =
				w->faces[index].face_vertices[i];
		w->faces[index].face_num_vertices = j;
		CSG_Face_Bounding_Box(w->vertices, w->faces[index].face_vertices,
							  w->faces[index].face_num_vertices,
							  &(w->faces[index].face_extent));
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Truncating face %d, verts %d\n", index,
			w->faces[index].face_num_vertices);
#endif

	}
}



Boolean
CSG_Edge_Edge_Edge_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
	int	start;
	int	first;
	int	second;
	int	i;

#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"E-E-E face %d edges %d %d\n", index, seg->start_vertex,
		seg->end_vertex);
#endif

	if ( seg->start_vertex == 0 &&
		 seg->end_vertex == w->faces[index].face_num_vertices - 1 )
	{
		Vector	temp_pt;

		/* Swap them around. */
		seg->start_vertex = seg->end_vertex;
		seg->end_vertex = 0;
		temp_pt = seg->end_point;
		seg->start_point = seg->end_point;
		seg->end_point = temp_pt;
	}

	if ( CSGVEqual(seg->start_point, seg->end_point, diff) )
	{
		/* The 2 endpoints are identical, so modify the segment a bit
		** and call the vert-edge-edge routines.
		*/
		seg->start_status = segment_vertex;
		seg->start_point = vertex(index, seg->start_vertex).location;
		CSG_Split_Vert_Edge_Edge(w, index, seg);

		return TRUE;
	}

	/* Add 2 new vertices. */
	if ( ! CSG_Split_Insert_Vertex(w, index, seg->start_vertex,
								   seg->start_point) )
	{
		seg->start_status = segment_vertex;
		seg->start_point = vertex(index, seg->start_vertex).location;
		CSG_Split_Vert_Edge_Edge(w, index, seg);
		return TRUE;
	}
	if ( ! CSG_Split_Insert_Vertex(w, index, seg->start_vertex + 1,
								   seg->end_point) )
	{
		seg->end_point = seg->start_point;
		seg->start_status = segment_vertex;
		seg->start_point = vertex(index, seg->start_vertex).location;
		CSG_Split_Vert_Edge_Edge(w, index, seg);
		return TRUE;
	}

	start = seg->start_vertex == 0 ?
			w->faces[index].face_num_vertices - 1 : seg->start_vertex - 1;
	first = seg->start_vertex + 1;
	second = seg->start_vertex + 2;

	/* Create the first new face. */
	w->faces = More(w->faces, CSGFace, w->num_faces + 2);
	/* Store it's vertices. */
	w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
	w->faces[w->num_faces].face_vertices = New(int, 3);
	w->faces[w->num_faces].face_vertices[0] =
			w->faces[index].face_vertices[start];
	w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[seg->start_vertex];
	w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[first];
	/* Calculate or store the other stuff. */
	CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
						  w->faces[w->num_faces].face_num_vertices,
						  &(w->faces[w->num_faces].face_extent));
	w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
	w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
	w->num_faces++;

	/* Create the second new face. */
	/* Store it's vertices. */
	w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
	w->faces[w->num_faces].face_vertices = New(int, 3);
	w->faces[w->num_faces].face_vertices[0] =
			w->faces[index].face_vertices[start];
	w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[first];
	w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[second];
	/* Calculate or store the other stuff. */
	CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
						  w->faces[w->num_faces].face_num_vertices,
						  &(w->faces[w->num_faces].face_extent));
	w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
	w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
	w->num_faces++;

	/* Delete the start and first vertices from the original face. */
	for ( i = second ; i < w->faces[index].face_num_vertices ; i++ )
		w->faces[index].face_vertices[i - 2] = w->faces[index].face_vertices[i];
	w->faces[index].face_num_vertices -= 2;
	CSG_Face_Bounding_Box(w->vertices, w->faces[index].face_vertices, 
						  w->faces[index].face_num_vertices,
						  &(w->faces[index].face_extent));
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Truncating face %d, verts %d\n", index,
			w->faces[index].face_num_vertices);
#endif


	return TRUE;
}


Boolean
CSG_Edge_Face_Edge_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"E-F-E face %d edges %d %d\n", index, seg->start_vertex,
		seg->end_vertex);
#endif


	/* Add 2 new vertices, then do Vert Face Vert. */
	CSG_Split_Insert_Vertex(w, index, seg->start_vertex, seg->start_point);
	seg->start_status = segment_vertex;
	if ( seg->end_vertex > seg->start_vertex )
		seg->end_vertex++;
	seg->start_vertex++;

	CSG_Split_Insert_Vertex(w, index, seg->end_vertex, seg->end_point);
	seg->end_status = segment_vertex;
	if ( seg->start_vertex > seg->end_vertex )
		seg->start_vertex++;
	seg->end_vertex++;

	CSG_Split_Vert_Face_Vert(w, index, seg);

	return TRUE;
}


Boolean
CSG_Edge_Face_Face_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
	/* The general approach is to add a new vertex on the edge then
	** call the Vertex face face routines.
	*/
	if ( seg->start_status == segment_edge )
	{
		CSG_Split_Insert_Vertex(w, index, seg->start_vertex, seg->start_point);
		seg->start_vertex++;
		seg->end_vertex++;
		seg->start_status = segment_vertex;

		CSG_Split_Vertex_Face_Face(w, index, seg);
	}
	else
	{
		CSG_Split_Insert_Vertex(w, index, seg->end_vertex, seg->end_point);
		seg->end_vertex++;
		seg->end_status = segment_vertex;

		CSG_Split_Face_Face_Vertex(w, index, seg);
	}

	return TRUE;
}


Boolean
CSG_Face_Face_Face_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
	int	new_1, new_2;
	int	start;
	int	start_next;
	int	end;
	int	end_next;
	int	middle_num;
	int	i, j;

#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"F-F-F face %d start %d end %d\n", index, seg->start_vertex,
			seg->end_vertex);
#endif

	if ( CSGVEqual(seg->start_point, seg->end_point, diff) )
	{
		new_1 = CSG_Split_Add_Face_Vertex(w, index, seg->start_point);
		new_2 = new_1;
	}
	else
	{
		new_1 = CSG_Split_Add_Face_Vertex(w, index, seg->start_point);
		new_2 = CSG_Split_Add_Face_Vertex(w, index, seg->end_point);
	}

	start_next = seg->start_vertex + 1;
	end_next = seg->end_vertex + 1;

	/* Allocate the right number of extra faces. */
	if ( seg->start_was_vert )
		if ( seg->end_was_vert )
			w->faces = More(w->faces, CSGFace, w->num_faces + 5);
		else
			w->faces = More(w->faces, CSGFace, w->num_faces + 4);
	else
		if ( seg->end_was_vert )
			w->faces = More(w->faces, CSGFace, w->num_faces + 4);
		else
			w->faces = More(w->faces, CSGFace, w->num_faces + 3);

	if ( seg->start_was_vert )
	{
		start = seg->start_vertex - 1;

		/* 2 new faces. */
		/* The first from new_1 to start to seg->start_vertex. */
		w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, 3);
		w->faces[w->num_faces].face_vertices[0] = new_1;
		w->faces[w->num_faces].face_vertices[1] = start == -1 ?
			w->faces[index].face_vertices[w->faces[index].face_num_vertices-1] :
			w->faces[index].face_vertices[start];
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[seg->start_vertex];
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;

		/* The second from new_1 to seg->start_vertex to start_next. */
		w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, 3);
		w->faces[w->num_faces].face_vertices[0] = new_1;
		w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[seg->start_vertex];
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[start_next];
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;
	}
	else
	{
		start = seg->start_vertex;

		/* 1 new face from new_1 to start to start_next. */
		w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, 3);
		w->faces[w->num_faces].face_vertices[0] = new_1;
		w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[start];
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[start_next];
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;
	}

	if ( seg->end_was_vert )
	{
		end = seg->end_vertex - 1;

		/* 2 more new faces. */
		/* The first from new_2 to end to seg->end_vertex. */
		w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, 3);
		w->faces[w->num_faces].face_vertices[0] = new_2;
		w->faces[w->num_faces].face_vertices[1] =
			w->faces[index].face_vertices[end];
		w->faces[w->num_faces].face_vertices[2] =
			w->faces[index].face_vertices[seg->end_vertex];
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;
	}
	else
		end = seg->end_vertex;

	/* Another from new_2 to seg->end_vertex to end_next. */
	w->faces[w->num_faces].face_num_vertices = 3;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
	w->faces[w->num_faces].face_vertices = New(int, 3);
	w->faces[w->num_faces].face_vertices[0] = new_2;
	w->faces[w->num_faces].face_vertices[1] =
		w->faces[index].face_vertices[seg->end_vertex];
	w->faces[w->num_faces].face_vertices[2] =
		end_next == w->faces[index].face_num_vertices ?
		w->faces[index].face_vertices[0] :
		w->faces[index].face_vertices[end_next];
	CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
						  w->faces[w->num_faces].face_num_vertices,
						  &(w->faces[w->num_faces].face_extent));
	w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
	w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
	w->num_faces++;

	/* Create the middle face. */
	/* If new_1 and new_2 are the same it may not exist */
	if ( new_1 != new_2 || end != start_next )
	{
		middle_num = end - start_next + ( new_1 == new_2 ? 2 : 3 );
		w->faces[w->num_faces].face_num_vertices = middle_num;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Creating face %d, %d verts\n", w->num_faces,
			w->faces[w->num_faces].face_num_vertices);
#endif
		w->faces[w->num_faces].face_vertices = New(int, middle_num);
		j = 0;
		w->faces[w->num_faces].face_vertices[j++] = new_2;
		if ( new_1 != new_2 )
			w->faces[w->num_faces].face_vertices[j++] = new_1;
		for ( i = start_next ; i <= end ; i++ )
			w->faces[w->num_faces].face_vertices[j++] =
				w->faces[index].face_vertices[i];
		CSG_Face_Bounding_Box(w->vertices, w->faces[w->num_faces].face_vertices,
							  w->faces[w->num_faces].face_num_vertices,
							  &(w->faces[w->num_faces].face_extent));
		w->faces[w->num_faces].face_plane = w->faces[index].face_plane;
		w->faces[w->num_faces].face_attribs = w->faces[index].face_attribs;
		w->num_faces++;
	}

	/* Update the original face. */
	/* If new_1 == new_2 it may need deleting. */
	if ( new_1 == new_2 &&
		 ( start == end_next ||
		  (start == -1 && end_next == w->faces[index].face_num_vertices - 1 ) ||
		  (start == 0 && end_next == w->faces[index].face_num_vertices ) ) )
	{
		/* Delete it. */
		free(w->faces[index].face_vertices);
		for ( i = index + 1 ; i < w->num_faces ; i++ )
			w->faces[i - 1] = w->faces[i];
		w->num_faces--;
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Deleting face %d\n", index);
#endif
		return FALSE;
	}
	else
	{
		if ( end_next - start < ( new_1 == new_2 ? 2 : 3 ) )
		{
			/* Need an extra vertex. */
			w->faces[index].face_num_vertices++;
			w->faces[index].face_vertices = More(w->faces[index].face_vertices,
							int, w->faces[index].face_num_vertices);
			for ( i = w->faces[index].face_num_vertices - 1 ; i > end_next ;i--)
				w->faces[index].face_vertices[i] =
					w->faces[index].face_vertices[i-1];
			end_next++;
		}
		j = start + 1;
		w->faces[index].face_vertices[j++] = new_1;
		if ( new_1 != new_2 )
			w->faces[index].face_vertices[j++] = new_2;
		for ( i = end_next ; i < w->faces[index].face_num_vertices ; i++ )
			w->faces[index].face_vertices[j++] =
				w->faces[index].face_vertices[i];
		w->faces[index].face_num_vertices = j;
		CSG_Face_Bounding_Box(w->vertices, w->faces[index].face_vertices,
							  w->faces[index].face_num_vertices,
							  &(w->faces[index].face_extent));
#if ( CSG_WIRE_DEBUG )
	fprintf(stderr,"Truncating face %d, verts %d\n", index,
			w->faces[index].face_num_vertices);
#endif

		return TRUE;
	}
}


/*	int
**	CSG_Split_Add_Vertex(CSGWireframePtr wire, Vector location)
**	Adds a new vertex to the wireframe, updating pointers as it goes.
**	Returns the new vertex.
*/
static int
CSG_Split_Add_Vertex(CSGWireframePtr wire, Vector location)
{
	int	i;

	/* Search through for an identical vertex. */
	for ( i = 0 ; i < wire->num_vertices ; i++ )
		if ( CSGVEqual(location, wire->vertices[i].location, diff) )
			return i;

	/* No identical. Need a new one. */
	if ( wire->num_vertices == wire->max_vertices )
	{
		wire->max_vertices += 25;
		wire->vertices = More(wire->vertices, CSGVertex, wire->max_vertices);
	}

	wire->vertices[wire->num_vertices].location = location;
	wire->vertices[wire->num_vertices].status = vertex_unknown;
	wire->vertices[wire->num_vertices].num_adjacent = 0;
	wire->vertices[wire->num_vertices].max_num_adjacent = 0;
	wire->num_vertices++;

	return wire->num_vertices - 1;
}



/*	int
**	CSG_Split_Insert_Vertex(CSGWireframePtr w, int f,short pred,Vector location)
**	Creates and inserts a new vertex at location into the vertex list
**	of face,where it will appear in order after pred.
*/
static int
CSG_Split_Insert_Vertex(CSGWireframePtr w, int f, short pred, Vector location) 
{
	int	ret;
	int	i;

	ret = CSG_Split_Add_Vertex(w, location);

	w->faces[f].face_vertices = More(w->faces[f].face_vertices, int,
									 w->faces[f].face_num_vertices + 1);

	for ( i = w->faces[f].face_num_vertices - 1 ; i > pred ; i-- )
		w->faces[f].face_vertices[i+1] = w->faces[f].face_vertices[i];
	w->faces[f].face_vertices[pred+1] = ret;
	w->faces[f].face_num_vertices++;

	return ret;
}


static int
CSG_Split_Add_Face_Vertex(CSGWireframePtr w, int f, Vector location)
{
	Vector	temp_v;
	double	dist;

	/* Project location onto the plane. */
	VSub(location, w->faces[f].face_plane.f_point, temp_v);
	dist = VDot(w->faces[f].face_plane.f_vector, temp_v);
	VScalarMul(w->faces[f].face_plane.f_vector, dist, temp_v);
	VSub(location, temp_v, location);

	return CSG_Split_Add_Vertex(w, location);
}

