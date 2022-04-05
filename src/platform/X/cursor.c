/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "X.h"

static Window	win;
static size_t	sz;

void cursor_draw(struct screen *scr, int x, int y) {
	/* 
	 * We place the cursor next to the pointer to prevent the cursor
	 * window from occluding it.
	 */
	XMoveWindow(dpy, win, scr->x + x - 1, scr->y + y + sz/2);
	XMapRaised(dpy, win);
}

void cursor_hide()
{
	XUnmapWindow(dpy, win);
}

void init_cursor(const char *color, size_t _sz)
{
	win = create_window(color);

	sz = _sz;

	/*
	 * Poke a 1 pixel hole in the center so
	 * we can center the cursor without
	 * occluding the pointer
	 *
	 * TODO: fixme, this introduces noticable
	 * lag when scrubbing :(.
	 */
	//Pixmap	mask = XCreatePixmap(dpy, win, sz, sz, 1);
	//GC	gc = XCreateGC(dpy, mask, 0, NULL);
	//XSetForeground(dpy, gc, 1);
	//XFillRectangle(dpy, mask, gc, 0, 0, sz, sz);
	//XSetForeground(dpy, gc, 0);
	//XFillRectangle(dpy, mask, gc, sz/2, sz/2, 1, 1);

	//XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, mask, ShapeSet);

	XResizeWindow(dpy, win, sz, sz);
}
