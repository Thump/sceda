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




Vector *
Vector_Spline(int numkf,int numf,double *kf,double *x,double *y,double *z)
{	double *xspline,*yspline,*zspline;
	Vector *spline;
	int i;
	
	debug(FUNC_NAME,fprintf(stderr,"Vector_Spline()\n"));

	spline=(Vector *)EMalloc(numf*sizeof(Vector));

	xspline=curv0(kf,x,numkf,numf,TENSION);
	yspline=curv0(kf,y,numkf,numf,TENSION);
	zspline=curv0(kf,z,numkf,numf,TENSION);

	for (i=0; i<numf; i++)	
	{	spline[i].x=xspline[i];
		spline[i].y=yspline[i];
		spline[i].z=zspline[i];
	}

	free(xspline); free(yspline); free(zspline);

	return(spline);
}


Quaternion *
Quaternion_Spline
	(int numkf,int numf,double *kf,double *a,double *b,double *c,double *d)
{	double *aspline,*bspline,*cspline,*dspline;
	Quaternion *spline;
	int i;
	
	debug(FUNC_NAME,fprintf(stderr,"Quaternion_Spline()\n"));

	spline=(Quaternion *)EMalloc(numf*sizeof(Quaternion));

	aspline=curv0(kf,a,numkf,numf,TENSION);
	bspline=curv0(kf,b,numkf,numf,TENSION);
	cspline=curv0(kf,c,numkf,numf,TENSION);
	dspline=curv0(kf,d,numkf,numf,TENSION);

	for (i=0; i<numf; i++)	
	{	spline[i].real_part=aspline[i];
		spline[i].vect_part.x=bspline[i];
		spline[i].vect_part.y=cspline[i];
		spline[i].vect_part.z=dspline[i];
	}

	free(aspline); free(bspline); free(cspline); free(dspline);

	return(spline);
}


/* This returns the frame (0-based) frame in which the seq'th (zero-based) 
** sequence begins.
*/
int
Seq_Start(Boolean *o_inframe, int seq, int nf)
{	int i,j,frame;

	debug(FUNC_NAME,fprintf(stderr,"Seq_Start()\n"));

	i=0; j=0;

	/* Find the beginning of the first sequence */
	while (i<nf && o_inframe[i]==FALSE)
		i++;

	while(i<nf)
	{	if (j==seq)
			return(i);

		while (i<nf && o_inframe[i]==TRUE)
			i++;
		
		while (i<nf && o_inframe[i]==FALSE)
			i++;

		j++;
	}

	return(i);
}
		

/* This returns the frame (0-based) in which the seq'th (0-based) 
** sequence ends: that is, the last TRUE frame of the seq'th sequence.
*/
int
Seq_Stop(Boolean *o_inframe, int seq, int nf)
{	int i,j,frame;

	debug(FUNC_NAME,fprintf(stderr,"Seq_Start()\n"));

	debug(SEQ,fprintf(stderr,"passed seq %d numf %d\n",seq,nf));

	i=0; j=0;

	/* Find the end of the first sequence */
	while (i<nf && o_inframe[i]==FALSE)
		i++;
	while (i<nf && o_inframe[i]==TRUE)
		i++;
	if (i!=0) i--;

	if (i==nf-1) return(i);

	debug(SEQ,fprintf(stderr,"Now looking at %d\n",i));

	while(i<nf)
	{	if (j==seq)
			return(i);

		i++;
		while (i<nf && o_inframe[i]==FALSE)
			i++;

		while (i<nf && o_inframe[i]==TRUE)
			i++;
		if (i!=0) i--;

	debug(SEQ,fprintf(stderr,"Now looking at %d\n",i));
		
		j++;
	}

	return(i);
}


/* This returns the number of keyframes that are used in the sequence
** that begins at frame number start and ends at frame number stop,
** with, as usual, stop and start both being 1-based.  
*/
int
Numkf_in_Seq(KeyFrame *key_frames,int start,int stop)
{	int i;
	KeyFrame *ptr;

	debug(FUNC_NAME,fprintf(stderr,"Numkf_in_Seq()\n"));

	ptr=key_frames;

	/* we add one to start and stop because frames are base 1 */

	while (ptr->next!=NULL && ptr->fnumber<start+1)
		ptr=ptr->next;

	i=1;

	while (ptr->next!=NULL && ptr->fnumber<stop+1)
	{	ptr=ptr->next;
		i++;
	}

	return(i);
}


