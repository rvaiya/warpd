#include "../../platform.h"

void x_platform_init();

void platform_init()
{
	x_platform_init();
	printf("initializing linux\n");
}
