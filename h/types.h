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
**	types.h: Header file containing type declarations and definitions.
**
**	Created: 04/03/94
*/

#ifndef __SCED_TYPES__
#define __SCED_TYPES__


/* An linear transformation type, consisting of an affine	*/
/*  transformation matrix (for scaling, rotating etc) and a	*/
/*  displacement vector for positioning in space.		*/
typedef struct _Transformation {
	Matrix	matrix;
	Vector	displacement;
	} Transformation;



/*	A Viewport specification type.	*/
typedef struct _Viewport {
	Vector			view_from;
	Vector			view_at;
	Vector			view_up;
	double			view_distance;
	double			eye_distance;
	Transformation	world_to_view;
	Transformation	view_to_world;
	Vector			eye_position;
	int				draw_mode;
	Dimension   	scr_width;
	Dimension		scr_height;
	int				magnify;
	Boolean			is_default;
	Transformation  *world_to_view_spline; /* array of splined world_to_views */
	Vector			*view_from_spline;
	Vector			*view_at_spline;
	Vector			*view_up_spline;
	double			*view_distance_spline;
	double			*eye_distance_spline;
	Vector			*eye_position_spline;
	Dimension   	*scr_width_spline;
	Dimension		*scr_height_spline;
	int				*magnify_spline;
	} Viewport, *ViewportPtr;


/* A Camera structure.  This needs to hold information about any one
** of three types of cameras.  Rayshade, POV, and Genray.
*/
typedef struct _Camera {
	Raytracer	type;			/* The target raytracer. */
	Boolean		default_cam;	/* Use the default camera for the raytracer. */
	Vector		location;		/* eyep, location or eye_position. */
	Vector		look_at;		/* lookp, look_at or lookat. */
	Vector		look_up;		/* up, sky or lookup. */
	double		horiz_fov;		/* fov in Rayshade. */
	double		vert_fov;		/* fov in Rayshade. */
	double		eye_dist;		/* focal_dist in Ray, ->look_from in Genray. */
	double		window_up;		/* window in Genray, ->up in POV, */
	double		window_right;	/* window in Genray, ->right in POV, */
	Dimension	scr_width;		/* mainly to allow for save/load. */
	Dimension	scr_height;
	} Camera;


/* A Cuboid is the 3D equivalent of a Rectangle.	*/
typedef struct _Cuboid {
	Vector min;
	Vector max;
	} Cuboid;

/* Need a type for projection extents. */
typedef struct _Extent2D {
	XPoint min;
	XPoint max;
	} Extent2D;


/* Specifications for a point used in a constraint definition. */
typedef struct _FeatureSpecifier {
	FeatureSpecType			spec_type;		/* The type of spec point this is.*/
	struct _ObjectInstance	*spec_object;	/* The object referenced, if any. */
	Vector					spec_vector;	/* The vector describing the pt.  */
	} FeatureSpecifier, *FeatureSpecPtr;

/* Feature data for specifying constraints. */
typedef struct _FeatureData {
	FeatureType			f_type;		/* The nature of the feature eg plane.	*/
	char				*f_label;	/* The label attached.					*/
	Boolean				f_status;	/* Whether or not it's active.			*/
	Vector				f_vector;	/* The defining vector. Normal for a
									** plane, direction for a line. Undefined
									** for a point.
									*/
	Vector				f_point;	/* A specifying point. Any point on a
									** plane or line, the point for a point.
									*/
	double				f_value;	/* Special value. Norm.Point for a plane.*/
	FeatureType			f_spec_type;/* The type it was defined as. */
	FeatureSpecifier	f_specs[3];	/* Specifications of the feature's
									** defining points.
									*/
	} FeatureData, *FeaturePtr;


/*	A Vertex has coordinates in view space and screen space.	*/
typedef struct _Vertex {
	Vector	view;
	XPoint	screen;
	} Vertex;

/* An attribute type.	*/
typedef struct _Attributes {
	Boolean		defined;		/* Whether attributes have been explicitly set*/
	XColor		colour;			/* Obvious. */
	double		diff_coef;		/* Diffuse lighting coeficient. */
	double		spec_coef;		/* Specular lighting coeficient. */
	double		spec_power;		/* Specular power. */
	double		reflect_coef;	/* Coeficient of reflection. */
	double		transparency;	/* Degree of transparency. */
	double		refract_index;	/* Index of refraction squared. */
	Boolean		use_extension;	/* Whether to use the extension field. */
	char		*extension;		/* A string of additional info. */
	Boolean		open;			/* For cylinders and cones. */
	Boolean		use_obj_trans;	/* Whether to transform textures with the obj.*/
	} Attributes, *AttributePtr;

/* The wireframe format. */
/* It uses the classic structure with lists of vertices and lists of faces,
** each face having an ordered list of vertex indices.
*/
typedef struct _Face {
	int		*vertices;		/* Clockwise ordered vertex indices. */
	int		num_vertices;
	Vector	normal;			/* A face normal. */
	AttributePtr	face_attribs;	/* A pointer to this face's attributes.	*/
	Boolean	draw;			/* Used with drawing routines. */
	} Face, *FacePtr;