void
Spline_Object(ObjectInstance *obj)
{	int nkf,nf,i,j,seqstart,seqstop,seqnum,sf;
	ObjectInstance *tempobj;
	double *x,*y,*z;
	double *a,*b,*c,*d,*sx,*sy,*sz,*kf,tmp;
	KeyFrame *kfptr;
	Vector *result;
	Quaternion *rotquats,quat;
	Vector *scalevecs,*posspline;
	Matrix *spline,rot,scale;

	debug(FUNC_NAME,fprintf(stderr,"Spline_Object()\n"));
	
	nf=Count_Frames(key_frames);

	obj->o_posspline=(Vector *)EMalloc(sizeof(Vector)*nf);
	obj->o_rotspline=(Matrix *)EMalloc(sizeof(Matrix)*nf);

	for (j=0; j<obj->o_numseqs; j++)
	{	seqstart=Seq_Start(obj->o_inframe,j,nf);
		seqstop=Seq_Stop(obj->o_inframe,j,nf);

		/* nkf is the number of keyframes in this sequence */
		nkf=Numkf_in_Seq(key_frames,seqstart,seqstop);

		if (nkf<2) continue;

		/* sf is the number of frames in this sequence */
		sf=seqstop-seqstart+1;

		debug(SEQ,fprintf(stderr,
			"Called on %s, seqs: %d\n",obj->o_label,obj->o_numseqs));
		debug(SEQ,fprintf(stderr,
			"seqstart: %d seqstop %d\n",seqstart,seqstop));
		debug(SEQ,fprintf(stderr,
			"%d keyframes and %d frames in this sequence\n",nkf,sf));

		/* this is to hold the splined rot/scaling */
		spline=(Matrix *)EMalloc(sf*sizeof(Matrix));

		/* this is to hold the framing info */
		kf=(double *)EMalloc(nkf*sizeof(double));

		/* these are the x, y, and z coordinates for position splining */
		x=(double *)EMalloc(nkf*sizeof(double));
		y=(double *)EMalloc(nkf*sizeof(double));
		z=(double *)EMalloc(nkf*sizeof(double));

		/* these are to hold quaternions (rotations) */
		a=(double *)EMalloc(nkf*sizeof(double));
		b=(double *)EMalloc(nkf*sizeof(double));
		c=(double *)EMalloc(nkf*sizeof(double));
		d=(double *)EMalloc(nkf*sizeof(double));

		/* these are to hold scales */
		sx=(double *)EMalloc(nkf*sizeof(double));
		sy=(double *)EMalloc(nkf*sizeof(double));
		sz=(double *)EMalloc(nkf*sizeof(double));

		kfptr=key_frames;
		seqnum=0;

		/* put kfptr on the first frame of the sequence */
		while(kfptr->next!=NULL && kfptr->fnumber<seqstart+1)
			kfptr=kfptr->next;

		/* and then loop 'til we hit the last one: we add one to
		** seqstop because frames are 1-based
		*/
		while( (kfptr != NULL) && (kfptr->fnumber <= seqstop+1) )
		{	tempobj=Object_From_Label(kfptr->all_instances,NULL,obj->o_label);

			kf[seqnum]=kfptr->fnumber;

			/* get position data */
			x[seqnum]=tempobj->o_transform.displacement.x;
			y[seqnum]=tempobj->o_transform.displacement.y;
			z[seqnum]=tempobj->o_transform.displacement.z;

			/* get quaternion data */
			a[seqnum]=tempobj->o_rot.real_part;
			b[seqnum]=tempobj->o_rot.vect_part.x;
			c[seqnum]=tempobj->o_rot.vect_part.y;
			d[seqnum]=tempobj->o_rot.vect_part.z;

			/* get scaling data */
			sx[seqnum]=tempobj->o_scale.x;
			sy[seqnum]=tempobj->o_scale.y;
			sz[seqnum]=tempobj->o_scale.z;

			seqnum++;
			kfptr=kfptr->next;
		}

		/* this splines and stores the info for the position spline */
		posspline=Vector_Spline(nkf,sf,kf,x,y,z);

		/* now, we know how many splined matrices are going to be in
		** in this sequence, so we malloc out our o_rotspline[] now.
		** We don't need to do this for the o_posspline[] because
		** Vector_Spline() does it for us.
		*/
		/* obj->o_rotspline[j]=(Matrix *)EMalloc(sizeof(Matrix)*seqnum); */

		/* here we spline the rot and scale data */
		rotquats=Quaternion_Spline(nkf,sf,kf,a,b,c,d);
		scalevecs=Vector_Spline(nkf,sf,kf,sx,sy,sz);

		/* and then we composite the matrix from the splined data */
		for (i=0; i<sf; i++)
		{	scale=Scale_To_Matrix(scalevecs[i].x,scalevecs[i].y,scalevecs[i].z);

			QUnit(rotquats[i],tmp,quat);
			rot=Quaternion_To_Matrix(quat);

			/* Oddly, the order here matters: multiplying the scale by
			** the rot makes for fucked up scaling... ?
			*/
			spline[i]=MMMul(&rot,&scale);
		}

		for (i=seqstart; i<=seqstop; i++)
		{	obj->o_posspline[i]=posspline[i-seqstart];
			obj->o_rotspline[i]=spline[i-seqstart];
		}

		/* free up the original splines */
		free(posspline); free(spline); 

		/* free the position-spline data */
		free(x); free(y); free(z); free(kf);

		/* free the rot/scale-spline data */
		free(a); free(b); free(c); free(d); 
		free(sx); free(sy); free(sz); 
		free(rotquats); free(scalevecs);
	}
}


