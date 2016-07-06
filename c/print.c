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
**	print.c : a variety of functions for printing out the contents
**            of various structures: debugging purposes only.
**
*/

#include <sced.h>
#include <View.h>
#include <select_point.h>

#include <load.h>

#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Label.h>


void
Print_XPoint(XPoint pt)
{	fprintf(stderr,"x:%hd  y:%hd\n",pt);
}


void
Print_Instance(ObjectInstancePtr elmt)
{	fprintf(stderr,"Instance '%s' at %p\n",elmt->o_label,elmt);
}


void
Print_InstanceList(InstanceList list)
{	InstanceList ptr;
	
	for (ptr=list; ptr!=NULL; ptr=ptr->next)
	{	Print_Instance(ptr->the_instance);
		fprintf(stderr,"\tprev: %p\n",ptr->prev);
		fprintf(stderr,"\tnext: %p\n",ptr->next);
	}
}


void
Print_Vector(Vector *v)
{	fprintf(stderr,"%f %f %f\n",v->x,v->y,v->z);
}


void
Print_Matrix(Matrix *m)
{	Print_Vector(&m->x);
	Print_Vector(&m->y);
	Print_Vector(&m->z);
	fprintf(stderr,"\n");
}


void
Print_KeyFrame(KeyFrame *kf)
{	fprintf(stderr,"all_instances: %p\n",kf->all_instances);
	fprintf(stderr,"selected_instances: %p\n",kf->selected_instances);
	fprintf(stderr,"edit_instances: %p\n",kf->edit_instances);
	fprintf(stderr,"kfnumber: %d\n",kf->kfnumber);
	fprintf(stderr,"fnumber: %d\n",kf->fnumber);
	fprintf(stderr,"edit_menu: %p\n",kf->edit_menu);
	fprintf(stderr,"next: %p\n",kf->next);
	fprintf(stderr,"prev: %p\n",kf->prev);
}


void
Print_Quaternion(Quaternion *q)
{	fprintf(stderr,"real: %f vector: ",q->real_part);
	Print_Vector(&q->vect_part);
}

void
Print_FeatureType(FeatureType *f)
{	switch(*f)
	{	case null_feature:
    		fprintf(stderr,"null_feature\n"); break;
    	case plane_feature:
    		fprintf(stderr,"plane_feature\n"); break;
    	case line_feature:
    		fprintf(stderr,"line_feature\n"); break;
    	case point_feature:
    		fprintf(stderr,"point_feature\n"); break;
    	case inconsistent_feature:
    		fprintf(stderr,"inconsistent_feature\n"); break;
    	case midplane_feature:
    		fprintf(stderr,"midplane_feature\n"); break;
    	case midpoint_feature:
    		fprintf(stderr,"midpoint_feature\n"); break;
    	case axis_plane_feature:
    		fprintf(stderr,"axis_plane_feature\n"); break;
    	case axis_feature:
    		fprintf(stderr,"axis_feature\n"); break;
    	case orig_line_feature:
    		fprintf(stderr,"orig_line_feature\n"); break;
    	case ref_line_feature:
    		fprintf(stderr,"ref_line_feature\n"); break;
    	case orig_plane_feature:
    		fprintf(stderr,"orig_plane_feature\n"); break;
    	case ref_plane_feature:
    		fprintf(stderr,"ref_plane_feature\n"); break;
		default:
			fprintf(stderr,"unknown feature type\n"); break;
	}
}


void
Print_FeatureSpec(FeatureSpecifier *f)
{	fprintf(stderr,"spec_type: ");
	Print_FeatureSpecType(&f->spec_type);

	if (f->spec_object!=NULL)
		fprintf(stderr,"obj: %p %s\n",f->spec_object,f->spec_object->o_label);

	fprintf(stderr,"spec_vector: ");
	Print_Vector(&f->spec_vector);
}


void
Print_FeatureSpecType(FeatureSpecType *f)
{	switch(*f)
	{	case absolute_spec:
			fprintf(stderr,"absolute_spec\n");
		case offset_spec:
			fprintf(stderr,"offset_spec\n");
		case reference_spec: 
			fprintf(stderr,"reference_spec\n");
		case origin_spec:
			fprintf(stderr,"origin_spec\n");
		case ref_point_spec:
			fprintf(stderr,"ref_point_spec\n");
	}
}