typedef struct _Wireframe {
	Vector		*vertices;
	int			num_vertices;
	FacePtr		faces;
	int			num_faces;
	Vector		*vertex_normals;	/* Vertex normals for phong shading. */
									/* Generally NULL. */
	AttributePtr	*attribs;		/* Face attributes, generally NULL. */
	int			num_attribs;
	} Wireframe, *WireframePtr;


/* A type for edge lists. */
typedef struct _EdgeElmt {
	int	v1;
	int	v2;
	int	new_v;
	struct _EdgeElmt *next;
	} EdgeElmt, *EdgePtr;


typedef struct _LightInfo {
	double	red;		/* The color of the light. */
	double	green;
	double	blue;
	double	val1;		/* Either outer rad or xnum. */
	double	val2;		/* Either tightness or ynum. */
	Boolean	flag;		/* Either invert or jitter. */
	} LightInfo, *LightInfoPtr;


/* A type to hold dependency info. */
typedef struct _Dependent {
	struct _ObjectInstance	*obj;
	char					count;
	} Dependent, *DependentList;


/*	A BaseObject type.  Instances inherit from a base object.	*/
typedef struct _BaseObject {
	String			b_label;		/* A name for the base class.			*/
	GenericObject	b_class;		/* The type of object this is.			*/
	short			b_ref_num;		/* The default reference for instances.	*/
	struct _CSGNode	*b_csgptr;		/* A pointer to a csg tree.				*/
	WireframePtr	b_major_wire;	/* A CSG or OFF wireframe for the object*/
	WireframePtr	b_wireframe;	/* The simplified version.				*/
	WireframePtr	*b_dense_wire;	/* An array of succ. denser wireframes.	*/
	short			b_max_density;	/* The number of dense_wireframes.		*/
	Boolean			b_use_full;		/* Use the full wireframe, not csg.		*/
	int				b_num_instances;/* The number of instances of this type.*/
	int				b_num_slots;	/* The number of spaces available.		*/
	struct _ObjectInstance	**b_instances;	/* An array of pointers to		*/
											/* instances.					*/
	} BaseObject, *BaseObjectPtr;


/*	A structure for a particular  object instance.	*/
typedef struct _ObjectInstance {
	String			o_label;		/* A name for the object.				*/
	BaseObject		*o_parent;		/* The base type for this object.		*/
	Wireframe		*o_wireframe;	/* Its wireframe.						*/
	Transformation	o_transform;	/* The transform from generic to world.	*/
	Transformation	o_inverse;		/* The transform from world to generic.	*/
	void			*o_attribs;		/* Visual properties.					*/
	Extent2D		o_proj_extent;	/* The projection extent.				*/
	int				o_layer;		/* Display layer number.				*/
	short			o_num_vertices;	/* The number of verts in its wireframe.*/
	Vector			*o_world_verts;	/* The vertices in world.				*/
	Vertex			*o_main_verts;	/* The array of wireframe main vertices.*/
	short			o_num_faces;	/* The number of faces on the object.	*/
	Vector			*o_normals;		/* The normals for the faces.			*/

	Matrix			o_axes;			/* The defining axes for the object's
									** coordinate space.					*/
	Vector			o_origin;		/* The point, offset from the center,
									** that is used to define the origin of
									** the object's reference coordinates.	*/
	Vector			o_reference;	/* The offset from the origin to the
									** objects reference point. For the moment
									** must be the location of a real vertex.*/
	FeaturePtr		o_origin_cons;	/* Constraints defining the location
									** of the origin (in addition to the
									** defaults).							*/
	short			o_origin_num;	/* The number of said constraints.		*/
	Boolean		   *o_origin_active;/* Those of the origin constraints that
									** are active.							*/
	FeaturePtr		o_scale_cons;	/* Constraints defining the scaling
									** of the body relative to its origin.
									** (in addition to the defaults).		*/
	short			o_scale_num;	/* The number of said constraints.		*/
	Boolean		   *o_scale_active;	/* Those of the scale constraints that
									** are active.							*/
	FeaturePtr		o_rotate_cons;	/* Constraints defining the rotation axes
									** of the body relative to its origin.
									** (in addition to the defaults).		*/
	short			o_rotate_num;	/* The number of rotation axes.			*/
	Boolean		   *o_rotate_active;/* Those of the rotate axes that
									** are active.							*/
	FeatureData		o_major_align;	/* The major alignment feature.		*/
	FeatureData		o_minor_align;	/* The minor alignment feature.		*/
	DependentList	o_dependents;	/* An array of objects with constraints	*/
									/* depending on this object. To allow	*/
	short			o_num_depend;	/* constraint maintainence.				*/
	int				o_flags;		/* Bit flags for visibility, selection etc*/
	unsigned long	o_dfs_mark;		/* A value needed for depth-first-search*/
	int				o_fstart;
	int				o_fstop;
	Boolean			*o_inframe;
	int				o_numseqs;
	Quaternion		o_rot;
	Vector			o_scale;
	Vector			*o_posspline;
	Matrix			*o_rotspline;
	} ObjectInstance, *ObjectInstancePtr;