void
Spline_Viewport(KeyFrame *key_frames)
{	int i,numkf, numf;

	Transformation *spline;

	double	*vfx,*vfy,*vfz, /* these are the arrays of key values for		*/
			*vax,*vay,*vaz, /* the x, y and z components of the view_from,	*/
			*vux,*vuy,*vuz, /* view_at, view_up and eye_position vectors	*/
			*epx,*epy,*epz,
			*mag, *tmags;

	Vector *vf,*va,*vu,*ep;	/* these are the splined vector arrays for the	*/
							/* above key value arrays.						*/

	double *vd,*vds,*ed,*eds,*kf;	/* these are the key value and splined	*/
									/* value arrays for view_distance and	*/
									/* eye_distance (and keyframes).		*/
	int *mags;
	KeyFrame *ptr;
	Viewport work;

	debug(FUNC_NAME,fprintf(stderr,"Spline_Viewport()\n"));

	numkf=Count_KeyFrames(key_frames);
	numf=Count_Frames(key_frames);

	/* setup arrays for all the data to be splined from */
	vfx=(double *)EMalloc(numkf*sizeof(double));
	vfy=(double *)EMalloc(numkf*sizeof(double));
	vfz=(double *)EMalloc(numkf*sizeof(double));
	vax=(double *)EMalloc(numkf*sizeof(double));
	vay=(double *)EMalloc(numkf*sizeof(double));
	vaz=(double *)EMalloc(numkf*sizeof(double));
	vux=(double *)EMalloc(numkf*sizeof(double));
	vuy=(double *)EMalloc(numkf*sizeof(double));
	vuz=(double *)EMalloc(numkf*sizeof(double));
	epx=(double *)EMalloc(numkf*sizeof(double));
	epy=(double *)EMalloc(numkf*sizeof(double));
	epz=(double *)EMalloc(numkf*sizeof(double));

	vd=(double *)EMalloc(numkf*sizeof(double));
	ed=(double *)EMalloc(numkf*sizeof(double));
	kf=(double *)EMalloc(numkf*sizeof(double));
	mag=(double *)EMalloc(numkf*sizeof(double));

	mags=(int *)EMalloc(numf*sizeof(int));

	spline=(Transformation *)EMalloc(numf*sizeof(Transformation));

	/* fill the above arrays from the data in the keyframes */
	i=0;
	for (ptr=key_frames; ptr!=NULL; ptr=ptr->next)
	{	vfx[i]=ptr->viewport.view_from.x;
		vfy[i]=ptr->viewport.view_from.y;
		vfz[i]=ptr->viewport.view_from.z;

		vax[i]=ptr->viewport.view_at.x;
		vay[i]=ptr->viewport.view_at.y;
		vaz[i]=ptr->viewport.view_at.z;

		vux[i]=ptr->viewport.view_up.x;
		vuy[i]=ptr->viewport.view_up.y;
		vuz[i]=ptr->viewport.view_up.z;

		epx[i]=ptr->viewport.eye_position.x;
		epy[i]=ptr->viewport.eye_position.y;
		epz[i]=ptr->viewport.eye_position.z;

		vd[i]=ptr->viewport.view_distance;
		ed[i]=ptr->viewport.eye_distance;

		mag[i]=ptr->magnification;

		kf[i]=ptr->fnumber;

		i++;
	}

	/* spline the data we just pulled from the keyframes */
	vf=Vector_Spline(numkf,numf,kf,vfx,vfy,vfz);
	va=Vector_Spline(numkf,numf,kf,vax,vay,vaz);
	vu=Vector_Spline(numkf,numf,kf,vux,vuy,vuz);
	ep=Vector_Spline(numkf,numf,kf,epx,epy,epz);

	vds=curv0(kf,vd,numkf,numf,TENSION);
	eds=curv0(kf,ed,numkf,numf,TENSION);

	tmags=curv0(kf,mag,numkf,numf,TENSION);

	/* save the splined info back to the viewport of the first keyframe */
	main_window.viewport.view_from_spline=vf;
	main_window.viewport.view_at_spline=va;
	main_window.viewport.view_up_spline=vu;
	main_window.viewport.eye_position_spline=ep;
	main_window.viewport.view_distance_spline=vds;
	main_window.viewport.eye_distance_spline=eds;

	/* now convert the splined data in a world_to_view matrix and save it */
	for (i=0; i<numf; i++)
	{	work.view_from=vf[i];
		work.view_at=va[i];
		work.view_up=vu[i];
		work.view_distance=vds[i];
		work.eye_distance=eds[i];

		Build_Viewport_Transformation(&work);

		spline[i]=work.world_to_view;
		mags[i]=tmags[i];
	}

	/* save more of the splines back to the viewport of the first keyframe */
	main_window.viewport.world_to_view_spline=spline;
	main_window.viewport.magnify_spline=mags;

	/* being good unixen, we free everything up */
	free(vfx); free(vfy); free(vfz); free(vax); free(vay); free(vaz);
	free(vux); free(vuy); free(vuz); free(vd); free(ed);
	free(kf); free(epx); free(epy); free(epz);
}


