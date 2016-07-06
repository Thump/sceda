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
**	load_internal.c : Code needed to load a scene description file.
**
**	External Functions:
**
**	void Load_Dialog_Func(Widget, XtPointer, XtPointer);
**	Puts up the load dialog box.
**
**	void Load_Internal_File(FILE*, Boolean)
**	Loads the contents of a file.
**
*/

#include <sced.h>
#include <base_objects.h>
#include <csg.h>
#include <csg_wire.h>
#include <hash.h>
#include <instance_list.h>
#include <layers.h>
#include <load.h>
#include <View.h>

#define Input_Error \
	{ \
		fprintf(stderr, "Malformed input file line %d\n", line_num); \
		exit(1); \
	}

#define Load_Float(f) \
	{ \
		if ((token = yylex()) == INT) \
			f = (double)lex_int; \
		else if (token == FLOAT) \
			f = lex_float; \
		else \
			Input_Error; \
	}

#define Load_Vector(v) \
	{ Load_Float((v).x); Load_Float((v).y); Load_Float((v).z); }

extern void		CSG_Generate_Full_Wireframe(BaseObjectPtr);
extern void		Radiance_Set_Extras(int, char*, char*, int, int, int, char*);

static int	Load_Layers();
static int	Load_Basetypes();
static int	Load_Instances();
static int	Load_Attributes(AttributePtr, AttributePtr);
static int	Load_Light_Info(LightInfoPtr);
static int	Load_Camera(Camera *);
static int	Load_Lights(Boolean);
static int	Load_Feature(FeaturePtr);
static int	Load_Constraints(FeaturePtr*, short*, Boolean **);
static int	Load_CSG_Trees();
static int	Load_Wireframe(WireframePtr*, Boolean*);
static int	Load_Wireframe_Attributes(AttributePtr*, int);

static ObjectInstancePtr	Load_Instance(char*);
static CSGNodePtr			Load_CSG_Tree(CSGNodePtr);

static void	Refresh_Instance_Pointers();

static HashTable	load_hash;
static InstanceList	new_instances = NULL;

static int			layer_offset;

LabelMapPtr map=NULL;
static Boolean newframe;


/*	void
**	Load_Internal_File(FILE *file, int merge)
**	Loads the information contained in file.
*/
void
Load_Internal_File(FILE *file, int merge)
{
	Camera	dummy_cam;
	int		token;

	load_hash = Hash_New_Table();

	if ( ( token = yylex() ) == VERS )
	{
		Load_Float(version);
		token = yylex();
	}

	debug(LOAD,fprintf(stderr,"beginning file load/merge...\n"));

	while (token != EOF_TOKEN)
	{
		switch (token)
		{
			case MAINVIEW:
				debug(LOAD,fprintf(stderr,"loading MAINVIEW\n"));

				if ( merge && ! newframe )
					token = Load_View(&(main_window.viewport), NULL, TRUE);
				else
				{
					token = Load_View(&(main_window.viewport), NULL, FALSE);
					if ( main_window.view_widget &&
						 main_window.viewport.scr_width )
						XtVaSetValues(main_window.view_widget,
							XtNwidth, main_window.viewport.scr_width,
							XtNdesiredWidth, main_window.viewport.scr_width,
							XtNheight, main_window.viewport.scr_height,
							XtNdesiredHeight, main_window.viewport.scr_height,
							NULL);
					if ( main_window.view_widget )
						XtVaSetValues(main_window.view_widget,
							XtNmagnification, main_window.viewport.magnify,
							NULL);
				}
				break;

			case CSGVIEW:
				debug(LOAD,fprintf(stderr,"loading CSGVIEW\n"));

				if ( merge )
					token = Load_View(&(csg_window.viewport), NULL, TRUE);
				else
				{
					token = Load_View(&(csg_window.viewport), NULL, FALSE);
					if ( csg_window.view_widget &&
						 csg_window.viewport.scr_width )
						XtVaSetValues(csg_window.view_widget,
							XtNwidth, csg_window.viewport.scr_width,
							XtNdesiredWidth, csg_window.viewport.scr_width,
							XtNheight, csg_window.viewport.scr_height,
							XtNdesiredHeight, csg_window.viewport.scr_height,
							NULL);
					if ( csg_window.view_widget )
						XtVaSetValues(csg_window.view_widget,
							XtNmagnification, csg_window.viewport.magnify,
							NULL);
				}
				break;

			case VIEWPORT:
				debug(LOAD,fprintf(stderr,"loading VIEWPORT\n"));

				if ( ( token = yylex() ) != STRING )
					break;
				if ( merge && ! newframe)
					token = Load_View(NULL, NULL, TRUE);
				else
					token = Load_View(NULL, lex_string, FALSE);
				break;

			case LAYER:
				debug(LOAD,fprintf(stderr,"loading LAYER\n"));

				if ( merge )
					layer_offset = Layer_Get_Num() - 1;
				else
					layer_offset = 0;
				token = Load_Layers();
				break;

			case CAMERA:
				debug(LOAD,fprintf(stderr,"loading CAMERA\n"));

				if ( merge )
					token = Load_Camera(&dummy_cam);
				else
					token = Load_Camera(&camera);
				break;

			case INCLUDES:
				debug(LOAD,fprintf(stderr,"loading INCLUDES\n"));

				token = Load_Includes(!merge);
				break;

			case DECLARE:
				debug(LOAD,fprintf(stderr,"loading DECLARE\n"));

				token = Load_Declarations(!merge);
				break;

			case BASEOBJECTS:
				debug(LOAD,fprintf(stderr,"loading BASEOBJECTS\n"));

				token = Load_Basetypes();
				break;

			case INSTANCES:
				debug(LOAD,fprintf(stderr,"loading INSTANCES\n"));

				token = Load_Instances();
				break;

			case CSG:
				debug(LOAD,fprintf(stderr,"loading CSG\n"));

				token = Load_CSG_Trees();
				break;

			case AMBIENT:
				debug(LOAD,fprintf(stderr,"loading AMBIENT\n"));

				token = Load_Lights(!merge);
				break;

			case NEWFRAME:
				debug(LOAD,fprintf(stderr,"loading NEWFRAME\n"));

				token = Reset_NewFrame(merge);
				break;

			default:
				if ( merge )
					fprintf(stderr, "Error token %d in file %s line %d\n",
							token, merge_filename, line_num);
				else
					fprintf(stderr, "Error token %d in file %s line %d\n",
							token, io_file_name, line_num);
				token = yylex();
		}
	}

	debug(LOAD,fprintf(stderr,"completed file load/merge...\n"));

	Update_to_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames));
	Update_from_KeyFrame(Nth_KeyFrame(1,key_frames),TRUE);

	Free_LabelMap(map);
	map=NULL;

	Refresh_Instance_Pointers();
	Hash_Free(load_hash);
	changed_scene = FALSE;
}


