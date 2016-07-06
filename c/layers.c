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
**	Sced: A Constraint Based Object Scene Editor
**
**	layers.c: Support for layers, which control object visibility.
*/

#include <sced.h>
#include <hash.h>
#include <instance_list.h>
#include <layers.h>
#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Toggle.h>

#define	NO_OP	0
#define ADD		1
#define MERGE	2

static HashTable	layer_hash;

static int		max_num_layers = 0;

static Widget	new_layer_shell = NULL;
static Widget	new_layer_dialog;
static Widget	layer_list_shell = NULL;
static Widget	layer_list_widget;
static Widget	layer_toggle_shell = NULL;
static Widget	layer_toggle_box;

static int		num_layers = 0;
static int		max_list_length = 0;
static char		**layer_name_list;
static int		*layer_list_map;
static Widget	*layer_toggles;

static WindowInfoPtr	current_window;
static int				operation;
static int				num_chosen;


static void
Layer_Check_Instance_Visibility(LayerPtr layer, ObjectInstancePtr inst)
{
	InstanceList victim;

	if ( ! layer->state )
	{
		inst->o_flags &= ( ObjAll ^ ( ObjVisible & ObjSelected ) );

		/* Look for it in the edit and selected lists. */
		if ( ( victim = Find_Object_In_Instances(inst,
										current_window->edit_instances) ) )
		Delete_Edit_Instance(current_window, victim);
		if ( ( victim = Find_Object_In_Instances(inst,
									current_window->selected_instances) ) )
		{
			if ( victim == current_window->selected_instances )
				current_window->selected_instances =
					current_window->selected_instances->next;
			Delete_Element(victim);
		}
	}
}

static void
Layer_Add_Selected_Instances(int layer_num)
{
	LayerPtr		layer = (LayerPtr)Hash_Get_Value(layer_hash, layer_num);
	InstanceList	elmt;

	for ( elmt = current_window->selected_instances ; elmt ; elmt = elmt->next )
	{
		Layer_Remove_Instance(NULL, elmt->the_instance->o_layer,
							  elmt->the_instance);
		elmt->the_instance->o_layer = layer_num;
		Layer_Add_Instance(layer, 0, elmt->the_instance);
		Layer_Check_Instance_Visibility(layer, elmt->the_instance);
	}

	Update_Projection_Extents(current_window->selected_instances);
	View_Update(current_window, current_window->selected_instances, CalcView);
}


static void
Layer_Remove(int victim)
{
	LayerPtr	layer = (LayerPtr)Hash_Delete(layer_hash, victim);
	int			index;
	Widget		temp;
	int			i;

	for ( index = 0 ; layer_list_map[index] != victim ; index++ );

	free(layer->instances);
	free(layer->name);
	free(layer);

	temp = layer_toggles[index];
	XtUnmanageChild(temp);

	for ( i = index + 1; i < num_layers ; i++ )
	{
		layer_name_list[i-1] = layer_name_list[i];
		layer_list_map[i-1] = layer_list_map[i];
		layer_toggles[i-1] = layer_toggles[i];
	}
	num_layers--;
	layer_toggles[num_layers] = temp;
	XawListChange(layer_list_widget, layer_name_list, num_layers, 0, TRUE);
}

static void
Layer_Merge(int src, int dest)
{
	LayerPtr	src_layer = (LayerPtr)Hash_Get_Value(layer_hash, (long)src);
	LayerPtr	dest_layer = (LayerPtr)Hash_Get_Value(layer_hash, (long)dest);
	int			temp;
	int			i;

	if ( src == 0 )
	{
		LayerPtr	temp;

		src = dest;
		dest = 0;
		temp = src_layer;
		src_layer = dest_layer;
		dest_layer = temp;
	}

	temp = src_layer->num_instances;
	for ( i = 0 ; i < temp ; i++ )
	{
		src_layer->instances[0]->o_layer = dest;
		Layer_Add_Instance(dest_layer, 0, src_layer->instances[0]);
		Layer_Check_Instance_Visibility(dest_layer, src_layer->instances[0]);
		Layer_Remove_Instance(src_layer, 0, src_layer->instances[0]);
	}

	Layer_Remove(src);

	/* To make sure of visibilities. */
	Update_Projection_Extents(current_window->selected_instances);
	View_Update(current_window, current_window->selected_instances, CalcView);
}


static void
Layer_List_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XawListReturnStruct	*info = (XawListReturnStruct*)ca;
	int			num;
	static int	first_chosen;

	num = layer_list_map[info->list_index];

	num_chosen++;

	switch ( operation )
	{
		case ADD:
			Layer_Add_Selected_Instances(num);
			break;
		case MERGE:
			if ( num_chosen == 2 )
			{
				if ( first_chosen == num )
				{
					num_chosen--;
					return;
				}
				Layer_Merge(first_chosen, num);
				break;
			}
			else
			{
				first_chosen = num;
				return;
			}
	}

	XtPopdown(layer_list_shell);
}


