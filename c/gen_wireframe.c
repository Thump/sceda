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
**	gen_wireframe.c: Functions to generate wireframe representations of objects.
**
**	Created: 05/03/93
**
**	External functions:
**
**	Wireframe*
**	Generic_Sphere_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic sphere.
**	NULL on failure.
**
**	Wireframe*
**	Generic_Cylinder_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic cylinder.
**	NULL on failure.
**
**	Wireframe*
**	Generic_Cone_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic cone.
**	NULL on failure.
**
**	Wireframe*
**	Generic_Cube_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic cube.
**	NULL on failure.
**
**	Wireframe*
**	Generic_Square_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic square.
**	NULL on failure.
**
**	Wireframe*
**	Generic_Plane_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic plane.
**	NULL on failure.
**
**	Wireframe*
**	Generic_Light_Wireframe()
**	Returns a pointer to a light wireframe structure.
*/


#include <math.h>
#include <sced.h>



/*	WireframePtr
**	Generic_Cube_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic cube.
*/
WireframePtr
Generic_Cube_Wireframe()
{
	WireframePtr	result = New(Wireframe, 1);
	int	i;

	/*	A cube wireframe has 6 faces, 12 edges and 8 vertices.
	**	There are an additional 7 reference points, one in the center and
	**	one for the center of each face.
	*/
	result->num_faces = 6;
	result->num_vertices = 15;

	/* These are postitioned and numbered as follows:
	**
	** Vertices:	0 at (1, 1, 1)
	**				1 at (-1, 1, 1)
	**				2 at (-1, -1, 1)
	**				3 at (1, -1, 1)
	**				4 at (1, 1, -1)
	**				5 at (-1, 1, -1)
	**				6 at (-1, -1, -1)
	**				7 at (1, -1, -1)
	**				8 at (1, 0, 0)
	**				9 at (-1, 0, 0)
	**				10 at (0, 1, 0)
	**				11 at (0, -1, 0)
	**				12 at (0, 0, 1)
	**				13 at (0, 0, -1)
	**				14 at (0, 0, 0)
	**
	**                  ____________
	**  Faces:         /|          /|
	**				  / |   4     / |
	**				 /___________/  |
	**				|   |    1   |  |
	**				| 3 |        | 2|
	**				|   |  0     |  |
	**				|   |________|__|
	**				|  /         | /
	**				| /     5    |/
	**				|____________|
	**
	*/

	/* Allocate vertices first. */
	result->vertices = New(Vector, result->num_vertices);
	VNew(1, 1, 1, result->vertices[0]);
	VNew(-1, 1, 1, result->vertices[1]);
	VNew(-1, -1, 1, result->vertices[2]);
	VNew(1, -1, 1, result->vertices[3]);
	VNew(1, 1, -1, result->vertices[4]);
	VNew(-1, 1, -1, result->vertices[5]);
	VNew(-1, -1, -1, result->vertices[6]);
	VNew(1, -1, -1, result->vertices[7]);
	VNew(1, 0, 0, result->vertices[8]);
	VNew(-1, 0, 0, result->vertices[9]);
	VNew(0, 1, 0, result->vertices[10]);
	VNew(0, -1, 0, result->vertices[11]);
	VNew(0, 0, 1, result->vertices[12]);
	VNew(0, 0, -1, result->vertices[13]);
	VNew(0, 0, 0, result->vertices[14]);

	/* Allocate faces. */
	result->faces = New(Face, result->num_faces);
	for ( i = 0 ; i < result->num_faces ; i++ )
	{
		result->faces[i].num_vertices = 4;
		result->faces[i].vertices = New(int, 4);
		result->faces[i].face_attribs = NULL;
	}

	result->faces[0].vertices[0] = 0;	result->faces[0].vertices[1] = 4;
	result->faces[0].vertices[2] = 7;	result->faces[0].vertices[3] = 3;
	VNew(1, 0, 0, result->faces[0].normal);

	result->faces[1].vertices[0] = 1;	result->faces[1].vertices[1] = 2;
	result->faces[1].vertices[2] = 6;	result->faces[1].vertices[3] = 5;
	VNew(-1, 0, 0, result->faces[1].normal);

	result->faces[2].vertices[0] = 0;	result->faces[2].vertices[1] = 1;
	result->faces[2].vertices[2] = 5;	result->faces[2].vertices[3] = 4;
	VNew(0, 1, 0, result->faces[2].normal);

	result->faces[3].vertices[0] = 2;	result->faces[3].vertices[1] = 3;
	result->faces[3].vertices[2] = 7;	result->faces[3].vertices[3] = 6;
	VNew(0, -1, 0, result->faces[3].normal);

	result->faces[4].vertices[0] = 3;	result->faces[4].vertices[1] = 2;
	result->faces[4].vertices[2] = 1;	result->faces[4].vertices[3] = 0;
	VNew(0, 0, 1, result->faces[4].normal);

	result->faces[5].vertices[0] = 4;	result->faces[5].vertices[1] = 5;
	result->faces[5].vertices[2] = 6;	result->faces[5].vertices[3] = 7;
	VNew(0, 0, -1, result->faces[5].normal);

	result->num_attribs = 0;
	result->attribs = NULL;
	result->vertex_normals = NULL;

	return result;

}


