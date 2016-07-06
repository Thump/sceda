#define PATCHLEVEL 0
/*
**    ScEdA: A Constraint Based Scene Editor/Animator.
**    Copyright (C) 1994  Denis McLaughlin (denism@cyberus.ca)
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
**	copy_obj.c : Functions for copying instance lists
**
*/

#include <sced.h>
#include <base_objects.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/SmeBSB.h>

extern int num_base_objects;


InstanceList
Copy_InstanceList(InstanceList copylist,InstanceList otherlist)
{	InstanceList	ptr,new,last,newlist=NULL;
	
	debug(FUNC_NAME,fprintf(stderr,"Copy_InstanceList()\n"));

	/* step through all objects on copylist: for each object, create a copy
    ** and add it to newlist, then return newlist
    */

	for ( ptr=copylist ; ptr!=NULL ; ptr=ptr->next )
	{	
		debug(FUNC_VAL,fprintf(stderr,"\t%s\n",ptr->the_instance->o_label));

		new=(InstanceList)EMalloc(sizeof(InstanceListElmt));
		new->the_instance=Copy_ObjectInstance(ptr->the_instance);
		new->next=NULL;

		if (newlist==NULL)
		{	newlist=new;
			last=newlist;
			new->prev=NULL;
		}
		else
		{	last->next=new;
			new->prev=last;
			last=new;
		}
	}
	debug(FUNC_VAL,fprintf(stderr,"\tObjects done\n\n"));
	Update_FeaturePtrs(newlist,otherlist);
	Update_DependentList(newlist,otherlist);

	return(newlist);
}

