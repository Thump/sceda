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
**	SimpleWire.h : Public header file for a simple widget used to draw a
**				   single wireframe.
**
**	Created: 19/03/94
*/


#ifndef _SimpleWire_h
#define _SimpleWire_h

/****************************************************************
 *
 * SimpleWire widget
 *
 ****************************************************************/

/* Resources:

 Name				Class				RepType		Default Value
 ----				-----				-------		-------------
 background			Background			Pixel		XtDefaultBackground
 border				BorderColor			Pixel		XtDefaultForeground
 borderWidth		BorderWidth			Dimension	1
 destroyCallback	Callback			Pointer		NULL
 height				Height				Dimension	0
 mappedWhenManaged	MappedWhenManaged	Boolean		True
 sensitive			Sensitive			Boolean		True
 width				Width				Dimension	0
 x					Position			Position	0
 y					Position			Position	0
 basePtr			Value				Pointer		NULL
 foreground			Color				Pixel		XtDefaultForeground
 font				Font				XFontStruct* XtDefaultFont
 callback			Callback			Pointer		NULL

*/

/* define any special resource names here that are not in <X11/StringDefs.h> */

#define XtNbasePtr "basePtr"

/* declare specific SimpleWireWidget class and instance datatypes */

typedef struct _SimpleWireClassRec*	SimpleWireWidgetClass;
typedef struct _SimpleWireRec*		SimpleWireWidget;

/* declare the class constant */

extern WidgetClass simpleWireWidgetClass;

/* declare convenience procedures */
extern void Update_SimpleWire_Wireframe(Widget, WireframePtr);

#endif /* _SimpleWire_h */
