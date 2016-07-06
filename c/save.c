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
**	save.c : Functions needed to save the scene.   It's remarkably complex.
**
**	External Functions:
**
**	void Save_Dialog_Func(Widget, XtPointer, XtPointer);
**	Puts up the save dialog box.
*/

#include <sced.h>
#include <base_objects.h>
#include <csg.h>
#include <csg_wire.h>
#include <hash.h>
#include <layers.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SelFile.h>
#include <X11/Shell.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiSrc.h>
#include <X11/Xaw/Dialog.h>
#include <View.h>

/* we need this for the definition of intptr_t */
#include <stdint.h>

#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif

extern void	Remove_Temporaries();
extern void	Radiance_Save_Extras(FILE*);

FILE*	Save_Name_Func(char **name);

int	Save_Header(FILE *);
int	Save_Viewports(FILE *);
int Save_CSG_Tree(FILE*, CSGNodePtr);
int	Save_BaseTypes(FILE *);
int	Save_Instances(FILE *);
int	Save_Instance(FILE *, ObjectInstancePtr);
static int	Save_Wireframe_Attributes(FILE*, WireframePtr);
int	Save_Attributes(FILE*, AttributePtr);
int	Save_Light_Info(FILE*, LightInfoPtr);
int	Save_Wireframe(FILE*, WireframePtr, Boolean);
int	Save_Instance(FILE*, ObjectInstancePtr);
int	Save_Camera(FILE*);
int	Save_Lights(FILE*);
int	Save_Constraints(FILE*, ObjectInstancePtr);
int Save_CSG_Trees(FILE*);
int	Save_String(FILE*, char*);
int Save_MainViewport(FILE *);

extern Viewport	*view_list;
extern String	*label_list;
extern int		num_views;

static int	on_completion;

static Boolean	write_to_pipe = FALSE;


/*	void
**	Save_Dialog_Func(Widget, XtPointer, XtPointer);
**	If necessary, creates, then pops up the save file dialog box.
*/
void
Save_Dialog_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
	FILE	*outfile;
	char	path_name[1024];
	struct stat	stat_struct;

	on_completion = (intptr_t)cl_data;

	if ( ! io_file_name )
	{
		if ( scene_path )
			strcpy(path_name, scene_path);
		else
			getcwd(path_name, 1024);
		strcat(path_name, "/*.scn");
	}
	else
		strcpy(path_name, io_file_name);
	outfile = XsraSelFile(main_window.shell, "Save to:", "Save", "Cancel", NULL,
							path_name, "w", NULL, &io_file_name);

	if ( outfile )
	{
		fclose(outfile);
		if ( stat(io_file_name, &stat_struct) != -1 &&
			 ! stat_struct.st_size )
			unlink(io_file_name);
		outfile = Save_Name_Func(&io_file_name);
		if ( outfile )
			Save_Func(outfile);
	}
}


FILE*
Save_Name_Func(char **name)
{
	char	*extension;
	char	*command_line;
	FILE	*result;

	if ( compress_output )
	{
		/* Make sure the extension is right. */
		extension = strrchr(*name, '.');
		if ( ! extension ||
			   ( strcmp(extension, ".gz") &&
			     strcmp(extension, ".z") &&
			     strcmp(extension, ".Z") ) )
		{
			*name = More(*name, char, strlen(*name) + 5);
#if ( HAVE_GZIP )
			strcat(*name, ".gz");
#else
			strcat(*name, ".Z");
#endif
		}

		command_line = New(char, strlen(*name) + 20);

#if ( HAVE_GZIP )
		strcpy(command_line, "gzip -c > ");
#else
		strcpy(command_line, "compress -c > ");
#endif
		strcat(command_line, *name);

		result =  popen(command_line, "w");
		free(command_line);

		write_to_pipe = TRUE;

		return result;
	}
	else
		return ( fopen(*name, "w") );
}


void
Save_Close(FILE *victim)
{
	if ( write_to_pipe )
		pclose(victim);
	else
		fclose(victim);

	write_to_pipe = FALSE;
}