/* This determines all objects in the animation, computes their sequence
** starts and stops, and then splines their postion, rotation and scaling.
**
** A object's sequence is a contigous sequence of keyframes in which it
** is present: if an object is present in kf1, kf2, kf4 and k5 (but not in 
** kf3), then the object has two sequences.  Sequences always begin and
** end on a keyframe.
*/
void
Spline_World()
{	InstanceListElmt *objptr;
	String label;
	ObjectInstance *obj,*temp;
	KeyFrame *kfptr,*start,*stop;
	int i,nf,nkf,num;

	debug(FUNC_NAME,fprintf(stderr,"Spline_World()\n"));

	main_window.all_instances=NULL;
	nf=Count_Frames(key_frames);
	nkf=Count_KeyFrames(key_frames);

	debug(SEQ,fprintf(stderr,"number of frames is: %d\n",nf));

	/* Go through all keyframes: for each keyframe, go through all
	** objects: if the object does not currently exist on the 
	** main_window.all_instances list, add it.
	**
	** This will leave us with a single instance of all objects
	** in all keyframes in the main_window.all_instances list.
	*/
	for (kfptr=key_frames; kfptr!=NULL; kfptr=kfptr->next)
		for (objptr=kfptr->all_instances; objptr!=NULL; objptr=objptr->next)
			if (Object_From_Label(main_window.all_instances,
					NULL,objptr->the_instance->o_label)==NULL)
			{	temp=Copy_ObjectInstance(objptr->the_instance);
				Insert_Element(&main_window.all_instances,temp);
			}

	/* For each object in the animation... */
	for (objptr=main_window.all_instances; objptr!=NULL; objptr=objptr->next)
	{	obj=objptr->the_instance;
		obj->o_numseqs=0;

		obj->o_inframe=(Boolean *)EMalloc(sizeof(Boolean)*nf);

		for (i=0; i<nf; i++)
			obj->o_inframe[i]=FALSE;

		label=obj->o_label;

		debug(SEQ,fprintf(stderr,"Sequencing %s\n",label));

		kfptr=key_frames;

		/* ...over all the keyframes in the animation... */
		while (kfptr!=NULL)
		{	/* ...find the frame where it is first present... */
			while (kfptr!=NULL && Object_From_Label(kfptr->all_instances,
					NULL,label) == NULL)
				kfptr=kfptr->next;

			if (kfptr==NULL)
				break;
			
			/* ...and save it... */
			start=kfptr;

			if (kfptr==NULL)
			{	fprintf(stderr,"Uh-oh, can't find keyframe for %s\n",label);
				break;
			}

			/* ...and then find the last frame it is displayed in... */
			while (kfptr->next!=NULL && 
				Object_From_Label(kfptr->next->all_instances,NULL,label)!=NULL)
				kfptr=kfptr->next;

			/* ...and save it... */
			stop=kfptr;
			kfptr=kfptr->next;

			debug(SEQ,fprintf(stderr,
				"TRUEing %d to %d\n",start->fnumber-1,stop->fnumber-1));

			/* ...and then TRUE the frames between the two saved keyframes... */
			for (i=start->fnumber-1; i<=stop->fnumber-1; i++)
				obj->o_inframe[i]=TRUE;

			/* ...increment the number of sequences, and do it again... */
			obj->o_numseqs++;
		}

		debug(SEQ,fprintf(stderr,"Splining %s\n",obj->o_label));

		Spline_Object(obj);
	}
}


