#include <math.h>
#include "elk_private.h"

Object	Sym_Union;
Object	Sym_Intersection;
Object	Sym_Difference;


/***********************************************************************
 *
 * Description: elk_object3d_print() is used internally by the scheme
 *		interpreter. This function is called whenever a user
 *		tries to print an object from scheme.
 *
 * Scheme example: (display (object3d-create "sphere")) => #[object3 0x1a4c00]
 *
 ***********************************************************************/
int
elk_object3d_print(Object w, Object port, int raw, int depth, int len)
{
	ObjectInstancePtr object3 = (ObjectInstancePtr) ELKOBJECT3(w)->object3;
	
	Printf(port, "#[object3 0x%x]\n", object3);
	return 0;
}

/***********************************************************************
 *
 * Description: elk_object3d_equal() is used internally by the scheme
 *		interpreter. This function is called whenever two objects
 *		are compared in scheme using the "equal?" operator.
 *
 * Scheme example: (equal?
 *			(object3d-create "sphere")
 *			(object3d-create "sphere"))
 *
 ***********************************************************************/
int
elk_object3d_equal(Object a, Object b)
{
	return (ELKOBJECT3(a)->object3 == ELKOBJECT3(b)->object3);
}

/***********************************************************************
 *
 * Description: elk_object3d_equiv() is used internally by the scheme
 *		interpreter. This function is called whenever two objects
 *		are compared in scheme using the "eqv?" operator.
 *
 * Scheme example: (eqv?
 *			(object3d-create "sphere")
 *			(object3d-create "sphere"))
 *
 ***********************************************************************/
int
elk_object3d_equiv(Object a, Object b)
{
	return (ELKOBJECT3(a)->object3 == ELKOBJECT3(b)->object3);
}

/***********************************************************************
 *
 * Description: elk_object3d_create() is the C callback for the scheme
 * 		function "object3d-create".
 *
 * Parameter:	strobj - string object specifying type of object to
 *		be created. This parameter sould match the name of one of the
 *      base objects.
 *
 * Scheme example: (object3d-create "Sphere")
 *
 * Return value: Returns the newly created object. This object may be
 *		 passed to other scheme functions that accept an object
 *		 parameter.
 *
 ***********************************************************************/
Object
elk_object3d_create(Object strobj)
{
	Object obj;
	char *objtype;
	ObjectInstancePtr new_obj;
	BaseObjectPtr base_obj;
	WindowInfo *window;

	window = (elk_is_csg_window) ? &csg_window : &main_window;
	/*
	 * First check to make sure that the type of strobj
	 * is a string.
	 */
	Check_Type(strobj, T_String);
	
	/*
	 * Next we allocate an Elk object to save the real object in.
	 */
	obj = Alloc_Object(sizeof(Elkobject3), OBJECT3_TYPE, 0);
	objtype = STRING(strobj)->data;
	
	/*
	 * Determine the type of base object to use.
	 */
	if ( ! ( base_obj = Get_Base_Object_From_Label(objtype) ) )
		return Void;	/* unknown object type */

	/*
	 * Create an instance of the base object.
	 */
	new_obj = Create_New_Object_From_Base(window, base_obj, FALSE);

	ELKOBJECT3(obj)->object3 = new_obj;

	return obj;
}

/***********************************************************************
 *
 * Description: elk_object3d_position() is the C callback for the scheme
 * 		function "object3d-position". This function is used to
 *		move an object to a new world coordinate.
 *
 * Parameter:	xobj - X coordinate to move to
 *		yobj - Y coordinate to move to
 *		zobj - Z coordinate to move to
 *		obj - object to move. This parameter should be of
 *			the same type as that returned by the scheme
 *			function "object3d-create".
 *
 * Scheme example: (object3d-position 1.0 1.0 1.0 (object3d-create "sphere"))
 *
 * Return value: Returns the object passed in thus allowing for succesive
 *		 transformations to be performed. For example:
 *		 (object3d-position 1.0 1.0 1.0
 *		   (object3d-position 1.0 1.0 1.0 (object3d-create "sphere")))
 *
 ***********************************************************************/
Object
elk_object3d_position(Object xobj, Object yobj, Object zobj, Object obj)
{
	Transformation *t;
	InstanceList e;
	WindowInfo *window;

	window = (elk_is_csg_window) ? &csg_window : &main_window;
	Check_Type(xobj, T_Flonum);
	Check_Type(yobj, T_Flonum);
	Check_Type(zobj, T_Flonum);
	Check_Type(obj, OBJECT3_TYPE);
	t = &ELKOBJECT3(obj)->object3->o_transform;
	t->displacement.x = FLONUM(xobj)->val;
	t->displacement.y = FLONUM(yobj)->val;
	t->displacement.z = FLONUM(zobj)->val;
	Transform_Instance(ELKOBJECT3(obj)->object3, t, TRUE);
	if ((e = Find_Object_In_Instances(ELKOBJECT3(obj)->object3,
				     window->all_instances)))
	{
		View_Update(window, e, CalcView);
		Update_Projection_Extents(e);
		changed_scene = TRUE;
	}
	return obj;
}