/*	WireframePtr
**	Generic_Square_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic square.
**	NULL on failure.
*/
WireframePtr
Generic_Square_Wireframe()
{
	WireframePtr	result = New(Wireframe, 1);
	int				i;

	/*	A square wireframe has 8 faces, 5 vertices.	*/
	/*	There are no additional reference vertices.	*/
	result->num_faces = 8;
	result->num_vertices = 5;


	/*	It looks like this:
	**
	**
	**                  2 (-1,-1,0) ____________1_____ 1 (-1, 1, 0)
	**                             /\              __/
	**                            /  6    1    __5- /
	**                           /    \_   __--    /
	**                          2  2   _4(0,0,0)0 /
	**                         /   _7--   \_     /
	**                        /__--    3    4_  0
	**                       /________3_______\/
	**             3 (1, -1, 0)              0 (1,1,0)
	**
	*/

	/* Allocate vertices first. */
	result->vertices = New(Vector, result->num_vertices);
	VNew(1, 1, 0, result->vertices[0]);
	VNew(-1, 1, 0, result->vertices[1]);
	VNew(-1, -1, 0, result->vertices[2]);
	VNew(1, -1, 0, result->vertices[3]);
	VNew(0, 0, 0, result->vertices[4]);

	/* Finally, allocate the faces. */
	result->faces = New(Face, result->num_faces);
	for ( i = 0 ; i < result->num_faces ; i++ )
	{
		result->faces[i].num_vertices = 3;
		result->faces[i].vertices = New(int, 3);
		result->faces[i].vertices[2] = 4;
		if ( i / 4 )
			VNew(0, 0, -1, result->faces[i].normal);
		else
			VNew(0, 0, 1, result->faces[i].normal);
		result->faces[i].face_attribs = NULL;
	}

	result->faces[0].vertices[0] = 1; result->faces[0].vertices[1] = 0;
	result->faces[1].vertices[0] = 2; result->faces[1].vertices[1] = 1;
	result->faces[2].vertices[0] = 3; result->faces[2].vertices[1] = 2;
	result->faces[3].vertices[0] = 0; result->faces[3].vertices[1] = 3;
	result->faces[4].vertices[0] = 0; result->faces[4].vertices[1] = 1;
	result->faces[5].vertices[0] = 1; result->faces[5].vertices[1] = 2;
	result->faces[6].vertices[0] = 2; result->faces[6].vertices[1] = 3;
	result->faces[7].vertices[0] = 3; result->faces[7].vertices[1] = 0;

	result->num_attribs = 0;
	result->attribs = NULL;
	result->vertex_normals = NULL;

	return result;
}



