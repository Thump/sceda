/* csg.c: Elk functions for csg related things.
*/

#include "elk_private.h"
#include <X11/Xaw/Tree.h>


int
elk_csg_print(Object w, Object port, int raw, int depth, int len)
{
	Printf(port, "#[csgnode 0x%x]\n", (CSGNodePtr)ELKCSGNODE(w)->csg_node);
	return 0;
}

int
elk_csg_equal(Object a, Object b)
{
	return (ELKCSGNODE(a)->csg_node == ELKCSGNODE(b)->csg_node);
}

int
elk_csg_equiv(Object a, Object b)
{
	return (ELKCSGNODE(a)->csg_node == ELKCSGNODE(b)->csg_node);
}

/*********************************************************************
 *
 * elk_csg_node() finds the csg node associated with a given instance.
 *
 * Parameter: obj - the instance to search for.
 *
 * Return: An object containing the csg node.
 *         Void if the node is not present.
 *
 *********************************************************************/
Object
elk_csg_node(Object obj)
{
	Object		res_obj;
	CSGNodePtr	result;

	/* Check that the window exists. */
	if ( ! csg_window.shell )
		return Void;

	Check_Type(obj, OBJECT3_TYPE);

	/* Look for a solution. */
	if ( ! ( result =
	  Find_Instance_In_Displayed((ObjectInstancePtr)ELKOBJECT3(obj)->object3)))
		return Void;

	res_obj = Alloc_Object(sizeof(Elkcsgnode), CSGNODE_TYPE, 0);
	ELKCSGNODE(res_obj)->csg_node = result;

	return res_obj;
}


static CSGTreeRoot*
elk_csg_get_root_node(CSGNodePtr node)
{
	int			i;

	for ( i = 0 ;
		  i < num_displayed_trees &&
		  displayed_trees[i].tree->csg_widget != node->csg_widget ;
		  i++ );

	if ( i == num_displayed_trees )
		return NULL;

	return displayed_trees + i;

}


Object
elk_csg_display(Object tree)
{
	CSGTreeRoot	*root;
	CSGNodePtr	node = ELKCSGNODE(tree)->csg_node;

	/* Check that the window exists. */
	if ( ! csg_window.shell )
		return Void;

	Check_Type(tree, CSGNODE_TYPE);

	if ( ! node )
		return Void;

	root = elk_csg_get_root_node(node);
	if ( ! root )
		return Void;

	if ( ! root->displayed )
	{
		CSG_Add_Instances_To_Displayed(root->instances);
		root->displayed = TRUE;
	}
	
	return tree;
}


Object
elk_csg_hide(Object tree)
{
	CSGTreeRoot	*root;
	CSGNodePtr	node = ELKCSGNODE(tree)->csg_node;
	int			i;

	/* Check that the window exists. */
	if ( ! csg_window.shell )
		return Void;

	Check_Type(tree, CSGNODE_TYPE);

	if ( ! node )
		return Void;

	for ( i = 0 ;
		  i < num_displayed_trees &&
		  displayed_trees[i].tree->csg_widget != node->csg_widget ;
		  i++ );
	if ( ! root )
		return Void;

	if ( root->displayed )
	{
		CSG_Remove_Instances_From_Displayed(root->instances);
		root->displayed = FALSE;
	}
	
	return tree;
}

Object
elk_csg_attach(Object left_obj, Object right_obj, Object op_obj)
{
	Object			res_obj;
	CSGTreeRoot		*root;
	CSGOperation	op;
	CSGNodePtr		left_child;
	CSGNodePtr		right_child;

	/* Check that the window exists. */
	if ( ! csg_window.shell )
		return Void;

	Check_Type(left_obj, CSGNODE_TYPE);
	Check_Type(right_obj, CSGNODE_TYPE);
	Check_Type(op_obj, T_Symbol);

	if ( EQ(op_obj, Sym_Union) )
		op = csg_union_op;
	else if ( EQ(op_obj, Sym_Intersection) )
		op = csg_intersection_op;
	else if ( EQ(op_obj, Sym_Difference) )
		op = csg_difference_op;
	else
		Primitive_Error("Invalid CSG Operation: ~s", op_obj);

	left_child = ELKCSGNODE(left_obj)->csg_node;
	right_child = ELKCSGNODE(right_obj)->csg_node;

	if ( ! left_child || ! right_child )
		return Void;

	/* Just check that the left child is a root node. */
	root = elk_csg_get_root_node(left_child);
	if ( ! root )
		return Void;

	/* Right is the root we actually want. */
	root = elk_csg_get_root_node(right_child);
	if ( ! root )
		return Void;

	Delete_Displayed_Tree(root->tree);

	/* Do the attach. */
	right_child->csg_parent = NULL;
	CSG_Add_Node(right_child, left_child, op, NULL, 0);

	XawTreeForceLayout(csg_tree_widget);

	changed_scene = TRUE;

	res_obj = Alloc_Object(sizeof(Elkcsgnode), CSGNODE_TYPE, 0);
	ELKCSGNODE(res_obj)->csg_node = left_child->csg_parent;

	return res_obj;
}

Object
elk_csg_complete(Object csgobj, Object labelobj)
{
	CSGNodePtr		tree;
	char			*label;
	BaseObjectPtr	new_base;

	Check_Type(csgobj, CSGNODE_TYPE);
	Check_Type(labelobj, T_String);
	tree = ELKCSGNODE(csgobj)->csg_node;
	label = STRING(labelobj)->data;

	if ( ! ( elk_csg_get_root_node ) )
		return Void;

	Delete_Displayed_Tree(tree);

	CSG_Free_Widgets(tree); 
	CSG_Update_Reference_Constraints(tree, tree);
	new_base = Add_CSG_Base_Object(tree, label, NULL, NULL);
	object_count[csg_obj]++;
	new_base->b_ref_num = 0;
	CSG_Add_Select_Option(new_base);

	return Make_String(new_base->b_label, strlen(new_base->b_label) + 1);
}

