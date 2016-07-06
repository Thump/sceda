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
#include <sced.h>
#include <csg.h>
#include <csg_wire.h>
#include <hash.h>

/*	Type declarations for adjacency list edge representation.
*/
typedef struct _EdgeEntry {
	int		endpoint;
	int		edge_num;
	int		*faces;
	short	num_faces;
	short	max_faces;
	} EdgeEntry, *EdgeEntryPtr;

typedef struct _EdgeTableEntry {
	EdgeEntryPtr	edges;
	int				num_edges;
	int				max_edges;
	} EdgeTableEntry, *EdgeTable;

static void			Wireframe_Merge_Faces(WireframePtr, EdgeTable);
static void			Wireframe_Remove_Colinear_Edges(WireframePtr);
static void			Wireframe_Remove_Vertices(WireframePtr);


WireframePtr
Wireframe_Copy(WireframePtr src)
{
	WireframePtr	result = New(Wireframe, 1);
	int	i, j;

	result->num_vertices = src->num_vertices;
	result->vertices = New(Vector, result->num_vertices);
	for ( i = 0 ; i < result->num_vertices ; i++ )
		result->vertices[i] = src->vertices[i];

	result->num_faces = src->num_faces;
	result->faces = New(Face, src->num_faces);
	for ( i = 0 ; i < result->num_faces ; i++ )
	{
		result->faces[i].num_vertices = src->faces[i].num_vertices;
		result->faces[i].vertices = New(int, result->faces[i].num_vertices);
		for ( j = 0 ; j < result->faces[i].num_vertices ; j++ )
			result->faces[i].vertices[j] = src->faces[i].vertices[j];
		result->faces[i].normal = src->faces[i].normal;
		result->faces[i].face_attribs = src->faces[i].face_attribs;
	}

	if ( src->vertex_normals )
	{
		result->vertex_normals = New(Vector, result->num_vertices);
		for ( i = 0 ; i < result->num_vertices ; i++ )
			result->vertex_normals[i] = src->vertex_normals[i];
	}
	else
		result->vertex_normals = NULL;

	if ( src->num_attribs )
	{
		result->num_attribs = src->num_attribs;
		result->attribs = New(AttributePtr, src->num_attribs);
		for ( i = 0 ; i < result->num_attribs ; i++ )
		{
			result->attribs[i] = New(Attributes, 1);
			*(result->attribs[i]) = *(src->attribs[i]);
		}
		/* Reset all the face ptrs for the new attributes list. */
		for ( i = 0 ; i < result->num_faces ; i++ )
		{
			for ( j = 0 ;
				  result->faces[i].face_attribs != src->attribs[j] ;
				  j++ );
			result->faces[i].face_attribs = result->attribs[j];
		}
	}
	else
	{
		result->attribs = NULL;
		result->num_attribs = 0;
	}

	return result;
}


void
Wireframe_Destroy(WireframePtr victim)
{
	int	i;

	free(victim->vertices);
	if ( victim->attribs )
	{
		for ( i = 0 ; i < victim->num_attribs ; i++ )
			free(victim->attribs[i]);
		free(victim->attribs);
	}
	free(victim->faces);
	if ( victim->vertex_normals )
		free(victim->vertex_normals);
	free(victim);
}


CSGWireframePtr
Wireframe_To_CSG(WireframePtr src, Boolean do_planes)
{
	CSGWireframePtr	result = New(CSGWireframe, 1);
	int	i, j;

	result->num_vertices = result->max_vertices = src->num_vertices;
	result->vertices = New(CSGVertex, result->num_vertices);
	for ( i = 0 ; i < result->num_vertices ; i++ )
	{
		result->vertices[i].location = src->vertices[i];
		result->vertices[i].num_adjacent =
		result->vertices[i].max_num_adjacent = 0;
		result->vertices[i].adjacent = NULL;
		result->vertices[i].status = vertex_unknown;
	}

	result->num_faces = src->num_faces;
	result->faces = New(CSGFace, src->num_faces);
	for ( i = 0 ; i < result->num_faces ; i++ )
	{
		result->faces[i].face_num_vertices = src->faces[i].num_vertices;
		result->faces[i].face_vertices = New(int,
										result->faces[i].face_num_vertices);
		for ( j = 0 ; j < result->faces[i].face_num_vertices ; j++ )
			result->faces[i].face_vertices[j] = src->faces[i].vertices[j];
		result->faces[i].face_plane.f_vector = src->faces[i].normal;
		if ( do_planes )
			result->faces[i].face_plane.f_point =
				result->vertices[result->faces[i].face_vertices[0]].location;
		result->faces[i].face_attribs = src->faces[i].face_attribs;
	}

	return result;
}


