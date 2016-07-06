#include "elk_private.h"
#include <View.h>


/***********************************************************************
 *
 * Description: viewport_destroy() is used to destroy a Viewport that
 *		was previously created with viewport_create().
 *
 * Parameter:	viewport - Viewport object to destroy.
 *
 ***********************************************************************/
static void
viewport_destroy(Viewport *viewport)
{
	if (viewport) {
		free(viewport);
	}
	return;
}


/***********************************************************************
 *
 * Description: elk_viewport_print() is used internally by the scheme
 *		interpreter. This function is called whenever a user
 *		tries to print out the viewport from scheme.
 *
 * Scheme example: (display (viewport-create)) => #[viewport 0x1a4c00]
 *
 ***********************************************************************/
int
elk_viewport_print(Object w, Object port, int raw, int depth, int len)
{
	Viewport *viewport = (Viewport *) ELKVIEWPORT(w)->viewport;
	
	Printf(port, "#[viewport 0x%x]\n", viewport);
	return 0;
}

/***********************************************************************
 *
 * Description: elk_viewport_equal() is used internally by the scheme
 *		interpreter. This function is called whenever two viewports
 *		are compared in scheme using the "equal?" operator.
 *
 * Scheme example: (equal? (viewport-create) (viewport-create))
 *
 ***********************************************************************/
int
elk_viewport_equal(Object a, Object b)
{
	return (ELKVIEWPORT(a)->viewport == ELKVIEWPORT(b)->viewport);
}

/***********************************************************************
 *
 * Description: elk_viewport_equiv() is used internally by the scheme
 *		interpreter. This function is called whenever two viewports
 *		are compared in scheme using the "eqv?" operator.
 *
 * Scheme example: (eqv? (viewport-create) (viewport-create))
 *
 ***********************************************************************/
int
elk_viewport_equiv(Object a, Object b)
{
	return (ELKVIEWPORT(a)->viewport == ELKVIEWPORT(b)->viewport);
}

/***********************************************************************
 *
 * Description: elk_viewport_create() is the C callback for the scheme
 * 		function "viewport-create". This function is used to
 *		create a new viewport object from scheme.
 *
 * Scheme example: (viewport-create)
 *
 * Return value: Returns a new viewport object
 *
 ***********************************************************************/
Object
elk_viewport_create()
{
	Object obj;
	
	/*
	 * First we allocate an Elk object to save the viewport in.
	 */
	obj = Alloc_Object(sizeof(Elkviewport), VIEWPORT_TYPE, 0);
	ELKVIEWPORT(obj)->viewport = New(Viewport, 1);

	Viewport_Init(ELKVIEWPORT(obj)->viewport);
	ELKVIEWPORT(obj)->viewport->scr_width = camera.scr_width;
	ELKVIEWPORT(obj)->viewport->scr_height = camera.scr_height;

	return obj;
}

/***********************************************************************
 *
 * Description: elk_camera_destroy() is the C callback for the scheme
 * 		function "viewport-destroy". This function is used to
 *		destroy a viewport object that was previously created
 *		with "viewport-create".
 *
 * Parameter:	viewportobj - viewport to be destroyed.
 *
 * Scheme example: (viewport-destroy (viewport-create))
 *
 * Return value: Returns the Void object.
 *
 ***********************************************************************/
Object
elk_viewport_destroy(Object viewportobj)
{
	Check_Type(viewportobj, VIEWPORT_TYPE);
	if (ELKVIEWPORT(viewportobj)->viewport)
		viewport_destroy(ELKVIEWPORT(viewportobj)->viewport);
	return Void;
}

/***********************************************************************
 *
 * Description: elk_viewport_lookat() is the C callback for the scheme
 * 		function "viewport-lookat". This function is used to
 *		set a new look point for the viewport.
 *
 * Parameter:	xobj - new look point's X coordinate
 *		yobj - new look point's Y coordinate
 *		zobj - new look point's Z coordinate
 *		viewportobj - viewport to change
 *
 * Scheme example: (viewport-lookat 1.0 1.0 1.0 (viewport-create))
 *
 * Return value: Returns the viewport object passed in.
 *
 ***********************************************************************/
Object
elk_viewport_lookat(Object xobj, Object yobj, Object zobj, Object viewportobj)
{
	Viewport *viewport;

	Check_Type(viewportobj, VIEWPORT_TYPE);
	Check_Type(xobj, T_Flonum);
	Check_Type(yobj, T_Flonum);
	Check_Type(zobj, T_Flonum);
	viewport = ELKVIEWPORT(viewportobj)->viewport;
	viewport->view_at.x = FLONUM(xobj)->val;
	viewport->view_at.y = FLONUM(yobj)->val;
	viewport->view_at.z = FLONUM(zobj)->val;

	return viewportobj;
}

/***********************************************************************
 *
 * Description: elk_viewport_position() is the C callback for the scheme
 * 		function "viewport-position". This function is used to
 *		change the current "view from" position of a viewport.
 *
 * Parameter:	xobj - new X coordinate
 *		yobj - new Y coordinate
 *		zobj - new Z coordinate
 *		viewportobj - viewport to change
 *
 * Scheme example: (viewport-position 1.0 1.0 1.0 (viewport-create))
 *
 * Return value: Returns the viewport object passed in.
 *
 ***********************************************************************/