int
Reset_NewFrame(int merge)
{	int token;

	debug(FUNC_NAME,fprintf(stderr,"Reset_NewFrame()\n"));

	View_Update(&main_window,main_window.all_instances,CalcView);

	Update_to_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames));

	if ( (token=yylex()) != INT )
		if ( merge )
			fprintf(stderr, "Error token %d in file %s line %d\n",
				token, merge_filename, line_num);
		else
			fprintf(stderr, "Error token %d in file %s line %d\n",
				token, io_file_name, line_num);
	else
		/* This is actually where all the work is done: New_KeyFrame()
		** will set the current keyframe to be a keyframe with the
		** appropriate frame number.  If it had to create a new frame
		** to do this, it will return TRUE, otherwise FALSE.
		*/
		newframe=New_KeyFrame(main_window.fnumber=lex_int,FALSE);

	return(yylex());
}



/*	int
**	Load_View(Viewport *view, char *label)
**	Loads information into the viewport view. Also saves if required.
*/
int
Load_View(Viewport *view, char *label, Boolean merge)
{
	Viewport	result;
	int			token;
	Boolean		finished = FALSE;

	/* Initialize the result in case of underspecification. */
	Viewport_Init(&result);

	while ( ! finished )
	{
		switch ( token = yylex() )
		{
			case LOOKFROM:
				debug(VIEW,fprintf(stderr,"Load_View(): LOOKFROM\n"));
				Load_Vector(result.view_from);
				break;

			case LOOKAT:
				debug(VIEW,fprintf(stderr,"Load_View(): LOOKAT\n"));
				Load_Vector(result.view_at);
				break;

			case LOOKUP:
				debug(VIEW,fprintf(stderr,"Load_View(): LOOKUP\n"));
				Load_Vector(result.view_up);
				break;

			case VIEWDIST:
				debug(VIEW,fprintf(stderr,"Load_View(): VIEWDIST\n"));
				Load_Float(result.view_distance);
				break;

			case EYEDIST:
				debug(VIEW,fprintf(stderr,"Load_View(): EYEDIST\n"));
				Load_Float(result.eye_distance);
				break;

			case MAGNIFY:
				debug(VIEW,fprintf(stderr,"Load_View(): MAGNIFY\n"));
				if ( (token = yylex()) != INT )
				{
					Input_Error;
					finished = TRUE;
				}
				else
					result.magnify = lex_int;
				break;

			case MODE:
				debug(VIEW,fprintf(stderr,"Load_View(): MODE\n"));
				if ( ( token = yylex() ) != INT )
				{
					Input_Error;
					finished = TRUE;
				}
				else
					result.draw_mode = (int)lex_int;
				break;

			case SCREEN:
				debug(VIEW,fprintf(stderr,"Load_View(): SCREEN\n"));
				if ((token = yylex()) != INT )
				{
					Input_Error;
					return token;
				}
				result.scr_width = (Dimension)lex_int;

				if ( (token = yylex()) != INT )
				{
					Input_Error;
					return token;
				}
				result.scr_height = (Dimension)lex_int;
				break;

/* 'Kay, here it is: this routine is called to load views, both for the
** main_window, the csg_window as well as any saved views.  'Cept there
** are two general cases of instances when it's called: when merging
** files, and when loading files.  When merging files in vanilla sced 0.81,
** there's a bug: objects with dependencies on the axis have a pointer to
** the axis' hash value in the save file.  When the save file is loaded,
** this hash value is supposed to be mapped to the real object pointer.
** 'Cept on a merge, when view and label are both passed in as null, the
** hash value for the axis isn't mapped to the real main_window axis, but 
** instead is mapped to the csg_window axis.  A really obscure bug, but 
** because (in sceda 0.81) the csg_window axis doesn't have a name, 
** attempting to clone a scene that has a merged-in object with dependencies
** on the main axis would have an dependent object label or null.  This
** broke things.
**
** Anyhow, don't worry about it: I've added another parameter to the
** Load_View() function: the merge boolean value.
*/

			case AXES:
				debug(VIEW,fprintf(stderr,"Load_View(): AXES\n"));
				if ((token = yylex()) != INT )
					Input_Error
				else
				{
					if ( view == &(main_window.viewport) )
						Hash_Insert(load_hash, (long)lex_int,
									(void*)&(main_window.axes));
					else 
						Hash_Insert(load_hash, (long)lex_int,
										(void*)&(csg_window.axes));
				}
				break;

			default:
				finished = TRUE;
		}
	}

	result.is_default = FALSE;

	Build_Viewport_Transformation(&result);

	/* if ( view ) *view = result; */
	if ( ! merge && view ) *view = result;

	if ( label ) View_Save(&result, label);

	return token;
}

