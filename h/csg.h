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
**	csg.h : header info for the csg functions.
*/

typedef enum _CSGOperation {
	csg_union_op,
	csg_intersection_op,
	csg_difference_op,
	csg_leaf_op
	} CSGOperation;

typedef enum _CSGEvent {
	csg_none,
	csg_attach,
	csg_move
	} CSGEvent;

typedef struct _CSGNode {
	Widget				csg_widget;
	CSGOperation		csg_op;
	Cuboid				csg_bound;
	ObjectInstancePtr	csg_instance;
	struct _CSGNode		*csg_left_child;
	struct _CSGNode		*csg_right_child;
	struct _CSGNode		*csg_parent;
	} CSGNode, *CSGNodePtr;

typedef struct _CSGTreeRoot {
	CSGNodePtr		tree;
	InstanceList	instances;
	Boolean			displayed;
	BaseObjectPtr	*orig_bases;
	int				num_orig_bases;
	} CSGTreeRoot, *CSGTreeList;

#define CSG_Node_Root(n, r)	for ( r = n ; \
								  r->csg_parent != NULL ; \
								  r = r->csg_parent )

#define csg_select_new		1
#define csg_select_edit		2
#define	csg_select_delete	3
#define	csg_select_copy		4
#define csg_select_save		5
#define csg_select_change	6

extern Widget		csg_tree_widget;
extern CSGTreeList	displayed_trees;
extern int			num_displayed_trees;
extern int			max_num_displayed_trees;
extern char			*complete_label;

extern void Create_CSG_Display();
extern CSGNodePtr	Create_CSG_Node(CSGNodePtr,ObjectInstancePtr,CSGOperation);
extern Widget	Create_CSG_Widget(CSGNodePtr); 
extern void		CSG_Set_Widget_Values(CSGNodePtr); 

extern void	Add_Displayed_Tree(CSGNodePtr, Boolean, BaseObjectPtr);
extern CSGTreeRoot	Delete_Displayed_Tree(CSGNodePtr);
extern void	CSG_Remove_Instances_From_Root(CSGNodePtr, CSGNodePtr);
extern void	CSG_Add_Instances_To_Root(CSGNodePtr, CSGTreeList);
extern void	CSG_Add_Instances_To_Displayed(InstanceList);
extern void	CSG_Remove_Instances_From_Displayed(InstanceList);
extern void	CSG_Reorder_Root_Widgets();
extern void	CSG_Insert_Existing_Tree(CSGNodePtr, Boolean, BaseObjectPtr);
extern void	CSG_Update_Reference_Constraints(CSGNodePtr, CSGNodePtr);
extern void	CSG_Delete_Original_Base(BaseObjectPtr);


extern void			CSG_Move_Tree(CSGNodePtr, CSGNodePtr);
extern void			CSG_Add_Node(CSGNodePtr, CSGNodePtr, CSGOperation,
								 BaseObjectPtr*, int);
extern CSGNodePtr	CSG_Remove_Node(CSGNodePtr);
extern void			CSG_Delete_Tree(CSGNodePtr);
extern CSGNodePtr	CSG_Copy_Tree(CSGNodePtr, CSGNodePtr);
extern void			CSG_Reorder_Children(CSGNodePtr);
extern void			CSG_Check_Tree_Ordering(CSGNodePtr);
extern void			CSG_Free_Widgets(CSGNodePtr);

extern CSGNodePtr	Find_Widget_In_Displayed(Widget);
extern CSGNodePtr	Find_Widget_In_Tree(Widget, CSGNodePtr);
extern CSGNodePtr	Find_Instance_In_Displayed(ObjectInstancePtr);
extern CSGNodePtr	Find_Instance_In_Tree(ObjectInstancePtr, CSGNodePtr);


extern void	CSG_Menu_Notify_Func(Widget, XtPointer, XtPointer);
extern void Init_Motion_Sequence(Widget, XtPointer, XtPointer);
extern void	Combine_Menu_Callback(Widget, XtPointer, XtPointer);
extern void	Cancel_Combine_Menu_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Display_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Complete_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Attach_Instance(ObjectInstancePtr);
extern void	CSG_Break_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Delete_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Copy_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Reorder_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Preview_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Modify_Existing_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Save_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Delete_Existing_Callback(Widget, XtPointer, XtPointer);
extern void	CSG_Copy_Existing_Callback(Widget, XtPointer, XtPointer);


extern void	CSG_Select_Popup(int);
extern void	CSG_Add_Select_Option(BaseObjectPtr);
extern void	CSG_Select_Destroy_Widget(int);

extern struct _CSGWireframe	*CSG_Generate_CSG_Wireframe(CSGNodePtr);

extern void		Calculate_CSG_Bounds(CSGNodePtr);

