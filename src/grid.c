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

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xfixes.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "grid.h"
#include "dbg.h"
#include "input.h"

static int lx, ly, ux, uy, cx, cy;
static int nc = 0, nr = 0;
static Window bw1 = 0, bw2, bw3, bw4, mw;
static Window *gridwins = NULL;

static int cursor_width = 0;
static int line_width = 0;
static int hidden = 0;

struct grid_keys *keys;
static int movement_increment;

static Display *dpy;

static int hex_to_rgb(const char *str, uint8_t *r, uint8_t *g, uint8_t *b) 
{
#define X2B(c) ((c >= '0' && c <= '9') ? (c & 0xF) : (((c | 0x20) - 'a') + 10))

	if(str == NULL) return 0;
	str = (*str == '#') ? str + 1 : str;

	if(strlen(str) != 6)
		return -1;

	*r = X2B(str[0]);
	*r <<= 4;
	*r |= X2B(str[1]);

	*g = X2B(str[2]);
	*g <<= 4;
	*g |= X2B(str[3]);

	*b = X2B(str[4]);
	*b <<= 4;
	*b |= X2B(str[5]);

	return 0;
}

static uint32_t color(uint8_t red, uint8_t green, uint8_t blue) 
{
	XColor col;
	col.red = (int)red << 8;
	col.green = (int)green << 8;
	col.blue = (int)blue << 8;
	col.flags = DoRed | DoGreen | DoBlue;

	assert(XAllocColor(dpy, XDefaultColormap(dpy, DefaultScreen(dpy)), &col));
	return col.pixel;
}

static Window create_win(const char *col) 
{
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	hex_to_rgb(col, &r, &g, &b);

	Window w = XCreateWindow(dpy,
				 DefaultRootWindow(dpy),
				 0, 0, 1, 1,
				 0,
				 DefaultDepth(dpy, DefaultScreen(dpy)),
				 InputOutput,
				 DefaultVisual(dpy, DefaultScreen(dpy)),
				 CWOverrideRedirect | CWBackPixel | CWBackingPixel | CWEventMask,
				 &(XSetWindowAttributes){
				 .backing_pixel = color(r,g,b),
				 .background_pixel = color(r,g,b),
				 .override_redirect = 1,
				 .event_mask = ExposureMask,
				 });


	return w;
}

static void redraw() 
{
	int i;
	int rowh, colw;
	static int mapped = 0;

	if(hidden) {
		XUnmapWindow(dpy, bw1);
		XUnmapWindow(dpy, bw2);
		XUnmapWindow(dpy, bw3);
		XUnmapWindow(dpy, bw4);
		XUnmapWindow(dpy, mw);

		for(i = 0; i < (nr + nc - 2); i++)
			XUnmapWindow(dpy, gridwins[i]);

		mapped = 0;
		XFlush(dpy);

		return;
	}

	XMoveWindow(dpy, mw, cx-(cursor_width/2), cy-(cursor_width/2));
	XResizeWindow(dpy, mw, cursor_width, cursor_width);

	XMoveWindow(dpy, bw2, lx, ly);
	XResizeWindow(dpy, bw2, ux-lx, line_width);

	XMoveWindow(dpy, bw1, lx, uy-line_width);
	XResizeWindow(dpy, bw1, ux-lx, line_width);

	XMoveWindow(dpy, bw3, lx, ly);
	XResizeWindow(dpy, bw3, line_width, uy-ly);

	XMoveWindow(dpy, bw4, ux-line_width, ly);
	XResizeWindow(dpy, bw4, line_width, uy-ly);

	XMoveWindow(dpy, bw4, ux-line_width, ly);
	XResizeWindow(dpy, bw4, line_width, uy-ly);

	rowh = (uy-ly)/nr;
	colw = (ux-lx)/nc;

	for(i = 0; i < nc-1; i++) {
		XMoveWindow(dpy,
			    gridwins[i],
			    lx + (i+1) * colw - (line_width/2), ly);
		XResizeWindow(dpy, gridwins[i], line_width, uy-ly);
	}

	for(i = 0; i < nr-1; i++) {
		XMoveWindow(dpy,
			    gridwins[nc+i-1],
			    lx, ly + (i+1) * rowh - (line_width/2));
		XResizeWindow(dpy, gridwins[nc+i-1], ux-lx, line_width);
	}

	if(!mapped) {
		XMapRaised(dpy, bw1);
		XMapRaised(dpy, bw2);
		XMapRaised(dpy, bw3);
		XMapRaised(dpy, bw4);
		for(i = 0; i < nc+nr-2; i++) XMapRaised(dpy, gridwins[i]);
		XMapRaised(dpy, mw);

		mapped = 1;
	}

	XFlush(dpy);
}

