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
#include "grid.h"

#define CURSOR_WIDTH 15
#define BORDER_WIDTH 5
#define GRID_LINE_WIDTH 2

//Colors defined in RGB
#define GRID_LINE_COLOR 255,0,0
#define BORDER_COLOR 255,0,0
#define CURSOR_COLOR 0,255,0

static int lx, ly, ux, uy, cx, cy;
static int nc = 0, nr = 0;
static Window bw1 = 0, bw2, bw3, bw4, mw;
static Window *gridwins = NULL;

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

static Window create_win(int r, int g, int b) 
{
	Window w = XCreateWindow(dpy,
				 DefaultRootWindow(dpy),
				 0, 0, 1, 1,
				 0,
				 DefaultDepth(dpy, DefaultScreen(dpy)),
				 InputOutput,
				 DefaultVisual(dpy, DefaultScreen(dpy)),
				 CWOverrideRedirect | CWBackPixel | CWBackingPixel,
				 &(XSetWindowAttributes){
				 .backing_pixel = color(r,g,b),
				 .background_pixel = color(r,g,b),
				 .override_redirect = 1
				 });


	XMapWindow(dpy, w);
	return w;
}

static void hide() 
{
	int i;

	XUnmapWindow(dpy, bw1);
	XUnmapWindow(dpy, bw2);
	XUnmapWindow(dpy, bw3);
	XUnmapWindow(dpy, bw4);
	XUnmapWindow(dpy, mw);

	for(i = 0; i < (nr + nc - 2); i++)
		XUnmapWindow(dpy, gridwins[i]);

	set_cursor_visibility(1);
	XFlush(dpy);
}

static void draw() 
{
	int i,j;
	int rowh, colw;

	hide();
	set_cursor_visibility(0);

	XMoveWindow(dpy, mw, cx-(CURSOR_WIDTH/2), cy-(CURSOR_WIDTH/2));
	XResizeWindow(dpy, mw, CURSOR_WIDTH, CURSOR_WIDTH);

	XMoveWindow(dpy, bw2, lx, ly);
	XResizeWindow(dpy, bw2, ux-lx, BORDER_WIDTH);

	XMoveWindow(dpy, bw1, lx, uy-BORDER_WIDTH);
	XResizeWindow(dpy, bw1, ux-lx, BORDER_WIDTH);

	XMoveWindow(dpy, bw3, lx, ly);
	XResizeWindow(dpy, bw3, BORDER_WIDTH, uy-ly);

	XMoveWindow(dpy, bw4, ux-BORDER_WIDTH, ly);
	XResizeWindow(dpy, bw4, BORDER_WIDTH, uy-ly);

	XMoveWindow(dpy, bw4, ux-BORDER_WIDTH, ly);
	XResizeWindow(dpy, bw4, BORDER_WIDTH, uy-ly);

	XMapRaised(dpy, bw1);
	XMapRaised(dpy, bw2);
	XMapRaised(dpy, bw3);
	XMapRaised(dpy, bw4);

	rowh = (uy-ly)/nr;
	colw = (ux-lx)/nc;

	for(i = 0; i < nc-1; i++) {
		XMoveWindow(dpy,
			    gridwins[i],
			    lx + (i+1) * colw - (GRID_LINE_WIDTH/2), ly);
		XResizeWindow(dpy, gridwins[i], GRID_LINE_WIDTH, uy-ly);
		XMapRaised(dpy, gridwins[i]);
	}

	for(i = 0; i < nr-1; i++) {
		XMoveWindow(dpy,
			    gridwins[nc+i-1],
			    lx, ly + (i+1) * rowh - (GRID_LINE_WIDTH/2));
		XResizeWindow(dpy, gridwins[nc+i-1], ux-lx, GRID_LINE_WIDTH);
		XMapRaised(dpy, gridwins[nc+i-1]);
	}

	XMapRaised(dpy, mw);

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

	draw();
	XFlush(dpy);
}

static void click(int btn) 
{
	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
	XSync(dpy, False);
}

static void focus_sector(int c, int r) 
{
	const int threshold = CURSOR_WIDTH;
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
}

void grid(Display *_dpy, int _nr, int _nc, int movement_increment, int startrow, int startcol, struct grid_keys *keys) 
{
	dpy = _dpy;

	int clicked = 0;
	int xfd = XConnectionNumber(dpy);

	if(!bw1) {
		bw1 = create_win(BORDER_COLOR);
		bw2 = create_win(BORDER_COLOR);
		bw3 = create_win(BORDER_COLOR);
		bw4 = create_win(BORDER_COLOR);
		mw = create_win(CURSOR_COLOR);

	}

	if(nc != _nc || nr != _nr) {
		int i;

		nc = _nc;
		nr = _nr;

		gridwins = malloc((nr + nc - 2) * sizeof(Window));
		for(i = 0; i < (nr + nc - 2); i++)
			gridwins[i] = create_win(GRID_LINE_COLOR);
	}

	XGrabKeyboard(dpy, DefaultRootWindow(dpy), 1, GrabModeAsync, GrabModeAsync, CurrentTime);
	reset();
	focus_sector(startcol, startrow);
	draw();

	while(1) {
		const int double_click_timeout = 300;
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(xfd, &fds);

		select(xfd+1,
		       &fds,
		       NULL,
		       NULL,
		       clicked ? &(struct timeval){0, double_click_timeout*1000} : NULL);

		if(!XPending(dpy)) {
			hide();
			XUngrabKeyboard(dpy, CurrentTime);

			return;
		}

		while(XPending(dpy)) {
			XEvent ev;
			XNextEvent(dpy, &ev);

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

				if(keys->button1 == ev.xkey.keycode ||
				   keys->button2 == ev.xkey.keycode ||
				   keys->button3 == ev.xkey.keycode) {
					hide();
					XUngrabKeyboard(dpy, CurrentTime);

					click(keys->button1 == ev.xkey.keycode ? 1 : 
					      keys->button2 == ev.xkey.keycode ? 2 : 3);

					//FIXME: Ugly kludge to fix the race condtion between XTestFakeButtonEvent and XMapRaised
					//TODO: Investigate the root cause (possibly related to WM window mapping and pointer grabs).
					usleep(10000);

					if(XGrabKeyboard(dpy, DefaultRootWindow(dpy), 1, GrabModeAsync, GrabModeAsync, CurrentTime)) {
						fprintf(stderr, "Failed to reacquire keyboard grab after click.\n");
						return;
					}
					clicked++;
					hide();
				}


				if(keys->close_key == ev.xkey.keycode) {
					XUngrabKeyboard(dpy, CurrentTime);
					hide();
					return;
				}

				for (int i = 0; i < nc; i++)
					for (int j = 0; j < nr; j++)
						if(keys->grid[j][i] == ev.xkey.keycode) {
							focus_sector(i, j);
							draw();
						}
			}
		}
	}
}