/*  This function copies everything about an object into a new object,
**  it more thoroughly creates new objects than does Copy_Objects_Callback.
*/
ObjectInstancePtr
Copy_ObjectInstance(ObjectInstancePtr old)
{	ObjectInstancePtr new;
	int j,i;

	debug(FUNC_NAME,fprintf(stderr,"Copy_ObjectInstance()\n"));

	new=(ObjectInstancePtr)EMalloc(sizeof(ObjectInstance));

	/* Copy the label, adding 1 for good luck. */
	new->o_label=(char *)EMalloc(strlen(old->o_label)+1);
	strcpy(new->o_label,old->o_label);

	/* Here we directly equate all simple data types and structures. */
	new->o_layer=old->o_layer;
	new->o_num_vertices=old->o_num_vertices;
	new->o_num_faces=old->o_num_faces;
	new->o_origin_num=old->o_origin_num;
	new->o_scale_num=old->o_scale_num;
	new->o_rotate_num=old->o_rotate_num;
	new->o_num_depend=old->o_num_depend;
	new->o_flags=old->o_flags;
	new->o_dfs_mark=old->o_dfs_mark;
	new->o_posspline=NULL;
	new->o_rotspline=NULL;
	new->o_transform=old->o_transform;
	new->o_inverse=old->o_inverse;
	new->o_axes=old->o_axes;
	new->o_reference=old->o_reference;
	new->o_origin=old->o_origin;
	new->o_proj_extent=old->o_proj_extent;
	new->o_rot=old->o_rot;
	new->o_scale=old->o_scale;

	/* I'm not sure just copying is sufficient for o_parent, but for
	** the moment, I'm not gonna play with it.
	**
	** Actually, that's not exactly true: Create_Instance() just makes
	** pointers to the original parent, so I think I'll do the same here.
	** Plus incrementing the parent's child count and adding the new
	** child to the parent's child list.
	*/
	new->o_parent=old->o_parent;
	object_count[new->o_parent->b_class]++;
	Add_Instance_To_Base(new,new->o_parent);

	Copy_FeatureData(&(old->o_major_align),&(new->o_major_align));
	Copy_FeatureData(&(old->o_minor_align),&(new->o_minor_align));

	new->o_world_verts=Copy_Vector(old->o_world_verts,old->o_num_vertices);
	new->o_normals=Copy_Vector(old->o_normals,old->o_num_faces);
	new->o_main_verts=Copy_Vertex(old->o_main_verts,old->o_num_vertices);

	/* 'Kay, the deal's this: origin, scale and rotate all have 3 
	** constraints (as distinct from _active_ constraints: constraints 
	** being the set of defined constraints, active constraints being that 
	** subset of the constraints which are currently active) that are 
	** default.  In addition, scale has another constraint (Uniform) which 
	** is added when the edit menu is popped up.  The 3 defaults for each of
	** scale/rot/origin _don't_ appear in the constraint list, but scale's
	** fourth constraint (Uniform) does.  So, the first three elements
	** in the boolean array that indicate which constraints are active
	** actually refer to constraints that _don't_ appear in the object's
	** constraint list: they're the implied defaults.  Confusing?  Yup.
	*/
	new->o_origin_active=Copy_Boolean(old->o_origin_active,old->o_origin_num+3);
	new->o_scale_active=Copy_Boolean(old->o_scale_active,old->o_scale_num+3);
	new->o_rotate_active=Copy_Boolean(old->o_rotate_active,old->o_rotate_num+3);

	new->o_origin_cons=Copy_FeaturePtr(old->o_origin_cons,old->o_origin_num);
	new->o_scale_cons=Copy_FeaturePtr(old->o_scale_cons,old->o_scale_num);
	new->o_rotate_cons=Copy_FeaturePtr(old->o_rotate_cons,old->o_rotate_num);

	/* 'Kay, so here's where it's at: I thought that o_wireframe was a
	** a pointer to the wireframe of the object.  Turns out, o_wireframe
	** is really a pointer to the wireframe of the objects parent, -unless-
	** the object has had it's wireframe made denser or less dense, in
	** which case it's really a separate wireframe.  So, we check if the
	** objects wireframe pointer points to it's parent's wireframe: if it
	** does, we just preserve that new wireframe.  If it doesn't, we create
	** a new wireframe for it.  'Kay?  Kay.
	*/
	if (old->o_wireframe != old->o_parent->b_wireframe)
		new->o_wireframe=Copy_Wireframe(old->o_wireframe);
	else
		new->o_wireframe=old->o_wireframe;

	/*  For the moment we just preserve a pointer to the old list of
	** dependents: once we have the new list fully constructed, we'll
	** reprocess this and set up links to the new instances of the
	** dependency.
	*/
	new->o_dependents=Copy_DependentList(old->o_dependents,old->o_num_depend);

	if (old->o_parent->b_class == light_obj ||
		old->o_parent->b_class == spotlight_obj ||
		old->o_parent->b_class == arealight_obj )
		new->o_attribs=Copy_LightInfo(old->o_attribs);
	else
		new->o_attribs=Copy_Attribute(old->o_attribs);

	return(new);
}


/* Creates a copy of num array elements of type DependentList: does
** nothing about resetting the pointers: this is done in 
** Update_DependentList
*/
DependentList
Copy_DependentList(DependentList old,short num)
{	DependentList new;
	int i;

	debug(FUNC_NAME,fprintf(stderr,"Copy_DependentList()\n"));

	new=(DependentList)EMalloc(sizeof(Dependent)*num);

	for (i=0; i<num; i++)
	{	new[i].obj=old[i].obj;
		new[i].count=old[i].count;
	}
	return(new);
}


