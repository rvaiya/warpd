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
#include <X11/extensions/shape.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "normal.h"
#include "input.h"
#include "scroll.h"

static Display *dpy;
static Window indicator = None;
static struct normal_keys *keys;
static int increment;
static int indicator_sz;
static int word_increment;

static float scroll_fling_timeout;
static float scroll_acceleration;
static float scroll_velocity;
static float scroll_fling_velocity;
static float scroll_fling_acceleration;
static float scroll_fling_deceleration;

static void hide()
{
	XUnmapWindow(dpy, indicator);
	XFlush(dpy);
}

static void draw()
{
	int x, y;

	input_get_cursor_position(&x, &y);
	XMoveWindow(dpy, indicator, x+1, y+1);
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


uint16_t normal_mode(uint16_t start_key)
{
	int opnum = 0;

	draw();

	while(1) {
		uint16_t keyseq;
		int num;

		if(start_key) {
			keyseq = start_key;
			start_key = 0;
		} else
			keyseq = input_next_key(0, 1);

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
		} else if(keyseq == keys->left_word) {
			rel_warp(-word_increment*(opnum ? opnum : 1), 0);
			opnum = 0;
		} else if(keyseq == keys->right_word) {
			rel_warp(word_increment*(opnum ? opnum : 1), 0);
			opnum = 0;
		} else if(keyseq == keys->up_word) {
			rel_warp(0, -word_increment*(opnum ? opnum : 1));
			opnum = 0;
		} else if(keyseq == keys->down_word) {
			rel_warp(0, word_increment*(opnum ? opnum : 1));
			opnum = 0;
		} else if ((num=tonum(keyseq)) != -1) {
			if(num == 0 && opnum == 0)
				rel_warp(-10000, 0);
			else
				opnum = opnum*10 + num;
		} else if(keyseq == keys->scroll_left || keyseq == keys->scroll_right) {
			uint16_t key;
				
			key = scroll(dpy,
				     keyseq,
				     keyseq == keys->scroll_left ? 6 : 7,
				     scroll_velocity,
				     scroll_acceleration,
				     scroll_fling_velocity,
				     scroll_fling_acceleration,
				     scroll_fling_deceleration,
				     scroll_fling_timeout);

			if(key)
				return normal_mode(key);
		} else if(keyseq == keys->scroll_up || keyseq == keys->scroll_down) {
			uint16_t key;
				
			key = scroll(dpy,
				     keyseq,
				     keyseq == keys->scroll_up ? 5 : 4,
				     scroll_velocity,
				     scroll_acceleration,
				     scroll_fling_velocity,
				     scroll_fling_acceleration,
				     scroll_fling_deceleration,
				     scroll_fling_timeout);

			if(key)
				return normal_mode(key);
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

void init_normal(Display *_dpy,
		 const int _increment,
		 const int _word_increment,
		 struct normal_keys *_keys,
		 const char *indicator_color,
		 size_t _indicator_sz,
		 float _scroll_fling_timeout,
		 float _scroll_velocity,
		 float _scroll_acceleration,
		 float _scroll_fling_velocity,
		 float _scroll_fling_acceleration,
		 float _scroll_fling_deceleration)
{
	keys = _keys;
	dpy = _dpy;
	increment = _increment;
	word_increment = _word_increment;
	indicator_sz = _indicator_sz;

	indicator_sz = _indicator_sz;
	scroll_fling_timeout = _scroll_fling_timeout;
	scroll_acceleration = _scroll_acceleration;
	scroll_velocity = _scroll_velocity;
	scroll_fling_velocity = _scroll_fling_velocity;
	scroll_fling_acceleration = _scroll_fling_acceleration;
	scroll_fling_deceleration = _scroll_fling_deceleration;

	XWindowAttributes info;
	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);


	indicator = create_win(indicator_color,
			       info.width - indicator_sz - 20, 20,
			       indicator_sz, indicator_sz);
}
