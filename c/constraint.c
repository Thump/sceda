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
**	constraint.c : Constraint solver and other stuff.
*/


#include <math.h>
#include <sced.h>
#include <constraint.h>

/* A type for the constraint combination functions. */
typedef FeatureData	(*CombineConstraintFunction)(FeaturePtr, FeaturePtr);

static FeatureData	Combine_Constraints(FeaturePtr, FeaturePtr);

static FeatureData	Combine_Plane_Plane_Constraints(FeaturePtr, FeaturePtr);
static FeatureData	Combine_Plane_Line_Constraints(FeaturePtr, FeaturePtr);
static FeatureData	Combine_Plane_Point_Constraints(FeaturePtr, FeaturePtr);
static FeatureData	Combine_Line_Line_Constraints(FeaturePtr, FeaturePtr);
static FeatureData	Combine_Line_Point_Constraints(FeaturePtr, FeaturePtr);
static FeatureData	Combine_Point_Point_Constraints(FeaturePtr, FeaturePtr);
static FeatureData	Combine_Null_Constraints(FeaturePtr, FeaturePtr);
static FeatureData	Combine_Inconsistent_Constraints(FeaturePtr, FeaturePtr);

static CombineConstraintFunction	combination_functions[5][5] = {
	{ Combine_Null_Constraints, Combine_Null_Constraints,
	  Combine_Null_Constraints, Combine_Null_Constraints,
	  Combine_Inconsistent_Constraints },
	{ 0, Combine_Plane_Plane_Constraints, Combine_Plane_Line_Constraints,
	  Combine_Plane_Point_Constraints, Combine_Inconsistent_Constraints },
	{ 0, 0, Combine_Line_Line_Constraints, Combine_Line_Point_Constraints,
	  Combine_Inconsistent_Constraints },
	{ 0, 0, 0, Combine_Point_Point_Constraints,
	  Combine_Inconsistent_Constraints },
	{ Combine_Inconsistent_Constraints } };


static Boolean	Point_On_Plane(Vector, double, Vector);
static Boolean	Point_On_Line(Vector, Vector, Vector);

double	Distance_Point_To_Plane(Vector, Vector, Vector);
static Vector	Closest_Plane_Point(Vector, Vector, Vector);
static Vector	Closest_Line_Point(Vector, Vector, Vector);


#define Point_On_Point(p1, p2) \
		(DEqual(p1.x, p2.x) && DEqual(p1.y, p2.y) && DEqual(p1.z, p2.z))


/*	void
**	Constraint_Solve_System(FeaturePtr *cons, int num, FeaturePtr result)
**	Solves those constraints that are active and puts the
**	resulting constraint in result.
*/
void
Constraint_Solve_System(FeaturePtr *cons, int num, FeaturePtr result)
{
	int	i;

	result->f_type = null_feature;

	for ( i = 0 ; i < num ; i++ )
	{
		if ( cons[i]->f_status )
		{
			if ( result->f_type == null_feature )
				*result = *(cons[i]);
			else
				*result = Combine_Constraints(cons[i], result);
		}
		if ( result->f_type == inconsistent_feature )
			return;
	}
}


/*	FeatureData
**	Combine_Constraints(FeaturePtr c1, FeaturePtr c2)
**	Combines the 2 constraints.  Basically just calls the appropriate
**	feature pair routine.
*/
static FeatureData
Combine_Constraints(FeaturePtr c1, FeaturePtr c2)
{
	if ( c1->f_type <= c2->f_type )
		return combination_functions[c1->f_type][c2->f_type](c1, c2);
	else
		return combination_functions[c2->f_type][c1->f_type](c2, c1);
}


/*
**	The following Combine_????_????_Constraints functions do pretty much
**	what their names suggest.  They may return inconsistent_feature if required.
*/

static FeatureData
Combine_Null_Constraints(FeaturePtr c1, FeaturePtr c2)
{
	FeatureData	result;

	result.f_type = null_feature;
	return result;
}


static FeatureData
Combine_Inconsistent_Constraints(FeaturePtr c1, FeaturePtr c2)
{
	FeatureData	result;

	result.f_type = inconsistent_feature;
	return result;
}



