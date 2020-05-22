#ifndef _H_DISCRETE_
#define _H_DISCRETE_

#include <X11/X.h>

struct discrete_keys {
	KeyCode buttons[8];

	KeyCode up;
	KeyCode down;
	KeyCode left;
	KeyCode right;

	KeyCode quit;
};

void discrete(Display *_dpy, const int inc, const int double_click_timeout, struct discrete_keys *keys);
#endif