static void
Layer_List_Cancel(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(layer_list_shell);
}


static void
Layer_Set_Instance_Visibility(LayerPtr layer)
{
	int	i;

	for ( i = 0 ; i < layer->num_instances ; i++ )
		if ( layer->state )
			layer->instances[i]->o_flags |= ObjVisible;
		else
		{
			layer->instances[i]->o_flags &= (ObjAll ^ ObjVisible);
			Layer_Check_Instance_Visibility(layer, layer->instances[i]);
		}

	if ( layer->state )
	{
		View_Update(current_window, current_window->all_instances, CalcView);
		Update_Projection_Extents(current_window->all_instances);
	}
	else
		View_Update(current_window, current_window->all_instances, ViewNone);
}


static void
Layer_Toggle_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	LayerPtr	layer;
	int			layer_num;
	Boolean		toggle_state;
	int			i;

	/* Find which widget. */
	for ( i = 0 ; layer_toggles[i] != w ; i++ );

	/* Get the layer and new state. */
	layer_num = layer_list_map[i];
	layer = (LayerPtr)Hash_Get_Value(layer_hash, (long)layer_num);
	XtVaGetValues(layer_toggles[i], XtNstate, &toggle_state, NULL);

	layer->state = toggle_state;

	/* Set the obejct visibility. */
	Layer_Set_Instance_Visibility(layer);
}

static void
Layer_Toggle_Finish(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(layer_toggle_shell);
}


static void
Create_Layer_List()
{
	Widget	box;
	Arg		args[5];
	int		n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Layers");			n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	layer_list_shell = XtCreatePopupShell("layerListShell",
						transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	box = XtCreateManagedWidget("listBox", boxWidgetClass, layer_list_shell,
								args, n);

	n = 0;
	XtSetArg(args[n], XtNlist, layer_name_list);	n++;
	XtSetArg(args[n], XtNnumberStrings, 0);			n++;
	XtSetArg(args[n], XtNdefaultColumns, 1);		n++;
	XtSetArg(args[n], XtNforceColumns, TRUE);		n++;
	layer_list_widget = XtCreateManagedWidget("layerList", listWidgetClass,
											  box, args, n);
	XtAddCallback(layer_list_widget, XtNcallback, Layer_List_Callback, NULL);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Cancel");						n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	XtAddCallback(XtCreateManagedWidget("listCancel", commandWidgetClass,
										box, args, n),
				  XtNcallback, Layer_List_Cancel, NULL);

	XtRealizeWidget(layer_list_shell);
}


static void
Create_Layer_Toggle_Shell()
{
	Arg		args[5];
	int		n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Layers");			n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);	n++;
	layer_toggle_shell = XtCreatePopupShell("layerToggleShell",
						transientShellWidgetClass, main_window.shell, args, n);

	n = 0;
	layer_toggle_box = XtCreateManagedWidget("layerBox", boxWidgetClass,
											  layer_toggle_shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Finish");						n++;
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtSetArg(args[n], XtNshapeStyle, XmuShapeRoundedRectangle);	n++;
	XtSetArg(args[n], XtNcornerRoundPercent, 30);				n++;
	XtSetArg(args[n], XtNhighlightThickness, 2);				n++;
#endif
	XtAddCallback(XtCreateManagedWidget("toggleFinish", commandWidgetClass,
										layer_toggle_box, args, n),
				  XtNcallback, Layer_Toggle_Finish, NULL);

	XtRealizeWidget(layer_toggle_shell);
}


static void
Add_Layer_To_Dialogs(int num, char *name, Boolean state)
{
	Arg		args[5];
	int		n;
	char	widg_name[12];
	int		i;

	if ( ! layer_list_shell )
		Create_Layer_List();
	if ( ! layer_toggle_shell )
		Create_Layer_Toggle_Shell();

	if ( num_layers >= max_list_length )
	{
		if ( max_list_length )
		{
			max_list_length += 5;
			layer_name_list = More(layer_name_list, char*, max_list_length);
			layer_list_map = More(layer_list_map, int, max_list_length);
			layer_toggles = More(layer_toggles, Widget, max_list_length);
		}
		else
		{
			max_list_length += 5;
			layer_name_list = New(char*, max_list_length);
			layer_list_map = New(int, max_list_length);
			layer_toggles = New(Widget, max_list_length);
		}

		n = 0;
		XtSetArg(args[n], XtNborderWidth, 0);	n++;
		for ( i = max_list_length - 5 ; i < max_list_length ; i++ )
		{
			sprintf(widg_name, "toggle%d", i);
			layer_toggles[i] = XtCreateWidget(widg_name, toggleWidgetClass,
												layer_toggle_box, args, n);
			XtAddCallback(layer_toggles[i], XtNcallback, Layer_Toggle_Callback,
						  NULL);
		}
	}

	layer_name_list[num_layers] = name;
	layer_list_map[num_layers] = num;
	XtVaSetValues(layer_toggles[num_layers], XtNlabel, name,
											 XtNstate, state, NULL);
	XtManageChild(layer_toggles[num_layers]);
	num_layers++;
	XawListChange(layer_list_widget, layer_name_list, num_layers, 0, TRUE);
}

void
Add_New_Layer(int num, char *name, Boolean state)
{
	LayerPtr	new_layer = New(Layer, 1);

	if ( num >= max_num_layers )
		max_num_layers = num + 1;

	new_layer->num = num;
	new_layer->name = Strdup(name);
	new_layer->state = state;
	new_layer->instances = NULL;
	new_layer->num_instances = 0;
	new_layer->max_num = 0;

	Hash_Insert(layer_hash, (long)num, (void*)new_layer);

	Add_Layer_To_Dialogs(num, new_layer->name, state);
}

static void
New_Layer_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	XtPopdown(new_layer_shell);
}