static FeatureData
Combine_Plane_Plane_Constraints(FeaturePtr c1, FeaturePtr c2)
{
	FeatureData	result;
	Vector		cross;

	/* The result is inconsistent if the planes are parallel and not
	** coincident, a plane if they are parallel and coincident and 
	** their line of intersection otherwise.
	*/

	VCross(c1->f_vector, c2->f_vector, cross);
	if ( VZero(cross) )
	{
		/* The planes are parallel. */
		if ( Point_On_Plane(c1->f_vector, c1->f_value, c2->f_point) )
			return *c1;

		result.f_type = inconsistent_feature;
		return result;
	}

	result.f_type = line_feature;
	result.f_vector = cross;

	/* Project one of the plane points onto the line to get the point. */
	if ( ! IsZero(cross.z) )
	{
		result.f_point.x = ( c2->f_vector.y * c1->f_value -
							 c1->f_vector.y * c2->f_value ) / cross.z;
		result.f_point.y = ( c1->f_vector.x * c2->f_value -
							 c2->f_vector.x * c1->f_value ) / cross.z;
		result.f_point.z = 0;
	}
	else if ( ! IsZero(cross.y) )
	{
		result.f_point.x = ( c1->f_vector.z * c2->f_value -
							 c2->f_vector.z * c1->f_value ) / cross.y;
		result.f_point.y = 0;
		result.f_point.z = ( c2->f_vector.x * c1->f_value -
							 c1->f_vector.x * c2->f_value ) / cross.y;
	}
	else
	{
		result.f_point.x = 0;
		result.f_point.y = ( c2->f_vector.z * c1->f_value -
							 c1->f_vector.z * c2->f_value ) / cross.x;
		result.f_point.z = ( c1->f_vector.y * c2->f_value -
							 c2->f_vector.y * c1->f_value ) / cross.x;
	}

	/* Now project a plane point onto the line to get a better point. */
	/* The reasons relate to interactive dragging along a line constraint,
	** where the distance between the mouse and the origin pt of the line
	** affects the accuracy of the motion. */
	result.f_point = Closest_Line_Point(cross, result.f_point, c1->f_point);

	return result;

}


static FeatureData
Combine_Plane_Line_Constraints(FeaturePtr c1, FeaturePtr c2)
{
	double		dot;
	double		alpha;
	Vector		temp_v;
	FeatureData	result;

	/* The result is inconsistent if the line is parallel to but not in the
	** plane, the line if it is parallel and in the plane, or the point
	** of intersection otherwise.
	*/

	dot = VDot( c1->f_vector, c2->f_vector );

	if ( IsZero(dot) )
	{
		/* The line and plane are parallel. */
		if ( Point_On_Plane(c1->f_vector, c1->f_value, c2->f_point) )
		{
			/* The line lies in the plane. */
			return *c2;
		}
		else
		{
			result.f_type = inconsistent_feature;
			return result;
		}

	}

	/* Find the intersection point. */
	alpha = ( VDot(c1->f_vector, c1->f_point) -
			  VDot(c1->f_vector, c2->f_point) ) / dot;
	result.f_type = point_feature;
	VScalarMul(c2->f_vector, alpha, temp_v);
	VAdd(temp_v, c2->f_point, result.f_point);
	
	return result;
}


static FeatureData
Combine_Plane_Point_Constraints(FeaturePtr c1, FeaturePtr c2)
{
	FeatureData	result;

	/* The result is either inconsistent if the point is not in the plane,
	** or the point itself otherwise.
	*/
	if ( Point_On_Plane(c1->f_vector, c1->f_value, c2->f_point) )
		return *c2;

	result.f_type = inconsistent_feature;
	return result;


}


