#include "elk_private.h"

extern int init_eval();
extern int Elk_Init(int,char **,int,char*);
extern char *Elk_Eval(char *expr);
extern int init_callbacks();

int
init_elk(int ac, char **av)
{
	int fakeac = 1;
	char *fakeav[1];

	fakeav[0] = av[0];
	Elk_Init(fakeac, fakeav, 0, "init.scm");
	init_eval();
	/*
	 * Now we define various functional interfaces for calling
	 * from scheme to C
	 */
	init_callbacks();
	Elk_Eval("(startup)\n");
	return 0;
}