/*	void
**	Save_Func(FILE *outfile)
**	Initiates the saving of a file from a command button.
*/
void	
Save_Func(FILE *outfile)
{	int i,numkf;

	/* So, this function should do one of two things: if you're working
	** on a normal scene, it should save a single scene that, with the
	** exception of Ani_Rot and Ani_Scale values, is compatible with
	** normal sced.  However, if there are more than one keyframe, this
	** routine will save the entire set of keyframes.
	*/

	numkf=Count_KeyFrames(key_frames);
	
	if (numkf!=1)
	{	Update_to_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames));
		Update_from_KeyFrame(key_frames,TRUE);
	}
	

	if ( Save_Header(outfile) < 0 ||
		 Save_MainViewport(outfile) < 0 ||
		 Save_Viewports(outfile) < 0 ||
		 Save_Lights(outfile) < 0 ||
		 Save_Camera(outfile) < 0 ||
		 POV_Save_Includes(outfile) < 0 ||
		 Save_Declarations(outfile) < 0 ||
		 Save_BaseTypes(outfile) < 0 ||
		 Save_Instances(outfile) < 0 ||
		 Save_CSG_Trees(outfile) < 0 )
	{
		Popup_Error("Write Failed!", main_window.shell, "Error");
		fclose(outfile);
		return;
	}

	/* Now we check if there is another keyframe, and if so, proceed to
	** save all the rest.
	*/
	if (numkf!=1)
		for (i=2; i<=numkf; i++)
		{	Update_from_KeyFrame(Nth_KeyFrame(i,key_frames),TRUE);

			if (Save_Frame(outfile,main_window.fnumber) < 0 ||
				Save_MainViewport(outfile) < 0 ||
				Save_Lights(outfile) < 0 ||
				Save_Camera(outfile) < 0 ||
				Save_Instances(outfile) < 0 )
			{
				Popup_Error("Write Failed!", main_window.shell, "Error");
				fclose(outfile);
				return;
			}
		}
				
	Save_Close(outfile);

	if ( on_completion == SAVE_QUIT )
	{
		Remove_Temporaries();
		exit(0);
	}

	changed_scene = FALSE;

	if ( on_completion == SAVE_LOAD )
		Load_Dialog_Func(NULL, NULL, NULL);

	if ( on_completion == SAVE_RESET )
		Reset_Dialog_Func(NULL, NULL, NULL);

	if ( on_completion == SAVE_CLEAR )
		Clear_Dialog_Func(NULL, NULL, NULL);

	on_completion = SAVE_ONLY;

}

/*	int
**	Save_Header(FILE *file)
**	Writes an appropriate header to file.
*/
int
Save_Header(FILE *file)
{
	time_t	current_time;

	time(&current_time);

	fprintf(file, "# Sced Scene Description file\n# %s\n",ctime(&current_time));
	fprintf(file, "Internal\n");

	return ( fprintf(file, "Version "VERSION"\n") );
}


/*	int
**	Save_Frame(FILE *file,int kfn)
**	Writes a frame separator.
*/
int
Save_Frame(FILE *file, int fn)
{
	debug(FUNC_NAME,fprintf(stderr,"Save_Frame()\n"));

	return(fprintf(file, "\nNewFrame %d\n",fn));
}


static int
Save_Viewport(FILE *file, ViewportPtr vp, int width, int height, int mag)
{
	fprintf(file, "LookFrom ");	VPrint(file, vp->view_from);
	fprintf(file, "LookAt ");	VPrint(file, vp->view_at);
	fprintf(file, "LookUp ");	VPrint(file,vp->view_up);
	fprintf(file, "ViewDist %1.15g\n", vp->view_distance);
	fprintf(file, "EyeDist %1.15g\n", vp->eye_distance);
	fprintf(file, "Magnify %d\n", mag);
	fprintf(file, "Mode %d\n", vp->draw_mode);
	return fprintf(file, "Screen %d %d\n", width, height);
}


/*	int
**	Save_Viewports(FILE *file)
**	Saves the viewport information to file.
**
** 'Kay, I split this code out from the old Save_Viewports() so
** that I could save this information repeatedly (per frame), but
** still save the viewport information just once.
*/
int
Save_MainViewport(FILE *file)
{
	Dimension	width, height;
	int			mag;
	int			i;

	debug(FUNC_NAME,fprintf(stderr,"Save_Viewports()\n"));

	/* First the main_viewport information. */
	XtVaGetValues(main_window.view_widget,
				XtNmagnification, &mag,
				XtNdesiredWidth, &width,
				XtNdesiredHeight, &height, NULL);
	fprintf(file, "MainViewport\n");
	Save_Viewport(file, &(main_window.viewport), (int)width, (int)height, mag);

	return fprintf(file, "\n");
}