/* This copies the contents of a single FeatureData element to
** another.  Both new and old must already exist.
*/
void
Copy_FeatureData(FeatureData *old, FeatureData *new)
{	int i,j;

	debug(FUNC_NAME,fprintf(stderr,"Copy_FeatureData()\n"));

	debug(FUNC_VAL,fprintf(stderr,"old: %p new: %p\n",old->f_label,new));

	if (old->f_type==null_feature)
	{	new->f_type=null_feature;
		return;
	}

	if (old->f_label!=NULL)
	{	new->f_label=(char *)EMalloc(strlen(old->f_label)+1);
		strcpy(new->f_label,old->f_label);
	}

	new->f_type=old->f_type;
	new->f_status=old->f_status;
	new->f_vector=old->f_vector;
	new->f_point=old->f_point;
	new->f_value=old->f_value;
	new->f_spec_type=old->f_spec_type;


	/* The number of elements in the f_specs array to be copied
	** depends on the f_spec_type: Num_RefFeatures() returns the
	** the appropriate number.
	*/

	i=Num_RefFeatures(new->f_spec_type);

	for (j=0; j<i; j++)
	{	new->f_specs[j].spec_type=old->f_specs[j].spec_type;

		/* For the moment we preserve the pointer to the reference
		** object in the old list: once we have all the new objects
		** in the new list, we'll go and reset them if needed.
		*/

		if (new->f_specs[j].spec_type == reference_spec )
			new->f_specs[j].spec_object=old->f_specs[j].spec_object;
		else
			new->f_specs[j].spec_object=NULL;
		new->f_specs[j].spec_vector=old->f_specs[j].spec_vector;
	}
}


/* This creates a copy of an array of num FeatureData elements.
*/
FeaturePtr
Copy_FeaturePtr(FeaturePtr old,short num)
{	FeaturePtr new;
	int i,j,k;

	debug(FUNC_NAME,fprintf(stderr,"Copy_FeaturePtr()\n"));

	if (old==NULL)
		return(NULL);

	new=(FeaturePtr)EMalloc(sizeof(FeatureData)*num);

	for (i=0; i<num; i++)
		Copy_FeatureData(&(old[i]),&(new[i]));

	return(new);
}


/* This is used to create a linked list of pointers to a separate list
** of objects: all is the master list of objects, list is the linked 
** list of pointers to the objects.
**
** The reason we need to have an all list, is because we want to 
** construct a pointer list of objects in all, and the objects being 
** pointed to by list may not be the same as the objects in all.
*/
InstanceList
Copy_InstancePtrList(InstanceList list, InstanceList all)
{	InstanceList ptr;
	InstanceList new=NULL;
	InstanceListElmt *newptr;
	InstanceListElmt *elmt;

	debug(FUNC_NAME,fprintf(stderr,"Copy_InstancePtrList()\n"));

	for (ptr=list; ptr!=NULL; ptr=ptr->next)
	{	elmt=(InstanceListElmt *)EMalloc(sizeof(InstanceListElmt));
		elmt->the_instance=
			Object_From_Label(all,NULL,ptr->the_instance->o_label);
		elmt->next=NULL;
		elmt->prev=NULL;

		if (new==NULL)
		{	new=elmt;
			newptr=elmt;
		}
		else
		{	elmt->prev=newptr;
			newptr->next=elmt;
			newptr=elmt;
		}
	}
	return(new);
}


/* Create a copy of an array of vectors.
*/
Vector
*Copy_Vector(Vector *v1,int n)
{	Vector *new;

	debug(FUNC_NAME,fprintf(stderr,"Copy_Vector()\n"));

	new=(Vector *)EMalloc(sizeof(Vector)*n);
	memcpy(new,v1,sizeof(Vector)*n);
	return(new);
}


/* Create a copy of an array of matrices.
*/
Matrix
*Copy_Matrix(Matrix *m1,int n)
{	Matrix *new;

	debug(FUNC_NAME,fprintf(stderr,"Copy_Matrix()\n"));

	new=(Matrix *)EMalloc(sizeof(Matrix)*n);
	memcpy(new,m1,sizeof(Matrix)*n);
	return(new);
}


/* Creates a copy of an array of vertices.
*/
Vertex
*Copy_Vertex(Vertex *m1,int n)
{	Vertex *new;
	
	debug(FUNC_NAME,fprintf(stderr,"Copy_Vertex()\n"));

	new=(Vertex *)EMalloc(sizeof(Vertex)*n);
	memcpy(new,m1,sizeof(Vertex)*n);
	return(new);
}


