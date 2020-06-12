#include "dbg.h"
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

static int debug = 0;

void init_dbg()
{
	debug = 1;
}

void _dbg(const char *function, const char *fmt, ...)
{
	if(debug) {
		va_list lst;
		va_start(lst, fmt);
		fprintf(stderr, "%s: ", function);
		vfprintf(stderr, fmt, lst);
		fprintf(stderr, "\n");
		fflush(stderr);
		va_end(lst);
	}
}