/*	int
**	Save_Viewports(FILE *file)
**	Saves the viewport information to file.
*/
int
Save_Viewports(FILE *file)
{
	Dimension	width, height;
	int			mag;
	int			i;

	debug(FUNC_NAME,fprintf(stderr,"Save_Viewports()\n"));

	fprintf(file, "Axes %ld\n", (long)&(main_window.axes));
	fprintf(file, "\n");

	/* Then the CSG Viewport. */
	if ( csg_window.view_widget )
	{
		XtVaGetValues(csg_window.view_widget,
					XtNmagnification, &mag,
					XtNdesiredWidth, &width,
					XtNdesiredHeight, &height, NULL);
		fprintf(file, "CSGViewport\n");
		Save_Viewport(file, &(csg_window.viewport), (int)width, (int)height,
					  mag);
		fprintf(file, "Axes %ld\n", (long)&(csg_window.axes));
	}

	/* Then any saved non defaults. */
	for ( i = 0 ; i < num_views ; i++ )
	{
		if ( view_list[i].is_default ) continue;
		fprintf(file, "Viewport \"%s\"\n", label_list[i + 1]);
		Save_Viewport(file, view_list + i, (int)view_list[i].scr_width,
					  (int)view_list[i].scr_height, view_list[i].magnify);
	}

	Save_Layers(file);

	return fprintf(file, "\n");
}



/*	int
**	Save_Camera(FILE *outfile)
**	Saves the camera specifications.  It's full of special cases.
*/
int
Save_Camera(FILE *outfile)
{
	debug(FUNC_NAME,fprintf(stderr,"Save_Camera()\n"));

	fprintf(outfile, "Camera\n");

	switch ( camera.type )
	{
		case NoTarget:
			fprintf(outfile, "None\n");
			fprintf(outfile, "Screen %d %d\n", (int)camera.scr_width,
					(int)camera.scr_height);

			return fprintf(outfile, "\n");

		case Rayshade:
			fprintf(outfile, "Rayshade\n");
			break;

		case POVray:
			fprintf(outfile, "POVray\n");
			break;

		case Radiance:
			fprintf(outfile, "Radiance ");
			Radiance_Save_Extras(outfile);
			break;

		case Renderman:
			fprintf(outfile, "Renderman\n");
			break;

		case Genray:
			fprintf(outfile, "Genray\n");
			break;

		case Genscan:
			fprintf(outfile, "Genscan\n");
			break;
	}

	if ( camera.default_cam )
		fprintf(outfile, "Default\n");
	fprintf(outfile, "LookFrom ");	VPrint(outfile, camera.location);
	fprintf(outfile, "LookAt ");	VPrint(outfile, camera.look_at);
	fprintf(outfile, "LookUp ");	VPrint(outfile, camera.look_up);
	fprintf(outfile, "EyeDist %1.15g\n", camera.eye_dist);
	fprintf(outfile, "HFOV %1.15g\n", camera.horiz_fov);
	fprintf(outfile, "VFOV %1.15g\n", camera.vert_fov);
	fprintf(outfile, "Up %1.15g\n", camera.window_up);
	fprintf(outfile, "Right %1.15g\n", camera.window_right);

	fprintf(outfile, "Screen %d %d\n", (int)camera.scr_width,
			(int)camera.scr_height);

	return fprintf(outfile, "\n");
}



/*	int
**	Save_BaseTypes(FILE *file)
**	Saves information about base types to file.
*/
int
Save_BaseTypes(FILE *file)
{
	int	i;

	/* Save information about non-default base objects. */
	fprintf(file, "BaseObjects\n");

	for ( i = 0 ; i < NUM_GENERIC_OBJS ; i++ )
		fprintf(file, "%ld\n", (long)(base_objects[i]));

	for ( ; i < num_base_objects ; i++ )
	{
	 	fprintf(file, "\"%s\"\n", base_objects[i]->b_label);
		if ( base_objects[i]->b_class == csg_obj )
			fprintf(file, "CSG\n");
		else if ( base_objects[i]->b_class == wireframe_obj )
			fprintf(file, "Wireframe\n");
		fprintf(file, "%ld\n", (long)(base_objects[i]));
		fprintf(file, "Reference %d\n", base_objects[i]->b_ref_num);
		if ( base_objects[i]->b_class == csg_obj )
		{
			Save_CSG_Tree(file, base_objects[i]->b_csgptr);
			Save_Wireframe(file, base_objects[i]->b_major_wire,
						   base_objects[i]->b_use_full);

			if ( save_simple_wires )
				Save_Wireframe(file, base_objects[i]->b_wireframe, FALSE);
		}
		else if ( base_objects[i]->b_class == wireframe_obj )
		{
			Save_Wireframe_Attributes(file, base_objects[i]->b_major_wire);
			Save_Wireframe(file, base_objects[i]->b_major_wire, FALSE);
		}
	}

	return fprintf(file, "\n");
}


