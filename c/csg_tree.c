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
**	csg_tree: csg tree manipulation functions.
*/

#include <sced.h>
#include <csg.h>
#include <edit.h>
#include <hash.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Tree.h>

static void	CSG_Delete_Node(CSGNodePtr);
static void	CSG_Reorder_Child_Widgets(CSGNodePtr);

static Pixmap	union_bitmap = 0;
#define union_width 16
#define union_height 16
static char union_bits[] = {
   0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38,
   0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38,
   0x3c, 0x3c, 0xf8, 0x1f, 0xf0, 0x0f, 0xc0, 0x03};

static Pixmap	intersection_bitmap = 0;
#define intersection_width 16
#define intersection_height 16
static char intersection_bits[] = {
   0xc0, 0x03, 0xf0, 0x0f, 0xf8, 0x1f, 0x3c, 0x3c, 0x1c, 0x38, 0x1c, 0x38,
   0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38,
   0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38};

static Pixmap	difference_bitmap = 0;
#define difference_width 16
#define difference_height 16
static char difference_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static String	node_translations =
				"<EnterWindow>: highlight() \n\
				 <LeaveWindow>: reset() \n\
				 <BtnDown>:	set() notify() reset() PopupMenu() menu_notify()";


/*
**	CSGNodePtr
**	Create_CSG_Node(CSGNodePtr parent, ObjectInstancePtr inst, CSGOperation op)
**	Creates a new node, giving it the given parameters. It DOES NOT attach it
**	to the tree.
*/
CSGNodePtr
Create_CSG_Node(CSGNodePtr parent, ObjectInstancePtr inst, CSGOperation op)
{
	CSGNodePtr	result = New(CSGNode, 1);

	result->csg_op = op;
	result->csg_instance = inst;
	result->csg_left_child = NULL;
	result->csg_right_child = NULL;
	result->csg_parent = parent;
	result->csg_widget = NULL;

	return result;
}


/*	Widget
**	Create_CSG_Widget(CSGNodePtr node)
**	Creates the widget for a csg node. Labels it appropriately and (by
**	virtue of creation) adds it to the tree widget. Returns the Widget as
**	well as filling the appropriate field of the node.
*/
Widget
Create_CSG_Widget(CSGNodePtr node)
{
	Arg	args[10];
	int	n = 0;

	switch ( node->csg_op )
	{
		case csg_union_op:
			if ( ! union_bitmap )
				union_bitmap = XCreateBitmapFromData(
								XtDisplay(csg_window.shell),
								XtWindow(csg_window.view_widget),
								union_bits, union_width, union_height);
			XtSetArg(args[n], XtNbitmap, union_bitmap);	n++;
			break;

		case csg_intersection_op:
			if ( ! intersection_bitmap )
				intersection_bitmap = XCreateBitmapFromData(
								XtDisplay(csg_window.shell),
								XtWindow(csg_window.view_widget),
								intersection_bits,
								intersection_width, intersection_height);
			XtSetArg(args[n], XtNbitmap, intersection_bitmap);	n++;
			break;

		case csg_difference_op:
			if ( ! difference_bitmap )
				difference_bitmap = XCreateBitmapFromData(
								XtDisplay(csg_window.shell),
								XtWindow(csg_window.view_widget),
								difference_bits,
								difference_width, difference_height);
			XtSetArg(args[n], XtNbitmap, difference_bitmap);	n++;
			break;

		case csg_leaf_op:
			XtSetArg(args[n], XtNlabel, node->csg_instance->o_label);	n++;
			break;
	}

#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);		n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 50);					n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);					n++;
#endif
	if ( node->csg_parent )
	{
		XtSetArg(args[n], XtNtreeParent, node->csg_parent->csg_widget);	n++;
		if ( node->csg_op == csg_leaf_op )
		{
			XtSetArg(args[n], XtNmenuName, "ExternalMenu");	n++;
		}
		else
		{
			XtSetArg(args[n], XtNmenuName, "InternalMenu");	n++;
		}
	}
	else
	{
		XtSetArg(args[n], XtNtreeParent, NULL);					n++;
		if ( node->csg_op == csg_leaf_op )
		{
			XtSetArg(args[n], XtNmenuName, "ExternalRootMenu");	n++;
		}
		else
		{
			XtSetArg(args[n], XtNmenuName, "InternalRootMenu");	n++;
		}
	}


	node->csg_widget = XtCreateManagedWidget("treeNode", menuButtonWidgetClass,
							csg_tree_widget, args, n);
	XtAddCallback(node->csg_widget, XtNcallback, CSG_Menu_Notify_Func,
					(XtPointer)node);
	XtOverrideTranslations(node->csg_widget,
							XtParseTranslationTable(node_translations));

	return node->csg_widget;
}


