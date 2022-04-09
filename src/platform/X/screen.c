#include "X.h"

struct screen screens[32];
size_t nr_screens = 0;

void window_set_color(Window w, const char *color)
{
	uint32_t col = parse_xcolor(color, NULL);

	XChangeWindowAttributes(dpy, w,
				CWBackPixel,
				&(((XSetWindowAttributes) { .background_pixel = col })));
}

void init_screens()
{
	Window chld, root;
	int n, x, y, _;
	unsigned int _u;
	XineramaScreenInfo *xscreens;

	/* Obtain absolute pointer coordinates */
	XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &chld, &x, &y, &_, &_,
		      &_u);

	xscreens = XineramaQueryScreens(dpy, &n);
	for (int i = 0; i < n; i++) {
		size_t j;

		struct screen *scr = &screens[nr_screens++];

		scr->y = xscreens[i].y_org;
		scr->x = xscreens[i].x_org;

		scr->w = xscreens[i].width;
		scr->h = xscreens[i].height;

		for (j = 0; j < MAX_BOXES; j++) {
			scr->boxes[j].win = create_window("#000000");
			XMapWindow(dpy, scr->boxes[j].win);
		}
	}
}

void screen_list(struct screen *rscreens[MAX_SCREENS], size_t *n)
{
	size_t i;

	for (i = 0; i < nr_screens; i++)
		rscreens[i] = &screens[i];

	*n = nr_screens;
}

void screen_get_dimensions(struct screen *scr, int *w, int *h)
{
	*w = scr->w;
	*h = scr->h;
}

void screen_clear(struct screen *scr)
{
	size_t i;

	for (i = 0; i < scr->nr_boxes; i++)
		XMoveWindow(dpy, scr->boxes[i].win, -1E6, -1E6);

	XMoveWindow(dpy, scr->hintwin, -1E6, -1E6);
	XMoveWindow(dpy, scr->cached_hintwin, -1E6, -1E6);

	scr->nr_boxes = 0;
}

void screen_draw_box(struct screen *scr, int x, int y, int w, int h, const char *color) 
{ 
	assert(scr->nr_boxes < MAX_BOXES);

	struct box *box = &scr->boxes[scr->nr_boxes++];

	if (strcmp(box->color, color)) {
		window_set_color(box->win, color);
		strcpy(box->color, color);
	};

	XMoveResizeWindow(dpy, box->win, scr->x + x, scr->y + y, w, h);
	XRaiseWindow(dpy, box->win);
}