static void
New_Layer_Dialog_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	InstanceList	elmt;
	char			*new_name;
	int				temp;

	new_name = XawDialogGetValueString(new_layer_dialog);

	XtPopdown(new_layer_shell);

	temp = max_num_layers;
	Add_New_Layer(max_num_layers, new_name, TRUE);

	for ( elmt = current_window->selected_instances ; elmt ; elmt = elmt->next )
	{
		Layer_Remove_Instance(NULL, elmt->the_instance->o_layer,
							  elmt->the_instance);
		elmt->the_instance->o_layer = temp;
		Layer_Add_Instance(NULL, temp, elmt->the_instance);
	}

	/* To make sure of visibilities. */
	Update_Projection_Extents(current_window->selected_instances);
	View_Update(current_window, current_window->selected_instances, CalcView);

}

void
New_Layer_Action_Function(Widget w, XEvent *e, String *s, Cardinal *n)
{
	New_Layer_Dialog_Callback(w, NULL, NULL);
}

static void
Create_New_Layer_Shell()
{
	Arg		args[5];
	int		n;

	new_layer_shell = XtCreatePopupShell("Zoom",
						transientShellWidgetClass, main_window.shell, NULL, 0);

	/* Create the dialog widget to go inside the shell. */
	n = 0;
	XtSetArg(args[n], XtNlabel, "New Layer:");	n++;
	XtSetArg(args[n], XtNvalue, "");			n++;
	new_layer_dialog = XtCreateManagedWidget("newLayerDialog",
						dialogWidgetClass, new_layer_shell, args, n);

	/* Add the button at the bottom of the dialog. */
	XawDialogAddButton(new_layer_dialog, "Create", New_Layer_Dialog_Callback,
					   NULL);
	XawDialogAddButton(new_layer_dialog, "Cancel", New_Layer_Cancel_Callback,
					   NULL);

	XtOverrideTranslations(XtNameToWidget(new_layer_dialog, "value"),
		XtParseTranslationTable(":<Key>Return: New_Layer_Action()"));

	XtVaSetValues(XtNameToWidget(new_layer_dialog, "label"),
				  XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(new_layer_dialog, "Create"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
	XtVaSetValues(XtNameToWidget(new_layer_dialog, "Cancel"),
				  XtNshapeStyle, XmuShapeRoundedRectangle,
				  XtNcornerRoundPercent, 30,
				  XtNhighlightThickness, 2,
				  NULL);
#endif

	XtRealizeWidget(new_layer_shell);
}

static void
New_Layer_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	char	text[10];

	current_window = (WindowInfoPtr)cl;

	if ( ! new_layer_shell )
		Create_New_Layer_Shell();

	sprintf(text, "Layer %d", max_num_layers);
	XtVaSetValues(new_layer_dialog, XtNvalue, text, NULL);

	SFpositionWidget(new_layer_shell);

	XtPopup(new_layer_shell, XtGrabExclusive);
}

static void
Add_Layer_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	current_window = (WindowInfoPtr)cl;

	SFpositionWidget(layer_list_shell);
	XtPopup(layer_list_shell, XtGrabExclusive);

	num_chosen = 0;
	operation = ADD;
}

static void
Merge_Layer_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	current_window = (WindowInfoPtr)cl;

	SFpositionWidget(layer_list_shell);
	XtPopup(layer_list_shell, XtGrabExclusive);
	num_chosen = 0;
	operation = MERGE;
}

static void
Visible_Layer_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	current_window = (WindowInfoPtr)cl;

	SFpositionWidget(layer_toggle_shell);
	XtPopup(layer_toggle_shell, XtGrabExclusive);
}