/*
** 'Kay, so here's what happens: you got a bunch of keyframes with a
** bunch of objects in different positions and rotations, and dependencies
** among them.  So you want to animate them: if you just spline the
** differences in position or rotation, the splined values, they may
** not correspond to the dependencies you set up.  This is a bad thing.
**
** So to fix this, first the array of splined values are setup.  Then
** each frame is loaded and all the dependencies are resolved, which
** adjusts the splined values.  Then the values are loaded from the
** frame back into the spline matrices.  In this way, the splined
** values are always correct: splined and then constrained.
*/
void
Constrain_World()
{	int i;
	InstanceList ptr,ptr2,list,temp,also;
	ObjectInstance *obj,*work1,*work2;
	KeyFrame *kfptr;

	debug(FUNC_NAME,fprintf(stderr,"Constrain_World()\n"));

	list=main_window.all_instances;

	/* for each keyframe */
	for (kfptr=key_frames; kfptr->next!=NULL; kfptr=kfptr->next)
	{	temp=Copy_InstanceList(kfptr->all_instances,NULL);
		main_window.all_instances=temp;

/* fprintf(stderr,"\nkf %d\n",kfptr->kfnumber); */
/* Print_InstanceList(temp); */

		/* loop over the frames relevant to that keyframe */
		for (i=kfptr->fnumber; i<kfptr->next->fnumber; i++)
		{	/* load what the pos/rot/scale are supposed to be */
			Update_from_Frame(i,list);

			/* then constrain all the objects */
			/* for (ptr=temp; ptr!=NULL; ptr=ptr->next) */
				/* Edit_Update_Object_Dependents(ptr->the_instance); */

			work1=Object_From_Label(temp,NULL,"frame_15");
			work2=Object_From_Label(temp,NULL,"Cone_2_3_4");
			/* fprintf(stderr,"\nframe %d\n",i); */
			/* Print_Instance(work2); */
			/* if (work1==NULL || work2==NULL)  */
				/* fprintf(stderr,"shit...\n"); */
			/* else  */
				/* Edit_Update_Object_Dependents(work2); */
				/* Edit_Update_Object_Dependents(work1); */
			/* Print_Instance(work2); */

			/* and now store the pos/rot/scale back into the spline */
			Update_to_Frame(i,list);
		}
		Free_InstanceList(temp);
	}
	main_window.all_instances=list;
}



