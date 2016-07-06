/*
**	Sced: A Constraint Based Object Scene Editor
**
**	load_lex.l : The lex file that does lexical analysis for load.c
**
*/

%p 4000

%{

#define PATCHLEVEL 0

#include <sced.h>
#include <load.h>
#if ( HAVE_STRING_H )
#include <string.h>
#elif ( HAVE_STRINGS_H )
#include <strings.h>
#endif

void comment(void);
void read_string(void);

long   lex_int;
double lex_float;
char   *lex_string;

int		line_num = 1;

%}


SPACE       ("\t"|" ")
LETTER		[A-Z,a-z]
DIGIT       [0-9]
DOT			"."
IDENTIFIER  ({LETTER}|{DIGIT})({LETTER}|{DIGIT}|"_")*
INTEGER     "-"?{DIGIT}+
FRACTION    {DOT}{INTEGER}
EXPONENT    ([eE][+-]?{INTEGER})
REAL        {INTEGER}({FRACTION}{EXPONENT}?|{EXPONENT})|{FRACTION}{EXPONENT}?
NEWLINE     "\n"

%%


{SPACE}				;
"#"					comment();
"\""				{
						read_string();
						return STRING;
					};
{REAL}				{
						lex_float=atof(yytext);
						return(FLOAT);
					};
{INTEGER}			{
						lex_int=atol(yytext);
						return(INT);
					};
"MainViewport"		return MAINVIEW;
"CSGViewport"		return CSGVIEW;
"Viewport"			return VIEWPORT;
"LookFrom"			return LOOKFROM;
"LookAt"			return LOOKAT;
"LookUp"			return LOOKUP;
"ViewDist"			return VIEWDIST;
"EyeDist"			return EYEDIST;
"Mode"				return MODE;
"BaseObjects"		return BASEOBJECTS;
"Instances"			return INSTANCES;
"Transformation"	return TRANSFORMATION;
"Matrix"			return MATRIX;
"Scale"				return SCALE;
"Rotate"			return ROTATE;
"Union"				return UNION;
"Intersection"		return INTERSECTION;
"Difference"		return DIFFERENCE;
"Wireframe"			return WIREFRAME;
"Attributes"		return ATTRIBUTES;
"Color"				return COLOUR;
"Colour"			return COLOUR;
"Diffuse"			return DIFFUSE;
"Specular"			return SPECULAR;
"Reflect"			return REFLECT;
"Refract"			return REFRACT;
"Transparency"		return TRANSPARENCY;
"Camera"			return CAMERA;
"None"				return NONE;
"Rayshade"			return RAYSHADE;
"POVray"			return POVRAY;
"Genray"			return GENRAY;
"Genscan"			return GENSCAN;
"Radiance"			return RADIANCE;
"Renderman"			return RENDERMAN;
"HFOV"				return HFOV;
"VFOV"				return VFOV;
"Up"				return UP;
"Right"				return RIGHT;
"Screen"			return SCREEN;
"Magnify"			return MAGNIFY;
"Light"				return LIGHT;
"Ambient"			return AMBIENT;
"Position"			return POSITION;
"Reference"			return REFERENCE;
"Dependents"		return DEPENDENTS;
"Constraints"		return CONSTRAINTS;
"Plane"				return PLANE;
"Line"				return LINE;
"Point"				return POINT;
"Active"			return ACTIVE;
"Axes"				return AXES;
"Origin"			return ORIGIN;
"Allign"			return ALLIGN;
"Layer"				return LAYER;
"Directory"			return SCENEDIR;
"Default"			return DEFAULT;
"Mid"				return MID;
"Major"				return MAJOR;
"Minor"				return MINOR;
"Dense"				return DENSE;
"Full"				return FULL;
"CSG"				return CSG;
"Target"			return TARGET;
"Include"			return INCLUDES;
"Compress"			return COMPRESS;
"Declare"			return DECLARE;
"Extend"			return EXTEND;
"Open"				return OPEN;
"Version"			return VERS;
"Normal"			return NORMAL;
"Internal"			return INTERNAL;
"Object"			return OBJECT;
"Ani_Rotation"		return ANI_ROT;
"Ani_Scale"			return ANI_SCALE;
"NewFrame"			return NEWFRAME;
"\n"				line_num++;
{IDENTIFIER}		{
						lex_string = yytext;
						return UNKNOWN;
					}

%%

void comment()
{
    int ch;

    while (((ch=input()) != '\n') && (ch != 0));
	line_num++;
}

void read_string()
{
	int ch;
	int max, i;
	int	last_ch = '\0';

	max = 24;
	lex_string = malloc(max);
	i = 0;

	while ((((ch=input()) != '"') || (last_ch == '\\')) && (ch != 0))
	{
		if (i == max)
		{
			max += 24;
			lex_string = realloc(lex_string, max);
		}

		if ( ch == '\n' ) line_num++;

		if ( ( last_ch == '\\' ) && ( ch != '"' ) )
			lex_string[i++] = last_ch;

		if ( ch != '\\' )
			lex_string[i++] = ch;

		last_ch = ch;
	}
	lex_string[i] = '\0';
}