Object
elk_viewport_position(Object xobj, Object yobj, Object zobj, Object viewportobj)
{
	Viewport *viewport;

	Check_Type(viewportobj, VIEWPORT_TYPE);
	Check_Type(xobj, T_Flonum);
	Check_Type(yobj, T_Flonum);
	Check_Type(zobj, T_Flonum);
	viewport = ELKVIEWPORT(viewportobj)->viewport;
	viewport->view_from.x = FLONUM(xobj)->val;
	viewport->view_from.y = FLONUM(yobj)->val;
	viewport->view_from.z = FLONUM(zobj)->val;

	return viewportobj;
}

/***********************************************************************
 *
 * Description: elk_viewport_upvector() is the C callback for the scheme
 * 		function "viewport-upvector". This function is used to
 *		change a viewport's notion of what up is.
 *
 * Parameter:	xobj - new X up coordinate
 *		yobj - new Y up coordinate
 *		zobj - new Z up coordinate
 *		viewportobj - viewport to change
 *
 * Scheme example: (viewport-upvector 0.0 1.0 0.0 (viewport-create))
 *
 * Return value: Returns the viewport object passed in.
 *
 ***********************************************************************/
Object
elk_viewport_upvector(Object xobj, Object yobj, Object zobj, Object viewportobj)
{
	Viewport *viewport;

	Check_Type(viewportobj, VIEWPORT_TYPE);
	Check_Type(xobj, T_Flonum);
	Check_Type(yobj, T_Flonum);
	Check_Type(zobj, T_Flonum);
	viewport = ELKVIEWPORT(viewportobj)->viewport;
	viewport->view_up.x = FLONUM(xobj)->val;
	viewport->view_up.y = FLONUM(yobj)->val;
	viewport->view_up.z = FLONUM(zobj)->val;

	return viewportobj;
}


/*************************************************************************
 *
 * Description: elk_viewport_distance() is the C callback for the scheme
 *      function "viewport-distance". This function is used to change the
 *      viewing distance.
 *
 * Parameters: distobj - new distance
 *             viewportobj - viewport to change
 *
 * Scheme example: (viewport-distance 10.0 (viewport-create))
 *
 * Return value: Returns the viewport object passed in.
 *
 *************************************************************************/
Object
elk_viewport_distance(Object distobj, Object viewportobj)
{
	Viewport *viewport;

	Check_Type(viewportobj, VIEWPORT_TYPE);
	Check_Type(distobj, T_Flonum);
	viewport = ELKVIEWPORT(viewportobj)->viewport;
	viewport->view_distance = FLONUM(distobj)->val;
	return viewportobj;
}

/*************************************************************************
 *
 * Description: elk_viewport_eye() is the C callback for the scheme
 *      function "viewport-eye". This function is used to change the
 *      eye distance.
 *
 * Parameters: eyeobj - new eye distance
 *             viewportobj - viewport to change
 *
 * Scheme example: (viewport-eye 10.0 (viewport-create))
 *
 * Return value: Returns the viewport object passed in.
 *
 *************************************************************************/
Object
elk_viewport_eye(Object eyeobj, Object viewportobj)
{
	Viewport *viewport;

	Check_Type(viewportobj, VIEWPORT_TYPE);
	Check_Type(eyeobj, T_Flonum);
	viewport = ELKVIEWPORT(viewportobj)->viewport;
	viewport->eye_distance = FLONUM(eyeobj)->val;
	return viewportobj;
}

Object
elk_viewport_zoom(Object zoomobj, Object viewportobj)
{
	Check_Type(viewportobj, VIEWPORT_TYPE);
	Check_Integer(zoomobj);
	ELKVIEWPORT(viewportobj)->viewport->magnify = Get_Integer(zoomobj);

	return viewportobj;
}

/***********************************************************************
 *
 * Description: elk_viewport_setup() is the C callback for the scheme
 * 		function "viewport-setup". This function is used to
 *		set the current viewport to be a new viewport object
 *		or to make changes to the current viewport take effect.
 *
 * Parameter:	viewportobj - new viewport to make into current viewport
 *
 * Scheme example: (let ((view (viewport-create)))
 *			(viewport-setup (viewport->lookat 1.0 1.0 1.0 view)))
 *
 * Return value: Returns the Void object.
 *
 ***********************************************************************/
Object
elk_viewport_setup(Object viewportobj)
{
	Viewport *viewport;

	viewport = ELKVIEWPORT(viewportobj)->viewport;

	elk_window->viewport = *viewport;
 	Build_Viewport_Transformation(&(elk_window->viewport));
	if (viewport->scr_width) {
		XtVaSetValues(elk_window->view_widget,
			      XtNwidth, viewport->scr_width,
			      XtNheight, viewport->scr_height,
			      XtNmagnification, viewport->magnify,
			      NULL);
	} else {
		XtVaSetValues(elk_window->view_widget,
			      XtNmagnification, viewport->magnify,
			      NULL);
	}

	View_Update(elk_window, elk_window->all_instances, CalcView);
	Update_Projection_Extents(elk_window->all_instances);
	return Void;
}