WireframePtr
CSG_To_Wireframe(CSGWireframePtr src)
{
	WireframePtr	result = New(Wireframe, 1);
	int	i, j;

	result->num_vertices = src->num_vertices;
	result->vertices = New(Vector, result->num_vertices);
	for ( i = 0 ; i < result->num_vertices ; i++ )
		result->vertices[i] = src->vertices[i].location;

	result->num_faces = src->num_faces;
	result->faces = New(Face, src->num_faces);
	for ( i = 0 ; i < result->num_faces ; i++ )
	{
		result->faces[i].num_vertices = src->faces[i].face_num_vertices;
		result->faces[i].vertices = New(int, result->faces[i].num_vertices);
		for ( j = 0 ; j < result->faces[i].num_vertices ; j++ )
			result->faces[i].vertices[j] = src->faces[i].face_vertices[j];
		result->faces[i].normal = src->faces[i].face_plane.f_vector;
		result->faces[i].face_attribs = src->faces[i].face_attribs;
	}

	result->vertex_normals = NULL;
	result->attribs = NULL;
	result->num_attribs = 0;

	return result;
}


/*	Construct a wireframe for a given object. The wireframe is
**	transformed appropriately for the object.
**	If copy_attribs is TRUE, new attribute structures are allocated as
**	required, otherwise the pointers only are copied.
*/
WireframePtr
Object_To_Wireframe(ObjectInstancePtr obj, Boolean copy_attribs)
{
	HashTable		attrib_hash;
	WireframePtr	res;
	double			temp_d;
	Vector			temp_v;
	Matrix			transp;
	int				i;
	int				max_attribs;

	res = Wireframe_Copy(obj->o_wireframe);

	/* Transform vertices. */
	for ( i = 0 ; i < res->num_vertices ; i++ )
	{
		MVMul(obj->o_transform.matrix, res->vertices[i], temp_v);
		VAdd(temp_v, obj->o_transform.displacement, res->vertices[i]);
	}

	/* Transform normals. */
	MTrans(obj->o_inverse.matrix, transp);
	for ( i = 0 ; i < res->num_faces ; i++ )
	{
		if ( VZero(res->faces[i].normal) )
			continue;
		MVMul(transp, res->faces[i].normal, temp_v);
		VUnit(temp_v, temp_d, res->faces[i].normal);
	}
	if ( res->vertex_normals )
	{
		for ( i = 0 ; i < res->num_vertices ; i++ )
		{
			MVMul(transp, res->vertex_normals[i], temp_v);
			VUnit(temp_v, temp_d, res->vertex_normals[i]);
		}
	}

	if ( copy_attribs )
	{
		attrib_hash = Hash_New_Table();
		res->num_attribs = max_attribs = 0;
		for ( i = 0 ; i < res->num_faces ; i++ )
			if ( Hash_Get_Value(attrib_hash,
				 (long)(res->faces[i].face_attribs)) == (void*)-1 )
			{
				Hash_Insert(attrib_hash, (long)(res->faces[i].face_attribs),
							(void*)(res->faces[i].face_attribs));
				if ( res->num_attribs == max_attribs )
				{
					if ( max_attribs )
						res->attribs = More(res->attribs, AttributePtr,
											max_attribs + 3);
					else
						res->attribs = New(AttributePtr, 3);
					max_attribs += 3;
				}
				res->attribs[res->num_attribs] = New(Attributes, 1);
				*(res->attribs[res->num_attribs]) =
					*(res->faces[i].face_attribs);
				res->num_attribs++;
			}
	}

	return res;
}


static EdgeTable
Wireframe_Edge_Table_Create(int num)
{
	EdgeTable	res = New(EdgeTableEntry, num);
	int			i;

	for ( i = 0 ; i < num ; i++ )
	{
		res[i].edges = NULL;
		res[i].num_edges = res[i].max_edges = 0;
	}

	return res;
}


static EdgeEntryPtr
Wireframe_Edge_Present(EdgeTable table, int start, int end)
{
	int	i;

	for ( i = 0 ;
		  i < table[start].num_edges && table[start].edges[i].endpoint != end ;
		  i++ );

	if ( i == table[start].num_edges )
		return NULL;
	else
		return table[start].edges + i;
}

