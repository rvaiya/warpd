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
 *
 * Original Author: Raheman Vaiya
 *
 * ---------------------------------------------------------------------
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xfixes.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <limits.h>
#include "keynames.h"

#define MAX_BINDINGS 200

#define CURSOR_WIDTH 15
#define BORDER_WIDTH 5
#define GRID_LINE_WIDTH 2

//Colors defined in RGB
#define GRID_LINE_COLOR 255,0,0
#define BORDER_COLOR 255,0,0
#define CURSOR_COLOR 0,255,0


const char usage[] = 
"warp [-l] [-h] [-a] [-r <number of rows>] [-c <number of columns>] [-k <grid element 1 key>[,<grid element 2 key>...]]\n\n"
"See the manpage for more details and usage examples.\n";

char lock_file[PATH_MAX];
char *activation_key = "M-k";
char *close_key = "Return";

char *up_key = "w";
char *left_key = "a";
char *down_key = "s";
char *right_key = "d";

int cursor_move_inc = 20;

int lx, ly, ux, uy, cx, cy;

//Number of columns and rows in the grid.
int nc = 2, nr = 2;

int opt_always_active = 0;
int opt_daemonize = 0;

struct {
	const char *key;
	int c;
	int r;
	int btn;
} bindings[MAX_BINDINGS] = {
	{"u", 0, 0, 0},
	{"i", 1, 0, 0},
	{"j", 0, 1, 0},
	{"k", 1, 1, 0},
	{"m", 0, 0, 1},
};

int bindings_sz = 5;

Display *dpy;

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