void
CSG_Set_Widget_Values(CSGNodePtr node)
{
	Arg	args[5];
	int	n = 0;

	switch ( node->csg_op )
	{
		case csg_union_op:
			if ( ! union_bitmap )
				union_bitmap = XCreateBitmapFromData(
								XtDisplay(csg_window.shell),
								XtWindow(csg_window.view_widget),
								union_bits, union_width, union_height);
			XtSetArg(args[n], XtNbitmap, union_bitmap);	n++;
			break;

		case csg_intersection_op:
			if ( ! intersection_bitmap )
				intersection_bitmap = XCreateBitmapFromData(
								XtDisplay(csg_window.shell),
								XtWindow(csg_window.view_widget),
								intersection_bits,
								intersection_width, intersection_height);
			XtSetArg(args[n], XtNbitmap, intersection_bitmap);	n++;
			break;

		case csg_difference_op:
			if ( ! difference_bitmap )
				difference_bitmap = XCreateBitmapFromData(
								XtDisplay(csg_window.shell),
								XtWindow(csg_window.view_widget),
								difference_bits,
								difference_width, difference_height);
			XtSetArg(args[n], XtNbitmap, difference_bitmap);	n++;
			break;

		case csg_leaf_op:
			XtSetArg(args[n], XtNlabel, node->csg_instance->o_label);	n++;
			XtSetArg(args[n], XtNbitmap, NULL);							n++;
			break;
	}

	if ( node->csg_parent )
	{
		if ( node->csg_op == csg_leaf_op )
		{
			XtSetArg(args[n], XtNmenuName, "ExternalMenu");	n++;
		}
		else
		{
			XtSetArg(args[n], XtNmenuName, "InternalMenu");	n++;
		}
	}
	else
	{
		if ( node->csg_op == csg_leaf_op )
		{
			XtSetArg(args[n], XtNmenuName, "ExternalRootMenu");	n++;
		}
		else
		{
			XtSetArg(args[n], XtNmenuName, "InternalRootMenu");	n++;
		}
	}

	XtSetValues(node->csg_widget, args, n);
}


