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
**	The RenderMan(r) Interface Procedures and RIB Protocol are:
**	Copyright 1988, 1989, Pixar.
**	All rights reserved.
**	RenderMan(r) is a registered trademark of Pixar.
*/

/*
**	renderman.c: File containing functions for exporting to renderman.
*/


#include <math.h>
#include <ctype.h>
#include <sced.h>
#include <base_objects.h>
#include <csg.h>
#include <hash.h>
#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif
#include <time.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Label.h>

/* I had to add this line to get USE_ROUNDED_CORNERS to work. */
#include <X11/Xaw/Command.h>


static void		Renderman_Export_Camera(FILE*, Camera*);
static void		Renderman_Export_Lights(FILE*, ScenePtr);
static void		Renderman_Export_Object(FILE*, ObjectInstancePtr);
static void		Renderman_Export_Instances(FILE*, InstanceList);
static char*	Renderman_Get_Filename(char*);

static Boolean	dialog_done;
static Boolean	dialog_cancel;

#define Renderman_Export_Transform(t) \
		fprintf(outfile, "ConcatTransform %1.15g %1.15g %1.15g 0.0 "\
										 "%1.15g %1.15g %1.15g 0.0 "\
										 "%1.15g %1.15g %1.15g 0.0 "\
										 "%1.15g %1.15g %1.15g 1.0\n",\
				(t).matrix.x.x, (t).matrix.y.x, (t).matrix.z.x,\
				(t).matrix.x.y, (t).matrix.y.y, (t).matrix.z.y,\
				(t).matrix.x.z, (t).matrix.y.z, (t).matrix.z.z,\
				(t).displacement.x, (t).displacement.y, (t).displacement.z);

int
Export_Renderman(FILE *outfile, char *filename, ScenePtr scene, Boolean preview)
{
	char	*image_filename;
	time_t	current_time;

	time(&current_time);

	if ( ! preview )
		image_filename = Renderman_Get_Filename(filename);
	else
		image_filename = "preview.tif";

	if ( ! image_filename )
		return 1;

	fprintf(outfile, "# File produced by Sced version "VERSION"\n");
	fprintf(outfile, "# %s\n", ctime(&current_time));
	if ( preview )
		fprintf(outfile, "Display \"preview.tif\" \"file\" \"rgba\"\n");
	else
		fprintf(outfile, "Display \"%s\" \"file\" \"rgba\"\n", image_filename);

	Renderman_Export_Camera(outfile, &(scene->camera));

	fprintf(outfile, "WorldBegin\n");

	Renderman_Export_Lights(outfile, scene);

	Renderman_Export_Instances(outfile, scene->instances);

	return fprintf(outfile, "WorldEnd\n");
}


static void
Renderman_File_Done_Callback(Widget w, XtPointer cl, XtPointer ca)
{
	dialog_cancel = cl ? FALSE : TRUE;
	dialog_done = TRUE;
}


void
Renderman_Action_Func(Widget w, XEvent *e, String *s, Cardinal *num)
{
	Renderman_File_Done_Callback(w, (XtPointer)TRUE, NULL);
}

