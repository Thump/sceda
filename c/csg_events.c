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
**	csg_events.c: Functions for dealing with events in the csg tree window.
*/

#include <sced.h>
#include <csg.h>
#include <instance_list.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Tree.h>
#include <X11/Xmu/Drawing.h>

#define Calculate_Motion_Rectangle(e)	\
				motion_rectangle.x = e->xmotion.x - motion_rectangle.width / 2;\
				motion_rectangle.y = e->xmotion.y - motion_rectangle.height / 2;


static void	Create_Complete_Shell();
static void	CSG_Complete_Name_Callback(Widget, XtPointer, XtPointer);
static void	CSG_Initialise_Preview();
static void	CSG_Prepare_Preview(CSGNodePtr);

extern Widget	csg_display_label[2];

char	*complete_label = NULL;

static CSGEvent	current_event = csg_none;
static Widget	current_widget;

static Boolean	drawing = FALSE;

static Widget	destination_widget;

static XRectangle	motion_rectangle;
static GC			motion_rect_gc = 0;

static CSGNodePtr	complete_tree;
static Boolean		attach_after;
static Boolean		attach_left;
static CSGOperation	attach_op;
static CSGNodePtr	attach_sibling;
static Widget		complete_label_shell;
static Widget		complete_label_dialog;

static WindowInfo		csg_preview_window;
static BaseObject		csg_preview_base;
static ObjectInstance	csg_preview_obj;

static Boolean			preview_initialised = FALSE;


void
CSG_Tree_Notify_Func(Widget w, XEvent *e, String *s, Cardinal *num)
{
	if ( drawing )
	{
		XDrawRectangles(XtDisplay(csg_tree_widget), XtWindow(csg_tree_widget),
						motion_rect_gc, &motion_rectangle, 1);
		drawing = FALSE;
	}

	if ( current_event == csg_attach )
	{
		XBell(XtDisplay(csg_window.shell), 0);
		XtRemoveGrab(csg_tree_widget);
		current_event = csg_none;
	}

	if ( current_event == csg_move )
	{
		CSGNodePtr	destination_node;
		Position	x;
		int			i, min_dist;

		/* Find the node closest to the left. */
		min_dist = MAX_SIGNED_SHORT;
		destination_node = NULL;
		for ( i = 0 ; i < num_displayed_trees ; i++ )
		{
			XtVaGetValues(displayed_trees[i].tree->csg_widget, XtNx, &x, NULL);

			if ( e->xmotion.x - (int)x > 0 && e->xmotion.x - (int)x < min_dist )
				destination_node = displayed_trees[i].tree;
		}

		/* Find the current widget's node. */
		for ( i = 0 ;
			  displayed_trees[i].tree->csg_widget != current_widget ;
			  i++ );

		CSG_Move_Tree(displayed_trees[i].tree, destination_node);

		XtRemoveGrab(csg_tree_widget);
		current_event = csg_none;
	}
}


void
CSG_Tree_Motion_Func(Widget w, XEvent *e, String *s, Cardinal *num)
{
	if ( drawing )
	{
		XDrawRectangles(XtDisplay(csg_tree_widget), XtWindow(csg_tree_widget),
						motion_rect_gc, &motion_rectangle, 1);
		Calculate_Motion_Rectangle(e);
		XDrawRectangles(XtDisplay(csg_tree_widget), XtWindow(csg_tree_widget),
						motion_rect_gc, &motion_rectangle, 1);
	}
}