/*	WireframePtr
**	Generic_Plane_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic plane.
**	NULL on failure.
*/
WireframePtr
Generic_Plane_Wireframe()
{
	WireframePtr	result = New(Wireframe, 1);
	int				i;

	/*	A plane wireframe has 1 face, 6 edges and 12 vertices.	*/
	/*	There is one additional reference vertex in the center.	*/
	result->num_faces = 10;
	result->num_vertices = 16;


	/*	It looks like an infinite square:
	**
	**
	**
	**                                   7 (-10, 0, 0)
	**                          8 (-10,-1,0)            6 (-10,1,0)
	**                                 _____________9____ 
	**                                /        /        /
	**                               /        /        /
	**         9 (-1,-10,0) ________/________/________/____5___ 5 (-1,10,0)
	**                     /       /        /        /         /
	**                    / 3,7   /        /        /         /
	**                   /       /        /        /         /
	**      10 (0,-10,0)/-------/--------/--------/-----4---/ 4 (0,10,0)
	**                 /       /        /        /         /
	**                6  2,6  /        /        /         7
	**   11 (1,-10,0)/_______/________/________/_____3___/ 3 (1,10,0)
	**                      /        /        /
	**                     0  0,4   1  1,5   2
	**                    /        /        /
	**                   /________/___8____/
	**          0 (10,-1, 0)              2 (10,1,0)
	**                    1 (10,0,0)
	**
	**	There is also a little arrow pointing in the normal direction.
	*/

	/* Allocate vertices first. */
	result->vertices = New(Vector, result->num_vertices);

	VNew(10, -1, 0, result->vertices[0]);
	VNew(10, 0, 0, result->vertices[1]);
	VNew(10, 1, 0, result->vertices[2]);
	VNew(1, 10, 0, result->vertices[3]);
	VNew(0, 10, 0, result->vertices[4]);
	VNew(-1, 10, 0, result->vertices[5]);
	VNew(-10, 1, 0, result->vertices[6]);
	VNew(-10, 0, 0, result->vertices[7]);
	VNew(-10, -1, 0, result->vertices[8]);
	VNew(-1, -10, 0, result->vertices[9]);
	VNew(0, -10, 0, result->vertices[10]);
	VNew(1, -10, 0, result->vertices[11]);
	VNew(1,   0, 0, result->vertices[12]);
	VNew(-1,  0, 0, result->vertices[13]);
	VNew(0,   0, -0.5, result->vertices[14]);
	VNew(0, 0, 0, result->vertices[15]);

	result->faces = New(Face, result->num_faces);
	for ( i = 0 ; i < 8 ; i++ )
	{
		result->faces[i].num_vertices = 4;
		result->faces[i].vertices = New(int, 4);
		if ( i / 4 )
			VNew(0, 0, -1, result->faces[i].normal);
		else
			VNew(0, 0, 1, result->faces[i].normal);
		result->faces[i].face_attribs = NULL;
	}
	for ( ; i < result->num_faces ; i++ )
	{
		result->faces[i].num_vertices = 3;
		result->faces[i].vertices = New(int, 3);
		result->faces[i].face_attribs = NULL;
	}
	VNew(0, 1, 0, result->faces[8].normal);
	VNew(0, -1, 0, result->faces[9].normal);

	result->faces[0].vertices[0] = 1;	result->faces[0].vertices[1] = 0;
	result->faces[0].vertices[2] = 8;	result->faces[0].vertices[3] = 7;
	result->faces[1].vertices[0] = 2;	result->faces[1].vertices[1] = 1;
	result->faces[1].vertices[2] = 7;	result->faces[1].vertices[3] = 6;
	result->faces[2].vertices[0] = 11;	result->faces[2].vertices[1] = 10;
	result->faces[2].vertices[2] = 4;	result->faces[2].vertices[3] = 3;
	result->faces[3].vertices[0] = 10;	result->faces[3].vertices[1] = 9;
	result->faces[3].vertices[2] = 5;	result->faces[3].vertices[3] = 4;
	result->faces[4].vertices[0] = 0;	result->faces[4].vertices[1] = 1;
	result->faces[4].vertices[2] = 7;	result->faces[4].vertices[3] = 8;
	result->faces[5].vertices[0] = 1;	result->faces[5].vertices[1] = 2;
	result->faces[5].vertices[2] = 6;	result->faces[5].vertices[3] = 7;
	result->faces[6].vertices[0] = 3;	result->faces[6].vertices[1] = 4;
	result->faces[6].vertices[2] = 10;	result->faces[6].vertices[3] = 11;
	result->faces[7].vertices[0] = 4;	result->faces[7].vertices[1] = 5;
	result->faces[7].vertices[2] = 9;	result->faces[7].vertices[3] = 10;
	result->faces[8].vertices[0] = 12;	result->faces[8].vertices[1] = 13;
	result->faces[8].vertices[2] = 14;
	result->faces[9].vertices[0] = 14;	result->faces[9].vertices[1] = 13;
	result->faces[9].vertices[2] = 12;

	result->num_attribs = 0;
	result->attribs = 0;
	result->vertex_normals = NULL;

	return result;
}