static int
Wireframe_Edge_Add_Edge(EdgeTable table, int *num, int start, int end,
						int face_num)
{
	EdgeEntryPtr	edge;
	int				temp_i;

	if ( start > end )
	{
		temp_i = start;
		start = end;
		end = temp_i;
	}

	/* Find the edge, or make space for a new one. */
	if ( ! ( edge = Wireframe_Edge_Present(table, start, end) ) )
	{
		if ( table[start].num_edges == table[start].max_edges )
		{
			if ( table[start].max_edges )
				table[start].edges = More(table[start].edges, EdgeEntry,
										  table[start].max_edges + 2);
			else
				table[start].edges = New(EdgeEntry, 2);
			table[start].max_edges += 2;
		}
		table[start].edges[table[start].num_edges].endpoint = end;
		table[start].edges[table[start].num_edges].edge_num = (*num)++;
		table[start].edges[table[start].num_edges].num_faces =
		table[start].edges[table[start].num_edges].max_faces = 0;
		edge = table[start].edges + table[start].num_edges;
		table[start].num_edges++;
	}

	if ( face_num == -1 )
		return edge->edge_num;

	/* Add the face. */
	if ( edge->num_faces == edge->max_faces )
	{
		if ( edge->max_faces )
			edge->faces = More(edge->faces, int, edge->max_faces + 2);
		else
			edge->faces = New(int, 2);
		edge->max_faces += 2;
	}
	edge->faces[edge->num_faces++] = face_num;

	return edge->edge_num;
}


static void
Wireframe_Edge_Free_Table(EdgeTable table, int size)
{
	int	i, j;

	for ( i = 0 ; i < size ; i++ )
		if ( table[i].max_edges )
		{
			for ( j = 0 ; j < table[i].num_edges ; j++ )
				if ( table[i].edges[j].num_faces )
					free(table[i].edges[j].faces);
			free(table[i].edges);
		}

	free(table);
}



WireframePtr
Wireframe_Simplify(WireframePtr src)
{
	WireframePtr	res;
	EdgeTable		edge_table;
	int				next_edge;
	int				i, j;

	/* Take a copy of src. */
	res = Wireframe_Copy(src);

	/* Build an edge table. */
	edge_table = Wireframe_Edge_Table_Create(res->num_vertices);
	next_edge = 0;
	for ( i = 0 ; i < res->num_faces ; i++ )
	{
		Wireframe_Edge_Add_Edge(edge_table, &next_edge,
					  res->faces[i].vertices[0],
					  res->faces[i].vertices[res->faces[i].num_vertices-1],
					  i);
		for ( j = 1 ; j < res->faces[i].num_vertices ; j++ )
			Wireframe_Edge_Add_Edge(edge_table, &next_edge,
									res->faces[i].vertices[j-1],
						  			res->faces[i].vertices[j], i);
	}

	/* Merge faces. */
	Wireframe_Merge_Faces(res, edge_table);

	/* Get rid of excess edges.*/
	Wireframe_Remove_Colinear_Edges(res);

	/* Remove surplus vertices. */
	Wireframe_Remove_Vertices(res);

	/* Free the edge table. */
	Wireframe_Edge_Free_Table(edge_table, src->num_vertices);

	return res;
}