/*	int
**	Load_Layers()
**	Loads a set of layer number-name pairs, and stores each.
*/
static int
Load_Layers()
{
	char	*name;
	int		num;
	int		token;

	while ( ( token = yylex() ) == STRING )
	{
		name = lex_string;
		if ( ( token = yylex() ) != INT )
			Input_Error
		else
			num = lex_int;
		if ( ( token = yylex() ) != INT )
			Input_Error;

		Add_New_Layer(num + layer_offset, name, lex_int ? TRUE : FALSE);
	}

	return token;
}


/*	int
**	Load_Camera(Camera* cam)
**	Loads camera information into the global structure "camera".
*/
static int
Load_Camera(Camera *cam)
{
	int	token;
	Raytracer	target;
	Boolean		loc = FALSE;
	Boolean		at = FALSE;
	Boolean		up = FALSE;
	Boolean		hfov = FALSE;
	Boolean		vfov = FALSE;
	Boolean		eye = FALSE;
	Boolean		wup = FALSE;
	Boolean		wright = FALSE;
	Boolean		screen = FALSE;
	Boolean		finished = FALSE;
	int			zone_type, var, det, qual;
	char		*zone_str, *exp_str, *ind_str;

	switch ( token = yylex() )
	{
		case NONE:
			target = NoTarget;
			break;

		case RAYSHADE:
			target = Rayshade;
			break;

		case RENDERMAN:
			target = Renderman;
			break;

		case POVRAY:
			target = POVray;
			break;

		case GENRAY:
			target = Genray;
			break;

		case GENSCAN:
			target = Genscan;
			break;

		case RADIANCE:
			target = Radiance;
			if ( ( token = yylex() ) != INT )
				Input_Error;
			zone_type = (int)lex_int;
			if ( ( token = yylex() ) != STRING )
				Input_Error;
			zone_str = lex_string;
			if ( ( token = yylex() ) != STRING )
				Input_Error;
			exp_str = lex_string;
			if ( ( token = yylex() ) != INT )
				Input_Error;
			var = (int)lex_int;
			if ( ( token = yylex() ) != INT )
				Input_Error;
			det = (int)lex_int;
			if ( ( token = yylex() ) != INT )
				Input_Error;
			qual = (int)lex_int;
			if ( ( token = yylex() ) != STRING )
				Input_Error;
			ind_str = lex_string;
			Radiance_Set_Extras(zone_type, zone_str, exp_str, var, det, qual,
								ind_str);
			free(zone_str);
			free(exp_str);
			free(ind_str);
			break;

		default:
			fprintf(stderr, "Error: Camera type expected file %s line %d\n",
					io_file_name, line_num);
			return -1;
	}

	cam->type = target;
	cam->default_cam = FALSE;

	while ( ! finished )
	{
		switch ( token = yylex() )
		{
			case DEFAULT:
				cam->default_cam = TRUE;
				break;
			case LOOKFROM:
				Load_Vector(cam->location);
				loc = TRUE;
				break;
			case LOOKAT:
				Load_Vector(cam->look_at);
				at = TRUE;
				break;
			case LOOKUP:
				Load_Vector(cam->look_up);
				up = TRUE;
				break;
			case HFOV:
				Load_Float(cam->horiz_fov);
				hfov = TRUE;
				break;
			case VFOV:
				Load_Float(cam->vert_fov);
				vfov = TRUE;
				break;
			case EYEDIST:
				Load_Float(cam->eye_dist);
				eye = TRUE;
				break;
			case UP:
				Load_Float(cam->window_up);
				wup = TRUE;
				break;
			case RIGHT:
				Load_Float(cam->window_right);
				wright = TRUE;
				break;
			case SCREEN:
				if ( (token = yylex()) != INT )
				{
					finished = TRUE;
					break;
				}
				else cam->scr_width = (Dimension)lex_int;
				if ( (token = yylex()) != INT )
				{
					finished = TRUE;
					break;
				}
				else
				{
					cam->scr_height = (Dimension)lex_int;
					screen = TRUE;
				}
				break;

			default:
				finished = TRUE;
		}
	}

	return token;

}

/*	Loads POV include file specs.
*/
int
Load_Includes(Boolean clear)
{
	int	token;

	if ( clear )
		POV_Clear_Includes();
	while ( ( token = yylex() ) == STRING )
	{
		POV_Add_Include(lex_string);
		free(lex_string);
	}

	return token;
}

/*	Loads declarations for any target.
*/
int
Load_Declarations(Boolean clear)
{
	int	token;

	if ( clear )
		Clear_Declarations();
	if ( ( token = yylex() ) != STRING )
		Input_Error;
	Add_Declarations(lex_string);
	free(lex_string);

	return ( token = yylex() );
}


