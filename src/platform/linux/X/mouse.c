#include "X.h"

static int hidden = 0;

void x_mouse_up(int btn)
{
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
	XSync(dpy, False);
}

void x_mouse_down(int btn)
{
	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XSync(dpy, False);
}

void x_mouse_click(int btn)
{
	if (x_active_mods & MOD_SHIFT)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Shift_L), 1, CurrentTime);
	if (x_active_mods & MOD_CONTROL)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Control_L), 1, CurrentTime);
	if (x_active_mods & MOD_META)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Meta_L), 1, CurrentTime);
	if (x_active_mods & MOD_ALT)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Alt_L), 1, CurrentTime);

	XSync(dpy, False);

	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);

	XSync(dpy, False);

	if (x_active_mods & MOD_SHIFT)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Shift_L), 0, CurrentTime);
	if (x_active_mods & MOD_CONTROL)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Control_L), 0, CurrentTime);
	if (x_active_mods & MOD_META)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Meta_L), 0, CurrentTime);
	if (x_active_mods & MOD_ALT)
		XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Alt_L), 0, CurrentTime);

	XSync(dpy, False);
}

void x_mouse_move(struct screen *scr, int x, int y)
{
	XTestFakeMotionEvent(dpy,
			     DefaultScreen(dpy),
			     scr->x + x, scr->y + y, 0);

	XSync(dpy, False);
}

void x_mouse_get_position(struct screen **_scr, int *_x, int *_y)
{
	size_t i;
	Window chld, root;
	int _;
	unsigned int _u;
	int x, y;

	/* Obtain absolute pointer coordinates */
	XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &chld, &x, &y, &_, &_,
		      &_u);

	for (i = 0; i < nr_xscreens; i++) {
		struct screen *scr = &xscreens[i];

		if ((x >= scr->x) && (x <= (scr->x + scr->w)) &&
		    (y >= scr->y) && (y <= (scr->y + scr->h))) {
			if (_scr)
				*_scr = scr;

			if (_x)
				*_x = x - scr->x;

			if (_y)
				*_y = y - scr->y;

			return;
		}
	}

	assert(0);
}

void x_mouse_hide()
{
	if (hidden)
		return;

	XFixesHideCursor(dpy, DefaultRootWindow(dpy));
	XSync(dpy, False);
	hidden = 1;
}

void x_mouse_show()
{
	if (!hidden)
		return;

	XFixesShowCursor(dpy, DefaultRootWindow(dpy));
	XSync(dpy, False);
	hidden = 0;
}
