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
**	csg_edit.c : CSG functions related to the list of objects displayed.
*/

#include <sced.h>
#include <csg.h>
#include <instance_list.h>
#include <layers.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Tree.h>


static void	Create_New_Tree(ObjectInstancePtr);

static void	CSG_Update_Selected_References(ObjectInstancePtr, InstanceList);

static InstanceList	Build_Instance_List_From_Tree(CSGNodePtr);

CSGTreeList	displayed_trees;
int			num_displayed_trees = 0;
int			max_num_displayed_trees = 0;


void
CSG_Window_Popup(Widget w, XtPointer cl, XtPointer ca)
{
	InstanceList		instances = *(InstanceList*)cl;
	InstanceList		elmt;
	InstanceList		victim;
	ObjectInstancePtr	obj;

	if ( ! csg_window.shell )
		Create_CSG_Display();

	XMapRaised(XtDisplay(csg_window.shell), XtWindow(csg_window.shell));

	/* Check for instances. */
	for ( elmt = instances ; elmt ; elmt = elmt->next )
	{
		/* Ignore light sources and planes and squares. */
		if ( Obj_Is_Light(elmt->the_instance) ||
			 elmt->the_instance->o_parent->b_class == plane_obj ||
			 elmt->the_instance->o_parent->b_class == square_obj )
		{
			/* Reset the highlight. */
			elmt->the_instance->o_flags &= ( ObjAll ^ ObjVisible );
			continue;
		}

		if ( Find_Object_In_Instances(elmt->the_instance,
									  csg_window.all_instances) )
			continue;

		if ( ! ( victim = Find_Object_In_Instances(elmt->the_instance,
												   main_window.all_instances)))
			continue;
		if ( main_window.all_instances == victim )
			main_window.all_instances = victim->next;
		Delete_Element(victim);

		obj = victim->the_instance;
		obj->o_flags &= ( ObjAll ^ ObjSelected );
		free(victim);

		/* Check for the victim in the edit lists. */
		if ( ( victim = Find_Object_In_Instances(obj,
											main_window.edit_instances) ) )
			Delete_Edit_Instance(&main_window, victim);

		/* Update dependencies. It's messy in this case. */
		CSG_Update_Selected_References(obj, instances);

		Create_New_Tree(obj);
	}

	Free_Selection_List(instances);
	*(InstanceList*)cl = NULL;

	View_Update(&main_window, main_window.all_instances, ViewNone);
}


void
New_CSG_Instance(ObjectInstancePtr inst)
{
	Create_New_Tree(inst);

	XawTreeForceLayout(csg_tree_widget);
}


static void
Create_New_Tree(ObjectInstancePtr instance)
{
	if ( num_displayed_trees >= max_num_displayed_trees )
	{
		if ( max_num_displayed_trees )
			displayed_trees = More(displayed_trees, CSGTreeRoot,
									max_num_displayed_trees + 5);
		else
			displayed_trees = New(CSGTreeRoot, 5);
		max_num_displayed_trees += 5;
	}

	displayed_trees[num_displayed_trees].tree =
		Create_CSG_Node(NULL, instance, csg_leaf_op);
	Create_CSG_Widget(displayed_trees[num_displayed_trees].tree);

	displayed_trees[num_displayed_trees].displayed = FALSE;

	displayed_trees[num_displayed_trees].instances = NULL;
	Insert_Element(&(displayed_trees[num_displayed_trees].instances), instance);

	displayed_trees[num_displayed_trees].orig_bases = NULL;
	displayed_trees[num_displayed_trees].num_orig_bases = 0;

	num_displayed_trees++;

	changed_scene = TRUE;
}