/* Creates a copy of an array of booleans.
*/
Boolean
*Copy_Boolean(Boolean *m1,int n)
{	Boolean *new;

	debug(FUNC_NAME,fprintf(stderr,"Copy_Boolean()\n"));

	new=(Boolean *)EMalloc(sizeof(Boolean)*n);
	memcpy(new,m1,sizeof(Boolean)*n);
	return(new);
}


/* Creates a copy of a single wireframe.
*/
Wireframe
*Copy_Wireframe(Wireframe *old)
{	Wireframe *new;
	int i;

	debug(FUNC_NAME,fprintf(stderr,"Copy_Wireframe()\n"));

	new=(Wireframe *)EMalloc(sizeof(Wireframe));

	new->num_vertices=old->num_vertices;
	new->vertices=Copy_Vector(old->vertices,old->num_vertices);
	
	if (old->vertex_normals!=NULL)
	{	fprintf(stderr,"Doing this, yupyup\n");
		new->vertex_normals=Copy_Vector(old->vertex_normals,old->num_vertices);
	}
	else
		new->vertex_normals=NULL;

	new->num_faces=old->num_faces;
	new->faces=Copy_Face(old->faces,new->num_faces);

	new->num_attribs=old->num_attribs;
	new->attribs=Copy_AttributePtr(old->attribs,new->num_attribs);

	return(new);
}


/* Creates a copy of an array of faces.
*/
FacePtr
Copy_Face(FacePtr old,int num)
{	FacePtr new;
	int i,j;

	debug(FUNC_NAME,fprintf(stderr,"Copy_Face()\n"));

	new=(FacePtr)EMalloc(sizeof(Face)*num);

	for (i=0; i<num; i++)
	{	new[i].num_vertices=old[i].num_vertices;
		new[i].vertices=(int *)EMalloc(sizeof(int)*new[i].num_vertices);
		memcpy(new[i].vertices,old[i].vertices,sizeof(int)*new[i].num_vertices);
		new[i].normal=old[i].normal;
		new[i].draw=old[i].draw;
		new[i].face_attribs=Copy_Attribute(old[i].face_attribs);
	}
	return(new);
}


AttributePtr *
Copy_AttributePtr(AttributePtr *old,int num)
{	AttributePtr *new;
	int i;

	debug(FUNC_NAME,fprintf(stderr,"Copy_AttributePtr()\n"));

	new=(AttributePtr *)EMalloc(sizeof(AttributePtr)*num);

	for (i=0; i<num; i++)
		new[i]=Copy_Attribute(old[i]);
	return(new);
}


/* Creates a copy of an attribute structure.
*/
AttributePtr
Copy_Attribute(AttributePtr old)
{	AttributePtr new;

	debug(FUNC_NAME,fprintf(stderr,"Copy_Attribute()\n"));

	if (old==NULL)
		return(NULL);

	new=(AttributePtr)EMalloc(sizeof(Attributes));

	if (old->extension!=NULL)
	{	new->extension=(char *)EMalloc(strlen(old->extension)+1);
		strcpy(new->extension,old->extension);
	}
	else
		new->extension=NULL;

	new->defined=old->defined;
	new->colour=old->colour;
	new->diff_coef=old->diff_coef;
	new->spec_coef=old->spec_coef;
	new->spec_power=old->spec_power;
	new->reflect_coef=old->reflect_coef;
	new->transparency=old->transparency;
	new->refract_index=old->refract_index;
	new->use_extension=old->use_extension;
	new->open=old->open;
	new->use_obj_trans=old->use_obj_trans;

	return(new);
}


/* Creates a copy of a lightinfo structure
*/
LightInfo *
Copy_LightInfo(LightInfo *old)
{	LightInfo *new;

	debug(FUNC_NAME,fprintf(stderr,"Copy_LightInfo()\n"));

	if (old==NULL)
		return(NULL);

	new=(LightInfo *)EMalloc(sizeof(LightInfo));

	new->red=old->red;
	new->green=old->green;
	new->blue=old->blue;
	new->val1=old->val1;
	new->val2=old->val2;
	new->flag=old->flag;

	return(new);
}


