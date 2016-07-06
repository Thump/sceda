#define PATCHLEVEL 0
/*
**    ScEdA: A Constraint Based Scene Editor/Animator.
**    Copyright (C) 1995  Denis McLaughlin (denism@cyberus.ca)
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
**  sced: A Constraint Based Object Scene Editor
**
**  spline.c : Functions for computing splines
**
*/

#include <math.h>
#include <sced.h>
#include <stdio.h>


double *curv0(double *x, double *y, int n, int total, double tension);
double *curvpp(double *x, double *y, int total);
void curv1(double *x, double *y, double *yp, int n, double tension);
double curv2(double *x,double *y,double *yp,double t,int n, double tension);


double *curv0(double *x, double *y, int n, int total, double tension)
{	int i, j, stop;
	double s,ds,xx,yy,sum;
	double *work,*xp,*yp,*temp;
	double *xpath,*ypath,*tmp;
	double **ans;
	double *result;

	debug(FUNC_NAME,fprintf(stderr,"curv0()\n"));

	yp=(double*)malloc(n*sizeof(double));
	temp=(double*)malloc(n*sizeof(double));
	xp=(double*)malloc(n*sizeof(double));
	work=(double*)malloc(n*sizeof(double));

	ans=(double **)malloc(2*sizeof(double *));
	xpath=(double*)malloc(total*sizeof(double));
	ypath=(double*)malloc(total*sizeof(double));
	*(ans)=xpath;
	*(ans+1)=ypath;


	work[0]=sum=0.;

	for (i=1; i<n; i++)
	{	xx=x[i]-x[i-1];
		yy=y[i]-y[i-1];
		work[i]=(sum+=sqrt(xx*xx + yy*yy));
	}

	curv1(work,x,xp,n,tension); 
	curv1(work,y,yp,n,tension);

	s=work[0];

	for (i=j=0; j<n-1; j++)
	{	stop=(total-1)*(j+1)/(n-1);
		ds=(work[j+1]-work[j])/(stop-i);
		for ( ; i<stop; i++)
		{	xpath[i]=curv2(work,x,xp,s,n,tension);
			ypath[i]=curv2(work,y,yp,s,n,tension);
			s+=ds;
		}
	}

	xpath[total-1]=x[n-1];
	ypath[total-1]=y[n-1];

	debug(SPLINE_IN,
		fprintf(stderr,"\nSpline-in values:\n");
		for (i=0; i<n; i++)
			fprintf(stderr,"x-in %f y-in %f\n",x[i],y[i]);
		fprintf(stderr,"\n");
	);


	debug(SPLINE_OUT,
		fprintf(stderr,"Spline-out values:\n");
		for (i=0; i<total; i++)
			fprintf(stderr,"x-out %f y-out %f\n",xpath[i],ypath[i]);
		fprintf(stderr,"\n");
	);

	result=curvpp2(curvpp(xpath,ypath,total),x,y,n);

	free(yp); free(temp); free(xp); free(work); free(ans);
	free(xpath); free(ypath);

	return(result);
}


/*  'Kay, this happy piece of magic prevents extraneous motion from
** cropping up in the spline: if between any two keyframes, a particular
** value did not change, this forces all intermediate splined values
** to also not change.  Like I said, magic.  It works.  Believe.
*/
double *curvpp2(double *val, double *x, double *y, int n)
{	int i,j;
	double test;

	debug(FUNC_NAME,fprintf(stderr,"curvpp2()\n"));

	for (i=0; i<n-1; i++)
		if (y[i]==y[i+1])
			for (j=x[i]-x[0]; j<x[i+1]-x[0]; j++)
				val[j]=y[i];

	debug(SPLINE_PP2,
		fprintf(stderr,"\nFinal post-processed spline values are:\n");
		for (i=0; i<n; i++)
			fprintf(stderr,"frame %d value %f\n",i,val[i]);
	);

	return(val);
}


double *curvpp(double *x, double *y, int total)
{	int i, j, pick, bottom, top, offset;
	double tempx,tempy,lasty,ydiff;
	double *path;

	debug(FUNC_NAME,fprintf(stderr,"curvpp()\n"));

	path=(double *)EMalloc(total*sizeof(double));

	/* Uh, yeah, I *did* miss a couple CS lectures, why do you ask? */
	for (i=0; i<total; i++)
		for (j=0; j<total-1; j++)
			if (x[j] > x[j+1])
			{	tempx=x[j+1];
				tempy=y[j+1];
				x[j+1]=x[j];
				y[j+1]=y[j];
				x[j]=tempx;
				y[j]=tempy;
			}

	/* Change our arrays to be 1-offset, to make things easier. */
	path=path-1; x=x-1; y=y-1; 

	/* offset is used to find the smallest frame we're processing */
	offset=x[1]-1;

	/* Set the first and last values. */
	path[1]=y[1];
	path[total]=y[total];

	/* This next bit should find appropriate values for integer frames:
	** without this, curv0() may not be returning a value for a certain
	** frame.  That is, just because of the data, a certain frame value
	** (represented by the x-axis) may have been skipped.  This for loop
	** should go through the frame-value pairs, and pick the nearest
	** value for integer frame values.  This means a linear interp from
	** the nearest two values, hence the top and bottom value.
	**
	** Yeah, I know, why are we linear interping in a spline function? 
	** Because we either interpolate from 2 points, or more than 2 points.
	** A spline of two points is linear anyway, and if we do more than
	** 2 points, the 1st and 2nd derivatives of the secondary spline
	** may not match the original one, making for weird motion.
	**
	** Hey, gimme a break, this mostly works.  Where it doesn't work is 
	** when the motions are so small that the linear interp messes up.
	** Read: very large rotations over few frames.  :(
	*/

	for (i=2; i<total; i++)
	{	j=1;

		/* Pick the one just under the frame value we want. */
		while (j<total && x[j+1]<i+offset)
			j++;
		bottom=j;

		/* Pick the one just over or equal to the frame value we want. */
		while (j<total && x[j]<i+offset)
			j++;
		top=j;

		/* Interpolate the two. */
		path[i]=(((y[top]-y[bottom])*((i+offset)-x[bottom]))
						/(x[top]-x[bottom]));
		path[i]+=y[bottom];

		debug(SPLINE_PICK,
			fprintf(stderr,"frame %d bottom %d top %d pick %f\n",
				i,bottom,top,path[i])
		);
	}

	debug(SPLINE_PICK,fprintf(stderr,"\n"));

	debug(SPLINE_PP,
		fprintf(stderr,"\nIntermediate post-processed spline values are:\n");
		for (i=1; i<=total; i++)
			fprintf(stderr,"frame %d value %f\n",i,path[i]);
	);

	return(path+1);
}