static Widget
Renderman_Create_Filename_Shell(Widget *dialog)
{
	Widget	shell;
	Arg		args[3];
	int		n;

	n = 0;
	XtSetArg(args[n], XtNtitle, "Image Filename");	n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
	shell = XtCreatePopupShell("rendermanFileShell", transientShellWidgetClass,
								main_window.shell, args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Output Image Filename");	n++;
	XtSetArg(args[n], XtNvalue, "");						n++;
	*dialog = XtCreateManagedWidget("rendermanFileDialog", dialogWidgetClass,
									shell, args, n);

	XawDialogAddButton(*dialog, "Done", Renderman_File_Done_Callback,
					   (XtPointer)TRUE);
	XawDialogAddButton(*dialog, "Cancel", Renderman_File_Done_Callback,
					   (XtPointer)FALSE);

	XtOverrideTranslations(XtNameToWidget(*dialog, "value"),
		XtParseTranslationTable(":<Key>Return: Renderman_File_Action()"));

	XtVaSetValues(XtNameToWidget(*dialog, "label"), XtNborderWidth, 0, NULL);
#if ( USE_ROUNDED_BUTTONS == 1 )
	XtVaSetValues(XtNameToWidget(*dialog, "Done"),
					XtNshapeStyle, XmuShapeRoundedRectangle,
					XtNcornerRoundPercent, 30,
					XtNhighlightThickness, 2,
					NULL);
	XtVaSetValues(XtNameToWidget(*dialog, "Cancel"),
					XtNshapeStyle, XmuShapeRoundedRectangle,
					XtNcornerRoundPercent, 30,
					XtNhighlightThickness, 2,
					NULL);
#endif

	XtRealizeWidget(shell);

	return shell;
}


static char*
Renderman_Get_Filename(char *default_base)
{
	static Widget	filename_shell = NULL;
	static Widget	filename_dialog = NULL;
	char			*default_name;
	char			*temp;
	XtAppContext	context;
	XEvent			event;

	if ( ! filename_shell )
		filename_shell = Renderman_Create_Filename_Shell(&filename_dialog);

	if ( ! default_base )
		default_name = Strdup("image.tif");
	else
	{
		temp = strrchr(default_base, '/');
		if ( temp )
			default_name = Strdup(temp + 1);
		else
			default_name = Strdup(default_base);
		temp = strrchr(default_name, '.');
		if ( temp )
			*temp = '\0';
		default_name = More(default_name, char, strlen(default_name) + 5);
		strcat(default_name, ".tif");
	}

	XtVaSetValues(filename_dialog, XtNvalue, default_name, NULL);

	free(default_name);

	SFpositionWidget(filename_shell);
	XtPopup(filename_shell, XtGrabExclusive);

	dialog_done = FALSE;
	dialog_cancel = FALSE;
	context = XtWidgetToApplicationContext(main_window.shell);
	while ( ! dialog_done )
	{
		XtAppNextEvent(context, &event);
		XtDispatchEvent(&event);
	}

	XtPopdown(filename_shell);

	if ( dialog_cancel )
		return NULL;

	return XawDialogGetValueString(filename_dialog);
}


static void
Renderman_Export_Camera(FILE *outfile, Camera *cam)
{
	Viewport	cam_view;
	double		min_fov;

	fprintf(outfile, "Format %d %d 1\n", cam->scr_width, cam->scr_height);

	min_fov = 360 * atan(min(cam->window_right, cam->window_up) * 0.5 /
					   cam->eye_dist) / M_PI;
	fprintf(outfile, "Projection \"perspective\" \"fov\" %g\n", min_fov);

	fprintf(outfile, "FrameAspectRatio %g\n",
			(double)cam->window_right / (double)cam->window_up);

	Camera_To_Viewport(cam, &cam_view);
	/* Export the camera transformation matrix. */
	fprintf(outfile,
			"Transform %1.15g %1.15g %1.15g 0.0 %1.15g %1.15g %1.15g 0.0 "
			"%1.15g %1.15g %1.15g 0.0 %1.15g %1.15g %1.15g 1.0\n",
			cam_view.world_to_view.matrix.x.x,
			cam_view.world_to_view.matrix.y.x,
			cam_view.world_to_view.matrix.z.x,
			cam_view.world_to_view.matrix.x.y,
			cam_view.world_to_view.matrix.y.y,
			cam_view.world_to_view.matrix.z.y,
			cam_view.world_to_view.matrix.x.z,
			cam_view.world_to_view.matrix.y.z,
			cam_view.world_to_view.matrix.z.z,
			cam_view.world_to_view.displacement.x,
			cam_view.world_to_view.displacement.y,
			cam_view.world_to_view.displacement.z + cam_view.eye_distance);

	fprintf(outfile, "\n");
}


static void
Renderman_Export_Light(FILE *outfile, ObjectInstancePtr obj, int num)
{
	Vector	color;
	double	intensity, temp_d;

	intensity = max(((LightInfoPtr)obj->o_attribs)->red,
					((LightInfoPtr)obj->o_attribs)->green);
	intensity = max(intensity, ((LightInfoPtr)obj->o_attribs)->blue);
	color.x = ((LightInfoPtr)obj->o_attribs)->red;
	color.y = ((LightInfoPtr)obj->o_attribs)->green;
	color.z = ((LightInfoPtr)obj->o_attribs)->blue;
	if ( ! IsZero(intensity) )
	{
		temp_d = 1 / intensity;
		VScalarMul(color, temp_d, color);
	}

	switch ( obj->o_parent->b_class )
	{
		case light_obj:
			fprintf(outfile,
					"LightSource \"pointlight\" %d \"lightcolor\" [ %g %g %g ]"
					" \"intensity\" %f \"from\" [ %1.15g %1.15g %1.15g ]\n",
					num, color.x, color.y, color.z, intensity,
					obj->o_transform.displacement.x,
					obj->o_transform.displacement.y,
					obj->o_transform.displacement.z);
			break;

		case arealight_obj:
			/* Begin the area light source definition. */
			fprintf(outfile,
				"AreaLightSource \"pointlight\" %d"
				" \"lightcolor\" [ %g %g %g ] \"intensity\" %f\n",
				num, color.x, color.y, color.z, intensity);
			/* Export the actual arealight as a polygon. */
			Renderman_Export_Transform(obj->o_transform);
			fprintf(outfile, "Polygon \"P\" [ 1 1 0 -1 1 0 -1 -1 0 1 -1 0 ]\n");
			break;

		case spotlight_obj:
			{
				double	radius;
				double	cos_rad;
				Vector	vect1, vect2;
				Vector	direction;

				/* Calculate the radius. */
				VSub(obj->o_world_verts[0], obj->o_world_verts[9], vect1);
				VSub(obj->o_world_verts[8], obj->o_world_verts[9], vect2);
				VUnit(vect1, radius, vect1);
				VUnit(vect2, radius, vect2);
				cos_rad = VDot(vect1, vect2);
				radius = acos(cos_rad);

				if ( ((LightInfoPtr)obj->o_attribs)->flag )
				{
					/* Invert it. */
					/* vect2 still points toward direction. */
					VScalarMul(vect2, -1.0, vect2);
					VAdd(vect2, obj->o_world_verts[9], direction);

					radius += M_PI_2;
				}
				else
					direction = obj->o_world_verts[8];

				fprintf(outfile,
						"LightSource \"spotlight\" %d"
						" \"lightcolor\" [ %g %g %g ] \"intensity\" %g"
						" \"from\" [ %1.15g %1.15g %1.15g ]"
						" \"to\" [ %1.15g %1.15g %1.15g ]"
						" \"coneangle\" %g"
						" \"conedeltaangle\" %g"
						" \"beamdistribution\" %g\n",
						num,
						color.x, color.y, color.x, intensity,
						obj->o_transform.displacement.x,
						obj->o_transform.displacement.y,
						obj->o_transform.displacement.z,
						direction.x, direction.y, direction.z,
						radius,
						radius * (((LightInfoPtr)obj->o_attribs)->val1 - 1.0),
						((LightInfoPtr)obj->o_attribs)->val2);
			}
			break;

		default:;
	}
	fprintf(outfile, "\n");
}


static void
Renderman_Export_Lights(FILE *outfile, ScenePtr scene)
{
	InstanceList	inst;
	int				light_number = 1;

	/* The ambient light source. */
	fprintf(outfile,
			"LightSource \"ambientlight\" %d \"lightcolor\" [ %f %f %f ]\n",
			light_number++,
			scene->ambient.red / (double)MAX_UNSIGNED_SHORT,
			scene->ambient.green / (double)MAX_UNSIGNED_SHORT,
			scene->ambient.blue / (double)MAX_UNSIGNED_SHORT);

	if ( scene->light )
	{
		((LightInfoPtr)scene->light->o_attribs)->red *= 5000;
		((LightInfoPtr)scene->light->o_attribs)->green *= 5000;
		((LightInfoPtr)scene->light->o_attribs)->blue *= 5000;
		Renderman_Export_Light(outfile, scene->light, light_number++);
	}

	/* Go through all the instances looking for lights. */
	for ( inst = scene->instances ; inst ; inst = inst->next )
		if ( Obj_Is_Light(inst->the_instance) )
			Renderman_Export_Light(outfile, inst->the_instance, light_number++);
}


static void
Renderman_Export_Attributes(FILE *outfile, AttributePtr attribs)
{
	if ( ! attribs->defined )
		return;

	fprintf(outfile, "Color %g %g %g\n",
			attribs->colour.red / (double)MAX_UNSIGNED_SHORT,
			attribs->colour.green / (double)MAX_UNSIGNED_SHORT,
			attribs->colour.blue / (double)MAX_UNSIGNED_SHORT);

	if ( attribs->use_extension )
	{
		fprintf(outfile, "Surface %s\n", attribs->extension);
		return;
	}

	/* Must be simple attributes. We'll export as plastic. */
	fprintf(outfile,
			"Surface \"plastic\" \"Ka\" 1.0 \"Kd\" %g \"Ks\" %g"
			" \"roughness\" %g\n",
			attribs->diff_coef, attribs->spec_coef, 1 / attribs->spec_power);
}

static void
Renderman_Export_CSG_Object(FILE *outfile, CSGNodePtr obj)
{
	if ( ! obj )
		return;

	switch ( obj->csg_op )
	{
		case csg_union_op:
			fprintf(outfile, "SolidBegin \"union\"\n");
			Renderman_Export_CSG_Object(outfile, obj->csg_left_child);
			Renderman_Export_CSG_Object(outfile, obj->csg_right_child);
			fprintf(outfile, "SolidEnd\n");
			break;

		case csg_intersection_op:
			fprintf(outfile, "SolidBegin \"intersection\"\n");
			Renderman_Export_CSG_Object(outfile, obj->csg_left_child);
			Renderman_Export_CSG_Object(outfile, obj->csg_right_child);
			fprintf(outfile, "SolidEnd\n");
			break;

		case csg_difference_op:
			fprintf(outfile, "SolidBegin \"difference\"\n");
			Renderman_Export_CSG_Object(outfile, obj->csg_left_child);
			Renderman_Export_CSG_Object(outfile, obj->csg_right_child);
			fprintf(outfile, "SolidEnd\n");
			break;

		case csg_leaf_op:
			fprintf(outfile, "SolidBegin \"primitive\"\n");
			Renderman_Export_Object(outfile, obj->csg_instance);
			fprintf(outfile, "SolidEnd\n");
			break;
	}
}


static void
Renderman_Export_Wireframe_Object(FILE *outfile, WireframePtr wireframe)
{
	int	i, j;

	for ( i = 0 ; i < wireframe->num_faces ; i++ )
	{
		if ( wireframe->faces[i].face_attribs &&
			 wireframe->faces[i].face_attribs->defined )
		{
			fprintf(outfile, "AttributeBegin\n");
			Renderman_Export_Attributes(outfile,
										wireframe->faces[i].face_attribs);
		}

		fprintf(outfile, "Polygon \"P\" [ ");
		for ( j = 0 ; j < wireframe->faces[i].num_vertices ; j++ )
			fprintf(outfile, "%1.15g %1.15g %1.15g ",
					wireframe->vertices[wireframe->faces[i].vertices[j]].x,
					wireframe->vertices[wireframe->faces[i].vertices[j]].y,
					wireframe->vertices[wireframe->faces[i].vertices[j]].z);
		if ( wireframe->vertex_normals )
		{
			fprintf(outfile, "] \"N\" [ ");
			for ( j = 0 ; j < wireframe->faces[i].num_vertices ; j++ )
				fprintf(outfile, "%1.15g %1.15g %1.15g ",
				  wireframe->vertex_normals[wireframe->faces[i].vertices[j]].x,
				  wireframe->vertex_normals[wireframe->faces[i].vertices[j]].y,
				  wireframe->vertex_normals[wireframe->faces[i].vertices[j]].z);
			fprintf(outfile, "]\n");
		}
		else
			fprintf(outfile, "]\n");

		if ( wireframe->faces[i].face_attribs &&
			 wireframe->faces[i].face_attribs->defined )
			fprintf(outfile, "AttributeEnd\n");
	}
}


static void
Renderman_Export_Object(FILE *outfile, ObjectInstancePtr obj)
{
	fprintf(outfile, "# %s\n", obj->o_label);

	/* Save attributes. */
	fprintf(outfile, "AttributeBegin\n");

	Renderman_Export_Transform(obj->o_transform);
	Renderman_Export_Attributes(outfile, (AttributePtr)obj->o_attribs);

	switch ( obj->o_parent->b_class )
	{
		case sphere_obj:
			fprintf(outfile, "Sphere 1 -1 1 360\n");
			break;

		case cube_obj:
			fprintf(outfile,
					"Polygon \"P\" [ 1 1 1 -1 1 1 -1 -1 1 1 -1 1 ]\n");
			fprintf(outfile,
					"Polygon \"P\" [ 1 1 -1 1 -1 -1 -1 -1 -1 -1 1 -1 ]\n");
			fprintf(outfile,
					"Polygon \"P\" [ 1 1 1 1 1 -1 1 -1 -1 1 -1 1 ]\n");
			fprintf(outfile,
					"Polygon \"P\" [ -1 1 1 -1 -1 1 -1 -1 -1 -1 1 -1 ]\n");
			fprintf(outfile,
					"Polygon \"P\" [ 1 1 1 -1 1 1 -1 1 -1 1 1 -1 ]\n");
			fprintf(outfile,
					"Polygon \"P\" [ 1 -1 1 1 -1 -1 -1 -1 -1 -1 -1 1 ]\n");
			break;

		case cylinder_obj:
			fprintf(outfile, "Cylinder 1 -1 1 360\n");
			if ( ! ( ((AttributePtr)obj->o_attribs)->defined &&
				 	 ((AttributePtr)obj->o_attribs)->use_extension &&
				 	 ((AttributePtr)obj->o_attribs)->open ) )
			{
				fprintf(outfile, "Disk 1 1 360\n");
				fprintf(outfile, "Disk -1 1 360\n");
			}
			break;

		case cone_obj:
			fprintf(outfile, "Translate 0 0 -1\n");
			fprintf(outfile, "Cone 2 1 360\n");
			if ( ! ( ((AttributePtr)obj->o_attribs)->defined &&
				 	 ((AttributePtr)obj->o_attribs)->use_extension &&
				 	 ((AttributePtr)obj->o_attribs)->open ) )
				fprintf(outfile, "Disk 0 1 360\n");
			break;

		case plane_obj:
			fprintf(outfile,
					"Polygon \"P\" [ 10 10 0 -10 10 0 -10 -10 0 10 -10 0 ]\n");
			break;

		case square_obj:
			fprintf(outfile,
					"Polygon \"P\" [ 1 1 0 -1 1 0 -1 -1 0 1 -1 0 ]\n");
			break;

		case csg_obj:
			Renderman_Export_CSG_Object(outfile, obj->o_parent->b_csgptr);
			break;

		case wireframe_obj:
			Renderman_Export_Wireframe_Object(outfile,
											  obj->o_parent->b_major_wire);
			break;

		default:;
	}

	/* Restore attributes. */
	fprintf(outfile, "AttributeEnd\n\n");
}


static void
Renderman_Export_Instances(FILE *outfile, InstanceList instances)
{
	InstanceList	inst;

	for ( inst = instances ; inst ; inst = inst->next )
		if ( ! Obj_Is_Light(inst->the_instance) )
			Renderman_Export_Object(outfile, inst->the_instance);
}