/* Creates a copy of an edge: this is obsolete code from 0.61: I don't
** know if it will be needed in future or not, so I'm leaving it in.
*/
EdgePtr
Copy_Edge(EdgePtr f)
{	EdgePtr new;

	debug(FUNC_NAME,fprintf(stderr,"Copy_Edge()\n"));

	new=(EdgePtr)EMalloc(sizeof(EdgeElmt));
	/* new->drawn=f->drawn; */
	/* new->p=f->p; */
	/* new->q=f->q; */

	return(new);
}


/* This returns a pointer to the first object instance in list
** which bears the name label.
*/
ObjectInstancePtr
Object_From_Label(InstanceList list,InstanceList otherlist,String label)
{	InstanceList	ptr;

	debug(FUNC_NAME,fprintf(stderr,"Object_From_Label()\n"));
	debug(FUNC_VAL,fprintf(stderr,"object %p %s\n",label,label));

	for (ptr=list; ptr!=NULL; ptr=ptr->next)
		if (!strcmp(ptr->the_instance->o_label,label))
			return(ptr->the_instance);

	for (ptr=otherlist; ptr!=NULL; ptr=ptr->next)
		if (!strcmp(ptr->the_instance->o_label,label))
			return(ptr->the_instance);

	if (!strcmp(label,main_window.axes.o_label))
		return(&main_window.axes);

	/* fprintf(stderr,"Uh-oh, couldn't convert %s to object\n",label); */

	return(NULL);
}


/* This specifies the number of f_specs in the 3 element array in the
** FeatureData type that are actually used: based on the value of
** f_spec_type.
*/
int
Num_RefFeatures(FeatureType type)
{ 	int i;

	debug(FUNC_NAME,fprintf(stderr,"Num_RefFeatures()\n"));
	debug(FUNC_VAL,Print_FeatureType(&type));
	
	switch(type)
	{	case axis_plane_feature:
		case plane_feature:
		case orig_plane_feature:
		case ref_plane_feature:
			i=3; break;
		case midplane_feature:
		case midpoint_feature:
		case line_feature:
		case axis_feature:
		case ref_line_feature:
		case orig_line_feature:
			i=2; break;
		case point_feature:
			i=1; break;
	}
	return(i);
}


/*  There are several points in the object lists in which an object uses
** a pointer to another object.  We cannot determine the value of this
** pointer while creating the new list, because the Nth object to be
** copied may reference the (N+1)th object.  To work around this, we copy
** the pointer to the old object list, and then, once we have copied all
** objects into the new list, we go through the new list, resetting the
** pointers from the old list into the new list.
**
**  It's magic, it works, believe.
*/
void
Update_FeaturePtrs(InstanceList list,InstanceList otherlist)
{	InstanceList	ptr;
	int i;

	debug(FUNC_NAME,fprintf(stderr,"Update_FeaturePtrs()\n"));

	for (ptr=list; ptr!=NULL; ptr=ptr->next )
	{	debug(FUNC_VAL,fprintf(stderr,"Update_FeaturePtrs: %s\n",
			ptr->the_instance->o_label));

		for (i=0; i<ptr->the_instance->o_origin_num; i++)
			Update_FeatureSpec(list,otherlist,
				&ptr->the_instance->o_origin_cons[i]);

		for (i=0; i<ptr->the_instance->o_scale_num; i++)
			Update_FeatureSpec(list,otherlist,
				&ptr->the_instance->o_scale_cons[i]);

		for (i=0; i<ptr->the_instance->o_rotate_num; i++)
			Update_FeatureSpec(list,otherlist,
				&ptr->the_instance->o_rotate_cons[i]);

		Update_FeatureSpec(list,otherlist,
			&(ptr->the_instance->o_major_align));

		Update_FeatureSpec(list,otherlist,
			&(ptr->the_instance->o_minor_align));
	}
}


