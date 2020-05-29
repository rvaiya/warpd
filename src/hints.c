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

#include <X11/extensions/Xinerama.h>
#include <X11/extensions/XTest.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "hints.h"
#include "input.h"
#include "dbg.h"

static Display *dpy;

#define FONT_NAME "*"

struct target {
	Window win;
	const char *fgcol;
	int font_height;
	const char *s;
	int text_xoff;
	int text_yoff;
	int x;
	int y;
	int w;
	int h;
};

static struct target targets[1024] = {0};
static size_t nc, nr;
static char *hint_characters = "";

static uint32_t x11_color(uint8_t red, uint8_t green, uint8_t blue) 
{
	XColor col;
	col.red = (int)red << 8;
	col.green = (int)green << 8;
	col.blue = (int)blue << 8;
	col.flags = DoRed | DoGreen | DoBlue;

	assert(XAllocColor(dpy, XDefaultColormap(dpy, DefaultScreen(dpy)), &col));
	return col.pixel;
}

static Window create_win(uint8_t r, uint8_t g, uint8_t b, int x, int y, int w, int h) 
{
	long t = 1;

	Window win = XCreateWindow(dpy,
				   DefaultRootWindow(dpy),
				   x, y, w, h,
				   0,
				   DefaultDepth(dpy, DefaultScreen(dpy)),
				   InputOutput,
				   DefaultVisual(dpy, DefaultScreen(dpy)),
				   CWOverrideRedirect | CWBackPixel | CWBackingPixel,
				   &(XSetWindowAttributes){
				   .backing_pixel = x11_color(r,g,b),
				   .background_pixel = x11_color(r,g,b),
				   .override_redirect = 1
				   });


	/* Try and bypass compositing, doesn't seem to work on XFWM4 */
	XChangeProperty(dpy,
			win,
			XInternAtom(dpy, "_NET_WM_BYPASS_COMPOSITOR", False),
			XA_CARDINAL,
			32,
			PropModeReplace,
			(unsigned char *)&t,
			1);

	XFlush(dpy);
	return win;
}