/*	int
**	Load_Basetypes()
**	Loads base type information.
*/
static int
Load_Basetypes()
{
	char			*label;
	Boolean			doing_csg;
	CSGNodePtr		tree = NULL;
	WireframePtr	wireframe;
	WireframePtr	simple_wireframe;
	Boolean			use_full;
	long			hash_index;
	BaseObjectPtr	res;
	AttributePtr	*attribs;
	int				num_attribs;
	int	ref;
	int	token;
	int	i;
	Boolean	dummy;

	for ( i = 0 ; ( token = yylex() ) == INT ; i++ )
		Hash_Insert(load_hash, (long)lex_int, (void*)(base_objects[i]));

	while ( token == STRING )
	{
		label = lex_string;

		if ( ( token = yylex()) == CSG )
		{
			doing_csg = TRUE;
			token = yylex();
		}
		else if ( token == WIREFRAME )
		{
			doing_csg = FALSE;
			token = yylex();
		}
		else
			doing_csg = TRUE;

		if ( token != INT )
			Input_Error;
		hash_index = lex_int;

		if ( (token = yylex()) != REFERENCE || (token = yylex()) != INT )
			Input_Error;
		ref = lex_int;

		if ( doing_csg )
			tree = Load_CSG_Tree(NULL);

		if ( ( token = yylex() ) == WIREFRAME )
			token = Load_Wireframe(&wireframe, &use_full);
		else if ( ! doing_csg && token == INT )
		{
			num_attribs = lex_int;
			attribs = New(AttributePtr, num_attribs);
			token = Load_Wireframe_Attributes(attribs, num_attribs);
			if ( token != WIREFRAME )
				Input_Error;
			token = Load_Wireframe(&wireframe, &use_full);
			wireframe->num_attribs = num_attribs;
			wireframe->attribs = attribs;
		}
		else
			Input_Error;

		if ( doing_csg )
		{
			if ( token == WIREFRAME )
			{
				token = Load_Wireframe(&simple_wireframe, &dummy);
			}
			else
				simple_wireframe = NULL;

			res = Add_CSG_Base_Object(tree, label, wireframe, simple_wireframe);

			res->b_ref_num = ref;

			Hash_Insert(load_hash, hash_index, (void*)res);

			if ( use_full )
				CSG_Generate_Full_Wireframe(res);

			CSG_Add_Select_Option(res);
		}
		else
		{
			res = Add_Wireframe_Base_Object(label, wireframe);
			res->b_ref_num = ref;
			Hash_Insert(load_hash, hash_index, (void*)res);
			Wireframe_Add_Select_Option(res);
		}

		free(label);
	}
	
	return token;
}


/*	int
**	Load_Instances()
**	Loads instances.
**
**  I added a chunk of code here to ensure uniqueness of object
** names when scenes are merged.
*/
static int
Load_Instances()
{
	char				*label,*tmp,*new;
	int					token;
	ObjectInstancePtr	obj;

	Boolean				need_remap=FALSE;

	token = yylex();
	while ( token == STRING)
	{
		label = lex_string;

		debug(LOAD,fprintf(stderr,"\tloading %s\n",label));

		if ((tmp=Find_LabelMap(map,label))!=NULL)
		{	debug(LOAD,
				fprintf(stderr,"\told map from %s to %s\n",label,tmp));
			free(label);
			label=(String)EMalloc(strlen(tmp)+2);
			strcpy(label,tmp);

		}
		else
			need_remap=ObjectLabel_Exists(label,main_window.kfnumber);
			
		obj = Load_Instance(label);
		free(label);

		if ( obj )
		{
			Insert_Element(&(main_window.all_instances), obj);

			/* Set visibility. */
			if ( Layer_Is_Visible(obj->o_layer) )
				obj->o_flags |= ObjVisible;

			Insert_Element(&(new_instances), obj);
		}

		token = yylex();

		if (need_remap)
		{	do
			{	new=(String)EMalloc(strlen(obj->o_label)+10);
				sprintf(new,"%s_%d",obj->o_label,
					object_count[obj->o_parent->b_class]);
			} 
			while(ObjectLabel_Exists(new,main_window.kfnumber));
	
			debug(LOAD,
				fprintf(stderr,"\tnew map from %s to %s\n",obj->o_label,new));
	
			map=Add_LabelMap(map,obj->o_label,new);
	
			free(obj->o_label);
			obj->o_label=new;
			new=NULL;
		}

	}

	if ( main_window.view_widget )
	{	
		View_Update(&main_window, new_instances, CalcView);
		Update_Projection_Extents(main_window.all_instances);
	}

	return token;
}