/*
**	CSG_Move_Tree(CSGNodePtr, CSGNodePtr)
**	Moves the target node so that it appears after the dest node in the
**	widget tree.
*/
void
CSG_Move_Tree(CSGNodePtr src, CSGNodePtr dest)
{
	CSGTreeRoot	src_root;
	Widget	temp_widg;
	char	*label, *src_label;
	Pixmap	bitmap, src_bitmap;
	Arg		get_args[2], set_args[2];
	int		src_index, dest_index;
	int		i;

	/* Find which trees they are. */
	for ( src_index = 0 ;
		  displayed_trees[src_index].tree != src ;
		  src_index++ );
	if ( dest )
		for ( dest_index = 0 ;
			  displayed_trees[dest_index].tree != dest ;
			  dest_index++ );
	else
		dest_index = -1;

	if ( dest_index == src_index || dest_index == src_index - 1 ) return;

	XtSetArg(get_args[0], XtNbitmap, &bitmap);
	XtSetArg(get_args[1], XtNlabel, &label);

	XtVaGetValues(src->csg_widget, XtNbitmap, &src_bitmap,
				  XtNlabel, &src_label, NULL);
	src_label=Strdup(src_label);

	src_root = displayed_trees[src_index];
	if ( src_index > dest_index )
	{
		dest_index++;
		for ( i = src_index ; i > dest_index ; i-- )
		{
			XtGetValues(displayed_trees[i-1].tree->csg_widget, get_args, 2);
			XtSetArg(set_args[0], XtNlabel, label);
			XtSetArg(set_args[1], XtNbitmap, bitmap);
			XtSetValues(displayed_trees[i].tree->csg_widget, set_args, 2);
			displayed_trees[i] = displayed_trees[i-1];
		}
		displayed_trees[dest_index] = src_root;
		XtVaSetValues(displayed_trees[dest_index+1].tree->csg_widget,
					  XtNbitmap, src_bitmap,
					  XtNlabel, src_label, NULL);

		temp_widg = displayed_trees[dest_index].tree->csg_widget;
		for ( i = dest_index ; i < src_index ; i++ )
		{
			displayed_trees[i].tree->csg_widget =
										displayed_trees[i+1].tree->csg_widget;
			if ( displayed_trees[i].tree->csg_left_child )
				XtVaSetValues(
					displayed_trees[i].tree->csg_left_child->csg_widget,
					XtNtreeParent, displayed_trees[i].tree->csg_widget,
					NULL);
			if ( displayed_trees[i].tree->csg_right_child )
				XtVaSetValues(
					displayed_trees[i].tree->csg_right_child->csg_widget,
					XtNtreeParent, displayed_trees[i].tree->csg_widget,
					NULL);
		}
		displayed_trees[src_index].tree->csg_widget = temp_widg;
		if ( displayed_trees[src_index].tree->csg_left_child )
			XtVaSetValues(
				displayed_trees[src_index].tree->csg_left_child->csg_widget,
				XtNtreeParent, displayed_trees[src_index].tree->csg_widget,
				NULL);
		if ( displayed_trees[src_index].tree->csg_right_child )
			XtVaSetValues(
				displayed_trees[src_index].tree->csg_right_child->csg_widget,
				XtNtreeParent, displayed_trees[src_index].tree->csg_widget,
				NULL);
	}
	else
	{
		for ( i = src_index ; i < dest_index ; i++ )
		{
			XtGetValues(displayed_trees[i+1].tree->csg_widget, get_args, 2);
			XtSetArg(set_args[0], XtNlabel, label);
			XtSetArg(set_args[1], XtNbitmap, bitmap);
			XtSetValues(displayed_trees[i].tree->csg_widget, set_args, 2);
			displayed_trees[i] = displayed_trees[i+1];
		}
		displayed_trees[dest_index] = src_root;
		XtVaSetValues(displayed_trees[dest_index-1].tree->csg_widget,
					  XtNbitmap, src_bitmap,
					  XtNlabel, src_label, NULL);

		temp_widg = displayed_trees[dest_index].tree->csg_widget;
		for ( i = dest_index ; i > src_index ; i-- )
		{
			displayed_trees[i].tree->csg_widget =
				displayed_trees[i-1].tree->csg_widget;
			if ( displayed_trees[i].tree->csg_left_child )
				XtVaSetValues(
					displayed_trees[i].tree->csg_left_child->csg_widget,
					XtNtreeParent, displayed_trees[i].tree->csg_widget,
					NULL);
			if ( displayed_trees[i].tree->csg_right_child )
				XtVaSetValues(
					displayed_trees[i].tree->csg_right_child->csg_widget,
					XtNtreeParent, displayed_trees[i].tree->csg_widget,
					NULL);
		}
		displayed_trees[src_index].tree->csg_widget = temp_widg;
		if ( displayed_trees[src_index].tree->csg_left_child )
			XtVaSetValues(
				displayed_trees[src_index].tree->csg_left_child->csg_widget,
				XtNtreeParent, displayed_trees[src_index].tree->csg_widget,
				NULL);
		if ( displayed_trees[src_index].tree->csg_right_child )
			XtVaSetValues(
				displayed_trees[src_index].tree->csg_right_child->csg_widget,
				XtNtreeParent, displayed_trees[src_index].tree->csg_widget,
				NULL);
	}
	free(src_label);

	XawTreeForceLayout(csg_tree_widget);
}


