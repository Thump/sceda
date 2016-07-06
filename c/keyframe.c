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

extern Widget main_option_buttons[];



void
Next_KeyFrame(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *current;

	debug(FUNC_NAME,fprintf(stderr,"Next_KeyFrame()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	if (current->next!=NULL)
		Update_from_KeyFrame(current->next,TRUE);
	else
		Update_from_KeyFrame(key_frames,TRUE);
}


void
Prev_KeyFrame(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *current;

	debug(FUNC_NAME,fprintf(stderr,"Prev_KeyFrame()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	if (current->prev!=NULL)
		Update_from_KeyFrame(current->prev,TRUE);
	else
		Update_from_KeyFrame(key_frames,TRUE);
}


/* This routine takes a frame number and sets the current keyframe to be 
** either the already existing keyframe of that frame number, or the
** newly created keyframe inserted appropriately in the keyframe sequence.
** The routine returns TRUE or FALSE, depending on whether it created a
** keyframe (TRUE) or not (FALSE).
**
** If insert is false, we've been told to attempt to find an existing
** keyframe that matches the frame number.  If insert is true, we're
** not to look for an existing match, but rather insert one immediately
** after the current keyframe.
*/
Boolean
New_KeyFrame(int fnum, Boolean insert)
{	KeyFrame *kfptr,*before=NULL,*new=NULL;
	Boolean newframe=FALSE;

	debug(FUNC_NAME,fprintf(stderr,"New_KeyFrame()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	changed_scene = TRUE;

	before=Nth_KeyFrame(main_window.kfnumber,key_frames);

	if ( !insert)
	{
		/* Loop across all keyframes, looking for an exact match, or looking
		** for the last keyframe with a frame number not larger than the
		** desired one.
		*/
		for (kfptr=key_frames; kfptr!=NULL; kfptr=kfptr->next)
		{	if (kfptr->fnumber==fnum)
			{	new=kfptr;
				break;
			}
	
			if ( (kfptr->next == NULL) || (kfptr->next->fnumber > fnum) )
			{	before=kfptr;
				break;
			}
		}
	}
	
	/* We exited the for loop with either new or before set to something.
	** Or, alternatively, we never entered the for loop at all.
	** If new is set to something, that means we found an existing match:
	** we set that to be the current keyframe and return FALSE.
	*/
	if (new != NULL)
	{	Update_from_KeyFrame(new,TRUE);
		return(FALSE);
	}
	
	/* If new is still NULL, meaning we didn't find an exact match, then
	** before is set to the keyframe just prior to slot in which we have
	** to insert a new keyframe.
	*/
	new=(KeyFrame *)EMalloc(sizeof(KeyFrame));

	new->all_instances=NULL;
	new->selected_instances=NULL;
	new->edit_instances=NULL;

	new->kfnumber=(before->kfnumber)+1;
	new->fnumber=fnum;

	new->magnification=100;

	new->prev=before;
	new->next=before->next;
	if (before->next!=NULL)
		(before->next)->prev=new;
	before->next=new;

	Renumber_KeyFrames(key_frames);

	Update_from_KeyFrame(new,TRUE);
	/* Update_to_KeyFrame(new); */

	return(TRUE);
}


void
Clone_KeyFrame(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *current,*new,*ptr;

	debug(FUNC_NAME,fprintf(stderr,"Clone_KeyFrame()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	changed_scene = TRUE;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	New_KeyFrame(main_window.fnumber+1,TRUE);

	/* we here assign new to be the keyframe corresponding to the current 
	** keyframe because New_KeyFrame() sets main_window to correspond to
    ** the newly created keyframe.
	*/
	new=Nth_KeyFrame(main_window.kfnumber,key_frames);

	/* Update_from_KeyFrame(current,TRUE); */

	new->all_instances=
		Copy_InstanceList(current->all_instances,NULL);

	new->selected_instances=
		Copy_InstancePtrList(current->selected_instances,new->all_instances);
	new->edit_instances=
		Copy_InstancePtrList(current->edit_instances,new->all_instances);

	new->kfnumber=current->kfnumber+1;
	new->fnumber=current->fnumber+1;

	memcpy(&(new->viewport),&(current->viewport),sizeof(Viewport));

	XtVaGetValues(main_window.view_widget,XtNmagnification,
		&(new->magnification), NULL);

	for (ptr=new->next; ptr!=NULL; ptr=ptr->next)
		ptr->fnumber=ptr->fnumber+1;

	Update_from_KeyFrame(new,TRUE);
}


void
Remove_KeyFrame(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *current,*kfptr,*new;
	int diff;
	InstanceList victims,ptr;

	debug(FUNC_NAME,fprintf(stderr,"Remove_KeyFrame()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	if (Count_KeyFrames(key_frames)==1)	return;

	changed_scene = TRUE;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	victims=Copy_InstanceList(current->all_instances,current->all_instances);

	for (ptr=victims; ptr!=NULL; ptr=ptr->next)
		DeleteObj_from_KeyFrame(current,ptr->the_instance->o_label);

	Free_InstanceList(victims);

	if (current->prev==NULL) /* if we're deleting the first keyframe... */
	{	key_frames=current->next;
		key_frames->prev=NULL;
		new=key_frames;
	}
	else if (current->next==NULL) /* or if it's the last keyframe... */
	{	(current->prev)->next=NULL;
		new=current->prev;
	}
	else  /* ...or if it's any old keyframe at all... */
	{	(current->prev)->next=(current->next);
		(current->next)->prev=(current->prev);
		new=current->prev;
	}

	/* renumber the frames, with the understanding that deleting a
	** keyframe also deletes the interval immediately following it.
	*/
	if (current->next!=NULL)
	{	diff=((current->next)->fnumber)-(current->fnumber);
		for (kfptr=current->next; kfptr!=NULL; kfptr=kfptr->next)
			kfptr->fnumber-=diff;
	}

	Free_InstanceList(current->all_instances);
	Free_InstanceList(current->selected_instances);
	Free_InstanceList(current->edit_instances);

	free(current);

	Renumber_KeyFrames(key_frames);
	Update_from_KeyFrame(new,TRUE);
}


/* This updates the saved keyframe with the current values in the 
** main_window structure.  Called before most keyframe manipulations,
** incase a new set of objects has been selected, edited or instanced.
*/
void
Update_to_KeyFrame(KeyFrame *key_frame)
{	int i;

	debug(FUNC_NAME,fprintf(stderr,"Update_to_KeyFrame()\n"));

	key_frame->all_instances=main_window.all_instances;
	key_frame->selected_instances=main_window.selected_instances;
	key_frame->edit_instances=main_window.edit_instances;
	key_frame->fnumber=main_window.fnumber;
	XtVaGetValues(main_window.view_widget,XtNmagnification,
		&(key_frame->magnification), NULL);

	for (i=0; i<main_window.edit_menu->num_children; i++)
		XtDestroyWidget(main_window.edit_menu->children[i]);

	XtVaSetValues(main_window.edit_menu->button,XtNsensitive,FALSE,NULL);

	memcpy(&(key_frame->viewport),&(main_window.viewport),sizeof(Viewport));
}


void 
Update_from_KeyFrame(KeyFrame *key_frame, Boolean redisplay)
{	Arg	args[3];
	int n;
	InstanceList ptr;
	char name[20];
	Dimension width;

	debug(FUNC_NAME,fprintf(stderr,"Update_from_KeyFrame()\n"));
		
	main_window.all_instances=key_frame->all_instances;
	main_window.selected_instances=key_frame->selected_instances;
	main_window.edit_instances=key_frame->edit_instances;
	main_window.kfnumber=key_frame->kfnumber;
	main_window.fnumber=key_frame->fnumber;
	XtVaSetValues(main_window.view_widget,
		XtNmagnification,key_frame->magnification,NULL);
	
	memcpy(&(main_window.viewport),&(key_frame->viewport),sizeof(Viewport));

	main_window.edit_menu->num_children=0;

	for (ptr=main_window.edit_instances; ptr!=NULL; ptr=ptr->next)
	{	if (main_window.edit_menu->num_children == 
			main_window.edit_menu->max_children )
		{	main_window.edit_menu->max_children += 5;
			main_window.edit_menu->children = 
				More(main_window.edit_menu->children,
				Widget, main_window.edit_menu->max_children);
		}
	
		sprintf(name, "editEntry%d", main_window.edit_menu->num_children);
		n = 0;
		XtSetArg(args[n], XtNlabel, ptr->the_instance->o_label);   n++;
		main_window.edit_menu->children[main_window.edit_menu->num_children] =
			XtCreateManagedWidget(name, smeBSBObjectClass,
				main_window.edit_menu->menu, args, n);
		XtAddCallback(
		  main_window.edit_menu->children[main_window.edit_menu->num_children],
		  XtNcallback, Edit_Menu_Callback,(XtPointer)&main_window);

		XtVaSetValues(main_window.edit_menu->button,XtNsensitive,TRUE,NULL);
	
		main_window.edit_menu->num_children++;
	}

	sprintf(name,"%d of %d",main_window.kfnumber,Count_KeyFrames(key_frames));
	XtVaSetValues(main_window.kfnum_label,XtNlabel,name,NULL);
	width=Match_Widths(&(main_window.kf_label),1);
	XtVaSetValues(main_window.kfnum_label,XtNwidth,width,NULL);

	sprintf(name,"%d of %d",main_window.fnumber,Count_Frames(key_frames));
	XtVaSetValues(main_window.fnum_label,XtNlabel,name,NULL);
	width=Match_Widths(&(main_window.f_label),1);
	XtVaSetValues(main_window.fnum_label,XtNwidth,width,NULL);

	if (redisplay)
		View_Update(&main_window,main_window.all_instances,CalcView);
}


void
Renumber_KeyFrames(KeyFrameList key_frames)
{	KeyFrame *current;
	int count;

	debug(FUNC_NAME,fprintf(stderr,"Renumber_KeyFrames()\n"));

	current=key_frames;
	count=1;

	while (current!=NULL)
	{	current->kfnumber=count;
		count++;
		current=current->next;
	}
}


/* Calling Nth_KeyFrame(1,key_frames) will return the first key frame.
** ie. There is no zero'th key_frame, for this function.
*/
KeyFrame *
Nth_KeyFrame(int n, KeyFrameList key_frames)
{	KeyFrame *current;

	debug(FUNC_NAME,fprintf(stderr,"Nth_KeyFrame()\n"));
	
	current=key_frames;
	
	while (current!=NULL && current->kfnumber!=n)
		current=current->next;

	if (current==NULL)
		fprintf(stderr,"Help: couldn't find keyframe %d\n",n);

	return(current);
}	


KeyFrame *
Last_KeyFrame(KeyFrameList key_frames)
{	KeyFrame *current;

	debug(FUNC_NAME,fprintf(stderr,"Last_KeyFrame()\n"));
	
	current=key_frames;
	
	while (current->next!=NULL)
		current=current->next;

	return(current);
}	


int
Count_KeyFrames(KeyFrameList key_frames)
{	KeyFrame *ptr;
	int i=1;

	debug(FUNC_NAME,fprintf(stderr,"Count_KeyFrames()\n"));

	for (ptr=key_frames; ptr!=NULL; ptr=ptr->next)
		i++;

	return(i-1);
}


int
Count_Frames(KeyFrameList key_frames)
{	KeyFrame *ptr;

	debug(FUNC_NAME,fprintf(stderr,"Count_Frames()\n"));

	ptr=Last_KeyFrame(key_frames);
	return(ptr->fnumber);
}


void
Initialize_KeyFrames(KeyFrameList *key_frames)
{	*key_frames=(KeyFrame *)EMalloc(sizeof(KeyFrame));

	debug(FUNC_NAME,fprintf(stderr,"Initialize_KeyFrames()\n"));

	(*key_frames)->all_instances=NULL;
	(*key_frames)->selected_instances=NULL;
	(*key_frames)->edit_instances=NULL;
	(*key_frames)->next=NULL;
	(*key_frames)->prev=NULL;

	Renumber_KeyFrames(*key_frames);
}


void
PumpUp_All(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *ptr;
	int count;

	debug(FUNC_NAME,fprintf(stderr,"PumpUp_All()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	Update_to_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames));

	ptr=key_frames->next;
	count=1;

	for ( ; ptr!=NULL; ptr=ptr->next)
	{	ptr->fnumber=ptr->fnumber+count;
		count++;
		changed_scene = TRUE;
	}

	Update_from_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames),TRUE);
}

	
void
PumpDown_All(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *ptr;
	int count=1;

	debug(FUNC_NAME,fprintf(stderr,"PumpDown_All()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	Update_to_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames));

	/* Check to see that we can pump down all intervals */
	for (ptr=key_frames->next; ptr!=NULL; ptr=ptr->next)
		if (ptr->fnumber-1 == ptr->prev->fnumber)
			return;

	count=1;

	for (ptr=key_frames->next; ptr!=NULL; ptr=ptr->next)
	{	ptr->fnumber=ptr->fnumber-count;
		count++;
		changed_scene = TRUE;
	}

	Update_from_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames),TRUE);
}


void
PumpUp_Next(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *ptr,*current,*last;
	int count;

	debug(FUNC_NAME,fprintf(stderr,"PumpUp_Next()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	for (ptr=current->next; ptr!=NULL; ptr=ptr->next)
	{	changed_scene = TRUE;
		ptr->fnumber=ptr->fnumber+1;
	}

	Update_from_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames),TRUE);
}


void
PumpDown_Next(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *ptr,*current,*last;
	int count;

	debug(FUNC_NAME,fprintf(stderr,"PumpDown_Next()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	for (ptr=current->next ; ptr!=NULL; ptr=ptr->next)
	{	if (ptr->fnumber-1 > ptr->prev->fnumber)
		{	ptr->fnumber=ptr->fnumber-1;
			changed_scene = TRUE;
		}
		else
			break;
	}

	Update_from_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames),TRUE);
}


void
Clone_to_Current(Widget w, XEvent *e, String *s, Cardinal *num)
{	InstanceList ptr,new;
	char *label;

	debug(FUNC_NAME,fprintf(stderr,"Clone_to_Current()\n"));

	debug(FUNC_VAL,
		for (ptr=main_window.selected_instances; ptr!=NULL; ptr=ptr->next)
			fprintf(stderr,"Cloning %s\n",ptr->the_instance->o_label);
	);

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	changed_scene = TRUE;

	new=Copy_InstanceList(main_window.selected_instances,
		main_window.all_instances);

	for (ptr=new; ptr!=NULL; ptr=ptr->next)
	{	label=New(char, strlen(ptr->the_instance->o_label)+10);
		sprintf(label,"%s_%d",ptr->the_instance->o_label,
			object_count[ptr->the_instance->o_parent->b_class]);
		free(ptr->the_instance->o_label);
		ptr->the_instance->o_label=label;

		ptr->the_instance->o_flags&=(ObjAll ^ ObjSelected);
	}

	Append_Instance_List(&(main_window.all_instances),new);

	View_Update(&main_window,main_window.all_instances,CalcView|ViewNone);
}


void
Clone_to_Next(Widget w, XEvent *e, String *s, Cardinal *num)
{	InstanceList ptr,new;
	char *label;
	KeyFrame *current;

	debug(FUNC_NAME,fprintf(stderr,"Clone_to_Next()\n"));

	debug(FUNC_VAL,
		for (ptr=main_window.selected_instances; ptr!=NULL; ptr=ptr->next)
			fprintf(stderr,"Cloning to next %s\n",ptr->the_instance->o_label);
	);

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	if (current->next==NULL) return;

	new=Copy_InstanceList(main_window.selected_instances,
		current->next->all_instances);

	for (ptr=new; ptr!=NULL; ptr=ptr->next)
	{	ptr->the_instance->o_flags &= (ObjAll ^ ObjSelected);
		DeleteObj_from_KeyFrame(current->next,ptr->the_instance->o_label);
		changed_scene = TRUE;
	}

	Append_Instance_List(&(current->next->all_instances),new);

	Update_from_KeyFrame(current->next,TRUE);
	View_Update(&main_window,main_window.all_instances,CalcView|ViewNone);
}


void
Clone_to_Prev(Widget w, XEvent *e, String *s, Cardinal *num)
{	InstanceList ptr,new;
	char *label;
	KeyFrame *current;

	debug(FUNC_NAME,fprintf(stderr,"Clone_to_Prev()\n"));

	debug(FUNC_VAL,
		for (ptr=main_window.selected_instances; ptr!=NULL; ptr=ptr->next)
			fprintf(stderr,"Cloning to prev %s\n",ptr->the_instance->o_label);
	);

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	if (current->prev==NULL) return;

	new=Copy_InstanceList(main_window.selected_instances,
		current->prev->all_instances);

	for (ptr=new; ptr!=NULL; ptr=ptr->next)
	{	ptr->the_instance->o_flags &= (ObjAll ^ ObjSelected);
		DeleteObj_from_KeyFrame(current->prev,ptr->the_instance->o_label);
		changed_scene = TRUE;
	}

	Append_Instance_List(&(current->prev->all_instances),new);

	Update_from_KeyFrame(current->prev,TRUE);
	View_Update(&main_window,main_window.all_instances,CalcView|ViewNone);
}


void
Clone_to_Keyframes(Widget w, XEvent *e, String *s, Cardinal *num)
{	InstanceList ptr,new;
	char *label;
	KeyFrame *current,*kfptr;

	debug(FUNC_NAME,fprintf(stderr,"Clone_to_Keyframes()\n"));

	debug(FUNC_VAL,
		for (ptr=main_window.selected_instances; ptr!=NULL; ptr=ptr->next)
			fprintf(stderr,"Cloning to next %s\n",ptr->the_instance->o_label);
	);

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	kfptr=current->next;

	while (kfptr!=NULL)
	{	new=Copy_InstanceList(current->selected_instances,
			kfptr->all_instances);

		/* don't want them selected or duplicated */
		for (ptr=new; ptr!=NULL; ptr=ptr->next)
		{	ptr->the_instance->o_flags &= (ObjAll ^ ObjSelected);
			DeleteObj_from_KeyFrame(kfptr,ptr->the_instance->o_label);
			changed_scene = TRUE;
		}

		Append_Instance_List(&(kfptr->all_instances),new);

		Update_from_KeyFrame(kfptr,TRUE);
		View_Update(&main_window,main_window.all_instances,CalcView|ViewNone);

		kfptr=kfptr->next;
	}
}


/* This function deletes the set of selected objects from all keyframes
** forward of the current one.
*/
void
Delete_from_Keyframes(Widget w, XEvent *e, String *s, Cardinal *num)
{	InstanceList ptr,victim,victims;
	ObjectInstancePtr obj;
	char *label;
	KeyFrame *current,*kfptr,*last;

	debug(FUNC_NAME,fprintf(stderr,"Delete_from_Keyframes()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	last=Last_KeyFrame(key_frames);
	Update_to_KeyFrame(current);

	/* get a copy of the selected list of objects */
	victims=Copy_InstanceList
		(current->selected_instances,current->all_instances);

	/* for each keyframe, and each object on the select list */
	for (kfptr=last; kfptr!=current->prev; kfptr=kfptr->prev)
		for (ptr=victims; ptr!=NULL; ptr=ptr->next)
		{	DeleteObj_from_KeyFrame(kfptr,ptr->the_instance->o_label);
			View_Update
				(&main_window,main_window.all_instances,CalcView|ViewNone);
			changed_scene = TRUE;
		}

	Free_InstanceList(victims);
	Update_from_KeyFrame(current,TRUE);
	View_Update(&main_window,main_window.all_instances,CalcView|ViewNone);
}


void
DeleteObj_from_KeyFrame(KeyFrame *kfptr, char *label)
{	ObjectInstancePtr obj;
	InstanceList victim;

	debug(FUNC_NAME,fprintf(stderr,"DeleteObj_from_KeyFrame()\n"));

	/* load the keyframe */
	Update_from_KeyFrame(kfptr,FALSE);

	/* attempt to find this object */
	obj=Object_From_Label(main_window.all_instances,NULL,label);

	/* if we can't, we don't need to delete it, so go to the next kf */
	if (obj==NULL) return;

	/* otherwise, return a pointer to instancelist of it */
	victim=Find_Object_In_Instances(obj,main_window.all_instances);

	/* and delete/free that instance list */
	if (main_window.all_instances==victim)
		main_window.all_instances=main_window.all_instances->next;
	Delete_Element(victim);
	free(victim);

	/* if it's on the edit list, remove it */
	if (victim=Find_Object_In_Instances(obj,main_window.edit_instances))
		Delete_Edit_Instance(&main_window,victim);

	/* if it's on the selected list, remove it */
	if (victim=Find_Object_In_Instances(obj,main_window.selected_instances))
	{	if (main_window.selected_instances == victim)
			main_window.selected_instances=victim->next;
		Delete_Element(victim);
		free(victim);
	}

	/* delete the object itself */
	Destroy_Instance(obj);

	/* save the changes we've made */
	Update_to_KeyFrame(kfptr);
}


void
Zero_All(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *ptr;

	debug(FUNC_NAME,fprintf(stderr,"Zero_All()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	Update_to_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames));

	for (ptr=key_frames; ptr!=NULL; ptr=ptr->next)
	{	ptr->fnumber=ptr->kfnumber;
		changed_scene = TRUE;
	}

	Update_from_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames),TRUE);
}


void
Zero_Next(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *ptr,*current,*last;
	int count;

	debug(FUNC_NAME,fprintf(stderr,"Zero_Next()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);

	if (current == Last_KeyFrame(key_frames)) return;

	Update_to_KeyFrame(current);

	count=(current->next->fnumber - current->fnumber)-1;

	for (ptr=current->next; ptr!=NULL; ptr=ptr->next)
	{	ptr->fnumber-=count;
		changed_scene = TRUE;
	}

	Update_from_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames),TRUE);
}


void
Pump_All(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *ptr;
	int count;

	debug(FUNC_NAME,fprintf(stderr,"Pump_All()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	Update_to_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames));

	count=1;

	for (ptr=key_frames; ptr!=NULL; ptr=ptr->next)
	{	ptr->fnumber=count;
		count+=PUMP;
		changed_scene = TRUE;
	}

	Update_from_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames),TRUE);
}


void
Clone_Attributes(Widget w, XEvent *e, String *s, Cardinal *num)
{	InstanceList ptr;
	ObjectInstancePtr obj;
	AttributePtr new;
	KeyFrame *current,*kfptr;

	debug(FUNC_NAME,fprintf(stderr,"Clone_to_Attributes()\n"));

	debug(FUNC_VAL,
		for (ptr=main_window.selected_instances; ptr!=NULL; ptr=ptr->next)
			fprintf(stderr,"Cloning to next %s\n",ptr->the_instance->o_label);
	);

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	for (ptr=current->selected_instances; ptr!=NULL; ptr=ptr->next)
		for (kfptr=current->next; kfptr!=NULL; kfptr=kfptr->next)
		{	new=Copy_Attribute(ptr->the_instance->o_attribs);

			obj=Object_From_Label
				(kfptr->all_instances,NULL,ptr->the_instance->o_label);

			if (obj==NULL) continue;

			free(((AttributePtr)obj->o_attribs)->extension);
			free(obj->o_attribs);

			obj->o_attribs=new;

			Update_from_KeyFrame(kfptr,TRUE);
			View_Update
				(&main_window,main_window.all_instances,CalcView|ViewNone);

			changed_scene = TRUE;
		}
}


void
Synch_View(Widget w, XEvent *e, String *s, Cardinal *num)
{	KeyFrame *current,*kfptr;
	Viewport vptr;
	int mag;

	debug(FUNC_NAME,fprintf(stderr,"Synch_View()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	vptr=main_window.viewport;

	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	XtVaGetValues(main_window.view_widget,XtNmagnification,&mag,NULL);

	for (kfptr=current->next; kfptr!=NULL; kfptr=kfptr->next)
	{	Update_from_KeyFrame(kfptr,TRUE);

		main_window.viewport=vptr;
		XtVaSetValues(main_window.view_widget,XtNmagnification,mag,NULL);

		Update_to_KeyFrame(kfptr);

		View_Update(&main_window,main_window.all_instances, CalcView);
		Update_Projection_Extents(main_window.all_instances);

		changed_scene = TRUE;
	}
}