/*	ObjectInstancePtr
**	Load_Instance()
**	Loads a single instance. Returns NULL on error.
*/
static ObjectInstancePtr
Load_Instance(char *label)
{
	int					token;
	void*				base_index;
	BaseObjectPtr		base;
	ObjectInstancePtr	obj;
	long				hash_index;
	Transformation		trans;
	Attributes			attribs = default_attributes;
	int					i;

#define Instance_Error \
	{ \
		fprintf(stderr, "Malformed instance line %d\n", line_num); \
		fprintf(stderr, "Last token was: "); \
		Print_Token(token); \
		Print_Token_Value(token,lex_int,lex_float); \
		return NULL; \
	}

#define Instance_Float(f) \
	{ \
		if ((token = yylex()) == INT) \
			f = (double)lex_int; \
		else if (token == FLOAT) \
			f = lex_float; \
		else \
			Instance_Error \
	}

#define Instance_Vector(v) \
	{ Instance_Float((v).x); Instance_Float((v).y); Instance_Float((v).z); }

#define Load_Transformation(t) \
	{ Instance_Vector((t).matrix.x); Instance_Vector((t).matrix.y); \
	  Instance_Vector((t).matrix.z); Instance_Vector((t).displacement); }

#define Instance_Quat(t) \
	{ Instance_Float((t).real_part); Instance_Vector((t).vect_part); }

	if ((token = yylex()) != INT )
		Instance_Error;
	hash_index = lex_int;

	if ((token = yylex()) != INT)
		Instance_Error;
	base_index = (void*)lex_int;

	if ((base_index = Hash_Get_Value(load_hash, (long)base_index)) ==(void*)-1 )
	{
		fprintf(stderr, "Couldn't find base object %ld line %d\n",
				(long)base_index, line_num);
		return NULL;
	}
	base = (BaseObjectPtr)base_index;

	if ((obj = Create_Instance(base, label)) == NULL)
	{
		fprintf(stderr,"Couldn't create instance %s line %d\n",
				label, line_num);
		return NULL;
	}

	Hash_Insert(load_hash, hash_index, (void*)obj);

	if ( ( token = yylex() ) == DENSE )
	{
		if ( ( token = yylex() ) != INT )
			Instance_Error;
		Object_Change_Wire_Level(obj, (int)lex_int);
		token = yylex();
	}

	if ( token != TRANSFORMATION)
		Instance_Error;
	Load_Transformation(trans);
	Transform_Instance(obj, &trans, TRUE);

	if ( (token = yylex()) != ATTRIBUTES)
		Instance_Error;
	if ( base->b_class == light_obj ||
		 base->b_class == spotlight_obj ||
		 base->b_class == arealight_obj )
		token = Load_Light_Info(((LightInfoPtr)obj->o_attribs));
	else
	{
		token = Load_Attributes(&attribs, (AttributePtr)obj->o_attribs);
		Modify_Instance_Attributes(obj, &attribs, ModSimple | ModExtend);
	}

	/* This should allow us to read both animated scenes, and normal.
	** If we find an Ani_Rotation in the file, assume it's an animate
	** scene file, and read the quaternion.  Then expect to see an
	** Ani_Scale: if we don't, bomb out with an error.  If we do find
	** Ani_Scale, read it and then pop the next token for the LAYER
	** keyword test.  If we didn't even find Ani_Rotation, keep the
	** popped token and go straight to the LAYER test.
	*/
	if ( token == ANI_ROT)
	{	Instance_Quat(obj->o_rot);

		if ( (token = yylex()) == ANI_SCALE)
		{	Instance_Vector(obj->o_scale); }
		else
		{	Instance_Error; }
		token=yylex();
	}

	if ( token != LAYER || (token = yylex()) != INT )
		Instance_Error
	else if ( lex_int )
	{
		Layer_Remove_Instance(NULL, obj->o_layer, obj);
		obj->o_layer = lex_int + layer_offset;
		Layer_Add_Instance(NULL, obj->o_layer, obj);
	}

	if ( (token = yylex()) != AXES )
		Instance_Error;
	Instance_Vector(obj->o_axes.x);
	Instance_Vector(obj->o_axes.y);
	Instance_Vector(obj->o_axes.z);
	if ( (token = yylex()) != ORIGIN )
		Instance_Error;
	Instance_Vector(obj->o_origin);
	if ( (token = yylex()) != REFERENCE )
		Instance_Error;
	Instance_Vector(obj->o_reference);

			
	if ((token = yylex()) != CONSTRAINTS)
		Instance_Error;
	if ( obj->o_scale_cons )
	{
		free(obj->o_scale_cons);
		obj->o_scale_num = 0;
	}
	free(obj->o_origin_active);
	free(obj->o_scale_active);
	free(obj->o_rotate_active);
	Load_Constraints(&(obj->o_origin_cons), &(obj->o_origin_num),
					&(obj->o_origin_active));
	Load_Constraints(&(obj->o_scale_cons), &(obj->o_scale_num),
					&(obj->o_scale_active));
	Load_Constraints(&(obj->o_rotate_cons), &(obj->o_rotate_num),
					&(obj->o_rotate_active));


	if ( (token = yylex()) == MAJOR )
	{
		Load_Feature(&(obj->o_major_align));
		token = yylex();
	}
	if ( token == MINOR )
	{
		Load_Feature(&(obj->o_minor_align));
		token = yylex();
	}

	if ( token != DEPENDENTS || (token = yylex()) != INT )
		Instance_Error;
	obj->o_num_depend = lex_int;
	if ( lex_int )
		obj->o_dependents = New(Dependent, lex_int);
	else
		obj->o_dependents = NULL;
	for ( i = 0 ; i < obj->o_num_depend ; i++ )
	{
		if ( (token = yylex()) != INT )
			Instance_Error;
		obj->o_dependents[i].obj = (ObjectInstancePtr)lex_int;
		if ( (token = yylex()) != INT )
			Instance_Error;
		obj->o_dependents[i].count = (char)lex_int;
	}

	return obj;
}