void
Print_FeatureData(FeatureData *f)
{	int i,j;

	fprintf(stderr,"f_type: ");
	Print_FeatureType(&f->f_type);

	if (f->f_label!=NULL)
		fprintf(stderr,"f_label: %s\n",f->f_label);

	fprintf(stderr,"f_vector: ");
	Print_Vector(&f->f_vector);
	
	fprintf(stderr,"f_point: ");
	Print_Vector(&f->f_point);

	fprintf(stderr,"f_value: %f\n",f->f_value);

	fprintf(stderr,"f_spec_type: ");
	Print_FeatureType(&f->f_spec_type);

	j=Num_RefFeatures(f->f_spec_type);
	for (i=0; i<j; i++)
	{	fprintf(stderr,"f_spec[%d of %d]\n",i+1,j);
		Print_FeatureSpec(&f->f_specs[i]);
	}
}


void
Print_ScaleCons(ObjectInstance *obj)
{	int i;

	for (i=0; i<obj->o_scale_num; i++)
	{	if (obj->o_scale_active[i+3]==TRUE)
		{	fprintf(stderr,"\nscale constraint %d for %s\n",i+3,obj->o_label);
			Print_FeatureData(&obj->o_scale_cons[i]);
		}
	}
}


void
Print_SelObjectScale()
{	ObjectInstance *obj;

	if (main_window.selected_instances==NULL) return;

	obj=main_window.selected_instances->the_instance;

	Print_ScaleCons(obj);
}


void
PrintSetMag(Widget w, XtPointer cl, XtPointer ca)
{	int mag;

	XtVaGetValues(main_window.view_widget,XtNmagnification,&mag, NULL);
	fprintf(stderr,"screen mag %d viewport mag %d\n",
		mag,main_window.viewport.magnify);
	XtVaSetValues(main_window.view_widget,XtNmagnification,mag+1,NULL);
	main_window.viewport.magnify=mag+1;
	View_Update(&main_window,main_window.all_instances,CalcView);
}

void
Print_Raytracer(Raytracer *type)
{	switch (*type)
    {
		case NoTarget: fprintf(stderr,"NoTarget\n"); break;
		case POVray: fprintf(stderr,"POVray\n"); break;
		case Rayshade: fprintf(stderr,"Rayshade\n"); break;
		case Radiance: fprintf(stderr,"Radiance\n"); break;
		case Genray: fprintf(stderr,"Genray\n"); break;
		case Genscan: fprintf(stderr,"Genscan\n"); break;
		case Renderman: fprintf(stderr,"Renderman\n"); break;
		default: fprintf(stderr,"I don't know what the fuck I am...\n"); break;
	}
}

void
Print_GenericObj(GenericObject *gen)
{	switch (*gen)
	{
		case cube_obj: fprintf(stderr,"cube_obj\n"); break;
		case sphere_obj: fprintf(stderr,"sphere_obj\n"); break;
		case cylinder_obj: fprintf(stderr,"cylinder_obj\n"); break;
		case cone_obj: fprintf(stderr,"cone_obj\n"); break;
		case square_obj: fprintf(stderr,"square_obj\n"); break;
		case plane_obj: fprintf(stderr,"plane_obj\n"); break;
		case light_obj: fprintf(stderr,"light_obj\n"); break;
		case spotlight_obj: fprintf(stderr,"spotlight_obj\n"); break;
		case arealight_obj: fprintf(stderr,"arealight_obj\n"); break;
		case csg_obj: fprintf(stderr,"csg_obj\n"); break;
		case wireframe_obj: fprintf(stderr,"wireframe_obj\n"); break;
		default: fprintf(stderr,"I don't know what the fuck I am...\n"); break;
	}
}


static Boolean dialog_done;
static Boolean dialog_cancel;


static void
Get_String_Done(Widget w, XtPointer cl, XtPointer ca)
{
	dialog_cancel = cl ? FALSE : TRUE;
	dialog_done = TRUE;
}


void
Get_String_Return(Widget w, XEvent *e, String *s, Cardinal *num)
{
	Get_String_Done(w, (XtPointer)TRUE, NULL);
}


