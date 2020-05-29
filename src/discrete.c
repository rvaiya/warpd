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
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "discrete.h"
#include "input.h"

static Display *dpy;
static Window indicator = None;

static void hide()
{
	XUnmapWindow(dpy, indicator);
	XFlush(dpy);
}

static void draw()
{
	XMapRaised(dpy, indicator);
	XFlush(dpy);
}


static void rel_warp(int x, int y) 
{
	XWarpPointer(dpy, 0, None, 0, 0, 0, 0, x, y);
	XFlush(dpy);
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


	return win;
}

int discrete(Display *_dpy,
	     const int increment,
	     const int double_click_timeout,
	     int start_button,
	     struct discrete_keys *keys,
	     const char *indicator_color,
	     size_t indicator_sz)
{
	dpy = _dpy;
	int clicked = 0;

	XWindowAttributes info;
	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);


	if(indicator == None)
		indicator = create_win(indicator_color,
				       info.width - indicator_sz - 20, 20,
				       indicator_sz, indicator_sz);

	draw();

	if(start_button) {
		clicked = 1;
		click(start_button);
	}

	while(1) {
		size_t i;
		uint16_t keyseq;

		keyseq = input_next_key(clicked ? double_click_timeout : 0);
		if(!keyseq)
			break;

		if(keyseq == keys->up)
			rel_warp(0, -increment);
		if(keyseq == keys->down)
			rel_warp(0, increment);
		if(keyseq == keys->left)
			rel_warp(-increment, 0);
		if(keyseq == keys->right)
			rel_warp(increment, 0);
		if(keyseq == keys->quit)
			break;

		for (i = 0; i < sizeof keys->buttons/sizeof keys->buttons[0]; i++)
			if(keys->buttons[i] == keyseq) {
				if(i < 3) clicked++; //Don't timeout on scroll
				click(i+1);
				break;
			}
	}

	hide();
	return 0;
}
