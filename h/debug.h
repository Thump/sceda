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

#define PATCHLEVEL 0
/*
**	sced: A Constraint Based Object Scene Editor
**
**	debug.h: debug defines
**
**	Created: 17/04/95
*/

/* 
** This allows us the facility of 'debug(val,command);', where if the
** val'th bit is set in debugv (a global variable denoting the debug
** level), command is run.  debugv is set with the -v # switch: this 
** will only work if DEBUG was defined at compile time.
**
** Typical uses of this would be commands like this:
**    debug(FUNC_NAME,fprintf(stderr,"Functionname()\n"));
** at the beginning of every function, so that if the FUNC_NAME'th bit
** (defined below) is set in debugv, the name of every function is printed
** when the function is entered.
**
** This can be used in more complicated for loops, however: see spline.c
** for examples.
**
** We're checking for the val'th bit set rather than a threshold
** value so that it is possible to select the exact debugging messages
** we want to see.
**
** The definition is surrounded by an #ifdef SCED_DEBUG so that all debug
** statements can be nulled out entirely if SCED_DEBUG is not defined. 
** This way there is no speed hit incurred by any of the debug code.
**
** Below the debug() define are the various debug values: the value of
** debugv is intended to be set from the command line with -v switch.
**
** Make sure it's SCED_DEBUG rather than just DEBUG: just DEBUG means
** stuff to the X environment (enables some X debugging!).
**
*/

#ifdef SCED_DEBUG
#define debug(val,com) if (debugv & 1<<val) {com;}
#else
#define debug(val,com)
#endif

/*
**  These values can be used as such: say you want to see the SPLINE_OUT
** debug messages, as well as FUNC_NAME:
**
**    debugv=2**FUNC_NAME+2**SPLINE_OUT=2**0+2**3=1+8=9
**
** so calling sced -v 9 will print out the desired messages.  Neat!
*/

#define FUNC_NAME 0		/* each function prints its name when entered	*/
#define FUNC_VAL 1		/* some functions print relevant values			*/
#define SPLINE_IN 2		/* print the values being passed to the spline	*/
#define SPLINE_OUT 3	/* print the values the spline returns			*/
#define SPLINE_PP 4		/* print the post-processed spline values		*/
#define SPLINE_PICK 5	/* print the post-processed spline and picks	*/
#define ROT_RT 6		/* prints realtime rotation values				*/
#define	ROT_FINAL 7		/* prints just the final rotation value			*/
#define ANI_MAT 8		/* prints the final animating matrix			*/
#define ROT_MAT 9		/* prints the rot animating matrix				*/
#define SCALE_MAT 10	/* prints the scaling animating matrix			*/
#define SEQ 11			/* prints sequencing information 				*/
#define SPLINE_PP2 12	/* prints results of 2nd post-processing stage	*/
#define LOAD 13			/* prints stages of file loading/merging		*/
#define VIEW 14			/* prints stages of viewport loading			*/
