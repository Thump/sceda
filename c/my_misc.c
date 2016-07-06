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
**	animate.c : Functions for animating keyframes.
**
*/

#include <sys/types.h>
#include <sys/stat.h>

#include <math.h>
#include <sced.h>
#include <base_objects.h>
#include <View.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/SmeBSB.h>


extern void Edit_Menu_Callback(Widget, XtPointer, XtPointer);
extern BaseObjectList base_objects;
extern int num_base_objects;
extern void Close_CSG_Callback(Widget, XtPointer, XtPointer);
extern void Help_Popup_Callback(Widget, XtPointer, XtPointer);

extern char *rayshade_path;
extern char *rayshade_options;
extern char *povray_path;
extern char *povray_options;
extern char *radiance_path;
extern char *radiance_options;
extern char *renderman_path;
extern char *renderman_options;
extern char *genray_path;
extern char *genray_options;
extern char *genscan_path;
extern char *genscan_options;

/* the edit_shell buttons, referenced here in order to check their
** sensitivity.  We don't allow none but sensitive, '90s kinda 
** buttons 'round these parts...
*/
extern Widget edit_buttons[];

/* These are the main_view buttons, also referenced for sensitivity 
** checking.
*/
extern Widget main_option_buttons[];

/* Similarly, these are the csg_view buttons, also referenced for 
** sensitivity checking.
*/
extern Widget csg_option_buttons[];


Quaternion
QMul(Quaternion q1, Quaternion q2)
{	Quaternion r;
	double a1,a2,b1,b2,c1,c2,d1,d2;

	debug(FUNC_NAME,fprintf(stderr,"QMul()\n"));

	a1=q1.real_part; a2=q2.real_part;
	b1=q1.vect_part.x; b2=q2.vect_part.x;
	c1=q1.vect_part.y; c2=q2.vect_part.y;
	d1=q1.vect_part.z; d2=q2.vect_part.z;

	r.real_part=((a1*a2)-(b1*b2)-(c1*c2)-(d1*d2));
	r.vect_part.x=((a1*b2)+(b1*a2)+(c1*d2)-(d1*c2));
	r.vect_part.y=((a1*c2)-(b1*d2)+(c1*a2)+(d1*b2));
	r.vect_part.z=((a1*d2)+(b1*c2)-(c1*b2)+(d1*a2));

	return(r);
}


Matrix
Scale_To_Matrix(double sx, double sy, double sz)
{	Vector vec1,vec2,vec3;
	Matrix mat1;

	debug(FUNC_NAME,fprintf(stderr,"Scale_To_Matrix()\n"));

	VNew(sy,0.0,0.0,vec1);
	VNew(0.0,sz,0.0,vec2);
	VNew(0.0,0.0,sx,vec3);

	MNew(vec1,vec2,vec3,mat1);
	
	return(mat1);
}


Quaternion
AngleVector_To_Quaternion(double angle, Vector v)
{	Quaternion *q;

	debug(FUNC_NAME,fprintf(stderr,"AngleVector_To_Quaternion()\n"));

	q=(Quaternion *)EMalloc(sizeof(Quaternion));

	q->real_part=cos(angle/2*D2R);
	VScalarMul(v,sin(angle/2*D2R),q->vect_part);

	return(*q);
}


void
Edit_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	debug(FUNC_NAME,fprintf(stderr,"Edit_Accel()\n"));

	/* So, before we bang off an edit session, we gotta check if the 
	** edits are allowed, by either checking whether the edit buttons
	** on the main_view or csg_view (depending on whether this is a 
	** main edit or csg edit, are sensitive.  Problem is, the widgets for
	** either of these are stuffed up into {main,csg}_option_buttons, and 
	** we ** can't be zackly certain which one it is.  But these buttons are
	** currently only ever {de}sensitized on a bulk basis (in 
	** Sensitize_{Main,CSG}_Buttons()) so all we do here is check the 
	** sensitivity of the first one, {main,csg}_option_buttons[0].
	*/
	
	if (!strcmp("Main",*s))
	{
		if ( ! XtIsSensitive(main_option_buttons[0]) ) return ;
		Edit_Objects_Function(NULL,&main_window,NULL);
	}
	else if (!strcmp("CSG",*s))
	{
		if ( ! XtIsSensitive(csg_option_buttons[0]) ) return ;
		Edit_Objects_Function(NULL,&csg_window,NULL);
	}
	else
		fprintf(stderr,"Uh-oh, edit accel called on %s\n",*s);
}