static int
Load_Light_Info(LightInfoPtr l)
{
	/* This gets very messy, because we need to be able to read both old
	** and new style formats. Old style used normal instance attributes,
	** whereas new style uses just the numbers.
	*/

	int			token;
	double		num_1;
	Attributes	dummy;
	double		dummy_fl;

	/* Read the first number. Either "defined" or an intensity spec. */
	Load_Float(num_1);

	if ( ( token = yylex() ) == INT || token == FLOAT )
	{
		l->red = num_1;
		if ( token == FLOAT )
			l->green = lex_float;
		else
			l->green = (double)lex_int;
		Load_Float(l->blue);

		Load_Float(l->val1);
		Load_Float(l->val2);
		if ((token = yylex()) != INT)
			Input_Error;
		l->flag = lex_int ? TRUE : FALSE;

		token = yylex();
	}
	else if ( token == COLOUR )
	{
		if ((token = yylex()) != INT)
			Input_Error;
		l->red = (double)lex_int / (double)MAX_UNSIGNED_SHORT;
		if ((token = yylex()) != INT)
			Input_Error;
		l->green = (double)lex_int / (double)MAX_UNSIGNED_SHORT;
		if ((token = yylex()) != INT)
			Input_Error;
		l->blue = (double)lex_int / (double)MAX_UNSIGNED_SHORT;

		/* Clean up any other stuff. */
		token = Load_Attributes(&dummy, &dummy);
	}
	else
	{
		if ( token == SPECULAR )
			Load_Float(dummy_fl);
		Load_Float(dummy_fl);
		token = Load_Attributes(&dummy, &dummy);
	}


	return token;
}


/*	int
**	Load_Attributes(AttributePtr a, AttributePtr target)
**	Loads what attributes it can find into a.  Will always read one past,
**	so returns the last token read.
**	Sets flags to indicate what was read.
*/
static int
Load_Attributes(AttributePtr a, AttributePtr target)
{
	int	token;
	int	first = TRUE;
	int	second = FALSE;

	/* Force explicit definition of attributes. */
	a->defined = FALSE;

	while (TRUE)
	{
		switch (token = yylex())
		{
			case INT:
				if ( first )
				{
					Hash_Insert(load_hash, (long)lex_int, (void*)target);
					second = TRUE;
				}
				else if ( second )
					a->defined = ( lex_int ? TRUE : FALSE );
				else
					return token;
				break;

			case COLOUR:
				if ((token = yylex()) != INT)
					Input_Error;
				a->colour.red = lex_int;
				if ((token = yylex()) != INT)
					Input_Error;
				a->colour.green = lex_int;
				if ((token = yylex()) != INT)
					Input_Error;
				a->colour.blue = lex_int;
				break;

			case DIFFUSE:
				Load_Float(a->diff_coef);
				break;

			case SPECULAR:
				Load_Float(a->spec_coef);
				Load_Float(a->spec_power);
				break;

			case TRANSPARENCY:
				Load_Float(a->transparency);
				break;

			case REFLECT:
				Load_Float(a->reflect_coef);
				break;

			case REFRACT:
				Load_Float(a->refract_index);
				break;

			case EXTEND:
				if ((token = yylex()) != INT)
					Input_Error;
				a->use_extension = lex_int ? TRUE : FALSE;
				if ( ( token = yylex() ) != STRING )
					Input_Error;
				a->extension = lex_string;
				break;

			case TRANSFORMATION:
				if ((token = yylex()) != INT)
					Input_Error;
				a->use_obj_trans = lex_int ? TRUE : FALSE;
				break;

			case OPEN:
				if ((token = yylex()) != INT)
					Input_Error;
				a->open = lex_int ? TRUE : FALSE;
				break;
				
			default:
				return token;
		}

		first = FALSE;
	}

	/* To keep the compiler happy. */
	return token;
}


/*	int
**	Load_Lights(Boolean do_ambient)
**	Loads the lights.
*/
static int
Load_Lights(Boolean do_ambient)
{
	int		token;

	if ((token = yylex()) != INT)
		Input_Error;
	if ( do_ambient )
		ambient_light.red = lex_int;
	if ((token = yylex()) != INT)
		Input_Error;
	if ( do_ambient )
		ambient_light.green = lex_int;
	if ((token = yylex()) != INT)
		Input_Error;
	if ( do_ambient )
		ambient_light.blue = lex_int;

	return yylex();
}


