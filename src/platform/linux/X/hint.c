/*
 * warpd - A modal keyboard-driven pointing system.
 *
 * © 2019 Raheman Vaiya (see: LICENSE).
 */

#include "X.h"

static int border_radius;
static const char *font_family;
static const char *fgcolor;
static const char *bgcolor;

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

	XftColorAllocValue(dpy, DefaultVisual(dpy, scr),
			   DefaultColormap(dpy, scr), &rc, &color);
	return color;
}

static XftFont *get_font(const char *name, int height)
{
	static XftFont *font;
	static char cached_name[256];
	static int cached_height;

	char xftname[256];
	int h;

	if (!strcmp(cached_name, name) && 
		    cached_height == height)
		return font;

	strcpy(cached_name, name);
	cached_height = height;

	h = height;
	do {
		snprintf(xftname, sizeof xftname, "%s:pixelsize=%d", name, h);
		font = XftFontOpenName(dpy, DefaultScreen(dpy), xftname);

		h--;
	} while (font->height > height);

	return font;
}

static int draw_text(Drawable drw, int x, int y, int w, int h,
		     const char *fontname, const char *s)
{
	XftDraw *xftdrw;
	XftColor col;

	XftFont *font;

	XGlyphInfo e;
	int font_height;

	font = get_font(fontname, h - 3);
	col = parse_xft_color(fgcolor);

	xftdrw = XftDrawCreate(dpy, drw, DefaultVisual(dpy, DefaultScreen(dpy)),
			       DefaultColormap(dpy, DefaultScreen(dpy)));

	XftTextExtentsUtf8(dpy, font, (FcChar8 *)s, strlen(s), &e);
	font_height = font->ascent + font->descent;

	x += (w - e.width) / 2;
	y += (h-font_height) / 2 + font->ascent;

	XftDrawStringUtf8(xftdrw, &col, font, x, y, (FcChar8 *)s,
			  strlen(s));

	return 0;
}

static void draw_rounded_rectangle(Drawable drw, GC gc, unsigned int x,
				   unsigned int y, unsigned int w,
				   unsigned int h, unsigned int r)
{

	XFillArc(dpy, drw, gc, x, y, 2 * r, 2 * r, 64 * 90 * 1, 64 * 90);
	XFillArc(dpy, drw, gc, x + w - 2 * r, y, 2 * r, 2 * r, 64 * 90 * 0,
		 64 * 90);
	XFillArc(dpy, drw, gc, x + w - 2 * r, y + h - 2 * r, 2 * r, 2 * r,
		 64 * 90 * 3, 64 * 90);
	XFillArc(dpy, drw, gc, x, y + h - 2 * r, 2 * r, 2 * r, 64 * 90 * 2,
		 64 * 90);

	XFillRectangle(dpy, drw, gc, x + r, y, w - 2 * r, h);
	XFillRectangle(dpy, drw, gc, x, y + r, w, h - 2 * r);
}

/* Draw the hints. */
void do_hint_draw(struct screen *scr, Window win, struct hint *hints, size_t n, Pixmap buf)
{
	size_t i = 0;

	Pixmap mask = XCreatePixmap(dpy, win, scr->w, scr->h, 1);
	GC gc = XCreateGC(dpy, mask, 0, NULL);
	GC mgc = XCreateGC(dpy, DefaultRootWindow(dpy),
			   GCForeground | GCFillStyle,
			   &(XGCValues){
			   .foreground = parse_xcolor(bgcolor, NULL),
			   .fill_style = FillSolid,
			   });

	XSetForeground(dpy, gc, 0);
	XFillRectangle(dpy, mask, gc, 0, 0, scr->w, scr->h);
	XSetForeground(dpy, gc, 1);

	XFillRectangle(dpy, buf, mgc, 0, 0, scr->w, scr->h);

	for (i = 0; i < n; i++) {
		struct hint *h = &hints[i];

		draw_rounded_rectangle(mask, gc, h->x, h->y, h->w, h->h,
				       border_radius);

		draw_text(buf, h->x, h->y,
			  h->w, h->h, font_family, h->label);
	}

	/* Expensive for large masks. */
	XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, mask, ShapeSet);

	XMoveWindow(dpy, win, scr->x, scr->y);
	XCopyArea(dpy, buf, win, mgc, 0, 0, scr->w, scr->h, 0, 0);
	XRaiseWindow(dpy, win);

	XFreePixmap(dpy, mask);
	XFreeGC(dpy, gc);
	XFreeGC(dpy, mgc);
}

void x_hint_draw(struct screen *scr, struct hint *hints, size_t n)
{
	Window win = scr->hintwin;
	Pixmap buf = scr->buf;

	XMoveWindow(dpy, scr->hintwin, -1E6, -1E6);
	XMoveWindow(dpy, scr->cached_hintwin, -1E6, -1E6);

	/* Use the cached window, if it exists. */
	if (n == scr->nr_cached_hints &&
	    !memcmp(scr->cached_hints, hints, sizeof(struct hint) * n)) {
		GC gc = XCreateGC(dpy, DefaultRootWindow(dpy),
				   GCForeground | GCFillStyle,
				   &(XGCValues){
				   .foreground = parse_xcolor(bgcolor, NULL),
				   .fill_style = FillSolid,
				   });

		XMoveWindow(dpy, scr->cached_hintwin, scr->x, scr->y);
		XCopyArea(dpy, scr->cached_hintbuf, scr->cached_hintwin,
			  gc, 0, 0, scr->w, scr->h, 0, 0);
		XRaiseWindow(dpy, scr->cached_hintwin);

		XFreeGC(dpy, gc);
		return;
	}

	/* 
	 * OPT: Cache large hint sets to avoid expensive shape calls. Note that
	 * using a single cached window makes assumptions about the 
	 * call pattern, namely that it will consist of intermittent
	 * large, and identical hint sets, with successive calls below
	 * the caching threshold.
	 */

	if (n > 50) {
		win = scr->cached_hintwin;
		buf = scr->cached_hintbuf;

		memcpy(scr->cached_hints, hints, n*sizeof(struct hint));
		scr->nr_cached_hints = n;
	}


	do_hint_draw(scr, win, hints, n, buf);
}

void x_init_hint(const char *bgcol, const char *fgcol, int _border_radius,
	       const char *_font_family)
{
	size_t i;

	bgcolor = bgcol;
	fgcolor = fgcol;
	border_radius = _border_radius;
	font_family = _font_family;


	for (i = 0; i < nr_xscreens; i++) {
		struct screen *scr = &xscreens[i];

		scr->hintwin = create_window(bgcol);
		scr->cached_hintwin = create_window(bgcol);

		scr->buf =
		    XCreatePixmap(dpy, DefaultRootWindow(dpy), scr->w, scr->h,
				  DefaultDepth(dpy, DefaultScreen(dpy)));
		scr->cached_hintbuf =
		    XCreatePixmap(dpy, DefaultRootWindow(dpy), scr->w, scr->h,
				  DefaultDepth(dpy, DefaultScreen(dpy)));


		XMoveResizeWindow(dpy, scr->hintwin, -1E6, -1E6, scr->w, scr->h);
		XMoveResizeWindow(dpy, scr->cached_hintwin, -1E6, -1E6, scr->w, scr->h);

		XMapWindow(dpy, scr->hintwin);
		XMapWindow(dpy, scr->cached_hintwin);
	}

}