/***********************************************************************
 *
 * Description: elk_object3d_scale() is the C callback for the scheme
 * 		function "object3d-scale". This function is used to
 *		rescale an object.
 *
 * Parameter:	xobj - amount to scale in X.
 *		yobj - amount to scale in Y.
 *		zobj - amount to scale in Z.
 *		obj - object to scale. This parameter should be of
 *			the same type as that returned by the scheme
 *			function "object3d-create".
 *
 * Scheme example: (object3d-scale 2.0 2.0 2.0 (object3d-create "sphere"))
 *
 * Return value: Returns the object passed in thus allowing for succesive
 *		 transformations to be performed. For example:
 *		 (object3d-scale 2.0 2.0 2.0
 *		   (object3d-scale 2.0 2.0 2.0 (object3d-create "sphere")))
 *
 ***********************************************************************/
Object
elk_object3d_scale(Object xobj, Object yobj, Object zobj, Object obj)
{
	Transformation a;
	InstanceList e;
	WindowInfo *window;

	window = (elk_is_csg_window) ? &csg_window : &main_window;
	Check_Type(xobj, T_Flonum);
	Check_Type(yobj, T_Flonum);
	Check_Type(zobj, T_Flonum);
	Check_Type(obj, OBJECT3_TYPE);

	a.matrix.x.x = FLONUM(xobj)->val;
	a.matrix.x.y = 0.0;
	a.matrix.x.z = 0.0;
	a.matrix.y.x = 0.0;
	a.matrix.y.y = FLONUM(yobj)->val;
	a.matrix.y.z = 0.0;
	a.matrix.z.x = 0.0;
	a.matrix.z.y = 0.0;
	a.matrix.z.z = FLONUM(zobj)->val;
	VNew(0, 0, 0, a.displacement);

	Transform_Instance(ELKOBJECT3(obj)->object3, &a, FALSE);

	if ((e = Find_Object_In_Instances(ELKOBJECT3(obj)->object3,
				     window->all_instances)))
	{
		View_Update(window, e, CalcView);
		Update_Projection_Extents(e);
		changed_scene = TRUE;
	}
	return obj;
}

/***********************************************************************
 *
 * Description: elk_object3d_scale() is the C callback for the scheme
 * 		function "object3d-scale". This function is used to
 *		rescale an object.
 *
 * Parameter:	xobj - amount to scale in X.
 *		yobj - amount to scale in Y.
 *		zobj - amount to scale in Z.
 *		obj - object to scale. This parameter should be of
 *			the same type as that returned by the scheme
 *			function "object3d-create".
 *
 * Scheme example: (object3d-scale 2.0 2.0 2.0 (object3d-create "sphere"))
 *
 * Return value: Returns the object passed in thus allowing for succesive
 *		 transformations to be performed. For example:
 *		 (object3d-scale 2.0 2.0 2.0
 *		   (object3d-scale 2.0 2.0 2.0 (object3d-create "sphere")))
 **********************************************************************/
Object
elk_object3d_rotate(Object xobj, Object yobj, Object zobj, Object obj)
{
	Transformation	t;
	InstanceList	e;
	WindowInfo		*window;
	Vector			vect;

	window = (elk_is_csg_window) ? &csg_window : &main_window;
	Check_Type(xobj, T_Flonum);
	Check_Type(yobj, T_Flonum);
	Check_Type(zobj, T_Flonum);
	Check_Type(obj, OBJECT3_TYPE);

	vect.x = FLONUM(xobj)->val;
	vect.y = FLONUM(yobj)->val;
	vect.z = FLONUM(zobj)->val;
	Vector_To_Rotation_Matrix(&vect, &(t.matrix));

	VNew(0, 0, 0, t.displacement);

	Transform_Instance(ELKOBJECT3(obj)->object3, &t, FALSE);

	if ((e = Find_Object_In_Instances(ELKOBJECT3(obj)->object3,
				     window->all_instances)))
	{
		View_Update(window, e, CalcView);
		Update_Projection_Extents(e);
		changed_scene = TRUE;
	}
	return obj;
}

/***********************************************************************
 *
 * Description: elk_object3d_destroy() is the C callback for the scheme
 * 		function "object3d-destroy". This function is used to
 *		destroy an object from scheme.
 *
 * Parameter:	obj - object to be destroyed. This parameter should be of
 *			the same type as that returned by the scheme
 *			function "object3d-create".
 *
 * Scheme example: (object3d-destroy (object3d-create "sphere"))
 *
 * Return value: Returns the Void object.
 *
 ***********************************************************************/