void
CSG_Menu_Notify_Func(Widget w, XtPointer cl, XtPointer ca)
{
	int	i;

	if ( drawing )
	{
		XDrawRectangles(XtDisplay(csg_tree_widget), XtWindow(csg_tree_widget),
						motion_rect_gc, &motion_rectangle, 1);
		drawing = FALSE;
	}

	if ( current_event == csg_attach )
	{
		if ( w == current_widget )
		{
			XBell(XtDisplay(csg_window.shell), 0);
			XtRemoveGrab(csg_tree_widget);
			current_event = csg_none;
			Popup_Error("Cannot Attach a tree to itself", csg_window.shell,
						"Error");
			return;
		}
		else
		{
			XtVaSetValues(w, XtNmenuName, "AttachMenu", NULL);
			destination_widget = w;
		}
	}

	if ( current_event == csg_move )
	{
		CSGNodePtr	destination_node;
		int			i;

		/* Find this widget's tree root. */
		CSG_Node_Root(Find_Widget_In_Displayed(w), destination_node);

		/* Find the current widget's node. */
		for ( i = 0 ;
			  displayed_trees[i].tree->csg_widget != current_widget ;
			  i++ );

		CSG_Move_Tree(displayed_trees[i].tree, destination_node);

		XtRemoveGrab(csg_tree_widget);
		current_event = csg_none;
	}

	if ( current_event == csg_none )
	{
		current_widget = w;
		/* Try to find it as a top level widget. */
		for ( i = 0 ;
			  i < num_displayed_trees &&
			  displayed_trees[i].tree->csg_widget != current_widget ;
			  i++ );
		if ( i != num_displayed_trees )
		{
			/* Set the display flag. */
			if ( displayed_trees[i].displayed )
			{
				XtVaSetValues(csg_display_label[0], XtNlabel, "Hide", NULL);
				XtVaSetValues(csg_display_label[1], XtNlabel, "Hide", NULL);
			}
			else
			{
				XtVaSetValues(csg_display_label[0], XtNlabel, "Display", NULL);
				XtVaSetValues(csg_display_label[1], XtNlabel, "Display", NULL);
			}

		}
	}

}



void
Init_Motion_Sequence(Widget w, XtPointer cl, XtPointer ca)
{
	Position	x, y;
	Dimension	width, height;

	drawing = TRUE;
	current_event = (CSGEvent)cl;
	XtAddGrab(csg_tree_widget, TRUE, FALSE);

	if ( ! motion_rect_gc )
	{
		XGCValues	gc_vals;
		Colormap	col_map;
		Pixel		foreground_pixel, background_pixel;

		/* Get the colormap. */
		XtVaGetValues(current_widget, XtNcolormap, &col_map,
					  XtNforeground, &foreground_pixel,
					  XtNbackground, &background_pixel, NULL);

		gc_vals.function = GXxor;

		if (DefaultDepthOfScreen(XtScreen(csg_window.shell)) == 1)
			gc_vals.foreground = foreground_pixel;
		else
		{
			XColor	motion_color;

			/* Allocate the color for the drag rectangle. */
			motion_color.red = MAX_UNSIGNED_SHORT;
			motion_color.green = 0;
			motion_color.blue = 0;
			XAllocColor(XtDisplay(csg_window.shell), col_map, &motion_color);

			gc_vals.foreground = motion_color.pixel ^ background_pixel;
		}
		/* Allocate the GCs. */
		gc_vals.foreground ^= background_pixel;
		motion_rect_gc = XtGetGC(csg_window.shell,
								 GCFunction | GCForeground, &gc_vals);
	}

	XtVaGetValues(current_widget, XtNx, &x, XtNy, &y,
				  XtNwidth, &width, XtNheight, &height, NULL);
	motion_rectangle.x = (short)x;
	motion_rectangle.y = (short)y;
	motion_rectangle.width = (unsigned short)width;
	motion_rectangle.height = (unsigned short)height;

	XDrawRectangles(XtDisplay(csg_tree_widget), XtWindow(csg_tree_widget),
					motion_rect_gc, &motion_rectangle, 1);

}



