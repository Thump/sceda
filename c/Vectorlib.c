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
/*	Vector functions.											*/
/*	Matrix manipulation functions.								*/
/*	Written by Stephen Chenney. Dec 1993.						*/
/*																*/
/****************************************************************/
/*																*/
/*																*/
/****************************************************************/

#include <stdio.h> /* for the NULL definition */
#include <math.h>
#include <Vector.h>


/* A small number for floating point comparisons. */
/* A floating point == 0.0 function. */
#define IsZero(f)	(((f) < 1.e-12) && ((f) > -1.e-12))


int
VRead(char *s, Vector* v)
/****************************************************************/
/*																*/
/*	Reads a vector from a file. (may be stdin).					*/
/*	Returns 0 on failure, 1 otherwise.							*/
/*																*/
/****************************************************************/
{
	if ((sscanf(s, "%lf %lf %lf", &((*v).x), &((*v).y), &((*v).z))) != 3)
		return 0;
	else
		return 1;
}




Matrix
MMMul(Matrix *m1, Matrix *m2)
/****************************************************************/
/*																*/
/*	Multiplies 2 matrices: r = m1m2.							*/
/*																*/
/****************************************************************/
{
	Matrix r;

	/* This is ugly code, caused by the vector structure. */

	Vector col1, col2, col3;  /* Columns of m2. */
	Vector row1, row2, row3;  /* Rows of m1. */

	/* Get the columns of m2, then it's easy. */
	col1.x = m2->x.x; col1.y = m2->y.x; col1.z = m2->z.x;
	col2.x = m2->x.y; col2.y = m2->y.y; col2.z = m2->z.y;
	col3.x = m2->x.z; col3.y = m2->y.z; col3.z = m2->z.z;
	/* Need copies of the rows of m1 because they may be written over. */
	row1 = m1->x; row2 = m1->y ; row3 = m1->z;

	r.x.x = VDot(row1, col1);
	r.x.y = VDot(row1, col2);
	r.x.z = VDot(row1, col3);
	r.y.x = VDot(row2, col1);
	r.y.y = VDot(row2, col2);
	r.y.z = VDot(row2, col3);
	r.z.x = VDot(row3, col1);
	r.z.y = VDot(row3, col2);
	r.z.z = VDot(row3, col3);

	return r;
}


Matrix
MInvert(Matrix *m)
/****************************************************************/
/*																*/
/*	Inverts the matrix m and puts it in r.						*/
/*	If the matrix m is singular, returns the zero matrix. 		*/
/*																*/
/****************************************************************/
{
	Matrix r;

	/* This does it by the "formula" method.*/
	/* a[i][j] = det(adjoint[i][j])/det(b)	*/

	double temp;	/* for the determinant. */
	double a11, a22, a33; /* Various adjoint determinants. */

	a11 = m->y.y * m->z.z - m->y.z * m->z.y;
	a22 = m->x.y * m->z.z - m->x.z * m->z.y;
	a33 = m->x.y * m->y.z - m->x.z * m->y.y;
	temp =	m->x.x * a11 - m->y.x * a22 + m->z.x * a33;
	
	/* Test for singularity. */
	if (IsZero(temp))
	{
		r.x.x=r.x.y=r.x.z=r.y.x=r.y.y=r.y.z=r.z.x=r.z.y=r.z.z=0;
		return r;
	}

	r.x.x = a11 / temp;
	r.x.y = -a22 / temp;
	r.x.z = a33 / temp;
	r.y.x = (m->y.z * m->z.x - m->y.x * m->z.z) / temp;
	r.y.y = (m->x.x * m->z.z - m->x.z * m->z.x) / temp;
	r.y.z = (m->x.z * m->y.x - m->x.x * m->y.z) / temp;
	r.z.x = (m->y.x * m->z.y - m->y.y * m->z.x) / temp;
	r.z.y = (m->x.y * m->z.x - m->x.x * m->z.y) / temp;
	r.z.z = (m->x.x * m->y.y - m->x.y * m->y.x) / temp;

	return r;
}