static void
Wireframe_Merge_Face(WireframePtr src, FacePtr f1, FacePtr f2, int edge_start,
					 int edge_end)
{
	int	index;
	int	old_num;
	int	i, j;
	int	f1_start, f1_end, f2_start, f2_end;

	f1_start = f1_end = f2_start = f2_end = -1;

	/* Find the indices for the vertices involved. */
	for ( i = 0 ; i < f1->num_vertices ; i++ )
	{
		index = f1->vertices[i];
		if ( index == edge_start || index == edge_end )
		{
			f1_start = i;
			if ( i )
				f1_end = i + 1;
			else
			{
				index = f1->vertices[i+1];
				if ( index == edge_start || index == edge_end )
					f1_end = i + 1;
				else
				{
					f1_start = f1->num_vertices - 1;
					f1_end = 0;
				}
			}
			if ( ( edge_start == f1->vertices[f1_start] &&
				   edge_end == f1->vertices[f1_end] ) ||
				 ( edge_start == f1->vertices[f1_end] &&
				   edge_end == f1->vertices[f1_start] ) )
				break;
		}
	}
	for ( i = 0 ; i < f2->num_vertices ; i++ )
	{
		index = f2->vertices[i];
		if ( index == edge_start || index == edge_end )
		{
			f2_start = i;
			if ( i )
				f2_end = i + 1;
			else
			{
				index = f2->vertices[i+1];
				if ( index == edge_start || index == edge_end )
					f2_end = i + 1;
				else
				{
					f2_start = f2->num_vertices - 1;
					f2_end = 0;
				}
			}
			if ( ( edge_start == f2->vertices[f2_start] &&
				   edge_end == f2->vertices[f2_end] ) ||
				 ( edge_start == f2->vertices[f2_end] &&
				   edge_end == f2->vertices[f2_start] ) )
				break;
		}
	}

	if ( f1_start == -1 || f1_end == -1 || f2_start == -1 || f2_end == -1 )
		abort();

	/* f1 now inherits f2's vertices. */
	old_num = f1->num_vertices;
	f1->num_vertices += f2->num_vertices - 2;
	f1->vertices = More(f1->vertices, int, f1->num_vertices);

	/* Move up f1's vertices that occur after and including f1_end. */
	for ( i = old_num + f2->num_vertices - 3, j = old_num - 1 ;
		  f1_end != 0 && j >= f1_end ;
		  j--, i-- )
		f1->vertices[i] = f1->vertices[j];

	/* Copy f2's vertices over. */
	i = f1_start + 1;
	j = ( f2_end == f2->num_vertices - 1 ? 0 : f2_end + 1 );
	while ( j != f2_start )
	{
		f1->vertices[i] = f2->vertices[j];
		i++;
		j = ( j == f2->num_vertices - 1 ? 0 : j + 1 );
	}

	/* free any f2 memory. */
	free(f2->vertices);
	f2->num_vertices = 0;
}


static Boolean
Wireframe_Remove_Internal_Edge(FacePtr face)
{
	/* We know this face intersects itself along an edge.
	** This edge is either on the path into a hole in the face,
	** or a removable internal line. We seek to detect the later case,
	** and remove the internal line.
	** There is no guarantee that an internal line exists, so be
	** prepared to fail and return having done nothing.
	*/

	int	start, end;
	int	nexts, nexte;
	int	i, j;

	/* Look for a triple of the form a-b-a */
	if ( face->vertices[face->num_vertices - 2] == face->vertices[0] )
	{
		start = face->num_vertices - 2;
		end = 0;
	}
	else if ( face->vertices[face->num_vertices - 1] == face->vertices[1] )
	{
		start = face->num_vertices - 1;
		end = 1;
	}
	else
	{
		for ( start = 0, end = 2 ;
			  end < face->num_vertices &&
			  face->vertices[start] != face->vertices[end] ;
			  start++, end++ );
		if ( end >= face->num_vertices )
			return FALSE; /* No internal line. */
	}

	/* Have a triple.
	** The idea now is to find its maximum extent. That is, the maximum
	** length palindrome.
	*/
	nexts = start == 0 ? face->num_vertices - 1 : start - 1;
	nexte = end == face->num_vertices - 1 ? 0 : end + 1;
	while ( face->vertices[nexts] == face->vertices[nexte] )
	{
		start = nexts;
		end = nexte;
		nexts = start == 0 ? face->num_vertices - 1 : start - 1;
		nexte = end == face->num_vertices - 1 ? 0 : end + 1;
	}

	/* DEBUG */
	/*
	printf("%d:", face->num_vertices);
	for ( nexts = 0 ; nexts < face->num_vertices ; nexts++ )
		printf(" %d", face->vertices[nexts]);
	printf(" : %d %d\n", start, end);
	*/

	/* Remove the palindrome. */
	if ( start > end )
	{
		/* The palindrome wraps around the end of the array. */
		/* Work in 2 stages. */
		/* Chop off the end of the array and hack start and end so the
		** next bit works. */
		face->num_vertices = start + 1;
		start = -1;
	}
	for ( i = start + 1, j = end + 1 ;
		  j < face->num_vertices ;
		  i++, j++ )
		face->vertices[i] = face->vertices[j];
	face->num_vertices -= ( end - start );

	/*
	printf("%d:", face->num_vertices);
	for ( nexts = 0 ; nexts < face->num_vertices ; nexts++ )
		printf(" %d", face->vertices[nexts]);
	printf("\n\n");
	*/

	return TRUE;
}



