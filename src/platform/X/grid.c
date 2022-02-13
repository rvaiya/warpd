/* Copyright © 2019 Raheman Vaiya.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "../../platform.h"
#include "assert.h"
#include "impl.h"
#include <X11/Xlib.h>

static Window bw1, bw2, bw3, bw4;
static Window gridwins[32];
static size_t nc, nr;
static int thickness;

void grid_hide()
{
	size_t i;

	window_hide(bw1);
	window_hide(bw2);
	window_hide(bw3);
	window_hide(bw4);
	//window_hide(mw);

	for(i = 0; i < (nr + nc - 2); i++)
		window_hide(gridwins[i]);

	window_commit();
}

static void redraw(int ux, int uy, int lx, int ly)
{
	size_t i;
	int rowh, colw;


	/* TODO: fixme */
	//XMoveWindow(dpy, mw, cx-(cursor_width/2), cy-(cursor_width/2));
	//XResizeWindow(dpy, mw, cursor_width, cursor_width);

	XMoveWindow(dpy, bw2, lx, ly);
	XResizeWindow(dpy, bw2, ux-lx, thickness);

	XMoveWindow(dpy, bw1, lx, uy-thickness);
	XResizeWindow(dpy, bw1, ux-lx, thickness);

	XMoveWindow(dpy, bw3, lx, ly);
	XResizeWindow(dpy, bw3, thickness, uy-ly);

	XMoveWindow(dpy, bw4, ux-thickness, ly);
	XResizeWindow(dpy, bw4, thickness, uy-ly);

	XMoveWindow(dpy, bw4, ux-thickness, ly);
	XResizeWindow(dpy, bw4, thickness, uy-ly);

	rowh = (uy-ly)/nr;
	colw = (ux-lx)/nc;

	for(i = 0; i < nc-1; i++) {
		XMoveWindow(dpy,
			    gridwins[i],
			    lx + (i+1) * colw - (thickness/2), ly);
		XResizeWindow(dpy, gridwins[i], thickness, uy-ly);
	}

	for(i = 0; i < nr-1; i++) {
		XMoveWindow(dpy,
			    gridwins[nc+i-1],
			    lx, ly + (i+1) * rowh - (thickness/2));
		XResizeWindow(dpy, gridwins[nc+i-1], ux-lx, thickness);
	}

	window_show(bw1);
	window_show(bw2);
	window_show(bw3);
	window_show(bw4);

	for(i = 0; i < nc+nr-2; i++) 
		window_show(gridwins[i]);

	window_commit();
	//window_show(dpy, mw);
}

void init_grid(const char *color, size_t _thickness, size_t _nc, size_t _nr)
{
	size_t i;

	nc = _nc;
	nr = _nr;
	thickness = _thickness;

	bw1 = create_window(color, 0, 0, 0, 0);
	bw2 = create_window(color, 0, 0, 0, 0);
	bw3 = create_window(color, 0, 0, 0, 0);
	bw4 = create_window(color, 0, 0, 0, 0);

	assert((nr + nc - 2) < sizeof(gridwins)/sizeof(gridwins[0]));

	for(i = 0; i < (nr + nc - 2); i++)
		gridwins[i] = create_window(color, 0, 0, 0, 0);
}

void grid_draw(int x, int y, int w, int h)
{
	mouse_hide();
	redraw(x+w, y+h, x, y);
}