void
unEdit_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	debug(FUNC_NAME,fprintf(stderr,"Edit_Accel()\n"));

	/* edit_buttons[] is the array of buttons defined for the edit shell:
	** here we're checking the status of the finish button.  If it's
	** not sensitive, we're not allowed to exit the edit menu yet.
	*/
	if (! XtIsSensitive(edit_buttons[0])) return;

	Edit_Shell_Finished(NULL,NULL,NULL);
}


void
Viewfrom_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	debug(FUNC_NAME,fprintf(stderr,"Viewfrom_Accel()\n"));

	/* If neither the edit menu nor the main view buttons nor the 
	** the csg buttons are sensitive, then disallow any viewport changes.
	** An instance of this is when you're changing eye distance or
	** something.
	*/

	if (!strcmp("Main",*s))
	{	/* Yeah, yeah, it's ugly, so sue me.  This reads: if there 
		** main_view buttons are sensitive, or we're editing an object
		** and the edit buttons are sensitive, then we're allowed to
		** start a viewpoint change.
		*/
		if ( (XtIsSensitive(main_option_buttons[0]))
			|| 
			 ( (main_window.current_state & edit_object)
			  &&
			   (XtIsSensitive(edit_buttons[0]))
		     )
		   )
				Initiate_Viewfrom_Change(NULL,&main_window,NULL);
	}
	else if (!strcmp("CSG",*s))
	{	/* This reads similar to the above: if the csg_view buttons
		** are sensitive, or we're editing an object and the edit 
		** buttons are sensitive, then we're allowed to start a 
		** viewpoint change.
		*/
		if ( (XtIsSensitive(csg_option_buttons[0]))
			|| 
			 ( (csg_window.current_state & edit_object)
			  &&
			   (XtIsSensitive(edit_buttons[0]))
		     )
		   )
				Initiate_Viewfrom_Change(NULL,&csg_window,NULL);
	}
	else
		fprintf(stderr,"Uh-oh, viewfrom accel called on %s\n",*s);
}


void
Apply_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	debug(FUNC_NAME,fprintf(stderr,"Apply_Accel()\n"));

	if ( ! XtIsSensitive(main_window.apply_button) ) return;

	Apply_Button_Callback(NULL,NULL,NULL);
}


void
Recall_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	debug(FUNC_NAME,fprintf(stderr,"Recall_Accel()\n"));

	if (!strcmp("Main",*s))
	{	/* This reads: if the main_view buttons are sensitive, or 
		** we're editing an object and the edit buttons are sensitive, 
		** then we're allowed to do a view recall.
		*/
		if ( (XtIsSensitive(main_option_buttons[0]))
			|| 
			 ( (main_window.current_state & edit_object)
			  &&
			   (XtIsSensitive(edit_buttons[0]))
		     )
		   )
				View_Recall_Callback(NULL,&main_window,NULL);
	}
	else if (!strcmp("CSG",*s))
	{	/* This reads: if the csg_view buttons are sensitive, or 
		** we're editing an object and the edit buttons are sensitive, 
		** then we're allowed to do a view recall.
		*/

		if ( (XtIsSensitive(csg_option_buttons[0]))
			|| 
		 	 ( (csg_window.current_state & edit_object)
		  	  &&
		   	   (XtIsSensitive(edit_buttons[0]))
	     	 )
	   	   )
				View_Recall_Callback(NULL,&csg_window,NULL);
	}
	else
		fprintf(stderr,"Uh-oh, recall accel called on %s\n",*s);
}


