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

#include <X11/extensions/shape.h>
#include <X11/Xft/Xft.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "impl.h"
#include "../../platform.h"

static struct hint	 hints[MAX_HINTS];
static size_t		 nhints;

static int		 border_radius;
static int		 boxw, boxh;
static uint8_t		 active_indices[MAX_HINTS];
static XftFont		*font;

/* TODO: Add multi screen support. Will probably require a cached fhwin for each geometry :/. */


/* 
 * Dedicated window for full hint display. We permanently keep this off screen
 * as an optimization to avoid the expensive map call on a shaped window with
 * all hints (particularly noticable when border_radius != 0).
 */

static Window		 fhwin;

static struct pixmap	*label_pixmap;

/* 
 * Window for all other hint subsets, usually small enough
 * to generate on the fly without performance penalties
 * (split thusly for optimization)
 */

static Window win; 

static XftColor parse_xft_color(const char *s) 
{
	uint8_t r, g, b, a;
	XftColor color;
	XRenderColor rc;

	int scr = DefaultScreen(dpy);

	hex_to_rgba(s, &r, &g, &b, &a);

	rc.red = r * 257;
	rc.green = g * 257;
	rc.blue = b * 257;
	rc.alpha = ~0;

	XftColorAllocValue(dpy, DefaultVisual(dpy, scr), DefaultColormap(dpy, scr), &rc, &color);
	return color;
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
		     int x,
		     int y,
		     XftColor *color,
		     const char *s) 
{
	uint32_t scr = DefaultScreen(dpy);
	static XftDraw *drw = NULL;
	static Window _win = 0;

	if(win != _win) {
		free(drw);
		drw = XftDrawCreate(dpy, win,
				    DefaultVisual(dpy, scr),
				    DefaultColormap(dpy, scr));

		_win = win;
	}

	XftDrawStringUtf8(drw,
			  color,
			  font,
			  x, y + font->ascent,
			  (FcChar8 *)s,
			  strlen(s));

	return 0;
}

void calculate_string_dimensions(const char *s, int *w, int *h)
{
	XGlyphInfo e;
	XftTextExtentsUtf8(dpy, font, (FcChar8 *)s, strlen(s), &e);

	if (w)
		*w = e.width;
	if (h)
		*h = e.height;
}

static struct pixmap *create_label_pixmap(struct hint *hints,
				   size_t n,
				   const char *bgcol,
				   const char *fgcol)
{
	size_t i;
	int scrw, scrh;

	screen_get_dimensions(&scrw, &scrh);

	struct pixmap *pm = create_pixmap(bgcol, scrw, scrh);

	/* Draw labels. */

	XftColor xftcolor = parse_xft_color(fgcol); 

	for (i = 0; i < n; i++) {
		int sw, sh;
		struct hint *hint = &hints[i];
		const int x = hint->x - boxw/2;
		const int y = hint->y - boxh/2;

		calculate_string_dimensions(hint->label, &sw, &sh);

		draw_text(pm->pixmap,
			  x + (boxw - sw)/2,
			  y,
			  &xftcolor,
			  hint->label);
	}

	return pm;
}

static void draw_rounded_rectangle(Drawable dst,
			    GC gc,
			    unsigned int x,
			    unsigned int y,
			    unsigned int w,
			    unsigned int h,
			    unsigned int r)
{

	XFillArc(dpy, dst, gc, x, y, 2*r, 2*r, 64*90*1, 64*90);
	XFillArc(dpy, dst, gc, x+w-2*r, y, 2*r, 2*r, 64*90*0, 64*90);
	XFillArc(dpy, dst, gc, x+w-2*r, y+h-2*r, 2*r, 2*r, 64*90*3, 64*90);
	XFillArc(dpy, dst, gc, x, y+h-2*r, 2*r, 2*r, 64*90*2, 64*90);

	XFillRectangle(dpy, dst, gc, x+r, y, w-2*r, h);
	XFillRectangle(dpy, dst, gc, x, y+r, w, h-2*r);
}

static void hidewin(Window win)
{
	int sw, sh;

	screen_get_dimensions(&sw, &sh);

	/* Avoid unmapping since map appears to be expensive for shaped windows. */
	window_move(win, -sw, -sh);
}

void hint_hide()
{
	hidewin(fhwin);
	hidewin(win);
}

static void showwin(Window win)
{
	window_move(win, 0, 0);
}

static void apply_mask(Window win, uint8_t *indices)
{
	size_t i;
	static Pixmap mask = 0;
	static GC gc = 0;
	int sw, sh;

	screen_get_dimensions(&sw, &sh);

	if(!mask) {
		mask = XCreatePixmap(dpy, win, sw, sh, 1);
		gc = XCreateGC(dpy, mask, 0, NULL);
	}

	XSetForeground(dpy, gc, 0);
	XFillRectangle(dpy, mask, gc, 0, 0, sw, sh);

	XSetForeground(dpy, gc, 1);

	for(i = 0; i < nhints; i++) {
		if (indices[i]) {
			struct hint *hint = &hints[i];

			draw_rounded_rectangle(mask, gc, hint->x-boxw/2, hint->y-boxh/2, boxw, boxh, border_radius);
		}
	}

	XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, mask, ShapeSet);
}


static void redraw()
{
	size_t nactive = 0;
	size_t i;
	int sw, sh;

	screen_get_dimensions(&sw, &sh);

	for (i = 0; i < nhints; i++)
		if (active_indices[i])
			nactive++;

	if (nactive == nhints) {
		hidewin(win);
		showwin(fhwin);
		pixmap_copy(label_pixmap, fhwin);

		window_commit();
		return;
	}

	hidewin(fhwin);
	showwin(win);

	apply_mask(win, active_indices);
	pixmap_copy(label_pixmap, win);
	window_commit();
}

/* indices must be the same size as the initialized hints */
void hint_show(uint8_t *indices)
{
	memcpy(active_indices, indices, sizeof(indices[0]) * nhints);

	redraw();
}

static Window create_fhwin(const char *color)
{
	int sw, sh;

	screen_get_dimensions(&sw, &sh);
	Window win = create_window(color, 0, 0, sw, sh);

	apply_mask(win, active_indices);

	return win;
}

void init_hint(struct hint *_hints,
	       size_t n,
	       int height,
	       int _border_radius,
	       const char *bgcol,
	       const char *fgcol,
	       const char *font_family)
{
	size_t i;
	int sw, sh;

	assert(n <= MAX_HINTS);

	screen_get_dimensions(&sw, &sh);

	memcpy(hints, _hints, sizeof(hints[0])*n);
	nhints = n;
	border_radius = _border_radius;
	boxh = height;

	font = get_font(font_family, boxh);
	calculate_string_dimensions("WW", &boxw, NULL);

	for(i = 0;i < nhints;i++)
		active_indices[i] = 1;

	label_pixmap	= create_label_pixmap(hints, nhints, bgcol, fgcol);
	fhwin		= create_fhwin(bgcol);
	win		= create_window(bgcol, 0, 0, sw, sh);

	window_move(win, -sw, -sh);
	window_move(fhwin, -sw, -sh);

	window_show(fhwin);
	window_show(win);

	hidewin(fhwin);
	hidewin(win);

	window_commit();
}