static void
Wireframe_Merge_Faces(WireframePtr src, EdgeTable edge_table)
{
	int		*face_map;
	int		i, j;
	int		end;
	int		num_faces;
	int		f1_index, f2_index;
	FacePtr	f1, f2;
	Vector	diff;

	/* Need a face table, mapping faces to new faces, so to speak. */
	/* Each face starts as itself. */
	face_map = New(int, src->num_faces);
	for ( i = 0 ; i < src->num_faces ; i++ )
		face_map[i] = i;

	/* Work through all the edges. */
	for ( i = 0 ; i < src->num_vertices ; i++ )
		for ( j = 0 ; j < edge_table[i].num_edges ; j++ )
		{
			/* Only interested in edges with 2 adjacent faces.
			** Other edges must mark angles. */
			if ( edge_table[i].edges[j].num_faces != 2 )
				continue;

			end = edge_table[i].edges[j].endpoint;
			for ( f1_index = edge_table[i].edges[j].faces[0] ;
				  face_map[f1_index] != f1_index ;
				  f1_index = face_map[f1_index] );
			for ( f2_index = edge_table[i].edges[j].faces[1] ;
				  face_map[f2_index] != f2_index ;
				  f2_index = face_map[f2_index] );
			f1 = src->faces + f1_index;
			f2 = src->faces + f2_index;

			if ( f1 == f2 )
				/* Previously found to be coplanar and merged.	*/
				Wireframe_Remove_Internal_Edge(f1);
			else if ( VEqual(f1->normal, f2->normal, diff) )
			{
				/* Must be co-planar - common edge and equal normals.
				** Merge the faces.
				*/
				/* Check for compatable attributes. */
				if ( f1->face_attribs != f2->face_attribs &&
					 ( ( f1->face_attribs && f1->face_attribs->defined ) ||
					   ( f2->face_attribs && f2->face_attribs->defined ) ) )
					continue;

				Wireframe_Merge_Face(src, f1, f2, i, end);
				face_map[f2_index] = f1_index;
			}
		}

	for ( num_faces = 0, i = 0 ; i < src->num_faces ; i++ )
	{
		if ( face_map[i] == i )
		{
			if ( num_faces == i )
				num_faces++;
			else
				src->faces[num_faces++] = src->faces[i];
		}
	}
	src->num_faces = num_faces;
	/* reallocate to free any extra memory used by deleted faces. */
	src->faces = More(src->faces, Face, src->num_faces);
}


static void
Wireframe_Remove_Colinear_Edges(WireframePtr src)
{
	FacePtr	f;
	Boolean	changed;
	int		i, j, k;

	for ( i = 0 ; i < src->num_faces ; i++ )
	{
		changed = FALSE;
		f = src->faces + i;
		for ( j = 0 ; f->num_vertices > 3 && j < f->num_vertices - 2 ; )
		{
			if (  Points_Colinear(src->vertices[f->vertices[j]],
								  src->vertices[f->vertices[j+1]],
								  src->vertices[f->vertices[j+2]]) )
			{
				for ( k = j + 2 ; k < f->num_vertices ; k++ )
					f->vertices[k-1] = f->vertices[k];
				f->num_vertices--;
				changed = TRUE;
			}
			else
				j++;
		}
		while ( f->num_vertices > 3 &&
				Points_Colinear(src->vertices[f->vertices[0]],
							src->vertices[f->vertices[f->num_vertices - 2]],
							src->vertices[f->vertices[f->num_vertices - 1]]))
		{
			f->num_vertices--;
			changed = TRUE;
		}
		if ( f->num_vertices > 3 &&
			 Points_Colinear(src->vertices[f->vertices[0]],
							 src->vertices[f->vertices[1]],
							 src->vertices[f->vertices[f->num_vertices - 1]]))
		{
			for ( k = 1 ; k < f->num_vertices ; k++ )
				f->vertices[k-1] = f->vertices[k];
			f->num_vertices--;
			changed = TRUE;
		}

		if ( changed )
			f->vertices = More(f->vertices, int, f->num_vertices);
	}
}