/* This resets the dependent list for the new list of objects.
*/
void
Update_DependentList(InstanceList list,InstanceList otherlist)
{	InstanceList ptr;
	DependentList dep;
	int i;

	debug(FUNC_NAME,fprintf(stderr,"Update_DependentList()\n"));

	for (ptr=list; ptr!=NULL; ptr=ptr->next)
	{	dep=ptr->the_instance->o_dependents;
		for (i=0; i<ptr->the_instance->o_num_depend; i++)
			dep[i].obj=Object_From_Label(list,otherlist,
				dep[i].obj->o_label);
	}
}


/* This resets the dependent list for the new list of objects.
*/
void
Update_FeatureSpec(InstanceList list,InstanceList otherlist, FeaturePtr obj)
{	int i,j;
	FeaturePtr fp;

	debug(FUNC_NAME,fprintf(stderr,"Update_FeatureSpec()\n"));

	if (obj!=NULL && obj->f_type!=null_feature)
	{	debug(FUNC_VAL,fprintf(stderr,"f_label: %s\n",obj->f_label));
		i=Num_RefFeatures(obj->f_spec_type);
		for (j=0; j<i; j++)
			if (obj->f_specs[j].spec_type==reference_spec)
				obj->f_specs[j].spec_object=Object_From_Label(list,
				  	otherlist,obj->f_specs[j].spec_object->o_label);
	}
}


void
Reset_BaseObjectInstances(BaseObjectPtr *base,InstanceList list)
{	InstanceList	ptr;
	int i,j;

	debug(FUNC_NAME,fprintf(stderr,"Reset_BaseObjectInstances()\n"));

	for (i=0; i<num_base_objects; i++)
	{	free(base[i]->b_instances);
		base[i]->b_num_instances=0;
		
		/* it's inefficient and ugly to do this twice, but hey... */
		for ( ptr=list ; ptr!=NULL ; ptr=ptr->next )
			if (!strcmp(base[i]->b_label,ptr->the_instance->o_label))
				base[i]->b_num_instances++;

		base[i]->b_instances=(struct _ObjectInstance **) 
			EMalloc(sizeof(struct _ObjectInstance *)*base[i]->b_num_instances);

		base[i]->b_num_slots=base[i]->b_num_instances;

		j=0;	
		for (ptr=list; ptr!=NULL; ptr=ptr->next)
			if (!strcmp(base[i]->b_label,ptr->the_instance->o_label))
			{	base[i]->b_instances[j]=(struct _ObjectInstance  *)ptr;
				j++;
			}
	}
}
				

void
Free_InstanceList(InstanceList freelist)
{	InstanceList	ptr;
	
	debug(FUNC_NAME,fprintf(stderr,"Free_InstanceList()\n"));

	for ( ptr=freelist ; ptr!=NULL ; ptr=ptr->next )
	{	
		Destroy_Instance(ptr->the_instance);
	}
}


void
Free_FeatureLabel(FeaturePtr f,int num)
{	int i;

	for (i=0; i<num; i++)
		if (f[i].f_label != NULL) free(f[i].f_label);
}


void
Free_Deps(ObjectInstance *obj)
{
	Free_FeatureLabel(obj->o_origin_cons,obj->o_origin_num);
	Free_FeatureLabel(obj->o_scale_cons,obj->o_scale_num);
	Free_FeatureLabel(obj->o_rotate_cons,obj->o_rotate_num);

	if (obj->o_origin_cons != NULL) free(obj->o_origin_cons);
	if (obj->o_scale_cons != NULL) free(obj->o_scale_cons);
	if (obj->o_rotate_cons != NULL) free(obj->o_rotate_cons);

	if (obj->o_origin_active != NULL) free(obj->o_origin_active);
	if (obj->o_scale_active != NULL) free(obj->o_scale_active);
	if (obj->o_rotate_active != NULL) free(obj->o_rotate_active);

	if (obj->o_major_align.f_label != NULL) free(obj->o_major_align.f_label);
	if (obj->o_minor_align.f_label != NULL) free(obj->o_minor_align.f_label);

	if (obj->o_dependents != NULL) free(obj->o_dependents);
}
