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
**	enum.h : A File containing typedefs for the various enumerated types.
**
**	Created: 26/03/94
*/

#ifndef _SCED_ENUM_
#define _SCED_ENUM_

/*	The types of editing. */
typedef enum _EditPrevious {
	none_previous, place_previous, scale_previous, rotate_previous
	} EditPrevious;

/* The raytracers supported. */
typedef enum _Raytracer {
	NoTarget, POVray, Rayshade, Radiance, Genray, Genscan, Renderman
	} Raytracer;

/* The types of transformations.  */
typedef enum _TransformType {
	rotate_transform, scale_transform
	} TransformType;


/* The generic objects. */
typedef enum _GenericObject {
	cube_obj,
	sphere_obj,
	cylinder_obj,
	cone_obj,
	square_obj,
	plane_obj,
	light_obj,
	spotlight_obj,
	arealight_obj,
	csg_obj,
	wireframe_obj
	} GenericObject;
#define NUM_GENERIC_OBJS 9


/*	The types of feature specifiers.	*/
typedef enum _FeatureSpecType {
	absolute_spec,
	offset_spec,
	reference_spec,
	origin_spec,
	ref_point_spec
	} FeatureSpecType;

/*	The various type of constraints.	*/
typedef enum _FeatureType {
	null_feature,
	plane_feature,
	line_feature,
	point_feature,
	inconsistent_feature,
	midplane_feature,
	midpoint_feature,
	axis_plane_feature,
	axis_feature,
	orig_line_feature,
	ref_line_feature,
	orig_plane_feature,
	ref_plane_feature
	} FeatureType;
#define NUM_CONSTRAINT_TYPES 13


#endif
