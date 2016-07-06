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

#define PATCHLEVEL 0
/*
**	sced: A Constraint Based Object Scene Editor
**
**	select_point.h : header file for vertex selection functions as used
**					 by add_constraint.c and placement.c.
*/

/* A procedure type for procedures called when the required number of points
** have been selected.
*/
typedef void (*SelectPointCallbackProc)(int*, FeatureSpecType*);

/* A structure for reference vertices.  Holds a screen vertex and information
** about how to find it again.
*/
typedef struct _SelectPointStruct {
	XPoint				vert;
	ObjectInstancePtr	obj;
	int					offset;
	} SelectPointStruct;

/* The window to do it in. */
extern WindowInfoPtr	select_window;

/* The vertices to select from. */
extern int					num_select_verts;
extern SelectPointStruct	*select_verts;

/* The number required.  When this number reaches 0, the callback will be
** invoked.  If 0 already the function will exit immediately.
*/
extern int		num_verts_required;
extern Boolean	specs_allowed[];
extern Vector	select_center;

/* The callback to invoke when all the points have been selected.	*/
extern SelectPointCallbackProc	select_finished_callback;

/* Whether to allow text entry of points. */
extern Boolean	allow_text_entry;

/* Control of highlighting. */
extern Boolean	select_highlight;

/* Function declarations. */
extern void	Select_Point_Action(Widget, XEvent*, String*, Cardinal*);
extern void Select_Point(XPoint, FeatureSpecType);
extern void	Select_Highlight_Object(Widget, XEvent*, String*, Cardinal*);
extern void	Select_Highlight_Action(Widget, XEvent*, String*, Cardinal*);
extern void	Select_Highlight_Closest(Window);
extern void Draw_Selection_Points(Drawable);
extern void	Select_Calc_Screen();
extern void	Build_Select_Verts_From_List(InstanceList);
extern void	Prepare_Selection_Drawing();
extern void	Cleanup_Selection();

extern void	Register_Select_Operation();
extern void	Cancel_Select_Operation();

