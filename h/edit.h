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
**	edit.h : header for common editing functions.
*/

#ifndef __EDIT__
#define __EDIT__


#define ARC_DIVISIONS 40

#define NO_DRAG 0
#define ORIGIN_DRAG 1
#define SCALE_DRAG 2
#define ROTATE_DRAG 3

#define NONE 0
#define ORIGIN 1
#define SCALE 2
#define ROTATE 3

#define MAJOR_AXIS 1
#define MINOR_AXIS 2
#define OTHER_AXIS 3

#define NO_ALLIGN "Not Aligned"
#define ALLIGNED "Aligned"

#define MAX_ADD_ITEMS 7 /* The max number of items in any add constraint menu.*/

/* I'm commenting this out here: I've moved this over to Vector.h because
** I needed to use a Quaternion in the ObjectInstance type.
*/

/* typedef struct _Quaternion { */
	/* Vector	vect_part; */
	/* double	real_part; */
	/* } Quaternion; */

typedef struct _AddInfo {
	FeatureType				type;
	struct _ConstraintBox	*box;
	} AddInfo, *AddInfoPtr;

typedef struct _ConPoint {
	FeatureSpecType	type;
	XPoint			pt;
	} ConPoint;

/*	A type to hold information for the constraint boxes. */
typedef struct _ConstraintBox {
	Widget		box_widget;
	Widget		label_widget;
	Widget		add_widget;
	AddInfo		add_info[MAX_ADD_ITEMS];
	int			num_options;
	int			max_num_options;
	Widget		*option_widgets;
	FeaturePtr	*options;
	} ConstraintBox, *ConstraintBoxPtr;

/* A type to hold all the general info about editing. */
typedef struct _EditInfo {
	WindowInfoPtr		window;
	InstanceList		inst;
	ObjectInstancePtr	obj;

	InstanceList	all_available;
	InstanceList	other_available;
	InstanceList	reference_available;

	Transformation	drag_transform;

	Matrix				axes;
	Matrix				axes_inverse;
	ObjectInstancePtr	axes_obj;

	Vector		origin;
	Vertex		origin_vert;
	XArc		origin_circle;
	FeatureData	origin_resulting;
	XPoint		origin_pts[4];
	ConPoint	*origin_def_pts;
	int			num_origin_pts;

	Vector		reference;
	Vertex		reference_vert;
	XArc		reference_circle;
	FeatureData	reference_resulting;
	XPoint		reference_pts[4];
	ConPoint	*reference_def_pts;
	int			num_reference_pts;

	ConPoint	*align_def_pts;
	int			num_align_pts;

	int			radius;
	XArc		circle_arc;
	XPoint		arc_points[ARC_DIVISIONS + 1];
	int			num_arc_pts;
	FeatureData	rotate_resulting;

	int			drag_type;
	Vector		drag_start;

	Boolean		selecting;
	Boolean		deleting;

	Quaternion	workrot;
	Quaternion	rotation;
	Vector		workscale;
	Vector		scale;

	} EditInfo, *EditInfoPtr;


typedef enum _EditOpType {
	edit_drag_op,		/* A drag of any type. */
	edit_select_op,		/* A constraint selection op. */
	edit_deselect_op,	/* A constraint deselection op. */
	edit_add_op,		/* An add constraint op. */
	edit_remove_op,		/* A remove constraint op. */
	edit_reference_op,	/* A reference change op. */
	edit_origin_op,		/* An origin change op. */
	edit_axis_op,		/* An axis change op. */
	edit_major_op,		/* A major alignment op. */
	edit_minor_op,		/* A minor alignment op. */
	edit_major_remove_op,	/* Removal of alignment constraints. */
	edit_minor_remove_op
	} EditOpType, *EditOpTypePtr;



extern EditInfoPtr	Edit_Get_Info();
extern void	Edit_Draw_Selection_Points(EditInfoPtr);
extern void	Edit_Cleanup_Selection(Boolean);
extern void	Edit_Clear_Info();
extern void	Edit_Transform_Vertices(Transformation*, EditInfoPtr);
extern void	Edit_Transform_Normals(Transformation*, EditInfoPtr);

extern Boolean	Edit_Update_Feature_Specs(FeaturePtr, Vector, Vector, Vector);
extern Boolean	Edit_Update_Object_Constraints(ConstraintBoxPtr, EditInfoPtr);
extern Boolean	Edit_Update_Active_Object_Cons(ConstraintBoxPtr, EditInfoPtr,
											   Vector);
extern Boolean	Edit_Update_Constraints(EditInfoPtr);

extern void	Edit_Calculate_Extras();
extern void	Edit_Calculate_Origin_Cons_Points();
extern void	Edit_Calculate_Scale_Cons_Points();
extern void	Edit_Calculate_Arc_Points(int, int);
extern void	Edit_Calculate_Spec_Points(FeaturePtr*, int, ConPoint**, int*);
extern void	Edit_Set_Origin_Defaults(FeaturePtr*, Vector);
extern void	Edit_Set_Scale_Defaults(FeaturePtr*, Vector, Matrix);
extern void	Edit_Set_Rotate_Defaults(FeaturePtr*, Matrix);

extern Matrix	Quaternion_To_Matrix(Quaternion);