void
Combine_Menu_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSGNodePtr	src, dest, dest_parent;
	int			i;
	BaseObjectPtr	*bases;
	int				num_bases;

	XtRemoveGrab(csg_tree_widget);
	current_event = csg_none;

	for ( i = 0 ; displayed_trees[i].tree->csg_widget != current_widget ; i++ );
	src = displayed_trees[i].tree;
	bases = displayed_trees[i].orig_bases;
	num_bases = displayed_trees[i].num_orig_bases;

	dest = Find_Widget_In_Displayed(destination_widget);

	/* Check for dest among src's children. */
	CSG_Node_Root(dest, dest_parent);
	if ( src == dest_parent )
	{
		XBell(XtDisplay(csg_window.shell), 0);
		return;
	}

	/* The src is at the root of the tree. Delete it from the displayed trees.*/
	Delete_Displayed_Tree(src);
	src->csg_parent = NULL;

	/* Combine it using the appropriate operator. */
	CSG_Add_Node(src, dest, (CSGOperation)cl, bases, num_bases);

	if ( num_bases )
		free(bases);

	XawTreeForceLayout(csg_tree_widget);

	if ( src->csg_parent->csg_parent )
		CSG_Check_Tree_Ordering(src->csg_parent->csg_parent);

	changed_scene = TRUE;
}

void
CSG_Menu_Button_Up_Func(Widget w, XEvent *e, String *s, Cardinal *n)
{
	CSGNodePtr  node;

	if ( current_event == csg_attach && current_widget != w )
	{
		node = Find_Widget_In_Displayed(destination_widget);
		destination_widget = w;

		if ( node->csg_op == csg_leaf_op )
		{
			if ( node->csg_parent )
				XtVaSetValues(destination_widget, XtNmenuName, "ExternalMenu",
							NULL);
			else
				XtVaSetValues(destination_widget, XtNmenuName,
							"ExternalRootMenu", NULL);
		}
		else
		{
			if ( node->csg_parent )
				XtVaSetValues(destination_widget, XtNmenuName, "InternalMenu",
							NULL);
			else
				XtVaSetValues(destination_widget, XtNmenuName,
							"InternalRootMenu", NULL);
		}
	}
}

void
Cancel_Combine_Menu_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSGNodePtr	node;

	node = Find_Widget_In_Displayed(destination_widget);

	XtRemoveGrab(csg_tree_widget);
	current_event = csg_none;

	if ( node->csg_op == csg_leaf_op )
	{
		if ( node->csg_parent )
			XtVaSetValues(destination_widget, XtNmenuName, "ExternalMenu",NULL);
		else
			XtVaSetValues(destination_widget, XtNmenuName, "ExternalRootMenu",
							NULL);
	}
	else
	{
		if ( node->csg_parent )
			XtVaSetValues(destination_widget, XtNmenuName, "InternalMenu",
				NULL);
		else
			XtVaSetValues(destination_widget, XtNmenuName, "InternalRootMenu",
				NULL);
	}
}


void
CSG_Display_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	int				i;

	for ( i = 0 ; displayed_trees[i].tree->csg_widget != current_widget ; i++ );

	if ( displayed_trees[i].displayed )
	{
		CSG_Remove_Instances_From_Displayed(displayed_trees[i].instances);
		displayed_trees[i].displayed = FALSE;
	}
	else
	{
		CSG_Add_Instances_To_Displayed(displayed_trees[i].instances);
		displayed_trees[i].displayed = TRUE;
	}
}


void
CSG_Complete_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSGNodePtr	tree;
	int			i = 0;
	char		default_name[16];

	attach_after =  ( cl ? TRUE : FALSE );
	if ( ! attach_after )
	{
		for ( i = 0 ;
			  displayed_trees[i].tree->csg_widget != current_widget ;
			  i++ );
		tree = displayed_trees[i].tree;
	}
	else
		tree = Find_Widget_In_Displayed(current_widget);

	/* Prompt for the name (and give an opportunity to cancel). */
	if ( ! complete_label_shell )
		Create_Complete_Shell();
	sprintf(default_name, "CSG_%d", object_count[csg_obj]);
	XtVaSetValues(complete_label_dialog, XtNvalue, default_name, NULL);

	SFpositionWidget(complete_label_shell);
	XtPopup(complete_label_shell, XtGrabExclusive);

	complete_tree = tree;
}

static void
CSG_Complete_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(complete_label_shell);
}