/*	int
**	Load_Feature(FeaturePtr feat)
**	Loads a single constraint feature structure.
*/
static int
Load_Feature(FeaturePtr feat)
{
	int	token;

#define Load_Specs(s) \
	if ( (token = yylex()) != INT ) Input_Error; \
	(s).spec_type = (FeatureSpecType)lex_int; \
	if ( (s).spec_type == reference_spec ) \
	{ \
		if ( (token = yylex()) != INT ) Input_Error; \
		(s).spec_object = (ObjectInstancePtr)lex_int; \
	} \
	else \
		(s).spec_object = NULL; \
	Load_Vector((s).spec_vector)
		
	token = yylex();
	if ( token == AXES )
	{
		if ( ( token = yylex() ) == PLANE )
			feat->f_spec_type = axis_plane_feature;
		else if ( token == LINE )
			feat->f_spec_type = axis_feature;
		else
			Input_Error;
		feat->f_type = line_feature;
	}
	else if ( token == MID )
	{
		if ( (token = yylex()) == PLANE )
		{
			feat->f_spec_type = midplane_feature;
			feat->f_type = plane_feature;
		}
		else if ( token == POINT )
		{
			feat->f_spec_type = midpoint_feature;
			feat->f_type = point_feature;
		}
		else
			Input_Error
	}
	else if ( token == PLANE )
	{
		feat->f_spec_type =
		feat->f_type = plane_feature;
	}
	else if ( token == LINE )
	{
		feat->f_type =
		feat->f_spec_type = line_feature;
	}
	else if ( token == POINT )
	{
		feat->f_type =
		feat->f_spec_type = point_feature;
	}
	else if ( token == REFERENCE )
	{
		if ( (token = yylex()) == LINE )
		{
			feat->f_spec_type = ref_line_feature;
			feat->f_type = line_feature;
		}
		else if ( token == PLANE )
		{
			feat->f_spec_type = ref_plane_feature;
			feat->f_type = plane_feature;
		}
		else
			Input_Error;
	}
	else if ( token == ORIGIN )
	{
		if ( (token = yylex()) == LINE )
		{
			feat->f_spec_type = orig_line_feature;
			feat->f_type = line_feature;
		}
		else if ( token == PLANE )
		{
			feat->f_spec_type = orig_plane_feature;
			feat->f_type = plane_feature;
		}
		else
			Input_Error;
	}
	else
		Input_Error;

	if ( (token = yylex()) != STRING )
		Input_Error
	feat->f_label = lex_string;

	if ( feat->f_type != point_feature )
		Load_Vector(feat->f_vector);
	Load_Vector(feat->f_point);

	if ( feat->f_type == plane_feature )
		feat->f_value = VDot(feat->f_vector, feat->f_point);

	switch ( feat->f_spec_type )
	{
		case plane_feature:
		case axis_plane_feature:
		case orig_plane_feature:
		case ref_plane_feature:
			Load_Specs(feat->f_specs[2])
		case line_feature:
		case axis_feature:
		case midplane_feature:
		case midpoint_feature:
		case ref_line_feature:
		case orig_line_feature:
			Load_Specs(feat->f_specs[1])
		case point_feature:
			Load_Specs(feat->f_specs[0])
		default:;
	}

	return token;
}


/*	int
**	Load_Constraints(FeaturePtr*, short*, Boolean **)
**	Loads a set of Constraints and activity.
*/
static int
Load_Constraints(FeaturePtr *ret_cons, short *ret_num, Boolean **ret_active)
{
	FeatureData	current;
	int			token;
	int			i;


	if ( (token = yylex()) != INT )
		Input_Error;
	*ret_num = lex_int;
	if ( *ret_num )
		*ret_cons = New(FeatureData, *ret_num);
	else
		*ret_cons = NULL;
	for ( i = 0 ; i < *ret_num ; i++ )
	{
		Load_Feature(&current);
		(*ret_cons)[i] = current;
	}

	*ret_active = New(Boolean, *ret_num + 3);

	if ( (token = yylex()) != ACTIVE )
		Input_Error;
	for ( i = 0 ; i < *ret_num + 3 ; i++ )
	{
		if ((token = yylex()) != INT)
			Input_Error;
		(*ret_active)[i] = ( lex_int == 0 ? FALSE : TRUE );
	}

	return 1;
}


/*	int
**	Load_Wireframe(WireframePtr*, Boolean *full)
**	Loads a wireframe and returns the next token.
**	It exits on error, because the base types must be entered properly.
*/
static int
Load_Wireframe(WireframePtr *wire, Boolean *full)
{
	int	token;
	WireframePtr	res;
	int	i, j;

	res = New(Wireframe, 1);

#define Wireframe_Error \
	{ \
		fprintf(stderr, "Malformed wireframe line %d\n", line_num); \
		exit(1); \
	}

#define Wireframe_Float(f) \
	{ \
		if ((token = yylex()) == INT) \
			f = (double)lex_int; \
		else if (token == FLOAT) \
			f = lex_float; \
		else \
			Wireframe_Error \
	}

#define Wireframe_Vector(v) \
	{ Wireframe_Float((v).x); Wireframe_Float((v).y); Wireframe_Float((v).z); }


	if ( ( token = yylex() ) == FULL )
	{
		*full = TRUE;
		token = yylex();
	}
	else
		*full = FALSE;

	if ( token != INT )
		Wireframe_Error
	res->num_vertices = lex_int;
	res->vertices = New(Vector, res->num_vertices);
	for ( i = 0 ; i < res->num_vertices ; i++ )
		Wireframe_Vector(res->vertices[i])


	if ( ( token = yylex() ) != INT )
		Wireframe_Error
	res->num_faces = lex_int;
	res->faces = New(Face, res->num_faces);
	for ( i = 0 ; i < res->num_faces ; i++ )
	{
		if ( ( token = yylex() ) != INT )
			Wireframe_Error
		res->faces[i].num_vertices = lex_int;
		res->faces[i].vertices = New(int, res->faces[i].num_vertices);
		for ( j = 0 ; j < res->faces[i].num_vertices ; j++ )
		{
			if ( ( token = yylex() ) != INT )
				Wireframe_Error
			res->faces[i].vertices[j] = lex_int;
		}
		
		if ((token = yylex()) == INT)
		{
			if ( ( res->faces[i].face_attribs =
					Hash_Get_Value(load_hash, lex_int) ) == (void*)-1 )
				res->faces[i].face_attribs = NULL;
			Wireframe_Vector(res->faces[i].normal);
		}
		else
		{
			res->faces[i].face_attribs = NULL;
			if ( token == FLOAT )
				res->faces[i].normal.x = lex_float;
			else
				Wireframe_Error
			Wireframe_Float(res->faces[i].normal.y);
			Wireframe_Float(res->faces[i].normal.z);
		}
	}

	res->num_attribs = 0;
	res->attribs = NULL;

	if ( ( token = yylex() ) == NORMAL )
	{
		res->vertex_normals = New(Vector, res->num_vertices);
		for ( i = 0 ; i < res->num_vertices ; i++ )
			Wireframe_Vector(res->vertex_normals[i])
		token = yylex();
	}
	else
		res->vertex_normals = NULL;

	*wire = res;
	return token;

#undef Wireframe_Error
#undef Wireframe_Float
#undef Wireframe_Vector

}


