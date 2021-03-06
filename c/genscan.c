/*
**    ScEd: A Constraint Based Scene Editor.
**    Copyright (C) 1994  Stephen Chenney (stephen@cs.su.oz.au)
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
**	genscan.c: Export and preview functions for genscan.
*/

#include <math.h>
#include <sced.h>
#include <base_objects.h>
#include <csg.h>
#include <hash.h>
#include <time.h> 

#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif

static int	Export_Camera(FILE*, Camera);
static int	Export_Light(FILE*, ObjectInstancePtr);
static int	Export_Instances(FILE*, InstanceList, ObjectInstancePtr);

/*
** Heres some really kludgy variables which make maintaining code real
** fun :). What happens is that I need a count of the number of vertices
** in the entire world before I start outputting any of the object
** data. So rather than writing any clever couting routines, I'm just
** going to have two passes through the export routines.. if 'counting'
** is set to True then all the wireframes are created but instead of
** actually exporting the data, the count of their vertices is added
** to 'count'. Export_Genscan() is the only entry point into the
** functions in this module, which then only calls Export_Instances(),
** so we can ensure that the variables get proper initial values.
*/

static int counting;
static int count;

/*	int
**	Export_Genscan(FILE *outfile, ScenePtr scene)
**	Exports all the relevant info into outfile.
*/

int
Export_Genscan(FILE *outfile, ScenePtr scene)
{
	time_t	current_time;

	time(&current_time);

	if ( fprintf(outfile, "/*File generated by sced %s at ",VERSION) < 0 || 
		 fprintf(outfile, "%s */\n", ctime(&current_time)) < 0 ||
		 Export_Camera(outfile, scene->camera) < 0 ||
		 fprintf(outfile, "/* Ambient light */\n") < 0 || 
		 fprintf(outfile, "%f %f %f\n",
				(double)scene->ambient.red / (double)MAX_UNSIGNED_SHORT,
				(double)scene->ambient.green / (double)MAX_UNSIGNED_SHORT,
				(double)scene->ambient.blue / (double)MAX_UNSIGNED_SHORT) < 0 ||
		 Export_Instances(outfile, scene->instances,scene->light) < 0 )
	{
		Popup_Error("Write failed!", main_window.shell, "Error");
		return 0;
	}

	return 1;
}

static int
Export_Camera(FILE *outfile, Camera cam)
{
	/* genscan takes all its camera info at the top of the file. */
	fprintf(outfile, "/* Camera */\n");
	VPrint(outfile, cam.location);
	VPrint(outfile, cam.look_at);
	VPrint(outfile, cam.look_up);
	fprintf(outfile, "%1.15g\n", cam.eye_dist);
	fprintf(outfile, "%f %f\n", cam.window_right, cam.window_up);
	fprintf(outfile, "%d %d\n", cam.scr_width, cam.scr_height);

	return 1;
}


static int
Export_Light(FILE *outfile, ObjectInstancePtr light)
{
	if ( ! light ) return 1;

	switch ( light->o_parent->b_class )
	{
		case light_obj:
			VPrint(outfile, light->o_transform.displacement);
			fprintf(outfile, "%f %f %f\n",
				((LightInfoPtr)light->o_attribs)->red,
				((LightInfoPtr)light->o_attribs)->green,
				((LightInfoPtr)light->o_attribs)->blue);
			break;

		case spotlight_obj:
		case arealight_obj:
			fprintf(stderr,"Spot and area lights not supported!\n");
			break;
		default:
			break;
	}

	return 1;
}



static int
Export_Polyhedral_Object(FILE *outfile, WireframePtr wire, char *name,
						 AttributePtr def_attribs)
{
	int		i, j;

	if(counting)
	{
		count += wire->num_vertices;
		return 1;
	}

	fprintf(outfile, "/* Vertices - %s */\n", name);
	fprintf(outfile, " %d\n",wire->num_vertices);

	for(i=0; i<wire->num_vertices; i++)
		VPrint(outfile,wire->vertices[i]);

	/* export the faces */

	fprintf(outfile, "/* Faces */\n");
	fprintf(outfile, " %d\n",wire->num_faces);

	for ( i=0 ; i<wire->num_faces ; i++ )
	{
		int e1,e2,last;

		if ( wire->faces[i].num_vertices < 3 )
			continue;

		last = wire->faces[i].vertices[0];

		for(j=1; j<wire->faces[i].num_vertices; j++)
		{
			e1 = last;
			e2 = wire->faces[i].vertices[j];

			fprintf(outfile,"%d %d ",e1,e2);

			last = e2;
		}

		fprintf(outfile,"%d %d ",last,wire->faces[i].vertices[0]);

		fprintf(outfile,"-1 ");

		VPrint(outfile,wire->faces[i].normal);

		if(wire->faces[i].face_attribs && wire->faces[i].face_attribs->defined)
		{
			fprintf(outfile, "%1.5g %1.5g %1.5g %1.5g %1.5g %1.5g\n",
					wire->faces[i].face_attribs->colour.red /
					(double)MAX_UNSIGNED_SHORT,
					wire->faces[i].face_attribs->colour.green /
					(double)MAX_UNSIGNED_SHORT,
					wire->faces[i].face_attribs->colour.blue /
					(double)MAX_UNSIGNED_SHORT,
					wire->faces[i].face_attribs->diff_coef,
					wire->faces[i].face_attribs->spec_coef,
					wire->faces[i].face_attribs->spec_power);
		}
		else if ( def_attribs && def_attribs->defined )
		{
			fprintf(outfile, "%1.5g %1.5g %1.5g %1.5g %1.5g %1.5g\n",
					def_attribs->colour.red / (double)MAX_UNSIGNED_SHORT,
					def_attribs->colour.green / (double)MAX_UNSIGNED_SHORT,
					def_attribs->colour.blue / (double)MAX_UNSIGNED_SHORT,
					def_attribs->diff_coef,
					def_attribs->spec_coef,
					def_attribs->spec_power);
		}
		else
			fprintf(outfile, "%1.5g %1.5g %1.5g %1.5g %1.5g %1.5g\n",0.5,0.5,0.5,0.1,0.3,20.0);
	}

	return 1;
}