static void
CSG_Modify_Dependent_Instances(BaseObjectPtr old, BaseObjectPtr new)
{
	InstanceList	main_instances = NULL;
	InstanceList	csg_instances = NULL;
	int	i;

	for ( i = old->b_num_instances - 1 ; i >= 0 ; i-- )
	{
		if ( old->b_instances[i]->o_flags & ObjPending )
		{
			Base_Change(old->b_instances[i], new, FALSE, NULL);
			if ( Find_Object_In_Instances(old->b_instances[i],
										  main_window.all_instances) )
				Insert_Element(&main_instances, old->b_instances[i]);
			else if ( Find_Object_In_Instances(old->b_instances[i],
											   csg_window.all_instances) )
				Insert_Element(&csg_instances, old->b_instances[i]);
		}
	}
	View_Update(&main_window, main_instances, CalcView);
	View_Update(&csg_window, csg_instances, CalcView);
	Update_Projection_Extents(main_instances);
	Update_Projection_Extents(csg_instances);
}

BaseObjectPtr
CSG_Complete_Tree(CSGNodePtr tree, char *label, Boolean is_root)
{
	CSGTreeRoot		old_root;
	BaseObjectPtr	new_base;
	int				i;

	CSG_Free_Widgets(tree);

	CSG_Update_Reference_Constraints(tree, tree);

	if ( is_root )
		old_root = Delete_Displayed_Tree(tree);

	new_base = Add_CSG_Base_Object(tree, label, NULL, NULL);

	object_count[csg_obj]++;

	Select_Base_Reference(new_base);

	CSG_Add_Select_Option(new_base);

	if ( is_root )
	{
		/* Modify any instances. */
		for ( i = 0 ; i < old_root.num_orig_bases ; i++ )
			CSG_Modify_Dependent_Instances(old_root.orig_bases[i], new_base);
		if ( old_root.num_orig_bases )
			free(old_root.orig_bases);
	}

	return new_base;
}


static void
CSG_Complete_Name_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	BaseObjectPtr	new_base;
	Cursor	time_cursor;

	if ( cl )
	{
		XtPopdown(complete_label_shell);
		complete_label = XawDialogGetValueString(complete_label_dialog);
	}

	/* Change the cursor. */
	time_cursor = XCreateFontCursor(XtDisplay(csg_window.shell), XC_watch);
    XDefineCursor(XtDisplay(csg_window.shell),
					XtWindow(csg_window.shell), time_cursor);
	XFlush(XtDisplay(csg_window.shell));

	/* We now have a name, move on with it. */
	if ( attach_after )
	{
		if ( complete_tree == complete_tree->csg_parent->csg_left_child )
		{
			attach_left = TRUE;
			attach_sibling = complete_tree->csg_parent->csg_right_child;
		}
		else
		{
			attach_left = FALSE;
			attach_sibling = complete_tree->csg_parent->csg_left_child;
		}
		attach_op = complete_tree->csg_parent->csg_op;
		CSG_Remove_Node(complete_tree);
	}

	new_base = CSG_Complete_Tree(complete_tree, complete_label, ! attach_after);

	if ( attach_after )
		Create_New_Object_From_Base(&csg_window, new_base, TRUE);

	attach_after = FALSE;

	/* Change the cursor back. */
    XDefineCursor(XtDisplay(csg_window.shell), XtWindow(csg_window.shell),None);
	XFreeCursor(XtDisplay(csg_window.shell), time_cursor);

	if ( cl )
		complete_label = NULL;
	else
		free(complete_label);


}


void
CSG_Attach_Instance(ObjectInstancePtr inst)
{
	CSGNodePtr	new_node;

	new_node = Create_CSG_Node(NULL, inst, csg_leaf_op);
	Create_CSG_Widget(new_node);

	CSG_Add_Node(new_node, attach_sibling, attach_op, NULL, 0);

	if ( attach_left )
		CSG_Reorder_Children(new_node->csg_parent);

	XawTreeForceLayout(csg_tree_widget);
}


void
CSG_Complete_Action_Func(Widget w, XEvent *e,String *s, Cardinal *num)
{
	CSG_Complete_Name_Callback(w, (XtPointer)TRUE, NULL);
}


