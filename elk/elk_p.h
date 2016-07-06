/* Private elk header file. */

#ifndef __ELK_P__
#define __ELK_P__


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


extern int elk_is_csg_window;

#define OBJECT3_TYPE	(T_Last + 1)
#define VIEWPORT_TYPE	(T_Last + 2)

typedef struct {
	ObjectInstancePtr object3;
} Elkobject3;

typedef struct {
	Viewport *viewport;
} Elkviewport;

#define ELKOBJECT3(OBJ) ((Elkobject3 *) POINTER(OBJ))
#define ELKVIEWPORT(OBJ) ((Elkviewport *) POINTER(OBJ))

#include <scheme.h>

#endif /* __ELK_P__ */