/* This setups main_window for frame i.  Note, however, that in calling
** this for frame i, it is assumed that it has been called on i-1: this
** is because object translation is done in increments, and i-1 hasn't
** been done, objects won't be in the correct position for i.
**
** Actually, I'm not sure the above is correct: it may be that position
** is actually done as an absolute.
**
** As you can see, the function returns void.  It does, however, do quite
** a bit, all of which is communicated back to the world through main_window.
** Remember: global variables are _your_ friend...
*/
void
Update_from_Frame(int i,InstanceList list)
{	ObjectInstance *obj,*splineobj;
	int j,seq,seqf;
	InstanceListElmt *ptr;
	Wireframe *wire;
	Matrix inverse,trans;
	double tmp;

	debug(FUNC_NAME,fprintf(stderr,"Update_from_Frame()\n"));

	/* update the camera position */
	main_window.viewport.world_to_view=
		main_window.viewport.world_to_view_spline[i];
	main_window.viewport.view_from=
		main_window.viewport.view_from_spline[i];
	main_window.viewport.view_at=
		main_window.viewport.view_at_spline[i];
	main_window.viewport.view_up=
		main_window.viewport.view_up_spline[i];
	main_window.viewport.eye_position=
		main_window.viewport.eye_position_spline[i];
	main_window.viewport.view_distance=
		main_window.viewport.view_distance_spline[i];
	main_window.viewport.eye_distance=
		main_window.viewport.eye_distance_spline[i];
	XtVaSetValues(main_window.view_widget,XtNmagnification,
		main_window.viewport.magnify_spline[i],NULL);

	/* go through and update the pos/rot/scale of every object */
	for (ptr=main_window.all_instances; ptr!=NULL; ptr=ptr->next)
	{	obj=ptr->the_instance;
		wire=obj->o_wireframe;

		splineobj=Object_From_Label(list,NULL,obj->o_label);
		if (splineobj==NULL) fprintf(stderr,"badness in Update_from_Frame\n");

		if (splineobj->o_inframe[i]==FALSE)
		{	obj->o_flags &= (ObjAll^ObjVisible);
			continue;
		}
		else
			obj->o_flags |= ObjVisible;

		for (j=0; j<obj->o_num_vertices; j++)
		{	MVMul(splineobj->o_rotspline[i],wire->vertices[j],
				obj->o_world_verts[j]);
			VAdd(obj->o_world_verts[j],splineobj->o_posspline[i],
				obj->o_world_verts[j]);
		}

		/* the above changes the vertices of an object, which affects
		** how the object is displayed.  The following line affects
		** how the object is exported.
		*/
		obj->o_transform.displacement=splineobj->o_posspline[i];

		debug(ANI_MAT, fprintf(stderr,"Frame %d\n",i);
			Print_Matrix(&splineobj->o_rotspline[i]));

		inverse=MInvert(&splineobj->o_rotspline[i]);
		MTrans(inverse,trans);

		/* likewise, this line is for exporting */
		obj->o_transform.matrix=splineobj->o_rotspline[i];

		for (j=0; j<obj->o_num_faces; j++)
		{	MVMul(trans,wire->faces[j].normal,obj->o_normals[j]);
			if (VMod(obj->o_normals[j])>EPSILON)
				VUnit(obj->o_normals[j],tmp,obj->o_normals[j]);
		}
	}
}