static void
Create_Complete_Shell()
{
	Arg		args[5];
	int		n;

	XtSetArg(args[0], XtNallowShellResize, TRUE);
	complete_label_shell = XtCreatePopupShell("Complete",
						transientShellWidgetClass, csg_window.shell, args, 1);

	/* Create the dialog widget to go inside the shell. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "Object Name:");	n++;
	XtSetArg(args[n], XtNvalue, "");				n++;
	complete_label_dialog = XtCreateManagedWidget("completeDialog",
							dialogWidgetClass, complete_label_shell, args, n);

	/* Add the buttons at the bottom of the dialog. */
	XawDialogAddButton(complete_label_dialog, "Complete",
					   CSG_Complete_Name_Callback, (XtPointer)TRUE);
	XawDialogAddButton(complete_label_dialog, "Cancel",
					   CSG_Complete_Cancel_Callback, NULL);

	XtOverrideTranslations(XtNameToWidget(complete_label_dialog, "value"),
		XtParseTranslationTable(":<Key>Return: csg_complete()"));

	XtRealizeWidget(complete_label_shell);
}


void
CSG_Break_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSGNodePtr	node;
	CSGNodePtr	root;
	CSGNodePtr	parent = NULL;
	int			i;

	node = Find_Widget_In_Displayed(current_widget);
	CSG_Node_Root(node, root);
	for ( i = 0 ; displayed_trees[i].tree != root ; i++ );

	if ( node->csg_parent )
		parent = node->csg_parent->csg_parent;
	CSG_Remove_Node(node);

	Add_Displayed_Tree(node, displayed_trees[i].displayed, NULL);

	XawTreeForceLayout(csg_tree_widget);

	if ( parent )
		CSG_Check_Tree_Ordering(parent);

	changed_scene = TRUE;
}


void
CSG_Delete_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSGNodePtr	node;
	CSGNodePtr	parent = NULL;
	CSGTreeRoot	old_root;

	node = Find_Widget_In_Displayed(current_widget);

	if ( node->csg_parent )
	{
		parent = node->csg_parent->csg_parent;
		CSG_Remove_Node(node);
	}
	else
	{
		old_root = Delete_Displayed_Tree(node);
		if ( old_root.num_orig_bases )
			free(old_root.orig_bases);
	}

	CSG_Delete_Tree(node);

	XawTreeForceLayout(csg_tree_widget);

	if ( parent )
		CSG_Check_Tree_Ordering(parent);

	changed_scene = TRUE;
}


void
CSG_Copy_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSGNodePtr	src = Find_Widget_In_Displayed(current_widget);
	CSGNodePtr	new_tree = CSG_Copy_Tree(src, NULL);

	Add_Displayed_Tree(new_tree, FALSE, NULL);

	XawTreeForceLayout(csg_tree_widget);

	changed_scene = TRUE;
}


void
CSG_Reorder_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSG_Reorder_Children(Find_Widget_In_Displayed(current_widget));
	XawTreeForceLayout(csg_tree_widget);

	changed_scene = TRUE;
}

void
CSG_Save_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSG_Select_Popup(csg_select_save);
}


void
CSG_Modify_Existing_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSG_Select_Popup(csg_select_edit);
}


void
CSG_Copy_Existing_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSG_Select_Popup(csg_select_copy);
}


void
CSG_Delete_Existing_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSG_Select_Popup(csg_select_delete);
}

static void
CSG_Update_Tree_Spec(FeatureSpecPtr spec, ObjectInstancePtr target,
					void *ptr, int abs)
{
	CSGNodePtr	tree = (CSGNodePtr)ptr;
	Vector	temp;

	/* Only check reference specs. */
	if ( spec->spec_type != reference_spec )	return;

	/* If the obj is in the list, ignore it. */
	if ( Find_Instance_In_Tree(spec->spec_object, tree) )
		return;

	/* Otherwise, remove it. */
	Edit_Remove_Obj_From_Dependencies(spec, target, NULL, 0);

	if ( abs )
	{
		spec->spec_type = absolute_spec;
		Transform_Vector(spec->spec_object->o_transform, spec->spec_vector,
						 spec->spec_vector);
	}
	else
	{
		spec->spec_type = offset_spec;
		Transform_Vector(spec->spec_object->o_transform, spec->spec_vector,
						 temp);
		VSub(temp, target->o_world_verts[target->o_num_vertices - 1],
			 spec->spec_vector);
	}
}



