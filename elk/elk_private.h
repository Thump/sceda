/* Private elk header file. */

#ifndef __ELK_P__
#define __ELK_P__

#include <sced.h>
#include <base_objects.h>
#include <csg.h>
#include <instance_list.h>

/*
 * Here we need to redefine Object, True, and False
 * since these are defined both by ELK and by X11.
 */
#define Object EObject
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif


extern int elk_use_csg;
extern int elk_use_main;
extern int elk_is_csg_window;
extern WindowInfoPtr	elk_window;


#define OBJECT3_TYPE	(T_Last + 1)
#define VIEWPORT_TYPE	(T_Last + 2)
#define CSGNODE_TYPE	(T_Last + 3)

typedef struct {
	ObjectInstancePtr object3;
} Elkobject3;

typedef struct {
	Viewport *viewport;
} Elkviewport;

typedef struct {
	CSGNodePtr	csg_node;
} Elkcsgnode;

#define ELKOBJECT3(OBJ) ((Elkobject3 *) POINTER(OBJ))
#define ELKVIEWPORT(OBJ) ((Elkviewport *) POINTER(OBJ))
#define ELKCSGNODE(OBJ) ((Elkcsgnode *) POINTER(OBJ))

#include <scheme.h>

extern Object	Sym_Union;
extern Object	Sym_Intersection;
extern Object	Sym_Difference;

/* Viewport callbacks. */
extern int		elk_viewport_print(Object, Object, int, int, int);
extern int		elk_viewport_equal(Object, Object);
extern int		elk_viewport_equiv(Object, Object);
extern Object	elk_viewport_create();
extern Object	elk_viewport_destroy(Object);
extern Object	elk_viewport_lookat(Object, Object, Object, Object);
extern Object	elk_viewport_position(Object, Object, Object, Object);
extern Object	elk_viewport_upvector(Object, Object, Object, Object);
extern Object	elk_viewport_distance(Object, Object);
extern Object	elk_viewport_eye(Object, Object);
extern Object	elk_viewport_setup(Object);
extern Object	elk_viewport_zoom(Object, Object);

/* Object3d callbacks. */
extern int		elk_object3d_print(Object, Object, int, int, int);
extern int		elk_object3d_equal(Object, Object);
extern int		elk_object3d_equiv(Object, Object);
extern Object	elk_object3d_create(Object);
extern Object	elk_object3d_position(Object, Object, Object, Object);
extern Object	elk_object3d_scale(Object, Object, Object, Object);
extern Object	elk_object3d_rotate(Object, Object, Object, Object);
extern Object	elk_object3d_destroy(Object);
extern Object	elk_object3d_wireframe_query(Object);
extern Object	elk_object3d_wireframe_level(Object, Object);
extern Object	elk_object3d_attribs_define(Object, Object);
extern Object	elk_object3d_color(Object, Object, Object, Object);
extern Object	elk_object3d_diffuse(Object, Object);
extern Object	elk_object3d_specular(Object, Object, Object);
extern Object	elk_object3d_reflect(Object, Object);
extern Object	elk_object3d_transparency(Object, Object, Object);

/* CSG callbacks. */
extern int		elk_csg_print(Object, Object, int, int, int);
extern int		elk_csg_equal(Object, Object);
extern int		elk_csg_equiv(Object, Object);
extern Object	elk_csg_node(Object);
extern Object	elk_csg_display(Object);
extern Object	elk_csg_hide(Object);
extern Object	elk_csg_attach(Object, Object, Object);
extern Object	elk_csg_complete(Object, Object);


#endif /* __ELK_P__ */
