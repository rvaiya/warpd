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
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "discrete.h"
#include "input.h"

static Display *dpy;
static Window indicator = None;
static struct discrete_keys *keys;
static int increment;
static int indicator_sz;

static void hide()
{
	XUnmapWindow(dpy, indicator);
	XFlush(dpy);
}

static void draw()
{
	Window chld, root;
	int cx, cy, _;
	unsigned int _u;

	XQueryPointer(dpy,
		DefaultRootWindow(dpy),
		&root,
		&chld,
		&cx, &cy,
		&_, &_, &_u);

	XMoveWindow(dpy, indicator, cx - indicator_sz/2, cy - indicator_sz/2);
	XMapRaised(dpy, indicator);
	XFlush(dpy);
}


static void rel_warp(int x, int y) 
{
	XWarpPointer(dpy, 0, None, 0, 0, 0, 0, x, y);
	XFlush(dpy);
}

static void abs_warp(int x, int y) 
{
	XWindowAttributes info;
	Window chld, root;
	int cx, cy, _;
	unsigned int _u;

	/* Obtain absolute pointer coordinates */
	XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &chld, &cx, &cy, &_, &_, &_u);
	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);

	x = x == -1 ? cx : (x * info.width) / 100;
	y = y == -1 ? cy : (y * info.height) / 100;

	XWarpPointer(dpy, 0, DefaultRootWindow(dpy), 0, 0, 0, 0, x, y);
	XFlush(dpy);
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

static int tonum(uint16_t keyseq)
{
	size_t i;
	static uint16_t nums[10] = {0};

	if(!nums[0]) {
		nums[0] = input_parse_keyseq("0");
		nums[1] = input_parse_keyseq("1");
		nums[2] = input_parse_keyseq("2");
		nums[3] = input_parse_keyseq("3");
		nums[4] = input_parse_keyseq("4");
		nums[5] = input_parse_keyseq("5");
		nums[6] = input_parse_keyseq("6");
		nums[7] = input_parse_keyseq("7");
		nums[8] = input_parse_keyseq("8");
		nums[9] = input_parse_keyseq("9");
	}

	for (i = 0; i < 10; i++) {
		if(keyseq == nums[i]) return i;
	}

	return -1;
}

uint16_t discrete_warp()
{
	draw();
	int opnum = 0;
	while(1) {
		uint16_t keyseq;
		int num;

		keyseq = input_next_key(0);

		if(keyseq == keys->home)
			abs_warp(-1, 0);
		else if(keyseq == keys->last)
			abs_warp(-1, 100);
		else if(keyseq == keys->middle)
			abs_warp(-1, 50);
		else if(keyseq == keys->beginning)
			abs_warp(0, -1);
		else if(keyseq == keys->end)
			abs_warp(100, -1);
		else if(keyseq == keys->up) {
			rel_warp(0, -increment*(opnum ? opnum : 1));
			opnum = 0;
		} else if(keyseq == keys->down) {
			rel_warp(0, increment*(opnum ? opnum : 1));
			opnum = 0;
		} else if(keyseq == keys->left) {
			rel_warp(-increment*(opnum ? opnum : 1), 0);
			opnum = 0;
		} else if(keyseq == keys->right) {
			rel_warp(increment*(opnum ? opnum : 1), 0);
			opnum = 0;
		} else if ((num=tonum(keyseq)) != -1) {
			if(num == 0 && opnum == 0)
				rel_warp(-10000, 0);
			else
				opnum = opnum*10 + num;
		} else if(keyseq == keys->scroll_up || keyseq == keys->scroll_down) {
			const int v0 = 10; //scroll events per second
			const int a = 30;

			int t = 0; //in ms
			int v = v0;
			int last_click = 0;
			const int btn = (keyseq == keys->scroll_up) ? 4 : 5;

			//Hack to ensure scroll events are sent to the correct window without having to unmap
			//the indicator.
			rel_warp(0, -indicator_sz);

			XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
			XTestFakeButtonEvent(dpy, btn, False, CurrentTime);

			while(1) {
				if(input_next_keyup(1) != TIMEOUT_KEYSEQ)
					break;

				t += 1;
				if((t - last_click)*v > 1000) {
					XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
					XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
					last_click = t;
				}
				v = (a*t)/1000 + v0;
			}

			rel_warp(0, indicator_sz);
		} else {
			size_t i;
			for (i = 0; i < sizeof keys->exit / sizeof keys->exit[0]; i++) {
				if(keys->exit[i] == keyseq) {
					hide();
					return keyseq;
				}
			}
		}

		draw();
	}
}

void init_discrete(Display *_dpy,
		  const int _increment,
		  struct discrete_keys *_keys,
		  const char *indicator_color,
		  size_t _indicator_sz)
{
	keys = _keys;
	dpy = _dpy;
	increment = _increment;
	indicator_sz = _indicator_sz;

	XWindowAttributes info;
	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);


	if(indicator == None)
		indicator = create_win(indicator_color,
				       info.width - indicator_sz - 20, 20,
				       indicator_sz, indicator_sz);
}