extern void	Edit_Initialize_Shell(EditInfoPtr);
extern void	Edit_Match_Widths();
extern void	Edit_Cancel_Remove();
extern void	Edit_Set_Drag_Label(int, Vector, double);
extern void	Edit_Set_Major_Align_Label(Boolean);
extern void	Edit_Set_Minor_Align_Label(Boolean);

extern void	Edit_Create_Constraint_Box(ConstraintBoxPtr, FeaturePtr,
									   Widget, char*, Widget, Widget, int);
extern void	Edit_Constraint_Box_Resize(ConstraintBoxPtr);
extern void	Edit_Add_Constraint(ConstraintBoxPtr, int);
extern void	Constraint_Box_Clear(ConstraintBoxPtr);
extern void	Edit_Set_Constraint_States(ConstraintBoxPtr, Boolean*);
extern void	Edit_Select_Constraint(EditInfoPtr, int, ConstraintBoxPtr,
								   Boolean, Boolean, Boolean);
extern Boolean	Edit_Remove_Constraint(EditInfoPtr, ConstraintBoxPtr, int,
					   					Boolean);
extern void	Add_Object_Constraint(FeaturePtr, ConstraintBoxPtr, int);
extern void	Add_Dependency(ObjectInstancePtr, ObjectInstancePtr);
extern Boolean	Edit_Check_Cycles(ObjectInstancePtr,int*,FeatureSpecType*,int);

extern void	Add_Spec(FeatureSpecifier*, Vector, Vector, FeatureSpecType,
					 ObjectInstancePtr, int);

extern void	Edit_Draw(WindowInfoPtr, int, EditInfoPtr, Boolean);
extern void	Draw_Edit_Extras(WindowInfoPtr, int, EditInfoPtr, Boolean);
extern void	Draw_Origin_Constraints(WindowInfoPtr, int, EditInfoPtr, Boolean);
extern void	Draw_Scale_Constraints(WindowInfoPtr, int, EditInfoPtr, Boolean);
extern void	Draw_Rotate_Constraints(WindowInfoPtr, int, EditInfoPtr, Boolean);

extern void	Initiate_Object_Edit(WindowInfoPtr);
extern void	Cancel_Object_Edit();

extern void	Edit_Start_Origin_Drag(XEvent*, EditInfoPtr);
extern void	Edit_Continue_Origin_Drag(XEvent*, EditInfoPtr);
extern void	Edit_Finish_Origin_Drag(XEvent*, EditInfoPtr);
extern Boolean	Edit_Force_Origin_Satisfaction(EditInfoPtr);

extern void	Edit_Start_Scale_Drag(XEvent*, EditInfoPtr);
extern void	Edit_Continue_Scale_Drag(XEvent*, EditInfoPtr);
extern void	Edit_Finish_Scale_Drag(XEvent*, EditInfoPtr);
extern void	Edit_Force_Scale_Satisfaction(EditInfoPtr);
extern Boolean	Edit_Dynamic_Scale(EditInfoPtr info, Vector);
extern void	Scale_Calculate_Transform(Transformation*, Vector, Vector, Vector,
									  Vector, Matrix*, Matrix*, Boolean, 
										Vector*);
extern Boolean	Edit_Scale_Force_Constraints(EditInfoPtr, Boolean);

extern void	Edit_Start_Rotate_Drag(XEvent*, EditInfoPtr);
extern void	Edit_Continue_Rotate_Drag(XEvent*, EditInfoPtr);
extern void	Edit_Finish_Rotate_Drag(XEvent*, EditInfoPtr);
extern void	Edit_Major_Align(Vector, Boolean);
extern void	Edit_Minor_Align(Vector, Boolean);

extern void	Edit_Maintain_All_Constraints(EditInfoPtr);
extern void	Edit_Maintain_Free_List();

extern void	Edit_Reference_Callback(Widget, XtPointer, XtPointer);
extern void	Edit_Origin_Callback(Widget, XtPointer, XtPointer);
extern void	Edit_Change_Axis_1_Callback(Widget, XtPointer, XtPointer);
extern void	Edit_Change_Axis_2_Callback(Widget, XtPointer, XtPointer);

extern void	Edit_Undo();
extern void	Edit_Redo(Widget, XtPointer, XtPointer);
extern void	Edit_Undo_Register_State(EditOpType, int, int);
extern void	Edit_Undo_Clear();

extern void	Edit_Align_With_Point(Widget, XtPointer, XtPointer);
extern void	Edit_Align_With_Line(Widget, XtPointer, XtPointer);
extern void	Edit_Align_With_Plane(Widget, XtPointer, XtPointer);
extern void	Edit_Remove_Alignment(Widget, XtPointer, XtPointer);
extern void	Edit_Force_Rotation_Constraints(EditInfoPtr, Boolean);
extern void	Edit_Force_Alignment_Satisfaction(EditInfoPtr);
extern Boolean	Edit_Dynamic_Align(EditInfoPtr, Transformation*, Vector);
extern Matrix	Major_Align_Matrix(Vector, Matrix*, Quaternion*);
extern Matrix	Minor_Align_Matrix(Vector, Matrix*, Quaternion*);

extern Boolean	DFS(ObjectInstancePtr, ObjectInstancePtr *, int,
					Boolean, InstanceList*);

extern Widget	edit_form;
extern ConstraintBox	origin_constraints;
extern ConstraintBox	scale_constraints;
extern ConstraintBox	rotate_constraints;

extern Boolean	do_maintenance;
extern InstanceList	topological_list;


#endif /* __EDIT__ */