Object
elk_object3d_destroy(Object obj)
{
	ObjectInstancePtr object3;
	WindowInfo *window;

	window = (elk_is_csg_window) ? &csg_window : &main_window;
	Check_Type(obj, OBJECT3_TYPE);
	object3 = ELKOBJECT3(obj)->object3;
	if (object3) {
		InstanceList victim;
		
		victim = Find_Object_In_Instances(object3,
						  window->all_instances);
		if (window->all_instances == victim)
			window->all_instances = victim->next;
		Delete_Element(victim);
		free(victim);
		/*
		 * Check for the victim in the edit lists.
		 */
		if ((victim = Find_Object_In_Instances(object3,
			       window->edit_instances)))
			Delete_Edit_Instance(window, victim);
		Destroy_Instance(object3);
		View_Update(window, window->all_instances, ViewNone);
	}
	return Void;
}

/**********************************************************************
 *
 * Description: elk_object3d_wireframe_query() is the C callback for the scheme
 *              function "object3d-wireframe-query". This function is used to
 *              obtain the wireframe level for an object.
 *
 * Parameter: obj - The object to query.
 *
 * Scheme example: (object3d-wireframe-query (object3d-create "sphere"))
 *                 0
 *
 * Return value: Returns an INT type object containing the wireframe level.
 **********************************************************************/
Object
elk_object3d_wireframe_query(Object obj)
{
	ObjectInstancePtr object3;

	Check_Type(obj, OBJECT3_TYPE);
	object3 = ELKOBJECT3(obj)->object3;
	if ( ! object3)
		return Void;

	return Make_Integer(Wireframe_Density_Level(object3));
}


/**********************************************************************
 *
 * Description: elk_object3d_wireframe_level() is the C callback for
 *              the scheme function "object3d-wireframe-level". This
 *              function is used to set the wireframe level for an object.
 *
 * Parameters: obj - the object to set.
 *             level_obj - the new level.
 *
 * Scheme example: (object3d-wireframe-level (object3d-create "sphere") 2)
 *
 * Return value: Returns the object obj.
 **********************************************************************/
Object
elk_object3d_wireframe_level(Object level_obj, Object obj)
{
	ObjectInstancePtr	object3;
	int					level;
	InstanceList		e;
	WindowInfo			*window;

	window = (elk_is_csg_window) ? &csg_window : &main_window;

	Check_Type(obj, OBJECT3_TYPE);
	Check_Integer(level_obj);
	object3 = ELKOBJECT3(obj)->object3;
	level = Get_Integer(level_obj);
	if ( ! object3)
		return Void;

	Object_Change_Wire_Level(object3, level);

	if ((e = Find_Object_In_Instances(object3, window->all_instances)))
	{
		View_Update(window, e, CalcView);
		Update_Projection_Extents(e);
		changed_scene = TRUE;
	}

	return obj;
}

Object
elk_object3d_attribs_define(Object val, Object obj)
{
	Check_Type(obj, OBJECT3_TYPE);

	((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->defined =
		Truep(val);

	changed_scene = TRUE;

	return obj;
}

Object
elk_object3d_color(Object robj, Object gobj, Object bobj, Object obj)
{
	Check_Type(robj, T_Flonum);
	Check_Type(gobj, T_Flonum);
	Check_Type(bobj, T_Flonum);
	Check_Type(obj, OBJECT3_TYPE);

	((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->colour.red =
		(unsigned short)(FLONUM(robj)->val * MAX_UNSIGNED_SHORT);
	((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->colour.green =
		(unsigned short)(FLONUM(gobj)->val * MAX_UNSIGNED_SHORT);
	((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->colour.blue =
		(unsigned short)(FLONUM(bobj)->val * MAX_UNSIGNED_SHORT);

	changed_scene = TRUE;

	return obj;
}

Object
elk_object3d_diffuse(Object difobj, Object obj)
{
	Check_Type(difobj, T_Flonum);
	Check_Type(obj, OBJECT3_TYPE);

	((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->diff_coef =
		FLONUM(difobj)->val;

	changed_scene = TRUE;

	return obj;
}

Object
elk_object3d_specular(Object specobj, Object powobj, Object obj)
{
	Check_Type(specobj, T_Flonum);
	Check_Type(powobj, T_Flonum);
	Check_Type(obj, OBJECT3_TYPE);

	((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->spec_coef =
		FLONUM(specobj)->val;
	((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->spec_power =
		FLONUM(powobj)->val;

	changed_scene = TRUE;

	return obj;
}

Object
elk_object3d_reflect(Object reflobj, Object obj)
{
	Check_Type(reflobj, T_Flonum);
	Check_Type(obj, OBJECT3_TYPE);

	((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->reflect_coef =
		FLONUM(reflobj)->val;

	changed_scene = TRUE;

	return obj;
}

Object
elk_object3d_transparency(Object transobj, Object refrobj, Object obj)
{
	Check_Type(transobj, T_Flonum);
	Check_Type(refrobj, T_Flonum);
	Check_Type(obj, OBJECT3_TYPE);

	((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->transparency =
		FLONUM(transobj)->val;
	((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->refract_index =
		FLONUM(refrobj)->val;

	changed_scene = TRUE;

	return obj;
}