void
Layers_Init()
{
	layer_hash = Hash_New_Table();
	Add_New_Layer(0, "World", TRUE);
}

void
Layers_Create_Menu(WindowInfoPtr window)
{
	Widget	menu_widget;
	Widget	menu_children[4];

	int		count = 0;

	menu_widget = XtCreatePopupShell("LayerMenu", simpleMenuWidgetClass,
										window->shell, NULL, 0);

	menu_children[count] = XtCreateManagedWidget("New", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, New_Layer_Callback,
				  (XtPointer)window);
	count++;

	menu_children[count] = XtCreateManagedWidget("Add Objects",
								smeBSBObjectClass, menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Add_Layer_Callback,
				  (XtPointer)window);
	count++;

	menu_children[count] = XtCreateManagedWidget("Merge", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Merge_Layer_Callback,
				  (XtPointer)window);
	count++;

	menu_children[count] = XtCreateManagedWidget("Display", smeBSBObjectClass,
													menu_widget, NULL, 0);
	XtAddCallback(menu_children[count], XtNcallback, Visible_Layer_Callback,
				  (XtPointer)window);
	count++;
}

Boolean
Layer_Is_Visible(int index)
{
	LayerPtr	layer = (LayerPtr)Hash_Get_Value(layer_hash, (long)index);

	if ( layer == (void*)-1 )
		return TRUE;

	return layer->state;
}

void
Update_Visible_List(WindowInfoPtr window)
{
	InstanceList	elmt;
	InstanceList	temp;
	int				other_flags = ObjAll ^ ObjVisible;

	/* Go through the instance list, setting visibility flags. */
	for ( elmt = window->all_instances ; elmt ; elmt = elmt->next )
		if ( Layer_Is_Visible(elmt->the_instance->o_layer) )
			elmt->the_instance->o_flags =
				( ( elmt->the_instance->o_flags & other_flags ) | ObjVisible );
		else
			elmt->the_instance->o_flags &= other_flags;

	/* Need to update the selection list so that only visible objects appear. */
	for ( elmt = window->selected_instances ; elmt ; )
	{
		temp = elmt;
		elmt = elmt->next;

		if ( ! ( temp->the_instance->o_flags & ObjVisible ) )
		{
			if ( temp == window->selected_instances )
				window->selected_instances = elmt;
			Delete_Element(temp);
			temp->the_instance->o_flags &= ( other_flags ^ ObjSelected );
			free(temp);
		}
	}

	View_Update(window, window->all_instances, CalcView );
	Update_Projection_Extents(window->all_instances);
}

void
Save_Layers(FILE *outfile)
{
	LayerPtr	layer;

	fprintf(outfile, "Layer\n");
	layer = (LayerPtr)Hash_Traverse(layer_hash, TRUE);
	do
	{
		if ( layer->num != 0 )
			fprintf(outfile, "\"%s\" %d %d\n", layer->name, layer->num,
					layer->state ? 1 : 0);
		layer = (LayerPtr)Hash_Traverse(layer_hash, FALSE);
	}
	while ( layer );
}

int
Layer_Get_Num()
{
	return max_num_layers;
}

void
Reset_Layers()
{
	int	i;

	for ( i = num_layers - 1 ; i > 0 ; i-- )
		Layer_Remove(layer_list_map[i]);
}


void
Layer_Add_Instance(LayerPtr layer, int num, ObjectInstancePtr inst)
{
	if ( ! layer )
		layer = (LayerPtr)Hash_Get_Value(layer_hash, (long)num);

	if ( layer->num_instances >= layer->max_num )
	{
		if ( layer->max_num )
			layer->instances = More(layer->instances, ObjectInstancePtr,
									layer->max_num + 5);
		else
			layer->instances = New(ObjectInstancePtr, 5);
		layer->max_num += 5;
	}

	layer->instances[layer->num_instances++] = inst;
}

void
Layer_Remove_Instance(LayerPtr layer, int num, ObjectInstancePtr victim)
{
	int			loc, i;

	if ( ! layer )
		layer = (LayerPtr)Hash_Get_Value(layer_hash, (long)num);

	for ( loc = 0 ;
		  loc < layer->num_instances && layer->instances[loc] != victim ;
		  loc++ );

	if ( loc == layer->num_instances )
#ifdef LAYER_DEBUG
	{
		fprintf(stderr, "Deleting non-existent layer object\n");
		abort();
	}
#else
		return;
#endif

	for ( i = loc + 1 ; i < layer->num_instances ; i++ )
		layer->instances[i-1] = layer->instances[i];
	if ( layer->num_instances )
		layer->num_instances--;
#ifdef LAYER_DEBUG
	else
	{
		fprintf(stderr, "Deleting negative layer instance\n");
		abort();
	}
#endif
}