static void
Wireframe_Remove_Vertices(WireframePtr src)
{
	Boolean	*used = New(Boolean, src->num_vertices);
	int		*map = New(int, src->num_vertices);
	int		num_verts;
	int		i, j;

	for ( i = 0 ; i < src->num_vertices ; i++ )
		used[i] = FALSE;

	for ( i = 0 ; i < src->num_faces ; i++ )
		for ( j = 0 ; j < src->faces[i].num_vertices ; j++ )
			used[src->faces[i].vertices[j]] = TRUE;

	for ( num_verts = 0, i = 0 ; i < src->num_vertices ; i++ )
	{
		if ( used[i] )
		{
			if ( num_verts == i )
			{
				num_verts++;
				map[i] = i;
			}
			else
			{
				map[i] = num_verts;
				src->vertices[num_verts++] = src->vertices[i];
			}
		}
	}
	src->num_vertices = num_verts + 1;
	/* reallocate vertices to free any excess. */
	src->vertices = More(src->vertices, Vector, src->num_vertices);
	VNew(0, 0, 0, src->vertices[src->num_vertices - 1]);

	/* Redo vertex ptr in faces. */
	for ( i = 0 ; i < src->num_faces ; i++ )
		for ( j = 0 ; j < src->faces[i].num_vertices ; j++ )
			src->faces[i].vertices[j] = map[src->faces[i].vertices[j]];
}




/*	Triangulate a polygon face. There is one main assumption here:
**	Polygons are convex. This is certainly the case for anything
**	sced creates, but may not be for arbitrary OFF files.
**	The original face remains intact.
*/
WireframePtr
Face_Triangulate(WireframePtr src, FacePtr face)
{
	WireframePtr	res = New(Wireframe, 1);
	int				i;

	if ( face->num_vertices <= 3 )
		return NULL;

	/* The wireframe returned will have n - 2 faces, with n vertices. */
	res->num_vertices = face->num_vertices;
	res->vertices = New(Vector, res->num_vertices);
	for ( i = 0 ; i < res->num_vertices ; i++ )
		res->vertices[i] = src->vertices[face->vertices[i]];

	res->num_faces = res->num_vertices - 2;
	res->faces = New(Face, res->num_faces);

	for ( i = 0 ; i < res->num_faces ; i++ )
	{
		res->faces[i].num_vertices = 3;
		res->faces[i].vertices = New(int, 3);
		res->faces[i].vertices[0] = 0;
		res->faces[i].vertices[1] = i + 1;
		res->faces[i].vertices[2] = i + 2;
		res->faces[i].normal = face->normal;
		res->faces[i].face_attribs = NULL;
	}

	if ( face->face_attribs )
	{
		res->attribs = New(AttributePtr, 1);
		res->attribs[0] = New(Attributes, 1);
		*(res->attribs[0]) = *(face->face_attribs);
		res->num_attribs = 1;
		for ( i = 0 ; i < res->num_faces ; i++ )
			res->faces[i].face_attribs = res->attribs[0];
	}
	else
	{
		res->num_attribs = 0;
		res->attribs = NULL;
	}

	if ( src->vertex_normals )
	{
		res->vertex_normals = New(Vector, res->num_vertices);
		for ( i = 0 ; i < res->num_vertices ; i++ )
			res->vertex_normals[i] = src->vertex_normals[face->vertices[i]];
	}
	else
		res->vertex_normals = NULL;

	return res;
}


Boolean
Wireframe_Has_Attributes(WireframePtr wire)
{
	int	i;

	for ( i = 0 ; i < wire->num_faces ; i++ )
		if ( wire->faces[i].face_attribs )
			return TRUE;

	return FALSE;
}


int
Wireframe_Count_Edges(WireframePtr wire)
{
	EdgeTable		edge_table;
	int				next_edge;
	int				i, j;

	/* Build an edge table. */
	edge_table = Wireframe_Edge_Table_Create(wire->num_vertices);
	next_edge = 0;
	for ( i = 0 ; i < wire->num_faces ; i++ )
	{
		Wireframe_Edge_Add_Edge(edge_table, &next_edge,
					  wire->faces[i].vertices[0],
					  wire->faces[i].vertices[wire->faces[i].num_vertices-1],
					  i);
		for ( j = 1 ; j < wire->faces[i].num_vertices ; j++ )
			Wireframe_Edge_Add_Edge(edge_table, &next_edge,
									wire->faces[i].vertices[j-1],
						  			wire->faces[i].vertices[j], i);
	}

	Wireframe_Edge_Free_Table(edge_table, wire->num_vertices);

	return next_edge;
}