/*	int
**	Save_CSG_Tree(FILE *file, CSGNodePtr tree)
**	Saves the tree to file.
*/
int
Save_CSG_Tree(FILE *file, CSGNodePtr tree)
{
	if ( ! tree ) return 1;

	if ( tree->csg_op == csg_leaf_op )
		Save_Instance(file, tree->csg_instance);
	else
	{
		switch ( tree->csg_op )
		{
			case csg_union_op:
				fprintf(file, "Union\n");
				break;
			case csg_intersection_op:
				fprintf(file, "Intersection\n");
				break;
			case csg_difference_op:
				fprintf(file, "Difference\n");
				break;
			default:;
		}
		Save_CSG_Tree(file, tree->csg_left_child);
		Save_CSG_Tree(file, tree->csg_right_child);
	}

	return 1;
}


/*	int
**	Save_Instances(FILE *file)
**	Saves the instance list to file.
*/
int
Save_Instances(FILE *file)
{
	InstanceList		inst_elmt;

	debug(FUNC_NAME,fprintf(stderr,"Save_Instances()\n"));

	fprintf(file, "Instances\n");

	for ( inst_elmt = main_window.all_instances ;
		  inst_elmt != NULL ;
		  inst_elmt = inst_elmt->next )
		Save_Instance(file, inst_elmt->the_instance);

	return fprintf(file, "\n");
}

int
Save_Instance(FILE *file, ObjectInstancePtr inst)
{
	int				i;

	fprintf(file, "\"%s\"\n", inst->o_label);
	fprintf(file, "%ld\n", (long)inst);
	fprintf(file, "%ld\n", (long)(inst->o_parent));
	if ( inst->o_wireframe != inst->o_parent->b_wireframe )
		fprintf(file, "Dense %d\n", Wireframe_Density_Level(inst));

	fprintf(file, "Transformation\n");
	MPrint(file, inst->o_transform.matrix);
	VPrint(file, inst->o_transform.displacement);

	if ( inst->o_parent->b_class == light_obj ||
		 inst->o_parent->b_class == spotlight_obj ||
		 inst->o_parent->b_class == arealight_obj )
		Save_Light_Info(file, ((LightInfoPtr)inst->o_attribs));
	else
		Save_Attributes(file, ((AttributePtr)inst->o_attribs));

	fprintf(file, "Ani_Rotation ");
	QPrint(file, inst->o_rot);

	fprintf(file, "Ani_Scale ");
	VPrint(file, inst->o_scale);

	fprintf(file, "Layer %d\n", inst->o_layer);

	Save_Constraints(file, inst);

	fprintf(file, "Dependents %d", inst->o_num_depend);
	for ( i = 0 ; i < inst->o_num_depend ; i++ )
		fprintf(file, " %ld %d", (long)(inst->o_dependents[i].obj),
								 (int)(inst->o_dependents[i].count));
	fprintf(file, "\n");

	return fprintf(file, "\n");
}



/*	int
**	Save_Wireframe(FILE *file, WireframePtr w, Boolean full)
**	Saves information about wireframe w to file.
*/
int
Save_Wireframe(FILE *file, WireframePtr w, Boolean full)
{
	int		i, j;

#define WVPrint(v) fprintf(file, "%1.15g %1.15g %1.15g\n", (v).x, (v).y, (v).z);

	if ( full )
		fprintf(file, "Wireframe Full\n");
	else
		fprintf(file, "Wireframe\n");

	fprintf(file, "%d\n", w->num_vertices);
	for ( i = 0 ; i < w->num_vertices ; i++ )
		WVPrint(w->vertices[i]);

	fprintf(file, "%d\n", w->num_faces);
	for ( i = 0 ; i < w->num_faces ; i++ )
	{
		fprintf(file, "%d ", w->faces[i].num_vertices);
		for ( j = 0 ; j < w->faces[i].num_vertices ; j++ )
			fprintf(file, "%d ", w->faces[i].vertices[j]);
		fprintf(file, "%ld ", (long)(w->faces[i].face_attribs));
		WVPrint(w->faces[i].normal);
	}

	if ( w->vertex_normals )
	{
		fprintf(file, "Normal\n");
		for ( i = 0 ; i < w->num_vertices ; i++ )
			WVPrint(w->vertex_normals[i]);
	}

	return fprintf(file, "\n");
	
}