void
Update_to_Frame(int i,InstanceList list)
{	ObjectInstance *obj,*splineobj;
	InstanceListElmt *ptr;
	int seq, seqf;

	debug(FUNC_NAME,fprintf(stderr,"Update_to_Frame()\n"));

	/* go through and update the pos/rot/scale of every object */
	for (ptr=main_window.all_instances; ptr!=NULL; ptr=ptr->next)
	{	obj=ptr->the_instance;

		splineobj=Object_From_Label(list,NULL,obj->o_label);
		if (splineobj==NULL) fprintf(stderr,"badness in Update_from_Frame\n");

		if (splineobj->o_inframe[i]==FALSE)
			obj->o_flags &= (ObjAll^ObjVisible);
		else
			obj->o_flags |= ObjVisible;

		splineobj->o_posspline[i]=obj->o_transform.displacement;
		splineobj->o_rotspline[i]=obj->o_transform.matrix;
	}
}


void
Animate(Widget w, XEvent *e, String *s, Cardinal *num)
{	int numframes,numkf,i;
	InstanceListElmt *ptr;

	debug(FUNC_NAME,fprintf(stderr,"Animate()\n"));

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	/* save the current keyframe */
	Update_to_KeyFrame(Nth_KeyFrame(main_window.kfnumber,key_frames));

	/* find out how many frames and keyframes we got altogether */
	numframes=Count_Frames(key_frames);
	numkf=Count_KeyFrames(key_frames);

	if (numkf==1) return;

	/* reset everything as if we were at the first keyframe, then 
	Update_from_KeyFrame(key_frames,TRUE);

	/* get us a fresh copy of objects to screw with */
	/* main_window.all_instances= */
		/* Copy_InstanceList(key_frames->all_instances,NULL); */

	/* calculate all the splined data */
	Spline_World();
	Spline_Viewport(key_frames);
	/* Constrain_World(); */

	/* for each frame, set things up, then display it */
	for (i=0; i<numframes; i++)
	{	Update_from_Frame(i,main_window.all_instances);
		View_Update(&main_window,main_window.all_instances,CalcView);
	}

	/* be polite with our memory */
	for (ptr=main_window.all_instances; ptr!=NULL; ptr=ptr->next)
	{	free(ptr->the_instance->o_posspline);
		free(ptr->the_instance->o_rotspline);
	}

	Free_InstanceList(main_window.all_instances);

	free(main_window.viewport.view_from_spline);
	free(main_window.viewport.view_at_spline);
	free(main_window.viewport.view_up_spline);
	free(main_window.viewport.view_distance_spline);
	free(main_window.viewport.eye_distance_spline);
	free(main_window.viewport.eye_position_spline);
	free(main_window.viewport.magnify_spline);
	free(main_window.viewport.world_to_view_spline);

	/* reset everything for the last key frame */
	free(main_window.all_instances);
	Update_from_KeyFrame(Nth_KeyFrame(numkf,key_frames),TRUE);
}


/*	void
**	Export_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**	The callback invoked for the export menu function.
**	Checks for a target, then pops up the export dialog.
*/
void
Export_Animation(Widget w, XEvent *e, String *s, Cardinal *num)
{	SceneStruct	export_scene;
	FILE		*infile,*scriptfile;
	char		in_name[1024],out_name[1024],*base=NULL,*script=NULL;
	int 		i,numframes;
	KeyFrame	*current;

	debug(FUNC_NAME,fprintf(stderr,"Export_Animation()\n"));

	/* Check that we should bother... */
	if ( (numframes=Count_Frames(key_frames))<2 )
		return; 

	/* save the current frame, then reset from the first key_frame 
	*/
	current=Nth_KeyFrame(main_window.kfnumber,key_frames);
	Update_to_KeyFrame(current);

	/* Check if we're allowed to be doing this at all... */
	if ( ! XtIsSensitive(main_option_buttons[0]) ) return;

	/* Need to have a target. */
	if ( camera.type == NoTarget )
	{	Target_Callback(NULL, "Export Animation", NULL);
		return;
	}

	Spline_World();
	Spline_Viewport(key_frames);
	/* Constrain_World(); */

	while (base==NULL)
		base=Get_String("Animation Basename","base");

	while (script==NULL)
		script=Get_String("Scriptname","script");

	scriptfile=fopen(script,"w");
	if (scriptfile==NULL)
		fprintf(stderr,"Ack! Can't open %s for write!\n",script);

	for (i=0; i<numframes; i++)
	{	Update_from_Frame(i,main_window.all_instances);

		Viewport_To_Camera(&(main_window.viewport),main_window.view_widget,
			&camera,FALSE);
		export_scene.camera = camera;
		export_scene.light = NULL;
		export_scene.ambient = ambient_light;
		export_scene.instances = main_window.all_instances;

		sprintf(in_name,"%s.in.%d",base,i);
		sprintf(out_name,"%s.out.%d",base,i);
		infile=fopen(in_name,"w");

		if (infile==NULL)
			fprintf(stderr,"Ack! Can't open %s for write!\n",in_name);
		else
			Export_File(infile,in_name,&export_scene,FALSE);

		fprintf(scriptfile,"echo rendering frame %d of %d\n",i,numframes-1);
		fprintf(scriptfile,"%s\n",
			Render_Command(camera.type,in_name,out_name,160,120));

		fclose(infile);
	}

	fprintf(scriptfile,"echo Rendering complete\n");
	fclose(scriptfile);
	chmod(script,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);

	Update_from_KeyFrame(current,TRUE);
}