void
Zoom_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	int curmag,inc;
	WindowInfoPtr window=NULL;

	debug(FUNC_NAME,fprintf(stderr,"Zoom_Accel()\n"));

	if (*num!=2 || (strcmp(*s,"Main") && strcmp(*s,"CSG"))) return;

	inc=atoi(s[1]);
		
	if (!strcmp("Main",s[0]))
	{	/* This reads: if the main_view buttons are sensitive, or 
		** we're editing an object and the edit buttons are sensitive, 
		** then we're allowed to do a zoom.
		*/
		if ( (XtIsSensitive(main_option_buttons[0]))
			|| 
			 ( (main_window.current_state & edit_object)
			  &&
			   (XtIsSensitive(edit_buttons[0]))
		     )
		   )
				window=&main_window;
	}
	else if (!strcmp("CSG",s[0]))
	{	/* This reads: if the csg_view buttons are sensitive, or 
		** we're editing an object and the edit buttons are sensitive, 
		** then we're allowed to do a zoom.
		*/

		if ( (XtIsSensitive(csg_option_buttons[0]))
			|| 
			 ( (csg_window.current_state & edit_object)
			  &&
			   (XtIsSensitive(edit_buttons[0]))
			 )
	   	   )
				window=&csg_window;
	}
	else
		fprintf(stderr,"Uh-oh, recall accel called on %s\n",s[0]);

	/* If window is still NULL by this point, either we're not allowed
	** to be doing zooms at this point, or we weren't passed the right
	** string.  Return and keep quiet, 'cus I'm too lazy to do this right...
	*/
	if (window==NULL) return;
	
	XtVaGetValues(window->view_widget,XtNmagnification,&curmag,NULL);
	if (curmag+inc < 0) return;
	XtVaSetValues(window->view_widget,XtNmagnification,curmag+inc,NULL);

	View_Update(window,window->all_instances,CalcScreen);
	Update_Projection_Extents(window->all_instances);
}


void
Pan_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	WindowInfoPtr window;

	debug(FUNC_NAME,fprintf(stderr,"Pan_Accel()\n"));

	if (!strcmp("Main",s[0]))
	{	/* This reads: if the main_view buttons are sensitive, or 
		** we're editing an object and the edit buttons are sensitive, 
		** then we're allowed to do a pan.
		*/
		if ( (XtIsSensitive(main_option_buttons[0]))
			|| 
			 ( (main_window.current_state & edit_object)
			  &&
			   (XtIsSensitive(edit_buttons[0]))
		     )
		   )
				Initiate_Pan_Change(NULL,&main_window,NULL);
	}
	else if (!strcmp("CSG",s[0]))
	{	/* This reads: if the csg_view buttons are sensitive, or 
		** we're editing an object and the edit buttons are sensitive, 
		** then we're allowed to do a pan.
		*/

		if ( (XtIsSensitive(csg_option_buttons[0]))
			|| 
			 ( (csg_window.current_state & edit_object)
			  &&
			   (XtIsSensitive(edit_buttons[0]))
			 )
	   	   )
				Initiate_Pan_Change(NULL,&csg_window,NULL);
	}
	else
		fprintf(stderr,"Uh-oh, pan accel called on %s\n",s[0]);
}


void
LA_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	WindowInfoPtr window;

	debug(FUNC_NAME,fprintf(stderr,"LA_Accel()\n"));

	if (!strcmp("Main",s[0]))
	{	/* This reads: if the main_view buttons are sensitive, or 
		** we're editing an object and the edit buttons are sensitive, 
		** then we're allowed to change our lookat point.
		*/
		if ( (XtIsSensitive(main_option_buttons[0]))
			|| 
			 ( (main_window.current_state & edit_object)
			  &&
			   (XtIsSensitive(edit_buttons[0]))
		     )
		   )
				Change_Lookat_Callback(NULL,&main_window,NULL);
	}
	else if (!strcmp("CSG",s[0]))
	{	/* This reads: if the csg_view buttons are sensitive, or 
		** we're editing an object and the edit buttons are sensitive, 
		** then we're allowed to change our lookat point.
		*/

		if ( (XtIsSensitive(csg_option_buttons[0]))
			|| 
			 ( (csg_window.current_state & edit_object)
			  &&
			   (XtIsSensitive(edit_buttons[0]))
			 )
	   	   )
				Change_Lookat_Callback(NULL,&csg_window,NULL);
	}
	else
		fprintf(stderr,"Uh-oh, lookat accel called on %s\n",s[0]);
}