static int
Save_Wireframe_Attributes(FILE *file, WireframePtr wire)
{
	int	i;

	fprintf(file, "%d\n", wire->num_attribs);
	for ( i = 0 ; i < wire->num_attribs ; i++ )
		Save_Attributes(file, wire->attribs[i]);

	return 1;
}



/*	int
**	Save_Attributes(FILE *file, AttributePtr a)
**	Saves the attributes from a.
*/
int
Save_Attributes(FILE *file, AttributePtr a)
{
	fprintf(file, "Attributes %ld %d\n", (long)a, a->defined ? 1 : 0);

	fprintf(file, "Color %d %d %d\n", a->colour.red, a->colour.green,
				a->colour.blue);
	fprintf(file, "Diffuse %g\n", a->diff_coef);
	fprintf(file, "Specular %g %g\n", a->spec_coef, a->spec_power);
	fprintf(file, "Reflect %g\n", a->reflect_coef);
	fprintf(file, "Transparency %g\n", a->transparency);
	fprintf(file, "Refract %g\n", a->refract_index);
	if ( a->extension )
	{
		fprintf(file, "Extend %d ", ( a->use_extension ? 1 : 0 ));
		Save_String(file, a->extension);
		fprintf(file, "\n");
	}
	fprintf(file, "Transformation %d\n", ( a->use_obj_trans ? 1 : 0 ));
	fprintf(file, "Open %d\n", ( a->open ? 1 : 0 ));

	return TRUE;
}


int
Save_Light_Info(FILE *file, LightInfoPtr l)
{
	fprintf(file, "Attributes %g %g %g %g %g %d\n",
			l->red, l->green, l->blue, l->val1, l->val2, ( l->flag ? 1 : 0 ));
	return TRUE;
}



/*	int
**	Save_Lights(FILE *file)
**	Saves information for the defined lights.
*/
int
Save_Lights(FILE *file)
{
	debug(FUNC_NAME,fprintf(stderr,"Save_Lights()\n"));

	fprintf(file, "Ambient %d %d %d\n", ambient_light.red,
			ambient_light.green, ambient_light.blue);

	return fprintf(file, "\n");
}



static void
Save_Feature_Specs(FILE *outfile, FeatureSpecifier *spec)
{
	fprintf(outfile, "%d ", spec->spec_type);
	if ( spec->spec_type == reference_spec )
		fprintf(outfile, "%ld ", (long)(spec->spec_object));
	VPrint(outfile, spec->spec_vector);
}


static void
Save_Feature(FILE *outfile, FeaturePtr feat)
{
	switch ( feat->f_spec_type )
	{
		case axis_plane_feature:
		case axis_feature:
			fprintf(outfile, "Axes ");
			break;
		case ref_line_feature:
		case ref_plane_feature:
			fprintf(outfile, "Reference ");
			break;
		case orig_line_feature:
		case orig_plane_feature:
			fprintf(outfile, "Origin ");
			break;
		default:;
	}

	switch ( feat->f_spec_type )
	{
		case axis_plane_feature:
		case plane_feature:
		case orig_plane_feature:
		case ref_plane_feature:
			fprintf(outfile, "Plane \"%s\" %1.15g %1.15g %1.15g ",
					feat->f_label, feat->f_vector.x,
					feat->f_vector.y, feat->f_vector.z);
			VPrint(outfile, feat->f_point);
			Save_Feature_Specs(outfile, feat->f_specs + 2);
			Save_Feature_Specs(outfile, feat->f_specs + 1);
			Save_Feature_Specs(outfile, feat->f_specs + 0);
			break;
		case midplane_feature:
			fprintf(outfile, "Mid Plane \"%s\" %1.15g %1.15g %1.15g ",
					feat->f_label, feat->f_vector.x,
					feat->f_vector.y, feat->f_vector.z);
			VPrint(outfile, feat->f_point);
			Save_Feature_Specs(outfile, feat->f_specs + 1);
			Save_Feature_Specs(outfile, feat->f_specs + 0);
			break;
		case midpoint_feature:
			fprintf(outfile, "Mid Point \"%s\" ", feat->f_label);
			VPrint(outfile, feat->f_point);
			Save_Feature_Specs(outfile, feat->f_specs + 1);
			Save_Feature_Specs(outfile, feat->f_specs + 0);
			break;
		case line_feature:
		case axis_feature:
		case ref_line_feature:
		case orig_line_feature:
			fprintf(outfile, "Line \"%s\" %1.15g %1.15g %1.15g ",
					feat->f_label, feat->f_vector.x,
					feat->f_vector.y, feat->f_vector.z);
			VPrint(outfile, feat->f_point);
			Save_Feature_Specs(outfile, feat->f_specs + 1);
			Save_Feature_Specs(outfile, feat->f_specs);
			break;
		case point_feature:
			fprintf(outfile, "Point \"%s\" ", feat->f_label);
			VPrint(outfile, feat->f_point);
			Save_Feature_Specs(outfile, feat->f_specs);
			break;
		default: ;
	}
}