char *
Render_Command(Raytracer target, char *in, char *out, int width, int height)
{	char	*system_string = NULL;

	debug(FUNC_NAME,fprintf(stderr,"Render_Command()\n"));

	switch (target)
	{	case Genray:
			if ( genray_path[0] == '\0' ) 
			{	Popup_Error("Can't find Genray",main_window.shell,"Error");
				return;
			}
			break;
		case Genscan:
			if ( genscan_path[0] == '\0' ) 
			{	Popup_Error("Can't find Genscan",main_window.shell,"Error");
				return;
			}
			break;
		case POVray:
			if ( povray_path[0] == '\0' )
			{	Popup_Error("Can't find POVRay",main_window.shell,"Error");
				return;
			}
			break;
		case Rayshade:
			if ( rayshade_path[0] == '\0' )
			{	Popup_Error("Can't find Rayshade",main_window.shell,"Error");
				return;
			}
			break;
		case Radiance:
			if ( radiance_path[0] == '\0' )
			{	Popup_Error("Can't find Radiance",main_window.shell,"Error");
				return;
			}
			break;
		case Renderman:
			if ( renderman_path[0] == '\0' )
			{	Popup_Error("Can't find RenderMan",main_window.shell,"Error");
				return;
			}
			break;
		default:;
	}

	switch ( target )
	{	case Rayshade:
			system_string = New(char,
							strlen(rayshade_path) + strlen(rayshade_options) +
							strlen(in)  + 30);
			sprintf(system_string, "%s %s -O %s < %s",
					rayshade_path, rayshade_options, out, in);
			break;
		case Radiance:
			system_string = New(char,
							strlen(radiance_path) + strlen(radiance_options) +
							strlen(in)  + 30);
			sprintf(system_string, "%s %s %s",
					radiance_path, radiance_options, in);
			break;
		case Renderman:
			system_string = New(char,
							strlen(renderman_path) + strlen(renderman_options) +
							strlen(in)  + 30);
			sprintf(system_string, "%s %s %s",
					renderman_path, renderman_options, in);
			break;
		case POVray:
			system_string = New(char,
							strlen(povray_path) + strlen(povray_options) +
							strlen(in)  + 50);
			sprintf(system_string, "%s -w%d -h%d -I%s -O%s %s",
					povray_path, width, height, in,
					out, povray_options);
			break;
		case Genray:
			system_string = New(char,
							strlen(genray_path) + strlen(genray_options) +
							strlen(in)  + 30);
			sprintf(system_string, "%s %s %s",
					genray_path, genray_options, in);
			break;
		case Genscan:
			system_string = New(char,
							strlen(genscan_path) + strlen(genscan_options) +
							strlen(in)  + 30);
			sprintf(system_string, "%s %s %s",
					genscan_path, genscan_options, in);
			break;
		default: ;
	}

	return(system_string);
}