void
New_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	WindowInfoPtr window;

	debug(FUNC_NAME,fprintf(stderr,"New_Accel()\n"));

	if (!strcmp("Main",s[0]))
	{	if (XtIsSensitive(main_option_buttons[0]))
			New_Object_Popup_Callback(NULL,&main_window,NULL);
	}
	else if (!strcmp("CSG",s[0]))
	{	if (XtIsSensitive(csg_option_buttons[0]))
			New_Object_Popup_Callback(NULL,&csg_window,NULL);
	}
	else
		fprintf(stderr,"Uh-oh, new accel called on %s\n",s[0]);
}


void
Quit_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	WindowInfoPtr window;

	debug(FUNC_NAME,fprintf(stderr,"Quit_Accel()\n"));

	if (XtIsSensitive(main_option_buttons[0]))
		Quit_Dialog_Func(NULL,NULL,NULL);
}


void
Close_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	WindowInfoPtr window;

	debug(FUNC_NAME,fprintf(stderr,"Quit_Accel()\n"));

	if (XtIsSensitive(csg_option_buttons[0]))
		Close_CSG_Callback(NULL,NULL,NULL);
}


/* This is quick quit
*/
void
QQuit_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	WindowInfoPtr window;

	debug(FUNC_NAME,fprintf(stderr,"QQuit_Accel()\n"));

	if (XtIsSensitive(main_option_buttons[0]))
		Quit_Func(NULL,NULL,NULL);
}


/* This is the help screen
*/
void
Help_Accel(Widget w, XEvent *e, String *s, Cardinal *num)
{	WindowInfoPtr window;

	debug(FUNC_NAME,fprintf(stderr,"Quit_Accel()\n"));

	if (XtIsSensitive(main_option_buttons[0]))
		Help_Popup_Callback(NULL,NULL,NULL);
}


/* This function returns true if there is any object with the name
** 'label' existing in keframes kfnumber+1 and beyond.  Used to 
** ensure uniqueness of name for scene merging.
*/
Boolean
ObjectLabel_Exists(char *label,int kfnumber)
{	KeyFrame *kfptr;
	InstanceList objptr;
	ObjectInstance *obj;

	debug(FUNC_NAME,fprintf(stderr,"ObjectLabel_Exists()\n"));

	kfptr=Nth_KeyFrame(kfnumber,key_frames);

	for (; kfptr!=NULL; kfptr=kfptr->next)
		for (objptr=kfptr->all_instances; objptr!=NULL; objptr=objptr->next)
			if (!strcmp(label,objptr->the_instance->o_label))
				return(TRUE);

	return(FALSE);
}
			

void
Free_LabelMap(LabelMapPtr map)
{	LabelMapPtr ptr1,ptr2;

	debug(FUNC_NAME,fprintf(stderr,"Free_LabelMap()\n"));

	ptr2=map;

	while (ptr2!=NULL)
	{	ptr1=ptr2; ptr2=ptr1->next;

		free(ptr1->old);
		free(ptr1->new);
		free(ptr1);
	}
}


LabelMapPtr
Add_LabelMap(LabelMapPtr map, String old, String new)
{	LabelMapPtr ptr;

	debug(FUNC_NAME,fprintf(stderr,"Add_LabelMap()\n"));

	if (map==NULL)
	{	map=(LabelMapPtr)EMalloc(sizeof(LabelMap));
		ptr=map;
	}
	else
	{	ptr=map;
		while(ptr->next!=NULL) ptr=ptr->next;
		ptr->next=(LabelMapPtr)EMalloc(sizeof(LabelMap));
		ptr=ptr->next;
	}


	ptr->next=NULL;

	ptr->old=(String)EMalloc(strlen(old));
	strcpy(ptr->old,old);

	ptr->new=(String)EMalloc(strlen(new));
	strcpy(ptr->new,new);

	return(map);
}


String
Find_LabelMap(LabelMapPtr map, String label)
{	LabelMapPtr ptr;

	debug(FUNC_NAME,fprintf(stderr,"Find_LabelMap()\n"));

	for(ptr=map; ptr!=NULL; ptr=ptr->next)
		if (!strcmp(ptr->old,label))
			return(ptr->new);

	return(NULL);
}