/*
**	CSG_Remove_Node(CSGNodePtr target)
**	Removes the target node from the CSG tree it is in.
**	Returns the removed node.
*/
CSGNodePtr
CSG_Remove_Node(CSGNodePtr target)
{
	CSGNodePtr	root_node;

	/* Remove the node instances from the root's list. */
	CSG_Node_Root(target, root_node);
	CSG_Remove_Instances_From_Root(target, root_node);

	/* There is at least one node above this one. We need to delete it
	** and put the target's sibling there instead.
	*/

	/* If the parent is at the top, it's a special case. */
	if ( target->csg_parent == root_node )
	{
		int	root_index;

		for ( root_index = 0 ; displayed_trees[root_index].tree != root_node ;
			  root_index++ );

		if ( target == target->csg_parent->csg_left_child )
			displayed_trees[root_index].tree =
										target->csg_parent->csg_right_child;
		else
			displayed_trees[root_index].tree =
										target->csg_parent->csg_left_child;
		displayed_trees[root_index].tree->csg_parent = NULL;
		XtDestroyWidget(displayed_trees[root_index].tree->csg_widget);
		Create_CSG_Widget(displayed_trees[root_index].tree);
		if ( displayed_trees[root_index].tree->csg_left_child )
			XtVaSetValues(
				displayed_trees[root_index].tree->csg_left_child->csg_widget,
				XtNtreeParent, displayed_trees[root_index].tree->csg_widget,
				NULL);
		if ( displayed_trees[root_index].tree->csg_right_child )
			XtVaSetValues(
				displayed_trees[root_index].tree->csg_right_child->csg_widget,
				XtNtreeParent, displayed_trees[root_index].tree->csg_widget,
				NULL);

		/* Delete the parent. */
		XtDestroyWidget(target->csg_parent->csg_widget);
		target->csg_parent->csg_widget = NULL;
		CSG_Delete_Node(target->csg_parent);
		target->csg_parent = NULL;

		return target;
	}

	/* Reattach the sibling. */
	if ( target == target->csg_parent->csg_left_child )
	{
		target->csg_parent->csg_right_child->csg_parent =
												target->csg_parent->csg_parent;
		XtVaSetValues(target->csg_parent->csg_right_child->csg_widget,
					  XtNtreeParent, target->csg_parent->csg_parent->csg_widget,
					  NULL);
		if ( target->csg_parent->csg_parent->csg_left_child ==
			 target->csg_parent )
			target->csg_parent->csg_parent->csg_left_child =
					target->csg_parent->csg_right_child;
		else
			target->csg_parent->csg_parent->csg_right_child = 
					target->csg_parent->csg_right_child;
	}
	else
	{
		target->csg_parent->csg_left_child->csg_parent =
												target->csg_parent->csg_parent;
		XtVaSetValues(target->csg_parent->csg_left_child->csg_widget,
					  XtNtreeParent, target->csg_parent->csg_parent->csg_widget,
					  NULL);
		if ( target->csg_parent->csg_parent->csg_left_child ==
			 target->csg_parent )
			target->csg_parent->csg_parent->csg_left_child =
					target->csg_parent->csg_left_child;
		else
			target->csg_parent->csg_parent->csg_right_child = 
					target->csg_parent->csg_left_child;
	}

	CSG_Check_Tree_Ordering(target->csg_parent->csg_parent);

	/* Delete the parent. */
	CSG_Delete_Node(target->csg_parent);
	target->csg_parent = NULL;

	return target;
}