static FeatureData
Combine_Line_Line_Constraints(FeaturePtr c1, FeaturePtr c2)
{
	Vector		cross;
	double		param1;
	Vector		temp_v1, temp_v2, temp_v3, temp_v4;
	Vector		point1;
	FeatureData	result;

	/* The result is inconsistent if the lines don't intersect, a point
	** otherwise (their point of intersection.
	*/

	VCross(c1->f_vector, c2->f_vector, cross);

	if ( VZero(cross) )
	{
		/* The lines are parallel. */
		if ( Point_On_Line( c1->f_vector, c1->f_point,
							c2->f_point) )
			return *c1;
		else
		{
			result.f_type = inconsistent_feature;
			return result;
		}
	}

	/* They still may or may not intersect. */
	VSub(c1->f_point, c2->f_point, temp_v1);
	VCross(c2->f_vector, temp_v1, temp_v2);
	VCross(c1->f_vector, temp_v1, temp_v3);

	/* temp_v3 and temp_v2 are both perp to temp_v1. */
	/* If temp_v2 is 0, then the lines intersect at c1->f_point1. */
	/* If temp_v3 is 0, then the lines intersect at c1->f_point2. */
	/* If temp_v2 and temp_v3 are parallel, the lines intersect somewhere. */
	/* Otherwise they don't. */
	if ( VZero(temp_v2) )
	{
		result.f_type = point_feature;
		result.f_point = c1->f_point;
		return result;
	}
	else if ( VZero(temp_v2) )
	{
		result.f_type = point_feature;
		result.f_point = c2->f_point;
		return result;
	}

	VCross(temp_v2, temp_v3, temp_v4);
	if ( ! VZero(temp_v4) )
	{
		result.f_type = inconsistent_feature;
		return result;
	}


	/* Get the parameter for the point of intersection. */
	if ( ! IsZero(cross.z) )
		param1 = temp_v2.z / cross.z;
	else if ( ! IsZero(cross.y) )
		param1 = temp_v2.y / cross.y;
	else
		param1 = temp_v2.x / cross.x;

	VScalarMul(c1->f_vector, param1, temp_v1);
	VAdd(temp_v1, c1->f_point, point1);

	result.f_type = point_feature;
	result.f_point = point1;
	return result;
}


static FeatureData
Combine_Line_Point_Constraints(FeaturePtr c1, FeaturePtr c2)
{
	FeatureData	result;

	/* The result is inconsistent if the point is not on the line,
	** the point otherwise.
	*/

	if ( Point_On_Line(c1->f_vector, c1->f_point, c2->f_point) )
		return *c2;

	result.f_type = inconsistent_feature;
	return result;


}


static FeatureData
Combine_Point_Point_Constraints(FeaturePtr c1, FeaturePtr c2)
{
	FeatureData	result;

	/* The result is inconsistent unless the points are the same. */

	if ( Point_On_Point(c1->f_point, c2->f_point) )
		result = *c1;
	else
		result.f_type = inconsistent_feature;

	return result;
}




/*	Boolean
**	Point_On_Plane(Vector normal, double plane_pt, Vector pt)
**	Returns TRUE if the point pt lies in the plane defined by plane_pt and
**	normal.
*/
static Boolean
Point_On_Plane(Vector normal, double plane_pt, Vector pt)
{
	double	temp1 = VDot(normal, pt);

	return DEqual(temp1, plane_pt);
}


/*	Boolean
**	Point_On_Line(Vector dir, Vector line_pt, Vector pt)
**	Returns TRUE if the point pt lies in the line defined by line_pt and dir.
*/
static Boolean
Point_On_Line(Vector dir, Vector line_pt, Vector pt)
{
	double	alpha;
	double	temp1, temp2;

	if ( ! IsZero(dir.x) )
	{
		alpha = ( pt.x - line_pt.x ) / dir.x;
		temp1 = dir.y * alpha + line_pt.y;
		temp2 = dir.z * alpha + line_pt.z;
		return ( DEqual(temp1, pt.y) && DEqual(temp2, pt.z) );
	}
	else if ( ! IsZero(dir.y) )
	{
		alpha = ( pt.y - line_pt.y ) / dir.y;
		temp1 = dir.z * alpha + line_pt.z;
		return ( DEqual(line_pt.x, pt.x) && DEqual(temp1, pt.z) );
	}
	else if ( ! IsZero(dir.z) )
		return ( DEqual(line_pt.x, pt.x) && DEqual(line_pt.y, pt.y) );

	return FALSE;

}


/*	Boolean
**	Point_Satisfies_Constraint(Vector pt, FeaturePtr con)
**	Returns TRUE if the point pt satisfies the constraint.
*/
Boolean
Point_Satisfies_Constraint(Vector pt, FeaturePtr con)
{
	switch ( con->f_type )
	{
		case plane_feature:
			return Point_On_Plane(con->f_vector, con->f_value, pt);

		case line_feature:
			return Point_On_Line(con->f_vector, con->f_point, pt);

		case point_feature:
			return Point_On_Point(con->f_point, pt);

		default: return TRUE;
	}

	return TRUE; /* To keep compilers happy. */

}


