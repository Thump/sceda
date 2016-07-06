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
**	macros.h : Assorted macros for global use.
**
**	Created: 13/03/94
*/

#ifndef __SCED_MACROS__
#define __SCED_MACROS__

/* Memory allocation macros. */
#define New(t, n) (t*)EMalloc(sizeof(t) * (n))
#define More(x, t, n) (t*)ERealloc((char*)x, sizeof(t) * (n))
#define Strdup(x) strcpy(New(char, strlen(x) + 2), (x))

/* Macros for comparing doubles. */
#define DEqual(a, b)    (((a) > (b) - EPSILON) && ((a) < (b) + EPSILON))
#define IsZero(x)       ((x) < EPSILON && (x) > -EPSILON)

#define MIsIdentity(m) \
				(DEqual((m).x.x, 1.0) && IsZero((m).x.y) && IsZero((m).x.z) && \
				 IsZero((m).y.x) && DEqual((m).y.y, 1.0) && IsZero((m).y.z) && \
				 IsZero((m).z.x) && IsZero((m).z.y) && DEqual((m).z.z, 1.0))

/* Standard min and max macros. */
#define max(a,b)	((a) > (b) ? (a) : (b))
#define min(a,b)	((a) < (b) ? (a) : (b))

/* A macro to produce the identity matrix. */
#define NewIdentityMatrix(m) { Vector v1,v2,v3; \
								VNew(1.0, 0.0, 0.0, v1); \
								VNew(0.0, 1.0, 0.0, v2); \
								VNew(0.0, 0.0, 1.0, v3); \
								MNew(v1, v2, v3, (m)); }

#define Identity_Transform(t) {\
			VNew(0, 0, 0, t.displacement); \
			NewIdentityMatrix(t.matrix); }

#define Point_In_Rect(pt, min, max) \
	((pt).x >= (min).x && (pt).x <= (max).x && \
	 (pt).y >= (min).y && (pt).y <= (max).y)

#define Obj_Is_Light(obj)	( obj->o_parent->b_class == light_obj || \
							  obj->o_parent->b_class == spotlight_obj || \
							  obj->o_parent->b_class == arealight_obj )

#define Transform_Vector(trans, vect, res) \
			{ \
				Vector	_temp_v; \
				MVMul((trans).matrix, (vect), _temp_v); \
				VAdd((trans).displacement, _temp_v, (res)); \
			}

#define Apply_Transform(orig, new, res) \
			(res).matrix = MMMul(&((new).matrix), &((orig).matrix)); \
			VAdd((orig).displacement, (new).displacement, (res).displacement)

#define Transform_Vertices(trans, vects, num) \
			{ \
				int	_i; \
				for ( _i = 0 ; _i < num ; _i++ ) \
					Transform_Vector(trans, vects[_i], vects[_i]) \
			}

#endif



