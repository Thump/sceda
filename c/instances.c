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
**	instances.c : Functions for instantiating and modifying object instances.
**
**	Created: 13/03/94
**
**	External functions:
**
**	ObjectInstancePtr
**	Create_Instance(BaseObjectPtr base, String label)
**	Creates an instance of base type type base with name label.
**	Returns the new object, or NULL on failure.
**
**	ObjectInstancePtr
**	Copy_Object_Instance(ObjectInstancePtr orig)
**	Copies all of orig, giving the copy a new name.  Returns NULL on failure.
**
**	void
**	Rename_Instance(ObjectInstancePtr, char*)
**	Changes the existing label on obj to label.
**
**	int
**	Transform_Instance(ObjectInstancePtr, Transformation*, Boolean)
**	Transforms object according to tranform.
**
**	void
**	Modify_Instance_Attributes(ObjectInstancePtr, Attributes*, int flag)
**	Modifies the attributes of obj to match new.
**
**	void
**	Destroy_Instance(ObjectInstancePtr)
**	Removes obj from its parent and frees all the memory associated with obj.
*/

#include <math.h>
#include <sced.h>
#include <layers.h>
#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif


extern void	Add_Dependency(ObjectInstancePtr, ObjectInstancePtr);

static void	Instance_Set_Constraint(ObjectInstancePtr);
void	Copy_Instance_Constraints(ObjectInstancePtr, ObjectInstancePtr);

/*	ObjectInstancePtr
**	Create_Instance(BaseObjectPtr base, String label)
**
**	Creates an instance of base type type base with name label.
**	The object has the identity transformation installed.
**	ONLY world vertices are filled in, and the bounding box is for these
**	coordinates.
**	The object has default attributes as inheritted from base.
**	Returns the new object, or NULL on failure.
*/
ObjectInstancePtr
Create_Instance(BaseObjectPtr base, String label)
{
	ObjectInstancePtr	res;
	short				num_verts;
	short				num_faces;
	int					i;

	/* Create the instance and allocate lots of memory. */
	res = New(ObjectInstance, 1);

	object_count[base->b_class]++;

	res->o_label = Strdup(label);

	/* Install it in it's parents instance list. */
	Add_Instance_To_Base(res, base);
	res->o_parent = base;
	res->o_wireframe = base->b_wireframe;

	num_verts = res->o_wireframe->num_vertices;
	num_faces = res->o_wireframe->num_faces;

	/* Set the transformations to the identity. */
	NewIdentityMatrix(res->o_transform.matrix);
	NewIdentityMatrix(res->o_inverse.matrix);
	VNew(0.0, 0.0, 0.0, res->o_transform.displacement);
	VNew(0.0, 0.0, 0.0, res->o_inverse.displacement);
	
	/* Set the initial rotation and scale to 0 or 1, as needed. */
	QNew(1.0, 0.0, 0.0, 0.0, res->o_rot);
	VNew(1.0, 1.0, 1.0, res->o_scale);

	/* Set posspline and rotspline to NULL: prolly not needed, but... */
	res->o_rotspline=NULL;
	res->o_posspline=NULL;

	/* Set all the attributes to the defaults. */
	if ( base->b_class == light_obj ||
		 base->b_class == spotlight_obj ||
		 base->b_class == arealight_obj )
	{
		res->o_attribs = (void*)New(LightInfo, 1);
		((LightInfoPtr)res->o_attribs)->red =
		((LightInfoPtr)res->o_attribs)->green =
		((LightInfoPtr)res->o_attribs)->blue = 1.0;
		if ( base->b_class == spotlight_obj )
		{
			((LightInfoPtr)res->o_attribs)->val1 = 1.0;
			((LightInfoPtr)res->o_attribs)->val2 = 10.0;
		}
		else if ( base->b_class == arealight_obj )
		{
			((LightInfoPtr)res->o_attribs)->val1 = 4.0;
			((LightInfoPtr)res->o_attribs)->val2 = 4.0;
		}
		else
			((LightInfoPtr)res->o_attribs)->val1 =
			((LightInfoPtr)res->o_attribs)->val2 = 0.0;
		((LightInfoPtr)res->o_attribs)->flag = FALSE;
	}
	else
	{
		res->o_attribs = (void*)New(Attributes, 1);
		*((AttributePtr)res->o_attribs) = default_attributes;
		((AttributePtr)res->o_attribs)->defined = FALSE;
	}

	/* It also starts in the default (world) layer. */
	res->o_layer = 0;
	Layer_Add_Instance(NULL, res->o_layer, res);

	/* Copy the world  vertices over from the parent. */
	res->o_num_vertices = num_verts;
	res->o_world_verts = New(Vector, num_verts);
	res->o_main_verts = New(Vertex, num_verts);
	for ( i = 0 ; i < res->o_num_vertices ; i++ )
		res->o_world_verts[i] = res->o_wireframe->vertices[i];

	/* Copy the world normals over also. */
	res->o_num_faces = num_faces;
	res->o_normals = New(Vector, num_faces);
	for ( i = 0 ; i < res->o_num_faces ; i++ )
		res->o_normals[i] = res->o_wireframe->faces[i].normal;

	/* Every body has the same default body axes.	*/
	VNew(0, 0, 1, res->o_axes.x);
	VNew(1, 0, 0, res->o_axes.y);
	VNew(0, 1, 0, res->o_axes.z);

	/* Everyone starts with their origin at their center. */
	VNew(0, 0, 0, res->o_origin);

	/* All objects start with the default position and rotate constraints. */
	res->o_origin_num = res->o_rotate_num = 0;
	res->o_origin_cons = res->o_rotate_cons = NULL;

	/* There are no special alignments for new objects. */
	res->o_major_align.f_type = res->o_major_align.f_spec_type =
	res->o_minor_align.f_type = res->o_minor_align.f_spec_type = null_feature;

	/* Nothing depends on a new object. */
	res->o_num_depend = 0;
	res->o_dependents = NULL;

	/* All constraints are inactive. */
	res->o_origin_active = New(Boolean, 3);
	for ( i = 0 ; i < 3 ; res->o_origin_active[i] = FALSE, i++ );
	res->o_scale_active = New(Boolean, 4);
	for ( i = 0 ; i < 4 ; res->o_scale_active[i] = FALSE, i++ );
	res->o_rotate_active = New(Boolean, 3);
	for ( i = 0 ; i < 3 ; res->o_rotate_active[i] = FALSE, i++ );

	/* Set type dependent information - reference and scale cons. */
	res->o_scale_num = 1;
	res->o_scale_cons = New(FeatureData, 1);
	Instance_Set_Constraint(res);

	res->o_flags = 0;
	res->o_dfs_mark = 0;

	return res;
}


