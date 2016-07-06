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
**	load.h : definitions for the lexical analyser for load.c
*/

#define EOF_TOKEN		0
#define STRING			1
#define INT				2
#define FLOAT			3
#define MAINVIEW		4
#define CSGVIEW			5
#define LOOKFROM		6
#define LOOKAT			7
#define LOOKUP			8
#define VIEWDIST		9
#define EYEDIST			10
#define BASEOBJECTS		11
#define INSTANCES		12
#define WIREFRAME		13
#define ATTRIBUTES		14
#define TRANSFORMATION	15
#define COLOUR			16
#define DIFFUSE			17
#define SPECULAR		18
#define TRANSPARENCY	19
#define REFLECT			20
#define REFRACT			21
#define CAMERA			22
#define NONE			23
#define RAYSHADE		24
#define POVRAY			25
#define GENRAY			26
#define HFOV			27
#define VFOV			28
#define UP				29
#define RIGHT			30
#define SCREEN			31
#define MAGNIFY			32
#define LIGHT			33
#define AMBIENT			34
#define POSITION		35
#define REFERENCE		36
#define DEPENDENTS		37
#define CONSTRAINTS		38
#define PLANE			39
#define LINE			40
#define POINT			41
#define ACTIVE			42
#define LAYER			43
#define SCALE			44
#define ROTATE			45
#define AXES			46
#define ORIGIN			47
#define ALLIGN			48
#define SCENEDIR		49
#define UNION			50
#define INTERSECTION	51
#define DIFFERENCE		52
#define DEFAULT			53
#define MID				54
#define MAJOR			55
#define MINOR			56
#define VIEWPORT		57
#define DENSE			58
#define FULL			59
#define CSG				60
#define TARGET			61
#define INCLUDES		62
#define COMPRESS		63
#define RADIANCE		64
#define DECLARE			65
#define EXTEND			66
#define OPEN			67
#define VERS			68
#define NORMAL			69
#define UNKNOWN			70
#define GENSCAN			71
#define INTERNAL		72
#define OBJECT			73
#define MATRIX			74
#define MODE			75
#define RENDERMAN		76
#define ANI_ROT			77
#define ANI_SCALE		78
#define NEWFRAME		79

extern	int		line_num;
extern	long	lex_int;
extern	double	lex_float;
extern	char	*lex_string;

extern	FILE	*yyin;

extern char	*merge_filename;

extern int		yylex();
extern void		Load_Internal_File(FILE*, int);
extern void		Load_Simple_File(FILE*, int, int);
extern int		Load_View(Viewport*, char*, Boolean);
extern int		Load_Includes(Boolean);
extern int		Load_Declarations(Boolean);

#if LEX==flex
extern	void	yyrestart(FILE*);
#endif