void curv1(double *x, double *y, double *yp, int n, double tension)
{	int i;
	double c1,c2,c3,deln,delnm1,delnn,dels,delx1,delx2,delx12;
	double diag1,diag2,diagin,dx1,dx2=0,exps;
	double sigmap,sinhs,sinhin,slpp1,slppn,spdiag;
	double *temp;
	double slp1=0.0,slpn=0.0;

	debug(FUNC_NAME,fprintf(stderr,"curv1()\n"));

	temp=(double*) malloc(n*sizeof(double));

	delx1=x[1] - x[0];
	dx1=(y[1] - y[0])/delx1;
	if(tension >= 0.) {slpp1=slp1; slppn=slpn;}
	else
		{if(n!=2)
			{delx2= x[2] - x[1];
			delx12= x[2] - x[0];
			c1= -(delx12 + delx1)/delx12/delx1;
			c2= delx12/delx1/delx2;
			c3= -delx1/delx12/delx2;
			slpp1= c1*y[0] + c2*y[1] + c3*y[2];
			deln= x[n-1] - x[n-2];
			delnm1= x[n-2] - x[n-3];
			delnn= x[n-1] - x[n-3];
			c1= (delnn + deln)/delnn/deln;
			c2= -delnn/deln/delnm1;
			c3= deln/delnn/delnm1;
			slppn= c3*y[n-3] + c2*y[n-2] + c1*y[n-1];
			}
		else yp[0]=yp[1]=0.;
		}
			/* denormalize tension factor */
	sigmap=fabs(tension)*(n-1)/(x[n-1]-x[0]);
			/* set up right hand side and tridiagonal system for
			   yp and perform forward elimination	*/
	dels=sigmap*delx1;
	exps=exp(dels);
	sinhs=.5*(exps-1./exps);
	sinhin=1./(delx1*sinhs);
	diag1=sinhin*(dels*.5*(exps+1./exps)-sinhs);
	diagin=1./diag1;
	yp[0]=diagin*(dx1-slpp1);
	spdiag=sinhin*(sinhs-dels);
	temp[0]=diagin*spdiag;
	if(n!=2)
		{for(i=1; i<=n-2; i++)
			{delx2=x[i+1]-x[i];
			dx2=(y[i+1]-y[i])/delx2;
			dels=sigmap*delx2;
			exps=exp(dels);
			sinhs=.5*(exps-1./exps);
			sinhin=1./(delx2*sinhs);
			diag2=sinhin*(dels*(.5*(exps+1./exps))-sinhs);
			diagin=1./(diag1+diag2-spdiag*temp[i-1]);
			yp[i]=diagin*(dx2-dx1-spdiag*yp[i-1]);
			spdiag=sinhin*(sinhs-dels);
			temp[i]=diagin*spdiag;
			dx1=dx2;
			diag1=diag2;
			}
		}
	diagin=1./(diag1-spdiag*temp[n-2]);
	yp[n-1]=diagin*(slppn-dx2-spdiag*yp[n-2]);
					/* perform back substitution */
	for (i=n-2; i>=0; i--) yp[i] -= temp[i]*yp[i+1];

	free(temp);
}


double curv2(double *x,double *y,double *yp,double t,int n, double tension)
{	int i;
	static int i1=1;
	double del1,del2,dels,exps,exps1,s,sigmap,sinhd1,sinhd2,sinhs;

	debug(FUNC_NAME,fprintf(stderr,"curv2()\n"));

	s=x[n-1]-x[0];
	sigmap=fabs(tension)*(n-1)/s;
	i=i1;
	while(i<n && t>=x[i]) i++;
	while(i>1 && x[i-1]>t) i--;
	i1=i;
	del1=t-x[i-1];
	del2=x[i]-t;
	dels=x[i] - x[i-1];
	exps1=exp(sigmap*del1); sinhd1=.5*(exps1-1./exps1);
	exps= exp(sigmap*del2); sinhd2=.5*(exps-1./exps);
	exps=exps1*exps;        sinhs=.5*(exps-1./exps);
	return ((yp[i]*sinhd1 + yp[i-1]*sinhd2)/sinhs +
			((y[i] - yp[i])*del1 + (y[i-1] - yp[i-1])*del2)/dels);
}