/*	A structure for maintaining lists of object instances. */
typedef struct _InstanceListElmt {
	ObjectInstancePtr			the_instance;
	struct _InstanceListElmt	*next;
	struct _InstanceListElmt	*prev;
	} InstanceListElmt, *InstanceList;


/*	A structure for floating menus ie menus which get larger and smaller. */
typedef struct _MenuInfo {
	Widget	menu;
	Widget	*children;
	short	num_children;
	short	max_children;
	Widget	button;
	} MenuInfo, *MenuInfoPtr;


/* The various states a window can be in.
** Which events go where, and the reaction to certain events depends on this.
*/
typedef int StateType;


/* A structure for containing window information.  By window information
** I mean viewports, widgets, etc.
*/
typedef struct _WindowInfo {
	Widget		shell;			/* The window shell. 						*/
	Widget		view_widget;	/* The view widget inside it. 				*/
	Viewport	viewport;		/* The viewport for the view widget.		*/
	ObjectInstance	axes;		/* The axes for the window.					*/
	char		*text_string;	/* The string for inputing text.			*/
	Widget		text_widget;	/* The text widget for the above string.	*/
	Widget		text_label;		/* The label at the bottom.					*/
	Widget		apply_button;	/* The apply button for the text.			*/
	Widget		kf_label;
	Widget		kfnum_label;
	Widget		f_label;
	Widget		fnum_label;
	MenuInfoPtr	edit_menu;		/* The menu for edit.						*/
	InstanceList	all_instances;	/* All the instances for the window.	*/
	InstanceList	selected_instances;	/* Those selected.					*/
	InstanceList	edit_instances;		/* All those pending editing.		*/
	StateType	current_state;	/* The current state for the window.		*/
	Pixmap		off_screen;		/* Off screen bitmap for drawing into.		*/
	Dimension	width;
	Dimension	height;			/* Current width and height.				*/
	int			kfnumber;
	int			fnumber;
	} WindowInfo, *WindowInfoPtr;


/*	A structure for passing scene information to the export functions. */
typedef struct _SceneStruct {
	Camera				camera;
	ObjectInstancePtr	light;
	XColor				ambient;
	InstanceList		instances;
	} SceneStruct, *ScenePtr;

/* A function type for functions that manipulate constraint specifiers. */
typedef void (*SpecFunction)(FeatureSpecPtr, ObjectInstancePtr, void*, int);

/* Application resources structure. */
typedef struct _ScedResources {
	Pixel	x_axis_color;
	Pixel	y_axis_color;
	Pixel	z_axis_color;
	int		axis_width;
	int		x_axis_length;
	int		y_axis_length;
	int		z_axis_length;
	int		axis_denom;
	Pixel	obj_x_axis_color;
	Pixel	obj_y_axis_color;
	Pixel	obj_z_axis_color;
	int		obj_axis_width;
	int		obj_x_axis_length;
	int		obj_y_axis_length;
	int		obj_z_axis_length;
	int		obj_axis_denom;
	int		edit_pt_rad;
	Pixel	scaling_color;
	Pixel	origin_color;
	Pixel	object_color;
	Pixel	selected_color;
	int		selected_width;
	Pixel	light_color;
	int		light_pt_rad;
	Pixel	constraint_color;
	int		plane_con_length;
	int		line_con_length;
	int		incon_con_length;
	int		point_con_rad;
	int		origin_con_width;
	int		scale_con_width;
	int		rotate_con_width;
	Pixel	referenced_color;
	Pixel	active_color;
	Pixel	selected_pt_color;
	int		select_pt_width;
	int		select_pt_line_width;
	Pixel	absolute_color;
	Pixel	offset_color;
	Pixel	reference_color;
	Pixel	arcball_color;
	} ScedResources;


/*	A structure for maintaining the list of defined keyframes. */
typedef struct _KeyFrame {
	InstanceList		all_instances;	/* All the instances for the window.*/
	InstanceList		selected_instances;	/* Those selected.				*/
	InstanceList		edit_instances;		/* All those pending editing.	*/
	int					kfnumber;			/* keyframe number 				*/
	int					fnumber;			/* frame number 				*/
	MenuInfoPtr			edit_menu;
	Viewport			viewport;
	struct _KeyFrame	*next;
	struct _KeyFrame	*prev;
	int					magnification;
	} KeyFrame, *KeyFrameList;

/* This is used to map names around when merging sequences together:
   if there's a name conflict, we have to pick a new name for the object,
   but if we pick a new name once, we have to pick the same name for
   all subsequent keyframes.  This structure is used to maintain the
   list of mappings.
*/
typedef struct _LabelMap {
	String				old;
	String				new;
	struct _LabelMap	*next;
	} LabelMap, *LabelMapPtr;


#endif
