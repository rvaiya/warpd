/*
 * warpd - A keyboard-driven modal pointer.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>

#include "impl.h"

static Window curwin;

void cursor_show(int x, int y)
{
	XMoveWindow(dpy, curwin, x + 1, y + 1);
	XMapRaised(dpy, curwin);
}

void cursor_hide() { XUnmapWindow(dpy, curwin); }

void init_cursor(const char *color, size_t sz)
{
	curwin = create_window(color, 0, 0, sz, sz);
}