void
Add_Displayed_Tree(CSGNodePtr root, Boolean display, BaseObjectPtr orig_base)
{
	if ( num_displayed_trees >= max_num_displayed_trees )
	{
		if ( max_num_displayed_trees )
			displayed_trees = More(displayed_trees, CSGTreeRoot,
									max_num_displayed_trees + 5);
		else
			displayed_trees = New(CSGTreeRoot, 5);
		max_num_displayed_trees += 5;
	}

	displayed_trees[num_displayed_trees].tree = root;
	XtDestroyWidget(root->csg_widget);
	Create_CSG_Widget(root);
	if ( root->csg_left_child )
	{
		XtVaSetValues(root->csg_left_child->csg_widget,
					  XtNtreeParent, root->csg_widget, NULL);
		XtVaSetValues(root->csg_right_child->csg_widget,
					  XtNtreeParent, root->csg_widget, NULL);
	}

	displayed_trees[num_displayed_trees].displayed = display;
	displayed_trees[num_displayed_trees].instances =
										Build_Instance_List_From_Tree(root);
	if ( orig_base )
	{
		displayed_trees[num_displayed_trees].orig_bases = New(BaseObjectPtr, 1);
		displayed_trees[num_displayed_trees].orig_bases[0] = orig_base;
		displayed_trees[num_displayed_trees].num_orig_bases = 1;
	}
	else
	{
		displayed_trees[num_displayed_trees].orig_bases = NULL;
		displayed_trees[num_displayed_trees].num_orig_bases = 0;
	}

	if ( display )
		CSG_Add_Instances_To_Displayed(
			displayed_trees[num_displayed_trees].instances);

	num_displayed_trees++;

	CSG_Reorder_Root_Widgets();
}


CSGTreeRoot
Delete_Displayed_Tree(CSGNodePtr root)
{
	CSGTreeRoot	result;
	int	target_index;
	int	i;

	for ( target_index = 0 ;
		  displayed_trees[target_index].tree != root ;
		  target_index++ );

	result = displayed_trees[target_index];

	if ( displayed_trees[target_index].displayed )
		CSG_Remove_Instances_From_Displayed(
									displayed_trees[target_index].instances);
	Free_Selection_List(displayed_trees[target_index].instances);

	/* Shuffle the others down. */
	for ( i = target_index + 1 ; i < num_displayed_trees ; i++ )
	{
		displayed_trees[i-1] = displayed_trees[i];
	}

	num_displayed_trees--;

	return result;
}