/*	WireframePtr
**	Generic_Cylinder_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic cylinder.
**	NULL on failure.
*/
WireframePtr
Generic_Cylinder_Wireframe()
{
	WireframePtr	result = New(Wireframe, 1);
	double			norm1, norm2;
	int				i;

	/*	A cylinder wireframe has 10 faces, 24 edges and 16 vertices.
	**	There are 3 reference vertices, one at the center of each endcap
	**	and one in the center of the object.
	*/
	result->num_faces = 10;
	result->num_vertices = 19;


	/*	Vertices are arranged 0-7 around the top, 8-15 on the bottom.
	**	Faces are 0-7 around the sides, 8 on top and 9 on bottom.
	*/

	/* Allocate vertices first. */
	result->vertices = New(Vector, result->num_vertices);
	VNew( 1,       0,       1, result->vertices[0]);
	VNew( M_SQRT1_2,  M_SQRT1_2,  1, result->vertices[1]);
	VNew( 0,       1,       1, result->vertices[2]);
	VNew(-M_SQRT1_2,  M_SQRT1_2,  1, result->vertices[3]);
	VNew(-1,       0,       1, result->vertices[4]);
	VNew(-M_SQRT1_2, -M_SQRT1_2,  1, result->vertices[5]);
	VNew( 0,      -1,       1, result->vertices[6]);
	VNew( M_SQRT1_2, -M_SQRT1_2,  1, result->vertices[7]);
	VNew( 1,       0,      -1, result->vertices[8]);
	VNew( M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[9]);
	VNew( 0,       1,      -1, result->vertices[10]);
	VNew(-M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[11]);
	VNew(-1,       0,      -1, result->vertices[12]);
	VNew(-M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[13]);
	VNew( 0,      -1,      -1, result->vertices[14]);
	VNew( M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[15]);
	VNew( 0, 0, 1, result->vertices[16]);
	VNew( 0, 0, -1, result->vertices[17]);
	VNew( 0, 0, 0, result->vertices[18]);

	/* Finally, allocate the faces. */
	result->faces = New(Face, result->num_faces);
	for ( i = 0 ; i < 8 ; i++ )
	{
		result->faces[i].num_vertices = 4;
		result->faces[i].vertices = New(int, 4);
		result->faces[i].vertices[0] = i;
		result->faces[i].vertices[1] = i + 1;
		result->faces[i].vertices[2] = i + 9;
		result->faces[i].vertices[3] = i + 8;
		result->faces[i].face_attribs = NULL;
	}
	result->faces[7].vertices[1] = 0;
	result->faces[7].vertices[2] = 8;

	norm1 = 1.0 / ( M_SQRT2 * sqrt( 2.0 - M_SQRT2 ) );
	norm2 = sqrt( 2.0 - M_SQRT2 ) / 2.0;

	VNew(norm1, norm2, 0, result->faces[0].normal);
	VNew(norm2, norm1, 0, result->faces[1].normal);
	VNew(-norm2, norm1, 0, result->faces[2].normal);
	VNew(-norm1, norm2, 0, result->faces[3].normal);
	VNew(-norm1, -norm2, 0, result->faces[4].normal);
	VNew(-norm2, -norm1, 0, result->faces[5].normal);
	VNew(norm2, -norm1, 0, result->faces[6].normal);
	VNew(norm1, -norm2, 0, result->faces[7].normal);

	for ( ; i < result->num_faces ; i++ )
	{
		result->faces[i].num_vertices = 8;
		result->faces[i].vertices = New(int, 8);
		result->faces[i].face_attribs = NULL;
	}
	for ( i = 0 ; i < 8 ; i++ )
		result->faces[8].vertices[i] = 7 - i;
	VNew(0, 0, 1, result->faces[8].normal);
	for ( i = 0 ; i < 8 ; i++ )
		result->faces[9].vertices[i] = i + 8;
	VNew(0, 0, -1, result->faces[9].normal);

	result->num_attribs = 0;
	result->attribs = NULL;
	result->vertex_normals = NULL;

	return result;
}