/*	void
**	Instance_Set_Constraint(ObjectInstancePtr res)
**	Sets the additional scaling constraint that all bodies have, the one
**	for uniform scaling.
**	The constraint is a line from the center of the body coords out to
**	the reference point. It is a reference specified constraint, so it
**	moves and changes with the body.
*/
static void
Instance_Set_Constraint(ObjectInstancePtr res)
{
	FeaturePtr	f = res->o_scale_cons;
	double		temp_d;

	res->o_reference = res->o_world_verts[res->o_parent->b_ref_num];
	f->f_type = line_feature;
	f->f_spec_type = line_feature;
	f->f_label = Strdup("Uniform");
	if ( VZero(res->o_reference) )
		VNew(1, 1, 1, f->f_vector);
	else
		VUnit(res->o_reference, temp_d, f->f_vector);
	f->f_point = res->o_origin;
	f->f_value = 0;

	/* Its first dependency is on the origin of the body. This is
	** a special origin dependency. */
	f->f_specs[0].spec_type = origin_spec;
	f->f_specs[0].spec_object = NULL;
	f->f_specs[0].spec_vector = res->o_origin;

	/* The second dependency is on the reference point. */
	f->f_specs[1].spec_type = ref_point_spec;
	f->f_specs[1].spec_object = NULL;
	f->f_specs[1].spec_vector = res->o_reference;
}