static void rel_warp(int x, int y) 
{
	lx += x;
	ly += y;
	cx += x;
	cy += y;
	ux += x;
	uy += y;

	redraw();
}

static void focus_sector(int r, int c) 
{
	const int threshold = cursor_width;
	int col_sz = (ux-lx)/nc;
	int row_sz = (uy-ly)/nr;

	int olx = lx;
	int oly = ly;
	int oux = ux;
	int ouy = uy;

	if(c != -1) {
		lx += col_sz * c;
		ux = lx + col_sz;
	}

	if(r != -1) {
		ly += row_sz * r;
		uy = ly + row_sz;
	}

	if((ux -lx) < threshold) {
		ux = oux;
		lx = olx;
	}

	if((uy -ly) < threshold) {
		uy = ouy;
		ly = oly;
	}

	cy = (uy + ly) / 2;
	cx = (ux + lx) / 2;
}

static void reset() 
{
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &attr);

	lx = 0;
	ly = 0;
	uy = attr.height;
	ux = attr.width;
	cx = ux/2;
	cy = uy/2;
	hidden = 0;
}

uint16_t grid_warp(int startrow, int startcol)
{
	reset();
	focus_sector(startrow, startcol);
	redraw();

	dbg("Entering main grid loop.");
	while(1) {
		uint16_t keyseq = input_next_key(0);

		if(keys->up == keyseq)
			rel_warp(0, -movement_increment);
		else if(keys->down == keyseq)
			rel_warp(0, movement_increment);
		else if(keys->left == keyseq)
			rel_warp(-movement_increment, 0);
		else if(keys->right == keyseq)
			rel_warp(movement_increment, 0);
		else {
			for (int i = 0; i < nr; i++)
				for (int j = 0; j < nc; j++)
					if(keys->grid[i*nc+j] == keyseq) {
						focus_sector(i, j);
						redraw();
					}

			for (size_t i = 0; i < sizeof keys->exit / sizeof keys->exit[0]; i++) {
				if(keys->exit[i] == keyseq) {
					hidden = 1;
					redraw();

					XWarpPointer(dpy, 0, DefaultRootWindow(dpy), 0, 0, 0, 0, cx, cy);
					XFlush(dpy);

					return keyseq;
				}
			}
		}
	}
}

void init_grid(Display *_dpy,
	       int _nr,
	       int _nc,
	       int _line_width,
	       int _cursor_width,
	       int _movement_increment,
	       const char *gridcol,
	       const char *mousecol,
	       struct grid_keys *_keys) 
{
	int i;

	dpy = _dpy;
	line_width = _line_width;
	cursor_width = _cursor_width;
	keys = _keys;
	movement_increment = _movement_increment;
	nr = _nr;
	nc = _nc;

	bw1 = create_win(gridcol);
	bw2 = create_win(gridcol);
	bw3 = create_win(gridcol);
	bw4 = create_win(gridcol);
	mw = create_win(mousecol);

	gridwins = malloc((nr + nc - 2) * sizeof(Window));
	for(i = 0; i < (nr + nc - 2); i++)
		gridwins[i] = create_win(gridcol);

}