/*	WireframePtr
**	Generic_Cone_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic cone.
*/
WireframePtr
Generic_Cone_Wireframe()
{
	WireframePtr	result = New(Wireframe, 1);
	double			temp_d;
	int				i;

	/*	A cone wireframe has 9 faces, 16 edges and 11 vertices.	*/
	/*	There is a reference vertex at the center.				*/
	result->num_faces = 9;
	result->num_vertices = 11;


	/*	Vertices are arranged 0 at the top, 1-8 on the bottom.
	**	Faces are 0-7 around the sides, 8 on the bottom.
	*/

	/* Allocate vertices first. */
	result->vertices = New(Vector, result->num_vertices);
	VNew( 0,       0,       1, result->vertices[0]);
	VNew( 1,       0,      -1, result->vertices[1]);
	VNew( M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[2]);
	VNew( 0,       1,      -1, result->vertices[3]);
	VNew(-M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[4]);
	VNew(-1,       0,      -1, result->vertices[5]);
	VNew(-M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[6]);
	VNew( 0,      -1,      -1, result->vertices[7]);
	VNew( M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[8]);
	VNew( 0, 0, -1, result->vertices[9]);
	VNew( 0, 0, 0, result->vertices[10]);

	result->faces = New(Face, result->num_faces);

	for ( i = 0 ; i < 8 ; i++ )
	{
		result->faces[i].num_vertices = 3;
		result->faces[i].vertices = New(int, 3);
		result->faces[i].vertices[0] = 0;
		result->faces[i].vertices[1] = i + 2;
		result->faces[i].vertices[2] = i + 1;
		result->faces[i].face_attribs = NULL;
	}
	result->faces[7].vertices[1] = 1;

	VNew(M_SQRT2, 2 - M_SQRT2, M_SQRT1_2, result->faces[0].normal);
	VNew(2 - M_SQRT2, M_SQRT2, M_SQRT1_2, result->faces[1].normal);
	VNew(M_SQRT2 - 2, M_SQRT2, M_SQRT1_2, result->faces[2].normal);
	VNew(-M_SQRT2, 2 - M_SQRT2, M_SQRT1_2, result->faces[3].normal);
	VNew(-M_SQRT2, M_SQRT2 - 2, M_SQRT1_2, result->faces[4].normal);
	VNew( M_SQRT2 - 2, -M_SQRT2, M_SQRT1_2, result->faces[5].normal);
	VNew(2 - M_SQRT2, -M_SQRT2, M_SQRT1_2, result->faces[6].normal);
	VNew(M_SQRT2, M_SQRT2 - 2, M_SQRT1_2, result->faces[7].normal);

	result->faces[8].num_vertices = 8;
	result->faces[8].vertices = New(int, 8);
	for ( i = 0 ; i < 8 ; i++ )
		result->faces[8].vertices[i] = i + 1;
	VNew(0, 0, -1, result->faces[8].normal);
	result->faces[8].face_attribs = NULL;

	/* Normalize the normals. */
	for ( i = 0 ; i < result->num_faces - 1 ; i++ )
		VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);

	result->attribs = NULL;
	result->vertex_normals = NULL;
	result->num_attribs = 0;

	return result;
}