double
Distance_Point_To_Plane(Vector norm, Vector plane_pt, Vector pt)
{
	Vector	temp_v;

	VSub(pt, plane_pt, temp_v);
	return VDot(temp_v, norm);
}

/*	Vector
**	Find_Required_Motion(Vector pt, FeaturePtr con)
**	Returns the displacement required to make the pt satisfy the constraint.
*/
Vector
Find_Required_Motion(Vector pt, FeaturePtr con)
{
	Vector	result;
	Vector	temp_v;

	switch ( con->f_type )
	{
		case plane_feature:
			temp_v = Closest_Plane_Point(con->f_vector, con->f_point, pt);
			break;
	
		case line_feature:
			temp_v = Closest_Line_Point(con->f_vector, con->f_point, pt);
			break;

		case point_feature:
			temp_v = con->f_point;
			break;

		default: VNew(0, 0, 0, result);
	}

	VSub(temp_v, pt, result);
	return result;

}


static Vector
Closest_Plane_Point(Vector norm, Vector plane_pt, Vector pt)
{
	Vector	result;

	/* Need to find the perpendicular from the point to the plane. */
	double	distance = Distance_Point_To_Plane(norm, plane_pt, pt);
	VScalarMul(norm, distance, result);
	VSub(pt, result, result);

	return result;
}


static Vector
Closest_Line_Point(Vector dir, Vector line_pt, Vector pt)
{
	Vector	result;
	double	temp_d;

	/* Need to find a perpendicular from the point to the line. */
	VSub(pt, line_pt, result);
	temp_d = VDot(result, dir) /
			 VDot(dir, dir);
	VScalarMul(dir, temp_d, result);
	VAdd(result, line_pt, result);

	return result;
}


static void
Constraint_Update_Spec(FeatureSpecPtr spec, ObjectInstancePtr target,
						void *ptr, int abs)
{
	Vector	temp;
	ObjectInstancePtr	src = (ObjectInstancePtr)ptr;

	if ( spec->spec_type == reference_spec && spec->spec_object == target )
	{
		Edit_Remove_Obj_From_Dependencies(spec, src, NULL, 0);

		if ( abs )
		{
			spec->spec_type = absolute_spec;
			Transform_Vector(target->o_transform, spec->spec_vector,
							 spec->spec_vector);
		}
		else
		{
			spec->spec_type = offset_spec;
			Transform_Vector(target->o_transform, spec->spec_vector, temp);
			VSub(temp, src->o_world_verts[src->o_num_vertices - 1],
				 spec->spec_vector);
		}
	}
}


void
Constraint_Remove_References(ObjectInstancePtr src, ObjectInstancePtr obj)
{
	int	i;

	for ( i = 0 ; i < src->o_origin_num ; i++ )
		Constraint_Manipulate_Specs(src->o_origin_cons + i, obj, (void*)src, 1,
									Constraint_Update_Spec);
	for ( i = 0 ; i < src->o_scale_num ; i++ )
		Constraint_Manipulate_Specs(src->o_scale_cons + i, obj, (void*)src, 0,
									Constraint_Update_Spec);
	for ( i = 0 ; i < src->o_rotate_num ; i++ )
		Constraint_Manipulate_Specs(src->o_rotate_cons + i, obj, (void*)src, 0,
									Constraint_Update_Spec);
	Constraint_Manipulate_Specs(&(src->o_major_align), obj, (void*)src, 0,
								Constraint_Update_Spec);
	Constraint_Manipulate_Specs(&(src->o_minor_align), obj, (void*)src, 0,
								Constraint_Update_Spec);
}


void
Constraint_Manipulate_Specs(FeaturePtr feat, ObjectInstancePtr obj,
							void *ptr, int num, SpecFunction func)
{
	switch ( feat->f_spec_type )
	{
		case plane_feature:
		case axis_plane_feature:
		case orig_plane_feature:
		case ref_plane_feature:
			func(feat->f_specs + 2, obj, ptr, num);
		case line_feature:
		case axis_feature:
		case midpoint_feature:
		case midplane_feature:
		case orig_line_feature:
		case ref_line_feature:
			func(feat->f_specs + 1, obj, ptr, num);
		case point_feature:
			func(feat->f_specs, obj, ptr, num);
		default:;
	}
}