/*	ObjectInstancePtr
**	Copy_Object_Instance(ObjectInstancePtr orig)
**	Copies all of orig, giving the copy a new name.  Returns NULL on failure.
*/
ObjectInstancePtr
Copy_Object_Instance(ObjectInstancePtr orig)
{
	ObjectInstancePtr	res;
	int					i;
	char				*label;

	/* Create the new label. */
	label = New(char, strlen(orig->o_label) + 10);
	sprintf(label, "%s_%d", orig->o_label,
			object_count[orig->o_parent->b_class]);
	res = Create_Instance(orig->o_parent, label);
	free(label);

	/* Copy the wireframe. If it's different (dense) then realloc vertices
	** and normals.
	*/
	if ( res->o_wireframe != orig->o_wireframe )
	{
		free(res->o_world_verts);
		free(res->o_main_verts);
		free(res->o_normals);
		res->o_wireframe = orig->o_wireframe;
		res->o_num_vertices = orig->o_num_vertices;
		res->o_num_faces = orig->o_num_faces;
		res->o_world_verts = New(Vector, res->o_num_vertices);
		res->o_main_verts = New(Vertex, res->o_num_vertices);
		res->o_normals = New(Vector, res->o_num_faces);
	}

	/* Copy the transformations. */
	res->o_transform = orig->o_transform;
	res->o_inverse = orig->o_inverse;

	/* Copy some of the animate specific stuff... */
	res->o_rot = orig->o_rot;
	res->o_scale = orig->o_scale;

	/* Set all the attributes to the parent's defaults. */
	if ( res->o_parent->b_class == light_obj ||
		 res->o_parent->b_class == spotlight_obj ||
		 res->o_parent->b_class == arealight_obj )
		*((LightInfoPtr)res->o_attribs) = *((LightInfoPtr)orig->o_attribs);
	else
		*((AttributePtr)res->o_attribs) = *((AttributePtr)orig->o_attribs);

	/* Copy the world  vertices over. */
	for ( i = 0 ; i < res->o_num_vertices ; i++ )
	{
		res->o_world_verts[i] = orig->o_world_verts[i];
		res->o_main_verts[i] = orig->o_main_verts[i];
	}

	/* Copy the world normals over also. */
	for ( i = 0 ; i < res->o_num_faces ; i++ )
		res->o_normals[i] = orig->o_normals[i];

	/* Copy the Constraint information. */
	Copy_Instance_Constraints(orig, res);

	if ( res->o_layer != orig->o_layer )
	{
		Layer_Remove_Instance(NULL, res->o_layer, res);
		res->o_layer = orig->o_layer;
		Layer_Add_Instance(NULL, res->o_layer, res);
	}

	return res;
}


static void
Instance_Add_Dependencies(FeatureSpecPtr spec, ObjectInstancePtr obj,
						  void *ptr, int dummy)
{
	ObjectInstancePtr	orig = (ObjectInstancePtr)ptr;

	if ( spec->spec_type == reference_spec )
	{
		if ( spec->spec_object == orig )
			spec->spec_object = obj;
		Add_Dependency(spec->spec_object, obj);
	}
}