static void
CSG_Update_Object_Spec(FeatureSpecPtr spec, ObjectInstancePtr target,
						void *ptr, int abs)
{
	InstanceList	list = (InstanceList)ptr;
	Vector			temp;

	/* Only check reference specs. */
	if ( spec->spec_type != reference_spec )	return;

	/* If the obj is not a light, plane or square, and in the list, ignore it.*/
	if ( ! spec->spec_object->o_parent ||
		 ( ! Obj_Is_Light(spec->spec_object) &&
		   spec->spec_object->o_parent->b_class != plane_obj &&
		   spec->spec_object->o_parent->b_class != square_obj &&
		   Find_Object_In_Instances(spec->spec_object, list) ) )
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



/*	void
**	CSG_Update_Selected_References(ObjectInstancePtr obj, InstanceList list)
**	Updates all the reference specs for constraints in obj so that
**	the only ones left appear in list.
*/
static void
CSG_Update_Selected_References(ObjectInstancePtr obj, InstanceList list)
{
	int	i;

	for ( i = 0 ; i < obj->o_origin_num ; i++ )
		Constraint_Manipulate_Specs(obj->o_origin_cons + i, obj, (void*)list, 1,
									CSG_Update_Object_Spec);
	for ( i = 0 ; i < obj->o_scale_num ; i++ )
		Constraint_Manipulate_Specs(obj->o_scale_cons + i, obj, (void*)list, 0,
									CSG_Update_Object_Spec);
	for ( i = 0 ; i < obj->o_rotate_num ; i++ )
		Constraint_Manipulate_Specs(obj->o_rotate_cons + i, obj, (void*)list, 0,
									CSG_Update_Object_Spec);
	Constraint_Manipulate_Specs(&(obj->o_major_align), obj, (void*)list, 0,
								CSG_Update_Object_Spec);
	Constraint_Manipulate_Specs(&(obj->o_minor_align), obj, (void*)list, 0,
								CSG_Update_Object_Spec);

	/* Also need to check that all dependents appear in the list. */
	for ( i = obj->o_num_depend - 1 ; i >= 0 ; i-- )
	{
		if ( Obj_Is_Light(obj->o_dependents[i].obj) ||
			 obj->o_dependents[i].obj->o_parent->b_class == plane_obj ||
			 obj->o_dependents[i].obj->o_parent->b_class == square_obj ||
			 ! Find_Object_In_Instances(obj->o_dependents[i].obj, list) )
			Constraint_Remove_References(obj->o_dependents[i].obj, obj);
	}
}


void
CSG_Add_Instances_To_Displayed(InstanceList new)
{
	InstanceList	new_list;

	new_list = Merge_Selection_Lists(csg_window.all_instances, new);
	Free_Selection_List(csg_window.all_instances);
	csg_window.all_instances = new_list;
	Update_Visible_List(&csg_window);
	Update_Projection_Extents(csg_window.all_instances);

}

void
CSG_Remove_Instances_From_Displayed(InstanceList old)
{
	InstanceList	new_list;
	InstanceList	temp_list;

	/* Need to undisplay it. */
	/* This means removing its instances from the window's lists. */

	for ( new_list = old ; new_list ; new_list = new_list->next )
	{
		/* Unset its selected status. */
		new_list->the_instance->o_flags &= ( ObjAll ^ ObjSelected );

		/* Take it off the edit list. */
		if (( temp_list = Find_Object_In_Instances(new_list->the_instance,
								csg_window.edit_instances)) )
			Delete_Edit_Instance(&csg_window, temp_list);
	}

	/* Take it off the selection list. */
	new_list = Remove_Selection_List(csg_window.all_instances, old);
	Free_Selection_List(csg_window.all_instances);
	csg_window.all_instances = new_list;

	/* Take it off the all list. */
	new_list = Remove_Selection_List(csg_window.selected_instances, old);
	Free_Selection_List(csg_window.selected_instances);
	csg_window.selected_instances = new_list;

	View_Update(&csg_window, csg_window.all_instances, ViewNone);
}



/*
**	CSG_Remove_Instances_From_Root(CSGNodePtr tree, CSGNodePtr root)
**	Removes those instances that occur in tree from root's list.
**	It DOES NOT remove them from any other list.
*/
void
CSG_Remove_Instances_From_Root(CSGNodePtr tree, CSGNodePtr root)
{
	InstanceList	old = Build_Instance_List_From_Tree(tree);
	InstanceList	temp;
	int				root_index;

	for ( root_index = 0 ;
		  displayed_trees[root_index].tree != root ;
		  root_index++ );

	if ( displayed_trees[root_index].displayed )
		CSG_Remove_Instances_From_Displayed(old);

	temp = Remove_Selection_List(displayed_trees[root_index].instances, old);
	Free_Selection_List(displayed_trees[root_index].instances);
	Free_Selection_List(old);
	displayed_trees[root_index].instances = temp;
}


/*
**	CSG_Add_Instances_To_Root(CSGNodePtr tree, CSGTreeList root)
**	Adds those instances that occur in tree from root's list.
**	If it is displayed, adds the instances to the relevant lists.
*/
void
CSG_Add_Instances_To_Root(CSGNodePtr tree, CSGTreeList root)
{
	InstanceList	new_insts = Build_Instance_List_From_Tree(tree);

	Append_Instance_List(&(root->instances), new_insts);

	if ( root->displayed )
		CSG_Add_Instances_To_Displayed(new_insts);
		
}


static InstanceList
Build_Instance_List_From_Tree(CSGNodePtr tree)
{
	InstanceList	left_list;
	InstanceList	right_list;
	InstanceList	result;

	if ( ! tree ) return NULL;

	if ( tree->csg_op == csg_leaf_op )
	{
		result = New(InstanceListElmt, 1);
		result->the_instance = tree->csg_instance;
		result->next = result->prev = NULL;

		return result;
	}

	left_list = Build_Instance_List_From_Tree(tree->csg_left_child);
	right_list = Build_Instance_List_From_Tree(tree->csg_right_child);

	Append_Instance_List(&left_list, right_list);

	return left_list;
}


/*	void
**	CSG_Reorder_Root_Widgets()
**	This procedure is needed to solve an annoying problem _ I have no
**	control over the order in which distinct trees appear in the tree widget,
**	particularly when modifying trees introduces new trees and removes
**	some old ones, and makes previously internal nodes root nodes.
**	Anyway, the solution is to (kind of) sort the displayed trees.
**	For each tree in turn, find the leftmost widget and do a swap if necessary.
*/
void
CSG_Reorder_Root_Widgets()
{
	Position	x, min_x;
	int			i, j, min_index;
	Arg			arg;
	Widget		swap;


	XtSetArg(arg, XtNx, &x);
	/* Match each displayed tree with the appropriate widget. */
	for ( i = 0 ; i < num_displayed_trees - 1 ; i++ )
	{
		XtGetValues(displayed_trees[i].tree->csg_widget, &arg, 1);
		min_x = x;
		min_index = i;
		for ( j = i ; j < num_displayed_trees ; j++ )
		{
			XtGetValues(displayed_trees[j].tree->csg_widget, &arg, 1);
			if ( x < min_x )
			{
				min_x = x;
				min_index = j;
			}
		}
		if ( min_index != i )	/* Swap widgets. */
		{
			swap = displayed_trees[min_index].tree->csg_widget;
			displayed_trees[min_index].tree->csg_widget =
											displayed_trees[i].tree->csg_widget;
			displayed_trees[i].tree->csg_widget = swap;

			CSG_Set_Widget_Values(displayed_trees[min_index].tree);
			CSG_Set_Widget_Values(displayed_trees[i].tree);

			if ( displayed_trees[i].tree->csg_left_child )
				XtVaSetValues(
					displayed_trees[i].tree->csg_left_child->csg_widget,
					XtNtreeParent, displayed_trees[i].tree->csg_widget, NULL);
			if ( displayed_trees[min_index].tree->csg_left_child )
				XtVaSetValues(
					displayed_trees[min_index].tree->csg_left_child->csg_widget,
					XtNtreeParent, displayed_trees[min_index].tree->csg_widget,
					NULL);
			if ( displayed_trees[i].tree->csg_right_child )
				XtVaSetValues(
					displayed_trees[i].tree->csg_right_child->csg_widget,
					XtNtreeParent, displayed_trees[i].tree->csg_widget, NULL);
			if ( displayed_trees[min_index].tree->csg_right_child )
				XtVaSetValues(
				displayed_trees[min_index].tree->csg_right_child->csg_widget,
				XtNtreeParent, displayed_trees[min_index].tree->csg_widget,
				NULL);
		}
	}
}


static void
CSG_Create_Tree_Widgets(CSGNodePtr tree)
{
	if ( ! tree ) return;

	Create_CSG_Widget(tree);
	if ( tree->csg_op != csg_leaf_op )
	{
		CSG_Create_Tree_Widgets(tree->csg_left_child);
		CSG_Create_Tree_Widgets(tree->csg_right_child);
	}
}


/*
**	CSG_Insert_Existing_Tree(CSGNodePtr tree, Boolean display)
**	Inserts a tree which already largely exists into the list of
**	editable trees. Used to modify base types.
*/
void
CSG_Insert_Existing_Tree(CSGNodePtr tree, Boolean display, BaseObjectPtr orig)
{
	CSG_Create_Tree_Widgets(tree);
	Add_Displayed_Tree(tree, display, orig);
}


/*	void
**	CSG_Rename_Instance(ObjectInstancePtr, char*)
**	Renames an instance from the CSG window.
*/
void
CSG_Rename_Instance(ObjectInstancePtr inst, char *new_name)
{
	CSGNodePtr	node;

	Rename_Instance(inst, new_name);

	node = Find_Instance_In_Displayed(inst);

	XtVaSetValues(node->csg_widget, XtNlabel, new_name, NULL);

	XawTreeForceLayout(csg_tree_widget);

	changed_scene = TRUE;
}

/*	void
**	CSG_Reset()
**	Resets (deletes all instances) the CSG window.
*/
void
CSG_Reset()
{
	CSGNodePtr	node;
	int	num_trees;
	int	i;

	num_trees = num_displayed_trees;
	for ( i = 0 ; i < num_trees ; i++ )
	{
		node = displayed_trees[0].tree;
		Delete_Displayed_Tree(node);
		CSG_Delete_Tree(node);
	}

	XawTreeForceLayout(csg_tree_widget);
	View_Update(&csg_window, csg_window.all_instances, ViewNone);
}


void
CSG_Delete_Original_Base(BaseObjectPtr base)
{
	int	i, j, k;

	for ( i = 0 ; i < num_displayed_trees ; i++ )
		for ( j = 0 ; j < displayed_trees[i].num_orig_bases ; j++ )
			if ( displayed_trees[i].orig_bases[j] == base )
				for ( k = j + 1 ; k < displayed_trees[i].num_orig_bases ; k++ )
					displayed_trees[i].orig_bases[k-1] =
						displayed_trees[i].orig_bases[k];
}