/*	WireframePtr
**	Generic_Sphere_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a generic sphere.
*/
WireframePtr
Generic_Sphere_Wireframe()
{
	WireframePtr	result = New(Wireframe, 1);
	double			norm_1, norm_2;
	int				i;

	/*	A sphere is represented by a ruled surface.
	**  Its wireframe has 32 faces, 56 edges and 27 vertices.
	**	There is just one extra vertex, the center.
	*/
	result->num_faces = 32;
	result->num_vertices = 27;


	/*	Vertices are arranged 0 at the top, 1 at the bottom, then in groups
	**	of 8 around each band from top to bottom.
	**	Faces are in groups of 8 for each band.
	*/

	result->vertices = New(Vector, result->num_vertices);
	VNew( 0.0      , 0.0      , 1.0      , result->vertices[0]);
	VNew( 0.0      , 0.0      ,-1.0      , result->vertices[1]);
	VNew( M_SQRT1_2, 0.0      , M_SQRT1_2, result->vertices[2]);
	VNew( 0.5      , 0.5      , M_SQRT1_2, result->vertices[3]);
	VNew( 0.0      , M_SQRT1_2, M_SQRT1_2, result->vertices[4]);
	VNew(-0.5      , 0.5      , M_SQRT1_2, result->vertices[5]);
	VNew(-M_SQRT1_2, 0.0      , M_SQRT1_2, result->vertices[6]);
	VNew(-0.5      ,-0.5      , M_SQRT1_2, result->vertices[7]);
	VNew( 0.0      ,-M_SQRT1_2, M_SQRT1_2, result->vertices[8]);
	VNew( 0.5      ,-0.5      , M_SQRT1_2, result->vertices[9]);
	VNew( 1.0      , 0.0      , 0.0      , result->vertices[10]);
	VNew( M_SQRT1_2, M_SQRT1_2, 0.0      , result->vertices[11]);
	VNew( 0.0      , 1.0      , 0.0      , result->vertices[12]);
	VNew(-M_SQRT1_2, M_SQRT1_2, 0.0      , result->vertices[13]);
	VNew(-1.0      , 0.0      , 0.0      , result->vertices[14]);
	VNew(-M_SQRT1_2,-M_SQRT1_2, 0.0      , result->vertices[15]);
	VNew( 0.0      ,-1.0      , 0.0      , result->vertices[16]);
	VNew( M_SQRT1_2,-M_SQRT1_2, 0.0      , result->vertices[17]);
	VNew( M_SQRT1_2, 0.0      ,-M_SQRT1_2, result->vertices[18]);
	VNew( 0.5      , 0.5      ,-M_SQRT1_2, result->vertices[19]);
	VNew( 0.0      , M_SQRT1_2,-M_SQRT1_2, result->vertices[20]);
	VNew(-0.5      , 0.5      ,-M_SQRT1_2, result->vertices[21]);
	VNew(-M_SQRT1_2, 0.0      ,-M_SQRT1_2, result->vertices[22]);
	VNew(-0.5      ,-0.5      ,-M_SQRT1_2, result->vertices[23]);
	VNew( 0.0      ,-M_SQRT1_2,-M_SQRT1_2, result->vertices[24]);
	VNew( 0.5      ,-0.5      ,-M_SQRT1_2, result->vertices[25]);
	VNew( 0.0      , 0.0      , 0.0      , result->vertices[26]);


	/* Finally, allocate the faces. */
	result->faces = New(Face, result->num_faces);

	norm_1 = M_SQRT2 - 1;
	norm_2 = 3 - 2 * M_SQRT2;

	for ( i = 0 ; i < 8 ; i++ )
	{
		result->faces[i].num_vertices = 3;
		result->faces[i].vertices = New(int, 3);
		result->faces[i].vertices[0] = 0;
		result->faces[i].vertices[1] = i + 3;
		result->faces[i].vertices[2] = i + 2;
		result->faces[i].face_attribs = NULL;
	}
	result->faces[7].vertices[1] = 2;
	VNew( norm_1, norm_2, 1.0, result->faces[0].normal);
	VNew( norm_2, norm_1, 1.0, result->faces[1].normal);
	VNew(-norm_2, norm_1, 1.0, result->faces[2].normal);
	VNew(-norm_1, norm_2, 1.0, result->faces[3].normal);
	VNew(-norm_1,-norm_2, 1.0, result->faces[4].normal);
	VNew(-norm_2,-norm_1, 1.0, result->faces[5].normal);
	VNew( norm_2,-norm_1, 1.0, result->faces[6].normal);
	VNew( norm_1,-norm_2, 1.0, result->faces[7].normal);

	for ( i = 24 ; i < 32 ; i++ )
	{
		result->faces[i].num_vertices = 3;
		result->faces[i].vertices = New(int, 3);
		result->faces[i].vertices[0] = 1;
		result->faces[i].vertices[1] = i - 6;
		result->faces[i].vertices[2] = i - 5;
		result->faces[i].face_attribs = NULL;
	}
	result->faces[31].vertices[2] = 18;
	VNew( norm_1, norm_2,-1.0, result->faces[24].normal);
	VNew( norm_2, norm_1,-1.0, result->faces[25].normal);
	VNew(-norm_2, norm_1,-1.0, result->faces[26].normal);
	VNew(-norm_1, norm_2,-1.0, result->faces[27].normal);
	VNew(-norm_1,-norm_2,-1.0, result->faces[28].normal);
	VNew(-norm_2,-norm_1,-1.0, result->faces[29].normal);
	VNew( norm_2,-norm_1,-1.0, result->faces[30].normal);
	VNew( norm_1,-norm_2,-1.0, result->faces[31].normal);

	norm_1 = M_SQRT2 - 1;

	for ( i = 8 ; i < 16 ; i++ )
	{
		result->faces[i].num_vertices = 4;
		result->faces[i].vertices = New(int, 4);
		result->faces[i].vertices[0] = i - 6;
		result->faces[i].vertices[1] = i - 5;
		result->faces[i].vertices[2] = i + 3;
		result->faces[i].vertices[3] = i + 2;
		result->faces[i].face_attribs = NULL;
	}
	result->faces[15].vertices[1] = 2;
	result->faces[15].vertices[2] = 10;
	VNew( 1.0   , norm_1, norm_1, result->faces[8].normal);
	VNew( norm_1, 1.0   , norm_1, result->faces[9].normal);
	VNew(-norm_1, 1.0   , norm_1, result->faces[10].normal);
	VNew(-1.0   , norm_1, norm_1, result->faces[11].normal);
	VNew(-1.0   ,-norm_1, norm_1, result->faces[12].normal);
	VNew(-norm_1,-1.0   , norm_1, result->faces[13].normal);
	VNew( norm_1,-1.0   , norm_1, result->faces[14].normal);
	VNew( 1.0   ,-norm_1, norm_1, result->faces[15].normal);

	for ( i = 16 ; i < 24 ; i++ )
	{
		result->faces[i].num_vertices = 4;
		result->faces[i].vertices = New(int, 4);
		result->faces[i].vertices[0] = i - 6;
		result->faces[i].vertices[1] = i - 5;
		result->faces[i].vertices[2] = i + 3;
		result->faces[i].vertices[3] = i + 2;
		result->faces[i].face_attribs = NULL;
	}
	result->faces[23].vertices[1] = 10;
	result->faces[23].vertices[2] = 18;
	VNew( 1.0   , norm_1,-norm_1, result->faces[16].normal);
	VNew( norm_1, 1.0   ,-norm_1, result->faces[17].normal);
	VNew(-norm_1, 1.0   ,-norm_1, result->faces[18].normal);
	VNew(-1.0   , norm_1,-norm_1, result->faces[19].normal);
	VNew(-1.0   ,-norm_1,-norm_1, result->faces[20].normal);
	VNew(-norm_1,-1.0   ,-norm_1, result->faces[21].normal);
	VNew( norm_1,-1.0   ,-norm_1, result->faces[22].normal);
	VNew( 1.0   ,-norm_1,-norm_1, result->faces[23].normal);

	/* Normalize the normals. */
	for ( i = 0 ; i < result->num_faces ; i++ )
		VUnit(result->faces[i].normal, norm_1, result->faces[i].normal);

	result->num_attribs = 0;
	result->attribs = NULL;
	result->vertex_normals = NULL;

	return result;
}


