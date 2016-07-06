#include "elk_private.h"

/*
 * The following code was taken from Elk-2.2's contrib directory
 */
static Object in, out;

static char *String_Eval(char *expr)
{
    Object str, res;
    char *p;
    GC_Node;
    static char buf[1024];

    str = Make_String(expr, strlen(expr));
    PORT(in)->name = str;
    PORT(in)->ptr = 0;
    res = General_Read(in, 0);
    GC_Link(res);
    res = Eval(res);
    (void)General_Print_Object(res, out, 1);
    str = P_Get_Output_String(out);
    p = Get_String(str);
    if (strlen(p) > sizeof buf - 1)
	p = "too long";
    strcpy(buf, p);
    GC_Unlink;
    return buf;
}

#if 0
/***********************************************************************
 *
 * Description: Elk_Eval() will execute the scheme instructions contained
 *		in the passed in parameter string. 
 *
 * Parameter:	strobj - string object specifying type of object to
 *		be created. Valid values for this parameter are:
 *			"cube", "sphere", "cylinder", "cone", "plane",
 *			"square", "light", and "csg".
 *
 * Scheme example: (object3d-create "sphere")
 *
 * Return value: Returns the newly created object. This object may be
 *		 passed to other scheme functions that accept an object
 *		 parameter.
 *
 ***********************************************************************/
char *
Elk_Eval(char *expr)
{
	char *newexpr;
	char *s;
	int slen;
	char *cc = "(if (call-with-current-continuation (lambda (c) (set! error-handler (lambda a (display \"Error in scheme expression\n\") (print a) (c #f))) #t))";
	
	slen = strlen(expr);
	newexpr = (char *) malloc(slen + strlen(cc) + 2);
	strcpy(newexpr, cc);
	strcat(newexpr, expr);
	strcat(newexpr, ")");
	s = String_Eval(newexpr);
	free(newexpr);
	return s;
}
#else
char *
Elk_Eval(char *expr)
{
    return String_Eval(expr);
}
#endif

void
init_eval()
{
    in = P_Open_Input_String(Make_String("", 0));
    Global_GC_Link(in);
    out = P_Open_Output_String();
    Global_GC_Link(out);
}