/*	void
**	Copy_Instance_Constraints(ObjectInstancePtr src, ObjectInstancePtr dest)
**	Copies all the constraint from an object.
*/
void
Copy_Instance_Constraints(ObjectInstancePtr src, ObjectInstancePtr dest)
{
	int	i;

	dest->o_origin = src->o_origin;
	dest->o_reference = src->o_reference;
	dest->o_axes = src->o_axes;

	free(dest->o_scale_cons);
	free(dest->o_origin_active);
	free(dest->o_scale_active);
	free(dest->o_rotate_active);

	/* Copy origin constraints. */
	dest->o_origin_num = src->o_origin_num;
	if ( dest->o_origin_num )
	{
		dest->o_origin_cons = New(FeatureData, dest->o_origin_num);
		for ( i = 0 ; i < dest->o_origin_num ; i++ )
			dest->o_origin_cons[i] = src->o_origin_cons[i];
	}
	dest->o_origin_active = New(Boolean, dest->o_origin_num + 3);
	for ( i = 0 ; i < dest->o_origin_num + 3 ; i++ )
		dest->o_origin_active[i] = src->o_origin_active[i];

	/* Copy scale constraints. */
	dest->o_scale_num = src->o_scale_num;
	if ( dest->o_scale_num )
	{
		dest->o_scale_cons = New(FeatureData, dest->o_scale_num);
		for ( i = 0 ; i < dest->o_scale_num ; i++ )
			dest->o_scale_cons[i] = src->o_scale_cons[i];
	}
	dest->o_scale_active = New(Boolean, dest->o_scale_num + 3);
	for ( i = 0 ; i < dest->o_scale_num + 3 ; i++ )
		dest->o_scale_active[i] = src->o_scale_active[i];

	/* Copy rotate constraints. */
	dest->o_rotate_num = src->o_rotate_num;
	if ( dest->o_rotate_num )
	{
		dest->o_rotate_cons = New(FeatureData, dest->o_rotate_num);
		for ( i = 0 ; i < dest->o_rotate_num ; i++ )
			dest->o_rotate_cons[i] = src->o_rotate_cons[i];
	}
	dest->o_rotate_active = New(Boolean, dest->o_rotate_num + 3);
	for ( i = 0 ; i < dest->o_rotate_num + 3 ; i++ )
		dest->o_rotate_active[i] = src->o_rotate_active[i];

	/* Copy alignment info. */
	if ( src->o_major_align.f_type != null_feature )
		dest->o_major_align = src->o_major_align;
	if ( src->o_minor_align.f_type != null_feature )
		dest->o_minor_align = src->o_minor_align;

	/* Need to add the copy to the dependents list of all the things it
	** depends on.
	*/
	for ( i = 0 ; i < dest->o_origin_num ; i++ )
		Constraint_Manipulate_Specs(dest->o_origin_cons + i, dest, (void*)src,
									0, Instance_Add_Dependencies);
	for ( i = 0 ; i < dest->o_scale_num ; i++ )
		Constraint_Manipulate_Specs(dest->o_scale_cons + i, dest, (void*)src,
									0, Instance_Add_Dependencies);
	for ( i = 0 ; i < dest->o_rotate_num ; i++ )
		Constraint_Manipulate_Specs(dest->o_rotate_cons + i, dest, (void*)src,
									0, Instance_Add_Dependencies);
	Constraint_Manipulate_Specs(&(dest->o_major_align), dest, (void*)src,
								0, Instance_Add_Dependencies);
	Constraint_Manipulate_Specs(&(dest->o_minor_align), dest, (void*)src,
								0, Instance_Add_Dependencies);
}


/*	void
**	Rename_Instance(ObjectInstancePtr obj, char *label)
**	Changes the existing label on obj to label.
*/
void
Rename_Instance(ObjectInstancePtr obj, char *label)
{
	free(obj->o_label);
	obj->o_label = Strdup(label);

	changed_scene = TRUE;
}


/*	int
**	Transform_Instance(ObjectInstancePtr obj, Transformation *transform,
**						Boolean replace)
**	Transforms object according to tranform.  If replace is True, overwrites
**	the existing transformation, otherwise multiplies onto it.
**	Returns 0 if the resulting transformation is singular, and leaves obj
**	unchanged.
*/
int
Transform_Instance(ObjectInstancePtr obj, Transformation *transform,
					Boolean replace)
{
	Transformation	existing = obj->o_transform;
	Matrix			transp;
	int				i;


	if (replace)
		obj->o_transform = *transform;
	else
	{
		obj->o_transform.matrix =
			MMMul(&(transform->matrix), &(existing.matrix));
		VAdd(transform->displacement, existing.displacement,
			obj->o_transform.displacement);
	}

	/* Calculate the inverse transformation. */
	obj->o_inverse.matrix = MInvert(&(obj->o_transform.matrix));
	if ( MZero(obj->o_inverse.matrix) )
	{
		obj->o_transform = existing;
		return FALSE;
	}
	VScalarMul(obj->o_transform.displacement, -1,
											obj->o_inverse.displacement);

	/* Transform all the vertices. */
	for ( i = 0 ; i < obj->o_num_vertices ; i++ )
	{
		MVMul(obj->o_transform.matrix,
			  obj->o_wireframe->vertices[i], obj->o_world_verts[i]);
		VAdd(obj->o_transform.displacement,
			 obj->o_world_verts[i], obj->o_world_verts[i]);
	}

	/* Calculate the face normals. */
	/* To transform a normal multiply it by the transpose of the inverse	*/
	/* transformation matrix.												*/
	MTrans(obj->o_inverse.matrix, transp);
	for ( i = 0 ; i < obj->o_num_faces ; i++ )
		MVMul(transp, obj->o_wireframe->faces[i].normal, obj->o_normals[i]);

	return TRUE;

}