/*	WireframePtr
**	Generic_Light_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a light.
*/
WireframePtr
Generic_Light_Wireframe()
{
	WireframePtr	result = New(Wireframe, 1);
	int				i;

	/*	A light wireframe has 6 faces, 3 edges and 7 vertices.	*/
	/*	There are no additional reference vertices.				*/
	result->num_faces = 6;
	result->num_vertices = 7;

	/* Allocate vertices first. */
	result->vertices = New(Vector, result->num_vertices);
	VNew(0.5, 0, 0, result->vertices[0]);
	VNew(-0.5, 0, 0, result->vertices[1]);
	VNew(0, 0.5, 0, result->vertices[2]);
	VNew(0, -0.5, 0, result->vertices[3]);
	VNew(0, 0, 0.50, result->vertices[4]);
	VNew(0, 0, -0.50, result->vertices[5]);
	VNew(0, 0, 0, result->vertices[6]);

	/* Finally, allocate the faces. */
	result->faces = New(Face, result->num_faces);
	for ( i = 0 ; i < 6 ; i++ )
	{
		result->faces[i].num_vertices = 4;
		result->faces[i].vertices = New(int, 4);
		result->faces[i].face_attribs = NULL;
	}

	result->faces[0].vertices[0] = 4;	result->faces[0].vertices[1] = 2;
	result->faces[0].vertices[2] = 5;	result->faces[0].vertices[3] = 3;
	VNew(1, 0, 0, result->faces[0].normal);
	result->faces[1].vertices[0] = 4;	result->faces[1].vertices[1] = 3;
	result->faces[1].vertices[2] = 5;	result->faces[1].vertices[3] = 2;
	VNew(-1, 0, 0, result->faces[1].normal);
	result->faces[2].vertices[0] = 4;	result->faces[2].vertices[1] = 1;
	result->faces[2].vertices[2] = 5;	result->faces[2].vertices[3] = 0;
	VNew(0, 1, 0, result->faces[2].normal);
	result->faces[3].vertices[0] = 5;	result->faces[3].vertices[1] = 1;
	result->faces[3].vertices[2] = 4;	result->faces[3].vertices[3] = 0;
	VNew(0, -1, 0, result->faces[3].normal);
	result->faces[4].vertices[0] = 1;	result->faces[4].vertices[1] = 2;
	result->faces[4].vertices[2] = 2;	result->faces[4].vertices[3] = 3;
	VNew(0, 0, 1, result->faces[4].normal);
	result->faces[5].vertices[0] = 0;	result->faces[5].vertices[1] = 2;
	result->faces[5].vertices[2] = 1;	result->faces[5].vertices[3] = 3;
	VNew(0, 0, -1, result->faces[5].normal);

	result->attribs = NULL;
	result->vertex_normals = NULL;
	result->num_attribs = 0;

	return result;
}