static int
Export_Object(FILE *outfile, ObjectInstancePtr obj)
{
	WireframePtr	wireframe = Object_To_Wireframe(obj, FALSE);

	Export_Polyhedral_Object(outfile, wireframe, obj->o_label, obj->o_attribs);

	Wireframe_Destroy(wireframe);

	return 1;
}


static int
Export_Plane(FILE *outfile, ObjectInstancePtr plane)
{
	WireframePtr	wireframe = New(Wireframe, 1);
	int				i;
	Vector			temp_v;

	/* Build a dummy wireframe. */
	wireframe->num_vertices = 4;
	wireframe->vertices = New(Vector, 4);
	VNew(10, 10, 0, wireframe->vertices[0]);
	VNew(-10, 10, 0, wireframe->vertices[1]);
	VNew(-10, -10, 0, wireframe->vertices[2]);
	VNew(10, -10, 0, wireframe->vertices[3]);
	wireframe->num_faces = 1;
	wireframe->faces = New(Face, 1);
	wireframe->faces[0].num_vertices = 4;
	wireframe->faces[0].vertices = New(int, 4);
	for ( i = 0 ; i < 4 ; i++ )
		wireframe->faces[0].vertices[i] = i;
	wireframe->faces[0].face_attribs = NULL;
	wireframe->vertex_normals = NULL;
	wireframe->num_attribs = 0;
	wireframe->attribs = NULL;

	for ( i = 0 ; i < 4 ; i++ )
	{
		MVMul(plane->o_transform.matrix, wireframe->vertices[i], temp_v);
		VAdd(temp_v, plane->o_transform.displacement, wireframe->vertices[i]);
	}

	Export_Polyhedral_Object(outfile, wireframe, plane->o_label,
							 plane->o_attribs);

	Wireframe_Destroy(wireframe);

	return 1;
}




static int
Export_Instances(FILE *outfile, InstanceList insts, ObjectInstancePtr light)
{
	InstanceList		inst_elmt;
	ObjectInstancePtr	inst;
	int 				numobj,numlight,numvert;

	counting = 1;
	count = 0;

	/* count the number of lights and objects */

	for ( inst_elmt = insts, numobj=0, numlight=0 ; inst_elmt != NULL ; inst_elmt = inst_elmt->next )
	{
		inst = inst_elmt->the_instance;

		switch ( inst->o_parent->b_class )
		{
			case cube_obj:
			case square_obj:
			case sphere_obj:
			case cylinder_obj:
			case cone_obj:
			case csg_obj:
			case wireframe_obj:
				numobj++;
				Export_Object(outfile, inst);
				break;

			case plane_obj:
				numobj++;
				Export_Plane(outfile, inst);
				break;

			case light_obj:
			case spotlight_obj:
			case arealight_obj:
				numlight++;
				break;

			default:;
		}
	}

	if(light != NULL)
		numlight++;

	/* print the lighting information */

	fprintf(outfile, "/* Number of lights */\n");
	fprintf(outfile, " %d\n", numlight);

	Export_Light(outfile, light);

	for ( inst_elmt = insts ; inst_elmt != NULL ; inst_elmt = inst_elmt->next )
	{
		inst = inst_elmt->the_instance;

		if ( inst->o_parent->b_class == spotlight_obj ||
			 inst->o_parent->b_class == arealight_obj ||
			 inst->o_parent->b_class == light_obj)
			Export_Light(outfile, inst);
	}

	fprintf(outfile, "/* Number of objects */\n");
	fprintf(outfile, " %d\n", numobj);

	/* yuck! count the number of vertices we will be exporting */

	numvert = count;

	fprintf(outfile, "/* Total number of vertices */\n");
	fprintf(outfile, " %d\n", numvert);

	/* actually output the objects */

	counting = 0;

	for ( inst_elmt = insts ; inst_elmt != NULL ; inst_elmt = inst_elmt->next )
	{
		inst = inst_elmt->the_instance;

		switch ( inst->o_parent->b_class )
		{
			case cube_obj:
			case square_obj:
			case sphere_obj:
			case cylinder_obj:
			case cone_obj:
			case csg_obj:  
			case wireframe_obj:
				Export_Object(outfile, inst);
				break;

			case plane_obj:
				Export_Plane(outfile, inst);
				break;

			case light_obj:
			case spotlight_obj:
			case arealight_obj:
				break;

			default:;
		}
	}

	return 1;
}