static void
CSG_Remove_Dependencies(ObjectInstancePtr src, ObjectInstancePtr obj,
						CSGNodePtr tree)
{
	/* If the obj is in the list, ignore it. */
	if ( Find_Instance_In_Tree(src, tree) )
		return;

	Constraint_Remove_References(src, obj);
}


void
CSG_Update_Reference_Constraints(CSGNodePtr target, CSGNodePtr tree)
{
	int	i;

	if ( ! target ) return;

	if ( target->csg_op == csg_leaf_op )
	{
		for ( i = 0 ; i < target->csg_instance->o_origin_num ; i++ )
			Constraint_Manipulate_Specs(target->csg_instance->o_origin_cons + i,
									   target->csg_instance, (void*)tree, 1,
									   CSG_Update_Tree_Spec);
		for ( i = 0 ; i < target->csg_instance->o_scale_num ; i++ )
			Constraint_Manipulate_Specs(target->csg_instance->o_scale_cons + i,
									   target->csg_instance, (void*)tree, 0,
									   CSG_Update_Tree_Spec);
		for ( i = 0 ; i < target->csg_instance->o_rotate_num ; i++ )
			Constraint_Manipulate_Specs(target->csg_instance->o_rotate_cons + i,
									   target->csg_instance, (void*)tree, 0,
									   CSG_Update_Tree_Spec);
		Constraint_Manipulate_Specs(&(target->csg_instance->o_major_align),
								   target->csg_instance, (void*)tree, 0,
								   CSG_Update_Tree_Spec);
		Constraint_Manipulate_Specs(&(target->csg_instance->o_minor_align),
								   target->csg_instance, (void*)tree, 0,
								   CSG_Update_Tree_Spec);

		for ( i = target->csg_instance->o_num_depend - 1 ; i >= 0 ; i-- )
			CSG_Remove_Dependencies(target->csg_instance->o_dependents[i].obj,
									target->csg_instance, tree);
		return;
	}

	CSG_Update_Reference_Constraints(target->csg_left_child, tree);
	CSG_Update_Reference_Constraints(target->csg_right_child, tree);
}


void
CSG_Preview_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	CSGNodePtr		tree = Find_Widget_In_Displayed(current_widget);

	if ( ! preview_initialised )
		CSG_Initialise_Preview();

	CSG_Prepare_Preview(tree);

	Preview_Callback(NULL, (XtPointer)&csg_preview_window, NULL);
	Preview_Sensitize(FALSE);

}


static void
CSG_Prepare_Preview(CSGNodePtr tree)
{
	if ( csg_preview_window.selected_instances )
		free(csg_preview_window.selected_instances);

	csg_preview_window = csg_window;

	csg_preview_window.selected_instances = New(InstanceListElmt, 1);
	csg_preview_window.selected_instances->next =
	csg_preview_window.selected_instances->prev = NULL;
	csg_preview_window.selected_instances->the_instance = &csg_preview_obj;

	csg_preview_base.b_csgptr = tree;

	Calculate_CSG_Bounds(tree);
}


static void
CSG_Initialise_Preview()
{
	csg_preview_window.selected_instances = NULL;

	csg_preview_base.b_label = "CSGPreviewObj";
	csg_preview_base.b_class = csg_obj;

	csg_preview_obj.o_label = "CSGPreviewObj";
	csg_preview_obj.o_parent = &csg_preview_base;
	Identity_Transform(csg_preview_obj.o_transform);
	Identity_Transform(csg_preview_obj.o_inverse);
	csg_preview_obj.o_attribs = (void*)&default_attributes;

	preview_initialised = TRUE;
}