/*	WireframePtr
**	Generic_Spot_Light_Wireframe();
**	Returns a pointer to a NEW wireframe structure for a spotlight wireframe.
**	A spotlight is virtually the same as a cone, but has a different origin
**	and is considered open.
**	NULL on failure.
*/
WireframePtr
Generic_Spot_Light_Wireframe()
{
	WireframePtr	result = New(Wireframe, 1);
	double			temp_d;
	int				i;

	/*	A spotlight wireframe has 8 faces and 10 vertices.	*/
	/*	There is a reference vertex at the center.			*/
	result->num_faces = 8;
	result->num_vertices = 11;


	/*	Vertices are arranged 0 at the top, 1-8 on the bottom.
	**	Faces are 0-7 around the sides.
	*/

	/* Allocate vertices first. */
	result->vertices = New(Vector, result->num_vertices);
	VNew( 1,       0,      -1, result->vertices[0]);
	VNew( M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[1]);
	VNew( 0,       1,      -1, result->vertices[2]);
	VNew(-M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[3]);
	VNew(-1,       0,      -1, result->vertices[4]);
	VNew(-M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[5]);
	VNew( 0,      -1,      -1, result->vertices[6]);
	VNew( M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[7]);
	VNew( 0, 0, -1, result->vertices[8]);
	VNew( 0, 0, 0, result->vertices[9]);

	/* Finally, allocate the faces. */
	result->faces = New(Face, result->num_faces);
	for ( i = 0 ; i < result->num_faces ; i++ )
	{
		result->faces[i].num_vertices = 3;
		result->faces[i].vertices = New(int, 3);
		result->faces[i].vertices[0] = 9;
		result->faces[i].vertices[1] = i + 1;
		result->faces[i].vertices[2] = i;
		result->faces[i].face_attribs = NULL;
	}
	result->faces[7].vertices[1] = 0;
	VNew(M_SQRT2, 2 - M_SQRT2, M_SQRT1_2, result->faces[0].normal);
	VNew(2 - M_SQRT2, M_SQRT2, M_SQRT1_2, result->faces[1].normal);
	VNew(M_SQRT2 - 2, M_SQRT2, M_SQRT1_2, result->faces[2].normal);
	VNew(-M_SQRT2, 2 - M_SQRT2, M_SQRT1_2, result->faces[3].normal);
	VNew(-M_SQRT2, M_SQRT2 - 2, M_SQRT1_2, result->faces[4].normal);
	VNew( M_SQRT2 - 2, -M_SQRT2, M_SQRT1_2, result->faces[5].normal);
	VNew(2 - M_SQRT2, -M_SQRT2, M_SQRT1_2, result->faces[6].normal);
	VNew(M_SQRT2, M_SQRT2 - 2, M_SQRT1_2, result->faces[7].normal);

	/* Normalize the normals. */
	for ( i = 0 ; i < result->num_faces ; i++ )
		VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);

	result->attribs = NULL;
	result->vertex_normals = NULL;
	result->num_attribs = 0;

	return result;
}