char *
Get_String(char *prompt,char *val)
{
	static Widget	get_string = NULL;
	static Widget	gs_dialog = NULL;
	XtAppContext	context;
	XEvent			event;

    Arg     args[3];
    int     n;

	if ( prompt == NULL || val == NULL)
		return(NULL);

	n = 0;
	XtSetArg(args[n], XtNtitle, "Get_String");	n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
	get_string = XtCreatePopupShell("get_stringTopLevel", 
		transientShellWidgetClass,main_window.shell, args, n);

	n = 0;
	XtSetArg(args[n],XtNlabel,prompt);	n++;
	gs_dialog = XtCreateManagedWidget("get_stringDialog",
		dialogWidgetClass,get_string,args,n);

	XawDialogAddButton(gs_dialog,"Done",Get_String_Done,(XtPointer)TRUE);
	XawDialogAddButton(gs_dialog,"Cancel",Get_String_Done,(XtPointer)FALSE);

	XtOverrideTranslations(get_string,
		XtParseTranslationTable(":<Key>Return: Get_String_Return()"));

	XtRealizeWidget(get_string);

	XtVaSetValues(gs_dialog,XtNvalue,val,NULL);

	SFpositionWidget(get_string);
	XtPopup(get_string, XtGrabExclusive);

	dialog_done = FALSE;
	dialog_cancel = FALSE;

	context = XtWidgetToApplicationContext(main_window.shell);
	while ( ! dialog_done )
	{
		XtAppNextEvent(context, &event);
		XtDispatchEvent(&event);
	}

	XtPopdown(get_string);

	if ( dialog_cancel )
		return (NULL);
	else
		return (XawDialogGetValueString(gs_dialog));
}


