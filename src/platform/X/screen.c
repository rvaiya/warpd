#include "X.h"

struct screen screens[32];
size_t	      nr_screens = 0;

void init_screens()
{
	Window		    chld, root;
	int		    n, x, y, _;
	unsigned int	    _u;
	XineramaScreenInfo *xscreens;

	/* Obtain absolute pointer coordinates */
	XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &chld, &x, &y, &_, &_,
		      &_u);

	xscreens = XineramaQueryScreens(dpy, &n);
	for (int i = 0; i < n; i++) {
		struct screen *scr = &screens[nr_screens++];

		scr->y = xscreens[i].y_org;
		scr->x = xscreens[i].x_org;

		scr->w = xscreens[i].width;
		scr->h = xscreens[i].height;
	}
}

void screen_get_dimensions(struct screen *scr, int *w, int *h)
{
	*w = scr->w;
	*h = scr->h;
}