/*	void
**	Displace_Instance(ObjectInstancePtr obj, Vector displacement)
**	Displaces the instance by displacement.  Its an additive displacement.
*/
void
Displace_Instance(ObjectInstancePtr obj, Vector displacement)
{
	int		i;


	VAdd(obj->o_transform.displacement, displacement,
		 obj->o_transform.displacement);
	VSub(obj->o_inverse.displacement, displacement,
		 obj->o_inverse.displacement);

	/* Transform all the vertices. */
	for ( i = 0 ; i < obj->o_num_vertices ; i++ )
	{
		VAdd(displacement, obj->o_world_verts[i], obj->o_world_verts[i]);
	}

}

/*	void
**	Modify_Instance_Attributes(ObjectInstancePtr obj, Attributes *new, int flag)
**	Modifies the attributes of obj indicated by flags to match new.
*/
void
Modify_Instance_Attributes(ObjectInstancePtr obj, AttributePtr new, int flag)
{
	/* Always modify use_extension. */
	((AttributePtr)obj->o_attribs)->defined = new->defined;
	((AttributePtr)obj->o_attribs)->use_extension = new->use_extension;

	if ( flag & ModSimple )
	{
		((AttributePtr)obj->o_attribs)->colour = new->colour;
		((AttributePtr)obj->o_attribs)->diff_coef = new->diff_coef;
		((AttributePtr)obj->o_attribs)->spec_coef = new->spec_coef;
		((AttributePtr)obj->o_attribs)->spec_power =  new->spec_power;
		((AttributePtr)obj->o_attribs)->reflect_coef = new->reflect_coef;
		((AttributePtr)obj->o_attribs)->transparency = new->transparency;
		((AttributePtr)obj->o_attribs)->refract_index = new->refract_index;
	}

	if ( flag & ModExtend )
	{
		if ( ((AttributePtr)obj->o_attribs)->extension )
			free(((AttributePtr)obj->o_attribs)->extension);
		((AttributePtr)obj->o_attribs)->extension = new->extension;
		((AttributePtr)obj->o_attribs)->use_obj_trans = new->use_obj_trans;
		((AttributePtr)obj->o_attribs)->open = new->open;
	}

	changed_scene = TRUE;
}



/*	void
**	Destroy_Instance(ObjectInstancePtr obj)
**	Removes obj from its parent and frees all the memory associated with obj.
*/
void
Destroy_Instance(ObjectInstancePtr obj)
{
	int	i;

	Remove_Instance_From_Base(obj);

	/* Remove any dependencies on and for this object. */
	for ( i = 0 ; i < obj->o_origin_num ; i++ )
		Constraint_Manipulate_Specs(obj->o_origin_cons + i, obj, NULL, 1,
									Edit_Remove_Obj_From_Dependencies);
	for ( i = 0 ; i < obj->o_scale_num ; i++ )
		Constraint_Manipulate_Specs(obj->o_scale_cons + i, obj, NULL, 1,
									Edit_Remove_Obj_From_Dependencies);
	for ( i = 0 ; i < obj->o_rotate_num ; i++ )
		Constraint_Manipulate_Specs(obj->o_rotate_cons + i, obj, NULL, 1,
									Edit_Remove_Obj_From_Dependencies);
	Constraint_Manipulate_Specs(&(obj->o_major_align), obj, NULL, 1,
								Edit_Remove_Obj_From_Dependencies);
	Constraint_Manipulate_Specs(&(obj->o_minor_align), obj, NULL, 1,
								Edit_Remove_Obj_From_Dependencies);
	for ( i = obj->o_num_depend - 1 ; i >= 0 ; i-- )
		Constraint_Remove_References(obj->o_dependents[i].obj, obj);

	Layer_Remove_Instance(NULL, obj->o_layer, obj);

	free(obj->o_label);
	free(obj->o_world_verts);
	free(obj->o_main_verts);
	if ( obj->o_origin_num )
		free(obj->o_origin_cons);
	if ( obj->o_scale_num )
		free(obj->o_scale_cons);
	if ( obj->o_rotate_num )
		free(obj->o_rotate_cons);
	free(obj->o_origin_active);
	free(obj->o_scale_active);
	free(obj->o_rotate_active);
	free(obj);
}

