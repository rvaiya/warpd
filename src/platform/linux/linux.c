#include <stdio.h>
#include <stdlib.h>
#include "../../platform.h"

void x_init();
void wayland_init();

#ifndef WARPD_X
void x_init()
{
	fprintf(stderr, "ERROR: warpd compiled without X support\n");
	exit(-1);
}
#endif

#ifndef WARPD_WAYLAND
void wayland_init()
{
	fprintf(stderr, "ERROR: warpd compiled without wayland support\n");
	exit(-1);
}
#endif

void platform_run(int (*main) (struct platform *platform))
{
	struct platform platform;

	if (getenv("WAYLAND_DISPLAY"))
		wayland_init(&platform);
	else
		x_init(&platform);

	exit(main(&platform));
}
