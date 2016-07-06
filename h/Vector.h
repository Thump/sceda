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

/****************************************************************/
/*																*/
/*	Header file for the vector macros.							*/
/*	Written by Stephen Chenney.  Dec 1993.						*/
/*																*/
/****************************************************************/

#ifndef __VECTOR__
#define __VECTOR__

/* Define a vector.  */
typedef struct _Vector
	{
		double  x;
		double  y;
		double  z;
	} Vector;

/* A Matrix is a set of vectors. */
typedef struct _Matrix {
	Vector x;
	Vector y;
	Vector z;
	} Matrix;

/* I'm adding this after having removed it from edit.h: I need to use
** a Quaternion in the ObjectInstance structure, and so need this defined
** before it would normally have been done in edit.h.
*/

typedef struct _Quaternion {
    Vector   vect_part;
    double   real_part;
    } Quaternion;

#define VECT_EPSILON 1.e-13
#define VECT_EPSILON_SQ 1.e-26

/* Zero vector check. */
#define VNearZero(v)	((v).x < VECT_EPSILON && \
						 (v).x > -VECT_EPSILON && \
						 (v).y < VECT_EPSILON && \
						 (v).y > -VECT_EPSILON && \
						 (v).z < VECT_EPSILON && \
						 (v).z > -VECT_EPSILON )

#define VZero(v)		( VNearZero(v) && VDot((v), (v)) < VECT_EPSILON_SQ )
#define VEqual(a, b, c)	( VSub((a), (b), (c)), \
						  VNearZero(c) && VZero(c) )

#define MZero(m)	(VZero((m).x) && VZero((m).y) && VZero((m).z))

/* Takes the modulus of v */
#define VMod(v)		(sqrt((v).x*(v).x + (v).y*(v).y + (v).z*(v).z))

/* Returns the dot product of v1 & v2 */
#define VDot(v1, v2)	((v1).x*(v2).x + (v1).y*(v2).y + (v1).z*(v2).z)

/* Fills the fields of a vector.	*/
#define VNew(a, b, c, r)	((r).x = (a), (r).y = (b), (r).z = (c))

#define VAdd(v1, v2, r)		((r).x = (v1).x + (v2).x , \
							 (r).y = (v1).y + (v2).y , \
							 (r).z = (v1).z + (v2).z )

#define VSub(v1, v2, r)		((r).x = (v1).x - (v2).x , \
							 (r).y = (v1).y - (v2).y , \
							 (r).z = (v1).z - (v2).z )

#define VScalarMul(v, d, r)	((r).x = (v).x * (d) , \
				 			 (r).y = (v).y * (d) , \
				 			 (r).z = (v).z * (d) )

#define VCross(v1, v2, r)	((r).x = (v1).y * (v2).z - (v1).z * (v2).y , \
				 			 (r).y = (v1).z * (v2).x - (v1).x * (v2).z , \
				 			 (r).z = (v1).x * (v2).y - (v1).y * (v2).x )

#define VUnit(v, t, r)		((t) = 1 / VMod(v) , \
				 			 VScalarMul(v, t, r) )

#define MNew(a, b, c, r)	((r).x = (a), (r).y = (b) , (r).z = (c) )

#define MScalarMul(m, d, r)	(VScalarMul((m).x, (d), (r).x) , \
				 			 VScalarMul((m).y, (d), (r).y) , \
				 			 VScalarMul((m).z, (d), (r).z) )

#define MVMul(m, v, r)		((r).x = VDot((m).x, (v)) , \
				 			 (r).y = VDot((m).y, (v)) , \
				 			 (r).z = VDot((m).z, (v)) )

#define MTrans(m, n)		((n).x.x = (m).x.x , (n).y.x = (m).x.y , \
							 (n).z.x = (m).x.z , (n).x.y = (m).y.x , \
							 (n).y.y = (m).y.y , (n).z.y = (m).y.z , \
							 (n).x.z = (m).z.x , (n).y.z = (m).z.y , \
							 (n).z.z = (m).z.z )

/* To set the value on a quaternion. */
#define QNew(a,b,c,d,r)     ((r).real_part=(a),   \
                             (r).vect_part.x=(b), \
                             (r).vect_part.y=(c), \
                             (r).vect_part.z=(d) )

/* Takes the modulus of v */
#define QMod(q)		(sqrt(	  (q).real_part*(q).real_part + \
							(q).vect_part.x*(q).vect_part.x + \
							(q).vect_part.y*(q).vect_part.y + \
							(q).vect_part.z*(q).vect_part.z))

/* multiplies a quaternion by a scalar */
#define QScalarMul(q, d, r)	((r).real_part = (q).real_part * (d) , \
				 			 (r).vect_part.x = (q).vect_part.x * (d) , \
				 			 (r).vect_part.y = (q).vect_part.y * (d) , \
				 			 (r).vect_part.z = (q).vect_part.z * (d))

/* Unit-izes a quaternion */
#define QUnit(q, t, r)		((t) = 1.0 / QMod(q) , \
							 QScalarMul(q, t, r) )
/* #define QUnit(q, t, r)		( QScalarMul(q, (1.0/QMod(q)), r) ) */

/* Printing functions. */
#define VPrint(of, v)	(fprintf(of,"%1.15g %1.15g %1.15g\n",(v).x,(v).y,(v).z))
#define MPrint(of, m)	(VPrint(of,(m).x), VPrint(of,(m).y), VPrint(of,(m).z))
#define QPrint(of, q)   (fprintf(of,"%1.15g ",(q).real_part), VPrint(of,(q).vect_part))


/* Extern declarations for the rest. */
extern int    VRead(char *s, Vector* v);

/* Matrix functions. */
/* These too don't become macros easily.  */
extern Matrix MMMul(Matrix* m1, Matrix* m2);
extern Matrix MInvert(Matrix* m);


#endif