static void
CSG_Delete_Node(CSGNodePtr victim)
{
	/* It mostly freeing it. */
	if ( victim->csg_instance )
		Destroy_Instance(victim->csg_instance);
	if ( victim->csg_widget )
		XtDestroyWidget(victim->csg_widget);
	free(victim);
}


void
CSG_Delete_Tree(CSGNodePtr victim)
{
	if ( ! victim ) return;

	CSG_Delete_Tree(victim->csg_left_child);
	CSG_Delete_Tree(victim->csg_right_child);
	CSG_Delete_Node(victim);
}


void
CSG_Switch_Ref(FeatureSpecPtr spec, ObjectInstancePtr inst, void *ptr, int i)
{
	ObjectInstancePtr	new_ref;
	HashTable			table = (HashTable)ptr;

	if ( spec->spec_type != reference_spec ) return;

	if ( ( new_ref =
		   (ObjectInstancePtr)Hash_Get_Value(table, (long)(spec->spec_object)))
		   != (void*)-1 )
	{
		Edit_Remove_Obj_From_Dependencies(spec, inst, NULL, 0);
		spec->spec_object = new_ref;
		Add_Dependency(new_ref, inst);
	}
}


/*	void
**	CSG_Switch_Tree_References(CSGNodePtr tree, HashTable table)
**	Switches any reference point specifiers which reference objects stored
**	in the hash table to their new values. Effectively this keeps any
**	constraints within the tree.
*/
static void
CSG_Switch_Tree_References(CSGNodePtr tree, HashTable table)
{
	int	i;

	if ( ! tree ) return;

	if ( tree->csg_op == csg_leaf_op )
	{
		for ( i = 0 ; i < tree->csg_instance->o_origin_num ; i++ )
			Constraint_Manipulate_Specs(tree->csg_instance->o_origin_cons + i,
							tree->csg_instance, (void*)table, 0,
							CSG_Switch_Ref);
		for ( i = 0 ; i < tree->csg_instance->o_scale_num ; i++ )
			Constraint_Manipulate_Specs(tree->csg_instance->o_scale_cons + i,
							tree->csg_instance, (void*)table, 0,
							CSG_Switch_Ref);
		for ( i = 0 ; i < tree->csg_instance->o_rotate_num ; i++ )
			Constraint_Manipulate_Specs(tree->csg_instance->o_rotate_cons + i,
							tree->csg_instance, (void*)table, 0,
							CSG_Switch_Ref);
		Constraint_Manipulate_Specs(&(tree->csg_instance->o_major_align),
						tree->csg_instance, (void*)table, 0,
						CSG_Switch_Ref);
		Constraint_Manipulate_Specs(&(tree->csg_instance->o_minor_align),
						tree->csg_instance, (void*)table, 0,
						CSG_Switch_Ref);
	}
	else
	{
		CSG_Switch_Tree_References(tree->csg_left_child, table);
		CSG_Switch_Tree_References(tree->csg_right_child, table);
	}
}


/*	CSGNodePtr
**	CSG_Copy_Tree_Main(CSGNodePtr src, CSGNodePtr parent, HashTable hash_table)
**	This is the tree copy work function. It copies the tree, storing
**	corresponding src-copy instances in the hash table to allow for
**	constraint updating.
*/
static CSGNodePtr
CSG_Copy_Tree_Main(CSGNodePtr src, CSGNodePtr parent, HashTable hash_table)
{
	CSGNodePtr	new;

	if ( ! src ) return NULL;

	if ( src->csg_op == csg_leaf_op )
	{
		new = Create_CSG_Node(parent, Copy_Object_Instance(src->csg_instance),
							  csg_leaf_op);
		Create_CSG_Widget(new);
		Hash_Insert(hash_table, (long)(src->csg_instance),
					(void*)(new->csg_instance));
		return new;
	}

	new = Create_CSG_Node(parent, NULL, src->csg_op);
	Create_CSG_Widget(new);
	new->csg_left_child = CSG_Copy_Tree_Main(src->csg_left_child, new,
											 hash_table);
	new->csg_right_child = CSG_Copy_Tree_Main(src->csg_right_child, new,
											  hash_table);

	return new;
	
}