static XftColor *xft_color(uint8_t red, 
			   uint8_t green, 
			   uint8_t blue) 
{
	XRenderColor col;
	int scr = DefaultScreen(dpy);

	col.red = red * 257;
	col.green = green * 257;
	col.blue = blue * 257;
	col.alpha = ~0;

	XftColor *r = malloc(sizeof(XftColor));
	XftColorAllocValue(dpy, DefaultVisual(dpy, scr), DefaultColormap(dpy, scr), &col, r);
	return r;
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

static int draw_text(Window win, int x, int y, int h, uint8_t r, uint8_t g, uint8_t b, const char *s) 
{
	XftDraw *drw;
	uint32_t scr = DefaultScreen(dpy);

	static int _h = 0;
	static XftFont *font = NULL;

	if(_h != h) {
		XftPattern *pat;
		XftResult res;
		char name[256];

		snprintf(name, sizeof name, "%s:pixelsize=%d", FONT_NAME, h);

		pat = XftFontMatch(dpy, DefaultScreen(dpy), XftNameParse(name), &res);
		if(res != XftResultMatch)
			return -1;
		font = XftFontOpenPattern(dpy, pat);

		_h = h;
	}

	drw = XftDrawCreate(dpy, win,
			    DefaultVisual(dpy, scr),
			    DefaultColormap(dpy, scr));

	XftDrawStringUtf8(drw,
			  xft_color(r,g,b),
			  font,
			  x, y + font->ascent,
			  (FcChar8 *)s,
			  strlen(s));

	free(drw);
	return 0;
}

static int calc_textbox_width(int height, const char *s)
{
	XGlyphInfo e;
	static int _height = 0;
	static XftFont *font = NULL;

	if (_height != height) {
		XftResult r;
		XftPattern *pat;
		char name[256];

		snprintf(name, sizeof name, "%s:pixelsize=%d", FONT_NAME, height);

		pat = XftFontMatch(dpy, DefaultScreen(dpy), XftNameParse(name), &r);
		if(r != XftResultMatch)
			return -1;
		font = XftFontOpenPattern(dpy, pat);

		_height = height;
	}


	XftTextExtentsUtf8(dpy, font, (FcChar8 *)s, strlen(s), &e);
	return e.width;
}

static void show_target(struct target *target)
{
	uint8_t r, g, b;

	hex_to_rgb(target->fgcol, &r, &g, &b);
	XMapRaised(dpy, target->win);

	draw_text(target->win,
		  target->text_xoff,
		  target->text_yoff,
		  target->font_height,
		  r, g, b,
		  target->s);
}

static void create_target(int x, int y, int w, int h, int font_height, const char *bgcol, const char *fgcol, const char *s, struct target *target)
{
	uint8_t r, g, b;
	int tw;
	int winh, winw;

	tw = calc_textbox_width(font_height, s);
	hex_to_rgb(bgcol, &r, &g, &b);

	winw = font_height*2;
	winh = font_height+10;

	//target->w = create_win(r, g, b, x, y, w, h);

	target->font_height = font_height;
	target->fgcol = fgcol;
	target->text_xoff = (winw - tw) / 2;
	target->text_yoff = (winh - font_height) / 2;
	target->x = x;
	target->y = y;
	target->w = w;
	target->h = h;
	target->s = s;
	

	target->win = create_win(r, g, b, x+(w - winw) / 2, y+(h - winh) / 2, winw, winh);
}

static int filter_targets(const char *s, struct target **selected)
{
	size_t i, n = 0;
	struct target *last;

	if(selected)
		*selected = NULL;

	for (i = 0; i < nr*nc; i++)
		if(strstr(targets[i].s, s) == targets[i].s) {
			n++;
			last = &targets[i];
			show_target(targets + i);
		} else
			XUnmapWindow(dpy, targets[i].win);

	if(n == 1) {
		*selected = last;
		XUnmapWindow(dpy, (*selected)->win);
		return 1;
	} else if(n == 0) {
		*selected = NULL;
		return 1;
	}

	return 0;
}

static void generate_targets(int target_width, int target_height, const char *bgcol, const char *fgcol)
{
	size_t i, j;
	XWindowAttributes info;
	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);

	nr = info.height / target_height;
	nc = info.width / target_width;

	if(nr > strlen(hint_characters)) {
		fprintf(stderr,
			"FATAL ERROR: Number of rows (%zd) exceeds hint size (%zd), try reducing the number of rows or increasing the size of the hint set.\n",
			nr, strlen(hint_characters));

		exit(-1);
	}

	if(nc > strlen(hint_characters)) {
		fprintf(stderr,
			"FATAL ERROR: Number of columns (%zd) exceeds hint size (%zd), try reducing the number of columns or increasing the size of the hint set.\n",
			nc, strlen(hint_characters));

		exit(-1);
	}

	for (i = 0; i < nr; i++) {
		for (j = 0; j < nc; j++) {
			char hint[4];
			hint[0] = hint_characters[i];
			hint[1] = hint_characters[j];
			hint[2] = '\0';

			create_target(target_width*j, target_height*i, target_width, target_height,
				      20, bgcol, fgcol, strdup(hint),
				      &targets[nc*i+j]);
		}
	}


	XFlush(dpy);
}

static const char *normalize(const char *s)
{
	if(!strcmp(s, "apostrophe"))
		return "'";
	else if(!strcmp(s, "period"))
		return ".";
	else if(!strcmp(s, "slash"))
		return "/";
	else if(!strcmp(s, "comma"))
		return ",";
	else if(!strcmp(s, "semicolon"))
		return ";";
	else
		return s;
}

int hints(Display *_dpy,
	  size_t _nc,
	  size_t _nr,
	  char *_hint_characters,
	  const char *bgcol,
	  const char *fgcol,
	  struct hint_keys *keys) 
{
	char buf[256];
	dpy = _dpy;
	XWindowAttributes info;
	hint_characters = _hint_characters;

	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);
	if(nr != _nr || nc != _nc) {
		dbg("Generating targets\n");
		generate_targets(info.width / _nc, info.height / _nr, bgcol, fgcol);
	}
	filter_targets("", NULL);

	buf[0] = '\0';
	uint16_t backspace = input_parse_keyseq("BackSpace");

	while(1) {
		struct target *target;

		int keyseq = input_next_key(0);
		if(keyseq == keys->quit) {
			size_t i;

			for(i = 0;i<nc*nr;i++)
				XUnmapWindow(dpy, targets[i].win);

			XFlush(dpy);
			return -1;
		}

		if(keyseq == backspace) {
			if(buf[0] != '\0')
				buf[strlen(buf)-1] = '\0';
		} else {
			strcpy(buf + strlen(buf), normalize(input_keyseq_to_string(keyseq)));
		}

		if(filter_targets(buf, &target)) {
			if(target) {
				XWarpPointer(dpy, 0, DefaultRootWindow(dpy), 0, 0, 0, 0, target->x+target->w/2, target->y+target->h/2);
				XFlush(dpy);
				return 0;
			}

			return -1;
		}
	}
}