void
Print_Token(int token)
{	switch (token)
	{	case EOF_TOKEN: fprintf(stderr,"EOF_TOKEN\n"); break;
		case STRING: fprintf(stderr,"STRING\n"); break;
		case INT: fprintf(stderr,"INT\n"); break;
		case FLOAT: fprintf(stderr,"FLOAT\n"); break;
		case MAINVIEW: fprintf(stderr,"MAINVIEW\n"); break;
		case CSGVIEW: fprintf(stderr,"CSGVIEW\n"); break;
		case LOOKFROM: fprintf(stderr,"LOOKFROM\n"); break;
		case LOOKAT: fprintf(stderr,"LOOKAT\n"); break;
		case LOOKUP: fprintf(stderr,"LOOKUP\n"); break;
		case VIEWDIST: fprintf(stderr,"VIEWDIST\n"); break;
		case EYEDIST: fprintf(stderr,"EYEDIST\n"); break;
		case BASEOBJECTS: fprintf(stderr,"BASEOBJECTS\n"); break;
		case INSTANCES: fprintf(stderr,"INSTANCES\n"); break;
		case WIREFRAME: fprintf(stderr,"WIREFRAME\n"); break;
		case ATTRIBUTES: fprintf(stderr,"ATTRIBUTES\n"); break;
		case TRANSFORMATION: fprintf(stderr,"TRANSFORMATION\n"); break;
		case COLOUR: fprintf(stderr,"COLOUR\n"); break;
		case DIFFUSE: fprintf(stderr,"DIFFUSE\n"); break;
		case SPECULAR: fprintf(stderr,"SPECULAR\n"); break;
		case TRANSPARENCY: fprintf(stderr,"TRANSPARENCY\n"); break;
		case REFLECT: fprintf(stderr,"REFLECT\n"); break;
		case REFRACT: fprintf(stderr,"REFRACT\n"); break;
		case CAMERA: fprintf(stderr,"CAMERA\n"); break;
		case NONE: fprintf(stderr,"NONE\n"); break;
		case RAYSHADE: fprintf(stderr,"RAYSHADE\n"); break;
		case POVRAY: fprintf(stderr,"POVRAY\n"); break;
		case GENRAY: fprintf(stderr,"GENRAY\n"); break;
		case HFOV: fprintf(stderr,"HFOV\n"); break;
		case VFOV: fprintf(stderr,"VFOV\n"); break;
		case UP: fprintf(stderr,"UP\n"); break;
		case RIGHT: fprintf(stderr,"RIGHT\n"); break;
		case SCREEN: fprintf(stderr,"SCREEN\n"); break;
		case MAGNIFY: fprintf(stderr,"MAGNIFY\n"); break;
		case LIGHT: fprintf(stderr,"LIGHT\n"); break;
		case AMBIENT: fprintf(stderr,"AMBIENT\n"); break;
		case POSITION: fprintf(stderr,"POSITION\n"); break;
		case REFERENCE: fprintf(stderr,"REFERENCE\n"); break;
		case DEPENDENTS: fprintf(stderr,"DEPENDENTS\n"); break;
		case CONSTRAINTS: fprintf(stderr,"CONSTRAINTS\n"); break;
		case PLANE: fprintf(stderr,"PLANE\n"); break;
		case LINE: fprintf(stderr,"LINE\n"); break;
		case POINT: fprintf(stderr,"POINT\n"); break;
		case ACTIVE: fprintf(stderr,"ACTIVE\n"); break;
		case LAYER: fprintf(stderr,"LAYER\n"); break;
		case SCALE: fprintf(stderr,"SCALE\n"); break;
		case ROTATE: fprintf(stderr,"ROTATE\n"); break;
		case AXES: fprintf(stderr,"AXES\n"); break;
		case ORIGIN: fprintf(stderr,"ORIGIN\n"); break;
		case ALLIGN: fprintf(stderr,"ALLIGN\n"); break;
		case SCENEDIR: fprintf(stderr,"SCENEDIR\n"); break;
		case UNION: fprintf(stderr,"UNION\n"); break;
		case INTERSECTION: fprintf(stderr,"INTERSECTION\n"); break;
		case DIFFERENCE: fprintf(stderr,"DIFFERENCE\n"); break;
		case DEFAULT: fprintf(stderr,"DEFAULT\n"); break;
		case MID: fprintf(stderr,"MID\n"); break;
		case MAJOR: fprintf(stderr,"MAJOR\n"); break;
		case MINOR: fprintf(stderr,"MINOR\n"); break;
		case VIEWPORT: fprintf(stderr,"VIEWPORT\n"); break;
		case DENSE: fprintf(stderr,"DENSE\n"); break;
		case FULL: fprintf(stderr,"FULL\n"); break;
		case CSG: fprintf(stderr,"CSG\n"); break;
		case TARGET: fprintf(stderr,"TARGET\n"); break;
		case INCLUDES: fprintf(stderr,"INCLUDES\n"); break;
		case COMPRESS: fprintf(stderr,"COMPRESS\n"); break;
		case RADIANCE: fprintf(stderr,"RADIANCE\n"); break;
		case DECLARE: fprintf(stderr,"DECLARE\n"); break;
		case EXTEND: fprintf(stderr,"EXTEND\n"); break;
		case OPEN: fprintf(stderr,"OPEN\n"); break;
		case VERS: fprintf(stderr,"VERS\n"); break;
		case NORMAL: fprintf(stderr,"NORMAL\n"); break;
		case UNKNOWN: fprintf(stderr,"UNKNOWN\n"); break;
		case GENSCAN: fprintf(stderr,"GENSCAN\n"); break;
		case INTERNAL: fprintf(stderr,"INTERNAL\n"); break;
		case OBJECT: fprintf(stderr,"OBJECT\n"); break;
		case MATRIX: fprintf(stderr,"MATRIX\n"); break;
		case MODE: fprintf(stderr,"MODE\n"); break;
		case RENDERMAN: fprintf(stderr,"RENDERMAN\n"); break;
		case ANI_ROT: fprintf(stderr,"ANI_ROT\n"); break;
		case ANI_SCALE: fprintf(stderr,"ANI_SCALE\n"); break;
		default: fprintf(stderr,"I dunno...\n");
	}
}


void
Print_Token_Value(int token,int lex_int,float lex_float)
{	switch (token)
	{	case INT: fprintf(stderr,"%d\n",lex_int); break;
		case FLOAT: fprintf(stderr,"%f\n",lex_float); break;
		case STRING: fprintf(stderr,"%s\n",lex_string); break;
	}
}


void
Print_Flags(int flags)
{	
	if (flags & ObjVisible)
		fprintf(stderr,"ObjVisible ");
	if (flags & ObjSelected)
		fprintf(stderr,"ObjSelected ");
	if (flags & ObjDepends)
		fprintf(stderr,"ObjDepends ");
	if (flags & ObjPending)
		fprintf(stderr,"ObjPending ");

	fprintf(stderr,"\n");
}


void
Maintenance(Widget w, XEvent *e, String *s, Cardinal *num)
{	ObjectInstance *obj;

	fprintf(stderr,"Maintenance()\n");

	Print_InstanceList(main_window.edit_instances);
	fprintf(stderr,"\n");
}
