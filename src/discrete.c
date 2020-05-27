#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "discrete.h"

static Display *dpy;
static Window indicator = None;

static void hide()
{
	XUnmapWindow(dpy, indicator);
}

static void draw()
{
	XMapRaised(dpy, indicator);
}


static void rel_warp(int x, int y) 
{
	XWarpPointer(dpy, 0, None, 0, 0, 0, 0, x, y);
}

static void click(int btn) 
{
	hide();
	XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
	XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
	XSync(dpy, False);
	draw();
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

static Window create_win(const char *col, int x, int y, int w, int h)
{
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	hex_to_rgb(col, &r, &g, &b);

	Window win = XCreateWindow(dpy,
					  DefaultRootWindow(dpy),
					  x, y, w, h,
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


	XMapWindow(dpy, win);
	return win;
}

void discrete(Display *_dpy, const int inc, const int double_click_timeout, struct discrete_keys *keys, const char *indicator_color, size_t indicator_sz)
{
	dpy = _dpy;
	int clicked = 0;
	int xfd = XConnectionNumber(dpy);
	XWindowAttributes info;

	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);

	if(XGrabKeyboard(dpy, DefaultRootWindow(dpy), False, GrabModeAsync, GrabModeAsync, CurrentTime)) {
		fprintf(stderr, "Failed to grab the keyboard\n");
		return;
	}

	if(indicator == None)
		indicator = create_win(indicator_color,
			info.width - indicator_sz - 20, 20,
			indicator_sz, indicator_sz);

	draw();
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
	hide();
	return;
}
