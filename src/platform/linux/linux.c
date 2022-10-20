#include <stdio.h>
#include <stdlib.h>

void x_platform_init();
void wl_platform_init();

#ifndef WARPD_X
void x_platform_init()
{
	fprintf(stderr, "ERROR: warpd compiled without X support\n");
	exit(-1);
}
#endif

#ifndef WARPD_WAYLAND
void wl_platform_init()
{
	fprintf(stderr, "ERROR: warpd compiled without wayland support\n");
	exit(-1);
}
#endif

void platform_init()
{
	if (getenv("WAYLAND_DISPLAY"))
		wl_platform_init();
	else
		x_platform_init();
}