static void draw(int hide) 
{
	int i,j;
	int rowh, colw;
	static Window bw1 = 0, bw2, bw3, bw4, mw;
	static Window *gridwins;

	if(!bw1) {
		bw1 = create_win(BORDER_COLOR);
		bw2 = create_win(BORDER_COLOR);
		bw3 = create_win(BORDER_COLOR);
		bw4 = create_win(BORDER_COLOR);
		mw = create_win(CURSOR_COLOR);

		gridwins = malloc((nr + nc - 2) * sizeof(Window));
		for(i = 0; i < (nr + nc - 2); i++)
			gridwins[i] = create_win(GRID_LINE_COLOR);
	}

	XUnmapWindow(dpy, bw1);
	XUnmapWindow(dpy, bw2);
	XUnmapWindow(dpy, bw3);
	XUnmapWindow(dpy, bw4);
	XUnmapWindow(dpy, mw);
	for(i = 0; i < (nr + nc - 2); i++)
		XUnmapWindow(dpy, gridwins[i]);

	XFlush(dpy);

	if(hide) {
		set_cursor_visibility(1);
		return;
	}

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

static void rel_warp(int x, int y) {
	lx += x;
	ly += y;
	cx += x;
	cy += y;
	ux += x;
	uy += y;

	draw(0);
	XFlush(dpy);
}

static void click(int btn) 
{
	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
	XFlush(dpy);
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

static void parse_key(const char* key, int *code, int *mods) 
{
	KeySym sym;
	if(!key || key[0] == 0) return;

	*mods = 0;
	while(key[1] == '-') {
		switch(key[0]) {
		case 'A':
			*mods |= Mod1Mask;
			break;
		case 'M':
			*mods |= Mod4Mask;
			break;
		case 'S':
			*mods |= ShiftMask;
			break;
		case 'C':
			*mods |= ControlMask;
			break;
		default:
			fprintf(stderr, "%s is not a valid key\n", key);
			exit(1);
		}

		key += 2;
	}

	sym = XStringToKeysym(key);

	if(sym == NoSymbol) {
		fprintf(stderr, "Could not find keysym for %s\n", key);
		exit(1);
	}

	if(key[1] == '\0' && isupper(key[0]))
		*mods |= ShiftMask;

	*code = XKeysymToKeycode(dpy, sym);
}

static void daemonize() 
{
	if(!fork())
		setsid();
	else
		exit(0);

	printf("Daemon started.\n");
}

static void proc_args(char **argv, int argc) 
{
	int r, c;
	int opt;

	while((opt = getopt(argc, argv, "hdar:c:k:lm:i:")) != -1) {
		switch(opt) {
			size_t i;
		case 'l':

			for(i = 0; i < sizeof(keynames)/sizeof(keynames[0]); i++)
				printf("%s\n", keynames[i]);
			exit(0);
			break;
		case 'd':
			opt_daemonize++;
			break;
		case 'h':
			fprintf(stderr, usage);
			exit(1);
			break;
		case 'a':
			opt_always_active++;
			activation_key = NULL;
			break;
		case 'r':
			nr = atoi(optarg);
			if(nr <= 0) {
				fprintf(stderr, "Number of rows must be greater than 0\n");
				exit(1);
			}
			break;
		case 'c':
			nc = atoi(optarg);
			if(nc <= 0) {
				fprintf(stderr, "Number of columns must be greater than 0\n");
				exit(1);
			}
			break;
		case 'i':
			cursor_move_inc = atoi(optarg);
			break;
		case 'm':
			up_key = strtok(optarg, ",");
			left_key = strtok(NULL, ",");
			down_key = strtok(NULL, ",");
			right_key = strtok(NULL, ",");

			if(!up_key || !left_key || !down_key || !right_key) {
				fprintf(stderr, "ERROR: Bad movement key spec, should be: <up key>,<left key>,<down key>,<right key>");
				exit(1);
			}

			break;
		case 'k':
			{
				int c = 0;
				int r = 0;
				int btn = 0;
				bindings_sz = 0;

				if(!opt_always_active) {
					activation_key = strtok(optarg, ",");
					optarg += strlen(optarg) + 1;
				} 

				for(const char *key = strtok(optarg, ",");key;key = strtok(NULL, ",")) {
					int code, mods;
					parse_key(key, &code, &mods);

					if(btn == 4) {
						close_key = strdup(key);
						continue;
					}

					bindings[bindings_sz].key = key;
					bindings[bindings_sz].btn = btn;
					bindings[bindings_sz].c = c;
					bindings[bindings_sz++].r = r;

					c = (c+1) % nc;
					if(c == 0) r++;
					if(r >= nr) btn++;
				}
				break;
			}
		default:
			exit(1);
		}
	}
}

static void reset() 
{
	XWindowAttributes attr;

	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &attr);

	XGrabKeyboard(dpy, DefaultRootWindow(dpy), 1, GrabModeAsync, GrabModeAsync, CurrentTime);

	lx = 0;
	ly = 0;
	uy = attr.height;
	ux = attr.width;
	cx = ux/2;
	cy = uy/2;

	focus_sector(-1, -1);
}


static int key_eq(const char *key, XKeyEvent *ev) 
{
	if(!key) return 0;

	int code, mods;
	parse_key(key, &code, &mods);
	return ev->state == mods && ev->keycode == code;
}

static void grab_key(const char *key) 
{
	if(!key) return;

	int code, mods;

	parse_key(key, &code, &mods);

	XGrabKey(dpy,
		 code,
		 mods,
		 DefaultRootWindow(dpy),
		 False,
		 GrabModeAsync,
		 GrabModeAsync);
}

static void check_lock_file() 
{
	int fd;

	const char *rundir = getenv("XDG_RUNTIME_DIR");
	if(!rundir) {
		fprintf(stderr, "Could not find XDG_RUNTIME_DIR, make sure X is running.");
		exit(1);
	}

	sprintf(lock_file, "%s/warp.lock.%s", rundir, getenv("DISPLAY"));

	if((fd = open(lock_file, O_CREAT | O_TRUNC, 0600)) < 0) {
		perror("open");
		exit(1);
	}

	if(flock(fd, LOCK_EX | LOCK_NB)) {
		fprintf(stderr, "ERROR: warp already appears to be running.\n");
		exit(1);
	}
}

int main(int argc, char **argv) 
{
	int i;

	int is_active = 0;
	dpy = XOpenDisplay(NULL);

	proc_args(argv, argc);

	check_lock_file();
	if(opt_daemonize) daemonize();

	grab_key(activation_key);

	if(opt_always_active) {
		for(i = 0; i < bindings_sz; i++)
			grab_key(bindings[i].key);
	}

	while(1) {
		XEvent ev;
		XNextEvent(dpy, &ev);
		if(ev.type == KeyPress) {
			KeySym sym = XKeycodeToKeysym(dpy, ev.xkey.keycode, 0);

			if(key_eq(up_key, &ev.xkey))
				rel_warp(0, -cursor_move_inc);
			if(key_eq(down_key, &ev.xkey))
				rel_warp(0, cursor_move_inc);
			if(key_eq(left_key, &ev.xkey))
				rel_warp(-cursor_move_inc, 0);
			if(key_eq(right_key, &ev.xkey))
				rel_warp(cursor_move_inc, 0);

			if(key_eq(activation_key, &ev.xkey)) {
				is_active = 1;
				reset();
				draw(0);
				continue;
			}


			for (int i = 0; i < bindings_sz; i++) {
				if(key_eq(bindings[i].key, &ev.xkey)) {
					if(bindings[i].btn) {
						draw(1);
						click(bindings[i].btn);
						//FIXME: Ugly kludge to fix the race condition between XTestFakeButtonEvent and XMapRaised
						//TODO: Investigate the root cause (possibly related to WM window mapping and pointer grabs).
						usleep(10000);
						draw(0);
					} else {
						if(!is_active) {
							is_active = 1;
							reset();
						}

						focus_sector(bindings[i].c, bindings[i].r);
						draw(0);
					}
				}
			}

			if(key_eq(close_key, &ev.xkey)) {
				XUngrabKeyboard(dpy, CurrentTime);
				set_cursor_visibility(1);
				draw(1);
				is_active = 0;
				continue;
			}
		}
	}
}