static int
Load_Wireframe_Attributes(AttributePtr *attribs, int num)
{
	int	token;
	int	i;

	token = yylex();
	for ( i = 0 ; i < num ; i++ )
	{
		attribs[i] = New(Attributes, 1);
		*(attribs[i]) = default_attributes;
		if ( token != ATTRIBUTES )
			Input_Error;
		token = Load_Attributes(attribs[i], attribs[i]);
	}

	return token;
}



/*	int
**	Load_CSG_Trees()
**	Loads a sequence of CSG trees and sets them up in the CSG window.
*/
static int
Load_CSG_Trees()
{
	CSGNodePtr	new_tree;
	int			token;
	int			num_trees;
	int			i;

	if ( ( token = yylex() ) != INT )
		Input_Error;

	num_trees = lex_int;

	if ( ! csg_window.shell )
		Create_CSG_Display();

	for ( i = 0 ; i < num_trees ; i++ )
	{
		new_tree = Load_CSG_Tree(NULL);

		if ( ( token = yylex() ) != INT )
			Input_Error;

		CSG_Insert_Existing_Tree(new_tree, lex_int ? TRUE : FALSE, NULL);
	}

	return yylex();
}


/*	CSGNodePtr
**	Loads a CSG tree spec. If it errors, it exits.
*/
static CSGNodePtr
Load_CSG_Tree(CSGNodePtr parent)
{
	char		*label;
	int			token;
	CSGNodePtr	res = New(CSGNode, 1);

	res->csg_parent = parent;
	res->csg_widget = NULL;

	if ( ( token = yylex() ) == STRING )
	{
		label = lex_string;
		res->csg_op = csg_leaf_op;
		res->csg_instance = Load_Instance(label);

		Insert_Element(&(new_instances), res->csg_instance);

		free(label);
		if ( ! res->csg_instance )
		{
			fprintf(stderr, "Unable to load CSG instance line %d\n", line_num);
			exit(1);
		}

		res->csg_left_child = res->csg_right_child = NULL;
	}
	else
	{
		switch ( token )
		{
			case UNION:
				res->csg_op = csg_union_op;
				break;
			case INTERSECTION:
				res->csg_op = csg_intersection_op;
				break;
			case DIFFERENCE:
				res->csg_op = csg_difference_op;
				break;
			default:
				fprintf(stderr, "Unknown CSG Op token %d int %ld line %d\n",
						token, lex_int, line_num);
				exit(1);
		}
		res->csg_instance = NULL;

		res->csg_left_child = Load_CSG_Tree(res);
		res->csg_right_child = Load_CSG_Tree(res);
	}

	return res;
}


static void
Refresh_Spec(FeatureSpecPtr spec, ObjectInstancePtr obj, void *ptr, int num)
{
	if ( spec->spec_type == reference_spec )
		spec->spec_object =
			(ObjectInstancePtr)Hash_Get_Value(load_hash,
											  (long)(spec->spec_object));
}

void
Refresh_Feature_References(FeaturePtr options, int num)
{
	int i;

	for ( i = 0 ; i < num ; i++ )
		Constraint_Manipulate_Specs(options + i,
									NULL, NULL, 0, Refresh_Spec);
}


/*	void
**	Refresh_Instance_Pointers()
**	Changes all the instance pointers which are stale (because they were
**	read from file) to new the current instance pointers.
*/
static void
Refresh_Instance_Pointers()
{
	ObjectInstancePtr	inst;
	InstanceList	current;
	int	i, j;

	for ( current = new_instances ; current ; current = current->next )
	{
		inst = current->the_instance;
		for ( i = 0 ; i < inst->o_num_depend ; i++ )
			if ( ( inst->o_dependents[i].obj = (ObjectInstancePtr)
					Hash_Get_Value(load_hash,
								   (long)(inst->o_dependents[i].obj)) ) ==
					(void*)-1 )
			{
				for ( j = i + 1 ; j < inst->o_num_depend ; j++ )
					inst->o_dependents[j-1] = inst->o_dependents[j];
				inst->o_num_depend--;
				i--;
			}
		Refresh_Feature_References(inst->o_origin_cons, inst->o_origin_num);
		Refresh_Feature_References(inst->o_scale_cons, inst->o_scale_num);
		Refresh_Feature_References(inst->o_rotate_cons, inst->o_rotate_num);
		Refresh_Feature_References(&(inst->o_major_align), 1);
		Refresh_Feature_References(&(inst->o_minor_align), 1);
	}
	Free_Selection_List(new_instances);
	new_instances = NULL;
}

