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

#include <X11/extensions/Xinerama.h>
#include <X11/extensions/XTest.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "hints.h"

Display *dpy;

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

struct target targets[1024] = {0};
int nc, nr;

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

static void hide_target(struct target *target)
{
	XUnmapWindow(dpy, target->win);
	XFlush(dpy);
}

static Window show_target(struct target *target)
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
	Window win;
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
	int i, n = 0;
	struct target *last;

	if(selected)
		*selected = NULL;

	for (i = 0; i < nr*nc; i++)
		if(strstr(targets[i].s, s)) {
			n++;
			last = &targets[i];
			show_target(targets + i);
		} else
			hide_target(targets + i);

	XFlush(dpy);

	if(n == 1) {
		*selected = last;
		hide_target(last);
		return 1;
	} else if(n == 0) {
		*selected = NULL;
		return 1;
	}

	return 0;
}

static int generate_targets(int target_width, int target_height)
{
	int i, j;
	XWindowAttributes info;
	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);

	const char hint_set1[] = "asdfghjkl;zxcvbn,./qwertyuiop[]\\";
	const char hint_set2[] = "asdfghjkl;zxcvbn,./qwertyuiop[]\\";

	printf("%d %d\n", info.width, info.height);

	nr = info.height / target_height;
	nc = info.width / target_width;

	if(nr > (sizeof(hint_set1)-1)) {
		fprintf(stderr, "FATAL ERROR: Number of columns(%d) exceeds hint size (%zd), try reducing target size\n", nr, sizeof hint_set1);
		exit(-1);
	}

	if(nc > (sizeof(hint_set2)-1)) {
		fprintf(stderr, "FATAL ERROR: Number of columns(%d) exceeds hint size (%zd), try reducing target size\n", nc, sizeof hint_set2);
		exit(-1);
	}

	for (i = 0; i < nr; i++) {
		for (j = 0; j < nc; j++) {
			char hint[4];
			hint[0] = hint_set1[i];
			hint[1] = hint_set2[j];
			hint[2] = '\0';
			create_target(target_width*j, target_height*i, target_width, target_height,
				      20, "#00ff00", "#000000", strdup(hint),
				      &targets[nc*i+j]);
		}
	}


	XFlush(dpy);
}

static void local_control(int inc, struct hint_keys *keys) {
	const int double_click_timeout = 200;
	int post_click = 0;
	int xfd = XConnectionNumber(dpy);
	if(!keys) {
		keys = &(struct hint_keys){
			.up = XKeysymToKeycode(dpy, XK_k),
			.down = XKeysymToKeycode(dpy, XK_j),
			.right = XKeysymToKeycode(dpy, XK_l),
			.left = XKeysymToKeycode(dpy, XK_h),
		};
	}

	while(1) {
		XEvent ev;
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(xfd, &fds);

		select(xfd+1,
		       &fds,
		       NULL,
		       NULL,
		       post_click ? &(struct timeval){0, double_click_timeout*1000} : NULL);

		if(!XPending(dpy)) {
			//printf("Click timeout, closing movement\n");
			XUngrabKeyboard(dpy, CurrentTime);
			XFlush(dpy);

			return;
		}

		while(XPending(dpy)) {
			XNextEvent(dpy, &ev);

			if(ev.type == KeyPress) {
				if(ev.xkey.keycode == keys->button1 ||
				   ev.xkey.keycode == keys->button2 ||
				   ev.xkey.keycode == keys->button3) {

					XUngrabKeyboard(dpy, CurrentTime);
					XFlush(dpy);

					const int btn = 
						ev.xkey.keycode == keys->button1 ? 1 :
						ev.xkey.keycode == keys->button2 ? 2 : 3;
					XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
					XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
					XSync(dpy, False);

					usleep(100000); //Give target an opportunity to grab the keyboard before we do.
					if(XGrabKeyboard(dpy, DefaultRootWindow(dpy), False, GrabModeAsync, GrabModeAsync, CurrentTime)) {
						fprintf(stderr, "Failed to grab keyboard (already grabbed?) try closing any active popups.\n");
						return;
					}

					post_click++;
				}

				if(ev.xkey.keycode == keys->up)
					XWarpPointer(dpy, 0, None, 0, 0, 0, 0, 0, -inc);
				else if(ev.xkey.keycode == keys->down)
					XWarpPointer(dpy, 0, None, 0, 0, 0, 0, 0, inc);
				else if(ev.xkey.keycode == keys->right)
					XWarpPointer(dpy, 0, None, 0, 0, 0, 0, inc, 0);
				else if(ev.xkey.keycode == keys->left)
					XWarpPointer(dpy, 0, None, 0, 0, 0, 0, -inc, 0);
				else if(ev.xkey.keycode == keys->quit) {
					XUngrabKeyboard(dpy, CurrentTime);
					XFlush(dpy);
					return;
				}
			}


			XSync(dpy, False);
		}
	}
}

void hints(Display *_dpy, int _nc, int _nr, int inc, struct hint_keys *keys) {
	dpy = _dpy;
	XWindowAttributes info;

	if(XGrabKeyboard(dpy, DefaultRootWindow(dpy), False, GrabModeAsync, GrabModeAsync, CurrentTime)) {
		fprintf(stderr, "Failed to grab keyboard (already grabbed?) try closing any active popups.\n");
		return;
	}

	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);
	if(nr != _nr || nc != _nc) {
		printf("Generating targets\n");
		generate_targets(info.width / _nc, info.height / _nr);
	}
	filter_targets("", NULL);

	char buf[256] = {0};
	buf[0] = '\0';

	while(1) {
		XEvent ev;
		XNextEvent(dpy, &ev);
		if(ev.type == KeyPress) {
			struct target *target;
			KeySym key = XKeycodeToKeysym(dpy, ev.xkey.keycode, 0);

			if(ev.xkey.keycode == keys->quit) {
				int i;

				for(i = 0;i<nc*nr;i++)
					hide_target(&targets[i]);

				XUngrabKeyboard(dpy, CurrentTime);
				XFlush(dpy);
				return;
			}

			if(key == XK_BackSpace) {
				if(buf[0] != '\0')
					buf[strlen(buf)-1] = '\0';
			} else {
				char s[20];
				XLookupString(&ev.xkey, s, sizeof s, NULL, NULL);

				strcpy(buf + strlen(buf), s);
			}

			if(filter_targets(buf, &target)) {
				if(target) {
					XWarpPointer(dpy, 0, DefaultRootWindow(dpy), 0, 0, 0, 0, target->x+target->w/2, target->y+target->h/2);
					XSync(dpy, False);
					local_control(inc, keys);
				}

				return;
			}
		}
	}
}