/*	CSGNodePtr
**	CSG_Copy_Tree(CSGNodePtr src, CSGNodePtr parent)
**	Copys a CSG tree. This includes copying all the leaf instances.
**	Returns the copy.
*/
CSGNodePtr
CSG_Copy_Tree(CSGNodePtr src, CSGNodePtr parent)
{
	CSGNodePtr	new;
	HashTable	copy_table = Hash_New_Table();

	if ( ! src ) return NULL;

	new = CSG_Copy_Tree_Main(src, parent, copy_table);

	CSG_Switch_Tree_References(new, copy_table);

	Hash_Free(copy_table);

	return new;
}


static void
CSG_Add_Bases_To_Root(CSGTreeRoot *root, BaseObjectPtr *bases, int num)
{
	int	i = root->num_orig_bases;
	int	j;

	if ( root->num_orig_bases )
		root->orig_bases = More(root->orig_bases, BaseObjectPtr,
								root->num_orig_bases + num);
	else
		root->orig_bases = New(BaseObjectPtr, num);
	root->num_orig_bases += num;

	for ( j = 0 ; i < root->num_orig_bases ; i++, j++ )
		root->orig_bases[i] = bases[j];
}


void
CSG_Add_Node(CSGNodePtr src, CSGNodePtr sibling, CSGOperation op,
			 BaseObjectPtr *bases, int num_bases)
{
	CSGNodePtr	new_node;
	CSGNodePtr	root_node;
	int			root_index;

	/* Find the root. */
	CSG_Node_Root(sibling, root_node);
	for ( root_index = 0  ;
		  displayed_trees[root_index].tree != root_node ;
		  root_index++ );

	/* We need a new node. */
	new_node = Create_CSG_Node(sibling->csg_parent, NULL, op);

	/* The new node's children are the src and sibling. */
	new_node->csg_left_child = sibling;
	new_node->csg_right_child = src;
	sibling->csg_parent = src->csg_parent = new_node;

	if ( ! ( new_node->csg_parent ) )
		displayed_trees[root_index].tree = new_node;
	else
	{
		if ( sibling == new_node->csg_parent->csg_left_child )
			new_node->csg_parent->csg_left_child = new_node;
		else
			new_node->csg_parent->csg_right_child = new_node;
	}
	Create_CSG_Widget(new_node);
	CSG_Set_Widget_Values(sibling);
	XtVaSetValues(sibling->csg_widget,
					XtNtreeParent, new_node->csg_widget, NULL);
	CSG_Set_Widget_Values(src);
	XtVaSetValues(src->csg_widget, XtNtreeParent, new_node->csg_widget, NULL);

	/* The src's instances must be added. */
	CSG_Add_Instances_To_Root(src, &(displayed_trees[root_index]));

	/* The base objects must be added. */
	if ( num_bases )
		CSG_Add_Bases_To_Root(&(displayed_trees[root_index]), bases, num_bases);

	if ( new_node->csg_parent )
		CSG_Check_Tree_Ordering(new_node->csg_parent);

	CSG_Reorder_Root_Widgets();
}


/*	void
**	CSG_Reorder_Children(CSGNodePtr node)
**	Reorders the children of node. This is most important for difference
**	operations.
*/
void
CSG_Reorder_Children(CSGNodePtr node)
{
	CSGNodePtr	temp_node;

	if ( ( ! node ) || ( node->csg_op == csg_leaf_op ) )	return;

	/* Basically just swap the two children and their widgets. */
	temp_node = node->csg_left_child;
	node->csg_left_child = node->csg_right_child;
	node->csg_right_child = temp_node;

	CSG_Reorder_Child_Widgets(node);
}


