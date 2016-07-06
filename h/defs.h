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
**	defs.h : global #defines
**
**	Created: 26/03/94
*/


#ifndef __SCED_DEFS__
#define __SCED_DEFS__

/* Flags for object things. Self explanatory. ;-) */
#define ObjVisible		(1)
#define ObjSelected		(1<<1)
#define ObjDepends		(1<<2)
#define ObjPending		(1<<3)
#define ObjAll			( ObjVisible | ObjSelected | ObjDepends | ObjPending )


/* Flags for view updates. */
#define	ViewNone		(0)		/* Just do the drawing, and nothing else.	*/
#define CalcView		(1)		/* Recalculate the view coords.  This 		*/
								/* implies CalcScreen.						*/
#define CalcScreen		(1<<1)	/* Recalculate the screen coords for the	*/
								/* objects.									*/
#define RemoveHidden	(1<<2)	/* Draw with hidden lines removed for each	*/
								/* object.									*/
#define JustExpose		(1<<3)	/* Called upon exposure exents. If the 		*/
								/* off screen bitmaps is for the right		*/
								/* window, just do a copy.					*/


/* Flags for the window state. */
#define wait            (0)
#define change_viewfrom (1) 
#define change_distance (1<<1) 
#define change_look     (1<<2)
#define edit_object		(1<<3)
#define window_select	(1<<4)


/* Flags for Modify_Instance_Attributes */
#define ModSimple	(1)
#define ModExtend	(1<<1)

/* Flags for base object selection. */
#define select_new		1
#define select_change	2


/* Flags for wireframe loading. */
#define WIRE_CREATE 1
#define WIRE_DELETE 2


/* Flags for saving. */
#define SAVE_ONLY 0
#define SAVE_QUIT 1
#define SAVE_LOAD 2
#define SAVE_RESET 3
#define SAVE_CLEAR 4

/* Window drawing modes. */
#define DRAW_ALL	1
#define DRAW_DASHED	2
#define DRAW_CULLED	3

#endif
