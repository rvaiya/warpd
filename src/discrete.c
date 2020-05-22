#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <stdlib.h>
#include <stdio.h>
#include "discrete.h"

static Display *dpy;

static void rel_warp(int x, int y) 
{
	XWarpPointer(dpy, 0, None, 0, 0, 0, 0, x, y);
}

static void click(int btn) 
{
	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
	XSync(dpy, False);
}

void discrete(Display *_dpy, const int inc, const int double_click_timeout, struct discrete_keys *keys)
{
	dpy = _dpy;
	int clicked = 0;
	int xfd = XConnectionNumber(dpy);

	if(XGrabKeyboard(dpy, DefaultRootWindow(dpy), False, GrabModeAsync, GrabModeAsync, CurrentTime)) {
		fprintf(stderr, "Failed to grab the keyboard\n");
		return;
	}

	while(1) {
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(xfd, &fds);

		select(xfd+1,
		       &fds,
		       NULL,
		       NULL,
		       (clicked && double_click_timeout) ? &(struct timeval){0, double_click_timeout*1000} : NULL);

		if(!XPending(dpy)) goto cleanup;

		while(XPending(dpy)) {
			XEvent ev;
			XNextEvent(dpy, &ev);

			if(ev.type == KeyPress) {
				int i;

				if(ev.xkey.keycode == keys->up)
					rel_warp(0, -inc);
				if(ev.xkey.keycode == keys->down)
					rel_warp(0, inc);
				if(ev.xkey.keycode == keys->left)
					rel_warp(-inc, 0);
				if(ev.xkey.keycode == keys->right)
					rel_warp(inc, 0);
				if(ev.xkey.keycode == keys->quit)
					goto cleanup;

				for (i = 0; i < sizeof keys->buttons/sizeof keys->buttons[0]; i++)
					if(keys->buttons[i] == ev.xkey.keycode) {
						if(i < 3) clicked++; //Don't timeout on scroll
						click(i+1);
						break;
					}
			}
		}
	}

cleanup:
	XUngrabKeyboard(dpy, CurrentTime);
	return;
}
