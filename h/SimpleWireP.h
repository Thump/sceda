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
**	SimpleWireP.h : Private header file for the SimpleWire widget class.
**
**	Created: 19/03/94
*/


#ifndef _SimpleWireP_h
#define _SimpleWireP_h

#include <SimpleWire.h>
/* include superclass private header file */
#include <X11/CoreP.h>

typedef struct {
	int	empty;
} SimpleWireClassPart;

typedef struct _SimpleWireClassRec {
    CoreClassPart		core_class;
    SimpleWireClassPart	simpleWire_class;
} SimpleWireClassRec;

extern SimpleWireClassRec simpleWireClassRec;

typedef struct {
    /* resources */
    XtPointer		base_ptr;
    Pixel			drawing_color;
    XFontStruct*	font;
    XtCallbackList	select_callback;
    /* private state */
    GC				drawing_gc;
    int				scale;
    Wireframe		*wireframe;
	Pixmap			off_screen;
} SimpleWirePart;

typedef struct _SimpleWireRec {
    CorePart		core;
    SimpleWirePart	simpleWire;
} SimpleWireRec;

#endif /* _SimpleWireP_h */