/*	int
**	Save_Constraints(FILE *outfile, ObjectInstancePtr inst)
**	Saves a list of constraints.
*/
int
Save_Constraints(FILE *outfile, ObjectInstancePtr inst)
{
	int	i;

	fprintf(outfile, "Axes\n");
	MPrint(outfile, inst->o_axes);
	fprintf(outfile, "Origin ");	VPrint(outfile, inst->o_origin);
	fprintf(outfile, "Reference ");	VPrint(outfile, inst->o_reference);

	fprintf(outfile, "Constraints\n");
	fprintf(outfile, "%d\n", inst->o_origin_num);
	for ( i = 0 ; i < inst->o_origin_num ; i++ )
		Save_Feature(outfile, inst->o_origin_cons + i);
	fprintf(outfile, "Active");
	for ( i = 0 ; i < inst->o_origin_num + 3 ; i++ )
		fprintf(outfile, " %d", ( inst->o_origin_active[i] ? 1 : 0));
	fprintf(outfile, "\n");

	fprintf(outfile, "%d\n", inst->o_scale_num);
	for ( i = 0 ; i < inst->o_scale_num ; i++ )
		Save_Feature(outfile, inst->o_scale_cons + i);
	fprintf(outfile, "Active");
	for ( i = 0 ; i < inst->o_scale_num + 3 ; i++ )
		fprintf(outfile, " %d", ( inst->o_scale_active[i] ? 1 : 0));
	fprintf(outfile, "\n");

	fprintf(outfile, "%d\n", inst->o_rotate_num);
	for ( i = 0 ; i < inst->o_rotate_num ; i++ )
		Save_Feature(outfile, inst->o_rotate_cons + i);
	fprintf(outfile, "Active");
	for ( i = 0 ; i < inst->o_rotate_num + 3 ; i++ )
		fprintf(outfile, " %d", ( inst->o_rotate_active[i] ? 1 : 0));
	fprintf(outfile, "\n");

	if ( inst->o_major_align.f_type != null_feature )
	{
		fprintf(outfile, "Major ");
		Save_Feature(outfile, &(inst->o_major_align));
	}
	if ( inst->o_minor_align.f_type != null_feature )
	{
		fprintf(outfile, "Minor ");
		Save_Feature(outfile, &(inst->o_minor_align));
	}

	return fprintf(outfile, "\n");
}


int
Save_CSG_Trees(FILE *outfile)
{
	int	i;

	fprintf(outfile, "CSG\n");
	fprintf(outfile, "%d\n", num_displayed_trees);

	for ( i = 0 ; i < num_displayed_trees ; i++ )
	{
		Save_CSG_Tree(outfile, displayed_trees[i].tree);
		fprintf(outfile, "%d\n\n", displayed_trees[i].displayed ? 1 : 0 );
	}

	return fprintf(outfile, "\n");
}


int
Save_String(FILE *outfile, char *string)
{
	int	length = strlen(string);
	int	i;

	fprintf(outfile, "\"");
	for ( i = 0 ; i < length ; i++ )
		if ( string[i] == '\"' )
			fprintf(outfile, "\\\"");
		else
			fprintf(outfile, "%c", string[i]);
	return fprintf(outfile, "\"");
}


