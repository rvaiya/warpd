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
#include <X11/extensions/shape.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "hint_drw.h"

static Display *dpy;

static struct hint *hints;
static int nhints;

static Pixmap backing_pixmap;
static Window win;
static int winh, winw;
static GC gc = None;

static void set_opacity(Display *dpy, Window w, int opacity) 
{
	Atom OPACITY_ATOM = XInternAtom (dpy, "_NET_WM_WINDOW_OPACITY", False);

	opacity = (unsigned int)(((double)opacity / 100) * (double)0xffffffff);
	XChangeProperty(dpy, w, OPACITY_ATOM,
			XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *) &opacity, 1L);
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

static Window create_win(uint8_t r, uint8_t g, uint8_t b,
			 int x,
			 int y,
			 int w,
			 int h)
{
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

static XftFont *get_font(const char *_name, int h)
{
	XftPattern *pat;
	XftResult res;
	XftFont *font;
	char name[256];
	int realh;

	snprintf(name, sizeof name, "%s:pixelsize=%d", _name, h);

	pat = XftFontMatch(dpy,
			   DefaultScreen(dpy),
			   XftNameParse(name),
			   &res);

	if(res != XftResultMatch) {
		fprintf(stderr, "Failed to open font %s\n", _name);
		exit(-1);
	}

	font = XftFontOpenPattern(dpy, pat);
	realh = font->height;

	snprintf(name, sizeof name, "%s:pixelsize=%d", _name, (h*h)/realh);

	pat = XftFontMatch(dpy,
			   DefaultScreen(dpy),
			   XftNameParse(name),
			   &res);

	if(res != XftResultMatch) {
		fprintf(stderr, "Failed to open font %s\n", _name);
		exit(-1);
	}

	font = XftFontOpenPattern(dpy, pat);
	return font;
}

static int draw_text(Drawable win,
		     XftFont *font,
		     int x,
		     int y,
		     uint8_t r,
		     uint8_t g,
		     uint8_t b,
		     const char *s) 
{
	uint32_t scr = DefaultScreen(dpy);
	static XftDraw *drw = NULL;
	static Window _win = 0;

	XGlyphInfo e;


	if(win != _win) {
		free(drw);
		drw = XftDrawCreate(dpy, win,
				    DefaultVisual(dpy, scr),
				    DefaultColormap(dpy, scr));

		_win = win;
	}

	XftTextExtentsUtf8(dpy, font, (FcChar8 *)s, strlen(s), &e);
	XftDrawStringUtf8(drw,
			  xft_color(r,g,b),
			  font,
			  x, y + font->ascent,
			  (FcChar8 *)s,
			  strlen(s));

	return 0;
}

static int textwidth(XftFont *font, const char *s)
{
	XGlyphInfo e;
	XftTextExtentsUtf8(dpy, font, (FcChar8 *)s, strlen(s), &e);
	return e.width;
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

static Pixmap create_backing_store(Display *dpy,
				   struct hint *hints,
				   size_t n,
				   const char *bgcol,
				   const char *fgcol)
{
	size_t i;
	uint8_t r, g, b;
	XftFont *font = get_font("Monospace", hints[0].h);

	XWindowAttributes info;
	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);

	Pixmap pm = XCreatePixmap(dpy,
				  DefaultRootWindow(dpy),
				  info.width,
				  info.height,
				  DefaultDepth(dpy, DefaultScreen(dpy)));


	hex_to_rgb(bgcol, &r, &g, &b);
	GC gc = XCreateGC(dpy, DefaultRootWindow(dpy),
			  GCForeground | GCFillStyle,
			  &(XGCValues){ 
			  .foreground = color(r, g, b), 
			  .fill_style = FillSolid,
			  });

	XFillRectangle(dpy, pm, gc, 0, 0, info.width, info.height);

	hex_to_rgb(fgcol, &r, &g, &b);
	for (i = 0; i < n; i++) {
		const int xoff = (hints[i].w - textwidth(font, hints[i].label))/2;
		draw_text(pm,
			  font,
			  hints[i].x + xoff,
			  hints[i].y,
			  r, g, b,
			  hints[i].label);
	}

	XFreeGC(dpy, gc);
	return pm;
}

void init_hint_drw(Display *_dpy,
		   struct hint *_hints,
		   size_t n,
		   int opacity,
		   const char *bgcol,
		   const char *fgcol)
{
	uint8_t r,g,b;
	dpy = _dpy;

	XWindowAttributes info;
	XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);

	hex_to_rgb(bgcol, &r, &g, &b);

	hints = _hints;
	nhints = n;

	backing_pixmap = create_backing_store(dpy, hints, nhints, bgcol, fgcol);


	win = create_win(r, g, b, 0, 0, info.width, info.height);
	winw = info.width;
	winh = info.height;
	gc = XCreateGC(dpy,
		       DefaultRootWindow(dpy),
		       GCFillStyle,
		       &(XGCValues){ .fill_style = FillSolid });

	set_opacity(dpy, win, opacity);
}

void hint_drw_filter(size_t *indices, size_t nindices)
{
	size_t i, n = 0;
	static XRectangle recs[MAX_HINTS];

	if(!nindices) {
		XUnmapWindow(dpy, win);
		return;
	}

	for (i = 0; i < nindices; i++) {
		struct hint *h = &hints[indices[i]];

		recs[n].x = h->x;
		recs[n].y = h->y;
		recs[n].width = h->w;
		recs[n++].height = h->h;
	}

	XShapeCombineRectangles(dpy, win,
				ShapeBounding,
				0,0,
				recs, n,
				ShapeSet,
				Unsorted);


	XMapRaised(dpy, win);
	XCopyArea(dpy, backing_pixmap, win, gc, 0, 0, winw, winh, 0, 0);
	XFlush(dpy);
}
