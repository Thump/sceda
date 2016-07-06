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
**	wireframe.h: Header for imported wireframe code.
*/

#ifndef __WIREFRAME__
#define __WIREFRAME__

#define WIRE_CREATE 1
#define WIRE_DELETE 2

/* Another wireframe format!!! */
/* This one is used for storing CSG wireframes (since the full version of such
** wireframes takes too much space), and for imported OFF wireframes.
*/
/* It uses the classic structure with lists of vertices and lists of faces,
** each face having an ordered list of vertex indices.
*/
typedef struct _Face {
	int		*vertices;		/* Clockwise ordered vertex indices. */
	int		num_vertices;
	Vector	normal;			/* A face normal. */
	void	*face_ptr;		/* An arbitrary pointer. Used for face objects
							** with CSG and face attributes with OFFs. */
	} Face, *FacePtr;

typedef struct _Wireframe {
	Vector		*vertices;
	int			num_vertices;
	FacePtr		faces;
	int			num_faces;
	Vector		*vertex_normals;	/* Vertex normals for phong shading. */
	AttributePtr	*attribs;		/* Face attributes. */
	int			num_attribs;
	} Wireframe, *WireframePtr;

extern void	Wireframe_Select_Popup(int);
extern void	Wireframe_Add_Select_Option(BaseObjectPtr);
extern void	Wireframe_Select_Destroy_Widget(int);
extern void	Set_Wireframe_Related_Sensitivity(Boolean);

extern WireframePtr			Wireframe_Copy(WireframePtr);
extern void					Wireframe_Destroy(WireframePtr, Boolean);
extern struct _CSGWireframe	*Wireframe_To_CSG(WireframePtr, Boolean);
extern WireframePtr			CSG_To_Wireframe(struct _CSGWireframe*);
extern EdgeWireframePtr		Wireframe_To_Edge(WireframePtr, Boolean, Boolean);

#endif /* __WIREFRAME__ */