static void
CSG_Reorder_Child_Widgets(CSGNodePtr node)
{
	Widget		temp_widget;

	temp_widget = node->csg_left_child->csg_widget;
	node->csg_left_child->csg_widget = node->csg_right_child->csg_widget;
	node->csg_right_child->csg_widget = temp_widget;
	CSG_Set_Widget_Values(node->csg_left_child);
	CSG_Set_Widget_Values(node->csg_right_child);

	if ( node->csg_left_child->csg_left_child )
	{
		XtVaSetValues(node->csg_left_child->csg_left_child->csg_widget,
					  XtNtreeParent, node->csg_left_child->csg_widget,  NULL);
		XtVaSetValues(node->csg_left_child->csg_right_child->csg_widget,
					  XtNtreeParent, node->csg_left_child->csg_widget,  NULL);
	}
	if ( node->csg_right_child->csg_left_child )
	{
		XtVaSetValues(node->csg_right_child->csg_left_child->csg_widget,
					  XtNtreeParent, node->csg_right_child->csg_widget,  NULL);
		XtVaSetValues(node->csg_right_child->csg_right_child->csg_widget,
					  XtNtreeParent, node->csg_right_child->csg_widget,  NULL);
	}
}


void
CSG_Check_Tree_Ordering(CSGNodePtr node)
{
	Position	left_x, right_x;

	XtVaGetValues(node->csg_left_child->csg_widget, XtNx, &left_x, NULL);
	XtVaGetValues(node->csg_right_child->csg_widget, XtNx, &right_x, NULL);

	if ( left_x > right_x )
	{
		CSG_Reorder_Child_Widgets(node);
		XawTreeForceLayout(csg_tree_widget);
	}
}


CSGNodePtr
Find_Widget_In_Displayed(Widget w)
{
	CSGNodePtr	res;
	int	i;

	for ( i = 0 ; i < num_displayed_trees ; i++ )
	{
		if ( ( res = Find_Widget_In_Tree(w, displayed_trees[i].tree) ) )
			return res;
	}
	return NULL;
}



CSGNodePtr
Find_Widget_In_Tree(Widget w, CSGNodePtr tree)
{
	CSGNodePtr	result;

	if ( ! tree ) return FALSE;

	if ( tree->csg_widget == w )
		return tree;

	if ( ( result = Find_Widget_In_Tree(w, tree->csg_left_child) ) )
		return result;

	if ( ( result = Find_Widget_In_Tree(w, tree->csg_right_child) ) )
		return result;


	return NULL;
}


CSGNodePtr
Find_Instance_In_Tree(ObjectInstancePtr inst, CSGNodePtr tree)
{
	CSGNodePtr	result;

	if ( ! tree ) return FALSE;

	if ( tree->csg_instance == inst )
		return tree;

	if ( ( result = Find_Instance_In_Tree(inst, tree->csg_left_child) ) )
		return result;

	if ( ( result = Find_Instance_In_Tree(inst, tree->csg_right_child) ) )
		return result;

	return NULL;
}


CSGNodePtr
Find_Instance_In_Displayed(ObjectInstancePtr inst)
{
	CSGNodePtr	res;
	int	i;

	for ( i = 0 ; i < num_displayed_trees ; i++ )
	{
		if ( ( res = Find_Instance_In_Tree(inst, displayed_trees[i].tree) ) )
			return res;
	}
	return NULL;
}


void
CSG_Free_Widgets(CSGNodePtr node)
{
	if ( ! node ) return;

	XtDestroyWidget(node->csg_widget);
	node->csg_widget = NULL;
	CSG_Free_Widgets(node->csg_left_child);
	CSG_Free_Widgets(node->csg_right_child);
}
