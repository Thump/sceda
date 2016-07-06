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
**	ViewP.h : Private header file for the View widget class.
**
**	Created: 19/03/94
*/


#ifndef _ViewP_h
#define _ViewP_h

#include <View.h>
/* include superclass private header file */
#include <X11/CoreP.h>


typedef struct {
	int	empty;
} ViewClassPart;

typedef struct _ViewClassRec {
    CoreClassPart	core_class;
    ViewClassPart	view_class;
} ViewClassRec;

extern ViewClassRec viewClassRec;

typedef struct {
    /* resources */
    XtCallbackList	expose_callback;
    Dimension		desired_width;
    Dimension		desired_height;
    Boolean			maintain_size;
    Pixel			foreground_pixel;
    Pixel			background_pixel;
    int				magnification;
    /* private state */
} ViewPart;

typedef struct _ViewRec {
    CorePart	core;
    ViewPart	view;
} ViewRec;

#endif /* _ViewP_h */
