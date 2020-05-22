/*
 * ---------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 * Original Author: Raheman Vaiya
 * ---------------------------------------------------------------------
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

static int lx, ly, ux, uy, cx, cy;
static int nc = 0, nr = 0;
static Window bw1 = 0, bw2, bw3, bw4, mw;
static Window *gridwins = NULL;

static int cursor_width = 0;
static int line_width = 0;
static int hidden = 0;

static Display *dpy;

//XFixes* functions are not idempotent (calling them more than
//once crashes the client, so we need this wrapper function).

static void set_cursor_visibility(int visible) 
{
	static int state = 1;

	if(visible == state) return;

	if(visible)
		XFixesShowCursor(dpy, DefaultRootWindow(dpy));
	else
		XFixesHideCursor(dpy, DefaultRootWindow(dpy));

	XFlush(dpy);
	state = visible;
}

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


	XMapWindow(dpy, w);
	return w;
}

static void hide() 
{
}

static void redraw() 
{
	int i,j;
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
		set_cursor_visibility(1);
		XFlush(dpy);

		return;
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
	set_cursor_visibility(0);

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

	XWarpPointer(dpy, 0, DefaultRootWindow(dpy), 0, 0, 0, 0, cx, cy);
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

static void click(int btn) 
{
	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
	XSync(dpy, False);
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

void grid(Display *_dpy,
	int _nr,
	int _nc,
	int _line_width,
	int _cursor_width,
	int double_click_timeout,
	int movement_increment,
	int startrow,
	int startcol,
	const char *gridcol,
	const char *mousecol,
	struct grid_keys *keys) 
{
	dpy = _dpy;
	line_width = _line_width;
	cursor_width = _cursor_width;

	int clicked = 0;
	int xfd = XConnectionNumber(dpy);

	if(!bw1) {
		bw1 = create_win(gridcol);
		bw2 = create_win(gridcol);
		bw3 = create_win(gridcol);
		bw4 = create_win(gridcol);
		mw = create_win(mousecol);

	}

	if(nc != _nc || nr != _nr) {
		int i;

		nc = _nc;
		nr = _nr;

		gridwins = malloc((nr + nc - 2) * sizeof(Window));
		for(i = 0; i < (nr + nc - 2); i++)
			gridwins[i] = create_win(gridcol);
	}

	XGrabKeyboard(dpy, DefaultRootWindow(dpy), 1, GrabModeAsync, GrabModeAsync, CurrentTime);
	reset();
	focus_sector(startrow, startcol);
	redraw();

	dbg("Entering main grid loop.");
	while(1) {
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(xfd, &fds);

		select(xfd+1,
		       &fds,
		       NULL,
		       NULL,
		       (clicked && double_click_timeout) ? &(struct timeval){0, double_click_timeout*1000} : NULL);

		if(!XPending(dpy)) goto exit;

		while(XPending(dpy)) {
			XEvent ev;
			XNextEvent(dpy, &ev);

			if(ev.type == Expose) redraw();

			if(ev.type == KeyPress) {
				KeySym sym = XKeycodeToKeysym(dpy, ev.xkey.keycode, 0);

				if(keys->up == ev.xkey.keycode)
					rel_warp(0, -movement_increment);
				if(keys->down == ev.xkey.keycode)
					rel_warp(0, movement_increment);
				if(keys->left == ev.xkey.keycode)
					rel_warp(-movement_increment, 0);
				if(keys->right == ev.xkey.keycode)
					rel_warp(movement_increment, 0);

				for(int i=0;i<sizeof keys->buttons;i++)
					if(keys->buttons[i] == ev.xkey.keycode) {
						hidden = 1;
						redraw();
						XUngrabKeyboard(dpy, CurrentTime);

						click(i+1);

						//FIXME: Ugly kludge to fix the race condtion between XTestFakeButtonEvent and XMapRaised
						//TODO: Investigate the root cause (possibly related to WM window mapping and pointer grabs).
						usleep(10000);

						if(XGrabKeyboard(dpy, DefaultRootWindow(dpy), 1, GrabModeAsync, GrabModeAsync, CurrentTime)) {
							fprintf(stderr, "Failed to reacquire keyboard grab after click.\n");
							return;
						}
						if(i < 3) clicked++; //Don't timeout on scroll buttons.
					}

				if(keys->close_key == ev.xkey.keycode)
					goto exit;

				for (int i = 0; i < nr; i++)
					for (int j = 0; j < nc; j++)
						if(keys->grid[i*nc+j] == ev.xkey.keycode) {
							focus_sector(i, j);
							redraw();
						}
			}
		}
	}

exit:
	hidden = 1;
	redraw();
	XUngrabKeyboard(dpy, CurrentTime);
	return;
}
